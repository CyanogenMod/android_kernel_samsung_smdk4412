/*
 * Driver for M9MO (16MP Camera) from NEC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __M9MO_H
#define __M9MO_H

#include <linux/wakelock.h>

#define CONFIG_CAM_DEBUG

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
#define CAM_DEBUG   (1 << 0)
#define CAM_TRACE   (1 << 1)
#define CAM_I2C     (1 << 2)

#define cam_dbg(fmt, ...)	\
	do { \
		if (to_state(sd)->dbg_level & CAM_DEBUG) \
			printk(KERN_DEBUG "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_trace(fmt, ...)	\
	do { \
		if (to_state(sd)->dbg_level & CAM_TRACE) \
			printk(KERN_DEBUG "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_i2c_dbg(fmt, ...)	\
	do { \
		if (to_state(sd)->dbg_level & CAM_I2C) \
			printk(KERN_DEBUG "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)
#else
#define cam_dbg(fmt, ...)
#define cam_trace(fmt, ...)
#define cam_i2c_dbg(fmt, ...)
#endif
#define FRM_RATIO(x)    ((x)->width*10/(x)->height)

u8 buf_port_seting0[] = {
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
		  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF,
		 };
u8 buf_port_seting1[] = {
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
		  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF,
		 };
u8 buf_port_seting2[] = {
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x10,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 };

enum m9mo_prev_frmsize {
	M9MO_PREVIEW_QCIF,
	M9MO_PREVIEW_QCIF2,
	M9MO_PREVIEW_QVGA,
	M9MO_PREVIEW_VGA,
	M9MO_PREVIEW_D1,
	M9MO_PREVIEW_WVGA,
	M9MO_PREVIEW_720P,
#if defined(CONFIG_MACH_Q1_BD)
	M9MO_PREVIEW_880_720,
	M9MO_PREVIEW_1200_800,
	M9MO_PREVIEW_1280_800,
	M9MO_PREVIEW_1280_768,
	M9MO_PREVIEW_1072_800,
	M9MO_PREVIEW_980_800,
#endif
	M9MO_PREVIEW_1080P,
	M9MO_PREVIEW_HDR,
	M9MO_PREVIEW_720P_60FPS,
	M9MO_PREVIEW_VGA_60FPS,
};

enum m9mo_cap_frmsize {
	M9MO_CAPTURE_1MP,	/* 1024 x 768 */
	M9MO_CAPTURE_2MPW,	/* 1920 x 1080 */
	M9MO_CAPTURE_3MP,	/* 1984 x 1488 */
	M9MO_CAPTURE_5MP,	/* 2592 x 1944 */
	M9MO_CAPTURE_8MP,	/* 3264 x 2448 */
	M9MO_CAPTURE_10MP,	/* 3648 x 2736 */
	M9MO_CAPTURE_12MPW,	/* 4608 x 2768 */
	M9MO_CAPTURE_14MP,	/* 4608 x 3072 */
	M9MO_CAPTURE_16MP,	/* 4608 x 3456 */
	M9MO_CAPTURE_POSTWVGA,	/* 800 x 480 */
	M9MO_CAPTURE_POSTVGA,	/* 640 x 480 */
	M9MO_CAPTURE_POSTWHD,	/* 1280 x 720 */
	M9MO_CAPTURE_POSTHD,	/* 960 x 720 */
};

enum cam_frmratio {
	CAM_FRMRATIO_QCIF   = 12,   /* 11 : 9 */
	CAM_FRMRATIO_VGA    = 13,   /* 4 : 3 */
	CAM_FRMRATIO_D1     = 15,   /* 3 : 2 */
	CAM_FRMRATIO_WVGA   = 16,   /* 5 : 3 */
	CAM_FRMRATIO_HD     = 17,   /* 16 : 9 */
};

struct m9mo_control {
	u32 id;
	s32 value;
	s32 minimum;		/* Note signedness */
	s32 maximum;
	s32 step;
	s32 default_value;
};

struct m9mo_frmsizeenum {
	unsigned int index;
	unsigned int width;
	unsigned int height;
	u8 reg_val;		/* a value for category parameter */
};

struct m9mo_isp {
	wait_queue_head_t wait;
	unsigned int irq;	/* irq issued by ISP */
	unsigned int issued;
	unsigned int int_factor;
	unsigned int bad_fw:1;
};

