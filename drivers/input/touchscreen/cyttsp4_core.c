/*
 * cyttsp4_core.c
 * Cypress TrueTouch(TM) Standard Product V4 Core driver module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 * Modified by: Cypress Semiconductor to add device functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include <linux/cyttsp4_bus.h>

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <linux/cyttsp4_core.h>
#include "cyttsp4_regs.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define CY_CORE_MODE_CHANGE_TIMEOUT		1000
#define CY_CORE_RESET_AND_WAIT_TIMEOUT		500
#define CY_CORE_WAKEUP_TIMEOUT			500

MODULE_FIRMWARE(CY_FW_FILE_NAME);

const char *cy_driver_core_name = CYTTSP4_CORE_NAME;
const char *cy_driver_core_version = CY_DRIVER_VERSION;
const char *cy_driver_core_date = CY_DRIVER_DATE;

enum cyttsp4_sleep_state {
	SS_SLEEP_OFF,
	SS_SLEEP_ON,
	SS_SLEEPING,
	SS_WAKING,
};

enum cyttsp4_startup_state {
	STARTUP_NONE,
	STARTUP_QUEUED,
	STARTUP_RUNNING,
	STARTUP_ILLEGAL,
};

struct cyttsp4_core_data;

static int request_exclusive(struct cyttsp4_core_data *cd, void *ownptr, int t);
static int release_exclusive(struct cyttsp4_core_data *cd, void *ownptr);
static int set_mode(struct cyttsp4_core_data *cd, void *ownptr,
		u8 new_dev_mode, int new_op_mode);
static int cyttsp4_exec_cmd_(struct cyttsp4_core_data *cd, u8 mode,
		u8 *cmd_buf, size_t cmd_size, u8 *return_buf,
		size_t return_buf_size, int timeout);

#define SEC_TSP_FACTORY_TEST

#ifdef SEC_TSP_FACTORY_TEST

#define I2C_RETRY_CNT 2

#define NODE_X_NUM 16
#define NODE_Y_NUM 26

#define GLOBAL_IDAC_NUM	10
#define LOCAL_IDAC_START 1
#define GLOBAL_IDAC_START 0

#define MIN_VALUE 4640
#define MAX_VALUE 11040

#define NODE_TOTAL_NUM 209
#define ADC_TOP_VALUE 16384

#define TSP_BUF_SIZE 1024

#define TSP_CMD_STR_LEN 32
#define TSP_CMD_RESULT_STR_LEN 512
#define TSP_CMD_PARAM_NUM 8

#define NODE_NUM 416 /* 16 * 26 */
#define GLOBAL_IDAC_NUM	10

#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

u8 hw_id = 0x24;

struct tsp_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func) (void *device_data);
};


static int g_touch_pressed;
static int tsp_testmode;
static uint16_t gv_refrence_node[NODE_TOTAL_NUM];
static s16 gv_delta_node[NODE_TOTAL_NUM];

signed int raw_count[NODE_NUM] = {0,};
signed int difference[NODE_NUM] = {0,};
signed int local_idac[NODE_NUM] = {0,};
signed int global_idac[NODE_NUM] = {0,};


static void get_config_ver(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_threshold(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_reference(void *device_data);
static void get_delta(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void run_reference_read(void *device_data);
static void run_delta_read(void *device_data);
static void not_support_cmd(void *device_data);
static void get_raw_count(void *device_data);
static void get_difference(void *device_data);
static void get_local_idac(void *device_data);
static void get_global_idac(void *device_data);
static void run_raw_count_read(void *device_data);
static void run_difference_read(void *device_data);
static void run_global_idac_read(void *device_data);
static void run_local_idac_read(void *device_data);
static void fw_update(void *device_data);
#endif


struct cyttsp4_core_data {
	struct device *dev;
	struct cyttsp4_core *core;
	struct list_head atten_list[CY_ATTEN_NUM_ATTEN];
	struct mutex system_lock;
	struct mutex adap_lock;
	enum cyttsp4_mode mode;
	enum cyttsp4_sleep_state sleep_state;
	enum cyttsp4_startup_state startup_state;
	int int_status;
	int cmd_toggle;
	spinlock_t spinlock;
	struct cyttsp4_core_platform_data *pdata;
	wait_queue_head_t wait_q;
	wait_queue_head_t sleep_q;
	int irq;
	struct workqueue_struct *startup_work_q;
	struct work_struct startup_work;
	struct workqueue_struct *mode_change_work_q;
	struct work_struct mode_change_work;
	struct cyttsp4_sysinfo sysinfo;
	void *exclusive_dev;
	int exclusive_waits;
	atomic_t ignore_irq;
	bool irq_enabled;
	struct cyttsp4_device *loader;
	cyttsp4_loader_func loader_func;
#ifdef VERBOSE_DEBUG
	u8 pr_buf[CY_MAX_PRBUF_SIZE];
#endif
	struct work_struct work;
	struct timer_list timer;

#if defined(SEC_TSP_FACTORY_TEST)
	int threshold;
	struct list_head cmd_list_head;
	unsigned char cmd_state;
	char cmd[TSP_CMD_STR_LEN];
	int cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;
	bool ft_flag;
#endif
	bool startup_done;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

struct atten_node {
	struct list_head node;
	int (*func)(struct cyttsp4_device *);
	struct cyttsp4_device *ttsp;
	int mode;
};


#define SEC_TSP_INFO_SHOW

#ifdef SEC_TSP_INFO_SHOW
extern struct class *sec_class;
struct device *sec_touchscreen;
struct device *sec_touchkey;

static int led_status;

static u16 menu_sensitivity;
static u16 home_sensitivity;
static u16 back_sensitivity;

#ifndef MIN
#define	MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define	MAX(a, b)		(((a) > (b)) ? (a) : (b))
#endif

static void get_tsp_vendor(struct cyttsp4_core_data *cd, char *buf)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;

#ifdef SAMSUNG_SYSINFO_DATA
	if (si->ready && si->si_ptrs.samsung_data) {
		u8 ic_vendorh = si->si_ptrs.samsung_data->ic_vendorh;
		u8 ic_vendorl = si->si_ptrs.samsung_data->ic_vendorl;
		u8 hw_version = si->si_ptrs.samsung_data->hw_version;
		u8 fw_versionh = si->si_ptrs.samsung_data->fw_versionh;
		u8 fw_versionl = si->si_ptrs.samsung_data->fw_versionl;
		sprintf(buf, "%c%c%02X%02X%02X", ic_vendorh, ic_vendorl,
				hw_version, fw_versionh, fw_versionl);
		return;
	}
#endif
	sprintf(buf, "%c%c%02X%02X%02X", 'C', 'Y', 0, 0, 0);
}

static void get_tsp_config(struct cyttsp4_core_data *cd, char *buf)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;

#ifdef SAMSUNG_SYSINFO_DATA
	if (si->ready && si->si_ptrs.samsung_data) {
		u8 config_version = si->si_ptrs.samsung_data->config_version;
		sprintf(buf, "0x%02X", config_version);
		return;
	}
#endif
	sprintf(buf, "0x%02X", 0);
}

static ssize_t set_firm_update_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);
	int error = 0;
	int count = 0;
	pr_err("[TSP] set_firm_update_show start!!\n");

	return count;
}


static ssize_t set_firm_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);
	int count;
	pr_info("Enter firmware_status_show by Factory command\n");


	count = sprintf(buf, "PASS\n");

	return count;

}

#define THRESHOLD_SHOW

#ifdef THRESHOLD_SHOW
static ssize_t threshold_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);
	int ret;

	ret = sprintf(buf, "%d\n", 13);

	return ret;

	/* return sprintf(buf, "%u\n", data->threshold); */
}

static ssize_t threshold_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	/*TO DO IT */
	unsigned int object_register = 7;
	u8 value;
	u8 val;
	int ret;
	u16 address = 0;
	u16 size_one;
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);


	return size;
}
#endif

static ssize_t set_firm_version_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	char fw_latest_version[10];

	get_tsp_vendor(cd, fw_latest_version);

	pr_info("Cypress Last firmware version is %s\n", fw_latest_version);
	return sprintf(buf, "%s\n", fw_latest_version);
}

static ssize_t set_firm_version_read_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	char fw_version[10];

	get_tsp_vendor(cd, fw_version);
	/* it's connected to at_sec_handler */
	return sprintf(buf, "%s", fw_version);
}

static ssize_t set_config_version_read_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	char config_version[4];

	get_tsp_config(cd, config_version);
	return sprintf(buf, "%s", config_version);
}

static ssize_t tsp_touchtype_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);
	char temp[15];

	sprintf(temp, "TSP : TMA463\n");
	strcat(buf, temp);

	return strlen(buf);
}

static ssize_t touchkey_led_control(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	int ret;

	if (cd->sleep_state != SS_SLEEP_OFF) {
		printk(KERN_INFO "[TSP] %s - skip\n", __func__);
		return size;
	}

	if (buf && buf[0] == '1' && led_status == 0) {
		printk(KERN_INFO "[TSP] %s - on\n", __func__);
		ret = cd->pdata->led_power(1);
		if (ret < 0)
			pr_info("%s : Led on Fail\n", __func__);

		led_status = 1;

	} else if (buf && buf[0] == '2' && led_status == 1) {
		printk(KERN_INFO "[TSP] %s - off\n", __func__);
		ret = cd->pdata->led_power(0);
		if (ret < 0)
			pr_info("%s : Led off Fail\n", __func__);

		led_status = 0;
	}

	return size;
}

static ssize_t menu_sensitivity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);

	u8 command_buf[8], return_buf[16];
	uint16_t menu_sensitivity_tmp = 0;
	s16 local_menu_sensitivity = 0;
	int rc;

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

#if 0
	/* Initialize baselines */
	command_buf[0] = CY_CMD_CAT_INIT_BASELINES;
	command_buf[1] = 0x0F;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute initialize baselines command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: initialize baselines command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}
#endif

	/* Execute panel scan (Button raw counts) */
	command_buf[0] = CY_CMD_CAT_EXEC_PANEL_SCAN;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 1, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute execute panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: execute panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Retrieve panel scan (Button raw counts) */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_PANEL_SCAN;
	command_buf[1] = 0x00;
	command_buf[2] = 0x00;
	command_buf[3] = 0x00;
	command_buf[4] = 0x10;
	command_buf[5] = 0x09;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 6, return_buf, 11, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit:
	pm_runtime_put(cd->dev);

	if (rc)
		return 0;

	/* Processing Data */
	/* return_buf[5] LSB BTN0 Raw counts */
	/* return_buf[6] MSB BTN0 Raw counts */

	/* return_buf[9] LSB BTN0 Diff counts */
	/* return_buf[10] MSB BTN0 Diff counts */

	menu_sensitivity_tmp = return_buf[9];
	menu_sensitivity_tmp += return_buf[10] << 8;
	/* unsigned -> signed */
	local_menu_sensitivity = (s16)menu_sensitivity_tmp;

	return sprintf(buf, "%d\n", local_menu_sensitivity);
}

static ssize_t home_sensitivity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;
	u16 local_home_sensitivity = 200;

	if (home_sensitivity > 0) {
		local_home_sensitivity = home_sensitivity;
		home_sensitivity = 0;
		return sprintf(buf, "%d\n", local_home_sensitivity);
	}

	printk(KERN_INFO "[TSP] %s, home_sensitivity : %d!!!\n",
		__func__, local_home_sensitivity);

	if (ret < 0)
		return sprintf(buf, "%s\n", "fail to read home_sensitivity.");
	else
		return sprintf(buf, "%d\n", local_home_sensitivity);
}

static ssize_t back_sensitivity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);

	u8 command_buf[8], return_buf[24];
	uint16_t back_sensitivity_tmp = 0;
	s16 local_back_sensitivity = 0;
	int rc;

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

#if 0
	/* Initialize baselines */
	command_buf[0] = CY_CMD_CAT_INIT_BASELINES;
	command_buf[1] = 0x0F;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute initialize baselines command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: initialize baselines command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}
#endif

	/* Execute panel scan (Button raw counts) */
	command_buf[0] = CY_CMD_CAT_EXEC_PANEL_SCAN;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 1, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute execute panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: execute panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Retrieve panel scan (Button raw counts) */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_PANEL_SCAN;
	command_buf[1] = 0x00;
	command_buf[2] = 0x00;
	command_buf[3] = 0x00;
	command_buf[4] = 0x10;
	command_buf[5] = 0x09;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 6, return_buf, 19, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit:
	pm_runtime_put(cd->dev);

	if (rc)
		return 0;

	/* Processing Data */
	/* return_buf[13] LSB BTN1 Raw counts */
	/* return_buf[14] MSB BTN1 Raw counts */

	/* return_buf[17] LSB BTN1 Diff counts */
	/* return_buf[18] MSB BTN1 Diff counts */

	back_sensitivity_tmp = return_buf[17];
	back_sensitivity_tmp += return_buf[18] << 8;
	/* unsigned -> signed */
	local_back_sensitivity = (s16)back_sensitivity_tmp;

	return sprintf(buf, "%d\n", local_back_sensitivity);
}

static ssize_t touchkey_threshold_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct cyttsp4_core_data *data = dev_get_drvdata(dev);
	int ret;

	ret = sprintf(buf, "%d\n", 30);

	return ret;
}


static DEVICE_ATTR(tsp_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
	set_firm_update_show, NULL);/* firmware update */
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
	set_firm_status_show, NULL);/* firmware update status return */
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	threshold_show, threshold_store);/* threshold return, store */
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
	set_firm_version_show, NULL);	/* PHONE */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_firm_version_read_show, NULL);/*PART*/
static DEVICE_ATTR(tsp_firm_version_config, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_config_version_read_show, NULL);
 /*PART*/			/* TSP config last modifying date */
static DEVICE_ATTR(tsp_touchtype, S_IRUGO | S_IWUSR | S_IWGRP,
		   tsp_touchtype_show, NULL);

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			NULL, touchkey_led_control);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, menu_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_home, S_IRUGO, home_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
#endif


#ifdef SEC_TSP_FACTORY_TEST

struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("get_raw_count", get_raw_count),},   /* added */
	{TSP_CMD("get_difference", get_difference),},     /* added */
	{TSP_CMD("get_local_idac", get_local_idac),},	     /* added */
	{TSP_CMD("get_global_idac", get_global_idac),},  /* added */
	{TSP_CMD("get_delta", get_delta),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("run_raw_count_read", run_raw_count_read),},  /* added */
	{TSP_CMD("run_difference_read", run_difference_read),},      /* added */
	{TSP_CMD("run_global_idac_read", run_global_idac_read),},   /* added */
	{TSP_CMD("run_local_idac_read", run_local_idac_read),},	 /* added */
	{TSP_CMD("run_delta_read", run_delta_read),},
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};

#endif

