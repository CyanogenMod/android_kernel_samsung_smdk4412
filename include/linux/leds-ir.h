/*
 * leds-ir_led.h - Flash-led driver for AAT 1290A
 *
 * Copyright (C) 2011 Samsung Electronics
 * DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_IR_H__
#define __LEDS_IR_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define LED_ERROR(x, ...) printk(KERN_ERR "%s : " x, __func__, ##__VA_ARGS__)

#define LEDS_IR_PWM_PERIOD	30
#define LEDS_IR_PWM_DEFAULT_ON_TIME		9

enum ir_led_status {
	STATUS_UNAVAILABLE = 0,
	STATUS_AVAILABLE,
	STATUS_INVALID,
};

#define IOCTL_IR_LED  'A'
#define IOCTL_IR_LED_GET_STATUS       \
	_IOR(IOCTL_IR_LED, 1, enum ir_led_status)
#define IOCTL_IR_LED_SET_POWER        _IOW(IOCTL_IR_LED, 2, int)

struct ir_led_platform_data {
	struct workqueue_struct *workqueue;
	struct work_struct ir_led_work;
	struct mutex set_lock;
	enum ir_led_status status;
	int Period;
	int onTime;
	int (*initGpio) (void);
	int (*freeGpio) (void);
	void (*setGpio)(int onoff);
};

ssize_t ir_led_power(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count);

#endif