struct m9mo_jpeg {
	int quality;
	unsigned int main_size;	/* Main JPEG file size */
	unsigned int thumb_size;	/* Thumbnail file size */
	unsigned int main_offset;
	unsigned int thumb_offset;
	unsigned int postview_offset;
};

struct m9mo_focus {
	unsigned int start:1;
	unsigned int lock:1;
	unsigned int touch:1;

	unsigned int mode;
#if defined(CONFIG_TARGET_LOCALE_NA)
	unsigned int ui_mode;
	unsigned int mode_select;
#endif
	unsigned int status;

	unsigned int pos_x;
	unsigned int pos_y;
};

struct m9mo_exif {
	char unique_id[7];
	u32 exptime;		/* us */
	u16 flash;
	u16 iso;
	int tv;			/* shutter speed */
	int bv;			/* brightness */
	int ebv;		/* exposure bias */
};

struct m9mo_state {
	struct m9mo_platform_data *pdata;
	struct device *m9mo_dev;
	struct v4l2_subdev sd;

	struct wake_lock wake_lock;

	struct m9mo_isp isp;

	const struct m9mo_frmsizeenum *preview;
	const struct m9mo_frmsizeenum *capture;

	enum v4l2_pix_format_mode format_mode;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_flash_mode flash_mode;
	enum v4l2_scene_mode scene_mode;
	int vt_mode;
	int zoom;
	int optical_zoom;

	int m9mo_fw_done;
	int fw_info_done;

	unsigned int fps;
	struct m9mo_focus focus;

	struct m9mo_jpeg jpeg;
	struct m9mo_exif exif;

	char *fw_version;

#ifdef CONFIG_CAM_DEBUG
	u8 dbg_level;
#endif

	int facedetect_mode;
	int running_capture_mode;
	int fd_eyeblink_cap;

	unsigned int face_beauty:1;
	unsigned int recording:1;
	unsigned int check_dataline:1;
	int anti_banding;
	int pixelformat;
};

/* Category */
#define M9MO_CATEGORY_SYS	0x00
#define M9MO_CATEGORY_PARM	0x01
#define M9MO_CATEGORY_MON	0x02
#define M9MO_CATEGORY_AE	0x03
#define M9MO_CATEGORY_NEW	0x04
#define M9MO_CATEGORY_WB	0x06
#define M9MO_CATEGORY_EXIF	0x07
#define M9MO_CATEGORY_FD	0x09
#define M9MO_CATEGORY_LENS	0x0A
#define M9MO_CATEGORY_CAPPARM	0x0B
#define M9MO_CATEGORY_CAPCTRL	0x0C
#define M9MO_CATEGORY_TEST	0x0D
#define M9MO_CATEGORY_ADJST	0x0E
#define M9MO_CATEGORY_FLASH	0x0F    /* F/W update */

/* M9MO_CATEGORY_SYS: 0x00 */
#define M9MO_SYS_PJT_CODE	0x01
#define M9MO_SYS_VER_FW		0x02
#define M9MO_SYS_VER_HW		0x04
#define M9MO_SYS_VER_PARAM	0x06
#define M9MO_SYS_VER_AWB	0x08
#define M9MO_SYS_USER_VER	0x0A
#define M9MO_SYS_MODE		0x0B
#define M9MO_SYS_ESD_INT	0x0E

#define M9MO_SYS_INT_EN		0x10
#define M9MO_SYS_INT_FACTOR	0x1C
#define M9MO_SYS_FRAMESYNC_CNT	0x14


/* M9MO_CATEGORY_PARAM: 0x01 */
#define M9MO_PARM_OUT_SEL	0x00
#define M9MO_PARM_MON_SIZE	0x01
#define M9MO_PARM_EFFECT	0x0B
#define M9MO_PARM_FLEX_FPS	0x67
#define M9MO_PARM_HDMOVIE	0x32

/* M9MO_CATEGORY_MON: 0x02 */
#define M9MO_MON_ZOOM		0x01
#define M9MO_MON_MON_REVERSE	0x05
#define M9MO_MON_MON_MIRROR	0x06
#define M9MO_MON_SHOT_REVERSE	0x07
#define M9MO_MON_SHOT_MIRROR	0x08
#define M9MO_MON_CFIXB		0x09
#define M9MO_MON_CFIXR		0x0A
#define M9MO_MON_COLOR_EFFECT	0x0B
#define M9MO_MON_CHROMA_LVL	0x0F
#define M9MO_MON_EDGE_LVL	0x11
#define M9MO_MON_TONE_CTRL	0x25

