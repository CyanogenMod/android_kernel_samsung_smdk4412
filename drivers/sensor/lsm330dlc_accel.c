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
#include <linux/sensor/lsm330dlc_accel.h>
#include <linux/sensor/sensors_core.h>
#include <linux/printk.h>
#include <linux/wakelock.h>

/* For debugging */
#if 1
#define accel_dbgmsg(str, args...)\
	pr_info("%s: " str, __func__, ##args)
#else
#define accel_dbgmsg(str, args...)\
	pr_debug("%s: " str, __func__, ##args)
#endif

#undef LSM330DLC_ACCEL_LOGGING
#undef DEBUG_ODR
#undef DEBUG_REACTIVE_ALERT
/* It will be used, when google fusion is enabled. */
#undef USES_INPUT_DEV

#define VENDOR		"STM"
#define CHIP_ID		"LSM330"

/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 */
#define DEFAULT_POWER_ON_SETTING (ODR400 | ENABLE_ALL_AXES)

#ifdef USES_MOVEMENT_RECOGNITION
#define DEFAULT_CTRL3_SETTING		0x60 /* INT1_A enable */
#define DEFAULT_CTRL6_SETTING		0x20 /* INT2_A enable */
#define DEFAULT_INTERRUPT_SETTING	0x0A /* INT1_A XH,YH : enable */
#define DEFAULT_INTERRUPT2_SETTING	0x20 /* INT2_A ZH enable */
#define DEFAULT_THRESHOLD		0x7F /* 2032mg (16*0x7F) */
#define DYNAMIC_THRESHOLD		300	/* mg */
#define DYNAMIC_THRESHOLD2		700	/* mg */
#define MOVEMENT_DURATION		0x00 /*INT1_A (DURATION/odr)ms*/
enum {
	OFF = 0,
	ON = 1
};
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#define ACC_DEV_NAME "accelerometer"

#ifdef CONFIG_SLP
#define CALIBRATION_FILE_PATH	"/csa/sensor/accel_cal_data"
#else
#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#endif
#define CAL_DATA_AMOUNT	20

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	u64 delay_ns; /* odr in ns */
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

static const int position_map[][3][3] = {
	{{-1,  0,  0}, { 0, -1,  0}, { 0,  0,  1} }, /* 0 top/upper-left */
	{{ 0, -1,  0}, { 1,  0,  0}, { 0,  0,  1} }, /* 1 top/upper-right */
	{{ 1,  0,  0}, { 0,  1,  0}, { 0,  0,  1} }, /* 2 top/lower-right */
	{{ 0,  1,  0}, {-1,  0,  0}, { 0,  0,  1} }, /* 3 top/lower-left */
	{{ 1,  0,  0}, { 0, -1,  0}, { 0,  0, -1} }, /* 4 bottom/upper-left */
	{{ 0,  1,  0}, { 1,  0,  0}, { 0,  0, -1} }, /* 5 bottom/upper-right */
	{{-1,  0,  0}, { 0,  1,  0}, { 0,  0, -1} }, /* 6 bottom/lower-right */
	{{ 0, -1,  0}, {-1,  0,  0}, { 0,  0, -1} }, /* 7 bottom/lower-left*/
};

struct lsm330dlc_accel_data {
	struct i2c_client *client;
	struct miscdevice lsm330dlc_accel_device;
	struct mutex read_lock;
	struct mutex write_lock;
	struct lsm330dlc_acc cal_data;
	struct device *dev;
	u8 ctrl_reg1_shadow;
	atomic_t opened; /* opened implies enabled */
#ifdef USES_MOVEMENT_RECOGNITION
	int movement_recog_flag;
	unsigned char interrupt_state;
	struct wake_lock reactive_wake_lock;
#endif
	ktime_t poll_delay;
#ifdef USES_INPUT_DEV
	struct input_dev *input_dev;
	struct work_struct work;
	struct workqueue_struct *work_queue;
	struct hrtimer timer;
#endif
	int position;
	struct lsm330dlc_acc acc_xyz;
	bool axis_adjust;
};

 /* Read X,Y and Z-axis acceleration raw data */
static int lsm330dlc_accel_read_raw_xyz(struct lsm330dlc_accel_data *data,
				struct lsm330dlc_acc *acc)
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
	acc->z = acc->z >> 4;

#if defined(CONFIG_MACH_M3_JPN_DCM)
	acc->y = -acc->y;
#endif

	return 0;
}

static int lsm330dlc_accel_read_xyz(struct lsm330dlc_accel_data *data,
				struct lsm330dlc_acc *acc)
{
	int err = 0;

	mutex_lock(&data->read_lock);
	err = lsm330dlc_accel_read_raw_xyz(data, acc);
	mutex_unlock(&data->read_lock);
	if (err < 0) {
		pr_err("%s: lsm330dlc_accel_read_xyz() failed\n", __func__);
		return err;
	}

	acc->x -= data->cal_data.x;
	acc->y -= data->cal_data.y;
	acc->z -= data->cal_data.z;

	return err;
}

static int lsm330dlc_accel_open_calibration(struct lsm330dlc_accel_data *data)
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

	accel_dbgmsg("(%d,%d,%d)\n",
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int lsm330dlc_accel_do_calibrate(struct device *dev, bool do_calib)
{
	struct lsm330dlc_accel_data *acc_data = dev_get_drvdata(dev);
	struct lsm330dlc_acc data = { 0, };
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err = 0;
	int i;
	mm_segment_t old_fs;

	if (do_calib) {
		for (i = 0; i < CAL_DATA_AMOUNT; i++) {
			mutex_lock(&acc_data->read_lock);
			err = lsm330dlc_accel_read_raw_xyz(acc_data, &data);
			mutex_unlock(&acc_data->read_lock);
			if (err < 0) {
				pr_err("%s: lsm330dlc_accel_read_raw_xyz() "
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
		if (acc_data->cal_data.z > 0) {
			acc_data->cal_data.z -= 1024;
			pr_info("acc_data->cal_data.z= %d\n",
				acc_data->cal_data.z);
		} else if (acc_data->cal_data.z < 0) {
			acc_data->cal_data.z += 1024;
			pr_info("acc_data->cal_data.z= %d\n",
				acc_data->cal_data.z);
		}
	} else {
		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;
	}

	printk(KERN_INFO "%s: cal data (%d,%d,%d)\n", __func__,
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

static int lsm330dlc_accel_enable(struct lsm330dlc_accel_data *data)
{
	int err = 0;

	mutex_lock(&data->write_lock);
	if (atomic_read(&data->opened) == 0) {
		err = lsm330dlc_accel_open_calibration(data);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: lsm330dlc_accel_open_calibration() failed\n",
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
#ifdef USES_INPUT_DEV
		hrtimer_start(&data->timer, data->poll_delay, HRTIMER_MODE_REL);
#endif
	}

	atomic_set(&data->opened, 1);
	mutex_unlock(&data->write_lock);

	return err;
}

static int lsm330dlc_accel_disable(struct lsm330dlc_accel_data *data)
{
	int err = 0;

	mutex_lock(&data->write_lock);
#ifdef USES_MOVEMENT_RECOGNITION
	if (data->movement_recog_flag == ON) {
		accel_dbgmsg("LOW_PWR_MODE.\n");
		err = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, LOW_PWR_MODE);
		if (atomic_read(&data->opened) == 1)
			data->ctrl_reg1_shadow = PM_OFF;
	} else if (atomic_read(&data->opened) == 1) {
#else
	if (atomic_read(&data->opened) == 1) {
#endif
#ifdef USES_INPUT_DEV
		hrtimer_cancel(&data->timer);
		cancel_work_sync(&data->work);
#endif
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
							PM_OFF);
		data->ctrl_reg1_shadow = PM_OFF;
	}
	atomic_set(&data->opened, 0);
	mutex_unlock(&data->write_lock);

	return err;
}

/*  open command for lsm330dlc_accel device file  */
static int lsm330dlc_accel_open(struct inode *inode, struct file *file)
{
	accel_dbgmsg("is called.\n");
	return 0;
}

/*  release command for lsm330dlc_accel device file */
static int lsm330dlc_accel_close(struct inode *inode, struct file *file)
{
	accel_dbgmsg("is called.\n");
	return 0;
}

static int lsm330dlc_accel_set_delay(struct lsm330dlc_accel_data *data
	, u64 delay_ns)
{
	int odr_value = ODR1;
	int err = 0, i;

	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	mutex_lock(&data->write_lock);
#ifdef USES_INPUT_DEV
	if (atomic_read(&data->opened))
		hrtimer_cancel(&data->timer);
#endif

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
	if (odr_value != (data->ctrl_reg1_shadow & ODR_MASK)) {
		u8 ctrl = (data->ctrl_reg1_shadow & ~ODR_MASK);
		ctrl |= odr_value;
		data->ctrl_reg1_shadow = ctrl;
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1, ctrl);
		if (err < 0)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
	}

#ifdef USES_INPUT_DEV
	if (atomic_read(&data->opened))
		hrtimer_start(&data->timer, data->poll_delay,
			HRTIMER_MODE_REL);
#endif
	mutex_unlock(&data->write_lock);
	return err;
}

/*  ioctl command for lsm330dlc_accel device file */
static long lsm330dlc_accel_ioctl(struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct lsm330dlc_accel_data *data =
		container_of(file->private_data, struct lsm330dlc_accel_data,
			lsm330dlc_accel_device);
	u64 delay_ns;
	int enable = 0, j;
	s16 raw[3] = {0,};
	struct lsm330dlc_acc xyz_adjusted = {0,};

	/* cmd mapping */
	switch (cmd) {
	case LSM330DLC_ACCEL_IOCTL_SET_ENABLE:
		if (copy_from_user(&enable, (void __user *)arg,
					sizeof(enable)))
			return -EFAULT;

		accel_dbgmsg("opened = %d, enable = %d\n",
			atomic_read(&data->opened), enable);
		if (enable)
			err = lsm330dlc_accel_enable(data);
		else
			err = lsm330dlc_accel_disable(data);
		break;
	case LSM330DLC_ACCEL_IOCTL_SET_DELAY:
		if (copy_from_user(&delay_ns, (void __user *)arg,
					sizeof(delay_ns)))
			return -EFAULT;
		err = lsm330dlc_accel_set_delay(data, delay_ns);

		break;
	case LSM330DLC_ACCEL_IOCTL_GET_DELAY:
		if (put_user(ktime_to_ns(data->poll_delay), (u64 __user *)arg))
			return -EFAULT;
		break;
	case LSM330DLC_ACCEL_IOCTL_READ_XYZ:
		err = lsm330dlc_accel_read_xyz(data, &data->acc_xyz);
		#ifdef LSM330DLC_ACCEL_LOGGING
		accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
			data->acc_xyz.x, data->acc_xyz.y,
			data->acc_xyz.z);
		#endif
		if (err)
			break;
		if (data->axis_adjust) {
			raw[0] = data->acc_xyz.x;
			raw[1] = data->acc_xyz.y;
			raw[2] = data->acc_xyz.z;
			for (j = 0; j < 3; j++) {
				xyz_adjusted.x +=
				(position_map[data->position][0][j] * raw[j]);
				xyz_adjusted.y +=
				(position_map[data->position][1][j] * raw[j]);
				xyz_adjusted.z +=
				(position_map[data->position][2][j] * raw[j]);
			}
#ifdef LSM330DLC_ACCEL_LOGGING
			accel_dbgmsg("adjusted x = %d, y = %d, z = %d\n",
				xyz_adjusted.x, xyz_adjusted.y, xyz_adjusted.z);
#endif
			if (copy_to_user((void __user *)arg,
				&xyz_adjusted, sizeof(xyz_adjusted)))
				return -EFAULT;
		} else
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

static int lsm330dlc_accel_suspend(struct device *dev)
{
	int res = 0;
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

#ifdef USES_INPUT_DEV
	if (atomic_read(&data->opened) > 0) {
		hrtimer_cancel(&data->timer);
		cancel_work_sync(&data->work);
	}
#endif

#ifdef USES_MOVEMENT_RECOGNITION
	if (data->movement_recog_flag == ON) {
		accel_dbgmsg("LOW_PWR_MODE.\n");
		res = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, LOW_PWR_MODE);
	} else if (atomic_read(&data->opened) > 0) {
#else
	if (atomic_read(&data->opened) > 0) {
#endif
		accel_dbgmsg("PM_OFF.\n");
		res = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, PM_OFF);
	}

	return res;
}

static int lsm330dlc_accel_resume(struct device *dev)
{
	int res = 0;
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	if (atomic_read(&data->opened) > 0) {
		accel_dbgmsg("ctrl_reg1_shadow = 0x%x\n"
			, data->ctrl_reg1_shadow);
		res = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						data->ctrl_reg1_shadow);
#ifdef USES_INPUT_DEV
		hrtimer_start(&data->timer,
				data->poll_delay, HRTIMER_MODE_REL);
#endif
	}

	return res;
}

static const struct dev_pm_ops lsm330dlc_accel_pm_ops = {
	.suspend = lsm330dlc_accel_suspend,
	.resume = lsm330dlc_accel_resume,
};

static const struct file_operations lsm330dlc_accel_fops = {
	.owner = THIS_MODULE,
	.open = lsm330dlc_accel_open,
	.release = lsm330dlc_accel_close,
	.unlocked_ioctl = lsm330dlc_accel_ioctl,
};

static ssize_t lsm330dlc_accel_fs_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	if (data->axis_adjust) {
		int i, j;
		s16 raw[3] = {0,}, accel_adjusted[3] = {0,};

		raw[0] = data->acc_xyz.x;
		raw[1] = data->acc_xyz.y;
		raw[2] = data->acc_xyz.z;
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++)
				accel_adjusted[i]
				+= position_map[data->position][i][j] * raw[j];
		}
		return sprintf(buf, "%d,%d,%d\n",
			accel_adjusted[0], accel_adjusted[1],
			accel_adjusted[2]);
	} else
		return sprintf(buf, "%d,%d,%d\n",
			data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
}

static ssize_t lsm330dlc_accel_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	err = lsm330dlc_accel_open_calibration(data);
	if (err < 0)
		pr_err("%s: lsm330dlc_accel_open_calibration() failed\n"\
		, __func__);

	if (!data->cal_data.x && !data->cal_data.y && !data->cal_data.z)
		err = -1;

	return sprintf(buf, "%d %d %d %d\n",
		err, data->cal_data.x, data->cal_data.y, data->cal_data.z);
}

static ssize_t lsm330dlc_accel_calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
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

	err = lsm330dlc_accel_do_calibrate(dev, do_calib);
	if (err < 0) {
		pr_err("%s: lsm330dlc_accel_do_calibrate() failed\n", __func__);
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

#ifdef USES_MOVEMENT_RECOGNITION
static ssize_t lsm330dlc_accel_reactive_alert_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int onoff = OFF, err = 0, ctrl_reg = 0;
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
	bool factory_test = false;
	struct lsm330dlc_acc raw_data;
	u8 thresh1 = 0, thresh2 = 0;

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

	if (onoff == ON && data->movement_recog_flag == OFF) {
		accel_dbgmsg("reactive alert is on.\n");
		data->interrupt_state = 0; /* Init interrupt state.*/

		if (atomic_read(&data->opened) == 0) {
			err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
				FASTEST_MODE);
			if (err) {
				ctrl_reg = CTRL_REG1;
				goto err_i2c_write;
			}
			/* trun on time, T = 7/odr ms */
			usleep_range(10000, 10000);
		}
		enable_irq(data->client->irq);
		if (device_may_wakeup(&data->client->dev))
			enable_irq_wake(data->client->irq);
		/* Get x, y, z data to set threshold1, threshold2. */
		err = lsm330dlc_accel_read_xyz(data, &raw_data);
		accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
			raw_data.x, raw_data.y, raw_data.z);
		if (err < 0) {
			pr_err("%s: lsm330dlc_accel_read_xyz failed\n",
				__func__);
			goto exit;
		}
		if (atomic_read(&data->opened) == 0) {
			err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
				LOW_PWR_MODE); /* Change to 50Hz*/
			if (err) {
				ctrl_reg = CTRL_REG1;
				goto err_i2c_write;
			}
		}
		/* Change raw data to threshold value & settng threshold */
		thresh1 = (MAX(ABS(raw_data.x), ABS(raw_data.y))
				+ DYNAMIC_THRESHOLD)/16;
		if (factory_test == true)
			thresh2 = 0; /* for z axis */
		else
			thresh2 = (ABS(raw_data.z) + DYNAMIC_THRESHOLD2)/16;
		accel_dbgmsg("threshold1 = 0x%x, threshold2 = 0x%x\n",
			thresh1, thresh2);
		err = i2c_smbus_write_byte_data(data->client, INT1_THS
				, thresh1);
		if (err) {
			ctrl_reg = INT1_THS;
			goto err_i2c_write;
		}
		ctrl_reg = INT2_THS;
		err = i2c_smbus_write_byte_data(data->client, INT2_THS,
			thresh2);
		if (err) {
			ctrl_reg = INT2_THS;
			goto err_i2c_write;
		}
		/* INT_A enable */
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG3,
			DEFAULT_CTRL3_SETTING);
		if (err) {
			ctrl_reg = CTRL_REG3;
			goto err_i2c_write;
		}
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG6,
			DEFAULT_CTRL6_SETTING);
		if (err) {
			ctrl_reg = CTRL_REG6;
			goto err_i2c_write;
		}

		data->movement_recog_flag = ON;
	} else if (onoff == OFF && data->movement_recog_flag == ON) {
		accel_dbgmsg("reactive alert is off.\n");
		/* INT_A disable */
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG3,
			PM_OFF);
		if (err) {
			ctrl_reg = CTRL_REG3;
			goto err_i2c_write;
		}
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG6,
			PM_OFF);
		if (err) {
			ctrl_reg = CTRL_REG6;
			goto err_i2c_write;
		}
		if (device_may_wakeup(&data->client->dev))
			disable_irq_wake(data->client->irq);
		disable_irq_nosync(data->client->irq);
		/* return the power state */
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
			data->ctrl_reg1_shadow);
		if (err) {
			ctrl_reg = CTRL_REG1;
			goto err_i2c_write;
		}
		data->movement_recog_flag = OFF;
		data->interrupt_state = 0; /* Init interrupt state.*/
	}
	return count;
