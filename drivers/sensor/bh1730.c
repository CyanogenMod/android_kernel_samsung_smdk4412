/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/bh1730.h>

#define VENDOR_NAME		"ROHM"
#define CHIP_NAME		"BH1730"

#define SENSOR_BH1730FVC_ADDR		0x29

#define BH1730FVC_DRV_NAME	"bh1730fvc"
#define DRIVER_VERSION		"1.0"

#define SET_CMD		0x80

#define LUX_MIN_VALUE		0
#define LUX_MAX_VALUE		65528

#define bh1730fvc_dbmsg(str, args...) pr_debug("%s: " str, __func__, ##args)

enum BH1730FVC_STATE {
	POWER_OFF = 0,
	POWER_ON,
};

#define COL 9

enum BH1730FVC_REG {
	REG_CON = 0x00,
	REG_TIME,
	REG_GAIN = 0x07,
	REG_ID = 0x12,
	REG_DATA0L = 0x14,
	REG_DATA0H,
	REG_DATA1L,
	REG_DATA1H,
};

enum BH1730FVC_CMD {
	CMD_PWR_OFF = 0,
	CMD_PWR_ON,
	CMD_TIME_50MS,
	CMD_TIME_100MS,
	CMD_TIME_120MS,
	CMD_TIME_200MS,
	CMD_GAIN_X2,
	CMD_GAIN_X64,
	CMD_GAIN_X128,
};
enum BH1730FVC_I2C_IF {
	REG = 0,
	CMD,
};

static const u8 cmds[COL][2] = {
	{REG_CON, 0x00}, /* Power Down */
	{REG_CON, 0x03}, /* Power On */
	{REG_TIME, 0xED}, /* Timing 50ms */
	{REG_TIME, 0xDA}, /* Timing 100ms */
	{REG_TIME, 0xD3}, /* Timing 120ms */
	{REG_TIME, 0xB6}, /* Timing 200ms */
	{REG_GAIN, 0x01}, /* Gain x2 */
	{REG_GAIN, 0x02}, /* Gain x64 */
	{REG_GAIN, 0x03} /* Gain x128 */
};

enum BH1730FVC_GAIN {
	X2 = 0,
	X64,
	X128,
};

enum BH1730FVC_TIME {
	T50MS = 0,
	T100MS,
	T120MS,
	T200MS,
};

static u16 integ_time[4] = {50, 100, 120, 200};
static u16 gain[3] = {2, 64, 128};

enum BH1730FVC_CASE {
	CASE_NORMAL = 1,
	CASE_LOW,
};

struct bh1730fvc_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work_light;
	struct hrtimer timer;
	struct mutex lock;
	struct workqueue_struct *wq;
	struct device *light_dev;
	ktime_t light_poll_delay;
	enum BH1730FVC_STATE state;
	u16 data0;
	u16 data1;
	u32 lux;
	u16 gain;
	u16 time;
	int check_case;
};

static int bh1730fvc_get_luxvalue(struct bh1730fvc_data *bh1730fvc);


static int bh1730fvc_read_byte(struct i2c_client *client, u8 reg, u8 *value)
{
	int err = 0;
	u8 cmd;

	cmd = SET_CMD | reg;

	err = i2c_master_send(client, &cmd, 1);
	if (err < 0)
		pr_err("%s: i2c_master_send fail err=%d\n", __func__, err);
	else {
		err = i2c_master_recv(client, value, 1);
		if (err != 1)
			pr_err("%s: i2c_master_recv fail err=%d\n",
				__func__, err);
	}

	return err;
}

static int bh1730fvc_read_data(struct i2c_client *client, u16 *value)
{
	int err = 0;
	u8 cmd;
	u8 data[4];

	cmd = SET_CMD | REG_DATA0L;

	err = i2c_master_send(client, &cmd, 1);
	if (err < 0)
		pr_err("%s: i2c_master_send fail err=%d\n", __func__, err);
	else {
		err = i2c_master_recv(client, (u8 *)data, 4);
		if (err != sizeof(u8) * 4)
			pr_err("%s: i2c_master_recv fail err=%d\n",
				__func__, err);
		else {
			value[0] = ((data[1] << 8) | data[0]);
			value[1] = ((data[3] << 8) | data[2]);
		}
	}

	return err;
}


