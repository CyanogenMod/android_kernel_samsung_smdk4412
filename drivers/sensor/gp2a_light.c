/*
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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/sensor/gp2a.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/sensor/sensors_core.h>

/* for debugging */
#undef DEBUG

#define	VENDOR		"SHARP"
#define	CHIP_ID		"GP2AP"

/*********** for debug ***************************/
#if 1
#define gprintk(fmt, x...) printk(KERN_INFO "%s(%d): " fmt\
, __func__ , __LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/***********************************************/

#define SENSOR_NAME "light_sensor"
#define SENSOR_MAX_DELAY	(2000)	/* 2000 ms */

#define LIGHT_BUFFER_NUM	5

struct sensor_data {
	struct mutex mutex;
	struct delayed_work work;
	struct device *light_dev;
	struct input_dev *input_dev;
	struct workqueue_struct *wq;
	int enabled;
	int delay;
	int light_buffer;
	int light_count;
};

/* global var */
static const int adc_table[4] = {
	15,			/*15 lux */
	140,			/* 150 lux */
	1490,			/* 1500 lux */
	15000,			/* 15000 lux */
};

static const int adc_table_030a[4] = {
	15,			/*15 lux */
	150,			/* 150 lux */
	1512,			/* 1500 lux */
	14397,			/* 15000 lux */
};

static struct platform_device *sensor_pdev;
static bool first_value = true;
u8 lightsensor_mode;		/* 0 = low, 1 = high */

/* prototype */
static int lightsensor_get_adc(void);
static int lightsensor_onoff(u8 onoff);

/* Light Sysfs interface */
static ssize_t lightsensor_file_state_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	int adc = 0;

	adc = lightsensor_get_adcvalue();

	return sprintf(buf, "%d\n", adc);
}

static ssize_t
light_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->delay);
}

static ssize_t
light_delay_store(struct device *dev, struct device_attribute *attr,
		  const char *buf, size_t count)
{
	struct sensor_data *data = dev_get_drvdata(dev);
	int delay;
	int err = 0;

	err = kstrtoint(buf, 10, &delay);

	if (err)
		printk(KERN_ERR "%s, kstrtoint failed.", __func__);

	if (delay < 0)
		return count;
	delay = delay / 1000000;	/* ns to msec */
#ifdef CONFIG_MACH_BAFFIN
	delay = delay / 100;
#endif


	gprintk("new_delay = %d, old_delay = %d", delay, data->delay);

	if (SENSOR_MAX_DELAY < delay)
		delay = SENSOR_MAX_DELAY;

	data->delay = delay;

	mutex_lock(&data->mutex);

	if (data->enabled) {
		cancel_delayed_work_sync(&data->work);
		queue_delayed_work(data->wq, &data->work,
				   msecs_to_jiffies(delay));
	}

	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t
light_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_data *data = dev_get_drvdata(dev);
	int enabled;

	enabled = data->enabled;

	return sprintf(buf, "%d\n", enabled);
}

static ssize_t
light_enable_store(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t count)
{
	struct sensor_data *data = dev_get_drvdata(dev);
	int value;
	int err = 0;

	err = kstrtoint(buf, 10, &value);

	if (err)
		printk(KERN_ERR "%s, kstrtoint failed.", __func__);

	gprintk("value = %d\n", value);

	if (value != 0 && value != 1)
		return count;

	mutex_lock(&data->mutex);

	if (data->enabled && !value) {
		cancel_delayed_work_sync(&data->work);
		gprintk("timer canceled.\n");
		lightsensor_onoff(0);
		data->enabled = value;
	}
	if (!data->enabled && value) {
		lightsensor_onoff(1);
		data->enabled = value;
		first_value = true;
		queue_delayed_work(data->wq, &data->work, 0);
		gprintk("timer started.\n");
	}

	mutex_unlock(&data->mutex);

	return count;
}


/* sysfs for vendor & name */
static ssize_t lightsensor_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t lightsensor_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return is_gp2a030a() ? sprintf(buf, "%s030\n", CHIP_ID)
		: sprintf(buf, "%s020\n", CHIP_ID);
}

static ssize_t lightsensor_raw_data_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	int adc = 0;

	adc = lightsensor_get_adcvalue();

	return sprintf(buf, "%d\n", adc);
}

