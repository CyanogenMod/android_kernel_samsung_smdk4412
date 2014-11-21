/*
 *  Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <asm/div64.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/sensor/k330_gyro.h>
#include <linux/sensor/sensors_core.h>

#undef LOGGING_GYRO
#undef DEBUG_REGISTER

#define VENDOR		"STM"
#define CHIP_ID		"K330"

#ifdef CONFIG_SLP
#define CALIBRATION_FILE_PATH	"/csa/sensor/gyro_cal_data"
#else
#define CALIBRATION_FILE_PATH	"/efs/gyro_cal_data"
#endif

/* default register setting for device init */
static const char default_ctrl_regs_bypass[] = {
	0x3F,	/* 95HZ, BW25, PM-normal, xyz enable */
	0x10,	/* reference signal for filtering */
	0x00,	/* fifo wtm interrupt off */
	0x90,	/* block data update, 500d/s */
	0x12,	/* fifo disable, high-pass filter enable */
};
static const char default_ctrl_regs_fifo[] = {
	0x3F,	/* 95HZ, BW25, PM-normal, xyz enable */
	0x00,	/* normal mode */
	0x04,	/* fifo wtm interrupt on */
	0x90,	/* block data update, 500d/s */
	0x40,	/* fifo enable */
};

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	u64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{ ODR760_BW30, 1315789LL }, /* 760Hz */
	{ ODR380_BW25, 2631578LL }, /* 380Hz */
	{ ODR190_BW25, 5263157LL }, /* 190Hz */
	{ ODR95_BW25, 10526315LL }, /* 95Hz */
};

/*
 * k330_gyro gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */
struct gyro_t {
	s16 x;
	s16 y;
	s16 z;
};

struct k330_gyro_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct mutex lock;
	struct workqueue_struct *k330_gyro_wq;
	struct work_struct work;
	struct hrtimer timer;
	atomic_t opened;
	bool enable;
	bool self_test;		/* is self_test or not? */
	bool interruptible;	/* interrupt or polling? */
	int entries;		/* number of fifo entries */
	int dps;		/* scale selection */
	/* fifo data entries */
	u8 fifo_data[sizeof(struct gyro_t) * 32];
	u8 ctrl_regs[5];	/* saving register settings */
	u32 time_to_read;	/* time needed to read one entry */
	ktime_t polling_delay;	/* polling time for timer */
	struct gyro_t xyz_data;

	struct device *dev;
	struct completion data_ready;
#ifdef LOGGING_GYRO
	int count;
#endif
	struct gyro_t cal_data;
	u8 position;
	axes_func_s16 convert_axes;
	bool *gyro_en;
	axes_func_s16 (*select_func) (u8);
};

