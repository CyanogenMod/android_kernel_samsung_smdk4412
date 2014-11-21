/*
 * max8997.c - Voltage regulator driver for the Maxim 8997
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
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>

#include <mach/sec_debug.h>

struct max8997_data {
	struct device		*dev;
	struct max8997_dev	*iodev;
	int			num_regulators;
	struct regulator_dev	**rdev;
	bool			buck1_gpiodvs;
	int			buck_set1;
	int			buck_set2;
	int			buck_set3;
	u8                      buck1_vol[8]; /* voltages for selection */
	unsigned int		buck1_idx; /* index to last changed voltage */
					   /* value in a set */
	bool			buck_ramp_en;
	int			buck_ramp_delay;
	struct max8997_buck1_dvs_funcs funcs;
	struct mutex		dvs_lock;
};

struct vol_cur_map_desc {
	int min;
	int max;
	int step;
};

/* Voltage maps */
static const struct vol_cur_map_desc ldos_vol_cur_map_desc = {
	.min = 800,	.step = 50,	.max = 3950,
};
static const struct vol_cur_map_desc buck1245_vol_cur_map_desc = {
	.min = 650,	.step = 25,	.max = 2225,
};
static const struct vol_cur_map_desc buck37_vol_cur_map_desc = {
	.min = 750,	.step = 50,	.max = 3900,
};

/* flash currents just aren't matching up right! */
static const struct vol_cur_map_desc flash_vol_cur_map_desc = {
	.min = 23440,	.step = 23440,	.max = 750080,
};
static const struct vol_cur_map_desc movie_vol_cur_map_desc = {
	.min = 15625,	.step = 15625,	.max = 250000,
};
#ifdef MAX8997_SUPPORT_TORCH
static const struct vol_cur_map_desc torch_vol_cur_map_desc = {
	.min = 15625,	.step = 15625,	.max = 250000,
};
#endif /* MAX8997_SUPPORT_TORCH */

static const struct vol_cur_map_desc *ldo_vol_cur_map[] = {
	NULL,
	&ldos_vol_cur_map_desc,		/* LDO1 */
	&ldos_vol_cur_map_desc,		/* LDO2 */
	&ldos_vol_cur_map_desc,		/* LDO3 */
	&ldos_vol_cur_map_desc,		/* LDO4 */
	&ldos_vol_cur_map_desc,		/* LDO5 */
	&ldos_vol_cur_map_desc,		/* LDO6 */
	&ldos_vol_cur_map_desc,		/* LDO7 */
	&ldos_vol_cur_map_desc,		/* LDO8 */
	&ldos_vol_cur_map_desc,		/* LDO9 */
	&ldos_vol_cur_map_desc,		/* LDO10 */
	&ldos_vol_cur_map_desc,		/* LDO11 */
	&ldos_vol_cur_map_desc,		/* LDO12 */
	&ldos_vol_cur_map_desc,		/* LDO13 */
	&ldos_vol_cur_map_desc,		/* LDO14 */
	&ldos_vol_cur_map_desc,		/* LDO15 */
	&ldos_vol_cur_map_desc,		/* LDO16 */
	&ldos_vol_cur_map_desc,		/* LDO17 */
	&ldos_vol_cur_map_desc,		/* LDO18 */
	&ldos_vol_cur_map_desc,		/* LDO21 */
	&buck1245_vol_cur_map_desc,	/* BUCK1 */
	&buck1245_vol_cur_map_desc,	/* BUCK2 */
	&buck37_vol_cur_map_desc,	/* BUCK3 */
	&buck1245_vol_cur_map_desc,	/* BUCK4 */
	&buck1245_vol_cur_map_desc,	/* BUCK5 */
	NULL,				/* BUCK6 */
	&buck37_vol_cur_map_desc,	/* BUCK7 */
	NULL,				/* EN32KH_AP */
	NULL,				/* EN32KH_CP */
	NULL,				/* ENVICHG */
	NULL,				/* ESAFEOUT1 */
	NULL,				/* ESAFEOUT2 */
	&flash_vol_cur_map_desc,	/* FLASH_EN */
	&movie_vol_cur_map_desc,	/* MOVIE_EN */
#ifdef MAX8997_SUPPORT_TORCH
	&torch_vol_cur_map_desc,	/* TORCH */
#endif /* MAX8997_SUPPORT_TORCH */
};

static inline int max8997_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8997_list_voltage(struct regulator_dev *rdev,
				unsigned int selector)
{
	const struct vol_cur_map_desc *desc;
	int ldo = max8997_get_ldo(rdev);
	int val;

	if (ldo >= ARRAY_SIZE(ldo_vol_cur_map))
		return -EINVAL;

	desc = ldo_vol_cur_map[ldo];
	if (desc == NULL)
		return -EINVAL;

	val = desc->min + desc->step * selector;
	if (val > desc->max)
		return -EINVAL;

	return val * 1000;
}

