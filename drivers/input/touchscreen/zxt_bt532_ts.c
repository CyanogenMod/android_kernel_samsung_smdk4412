/*
 *
 * Zinitix bt532 touchscreen driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */


#define SEC_FACTORY_TEST
#define SUPPORTED_TOUCH_KEY

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ioctl.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#if defined(CONFIG_PM_RUNTIME)
#include <linux/pm_runtime.h>
#endif
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
/*#include <mach/gpio.h>*/
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/input/mt.h>

#include <linux/i2c/bt532_ts.h>
#include "zxt_bt532_fw.h"

#define ZINITIX_DEBUG				1

/* added header file */

#include "zxt_bt532_ts.h"

#define zinitix_debug_msg(fmt, args...) \
	do { \
	if (m_ts_debug_mode) \
	printk(KERN_INFO "bt532_ts[%-18s:%5d] " fmt, \
	__func__, __LINE__, ## args); \
	} while (0);

#define zinitix_printk(fmt, args...) \
	do { \
	printk(KERN_INFO "bt532_ts[%-18s:%5d] " fmt, \
	__func__, __LINE__, ## args); \
	} while (0);

#define bt532_err(fmt) \
	do { \
	pr_err("bt532_ts : %s " fmt, __func__); \
	} while (0);

#ifdef SEC_FACTORY_TEST
/* Touch Screen */
#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN	512
#define TSP_CMD_PARAM_NUM		8
#define TSP_CMD_Y_NUM			18
#define TSP_CMD_X_NUM			30
#define TSP_CMD_NODE_NUM		(TSP_CMD_Y_NUM * TSP_CMD_X_NUM)

struct tsp_factory_info {
	struct list_head cmd_list_head;
	char cmd[TSP_CMD_STR_LEN];
	char cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	char cmd_buff[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;
	u8 cmd_state;
};

struct tsp_raw_data {
	s16 ref_data[TSP_CMD_NODE_NUM];
	s16 scantime_data[TSP_CMD_NODE_NUM];
	s16 delta_data[TSP_CMD_NODE_NUM];
};

enum {
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
};

struct tsp_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void not_support_cmd(void *device_data);

/* Vendor dependant command */
static void run_reference_read(void *device_data);
static void get_reference(void *device_data);
/*
static void run_scantime_read(void *device_data);
static void get_scantime(void *device_data);
*/
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);

#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

static struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", module_off_slave),},
	{TSP_CMD("module_on_slave", module_on_slave),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},

	/* vendor dependant command */
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("get_reference", get_reference),},
/*
	{TSP_CMD("run_scantime_read", run_scantime_read),},
	{TSP_CMD("get_scantime", get_scantime),},
*/
	{TSP_CMD("run_delta_read", run_delta_read),},
	{TSP_CMD("get_delta", get_delta),},
};

#ifdef SUPPORTED_TOUCH_KEY
/* Touch Key */
static ssize_t touchkey_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touch_sensitivity(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_back(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_menu(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t autocal_stat(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_raw_back(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_raw_menu(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_idac_back(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t touchkey_idac_menu(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
#endif
#endif

#define TSP_NORMAL_EVENT_MSG 1
static int m_ts_debug_mode = ZINITIX_DEBUG;

#if ESD_TIMER_INTERVAL
static struct workqueue_struct *esd_tmr_workqueue;
#endif

struct coord {
	u16	x;
	u16	y;
	u8	width;
	u8	sub_status;
#if (TOUCH_POINT_MODE == 2)
	u8	minor_width;
	u8	angle;
#endif
};

struct point_info {
	u16	status;
#if (TOUCH_POINT_MODE == 1)
	u16	event_flag;
#else
	u8	finger_cnt;
	u8	time_stamp;
#endif
	struct coord coord[MAX_SUPPORTED_FINGER_NUM];
};

#define TOUCH_V_FLIP	0x01
#define TOUCH_H_FLIP	0x02
#define TOUCH_XY_SWAP	0x04

struct capa_info {
	u16	vendor_id;
	u16	ic_revision;
	u16	fw_version;
	u16	fw_minor_version;
	u16	reg_data_version;
	u16	threshold;
	u32	ic_fw_size;
	u32	x_max;
	u32	y_max;
	u32	x_min;
	u32	y_min;
	u8	gesture_support;
	u16	multi_fingers;
	u16	button_num;
	u16	ic_int_mask;
	u16	x_node_num;
	u16	y_node_num;
	u16	total_node_num;
	u16	hw_id;
	u16	afe_frequency;
};

enum work_state {
	NOTHING = 0,
	NORMAL,
	ESD_TIMER,
	EALRY_SUSPEND,
	SUSPEND,
	RESUME,
	LATE_RESUME,
	UPGRADE,
	REMOVE,
	SET_MODE,
	HW_CALIBRAION,
	RAW_DATA,
};

enum {
	BUILT_IN = 0,
	UMS,
	REQ_FW,
};

struct bt532_ts_data {
	struct i2c_client				*client;
	struct input_dev				*input_dev;
	struct bt532_ts_platform_data	*pdata;
	struct capa_info				cap_info;
	struct point_info				touch_info;
	struct point_info				reported_touch_info;
	struct semaphore				work_lock;
	char							phys[32];
	struct semaphore				raw_data_lock;
	u16								touch_mode;
	s16								cur_data[MAX_TRAW_DATA_SZ];
	u16								icon_event_reg;
	u32								irq;
	u8								update;
	u8								button[MAX_SUPPORTED_BUTTON_NUM];
	u8								work_state;
	
	/*struct task_struct			*task;*/
	/*wait_queue_head_t				wait;*/
	/*struct semaphore				update_lock;*/
	/*u32							i2c_dev_addr;*/
	/*u16							event_type;*/
	/*u16							debug_reg[8];*/ /* for debug */
#if ESD_TIMER_INTERVAL
	struct work_struct				tmr_work;
	struct timer_list				esd_timeout_tmr;
	struct timer_list				*p_esd_timeout_tmr;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend			early_suspend;
#endif

#ifdef SEC_FACTORY_TEST
	struct tsp_factory_info			*factory_info;
	struct tsp_raw_data				*raw_data;
#endif
};

u32 BUTTON_MAPPING_KEY[MAX_SUPPORTED_BUTTON_NUM] = {
	KEY_DUMMY_MENU, KEY_MENU, KEY_DUMMY_HOME1,
	KEY_DUMMY_HOME2, KEY_BACK, KEY_DUMMY_BACK};

/* define i2c sub functions*/
static inline s32 read_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	/* select register*/
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0)
		return ret;
	/* for setup tx transaction. */
	udelay(DELAY_FOR_TRANSCATION);
	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 write_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	u8 pkt[10];	/* max packet */
	pkt[0] = (reg)&0xff;	/* reg addr */
	pkt[1] = (reg >> 8)&0xff;
	memcpy((u8 *)&pkt[2], values, length);

	ret = i2c_master_send(client , pkt , length+2);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 write_reg(struct i2c_client *client, u16 reg, u16 value)
{
	if (write_data(client, reg, (u8 *)&value, 2) < 0)
		return I2C_FAIL;

	return I2C_SUCCESS;
}

static inline s32 write_cmd(struct i2c_client *client, u16 reg)
{
	s32 ret;

	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0)
		return I2C_FAIL;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return I2C_SUCCESS;
}

static inline s32 read_raw_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	s32 ret;
	/* select register */

	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0)
		return ret;

	/* for setup tx transaction. */
	udelay(200);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 read_firmware_data(struct i2c_client *client,
	u16 addr, u8 *values, u16 length)
{
	s32 ret;
	/* select register*/

	ret = i2c_master_send(client , (u8 *)&addr , 2);
	if (ret < 0)
		return ret;

	/* for setup tx transaction. */
	mdelay(1);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bt532_ts_early_suspend(struct early_suspend *h);
static void bt532_ts_late_resume(struct early_suspend *h);
#endif

static bool bt532_power_control(struct bt532_ts_data *touch_dev, u8 ctl);

static bool init_touch(struct bt532_ts_data *touch_dev);
static bool mini_init_touch(struct bt532_ts_data *touch_dev);
static void clear_report_data(struct bt532_ts_data *touch_dev);
static void esd_timer_start(u16 sec, struct bt532_ts_data *touch_dev);
static void esd_timer_stop(struct bt532_ts_data *touch_dev);
static void esd_timer_init(struct bt532_ts_data *touch_dev);
static void esd_timeout_handler(unsigned long data);

static long ts_misc_fops_ioctl(struct file *filp, unsigned int cmd,
								unsigned long arg);
static int ts_misc_fops_open(struct inode *inode, struct file *filp);
static int ts_misc_fops_close(struct inode *inode, struct file *filp);

static const struct file_operations ts_misc_fops = {
	.owner = THIS_MODULE,
	.open = ts_misc_fops_open,
	.release = ts_misc_fops_close,
	.unlocked_ioctl = ts_misc_fops_ioctl,
};

static struct miscdevice touch_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "zinitix_touch_misc",
	.fops = &ts_misc_fops,
};

#define TOUCH_IOCTL_BASE	0xbc
#define TOUCH_IOCTL_GET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 0, int)
#define TOUCH_IOCTL_SET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 1, int)
#define TOUCH_IOCTL_GET_CHIP_REVISION		_IOW(TOUCH_IOCTL_BASE, 2, int)
#define TOUCH_IOCTL_GET_FW_VERSION			_IOW(TOUCH_IOCTL_BASE, 3, int)
#define TOUCH_IOCTL_GET_REG_DATA_VERSION	_IOW(TOUCH_IOCTL_BASE, 4, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_SIZE		_IOW(TOUCH_IOCTL_BASE, 5, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_DATA		_IOW(TOUCH_IOCTL_BASE, 6, int)
#define TOUCH_IOCTL_START_UPGRADE			_IOW(TOUCH_IOCTL_BASE, 7, int)
#define TOUCH_IOCTL_GET_X_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 8, int)
#define TOUCH_IOCTL_GET_Y_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 9, int)
#define TOUCH_IOCTL_GET_TOTAL_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 10, int)
#define TOUCH_IOCTL_SET_RAW_DATA_MODE		_IOW(TOUCH_IOCTL_BASE, 11, int)
#define TOUCH_IOCTL_GET_RAW_DATA			_IOW(TOUCH_IOCTL_BASE, 12, int)
#define TOUCH_IOCTL_GET_X_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 13, int)
#define TOUCH_IOCTL_GET_Y_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 14, int)
#define TOUCH_IOCTL_HW_CALIBRAION			_IOW(TOUCH_IOCTL_BASE, 15, int)
#define TOUCH_IOCTL_GET_REG					_IOW(TOUCH_IOCTL_BASE, 16, int)
#define TOUCH_IOCTL_SET_REG					_IOW(TOUCH_IOCTL_BASE, 17, int)
#define TOUCH_IOCTL_SEND_SAVE_STATUS		_IOW(TOUCH_IOCTL_BASE, 18, int)
#define TOUCH_IOCTL_DONOT_TOUCH_EVENT		_IOW(TOUCH_IOCTL_BASE, 19, int)

struct bt532_ts_data *misc_info;

static bool get_raw_data(struct bt532_ts_data *touch_dev, u8 *buff, int skip_cnt)
{
	struct i2c_client *client = touch_dev->client;
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;
	/*struct tsp_raw_data *raw_data = touch_dev->raw_data;*/
	u32 total_node = touch_dev->cap_info.total_node_num;
	u32 sz;
	int i;

	disable_irq(touch_dev->irq);

	down(&touch_dev->work_lock);
	if (touch_dev->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			touch_dev->work_state);
		enable_irq(touch_dev->irq);
		up(&touch_dev->work_lock);
		return false;
		}

	touch_dev->work_state = RAW_DATA;

	for(i = 0; i < skip_cnt; i++) {
		while (gpio_get_value(pdata->gpio_int))
			msleep(1);

		write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
		msleep(1);
	}

	zinitix_debug_msg("read raw data\r\n");
	sz = total_node*2;

	while (gpio_get_value(pdata->gpio_int))
		msleep(1);

	if (read_raw_data(client, ZXT_RAWDATA_REG, (char *)buff, sz) < 0) {
		zinitix_printk("error : read zinitix tc raw data\n");
		touch_dev->work_state = NOTHING;
		enable_irq(touch_dev->irq);
		up(&touch_dev->work_lock);
		return false;
	}

	write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
	touch_dev->work_state = NOTHING;
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);

	return true;
}

static bool ts_get_raw_data(struct bt532_ts_data *touch_dev)
{
	struct i2c_client *client = touch_dev->client;
	u32 total_node = touch_dev->cap_info.total_node_num;
	u32 sz;

	if (down_trylock(&touch_dev->raw_data_lock)) {
		dev_err(&client->dev, "Failed to occupy sema\n");
		touch_dev->touch_info.status = 0;
		return true;
	}

	sz = total_node * 2 + sizeof(struct point_info);

	if (read_raw_data(touch_dev->client, ZXT_RAWDATA_REG,
			(char *)touch_dev->cur_data, sz) < 0) {
		dev_err(&client->dev, "Failed to read raw data\n");
		up(&touch_dev->raw_data_lock);
		return false;
	}

	touch_dev->update = 1;
	memcpy((u8 *)(&touch_dev->touch_info),
		(u8 *)&touch_dev->cur_data[total_node],
		sizeof(struct point_info));
	up(&touch_dev->raw_data_lock);

	return true;
}

static bool ts_read_coord(struct bt532_ts_data *touch_dev)
{
	/*struct bt532_ts_platform_data *pdata = touch_dev->pdata;*/
	struct i2c_client *client = touch_dev->client;
#if (TOUCH_POINT_MODE == 1)
	int i;
#endif

	/* zinitix_debug_msg("ts_read_coord+\r\n"); */

	/* for  Debugging Tool */

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) {
		if (ts_get_raw_data(touch_dev) == false)
			return false;

		dev_err(&client->dev, "status = 0x%04X\n", touch_dev->touch_info.status);

		goto out;
	}

#if (TOUCH_POINT_MODE == 1)
	memset(&touch_dev->touch_info,
			0x0, sizeof(struct point_info));

	if (read_data(touch_dev->client, ZXT_POINT_STATUS_REG,
			(u8 *)(&touch_dev->touch_info), 4) < 0) {
		dev_err(&client->dev, "%s: Failed to read point touch_dev\n", __func__);

		return false;
	}

	dev_info(&client->dev, "status reg = 0x%x , event_flag = 0x%04x\n",
				touch_dev->touch_info.status, touch_dev->touch_info.event_flag);

	if (touch_dev->touch_info.event_flag == 0)
		goto out;

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		if (zinitix_bit_test(touch_dev->touch_info.event_flag, i)) {
			udelay(20);

			if (read_data(touch_dev->client, ZXT_POINT_STATUS_REG + 2 + ( i * 4),
					(u8 *)(&touch_dev->touch_info.coord[i]),
					sizeof(struct coord)) < 0) {
				dev_err(&client->dev, "Failed to read point touch_dev\n");

				return false;
			}
		}
	}

#else
	if (read_data(touch_dev->client, ZXT_POINT_STATUS_REG,
			(u8 *)(&touch_dev->touch_info), sizeof(struct point_info)) < 0) {
		dev_err(&client->dev, "Failed to read point touch_dev\n");

		return false;
	}
#endif

out:
	/* error */
	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_MUST_ZERO)) {
		dev_err(&client->dev, "Invalid must zero bit(%04x)\n",
			touch_dev->touch_info.status);
		/*write_cmd(touch_dev->client, ZXT_CLEAR_INT_STATUS_CMD);
		udelay(DELAY_FOR_SIGNAL_DELAY);*/
		return false;
	}
