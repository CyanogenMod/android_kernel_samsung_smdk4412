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
};

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
		|| defined(CONFIG_MACH_Q1_BD)
	acc->z = -acc->z >> 4;
	#else
	acc->z = acc->z >> 4;
	#endif

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
		acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT) - 1024;
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

		err = i2c_smbus_write_byte_data(data->client, CTRL_REG4,
						CTRL_REG4_HR);
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
#endif
static DEVICE_ATTR(calibration, 0664,
		   k3dh_calibration_show, k3dh_calibration_store);

void k3dh_shutdown(struct i2c_client *client)
{
	int res = 0;
	struct k3dh_data *data = i2c_get_clientdata(client);

	k3dh_infomsg("is called.\n");

	res = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
	if (res < 0)
		pr_err("%s: pm_off failed %d\n", __func__, res);
}

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
	misc_deregister(&data->k3dh_device);
#else
err_name_device_create_file:
	device_remove_file(data->dev, &dev_attr_vendor);
err_vendor_device_create_file:
	device_remove_file(data->dev, &dev_attr_calibration);
err_cal_device_create_file:
	device_remove_file(data->dev, &dev_attr_raw_data);
err_acc_device_create_file:
	sensors_classdev_unregister(data->dev);
err_acc_device_create:
	misc_deregister(&data->k3dh_device);
#endif
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
