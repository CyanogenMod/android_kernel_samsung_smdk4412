/*
 * bq24190_charger.h
 * Samsung SMB328 Charger Header
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

#ifndef __BQ24190_CHARGER_H
#define __BQ24190_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#if defined(CONFIG_CHARGER_BQ24191)
#define SEC_CHARGER_I2C_SLAVEADDR	0x6a
#else
/* bq24190, bq24192 */
#define SEC_CHARGER_I2C_SLAVEADDR	0x6b
#endif

#define BQ24190_CHARGING_ENABLE			0x20
#define BQ24190_CHARGING_DONE			0x30

/* BQ2419x Registers */

/* Input Source Control */
#define BQ24190_REG_INSRC	0x00
/* Power-On Configuration */
#define BQ24190_REG_PWRON_CFG	0x01
/* Charge Current Control */
#define BQ24190_REG_CHRG_C	0x02
/* Pre-charge/Termination Current Control */
#define BQ24190_REG_PCHRG_TRM_C	0x03
/* Charge Voltage Control */
#define BQ24190_REG_CHRG_V	0x04
/* Charge Termination/Timer Control */
#define BQ24190_REG_CHRG_TRM_TMR	0x05
/* IR Compensation / Thermal Regulation Control */
#define BQ24190_REG_IRCMP_TREG	0x06
/* Misc Operation Control */
#define BQ24190_REG_MISC_OP	0x07
/* System Status */
#define BQ24190_REG_STATUS	0x08
/* Fault */
#define BQ24190_REG_FAULT	0x09
/* Vendor / Part / Revision Status */
#define BQ24190_REG_DEVID	0x0A

#endif /* __BQ24190_CHARGER_H */
