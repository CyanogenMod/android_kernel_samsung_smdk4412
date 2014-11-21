/*
 *  STMicroelectronics lsm330dlc_accel acceleration sensor driver
 *
 *  Copyright (C) 2011 Samsung Electronics Co.Ltd
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
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/sensor/k330_accel.h>
#include <linux/sensor/sensors_core.h>
#include <linux/printk.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <mach/gpio-midas.h>

/* For debugging */
#if 1
#define accel_dbgmsg(str, args...)\
	pr_info("%s: " str, __func__, ##args)
#else
#define accel_dbgmsg(str, args...)\
	pr_debug("%s: " str, __func__, ##args)
#endif

#undef K330_ACCEL_LOGGING
#define K330_ACCEL_DEBUG_REGISTERS
#undef DEBUG_REACTIVE_ALERT

#define VENDOR		"STM"
#define CHIP_ID		"K330"

/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 */
#define DEFAULT_POWER_ON_SETTING (ODR100 | K330ACCEL_ALL_AXES)
#ifdef CONFIG_SENSORS_SMART_ALERT
#define DEFAULT_MASKA_1_SETTING		0xF0 /* SM1 MASK : X, Y */
#define DEFAULT_MASKA_2_SETTING		0x0C /* SM2 MASK : Z */
#define DEFAULT_SM1_S0_SETTING		0x05 /* OPcode : GNTH1 */
#define DEFAULT_SM1_S1_SETTING		0x11 /* CMD : CONT */
#define DEFAULT_SM2_S0_SETTING		0x05 /* OPcode : GNTH1 */
#define DEFAULT_SM2_S1_SETTING		0x11 /* CMD : CONT */
#define DEFAULT_THRESHOLD		0x7F /* 2032mg (16*0x7F) */
#define DYNAMIC_THRESHOLD		300
#define DYNAMIC_THRESHOLD2		700
#define SENSITIVITY_2G			61
#define K330ACCEL_SENSITIVITY		SENSITIVITY_2G
#ifdef CONFIG_K330_SMART_ALERT_INT2	/* In case of using INT2 */
#define INT1_ENABLE			0x10/* FALLING / INT2 */
#define SM1_ENABLE			0x09
#define SM2_ENABLE			0x09
#else
#define INT1_ENABLE			0x08/* FALLING / INT1 */
#define SM1_ENABLE			0x01
#define SM2_ENABLE			0x01
#endif
#endif

enum {
	OFF = 0,
	ON = 1
};
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MG_TO_LSB(x)	(x * ACCEL_1G / 1000)

#define ACC_DEV_NAME		"accelerometer"
#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#define CAL_DATA_AMOUNT	20

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	u64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{ ODR1600,     625000LL }, /* 1600Hz */
	{  ODR800,    1250000LL }, /* 800Hz */
	{  ODR400,    2500000LL }, /* 400Hz */
	{  ODR100,   10000000LL }, /* 100Hz */
	{   ODR50,   20000000LL }, /* 50Hz */
	{   ODR25,   40000000LL }, /* 25Hz */
	{   ODR12_5,  80000000LL }, /* 12.5Hz */
	{   ODR3_125, 320000000LL }, /* 3.125Hz */
};

struct k330_accel_data {
	struct i2c_client *client;
	struct miscdevice k330_accel_device;
	struct mutex read_lock;
	struct mutex write_lock;
	struct k330_acc cal_data;
	struct k330_acc acc_offset;
	struct device *dev;
	u8 ctrl_reg4_shadow;
	u8 resume_state[K330_RESUME_ENTRIES];
	atomic_t opened; /* opened implies enabled */
#ifdef CONFIG_SENSORS_SMART_ALERT
	int movement_recog_flag;
	unsigned char interrupt_state;
	struct wake_lock reactive_wake_lock;
#endif
	ktime_t poll_delay;
	u8 position;
	bool sum_when_gyro_is_on;
	bool *gyro_en;
	struct k330_acc acc_xyz;
	struct k330_acc acc_prev_xyz;

	axes_func_s16 convert_axes;
	axes_func_s16 (*select_func) (u8);
};

static int k330_acc_i2c_write(struct k330_accel_data *data,
	u8 * buf, int len);


 /* Read X,Y and Z-axis acceleration raw data */
static int k330_accel_read_raw_xyz(struct k330_accel_data *data,
				struct k330_acc *acc)
{
	int err;
	/* read from OUT_X_L to OUT_Z_H by auto-inc */
	s8 reg = OUT_X_L | K330ACCEL_AC;
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

	return 0;
}

static int k330_accel_read_xyz(struct k330_accel_data *data,
				struct k330_acc *acc)
{
	int err;
	s16 x_temp, y_temp, z_temp;

	mutex_lock(&data->read_lock);
	err = k330_accel_read_raw_xyz(data, acc);
	mutex_unlock(&data->read_lock);
	if (err < 0) {
		pr_err("%s: k330_accel_read_xyz() failed\n", __func__);
		return err;
	}

	x_temp = acc->x - data->cal_data.x;
	y_temp = acc->y - data->cal_data.y;
	z_temp = acc->z - data->cal_data.z;

	if (!(x_temp >> 15 ==  acc->x >> 15) &&\
		!(data->cal_data.x >> 15 == acc->x >> 15)) {
		pr_debug("%s, accel x is overflowed!\n", __func__);
		x_temp =
			(x_temp > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}
	if (!(y_temp >> 15 ==  acc->y >> 15) &&\
		!(data->cal_data.y >> 15 == acc->y >> 15)) {
		pr_debug("%s, accel y is overflowed!\n", __func__);
		y_temp =
			(y_temp > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}
	if (!(z_temp >> 15 ==  acc->z >> 15) &&\
		!(data->cal_data.z >> 15 == acc->z >> 15)) {
		pr_debug("%s, accel z is overflowed!\n", __func__);
		z_temp =
			(z_temp > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}

	if (data->gyro_en) {
		if (*data->gyro_en == data->sum_when_gyro_is_on) {
			x_temp += data->acc_offset.x;
			y_temp += data->acc_offset.y;
			z_temp += data->acc_offset.z;
		}
	}

	acc->x = x_temp;
	acc->y = y_temp;
	acc->z = z_temp;

	return err;
}

static int k330_accel_open_calibration(struct k330_accel_data *data)
{
	struct file *cal_filp;
	int err;
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

	accel_dbgmsg("(%d,%d,%d)\n",
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k330_accel_do_calibrate(struct device *dev, bool do_calib)
{
	struct k330_accel_data *acc_data = dev_get_drvdata(dev);
	struct k330_acc data = { 0, };
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err;
	int i;
	mm_segment_t old_fs;
	s16 *cal_data_temp;

	if (do_calib) {
		for (i = 0; i < CAL_DATA_AMOUNT; i++) {
			mutex_lock(&acc_data->read_lock);
			err = k330_accel_read_raw_xyz(acc_data, &data);
			mutex_unlock(&acc_data->read_lock);
			if (err < 0) {
				pr_err("%s: k330_accel_read_raw_xyz() "
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
		acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT);

#ifdef CONFIG_MACH_GD2
		cal_data_temp = &acc_data->cal_data.x;
#else
		cal_data_temp = &acc_data->cal_data.z;
#endif
		if (*cal_data_temp > 0) {
			*cal_data_temp -= ACCEL_1G;
			accel_dbgmsg("ref. axis cal data = %d\n", *cal_data_temp);
		} else if (*cal_data_temp < 0) {
			*cal_data_temp += ACCEL_1G;
			accel_dbgmsg("ref. axis cal data = %d\n", *cal_data_temp);
		}
	} else {
		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;
	}

	pr_info("%s: cal data (%d,%d,%d)\n", __func__,
			acc_data->cal_data.x, acc_data->cal_data.y
			, acc_data->cal_data.z);

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

static int k330_accel_enable(struct k330_accel_data *data)
{
	int err = 0;

	mutex_lock(&data->write_lock);
	if (atomic_read(&data->opened) == 0) {
		err = k330_accel_open_calibration(data);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: k330_accel_open_calibration() failed\n",
				__func__);
		data->ctrl_reg4_shadow = DEFAULT_POWER_ON_SETTING;
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, DEFAULT_POWER_ON_SETTING);
		if (err)
			pr_err("%s: i2c write ctrl_reg4 failed\n", __func__);
	}

	atomic_set(&data->opened, 1);
	mutex_unlock(&data->write_lock);

	return err;
}

static int k330_accel_disable(struct k330_accel_data *data)
{
	int err = 0;

	mutex_lock(&data->write_lock);
#ifdef CONFIG_SENSORS_SMART_ALERT
	if (data->movement_recog_flag == ON) {
		accel_dbgmsg("LOW_PWR_MODE.\n");
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, LOW_PWR_MODE);
		if (atomic_read(&data->opened) == 1)
			data->ctrl_reg4_shadow = PM_OFF;
	} else if (atomic_read(&data->opened) == 1) {
#else
	if (atomic_read(&data->opened) == 1) {
#endif
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, PM_OFF);
		data->ctrl_reg4_shadow = PM_OFF;
	}
	atomic_set(&data->opened, 0);
	mutex_unlock(&data->write_lock);

	return err;
}

/*  open command for k330_accel device file  */
static int k330_accel_open(struct inode *inode, struct file *file)
{
	accel_dbgmsg("is called.\n");
	return 0;
}

/*  release command for k330_accel device file */
static int k330_accel_close(struct inode *inode, struct file *file)
{
	accel_dbgmsg("is called.\n");
	return 0;
}

static int k330_accel_set_delay(struct k330_accel_data *data
	, u64 delay_ns)
{
	int odr_value = ODR3_125;
	int err = 0, i;

	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	mutex_lock(&data->write_lock);

	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;
	odr_value = odr_delay_table[i].odr;

	accel_dbgmsg("old=%lldns,new=%lldns,odr=0x%x,opened=%d\n",
		     ktime_to_ns(data->poll_delay), delay_ns, odr_value,
		     atomic_read(&data->opened));
	data->poll_delay = ns_to_ktime(delay_ns);
	if (odr_value != (data->ctrl_reg4_shadow & K330ACCEL_ODR_MASK)) {
		u8 ctrl = (data->ctrl_reg4_shadow & ~K330ACCEL_ODR_MASK);
		ctrl |= odr_value;
		data->ctrl_reg4_shadow = ctrl;
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, ctrl);
		if (err < 0)
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
				__func__, err);
	}

	mutex_unlock(&data->write_lock);
	return err;
}

/*  ioctl command for k330_accel device file */
static long k330_accel_ioctl(struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct k330_accel_data *data =
		container_of(file->private_data, struct k330_accel_data,
			k330_accel_device);
	u64 delay_ns;
	int enable = 0;
	static int count;
	u8 reg_data;

	/* cmd mapping */
	switch (cmd) {
	case K330_ACCEL_IOCTL_SET_ENABLE:
		if (copy_from_user(&enable, (void __user *)arg,
					sizeof(enable)))
			return -EFAULT;

		accel_dbgmsg("opened = %d, enable = %d\n",
			atomic_read(&data->opened), enable);
		if (enable)
			err = k330_accel_enable(data);
		else
			err = k330_accel_disable(data);
		break;
	case K330_ACCEL_IOCTL_SET_DELAY:
		if (copy_from_user(&delay_ns, (void __user *)arg,
					sizeof(delay_ns)))
			return -EFAULT;

#if 0 /* fix odr, because accel data is not updated rarely on low odr. */
		err = k330_accel_set_delay(data, delay_ns);
#else
		accel_dbgmsg("old=%lldns, new=%lldns, opened=%d\n",
		     ktime_to_ns(data->poll_delay), delay_ns,
		     atomic_read(&data->opened));
		data->poll_delay = ns_to_ktime(delay_ns);
#endif

		break;
	case K330_ACCEL_IOCTL_GET_DELAY:
		if (put_user(ktime_to_ns(data->poll_delay), (u64 __user *)arg))
			return -EFAULT;
		break;
	case K330_ACCEL_IOCTL_READ_XYZ:
		err = k330_accel_read_xyz(data, &data->acc_xyz);
		if (err)
			break;
		if (data->convert_axes)
			data->convert_axes(&data->acc_xyz.x,
				&data->acc_xyz.y, &data->acc_xyz.z);
		/* Temp for debugging autorotation problem. */
		if (count++ == 100) {
			accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
			data->acc_xyz.x, data->acc_xyz.y,
			data->acc_xyz.z);
			count = 0;

			/* Return zero to match */
			if (!memcmp(&data->acc_prev_xyz, &data->acc_xyz, sizeof(struct k330_acc))) {
				/* read CTRL_REG4 */
				reg_data = (u8)i2c_smbus_read_byte_data(data->client
					, K330ACCEL_CTRL_REG4);
				if (reg_data < 0)
					pr_err("%s, read CTRL_REG4 failed\n", __func__);

				accel_dbgmsg("CTRL_REG4=0x%x\n", reg_data);
				accel_dbgmsg("raw pre x = %d, y = %d, z = %d\n",
					data->acc_prev_xyz.x, data->acc_prev_xyz.y, data->acc_prev_xyz.z);
				accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
					data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
			}
		}

		/* data copy */
		memcpy(&data->acc_prev_xyz, &data->acc_xyz, sizeof(struct k330_acc));

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

static int k330_accel_suspend(struct device *dev)
{
	int res = 0;
	struct k330_accel_data *data = dev_get_drvdata(dev);

#ifdef CONFIG_SENSORS_SMART_ALERT
	if (data->movement_recog_flag == ON) {
		accel_dbgmsg("LOW_PWR_MODE.\n");
		res = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, LOW_PWR_MODE);
	} else if (atomic_read(&data->opened) > 0) {
#else
	if (atomic_read(&data->opened) > 0) {
#endif
		accel_dbgmsg("PM_OFF.\n");
		res = i2c_smbus_write_byte_data(data->client,
						K330ACCEL_CTRL_REG4, PM_OFF);
	}

	return res;
}

static int k330_accel_resume(struct device *dev)
{
	int res = 0;
	struct k330_accel_data *data = dev_get_drvdata(dev);

	if (atomic_read(&data->opened) > 0) {
		accel_dbgmsg("ctrl_reg4_shadow = 0x%x\n"
			, data->ctrl_reg4_shadow);
		res = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, data->ctrl_reg4_shadow);
	}

	return res;
}

static const struct dev_pm_ops k330_accel_pm_ops = {
	.suspend = k330_accel_suspend,
	.resume = k330_accel_resume,
};

static const struct file_operations k330_accel_fops = {
	.owner = THIS_MODULE,
	.open = k330_accel_open,
	.release = k330_accel_close,
	.unlocked_ioctl = k330_accel_ioctl,
};

static ssize_t k330_accel_fs_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n",
		data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
}

static ssize_t k330_accel_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct k330_accel_data *data = dev_get_drvdata(dev);

	err = k330_accel_open_calibration(data);
	if (err < 0)
		pr_err("%s: k330_accel_open_calibration() failed\n"\
		, __func__);

	if (!data->cal_data.x && !data->cal_data.y && !data->cal_data.z)
		err = -1;

	return sprintf(buf, "%d %d %d %d\n",
		err, data->cal_data.x, data->cal_data.y, data->cal_data.z);
}

static ssize_t k330_accel_calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);
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
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, DEFAULT_POWER_ON_SETTING);
		if (err) {
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
				__func__, err);
		}
	}

	err = k330_accel_do_calibrate(dev, do_calib);
	if (err < 0) {
		pr_err("%s: k330_accel_do_calibrate() failed\n", __func__);
		return err;
	}

	if (atomic_read(&data->opened) == 0) {
		/* if off, turn on the device.*/
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, PM_OFF);
		if (err) {
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
				__func__, err);
		}
	}

	return count;
}

