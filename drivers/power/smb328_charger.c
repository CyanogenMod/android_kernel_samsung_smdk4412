/*
 *  smb328_charger.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/power/smb328_charger.h>
#define DEBUG

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_MISC,
	CABLE_TYPE_OTG,
};

#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
static bool is_ovp_status;
#endif

static int smb328_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret = 0;

	if (!client)
		return -ENODEV;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	*data = ret & 0xff;
	return *data;
}

static int smb328_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	if (!client)
		return -ENODEV;

	return i2c_smbus_write_byte_data(client, reg, data);
}

static void smb328_test_read(struct i2c_client *client)
{
	struct smb328_chip *chg = i2c_get_clientdata(client);
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr < 0x0c; addr++) {
		smb328_i2c_read(chg->client, addr, &data);
		dev_info(&client->dev,
			"smb328 addr : 0x%02x data : 0x%02x\n", addr, data);
	}

	for (addr = 0x30; addr < 0x3D; addr++) {
		smb328_i2c_read(chg->client, addr, &data);
		dev_info(&client->dev,
			"smb328 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static void smb328a_charger_function_conrol(struct i2c_client *client);

static int smb328_get_charging_status(struct i2c_client *client)
{
	struct smb328_chip *chg = i2c_get_clientdata(client);
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;

	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_A, &data_a);
	dev_info(&client->dev, "%s : charging status A(0x%02x)\n",
		__func__, data_a);
	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_B, &data_b);
	dev_info(&client->dev, "%s : charging status B(0x%02x)\n",
		__func__, data_b);
	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_C, &data_c);
	dev_info(&client->dev, "%s : charging status C(0x%02x)\n",
		__func__, data_c);

	/* check for safety timer in USB charging */
	/* If safety timer is activated in USB charging, reset charger */
#if 1
	/* write 0xAA in register 0x30 to reset watchdog timer, */
	/* it can replace this work-around */
	if (chg->is_enable && chg->cable_type == CABLE_TYPE_USB) {
		if ((data_c & 0x30) == 0x20) {	/* safety timer activated */
			/* reset charger */
			dev_info(&client->dev,
			"%s : Reset charger, safety timer is activated!\n",
			__func__);

			chg->is_enable = false;
			smb328a_charger_function_conrol(chg->client);

			chg->is_enable = true;
			smb328a_charger_function_conrol(chg->client);
		}
	}
#endif

	/* At least one charge cycle terminated, */
	/* Charge current < Termination Current */
	if ((data_c & 0xc0) == 0xc0) {
		/* top-off by full charging */
		status = POWER_SUPPLY_STATUS_FULL;
		goto charging_status_end;
	}

	/* Is enabled ? */
	if (data_c & 0x01) {
		/* check for 0x30 : 'safety timer' (0b01 or 0b10) or */
		/* 'waiting to begin charging' (0b11) */
		/* check for 0x06 : no charging (0b00) */
		if ((data_c & 0x30) || !(data_c & 0x06))  {
			/* not charging */
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

static int smb328_get_charging_health(struct i2c_client *client)
{
	struct smb328_chip *chg = i2c_get_clientdata(client);
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;

	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_A, &data_a);
	dev_info(&client->dev, "%s : charging status A(0x%02x)\n",
		__func__, data_a);
	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_B, &data_b);
	dev_info(&client->dev, "%s : charging status B(0x%02x)\n",
		__func__, data_b);
	smb328_i2c_read(chg->client,
		SMB328A_BATTERY_CHARGING_STATUS_C, &data_c);
	dev_info(&client->dev, "%s : charging status C(0x%02x)\n",
		__func__, data_c);

	/* Is enabled ? */
	if (data_c & 0x01) {
		if (!(data_a & 0x02))	/* Input current is NOT OK */
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}

#if defined(CONFIG_MACH_Q1_CHN)
	{
		u8 data_chn;
		smb328_i2c_read(chg->client, 0x37, &data_chn);
		dev_info(&client->dev,
			"%s : charging interrupt status C(0x%02x)\n",
			__func__, data_c);

		if (data_chn & 0x04)
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;

		if (health == POWER_SUPPLY_HEALTH_OVERVOLTAGE)
			is_ovp_status = true;
		else
			is_ovp_status = false;
	}
#endif
	return (int)health;
}

