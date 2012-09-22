/*
 *  k3g_ring.c - ST Microelectronics three-axis gyroscope sensor
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

#include "../iio.h"
#include "../sysfs.h"
#include "../ring_generic.h"
#include "../ring_hw.h"
#include "gyro.h"
#include "k3g.h"

/**
 * k3g_rip_hw_rb() - main ring access function, pulls data from ring
 * @r:			the ring
 * @count:		number of samples to try and pull
 * @data:		output the actual samples pulled from the hw ring
 *
 * Currently does not provide timestamps.  As the hardware doesn't add them they
 * can only be inferred aproximately from ring buffer events such as 50% full
 * and knowledge of when buffer was last emptied.  This is left to userspace.
 **/
static int k3g_read_first_n_hw_rb(struct iio_ring_buffer *r,
			     size_t count, char __user *buf)
{
	struct iio_hw_ring_buffer *hw_ring = iio_to_hw_ring_buf(r);
	struct iio_dev *indio_dev = hw_ring->private;
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret = 0, scan_size, i;
	int bytes_per_sample = 2, num_channels = 3;
	u8 *data;

	mutex_lock(&chip->ring_lock);
	scan_size = bytes_per_sample * num_channels;
	if ((count % scan_size) || (count < scan_size)) {
		ret = -EINVAL;
		goto error_ret;
	}

	data = kzalloc(count, GFP_KERNEL);
	if (data == NULL) {
		ret = -ENOMEM;
		goto error_ret;
	}

	for (i = 0 ; i < count / scan_size; i++)
		ret = i2c_smbus_read_i2c_block_data(chip->client,
			K3G_MULTI_OUT_X_L, 6, data + (i * scan_size));

	if (copy_to_user(buf, data, count))
		ret = -EFAULT;
	kfree(data);
	r->stufftoread = 0;
error_ret:
	mutex_unlock(&chip->ring_lock);

	return (ret < 0) ? ret : count;
}

static int k3g_ring_get_length(struct iio_ring_buffer *r)
{
	return 32;
}

static int k3g_ring_get_bytes_per_datum(struct iio_ring_buffer *r)
{
	return 6;
}
static void k3g_ring_release(struct device *dev)
{
	struct iio_ring_buffer *r = to_iio_ring_buffer(dev);
	kfree(iio_to_hw_ring_buf(r));
}

static IIO_RING_ENABLE_ATTR;
static IIO_RING_BYTES_PER_DATUM_ATTR;
static IIO_RING_LENGTH_ATTR;

static ssize_t k3g_show_ring_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *indio_dev = ring->indio_dev;
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret = k3g_get_8bit_value(chip, K3G_FIFO_STORED_SHIFT,
			K3G_FIFO_STORED_MASK, K3G_FIFO_SRC_REG);
	return sprintf(buf, "%d\n", ret);
}
static IIO_DEVICE_ATTR(ring_level, S_IRUGO,
				k3g_show_ring_level, NULL, 0);

static ssize_t k3g_store_watermark_level(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *indio_dev = ring->indio_dev;
	struct k3g_chip *chip = indio_dev->dev_data;
	unsigned long val;
	int ret;

	if (!count)
		return -EINVAL;

	ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	ret = k3g_set_8bit_value(chip, val,
		(u8 *) &chip->pdata->fifo_threshold, K3G_WATERMARK_THRES_SHIFT,
		K3G_WATERMARK_THRES_MASK, K3G_FIFO_CTRL_REG);

	if (ret < 0)
		return ret;

	return count;
}
static ssize_t k3g_show_watermark_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *indio_dev = ring->indio_dev;
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret = chip->pdata->fifo_threshold >> K3G_WATERMARK_THRES_SHIFT;

	return sprintf(buf, "%d\n", ret);
}
static IIO_DEVICE_ATTR(watermark_level, S_IRUGO | S_IWUSR,
		k3g_show_watermark_level, k3g_store_watermark_level, 0);

/*
 * Ring buffer attributes
 * This device is a bit unusual in that the sampling frequency and bpse
 * only apply to the ring buffer.  At all times full rate and accuracy
 * is available via direct reading from registers.
 */
