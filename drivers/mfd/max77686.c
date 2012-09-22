/*
 * max77686.c - mfd core driver for the Maxim 77686
 *
 * Copyright (C) 2011 Samsung Electronics
 * Chiwoong Byun <woong.byun@smasung.com>
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
 *
 * This driver is based on max8997.c
 */

#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77686.h>
#include <linux/mfd/max77686-private.h>
#include <linux/interrupt.h>

#define I2C_ADDR_RTC	(0x0C >> 1)

static struct mfd_cell max77686_devs[] = {
	{ .name = "max77686-pmic", },
#ifdef CONFIG_RTC_DRV_MAX77686
	{ .name = "max77686-rtc", },
#endif
};

int max77686_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77686->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77686->iolock);
	if (ret < 0)
		return ret;

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77686_read_reg);

int max77686_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77686->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77686->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77686_bulk_read);

int max77686_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77686->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max77686->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77686_write_reg);

int max77686_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77686->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77686->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77686_bulk_write);

int max77686_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77686->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77686->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77686_update_reg);

static int max77686_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max77686_dev *max77686;
	struct max77686_platform_data *pdata = i2c->dev.platform_data;
	u8 data;
	int ret = 0;

	max77686 = kzalloc(sizeof(struct max77686_dev), GFP_KERNEL);
	if (max77686 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, max77686);
	max77686->dev = &i2c->dev;
	max77686->i2c = i2c;
	max77686->type = id->driver_data;

	if (!pdata) {
		ret = -EIO;
		goto err;
	}

	max77686->wakeup = pdata->wakeup;
	max77686->irq_gpio = pdata->irq_gpio;
	max77686->irq_base = pdata->irq_base;
	max77686->wtsr_smpl = pdata->wtsr_smpl;

	mutex_init(&max77686->iolock);

	if (max77686_read_reg(i2c, MAX77686_REG_DEVICE_ID, &data) < 0) {
		dev_err(max77686->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err;
	} else
		dev_info(max77686->dev, "device found\n");

#ifdef CONFIG_RTC_DRV_MAX77686
	max77686->rtc = i2c_new_dummy(i2c->adapter, I2C_ADDR_RTC);
	i2c_set_clientdata(max77686->rtc, max77686);
#endif

	max77686_irq_init(max77686);

	ret = mfd_add_devices(max77686->dev, -1, max77686_devs,
			      ARRAY_SIZE(max77686_devs), NULL, 0);

	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77686->dev, pdata->wakeup);

	return ret;

err_mfd:
	mfd_remove_devices(max77686->dev);
#ifdef CONFIG_RTC_DRV_MAX77686
	i2c_unregister_device(max77686->rtc);
#endif
err:
	kfree(max77686);
	return ret;
}

static int max77686_i2c_remove(struct i2c_client *i2c)
{
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77686->dev);
#ifdef CONFIG_RTC_DRV_MAX77686
	i2c_unregister_device(max77686->rtc);
#endif
	kfree(max77686);

	return 0;
}

static const struct i2c_device_id max77686_i2c_id[] = {
	{ "max77686", TYPE_MAX77686 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77686_i2c_id);

#ifdef CONFIG_PM
static int max77686_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);

	disable_irq(max77686->irq);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77686->irq);

	return 0;
}

static int max77686_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77686->irq);

	enable_irq(max77686->irq);

	return max77686_irq_resume(max77686);
}
#else
#define max77686_suspend	NULL
#define max77686_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

