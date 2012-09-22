/* linux/driver/input/misc/gp2a.c
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
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/gp2a.h>
#include <plat/adc.h>

/* Note about power vs enable/disable:
 *  The chip has two functions, proximity and ambient light sensing.
 *  There is no separate power enablement to the two functions (unlike
 *  the Capella CM3602/3623).
 *  This module implements two drivers: /dev/proximity and /dev/light.
 *  When either driver is enabled (via sysfs attributes), we give power
 *  to the chip.  When both are disabled, we remove power from the chip.
 *  In suspend, we remove power if light is disabled but not if proximity is
 *  enabled (proximity is allowed to wakeup from suspend).
 *
 *  There are no ioctls for either driver interfaces.  Output is via
 *  input device framework and control via sysfs attributes.
 */

#if defined(CONFIG_MACH_P8)
	#define GP2A_MODE_B
#endif

#define gp2a_dbgmsg(str, args...) pr_debug("%s: " str, __func__, ##args)

#define VENDOR_NAME		"SHARP"
#define CHIP_NAME		"GP2A0002"

/* ADDSEL is LOW */
#define REGS_PROX		0x0 /* Read  Only */
#define REGS_GAIN		0x1 /* Write Only */
#define REGS_HYS		0x2 /* Write Only */
#define REGS_CYCLE		0x3 /* Write Only */
#define REGS_OPMOD		0x4 /* Write Only */
#define REGS_CON		0x6 /* Write Only */

/* sensor type */
#define LIGHT		0
#define PROXIMITY	1
#define ALL		2

#ifdef GP2A_MODE_B

#define GP2A_MSK_PROX_VO			0x01
#define GP2A_BIT_PROX_VO_NO_DETECTION		0x01
#define GP2A_BIT_PROX_VO_DETECTION		0x00

#define GP2A_BIT_OPMOD_SSD_SHUTDOWN_MODE	0x00
#define GP2A_BIT_OPMOD_SSD_OPERATING_MODE	0x01
#define GP2A_BIT_OPMOD_VCON_NORMAL_MODE		0x00
#define GP2A_BIT_OPMOD_VCON_INTERRUPT_MODE	0x02
#define GP2A_BIT_OPMOD_ASD_INEFFECTIVE		0x00
#define GP2A_BIT_OPMOD_ASD_EFFECTIVE		0x10

#define GP2A_BIT_CON_OCON_ENABLE_VOUT		0x00
#define GP2A_BIT_CON_OCON_FORCE_VOUT_HIGH	0x18
#define GP2A_BIT_CON_OCON_FORCE_VOUT_LOW	0x10

#define GP2A_INPUT_RANGE_MIN	0
#define GP2A_INPUT_RANGE_MAX	1
#define GP2A_INPUT_RANGE_FUZZ	0
#define GP2A_INPUT_RANGE_FLAT	0

#define VO_0		0x40
#define VO_1		0x20

#else
/* GP2A MODE A*/
#define VO_0		0x40	/* 0x40 */
#define VO_1		0x20	/* 0x20 */

static u8 reg_defaults[5] = {
	0x00, /* PROX: read only register */
	0x08, /* GAIN: large LED drive level */
	VO_0, /* HYS: receiver sensitivity */
	0x04, /* CYCLE: */
	0x01, /* OPMOD: normal operating mode */
};

#endif

#define LIGHT_TIMER_PERIOD_MS	200

#define LIGHT_FAKE_THRESHOLD	80
#define LIGHT_FAKE_LIMIT	(LIGHT_FAKE_THRESHOLD*2)

/* light sensor adc channel */
#define ALS_IOUT_ADC	9

struct gp2a_data;

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

extern int sensors_register(struct device *dev, void * drvdata,
		struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev);

struct platform_device *pdev_gp2a_adc;

