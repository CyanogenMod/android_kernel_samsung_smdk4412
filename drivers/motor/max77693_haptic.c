/*
 * haptic motor driver for max77693 - max77673_haptic.c
 *
 * Copyright (C) 2011 ByungChang Cha <bc.cha@samsung.com>
 * Copyright (C) 2012 The CyanogenMod Project
 *                    Daniel Hillenbrand <codeworkx@cyanogenmod.org>
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
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>

static unsigned long pwm_val = 50; /* duty in percent */
static int pwm_duty = 27787; /* duty value, 37050=100%, 27787=50%, 18525=0% */

struct max77693_haptic_data {
	struct max77693_dev *max77693;
	struct i2c_client *i2c;
	struct i2c_client *pmic_i2c;
	struct max77693_haptic_platform_data *pdata;

	struct pwm_device *pwm;
	struct regulator *regulator;

	struct timed_output_dev tout_dev;
	struct hrtimer timer;
	unsigned int timeout;

	struct workqueue_struct *workqueue;
	struct work_struct work;
	spinlock_t lock;
	bool running;
};

struct max77693_haptic_data *g_hap_data;

static void max77693_haptic_i2c(struct max77693_haptic_data *hap_data, bool en)
{
	int ret;
	u8 value = hap_data->pdata->reg2;
	u8 lscnfg_val = 0x00;

	pr_debug("[VIB] %s %d\n", __func__, en);

	if (en) {
		value |= MOTOR_EN;
		lscnfg_val = 0x80;
	}

	ret = max77693_update_reg(hap_data->pmic_i2c, MAX77693_PMIC_REG_LSCNFG,
				lscnfg_val, 0x80);
	if (ret)
		pr_err("[VIB] i2c update error %d\n", ret);

	ret = max77693_write_reg(hap_data->i2c,
				 MAX77693_HAPTIC_REG_CONFIG2, value);
	if (ret)
		pr_err("[VIB] i2c write error %d\n", ret);
}

