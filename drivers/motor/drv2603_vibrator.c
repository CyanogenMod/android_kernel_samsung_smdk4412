/* drivers/motor/drv2603_vibrator.c
 *
 * Copyright (C) 2012 Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/timed_output.h>
#include <linux/drv2603_vibrator.h>
#include <linux/fs.h>

struct drv2603_vibrator_drvdata {
	struct timed_output_dev dev;
	struct hrtimer timer;
	struct work_struct work;
	struct pwm_device	*pwm;
	struct i2c_client *client;
	int (*gpio_en) (bool) ;
	spinlock_t lock;
	bool running;
	int timeout;
	int max_timeout;
	u8 duty;
	u8 period;
	int pwm_id;
	u16 pwm_duty;
	u16 pwm_period;
};
#ifdef CONFIG_VIBETONZ
struct drv2603_vibrator_drvdata *global_drv2603_data;
#endif
void vibtonz_clk_config(struct drv2603_vibrator_drvdata *data, int duty)
{
	u32 duty_ns = 0;

	duty_ns = (u32)(data->pwm_period * duty)
		/ 1000;

	pwm_config(data->pwm,
		duty_ns,
		data->pwm_period);

}

static enum hrtimer_restart drv2603_vibrator_timer_func(struct hrtimer *_timer)
{
	struct drv2603_vibrator_drvdata *data =
		container_of(_timer, struct drv2603_vibrator_drvdata, timer);

	data->timeout = 0;

	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}

static void drv2603_vibrator_work(struct work_struct *_work)
{
	struct drv2603_vibrator_drvdata *data =
		container_of(_work, struct drv2603_vibrator_drvdata, work);

	if (0 == data->timeout) {
		if (!data->running)
			return ;

		data->running = false;
		pwm_disable(data->pwm);
		pwm_config(data->pwm,
			data->pwm_period / 2,
			data->pwm_period);
		data->gpio_en(false);
	} else {
		if (data->running)
			return ;

		data->running = true;
		data->gpio_en(true);
		pwm_config(data->pwm,
			data->pwm_duty,
			data->pwm_period);
		pwm_enable(data->pwm);
	}
}

static int drv2603_vibrator_get_time(struct timed_output_dev *_dev)
{
	struct drv2603_vibrator_drvdata	*data =
		container_of(_dev, struct drv2603_vibrator_drvdata, dev);

	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static void drv2603_vibrator_enable(struct timed_output_dev *_dev, int value)
{
	struct drv2603_vibrator_drvdata	*data =
		container_of(_dev, struct drv2603_vibrator_drvdata, dev);
	unsigned long	flags;

	printk(KERN_DEBUG "[VIB] time = %dms\n", value);

	cancel_work_sync(&data->work);
	hrtimer_cancel(&data->timer);
	data->timeout = value;
	schedule_work(&data->work);
	spin_lock_irqsave(&data->lock, flags);
	if (value > 0) {
		if (value > data->max_timeout)
			value = data->max_timeout;

		hrtimer_start(&data->timer,
			ns_to_ktime((u64)value * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&data->lock, flags);
}
#ifdef CONFIG_VIBETONZ
void vibtonz_en(bool en)
{
	if (en) {
		if (global_drv2603_data->running)
			return;

		pwm_enable(global_drv2603_data->pwm);

		global_drv2603_data->gpio_en(true);

		global_drv2603_data->running = true;
	} else {
		if (!global_drv2603_data->running)
			return;

		global_drv2603_data->gpio_en(false);

		pwm_disable(global_drv2603_data->pwm);

		global_drv2603_data->running = false;
	}
}
EXPORT_SYMBOL(vibtonz_en);

void vibtonz_pwm(int nForce)
{
	/* add to avoid the glitch issue */
	static int prev_duty;
	int pwm_period = 0, pwm_duty = 0;

	if (global_drv2603_data == NULL) {
		printk(KERN_ERR "[VIB] the motor is not ready!!!");
		return ;
	}

	pwm_period = global_drv2603_data->pwm_period;
	pwm_duty = pwm_period / 2 + ((pwm_period / 2 - 2) * nForce) / 127;

	if (pwm_duty > global_drv2603_data->pwm_duty)
		pwm_duty = global_drv2603_data->pwm_duty;
	else if (pwm_period - pwm_duty > global_drv2603_data->pwm_duty)
		pwm_duty = pwm_period - global_drv2603_data->pwm_duty;

	/* add to avoid the glitch issue */
	if (prev_duty != pwm_duty) {
		prev_duty = pwm_duty;
		pwm_config(global_drv2603_data->pwm, pwm_duty, pwm_period);
	}
}
EXPORT_SYMBOL(vibtonz_pwm);
#endif
static int __devinit drv2603_vibrator_probe(struct platform_device *pdev)
{
	struct drv2603_vibrator_platform_data *pdata = pdev->dev.platform_data;
	struct drv2603_vibrator_drvdata *ddata;
	int ret = 0;

	ddata = kzalloc(sizeof(struct drv2603_vibrator_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		printk(KERN_ERR "[VIB] Failed to alloc memory\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	ddata->pwm = pwm_request(pdata->pwm_id, "vibrator");
	if (IS_ERR(ddata->pwm)) {
		pr_err("[VIB] Failed to request pwm.\n");
		ret = -EFAULT;
		goto err_request_clk;
	}

	ddata->pwm_id = pdata->pwm_id;
	ddata->pwm_duty = pdata->pwm_duty;
	ddata->pwm_period = pdata->pwm_period;
	ddata->max_timeout = pdata->max_timeout;
	ddata->gpio_en = pdata->gpio_en;
	ddata->running = false;

	ddata->dev.name = "vibrator";
	ddata->dev.get_time = drv2603_vibrator_get_time;
	ddata->dev.enable = drv2603_vibrator_enable;
#ifdef CONFIG_VIBETONZ
	global_drv2603_data = ddata;
#endif
	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = drv2603_vibrator_timer_func;
	INIT_WORK(&ddata->work, drv2603_vibrator_work);
	spin_lock_init(&ddata->lock);

	platform_set_drvdata(pdev, ddata);

	ret = timed_output_dev_register(&ddata->dev);
	if (ret < 0) {
		printk(KERN_ERR
			"[VIB] Failed to register timed_output : -%d\n", ret);
		goto err_to_dev_reg;
	}

	return 0;

err_to_dev_reg:
err_request_clk:
	kfree(ddata);
err_free_mem:
	return ret;
}

static int __devexit drv2603_vibrator_remove(struct platform_device *pdev)
{
	struct drv2603_vibrator_drvdata *ddata = platform_get_drvdata(pdev);
	timed_output_dev_unregister(&ddata->dev);
	kfree(ddata);
	return 0;
}

#ifdef CONFIG_PM
static int drv2603_vibrator_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drv2603_vibrator_drvdata *ddata = platform_get_drvdata(pdev);

	cancel_work_sync(&ddata->work);
	hrtimer_cancel(&ddata->timer);

	ddata->timeout = 0;
	ddata->running = false;
	pwm_disable(ddata->pwm);
	pwm_config(ddata->pwm,
		ddata->pwm_period / 2,
		ddata->pwm_period);
	ddata->gpio_en(false);

	return 0;
}

static int drv2603_vibrator_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	struct drv2603_vibrator_drvdata *ddata = platform_get_drvdata(pdev);
	struct drv2603_vibrator_platform_data *pdata = pdev->dev.platform_data;
#endif
	return 0;
}

static const struct dev_pm_ops drv2603_vibrator_pm_ops = {
	.suspend		= drv2603_vibrator_suspend,
	.resume		= drv2603_vibrator_resume,
};
#endif

static struct platform_driver drv2603_vibrator_driver = {
	.probe		= drv2603_vibrator_probe,
	.remove		= __devexit_p(drv2603_vibrator_remove),
	.driver		= {
		.name	= "drv2603_vibrator",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &drv2603_vibrator_pm_ops,
#endif
	}
};

static int __init drv2603_vibrator_init(void)
{
	return platform_driver_register(&drv2603_vibrator_driver);
}

static void __exit drv2603_vibrator_exit(void)
{
	platform_driver_unregister(&drv2603_vibrator_driver);
}

module_init(drv2603_vibrator_init);
module_exit(drv2603_vibrator_exit);