/* driver data */
struct gp2a_data {
	struct input_dev		*proximity_input_dev;
	struct input_dev		*light_input_dev;
	struct gp2a_platform_data	*pdata;
	struct i2c_client		*i2c_client;
	struct device			*light_dev;
	struct device			*proximity_dev;
	struct device			*switch_cmd_dev;
	struct class			*lightsensor_class;
	int				irq;
	struct work_struct		work_light;
	struct hrtimer			timer;
	ktime_t				light_poll_delay;
	bool				on;
	u8				power_state;
	struct mutex			power_mutex;
	struct mutex			adc_mutex;
	struct wake_lock		prx_wake_lock;
	struct workqueue_struct		*wq;
	struct s3c_adc_client		*padc;
	bool				enable_wakeup;
	int				prox_value;
#ifdef GP2A_MODE_B
	struct work_struct		work_proximity;
#endif
};

int gp2a_i2c_write(struct gp2a_data *gp2a, u8 reg, u8 *val)
{
	int err = 0;
	struct i2c_msg msg[1];
	u8 data[2];
	int retry = 10;
	struct i2c_client *client = gp2a->i2c_client;

	pr_info("%s\n", __func__);

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = reg;
		data[1] = *val;

		msg->addr = client->addr;
		msg->flags = 0; /* write */
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	}
	return err;
}

static void gp2a_light_enable(struct gp2a_data *gp2a)
{
	gp2a_dbgmsg("starting poll timer, delay %lldns\n",
		ktime_to_ns(gp2a->light_poll_delay));
	hrtimer_start(&gp2a->timer, gp2a->light_poll_delay, HRTIMER_MODE_REL);
}

static void gp2a_light_disable(struct gp2a_data *gp2a)
{
	gp2a_dbgmsg("cancelling poll timer\n");
	hrtimer_cancel(&gp2a->timer);
	cancel_work_sync(&gp2a->work_light);
}

static ssize_t poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(gp2a->light_poll_delay));
}

static ssize_t poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	gp2a_dbgmsg("new delay = %lldns, old delay = %lldns\n",
		new_delay, ktime_to_ns(gp2a->light_poll_delay));
	mutex_lock(&gp2a->power_mutex);
	if (new_delay != ktime_to_ns(gp2a->light_poll_delay)) {
		gp2a->light_poll_delay = ns_to_ktime(new_delay);
		if (gp2a->power_state & LIGHT_ENABLED) {
			gp2a_light_disable(gp2a);
			gp2a_light_enable(gp2a);
		}
	}
	mutex_unlock(&gp2a->power_mutex);

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
			(gp2a->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
			(gp2a->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&gp2a->power_mutex);
	gp2a_dbgmsg("new_value = %d, old state = %d\n",
		new_value, (gp2a->power_state & LIGHT_ENABLED) ? 1 : 0);
	if (new_value && !(gp2a->power_state & LIGHT_ENABLED)) {
		gp2a->power_state |= LIGHT_ENABLED;
		gp2a_light_enable(gp2a);
	} else if (!new_value && (gp2a->power_state & LIGHT_ENABLED)) {
		gp2a_light_disable(gp2a);
		gp2a->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&gp2a->power_mutex);
	return size;
}

static ssize_t proximity_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	bool new_value;
#ifdef GP2A_MODE_B
	u8 val;