static int k330_gyro_open_calibration(struct k330_gyro_data *data)
{
	struct file *cal_filp;
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

	pr_info("%s: (%d,%d,%d)\n", __func__,
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k330_gyro_restart_fifo(struct k330_gyro_data *data)
{
	int res;

	res = i2c_smbus_write_byte_data(data->client,
			FIFO_CTRL_REG, BYPASS_MODE);
	if (res < 0) {
		pr_err("%s : failed to set bypass_mode\n", __func__);
		return res;
	}

	res = i2c_smbus_write_byte_data(data->client,
			FIFO_CTRL_REG, FIFO_MODE | (data->entries - 1));

	if (res < 0)
		pr_err("%s : failed to set fifo_mode\n", __func__);

	return res;
}

/* gyroscope data readout */
static int k330_gyro_read_values(struct i2c_client *client,
				struct gyro_t *data, int total_read)
{
	int err;
	int len = sizeof(*data) * (total_read ? (total_read - 1) : 1);
	struct k330_gyro_data *gyro_data = i2c_get_clientdata(client);
	struct i2c_msg msg[2];
	u8 reg_buf = AXISDATA_REG | AC;

	msg[0].addr = client->addr;
	msg[0].buf = &reg_buf;
	msg[0].flags = 0;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = gyro_data->fifo_data;

	if (total_read > 1) {
		msg[1].len = len;

		err = i2c_transfer(client->adapter, msg, 2);
		if (err != 2)
			return (err < 0) ? err : -EIO;
	}

	msg[1].len = sizeof(*data);
	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	data->x = (gyro_data->fifo_data[1] << 8) | gyro_data->fifo_data[0];
	data->y = (gyro_data->fifo_data[3] << 8) | gyro_data->fifo_data[2];
	data->z = (gyro_data->fifo_data[5] << 8) | gyro_data->fifo_data[4];

	return 0;
}

static int k330_gyro_report_values\
	(struct k330_gyro_data *data)
{
	int res;

	res = k330_gyro_read_values(data->client,
		&data->xyz_data, data->entries);
	if (res < 0)
		return res;

	data->xyz_data.x -= data->cal_data.x;
	data->xyz_data.y -= data->cal_data.y;
	data->xyz_data.z -= data->cal_data.z;

	if (data->convert_axes)
		data->convert_axes(&data->xyz_data.x, &data->xyz_data.y,
			&data->xyz_data.z);

	input_report_rel(data->input_dev, REL_RX, data->xyz_data.x);
	input_report_rel(data->input_dev, REL_RY, data->xyz_data.y);
	input_report_rel(data->input_dev, REL_RZ, data->xyz_data.z);
	input_sync(data->input_dev);

	k330_gyro_restart_fifo(data);

#ifdef LOGGING_GYRO
	pr_info("%s, x = %d, y = %d, z = %d\n"
			, __func__, data->xyz_data.x, data->xyz_data.y,
			data->xyz_data.z);
#endif

	return res;
}

static enum hrtimer_restart k330_gyro_timer_func(struct hrtimer *timer)
{
	struct k330_gyro_data *gyro_data\
		= container_of(timer, struct k330_gyro_data, timer);
	queue_work(gyro_data->k330_gyro_wq, &gyro_data->work);
	hrtimer_forward_now(&gyro_data->timer, gyro_data->polling_delay);
	return HRTIMER_RESTART;
}

static void k330_gyro_work_func(struct work_struct *work)
{
	int res, retry = 3;
	struct k330_gyro_data *data\
		= container_of(work, struct k330_gyro_data, work);
	s32 status = 0;
	s16 x_temp, y_temp, z_temp;

	do {
		status = i2c_smbus_read_byte_data(data->client,
					STATUS_REG);
		if (status & 0x08)
			break;
	} while (retry--);

	if (status & 0x08 && data->self_test == false) {
		res = k330_gyro_read_values(data->client,
			&data->xyz_data, 0);
		if (res < 0)
			pr_err("%s, reading data fail(res = %d)\n",
				__func__, res);

		if (data->dps == DPS500) {
			x_temp = data->xyz_data.x - data->cal_data.x;
			y_temp = data->xyz_data.y - data->cal_data.y;
			z_temp = data->xyz_data.z - data->cal_data.z;

			if (!(x_temp >> 15 == data->xyz_data.x >> 15) &&\
				!(data->cal_data.x >> 15 == data->xyz_data.x >> 15)) {
				pr_debug("%s, gyro x is overflowed!\n", __func__);
				x_temp =
					(x_temp >= 0 ? K330_MIN_GYRO : K330_MAX_GYRO);
			}
			if (!(y_temp >> 15 == data->xyz_data.y >> 15) &&\
				!(data->cal_data.y >> 15 == data->xyz_data.y >> 15)) {
				pr_debug("%s, gyro y is overflowed!\n", __func__);
				y_temp =
					(y_temp >= 0 ? K330_MIN_GYRO : K330_MAX_GYRO);
			}
			if (!(z_temp >> 15 == data->xyz_data.z >> 15) &&\
				!(data->cal_data.z >> 15 == data->xyz_data.z >> 15)) {
				pr_debug("%s, gyro z is overflowed!\n", __func__);
				z_temp =
					(z_temp >= 0 ? K330_MIN_GYRO : K330_MAX_GYRO);
			}

			data->xyz_data.x = x_temp;
			data->xyz_data.y = y_temp;
			data->xyz_data.z = z_temp;
		}

	} else {
		pr_warn("%s, use last data(%d, %d, %d), status=%d, selftest=%d\n",
			__func__, data->xyz_data.x, data->xyz_data.y,
			data->xyz_data.z, status, data->self_test);
	}

	if (data->convert_axes)
		data->convert_axes(&data->xyz_data.x, &data->xyz_data.y,
			&data->xyz_data.z);

	input_report_rel(data->input_dev, REL_RX, data->xyz_data.x);
	input_report_rel(data->input_dev, REL_RY, data->xyz_data.y);
	input_report_rel(data->input_dev, REL_RZ, data->xyz_data.z);
	input_sync(data->input_dev);

#ifdef LOGGING_GYRO
	pr_info("%s, x = %d, y = %d, z = %d\n"
		, __func__, data->xyz_data.x, data->xyz_data.y,
		data->xyz_data.z);
#endif
}

static irqreturn_t k330_gyro_interrupt_thread(int irq\
	, void *k330_gyro_data_p)
{
	int res;
	struct k330_gyro_data *data = k330_gyro_data_p;

	if (unlikely(data->self_test)) {
		disable_irq_nosync(irq);
		complete(&data->data_ready);
		return IRQ_HANDLED;
	}
#ifdef LOGGING_GYRO
	data->count++;
#endif
	res = k330_gyro_report_values(data);
	if (res < 0)
		pr_err("%s: failed to report gyro values\n", __func__);

	return IRQ_HANDLED;
}

static ssize_t k330_gyro_selftest_dps_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->dps);
}

#ifdef DEBUG_REGISTER
static ssize_t register_data_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	struct i2c_msg msg[2];
	u8 reg1, reg2, reg3, reg4, reg5, fifo_ctrl, fifo_src;
	u8 reg_buf;
	int err;

	msg[0].addr = data->client->addr;
	msg[0].buf = &reg_buf;
	msg[0].flags = 0;
	msg[0].len = 1;

	reg_buf = CTRL_REG1;
	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &reg1;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = CTRL_REG2;
	msg[1].buf = &reg2;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = CTRL_REG3;
	msg[1].buf = &reg3;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = CTRL_REG4;
	msg[1].buf = &reg4;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = CTRL_REG5;
	msg[1].buf = &reg5;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = FIFO_CTRL_REG;
	msg[1].buf = &fifo_ctrl;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	reg_buf = FIFO_SRC_REG;
	msg[1].buf = &fifo_src;

	err = i2c_transfer(data->client->adapter, msg, 2);
	if (err != 2)
		return (err < 0) ? err : -EIO;

	return sprintf(buf, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n"\
		, reg1, reg2, reg3, reg4, reg5, fifo_ctrl, fifo_src);
}
#endif

static ssize_t k330_gyro_selftest_dps_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	u16 new_dps;
	int err;
	u8 ctrl;

	err = kstrtou16(buf, 10, &new_dps);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	/* check out dps validity */
	if (new_dps != DPS250 && new_dps != DPS500 && new_dps != DPS2000) {
		pr_err("%s: wrong dps(%d)\n", __func__, new_dps);
		return -1;
	}

	ctrl = (data->ctrl_regs[3] & ~FS_MASK);

	if (new_dps == DPS250)
		ctrl |= FS_250DPS;
	else if (new_dps == DPS500)
		ctrl |= FS_500DPS;
	else if (new_dps == DPS2000)
		ctrl |= FS_2000DPS;
	else
		ctrl |= FS_DEFULAT_DPS;

	/* apply new dps */
	mutex_lock(&data->lock);
	data->ctrl_regs[3] = ctrl;

	err = i2c_smbus_write_byte_data(data->client, CTRL_REG4, ctrl);
	if (err < 0) {
		pr_err("%s: updating dps failed\n", __func__);
		mutex_unlock(&data->lock);
		return err;
	}
	mutex_unlock(&data->lock);

	data->dps = new_dps;
	pr_info("%s: %d dps stored\n", __func__, data->dps);

	return count;
}

static ssize_t k330_gyro_show_enable(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data  = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->enable);
}

