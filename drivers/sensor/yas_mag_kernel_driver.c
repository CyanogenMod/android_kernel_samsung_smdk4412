/*
 * Copyright (c) 2010-2011 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#define __LINUX_KERNEL_DRIVER__
#include <linux/sensor/yas.h>
#include <linux/sensor/sensors_core.h>
#include "yas_mag_driver.c"

#define SYSFS_PCBTEST
#ifdef SYSFS_PCBTEST
#include "yas_pcb_test.h"
#include "yas_pcb_test.c"
#endif

#define GEOMAGNETIC_I2C_DEVICE_NAME         "yas532"
#define GEOMAGNETIC_INPUT_NAME              "geomagnetic"
#define GEOMAGNETIC_INPUT_RAW_NAME          "geomagnetic_raw"
#undef GEOMAGNETIC_PLATFORM_API

#define ABS_STATUS                          (ABS_BRAKE)
#define ABS_WAKE                            (ABS_MISC)

#define ABS_RAW_DISTORTION                  (ABS_THROTTLE)
#define ABS_RAW_THRESHOLD                   (ABS_RUDDER)
#define ABS_RAW_SHAPE                       (ABS_WHEEL)
#define ABS_RAW_MODE                        (ABS_HAT0X)
#define ABS_RAW_REPORT                      (ABS_GAS)

struct geomagnetic_data {
	struct input_dev *input_data;
	struct input_dev *input_raw;
	struct delayed_work work;
	struct semaphore driver_lock;
	struct semaphore multi_lock;
	atomic_t last_data[3];
	atomic_t last_status;
	atomic_t enable;
	int filter_enable;
	int filter_len;
	int32_t filter_noise[3];
	int32_t filter_threshold;
	int delay;
	int32_t threshold;
	int32_t distortion[3];
	int32_t shape;
	int32_t ellipsoid_mode;
	struct yas_mag_offset driver_offset;
#if DEBUG
	int suspend;
#endif
#ifdef YAS_MAG_MANUAL_OFFSET
	struct yas_vector manual_offset;
#endif
	struct yas_matrix static_matrix;
	struct yas_matrix dynamic_matrix;
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct list_head devfile_list;
	struct list_head raw_devfile_list;
#endif
	struct device *magnetic_sensor_device;
	struct mag_platform_data *mag_pdata;
	uint8_t dev_id;
	int noise_test_init;
};

static struct i2c_client *this_client;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE

#include <linux/miscdevice.h>
#define MAX_COUNT (64)
#define SENSOR_NAME "geomagnetic"
#define SENSOR_RAW_NAME "geomagnetic_raw"

struct sensor_device {
	struct list_head list;
	struct mutex lock;
	wait_queue_head_t waitq;
	struct input_event events[MAX_COUNT];
	int head, num_event;
};

static void get_time_stamp(struct timeval *tv)
{
	struct timespec ts;
	ktime_get_ts(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / 1000;
}

static void make_event(struct input_event *ev, int type, int code, int value)
{
	struct timeval tv;
	get_time_stamp(&tv);
	ev->type = type;
	ev->code = code;
	ev->value = value;
	ev->time = tv;
}

static void
make_event_w_time(struct input_event *ev, int type, int code, int value,
		  struct timeval *tv)
{
	ev->type = type;
	ev->code = code;
	ev->value = value;
	ev->time = *tv;
}

static void sensor_enq(struct sensor_device *kdev, struct input_event *ev)
{
	int idx;

	idx = kdev->head + kdev->num_event;
	if (MAX_COUNT <= idx)
		idx -= MAX_COUNT;
	kdev->events[idx] = *ev;
	kdev->num_event++;
	if (MAX_COUNT < kdev->num_event) {
		kdev->num_event = MAX_COUNT;
		kdev->head++;
		if (MAX_COUNT <= kdev->head)
			kdev->head -= MAX_COUNT;
	}
}

static int sensor_deq(struct sensor_device *kdev, struct input_event *ev)
{
	if (kdev->num_event == 0)
		return 0;

	*ev = kdev->events[kdev->head];
	kdev->num_event--;
	kdev->head++;
	if (MAX_COUNT <= kdev->head)
		kdev->head -= MAX_COUNT;
	return 1;
}

static void
sensor_event(struct list_head *devlist, struct input_event *ev, int num)
{
	struct sensor_device *kdev;
	int i;

	list_for_each_entry(kdev, devlist, list) {
		mutex_lock(&kdev->lock);
		for (i = 0; i < num; i++)
			sensor_enq(kdev, &ev[i]);
		mutex_unlock(&kdev->lock);
		wake_up_interruptible(&kdev->waitq);
	}
}

static ssize_t
sensor_write(struct file *f, const char __user *buf, size_t count,
	     loff_t *pos)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	struct sensor_device *kdev;
	struct input_event ev[MAX_COUNT];
	int num, i;

	if (count < sizeof(struct input_event))
		return -EINVAL;
	num = count / sizeof(struct input_event);
	if (MAX_COUNT < num)
		num = MAX_COUNT;

	if (copy_from_user(ev, buf, num * sizeof(struct input_event)))
		return -EFAULT;

	list_for_each_entry(kdev, &data->devfile_list, list) {
		mutex_lock(&kdev->lock);
		for (i = 0; i < num; i++)
			sensor_enq(kdev, &ev[i]);
		mutex_unlock(&kdev->lock);
		wake_up_interruptible(&kdev->waitq);
	}

	return count;
}

static ssize_t
sensor_read(struct file *f, char __user *buf, size_t count, loff_t *pos)
{
	struct sensor_device *kdev = f->private_data;
	int rt, num;
	struct input_event ev[MAX_COUNT];

	if (count < sizeof(struct input_event))
		return -EINVAL;

	rt = wait_event_interruptible(kdev->waitq, kdev->num_event != 0);
	if (rt)
		return rt;

	mutex_lock(&kdev->lock);
	for (num = 0; num < count / sizeof(struct input_event); num++) {
		if (!sensor_deq(kdev, &ev[num]))
			break;
	}
	mutex_unlock(&kdev->lock);

	if (copy_to_user(buf, ev, num * sizeof(struct input_event)))
		return -EFAULT;

	return num * sizeof(struct input_event);
}

static unsigned int sensor_poll(struct file *f, struct poll_table_struct *wait)
{
	struct sensor_device *kdev = f->private_data;

	poll_wait(f, &kdev->waitq, wait);
	if (kdev->num_event != 0)
		return POLLIN | POLLRDNORM;

	return 0;
}

static int sensor_open(struct inode *inode, struct file *f)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	struct sensor_device *kdev;

	kdev = kzalloc(sizeof(struct sensor_device), GFP_KERNEL);
	if (!kdev)
		return -ENOMEM;

	mutex_init(&kdev->lock);
	init_waitqueue_head(&kdev->waitq);
	f->private_data = kdev;
	kdev->head = 0;
	kdev->num_event = 0;
	list_add(&kdev->list, &data->devfile_list);

	return 0;
}

static int sensor_release(struct inode *inode, struct file *f)
{
	struct sensor_device *kdev = f->private_data;

	list_del(&kdev->list);
	kfree(kdev);

	return 0;
}

static int sensor_raw_open(struct inode *inode, struct file *f)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	struct sensor_device *kdev;

	kdev = kzalloc(sizeof(struct sensor_device), GFP_KERNEL);
	if (!kdev)
		return -ENOMEM;

	mutex_init(&kdev->lock);
	init_waitqueue_head(&kdev->waitq);
	f->private_data = kdev;
	kdev->head = 0;
	kdev->num_event = 0;
	list_add(&kdev->list, &data->raw_devfile_list);

	return 0;
}

const struct file_operations sensor_fops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.release = sensor_release,
	.write = sensor_write,
	.read = sensor_read,
	.poll = sensor_poll,
};

static struct miscdevice sensor_devfile = {
	.name = SENSOR_NAME,
	.fops = &sensor_fops,
	.minor = MISC_DYNAMIC_MINOR,
};

const struct file_operations sensor_raw_fops = {
	.owner = THIS_MODULE,
	.open = sensor_raw_open,
	.release = sensor_release,
	.write = sensor_write,
	.read = sensor_read,
	.poll = sensor_poll,
};

static struct miscdevice sensor_raw_devfile = {
	.name = SENSOR_RAW_NAME,
	.fops = &sensor_raw_fops,
	.minor = MISC_DYNAMIC_MINOR,
};

#endif

static int geomagnetic_i2c_open(void)
{
	return 0;
}

static int geomagnetic_i2c_close(void)
{
	return 0;
}

#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS529
static int geomagnetic_i2c_write(const uint8_t *buf, int len)
{
	if (i2c_master_send(this_client, buf, len) < 0)
		return -1;
#if DEBUG
	YLOGD(("[W] [%02x]\n", buf[0]));
#endif

	return 0;
}

static int geomagnetic_i2c_read(uint8_t *buf, int len)
{
	if (i2c_master_recv(this_client, buf, len) < 0)
		return -1;
	return 0;
}

#else

static int geomagnetic_i2c_write(uint8_t addr, const uint8_t *buf, int len)
{
	uint8_t tmp[16];

	if (sizeof(tmp) - 1 < len)
		return -1;

	tmp[0] = addr;
	memcpy(&tmp[1], buf, len);

	if (i2c_master_send(this_client, tmp, len + 1) < 0)
		return -1;
#if DEBUG
	YLOGD(("[W] addr[%02x] [%02x]\n", addr, buf[0]));
#endif

	return 0;
}

static int geomagnetic_i2c_read(uint8_t addr, uint8_t *buf, int len)
{
	struct i2c_msg msg[2];
	int err;

	msg[0].addr = this_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;
	msg[1].addr = this_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	err = i2c_transfer(this_client->adapter, msg, 2);
	if (err != 2) {
		dev_err(&this_client->dev,
			"i2c_transfer() read error: slave_addr=%02x, reg_addr=%02x, err=%d\n",
			this_client->addr, addr, err);
		return err;
	}

	return 0;
}

#endif

static int geomagnetic_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);
	rt = down_interruptible(&data->driver_lock);
	if (rt < 0)
		up(&data->driver_lock);
	return rt;
}

static int geomagnetic_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);
	up(&data->driver_lock);
	return 0;
}

static void geomagnetic_msleep(int ms)
{
	usleep_range(ms * 999, ms * 1000);
}

static void geomagnetic_current_time(int32_t *sec, int32_t *msec)
{
	struct timeval tv;

	do_gettimeofday(&tv);

	*sec = tv.tv_sec;
	*msec = tv.tv_usec / 1000;
}

static struct yas_mag_driver hwdep_driver = {
	.callback = {
		     .lock = geomagnetic_lock,
		     .unlock = geomagnetic_unlock,
		     .device_open = geomagnetic_i2c_open,
		     .device_close = geomagnetic_i2c_close,
		     .device_read = geomagnetic_i2c_read,
		     .device_write = geomagnetic_i2c_write,
		     .msleep = geomagnetic_msleep,
		     .current_time = geomagnetic_current_time,
		     },
};

static int geomagnetic_multi_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);
	rt = down_interruptible(&data->multi_lock);
	if (rt < 0)
		up(&data->multi_lock);
	return rt;
}

static int geomagnetic_multi_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);
	up(&data->multi_lock);
	return 0;
}

static int geomagnetic_enable(struct geomagnetic_data *data)
{
	if (!atomic_cmpxchg(&data->enable, 0, 1))
		schedule_delayed_work(&data->work, 0);

	return 0;
}

static int geomagnetic_disable(struct geomagnetic_data *data)
{
	if (atomic_cmpxchg(&data->enable, 1, 0))
		cancel_delayed_work_sync(&data->work);

	return 0;
}

/* Sysfs interface */
static ssize_t
geomagnetic_delay_show(struct device *dev,
		       struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int delay;

	geomagnetic_multi_lock();

	delay = data->delay;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", delay);
}

