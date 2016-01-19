/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/db8131m_platform.h>
#include <linux/videodev2_exynos_camera.h>
 
#ifdef CONFIG_MACH_ZEST
#include "db8131m_reg_zest.h"
#else
#include "db8131m_reg.h"
#endif
 
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
 
#define DB8131M_DRIVER_NAME	DB8131M_DEVICE_NAME
 static const char driver_name[] = DB8131M_DRIVER_NAME;

 /************************************
  * FEATURE DEFINITIONS
  ************************************/
#define CONFIG_LOAD_FILE		false /* for tuning */
 
 /** Debuging Feature **/
#define CONFIG_CAM_DEBUG		true
#define CONFIG_CAM_TRACE		true /* Enable me with CAM_DEBUG */
 
 /* #define VGA_CAM_DEBUG */
 
#ifdef VGA_CAM_DEBUG
#define dev_dbg	dev_err
#endif

#define cam_err(fmt, ...)	\
	printk(KERN_ERR "[""%s""] " fmt, driver_name,##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	printk(KERN_WARNING "[""%s""] " fmt, driver_name, ##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	printk(KERN_INFO "[""%s""] " fmt, driver_name, ##__VA_ARGS__)

#if CONFIG_CAM_DEBUG
#define cam_dbg(fmt, ...)	\
	printk(KERN_DEBUG "[""%s""] " fmt, driver_name, ##__VA_ARGS__)
#else
#define cam_dbg(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_DEBUG) \
			printk(KERN_DEBUG "[""%s""] " fmt, driver_name, ##__VA_ARGS__); \
	} while (0)
#endif

#if CONFIG_CAM_DEBUG && CONFIG_CAM_TRACE
#define cam_trace(fmt, ...)	cam_dbg("%s: " fmt, __func__, ##__VA_ARGS__);
#else
#define cam_trace(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_TRACE) \
			printk(KERN_DEBUG "[""%s""] ""%s: " fmt, \
				driver_name, __func__, ##__VA_ARGS__); \
	} while (0)
#endif

#define CHECK_ERR_COND(condition, ret)	\
	 do { if (unlikely(condition)) return ret; } while (0)
#define CHECK_ERR_COND_MSG(condition, ret, fmt, ...) \
		 if (unlikely(condition)) { \
			 cam_err("%s: error, " fmt, __func__, ##__VA_ARGS__); \
			 return ret; \
		 }
 
#define CHECK_ERR(x)	CHECK_ERR_COND(((x) < 0), (x))
#define CHECK_ERR_MSG(x, fmt, ...) \
	 CHECK_ERR_COND_MSG(((x) < 0), (x), fmt, ##__VA_ARGS__)

 /* Default resolution & pixelformat. plz ref db8131m_platform.h */
#define DEFAULT_RESOLUTION	WVGA		/* Index of resoultion */
#define DEFAUT_FPS_INDEX	DB8131M_15FPS
#define DEFUALT_MCLK            24000000		/* 24MHz default */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define POLL_TIME_MS		10
#define DB8131M_DELAY		0xE700
#define EV_MIN_VLAUE		EV_MINUS_4
#define GET_EV_INDEX(EV)	((EV) - (EV_MIN_VLAUE))
 
#if CONFIG_LOAD_FILE
 
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
 
 static char *db8131m_regs_table;
 static int db8131m_regs_table_size;
 static int db8131m_write_regs_from_sd(char *name, struct i2c_client *client);
 //static int db8131m_i2c_write_multi(unsigned short addr, unsigned int w_data);
 
 struct test {
	 u8 data;
	 struct test *nextBuf;
 };
 static struct test *testBuf;
 static s32 large_file;
#endif
 
 /*
  * Specification
  * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10
  * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
  * Resolution : 1280 (H) x 1024 (V)
  * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
  * Effect : Mono, Negative, Sepia, Aqua, Sketch
  * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
  * Max. pixel clock frequency : 48MHz(upto)
  * Internal PLL (6MHz to 27MHz input frequency)
  */
 
 
 /* Camera functional setting values configured by user concept */
 struct db8131m_userset {
	 int exposure_bias;	 /* V4L2_CID_EXPOSURE */
	 unsigned int ae_lock;
	 unsigned int awb_lock;
	 unsigned int auto_wb;	 /* V4L2_CID_AUTO_WHITE_BALANCE */
	 unsigned int manual_wb; /* V4L2_CID_WHITE_BALANCE_PRESET */
	 unsigned int wb_temp;	 /* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	 unsigned int effect;	 /* Color FX (AKA Color tone) */
	 unsigned int contrast;  /* V4L2_CID_CONTRAST */
	 unsigned int saturation;	 /* V4L2_CID_SATURATION */
	 unsigned int sharpness; /* V4L2_CID_SHARPNESS */
	 unsigned int glamour;
 };
 
 struct db8131m_state {
	 struct db8131m_platform_data *pdata;
	 struct v4l2_subdev sd;
	 struct v4l2_pix_format pix;
	 struct v4l2_fract timeperframe;
	 struct db8131m_userset userset;
	 struct v4l2_streamparm strm;
	 enum v4l2_pix_format_mode format_mode;
	 enum v4l2_sensor_mode sensor_mode;
	 int framesize_index;
	 int freq;		 /* MCLK in KHz */
	 int is_mipi;
	 int isize;
	 int ver;
	 int fps;
	 int vt_mode;		 /*For VT camera */
	 int check_dataline;
	 int check_previewdata;
 };
 
 enum {
	 DB8131M_PREVIEW_QCIF = 0,
	 DB8131M_PREVIEW_QVGA,
	 DB8131M_PREVIEW_CIF,
	 DB8131M_PREVIEW_VGA,
 };
 
 enum {
	 DB8131M_CAPTURE_VGA = 0,
	 DB8131M_CAPTURE_1MP,
 };
 
 
 struct db8131m_enum_framesize {
	 unsigned int index;
	 unsigned int width;
	 unsigned int height;
 };

#define TO_STATE(p, m)	(container_of(p, struct db8131m_state, m))

extern struct class *camera_class;

static inline struct db8131m_state *to_state(struct v4l2_subdev *sd)
{
	return TO_STATE(sd, sd);
}

static int db8131m_init(struct v4l2_subdev *sd, u32 val);

/*********** Sensor specific ************/
#define DB8131M_CHIP_ID			0x6100
#define DB8131M_CHIP_REV		0x06
#define DB8131M_CHIP_REV_OLD		0x04

