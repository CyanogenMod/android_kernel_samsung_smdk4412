/*
 *  STMicroelectronics k3dh acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/k3dh.h>
#include "k3dh_reg.h"
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
#include <linux/input.h>
#endif

/* For Debugging */
#if 1
#define k3dh_dbgmsg(str, args...) pr_debug("%s: " str, __func__, ##args)
#endif
#define k3dh_infomsg(str, args...) pr_info("%s: " str, __func__, ##args)

#define VENDOR		"STM"
#define CHIP_ID		"K3DH"

/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 */
#define DEFAULT_POWER_ON_SETTING (ODR400 | ENABLE_ALL_AXES)
#define ACC_DEV_MAJOR 241

#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#define CAL_DATA_AMOUNT	20

#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
/* ABS axes parameter range [um/s^2] (for input event) */
#define GRAVITY_EARTH		9806550
#define ABSMAX_2G		(GRAVITY_EARTH * 2)
#define ABSMIN_2G		(-GRAVITY_EARTH * 2)
#define MIN_DELAY	5
#define MAX_DELAY	200
#endif

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	s64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{ ODR1344,     744047LL }, /* 1344Hz */
	{  ODR400,    2500000LL }, /*  400Hz */
	{  ODR200,    5000000LL }, /*  200Hz */
	{  ODR100,   10000000LL }, /*  100Hz */
	{   ODR50,   20000000LL }, /*   50Hz */
	{   ODR25,   40000000LL }, /*   25Hz */
	{   ODR10,  100000000LL }, /*   10Hz */
	{    ODR1, 1000000000LL }, /*    1Hz */
};

/* K3DH acceleration data */
struct k3dh_acc {
	s16 x;
	s16 y;
	s16 z;
};

struct k3dh_data {
	struct i2c_client *client;
	struct miscdevice k3dh_device;
	struct mutex read_lock;
	struct mutex write_lock;
	struct completion data_ready;
#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
	struct class *acc_class;
#else
	struct device *dev;
#endif
	struct k3dh_acc cal_data;
	struct k3dh_acc acc_xyz;
	u8 ctrl_reg1_shadow;
	atomic_t opened; /* opened implies enabled */
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
	struct input_dev *input;
	struct delayed_work work;
	atomic_t delay;
	atomic_t enable;
#endif
	bool axis_adjust;
	int position;
};

static struct k3dh_data *g_k3dh;


static void k3dh_xyz_position_adjust(struct k3dh_acc *acc,
		int position)
{
	const int position_map[][3][3] = {
	{{ 0,  1,  0}, {-1,  0,  0}, { 0,  0,  1} }, /* 0 top/upper-left */
	{{-1,  0,  0}, { 0, -1,  0}, { 0,  0,  1} }, /* 1 top/upper-right */
	{{ 0, -1,  0}, { 1,  0,  0}, { 0,  0,  1} }, /* 2 top/lower-right */
	{{ 1,  0,  0}, { 0,  1,  0}, { 0,  0,  1} }, /* 3 top/lower-left */
	{{ 0, -1,  0}, {-1,  0,  0}, { 0,  0, -1} }, /* 4 bottom/upper-left */
	{{ 1,  0,  0}, { 0, -1,  0}, { 0,  0, -1} }, /* 5 bottom/upper-right */
	{{ 0,  1,  0}, { 1,  0,  0}, { 0,  0, -1} }, /* 6 bottom/lower-right */
	{{-1,  0,  0}, { 0,  1,  0}, { 0,  0, -1} }, /* 7 bottom/lower-left*/
	};

	struct k3dh_acc xyz_adjusted = {0,};
	s16 raw[3] = {0,};
	int j;
	raw[0] = acc->x;
	raw[1] = acc->y;
	raw[2] = acc->z;
	for (j = 0; j < 3; j++) {
		xyz_adjusted.x +=
		(position_map[position][0][j] * raw[j]);
		xyz_adjusted.y +=
		(position_map[position][1][j] * raw[j]);
		xyz_adjusted.z +=
		(position_map[position][2][j] * raw[j]);
	}
	acc->x = xyz_adjusted.x;
	acc->y = xyz_adjusted.y;
	acc->z = xyz_adjusted.z;
}

/* Read X,Y and Z-axis acceleration raw data */
static int k3dh_read_accel_raw_xyz(struct k3dh_data *data,
				struct k3dh_acc *acc)
{
	int err;
	s8 reg = OUT_X_L | AC; /* read from OUT_X_L to OUT_Z_H by auto-inc */
	u8 acc_data[6];

