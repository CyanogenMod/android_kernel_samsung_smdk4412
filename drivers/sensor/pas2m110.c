/* linux/driver/input/misc/pas2m110.c
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
#include <linux/sensor/pas2m110.h>

#define LIGHT_BUFFER_NUM	3
#define PROX_READ_NUM	40

/* ADDSEL is LOW */
#define REGS_PS1_CMD		0x00
#define REGS_PS2_CMD		0x01
#define REGS_ALS1_CMD		0x02
#define REGS_ALS2_CMD		0x03
#define REGS_ALS_LSB		0x04
#define REGS_ALS_MSB		0x05
#define REGS_PS_DATA		0x06


#define SUM_BUFFER_SIZE 5
#define TOTAL_BUFFER_SIZE 3	/* average */

int sum_value[SUM_BUFFER_SIZE] = {0, };
u8 sum_cnt = 0, buf_cnt = 0;
int sum_buffer[TOTAL_BUFFER_SIZE] = {0,};
int use_value = 100;



enum {
	PS_CMD1,
	PS_CMD2,
	ALS_CMD1,
	ALS_CMD2,
	ALS_LSB,
	ALS_MSB,
	PS_DATA,
};

static u8 reg_defaults[7] = {
	0x51, /* ps1 register - Gain High1, 110% Current, 8msec, PS Active*/
	0x08, /* ps2 register
			Level Low, PS interrupt output,
			ALS interrupt disable, PS interrupt disable */
	0x31, /* als1 register
			128Step, *8Lux, 12.5ms, Manual mode start, ALS Active */
	0x01, /* als2 register - fine ALS 66.7%*/
	0x00, /* als lsb data */
	0x00, /* als msb data */
	0x00, /* PS_DATA:*/
};

static const int adc_table[4] = {
	15,
	150,
	1500,
	15000,
};

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* driver data */
struct pas2m110_data {
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
	struct pas2m110_platform_data *pdata;
	bool als_buf_initialized;
	bool on;
	int als_index_count;
	int irq;
	int avg[3];
	int light_count;
	int light_buffer;
	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;
	u8 power_state;
};

int pas2m110_i2c_read(struct pas2m110_data *pas2m110, u8 cmd, u8 *val)
{
	int err = 0;
	int retry = 10;
	struct i2c_client *client = pas2m110->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		err = i2c_smbus_read_i2c_block_data(client, cmd, 1, val);
		if (err >= 0)
			return err;
	}

	pr_err("%s: i2c read failed at cmd 0x%x: %d\n", __func__, cmd, err);
	return err;
}

int pas2m110_i2c_write(struct pas2m110_data *pas2m110, u8 cmd, u8 val)
{
	u8 data[2] = {0, };
	int err = 0;
	int retry = 10;
	struct i2c_msg msg[1];
	struct i2c_client *client = pas2m110->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	data[0] = cmd;
	data[1] = val;

	msg->addr = 0x88 >> 1;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}

	pr_err("%s: i2c write failed at addr 0x%x: %d\n", __func__, cmd, err);
	return err;
}

static void pas2m110_light_enable(struct pas2m110_data *pas2m110)
{
	pas2m110->light_count = 0;
	pas2m110->light_buffer = 0;
	pas2m110_i2c_write(pas2m110, REGS_ALS1_CMD, reg_defaults[2]);
	pas2m110_i2c_write(pas2m110, REGS_ALS2_CMD, reg_defaults[3]);

	hrtimer_start(&pas2m110->light_timer, pas2m110->light_poll_delay,
						HRTIMER_MODE_REL);
}

static void pas2m110_light_disable(struct pas2m110_data *pas2m110)
{
	pas2m110_i2c_write(pas2m110, REGS_ALS1_CMD,
		reg_defaults[2]&(0x01^0xFF));
	hrtimer_cancel(&pas2m110->light_timer);
	cancel_work_sync(&pas2m110->work_light);
}

