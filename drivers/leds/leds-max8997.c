/*
 * leds-max8997.c - LED class driver for MAX8997 LEDs.
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * Inspired by leds-regulator driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#include <linux/leds-max8997.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define to_regulator_led(led_cdev) \
	container_of(led_cdev, struct regulator_led, cdev)

struct regulator_led {
	struct led_classdev cdev;
	enum led_brightness value;
	int enabled;
	struct mutex mutex;
	struct work_struct work;

	struct regulator *vcc;
};

static inline int led_regulator_get_max_brightness(struct regulator *supply)
{
	int ret;
	int voltage = regulator_list_voltage(supply, 0);

	/*TODO*/
#if defined(CONFIG_MACH_Q1_BD)  || defined(CONFIG_MACH_U1_NA_USCC)
	return LED_BRIGHTNESS_MAX_LEVEL;
#else
	return 1;
#endif

	if (voltage <= 0)
		return 1;

	/* even if regulator can't change voltages,
	 * we still assume it can change status
	 * and the LED can be turned on and off.
	 */
	ret = regulator_set_voltage(supply, voltage, voltage);
	if (ret < 0)
		return 1;

	return regulator_count_voltages(supply);
}

static int led_regulator_get_voltage(struct regulator *supply,
		enum led_brightness brightness)
{
	if (brightness == 0)
		return -EINVAL;

	return regulator_list_voltage(supply, brightness - 1);
}


static void regulator_led_enable(struct regulator_led *led)
{
	int ret;

	if (led->enabled)
		return;

	ret = regulator_enable(led->vcc);
	if (ret != 0) {
		dev_err(led->cdev.dev, "Failed to enable vcc: %d\n", ret);
		return;
	}

	led->enabled = 1;
}

static void regulator_led_disable(struct regulator_led *led)
{
	int ret;

	if (!led->enabled)
		return;

	ret = regulator_disable(led->vcc);
	if (ret != 0) {
		dev_err(led->cdev.dev, "Failed to disable vcc: %d\n", ret);
		return;
	}

	led->enabled = 0;
}

static void regulator_led_set_value(struct regulator_led *led)
{
	int voltage;
	int ret;
	struct regulator *movie;

	mutex_lock(&led->mutex);

	if (led->cdev.flags & LED_SUSPENDED)
		goto out;

	if (led->value == LED_OFF) {
		regulator_led_disable(led);
		goto out;
	}

	movie = regulator_get(NULL, "led_movie");
	if (IS_ERR(movie)) {
		pr_err("%s: led_movie is failed.\n", __func__);
		goto out;
	}

/* Q1 has torch light widget, and it changes */
/* its brightness level without disabling rerulator */
/* So, remove the below code for Q1 */
#if !defined(CONFIG_MACH_Q1_BD) && !defined(CONFIG_MACH_U1_NA_USCC)
	if (regulator_is_enabled(movie) > 0) {
		pr_info("%s: led_movie is enabled.\n", __func__);
		goto end;
	}
#endif

#if 0 /*TODO*/
	if (led->cdev.max_brightness > 1) {
		voltage = led_regulator_get_voltage(led->vcc, led->value);
		dev_dbg(led->cdev.dev, "brightness: %d voltage: %d\n",
				led->value, voltage);

		ret = regulator_set_voltage(led->vcc, voltage, voltage);
		if (ret != 0)
			dev_err(led->cdev.dev, "Failed to set voltage %d: %d\n",
				voltage, ret);
	}
#endif

#if defined(CONFIG_MACH_Q1_BD)  || defined(CONFIG_MACH_U1_NA_USCC)
	switch (led->value) {
	case LED_BRIGHTNESS_LEVEL1:
		regulator_set_current_limit(led->vcc, 10000, 20000);
		break;
	case LED_BRIGHTNESS_LEVEL2:
	regulator_set_current_limit(led->vcc, 90000, 110000);
		break;
	case LED_BRIGHTNESS_LEVEL3:
		regulator_set_current_limit(led->vcc, 180000, 200000);
		break;
	default:
		pr_err("%s: fail to set current, out of range\n", __func__);
		goto end;
		break;
	}
#else
	regulator_set_current_limit(led->vcc, 90000, 110000);
#endif

	regulator_led_enable(led);

end:
	regulator_put(movie);

out:
	mutex_unlock(&led->mutex);
}