	err = i2c_smbus_read_i2c_block_data(data->client, reg,
					    sizeof(acc_data), acc_data);
	if (err != sizeof(acc_data)) {
		pr_err("%s : failed to read 6 bytes for getting x/y/z\n",
		       __func__);
		return -EIO;
	}

	acc->x = (acc_data[1] << 8) | acc_data[0];
	acc->y = (acc_data[3] << 8) | acc_data[2];
	acc->z = (acc_data[5] << 8) | acc_data[4];

	acc->x = acc->x >> 4;
	acc->y = acc->y >> 4;

#if defined(CONFIG_MACH_U1_NA_SPR_REV05) \
	|| defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00) \
	|| defined(CONFIG_MACH_U1_NA_USCC_REV05) \
	|| defined(CONFIG_MACH_Q1_BD) \
	|| defined(CONFIG_MACH_U1_NA_USCC) \
	|| defined(CONFIG_MACH_U1_NA_SPR)
	acc->z = -acc->z >> 4;
#else
	acc->z = acc->z >> 4;
#endif

	if (data->axis_adjust)
		k3dh_xyz_position_adjust(acc, data->position);
	return 0;
}

static int k3dh_read_accel_xyz(struct k3dh_data *data,
				struct k3dh_acc *acc)
{
	int err = 0;

	mutex_lock(&data->read_lock);
	err = k3dh_read_accel_raw_xyz(data, acc);
	mutex_unlock(&data->read_lock);
	if (err < 0) {
		pr_err("%s: k3dh_read_accel_raw_xyz() failed\n", __func__);
		return err;
	}

	acc->x -= data->cal_data.x;
	acc->y -= data->cal_data.y;
	acc->z -= data->cal_data.z;

	return err;
}

static int k3dh_open_calibration(struct k3dh_data *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		err = PTR_ERR(cal_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	k3dh_dbgmsg("%s: (%u,%u,%u)\n", __func__,
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k3dh_do_calibrate(struct device *dev, bool do_calib)
{
	struct k3dh_data *acc_data = dev_get_drvdata(dev);
	struct k3dh_acc data = { 0, };
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err = 0;
	int i;
	mm_segment_t old_fs;

	if (do_calib) {
		for (i = 0; i < CAL_DATA_AMOUNT; i++) {
			mutex_lock(&acc_data->read_lock);
			err = k3dh_read_accel_raw_xyz(acc_data, &data);
			mutex_unlock(&acc_data->read_lock);
			if (err < 0) {
				pr_err("%s: k3dh_read_accel_raw_xyz() "
					"failed in the %dth loop\n",
					__func__, i);
				return err;
			}

			sum[0] += data.x;
			sum[1] += data.y;
			sum[2] += data.z;
		}

		acc_data->cal_data.x = sum[0] / CAL_DATA_AMOUNT;
		acc_data->cal_data.y = sum[1] / CAL_DATA_AMOUNT;
		if (sum[2] >= 0)
			acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT)-1024;
		else
			acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT)+1024;
	} else {
		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;
	}

	printk(KERN_INFO "%s: cal data (%d,%d,%d)\n", __func__,
	      acc_data->cal_data.x, acc_data->cal_data.y, acc_data->cal_data.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		return err;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&acc_data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k3dh_accel_enable(struct k3dh_data *data)
{
	int err = 0;

	if (atomic_read(&data->opened) == 0) {
		err = k3dh_open_calibration(data);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: k3dh_open_calibration() failed\n",
				__func__);
		data->ctrl_reg1_shadow = DEFAULT_POWER_ON_SETTING;
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						DEFAULT_POWER_ON_SETTING);
		if (err)
			pr_err("%s: i2c write ctrl_reg1 failed\n", __func__);

#if defined(CONFIG_SENSORS_K2DH)
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG4,
						CTRL_REG4_HR | CTRL_REG4_BDU);
#else
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG4,
						CTRL_REG4_HR);
#endif
		if (err)
			pr_err("%s: i2c write ctrl_reg4 failed\n", __func__);
	}

	atomic_add(1, &data->opened);

	return err;
}

static int k3dh_accel_disable(struct k3dh_data *data)
{
	int err = 0;

	atomic_sub(1, &data->opened);
	if (atomic_read(&data->opened) == 0) {
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
								PM_OFF);
		data->ctrl_reg1_shadow = PM_OFF;
	}

	return err;
}