err_i2c_write:
	pr_err("%s: i2c write ctrl_reg = 0x%d failed(err=%d)\n",
				__func__, ctrl_reg, err);
exit:
	return ((err < 0) ? err : -err);
}

static ssize_t lsm330dlc_accel_reactive_alert_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->interrupt_state);
}
#endif

#ifdef DEBUG_ODR
static ssize_t lsm330dlc_accel_odr_read(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
	u8 odr_data = 0, fs_data = 0;

	/* read output data rate & operation mode */
	odr_data = (u8)i2c_smbus_read_byte_data(data->client, CTRL_REG1);
	if (odr_data < 0)
		pr_err("%s, read CTRL_REG1 failed\n", __func__);

	/* read full scale setting */
	fs_data = (u8)i2c_smbus_read_byte_data(data->client, CTRL_REG4);
	if (fs_data < 0)
		pr_err("%s, read CTRL_REG4 failed\n", __func__);

	return sprintf(buf, "odr:0x%x,fs:0x%x\n",
		odr_data, fs_data);
}
#endif

#ifdef USES_INPUT_DEV
static ssize_t lsm330dlc_accel_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", ktime_to_ns(data->poll_delay));
}

static ssize_t lsm330dlc_accel_delay_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err = 0;
	u64 delay_ns = 0;
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	err = strict_strtoll(buf, 10, &delay_ns);

	if (err < 0)
		return err;

	lsm330dlc_accel_set_delay(data, delay_ns);

	return count;
}

