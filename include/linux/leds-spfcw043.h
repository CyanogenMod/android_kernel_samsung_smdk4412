/*
 * leds-spfcw043.h - Flash-led driver for SPFCW043
 *
 * Copyright (C) 2011 Samsung Electronics
 * DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_SPFCW043_H__
#define __LEDS_SPFCW043_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define LED_ERROR(x, ...) printk(KERN_ERR "%s : " x, __func__, ##__VA_ARGS__)

enum spfcw043_brightness {
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

enum spfcw043_status {
	STATUS_UNAVAILABLE = 0,
	STATUS_AVAILABLE,
	STATUS_INVALID,
};

#define IOCTL_SPFCW043  'A'
#define IOCTL_SPFCW043_SET_BRIGHTNESS	\
	_IOW(IOCTL_SPFCW043, 0, enum spfcw043_brightness)
#define IOCTL_SPFCW043_GET_STATUS       \
	_IOR(IOCTL_SPFCW043, 1, enum spfcw043_status)
#define IOCTL_SPFCW043_SET_POWER        _IOW(IOCTL_SPFCW043, 2, int)

struct spfcw043_led_platform_data {
	enum spfcw043_brightness brightness;
	enum spfcw043_status status;
	int (*setGpio) (void);
	int (*freeGpio) (void);
	void (*torch_en) (int onoff);
	void (*torch_set) (int onoff);
};
#endif
