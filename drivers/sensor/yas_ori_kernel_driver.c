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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#define __LINUX_KERNEL_DRIVER__
#include <linux/sensor/yas.h>

#define SENSOR_NAME "orientation"
#define SENSOR_DEFAULT_DELAY            (200)	/* 200 ms */
#define SENSOR_MAX_DELAY                (2000)	/* 2000 ms */
#define ABS_STATUS                      (ABS_BRAKE)
#define ABS_WAKE                        (ABS_MISC)
#define ABS_CONTROL_REPORT              (ABS_THROTTLE)

static int suspend(void);
static int resume(void);

struct sensor_data {
	struct mutex mutex;
	int enabled;
	int delay;
#if DEBUG
	int suspend;
#endif
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct list_head devfile_list;
#endif
};

static struct platform_device *sensor_pdev;
static struct input_dev *this_data;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
#include <linux/miscdevice.h>
#define MAX_COUNT (64)

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
	struct sensor_data *data = input_get_drvdata(this_data);
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
	struct sensor_data *data = input_get_drvdata(this_data);
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

#endif

static int suspend(void)
{
	/* implement suspend of the sensor */
	YLOGD(("%s: suspend\n", SENSOR_NAME));

	return 0;
}

static int resume(void)
{
	/* implement resume of the sensor */
	YLOGD(("%s: resume\n", SENSOR_NAME));

	return 0;
}

/* Sysfs interface */
static ssize_t
sensor_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct sensor_data *data = input_get_drvdata(input_data);
	int delay;

	mutex_lock(&data->mutex);

	delay = data->delay;

	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d\n", delay);
}

static ssize_t
sensor_delay_store(struct device *dev,
		   struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct sensor_data *data = input_get_drvdata(input_data);
	long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev[1];
#endif

	if (value < 0)
		return count;

	if (SENSOR_MAX_DELAY < value)
		value = SENSOR_MAX_DELAY;

	mutex_lock(&data->mutex);

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	make_event(ev, EV_ABS, ABS_CONTROL_REPORT,
		   (data->enabled << 16) | value);
	sensor_event(&data->devfile_list, ev, 1);
#endif
	data->delay = value;
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	input_report_abs(input_data, ABS_CONTROL_REPORT,
			 (data->enabled << 16) | value);
#endif

	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t
sensor_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct sensor_data *data = input_get_drvdata(input_data);
	int enabled;

	mutex_lock(&data->mutex);

	enabled = data->enabled;

	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d\n", enabled);
}

static ssize_t
sensor_enable_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct sensor_data *data = input_get_drvdata(input_data);
	unsigned long value;
	int error;

	error = strict_strtoul(buf, 10, &value);
	if (unlikely(error))
		return error;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct input_event ev[1];
#endif

	value = !(!value);

	mutex_lock(&data->mutex);

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	make_event(ev, EV_ABS, ABS_CONTROL_REPORT, (value << 16) | data->delay);
	sensor_event(&data->devfile_list, ev, 1);
#else
	input_report_abs(input_data, ABS_CONTROL_REPORT,
			 (value << 16) | data->delay);
	input_sync(input_data);
#endif

	if (data->enabled && !value)
		suspend();
	if (!data->enabled && value)
		resume();
	data->enabled = value;

	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t
sensor_wake_store(struct device *dev,
		  struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	static int cnt = 1;
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	struct sensor_data *data = input_get_drvdata(input_data);
	struct input_event ev[2];
	struct timeval tv;
	get_time_stamp(&tv);
	make_event_w_time(&ev[0], EV_ABS, ABS_WAKE, cnt++, &tv);
	make_event_w_time(&ev[1], EV_SYN, 0, 0, &tv);
	sensor_event(&data->devfile_list, ev, 2);
#else
	input_report_abs(input_data, ABS_WAKE, cnt++);
	input_sync(input_data);
#endif

	return count;
}

#if DEBUG

static int sensor_suspend(struct platform_device *pdev, pm_message_t state);
static int sensor_resume(struct platform_device *pdev);

static ssize_t
sensor_debug_suspend_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct sensor_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", data->suspend);
}

static ssize_t
sensor_debug_suspend_store(struct device *dev,
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
		sensor_suspend(sensor_pdev, msg);
	} else
		sensor_resume(sensor_pdev);

	return count;
}

#endif /* DEBUG */

static ssize_t
sensor_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	int x, y, z;

	x = input_abs_get_val(input_data, ABS_X);
	y = input_abs_get_val(input_data, ABS_Y);
	z = input_abs_get_val(input_data, ABS_Z);

	return sprintf(buf, "%d %d %d\n", x, y, z);
}

static ssize_t
sensor_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	int status;

	status = input_abs_get_val(input_data, ABS_STATUS);

	return sprintf(buf, "%d\n", status);
}

static DEVICE_ATTR(delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   sensor_delay_show, sensor_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   sensor_enable_show, sensor_enable_store);
static DEVICE_ATTR(wake, S_IWUSR | S_IWGRP, NULL, sensor_wake_store);
static DEVICE_ATTR(data, S_IRUGO, sensor_data_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, sensor_status_show, NULL);

#if DEBUG
static DEVICE_ATTR(debug_suspend, S_IRUGO | S_IWUSR,
		   sensor_debug_suspend_show, sensor_debug_suspend_store);
