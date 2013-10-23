/*
 *  drivers/input/touchscreen/synaptics_sysfs.h
 *
 *  Copyright (c) 2010 Samsung Electronics Co., LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _SEC_TSP_SYSFS_H
#define _SEC_TSP_SYSFS_H

#include <linux/wakelock.h>

#define SYNAPTICS_FW "/sdcard/firmware/synaptics_fw"
#define SYNAPTICS_FW2 "/sdcard/synaptics_fw.img"
#define FULL_RAW_CAP_LOWER_LIMIT	1000
#define FULL_RAW_CAP_UPPER_LIMIT	3000
#define MAX_RX_SIZE		45
#define MAX_TX_SIZE		29
#define NOISEMITIGATION	0xb1
#define ABS_POS_BIT		(0x1 << 3)

enum REPORT_TYPE {
	REPORT_TYPE_RESERVED = 1,
	REPORT_TYPE_DELTA_CAP,
	REPORT_TYPE_RAW_CAP,
	REPORT_TYPE_HIGH_REG,
	REPORT_TYPE_TX_TO_TX,	/* 5 */
	REPORT_TYPE_RX_TO_RX = 7,
	REPORT_TYPE_TRUE_BASELINE = 9,
	REPORT_TYPE_RAW_CAP2 = 13,
	REPORT_TYPE_RX_OPEN,
	REPORT_TYPE_TX_OPEN,		/* 15 */
	REPORT_TYPE_TX_TO_GND,
	REPORT_TYPE_RX_TO_RX2,
	REPORT_TYPE_MAX,
};

enum CMD_STATUS {
	CMD_STATUS_RESERVED = 0,
	CMD_STATUS_WAITING,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,	/* 5 */
};

enum CMD_FW_CMD {
	CMD_FW_CMD_BUILT_IN = 0,
	CMD_FW_CMD_UMS,
};

enum CMD_LIST {
	CMD_LIST_FW_UPDATE = 0,
	CMD_LIST_FW_VER_BIN,
	CMD_LIST_FW_VER_IC,
	CMD_LIST_CONFIG_VER,
	CMD_LIST_GET_THRESHOLD,
	CMD_LIST_POWER_OFF,
	CMD_LIST_POWER_ON,
	CMD_LIST_VENDOR,
	CMD_LIST_IC_NAME,
	CMD_LIST_X_SIZE,
	CMD_LIST_Y_SIZE,
	CMD_LIST_READ_REF,
	CMD_LIST_READ_RX,
	CMD_LIST_READ_TX,
	CMD_LIST_READ_TXG,
	CMD_LIST_GET_REF,
	CMD_LIST_GET_RX,
	CMD_LIST_GET_TX,
	CMD_LIST_GET_TXG,
	CMD_LIST_MAX,
};

#endif  /* _SEC_TSP_SYSFS_H */