/* M9MO_CATEGORY_AE: 0x03 */
#define M9MO_AE_LOCK		0x00
#define M9MO_AE_MODE		0x01
#define M9MO_AE_ISOSEL		0x05
#define M9MO_AE_FLICKER		0x06
#define M9MO_AE_INDEX		0x09
#define M9MO_AE_EP_MODE_MON	0x0A
#define M9MO_AE_EP_MODE_CAP	0x0B
#define M9MO_AE_AUTO_BRACKET_EV	0x20
#define M9MO_AE_ONESHOT_MAX_EXP	0x36

/* M9MO_CATEGORY_NEW: 0x04 */
#define M9MO_NEW_DETECT_SCENE	0x0B

/* M9MO_CATEGORY_WB: 0x06 */
#define M9MO_AWB_LOCK		0x00
#define M9MO_WB_AWB_MODE	0x02
#define M9MO_WB_AWB_MANUAL	0x03

/* M9MO_CATEGORY_EXIF: 0x07 */
#define M9MO_EXIF_EXPTIME_NUM	0x00
#define M9MO_EXIF_EXPTIME_DEN	0x04
#define M9MO_EXIF_TV_NUM	0x08
#define M9MO_EXIF_TV_DEN	0x0C
#define M9MO_EXIF_BV_NUM	0x18
#define M9MO_EXIF_BV_DEN	0x1C
#define M9MO_EXIF_EBV_NUM	0x20
#define M9MO_EXIF_EBV_DEN	0x24
#define M9MO_EXIF_ISO		0x28
#define M9MO_EXIF_FLASH		0x2A

/* M9MO_CATEGORY_FD: 0x09 */
#define M9MO_FD_CTL					0x00
#define M9MO_FD_SIZE				0x01
#define M9MO_FD_MAX					0x02
#define M9MO_FD_RED_EYE				0x55
#define M9MO_FD_BLINK_FRAMENO		0x59
#define M9MO_FD_BLINK_LEVEL_1		0x5A
#define M9MO_FD_BLINK_LEVEL_2		0x5B
#define M9MO_FD_BLINK_LEVEL_3		0x5C

/* M9MO_CATEGORY_LENS: 0x0A */
#define M9MO_LENS_AF_INITIAL		0x00
#define M9MO_LENS_AF_MODE			0x01
#define M9MO_LENS_AF_ZOOM_CTRL		0x02
#define M9MO_LENS_AF_START_STOP		0x03
#define M9MO_LENS_AF_STATUS			0x03
#define M9MO_LENS_AF_IRIS_STEP		0x05
#define M9MO_LENS_AF_ZOOM_LEVEL		0x06
#define M9MO_LENS_AF_BACKLASH_ADJ	0x0A
#define M9MO_LENS_AF_FOCUS_ADJ		0x0B
#define M9MO_LENS_AF_TILT_ADJ		0x0C
#define M9MO_LENS_AF_AF_ADJ			0x0D
#define M9MO_LENS_AF_PUNT_ADJ		0x0E
#define M9MO_LENS_AF_ZOOM_ADJ		0x0F
#define M9MO_LENS_AF_ADJ_TEMP_VALUE	0x0C
#define M9MO_LENS_AF_ALGORITHM		0x0D
#define M9MO_LENS_AF_CAL			0x1D
#define M9MO_LENS_AF_TOUCH_POSX		0x30
#define M9MO_LENS_AF_TOUCH_POSY		0x32

/* M9MO_CATEGORY_CAPPARM: 0x0B */
#define M9MO_CAPPARM_YUVOUT_MAIN	0x00
#define M9MO_CAPPARM_MAIN_IMG_SIZE	0x01
#define M9MO_CAPPARM_YUVOUT_PREVIEW	0x05
#define M9MO_CAPPARM_PREVIEW_IMG_SIZE	0x06
#define M9MO_CAPPARM_YUVOUT_THUMB	0x0A
#define M9MO_CAPPARM_THUMB_IMG_SIZE	0x0B
#define M9MO_CAPPARM_JPEG_SIZE_MAX	0x0F
#define M9MO_CAPPARM_JPEG_SIZE_MIN	0x13
#define M9MO_CAPPARM_JPEG_RATIO		0x17
#define M9MO_CAPPARM_MCC_MODE		0x1D
#define M9MO_CAPPARM_STROBE_EN		0x22
#define M9MO_CAPPARM_WDR_EN			0x2C
#define M9MO_CAPPARM_JPEG_RATIO_OFS	0x34
#define M9MO_CAPPARM_THUMB_JPEG_MAX	0x3C
#define M9MO_CAPPARM_AFB_CAP_EN		0x53