static int lightsensor_get_alsvalue(struct pas2m110_data *pas2m110)
{
	int value = 0;
	u8 als_high, als_low;
	u8 i, j;
	int temp;
	int sum_value2[SUM_BUFFER_SIZE];
	int total_value = 0;

	/* get ALS */
	pas2m110_i2c_read(pas2m110, REGS_ALS_LSB, &als_low);
	pas2m110_i2c_read(pas2m110, REGS_ALS_MSB, &als_high);

	value = ((als_high << 8) | als_low);	/* *1 */

	/* filter 1 */
	sum_value[sum_cnt++] = value;
	if (sum_cnt == SUM_BUFFER_SIZE)
		sum_cnt = 0;

	for (i = 0; i < SUM_BUFFER_SIZE; i++)
		sum_value2[i] = sum_value[i];

	for (i = 0; i < (SUM_BUFFER_SIZE/2+1); i++)	{
		for (j = i; j < SUM_BUFFER_SIZE; j++) {
			if (sum_value2[i] > sum_value2[j]) {
				temp = sum_value2[j];
				sum_value2[j] = sum_value2[i];
				sum_value2[i] = temp;
			}
		}
	}

	/* filter 2 */
	sum_buffer[buf_cnt++] = sum_value2[SUM_BUFFER_SIZE/2];
	if (buf_cnt == TOTAL_BUFFER_SIZE)
		buf_cnt = 0;

	for (i = 0; i < TOTAL_BUFFER_SIZE; i++)
		total_value += sum_buffer[i];

	value = total_value/TOTAL_BUFFER_SIZE;

	/* filter 3 */
	if (0 == value)
		use_value = 0;
	else if (0 < value && value <= 8)
		use_value = 4;
	else if (8 < value && value <= 16)
		use_value = 8;
	else if (16 < value && value <= 200) {
		if ((use_value - (use_value/10)) > value)
			use_value = value;
		else if (value > (use_value + (use_value/10)))
			use_value = value;
	} else if (value > 200) {
		if ((use_value - (use_value*5/100)) > value)
			use_value = value;
		else if (value > (use_value + (use_value*5/100)))
			use_value = value;
	}

	return use_value;
}

static void proxsensor_get_avgvalue(struct pas2m110_data *pas2m110)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u8 proximity_value = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);

		/*Use PS[0] bit Check */
		pas2m110_i2c_read(pas2m110, REGS_PS_DATA, &proximity_value);

		if (proximity_value == 252)
			proximity_value = 0;
		avg += proximity_value;

		if (!i)
			min = proximity_value;
		else if (proximity_value < min)
			min = proximity_value;

		if (proximity_value > max)
			max = proximity_value;
	}
	avg /= PROX_READ_NUM;

	pas2m110->avg[0] = min;
	pas2m110->avg[1] = avg;
	pas2m110->avg[2] = max;
}

static ssize_t pas2m110_proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	u8 proximity_value = 0;

	if (!(pas2m110->power_state & PROXIMITY_ENABLED)) {
		mutex_lock(&pas2m110->power_lock);
		pas2m110->pdata->proximity_power(1);
		pas2m110_i2c_write(pas2m110,
			REGS_PS1_CMD, reg_defaults[0]);
		pas2m110_i2c_write(pas2m110,
			REGS_PS2_CMD, reg_defaults[1]);
		mutex_unlock(&pas2m110->power_lock);
	}

	msleep(20);

	/* Use PS[0] bit Check */
	pas2m110_i2c_read(pas2m110, REGS_PS_DATA, &proximity_value);

	if (proximity_value == 252)
		proximity_value = 0;

	if (!(pas2m110->power_state & PROXIMITY_ENABLED)) {
		mutex_lock(&pas2m110->power_lock);

		/* PS sleep */
		pas2m110_i2c_write(pas2m110,
			REGS_PS1_CMD, reg_defaults[0]&(0x01^0xFF));

		pas2m110->pdata->proximity_power(0);
		mutex_unlock(&pas2m110->power_lock);
	}

	return sprintf(buf, "%d", proximity_value);
}

static ssize_t pas2m110_lightsensor_file_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	int adc = 0;

	if (!(pas2m110->power_state & LIGHT_ENABLED))
		pas2m110_light_enable(pas2m110);

	adc = lightsensor_get_alsvalue(pas2m110);

	if (!(pas2m110->power_state & LIGHT_ENABLED))
		pas2m110_light_disable(pas2m110);

	return sprintf(buf, "%d\n", adc);
}

static ssize_t poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(pas2m110->light_poll_delay));
}

