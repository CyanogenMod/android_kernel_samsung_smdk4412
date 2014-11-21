/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __MXT_H__
#define __MXT_H__

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define MXT_DEV_NAME	"Atmel MXT1188S"
//#include <asm/system_info.h>

#define FW_UPDATE_ENABLE		1
#define MXT_DEFAULT_FIRMWARE_NAME	"MXTS.fw"
#define MXT_FIRMWARE_NAME_UMS	"mXT1188S.fw"

#define MXT_FIRMWARE_INKERNEL_PATH	"tsp_atmel/"
#define MXT_MAX_FW_PATH			30
#define MXT_FIRMWARE_UPDATE_TYPE	true

#define MXT_BACKUP_TIME			25	/* msec */
#define MXT_RESET_INTEVAL_TIME		50	/* msec */

#define MXT_SW_RESET_TIME		300	/* msec */
#define MXT_HW_RESET_TIME		80	/* msec */

enum {
	MXT_RESERVED_T0 = 0,
	MXT_RESERVED_T1,
	MXT_DEBUG_DELTAS_T2,
	MXT_DEBUG_REFERENCES_T3,
	MXT_DEBUG_SIGNALS_T4,
	MXT_GEN_MESSAGEPROCESSOR_T5,
	MXT_GEN_COMMANDPROCESSOR_T6,
	MXT_GEN_POWERCONFIG_T7,
	MXT_GEN_ACQUISITIONCONFIG_T8,
	MXT_TOUCH_MULTITOUCHSCREEN_T9,
	MXT_TOUCH_SINGLETOUCHSCREEN_T10,
	MXT_TOUCH_XSLIDER_T11,
	MXT_TOUCH_YSLIDER_T12,
	MXT_TOUCH_XWHEEL_T13,
	MXT_TOUCH_YWHEEL_T14,
	MXT_TOUCH_KEYARRAY_T15,
	MXT_PROCG_SIGNALFILTER_T16,
	MXT_PROCI_LINEARIZATIONTABLE_T17,
	MXT_SPT_COMCONFIG_T18,
	MXT_SPT_GPIOPWM_T19,
	MXT_PROCI_GRIPFACESUPPRESSION_T20,
	MXT_RESERVED_T21,
	MXT_PROCG_NOISESUPPRESSION_T22,
	MXT_TOUCH_PROXIMITY_T23,
	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	MXT_SPT_SELFTEST_T25,
	MXT_DEBUG_CTERANGE_T26,
	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	MXT_SPT_CTECONFIG_T28,
	MXT_SPT_GPI_T29,
	MXT_SPT_GATE_T30,
	MXT_TOUCH_KEYSET_T31,
	MXT_TOUCH_XSLIDERSET_T32,
	MXT_RESERVED_T33,
	MXT_GEN_MESSAGEBLOCK_T34,
	MXT_SPT_GENERICDATA_T35,
	MXT_RESERVED_T36,
	MXT_DEBUG_DIAGNOSTIC_T37,
	MXT_SPT_USERDATA_T38,
	MXT_SPARE_T39,
	MXT_PROCI_GRIPSUPPRESSION_T40,
	MXT_SPARE_T41,
	MXT_PROCI_TOUCHSUPPRESSION_T42,
	MXT_SPT_DIGITIZER_T43,
	MXT_SPARE_T44,
	MXT_SPARE_T45,
	MXT_SPT_CTECONFIG_T46,
	MXT_PROCI_STYLUS_T47,
	MXT_PROCG_NOISESUPPRESSION_T48,
	MXT_SPARE_T49,
	MXT_SPARE_T50,
	MXT_SPARE_T51,
	MXT_TOUCH_PROXIMITY_KEY_T52,
	MXT_GEN_DATASOURCE_T53,
	MXT_SPARE_T54,
	MXT_ADAPTIVE_T55,
	MXT_PROCI_SHIELDLESS_T56,
	MXT_PROCI_EXTRATOUCHSCREENDATA_T57,
	MXT_SPARE_T58,
	MXT_SPARE_T59,
	MXT_SPARE_T60,
	MXT_SPT_TIMER_T61,
	MXT_PROCG_NOISESUPPRESSION_T62,
	MXT_PROCI_ACTIVESTYLUS_T63,
	MXT_SPARE_T64,
	MXT_PROCI_LENSBENDING_T65,
	MXT_SPT_GOLDENREFERENCES_T66,
	MXT_SPARE_T67,
	MXT_SPARE_T68,
	MXT_PROCI_PALMGESTUREPROCESSOR_T69,
	MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70,
	MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71,
	MXT_PROCG_NOISESUPPRESSION_T72,
	MXT_TOUCH_MULTITOUCHSCREEN_T100 = 100,
	MXT_SPT_TOUCHSCREENHOVER_T101,
	MXT_SPT_SELFCAPHOVERCTECONFIG_T102,
	MXT_RESERVED_T255 = 255,
};

