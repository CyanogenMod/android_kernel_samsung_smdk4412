/* drivers/media/video/sr130pc20.h
 *
 * Driver for sr130pc20 (1.5MP Camera) from siliconfile
 *
 * Copyright (C) 2010, SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * - change date: 2012.06.28
 */

#ifndef __SR130PC20_H__
#define __SR130PC20_H__
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/sr130pc20_platform.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/workqueue.h>

#define SR130PC20_DRIVER_NAME	"SR130PC20"

#define SR130PC20_DELAY		0xFFFF0000

/************************************
 * FEATURE DEFINITIONS
 ************************************/
#define CONFIG_CAM_YUV_CAPTURE
#define CONFIG_CAM_I2C_LITTLE_ENDIAN
/* #define CONFIG_LOAD_FILE *//* for tuning */
/* #define CONFIG_DEBUG_NO_FRAME */

/** Debuging Feature **/
// #define CONFIG_CAM_DEBUG */
// #define CONFIG_CAM_TRACE *//* Enable it with CONFIG_CAM_DEBUG */
// #define CONFIG_CAM_AF_DEBUG *//* Enable it with CONFIG_CAM_DEBUG */
#define DEBUG_WRITE_REGS
/***********************************/

#ifdef CONFIG_VIDEO_SR130PC20_DEBUG
enum {
	SR130PC20_DEBUG_I2C		= 1U << 0,
	SR130PC20_DEBUG_I2C_BURSTS	= 1U << 1,
};
static uint32_t sr130pc20_debug_mask = SR130PC20_DEBUG_I2C_BURSTS;
module_param_named(debug_mask, sr130pc20_debug_mask, uint, S_IWUSR | S_IRUGO);

#define sr130pc20_debug(mask, x...) \
	do { \
		if (sr130pc20_debug_mask & mask) \
			pr_info(x);	\
	} while (0)
#else
#define sr130pc20_debug(mask, x...)
#endif

#define TAG_NAME	"["SR130PC20_DRIVER_NAME"]"" "

/* Define debug level */
#define CAMDBG_LEVEL_ERR		(1 << 0)
#define CAMDBG_LEVEL_WARN		(1 << 1)
#define CAMDBG_LEVEL_INFO		(1 << 2)
#define CAMDBG_LEVEL_DEBUG		(1 << 3)
#define CAMDBG_LEVEL_TRACE		(1 << 4)
#define CAMDBG_LEVEL_DEFAULT	\
	(CAMDBG_LEVEL_ERR | CAMDBG_LEVEL_WARN | CAMDBG_LEVEL_INFO)