#endif
	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&gp2a->power_mutex);
	gp2a_dbgmsg("new_value = %d, old state = %d\n",
		new_value, (gp2a->power_state & PROXIMITY_ENABLED) ? 1 : 0);
	if (new_value && !(gp2a->power_state & PROXIMITY_ENABLED)) {
		gp2a->power_state |= PROXIMITY_ENABLED;

#ifdef GP2A_MODE_B
		val = GP2A_BIT_OPMOD_SSD_OPERATING_MODE
			| GP2A_BIT_OPMOD_VCON_NORMAL_MODE
			| GP2A_BIT_OPMOD_ASD_INEFFECTIVE;
		gp2a_i2c_write(gp2a, REGS_OPMOD, &val);

		val = GP2A_BIT_CON_OCON_ENABLE_VOUT;
		gp2a_i2c_write(gp2a, REGS_CON, &val);

		val = VO_0;
		gp2a_i2c_write(gp2a, REGS_HYS, &val);
#else
		gp2a_i2c_write(gp2a, REGS_GAIN, &reg_defaults[1]);
		gp2a_i2c_write(gp2a, REGS_HYS, &reg_defaults[2]);
		gp2a_i2c_write(gp2a, REGS_CYCLE, &reg_defaults[3]);
		gp2a_i2c_write(gp2a, REGS_OPMOD, &reg_defaults[4]);
		gp2a->prox_value = 1;
#endif
		enable_irq(gp2a->irq);
		if (gp2a->enable_wakeup)
			enable_irq_wake(gp2a->irq);
	} else if (!new_value && (gp2a->power_state & PROXIMITY_ENABLED)) {
		if (gp2a->enable_wakeup)
			disable_irq_wake(gp2a->irq);
		disable_irq(gp2a->irq);

#ifdef GP2A_MODE_B
		val = GP2A_BIT_CON_OCON_FORCE_VOUT_HIGH;
		gp2a_i2c_write(gp2a, REGS_CON, &val);

		val = GP2A_BIT_OPMOD_SSD_SHUTDOWN_MODE
			| GP2A_BIT_OPMOD_VCON_NORMAL_MODE
			| GP2A_BIT_OPMOD_ASD_INEFFECTIVE;
		gp2a_i2c_write(gp2a, REGS_OPMOD, &val);
#else
		gp2a_i2c_write(gp2a, REGS_OPMOD, &reg_defaults[0]);
#endif
		gp2a->power_state &= ~PROXIMITY_ENABLED;
	}
	mutex_unlock(&gp2a->power_mutex);
	return size;
}

static int lightsensor_get_adcvalue(struct gp2a_data *gp2a)
{
	int value = 0;
	int fake_value;

	/* get ADC */
	/* value = gp2a->pdata->light_adc_value(); */

	mutex_lock(&gp2a->adc_mutex);

	value = s3c_adc_read(gp2a->padc, ALS_IOUT_ADC);

	mutex_unlock(&gp2a->adc_mutex);

	if (value < 0) {
		pr_err("%s : ADC Fail, ret = %d", __func__, value);
		value = 0;
	}

	/* Cut off ADC Value
	 */
	if (value < LIGHT_FAKE_LIMIT) {
		fake_value =
			(value < LIGHT_FAKE_THRESHOLD) ?
			0 : 2 * (value) - LIGHT_FAKE_LIMIT;
		value = fake_value;
	}

	return value;
}

static ssize_t lightsensor_file_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int adc = 0;

	adc = lightsensor_get_adcvalue(gp2a);
	return sprintf(buf, "%d\n", adc);
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", gp2a->prox_value);
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


static DEVICE_ATTR(lightsensor_file_illuminance, S_IRUGO,
	lightsensor_file_state_show, NULL);

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

static struct device_attribute dev_attr_light_raw_data =
		__ATTR(raw_data, S_IRUGO, lightsensor_file_state_show, NULL);

static struct device_attribute dev_attr_light_lux =
		__ATTR(lux, S_IRUGO, lightsensor_file_state_show, NULL);

static struct device_attribute dev_attr_proximity_raw_data =
		__ATTR(raw_data, S_IRUGO, proximity_state_show, NULL);

static DEVICE_ATTR(vendor, S_IRUGO, get_vendor_name, NULL);
static DEVICE_ATTR(name, S_IRUGO, get_chip_name, NULL);

