/*
 *  smb358_charger.c
 *  Samsung SMB358 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/battery/sec_charger.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int smb358_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int smb358_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static void smb358_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;
	for (i = 0; i < size; i += 3)
		smb358_i2c_write(client, (u8) (*(buf + i)), (buf + i) + 1);
}

static int smb358_update_reg(struct i2c_client *client, int reg, u8 data)
{
	int ret;
	u8 r_data = 0;
	u8 w_data = 0;
	u8 o_data = data;

	ret = smb358_i2c_read(client, reg, &r_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	w_data  = r_data | data;
	ret = smb358_i2c_write(client, reg, &w_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - write(%d)\n", __func__, ret);
		goto error;
	}

	ret = smb358_i2c_read(client, reg, &data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	dev_info(&client->dev,
		"%s: reg(0x%02x) 0x%02x : 0x%02x -> 0x%02x -> 0x%02x\n",
		__func__, reg, o_data, r_data, w_data, data);

error:
	return ret;
}

static int smb358_clear_reg(struct i2c_client *client, int reg, u8 data)
{
	int ret;
	u8 r_data = 0;
	u8 w_data = 0;
	u8 o_data = data;

	ret = smb358_i2c_read(client, reg, &r_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	w_data  = r_data & (~data);
	ret = smb358_i2c_write(client, reg, &w_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - write(%d)\n", __func__, ret);
		goto error;
	}

	ret = smb358_i2c_read(client, reg, &data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	dev_info(&client->dev,
		"%s: reg(0x%02x)- 0x%02x : 0x%02x -> 0x%02x -> 0x%02x\n",
		__func__, reg, o_data, r_data, w_data, data);

error:
	return ret;
}

static int smb358_volatile_writes(struct i2c_client *client, u8 value)
{
	int ret = 0;

	if (value == SMB358_ENABLE_WRITE) {
		ret = smb358_update_reg(client, SMB358_COMMAND_A, 0x80);
		if (ret < 0) {
			dev_err(&client->dev, "%s: error(%d)\n", __func__, ret);
			goto error;
		}
		dev_info(&client->dev, "%s: ENABLED\n", __func__);

	} else {
		ret = smb358_clear_reg(client, SMB358_COMMAND_A, 0x80);
		if (ret < 0) {
			dev_err(&client->dev, "%s: error(%d)\n", __func__, ret);
			goto error;
		}
		dev_info(&client->dev, "%s: DISABLED\n", __func__);
	}

error:
	return ret;
}

static void smb358_set_command(struct i2c_client *client,
				int reg, u8 datum)
{
	int val;
	u8 after_data;

	if (smb358_i2c_write(client, reg, &datum) < 0)
		dev_err(&client->dev,
			"%s : error!\n", __func__);

	msleep(20);
	val = smb358_i2c_read(client, reg, &after_data);
	if (val >= 0)
		dev_info(&client->dev,
			"%s : reg(0x%02x) 0x%02x => 0x%02x\n",
			__func__, reg, datum, after_data);
	else
		dev_err(&client->dev, "%s : error!\n", __func__);
}

static void smb358_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;
	for (addr = 0; addr <= 0x0f; addr++) {
		smb358_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"%s : smb358 addr : 0x%02x data : 0x%02x\n",
						__func__, addr, data);
	}
	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb358_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"%s : smb358 addr : 0x%02x data : 0x%02x\n",
						__func__, addr, data);
	}
}

static void smb358_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x0f; addr++) {
		smb358_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb358_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}


static int smb358_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;

	smb358_i2c_read(client, SMB358_STATUS_A, &data_a);
	dev_dbg(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb358_i2c_read(client, SMB358_STATUS_B, &data_b);
	dev_dbg(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb358_i2c_read(client, SMB358_STATUS_C, &data_c);
	dev_dbg(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb358_i2c_read(client, SMB358_STATUS_D, &data_d);
	dev_dbg(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb358_i2c_read(client, SMB358_STATUS_E, &data_e);
	dev_dbg(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);
	/* At least one charge cycle terminated,
	 * Charge current < Termination Current
	 */
	if (data_c & 0x20) {
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

static int smb358_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;

	smb358_i2c_read(client, SMB358_STATUS_A, &data_a);
	dev_info(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb358_i2c_read(client, SMB358_STATUS_B, &data_b);
	dev_info(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb358_i2c_read(client, SMB358_STATUS_C, &data_c);
	dev_info(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb358_i2c_read(client, SMB358_STATUS_D, &data_d);
	dev_info(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb358_i2c_read(client, SMB358_STATUS_E, &data_e);
	dev_info(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);

	/* Is enabled ? */
	if (data_c & 0x01) {
		smb358_i2c_read(client, SMB358_INTERRUPT_STATUS_E, &data_e);
		dev_info(&client->dev,
			"%s : charger intterupt status E(0x%02x)\n",
			__func__, data_e);

		if (data_e & 0x01)
			health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		else if (data_e & 0x04)
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}

	return (int)health;
}

static void smb358_allow_volatile_writes(struct i2c_client *client)
{
	int val, reg;
	u8 data;
	reg = SMB358_COMMAND_A;
	val = smb358_i2c_read(client, reg, &data);
	if ((val >= 0) && !(data & 0x80)) {
		dev_dbg(&client->dev,
			"%s : reg(0x%02x): 0x%02x", __func__, reg, data);
		data |= (0x1 << 7);
		if (smb358_i2c_write(client, reg, &data) < 0)
			dev_err(&client->dev, "%s : error!\n", __func__);
		val = smb358_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8) data;
			dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static u8 smb358_get_float_voltage_data(
			int float_voltage)
{
	u8 data;

	if (float_voltage < 3500)
		data = 0;
	else if(float_voltage <= 4340)
		data = (float_voltage - 3500) / 20;
	else if(float_voltage == 4350)
		data = 43; /* (4340 -3500)/20 + 1 */
	else if(float_voltage <= 4500)
		data = (float_voltage - 3500) / 20 + 1;
	else 
		data = 51;

	return data;
}

static u8 smb358_get_input_current_limit_data(
			struct sec_charger_info *charger, int input_current)
{
	u8 data;

	if (input_current <= 300)
		data = 0x00;
	else if (input_current <= 500)
		data = 0x01;
	else if (input_current <= 700)
		data = 0x02;
	else if (input_current <= 1000)
		data = 0x03;
	else if (input_current <= 1200)
		data = 0x04;
	else if (input_current <= 1500)
		data = 0x05;
	else if (input_current <= 1800)
		data = 0x06;
	else if (input_current <= 2000)
		data = 0x07;
	else
		data = 0x08;	/* No input current limit */

	return (data << 4);
}

static u8 smb358_get_termination_current_limit_data(
			int termination_current)
{
	u8 data;

	if (termination_current <= 30)
		data = 0x00;
	else if (termination_current <= 40)
		data = 0x01;
	else if (termination_current <= 60)
		data = 0x02;
	else if (termination_current <= 80)
		data = 0x03;
	else if (termination_current <= 100)
		data = 0x04;
	else if (termination_current <= 125)
		data = 0x05;
	else if (termination_current <= 150)
		data = 0x06;
	else if (termination_current <= 200)
		data = 0x07;
	else
		data = 0x00;

	return data;
}

static u8 smb358_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;

	if (fast_charging_current <= 200)
		data = 0x00;
	else if (fast_charging_current <= 450)
		data = 0x01;
	else if (fast_charging_current <= 600)
		data = 0x02;
	else if (fast_charging_current <= 900)
		data = 0x03;
	else if (fast_charging_current <= 1300)
		data = 0x04;
	else if (fast_charging_current <= 1500)
		data = 0x05;
	else if (fast_charging_current <= 1800)
		data = 0x06;
	else if (fast_charging_current <= 2000)
		data = 0x07;
	else
		data = 0x00;

	return data << 5;
}

static void smb358_charger_function_control(
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
	smb358_volatile_writes(client, SMB358_ENABLE_WRITE);

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* Charger Disabled */
		smb358_set_command(client, SMB358_COMMAND_A, 0xc0);

		smb358_set_command(client, SMB358_COMMAND_B, 0x00);

	} else {

		/* [STEP - 1] ================================================
                 * Volatile write permission(bit 7) - allow(1)
                 * Charging Enable(bit 1) - Enabled(1)
                 * STAT Output(bit 1) - Enabled(1)
                */
                smb358_set_command(client,
                        SMB358_COMMAND_A, 0xC2);

                /* [STEP - 2] ================================================
                 * USB 5/1(9/1.5) Mode(bit 1) - USB1/USB1.5(0), USB5/USB9(1)
                 * USB/HC Mode(bit 0) - USB5/1 or USB9/1.5 Mode(0)
                 *                      High-Current Mode(1)
                */
                switch (charger->cable_type) {
                case POWER_SUPPLY_TYPE_MAINS:
                case POWER_SUPPLY_TYPE_MISC:
                        /* High-current mode */
                        data = 0x03;
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
                smb358_set_command(client,
                        SMB358_COMMAND_B, data);


		/* [STEP 3] Charge Current(0x00) ===============================
		 * Set pre-charge current(bit 4:3) - 450mA(11)
		 * Set fast charge current(bit 7:5)
		 * Set termination current(bit 2:0)
		*/
		dev_info(&client->dev,
			"%s : fast charging current (%dmA)\n",
			__func__, charger->charging_current);
		dev_info(&client->dev,
			"%s : termination current (%dmA)\n",
			__func__, charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);

		data = 0x18;
		data |= smb358_get_fast_charging_current_data(
			charger->charging_current);
		data |= smb358_get_termination_current_limit_data(
			charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		smb358_set_command(client,
			SMB358_CHARGE_CURRENT, data);

		/* [STEP - 4] =================================================
		 * Enable(EN) Pin Control(bit 6) - i2c(0), Pin(1)
		 * Pin control(bit 5) - active high(0), active low(1)
		 * USB5/1/HC input State(bit3) - Dual-state input(1)
		 * USB Input Pre-bias(bit 0) - Enable(1)
		*/
		data = 0x09;
		if (charger->pdata->chg_gpio_en)
			data |= 0x40;
		if (charger->pdata->chg_polarity_en)
			data |= 0x20;
		smb358_set_command(client,
			SMB358_PIN_ENABLE_CONTROL, data);

		/* [STEP - 5] =============================================== */
		dev_info(&client->dev, "%s : input current (%dmA)\n",
			__func__, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		/* Input current limit */
		data = 0x00;
		data |= smb358_get_input_current_limit_data(
			charger,
			charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		smb358_set_command(client,
			SMB358_INPUT_CURRENTLIMIT, data);

		/* [STEP - 6] =================================================
		 * Input to System FET(bit 7) - Controlled by Register(1)
		 * Max System voltage(bit 5) -  Vflt + 0.1v(0)
		 * AICL(bit 4) - Enalbed(1)
		 * VCHG Function(bit 0) - Enabled(1)
		 */
		if (charger->pdata->chg_functions_setting &
			SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT)
			/* disable AICL */
			smb358_set_command(client,
				SMB358_VARIOUS_FUNCTIONS, 0x81);
		else
			/* enable AICL */
			smb358_set_command(client,
				SMB358_VARIOUS_FUNCTIONS, 0x95);

		/* [STEP - 7] =================================================
		 * Pre-charged to Fast-charge Voltage Threshold(Bit 7:6) - 2.3V
		 * Float Voltage(bit 5:0)
		*/
		dev_dbg(&client->dev, "%s : float voltage (%dmV)\n",
				__func__, charger->pdata->chg_float_voltage);
		data = 0x00 ;
		data |= smb358_get_float_voltage_data(
			charger->pdata->chg_float_voltage);
		smb358_set_command(client,
			SMB358_FLOAT_VOLTAGE, data);


		/* [STEP - 8] =================================================
		 * Charge control
		 * Automatic Recharge disable(bit 7),
		 * Current Termination disable(bit 6),
		 * BMD disable(bit 5:4),
		 * INOK Output Configuration : Push-pull(bit 3)
		 * APSD disable
		*/
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
			data &= 0xB0;
			break;
		}
		smb358_set_command(client,
			SMB358_CHARGE_CONTROL, data);

		/* [STEP - 9] =================================================
		 *  STAT active low(bit 7),
		 *  Complete charge Timeou(bit 3:2) - Disabled(11)
		 *  Pre-charge Timeout(bit 1:0) - Disable(11)
		*/
		smb358_set_command(client,
			SMB358_STAT_TIMERS_CONTROL, 0x1F);


		/* [STEP - 10] =================================================
		 * Mininum System Voltage(bit 6) - 3.6v(1)
		 * Therm monitor(bit 4) - Disabled(1)
		 * Soft Cold/Hot Temp Limit Behavior(bit 3:2, bit 1:0) -
		 *	Charger Current + Float voltage Compensation(11)
		*/
		smb358_set_command(client,
			SMB358_THERM_CONTROL_A, 0xFF);

		/* [STEP - 11] ================================================
		 * OTG/ID Pin Control(bit 7:6) - RID Disabled, OTG I2c(00)
		 * Minimum System Voltage(bit 4) - 3.6V
		 * Low-Battery/SYSOK Voltage threshold(bit 3:0) - Disabled(0000)
		*/
		smb358_set_command(client,
			SMB358_OTHER_CONTROL_A, 0x00);

		/* [STEP - 12] ================================================
		 * Charge Current Compensation(bit 7:6) - 200mA(00)
		 * Digital Thermal Regulation Threshold(bit 5:4) - 130c
		 * OTG current Limit at USBIN(Bit 3:2) - 900mA(11)
		 * OTG Battery UVLO Threshold(Bit 1:0) - 3.3V(11)
		*/
		smb358_set_command(client,
			SMB358_OTG_TLIM_THERM_CONTROL, 0x3F);

		/* [STEP - 13] ================================================
		 * Hard/Soft Limit Cell temp monitor
		*/
		smb358_set_command(client,
			SMB358_LIMIT_CELL_TEMPERATURE_MONITOR, 0x01);

		/* [STEP - 14] ================================================
		 * FAULT interrupt - Disabled
		*/
		smb358_set_command(client,
			SMB358_FAULT_INTERRUPT, 0x00);

		/* [STEP - 15] ================================================
		 * STATUS ingerrupt - Clear
		*/
		smb358_set_command(client,
			SMB358_STATUS_INTERRUPT, 0x00);

	}

	smb358_volatile_writes(client, SMB358_DISABLE_WRITE);
}

static void smb358_charger_otg_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	smb358_volatile_writes(client, SMB358_ENABLE_WRITE);

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* Charger Disabled */
		smb358_clear_reg(client, SMB358_COMMAND_A, 0x02);
	} else {
		/* Change "OTG output current limit" to 250mA */
		smb358_clear_reg(client, SMB358_OTG_TLIM_THERM_CONTROL, 0x0C);

		/* OTG Enalbed*/
		smb358_update_reg(client, SMB358_COMMAND_A, 0x10);

		smb358_set_command(client, SMB358_COMMAND_B, 0x00);

		/* Change "OTG output current limit" to 750mA */
		smb358_update_reg(client, SMB358_OTG_TLIM_THERM_CONTROL, 0x80);
	}

	smb358_volatile_writes(client, SMB358_DISABLE_WRITE);

}

static void smb358_set_charging_current(
		struct i2c_client *client, int charging_current)
{
	u8 data;

	smb358_i2c_read(client, SMB358_CHARGE_CURRENT, &data);
	data &= 0xe0;
	data |= smb358_get_fast_charging_current_data(charging_current);
	smb358_set_command(client, SMB358_CHARGE_CURRENT, data);
}

static void smb358_set_charging_input_current_limit(
		struct i2c_client *client, int input_current_limit)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	/* Input current limit */
	data = 0;
	data = smb358_get_input_current_limit_data(
		charger, input_current_limit);
	smb358_set_command(client, SMB358_INPUT_CURRENTLIMIT, data);
}

static int smb358_debugfs_show(struct seq_file *s, void *data)
{
	struct sec_charger_info *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "SMB CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = 0x00; reg <= 0x0E; reg++) {
		smb358_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	for (reg = 0x30; reg <= 0x3F; reg++) {
		smb358_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");
	return 0;
}

static int smb358_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, smb358_debugfs_show, inode->i_private);
}

static const struct file_operations smb358_debugfs_fops = {
	.open           = smb358_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

bool sec_hal_chg_init(struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	dev_info(&client->dev,
		"%s: SMB358 Charger init(Start)!!\n", __func__);

	smb358_test_read(client);
	(void) debugfs_create_file("smb358_regs",
		S_IRUGO, NULL, (void *)charger, &smb358_debugfs_fops);

	return true;
}

bool sec_hal_chg_suspend(struct i2c_client *client)
{
	dev_info(&client->dev,
                "%s: CHARGER - SMB358(suspend mode)!!\n", __func__);

	return true;
}

bool sec_hal_chg_resume(struct i2c_client *client)
{
	dev_info(&client->dev,
                "%s: CHARGER - SMB358(resume mode)!!\n", __func__);

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
		val->intval = smb358_get_charging_status(client);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = smb358_get_charging_health(client);
		break;
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
		if (charger->charging_current) {
			smb358_i2c_read(client, SMB358_STATUS_B, &data);
			if (data & 0x20)
				switch ((data & 0x18) >> 3) {
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
					val->intval = 100;
					break;
				case 1:
					val->intval = 200;
					break;
				case 2:
					val->intval = 450;
					break;
				case 3:
					val->intval = 600;
					break;
				case 4:
					val->intval = 900;
					break;
				case 5:
					val->intval = 1300;
					break;
				case 6:
					val->intval = 1500;
					break;
				case 7:
					val->intval = 1800;
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
			smb358_charger_otg_control(client);
		else if (charger->charging_current > 0)
			smb358_charger_function_control(client);
		else {
			smb358_charger_function_control(client);
			smb358_charger_otg_control(client);
		}
		smb358_test_read(client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		smb358_set_charging_input_current_limit(client, val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		smb358_set_charging_current(client, val->intval);
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

		smb358_read_regs(chg->client, str);
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
			smb358_i2c_read(chg->client,
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
			smb358_i2c_write(chg->client,
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
