/*
 *  kr3dh.c - ST Microelectronics three-axes accelerometer
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/kr3dh.h>

#define KR3DH_AUTO_INCREASE	0x80
#define KR3DH_WHO_AM_I		0x0f
#define KR3DH_CTRL_REG1		0x20
#define KR3DH_CTRL_REG2		0x21
#define KR3DH_CTRL_REG3		0x22
#define KR3DH_CTRL_REG4		0x23
#define KR3DH_CTRL_REG5		0x24
#define KR3DH_HP_FILTER_RESET	0x25
#define KR3DH_REFERENCE		0x26
#define KR3DH_STATUS_REG	0x27
#define KR3DH_OUT_X_L		0x28
#define KR3DH_OUT_X_H		0x29
#define KR3DH_OUT_Y_L		0x2a
#define KR3DH_OUT_Y_H		0x2b
#define KR3DH_OUT_Z_L		0x2c
#define KR3DH_OUT_Z_H		0x2d
#define KR3DH_INT1_CFG_REG	0x30
#define KR3DH_INT1_SRC_REG	0x31
#define KR3DH_INT1_THS		0x32
#define KR3DH_INT1_DURATION	0x33
#define KR3DH_INT2_CFG_REG	0x34
#define KR3DH_INT2_SRC_REG	0x35
#define KR3DH_INT2_THS		0x36
#define KR3DH_INT2_DURATION	0x37
#define KR3DH_MULTI_OUT_X_L	(KR3DH_AUTO_INCREASE | KR3DH_OUT_X_L)

#define KR3DH_PM_SHIFT		5
#define KR3DH_PM_MASK		((0x7) << KR3DH_PM_SHIFT)
#define KR3DH_DATA_RATE_SHIFT	3
#define KR3DH_DATA_RATE_MASK	((0x3) << KR3DH_DATA_RATE_SHIFT)
#define KR3DH_ZEN_SHIFT		2
#define KR3DH_ZEN_MASK		((0x1) << KR3DH_ZEN_SHIFT)
#define KR3DH_YEN_SHIFT		1
#define KR3DH_YEN_MASK		((0x1) << KR3DH_YEN_SHIFT)
#define KR3DH_XEN_SHIFT		0
#define KR3DH_XEN_MASK		((0x1) << KR3DH_XEN_SHIFT)

#define KR3DH_BOOT_SHIFT	7
#define KR3DH_BOOT_MASK		((0x1) << KR3DH_BOOT_SHIFT)
#define KR3DH_HPM_SHIFT		5
#define KR3DH_HPM_MASK		((0x3) << KR3DH_HPM_SHIFT)
#define KR3DH_FDS_SHIFT		4
#define KR3DH_FDS_MASK		((0x1) << KR3DH_FDS_SHIFT)
#define KR3DH_HPEN2_SHIFT	3
#define KR3DH_HPEN2_MASK	((0x1) << KR3DH_HPEN2_SHIFT)
#define KR3DH_HPEN1_SHIFT	2
#define KR3DH_HPEN1_MASK	((0x1) << KR3DH_HPEN1_SHIFT)
#define KR3DH_HPCF_SHIFT	0
#define KR3DH_HPCF_MASK		((0x3) << KR3DH_HPCF_SHIFT)

#define KR3DH_INT_ACTIVE_SHIFT	7
#define KR3DH_INT_ACTIVE_MASK	((0x1) << KR3DH_INT_ACTIVE_SHIFT)
#define KR3DH_INT_PPOD_SHIFT	6
#define KR3DH_INT_PPOD_MASK	((0x1) << KR3DH_INT_PPOD_SHIFT)
#define KR3DH_LIR2_SHIFT	5
#define KR3DH_LIR2_MASK		((0x1) << KR3DH_LIR2_SHIFT)
#define KR3DH_INT2_CFG_SHIFT	3
#define KR3DH_INT2_CFG_MASK	((0x3) << KR3DH_INT2_CFG_SHIFT)
#define KR3DH_LIR1_SHIFT	2
#define KR3DH_LIR1_MASK		((0x1) << KR3DH_LIR1_SHIFT)
#define KR3DH_INT1_CFG_SHIFT	0
#define KR3DH_INT1_CFG_MASK	((0x3) << KR3DH_INT1_CFG_SHIFT)

#define KR3DH_BDU_SHIFT		7
#define KR3DH_BDU_MASK		((0x1) << KR3DH_BDU_SHIFT)
#define KR3DH_BLE_SHIFT		6
#define KR3DH_BLE_MASK		((0x1) << KR3DH_BLE_SHIFT)
#define KR3DH_FS_SHIFT		4
#define KR3DH_FS_MASK		((0x3) << KR3DH_FS_SHIFT)
#define KR3DH_ST_SIGN_SHIFT	3
#define KR3DH_ST_SIGN_MASK	((0x1) << KR3DH_ST_SIGN_SHIFT)
#define KR3DH_ST_SHIFT		1
#define KR3DH_ST_MASK		((0x3) << KR3DH_ST_SHIFT)
#define KR3DH_SIM_SHIFT		0
#define KR3DH_SIM_MASK		((0x1) << KR3DH_SIM_SHIFT)

#define KR3DH_TURNON_SHIFT	0
#define KR3DH_TURNON_MASK	((0x3) << KR3DH_BOOT_SHIFT)

#define KR3DH_ZYXOR_SHIFT	7
#define KR3DH_ZYXOR_MASK	((0x1) << KR3DH_ZYXOR_SHIFT)
#define KR3DH_ZOR_SHIFT		6
#define KR3DH_ZOR_MASK		((0x1) << KR3DH_ZOR_SHIFT)
#define KR3DH_YOR_SHIFT		5
#define KR3DH_YOR_MASK		((0x1) << KR3DH_YOR_SHIFT)
#define KR3DH_XOR_SHIFT		4
#define KR3DH_XOR_MASK		((0x1) << KR3DH_XOR_SHIFT)
#define KR3DH_ZYXDA_SHIFT	3
#define KR3DH_ZYXDA_MASK	((0x1) << KR3DH_ZYXDA_SHIFT)
#define KR3DH_ZDA_SHIFT		2
#define KR3DH_ZDA_MASK		((0x1) << KR3DH_ZDA_SHIFT)
#define KR3DH_YDA_SHIFT		1
#define KR3DH_YDA_MASK		((0x1) << KR3DH_YDA_SHIFT)
#define KR3DH_XDA_SHIFT		0
#define KR3DH_XDA_MASK		((0x1) << KR3DH_XDA_SHIFT)

#define KR3DH_AOR_SHIFT		7
#define KR3DH_AOR_MASK		((0x1) << KR3DH_AOR_SHIFT)
#define KR3DH_6D_SHIFT		6
#define KR3DH_6D_MASK		((0x1) << KR3DH_6D_SHIFT)
#define KR3DH_ZHIE_SHIFT	5
#define KR3DH_ZHIE_MASK		((0x1) << KR3DH_ZHIE_SHIFT)
#define KR3DH_ZLIE_SHIFT	4
#define KR3DH_ZLIE_MASK		((0x1) << KR3DH_ZLIE_SHIFT)
#define KR3DH_YHIE_SHIFT	3
#define KR3DH_YHIE_MASK		((0x1) << KR3DH_YHIE_SHIFT)
#define KR3DH_YLIE_SHIFT	2
#define KR3DH_YLIE_MASK		((0x1) << KR3DH_YLIE_SHIFT)
#define KR3DH_XHIE_SHIFT	1
#define KR3DH_XHIE_MASK		((0x1) << KR3DH_XHIE_SHIFT)
#define KR3DH_XLIE_SHIFT	0
#define KR3DH_XLIE_MASK		((0x1) << KR3DH_XLIE_SHIFT)

#define KR3DH_IA_SHIFT		6
#define KR3DH_IA_MASK		((0x1) << KR3DH_IA_SHIFT)
#define KR3DH_ZH_SHIFT		5
#define KR3DH_ZH_MASK		((0x1) << KR3DH_ZH_SHIFT)
#define KR3DH_ZL_SHIFT		4
#define KR3DH_ZL_MASK		((0x1) << KR3DH_ZL_SHIFT)
#define KR3DH_YH_SHIFT		3
#define KR3DH_YH_MASK		((0x1) << KR3DH_YH_SHIFT)
#define KR3DH_YL_SHIFT		2
#define KR3DH_YL_MASK		((0x1) << KR3DH_YL_SHIFT)
#define KR3DH_XH_SHIFT		1
#define KR3DH_XH_MASK		((0x1) << KR3DH_XH_SHIFT)
#define KR3DH_XL_SHIFT		0
#define KR3DH_XL_MASK		((0x1) << KR3DH_XL_SHIFT)

#define KR3DH_INT_THS_MASK	0x7f
#define KR3DH_INT_DURATION_MASK	0x7f

#define KR3DH_DEV_ID		0x32
#define K3DH_DEV_ID		0x33

#define KR3DH_OUT_MIN_VALUE	(-32768)
#define KR3DH_OUT_MAX_VALUE	(32767)

#define KR3DH_ON		1
#define KR3DH_OFF		0

struct kr3dh_chip {
	struct i2c_client		*client;
	struct device			*dev;
	struct input_dev		*idev;
	struct work_struct		work1;
	struct work_struct		work2;
	struct mutex			lock;
	struct kr3dh_platform_data	*pdata;

	s16 x;
	s16 y;
	s16 z;

	int irq2;
	u8 power_mode;
	u8 data_rate;
	u8 zen;
	u8 yen;
	u8 xen;
	u8 reboot;
	u8 hpmode;
	u8 filter_sel;
	u8 hp_enable_1;
	u8 hp_enable_2;
	u8 hpcf;
	u8 int_hl_active;
	u8 int_pp_od;
	u8 int2_latch;
	u8 int2_cfg;
	u8 int1_latch;
	u8 int1_cfg;
	u8 block_data_update;
	u8 endian;
	u8 fullscale;
	u8 selftest_sign;
	u8 selftest;
	u8 spi_mode;
	u8 turn_on_mode;
	u8 int1_combination;
	u8 int1_6d_enable;
	u8 int1_z_high_enable;
	u8 int1_z_low_enable;
	u8 int1_y_high_enable;
	u8 int1_y_low_enable;
	u8 int1_x_high_enable;
	u8 int1_x_low_enable;
	u8 int1_threshold;
	u8 int1_duration;
	u8 int2_combination;
	u8 int2_6d_enable;
	u8 int2_z_high_enable;
	u8 int2_z_low_enable;
	u8 int2_y_high_enable;
	u8 int2_y_low_enable;
	u8 int2_x_high_enable;
	u8 int2_x_low_enable;
	u8 int2_threshold;
	u8 int2_duration;
	u8 resume_power_mode;
};

static int kr3dh_write_reg(struct i2c_client *client, u8 reg,
			u8 *value, u8 length)
{
	int ret;

	if (length == 1)
		ret = i2c_smbus_write_byte_data(client, reg, *value);
	else
		ret = i2c_smbus_write_i2c_block_data(client, reg,
						length, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, err %d\n",
			__func__, reg, ret);

	return ret;
}

static int kr3dh_read_reg(struct i2c_client *client, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, err %d\n",
			__func__, reg, ret);

	return ret;
}

static void kr3dh_set_power_mode(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_PM_SHIFT) & KR3DH_PM_MASK;
	value = temp | chip->data_rate | chip->zen |
		chip->yen | chip->xen;

	mutex_lock(&chip->lock);
	chip->power_mode = temp;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG1, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_power_mode(struct kr3dh_chip *chip)
{
	return chip->power_mode >> KR3DH_PM_SHIFT;
}

static void kr3dh_set_zen(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_ZEN_SHIFT) & KR3DH_ZEN_MASK;
	value = temp | chip->power_mode | chip->data_rate |
		chip->yen | chip->xen;
	mutex_lock(&chip->lock);
	chip->zen = temp;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG1, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_zen(struct kr3dh_chip *chip)
{
	return chip->zen >> KR3DH_ZEN_SHIFT;
}

static void kr3dh_set_yen(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_YEN_SHIFT) & KR3DH_YEN_MASK;
	value = temp | chip->power_mode | chip->data_rate |
		chip->zen | chip->xen;
	mutex_lock(&chip->lock);
	chip->yen = temp;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG1, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_yen(struct kr3dh_chip *chip)
{
	return chip->yen >> KR3DH_YEN_SHIFT;
}

static void kr3dh_set_xen(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_XEN_SHIFT) & KR3DH_XEN_MASK;
	value = temp | chip->power_mode | chip->data_rate |
		chip->zen | chip->yen;
	mutex_lock(&chip->lock);
	chip->xen = temp;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG1, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_xen(struct kr3dh_chip *chip)
{
	return chip->xen >> KR3DH_XEN_SHIFT;
}

static void kr3dh_set_reboot(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_BOOT_SHIFT) & KR3DH_BOOT_MASK;
	value = temp | chip->hpmode | chip->filter_sel | chip->hp_enable_2 |
		chip->hp_enable_1 | chip->hpcf;
	mutex_lock(&chip->lock);
	chip->reboot = temp;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG2, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_reboot(struct kr3dh_chip *chip)
{
	return chip->reboot >> KR3DH_BOOT_SHIFT;
}

static void kr3dh_set_int1_6d_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_6D_SHIFT) & KR3DH_6D_MASK;
	value = temp | chip->int1_combination | chip->int1_z_high_enable |
		chip->int1_z_low_enable | chip->int1_y_high_enable |
		chip->int1_y_low_enable | chip->int1_x_high_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_6d_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_6d_enable(struct kr3dh_chip *chip)
{
	return chip->int1_z_high_enable >> KR3DH_6D_SHIFT;
}

static void kr3dh_set_int1_z_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_ZHIE_SHIFT) & KR3DH_ZHIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_low_enable | chip->int1_y_high_enable |
		chip->int1_y_low_enable | chip->int1_x_high_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_z_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_z_high_enable(struct kr3dh_chip *chip)
{
	return chip->int1_z_high_enable >> KR3DH_ZHIE_SHIFT;
}

static void kr3dh_set_int1_z_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_ZLIE_SHIFT) & KR3DH_ZLIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_y_high_enable |
		chip->int1_y_low_enable | chip->int1_x_high_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_z_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_z_low_enable(struct kr3dh_chip *chip)
{
	return chip->int1_z_low_enable >> KR3DH_ZLIE_SHIFT;
}

static void kr3dh_set_int1_y_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_YHIE_SHIFT) & KR3DH_YHIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_z_low_enable |
		chip->int1_y_low_enable | chip->int1_x_high_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_y_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_y_high_enable(struct kr3dh_chip *chip)
{
	return chip->int1_y_high_enable >> KR3DH_YHIE_SHIFT;
}

static void kr3dh_set_int1_y_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_YLIE_SHIFT) & KR3DH_YLIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_z_low_enable |
		chip->int1_y_high_enable | chip->int1_x_high_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_y_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_y_low_enable(struct kr3dh_chip *chip)
{
	return chip->int1_y_low_enable >> KR3DH_YLIE_SHIFT;
}

static void kr3dh_set_int1_x_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_XHIE_SHIFT) & KR3DH_XHIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_z_low_enable |
		chip->int1_y_high_enable | chip->int1_y_low_enable |
		chip->int1_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int1_x_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_x_high_enable(struct kr3dh_chip *chip)
{
	return chip->int1_x_high_enable >> KR3DH_XHIE_SHIFT;
}

static void kr3dh_set_int1_x_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_XLIE_SHIFT) & KR3DH_XLIE_MASK;
	value = temp | chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_z_low_enable |
		chip->int1_y_high_enable | chip->int1_y_low_enable |
		chip->int1_x_high_enable;
	mutex_lock(&chip->lock);
	chip->int1_x_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_x_low_enable(struct kr3dh_chip *chip)
{
	return chip->int1_x_low_enable >> KR3DH_XLIE_SHIFT;
}

static void kr3dh_set_int1_threshold(struct kr3dh_chip *chip, u8 val)
{
	u8 value = val;

	mutex_lock(&chip->lock);
	chip->int1_threshold = val & KR3DH_INT_THS_MASK;
	kr3dh_write_reg(chip->client, KR3DH_INT1_THS, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_threshold(struct kr3dh_chip *chip)
{
	return chip->int1_threshold;
}

static void kr3dh_set_int1_duration(struct kr3dh_chip *chip, u8 val)
{
	u8 value = val;

	mutex_lock(&chip->lock);
	chip->int1_duration = val & KR3DH_INT_DURATION_MASK;
	kr3dh_write_reg(chip->client, KR3DH_INT1_DURATION, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int1_duration(struct kr3dh_chip *chip)
{
	return chip->int1_duration;
}

static void kr3dh_set_int2_6d_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_6D_SHIFT) & KR3DH_6D_MASK;
	value = temp | chip->int2_combination | chip->int2_z_high_enable |
		chip->int2_z_low_enable | chip->int2_y_high_enable |
		chip->int2_y_low_enable | chip->int2_x_high_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_6d_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_6d_enable(struct kr3dh_chip *chip)
{
	return chip->int2_6d_enable >> KR3DH_6D_SHIFT;
}

static void kr3dh_set_int2_z_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_ZHIE_SHIFT) & KR3DH_ZHIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_low_enable | chip->int2_y_high_enable |
		chip->int2_y_low_enable | chip->int2_x_high_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_z_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_z_high_enable(struct kr3dh_chip *chip)
{
	return chip->int2_z_high_enable >> KR3DH_ZHIE_SHIFT;
}

static void kr3dh_set_int2_z_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_ZLIE_SHIFT) & KR3DH_ZLIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_y_high_enable |
		chip->int2_y_low_enable | chip->int2_x_high_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_z_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_z_low_enable(struct kr3dh_chip *chip)
{
	return chip->int2_z_low_enable >> KR3DH_ZLIE_SHIFT;
}

static void kr3dh_set_int2_y_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_YHIE_SHIFT) & KR3DH_YHIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_z_low_enable |
		chip->int2_y_low_enable | chip->int2_x_high_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_y_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_y_high_enable(struct kr3dh_chip *chip)
{
	return chip->int2_y_high_enable >> KR3DH_YHIE_SHIFT;
}

static void kr3dh_set_int2_y_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_YLIE_SHIFT) & KR3DH_YLIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_z_low_enable |
		chip->int2_y_high_enable | chip->int2_x_high_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_y_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_y_low_enable(struct kr3dh_chip *chip)
{
	return chip->int2_y_low_enable >> KR3DH_YLIE_SHIFT;
}

static void kr3dh_set_int2_x_high_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_XHIE_SHIFT) & KR3DH_XHIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_z_low_enable |
		chip->int2_y_high_enable | chip->int2_y_low_enable |
		chip->int2_x_low_enable;
	mutex_lock(&chip->lock);
	chip->int2_x_high_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_x_high_enable(struct kr3dh_chip *chip)
{
	return chip->int2_x_high_enable >> KR3DH_XHIE_SHIFT;
}

static void kr3dh_set_int2_x_low_enable(struct kr3dh_chip *chip, u8 val)
{
	u8 value, temp;

	temp = (val << KR3DH_XLIE_SHIFT) & KR3DH_XLIE_MASK;
	value = temp | chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_z_low_enable |
		chip->int2_y_high_enable | chip->int2_y_low_enable |
		chip->int2_x_high_enable;
	mutex_lock(&chip->lock);
	chip->int2_x_low_enable = temp;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_x_low_enable(struct kr3dh_chip *chip)
{
	return chip->int2_x_low_enable >> KR3DH_XLIE_SHIFT;
}

static void kr3dh_set_int2_threshold(struct kr3dh_chip *chip, u8 val)
{
	u8 value = val;

	mutex_lock(&chip->lock);
	chip->int2_threshold = val & KR3DH_INT_THS_MASK;
	kr3dh_write_reg(chip->client, KR3DH_INT2_THS, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_threshold(struct kr3dh_chip *chip)
{
	return chip->int2_threshold;
}

static void kr3dh_set_int2_duration(struct kr3dh_chip *chip, u8 val)
{
	u8 value = val;

	mutex_lock(&chip->lock);
	chip->int2_duration = val & KR3DH_INT_DURATION_MASK;
	kr3dh_write_reg(chip->client, KR3DH_INT2_DURATION, &value, 1);
	mutex_unlock(&chip->lock);
}

static u8 kr3dh_get_int2_duration(struct kr3dh_chip *chip)
{
	return chip->int2_duration;
}

static void kr3dh_get_xyz(struct kr3dh_chip *chip)
{
	u8 xyz_values[6];
	s16 temp;
	int ret;

	mutex_lock(&chip->lock);

	ret = i2c_smbus_read_i2c_block_data(chip->client,
		KR3DH_MULTI_OUT_X_L, 6, xyz_values);
	if (ret < 0)
		goto out;

	temp = ((xyz_values[1] << BITS_PER_BYTE) | xyz_values[0]);
	chip->x = chip->pdata->negate_x ? -(temp >> 4) : temp >> 4;
	temp = ((xyz_values[3] << BITS_PER_BYTE) | xyz_values[2]);
	chip->y = chip->pdata->negate_y ? -(temp >> 4) : temp >> 4;
	temp = ((xyz_values[5] << BITS_PER_BYTE) | xyz_values[4]);
	chip->z = chip->pdata->negate_z ? -(temp >> 4) : temp >> 4;

	if (chip->pdata->change_xy) {
		temp = chip->x;
		chip->x = chip->y;
		chip->y = temp;
	}
out:
	mutex_unlock(&chip->lock);
}

static void kr3dh_work1(struct work_struct *work)
{
	struct kr3dh_chip *chip = container_of(work,
			struct kr3dh_chip, work1);
	u8 int1_src = 0;

	int1_src = kr3dh_read_reg(chip->client, KR3DH_INT1_SRC_REG);
	if (int1_src ||
	   ((chip->int1_cfg & KR3DH_INT1_CFG_MASK) == KR3DH_DATA_READY)) {
		kr3dh_get_xyz(chip);
		mutex_lock(&chip->lock);
		input_report_abs(chip->idev, ABS_MISC, 1);
		input_report_abs(chip->idev, ABS_MISC, int1_src);
		input_report_abs(chip->idev, ABS_X, chip->x);
		input_report_abs(chip->idev, ABS_Y, chip->y);
		input_report_abs(chip->idev, ABS_Z, chip->z);
		input_sync(chip->idev);
		mutex_unlock(&chip->lock);
	}

	enable_irq(chip->client->irq);
}

static void kr3dh_work2(struct work_struct *work)
{
	struct kr3dh_chip *chip = container_of(work,
			struct kr3dh_chip, work2);
	u8 int2_src = 0;

	int2_src = kr3dh_read_reg(chip->client, KR3DH_INT2_SRC_REG);
	if (int2_src ||
	   ((chip->int2_cfg & KR3DH_INT2_CFG_MASK) == KR3DH_DATA_READY)) {
		kr3dh_get_xyz(chip);
		mutex_lock(&chip->lock);
		input_report_abs(chip->idev, ABS_MISC, 2);
		input_report_abs(chip->idev, ABS_MISC, int2_src);
		input_report_abs(chip->idev, ABS_X, chip->x);
		input_report_abs(chip->idev, ABS_Y, chip->y);
		input_report_abs(chip->idev, ABS_Z, chip->z);
		input_sync(chip->idev);
		mutex_unlock(&chip->lock);
	}

	enable_irq(chip->irq2);
}

static irqreturn_t kr3dh_irq1(int irq, void *data)
{
	struct kr3dh_chip *chip = data;

	if (!work_pending(&chip->work1)) {
		disable_irq_nosync(irq);
		schedule_work(&chip->work1);
	} else {
		dev_err(&chip->client->dev, "work pending\n");
	}

	return IRQ_HANDLED;
}

static irqreturn_t kr3dh_irq2(int irq, void *data)
{
	struct kr3dh_chip *chip = data;

	if (!work_pending(&chip->work2)) {
		disable_irq_nosync(irq);
		schedule_work(&chip->work2);
	} else {
		dev_err(&chip->client->dev, "work pending\n");
	}

	return IRQ_HANDLED;
}

static ssize_t kr3dh_show_xyz(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kr3dh_chip *chip = dev_get_drvdata(dev);
	kr3dh_get_xyz(chip);
	return sprintf(buf, "%d %d %d\n", chip->x, chip->y, chip->z);
}
static DEVICE_ATTR(xyz, S_IRUGO, kr3dh_show_xyz, NULL);

static ssize_t kr3dh_show_reg_state(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kr3dh_chip *chip = dev_get_drvdata(dev);
	u8 ctrl_reg[5], int1_reg[4];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(chip->client,
		(KR3DH_CTRL_REG1 | KR3DH_AUTO_INCREASE), 5, ctrl_reg);

	if (ret < 0) {
		dev_err(&chip->client->dev, "%s: reg 0x%x, err %d\n",
			__func__, KR3DH_CTRL_REG1, ret);
		return ret;
	}

	ret = i2c_smbus_read_i2c_block_data(chip->client,
		(KR3DH_INT1_CFG_REG | KR3DH_AUTO_INCREASE), 4, int1_reg);

	if (ret < 0) {
		dev_err(&chip->client->dev, "%s: reg 0x%x, err %d\n",
			__func__, KR3DH_INT1_CFG_REG, ret);
		return ret;
	}

	ret = i2c_smbus_read_byte_data(chip->client, KR3DH_STATUS_REG);
	if (ret < 0) {
		dev_err(&chip->client->dev, "%s: reg 0x%x, err %d\n",
			__func__, KR3DH_STATUS_REG, ret);
		return ret;
	}

	return sprintf(buf, "ctrl1:0x%x ctrl2:0x%x ctrl3:0x%x\n"
				"ctrl4:0x%x ctrl5:0x%x\n"
				"int1_cfg:0x%x int1_src:0x%x int1_ths:0x%x\n"
				"int1_dur:0x%x status_reg:0x%x\n",
				ctrl_reg[0], ctrl_reg[1], ctrl_reg[2],
				ctrl_reg[3], ctrl_reg[4],
				int1_reg[0], int1_reg[1], int1_reg[2],
				int1_reg[3], ret);
}
static DEVICE_ATTR(reg_state, S_IRUGO, kr3dh_show_reg_state, NULL);

#define KR3DH_INPUT(name)						\
static ssize_t kr3dh_store_##name(struct device *dev,			\
	struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct kr3dh_chip *chip = dev_get_drvdata(dev);			\
	unsigned long val;						\
	int ret;							\
									\
	if (!count)							\
		return -EINVAL;						\
	ret = strict_strtoul(buf, 10, &val);				\
	if (ret) {							\
		dev_err(dev, "fail: conversion %s 5o number\n", buf);	\
		return count;						\
	}								\
	kr3dh_set_##name(chip, (u8) val);				\
	return count;							\
}									\
static ssize_t kr3dh_show_##name(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct kr3dh_chip *chip = dev_get_drvdata(dev);			\
	u16 ret = kr3dh_get_##name(chip);				\
	return sprintf(buf, "%d\n", ret);				\
}									\
static DEVICE_ATTR(name, S_IRUGO | S_IWUSR,				\
		kr3dh_show_##name, kr3dh_store_##name);

KR3DH_INPUT(power_mode);
KR3DH_INPUT(zen);
KR3DH_INPUT(yen);
KR3DH_INPUT(xen);
KR3DH_INPUT(reboot);
KR3DH_INPUT(int1_6d_enable);
KR3DH_INPUT(int1_z_high_enable);
KR3DH_INPUT(int1_z_low_enable);
KR3DH_INPUT(int1_y_high_enable);
KR3DH_INPUT(int1_y_low_enable);
KR3DH_INPUT(int1_x_high_enable);
KR3DH_INPUT(int1_x_low_enable);
KR3DH_INPUT(int1_threshold);
KR3DH_INPUT(int1_duration);
KR3DH_INPUT(int2_6d_enable);
KR3DH_INPUT(int2_z_high_enable);
KR3DH_INPUT(int2_z_low_enable);
KR3DH_INPUT(int2_y_high_enable);
KR3DH_INPUT(int2_y_low_enable);
KR3DH_INPUT(int2_x_high_enable);
KR3DH_INPUT(int2_x_low_enable);
KR3DH_INPUT(int2_threshold);
KR3DH_INPUT(int2_duration);

static struct attribute *kr3dh_attributes[] = {
	&dev_attr_power_mode.attr,
	&dev_attr_zen.attr,
	&dev_attr_yen.attr,
	&dev_attr_xen.attr,
	&dev_attr_reboot.attr,
	&dev_attr_int1_6d_enable.attr,
	&dev_attr_int1_z_high_enable.attr,
	&dev_attr_int1_z_low_enable.attr,
	&dev_attr_int1_y_high_enable.attr,
	&dev_attr_int1_y_low_enable.attr,
	&dev_attr_int1_x_high_enable.attr,
	&dev_attr_int1_x_low_enable.attr,
	&dev_attr_int1_threshold.attr,
	&dev_attr_int1_duration.attr,
	&dev_attr_int2_6d_enable.attr,
	&dev_attr_int2_z_high_enable.attr,
	&dev_attr_int2_z_low_enable.attr,
	&dev_attr_int2_y_high_enable.attr,
	&dev_attr_int2_y_low_enable.attr,
	&dev_attr_int2_x_high_enable.attr,
	&dev_attr_int2_x_low_enable.attr,
	&dev_attr_int2_threshold.attr,
	&dev_attr_int2_duration.attr,
	&dev_attr_xyz.attr,
	&dev_attr_reg_state.attr,
	NULL
};

static const struct attribute_group kr3dh_group = {
	.attrs = kr3dh_attributes,
};

static void kr3dh_unregister_input_device(struct kr3dh_chip *chip)
{
	struct i2c_client *client = chip->client;

	if (client->irq > 0)
		free_irq(client->irq, chip);
	if (chip->irq2 > 0)
		free_irq(chip->irq2, chip);

	input_unregister_device(chip->idev);
	chip->idev = NULL;
}

static int kr3dh_register_input_device(struct kr3dh_chip *chip)
{
	struct i2c_client *client = chip->client;
	struct input_dev *idev;
	int ret;

	idev = chip->idev = input_allocate_device();
	if (!idev) {
		dev_err(&client->dev, "allocating input device failed\n");
		ret = -ENOMEM;
		goto error_alloc;
	}

	idev->name = "KR3DH accelerometer";
	idev->id.bustype = BUS_I2C;
	idev->dev.parent = &client->dev;
	idev->evbit[0] = BIT_MASK(EV_ABS);

	input_set_abs_params(idev, ABS_MISC, 0,	255, 0, 0);
	input_set_abs_params(idev, ABS_X, KR3DH_OUT_MIN_VALUE,
			     KR3DH_OUT_MAX_VALUE, 0, 0);
	input_set_abs_params(idev, ABS_Y, KR3DH_OUT_MIN_VALUE,
			     KR3DH_OUT_MAX_VALUE, 0, 0);
	input_set_abs_params(idev, ABS_Z, KR3DH_OUT_MIN_VALUE,
			     KR3DH_OUT_MAX_VALUE, 0, 0);

	input_set_drvdata(idev, chip);

	ret = input_register_device(idev);
	if (ret) {
		dev_err(&client->dev, "registering input device failed\n");
		goto error_reg;
	}

	if (client->irq > 0) {
		unsigned long irq_flag = IRQF_DISABLED;

		irq_flag |= IRQF_TRIGGER_RISING;
		ret = request_irq(client->irq, kr3dh_irq1, irq_flag,
				"KR3DH accelerometer int1", chip);
		if (ret) {
			dev_err(&client->dev, "can't get IRQ %d, ret %d\n",
					client->irq, ret);
			goto error_irq1;
		}
	}

	if (chip->irq2 > 0) {
		unsigned long irq_flag = IRQF_DISABLED;

		irq_flag |= IRQF_TRIGGER_RISING;
		ret = request_irq(chip->irq2, kr3dh_irq2, irq_flag,
				"KR3DH accelerometer int2", chip);
		if (ret) {
			dev_err(&client->dev, "can't get IRQ %d, ret %d\n",
					chip->irq2, ret);
			goto error_irq2;
		}
	}

	return 0;

error_irq2:
	if (client->irq > 0)
		free_irq(client->irq, chip);
error_irq1:
	input_unregister_device(idev);
error_reg:
	input_free_device(idev);
error_alloc:
	return ret;
}

static void kr3dh_initialize_chip(struct kr3dh_chip *chip)
{
	u8 value;

	/* CTRL_REG1 */
	value = chip->power_mode | chip->data_rate | chip->zen | chip->yen |
		chip->xen;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG1, &value, 1);

	/* CTRL_REG2 */
	value = chip->reboot | chip->hpmode | chip->filter_sel |
		chip->hp_enable_2 | chip->hp_enable_1 | chip->hpcf;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG2, &value, 1);

	/* CTRL_REG3 */
	value = chip->int_hl_active | chip->int_pp_od | chip->int2_latch |
		chip->int2_cfg | chip->int1_latch | chip->int1_cfg;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG3, &value, 1);

	/* CTRL_REG4 */
	value = chip->block_data_update | chip->endian | chip->fullscale |
		chip->selftest_sign | chip->selftest | chip->spi_mode;
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG4, &value, 1);

	/* INT1_THS_REG */
	kr3dh_write_reg(chip->client, KR3DH_INT1_THS,
			&chip->int1_threshold, 1);

	/* INT1_DURATION_REG */
	kr3dh_write_reg(chip->client, KR3DH_INT1_DURATION,
			&chip->int1_duration, 1);

	/* INT2_THS_REG */
	kr3dh_write_reg(chip->client, KR3DH_INT2_THS,
			&chip->int2_threshold, 1);

	/* INT2_DURATION_REG */
	kr3dh_write_reg(chip->client, KR3DH_INT2_DURATION,
			&chip->int2_duration, 1);

	/* INT1_CFG_REG */
	value = chip->int1_combination | chip->int1_6d_enable |
		chip->int1_z_high_enable | chip->int1_z_low_enable |
		chip->int1_y_high_enable | chip->int1_y_low_enable |
		chip->int1_x_high_enable | chip->int1_x_low_enable;
	kr3dh_write_reg(chip->client, KR3DH_INT1_CFG_REG, &value, 1);

	/* INT2_CFG_REG */
	value = chip->int2_combination | chip->int2_6d_enable |
		chip->int2_z_high_enable | chip->int2_z_low_enable |
		chip->int2_y_high_enable | chip->int2_y_low_enable |
		chip->int2_x_high_enable | chip->int2_x_low_enable;
	kr3dh_write_reg(chip->client, KR3DH_INT2_CFG_REG, &value, 1);

	/* CTRL_REG5 */
	kr3dh_write_reg(chip->client, KR3DH_CTRL_REG5, &chip->turn_on_mode, 1);

}