static int max8997_list_current(struct regulator_dev *rdev,
				unsigned int selector)
{
	const struct vol_cur_map_desc *desc;
	int co = max8997_get_ldo(rdev);
	int val;

	if (co >= ARRAY_SIZE(ldo_vol_cur_map))
		return -EINVAL;

	desc = ldo_vol_cur_map[co];
	if (desc == NULL)
		return -EINVAL;

	val = desc->min + desc->step * selector;
	if (val > desc->max)
		return -EINVAL;

	return val;
}

static int max8997_get_enable_register(struct regulator_dev *rdev,
					int *reg, int *shift)
{
	int ldo = max8997_get_ldo(rdev);

	switch (ldo) {
	case MAX8997_LDO1 ... MAX8997_LDO21:
		*reg = MAX8997_REG_LDO1CTRL + (ldo - MAX8997_LDO1);
		*shift = 6;
		break;
	case MAX8997_BUCK1:
		*reg = MAX8997_REG_BUCK1CTRL;
		*shift = 0;
		break;
	case MAX8997_BUCK2:
		*reg = MAX8997_REG_BUCK2CTRL;
		*shift = 0;
		break;
	case MAX8997_BUCK3:
		*reg = MAX8997_REG_BUCK3CTRL;
		*shift = 0;
		break;
	case MAX8997_BUCK4:
		*reg = MAX8997_REG_BUCK4CTRL;
		*shift = 0;
		break;
	case MAX8997_BUCK5:
		*reg = MAX8997_REG_BUCK5CTRL;
		*shift = 0;
		break;
	case MAX8997_BUCK6:
		*reg = MAX8997_REG_BUCK6CTRL1;
		*shift = 0;
		break;
	case MAX8997_BUCK7:
		*reg = MAX8997_REG_BUCK7CTRL;
		*shift = 0;
		break;
	case MAX8997_EN32KHZ_AP ... MAX8997_EN32KHZ_CP:
		*reg = MAX8997_REG_CONTROL1;
		*shift = 0 + (ldo - MAX8997_EN32KHZ_AP);
		break;
	case MAX8997_ENVICHG:
		*reg = MAX8997_REG_MBCCTRL1;
		*shift = 7;
		break;
	case MAX8997_ESAFEOUT1 ... MAX8997_ESAFEOUT2:
		*reg = MAX8997_REG_SAFEOUTCTRL;
		*shift = 6 + (ldo - MAX8997_ESAFEOUT1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int max8997_get_enable_mask(struct regulator_dev *rdev)
{
	int ret = 0;
	int ldo = max8997_get_ldo(rdev);

	switch (ldo) {
	case MAX8997_LDO1 ... MAX8997_LDO21:
		ret = 3;
		break;
	case MAX8997_BUCK1 ... MAX8997_ESAFEOUT2:
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

static int max8997_get_disable_val(struct regulator_dev *rdev)
{
	int ret = 0;
	int ldo = max8997_get_ldo(rdev);

	switch (ldo) {
	case MAX8997_LDO1:
	case MAX8997_LDO10:
	case MAX8997_LDO21:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static int max8997_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret, reg, shift = 8;
	u8 val, mask;

	ret = max8997_get_enable_register(rdev, &reg, &shift);
	if (ret < 0)
		return ret;

	ret = max8997_read_reg(i2c, reg, &val);
	if (ret < 0)
		return ret;

	mask = max8997_get_enable_mask(rdev);

	return val & (mask << shift);
}

static int max8997_ldo_enable(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int reg, shift = 8, ret;
	u8 mask;

	ret = max8997_get_enable_register(rdev, &reg, &shift);
	if (ret < 0)
		return ret;

	mask = max8997_get_enable_mask(rdev);

	return max8997_update_reg(i2c, reg, mask<<shift, mask<<shift);
}

static int max8997_ldo_disable(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int reg, shift = 8, ret, val;
	u8 mask;

	ret = max8997_get_enable_register(rdev, &reg, &shift);
	if (ret < 0)
		return ret;

	mask = max8997_get_enable_mask(rdev);
	val = max8997_get_disable_val(rdev);

	return max8997_update_reg(i2c, reg, val<<shift, mask<<shift);
}

static int max8997_ldo_suspend_enable(struct regulator_dev *rdev)
{
	if (rdev->use_count > 0)
		return max8997_ldo_enable(rdev);
	else
		return max8997_ldo_disable(rdev);
}

static int max8997_get_voltage_register(struct regulator_dev *rdev,
				int *_reg, int *_shift, int *_mask)
{
	int ldo = max8997_get_ldo(rdev);
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	int reg, shift = 0, mask = 0xff;

	switch (ldo) {
	case MAX8997_LDO1 ... MAX8997_LDO21:
		reg = MAX8997_REG_LDO1CTRL + (ldo - MAX8997_LDO1);
		mask = 0x3f;
		break;
	case MAX8997_BUCK1:
		reg = MAX8997_REG_BUCK1DVSTV1 + max8997->buck1_idx;
		break;
	case MAX8997_BUCK2:
		reg = MAX8997_REG_BUCK2DVSTV1 + 1;
		break;
	case MAX8997_BUCK3:
		reg = MAX8997_REG_BUCK3DVSTV;
		break;
	case MAX8997_BUCK4:
		reg = MAX8997_REG_BUCK4DVSTV;
		break;
	case MAX8997_BUCK5:
		reg = MAX8997_REG_BUCK5DVSTV1 + 1;
		break;
	case MAX8997_BUCK7:
		reg = MAX8997_REG_BUCK7DVSTV;
		break;
	default:
		return -EINVAL;
	}

	*_reg = reg;
	*_shift = shift;
	*_mask = mask;

	return 0;
}

static int max8997_get_voltage(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int reg, shift = 0, mask, ret;
	u8 val;

	ret = max8997_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret < 0)
		return ret;

	ret = max8997_read_reg(i2c, reg, &val);
	if (ret < 0)
		return ret;

	val >>= shift;
	val &= mask;

	return max8997_list_voltage(rdev, val);
}

static int max8997_set_voltage_ldo(struct regulator_dev *rdev,
				int min_uV, int max_uV, unsigned *selector)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const struct vol_cur_map_desc *desc;
	int ldo = max8997_get_ldo(rdev);
	int reg, shift = 0, mask, ret;
	int i = 0;

	if (ldo >= ARRAY_SIZE(ldo_vol_cur_map))
		return -EINVAL;

	desc = ldo_vol_cur_map[ldo];
	if (desc == NULL)
		return -EINVAL;

	if (max_vol < desc->min || min_vol > desc->max)
		return -EINVAL;

	while (desc->min + desc->step*i < min_vol &&
	       desc->min + desc->step*i < desc->max)
		i++;

	if (desc->min + desc->step*i > max_vol)
		return -EINVAL;

	ret = max8997_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret < 0)
		return ret;

	ret = max8997_update_reg(i2c, reg, i<<shift, mask<<shift);
	*selector = i;

	return ret;
}

static inline void buck1_gpio_set(struct max8997_data *max8997, int v)
{
	static int prev_v = 0x1;

	int gpio1 = max8997->buck_set1;
	int gpio2 = max8997->buck_set2;
	int gpio3 = max8997->buck_set3;

	if (prev_v > v) {
		/* raise the volage */
		gpio_set_value(gpio3, (v >> 2) & 0x1);
		udelay(40);
		gpio_set_value(gpio2, (v >> 1) & 0x1);
		udelay(40);
		gpio_set_value(gpio1, v & 0x1);
	} else {
		gpio_set_value(gpio1, v & 0x1);
		udelay(40);
		gpio_set_value(gpio2, (v >> 1) & 0x1);
		udelay(40);
		gpio_set_value(gpio3, (v >> 2) & 0x1);
	}

	prev_v = v;
}

static int max8997_set_voltage_buck(struct regulator_dev *rdev,
				    int min_uV, int max_uV, unsigned *selector)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int i = 0, j, k;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const struct vol_cur_map_desc *desc;
	int buck = max8997_get_ldo(rdev);
	int reg, shift = 0, mask, ret;
	int difference = 0, previous_vol = 0, current_vol = 0;
	u8 data[7];

	mutex_lock(&max8997->dvs_lock);

	if (buck >= ARRAY_SIZE(ldo_vol_cur_map)) {
		ret = -EINVAL;
		goto out;
	}

	desc = ldo_vol_cur_map[buck];

	if (desc == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (max_vol < desc->min || min_vol > desc->max) {
		ret = -EINVAL;
		goto out;
	}

	while (desc->min + desc->step*i < min_vol &&
	       desc->min + desc->step*i < desc->max)
		i++;

	*selector = i;

	if (desc->min + desc->step*i > max_vol) {
		ret = -EINVAL;
		goto out;
	}

	ret = max8997_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret < 0)
		goto out;

	previous_vol = max8997_get_voltage(rdev);

	/* Check if voltage needs to be changed */
	/* if previous_voltage equal new voltage, return */
	if (previous_vol == max8997_list_voltage(rdev, i)) {
		dev_dbg(max8997->dev, "No voltage change, old:%d, new:%d\n",
			previous_vol, max8997_list_voltage(rdev, i));
		ret = 0;
		goto out;
	}

	switch (buck) {
	case MAX8997_BUCK1:
		sec_debug_aux_log(SEC_DEBUG_AUXLOG_CPU_BUS_CLOCK_CHANGE,
				"%s: BUCK1: min_vol=%d, max_vol=%d(%ps)",
				__func__, min_vol, max_vol,
				__builtin_return_address(0));

		if (!max8997->buck1_gpiodvs) {
			ret = max8997_write_reg(i2c, reg, i);
			break;
		}

		if (gpio_is_valid(max8997->buck_set1) &&
		    gpio_is_valid(max8997->buck_set2) &&
		    gpio_is_valid(max8997->buck_set3)) {
			/* check if requested voltage */
			/* value is already defined */
			for (j = 1; j < ARRAY_SIZE(max8997->buck1_vol); j++) {
				if (max8997->buck1_vol[j] == i) {
					max8997->buck1_idx = j;
					buck1_gpio_set(max8997, j);
					goto buck1_exit;
				}
			}

			/* no predefine regulator found */
			dev_warn(max8997->dev, "BUCK1 no predefined:%d,%d\n",
				min_vol, max_vol);
			max8997->buck1_idx = 7;
			max8997->buck1_vol[max8997->buck1_idx] = i;
			ret = max8997_get_voltage_register(rdev, &reg, &shift,
							   &mask);
			if (ret < 0)
				break;

			ret = max8997_write_reg(i2c, reg, i);
			if (ret < 0)
				break;

			buck1_gpio_set(max8997, max8997->buck1_idx);
buck1_exit:
			dev_dbg(max8997->dev, "%s: SET3:%d,SET2:%d,SET1:%d\n",
				__func__, gpio_get_value(max8997->buck_set3),
				gpio_get_value(max8997->buck_set2),
				gpio_get_value(max8997->buck_set1));
			break;
		} else
			ret = max8997_write_reg(i2c, reg, i);
		break;
	case MAX8997_BUCK2:
		sec_debug_aux_log(SEC_DEBUG_AUXLOG_CPU_BUS_CLOCK_CHANGE,
				"%s: BUCK2: min_vol=%d, max_vol=%d(%ps)",
				__func__, min_vol, max_vol,
				__builtin_return_address(0));
	case MAX8997_BUCK5:
		for (k = 0; k < 7; k++)
			data[k] = i;
		ret = max8997_bulk_write(i2c, reg, 7, data);
		break;
	case MAX8997_BUCK3:
	case MAX8997_BUCK4:
	case MAX8997_BUCK7:
		ret = max8997_write_reg(i2c, reg, i);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0) {
		dev_err(max8997->dev, "Failed to write buck%d voltage"
			"register: %d\n", buck - MAX8997_BUCK1 + 1, ret);
		goto out;
	}

	if (!max8997->buck_ramp_en)
		goto out;

	current_vol = desc->min + desc->step*i;
	if (previous_vol/1000 < current_vol)
		difference = current_vol - previous_vol/1000;
	else
		difference = previous_vol/1000 - current_vol;

	udelay(difference / max8997->buck_ramp_delay);
out:
	mutex_unlock(&max8997->dvs_lock);
	return ret;
}

static int max8997_flash_is_enabled(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;
	u8 val, mask;

	switch (max8997_get_ldo(rdev)) {
	case MAX8997_FLASH_CUR:
		mask = MAX8997_FLASH_EN_MASK;
		break;
#ifdef MAX8997_SUPPORT_TORCH
	case MAX8997_FLASH_TORCH:
#endif /* MAX8997_FLASH_TORCH */
	case MAX8997_MOVIE_CUR:
		mask = MAX8997_MOVIE_EN_MASK;
		break;
	default:
		return -EINVAL;
	}

	ret = max8997_read_reg(i2c, MAX8997_REG_LED_CNTL, &val);
	if (ret < 0)
		return ret;

	return val & mask;
}

static int max8997_flash_enable(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;

	ret = max8997_update_reg(i2c, MAX8997_REG_BOOST_CNTL,
			1 << MAX8997_BOOST_EN_SHIFT, MAX8997_BOOST_EN_MASK);
	if (ret < 0)
		return ret;

	switch (max8997_get_ldo(rdev)) {
	case MAX8997_FLASH_CUR:
		ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			7 << MAX8997_FLASH_EN_SHIFT, MAX8997_FLASH_EN_MASK);
		break;
	case MAX8997_MOVIE_CUR:
		ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			7 << MAX8997_MOVIE_EN_SHIFT, MAX8997_MOVIE_EN_MASK);
		break;
#ifdef MAX8997_SUPPORT_TORCH
	case MAX8997_FLASH_TORCH:
		ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			3 << MAX8997_MOVIE_EN_SHIFT, MAX8997_MOVIE_EN_MASK);
		break;
#endif /* MAX8997_SUPPORT_TORCH */
	default:
		return -EINVAL;
	}

	return ret;
}

