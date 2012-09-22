/*
 *  max17042-fuelgauge.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 *  based on max17040_battery.c
 *
 *  <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/power/max17042_fuelgauge_u1.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>

static ssize_t sec_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf);

static ssize_t sec_fg_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count);

struct max17042_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct max17042_platform_data	*pdata;

	int vcell;			/* battery voltage */
	int avgvcell;		/* average battery voltage */
	int vfocv;		/* calculated battery voltage */
	int soc;			/* battery capacity */
	int raw_soc;		/* fuel gauge raw data */
	int temperature;
	int fuel_alert_soc;		/* fuel alert threshold */
	bool is_fuel_alerted;	/* fuel alerted */
	struct wake_lock fuel_alert_wake_lock;
	bool is_enable;			/* can be fuel guage enable */

#ifdef RECAL_SOC_FOR_MAXIM
	int cnt;
	int recalc_180s;
	int boot_cnt;
#endif
};

static int max17042_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17042_chip *chip = container_of(psy,
						  struct max17042_chip,
						  battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		switch (val->intval) {
		case 0:	/*vcell */
		val->intval = chip->vcell;
		break;
		case 1:	/*vfocv */
			val->intval = chip->vfocv;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		val->intval = chip->avgvcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		switch (val->intval) {
		case 0:	/*normal soc */
		val->intval = chip->soc;
		break;
		case 1: /*raw soc */
			val->intval = chip->raw_soc;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = chip->temperature / 100;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max17042_write_reg(struct i2c_client *client, int reg, u8 * buf)
{
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d (reg = 0x%x)\n",
		__func__, ret, reg);

	return ret;
}

static int max17042_read_reg(struct i2c_client *client, int reg, u8 * buf)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d (reg = 0x%x)\n",
		__func__, ret, reg);

	return ret;
}

static void max17042_write_reg_array(struct i2c_client *client,
				struct max17042_reg_data *data, int size)
{
	int i;

	for (i = 0; i < size; i += 3)
		max17042_write_reg(client, (data + i)->reg_addr,
				   ((u8 *) (data + i)) + 1);
}

static void max17042_init_regs(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	/* struct max17042_platform_data *pdata = client->dev.platform_data; */

	dev_info(&client->dev, "%s\n", __func__);

	if (chip->is_enable) {
		/* max17042_write_reg_array(client, pdata->init,
			pdata->init_size); */

		if (max17042_read_reg(client, MAX17042_REG_FILTERCFG, data) < 0)
			return;

		/* Clear average vcell (12 sec) */
		data[0] &= 0x8f;

		max17042_write_reg(client, MAX17042_REG_FILTERCFG, data);
	}
}

static void max17042_alert_init(struct i2c_client *client)
{
	struct max17042_platform_data *pdata = client->dev.platform_data;

	dev_info(&client->dev, "%s\n", __func__);

	max17042_write_reg_array(client, pdata->alert_init,
		pdata->alert_init_size);
}

static int max17042_read_vfocv(struct i2c_client *client)
{
	u8 data[2];
	u32 vfocv = 0;
	struct max17042_chip *chip = i2c_get_clientdata(client);

	if (chip->is_enable) {
		if (max17042_read_reg(client, MAX17042_REG_VFOCV, data) < 0)
			return -1;

		vfocv = ((data[0] >> 3) + (data[1] << 5)) * 625 / 1000;

		chip->vfocv = vfocv;

		return vfocv;
	} else
		return 4000;

}

#if 0
static int max17042_read_vfsoc(struct i2c_client *client)
{
	u8 data[2];
	u32 vfsoc = 0;

#ifndef NO_READ_I2C_FOR_MAXIM
	if (max17042_read_reg(client, MAX17042_REG_SOC_VF, data) < 0)
		return -1;

	vfsoc = data[1];

	return vfsoc;
#else
	return 100;
#endif
}
#else
static void max17042_get_soc(struct i2c_client *client);
static int max17042_read_vfsoc(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	max17042_get_soc(client);

	return chip->soc;
}
#endif

