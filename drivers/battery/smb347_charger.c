/*
 *  smb347_charger.c
 *  Samsung SMB347 Charger Driver
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
static int smb347_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int smb347_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static void smb347_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;
	for (i = 0; i < size; i += 3)
		smb347_i2c_write(client, (u8) (*(buf + i)), (buf + i) + 1);
}

static void smb347_set_command(struct i2c_client *client,
				int reg, int datum)
{
	int val;
	u8 data = 0;
	val = smb347_i2c_read(client, reg, &data);
	if (val >= 0) {
		dev_dbg(&client->dev, "%s : reg(0x%02x): 0x%02x",
			__func__, reg, data);
		if (data != datum) {
			data = datum;
			if (smb347_i2c_write(client, reg, &data) < 0)
				dev_err(&client->dev,
					"%s : error!\n", __func__);
			val = smb347_i2c_read(client, reg, &data);
			if (val >= 0)
				dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static void smb347_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;
	for (addr = 0; addr <= 0x0f; addr++) {
		smb347_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"smb347 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb347_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"smb347 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static void smb347_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x0f; addr++) {
		smb347_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb347_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}


static int smb347_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;

	smb347_i2c_read(client, SMB347_STATUS_A, &data_a);
	dev_info(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb347_i2c_read(client, SMB347_STATUS_B, &data_b);
	dev_info(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb347_i2c_read(client, SMB347_STATUS_C, &data_c);
	dev_info(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb347_i2c_read(client, SMB347_STATUS_D, &data_d);
	dev_info(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb347_i2c_read(client, SMB347_STATUS_E, &data_e);
	dev_info(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);

	/* At least one charge cycle terminated,
	 * Charge current < Termination Current
	 */
	if ((data_c & 0x20) == 0x20) {
		/* top-off by full charging */
		status = POWER_SUPPLY_STATUS_FULL;
		goto charging_status_end;
	}

	/* Is enabled ? */
	if (data_c & 0x01) {
		/* check for 0x06 : no charging (0b00) */
		/* not charging */
		if (!(data_c & 0x06)) {
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			goto charging_status_end;
		} else {
			status = POWER_SUPPLY_STATUS_CHARGING;
			goto charging_status_end;
		}
	} else
		status = POWER_SUPPLY_STATUS_DISCHARGING;
charging_status_end:
	return (int)status;
}