/*  open command for K3DH device file  */
static int k3dh_open(struct inode *inode, struct file *file)
{
	k3dh_infomsg("is called.\n");
	return 0;
}

/*  release command for K3DH device file */
static int k3dh_close(struct inode *inode, struct file *file)
{
	k3dh_infomsg("is called.\n");
	return 0;
}

static s64 k3dh_get_delay(struct k3dh_data *data)
{
	int i;
	u8 odr;
	s64 delay = -1;

	odr = data->ctrl_reg1_shadow & ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (odr == odr_delay_table[i].odr) {
			delay = odr_delay_table[i].delay_ns;
			break;
		}
	}
	return delay;
}

static int k3dh_set_delay(struct k3dh_data *data, s64 delay_ns)
{
	int odr_value = ODR1;
	int res = 0;
	int i;

	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;
	odr_value = odr_delay_table[i].odr;
	delay_ns = odr_delay_table[i].delay_ns;

	k3dh_infomsg("old=%lldns, new=%lldns, odr=0x%x/0x%x\n",
		k3dh_get_delay(data), delay_ns, odr_value,
			data->ctrl_reg1_shadow);
	mutex_lock(&data->write_lock);
	if (odr_value != (data->ctrl_reg1_shadow & ODR_MASK)) {
		u8 ctrl = (data->ctrl_reg1_shadow & ~ODR_MASK);
		ctrl |= odr_value;
		data->ctrl_reg1_shadow = ctrl;
		res = i2c_smbus_write_byte_data(data->client, CTRL_REG1, ctrl);
	}
	mutex_unlock(&data->write_lock);
	return res;
}

/*  ioctl command for K3DH device file */
static long k3dh_ioctl(struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct k3dh_data *data = container_of(file->private_data,
		struct k3dh_data, k3dh_device);
	s64 delay_ns;
	int enable = 0;

	/* cmd mapping */
	switch (cmd) {
	case K3DH_IOCTL_SET_ENABLE:
		if (copy_from_user(&enable, (void __user *)arg,
					sizeof(enable)))
			return -EFAULT;
		k3dh_infomsg("opened = %d, enable = %d\n",
			atomic_read(&data->opened), enable);
		if (enable)
			err = k3dh_accel_enable(data);
		else
			err = k3dh_accel_disable(data);
		break;
	case K3DH_IOCTL_SET_DELAY:
		if (copy_from_user(&delay_ns, (void __user *)arg,
					sizeof(delay_ns)))
			return -EFAULT;
		err = k3dh_set_delay(data, delay_ns);
		break;
	case K3DH_IOCTL_GET_DELAY:
		delay_ns = k3dh_get_delay(data);
		if (put_user(delay_ns, (s64 __user *)arg))
			return -EFAULT;
		break;
	case K3DH_IOCTL_READ_ACCEL_XYZ:
		err = k3dh_read_accel_xyz(data, &data->acc_xyz);
		if (err)
			break;
		if (copy_to_user((void __user *)arg,
			&data->acc_xyz, sizeof(data->acc_xyz)))
			return -EFAULT;
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int k3dh_suspend(struct device *dev)
{
	int res = 0;
	struct k3dh_data *data = dev_get_drvdata(dev);
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
#endif
	if (atomic_read(&data->opened) > 0)
		res = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, PM_OFF);

	return res;
}

static int k3dh_resume(struct device *dev)
{
	int res = 0;
	struct k3dh_data *data = dev_get_drvdata(dev);

	if (atomic_read(&data->opened) > 0)
		res = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						data->ctrl_reg1_shadow);
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
		if (atomic_read(&data->enable))
			schedule_delayed_work(&data->work,
				msecs_to_jiffies(5));
#endif
	return res;
}

static const struct dev_pm_ops k3dh_pm_ops = {
	.suspend = k3dh_suspend,
	.resume = k3dh_resume,
};

static const struct file_operations k3dh_fops = {
	.owner = THIS_MODULE,
	.open = k3dh_open,
	.release = k3dh_close,
	.unlocked_ioctl = k3dh_ioctl,
};

