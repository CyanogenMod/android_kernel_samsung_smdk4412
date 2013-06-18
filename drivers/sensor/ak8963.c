/*
 *	Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
*/

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/sensor/ak8963.h>
#include <linux/completion.h>
#include "ak8963-reg.h"
#include <linux/sensor/sensors_core.h>

#if defined(CONFIG_SLP) || defined(CONFIG_MACH_GC1)\
	|| defined(CONFIG_MACH_M3_USA_TMO)
#define FACTORY_TEST
#else
#undef FACTORY_TEST
#endif
#undef MAGNETIC_LOGGING

#define VENDOR		"AKM"
#define CHIP_ID		"AK8963C"


struct akm8963_data {
	struct i2c_client *this_client;
	struct akm8963_platform_data *pdata;
	struct mutex lock;
	struct miscdevice akmd_device;
	struct completion data_ready;
	struct device *dev;
	wait_queue_head_t state_wq;
	u8 asa[3];
	int irq;
};


static s32 akm8963_ecs_set_mode_power_down(struct akm8963_data *akm)
{
	s32 ret;
	ret = i2c_smbus_write_byte_data(akm->this_client,
			AK8963_REG_CNTL1, AK8963_CNTL1_POWER_DOWN);
	return ret;
}

static int akm8963_ecs_set_mode(struct akm8963_data *akm, char mode)
{
	s32 ret;

	switch (mode) {
	case AK8963_CNTL1_SNG_MEASURE:
		ret = i2c_smbus_write_byte_data(akm->this_client,
				AK8963_REG_CNTL1, AK8963_CNTL1_SNG_MEASURE);
		break;
	case AK8963_CNTL1_FUSE_ACCESS:
		ret = i2c_smbus_write_byte_data(akm->this_client,
				AK8963_REG_CNTL1, AK8963_CNTL1_FUSE_ACCESS);
		break;
	case AK8963_CNTL1_POWER_DOWN:
		ret = akm8963_ecs_set_mode_power_down(akm);
		break;
	case AK8963_CNTL1_SELF_TEST:
		ret = i2c_smbus_write_byte_data(akm->this_client,
				AK8963_REG_CNTL1, AK8963_CNTL1_SELF_TEST);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	/* Wait at least 300us after changing mode. */
	udelay(300);

	return 0;
}

static int akm8963_Reset(struct akm8963_data *akm)
{
	int err = 0;

	gpio_set_value(GPIO_MSENSE_RST_N, 0);
	udelay(5);
	gpio_set_value(GPIO_MSENSE_RST_N, 1);
	/* Device will be accessible 100 us after */
	udelay(100);

	return err;
}

static int akmd_copy_in(unsigned int cmd, void __user *argp,
			void *buf, size_t buf_size)
{
	if (!(cmd & IOC_IN))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_from_user(buf, argp, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static int akmd_copy_out(unsigned int cmd, void __user *argp,
			 void *buf, size_t buf_size)
{
	if (!(cmd & IOC_OUT))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_to_user(argp, buf, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static void akm8963_disable_irq(struct akm8963_data *akm)
{
	disable_irq(akm->irq);
	if (try_wait_for_completion(&akm->data_ready)) {
		/* we actually got the interrupt before we could disable it
		 * so we need to enable again to undo our disable since the
		 * irq_handler already disabled it
		 */
		enable_irq(akm->irq);
	}
}

static irqreturn_t akm8963_irq_handler(int irq, void *data)
{
	struct akm8963_data *akm = data;
	disable_irq_nosync(irq);
	complete(&akm->data_ready);
	return IRQ_HANDLED;
}

static int akm8963_wait_for_data_ready(struct akm8963_data *akm)
{
	int data_ready = gpio_get_value(akm->pdata->gpio_data_ready_int);
	int err;

	if (data_ready)
		return 0;

	enable_irq(akm->irq);

	err = wait_for_completion_timeout(&akm->data_ready, 2*HZ);
	if (err > 0)
		return 0;

	akm8963_disable_irq(akm);

	if (err == 0) {
		pr_err("akm: wait timed out\n");
		return -ETIMEDOUT;
	}

	pr_err("akm: wait restart\n");
	return err;
}

static ssize_t akmd_read(struct file *file, char __user *buf,
					size_t count, loff_t *pos)
{
	struct akm8963_data *akm = container_of(file->private_data,
			struct akm8963_data, akmd_device);
	short x = 0, y = 0, z = 0;
	int ret;
	u8 data[8];

	mutex_lock(&akm->lock);
	ret = akm8963_ecs_set_mode(akm, AK8963_CNTL1_SNG_MEASURE);
	if (ret) {
		mutex_unlock(&akm->lock);
		goto done;
	}
	ret = akm8963_wait_for_data_ready(akm);
	if (ret) {
		mutex_unlock(&akm->lock);
		goto done;
	}
	ret = i2c_smbus_read_i2c_block_data(akm->this_client, AK8963_REG_ST1,
						sizeof(data), data);
	mutex_unlock(&akm->lock);

	if (ret != sizeof(data)) {
		pr_err("%s: failed to read %d bytes of mag data\n",
		       __func__, sizeof(data));
		goto done;
	}

	if (data[0] & 0x01) {
		x = (data[2] << 8) + data[1];
		y = (data[4] << 8) + data[3];
		z = (data[6] << 8) + data[5];
	} else
		pr_err("%s: invalid raw data(st1 = %d)\n",
					__func__, data[0] & 0x01);

done:
	return sprintf(buf, "%d,%d,%d\n", x, y, z);
}

static long akmd_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct akm8963_data *akm = container_of(file->private_data,
			struct akm8963_data, akmd_device);
	int ret;
	#ifdef MAGNETIC_LOGGING
	short x, y, z;
	#endif
	union {
		char raw[RWBUF_SIZE];
		int status;
		char mode;
		u8 data[8];
	} rwbuf;

	ret = akmd_copy_in(cmd, argp, rwbuf.raw, sizeof(rwbuf));
	if (ret)
		return ret;

	switch (cmd) {
	case ECS_IOCTL_WRITE:
		if ((rwbuf.raw[0] < 2) || (rwbuf.raw[0] > (RWBUF_SIZE - 1)))
			return -EINVAL;
		if (copy_from_user(&rwbuf.raw[2], argp+2, rwbuf.raw[0]-1))
			return -EFAULT;

		ret = i2c_smbus_write_i2c_block_data(akm->this_client,
						     rwbuf.raw[1],
						     rwbuf.raw[0] - 1,
						     &rwbuf.raw[2]);
		break;
	case ECS_IOCTL_READ:
		if ((rwbuf.raw[0] < 1) || (rwbuf.raw[0] > (RWBUF_SIZE - 1)))
			return -EINVAL;

		ret = i2c_smbus_read_i2c_block_data(akm->this_client,
						    rwbuf.raw[1],
						    rwbuf.raw[0],
						    &rwbuf.raw[1]);
		if (ret < 0)
			return ret;
		if (copy_to_user(argp+1, rwbuf.raw+1, rwbuf.raw[0]))
			return -EFAULT;
		return 0;
	case ECS_IOCTL_SET_MODE:
		mutex_lock(&akm->lock);
		ret = akm8963_ecs_set_mode(akm, rwbuf.mode);
		mutex_unlock(&akm->lock);
		break;
	case ECS_IOCTL_RESET:
		akm8963_Reset(akm);
		break;
	case ECS_IOCTL_GETDATA:
		mutex_lock(&akm->lock);
		ret = akm8963_wait_for_data_ready(akm);
		if (ret) {
			mutex_unlock(&akm->lock);
			return ret;
		}
		ret = i2c_smbus_read_i2c_block_data(akm->this_client,
						    AK8963_REG_ST1,
						    sizeof(rwbuf.data),
						    rwbuf.data);

		#ifdef MAGNETIC_LOGGING
		x = (rwbuf.data[2] << 8) + rwbuf.data[1];
		y = (rwbuf.data[4] << 8) + rwbuf.data[3];
		z = (rwbuf.data[6] << 8) + rwbuf.data[5];

		pr_info("%s:ST1=%d, x=%d, y=%d, z=%d, ST2=%d\n",
			__func__, rwbuf.data[0], x, y, z, rwbuf.data[7]);
		#endif

		mutex_unlock(&akm->lock);
		if (ret != sizeof(rwbuf.data)) {
			pr_err("%s : failed to read %d bytes of mag data\n",
			       __func__, sizeof(rwbuf.data));
			return -EIO;
		}
		break;
	default:
		return -ENOTTY;
	}

	if (ret < 0)
		return ret;

	return akmd_copy_out(cmd, argp, rwbuf.raw, sizeof(rwbuf));
}

static const struct file_operations akmd_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.read = akmd_read,
	.unlocked_ioctl = akmd_ioctl,
};

static int akm8963_setup_irq(struct akm8963_data *akm)
{
	int rc = -EIO;
	struct akm8963_platform_data *pdata = akm->pdata;
	int irq;

	if (akm->this_client->irq)
		irq = akm->this_client->irq;
	else {
		rc = gpio_request(pdata->gpio_data_ready_int, "gpio_akm_int");
		if (rc < 0) {
			pr_err("%s: gpio %d request failed (%d)\n",
				__func__, pdata->gpio_data_ready_int, rc);
			return rc;
		}

		rc = gpio_direction_input(pdata->gpio_data_ready_int);
		if (rc < 0) {
			pr_err("%s: failed to set gpio %d as input (%d)\n",
				__func__, pdata->gpio_data_ready_int, rc);
			goto err_request_irq;
		}

		irq = gpio_to_irq(pdata->gpio_data_ready_int);
	}
	/* trigger high so we don't miss initial interrupt if it
	 * is already pending
	 */
	rc = request_irq(irq, akm8963_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, "akm_int", akm);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->gpio_data_ready_int, rc);
		goto err_request_irq;
	}

	/* start with interrupt disabled until the driver is enabled */
	akm->irq = irq;
	akm8963_disable_irq(akm);

	goto done;

err_request_irq:
	gpio_free(pdata->gpio_data_ready_int);
done:
	return rc;
}

#ifdef FACTORY_TEST
static int ak8963c_selftest(struct akm8963_data *ak_data, int *sf)
{
	u8 buf[6];
	s16 x, y, z;
	int retry_count = 0;

retry:
	/* read device info */
	i2c_smbus_read_i2c_block_data(ak_data->this_client,
					AK8963_REG_WIA, 2, buf);
	pr_info("%s: device id = 0x%x, info = 0x%x\n",
		__func__, buf[0], buf[1]);

	/* set ATSC self test bit to 1 */
	i2c_smbus_write_byte_data(ak_data->this_client,
					AK8963_REG_ASTC, 0x40);

	/* start self test */
	i2c_smbus_write_byte_data(ak_data->this_client,
					AK8963_REG_CNTL1,
					AK8963_CNTL1_SELF_TEST);

	/* wait for data ready */
	while (1) {
		msleep(20);
		if (i2c_smbus_read_byte_data(ak_data->this_client,
						AK8963_REG_ST1) == 1) {
			break;
		}
	}

	i2c_smbus_read_i2c_block_data(ak_data->this_client,
					AK8963_REG_HXL, sizeof(buf), buf);

	/* set ATSC self test bit to 0 */
	i2c_smbus_write_byte_data(ak_data->this_client,
					AK8963_REG_ASTC, 0x00);

	x = buf[0] | (buf[1] << 8);
	y = buf[2] | (buf[3] << 8);
	z = buf[4] | (buf[5] << 8);

	/* Hadj = (H*(Asa+128))/256 */
	x = (x*(ak_data->asa[0] + 128)) >> 8;
	y = (y*(ak_data->asa[1] + 128)) >> 8;
	z = (z*(ak_data->asa[2] + 128)) >> 8;

	pr_info("%s: self test x = %d, y = %d, z = %d\n",
		__func__, x, y, z);
	if ((x >= -200) && (x <= 200))
		pr_info("%s: x passed self test, expect -200<=x<=200\n",
			__func__);
	else
		pr_info("%s: x failed self test, expect -200<=x<=200\n",
			__func__);
	if ((y >= -200) && (y <= 200))
		pr_info("%s: y passed self test, expect -200<=y<=200\n",
			__func__);
	else
		pr_info("%s: y failed self test, expect -200<=y<=200\n",
			__func__);
	if ((z >= -3200) && (z <= -800))
		pr_info("%s: z passed self test, expect -3200<=z<=-800\n",
			__func__);
	else
		pr_info("%s: z failed self test, expect -3200<=z<=-800\n",
			__func__);

	sf[0] = x;
	sf[1] = y;
	sf[2] = z;

	if (((x >= -200) && (x <= 200)) &&
		((y >= -200) && (y <= 200)) &&
		((z >= -3200) && (z <= -800))) {
		pr_info("%s, Selftest is successful.\n", __func__);
		return 1;
	} else {
		if (retry_count < 5) {
			retry_count++;
			pr_warn("############################################");
			pr_warn("%s, retry_count=%d\n", __func__, retry_count);
			pr_warn("############################################");
			goto retry;
		} else {
			pr_err("%s, Selftest is failed.\n", __func__);
			return 0;
		}
	}
}

static ssize_t ak8963c_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct akm8963_data *ak_data  = dev_get_drvdata(dev);

	return sprintf(buf, "%d, %d, %d\n", ak_data->asa[0],
		ak_data->asa[1], ak_data->asa[2]);
}

