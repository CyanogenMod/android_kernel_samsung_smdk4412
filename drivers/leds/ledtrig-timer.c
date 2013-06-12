/*
 * LED Kernel Timer Trigger
 *
 * Copyright 2005-2006 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include "leds.h"

#ifdef CONFIG_TARGET_LOCALE_NA
#define CONFIG_RTC_LEDTRIG_TIMER 1//chief.rtc.ledtrig
#endif /* CONFIG_TARGET_LOCALE_NA */

 #ifdef CONFIG_TARGET_LOCALE_NA
#if defined(CONFIG_RTC_LEDTRIG_TIMER)
#include <linux/android_alarm.h>
#include <linux/wakelock.h>
#include <asm/mach/time.h>
#include <linux/hrtimer.h>
#include <linux/hrtimer.h>
#endif

#if defined(CONFIG_RTC_LEDTRIG_TIMER)
static struct wake_lock ledtrig_rtc_timer_wakelock;
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */

struct timer_trig_data {
	int brightness_on;		/* LED brightness during "on" period.
					 * (LED_OFF < brightness_on <= LED_FULL)
					 */
	unsigned long delay_on;		/* milliseconds on */
	unsigned long delay_off;	/* milliseconds off */
	struct timer_list timer;
#ifdef CONFIG_TARGET_LOCALE_NA
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
	struct alarm alarm;
	struct wake_lock wakelock;
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
	struct hrtimer hrtimer;
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */
};


#ifdef CONFIG_TARGET_LOCALE_NA
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
static int led_rtc_set_alarm(struct led_classdev *led_cdev, unsigned long msec)
{
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	ktime_t expire;
	ktime_t now;

	now = ktime_get_real();
	expire = ktime_add(now, ns_to_ktime((u64)msec*1000*1000));

	alarm_start_range(&timer_data->alarm, expire, expire);
	if(msec < 1500) {
		/* If expire time is less than 1.5s keep a wake lock to prevent constant
		 * suspend fail. RTC alarm fails to suspend if the earliest expiration
		 * time is less than a second. Keep the wakelock just a jiffy more than
		 * the expire time to prevent wake lock timeout. */
		wake_lock_timeout(&timer_data->wakelock, (msec*HZ/1000)+1);
	}
	return 0;
}

static void led_rtc_timer_function(struct alarm *alarm)
{
	struct timer_trig_data *timer_data = container_of(alarm, struct timer_trig_data, alarm);

	/* let led_timer_function do the actual work */
	mod_timer(&timer_data->timer, jiffies + 1);
}
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
extern int msm_pm_request_wakeup(struct hrtimer *timer);
static enum hrtimer_restart ledtrig_hrtimer_function(struct hrtimer *timer)
{
	struct timer_trig_data *timer_data = container_of(timer, struct timer_trig_data, hrtimer);

	/* let led_timer_function do the actual work */
	mod_timer(&timer_data->timer, jiffies + 1);
}
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */


static ssize_t led_delay_on_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	return sprintf(buf, "%lu\n", led_cdev->blink_delay_on);
}

static ssize_t led_delay_on_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		led_blink_set(led_cdev, &state, &led_cdev->blink_delay_off);
		led_cdev->blink_delay_on = state;
		ret = count;
	}

	return ret;
}

static ssize_t led_delay_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	return sprintf(buf, "%lu\n", led_cdev->blink_delay_off);
}

static ssize_t led_delay_off_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		led_blink_set(led_cdev, &led_cdev->blink_delay_on, &state);
		led_cdev->blink_delay_off = state;
		ret = count;
	}

	return ret;
}

static DEVICE_ATTR(delay_on, 0644, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0644, led_delay_off_show, led_delay_off_store);

static void timer_trig_activate(struct led_classdev *led_cdev)
{
	int rc;

	led_cdev->trigger_data = NULL;

	rc = device_create_file(led_cdev->dev, &dev_attr_delay_on);
	if (rc)
		return;
	rc = device_create_file(led_cdev->dev, &dev_attr_delay_off);
	if (rc)
		goto err_out_delayon;

	led_blink_set(led_cdev, &led_cdev->blink_delay_on,
		      &led_cdev->blink_delay_off);

	led_cdev->trigger_data = (void *)1;

	return;

err_out_delayon:
	device_remove_file(led_cdev->dev, &dev_attr_delay_on);
}

static void timer_trig_deactivate(struct led_classdev *led_cdev)
{
	if (led_cdev->trigger_data) {
		device_remove_file(led_cdev->dev, &dev_attr_delay_on);
		device_remove_file(led_cdev->dev, &dev_attr_delay_off);
	}

	/* Stop blinking */
	led_brightness_set(led_cdev, LED_OFF);
}

static struct led_trigger timer_led_trigger = {
	.name     = "timer",
	.activate = timer_trig_activate,
	.deactivate = timer_trig_deactivate,
};

static int __init timer_trig_init(void)
{
	return led_trigger_register(&timer_led_trigger);
}

static void __exit timer_trig_exit(void)
{
	led_trigger_unregister(&timer_led_trigger);
}

module_init(timer_trig_init);
module_exit(timer_trig_exit);

MODULE_AUTHOR("Richard Purdie <rpurdie@openedhand.com>");
MODULE_DESCRIPTION("Timer LED trigger");
MODULE_LICENSE("GPL");
