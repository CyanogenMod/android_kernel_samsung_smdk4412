/*
 * max77693.c - Regulator driver for the Maxim 77693
 *
 * Copyright (C) 2011 Samsung Electronics
 * Sukdong Kim <sukdong.kim@smasung.com>
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

#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>

struct max77693_data {
	struct device *dev;
	struct max77693_dev *iodev;
	int num_regulators;
	struct regulator_dev **rdev;

	u8 saved_states[MAX77693_REG_MAX];
};

struct voltage_map_desc {
	int min;
	int max;
	int step;
	unsigned int n_bits;
};

/* current map in mA */
static const struct voltage_map_desc charger_current_map_desc = {
	.min = 60, .max = 2580, .step = 20, .n_bits = 7,
};

static const struct voltage_map_desc topoff_current_map_desc = {
	.min = 50, .max = 200, .step = 10, .n_bits = 4,
};

static const struct voltage_map_desc *reg_voltage_map[] = {
	[MAX77693_ESAFEOUT1] = NULL,
	[MAX77693_ESAFEOUT2] = NULL,
	[MAX77693_CHARGER] = &charger_current_map_desc,
};

static inline int max77693_get_rid(struct regulator_dev *rdev)
{
	dev_info(&rdev->dev, "func:%s\n", __func__);
	return rdev_get_id(rdev);
}

static int max77693_list_voltage_safeout(struct regulator_dev *rdev,
					 unsigned int selector)
{
	int rid = max77693_get_rid(rdev);
	dev_info(&rdev->dev, "func:%s\n", __func__);
	if (rid == MAX77693_ESAFEOUT1 || rid == MAX77693_ESAFEOUT2) {
		switch (selector) {
		case 0:
			return 4850000;
		case 1:
			return 4900000;
		case 2:
			return 4950000;
		case 3:
			return 3300000;
		default:
			return -EINVAL;
		}
	}

	return -EINVAL;
}

static int max77693_get_enable_register(struct regulator_dev *rdev,
					int *reg, int *mask, int *pattern)
{
	int rid = max77693_get_rid(rdev);
	dev_info(&rdev->dev, "func:%s\n", __func__);
	switch (rid) {
	case MAX77693_ESAFEOUT1...MAX77693_ESAFEOUT2:
		*reg = MAX77693_CHG_REG_SAFEOUT_CTRL;
		*mask = 0x40 << (rid - MAX77693_ESAFEOUT1);
		*pattern = 0x40 << (rid - MAX77693_ESAFEOUT1);
		break;
	case MAX77693_CHARGER:
		*reg = MAX77693_CHG_REG_CHG_CNFG_00;
		*mask = 0xf;
		*pattern = 0x5;
		break;
	default:
		/* Not controllable or not exists */
		return -EINVAL;
	}

	return 0;
}

static int max77693_get_disable_register(struct regulator_dev *rdev,
					int *reg, int *mask, int *pattern)
{
	int rid = max77693_get_rid(rdev);
	dev_info(&rdev->dev, "func:%s\n", __func__);
	switch (rid) {
	case MAX77693_ESAFEOUT1...MAX77693_ESAFEOUT2:
		*reg = MAX77693_CHG_REG_SAFEOUT_CTRL;
		*mask = 0x40 << (rid - MAX77693_ESAFEOUT1);
		*pattern = 0x00;
		break;
	case MAX77693_CHARGER:
		*reg = MAX77693_CHG_REG_CHG_CNFG_00;
		*mask = 0xf;
		*pattern = 0x00;
		break;
	default:
		/* Not controllable or not exists */
		return -EINVAL;
	}

	return 0;
}

static int max77693_reg_is_enabled(struct regulator_dev *rdev)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int ret, reg, mask, pattern;
	u8 val;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	ret = max77693_get_enable_register(rdev, &reg, &mask, &pattern);
	if (ret == -EINVAL)
		return 1;	/* "not controllable" */
	else if (ret)
		return ret;

	ret = max77693_read_reg(i2c, reg, &val);
	if (ret)
		return ret;

	return (val & mask) == pattern;
}