static int max8997_flash_disable(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;

	ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			0 << MAX8997_FLASH_EN_SHIFT, MAX8997_FLASH_EN_MASK);
	if (ret < 0)
		return ret;

	switch (max8997_get_ldo(rdev)) {
	case MAX8997_FLASH_CUR:
		ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			0 << MAX8997_FLASH_EN_SHIFT, MAX8997_FLASH_EN_MASK);
		break;
#ifdef MAX8997_SUPPORT_TORCH
	case MAX8997_FLASH_TORCH:
#endif /* MAX8997_SUPPORT_TORCH */
	case MAX8997_MOVIE_CUR:
		ret = max8997_update_reg(i2c, MAX8997_REG_LED_CNTL,
			0 << MAX8997_MOVIE_EN_SHIFT, MAX8997_MOVIE_EN_MASK);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	return max8997_update_reg(i2c, MAX8997_REG_BOOST_CNTL,
			0 << MAX8997_BOOST_EN_SHIFT, MAX8997_BOOST_EN_MASK);
}

static int max8997_flash_get_current(struct regulator_dev *rdev)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;
	u8 val, shift = 0;

	switch (max8997_get_ldo(rdev)) {
	case MAX8997_FLASH_CUR:
		/* Assume that FLED1 and FLED2 have same current setting */
		ret = max8997_read_reg(i2c, MAX8997_REG_FLASH1_CUR, &val);
		shift = 3;
		break;
#ifdef MAX8997_SUPPORT_TORCH
	case MAX8997_FLASH_TORCH:
#endif /* MAX8997_SUPPORT_TORCH */
	case MAX8997_MOVIE_CUR:
		ret = max8997_read_reg(i2c, MAX8997_REG_MOVIE_CUR, &val);
		shift = 4;
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	val >>= shift;

	return max8997_list_current(rdev, val);
}