static ssize_t
geomagnetic_delay_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	if (hwdep_driver.set_delay == NULL)
		return -ENOTTY;

	geomagnetic_multi_lock();

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;
	if (hwdep_driver.set_delay(value) == 0)
		data->delay = value;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t
geomagnetic_enable_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;
	value = !(!value);

	if (hwdep_driver.set_enable == NULL)
		return -ENOTTY;

	if (geomagnetic_multi_lock() < 0)
		return count;

	if (hwdep_driver.set_enable(value) == 0) {
		if (value)
			geomagnetic_enable(data);
		else
			geomagnetic_disable(data);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int filter_enable;

	geomagnetic_multi_lock();

	filter_enable = data->filter_enable;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_enable);
}

static ssize_t
geomagnetic_filter_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	unsigned long value;
	int error;

	if (hwdep_driver.set_filter_enable == NULL)
		return -ENOTTY;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	if (geomagnetic_multi_lock() < 0)
		return count;

	if (hwdep_driver.set_filter_enable(value) == 0)
		data->filter_enable = !!value;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_len_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int filter_len;

	geomagnetic_multi_lock();

	filter_len = data->filter_len;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_len);
}

static ssize_t
geomagnetic_filter_len_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	struct yas_mag_filter filter;
	unsigned long value;
	int error;

	if (hwdep_driver.get_filter == NULL || hwdep_driver.set_filter == NULL)
		return -ENOTTY;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	if (geomagnetic_multi_lock() < 0)
		return count;

	hwdep_driver.get_filter(&filter);
	filter.len = value;
	if (hwdep_driver.set_filter(&filter) == 0)
		data->filter_len = value;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_noise_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int rt;

	geomagnetic_multi_lock();

	rt = sprintf(buf, "%d %d %d\n",
		     data->filter_noise[0],
		     data->filter_noise[1], data->filter_noise[2]);

	geomagnetic_multi_unlock();

	return rt;
}