static ssize_t poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&pas2m110->power_lock);
	if (new_delay != ktime_to_ns(pas2m110->light_poll_delay)) {
		pas2m110->light_poll_delay = ns_to_ktime(new_delay);
		if (pas2m110->power_state & LIGHT_ENABLED) {
			pas2m110_light_disable(pas2m110);
			pas2m110_light_enable(pas2m110);
		}
	}
	mutex_unlock(&pas2m110->power_lock);

	return size;
}

static ssize_t pas2m110_light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (pas2m110->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t pas2m110_proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (pas2m110->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t pas2m110_light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&pas2m110->power_lock);
	if (new_value && !(pas2m110->power_state & LIGHT_ENABLED)) {
		pas2m110->power_state |= LIGHT_ENABLED;
		pas2m110_light_enable(pas2m110);
	} else if (!new_value && (pas2m110->power_state & LIGHT_ENABLED)) {
		pas2m110_light_disable(pas2m110);
		pas2m110->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&pas2m110->power_lock);
	return size;
}

static ssize_t pas2m110_proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&pas2m110->power_lock);
	if (new_value && !(pas2m110->power_state & PROXIMITY_ENABLED)) {
		pas2m110->power_state |= PROXIMITY_ENABLED;
		pas2m110->pdata->proximity_power(1);
		pas2m110_i2c_write(pas2m110,
			REGS_PS1_CMD, reg_defaults[0]);
		msleep(90);
		pas2m110_i2c_write(pas2m110,
			REGS_PS2_CMD, reg_defaults[1]);
		enable_irq(pas2m110->irq);
		enable_irq_wake(pas2m110->irq);
	} else if (!new_value && (pas2m110->power_state & PROXIMITY_ENABLED)) {
		pas2m110->power_state &= ~PROXIMITY_ENABLED;
		disable_irq_wake(pas2m110->irq);
		disable_irq(pas2m110->irq);
		pas2m110_i2c_write(pas2m110,
			REGS_PS2_CMD, reg_defaults[1]|0x04);
		/* PS sleep */
		pas2m110_i2c_write(pas2m110,
			REGS_PS1_CMD, reg_defaults[0]&(0x01^0xFF));
		pas2m110->pdata->proximity_power(0);
	}
	mutex_unlock(&pas2m110->power_lock);
	return size;
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n",
		pas2m110->avg[0], pas2m110->avg[1], pas2m110->avg[2]);
}

static ssize_t pas2m110_proximity_avg_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct pas2m110_data *pas2m110 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&pas2m110->power_lock);
	if (new_value) {
		if (!(pas2m110->power_state & PROXIMITY_ENABLED)) {
			pas2m110->pdata->proximity_power(1);
			pas2m110_i2c_write(pas2m110,
					REGS_PS1_CMD, reg_defaults[0]);
			pas2m110_i2c_write(pas2m110,
					REGS_PS2_CMD, reg_defaults[1]);
		}
		hrtimer_start(&pas2m110->prox_timer, pas2m110->prox_poll_delay,
							HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&pas2m110->prox_timer);
		cancel_work_sync(&pas2m110->work_prox);
		if (!(pas2m110->power_state & PROXIMITY_ENABLED)) {
			/* ALS interrupt disable */
			pas2m110_i2c_write(pas2m110,
				REGS_PS1_CMD, reg_defaults[0]&(0x01^0xFF));
			pas2m110->pdata->proximity_power(0);
		}
	}
	mutex_unlock(&pas2m110->power_lock);

	return size;
}

static DEVICE_ATTR(proximity_avg, 0644,
			proximity_avg_show, pas2m110_proximity_avg_store);

static DEVICE_ATTR(proximity_state, 0644,
			pas2m110_proximity_state_show, NULL);

static DEVICE_ATTR(lightsensor_file_state, 0644,
			pas2m110_lightsensor_file_state_show, NULL);

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			poll_delay_show, poll_delay_store);

static struct device_attribute dev_attr_light_enable =
		__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		pas2m110_light_enable_show, pas2m110_light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
		__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		pas2m110_proximity_enable_show,
		pas2m110_proximity_enable_store);

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