static int max8997_flash_set_current(struct regulator_dev *rdev,
				     int min_uA, int max_uA)
{
	struct max8997_data *max8997 = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8997->iodev->i2c;
	int min_amp = min_uA, max_amp = max_uA;
	const struct vol_cur_map_desc *desc;
	int co = max8997_get_ldo(rdev);
	int ret;
	int i = 0;

	if (co >= ARRAY_SIZE(ldo_vol_cur_map))
		return -EINVAL;

	desc = ldo_vol_cur_map[co];
	if (desc == NULL)
		return -EINVAL;

	if (max_amp < desc->min || min_amp > desc->max)
		return -EINVAL;

	while (desc->min + desc->step*i < min_amp &&
	       desc->min + desc->step*i < desc->max)
		i++;

	if (desc->min + desc->step*i > max_amp)
		return -EINVAL;

	switch (co) {
	case MAX8997_FLASH_CUR:
		ret = max8997_write_reg(i2c, MAX8997_REG_FLASH1_CUR, i << 3);
		if (ret < 0)
			return ret;
		ret = max8997_write_reg(i2c, MAX8997_REG_FLASH2_CUR, i << 3);
		break;
	case MAX8997_MOVIE_CUR:
#if 0
		ret = max8997_write_reg(i2c, MAX8997_REG_MOVIE_CUR, i << 4);
#else
		/* Workaround: MOVIE_CUR setting was ignored because
		 * GSMB is always high.
		 */
		ret = max8997_update_reg(i2c, MAX8997_REG_GSMB_CUR, i << 2,
				0xf << 2);
#endif
		break;
#ifdef MAX8997_SUPPORT_TORCH
	case MAX8997_FLASH_TORCH:
		ret = max8997_write_reg(i2c, MAX8997_REG_MOVIE_CUR, i << 4);
		break;
#endif /* MAX8997_SUPPORT_TORCH */
	default:
		return -EINVAL;
	}

	return ret;
}