static ssize_t k330_gyro_set_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int err = 0;
	struct k330_gyro_data *data  = dev_get_drvdata(dev);
	bool new_enable;

	if (sysfs_streq(buf, "1"))
		new_enable = true;
	else if (sysfs_streq(buf, "0"))
		new_enable = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_info("%s, %d enable : %d.\n", __func__, __LINE__,\
		new_enable);

	if (new_enable == data->enable)
		return size;

	mutex_lock(&data->lock);
	if (new_enable) {
		/* load cal data */
		err = k330_gyro_open_calibration(data);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: k330_gyro_open_calibration() failed\n",
				__func__);
		/* turning on */
		err = i2c_smbus_write_i2c_block_data(data->client,
			CTRL_REG1 | AC, sizeof(data->ctrl_regs),
						data->ctrl_regs);
		if (err < 0) {
			err = -EIO;
			goto unlock;
		}

		if (data->gyro_en)
			*data->gyro_en = true;

		if (data->interruptible) {
			enable_irq(data->client->irq);

			/* reset fifo entries */
			err = k330_gyro_restart_fifo(data);
			if (err < 0) {
				err = -EIO;
				goto turn_off;
			}
		}

		if (!data->interruptible) {
			hrtimer_start(&data->timer,
				data->polling_delay, HRTIMER_MODE_REL);
		}
	} else {
		if (data->interruptible)
			disable_irq(data->client->irq);
		else {
			hrtimer_cancel(&data->timer);
			cancel_work_sync(&data->work);
		}
		/* turning off */
		err = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, 0x00);
		if (err < 0)
			goto unlock;
		if (data->gyro_en)
			*data->gyro_en = false;
	}
	data->enable = new_enable;

	pr_info("%s, %d k330_gyro enable is done.\n",\
		__func__, __LINE__);

turn_off:
	if (err < 0)
		i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, 0x00);
unlock:
	mutex_unlock(&data->lock);

	return err ? err : size;
}

static u64 k330_gyro_get_delay_ns(struct k330_gyro_data *k3)
{
	u64 delay;

	delay = k3->time_to_read * k3->entries;
	delay = ktime_to_ns(ns_to_ktime(delay));

	return delay;
}

static int k330_gyro_set_delay_ns(struct k330_gyro_data *k3\
	, u64 delay_ns)
{
	int res = 0;
	int odr_value = ODR95_BW25;
	int i = 0, odr_table_size = ARRAY_SIZE(odr_delay_table) - 1;
	u8 ctrl;
	u64 new_delay = 0;

	mutex_lock(&k3->lock);
	if (!k3->interruptible) {
		hrtimer_cancel(&k3->timer);
		cancel_work_sync(&k3->work);
	} else
		disable_irq(k3->client->irq);

	/* round to the nearest supported ODR that is equal or above than
	 * the requested value
	 * 10ms(entries = 1), 20ms(entries = 2),
	 * 67ms(entries = 6), 200ms(entries = 20)
	 */
	if (delay_ns == 10000000LL && k3->interruptible)
		delay_ns = odr_delay_table[odr_table_size].delay_ns;

	if (delay_ns >= MAX_DELAY) {/* > max delay */
		k3->entries = MAX_ENTRY;
		odr_value = odr_delay_table[odr_table_size].odr;
		k3->time_to_read = odr_delay_table[odr_table_size].delay_ns;
		new_delay = MAX_DELAY;
	} else if (delay_ns <= odr_delay_table[0].delay_ns) { /* < min delay */
		k3->entries = 1;
		odr_value = odr_delay_table[0].odr;
		k3->time_to_read = odr_delay_table[0].delay_ns;
		new_delay = odr_delay_table[0].delay_ns;
	} else {
		for (i = odr_table_size; i >= 0; i--) {
			if (delay_ns >= odr_delay_table[i].delay_ns) {
				new_delay = delay_ns;
				do_div(delay_ns, odr_delay_table[i].delay_ns);
				k3->entries = delay_ns;
				odr_value = odr_delay_table[i].odr;
				k3->time_to_read = odr_delay_table[i].delay_ns;
				break;
			}
		}
	}

	if (k3->interruptible)
		pr_info("%s, k3->entries=%d, odr_value=0x%x\n", __func__,
			k3->entries, odr_value);

	if (odr_value != (k3->ctrl_regs[0] & ODR_MASK)) {
		ctrl = (k3->ctrl_regs[0] & ~ODR_MASK);
		ctrl |= odr_value;
		k3->ctrl_regs[0] = ctrl;
		res = i2c_smbus_write_byte_data(k3->client, CTRL_REG1, ctrl);
	}

	if (k3->interruptible) {
		enable_irq(k3->client->irq);

		/* (re)start fifo */
		k330_gyro_restart_fifo(k3);
	}

	if (!k3->interruptible) {
		pr_info("%s, delay_ns=%lld, odr_value=0x%x\n",
					__func__, new_delay, odr_value);
		k3->polling_delay = ns_to_ktime(new_delay);
		if (k3->enable)
			hrtimer_start(&k3->timer,
				k3->polling_delay, HRTIMER_MODE_REL);
	}

	mutex_unlock(&k3->lock);

	return res;
}

static ssize_t k330_gyro_show_delay(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data  = dev_get_drvdata(dev);
	u64 delay;

	if (!data->interruptible)
		return sprintf(buf, "%lld\n", ktime_to_ns(data->polling_delay));

	delay = k330_gyro_get_delay_ns(data);

	return sprintf(buf, "%lld\n", delay);
}

static ssize_t k330_gyro_set_delay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int res;
	struct k330_gyro_data *data  = dev_get_drvdata(dev);
	u64 delay_ns;

	res = strict_strtoll(buf, 10, &delay_ns);
	if (res < 0)
		return res;

	k330_gyro_set_delay_ns(data, delay_ns);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
			k330_gyro_show_enable, k330_gyro_set_enable);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			k330_gyro_show_delay, k330_gyro_set_delay);

/*************************************************************************/
/* k330_gyro Sysfs															 */
/*************************************************************************/

