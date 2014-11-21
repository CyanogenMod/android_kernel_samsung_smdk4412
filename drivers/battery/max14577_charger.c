/*
 *  max14577_charger.c
 *  Samsung MAX14577 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/battery/sec_charger.h>


static void max14577_set_command(struct sec_charger_info *charger,
				int reg, int datum)
{
	int val;
	u8 data = 0;
	val = max14577_read_reg(charger->client, (u8)reg, &data);
	if (val >= 0) {
		dev_dbg(&charger->client->dev, "%s : reg(0x%02x): 0x%02x",
			__func__, reg, data);
		if (data != datum) {
			data = datum;
			if (max14577_write_reg(charger->client,
				(u8)reg, data) < 0)
				dev_err(&charger->client->dev,
					"%s : error!\n", __func__);
			val = max14577_read_reg(charger->client,
				(u8)reg, &data);
			if (val >= 0)
				dev_dbg(&charger->client->dev,
					" => 0x%02x\n", data);
		}
	}
}

static void max14577_read_regs(struct sec_charger_info *charger, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	max14577_read_reg(charger->client, MAX14577_CHG_REG_STATUS3, &data);
	sprintf(str+strlen(str), "0x%x, ", data);

	for (addr = MAX14577_CHG_REG_CHG_CTRL1;
		addr <= MAX14577_CHG_REG_CHG_CTRL7; addr++) {
		max14577_read_reg(charger->client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static void max14577_test_read_regs(struct sec_charger_info *charger)
{
	u8 data = 0;
	u32 addr = 0;

	max14577_read_reg(charger->client, MAX14577_CHG_REG_STATUS3, &data);
	dev_info(&charger->client->dev, "%s : 0x%02x (0x%02x)\n",
		__func__, MAX14577_CHG_REG_STATUS3, data);

	for (addr = MAX14577_CHG_REG_CHG_CTRL1;
		addr <= MAX14577_CHG_REG_CHG_CTRL7; addr++) {
		max14577_read_reg(charger->client, addr, &data);
		dev_info(&charger->client->dev, "%s : 0x%02x (0x%02x)\n",
			__func__, addr, data);
	}
}

static int max14577_get_charging_status(struct sec_charger_info *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data = 0;

	max14577_read_reg(charger->client, MAX14577_CHG_REG_STATUS3, &data);
	dev_info(&charger->client->dev,
		"%s : charger status (0x%02x)\n", __func__, data);

	if (data & 0x01)
		status = POWER_SUPPLY_STATUS_FULL;
	else if (data & 0x02)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else if (data & 0x04)
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	else
		status = POWER_SUPPLY_STATUS_DISCHARGING;

	return (int)status;
}

static int max14577_get_charging_health(struct sec_charger_info *charger)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data = 0;

	max14577_read_reg(charger->client, MAX14577_CHG_REG_STATUS3, &data);
	dev_info(&charger->client->dev,
		"%s : charger status (0x%02x)\n", __func__, data);

	if (data & 0x04)
		health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	else
		health = POWER_SUPPLY_HEALTH_GOOD;

	return (int)health;
}

static u8 max14577_get_float_voltage_data(
			int float_voltage)
{
	u8 data;
	data = 0;

	if (float_voltage < 4000)
		float_voltage = 4000;

	if (float_voltage > 4350)
		float_voltage = 4350;

	if (float_voltage >= 4000 && float_voltage < 4200)
		data = (float_voltage - 4000) / 20 + 0x1;
	else if (float_voltage == 4200)
		data = 0x0;	/* 4200mV */
	else if (float_voltage > 4200 && float_voltage <= 4280)
		data = (float_voltage - 4220) / 20 + 0xb;
	else
		data = 0xf;	/* 4350mV */

	return data;
}

static u8 max14577_get_term_current_data(
			int termination_current)
{
	u8 data;
	data = 0;

	if (termination_current < 50)
		termination_current = 50;

	if (termination_current > 200)
		termination_current = 200;

	data = (termination_current - 50) / 10;

	return data;
}

static u8 max14577_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;
	data = 0;

	if (fast_charging_current >= 200)
		data |= 0x10;	/* enable fast charge current set */
	else
		goto set_low_bit;	/* 90mA */

	if (fast_charging_current > 950)
		fast_charging_current = 950;

	data |= (fast_charging_current - 200) / 50;

set_low_bit:
	return data;
}