static DEVICE_ATTR(poll_delay, 0664, light_delay_show, light_delay_store);
static DEVICE_ATTR(enable, 0664, light_enable_show, light_enable_store);
static DEVICE_ATTR(lux, 0664, lightsensor_file_state_show, NULL);
static DEVICE_ATTR(vendor, 0664, lightsensor_vendor_show, NULL);
static DEVICE_ATTR(name, 0664, lightsensor_name_show, NULL);
static DEVICE_ATTR(raw_data, 0664, lightsensor_raw_data_show, NULL);


static struct attribute *lightsensor_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group lightsensor_attribute_group = {
	.attrs = lightsensor_attributes
};

static int lightsensor_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sensor_data *data = platform_get_drvdata(pdev);
	int rt = 0;

	mutex_lock(&data->mutex);

	if (data->enabled) {
		rt = cancel_delayed_work_sync(&data->work);
		gprintk(": The timer is cancled.\n");
	}

	mutex_unlock(&data->mutex);

	return rt;
}

static int lightsensor_resume(struct platform_device *pdev)
{
	struct sensor_data *data = platform_get_drvdata(pdev);
	int rt = 0;

	data->light_count = 0;
	data->light_buffer = 0;
	first_value = true;

	mutex_lock(&data->mutex);

	if (data->enabled) {
		rt = queue_delayed_work(data->wq, &data->work, 0);
		gprintk(": The timer is started.\n");
	}

	mutex_unlock(&data->mutex);

	return rt;
}

