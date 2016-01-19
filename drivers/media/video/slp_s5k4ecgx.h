/*
 * Driver for S5K4ECGX 5M ISP from Samsung
 * Copyright (C) 2012 Samsung Electronics Co., Ltd.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5K4ECGX_H
#define __S5K4ECGX_H

#include <linux/types.h>

#define S5K4ECGX_DRIVER_NAME	"S5K4ECGX"
#define is_focus(__mode)		(state->focus.mode == __mode)

#define FIRST_AF_SEARCH_COUNT   80
#define SECOND_AF_SEARCH_COUNT  80

extern struct class *camera_class;

enum stream_cmd {
	STREAM_STOP,
	STREAM_START,
};

struct s5k4ecgx_framesize {
	u32 index;
	u32 width;
	u32 height;
};

struct s5k4ecgx_exif {
	u32 exptime;		/* us */
	u16 flash;
	u16 iso;
	int tv;			/* shutter speed */
	int bv;			/* brightness */
	int ebv;		/* exposure bias */
};

struct s5k4ecgx_focus {
	unsigned int mode;
	unsigned int lock;
	unsigned int status;
	unsigned int touch;
	unsigned int pos_x;
	unsigned int pos_y;
	unsigned int width;
	unsigned int height;
	unsigned int start;
	bool focusing;
	bool needed;
	bool first;
};

enum preflash_status {
	PREFLASH_NONE = 0,
	PREFLASH_OFF,
	PREFLASH_ON,
};

enum s5k4ecgx_runmode {
	RUNMODE_NOTREADY,
	RUNMODE_INIT,
	RUNMODE_RUNNING, /* previewing */
	RUNMODE_RUNNING_STOP,
	RUNMODE_CAPTURING,
	RUNMODE_CAPTURE_STOP,
};

/*
 * Driver information
 */
struct s5k4ecgx_state {
	struct v4l2_subdev sd;
	struct device *s5k4ecgx_dev;
	struct s5k4ecgx_platform_data *pdata;
	/*
	 * req_fmt is the requested format from the application.
	 * set_fmt is the output format of the camera. Finally FIMC
	 * converts the camera output(set_fmt) to the requested format
	 * with hardware scaler.
	 */
	struct v4l2_pix_format req_fmt;
	const struct s5k4ecgx_framesize *preview_frmsizes;
	const struct s5k4ecgx_framesize *capture_frmsizes;
	struct s5k4ecgx_exif exif;
	struct s5k4ecgx_focus focus;

	enum v4l2_flash_mode flash_mode;
	enum preflash_status preflash;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_scene_mode scene_mode;
	enum v4l2_wb_mode wb_mode;
	enum s5k4ecgx_runmode runmode;

	struct mutex ctrl_lock; /* Mutex */
	struct mutex af_lock; /* Mutex */
	struct work_struct af_work; /* workque for AF */
	struct work_struct af_win_work; /* workque for AF */
	struct workqueue_struct *workqueue; /* workque for AF */

	s32 vt_mode;
	s32 check_dataline;
	u32 req_fps;
	u32 set_fps;
	u32 initialized;
	u32 zoom;
	bool lowlight;
	bool ae_lock;
	bool awb_lock;
};

enum AF_1ST_STATUS {
	AF_PROGRESS_1ST = 1,
	AF_SUCCESS_1ST	,
	AF_FAIL_1ST,
};

enum AF_2ND_STATUS {
	AF_SUCCESS_2ND = 0,
	AF_PROGRESS_2ND = 1,
};

enum s5k4ecgx_prev_frmsize {
	S5K4ECGX_PREVIEW_176, /* 176 x 144 */
	S5K4ECGX_PREVIEW_320, /* 320 x 240 */
	S5K4ECGX_PREVIEW_640, /* 640 x 480 */
	S5K4ECGX_PREVIEW_720, /* 720 x 480 */
	S5K4ECGX_PREVIEW_800, /* 800 x 480 */
	S5K4ECGX_PREVIEW_1280, /* 1280 x 720 */
};

enum s5k4ecgx_cap_frmsize {
	S5K4ECGX_CAPTURE_VGA,	/* 640 x 480 */
	S5K4ECGX_CAPTURE_XGA,	/* 1024 x 768 */
	S5K4ECGX_CAPTURE_1MP,	/* 1280 x 960 */
	S5K4ECGX_CAPTURE_2MP,	/* UXGA - 1600 x 1200 */
	S5K4ECGX_CAPTURE_3MP,	/* QXGA - 2048 x 1536 */
	S5K4ECGX_CAPTURE_5MP,	/* 2560 x 1920 */
};

static inline struct s5k4ecgx_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct s5k4ecgx_state, sd);
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


/************ driver feature ************/
#define S5K4ECGX_USLEEP
/* #define CONFIG_LOAD_FILE */


/*********** Sensor specific ************/
/* #define S5K4ECGX_100MS_DELAY	0xAA55AA5F */
/* #define S5K4ECGX_10MS_DELAY	0xAA55AA5E */
#define S5K4ECGX_DELAY		0xFFFF0000
#define S5K4ECGX_DEF_APEX_DEN	100

/* Register address */
#define REG_PAGE_SHUTTER    0x7000
#define REG_ADDR_SHUTTER    0x14D0
#define REG_PAGE_ISO        0x7000
#define REG_ADDR_ISO        0x14C8

#include  "slp_s5k4ecgx_setfile.h"

#endif /* __S5K4ECGX_H */