#ifdef SEC_TSP_FACTORY_TEST
static void set_cmd_result(struct cyttsp4_core_data *ts, char *buff, int len)
{
	strncat(ts->cmd_result, buff, len);
}

static ssize_t show_close_tsp_test(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return snprintf(buf, TSP_BUF_SIZE, "%u\n", 0);
}

static void set_default_result(struct cyttsp4_core_data *ts)
{
	char delim = ':';

	memset(ts->cmd_result, 0x00, ARRAY_SIZE(ts->cmd_result));
	memcpy(ts->cmd_result, ts->cmd, strlen(ts->cmd));
	strncat(ts->cmd_result, &delim, 1);
}

static void not_support_cmd(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;
	char buff[16] = {0};

	set_default_result(ts);
	sprintf(buff, "%s", "NA");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 4;
	dev_info(ts->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	int hw_rev;

	set_default_result(cd);

	get_tsp_vendor(cd, buff);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));
	cd->cmd_state = 2;
	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
		buff, strnlen(buff, sizeof(buff)));

}

static void get_fw_ver_ic(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	int hw_rev;

	printk(KERN_INFO "[TSP] %s, %d\n", __func__, __LINE__);

	set_default_result(cd);

	get_tsp_vendor(cd, buff);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));
	cd->cmd_state = 2;
	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
		buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	char buff[16] = {0};

	printk(KERN_INFO "[TSP] %s, %d\n", __func__, __LINE__);

	set_default_result(cd);

	get_tsp_config(cd, buff);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));
	cd->cmd_state = 2;

	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
		buff, strnlen(buff, sizeof(buff)));

}

static void get_threshold(void *device_data)
{

	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;
	char buff[16] = {0};

	printk(KERN_INFO "[TSP] %s, %d\n", __func__, __LINE__);

	set_default_result(ts);

	snprintf(buff, sizeof(buff), "%d", ts->threshold);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_vendor(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};

	set_default_result(ts);

	snprintf(buff, sizeof(buff), "%s", "Cypress");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(ts->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};

	set_default_result(ts);

	snprintf(buff, sizeof(buff), "%s", "TMA463");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(ts->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

}

static int check_rx_tx_num(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[TSP_CMD_STR_LEN] = {0};
	int node;

	if (ts->cmd_param[0] < 0 ||
		ts->cmd_param[0] >= NODE_X_NUM ||
		ts->cmd_param[1] < 0 ||
		ts->cmd_param[1] >= NODE_Y_NUM) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = 3;

		dev_info(ts->dev, "%s: parameter error: %u,%u\n",
				__func__, ts->cmd_param[0],
				ts->cmd_param[1]);
		node = -1;
		return node;
	}
	node = ts->cmd_param[0] * NODE_Y_NUM + ts->cmd_param[1];
	dev_info(ts->dev, "%s: node = %d\n", __func__,
			node);
	return node;

}

static int global_value_check(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	return GLOBAL_IDAC_START;
}


static void get_reference(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(ts);
	node = check_rx_tx_num(ts);

	if (node < 0)
		return;

	val = gv_refrence_node[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));

}

static void get_difference(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	signed int val;
	int node;

	set_default_result(ts);
	node = check_rx_tx_num(ts);

	if (node < 0)
		return;

	val = difference[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_local_idac(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	signed int val;
	int node;

	set_default_result(ts);
	node = check_rx_tx_num(ts);

	if (node < 0)
		return;

	val = local_idac[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));

}


static void get_global_idac(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	signed int val;

	int x_channel_value = 0;

	set_default_result(ts);

	x_channel_value = global_value_check(ts);

	val = global_idac[x_channel_value];

	printk(KERN_INFO "[TSP] global_idac[%d]=%d\n", x_channel_value, val);

	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));

}

static void get_delta(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(ts);
	node = check_rx_tx_num(ts);

	if (node < 0)
		return;

	val = gv_delta_node[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));

}

static void get_x_num(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	int ver;

	set_default_result(ts);

	snprintf(buff, sizeof(buff), "%d", NODE_X_NUM);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(ts->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	int ver;

	printk(KERN_INFO "[TSP] %s, x channel=%d\n", __func__ , NODE_Y_NUM);

	set_default_result(ts);

	snprintf(buff, sizeof(buff), "%d", NODE_Y_NUM);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(ts->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void run_reference_read(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	not_support_cmd(ts);

}

static void get_raw_count(void *device_data)
{
	struct cyttsp4_core_data *ts = (struct cyttsp4_core_data *)device_data;

	char buff[16] = {0};
	signed int val;
	int node;

	set_default_result(ts);
	node = check_rx_tx_num(ts);

	if (node < 0)
		return;

	val = raw_count[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;

	dev_info(ts->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void run_raw_count_read(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	int col_num = NODE_Y_NUM;
	int row_num = NODE_X_NUM;
	int intersect_num = col_num * row_num;
	uint8_t command_buf[8], return_buf[NODE_NUM + 5];
	int8_t *raw_data = (int8_t *)&return_buf[5];
	char buff[TSP_CMD_STR_LEN] = {0};
	s32 max_value = 0, min_value = 0;
	int i, j, k;
	int rc;

	tsp_testmode = 1;

	set_default_result(cd);

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit_put;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

	/* Initialize baselines */
	command_buf[0] = CY_CMD_CAT_INIT_BASELINES;
	command_buf[1] = 0x0F;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute initialize baselines command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: initialize baselines command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Execute panel scan */
	command_buf[0] = CY_CMD_CAT_EXEC_PANEL_SCAN;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 1, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute execute panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: execute panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Retrieve panel scan (Screen raw counts) */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_PANEL_SCAN;
	command_buf[1] = 0x00; /* Offset Hi */
	command_buf[2] = 0x00; /* Offset Lo */
	command_buf[3] = (intersect_num >> 8) & 0xFF; /* Length Hi */
	command_buf[4] = intersect_num & 0xFF; /* Length Lo */
	command_buf[5] = 0x00; /* Raw Data */
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT, command_buf, 6, return_buf,
			intersect_num + 5, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit_put:
	pm_runtime_put(cd->dev);

	if (!rc) {
		min_value = max_value = raw_data[0];
		for (i = 0 ; i < intersect_num ; i++) {
			max_value = MAX(max_value, raw_data[i]);
			min_value = MIN(min_value, raw_data[i]);
			raw_count[i] = raw_data[i];
		}
	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));

	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

	tsp_testmode = 0;

	cd->cmd_state = 2;
}


static void run_difference_read(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	int col_num = NODE_Y_NUM;
	int row_num = NODE_X_NUM;
	int intersect_num = col_num * row_num;
	uint8_t command_buf[8], return_buf[NODE_NUM + 5];
	int8_t *diff_data = (int8_t *)&return_buf[5];
	char buff[TSP_CMD_STR_LEN] = {0};
	s32 max_value = 0, min_value = 0;
	int i, j, k;
	int rc;

	tsp_testmode = 1;

	set_default_result(cd);

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit_put;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

	/* Execute the panel scan */
	command_buf[0] = CY_CMD_CAT_EXEC_PANEL_SCAN;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 1, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute execute panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: execute panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Retrieve panel scan (Screen diff counts) */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_PANEL_SCAN;
	command_buf[1] = 0x00; /* Offset Hi */
	command_buf[2] = 0x00; /* Offset Lo */
	command_buf[3] = (intersect_num >> 8) & 0xFF; /* Length Hi */
	command_buf[4] = intersect_num & 0xFF; /* Length Lo */
	command_buf[5] = 0x02; /* Difference Data */
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT, command_buf, 6, return_buf,
			intersect_num + 5, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit_put:
	pm_runtime_put(cd->dev);

	if (!rc) {
		min_value = max_value = diff_data[0];
		for (i = 0 ; i < intersect_num ; i++) {
			max_value = MAX(max_value, diff_data[i]);
			min_value = MIN(min_value, diff_data[i]);
			difference[i] = diff_data[i];
		}
	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));

	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

	tsp_testmode = 0;

	cd->cmd_state = 2;
}

static void run_global_idac_read(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	uint8_t command_buf[8], return_buf[8];
	int8_t *diff_data = (int8_t *)&return_buf[5];
	char buff[TSP_CMD_STR_LEN] = {0};
	s32 max_value = 0, min_value = 0;
	int i, j, k;
	int rc;

	tsp_testmode = 1;

	set_default_result(cd);

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit_put;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

#ifdef SAMSUNG_PERFORM_CALIBRATION_BEFORE_LOCAL_IDAC_READ
	/* Calibrate IDACs for Screen */
	command_buf[0] = CY_CMD_CAT_CALIBRATE_IDACS;
	command_buf[1] = 0x00;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute calibrate idacs command for screen\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: calibrate idacs command for screen unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Calibrate IDACs for Buttons */
	command_buf[0] = CY_CMD_CAT_CALIBRATE_IDACS;
	command_buf[1] = 0x01;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute calibrate idacs command for buttons\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: calibrate idacs command for buttons unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}
#endif

	/* Retrieve data for IDACs */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_DATA_STRUCTURE;
	command_buf[1] = 0x00; /* Offset Hi */
	command_buf[2] = 0x00; /* Offset Lo */
	command_buf[3] = 0x00; /* Length Hi */
	command_buf[4] = 0x01; /* Length Lo */
	command_buf[5] = 0x00; /* Mutual IDAC Data */
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 6, return_buf, 6, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit_put:
	pm_runtime_put(cd->dev);

	if (!rc) {
		global_idac[0] = return_buf[5];

		snprintf(buff, sizeof(buff), "%d,%d",
			global_idac[0], global_idac[0]);
	}

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));

	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

	tsp_testmode = 0;

	cd->cmd_state = 2;
}

static void run_local_idac_read(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	int col_num = NODE_Y_NUM;
	int row_num = NODE_X_NUM;
	int intersect_num = col_num * row_num;
	uint8_t command_buf[8], return_buf[NODE_NUM + 5];
	int8_t *local_idac_data = (int8_t *)&return_buf[5];
	char buff[TSP_CMD_STR_LEN] = {0};
	s32 max_value = 0, min_value = 0;
	int i, j, k;
	int rc;

	tsp_testmode = 1;

	set_default_result(cd);

	pm_runtime_get_sync(cd->dev);

	/* Request exclusive access */
	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit_put;
	}

	/* Switch to CAT mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_CAT, CY_MODE_CAT);
	rc = set_mode(cd, cd->core, CY_HST_CAT, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail set mode to CAT\n", __func__);
		goto exit_release;
	}

#ifdef SAMSUNG_PERFORM_CALIBRATION_BEFORE_LOCAL_IDAC_READ
	/* Calibrate IDACs for Screen */
	command_buf[0] = CY_CMD_CAT_CALIBRATE_IDACS;
	command_buf[1] = 0x00;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute calibrate idacs command for screen\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: calibrate idacs command for screen unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

	/* Calibrate IDACs for Buttons */
	command_buf[0] = CY_CMD_CAT_CALIBRATE_IDACS;
	command_buf[1] = 0x01;
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT,
			command_buf, 2, return_buf, 1, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute calibrate idacs command for buttons\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: calibrate idacs command for buttons unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}
#endif

	/* Retrieve data for IDACs */
	command_buf[0] = CY_CMD_CAT_RETRIEVE_DATA_STRUCTURE;
	command_buf[1] = 0x00; /* Offset Hi */
	command_buf[2] = 0x01; /* Offset Lo */
	command_buf[3] = (intersect_num >> 8) & 0xFF; /* Length Hi */
	command_buf[4] = intersect_num & 0xFF; /* Length Lo */
	command_buf[5] = 0x00; /* Mutual IDAC Data */
	rc = cyttsp4_exec_cmd_(cd, CY_MODE_CAT, command_buf, 6, return_buf,
			intersect_num + 5, 500);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail to execute retrieve panel scan command\n",
			__func__);
		goto exit_switch;
	}
	if (return_buf[0] != 0) {
		dev_err(cd->dev, "%s: retrieve panel scan command unsuccessful\n",
			__func__);
		rc = -EINVAL;
		goto exit_switch;
	}

exit_switch:
	/* Switch to Operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	rc = set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set mode to Operational\n",
			__func__);

exit_release:
	/* Release exclusive access */
	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit_put:
	pm_runtime_put(cd->dev);

	if (!rc) {
		min_value = max_value = local_idac_data[0];
		for (i = 0 ; i < intersect_num ; i++) {
			max_value = MAX(max_value, local_idac_data[i]);
			min_value = MIN(min_value, local_idac_data[i]);
			local_idac[i] = local_idac_data[i];
		}
	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);

	set_cmd_result(cd, buff, strnlen(buff, sizeof(buff)));

	dev_info(cd->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

	tsp_testmode = 0;

	cd->cmd_state = 2;
}

static void run_delta_read(void *device_data)
{
	struct cyttsp4 *ts = (struct cyttsp4 *)device_data;

	not_support_cmd(ts);
}

static void fw_update(void *device_data)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)device_data;
	cyttsp4_loader_func loader_func;
	struct cyttsp4_device *loader;
	struct cyttsp4_touch_firmware *fw;

	set_default_result(cd);

	printk(KERN_INFO "fw_update !!!!!!!!!!!!!!\n");

	mutex_lock(&cd->system_lock);
	loader = cd->loader;
	loader_func = cd->loader_func;
	fw = cd->pdata->fw;
	mutex_unlock(&cd->system_lock);

	if (!loader || !loader_func)
		dev_err(cd->dev, "%s: No loader found for core.\n", __func__);
	else if (!fw)
		dev_err(cd->dev, "%s: No built-in fw found.\n", __func__);
	else
		loader_func(loader, fw);

	cd->cmd_state = 2;
}