static void max17042_reset_soc(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (chip->is_enable) {
		dev_info(&client->dev, "%s : Before quick-start - "
			"VfOCV(%d), VfSOC(%d)\n",
			__func__, max17042_read_vfocv(client),
			max17042_read_vfsoc(client));

		if (max17042_read_reg(client, MAX17042_REG_MISCCFG, data) < 0)
			return;

		/* Set bit10 makes quick start */
		data[1] |= (0x1 << 2);
		max17042_write_reg(client, MAX17042_REG_MISCCFG, data);

		msleep(500);

		dev_info(&client->dev, "%s : After quick-start - "
			"VfOCV(%d), VfSOC(%d)\n",
			__func__, max17042_read_vfocv(client),
			max17042_read_vfsoc(client));
	}

	return;
}

static void max17042_get_vcell(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (chip->is_enable) {
		if (max17042_read_reg(client, MAX17042_REG_VCELL, data) < 0)
			return;

		chip->vcell = ((data[0] >> 3) + (data[1] << 5)) * 625;

		if (max17042_read_reg(client, MAX17042_REG_AVGVCELL, data) < 0)
			return;

		chip->avgvcell = ((data[0] >> 3) + (data[1] << 5)) * 625;
	} else {
		chip->vcell = 4000000;
		chip->avgvcell = 4000000;
	}
}

static int max17042_recalc_soc(struct i2c_client *client, int boot_cnt)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	u8 data[2];
	u16 temp_vfocv = 0;
	int soc;

	if (chip->is_enable) {
		/* AverageVcell : 175.8ms * 256 = 45s sampling */
		if (boot_cnt < 4) /* 30s using VCELL */
			max17042_read_reg(client, MAX17042_REG_VCELL, data);
		else
			max17042_read_reg(client, MAX17042_REG_AVGVCELL, data);
		temp_vfocv = (data[1] << 8);
		temp_vfocv |= data[0];

		if (psy != NULL) {
			psy->get_property(psy,
				POWER_SUPPLY_PROP_ONLINE, &value);

			if (value.intval == 0)
				temp_vfocv = temp_vfocv + 0x0380; /* +70mV */
			else
				temp_vfocv = temp_vfocv - 0x0380; /* -70mV */

			dev_info(&client->dev, "cable = %d, ", value.intval);
		} else
			temp_vfocv = temp_vfocv + 0x0380; /* +70mV */

		data[1] = temp_vfocv >> 8;
		data[0] = 0x00FF & temp_vfocv;
		dev_info(&client->dev, "forced write to vfocv %d mV\n",
			 (temp_vfocv >> 4) * 125 / 100);
		max17042_write_reg(client, MAX17042_REG_VFOCV, data);

		msleep(200);

		max17042_read_reg(client, MAX17042_REG_SOC_VF, data);
		soc = min((int)data[1], 100);

		max17042_read_reg(client, MAX17042_REG_VCELL, data);
		dev_info(&client->dev, "new vcell = %d, vfocv = %d, soc = %d\n",
			 ((data[0] >> 3) + (data[1] << 5)) * 625 / 1000,
			 max17042_read_vfocv(client), soc);
	} else
		soc = 70;

	return soc;
}

