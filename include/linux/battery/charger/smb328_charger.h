/*
 * smb328_charger.h
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

#ifndef __SMB328_CHARGER_H
#define __SMB328_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR (0x69>>1)

/* Register define */
#define SMB328A_INPUT_AND_CHARGE_CURRENTS	0x00
#define	SMB328A_CURRENT_TERMINATION			0x01
#define SMB328A_FLOAT_VOLTAGE				0x02
#define SMB328A_FUNCTION_CONTROL_A1			0x03
#define SMB328A_FUNCTION_CONTROL_A2			0x04
#define SMB328A_FUNCTION_CONTROL_B			0x05
#define SMB328A_OTG_PWR_AND_LDO_CONTROL		0x06
#define SMB328A_VARIOUS_CONTROL_FUNCTION_A	0x07
#define SMB328A_CELL_TEMPERATURE_MONITOR	0x08
#define SMB328A_INTERRUPT_SIGNAL_SELECTION	0x09
#define SMB328A_I2C_BUS_SLAVE_ADDRESS		0x0A

#define SMB328A_CLEAR_IRQ					0x30
#define SMB328A_COMMAND						0x31
#define SMB328A_INTERRUPT_STATUS_A			0x32
#define SMB328A_BATTERY_CHARGING_STATUS_A	0x33
#define SMB328A_INTERRUPT_STATUS_B			0x34
#define SMB328A_BATTERY_CHARGING_STATUS_B	0x35
#define SMB328A_BATTERY_CHARGING_STATUS_C	0x36
#define SMB328A_INTERRUPT_STATUS_C			0x37
#define SMB328A_BATTERY_CHARGING_STATUS_D	0x38
#define SMB328A_AUTOMATIC_INPUT_CURRENT_LIMMIT_STATUS	0x39

#endif /* __SMB328_CHARGER_H */
