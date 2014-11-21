/*
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

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sensor/sensors_core.h>

#undef LSC_DBG

#define SENSOR_AL3201_ADDR	0x1c
#define SENSOR_BH1721FVC_ADDR	0x23

#define VENDOR_NAME		"LIGHTON"
#define CHIP_NAME		"AL3201"
#define DRIVER_VERSION		"1.0"

#define LUX_MIN_VALUE		0
#define LUX_MAX_VALUE		65535
#define DARK_COUNT		5

#define AL3201_RAN_COMMAND	0x01
#define AL3201_RAN_MASK		0x01
#define AL3201_RAN_SHIFT	(0)
#define AL3201_RST_MASK		0x03
#define AL3201_RST_SHIFT	(2)

#define AL3201_RT_COMMAND	0x02
#define AL3201_RT_MASK		0x03
#define AL3201_RT_SHIFT	(0)

#define AL3201_POW_COMMAND	0x00
#define AL3201_POW_MASK		0x03
#define AL3201_POW_UP		0x03
#define AL3201_POW_DOWN		0x00
#define AL3201_POW_SHIFT	(0)

#define	AL3201_ADC_LSB		0x0c
#define	AL3201_ADC_MSB		0x0d

#define AL3201_NUM_CACHABLE_REGS	5

static const u8 al3201_reg[AL3201_NUM_CACHABLE_REGS] = {
	0x00,
	0x01,
	0x02,
	0x0c,
	0x0d
};

enum SENSOR_STATE {
	OFF = 0,
	ON = 1,
};

struct al3201_data {
	struct i2c_client *client;
	struct mutex lock;
	struct input_dev *input;
	struct work_struct work_light;
	struct workqueue_struct *wq;
	struct hrtimer timer;
	struct device *light_dev;

	ktime_t light_poll_delay;

	u8 reg_cache[AL3201_NUM_CACHABLE_REGS];
	int state;
};

/*
 * register access helpers
 */
static int al3201_read_reg(struct i2c_client *client,
			   u32 reg, u8 mask, u8 shift)
{
	struct al3201_data *data = i2c_get_clientdata(client);

	return (data->reg_cache[reg] & mask) >> shift;
}

static int al3201_write_reg(struct i2c_client *client,
			    u32 reg, u8 mask, u8 shift, u8 val)
{
	u8 tmp;
	int ret = 0;
	struct al3201_data *data = i2c_get_clientdata(client);

	if (reg >= AL3201_NUM_CACHABLE_REGS)
		return -EINVAL;

	mutex_lock(&data->lock);

	tmp = data->reg_cache[reg];
	tmp &= ~mask;
	tmp |= val << shift;

	ret = i2c_smbus_write_byte_data(client, reg, tmp);
	if (!ret)
		data->reg_cache[reg] = tmp;
	else
		pr_err("%s: I2C read failed!\n", __func__);

	mutex_unlock(&data->lock);
	return ret;
}

/*
 * internally used functions
 */

/* range */
/*
static int al3201_sw_reset(struct i2c_client *client)
{
	al3201_write_reg(client, AL3201_RAN_COMMAND,
			 AL3201_RST_MASK, AL3201_RST_SHIFT, 0x02);
	msleep(20);
	al3201_write_reg(client, AL3201_RAN_COMMAND,
			 AL3201_RST_MASK, AL3201_RST_SHIFT, 0x00);
	return 0;
}
*/

static int al3201_get_range(struct i2c_client *client)
{
	int tmp;
	tmp = al3201_read_reg(client, AL3201_RAN_COMMAND,
			      AL3201_RAN_MASK, AL3201_RAN_SHIFT);
	return tmp;
}

static int al3201_set_range(struct i2c_client *client, int range)
{
	return al3201_write_reg(client, AL3201_RAN_COMMAND,
				AL3201_RAN_MASK, AL3201_RAN_SHIFT, range);
}

/* Response time */
static int al3201_set_response_time(struct i2c_client *client, int time)
{
	return al3201_write_reg(client, AL3201_RT_COMMAND,
				AL3201_RT_MASK, AL3201_RT_SHIFT, time);
}

/* power_state */
static int al3201_set_power_state(struct i2c_client *client, int state)
{
	return al3201_write_reg(client, AL3201_POW_COMMAND,
				AL3201_POW_MASK, AL3201_POW_SHIFT,
				state ? AL3201_POW_UP : AL3201_POW_DOWN);
}
/*
static int al3201_get_power_state(struct i2c_client *client)
{
	u8 cmdreg;
	struct al3201_data *data = i2c_get_clientdata(client);

	cmdreg = data->reg_cache[AL3201_POW_COMMAND];
	return (cmdreg & AL3201_POW_MASK) >> AL3201_POW_SHIFT;
} */

/* power & timer enable */
static int al3201_enable(struct al3201_data *data)
{
	int err;

	err = al3201_set_power_state(data->client, ON);
	if (err) {
		pr_err("%s: Failed to write byte (POWER_UP)\n", __func__);
		return err;
	}

	hrtimer_start(&data->timer, ns_to_ktime(300000000), HRTIMER_MODE_REL);

	return err;
}