static void max17042_get_soc(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	int soc;
	int diff = 0;

	if (chip->is_enable) {
		if (max17042_read_reg(client, MAX17042_REG_SOC_VF, data) < 0)
			return;
#ifndef PRODUCT_SHIP
		dev_info(&chip->client->dev, "%s : soc(%02x%02x)\n",
				__func__, data[1], data[0]);
#endif
		soc = (data[1] * 100) + (data[0] * 100 / 256);

		chip->raw_soc = min(soc / 100, 100);

#ifdef RECAL_SOC_FOR_MAXIM
		if (chip->pdata->need_soc_recal()) {
			dev_info(&client->dev,
				"%s : recalculate soc\n", __func__);

			/*modified 3.6V cut-off */
			/*raw 3% ~ 95% */
			soc = (soc < 300) ? 0 : ((soc - 300) * 100 / 9200) + 1;

			if (chip->boot_cnt < 4)
				chip->boot_cnt++;

			dev_info(&client->dev,
				"vcell = %d, vfocv = %d, soc = %d\n",
				chip->vcell,  max17042_read_vfocv(client), soc);

			if (soc < 5) {
				dev_info(&client->dev,
					"recalc force soc = %d, diff = %d!\n",
					soc, diff);
				chip->recalc_180s = 1;
			}

			/*when using fuelgauge level, diff is calculated */
			if (chip->recalc_180s == 0 && chip->boot_cnt != 1) {
				if (chip->soc > soc)
					diff = chip->soc - soc;
				else
					diff = soc - chip->soc;
			} else	/*during recalc, diff is not valid */
				diff = 0;

			if (diff > 10) {
				dev_info(&client->dev,
					"recalc 180s soc = %d, diff = %d!\n",
					soc, diff);
				chip->recalc_180s = 1;
			}

			if (chip->recalc_180s == 1) {
				if (chip->cnt < 18) {
					chip->cnt++;
					soc = max17042_recalc_soc(client,
						chip->boot_cnt);
				} else {
					chip->recalc_180s = 0;
					chip->cnt = 0;
				}
			}
		} else {
			/*modified 3.4V cut-off */
			/*raw 1.6% ~ 97.6% */
			soc = (soc > 100) ? ((soc - 60) * 100 / 9700) : 0;
			/*raw 1.5% ~ 95% */
		/*soc = (soc < 150) ? 0 : ((soc - 150) * 100 / 9350) + 1; */
		}
#else
		/* adjusted soc by adding 0.45 */
		soc += 45;
		soc /= 100;
#endif
	} else
		soc = 70;

	soc = min(soc, 100);

	chip->soc = soc;

#ifndef PRODUCT_SHIP
	dev_info(&client->dev, "%s : use raw (%d), soc (%d)\n",
		__func__, chip->raw_soc, soc);
#endif
}

static void max17042_get_temperature(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	s32 temper = 0;

	if (chip->is_enable) {
		if (max17042_read_reg(client,
			MAX17042_REG_TEMPERATURE, data) < 0)
			return;

		/* data[] store 2's compliment format number */
		if (data[1] & (0x1 << 7)) {
			/* Negative */
			temper = ((~(data[1])) & 0xFF) + 1;
			temper *= (-1000);
		} else {
			temper = data[1] & 0x7F;
			temper *= 1000;
			temper += data[0] * 39 / 10;
		}

		dev_dbg(&client->dev, "%s: MAX17042 Temperature = %d\n",
			__func__, chip->temperature);
	} else
		temper = 40;

	chip->temperature = temper;
}

static void max17042_get_version(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (chip->is_enable) {
		if (max17042_read_reg(client, MAX17042_REG_VERSION, data) < 0)
			return;

		dev_info(&client->dev, "MAX17042 Fuel-Gauge Ver %d%d\n",
				data[0], data[1]);
	}
}

