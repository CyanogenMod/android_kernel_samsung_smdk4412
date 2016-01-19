/*
 *  smb347_charger.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  SangYoung Son <hello.son@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/power/sec_battery_px.h>
#include <linux/power/smb347_charger.h>
#include <linux/mfd/max8997.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <plat/gpio-cfg.h>

/* Slave address */
#define SMB347_SLAVE_ADDR		0x0C

/* SMB347 Registers. */
#define SMB347_CHARGE_CURRENT		0X00
#define SMB347_INPUT_CURRENTLIMIT	0X01
#define SMB347_VARIOUS_FUNCTIONS	0X02
#define SMB347_FLOAT_VOLTAGE		0X03
#define SMB347_CHARGE_CONTROL		0X04
#define SMB347_STAT_TIMERS_CONTROL	0x05
#define SMB347_PIN_ENABLE_CONTROL	0x06
#define SMB347_THERM_CONTROL_A		0x07
#define SMB347_SYSOK_USB30_SELECTION	0x08
#define SMB347_OTHER_CONTROL_A	0x09
#define SMB347_OTG_TLIM_THERM_CONTROL	0x0A
#define SMB347_LIMIT_CELL_TEMPERATURE_MONITOR	0x0B
#define SMB347_FAULT_INTERRUPT	0x0C
#define SMB347_STATUS_INTERRUPT	0x0D
#define SMB347_I2C_BUS_SLAVE_ADDR	0x0E

#define SMB347_COMMAND_A	0x30
#define SMB347_COMMAND_B	0x31
#define SMB347_COMMAND_C	0x33
#define SMB347_INTERRUPT_STATUS_A	0x35
#define SMB347_INTERRUPT_STATUS_B	0x36
#define SMB347_INTERRUPT_STATUS_C	0x37
#define SMB347_INTERRUPT_STATUS_D	0x38
#define SMB347_INTERRUPT_STATUS_E	0x39
#define SMB347_INTERRUPT_STATUS_F	0x3A
#define SMB347_STATUS_A	0x3B
#define SMB347_STATUS_B	0x3C
#define SMB347_STATUS_C	0x3D
#define SMB347_STATUS_D	0x3E
#define SMB347_STATUS_E	0x3F

/* Status register C */
#define SMB347_CHARGING_ENABLE	(1 << 0)
#define SMB347_CHARGING_STATUS	(1 << 5)
#define SMB347_CHARGER_ERROR	(1 << 6)

struct smb347_chg_data {
	struct i2c_client *client;
	struct smb_charger_data *pdata;
	struct smb_charger_callbacks *callbacks;
};

static struct smb347_chg_data *smb347_chg;

static bool smb347_check_powersource(struct smb347_chg_data *chg)
{
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_TAB3)
	/* p4 note pq has no problem for charger power */
	return true;
#endif

	/* Power source needs for only P4C H/W rev0.2 */
	if (system_rev != 2)
		return true;

	/* V_BUS detect by TA_nConnected */
	if (!gpio_get_value(chg->pdata->ta_nconnected)) {
		pr_err("smb347 power source is not detected\n");
		return false;
	}

	return true;
}

static int smb347_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	struct smb347_chg_data *chg = smb347_chg;
	int ret = 0;

	/* Only for P4C rev0.2, Check vbus for opeartion charger */
	if (!smb347_check_powersource(chg))
		return -EINVAL;

	if (!client) {
		pr_err("smb347 i2c client error(addr : 0x%02x)\n", reg);
		return -ENODEV;
	}

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		pr_err("smb347 i2c read error(addr : 0x%02x)\n", reg);
		return -EIO;
	}

	*data = ret & 0xff;
	return 0;
}

static int smb347_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	struct smb347_chg_data *chg = smb347_chg;
	int ret = 0;

	/* Only for P4C rev0.2, Check vbus for opeartion charger */
	if (!smb347_check_powersource(chg))
		return -EINVAL;

	if (!client) {
		pr_err("smb347 i2c client error(addr:0x%02x data:0x%02x)\n",
			reg, data);
		return -ENODEV;
	}

	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0) {
		pr_err("smb347 i2c write error(addr:0x%02x data:0x%02x)\n",
			reg, data);
		return -EIO;
	}

	udelay(10);
	return ret;
}