static ssize_t ak8963c_get_selftest(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int sf[3] = {0,};

	ret = ak8963c_selftest(dev_get_drvdata(dev), sf);
	return sprintf(buf, "%d, %d, %d, %d\n", ret, sf[0], sf[1], sf[2]);
}

static ssize_t ak8963c_check_registers(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	struct akm8963_data *ak_data  = dev_get_drvdata(dev);
	u8 buf[13];

	/* power down */
	i2c_smbus_write_byte_data(ak_data->this_client,
		AK8963_REG_CNTL1, AK8963_CNTL1_POWER_DOWN);

	/* get the value */
	i2c_smbus_read_i2c_block_data(ak_data->this_client,
					AK8963_REG_WIA, 11, buf);

	buf[11] = i2c_smbus_read_byte_data(ak_data->this_client,
					AK8963_REG_ASTC);
	buf[12] = i2c_smbus_read_byte_data(ak_data->this_client,
					AK8963_REG_I2CDIS);


	return sprintf(strbuf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
			buf[6], buf[7], buf[8], buf[9], buf[10], buf[11],
			buf[12]);
}

static ssize_t ak8963c_check_cntl(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	struct akm8963_data *ak_data  = dev_get_drvdata(dev);
	u8 buf;
	int err;

	/* power down */
	err = i2c_smbus_write_byte_data(ak_data->this_client,
		AK8963_REG_CNTL1, AK8963_CNTL1_POWER_DOWN);

	buf = i2c_smbus_read_byte_data(ak_data->this_client,
					AK8963_REG_CNTL1);

	return sprintf(strbuf, "%s\n",
		((buf == AK8963_CNTL1_POWER_DOWN) ? "OK" : "NG"));
}

