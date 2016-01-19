/*
 * Regulator driver for National Semiconductors LP8720 PMIC chip
 *
 * Based on lp3972.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/lp8720.h>
#include <mach/gpio.h>
#include <linux/slab.h>
#include <plat/gpio-cfg.h>

static const int ldo_output_enable_mask[] = {
	LP8720_LDO1_EN,
	LP8720_LDO2_EN,
	LP8720_LDO3_EN,
	LP8720_LDO4_EN,
	LP8720_LDO5_EN,
	LP8720_BUCK_V1_EN,
	LP8720_BUCK_V2_EN,
};

static const int ldo_output_enable_addr[] = {
	LP8720_LDO1_REG,
	LP8720_LDO2_REG,
	LP8720_LDO3_REG,
	LP8720_LDO4_REG,
	LP8720_LDO5_REG,
	LP8720_BUCK_V1_REG,
	LP8720_BUCK_V2_REG,
};

#define LP8720_LDO_OUTPUT_ENABLE_MASK(x) (ldo_output_enable_mask[x])
#define LP8720_LDO_OUTPUT_ENABLE_REG(x) (ldo_output_enable_addr[x])


static const int ldo1235_voltage_map[] = {
	1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550,
	1600, 1650, 1700, 1750, 1800, 1850, 1900, 2000,
	2100, 2200, 2300, 2400, 2500, 2600, 2650, 2700,
	2750, 2800, 2850, 2900, 2950, 3000, 3100, 3300,
};

static const int ldo4_voltage_map[] = {
	800, 850, 900, 1000, 1100, 1200, 1250, 1300,
	1350, 1400, 1450, 1500, 1550, 1600, 1650, 1700,
	1750, 1800, 1850, 1900, 2000, 2100, 2200, 2300,
	2400, 2500, 2600, 2650, 2700, 2750, 2800, 2850,
};

static const int buck12_voltage_map[] = {
	0, 800, 850, 900, 950, 1000, 1050, 1100,
	1150, 1200, 1250, 1300, 1350, 1400, 1450, 1500,
	1550, 1600, 1650, 1700, 1750, 1800, 1850, 1900,
	1950, 2000, 2050, 2100, 2150, 2200, 2250, 2300,
};

static const int *ldo_voltage_map[] = {
	ldo1235_voltage_map, /* LDO1 */
	ldo1235_voltage_map, /* LDO2 */
	ldo1235_voltage_map, /* LDO3 */
	ldo4_voltage_map, /* LDO4 */
	ldo1235_voltage_map, /* LDO5 */
	buck12_voltage_map, /* BUCK_V1 */
	buck12_voltage_map, /* BUCK_V2 */
};

#define LDO_VOL_VALUE_MAP(x) (ldo_voltage_map[(x - LP8720_LDO1)])
#define LDO_VOL_MIN_IDX (0)
#define LDO_VOL_MAX_IDX (31)

static int lp8720_i2c_read(struct i2c_client *i2c, char reg, int count,
	u8 *dest)
{
	u8 ret;

	if (count != 1)
		return -EIO;

	ret = i2c_smbus_read_byte_data(i2c, reg);

	if (ret < 0)
		return ret;

	*dest = ret;
	return 0;
}

static int lp8720_i2c_write(struct i2c_client *i2c, u8 reg, int count,
	const u8 value)
{
	u8 ret;

	if (count != 1)
		return -EIO;

	ret = i2c_smbus_write_byte_data(i2c, reg, value);

	return ret;
}

static u8 lp8720_reg_read(struct lp8720 *lp8720, u8 reg)
{
	u8 val = 0;

	mutex_lock(&lp8720->io_lock);
	lp8720_i2c_read(lp8720->i2c, reg, 1, &val);
	dev_dbg(lp8720->dev, "reg read 0x%02x -> 0x%02x\n", (int)reg,
		(unsigned)val & 0xff);
	mutex_unlock(&lp8720->io_lock);

	return val & 0xff;
}


static int lp8720_set_bits(struct lp8720 *lp8720, u8 reg, u16 mask, u16 val)
{
	u8 tmp;
	int ret;

	mutex_lock(&lp8720->io_lock);
	ret = lp8720_i2c_read(lp8720->i2c, reg, 1, &tmp);
	tmp = (tmp & ~mask) | val;
	if (ret == 0) {
		ret = lp8720_i2c_write(lp8720->i2c, reg, 1, tmp);
		dev_dbg(lp8720->dev, "reg write 0x%02x -> 0x%02x\n", (int)reg,
			(unsigned)val & 0xff);
	}
	mutex_unlock(&lp8720->io_lock);

	return ret;
}

static int lp8720_ldo_is_enabled(struct regulator_dev *dev)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;
	u16 mask = LP8720_LDO_OUTPUT_ENABLE_MASK(ldo);
	u16 val;

	val = lp8720_reg_read(lp8720, LP8720_ENABLE_REG);
	dev_dbg(lp8720->dev, "%s, is_enabled ldo:%d, mask:0x%4x - %s\n",
			__func__, ldo, mask,
			!!(val & mask) ? "enabled" : "disabled");

	return !!(val & mask);
}

