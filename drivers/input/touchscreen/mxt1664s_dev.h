/*
 *  Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __MXT_1664S_DEV_H
#define __MXT_1664S_DEV_H

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define OBJECT_TABLE_ELEMENT_SIZE	6
#define OBJECT_TABLE_START_ADDRESS	7

#define CMD_RESET_OFFSET		0
#define CMD_BACKUP_OFFSET		1
#define CMD_CALIBRATE_OFFSET		2
#define CMD_REPORTATLL_OFFSET		3
#define CMD_DEBUG_CTRL_OFFSET		4
#define CMD_DIAGNOSTIC_OFFSET		5

#define DETECT_MSG_MASK		0x80
#define PRESS_MSG_MASK		0x40
#define RELEASE_MSG_MASK	0x20
#define MOVE_MSG_MASK		0x10
#define AMPLITUDE_MSG_MASK	0x04
#define SUPPRESS_MSG_MASK	0x02

/* Slave addresses */
#define MXT_APP_LOW		0x4a
#define MXT_APP_HIGH		0x4b
#define MXT_BOOT_LOW		0x26
#define MXT_BOOT_HIGH		0x27

#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE	0x55

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0
#define MXT_WAITING_FRAME_DATA		0x80
#define MXT_FRAME_CRC_CHECK		0x02
#define MXT_FRAME_CRC_FAIL		0x03
#define MXT_FRAME_CRC_PASS		0x04
#define MXT_APP_CRC_FAIL		0x40
#define MXT_BOOT_STATUS_MASK		0x3f

/* Bootloader ID */
#define MXT_BOOT_EXTENDED_ID		0x20
#define MXT_BOOT_ID_MASK		0x1f

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB		0xaa
#define MXT_UNLOCK_CMD_LSB		0xdc

#define ID_BLOCK_SIZE			7

#define MXT_STATE_INACTIVE		-1
#define MXT_STATE_RELEASE		0
#define MXT_STATE_PRESS			1
#define MXT_STATE_MOVE			2

/* Diagnostic cmds  */
#define MXT_DIAG_PAGE_UP		0x01
#define MXT_DIAG_PAGE_DOWN		0x02
#define MXT_DIAG_DELTA_MODE		0x10
#define MXT_DIAG_REFERENCE_MODE		0x11
#define MXT_DIAG_CTE_MODE		0x31
#define MXT_DIAG_IDENTIFICATION_MODE	0x80
#define MXT_DIAG_TOCH_THRESHOLD_MODE	0xF4

#define MXT_DIAG_MODE_MASK	0xFC
#define MXT_DIAGNOSTIC_MODE	0
#define MXT_DIAGNOSTIC_PAGE	1

/* Firmware name */
#define MXT_FW_NAME		"mXT1664S.fw"
#define MXT_MAX_FW_PATH		255

/* Firmware version */
#define MXT_FIRM_VERSION	0x10
#define MXT_FIRM_BUILD		0xAA

/* Feature */
#define TSP_FIRMUP_ON_PROBE	1
#define TSP_BOOSTER			1
#define TSP_DEBUG_INFO			0
#define TSP_SEC_SYSFS			1
#define TSP_USE_SHAPETOUCH	1
#define CHECK_ANTITOUCH		1
#define TSP_INFORM_CHARGER	1

/* TSP_ITDEV feature just for atmel tunning app
* so it should be disabled after finishing tunning
* because it use other write permission. it will be cause
* failure of CTS
*/
#define TSP_ITDEV		1

#define MXT_T7_IDLE_ACQ_INT	0
#define MXT_T7_ACT_ACQ_INT	1

#if CHECK_ANTITOUCH
#define MXT_T61_TIMER_ONESHOT	0
#define MXT_T61_TIMER_REPEAT	1
#define MXT_T61_TIMER_CMD_START		1
#define MXT_T61_TIMER_CMD_STOP		2
#endif

#if TSP_SEC_SYSFS
#define TSP_BUF_SIZE	 1024

#define NODE_NUM	1664

#define NODE_PER_PAGE	64
#define DATA_PER_NODE	2

#define REF_OFFSET_VALUE	16384
#define REF_MIN_VALUE		(19744 - REF_OFFSET_VALUE)
#define REF_MAX_VALUE		(28884 - REF_OFFSET_VALUE)