/*
	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_ICON_EVENT)) {
		udelay(20);
		if (read_data(touch_dev->client,
			ZXT_ICON_STATUS_REG,
			(u8 *)(&touch_dev->icon_event_reg), 2) < 0) {
			zinitix_printk("error read icon touch_dev using i2c.\n");
			return false;
		}
	}
*/
	write_cmd(touch_dev->client, ZXT_CLEAR_INT_STATUS_CMD);
	/* udelay(DELAY_FOR_SIGNAL_DELAY); */
	/* zinitix_debug_msg("ts_read_coord-\r\n"); */
	return true;
}

#if ESD_TIMER_INTERVAL
static void esd_timeout_handler(unsigned long data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)data;

	if (touch_dev == NULL)
		return;

	touch_dev->p_esd_timeout_tmr = NULL;
	queue_work(esd_tmr_workqueue, &touch_dev->tmr_work);
}

static void esd_timer_start(u16 sec, struct bt532_ts_data *touch_dev)
{
	if (touch_dev == NULL)
		return;

	if (touch_dev->p_esd_timeout_tmr != NULL)
		del_timer(touch_dev->p_esd_timeout_tmr);

	touch_dev->p_esd_timeout_tmr = NULL;
	init_timer(&(touch_dev->esd_timeout_tmr));
	touch_dev->esd_timeout_tmr.data = (unsigned long)(touch_dev);
	touch_dev->esd_timeout_tmr.function = esd_timeout_handler;
	touch_dev->esd_timeout_tmr.expires = jiffies + HZ*sec;
	touch_dev->p_esd_timeout_tmr = &touch_dev->esd_timeout_tmr;
	add_timer(&touch_dev->esd_timeout_tmr);
}

static void esd_timer_stop(struct bt532_ts_data *touch_dev)
{
	if (touch_dev == NULL)
			return;

	if (touch_dev->p_esd_timeout_tmr)
		del_timer(touch_dev->p_esd_timeout_tmr);

	touch_dev->p_esd_timeout_tmr = NULL;
}

static void esd_timer_init(struct bt532_ts_data *touch_dev)
{
	if (touch_dev == NULL)
			return;

	init_timer(&(touch_dev->esd_timeout_tmr));
	touch_dev->esd_timeout_tmr.data = (unsigned long)(touch_dev);
	touch_dev->esd_timeout_tmr.function = esd_timeout_handler;
	touch_dev->p_esd_timeout_tmr = NULL;
}

static void ts_tmr_work(struct work_struct *work)
{
	struct bt532_ts_data *touch_dev =
		container_of(work, struct bt532_ts_data, tmr_work);

	zinitix_printk("tmr queue work ++\n");
	if (touch_dev == NULL) {
		zinitix_printk("touch_dev == NULL ?\n");
		goto fail_time_out_init;
	}

	if(down_trylock(&touch_dev->work_lock)) {
		zinitix_printk("failed to occupy sema\r\n");
		esd_timer_start(CHECK_ESD_TIMER, touch_dev);
		return;
	}

	if (touch_dev->work_state != NOTHING) {
		zinitix_printk("other process occupied (%d)\r\n",
			touch_dev->work_state);
		up(&touch_dev->work_lock);
		return;
	}
	touch_dev->work_state = ESD_TIMER;

	disable_irq(touch_dev->irq);
	zinitix_printk("error. timeout occured. maybe ts device dead. so reset & reinit.\r\n");
	bt532_power_control(touch_dev, POWER_OFF);
	bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

	zinitix_debug_msg("clear all reported points\r\n");
	clear_report_data(touch_dev);
	if (mini_init_touch(touch_dev) == false)
		goto fail_time_out_init;

	touch_dev->work_state = NOTHING;
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);
	zinitix_printk("tmr queue work ----\n");
	return;
fail_time_out_init:
	zinitix_printk("tmr work : restart error\n");
	esd_timer_start(CHECK_ESD_TIMER, touch_dev);
	touch_dev->work_state = NOTHING;
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);
}
#endif