int lightsensor_get_adc(void)
{
	unsigned char get_data[4] = { 0, };
	int D0_raw_data;
	int D1_raw_data;
	int D0_data;
	int D1_data;
	int lx = 0;
	u8 value;
	int light_alpha;
	int light_beta;
	static int lx_prev;
	int ret = 0;
	int d0_boundary = 93;

	ret = opt_i2c_read(DATA0_LSB, get_data, sizeof(get_data));
	if (ret < 0)
		return lx_prev;
	D0_raw_data = (get_data[1] << 8) | get_data[0];	/* clear */
	D1_raw_data = (get_data[3] << 8) | get_data[2];	/* IR */
	if (is_gp2a030a()) {
		#if defined(CONFIG_MACH_GRANDE)
			if (100 * D1_raw_data <= 41 * D0_raw_data) {
				light_alpha = 1186;
				light_beta = 0;
			} else if (100 * D1_raw_data <= 62 * D0_raw_data) {
				light_alpha = 2930;
				light_beta = 4252;
			} else if (100 * D1_raw_data <= d0_boundary * D0_raw_data) {
				light_alpha = 924;
				light_beta = 1015;
			} else {
				light_alpha = 0;
				light_beta = 0;
			}
		#else
			if (100 * D1_raw_data <= 41 * D0_raw_data) {
				light_alpha = 736;
				light_beta = 0;
			} else if (100 * D1_raw_data <= 62 * D0_raw_data) {
				light_alpha = 1855;
				light_beta = 2693;
			} else if (100 * D1_raw_data <= d0_boundary * D0_raw_data) {
				light_alpha = 544;
				light_beta = 595;
			} else {
				light_alpha = 0;
				light_beta = 0;
			}
		#endif
	} else {
		if (lightsensor_mode) {	/* HIGH_MODE */
			if (100 * D1_raw_data <= 32 * D0_raw_data) {
				light_alpha = 800;
				light_beta = 0;
			} else if (100 * D1_raw_data <= 67 * D0_raw_data) {
				light_alpha = 2015;
				light_beta = 2925;
			} else if (100 * D1_raw_data <=
				   d0_boundary * D0_raw_data) {
				light_alpha = 56;
				light_beta = 12;
			} else {
				light_alpha = 0;
				light_beta = 0;
			}
		} else {	/* LOW_MODE */
			if (100 * D1_raw_data <= 32 * D0_raw_data) {
				light_alpha = 800;
				light_beta = 0;
			} else if (100 * D1_raw_data <= 67 * D0_raw_data) {
				light_alpha = 2015;
				light_beta = 2925;
			} else if (100 * D1_raw_data <=
				   d0_boundary * D0_raw_data) {
				light_alpha = 547;
				light_beta = 599;
			} else {
				light_alpha = 0;
				light_beta = 0;
			}
		}
	}

	if (lightsensor_mode) {	/* HIGH_MODE */
		D0_data = D0_raw_data * 16;
		D1_data = D1_raw_data * 16;
	} else {		/* LOW_MODE */
		D0_data = D0_raw_data;
		D1_data = D1_raw_data;
	}
	if (is_gp2a030a()) {
		if (D0_data < 3) {
			lx = 0;
#ifdef DEBUG
			gprintk("lx is 0 : D0=%d, D1=%d\n", D0_raw_data,
				D1_raw_data);
#endif
		} else if (lightsensor_mode == 0
			   && (D0_raw_data >= 16000 || D1_raw_data >= 16000)
			   && (D0_raw_data <= 16383 && D1_raw_data <= 16383)) {
#ifdef DEBUG
			gprintk("need to changed HIGH_MODE D0=%d, D1=%d\n",
				D0_raw_data, D1_raw_data);
#endif
			lx = lx_prev;
		} else if (100 * D1_data > d0_boundary * D0_data) {
			lx = lx_prev;
#ifdef DEBUG
gprintk
	("Data range over so ues prev_lx value=%d D0=%d, D1=%d mode=%d\n",
	 lx, D0_data, D1_data, lightsensor_mode);
#endif
			return lx;
		} else {
			lx = (int)((light_alpha / 10 * D0_data * 33)
				   - (light_beta / 10 * D1_data * 33)) / 1000;
#ifdef DEBUG
	gprintk
	    ("D0=%d, D1=%d, lx=%d mode=%d a=%d, b=%d prev_lx=%d\n",
	     D0_raw_data, D1_raw_data, lx, lightsensor_mode,
	     light_alpha, light_beta, lx_prev);
#endif
		}
	} else {
		if ((D0_data == 0 || D1_data == 0)
		    && (D0_data < 300 && D1_data < 300)) {
			lx = 0;
#ifdef DEBUG
			gprintk("lx is 0 : D0=%d, D1=%d\n", D0_raw_data,
				D1_raw_data);
#endif
		} else if ((lightsensor_mode == 0)
			   && (D0_raw_data >= 16000 || D1_raw_data >= 16000)
			   && (D0_raw_data <= 16383 && D1_raw_data <= 16383)) {
#ifdef DEBUG
			gprintk("need to changed HIGH_MODE D0=%d, D1=%d\n",
				D0_raw_data, D1_raw_data);
#endif
			lx = lx_prev;
		} else if ((100 * D1_data > d0_boundary * D0_data)
			   || (100 * D1_data < 15 * D0_data)) {
			lx = lx_prev;
#ifdef DEBUG
	gprintk
	    ("Data range over so ues prev_lx value=%d D0=%d, D1=%d mode=%d\n",
	     lx, D0_data, D1_data, lightsensor_mode);
#endif
			return lx;
		} else {
			lx = (int)((light_alpha / 10 * D0_data * 33)
				   - (light_beta / 10 * D1_data * 33)) / 1000;
#ifdef DEBUG
	gprintk
	    ("D0=%d, D1=%d, lx=%d mode=%d a=%d, b=%d prev_lx=%d\n",
	     D0_raw_data, D1_raw_data, lx, lightsensor_mode,
	     light_alpha, light_beta, lx_prev);
#endif
		}
	}

	lx_prev = lx;

	if (lightsensor_mode) {	/* HIGH MODE */
		if (D0_raw_data < 1000) {
#ifdef DEBUG
			gprintk("change to LOW_MODE detection=%d\n",
				proximity_sensor_detection);
#endif
			lightsensor_mode = 0;	/* change to LOW MODE */

			value = 0x0C;
			opt_i2c_write(COMMAND1, &value);

			if (proximity_sensor_detection)
				value = 0x23;
			else
				value = 0x63;
			opt_i2c_write(COMMAND2, &value);

			if (proximity_enable)
				value = 0xCC;
			else
				value = 0xDC;
			opt_i2c_write(COMMAND1, &value);
		}
	} else {		/* LOW MODE */
		if (D0_raw_data > 16000 || D1_raw_data > 16000) {
#ifdef DEBUG
			gprintk("change to HIGH_MODE detection=%d\n",
				proximity_sensor_detection);
#endif
			lightsensor_mode = 1;	/* change to HIGH MODE */

			value = 0x0C;
			opt_i2c_write(COMMAND1, &value);

			if (proximity_sensor_detection)
				value = 0x27;
			else
				value = 0x67;
			opt_i2c_write(COMMAND2, &value);

			if (proximity_enable)
				value = 0xCC;
			else
				value = 0xDC;
			opt_i2c_write(COMMAND1, &value);
		}
	}

	return lx;
}

