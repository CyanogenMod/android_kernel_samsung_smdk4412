/*
 * ncp1851_charger.h
 * Samsung SMB136 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __NCP1851_CHARGER_H
#define __NCP1851_CHARGER_H __FILE__

#define SEC_CHARGER_I2C_SLAVEADDR       (0x6C >> 1)

#define NCP1851_STATUS		0x0
#define NCP1851_CTRL1		0x1
#define NCP1851_CTRL2		0x2
#define NCP1851_STAT_INT	0x3
#define NCP1851_CH1_INT		0x4
#define NCP1851_CH2_INT		0x5
#define NCP1851_BST_INT		0x6
#define NCP1851_VIN_SNS		0x7
#define NCP1851_VBAT_SNS	0x8
#define NCP1851_TEMP_SNS	0x9
#define NCP1851_STAT_MSK	0xA
#define NCP1851_CH1_MSK		0xB
#define NCP1851_CH2_MSK		0xC
#define NCP1851_BST_MSK		0xD
#define NCP1851_VBAT_SET	0xE
#define NCP1851_IBAT_SET	0xF
#define NCP1851_MISC_SET	0x10
#define NCP1851_NTC_SET1	0x11
#define NCP1851_NTC_SET2	0x12

static int ncp1851_get_charging_status(struct i2c_client *client);
static void ncp1851_charger_function_control(struct i2c_client *client);
#endif