static ssize_t ak8963c_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct akm8963_data *ak_data  = dev_get_drvdata(dev);
	int success;

	if ((ak_data->asa[0] == 0) | (ak_data->asa[0] == 0xff)
		| (ak_data->asa[1] == 0) | (ak_data->asa[1] == 0xff)
		| (ak_data->asa[2] == 0) | (ak_data->asa[2] == 0xff))
		success = 0;
	else
		success = 1;

	return sprintf(buf, "%s\n", (success ? "OK" : "NG"));
}

static ssize_t ak8963_adc(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	struct akm8963_data *ak_data  = dev_get_drvdata(dev);
	u8 buf[8];
	s16 x, y, z;
	int err, success;

	/* start ADC conversion */
	err = i2c_smbus_write_byte_data(ak_data->this_client,
			AK8963_REG_CNTL1, AK8963_CNTL1_SNG_MEASURE);

	/* wait for ADC conversion to complete */
	err = akm8963_wait_for_data_ready(ak_data);
	if (err) {
		pr_err("%s: wait for data ready failed\n", __func__);
		return err;
	}
	msleep(20);
	/* get the value and report it */
	err = i2c_smbus_read_i2c_block_data(ak_data->this_client,
					AK8963_REG_ST1, sizeof(buf), buf);
	if (err != sizeof(buf)) {
		pr_err("%s: read data over i2c failed\n", __func__);
		return err;
	}

	/* buf[0] is status1, buf[7] is status2 */
	if ((buf[0] == 0) | (buf[7] == 1))
		success = 0;
	else
		success = 1;

	x = buf[1] | (buf[2] << 8);
	y = buf[3] | (buf[4] << 8);
	z = buf[5] | (buf[6] << 8);

	pr_info("raw x = %d, y = %d, z = %d\n", x, y, z);
	return sprintf(strbuf,
		"%s, %d, %d, %d\n", (success ? "OK" : "NG"), x, y, z);
}
#endif