/* M9MO_CATEGORY_CAPCTRL: 0x0C */
#define M9MO_CAPCTRL_CAP_MODE	0x00
#define M9MO_CAPCTRL_CAP_FRM_INTERVAL 0x01
#define M9MO_CAPCTRL_CAP_FRM_COUNT 0x02
#define M9MO_CAPCTRL_START_DUALCAP 0x05
#define M9MO_CAPCTRL_FRM_SEL	0x06
#define M9MO_CAPCTRL_FRM_PRV_SEL	0x07
#define M9MO_CAPCTRL_TRANSFER	0x09
#define M9MO_CAPCTRL_IMG_SIZE	0x0D
#define M9MO_CAPCTRL_THUMB_SIZE	0x11

/* M9MO_CATEGORY_CAPCTRL: 0x0C  M9MO_CAPCTRL_CAP_MODE: 0x00 */
#define M9MO_CAP_MODE_SINGLE_CAPTURE		(0x00)
#define M9MO_CAP_MODE_MULTI_CAPTURE			(0x01)
#define M9MO_CAP_MODE_DUAL_CAPTURE			(0x05)
#define M9MO_CAP_MODE_BRACKET_CAPTURE		(0x06)
#define M9MO_CAP_MODE_ADDPIXEL_CAPTURE		(0x08)
#define M9MO_CAP_MODE_PANORAMA_CAPTURE		(0x0B)
#define M9MO_CAP_MODE_BLINK_CAPTURE			(0x0C)

/* M9MO_CATEGORY_ADJST: 0x0E */
#define M9MO_ADJST_SHUTTER_MODE	0x33
#define M9MO_ADJST_AWB_RG_H	0x3C
#define M9MO_ADJST_AWB_RG_L	0x3D
#define M9MO_ADJST_AWB_BG_H	0x3E
#define M9MO_ADJST_AWB_BG_L	0x3F

/* M9MO_CATEGORY_FLASH: 0x0F */
#define M9MO_FLASH_ADDR		0x00
#define M9MO_FLASH_BYTE		0x04
#define M9MO_FLASH_ERASE	0x06
#define M9MO_FLASH_WR		0x07
#define M9MO_FLASH_RAM_CLEAR	0x08
#define M9MO_FLASH_CAM_START	0x12
#define M9MO_FLASH_SEL		0x13

/* M9MO_CATEGORY_TEST:	0x0D */
#define M9MO_TEST_OUTPUT_YCO_TEST_DATA	0x1B
#define M9MO_TEST_ISP_PROCESS		0x59

/* M9MO Sensor Mode */
#define M9MO_SYSINIT_MODE	0x0
#define M9MO_PARMSET_MODE	0x1
#define M9MO_MONITOR_MODE	0x2
#define M9MO_STILLCAP_MODE	0x3

/* Interrupt Factor */
#define M9MO_INT_SOUND		(1 << 15)
#define M9MO_INT_LENS_INIT	(1 << 14)
#define M9MO_INT_FD		(1 << 13)
#define M9MO_INT_FRAME_SYNC	(1 << 12)
#define M9MO_INT_CAPTURE	(1 << 11)
#define M9MO_INT_ZOOM		(1 << 10)
#define M9MO_INT_AF		(1 << 9)
#define M9MO_INT_MODE		(1 << 8)
#define M9MO_INT_ATSCENE	(1 << 7)
#define M9MO_INT_ATSCENE_UPDATE	(1 << 6)
#define M9MO_INT_OIS_INIT	(1 << 3)
#define M9MO_INT_STNW_DETECT	(1 << 2)
#define M9MO_INT_SCENARIO_FIN	(1 << 1)
#define M9MO_INT_PRINT	(1 << 0)

/* ESD Interrupt */
#define M9MO_INT_ESD		(1 << 0)

#endif /* __M9MO_H */