static int haptic_get_time(struct timed_output_dev *tout_dev)
{
	struct max77693_haptic_data *hap_data
		= container_of(tout_dev, struct max77693_haptic_data, tout_dev);

	struct hrtimer *timer = &hap_data->timer;
	if (hrtimer_active(timer)) {
		ktime_t remain = hrtimer_get_remaining(timer);
		struct timeval t = ktime_to_timeval(remain);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

static void haptic_enable(struct timed_output_dev *tout_dev, int value)
{
	struct max77693_haptic_data *hap_data
		= container_of(tout_dev, struct max77693_haptic_data, tout_dev);

	struct hrtimer *timer = &hap_data->timer;
	unsigned long flags;


	cancel_work_sync(&hap_data->work);
	hrtimer_cancel(timer);
	hap_data->timeout = value;
	queue_work(hap_data->workqueue, &hap_data->work);
	spin_lock_irqsave(&hap_data->lock, flags);
	if (value > 0) {
		pr_debug("%s value %d\n", __func__, value);
		value = min(value, (int)hap_data->pdata->max_timeout);
		hrtimer_start(timer, ns_to_ktime((u64)value * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&hap_data->lock, flags);
#ifdef SEC_DEBUG_VIB
	printk(KERN_DEBUG "[VIB] haptic_enable is called\n");
#endif
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct max77693_haptic_data *hap_data
		= container_of(timer, struct max77693_haptic_data, timer);
	unsigned long flags;

	hap_data->timeout = 0;
	queue_work(hap_data->workqueue, &hap_data->work);
	return HRTIMER_NORESTART;
}

static int vibetonz_clk_on(struct device *dev, bool en)
{
	struct clk *vibetonz_clk = NULL;
	vibetonz_clk = clk_get(dev, "timers");
	pr_debug("[VIB] DEV NAME %s %lu\n",
		 dev_name(dev), clk_get_rate(vibetonz_clk));

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

static void haptic_work(struct work_struct *work)
{
	struct max77693_haptic_data *hap_data
		= container_of(work, struct max77693_haptic_data, work);

	pr_debug("[VIB] %s\n", __func__);
	if (hap_data->timeout > 0) {
		if (hap_data->running)
			return;

		max77693_haptic_i2c(hap_data, true);

		pwm_config(hap_data->pwm, pwm_duty, hap_data->pdata->period);
        pr_info("[VIB] %s: pwm_config duty=%d\n", __func__, pwm_duty);
		pwm_enable(hap_data->pwm);

		if (hap_data->pdata->motor_en)
			hap_data->pdata->motor_en(true);

		regulator_enable(hap_data->regulator);

		hap_data->running = true;
	} else {
		if (!hap_data->running)
			return;

		if (hap_data->pdata->motor_en)
			hap_data->pdata->motor_en(false);
#ifdef CONFIG_MACH_GC1
		regulator_disable(hap_data->regulator);
#else
		regulator_force_disable(hap_data->regulator);
#endif
		pwm_disable(hap_data->pwm);

		max77693_haptic_i2c(hap_data, false);

		hap_data->running = false;
	}
#ifdef SEC_DEBUG_VIB
	printk(KERN_DEBUG "[VIB] haptic_work is called\n");
#endif
	return;
}

#ifdef CONFIG_VIBETONZ
void vibtonz_en(bool en)
{
	if (g_hap_data == NULL) {
		pr_err("[VIB] %s: the motor is not ready!!!", __func__);
		return ;
	}

	if (en) {
		if (g_hap_data->running)
			return;

		max77693_haptic_i2c(g_hap_data, true);
		pwm_enable(g_hap_data->pwm);

		if (g_hap_data->pdata->motor_en)
			g_hap_data->pdata->motor_en(true);
		else
			regulator_enable(g_hap_data->regulator);

		g_hap_data->running = true;
	} else {
		if (!g_hap_data->running)
			return;

		if (g_hap_data->pdata->motor_en)
			g_hap_data->pdata->motor_en(false);
		else
			regulator_force_disable(g_hap_data->regulator);

		pwm_disable(g_hap_data->pwm);

		max77693_haptic_i2c(g_hap_data, false);

		g_hap_data->running = false;
	}
#ifdef SEC_DEBUG_VIB
	printk(KERN_DEBUG "[VIB] vibtonz_en is called\n");
#endif
}
EXPORT_SYMBOL(vibtonz_en);

void vibtonz_pwm(int nForce)
{
	/* add to avoid the glitch issue */
	static int prev_duty;
	int pwm_period = 0;

	if (g_hap_data == NULL) {
		pr_err("[VIB] %s: the motor is not ready!!!", __func__);
		return ;
	}

	/* add to avoid the glitch issue */
	if (prev_duty != pwm_duty) {
		prev_duty = pwm_duty;

        pr_debug("[VIB] %s: setting pwm_duty=%d", __func__, pwm_duty);
		pwm_config(g_hap_data->pwm, pwm_duty, pwm_period);
	}
#ifdef SEC_DEBUG_VIB
	printk(KERN_DEBUG "[VIB] vibtonz_pwm is called(%d)\n", nForce);
#endif
}
EXPORT_SYMBOL(vibtonz_pwm);
#endif

static ssize_t pwm_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count;

	pwm_val = ((pwm_duty - 18525) * 100) / 18525;

	count = sprintf(buf, "%lu\n", pwm_val);
	pr_debug("[VIB] pwm_value: %lu\n", pwm_val);

	return count;
}

ssize_t pwm_value_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	if (kstrtoul(buf, 0, &pwm_val))
		pr_err("[VIB] %s: error on storing pwm_val\n", __func__); 

	pr_info("[VIB] %s: pwm_value=%lu\n", __func__, pwm_val);

	pwm_duty = (pwm_val * 18525) / 100 + 18525;

	/* make sure new pwm duty is in range */
	if(pwm_duty > 37050)
	{
		pwm_duty = 37050;
	}
	else if (pwm_duty < 18525)
	{
		pwm_duty = 18525;
	}

	pr_info("[VIB] %s: pwm_duty=%d\n", __func__, pwm_duty);

	return size;
}
static DEVICE_ATTR(pwm_value, S_IRUGO | S_IWUSR,
		pwm_value_show, pwm_value_store);

static int max77693_haptic_probe(struct platform_device *pdev)
{
	int error = 0;
	struct max77693_dev *max77693 = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *max77693_pdata
		= dev_get_platdata(max77693->dev);
	struct max77693_haptic_platform_data *pdata
		= max77693_pdata->haptic_data;
	struct max77693_haptic_data *hap_data;

	pr_debug("[VIB] ++ %s\n", __func__);
	 if (pdata == NULL) {
		pr_err("%s: no pdata\n", __func__);
		return -ENODEV;
	}

	hap_data = kzalloc(sizeof(struct max77693_haptic_data), GFP_KERNEL);
	if (!hap_data)
		return -ENOMEM;

	platform_set_drvdata(pdev, hap_data);
	g_hap_data = hap_data;
	hap_data->max77693 = max77693;
	hap_data->i2c = max77693->haptic;
	hap_data->pmic_i2c = max77693->i2c;
	hap_data->pdata = pdata;

	hap_data->workqueue = create_singlethread_workqueue("hap_work");
	INIT_WORK(&(hap_data->work), haptic_work);
	spin_lock_init(&(hap_data->lock));

	hap_data->pwm = pwm_request(hap_data->pdata->pwm_id, "vibrator");
	if (IS_ERR(hap_data->pwm)) {
		pr_err("[VIB] Failed to request pwm\n");
		error = -EFAULT;
		goto err_pwm_request;
	}
	pwm_config(hap_data->pwm, pdata->period / 2, pdata->period);

	vibetonz_clk_on(&pdev->dev, true);

	if (pdata->init_hw)
		pdata->init_hw();

	hap_data->regulator
		= regulator_get(NULL, pdata->regulator_name);

	if (IS_ERR(hap_data->regulator)) {
		pr_err("[VIB] Failed to get vmoter regulator.\n");
		error = -EFAULT;
		goto err_regulator_get;
	}

	/* hrtimer init */
	hrtimer_init(&hap_data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hap_data->timer.function = haptic_timer_func;

	/* timed_output_dev init*/
	hap_data->tout_dev.name = "vibrator";
	hap_data->tout_dev.get_time = haptic_get_time;
	hap_data->tout_dev.enable = haptic_enable;

#ifdef CONFIG_ANDROID_TIMED_OUTPUT
	error = timed_output_dev_register(&hap_data->tout_dev);
	if (error < 0) {
		pr_err("[VIB] Failed to register timed_output : %d\n", error);
		error = -EFAULT;
		goto err_timed_output_register;
	}

	pr_err("[VIB] timed_output device is registrated\n");

	/* User controllable pwm level */
	error = device_create_file(hap_data->tout_dev.dev, &dev_attr_pwm_value);
	if (error < 0) {
		pr_err("[VIB] create sysfs fail: pwm_value\n");
	}
#endif
	pr_debug("[VIB] -- %s\n", __func__);

	return error;

err_timed_output_register:
	regulator_put(hap_data->regulator);
err_regulator_get:
	pwm_free(hap_data->pwm);
err_pwm_request:
	kfree(hap_data);
	g_hap_data = NULL;
	return error;
}

static int __devexit max77693_haptic_remove(struct platform_device *pdev)
{
	struct max77693_haptic_data *data = platform_get_drvdata(pdev);
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
	timed_output_dev_unregister(&data->tout_dev);
#endif
	regulator_put(data->regulator);
	pwm_free(data->pwm);
	destroy_workqueue(data->workqueue);
	kfree(data);
	g_hap_data = NULL;

	return 0;
}

static int max77693_haptic_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	vibetonz_clk_on(&pdev->dev, false);
	return 0;
}
static int max77693_haptic_resume(struct platform_device *pdev)
{
	vibetonz_clk_on(&pdev->dev, true);
	return 0;
}

static struct platform_driver max77693_haptic_driver = {
	.probe		= max77693_haptic_probe,
	.remove		= max77693_haptic_remove,
	.suspend	= max77693_haptic_suspend,
	.resume		= max77693_haptic_resume,
	.driver = {
		.name	= "max77693-haptic",
		.owner	= THIS_MODULE,
	},
};

static int __init max77693_haptic_init(void)
{
	pr_debug("[VIB] %s\n", __func__);
	return platform_driver_register(&max77693_haptic_driver);
}
module_init(max77693_haptic_init);

static void __exit max77693_haptic_exit(void)
{
	platform_driver_unregister(&max77693_haptic_driver);
}
module_exit(max77693_haptic_exit);

MODULE_AUTHOR("ByungChang Cha <bc.cha@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MAX77693 haptic driver");