static void max14577_set_charging(struct sec_charger_info *charger)
{
	union power_supply_propval val;
	int full_check_type;
	u8 data;

	if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		/* turn off charger */
		data = 0x80;
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL2, data);
	} else {
		/* Battery Fast-Charge Timer disabled [6:4] = 0b111 */
		data = 0x70;
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL1, data);

		/* Battery-Charger Constant Voltage(CV) Mode, float voltage */
		data = 0;
		data |= max14577_get_float_voltage_data(
			charger->pdata->chg_float_voltage);
		dev_dbg(&charger->client->dev, "%s : float voltage (%dmV)\n",
			__func__, charger->pdata->chg_float_voltage);
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL3, data);

		/* Fast Battery Charge Current */
		data = 0;
		data |= max14577_get_fast_charging_current_data(
			charger->charging_current);
		dev_dbg(&charger->client->dev, "%s : charging current (%dmA)\n",
			__func__, charger->charging_current);
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL4, data);

		/* End-of-Charge Current Setting */
		data = 0;
		psy_do_property("battery", get,
			POWER_SUPPLY_PROP_CHARGE_NOW, val);
		if (val.intval == SEC_BATTERY_CHARGING_1ST)
			full_check_type = charger->pdata->full_check_type;
		else
			full_check_type = charger->pdata->full_check_type_2nd;
		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		case SEC_BATTERY_FULLCHARGED_CHGINT:
		case SEC_BATTERY_FULLCHARGED_CHGPSY:
			if (val.intval == SEC_BATTERY_CHARGING_1ST) {
				data |= max14577_get_term_current_data(
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_1st);
				dev_dbg(&charger->client->dev,
					"%s : term current (%dmA)\n", __func__,
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_1st);
			} else {
				data |= max14577_get_term_current_data(
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_2nd);
				dev_dbg(&charger->client->dev,
					"%s : term current (%dmA)\n", __func__,
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_2nd);
			}
			break;
		}
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL5, data);

		/* Auto Charging Stop disabled [5] = 0 */
		data = 0;
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL6, data);

		/* Overvoltage-Protection Threshold 6.5V [1:0] = 0b10 */
		data = 0x02;
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL7, data);

		/* turn on charger */
		data = 0xc0;
		max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL2, data);
	}
}

static void max14577_set_charging_current(
		struct sec_charger_info *charger, int charging_current)
{
	u8 data;

	/* Fast Battery Charge Current */
	data = 0;
	data |= max14577_get_fast_charging_current_data(charging_current);
	max14577_set_command(charger, MAX14577_CHG_REG_CHG_CTRL4, data);
}

bool sec_hal_chg_init(struct sec_charger_info *charger)
{
	return true;
}

bool sec_hal_chg_suspend(struct sec_charger_info *charger)
{
	return true;
}

bool sec_hal_chg_resume(struct sec_charger_info *charger)
{
	return true;
}

bool sec_hal_chg_get_property(struct sec_charger_info *charger,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	u8 data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = max14577_get_charging_status(charger);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = max14577_get_charging_health(charger);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->cable_type;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			max14577_read_reg(charger->client,
				MAX14577_CHG_REG_CHG_CTRL4, &data);
			if (data & 0x10) /* enable fast charge current set */
				val->intval = (data & 0x0f) * 50 + 200;
			else 
				val->intval = 90;
		} else
			val->intval = 0;

		dev_dbg(&charger->client->dev,
			"%s : set-current(%dmA), current now(%dmA)\n",
			__func__, charger->charging_current, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

bool sec_hal_chg_set_property(struct sec_charger_info *charger,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	u8 data;

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		max14577_set_charging(charger);
		break;
	/* val->intval : input current limit
	 * no input current limit for MAX14577
	 */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		/* no input current limit for MAX14577 */
		charger->charging_current = val->intval;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		max14577_set_charging_current(charger, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

ssize_t sec_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		max14577_read_regs(chg, str);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			str);

		kfree(str);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sec_hal_chg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int ret = 0;
	int x = 0;
	u8 data = 0;

	switch (offset) {
	case CHG_REG:
		if (sscanf(buf, "%x\n", &x) == 1) {
			chg->reg_addr = x;
			max14577_read_reg(chg->client,
				chg->reg_addr, &data);
			chg->reg_data = data;
			dev_dbg(dev, "%s: (read) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, chg->reg_data);
			ret = count;
		}
		break;
	case CHG_DATA:
		if (sscanf(buf, "%x\n", &x) == 1) {
			data = (u8)x;
			dev_dbg(dev, "%s: (write) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, data);
			max14577_write_reg(chg->client,
				chg->reg_addr, data);
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