#define MXT_INFOMATION_BLOCK_SIZE	7
#define MXT_OBJECT_TABLE_ELEMENT_SIZE	6
#define MXT_OBJECT_TABLE_START_ADDRESS	7

#define MXT_MAX_FINGER		10
#define MXT_AREA_MAX		255
#define MXT_AMPLITUDE_MAX	255

/* Information of each Object */

/* MXT_GEN_COMMAND_T6 Field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DEBUGCTL	4
#define MXT_COMMAND_DIAGNOSTIC	5

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_RESET_VALUE		0x01
#define MXT_BOOT_VALUE		0xa5
/*
 * If the firmware can support dynamic configuration,
 * 0x55 : backup configuration data and resume event handling.
 * 0x44 : restore configuration data
 * 0x33 : restore configuration data and stop event handiling.
 *
 * if the firmware did not support dynamic configuration,
 * 0x33, 0x44 is not affect.
 */
#define MXT_BACKUP_VALUE	0x55
#define MXT_RESTORE_VALUE	0x44
#define MXT_DISALEEVT_VALUE	0x33

/* MXT_GEN_POWER_T7 Field */
#define MXT_POWER_IDLEACQINT	0
#define MXT_POWER_ACTACQINT	1

/* MXT_TOUCH_MULTI_T9 Field */
#define MXT_TOUCH_CTRL		0
#define MXT_TOUCH_XSIZE		3
#define MXT_TOUCH_YSIZE		4
#define MXT_TOUCH_ORIENT	9
#define MXT_TOUCH_XRANGE_LSB	18
#define MXT_TOUCH_XRANGE_MSB	19
#define MXT_TOUCH_YRANGE_LSB	20
#define MXT_TOUCH_YRANGE_MSB	21

/* MXT_TOUCH_KEYARRAY_T15 Field */
#define MXT_KEYARRY_CTRL	0
#define MXT_KEYARRY_XORIGIN	1
#define MXT_KEYARRY_YORIGIN	2

/* Touch message bit masking value */
#define MXT_SUPPRESS_MSG_MASK	(1 << 1)
#define MXT_AMPLITUDE_MSG_MASK	(1 << 2)
#define MXT_VECTOR_MSG_MASK	(1 << 3)
#define MXT_MOVE_MSG_MASK	(1 << 4)
#define MXT_RELEASE_MSG_MASK	(1 << 5)
#define MXT_PRESS_MSG_MASK	(1 << 6)
#define MXT_DETECT_MSG_MASK	(1 << 7)

/* Slave addresses */
#define MXT_APP_LOW		0x4a
#define MXT_APP_HIGH		0x4b
#define MXT_BOOT_LOW		0x24
#define MXT_BOOT_HIGH		0x25

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

#define MXT_STATE_INACTIVE		0
#define MXT_STATE_RELEASE		1
#define MXT_STATE_PRESS			2
#define MXT_STATE_MOVE			3