static void smb347_test_read(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	u8 data = 0;
	u32 addr = 0;
	pr_info("%s\n", __func__);

	/* Only for P4C rev0.2, Check vbus for opeartion charger */
	if (!smb347_check_powersource(chg))
		return;

	for (addr = 0; addr <= 0x0E; addr++) {
		smb347_i2c_read(chg->client, addr, &data);
		pr_info("smb347 addr : 0x%02x data : 0x%02x\n", addr, data);
	}

	for (addr = 0x30; addr <= 0x3F; addr++) {
		smb347_i2c_read(chg->client, addr, &data);
		pr_info("smb347 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static void smb347_enable_charging(struct smb347_chg_data *chg)
{
	pr_info("%s\n", __func__);
	smb347_i2c_write(chg->client, SMB347_COMMAND_A, 0x82);
}

static void smb347_disable_charging(struct smb347_chg_data *chg)
{
	pr_info("%s\n", __func__);
	smb347_i2c_write(chg->client, SMB347_COMMAND_A, 0x80);
}

static void smb347_charger_init(struct smb347_chg_data *chg)
{
	pr_info("%s\n", __func__);

	/* Only for P4C rev0.2, Check vbus for opeartion charger */
	if (!smb347_check_powersource(chg))
		return;

	/* Set GPIO_TA_EN as HIGH, charging disable */
	smb347_disable_charging(chg);
	mdelay(100);

	/* Allow volatile writes to CONFIG registers */
	smb347_i2c_write(chg->client, SMB347_COMMAND_A, 0x80);

	/* Command B : USB1 mode, USB mode */
	smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x00);

	/* Charge curr : Fast-chg 2200mA */
	/* Pre-charge curr 250mA, Term curr 250mA */
	smb347_i2c_write(chg->client, SMB347_CHARGE_CURRENT, 0xDD);

	/* Pin enable control : Charger enable control EN Pin - I2C */
	/*  : USB5/1/HC or USB9/1.5/HC Control - Register Control */
	/*  : USB5/1/HC Input state - Tri-state Input */
	smb347_i2c_write(chg->client, SMB347_PIN_ENABLE_CONTROL, 0x00);

	/* Input current limit : DCIN 1800mA, USBIN HC 1800mA */
	smb347_i2c_write(chg->client, SMB347_INPUT_CURRENTLIMIT, 0x66);

	/* Various func. : USBIN primary input, VCHG func. enable */
	smb347_i2c_write(chg->client, SMB347_VARIOUS_FUNCTIONS, 0xB7);

	/* Float voltage : 4.2V */
	smb347_i2c_write(chg->client, SMB347_FLOAT_VOLTAGE, 0x63);

	/* Charge control : Auto recharge disable, APSD disable */
	smb347_i2c_write(chg->client, SMB347_CHARGE_CONTROL, 0x80);

	/* STAT, Timer control : STAT active low, Complete time out 1527min. */
	smb347_i2c_write(chg->client, SMB347_STAT_TIMERS_CONTROL, 0x1A);

	/* Therm control : Therm monitor disable */
	smb347_i2c_write(chg->client, SMB347_THERM_CONTROL_A, 0xBF);

	/* Other control */
	smb347_i2c_write(chg->client, SMB347_OTHER_CONTROL_A, 0x0D);

	/* OTG tlim therm control */
	smb347_i2c_write(chg->client, SMB347_OTG_TLIM_THERM_CONTROL, 0x3F);

	/* Limit cell temperature */
	smb347_i2c_write(chg->client, SMB347_LIMIT_CELL_TEMPERATURE_MONITOR,
	0x01);

	/* Fault interrupt : Clear */
	smb347_i2c_write(chg->client, SMB347_FAULT_INTERRUPT, 0x00);

	/* STATUS ingerrupt : Clear */
	smb347_i2c_write(chg->client, SMB347_STATUS_INTERRUPT, 0x00);
}

static int smb347_get_charging_state(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data = 0;

	smb347_i2c_read(chg->client, SMB347_STATUS_C, &data);
	pr_info("%s : 0x%xh(0x%02x)\n", __func__, SMB347_STATUS_C, data);

	if (data & SMB347_CHARGING_ENABLE)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else {
		/* if error bit check, ignore the status of charger-ic */
		if (data & SMB347_CHARGER_ERROR)
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		/* At least one charge cycle terminated */
		/*Charge current < Termination Current */
		else if (data & SMB347_CHARGING_STATUS)
			status = POWER_SUPPLY_STATUS_FULL;
	}

	return status;
}

static int smb347_get_charger_is_full(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data = 0;

	smb347_i2c_read(chg->client, SMB347_STATUS_C, &data);
	pr_info("%s : 0x%xh(0x%02x)\n", __func__, SMB347_STATUS_C, data);

	if (data & SMB347_CHARGER_ERROR)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (data & SMB347_CHARGING_STATUS)
		status = POWER_SUPPLY_STATUS_FULL;

	return status;
}

static void smb347_set_charging_state(int enable, int charging_mode)
{
	struct smb347_chg_data *chg = smb347_chg;
	pr_info("%s : enable(%d), charging_mode(%d)\n",
		__func__, enable, charging_mode);

	if (enable) {
		/* Only for P4C rev0.2, Check vbus for opeartion charger */
		if (!smb347_check_powersource(chg))
			return;

		/* Init smb347 charger */
		smb347_charger_init(chg);

		switch (charging_mode) {
		case CABLE_TYPE_TA:
			/* Input current limit : DCIN 1800mA, USBIN HC 1800mA */
			smb347_i2c_write(chg->client,
				SMB347_INPUT_CURRENTLIMIT, 0x66);

			/* CommandB : High-current mode */
			smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x03);

			pr_info("%s : 1.8A charging enable\n", __func__);
			break;
		case CABLE_TYPE_DESKDOCK:
			/* Input current limit : DCIN 1500mA, USBIN HC 1500mA */
			smb347_i2c_write(chg->client,
				SMB347_INPUT_CURRENTLIMIT, 0x55);

			/* CommandB : High-current mode */
			smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x03);
			pr_info("%s : 1.5A charging enable\n", __func__);
			break;
		case CABLE_TYPE_USB:
			/* CommandB : USB5 */
			smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x02);
			pr_info("%s : LOW(USB5) charging enable\n", __func__);
			break;
		default:
			/* CommandB : USB1 */
			smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x00);
			pr_info("%s : LOW(USB1) charging enable\n", __func__);
			break;
		}

		smb347_enable_charging(chg);
	} else {
		smb347_disable_charging(chg);
	}

	smb347_test_read();
}

