/*
 *  dummy_charger.c
 *  Samsung Dummy Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/battery/sec_charger.h>

static int dummy_i2c_write(struct i2c_client *client, int reg, u8 *buf)
{
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);

	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

static int dummy_i2c_read(struct i2c_client *client, int reg, u8 *buf)
{
	int ret;
 
	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
 
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
 
	return ret;
}

static void dummy_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;

	for (i = 0; i < size; i += 3)
		dummy_i2c_write(client, (u8)(*(buf + i)),
			(buf + i) + 1);
}
 
bool sec_hal_chg_init(struct i2c_client *client)
{
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
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
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
 
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		break;
	default:
		return false;
	}
	return true;
}