static int bh1730fvc_write_byte(struct i2c_client *client, u8 reg, u8 value)
{
	int err = 0;
	u8 cmd[2];

	cmd[0] = SET_CMD | reg;
	cmd[1] = value;

	err = i2c_master_send(client, cmd, 2);
	if (err < 0)
		pr_err("%s: i2c_master_send fail err=%d\n", __func__, err);

	return err;
}

static int bh1730fvc_enable(struct bh1730fvc_data *bh1730fvc)
{
	int err;

	bh1730fvc_dbmsg("starting poll timer, delay %lldns\n",
			ktime_to_ns(bh1730fvc->light_poll_delay));

	err = bh1730fvc_write_byte(bh1730fvc->client, cmds[CMD_PWR_ON][REG],
			cmds[CMD_PWR_ON][CMD]);
	if (unlikely(err < 0)) {
		pr_err("%s: Failed to write byte (POWER_ON)\n", __func__);
		goto done;
	}

	msleep(150);

	hrtimer_start(&bh1730fvc->timer, bh1730fvc->light_poll_delay,
		HRTIMER_MODE_REL);
done:
	return err;
}

static int bh1730fvc_disable(struct bh1730fvc_data *bh1730fvc)
{
	int err;

	hrtimer_cancel(&bh1730fvc->timer);
	cancel_work_sync(&bh1730fvc->work_light);
	err = bh1730fvc_write_byte(bh1730fvc->client, cmds[CMD_PWR_OFF][REG],
			cmds[CMD_PWR_OFF][CMD]);
	if (unlikely(err < 0))
		pr_err("%s: Failed to write byte (POWER_OFF)\n", __func__);

	return err;
}

static ssize_t bh1730fvc_poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct bh1730fvc_data *bh1730fvc = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n",
		ktime_to_ns(bh1730fvc->light_poll_delay));
}

static ssize_t bh1730fvc_poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int err;
	int64_t new_delay;
	struct bh1730fvc_data *bh1730fvc = dev_get_drvdata(dev);

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	bh1730fvc_dbmsg("new delay = %lldns, old delay = %lldns\n",
			new_delay, ktime_to_ns(bh1730fvc->light_poll_delay));

	mutex_lock(&bh1730fvc->lock);
	if (new_delay != ktime_to_ns(bh1730fvc->light_poll_delay)) {
		bh1730fvc->light_poll_delay = ns_to_ktime(new_delay);
		if (bh1730fvc->state) {
			bh1730fvc_disable(bh1730fvc);
			bh1730fvc_enable(bh1730fvc);
		}

	}
	mutex_unlock(&bh1730fvc->lock);

	return size;
}

static ssize_t bh1730fvc_light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct bh1730fvc_data *bh1730fvc = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", bh1730fvc->state);
}

static ssize_t bh1730fvc_light_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int err = 0;
	bool new_value = false;
	struct bh1730fvc_data *bh1730fvc = dev_get_drvdata(dev);

	bh1730fvc_dbmsg("enable %s\n", buf);

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	bh1730fvc_dbmsg("new_value = %d, old state = %d\n",
			new_value, bh1730fvc->state);

	mutex_lock(&bh1730fvc->lock);

	bh1730fvc->check_case = CASE_NORMAL;

	if (new_value && (!bh1730fvc->state)) {
		err = bh1730fvc_enable(bh1730fvc);
		if (err < 0) {
			pr_err("%s: couldn't enable", __func__);
			bh1730fvc->state = POWER_OFF;
		} else {
			bh1730fvc->state = POWER_ON;
		}
	} else if (!new_value && bh1730fvc->state) {
		err = bh1730fvc_disable(bh1730fvc);
		if (err < 0)
			pr_err("%s: couldn't disable", __func__);
		else
			bh1730fvc->state = POWER_OFF;
	} else
		bh1730fvc_dbmsg("nothing\n");

	mutex_unlock(&bh1730fvc->lock);

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		bh1730fvc_poll_delay_show, bh1730fvc_poll_delay_store);

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		bh1730fvc_light_enable_show, bh1730fvc_light_enable_store);

static struct attribute *bh1730fvc_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group bh1730fvc_attribute_group = {
	.attrs = bh1730fvc_sysfs_attrs,
};