#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
static ssize_t k3dh_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct k3dh_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t k3dh_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct k3dh_data *data = input_get_drvdata(input);
	unsigned long enable = 0;
	int err;

	if (strict_strtoul(buf, 10, &enable))
		return -EINVAL;
	k3dh_open_calibration(data);

	if (enable) {
		err = k3dh_accel_enable(data);
		if (err < 0)
			goto done;
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(5));
	} else {
		cancel_delayed_work_sync(&data->work);
		err = k3dh_accel_disable(data);
		if (err < 0)
			goto done;
	}
	atomic_set(&data->enable, enable);
	pr_info("%s, enable = %ld\n", __func__, enable);
done:
	return count;
}
static DEVICE_ATTR(enable,
		   S_IRUGO | S_IWUSR | S_IWGRP,
		   k3dh_enable_show, k3dh_enable_store);

static ssize_t k3dh_delay_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct k3dh_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", atomic_read(&data->delay));
}

static ssize_t k3dh_delay_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct k3dh_data *data = input_get_drvdata(input);
	unsigned long delay = 0;
	if (strict_strtoul(buf, 10, &delay))
		return -EINVAL;

	if (delay > MAX_DELAY)
		delay = MAX_DELAY;
	if (delay < MIN_DELAY)
		delay = MIN_DELAY;
	atomic_set(&data->delay, delay);
	k3dh_set_delay(data, delay * 1000000);
	pr_info("%s, delay = %ld\n", __func__, delay);
	return count;
}
static DEVICE_ATTR(poll_delay,
		   S_IRUGO | S_IWUSR | S_IWGRP,
		   k3dh_delay_show, k3dh_delay_store);

static struct attribute *k3dh_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group k3dh_attribute_group = {
	.attrs = k3dh_attributes
};
#endif

static ssize_t k3dh_fs_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct k3dh_data *data = dev_get_drvdata(dev);

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
	int err = 0;
	int on;

	mutex_lock(&data->write_lock);
	on = atomic_read(&data->opened);
	if (on == 0) {
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						DEFAULT_POWER_ON_SETTING);
	}
	mutex_unlock(&data->write_lock);

	if (err < 0) {
		pr_err("%s: i2c write ctrl_reg1 failed\n", __func__);
		return err;
	}

	err = k3dh_read_accel_xyz(data, &data->acc_xyz);
	if (err < 0) {
		pr_err("%s: k3dh_read_accel_xyz failed\n", __func__);
		return err;
	}

	if (on == 0) {
		mutex_lock(&data->write_lock);
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
								PM_OFF);
		mutex_unlock(&data->write_lock);
		if (err)
			pr_err("%s: i2c write ctrl_reg1 failed\n", __func__);
	}
#endif
	return sprintf(buf, "%d,%d,%d\n",
		data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
}

static ssize_t k3dh_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct k3dh_data *data = dev_get_drvdata(dev);

	err = k3dh_open_calibration(data);
	if (err < 0)
		pr_err("%s: k3dh_open_calibration() failed\n", __func__);

	if (!data->cal_data.x && !data->cal_data.y && !data->cal_data.z)
		err = -1;

	return sprintf(buf, "%d %d %d %d\n",
		err, data->cal_data.x, data->cal_data.y, data->cal_data.z);
}

static ssize_t k3dh_calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct k3dh_data *data = dev_get_drvdata(dev);
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1"))
		do_calib = true;
	else if (sysfs_streq(buf, "0"))
		do_calib = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (atomic_read(&data->opened) == 0) {
		/* if off, turn on the device.*/
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
			DEFAULT_POWER_ON_SETTING);
		if (err) {
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
		}
	}

	err = k3dh_do_calibrate(dev, do_calib);
	if (err < 0) {
		pr_err("%s: k3dh_do_calibrate() failed\n", __func__);
		return err;
	}

	if (atomic_read(&data->opened) == 0) {
		/* if off, turn on the device.*/
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
			PM_OFF);
		if (err) {
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
		}
	}

	return count;
}

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
static DEVICE_ATTR(acc_file, 0664, k3dh_fs_read, NULL);
#else
static ssize_t
k3dh_accel_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct k3dh_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->position);
}

static ssize_t
k3dh_accel_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct k3dh_data *data = dev_get_drvdata(dev);
	int err = 0;

	err = kstrtoint(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	return count;
}

static ssize_t k3dh_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t k3dh_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(name, 0664, k3dh_accel_name_show, NULL);
static DEVICE_ATTR(vendor, 0664, k3dh_accel_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0664, k3dh_fs_read, NULL);
static DEVICE_ATTR(position, 0664,
	k3dh_accel_position_show, k3dh_accel_position_store);
