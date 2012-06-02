/*
 *  smb136_charger.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/power/smb136_charger_q1.h>
#define DEBUG

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_MISC,
};

static int smb136_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret = 0;

	if (!client)
		return -ENODEV;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	*data = ret & 0xff;
	return 0;
}

static int smb136_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	if (!client)
		return -ENODEV;

	return i2c_smbus_write_byte_data(client, reg, data);
}

static void smb136_test_read(struct i2c_client *client)
{
	struct smb136_chip *chg = i2c_get_clientdata(client);
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr < 0x0c; addr++) {
		smb136_i2c_read(chg->client, addr, &data);
		dev_info(&client->dev,
			"SMB136 addr : 0x%02x data : 0x%02x\n", addr, data);
	}

	for (addr = 0x31; addr < 0x3D; addr++) {
		smb136_i2c_read(chg->client, addr, &data);
		dev_info(&client->dev,
			"SMB136 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static int smb136_get_charging_status(struct i2c_client *client)
{
	struct smb136_chip *chg = i2c_get_clientdata(client);
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data1 = 0;
	u8 data2 = 0;

	smb136_i2c_read(chg->client, 0x36, &data1);
	smb136_i2c_read(chg->client, 0x39, &data2);
	dev_info(&client->dev, "%s : 0x36h(0x%02x), 0x39h(0x%02x)\n",
		__func__, data1, data2);

	if (data2 & 0x01)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else {
		if ((data1 & 0x08) == 0x08) {
			/* if error bit check,
				ignore the status of charger-ic */
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else if ((data1 & 0xc0) == 0xc0) {
			/* At least one charge cycle terminated,
				Charge current < Termination Current */
			status = POWER_SUPPLY_STATUS_FULL;
		}
	}

	return (int)status;
}

static int smb136_charging(struct i2c_client *client)
{
	struct smb136_chip *chg = i2c_get_clientdata(client);
	u8 data = 0;
	int gpio = 0;

	dev_info(&client->dev, "%s : enable(%d), cable(%d)\n",
		__func__, chg->is_enable, chg->cable_type);

	if (chg->is_enable) {
		switch (chg->cable_type) {
		case CABLE_TYPE_AC:
			/* 1. HC mode */
			data = 0x8c;

			smb136_i2c_write(chg->client, SMB_CommandA, data);
			udelay(10);

			/* 2. Change USB5/1/HC Control from Pin to I2C */
			/* 2. EN pin control - active low */
			smb136_i2c_write(chg->client, SMB_PinControl, 0x8);
			udelay(10);

			smb136_i2c_write(chg->client, SMB_CommandA, 0x8c);
			udelay(10);

			/* 3. Set charge current to 950mA,
				termination current to 150mA */
			data = 0x94;

			smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
			udelay(10);
			break;
		case CABLE_TYPE_USB:
		default:
			/* 1. USBIN 500mA mode */
			data = 0x88;

			smb136_i2c_write(chg->client, SMB_CommandA, data);
			udelay(10);

			/* 2. Change USB5/1/HC Control from Pin to I2C */
			/* 2. EN pin control - active low */
			smb136_i2c_write(chg->client, SMB_PinControl, 0x8);
			udelay(10);

			smb136_i2c_write(chg->client, SMB_CommandA, 0x88);
			udelay(10);

			/* 3. Set charge current to 500mA,
				termination current to 150mA */
			data = 0x14;

			smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
			udelay(10);
			break;
		}

		/* 3. Enable Automatic Input Current Limit to 1000mA
			(threshold 4.25V) */
		data = 0x60;
		smb136_i2c_write(chg->client, SMB_InputCurrentLimit, data);
		udelay(10);

		/* 4. Automatic Recharge Disabed */
		data = 0x8c;
		smb136_i2c_write(chg->client, SMB_ControlA, data);
		udelay(10);

		/* 5. Safety timer Disabled */
		data = 0x28;
		smb136_i2c_write(chg->client, SMB_ControlB, data);
		udelay(10);

		/* 6. Disable USB D+/D- Detection */
		data = 0x28;
		smb136_i2c_write(chg->client, SMB_OTGControl, data);
		udelay(10);

		/* 7. Set Output Polarity for STAT */
		/* 7. Set float voltage to 4.2V */
		data = 0x4a;
		smb136_i2c_write(chg->client, SMB_FloatVoltage, data);
		udelay(10);

		/* 8. Re-load Enable */
		data = 0x4b;
		smb136_i2c_write(chg->client, SMB_SafetyTimer, data);
		udelay(10);
	} else {
		/* do nothing... */
	}

	/* CHG_EN pin control - active low */
	gpio = gpio_request(chg->pdata->gpio_chg_en, "CHG_EN");
	if (!gpio) {
		gpio_direction_output(chg->pdata->gpio_chg_en,
			!(chg->is_enable));
		dev_info(&client->dev, "gpio(CHG_EN)is %d\n",
			gpio_get_value(chg->pdata->gpio_chg_en));
		gpio_free(chg->pdata->gpio_chg_en);
	} else
		dev_err(&client->dev,
			"faile to request gpio(CHG_EN)\n");

	/* smb136_test_read(client); */

	return 0;
}