int lightsensor_get_adcvalue(void)
{
	int i, j, value, adc_avr_value;
	unsigned int adc_total = 0, adc_max, adc_min, adc_index;
	static unsigned int adc_index_count;
	static int adc_value_buf[ADC_BUFFER_NUM] = { 0, };

	value = lightsensor_get_adc();

	adc_index = (adc_index_count++) % ADC_BUFFER_NUM;

	/*ADC buffer initialize (light sensor off -> light sensor on) */
	if (first_value == true) {
		for (j = 0; j < ADC_BUFFER_NUM; j++)
			adc_value_buf[j] = value;
		first_value = false;
	} else {
		adc_value_buf[adc_index] = value;
	}

	adc_max = adc_value_buf[0];
	adc_min = adc_value_buf[0];

	for (i = 0; i < ADC_BUFFER_NUM; i++) {
		adc_total += adc_value_buf[i];

		if (adc_max < adc_value_buf[i])
			adc_max = adc_value_buf[i];

		if (adc_min > adc_value_buf[i])
			adc_min = adc_value_buf[i];
	}
	adc_avr_value =
	    (adc_total - (adc_max + adc_min)) / (ADC_BUFFER_NUM - 2);

	if (adc_index_count == ADC_BUFFER_NUM - 1)
		adc_index_count = 0;

	return adc_avr_value;
}

static int lightsensor_onoff(u8 onoff)
{
	u8 value = 0;

#ifdef DEBUG
	gprintk("lightsensor_onoff = %d\n", onoff);
	gprintk("proximity_enable onoff = %d\n", proximity_enable);
#endif

	if (onoff) {
		/*in calling, must turn on proximity sensor */
		if (proximity_enable == 0) {
			value = 0x01;
			opt_i2c_write(COMMAND4, &value);

			value = 0x63;
			opt_i2c_write(COMMAND2, &value);
			/*OP3 : 1(operating mode) OP2 :1
			   (coutinuous operating mode)
			   OP1 : 01(ALS mode) TYPE=0(auto) */
			value = 0xD0;
			opt_i2c_write(COMMAND1, &value);
			/* other setting have defualt value. */
		}
	} else {
		/*in calling, must turn on proximity sensor */
		if (proximity_enable == 0) {
			value = 0x00;	/*shutdown mode */
			opt_i2c_write((u8) (COMMAND1), &value);
		}
	}

	return 0;
}

static void gp2a_work_func_light(struct work_struct *work)
{
	struct sensor_data *data = container_of((struct delayed_work *)work,
						struct sensor_data, work);
	int i;
	int adc = 0;

	adc = lightsensor_get_adcvalue();

	if (is_gp2a030a()) {
		for (i = 0; ARRAY_SIZE(adc_table_030a); i++)
			if (adc <= adc_table_030a[i])
				break;
	} else {
		for (i = 0; ARRAY_SIZE(adc_table); i++)
			if (adc <= adc_table[i])
				break;
	}

	if (data->light_buffer == i) {
		if (data->light_count++ == LIGHT_BUFFER_NUM) {
			input_report_rel(data->input_dev, REL_MISC,
			(adc ? adc : 1));
			input_sync(data->input_dev);
			data->light_count = 0;
		}
	} else {
		data->light_buffer = i;
		data->light_count = 0;
	}

	if (data->enabled)
		queue_delayed_work(data->wq, &data->work,
				   msecs_to_jiffies(data->delay));
}