#endif
static DEVICE_ATTR(calibration, 0664,
		   k3dh_calibration_show, k3dh_calibration_store);

void k3dh_shutdown(struct i2c_client *client)
{
	int res = 0;
	struct k3dh_data *data = i2c_get_clientdata(client);

	k3dh_infomsg("is called.\n");

#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
#endif
	res = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
	if (res < 0)
		pr_err("%s: pm_off failed %d\n", __func__, res);
}

#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
static void k3dh_work_func(struct work_struct *work)
{
	k3dh_read_accel_xyz(g_k3dh, &g_k3dh->acc_xyz);
	pr_debug("%s: x: %d, y: %d, z: %d\n", __func__,
		g_k3dh->acc_xyz.x, g_k3dh->acc_xyz.y, g_k3dh->acc_xyz.z);
	input_report_abs(g_k3dh->input, ABS_X, g_k3dh->acc_xyz.x);
	input_report_abs(g_k3dh->input, ABS_Y, g_k3dh->acc_xyz.y);
	input_report_abs(g_k3dh->input, ABS_Z, g_k3dh->acc_xyz.z);
	input_sync(g_k3dh->input);
	schedule_delayed_work(&g_k3dh->work, msecs_to_jiffies(
		atomic_read(&g_k3dh->delay)));
}

/* ----------------- *
   Input device interface
 * ------------------ */
static int k3dh_input_init(struct k3dh_data *data)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	dev->name = "accelerometer";
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(dev, data);

	err = input_register_device(dev);
	if (err < 0)
		goto done;
	data->input = dev;
done:
	return 0;
}
#endif
static int k3dh_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct k3dh_data *data;
#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
	struct device *dev_t, *dev_cal;
#endif
	struct accel_platform_data *pdata;
	int err;

	k3dh_infomsg("is started.\n");

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		err = -ENODEV;
		goto exit;
	}

	data = kzalloc(sizeof(struct k3dh_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	/* Checking device */
	err = i2c_smbus_write_byte_data(client, CTRL_REG1,
		PM_OFF);
	if (err) {
		pr_err("%s: there is no such device, err = %d\n",
							__func__, err);
		goto err_read_reg;
	}

	data->client = client;
	g_k3dh = data;
	i2c_set_clientdata(client, data);

	init_completion(&data->data_ready);
	mutex_init(&data->read_lock);
	mutex_init(&data->write_lock);
	atomic_set(&data->opened, 0);

	/* sensor HAL expects to find /dev/accelerometer */
	data->k3dh_device.minor = MISC_DYNAMIC_MINOR;
	data->k3dh_device.name = ACC_DEV_NAME;
	data->k3dh_device.fops = &k3dh_fops;

	err = misc_register(&data->k3dh_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		/*Set by default position 3, it doesn't adjust raw value*/
		data->position = 3;
		data->axis_adjust = false;
		pr_err("using defualt position = %d\n", data->position);
	} else {
		if (pdata->accel_get_position)
			data->position = pdata->accel_get_position();
		data->axis_adjust = pdata->axis_adjust;
		pr_info("successful, position = %d\n", data->position);
	}
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
	atomic_set(&data->enable, 0);
	atomic_set(&data->delay, 200);
	k3dh_input_init(data);

	/* Setup sysfs */
	err =
	    sysfs_create_group(&data->input->dev.kobj,
			       &k3dh_attribute_group);
	if (err < 0)
		goto err_sysfs_create_group;

	/* Setup driver interface */
	INIT_DELAYED_WORK(&data->work, k3dh_work_func);
#endif
#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
	/* creating class/device for test */
	data->acc_class = class_create(THIS_MODULE, "accelerometer");
	if (IS_ERR(data->acc_class)) {
		pr_err("%s: class create failed(accelerometer)\n", __func__);
		err = PTR_ERR(data->acc_class);
		goto err_class_create;
	}

	dev_t = device_create(data->acc_class, NULL,
				MKDEV(ACC_DEV_MAJOR, 0), "%s", "accelerometer");
	if (IS_ERR(dev_t)) {
		pr_err("%s: class create failed(accelerometer)\n", __func__);
		err = PTR_ERR(dev_t);
		goto err_acc_device_create;
	}

	err = device_create_file(dev_t, &dev_attr_acc_file);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_acc_file.attr.name);
		goto err_acc_device_create_file;
	}
	dev_set_drvdata(dev_t, data);

	/* creating device for calibration */
	dev_cal = device_create(sec_class, NULL, 0, NULL, "gsensorcal");
	if (IS_ERR(dev_cal)) {
		pr_err("%s: class create failed(gsensorcal)\n", __func__);
		err = PTR_ERR(dev_cal);
		goto err_cal_device_create;
	}

	err = device_create_file(dev_cal, &dev_attr_calibration);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_calibration.attr.name);
		goto err_cal_device_create_file;
	}
	dev_set_drvdata(dev_cal, data);
