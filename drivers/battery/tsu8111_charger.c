/*
 *  tsu8111_charger.c
 *  Samsung TSU8111 Charger Driver
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
static int tsu8111_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int tsu8111_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static void tsu8111_set_command(struct i2c_client *client,
				int reg, int datum)
{
	int val;
	u8 data = 0;
	val = tsu8111_i2c_read(client, reg, &data);
	if (val >= 0) {
		dev_dbg(&client->dev, "%s : reg(0x%02x): 0x%02x",
			__func__, reg, data);
		if (data != datum) {
			data = datum;
			if (tsu8111_i2c_write(client, reg, &data) < 0)
				dev_err(&client->dev,
					"%s : error!\n", __func__);
			val = tsu8111_i2c_read(client, reg, &data);
			if (val >= 0)
				dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static void tsu8111_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0x01; addr <= 0x0d; addr++) {
		tsu8111_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}

	tsu8111_i2c_read(client, TSU8111_MANUALSW1, &data);
	sprintf(str+strlen(str), "0x%x, ", data);
	tsu8111_i2c_read(client, TSU8111_MANUALSW2, &data);
	sprintf(str+strlen(str), "0x%x, ", data);
	tsu8111_i2c_read(client, TSU8111_DEVICE_TYPE3, &data);
	sprintf(str+strlen(str), "0x%x, ", data);
	tsu8111_i2c_read(client, TSU8111_RESET, &data);
	sprintf(str+strlen(str), "0x%x, ", data);

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x20; addr <= 0x26; addr++) {
		tsu8111_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static int tsu8111_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data = 0;

	tsu8111_i2c_read(client, TSU8111_CHARGER_STATUS, &data);
	dev_info(&client->dev,
		"%s : charger status (0x%02x)\n", __func__, data);

	if (data & CH_IDLE)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if ((data & CH_PC) || (data & CH_FC) || (data & CH_CV))
		status = POWER_SUPPLY_STATUS_CHARGING;
	else if (data & CH_DONE)
		status = POWER_SUPPLY_STATUS_FULL;
	else if ((data & CH_PTE) || (data & CH_FTE))
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

	return (int)status;
}

static int tsu8111_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;

	return (int)health;
}

static u8 tsu8111_get_float_voltage_data(
			int float_voltage)
{
	u8 data;
	data = 0;

	if (float_voltage < 4000)
		float_voltage = 4000;

	if (float_voltage > 4350)
		float_voltage = 4350;

	data = (float_voltage - 4000) / 20;

	return data;
}

static u8 tsu8111_get_term_current_data(
			int termination_current)
{
	u8 data;
	data = 0;

	if (termination_current < 50)
		termination_current = 50;

	if (termination_current > 200)
		termination_current = 200;

	data = (termination_current - 50) / 10;

	return data << 4;
}

static u8 tsu8111_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;
	data = 0;

	if (fast_charging_current >= 200)
		data |= 0x10;	/* enable fast charge current set */
	else
		goto set_low_bit;

	if (fast_charging_current > 950)
		fast_charging_current = 950;

	data |= (fast_charging_current - 200) / 50;

set_low_bit:
	return data;
}

static void tsu8111_set_charging(struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	union power_supply_propval val;
	int full_check_type;
	u8 data;

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* turn off charger */
		tsu8111_i2c_read(client, TSU8111_CHARGER_CONTROL1, &data);
		data |= 0x80;
		tsu8111_set_command(client, TSU8111_CHARGER_CONTROL1, data);
	} else {
		data = 0x7b;
		tsu8111_set_command(client, TSU8111_CHARGER_CONTROL1, data);

		data = 0;
		data |= tsu8111_get_float_voltage_data(
			charger->pdata->chg_float_voltage);

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
			if (val.intval == SEC_BATTERY_CHARGING_1ST)
				data |= tsu8111_get_term_current_data(
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_1st);
			else
				data |= tsu8111_get_term_current_data(
					charger->pdata->charging_current[
					charger->cable_type].
					full_check_current_2nd);
			break;
		}
		tsu8111_set_command(client, TSU8111_CHARGER_CONTROL2, data);

		/* OVP 6.5V, Disable charging shutoff */
		data = 0x40;
		data |= tsu8111_get_fast_charging_current_data(
			charger->charging_current);
		tsu8111_set_command(client, TSU8111_CHARGER_CONTROL3, data);

		/* turn on charger */
		tsu8111_i2c_read(client, TSU8111_CHARGER_CONTROL1, &data);
		data &= 0x7f;
		tsu8111_set_command(client, TSU8111_CHARGER_CONTROL1, data);
	}

	
}

static void tsu8111_set_charging_current(
		struct i2c_client *client, int charging_current)
{
	u8 data;

	/* OVP 6.5V, Disable charging shutoff */
	data = 0x40;
	data |= tsu8111_get_fast_charging_current_data(charging_current);
	tsu8111_set_command(client, TSU8111_CHARGER_CONTROL3, data);
}

bool sec_hal_chg_init(struct i2c_client *client)
{
	u8 data;

	/* Reset interrupt masking */
	tsu8111_set_command(client, TSU8111_CONTROL, 0x1e);

	/* Masking VBUS */
	tsu8111_set_command(client, TSU8111_INTERRUPT_MASK1, 0x40);
	/* Masking connect */
	tsu8111_set_command(client, TSU8111_INTERRUPT_MASK2, 0x20);

	/* Register 25h bit5 should be written high on initialization */
	tsu8111_set_command(client, TSU8111_CHARGER_INTMASK, 0x20);

	/* Reset interrupt result */
	tsu8111_i2c_read(client, TSU8111_INTERRUPT1, &data);
	tsu8111_i2c_read(client, TSU8111_INTERRUPT2, &data);
	return true;
}

bool sec_hal_chg_suspend(struct i2c_client *client)
{
	return true;
}

bool sec_hal_chg_resume(struct i2c_client *client)
{
	return true;
}

bool sec_hal_chg_get_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = tsu8111_get_charging_status(client);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = tsu8111_get_charging_health(client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			tsu8111_i2c_read(client,
				TSU8111_CHARGER_CONTROL3, &data);
			if (data & 0x10) /* enable fast charge current set */
				val->intval = (data & 0x0f) * 50 + 200;
			else 
				val->intval = 90;
		} else
			val->intval = 0;

		dev_dbg(&client->dev,
			"%s : set-current(%dmA), current now(%dmA)\n",
			__func__, charger->charging_current, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

bool sec_hal_chg_set_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		/* Reset interrupt result */
		tsu8111_i2c_read(client, TSU8111_INTERRUPT1, &data);
		tsu8111_i2c_read(client, TSU8111_INTERRUPT2, &data);

		tsu8111_set_charging(client);
		break;
	/* val->intval : input current limit
	 * no input current limit for TSU8111
	 */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		/* no input current limit for TSU8111 */
		charger->charging_current = val->intval;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		tsu8111_set_charging_current(client, val->intval);
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

		tsu8111_read_regs(chg->client, str);
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
			tsu8111_i2c_read(chg->client,
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
			tsu8111_i2c_write(chg->client,
				chg->reg_addr, &data);
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
