/* driver/sensor/cm3323.c
 * Copyright (c) 2011 SAMSUNG
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

#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sensor/sensors_core.h>

/* For debugging */
#undef	CM3323_DEBUG

#define	VENDOR		"CAPELLA"
#define	CHIP_ID		"CM3323"

#define I2C_M_WR	0 /* for i2c Write */
#define I2c_M_RD	1 /* for i2c Read */

#define REL_RED		REL_X
#define REL_GREEN	REL_Y
#define REL_BLUE	REL_Z
#define REL_WHITE	REL_MISC

/* register addresses */
/* Ambient light sensor */
#define REG_CS_CONF1	0x00
#define REG_RED		0x08
#define REG_GREEN		0x09
#define REG_BLUE		0x0A
#define REG_WHITE		0x0B

#define ALS_REG_NUM	2

/*lightsnesor log time 6SEC 200mec X 30*/
#define LIGHT_LOG_TIME	30
#define LIGHT_ADD_STARTTIME 300000000
enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* register settings */
static u8 als_reg_setting[ALS_REG_NUM][2] = {
	{REG_CS_CONF1, 0x00},	/* enable */
	{REG_CS_CONF1, 0x01},	/* disable */
};

/* driver data */
struct cm3323_data {
	struct i2c_client *i2c_client;
	struct input_dev *light_input_dev;
	struct mutex power_lock;
	struct mutex read_lock;
	struct hrtimer light_timer;
	struct workqueue_struct *light_wq;
	struct work_struct work_light;
	struct device *light_dev;
	ktime_t light_poll_delay;
	int irq;
	u8 power_state;
	u16 color[4];
	int count_log_time;
};

int cm3323_i2c_read_byte(struct cm3323_data *cm3323,
	u8 command, u8 * val)
{
	int err = 0;
	int retry = 3;
	struct i2c_msg msg[1];
	struct i2c_client *client = cm3323->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msg->addr = client->addr;
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = val;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}
	pr_err("%s: i2c read failed at addr 0x%x: %d\n", __func__,
		client->addr, err);
	return err;
}

int cm3323_i2c_read_word(struct cm3323_data *cm3323, u8 command,
			  u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = cm3323->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		/* send slave address & command */
		msg[0].addr = client->addr;
		msg[0].flags = I2C_M_WR;
		msg[0].len = 1;
		msg[0].buf = &command;

		/* read word data */
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 2;
		msg[1].buf = data;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0) {
			value = (u16)data[1];
			*val = (value << 8) | (u16)data[0];
			return err;
		}
	}
	printk(KERN_ERR "%s, i2c transfer error ret=%d\n", __func__, err);
	return err;
}

int cm3323_i2c_write_byte(struct cm3323_data *cm3323, u8 command,
			   u8 val)
{
	int err = 0;
	struct i2c_client *client = cm3323->i2c_client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = command;
		data[1] = val;

		/* send slave address & command */
		msg->addr = client->addr;
		msg->flags = I2C_M_WR;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	}
	pr_err("%s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

int cm3323_i2c_write_word(struct cm3323_data *cm3323, u8 command,
			   u16 val)
{
	int err = 0;
	struct i2c_client *client = cm3323->i2c_client;
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		err = i2c_smbus_write_word_data(client, command, val);
		if (err >= 0)
			return 0;
	}
	pr_err("%s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

static void cm3323_light_enable(struct cm3323_data *cm3323)
{
	/* enable setting */
	int64_t start_add_time = 0;
	start_add_time = ktime_to_ns(cm3323->light_poll_delay)\
	+ LIGHT_ADD_STARTTIME;
	cm3323_i2c_write_byte(cm3323, REG_CS_CONF1,
		als_reg_setting[0][1]);
	hrtimer_start(&cm3323->light_timer, cm3323->light_poll_delay,
	      HRTIMER_MODE_REL);
}

static void cm3323_light_disable(struct cm3323_data *cm3323)
{
	/* disable setting */
	cm3323_i2c_write_byte(cm3323, REG_CS_CONF1,
		als_reg_setting[1][1]);
	hrtimer_cancel(&cm3323->light_timer);
	cancel_work_sync(&cm3323->work_light);
}

/* sysfs */
static ssize_t cm3323_poll_delay_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(cm3323->light_poll_delay));
}