static void kr3dh_format_chip_data(struct kr3dh_chip *chip,
			struct kr3dh_platform_data *pdata)
{
	chip->irq2 = pdata->irq2;

	/* CTRL_REG1 */
	chip->power_mode = (pdata->power_mode << KR3DH_PM_SHIFT) &
			   KR3DH_PM_MASK;
	chip->resume_power_mode = chip->power_mode;
	chip->data_rate = (pdata->data_rate << KR3DH_DATA_RATE_SHIFT) &
			  KR3DH_DATA_RATE_MASK;
	chip->zen = (pdata->zen << KR3DH_ZEN_SHIFT) & KR3DH_ZEN_MASK;
	chip->yen = (pdata->yen << KR3DH_YEN_SHIFT) & KR3DH_YEN_MASK;
	chip->xen = (pdata->xen << KR3DH_XEN_SHIFT) & KR3DH_XEN_MASK;

	/* CTRL_REG2 */
	chip->reboot = (pdata->reboot << KR3DH_BOOT_SHIFT) & KR3DH_BOOT_MASK;
	chip->hpmode = (pdata->hpmode << KR3DH_HPM_SHIFT) & KR3DH_HPM_MASK;
	chip->filter_sel = (pdata->filter_sel << KR3DH_FDS_SHIFT) &
			   KR3DH_FDS_MASK;
	chip->hp_enable_2 = (pdata->hp_enable_2 << KR3DH_HPEN2_SHIFT) &
			    KR3DH_HPEN2_MASK;
	chip->hp_enable_1 = (pdata->hp_enable_1 << KR3DH_HPEN1_SHIFT) &
			    KR3DH_HPEN1_MASK;
	chip->hpcf = (pdata->hpcf << KR3DH_HPCF_SHIFT) & KR3DH_HPCF_MASK;