static ssize_t
geomagnetic_filter_noise_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct yas_mag_filter filter;
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int32_t filter_noise[3];

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d",
	       &filter_noise[0], &filter_noise[1], &filter_noise[2]);
	hwdep_driver.get_filter(&filter);
	memcpy(filter.noise, filter_noise, sizeof(filter.noise));
	if (hwdep_driver.set_filter(&filter) == 0)
		memcpy(data->filter_noise, filter_noise,
		       sizeof(data->filter_noise));

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_threshold_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int32_t filter_threshold;

	geomagnetic_multi_lock();

	filter_threshold = data->filter_threshold;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_threshold);
}

static ssize_t
geomagnetic_filter_threshold_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	struct yas_mag_filter filter;
	unsigned long value;
	int error;

	if (hwdep_driver.get_filter == NULL || hwdep_driver.set_filter == NULL)
		return -ENOTTY;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	if (geomagnetic_multi_lock() < 0)
		return count;

	hwdep_driver.get_filter(&filter);
	filter.threshold = value;
	if (hwdep_driver.set_filter(&filter) == 0)
		data->filter_threshold = value;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_position_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	if (hwdep_driver.get_position == NULL)
		return -ENOTTY;
	return sprintf(buf, "%d\n", hwdep_driver.get_position());
}

static ssize_t
geomagnetic_position_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	if (hwdep_driver.set_position == NULL)
		return -ENOTTY;
	hwdep_driver.set_position(value);

	return count;
}

static ssize_t
geomagnetic_data_show(struct device *dev,
		      struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d %d %d\n",
		     atomic_read(&data->last_data[0]),
		     atomic_read(&data->last_data[1]),
		     atomic_read(&data->last_data[2]));

	return rt;
}

static ssize_t
geomagnetic_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d\n", atomic_read(&data->last_status));

	return rt;
}

static ssize_t
geomagnetic_status_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	static int16_t cnt = 1;
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev;
#endif
	int accuracy = 0;
	int code = 0;
	int value = 0;

	geomagnetic_multi_lock();

	sscanf(buf, "%d", &accuracy);
	if (0 <= accuracy && accuracy <= 3)
		atomic_set(&data->last_status, accuracy);
	code |= YAS_REPORT_CALIB_OFFSET_CHANGED;
	value = (cnt++ << 16) | (code);

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	mkev(&ev, EV_ABS, ABS_RAW_REPORT, value);
	sensor_event(&data->raw_devfile_list, &ev, 1);
#else
	input_report_abs(data->input_raw, ABS_RAW_REPORT, value);
	input_sync(data->input_raw);
#endif

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_wake_store(struct device *dev,
		       struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	static int16_t cnt = 1;
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev[1];
	make_event(ev, EV_ABS, ABS_WAKE, cnt++);
	sensor_event(&data->devfile_list, ev, 1);
#else
	input_report_abs(data->input_data, ABS_WAKE, cnt++);
	input_sync(data->input_data);
#endif

	return count;
}

#if DEBUG

static int geomagnetic_suspend(struct i2c_client *client, pm_message_t mesg);
static int geomagnetic_resume(struct i2c_client *client);

static ssize_t
geomagnetic_debug_suspend_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", data->suspend);
}

static ssize_t
geomagnetic_debug_suspend_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long suspend;
	int error;

	error = strict_strtoul(buf, 10, &suspend);
	if (unlikely(error))
		return error;

	if (suspend) {
		pm_message_t msg;
		memset(&msg, 0, sizeof(msg));
		geomagnetic_suspend(this_client, msg);
	} else
		geomagnetic_resume(this_client);

	return count;
}

#endif /* DEBUG */

static DEVICE_ATTR(delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_delay_show, geomagnetic_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_enable_show, geomagnetic_enable_store);
static DEVICE_ATTR(filter_enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_filter_enable_show,
		   geomagnetic_filter_enable_store);
static DEVICE_ATTR(filter_len, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_filter_len_show, geomagnetic_filter_len_store);
static DEVICE_ATTR(filter_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_filter_threshold_show,
		   geomagnetic_filter_threshold_store);
static DEVICE_ATTR(filter_noise, S_IRUGO | S_IWUSR | S_IWGRP,
		   geomagnetic_filter_noise_show,
		   geomagnetic_filter_noise_store);
static DEVICE_ATTR(data, S_IRUGO, geomagnetic_data_show, NULL);
static DEVICE_ATTR(status, S_IRUGO|S_IWUSR|S_IWGRP, geomagnetic_status_show,
		geomagnetic_status_store);
static DEVICE_ATTR(wake, S_IWUSR | S_IWGRP, NULL, geomagnetic_wake_store);
static DEVICE_ATTR(position, S_IRUGO | S_IWUSR,
		   geomagnetic_position_show, geomagnetic_position_store);
#if DEBUG
static DEVICE_ATTR(debug_suspend, S_IRUGO | S_IWUSR,
		   geomagnetic_debug_suspend_show,
		   geomagnetic_debug_suspend_store);
#endif /* DEBUG */

static struct attribute *geomagnetic_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_filter_enable.attr,
	&dev_attr_filter_len.attr,
	&dev_attr_filter_threshold.attr,
	&dev_attr_filter_noise.attr,
	&dev_attr_data.attr,
	&dev_attr_status.attr,
	&dev_attr_wake.attr,
	&dev_attr_position.attr,
#if DEBUG
	&dev_attr_debug_suspend.attr,
#endif /* DEBUG */
	NULL
};

static struct attribute_group geomagnetic_attribute_group = {
	.attrs = geomagnetic_attributes
};

static ssize_t
geomagnetic_raw_threshold_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int threshold;

	geomagnetic_multi_lock();

	threshold = data->threshold;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", threshold);
}

static ssize_t
geomagnetic_raw_threshold_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	geomagnetic_multi_lock();

	if (value <= 2) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		struct input_event ev[1];
		make_event(ev, EV_ABS, ABS_RAW_THRESHOLD, value);
		sensor_event(&data->raw_devfile_list, ev, 1);
#endif
		data->threshold = value;
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		input_report_abs(data->input_raw, ABS_RAW_THRESHOLD, value);
		input_sync(data->input_raw);
#endif
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_distortion_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int rt;

	geomagnetic_multi_lock();

	rt = sprintf(buf, "%d %d %d\n",
		     data->distortion[0],
		     data->distortion[1], data->distortion[2]);

	geomagnetic_multi_unlock();

	return rt;
}