static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct cyttsp4_core_data *ts = dev_get_drvdata(dev);

	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (ts->cmd_is_running == true) {
		dev_err(ts->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}


	/* check lock  */
	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = true;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = 1;

	for (i = 0; i < ARRAY_SIZE(ts->cmd_param); i++)
		ts->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(ts->cmd, 0x00, ARRAY_SIZE(ts->cmd));
	memcpy(ts->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &ts->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &ts->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				ret = kstrtoint(buff, 10,\
						ts->cmd_param + param_cnt);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}

	dev_info(ts->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(ts->dev, "cmd param %d= %d\n", i,
							ts->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(ts);


err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct cyttsp4_core_data *ts = dev_get_drvdata(dev);
	char buff[16] = {0};

	dev_info(ts->dev, "tsp cmd: status:%d\n",
			ts->cmd_state);

	if (ts->cmd_state == 0)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (ts->cmd_state == 1)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (ts->cmd_state == 2)
		snprintf(buff, sizeof(buff), "OK");

	else if (ts->cmd_state == 3)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (ts->cmd_state == 4)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct cyttsp4_core_data *ts = dev_get_drvdata(dev);

	dev_info(ts->dev, "tsp cmd: result: %s\n", ts->cmd_result);

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = 0;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", ts->cmd_result);
}

static DEVICE_ATTR(close_tsp_test, S_IRUGO, show_close_tsp_test, NULL);
static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);


static struct attribute *sec_touch_facotry_attributes[] = {
		&dev_attr_close_tsp_test.attr,
		&dev_attr_cmd.attr,
		&dev_attr_cmd_status.attr,
		&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif /* SEC_TSP_FACTORY_TEST */

static inline size_t merge_bytes(u8 high, u8 low)
{
	return (high << 8) + low;
}

#ifdef VERBOSE_DEBUG
void cyttsp4_pr_buf(struct device *dev, u8 *pr_buf, u8 *dptr, int size,
		const char *data_name)
{
	int i, k;
	const char fmt[] = "%02X ";
	int max;

	if (!size)
		return;

	max = (CY_MAX_PRBUF_SIZE - 1) - sizeof(CY_PR_TRUNCATED);

	pr_buf[0] = 0;
	for (i = k = 0; i < size && k < max; i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, dptr[i]);

	dev_vdbg(dev, "%s:  %s[0..%d]=%s%s\n", __func__, data_name, size - 1,
			pr_buf, size <= max ? "" : CY_PR_TRUNCATED);
}
EXPORT_SYMBOL(cyttsp4_pr_buf);
#endif

static int cyttsp4_load_status_regs(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	struct device *dev = cd->dev;
	int rc;

	if (!si->xy_mode) {
		dev_err(cd->dev, "%s: NULL xy_mode pointer\n", __func__);
		return -EINVAL;
	}

	rc = cyttsp4_adap_read(cd->core->adap, CY_REG_BASE,
		si->xy_mode, si->si_ofs.mode_size);
	if (rc < 0)
		dev_err(dev, "%s: fail read mode regs r=%d\n",
			__func__, rc);
	else
		cyttsp4_pr_buf(dev, cd->pr_buf, si->xy_mode,
			si->si_ofs.mode_size, "xy_mode");

	return rc;
}

static int cyttsp4_handshake(struct cyttsp4_core_data *cd, u8 mode)
{
	u8 cmd = mode ^ CY_HST_TOGGLE;
	int rc;

	if (mode & CY_HST_MODE_CHANGE) {
		dev_err(cd->dev, "%s: Host mode change bit set, NO handshake\n",
				__func__);
		return 0;
	}

	rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE, &cmd,
			sizeof(cmd));

	if (rc < 0)
		dev_err(cd->dev, "%s: bus write fail on handshake (ret=%d)\n",
				__func__, rc);

	return rc;
}

static int cyttsp4_toggle_low_power(struct cyttsp4_core_data *cd, u8 mode)
{
	u8 cmd = mode ^ CY_HST_LOWPOW;
	int rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE, &cmd,
			sizeof(cmd));
	if (rc < 0)
		dev_err(cd->dev,
			"%s: bus write fail on toggle low power (ret=%d)\n",
			__func__, rc);
	return rc;
}

static int cyttsp4_hw_soft_reset(struct cyttsp4_core_data *cd)
{
	u8 cmd = CY_HST_RESET | CY_HST_MODE_CHANGE;
	int rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE, &cmd,
			sizeof(cmd));
	if (rc < 0) {
		dev_err(cd->dev, "%s: FAILED to execute SOFT reset\n",
				__func__);
		return rc;
	}
	dev_dbg(cd->dev, "%s: execute SOFT reset\n", __func__);
	return 0;
}

static int cyttsp4_hw_hard_reset(struct cyttsp4_core_data *cd)
{
	if (cd->pdata->xres) {
		cd->pdata->xres(cd->pdata, cd->dev);
		dev_dbg(cd->dev, "%s: execute HARD reset\n", __func__);
		return 0;
	}
	dev_err(cd->dev, "%s: FAILED to execute HARD reset\n", __func__);
	return -ENOSYS;
}

static int cyttsp4_hw_reset(struct cyttsp4_core_data *cd)
{
	int rc = cyttsp4_hw_hard_reset(cd);
	if (rc == -ENOSYS)
		rc = cyttsp4_hw_soft_reset(cd);
	return rc;
}

static inline int cyttsp4_bits_2_bytes(int nbits, int *max)
{
	*max = 1 << nbits;
	return (nbits + 7) / 8;
}

static int cyttsp4_si_data_offsets(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc = cyttsp4_adap_read(cd->core->adap, CY_REG_BASE, &si->si_data,
				   sizeof(si->si_data));
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail read sysinfo data offsets r=%d\n",
			__func__, rc);
		return rc;
	}

	/* Print sysinfo data offsets */
	cyttsp4_pr_buf(cd->dev, cd->pr_buf, (u8 *)&si->si_data,
		       sizeof(si->si_data), "sysinfo_data_offsets");

	/* convert sysinfo data offset bytes into integers */

	si->si_ofs.map_sz = merge_bytes(si->si_data.map_szh,
			si->si_data.map_szl);
	si->si_ofs.map_sz = merge_bytes(si->si_data.map_szh,
			si->si_data.map_szl);
	si->si_ofs.cydata_ofs = merge_bytes(si->si_data.cydata_ofsh,
			si->si_data.cydata_ofsl);
	si->si_ofs.test_ofs = merge_bytes(si->si_data.test_ofsh,
			si->si_data.test_ofsl);
	si->si_ofs.pcfg_ofs = merge_bytes(si->si_data.pcfg_ofsh,
			si->si_data.pcfg_ofsl);
	si->si_ofs.opcfg_ofs = merge_bytes(si->si_data.opcfg_ofsh,
			si->si_data.opcfg_ofsl);
	si->si_ofs.ddata_ofs = merge_bytes(si->si_data.ddata_ofsh,
			si->si_data.ddata_ofsl);
	si->si_ofs.mdata_ofs = merge_bytes(si->si_data.mdata_ofsh,
			si->si_data.mdata_ofsl);
	return rc;
}

static int cyttsp4_si_get_cydata(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;
	int size;
	u8 *buf;
	u8 *p;
	struct cyttsp4_cydata *cydata;

	/* Allocate a temp buffer for reading CYDATA registers */
	si->si_ofs.cydata_size = si->si_ofs.test_ofs - si->si_ofs.cydata_ofs;
	dev_dbg(cd->dev, "%s: cydata size: %d\n", __func__,
			si->si_ofs.cydata_size);
	buf = kzalloc(si->si_ofs.cydata_size, GFP_KERNEL);
	if (buf == NULL) {
		dev_err(cd->dev, "%s: fail alloc buffer for reading cydata\n",
			__func__);
		return -ENOMEM;
	}

	/* Read the CYDA registers to the temp buf */
	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.cydata_ofs,
				buf, si->si_ofs.cydata_size);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail read cydata r=%d\n",
				__func__, rc);
		goto free_buf;
	}

	/* Allocate local cydata structure */
	if (si->si_ptrs.cydata == NULL)
		si->si_ptrs.cydata = kzalloc(sizeof(struct cyttsp4_cydata),
					GFP_KERNEL);
	if (si->si_ptrs.cydata == NULL) {
		dev_err(cd->dev, "%s: fail alloc cydata memory\n", __func__);
		rc = -ENOMEM;
		goto free_buf;
	}

	cydata = (struct cyttsp4_cydata *)buf;

	/* Allocate MFGID memory */
	if (si->si_ptrs.cydata->mfg_id == NULL)
		si->si_ptrs.cydata->mfg_id = kzalloc(cydata->mfgid_sz,
			GFP_KERNEL);
	if (si->si_ptrs.cydata->mfg_id == NULL) {
		kfree(si->si_ptrs.cydata);
		si->si_ptrs.cydata = NULL;
		dev_err(cd->dev, "%s: fail alloc mfgid memory\n", __func__);
		rc = -ENOMEM;
		goto free_buf;
	}

	/* Copy all fields up to MFGID to local cydata structure */
	p = buf;
	size = offsetof(struct cyttsp4_cydata, mfgid_sz) + 1;
	memcpy(si->si_ptrs.cydata, p, size);

	cyttsp4_pr_buf(cd->dev, cd->pr_buf, (u8 *)si->si_ptrs.cydata,
			size, "sysinfo_cydata");

	/* Copy MFGID */
	p += size;
	memcpy(si->si_ptrs.cydata->mfg_id, p, si->si_ptrs.cydata->mfgid_sz);

	cyttsp4_pr_buf(cd->dev, cd->pr_buf, (u8 *)si->si_ptrs.cydata->mfg_id,
			si->si_ptrs.cydata->mfgid_sz, "sysinfo_cydata mfgid");

	/* Copy remaining registers after MFGID */
	p += si->si_ptrs.cydata->mfgid_sz;
	size = sizeof(struct cyttsp4_cydata) -
			offsetof(struct cyttsp4_cydata, cyito_idh) - 1;
	memcpy((u8 *)&si->si_ptrs.cydata->cyito_idh, p, size);

	cyttsp4_pr_buf(cd->dev, cd->pr_buf, &si->si_ptrs.cydata->cyito_idh,
			size, "sysinfo_cydata");

free_buf:
	kfree(buf);
	return rc;
}

static int cyttsp4_si_get_test_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	si->si_ofs.test_size = si->si_ofs.pcfg_ofs - si->si_ofs.test_ofs;
	if (si->si_ptrs.test == NULL)
		si->si_ptrs.test = kzalloc(si->si_ofs.test_size, GFP_KERNEL);
	if (si->si_ptrs.test == NULL) {
		dev_err(cd->dev, "%s: fail alloc test memory\n", __func__);
		return -ENOMEM;
	}

	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.test_ofs,
		si->si_ptrs.test, si->si_ofs.test_size);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail read test data r=%d\n",
			__func__, rc);
		return rc;
	}

	cyttsp4_pr_buf(cd->dev, cd->pr_buf,
		       (u8 *)si->si_ptrs.test, si->si_ofs.test_size,
		       "sysinfo_test_data");
	if (si->si_ptrs.test->post_codel &
	    CY_POST_CODEL_WDG_RST)
		dev_info(cd->dev, "%s: %s codel=%02X\n",
			 __func__, "Reset was a WATCHDOG RESET",
			 si->si_ptrs.test->post_codel);

	if (!(si->si_ptrs.test->post_codel &
	      CY_POST_CODEL_CFG_DATA_CRC_FAIL))
		dev_info(cd->dev, "%s: %s codel=%02X\n", __func__,
			 "Config Data CRC FAIL",
			 si->si_ptrs.test->post_codel);

	if (!(si->si_ptrs.test->post_codel &
	      CY_POST_CODEL_PANEL_TEST_FAIL))
		dev_info(cd->dev, "%s: %s codel=%02X\n",
			 __func__, "PANEL TEST FAIL",
			 si->si_ptrs.test->post_codel);

	dev_info(cd->dev, "%s: SCANNING is %s codel=%02X\n",
		 __func__, si->si_ptrs.test->post_codel & 0x08 ?
		 "ENABLED" : "DISABLED",
		 si->si_ptrs.test->post_codel);
	return rc;
}

static int cyttsp4_si_get_pcfg_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	dev_vdbg(cd->dev, "%s: get pcfg data\n", __func__);
	si->si_ofs.pcfg_size = si->si_ofs.opcfg_ofs - si->si_ofs.pcfg_ofs;
	if (si->si_ptrs.pcfg == NULL)
		si->si_ptrs.pcfg = kzalloc(si->si_ofs.pcfg_size, GFP_KERNEL);
	if (si->si_ptrs.pcfg == NULL) {
		rc = -ENOMEM;
		dev_err(cd->dev, "%s: fail alloc pcfg memory r=%d\n",
			__func__, rc);
		return rc;
	}
	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.pcfg_ofs,
			       si->si_ptrs.pcfg, si->si_ofs.pcfg_size);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail read pcfg data r=%d\n",
			__func__, rc);
		return rc;
	}

	si->si_ofs.max_x = merge_bytes((si->si_ptrs.pcfg->res_xh
			& CY_PCFG_RESOLUTION_X_MASK), si->si_ptrs.pcfg->res_xl);
	si->si_ofs.x_origin = !!(si->si_ptrs.pcfg->res_xh
			& CY_PCFG_ORIGIN_X_MASK);
	si->si_ofs.max_y = merge_bytes((si->si_ptrs.pcfg->res_yh
			& CY_PCFG_RESOLUTION_Y_MASK), si->si_ptrs.pcfg->res_yl);
	si->si_ofs.y_origin = !!(si->si_ptrs.pcfg->res_yh
			& CY_PCFG_ORIGIN_Y_MASK);
	si->si_ofs.max_p = merge_bytes(si->si_ptrs.pcfg->max_zh,
			si->si_ptrs.pcfg->max_zl);

	cyttsp4_pr_buf(cd->dev, cd->pr_buf,
		       (u8 *)si->si_ptrs.pcfg,
		       si->si_ofs.pcfg_size, "sysinfo_pcfg_data");
	return rc;
}