#if defined(CONFIG_MACH_Q1_CHN)
static int smb328_is_ovp_status(struct i2c_client *client)
{
	struct smb328_chip *chg = i2c_get_clientdata(client);
	int status = POWER_SUPPLY_HEALTH_UNKNOWN;
	u8 data = 0;

	smb328_i2c_read(chg->client, 0x37, &data);
	dev_info(&client->dev, "%s : 0x37h(0x%02x)\n",
		__func__, data);

	if (data & 0x04) {
		is_ovp_status = true;
		status = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	} else {
		is_ovp_status = false;
		status = POWER_SUPPLY_HEALTH_GOOD;
	}

	return (int)status;
}
#endif

static void smb328a_allow_volatile_writes(struct i2c_client *client)
{
	int val, reg;
	u8 data;

	reg = SMB328A_COMMAND;
	val = smb328_i2c_read(client, reg, &data);
	if ((val >= 0) && !(val & 0x80)) {
		data = (u8)val;
		dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
			__func__, reg, data);
		data |= (0x1 << 7);
		if (smb328_i2c_write(client, reg, data) < 0)
			pr_err("%s : error!\n", __func__);
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)data;
			pr_info("%s : => reg (0x%x) = 0x%x\n",
				__func__, reg, data);
		}
	}
}