static ssize_t ak8963_show_raw_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct akm8963_data *akm = dev_get_drvdata(dev);
	short x = 0, y = 0, z = 0;
	int ret;
	u8 data[8] = {0,};

	mutex_lock(&akm->lock);
	ret = akm8963_ecs_set_mode(akm, AK8963_CNTL1_SNG_MEASURE);
	if (ret) {
		mutex_unlock(&akm->lock);
		goto done;
	}
	ret = akm8963_wait_for_data_ready(akm);
	if (ret) {
		mutex_unlock(&akm->lock);
		goto done;
	}
	ret = i2c_smbus_read_i2c_block_data(akm->this_client, AK8963_REG_ST1,
						sizeof(data), data);
	mutex_unlock(&akm->lock);

	if (ret != sizeof(data)) {
		pr_err("%s: failed to read %d bytes of mag data\n",
			   __func__, sizeof(data));
		goto done;
	}

	if (data[0] & 0x01) {
		x = (data[2] << 8) + data[1];
		y = (data[4] << 8) + data[3];
		z = (data[6] << 8) + data[5];
	} else
		pr_err("%s: invalid raw data(st1 = %d)\n",
					__func__, data[0] & 0x01);

done:
	return sprintf(buf, "%d,%d,%d\n", x, y, z);
}