static int cyttsp4_si_get_opcfg_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int i;
	enum cyttsp4_tch_abs abs;
	int rc;

	dev_vdbg(cd->dev, "%s: get opcfg data\n", __func__);
	si->si_ofs.opcfg_size = si->si_ofs.ddata_ofs - si->si_ofs.opcfg_ofs;
	if (si->si_ptrs.opcfg == NULL)
		si->si_ptrs.opcfg = kzalloc(si->si_ofs.opcfg_size, GFP_KERNEL);
	if (si->si_ptrs.opcfg == NULL) {
		dev_err(cd->dev, "%s: fail alloc opcfg memory\n", __func__);
		rc = -ENOMEM;
		goto cyttsp4_si_get_opcfg_data_exit;
	}

	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.opcfg_ofs,
		si->si_ptrs.opcfg, si->si_ofs.opcfg_size);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail read opcfg data r=%d\n",
			__func__, rc);
		goto cyttsp4_si_get_opcfg_data_exit;
	}
	si->si_ofs.cmd_ofs = si->si_ptrs.opcfg->cmd_ofs;
	si->si_ofs.rep_ofs = si->si_ptrs.opcfg->rep_ofs;
	si->si_ofs.rep_sz = (si->si_ptrs.opcfg->rep_szh * 256) +
		si->si_ptrs.opcfg->rep_szl;
	si->si_ofs.num_btns = si->si_ptrs.opcfg->num_btns;
	si->si_ofs.num_btn_regs = (si->si_ofs.num_btns +
		CY_NUM_BTN_PER_REG - 1) / CY_NUM_BTN_PER_REG;
	si->si_ofs.tt_stat_ofs = si->si_ptrs.opcfg->tt_stat_ofs;
	si->si_ofs.obj_cfg0 = si->si_ptrs.opcfg->obj_cfg0;
	si->si_ofs.max_tchs = si->si_ptrs.opcfg->max_tchs &
		CY_BYTE_OFS_MASK;
	si->si_ofs.tch_rec_size = si->si_ptrs.opcfg->tch_rec_size &
		CY_BYTE_OFS_MASK;

	/* Get the old touch fields */
	for (abs = CY_TCH_X; abs < CY_NUM_TCH_FIELDS; abs++) {
		si->si_ofs.tch_abs[abs].ofs =
			si->si_ptrs.opcfg->tch_rec_old[abs].loc &
			CY_BYTE_OFS_MASK;
		si->si_ofs.tch_abs[abs].size =
			cyttsp4_bits_2_bytes
			(si->si_ptrs.opcfg->tch_rec_old[abs].size,
			&si->si_ofs.tch_abs[abs].max);
		si->si_ofs.tch_abs[abs].bofs =
			(si->si_ptrs.opcfg->tch_rec_old[abs].loc &
			CY_BOFS_MASK) >> CY_BOFS_SHIFT;
	}

	/* button fields */
	si->si_ofs.btn_rec_size = si->si_ptrs.opcfg->btn_rec_size;
	si->si_ofs.btn_diff_ofs = si->si_ptrs.opcfg->btn_diff_ofs;
	si->si_ofs.btn_diff_size = si->si_ptrs.opcfg->btn_diff_size;

	if (si->si_ofs.tch_rec_size > CY_TMA1036_TCH_REC_SIZE) {
		/* Get the extended touch fields */
		for (i = 0; i < CY_NUM_EXT_TCH_FIELDS; abs++, i++) {
			si->si_ofs.tch_abs[abs].ofs =
				si->si_ptrs.opcfg->tch_rec_new[i].loc &
				CY_BYTE_OFS_MASK;
			si->si_ofs.tch_abs[abs].size =
				cyttsp4_bits_2_bytes
				(si->si_ptrs.opcfg->tch_rec_new[i].size,
				&si->si_ofs.tch_abs[abs].max);
			si->si_ofs.tch_abs[abs].bofs =
				(si->si_ptrs.opcfg->tch_rec_new[i].loc
				& CY_BOFS_MASK) >> CY_BOFS_SHIFT;
		}
	}

	for (abs = 0; abs < CY_TCH_NUM_ABS; abs++) {
		dev_dbg(cd->dev, "%s: tch_rec_%s\n", __func__,
			cyttsp4_tch_abs_string[abs]);
		dev_dbg(cd->dev, "%s:     ofs =%2d\n", __func__,
			si->si_ofs.tch_abs[abs].ofs);
		dev_dbg(cd->dev, "%s:     siz =%2d\n", __func__,
			si->si_ofs.tch_abs[abs].size);
		dev_dbg(cd->dev, "%s:     max =%2d\n", __func__,
			si->si_ofs.tch_abs[abs].max);
		dev_dbg(cd->dev, "%s:     bofs=%2d\n", __func__,
			si->si_ofs.tch_abs[abs].bofs);
	}

	si->si_ofs.mode_size = si->si_ofs.tt_stat_ofs + 1;
	si->si_ofs.data_size = si->si_ofs.max_tchs *
		si->si_ptrs.opcfg->tch_rec_size;

	cyttsp4_pr_buf(cd->dev, cd->pr_buf, (u8 *)si->si_ptrs.opcfg,
		si->si_ofs.opcfg_size, "sysinfo_opcfg_data");

cyttsp4_si_get_opcfg_data_exit:
	return rc;
}

static int cyttsp4_si_get_ddata(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	dev_vdbg(cd->dev, "%s: get ddata data\n", __func__);
	si->si_ofs.ddata_size = si->si_ofs.mdata_ofs - si->si_ofs.ddata_ofs;
	if (si->si_ptrs.ddata == NULL)
		si->si_ptrs.ddata = kzalloc(si->si_ofs.ddata_size, GFP_KERNEL);
	if (si->si_ptrs.ddata == NULL) {
		dev_err(cd->dev, "%s: fail alloc ddata memory\n", __func__);
		return -ENOMEM;
	}

	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.ddata_ofs,
			       si->si_ptrs.ddata, si->si_ofs.ddata_size);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail read ddata data r=%d\n",
			__func__, rc);
	else
		cyttsp4_pr_buf(cd->dev, cd->pr_buf,
			       (u8 *)si->si_ptrs.ddata,
			       si->si_ofs.ddata_size, "sysinfo_ddata");
	return rc;
}

static int cyttsp4_si_get_mdata(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	dev_vdbg(cd->dev, "%s: get mdata data\n", __func__);
	si->si_ofs.mdata_size = si->si_ofs.map_sz - si->si_ofs.mdata_ofs;
	if (si->si_ptrs.mdata == NULL)
		si->si_ptrs.mdata = kzalloc(si->si_ofs.mdata_size, GFP_KERNEL);
	if (si->si_ptrs.mdata == NULL) {
		dev_err(cd->dev, "%s: fail alloc mdata memory\n", __func__);
		return -ENOMEM;
	}

	rc = cyttsp4_adap_read(cd->core->adap, si->si_ofs.mdata_ofs,
			       si->si_ptrs.mdata, si->si_ofs.mdata_size);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail read mdata data r=%d\n",
			__func__, rc);
	else
		cyttsp4_pr_buf(cd->dev, cd->pr_buf,
			       (u8 *)si->si_ptrs.mdata,
			       si->si_ofs.mdata_size, "sysinfo_mdata");
	return rc;
}

static int cyttsp4_si_get_btn_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int btn;
	int num_defined_keys;
	u16 *key_table;
	int rc = 0;

	dev_vdbg(cd->dev, "%s: get btn data\n", __func__);
	if (si->si_ofs.num_btns) {
		si->si_ofs.btn_keys_size = si->si_ofs.num_btns *
			sizeof(struct cyttsp4_btn);
		if (si->btn == NULL)
			si->btn = kzalloc(si->si_ofs.btn_keys_size, GFP_KERNEL);
		if (si->btn == NULL) {
			dev_err(cd->dev, "%s: %s\n", __func__,
				"fail alloc btn_keys memory");
			return -ENOMEM;
		}
		if (cd->pdata->sett[CY_IC_GRPNUM_BTN_KEYS] == NULL)
			num_defined_keys = 0;
		else if (cd->pdata->sett[CY_IC_GRPNUM_BTN_KEYS]->data == NULL)
			num_defined_keys = 0;
		else
			num_defined_keys = cd->pdata->sett
				[CY_IC_GRPNUM_BTN_KEYS]->size;

		for (btn = 0; btn < si->si_ofs.num_btns &&
			btn < num_defined_keys; btn++) {
			key_table = (u16 *)cd->pdata->sett
				[CY_IC_GRPNUM_BTN_KEYS]->data;
			si->btn[btn].key_code = key_table[btn];
			si->btn[btn].enabled = true;
		}
		for (; btn < si->si_ofs.num_btns; btn++) {
			si->btn[btn].key_code = KEY_RESERVED;
			si->btn[btn].enabled = true;
		}

		return rc;
	}

	si->si_ofs.btn_keys_size = 0;
	kfree(si->btn);
	si->btn = NULL;
	return rc;
}

#ifdef SAMSUNG_SYSINFO_DATA
static int cyttsp4_si_get_samsung_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	dev_vdbg(cd->dev, "%s: get Samsung data\n", __func__);
	si->si_ofs.samsung_data_size = sizeof(struct cyttsp4_samsung_data);
	if (si->si_ptrs.samsung_data == NULL)
		si->si_ptrs.samsung_data =
			kzalloc(si->si_ofs.samsung_data_size, GFP_KERNEL);
	if (si->si_ptrs.samsung_data == NULL) {
		dev_err(cd->dev, "%s: fail alloc Samsung data memory\n",
			__func__);
		return -ENOMEM;
	}

	rc = cyttsp4_adap_read(cd->core->adap, SAMSUNG_DATA_OFFSET,
					si->si_ptrs.samsung_data,
					si->si_ofs.samsung_data_size);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail read Samsung data r=%d\n",
			__func__, rc);
	else
		cyttsp4_pr_buf(cd->dev, cd->pr_buf,
					(u8 *)si->si_ptrs.samsung_data,
					si->si_ofs.samsung_data_size,
					"Samsung data");
	return rc;
}
#endif

static int cyttsp4_si_get_op_data_ptrs(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	if (si->xy_mode == NULL) {
		si->xy_mode = kzalloc(si->si_ofs.mode_size, GFP_KERNEL);
		if (si->xy_mode == NULL)
			return -ENOMEM;
	}

	if (si->xy_data == NULL) {
		si->xy_data = kzalloc(si->si_ofs.data_size, GFP_KERNEL);
		if (si->xy_data == NULL)
			return -ENOMEM;
	}

	if (si->btn_rec_data == NULL) {
		si->btn_rec_data = kzalloc(si->si_ofs.btn_rec_size *
					   si->si_ofs.num_btns, GFP_KERNEL);
		if (si->btn_rec_data == NULL)
			return -ENOMEM;
	}
#ifdef SHOK_SENSOR_DATA_MODE
	/* initialize */
	si->monitor.mntr_status = CY_MNTR_DISABLED;
	memset(si->monitor.sensor_data, 0, sizeof(si->monitor.sensor_data));
#endif
	return 0;
}

static void cyttsp4_si_put_log_data(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	dev_dbg(cd->dev, "%s: cydata_ofs =%4d siz=%4d\n", __func__,
		si->si_ofs.cydata_ofs, si->si_ofs.cydata_size);
	dev_dbg(cd->dev, "%s: test_ofs   =%4d siz=%4d\n", __func__,
		si->si_ofs.test_ofs, si->si_ofs.test_size);
	dev_dbg(cd->dev, "%s: pcfg_ofs   =%4d siz=%4d\n", __func__,
		si->si_ofs.pcfg_ofs, si->si_ofs.pcfg_size);
	dev_dbg(cd->dev, "%s: opcfg_ofs  =%4d siz=%4d\n", __func__,
		si->si_ofs.opcfg_ofs, si->si_ofs.opcfg_size);
	dev_dbg(cd->dev, "%s: ddata_ofs  =%4d siz=%4d\n", __func__,
		si->si_ofs.ddata_ofs, si->si_ofs.ddata_size);
	dev_dbg(cd->dev, "%s: mdata_ofs  =%4d siz=%4d\n", __func__,
		si->si_ofs.mdata_ofs, si->si_ofs.mdata_size);

	dev_dbg(cd->dev, "%s: cmd_ofs       =%4d\n", __func__,
		si->si_ofs.cmd_ofs);
	dev_dbg(cd->dev, "%s: rep_ofs       =%4d\n", __func__,
		si->si_ofs.rep_ofs);
	dev_dbg(cd->dev, "%s: rep_sz        =%4d\n", __func__,
		si->si_ofs.rep_sz);
	dev_dbg(cd->dev, "%s: num_btns      =%4d\n", __func__,
		si->si_ofs.num_btns);
	dev_dbg(cd->dev, "%s: num_btn_regs  =%4d\n", __func__,
		si->si_ofs.num_btn_regs);
	dev_dbg(cd->dev, "%s: tt_stat_ofs   =%4d\n", __func__,
		si->si_ofs.tt_stat_ofs);
	dev_dbg(cd->dev, "%s: tch_rec_size   =%4d\n", __func__,
		si->si_ofs.tch_rec_size);
	dev_dbg(cd->dev, "%s: max_tchs      =%4d\n", __func__,
		si->si_ofs.max_tchs);
	dev_dbg(cd->dev, "%s: mode_size     =%4d\n", __func__,
		si->si_ofs.mode_size);
	dev_dbg(cd->dev, "%s: data_size     =%4d\n", __func__,
		si->si_ofs.data_size);
	dev_dbg(cd->dev, "%s: map_sz        =%4d\n", __func__,
		si->si_ofs.map_sz);

	dev_dbg(cd->dev, "%s: btn_rec_size   =%2d\n", __func__,
		si->si_ofs.btn_rec_size);
	dev_dbg(cd->dev, "%s: btn_diff_ofs  =%2d\n", __func__,
		si->si_ofs.btn_diff_ofs);
	dev_dbg(cd->dev, "%s: btn_diff_size  =%2d\n", __func__,
		si->si_ofs.btn_diff_size);

	dev_dbg(cd->dev, "%s: max_x    = 0x%04X (%d)\n", __func__,
		si->si_ofs.max_x, si->si_ofs.max_x);
	dev_dbg(cd->dev, "%s: x_origin = %d (%s)\n", __func__,
		si->si_ofs.x_origin,
		si->si_ofs.x_origin == CY_NORMAL_ORIGIN ?
		"left corner" : "right corner");
	dev_dbg(cd->dev, "%s: max_y    = 0x%04X (%d)\n", __func__,
		si->si_ofs.max_y, si->si_ofs.max_y);
	dev_dbg(cd->dev, "%s: y_origin = %d (%s)\n", __func__,
		si->si_ofs.y_origin,
		si->si_ofs.y_origin == CY_NORMAL_ORIGIN ?
		"upper corner" : "lower corner");
	dev_dbg(cd->dev, "%s: max_p    = 0x%04X (%d)\n", __func__,
		si->si_ofs.max_p, si->si_ofs.max_p);

	dev_dbg(cd->dev, "%s: xy_mode=%p xy_data=%p\n", __func__,
		si->xy_mode, si->xy_data);
}