static void smb328a_charger_function_conrol(struct i2c_client *client)
{
	struct smb328_chip *chip = i2c_get_clientdata(client);
	int val, reg;
	u8 data, set_data;

	if (chip->is_otg) {
		dev_info(&client->dev,
		"%s : OTG is activated. Ignore command (type:%d, enable:%s)\n",
		__func__, chip->cable_type,
		chip->is_enable ? "true" : "false");
		return;
	}

	smb328a_allow_volatile_writes(client);

	if (!chip->is_enable) {
		reg = SMB328A_FUNCTION_CONTROL_B;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
				data = 0x0;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_COMMAND;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data = 0x98;	/* turn off charger */
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}
	} else {
		/* reset watchdog timer if it occured */
		reg = SMB328A_CLEAR_IRQ;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data = 0xaa;
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}

		reg = SMB328A_INPUT_AND_CHARGE_CURRENTS;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (chip->cable_type == CABLE_TYPE_AC) {
				/* fast 1000mA, termination 200mA */
				set_data = 0xb7;
			} else if (chip->cable_type == CABLE_TYPE_MISC) {
				/* fast 700mA, termination 200mA */
				set_data = 0x57;
			} else {
				/* fast 500mA, termination 200mA */
				set_data = 0x17;
			}
			if (data != set_data) {
				/* this can be changed with top-off setting */
				data = set_data;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_CURRENT_TERMINATION;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (chip->cable_type == CABLE_TYPE_AC) {
				/* input 1A, threshold 4.25V, AICL enable */
				set_data = 0xb0;
			} else if (chip->cable_type == CABLE_TYPE_MISC) {
				/* input 700mA, threshold 4.25V, AICL enable */
				set_data = 0x50;
			} else {
				/* input 450mA, threshold 4.25V, AICL disable */
				set_data = 0x14;
#if defined(CONFIG_MACH_Q1_CHN)
				/* turn off pre-bias for ovp */
				set_data &= ~(0x10);
#endif
			}
			if (data != set_data) {
				data = set_data;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_FLOAT_VOLTAGE;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0xca) {
				data = 0xca; /* 4.2V float voltage */
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_FUNCTION_CONTROL_A1;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
#if 1
			if (data != 0xda) {
				data = 0xda;	/* top-off by ADC */
#else
			if (data != 0x9a) {
				data = 0x9a;	/* top-off by charger */
#endif
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_FUNCTION_CONTROL_A2;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			/* 0x4c -> 0x4e (watchdog timer enabled - SUMMIT) */
			if (data != 0x4e) {
				data = 0x4e;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_FUNCTION_CONTROL_B;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
#if defined(CONFIG_MACH_Q1_CHN)
				data = 0x80;
#else
				data = 0x0;
#endif
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_OTG_PWR_AND_LDO_CONTROL;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			set_data = 0xf5;
			if (chip->cable_type == CABLE_TYPE_AC)
				set_data = 0xf5;
			else if (chip->cable_type == CABLE_TYPE_MISC)
				set_data = 0xf5;
			else
				set_data = 0xcd;
			if (data != set_data) {
				data = set_data;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_VARIOUS_CONTROL_FUNCTION_A;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0xf6) {
				/* this can be changed with top-off setting */
				data = 0xf6;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_CELL_TEMPERATURE_MONITOR;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
				data = 0x0;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_INTERRUPT_SIGNAL_SELECTION;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
				data = 0x0;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		reg = SMB328A_COMMAND;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			/* turn on charger */
			if (chip->cable_type == CABLE_TYPE_AC)
				data = 0x8c;
			else if (chip->cable_type == CABLE_TYPE_MISC)
				data = 0x88;
			else
				data = 0x88; /* USB */
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}
	}
}

static void smb328a_charger_otg_conrol(struct i2c_client *client)
{
	struct smb328_chip *chip = i2c_get_clientdata(client);
	int val, reg;
	u8 data;

	smb328a_allow_volatile_writes(client);

	if (chip->is_otg) {
		reg = SMB328A_FUNCTION_CONTROL_B;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
				data = 0x0;
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		/* delay for reset of charger */
		mdelay(150);

		reg = SMB328A_OTG_PWR_AND_LDO_CONTROL;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data = 0xcd;	/* OTG 350mA */
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}

		reg = SMB328A_COMMAND;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data = 0x9a;	/* turn on OTG */
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}
	} else {
		reg = SMB328A_FUNCTION_CONTROL_B;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			if (data != 0x0) {
				data = 0x0c;	/* turn off charger */
				if (smb328_i2c_write(client, reg, data) < 0)
					pr_err("%s : error!\n", __func__);
				val = smb328_i2c_read(client, reg, &data);
				if (val >= 0) {
					data = (u8)val;
					dev_info(&client->dev,
						"%s : => reg (0x%x) = 0x%x\n",
						__func__, reg, data);
				}
			}
		}

		/* delay for reset of charger */
		mdelay(150);

		reg = SMB328A_COMMAND;
		val = smb328_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8)val;
			dev_info(&client->dev, "%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data = 0x98;	/* turn off OTG */
			if (smb328_i2c_write(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb328_i2c_read(client, reg, &data);
			if (val >= 0) {
				data = (u8)val;
				dev_info(&client->dev,
					"%s : => reg (0x%x) = 0x%x\n",
					__func__, reg, data);
			}
		}
	}
}

static int smb328_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct smb328_chip *chip = container_of(psy,
						  struct smb328_chip,
						  charger);
	u8 data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = smb328_get_charging_status(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = chip->cable_type;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = smb328_get_charging_health(chip->client);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->is_enable;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (chip->is_enable) {
			smb328_i2c_read(chip->client,
				SMB328A_INPUT_AND_CHARGE_CURRENTS, &data);
			switch (data >> 5) {
			case 0:
				val->intval = 450;
				break;
			case 1:
				val->intval = 600;
				break;
			case 2:
				val->intval = 700;
				break;
			case 3:
				val->intval = 800;
				break;
			case 4:
				val->intval = 900;
				break;
			case 5:
				val->intval = 1000;
				break;
			case 6:
				val->intval = 1100;
				break;
			case 7:
				val->intval = 1200;
				break;
			}
		} else
			val->intval = 0;
		break;
#if defined(CONFIG_MACH_Q1_CHN)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = smb328_is_ovp_status(chip->client);
		break;
#endif
	default:
		return -EINVAL;
	}

	dev_info(&chip->client->dev, "%s: smb328_get_property (%d,%d)\n",
		__func__, psp, val->intval);

	return 0;
}

