/* drivers/input/touchscreen/melfas_ts.c
 *
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define SEC_TSP
#ifdef SEC_TSP
#define ENABLE_NOISE_TEST_MODE
#define TSP_FACTORY_TEST

#endif

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/melfas_ts.h>
#include <mach/cpufreq.h>
#include <mach/dev.h>

#ifdef CONFIG_INPUT_FBSUSPEND
#include <linux/fb.h>
#endif

#ifdef SEC_TSP
#include <linux/gpio.h>
#endif

#define TS_MAX_Z_TOUCH			255
#define TS_MAX_W_TOUCH		30
#define TS_MAX_ANGLE		90
#define TS_MIN_ANGLE		-90

#define TS_MAX_X_COORD		720
#define TS_MAX_Y_COORD		1280

#define FW_VERSION_4_8		0x74
#define FW_VERSION_4_65		0x61

#define TS_READ_START_ADDR	0x0F
#define TS_READ_START_ADDR2	0x10
#define TS_READ_VERSION_ADDR	0xF0
#define TS_READ_CONF_VERSION	0xF5
#define TSP_STATUS_ESD	0x0f

#ifdef SEC_TSP
#define TS_THRESHOLD			0x05
/* #define TS_READ_REGS_LEN		5 */
#define TS_WRITE_REGS_LEN		16
#endif

#define TS_READ_REGS_LEN		88
#define MELFAS_MAX_TOUCH		11

#define DEBUG_PRINT			0
#define	X_LINE					14
#define	Y_LINE					26

#define SET_DOWNLOAD_BY_GPIO	1

#define TSP_STATE_INACTIVE		-1
#define TSP_STATE_RELEASE		0
#define TSP_STATE_PRESS		1
#define TSP_STATE_MOVE		2

#if defined(CONFIG_SLP) || !(defined(CONFIG_EXYNOS4_CPUFREQ)\
	&& defined(CONFIG_BUSFREQ_OPP))
#define TOUCH_BOOSTER			0
#else
#define TOUCH_BOOSTER			1
#define TOUCH_BOOSTER_TIME		100
#endif

#undef CPURATE_DEBUG_FOR_TSP

#ifdef CPURATE_DEBUG_FOR_TSP
#define DbgOut(_x_) printk _x_
#else
#define DbgOut(_x_)
#endif

#if SET_DOWNLOAD_BY_GPIO
#include "mms100_ISP_download.h"
#endif

unsigned long saved_rate;
static bool lock_status;

static int tsp_enabled;
int touch_is_pressed;

struct device *sec_touchscreen;
static struct device *bus_dev;

struct muti_touch_info {
	int status;
	int strength;
	int width;
	int posX;
	int posY;
	int angle;
	int minor;
	int major;
	int palm;
};

struct melfas_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct melfas_tsi_platform_data *pdata;
#if TOUCH_BOOSTER
	struct delayed_work  dvfs_work;
#endif
	uint32_t flags;
	int (*power) (int on);
	void (*input_event)(void *data);
	void (*set_touch_i2c) (void);
	void (*set_touch_i2c_to_gpio) (void);
	struct early_suspend early_suspend;
	bool mt_protocol_b;
	bool enable_btn_touch;
#ifdef CONFIG_INPUT_FBSUSPEND
	struct notifier_block fb_notif;
	bool was_enabled_at_suspend;
#endif
#if defined(CONFIG_MACH_M0_CHNOPEN) ||					\
	defined(CONFIG_MACH_M0_CMCC) || defined(CONFIG_MACH_M0_CTC)
	int (*lcd_type)(void);
#endif
};

struct melfas_ts_data *ts_data;

#ifdef SEC_TSP
extern struct class *sec_class;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif

#ifdef SEC_TSP
static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int melfas_ts_resume(struct i2c_client *client);
#endif

#if TOUCH_BOOSTER
static bool dvfs_lock_status = false;
static bool press_status = false;
#endif

static struct muti_touch_info g_Mtouch_info[MELFAS_MAX_TOUCH];
static int firm_status_data;

static int melfas_init_panel(struct melfas_ts_data *ts)
{
	const char buf = 0x00;
	int ret;
	ret = i2c_master_send(ts->client, &buf, 1);

	ret = i2c_master_send(ts->client, &buf, 1);

	if (ret < 0) {
		pr_err("%s : i2c_master_send() failed\n [%d]",
		       __func__, ret);
		return 0;
	}

	return true;
}

#if TOUCH_BOOSTER
static void set_dvfs_off(struct work_struct *work)
{
	int ret;
	if (dvfs_lock_status && !press_status) {
		ret = dev_unlock(bus_dev, sec_touchscreen);
		if (ret < 0) {
			pr_err("%s: bus unlock failed(%d)\n",
			__func__, __LINE__);
			return;
		}
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		dvfs_lock_status = false;
#if DEBUG_PRINT
		pr_info("[TSP] TSP DVFS mode exit ");
#endif
	}
}
#endif