/* Device Initialization  */
static int device_init(struct k330_gyro_data *data)
{
	int err;
	u8 buf[5];

	buf[0] = 0x6f;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	err = i2c_smbus_write_i2c_block_data(data->client,
					CTRL_REG1 | AC, sizeof(buf), buf);
	if (err < 0)
		pr_err("%s: CTRL_REGs i2c writing failed\n", __func__);

	return err;
}
static ssize_t k330_gyro_power_off(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	int err;

	err = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, 0x00);

	pr_info("[%s] result of power off = %d\n", __func__, err);

	return sprintf(buf, "%d\n", (err < 0 ? 0 : 1));
}

static ssize_t k330_gyro_power_on(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	int err;

	err = device_init(data);
	if (err < 0) {
		pr_err("%s: device_init() failed\n", __func__);
		return 0;
	}

	pr_info("[%s] result of device init = %d\n", __func__, err);

	return sprintf(buf, "%d\n", 1);
}

static ssize_t k330_gyro_get_temp(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	char temp;

	temp = i2c_smbus_read_byte_data(data->client, OUT_TEMP);
	if (temp < 0) {
		pr_err("%s: STATUS_REGS i2c reading failed\n", __func__);
		return 0;
	}

	pr_info("[%s] read temperature : %d\n", __func__, temp);

	return sprintf(buf, "%d\n", temp);
}

static int k330_gyro_fifo_self_test(struct k330_gyro_data *data,\
	u8 *cal_pass, s16 *zero_rate_data)
{
	struct gyro_t raw_data;
	int err;
	int i, j;
	s16 raw[3] = {0,}, zero_rate_delta[3] = {0,};
	int sum_raw[3] = {0,};
	bool zero_rate_read_2nd = false;
	u8 reg[5];
	u8 fifo_pass = 2;
	u8 status_reg;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	/* fifo mode, enable interrupt, 500dps */
	reg[0] = 0x6F;
	reg[1] = 0x00;
	reg[2] = 0x04;
	reg[3] = 0x90;
	reg[4] = 0x40;

	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_i2c_block_data(data->client,
			CTRL_REG1 | AC,	sizeof(reg), reg);
		if (err >= 0)
			break;
	}
	if (err < 0) {
		pr_err("%s: CTRL_REGs i2c writing failed\n", __func__);
		goto exit;
	}

	/* Power up, wait for 800ms for stable output */
	msleep(800);

read_zero_rate_again:
	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_byte_data(data->client,
				FIFO_CTRL_REG, BYPASS_MODE);
		if (err >= 0)
			break;
	}
	if (err < 0) {
		pr_err("%s : failed to set bypass_mode\n", __func__);
		goto exit;
	}

	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_byte_data(data->client,
				FIFO_CTRL_REG, FIFO_MODE | FIFO_TEST_WTM);
		if (err >= 0)
			break;
	}
	if (err < 0) {
		pr_err("%s: failed to set fifo_mode\n", __func__);
		goto exit;
	}

	/* if interrupt mode */
	if (!data->enable && data->interruptible) {
		enable_irq(data->client->irq);
		err = wait_for_completion_timeout(&data->data_ready, 5*HZ);
		msleep(200);
		if (err <= 0) {
			disable_irq(data->client->irq);
			if (!err)
				pr_err("%s: wait timed out\n", __func__);
			goto exit;
		}
	/* if polling mode */
	} else
		msleep(200);

	/* check out watermark status */
	status_reg = i2c_smbus_read_byte_data(data->client, FIFO_SRC_REG);
	if (!(status_reg & 0x80)) {
		pr_err("%s: Watermark level is not enough(0x%x)\n",
			__func__, status_reg);
		goto exit;
	}

	/* read fifo entries */
	err = k330_gyro_read_values(data->client,
				&raw_data, FIFO_TEST_WTM + 2);
	if (err < 0) {
		pr_err("%s: k330_gyro_read_values() failed\n", __func__);
		goto exit;
	}

	/* print out fifo data */
	pr_info("[gyro_self_test] fifo data\n");
	for (i = 0; i < sizeof(raw_data) * (FIFO_TEST_WTM + 1);
		i += sizeof(raw_data)) {
		raw[0] = (data->fifo_data[i+1] << 8)
				| data->fifo_data[i];
		raw[1] = (data->fifo_data[i+3] << 8)
				| data->fifo_data[i+2];
		raw[2] = (data->fifo_data[i+5] << 8)
				| data->fifo_data[i+4];
		pr_info("%2dth: %8d %8d %8d\n", i/6, raw[0], raw[1], raw[2]);

		/* for calibration of gyro sensor data */
		sum_raw[0] += raw[0];
		sum_raw[1] += raw[1];
		sum_raw[2] += raw[2];

		for (j = 0; j < 3; j++) {
			if (raw[j] < MIN_ZERO_RATE || raw[j] > MAX_ZERO_RATE) {
				pr_err("%s: %dth data(%d) is out of zero-rate",
					__func__, i/6, raw[j]);
				pr_err("%s: fifo test failed\n", __func__);
				fifo_pass = 0;
				*cal_pass = 0;
				goto exit;
			}
		}
	}

	/* zero_rate_data */
	zero_rate_data[0] = raw[0] * 175 / 10000;
	zero_rate_data[1] = raw[1] * 175 / 10000;
	zero_rate_data[2] = raw[2] * 175 / 10000;

	if (zero_rate_read_2nd == true) {
		/* check zero_rate second time */
		zero_rate_delta[0] -= zero_rate_data[0];
		zero_rate_delta[1] -= zero_rate_data[1];
		zero_rate_delta[2] -= zero_rate_data[2];
		pr_info("[gyro_self_test] zero rate second: %8d %8d %8d\n",
			zero_rate_data[0], zero_rate_data[1],
			zero_rate_data[2]);
		pr_info("[gyro_self_test] zero rate delta: %8d %8d %8d\n",
			zero_rate_delta[0], zero_rate_delta[1],
			zero_rate_delta[2]);

		if ((-5 < zero_rate_delta[0] && zero_rate_delta[0] < 5) &&
			(-5 < zero_rate_delta[1] && zero_rate_delta[1] < 5) &&
			(-5 < zero_rate_delta[2] && zero_rate_delta[2] < 5)) {
			/* calibration of gyro sensor data */
			data->cal_data.x = sum_raw[0]/(FIFO_TEST_WTM + 1);
			data->cal_data.y = sum_raw[1]/(FIFO_TEST_WTM + 1);
			data->cal_data.z = sum_raw[2]/(FIFO_TEST_WTM + 1);
			pr_info("%s: cal data (%d,%d,%d)\n", __func__,
				data->cal_data.x, data->cal_data.y,
				data->cal_data.z);

			/* save cal data */
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			cal_filp = filp_open(CALIBRATION_FILE_PATH,
					O_CREAT | O_TRUNC | O_WRONLY, 0666);
			if (IS_ERR(cal_filp)) {
				pr_err("%s: Can't open calibration file\n",
					__func__);
				set_fs(old_fs);
				err = PTR_ERR(cal_filp);
				return err;
			}

			err = cal_filp->f_op->write(cal_filp,
				(char *)&data->cal_data, 3 * sizeof(s16),
				&cal_filp->f_pos);
			if (err != 3 * sizeof(s16)) {
				pr_err("%s: Can't write the cal data to file\n",
					__func__);
			}

			filp_close(cal_filp, current->files);
			set_fs(old_fs);

			*cal_pass = 1;
		} /* else calibration is failed */
	} else { /* check zero_rate first time, go to check again */
		zero_rate_read_2nd = true;
		sum_raw[0] = 0;
		sum_raw[1] = 0;
		sum_raw[2] = 0;
		zero_rate_delta[0] = zero_rate_data[0];
		zero_rate_delta[1] = zero_rate_data[1];
		zero_rate_delta[2] = zero_rate_data[2];
		pr_info("[gyro_self_test] zero rate first: %8d %8d %8d\n",
			zero_rate_data[0], zero_rate_data[1],
			zero_rate_data[2]);
		goto read_zero_rate_again;
	}

	fifo_pass = 1;