	/* CTRL_REG3 */
	chip->int_hl_active = (pdata->int_hl_active << KR3DH_INT_ACTIVE_SHIFT) &
			      KR3DH_INT_ACTIVE_MASK;
	chip->int_pp_od = (pdata->int_pp_od << KR3DH_INT_PPOD_SHIFT) &
			  KR3DH_INT_PPOD_MASK;
	chip->int2_latch = (pdata->int2_latch << KR3DH_LIR2_SHIFT) &
			   KR3DH_LIR2_MASK;
	chip->int2_cfg = (pdata->int2_cfg << KR3DH_INT2_CFG_SHIFT) &
			 KR3DH_INT2_CFG_MASK;
	chip->int1_latch = (pdata->int1_latch << KR3DH_LIR1_SHIFT) &
			   KR3DH_LIR1_MASK;
	chip->int1_cfg = (pdata->int1_cfg << KR3DH_INT1_CFG_SHIFT) &
			 KR3DH_INT1_CFG_MASK;

	/* CTRL_REG4 */
	chip->block_data_update =
			((pdata->block_data_update << KR3DH_BDU_SHIFT)
			& KR3DH_BDU_MASK);
	chip->endian = (pdata->endian << KR3DH_BLE_SHIFT) & KR3DH_BLE_MASK;
	chip->fullscale = (pdata->fullscale << KR3DH_FS_SHIFT) & KR3DH_FS_MASK;
	chip->selftest_sign = (pdata->selftest_sign << KR3DH_ST_SIGN_SHIFT) &
			      KR3DH_ST_SIGN_MASK;
	chip->selftest = (pdata->selftest << KR3DH_ST_SHIFT) & KR3DH_ST_MASK;
	chip->spi_mode = (pdata->spi_mode << KR3DH_SIM_SHIFT) & KR3DH_SIM_MASK;