static int lp8720_ldo_enable(struct regulator_dev *dev)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;
	u16 mask = LP8720_LDO_OUTPUT_ENABLE_MASK(ldo);

	dev_dbg(lp8720->dev, "%s, enable ldo:%d, mask:0x%4x\n",
			__func__, ldo, mask);
	return lp8720_set_bits(lp8720, LP8720_ENABLE_REG,
				mask, mask);
}

static int lp8720_ldo_disable(struct regulator_dev *dev)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;
	u16 mask = LP8720_LDO_OUTPUT_ENABLE_MASK(ldo);

	dev_dbg(lp8720->dev, "%s, disable ldo:%d, mask:0x%4x\n",
			__func__, ldo, mask);
	return lp8720_set_bits(lp8720, LP8720_ENABLE_REG,
				mask, 0);
}

static int lp8720_ldo_list_voltage(struct regulator_dev *dev, unsigned index)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;

	if (ldo < 0 || ldo > ARRAY_SIZE(ldo_voltage_map)) {
		dev_err(lp8720->dev, "[ldo = %d] is out of range\n", ldo);
		return -EINVAL;
	}
	if (index < 0 || index > LDO_VOL_MAX_IDX) {
		dev_err(lp8720->dev, "[index = %d] is out of range\n", index);
		return -EINVAL;
	}
	return 1000 * LDO_VOL_VALUE_MAP(ldo)[index];
}

static int lp8720_ldo_get_voltage(struct regulator_dev *dev)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;
	u16 val, reg;

	reg = lp8720_reg_read(lp8720, LP8720_LDO_VOL_CONTR_REG(ldo));
	val = reg & LP8720_LDOV_MASK;

	return 1000 * LDO_VOL_VALUE_MAP(ldo)[val];
}

static int lp8720_ldo_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV,
				  unsigned int *selector)
{
	struct lp8720 *lp8720 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - LP8720_LDO1;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = LDO_VOL_VALUE_MAP(ldo);
	u16 val;

	if (min_vol < vol_map[LDO_VOL_MIN_IDX] ||
	    min_vol > vol_map[LDO_VOL_MAX_IDX]) {
		dev_err(lp8720->dev, "[min_vol = %d] is out of range\n",
				min_vol);
		dev_err(lp8720->dev, "vol_map[%d] : %d, vol_map[%d] : %d\n",
				LDO_VOL_MIN_IDX, vol_map[LDO_VOL_MIN_IDX],
				LDO_VOL_MAX_IDX, vol_map[LDO_VOL_MAX_IDX]);
		return -EINVAL;
	}

	for (val = LDO_VOL_MIN_IDX; val <= LDO_VOL_MAX_IDX; val++)
		if (vol_map[val] >= min_vol)
			break;

	if (val > LDO_VOL_MAX_IDX || vol_map[val] > max_vol) {
		dev_err(lp8720->dev, "[val = %d] is out of range\n", val);
		return -EINVAL;
	}

	*selector = val;
	dev_dbg(lp8720->dev, "%s, disable ldo:%d, val:0x%4x\n",
			__func__, ldo, val);

	return lp8720_set_bits(lp8720, LP8720_LDO_VOL_CONTR_REG(ldo),
			LP8720_LDOV_MASK, val);
}