#define cam_err(fmt, ...)	\
	printk(KERN_ERR TAG_NAME fmt, ##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	printk(KERN_WARNING TAG_NAME fmt, ##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	printk(KERN_INFO TAG_NAME fmt, ##__VA_ARGS__)

#if defined(CONFIG_CAM_DEBUG)
#define cam_dbg(fmt, ...)	\
	printk(KERN_DEBUG TAG_NAME fmt, ##__VA_ARGS__)
#else
#define cam_dbg(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_DEBUG) \
			printk(KERN_DEBUG TAG_NAME fmt, ##__VA_ARGS__); \
	} while (0)
#endif

#if defined(CONFIG_CAM_DEBUG) && defined(CONFIG_CAM_TRACE)
#define cam_trace(fmt, ...)	cam_dbg("%s: " fmt, __func__, ##__VA_ARGS__);
#else
#define cam_trace(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_TRACE) \
			printk(KERN_DEBUG TAG_NAME "%s: " fmt, \
				__func__, ##__VA_ARGS__); \
	} while (0)
#endif

#if defined(CONFIG_CAM_DEBUG) && defined(CONFIG_CAM_AF_DEBUG)
#define af_dbg(fmt, ...)	cam_dbg(fmt, ##__VA_ARGS__);
#else
#define af_dbg(fmt, ...)
#endif
#if defined(CONFIG_CAM_DEBUG) && defined(CONFIG_CAM_BOOT_DEBUG)
#define boot_dbg(fmt, ...)	cam_dbg(fmt, ##__VA_ARGS__);
#else
#define boot_dbg(fmt, ...)
#endif

#if 0
#define cam_bug_on(arg...)	\
	do { cam_err(arg); BUG_ON(1); } while (0)
#else
#define cam_bug_on(arg...)
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

/* result values returned to HAL */
enum af_result_status {
	AF_RESULT_NONE = 0x00,
	AF_RESULT_FAILED = 0x01,
	AF_RESULT_SUCCESS = 0x02,
	AF_RESULT_CANCELLED = 0x04,
	AF_RESULT_DOING = 0x08
};

enum af_operation_status {
	AF_NONE = 0,
	AF_START,
	AF_CANCEL,
};

enum preflash_status {
	PREFLASH_NONE = 0,
	PREFLASH_OFF,
	PREFLASH_ON,
};

enum sr130pc20_oprmode {
	SR130PC20_OPRMODE_VIDEO = 0,
	SR130PC20_OPRMODE_IMAGE = 1,
};

enum stream_cmd {
	STREAM_STOP,
	STREAM_START,
};

enum wide_req_cmd {
	WIDE_REQ_NONE,
	WIDE_REQ_CHANGE,
	WIDE_REQ_RESTORE,
};

/* Preview Size List: refer to the belows. */
enum sr130pc20_preview_frame_size {
	PREVIEW_SZ_QCIF = 0,	/* 176x144 */
	PREVIEW_SZ_320x240,	/* 320x240 */
	PREVIEW_SZ_CIF,		/* 352x288 */
	PREVIEW_SZ_528x432,	/* 528x432 */
	PREVIEW_SZ_VGA,		/* 640x480 */
	PREVIEW_SZ_D1,		/* 720x480 */
	PREVIEW_SZ_880x720,	/* 880x720 */
	PREVIEW_SZ_SVGA,	/* 800x600 */
	PREVIEW_SZ_1024x576,	/* 1024x576, 16:9 */
	PREVIEW_SZ_1024x616,	/* 1024x616, ? */
	PREVIEW_SZ_XGA,		/* 1024x768 */
	PREVIEW_SZ_PVGA,	/* 1280x720 */
	PREVIEW_SZ_SXGA,	/* 1280x1024 */
	PREVIEW_SZ_MAX,
};

/* Capture Size List: Capture size is defined as below.
 *
 *	CAPTURE_SZ_VGA:		640x480
 *	CAPTURE_SZ_WVGA:	800x480
 *	CAPTURE_SZ_SVGA:	800x600
 *	CAPTURE_SZ_WSVGA:	1024x600
 *	CAPTURE_SZ_1MP:		1280x960
 *	CAPTURE_SZ_W1MP:	1600x960
 *	CAPTURE_SZ_2MP:		UXGA - 1600x1200
 *	CAPTURE_SZ_W2MP:	35mm Academy Offset Standard 1.66
 *				2048x1232, 2.4MP
 *	CAPTURE_SZ_3MP:		QXGA  - 2048x1536
 *	CAPTURE_SZ_W4MP:	WQXGA - 2560x1536
 *	CAPTURE_SZ_5MP:		2560x1920
 */

enum sr130pc20_capture_frame_size {
	/*CAPTURE_SZ_VGA = 0,*/	/* 640x480 */
	/*CAPTURE_SZ_960_720,*/
	CAPTURE_SZ_1MP,/* 1280x960 */
	CAPTURE_SZ_MAX,
};

#define PREVIEW_WIDE_SIZE	PREVIEW_SZ_1024x576
#define CAPTURE_WIDE_SIZE	CAPTURE_SZ_W2MP

enum frame_ratio {
	FRMRATIO_QCIF   = 12,   /* 11 : 9 */
	FRMRATIO_VGA    = 13,   /* 4 : 3 */
	FRMRATIO_D1     = 15,   /* 3 : 2 */
	FRMRATIO_WVGA   = 16,   /* 5 : 3 */
	FRMRATIO_HD     = 17,   /* 16 : 9 */
};

enum sr130pc20_fps_index {
	I_FPS_0,
	I_FPS_7,
	I_FPS_10,
	I_FPS_12,
	I_FPS_15,
	I_FPS_25,
	I_FPS_30,
	I_FPS_MAX,
};

enum ae_awb_lock {
	AEAWB_UNLOCK = 0,
	AEAWB_LOCK,
	AEAWB_LOCK_MAX,
};

enum runmode {
	RUNMODE_NOTREADY,
	RUNMODE_INIT,
	/*RUNMODE_IDLE,*/
	RUNMODE_RUNNING,	/* previewing */
	RUNMODE_RUNNING_STOP,
	RUNMODE_CAPTURING,
	RUNMODE_CAPTURING_STOP,
	RUNMODE_RECORDING,	/* camcorder mode */
	RUNMODE_RECORDING_STOP,
};

enum sr130pc20_stby_type {
	SR130PC20_STBY_HW,
	SR130PC20_STBY_SW,
};

enum sr130_pc20_preview_mode {
	PREVIEW_CAMERA = 1,
	PREVIEW_VIDEOCALL,
	PREVIEW_SMARTSTAY,
};

struct sr130pc20_control {
	u32 id;
	s32 value;
	s32 default_value;
};

#define SR130PC20_INIT_CONTROL(ctrl_id, default_val) \
	{					\
		.id = ctrl_id,			\
		.value = default_val,		\
		.default_value = default_val,	\
	}

struct sr130pc20_framesize {
	s32 index;
	u32 width;
	u32 height;
};

#define FRM_RATIO(framesize) \
	(((framesize)->width) * 10 / ((framesize)->height))

struct sr130pc20_fps {
	u32 index;
	u32 fps;
};

struct sr130pc20_version {
	u32 major;
	u32 minor;
};

struct sr130pc20_date_info {
	u32 year;
	u32 month;
	u32 date;
};

struct sr130pc20_firmware {
	u32 addr;
	u32 size;
};

struct sr130pc20_jpeg_param {
	u32 enable;
	u32 quality;
	u32 main_size;		/* Main JPEG file size */
	u32 thumb_size;		/* Thumbnail file size */
	u32 main_offset;
	u32 thumb_offset;
	/* u32 postview_offset; */
};

struct sr130pc20_position {
	s32 x;
	s32 y;
};

struct sr130pc20_rect {
	s32 x;
	s32 y;
	u32 width;
	u32 height;
};

struct gps_info_common {
	u32 direction;
	u32 dgree;
	u32 minute;
	u32 second;
};

struct sr130pc20_gps_info {
	u8 gps_buf[8];
	u8 altitude_buf[4];
	s32 gps_timeStamp;
};

struct sr130pc20_preview {
	const struct sr130pc20_framesize *frmsize;
	u32 update_frmsize:1;
	u32 fast_ae:1;
};

struct sr130pc20_capture {
	const struct sr130pc20_framesize *frmsize;
	u32 pre_req;	/* for fast capture */
	u32 ae_manual_mode:1;
	u32 lowlux_night:1;
	u32 ready:1;	/* for fast capture */
};

/* Focus struct */
struct sr130pc20_focus {
	enum v4l2_focusmode mode;
	enum af_result_status status;

	u32 pos_x;
	u32 pos_y;

	u32 start:1;	/* enum v4l2_auto_focus*/
	u32 touch:1;
	u32 lock:1;	/* set if single AF is done */
};

/* struct for sensor specific data */
struct sr130pc20_ae_gain_offset {
	u32	ae_auto;
	u32	ae_now;
	u32	ersc_auto;
	u32	ersc_now;

	u32	ae_ofsetval;
	u32	ae_maxdiff;
};

/* Flash struct */
struct sr130pc20_flash {
	struct sr130pc20_ae_gain_offset ae_offset;
	enum v4l2_flash_mode mode;
	enum preflash_status preflash;
	u32 awb_delay;
	u32 ae_scl;	/* for back-up */
	u32 on:1;	/* flash on/off */
	u32 ignore_flash:1;
	u32 ae_flash_lock:1;
};

/* Exposure struct */
struct sr130pc20_exposure {
	s32 val;	/* exposure value */
	u32 ae_lock:1;
};

/* White Balance struct */
struct sr130pc20_whitebalance {
	enum v4l2_wb_mode mode; /* wb mode */
	u32 awb_lock:1;
};

struct sr130pc20_exif {
	u16 exp_time_den;
	u16 iso;
	u16 flash;

	/*int bv;*/		/* brightness */
	/*int ebv;*/		/* exposure bias */
};

/* EXIF - flash filed */
#define EXIF_FLASH_FIRED		(0x01)
#define EXIF_FLASH_MODE_FIRING		(0x01 << 3)
#define EXIF_FLASH_MODE_SUPPRESSION	(0x02 << 3)
#define EXIF_FLASH_MODE_AUTO		(0x03 << 3)

struct sr130pc20_stream_time {
	struct timeval curr_time;
	struct timeval before_time;
};

#define GET_ELAPSED_TIME(cur, before) \
		(((cur).tv_sec - (before).tv_sec) * USEC_PER_SEC \
		+ ((cur).tv_usec - (before).tv_usec))

typedef u16 sr130pc20_regset_t;

typedef struct isx012_regset {
	u16 subaddr;
	u32 value;
	u32 len;
} isx012_regset_t;

#ifdef CONFIG_LOAD_FILE
#define DEBUG_WRITE_REGS
struct regset_table {
	const char	*const name;
};

#define SR130PC20_REGSET(x, y, z)		\
	[(x)] = {			\
		.name		= #y,	\
	}

#define SR130PC20_REGSET_TABLE(y, z)	\
	{				\
		.name		= #y,	\
	}

#else /* !CONFIG_LOAD_FILE */

struct regset_table {
	const sr130pc20_regset_t * const reg;
	const u32	array_size;
#ifdef DEBUG_WRITE_REGS
	const char	* const name;
#endif
	const u32	burst;	/* on/off */
};

#ifdef DEBUG_WRITE_REGS
#define SR130PC20_REGSET(x, y, z)		\
	[(x)] = {					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
		.burst		= z,			\
	}
#define SR130PC20_REGSET_TABLE(y, z)		\
	{					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
		.burst		= z,			\
	}
#else /* !DEBUG_WRITE_REGS */
#define SR130PC20_REGSET(x, y, z)		\
	[(x)] = {					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.burst		= z,			\
	}
#define SR130PC20_REGSET_TABLE(y, z)		\
	{					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.burst		= z,			\
	}
#endif /* DEBUG_WRITE_REGS */

#endif /* CONFIG_LOAD_FILE */

#define EV_MIN_VLAUE		EV_MINUS_4
#define GET_EV_INDEX(EV)	((EV) - (EV_MIN_VLAUE))

struct sr130pc20_regs {
	struct regset_table ev[GET_EV_INDEX(EV_MAX_V4L2)];
	struct regset_table metering[METERING_MAX];
	struct regset_table iso[ISO_MAX];
	struct regset_table effect[IMAGE_EFFECT_MAX];
	struct regset_table white_balance[WHITE_BALANCE_MAX];
	struct regset_table preview_size[PREVIEW_SZ_MAX];
	struct regset_table capture_size[CAPTURE_SZ_MAX];
	struct regset_table scene_mode[SCENE_MODE_MAX];
	struct regset_table fps[I_FPS_MAX];
	/* camera mode */
	struct regset_table preview_mode;
	struct regset_table capture_mode;
	struct regset_table capture_mode_night;
	struct regset_table stream_stop;
	struct regset_table init_reg;
	struct regset_table VT_init_reg;
	struct regset_table SS_init_reg;
};

struct sr130pc20_state {
	struct sr130pc20_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format req_fmt;
	struct sr130pc20_preview preview;
	struct sr130pc20_capture capture;
	struct sr130pc20_focus focus;
	struct sr130pc20_flash flash;
	struct sr130pc20_exposure exposure;
	struct sr130pc20_whitebalance wb;
	struct sr130pc20_exif exif;
#if !defined(CONFIG_CAM_YUV_CAPTURE)
	struct sr130pc20_jpeg_param jpeg;
#endif
	struct sr130pc20_stream_time stream_time;
	const struct sr130pc20_regs *regs;
	struct mutex ctrl_lock;
	struct mutex af_lock;
	struct workqueue_struct *workqueue;
	struct work_struct af_work;
	struct work_struct af_win_work;
#ifdef CONFIG_DEBUG_NO_FRAME
	struct work_struct frame_work;
#endif
	enum runmode runmode;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_pix_format_mode format_mode;
	enum v4l2_scene_mode scene_mode;
	enum v4l2_iso_mode iso;

	s32 vt_mode;
	s32 req_fps;
	s32 fps;
        s32 write_fps;
	s32 freq;		/* MCLK in Hz */
	u32 one_frame_delay_ms;
	u32 light_level;	/* light level */
	u32 lux_level_flash;
	u32 shutter_level_flash;
	u8 *dbg_level;
#ifdef CONFIG_DEBUG_NO_FRAME
	bool frame_check;
#endif
	u32 recording:1;
	u32 hd_videomode:1;
	u32 need_wait_streamoff:1;
	u32 initialized:1;
};

static inline struct  sr130pc20_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sr130pc20_state, sd);
}