static ssize_t lsm330dlc_enable_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&data->opened));
}

static ssize_t lsm330dlc_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
	int enable = -1, err = 0;

	err = kstrtoint(buf, 10, &enable);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	accel_dbgmsg("opened = %d, enable = %d\n",
			atomic_read(&data->opened), enable);

	if (enable)
		lsm330dlc_accel_enable(data);
	else
		lsm330dlc_accel_disable(data);

	return count;
}

static DEVICE_ATTR(poll_delay, 0644,
	lsm330dlc_accel_delay_show, lsm330dlc_accel_delay_store);
static DEVICE_ATTR(enable, 0644,
	lsm330dlc_enable_show, lsm330dlc_enable_store);

static struct attribute *lsm330dlc_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group lsm330dlc_attribute_group = {
	.attrs =  lsm330dlc_sysfs_attrs,
};
#endif

static ssize_t
lsm330dlc_accel_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->position);
}

static ssize_t
lsm330dlc_accel_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct lsm330dlc_accel_data *data = dev_get_drvdata(dev);
	int err = 0;

	err = kstrtoint(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	return count;
}

static ssize_t lsm330dlc_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t lsm330dlc_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(name, 0664,
	lsm330dlc_accel_name_show, NULL);

static DEVICE_ATTR(vendor, 0664,
	lsm330dlc_accel_vendor_show, NULL);