static int max77693_reg_enable(struct regulator_dev *rdev)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int ret, reg, mask, pattern;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	ret = max77693_get_enable_register(rdev, &reg, &mask, &pattern);
	if (ret)
		return ret;

	return max77693_update_reg(i2c, reg, pattern, mask);
}

static int max77693_reg_disable(struct regulator_dev *rdev)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int ret, reg, mask, pattern;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	ret = max77693_get_disable_register(rdev, &reg, &mask, &pattern);
	if (ret)
		return ret;

	return max77693_update_reg(i2c, reg, pattern, mask);
}

static int max77693_get_voltage_register(struct regulator_dev *rdev,
					 int *_reg, int *_shift, int *_mask)
{
	int rid = max77693_get_rid(rdev);
	int reg, shift = 0, mask = 0x3f;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	switch (rid) {
	case MAX77693_ESAFEOUT1...MAX77693_ESAFEOUT2:
		reg = MAX77693_CHG_REG_SAFEOUT_CTRL;
		shift = (rid == MAX77693_ESAFEOUT2) ? 2 : 0;
		mask = 0x3;
		break;
	case MAX77693_CHARGER:
		reg = MAX77693_CHG_REG_CHG_CNFG_09;
		shift = 0;
		mask = 0x7f;
		break;
	default:
		return -EINVAL;
	}

	*_reg = reg;
	*_shift = shift;
	*_mask = mask;

	return 0;
}

static int max77693_list_voltage(struct regulator_dev *rdev,
				 unsigned int selector)
{
	const struct voltage_map_desc *desc;
	int rid = max77693_get_rid(rdev);
	int val;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	if (rid >= ARRAY_SIZE(reg_voltage_map) || rid < 0)
		return -EINVAL;

	desc = reg_voltage_map[rid];
	if (desc == NULL)
		return -EINVAL;

	/* the first four codes for charger current are all 60mA */
	if (rid == MAX77693_CHARGER) {
		if (selector <= 3)
			selector = 0;
		else
			selector -= 3;
	}

	val = desc->min + desc->step * selector;
	if (val > desc->max)
		return -EINVAL;

	return val * 1000;
}

static int max77693_get_voltage(struct regulator_dev *rdev)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int reg, shift, mask, ret;

	u8 val;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	ret = max77693_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	ret = max77693_read_reg(i2c, reg, &val);
	if (ret)
		return ret;

	val >>= shift;
	val &= mask;

	if (rdev->desc && rdev->desc->ops && rdev->desc->ops->list_voltage)
		return rdev->desc->ops->list_voltage(rdev, val);

	/*
	 * max77693_list_voltage returns value for any rdev with voltage_map,
	 * which works for "CHARGER" and "CHARGER TOPOFF" that do not have
	 * list_voltage ops (they are current regulators).
	 */
	return max77693_list_voltage(rdev, val);
}

static inline int max77693_get_voltage_proper_val(
		const struct voltage_map_desc *desc,
		int min_vol, int max_vol)
{
	int i = 0;

	if (desc == NULL)
		return -EINVAL;

	if (max_vol < desc->min || min_vol > desc->max)
		return -EINVAL;

	while (desc->min + desc->step * i < min_vol &&
			desc->min + desc->step * i < desc->max)
		i++;

	if (desc->min + desc->step * i > max_vol)
		return -EINVAL;

	if (i >= (1 << desc->n_bits))
		return -EINVAL;

	return i;
}

