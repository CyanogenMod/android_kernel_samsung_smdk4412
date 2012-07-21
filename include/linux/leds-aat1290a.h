/*
 * leds-aat1290a.h - Flash-led driver for AAT 1290A
 *
 * Copyright (C) 2011 Samsung Electronics
 * DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_AAT1290A_H__
#define __LEDS_AAT1290A_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define LED_ERROR(x, ...) printk(KERN_ERR "%s : " x, __func__, ##__VA_ARGS__)

enum aat1290a_brightness {
	TORCH_BRIGHTNESS_100 = 1,
	TORCH_BRIGHTNESS_89,
	TORCH_BRIGHTNESS_79,
	TORCH_BRIGHTNESS_71,
	TORCH_BRIGHTNESS_63,
	TORCH_BRIGHTNESS_56,
	TORCH_BRIGHTNESS_50,
	TORCH_BRIGHTNESS_45,
	TORCH_BRIGHTNESS_40,
	TORCH_BRIGHTNESS_36,
	TORCH_BRIGHTNESS_32,
	TORCH_BRIGHTNESS_28,
	TORCH_BRIGHTNESS_25,
	TORCH_BRIGHTNESS_22,
	TORCH_BRIGHTNESS_20,
	TORCH_BRIGHTNESS_0,
	TORCH_BRIGHTNESS_INVALID,
};

enum aat1290a_status {
	STATUS_UNAVAILABLE = 0,
	STATUS_AVAILABLE,
	STATUS_INVALID,
};

#define IOCTL_AAT1290A  'A'
#define IOCTL_AAT1290A_SET_BRIGHTNESS	\
	_IOW(IOCTL_AAT1290A, 0, enum aat1290a_brightness)
#define IOCTL_AAT1290A_GET_STATUS       \
	_IOR(IOCTL_AAT1290A, 1, enum aat1290a_status)
#define IOCTL_AAT1290A_SET_POWER        _IOW(IOCTL_AAT1290A, 2, int)

struct aat1290a_led_platform_data {
	enum aat1290a_brightness brightness;
	enum aat1290a_status status;
	void (*switch_sel) (int enable);
	int (*initGpio) (void);
	int (*setGpio) (void);
	int (*freeGpio) (void);
	void (*torch_en) (int onoff);
	void (*torch_set) (int onoff);
};

ssize_t aat1290a_power(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count);

#endif
