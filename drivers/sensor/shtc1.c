/* driver/sensor/shtc1.c
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

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sensor/shtc1.h>
#include <linux/sensor/sensors_core.h>

#define VENDOR		"SENSIRION"
#define CHIP_ID		"SHTC1"
#define MODEL_NAME	"EI-CN10"

#define I2C_M_WR	0 /* for i2c Write */
#define I2c_M_RD	1 /* for i2c Read */

#define EVENT_TYPE_TEMPERATURE	REL_HWHEEL
#define EVENT_TYPE_HUMIDITY		REL_DIAL

#define SHTC1_CMD_LENGTH      2

/* register addresses */

 /* driver data */
struct shtc1_data {
	struct i2c_client *i2c_client;
	struct input_dev *shtc1_input_dev;
	struct shtc1_platform_data *pdata;
	struct mutex power_lock;
	struct mutex read_lock;
	struct hrtimer shtc1_timer;
	struct workqueue_struct *shtc1_wq;
	struct work_struct work_shtc1;
	struct device *shtc1_dev;
	ktime_t shtc1_poll_delay;
	u8 enabled;
	int crc_result;
	int temperature;
	int humidity;
	char *comp_engine_ver;
};

static const uint8_t CMD_READ_ID_REG[]    = {0xef, 0xc8};

const uint8_t CRC_POLYNOMIAL = 0x31;
const uint8_t CRC_INIT       = 0xff;

static bool shtc1_check_crc(uint8_t *data, uint8_t data_length, uint8_t checksum)
{
	uint8_t crc = CRC_INIT;	
	uint8_t current_byte;
	uint8_t bit;
	//calculates 8-Bit checksum with given polynomial
	for (current_byte = 0; current_byte < data_length; ++current_byte)
	{
		crc ^= (data[current_byte]);
		for (bit = 8; bit > 0; --bit)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}
	return crc == checksum;
}

int shtc1_i2c_read_byte(struct shtc1_data *shtc1, u8 addr, u8 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_msg msg[1];
	struct i2c_client *client = shtc1->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msg->addr = addr >> 1;
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = val;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}
	pr_err("%s: i2c read failed at addr 0x%x: %d\n", __func__, addr, err);
	return err;
}

int shtc1_i2c_read_word(struct shtc1_data *shtc1,
			u8 addr, u8 command, u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = shtc1->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		/* send slave address & command */
		msg[0].addr = addr>>1;
		msg[0].flags = I2C_M_WR;
		msg[0].len = 1;
		msg[0].buf = &command;

		/* read word data */
		msg[1].addr = addr>>1;
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

int shtc1_i2c_write_byte(struct shtc1_data *shtc1,
			u8 addr, u8 command, u8 val)
{
	int err = 0;
	struct i2c_client *client = shtc1->i2c_client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = command;
		data[1] = val;

		/* send slave address & command */
		msg->addr = addr>>1;
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

static void shtc1_update_measurements(struct shtc1_data *shtc1,
			int *temperature, int *humidity, int *crc)
{
	int val;
	unsigned char data[6];
	unsigned char cmd[2] = {0x64, 0x58};
	i2c_master_send(shtc1->i2c_client, cmd, 2);
	msleep(50);
	i2c_master_recv(shtc1->i2c_client, data, 6);

	if (!shtc1_check_crc(data, 2, data[2]) ||
		!shtc1_check_crc(data+3, 2, data[5])) {
		pr_info("%s : crc check fail\n", __func__);
		*crc = 0;
		return;
	}
	*crc = 1;
	val = be16_to_cpup((__be16 *)data);
	*temperature = ((21875 * val) >> 13) - 45000;
	val = be16_to_cpup((__be16 *)(data+3));
	*humidity = ((15000 * val) >> 13) - 10000;
}

static void shtc1_enable(struct shtc1_data *shtc1)
{
	/* enable setting */
	pr_info("%s: starting poll timer, delay %lldns\n",
				__func__, ktime_to_ns(shtc1->shtc1_poll_delay));
	hrtimer_start(&shtc1->shtc1_timer, shtc1->shtc1_poll_delay,
					HRTIMER_MODE_REL);
}

static void shtc1_disable(struct shtc1_data *shtc1)
{
	/* disable setting */
	pr_info("%s: cancelling poll timer\n", __func__);
	hrtimer_cancel(&shtc1->shtc1_timer);
	cancel_work_sync(&shtc1->work_shtc1);
}

/* sysfs */
static ssize_t shtc1_poll_delay_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(shtc1->shtc1_poll_delay));
}