static ssize_t factory_file_illuminance_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int err;
	struct bh1730fvc_data *bh1730fvc = dev_get_drvdata(dev);

	if (bh1730fvc->state == POWER_OFF) {
		err = bh1730fvc_write_byte(bh1730fvc->client,
				cmds[CMD_PWR_ON][REG], cmds[CMD_PWR_ON][CMD]);
		if (err < 0)
			pr_err("%s: i2c write fail err =%d\n",
			__func__, err);

		msleep(210);

		/* raw data */
		 bh1730fvc_get_luxvalue(bh1730fvc);

		bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_PWR_OFF][REG], cmds[CMD_PWR_OFF][CMD]);
	}
	return sprintf(buf, "%u,%u\n", bh1730fvc->data0, bh1730fvc->data1);
}

static ssize_t get_vendor_name(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR_NAME);
}

static ssize_t get_chip_name(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_NAME);
}

static DEVICE_ATTR(raw_data, 0644, factory_file_illuminance_show, NULL);
static DEVICE_ATTR(vendor, 0644, get_vendor_name, NULL);
static DEVICE_ATTR(name, 0644, get_chip_name, NULL);

static void bh1730fvc_set_gain_timing(struct bh1730fvc_data *bh1730fvc, int num)
{
	switch (num) {
	case CASE_NORMAL:
		bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_GAIN_X2][REG], cmds[CMD_GAIN_X2][CMD]);
		bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_TIME_120MS][REG], cmds[CMD_TIME_120MS][CMD]);
		bh1730fvc->gain = gain[X2];
		bh1730fvc->time = integ_time[T120MS];
		break;
	case CASE_LOW:
		bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_GAIN_X64][REG], cmds[CMD_GAIN_X64][CMD]);
		bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_TIME_100MS][REG], cmds[CMD_TIME_100MS][CMD]);
		bh1730fvc->gain = gain[X64];
		bh1730fvc->time = integ_time[T100MS];
		break;
	default:
		pr_err("%s: invalid value %d\n", __func__, num);
	}
	msleep(bh1730fvc->time);
}

static void bh1730fvc_get_raw_data(struct bh1730fvc_data *bh1730fvc)
{
	u16 raw_data[2] = {0,};
	int err = 0;

	/* get raw data */
	err = bh1730fvc_read_data(bh1730fvc->client, raw_data);
	if (err != sizeof(u8) * 4) {
		pr_err("%s : failed to read\n", __func__);
		err = -EIO;
	}
	pr_debug("%s: data0 %d, data1 %d", __func__,
		raw_data[0], raw_data[1]);
	bh1730fvc->data0 = raw_data[0];
	bh1730fvc->data1 = raw_data[1];
}

static int bh1730fvc_get_luxvalue(struct bh1730fvc_data *bh1730fvc)
{
	int retry = 2;
	bool fixed = false;
	bool check_low = false;

	do {
		switch (bh1730fvc->check_case) {
		case CASE_NORMAL:
			bh1730fvc_set_gain_timing(bh1730fvc, CASE_NORMAL);
			bh1730fvc_get_raw_data(bh1730fvc);
			if (check_low)
				fixed = true;
			else {
				if (bh1730fvc->data0 < 80 &&
					bh1730fvc->data1 < 1000)
					bh1730fvc->check_case = CASE_LOW;
				else if (bh1730fvc->data0 < 1000 &&
					bh1730fvc->data1 < 80)
					bh1730fvc->check_case = CASE_LOW;
				else
					fixed = true;
			}
			break;
		case CASE_LOW:
			bh1730fvc_set_gain_timing(bh1730fvc, CASE_LOW);
			bh1730fvc_get_raw_data(bh1730fvc);
			if (bh1730fvc->data0 < 32000 &&
				bh1730fvc->data1 < 32000)
				fixed = true;
			else {
				bh1730fvc->check_case = CASE_NORMAL;
				check_low = true;
			}
			break;
		default:
			pr_err("%s: invalid value %d\n", __func__,
				bh1730fvc->check_case);

		}
		if (fixed)
			break;
		if (!retry)
			pr_err("%s: failed fixing data\n", __func__);
	} while (retry-- > 0);

	bh1730fvc_get_raw_data(bh1730fvc);
	return 0;
}


