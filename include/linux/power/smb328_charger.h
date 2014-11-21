/*
 *  Copyright (C) 2011 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SMB328_CHARGER_H_
#define __SMB328_CHARGER_H_

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
#define SMB328_SLAVE_ADDR		0x69

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

/* fast charging current defines */
#define FAST_500mA		500
#define FAST_600mA		600
#define FAST_700mA		700
#define FAST_800mA		800
#define FAST_900mA		900
#define FAST_1000mA		1000
#define FAST_1100mA		1100
#define FAST_1200mA		1200

/* input current limit defines */
#define ICL_275mA		275
#define ICL_450mA		450
#define ICL_600mA		600
#define ICL_700mA		700
#define ICL_800mA		800
#define ICL_900mA		900
#define ICL_1000mA		1000
#define ICL_1100mA		1100
#define ICL_1200mA		1200

static enum power_supply_property smb328_charger_props[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

struct smb328_platform_data {
	int	(*topoff_cb)(void);
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
	int (*ovp_cb)(bool);
#endif
	void	(*set_charger_name)(void);
	int	gpio_chg_en;
	int	gpio_otg_en;
	int	gpio_chg_ing;
	int	gpio_ta_nconnected;
};

struct smb328_chip {
	struct i2c_client *client;
	struct power_supply charger;
	struct smb328_platform_data *pdata;

	bool is_otg;
	bool is_enable;
	int cable_type;
};

#endif