static struct regulator_ops lp8720_ldo_ops = {
	.list_voltage = lp8720_ldo_list_voltage,
	.is_enabled = lp8720_ldo_is_enabled,
	.enable = lp8720_ldo_enable,
	.disable = lp8720_ldo_disable,
	.get_voltage = lp8720_ldo_get_voltage,
	.set_voltage = lp8720_ldo_set_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name = "LDO1",
		.id = LP8720_LDO1,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo1235_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO2",
		.id = LP8720_LDO2,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo1235_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO3",
		.id = LP8720_LDO3,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo1235_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO4",
		.id = LP8720_LDO4,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo4_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO5",
		.id = LP8720_LDO5,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo1235_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK_V1",
		.id = LP8720_BUCK_V1,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(buck12_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "BUCK_V2",
		.id = LP8720_BUCK_V2,
		.ops = &lp8720_ldo_ops,
		.n_voltages = ARRAY_SIZE(buck12_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};

static int __devinit setup_regulators(struct lp8720 *lp8720,
	struct lp8720_platform_data *pdata)
{
	int i, err;

	lp8720->num_regulators = pdata->num_regulators;
	lp8720->rdev = kcalloc(pdata->num_regulators,
				sizeof(struct regulator_dev *), GFP_KERNEL);
	if (!lp8720->rdev) {
		err = -ENOMEM;
		goto err_nomem;
	}

	/* Instantiate the regulators */
	for (i = 0; i < pdata->num_regulators; i++) {
		struct lp8720_regulator_subdev *reg = &pdata->regulators[i];
		lp8720->rdev[i] = regulator_register(&regulators[reg->id],
					lp8720->dev, reg->initdata, lp8720);
		if (IS_ERR(lp8720->rdev[i])) {
			err = PTR_ERR(lp8720->rdev[i]);
			dev_err(lp8720->dev, "regulator init failed: %d\n",
				err);
			goto error;
		}
	}
	return 0;
error:
	while (--i >= 0)
		regulator_unregister(lp8720->rdev[i]);
	kfree(lp8720->rdev);
	lp8720->rdev = NULL;
err_nomem:
	return err;
}

static int __devinit lp8720_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct lp8720 *lp8720;
	struct lp8720_platform_data *pdata = i2c->dev.platform_data;
	int ret;
	u8 readbyte = 0;
	u8 enable = 0;

	if (!pdata) {
		dev_dbg(&i2c->dev, "No platform init data supplied\n");
		return -ENODEV;
	}

	lp8720 = kzalloc(sizeof(struct lp8720), GFP_KERNEL);
	if (!lp8720)
		return -ENOMEM;

	lp8720->i2c = i2c;
	lp8720->dev = &i2c->dev;

	mutex_init(&lp8720->io_lock);
	ret = lp8720_i2c_read(i2c, 0x00, 1, &readbyte);
	if (ret == 0 &&
		readbyte != 0x05) {
		ret = -ENODEV;
		dev_err(&i2c->dev, "chip reported: [00h]= 0x%x\n", readbyte);
	}
	if (ret < 0) {
		dev_err(&i2c->dev, "failed to detect device. ret = %d\n", ret);
		goto err_detect;
	}

	ret = setup_regulators(lp8720, pdata);
	if (ret < 0)
		goto err_detect;

	i2c_set_clientdata(i2c, lp8720);
	if (!strncmp(pdata->name,
		"lp8720_folder_pmic",
		strlen("lp8720_folder_pmic"))) {
		printk(KERN_DEBUG "%s, folder_pmic\n", __func__);
	} else if (!strncmp(pdata->name,
		"lp8720_sub_pmic",
		strlen("lp8720_sub_pmic"))) {
		lp8720_i2c_read(i2c, LP8720_ENABLE_REG, 1, &readbyte);
		enable = readbyte & 0xC0;
		lp8720_i2c_write(i2c, LP8720_ENABLE_REG, 1, enable);
		lp8720_i2c_read(i2c, LP8720_ENABLE_REG, 1, &readbyte);
		printk(KERN_DEBUG "%s, [%s] - ENABLE_REG : 0x%2x\n",
				__func__, pdata->name, readbyte);
	}
	ret = gpio_request(pdata->en_pin, pdata->name);
	if (ret) {
		printk(KERN_ERR "%s, ERROR [%s] - gpio_request(%d) failed\n",
				__func__,
				pdata->name,
				pdata->en_pin);
	}
	s3c_gpio_cfgpin(pdata->en_pin, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(pdata->en_pin, S3C_GPIO_PULL_NONE);
	gpio_set_value(pdata->en_pin, 1);
	if (!gpio_get_value(pdata->en_pin)) {
		printk(KERN_ERR "%s, ERROR [%s] - gpio_get_value(%d) is %s\n",
				__func__,
				pdata->name,
				pdata->en_pin,
				gpio_get_value(pdata->en_pin) ? "HIGH" : "LOW");
	}

	return 0;

err_detect:
	kfree(lp8720);
	return ret;
}

static int __devexit lp8720_i2c_remove(struct i2c_client *i2c)
{
	struct lp8720 *lp8720 = i2c_get_clientdata(i2c);
	struct lp8720_platform_data *pdata = i2c->dev.platform_data;
	int i;

	for (i = 0; i < lp8720->num_regulators; i++)
		regulator_unregister(lp8720->rdev[i]);
	kfree(lp8720->rdev);
	kfree(lp8720);
	gpio_free(pdata->en_pin);
	return 0;
}

static const struct i2c_device_id lp8720_i2c_id[] = {
	{ "lp8720", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lp8720_i2c_id);

static struct i2c_driver lp8720_i2c_driver = {
	.driver = {
		.name = "lp8720",
		.owner = THIS_MODULE,
	},
	.probe    = lp8720_i2c_probe,
	.remove   = __devexit_p(lp8720_i2c_remove),
	.id_table = lp8720_i2c_id,
};

static int __init lp8720_module_init(void)
{
	return i2c_add_driver(&lp8720_i2c_driver);
}
subsys_initcall(lp8720_module_init);

static void __exit lp8720_module_exit(void)
{
	i2c_del_driver(&lp8720_i2c_driver);
}
module_exit(lp8720_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axel Lin <axel.lin@gmail.com>");
MODULE_DESCRIPTION("LP8720 PMIC driver");