static struct regulator_ops max8997_ldo_ops = {
	.list_voltage		= max8997_list_voltage,
	.is_enabled		= max8997_ldo_is_enabled,
	.enable			= max8997_ldo_enable,
	.disable		= max8997_ldo_disable,
	.get_voltage		= max8997_get_voltage,
	.set_voltage		= max8997_set_voltage_ldo,
	.set_suspend_enable	= max8997_ldo_suspend_enable,
	.set_suspend_disable	= max8997_ldo_disable,
};

static struct regulator_ops max8997_buck_ops = {
	.list_voltage		= max8997_list_voltage,
	.is_enabled		= max8997_ldo_is_enabled,
	.enable			= max8997_ldo_enable,
	.disable		= max8997_ldo_disable,
	.get_voltage		= max8997_get_voltage,
	.set_voltage		= max8997_set_voltage_buck,
	.set_suspend_enable	= max8997_ldo_enable,
	.set_suspend_disable	= max8997_ldo_disable,
};

static struct regulator_ops max8997_flash_ops = {
	.is_enabled		= max8997_flash_is_enabled,
	.enable			= max8997_flash_enable,
	.disable		= max8997_flash_disable,
	.set_current_limit	= max8997_flash_set_current,
	.get_current_limit	= max8997_flash_get_current,
	.set_suspend_enable	= max8997_flash_enable,
	.set_suspend_disable	= max8997_flash_disable,
};