static ssize_t cm3323_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm3323->power_lock);
	if (new_delay != ktime_to_ns(cm3323->light_poll_delay)) {
		cm3323->light_poll_delay = ns_to_ktime(new_delay);
		if (cm3323->power_state & LIGHT_ENABLED) {
			cm3323_light_disable(cm3323);
			cm3323_light_enable(cm3323);
		}
		pr_info("%s, poll_delay = %lld\n", __func__, new_delay);
	}
	mutex_unlock(&cm3323->power_lock);

	return size;
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm3323->power_lock);
	pr_info("%s,new_value=%d\n", __func__, new_value);
	if (new_value && !(cm3323->power_state & LIGHT_ENABLED)) {
		cm3323->power_state |= LIGHT_ENABLED;
		cm3323_light_enable(cm3323);
	} else if (!new_value && (cm3323->power_state & LIGHT_ENABLED)) {
		cm3323_light_disable(cm3323);
		cm3323->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&cm3323->power_lock);
	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (cm3323->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   cm3323_poll_delay_show, cm3323_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	light_enable_show, light_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

/* light sysfs */
static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm3323->color[0]+1, cm3323->color[1]+1,
		cm3323->color[2]+1, cm3323->color[3]+1);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm3323->color[0]+1, cm3323->color[1]+1,
		cm3323->color[2]+1, cm3323->color[3]+1);
}

static DEVICE_ATTR(lux, 0644, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, 0644, light_data_show, NULL);

/* sysfs for vendor & name */
static ssize_t cm3323_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t cm3323_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}
static DEVICE_ATTR(vendor, 0644, cm3323_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, cm3323_name_show, NULL);