	/* CTRL_REG5 */
	chip->turn_on_mode = pdata->turn_on_mode & KR3DH_TURNON_MASK;

	/* INT1_CFG_REG */
	chip->int1_combination = (pdata->int1_combination << KR3DH_AOR_SHIFT) &
				 KR3DH_AOR_MASK;
	chip->int1_6d_enable = (pdata->int1_6d_enable << KR3DH_6D_SHIFT) &
				KR3DH_6D_MASK;
	chip->int1_z_high_enable = (pdata->int1_z_high_enable <<
				KR3DH_ZHIE_SHIFT) & KR3DH_ZHIE_MASK;
	chip->int1_z_low_enable = (pdata->int1_z_low_enable <<
				KR3DH_ZLIE_SHIFT) & KR3DH_ZLIE_MASK;
	chip->int1_y_high_enable = (pdata->int1_y_high_enable <<
				KR3DH_YHIE_SHIFT) & KR3DH_YHIE_MASK;
	chip->int1_y_low_enable = (pdata->int1_y_low_enable <<
				KR3DH_YLIE_SHIFT) & KR3DH_YLIE_MASK;
	chip->int1_x_high_enable = (pdata->int1_x_high_enable <<
				KR3DH_XHIE_SHIFT) & KR3DH_XHIE_MASK;
	chip->int1_x_low_enable = (pdata->int1_x_low_enable <<
				KR3DH_XLIE_SHIFT) & KR3DH_XLIE_MASK;