#ifdef SEC_TSP
static int
melfas_i2c_read(struct i2c_client *client, u16 addr, u8 length, u8 * value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 2;
	msg[0].buf = (u8 *) & addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = (u8 *) value;

	if (i2c_transfer(adapter, msg, 2) == 2)
		return 0;
	else
		return -EIO;

}

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length)
{
	int i;
	char data[TS_WRITE_REGS_LEN];

	if (length > TS_WRITE_REGS_LEN) {
		pr_err("[TSP] size error - %s\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < length; i++)
		data[i] = *buf++;

	i = i2c_master_send(client, (char *)data, length);

	if (i == length)
		return length;
	else
		return -EIO;
}

static void release_all_fingers(struct melfas_ts_data *ts)
{
	int i, ret;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		g_Mtouch_info[i].status = TSP_STATE_INACTIVE;
		g_Mtouch_info[i].strength = 0;
		g_Mtouch_info[i].posX = 0;
		g_Mtouch_info[i].posY = 0;
		g_Mtouch_info[i].angle = 0;
		g_Mtouch_info[i].major = 0;
		g_Mtouch_info[i].minor = 0;
		g_Mtouch_info[i].palm = 0;

		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev,
			MT_TOOL_FINGER, 0);
	}
	input_sync(ts->input_dev);
#if TOUCH_BOOSTER
	if (dvfs_lock_status) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		ret = dev_unlock(bus_dev, sec_touchscreen);
		if (ret < 0) {
			pr_err("%s: bus unlock failed(%d)\n",
			__func__, __LINE__);
			return;
		}
		dvfs_lock_status = false;
		press_status = false;
#if DEBUG_PRINT
		pr_info("[TSP] %s : DVFS mode exit\n", __func__);
#endif
	}
#endif
}

static void firmware_update(struct melfas_ts_data *ts)
{
	char buf[4] = { 0, };
	int ret = 0;
	u8 FW_VERSION;

#if SET_DOWNLOAD_BY_GPIO
	buf[0] = TS_READ_VERSION_ADDR;
	ret = i2c_master_send(ts->client, buf, 1);
	if (ret < 0) {
		pr_err("[TSP]%s: i2c_master_send [%d]\n", __func__,
		       ret);
	}

	ret = i2c_master_recv(ts->client, buf, 4);
	if (ret < 0) {
		pr_err("[TSP]%s: i2c_master_recv [%d]\n", __func__,
		       ret);
	}
	pr_info("[TSP] firmware_update");

#if defined(CONFIG_MACH_M0_CHNOPEN) || defined(CONFIG_MACH_M0_CTC)
	if (ts->lcd_type() == 0x20) {
		FW_VERSION = FW_VERSION_4_65;
		pr_info("[TSP] lcd type is 4.8, FW_VER: 0x%x\n", buf[3]);
	} else {
		FW_VERSION = FW_VERSION_4_8;
		pr_info("[TSP] lcd type is 4.65, FW_VER: 0x%x\n", buf[3]);
	}
#elif defined(CONFIG_MACH_M0_CMCC)
	if (ts->lcd_type() == 0x20) {
		FW_VERSION = FW_VERSION_4_8;
		pr_info("[TSP] lcd type is 4.8, FW_VER: 0x%x\n", buf[3]);
	} else {
		FW_VERSION = FW_VERSION_4_65;
		pr_info("[TSP] lcd type is 4.65, FW_VER: 0x%x\n", buf[3]);
	}
#else

#if defined(CONFIG_MACH_M0)
	if (system_rev == 2 || system_rev >= 5)
#else
	if (system_rev == 2 || system_rev >= 4)
#endif
		FW_VERSION = FW_VERSION_4_8;
	else
		FW_VERSION = FW_VERSION_4_65;
#endif

#if defined(CONFIG_MACH_M0_CHNOPEN) ||					\
	defined(CONFIG_MACH_M0_CMCC) || defined(CONFIG_MACH_M0_CTC)
	if (buf[3] != FW_VERSION || buf[3] == 0xFF) {
#else
	if (buf[3] < FW_VERSION || buf[3] == 0xFF) {
#endif
		ts->set_touch_i2c_to_gpio();
		pr_err("[TSP]FW Upgrading... FW_VERSION: 0x%02x\n",
		       buf[3]);

		ret = mms100_download(ts->pdata);

		if (ret != 0) {
			pr_err(
			       "[TSP]SET Download Fail - error code [%d]\n",
			       ret);
		}
		ts->set_touch_i2c();
		msleep(100);
		ts->power(0);
		msleep(200);
		ts->power(1);
		msleep(100);
	}
#endif				/* SET_DOWNLOAD_BY_GPIO */
}

static ssize_t
show_firm_version_phone(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	u8 FW_VERSION;

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	if (!tsp_enabled)
		return 0;

#if defined(CONFIG_MACH_M0_CHNOPEN) ||					\
	defined(CONFIG_MACH_M0_CMCC) || defined(CONFIG_MACH_M0_CTC)
	if (ts->lcd_type() == 0x20)
		FW_VERSION = FW_VERSION_4_65;
	else
		FW_VERSION = FW_VERSION_4_8;
#else

#if defined(CONFIG_MACH_M0)
	if (system_rev == 2 || system_rev >= 5)
#else
	if (system_rev == 2 || system_rev >= 4)
#endif
		FW_VERSION = FW_VERSION_4_8;
	else
		FW_VERSION = FW_VERSION_4_65;
#endif

	return sprintf(buf, "%#02x\n", FW_VERSION);
}

static ssize_t
show_firm_version_panel(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 tsp_version_disp;
	int ret;
	char buff[4] = { TS_READ_VERSION_ADDR, 0, };

	if (!tsp_enabled)
		return 0;
	ret = i2c_master_send(ts->client, (const char *)buff, 1);
	if (ret < 0) {
		pr_err("%s : i2c_master_send [%d]\n", __func__, ret);
	}

	ret = i2c_master_recv(ts->client, buff, 4);
	if (ret < 0) {
		pr_err("%s : i2c_master_recv [%d]\n", __func__, ret);
	}
	tsp_version_disp = buff[3];

	return sprintf(buf, "%#02x\n", tsp_version_disp);
}

static ssize_t
show_firm_conf_version(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 tsp_version_disp;
	int ret;
	char buff[4] = { TS_READ_CONF_VERSION, 0, };

	if (!tsp_enabled)
		return 0;
	ret = i2c_master_send(ts->client, (const char *)buff, 1);
	if (ret < 0)
		pr_err("%s : i2c_master_send [%d]\n", __func__, ret);

	ret = i2c_master_recv(ts->client, buff, 4);
	if (ret < 0)
		pr_err("%s : i2c_master_recv [%d]\n", __func__, ret);
	tsp_version_disp = buff[3];

	return sprintf(buf, "%#02x\n", tsp_version_disp);
}

static ssize_t
tsp_firm_update_mode(struct device *dev,
		     struct device_attribute *attr,
		     const char *buf, size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	int ret = 0;

	if (!tsp_enabled)
		return 0;

	disable_irq(ts->client->irq);

	firm_status_data = 1;

	ts->set_touch_i2c_to_gpio();

	pr_info("[TSP] ADB F/W UPDATE MODE ENTER!");
	if (*buf == 'S')
		ret = mms100_download(ts->pdata);
	else if (*buf == 'F')
		ret = mms100_download_file(ts->pdata);
	pr_info("[TSP] ADB F/W UPDATE MODE FROM %s END! %s",
		(*buf == 'S' ? "BINARY" : "FILE"), (ret ? "fail" : "success"));

	firm_status_data = (ret ? 3 : 2);

	ts->set_touch_i2c();
	release_all_fingers(ts);

	ts->power(0);
	msleep(200);
	ts->power(1);
	msleep(100);

	enable_irq(ts->client->irq);

	return count;
}

static ssize_t
show_threshold(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 threshold;

	if (!tsp_enabled)
		return 0;

	melfas_i2c_read(ts->client, TS_THRESHOLD, 1, &threshold);

	return sprintf(buf, "%d\n", threshold);
}

static ssize_t
show_firm_update_status(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int count;
	pr_info("[TSP] Enter firmware_status_show by Factory command\n");

	if (!tsp_enabled)
		return 0;

	if (firm_status_data == 1) {
		count = sprintf(buf, "DOWNLOADING\n");
	} else if (firm_status_data == 2) {
		count = sprintf(buf, "PASS\n");
	} else if (firm_status_data == 3) {
		count = sprintf(buf, "FAIL\n");
	} else
		count = sprintf(buf, "PASS\n");

	return count;
}

static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO,
		   show_firm_version_phone, NULL);