static struct device_attribute *light_sensor_attrs[] = {
	&dev_attr_light_raw_data,
	&dev_attr_light_lux,
	&dev_attr_light_enable,
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static struct device_attribute *proximity_sensor_attrs[] = {
	&dev_attr_proximity_raw_data,
	&dev_attr_proximity_enable,
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static void gp2a_work_func_light(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of(work, struct gp2a_data,
					work_light);
	int adc = lightsensor_get_adcvalue(gp2a);

	input_report_abs(gp2a->light_input_dev, ABS_MISC, adc);
	input_sync(gp2a->light_input_dev);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart gp2a_timer_func(struct hrtimer *timer)
{
	struct gp2a_data *gp2a = container_of(timer, struct gp2a_data, timer);
	queue_work(gp2a->wq, &gp2a->work_light);
	hrtimer_forward_now(&gp2a->timer, gp2a->light_poll_delay);
	return HRTIMER_RESTART;
}

#ifdef GP2A_MODE_B

static void gp2a_work_func_proximity(struct work_struct *work)
{
	int	ret;
	u8 value;

	struct gp2a_data *gp2a = container_of(work, struct gp2a_data,
					work_proximity);
	pr_info("%s : gp2a mode b", __func__);

	mutex_lock(&gp2a->power_mutex);
	/* GP2A initialized and powered on => do the job */
	ret = gpio_get_value(gp2a->pdata->p_out);

	if (ret < 0) {
		pr_info("Failed to get GP2A proximity value "
				"[errno=%d]; ignored", ret);
	} else {
		gp2a->prox_value = ret;

		if (GP2A_BIT_PROX_VO_DETECTION == gp2a->prox_value) {
			ret = GP2A_INPUT_RANGE_MIN;
			pr_info("GP2A_INPUT_RANGE_MIN");
		} else {
			ret = GP2A_INPUT_RANGE_MAX;
			pr_info("GP2A_INPUT_RANGE_MAX");
		}
		input_report_abs(gp2a->proximity_input_dev, ABS_DISTANCE, ret);
		input_sync(gp2a->proximity_input_dev);
		pr_info("input_report_abs proximity_input_dev");
	}

	if (ret)
		value = VO_0;
	else
		value = VO_1;

	pr_info("value = 0x%x\n", value);

	gp2a_i2c_write(gp2a, REGS_HYS, &value);

	/* enabling VOUT terminal in nomal operation */
	value = 0x00;
	gp2a_i2c_write(gp2a, REGS_CON, &value);

	mutex_unlock(&gp2a->power_mutex);
}
#endif

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t gp2a_irq_handler(int irq, void *data)
{
	struct gp2a_data *ip = data;

#ifdef GP2A_MODE_B
/* GP2A MODE B */
	queue_work(ip->wq, &ip->work_proximity);
#else
/* GP2A MODE A */
	u8 setting;
	int val = gpio_get_value(ip->pdata->p_out);
	if (val < 0) {
		pr_err("%s: gpio_get_value error %d\n", __func__, val);
		return IRQ_HANDLED;
	}

	if (val != ip->prox_value) {
		if (val)
			setting = VO_0;
		else
			setting = VO_1;
		gp2a_i2c_write(ip, REGS_HYS, &setting);
	}

	ip->prox_value = val;
	pr_err("gp2a: proximity val = %d\n", val);

	/* 0 is close, 1 is far */
	input_report_abs(ip->proximity_input_dev, ABS_DISTANCE, val);
	input_sync(ip->proximity_input_dev);
	wake_lock_timeout(&ip->prx_wake_lock, 3*HZ);
#endif
	return IRQ_HANDLED;
}

static int gp2a_setup_irq(struct gp2a_data *gp2a)
{
	int rc = -EIO;
	struct gp2a_platform_data *pdata = gp2a->pdata;
	int irq;

	gp2a_dbgmsg("start\n");

	rc = gpio_request(pdata->p_out, "gpio_proximity_out");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->p_out, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->p_out);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->p_out, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->p_out);
	rc = request_threaded_irq(irq, NULL,
			 gp2a_irq_handler,
			 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			 "proximity_int",
			 gp2a);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq, pdata->p_out, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(irq);
	gp2a->irq = irq;

	gp2a_dbgmsg("success\n");

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->p_out);
done:
	return rc;
}



static const struct file_operations light_fops = {
	.owner	= THIS_MODULE,
};

static struct miscdevice light_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "light",
	.fops	= &light_fops,
};