static int max17042_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct max17042_chip *chip = container_of(psy,
						  struct max17042_chip,
						  battery);
	u8 data[2];

	switch (psp) {
	case POWER_SUPPLY_PROP_TEMP:
		if (chip->pdata->enable_gauging_temperature &&
			chip->is_enable) {
			data[0] = 0;
			data[1] = val->intval;
			max17042_write_reg(chip->client,
				MAX17042_REG_TEMPERATURE, data);
		} else
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void max17042_work(struct work_struct *work)
{
	struct max17042_chip *chip;

	chip = container_of(work, struct max17042_chip, work.work);

	max17042_get_vcell(chip->client);
	max17042_read_vfocv(chip->client);
	max17042_get_soc(chip->client);

	if (chip->pdata->enable_gauging_temperature)
		max17042_get_temperature(chip->client);

	/* polling check for fuel alert for booting in low battery*/
	if (chip->raw_soc < chip->fuel_alert_soc) {
		if (!(chip->is_fuel_alerted) &&
			chip->pdata->low_batt_cb) {
			wake_lock(&chip->fuel_alert_wake_lock);
			chip->pdata->low_batt_cb();
			chip->is_fuel_alerted = true;

			dev_info(&chip->client->dev,
				"fuel alert activated by polling check (raw:%d)\n",
				chip->raw_soc);
		} else
			dev_info(&chip->client->dev,
				"fuel alert already activated (raw:%d)\n",
				chip->raw_soc);
	} else if (chip->raw_soc >= chip->fuel_alert_soc) {
		if (chip->is_fuel_alerted) {
			wake_unlock(&chip->fuel_alert_wake_lock);
			chip->is_fuel_alerted = false;

			dev_info(&chip->client->dev,
				"fuel alert deactivated by polling check (raw:%d)\n",
				chip->raw_soc);
		}
	}

#ifdef LOG_REG_FOR_MAXIM
	{
		int reg;
		int i;
		u8	data[2];
		u8	buf[1024];

		i = 0;
		for (reg = 0; reg < 0x50; reg++) {
			max17042_read_reg(chip->client, reg, data);
			i += sprintf(buf + i, "0x%02x%02xh,", data[1], data[0]);
		}
		printk(KERN_INFO "%s", buf);

		i = 0;
		for (reg = 0xe0; reg < 0x100; reg++) {
			max17042_read_reg(chip->client, reg, data);
			i += sprintf(buf + i, "0x%02x%02xh,", data[1], data[0]);
		}
		printk(KERN_INFO "%s", buf);
	}
#endif

	schedule_delayed_work(&chip->work, MAX17042_LONG_DELAY);
}

static enum power_supply_property max17042_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

#define SEC_FG_ATTR(_name)			\
{						\
	.attr = { .name = #_name,		\
		  .mode = 0664 },		\
	.show = sec_fg_show_property,		\
	.store = sec_fg_store,			\
}

static struct device_attribute sec_fg_attrs[] = {
	SEC_FG_ATTR(fg_reset_soc),
	SEC_FG_ATTR(fg_read_soc),
};

enum {
	FG_RESET_SOC = 0,
	FG_READ_SOC,
};

static ssize_t sec_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max17042_chip *chip = container_of(psy,
						  struct max17042_chip,
						  battery);

	int i = 0;
	const ptrdiff_t off = attr - sec_fg_attrs;

	switch (off) {
	case FG_READ_SOC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			max17042_read_vfsoc(chip->client));
		break;
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_fg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max17042_chip *chip = container_of(psy,
						  struct max17042_chip,
						  battery);

	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - sec_fg_attrs;

	switch (off) {
	case FG_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1)
				max17042_reset_soc(chip->client);
			ret = count;
		}
#if 1
/* tester requested to sustain unity with parental models but
	keep source code for later */
		{
			struct power_supply *psy =
				power_supply_get_by_name("battery");
			union power_supply_propval value;

			if (!psy) {
				pr_err("%s: fail to get battery ps\n",
					__func__);
				return -ENODEV;
			}

			value.intval = 0;	/* dummy value */
			psy->set_property(psy,
				POWER_SUPPLY_PROP_CAPACITY, &value);
		}
#endif
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int fuelgauge_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_fg_attrs); i++) {
		rc = device_create_file(dev, &sec_fg_attrs[i]);
		if (rc)
			goto fg_attrs_failed;
	}
	goto succeed;

fg_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_fg_attrs[i]);
succeed:
	return rc;
}

static bool max17042_check_status(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	bool ret = false;

	if (chip->is_enable) {
		/* check if Smn was generated */
		if (max17042_read_reg(client, MAX17042_REG_STATUS, data) < 0)
			return ret;

		dev_info(&client->dev, "%s : status_reg(%02x%02x)\n",
				__func__, data[1], data[0]);

		/* minimum SOC threshold exceeded. */
		if (data[1] & (0x1 << 2))
			ret = true;

		/* clear status reg */
		if (!ret) {
			data[1] = 0;
			max17042_write_reg(client, MAX17042_REG_STATUS, data);
			msleep(200);
		}
	} else
		ret = true;

	return ret;
}