static ssize_t ak8963_show_vendor(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t ak8963_show_name(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(raw_data, 0664,
		ak8963_show_raw_data, NULL);
static DEVICE_ATTR(vendor, 0664,
		ak8963_show_vendor, NULL);
static DEVICE_ATTR(name, 0664,
		ak8963_show_name, NULL);

#ifdef FACTORY_TEST
static DEVICE_ATTR(asa, 0664,
		ak8963c_get_asa, NULL);
static DEVICE_ATTR(selftest, 0664,
		ak8963c_get_selftest, NULL);
static DEVICE_ATTR(chk_registers, 0664,
		ak8963c_check_registers, NULL);
static DEVICE_ATTR(dac, 0664,
		ak8963c_check_cntl, NULL);
static DEVICE_ATTR(status, 0664,
		ak8963c_get_status, NULL);
static DEVICE_ATTR(adc, 0664,
		ak8963_adc, NULL);
#endif

int akm8963_probe(struct i2c_client *client,
		const struct i2c_device_id *devid)
{
	struct akm8963_data *akm;
	int err;

	pr_info("%s is called.\n", __func__);
	if (client->dev.platform_data == NULL && client->irq == 0) {
		dev_err(&client->dev, "platform data & irq are NULL.\n");
		err = -ENODEV;
		goto exit_platform_data_null;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check failed, exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	akm = kzalloc(sizeof(struct akm8963_data), GFP_KERNEL);
	if (!akm) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	akm->pdata = client->dev.platform_data;
	mutex_init(&akm->lock);
	init_completion(&akm->data_ready);

	i2c_set_clientdata(client, akm);
	akm->this_client = client;

	err = akm8963_ecs_set_mode_power_down(akm);
	if (err < 0) {
		pr_err("%s: akm8963_ecs_set_mode_power_down fail(err=%d)\n",
			__func__, err);
		goto exit_set_mode_power_down_failed;
	}

	err = akm8963_setup_irq(akm);
	if (err) {
		pr_err("%s: could not setup irq\n", __func__);
		goto exit_setup_irq;
	}

	akm->akmd_device.minor = MISC_DYNAMIC_MINOR;
	akm->akmd_device.name = "akm8963";
	akm->akmd_device.fops = &akmd_fops;

	err = misc_register(&akm->akmd_device);
	if (err) {
		pr_err("%s, misc_register failed.\n", __func__);
		goto exit_akmd_device_register_failed;
	}

	init_waitqueue_head(&akm->state_wq);

	/* put into fuse access mode to read asa data */
	err = i2c_smbus_write_byte_data(client, AK8963_REG_CNTL1,
					AK8963_CNTL1_FUSE_ACCESS);
	if (err) {
		pr_err("%s: unable to enter fuse rom mode\n", __func__);
		goto exit_i2c_failed;
	}

	err = i2c_smbus_read_i2c_block_data(client, AK8963_FUSE_ASAX,
					sizeof(akm->asa), akm->asa);
	if (err != sizeof(akm->asa)) {
		pr_err("%s: unable to load factory sensitivity adjust values\n",
			__func__);
		goto exit_i2c_failed;
	} else
		pr_info("%s: asa_x = %d, asa_y = %d, asa_z = %d\n", __func__,
			akm->asa[0], akm->asa[1], akm->asa[2]);

	err = i2c_smbus_write_byte_data(client, AK8963_REG_CNTL1,
					AK8963_CNTL1_POWER_DOWN);
	if (err) {
		dev_err(&client->dev, "Error in setting power down mode\n");
		goto exit_i2c_failed;
	}

	akm->dev = sensors_classdev_register("magnetic_sensor");
	if (IS_ERR(akm->dev)) {
		pr_err("Failed to create device!");
		goto exit_class_create_failed;
	}

	if (device_create_file(akm->dev, &dev_attr_raw_data) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_raw_data.attr.name);
		goto exit_device_create_raw_data;
	}

	if (device_create_file(akm->dev, &dev_attr_vendor) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_name.attr.name);
		goto exit_device_create_vendor;
	}

	if (device_create_file(akm->dev, &dev_attr_name) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_raw_data.attr.name);
		goto exit_device_create_name;
	}

#ifdef FACTORY_TEST
	if (device_create_file(akm->dev, &dev_attr_adc) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_adc.attr.name);
		goto exit_device_create_file1;
	}

	if (device_create_file(akm->dev, &dev_attr_status) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_status.attr.name);
		goto exit_device_create_file2;
	}

	if (device_create_file(akm->dev, &dev_attr_asa) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_asa.attr.name);
		goto exit_device_create_file3;
	}
	if (device_create_file(akm->dev, &dev_attr_selftest) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_selftest.attr.name);
		goto exit_device_create_file4;
	}
	if (device_create_file(akm->dev,
		&dev_attr_chk_registers) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_chk_registers.attr.name);
		goto exit_device_create_file5;
	}
	if (device_create_file(akm->dev, &dev_attr_dac) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_dac.attr.name);
		goto exit_device_create_file6;
	}