/* PHONE *//* firmware version resturn in phone driver version */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO,
		   show_firm_version_panel, NULL);
/*PART*//* firmware version resturn in TSP panel version */
static DEVICE_ATTR(tsp_firm_version_config, S_IRUGO,
		   show_firm_conf_version, NULL);
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO,
		   show_firm_update_status, NULL);
static DEVICE_ATTR(tsp_threshold, S_IRUGO, show_threshold, NULL);
static DEVICE_ATTR(tsp_firm_update, S_IWUSR | S_IWGRP, NULL,
		   tsp_firm_update_mode);

static struct attribute *sec_touch_attributes[] = {
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,
	&dev_attr_tsp_firm_version_config.attr,
	&dev_attr_tsp_firm_update_status.attr,
	&dev_attr_tsp_threshold.attr,
	&dev_attr_tsp_firm_update.attr,
	NULL,
};

static struct attribute_group sec_touch_attr_group = {
	.attrs = sec_touch_attributes,
};
#endif

#ifdef TSP_FACTORY_TEST
static bool debug_print = true;
static u16 index_reference;
static u16 inspection_data[X_LINE * Y_LINE] = { 0, };
static u16 intensity_data[X_LINE * Y_LINE] = { 0, };
static u16 reference_data[X_LINE * Y_LINE] = { 0, };

static ssize_t set_tsp_module_control(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	char write_buffer[2];

	if (*buf == '0' && tsp_enabled == true) {
		tsp_enabled = false;
		release_all_fingers(ts);
		ts->power(false);
		msleep(200);
	} else if (*buf == '1' && tsp_enabled == false) {
		ts->power(true);
		msleep(200);
		melfas_i2c_write(ts->client, (char *)write_buffer, 2);
		msleep(150);
		tsp_enabled = true;
	} else
		pr_info("[TSP]tsp_power_control bad command!");
	return count;
}

static int check_debug_data(struct melfas_ts_data *ts)
{
	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;
	int count = 0;

	disable_irq(ts->client->irq);
	/* enter the debug mode */
	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt */
	while (gpio_get_value(gpio)) {
		pr_info(".");
		udelay(100);
		count++;
		if (count == 100000) {
			enable_irq(ts->client->irq);
			return -1;
		}
	}

	if (debug_print)
		pr_info("[TSP] read dummy\n");

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);

	if (debug_print)
		pr_info("[TSP] read inspenction data\n");
	write_buffer[5] = 0x02;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;\
			exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);
			reference_data[exciting_line + sensing_line * Y_LINE] =
			    (read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	pr_info("[TSP] Reading data end.\n");

	msleep(200);
	melfas_ts_suspend(ts->client, PMSG_SUSPEND);

	msleep(200);
	melfas_ts_resume(ts->client);

	enable_irq(ts->client->irq);
	return 0;
}

