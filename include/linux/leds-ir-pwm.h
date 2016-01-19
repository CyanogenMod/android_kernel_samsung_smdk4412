/*
 * leds-ir-pwm.h
 *
 * Copyright (C) 2013 Samsung Electronics
 * Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_IR_PWM_H__
#define __LEDS_IR_PWM_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define LED_ERROR(x, ...) printk(KERN_ERR "%s : " x, __func__, ##__VA_ARGS__)
#define LED_INFO(x, ...) printk(KERN_INFO "%s : " x, __func__, ##__VA_ARGS__)

#define LEDS_IR_PWM_PERIOD	30
#define LEDS_IR_PWM_DEFAULT_ON_TIME		9
#define LEDS_IR_PWM_DEFAULT_OFF		0

enum ir_led_status {
	STATUS_UNAVAILABLE = 0,
	STATUS_AVAILABLE,
	STATUS_INVALID,
};

struct ir_led_pwm_platform_data {
	int (*power_en) (bool) ;
	int pwm_id;
	int max_timeout;
	unsigned int pwm_duty;
	unsigned int pwm_period;
};
#endif