static DEVICE_ATTR(position, 0664,
	lsm330dlc_accel_position_show, lsm330dlc_accel_position_store);

static DEVICE_ATTR(calibration, 0664,
		   lsm330dlc_accel_calibration_show
		   , lsm330dlc_accel_calibration_store);
static DEVICE_ATTR(raw_data, 0664, lsm330dlc_accel_fs_read, NULL);
#ifdef USES_MOVEMENT_RECOGNITION
static DEVICE_ATTR(reactive_alert, 0664,
	lsm330dlc_accel_reactive_alert_show,
	lsm330dlc_accel_reactive_alert_store);
#endif

#ifdef DEBUG_ODR
static DEVICE_ATTR(odr, 0664, lsm330dlc_accel_odr_read, NULL);
#endif

void lsm330dlc_accel_shutdown(struct i2c_client *client)
{
	int res = 0;
	struct lsm330dlc_accel_data *data = i2c_get_clientdata(client);

	accel_dbgmsg("is called.\n");
#ifdef USES_INPUT_DEV
	if (atomic_read(&data->opened) > 0) {
		hrtimer_cancel(&data->timer);
		cancel_work_sync(&data->work);
	}
#endif
	res = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
	if (res < 0)
		pr_err("%s: pm_off failed %d\n", __func__, res);
}

