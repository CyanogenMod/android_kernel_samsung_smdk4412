/*
 * include/linux/synaptics_s7301.h
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef _LINUX_SYNAPTICS_TS_H_
#define _LINUX_SYNAPTICS_TS_H_

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>

#define SYNAPTICS_TS_NAME "synaptics_ts"
#define SYNAPTICS_TS_ADDR 0x20

#if defined(CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK)
#include <mach/cpufreq.h>
#include <mach/dev.h>
#define SEC_DVFS_LOCK_TIMEOUT	200
#define SEC_DVFS_LOCK_FREQ		800000
#define SEC_BUS_LOCK_FREQ		267160
#define SEC_BUS_LOCK_FREQ2	400200
#endif

#define MAX_TOUCH_NUM			10
#define I2C_RETRY_CNT			5
#define MAX_MT_CNT				10
#define MAX_CMD_SIZE			64
#define FUNC_ADDR_SIZE			6
#define FUNC_ADDR_LAST			0xdd
#define FUNC_ADDR_FIRST		0xe9
#define PAGE_MAX				0X100
#define MAX_FUNC				0x55
#define CHARGER_CONNECT_BIT	(0x1 << 5)

enum MT_STATUS {
	MT_STATUS_INACTIVE = 0,
	MT_STATUS_PRESS,
	MT_STATUS_MOVE,
	MT_STATUS_RELEASE,
	MT_STATUS_MAX,
};

struct finger_info {
	int x;
	int y;
	int w_min;
	int w_max;
	int z;
	int status;
};

struct finger_data {
	u8 x_msb;
	u8 y_msb;
	u8 xy_lsb;
	u8 w;
	u8 z;
};

struct function_info {
	u16 query_base_addr;
	u16 command_base_addr;
	u16 control_base_addr;
	u16 data_base_addr;
	u8 interrupt_source_count;
	u8 function_number;
	u8 function_version;
};

struct charger_callbacks {
	void (*inform_charger)(struct charger_callbacks *, int mode);
};

/* Variables for F34 functionality */
struct synaptics_ts_fw_block {
	u8 *fw_data;
	u8 *fw_imgdata;
	u8 *config_imgdata;
	u8 *lock_imgdata;
	u16 f01_database;
	u16 f01_commandbase;
	u16 f34_database;
	u16 f34_querybase;
	u16 f01_controlbase;
	u16 f34_reflash_blocknum;
	u16 f34_reflash_blockdata;
	u16 f34_reflashquery_boot_id;
	u16 f34_reflashquery_flashpropertyquery;
	u16 f34_reflashquery_fw_blocksize;
	u16 f34_reflashquery_fw_blockcount;
	u16 f34_reflashquery_config_blocksize;
	u16 f34_reflashquery_config_blockcount;
	u16 f34_flashcontrol;
	u16 fw_blocksize;
	u16 fw_blockcount;
	u16 config_blocksize;
	u16 config_blockcount;
	u16 fw_version;
	u16 boot_id;
	u32 imagesize;
	u32 config_imagesize;
};

struct synaptics_platform_data {
	int gpio_attn;
	int max_x;
	int max_y;
	int max_pressure;
	int max_width;
	u16 x_line;
	u16 y_line;
	int (*set_power)(bool);
	void (*hw_reset)(void);
	void	(*register_cb)(struct charger_callbacks *);
};

struct synaptics_drv_data {
	struct i2c_client	*client;
	struct device *dev;
	struct input_dev	*input;
	struct synaptics_platform_data *pdata;
	struct mutex	mutex;
	struct synaptics_ts_fw_block *fw;
	struct wake_lock wakelock;
	struct work_struct fw_update_work;
	struct function_info f01;
	struct function_info f11;
	struct function_info f34;
	struct function_info f54;
	struct delayed_work init_dwork;
	struct delayed_work resume_dwork;
	struct delayed_work noti_dwork;
	struct charger_callbacks callbacks;
	struct finger_info	finger[MAX_MT_CNT];
#if CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
#if defined(CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK)
	struct delayed_work dvfs_dwork;
	struct device *bus_dev;
	u32 cpufreq_level;
	bool dvfs_lock_status;
#endif
	bool ready;
	bool input_open;
	bool charger_connection;
	bool drawing_mode;
	bool suspend;
	bool debug;
	int gpio;
	u8 page;
	u8 cmd_status;
	u8 cmd_report_type;
	u8 cmd_result[MAX_CMD_SIZE];
	u8 firm_version[4];
	u8 firm_config[13];
	u8 *cmd_temp;
	u8 *references;
	u8 *tx_to_tx;
	u8 *tx_to_gnd;
	u16 x_line;
	u16 y_line;
	u16 refer_max;
	u16 refer_min;
	u16 rx_to_rx[42][42];
	unsigned long func_bit[BITS_TO_LONGS(MAX_FUNC+1)];
};

extern struct class *sec_class;
extern int set_tsp_sysfs(struct synaptics_drv_data *data);
extern void remove_tsp_sysfs(struct synaptics_drv_data *data);
extern int synaptics_fw_updater(struct synaptics_drv_data *data,
	u8 *fw_data);
extern void forced_fw_upload(struct synaptics_drv_data *data);
extern int synaptics_ts_write_data(struct synaptics_drv_data *data,
	u16 addr, u8 buf);
extern int synaptics_ts_read_data(struct synaptics_drv_data *data,
	u16 addr, u8 *buf);
extern int synaptics_ts_write_block(struct synaptics_drv_data *data,
	u16 addr, u8 *buf, u16 count);
extern int synaptics_ts_read_block(struct synaptics_drv_data *data,
	u16 addr, u8 *buf, u16 count);
extern void forced_fw_update(struct synaptics_drv_data *data);
extern void synaptics_ts_drawing_mode(struct synaptics_drv_data *data);

#endif	/* _LINUX_SYNAPTICS_TS_H_ */