static int cyttsp4_get_sysinfo_regs(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	int rc;

	rc = cyttsp4_si_data_offsets(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_cydata(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_test_data(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_pcfg_data(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_opcfg_data(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_ddata(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_mdata(cd);
	if (rc < 0)
		return rc;

	rc = cyttsp4_si_get_btn_data(cd);
	if (rc < 0)
		return rc;

#ifdef SAMSUNG_SYSINFO_DATA
	rc = cyttsp4_si_get_samsung_data(cd);
	if (rc < 0)
		return rc;
#endif

	rc = cyttsp4_si_get_op_data_ptrs(cd);
	if (rc < 0) {
		dev_err(cd->dev, "%s: failed to get_op_data\n",
			__func__);
		return rc;
	}

	cyttsp4_si_put_log_data(cd);

	/* provide flow control handshake */
	rc = cyttsp4_handshake(cd, si->si_data.hst_mode);
	if (rc < 0)
		dev_err(cd->dev, "%s: handshake fail on sysinfo reg\n",
			__func__);

	si->ready = true;
	return rc;
}

static void cyttsp4_queue_startup_(struct cyttsp4_core_data *cd)
{
	if (cd->startup_state == STARTUP_NONE) {
		cd->startup_state = STARTUP_QUEUED;
		queue_work(cd->startup_work_q, &cd->startup_work);
		dev_info(cd->dev, "%s: cyttsp4_startup queued\n", __func__);
	} else
		dev_dbg(cd->dev, "%s: startup_state = %d\n", __func__,
			cd->startup_state);
}

static void cyttsp4_queue_startup(struct cyttsp4_core_data *cd)
{
	dev_vdbg(cd->dev, "%s: enter\n", __func__);
	mutex_lock(&cd->system_lock);
	cyttsp4_queue_startup_(cd);
	mutex_unlock(&cd->system_lock);
}

static void call_atten_cb(struct cyttsp4_core_data *cd, int mode)
{
	struct atten_node *atten, *atten_n;

	dev_vdbg(cd->dev, "%s: check list mode=%d\n", __func__, mode);
	spin_lock(&cd->spinlock);
	list_for_each_entry_safe(atten, atten_n,
			&cd->atten_list[CY_ATTEN_IRQ], node) {
		if (atten->mode & mode) {
			spin_unlock(&cd->spinlock);
			dev_vdbg(cd->dev, "%s: attention for mode=%d",
					__func__, atten->mode);
			atten->func(atten->ttsp);
			spin_lock(&cd->spinlock);
		}
	}
	spin_unlock(&cd->spinlock);
}

static irqreturn_t cyttsp4_irq(int irq, void *handle)
{
	struct cyttsp4_core_data *cd = handle;
	struct device *dev = cd->dev;
	enum cyttsp4_mode cur_mode;
	u8 cmd_ofs = cd->sysinfo.si_ofs.cmd_ofs;
	u8 mode[3];
	int rc;
	u8 cat_masked_cmd;

	/*
	 * Check whether this IRQ should be ignored (external)
	 * This should be the very first thing to check since
	 * ignore_irq may be set for a very short period of time
	 */
	if (atomic_read(&cd->ignore_irq)) {
		dev_vdbg(dev, "%s: Ignoring IRQ\n", __func__);
		return IRQ_HANDLED;
	}

	dev_dbg(dev, "%s int:0x%x\n", __func__, cd->int_status);

	mutex_lock(&cd->system_lock);

	/* Just to debug */
	if (cd->sleep_state == SS_SLEEP_ON) {
		dev_vdbg(dev, "%s: Received IRQ while in sleep\n",
			__func__);
	} else if (cd->sleep_state == SS_SLEEPING) {
		dev_vdbg(dev, "%s: Received IRQ while sleeping\n",
			__func__);
	}

	rc = cyttsp4_adap_read(cd->core->adap, CY_REG_BASE, mode, sizeof(mode));
	if (rc) {
		dev_err(cd->dev, "%s: Fail read adapter r=%d\n", __func__, rc);
		goto cyttsp4_irq_exit;
	}
	dev_vdbg(dev, "%s mode[0-2]:0x%X 0x%X 0x%X\n", __func__,
			mode[0], mode[1], mode[2]);

	if (IS_BOOTLOADER(mode[0])) {
		cur_mode = CY_MODE_BOOTLOADER;
		dev_vdbg(dev, "%s: bl running\n", __func__);
		call_atten_cb(cd, cur_mode);
		if (cd->mode == CY_MODE_BOOTLOADER) {
			/* Signal bootloader heartbeat heard */
			wake_up(&cd->wait_q);
			goto cyttsp4_irq_exit;
		}

		/* switch to bootloader */
		dev_dbg(dev, "%s: restart switch to bl m=%d -> m=%d\n",
			__func__, cd->mode, cur_mode);

		/* catch operation->bl glitch */
		if (cd->mode != CY_MODE_UNKNOWN) {
			cyttsp4_queue_startup_(cd);
			goto cyttsp4_irq_exit;
		}

		/*
		 * do not wake thread on this switch since
		 * it is possible to get an early heartbeat
		 * prior to performing the reset
		 */
		cd->mode = cur_mode;

		goto cyttsp4_irq_exit;
	}

	switch (mode[0] & CY_HST_MODE) {
	case CY_HST_OPERATE:
		cur_mode = CY_MODE_OPERATIONAL;
		dev_vdbg(dev, "%s: operational\n", __func__);
		break;
	case CY_HST_CAT:
		cur_mode = CY_MODE_CAT;
		/* set the start sensor mode state. */
		cat_masked_cmd = mode[2] & CY_CMD_MASK;

#ifdef SHOK_SENSOR_DATA_MODE
		if (cat_masked_cmd == CY_CMD_CAT_START_SENSOR_DATA_MODE)
			cd->sysinfo.monitor.mntr_status = CY_MNTR_INITIATED;
		else
			cd->sysinfo.monitor.mntr_status = CY_MNTR_DISABLED;
#endif
		/* Get the Debug info for the interrupt. */
		if (cat_masked_cmd != CY_CMD_CAT_NULL &&
				cat_masked_cmd !=
					CY_CMD_CAT_RETRIEVE_PANEL_SCAN &&
				cat_masked_cmd != CY_CMD_CAT_EXEC_PANEL_SCAN)
			dev_info(cd->dev,
				"%s: cyttsp4_CaT_IRQ=%02X %02X %02X\n",
				__func__, mode[0], mode[1], mode[2]);
		dev_vdbg(dev, "%s: CaT\n", __func__);
		break;
	case CY_HST_SYSINFO:
		cur_mode = CY_MODE_SYSINFO;
		dev_vdbg(dev, "%s: sysinfo\n", __func__);
		break;
	default:
		cur_mode = CY_MODE_UNKNOWN;
		dev_err(dev, "%s: unknown HST mode 0x%02X\n", __func__,
			mode[0]);
		break;
	}

	/* Check whether this IRQ should be ignored (internal) */
	if (cd->int_status & CY_INT_IGNORE) {
		dev_vdbg(dev, "%s: Ignoring IRQ\n", __func__);
		goto cyttsp4_irq_exit;
	}

	/* Check for wake up interrupt */
	if (cd->int_status & CY_INT_AWAKE) {
		cd->int_status &= ~CY_INT_AWAKE;
		wake_up(&cd->sleep_q);
		dev_vdbg(dev, "%s: Received wake up interrupt\n", __func__);
		goto cyttsp4_irq_handshake;
	}

	/* Expecting mode change interrupt */
	if ((cd->int_status & CY_INT_MODE_CHANGE)
			&& (mode[0] & CY_HST_MODE_CHANGE) == 0) {
		cd->int_status &= ~CY_INT_MODE_CHANGE;
		dev_dbg(dev, "%s: finish mode switch m=%d -> m=%d\n",
				__func__, cd->mode, cur_mode);
		cd->mode = cur_mode;
		wake_up(&cd->wait_q);
		goto cyttsp4_irq_handshake;
	}

	/* compare current core mode to current device mode */
	dev_vdbg(dev, "%s: cd->mode=%d cur_mode=%d\n",
			__func__, cd->mode, cur_mode);
	if ((mode[0] & CY_HST_MODE_CHANGE) == 0 && cd->mode != cur_mode) {
		/* Unexpected mode change occurred */
		dev_err(dev, "%s %d->%d 0x%x\n", __func__, cd->mode,
				cur_mode, cd->int_status);
		dev_vdbg(dev, "%s: Unexpected mode change, startup\n",
				__func__);
		cyttsp4_queue_startup_(cd);
		goto cyttsp4_irq_exit;
	}

	/* Expecting command complete interrupt */
	dev_vdbg(dev, "%s: command byte:0x%x, toggle:0x%x\n",
			__func__, mode[cmd_ofs], cd->cmd_toggle);
	if ((cd->int_status & CY_INT_EXEC_CMD)
			&& mode[cmd_ofs] & CY_CMD_COMPLETE) {
		cd->int_status &= ~CY_INT_EXEC_CMD;
		dev_vdbg(dev, "%s: Received command complete interrupt\n",
				__func__);
		wake_up(&cd->wait_q);
		goto cyttsp4_irq_handshake;
	}

	/* This should be status report, read status regs */
	if (cd->mode == CY_MODE_OPERATIONAL) {
		dev_vdbg(dev, "%s: Read status registers\n", __func__);
		rc = cyttsp4_load_status_regs(cd);
		if (rc < 0)
			dev_err(dev, "%s: fail read mode regs r=%d\n",
				__func__, rc);
	}

	/* attention IRQ */
	call_atten_cb(cd, cd->mode);

cyttsp4_irq_handshake:
	/* handshake the event */
	dev_vdbg(dev, "%s: Handshake mode=0x%02X r=%d\n",
			__func__, mode[0], rc);
	rc = cyttsp4_handshake(cd, mode[0]);
	if (rc < 0)
		dev_err(dev, "%s: Fail handshake mode=0x%02X r=%d\n",
				__func__, mode[0], rc);

	/*
	 * a non-zero udelay period is required for using
	 * IRQF_TRIGGER_LOW in order to delay until the
	 * device completes isr deassert
	 */
	udelay(cd->pdata->level_irq_udelay);

cyttsp4_irq_exit:
	mutex_unlock(&cd->system_lock);
	dev_vdbg(dev, "%s: irq done\n", __func__);
	return IRQ_HANDLED;
}

static void cyttsp4_start_wd_timer(struct cyttsp4_core_data *cd, int wd_state)
{
	int selection;
	static int timer_state;

	if (!wd_state)
		timer_state = 0;
	else
		timer_state++;

	selection = timer_state / 3;

	switch (selection) {
	case 0:
		mod_timer(&cd->timer, jiffies + CY_WATCHDOG_TIMEOUT);
		break;
	case 1:
		mod_timer(&cd->timer, jiffies + CY_WATCHDOG_TIMEOUT2);
		break;
	case 2:
	default:
		mod_timer(&cd->timer, jiffies + CY_WATCHDOG_TIMEOUT3);
		break;
	}

	return;
}

static void cyttsp4_stop_wd_timer(struct cyttsp4_core_data *cd)
{
	if (!CY_WATCHDOG_TIMEOUT)
		return;

	del_timer(&cd->timer);
	cancel_work_sync(&cd->work);
	return;
}

static void cyttsp4_timer_watchdog(struct work_struct *work)
{
	struct cyttsp4_core_data *cd =
			container_of(work, struct cyttsp4_core_data, work);
	int rep_stat;
	int mode;
	int retval;
	int wd_state = 0;

	if (cd == NULL) {
		dev_err(cd->dev, "%s: NULL context pointer\n", __func__);
		return;
	}

	mutex_lock(&cd->system_lock);
	retval = cyttsp4_load_status_regs(cd);
	if (retval < 0) {
		dev_err(cd->dev,
			"%s: failed to access device in watchdog timer r=%d\n",
			__func__, retval);
		cyttsp4_queue_startup_(cd);
		wd_state = -1;
		goto cyttsp4_timer_watchdog_exit_error;
	}
	mode = cd->sysinfo.xy_mode[CY_REG_BASE];
	rep_stat = cd->sysinfo.xy_mode[cd->sysinfo.si_ofs.rep_ofs];
	if (IS_BOOTLOADER(mode) && IS_BOOTLOADER(rep_stat)) {
		dev_err(cd->dev,
			"%s: device found in bootloader mode when operational mode rep_stat=0x%02X\n",
				__func__, rep_stat);
		cyttsp4_queue_startup_(cd);
		goto cyttsp4_timer_watchdog_exit_error;
	}

	cyttsp4_start_wd_timer(cd, wd_state);

 cyttsp4_timer_watchdog_exit_error:
	mutex_unlock(&cd->system_lock);
	return;
}

static void cyttsp4_timer(unsigned long handle)
{
	struct cyttsp4_core_data *cd = (struct cyttsp4_core_data *)handle;

	dev_vdbg(cd->dev, "%s: Timer triggered\n", __func__);

	if (!cd)
		return;

	if (!work_pending(&cd->work))
		schedule_work(&cd->work);

	return;
}

static int cyttsp4_write_(struct cyttsp4_device *ttsp, int mode, u8 addr,
	const void *buf, int size)
{
	struct device *dev = &ttsp->dev;
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc = 0;

	mutex_lock(&cd->adap_lock);
	if (mode != cd->mode) {
		dev_dbg(dev, "%s: %s (having %x while %x requested)\n",
			__func__, "attempt to write in missing mode",
			cd->mode, mode);
		rc = -EACCES;
		goto exit;
	}
	rc = cyttsp4_adap_write(core->adap, addr, buf, size);
exit:
	mutex_unlock(&cd->adap_lock);
	return rc;
}

static int cyttsp4_read_(struct cyttsp4_device *ttsp, int mode, u8 addr,
	void *buf, int size)
{
	struct device *dev = &ttsp->dev;
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc = 0;

	mutex_lock(&cd->adap_lock);
	if (mode != cd->mode) {
		dev_dbg(dev, "%s: %s (having %x while %x requested)\n",
			__func__, "attempt to read in missing mode",
			cd->mode, mode);
		rc = -EACCES;
		goto exit;
	}
	rc = cyttsp4_adap_read(core->adap, addr, buf, size);
exit:
	mutex_unlock(&cd->adap_lock);
	return rc;
}

static int cyttsp4_subscribe_attention_(struct cyttsp4_device *ttsp,
	enum cyttsp4_atten_type type,
	int (*func)(struct cyttsp4_device *), int mode)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	unsigned long flags;
	struct atten_node *atten, *atten_new;

	atten_new = kzalloc(sizeof(*atten_new), GFP_KERNEL);
	if (!atten_new) {
		dev_err(cd->dev, "%s: Fail alloc atten node\n", __func__);
		return -ENOMEM;
	}

	dev_dbg(cd->dev, "%s from '%s'\n", __func__, dev_name(cd->dev));

	spin_lock_irqsave(&cd->spinlock, flags);
	list_for_each_entry(atten, &cd->atten_list[type], node) {
		if (atten->ttsp == ttsp && atten->mode == mode) {
			spin_unlock_irqrestore(&cd->spinlock, flags);
			dev_vdbg(cd->dev, "%s: %s=%p %s=%d\n",
				 __func__,
				 "already subscribed attention",
				 ttsp, "mode", mode);

			return 0;
		}
	}

	atten_new->ttsp = ttsp;
	atten_new->mode = mode;
	atten_new->func = func;

	list_add(&atten_new->node, &cd->atten_list[type]);
	spin_unlock_irqrestore(&cd->spinlock, flags);

	return 0;
}

static int cyttsp4_unsubscribe_attention_(struct cyttsp4_device *ttsp,
	enum cyttsp4_atten_type type, int (*func)(struct cyttsp4_device *),
	int mode)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	struct atten_node *atten, *atten_n;
	unsigned long flags;

	spin_lock_irqsave(&cd->spinlock, flags);
	list_for_each_entry_safe(atten, atten_n, &cd->atten_list[type], node) {
		if (atten->ttsp == ttsp && atten->mode == mode) {
			list_del(&atten->node);
			spin_unlock_irqrestore(&cd->spinlock, flags);
			kfree(atten);
			dev_vdbg(cd->dev, "%s: %s=%p %s=%d\n",
				__func__,
				"unsub for atten->ttsp", atten->ttsp,
				"atten->mode", atten->mode);
			return 0;
		}
	}
	spin_unlock_irqrestore(&cd->spinlock, flags);

	return -ENODEV;
}

static int request_exclusive(struct cyttsp4_core_data *cd, void *ownptr, int t)
{
	bool with_timeout = (t != 0);

	mutex_lock(&cd->system_lock);
	if (!cd->exclusive_dev && cd->exclusive_waits == 0) {
		cd->exclusive_dev = ownptr;
		goto exit;
	}

	cd->exclusive_waits++;
wait:
	mutex_unlock(&cd->system_lock);
	if (with_timeout) {
		t = wait_event_timeout(cd->wait_q, !cd->exclusive_dev, t);
		if (IS_TMO(t)) {
			dev_err(cd->dev, "%s: tmo waiting exclusive access\n",
				__func__);
			return -ETIME;
		}
	} else {
		wait_event(cd->wait_q, !cd->exclusive_dev);
	}
	mutex_lock(&cd->system_lock);
	if (cd->exclusive_dev)
		goto wait;
	cd->exclusive_dev = ownptr;
	cd->exclusive_waits--;
exit:
	mutex_unlock(&cd->system_lock);
	dev_vdbg(cd->dev, "%s: request_exclusive ok=%p\n",
		__func__, ownptr);

	return 0;
}

static int cyttsp4_request_exclusive_(struct cyttsp4_device *ttsp, int t)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	return request_exclusive(cd, (void *)ttsp, t);
}

/*
 * returns error if was not owned
 */
static int release_exclusive(struct cyttsp4_core_data *cd, void *ownptr)
{
	mutex_lock(&cd->system_lock);
	if (cd->exclusive_dev != ownptr) {
		mutex_unlock(&cd->system_lock);
		return -EINVAL;
	}

	dev_vdbg(cd->dev, "%s: exclusive_dev %p freed\n",
		__func__, cd->exclusive_dev);
	cd->exclusive_dev = NULL;
	wake_up(&cd->wait_q);
	mutex_unlock(&cd->system_lock);
	return 0;
}

static int cyttsp4_release_exclusive_(struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	return release_exclusive(cd, (void *)ttsp);
}

static int cyttsp4_wait_bl_heartbeat(struct cyttsp4_core_data *cd)
{
	long t;
	int rc = 0;

	/* wait heartbeat */
	dev_vdbg(cd->dev, "%s: wait heartbeat...\n", __func__);
	t = wait_event_timeout(cd->wait_q, cd->mode == CY_MODE_BOOTLOADER,
		msecs_to_jiffies(CY_CORE_RESET_AND_WAIT_TIMEOUT));
	if  (IS_TMO(t)) {
		dev_err(cd->dev, "%s: tmo waiting bl heartbeat cd->mode=%d\n",
				__func__, cd->mode);
		rc = -ETIME;
	}

	return rc;
}

static int cyttsp4_wait_mode_change(struct cyttsp4_core_data *cd, int mode)
{
	long t;

	dev_vdbg(cd->dev, "%s: wait sysinfo...\n", __func__);

	t = wait_event_timeout(cd->wait_q, cd->mode == mode,
			msecs_to_jiffies(CY_CORE_MODE_CHANGE_TIMEOUT));
	if (IS_TMO(t)) {
		dev_err(cd->dev, "%s: tmo waiting exit bl cd->mode=%d\n",
				__func__, cd->mode);
		mutex_lock(&cd->system_lock);
		cd->int_status &= ~CY_INT_MODE_CHANGE;
		mutex_unlock(&cd->system_lock);
		return -ETIME;
	}

	return 0;
}

static int cyttsp4_reset_and_wait(struct cyttsp4_core_data *cd)
{
	int rc;

	/* reset hardware */
	mutex_lock(&cd->system_lock);
	dev_dbg(cd->dev, "%s: reset hw...\n", __func__);
	rc = cyttsp4_hw_reset(cd);
	cd->mode = CY_MODE_UNKNOWN;
	mutex_unlock(&cd->system_lock);
	if (rc < 0) {
		dev_err(cd->dev, "%s: %s adap='%s' r=%d\n", __func__,
			"Fail hw reset", cd->core->adap->id, rc);
		return rc;
	}

	return cyttsp4_wait_bl_heartbeat(cd);
}

/*
 * returns err if refused or timeout(core uses fixed timeout period) occurs;
 * blocks until ISR occurs
 */
static int cyttsp4_request_reset_(struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc;

	rc = cyttsp4_reset_and_wait(cd);
	if (rc < 0)
		dev_err(cd->dev, "%s: Error on h/w reset r=%d\n",
			__func__, rc);

	return rc;
}

/*
 * returns err if refused ; if no error then restart has completed
 * and system is in normal operating mode
 */
static int cyttsp4_request_restart_(struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);

	cyttsp4_queue_startup(cd);

	return 0;
}

/*
 * returns err if refused or timeout; block until mode change complete
 * bit is set (mode change interrupt)
 */
static int set_mode(struct cyttsp4_core_data *cd, void *ownptr,
		u8 new_dev_mode, int new_op_mode)
{
	long t;
	int rc;

	/* change mode */
	dev_dbg(cd->dev, "%s: %s=%p new_dev_mode=%02X new_mode=%d\n",
			__func__, "have exclusive", cd->exclusive_dev,
			new_dev_mode, new_op_mode);
	new_dev_mode |= CY_HST_MODE_CHANGE;

	mutex_lock(&cd->system_lock);
	cd->int_status |= CY_INT_MODE_CHANGE;
	rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE,
			&new_dev_mode, sizeof(new_dev_mode));
	if (rc < 0)
		dev_err(cd->dev, "%s: Fail write mode change r=%d\n",
				__func__, rc);
	mutex_unlock(&cd->system_lock);

	/* wait for mode change done interrupt */
	t = wait_event_timeout(cd->wait_q,
			(cd->int_status & CY_INT_MODE_CHANGE) == 0,
			msecs_to_jiffies(CY_CORE_MODE_CHANGE_TIMEOUT));
	dev_dbg(cd->dev, "%s: back from wait t=%ld cd->mode=%d\n",
			__func__, t, cd->mode);

	if (IS_TMO(t)) {
		dev_err(cd->dev, "%s: %s\n", __func__,
				"tmo waiting mode change");
		mutex_lock(&cd->system_lock);
		cd->int_status &= ~CY_INT_MODE_CHANGE;
		mutex_unlock(&cd->system_lock);
		rc = -EINVAL;
	}

	return rc;
}

static int cyttsp4_request_set_mode_(struct cyttsp4_device *ttsp, int mode)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc;
	enum cyttsp4_hst_mode_bits mode_bits;
	switch (mode) {
	case CY_MODE_OPERATIONAL:
		mode_bits = CY_HST_OPERATE;
		break;
	case CY_MODE_SYSINFO:
		mode_bits = CY_HST_SYSINFO;
		break;
	case CY_MODE_CAT:
		mode_bits = CY_HST_CAT;
		break;
	default:
		dev_err(cd->dev, "%s: invalid mode: %02X(%d)\n",
			__func__, mode, mode);
		return -EINVAL;
	}

	rc = set_mode(cd, (void *)ttsp, mode_bits, mode);
	if (rc < 0)
		dev_err(cd->dev, "%s: fail set_mode=%02X(%d)\n",
			__func__, cd->mode, cd->mode);
#ifdef SHOK_SENSOR_DATA_MODE
	if (cd->sysinfo.monitor.mntr_status == CY_MNTR_INITIATED &&
			mode == CY_MODE_OPERATIONAL)
		cd->sysinfo.monitor.mntr_status = CY_MNTR_STARTED;
	else
		cd->sysinfo.monitor.mntr_status = CY_MNTR_DISABLED;
#endif

	return rc;
}