static ssize_t
geomagnetic_raw_distortion_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int32_t distortion[3];
	static int32_t val = 1;
	int i;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d", &distortion[0], &distortion[1], &distortion[2]);
	if (distortion[0] > 0 && distortion[1] > 0 && distortion[2] > 0) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		struct input_event ev[1];
		make_event(ev, EV_ABS, ABS_RAW_DISTORTION, val++);
		sensor_event(&data->raw_devfile_list, ev, 1);
#endif
		for (i = 0; i < 3; i++)
			data->distortion[i] = distortion[i];
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		input_report_abs(data->input_raw, ABS_RAW_DISTORTION, val++);
		input_sync(data->input_raw);
#endif
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_shape_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int shape;

	geomagnetic_multi_lock();

	shape = data->shape;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", shape);
}

static ssize_t
geomagnetic_raw_shape_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

	geomagnetic_multi_lock();

	if (0 == value || value == 1) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		struct input_event ev[1];
		make_event(ev, EV_ABS, ABS_RAW_SHAPE, value);
		sensor_event(&data->raw_devfile_list, ev, 1);
#endif
		data->shape = value;
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		input_report_abs(data->input_raw, ABS_RAW_SHAPE, value);
		input_sync(data->input_raw);
#endif
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_offsets_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_mag_offset offset;
	int accuracy;

	geomagnetic_multi_lock();

	offset = data->driver_offset;
	accuracy = atomic_read(&data->last_status);

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d %d %d %d %d %d %d\n",
		       offset.hard_offset[0],
		       offset.hard_offset[1],
		       offset.hard_offset[2],
		       offset.calib_offset.v[0],
		       offset.calib_offset.v[1],
		       offset.calib_offset.v[2], accuracy);
}

static ssize_t
geomagnetic_raw_offsets_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_mag_offset offset;
	int32_t hard_offset[3];
	int i, accuracy;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d %d %d %d %d",
	       &hard_offset[0],
	       &hard_offset[1],
	       &hard_offset[2],
	       &offset.calib_offset.v[0],
	       &offset.calib_offset.v[1], &offset.calib_offset.v[2], &accuracy);
	if (0 <= accuracy && accuracy <= 3) {
		for (i = 0; i < 3; i++)
			offset.hard_offset[i] = (int8_t) hard_offset[i];

		if (hwdep_driver.set_offset(&offset) == 0) {
			atomic_set(&data->last_status, accuracy);
			data->driver_offset = offset;
		}
	}

	geomagnetic_multi_unlock();

	return count;
}

#ifdef YAS_MAG_MANUAL_OFFSET
static ssize_t
geomagnetic_raw_manual_offsets_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_vector offset;

	geomagnetic_multi_lock();

	offset = data->manual_offset;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d %d %d\n", offset.v[0], offset.v[1],
		       offset.v[2]);
}

static ssize_t
geomagnetic_raw_manual_offsets_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_vector offset;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d", &offset.v[0], &offset.v[1], &offset.v[2]);
	if (hwdep_driver.set_manual_offset(&offset) == 0)
		data->manual_offset = offset;

	geomagnetic_multi_unlock();

	return count;
}
#endif

static ssize_t
geomagnetic_raw_static_matrix_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_matrix matrix;

	geomagnetic_multi_lock();

	matrix = data->static_matrix;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d %d %d %d %d %d %d %d %d\n",
		       matrix.matrix[0], matrix.matrix[1], matrix.matrix[2],
		       matrix.matrix[3], matrix.matrix[4], matrix.matrix[5],
		       matrix.matrix[6], matrix.matrix[7], matrix.matrix[8]);
}

static ssize_t
geomagnetic_raw_static_matrix_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_matrix matrix;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d %d %d %d %d %d %d",
	       &matrix.matrix[0], &matrix.matrix[1], &matrix.matrix[2],
	       &matrix.matrix[3], &matrix.matrix[4], &matrix.matrix[5],
	       &matrix.matrix[6], &matrix.matrix[7], &matrix.matrix[8]);
	if (hwdep_driver.set_static_matrix(&matrix) == 0)
		data->static_matrix = matrix;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_dynamic_matrix_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_matrix matrix;

	geomagnetic_multi_lock();

	matrix = data->dynamic_matrix;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d %d %d %d %d %d %d %d %d\n",
		       matrix.matrix[0], matrix.matrix[1], matrix.matrix[2],
		       matrix.matrix[3], matrix.matrix[4], matrix.matrix[5],
		       matrix.matrix[6], matrix.matrix[7], matrix.matrix[8]);
}

static ssize_t
geomagnetic_raw_dynamic_matrix_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_matrix matrix;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d %d %d %d %d %d %d",
	       &matrix.matrix[0], &matrix.matrix[1], &matrix.matrix[2],
	       &matrix.matrix[3], &matrix.matrix[4], &matrix.matrix[5],
	       &matrix.matrix[6], &matrix.matrix[7], &matrix.matrix[8]);
	if (hwdep_driver.set_dynamic_matrix(&matrix) == 0)
		data->dynamic_matrix = matrix;

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_ellipsoid_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int ellipsoid_mode;

	geomagnetic_multi_lock();

	ellipsoid_mode = data->ellipsoid_mode;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", ellipsoid_mode);
}

static ssize_t
geomagnetic_raw_ellipsoid_mode_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;
	value = !(!value);
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev[1];
#endif

	geomagnetic_multi_lock();

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	make_event(ev, EV_ABS, ABS_RAW_MODE, value);
	sensor_event(&data->raw_devfile_list, ev, 1);
#endif
	data->ellipsoid_mode = value;
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	input_report_abs(data->input_raw, ABS_RAW_MODE, value);
	input_sync(data->input_raw);
#endif

	geomagnetic_multi_unlock();

	return count;
}

#ifdef SYSFS_PCBTEST

static int
pcbtest_i2c_write(uint8_t slave, uint8_t addr, const uint8_t *buf, int len)
{
	return geomagnetic_i2c_write(addr, buf, len);
}

static int
pcbtest_i2c_read(uint8_t slave, uint8_t addr, uint8_t *buf, int len)
{
	return geomagnetic_i2c_read(addr, buf, len);
}

static struct yas_pcb_test pcbtest = {
	.callback = {
		.power_on	= NULL,
		.power_off	= NULL,
		.i2c_open	= geomagnetic_i2c_open,
		.i2c_close	= geomagnetic_i2c_close,
		.i2c_read	= pcbtest_i2c_read,
		.i2c_write	= pcbtest_i2c_write,
		.msleep		= geomagnetic_msleep,
		.read_intpin	= NULL,
	},
};


