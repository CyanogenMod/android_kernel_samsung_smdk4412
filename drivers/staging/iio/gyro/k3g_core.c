/*
 *  k3g_core.c - ST Microelectronics three-axis gyroscope sensor
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
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include "../iio.h"
#include "../ring_generic.h"
#include "gyro.h"
#include "k3g.h"

#define K3G_INT_LINE	2

int k3g_set_8bit_value(struct k3g_chip *chip, u8 val,
			u8 *cached_value, u8 shift, u8 mask, u8 reg)
{
	int ret = 0;
	u8 value, temp;

	mutex_lock(&chip->lock);

	temp = (val << shift) & mask;
	if (temp == *cached_value)
		goto out;

	ret = value = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0)
		goto out;

	value &= ~mask;
	value |= temp;
	*cached_value = temp;

	ret = i2c_smbus_write_byte_data(chip->client, reg, value);
out:
	mutex_unlock(&chip->lock);
	return ret;
}

int k3g_get_raw_value(struct k3g_chip *chip, int reg)
{
	u8 values[2];
	s16 raw_value;
	int ret;

	mutex_lock(&chip->lock);
	ret = values[0] = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0)
		goto out;

	ret = values[1] = i2c_smbus_read_byte_data(chip->client,
						reg + 1);
	if (ret < 0)
		goto out;

	raw_value = (values[1] << BITS_PER_BYTE) | values[0];
out:
	mutex_unlock(&chip->lock);
	return raw_value;
}

int k3g_get_8bit_value(struct k3g_chip *chip, u8 shift, u8 mask, u8 reg)
{
	int ret = 0;

	mutex_lock(&chip->lock);

	ret = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0)
		goto out;

	ret &= mask;
	ret >>= shift;
out:
	mutex_unlock(&chip->lock);
	return ret;
}

static int k3g_set_16bit_value(struct k3g_chip *chip, u16 val,
			u16 *cached_value, u8 reg)
{
	u8 values[2];
	int ret = 0;

	mutex_lock(&chip->lock);

	values[0] = val >> BITS_PER_BYTE;
	values[1] = val & 0xff;

	ret = i2c_smbus_write_byte_data(chip->client, reg, values[0]);
	if (ret < 0)
		goto out;

	ret = i2c_smbus_write_byte_data(chip->client,
					(reg + 1), values[1]);
	if (ret < 0)
		goto out;

	*cached_value = (values[0] << BITS_PER_BYTE) | values[1];
out:
	mutex_unlock(&chip->lock);
	return ret;
}

static ssize_t k3g_output_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret = k3g_get_raw_value(chip, this_attr->address);

	return sprintf(buf, "%d\n", ret);
}

#define K3G_OUTPUT(name, shift, mask, reg)				\
static ssize_t k3g_show_##name(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct iio_dev *indio_dev = dev_get_drvdata(dev);		\
	struct k3g_chip *chip = indio_dev->dev_data;			\
	int ret = k3g_get_8bit_value(chip, shift, mask, reg);		\
	return sprintf(buf, "%d\n", ret);				\
}									\
static IIO_DEVICE_ATTR(name, S_IRUGO, k3g_show_##name, NULL, 0);

#define K3G_INPUT(name, field_name, shift, mask, reg)			\
static ssize_t k3g_store_##name(struct device *dev,			\
	struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct iio_dev *indio_dev = dev_get_drvdata(dev);		\
	struct k3g_chip *chip = indio_dev->dev_data;			\
	unsigned long val;						\
	int ret;							\
									\
	if (!count)							\
		return -EINVAL;						\
									\
	ret = strict_strtoul(buf, 10, &val);				\
	if (ret)							\
		return -EINVAL;						\
									\
	ret = k3g_set_8bit_value(chip, val,				\
		(u8 *) &chip->pdata->field_name, shift,	mask, reg);	\
									\
	if (ret < 0)							\
		return ret;						\
									\
	return count;							\
}									\
static ssize_t k3g_show_##name(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct iio_dev *indio_dev = dev_get_drvdata(dev);		\
	struct k3g_chip *chip = indio_dev->dev_data;			\
	int ret = chip->pdata->field_name >> shift;			\
									\
	return sprintf(buf, "%d\n", ret);				\
}									\
static IIO_DEVICE_ATTR(name, S_IRUGO | S_IWUSR,				\
		k3g_show_##name, k3g_store_##name, 0);

#define K3G_INPUT_16BIT(name, field_name, reg)				\
static ssize_t k3g_store_##name(struct device *dev,			\
	struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct iio_dev *indio_dev = dev_get_drvdata(dev);		\
	struct k3g_chip *chip = indio_dev->dev_data;			\
	unsigned long val;						\
	int ret;							\
									\
	if (!count)							\
		return -EINVAL;						\
									\
	ret = strict_strtoul(buf, 10, &val);				\
	if (ret)							\
		return -EINVAL;						\
									\
	ret = k3g_set_16bit_value(chip, val,				\
		(u16 *) &chip->pdata->field_name, reg);			\
									\
	if (ret < 0)							\
		return ret;						\
									\
	return count;							\
}									\
static ssize_t k3g_show_##name(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct iio_dev *indio_dev = dev_get_drvdata(dev);		\
	struct k3g_chip *chip = indio_dev->dev_data;			\
	int ret = chip->pdata->field_name;				\
									\
	return sprintf(buf, "%d\n", ret);				\
}									\
static IIO_DEVICE_ATTR(name, S_IRUGO | S_IWUSR,				\
		k3g_show_##name, k3g_store_##name, 0);

K3G_INPUT(power_down, powerdown, K3G_POWERDOWN_SHIFT,
	  K3G_POWERDOWN_MASK, K3G_CTRL_REG1);
K3G_INPUT(z_en, zen, K3G_Z_EN_SHIFT, K3G_Z_EN, K3G_CTRL_REG1);
K3G_INPUT(y_en, yen, K3G_Y_EN_SHIFT, K3G_Y_EN, K3G_CTRL_REG1);
K3G_INPUT(x_en, xen, K3G_X_EN_SHIFT, K3G_X_EN, K3G_CTRL_REG1);
K3G_INPUT(reboot, reboot, K3G_REBOOT_SHIFT, K3G_REBOOT, K3G_CTRL_REG5);

K3G_OUTPUT(temp_raw, K3G_TEMP_SHIFT, K3G_TEMP_MASK, K3G_OUT_TEMP);

static IIO_DEV_ATTR_GYRO_X(k3g_output_raw, K3G_OUT_X_L);
static IIO_DEV_ATTR_GYRO_Y(k3g_output_raw, K3G_OUT_Y_L);
static IIO_DEV_ATTR_GYRO_Z(k3g_output_raw, K3G_OUT_Z_L);

static struct attribute *k3g_attributes[] = {
	&iio_dev_attr_power_down.dev_attr.attr,
	&iio_dev_attr_z_en.dev_attr.attr,
	&iio_dev_attr_y_en.dev_attr.attr,
	&iio_dev_attr_x_en.dev_attr.attr,
	&iio_dev_attr_reboot.dev_attr.attr,
	&iio_dev_attr_temp_raw.dev_attr.attr,
	&iio_dev_attr_gyro_x_raw.dev_attr.attr,
	&iio_dev_attr_gyro_y_raw.dev_attr.attr,
	&iio_dev_attr_gyro_z_raw.dev_attr.attr,
	NULL
};

static const struct attribute_group k3g_group = {
	.attrs = k3g_attributes,
};

static irqreturn_t k3g_thresh_handler(int irq, void *private)
{
	struct iio_dev *indio_dev = private;
	struct k3g_chip *chip;
	int int1_src;
	s64 last_timestamp = iio_get_time_ns();

	chip = indio_dev->dev_data;
	int1_src = i2c_smbus_read_byte_data(chip->client, K3G_INT1_SRC_REG);
	if (int1_src < 0)
		pr_err("K3G_INT1_SRC_REG read fail(%d)\n", int1_src);
	else if (int1_src & K3G_INTERRUPT_ACTIVE) {
		if ((int1_src & K3G_Z_HIGH) &&
		    (chip->pdata->int1_z_high_enable & K3G_Z_HIGH_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_Z,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_RISING),
				last_timestamp);

		if ((int1_src & K3G_Z_LOW) &&
		    (chip->pdata->int1_z_low_enable & K3G_Z_LOW_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_Z,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_FALLING),
				last_timestamp);

		if ((int1_src & K3G_Y_HIGH) &&
		    (chip->pdata->int1_y_high_enable & K3G_Y_HIGH_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_Y,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_RISING),
				last_timestamp);

		if ((int1_src & K3G_Y_LOW) &&
		    (chip->pdata->int1_y_low_enable & K3G_Y_LOW_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_Y,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_FALLING),
				last_timestamp);

		if ((int1_src & K3G_X_HIGH) &&
		    (chip->pdata->int1_x_high_enable & K3G_X_HIGH_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_X,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_RISING),
				last_timestamp);

		if ((int1_src & K3G_X_LOW) &&
		    (chip->pdata->int1_x_low_enable & K3G_X_LOW_INT_EN))
			iio_push_event(chip->indio_dev, 0,
				IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_X,
						IIO_EV_TYPE_THRESH,
						IIO_EV_DIR_FALLING),
				last_timestamp);
	}

	return IRQ_HANDLED;
}

K3G_INPUT(int1_en, int1_enable, K3G_INT1_EN_SHIFT, K3G_INT1_EN, K3G_CTRL_REG3);
K3G_INPUT(wait_en, int1_wait_enable, K3G_INT1_WAIT_EN_SHIFT,
	  K3G_INT1_WAIT_EN, K3G_INT1_DURATION_REG);
K3G_INPUT(gyro_z_mag_pos_rising_en, int1_z_high_enable,
	  K3G_Z_HIGH_INT_EN_SHIFT, K3G_Z_HIGH_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_z_mag_neg_rising_en, int1_z_low_enable,
	  K3G_Z_LOW_INT_EN_SHIFT, K3G_Z_LOW_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_y_mag_pos_rising_en, int1_y_high_enable,
	  K3G_Y_HIGH_INT_EN_SHIFT, K3G_Y_HIGH_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_y_mag_neg_rising_en, int1_y_low_enable,
	  K3G_Y_LOW_INT_EN_SHIFT, K3G_Y_LOW_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_x_mag_pos_rising_en, int1_x_high_enable,
	  K3G_X_HIGH_INT_EN_SHIFT, K3G_X_HIGH_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_x_mag_neg_rising_en, int1_x_low_enable,
	  K3G_X_LOW_INT_EN_SHIFT, K3G_X_LOW_INT_EN, K3G_INT1_CFG_REG);
K3G_INPUT(gyro_mag_either_rising_period, int1_wait_duration,
	  K3G_INT1_DURATION_SHIFT, K3G_INT1_DURATION_MASK,
	  K3G_INT1_DURATION_REG);
K3G_INPUT_16BIT(gyro_x_mag_either_rising_value, int1_x_threshold,
		K3G_INT1_THS_XH);
K3G_INPUT_16BIT(gyro_y_mag_either_rising_value, int1_y_threshold,
		K3G_INT1_THS_YH);
K3G_INPUT_16BIT(gyro_z_mag_either_rising_value, int1_z_threshold,
		K3G_INT1_THS_ZH);

static struct attribute *k3g_threshold_event_attributes[] = {
	&iio_dev_attr_int1_en.dev_attr.attr,
	&iio_dev_attr_wait_en.dev_attr.attr,
	&iio_dev_attr_gyro_z_mag_pos_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_z_mag_neg_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_y_mag_pos_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_y_mag_neg_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_x_mag_pos_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_x_mag_neg_rising_en.dev_attr.attr,
	&iio_dev_attr_gyro_mag_either_rising_period.dev_attr.attr,
	&iio_dev_attr_gyro_x_mag_either_rising_value.dev_attr.attr,
	&iio_dev_attr_gyro_y_mag_either_rising_value.dev_attr.attr,
	&iio_dev_attr_gyro_z_mag_either_rising_value.dev_attr.attr,
	NULL
};

static irqreturn_t k3g_fifo_handler(int irq, void *private)
{
	struct iio_dev *indio_dev = private;
	struct k3g_chip *chip;
	int int2_src, ctrl_reg3;
	s64 last_timestamp = iio_get_time_ns();

	chip = indio_dev->dev_data;
	int2_src = i2c_smbus_read_byte_data(chip->client, K3G_FIFO_SRC_REG);
	if (int2_src < 0)
		goto fifo_done;

	ctrl_reg3 = i2c_smbus_read_byte_data(chip->client, K3G_CTRL_REG3);
	if (ctrl_reg3 < 0)
		goto fifo_done;

	if (ctrl_reg3 & K3G_INT2_DATA_READY)
		iio_push_event(chip->indio_dev, 1,
			IIO_MOD_EVENT_CODE(IIO_EV_CLASS_GYRO,
						0,
						IIO_EV_MOD_X_OR_Y_OR_Z,
						IIO_EV_TYPE_MAG,
						IIO_EV_DIR_RISING),
			last_timestamp);
	else
		k3g_ring_int_process(int2_src, chip->indio_dev->ring);

	enable_irq(chip->pdata->irq2);

fifo_done:
	return IRQ_HANDLED;
}

static ssize_t k3g_store_int2_src(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct k3g_chip *chip = indio_dev->dev_data;
	unsigned long val;
	int ret;

	if (!count)
		return -EINVAL;

	ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val < 4)
		val = 1 << val;
	else
		return -EINVAL;

	ret = k3g_set_8bit_value(chip, val, &chip->pdata->int2_src,
		K3G_INT2_SRC_SHIFT, K3G_INT2_SRC_MASK, K3G_CTRL_REG3);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t k3g_show_int2_src(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	struct k3g_chip *chip = indio_dev->dev_data;
	int val = chip->pdata->int2_src >> K3G_INT2_SRC_SHIFT;

	return sprintf(buf, "%d\n", !!(val & this_attr->address));
}

IIO_DEVICE_ATTR(data_rdy, S_IRUGO | S_IWUSR, k3g_show_int2_src,
		  k3g_store_int2_src, K3G_INT2_DATA_READY);
IIO_DEVICE_ATTR(ring_empty, S_IRUGO | S_IWUSR, k3g_show_int2_src,
		  k3g_store_int2_src, K3G_INT2_EMPTY);
IIO_DEVICE_ATTR(ring_100_full, S_IRUGO | S_IWUSR, k3g_show_int2_src,
		  k3g_store_int2_src, K3G_INT2_OVERRUN);
IIO_DEVICE_ATTR(ring_watermark, S_IRUGO | S_IWUSR, k3g_show_int2_src,
		  k3g_store_int2_src, K3G_INT2_WATERMARK);

static ssize_t k3g_store_fifo_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct k3g_chip *chip = indio_dev->dev_data;
	unsigned long val;
	int ret;

	if (!count)
		return -EINVAL;

	ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	ret = k3g_set_8bit_value(chip, val, &chip->pdata->fifo_mode,
		K3G_FIFO_MODE_SHIFT, K3G_FIFO_MODE_MASK, K3G_FIFO_CTRL_REG);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t k3g_show_fifo_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret = chip->pdata->fifo_mode >> K3G_FIFO_MODE_SHIFT;
	int len = 0;

	ret <<= K3G_FIFO_MODE_SHIFT;

	switch (ret) {
	case K3G_FIFO_BYPASS_MODE:
		len = sprintf(buf, "BYPASS_MODE\n");
		break;
	case K3G_FIFO_FIFO_MODE:
		len = sprintf(buf, "FIFO_MODE\n");
		break;
	case K3G_FIFO_STREAM_MODE:
		len = sprintf(buf, "STREAM_MODE\n");
		break;
	case K3G_FIFO_STREAM_TO_FIFO_MODE:
		len = sprintf(buf, "STREAM_TO_FIFO_MODE\n");
		break;
	case K3G_FIFO_BYPASS_TO_STREAM_MODE:
		len = sprintf(buf, "BYPASS_TO_STREAM_MODE\n");
		break;
	default:
		len = sprintf(buf, "NOTHING\n");
		break;
	}

	return len;
}

static IIO_DEVICE_ATTR(fifo_mode, S_IRUGO | S_IWUSR,
		k3g_show_fifo_mode, k3g_store_fifo_mode, 0);

static struct attribute *k3g_fifo_event_attributes[] = {
	&iio_dev_attr_data_rdy.dev_attr.attr,
	&iio_dev_attr_ring_empty.dev_attr.attr,
	&iio_dev_attr_ring_100_full.dev_attr.attr,
	&iio_dev_attr_ring_watermark.dev_attr.attr,
	&iio_dev_attr_fifo_mode.dev_attr.attr,
	NULL
};

static struct attribute_group k3g_event_group[K3G_INT_LINE] = {
	{
		.attrs = k3g_threshold_event_attributes,
	}, {
		.attrs = k3g_fifo_event_attributes,
	}
};

static int k3g_initialize_chip(struct k3g_chip *chip)
{
	u8 value, threshold[6];
	int ret;

	/* CTRL_REG2 */
	value = chip->pdata->hpmode | chip->pdata->hpcf;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_CTRL_REG2, value);
	if (ret < 0)
		return ret;

	/* CTRL_REG3 */
	value = chip->pdata->int1_enable | chip->pdata->int1_boot |
		chip->pdata->int1_hl_active | chip->pdata->int_pp_od |
		chip->pdata->int2_src;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_CTRL_REG3, value);
	if (ret < 0)
		return ret;

	/* CTRL_REG4 */
	value = chip->pdata->block_data_update | chip->pdata->endian |
		chip->pdata->fullscale | chip->pdata->selftest |
		chip->pdata->spi_mode;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_CTRL_REG4, value);
	if (ret < 0)
		return ret;

	/* CTRL_REG5 */
	value = chip->pdata->reboot | chip->pdata->fifo_enable |
		chip->pdata->hp_enable | chip->pdata->int1_sel |
		chip->pdata->out_sel;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_CTRL_REG5, value);
	if (ret < 0)
		return ret;

	/* FIFO_CTRL_REG */
	value = chip->pdata->fifo_mode | chip->pdata->fifo_threshold;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_FIFO_CTRL_REG,
					value);
	if (ret < 0)
		return ret;

	/* INT1_CFG_REG */
	value = chip->pdata->int1_combination | chip->pdata->int1_latch |
		chip->pdata->int1_z_high_enable |
		chip->pdata->int1_z_low_enable |
		chip->pdata->int1_y_high_enable |
		chip->pdata->int1_y_low_enable |
		chip->pdata->int1_x_high_enable |
		chip->pdata->int1_x_low_enable;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_CFG_REG, value);
	if (ret < 0)
		return ret;

	/* INT1_THS_REG */
	threshold[0] = chip->pdata->int1_x_threshold >> BITS_PER_BYTE;
	threshold[1] = chip->pdata->int1_x_threshold & 0xff;
	threshold[2] = chip->pdata->int1_y_threshold >> BITS_PER_BYTE;
	threshold[3] = chip->pdata->int1_y_threshold & 0xff;
	threshold[4] = chip->pdata->int1_z_threshold >> BITS_PER_BYTE;
	threshold[5] = chip->pdata->int1_z_threshold & 0xff;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_XH,
					threshold[0]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_XL,
					threshold[1]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_YH,
					threshold[2]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_YL,
					threshold[3]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_ZH,
					threshold[4]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_THS_ZL,
					threshold[5]);
	if (ret < 0)
		return ret;

	/* INT1_DURATION_REG */
	value = chip->pdata->int1_wait_enable |
		chip->pdata->int1_wait_duration;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_INT1_DURATION_REG,
					value);
	if (ret < 0)
		return ret;

	/* CTRL_REG1 */
	value = chip->pdata->data_rate | chip->pdata->bandwidth |
		chip->pdata->powerdown | chip->pdata->zen |
		chip->pdata->yen | chip->pdata->xen;
	ret = i2c_smbus_write_byte_data(chip->client, K3G_CTRL_REG1, value);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_read_byte_data(chip->client, K3G_CTRL_REG1);
	return 0;
}