#endif /* DEBUG */

static struct attribute *sensor_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_wake.attr,
	&dev_attr_data.attr,
	&dev_attr_status.attr,
#if DEBUG
	&dev_attr_debug_suspend.attr,
#endif /* DEBUG */
	NULL
};

static struct attribute_group sensor_attribute_group = {
	.attrs = sensor_attributes
};

static int sensor_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sensor_data *data = input_get_drvdata(this_data);
	int rt = 0;

	mutex_lock(&data->mutex);

	if (data->enabled) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		struct input_event ev[1];
		make_event(ev, EV_ABS, ABS_CONTROL_REPORT,
			   (0 << 16) | data->delay);
		sensor_event(&data->devfile_list, ev, 1);
#else
		input_report_abs(this_data, ABS_CONTROL_REPORT,
				 (0 << 16) | data->delay);
#endif
		rt = suspend();
	}

	mutex_unlock(&data->mutex);

	return rt;
}

static int sensor_resume(struct platform_device *pdev)
{
	struct sensor_data *data = input_get_drvdata(this_data);
	int rt = 0;

	mutex_lock(&data->mutex);

	if (data->enabled) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		struct input_event ev[1];
		make_event(ev, EV_ABS, ABS_CONTROL_REPORT,
			   (1 << 16) | data->delay);
		sensor_event(&data->devfile_list, ev, 1);
#endif
		rt = resume();
#ifndef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
		input_report_abs(this_data, ABS_CONTROL_REPORT,
				 (1 << 16) | data->delay);
#endif
	}

	mutex_unlock(&data->mutex);

	return rt;
}

static int sensor_probe(struct platform_device *pdev)
{
	struct sensor_data *data = NULL;
	struct input_dev *input_data = NULL;
	int input_registered = 0, sysfs_created = 0;
	int rt;

	data = kzalloc(sizeof(struct sensor_data), GFP_KERNEL);
	if (!data) {
		rt = -ENOMEM;
		goto err;
	}
	data->enabled = 0;
	data->delay = SENSOR_DEFAULT_DELAY;

	input_data = input_allocate_device();
	if (!input_data) {
		rt = -ENOMEM;
		YLOGE(("sensor_probe: Failed to allocate input_data device\n"));
		goto err;
	}

	set_bit(EV_ABS, input_data->evbit);
	input_set_abs_params(input_data, ABS_X, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_Y, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_Z, 0x80000000, 0x7fffffff, 0, 0);
	input_set_abs_params(input_data, ABS_RUDDER, 0x80000000, 0x7fffffff, 0,
			     0);
	input_set_abs_params(input_data, ABS_STATUS, 0, 3, 0, 0);
	input_set_abs_params(input_data, ABS_WAKE, 0x80000000, 0x7fffffff, 0,
			     0);
	input_set_abs_params(input_data, ABS_CONTROL_REPORT, 0x80000000,
			     0x7fffffff, 0, 0);
	input_data->name = SENSOR_NAME;

	rt = input_register_device(input_data);
	if (rt) {
		YLOGE(("ori Unable to reg input_data %s\n", input_data->name));
		goto err;
	}
	input_set_drvdata(input_data, data);
	input_registered = 1;

	rt = sysfs_create_group(&input_data->dev.kobj, &sensor_attribute_group);
	if (rt) {
		YLOGE(("sensor_probe: sysfs_create_group failed[%s]\n",
		       input_data->name));
		goto err;
	}
	sysfs_created = 1;
	mutex_init(&data->mutex);
	this_data = input_data;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	INIT_LIST_HEAD(&data->devfile_list);
	if (misc_register(&sensor_devfile) < 0)
		goto err;
#endif

	return 0;

err:
	if (data != NULL) {
		if (input_data != NULL) {
#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
			if (sysfs_created)
				sysfs_remove_group(&input_data->dev.kobj,
						   &sensor_attribute_group);
#endif
			if (input_registered)
				input_unregister_device(input_data);
			else
				input_free_device(input_data);
			input_data = NULL;
		}
		kfree(data);
	}

	return rt;
}

static int sensor_remove(struct platform_device *pdev)
{
	struct sensor_data *data;

#ifdef YAS_SENSOR_KERNEL_DEVFILE_INTERFACE
	misc_deregister(&sensor_devfile);
#endif
	if (this_data != NULL) {
		data = input_get_drvdata(this_data);
		sysfs_remove_group(&this_data->dev.kobj,
				   &sensor_attribute_group);
		input_unregister_device(this_data);
		if (data != NULL)
			kfree(data);
	}

	return 0;
}

/*
 * Module init and exit
 */
static struct platform_driver sensor_driver = {
	.probe = sensor_probe,
	.remove = sensor_remove,
	.suspend = sensor_suspend,
	.resume = sensor_resume,
	.driver = {
		   .name = SENSOR_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init sensor_init(void)
{
	sensor_pdev = platform_device_register_simple(SENSOR_NAME, 0, NULL, 0);
	if (IS_ERR(sensor_pdev))
		return -1;
	return platform_driver_register(&sensor_driver);
}

module_init(sensor_init);

static void __exit sensor_exit(void)
{
	platform_driver_unregister(&sensor_driver);
	platform_device_unregister(sensor_pdev);
}

module_exit(sensor_exit);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_LICENSE("GPL");
MODULE_VERSION("4.4.702a");