static struct regulator_ops max8997_others_ops = {
	.is_enabled		= max8997_ldo_is_enabled,
	.enable			= max8997_ldo_enable,
	.disable		= max8997_ldo_disable,
	.set_suspend_enable	= max8997_ldo_enable,
	.set_suspend_disable	= max8997_ldo_disable,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDO1",
		.id		= MAX8997_LDO1,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO2",
		.id		= MAX8997_LDO2,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO3",
		.id		= MAX8997_LDO3,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO4",
		.id		= MAX8997_LDO4,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO5",
		.id		= MAX8997_LDO5,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO6",
		.id		= MAX8997_LDO6,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO7",
		.id		= MAX8997_LDO7,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO8",
		.id		= MAX8997_LDO8,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO9",
		.id		= MAX8997_LDO9,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO10",
		.id		= MAX8997_LDO10,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO11",
		.id		= MAX8997_LDO11,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO12",
		.id		= MAX8997_LDO12,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO13",
		.id		= MAX8997_LDO13,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO14",
		.id		= MAX8997_LDO14,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO15",
		.id		= MAX8997_LDO15,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO16",
		.id		= MAX8997_LDO16,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO17",
		.id		= MAX8997_LDO17,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO18",
		.id		= MAX8997_LDO18,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO21",
		.id		= MAX8997_LDO21,
		.ops		= &max8997_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK1",
		.id		= MAX8997_BUCK1,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK2",
		.id		= MAX8997_BUCK2,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK3",
		.id		= MAX8997_BUCK3,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK4",
		.id		= MAX8997_BUCK4,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK5",
		.id		= MAX8997_BUCK5,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK6",
		.id		= MAX8997_BUCK6,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK7",
		.id		= MAX8997_BUCK7,
		.ops		= &max8997_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "EN32KHz AP",
		.id		= MAX8997_EN32KHZ_AP,
		.ops		= &max8997_others_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "EN32KHz CP",
		.id		= MAX8997_EN32KHZ_CP,
		.ops		= &max8997_others_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "ENVICHG",
		.id		= MAX8997_ENVICHG,
		.ops		= &max8997_others_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "ESAFEOUT1",
		.id		= MAX8997_ESAFEOUT1,
		.ops		= &max8997_others_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "ESAFEOUT2",
		.id		= MAX8997_ESAFEOUT2,
		.ops		= &max8997_others_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "FLASH_CUR",
		.id		= MAX8997_FLASH_CUR,
		.ops		= &max8997_flash_ops,
		.type		= REGULATOR_CURRENT,
		.owner		= THIS_MODULE,
	}, {
		.name		= "MOVIE_CUR",
		.id		= MAX8997_MOVIE_CUR,
		.ops		= &max8997_flash_ops,
		.type		= REGULATOR_CURRENT,
		.owner		= THIS_MODULE,
#ifdef MAX8997_SUPPORT_TORCH
	}, {
		.name		= "FLASH_TORCH",
		.id		= MAX8997_FLASH_TORCH,
		.ops		= &max8997_flash_ops,
		.type		= REGULATOR_CURRENT,
		.owner		= THIS_MODULE,
#endif /* MAX8997_SUPPORT_TORCH */
	}
};

static int max8997_set_buck_max_voltage(struct max8997_data *max8997,
				int buck_no, unsigned int buck_max_vol)
{
	struct i2c_client *i2c;
	int i = 0, j;
	u8 reg, data[8];

	i2c = max8997->iodev->i2c;

	while (buck1245_vol_cur_map_desc.min + buck1245_vol_cur_map_desc.step*i
	       != (buck_max_vol / 1000)) {
		i++;
		if (i > 0x3f) {
			dev_err(max8997->dev, "%s: invalid voltage(%d,%d)\n",
					__func__, buck_no, buck_max_vol);
			return -EINVAL;
		}
	}

	dev_info(max8997->dev, "%s: buck%d: %d, i:%d\n", __func__, buck_no,
			buck_max_vol, i);

	switch (buck_no) {
	case 1:
		max8997->buck1_vol[0] = i;
		reg = MAX8997_REG_BUCK1DVSTV1;
		break;
	case 2:
		reg = MAX8997_REG_BUCK2DVSTV1;
		break;
	case 5:
		reg = MAX8997_REG_BUCK5DVSTV1;
		break;
	default:
		pr_err("%s: wrong BUCK number(%d)!\n", __func__, buck_no);
		return -EINVAL;
	}

	for (j = 0; j < 8; j++)
		data[j] = i;

	return max8997_bulk_write(i2c, reg, 8, data);
}

/* Set predefined value for BUCK1 registers */
static int max8997_set_buck1_voltages(struct max8997_data *max8997,
				unsigned int *buck1_voltages, int arr_size)
{
	struct i2c_client *i2c;
	int i, j, idx = 1, ret = 0;
	u8 reg;

	mutex_lock(&max8997->dvs_lock);

	if (arr_size > BUCK1_TABLE_SIZE) {
		dev_err(max8997->dev, "%s: too big voltage array!(%d)\n",
				__func__, arr_size);
		ret = -EINVAL;
		goto out;
	}

	i2c = max8997->iodev->i2c;

	for (i = 0; i < arr_size; i++) {
		if (buck1_voltages[i] == 0)
			continue;

		j = 0;
		while (buck1245_vol_cur_map_desc.min
				+ buck1245_vol_cur_map_desc.step*j
				!= (buck1_voltages[i] / 1000))
			j++;

		dev_info(max8997->dev, "%s: buck1_voltages[%d]=%d, "
				"buck1_vol[%d]=%d\n", __func__,
				i, buck1_voltages[i], idx, j);

		max8997->buck1_vol[idx] = j;
		reg = MAX8997_REG_BUCK1DVSTV1 + idx;
		ret = max8997_write_reg(i2c, reg, j);
		if (ret < 0) {
			dev_err(max8997->dev, "%s: fail to write reg(%d)!\n",
					__func__, ret);
			break;
		}
		idx++;
	}
out:
	mutex_unlock(&max8997->dvs_lock);
	return ret;
}