#define K3G_EVENT_MASK	(IIO_EV_BIT(IIO_EV_TYPE_THRESH, IIO_EV_DIR_EITHER))

static struct iio_chan_spec k3g_channels[] = {
	IIO_CHAN(IIO_GYRO, 1, 0, 0, NULL, 0, IIO_MOD_X, 0,
		0, 0, IIO_ST('s', 16, 16, 0), K3G_EVENT_MASK),
	IIO_CHAN(IIO_GYRO, 1, 0, 0, NULL, 0, IIO_MOD_Y, 0,
		1, 1, IIO_ST('s', 16, 16, 0), K3G_EVENT_MASK),
	IIO_CHAN(IIO_GYRO, 1, 0, 0, NULL, 0, IIO_MOD_Z, 0,
		2, 2, IIO_ST('s', 16, 16, 0), K3G_EVENT_MASK),
};

static const struct iio_info k3g_info = {
	.driver_module = THIS_MODULE,
	.num_interrupt_lines = K3G_INT_LINE,
	.event_attrs = k3g_event_group,
	.attrs = &k3g_group,
};

static int __devinit k3g_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct k3g_chip *chip;
	int ret, regdone = 0;

	chip = kzalloc(sizeof(struct k3g_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	/* Detect device id */
	ret = i2c_smbus_read_byte_data(client, K3G_WHO_AM_I);
	if (ret != K3G_DEV_ID) {
		dev_err(&client->dev, "failed to detect device id\n");
		goto error_devid_detect;
	}

	chip->pdata = client->dev.platform_data;
	if (!chip->pdata) {
		ret = -ENOENT;
		goto error_no_pdata;
	}
	chip->client = client;

	i2c_set_clientdata(client, chip);
	mutex_init(&chip->lock);
	mutex_init(&chip->ring_lock);

	chip->indio_dev = iio_allocate_device(0);
	if (!chip->indio_dev)
		goto error_allocate_iio;

	chip->indio_dev->name = client->name;
	chip->indio_dev->dev.parent = &client->dev;
	chip->indio_dev->dev_data = (void *)(chip);
	chip->indio_dev->modes = INDIO_DIRECT_MODE;
	chip->indio_dev->info = &k3g_info;

	ret = k3g_configure_ring(chip->indio_dev);
	if (ret)
		goto error_register_iio;

	ret = iio_device_register(chip->indio_dev);
	if (ret)
		goto error_register_ring_buffer;
	regdone = 1;

	ret = iio_ring_buffer_register_ex(chip->indio_dev->ring, 0,
					  k3g_channels,
					  ARRAY_SIZE(k3g_channels));
	if (ret) {
		dev_err(&client->dev, "failed to initialize the ring\n");
		goto error_register_ring_buffer;
	}

	if (client->irq > 0) {
		ret = request_threaded_irq(client->irq,
						NULL,
						&k3g_thresh_handler,
						IRQF_TRIGGER_RISING,
						"K3G INT1",
						chip->indio_dev);
		if (ret)
			goto error_register_interrupt1;
	}

	if (chip->pdata->irq2 > 0) {
		ret = request_threaded_irq(chip->pdata->irq2,
						NULL,
						&k3g_fifo_handler,
						IRQF_TRIGGER_RISING,
						"K3G INT2",
						chip->indio_dev);
		if (ret)
			goto error_register_interrupt2;
	}

	k3g_register_ring_funcs(chip->indio_dev);

	ret = k3g_initialize_chip(chip);
	if (ret)
		goto error_initialize;

	pm_runtime_set_active(&client->dev);

	dev_info(&client->dev, "%s registered\n", id->name);

	return 0;

error_initialize:
	if (chip->pdata->irq2 > 0)
		free_irq(chip->pdata->irq2, chip->indio_dev);
error_register_interrupt2:
	if (client->irq > 0)
		free_irq(client->irq, chip->indio_dev);
error_register_interrupt1:
	iio_ring_buffer_unregister(chip->indio_dev->ring);
error_register_ring_buffer:
	k3g_unconfigure_ring(chip->indio_dev);
error_register_iio:
	if (regdone)
		iio_device_unregister(chip->indio_dev);
	else
		iio_free_device(chip->indio_dev);
error_allocate_iio:
error_no_pdata:
error_devid_detect:
	kfree(chip);
	return ret;
}

static int __devexit k3g_remove(struct i2c_client *client)
{
	struct k3g_chip *chip = i2c_get_clientdata(client);

	disable_irq_nosync(client->irq);
	disable_irq_nosync(chip->pdata->irq2);
	cancel_work_sync(&chip->work_thresh);
	cancel_work_sync(&chip->work_fifo);

	if (client->irq > 0)
		free_irq(client->irq, chip->indio_dev);

	if (chip->pdata->irq2 > 0)
		free_irq(chip->pdata->irq2, chip->indio_dev);

	iio_ring_buffer_unregister(chip->indio_dev->ring);
	k3g_unconfigure_ring(chip->indio_dev);
	iio_device_unregister(chip->indio_dev);
	kfree(chip);

	return 0;
}

#ifdef CONFIG_PM
static int k3g_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct k3g_chip *chip = i2c_get_clientdata(client);

	disable_irq_nosync(client->irq);
	disable_irq_nosync(chip->pdata->irq2);
	cancel_work_sync(&chip->work_thresh);
	cancel_work_sync(&chip->work_fifo);

	k3g_set_8bit_value(chip, false, &chip->pdata->powerdown,
		K3G_POWERDOWN_SHIFT, K3G_POWERDOWN_MASK, K3G_CTRL_REG1);

	return 0;
}

