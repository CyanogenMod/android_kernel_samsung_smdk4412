/*
 * max14577.c - mfd core driver for the Maxim 14577
 *
 * Copyright (C) 2011 Samsung Electrnoics
 * SeungJin Hahn <sjin.hahn@samsung.com>
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
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max14577.h>
#include <linux/mfd/max14577-private.h>
#include <linux/regulator/machine.h>

#if defined(CONFIG_USE_MUIC)
#include <linux/muic/muic.h>
#endif /* CONFIG_USE_MUIC */

int max14577_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max14577->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max14577->i2c_lock);
	if (ret < 0)
		return ret;

	ret &= 0xff;
	*dest = ret;
	return 0;
}

int max14577_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max14577->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max14577->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int max14577_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max14577->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max14577->i2c_lock);
	return ret;
}

int max14577_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max14577->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max14577->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int max14577_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max14577->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max14577->i2c_lock);
	return ret;
}

int max14577_set_cdetctrl1_reg(struct i2c_client *i2c, u8 val)
{
	pr_info("%s:%s(0x%02x)\n", MFD_DEV_NAME, __func__, val);

	return max14577_write_reg(i2c, MAX14577_REG_CDETCTRL1, val);
}

int max14577_get_cdetctrl1_reg(struct i2c_client *i2c, u8 *val)
{
	u8 value = 0;
	int ret = 0;

	ret = max14577_read_reg(i2c, MAX14577_REG_CDETCTRL1, &value);
	if (ret)
		*val = 0xff;
	else
		*val = value;

	pr_info("%s:%s(0x%02x), ret(%d)\n", MFD_DEV_NAME, __func__, *val, ret);

	return ret;
}

int max14577_set_control2_reg(struct i2c_client *i2c, u8 val)
{
	pr_info("%s:%s(0x%02x)\n", MFD_DEV_NAME, __func__, val);

	return max14577_write_reg(i2c, MAX14577_REG_CONTROL2, val);
}

int max14577_get_control2_reg(struct i2c_client *i2c, u8 *val)
{
	u8 value = 0;
	int ret = 0;

	ret = max14577_read_reg(i2c, MAX14577_REG_CONTROL2, &value);
	if (ret)
		*val = 0xff;
	else
		*val = value;

	pr_info("%s:%s(0x%02x), ret(%d)\n", MFD_DEV_NAME, __func__, *val, ret);

	return ret;
}

static struct mfd_cell max14577_devs[] = {
#if defined(CONFIG_USE_MUIC)
	{ .name = MUIC_DEV_NAME, },
#endif /* CONFIG_USE_MUIC */
	{ .name = "max14577-safeout", },
	{ .name = "max14577-charger", },
};

static int max14577_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max14577_dev *max14577;
	struct max14577_platform_data *pdata = i2c->dev.platform_data;
	u8 reg_data;
	int ret = 0;

	max14577 = kzalloc(sizeof(struct max14577_dev), GFP_KERNEL);
	if (max14577 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, max14577);
	max14577->dev = &i2c->dev;
	max14577->i2c = i2c;
	max14577->irq = i2c->irq;
	if (pdata) {
		max14577->pdata = pdata;
	} else {
		ret = -EIO;
		goto err;
	}
	pdata->set_cdetctrl1_reg = max14577_set_cdetctrl1_reg;
	pdata->get_cdetctrl1_reg = max14577_get_cdetctrl1_reg;
	pdata->set_control2_reg = max14577_set_control2_reg;
	pdata->get_control2_reg = max14577_get_control2_reg;

	mutex_init(&max14577->i2c_lock);

	ret = max14577_read_reg(i2c, MAX14577_REG_DEVICEID, &reg_data);
	if (ret < 0) {
		pr_err("%s:%s device not found on this channel(%d)\n",
				MFD_DEV_NAME, __func__, ret);
		goto err;
	} else {
		/* print Device Id */
		max14577->vendor_id = (reg_data & 0x7);
		max14577->device_id = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s:%s device found: vendor=0x%x, device_id=0x%x\n",
				MFD_DEV_NAME, __func__, max14577->vendor_id,
				max14577->device_id);
	}

	ret = max14577_irq_init(max14577);
	if (ret < 0)
		goto err;

	ret = mfd_add_devices(max14577->dev, -1, max14577_devs,
			ARRAY_SIZE(max14577_devs), NULL, 0);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max14577->dev, pdata->wakeup);

	return ret;

err_mfd:
	mfd_remove_devices(max14577->dev);
err:
	kfree(max14577);
	return ret;
}

static int max14577_i2c_remove(struct i2c_client *i2c)
{
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max14577->dev);
	kfree(max14577);

	return 0;
}

static const struct i2c_device_id max14577_i2c_id[] = {
	{ MFD_DEV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max14577_i2c_id);

#ifdef CONFIG_PM
static int max14577_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(max14577->irq);

	return 0;
}

static int max14577_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max14577_dev *max14577 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max14577->irq);

	return max14577_irq_resume(max14577);
}
#else
#define max14577_suspend	NULL
#define max14577_resume		NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops max14577_pm = {
	.suspend = max14577_suspend,
	.resume = max14577_resume,
};

static struct i2c_driver max14577_i2c_driver = {
	.driver = {
		.name = MFD_DEV_NAME,
		.owner = THIS_MODULE,
		.pm = &max14577_pm,
	},
	.probe = max14577_i2c_probe,
	.remove = max14577_i2c_remove,
	.id_table = max14577_i2c_id,
};

static int __init max14577_i2c_init(void)
{
	return i2c_add_driver(&max14577_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max14577_i2c_init);

static void __exit max14577_i2c_exit(void)
{
	i2c_del_driver(&max14577_i2c_driver);
}
module_exit(max14577_i2c_exit);

MODULE_DESCRIPTION("MAXIM 14577 multi-function core driver");
MODULE_AUTHOR("SeungJin Hahn <sjin.hahn@samsung.com>");
MODULE_LICENSE("GPL");
