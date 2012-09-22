/* linux/driver/input/misc/cm3663.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/sensor/cm3663.h>

#define PROX_READ_NUM		40

/* ADDSEL is LOW */
#define REGS_ARA		0x18
#define REGS_ALS_CMD		0x20
#define REGS_ALS_MSB		0x21
#define REGS_INIT		0x22
#define REGS_ALS_LSB		0x23
#define REGS_PS_CMD		0xB0
#define REGS_PS_DATA		0xB1
#define REGS_PS_THD		0xB2

static u8 reg_defaults[8] = {
	0x00, /* ARA: read only register */
	0x00, /* ALS_CMD: als cmd */
	0x00, /* ALS_MSB: read only register */
	0x20, /* INIT: interrupt disable */
	0x00, /* ALS_LSB: read only register */
	0x30, /* PS_CMD: interrupt disable */
	0x00, /* PS_DATA: read only register */
	0x0A, /* PS_THD: 10 */
};


enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* driver data */
struct cm3663_data {
	struct input_dev *proximity_input_dev;
	struct input_dev *light_input_dev;
	struct i2c_client *i2c_client;
	struct work_struct work_light;
	struct work_struct work_prox;
	struct hrtimer light_timer;
	struct hrtimer prox_timer;
	struct mutex power_lock;
	struct wake_lock prx_wake_lock;
	struct workqueue_struct *light_wq;
	struct workqueue_struct *prox_wq;
	struct class *lightsensor_class;
	struct class *proximity_class;
	struct device *switch_cmd_dev;
	struct device *proximity_dev;
	struct cm3663_platform_data *pdata;
	bool als_buf_initialized;
	bool on;
	int als_index_count;
	int irq;
	int avg[3];
	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;
	u8 power_state;
};

int cm3663_i2c_read(struct cm3663_data *cm3663, u8 addr, u8 *val)
{
	int err = 0;
	int retry = 10;
	struct i2c_msg msg[1];
	struct i2c_client *client = cm3663->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	msg->addr = addr >> 1;
	msg->flags = 1;
	msg->len = 1;
	msg->buf = val;

	if (addr == REGS_ARA)
		retry = 2;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}

	if (addr == REGS_ARA)
		return 0;

	pr_err("%s: i2c read failed at addr 0x%x: %d\n", __func__, addr, err);
	return err;
}

int cm3663_i2c_write(struct cm3663_data *cm3663, u8 addr, u8 val)
{
	u8 data = val;
	int err = 0;
	int retry = 10;
	struct i2c_msg msg[1];
	struct i2c_client *client = cm3663->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	msg->addr = addr >> 1;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = &data;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}

	pr_err("%s: i2c write failed at addr 0x%x: %d\n", __func__, addr, err);
	return err;
}

static void cm3663_light_enable(struct cm3663_data *cm3663)
{
	u8 tmp;
	int64_t temp_time = 0;
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_write(cm3663, REGS_INIT, reg_defaults[3]);
	cm3663_i2c_write(cm3663, REGS_ALS_CMD, reg_defaults[1]);
	temp_time = ktime_to_ns(cm3663->light_poll_delay) + 100000000;
	hrtimer_start(&cm3663->light_timer, ns_to_ktime(temp_time),
						HRTIMER_MODE_REL);
}

static void cm3663_light_disable(struct cm3663_data *cm3663)
{
	cm3663_i2c_write(cm3663, REGS_ALS_CMD, 0x01);
	hrtimer_cancel(&cm3663->light_timer);
	cancel_work_sync(&cm3663->work_light);
}

static int lightsensor_get_alsvalue(struct cm3663_data *cm3663)
{
	int value = 0;
	u8 als_high, als_low;

	/* get ALS */
	cm3663_i2c_read(cm3663, REGS_ALS_LSB, &als_low);
	cm3663_i2c_read(cm3663, REGS_ALS_MSB, &als_high);

	value = ((als_high << 8) | als_low) * 5;
	return value;
}

