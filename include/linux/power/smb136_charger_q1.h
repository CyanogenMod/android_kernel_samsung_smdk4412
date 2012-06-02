/*
 *  Copyright (C) 2011 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SMB136_CHARGER_H_
#define __SMB136_CHARGER_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <plat/gpio-cfg.h>
#include <linux/power/sec_battery_u1.h>

/* Slave address */
#define SMB136_SLAVE_ADDR		0x9A

/* SMB136 Registers. */
#define SMB_ChargeCurrent		0x00
#define SMB_InputCurrentLimit	0x01
#define SMB_FloatVoltage		0x02
#define SMB_ControlA			0x03
#define SMB_ControlB			0x04
#define SMB_PinControl			0x05
#define SMB_OTGControl			0x06
#define SMB_Fault				0x07
#define SMB_Temperature		0x08
#define SMB_SafetyTimer			0x09
#define SMB_VSYS				0x0A
#define SMB_I2CAddr			0x0B

#define SMB_IRQreset			0x30
#define SMB_CommandA			0x31
#define SMB_StatusA			0x32
#define SMB_StatusB			0x33
#define SMB_StatusC			0x34
#define SMB_StatusD			0x35
#define SMB_StatusE			0x36
#define SMB_StatusF			0x37
#define SMB_StatusG			0x38
#define SMB_StatusH			0x39
#define SMB_DeviceID			0x3B
#define SMB_CommandB			0x3C

/* SMB_StatusC register bit. */
#define SMB_USB				1
#define SMB_CHARGER			0
#define Compelete				1
#define Busy					0
#define InputCurrent275			0xE
#define InputCurrent500			0xF
#define InputCurrent700			0x0
#define InputCurrent800			0x1
#define InputCurrent900			0x2
#define InputCurrent1000		0x3
#define InputCurrent1100		0x4
#define InputCurrent1200		0x5
#define InputCurrent1300		0x6
#define InputCurrent1400		0x7

static enum power_supply_property smb136_charger_props[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

struct smb136_platform_data {
	int	(*topoff_cb)(void);
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER)
	int (*ovp_cb)(bool);
#endif
	void (*set_charger_name)(void);
	int	gpio_chg_en;
	int	gpio_otg_en;
	int	gpio_chg_ing;
	int	gpio_ta_nconnected;
};

struct smb136_chip {
	struct i2c_client *client;
	struct power_supply charger;
	struct smb136_platform_data *pdata;

	bool is_enable;
	int cable_type;
};

#endif