static void max8997_set_buckramp(struct max8997_data *max8997,
		struct max8997_platform_data *pdata)
{
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;
	u8 val = 0;

	max8997->buck_ramp_en = pdata->buck_ramp_en;
	if (max8997->buck_ramp_en)
		val = (MAX8997_ENRAMPBUCK1 | MAX8997_ENRAMPBUCK2
			| MAX8997_ENRAMPBUCK4 | MAX8997_ENRAMPBUCK5);

	max8997->buck_ramp_delay = pdata->buck_ramp_delay;
	switch (max8997->buck_ramp_delay) {
	case 1 ... 12:
		val |= max8997->buck_ramp_delay - 1;
		break;
	case 16:
		val |= 0xc; /* 16.67mV/us */
		break;
	case 25:
		val |= 0xd;
		break;
	case 50:
		val |= 0xe;
		break;
	case 100:
		val |= 0xf;
		break;
	default:
		dev_warn(max8997->dev, "%s: invalid ramp delay, "
			"set ramp delay as 10mV/us\n", __func__);
		max8997->buck_ramp_delay = 10;
		val |= 0x9;
		break;
	}

	dev_info(max8997->dev, "RAMP REG(0x%x)\n", val);
	ret = max8997_write_reg(i2c, MAX8997_REG_BUCKRAMP, val);
	if (ret < 0)
		dev_err(max8997->dev, "failed to write RAMP REG(%d)\n", ret);
}

static int max8997_set_buck1_dvs_table(struct max8997_buck1_dvs_funcs *ptr,
				unsigned int *voltage_table, int arr_size)
{
	struct max8997_data *max8997
		= container_of(ptr, struct max8997_data, funcs);
	int ret;

	ret = max8997_set_buck1_voltages(max8997, voltage_table, arr_size);

	if (ret >= 0) {
		max8997->buck1_gpiodvs = true;
		dev_info(max8997->dev, "%s: enable BUCK1 GPIO DVS\n",
				__func__);

	}
	return ret;
}

static void max8997_set_mr_debouce_time(struct max8997_data *max8997,
					struct max8997_platform_data *pdata)
{
	struct i2c_client *i2c = max8997->iodev->i2c;
	int ret;
	u8 val = pdata->mr_debounce_time;

	if (val > 8) {
		dev_err(max8997->dev, "Invalid MR debounce time(%d)\n", val);
		return;
	}

	dev_info(max8997->dev, "manual reset debouce time: %d sec.\n", val);

	ret = max8997_write_reg(i2c, MAX8997_REG_CONTROL2, --val);
	if (ret < 0)
		dev_err(max8997->dev, "failed to write CONTROL2(%d)\n", ret);
}