static void proxsensor_get_avgvalue(struct cm3663_data *cm3663)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u8 proximity_value = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		cm3663_i2c_read(cm3663, REGS_PS_DATA, &proximity_value);
		avg += proximity_value;

		if (!i)
			min = proximity_value;
		else if (proximity_value < min)
			min = proximity_value;

		if (proximity_value > max)
			max = proximity_value;
	}
	avg /= PROX_READ_NUM;

	cm3663->avg[0] = min;
	cm3663->avg[1] = avg;
	cm3663->avg[2] = max;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	u8 proximity_value = 0;

	if (!(cm3663->power_state & PROXIMITY_ENABLED)) {
		mutex_lock(&cm3663->power_lock);
		cm3663->pdata->proximity_power(1);
		cm3663_i2c_write(cm3663, REGS_PS_CMD, reg_defaults[5]);
		mutex_unlock(&cm3663->power_lock);
	}

	msleep(20);
	cm3663_i2c_read(cm3663, REGS_PS_DATA, &proximity_value);

	if (!(cm3663->power_state & PROXIMITY_ENABLED)) {
		mutex_lock(&cm3663->power_lock);
		cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01);
		cm3663->pdata->proximity_power(0);
		mutex_unlock(&cm3663->power_lock);
	}

	return sprintf(buf, "%d", proximity_value);
}

static ssize_t lightsensor_file_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	int adc = 0;

	if (!(cm3663->power_state & LIGHT_ENABLED))
		cm3663_light_enable(cm3663);

	adc = lightsensor_get_alsvalue(cm3663);

	if (!(cm3663->power_state & LIGHT_ENABLED))
		cm3663_light_disable(cm3663);

	return sprintf(buf, "%d\n", adc);
}

static ssize_t poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(cm3663->light_poll_delay));
}

static ssize_t poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm3663->power_lock);
	if (new_delay != ktime_to_ns(cm3663->light_poll_delay)) {
		cm3663->light_poll_delay = ns_to_ktime(new_delay);
		if (cm3663->power_state & LIGHT_ENABLED) {
			cm3663_light_disable(cm3663);
			cm3663_light_enable(cm3663);
		}
	}
	mutex_unlock(&cm3663->power_lock);

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (cm3663->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (cm3663->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm3663->power_lock);
	if (new_value && !(cm3663->power_state & LIGHT_ENABLED)) {
		cm3663->power_state |= LIGHT_ENABLED;
		cm3663_light_enable(cm3663);
	} else if (!new_value && (cm3663->power_state & LIGHT_ENABLED)) {
		cm3663_light_disable(cm3663);
		cm3663->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&cm3663->power_lock);
	return size;
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	bool new_value;
	u8 tmp;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm3663->power_lock);
	if (new_value && !(cm3663->power_state & PROXIMITY_ENABLED)) {
		cm3663->pdata->proximity_power(1);
		cm3663->power_state |= PROXIMITY_ENABLED;
		cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
		cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
		cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
		cm3663_i2c_write(cm3663, REGS_INIT, reg_defaults[3]);
		cm3663_i2c_write(cm3663, REGS_PS_THD, reg_defaults[7]);
		cm3663_i2c_write(cm3663, REGS_PS_CMD, reg_defaults[5]);

		enable_irq(cm3663->irq);
		enable_irq_wake(cm3663->irq);
	} else if (!new_value && (cm3663->power_state & PROXIMITY_ENABLED)) {
		cm3663->power_state &= ~PROXIMITY_ENABLED;
		disable_irq_wake(cm3663->irq);
		disable_irq(cm3663->irq);
		cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01);
		cm3663->pdata->proximity_power(0);
	}
	mutex_unlock(&cm3663->power_lock);
	return size;
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n",
		cm3663->avg[0], cm3663->avg[1], cm3663->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cm3663_data *cm3663 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm3663->power_lock);
	if (new_value) {
		if (!(cm3663->power_state & PROXIMITY_ENABLED)) {
			cm3663->pdata->proximity_power(1);
			cm3663_i2c_write(cm3663, REGS_PS_CMD, reg_defaults[5]);
		}
		hrtimer_start(&cm3663->prox_timer, cm3663->prox_poll_delay,
							HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&cm3663->prox_timer);
		cancel_work_sync(&cm3663->work_prox);
		if (!(cm3663->power_state & PROXIMITY_ENABLED)) {
			cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01);
			cm3663->pdata->proximity_power(0);
		}
	}
	mutex_unlock(&cm3663->power_lock);

	return size;
}