static ssize_t shtc1_poll_delay_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&shtc1->power_lock);
	if (new_delay != ktime_to_ns(shtc1->shtc1_poll_delay)) {
		shtc1->shtc1_poll_delay = ns_to_ktime(new_delay);
		if (shtc1->enabled) {
			shtc1_disable(shtc1);
			shtc1_enable(shtc1);
		}
		pr_info("%s, poll_delay = %lld\n", __func__, new_delay);
	}
	mutex_unlock(&shtc1->power_lock);

	return size;
}

static ssize_t shtc1_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&shtc1->power_lock);
	pr_info("%s: new_value = %d\n", __func__, new_value);
	if (new_value && !shtc1->enabled) {
		shtc1->enabled = 1;
		shtc1_enable(shtc1);
	} else if (!new_value && shtc1->enabled) {
		shtc1_disable(shtc1);
		shtc1->enabled = 0;
	}
	mutex_unlock(&shtc1->power_lock);
	return size;
}

static ssize_t shtc1_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", shtc1->enabled);
}

static ssize_t shtc1_raw_data_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);
	int temperature = 0, humidity = 0, crc = 0;

	mutex_lock(&shtc1->read_lock);
	shtc1_update_measurements(shtc1, &temperature, &humidity, &crc);
	mutex_unlock(&shtc1->read_lock);
	return sprintf(buf, "%d, %d\n", temperature, humidity);
}

static ssize_t shtc1_crc_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);
	int temperature = 0, humidity = 0, crc = 0;

	if (shtc1->enabled)
		return sprintf(buf, "%s\n", (shtc1->crc_result)? "OK":"NG");
	else {
		mutex_lock(&shtc1->read_lock);
		shtc1_update_measurements(shtc1, &temperature,
				&humidity, &crc);
		mutex_unlock(&shtc1->read_lock);
		return sprintf(buf, "%s\n", (crc)? "OK":"NG");
	}
}

static ssize_t shtc1_engine_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);

	pr_info("%s - engine_ver = %s_%s\n",
		__func__, MODEL_NAME, shtc1->comp_engine_ver);

	return sprintf(buf, "%s_%s\n",
		MODEL_NAME, shtc1->comp_engine_ver);
}

static ssize_t shtc1_engine_version_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);

	kfree(shtc1->comp_engine_ver);
	shtc1->comp_engine_ver =
		    kzalloc(((strlen(buf)+1) * sizeof(char)), GFP_KERNEL);
	strncpy(shtc1->comp_engine_ver, buf, strlen(buf)+1);
	pr_info("%s - engine_ver = %s, %s\n",
		__func__, shtc1->comp_engine_ver, buf);

	return size;
}

static DEVICE_ATTR(raw_data, S_IRUGO, shtc1_raw_data_show, NULL);
static DEVICE_ATTR(crc_check, S_IRUGO, shtc1_crc_check_show, NULL);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			shtc1_poll_delay_show, shtc1_poll_delay_store);
static DEVICE_ATTR(engine_ver, S_IRUGO | S_IWUSR | S_IWGRP,
			shtc1_engine_version_show, shtc1_engine_version_store);

/* sysfs for vendor & name */
static ssize_t shtc1_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t shtc1_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(vendor, 0644, shtc1_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, shtc1_name_show, NULL);

static struct device_attribute dev_attr_shtc1_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	shtc1_enable_show, shtc1_enable_store);