static int smb136_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct smb136_chip *chip = container_of(psy,
						  struct smb136_chip,
						  charger);
	u8 data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = smb136_get_charging_status(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = chip->cable_type;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->is_enable;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		smb136_i2c_read(chip->client, SMB_ChargeCurrent, &data);
		switch (data >> 5) {
		case 0:
			val->intval = 500;
			break;
		case 1:
			val->intval = 650;
			break;
		case 2:
			val->intval = 750;
			break;
		case 3:
			val->intval = 850;
			break;
		case 4:
			val->intval = 950;
			break;
		case 5:
			val->intval = 1100;
			break;
		case 6:
			val->intval = 1300;
			break;
		case 7:
			val->intval = 1500;
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	dev_info(&chip->client->dev, "%s: smb136_get_property (%d,%d)\n",
		__func__, psp, val->intval);

	return 0;
}

static int smb136_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct smb136_chip *chip = container_of(psy,
						  struct smb136_chip,
						  charger);

	dev_info(&chip->client->dev, "%s: smb136_set_property (%d,%d)\n",
		__func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		chip->is_enable = (val->intval == POWER_SUPPLY_STATUS_CHARGING);
		smb136_charging(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (val->intval) {
		case CABLE_TYPE_USB:
		case CABLE_TYPE_AC:
			chip->cable_type = val->intval;
			break;
		default:
			dev_err(&chip->client->dev, "cable type NOT supported!\n");
			chip->cable_type = CABLE_TYPE_NONE;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		chip->is_enable = (bool)val->intval;
		smb136_charging(chip->client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (val->intval <= 450)
			chip->cable_type = CABLE_TYPE_USB;	/* USB */
		else
			chip->cable_type = CABLE_TYPE_AC;	/* TA */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
static bool is_ovp_status;
#endif

static irqreturn_t smb136_irq_thread(int irq, void *data)
{
	struct smb136_chip *chip = data;
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
	int ret = 0;
	u8 data1 = 0;
#endif

	dev_info(&chip->client->dev, "%s\n", __func__);

	smb136_test_read(chip->client);
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
	smb136_i2c_read(chip->client, 0x33, &data1);

	if (data1 & 0x02) {
		if (is_ovp_status == false) {
			is_ovp_status = true;
			if (chip->pdata->ovp_cb)
				ret = chip->pdata->ovp_cb(true);
			dev_info(&chip->client->dev, "$s OVP!!\n");
		}
	} else {
		if (is_ovp_status == true) {
			is_ovp_status = false;
			if (chip->pdata->ovp_cb)
				ret = chip->pdata->ovp_cb(false);
			dev_info(&chip->client->dev,
				"$s ovp status released!!\n");
		}
	}
#endif

	return IRQ_HANDLED;
}

static int smb136_irq_init(struct smb136_chip *chip)
{
	struct i2c_client *client = chip->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
			smb136_irq_thread, IRQ_TYPE_EDGE_BOTH,
#else
			smb136_irq_thread, IRQ_TYPE_EDGE_FALLING,
#endif
			"SMB136 charger", chip);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(client->irq);
		if (ret < 0)
			dev_err(&client->dev,
				"failed to enable wakeup src %d\n", ret);
	}

	return 0;
}

static int smb136_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb136_chip *chip;
	int ret = 0;
	int gpio = 0;
	u8 data;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	if (smb136_i2c_read(client, 0x36, &data) < 0)	/* check HW */
		return -EIO;

	dev_info(&client->dev,
		"%s : SMB136 Charger Driver Loading\n", __func__);

	chip = kzalloc(sizeof(struct smb136_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	if (!chip->pdata) {
		dev_err(&client->dev,
			"%s : No platform data supplied\n", __func__);
		ret = -EINVAL;
		goto err_pdata;
	}

	if (chip->pdata->set_charger_name)
		chip->pdata->set_charger_name();

	chip->is_enable = false;
	chip->cable_type = CABLE_TYPE_NONE;

	chip->charger.name		= "smb136-charger";
	chip->charger.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->charger.get_property	= smb136_get_property;
	chip->charger.set_property	= smb136_set_property;
	chip->charger.properties	= smb136_charger_props;
	chip->charger.num_properties	= ARRAY_SIZE(smb136_charger_props);

	ret = power_supply_register(&client->dev, &chip->charger);
	if (ret) {
		dev_err(&client->dev,
			"failed: power supply register\n");
		kfree(chip);
		return ret;
	}

	/* CHG_EN pin control - active low */
	if (chip->pdata->gpio_chg_en) {
		s3c_gpio_cfgpin(chip->pdata->gpio_chg_en, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(chip->pdata->gpio_chg_en, S3C_GPIO_PULL_NONE);

		gpio = gpio_request(chip->pdata->gpio_chg_en, "CHG_EN");
		if (!gpio) {
			gpio_direction_output(chip->pdata->gpio_chg_en,
				GPIO_LEVEL_HIGH);
			gpio_free(chip->pdata->gpio_chg_en);
		} else
			dev_err(&client->dev,
			"faile to request gpio(CHG_EN)\n");
	}

	if (chip->pdata->gpio_otg_en) {
		s3c_gpio_cfgpin(chip->pdata->gpio_otg_en, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(chip->pdata->gpio_otg_en, S3C_GPIO_PULL_NONE);

		gpio = gpio_request(chip->pdata->gpio_otg_en, "OTG_EN");
		if (!gpio) {
			gpio_direction_output(chip->pdata->gpio_otg_en,
				GPIO_LEVEL_LOW);
			gpio_free(chip->pdata->gpio_otg_en);
		} else
			dev_err(&client->dev,
			"faile to request gpio(OTG_EN)\n");
	}

	if (chip->pdata->gpio_ta_nconnected) {
		s3c_gpio_cfgpin(chip->pdata->gpio_ta_nconnected,
			S3C_GPIO_INPUT);
		s3c_gpio_setpull(chip->pdata->gpio_ta_nconnected,
			S3C_GPIO_PULL_NONE);
	}

	if (chip->pdata->gpio_chg_ing) {
#if 1
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
		s3c_gpio_cfgpin(chip->pdata->gpio_chg_ing, S3C_GPIO_SFN(0xf));
#endif
		client->irq = gpio_to_irq(chip->pdata->gpio_chg_ing);
		ret = smb136_irq_init(chip);
		if (ret)
			goto err_pdata;
#else
		s3c_gpio_cfgpin(chip->pdata->gpio_chg_ing, S3C_GPIO_INPUT);
		s3c_gpio_setpull(chip->pdata->gpio_chg_ing, S3C_GPIO_PULL_NONE);
#endif
	}

#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB136_CHARGER_Q1)
	is_ovp_status = false;
#endif

	smb136_test_read(client);

	return 0;

err_pdata:
	kfree(chip);
	return ret;
}

static int __devexit smb136_remove(struct i2c_client *client)
{
	struct smb136_chip *chip = i2c_get_clientdata(client);

	kfree(chip);
	return 0;
}

static const struct i2c_device_id smb136_id[] = {
	{"smb136-charger", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, smb136_id);

static struct i2c_driver smb136_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "smb136-charger",
	},
	.probe	= smb136_probe,
	.remove	= __devexit_p(smb136_remove),
	.command = NULL,
	.id_table	= smb136_id,
};

static int __init smb136_init(void)
{
	return i2c_add_driver(&smb136_i2c_driver);
}

static void __exit smb136_exit(void)
{
	i2c_del_driver(&smb136_i2c_driver);
}

module_init(smb136_init);
module_exit(smb136_exit);

MODULE_AUTHOR("Ikkeun Kim <iks.kim@samsung.com>");
MODULE_DESCRIPTION("smb136 charger driver");
MODULE_LICENSE("GPL");