#endif
	dev_set_drvdata(akm->dev, akm);

pr_info("%s is successful.\n", __func__);
return 0;

#ifdef FACTORY_TEST
exit_device_create_file6:
	device_remove_file(akm->dev, &dev_attr_chk_registers);
exit_device_create_file5:
	device_remove_file(akm->dev, &dev_attr_selftest);
exit_device_create_file4:
	device_remove_file(akm->dev, &dev_attr_asa);
exit_device_create_file3:
	device_remove_file(akm->dev, &dev_attr_status);
exit_device_create_file2:
	device_remove_file(akm->dev, &dev_attr_adc);
exit_device_create_file1:
	device_remove_file(akm->dev, &dev_attr_name);
#endif
exit_device_create_name:
	device_remove_file(akm->dev, &dev_attr_vendor);
exit_device_create_vendor:
	device_remove_file(akm->dev, &dev_attr_raw_data);
exit_device_create_raw_data:
	sensors_classdev_unregister(akm->dev);
exit_class_create_failed:
exit_i2c_failed:
	misc_deregister(&akm->akmd_device);
exit_akmd_device_register_failed:
	free_irq(akm->irq, akm);
	gpio_free(akm->pdata->gpio_data_ready_int);
exit_setup_irq:
exit_set_mode_power_down_failed:
	mutex_destroy(&akm->lock);
	kfree(akm);
exit_alloc_data_failed:
exit_check_functionality_failed:
exit_platform_data_null:
	return err;
}

static int __devexit akm8963_remove(struct i2c_client *client)
{
	struct akm8963_data *akm = i2c_get_clientdata(client);

	#ifdef FACTORY_TEST
	device_remove_file(akm->dev, &dev_attr_adc);
	device_remove_file(akm->dev, &dev_attr_status);
	device_remove_file(akm->dev, &dev_attr_asa);
	device_remove_file(akm->dev, &dev_attr_selftest);
	device_remove_file(akm->dev, &dev_attr_chk_registers);
	device_remove_file(akm->dev, &dev_attr_dac);
	#endif
	device_remove_file(akm->dev, &dev_attr_name);
	device_remove_file(akm->dev, &dev_attr_vendor);
	device_remove_file(akm->dev, &dev_attr_raw_data);
	sensors_classdev_unregister(akm->dev);
	misc_deregister(&akm->akmd_device);
	free_irq(akm->irq, akm);
	gpio_free(akm->pdata->gpio_data_ready_int);
	mutex_destroy(&akm->lock);
	kfree(akm);
	return 0;
}

static const struct i2c_device_id akm8963_id[] = {
	{AKM8963_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver akm8963_driver = {
	.probe		= akm8963_probe,
	.remove		= akm8963_remove,
	.id_table	= akm8963_id,
	.driver = {
		.name = AKM8963_I2C_NAME,
	},
};

static int __init akm8963_init(void)
{
	return i2c_add_driver(&akm8963_driver);
}

static void __exit akm8963_exit(void)
{
	i2c_del_driver(&akm8963_driver);
}

module_init(akm8963_init);
module_exit(akm8963_exit);

MODULE_DESCRIPTION("AKM8963 compass driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