static DEVICE_ATTR(proximity_avg, 0644,
		   proximity_avg_show, proximity_avg_store);
static DEVICE_ATTR(proximity_state, 0644, proximity_state_show, NULL);

static DEVICE_ATTR(lightsensor_file_state, 0644, lightsensor_file_state_show,
	NULL);

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   poll_delay_show, poll_delay_store);

static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void cm3663_work_func_light(struct work_struct *work)
{
	int als;
	struct cm3663_data *cm3663 = container_of(work, struct cm3663_data,
					      work_light);

	als = lightsensor_get_alsvalue(cm3663);
	input_report_abs(cm3663->light_input_dev,
					ABS_MISC, als + 1);
	input_sync(cm3663->light_input_dev);
}

static void cm3663_work_func_prox(struct work_struct *work)
{
	struct cm3663_data *cm3663 = container_of(work, struct cm3663_data,
					      work_prox);
	proxsensor_get_avgvalue(cm3663);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm3663_light_timer_func(struct hrtimer *timer)
{
	struct cm3663_data *cm3663
			= container_of(timer, struct cm3663_data, light_timer);
	queue_work(cm3663->light_wq, &cm3663->work_light);
	hrtimer_forward_now(&cm3663->light_timer, cm3663->light_poll_delay);
	return HRTIMER_RESTART;
}

static enum hrtimer_restart cm3663_prox_timer_func(struct hrtimer *timer)
{
	struct cm3663_data *cm3663
			= container_of(timer, struct cm3663_data, prox_timer);
	queue_work(cm3663->prox_wq, &cm3663->work_prox);
	hrtimer_forward_now(&cm3663->prox_timer, cm3663->prox_poll_delay);
	return HRTIMER_RESTART;
}

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t cm3663_irq_thread_fn(int irq, void *data)
{
	struct cm3663_data *ip = data;
	u8 val = 1;
	u8 tmp;

	val = gpio_get_value(ip->i2c_client->irq);
	if (val < 0) {
		pr_err("%s: gpio_get_value error %d\n", __func__, val);
		return IRQ_HANDLED;
	}

	/* for debugging : going to be removed */
	cm3663_i2c_read(ip, REGS_PS_DATA, &tmp);
	pr_err("%s: proximity value = %d, val = %d\n", __func__, tmp, val);

	/* 0 is close, 1 is far */
	input_report_abs(ip->proximity_input_dev, ABS_DISTANCE, val);
	input_sync(ip->proximity_input_dev);
	wake_lock_timeout(&ip->prx_wake_lock, 3*HZ);

	return IRQ_HANDLED;
}

static int cm3663_setup_irq(struct cm3663_data *cm3663)
{
	int rc = -EIO;
	int irq;

	irq = gpio_to_irq(cm3663->i2c_client->irq);
	rc = request_threaded_irq(irq, NULL, cm3663_irq_thread_fn,
			 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			 "proximity_int", cm3663);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq, irq, rc);
		return rc;
	}

	/* start with interrupts disabled */
	disable_irq(irq);
	cm3663->irq = irq;

	return rc;
}

