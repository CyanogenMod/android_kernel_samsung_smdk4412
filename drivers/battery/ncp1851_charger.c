/*
 *  ncp1851_charger.c
 *  Samsung SMB136 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#define CTRL_CHARGER_ENABLE	BIT(6)

#include <linux/battery/sec_charger.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static struct dentry    *ncp1851_dentry;

static int ncp1851_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int ncp1851_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static void ncp1851_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;
	for (i = 0; i < size; i += 3)
		ncp1851_i2c_write(client, (u8) (*(buf + i)), (buf + i) + 1);
}

static void ncp1851_set_command(struct i2c_client *client,
				int reg, int datum)
{
	int val;
	u8 data = 0;
	val = ncp1851_i2c_read(client, reg, &data);
	if (val >= 0) {
		dev_dbg(&client->dev, "%s : reg(0x%02x): 0x%02x",
			__func__, reg, data);
		if (data != datum) {
			data = datum;
			if (ncp1851_i2c_write(client, reg, &data) < 0)
				dev_err(&client->dev,
					"%s : error!\n", __func__);
			val = ncp1851_i2c_read(client, reg, &data);
			if (val >= 0)
				dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static void ncp1851_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;
	for (addr = 0; addr <= 0x12; addr++) {
		ncp1851_i2c_read(client, addr, &data);
		dev_info(&client->dev,
			"ncp1851 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static void ncp1851_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x12; addr++) {
		ncp1851_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static int ncp1851_get_charging_status(struct i2c_client *client)
{
	int stat = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 stat_reg = 0;
	u8 ctrl_reg = 0;
	u8 status = 0;

	ncp1851_i2c_read(client, NCP1851_STATUS, &stat_reg);
	dev_info(&client->dev,
		"%s : charger status Register(0x%02x)\n", __func__, stat_reg);

	ncp1851_i2c_read(client, NCP1851_CTRL1, &ctrl_reg);
	dev_info(&client->dev,
		"%s : charger Control_1 Register(0x%02x)\n",
		__func__, ctrl_reg);

	/* Charge mode : FAULT */
	if ((stat_reg & 0xb0) == 0xb0)
		goto charging_status_end;

	/* At least one charge cycle terminated,
	 * Charge current < Termination Current
	 */
	if ((stat_reg & 0x60) == 0x60) {
		/* top-off by full charging */
		stat = POWER_SUPPLY_STATUS_FULL;
		goto charging_status_end;
	}

	/* Is enabled ? */
	status = (stat_reg & 0xF0) >> 4;
	if (ctrl_reg & 0x40) {
		/* check for 0x06 : no charging (0b00) */
		/* not charging */
		if ((stat_reg & 0xB0) == 0xB0) {
			stat = POWER_SUPPLY_STATUS_NOT_CHARGING;
			goto charging_status_end;
		} else if (stat_reg >= 0x2 && stat_reg <= 6) {
			stat = POWER_SUPPLY_STATUS_CHARGING;
			goto charging_status_end;
		}
	} else
		stat = POWER_SUPPLY_STATUS_DISCHARGING;
charging_status_end:
	return (int)stat;
}

static int ncp1851_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 vin_reg = 0;
	u8 ctrl_reg = 0;

	ncp1851_i2c_read(client, NCP1851_VIN_SNS, &vin_reg);
	dev_info(&client->dev,
		"%s : charger NCP1851_VIN_SNS(0x%02x)\n", __func__, vin_reg);

	ncp1851_i2c_read(client, NCP1851_CTRL1, &ctrl_reg);
	dev_info(&client->dev,
		"%s : charger NCP1851_CTRL1(0x%02x)\n",
		__func__, ctrl_reg);
	/* Is enabled ? */
	if (ctrl_reg & 0x40) {
		if ((vin_reg & 0x80))	/* Input current is NOT OK */
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}
	return (int)health;
}

static u8 ncp1851_get_input_current_limit_data(
			struct sec_charger_info *charger, int input_current)
{
	u8 data;

	if (input_current <= 100)
		data = 0x0;
	else if (input_current <= 500)
		data = 0x1;
	else if (input_current <= 900)
		data = 0x2;
	else if (input_current <= 1500)
		data = 0x3;
	else
		data = 0x3;

	return data;
}

static u8 ncp1851_get_termination_current_limit_data(
			int termination_current)
{
	u8 data;

	if (termination_current <= 100)
		data = 0x0;
	else if (termination_current <= 275)
		data = (termination_current - 100) / 25;
	else
		data = 0x7;

	return data;
}