/*
 * returns NULL if sysinfo has not been acquired from the device yet
 */
static struct cyttsp4_sysinfo *cyttsp4_request_sysinfo_(
		struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);

	if (cd->sysinfo.ready)
		return &cd->sysinfo;

	return NULL;
}

static struct cyttsp4_touch_firmware *cyttsp4_request_firmware_(
		struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	return cd->pdata->fw;
}

static int cyttsp4_request_handshake_(struct cyttsp4_device *ttsp, u8 mode)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc;

	rc = cyttsp4_handshake(cd, mode);
	if (rc < 0)
		dev_err(&core->dev, "%s: Fail handshake r=%d\n", __func__, rc);

	return rc;
}

static int cyttsp4_request_toggle_lowpower_(struct cyttsp4_device *ttsp,
		u8 mode)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc = cyttsp4_toggle_low_power(cd, mode);
	if (rc < 0)
		dev_err(&core->dev, "%s: Fail toggle low power r=%d\n",
				__func__, rc);
	return rc;
}

static int cyttsp4_set_loader_(struct cyttsp4_device *ttsp,
		cyttsp4_loader_func func)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc = 0;

	dev_vdbg(cd->dev, "%s: enter...\n", __func__);

	if (!ttsp || !func)
		return -EINVAL;

	mutex_lock(&cd->system_lock);
	if (!cd->loader) {
		cd->loader = ttsp;
		cd->loader_func = func;
	} else
		rc = -EBUSY;
	mutex_unlock(&cd->system_lock);

	return rc;
}

static int cyttsp4_unset_loader_(struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	int rc = 0;

	dev_vdbg(cd->dev, "%s: enter...\n", __func__);

	mutex_lock(&cd->system_lock);
	if (cd->loader == ttsp) {
		cd->loader = NULL;
		cd->loader_func = NULL;
	} else
		rc = -EINVAL;
	mutex_unlock(&cd->system_lock);

	return rc;
}

static const u8 ldr_exit[] = {
	0xFF, 0x01, 0x3B, 0x00, 0x00, 0x4F, 0x6D, 0x17
};

/*
 * Send command to device for CAT and OP modes
 * return negative value on error, 0 on success
 */
static int cyttsp4_exec_cmd_(struct cyttsp4_core_data *cd, u8 mode,
		u8 *cmd_buf, size_t cmd_size, u8 *return_buf,
		size_t return_buf_size, int timeout)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;
	struct device *dev = cd->dev;
	int cmd_ofs;
	u8 command;
	int rc;

	mutex_lock(&cd->system_lock);
	if (mode != cd->mode) {
		dev_err(dev, "%s: %s (having %x while %x requested)\n",
				__func__, "attempt to exec cmd in missing mode",
				cd->mode, mode);
		mutex_unlock(&cd->system_lock);
		return -EACCES;
	}

	switch (mode) {
	case CY_MODE_CAT:
		cmd_ofs = CY_REG_CAT_CMD;
		break;
	case CY_MODE_OPERATIONAL:
		cmd_ofs = si->si_ofs.cmd_ofs;
		break;
	default:
		dev_err(dev, "%s: Unsupported mode %x for exec cmd\n",
				__func__, mode);
		mutex_unlock(&cd->system_lock);
		return -EACCES;
	}

	/* Check if complete is set, so write new command */
	rc = cyttsp4_adap_read(cd->core->adap, cmd_ofs, &command, 1);
	if (rc < 0) {
		dev_err(dev, "%s: Error on read r=%d\n", __func__, rc);
		mutex_unlock(&cd->system_lock);
		return rc;
	}

	cd->cmd_toggle = GET_TOGGLE(command);
	cd->int_status |= CY_INT_EXEC_CMD;

	if ((command & CY_CMD_COMPLETE_MASK) == 0) {
		/* Let irq handler run */
		mutex_unlock(&cd->system_lock);
		rc = wait_event_timeout(cd->wait_q,
				(cd->int_status & CY_INT_EXEC_CMD) == 0,
				msecs_to_jiffies(timeout));
		if (IS_TMO(rc)) {
			dev_err(dev, "%s: Command execution timed out\n",
					__func__);
			cd->int_status &= ~CY_INT_EXEC_CMD;
			return -EINVAL;
		}

		/* For next command */
		mutex_lock(&cd->system_lock);
		rc = cyttsp4_adap_read(cd->core->adap, cmd_ofs, &command, 1);
		if (rc < 0) {
			dev_err(dev, "%s: Error on read r=%d\n", __func__, rc);
			mutex_unlock(&cd->system_lock);
			return rc;
		}
		cd->cmd_toggle = GET_TOGGLE(command);
		cd->int_status |= CY_INT_EXEC_CMD;
	}

	/*
	 * Write new command
	 * Only update command bits 0:5
	 * Clear command complete bit & toggle bit
	 */
	cmd_buf[0] = cmd_buf[0] & CY_CMD_MASK;
	rc = cyttsp4_adap_write(cd->core->adap, cmd_ofs, cmd_buf, cmd_size);
	if (rc < 0) {
		dev_err(dev, "%s: Error on write command r=%d\n",
				__func__, rc);
		mutex_unlock(&cd->system_lock);
		return rc;
	}

	/*
	 * Wait command to be completed
	 */
	mutex_unlock(&cd->system_lock);
	rc = wait_event_timeout(cd->wait_q,
			(cd->int_status & CY_INT_EXEC_CMD) == 0,
			msecs_to_jiffies(timeout));
	if (IS_TMO(rc)) {
		dev_err(dev, "%s: Command execution timed out\n", __func__);
		cd->int_status &= ~CY_INT_EXEC_CMD;
		return -EINVAL;
	}

	rc = cyttsp4_adap_read(cd->core->adap, cmd_ofs + 1, return_buf,
			return_buf_size);
	if (rc < 0) {
		dev_err(dev, "%s: Error on read 3 r=%d\n", __func__, rc);
		return rc;
	}

	return 0;
}

static int cyttsp4_request_exec_cmd_(struct cyttsp4_device *ttsp, u8 mode,
		u8 *cmd_buf, size_t cmd_size, u8 *return_buf,
		size_t return_buf_size, int timeout)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);

	return cyttsp4_exec_cmd_(cd, mode, cmd_buf, cmd_size, return_buf,
			return_buf_size, timeout);
}

static int cyttsp4_request_stop_wd_(struct cyttsp4_device *ttsp)
{
	struct cyttsp4_core *core = ttsp->core;
	struct cyttsp4_core_data *cd = dev_get_drvdata(&core->dev);
	cyttsp4_stop_wd_timer(cd);
	return 0;
}

