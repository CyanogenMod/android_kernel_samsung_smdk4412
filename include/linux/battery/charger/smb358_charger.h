/*
 * smb358_charger.h
 * Samsung SMB358 Charger Header
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

#ifndef __SMB358_CHARGER_H
#define __SMB358_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR	(0xD4 >> 1)

/* Register define */
#define SMB358_CHARGE_CURRENT				0x00
#define SMB358_INPUT_CURRENTLIMIT			0x01
#define SMB358_VARIOUS_FUNCTIONS			0x02
#define SMB358_FLOAT_VOLTAGE				0x03
#define SMB358_CHARGE_CONTROL				0x04
#define SMB358_STAT_TIMERS_CONTROL			0x05
#define SMB358_PIN_ENABLE_CONTROL			0x06
#define SMB358_THERM_CONTROL_A				0x07
#define SMB358_SYSOK_USB30_SELECTION			0x08
#define SMB358_OTHER_CONTROL_A				0x09
#define SMB358_OTG_TLIM_THERM_CONTROL			0x0A
#define SMB358_LIMIT_CELL_TEMPERATURE_MONITOR		0x0B
#define SMB358_FAULT_INTERRUPT				0x0C
#define SMB358_STATUS_INTERRUPT				0x0D
#define SMB358_I2C_BUS_SLAVE_ADDR			0x0E

#define SMB358_COMMAND_A				0x30
#define SMB358_COMMAND_B				0x31
#define SMB358_COMMAND_C				0x33
#define SMB358_ADC_STATUS				0x34
#define SMB358_INTERRUPT_STATUS_A			0x35
#define SMB358_INTERRUPT_STATUS_B			0x36
#define SMB358_INTERRUPT_STATUS_C			0x37
#define SMB358_INTERRUPT_STATUS_D			0x38
#define SMB358_INTERRUPT_STATUS_E			0x39
#define SMB358_INTERRUPT_STATUS_F			0x3A
#define SMB358_STATUS_A					0x3B
#define SMB358_STATUS_B					0x3C
#define SMB358_STATUS_C					0x3D
#define SMB358_STATUS_D					0x3E
#define SMB358_STATUS_E					0x3F

#define ENABLE_WRT_ACCESS				0x80
#define SMB358_ENABLE_WRITE				1
#define SMB358_DISABLE_WRITE				0

#define SMB358_ENABLE_CHARGER				0x02
#endif	/* __SMB358_CHARGER_H */