/* power & timer disable */
static int al3201_disable(struct al3201_data *data)
{
	int err;

	hrtimer_cancel(&data->timer);
	cancel_work_sync(&data->work_light);

	err = al3201_set_power_state(data->client, OFF);
	if (unlikely(err != 0))
		pr_err("%s: Failed to write byte (POWER_DOWN)\n", __func__);

	return err;
}

static int al3201_get_adc_value(struct i2c_client *client)
{
	int lsb, msb, range;
	u32 val;
	struct al3201_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	lsb = i2c_smbus_read_byte_data(client, AL3201_ADC_LSB);
	if (lsb < 0) {
		mutex_unlock(&data->lock);
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, AL3201_ADC_MSB);

	mutex_unlock(&data->lock);

	if (msb < 0)
		return msb;

	range = al3201_get_range(client);
	val = (u32) (msb << 8 | lsb);
	if (val <= DARK_COUNT)
		val = 0;

	return val;
}

static void al3201_work_func_light(struct work_struct *work)
{
	int result = 0;

	struct al3201_data *data =
	    container_of(work, struct al3201_data, work_light);

	result = al3201_get_adc_value(data->client);

#ifdef LSC_DBG
	pr_info("%s, value = %d\n", __func__, result);
#endif
	input_report_rel(data->input, REL_MISC, result+1);
	input_sync(data->input);

}

static enum hrtimer_restart al3201_timer_func(struct hrtimer *timer)
{
	struct al3201_data *data =
	    container_of(timer, struct al3201_data, timer);

	queue_work(data->wq, &data->work_light);
	hrtimer_forward_now(&data->timer, data->light_poll_delay);

	return HRTIMER_RESTART;
}

/*
 * sysfs layer
 */
static ssize_t al3201_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int result;
	struct input_dev *input = to_input_dev(dev);
	struct al3201_data *data = input_get_drvdata(input);

	/* No LUX data if not operational */
	if (data->state == OFF) {
		al3201_set_power_state(data->client, ON);
		msleep(180);
	}

	result = al3201_get_adc_value(data->client);

	if (data->state == OFF)
		al3201_set_power_state(data->client, OFF);

	return sprintf(buf, "%d\n", result);
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

static DEVICE_ATTR(raw_data, 0644, al3201_raw_data_show, NULL);
static DEVICE_ATTR(vendor, 0644, get_vendor_name, NULL);
static DEVICE_ATTR(name, 0644, get_chip_name, NULL);

/* factory test*/
#ifdef LSC_DBG
/* engineer mode */
static ssize_t al3201_em_read(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	u8 tmp;
	int i;
	struct input_dev *input = to_input_dev(dev);
	struct al3201_data *data = input_get_drvdata(input);

	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		mutex_lock(&data->lock);
		tmp = i2c_smbus_read_byte_data(data->client, al3201_reg[i]);
		mutex_unlock(&data->lock);

		pr_info("%s, Reg[0x%x] Val[0x%x]\n", __func__,
			al3201_reg[i], tmp);
	}
	return 0;
}

static ssize_t al3201_em_write(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	int ret = 0;
	u32 addr, val;
	struct input_dev *input = to_input_dev(dev);
	struct al3201_data *data = input_get_drvdata(input);

	sscanf(buf, "%x%x", &addr, &val);
	pr_info("%s, Write [%x] to Reg[%x]...\n", __func__, val, addr);

	mutex_lock(&data->lock);

	ret = i2c_smbus_write_byte_data(data->client, addr, val);
	if (!ret)
		data->reg_cache[addr] = val;

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(em, S_IWUSR | S_IRUGO, al3201_em_read, al3201_em_write);
#endif

static ssize_t al3201_poll_delay_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct al3201_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", ktime_to_ns(data->light_poll_delay));
}

static ssize_t al3201_poll_delay_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	int err;
	int64_t new_delay;
	struct al3201_data *data = dev_get_drvdata(dev);

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	pr_info("%s, new delay = %lldns, old delay = %lldns\n", __func__,
	       new_delay, ktime_to_ns(data->light_poll_delay));

	if (new_delay != ktime_to_ns(data->light_poll_delay)) {
		data->light_poll_delay = ns_to_ktime(new_delay);
		if (data->state) {
			al3201_disable(data);
			al3201_enable(data);
		}
	}

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   al3201_poll_delay_show, al3201_poll_delay_store);

static ssize_t al3201_light_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct al3201_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->state);
}

static ssize_t al3201_light_enable_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	bool new_value = false;
	int err = 0;
	struct al3201_data *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_info("%s, new_value = %d, old state = %d\n",
		__func__, new_value, data->state);

	if (new_value && (!data->state)) {
		err = al3201_enable(data);
		if (!err) {
			data->state = ON;
		} else {
			pr_err("%s: couldn't enable", __func__);
			data->state = OFF;
		}
	} else if (!new_value && (data->state)) {
		err = al3201_disable(data);
		if (!err)
			data->state = OFF;
		else
			pr_err("%s: couldn't disable", __func__);
	}

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   al3201_light_enable_show, al3201_light_enable_store);