static int cyttsp4_core_sleep_(struct cyttsp4_core_data *cd)
{
	enum cyttsp4_sleep_state ss = SS_SLEEP_ON;
	int rc = 0;
	u8 mode;
	int wd_state = 0;

	dev_vdbg(cd->dev, "%s: enter...\n", __func__);

	/* Already in sleep mode? */
	mutex_lock(&cd->system_lock);
	if (cd->sleep_state == SS_SLEEP_ON) {
		mutex_unlock(&cd->system_lock);
		return 0;
	}
	cd->sleep_state = SS_SLEEPING;
	mutex_unlock(&cd->system_lock);

	/* Wait until currently running IRQ handler exits and disable IRQ */
	disable_irq(cd->irq);

	cyttsp4_stop_wd_timer(cd);

	if (cd->startup_done == false) {
		dev_err(cd->dev, "%s: Fail startup_state\n", __func__);
		goto power_off;
	}

	rc = cyttsp4_adap_read(cd->core->adap, CY_REG_BASE, &mode,
			sizeof(mode));
	if (rc) {
		dev_err(cd->dev, "%s: Fail read adapter r=%d\n", __func__, rc);
		wd_state = -1;
		goto error;
	}

	if (IS_BOOTLOADER(mode)) {
		dev_err(cd->dev, "%s: Device in BOOTLADER mode.\n", __func__);
		rc = -EINVAL;
		goto error;
	}

power_off:
	if (cd->pdata->led_power) {
		printk(KERN_INFO "led_power off\n");
		rc = cd->pdata->led_power(0);
		led_status = 0;
		if (rc < 0)
			dev_dbg(cd->dev, "Fail to led_power(0)\n");
	}

	/* Call board specific power function to power down */
	if (cd->pdata->power) {
		dev_dbg(cd->dev, "%s: Power down HW\n", __func__);
		rc = cd->pdata->power(cd->pdata, 0, cd->dev, &cd->ignore_irq);
	} else {
		dev_dbg(cd->dev, "%s: No power function\n", __func__);
		rc = -ENOSYS;
	}
	if (rc < 0)
		dev_err(cd->dev, "%s: HW Power down fails r=%d\n",
				__func__, rc);
	else
		goto exit;

error:
	ss = SS_SLEEP_OFF;
	cyttsp4_start_wd_timer(cd, wd_state);

exit:
	mutex_lock(&cd->system_lock);
	cd->sleep_state = ss;
	mutex_unlock(&cd->system_lock);

	enable_irq(cd->irq);

	return rc;
}

static int cyttsp4_core_sleep(struct cyttsp4_core_data *cd)
{
	int rc;

	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit;
	}

	rc = cyttsp4_core_sleep_(cd);

	if (release_exclusive(cd, cd->core) < 0)
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit:
	return rc;
}

static int cyttsp4_core_wake_(struct cyttsp4_core_data *cd)
{
	struct device *dev = cd->dev;
	int mode;
	u8 host_mode;
	int rc;
	bool sysinfo_ready;
	int wd_state = 0;

	dev_vdbg(dev, "%s: enter...\n", __func__);

	/* Already woken? */
	mutex_lock(&cd->system_lock);
	if (cd->sleep_state == SS_SLEEP_OFF) {
		mutex_unlock(&cd->system_lock);
		return 0;
	}

	cd->sleep_state = SS_WAKING;
	/* Get the last active mode and set the mode to unknown*/
	mode = cd->mode;
	cd->mode = CY_MODE_UNKNOWN;
	sysinfo_ready = cd->sysinfo.ready;
	mutex_unlock(&cd->system_lock);

	if (cd->pdata->led_power) {
		printk(KERN_INFO "led_power on\n");
		rc = cd->pdata->led_power(1);
		led_status = 1;
		if (rc < 0)
			dev_dbg(cd->dev, "Fail to led_power(1)\n");
	}

	/* Call board specific power function to power up */
	if (cd->pdata->power) {
		dev_dbg(dev, "%s: Power up HW\n", __func__);
		rc = cd->pdata->power(cd->pdata, 1, dev, &cd->ignore_irq);
	} else {
		dev_dbg(dev, "%s: No power function\n", __func__);
		rc = -ENOSYS;
	}
	if (rc < 0) {
		dev_err(dev, "%s: HW Power up fails r=%d\n",
				__func__, rc);
	} else
		dev_vdbg(dev, "%s: HW power up succeeds\n",
			__func__);

	/* Wait for heartbeat */
	rc = cyttsp4_wait_bl_heartbeat(cd);
	if (rc)
		dev_err(dev, "%s: Error on waiting bl heartbeat r=%d\n",
				__func__, rc);

	if (mode == CY_MODE_BOOTLOADER)
		goto exit;

	/* exit bl into sysinfo mode */
	dev_vdbg(dev, "%s: write exit ldr...\n", __func__);
	mutex_lock(&cd->system_lock);
	cd->int_status |= CY_INT_MODE_CHANGE;
	mutex_unlock(&cd->system_lock);
	rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE,
		(u8 *)ldr_exit, sizeof(ldr_exit));
	if (rc < 0) {
		dev_err(cd->dev, "%s: Fail write adap='%s' r=%d\n",
			__func__, cd->core->adap->id, rc);
		wd_state--;
	}

	rc = cyttsp4_wait_mode_change(cd, CY_MODE_SYSINFO);
	if (rc) {
		dev_err(dev, "%s: Error on switching to sysinfo mode r=%d\n",
				__func__, rc);
		wd_state--;
	}

	/* read sysinfo data */
	if (!sysinfo_ready) {
		dev_vdbg(dev, "%s: get sysinfo regs..\n", __func__);
		rc = cyttsp4_get_sysinfo_regs(cd);
		if (rc < 0) {
			dev_err(dev, "%s: failed to get sysinfo regs rc=%d\n",
				__func__, rc);
			wd_state--;
		}
	}

	if (mode == CY_MODE_SYSINFO)
		goto exit;

	/* Check for CAT mode */
	if (mode == CY_MODE_CAT) {
		host_mode = CY_HST_CAT;
	} else {
		/* Go with Operational mode */
		mode = CY_MODE_OPERATIONAL;
		host_mode = CY_HST_OPERATE;
	}

	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, host_mode, mode);
	set_mode(cd, cd->core, host_mode, mode);

exit:
	mutex_lock(&cd->system_lock);
	cd->sleep_state = SS_SLEEP_OFF;
	mutex_unlock(&cd->system_lock);

	/* Do not start watchdog in already woken state */
	cyttsp4_start_wd_timer(cd, wd_state);

	return rc;
}

static int cyttsp4_core_wake(struct cyttsp4_core_data *cd)
{
	int rc;

	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		return 0;
	}

	rc = cyttsp4_core_wake_(cd);

	if (release_exclusive(cd, cd->core) < 0)
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

	return rc;
}

static int cyttsp4_startup_(struct cyttsp4_core_data *cd)
{
	struct atten_node *atten, *atten_n;
	unsigned long flags;
	int rc;

	dev_dbg(cd->dev, "%s: enter...\n", __func__);

	cd->startup_done = false;
	cyttsp4_stop_wd_timer(cd);

	/* reset hardware and wait for heartbeat */
	rc = cyttsp4_reset_and_wait(cd);
	if (rc < 0)
		dev_err(cd->dev, "%s: Error on h/w reset r=%d\n", __func__, rc);

	/* exit bl into sysinfo mode */
	dev_vdbg(cd->dev, "%s: write exit ldr...\n", __func__);
	mutex_lock(&cd->system_lock);
	cd->int_status &= ~CY_INT_IGNORE;
	cd->int_status |= CY_INT_MODE_CHANGE;
	rc = cyttsp4_adap_write(cd->core->adap, CY_REG_BASE,
		(u8 *)ldr_exit, sizeof(ldr_exit));
	if (rc < 0)
		dev_err(cd->dev, "%s: Fail write adap='%s' r=%d\n",
			__func__, cd->core->adap->id, rc);
	mutex_unlock(&cd->system_lock);

	rc = cyttsp4_wait_mode_change(cd, CY_MODE_SYSINFO);
	if (rc) {
		cyttsp4_start_wd_timer(cd, -1);

		/*
		 * Unable to switch to SYSINFO mode,
		 * Corrupted FW may cause crash, exit here.
		 */
		return 0;
	}

	/* read sysinfo data */
	dev_vdbg(cd->dev, "%s: get sysinfo regs..\n", __func__);
	rc = cyttsp4_get_sysinfo_regs(cd);
	if (rc < 0) {
		dev_err(cd->dev, "%s: failed to get sysinfo regs rc=%d\n",
			__func__, rc);
		cyttsp4_start_wd_timer(cd, rc);
	} else
		cyttsp4_start_wd_timer(cd, 0);
	/* switch to operational mode */
	dev_vdbg(cd->dev, "%s: set mode cd->core=%p hst_mode=%02X mode=%d...\n",
		__func__, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);
	set_mode(cd, cd->core, CY_HST_OPERATE, CY_MODE_OPERATIONAL);

	/* attention startup */
	spin_lock_irqsave(&cd->spinlock, flags);
	list_for_each_entry_safe(atten, atten_n,
		&cd->atten_list[CY_ATTEN_STARTUP], node) {
		dev_dbg(cd->dev, "%s: attention for '%s'", __func__,
			dev_name(&atten->ttsp->dev));
		spin_unlock_irqrestore(&cd->spinlock, flags);
		atten->func(atten->ttsp);
		spin_lock_irqsave(&cd->spinlock, flags);
	}
	spin_unlock_irqrestore(&cd->spinlock, flags);

	/* restore to sleep if was suspended */
	mutex_lock(&cd->system_lock);
	if (cd->sleep_state == SS_SLEEP_ON) {
		cd->sleep_state = SS_SLEEP_OFF;
		mutex_unlock(&cd->system_lock);
		cyttsp4_core_sleep_(cd);
		mutex_lock(&cd->system_lock);
	}
	mutex_unlock(&cd->system_lock);
	/* Required for signal to the TTHE */
	dev_info(cd->dev, "%s: cyttsp4_exit startup r=%d...\n", __func__, rc);
	cd->startup_done = true;
	return rc;
}

static int cyttsp4_startup(struct cyttsp4_core_data *cd)
{
	int rc;

	mutex_lock(&cd->system_lock);
	cd->startup_state = STARTUP_RUNNING;
	mutex_unlock(&cd->system_lock);

	rc = request_exclusive(cd, cd->core, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(cd->dev, "%s: fail get exclusive ex=%p own=%p\n",
				__func__, cd->exclusive_dev, cd->core);
		goto exit;
	}

	rc = cyttsp4_startup_(cd);

	if (release_exclusive(cd, cd->core) < 0)
		/* Don't return fail code, mode is already changed. */
		dev_err(cd->dev, "%s: fail to release exclusive\n", __func__);
	else
		dev_vdbg(cd->dev, "%s: pass release exclusive\n", __func__);

exit:
	mutex_lock(&cd->system_lock);
	cd->startup_state = STARTUP_NONE;
	mutex_unlock(&cd->system_lock);

	return rc;
}

static void cyttsp4_startup_work_function(struct work_struct *work)
{
	struct cyttsp4_core_data *cd =  container_of(work,
		struct cyttsp4_core_data, startup_work);
	int rc;

	rc = cyttsp4_startup(cd);
	if (rc < 0)
		dev_err(cd->dev, "%s: Fail queued startup r=%d\n",
			__func__, rc);
}

static void cyttsp4_free_si_ptrs(struct cyttsp4_core_data *cd)
{
	struct cyttsp4_sysinfo *si = &cd->sysinfo;

	kfree(si->si_ptrs.cydata->mfg_id);
	kfree(si->si_ptrs.cydata);
	kfree(si->si_ptrs.test);
	kfree(si->si_ptrs.pcfg);
	kfree(si->si_ptrs.opcfg);
	kfree(si->si_ptrs.ddata);
	kfree(si->si_ptrs.mdata);
#ifdef SAMSUNG_SYSINFO_DATA
	kfree(si->si_ptrs.samsung_data);
#endif
	kfree(si->btn);
	kfree(si->xy_mode);
	kfree(si->xy_data);
	kfree(si->btn_rec_data);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static int cyttsp4_core_early_suspend(struct early_suspend *h)
{
	struct cyttsp4_core_data *cd =
		container_of(h, struct cyttsp4_core_data, early_suspend);
	int rc;

	printk(KERN_INFO "%s\n", __func__);

	rc = cyttsp4_core_sleep(cd);
	if (rc < 0) {
		printk(KERN_INFO "%s: Error on sleep\n", __func__);
		return -EAGAIN;
	}

	return 0;
}

static int cyttsp4_core_late_resume(struct early_suspend *h)
{
	struct cyttsp4_core_data *cd =
		container_of(h, struct cyttsp4_core_data, early_suspend);
	int rc;

	printk(KERN_INFO "%s\n", __func__);

	rc = cyttsp4_core_wake(cd);
	if (rc < 0) {
		printk(KERN_INFO "%s: Error on wake\n", __func__);
		return -EAGAIN;
	}

	return 0;
}
#endif

#if defined(CONFIG_PM_SLEEP) || defined(CONFIG_PM_RUNTIME)
static int cyttsp4_core_suspend(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = cyttsp4_core_sleep(cd);
	if (rc < 0) {
		dev_err(dev, "%s: Error on sleep\n", __func__);
		return -EAGAIN;
	}

	return 0;
}

static int cyttsp4_core_resume(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = cyttsp4_core_wake(cd);
	if (rc < 0) {
		dev_err(dev, "%s: Error on wake\n", __func__);
		return -EAGAIN;
	}

	return 0;
}
#endif

static const struct dev_pm_ops cyttsp4_core_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cyttsp4_core_suspend, cyttsp4_core_resume)
	SET_RUNTIME_PM_OPS(cyttsp4_core_suspend, cyttsp4_core_resume, NULL)
};

/*
 * Show Firmware version via sysfs
 */
static ssize_t cyttsp4_ic_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp4_cydata *cydata = cd->sysinfo.si_ptrs.cydata;

	return sprintf(buf,
		"%s: 0x%02X 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n"
		"%s: 0x%02X\n"
		"%s: 0x%02X\n",
		"TrueTouch Product ID", cydata->ttpidh, cydata->ttpidl,
		"Firmware Major Version", cydata->fw_ver_major,
		"Firmware Minor Version", cydata->fw_ver_minor,
		"Revision Control Number", cydata->revctrl[0],
		cydata->revctrl[1], cydata->revctrl[2], cydata->revctrl[3],
		cydata->revctrl[4], cydata->revctrl[5], cydata->revctrl[6],
		cydata->revctrl[7],
		"Bootloader Major Version", cydata->blver_major,
		"Bootloader Minor Version", cydata->blver_minor);
}

/*
 * Show Driver version via sysfs
 */
static ssize_t cyttsp4_drv_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Driver: %s\nVersion: %s\nDate: %s\n",
		cy_driver_core_name, cy_driver_core_version,
		cy_driver_core_date);
}

/*
 * HW reset via sysfs
 */
static ssize_t cyttsp4_hw_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	int rc = 0;

	rc = cyttsp4_startup(cd);
	if (rc < 0)
		dev_err(dev, "%s: HW reset failed r=%d\n",
			__func__, rc);

	return size;
}

/*
 * Show IRQ status via sysfs
 */