/* Diagnostic command defines  */
#define MXT_DIAG_PAGE_UP		0x01
#define MXT_DIAG_PAGE_DOWN		0x02
#define MXT_DIAG_DELTA_MODE		0x10
#define MXT_DIAG_REFERENCE_MODE		0x11
#define MXT_DIAG_CTE_MODE		0x31
#define MXT_DIAG_IDENTIFICATION_MODE	0x80
#define MXT_DIAG_TOCH_THRESHOLD_MODE	0xF4

#define MXT_DIAG_MODE_MASK		0xFC
#define MXT_DIAGNOSTIC_MODE		0
#define MXT_DIAGNOSTIC_PAGE		1

#define MXT_CONFIG_VERSION_LENGTH	30

/* Touchscreen configuration infomation */
#define MXT_FW_MAGIC		0x4D3C2B1A

/* Message type of T100 object */
#define MXT_T100_SCREEN_MSG_FIRST_RPT_ID	0
#define MXT_T100_SCREEN_MSG_SECOND_RPT_ID	1
#define MXT_T100_SCREEN_MESSAGE_NUM_RPT_ID	2

/* Event Types of T100 object */
#define MXT_T100_DETECT_MSG_MASK	7

#define MXT_T100_EVENT_NONE		0
#define MXT_T100_EVENT_MOVE		1
#define MXT_T100_EVENT_UNSUPPRESS	2
#define MXT_T100_EVENT_SUPPESS		3
#define MXT_T100_EVENT_DOWN		4
#define MXT_T100_EVENT_UP		5
#define MXT_T100_EVENT_UNSUPSUP		6
#define MXT_T100_EVENT_UNSUPUP		7
#define MXT_T100_EVENT_DOWNSUP		8
#define MXT_T100_EVENT_DOWNUP		9

/* Tool types of T100 object */
#define MXT_T100_TYPE_RESERVED		0
#define MXT_T100_TYPE_FINGER		1
#define MXT_T100_TYPE_PASSIVE_STYLUS	2
#define MXT_T100_TYPE_ACTIVE_STYLUS	3
#define MXT_T100_TYPE_HOVERING_FINGER	4

/* Related Timer */
#define MXT_T61_TIMER_ONESHOT	0
#define MXT_T61_TIMER_REPEAT	1
#define MXT_T61_TIMER_CMD_START		1
#define MXT_T61_TIMER_CMD_STOP		2

/* Revision info of Touch IC
 *
 * Revision	/	Bootloader ID
 * G			0x2103
 * I			0x3303
 */
#define MXT_REVISION_G	0
#define MXT_REVISION_I	1	/* Support hovering */

/************** Feature + **************/
#define TSP_BOOSTER			0
#define TSP_SEC_FACTORY			1
#define TSP_INFORM_CHARGER		1
#define TSP_USE_SHAPETOUCH		0
#define ENABLE_TOUCH_KEY		1
#define TSP_CHECK_ANTITOUCH		1

/* TODO TEMP_HOVER : Need to check and modify
 * it can be changed related potocol of hover So current
 * implementation is temporary code.
 */
#define TSP_HOVER_WORKAROUND		0

/* TSP_USE_ATMELDBG feature just for atmel tunning app
* so it should be disabled after finishing tunning
* because it use other write permission. it will be cause
* failure of CTS
*/
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
#define TSP_USE_ATMELDBG		1
#else
#define TSP_USE_ATMELDBG		0
#endif
/************** Feature - **************/

#if TSP_USE_SHAPETOUCH
#define MXT_COMPONENT_MAX	255
#define MXT_SUMSIZE_MAX		(16 * 26)
#endif
#if TSP_SEC_FACTORY
#define TSP_BUF_SIZE		1024

#define NODE_PER_PAGE		64
#define DATA_PER_NODE		2

#define REF_OFFSET_VALUE	16384
#define REF_MIN_VALUE		(19744 - REF_OFFSET_VALUE)
#define REF_MAX_VALUE		(28884 - REF_OFFSET_VALUE)