#ifdef CONFIG_SENSORS_SMART_ALERT
static int k330_acc_i2c_write(struct k330_accel_data *data,
	u8 * buf, int len)
{
	int err;
	int tries = 0, retry = 2;

	struct i2c_msg msgs[] = {
		{
			.addr = data->client->addr,
			.flags = data->client->flags & I2C_M_TEN,
			.len = len + 1,
			.buf = buf,
		},
	};

	do {
		err = i2c_transfer(data->client->adapter, msgs, 1);
		if (err != 1) {
			usleep_range(1000, 1000);
		}
	} while ((err != 1) && (++tries < retry));

	if (err != 1) {
		dev_err(&data->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static ssize_t k330_accel_reactive_alert_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int onoff = OFF, err = 0, ctrl_reg = 0;
	struct k330_accel_data *data = dev_get_drvdata(dev);
	bool factory_test = false;
	struct k330_acc raw_data;
	u8 thresh1 = 0, thresh2 = 0, send_buf[4] = {0,};

	if (sysfs_streq(buf, "1"))
		onoff = ON;
	else if (sysfs_streq(buf, "0"))
		onoff = OFF;
	else if (sysfs_streq(buf, "2")) {
		onoff = ON;
		factory_test = true;
		accel_dbgmsg("factory_test = %d\n", factory_test);
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&data->write_lock);

	if (onoff == ON && data->movement_recog_flag == OFF) {
		accel_dbgmsg("reactive alert is on.\n");
		data->interrupt_state = 0; /* Init interrupt state.*/

		if (atomic_read(&data->opened) == 0) {
			err = i2c_smbus_write_byte_data(data->client,
				K330ACCEL_CTRL_REG4, FASTEST_MODE);
			if (err) {
				ctrl_reg = K330ACCEL_CTRL_REG4;
				goto err_i2c_write;
			}
			/* trun on time, T = 25/odr ms */
			usleep_range(17000, 20000);
		}
		enable_irq(data->client->irq);
		if (device_may_wakeup(&data->client->dev))
			enable_irq_wake(data->client->irq);

		/* Get x, y, z data to set threshold1, threshold2. */
		mutex_lock(&data->read_lock);
		err = k330_accel_read_raw_xyz(data, &raw_data);
		mutex_unlock(&data->read_lock);

		accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
			raw_data.x, raw_data.y, raw_data.z);
		if (err < 0) {
			pr_err("%s: k330_accel_read_xyz failed\n",
				__func__);
			goto exit;
		}
		if (atomic_read(&data->opened) == 0) {
			/* Change to 50Hz*/
			err = i2c_smbus_write_byte_data(data->client,
				K330ACCEL_CTRL_REG4, LOW_PWR_MODE);
			if (err) {
				ctrl_reg = K330ACCEL_CTRL_REG4;
				goto err_i2c_write;
			}
		}
		/* Change raw data to threshold value & settng threshold */
		thresh1 = (MAX(ABS(raw_data.x), ABS(raw_data.y)) * K330ACCEL_SENSITIVITY / 1000
				+ DYNAMIC_THRESHOLD) / 16;
		if (factory_test == true) {
			thresh2 = 0; /* for z axis */
		} else
			thresh2 = (ABS(raw_data.z) * K330ACCEL_SENSITIVITY / 1000
				+ DYNAMIC_THRESHOLD2) / 16;
		accel_dbgmsg("threshold1 = 0x%x, threshold2 = 0x%x\n",
			thresh1, thresh2);
		err = i2c_smbus_write_byte_data(data->client, THRS1_1,
			thresh1);
		if (err) {
			ctrl_reg = THRS1_1;
			goto err_i2c_write;
		}
		err = i2c_smbus_write_byte_data(data->client, THRS1_2,
			thresh2);
		if (err) {
			ctrl_reg = THRS1_2;
			goto err_i2c_write;
		}

		send_buf[0] = (K330ACCEL_CTRL_REG1 | K330ACCEL_AC);
		/* SM1_EN */
		send_buf[1] = SM1_ENABLE;
		/* SM2_EN */
		send_buf[2] = SM2_ENABLE;
		/* IEA, INT1_EN */
		send_buf[3] = INT1_ENABLE;
		err = k330_acc_i2c_write(data, send_buf, 3);
		if (err) {
			ctrl_reg = K330ACCEL_CTRL_REG1;
			goto err_i2c_write;
		}
		data->movement_recog_flag = ON;
	} else if (onoff == OFF && data->movement_recog_flag == ON) {
		accel_dbgmsg("reactive alert is off, int state (%d)\n",
			data->interrupt_state);
		if (device_may_wakeup(&data->client->dev))
			disable_irq_wake(data->client->irq);
		disable_irq_nosync(data->client->irq);
		data->movement_recog_flag = OFF;

		if (!data->interrupt_state) {
			/* Clear Interrupt */
			err = (u8)i2c_smbus_read_byte_data(data->client, INT1_SRC);
			if (err < 0)
				pr_err("%s, read int1_src failed(%d)\n", __func__, err);

			err = (u8)i2c_smbus_read_byte_data(data->client, INT2_SRC);
			if (err < 0)
				pr_err("%s, read int2_src failed(%d)\n", __func__, err);

			send_buf[0] = (K330ACCEL_CTRL_REG1 | K330ACCEL_AC);
			/* SM1_EN : disable */
			send_buf[1] = PM_OFF;
			/* SM2_EN : disable */
			send_buf[2] = PM_OFF;
			/* IEA : falling, INT1_EN/INT2_EN : disable*/
			send_buf[3] = PM_OFF;
			err = k330_acc_i2c_write(data, send_buf, 3);
			if (err) {
				pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
					__func__, err);
			}

		} else {
			data->interrupt_state = 0; /* Init interrupt state.*/
		}

		/* return the power state */
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, PM_OFF);
		if (err) {
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
			__func__, err);
		}
		err = i2c_smbus_write_byte_data(data->client,
			K330ACCEL_CTRL_REG4, data->ctrl_reg4_shadow);
		if (err) {
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
				__func__, err);
		}
	}

	mutex_unlock(&data->write_lock);

	return count;