exit:
	/* 1: success, 0: fail, 2: retry */
	return fifo_pass;
}

static int k330_gyro_bypass_self_test\
	(struct k330_gyro_data *gyro_data, int NOST[3], int ST[3])
{
	int differ_x = 0, differ_y = 0, differ_z = 0;
	int err, i, j, sample_num = 6;
	s16 raw[3] = {0,};
	s32 temp = 0;
	u8 data[6] = {0,};
	u8 reg[5];
	u8 bZYXDA = 0;
	u8 bypass_pass = 2;

	/* Initialize Sensor, turn on sensor, enable P/R/Y */
	/* Set BDU=1, Set ODR=200Hz, Cut-Off Frequency=50Hz, FS=2000dps */
	reg[0] = 0x6f;
	reg[1] = 0x00;
	reg[2] = 0x00;
	reg[3] = 0xA0;
	reg[4] = 0x02;

	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_i2c_block_data(gyro_data->client,
			CTRL_REG1 | AC,	sizeof(reg), reg);
		if (err >= 0)
			break;
	}
	if (err < 0) {
		pr_err("%s: CTRL_REGs i2c writing failed\n", __func__);
		goto exit;
	}

	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_byte_data(gyro_data->client,
				FIFO_CTRL_REG, BYPASS_MODE);
		if (err >= 0)
			break;
	}
	if (err < 0) {
		pr_err("%s : failed to set bypass_mode\n", __func__);
		goto exit;
	}

	/* Power up, wait for 800ms for stable output */
	msleep(800);

	/* Read 5 samples output before self-test on */
	for (i = 0; i < sample_num; i++) {
		/* check ZYXDA ready bit */
		for (j = 0; j < 10; j++) {
			temp = i2c_smbus_read_byte_data(gyro_data->client,
							STATUS_REG);
			if (temp >= 0) {
				bZYXDA = temp & 0x08;
				if (!bZYXDA) {
					usleep_range(10000, 20000);
					pr_err("%s: %d,%d: no_data_ready, (0x%x)",
							__func__, i, j, temp);
					continue;
				} else
					break;
			}
		}
		if (temp < 0) {
			pr_err("%s: STATUS_REGS i2c reading failed\n",
								__func__);
			goto exit;
		}

		for (j = 0; j < 6; j++) {
			data[j] = i2c_smbus_read_byte_data(gyro_data->client,
							AXISDATA_REG+j);
		}
		if (i == 0)
			continue;

		raw[0] = (data[1] << 8) | data[0];
		raw[1] = (data[3] << 8) | data[2];
		raw[2] = (data[5] << 8) | data[4];

		NOST[0] += raw[0];
		NOST[1] += raw[1];
		NOST[2] += raw[2];

		pr_info("[gyro_self_test] raw[0] = %d\n", raw[0]);
		pr_info("[gyro_self_test] raw[1] = %d\n", raw[1]);
		pr_info("[gyro_self_test] raw[2] = %d\n\n", raw[2]);
	}

	for (i = 0; i < 3; i++)
		pr_info("[gyro_self_test] "
			"SUM of NOST[%d] = %d\n", i, NOST[i]);

	/* calculate average of NOST and covert from ADC to DPS */
	for (i = 0; i < 3; i++) {
		NOST[i] = (NOST[i] / 5) * 70 / 1000;
		pr_info("[gyro_self_test] "
			"AVG of NOST[%d] = %d\n", i, NOST[i]);
	}
	pr_info("\n");

	/* Enable Self Test */
	reg[0] = 0xA2;
	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_byte_data(gyro_data->client,
						CTRL_REG4, reg[0]);
		if (err >= 0)
			break;
	}
	if (temp < 0) {
		pr_err("%s: CTRL_REG4 i2c writing failed\n", __func__);
		goto exit;
	}

	msleep(100);

	/* Read 5 samples output after self-test on */
	/* The first data is useless on the k330 selftest. */
	for (i = 0; i < sample_num; i++) {
		/* check ZYXDA ready bit */
		for (j = 0; j < 10; j++) {
			temp = i2c_smbus_read_byte_data(gyro_data->client,
							STATUS_REG);
			if (temp >= 0) {
				bZYXDA = temp & 0x08;
				if (!bZYXDA) {
					usleep_range(10000, 20000);
					pr_err("%s: %d,%d: no_data_ready, (0x%x)",
							__func__, i, j, temp);
					continue;
				} else
					break;
			}

		}
		if (temp < 0) {
			pr_err("%s: STATUS_REGS i2c reading failed\n",
								__func__);
			goto exit;
		}

		for (j = 0; j < 6; j++) {
			data[j] = i2c_smbus_read_byte_data(gyro_data->client,
							AXISDATA_REG+j);
		}
		if (i == 0)
			continue;

		raw[0] = (data[1] << 8) | data[0];
		raw[1] = (data[3] << 8) | data[2];
		raw[2] = (data[5] << 8) | data[4];

		ST[0] += raw[0];
		ST[1] += raw[1];
		ST[2] += raw[2];

		pr_info("[gyro_self_test] raw[0] = %d\n", raw[0]);
		pr_info("[gyro_self_test] raw[1] = %d\n", raw[1]);
		pr_info("[gyro_self_test] raw[2] = %d\n\n", raw[2]);
	}

	for (i = 0; i < 3; i++)
		pr_info("[gyro_self_test] "
			"SUM of ST[%d] = %d\n", i, ST[i]);

	/* calculate average of ST and convert from ADC to dps */
	for (i = 0; i < 3; i++)	{
		/* When FS=2000, 70 mdps/digit */
		ST[i] = (ST[i] / 5) * 70 / 1000;
		pr_info("[gyro_self_test] "
			"AVG of ST[%d] = %d\n", i, ST[i]);
	}

	/* check whether pass or not */
	if (ST[0] >= NOST[0]) /* for x */
		differ_x = ST[0] - NOST[0];
	else
		differ_x = NOST[0] - ST[0];

	if (ST[1] >= NOST[1]) /* for y */
		differ_y = ST[1] - NOST[1];
	else
		differ_y = NOST[1] - ST[1];

	if (ST[2] >= NOST[2]) /* for z */
		differ_z = ST[2] - NOST[2];
	else
		differ_z = NOST[2] - ST[2];

	pr_info("[gyro_self_test] differ x:%d, y:%d, z:%d\n",
						differ_x, differ_y, differ_z);

	if ((MIN_ST <= differ_x && differ_x <= MAX_ST)
		&& (MIN_ST <= differ_y && differ_y <= MAX_ST)
		&& (MIN_ST <= differ_z && differ_z <= MAX_ST))
		bypass_pass = 1;
	else
		bypass_pass = 0;

