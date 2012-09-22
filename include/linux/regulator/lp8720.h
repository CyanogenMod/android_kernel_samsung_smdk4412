/*
 * National Semiconductors LP8720 PMIC chip client interface
 *
 * Based on lp3971.h
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_REGULATOR_LP8720_H
#define __LINUX_REGULATOR_LP8720_H

#include <linux/regulator/machine.h>

/* LP8720 regulator ids */
enum {
	LP8720_LDO1 = 0,
	LP8720_LDO2,
	LP8720_LDO3,
	LP8720_LDO4,
	LP8720_LDO5,
	LP8720_BUCK_V1,
	LP8720_BUCK_V2,
	LP8720_REG_MAX,
};

#define LP8720_LDO1_EN		BIT(0)
#define LP8720_LDO2_EN		BIT(1)
#define LP8720_LDO3_EN		BIT(2)
#define LP8720_LDO4_EN		BIT(3)
#define LP8720_LDO5_EN		BIT(4)
#define LP8720_BUCK_V1_EN	BIT(5)
#define LP8720_BUCK_V2_EN	BIT(5)

#define LP8720_LDO1_REG		(0x01)
#define LP8720_LDO2_REG		(0x02)
#define LP8720_LDO3_REG		(0x03)
#define LP8720_LDO4_REG		(0x04)
#define LP8720_LDO5_REG		(0x05)
#define LP8720_BUCK_V1_REG	(0x06)
#define LP8720_BUCK_V2_REG	(0x07)

/* LP8720 PMIC Registers. */
#define LP8720_GENERAL_SETTINGsS_REG	(0x00)
#define LP8720_LDO1_SETTINGS_REG	(0x01)
#define LP8720_LDO2_SETTINGS_REG	(0x02)
#define LP8720_LDO3_SETTINGS_REG	(0x03)
#define LP8720_LDO4_SETTINGS_REG	(0x04)
#define LP8720_LDO5_SETTINGS_REG	(0x05)
#define LP8720_BUCK_SETTINGS1_REG	(0x06)
#define LP8720_BUCK_SETTINGS2_REG	(0x07)
#define LP8720_ENABLE_REG		(0x08)
#define LP8720_PULLDOWN_REG		(0x09)
#define LP8720_STATUS_REG		(0x0A)
#define LP8720_INTERRUPT_REG		(0x0B)
#define LP8720_INTERRUPTM_REG		(0x0C)

#define LP8720_LDOV_SHIFT		(0)
#define LP8720_LDOT_SHIFT		(5)
#define LP8720_BUCK_SHIFT		(0)
#define LP8720_LDOV_MASK		(0x1F << LP8720_LDOV_SHIFT)
#define LP8720_LDOT_MASK		(0x7 << LP8720_LDOT_SHIFT)
#define LP8720_BUCK_MASK		(0x1F << LP8720_BUCK_SHIFT)

#define LP8720_LDO_VOL_CONTR_BASE 0x01
#define LP8720_LDO_VOL_CONTR_REG(x)	(LP8720_LDO_VOL_CONTR_BASE + (x))

#define LP8720_BUCK_VOL_CONTR_BASE 0x06
#define LP8720_BUCK_VOL_CONTR_REG(x)	(LP8720_BUCK_VOL_CONTR_BASE + (x))

struct lp8720 {
	struct device *dev;
	struct mutex io_lock;
	struct i2c_client *i2c;
	int num_regulators;
	struct regulator_dev **rdev;
};

struct lp8720_regulator_subdev {
	int id;
	struct regulator_init_data *initdata;
};

struct lp8720_platform_data {
	char *name;
	unsigned int en_pin;
	int num_regulators;
	struct lp8720_regulator_subdev *regulators;
};
#endif	/* __LINUX_REGULATOR_LP8720_H */