#define TSP_CMD_STR_LEN		32
#define TSP_CMD_RESULT_STR_LEN	512
#define TSP_CMD_PARAM_NUM	8

/* Related Golden Reference */
#define MXT_FCALCMD(x)		((x) << 2)
#define MXT_FCALCMD_NONE	0
#define MXT_FCALCMD_PRIME	1
#define MXT_FCALCMD_GENERATE	2
#define MXT_FCALCMD_STORE	3

#define MXT_FCALSTATE(x)	(((x) & 0x06) >> 1)
#define MXT_FCALSTATE_IDLE	0
#define MXT_FCALSTATE_PRIMED	1
#define MXT_FCALSTATE_GENERATED	2

#define MXT_FCALSTATUS_FAIL		0x80
#define MXT_FCALSTATUS_PASS		0x40
#define MXT_FCALSTATUS_SEQDONE		0x20
#define MXT_FCALSTATUS_SEQTO		0x10
#define MXT_FCALSTATUS_SEQERR		0x08
#define MXT_FCALSTATUS_BADSTORED	0x01

#define FCALSEQDONE_MAGIC		0x7777

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
#define SEC_DVFS_LOCK_FREQ	800000
#define SEC_BUS_LOCK_FREQ	267160
#define SEC_BUS_LOCK_FREQ2	400200
#endif

#if ENABLE_TOUCH_KEY
/* MXT_TOUCH_KEYARRAY_T15 Message address definitions */
#define MXT_MSG_T15_STATUS	0x00
#define MXT_MSG_T15_KEYSTATE	0x01
#define MXT_MSGB_T15_DETECT	0x80

/* TSP Key Feature*/
#define KEY_PRESS       1
#define KEY_RELEASE     0
#define TOUCH_KEY_NULL	0

struct mxt_touchkey {
	unsigned int value;
	unsigned int keycode;
	char *name;
	unsigned int x_node;
	unsigned int y_node;
};
#endif

struct mxt_callbacks {
	void (*inform_charger)(struct mxt_callbacks *, bool);
};

struct mxt_platform_data {
	unsigned char num_xnode;
	unsigned char num_ynode;
	unsigned int max_x;
	unsigned int max_y;
	unsigned long irqflags;
	unsigned char boot_address;
	unsigned char revision;
	const char *firmware_name;
	const char *project_name;
	const u8 **config;
	bool (*read_chg)(void);
	int (*power_on) (void);
	int (*power_off) (void);
	int (*power_reset) (void);
	void (*register_cb) (void *);
#if ENABLE_TOUCH_KEY
	unsigned int num_touchkey;
	struct mxt_touchkey *touchkey;
	int (*led_power_on) (void);
	int (*led_power_off) (void);
#endif
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;

	/* added for mapping obj to report id*/
	u8 max_reportid;
};

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_message {
	u8 reportid;
	u8 message[8];
};

/**
 * struct mxt_finger - Represents fingers.
 * @ x: x position.
 * @ y: y position.
 * @ w: size of touch.
 * @ z: touch amplitude(sum of measured deltas).
 * @ component: vector info.
 * @ state: finger status.
 * @ type: type of touch. if firmware can support.
 * @ mcount: moving counter for debug.
 */
struct mxt_finger {
	u16 x;
	u16 y;
	u16 w;
	u16 z;
#if TSP_USE_SHAPETOUCH
	u16 component;
#endif
	u8 state;
	u8 type;
	u8 event;
	u16 mcount;
};

struct mxt_reportid {
	u8 type;
	u8 index;
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

#if TSP_USE_ATMELDBG
struct atmel_dbg {
	u16 last_read_addr;
	u8 stop_sync;		/* to disable input sync report */
	u8 display_log;		/* to display raw message */
	u8 block_access;	/* to prevent access IC with I2c */
};
#endif

#if TSP_SEC_FACTORY
struct mxt_fac_data {
	struct device *fac_dev_ts;
#if ENABLE_TOUCH_KEY
	struct device *touchkey_dev_ts;
#endif
	struct list_head cmd_list_head;
	u8 cmd_state;
	char	cmd[TSP_CMD_STR_LEN];
	int cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex	cmd_lock;
	bool	cmd_is_running;