static ssize_t
set_all_refer_mode_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int status = 0;
	int i;
	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	if (!tsp_enabled)
		return 0;

	for (i = 0; i < 3; i++) {
		if (!check_debug_data(ts)) {
			status = 0;
			break;
		} else {
			pr_info("[TSP] check_debug_data Error try=%d", i);
			msleep(200);
			melfas_ts_suspend(ts->client, PMSG_SUSPEND);

			msleep(200);
			melfas_ts_resume(ts->client);
			msleep(300);
			status = 1;
		}
	}
	if (!status) {
		for (i = 0; i < X_LINE * Y_LINE; i++) {
			/* out of range */
			if (reference_data[i] < 30) {
				status = 1;
				break;
			}

			if (debug_print) {
				if (0 == i % X_LINE)
					pr_info("[TSP]\n");
				pr_info("[TSP] %5u  ", reference_data[i]);
			}
		}
	} else {
		pr_info
		    ("[TSP] all_refer_show func & check_debug_data error[%d]",
		     status);
		return sprintf(buf, "%u\n", status);
	}

	pr_info("[TSP] all_refer_show func [%d]", status);
	return sprintf(buf, "%u\n", status);
}

static int index;

static void check_intensity_data(struct melfas_ts_data *ts, int num)
{
	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;
	int i = 0, ret;

	if (0 == reference_data[0]) {
		disable_irq(ts->client->irq);

		/* enter the debug mode */
		write_buffer[0] = 0xB0;
		write_buffer[1] = 0x1A;
		write_buffer[2] = 0x0;
		write_buffer[3] = 0x0;
		write_buffer[4] = 0x0;
		write_buffer[5] = 0x01;
		melfas_i2c_write(ts->client, (char *)write_buffer, 6);

		/* wating for the interrupt*/
		while (gpio_get_value(gpio)) {
			pr_info(".");
			udelay(100);
		}

		/* read the dummy data */
		melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);

		if (debug_print)
			pr_info("[TSP] read the dummy data\n");

		write_buffer[5] = 0x07;
		for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
			for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
				write_buffer[2] = exciting_line;
				write_buffer[3] = sensing_line;
				melfas_i2c_write(ts->client,
						(char *)write_buffer, 6);
				melfas_i2c_read(ts->client, 0xBF, 2,
								read_buffer);
				reference_data[exciting_line +
						sensing_line * Y_LINE] =
					(read_buffer[1] & 0xf) << 8
						| read_buffer[0];
			}
		}
		msleep(200);
		melfas_ts_suspend(ts->client, PMSG_SUSPEND);

		msleep(200);
		melfas_ts_resume(ts->client);

		msleep(100);
		enable_irq(ts->client->irq);
		msleep(100);
	}

	disable_irq(ts->client->irq);
	release_all_fingers(ts);

	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x04;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;\
			exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);
			intensity_data[exciting_line + sensing_line * Y_LINE] =
			    (read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	enable_irq(ts->client->irq);
}

#define SET_SHOW_FN(name, fn, format, ...)	\
static ssize_t show_##name(struct device *dev,	\
				     struct device_attribute *attr,	\
				     char *buf)		\
{	\
	struct melfas_ts_data *ts = dev_get_drvdata(dev);	\
	if ((NULL == ts) || !tsp_enabled) {		\
		printk(KERN_DEBUG "[TSP] drvdata is not set\n");\
		return sprintf(buf, "\n");	\
	}		\
	fn;	\
	return sprintf(buf, format "\n", ## __VA_ARGS__);	\
}

#define ATTR_SHOW_REF(num, node)	\
SET_SHOW_FN(set_refer##num,	\
	check_intensity_data(ts, num),	\
	"%u", node)

#define ATTR_SHOW_INTENSITY(num, node)	\
SET_SHOW_FN(set_intensity##num, ,	\
	"%u", node)

ATTR_SHOW_REF(0, reference_data[28]);
ATTR_SHOW_REF(1, reference_data[288]);
ATTR_SHOW_REF(2, reference_data[194]);
ATTR_SHOW_REF(3, reference_data[49]);
ATTR_SHOW_REF(4, reference_data[309]);

ATTR_SHOW_INTENSITY(0, intensity_data[28]);
ATTR_SHOW_INTENSITY(1, intensity_data[288]);
ATTR_SHOW_INTENSITY(2, intensity_data[194]);
ATTR_SHOW_INTENSITY(3, intensity_data[49]);
ATTR_SHOW_INTENSITY(4, intensity_data[309]);


static ssize_t show_tsp_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", MELFAS_TS_NAME);
}
static ssize_t show_tsp_x_line_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", X_LINE);
}

static ssize_t show_tsp_y_line_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", Y_LINE);
}
static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return -1;
	while (str[count] && str[count] >= '0' && str[count] <= '9') {
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}
static ssize_t set_debug_data1(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

/*	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data1 but TSP status OFF!");
		return count;
	}
*/
	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		pr_info(".");
		udelay(100);
	}

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);

	pr_info("[TSP] read Reference data\n");
	write_buffer[5] = 0x03;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			reference_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}

	ts->power(0);
	mdelay(200);
	ts->power(1);

	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}

static ssize_t set_debug_data2(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

/*	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data2 but TSP status OFF!");
		return count;
	}
*/
	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		pr_info(".");
		udelay(100);
	}

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);

	pr_info("[TSP] read Inspection data\n");
	write_buffer[5] = 0x02;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);
			inspection_data[exciting_line +
						sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	ts->power(0);
	mdelay(200);
	ts->power(1);

	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}

static ssize_t set_debug_data3(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

/*	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data3 but TSP status OFF!");
		return count;
	}
*/
	pr_info("[TSP] read lntensity data\n");

	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;

	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		pr_info(".");
		udelay(100);
	}

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);

	pr_info("[TSP] read Inspection data\n");
	write_buffer[5] = 0x04;

	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xBF, 2, read_buffer);
			intensity_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	ts->power(0);
	mdelay(200);
	ts->power(1);

	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}