static ssize_t
geomagnetic_raw_self_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	int id, x, y1, y2, dir, sx, sy, ohx, ohy, ohz;
	int err1, err2, err3, err4, err5, err6, err7;

	if (data->noise_test_init)
		pcbtest.power_off();
	err1 = pcbtest.power_on_and_device_check(&id);
	err3 = pcbtest.initialization();
	err4 =
		pcbtest.offset_control_measurement_and_set_offset_register(
			&x, &y1, &y2);
	err5 = pcbtest.direction_measurement(&dir);
	err6 =
		pcbtest.sensitivity_measurement_of_magnetic_sensor_by_test_coil(
			&sx, &sy);
	err7 = pcbtest.magnetic_field_level_check(&ohx, &ohy, &ohz);
	err2 = pcbtest.power_off();
	data->noise_test_init = 0;
	if (unlikely(id != 0x2))
		err1 = -1;
	if (unlikely(x < -30 || x > 30))
		err4 = -1;
	if (unlikely(y1 < -30 || y1 > 30))
		err4 = -1;
	if (unlikely(y2 < -30 || y2 > 30))
		err4 = -1;
	if (unlikely(sx < 17 || sy < 22))
		err6 = -1;
	if (unlikely(ohx < -600 || ohx > 600))
		err7 = -1;
	if (unlikely(ohy < -600 || ohy > 600))
		err7 = -1;
	if (unlikely(ohz < -600 || ohz > 600))
		err7 = -1;

	pr_info("%s\n"
		"Test1 - err = %d, id = %d\n"
		"Test3 - err = %d\n"
		"Test4 - err = %d, offset = %d,%d,%d\n"
		"Test5 - err = %d, direction = %d\n"
		"Test6 - err = %d, sensitivity = %d,%d\n"
		"Test7 - err = %d, offset = %d,%d,%d\n"
		"Test2 - err = %d\n", __func__,
		err1, id, err3, err4, x, y1, y2, err5, 0, err6, sx, sy,
		err7, ohx, ohy, ohz, err2);

	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err1, id, err3, err4, x, y1, y2, err5, 0, err6, sx,
			sy, err7, ohx, ohy, ohz, err2);
}

static ssize_t
geomagnetic_raw_self_test_noise_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	int id, x, y1, y2, dir, hx0, hy0, hz0;
	int err8;

	pcbtest.power_on_and_device_check(&id);
	pcbtest.initialization();
	pcbtest.offset_control_measurement_and_set_offset_register(
		&x, &y1, &y2);
	pcbtest.direction_measurement(&dir);
	err8 = pcbtest.noise_level_check(&hx0, &hy0, &hz0);
	if (err8 < 0) {
		pr_err("%s: err8=%d\n", __func__, err8);
		hx0 = 0;
		hy0 = 0;
		hz0 = 0;
	}
	usleep_range(3000, 3100);
	data->noise_test_init = 1;
	pr_debug("%s: %d, %d, %d\n", __func__, hx0, hy0, hz0);

	if (hx0 > YAS_NOISE_MAX)
		hx0 = YAS_NOISE_MAX;
	else if (hx0 < YAS_NOISE_MIN)
		hx0 = YAS_NOISE_MIN;

	if (hy0 > YAS_NOISE_MAX)
		hy0 = YAS_NOISE_MAX;
	else if (hy0 < YAS_NOISE_MIN)
		hy0 = YAS_NOISE_MIN;

	if (hz0 > YAS_NOISE_MAX)
		hz0 = YAS_NOISE_MAX;
	else if (hz0 < YAS_NOISE_MIN)
		hz0 = YAS_NOISE_MIN;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", hx0, hy0, hz0);
}

static DEVICE_ATTR(self_test, S_IRUSR, geomagnetic_raw_self_test_show, NULL);
static DEVICE_ATTR(self_test_noise, S_IRUSR,
		geomagnetic_raw_self_test_noise_show, NULL);

#endif


static DEVICE_ATTR(threshold, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_threshold_show,
		   geomagnetic_raw_threshold_store);
static DEVICE_ATTR(distortion, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_distortion_show,
		   geomagnetic_raw_distortion_store);
static DEVICE_ATTR(shape, S_IRUGO | S_IWUSR, geomagnetic_raw_shape_show,
		   geomagnetic_raw_shape_store);
static DEVICE_ATTR(offsets, S_IRUGO | S_IWUSR, geomagnetic_raw_offsets_show,
		   geomagnetic_raw_offsets_store);
#ifdef YAS_MAG_MANUAL_OFFSET
static DEVICE_ATTR(manual_offsets, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_manual_offsets_show,
		   geomagnetic_raw_manual_offsets_store);
#endif
static DEVICE_ATTR(static_matrix, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_static_matrix_show,
		   geomagnetic_raw_static_matrix_store);
static DEVICE_ATTR(dynamic_matrix, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_dynamic_matrix_show,
		   geomagnetic_raw_dynamic_matrix_store);
static DEVICE_ATTR(ellipsoid_mode, S_IRUGO | S_IWUSR,
		   geomagnetic_raw_ellipsoid_mode_show,
		   geomagnetic_raw_ellipsoid_mode_store);
static struct attribute *geomagnetic_raw_attributes[] = {
#ifdef SYSFS_PCBTEST
	&dev_attr_self_test.attr,
	&dev_attr_self_test_noise.attr,
#endif
	&dev_attr_threshold.attr,
	&dev_attr_distortion.attr,
	&dev_attr_shape.attr,
	&dev_attr_offsets.attr,
#ifdef YAS_MAG_MANUAL_OFFSET
	&dev_attr_manual_offsets.attr,
#endif
	&dev_attr_static_matrix.attr,
	&dev_attr_dynamic_matrix.attr,
	&dev_attr_ellipsoid_mode.attr,
	NULL
};

static struct attribute_group geomagnetic_raw_attribute_group = {
	.attrs = geomagnetic_raw_attributes
};
static struct device_attribute dev_attr_magnetic_sensor_selftest =
	__ATTR(selftest, S_IRUGO | S_IWUSR | S_IWGRP,
	geomagnetic_raw_self_test_show, NULL);

static struct device_attribute dev_attr_magnetic_sensor_raw_data =
	__ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
	geomagnetic_raw_self_test_noise_show, NULL);

static ssize_t magnetic_vendor_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "%s\n", "YAMAHA");
}
static struct device_attribute dev_attr_magnetic_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, magnetic_vendor_show, NULL);

static ssize_t magnetic_name_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	int ret;
	if (data->dev_id == YAS_YAS532_DEVICE_ID)
		ret = sprintf(buf, "%s\n", "YAS532");
	else
		ret = sprintf(buf, "%s\n", "YAS530C");
	return ret;
}
static struct device_attribute dev_attr_magnetic_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, magnetic_name_show, NULL);