err_i2c_write:
	pr_err("%s: i2c write ctrl_reg = 0x%d failed(err=%d)\n",
				__func__, ctrl_reg, err);
exit:
	mutex_unlock(&data->write_lock);
	return ((err < 0) ? err : -err);
}

static ssize_t k330_accel_reactive_alert_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->interrupt_state);
}
#endif

#ifdef K330_ACCEL_DEBUG_REGISTERS
static ssize_t k330_accel_reg_read(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);
	u8 reg1, reg2, reg3, odr_data, fs_data, src1, src2;
	u8 mask1, mask2, thresh1, thresh2, sm1_s0, sm1_s1, sm2_s0, sm2_s1;

	reg1 = (u8)i2c_smbus_read_byte_data(data->client,
		K330ACCEL_CTRL_REG1);
	if (reg1 < 0)
		pr_err("%s, read CTRL_REG1 failed\n", __func__);
	reg2 = (u8)i2c_smbus_read_byte_data(data->client,
		K330ACCEL_CTRL_REG2);
	if (reg2 < 0)
		pr_err("%s, read CTRL_REG2 failed\n", __func__);
	reg3 = (u8)i2c_smbus_read_byte_data(data->client,
		K330ACCEL_CTRL_REG3);
	if (reg3 < 0)
		pr_err("%s, read CTRL_REG3 failed\n", __func__);

	/* read output data rate & operation mode */
	odr_data = (u8)i2c_smbus_read_byte_data(data->client,
		K330ACCEL_CTRL_REG4);
	if (odr_data < 0)
		pr_err("%s, read CTRL_REG4 failed\n", __func__);

	/* read band width and full scale setting */
	fs_data = (u8)i2c_smbus_read_byte_data(data->client,
		K330ACCEL_CTRL_REG5);
	if (fs_data < 0)
		pr_err("%s, read CTRL_REG5 failed\n", __func__);

	src1 = (u8)i2c_smbus_read_byte_data(data->client,
		INT1_SRC);
	if (src1 < 0)
		pr_err("%s, read INT1_SRC failed\n", __func__);
	src2 = (u8)i2c_smbus_read_byte_data(data->client,
		INT2_SRC);
	if (src2 < 0)
		pr_err("%s, read INT2_SRC failed\n", __func__);
	mask1 = (u8)i2c_smbus_read_byte_data(data->client,
		MASKA_1);
	if (mask1 < 0)
		pr_err("%s, read MASKA_1 failed\n", __func__);
	mask2 = (u8)i2c_smbus_read_byte_data(data->client,
		MASKA_2);
	if (mask2 < 0)
		pr_err("%s, read MASKA_2 failed\n", __func__);
	thresh1 = (u8)i2c_smbus_read_byte_data(data->client,
		THRS1_1);
	if (thresh1 < 0)
		pr_err("%s, read THRS1_1 failed\n", __func__);
	thresh2 = (u8)i2c_smbus_read_byte_data(data->client,
		THRS1_2);
	if (thresh2 < 0)
		pr_err("%s, read THRS1_2 failed\n", __func__);
	sm1_s0 = (u8)i2c_smbus_read_byte_data(data->client,
		SM1_S0);
	if (sm1_s0 < 0)
		pr_err("%s, read SM1_S0 failed\n", __func__);
	sm1_s1 = (u8)i2c_smbus_read_byte_data(data->client,
		SM1_S1);
	if (sm1_s1 < 0)
		pr_err("%s, read SM1_S1 failed\n", __func__);
	sm2_s0 = (u8)i2c_smbus_read_byte_data(data->client,
		SM2_S0);
	if (sm2_s0 < 0)
		pr_err("%s, read SM2_S0 failed\n", __func__);
	sm2_s1 = (u8)i2c_smbus_read_byte_data(data->client,
		SM2_S1);
	if (sm2_s1 < 0)
		pr_err("%s, read SM2_S1 failed\n", __func__);

	return sprintf(buf, "reg1:0x%x,reg2:0x%x,reg3:0x%x,odr:0x%x,fs:0x%x\n"\
		"src1:0x%x,src2:0x%x,mask1:0x%x,mask2:0x%x,thresh1:0x%x,thresh2:0x%x\n"\
		"sm1_s0:0x%x,sm1_s1:0x%x,sm2_s0:0x%x,sm2_s1:0x%x, irq:%d\n",
		reg1, reg2, reg3, odr_data, fs_data,
		src1, src2, mask1, mask2, thresh1, thresh2,
		sm1_s0, sm1_s1, sm2_s0, sm2_s1, gpio_get_value(GPIO_ACC_INT));
}