#else
	/* creating device for test & calibration */
	data->dev = sensors_classdev_register("accelerometer_sensor");
	if (IS_ERR(data->dev)) {
		pr_err("%s: class create failed(accelerometer_sensor)\n",
			__func__);
		err = PTR_ERR(data->dev);
		goto err_acc_device_create;
	}

	err = device_create_file(data->dev, &dev_attr_position);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
			__func__, dev_attr_position.attr.name);
		goto err_position_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_raw_data);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_raw_data.attr.name);
		goto err_acc_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_calibration);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_calibration.attr.name);
		goto err_cal_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_vendor);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_vendor.attr.name);
		goto err_vendor_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_name);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_name.attr.name);
		goto err_name_device_create_file;
	}

	dev_set_drvdata(data->dev, data);
#endif

	k3dh_infomsg("is successful.\n");

	return 0;

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
err_cal_device_create_file:
	device_destroy(sec_class, 0);
err_cal_device_create:
	device_remove_file(dev_t, &dev_attr_acc_file);
err_acc_device_create_file:
	device_destroy(data->acc_class, MKDEV(ACC_DEV_MAJOR, 0));
err_acc_device_create:
	class_destroy(data->acc_class);
err_class_create:
#else
err_name_device_create_file:
	device_remove_file(data->dev, &dev_attr_vendor);
err_vendor_device_create_file:
	device_remove_file(data->dev, &dev_attr_calibration);
err_cal_device_create_file:
	device_remove_file(data->dev, &dev_attr_raw_data);
err_acc_device_create_file:
	device_remove_file(data->dev, &dev_attr_position);
err_position_device_create_file:
	sensors_classdev_unregister(data->dev);

err_acc_device_create:
#endif
#ifdef CONFIG_SENSOR_K3DH_INPUTDEV
	input_free_device(data->input);
err_sysfs_create_group:
#endif
misc_deregister(&data->k3dh_device);
err_misc_register:
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
err_read_reg:
	kfree(data);
exit:
	return err;
}

static int k3dh_remove(struct i2c_client *client)
{
	struct k3dh_data *data = i2c_get_clientdata(client);
	int err = 0;

	if (atomic_read(&data->opened) > 0) {
		err = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
		if (err < 0)
			pr_err("%s: pm_off failed %d\n", __func__, err);
	}

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_TRATS)
	device_destroy(sec_class, 0);
	device_destroy(data->acc_class, MKDEV(ACC_DEV_MAJOR, 0));
	class_destroy(data->acc_class);
#else
	device_remove_file(data->dev, &dev_attr_name);
	device_remove_file(data->dev, &dev_attr_vendor);
	device_remove_file(data->dev, &dev_attr_calibration);
	device_remove_file(data->dev, &dev_attr_raw_data);
	device_remove_file(data->dev, &dev_attr_position);
	sensors_classdev_unregister(data->dev);
#endif
	misc_deregister(&data->k3dh_device);
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
	kfree(data);

	return 0;
}

static const struct i2c_device_id k3dh_id[] = {
	{ "k3dh", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, k3dh_id);

static struct i2c_driver k3dh_driver = {
	.probe = k3dh_probe,
	.shutdown = k3dh_shutdown,
	.remove = __devexit_p(k3dh_remove),
	.id_table = k3dh_id,
	.driver = {
		.pm = &k3dh_pm_ops,
		.owner = THIS_MODULE,
		.name = "k3dh",
	},
};

static int __init k3dh_init(void)
{
	return i2c_add_driver(&k3dh_driver);
}

static void __exit k3dh_exit(void)
{
	i2c_del_driver(&k3dh_driver);
}

module_init(k3dh_init);
module_exit(k3dh_exit);

MODULE_DESCRIPTION("k3dh accelerometer driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