static struct device_attribute *magnetic_sensor_attrs[] = {
	&dev_attr_magnetic_sensor_selftest,
	&dev_attr_magnetic_sensor_raw_data,
	&dev_attr_magnetic_sensor_vendor,
	&dev_attr_magnetic_sensor_name,
	NULL,
};


/* Interface Functions for Lower Layer */
#ifdef YAS_MAG_MANUAL_OFFSET
void geomagnetic_manual_offset(void)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	struct yas_vector offset;
	if (data->mag_pdata->chg_status == CABLE_TYPE_NONE) {
		offset = data->mag_pdata->full_offset;
		if (hwdep_driver.set_manual_offset(&offset) == 0)
			data->manual_offset = offset;
	} else if (data->mag_pdata->chg_status == CABLE_TYPE_USB) {
		offset = data->mag_pdata->usb_offset;
		if (hwdep_driver.set_manual_offset(&offset) == 0)
			data->manual_offset = offset;
	} else if (data->mag_pdata->chg_status == CABLE_TYPE_AC) {
		offset = data->mag_pdata->ta_offset;
		if (hwdep_driver.set_manual_offset(&offset) == 0)
			data->manual_offset = offset;
	} else {
		offset = data->mag_pdata->full_offset;
		if (hwdep_driver.set_manual_offset(&offset) == 0)
			data->manual_offset = offset;
	}
	data->mag_pdata->offset_enable = 0;
}
#endif

static int geomagnetic_work(struct yas_mag_data *magdata)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	uint32_t time_delay_ms = 100;
	static int cnt;
	int rt, i, accuracy;
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev[5];
	struct timeval tv;
#endif

#ifdef YAS_MAG_MANUAL_OFFSET
	if (data->mag_pdata) {
		if (data->mag_pdata->offset_enable)
			geomagnetic_manual_offset();
	}
#endif

	if (hwdep_driver.measure == NULL || hwdep_driver.get_offset == NULL)
		return time_delay_ms;

	rt = hwdep_driver.measure(magdata, &time_delay_ms);
	if (rt < 0)
		YLOGE(("measure failed[%d]\n", rt));
	YLOGD(("xy1y2 [%d][%d][%d] raw[%d][%d][%d]\n",
	       magdata->xy1y2.v[0], magdata->xy1y2.v[1], magdata->xy1y2.v[2],
	       magdata->xyz.v[0], magdata->xyz.v[1], magdata->xyz.v[2]));

	if (rt >= 0) {
		accuracy = atomic_read(&data->last_status);

		if ((rt & YAS_REPORT_OVERFLOW_OCCURED)
		    || (rt & YAS_REPORT_HARD_OFFSET_CHANGED)
		    || (rt & YAS_REPORT_CALIB_OFFSET_CHANGED)) {
			static uint16_t count = 1;
			int code = 0;
			int value = 0;

			hwdep_driver.get_offset(&data->driver_offset);
			if (rt & YAS_REPORT_OVERFLOW_OCCURED) {
				atomic_set(&data->last_status, 0);
				accuracy = 0;
			}

			/* report event */
			code |= (rt & YAS_REPORT_OVERFLOW_OCCURED);
			code |= (rt & YAS_REPORT_HARD_OFFSET_CHANGED);
			code |= (rt & YAS_REPORT_CALIB_OFFSET_CHANGED);
			value = (count++ << 16) | (code);
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
			make_event(ev, EV_ABS, ABS_RAW_REPORT, value);
			sensor_event(&data->raw_devfile_list, ev, 1);
#else
			input_report_abs(data->input_raw, ABS_RAW_REPORT,
					 value);
			input_sync(data->input_raw);
#endif
		}

		if (rt & YAS_REPORT_DATA) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
			get_time_stamp(&tv);
			make_event_w_time(&ev[0], EV_ABS, ABS_X,
					  magdata->xyz.v[0], &tv);
			make_event_w_time(&ev[1], EV_ABS, ABS_Y,
					  magdata->xyz.v[1], &tv);
			make_event_w_time(&ev[2], EV_ABS, ABS_Z,
					  magdata->xyz.v[2], &tv);
			make_event_w_time(&ev[3], EV_ABS, ABS_STATUS, accuracy,
					  &tv);
			make_event_w_time(&ev[4], EV_SYN, 0, 0, &tv);
			sensor_event(&data->devfile_list, ev, 5);
#else
			/* report magnetic data in [nT] */
			input_report_abs(data->input_data, ABS_X,
					 magdata->xyz.v[0]);
			input_report_abs(data->input_data, ABS_Y,
					 magdata->xyz.v[1]);
			input_report_abs(data->input_data, ABS_Z,
					 magdata->xyz.v[2]);
			if (atomic_read(&data->last_data[0]) ==
			    magdata->xyz.v[0]
			    && atomic_read(&data->last_data[1]) ==
			    magdata->xyz.v[1]
			    && atomic_read(&data->last_data[2]) ==
			    magdata->xyz.v[2]) {
				input_report_abs(data->input_data, ABS_RUDDER,
						 cnt++);
			}
			input_report_abs(data->input_data, ABS_STATUS,
					 accuracy);
			input_sync(data->input_data);
#endif

			for (i = 0; i < 3; i++)
				atomic_set(&data->last_data[i],
					magdata->xyz.v[i]);
		}

		if (rt & YAS_REPORT_CALIB) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
			get_time_stamp(&tv);
			make_event_w_time(&ev[0], EV_ABS, ABS_X,
					  magdata->raw.v[0], &tv);
			make_event_w_time(&ev[1], EV_ABS, ABS_Y,
					  magdata->raw.v[1], &tv);
			make_event_w_time(&ev[2], EV_ABS, ABS_Z,
					  magdata->raw.v[2], &tv);
			make_event_w_time(&ev[3], EV_SYN, 0, 0, &tv);
			sensor_event(&data->raw_devfile_list, ev, 4);
#else
			/* report raw magnetic data */
			input_report_abs(data->input_raw, ABS_X,
					 magdata->raw.v[0]);
			input_report_abs(data->input_raw, ABS_Y,
					 magdata->raw.v[1]);
			input_report_abs(data->input_raw, ABS_Z,
					 magdata->raw.v[2]);
			input_sync(data->input_raw);
#endif
		}
	} else {
		time_delay_ms = 100;
	}

	return time_delay_ms;

}

static void geomagnetic_input_work_func(struct work_struct *work)
{
	struct geomagnetic_data *data =
	    container_of((struct delayed_work *)work,
			 struct geomagnetic_data, work);
	uint32_t time_delay_ms;
	struct yas_mag_data magdata;

	time_delay_ms = geomagnetic_work(&magdata);

	if (time_delay_ms > 60)
		schedule_delayed_work(&data->work,
				      msecs_to_jiffies(time_delay_ms));
	else {
		if (time_delay_ms > 0)
			usleep_range(time_delay_ms * 1000,
				time_delay_ms * 1100);
		schedule_delayed_work(&data->work, 0);
	}
}