static void bh1730fvc_work_func_light(struct work_struct *work)
{
	int err;
	u32 result = 0;
	struct bh1730fvc_data *bh1730fvc = container_of(work,
					struct bh1730fvc_data, work_light);

	err = bh1730fvc_get_luxvalue(bh1730fvc);
	if (!err) {
		input_report_rel(bh1730fvc->input_dev, REL_MISC,
			bh1730fvc->data0 + 1);
		input_report_rel(bh1730fvc->input_dev, REL_RX,
			bh1730fvc->data1 + 1);
		bh1730fvc_dbmsg("data1 %d, data0 %d\n",
			bh1730fvc->data1, bh1730fvc->data0);
		result = (bh1730fvc->gain << 16) | bh1730fvc->time;
		bh1730fvc_dbmsg("gain %d, time %d\n",
			bh1730fvc->gain, bh1730fvc->time);
		input_report_rel(bh1730fvc->input_dev, REL_HWHEEL, result);
		input_sync(bh1730fvc->input_dev);
	} else
		pr_err("%s: read word failed! (errno=%d)\n",
			__func__, err);
}

static enum hrtimer_restart bh1730fvc_timer_func(struct hrtimer *timer)
{
	struct bh1730fvc_data *bh1730fvc = container_of(timer,
		struct bh1730fvc_data, timer);

	queue_work(bh1730fvc->wq, &bh1730fvc->work_light);
	hrtimer_forward_now(&bh1730fvc->timer, bh1730fvc->light_poll_delay);
	return HRTIMER_RESTART;
}

int bh1730fvc_test_device(struct bh1730fvc_data *bh1730fvc)
{
	int err = 0;
	u8 reg;

	pr_err("%s : POWER ON %x, %x\n", __func__,
		cmds[CMD_PWR_ON][REG], cmds[CMD_PWR_ON][CMD]);

	err = bh1730fvc_write_byte(bh1730fvc->client,
			cmds[CMD_PWR_ON][REG], cmds[CMD_PWR_ON][CMD]);
	if (err < 0)
		goto err_exit;

	err = bh1730fvc_read_byte(bh1730fvc->client,
			REG_ID, &reg);
	if (err != sizeof(reg)) {
		pr_err("%s : failed to read\n", __func__);
		err = -EIO;
		goto err_exit;
	}

	pr_info("%s : PART_ID_REG = %x\n", __func__, reg);

	err = bh1730fvc_write_byte(bh1730fvc->client,
		cmds[CMD_PWR_OFF][REG], cmds[CMD_PWR_OFF][CMD]);
	if (err < 0)
		pr_err("%s: Failed to write byte (CMD_PWR_OFF)\n", __func__);

err_exit:
	return err;
}

static int __devinit bh1730fvc_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	int err = 0;
	struct bh1730fvc_data *bh1730fvc;
	struct input_dev *input_dev;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("%s: is started!\n", __func__);
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	bh1730fvc = kzalloc(sizeof(*bh1730fvc), GFP_KERNEL);
	if (!bh1730fvc) {
		pr_err("%s, failed to alloc memory for module data\n",
			__func__);
		return -ENOMEM;
	}

	bh1730fvc->client = client;
	i2c_set_clientdata(client, bh1730fvc);

	mutex_init(&bh1730fvc->lock);
	bh1730fvc->state = POWER_OFF;
	bh1730fvc->check_case = CASE_NORMAL;