static ssize_t
k330_accel_reg_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);
	int err;
	u16 reg_addr;

	err = kstrtou16(buf, 16, &reg_addr);
	if (err < 0)
		pr_err("%s, kstrtou16 failed.", __func__);

	pr_info("%s,0x%x 0x%x, 0x%x\n", __func__
		, reg_addr, (u8)((reg_addr >> 8) & 0xff), (u8)(reg_addr & 0xff));

	err = i2c_smbus_write_byte_data(data->client,
					(u8)((reg_addr >> 8) & 0xff), (u8)(reg_addr & 0xff));
	if (err < 0)
		pr_err("%s: reg write failed %d\n", __func__, err);

	return count;
}
#endif

static ssize_t
k330_accel_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->position);
}

static ssize_t
k330_accel_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct k330_accel_data *data = dev_get_drvdata(dev);
	int err;

	err = kstrtou8(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	if (data->select_func)
		data->convert_axes = data->select_func(data->position);

	return count;
}

static ssize_t k330_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t k330_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(name, S_IRUGO,
	k330_accel_name_show, NULL);

static DEVICE_ATTR(vendor, S_IRUGO,
	k330_accel_vendor_show, NULL);

static DEVICE_ATTR(position,   S_IRUGO | S_IRWXU | S_IRWXG,
	k330_accel_position_show, k330_accel_position_store);