static int geomagnetic_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
#if DEBUG
	data->suspend = 1;
#endif

	return 0;
}

static int geomagnetic_resume(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (atomic_read(&data->enable))
		schedule_delayed_work(&data->work, 0);
#if DEBUG
	data->suspend = 0;
#endif

	return 0;
}

static int
geomagnetic_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct geomagnetic_data *data = NULL;
	struct input_dev *input_data = NULL, *input_raw = NULL;
	int rt, sysfs_created = 0, sysfs_raw_created = 0;
	int data_registered = 0, raw_registered = 0, i;
	struct yas_mag_filter filter;
	struct mag_platform_data *pdata;

	pr_err("%s, is called\n", __func__);

	i2c_set_clientdata(client, NULL);
	data = kzalloc(sizeof(struct geomagnetic_data), GFP_KERNEL);
	if (data == NULL) {
		rt = -ENOMEM;
		goto err;
	}

	pdata = (struct mag_platform_data *) client->dev.platform_data;
	data->mag_pdata = pdata;

	if (pdata) {
		if (pdata->power_on)
			pdata->power_on(true);
	}
	data->threshold = YAS_DEFAULT_MAGCALIB_THRESHOLD;
	for (i = 0; i < 3; i++)
		data->distortion[i] = YAS_DEFAULT_MAGCALIB_DISTORTION;
	data->shape = 0;
	atomic_set(&data->enable, 0);
	for (i = 0; i < 3; i++)
		atomic_set(&data->last_data[i], 0);
	atomic_set(&data->last_status, 0);
	INIT_DELAYED_WORK(&data->work, geomagnetic_input_work_func);
	sema_init(&data->driver_lock, 1);
	sema_init(&data->multi_lock, 1);

	input_data = input_allocate_device();
	if (input_data == NULL) {
		rt = -ENOMEM;
		YLOGE(("mag Failed to allocate input_data device\n"));
		goto err;
	}

	input_data->name = GEOMAGNETIC_INPUT_NAME;
	input_data->id.bustype = BUS_I2C;
	set_bit(EV_ABS, input_data->evbit);
	input_set_abs_params(input_data, ABS_X, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_Y, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_Z, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_RUDDER, 0x80000000, 0x7fffffff, 0,
			     0);
	input_set_abs_params(input_data, ABS_STATUS, 0, 3, 0, 0);
	input_set_abs_params(input_data, ABS_WAKE, 0x80000000, 0x7fffffff, 0,
			     0);
	input_data->dev.parent = &client->dev;

	rt = input_register_device(input_data);
	if (rt) {
		YLOGE(("mag: Unable to reg input_data %s\n", input_data->name));
		goto err;
	}
	data_registered = 1;

	rt = sysfs_create_group(&input_data->dev.kobj,
				&geomagnetic_attribute_group);
	if (rt) {
		YLOGE(("mag sysfs_create failed[%s]\n", input_data->name));
		goto err;
	}
	sysfs_created = 1;

	input_raw = input_allocate_device();
	if (input_raw == NULL) {
		rt = -ENOMEM;
		YLOGE(("mag Failed to alloc input_raw dev\n"));
		goto err;
	}

	input_raw->name = GEOMAGNETIC_INPUT_RAW_NAME;
	input_raw->id.bustype = BUS_I2C;
	set_bit(EV_ABS, input_raw->evbit);
	input_set_abs_params(input_raw, ABS_X, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_raw, ABS_Y, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_raw, ABS_Z, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_raw, ABS_RAW_DISTORTION, 0, 0x7fffffff, 0,
			     0);
	input_set_abs_params(input_raw, ABS_RAW_THRESHOLD, 0, 2, 0, 0);
	input_set_abs_params(input_raw, ABS_RAW_SHAPE, 0, 1, 0, 0);
	input_set_abs_params(input_raw, ABS_RAW_MODE, 0, 1, 0, 0);
	input_set_abs_params(input_raw, ABS_RAW_REPORT, 0x80000000, 0x7fffffff,
			     0, 0);
	input_raw->dev.parent = &client->dev;

	rt = input_register_device(input_raw);
	if (rt) {
		YLOGE(("mag: Unable to reg input_raw dev\n"));
		goto err;
	}
	raw_registered = 1;

	rt = sysfs_create_group(&input_raw->dev.kobj,
				&geomagnetic_raw_attribute_group);
	if (rt) {
		YLOGE(("geomagnetic_probe: sysfs_create_group failed[%s]\n",
		       input_data->name));
		goto err;
	}
	sysfs_raw_created = 1;

	this_client = client;
	data->input_raw = input_raw;
	data->input_data = input_data;
	input_set_drvdata(input_data, data);
	input_set_drvdata(input_raw, data);
	i2c_set_clientdata(client, data);
	rt = yas_mag_driver_init(&hwdep_driver);
	if (rt < 0) {
		YLOGE(("yas_mag_driver_init failed[%d]\n", rt));
		goto err;
	}
	if (hwdep_driver.init != NULL) {
		rt = hwdep_driver.init();
		if (rt < 0) {
			YLOGE(("hwdep_driver.init() failed[%d]\n", rt));
			goto err;
		}
	}
	if (hwdep_driver.set_position != NULL) {
		if (pdata) {
			if (pdata->orientation) {
				pr_info("%s: set from board file.\n", __func__);
				if (hwdep_driver.
					set_position(pdata->orientation
						- YAS532_POSITION_OFFSET) < 0) {
					pr_err("set_position failed %d\n", rt);
					goto err;
				}
			}  else {
				pr_info("%s: set from defconfig.\n", __func__);
				if (hwdep_driver.
					set_position(
					CONFIG_INPUT_YAS_MAGNETOMETER_POSITION)
					< 0) {
					pr_err("set_position failed %d\n", rt);
					goto err;
				}
			}
		} else {
			if (hwdep_driver.
				set_position(
					CONFIG_INPUT_YAS_MAGNETOMETER_POSITION)
						< 0) {
				pr_err("set_position() failed[%d]\n", rt);
				goto err;
			}
		}
		pr_info("%s: yas magnetic position is %d\n", __func__,
			hwdep_driver.get_position());
	}
	if (hwdep_driver.get_offset != NULL) {
		if (hwdep_driver.get_offset(&data->driver_offset) < 0) {
			YLOGE(("hwdep_driver get_driver_state failed\n"));
			goto err;
		}
	}
	if (hwdep_driver.get_delay != NULL)
		data->delay = hwdep_driver.get_delay();

	if (hwdep_driver.set_filter_enable != NULL) {
		/* default to enable */
		if (hwdep_driver.set_filter_enable(1) == 0)
			data->filter_enable = 1;
	}
	if (hwdep_driver.get_filter != NULL) {
		if (hwdep_driver.get_filter(&filter) < 0) {
			YLOGE(("hwdep_driver get_filter failed\n"));
			goto err;
		}
		data->filter_len = filter.len;
		for (i = 0; i < 3; i++)
			data->filter_noise[i] = filter.noise[i];
		data->filter_threshold = filter.threshold;
	}
	if (hwdep_driver.get_static_matrix != NULL)
		hwdep_driver.get_static_matrix(&data->static_matrix);
	if (hwdep_driver.get_dynamic_matrix != NULL)
		hwdep_driver.get_dynamic_matrix(&data->dynamic_matrix);