static __devinit int max8997_pmic_probe(struct platform_device *pdev)
{
	struct max8997_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max8997_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct max8997_data *max8997;
	struct i2c_client *i2c;
	int i, size, ret = 0;

	if (!pdata) {
		dev_err(pdev->dev.parent, "No platform init data supplied\n");
		return -ENODEV;
	}

	max8997 = kzalloc(sizeof(struct max8997_data), GFP_KERNEL);
	if (!max8997)
		return -ENOMEM;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	max8997->rdev = kzalloc(size, GFP_KERNEL);
	if (!max8997->rdev) {
		ret = -ENOMEM;
		goto err3;
	}

	mutex_init(&max8997->dvs_lock);

	rdev = max8997->rdev;
	max8997->dev = &pdev->dev;
	max8997->iodev = iodev;
	max8997->num_regulators = pdata->num_regulators;
	platform_set_drvdata(pdev, max8997);
	i2c = max8997->iodev->i2c;
	max8997->buck1_gpiodvs = pdata->buck1_gpiodvs;
	max8997->buck_set1 = pdata->buck_set1;
	max8997->buck_set2 = pdata->buck_set2;
	max8997->buck_set3 = pdata->buck_set3;

	/* NOTE:
	 * This MAX8997 PMIC driver support only BUCK1 GPIO DVS
	 * because BUCK1(ARM clock voltage) is most frequently changed
	 */
	/* For the safety, set max voltage before DVS configuration */
	if (!pdata->buck1_max_vol || !pdata->buck2_max_vol
			|| !pdata->buck5_max_vol) {
		pr_err("MAX8997: must set buck max voltage!\n");
		goto err2;
	}

	ret = max8997_set_buck_max_voltage(max8997, 1, pdata->buck1_max_vol);
	if (ret < 0) {
		pr_err("MAX8997: fail to set buck1 max voltage!\n");
		goto err2;
	}

	ret = max8997_set_buck_max_voltage(max8997, 2, pdata->buck2_max_vol);
	if (ret < 0) {
		pr_err("MAX8997: fail to set buck2 max voltage!\n");
		goto err2;
	}

	ret = max8997_set_buck_max_voltage(max8997, 5, pdata->buck5_max_vol);
	if (ret < 0) {
		pr_err("MAX8997: fail to set buck5 max voltage!\n");
		goto err2;
	}

	/* NOTE: */
	/* For unused GPIO NOT marked as -1 (thereof equal to 0)  WARN_ON */
	/* will be displayed */
	/* Check if MAX8997 voltage selection GPIOs are defined */
	if (gpio_is_valid(max8997->buck_set1) &&
	    gpio_is_valid(max8997->buck_set2) &&
	    gpio_is_valid(max8997->buck_set3)) {
		/* Check if SET1 is not equal to 0 */
		if (!max8997->buck_set1) {
			pr_err("MAX8997 SET1 GPIO defined as 0 !\n");
			WARN_ON(!pdata->buck_set1);
			ret = -EIO;
			goto err2;
		}
		/* Check if SET2 is not equal to 0 */
		if (!max8997->buck_set2) {
			pr_err("MAX8998 SET2 GPIO defined as 0 !\n");
			WARN_ON(!pdata->buck_set2);
			ret = -EIO;
			goto err2;
		}
		/* Check if SET3 is not equal to 0 */
		if (!max8997->buck_set3) {
			pr_err("MAX8997 SET3 GPIO defined as 0 !\n");
			WARN_ON(!max8997->buck_set3);
			ret = -EIO;
			goto err2;
		}

		/* To prepare watchdog reset, index 0 of voltage table is
		 * always highest voltage.
		 * Default voltage of BUCK1,2,5 was configured by bootloader.
		 */
		max8997->buck1_idx = 1;
		gpio_request(max8997->buck_set1, "MAX8997 BUCK_SET1");
		gpio_direction_output(max8997->buck_set1, 1);
		gpio_request(max8997->buck_set2, "MAX8997 BUCK_SET2");
		gpio_direction_output(max8997->buck_set2, 0);
		gpio_request(max8997->buck_set3, "MAX8997 BUCK_SET3");
		gpio_direction_output(max8997->buck_set3, 0);

		if (max8997->buck1_gpiodvs) {
			/* Set predefined value for BUCK1 register 2 ~ 8 */
			ret = max8997_set_buck1_voltages(max8997,
				pdata->buck1_voltages, BUCK1_TABLE_SIZE);
			if (ret < 0)
				goto err2;
		}
	} else {
		pr_err("MAX8997 SETx GPIO is invalid!\n");
		goto err2;
	}

	max8997->funcs.set_buck1_dvs_table = max8997_set_buck1_dvs_table;
	if (pdata->register_buck1_dvs_funcs)
		pdata->register_buck1_dvs_funcs(&max8997->funcs);

	max8997_set_buckramp(max8997, pdata);

	if (pdata->flash_cntl_val) {
		ret =  max8997_write_reg(i2c, MAX8997_REG_FLASH_CNTL,
				pdata->flash_cntl_val);
		if (ret < 0) {
			dev_err(max8997->dev, "flash init failed: %d\n", ret);
			goto err2;
		}
	}

	if (pdata->mr_debounce_time)
		max8997_set_mr_debouce_time(max8997, pdata);

	for (i = 0; i < pdata->num_regulators; i++) {
		const struct vol_cur_map_desc *desc;
		int id = pdata->regulators[i].id;
		int index = id - MAX8997_LDO1;

		if (pdata->regulators[i].is_valid_regulator) {
			if (!pdata->regulators[i].is_valid_regulator(id,
						pdata->regulators[i].initdata))
				continue;
		}

		desc = ldo_vol_cur_map[id];
		if (desc && regulators[index].ops != &max8997_others_ops) {
			int count = (desc->max - desc->min) / desc->step + 1;
			regulators[index].n_voltages = count;
		}
		rdev[i] = regulator_register(&regulators[index], max8997->dev,
				pdata->regulators[i].initdata, max8997);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(max8997->dev, "regulator init failed\n");
			rdev[i] = NULL;
			goto err1;
		}
	}

	return 0;
err1:
	for (i = 0; i < max8997->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
err2:
	kfree(max8997->rdev);
err3:
	kfree(max8997);

	return ret;
}

static int __devexit max8997_pmic_remove(struct platform_device *pdev)
{
	struct max8997_data *max8997 = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = max8997->rdev;
	int i;

	for (i = 0; i < max8997->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(max8997->rdev);
	kfree(max8997);

	return 0;
}

static struct platform_driver max8997_pmic_driver = {
	.driver = {
		.name = "max8997-pmic",
		.owner = THIS_MODULE,
	},
	.probe = max8997_pmic_probe,
	.remove = __devexit_p(max8997_pmic_remove),
};

static int __init max8997_pmic_init(void)
{
	return platform_driver_register(&max8997_pmic_driver);
}
subsys_initcall(max8997_pmic_init);

static void __exit max8997_pmic_cleanup(void)
{
	platform_driver_unregister(&max8997_pmic_driver);
}
module_exit(max8997_pmic_cleanup);

MODULE_DESCRIPTION("MAXIM 8997 voltage regulator driver");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