static int smb328_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct smb328_chip *chip = container_of(psy,
						  struct smb328_chip,
						  charger);

	dev_info(&chip->client->dev, "%s: smb328_set_property (%d,%d)\n",
		__func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
#if defined(CONFIG_MACH_Q1_CHN)
		is_ovp_status = false;
#endif
		chip->is_enable = (val->intval == POWER_SUPPLY_STATUS_CHARGING);
		smb328a_charger_function_conrol(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		/* only for OTG support */
		chip->is_otg = val->intval;
		smb328a_charger_otg_conrol(chip->client);
		smb328_test_read(chip->client);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		chip->is_enable = (bool)val->intval;
		smb328a_charger_function_conrol(chip->client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (val->intval <= 450)
			chip->cable_type = CABLE_TYPE_USB;
		else
			chip->cable_type = CABLE_TYPE_AC;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static irqreturn_t smb328_irq_thread(int irq, void *data)
{
	struct smb328_chip *chip = data;
	int ret = 0;
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
	u8 data1 = 0;
#endif

	dev_info(&chip->client->dev, "%s: chg_ing IRQ occurred!\n", __func__);

#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
	smb328_i2c_read(chip->client, 0x37, &data1);

	if (data1 & 0x04) {
		/* if usbin(usb vbus) is in over-voltage status. */
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
#else
	if (chip->pdata->topoff_cb)
		ret = chip->pdata->topoff_cb();

	if (ret) {
		dev_err(&chip->client->dev,
			"%s: error from topoff_cb(%d)\n",
			__func__, ret);
		return IRQ_HANDLED;
	}
#endif
	return IRQ_HANDLED;
}

static int smb328_irq_init(struct smb328_chip *chip)
{
	struct i2c_client *client = chip->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			smb328_irq_thread,
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
			IRQ_TYPE_EDGE_BOTH,
#else
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
#endif
			"SMB328 charger", chip);
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

static int smb328_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb328_chip *chip;
	int ret = 0;
	int gpio = 0;
	u8 data;
	int i;

	i = 10;
	while (1) {
		if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
			goto I2CERROR;

		if (smb328_i2c_read(client, 0x36, &data) >= 0)	/* check HW */
			break;

I2CERROR:
		if (!i--)
			return -EIO;
		msleep(300);
	}

	dev_info(&client->dev,
		"%s : SMB328 Charger Driver Loading\n", __func__);

	chip = kzalloc(sizeof(struct smb328_chip), GFP_KERNEL);
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

	chip->is_otg = false;
	chip->is_enable = false;
	chip->cable_type = CABLE_TYPE_NONE;

	chip->charger.name		= "smb328-charger";
	chip->charger.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->charger.get_property	= smb328_get_property;
	chip->charger.set_property	= smb328_set_property;
	chip->charger.properties	= smb328_charger_props;
	chip->charger.num_properties	= ARRAY_SIZE(smb328_charger_props);

	ret = power_supply_register(&client->dev, &chip->charger);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
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
#if defined(CONFIG_MACH_Q1_CHN)
#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
		/*  set external interrupt */
		s3c_gpio_cfgpin(chip->pdata->gpio_chg_ing,
			S3C_GPIO_SFN(0xf));
#endif
		client->irq = gpio_to_irq(chip->pdata->gpio_chg_ing);
		ret = smb328_irq_init(chip);
		if (ret)
			goto err_pdata;
#else
		s3c_gpio_cfgpin(chip->pdata->gpio_chg_ing,
			S3C_GPIO_INPUT);
		s3c_gpio_setpull(chip->pdata->gpio_chg_ing,
			S3C_GPIO_PULL_NONE);
#endif
	}

#if defined(CONFIG_MACH_Q1_CHN) && defined(CONFIG_SMB328_CHARGER)
	is_ovp_status = false;
#endif

	smb328_test_read(client);

	return 0;

err_pdata:
	kfree(chip);
	return ret;
}

static int __devexit smb328_remove(struct i2c_client *client)
{
	struct smb328_chip *chip = i2c_get_clientdata(client);

	kfree(chip);
	return 0;
}

static const struct i2c_device_id smb328_id[] = {
	{"smb328-charger", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, smb328_id);

static struct i2c_driver smb328_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "smb328-charger",
	},
	.probe	= smb328_probe,
	.remove	= __devexit_p(smb328_remove),
	.command = NULL,
	.id_table	= smb328_id,
};

static int __init smb328_init(void)
{
	return i2c_add_driver(&smb328_i2c_driver);
}

static void __exit smb328_exit(void)
{
	i2c_del_driver(&smb328_i2c_driver);
}

module_init(smb328_init);
module_exit(smb328_exit);

MODULE_AUTHOR("Ikkeun Kim <iks.kim@samsung.com>");
MODULE_DESCRIPTION("smb328 charger driver");
MODULE_LICENSE("GPL");