#ifdef SYSFS_PCBTEST
	rt = yas_pcb_test_init(&pcbtest);
	if (rt < 0) {
		YLOGE(("yas_pcb_test_init failed[%d]\n", rt));
		goto err;
	}
#endif
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	INIT_LIST_HEAD(&data->devfile_list);
	INIT_LIST_HEAD(&data->raw_devfile_list);
	if (misc_register(&sensor_devfile) < 0)
		goto err;
	if (misc_register(&sensor_raw_devfile) < 0) {
		misc_deregister(&sensor_devfile);
		goto err;
	}
#endif

	rt =
		geomagnetic_i2c_read(YAS_REGADDR_DEVICE_ID, &data->dev_id, 1);
	if (rt) {
		pr_err("%s: cound not read device id(%d).\n",
					__func__, rt);
				goto err;
	} else
		pr_info("%s: yamaha magnetic sensor id = %x\n",
			__func__, data->dev_id);

	rt = sensors_register(data->magnetic_sensor_device,
		NULL, magnetic_sensor_attrs,
			"magnetic_sensor");
	if (rt) {
		pr_err("%s: cound not register magnetic sensor device(%d).\n",
			__func__, rt);
		goto out_sensor_register_failed;
	}

	return 0;

out_sensor_register_failed:
err:
	if (data != NULL) {
		if (input_raw != NULL) {
			if (sysfs_raw_created)
				sysfs_remove_group(&input_raw->dev.kobj,
					&geomagnetic_raw_attribute_group);

			if (raw_registered)
				input_unregister_device(input_raw);
			else
				input_free_device(input_raw);
		}
		if (input_data != NULL) {
			if (sysfs_created)
				sysfs_remove_group(&input_data->dev.kobj,
					&geomagnetic_attribute_group);
			if (data_registered)
				input_unregister_device(input_data);
			else
				input_free_device(input_data);
		}
		if (pdata) {
			if (pdata->power_on)
				pdata->power_on(false);
		}
		kfree(data);
	}

	return rt;
}

static int geomagnetic_remove(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	misc_deregister(&sensor_devfile);
	misc_deregister(&sensor_raw_devfile);
#endif
	if (data != NULL) {
		geomagnetic_disable(data);
		if (hwdep_driver.term != NULL)
			hwdep_driver.term();

		input_unregister_device(data->input_raw);
		sysfs_remove_group(&data->input_data->dev.kobj,
				   &geomagnetic_attribute_group);
		sysfs_remove_group(&data->input_raw->dev.kobj,
				   &geomagnetic_raw_attribute_group);
		input_unregister_device(data->input_data);
		kfree(data);
	}

	return 0;
}

static void geomagnetic_shutdown(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (data != NULL) {
		geomagnetic_disable(data);
		if (data->mag_pdata) {
			if (data->mag_pdata->power_on)
				data->mag_pdata->power_on(false);
		}
	}
}

/* I2C Device Driver */
static struct i2c_device_id geomagnetic_idtable[] = {
	{GEOMAGNETIC_I2C_DEVICE_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, geomagnetic_idtable);

static struct i2c_driver geomagnetic_i2c_driver = {
	.driver = {
		   .name = GEOMAGNETIC_I2C_DEVICE_NAME,
		   .owner = THIS_MODULE,
		   },

	.id_table = geomagnetic_idtable,
	.probe = geomagnetic_probe,
	.remove = geomagnetic_remove,
	.suspend = geomagnetic_suspend,
	.resume = geomagnetic_resume,
	.shutdown = geomagnetic_shutdown,
};

static int __init geomagnetic_init(void)
{
	return i2c_add_driver(&geomagnetic_i2c_driver);
}

static void __exit geomagnetic_term(void)
{
	i2c_del_driver(&geomagnetic_i2c_driver);
}

#ifdef GEOMAGNETIC_PLATFORM_API
static int geomagnetic_api_enable(int enable)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	int rt;

	if (geomagnetic_multi_lock() < 0)
		return -1;
	enable = !!enable;
	rt = hwdep_driver.set_enable(enable);
	if (rt == 0) {
		atomic_set(&data->enable, enable);
		if (enable)
			rt = hwdep_driver.set_delay(20);
	}

	geomagnetic_multi_unlock();

	return rt;
}

int geomagnetic_api_resume(void)
{
	return geomagnetic_api_enable(1);
}
EXPORT_SYMBOL(geomagnetic_api_resume);

int geomagnetic_api_suspend(void)
{
	return geomagnetic_api_enable(0);
}
EXPORT_SYMBOL(geomagnetic_api_suspend);

int geomagnetic_api_read(int *xyz, int *raw, int *xy1y2, int *accuracy)
{
	struct geomagnetic_data *data = i2c_get_clientdata(this_client);
	struct yas_mag_data magdata;
	int i;

	geomagnetic_work(&magdata);
	if (xyz != NULL) {
		for (i = 0; i < 3; i++)
			xyz[i] = magdata.xyz.v[i];
	}
	if (raw != NULL) {
		for (i = 0; i < 3; i++)
			raw[i] = magdata.raw.v[i];
	}
	if (xy1y2 != NULL) {
		for (i = 0; i < 3; i++)
			xy1y2[i] = magdata.xy1y2.v[i];
	}
	if (accuracy != NULL)
		*accuracy = atomic_read(&data->last_status);

	return 0;
}
EXPORT_SYMBOL(geomagnetic_api_read);
#endif

module_init(geomagnetic_init);
module_exit(geomagnetic_term);

MODULE_AUTHOR("Yamaha Corporation");
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS529
MODULE_DESCRIPTION("YAS529 Geomagnetic Sensor Driver");
#elif YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS530
MODULE_DESCRIPTION("YAS530 Geomagnetic Sensor Driver");
#elif CONFIG_SENSORS_YAS532
MODULE_DESCRIPTION("YAS532 Geomagnetic Sensor Driver");
#endif
MODULE_LICENSE("GPL");
MODULE_VERSION("4.4.702a");