static struct attribute *al3201_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,

#ifdef LSC_DBG
	&dev_attr_em.attr,
#endif
	NULL
};

static const struct attribute_group al3201_attribute_group = {
	.attrs = al3201_attributes,
};

static int al3201_init_client(struct i2c_client *client)
{
	int i;
	struct al3201_data *data = i2c_get_clientdata(client);

	/* read all the registers once to fill the cache.
	 * if one of the reads fails, we consider the init failed */
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		int v = i2c_smbus_read_byte_data(client, al3201_reg[i]);
		if (v < 0)
			return -ENODEV;
		data->reg_cache[i] = v;
	}

	/*
	* 0 : Low resolution range, 0 to 32768 lux
	* 1 : High resolution range, 0 to 8192 lux
	*/
	al3201_set_range(client, 1);
	/* 0x02 : Response time 200ms low pass fillter */
	al3201_set_response_time(client, 0x02);
	/* chip power off */
	al3201_set_power_state(client, OFF);

	return 0;
}

static int __devinit al3201_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int err = 0;
	struct al3201_data *data;
	struct input_dev *input_dev;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("%s: start\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("%s, failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->lock);

	/* initialize the AL3201 chip */
	err = al3201_init_client(client);
	if (err) {
		pr_err("%s: No such device\n", __func__);
		goto err_initializ_chip;
	}

	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	data->timer.function = al3201_timer_func;
	data->state = OFF;

	data->wq = alloc_workqueue("al3201_wq", WQ_UNBOUND | WQ_RESCUER, 1);
	if (!data->wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	INIT_WORK(&data->work_light, al3201_work_func_light);

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(input_dev, data);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_REL, REL_MISC);

	err = input_register_device(input_dev);
	if (err < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}

	data->input = input_dev;

	err = sysfs_create_group(&input_dev->dev.kobj, &al3201_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* set sysfs for light sensor */
	data->light_dev = sensors_classdev_register("light_sensor");
	if (IS_ERR(data->light_dev)) {
		pr_err("%s: could not create light_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(data->light_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_light_device_create_file1;
	}

	if (device_create_file(data->light_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_light_device_create_file2;
	}

	if (device_create_file(data->light_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_light_device_create_file3;
	}

	dev_set_drvdata(data->light_dev, data);
	pr_info("%s: success!\n", __func__);
	goto done;

 err_light_device_create_file3:
	device_remove_file(data->light_dev, &dev_attr_vendor);
err_light_device_create_file2:
	device_remove_file(data->light_dev, &dev_attr_raw_data);
err_light_device_create_file1:
	sensors_classdev_unregister(data->light_dev);
 err_light_device_create:
	sysfs_remove_group(&data->input->dev.kobj, &al3201_attribute_group);
 err_sysfs_create_group_light:
	input_unregister_device(data->input);
 err_input_register_device_light:
 err_input_allocate_device_light:
	destroy_workqueue(data->wq);
 err_create_workqueue:
 err_initializ_chip:
	mutex_destroy(&data->lock);
	kfree(data);
 done:
	return err;
}

static int al3201_remove(struct i2c_client *client)
{
	struct al3201_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&data->input->dev.kobj, &al3201_attribute_group);
	input_unregister_device(data->input);

	al3201_set_power_state(client, OFF);

	if (data->state)
		al3201_disable(data);

	destroy_workqueue(data->wq);
	mutex_destroy(&data->lock);
	kfree(data);

	return 0;
}

static int al3201_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct al3201_data *data = i2c_get_clientdata(client);

	if (data->state) {
		err = al3201_disable(data);
		if (err)
			pr_err("%s: could not disable\n", __func__);
	}

	return err;
}

static int al3201_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct al3201_data *data = i2c_get_clientdata(client);

	if (data->state) {
		err = al3201_enable(data);
		if (err)
			pr_err("%s: could not enable\n", __func__);
	}
	return err;
}

static const struct i2c_device_id al3201_id[] = {
	{CHIP_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, al3201_id);

static const struct dev_pm_ops al3201_pm_ops = {
	.suspend = al3201_suspend,
	.resume = al3201_resume,
};

static struct i2c_driver al3201_driver = {
	.driver = {
		   .name = CHIP_NAME,
		   .owner = THIS_MODULE,
		   .pm = &al3201_pm_ops,
		   },
	.probe = al3201_probe,
	.remove = al3201_remove,
	.id_table = al3201_id,
};

static int __init al3201_init(void)
{
	return i2c_add_driver(&al3201_driver);
}

static void __exit al3201_exit(void)
{
	i2c_del_driver(&al3201_driver);
}

MODULE_AUTHOR("Kyusung Kim <gs0816.kim@samsung.com>");
MODULE_DESCRIPTION("AL3201 Ambient light sensor driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
module_init(al3201_init);
module_exit(al3201_exit);