static int cm3663_setup_reg(struct cm3663_data *cm3663)
{
	int err = 0;
	u8 tmp;

	/* initializing the proximity and light sensor registers */
	mutex_lock(&cm3663->power_lock);
	cm3663->pdata->proximity_power(1);
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_read(cm3663, REGS_ARA, &tmp);
	cm3663_i2c_write(cm3663, REGS_INIT, reg_defaults[3]);
	cm3663_i2c_write(cm3663, REGS_PS_THD, reg_defaults[7]);
	cm3663_i2c_write(cm3663, REGS_PS_CMD, reg_defaults[5]);
	mutex_unlock(&cm3663->power_lock);

	/* printing the inital proximity value with no contact */
	msleep(50);
	err = cm3663_i2c_read(cm3663, REGS_PS_DATA, &tmp);
	if (err < 0) {
		pr_err("%s: read ps_data failed\n", __func__);
		err = -EIO;
	}
	pr_err("%s: initial proximity value = %d\n", __func__, tmp);
	mutex_lock(&cm3663->power_lock);
	cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01);
	cm3663->pdata->proximity_power(0);
	mutex_unlock(&cm3663->power_lock);

	return err;
}

static int cm3663_i2c_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct input_dev *input_dev;
	struct cm3663_data *cm3663;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	cm3663 = kzalloc(sizeof(struct cm3663_data), GFP_KERNEL);
	if (!cm3663) {
		pr_err("%s: failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	cm3663->pdata = client->dev.platform_data;
	cm3663->i2c_client = client;
	i2c_set_clientdata(client, cm3663);

	/* wake lock init */
	wake_lock_init(&cm3663->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&cm3663->power_lock);

	/* setup initial registers */
	ret = cm3663_setup_reg(cm3663);
	if (ret < 0) {
		pr_err("%s: could not setup regs\n", __func__);
		goto err_setup_reg;
	}

	ret = cm3663_setup_irq(cm3663);
	if (ret) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		goto err_input_allocate_device_proximity;
	}
	cm3663->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, cm3663);
	input_dev->name = "proximity_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_proximity;
	}
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm3663->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm3663->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm3663->light_timer.function = cm3663_light_timer_func;

	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm3663->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm3663->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);
	cm3663->prox_timer.function = cm3663_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm3663->light_wq = create_singlethread_workqueue("cm3663_light_wq");
	if (!cm3663->light_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create light workqueue\n", __func__);
		goto err_create_light_workqueue;
	}
	cm3663->prox_wq = create_singlethread_workqueue("cm3663_prox_wq");
	if (!cm3663->prox_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create prox workqueue\n", __func__);
		goto err_create_prox_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm3663->work_light, cm3663_work_func_light);
	INIT_WORK(&cm3663->work_prox, cm3663_work_func_prox);

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, cm3663);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	cm3663->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* set sysfs for proximity sensor */
	cm3663->proximity_class = class_create(THIS_MODULE, "proximity");
	if (IS_ERR(cm3663->proximity_class)) {
		pr_err("%s: could not create proximity_class\n", __func__);
		goto err_proximity_class_create;
	}

	cm3663->proximity_dev = device_create(cm3663->proximity_class,
						NULL, 0, NULL, "proximity");
	if (IS_ERR(cm3663->proximity_dev)) {
		pr_err("%s: could not create proximity_dev\n", __func__);
		goto err_proximity_device_create;
	}

	if (device_create_file(cm3663->proximity_dev,
		&dev_attr_proximity_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_proximity_state.attr.name);
		goto err_proximity_device_create_file1;
	}

	if (device_create_file(cm3663->proximity_dev,
		&dev_attr_proximity_avg) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_proximity_avg.attr.name);
		goto err_proximity_device_create_file2;
	}
	dev_set_drvdata(cm3663->proximity_dev, cm3663);

	/* set sysfs for light sensor */
	cm3663->lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(cm3663->lightsensor_class)) {
		pr_err("%s: could not create lightsensor_class\n", __func__);
		goto err_light_class_create;
	}

	cm3663->switch_cmd_dev = device_create(cm3663->lightsensor_class,
						NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(cm3663->switch_cmd_dev)) {
		pr_err("%s: could not create switch_cmd_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(cm3663->switch_cmd_dev,
		&dev_attr_lightsensor_file_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_lightsensor_file_state.attr.name);
		goto err_light_device_create_file;
	}
	dev_set_drvdata(cm3663->switch_cmd_dev, cm3663);

	goto done;

/* error, unwind it all */
err_light_device_create_file:
	device_destroy(cm3663->lightsensor_class, 0);