static void led_work(struct work_struct *work)
{
	struct regulator_led *led;

	led = container_of(work, struct regulator_led, work);
	regulator_led_set_value(led);
}

static void regulator_led_brightness_set(struct led_classdev *led_cdev,
			   enum led_brightness value)
{
	struct regulator_led *led = to_regulator_led(led_cdev);

	led->value = value;
	schedule_work(&led->work);
}

static int __devinit regulator_led_probe(struct platform_device *pdev)
{
	struct led_max8997_platform_data *pdata = pdev->dev.platform_data;
	struct regulator_led *led;
	struct regulator *vcc;
	int ret = 0;

	pr_info("%s +\n", __func__);

	if (pdata == NULL) {
		dev_err(&pdev->dev, "no platform data\n");
		return -ENODEV;
	}

	vcc = regulator_get_exclusive(&pdev->dev, "led_torch");
	if (IS_ERR(vcc)) {
		dev_err(&pdev->dev, "Cannot get vcc for %s\n", pdata->name);
		return PTR_ERR(vcc);
	}

	led = kzalloc(sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		ret = -ENOMEM;
		goto err_vcc;
	}

	led->cdev.max_brightness = led_regulator_get_max_brightness(vcc);
	if (pdata->brightness > led->cdev.max_brightness) {
		dev_err(&pdev->dev, "Invalid default brightness %d\n",
				pdata->brightness);
		ret = -EINVAL;
		goto err_led;
	}

	/* Set default brightness */
	led->value = pdata->brightness;

	led->cdev.brightness_set = regulator_led_brightness_set;
	/* Set leds name for class driver */
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;
	led->vcc = vcc;

	mutex_init(&led->mutex);
	INIT_WORK(&led->work, led_work);

	platform_set_drvdata(pdev, led);

	ret = led_classdev_register(&pdev->dev, &led->cdev);
	if (ret < 0) {
		cancel_work_sync(&led->work);
		goto err_led;
	}

	/* to expose the default value to userspace */
	led->cdev.brightness = led->value;

	/* Set the default led status */
	regulator_led_set_value(led);

	pr_info("%s -\n", __func__);

	return 0;

err_led:
	kfree(led);
err_vcc:
	regulator_put(vcc);
	return ret;
}

static int __devexit regulator_led_remove(struct platform_device *pdev)
{
	struct regulator_led *led = platform_get_drvdata(pdev);

	led_classdev_unregister(&led->cdev);
	cancel_work_sync(&led->work);
	regulator_led_disable(led);
	regulator_put(led->vcc);
	kfree(led);
	return 0;
}

static void regulator_led_shutdown(struct platform_device *pdev)
{
	struct regulator_led *led = platform_get_drvdata(pdev);

	if (regulator_is_enabled(led->vcc))
		regulator_led_disable(led);

	return;
}

static struct platform_driver regulator_led_driver = {
	.driver = {
		   .name  = "leds-max8997",
		   .owner = THIS_MODULE,
		   },
	.probe  = regulator_led_probe,
	.remove = __devexit_p(regulator_led_remove),
	.shutdown = regulator_led_shutdown,
};

static int __init regulator_led_init(void)
{
	return platform_driver_register(&regulator_led_driver);
}
module_init(regulator_led_init);

static void __exit regulator_led_exit(void)
{
	platform_driver_unregister(&regulator_led_driver);
}
module_exit(regulator_led_exit);

MODULE_AUTHOR("Byun.C.W");
MODULE_DESCRIPTION("MAX8997 LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-max8997");
