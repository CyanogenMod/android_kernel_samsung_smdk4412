/*
 * max8952.c - Voltage and current regulation for the Maxim 8952
 *
 * Copyright (C) 2010 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/max8952.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/slab.h>

/* Registers */
enum {
	MAX8952_REG_MODE0,
	MAX8952_REG_MODE1,
	MAX8952_REG_MODE2,
	MAX8952_REG_MODE3,
	MAX8952_REG_CONTROL,
	MAX8952_REG_SYNC,
	MAX8952_REG_RAMP,
	MAX8952_REG_CHIP_ID1,
	MAX8952_REG_CHIP_ID2,
};

struct max8952_data {
	struct i2c_client	*client;
	struct device		*dev;
	struct mutex		mutex;
	struct max8952_platform_data *pdata;
	struct regulator_dev	*rdev;

	bool vid0;
	bool vid1;
	bool en;
};

static int max8952_read_reg(struct max8952_data *max8952, u8 reg)
{
	int ret = i2c_smbus_read_byte_data(max8952->client, reg);
	if (ret > 0)
		ret &= 0xff;

	return ret;
}

static int max8952_write_reg(struct max8952_data *max8952,
		u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(max8952->client, reg, value);
}

static int max8952_voltage(struct max8952_data *max8952, u8 mode)
{
	return (max8952->pdata->dvs_mode[mode] * 10 + 770) * 1000;
}

static int max8952_list_voltage(struct regulator_dev *rdev,
		unsigned int selector)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);

	if (rdev_get_id(rdev) != 0)
		return -EINVAL;

	return max8952_voltage(max8952, selector);
}

static int max8952_is_enabled(struct regulator_dev *rdev)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);
	return max8952->en;
}

static int max8952_enable(struct regulator_dev *rdev)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);

	/* If not valid, assume "ALWAYS_HIGH" */
	if (gpio_is_valid(max8952->pdata->gpio_en))
		gpio_set_value(max8952->pdata->gpio_en, 1);

	max8952->en = true;
	return 0;
}

static int max8952_disable(struct regulator_dev *rdev)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);

	/* If not valid, assume "ALWAYS_HIGH" -> not permitted */
	if (gpio_is_valid(max8952->pdata->gpio_en))
		gpio_set_value(max8952->pdata->gpio_en, 0);
	else
		return -EPERM;

	max8952->en = false;
	return 0;
}

static int max8952_get_voltage(struct regulator_dev *rdev)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);
	u8 vid = 0;

	if (max8952->vid0)
		vid += 1;
	if (max8952->vid1)
		vid += 2;

	return max8952_voltage(max8952, vid);
}

static int max8952_set_voltage(struct regulator_dev *rdev,
			       int min_uV, int max_uV, unsigned *selector)
{
	struct max8952_data *max8952 = rdev_get_drvdata(rdev);
	s8 vid = -1, i;

	if (!gpio_is_valid(max8952->pdata->gpio_vid0) ||
			!gpio_is_valid(max8952->pdata->gpio_vid1)) {
		/* DVS not supported */
		return -EPERM;
	}

	for (i = 0; i < MAX8952_NUM_DVS_MODE; i++) {
		int volt = max8952_voltage(max8952, i);

		/* Set the voltage as low as possible within the range */
		if (volt <= max_uV && volt >= min_uV)
			if (vid == -1 || max8952_voltage(max8952, vid) > volt)
				vid = i;
	}

	if (vid >= 0 && vid < MAX8952_NUM_DVS_MODE) {
		max8952->vid0 = (vid % 2 == 1);
		max8952->vid1 = (((vid >> 1) % 2) == 1);
		*selector = vid;
		gpio_set_value(max8952->pdata->gpio_vid0, max8952->vid0);
		gpio_set_value(max8952->pdata->gpio_vid1, max8952->vid1);
	} else
		return -EINVAL;

	return 0;
}

static struct regulator_ops max8952_ops = {
	.list_voltage		= max8952_list_voltage,
	.is_enabled		= max8952_is_enabled,
	.enable			= max8952_enable,
	.disable		= max8952_disable,
	.get_voltage		= max8952_get_voltage,
	.set_voltage		= max8952_set_voltage,
	.set_suspend_disable	= max8952_disable,
};

static struct regulator_desc regulator = {
	.name		= "MAX8952_VOUT",
	.id		= 0,
	.n_voltages	= MAX8952_NUM_DVS_MODE,
	.ops		= &max8952_ops,
	.type		= REGULATOR_VOLTAGE,
	.owner		= THIS_MODULE,
};

static int __devinit max8952_pmic_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max8952_platform_data *pdata = client->dev.platform_data;
	struct max8952_data *max8952;

	int ret = 0, err = 0;
	int readbyte;

	max8952 = kzalloc(sizeof(struct max8952_data), GFP_KERNEL);
	if (!max8952)
		return -ENOMEM;

	max8952->client = client;
	max8952->dev = &client->dev;
	max8952->pdata = pdata;
	mutex_init(&max8952->mutex);

	max8952_write_reg(max8952, MAX8952_REG_MODE3, 0x2B);
	readbyte = max8952_read_reg(max8952, MAX8952_REG_MODE3);
	printk(KERN_DEBUG "%s, readbyte : 0x%2x\n", __func__, readbyte);

	i2c_set_clientdata(client, max8952);

	return 0;

err_reg:
	kfree(max8952);
	return ret;
}

static int __devexit max8952_pmic_remove(struct i2c_client *client)
{
	struct max8952_data *max8952 = i2c_get_clientdata(client);
	struct max8952_platform_data *pdata = max8952->pdata;
	struct regulator_dev *rdev = max8952->rdev;

	kfree(max8952);
	return 0;
}

static const struct i2c_device_id max8952_ids[] = {
	{ "max8952_grande", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max8952_ids);

static struct i2c_driver max8952_pmic_driver = {
	.probe		= max8952_pmic_probe,
	.remove		= __devexit_p(max8952_pmic_remove),
	.driver		= {
		.name	= "max8952_grande",
	},
	.id_table	= max8952_ids,
};

static int __init max8952_pmic_init(void)
{
	return i2c_add_driver(&max8952_pmic_driver);
}
subsys_initcall(max8952_pmic_init);

static void __exit max8952_pmic_exit(void)
{
	i2c_del_driver(&max8952_pmic_driver);
}
module_exit(max8952_pmic_exit);

MODULE_DESCRIPTION("MAXIM 8952 voltage regulator driver");
MODULE_AUTHOR("MyungJoo Ham <myungjoo.ham@samsung.com>");
MODULE_LICENSE("GPL");
