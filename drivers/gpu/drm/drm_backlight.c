/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/backlight.h>
#include <linux/lcd.h>

#include "drm_backlight.h"
#include "drm_mode.h"

static DEFINE_MUTEX(drm_bl_mutex);
static LIST_HEAD(drm_bl_list);

struct drm_bl_data {
	struct device *dev;
	struct list_head list;
	int type;
};

int drm_bl_register(struct device *dev, int type)
{
	struct drm_bl_data *data;

	switch (type) {
	case BL_BACKLIGHT_CLASS:
	case BL_LCD_CLASS:
	case BL_TSP_CLASS:
		break;
	default:
		return -EINVAL;
	}

	data = kzalloc(sizeof(struct drm_bl_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dev = dev;
	data->type = type;

	mutex_lock(&drm_bl_mutex);
	list_add(&data->list, &drm_bl_list);
	mutex_unlock(&drm_bl_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(drm_bl_register);

void drm_bl_unregister(struct device *dev)
{
	struct drm_bl_data *data;

	list_for_each_entry(data, &drm_bl_list, list) {
		if (data->dev == dev) {
			mutex_lock(&drm_bl_mutex);
			list_del(&data->list);
			mutex_unlock(&drm_bl_mutex);
			kfree(data);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(drm_bl_unregister);

/* This is called from dpms function of CRTC or encoder */
void drm_bl_dpms(int mode)
{
	struct drm_bl_data *data;
	struct backlight_device *bd;
	struct lcd_device *ld;
	struct drm_bl_notifier *bl_noti;
	int blank;

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		blank = FB_BLANK_UNBLANK;
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		/* TODO */
	default:
		blank = FB_BLANK_POWERDOWN;
		break;
	}

	list_for_each_entry(data, &drm_bl_list, list) {
		switch (data->type) {
		case BL_BACKLIGHT_CLASS:
			bd = container_of(data->dev, struct backlight_device,
					dev);
			bd->props.power = blank;
			bd->props.fb_blank = blank;
			backlight_update_status(bd);
			break;
		case BL_LCD_CLASS:
			ld = container_of(data->dev, struct lcd_device, dev);
			if (!ld->ops->set_power)
				break;
			ld->ops->set_power(ld, blank);
			break;
		case BL_TSP_CLASS:
			bl_noti = container_of(data->dev,
					struct drm_bl_notifier, dev);
			if (!bl_noti->set_power)
				break;
			bl_noti->set_power(bl_noti->priv, blank);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(drm_bl_dpms);