static DEVICE_ATTR(calibration,  S_IRUGO | S_IRWXU | S_IRWXG,
		   k330_accel_calibration_show
		   , k330_accel_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, k330_accel_fs_read, NULL);

#ifdef CONFIG_SENSORS_SMART_ALERT
static DEVICE_ATTR(reactive_alert,  S_IRUGO | S_IRWXU | S_IRWXG,
	k330_accel_reactive_alert_show,
	k330_accel_reactive_alert_store);
#endif

#ifdef K330_ACCEL_DEBUG_REGISTERS
static DEVICE_ATTR(reg_debug, S_IRUGO | S_IRWXU, k330_accel_reg_read, k330_accel_reg_store);
#endif

void k330_accel_shutdown(struct i2c_client *client)
{
	int res;
	struct k330_accel_data *data = i2c_get_clientdata(client);

	accel_dbgmsg("is called.\n");
	res = i2c_smbus_write_byte_data(data->client,
					K330ACCEL_CTRL_REG4, PM_OFF);
	if (res < 0)
		pr_err("%s: pm_off failed %d\n", __func__, res);

	res = i2c_smbus_write_byte_data(data->client,
					K330ACCEL_CTRL_REG3, 0x01);
	if (res < 0)
		pr_err("%s: sw reset failed %d\n", __func__, res);
}