#ifdef USES_MOVEMENT_RECOGNITION
static irqreturn_t lsm330dlc_accel_interrupt_thread(int irq\
	, void *lsm330dlc_accel_data_p)
{
	struct lsm330dlc_accel_data *data = lsm330dlc_accel_data_p;
#ifdef DEBUG_REACTIVE_ALERT
	u8 int1_src_reg = 0, int2_src_reg = 0;
#else
	int err = 0;
#endif

#ifndef DEBUG_REACTIVE_ALERT
	/* INT1_A disable */
	err = i2c_smbus_write_byte_data(data->client, CTRL_REG3,
		PM_OFF);
	if (err)
		pr_err("%s: i2c write ctrl_reg3 failed\n", __func__);

	/* INT2_A disable */
	err = i2c_smbus_write_byte_data(data->client, CTRL_REG6,
		PM_OFF);
	if (err)
		pr_err("%s: i2c write ctrl_reg6 failed\n", __func__);
#else
	int1_src_reg = (u8)i2c_smbus_read_byte_data(data->client, INT1_SRC);
	if (int1_src_reg < 0)
		pr_err("%s, read int1_src failed\n", __func__);
	accel_dbgmsg("interrupt source reg1 = 0x%x\n", int1_src_reg);

	int2_src_reg = (u8)i2c_smbus_read_byte_data(data->client, INT2_SRC);
	if (int2_src_reg < 0)
		pr_err("%s, read int2_src failed\n", __func__);
	accel_dbgmsg("interrupt source reg2 = 0x%x\n", int2_src_reg);
#endif

	data->interrupt_state = 1;
	wake_lock_timeout(&data->reactive_wake_lock, msecs_to_jiffies(2000));
	accel_dbgmsg("irq is handled\n");

	return IRQ_HANDLED;
}
#endif