int smb347_get_charging_current(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	u8 data = 0;
	int get_current = 0;

	smb347_i2c_read(chg->client, SMB347_CHARGE_CURRENT, &data);
	switch (data >> 5) {
	case 0:
		get_current = 700;
		break;
	case 1:
		get_current = 900;
		break;
	case 2:
		get_current = 1200;
		break;
	case 3:
		get_current = 1500;
		break;
	case 4:
		get_current = 1800;
		break;
	case 5:
		get_current = 2000;
		break;
	case 6:
		get_current = 2200;
		break;
	case 7:
		get_current = 2500;
		break;
	default:
		get_current = 700;
		break;
	}
	pr_info("%s: Get charging current as %dmA.\n", __func__, get_current);
	return get_current;
}

int smb347_get_input_current(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	u8 data = 0;
	u8 data_dummy = 0;
	int get_current = 0;

	smb347_i2c_read(chg->client, SMB347_INPUT_CURRENTLIMIT, &data);
	data_dummy = (data << 4);

	switch (data_dummy >> 4) {
	case 0:
		get_current = 300;
		break;
	case 1:
		get_current = 500;
		break;
	case 2:
		get_current = 700;
		break;
	case 3:
		get_current = 900;
		break;
	case 4:
		get_current = 1200;
		break;
	case 5:
		get_current = 1500;
		break;
	case 6:
		get_current = 1800;
		break;
	case 7:
		get_current = 2000;
		break;
	case 8:
		get_current = 2200;
		break;
	default:
		get_current = 2500;
		break;
	}
	pr_info("%s: Get Input current as %dmA.\n", __func__, get_current);
	return get_current;
}



