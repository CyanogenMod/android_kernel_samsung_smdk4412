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
	int config;
	int rcomp;
	int status;
	int temperature;
	bool is_fuel_alerted;	/* fuel alerted */
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
		val->intval = chip->temperature / 100; /* result unit 0.1'C */
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
	u8 data[2];
	/*struct max17042_platform_data *pdata = client->dev.platform_data; */

	dev_info(&client->dev, "%s\n", __func__);

/*	max17042_write_reg_array(client, pdata->init,
		pdata->init_size);*/

	if (max17042_read_reg(client, MAX17042_REG_FILTERCFG, data) < 0)
		return;

	/* Clear average vcell (12 sec) */
	data[0] &= 0x8f;

	max17042_write_reg(client, MAX17042_REG_FILTERCFG, data);
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

	if (max17042_read_reg(client, MAX17042_REG_VFOCV, data) < 0)
		return -1;

	vfocv = ((data[0] >> 3) + (data[1] << 5)) * 625 / 1000;

	chip->vfocv = vfocv;

	return vfocv;
}

static int max17042_read_vfsoc(struct i2c_client *client)
{
	u8 data[2];
	u32 vfsoc = 0;

	if (max17042_read_reg(client, MAX17042_REG_SOC_VF, data) < 0)
		return -1;

	vfsoc = data[1];

	return vfsoc;
}

static void max17042_reset_soc(struct i2c_client *client)
{
	u8 data[2];

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

#if 0
	data[0] = 0x0F;
	data[1] = 0x00;
	max17042_write_reg(client, 0x60, data);
#endif

	return;
}

static void max17042_get_vcell(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (max17042_read_reg(client, MAX17042_REG_VCELL, data) < 0)
		return;

	chip->vcell = ((data[0] >> 3) + (data[1] << 5)) * 625;

	if (max17042_read_reg(client, MAX17042_REG_AVGVCELL, data) < 0)
		return;

	chip->avgvcell = ((data[0] >> 3) + (data[1] << 5)) * 625;

	/* printk(KERN_ERR "%s : vcell = %dmV\n", __func__, chip->vcell); */
}

static void max17042_get_config(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (max17042_read_reg(client, MAX17042_REG_CONFIG, data) < 0)
		return;

	chip->config = (data[1] << 8) | data[0];

	/* printk(KERN_ERR "%s : %x, %x, config = 0x%x\n", __func__,
			data[1], data[0], chip->config); */
}

static void max17042_get_rcomp(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (max17042_read_reg(client, MAX17042_REG_RCOMP, data) < 0)
		return;

	chip->rcomp = (data[1] << 8) | data[0];

	/* printk(KERN_ERR "%s : %x, %x, rcomp = 0x%x\n", __func__,
			 data[1], data[0], chip->rcomp); */
}

static void max17042_set_rcomp(struct i2c_client *client, u16 rcomp)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	data[0] = rcomp & 0xff;
	data[1] = rcomp >> 8;
	if (max17042_write_reg(client, MAX17042_REG_RCOMP, data) < 0)
		return;
}

static void max17042_get_status(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	if (max17042_read_reg(client, MAX17042_REG_STATUS, data) < 0)
		return;

	chip->status = (data[1] << 8) | data[0];

	/* printk(KERN_ERR "%s : %x, %x = status = 0x%x\n", __func__,
			 data[1], data[0], chip->status); */
}

static void max17042_get_soc(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	unsigned int soc, psoc;
	int temp_soc;
	static int fg_zcount;

	if (max17042_read_reg(client, MAX17042_REG_SOC_VF, data) < 0)
		return;

	psoc = (data[1]*100)+(data[0]*100/256);
	chip->raw_soc = psoc;

	if (psoc > 100) {
		temp_soc = ((psoc-60)*10000)/9700;
		/* under 160(psoc), 0% */
	} else
		temp_soc = 0;

	soc = temp_soc/100;
	soc = min(soc, (uint)100);

	if (soc == 0) {
		fg_zcount++;
		if (fg_zcount >= 3) {
			/* pr_info("[fg] real 0%\n"); */
			soc = 0;
			fg_zcount = 0;
		} else
			soc = chip->soc;
	} else
		fg_zcount = 0;

	chip->soc = soc;

	/* pr_info("soc = %d%%, raw_soc = %d\n",
			 chip->soc, chip->raw_soc); */
}

static void max17042_get_temperature(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	s32 temper = 0;

	if (max17042_read_reg(client, MAX17042_REG_TEMPERATURE, data) < 0)
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

	chip->temperature = temper;
}

static void max17042_get_version(struct i2c_client *client)
{
	u8 data[2];

	if (max17042_read_reg(client, MAX17042_REG_VERSION, data) < 0)
		return;

	dev_info(&client->dev, "MAX17042 Fuel-Gauge Ver %d%d\n",
			data[0], data[1]);
}

static void max17042_disable_intr(struct i2c_client *client)
{
	u8 data[2];

	pr_info("%s :\n", __func__);

	if (max17042_read_reg(client, MAX17042_REG_CONFIG, data) < 0) {
		dev_err(&client->dev, "%s : fail to read reg\n", __func__);
		return;
	}
	data[0] &= (~(0x1 << 2));

	max17042_write_reg(client, MAX17042_REG_CONFIG, data);
}