err_light_device_create:
	class_destroy(cm3663->lightsensor_class);
err_light_class_create:
	device_remove_file(cm3663->proximity_dev, &dev_attr_proximity_avg);
err_proximity_device_create_file2:
	device_remove_file(cm3663->proximity_dev, &dev_attr_proximity_state);
err_proximity_device_create_file1:
	device_destroy(cm3663->proximity_class, 0);
err_proximity_device_create:
	class_destroy(cm3663->proximity_class);
err_proximity_class_create:
	sysfs_remove_group(&input_dev->dev.kobj,
			&light_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(cm3663->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(cm3663->prox_wq);
err_create_prox_workqueue:
	destroy_workqueue(cm3663->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm3663->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(cm3663->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
	free_irq(cm3663->irq, 0);
err_setup_irq:
err_setup_reg:
	mutex_destroy(&cm3663->power_lock);
	wake_lock_destroy(&cm3663->prx_wake_lock);
	kfree(cm3663);
done:
	return ret;
}

static int cm3663_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   cm3663->power_state because we use that state in resume.
	*/
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3663_data *cm3663 = i2c_get_clientdata(client);
	if (cm3663->power_state & LIGHT_ENABLED)
		cm3663_light_disable(cm3663);

	return 0;
}

static int cm3663_resume(struct device *dev)
{
	/* Turn power back on if we were before suspend. */
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3663_data *cm3663 = i2c_get_clientdata(client);

	if (cm3663->power_state & LIGHT_ENABLED)
		cm3663_light_enable(cm3663);
	return 0;
}

static int cm3663_i2c_remove(struct i2c_client *client)
{
	struct cm3663_data *cm3663 = i2c_get_clientdata(client);

	device_remove_file(cm3663->proximity_dev,
				&dev_attr_proximity_avg);
	device_remove_file(cm3663->switch_cmd_dev,
				&dev_attr_lightsensor_file_state);
	device_destroy(cm3663->lightsensor_class, 0);
	class_destroy(cm3663->lightsensor_class);
	device_remove_file(cm3663->proximity_dev, &dev_attr_proximity_avg);
	device_remove_file(cm3663->proximity_dev, &dev_attr_proximity_state);
	device_destroy(cm3663->proximity_class, 0);
	class_destroy(cm3663->proximity_class);
	sysfs_remove_group(&cm3663->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(cm3663->light_input_dev);
	sysfs_remove_group(&cm3663->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(cm3663->proximity_input_dev);
	free_irq(cm3663->irq, NULL);
	if (cm3663->power_state) {
		if (cm3663->power_state & LIGHT_ENABLED)
			cm3663_light_disable(cm3663);
		if (cm3663->power_state & PROXIMITY_ENABLED) {
			cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01);
			cm3663->pdata->proximity_power(0);
		}
	}
	destroy_workqueue(cm3663->light_wq);
	destroy_workqueue(cm3663->prox_wq);
	mutex_destroy(&cm3663->power_lock);
	wake_lock_destroy(&cm3663->prx_wake_lock);
	kfree(cm3663);
	return 0;
}

static const struct i2c_device_id cm3663_device_id[] = {
	{"cm3663", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cm3663_device_id);

static const struct dev_pm_ops cm3663_pm_ops = {
	.suspend = cm3663_suspend,
	.resume = cm3663_resume
};

static struct i2c_driver cm3663_i2c_driver = {
	.driver = {
		.name = "cm3663",
		.owner = THIS_MODULE,
		.pm = &cm3663_pm_ops
	},
	.probe		= cm3663_i2c_probe,
	.remove		= cm3663_i2c_remove,
	.id_table	= cm3663_device_id,
};


static int __init cm3663_init(void)
{
	return i2c_add_driver(&cm3663_i2c_driver);
}

static void __exit cm3663_exit(void)
{
	i2c_del_driver(&cm3663_i2c_driver);
}

module_init(cm3663_init);
module_exit(cm3663_exit);

MODULE_AUTHOR("tim.sk.lee@samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for cm3663");
MODULE_LICENSE("GPL");