	u8 num_xnode;
	u8 num_ynode;
	u16 num_nodes;
	u16 *reference;
	s16 *delta;

	u32 ref_max_data;
	u32 ref_min_data;
	s16 delta_max_data;
	u16 delta_max_node;

	u8 fw_ver;
	u8 build_ver;
};
#endif

#if TSP_CHECK_ANTITOUCH
struct mxt_atch {
	u8 enable;
	u8 coin;
	u8 calgood;
	u8 autocal;
	u8 timer;
	u8 timer_id;
	u8 coin_time;
	u8 calgood_time;
	u8 autocal_size;
	u8 autocal_time;
	u8 autocal_distance;
	u8 coin_atch_min;
	u8 coin_atch_ratio;
	u8 big_atch_ratio;
	u8 big_atch_tch_max;
	u8 big_tch_ratio;
	u8 big_tch_atch_min;
};
#endif

struct mxt_data {
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct input_dev *input_dev;
	const struct mxt_platform_data *pdata;
	struct mxt_info info;
	struct mxt_object *objects;
	struct mxt_reportid *reportids;
	struct completion init_done;
	struct mxt_finger fingers[MXT_MAX_FINGER];
	u8 max_reportid;
	u8 finger_mask ;
	bool mxt_enabled;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#if TSP_BOOSTER
	struct touch_booster booster;
#endif
#if TSP_USE_ATMELDBG
	struct atmel_dbg atmeldbg;
#endif
#if TSP_SEC_FACTORY
	struct mxt_fac_data *fdata;
#endif
#if TSP_USE_SHAPETOUCH
	u16 sumsize;
#endif
#if TSP_INFORM_CHARGER
	struct mxt_callbacks callbacks;
	struct delayed_work noti_dwork;
	bool charging_mode;
#endif
#if TSP_HOVER_WORKAROUND
/* TODO HOVER : Current firmware need to current calibration for hover manually
 * I think it is should to move into IC level, and it is completed,
 *  remove below boolean...*/
	bool cur_cal_status;
#endif
#if ENABLE_TOUCH_KEY
	u16 tsp_keystatus;
#endif
#if TSP_CHECK_ANTITOUCH
	struct mxt_atch atch;
#endif
};

/**
 * struct mxt_fw_image - Represents a firmware file.
 * @ magic_code: Identifier of file type.
 * @ hdr_len: Size of file header (struct mxt_fw_image).
 * @ cfg_len: Size of configuration data.
 * @ fw_len: Size of firmware data.
 * @ cfg_crc: CRC of configuration settings.
 * @ fw_ver: Version of firmware.
 * @ build_ver: Build version of firmware.
 * @ data: Configuration data followed by firmware image.
 */
struct mxt_fw_image {
	__le32 magic_code;
	__le32 hdr_len;
	__le32 cfg_len;
	__le32 fw_len;
	__le32 cfg_crc;
	u8 fw_ver;
	u8 build_ver;
	u8 data[0];
} __packed;

/**
 * struct mxt_cfg_data - Represents a configuration data item.
 * @ type: Type of object.
 * @ instance: Instance number of object.
 * @ size: Size of object.
 * @ register_val: Series of register values for object.
 */
struct mxt_cfg_data {
	u8 type;
	u8 instance;
	u8 size;
	u8 register_val[0];
} __packed;

struct mxt_fw_info {
	u8 fw_ver;
	u8 build_ver;
	u32 hdr_len;
	u32 cfg_len;
	u32 fw_len;
	u32 cfg_crc;
	const u8 *cfg_raw_data;	/* start address of configuration data */
	const u8 *fw_raw_data;	/* start address of firmware data */
	struct mxt_data *data;
};

#if TSP_SEC_FACTORY
extern struct class *sec_class;
#endif
#endif