static int k3g_resume(struct i2c_client *client)
{
	struct k3g_chip *chip = i2c_get_clientdata(client);

	k3g_set_8bit_value(chip, true, &chip->pdata->powerdown,
		K3G_POWERDOWN_SHIFT, K3G_POWERDOWN_MASK, K3G_CTRL_REG1);

	enable_irq(client->irq);
	enable_irq(chip->pdata->irq2);

	k3g_initialize_chip(chip);

	return 0;
}
#else
#define k3g_suspend NULL
#define k3g_resume NULL
#endif

static const struct i2c_device_id k3g_id[] = {
	{ "K3G", 0 },
	{ "K3G_1", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, k3g_id);

static struct i2c_driver k3g_i2c_driver = {
	.driver = {
		.name	= "K3G",
	},
	.probe		= k3g_probe,
	.remove		= __exit_p(k3g_remove),
	.suspend	= k3g_suspend,
	.resume		= k3g_resume,
	.id_table	= k3g_id,
};

static int __init k3g_init(void)
{
	return i2c_add_driver(&k3g_i2c_driver);
}
module_init(k3g_init);

static void __exit k3g_exit(void)
{
	i2c_del_driver(&k3g_i2c_driver);
}
module_exit(k3g_exit);

MODULE_AUTHOR("Donggeun Kim <dg77.kim@samsung.com>");
MODULE_DESCRIPTION("K3G Gyroscope sensor driver");
MODULE_LICENSE("GPL");