#ifdef CONFIG_SENSORS_SMART_ALERT
static irqreturn_t k330_accel_interrupt_thread(int irq\
	, void *k330_accel_data_p)
{
	struct k330_accel_data *data = k330_accel_data_p;
	u8 int1_src_reg = 0, int2_src_reg = 0;
	u8 send_buf[4] = {0,};
	int err = 0;

	/* Clear Interrupt */
	int1_src_reg = (u8)i2c_smbus_read_byte_data(data->client, INT1_SRC);
	if (int1_src_reg < 0)
		pr_err("%s, read int1_src failed\n", __func__);

	int2_src_reg = (u8)i2c_smbus_read_byte_data(data->client, INT2_SRC);
	if (int2_src_reg < 0)
		pr_err("%s, read int2_src failed\n", __func__);

	send_buf[0] = (K330ACCEL_CTRL_REG1 | K330ACCEL_AC);
	/* SM1_EN : disable */
	send_buf[1] = PM_OFF;
	/* SM2_EN : disable */
	send_buf[2] = PM_OFF;
	/* IEA : falling, INT1_EN/INT2_EN : disable*/
	send_buf[3] = PM_OFF;
	err = k330_acc_i2c_write(data, send_buf, 3);
	if (err) {
		pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
			__func__, err);
	}

	data->interrupt_state = 1;
	wake_lock_timeout(&data->reactive_wake_lock, msecs_to_jiffies(2000));
	accel_dbgmsg("irq is handled\n");

	return IRQ_HANDLED;
}

static int k330_accel_reactive_alert_init(struct k330_accel_data *data)
{
	int err;
	u8 send_buf[3] = {0,};

	data->movement_recog_flag = OFF;
	/* wake lock init for accelerometer sensor */
	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		       "reactive_wake_lock");

	/* Threshold for interrupt of State Machine1 */
	err = i2c_smbus_write_byte_data(data->client, THRS1_1
		, DEFAULT_THRESHOLD);
	if (err) {
		pr_err("%s: i2c write THRS1_1 failed\n", __func__);
		goto exit;
	}
	/* Threshold for interrupt of State Machine2 */
	err = i2c_smbus_write_byte_data(data->client, THRS1_2
		, DEFAULT_THRESHOLD);
	if (err) {
		pr_err("%s: i2c write THRS1_2 failed\n"
			, __func__);
		goto exit;
	}

	/* Mask for State Machine1 */
	err = i2c_smbus_write_byte_data(data->client, MASKA_1,
		DEFAULT_MASKA_1_SETTING); /* X, Y */
	if (err) {
		pr_err("%s: i2c write MASKA_1 failed\n", __func__);
		goto exit;
	}

	/* Mask for State Machine2 */
	err = i2c_smbus_write_byte_data(data->client, MASKA_2,
		DEFAULT_MASKA_2_SETTING); /* Z */
	if (err) {
		pr_err("%s: i2c write MASKA_2 failed\n", __func__);
		goto exit;
	}

	/* Code registers for State Machine1 */
	send_buf[0] = (SM1_S0 | K330ACCEL_AC);
	send_buf[1] = DEFAULT_SM1_S0_SETTING;
	send_buf[2] = DEFAULT_SM1_S1_SETTING;
	err = k330_acc_i2c_write(data, send_buf, 2);
	if (err) {
		pr_err("%s: i2c write SM1_S0 failed\n", __func__);
		goto exit;
	}

	/* Code registers for State Machine1 */
	send_buf[0] = (SM2_S0 | K330ACCEL_AC);
	send_buf[1] = DEFAULT_SM2_S0_SETTING;
	send_buf[2] = DEFAULT_SM2_S1_SETTING;
	err = k330_acc_i2c_write(data, send_buf, 2);
	if (err) {
		pr_err("%s: i2c write SM2_S0 failed\n", __func__);
		goto exit;
	}

	err = i2c_smbus_write_byte_data(data->client, SM1_SETT,
		0x01);
	if (err) {
		pr_err("%s: i2c write SM1_SETT failed\n", __func__);
		goto exit;
	}
	err = i2c_smbus_write_byte_data(data->client, SM2_SETT,
		0x01);
	if (err) {
		pr_err("%s: i2c write SM2_SETT failed\n", __func__);
		goto exit;
	}

	err = request_threaded_irq(data->client->irq, NULL,
		k330_accel_interrupt_thread\
		, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,\
			"k330_accel", data);
	if (err < 0) {
		pr_err("%s: can't allocate irq.\n", __func__);
		goto exit;
	}

	disable_irq(data->client->irq);
	device_init_wakeup(&data->client->dev, 1);

	return err;