static inline int sr130pc20_restore_sensor_flash(struct v4l2_subdev *sd);
static int sr130pc20_set_capture(struct v4l2_subdev *sd);
static int sr130pc20_prepare_fast_capture(struct v4l2_subdev *sd);
static int sr130pc20_control_stream(struct v4l2_subdev *sd, u32 cmd);

extern struct class *camera_class;
extern int sr130pc20_create_file(struct class *cls);

#if !defined(CONFIG_CAM_YUV_CAPTURE)
/* JPEG MEMORY SIZE */
#define SENSOR_JPEG_OUTPUT_MAXSIZE	0x29999A /*2726298bytes, 2.6M */
#define EXTRA_MEMSIZE			(0 * SZ_1K)
#define SENSOR_JPEG_SNAPSHOT_MEMSIZE \
	(((SENSOR_JPEG_OUTPUT_MAXSIZE + EXTRA_MEMSIZE  + SZ_16K-1) / SZ_16K) * SZ_16K)
#endif

/*********** Sensor specific ************/
#define DELAY_SEQ               0xFF
#define SR130PC20_CHIP_ID	0xC1

#define SR130PC20_INTSRC_VINT		(0x01 << 5)

#define POLL_TIME_MS		10
#define CAPTURE_POLL_TIME_MS    1000