static bool bt532_power_sequence(struct bt532_ts_data *touch_dev)
{
	struct i2c_client *client = touch_dev->client;
	int retry = 0;
	u16 chip_code;

retry_power_sequence:
	if (write_reg(client, 0xc000, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(vendor cmd enable)\n");
		goto fail_power_sequence;
	}
	udelay(10);

	if (read_data(client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		dev_err(&client->dev, "Failed to read chip code\n");
		goto fail_power_sequence;
	}

	dev_info(&client->dev, "%s: chip code = 0x%x\n", __func__, chip_code);
	udelay(10);

	if (write_cmd(client, 0xc004) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(intn clear)\n");
		goto fail_power_sequence;
	}
	udelay(10);

	if (write_reg(client, 0xc002, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(nvm init)\n");
		goto fail_power_sequence;
	}
	mdelay(2);

	if (write_reg(client, 0xc001, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(program start)\n");
		goto fail_power_sequence;
	}
	msleep(FIRMWARE_ON_DELAY);	/* wait for checksum cal */

	return true;

fail_power_sequence:
	if (retry++ < 3) {
		msleep(CHIP_ON_DELAY);
		dev_info(&client->dev, "retry = %d\n", retry);
		goto retry_power_sequence;
	}

	return false;
}

static bool bt532_power_control(struct bt532_ts_data *touch_dev, u8 ctl)
{
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;
	bool ret = true;

	pdata->power((bool)ctl);

	if (ctl)
		msleep(CHIP_ON_DELAY);
	else
		msleep(CHIP_OFF_DELAY);

	if (ctl == POWER_ON_SEQUENCE)
		ret = bt532_power_sequence(touch_dev);

	return ret;
}

#if TOUCH_ONESHOT_UPGRADE
static bool ts_check_need_upgrade(struct bt532_ts_data *touch_dev,
	u16 cur_ver, u16 cur_minor_ver, u16 cur_reg_ver, u16 cur_hw_id)
{
	u16	new_ver;
	u16	new_minor_ver;
	u16	new_reg_ver;
	u16	new_chip_code;
	u16	new_hw_id;

	new_ver = (u16) (m_firmware_data[52] | (m_firmware_data[53]<<8));
	new_minor_ver = (u16) (m_firmware_data[56] | (m_firmware_data[57]<<8));
	new_reg_ver = (u16) (m_firmware_data[60] | (m_firmware_data[61]<<8));
	new_chip_code = (u16) (m_firmware_data[64] | (m_firmware_data[65]<<8));

	new_hw_id = (u16) (m_firmware_data[0x6b12] | (m_firmware_data[0x6b13]<<8));

#if CHECK_HWID
	zinitix_printk("cur HW_ID = 0x%x, new HW_ID = 0x%x\n",
		cur_hw_id, new_hw_id);
	if (cur_hw_id != new_hw_id)
		return false;
#endif

	zinitix_printk("cur ver = 0x%x, new ver = 0x%x\n",
		cur_ver, new_ver);
	if (cur_ver < new_ver)
		return true;
	else if (cur_ver > new_ver)
		return false;

	zinitix_printk("cur minor ver = 0x%x, new minor ver = 0x%x\n",
			cur_minor_ver, new_minor_ver);
	if (cur_minor_ver < new_minor_ver)
		return true;
	else if (cur_minor_ver > new_minor_ver)
		return false;

	zinitix_printk("cur reg data ver = 0x%x, new reg data ver = 0x%x\n",
			cur_reg_ver, new_reg_ver);
	if (cur_reg_ver < new_reg_ver)
		return true;

	return false;
}
#endif

#define TC_SECTOR_SZ		8

static u8 ts_upgrade_firmware(struct bt532_ts_data *touch_dev,
	const u8 *firmware_data, u32 size)
{
	struct i2c_client *client = touch_dev->client;
	u16 flash_addr;
	u8 *verify_data;
	int retry_cnt = 0;
	int i;
	int page_sz = 64;
	u16 chip_code;

	verify_data = kzalloc(size, GFP_KERNEL);
	if (verify_data == NULL) {
		zinitix_printk(KERN_ERR "cannot alloc verify buffer\n");
		return false;
	}

retry_upgrade:
	bt532_power_control(touch_dev, POWER_OFF);
	bt532_power_control(touch_dev, POWER_ON);
	mdelay(10);

	if (write_reg(client, 0xc000, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (vendor cmd enable)\n");
		goto fail_upgrade;
	}

	udelay(10);

	if (read_data(client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		zinitix_printk("failed to read chip code\n");
		goto fail_upgrade;
	}

	zinitix_printk("chip code = 0x%x\n", chip_code);
	page_sz = 128;
	udelay(10);

	if (write_cmd(client, 0xc004) != I2C_SUCCESS){
		zinitix_printk("power sequence error (intn clear)\n");
		goto fail_upgrade;
	}

	udelay(10);

	if (write_reg(client, 0xc002, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (nvm init)\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "init flash\n");

	if (write_reg(client, 0xc003, 0x0001) != I2C_SUCCESS){
		zinitix_printk("failed to write nvm vpp on\n");
		goto fail_upgrade;
	}

	if (write_reg(client, 0xc104, 0x0001) != I2C_SUCCESS){
		zinitix_printk("failed to write nvm wp disable\n");
		goto fail_upgrade;
	}

	if (write_cmd(client, ZXT_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "writing firmware data\n");
	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			//zinitix_debug_msg("write :addr=%04x, len=%d\n",	flash_addr, TC_SECTOR_SZ);
			if (write_data(client,
				ZXT_WRITE_FLASH,
				(u8 *)&firmware_data[flash_addr],TC_SECTOR_SZ) < 0) {
				zinitix_printk(KERN_INFO"error : write zinitix tc firmare\n");
				goto fail_upgrade;
			}
			flash_addr += TC_SECTOR_SZ;
			udelay(100);
		}

		mdelay(30);	/*for fuzing delay*/
	}

	if (write_reg(client, 0xc003, 0x0000) != I2C_SUCCESS){
		zinitix_printk("nvm write vpp off\n");
		goto fail_upgrade;
	}

	if (write_reg(client, 0xc104, 0x0000) != I2C_SUCCESS){
		zinitix_printk("nvm wp enable\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "init flash\n");

	if (write_cmd(client, ZXT_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "read firmware data\n");

	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			//zinitix_debug_msg("read :addr=%04x, len=%d\n", flash_addr, TC_SECTOR_SZ);
			if (read_firmware_data(client,
				ZXT_READ_FLASH,
				(u8*)&verify_data[flash_addr], TC_SECTOR_SZ) < 0) {
				zinitix_printk(KERN_INFO "error : read zinitix tc firmare\n");

				goto fail_upgrade;
			}

			flash_addr += TC_SECTOR_SZ;
		}
	}
	/* verify */
	printk(KERN_INFO "verify firmware data\n");
	if (memcmp((u8 *)&firmware_data[0], (u8 *)&verify_data[0], size) == 0) {
		zinitix_printk(KERN_INFO "upgrade finished\n");
		kfree(verify_data);
		bt532_power_control(touch_dev, POWER_OFF);
		bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

		return true;
	}

fail_upgrade:
	bt532_power_control(touch_dev, POWER_OFF);

	if (retry_cnt++ < INIT_RETRY_CNT) {
		zinitix_printk(KERN_INFO "upgrade failed : so retry... (%d)\n", retry_cnt);
		goto retry_upgrade;
	}

	if (verify_data != NULL)
		kfree(verify_data);

	zinitix_printk(KERN_INFO "upgrade failed..\n");
	return false;
}

static bool ts_hw_calibration(struct bt532_ts_data *touch_dev)
{
	struct i2c_client *client = touch_dev->client;
	u16	chip_eeprom_info;
	int time_out = 0;

	if (write_reg(client,
		ZXT_TOUCH_MODE, 0x07) != I2C_SUCCESS)
		return false;
	mdelay(10);
	write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
	mdelay(10);
	write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
	mdelay(50);
	write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
	mdelay(10);

	if (write_cmd(client,
		ZXT_CALIBRATE_CMD) != I2C_SUCCESS)
		return false;

	if (write_cmd(client,
		ZXT_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
		return false;

	mdelay(10);
	write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);

	/* wait for h/w calibration*/
	do {
		mdelay(500);
		write_cmd(client,
				ZXT_CLEAR_INT_STATUS_CMD);

		if (read_data(client,
			ZXT_EEPROM_INFO_REG,
			(u8 *)&chip_eeprom_info, 2) < 0)
			return false;

		zinitix_debug_msg("touch eeprom touch_dev = 0x%04X\r\n",
			chip_eeprom_info);
		if (!zinitix_bit_test(chip_eeprom_info, 0))
			break;

		if(time_out++ == 4){
			write_cmd(client, ZXT_CALIBRATE_CMD);
			mdelay(10);
			write_cmd(client,
				ZXT_CLEAR_INT_STATUS_CMD);
			zinitix_printk("h/w calibration retry timeout.\n");
		}

		if(time_out++ > 10){
			zinitix_printk("[error] h/w calibration timeout.\n");
			break;
		}

	} while (1);

	if (write_reg(client,
		ZXT_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		return false;

	if (touch_dev->cap_info.ic_int_mask != 0) {
		if (write_reg(client,
			ZXT_INT_ENABLE_FLAG,
			touch_dev->cap_info.ic_int_mask)
			!= I2C_SUCCESS)
			return false;
	}

	write_reg(client, 0xc003, 0x0001);
	write_reg(client, 0xc104, 0x0001);
	udelay(100);
	if (write_cmd(client,
		ZXT_SAVE_CALIBRATION_CMD) != I2C_SUCCESS)
		return false;

	mdelay(1000);
	write_reg(client, 0xc003, 0x0000);
	write_reg(client, 0xc104, 0x0000);
	return true;
}

static bool init_touch(struct bt532_ts_data *touch_dev)
{
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;
	struct i2c_client *client = touch_dev->client;
	struct capa_info *cap = &(touch_dev->cap_info);
	u16 reg_val;
	int i;
	u16 chip_eeprom_info;
#if USE_CHECKSUM
	u16 chip_check_sum;
	u8 checksum_err;
#endif
	int retry_cnt = 0;

retry_init:
	for(i = 0; i < INIT_RETRY_CNT; i++) {
		if (read_data(client, ZXT_EEPROM_INFO_REG,
						(u8 *)&chip_eeprom_info, 2) < 0) {
			dev_err(&client->dev, "Failed to read eeprom touch_dev(%d)\n", i);
			mdelay(10);
			continue;
		} else
			break;
	}

	if (i == INIT_RETRY_CNT)
		goto fail_init;

#if USE_CHECKSUM
	dev_info(&client->dev,"%s: Check checksum\n", __func__);

	checksum_err = 0;

	for (i = 0; i < INIT_RETRY_CNT; i++) {
		if (read_data(client, ZXT_CHECKSUM_RESULT,
						(u8 *)&chip_check_sum, 2) < 0) {
			mdelay(10);
			continue;
		}

		dev_info(&client->dev, "0x%04X\n", chip_check_sum);

		if(chip_check_sum == 0x55aa)
			break;
		else {
			checksum_err = 1;
			break;
		}
	}

	if (i == INIT_RETRY_CNT || checksum_err) {
		dev_err(&client->dev, "Failed to check firmware data\n");
		if(checksum_err == 1 && retry_cnt < INIT_RETRY_CNT)
			retry_cnt = INIT_RETRY_CNT;

		goto fail_init;
	}
#endif

	if (write_cmd(client, ZXT_SWRESET_CMD) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to write reset command\n");
		goto fail_init;
	}

	cap->button_num = SUPPORTED_BUTTON_NUM;

	reg_val = 0;
	zinitix_bit_set(reg_val, BIT_PT_CNT_CHANGE);
	zinitix_bit_set(reg_val, BIT_DOWN);
	zinitix_bit_set(reg_val, BIT_MOVE);
	zinitix_bit_set(reg_val, BIT_UP);
	zinitix_bit_set(reg_val, BIT_PALM);
	zinitix_bit_set(reg_val, BIT_PALM_REJECT);

	if (cap->button_num > 0)
		zinitix_bit_set(reg_val, BIT_ICON_EVENT);

	cap->ic_int_mask = reg_val;

	if (write_reg(client, ZXT_INT_ENABLE_FLAG, 0x0) != I2C_SUCCESS)
		goto fail_init;

	dev_info(&client->dev, "%s: Send reset command\n", __func__);
	if (write_cmd(client, ZXT_SWRESET_CMD) != I2C_SUCCESS)
		goto fail_init;

	/* get chip information */
	if (read_data(client, ZXT_VENDOR_ID,
					(u8 *)&cap->vendor_id, 2) < 0) {
		zinitix_printk("failed to read chip revision\n");
		goto fail_init;
	}

	if (read_data(client, ZXT_CHIP_REVISION,
					(u8 *)&cap->ic_revision, 2) < 0) {
		zinitix_printk("failed to read chip revision\n");
		goto fail_init;
	}

	cap->ic_fw_size = 32*1024;

	if (read_data(client, ZXT_HW_ID, (u8 *)&cap->hw_id, 2) < 0) {
		dev_err(&client->dev, "Failed to read hw id\n");
		goto fail_init;
	}

	if (read_data(client, ZXT_SENSITIVITY,
					(u8 *)&cap->threshold, 2) < 0)
		goto fail_init;

	if (read_data(client, ZXT_TOTAL_NUMBER_OF_X,
					(u8 *)&cap->x_node_num, 2) < 0)
		goto fail_init;

	if (read_data(client, ZXT_TOTAL_NUMBER_OF_Y,
					(u8 *)&cap->y_node_num, 2) < 0)
		goto fail_init;

	cap->total_node_num = cap->x_node_num * cap->y_node_num;

	if (read_data(client, ZXT_AFE_FREQUENCY,
					(u8 *)&cap->afe_frequency, 2) < 0)
		goto fail_init;

	zinitix_debug_msg("afe frequency = %d\n", cap->afe_frequency);

	//--------------------------------------------------------

	/* get chip firmware version */
	if (read_data(client, ZXT_FIRMWARE_VERSION,
					(u8 *)&cap->fw_version, 2) < 0)
		goto fail_init;

	if (read_data(client, ZXT_MINOR_FW_VERSION,
					(u8 *)&cap->fw_minor_version, 2) < 0)
		goto fail_init;

	if (read_data(client, ZXT_DATA_VERSION_REG,
					(u8 *)&cap->reg_data_version, 2) < 0)
		goto fail_init;

#if TOUCH_ONESHOT_UPGRADE
	if (ts_check_need_upgrade(touch_dev, cap->fw_version,
								cap->fw_minor_version, cap->reg_data_version,
								cap->hw_id) == true) {
		zinitix_printk("start upgrade firmware\n");

		if(ts_upgrade_firmware(touch_dev, m_firmware_data,
			cap->ic_fw_size) == false)
			goto fail_init;

		if(ts_hw_calibration(touch_dev) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (write_reg(client, ZXT_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;

		/* get chip firmware version */
		if (read_data(client, ZXT_FIRMWARE_VERSION,
						(u8 *)&cap->fw_version, 2) < 0)
			goto fail_init;

		if (read_data(client, ZXT_MINOR_FW_VERSION,
						(u8 *)&cap->fw_minor_version, 2) < 0)
			goto fail_init;

		if (read_data(client, ZXT_DATA_VERSION_REG,
						(u8 *)&cap->reg_data_version, 2) < 0)
			goto fail_init;
	}
#endif

	if (read_data(client, ZXT_EEPROM_INFO_REG,
					(u8 *)&chip_eeprom_info, 2) < 0)
		goto fail_init;

	if (zinitix_bit_test(chip_eeprom_info, 0)) { /* hw calibration bit*/
		if(ts_hw_calibration(touch_dev) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (write_reg(client,
			ZXT_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;
	}

	/* initialize */
	if (write_reg(client, ZXT_X_RESOLUTION,
					(u16)pdata->x_resolution) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client, ZXT_Y_RESOLUTION,
					(u16)pdata->y_resolution) != I2C_SUCCESS)
		goto fail_init;

	cap->x_min = (u32)0;
	cap->y_min = (u32)0;
	cap->x_max = (u32)pdata->x_resolution;
	cap->y_max = (u32)pdata->y_resolution;

	if (write_reg(client,
		ZXT_BUTTON_SUPPORTED_NUM,
		(u16)cap->button_num) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client,
		ZXT_SUPPORTED_FINGER_NUM,
		(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_init;

	cap->multi_fingers = MAX_SUPPORTED_FINGER_NUM;

	zinitix_debug_msg("max supported finger num = %d\r\n",
		cap->multi_fingers);
	cap->gesture_support = 0;
	zinitix_debug_msg("set other configuration\r\n");

	if (write_reg(client,
		ZXT_INITIAL_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client,
		ZXT_TOUCH_MODE, touch_dev->touch_mode) != I2C_SUCCESS)
		goto fail_init;

	/* soft calibration */
	if (write_cmd(client,
		ZXT_CALIBRATE_CMD) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client,
		ZXT_INT_ENABLE_FLAG,
		cap->ic_int_mask) != I2C_SUCCESS)
		goto fail_init;

	/* read garbage data */
	for (i = 0; i < 10; i++) {
		write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
		udelay(10);
	}

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) { /* Test Mode */
		if (write_reg(client,
			ZXT_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS) {
			zinitix_printk("Fail to set ZXT_DELAY_RAW_FOR_HOST.\r\n");
			goto fail_init;
		}
	}
#if ESD_TIMER_INTERVAL
	if (write_reg(client, ZXT_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_init;

	read_data(client, ZXT_PERIODICAL_INTERRUPT_INTERVAL, (u8 *)&reg_val, 2);
	dev_info(&client->dev, "Esd timer register = %d\n", reg_val);
#endif
	zinitix_debug_msg("successfully initialized\r\n");
	return true;

fail_init:
	if (++retry_cnt <= INIT_RETRY_CNT) {
		bt532_power_control(touch_dev, POWER_OFF);
		bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n",
			retry_cnt);
		goto	retry_init;

	} else if(retry_cnt == INIT_RETRY_CNT+1) {
		cap->ic_fw_size = 32*1024;

		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n", retry_cnt);
#if TOUCH_FORCE_UPGRADE
		if (ts_upgrade_firmware(touch_dev, m_firmware_data,
			cap->ic_fw_size) == false) {
			zinitix_printk("upgrade failed\n");
			return false;
		}
		mdelay(100);

		// hw calibration and make checksum
		if(ts_hw_calibration(touch_dev) == false) {
			zinitix_printk("failed to initiallize\r\n");
			return false;
		}
		goto retry_init;
#endif
	}

	zinitix_printk("failed to initiallize\r\n");
	return false;
}

static bool mini_init_touch(struct bt532_ts_data *touch_dev)
{
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;
	struct i2c_client *client = touch_dev->client;
	int i;
#if USE_CHECKSUM
	u16 chip_check_sum;

	dev_info(&client->dev, "check checksum\n");

	if (read_data(client, ZXT_CHECKSUM_RESULT,
					(u8 *)&chip_check_sum, 2) < 0)
		goto fail_mini_init;

	if( chip_check_sum != 0x55aa ) {
		dev_err(&client->dev, "Failed to check firmware"
					" checksum(0x%04x)\n", chip_check_sum);

		goto fail_mini_init;
	}
#endif

	if (write_cmd(client, ZXT_SWRESET_CMD) != I2C_SUCCESS) {
		dev_info(&client->dev, "Failed to write reset command\n");

		goto fail_mini_init;
	}

	/* initialize */
	if (write_reg(client, ZXT_X_RESOLUTION,
			(u16)(pdata->x_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client,ZXT_Y_RESOLUTION,
			(u16)(pdata->y_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	dev_info(&client->dev, "touch max x = %d\r\n", pdata->x_resolution);
	dev_info(&client->dev, "touch max y = %d\r\n", pdata->y_resolution);

	if (write_reg(client, ZXT_BUTTON_SUPPORTED_NUM,
			(u16)touch_dev->cap_info.button_num) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, ZXT_SUPPORTED_FINGER_NUM,
			(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, ZXT_INITIAL_TOUCH_MODE,
			TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, ZXT_TOUCH_MODE,
			touch_dev->touch_mode) != I2C_SUCCESS)
		goto fail_mini_init;

	/* soft calibration */
	if (write_cmd(client, ZXT_CALIBRATE_CMD) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, ZXT_INT_ENABLE_FLAG,
			touch_dev->cap_info.ic_int_mask) != I2C_SUCCESS)
		goto fail_mini_init;

	/* read garbage data */
	for (i = 0; i < 10; i++) {
		write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
		udelay(10);
	}

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) {
		if (write_reg(client, ZXT_DELAY_RAW_FOR_HOST,
				RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS){
			dev_err(&client->dev, "Failed to set ZXT_DELAY_RAW_FOR_HOST\n");

			goto fail_mini_init;
		}
	}

#if ESD_TIMER_INTERVAL
	if (write_reg(client, ZXT_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_mini_init;

	esd_timer_start(CHECK_ESD_TIMER, touch_dev);
	dev_info(&client->dev, "Started esd timer\n");
#endif

	dev_info(&client->dev, "Successfully mini initialized\r\n");
	return true;

fail_mini_init:
	dev_err(&client->dev, "Failed to initialize mini init\n");
	bt532_power_control(touch_dev, POWER_OFF);
	bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

	if(init_touch(touch_dev) == false) {
		dev_err(&client->dev, "Failed to initialize\n");

		return false;
	}

#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, touch_dev);
	dev_info(&client->dev, "Started esd timer\n");
#endif
	return true;
}

static void clear_report_data(struct bt532_ts_data *touch_dev)
{
	int i;
	u8 reported = 0;
	u8 sub_status;

	for (i = 0; i < touch_dev->cap_info.button_num; i++) {
		if (touch_dev->button[i] == ICON_BUTTON_DOWN) {
			touch_dev->button[i] = ICON_BUTTON_UP;
			input_report_key(touch_dev->input_dev,
				BUTTON_MAPPING_KEY[i], 0);
			reported = true;
			zinitix_debug_msg("button up = %d \r\n", i);
		}
	}

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		sub_status = touch_dev->reported_touch_info.coord[i].sub_status;
		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev,
				MT_TOOL_FINGER, 0);
			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, -1);
			reported = true;
			if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
				printk(KERN_INFO "[TSP] R %02d\r\n", i);
		}
		touch_dev->reported_touch_info.coord[i].sub_status = 0;
	}

	if (reported) {
		input_sync(touch_dev->input_dev);
	}
}

#define	PALM_REPORT_WIDTH	200
#define	PALM_REJECT_WIDTH	255

static irqreturn_t bt532_touch_work(int irq, void *data)
{
	struct bt532_ts_data* touch_dev = (struct bt532_ts_data*)data;
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;
	struct i2c_client *client = touch_dev->client;
	int i;
	u8 reported = false;
	u8 sub_status;
	u8 prev_sub_status;
	u32 x, y, maxX, maxY;
	u32 w;
	u32 tmp;
	u8 palm = 0;

	if (gpio_get_value(touch_dev->pdata->gpio_int)) {
		dev_err(&client->dev, "Invalid interrupt\n");

		return IRQ_HANDLED;
	}

	if (down_trylock(&touch_dev->work_lock)) {
		dev_err(&client->dev, "%s: Failed to occupy work lock\n", __func__);
		write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);

		return IRQ_HANDLED;
	}

	if (touch_dev->work_state != NOTHING) {
		dev_err(&client->dev, "%s: Other process occupied\n", __func__);
		udelay(DELAY_FOR_SIGNAL_DELAY);

		if (!gpio_get_value(touch_dev->pdata->gpio_int)) {
			write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);
			udelay(DELAY_FOR_SIGNAL_DELAY);
		}

		goto out;
	}

	touch_dev->work_state = NORMAL;

#if ESD_TIMER_INTERVAL
	esd_timer_stop(touch_dev);
	dev_info(&client->dev, "Stopped esd timer\n");
#endif

	if (ts_read_coord(touch_dev) == false || touch_dev->touch_info.status == 0xffff
		|| touch_dev->touch_info.status == 0x1) { /* maybe desirable reset */
		dev_err(&client->dev, "Failed to read touch_dev coord\n");
		bt532_power_control(touch_dev, POWER_OFF);
		bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

		clear_report_data(touch_dev);
		mini_init_touch(touch_dev);

		goto out;
	}

	/* invalid : maybe periodical repeated int. */
	if (touch_dev->touch_info.status == 0x0) {
		dev_info(&client->dev, "Periodical esd repeated int occured\n");

		goto out;
	}

	reported = false;

	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_ICON_EVENT)) {
		if (read_data(touch_dev->client, ZXT_ICON_STATUS_REG,
			(u8 *)(&touch_dev->icon_event_reg), 2) < 0) {
			dev_err(&client->dev, "Failed to read button touch_dev\n");
			write_cmd(client, ZXT_CLEAR_INT_STATUS_CMD);

			goto out;
		}

		for (i = 0; i < touch_dev->cap_info.button_num; i++) {
			if (zinitix_bit_test(touch_dev->icon_event_reg,
									(BIT_O_ICON0_DOWN + i))) {
				touch_dev->button[i] = ICON_BUTTON_DOWN;
				input_report_key(touch_dev->input_dev, BUTTON_MAPPING_KEY[i], 1);
				reported = true;
				dev_info(&client->dev, "Button down = %d\n", i);
			}
		}

		for (i = 0; i < touch_dev->cap_info.button_num; i++) {
			if (zinitix_bit_test(touch_dev->icon_event_reg,
									(BIT_O_ICON0_UP + i))) {
				touch_dev->button[i] = ICON_BUTTON_UP;
				input_report_key(touch_dev->input_dev, BUTTON_MAPPING_KEY[i], 0);
				reported = true;
				dev_info(&client->dev, "Button up = %d\n", i);
			}
		}
	}

	/* if button press or up event occured... */
	if (reported == true ||
			!zinitix_bit_test(touch_dev->touch_info.status, BIT_PT_EXIST)) {
		for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
			prev_sub_status = touch_dev->reported_touch_info.coord[i].sub_status;
			if (zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
				dev_info(&client->dev, "Finger [%02d] up\n", i);
				input_mt_slot(touch_dev->input_dev, i);
				input_mt_report_slot_state(touch_dev->input_dev,
											MT_TOOL_FINGER, 0);
				input_report_abs(touch_dev->input_dev, ABS_MT_TRACKING_ID, -1);
			}
		}
		memset(&touch_dev->reported_touch_info, 0x0, sizeof(struct point_info));
		input_sync(touch_dev->input_dev);

		if(reported == true) /* for button event */
			udelay(100);

		goto out;
	}

	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_PALM)) {
		dev_info(&client->dev, "Palm report\n");
		palm = 1;
	}

	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_PALM_REJECT)){
		dev_info(&client->dev, "Palm reject\n");
		palm = 2;
	}

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		sub_status = touch_dev->touch_info.coord[i].sub_status;
		prev_sub_status = touch_dev->reported_touch_info.coord[i].sub_status;

		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
			x = touch_dev->touch_info.coord[i].x;
			y = touch_dev->touch_info.coord[i].y;
			w = touch_dev->touch_info.coord[i].width;

			 /* transformation from touch to screen orientation */
			if (pdata->orientation & TOUCH_V_FLIP)
				y = touch_dev->cap_info.y_max
					+ touch_dev->cap_info.y_min - y;

			if (pdata->orientation & TOUCH_H_FLIP)
				x = touch_dev->cap_info.x_max
					+ touch_dev->cap_info.x_min - x;

			maxX = touch_dev->cap_info.x_max;
			maxY = touch_dev->cap_info.y_max;

			if (pdata->orientation & TOUCH_XY_SWAP) {
				zinitix_swap_v(x, y, tmp);
				maxX = touch_dev->cap_info.y_max;
				maxY = touch_dev->cap_info.x_max;
			}

			if (x > maxX || y > maxY) {
				dev_err(&client->dev,
							"Invalid coord %d : x=%d, y=%d\n", i, x, y);
				continue;
			}

			touch_dev->touch_info.coord[i].x = x;
			touch_dev->touch_info.coord[i].y = y;
			if (zinitix_bit_test(sub_status, SUB_BIT_DOWN))
				dev_info(&client->dev, "Finger [%02d] x = %d, y = %d,"
							" w = %d\n", i, x, y, w);

			if (w == 0)
				w = 1;

			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev, MT_TOOL_FINGER, 1);
			if (!zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST))
				input_report_abs(touch_dev->input_dev, ABS_MT_TRACKING_ID, i);

#if (TOUCH_POINT_MODE == 2)
			if (palm == 0) {
				if(w >= PALM_REPORT_WIDTH)
					w = PALM_REPORT_WIDTH - 10;
			} else if (palm == 1) {	//palm report
				w = PALM_REPORT_WIDTH;
//				touch_dev->touch_info.coord[i].minor_width = PALM_REPORT_WIDTH;
			} else if (palm == 2){	// palm reject
//				x = y = 0;
				w = PALM_REJECT_WIDTH;
//				touch_dev->touch_info.coord[i].minor_width = PALM_REJECT_WIDTH;
			}
#endif

			input_report_abs(touch_dev->input_dev, ABS_MT_TOUCH_MAJOR, (u32)w);
			input_report_abs(touch_dev->input_dev, ABS_MT_PRESSURE, (u32)w);
			input_report_abs(touch_dev->input_dev, ABS_MT_WIDTH_MAJOR,
								(u32)((palm == 1)?w-40:w));
#if (TOUCH_POINT_MODE == 2)
			input_report_abs(touch_dev->input_dev,
				ABS_MT_TOUCH_MINOR, (u32)touch_dev->touch_info.coord[i].minor_width);
//			input_report_abs(touch_dev->input_dev,
//				ABS_MT_WIDTH_MINOR, (u32)touch_dev->touch_info.coord[i].minor_width);
//			input_report_abs(touch_dev->input_dev,
//				ABS_MT_ANGLE, touch_dev->touch_info.coord[i].angle - 90);
//			zinitix_debug_msg("finger [%02d] angle = %03d\r\n", i, touch_dev->touch_info.coord[i].angle);
//			input_report_abs(touch_dev->input_dev, ABS_MT_PALM, (palm==2)?1:0);
//			input_report_abs(touch_dev->input_dev, ABS_MT_PALM, 1);
#endif

			input_report_abs(touch_dev->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(touch_dev->input_dev, ABS_MT_POSITION_Y, y);
		} else if (zinitix_bit_test(sub_status, SUB_BIT_UP)||
			zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
			dev_info(&client->dev, "Finger [%02d] up\n", i);

			memset(&touch_dev->touch_info.coord[i], 0x0, sizeof(struct coord));
			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev, MT_TOOL_FINGER, 0);
			input_report_abs(touch_dev->input_dev, ABS_MT_TRACKING_ID, -1);
		} else {
			memset(&touch_dev->touch_info.coord[i], 0x0, sizeof(struct coord));
		}
	}
	memcpy((char *)&touch_dev->reported_touch_info, (char *)&touch_dev->touch_info,
			sizeof(struct point_info));
	input_sync(touch_dev->input_dev);

out:
	if (touch_dev->work_state == NORMAL) {
#if ESD_TIMER_INTERVAL
		esd_timer_start(CHECK_ESD_TIMER, touch_dev);
		dev_info(&client->dev, "Started esd tmr\n");
#endif
		touch_dev->work_state = NOTHING;
	}

	up(&touch_dev->work_lock);

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bt532_ts_late_resume(struct early_suspend *h)
{
	struct bt532_ts_data *touch_dev = misc_info;
	//touch_dev = container_of(h, struct bt532_ts_data, early_suspend);

	if (touch_dev == NULL)
		return;
	zinitix_printk("late resume++\r\n");

	down(&touch_dev->work_lock);
	if (touch_dev->work_state != RESUME
		&& touch_dev->work_state != EALRY_SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_state);
		up(&touch_dev->work_lock);
		return;
	}
/*#ifdef CONFIG_PM*/
#if 0
	write_cmd(touch_dev->client, ZXT_WAKEUP_CMD);
	mdelay(1);
#else
	bt532_power_control(touch_dev, POWER_ON_SEQUENCE);
#endif
	if (mini_init_touch(touch_dev) == false)
		goto fail_late_resume;
	enable_irq(touch_dev->irq);
	touch_dev->work_state = NOTHING;
	up(&touch_dev->work_lock);
	zinitix_printk("late resume--\n");
	return;
fail_late_resume:
	zinitix_printk("failed to late resume\n");
	enable_irq(touch_dev->irq);
	touch_dev->work_state = NOTHING;
	up(&touch_dev->work_lock);
	return;
}

static void bt532_ts_early_suspend(struct early_suspend *h)
{
	struct bt532_ts_data *touch_dev = misc_info;
	/*touch_dev = container_of(h, struct bt532_ts_data, early_suspend);*/

	if (touch_dev == NULL)
		return;

	zinitix_printk("early suspend++\n");

	disable_irq(touch_dev->irq);
#if ESD_TIMER_INTERVAL
	flush_work(&touch_dev->tmr_work);
#endif

	down(&touch_dev->work_lock);
	if (touch_dev->work_state != NOTHING) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_state);
		up(&touch_dev->work_lock);
		enable_irq(touch_dev->irq);
		return;
	}
	touch_dev->work_state = EALRY_SUSPEND;

	zinitix_debug_msg("clear all reported points\r\n");
	clear_report_data(touch_dev);

#if ESD_TIMER_INTERVAL
	write_reg(touch_dev->client, ZXT_PERIODICAL_INTERRUPT_INTERVAL, 0);
	esd_timer_stop(touch_dev);
	dev_info(&client->dev, "Stopped esd timer\n");
#endif

#ifdef CONIFG_PM
	write_reg(touch_dev->client, ZXT_INT_ENABLE_FLAG, 0x0);

	udelay(100);
	if (write_cmd(touch_dev->client, ZXT_SLEEP_CMD) != I2C_SUCCESS) {
		zinitix_printk("failed to enter into sleep mode\n");
		up(&touch_dev->work_lock);
		return;
	}
#else
	bt532_power_control(touch_dev, POWER_OFF);
#endif
	touch_dev->pdata->led_power(0);
	zinitix_printk("early suspend--\n");
	up(&touch_dev->work_lock);
	return;
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int bt532_ts_resume(struct device *dev)
{
	//struct i2c_client *client = to_i2c_client(dev);
	struct bt532_ts_data *touch_dev = misc_info; //i2c_get_clientdata(client);

	if (touch_dev == NULL)
		return -1;

	zinitix_printk("resume++\n");
	down(&touch_dev->work_lock);
	if (touch_dev->work_state != SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_state);
		up(&touch_dev->work_lock);
		return 0;
	}

	bt532_power_control(touch_dev, POWER_ON_SEQUENCE);

#ifdef CONFIG_HAS_EARLYSUSPEND
	touch_dev->work_state = RESUME;
#else
	touch_dev->work_state = NOTHING;
	if (mini_init_touch(touch_dev) == false)
		zinitix_printk("failed to resume\n");
	enable_irq(touch_dev->irq);
#endif

	zinitix_printk("resume--\n");
	up(&touch_dev->work_lock);
	return 0;
}

static int bt532_ts_suspend(struct device *dev)
{
	//struct i2c_client *client = to_i2c_client(dev);
	struct bt532_ts_data *touch_dev = misc_info; //i2c_get_clientdata(client);

	if (touch_dev == NULL)
		return -1;

	zinitix_printk("suspend++\n");

#ifndef CONFIG_HAS_EARLYSUSPEND
	disable_irq(touch_dev->irq);
#endif
#if ESD_TIMER_INTERVAL
	flush_work(&touch_dev->tmr_work);
#endif

	down(&touch_dev->work_lock);
	if (touch_dev->work_state != NOTHING
		&& touch_dev->work_state != EALRY_SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_state);
		up(&touch_dev->work_lock);
#ifndef CONFIG_HAS_EARLYSUSPEND
		enable_irq(touch_dev->irq);
#endif
		return 0;
	}

#ifndef CONFIG_HAS_EARLYSUSPEND
	clear_report_data(touch_dev);

#if ESD_TIMER_INTERVAL
	esd_timer_stop(touch_dev);
	dev_info(&client->dev, "Stopped esd timer\n");
#endif

	write_cmd(touch_dev->client, ZXT_SLEEP_CMD);
	udelay(100);
#endif
	bt532_power_control(touch_dev, POWER_OFF);
	touch_dev->work_state = SUSPEND;

	zinitix_printk("suspend--\n");
	up(&touch_dev->work_lock);
	return 0;
}
#endif

static bool ts_set_touchmode(u16 value){
	int i;

	disable_irq(misc_info->irq);

	down(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			misc_info->work_state);
		enable_irq(misc_info->irq);
		up(&misc_info->work_lock);
		return -1;
	}

	misc_info->work_state = SET_MODE;

	if(value == TOUCH_DND_MODE) {
		if (write_reg(misc_info->client, ZXT_DND_N_COUNT,
						SEC_DND_N_COUNT)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set ZXT_DND_N_COUNT %d.\n", SEC_DND_N_COUNT);

		if (write_reg(misc_info->client, ZXT_AFE_FREQUENCY,
						SEC_DND_FREQUENCY)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set ZXT_AFE_FREQUENCY %d.\n", SEC_DND_FREQUENCY);
	} else if(misc_info->touch_mode == TOUCH_DND_MODE) {
		if (write_reg(misc_info->client, ZXT_AFE_FREQUENCY,
						misc_info->cap_info.afe_frequency)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set ZXT_AFE_FREQUENCY %d.\n",
					misc_info->cap_info.afe_frequency);
	}

	if(value == TOUCH_SEC_MODE)
		misc_info->touch_mode = TOUCH_POINT_MODE;
	else
		misc_info->touch_mode = value;

	printk(KERN_INFO "[zinitix_touch] tsp_set_testmode, "
			"touchkey_testmode = %d\r\n", misc_info->touch_mode);

	if(misc_info->touch_mode != TOUCH_POINT_MODE) {
		if (write_reg(misc_info->client, ZXT_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS)
			zinitix_printk("Fail to set ZXT_DELAY_RAW_FOR_HOST.\r\n");
	}

	if (write_reg(misc_info->client, ZXT_TOUCH_MODE,
					misc_info->touch_mode)!=I2C_SUCCESS)
		printk(KERN_INFO "[zinitix_touch] TEST Mode : "
				"Fail to set ZINITX_TOUCH_MODE %d.\r\n", misc_info->touch_mode);

	// clear garbage data
	for(i=0; i < 10; i++) {
		mdelay(20);
		write_cmd(misc_info->client, ZXT_CLEAR_INT_STATUS_CMD);
	}

	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	up(&misc_info->work_lock);
	return 1;
}

static int ts_upgrade_sequence(const u8 *firmware_data)
{
	disable_irq(misc_info->irq);
	down(&misc_info->work_lock);
	misc_info->work_state = UPGRADE;

#if ESD_TIMER_INTERVAL
	esd_timer_stop(misc_info);
#endif
	zinitix_debug_msg("clear all reported points\r\n");
	clear_report_data(misc_info);

	printk(KERN_INFO "start upgrade firmware\n");
	if (ts_upgrade_firmware(misc_info,
		firmware_data,
		misc_info->cap_info.ic_fw_size) == false) {
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;
	}

	if (init_touch(misc_info) == false) {
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;
	}

#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, misc_info);
	dev_info(&client->dev, "Started esd timer\n");
#endif

	enable_irq(misc_info->irq);
	misc_info->work_state = NOTHING;
	up(&misc_info->work_lock);
	return 0;
}

#ifdef SEC_FACTORY_TEST
static inline void set_cmd_result(struct bt532_ts_data *touch_dev, char *buff, int len)
{
	strncat(touch_dev->factory_info->cmd_result, buff, len);
}

static inline void set_default_result(struct bt532_ts_data *touch_dev)
{
	char delim = ':';
	memset(touch_dev->factory_info->cmd_result, 0x00, ARRAY_SIZE(touch_dev->factory_info->cmd_result));
	memcpy(touch_dev->factory_info->cmd_result, touch_dev->factory_info->cmd, strlen(touch_dev->factory_info->cmd));
	strncat(touch_dev->factory_info->cmd_result, &delim, 1);
}

#define MAX_FW_PATH 255
#define TSP_FW_FILENAME "/sdcard/zinitix_fw.bin"

int ts_get_ums_data(struct bt532_ts_data *touch_dev, u8 **ums_data)
{
	struct i2c_client *client = touch_dev->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(TSP_FW_FILENAME, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&client->dev,
			"file %s open error:%d\n", TSP_FW_FILENAME, (s32)fp);
		touch_dev->factory_info->cmd_state = 3;
		return -ENOENT;
		/*goto err_open;*/
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	dev_info(&client->dev,
		"start, file path %s, size %ld Bytes\n",
		TSP_FW_FILENAME, fsize);
	if(fsize != touch_dev->cap_info.ic_fw_size) {
		dev_err(&client->dev, "invalid fw size!!\n");
		touch_dev->factory_info->cmd_state = 3;
		ret = -EFBIG;
		goto size_error;
	}

	*ums_data = kmalloc(fsize, GFP_KERNEL);
	if (IS_ERR(*ums_data)) {
		dev_err(&client->dev, "kmalloc failed\n");
		ret = -EFAULT;
		goto malloc_error;
	}

	nread = vfs_read(fp, (char __user *)*ums_data,
		fsize, &fp->f_pos);
	if (nread != fsize) {
		dev_err(&client->dev,
			"failed to read firmware file, nread %ld Bytes\n",
			nread);
		touch_dev->factory_info->cmd_state = 3;
		ret = -EIO;
		kfree(*ums_data);
		goto read_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	dev_info(&client->dev, "ums fw is loaded!\n");

	return 0;

read_err:
malloc_error:
size_error:
	filp_close(fp, current->files);
	set_fs(old_fs);
	return ret;
}

int ts_upgrade_ums(struct bt532_ts_data *touch_dev)
{
	struct i2c_client *client = touch_dev->client;
	int ret = 0;
	u8 *ums_data = NULL;

	dev_info(&client->dev, "Start firmware flashing (UMS)\n");

	/*read firmware data*/
	ret = ts_get_ums_data(touch_dev, &ums_data);
	if (ret < 0) {
		dev_info(&client->dev, "file op is failed.(%d)\n", ret);
		goto err_read_ums_data;
	}

	/*start firm update*/
	ret = ts_upgrade_sequence((u8*)ums_data);
	if (ret < 0) {
		dev_info(&client->dev, "failed to update firmware(%d)\n", ret);
		touch_dev->factory_info->cmd_state = 3;
		goto err_update_firmware;
	}

err_update_firmware:
	kfree(ums_data);
err_read_ums_data:
	return ret;
}

static void fw_update(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	int ret = 0;
	char result[16] = {0, };

	set_default_result(touch_dev);

	switch (touch_dev->factory_info->cmd_param[0]) {
	case BUILT_IN:
		ret = ts_upgrade_sequence((u8*)m_firmware_data);
		if (ret < 0) {
			touch_dev->factory_info->cmd_state = 3;
			goto err_fw_update;
		}
		break;
	case UMS:
		ret = ts_upgrade_ums(touch_dev);
		if (ret < 0) {
			touch_dev->factory_info->cmd_state = 3;
			goto err_fw_update;
		}
		break;
	default:
		dev_err(&client->dev, "invalid fw file type!!\n");
		goto err_fw_update;
	}

	touch_dev->factory_info->cmd_state = 2;
	snprintf(result, sizeof(result) , "%s", "OK");
	set_cmd_result(touch_dev, result,
				strnlen(result, sizeof(result)));

	return ;

err_fw_update:
	snprintf(result, sizeof(result) , "%s", "NG");
	set_cmd_result(touch_dev, result, strnlen(result, sizeof(result)));
	return ;
}

static void get_fw_ver_bin(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;
	u16 fw_version, fw_minor_version, reg_version, hw_id, vendor_id;
	u32 version, length;

	set_default_result(touch_dev);

	/* To Do */
	/* modify m_firmware_data */
	fw_version = (u16)(m_firmware_data[52] | (m_firmware_data[53] << 8));
	fw_minor_version = (u16)(m_firmware_data[56] | (m_firmware_data[57] << 8));
	reg_version = (u16)(m_firmware_data[60] | (m_firmware_data[61] << 8));
	hw_id = (u16)(m_firmware_data[0x6b12] | (m_firmware_data[0x6b13] << 8));
	vendor_id = ntohs(*(u16 *)&m_firmware_data[0x6b22]);
	version = (u32)((u32)(hw_id & 0xff) << 16) | ((fw_version & 0xf ) << 12)
				| ((fw_minor_version & 0xf) << 8) | (reg_version & 0xff);

	length = sizeof(vendor_id);
	snprintf(finfo->cmd_buff, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(finfo->cmd_buff + length, sizeof(finfo->cmd_buff) - length,
				"%06X", version);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_fw_ver_ic(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;
	u16 fw_version, fw_minor_version, reg_version, hw_id, vendor_id;
	u32 version, length;

	set_default_result(touch_dev);

	fw_version = touch_dev->cap_info.fw_version;
	fw_minor_version = touch_dev->cap_info.fw_minor_version;
	reg_version = touch_dev->cap_info.reg_data_version;
	hw_id = touch_dev->cap_info.hw_id;
	vendor_id = ntohs(touch_dev->cap_info.vendor_id);
	version = (u32)((u32)(hw_id & 0xff) << 16) | ((fw_version & 0xf) << 12)
				| ((fw_minor_version & 0xf) << 8) | (reg_version & 0xff);

	length = sizeof(vendor_id);
	snprintf(finfo->cmd_buff, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(finfo->cmd_buff + length, sizeof(finfo->cmd_buff) - length,
				"%06X", version);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_threshold(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
				"%d", touch_dev->cap_info.threshold);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void module_off_master(void *device_data)
{
	return;
}

static void module_on_master(void *device_data)
{
	return;
}

static void module_off_slave(void *device_data)
{
	return;
}

static void module_on_slave(void *device_data)
{
	return;
}

#define ZXT_VENDOR_NAME "ZINITIX"

static void get_chip_vendor(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
				"%s", ZXT_VENDOR_NAME);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

#define ZXT_CHIP_NAME "ZFT400"

static void get_chip_name(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", ZXT_CHIP_NAME);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_x_num(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
				"%u", touch_dev->cap_info.x_node_num);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_y_num(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
				"%u", touch_dev->cap_info.y_node_num);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void not_support_cmd(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	set_default_result(touch_dev);

	sprintf(finfo->cmd_buff, "%s", "NA");
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	touch_dev->factory_info->cmd_state = NOT_APPLICABLE;

	dev_info(&client->dev, "%s: \"%s(%d)\"\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void run_reference_read(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;
	struct tsp_raw_data *raw_data = touch_dev->raw_data;
	u32 min, max;
	s32 i,j;

	set_default_result(touch_dev);

	ts_set_touchmode(TOUCH_DND_MODE);
	get_raw_data(touch_dev, (u8 *)raw_data->ref_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	min = raw_data->ref_data[0];
	max = raw_data->ref_data[0];

	for(i = 0; i < 30; i++)
	{
		for(j = 0; j < 18; j++)
		{
		/*	printk("[TSP] raw_data->ref_data : %d ", raw_data->ref_data[j+i]); */

			if(raw_data->ref_data[j+i] < min)
				min = raw_data->ref_data[j+i];

			if(raw_data->ref_data[j+i] > max)
				max = raw_data->ref_data[j+i];

		}
	/*printk("\n");*/
	}

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%d,%d\n", min, max);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: \"%s\"(%d)\n", __func__, finfo->cmd_buff,
				strlen(finfo->cmd_buff));

	return;
}

static void get_reference(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;
	struct tsp_raw_data *raw_data = touch_dev->raw_data;
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(touch_dev);

	x_node = finfo->cmd_param[0];
	y_node = finfo->cmd_param[1];

	if (x_node < 0 || x_node > touch_dev->cap_info.x_node_num ||
		y_node < 0 || y_node > touch_dev->cap_info.y_node_num) {
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", "abnormal");
		set_cmd_result(touch_dev, finfo->cmd_buff,
						strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
		touch_dev->factory_info->cmd_state = FAIL;
		return;
	}

	node_num = x_node * touch_dev->cap_info.x_node_num + y_node;

	val = raw_data->ref_data[node_num];
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u", val);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}
/*
static void run_scantime_read(void *device_data)
{
	return;
}

static void get_scantime(void *device_data)
{
	return;
}
*/
static void run_delta_read(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	/*struct i2c_client *client = touch_dev->client;*/
	/*struct tsp_factory_info *finfo = touch_dev->factory_info;*/
	/*struct tsp_raw_data *raw_data = touch_dev->raw_data;*/

	set_default_result(touch_dev);
/*
	ts_set_touchmode(TOUCH_DELTA_MODE);
	get_raw_data(touch_dev, (u8 *)touch_dev->delta_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);
	finfo->cmd_state = OK;
*/
	return;
}

static void get_delta(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	/*struct i2c_client *client = touch_dev->client;*/
	/*struct tsp_factory_info *finfo = touch_dev->factory_info;*/
	/*struct tsp_raw_data *raw_data = touch_dev->raw_data;*/
	/*unsigned int val;*/
	/*int x_node, y_node;*/
	/*int node_num;*/

	set_default_result(touch_dev);
/*
	x_node = finfo->cmd_param[0];
	y_node = finfo->cmd_param[1];

	if (x_node < 0 || x_node > touch_dev->cap_info.x_node_num ||
		y_node < 0 || y_node > touch_dev->cap_info.y_node_num) {
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", "abnormal");
		set_cmd_result(touch_dev, finfo->cmd_buff,
						strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
		touch_dev->factory_info->cmd_state = FAIL;
		return;
	}

	node_num = x_node * touch_dev->cap_info.x_node_num + y_node;

	val = raw_data->delta_data[node_num];
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u", val);
	set_cmd_result(touch_dev, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	touch_dev->factory_info->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
				strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
*/
	return;
}

/*
static void run_intensity_read(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;

	set_default_result(touch_dev);

	ts_set_touchmode(TOUCH_DND_MODE);
	get_raw_data(touch_dev, (u8 *)touch_dev->dnd_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	//////test////////////////////////////////////////////////////
	int i,j;

	for(i=0; i<30; i++)
	{
		for(j=0; j<18; j++)
			printk("[TSP] touch_dev->dnd_data : %d ", touch_dev->dnd_data[j+i]);

		printk("\n");
	}
	//////test////////////////////////////////////////////////////

	touch_dev->factory_info->cmd_state = 2;
}

static void get_normal(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(touch_dev);

	x_node = touch_dev->factory_info->cmd_param[0];
	y_node = touch_dev->factory_info->cmd_param[1];

	if (x_node < 0 || x_node > touch_dev->cap_info.x_node_num ||
		y_node < 0 || y_node > touch_dev->cap_info.y_node_num) {
		snprintf(buff, sizeof(buff), "%s", "abnormal");
		set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
		touch_dev->factory_info->cmd_state = 3;
		return;
	}

	node_num = x_node*touch_dev->cap_info.x_node_num + y_node;

	val = touch_dev->normal_data[node_num];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
	touch_dev->factory_info->cmd_state = 2;

	dev_info(&touch_dev->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
}

static void get_tkey_delta(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	char buff[16] = {0};
	u16 val;
	int btn_node;
	int ret;

	set_default_result(touch_dev);

	btn_node = touch_dev->factory_info->cmd_param[0];

	if (btn_node < 0 || btn_node > MAX_SUPPORTED_BUTTON_NUM)
		goto err_out;

	disable_irq(misc_info->irq);
	down(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			misc_info->work_state);
		enable_irq(misc_info->irq);
		up(&misc_info->work_lock);
		goto err_out;
	}
	misc_info->work_state = SET_MODE;

	ret = read_data(misc_info->client, ZXT_BTN_WIDTH + btn_node, (u8*)&val, 2);

	if (ret < 0) {
		printk(KERN_INFO "read error..\n");
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		goto err_out;
	}
	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	up(&misc_info->work_lock);

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
	touch_dev->factory_info->cmd_state = 2;

	dev_info(&touch_dev->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;

err_out:
	snprintf(buff, sizeof(buff), "%s", "abnormal");
	set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
	touch_dev->factory_info->cmd_state = 3;
}

static void get_intensity(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(touch_dev);

	x_node = touch_dev->factory_info->cmd_param[0];
	y_node = touch_dev->factory_info->cmd_param[1];

	if (x_node < 0 || x_node > touch_dev->cap_info.x_node_num ||
		y_node < 0 || y_node > touch_dev->cap_info.y_node_num) {
		snprintf(buff, sizeof(buff), "%s", "abnormal");
		set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
		touch_dev->factory_info->cmd_state = 3;
		return;
	}

	node_num = x_node*touch_dev->cap_info.x_node_num + y_node;

	val = touch_dev->dnd_data[node_num];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
	touch_dev->factory_info->cmd_state = 2;

	dev_info(&touch_dev->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
}

static void run_normal_read(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;

	set_default_result(touch_dev);

	ts_set_touchmode(TOUCH_NORMAL_MODE);
	get_raw_data(touch_dev, (u8 *)touch_dev->normal_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	touch_dev->factory_info->cmd_state = 2;
}

static void get_key_threshold(void *device_data)
{
	struct bt532_ts_data *touch_dev = (struct bt532_ts_data *)device_data;
	int ret = 0;
	u16 threshold;
	char buff[16] = {0};

	set_default_result(touch_dev);

	ret = read_data(misc_info->client, ZXT_BUTTON_SENSITIVITY, (u8*)&threshold, 2);

	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "failed");
		set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
		touch_dev->factory_info->cmd_state = 3;
		return;
	}

	snprintf(buff, sizeof(buff), "%u", threshold);

	set_cmd_result(touch_dev, buff, strnlen(buff, sizeof(buff)));
	touch_dev->factory_info->cmd_state = 2;
	dev_info(&touch_dev->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
*/

static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;
	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;

	if (finfo->cmd_is_running == true) {
		dev_err(&client->dev, "%s: other cmd is running\n", __func__);
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&finfo->cmd_lock);
	finfo->cmd_is_running = true;
	mutex_unlock(&finfo->cmd_lock);

	finfo->cmd_state = RUNNING;

	for (i = 0; i < ARRAY_SIZE(finfo->cmd_param); i++)
		finfo->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;

	memset(finfo->cmd, 0x00, ARRAY_SIZE(finfo->cmd));
	memcpy(finfo->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &finfo->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &finfo->cmd_list_head, list) {
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
				finfo->cmd_param[param_cnt] =
								(int)simple_strtol(buff, NULL, 10);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}

	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n", i, finfo->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(touch_dev);

err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	dev_info(&client->dev, "tsp cmd: status:%d\n", finfo->cmd_state);

	if (finfo->cmd_state == WAITING)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "WAITING");

	else if (finfo->cmd_state == RUNNING)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "RUNNING");

	else if (finfo->cmd_state == OK)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "OK");

	else if (finfo->cmd_state == FAIL)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "FAIL");

	else if (finfo->cmd_state == NOT_APPLICABLE)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "NOT_APPLICABLE");

	return snprintf(buf, sizeof(finfo->cmd_buff),
					"%s\n", finfo->cmd_buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	struct i2c_client *client = touch_dev->client;
	struct tsp_factory_info *finfo = touch_dev->factory_info;

	dev_info(&client->dev, "tsp cmd: result: %s\n", finfo->cmd_result);

	mutex_lock(&finfo->cmd_lock);
	finfo->cmd_is_running = false;
	mutex_unlock(&finfo->cmd_lock);

	finfo->cmd_state = WAITING;

	return snprintf(buf, sizeof(finfo->cmd_result),
					"%s\n", finfo->cmd_result);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes,
};

#ifdef SUPPORTED_TOUCH_KEY
static ssize_t show_touchkey_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u16 threshold;
	int ret = 0;

	ret = read_data(misc_info->client, ZXT_BUTTON_SENSITIVITY,
			(u8*)&threshold, 2);

	if (ret < 0) {
		sprintf(buf, "%s", "failed");
		return 0;
	}

	sprintf(buf, "%d\n", threshold);

	return 0;
}
/*
static ssize_t back_key_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
}
*/
static ssize_t show_back_key_sensitivity(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	struct i2c_client *client = touch_dev->client;
	u16 val;
	int ret;

	disable_irq(touch_dev->irq);
	down(&touch_dev->work_lock);

	if (touch_dev->work_state != NOTHING) {
		dev_err(&client->dev, "%s: Other process occupied.. (%d)\n",
					__func__, touch_dev->work_state);
		goto err_out;
	}

	touch_dev->work_state = SET_MODE;

	ret = read_data(client, ZXT_BTN_WIDTH + 1, (u8*)&val, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Failed to read key sensitivity\n",
					__func__);
		touch_dev->work_state = NOTHING;
		goto err_out;
	}

	touch_dev->work_state = NOTHING;
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);

	dev_info(&client->dev, "%s: back key sensitivity = %d\n", __func__, val);

	return snprintf(buf, 6, "%d", val);

err_out:
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);

	return snprintf(buf, 3, "%s", "NG");
}

static ssize_t show_menu_key_sensitivity(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	struct i2c_client *client = touch_dev->client;
	u16 val;
	int ret;

	disable_irq(touch_dev->irq);
	down(&touch_dev->work_lock);

	if (touch_dev->work_state != NOTHING) {
		dev_err(&client->dev, "%s: Other process occupied.. (%d)\n",
			__func__, touch_dev->work_state);
		goto err_out;
	}

	touch_dev->work_state = SET_MODE;

	ret = read_data(client, ZXT_BTN_WIDTH + 0, (u8*)&val, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Failed to read key sensitivity\n",
					__func__);
		touch_dev->work_state = NOTHING;
		goto err_out;
	}

	touch_dev->work_state = NOTHING;
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);

	dev_info(&client->dev, "%s: menu key sensitivity = %d\n", __func__, val);

	return snprintf(buf, 6, "%d", val);

err_out:
	enable_irq(touch_dev->irq);
	up(&touch_dev->work_lock);

	return snprintf(buf, 3, "%s", "NG");
}

static ssize_t show_autocal_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_back_key_raw_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_menu_key_raw_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_back_key_idac_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_menu_key_idac_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t touchkey_led_control(struct device *dev,
struct device_attribute *attr, const char *buf,
	size_t size)
{
	struct bt532_ts_data *touch_dev = dev_get_drvdata(dev);
	int cmd;
	int ret;

	ret = sscanf(buf, "%d", &cmd);
	if (ret != 1) {
		printk(KERN_DEBUG "[TouchKey] %s, %d err\n",
			__func__, __LINE__);
		return size;
	}

	printk(KERN_DEBUG "[TouchKey] %s wrong cmd %x\n",
		__func__, cmd);

	if (cmd != 0 && cmd != 1) {
		printk(KERN_DEBUG "[TouchKey] %s wrong cmd %x\n",
			__func__, cmd);
		return size;
	}

	touch_dev->pdata->led_power(cmd);

	return size;
}

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, show_touchkey_threshold, NULL);
/* static DEVICE_ATTR(touch_sensitivity, S_IRUGO, back_key_state_show, NULL); */
static DEVICE_ATTR(touchkey_back, S_IRUGO, show_back_key_sensitivity, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, show_menu_key_sensitivity, NULL);
static DEVICE_ATTR(autocal_stat, S_IRUGO, show_autocal_status, NULL);
static DEVICE_ATTR(touchkey_raw_back, S_IRUGO, show_back_key_raw_data, NULL);
static DEVICE_ATTR(touchkey_raw_menu, S_IRUGO, show_menu_key_raw_data, NULL);
static DEVICE_ATTR(touchkey_idac_back, S_IRUGO, show_back_key_idac_data, NULL);
static DEVICE_ATTR(touchkey_idac_menu, S_IRUGO, show_menu_key_idac_data, NULL);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_led_control);

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	/*&dev_attr_touch_sensitivity.attr,*/
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_autocal_stat.attr,
	&dev_attr_touchkey_raw_back.attr,
	&dev_attr_touchkey_raw_menu.attr,
	&dev_attr_touchkey_idac_back.attr,
	&dev_attr_touchkey_idac_menu.attr,
	&dev_attr_brightness.attr,
	NULL,
};
static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};
#endif