static u8 ncp1851_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;

	if (fast_charging_current <= 400)
		data = 0x0;
	else if (fast_charging_current <= 1600)
		data = (fast_charging_current - 400) / 100;
	else
		data = 0xC;

	return data;
}

static void ncp1851_charger_function_conrol(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;
	u8 cur_reg;

	if (charger->charging_current < 0) {
		dev_dbg(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}

	pr_info("%s: charger->cable_type : %d\n", __func__,
					charger->cable_type);

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		ncp1851_i2c_read(client, NCP1851_CTRL1, &cur_reg);
		dev_info(&client->dev,
			"%s : charger Control_1 Register(0x%x)\n",
			__func__, cur_reg);
		cur_reg &= ~0x40;

		/* turn off charger */
		ncp1851_set_command(client,
			NCP1851_CTRL1, cur_reg);
	} else {
		/* Pre-charge curr 250mA */
		dev_dbg(&client->dev,
			"%s : fast charging current (%dmA)\n",
			__func__, charger->charging_current);
		dev_dbg(&client->dev,
			"%s : termination current (%dmA)\n",
			__func__, charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);

		data = ncp1851_get_fast_charging_current_data(
			charger->pdata->charging_current[
			charger->cable_type].input_current_limit);
		data |= (ncp1851_get_termination_current_limit_data(
			charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st) << 4);
		ncp1851_set_command(client, NCP1851_IBAT_SET, data);

		/* Input current limit */
		dev_dbg(&client->dev, "%s : input current (%dmA)\n",
			__func__, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		data = ncp1851_get_input_current_limit_data(
			charger,
			charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		/* ncp1851_i2c_read(client, NCP1851_MISC_SET, &cur_reg);
		data |= (cur_reg & 0xfc) */
		cur_reg = 0x60 | data;
		ncp1851_i2c_write(client, NCP1851_MISC_SET, &cur_reg);

		cur_reg = 0x41;
		ncp1851_i2c_write(client, NCP1851_CTRL1, &cur_reg);

		/* Vchg : 4.3V */
		cur_reg = charger->pdata->chg_float_voltage ?
			(charger->pdata->chg_float_voltage - 3300) / 25 :
			0x24;
		ncp1851_i2c_write(client, NCP1851_VBAT_SET, &cur_reg);

		/* IINSET_PIN_Disable, IINLIM_enable, AICL disable
		** if USB Trans pin enable
		*/
		/*
		cur_reg = charger->cable_type == POWER_SUPPLY_TYPE_USB ?
			  0xF2 : 0xE2;
		*/
		cur_reg = 0xF2;
		ncp1851_i2c_write(client, NCP1851_CTRL2, &cur_reg);
	}
	ncp1851_test_read(client);
}

static void ncp1851_charger_otg_conrol(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
}

static int ncp1851_debugfs_show(struct seq_file *s, void *data)
{
	struct sec_charger_info *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "NCP1851 CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = 0x00; reg <= 0x12; reg++) {
		ncp1851_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");

	return 0;
}

static int ncp1851_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, ncp1851_debugfs_show, inode->i_private);
}

static const struct file_operations ncp1851_debugfs_fops = {
	.open           = ncp1851_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

bool sec_hal_chg_init(struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 ctrl_reg;

	ncp1851_i2c_read(client, NCP1851_CTRL1, &ctrl_reg);
	dev_info(&client->dev,
		"%s : charger Control_1 Register(0x%02x)\n",
		__func__, ctrl_reg);

	ctrl_reg &= ~0x1;
	ncp1851_i2c_write(client, NCP1851_CTRL1, &ctrl_reg);

	ncp1851_test_read(client);
	ncp1851_dentry = debugfs_create_file("ncp1851-regs",
			S_IRUSR, NULL, charger, &ncp1851_debugfs_fops);

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
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = ncp1851_get_charging_status(client);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = ncp1851_get_charging_health(client);
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
	u8 cur_reg = 0;

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: set Charging enable : %d\n",
			__func__, val->intval);

		ncp1851_i2c_read(client, NCP1851_CTRL1, &cur_reg);
		if (val->intval)
			cur_reg |= CTRL_CHARGER_ENABLE;
		else
			cur_reg &= ~CTRL_CHARGER_ENABLE;

		ncp1851_charger_function_conrol(client);
		/* ncp1851_test_read(client); */
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->charging_current = val->intval;
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

		ncp1851_read_regs(chg->client, str);
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
			ncp1851_i2c_read(chg->client,
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
			ncp1851_i2c_write(chg->client,
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