exit:
	pr_err("%s, err = %d\n", __func__, err);
	wake_lock_destroy(&data->reactive_wake_lock);
	return err;
}
#endif

/* sets default init values to be written in registers at probe stage */
static void k330_acc_set_init_register_values(struct k330_accel_data *data)
{
	int err;

	data->resume_state[K330_RES_LC_L] = 0x00;
	data->resume_state[K330_RES_LC_H] = 0x00;

	data->resume_state[K330_RES_CTRL_REG1] = K330_INT_ACT_H;
	data->resume_state[K330_RES_CTRL_REG2] = 0x00;
	data->resume_state[K330_RES_CTRL_REG3] = 0x00;
	data->resume_state[K330_RES_CTRL_REG4] = 0x00;
	/* BW : 50Hz, Full Scale = +-2G */
	data->resume_state[K330_RES_CTRL_REG5] = 0xC0;

	/* Need to write to the registers */
	err = i2c_smbus_write_byte_data(data->client, K330ACCEL_CTRL_REG5,
		data->resume_state[K330_RES_CTRL_REG5]);
	if (err)
		pr_err("%s: i2c write K330ACCEL_CTRL_REG5 failed\n", __func__);
}

static int k330_read_offset_register(struct k330_accel_data *data)
{
	int err;
	u8 buf[4];
	int offset_x, offset_y, offset_z;
	bool sum_when_gyro_is_on;

	/* TO READ FLASH */
	err = i2c_smbus_write_byte_data(data->client, K330ACCEL_ENABLE_FLASH,
			FLASH_ENALBE);
	if (err)
		pr_err("%s: i2c write ENABLE_FLASH failed\n", __func__);

	if (err < 0)
		goto k330_read_offset_register_error;

	err = i2c_smbus_read_i2c_block_data(data->client,
		K330_ACC_OFFSET_ADDR | K330ACCEL_AC, sizeof(buf), buf);

	if ((err != sizeof(buf)) || ((buf[0] != K330_ACC_OPCODE_1) &&\
		(buf[0] != K330_ACC_OPCODE_2))) {
		data->acc_offset.x = K330_ACC_OFFSET_X;
		data->acc_offset.y = K330_ACC_OFFSET_Y;
		data->acc_offset.z = MG_TO_LSB(K330_ACC_OFFSET_Z);
		data->sum_when_gyro_is_on = false;
	} else {
		if (buf[0] == K330_ACC_OPCODE_1) {
			offset_x = (0x7f & buf[1]);
			if ((0x80 & buf[1]) != 0)
				offset_x = -offset_x;
			offset_y = (0x7f & buf[2]);
			if ((0x80 & buf[2]) != 0)
				offset_y = -offset_y;
			offset_z = (0x7f & buf[3]);
			if ((0x80 & buf[3]) != 0)
				offset_z = -offset_z;
			sum_when_gyro_is_on = true;
		} else if (buf[0] == K330_ACC_OPCODE_2) {
			offset_x = -(0x7f & buf[1]);
			if ((0x80 & buf[1]) != 0)
				offset_x = -offset_x;
			offset_y = -(0x7f & buf[2]);
			if ((0x80 & buf[2]) != 0)
				offset_y = -offset_y;
			offset_z = -(0x7f & buf[3]);
			if ((0x80 & buf[3]) != 0)
				offset_z = -offset_z;
			sum_when_gyro_is_on = true;
		}
		data->acc_offset.x = MG_TO_LSB(offset_x);
		data->acc_offset.y = MG_TO_LSB(offset_y);
		data->acc_offset.z = MG_TO_LSB(offset_z);
		data->sum_when_gyro_is_on = sum_when_gyro_is_on;
	}

	pr_info("%s, buf[0] = 0x%x, sum_when_gyro_is_on = %u\n",
		__func__, buf[0], data->sum_when_gyro_is_on);
	/* TO LOCK FLASH */
	err = i2c_smbus_write_byte_data(data->client, K330ACCEL_ENABLE_FLASH,
			PM_OFF);
	if (err)
		pr_err("%s: i2c write ENABLE_FLASH failed\n", __func__);

k330_read_offset_register_error:
	return err;
}