#ifdef USES_INPUT_DEV
static enum hrtimer_restart lsm330dlc_timer_func(struct hrtimer *timer)
{
	struct lsm330dlc_accel_data *data
		= container_of(timer, struct lsm330dlc_accel_data, timer);
	queue_work(data->work_queue, &data->work);
	hrtimer_forward_now(&data->timer, data->poll_delay);
	return HRTIMER_RESTART;
}

static void lsm330dlc_work_func(struct work_struct *work)
{
	struct lsm330dlc_accel_data *data
		= container_of(work, struct lsm330dlc_accel_data, work);
	s16 raw[3] = {0,}, accel_adjusted[3] = {0,};
	int i, j;

	lsm330dlc_accel_read_xyz(data, &data->acc_xyz);

	if (data->axis_adjust) {
		raw[0] = data->acc_xyz.x;
		raw[1] = data->acc_xyz.y;
		raw[2] = data->acc_xyz.z;
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++)
				accel_adjusted[i]
				+= position_map[data->position][i][j] * raw[j];
		}
	} else {
		accel_adjusted[0] = data->acc_xyz.x;
		accel_adjusted[1] = data->acc_xyz.y;
		accel_adjusted[2] = data->acc_xyz.z;
	}
	input_report_rel(data->input_dev, REL_X, accel_adjusted[0]);
	input_report_rel(data->input_dev, REL_Y, accel_adjusted[1]);
	input_report_rel(data->input_dev, REL_Z, accel_adjusted[2]);
	input_sync(data->input_dev);
#ifdef LSM330DLC_ACCEL_LOGGING
	accel_dbgmsg("raw x = %d, y = %d, z = %d\n",
		accel_adjusted[0], accel_adjusted[1], accel_adjusted[2]);
#endif
}
#endif

static int lsm330dlc_accel_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct lsm330dlc_accel_data *data;
	struct accel_platform_data *pdata;
	int err = 0;

	accel_dbgmsg("is started\n");
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		err = -ENODEV;
		goto exit;
	}

	data = kzalloc(sizeof(struct lsm330dlc_accel_data), GFP_KERNEL);
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

	mutex_init(&data->read_lock);
	mutex_init(&data->write_lock);
	atomic_set(&data->opened, 0);
