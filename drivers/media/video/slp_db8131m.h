/*
 * linux/drivers/media/video/slp_db8131m.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB8131M_H
#define __DB8131M_H

#include <linux/types.h>

extern struct class *camera_class;

#define DB8131M_DRIVER_NAME	"DB8131M"

struct db8131m_framesize {
	u32 width;
	u32 height;
};

struct db8131m_exif {
	u32 shutter_speed;
	u16 iso;
};


/*
 * Driver information
 */
struct db8131m_state {
	struct v4l2_subdev sd;
	struct device *db8131m_dev;
	/*
	 * req_fmt is the requested format from the application.
	 * set_fmt is the output format of the camera. Finally FIMC
	 * converts the camera output(set_fmt) to the requested format
	 * with hardware scaler.
	 */
	struct v4l2_pix_format req_fmt;
	struct db8131m_framesize preview_frmsizes;
	struct db8131m_framesize capture_frmsizes;
	struct db8131m_exif exif;

	enum v4l2_sensor_mode sensor_mode;
	s32 vt_mode;
	s32 check_dataline;
	u32 req_fps;
	u32 set_fps;
	u32 initialized;
};

static inline struct db8131m_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct db8131m_state, sd);
}

/*#define CONFIG_CAM_DEBUG */
#define cam_warn(fmt, ...)	\
	do { \
		printk(KERN_WARNING "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_err(fmt, ...)	\
	do { \
		printk(KERN_ERR "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_info(fmt, ...)	\
	do { \
		printk(KERN_INFO "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#ifdef CONFIG_CAM_DEBUG
#define cam_dbg(fmt, ...)	\
	do { \
		printk(KERN_DEBUG "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)
#else
#define cam_dbg(fmt, ...)
#endif /* CONFIG_CAM_DEBUG */



/*********** Sensor specific ************/
#define DB8131M_DELAY		0xFFFF0000
#define DB8131M_DEF_APEX_DEN	100

/* Register address */
#define REG_PAGE_SHUTTER    0x7000
#define REG_ADDR_SHUTTER    0x14D0
#define REG_PAGE_ISO        0x7000
#define REG_ADDR_ISO        0x14C8

#include  "slp_db8131m_setfile.h"

#endif /* __DB8131M_H */