static ssize_t set_index_reference(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	index_reference = atoi(buf);
	if (index_reference < 0 || index_reference >= X_LINE*Y_LINE){
		pr_info("[TSP] input bad index_reference value");
		return -1;
	}else{
		pr_info("[TSP]index_reference =%d ",index_reference);
		return count;
	}
}
static ssize_t show_reference_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (0 == i % Y_LINE)
				pr_info("\n");
			pr_info("%4u", reference_data[i]);
		}
	}
	return sprintf(buf, "%d\n", reference_data[index_reference]);
}
static ssize_t show_inspection_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (0 == i % Y_LINE)
				pr_info("\n");
			pr_info("%5u", inspection_data[i]);
		}
	}
	return sprintf(buf, "%d\n", inspection_data[index_reference]);
}
static ssize_t show_intensity_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (0 == i % Y_LINE)
				pr_info("\n");
			pr_info("%4u", intensity_data[i]);
		}
	}
	return sprintf(buf, "%d\n", intensity_data[index_reference]);
}

static DEVICE_ATTR(set_all_refer, S_IRUGO, set_all_refer_mode_show, NULL);
static DEVICE_ATTR(set_refer0, S_IRUGO, show_set_refer0, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO, show_set_intensity0, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO, show_set_refer1, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO, show_set_intensity1, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO, show_set_refer2, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO, show_set_intensity2, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO, show_set_refer3, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO, show_set_intensity3, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO, show_set_refer4, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO, show_set_intensity4, NULL);
static DEVICE_ATTR(set_threshold, S_IRUGO, show_threshold, NULL);
/* touch threshold return */
static DEVICE_ATTR(tsp_info, S_IRUGO, show_tsp_info, NULL);
static DEVICE_ATTR(tsp_x_line, S_IRUGO, show_tsp_x_line_info, NULL);
static DEVICE_ATTR(tsp_y_line, S_IRUGO, show_tsp_y_line_info, NULL);
static DEVICE_ATTR(tsp_module, S_IWUSR | S_IWGRP, NULL, set_tsp_module_control);
static DEVICE_ATTR(set_debug_data1,
		S_IWUSR | S_IWGRP | S_IRUGO, NULL, set_debug_data1);
static DEVICE_ATTR(set_debug_data2,
		S_IWUSR | S_IWGRP | S_IRUGO, NULL, set_debug_data2);
static DEVICE_ATTR(set_debug_data3,
		S_IWUSR | S_IWGRP | S_IRUGO, NULL, set_debug_data3);
static DEVICE_ATTR(set_index_ref, S_IWUSR
		| S_IWGRP, NULL, set_index_reference);
static DEVICE_ATTR(show_reference_info, S_IRUGO, show_reference_info, NULL);
static DEVICE_ATTR(show_inspection_info, S_IRUGO, show_inspection_info, NULL);
static DEVICE_ATTR(show_intensity_info, S_IRUGO, show_intensity_info, NULL);



static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_set_all_refer.attr,
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_set_threshold.attr,
	&dev_attr_tsp_info.attr,
	&dev_attr_tsp_x_line.attr,
	&dev_attr_tsp_y_line.attr,
	&dev_attr_tsp_module.attr,
	&dev_attr_set_debug_data1.attr,
	&dev_attr_set_debug_data2.attr,
	&dev_attr_set_debug_data3.attr,
	&dev_attr_set_index_ref.attr,
	&dev_attr_show_reference_info.attr,
	&dev_attr_show_inspection_info.attr,
	&dev_attr_show_intensity_info.attr,

	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif

void TSP_force_released(void)
{
	pr_err("%s satrt!\n", __func__);

	if (tsp_enabled == false) {
		pr_err("[TSP] Disabled\n");
		return;
	}
	release_all_fingers(ts_data);

	touch_is_pressed = 0;
}
EXPORT_SYMBOL(TSP_force_released);

#ifdef CONFIG_INPUT_FBSUSPEND
static int
melfas_fb_notifier_callback(struct notifier_block *self,
			    unsigned long event, void *fb_evdata)
{
	struct melfas_ts_data *data;
	struct fb_event *evdata = fb_evdata;
	int blank;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	data = container_of(self, struct melfas_ts_data, fb_notif);
	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
		if (tsp_enabled == 0) {
			data->power(true);
			enable_irq(data->client->irq);
			tsp_enabled = 1;
		} else {
			pr_err("[TSP] touchscreen already on\n");
		}
		break;
	case FB_BLANK_POWERDOWN:
		TSP_force_released();
		if (tsp_enabled == 1) {
			disable_irq(data->client->irq);
			data->power(false);
			tsp_enabled = 0;
		} else {
			pr_err("[TSP] touchscreen already off\n");
		}
		break;
	default:
		break;
	}
	return 0;
}

static int tsp_register_fb(struct melfas_ts_data *ts)
{
	memset(&ts->fb_notif, 0, sizeof(ts->fb_notif));
	ts->fb_notif.notifier_call = melfas_fb_notifier_callback;
	return fb_register_client(&ts->fb_notif);
}

static void tsp_unregister_fb(struct melfas_ts_data *ts)
{
	fb_unregister_client(&ts->fb_notif);
}
#endif