static void max17042_enable_intr(struct i2c_client *client)
{
	u8 data[2];

	pr_info("%s :\n", __func__);

	if (max17042_read_reg(client, MAX17042_REG_CONFIG, data) < 0) {
		dev_err(&client->dev, "%s : fail to read reg\n", __func__);
		return;
	}
	data[0] |= (0x1 << 2);

	max17042_write_reg(client, MAX17042_REG_CONFIG, data);
}

static void max17042_clear_intr(struct i2c_client *client)
{
	u8 data[2];

	pr_info("%s :\n", __func__);

	if (max17042_read_reg(client, MAX17042_REG_STATUS, data) < 0) {
		dev_err(&client->dev, "%s : fail to read reg\n", __func__);
		return;
	}

	data[1] &= 0x88;
	max17042_write_reg(client, MAX17042_REG_STATUS, data);
	msleep(200);
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
		if (chip->pdata->enable_gauging_temperature) {
			data[0] = 0;
			data[1] = val->intval;
			/*
			pr_info("MAX17042"
				"Temperature(write) = %d\n",
				data[1]);
			*/
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
	max17042_get_status(chip->client);

	if (chip->pdata->enable_gauging_temperature)
		max17042_get_temperature(chip->client);

	if ((chip->status&0x02) == 0x02) {
		max17042_get_config(chip->client);
		pr_info("config 0x%x, status 0x%x, vcell 0x%x, row_soc 0x%x\n",
			chip->config, chip->status, chip->vcell, chip->raw_soc);
		/*
		panic("[1]fuel gauge reset occurred!");
		*/
	}

	if (chip->is_fuel_alerted) {
		if (!(chip->status&0x0400)) { /* Smn clear check */
			chip->is_fuel_alerted = false;
			max17042_enable_intr(chip->client);
			max17042_get_config(chip->client);
			pr_info("config 0x%x, status 0x%x\n",
				chip->config, chip->status);
		} else {
			max17042_get_config(chip->client);
			if (chip->config&0x4) /* intr disable check */
				max17042_disable_intr(chip->client);
		}
	}

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
		max17042_get_soc(chip->client);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			chip->soc);
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
	u8 data[2];
	bool ret = false;

	/* check if Smn was generated */
	if (max17042_read_reg(client, MAX17042_REG_STATUS, data) < 0)
		return -1;

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

	return ret;
}

static irqreturn_t max17042_irq_thread(int irq, void *irq_data)
{
	bool max17042_alert_status = false;
	struct max17042_chip *chip = irq_data;

	max17042_get_soc(chip->client);
	max17042_alert_status = max17042_check_status(chip->client);

	if (max17042_alert_status) {
		chip->is_fuel_alerted = true;
		if (chip->pdata->low_batt_cb)
			chip->pdata->low_batt_cb();
		max17042_disable_intr(chip->client);
		max17042_get_config(chip->client);
		max17042_get_status(chip->client);
		pr_info("status = 0x%x\n", chip->status);
	} else {
		printk(KERN_ERR "fuelguage invalid alert!\n");
	}

	dev_info(&chip->client->dev,
		"%s : PMIC IRQ (%d), FUEL GAUGE IRQ (%d),"
		"STATUS (0x%x), CONFIG (0x%x)\n",
		__func__, gpio_get_value(GPIO_PMIC_IRQ),
		gpio_get_value(chip->pdata->alert_gpio),
		chip->status, chip->config);

	msleep(200);

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
	if (chip->client->irq) {
		ret = request_threaded_irq(chip->client->irq, NULL,
				max17042_irq_thread,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				"max17042 fuel alert", chip);
		if (ret) {
			dev_err(&chip->client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(chip->client->irq);
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

	dev_info(&chip->client->dev, "%s : config_reg(%02x%02x)\n",
		__func__, data[1], data[0]);

	return 0;
}

static int __devinit max17042_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17042_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "fuelgauge",
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY,
	chip->battery.get_property	= max17042_get_property,
	chip->battery.set_property	= max17042_set_property,
	chip->battery.properties	= max17042_battery_props,
	chip->battery.num_properties	= ARRAY_SIZE(max17042_battery_props),
	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

	/* initialize fuel gauge registers */
	max17042_init_regs(client);

	/* check rcomp & config */
	max17042_get_rcomp(client);
	max17042_get_config(client);
	pr_info("rcomp = 0x%04x, config = 0x%04x\n",
		chip->rcomp, chip->config);

	if (chip->rcomp != MAX17042_NEW_RCOMP) {
		max17042_set_rcomp(client, MAX17042_NEW_RCOMP);
		pr_info("set new rcomp = 0x%04x\n", MAX17042_NEW_RCOMP);
		max17042_get_rcomp(client);
		pr_info("new rcomp = 0x%04x\n", chip->rcomp);
	}

	/* register low batt intr */
	ret = max17042_irq_init(chip);
	if (ret)
		goto err_kfree;

	max17042_get_version(client);

	/* create fuelgauge attributes */
	fuelgauge_create_attrs(chip->battery.dev);

	max17042_get_status(client);
	if (chip->status & 0x7700) {
		max17042_clear_intr(client);
		max17042_get_status(client);
		pr_info("reset status = 0x%x\n", chip->status);
	}

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17042_work);
	schedule_delayed_work(&chip->work, MAX17042_SHORT_DELAY);

	return 0;

err_kfree:
	kfree(chip);
	return ret;
}

static int __devexit max17042_remove(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
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
