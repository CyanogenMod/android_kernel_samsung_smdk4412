/*
 * haptic motor driver for max8997 - max8997_vibrator.c
 *
 * Copyright (C) 2011 Unknown Samsung Employees (Original file was missing copyright header)
 * Copyright (C) 2012 The CyanogenMod Project
 *                    Daniel Hillenbrand <codeworkx@cyanogenmod.com>
 *                    Andrew Dodd <atd7@cornell.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timed_output.h>
#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>

#ifdef CONFIG_MACH_Q1_BD
#include "mach/gpio.h"
#endif

static int pwm_duty_max;
static int pwm_duty_min;

static unsigned long pwm_val = 50; /* duty in percent */
static int pwm_duty; /* duty value */

struct vibrator_drvdata {
	struct max8997_motor_data *pdata;
	struct pwm_device	*pwm;
	struct regulator *regulator;
	struct i2c_client *client;
	struct timed_output_dev dev;
	struct hrtimer timer;
	struct work_struct work;
	spinlock_t lock;
	bool running;
	int timeout;
};

#ifdef CONFIG_VIBETONZ
static struct vibrator_drvdata *g_data;
#endif

static int vibetonz_clk_on(struct device *dev, bool en)
{
	struct clk *vibetonz_clk = NULL;
	vibetonz_clk = clk_get(dev, "timers");
	if (IS_ERR(vibetonz_clk)) {
		pr_err("[VIB] failed to get clock for the motor\n");
		goto err_clk_get;
	}
	if (en)
		clk_enable(vibetonz_clk);
	else
		clk_disable(vibetonz_clk);
	clk_put(vibetonz_clk);
	return 0;

err_clk_get:
	clk_put(vibetonz_clk);
	return -EINVAL;
}

static void i2c_max8997_hapticmotor(struct vibrator_drvdata *data, bool en)
{
	int ret;
	u8 value = data->pdata->reg2;

	/* the setting of reg2 should be set */
	if (0 == value)
#if defined(CONFIG_MACH_P2)
		value = MOTOR_LRA | EXT_PWM | DIVIDER_128;
#else
		value = MOTOR_LRA | EXT_PWM | DIVIDER_256;
#endif

	if (en)
		value |= MOTOR_EN;

	ret = max8997_write_reg(data->client,
				 MAX8997_MOTOR_REG_CONFIG2, value);
	if (ret < 0)
		pr_err("[VIB] i2c write error\n");
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *_timer)
{
	struct vibrator_drvdata *data =
		container_of(_timer, struct vibrator_drvdata, timer);

	data->timeout = 0;

	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}

static void vibrator_work(struct work_struct *_work)
{
	struct vibrator_drvdata *data =
		container_of(_work, struct vibrator_drvdata, work);

	pr_debug("[VIB] time = %dms\n", data->timeout);

	if (0 == data->timeout) {
		if (!data->running)
			return ;
		pwm_disable(data->pwm);
		i2c_max8997_hapticmotor(data, false);
		if (data->pdata->motor_en)
			data->pdata->motor_en(false);
		else
			regulator_force_disable(data->regulator);
		data->running = false;

	} else {
		if (data->running)
			return ;
		if (data->pdata->motor_en)
			data->pdata->motor_en(true);
		else
			regulator_enable(data->regulator);
		i2c_max8997_hapticmotor(data, true);
		pwm_config(data->pwm, pwm_duty, data->pdata->period);
		pr_info("[VIB] %s: pwm_config duty=%d\n", __func__, pwm_duty);
		pwm_enable(data->pwm);

		data->running = true;
	}
}