static void melfas_ts_get_data(struct work_struct *work)
{
	struct melfas_ts_data *ts =
	    container_of(work, struct melfas_ts_data, work);
	int ret = 0, i;
	int _touch_is_pressed;
	u8 read_num = 0, FingerID = 0;
	u8 buf[TS_READ_REGS_LEN];
	int pre_status = 0;

	ret = melfas_i2c_read(ts->client,
		TS_READ_START_ADDR, 1, &read_num);
	if (ret < 0) {
		pr_err("%s: i2c failed(%d)\n", __func__, __LINE__);
		return ;
	}

	if (read_num > 0) {
		ret = melfas_i2c_read(ts->client,
			TS_READ_START_ADDR2, read_num, buf);
		if (ret < 0) {
			pr_err("%s: i2c failed(%d)\n", \
				__func__, __LINE__);
			return ;
		}

		switch (buf[0]) {
		case TSP_STATUS_ESD:
			printk(KERN_DEBUG "[TSP] ESD protection.\n");
			disable_irq_nosync(ts->client->irq);
			ts->power(0);
			TSP_force_released();
			mdelay(200);
			ts->power(1);
			mdelay(200);
			enable_irq(ts->client->irq);
			return ;

		default:
			break;
		}

		if (read_num % 8 != 0) {
			pr_err("[TSP] incorrect read_num  %d\n", read_num);
			read_num = (read_num / 8) * 8;
		}

		for (i = 0; i < read_num; i = i + 8) {
			FingerID = (buf[i] & 0x0F) - 1;
			g_Mtouch_info[FingerID].posX =
			    (uint16_t) (buf[i + 1] & 0x0F) << 8 | buf[i + 2];
			g_Mtouch_info[FingerID].posY =
			    (uint16_t) (buf[i + 1] & 0xF0) << 4 | buf[i + 3];
#if !defined(CONFIG_MACH_C1) && \
	!defined(CONFIG_MACH_M3) && \
	!defined(CONFIG_MACH_M0) && \
	!defined(CONFIG_MACH_SLP_PQ) && \
	!defined(CONFIG_MACH_SLP_PQ_LTE)
			g_Mtouch_info[FingerID].posX =
			    720 - g_Mtouch_info[FingerID].posX;
			g_Mtouch_info[FingerID].posY =
			    1280 - g_Mtouch_info[FingerID].posY;
#endif
			g_Mtouch_info[FingerID].width = buf[i + 4];
			g_Mtouch_info[FingerID].angle =
			    (buf[i + 5] >=
			     127) ? (-(256 - buf[i + 5])) : buf[i + 5];
			g_Mtouch_info[FingerID].major = buf[i + 6];
			g_Mtouch_info[FingerID].minor = buf[i + 7];
			g_Mtouch_info[FingerID].palm = (buf[i] & 0x10) >> 4;
			pre_status = g_Mtouch_info[FingerID].status;
			if ((buf[i] & 0x80) == 0) {
				g_Mtouch_info[FingerID].strength = 0;
				g_Mtouch_info[FingerID].status =
					TSP_STATE_RELEASE;
			} else {
				g_Mtouch_info[FingerID].strength = buf[i + 4];

				if (TSP_STATE_PRESS == \
					g_Mtouch_info[FingerID].status)
					g_Mtouch_info[FingerID].status =
						TSP_STATE_MOVE;
				else
					g_Mtouch_info[FingerID].status =
						TSP_STATE_PRESS;
			}
			/*g_Mtouch_info[FingerID].width = buf[i + 5];*/
		}

	}

	_touch_is_pressed = 0;
	if (ret < 0) {
		pr_err("%s: i2c failed(%d)\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		if (TSP_STATE_INACTIVE == g_Mtouch_info[i].status)
			continue;

		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev,
			MT_TOOL_FINGER,
			!!g_Mtouch_info[i].strength);

		if (TSP_STATE_RELEASE == g_Mtouch_info[i].status) {
			g_Mtouch_info[i].status = TSP_STATE_INACTIVE;
			printk(KERN_DEBUG "[TSP] %d released\n", i);
			continue;
		}

			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					 g_Mtouch_info[i].posX);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					 g_Mtouch_info[i].posY);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
					 g_Mtouch_info[i].major);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR,
					 g_Mtouch_info[i].minor);

		if (ts->mt_protocol_b)
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
				g_Mtouch_info[i].width);
		else {
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
				g_Mtouch_info[i].width);
			input_report_key(ts->input_dev, BTN_TOUCH,
				!!g_Mtouch_info[i].strength);
			if (pre_status == -1)
				printk(KERN_DEBUG "[TSP] %d (%d, %d) %d\n",
					i, g_Mtouch_info[i].posX,
					g_Mtouch_info[i].posY,
					g_Mtouch_info[i].major);

/*			if ((TSP_STATE_PRESS == g_Mtouch_info[i].status))
				printk(KERN_DEBUG "[TSP] %d (%d, %d) %d\n",
					i, g_Mtouch_info[i].posX,
					g_Mtouch_info[i].posY,
					g_Mtouch_info[i].major);
			else if (TSP_STATE_RELEASE == g_Mtouch_info[i].status)
				printk(KERN_DEBUG "[TSP] %d released\n", i); */
		}

			input_report_abs(ts->input_dev, ABS_MT_ANGLE,
					 g_Mtouch_info[i].angle);
			input_report_abs(ts->input_dev, ABS_MT_PALM,
					 g_Mtouch_info[i].palm);