static int init_sec_factory(struct bt532_ts_data *touch_dev)
{
	struct device *factory_ts_dev;
#ifdef SUPPORTED_TOUCH_KEY
	struct device *factory_tk_dev;
#endif
	struct tsp_factory_info *factory_info;
	struct tsp_raw_data *raw_data;
	int ret;
	int i;

	factory_info = kzalloc(sizeof(struct tsp_factory_info), GFP_KERNEL);
	raw_data = kzalloc(sizeof(struct tsp_raw_data), GFP_KERNEL);
	if (unlikely(!factory_info || !raw_data)) {
		dev_err(&touch_dev->client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	INIT_LIST_HEAD(&factory_info->cmd_list_head);
	for(i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &factory_info->cmd_list_head);

	factory_ts_dev = device_create(sec_class, NULL, 0, touch_dev, "tsp");
	if (unlikely(!factory_ts_dev)) {
		dev_err(&touch_dev->client->dev, "Failed to create factory dev\n");
		ret = -ENODEV;
		goto err_create_device;
	}

#ifdef SUPPORTED_TOUCH_KEY
	factory_tk_dev = device_create(sec_class, NULL, 0, touch_dev, "sec_touchkey");
	if (IS_ERR(factory_tk_dev)) {
		dev_err(&touch_dev->client->dev, "Failed to create factory dev\n");
		ret = -ENODEV;
		goto err_create_device;
	}
#endif

	ret = sysfs_create_group(&factory_ts_dev->kobj, &touchscreen_attr_group);
	if (unlikely(ret)) {
		dev_err(&touch_dev->client->dev, "Failed to create touchscreen sysfs group\n");
		goto err_create_sysfs;
	}

#ifdef SUPPORTED_TOUCH_KEY
	ret = sysfs_create_group(&factory_tk_dev->kobj, &touchkey_attr_group);
	if (unlikely(ret)) {
		dev_err(&touch_dev->client->dev, "Failed to create touchkey sysfs group\n");
		goto err_create_sysfs;
	}
#endif

	mutex_init(&factory_info->cmd_lock);
	factory_info->cmd_is_running = false;

	touch_dev->factory_info = factory_info;
	touch_dev->raw_data = raw_data;

	return ret;

err_create_sysfs:
err_create_device:
	kfree(raw_data);
	kfree(factory_info);
err_alloc:

	return ret;
}
#endif

static int ts_misc_fops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ts_misc_fops_close(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ts_misc_fops_ioctl(struct inode *inode,
	struct file *filp, unsigned int cmd,
	unsigned long arg)
#else
*/
static long ts_misc_fops_ioctl(struct file *filp,
	unsigned int cmd, unsigned long arg)
/* #endif */
{
	void __user *argp = (void __user *)arg;
	struct raw_ioctl raw_ioctl;
	u8 *u8Data;
	int ret = 0;
	size_t sz = 0;
	u16 version;
	u16 mode;

	struct reg_ioctl reg_ioctl;
	u16 val;
	int nval = 0;

	if (misc_info == NULL)
		return -1;

	/* zinitix_debug_msg("cmd = %d, argp = 0x%x\n", cmd, (int)argp); */

	switch (cmd) {

	case TOUCH_IOCTL_GET_DEBUGMSG_STATE:
		ret = m_ts_debug_mode;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_SET_DEBUGMSG_STATE:
		if (copy_from_user(&nval, argp, 4)) {
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}
		if (nval)
			printk(KERN_INFO "[zinitix_touch] on debug mode (%d)\n",
				nval);
		else
			printk(KERN_INFO "[zinitix_touch] off debug mode (%d)\n",
				nval);
		m_ts_debug_mode = nval;
		break;

	case TOUCH_IOCTL_GET_CHIP_REVISION:
		ret = misc_info->cap_info.ic_revision;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_FW_VERSION:
		ret = misc_info->cap_info.fw_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_REG_DATA_VERSION:
		ret = misc_info->cap_info.reg_data_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_SIZE:
		if (copy_from_user(&sz, argp, sizeof(size_t)))
			return -1;

		printk(KERN_INFO "firmware size = %d\r\n", sz);
		if (misc_info->cap_info.ic_fw_size != sz) {
			printk(KERN_INFO "firmware size error\r\n");
			return -1;
		}
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_DATA:
		if (copy_from_user(m_firmware_data,
			argp, misc_info->cap_info.ic_fw_size))
			return -1;

		version = (u16) (m_firmware_data[52] | (m_firmware_data[53]<<8));

		printk(KERN_INFO "firmware version = %x\r\n", version);

		if (copy_to_user(argp, &version, sizeof(version)))
			return -1;
		break;

	case TOUCH_IOCTL_START_UPGRADE:
		return ts_upgrade_sequence((u8*)m_firmware_data);

	case TOUCH_IOCTL_GET_X_RESOLUTION:
		ret = misc_info->pdata->x_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_RESOLUTION:
		ret = misc_info->pdata->y_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_X_NODE_NUM:
		ret = misc_info->cap_info.x_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_NODE_NUM:
		ret = misc_info->cap_info.y_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_TOTAL_NODE_NUM:
		ret = misc_info->cap_info.total_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_HW_CALIBRAION:
		ret = -1;
		disable_irq(misc_info->irq);
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = HW_CALIBRAION;
		mdelay(100);

		/* h/w calibration */
		if(ts_hw_calibration(misc_info) == true)
			ret = 0;

		mode = misc_info->touch_mode;
		if (write_reg(misc_info->client,
			ZXT_TOUCH_MODE, mode) != I2C_SUCCESS) {
			printk(KERN_INFO "failed to set touch mode %d.\n",
				mode);
			goto fail_hw_cal;
		}

		if (write_cmd(misc_info->client,
			ZXT_SWRESET_CMD) != I2C_SUCCESS)
			goto fail_hw_cal;

		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;
fail_hw_cal:
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;

	case TOUCH_IOCTL_SET_RAW_DATA_MODE:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		if (copy_from_user(&nval, argp, 4)) {
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\r\n");
			misc_info->work_state = NOTHING;
			return -1;
		}
		ts_set_touchmode((u16)nval);

		return 0;

	case TOUCH_IOCTL_GET_REG:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			printk(KERN_INFO "other process occupied.. (%d)\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;

		if (copy_from_user(&reg_ioctl,
			argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (read_data(misc_info->client,
			reg_ioctl.addr, (u8 *)&val, 2) < 0)
			ret = -1;

		nval = (int)val;

		if (copy_to_user(reg_ioctl.val, (u8 *)&nval, 4)) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_to_user\n");
			return -1;
		}

		zinitix_debug_msg("read : reg addr = 0x%x, val = 0x%x\n",
			reg_ioctl.addr, nval);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_SET_REG:

		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			printk(KERN_INFO "other process occupied.. (%d)\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (copy_from_user(&reg_ioctl,
				argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (copy_from_user(&val, reg_ioctl.val, 4)) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (write_reg(misc_info->client,
			reg_ioctl.addr, val) != I2C_SUCCESS)
			ret = -1;

		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x%x\r\n",
			reg_ioctl.addr, val);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_DONOT_TOUCH_EVENT:

		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (write_reg(misc_info->client,
			ZXT_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			ret = -1;
		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x0\r\n",
			ZXT_INT_ENABLE_FLAG);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_SEND_SAVE_STATUS:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = SET_MODE;
		ret = 0;
		write_reg(misc_info->client, 0xc003, 0x0001);
		write_reg(misc_info->client, 0xc104, 0x0001);
		if (write_cmd(misc_info->client,
			ZXT_SAVE_STATUS_CMD) != I2C_SUCCESS)
			ret =  -1;

		mdelay(1000);	/* for fusing eeprom */
		write_reg(misc_info->client, 0xc003, 0x0000);
		write_reg(misc_info->client, 0xc104, 0x0000);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_GET_RAW_DATA:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}

		if (misc_info->touch_mode == TOUCH_POINT_MODE)
			return -1;

		down(&misc_info->raw_data_lock);
		if (misc_info->update == 0) {
			up(&misc_info->raw_data_lock);
			return -2;
		}

		if (copy_from_user(&raw_ioctl,
			argp, sizeof(raw_ioctl))) {
			up(&misc_info->raw_data_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\r\n");
			return -1;
		}

		misc_info->update = 0;

		u8Data = (u8 *)&misc_info->cur_data[0];

		if (copy_to_user(raw_ioctl.buf, (u8 *)u8Data,
			raw_ioctl.sz)) {
			up(&misc_info->raw_data_lock);
			return -1;
		}

		up(&misc_info->raw_data_lock);
		return 0;

	default:
		break;
	}
	return 0;
}

static int bt532_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct bt532_ts_platform_data *pdata = client->dev.platform_data;
	struct bt532_ts_data *touch_dev;
	struct input_dev *input;
	int ret = 0;
	int i;

	if (!pdata) {
		dev_err(&client->dev, "Not exist platform data\n");
		return -EINVAL;
	}

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		dev_err(&client->dev, "failed to check i2c functionality\n");
		return -EIO;
	}

	touch_dev = kzalloc(sizeof(struct bt532_ts_data), GFP_KERNEL);
	if (!touch_dev) {
		dev_err(&client->dev, "failed to allocate touch_dev\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, touch_dev);
	touch_dev->client = client;
	touch_dev->pdata = pdata;

	input = input_allocate_device();
	if (input == NULL) {
		dev_err(&client->dev, "Failed to allocate input device\n");
		goto err_alloc;
	}

	touch_dev->input_dev = input;

	// power on
	if (bt532_power_control(touch_dev, POWER_ON_SEQUENCE) == false) {
		goto err_power_sequence;
	}

/* To Do */
/* FW version read from tsp */

	memset(&touch_dev->reported_touch_info,
		0x0, sizeof(struct point_info));

	/* init touch mode */
	touch_dev->touch_mode = TOUCH_POINT_MODE;
	misc_info = touch_dev;

	if(init_touch(touch_dev) == false) {
		goto err_input_register_device;
	}

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		touch_dev->button[i] = ICON_BUTTON_UNCHANGE;

	snprintf(touch_dev->phys, sizeof(touch_dev->phys),
		"%s/input0", dev_name(&client->dev));
	input->name = "sec_touchscreen";
	input->id.bustype = BUS_I2C;
/*	input->id.vendor = 0x0001; */
	input->phys = touch_dev->phys;
/*	input->id.product = 0x0002; */
/*	input->id.version = 0x0100; */

	set_bit(EV_SYN, touch_dev->input_dev->evbit);
	set_bit(EV_KEY, touch_dev->input_dev->evbit);
	set_bit(EV_LED, touch_dev->input_dev->evbit);
	set_bit(LED_MISC, touch_dev->input_dev->ledbit);
	set_bit(EV_ABS, touch_dev->input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, touch_dev->input_dev->propbit);

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		set_bit(BUTTON_MAPPING_KEY[i], touch_dev->input_dev->keybit);

	if (pdata->orientation & TOUCH_XY_SWAP) {
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_Y,
			touch_dev->cap_info.x_min,
			touch_dev->cap_info.x_max + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_X,
			touch_dev->cap_info.y_min,
			touch_dev->cap_info.y_max + ABS_PT_OFFSET,
			0, 0);
	} else {
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_X,
			touch_dev->cap_info.x_min,
			touch_dev->cap_info.x_max + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_Y,
			touch_dev->cap_info.y_min,
			touch_dev->cap_info.y_max + ABS_PT_OFFSET,
			0, 0);
	}

	input_set_abs_params(touch_dev->input_dev, ABS_MT_TOUCH_MAJOR,
		0, 255, 0, 0);
	input_set_abs_params(touch_dev->input_dev, ABS_MT_WIDTH_MAJOR,
		0, 255, 0, 0);

#if (TOUCH_POINT_MODE == 2)
	input_set_abs_params(touch_dev->input_dev, ABS_MT_TOUCH_MINOR,
		0, 255, 0, 0);
/*	input_set_abs_params(touch_dev->input_dev, ABS_MT_WIDTH_MINOR,
		0, 255, 0, 0); */
	input_set_abs_params(touch_dev->input_dev, ABS_MT_ORIENTATION,
		-128, 127, 0, 0);
/*	input_set_abs_params(touch_dev->input_dev, ABS_MT_ANGLE,
		-90, 90, 0, 0);
	input_set_abs_params(touch_dev->input_dev, ABS_MT_PALM,
		0, 1, 0, 0); */
#endif

	set_bit(MT_TOOL_FINGER, touch_dev->input_dev->keybit);
	input_mt_init_slots(touch_dev->input_dev, touch_dev->cap_info.multi_fingers);

	input_set_abs_params(touch_dev->input_dev, ABS_MT_TRACKING_ID,
		0, touch_dev->cap_info.multi_fingers, 0, 0);

	zinitix_debug_msg("register %s input device \r\n",
		touch_dev->input_dev->name);
	input_set_drvdata(input, touch_dev);
	ret = input_register_device(touch_dev->input_dev);
	if (ret) {
		printk(KERN_ERR "unable to register %s input device\r\n",
			touch_dev->input_dev->name);
		goto err_input_register_device;
	}

	/* configure irq */
	touch_dev->irq = gpio_to_irq(pdata->gpio_int);
	if (touch_dev->irq < 0)
		printk(KERN_INFO "error. gpio_to_irq(..) function is not \
			supported? you should define GPIO_TOUCH_IRQ.\r\n");

	zinitix_debug_msg("request irq (irq = %d, pin = %d) \r\n",
		touch_dev->irq, pdata->gpio_int);

	touch_dev->work_state = NOTHING;
	sema_init(&touch_dev->work_lock, 1);

	/* ret = request_threaded_irq(touch_dev->irq, ts_int_handler, bt532_touch_work,*/
	ret = request_threaded_irq(touch_dev->irq, NULL, bt532_touch_work,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT , BT532_TS_NAME, touch_dev);

	if (ret) {
		printk(KERN_ERR "unable to register irq.(%s)\r\n",
			touch_dev->input_dev->name);
		goto err_request_irq;
	}
	dev_info(&client->dev, "zinitix touch probe.\r\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	touch_dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	touch_dev->early_suspend.suspend = bt532_ts_early_suspend;
	touch_dev->early_suspend.resume = bt532_ts_late_resume;
	register_early_suspend(&touch_dev->early_suspend);
#endif

#if defined(CONFIG_PM_RUNTIME)
	pm_runtime_enable(&client->dev);
#endif

	sema_init(&touch_dev->raw_data_lock, 1);

	ret = misc_register(&touch_misc_device);
	if (ret)
		zinitix_debug_msg("Fail to register touch misc device.\n");

#ifdef SEC_FACTORY_TEST
	init_sec_factory(touch_dev);
#endif

#if ESD_TIMER_INTERVAL
	INIT_WORK(&touch_dev->tmr_work, ts_tmr_work);
	esd_tmr_workqueue =
		create_singlethread_workqueue("esd_tmr_workqueue");

	if (!esd_tmr_workqueue) {
		dev_err(&client->dev, "Failed to create esd tmr work queue\n");
		goto err_kthread_create_failed;
	}

	esd_timer_init(touch_dev);
	esd_timer_start(CHECK_ESD_TIMER, touch_dev);
	dev_err(&client->dev, "Started esd timer\n");
#endif

	return 0;

#if ESD_TIMER_INTERVAL
err_kthread_create_failed:
#endif
err_request_irq:
	input_unregister_device(touch_dev->input_dev);
err_input_register_device:
	input_free_device(touch_dev->input_dev);
err_power_sequence:
err_alloc:
	kfree(touch_dev);
	return ret;
}

static int bt532_ts_remove(struct i2c_client *client)
{
	struct bt532_ts_data *touch_dev = i2c_get_clientdata(client);
	struct bt532_ts_platform_data *pdata = touch_dev->pdata;

	if(touch_dev == NULL)
		return 0;

	disable_irq(touch_dev->irq);
	down(&touch_dev->work_lock);

	touch_dev->work_state = REMOVE;

#if ESD_TIMER_INTERVAL
	flush_work(&touch_dev->tmr_work);
	write_reg(touch_dev->client, ZXT_PERIODICAL_INTERRUPT_INTERVAL, 0);
	esd_timer_stop(touch_dev);
	dev_info(&client->dev, "Stopped esd timer\n");
	destroy_workqueue(esd_tmr_workqueue);
#endif

	if (touch_dev->irq)
		free_irq(touch_dev->irq, touch_dev);

	misc_deregister(&touch_misc_device);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&touch_dev->early_suspend);
#endif

	if (gpio_is_valid(pdata->gpio_int) != 0)
		gpio_free(pdata->gpio_int);

	input_unregister_device(touch_dev->input_dev);
	input_free_device(touch_dev->input_dev);
	up(&touch_dev->work_lock);
	kfree(touch_dev);

	return 0;
}

static struct i2c_device_id bt532_idtable[] = {
	{BT532_TS_NAME, 0},
	{ }
};

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops bt532_ts_pm_ops = {
#if defined(CONFIG_PM_RUNTIME)
	SET_RUNTIME_PM_OPS(bt532_ts_suspend, bt532_ts_resume, NULL)
#else
	SET_SYSTEM_SLEEP_PM_OPS(bt532_ts_suspend, bt532_ts_resume)
#endif
};
#endif

static struct i2c_driver bt532_ts_driver = {
	.probe	= bt532_ts_probe,
	.remove	= bt532_ts_remove,
	.id_table	= bt532_idtable,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= BT532_TS_NAME,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		.pm		= &bt532_ts_pm_ops,
#endif
	},
};

static int __devinit bt532_ts_init(void)
{
	return i2c_add_driver(&bt532_ts_driver);
}

static void __exit bt532_ts_exit(void)
{
	i2c_del_driver(&bt532_ts_driver);
}

module_init(bt532_ts_init);
module_exit(bt532_ts_exit);

MODULE_DESCRIPTION("touch-screen device driver using i2c interface");
MODULE_AUTHOR("<junik.lee@samsung.com>");
MODULE_LICENSE("GPL");