static int vibrator_get_time(struct timed_output_dev *_dev)
{
	struct vibrator_drvdata	*data =
		container_of(_dev, struct vibrator_drvdata, dev);

	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static void vibrator_enable(struct timed_output_dev *_dev, int value)
{
	struct vibrator_drvdata	*data =
		container_of(_dev, struct vibrator_drvdata, dev);
	unsigned long	flags;

	cancel_work_sync(&data->work);
	hrtimer_cancel(&data->timer);
	data->timeout = value;
	schedule_work(&data->work);
	spin_lock_irqsave(&data->lock, flags);
	if (value > 0) {
		if (value > data->pdata->max_timeout)
			value = data->pdata->max_timeout;

		hrtimer_start(&data->timer,
			ns_to_ktime((u64)value * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&data->lock, flags);
}

#ifdef CONFIG_VIBETONZ
void vibtonz_en(bool en)
{
	struct vibrator_drvdata	*data = g_data;

	if (en) {
		if (data->running)
			return ;
		if (data->pdata->motor_en)
			data->pdata->motor_en(true);
		else
			regulator_enable(data->regulator);
		i2c_max8997_hapticmotor(data, true);
		pwm_enable(data->pwm);
		data->running = true;
	} else {
		if (!data->running)
			return ;

		pwm_disable(data->pwm);
		i2c_max8997_hapticmotor(data, false);
		if (data->pdata->motor_en)
			data->pdata->motor_en(false);
		else
			regulator_force_disable(data->regulator);
		data->running = false;
	}
}
EXPORT_SYMBOL(vibtonz_en);

void vibtonz_pwm(int nForce)
{
	struct vibrator_drvdata	*data = g_data;
	/* add to avoid the glitch issue */
	static int prev_duty;
	int pwm_period = data->pdata->period;

#if defined(CONFIG_MACH_P4)
	if (pwm_duty > data->pdata->duty)
		pwm_duty = data->pdata->duty;
	else if (pwm_period - pwm_duty > data->pdata->duty)
		pwm_duty = pwm_period - data->pdata->duty;
#endif

	/* add to avoid the glitch issue */
	if (prev_duty != pwm_duty) {
		prev_duty = pwm_duty;
		pr_debug("[VIB] %s: setting pwm_duty=%d", __func__, pwm_duty);
		pwm_config(data->pwm, pwm_duty, pwm_period);
	}
}
EXPORT_SYMBOL(vibtonz_pwm);

static ssize_t pwm_val_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count;

	pwm_val = ((pwm_duty - pwm_duty_min) * 100) / pwm_duty_min;

	count = sprintf(buf, "%lu\n", pwm_val);
	pr_debug("[VIB] pwm_val: %lu\n", pwm_val);

	return count;
}

ssize_t pwm_val_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	if (kstrtoul(buf, 0, &pwm_val))
		pr_err("[VIB] %s: error on storing pwm_val\n", __func__); 

	pr_info("[VIB] %s: pwm_val=%lu\n", __func__, pwm_val);

	pwm_duty = (pwm_val * pwm_duty_min) / 100 + pwm_duty_min;

	/* make sure new pwm duty is in range */
	if(pwm_duty > pwm_duty_max) {
		pwm_duty = pwm_duty_max;
	}
	else if (pwm_duty < pwm_duty_min) {
		pwm_duty = pwm_duty_min;
	}

	pr_info("[VIB] %s: pwm_duty=%d\n", __func__, pwm_duty);

	return size;
}
static DEVICE_ATTR(pwm_val, S_IRUGO | S_IWUSR,
		pwm_val_show, pwm_val_store);
#endif

static int create_vibrator_sysfs(void)
{
	int ret;
	struct kobject *vibrator_kobj;
	vibrator_kobj = kobject_create_and_add("vibrator", NULL);
	if (unlikely(!vibrator_kobj))
		return -ENOMEM;

	ret = sysfs_create_file(vibrator_kobj,
			&dev_attr_pwm_val.attr);
	if (unlikely(ret < 0)) {
		pr_err("[VIB] sysfs_create_file failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int __devinit vibrator_probe(struct platform_device *pdev)
{
	struct max8997_dev *max8997 = dev_get_drvdata(pdev->dev.parent);
	struct max8997_platform_data *max8997_pdata
		= dev_get_platdata(max8997->dev);
	struct max8997_motor_data *pdata = max8997_pdata->motor;
	struct vibrator_drvdata *ddata;
	int error = 0;

	ddata = kzalloc(sizeof(struct vibrator_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		pr_err("[VIB] Failed to alloc memory\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	if (pdata->init_hw)
		pdata->init_hw();
	else {
		ddata->regulator = regulator_get(NULL, "vmotor");
		if (IS_ERR(ddata->regulator)) {
			pr_err("[VIB] Failed to get vmoter regulator.\n");
			error = -EFAULT;
			goto err_regulator_get;
		}
	}

	ddata->pdata = pdata;
	ddata->dev.name = "vibrator";
	ddata->dev.get_time = vibrator_get_time;
	ddata->dev.enable = vibrator_enable;
	ddata->client = max8997->hmotor;

	platform_set_drvdata(pdev, ddata);

	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = vibrator_timer_func;
	INIT_WORK(&ddata->work, vibrator_work);
	spin_lock_init(&ddata->lock);

	create_vibrator_sysfs();

	ddata->pwm = pwm_request(pdata->pwm_id, "vibrator");
	if (IS_ERR(ddata->pwm)) {
		pr_err("[VIB] Failed to request pwm.\n");
		error = -EFAULT;
		goto err_pwm_request;
	}
	pwm_config(ddata->pwm,
		ddata->pdata->period/2, ddata->pdata->period);

	vibetonz_clk_on(&pdev->dev, true);

	error = timed_output_dev_register(&ddata->dev);
	if (error < 0) {
		pr_err("[VIB] Failed to register timed_output : %d\n", error);
		error = -EFAULT;
		goto err_timed_output_register;
	}

#ifdef CONFIG_VIBETONZ
	g_data = ddata;
	pwm_duty_max = g_data->pdata->duty;
	pwm_duty_min = pwm_duty_max/2;
	pwm_duty = (pwm_duty_min + pwm_duty_max)/2;
#endif

	return 0;

err_timed_output_register:
	timed_output_dev_unregister(&ddata->dev);
err_regulator_get:
	regulator_put(ddata->regulator);
err_pwm_request:
	pwm_free(ddata->pwm);
err_free_mem:
	kfree(ddata);
	return error;
}

static int __devexit vibrator_remove(struct platform_device *pdev)
{
	struct vibrator_drvdata *data = platform_get_drvdata(pdev);
	timed_output_dev_unregister(&data->dev);
#ifdef CONFIG_MACH_Q1_BD
	gpio_free(GPIO_MOTOR_EN);
#else
	regulator_put(data->regulator);
#endif
	pwm_free(data->pwm);
	kfree(data);
	return 0;
}

static int vibrator_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	vibetonz_clk_on(&pdev->dev, false);
	return 0;
}
static int vibrator_resume(struct platform_device *pdev)
{
	vibetonz_clk_on(&pdev->dev, true);
	return 0;
}

static struct platform_driver vibrator_driver = {
	.probe	= vibrator_probe,
	.remove	= __devexit_p(vibrator_remove),
	.suspend = vibrator_suspend,
	.resume	= vibrator_resume,
	.driver	= {
		.name	= "max8997-hapticmotor",
		.owner	= THIS_MODULE,
	}
};

static int __init vibrator_init(void)
{
	return platform_driver_register(&vibrator_driver);
}

static void __exit vibrator_exit(void)
{
	platform_driver_unregister(&vibrator_driver);
}

late_initcall(vibrator_init);
module_exit(vibrator_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vibrator driver");