#if 0
			printk(KERN_DEBUG
			     "[TSP]melfas_ts_get_data: Touch ID: %d, "
			     "State : %d, x: %d, y: %d, major: %d "
			     "minor: %d w: %d a: %d p: %d\n",
			     i, (g_Mtouch_info[i].strength > 0),
			     g_Mtouch_info[i].posX, g_Mtouch_info[i].posY,
			     g_Mtouch_info[i].major, g_Mtouch_info[i].minor,
			     g_Mtouch_info[i].width, g_Mtouch_info[i].angle,
			     g_Mtouch_info[i].palm);
#endif
		if (g_Mtouch_info[i].strength > 0)
			_touch_is_pressed = 1;

	}

	input_sync(ts->input_dev);
	touch_is_pressed = _touch_is_pressed;

/*	if (touch_is_pressed > 0) {	*//* when touch is pressed. */
/*		if (lock_status == 0) {
			lock_status = 1;
		}
	} else {		*//* when touch is released. */
/*		if (read_num > 0) {
			lock_status = 0;
		}
	}*/

#if TOUCH_BOOSTER
		if (touch_is_pressed)
			press_status = true;
		else
			press_status = false;

		cancel_delayed_work(&ts->dvfs_work);
		schedule_delayed_work(&ts->dvfs_work,\
			msecs_to_jiffies(TOUCH_BOOSTER_TIME));

		if (!dvfs_lock_status && press_status) {
			ret = exynos_cpufreq_lock(DVFS_LOCK_ID_TSP, L7);
			if (ret < 0) {
				pr_err("%s: cpufreq lock failed(%d)\n",
					__func__, __LINE__);
				return;
			}

			ret = dev_lock(bus_dev, sec_touchscreen, 267160);
			if (ret < 0) {
				pr_err("%s: bus lock failed(%d)\n",
				__func__, __LINE__);
				return;
			}
			dvfs_lock_status = true;
#if DEBUG_PRINT
			printk(KERN_DEBUG"[TSP] TSP DVFS mode enter");
#endif
		}
#endif

#if DEBUG_PRINT
	if (ts->mt_protocol_b)
		pr_err("melfas_ts_get_data: touch_is_pressed=%d\n",
		       touch_is_pressed);
#endif
}

static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)handle;
#if DEBUG_PRINT
	pr_err("melfas_ts_irq_handler\n");
#endif

	if (ts->input_event)
		ts->input_event(ts);

	melfas_ts_get_data(&ts->work);

	return IRQ_HANDLED;
}

static int
melfas_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct melfas_ts_data *ts;
	struct melfas_tsi_platform_data *data = client->dev.platform_data;
#ifdef SEC_TSP
/*	struct device *sec_touchscreen; */
	struct device *tsp_noise_test;