exit:
	return bypass_pass;
}

static ssize_t k330_gyro_self_test(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	int NOST[3] = {0,}, ST[3] = {0,};
	int err;
	int i;
	u8 backup_regs[5] = {0,};
	u8 fifo_pass = 2;
	u8 bypass_pass = 2;
	u8 cal_pass = 0;
	s16 zero_rate_data[3] = {0,};

	data->self_test = true;
	/* before starting self-test, backup register */
	for (i = 0; i < 10; i++) {
		err = i2c_smbus_read_i2c_block_data(data->client,
			CTRL_REG1 | AC,	sizeof(backup_regs), backup_regs);
		if (err == sizeof(backup_regs))
			break;
	}
	if (err < 0) {
		pr_err("%s: CTRL_REGs i2c reading failed\n", __func__);
		goto exit;
	}

	for (i = 0; i < 5; i++)
		pr_info("[gyro_self_test] "
			"backup reg[%d] = %2x\n", i, backup_regs[i]);

	/* fifo self test & gyro calibration */
	pr_info("\n[gyro_self_test] fifo self-test\n");

	fifo_pass = k330_gyro_fifo_self_test(data,
		&cal_pass, zero_rate_data);

	/* fifo self test result */
	if (fifo_pass == 1)
		pr_info("[gyro_self_test] fifo self-test success\n");
	else if (!fifo_pass)
		pr_info("[gyro_self_test] fifo self-test fail\n");
	else
		pr_info("[gyro_self_test] fifo self-test retry\n");

	/* calibration result */
	if (cal_pass == 1)
		pr_info("[gyro_self_test] calibration success\n");
	else if (!cal_pass)
		pr_info("[gyro_self_test] calibration fail\n");

	/* bypass self test */
	pr_info("\n[gyro_self_test] bypass self-test\n");

	bypass_pass = k330_gyro_bypass_self_test(data, NOST, ST);
	if (bypass_pass == 1)
		pr_info("[gyro_self_test] bypass self-test success\n\n");
	else if (!bypass_pass)
		pr_info("[gyro_self_test] bypass self-test fail\n\n");
	else
		pr_info("[gyro_self_test] bypass self-test retry\n\n");

	/* restore backup register */
	for (i = 0; i < 10; i++) {
		err = i2c_smbus_write_i2c_block_data(data->client,
			CTRL_REG1 | AC, sizeof(backup_regs),
			backup_regs);
		if (err >= 0)
			break;
	}
	if (err < 0)
		pr_err("%s: CTRL_REGs i2c writing failed\n", __func__);

exit:
	data->self_test = false;
	if (!data->enable) {
		/* If gyro is not enabled, make it go to the power down mode. */
		err = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, 0x00);
		if (err < 0)
			pr_err("%s: CTRL_REGs i2c writing failed\n", __func__);
	}

	if (fifo_pass == 2 && bypass_pass == 2)
		pr_info("[gyro_self_test] self-test result : retry\n");
	else
		pr_info("[gyro_self_test] self-test result : %s\n",
			fifo_pass & bypass_pass & cal_pass ? "pass" : "fail");

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		NOST[0], NOST[1], NOST[2], ST[0], ST[1], ST[2],
		zero_rate_data[0], zero_rate_data[1], zero_rate_data[2],
		fifo_pass & bypass_pass & cal_pass, fifo_pass, cal_pass);
}

static ssize_t k330_gyro_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t k330_gyro_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t
k330_gyro_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->position);
}