static int gp2a_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct input_dev *input_dev;
	struct gp2a_data *gp2a;
	struct gp2a_platform_data *pdata = client->dev.platform_data;

	pr_info("==============================\n");
	pr_info("=========     GP2A     =======\n");
	pr_info("==============================\n");

	if (!pdata) {
		pr_err("%s: missing pdata!\n", __func__);
		return ret;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	gp2a = kzalloc(sizeof(struct gp2a_data), GFP_KERNEL);
	if (!gp2a) {
		pr_err("%s: failed to alloc memory for module data\n",
			__func__);
		return -ENOMEM;
	}

#if defined(CONFIG_OPTICAL_WAKE_ENABLE)
	if (system_rev >= 0x03) {
		pr_info("GP2A Reset GPIO = GPX0(1) (rev%02d)\n", system_rev);
		gp2a->enable_wakeup = true;
	} else {
		pr_info("GP2A Reset GPIO = GPL0(6) (rev%02d)\n", system_rev);
		gp2a->enable_wakeup = false;
	}
#else
	gp2a->enable_wakeup = false;
#endif

	gp2a->pdata = pdata;
	gp2a->i2c_client = client;
	i2c_set_clientdata(client, gp2a);

	/* wake lock init */
	wake_lock_init(&gp2a->prx_wake_lock, WAKE_LOCK_SUSPEND,
			"prx_wake_lock");
	mutex_init(&gp2a->power_mutex);
	mutex_init(&gp2a->adc_mutex);

	ret = gp2a_setup_irq(gp2a);
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

	gp2a->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, gp2a);
	input_dev->name = "proximity_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	gp2a_dbgmsg("registering proximity input device\n");
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

	/* hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&gp2a->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gp2a->light_poll_delay =
			ns_to_ktime(LIGHT_TIMER_PERIOD_MS * NSEC_PER_MSEC);
	gp2a->timer.function = gp2a_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	gp2a->wq = create_singlethread_workqueue("gp2a_wq");
	if (!gp2a->wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&gp2a->work_light, gp2a_work_func_light);

#ifdef GP2A_MODE_B
	/* this is the thread function we run on the work queue */
	INIT_WORK(&gp2a->work_proximity, gp2a_work_func_proximity);
#endif

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(input_dev, gp2a);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	gp2a_dbgmsg("registering lightsensor-level input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}

	gp2a->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj,
				&light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* alloc platform device for adc client */
	pdev_gp2a_adc = platform_device_alloc("gp2a-adc", -1);
	if (!pdev_gp2a_adc) {
		pr_err("%s: could not allocation pdev_gp2a_adc.\n", __func__);
		ret = -ENOMEM;
		goto err_platform_allocate_device_adc;
	}

	/* Register adc client */
	gp2a->padc = s3c_adc_register(pdev_gp2a_adc, NULL, NULL, 0);

	if (IS_ERR(gp2a->padc)) {
		dev_err(&pdev_gp2a_adc->dev, "cannot register adc\n");
		ret = PTR_ERR(gp2a->padc);
		goto err_platform_register_device_adc;
	}

	/* set sysfs for light sensor */

	ret = misc_register(&light_device);
	if (ret)
		pr_err(KERN_ERR "misc_register failed - light\n");

	gp2a->lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(gp2a->lightsensor_class))
		pr_err("Failed to create class(lightsensor)!\n");

	gp2a->switch_cmd_dev = device_create(gp2a->lightsensor_class,
		NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(gp2a->switch_cmd_dev))
		pr_err("Failed to create device(switch_cmd_dev)!\n");

	if (device_create_file(gp2a->switch_cmd_dev,
		&dev_attr_lightsensor_file_illuminance) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_lightsensor_file_illuminance.attr.name);

	dev_set_drvdata(gp2a->switch_cmd_dev, gp2a);

/* new sysfs */
	ret = sensors_register(gp2a->light_dev, gp2a, light_sensor_attrs,
						"light_sensor");
	if (ret) {
		pr_err("%s: cound not register light sensor device(%d).\n",
		__func__, ret);
		goto out_light_sensor_register_failed;
	}

	ret = sensors_register(gp2a->proximity_dev,
			gp2a, proximity_sensor_attrs, "proximity_sensor");
	if (ret) {
		pr_err("%s: cound not register proximity sensor device(%d).\n",
		__func__, ret);
		goto out_proximity_sensor_register_failed;
	}

	/* set initial proximity value as 1 */
	gp2a->prox_value = 1;
	input_report_abs(gp2a->proximity_input_dev, ABS_DISTANCE, 1);
	input_sync(gp2a->proximity_input_dev);

	goto done;

	/* error, unwind it all */
out_light_sensor_register_failed:
	sensors_unregister(gp2a->light_dev);
out_proximity_sensor_register_failed:
	sensors_unregister(gp2a->proximity_dev);