static int smb347_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;

	smb347_i2c_read(client, SMB347_STATUS_A, &data_a);
	dev_info(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb347_i2c_read(client, SMB347_STATUS_B, &data_b);
	dev_info(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb347_i2c_read(client, SMB347_STATUS_C, &data_c);
	dev_info(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb347_i2c_read(client, SMB347_STATUS_D, &data_d);
	dev_info(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb347_i2c_read(client, SMB347_STATUS_E, &data_e);
	dev_info(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);

	/* Is enabled ? */
	/*
	if (data_c & 0x01) {
		if (!(data_a & 0x02))	// Input current is NOT OK //
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}
	*/
	return (int)health;
}

static void smb347_allow_volatile_writes(struct i2c_client *client)
{
	int val, reg;
	u8 data;
	reg = SMB347_COMMAND_A;
	val = smb347_i2c_read(client, reg, &data);
	if ((val >= 0) && !(data & 0x80)) {
		dev_dbg(&client->dev,
			"%s : reg(0x%02x): 0x%02x", __func__, reg, data);
		data |= (0x1 << 7);
		if (smb347_i2c_write(client, reg, &data) < 0)
			dev_err(&client->dev, "%s : error!\n", __func__);
		val = smb347_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8) data;
			dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static u8 smb347_get_float_voltage_data(
			int float_voltage)
{
	u8 data;

	if (float_voltage < 3500)
		float_voltage = 3500;

	data = (float_voltage - 3500) / 20;

	return data;
}

static u8 smb347_get_input_current_limit_data(
			struct sec_charger_info *charger, int input_current)
{
	u8 data;

	if (input_current <= 340)
		data = 0x0;
	else if (input_current <= 500)
		data = 0x1;
	else if (input_current <= 660)
		data = 0x2;
	else if (input_current <= 820)
		data = 0x3;
	else if (input_current <= 1030)
		data = 0x4;
	else if (input_current <= 1300)
		data = 0x5;
	else if (input_current <= 1540)
		data = 0x6;
	else if (input_current <= 1700)
		data = 0x7;
	else if (input_current <= 1860)
		data = 0x8;
	else if (input_current <= 2100)
		data = 0x9;
	else
		data = 0;

	return data;
}

static u8 smb347_get_termination_current_limit_data(
			int termination_current)
{
	u8 data;

	if (termination_current <= 37)
		data = 0x0;
	else if (termination_current <= 50)
		data = 0x1;
	else if (termination_current <= 100)
		data = 0x2;
	else if (termination_current <= 150)
		data = 0x3;
	else if (termination_current <= 200)
		data = 0x4;
	else if (termination_current <= 250)
		data = 0x5;
	else if (termination_current <= 500)
		data = 0x6;
	else if (termination_current <= 600)
		data = 0x7;
	else
		data = 0;

	return data;
}

static u8 smb347_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;

	if (fast_charging_current <= 700)
		data = 0x0;
	else if (fast_charging_current <= 900)
		data = 0x1;
	else if (fast_charging_current <= 1200)
		data = 0x2;
	else if (fast_charging_current <= 1500)
		data = 0x3;
	else if (fast_charging_current <= 1800)
		data = 0x4;
	else if (fast_charging_current <= 2000)
		data = 0x5;
	else if (fast_charging_current <= 2200)
		data = 0x6;
	else if (fast_charging_current <= 2500)
		data = 0x7;
	else
		data = 0;

	return data << 5;
}

static void smb347_charger_function_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	union power_supply_propval val;
	int full_check_type;
	u8 data;

	if (charger->charging_current < 0) {
		dev_dbg(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}
	smb347_allow_volatile_writes(client);

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* turn off charger */
		smb347_set_command(client,
			SMB347_COMMAND_A, 0x80);

		/* high current mode for system current */
		smb347_set_command(client,
			SMB347_COMMAND_B, 0x01);
	} else {
		/* Pre-charge curr 250mA */
		dev_dbg(&client->dev,
			"%s : fast charging current (%dmA)\n",
			__func__, charger->charging_current);
		dev_dbg(&client->dev,
			"%s : termination current (%dmA)\n",
			__func__, charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		data = 0x1c;
		data |= smb347_get_fast_charging_current_data(
			charger->charging_current);
		data |= smb347_get_termination_current_limit_data(
			charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		smb347_set_command(client,
			SMB347_CHARGE_CURRENT, data);

		/* Pin enable control */
		/* DCIN Input Pre-bias Enable */
		data = 0x01;
		if (charger->pdata->chg_gpio_en)
			data |= 0x40;
		if (charger->pdata->chg_polarity_en)
			data |= 0x20;
		smb347_set_command(client,
			SMB347_PIN_ENABLE_CONTROL, data);

		/* Input current limit */
		dev_dbg(&client->dev, "%s : input current (%dmA)\n",
			__func__, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		data = 0;
		data = smb347_get_input_current_limit_data(
			charger,
			charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		smb347_set_command(client,
			SMB347_INPUT_CURRENTLIMIT, data);

		/*
		 * Input to System FET by Register
		 * Enable AICL, VCHG
		 * Max System voltage =Vflt + 0.1v
		 * Input Source Priority : USBIN
		 */
		if (charger->pdata->chg_functions_setting &
			SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT)
			/* disable AICL */
			smb347_set_command(client,
				SMB347_VARIOUS_FUNCTIONS, 0x85);
		else
			/* enable AICL */
			smb347_set_command(client,
				SMB347_VARIOUS_FUNCTIONS, 0x95);

		/* Float voltage, Vprechg : 2.4V */
		dev_dbg(&client->dev, "%s : float voltage (%dmV)\n",
				__func__, charger->pdata->chg_float_voltage);
		data = 0;
		data |= smb347_get_float_voltage_data(
			charger->pdata->chg_float_voltage);
		smb347_set_command(client,
			SMB347_FLOAT_VOLTAGE, data);

		/* Charge control
		 * Automatic Recharge disable,
		 * Current Termination disable,
		 * BMD disable, Recharge Threshold =50mV,
		 * APSD disable */
		data = 0xC0;
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
			/* Enable Current Termination */
			data &= 0xBF;
			break;
		}
		smb347_set_command(client,
			SMB347_CHARGE_CONTROL, data);

		/* STAT, Timer control : STAT active low,
		 * Complete time out 1527min.
		 */
		smb347_set_command(client,
			SMB347_STAT_TIMERS_CONTROL, 0x1A);

		/* Pin/Enable
		 * USB 5/1/HC Dual state
		 * DCIN pre-bias Enable
		 */
		smb347_set_command(client,
			SMB347_PIN_ENABLE_CONTROL, 0x09);

		/* Therm control :
		 * Therm monitor disable,
		 * Minimum System Voltage 3.60V
		 */
		smb347_set_command(client,
			SMB347_THERM_CONTROL_A, 0x7F);

		/* USB selection : USB2.0(100mA/500mA),
		 * INOK polarity Active low
		 */
		smb347_set_command(client,
			SMB347_SYSOK_USB30_SELECTION, 0x08);

		/* Other control
		 * Low batt detection disable
		 * Minimum System Voltage 3.60V
		 */
		smb347_set_command(client,
			SMB347_OTHER_CONTROL_A, 0x00);

		/* OTG tlim therm control */
		smb347_set_command(client,
			SMB347_OTG_TLIM_THERM_CONTROL, 0x3F);

		/* Limit cell temperature */
		smb347_set_command(client,
			SMB347_LIMIT_CELL_TEMPERATURE_MONITOR, 0x01);

		/* Fault interrupt : Clear */
		smb347_set_command(client,
			SMB347_FAULT_INTERRUPT, 0x00);

		/* STATUS ingerrupt : Clear */
		smb347_set_command(client,
			SMB347_STATUS_INTERRUPT, 0x00);

		/* turn on charger */
		smb347_set_command(client,
			SMB347_COMMAND_A, 0x82);

		/* HC or USB5 mode */
		switch (charger->cable_type) {
		case POWER_SUPPLY_TYPE_MAINS:
		case POWER_SUPPLY_TYPE_MISC:
			/* High-current mode */
			data = 0x01;
			break;
		case POWER_SUPPLY_TYPE_USB:
		case POWER_SUPPLY_TYPE_USB_DCP:
		case POWER_SUPPLY_TYPE_USB_CDP:
		case POWER_SUPPLY_TYPE_USB_ACA:
			/* USB5 */
			data = 0x02;
			break;
		default:
			/* USB1 */
			data = 0x00;
			break;
		}
		smb347_set_command(client,
			SMB347_COMMAND_B, data);
	}
}

static void smb347_charger_otg_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	smb347_allow_volatile_writes(client);
	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* turn off charger */
		smb347_set_command(client,
			SMB347_COMMAND_A, 0x80);
	} else {
		/* turn on OTG */
		smb347_set_command(client,
			SMB347_COMMAND_A, (0x1 << 4));
	}
}

static void smb347_set_charging_current(
		struct i2c_client *client, int charging_current)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	smb347_i2c_read(client, SMB347_CHARGE_CURRENT, &data);
	data &= 0xe0;
	data |= smb347_get_fast_charging_current_data(charging_current);
	smb347_set_command(client, SMB347_CHARGE_CURRENT, data);
}

static void smb347_set_charging_input_current_limit(
		struct i2c_client *client, int input_current_limit)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	/* Input current limit */
	data = 0;
	data = smb347_get_input_current_limit_data(
		charger, input_current_limit);
	smb347_set_command(client, SMB347_INPUT_CURRENTLIMIT, data);
}

bool sec_hal_chg_init(struct i2c_client *client)
{
	smb347_test_read(client);
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
		val->intval = smb347_get_charging_status(client);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = smb347_get_charging_health(client);
		break;
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
		if (charger->charging_current) {
			smb347_i2c_read(client, SMB347_STATUS_B, &data);
			if (data & 0x20)
				switch (data & 0x18) {
				case 0:
					val->intval = 100;
					break;
				case 1:
					val->intval = 150;
					break;
				case 2:
					val->intval = 200;
					break;
				case 3:
					val->intval = 250;
					break;
				}
			else
				switch (data & 0x07) {
				case 0:
					val->intval = 700;
					break;
				case 1:
					val->intval = 900;
					break;
				case 2:
					val->intval = 1200;
					break;
				case 3:
					val->intval = 1500;
					break;
				case 4:
					val->intval = 1800;
					break;
				case 5:
					val->intval = 2000;
					break;
				case 6:
					val->intval = 2200;
					break;
				case 7:
					val->intval = 2500;
					break;
				}
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

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		if (charger->charging_current < 0)
			smb347_charger_otg_control(client);
		else if (charger->charging_current > 0)
			smb347_charger_function_control(client);
		else {
			smb347_charger_function_control(client);
			smb347_charger_otg_control(client);
		}
		smb347_test_read(client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		smb347_set_charging_input_current_limit(client, val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		smb347_set_charging_current(client, val->intval);
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

		smb347_read_regs(chg->client, str);
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
			smb347_i2c_read(chg->client,
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
			smb347_i2c_write(chg->client,
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
