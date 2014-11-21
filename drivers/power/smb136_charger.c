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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/power/sec_battery_px.h>
#include <linux/power/smb136_charger.h>
#include <linux/mfd/max8997.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-p2.h>


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

/* SIOP */
#define CHARGING_CURRENT_HIGH_LOW_STANDARD	450

struct smb136_chg_data {
	struct i2c_client *client;
	struct smb_charger_data *pdata;
	struct smb_charger_callbacks *callbacks;
};

static struct smb136_chg_data *smb136_chg;
static int charger_i2c_init;


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

static void smb136_test_read(void)
{
	struct smb136_chg_data *chg = smb136_chg;
	u8 data = 0;
	u32 addr = 0;

	if (!charger_i2c_init) {
		pr_info("%s : smb136 charger IC i2c is not initialized!!\n"
			, __func__);
		return ;
	}

	for (addr = 0; addr < 0x0c; addr++) {
		smb136_i2c_read(chg->client, addr, &data);
		pr_info("SMB136 addr : 0x%02x data : 0x%02x\n", addr, data);
	}

	for (addr = 0x31; addr < 0x3D; addr++) {
		smb136_i2c_read(chg->client, addr, &data);
		pr_info("SMB136 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static int smb136_get_charging_state(void)
{
	struct smb136_chg_data *chg = smb136_chg;
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 command_a_data, statue_e_data, statue_h_data;

	smb136_i2c_read(chg->client, SMB_CommandA, &command_a_data);
	smb136_i2c_read(chg->client, SMB_StatusE, &statue_e_data);
	smb136_i2c_read(chg->client, SMB_StatusH, &statue_h_data);

	if (statue_h_data & 0x01) {
		pr_debug("%s: POWER_SUPPLY_STATUS_CHARGING\n", __func__);
		return POWER_SUPPLY_STATUS_CHARGING;
	} else {
		if (!(statue_e_data & 0x06)) {
			pr_debug("%s: POWER_SUPPLY_STATUS_DISCHARGING\n"
				, __func__);
			return POWER_SUPPLY_STATUS_DISCHARGING;
		} else if ((statue_e_data & 0xc0)) {
			pr_debug("%s: POWER_SUPPLY_STATUS_FULL\n", __func__);
			return POWER_SUPPLY_STATUS_FULL;
		}
	}

	pr_info("%s: POWER_SUPPLY_STATUS_UNKNOWN\n", __func__);
	pr_info("%s: 0x31h(0x%02x), 0x36h(0x%02x), 0x39h(0x%02x)\n",
		__func__, command_a_data, statue_e_data, statue_h_data);
	return (int)status;
}

static void smb136_set_charging_state(int en, int cable_status)
{
	struct smb136_chg_data *chg = smb136_chg;
	u8 data = 0;

	if (!charger_i2c_init) {
		pr_info("%s : smb136 charger IC i2c is not initialized!!\n"
			, __func__);
		return;
	}

	pr_info("%s : enable(%d), cable_status(%d)\n"
		, __func__, en, cable_status);

	if (en) {	/* enable */
		/* 2. Change USB5/1/HC Control from Pin to I2C */
		smb136_i2c_write(chg->client, SMB_PinControl, 0x8);
		udelay(10);

		/* 1. USB 100mA Mode, USB5/1 Current Levels */
		/* Prevent in-rush current */
		data = 0x80;
		smb136_i2c_write(chg->client, SMB_CommandA, data);
		udelay(10);

		/* 3. Set charge current to 100mA */
		/* Prevent in-rush current */
		data = 0x14;
		smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
		udelay(10);

		/* 4. Disable Automatic Input Current Limit */
		data = 0xe6;
		smb136_i2c_write(chg->client, SMB_InputCurrentLimit, data);
		udelay(10);

		/* 4. Automatic Recharge Disabed */
		data = 0x8c;
		smb136_i2c_write(chg->client, SMB_ControlA, data);
		udelay(10);

		/* 5. Safty timer Disabled */
		data = 0x28;
		smb136_i2c_write(chg->client, SMB_ControlB, data);
		udelay(10);

		/* 6. Disable USB D+/D- Detection */
		data = 0x28;
		smb136_i2c_write(chg->client, SMB_OTGControl, data);
		udelay(10);

		/* 7. Set Output Polarity for STAT */
		data = 0xCA;
		smb136_i2c_write(chg->client, SMB_FloatVoltage, data);
		udelay(10);

		/* 9. Re-load Enable */
		data = 0x4b;
		smb136_i2c_write(chg->client, SMB_SafetyTimer, data);
		udelay(10);

		/* Enable charge */
		gpio_set_value(chg->pdata->enable, 0);

		switch (cable_status) {
		case CABLE_TYPE_TA:
			/* HC mode */
			data = 0x8c;
			smb136_i2c_write(chg->client, SMB_CommandA, data);
			udelay(10);

			/* Set charge current to 1500mA */
			data = 0xf4;
			smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
			udelay(10);
			break;
		case CABLE_TYPE_STATION:
			/* HC mode */
			data = 0x8c;
			smb136_i2c_write(chg->client, SMB_CommandA, data);
			udelay(10);

			/* Set charge current to 750mA */
			data = 0x54;
			smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
			udelay(10);

			/* Disable Automatic Input Current Limit,
			  * USBIN Current Limit to 700mA */
			data = 0x06;
			smb136_i2c_write(chg->client,
				SMB_InputCurrentLimit, data);
			udelay(10);
			break;
		case CABLE_TYPE_USB:
		default:
			/* Prevent in-rush current */
			msleep(100);

			/* USBIN 500mA mode */
			data = 0x88;
			smb136_i2c_write(chg->client, SMB_CommandA, data);
			udelay(10);

			/* Set charge current to 500mA */
			data = 0x14;
			smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
			udelay(10);
			break;
		}
	} else {
		gpio_set_value(chg->pdata->enable, 1);

		/* USB 100mA Mode, USB5/1 Current Levels */
		/* Prevent in-rush current */
		data = 0x80;
		smb136_i2c_write(chg->client, SMB_CommandA, data);
		udelay(10);

		/* Set charge current to 100mA */
		/* Prevent in-rush current */
		data = 0x14;
		smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
		udelay(10);
	}

	return;
}

static int smb136_get_charging_current(void)
{
	struct smb136_chg_data *chg = smb136_chg;
	u8 data = 0;
	int get_current = 0;

	smb136_i2c_read(chg->client, SMB_ChargeCurrent, &data);
	switch (data >> 5) {
	case 0:
		get_current = 500;	break;
	case 1:
		get_current = 650;	break;
	case 2:
		get_current = 750;	break;
	case 3:
		get_current = 850;	break;
	case 4:
		get_current = 950;	break;
	case 5:
		get_current = 1100;	break;
	case 6:
		get_current = 1300;	break;
	case 7:
		get_current = 1500;	break;
	default:
		get_current = 500;
		break;
	}
	pr_debug("%s: Get charging current as %dmA.\n", __func__, get_current);
	return get_current;
}

static void smb136_set_charging_current(int set_current)
{
	struct smb136_chg_data *chg = smb136_chg;
	u8 data = 0;

	if (set_current > 450) {
		/* HC mode */
		data = 0x8c;
		smb136_i2c_write(chg->client, SMB_CommandA, data);
		udelay(10);

		/* Set charge current to 1500mA */
		data = 0xf4;
		smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
		udelay(10);
	} else {
		/* USBIN 500mA mode */
		data = 0x88;
		smb136_i2c_write(chg->client, SMB_CommandA, data);
		udelay(10);

		/* Set charge current to 500mA */
		data = 0x14;
		smb136_i2c_write(chg->client, SMB_ChargeCurrent, data);
		udelay(10);
	}
	pr_debug("%s: Set charging current as %dmA.\n", __func__, set_current);
}

static int smb136_i2c_probe
(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb136_chg_data *chg;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	pr_info("%s : SMB136 Charger Driver Loading\n", __func__);

	chg = kzalloc(sizeof(struct smb136_chg_data), GFP_KERNEL);
	if (!chg)
		return -ENOMEM;

	chg->callbacks = kzalloc(sizeof(struct smb_charger_callbacks)
		, GFP_KERNEL);
	if (!chg->callbacks) {
		pr_err("%s : No callbacks\n", __func__);
		ret = -ENOMEM;
		goto err_callbacks;
	}

	chg->client = client;
	if (!chg->client) {
		pr_err("%s : No client\n", __func__);
		ret = -EINVAL;
		goto err_client;
	}

	chg->pdata = client->dev.platform_data;
	if (!chg->pdata) {
		pr_err("%s : No platform data supplied\n", __func__);
		ret = -EINVAL;
		goto err_pdata;
	}

	i2c_set_clientdata(client, chg);
	smb136_chg = chg;
	pr_info("register callback functions!\n");

	chg->callbacks->set_charging_state = smb136_set_charging_state;
	chg->callbacks->get_charging_state = smb136_get_charging_state;
	chg->callbacks->set_charging_current = smb136_set_charging_current;
	chg->callbacks->get_charging_current = smb136_get_charging_current;
	if (chg->pdata && chg->pdata->register_callbacks)
		chg->pdata->register_callbacks(chg->callbacks);

	charger_i2c_init = 1;
	pr_info("Smb136 charger attach success!!!\n");

	smb136_test_read();

	return 0;

err_pdata:
err_client:
err_callbacks:
	kfree(chg);
	return ret;
}

static int __devexit smb136_remove(struct i2c_client *client)
{
	struct smb136_chg_data *chg = i2c_get_clientdata(client);

	if (chg->pdata && chg->pdata->unregister_callbacks)
		chg->pdata->unregister_callbacks();

	kfree(chg);
	return 0;
}

static const struct i2c_device_id smb136_id[] = {
	{ "smb136-charger", 0 },
	{ }
};


static struct i2c_driver smb136_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "smb136-charger",
	},
	.id_table	= smb136_id,
	.probe	= smb136_i2c_probe,
	.remove	= __devexit_p(smb136_remove),
	.command = NULL,
};


MODULE_DEVICE_TABLE(i2c, smb136_id);

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