static void pas2m110_work_func_light(struct work_struct *work)
{
	int i;
	int als;
	struct pas2m110_data *pas2m110 = container_of(work,
				struct pas2m110_data, work_light);

	als = lightsensor_get_alsvalue(pas2m110);

	for (i = 0; ARRAY_SIZE(adc_table); i++)
		if (als <= adc_table[i])
			break;

	if (pas2m110->light_buffer == i) {
		if (pas2m110->light_count++ == LIGHT_BUFFER_NUM) {
			input_report_abs(pas2m110->light_input_dev,
							ABS_MISC, als + 1);
			input_sync(pas2m110->light_input_dev);
			pas2m110->light_count = 0;
		}
	} else {
		pas2m110->light_buffer = i;
		pas2m110->light_count = 0;
	}
}

static void pas2m110_work_func_prox(struct work_struct *work)
{
	struct pas2m110_data *pas2m110 = container_of(work,
				struct pas2m110_data, work_prox);
	proxsensor_get_avgvalue(pas2m110);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart pas2m110_light_timer_func(struct hrtimer *timer)
{
	struct pas2m110_data *pas2m110 = container_of(timer,
				struct pas2m110_data, light_timer);
	queue_work(pas2m110->light_wq, &pas2m110->work_light);
	hrtimer_forward_now(&pas2m110->light_timer, pas2m110->light_poll_delay);
	return HRTIMER_RESTART;
}

static enum hrtimer_restart pas2m110_prox_timer_func(struct hrtimer *timer)
{
	struct pas2m110_data *pas2m110
			= container_of(timer, struct pas2m110_data, prox_timer);
	queue_work(pas2m110->prox_wq, &pas2m110->work_prox);
	hrtimer_forward_now(&pas2m110->prox_timer, pas2m110->prox_poll_delay);
	return HRTIMER_RESTART;
}

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t pas2m110_irq_thread_fn(int irq, void *data)
{
	struct pas2m110_data *ip = data;
	u8 val = 1;
	u8 tmp;

	val = gpio_get_value(ip->i2c_client->irq);
	if (val < 0) {
		pr_err("%s: gpio_get_value error %d\n", __func__, val);
		return IRQ_HANDLED;
	}

	/* for debugging : going to be removed */
	pas2m110_i2c_read(ip, REGS_PS_DATA, &tmp);
	if (tmp == 252)
		tmp = 0;
	pr_err("%s: proximity value = %d, val = %d\n", __func__, tmp, val);


	/* 0 is close, 1 is far */
	input_report_abs(ip->proximity_input_dev, ABS_DISTANCE, val);
	input_sync(ip->proximity_input_dev);
	wake_lock_timeout(&ip->prx_wake_lock, 3*HZ);

	return IRQ_HANDLED;
}

static int pas2m110_setup_irq(struct pas2m110_data *pas2m110)
{
	int rc = -EIO;
	int irq;

	irq = gpio_to_irq(pas2m110->i2c_client->irq);
	rc = request_threaded_irq(irq, NULL, pas2m110_irq_thread_fn,
			 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			 "proximity_int", pas2m110);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq, irq, rc);
		return rc;
	}

	/* start with interrupts disabled */
	disable_irq(irq);
	pas2m110->irq = irq;

	return rc;
}

static int pas2m110_setup_reg(struct pas2m110_data *pas2m110)
{
	int err = 0;
	u8 tmp;
	/* initializing the proximity and light sensor registers */
	mutex_lock(&pas2m110->power_lock);
	pas2m110->pdata->proximity_power(1);
	pas2m110_i2c_write(pas2m110, REGS_PS2_CMD, reg_defaults[1]|0x04);
	pas2m110_i2c_write(pas2m110, REGS_PS1_CMD, reg_defaults[0]);
/*	pas2m110_i2c_write(pas2m110, REGS_PS2_CMD, reg_defaults[1]); */
	mutex_unlock(&pas2m110->power_lock);

	/* printing the inital proximity value with no contact */
	msleep(50);
	err = pas2m110_i2c_read(pas2m110, REGS_PS_DATA, &tmp);
	if (tmp == 252)
		tmp = 0;

	/* while(1) msleep(100); */

	if (err < 0) {
		pr_err("%s: read ps_data failed\n", __func__);
		err = -EIO;
	}
	pr_err("%s: initial proximity value = %d\n", __func__, tmp);
	mutex_lock(&pas2m110->power_lock);
	pas2m110_i2c_write(pas2m110,
			REGS_PS2_CMD, reg_defaults[1]|0x04);
	/* PS sleep */
	pas2m110_i2c_write(pas2m110,
	REGS_PS1_CMD, reg_defaults[0]&(0x01^0xFF));
	pas2m110->pdata->proximity_power(0);
	mutex_unlock(&pas2m110->power_lock);

	return err;
}