static irqreturn_t max17042_irq_thread(int irq, void *irq_data)
{
	u8 data[2];
	bool max17042_alert_status = false;
	struct max17042_chip *chip = irq_data;

	/* update SOC */
	max17042_get_soc(chip->client);

	max17042_alert_status = max17042_check_status(chip->client);

	if (max17042_alert_status /* && !(chip->is_fuel_alerted)*/) {
		if (max17042_read_reg(chip->client, MAX17042_REG_CONFIG, data)
			< 0)
			return IRQ_HANDLED;

		data[1] |= (0x1 << 3);

		if (chip->pdata->low_batt_cb && !(chip->is_fuel_alerted)) {
			wake_lock(&chip->fuel_alert_wake_lock);
			chip->pdata->low_batt_cb();
			chip->is_fuel_alerted = true;
		} else
			dev_err(&chip->client->dev,
				"failed to call low_batt_cb()\n");

		max17042_write_reg(chip->client, MAX17042_REG_CONFIG, data);

		dev_info(&chip->client->dev,
			"%s : low batt alerted!! config_reg(%02x%02x)\n",
			__func__, data[1], data[0]);
	} else if (!max17042_alert_status /* && (chip->is_fuel_alerted)*/) {
		if (chip->is_fuel_alerted)
			wake_unlock(&chip->fuel_alert_wake_lock);
		chip->is_fuel_alerted = false;

		if (max17042_read_reg(chip->client, MAX17042_REG_CONFIG, data)
			< 0)
			return IRQ_HANDLED;

		data[1] &= (~(0x1 << 3));

		max17042_write_reg(chip->client, MAX17042_REG_CONFIG, data);

		dev_info(&chip->client->dev,
			"%s : low batt released!! config_reg(%02x%02x)\n",
			__func__, data[1], data[0]);
	}

	max17042_read_reg(chip->client, MAX17042_REG_VCELL, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_VCELL(%02x%02x)\n",
		__func__, data[1], data[0]);

	max17042_read_reg(chip->client, MAX17042_REG_TEMPERATURE, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_TEMPERATURE(%02x%02x)\n",
		__func__, data[1], data[0]);

	max17042_read_reg(chip->client, MAX17042_REG_CONFIG, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_CONFIG(%02x%02x)\n",
		__func__, data[1], data[0]);

	max17042_read_reg(chip->client, MAX17042_REG_VFOCV, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_VFOCV(%02x%02x)\n",
		__func__, data[1], data[0]);

	max17042_read_reg(chip->client, MAX17042_REG_SOC_VF, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_SOC_VF(%02x%02x)\n",
		__func__, data[1], data[0]);

	dev_err(&chip->client->dev,
		"%s : PMIC IRQ (%d), FUEL GAUGE IRQ (%d)\n",
		__func__, gpio_get_value(GPIO_PMIC_IRQ),
		gpio_get_value(chip->pdata->alert_gpio));

#if 0
	**max17042_read_reg(chip->client, MAX17042_REG_STATUS, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_STATUS(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_VALRT_TH, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_VALRT_TH(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_TALRT_TH, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_TALRT_TH(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_SALRT_TH, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_SALRT_TH(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_AVGVCELL, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_AVGVCELL(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_VERSION, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_VERSION(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_LEARNCFG, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_LEARNCFG(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_MISCCFG, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_MISCCFG(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_CGAIN, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_CGAIN(%02x%02x)\n",
		__func__, data[1], data[0]);

	**max17042_read_reg(chip->client, MAX17042_REG_RCOMP, data);
	dev_info(&chip->client->dev, "%s : MAX17042_REG_RCOMP(%02x%02x)\n",
		__func__, data[1], data[0]);
#endif

	return IRQ_HANDLED;
}

static int max17042_irq_init(struct max17042_chip *chip)
{
	int ret;
	u8 data[2];

	/* 1. Set max17042 alert configuration. */
	max17042_alert_init(chip->client);

	chip->is_fuel_alerted = false;

	/* 2. Request irq */
	if (chip->pdata->alert_irq) {
		ret = request_threaded_irq(chip->pdata->alert_irq, NULL,
			max17042_irq_thread, IRQF_TRIGGER_LOW | IRQF_ONESHOT,
			"max17042 fuel alert", chip);
		if (ret) {
			dev_err(&chip->client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(chip->pdata->alert_irq);
		if (ret < 0)
			dev_err(&chip->client->dev,
				"failed to enable wakeup src %d\n", ret);
	}

	if (max17042_read_reg(chip->client, MAX17042_REG_CONFIG, data)
		< 0)
		return -1;

	/*Enable Alert (Aen = 1) */
	data[0] |= (0x1 << 2);

	max17042_write_reg(chip->client, MAX17042_REG_CONFIG, data);

	dev_info(&chip->client->dev, "%s : config_reg(%02x%02x) irq(%d)\n",
		__func__, data[1], data[0], chip->pdata->alert_irq);

	return 0;
}

static int __devinit max17042_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17042_chip *chip;
	int i;
	struct max17042_reg_data *data;
	int ret;
	u8 i2c_data[2];

	dev_info(&client->dev, "%s: MAX17042 Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "fuelgauge";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17042_get_property;
	chip->battery.set_property	= max17042_set_property;
	chip->battery.properties	= max17042_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17042_battery_props);

#ifdef RECAL_SOC_FOR_MAXIM
	chip->cnt = 0;
	chip->recalc_180s = 0;
	chip->boot_cnt = 0;
#endif

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

	if (max17042_read_reg(client, MAX17042_REG_VERSION, i2c_data) < 0)
		chip->is_enable = false;
	else
		chip->is_enable = true;

	dev_info(&client->dev, "%s : is enable (%d)\n",
		__func__, chip->is_enable);

	/* initialize fuel gauge registers */
	max17042_init_regs(client);

	if (chip->is_enable) {
		/* register low batt intr */
		chip->pdata->alert_irq = gpio_to_irq(chip->pdata->alert_gpio);

		wake_lock_init(&chip->fuel_alert_wake_lock, WAKE_LOCK_SUSPEND,
			       "fuel_alerted");

		data = chip->pdata->alert_init;
		for (i = 0; i < chip->pdata->alert_init_size; i += 3)
			if ((data + i)->reg_addr ==
				MAX17042_REG_SALRT_TH)
				chip->fuel_alert_soc =
				(data + i)->reg_data1;

		dev_info(&client->dev, "fuel alert soc (%d)\n",
			chip->fuel_alert_soc);

		ret = max17042_irq_init(chip);
		if (ret)
			goto err_kfree;
	}

	max17042_get_version(client);

	/* create fuelgauge attributes */
	fuelgauge_create_attrs(chip->battery.dev);

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17042_work);
	schedule_delayed_work(&chip->work, MAX17042_SHORT_DELAY);

	dev_info(&client->dev, "%s: MAX17042 Driver Loaded\n", __func__);

	return 0;

err_kfree:
	wake_lock_destroy(&chip->fuel_alert_wake_lock);
	kfree(chip);
	return ret;
}

static int __devexit max17042_remove(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	wake_lock_destroy(&chip->fuel_alert_wake_lock);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17042_suspend(struct i2c_client *client, pm_message_t state)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int max17042_resume(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	schedule_delayed_work(&chip->work, MAX17042_SHORT_DELAY);
	return 0;
}

#else

#define max17042_suspend NULL
#define max17042_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17042_id[] = {
	{"max17042", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, max17042_id);

static struct i2c_driver max17042_i2c_driver = {
	.driver	= {
		.name	= "max17042",
	},
	.probe		= max17042_probe,
	.remove		= __devexit_p(max17042_remove),
	.suspend	= max17042_suspend,
	.resume		= max17042_resume,
	.id_table	= max17042_id,
};

static int __init max17042_init(void)
{
	return i2c_add_driver(&max17042_i2c_driver);
}

module_init(max17042_init);

static void __exit max17042_exit(void)
{
	i2c_del_driver(&max17042_i2c_driver);
}

module_exit(max17042_exit);

MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_DESCRIPTION("MAX17042 Fuel Gauge");
MODULE_LICENSE("GPL");