static struct attribute *shtc1_sysfs_attrs[] = {
	&dev_attr_shtc1_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group shtc1_attribute_group = {
	.attrs = shtc1_sysfs_attrs,
};

/* This function is for shtc1 sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart shtc1_shtc1_timer_func(struct hrtimer *timer)
{
	struct shtc1_data *shtc1
	    = container_of(timer, struct shtc1_data, shtc1_timer);

	queue_work(shtc1->shtc1_wq, &shtc1->work_shtc1);
	hrtimer_forward_now(&shtc1->shtc1_timer, shtc1->shtc1_poll_delay);
	return HRTIMER_RESTART;
}

static void shtc1_work_func_shtc1(struct work_struct *work)
{
	struct shtc1_data *shtc1 = container_of(work, struct shtc1_data,
						    work_shtc1);

	mutex_lock(&shtc1->read_lock);
	shtc1_update_measurements(shtc1, &shtc1->temperature,
					&shtc1->humidity,
					&shtc1->crc_result);
	mutex_unlock(&shtc1->read_lock);


	input_report_rel(shtc1->shtc1_input_dev, EVENT_TYPE_TEMPERATURE,
						shtc1->temperature);
	input_report_rel(shtc1->shtc1_input_dev, EVENT_TYPE_HUMIDITY,
						shtc1->humidity);
	input_sync(shtc1->shtc1_input_dev);

	pr_info("%s: temperature = %d, humidity = %d\n",
			__func__, shtc1->temperature, shtc1->humidity);
}

static int shtc1_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct shtc1_data *shtc1 = NULL;

	pr_info("%s is called.\n", __func__);

	/* Check i2c functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	shtc1 = kzalloc(sizeof(struct shtc1_data), GFP_KERNEL);
	if (!shtc1) {
		pr_err("%s: failed to alloc memory for shtc1_data\n", __func__);
		return -ENOMEM;
	}

	shtc1->pdata = client->dev.platform_data;
	shtc1->i2c_client = client;
	i2c_set_clientdata(client, shtc1);
	mutex_init(&shtc1->power_lock);
	mutex_init(&shtc1->read_lock);

	/* Check if the device is there or not. */
	ret = i2c_master_send(shtc1->i2c_client,
				CMD_READ_ID_REG, SHTC1_CMD_LENGTH);
	if (ret < 0) {
		pr_err("%s: failed i2c_master_send (err = %d)\n", __func__, ret);
		/* goto err_i2c_master_send_shtc1;*/
	}

	/* allocate shtc1 input_device */
	shtc1->shtc1_input_dev = input_allocate_device();
	if (!shtc1->shtc1_input_dev) {
		pr_err("%s: could not allocate shtc1 input device\n", __func__);
		goto err_input_allocate_device_shtc1;
	}