#endif
	int ret = 0, i;
	char buf[4] = { 0, };

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kmalloc(sizeof(struct melfas_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		pr_err("%s: failed to create a state of melfas-ts\n",
		       __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	ts_data = ts;
	ts->pdata = client->dev.platform_data;
	data = client->dev.platform_data;
	ts->power = data->power;
	ts->mt_protocol_b = data->mt_protocol_b;
	ts->enable_btn_touch = data->enable_btn_touch;
	ts->set_touch_i2c = data->set_touch_i2c;
	ts->set_touch_i2c_to_gpio = data->set_touch_i2c_to_gpio;
	ts->input_event = data->input_event;

#if defined(CONFIG_MACH_M0_CHNOPEN) ||					\
	defined(CONFIG_MACH_M0_CMCC) || defined(CONFIG_MACH_M0_CTC)
	ts->lcd_type = data->lcd_type;
#endif

	ts->power(0);
	ts->power(true);
	msleep(100);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	ret = i2c_master_send(ts->client, buf, 1);
	if (ret < 0) {
#if DEBUG_PRINT
		pr_err("%s: i2c_master_send() [%d], Add[%d]\n",
		       __func__, ret, ts->client->addr);
#endif
	}

	firmware_update(ts);

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		pr_err("%s: Not enough memory\n", __func__);
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->name = "sec_touchscreen";
	__set_bit(EV_ABS, ts->input_dev->evbit);
	__set_bit(EV_KEY, ts->input_dev->evbit);
	__set_bit(BTN_TOUCH, ts->input_dev->keybit);

	if (ts->enable_btn_touch) {
		input_set_abs_params(ts->input_dev, ABS_X, 0,
			TS_MAX_X_COORD, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_Y, 0,
			TS_MAX_Y_COORD, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,
			TS_MAX_X_COORD, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0,
			TS_MAX_Y_COORD, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0,
			TS_MAX_Z_TOUCH, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0,
			TS_MAX_W_TOUCH, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_ANGLE,
			TS_MIN_ANGLE, TS_MAX_ANGLE, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_PALM,
			0, 1, 0, 0);
		input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
	} else {
		input_mt_init_slots(ts->input_dev,
			MELFAS_MAX_TOUCH - 1);
		input_set_abs_params(ts->input_dev,
			ABS_MT_POSITION_X, 0, TS_MAX_X_COORD - 1, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_POSITION_Y, 0, TS_MAX_Y_COORD - 1, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_TOUCH_MAJOR, 0, TS_MAX_Z_TOUCH, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_TOUCH_MINOR, 0, TS_MAX_Z_TOUCH, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_WIDTH_MAJOR, 0, TS_MAX_W_TOUCH, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_ANGLE, TS_MIN_ANGLE, TS_MAX_ANGLE, 0, 0);
		input_set_abs_params(ts->input_dev,
			ABS_MT_PALM, 0, 1, 0, 0);

		__set_bit(MT_TOOL_FINGER, ts->input_dev->keybit);
		__set_bit(EV_SYN, ts->input_dev->evbit);
		__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	}

	ret = input_register_device(ts->input_dev);
	if (ret) {
		pr_err("%s: Failed to register device\n", __func__);
		ret = -ENOMEM;
		goto err_input_register_device_failed;
	}

#if TOUCH_BOOSTER
	INIT_DELAYED_WORK(&ts->dvfs_work, set_dvfs_off);
	bus_dev = dev_get("exynos-busfreq");
#endif

	if (ts->client->irq) {
#if DEBUG_PRINT
		pr_err("%s: trying to request irq: %s-%d\n", __func__,
		       ts->client->name, ts->client->irq);
#endif
		ret =
		    request_threaded_irq(client->irq, NULL,
					 melfas_ts_irq_handler,
					 IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					 ts->client->name, ts);
		if (ret > 0) {
			pr_err("%s: Can't allocate irq %d, ret %d\n",
			       __func__, ts->client->irq, ret);
			ret = -EBUSY;
			goto err_request_irq;
		}
	}

	for (i = 0; i < MELFAS_MAX_TOUCH; i++)	/* _SUPPORT_MULTITOUCH_ */
		g_Mtouch_info[i].status = TSP_STATE_INACTIVE;

	tsp_enabled = true;

#if DEBUG_PRINT
	pr_err("%s: succeed to register input device\n", __func__);
#endif

#if 1				/* 0//SEC_TSP */
	sec_touchscreen =
	    device_create(sec_class, NULL, 0, ts, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen))
		pr_err("[TSP] Failed to create device for the sysfs\n");

	ret = sysfs_create_group(&sec_touchscreen->kobj, \
		&sec_touch_attr_group);
	if (ret)
		pr_err("[TSP] Failed to create sysfs group\n");
#endif

#if 1				/* 0//TSP_FACTORY_TEST */
	tsp_noise_test =
	    device_create(sec_class, NULL, 0, ts, "tsp_noise_test");
	if (IS_ERR(tsp_noise_test))
		pr_err("[TSP] Failed to create device for the sysfs\n");

	ret =
	    sysfs_create_group(&tsp_noise_test->kobj,
			       &sec_touch_factory_attr_group);
	if (ret)
		pr_err("[TSP] Failed to create sysfs group\n");
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = melfas_ts_early_suspend;
	ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

#ifdef CONFIG_INPUT_FBSUSPEND
	ret = tsp_register_fb(ts);
	if (ret)
		pr_err("[TSP] Failed to register fb\n");
#endif

#if DEBUG_PRINT
	pr_info("%s: Start touchscreen. name: %s, irq: %d\n", __func__,
	       ts->client->name, ts->client->irq);
#endif
	return 0;

 err_request_irq:
	pr_err("melfas-ts: err_request_irq failed\n");
 err_input_register_device_failed:
	pr_err("melfas-ts: err_input_register_device failed\n");
	input_free_device(ts->input_dev);
 err_input_dev_alloc_failed:
	pr_err("melfas-ts: err_input_dev_alloc failed\n");
	kfree(ts);
 err_alloc_data_failed:
	pr_err("melfas-ts: err_alloc_data failed_\n");
 err_check_functionality_failed:
	pr_err("melfas-ts: err_check_functionality failed_\n");

	return ret;
}

static int melfas_ts_remove(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);

	unregister_early_suspend(&ts->early_suspend);
#ifdef CONFIG_INPUT_FBSUSPEND
	tsp_unregister_fb(ts);
#endif
	free_irq(client->irq, ts);
	ts->power(false);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	if (!tsp_enabled) {
#ifdef CONFIG_INPUT_FBSUSPEND
		ts->was_enabled_at_suspend = false;
#endif
		return 0;
	}

#ifdef CONFIG_INPUT_FBSUSPEND
	ts->was_enabled_at_suspend = true;
#endif

	disable_irq(client->irq);
	tsp_enabled = false;
	release_all_fingers(ts);
	ts->power(false);
	return 0;
}

static int melfas_ts_resume(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	if (tsp_enabled)
		return 0;
#ifdef CONFIG_INPUT_FBSUSPEND
	if (!ts->was_enabled_at_suspend)
		return 0;
#endif

	ts->power(true);
	msleep(100);

	/* Because irq_type by EXT_INTxCON register is changed to low_level
	 *  after wakeup, irq_type set to falling edge interrupt again.
	*/
	irq_set_irq_type(client->irq, IRQ_TYPE_EDGE_FALLING);
	enable_irq(client->irq);	/* scl wave */
	tsp_enabled = true;
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
	struct melfas_ts_data *ts;

	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void melfas_ts_late_resume(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id melfas_ts_id[] = {
	{MELFAS_TS_NAME, 0},
	{}
};

static struct i2c_driver melfas_ts_driver = {
	.driver = {
		   .name = MELFAS_TS_NAME,
		   },
	.id_table = melfas_ts_id,
	.probe = melfas_ts_probe,
	.remove = __devexit_p(melfas_ts_remove),
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend = melfas_ts_suspend,
	.resume = melfas_ts_resume,
#endif
};

#ifdef CONFIG_BATTERY_SEC
extern unsigned int is_lpcharging_state(void);
#endif

static int __devinit melfas_ts_init(void)
{
#ifdef CONFIG_BATTERY_SEC
	if (is_lpcharging_state()) {
		pr_info("%s : LPM Charging Mode! return 0\n", __func__);
		return 0;
	}
#endif

	return i2c_add_driver(&melfas_ts_driver);
}

static void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_driver);
}

MODULE_DESCRIPTION("Driver for Melfas MTSI Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

module_init(melfas_ts_init);
module_exit(melfas_ts_exit);