static int lightsensor_probe(struct platform_device *pdev)
{
	struct sensor_data *data = NULL;
	int rt = -ENXIO;
	unsigned char get_data = 0;

	/* Check I2C communication */
	rt = opt_i2c_read(DATA0_LSB, &get_data, sizeof(get_data));

	if (rt < 0) {
		pr_err("%s failed : threre is no such device.\n", __func__);
		return rt;
	}

	gprintk("probe start!\n");

	data = kzalloc(sizeof(struct sensor_data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	data->enabled = 0;

	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		rt = -ENOMEM;
		goto err_input_allocate_device_light;
	}

	input_set_capability(data->input_dev, EV_REL, REL_MISC);
	data->input_dev->name = SENSOR_NAME;

	rt = input_register_device(data->input_dev);
	if (rt) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(data->input_dev);
		goto err_input_register_device_light;
	}
	input_set_drvdata(data->input_dev, data);

	rt = sysfs_create_group(&data->input_dev->dev.kobj,
				&lightsensor_attribute_group);
	if (rt) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}
	mutex_init(&data->mutex);

	data->light_dev = sensors_classdev_register("light_sensor");
	if (IS_ERR(data->light_dev)) {
		pr_err("%s: could not create light_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(data->light_dev, &dev_attr_lux) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_lux.attr.name);
		goto err_light_device_create_file;
	}

	if (device_create_file(data->light_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_light_device_create_file1;
	}

	if (device_create_file(data->light_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_light_device_create_file2;
	}

	if (device_create_file(data->light_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_light_device_create_file3;
	}

	dev_set_drvdata(data->light_dev, data);

	data->wq = create_singlethread_workqueue("gp2a_wq");
	if (!data->wq) {
		rt = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_DELAYED_WORK(&data->work, gp2a_work_func_light);

	/* set platdata */
	platform_set_drvdata(pdev, data);

	gprintk("probe success!\n");

	goto done;

/* error, unwind it all */
err_light_device_create_file3:
	device_remove_file(data->light_dev, &dev_attr_raw_data);
err_light_device_create_file2:
	device_remove_file(data->light_dev, &dev_attr_name);
err_light_device_create_file1:
	device_remove_file(data->light_dev, &dev_attr_vendor);
err_create_workqueue:
	device_remove_file(data->light_dev, &dev_attr_lux);
err_light_device_create_file:
	sensors_classdev_unregister(data->light_dev);
err_light_device_create:
	mutex_destroy(&data->mutex);
	sysfs_remove_group(&data->input_dev->dev.kobj,
			   &lightsensor_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(data->input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	kfree(data);

done:
	return rt;
}

static int lightsensor_remove(struct platform_device *pdev)
{
	struct sensor_data *data = platform_get_drvdata(pdev);
	int rt = 0;

	if (data != NULL) {
		sysfs_remove_group(&data->input_dev->dev.kobj,
				   &lightsensor_attribute_group);

		device_remove_file(data->light_dev, &dev_attr_lux);
		device_remove_file(data->light_dev, &dev_attr_vendor);
		device_remove_file(data->light_dev, &dev_attr_name);
		device_remove_file(data->light_dev, &dev_attr_raw_data);
		sensors_classdev_unregister(data->light_dev);

		cancel_delayed_work_sync(&data->work);
		flush_workqueue(data->wq);
		destroy_workqueue(data->wq);
		input_unregister_device(data->input_dev);
		mutex_destroy(&data->mutex);
		kfree(data);
	}

	return rt;
}

static void lightsensor_shutdown(struct platform_device *pdev)
{
	struct sensor_data *data = platform_get_drvdata(pdev);

	if (data != NULL) {
		sysfs_remove_group(&data->input_dev->dev.kobj,
				   &lightsensor_attribute_group);
		device_remove_file(data->light_dev, &dev_attr_lux);
		sensors_classdev_unregister(data->light_dev);

		cancel_delayed_work_sync(&data->work);
		flush_workqueue(data->wq);
		destroy_workqueue(data->wq);
		input_unregister_device(data->input_dev);
		kfree(data);
	}
}

/*
 * Module init and exit
 */
static struct platform_driver lightsensor_driver = {
	.probe = lightsensor_probe,
	.remove = lightsensor_remove,
	.suspend = lightsensor_suspend,
	.resume = lightsensor_resume,
	.shutdown = lightsensor_shutdown,
	.driver = {
		   .name = SENSOR_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init lightsensor_init(void)
{
	sensor_pdev = platform_device_register_simple(SENSOR_NAME, 0, NULL, 0);
	if (IS_ERR(sensor_pdev))
		return -1;

	return platform_driver_register(&lightsensor_driver);
}

module_init(lightsensor_init);

static void __exit lightsensor_exit(void)
{
	platform_driver_unregister(&lightsensor_driver);
	platform_device_unregister(sensor_pdev);
}

module_exit(lightsensor_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for GP2AP020A00F");
MODULE_LICENSE("GPL");