static int max77693_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const struct voltage_map_desc *desc;
	int rid = max77693_get_rid(rdev);
	int reg, shift = 0, mask, ret;
	int i;
	u8 org;

	switch (rid) {
	case MAX77693_CHARGER:
		break;
	default:
		return -EINVAL;
	}

	desc = reg_voltage_map[rid];

	i = max77693_get_voltage_proper_val(desc, min_vol, max_vol);
	if (i < 0)
		return i;

	ret = max77693_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	max77693_read_reg(i2c, reg, &org);
	org = (org & mask) >> shift;

	/* the first four codes for charger current are all 60mA */
	if (rid == MAX77693_CHARGER)
		i += 3;

	ret = max77693_update_reg(i2c, reg, i << shift, mask << shift);

	return ret;
}

static const int safeoutvolt[] = {
	3300000,
	4850000,
	4900000,
	4950000,
};

/* For SAFEOUT1 and SAFEOUT2 */
static int max77693_set_voltage_safeout(struct regulator_dev *rdev,
					int min_uV, int max_uV,
					unsigned *selector)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int rid = max77693_get_rid(rdev);
	int reg, shift = 0, mask, ret;
	int i = 0;
	u8 val;
	dev_info(&rdev->dev, "func:%s\n", __func__);
	if (rid != MAX77693_ESAFEOUT1 && rid != MAX77693_ESAFEOUT2)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(safeoutvolt); i++) {
		if (min_uV <= safeoutvolt[i] && max_uV >= safeoutvolt[i])
			break;
	}

	if (i >= ARRAY_SIZE(safeoutvolt))
		return -EINVAL;

	if (i == 0)
		val = 0x3;
	else
		val = i - 1;

	ret = max77693_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	ret = max77693_update_reg(i2c, reg, val << shift, mask << shift);
	*selector = val;

	return ret;
}

static int max77693_reg_enable_suspend(struct regulator_dev *rdev)
{
	dev_info(&rdev->dev, "func:%s\n", __func__);
	return 0;
}

static int max77693_reg_disable_suspend(struct regulator_dev *rdev)
{
	struct max77693_data *max77693 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max77693->iodev->i2c;
	int ret, reg, mask, pattern;
	int rid = max77693_get_rid(rdev);
	dev_info(&rdev->dev, "func:%s\n", __func__);
	ret = max77693_get_disable_register(rdev, &reg, &mask, &pattern);
	if (ret)
		return ret;

	max77693_read_reg(i2c, reg, &max77693->saved_states[rid]);

	dev_dbg(&rdev->dev, "Full Power-Off for %s (%xh -> %xh)\n",
		rdev->desc->name, max77693->saved_states[rid] & mask,
		(~pattern) & mask);
	return max77693_update_reg(i2c, reg, pattern, mask);
}

static struct regulator_ops max77693_safeout_ops = {
	.list_voltage = max77693_list_voltage_safeout,
	.is_enabled = max77693_reg_is_enabled,
	.enable = max77693_reg_enable,
	.disable = max77693_reg_disable,
	.get_voltage = max77693_get_voltage,
	.set_voltage = max77693_set_voltage_safeout,
	.set_suspend_enable = max77693_reg_enable_suspend,
	.set_suspend_disable = max77693_reg_disable_suspend,
};

static struct regulator_ops max77693_charger_ops = {
	.is_enabled		= max77693_reg_is_enabled,
	.enable			= max77693_reg_enable,
	.disable		= max77693_reg_disable,
	.get_current_limit	= max77693_get_voltage,
	.set_current_limit	= max77693_set_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name = "ESAFEOUT1",
		.id = MAX77693_ESAFEOUT1,
		.ops = &max77693_safeout_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	}, {
		.name = "ESAFEOUT2",
		.id = MAX77693_ESAFEOUT2,
		.ops = &max77693_safeout_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	}, {
		.name = "CHARGER",
		.id = MAX77693_CHARGER,
		.ops = &max77693_charger_ops,
		.type = REGULATOR_CURRENT,
		.owner = THIS_MODULE,
	}
};