static ssize_t
k330_gyro_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct k330_gyro_data *data = dev_get_drvdata(dev);
	int err;

	err = kstrtou8(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	if (data->select_func)
		data->convert_axes = data->select_func(data->position);

	return count;
}

static DEVICE_ATTR(power_off, S_IRUGO,
	k330_gyro_power_off, NULL);
static DEVICE_ATTR(power_on, S_IRUGO,
	k330_gyro_power_on, NULL);
static DEVICE_ATTR(temperature, S_IRUGO,
	k330_gyro_get_temp, NULL);
static DEVICE_ATTR(selftest, S_IRUGO,
	k330_gyro_self_test, NULL);
static DEVICE_ATTR(selftest_dps,  S_IRUGO | S_IRWXU | S_IRWXG,
	k330_gyro_selftest_dps_show, k330_gyro_selftest_dps_store);
static DEVICE_ATTR(vendor, S_IRUGO,
	k330_gyro_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO,
	k330_gyro_name_show, NULL);
static DEVICE_ATTR(position,  S_IRUGO | S_IRWXU | S_IRWXG,
	k330_gyro_position_show, k330_gyro_position_store);
#ifdef DEBUG_REGISTER
static DEVICE_ATTR(reg_data, S_IRUGO,
	register_data_show, NULL);
#endif

/*************************************************************************/
/* End of k330_gyro Sysfs							 */
/*************************************************************************/
static int k330_gyro_probe(struct i2c_client *client,
			       const struct i2c_device_id *devid)
{
	int ret, retry = 0;
	int err = 0;
	struct k330_gyro_data *data;
	struct input_dev *input_dev;
	struct gyro_platform_data *pdata;

	pr_info("%s, started", __func__);
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	data->client = client;
	data->dps = DPS500;
	/* read chip id */
	do {
		ret = i2c_smbus_read_byte_data(client, WHO_AM_I);
		if (ret == DEVICE_ID)
			break;
		usleep_range(10000, 10000);
		retry++;
		pr_err("%s, ret=%d, retry=%d\n", __func__, ret, retry);
	} while (ret != DEVICE_ID && retry < 3);

	if (ret != DEVICE_ID) {
		if (ret < 0) {
			pr_err("%s: i2c for reading chip id failed\n",
				__func__);
			err = ret;
		} else {
			pr_err("%s : Device identification failed(%d)\n",
				__func__, ret);
			err = -ENODEV;
		}
		goto err_read_reg;
	}

	mutex_init(&data->lock);
	atomic_set(&data->opened, 0);
	init_completion(&data->data_ready);

	/* allocate gyro input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device;
	}

	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = K330_GYR_INPUT_NAME;
	/* X */
	input_set_capability(input_dev, EV_REL, REL_RX);
	/* Y */
	input_set_capability(input_dev, EV_REL, REL_RY);
	/* Z */
	input_set_capability(input_dev, EV_REL, REL_RZ);

	err = input_register_device(input_dev);
	if (err < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(data->input_dev);
		goto err_input_register_device;
	}

	i2c_set_clientdata(client, data);
	dev_set_drvdata(&input_dev->dev, data);

	if (data->client->irq >= 0) { /* interrupt */
		memcpy(&data->ctrl_regs, &default_ctrl_regs_fifo,
			sizeof(default_ctrl_regs_fifo));
		data->interruptible = true;
		data->entries = 1;
		err = request_threaded_irq(data->client->irq, NULL,
			k330_gyro_interrupt_thread\
			, IRQF_TRIGGER_RISING | IRQF_ONESHOT,\
				"k330_gyro", data);
		if (err < 0) {
			pr_err("%s: can't allocate irq.\n", __func__);
			goto err_request_irq;
		}
		disable_irq(data->client->irq);
	} else { /* polling */
		memcpy(&data->ctrl_regs, &default_ctrl_regs_bypass,
			sizeof(default_ctrl_regs_bypass));
		data->ctrl_regs[2] = 0x00; /* disable interrupt */
		/* hrtimer settings.  we poll for gyro values using a timer. */
		hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		data->polling_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
		data->timer.function = k330_gyro_timer_func;

		/* the timer just fires off a work queue request.
		   We need a thread to read i2c (can be slow and blocking). */
		data->k330_gyro_wq \
		= create_singlethread_workqueue("k330_gyro_wq");
		if (!data->k330_gyro_wq) {
			err = -ENOMEM;
			pr_err("%s: could not create workqueue\n", __func__);
			goto err_create_workqueue;
		}
		/* this is the thread function we run on the work queue */
		INIT_WORK(&data->work, k330_gyro_work_func);
	}

	if (device_create_file(&input_dev->dev,
				&dev_attr_enable) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
				dev_attr_enable.attr.name);
		goto err_device_create_file;
	}

	if (device_create_file(&input_dev->dev,
				&dev_attr_poll_delay) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
				dev_attr_poll_delay.attr.name);
		goto err_device_create_file2;
	}

	/* create device node for k330_gyro digital gyroscope */
	data->dev = sensors_classdev_register("gyro_sensor");

	if (IS_ERR(data->dev)) {
		pr_err("%s: Failed to create device(gyro)\n", __func__);
		err = PTR_ERR(data->dev);
		goto err_device_create;
	}

	if (device_create_file(data->dev, &dev_attr_power_on) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_power_on.attr.name);
		goto err_device_create_file3;
	}

	if (device_create_file(data->dev, &dev_attr_power_off) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_power_off.attr.name);
		goto err_device_create_file4;
	}

	if (device_create_file(data->dev, &dev_attr_temperature) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_temperature.attr.name);
		goto err_device_create_file5;
	}

	if (device_create_file(data->dev, &dev_attr_selftest) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_selftest.attr.name);
		goto err_device_create_file6;
	}

	if (device_create_file(data->dev, &dev_attr_selftest_dps) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_selftest_dps.attr.name);
		goto err_device_create_file7;
	}

	if (device_create_file(data->dev, &dev_attr_vendor) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_vendor.attr.name);
		goto err_device_create_file8;
	}

	if (device_create_file(data->dev, &dev_attr_name) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_name.attr.name);
		goto err_device_create_file9;
	}

	/* set mounting position of the sensor */
	pdata = client->dev.platform_data;

	if (pdata) {
		data->gyro_en = pdata->gyro_en;
		*data->gyro_en = false;
	}

	if (pdata && pdata->select_func) {
		data->select_func = pdata->select_func;
		data->position = pdata->gyro_get_position();
		data->convert_axes = pdata->select_func(data->position);
		pr_info("%s, position = %d\n", __func__, data->position);
	}

	err = device_create_file(data->dev, &dev_attr_position);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
			__func__, dev_attr_position.attr.name);
		goto err_device_create_file10;
	}