err_sysfs_create_group_light:
	input_unregister_device(gp2a->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(gp2a->wq);
err_platform_allocate_device_adc:
	platform_device_unregister(pdev_gp2a_adc);
err_platform_register_device_adc:
	s3c_adc_release(gp2a->padc);
err_create_workqueue:
	sysfs_remove_group(&gp2a->proximity_input_dev->dev.kobj,
			&proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(gp2a->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
	free_irq(gp2a->irq, 0);
	gpio_free(gp2a->pdata->p_out);
err_setup_irq:
	mutex_destroy(&gp2a->adc_mutex);
	mutex_destroy(&gp2a->power_mutex);

	wake_lock_destroy(&gp2a->prx_wake_lock);
	kfree(gp2a);
done:
	return ret;
}

static int gp2a_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   gp2a->power_state because we use that state in resume.
	*/
	struct i2c_client *client = to_i2c_client(dev);
	struct gp2a_data *gp2a = i2c_get_clientdata(client);

	if (gp2a->power_state & LIGHT_ENABLED)
		gp2a_light_disable(gp2a);
	/* if (gp2a->power_state == LIGHT_ENABLED)
		gp2a->pdata->power(false); */
	return 0;
}

static void gp2a_shutdown(struct i2c_client *client)
{
	struct gp2a_data *gp2a = i2c_get_clientdata(client);

	pr_err("%s\n", __func__);
	if (gp2a->power_state & LIGHT_ENABLED)
		gp2a_light_disable(gp2a);
	/* if (gp2a->power_state == LIGHT_ENABLED)
		gp2a->pdata->power(false); */
}

static int gp2a_resume(struct device *dev)
{
	/* Turn power back on if we were before suspend. */
	struct i2c_client *client = to_i2c_client(dev);
	struct gp2a_data *gp2a = i2c_get_clientdata(client);
	/* if (gp2a->power_state == LIGHT_ENABLED)
		gp2a->pdata->power(true); */
	if (gp2a->power_state & LIGHT_ENABLED)
		gp2a_light_enable(gp2a);
	return 0;
}

static int gp2a_i2c_remove(struct i2c_client *client)
{
	struct gp2a_data *gp2a = i2c_get_clientdata(client);
	sysfs_remove_group(&gp2a->light_input_dev->dev.kobj,
			&light_attribute_group);
	input_unregister_device(gp2a->light_input_dev);
	sysfs_remove_group(&gp2a->proximity_input_dev->dev.kobj,
			&proximity_attribute_group);
	input_unregister_device(gp2a->proximity_input_dev);

	platform_device_unregister(pdev_gp2a_adc);
	free_irq(gp2a->irq, NULL);
	gpio_free(gp2a->pdata->p_out);
	if (gp2a->power_state) {
		if (gp2a->power_state & LIGHT_ENABLED)
			gp2a_light_disable(gp2a);
		/* gp2a->pdata->power(false); */
	}

	destroy_workqueue(gp2a->wq);
	mutex_destroy(&gp2a->power_mutex);
	mutex_destroy(&gp2a->adc_mutex);

	wake_lock_destroy(&gp2a->prx_wake_lock);
	s3c_adc_release(gp2a->padc);
	kfree(gp2a);
	return 0;
}

static const struct i2c_device_id gp2a_device_id[] = {
	{"gp2a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gp2a_device_id);

static const struct dev_pm_ops gp2a_pm_ops = {
	.suspend	= gp2a_suspend,
	.resume		= gp2a_resume,
};

static struct i2c_driver gp2a_i2c_driver = {
	.driver = {
		.name	= "gp2a",
		.owner	= THIS_MODULE,
		.pm	= &gp2a_pm_ops
	},
	.probe		= gp2a_i2c_probe,
	.remove		= gp2a_i2c_remove,
	.id_table	= gp2a_device_id,
	.shutdown	= gp2a_shutdown
};

static int __init gp2a_init(void)
{
	return i2c_add_driver(&gp2a_i2c_driver);
}

static void __exit gp2a_exit(void)
{
	i2c_del_driver(&gp2a_i2c_driver);
}

module_init(gp2a_init);
module_exit(gp2a_exit);

MODULE_AUTHOR("mjchen@sta.samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for gp2ap002a00f");
MODULE_LICENSE("GPL");