/*
	err = bh1730fvc_test_device(bh1730fvc);
	if (err < 0) {
		pr_err("%s: No search bh1730fvc lightsensor!\n", __func__);
		goto err_test_lightsensor;
	} else {
		pr_info("%s: find chip : %d\n", __func__, err);
	}
*/
	hrtimer_init(&bh1730fvc->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	bh1730fvc->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	bh1730fvc->timer.function = bh1730fvc_timer_func;

	bh1730fvc->wq = alloc_workqueue("bh1730fvc_wq",
		WQ_UNBOUND | WQ_RESCUER, 1);
	if (!bh1730fvc->wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	INIT_WORK(&bh1730fvc->work_light, bh1730fvc_work_func_light);

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, bh1730fvc);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_REL, REL_MISC);
	input_set_capability(input_dev, EV_REL, REL_RX);
	input_set_capability(input_dev, EV_REL, REL_HWHEEL);
	bh1730fvc_dbmsg("registering lightsensor-level input device\n");

	err = input_register_device(input_dev);
	if (err < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	bh1730fvc->input_dev = input_dev;
	err = sysfs_create_group(&input_dev->dev.kobj,
		&bh1730fvc_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* set sysfs for light sensor */
	bh1730fvc->light_dev = sensors_classdev_register("light_sensor");
	if (IS_ERR(bh1730fvc->light_dev)) {
		pr_err("%s: could not create light_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(bh1730fvc->light_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_light_device_create_file1;
	}

	if (device_create_file(bh1730fvc->light_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_light_device_create_file2;
	}

	if (device_create_file(bh1730fvc->light_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_light_device_create_file3;
	}

	dev_set_drvdata(bh1730fvc->light_dev, bh1730fvc);
	pr_info("%s: success!\n", __func__);


	goto done;

err_light_device_create_file3:
	device_remove_file(bh1730fvc->light_dev, &dev_attr_vendor);
err_light_device_create_file2:
	device_remove_file(bh1730fvc->light_dev, &dev_attr_raw_data);
err_light_device_create_file1:
	sensors_classdev_unregister(bh1730fvc->light_dev);
err_light_device_create:
	sysfs_remove_group(&bh1730fvc->input_dev->dev.kobj,
				&bh1730fvc_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(bh1730fvc->input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(bh1730fvc->wq);
err_create_workqueue:
err_test_lightsensor:
	mutex_destroy(&bh1730fvc->lock);
	kfree(bh1730fvc);
done:
	return err;
}

static int  bh1730fvc_remove(struct i2c_client *client)
{
	struct bh1730fvc_data *bh1730fvc = i2c_get_clientdata(client);

	device_remove_file(bh1730fvc->light_dev, &dev_attr_name);
	device_remove_file(bh1730fvc->light_dev, &dev_attr_vendor);
	device_remove_file(bh1730fvc->light_dev, &dev_attr_raw_data);
	sensors_classdev_unregister(bh1730fvc->light_dev);
	sysfs_remove_group(&bh1730fvc->input_dev->dev.kobj,
				&bh1730fvc_attribute_group);
	input_unregister_device(bh1730fvc->input_dev);

	if (bh1730fvc->state)
		bh1730fvc_disable(bh1730fvc);

	destroy_workqueue(bh1730fvc->wq);
	mutex_destroy(&bh1730fvc->lock);
	kfree(bh1730fvc);

	bh1730fvc_dbmsg("bh1730fvc_remove -\n");
	return 0;
}

static int bh1730fvc_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bh1730fvc_data *bh1730fvc = i2c_get_clientdata(client);

	if (bh1730fvc->state) {
		err = bh1730fvc_disable(bh1730fvc);
		if (err)
			pr_err("%s: could not disable\n", __func__);
	}

	return err;
}

static int bh1730fvc_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bh1730fvc_data *bh1730fvc = i2c_get_clientdata(client);

	bh1730fvc_dbmsg("bh1730fvc_resume +\n");

	if (bh1730fvc->state) {
		err = bh1730fvc_enable(bh1730fvc);
		if (err)
			pr_err("%s: could not enable\n", __func__);
	}

	bh1730fvc_dbmsg("bh1730fvc_resume -\n");
	return err;
}

static const struct i2c_device_id bh1730fvc_id[] = {
	{ BH1730FVC_DRV_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, bh1730fvc_id);

static const struct dev_pm_ops bh1730fvc_pm_ops = {
	.suspend	= bh1730fvc_suspend,
	.resume		= bh1730fvc_resume,
};

static struct i2c_driver bh1730fvc_driver = {
	.driver = {
		.name	= BH1730FVC_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &bh1730fvc_pm_ops,
	},
	.probe		= bh1730fvc_probe,
	.remove		= bh1730fvc_remove,
	.id_table	= bh1730fvc_id,
};

static int __init bh1730fvc_init(void)
{
	return i2c_add_driver(&bh1730fvc_driver);
}
module_init(bh1730fvc_init);

static void __exit bh1730fvc_exit(void)
{
	bh1730fvc_dbmsg("bh1730fvc_exit +\n");
	i2c_del_driver(&bh1730fvc_driver);
	bh1730fvc_dbmsg("bh1730fvc_exit -\n");
}
module_exit(bh1730fvc_exit);

MODULE_AUTHOR("Won Huh <won.huh@samsung.com>");
MODULE_DESCRIPTION("BH1730FVC Ambient light sensor driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