#ifdef DEBUG_REGISTER
	if (device_create_file(data->dev, &dev_attr_reg_data) < 0) {
		pr_err("%s: Failed to create device file(%s)!\n", __func__,
			dev_attr_reg_data.attr.name);
		goto err_device_create_file11;
	}
#endif

	dev_set_drvdata(data->dev, data);
	pr_info("%s, successful", __func__);

	return 0;

#ifdef DEBUG_REGISTER
err_device_create_file11:
	device_remove_file(data->dev, &dev_attr_position);
#endif
err_device_create_file10:
	device_remove_file(data->dev, &dev_attr_name);
err_device_create_file9:
	device_remove_file(data->dev, &dev_attr_vendor);
err_device_create_file8:
	device_remove_file(data->dev, &dev_attr_selftest_dps);
err_device_create_file7:
	device_remove_file(data->dev, &dev_attr_selftest);
err_device_create_file6:
	device_remove_file(data->dev, &dev_attr_temperature);
err_device_create_file5:
	device_remove_file(data->dev, &dev_attr_power_off);
err_device_create_file4:
	device_remove_file(data->dev, &dev_attr_power_on);
err_device_create_file3:
	sensors_classdev_unregister(data->dev);
err_device_create:
	device_remove_file(&input_dev->dev, &dev_attr_poll_delay);
err_device_create_file2:
	device_remove_file(&input_dev->dev, &dev_attr_enable);
err_device_create_file:
	if (data->interruptible)
		free_irq(data->client->irq, data);
	else
		destroy_workqueue(data->k330_gyro_wq);
err_create_workqueue:
err_request_irq:
	input_unregister_device(data->input_dev);
err_input_register_device:
err_input_allocate_device:
	mutex_destroy(&data->lock);
err_read_reg:
	pr_err("%s, err = %d\n", __func__, err);
	kfree(data);
exit:
	return err;
}

static int k330_gyro_remove(struct i2c_client *client)
{
	int err = 0;
	struct k330_gyro_data *data = i2c_get_clientdata(client);

	device_remove_file(data->dev, &dev_attr_position);
	device_remove_file(data->dev, &dev_attr_vendor);
	device_remove_file(data->dev, &dev_attr_name);
	device_remove_file(data->dev, &dev_attr_selftest_dps);
	device_remove_file(data->dev, &dev_attr_selftest);
	device_remove_file(data->dev, &dev_attr_temperature);
	device_remove_file(data->dev, &dev_attr_power_on);
	device_remove_file(data->dev, &dev_attr_power_off);
#ifdef DEBUG_REGISTER
	device_remove_file(data->dev, &dev_attr_reg_data);
#endif
	sensors_classdev_unregister(data->dev);

	if (data->interruptible) {
		if (data->enable)
			disable_irq(data->client->irq);
		free_irq(data->client->irq, data);
	} else {
		hrtimer_cancel(&data->timer);
	    cancel_work_sync(&data->work);
		destroy_workqueue(data->k330_gyro_wq);
	}

	if (data->enable)
		err = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, 0x00);

	device_remove_file(&data->input_dev->dev, &dev_attr_enable);
	device_remove_file(&data->input_dev->dev, &dev_attr_poll_delay);
	input_unregister_device(data->input_dev);
	mutex_destroy(&data->lock);
	kfree(data);

	return err;
}

static int k330_gyro_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct k330_gyro_data *data = i2c_get_clientdata(client);

	if (data->enable) {
		mutex_lock(&data->lock);
		if (!data->interruptible) {
			hrtimer_cancel(&data->timer);
			cancel_work_sync(&data->work);
		} else
			disable_irq(data->client->irq);

		err = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, 0x00);
		mutex_unlock(&data->lock);
	}

	return err;
}

static int k330_gyro_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct k330_gyro_data *data = i2c_get_clientdata(client);

	if (data->enable) {
		mutex_lock(&data->lock);
		err = i2c_smbus_write_i2c_block_data(client,
				CTRL_REG1 | AC, sizeof(data->ctrl_regs),
							data->ctrl_regs);

		if (data->interruptible) {
			enable_irq(data->client->irq);

			k330_gyro_restart_fifo(data);
		}

		if (!data->interruptible)
			hrtimer_start(&data->timer,
				data->polling_delay, HRTIMER_MODE_REL);

		mutex_unlock(&data->lock);
	}

	return err;
}

static const struct dev_pm_ops k330_gyro_pm_ops = {
	.suspend = k330_gyro_suspend,
	.resume = k330_gyro_resume
};

static const struct i2c_device_id k330_gyro_id[] = {
	{ K330_GYRO_DEV_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, k330_gyro_id);

static struct i2c_driver k330_gyro_driver = {
	.probe = k330_gyro_probe,
	.remove = __devexit_p(k330_gyro_remove),
	.id_table = k330_gyro_id,
	.driver = {
		.pm = &k330_gyro_pm_ops,
		.owner = THIS_MODULE,
		.name = K330_GYRO_DEV_NAME,
	},
};

static int __init k330_gyro_init(void)
{
	return i2c_add_driver(&k330_gyro_driver);
}

static void __exit k330_gyro_exit(void)
{
	i2c_del_driver(&k330_gyro_driver);
}

module_init(k330_gyro_init);
module_exit(k330_gyro_exit);

MODULE_DESCRIPTION("K330 digital gyroscope driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