static int k330_accel_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct k330_accel_data *data;
	struct accel_platform_data *pdata;
	int err = 0, retry = 0;

	accel_dbgmsg("is started\n");

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		pr_err("%s: i2c functionality failed!\n", __func__);
		goto exit_check_functionality_failed;
	}

	data = kzalloc(sizeof(struct k330_accel_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit_check_functionality_failed;
	}

	/* Checking device */
	do {
		err = i2c_smbus_write_byte_data(client, K330ACCEL_CTRL_REG4,
			PM_OFF);
		if (err == 0)
			break;

		pr_err("%s, i2c fail.(err = %d, retry = %d)\n", __func__, err,
			retry);
		usleep_range(10000, 11000);
		retry++;
	} while (err && retry < 3);

	if (err) {
		pr_err("%s: there is no such device, err = %d\n",
			__func__, err);
		goto err_read_reg;
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->read_lock);
	mutex_init(&data->write_lock);
	atomic_set(&data->opened, 0);

	/* sensor HAL expects to find /dev/accelerometer */
	data->k330_accel_device.minor = MISC_DYNAMIC_MINOR;
	data->k330_accel_device.name = ACC_DEV_NAME;
	data->k330_accel_device.fops = &k330_accel_fops;

	err = misc_register(&data->k330_accel_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* resume state init config */
	memset(data->resume_state, 0, ARRAY_SIZE(data->resume_state));
	k330_acc_set_init_register_values(data);

	/* reactive alert */
#ifdef CONFIG_SENSORS_SMART_ALERT
	err = k330_accel_reactive_alert_init(data);
	if (err) {
		pr_err("%s: k330_accel_reactive_alert_init failed\n", __FILE__);
		goto err_reactive_init;
	}
#endif
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
#ifdef K330_ACCEL_DEBUG_REGISTERS
	err = device_create_file(data->dev, &dev_attr_reg_debug);
		if (err < 0) {
			pr_err("%s: Failed to create device file(%s)\n",
					__func__, dev_attr_reg_debug.attr.name);
			goto err_reg_debug_device_create_file;
		}
#endif

	/* set mounting position of the sensor */
	pdata = client->dev.platform_data;

	if (pdata) {
		data->gyro_en = pdata->gyro_en;
		*data->gyro_en = 0;
	}

	if (pdata && pdata->select_func) {
		data->select_func = pdata->select_func;
		data->position = pdata->accel_get_position();
		data->convert_axes = pdata->select_func(data->position);
		accel_dbgmsg("position = %d\n", data->position);
	}

	err = device_create_file(data->dev, &dev_attr_position);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
			__func__, dev_attr_position.attr.name);
		goto err_position_device_create_file;
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

#ifdef CONFIG_SENSORS_SMART_ALERT
	err = device_create_file(data->dev, &dev_attr_reactive_alert);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_reactive_alert.attr.name);
		goto err_reactive_device_create_file;
	}
#endif

	err = k330_read_offset_register(data);
	if (err < 0)
		goto err_read_offset_register;

	dev_set_drvdata(data->dev, data);

	return 0;

err_read_offset_register:
#ifdef CONFIG_SENSORS_SMART_ALERT
	device_remove_file(data->dev, &dev_attr_reactive_alert);
err_reactive_device_create_file:
#endif
	device_remove_file(data->dev, &dev_attr_name);
err_name_device_create_file:
	device_remove_file(data->dev, &dev_attr_vendor);
err_vendor_device_create_file:
	device_remove_file(data->dev, &dev_attr_position);
err_position_device_create_file:
#ifdef K330_ACCEL_DEBUG_REGISTERS
	device_remove_file(data->dev, &dev_attr_reg_debug);
err_reg_debug_device_create_file:
#endif
	device_remove_file(data->dev, &dev_attr_calibration);
err_cal_device_create_file:
	device_remove_file(data->dev, &dev_attr_raw_data);
err_acc_device_create_file:
	sensors_classdev_unregister(data->dev);
err_acc_device_create:
#ifdef CONFIG_SENSORS_SMART_ALERT
	free_irq(data->client->irq, data);
	wake_lock_destroy(&data->reactive_wake_lock);
err_reactive_init:
#endif
	misc_deregister(&data->k330_accel_device);
err_misc_register:
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
err_read_reg:
	kfree(data);
exit_check_functionality_failed:
	return err;
}

static int k330_accel_remove(struct i2c_client *client)
{
	struct k330_accel_data *data = i2c_get_clientdata(client);
	int err = 0;

	if (atomic_read(&data->opened) > 0) {
		err = i2c_smbus_write_byte_data(data->client,
					K330ACCEL_CTRL_REG4, PM_OFF);
		if (err < 0)
			pr_err("%s: pm_off failed %d\n", __func__, err);
	}

	device_remove_file(data->dev, &dev_attr_name);
	device_remove_file(data->dev, &dev_attr_vendor);
	device_remove_file(data->dev, &dev_attr_position);
#ifdef K330_ACCEL_DEBUG_REGISTERS
	device_remove_file(data->dev, &dev_attr_reg_debug);
#endif

#ifdef CONFIG_SENSORS_SMART_ALERT
	device_remove_file(data->dev, &dev_attr_reactive_alert);
#endif
	device_remove_file(data->dev, &dev_attr_calibration);
	device_remove_file(data->dev, &dev_attr_raw_data);
	sensors_classdev_unregister(data->dev);

#ifdef CONFIG_SENSORS_SMART_ALERT
	device_init_wakeup(&data->client->dev, 0);
	free_irq(data->client->irq, data);
	wake_lock_destroy(&data->reactive_wake_lock);
#endif

	misc_deregister(&data->k330_accel_device);
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
	kfree(data);

	return 0;
}

static const struct i2c_device_id k330_accel_id[] = {
	{ "k330_accel", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, k330_accel_id);

static struct i2c_driver k330_accel_driver = {
	.probe = k330_accel_probe,
	.shutdown = k330_accel_shutdown,
	.remove = __devexit_p(k330_accel_remove),
	.id_table = k330_accel_id,
	.driver = {
		.pm = &k330_accel_pm_ops,
		.owner = THIS_MODULE,
		.name = "k330_accel",
	},
};

static int __init k330_accel_init(void)
{
	return i2c_add_driver(&k330_accel_driver);
}

static void __exit k330_accel_exit(void)
{
	i2c_del_driver(&k330_accel_driver);
}

module_init(k330_accel_init);
module_exit(k330_accel_exit);

MODULE_DESCRIPTION("K330 accelerometer driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