u8 max77686_dumpaddr_pmic[] = {
	MAX77686_REG_INT1MSK,
	MAX77686_REG_INT2MSK,
	MAX77686_REG_ONOFF_DELAY,
	MAX77686_REG_MRSTB	,
	/* Reserved: 0x0B-0x0F */
	MAX77686_REG_BUCK1CTRL,
	MAX77686_REG_BUCK1OUT,
	MAX77686_REG_BUCK2CTRL1,
	MAX77686_REG_BUCK234FREQ,
	MAX77686_REG_BUCK2DVS1,
	MAX77686_REG_BUCK2DVS2,
	MAX77686_REG_BUCK2DVS3,
	MAX77686_REG_BUCK2DVS4,
	MAX77686_REG_BUCK2DVS5,
	MAX77686_REG_BUCK2DVS6,
	MAX77686_REG_BUCK2DVS7,
	MAX77686_REG_BUCK2DVS8,
	MAX77686_REG_BUCK3CTRL1,
	/* Reserved: 0x1D */
	MAX77686_REG_BUCK3DVS1,
	MAX77686_REG_BUCK3DVS2,
	MAX77686_REG_BUCK3DVS3,
	MAX77686_REG_BUCK3DVS4,
	MAX77686_REG_BUCK3DVS5,
	MAX77686_REG_BUCK3DVS6,
	MAX77686_REG_BUCK3DVS7,
	MAX77686_REG_BUCK3DVS8,
	MAX77686_REG_BUCK4CTRL1,
	/* Reserved: 0x27 */
	MAX77686_REG_BUCK4DVS1,
	MAX77686_REG_BUCK4DVS2,
	MAX77686_REG_BUCK4DVS3,
	MAX77686_REG_BUCK4DVS4,
	MAX77686_REG_BUCK4DVS5,
	MAX77686_REG_BUCK4DVS6,
	MAX77686_REG_BUCK4DVS7,
	MAX77686_REG_BUCK4DVS8,
	MAX77686_REG_BUCK5CTRL,
	MAX77686_REG_BUCK5OUT,
	MAX77686_REG_BUCK6CTRL,
	MAX77686_REG_BUCK6OUT,
	MAX77686_REG_BUCK7CTRL,
	MAX77686_REG_BUCK7OUT,
	MAX77686_REG_BUCK8CTRL,
	MAX77686_REG_BUCK8OUT,
	MAX77686_REG_BUCK9CTRL,
	MAX77686_REG_BUCK9OUT,
	/* Reserved: 0x3A-0x3F */
	MAX77686_REG_LDO1CTRL1	,
	MAX77686_REG_LDO2CTRL1	,
	MAX77686_REG_LDO3CTRL1	,
	MAX77686_REG_LDO4CTRL1	,
	MAX77686_REG_LDO5CTRL1	,
	MAX77686_REG_LDO6CTRL1,
	MAX77686_REG_LDO7CTRL1	,
	MAX77686_REG_LDO8CTRL1	,
	MAX77686_REG_LDO9CTRL1	,
	MAX77686_REG_LDO10CTRL1,
	MAX77686_REG_LDO11CTRL1,
	MAX77686_REG_LDO12CTRL1,
	MAX77686_REG_LDO13CTRL1,
	MAX77686_REG_LDO14CTRL1,
	MAX77686_REG_LDO15CTRL1,
	MAX77686_REG_LDO16CTRL1,
	MAX77686_REG_LDO17CTRL1,
	MAX77686_REG_LDO18CTRL1,
	MAX77686_REG_LDO19CTRL1,
	MAX77686_REG_LDO20CTRL1,
	MAX77686_REG_LDO21CTRL1,
	MAX77686_REG_LDO22CTRL1,
	MAX77686_REG_LDO23CTRL1,
	MAX77686_REG_LDO24CTRL1,
	MAX77686_REG_LDO25CTRL1,
	MAX77686_REG_LDO26CTRL1,
	/* Reserved: 0x5A-0x5F */
	MAX77686_REG_LDO1CTRL2	,
	MAX77686_REG_LDO2CTRL2	,
	MAX77686_REG_LDO3CTRL2	,
	MAX77686_REG_LDO4CTRL2	,
	MAX77686_REG_LDO5CTRL2	,
	MAX77686_REG_LDO6CTRL2,
	MAX77686_REG_LDO7CTRL2	,
	MAX77686_REG_LDO8CTRL2	,
	MAX77686_REG_LDO9CTRL2	,
	MAX77686_REG_LDO10CTRL2,
	MAX77686_REG_LDO11CTRL2,
	MAX77686_REG_LDO12CTRL2,
	MAX77686_REG_LDO13CTRL2,
	MAX77686_REG_LDO14CTRL2,
	MAX77686_REG_LDO15CTRL2,
	MAX77686_REG_LDO16CTRL2,
	MAX77686_REG_LDO17CTRL2,
	MAX77686_REG_LDO18CTRL2,
	MAX77686_REG_LDO19CTRL2,
	MAX77686_REG_LDO20CTRL2,
	MAX77686_REG_LDO21CTRL2,
	MAX77686_REG_LDO22CTRL2,
	MAX77686_REG_LDO23CTRL2,
	MAX77686_REG_LDO24CTRL2,
	MAX77686_REG_LDO25CTRL2,
	MAX77686_REG_LDO26CTRL2,
	MAX77686_REG_BBAT_CHG,
	MAX77686_REG_32KHZ,
};

static int max77686_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77686_dumpaddr_pmic); i++)
		max77686_read_reg(i2c, max77686_dumpaddr_pmic[i],
				&max77686->reg_dump[i]);

	disable_irq(max77686->irq);

	return 0;
}

static int max77686_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77686_dev *max77686 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77686->irq);

	for (i = 0; i < ARRAY_SIZE(max77686_dumpaddr_pmic); i++)
		max77686_write_reg(i2c, max77686_dumpaddr_pmic[i],
				max77686->reg_dump[i]);

	return 0;
}
#endif

const struct dev_pm_ops max77686_pm = {
	.suspend = max77686_suspend,
	.resume = max77686_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77686_freeze,
	.thaw = max77686_restore,
	.restore = max77686_restore,
#endif
};

static struct i2c_driver max77686_i2c_driver = {
	.driver = {
		.name = "max77686",
		.owner = THIS_MODULE,
		.pm = &max77686_pm,
	},
	.probe = max77686_i2c_probe,
	.remove = max77686_i2c_remove,
	.id_table = max77686_i2c_id,
};

static int __init max77686_i2c_init(void)
{
	return i2c_add_driver(&max77686_i2c_driver);
}
/* init early so consumer devices can complete system boot */
#ifdef CONFIG_FAST_RESUME
beforeresume_initcall(max77686_i2c_init);
#else
subsys_initcall(max77686_i2c_init);
#endif

static void __exit max77686_i2c_exit(void)
{
	i2c_del_driver(&max77686_i2c_driver);
}
module_exit(max77686_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77686 multi-function core driver");
MODULE_AUTHOR("Chiwoong Byun <woong.byun@samsung.com>");
MODULE_LICENSE("GPL");