int smb347_get_aicl_current(void)
{
	struct smb347_chg_data *chg = smb347_chg;
	u8 data = 0;
	u8 data_dummy = 0;
	int get_current = 0;

	smb347_i2c_read(chg->client, SMB347_STATUS_E, &data);
	data_dummy = (data << 4);

	if ((data & 0x10)) {
		switch (data_dummy >> 4) {
		case 0:
			get_current = 300;
			break;
		case 1:
			get_current = 500;
			break;
		case 2:
			get_current = 700;
			break;
		case 3:
			get_current = 900;
			break;
		case 4:
			get_current = 1200;
			break;
		case 5:
			get_current = 1500;
			break;
		case 6:
			get_current = 1800;
			break;
		case 7:
			get_current = 2000;
			break;
		case 8:
			get_current = 2200;
			break;
		default:
			get_current = 2500;
			break;
		}
	} else {
		get_current = 0;
	}
	pr_info("%s: Get AICL current as %dmA\n", __func__, get_current);

	return get_current;
}

void smb347_set_charging_current(int set_current)
{
	struct smb347_chg_data *chg = smb347_chg;

	if (set_current > 450) {
		/* CommandB : High-current mode */
		smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x03);
		udelay(10);
	} else {
		/* CommandB : USB5 */
		smb347_i2c_write(chg->client, SMB347_COMMAND_B, 0x02);
		udelay(10);
	}
	pr_debug("%s: Set charging current as %dmA.\n", __func__, set_current);
}

void smb347_set_aicl_state(int state)
{
	struct smb347_chg_data *chg = smb347_chg;

	if (state)
		smb347_i2c_write(chg->client,
			SMB347_VARIOUS_FUNCTIONS, 0xB7);
	else
		smb347_i2c_write(chg->client,
			SMB347_VARIOUS_FUNCTIONS, 0xA7);
	pr_info("%s : AICL STATE(%d)\n", __func__, state);
}

static int smb347_i2c_probe
(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb347_chg_data *chg;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	pr_info("%s : smb347 Charger Driver Loading\n", __func__);

	chg = kzalloc(sizeof(struct smb347_chg_data), GFP_KERNEL);
	if (!chg)
		return -ENOMEM;

	chg->callbacks = kzalloc(sizeof(struct smb_charger_callbacks),
		GFP_KERNEL);
	if (!chg->callbacks) {
		kfree(chg);
		return -ENOMEM;
	}

	chg->client = client;
	chg->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chg);
	smb347_chg = chg;

	if (!chg->pdata) {
		pr_err("%s : No platform data supplied\n", __func__);
		ret = -EINVAL;
		goto err_pdata;
	}

	pr_info("register callback functions!\n");
	chg->callbacks->set_charging_state = smb347_set_charging_state;
	chg->callbacks->get_charging_state = smb347_get_charging_state;
	chg->callbacks->set_charging_current = smb347_set_charging_current;
	chg->callbacks->get_charging_current = smb347_get_charging_current;
	chg->callbacks->get_charger_is_full = smb347_get_charger_is_full;
	chg->callbacks->get_aicl_current = smb347_get_aicl_current;
	chg->callbacks->get_input_current = smb347_get_input_current;
	chg->callbacks->set_aicl_state = smb347_set_aicl_state;
	if (chg->pdata && chg->pdata->register_callbacks)
		chg->pdata->register_callbacks(chg->callbacks);

	pr_info("smb347 charger initialized.\n");

	return 0;

err_pdata:
	kfree(chg->callbacks);
	kfree(chg);
	return ret;
}

static int __devexit smb347_remove(struct i2c_client *client)
{
	struct smb347_chg_data *chg = i2c_get_clientdata(client);

	if (chg->pdata && chg->pdata->unregister_callbacks)
		chg->pdata->unregister_callbacks();

	kfree(chg);
	return 0;
}

static const struct i2c_device_id smb347_id[] = {
	{ "smb347-charger", 0 },
};


static struct i2c_driver smb347_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "smb347-charger",
	},
	.probe		= smb347_i2c_probe,
	.remove		= smb347_remove,
	.id_table	= smb347_id,
};


MODULE_DEVICE_TABLE(i2c, smb347_id);

static int __init smb347_init(void)
{
	return i2c_add_driver(&smb347_i2c_driver);
}

static void __exit smb347_exit(void)
{
	i2c_del_driver(&smb347_i2c_driver);
}

module_init(smb347_init);
module_exit(smb347_exit);

MODULE_AUTHOR("SangYoung Son <hello.son@samsung.com>");
MODULE_DESCRIPTION("smb347 charger driver");
MODULE_LICENSE("GPL");
