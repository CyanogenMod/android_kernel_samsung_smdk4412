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
#define PAGE_MAX				0X400
#define NEXT_PAGE				0X100
#define MAX_FUNC				0x55
#define CHARGER_CONNECT_BIT	(0x1 << 5)

#define MIN_ANGLE	-90
#define MAX_ANGLE	90

#define BUTTON_0_MASK		(1 << 0)
#define BUTTON_1_MASK		(1 << 1)
#define BUTTON_2_MASK		(1 << 2)
#define BUTTON_3_MASK		(1 << 3)
#define BUTTON_4_MASK		(1 << 4)
#define BUTTON_5_MASK		(1 << 5)
#define BUTTON_6_MASK		(1 << 6)
#define BUTTON_7_MASK		(1 << 7)

/* button threshhold limit */
#define BUTTON_THRESHOLD_LIMIT	1000
#define BUTTON_THRESHOLD_MIN		0

/* fixed threshold */
#define BUTTON2_0_THRESHOLD	78
#define BUTTON2_1_THRESHOLD	50

#if defined(CONFIG_KONA_01_BD)
#define BUTTON5_0_THRESHOLD	250
#define BUTTON5_1_THRESHOLD	70
#define BUTTON5_2_THRESHOLD	220
#define BUTTON5_3_THRESHOLD	70
#define BUTTON5_4_THRESHOLD	250
#else
#define BUTTON5_0_THRESHOLD	256
#define BUTTON5_1_THRESHOLD	65
#define BUTTON5_2_THRESHOLD	400
#define BUTTON5_3_THRESHOLD	70
#define BUTTON5_4_THRESHOLD	300
#endif

enum BUTTON{
	BUTTON1 = 0,
	BUTTON2,
	BUTTON3,
	BUTTON4,
	BUTTON5,
	BUTTON6,
	BUTTON7,
	BUTTON8,
	BUTTON_MAX,
};

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
        int angle;
        int width;

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

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
struct synaptics_button_map {
	u8 nbuttons;
	u8 *map;
};

struct synaptics_extend_button_map {
	u8 nbuttons;
	u8 *map;
	u8 button_mask;
};

#endif

struct synaptics_platform_data {
	int gpio_attn;
	int max_x;
	int max_y;
	int max_pressure;
	int max_width;
	bool swap_xy;
	bool invert_x;
	bool invert_y;
#ifdef CONFIG_SEC_TOUCHSCREEN_SURFACE_TOUCH
	u8 palm_threshold;
#endif
	u16 x_line;
	u16 y_line;
	int (*set_power)(bool);
	void (*hw_reset)(void);
	void	(*register_cb)(struct charger_callbacks *);
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
	void (*led_control)(int);
	bool led_event;
#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	struct synaptics_button_map *button_map;
	struct synaptics_extend_button_map *extend_button_map;
	bool support_extend_button;
	bool enable_extend_button_event;
	int button_pressure[BUTTON_MAX];
#endif
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
#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	struct function_info f1a;
#endif
	struct function_info f34;
	struct function_info f54;
#ifdef CONFIG_SEC_TOUCHSCREEN_SURFACE_TOUCH
	struct function_info f51;
#endif
	struct delayed_work init_dwork;
	struct delayed_work resume_dwork;
	struct delayed_work noti_dwork;
#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_WORKAROUND)
	struct delayed_work reset_dwork;
	bool firmware_update_check;
#endif
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
	u8 firm_version[5];
	u8 firm_config[13];
	u8 *cmd_temp;
	u8 *references;
	u8 *tx_to_tx;
	u8 *tx_to_gnd;
#if defined(CONFIG_SEC_TOUCHSCREEN_SURFACE_TOUCH)
	u8 palm_flag;
#endif
	u16 x_line;
	u16 y_line;
	u16 refer_max;
	u16 refer_min;
	u16 rx_to_rx[42][42];
	unsigned long func_bit[BITS_TO_LONGS(MAX_FUNC+1)];
	atomic_t keypad_enable;
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