#define TSP_CMD_STR_LEN		32
#define TSP_CMD_RESULT_STR_LEN	512
#define TSP_CMD_PARAM_NUM	8

enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

enum {
	MXT_FW_FROM_BUILT_IN = 0,
	MXT_FW_FROM_UMS,
	MXT_FW_FROM_REQ_FW,
};
#endif

#if TSP_BOOSTER
#include <mach/cpufreq.h>
#include <mach/dev.h>
#define SEC_DVFS_LOCK_TIMEOUT	200
#define SEC_DVFS_LOCK_FREQ		800000
#define SEC_BUS_LOCK_FREQ		267160
#define SEC_BUS_LOCK_FREQ2	400200
#endif

struct mxt_object {
	u8 object_type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;
} __packed;

struct mxt_info_block {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_finger_info {
	s16 x;
	s16 y;
	s16 z;
	u16 w;
	s8 state;
#if TSP_USE_SHAPETOUCH
	int16_t component;
#endif
	u16 mcount;
};

struct mxt_report_id_map {
	u8 object_type;
	u8 instance;
};

#if TSP_BOOSTER
struct touch_booster {
	bool touch_cpu_lock_status;
	int cpu_lv;
	struct delayed_work dvfs_dwork;
	struct device *bus_dev;
	struct device *dev;
};
#endif

#if TSP_SEC_SYSFS
struct mxt_data_sysfs {
	struct list_head			cmd_list_head;
	u8			cmd_state;
	char			cmd[TSP_CMD_STR_LEN];
	int			cmd_param[TSP_CMD_PARAM_NUM];
	char			cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex			cmd_lock;
	bool			cmd_is_running;

	u16 reference[NODE_NUM];
	s16 delta[NODE_NUM];

	u32 ref_max_data;
	u32 ref_min_data;
	s16 delta_max_data;
	u16 delta_max_node;
};
#endif

struct mxt_data {
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct input_dev *input_dev;
	const struct mxt_platform_data *pdata;
	struct mxt_info_block info;
	struct mxt_object *objects;
	struct mxt_report_id_map *rid_map;
	struct mxt_callbacks callbacks;
	struct delayed_work resume_dwork;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#if TSP_SEC_SYSFS
	struct mxt_data_sysfs *sysfs_data;
#endif
#if TSP_BOOSTER
	struct touch_booster booster;
#endif
#if TSP_INFORM_CHARGER
	struct delayed_work noti_dwork;
	struct delayed_work acq_int_dwork;
	bool charging_mode;
	u8 charger_inform_buf_size;
	u8 *charger_inform_buf;
#endif
#ifdef TSP_ITDEV
	int driver_paused;
	int debug_enabled;
	u16 last_read_addr;
#endif
#if CHECK_ANTITOUCH
	u8 check_antitouch;
	u8 check_timer;
	u8 check_autocal;
	u8 check_calgood;
#endif
	const char *config_version;
	u8 tsp_ctl;
	u8 x_num;
	u8 y_num;
	u8 max_report_id;
	u8 finger_report_id;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;
	u32 finger_mask;
	int num_fingers;
	bool mxt_enabled;
	bool debug_log;
#if TSP_USE_SHAPETOUCH
	int16_t sumsize;
#endif
	struct mutex lock;
	struct mxt_finger_info fingers[];
};


#if TSP_SEC_SYSFS
extern struct class *sec_class;
#endif

extern int  __devinit mxt_sysfs_init(struct i2c_client *client);

extern int mxt_read_mem(struct mxt_data *data, u16 reg, u8 len, u8 *buf);
extern int mxt_write_mem(struct mxt_data *data,	u16 reg, u8 len, const u8 *buf);
extern struct mxt_object *
	mxt_get_object_info(struct mxt_data *data, u8 object_type);
extern int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val);
extern int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val);

#if TSP_SEC_SYSFS
extern int mxt_flash_fw_from_sysfs(struct mxt_data *data,
		const u8 *fw_data, size_t fw_size);
#endif

#if TSP_BOOSTER
extern void mxt_set_dvfs_on(struct mxt_data *data, bool en);
extern int mxt_init_dvfs(struct mxt_data *data);
#endif	/* TSP_BOOSTER */

#endif /* __MXT_1664S_DEV_H */