static int cm3323_setup_reg(struct cm3323_data *cm3323)
{
	int err = 0;

	/* ALS initialization */
	err = cm3323_i2c_write_byte(cm3323,
			als_reg_setting[0][0],
			als_reg_setting[0][1]);
	if (err < 0) {
		pr_err("%s: cm3323_als_reg is failed. %d\n", __func__,
			err);
		return err;
	}
	/* turn off */
	cm3323_i2c_write_byte(cm3323, REG_CS_CONF1, 0x01);

	pr_info("%s is success.", __func__);
	return err;
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm3323_light_timer_func(struct hrtimer *timer)
{
	struct cm3323_data *cm3323
	    = container_of(timer, struct cm3323_data, light_timer);
	queue_work(cm3323->light_wq, &cm3323->work_light);
	hrtimer_forward_now(&cm3323->light_timer, cm3323->light_poll_delay);
	return HRTIMER_RESTART;
}

static void cm3323_work_func_light(struct work_struct *work)
{
	struct cm3323_data *cm3323 = container_of(work, struct cm3323_data,
						    work_light);

	mutex_lock(&cm3323->read_lock);
	cm3323_i2c_read_word(cm3323, REG_RED, &cm3323->color[0]);
	cm3323_i2c_read_word(cm3323, REG_GREEN, &cm3323->color[1]);
	cm3323_i2c_read_word(cm3323, REG_BLUE, &cm3323->color[2]);
	cm3323_i2c_read_word(cm3323, REG_WHITE, &cm3323->color[3]);
	mutex_unlock(&cm3323->read_lock);

	input_report_rel(cm3323->light_input_dev, REL_RED,
		cm3323->color[0]+1);
	input_report_rel(cm3323->light_input_dev, REL_GREEN,
		cm3323->color[1]+1);
	input_report_rel(cm3323->light_input_dev, REL_BLUE,
		cm3323->color[2]+1);
	input_report_rel(cm3323->light_input_dev, REL_WHITE,
		cm3323->color[3]+1);
	input_sync(cm3323->light_input_dev);

	if (cm3323->count_log_time >= LIGHT_LOG_TIME) {
		pr_info("%s, red = %u green = %u blue = %u white = %u\n",
			__func__, cm3323->color[0]+1, cm3323->color[1]+1,
			cm3323->color[2]+1, cm3323->color[3]+1);
		cm3323->count_log_time = 0;
	} else
		cm3323->count_log_time++;

#ifdef CM3323_DEBUG
	pr_info("%s, red = %u green = %u blue = %u white = %u\n",
		__func__, cm3323->color[0]+1, cm3323->color[1]+1,
		cm3323->color[2]+1, cm3323->color[3]1);
#endif
}

static int cm3323_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct cm3323_data *cm3323 = NULL;

	pr_info("%s is called.\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	cm3323 = kzalloc(sizeof(struct cm3323_data), GFP_KERNEL);
	if (!cm3323) {
		pr_err
		    ("%s: failed to alloc memory for RGB sensor module data\n",
		     __func__);
		return -ENOMEM;
	}

	cm3323->i2c_client = client;
	i2c_set_clientdata(client, cm3323);
	mutex_init(&cm3323->power_lock);
	mutex_init(&cm3323->read_lock);

	/* Check if the device is there or not. */
	ret = cm3323_setup_reg(cm3323);
	if (ret < 0) {
		pr_err("%s: could not setup regs\n", __func__);
		goto err_setup_reg;
	}

	/* allocate lightsensor input_device */
	cm3323->light_input_dev = input_allocate_device();
	if (!cm3323->light_input_dev) {
		pr_err("%s: could not allocate light input device\n", __func__);
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(cm3323->light_input_dev, cm3323);
	cm3323->light_input_dev->name = "light_sensor";
	input_set_capability(cm3323->light_input_dev, EV_REL, REL_RED);
	input_set_capability(cm3323->light_input_dev, EV_REL, REL_GREEN);
	input_set_capability(cm3323->light_input_dev, EV_REL, REL_BLUE);
	input_set_capability(cm3323->light_input_dev, EV_REL, REL_WHITE);

	ret = input_register_device(cm3323->light_input_dev);
	if (ret < 0) {
		input_free_device(cm3323->light_input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_light;
	}

	ret = sysfs_create_group(&cm3323->light_input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm3323->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm3323->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm3323->light_timer.function = cm3323_light_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm3323->light_wq = create_singlethread_workqueue("cm3323_light_wq");
	if (!cm3323->light_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create light workqueue\n", __func__);
		goto err_create_light_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm3323->work_light, cm3323_work_func_light);

	/* set sysfs for light sensor */
	cm3323->light_dev = sensors_classdev_register("light_sensor");
	if (IS_ERR(cm3323->light_dev)) {
		pr_err("%s: could not create light_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(cm3323->light_dev, &dev_attr_lux) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_lux.attr.name);
		goto err_light_device_create_file1;
	}

	if (device_create_file(cm3323->light_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_light_device_create_file2;
	}

	if (device_create_file(cm3323->light_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_light_device_create_file3;
	}

	if (device_create_file(cm3323->light_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_light_device_create_file4;
	}

	dev_set_drvdata(cm3323->light_dev, cm3323);

	pr_info("%s is success.\n", __func__);
	goto done;

/* error, unwind it all */
err_light_device_create_file4:
	device_remove_file(cm3323->light_dev, &dev_attr_vendor);
err_light_device_create_file3:
	device_remove_file(cm3323->light_dev, &dev_attr_raw_data);
err_light_device_create_file2:
	device_remove_file(cm3323->light_dev, &dev_attr_lux);
err_light_device_create_file1:
	sensors_classdev_unregister(cm3323->light_dev);
err_light_device_create:
	destroy_workqueue(cm3323->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm3323->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(cm3323->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
err_setup_reg:
	mutex_destroy(&cm3323->read_lock);
	mutex_destroy(&cm3323->power_lock);
	kfree(cm3323);
done:
	return ret;
}

static int cm3323_i2c_remove(struct i2c_client *client)
{
	struct cm3323_data *cm3323 = i2c_get_clientdata(client);


	/* device off */
	if (cm3323->power_state & LIGHT_ENABLED)
		cm3323_light_disable(cm3323);

	/* destroy workqueue */
	destroy_workqueue(cm3323->light_wq);

	/* sysfs destroy */
	device_remove_file(cm3323->light_dev, &dev_attr_name);
	device_remove_file(cm3323->light_dev, &dev_attr_vendor);
	device_remove_file(cm3323->light_dev, &dev_attr_raw_data);
	device_remove_file(cm3323->light_dev, &dev_attr_lux);
	sensors_classdev_unregister(cm3323->light_dev);

	/* input device destroy */
	sysfs_remove_group(&cm3323->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(cm3323->light_input_dev);

	/* lock destroy */
	mutex_destroy(&cm3323->read_lock);
	mutex_destroy(&cm3323->power_lock);

	kfree(cm3323);

	return 0;
}

static int cm3323_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   cm3323->power_state because we use that state in resume.
	 */
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);

	if (cm3323->power_state & LIGHT_ENABLED)
		cm3323_light_disable(cm3323);

	return 0;
}

static int cm3323_resume(struct device *dev)
{
	struct cm3323_data *cm3323 = dev_get_drvdata(dev);

	if (cm3323->power_state & LIGHT_ENABLED)
		cm3323_light_enable(cm3323);

	return 0;
}

static const struct i2c_device_id cm3323_device_id[] = {
	{"cm3323", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cm3323_device_id);

static const struct dev_pm_ops cm3323_pm_ops = {
	.suspend = cm3323_suspend,
	.resume = cm3323_resume
};

static struct i2c_driver cm3323_i2c_driver = {
	.driver = {
		   .name = "cm3323",
		   .owner = THIS_MODULE,
		   .pm = &cm3323_pm_ops},
	.probe = cm3323_i2c_probe,
	.remove = cm3323_i2c_remove,
	.id_table = cm3323_device_id,
};

static int __init cm3323_init(void)
{
	return i2c_add_driver(&cm3323_i2c_driver);
}

static void __exit cm3323_exit(void)
{
	i2c_del_driver(&cm3323_i2c_driver);
}

module_init(cm3323_init);
module_exit(cm3323_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("RGB Sensor device driver for cm3323");
MODULE_LICENSE("GPL");