static int pas2m110_i2c_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct input_dev *input_dev;
	struct pas2m110_data *pas2m110;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	pas2m110 = kzalloc(sizeof(struct pas2m110_data), GFP_KERNEL);
	if (!pas2m110) {
		pr_err("%s: failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	pas2m110->pdata = client->dev.platform_data;
	pas2m110->i2c_client = client;
	i2c_set_clientdata(client, pas2m110);

	/* wake lock init */
	wake_lock_init(&pas2m110->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&pas2m110->power_lock);

	/* setup initial registers */
	ret = pas2m110_setup_reg(pas2m110);
	if (ret < 0) {
		pr_err("%s: could not setup regs\n", __func__);
		goto err_setup_reg;
	}

	ret = pas2m110_setup_irq(pas2m110);
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
	pas2m110->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, pas2m110);
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
	hrtimer_init(&pas2m110->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pas2m110->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	pas2m110->light_timer.function = pas2m110_light_timer_func;

	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&pas2m110->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pas2m110->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);
	pas2m110->prox_timer.function = pas2m110_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	pas2m110->light_wq = create_singlethread_workqueue("pas2m110_light_wq");
	if (!pas2m110->light_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create light workqueue\n", __func__);
		goto err_create_light_workqueue;
	}
	pas2m110->prox_wq = create_singlethread_workqueue("pas2m110_prox_wq");
	if (!pas2m110->prox_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create prox workqueue\n", __func__);
		goto err_create_prox_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&pas2m110->work_light, pas2m110_work_func_light);
	INIT_WORK(&pas2m110->work_prox, pas2m110_work_func_prox);

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, pas2m110);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	pas2m110->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* set sysfs for proximity sensor */
	pas2m110->proximity_class = class_create(THIS_MODULE, "proximity");
	if (IS_ERR(pas2m110->proximity_class)) {
		pr_err("%s: could not create proximity_class\n", __func__);
		goto err_proximity_class_create;
	}

	pas2m110->proximity_dev = device_create(pas2m110->proximity_class,
						NULL, 0, NULL, "proximity");
	if (IS_ERR(pas2m110->proximity_dev)) {
		pr_err("%s: could not create proximity_dev\n", __func__);
		goto err_proximity_device_create;
	}

	if (device_create_file(pas2m110->proximity_dev,
		&dev_attr_proximity_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_proximity_state.attr.name);
		goto err_proximity_device_create_file1;
	}

	if (device_create_file(pas2m110->proximity_dev,
		&dev_attr_proximity_avg) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_proximity_avg.attr.name);
		goto err_proximity_device_create_file2;
	}
	dev_set_drvdata(pas2m110->proximity_dev, pas2m110);

	/* set sysfs for light sensor */
	pas2m110->lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(pas2m110->lightsensor_class)) {
		pr_err("%s: could not create lightsensor_class\n", __func__);
		goto err_light_class_create;
	}

	pas2m110->switch_cmd_dev = device_create(pas2m110->lightsensor_class,
						NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(pas2m110->switch_cmd_dev)) {
		pr_err("%s: could not create switch_cmd_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(pas2m110->switch_cmd_dev,
		&dev_attr_lightsensor_file_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			dev_attr_lightsensor_file_state.attr.name);
		goto err_light_device_create_file;
	}
	dev_set_drvdata(pas2m110->switch_cmd_dev, pas2m110);

	goto done;

/* error, unwind it all */
err_light_device_create_file:
	device_destroy(pas2m110->lightsensor_class, 0);
err_light_device_create:
	class_destroy(pas2m110->lightsensor_class);
err_light_class_create:
	device_remove_file(pas2m110->proximity_dev, &dev_attr_proximity_avg);
err_proximity_device_create_file2:
	device_remove_file(pas2m110->proximity_dev, &dev_attr_proximity_state);
err_proximity_device_create_file1:
	device_destroy(pas2m110->proximity_class, 0);
err_proximity_device_create:
	class_destroy(pas2m110->proximity_class);
