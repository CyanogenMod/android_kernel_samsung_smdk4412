/*
 * smb347_charger.h
 * Samsung SMB347 Charger Header
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

#ifndef __SMB347_CHARGER_H
#define __SMB347_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR	(0x0C >> 1)

/* Register define */
#define SMB347_CHARGE_CURRENT			0x00
#define SMB347_INPUT_CURRENTLIMIT		0x01
#define SMB347_VARIOUS_FUNCTIONS		0x02
#define SMB347_FLOAT_VOLTAGE			0x03
#define SMB347_CHARGE_CONTROL			0x04
#define SMB347_STAT_TIMERS_CONTROL		0x05
#define SMB347_PIN_ENABLE_CONTROL		0x06
#define SMB347_THERM_CONTROL_A			0x07
#define SMB347_SYSOK_USB30_SELECTION	0x08
#define SMB347_OTHER_CONTROL_A			0x09
#define SMB347_OTG_TLIM_THERM_CONTROL	0x0A
#define SMB347_LIMIT_CELL_TEMPERATURE_MONITOR	0x0B
#define SMB347_FAULT_INTERRUPT			0x0C
#define SMB347_STATUS_INTERRUPT			0x0D
#define SMB347_I2C_BUS_SLAVE_ADDR		0x0E

#define SMB347_COMMAND_A				0x30
#define SMB347_COMMAND_B				0x31
#define SMB347_COMMAND_C				0x33
#define SMB347_INTERRUPT_STATUS_A		0x35
#define SMB347_INTERRUPT_STATUS_B		0x36
#define SMB347_INTERRUPT_STATUS_C		0x37
#define SMB347_INTERRUPT_STATUS_D		0x38
#define SMB347_INTERRUPT_STATUS_E		0x39
#define SMB347_INTERRUPT_STATUS_F		0x3A
#define SMB347_STATUS_A					0x3B
#define SMB347_STATUS_B					0x3C
#define SMB347_STATUS_C					0x3D
#define SMB347_STATUS_D					0x3E
#define SMB347_STATUS_E					0x3F

#endif /* __SMB347_CHARGER_H */