static struct attribute *k3g_ring_attributes[] = {
	&dev_attr_length.attr,
	&dev_attr_bytes_per_datum.attr,
	&dev_attr_enable.attr,
	&iio_dev_attr_ring_level.dev_attr.attr,
	&iio_dev_attr_watermark_level.dev_attr.attr,
	NULL,
};

static struct attribute_group k3g_ring_attr = {
	.attrs = k3g_ring_attributes,
};

static const struct attribute_group *k3g_ring_attr_groups[] = {
	&k3g_ring_attr,
	NULL
};

static struct device_type k3g_ring_type = {
	.release = k3g_ring_release,
	.groups = k3g_ring_attr_groups,
};

static struct iio_ring_buffer *k3g_rb_allocate(struct iio_dev *indio_dev)
{
	struct iio_ring_buffer *buf;
	struct iio_hw_ring_buffer *ring;

	ring = kzalloc(sizeof *ring, GFP_KERNEL);
	if (!ring)
		return NULL;

	ring->private = indio_dev;
	buf = &ring->buf;
	buf->stufftoread = 0;
	iio_ring_buffer_init(buf, indio_dev);
	buf->dev.type = &k3g_ring_type;
	buf->dev.parent = &indio_dev->dev;
	dev_set_drvdata(&buf->dev, (void *)buf);

	return buf;
}

static inline void k3g_rb_free(struct iio_ring_buffer *r)
{
	if (r)
		iio_put_ring_buffer(r);
}

static const struct iio_ring_access_funcs k3g_ring_access_funcs = {
	.read_first_n = &k3g_read_first_n_hw_rb,
	.get_length = &k3g_ring_get_length,
	.get_bytes_per_datum = &k3g_ring_get_bytes_per_datum,
};

int k3g_configure_ring(struct iio_dev *indio_dev)
{
	indio_dev->ring = k3g_rb_allocate(indio_dev);
	if (indio_dev->ring == NULL)
		return -ENOMEM;
	indio_dev->modes |= INDIO_RING_HARDWARE_BUFFER;

	indio_dev->ring->access = &k3g_ring_access_funcs;

	iio_scan_mask_set(indio_dev->ring, 0);
	iio_scan_mask_set(indio_dev->ring, 1);
	iio_scan_mask_set(indio_dev->ring, 2);

	return 0;
}

void k3g_unconfigure_ring(struct iio_dev *indio_dev)
{
	k3g_rb_free(indio_dev->ring);
}

static inline
int __k3g_hw_ring_state_set(struct iio_dev *indio_dev, bool state)
{
	struct k3g_chip *chip = indio_dev->dev_data;
	int ret;

	ret = k3g_set_8bit_value(chip, state,
		&chip->pdata->fifo_enable,
		K3G_FIFO_EN_SHIFT, K3G_FIFO_EN, K3G_CTRL_REG5);

	return ret;
}
/**
 * k3g_hw_ring_preenable() hw ring buffer preenable function
 *
 * Very simple enable function as the chip will allows normal reads
 * during ring buffer operation so as long as it is indeed running
 * before we notify the core, the precise ordering does not matter.
 **/
static int k3g_hw_ring_preenable(struct iio_dev *indio_dev)
{
	return __k3g_hw_ring_state_set(indio_dev, 1);
}

static int k3g_hw_ring_postdisable(struct iio_dev *indio_dev)
{
	return __k3g_hw_ring_state_set(indio_dev, 0);
}

static const struct iio_ring_setup_ops k3g_ring_setup_ops = {
	.preenable = &k3g_hw_ring_preenable,
	.postdisable = &k3g_hw_ring_postdisable,
};

void k3g_register_ring_funcs(struct iio_dev *indio_dev)
{
	indio_dev->ring->setup_ops = &k3g_ring_setup_ops;
}

/**
 * k3g_ring_int_process() ring specific interrupt handling.
 *
 * This is only split from the main interrupt handler so as to
 * reduce the amount of code if the ring buffer is not enabled.
 **/
void k3g_ring_int_process(int val, struct iio_ring_buffer *ring)
{
	if (val & (K3G_EMPTY | K3G_OVERRUN | K3G_WATERMARK)) {
		ring->stufftoread = true;
		wake_up_interruptible(&ring->pollq);
	}
}