	/* INT1_THS_REG */
	chip->int1_threshold = pdata->int1_threshold & KR3DH_INT_THS_MASK;

	/* INT1_DURATION_REG */
	chip->int1_duration = pdata->int1_duration & KR3DH_INT_DURATION_MASK;

	/* INT2_CFG_REG */
	chip->int2_combination = (pdata->int2_combination << KR3DH_AOR_SHIFT) &
				 KR3DH_AOR_MASK;
	chip->int2_6d_enable = (pdata->int2_6d_enable << KR3DH_6D_SHIFT) &
				KR3DH_6D_MASK;
	chip->int2_z_high_enable = (pdata->int2_z_high_enable <<
				KR3DH_ZHIE_SHIFT) & KR3DH_ZHIE_MASK;
	chip->int2_z_low_enable = (pdata->int2_z_low_enable <<
				KR3DH_ZLIE_SHIFT) & KR3DH_ZLIE_MASK;
	chip->int2_y_high_enable = (pdata->int2_y_high_enable <<
				KR3DH_YHIE_SHIFT) & KR3DH_YHIE_MASK;
	chip->int2_y_low_enable = (pdata->int2_y_low_enable <<
				KR3DH_YLIE_SHIFT) & KR3DH_YLIE_MASK;
	chip->int2_x_high_enable = (pdata->int2_x_high_enable <<
				KR3DH_XHIE_SHIFT) & KR3DH_XHIE_MASK;
	chip->int2_x_low_enable = (pdata->int2_x_low_enable <<
				KR3DH_XLIE_SHIFT) & KR3DH_XLIE_MASK;