/* maximum time for one frame in norma light */
#define ONE_FRAME_DELAY_MS_NORMAL		66
/* maximum time for one frame in low light: minimum 10fps. */
#define ONE_FRAME_DELAY_MS_LOW			100
/* maximum time for one frame in night mode: 6fps */
#define ONE_FRAME_DELAY_MS_NIGHTMODE		166

/* level at or below which we need to enable flash when in auto mode */
#define LUX_LEVEL_MAX			0x00 /* the brightest */
#define LUX_LEVEL_LOW			0x3D /* low light */
#define LUX_LEVEL_FLASH_ON		0x2B

/* Count for loop */
#define SR130PC20_CNT_CAPTURE_FRM		330
#define SR130PC20_CNT_CLEAR_VINT		20
#define SR130PC20_CNT_AE_STABLE		100 /* for checking MODESEL_FIX */
#define SR130PC20_CNT_CAPTURE_AWB		3 /* 8 -> 3 */
#define SR130PC20_CNT_OM_CHECK		30
#define SR130PC20_CNT_CM_CHECK		280 /* 160 -> 180 */
#define SR130PC20_CNT_STREAMOFF		300

#define AF_SEARCH_COUNT			550 /* about 6s */
#define AE_STABLE_SEARCH_COUNT		7

/* Sensor AF first,second window size.
 * we use constant values intead of reading sensor register */
#define DEFAULT_WINDOW_WIDTH		80
#define DEFAULT_WINDOW_HEIGHT		80
#define AF_PRECISION	100

/* diff value fior fast AE in preview */
#define AESCL_DIFF_FASTAE		1000


/*
 * Register Address Definition
 */

/* The Path of Setfile */
#ifdef CONFIG_LOAD_FILE
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#ifdef CONFIG_MACH_KONA
#define TUNING_FILE_PATH "/mnt/sdcard/sr130pc20_regs_kona.h"
#else
#define TUNING_FILE_PATH "/mnt/sdcard/sr130pc20_regs.h"
#endif /* CONFIG_MACH_KONA */
#endif /* CONFIG_LOAD_FILE*/

#ifdef CONFIG_MACH_KONA
#include "sr130pc20_regs_kona.h"
#else
#include "sr130pc20_regs.h"
#endif

#endif /* __SR130PC20_H__ */