static ssize_t cyttsp4_hw_irq_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	int retval;

	if (cd->pdata->irq_stat) {
		retval = cd->pdata->irq_stat(cd->pdata, dev);
		switch (retval) {
		case 0:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Interrupt line is LOW.\n");
		case 1:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Interrupt line is HIGH.\n");
		default:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Function irq_stat() returned %d.\n", retval);
		}
	}

	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Function irq_stat() undefined.\n");
}

/*
 * Show IRQ enable/disable status via sysfs
 */
static ssize_t cyttsp4_drv_irq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&cd->system_lock);
	if (cd->irq_enabled)
		ret = snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Driver interrupt is ENABLED\n");
	else
		ret = snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Driver interrupt is DISABLED\n");
	mutex_unlock(&cd->system_lock);

	return ret;
}

/*
 * Enable/disable IRQ via sysfs
 */
static ssize_t cyttsp4_drv_irq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	unsigned long value;
	int retval = 0;

	retval = kstrtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(dev, "%s: Invalid value\n", __func__);
		goto cyttsp4_drv_irq_store_error_exit;
	}

	mutex_lock(&cd->system_lock);
	switch (value) {
	case 0:
		if (cd->irq_enabled) {
			cd->irq_enabled = false;
			/* Disable IRQ */
			disable_irq_nosync(cd->irq);
			dev_info(dev, "%s: Driver IRQ now disabled\n",
				__func__);
		} else
			dev_info(dev, "%s: Driver IRQ already disabled\n",
				__func__);
		break;

	case 1:
		if (cd->irq_enabled == false) {
			cd->irq_enabled = true;
			/* Enable IRQ */
			enable_irq(cd->irq);
			dev_info(dev, "%s: Driver IRQ now enabled\n",
				__func__);
		} else
			dev_info(dev, "%s: Driver IRQ already enabled\n",
				__func__);
		break;

	default:
		dev_err(dev, "%s: Invalid value\n", __func__);
	}
	mutex_unlock(&(cd->system_lock));

cyttsp4_drv_irq_store_error_exit:

	return size;
}

/*
 * Debugging options via sysfs
 */
static ssize_t cyttsp4_drv_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	unsigned long value = 0;
	int rc = 0;

	rc = kstrtoul(buf, 10, &value);
	if (rc < 0) {
		dev_err(dev, "%s: Invalid value\n", __func__);
		goto cyttsp4_drv_debug_store_exit;
	}

	switch (value) {
	case CY_DBG_SUSPEND:
		dev_info(dev, "%s: SUSPEND (cd=%p)\n", __func__, cd);
		rc = cyttsp4_core_sleep(cd);
		if (rc)
			dev_err(dev, "%s: Suspend failed rc=%d\n",
				__func__, rc);
		else
			dev_info(dev, "%s: Suspend succeeded\n", __func__);
		break;

	case CY_DBG_RESUME:
		dev_info(dev, "%s: RESUME (cd=%p)\n", __func__, cd);
		rc = cyttsp4_core_wake(cd);
		if (rc)
			dev_err(dev, "%s: Resume failed rc=%d\n",
				__func__, rc);
		else
			dev_info(dev, "%s: Resume succeeded\n", __func__);
		break;
	case CY_DBG_SOFT_RESET:
		dev_info(dev, "%s: SOFT RESET (cd=%p)\n", __func__, cd);
		rc = cyttsp4_hw_soft_reset(cd);
		break;
	case CY_DBG_RESET:
		dev_info(dev, "%s: HARD RESET (cd=%p)\n", __func__, cd);
		rc = cyttsp4_hw_hard_reset(cd);
		break;
	default:
		dev_err(dev, "%s: Invalid value\n", __func__);
	}

cyttsp4_drv_debug_store_exit:
	return size;
}

/*
 * Show system status on deep sleep status via sysfs
 */
static ssize_t cyttsp4_sleep_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&cd->system_lock);
	if (cd->sleep_state == SS_SLEEP_ON)
		ret = snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Deep Sleep is ENABLED\n");
	else
		ret = snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Deep Sleep is DISABLED\n");
	mutex_unlock(&cd->system_lock);

	return ret;
}

static struct device_attribute attributes[] = {
	__ATTR(ic_ver, S_IRUGO, cyttsp4_ic_ver_show, NULL),
	__ATTR(drv_ver, S_IRUGO, cyttsp4_drv_ver_show, NULL),
	__ATTR(hw_reset, S_IWUSR, NULL, cyttsp4_hw_reset_store),
	__ATTR(hw_irq_stat, S_IRUSR, cyttsp4_hw_irq_stat_show, NULL),
	__ATTR(drv_irq, S_IRUSR | S_IWUSR, cyttsp4_drv_irq_show,
		cyttsp4_drv_irq_store),
	__ATTR(drv_debug, S_IWUSR, NULL, cyttsp4_drv_debug_store),
	__ATTR(sleep_status, S_IRUSR, cyttsp4_sleep_status_show, NULL),
};

static int add_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto undo;
	return 0;
undo:
	for (; i >= 0 ; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s: failed to create sysfs interface\n", __func__);
	return -ENODEV;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
}

static int cyttsp4_core_probe(struct cyttsp4_core *core)
{
	struct cyttsp4_core_data *cd;
	struct device *dev = &core->dev;
	struct cyttsp4_core_platform_data *pdata = dev_get_platdata(dev);
	enum cyttsp4_atten_type type;
	unsigned long irq_flags;
	int rc = 0;

#ifdef SEC_TSP_FACTORY_TEST
	struct device *fac_dev_ts;
	int i, retval = 0;
#endif

	dev_info(dev, "%s: startup\n", __func__);
	dev_dbg(dev, "%s: debug on\n", __func__);
	dev_vdbg(dev, "%s: verbose debug on\n", __func__);

	/* get context and debug print buffers */
	cd = kzalloc(sizeof(*cd), GFP_KERNEL);
	if (cd == NULL) {
		dev_err(dev, "%s: Error, kzalloc\n", __func__);
		rc = -ENOMEM;
		goto error_alloc_data_failed;
	}

	/* point to core device and init lists */
	cd->core = core;
	mutex_init(&cd->system_lock);
	mutex_init(&cd->adap_lock);
	for (type = 0; type < CY_ATTEN_NUM_ATTEN; type++)
		INIT_LIST_HEAD(&cd->atten_list[type]);
	init_waitqueue_head(&cd->wait_q);
	init_waitqueue_head(&cd->sleep_q);
	cd->startup_work_q = create_singlethread_workqueue("startup_work_q");
	if (cd->startup_work_q == NULL) {
		dev_err(dev, "%s: No memory for %s\n", __func__,
			"startup_work_q");
		goto error_init;
	}

	dev_dbg(dev, "%s: initialize core data\n", __func__);
	spin_lock_init(&cd->spinlock);
	cd->dev = dev;
	cd->pdata = pdata;
	cd->irq = gpio_to_irq(pdata->irq_gpio);
	cd->irq_enabled = true;
	dev_set_drvdata(dev, cd);
	if (cd->irq < 0) {
		rc = -EINVAL;
		goto error_gpio_irq;
	}

	if (cd->pdata->init) {
		dev_info(cd->dev, "%s: Init HW\n", __func__);
		rc = cd->pdata->init(cd->pdata, 1, cd->dev);
	} else {
		dev_info(cd->dev, "%s: No HW INIT function\n", __func__);
		rc = 0;
	}
	if (rc < 0)
		dev_err(cd->dev, "%s: HW Init fail r=%d\n", __func__, rc);

	INIT_WORK(&cd->startup_work, cyttsp4_startup_work_function);

	dev_dbg(dev, "%s: initialize threaded irq=%d\n", __func__, cd->irq);
	if (cd->pdata->level_irq_udelay > 0)
		/* use level triggered interrupts */
		irq_flags = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	else
		/* use edge triggered interrupts */
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;

	rc = request_threaded_irq(cd->irq, NULL, cyttsp4_irq, irq_flags,
		dev_name(dev), cd);
	if (rc < 0) {
		dev_err(dev, "%s: Error, could not request irq\n", __func__);
		goto error_request_irq;
	}

	INIT_WORK(&cd->work, cyttsp4_timer_watchdog);
	setup_timer(&cd->timer, cyttsp4_timer, (unsigned long)cd);

	dev_dbg(dev, "%s: add sysfs interfaces\n", __func__);
	rc = add_sysfs_interfaces(dev);
	if (rc < 0) {
		dev_err(dev, "%s: Error, fail sysfs init\n", __func__);
		goto error_attr_create;
	}

	pm_runtime_enable(dev);

	/*
	 * call startup directly to ensure that the device
	 * is tested before leaving the probe
	 */
	dev_dbg(dev, "%s: call startup\n", __func__);

	pm_runtime_get_sync(dev);

	rc = cyttsp4_startup(cd);

	pm_runtime_put(dev);

	if (rc < 0) {
		dev_err(cd->dev, "%s: Fail initial startup r=%d\n",
			__func__, rc);
		goto error_startup;
	}

	/*msleep(400);*/	/* KEVI added + by BTOK */

#ifdef SEC_TSP_FACTORY_TEST
	INIT_LIST_HEAD(&cd->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &cd->cmd_list_head);

	mutex_init(&cd->cmd_lock);
	cd->cmd_is_running = false;

	fac_dev_ts = device_create(sec_class, NULL, 0, cd, "tsp");
	if (IS_ERR(fac_dev_ts))
		dev_err(cd->dev, "Failed to create device for the sysfs\n");

	retval = sysfs_create_group(&fac_dev_ts->kobj,
			&sec_touch_factory_attr_group);
	if (retval)
		dev_err(cd->dev, "Failed to create sysfs group\n");
#endif

#ifdef SEC_TSP_INFO_SHOW
	sec_touchscreen =
		device_create(sec_class, NULL, 0, cd, "sec_touchscreen");

	sec_touchkey =
		device_create(sec_class, NULL, 0, cd, "sec_touchkey");

	if (IS_ERR(sec_touchscreen))
		pr_err("[TSP] Failed to create device(sec_touchscreen)!\n");

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_update) < 0)
		pr_err("[TSP] Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_update.attr.name);
	if (device_create_file
		(sec_touchscreen, &dev_attr_tsp_firm_update_status) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_update_status.attr.name);
	if (device_create_file(sec_touchscreen, &dev_attr_tsp_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_threshold.attr.name);
	if (device_create_file
		(sec_touchscreen, &dev_attr_tsp_firm_version_phone) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_version_phone.attr.name);
	if (device_create_file
		(sec_touchscreen, &dev_attr_tsp_firm_version_panel) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_version_panel.attr.name);
	if (device_create_file
		(sec_touchscreen, &dev_attr_tsp_firm_version_config) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_version_config.attr.name);
	if (device_create_file(sec_touchscreen, &dev_attr_tsp_touchtype) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_touchtype.attr.name);

	if (IS_ERR(sec_touchkey))
		pr_err("[TSP] Failed to create device(sec_touchkey)!\n");

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_menu) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_menu.attr.name);
	if (device_create_file(sec_touchkey, &dev_attr_touchkey_back) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_back.attr.name);
	if (device_create_file(sec_touchkey, &dev_attr_touchkey_home) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_home.attr.name);
	if (device_create_file(sec_touchkey, &dev_attr_touchkey_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_threshold.attr.name);
	if (device_create_file(sec_touchkey, &dev_attr_brightness) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_brightness.attr.name);

#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	cd->early_suspend.suspend =
		(void *)cyttsp4_core_early_suspend;
	cd->early_suspend.resume =
		(void *)cyttsp4_core_late_resume;
	register_early_suspend(&cd->early_suspend);
#endif

	dev_info(dev, "%s: ok\n", __func__);
	rc = 0;
	goto no_error;

error_startup:
	pm_runtime_disable(dev);
	cyttsp4_free_si_ptrs(cd);
error_attr_create:
	free_irq(cd->irq, cd);
error_request_irq:
error_gpio_irq:
	if (pdata->init)
		pdata->init(pdata, 0, dev);
error_init:
	dev_set_drvdata(dev, NULL);
	kfree(cd);
error_alloc_data_failed:
	dev_err(dev, "%s failed.\n", __func__);
no_error:
	return rc;
}

static int cyttsp4_core_release(struct cyttsp4_core *core)
{
	struct device *dev = &core->dev;
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	cyttsp4_stop_wd_timer(cd);

	pm_runtime_suspend(dev);
	pm_runtime_disable(dev);

	remove_sysfs_interfaces(dev);
	free_irq(cd->irq, cd);
	if (cd->pdata->init)
		cd->pdata->init(cd->pdata, 0, dev);
	dev_set_drvdata(dev, NULL);
	cyttsp4_free_si_ptrs(cd);
	kfree(cd);
	return 0;
}

struct cyttsp4_core_driver cyttsp4_core_driver = {
	.probe = cyttsp4_core_probe,
	.remove = cyttsp4_core_release,
	.subscribe_attention = cyttsp4_subscribe_attention_,
	.unsubscribe_attention = cyttsp4_unsubscribe_attention_,
	.request_exclusive = cyttsp4_request_exclusive_,
	.release_exclusive = cyttsp4_release_exclusive_,
	.request_reset = cyttsp4_request_reset_,
	.request_restart = cyttsp4_request_restart_,
	.request_set_mode = cyttsp4_request_set_mode_,
	.request_sysinfo = cyttsp4_request_sysinfo_,
	.request_firmware = cyttsp4_request_firmware_,
	.request_handshake = cyttsp4_request_handshake_,
	.request_exec_cmd = cyttsp4_request_exec_cmd_,
	.request_stop_wd = cyttsp4_request_stop_wd_,
	.request_toggle_lowpower = cyttsp4_request_toggle_lowpower_,
	.set_loader = cyttsp4_set_loader_,
	.unset_loader = cyttsp4_unset_loader_,
	.write = cyttsp4_write_,
	.read = cyttsp4_read_,
	.driver = {
		.name = CYTTSP4_CORE_NAME,
		.bus = &cyttsp4_bus_type,
		.owner = THIS_MODULE,
#ifndef CONFIG_HAS_EARLYSUSPEND
		.pm = &cyttsp4_core_pm_ops,
#endif
	},
};

static int __init cyttsp4_core_init(void)
{
	int rc = 0;

	rc = cyttsp4_register_core_driver(&cyttsp4_core_driver);
	pr_info("%s: Cypress TTSP v4 core driver (Built %s) rc=%d\n",
		 __func__, CY_DRIVER_DATE, rc);
	return rc;
}
module_init(cyttsp4_core_init);

static void __exit cyttsp4_core_exit(void)
{
	cyttsp4_unregister_core_driver(&cyttsp4_core_driver);
	pr_info("%s: module exit\n", __func__);
}
module_exit(cyttsp4_core_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard touchscreen core driver");
MODULE_AUTHOR("Aleksej Makarov <aleksej.makarov@sonyericsson.com>");