	/* INT2_THS_REG */
	chip->int2_threshold = pdata->int2_threshold & KR3DH_INT_THS_MASK;

	/* INT2_DURATION_REG */
	chip->int2_duration = pdata->int2_duration & KR3DH_INT_DURATION_MASK;
}

static int __devinit kr3dh_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct kr3dh_chip *chip;
	struct kr3dh_platform_data *pdata;
	u8 value;
	int ret = -1;

	chip = kzalloc(sizeof(struct kr3dh_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	pdata = client->dev.platform_data;
	chip->pdata = pdata;

	/* Detect device id */
	value = kr3dh_read_reg(client, KR3DH_WHO_AM_I);
	if ((value != KR3DH_DEV_ID) && (value != K3DH_DEV_ID)) {
		dev_err(&client->dev, "failed to detect device id\n");
		goto error_devid_detect;
	}

	chip->client = client;

	i2c_set_clientdata(client, chip);
	INIT_WORK(&chip->work1, kr3dh_work1);
	INIT_WORK(&chip->work2, kr3dh_work2);
	mutex_init(&chip->lock);

	ret = sysfs_create_group(&client->dev.kobj, &kr3dh_group);
	if (ret) {
		dev_err(&client->dev,
				"creating attribute group failed\n");
		goto error_sysfs;
	}

	if (pdata)
		kr3dh_format_chip_data(chip, pdata);

	ret = kr3dh_register_input_device(chip);
	if (ret) {
		dev_err(&client->dev, "registering input device failed\n");
		goto error_input;
	}

	kr3dh_initialize_chip(chip);

	pm_runtime_set_active(&client->dev);

	dev_info(&client->dev, "%s registered\n", id->name);

	return 0;

error_input:
	sysfs_remove_group(&client->dev.kobj, &kr3dh_group);
error_devid_detect:
error_sysfs:
	kfree(chip);
	return ret;
}

static int __devexit kr3dh_remove(struct i2c_client *client)
{
	struct kr3dh_chip *chip = i2c_get_clientdata(client);

	disable_irq_nosync(client->irq);
	disable_irq_nosync(chip->irq2);
	cancel_work_sync(&chip->work1);
	cancel_work_sync(&chip->work2);

	kr3dh_unregister_input_device(chip);
	sysfs_remove_group(&client->dev.kobj, &kr3dh_group);
	kfree(chip);

	return 0;
}

#ifdef CONFIG_PM
static int kr3dh_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kr3dh_chip *chip = i2c_get_clientdata(client);

	disable_irq_nosync(client->irq);
	disable_irq_nosync(chip->irq2);
	cancel_work_sync(&chip->work1);
	cancel_work_sync(&chip->work2);

	chip->resume_power_mode = chip->power_mode;
	kr3dh_set_power_mode(chip, KR3DH_POWER_DOWN << KR3DH_PM_SHIFT);

	return 0;
}

