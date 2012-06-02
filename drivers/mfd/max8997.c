/*
 * max8997.c - mfd core driver for the Maxim 8997
 *
 *  Copyright (C) 2009-2010 Samsung Electronics
 *
 *  based on max8998.c
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
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>

#define RTC_I2C_ADDR		(0x0c >> 1)
#define MUIC_I2C_ADDR		(0x4a >> 1)
#define HMOTOR_I2C_ADDR		(0x90 >> 1)

static struct mfd_cell max8997_devs[] = {
	{
		.name = "max8997-pmic",
	}, {
		.name = "max8997-charger",
	}, {
		.name = "max8997-rtc",
	}, {
		.name = "max8997-muic",
	}, {
		.name = "max8997-hapticmotor",
	},
};

int max8997_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8997->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max8997->iolock);
	if (ret < 0)
		return ret;

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL(max8997_read_reg);

int max8997_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8997->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max8997->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(max8997_bulk_read);

int max8997_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8997->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max8997->iolock);
	return ret;
}
EXPORT_SYMBOL(max8997_write_reg);

int max8997_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8997->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max8997->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(max8997_bulk_write);

int max8997_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8997->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max8997->iolock);
	return ret;
}
EXPORT_SYMBOL(max8997_update_reg);

static int max8997_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct max8997_platform_data *pdata = i2c->dev.platform_data;
	struct max8997_dev *max8997;
	int ret = 0;

	max8997 = kzalloc(sizeof(struct max8997_dev), GFP_KERNEL);
	if (max8997 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, max8997);
	max8997->dev = &i2c->dev;
	max8997->i2c = i2c;
	max8997->irq = i2c->irq;
	max8997->type = id->driver_data;
	if (pdata) {
		max8997->ono = pdata->ono;
		max8997->irq_base = pdata->irq_base;
		max8997->wakeup = pdata->wakeup;
	}
	mutex_init(&max8997->iolock);

	max8997->rtc = i2c_new_dummy(i2c->adapter, RTC_I2C_ADDR);
	i2c_set_clientdata(max8997->rtc, max8997);

	max8997->muic = i2c_new_dummy(i2c->adapter, MUIC_I2C_ADDR);
	i2c_set_clientdata(max8997->muic, max8997);

	max8997->hmotor = i2c_new_dummy(i2c->adapter, HMOTOR_I2C_ADDR);
	i2c_set_clientdata(max8997->hmotor, max8997);

	max8997_irq_init(max8997);

	ret = mfd_add_devices(max8997->dev, -1,
			      max8997_devs, ARRAY_SIZE(max8997_devs),
			      NULL, 0);
	if (ret < 0)
		goto err;


	return ret;

err:
	mfd_remove_devices(max8997->dev);
	max8997_irq_exit(max8997);
	i2c_unregister_device(max8997->rtc);
	kfree(max8997);
	return ret;
}

static int max8997_i2c_remove(struct i2c_client *i2c)
{
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max8997->dev);
	max8997_irq_exit(max8997);
	i2c_unregister_device(max8997->rtc);
	kfree(max8997);

	return 0;
}

static const struct i2c_device_id max8997_i2c_id[] = {
	{ "max8997", TYPE_MAX8997 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max8997_i2c_id);

#ifdef CONFIG_PM
static int max8997_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);

	if (max8997->wakeup)
		enable_irq_wake(max8997->irq);

	disable_irq(max8997->irq);

	return 0;
}

static int max8997_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max8997_dev *max8997 = i2c_get_clientdata(i2c);

	if (max8997->wakeup)
		disable_irq_wake(max8997->irq);

	enable_irq(max8997->irq);

	return 0;
}
#else
#define max8997_suspend		NULL
#define max8997_resume		NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops max8997_pm = {
	.suspend = max8997_suspend,
	.resume = max8997_resume,
};

static struct i2c_driver max8997_i2c_driver = {
	.driver = {
		   .name = "max8997",
		   .owner = THIS_MODULE,
		   .pm = &max8997_pm,
	},
	.probe = max8997_i2c_probe,
	.remove = max8997_i2c_remove,
	.id_table = max8997_i2c_id,
};

static int __init max8997_i2c_init(void)
{
	return i2c_add_driver(&max8997_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max8997_i2c_init);

static void __exit max8997_i2c_exit(void)
{
	i2c_del_driver(&max8997_i2c_driver);
}
module_exit(max8997_i2c_exit);

MODULE_DESCRIPTION("MAXIM 8997 multi-function core driver");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