static __devinit int max77693_pmic_probe(struct platform_device *pdev)
{
	struct max77693_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct max77693_data *max77693;
	struct i2c_client *i2c;
	int i, ret, size;
	dev_info(&pdev->dev, "%s\n", __func__);
	if (!pdata) {
		pr_err("[%s:%d] !pdata\n", __FILE__, __LINE__);
		dev_err(pdev->dev.parent, "No platform init data supplied.\n");
		return -ENODEV;
	}

	max77693 = kzalloc(sizeof(struct max77693_data), GFP_KERNEL);
	if (!max77693) {
		pr_err("[%s:%d] if (!max77693)\n", __FILE__, __LINE__);
		return -ENOMEM;
	}
	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	max77693->rdev = kzalloc(size, GFP_KERNEL);
	if (!max77693->rdev) {
		pr_err("[%s:%d] if (!max77693->rdev)\n", __FILE__, __LINE__);
		kfree(max77693);
		return -ENOMEM;
	}

	rdev = max77693->rdev;
	max77693->dev = &pdev->dev;
	max77693->iodev = iodev;
	max77693->num_regulators = pdata->num_regulators;
	platform_set_drvdata(pdev, max77693);
	i2c = max77693->iodev->i2c;
	pr_debug("[%s:%d] pdata->num_regulators:%d\n", __FILE__, __LINE__,
		pdata->num_regulators);
	for (i = 0; i < pdata->num_regulators; i++) {

		const struct voltage_map_desc *desc;
		int id = pdata->regulators[i].id;
		pr_debug("[%s:%d] for in pdata->num_regulators:%d\n", __FILE__,
			__LINE__, pdata->num_regulators);
		desc = reg_voltage_map[id];
		if (id == MAX77693_ESAFEOUT1 || id == MAX77693_ESAFEOUT2)
			regulators[id].n_voltages = 4;

		rdev[i] = regulator_register(&regulators[id], max77693->dev,
					     pdata->regulators[i].initdata,
					     max77693);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(max77693->dev, "regulator init failed for %d\n",
				id);
			rdev[i] = NULL;
			goto err;
		}
	}

	return 0;
 err:
	pr_err("[%s:%d] err:\n", __FILE__, __LINE__);
	for (i = 0; i < max77693->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
 err_alloc:
	pr_err("[%s:%d] err_alloc\n", __FILE__, __LINE__);
	kfree(max77693->rdev);
	kfree(max77693);

	return ret;
}

static int __devexit max77693_pmic_remove(struct platform_device *pdev)
{
	struct max77693_data *max77693 = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = max77693->rdev;
	int i;
	dev_info(&pdev->dev, "%s\n", __func__);
	for (i = 0; i < max77693->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(max77693->rdev);
	kfree(max77693);

	return 0;
}

static const struct platform_device_id max77693_pmic_id[] = {
	{"max77693-safeout", 0},
	{},
};

MODULE_DEVICE_TABLE(platform, max77693_pmic_id);

static struct platform_driver max77693_pmic_driver = {
	.driver = {
		   .name = "max77693-safeout",
		   .owner = THIS_MODULE,
		   },
	.probe = max77693_pmic_probe,
	.remove = __devexit_p(max77693_pmic_remove),
	.id_table = max77693_pmic_id,
};

static int __init max77693_pmic_init(void)
{
	return platform_driver_register(&max77693_pmic_driver);
}
#ifdef CONFIG_FAST_RESUME
beforeresume_initcall(max77693_pmic_init);
#else
subsys_initcall(max77693_pmic_init);
#endif

static void __exit max77693_pmic_cleanup(void)
{
	platform_driver_unregister(&max77693_pmic_driver);
}

module_exit(max77693_pmic_cleanup);

MODULE_DESCRIPTION("MAXIM 77693 Regulator Driver");
MODULE_AUTHOR("Sukdong Kim <Sukdong.Kim@samsung.com>");
MODULE_LICENSE("GPL");