#ifdef USES_INPUT_DEV
	hrtimer_init(&data->timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	data->timer.function = lsm330dlc_timer_func;
	data->work_queue =
		create_singlethread_workqueue("lsm330dlc_workqueue");
	if (!data->work_queue) {
		err = -ENOMEM;
		pr_err("%s: count not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	INIT_WORK(&data->work, lsm330dlc_work_func);

	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		pr_err("%s: count not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate;
	}

	input_set_drvdata(data->input_dev, data);
	data->input_dev->name = "accelerometer_sensor";
	input_set_capability(data->input_dev, EV_REL, REL_X);
	input_set_capability(data->input_dev, EV_REL, REL_Y);
	input_set_capability(data->input_dev, EV_REL, REL_Z);

	err = input_register_device(data->input_dev);
	if (err < 0) {
		input_free_device(data->input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_allocate;
	}

	err = sysfs_create_group(&data->input_dev->dev.kobj,
		&lsm330dlc_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_create_sysfs;
	}
#else
	/* sensor HAL expects to find /dev/accelerometer */
	data->lsm330dlc_accel_device.minor = MISC_DYNAMIC_MINOR;
	data->lsm330dlc_accel_device.name = ACC_DEV_NAME;
	data->lsm330dlc_accel_device.fops = &lsm330dlc_accel_fops;

	err = misc_register(&data->lsm330dlc_accel_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
#endif

#ifdef USES_MOVEMENT_RECOGNITION
	data->movement_recog_flag = OFF;
	/* wake lock init for accelerometer sensor */
	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		       "reactive_wake_lock");
	err = i2c_smbus_write_byte_data(data->client, INT1_THS
		, DEFAULT_THRESHOLD);
	if (err) {
		pr_err("%s: i2c write int1_ths failed\n", __func__);
		goto err_request_irq;
	}

	err = i2c_smbus_write_byte_data(data->client, INT1_DURATION
		, MOVEMENT_DURATION);
	if (err) {
		pr_err("%s: i2c write int1_duration failed\n"
		, __func__);
		goto err_request_irq;
	}

	err = i2c_smbus_write_byte_data(data->client, INT1_CFG,
		DEFAULT_INTERRUPT_SETTING);
	if (err) {
		pr_err("%s: i2c write int1_cfg failed\n", __func__);
		goto err_request_irq;
	}

	err = i2c_smbus_write_byte_data(data->client, INT2_THS
			, DEFAULT_THRESHOLD);
	if (err) {
		pr_err("%s: i2c write int2_ths failed\n", __func__);
		goto err_request_irq;
	}

	err = i2c_smbus_write_byte_data(data->client, INT2_DURATION
		, MOVEMENT_DURATION);
	if (err) {
		pr_err("%s: i2c write int1_duration failed\n"
		, __func__);
		goto err_request_irq;
	}
	err = i2c_smbus_write_byte_data(data->client, INT2_CFG,
		DEFAULT_INTERRUPT2_SETTING);
	if (err) {
		pr_err("%s: i2c write ctrl_reg3 failed\n", __func__);
		goto err_request_irq;
	}

	err = request_threaded_irq(data->client->irq, NULL,
		lsm330dlc_accel_interrupt_thread\
		, IRQF_TRIGGER_RISING | IRQF_ONESHOT,\
			"lsm330dlc_accel", data);
	if (err < 0) {
		pr_err("%s: can't allocate irq.\n", __func__);
		goto err_request_irq;
	}

	disable_irq(data->client->irq);
	device_init_wakeup(&data->client->dev, 1);
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

#ifdef USES_MOVEMENT_RECOGNITION
	err = device_create_file(data->dev, &dev_attr_reactive_alert);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_reactive_alert.attr.name);
		goto err_reactive_device_create_file;
	}
#endif

#ifdef DEBUG_ODR
	err = device_create_file(data->dev, &dev_attr_odr);
		if (err < 0) {
			pr_err("%s: Failed to create device file(%s)\n",
					__func__, dev_attr_odr.attr.name);
			goto err_odr_device_create_file;
		}
#endif

	/* set mounting position of the sensor */
	pdata = client->dev.platform_data;
	if (!pdata) {
		/*Set by default position 2, it doesn't adjust raw value*/
		data->position = 2;
		data->axis_adjust = false;
		accel_dbgmsg("using defualt position = %d\n", data->position);
	} else {
		data->position = pdata->accel_get_position();
		data->axis_adjust = pdata->axis_adjust;
		accel_dbgmsg("successful, position = %d\n", data->position);
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

	dev_set_drvdata(data->dev, data);

	return 0;

err_name_device_create_file:
	device_remove_file(data->dev, &dev_attr_vendor);
err_vendor_device_create_file:
	device_remove_file(data->dev, &dev_attr_position);
err_position_device_create_file:
#ifdef DEBUG_ODR
	device_remove_file(data->dev, &dev_attr_odr);
err_odr_device_create_file:
#ifdef USES_MOVEMENT_RECOGNITION
	device_remove_file(data->dev, &dev_attr_reactive_alert);
#else
	device_remove_file(data->dev, &dev_attr_calibration);
#endif
#endif
#ifdef USES_MOVEMENT_RECOGNITION
err_reactive_device_create_file:
	device_remove_file(data->dev, &dev_attr_calibration);
#endif
err_cal_device_create_file:
	device_remove_file(data->dev, &dev_attr_raw_data);
err_acc_device_create_file:
	sensors_classdev_unregister(data->dev);
err_acc_device_create:
#ifdef USES_MOVEMENT_RECOGNITION
	free_irq(data->client->irq, data);
err_request_irq:
	wake_lock_destroy(&data->reactive_wake_lock);
#endif
#ifdef USES_INPUT_DEV
	sysfs_remove_group(&data->input_dev->dev.kobj,
		&lsm330dlc_attribute_group);
err_create_sysfs:
	input_unregister_device(data->input_dev);
err_input_allocate:
	destroy_workqueue(data->work_queue);
err_create_workqueue:
#else
	misc_deregister(&data->lsm330dlc_accel_device);
err_misc_register:
#endif
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
err_read_reg:
	kfree(data);
exit:
	return err;
}

static int lsm330dlc_accel_remove(struct i2c_client *client)
{
	struct lsm330dlc_accel_data *data = i2c_get_clientdata(client);
	int err = 0;

	if (atomic_read(&data->opened) > 0) {
#ifdef USES_INPUT_DEV
		hrtimer_cancel(&data->timer);
		cancel_work_sync(&data->work);
#endif
		err = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
		if (err < 0)
			pr_err("%s: pm_off failed %d\n", __func__, err);
	}

	device_remove_file(data->dev, &dev_attr_name);
	device_remove_file(data->dev, &dev_attr_vendor);
	device_remove_file(data->dev, &dev_attr_position);
#ifdef DEBUG_ODR
	device_remove_file(data->dev, &dev_attr_odr);
#endif
#ifdef USES_MOVEMENT_RECOGNITION
	device_remove_file(data->dev, &dev_attr_reactive_alert);
#endif
	device_remove_file(data->dev, &dev_attr_calibration);
	device_remove_file(data->dev, &dev_attr_raw_data);
	sensors_classdev_unregister(data->dev);

#ifdef USES_MOVEMENT_RECOGNITION
	device_init_wakeup(&data->client->dev, 0);
	free_irq(data->client->irq, data);
	wake_lock_destroy(&data->reactive_wake_lock);
#endif
#ifdef USES_INPUT_DEV
	destroy_workqueue(data->work_queue);
	sysfs_remove_group(&data->input_dev->dev.kobj,
		&lsm330dlc_attribute_group);
	input_unregister_device(data->input_dev);
#else
	misc_deregister(&data->lsm330dlc_accel_device);
#endif
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
	kfree(data);

	return 0;
}

static const struct i2c_device_id lsm330dlc_accel_id[] = {
	{ "lsm330dlc_accel", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lsm330dlc_accel_id);

static struct i2c_driver lsm330dlc_accel_driver = {
	.probe = lsm330dlc_accel_probe,
	.shutdown = lsm330dlc_accel_shutdown,
	.remove = __devexit_p(lsm330dlc_accel_remove),
	.id_table = lsm330dlc_accel_id,
	.driver = {
		.pm = &lsm330dlc_accel_pm_ops,
		.owner = THIS_MODULE,
		.name = "lsm330dlc_accel",
	},
};

static int __init lsm330dlc_accel_init(void)
{
	return i2c_add_driver(&lsm330dlc_accel_driver);
}

static void __exit lsm330dlc_accel_exit(void)
{
	i2c_del_driver(&lsm330dlc_accel_driver);
}

module_init(lsm330dlc_accel_init);
module_exit(lsm330dlc_accel_exit);

MODULE_DESCRIPTION("LSM330DLC accelerometer driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