	input_set_drvdata(shtc1->shtc1_input_dev, shtc1);
	shtc1->shtc1_input_dev->name = "temp_humidity_sensor";
	input_set_capability(shtc1->shtc1_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(shtc1->shtc1_input_dev, EV_REL, REL_DIAL);

	ret = input_register_device(shtc1->shtc1_input_dev);
	if (ret < 0) {
		input_free_device(shtc1->shtc1_input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_shtc1;
	}

	ret = sysfs_create_group(&shtc1->shtc1_input_dev->dev.kobj,
				&shtc1_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_shtc1;
	}

	/* Timer settings. We poll for shtc1 values using a timer. */
	hrtimer_init(&shtc1->shtc1_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	shtc1->shtc1_poll_delay = ns_to_ktime(1000 * NSEC_PER_MSEC);
	shtc1->shtc1_timer.function = shtc1_shtc1_timer_func;

	/* Timer just fires off a work queue request.  We need a thread
	   to read the i2c (can be slow and blocking). */
	shtc1->shtc1_wq = create_singlethread_workqueue("shtc1_temphumidity_wq");
	if (!shtc1->shtc1_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create shtc1 workqueue\n", __func__);
		goto err_create_workqueue_shtc1;
	}

	/* This is the thread function we run on the work queue */
	INIT_WORK(&shtc1->work_shtc1, shtc1_work_func_shtc1);

	/* Set sysfs for shtc1 */
	shtc1->shtc1_dev = sensors_classdev_register("temphumidity_sensor");
	if (IS_ERR(shtc1->shtc1_dev)) {
		pr_err("%s: could not create shtc1_dev\n", __func__);
		goto err_shtc1_class_create;
	}

	if (device_create_file(shtc1->shtc1_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_shtc1_device_create_file1;
	}

	if (device_create_file(shtc1->shtc1_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_shtc1_device_create_file2;
	}

	if (device_create_file(shtc1->shtc1_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_shtc1_device_create_file3;
	}

	if (device_create_file(shtc1->shtc1_dev, &dev_attr_crc_check) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_crc_check.attr.name);
		goto err_shtc1_device_create_file4;
	}

	if (device_create_file(shtc1->shtc1_dev, &dev_attr_engine_ver) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_engine_ver.attr.name);
		goto err_shtc1_device_create_file5;
	}

	dev_set_drvdata(shtc1->shtc1_dev, shtc1);

	pr_info("%s is success.\n", __func__);
	goto done;

/* error, unwind it all */
err_shtc1_device_create_file5:
	device_remove_file(shtc1->shtc1_dev, &dev_attr_crc_check);
err_shtc1_device_create_file4:
	device_remove_file(shtc1->shtc1_dev, &dev_attr_raw_data);
err_shtc1_device_create_file3:
	device_remove_file(shtc1->shtc1_dev, &dev_attr_name);
err_shtc1_device_create_file2:
	device_remove_file(shtc1->shtc1_dev, &dev_attr_vendor);
err_shtc1_device_create_file1:
	sensors_classdev_unregister(shtc1->shtc1_dev);
err_shtc1_class_create:
	destroy_workqueue(shtc1->shtc1_wq);
err_create_workqueue_shtc1:
	sysfs_remove_group(&shtc1->shtc1_input_dev->dev.kobj,
				&shtc1_attribute_group);
err_sysfs_create_group_shtc1:
	input_unregister_device(shtc1->shtc1_input_dev);
err_input_register_device_shtc1:
err_input_allocate_device_shtc1:
/*err_i2c_master_send_shtc1:*/
	mutex_destroy(&shtc1->read_lock);
	mutex_destroy(&shtc1->power_lock);
	kfree(shtc1);
done:
	return ret;
}

static int shtc1_i2c_remove(struct i2c_client *client)
{
	struct shtc1_data *shtc1 = i2c_get_clientdata(client);

	if (shtc1->comp_engine_ver != NULL)
		kfree(shtc1->comp_engine_ver);

	/* device off */
	if (shtc1->enabled)
		shtc1_disable(shtc1);

	/* destroy workqueue */
	destroy_workqueue(shtc1->shtc1_wq);

	/* sysfs destroy */
	device_remove_file(shtc1->shtc1_dev, &dev_attr_name);
	device_remove_file(shtc1->shtc1_dev, &dev_attr_vendor);
	device_remove_file(shtc1->shtc1_dev, &dev_attr_raw_data);
	device_remove_file(shtc1->shtc1_dev, &dev_attr_crc_check);
	device_remove_file(shtc1->shtc1_dev, &dev_attr_engine_ver);

	sensors_classdev_unregister(shtc1->shtc1_dev);

	/* input device destroy */
	sysfs_remove_group(&shtc1->shtc1_input_dev->dev.kobj,
				&shtc1_attribute_group);
	input_unregister_device(shtc1->shtc1_input_dev);

	/* lock destroy */
	mutex_destroy(&shtc1->read_lock);
	mutex_destroy(&shtc1->power_lock);
	kfree(shtc1);

	return 0;
}

static int shtc1_suspend(struct device *dev)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);

	if (shtc1->enabled)
		shtc1_disable(shtc1);

	return 0;
}

static int shtc1_resume(struct device *dev)
{
	struct shtc1_data *shtc1 = dev_get_drvdata(dev);

	if (shtc1->enabled)
		shtc1_enable(shtc1);

	return 0;
}

static const struct i2c_device_id shtc1_device_id[] = {
	{"shtc1", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, shtc1_device_id);

static const struct dev_pm_ops shtc1_pm_ops = {
	.suspend = shtc1_suspend,
	.resume = shtc1_resume
};

static struct i2c_driver shtc1_i2c_driver = {
	.driver = {
			.name = "shtc1",
			.owner = THIS_MODULE,
			.pm = &shtc1_pm_ops},
	.probe = shtc1_i2c_probe,
	.remove = shtc1_i2c_remove,
	.id_table = shtc1_device_id,
};

static int __init shtc1_init(void)
{
	return i2c_add_driver(&shtc1_i2c_driver);
}

static void __exit shtc1_exit(void)
{
	i2c_del_driver(&shtc1_i2c_driver);
}

module_init(shtc1_init);
module_exit(shtc1_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("shtc1 Temperature      Sensor & shtc1 sensor device driver");
MODULE_LICENSE("GPL");
