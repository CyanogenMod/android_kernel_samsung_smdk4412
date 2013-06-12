/*
 * LED notification trigger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include "leds.h"
#include <linux/android_alarm.h>
#include <linux/wakelock.h>
#include <asm/mach/time.h>
#include <linux/hrtimer.h>
#include <linux/hrtimer.h>

static struct wake_lock ledtrig_rtc_timer_wakelock;

struct notification_trig_data {
	int brightness_on;		/* LED brightness during "on" period.
					 * (LED_OFF < brightness_on <= LED_FULL)
					 */
	unsigned long delay_on;		/* milliseconds on */
	unsigned long delay_off;	/* milliseconds off */
	unsigned long blink_cnt;    /* number of blink times */
	unsigned long off_duration; /* blink stop duration */

	unsigned long current_blink_cnt;    /* current blink count */

	struct timer_list timer;
	struct alarm alarm;
	struct wake_lock wakelock;
};

static int led_rtc_set_alarm(struct led_classdev *led_cdev, unsigned long msec)
{
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
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
	struct notification_trig_data *timer_data = container_of(alarm, struct notification_trig_data, alarm);

	/* let led_timer_function do the actual work */
	mod_timer(&timer_data->timer, jiffies + 1);
}

static void led_timer_function(unsigned long data)
{
	struct led_classdev *led_cdev = (struct led_classdev *) data;
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	unsigned long brightness;
	unsigned long delay;

	if (!timer_data->delay_on || !timer_data->delay_off || !timer_data->blink_cnt) {
		led_set_brightness(led_cdev, LED_OFF);
		return;
	}

	brightness = led_get_brightness(led_cdev);
	if (!brightness) {
		/* Time to switch the LED on. */
		brightness = timer_data->brightness_on;
		delay = timer_data->delay_on;
	} else {
		/* Store the current brightness value to be able
		 * to restore it when the delay_off period is over.
		 */
		timer_data->brightness_on = brightness;
		brightness = LED_OFF;
		delay = timer_data->delay_off;

		if(timer_data->blink_cnt <= ++timer_data->current_blink_cnt) {
			timer_data->current_blink_cnt = 0;
			delay+=timer_data->off_duration;
		}
	}

	led_set_brightness(led_cdev, brightness);

	led_rtc_set_alarm(led_cdev, delay);
}

static ssize_t led_delay_on_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->delay_on);
}

static ssize_t led_delay_on_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->delay_on != state) {
			/* the new value differs from the previous */
			timer_data->delay_on = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			mod_timer(&timer_data->timer, jiffies + 1);
		}
		ret = count;
	}

	return ret;
}

static ssize_t led_delay_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->delay_off);
}

static ssize_t led_delay_off_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->delay_off != state) {
			/* the new value differs from the previous */
			timer_data->delay_off = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			mod_timer(&timer_data->timer, jiffies + 1);
		}
		ret = count;
	}

	return ret;
}

static ssize_t led_blink_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->blink_cnt);
}

static ssize_t led_blink_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->blink_cnt != state) {
			/* the new value differs from the previous */
			timer_data->blink_cnt = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			mod_timer(&timer_data->timer, jiffies + 1);
		}
		ret = count;
	}

	return ret;
}

static ssize_t led_off_duration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->off_duration);
}

static ssize_t led_off_duration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->off_duration != state) {
			/* the new value differs from the previous */
			timer_data->off_duration = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			mod_timer(&timer_data->timer, jiffies + 1);
		}
		ret = count;
	}

	return ret;
}


static DEVICE_ATTR(delay_on, 0777, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0777, led_delay_off_show, led_delay_off_store);
static DEVICE_ATTR(blink_count, 0777, led_blink_count_show, led_blink_count_store);
static DEVICE_ATTR(off_duration, 0777, led_off_duration_show, led_off_duration_store);

static void notification_trig_activate(struct led_classdev *led_cdev)
{
	struct notification_trig_data *timer_data;
	int rc;

	timer_data = kzalloc(sizeof(struct notification_trig_data), GFP_KERNEL);
	if (!timer_data)
		return;

	timer_data->brightness_on = led_get_brightness(led_cdev);
	if (timer_data->brightness_on == LED_OFF)
		timer_data->brightness_on = led_cdev->max_brightness;
	led_cdev->trigger_data = timer_data;

	init_timer(&timer_data->timer);
	timer_data->timer.function = led_timer_function;
	timer_data->timer.data = (unsigned long) led_cdev;

	alarm_init(&timer_data->alarm, ANDROID_ALARM_RTC_WAKEUP,
			led_rtc_timer_function);
	wake_lock_init(&timer_data->wakelock, WAKE_LOCK_SUSPEND, "ledtrig_rtc_timer");

	rc = device_create_file(led_cdev->dev, &dev_attr_delay_on);
	if (rc)
		goto err_out;
	rc = device_create_file(led_cdev->dev, &dev_attr_delay_off);
	if (rc)
		goto err_out_delayon;
	rc = device_create_file(led_cdev->dev, &dev_attr_blink_count);
	if (rc)
		goto err_attr_delay_off;
	rc = device_create_file(led_cdev->dev, &dev_attr_off_duration);
	if (rc)
		goto err_attr_blink_count;

	/* If there is hardware support for blinking, start one
	 * user friendly blink rate chosen by the driver.
	 */
	if (led_cdev->blink_set)
		led_cdev->blink_set(led_cdev,
			&timer_data->delay_on, &timer_data->delay_off);

	return;
err_attr_blink_count:
	device_remove_file(led_cdev->dev, &dev_attr_blink_count);
err_attr_delay_off:
	device_remove_file(led_cdev->dev, &dev_attr_delay_off);
err_out_delayon:
	device_remove_file(led_cdev->dev, &dev_attr_delay_on);
err_out:
	led_cdev->trigger_data = NULL;
	kfree(timer_data);
}

static void notification_trig_deactivate(struct led_classdev *led_cdev)
{
	struct notification_trig_data *timer_data = led_cdev->trigger_data;
	unsigned long on = 0, off = 0;

	if (timer_data) {
		device_remove_file(led_cdev->dev, &dev_attr_delay_on);
		device_remove_file(led_cdev->dev, &dev_attr_delay_off);
		device_remove_file(led_cdev->dev, &dev_attr_blink_count);
		device_remove_file(led_cdev->dev, &dev_attr_off_duration);
		del_timer_sync(&timer_data->timer);
		alarm_cancel(&timer_data->alarm);
		wake_lock_destroy(&timer_data->wakelock);
		kfree(timer_data);
	}

	/* If there is hardware support for blinking, stop it */
	if (led_cdev->blink_set)
		led_cdev->blink_set(led_cdev, &on, &off);
}

static struct led_trigger notification_led_trigger = {
	.name     = "notification",
	.activate = notification_trig_activate,
	.deactivate = notification_trig_deactivate,
};

static int __init notification_trig_init(void)
{
	return led_trigger_register(&notification_led_trigger);
}

static void __exit notification_trig_exit(void)
{
	led_trigger_unregister(&notification_led_trigger);
}

module_init(notification_trig_init);
module_exit(notification_trig_exit);

MODULE_DESCRIPTION("Notification LED trigger");
MODULE_LICENSE("GPL");