static int kr3dh_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kr3dh_chip *chip = i2c_get_clientdata(client);

	kr3dh_set_power_mode(chip, chip->resume_power_mode >> KR3DH_PM_SHIFT);

	enable_irq(client->irq);
	enable_irq(chip->irq2);

	return 0;
}

static int kr3dh_freeze(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kr3dh_chip *chip = i2c_get_clientdata(client);

	disable_irq_nosync(client->irq);
	disable_irq_nosync(chip->irq2);
	cancel_work_sync(&chip->work1);
	cancel_work_sync(&chip->work2);

	return 0;
}

static int kr3dh_restore(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kr3dh_chip *chip = i2c_get_clientdata(client);

	kr3dh_initialize_chip(chip);

	enable_irq(client->irq);
	enable_irq(chip->irq2);

	return 0;
}


static const struct dev_pm_ops kr3dh_dev_pm_ops = {
	.suspend		= kr3dh_suspend,
	.resume			= kr3dh_resume,
	.freeze			= kr3dh_freeze,
	.restore		= kr3dh_restore,
};

#define KR3DH_DEV_PM_OPS	(&kr3dh_dev_pm_ops)
#else
#define KR3DH_DEV_PM_OPS	NULL
#endif

static const struct i2c_device_id kr3dh_id[] = {
	{ "KR3DH", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kr3dh_id);

static struct i2c_driver kr3dh_i2c_driver = {
	.driver = {
		.name	= "KR3DH",
		.pm	= KR3DH_DEV_PM_OPS,
	},
	.probe		= kr3dh_probe,
	.remove		= __exit_p(kr3dh_remove),
	.id_table	= kr3dh_id,
};

static int __init kr3dh_init(void)
{
	return i2c_add_driver(&kr3dh_i2c_driver);
}
module_init(kr3dh_init);

static void __exit kr3dh_exit(void)
{
	i2c_del_driver(&kr3dh_i2c_driver);
}
module_exit(kr3dh_exit);

MODULE_AUTHOR("Donggeun Kim <dg77.kim@samsung.com>");
MODULE_DESCRIPTION("KR3DH accelerometer driver");
MODULE_LICENSE("GPL");
