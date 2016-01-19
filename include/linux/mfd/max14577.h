/*
 * max14577.h - Driver for the Maxim 14577
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
 * This driver is based on max8997.h
 *
 * MAX14577 has MUIC, Charger devices.
 * The devices share the same I2C bus and interrupt line
 * included in this mfd driver.
 */

#ifndef __MAX14577_H__
#define __MAX14577_H__

#include <linux/regulator/consumer.h>

#define MFD_DEV_NAME "max14577"

/* MAX14577 regulator IDs */
enum max14577_regulators {
	MAX14577_SAFEOUT1 = 0,
	MAX14577_SAFEOUT2,

	MAX14577_CHARGER,

	MAX14577_REG_MAX,
};

struct max14577_regulator_data {
	int id;
	struct regulator_init_data *initdata;
};

struct max14577_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;

	/* current control GPIOs */
	int gpio_pogo_vbatt_en;
	int gpio_pogo_vbus_en;

	/* current control GPIO control function */
	int (*set_gpio_pogo_vbatt_en) (int gpio_val);
	int (*set_gpio_pogo_vbus_en) (int gpio_val);

	int (*set_gpio_pogo_cb) (int new_dev);

	/* max14577 internal API function */
	int (*set_cdetctrl1_reg) (struct i2c_client *i2c, u8 val);
	int (*get_cdetctrl1_reg) (struct i2c_client *i2c, u8 *val);
	int (*set_control2_reg) (struct i2c_client *i2c, u8 val);
	int (*get_control2_reg) (struct i2c_client *i2c, u8 *val);

	struct muic_platform_data *muic_pdata;

	int num_regulators;
	struct max14577_regulator_data *regulators;
};

#endif /* __MAX14577_H__ */