err_proximity_class_create:
	sysfs_remove_group(&input_dev->dev.kobj,
			&light_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(pas2m110->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(pas2m110->prox_wq);
err_create_prox_workqueue:
	destroy_workqueue(pas2m110->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&pas2m110->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(pas2m110->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
	free_irq(pas2m110->irq, 0);
err_setup_irq:
err_setup_reg:
	mutex_destroy(&pas2m110->power_lock);
	wake_lock_destroy(&pas2m110->prx_wake_lock);
	kfree(pas2m110);
done:
	return ret;
}

static int pas2m110_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   pas2m110->power_state because we use that state in resume.
	*/
	struct i2c_client *client = to_i2c_client(dev);
	struct pas2m110_data *pas2m110 = i2c_get_clientdata(client);
	if (pas2m110->power_state & LIGHT_ENABLED)
		pas2m110_light_disable(pas2m110);
	return 0;
}

static int pas2m110_resume(struct device *dev)
{
	/* Turn power back on if we were before suspend. */
	struct i2c_client *client = to_i2c_client(dev);
	struct pas2m110_data *pas2m110 = i2c_get_clientdata(client);
	if (pas2m110->power_state & LIGHT_ENABLED)
		pas2m110_light_enable(pas2m110);
	return 0;
}

static int pas2m110_i2c_remove(struct i2c_client *client)
{
	struct pas2m110_data *pas2m110 = i2c_get_clientdata(client);

	device_remove_file(pas2m110->proximity_dev,
				&dev_attr_proximity_avg);
	device_remove_file(pas2m110->switch_cmd_dev,
				&dev_attr_lightsensor_file_state);
	device_destroy(pas2m110->lightsensor_class, 0);
	class_destroy(pas2m110->lightsensor_class);
	device_remove_file(pas2m110->proximity_dev, &dev_attr_proximity_avg);
	device_remove_file(pas2m110->proximity_dev, &dev_attr_proximity_state);
	device_destroy(pas2m110->proximity_class, 0);
	class_destroy(pas2m110->proximity_class);
	sysfs_remove_group(&pas2m110->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(pas2m110->light_input_dev);
	sysfs_remove_group(&pas2m110->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(pas2m110->proximity_input_dev);
	free_irq(pas2m110->irq, NULL);
	if (pas2m110->power_state) {
		if (pas2m110->power_state & LIGHT_ENABLED)
			pas2m110_light_disable(pas2m110);
		if (pas2m110->power_state & PROXIMITY_ENABLED) {
			/* cm3663_i2c_write(cm3663, REGS_PS_CMD, 0x01); */
			pas2m110_i2c_write(pas2m110,
				REGS_PS2_CMD, reg_defaults[1]|0x04);
	/* PS sleep */
		pas2m110_i2c_write(pas2m110,
				REGS_PS1_CMD, reg_defaults[0]&(0x01^0xFF));
			pas2m110->pdata->proximity_power(0);
		}
	}
	destroy_workqueue(pas2m110->light_wq);
	destroy_workqueue(pas2m110->prox_wq);
	mutex_destroy(&pas2m110->power_lock);
	wake_lock_destroy(&pas2m110->prx_wake_lock);
	kfree(pas2m110);
	return 0;
}

static const struct i2c_device_id pas2m110_device_id[] = {
	{"pas2m110", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, pas2m110_device_id);

static const struct dev_pm_ops pas2m110_pm_ops = {
	.suspend = pas2m110_suspend,
	.resume = pas2m110_resume
};

static struct i2c_driver pas2m110_i2c_driver = {
	.driver = {
		.name = "pas2m110",
		.owner = THIS_MODULE,
		.pm = &pas2m110_pm_ops
	},
	.probe		= pas2m110_i2c_probe,
	.remove		= pas2m110_i2c_remove,
	.id_table	= pas2m110_device_id,
};


static int __init pas2m110_init(void)
{
	return i2c_add_driver(&pas2m110_i2c_driver);
}

static void __exit pas2m110_exit(void)
{
	i2c_del_driver(&pas2m110_i2c_driver);
}

module_init(pas2m110_init);
module_exit(pas2m110_exit);

MODULE_AUTHOR("tim.sk.lee@samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for pas2m110");
MODULE_LICENSE("GPL");
