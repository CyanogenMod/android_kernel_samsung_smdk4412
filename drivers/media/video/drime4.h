/*
 * Driver for LSI DRIME4 (ISP for 8MP Camera)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __DRIME4_H
#define __DRIME4_H

#include <linux/mutex.h>

extern int g_drime4_boot_error;

#define CONFIG_CAM_DEBUG	1
/*#define FEATURE_DEBUG_DUMP*/

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
#define CAM_DEBUG	(1 << 0)
#define CAM_TRACE	(1 << 1)
#define CAM_I2C		(1 << 2)

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

enum drime4_fw_path {
	DRIME4_SD_CARD,
	DRIME4_IN_DATA,
	DRIME4_IN_SYSTEM,
	DRIME4_PATH_MAX,
};

enum drime4_lens_fw_path {
	DRIME4_LENS_SD_CARD,
	DRIME4_LENS_EXT_SD_CARD,
	DRIME4_LENS_PATH_MAX,
};

enum drime4_prev_frmsize {
	DRIME4_PREVIEW_720P,
	DRIME4_PREVIEW_1056X704,
	DRIME4_PREVIEW_960X720,
	DRIME4_PREVIEW_704X704,
	DRIME4_PREVIEW_VGA,
};

enum drime4_movi_frmsize {
	DRIME4_MOVIE_FHD,	/* 1920 x 1080 : default */
	DRIME4_MOVIE_CINE,	/* 1920 x 810 */
	DRIME4_MOVIE_HD,	/* 1280 x 720 */
	DRIME4_MOVIE_960_720,	/* 960 x 720 */
	DRIME4_MOVIE_704_576,	/* 704 x 576 */
	DRIME4_MOVIE_VGA,	/* 640 x 480 */
	DRIME4_MOVIE_QVGA,	/* 320 x 240 */
};

enum drime4_cap_frmsize {
	DRIME4_CAPTURE_16p9MW,	/* 5472 x 3080 : default */
	DRIME4_CAPTURE_7p8MW,	/* 3712 x 2088 */
	DRIME4_CAPTURE_4p9MW,	/* 2944 x 1656 */
	DRIME4_CAPTURE_2p1MW,	/* 1920 x 1080 */
	DRIME4_CAPTURE_20MP,	/* 5472 x 3648 */
	DRIME4_CAPTURE_10p1MP,	/* 3888 x 2592 */
	DRIME4_CAPTURE_5p9MP,	/* 2976 x 1984 */
	DRIME4_CAPTURE_2MP,		/* 1728 x 1152 */
	DRIME4_CAPTURE_13p3MS,	/* 3648 x 3648 */
	DRIME4_CAPTURE_7MS,		/* 2640 x 2640 */
	DRIME4_CAPTURE_4MS,		/* 2000 x 2000 */
	DRIME4_CAPTURE_1p1MS,	/* 1024 x 1024 */
	DRIME4_CAPTURE_15MP,	/* 2048 x 7680 */
	DRIME4_CAPTURE_5MP,		/* 2736 x 1824 : burst */
	DRIME4_CAPTURE_4p1MW_3D,	/* 2668 x 1512 : 3D */
	DRIME4_CAPTURE_2p1MW_3D,	/* 1920 x 1080 : 3D */
	DRIME4_CAPTURE_VGA,		/* 640 x 480 */
	DRIME4_CT3_MODE_STILL,	/* 5600 x 3714 */
	DRIME4_CT3_MODE_BURST,	/* 2800 x 1864 */
	DRIME4_CT3_MODE_FULLHD,	/* 2800 x 1576 */
	DRIME4_CT3_MODE_CINEMA,	/* 2800 x  1206 */
	DRIME4_CT3_MODE_HD,	/* 1864 x 1064 */
	DRIME4_CT3_MODE_LIVEVIEW,	/* 1864 x 1248 */
	DRIME4_CT3_MODE_MF,		/* 1864 x 1248 */
	DRIME4_CT3_MODE_FASTAF,	/* 1864 x 544 */
	DRIME4_CT3_MODE_PAFLIVEVIEW,	/* 3364 x 1248 */
};

enum drime4_isneed_flash_tristate {
	DRIME4_ISNEED_FLASH_OFF = 0x00,
	DRIME4_ISNEED_FLASH_ON = 0x01,
	DRIME4_ISNEED_FLASH_UNDEFINED = 0x02,
};

enum drime4_capture_mode {
	DRIME4_CAP_MODE_SINGLE	= 0x00,
	DRIME4_CAP_MODE_CONT_9	= 0x01,
	DRIME4_CAP_MODE_CONT_5	= 0x02,
	DRIME4_CAP_MODE_BURST	= 0x03,
	DRIME4_CAP_MODE_AEB		= 0x05,
	DRIME4_CAP_MODE_WBB		= 0x06,
	DRIME4_CAP_MODE_PWB		= 0x07,
	DRIME4_CAP_MODE_SMART	= 0x08,
	DRIME4_CAP_MODE_BULB	= 0x10,
	DRIME4_CAP_MODE_CUST_WB	= 0x13,
	DRIME4_CAP_MODE_HDR		= 0x14,
};

struct drime4_control {
	u32 id;
	s32 value;
	s32 minimum;		/* Note signedness */
	s32 maximum;
	s32 step;
	s32 default_value;
};

struct drime4_frmsizeenum {
	unsigned int index;
	unsigned int width;
	unsigned int height;
	u8 reg_val;		/* a value for category parameter */
};

struct drime4_effectenum {
	unsigned int index;
	unsigned int reg_val;
};

struct drime4_fraction {
	unsigned int num;
	unsigned int den;
};

struct drime4_isp {
	wait_queue_head_t ap_wait;
	wait_queue_head_t d4_wait;
	unsigned int ap_irq;	/* irq issued by ISP */
	unsigned int ap_issued;
	unsigned int ap_int_factor;
	unsigned int bad_fw:1;
	unsigned int d4_irq;
	unsigned int d4_issued;
	unsigned int d4_int_factor;
};

struct drime4_fw_version {
	unsigned int index;
	unsigned int opened;
	char path[25];
	char ver[20];
};

struct drime4_focus {
	unsigned int mode;
	unsigned int lock;
	unsigned int status;
	unsigned int touch;
	unsigned int pos_x;
	unsigned int pos_y;
};

struct drime4_state {
	struct drime4_platform_data *pdata;
	struct v4l2_subdev sd;

	struct drime4_isp isp;

	const struct drime4_frmsizeenum *preview;
	const struct drime4_frmsizeenum *capture;
	const struct drime4_frmsizeenum *photo;
	const struct drime4_frmsizeenum *movie;

	enum v4l2_pix_format_mode format_mode;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_flash_mode flash_mode;
	enum v4l2_wb_mode wb_mode;
	enum v4l2_scene_mode scene_mode;
	int capture_mode;
	int drive_capture_mode;
	int capture_cnt;
	int PASM_mode;
	int vt_mode;
	int hdr_mode;
	int facedetect_mode;
	int hybrid_mode;
	int fast_mode;
	int yuv_snapshot;
	int zoom;
	int stream_enable;
	int ae_lock;
	int awb_lock;
	int fw_index;
	u32 cal_device;
	u32 cal_dll;
	int isp_mode;
	int af_area;
	int focus_peaking_level;
	int focus_peaking_color;
	bool capturingGolfShot;
	unsigned int fps;
	struct drime4_focus focus;
	int caf_mode;
	char isflash;

	u32 real_preview_width;
	u32 real_preview_height;
	u32 preview_width;
	u32 preview_height;
	u32 jpeg_width;
	u32 jpeg_height;
	u32 movie_width;
	u32 movie_height;
	u32 sensor_size;

	u8 sensor_fw[20];
	u8 phone_fw[20];
	u8 hotshoe_status[2];

	int lens_fw_path;

	int pixelformat;

#ifdef CONFIG_CAM_DEBUG
	u8 dbg_level;
#endif

	u8 factory_running;
	int hal_power_off;
	int noti_ctrl_status;

	int af_step;

	int done_3a_lock;
	int smart_read[4];
	int samsung_app;
	char distance_scale_data[100];
	int distance_scale_length;
	int distance_scale_enable;
	struct mutex drime4_lock;
	int factory_next_ready;
	u8 OSD_data[500];
	int OSD_length;
	u8 factory_log_start;
	int first_boot_param;
};

#define DRIME4_PACKET_DSP_QUERY			0x80
#define DRIME4_PACKET_DSP_QUERY_RES		0x81
#define DRIME4_PACKET_DSP_SET			0x82
#define DRIME4_PACKET_DSP_SET_RES		0x83
#define DRIME4_PACKET_DSP_CMD			0x84
#define DRIME4_PACKET_DSP_CMD_RES		0x85
#define DRIME4_PACKET_DSP_NOTI			0x86
#define DRIME4_PACKET_ADJUST_PC_DATA	0xA0
#define DRIME4_PACKET_LOG_WRITE			0xA2
#define DRIME4_PACKET_LOG_WRITE_RES		0xA3
#define DRIME4_PACKET_CMD_DATA_SIZE		0xA4
#define DRIME4_PACKET_CMD_DATA_SIZE_RES	0xA5
#define DRIME4_PACKET_ISP_LOG_CHECK		0xC0
#define DRIME4_PACKET_ISP_LOG_CHECK_RES	0xC1
#define DRIME4_PACKET_ISP_LOG_SAVE		0xC2
#define DRIME4_PACKET_ISP_LOG_SAVE_RES	0xC3
#define DRIME4_PACKET_ERROR_RES			0xF0
#define DRIME4_PACKET_FWUP_START		0xF1
#define DRIME4_PACKET_FWUP_READY		0xF2
#define DRIME4_PACKET_FWUP_DATA			0xF3
#define DRIME4_PACKET_FWUP_DATA_COM		0xF4
#define DRIME4_PACKET_FWUP_COM			0xF5
#define DRIME4_PACKET_LENS_FWUP_START	0xF6
#define DRIME4_PACKET_LENS_FWUP_READY	0xF7

#define DRIME4_PACKET_RES_OK			0x00
#define DRIME4_PACKET_RES_ERR			0x01


/* DRIME4_PACKET_DSP_QUERY : 0x80 */
#define DRIME4_QRY_FW_VERSION			0x0103
#define DRIME4_QRY_CHECK_AVS			0x0105
#define DRIME4_QRY_STROBO_STATUS		0x0106
#define DRIME4_QRY_STROBO_CHARGING		0x0107

#define DRIME4_QRY_LIVE_3A_DATA			0x0364
#define DRIME4_QRY_AF_SCAN_RES			0x0365
#define DRIME4_QRY_3A_LOCK_DATA			0x0366

#define DRIME4_QRY_LENS_INFO			0x0501
#define DRIME4_QRY_LENS_MODEL			0x0503
#define DRIME4_QRY_LENS_NEED_DISTANCE	0x0504


/* DRIME4_PACKET_DSP_SET : 0x82 */
#define DRIME4_SET_AVS_VALUE			0x0101
#define DRIME4_SET_SYSTEM_MODE			0x0102
#define DRIME4_SET_SMART_SUB_MODE		0x0103
#define DRIME4_SET_EXPERT_SUB_MODE		0x0104
#define DRIME4_SET_EXIF_SYSTEM_TIME		0x0106
#define DRIME4_SET_EXIF_SYSTEM_ORIENT	0x0107
#define DRIME4_SET_EXIF_GPS_INFO		0x0108
#define DRIME4_SET_EXIF_MODLE_NAME		0x0109
#define DRIME4_SET_WEATHER_INFO			0x010A

#define DRIME4_SET_CAP_CNT				0x0201
#define DRIME4_SET_SHUTTER_SPEED		0x0202
#define DRIME4_SET_F_NUMBER				0x0203
#define DRIME4_SET_F_NUM_OF_i_DEPTH		0x0204
#define DRIME4_SET_EVC					0x0205
#define DRIME4_SET_MULTI_EXPOSURE_CNT	0x0206
#define DRIME4_SET_PROGRAM_SHIFT		0x0207
#define DRIME4_SET_SMART_PRO_LVL		0x0301
#define DRIME4_SET_i_ZOOM				0x0302
#define DRIME4_SET_SATURATION			0x0303
#define DRIME4_SET_SHARPNESS			0x0304
#define DRIME4_SET_CONTRAST				0x0305
#define DRIME4_SET_COLOR_ADJ			0x0306
#define DRIME4_SET_WB_GBAM				0x0307
#define DRIME4_SET_WB_KELVIN			0x0308
#define DRIME4_SET_AF_WIDTH				0x0309
#define DRIME4_SET_LIVEVIEW_SIZE		0x0310
#define DRIME4_SET_LIVEVIEW_FPS			0x0311

#define DRIME4_SET_CAPTURE_SIZE			0x1001
#define DRIME4_SET_QUALITY				0x1002
#define DRIME4_SET_ISO					0x1003
#define DRIME4_SET_WB					0x1004
#define DRIME4_SET_AF_MODE				0x1007
#define DRIME4_SET_AF_AREA				0x1008
#define DRIME4_SET_TOUCH_AF				0x1009
#define DRIME4_SET_MF_ASSIST			0x100A
#define DRIME4_SET_FOCUS_PEAKING		0x100B
#define DRIME4_SET_LINK_AE_TO_AF_POINT	0x100C
#define DRIME4_SET_FRAMING_MODE			0x100D
#define DRIME4_SET_OIS					0x100E
#define DRIME4_SET_DRIVE				0x100F
#define DRIME4_SET_METERING				0x1010
#define DRIME4_SET_DYNAMIC_RANGE		0x1011
#define DRIME4_SET_FLASH_MODE			0x1012
#define DRIME4_SET_MOVIE_SIZE			0x1013
#define DRIME4_SET_MOVIE_FPS			0x1015
#define DRIME4_SET_MOVIE_FADER			0x1017
#define DRIME4_SET_ISO_STEP				0x1018
#define DRIME4_SET_HIGH_ISO_NR			0x1019
#define DRIME4_SET_LONG_TERM_NR			0x101A
#define DRIME4_SET_AEB_SET				0x101B
#define DRIME4_SET_WBB_SET				0x101C
#define DRIME4_SET_DMF					0x101E
#define DRIME4_SET_COLOR_SPACE			0x101F
#define DRIME4_SET_LDC					0x1020
#define DRIME4_SET_AF_LAMP				0x1023
#define DRIME4_SET_QUICKVIEW			0x1024
#define DRIME4_SET_3D_SHOT				0x1027
#define DRIME4_SET_ISO_AUTO_MAX			0x1029
#define DRIME4_SET_FLASH_STEP			0x102A
#define DRIME4_SET_AF_PRIORITY			0x102B
#define DRIME4_SET_NO_LENS_CAPTRUE		0x102C
#define DRIME4_SET_OVER_EXPOSURE_GUIDE	0x102D
#define DRIME4_SET_AF_FRAME_SIZE		0x102E
#define DRIME4_SET_THUMBNAIL_SIZE		0x102F
#define DRIME4_SET_VIDEO_OUT_FORMAT		0x1031
/* DRIME4_PACKET_DSP_CMD : 0x84 */
#if 1		/* v0.98 */
#define DRIME4_CMD_POWER				0x0102
#define DRIME4_CMD_CIS_CLEAN			0x0103
#define DRIME4_CMD_STROBO_POPUP			0x0104
#define DRIME4_CMD_TIMER_LED			0x0105
#define DRIME4_CMD_NOTI_DISABLE			0x0107
#define DRIME4_CMD_MIC_CONTROL			0x0108
#define DRIME4_CMD_SYSTEM_RESET_CWB		0x0109
#define DRIME4_CMD_CAPTURE				0x0201
#define DRIME4_CMD_CAPTURE_STOP			0x0202
#define DRIME4_CMD_TRANSFER				0x0203
#define DRIME4_CMD_QUICKVIEW_CANCEL		0x0204
#define DRIME4_CMD_LIVEVIEW				0x0301
#define DRIME4_CMD_FACEDETECT			0x0302
#define DRIME4_CMD_START_STOP_TRACKING_AF 0x0304
#define DRIME4_CMD_OPTICAL_PREVIEW		0x0305
#define DRIME4_CMD_3A_LOCK				0x0306
#define DRIME4_CMD_AE_AF_LOCK			0x0307
#define DRIME4_CMD_AF_WIDTH				0x030A
#define DRIME4_CMD_i_FN_ENABLE			0x030B
#define DRIME4_CMD_TRACKING_AF			0x030C
#define DRIME4_CMD_MF_ENLARGE			0x030D
#define DRIME4_CMD_LIVEVIEW_PREPARE		0x030E
#define DRIME4_CMD_MF_POSITION_SET		0x030F
#define DRIME4_CMD_MOVE_TO_CENTER_AF	0x0310
#define DRIME4_CMD_MOVIE				0x0401
#define DRIME4_CMD_MOVIE_FADEOUT		0x0402
#else
#define DRIME4_CMD_POWER				0x0102
#define DRIME4_CMD_CAPTURE				0x0201
#define DRIME4_CMD_CAPTURE_STOP			0x0202
#define DRIME4_CMD_3A_LOCK				0x0203
#define DRIME4_CMD_STROBO_POPUP			0x0206
#define DRIME4_CMD_TRANSFER				0x0209
#define DRIME4_CMD_AF_WIDTH				0x020B
#define DRIME4_CMD_LIVEVIEW				0x0301
#define DRIME4_CMD_FACEDETECT			0x0302
#define DRIME4_CMD_CIS_CLEAN			0x0307
#define DRIME4_CMD_MOVIE				0x0401
#endif

#define DRIME4_CMD_PRODUCTION_MODE		0xAA01
#define DRIME4_CMD_SCRIPT_BYPASS		0xAA02
#define DRIME4_CMD_PC_DATA_BYPASS		0xAA03
#define DRIME4_CMD_FW_VERSION_CHECK		0xAA04
#define DRIME4_CMD_WRITE_NAND_FLASH		0xAA05
#define DRIME4_CMD_MAKE_CSV				0xAA06
#define DRIME4_CMD_SET_FOCUS_POS		0xAA07
#define DRIME4_CMD_SET_AF_LED			0xAA08
#define DRIME4_CMD_PRODUCTION_YUV_CAPTURE 0xAA0A
#define DRIME4_CMD_DEBUG_SSIF_RAW_CAPTURE 0xAA0B
#define DRIME4_CMD_DEBUG_PP_RAW_CAPTURE 0xAA0C
#define DRIME4_CMD_GET_ISP_LOG			0xAA0D
#define DRIME4_CMD_COLD_REBOOT			0xAA0E

/* DRIME4_PACKET_DSP_NOTI : 0x86 */
#define DRIME4_NOTI_EXT_MIC_ATTACH		0x0107
#define DRIME4_NOTI_EXT_MIC_DETACH		0x0108
#define DRIME4_NOTI_STROBO_OPEN			0x0109
#define DRIME4_NOTI_STROBO_CLOSE		0x010A
#define DRIME4_NOTI_STROBO_CHARGE_DONE	0x010B
#define DRIME4_NOTI_EXT_STROBO_ATTACH	0x010C
#define DRIME4_NOTI_EXT_STROBO_DETACH	0x010D
#define DRIME4_NOTI_CIS_CLEAN_DONE		0x010E
#define DRIME4_NOTI_ISP_DEBUG_STRING	0x01AA
#define DRIME4_NOTI_CAPTURE_START		0x0201
#define DRIME4_NOTI_PROCESSING			0x0204
#define DRIME4_NOTI_QUICKVIEW_DONE		0x0205
#define DRIME4_NOTI_NEXT_SHOT_READY		0x0206
#define DRIME4_NOTI_LIVEVIEW_RESTART	0x0207
#define DRIME4_NOTI_IMG_READY			0x0208
#define DRIME4_NOTI_LIVEVIEW_START_DONE	0x0301
#define DRIME4_NOTI_LIVEVIEW_STOT_DONE	0x0302
#define DRIME4_NOTI_AF_SCAN_DONE		0x0304
#define DRIME4_NOTI_3A_LOCK_DONE		0x0305
#define DRIME4_NOTI_3A_UNLOCK_DONE		0x0306
#define DRIME4_NOTI_CAF_RESTART_DONE	0x0307
#define DRIME4_NOTI_TRACKING_OBJECT_AF_LOCK		0x030E
#define DRIME4_NOTI_TRACKING_OBJECT_AF_UNLOCK	0x030F
#define DRIME4_NOTI_TRACKING_OBJECT_AF_FAIL		0x0310
#define DRIME4_NOTI_MOVIE_FADEOUT_DONE	0x0401
#define DRIME4_NOTI_3D_MOVIE_LOW_LIGHT 0x0402
#define DRIME4_NOTI_LENS_DETECT			0x0501
#define DRIME4_NOTI_LENS_READY			0x0502
#define DRIME4_NOTI_I_FUNCTION_KEY		0x0503
#define DRIME4_NOTI_LENS_FATAL_ERR		0x0504
#define DRIME4_NOTI_LENS_COMMU_ERR		0x0505
#define DRIME4_NOTI_LENS_LOCK			0x0506
#define DRIME4_NOTI_FOCUS_RANGE			0x0507
#define DRIME4_NOTI_MF_SWITCH			0x0509
#define DRIME4_NOTI_OIS_SWITCH			0x050A
#define DRIME4_NOTI_3D_SWITCH			0x050C
#define DRIME4_NOTI_MF_OPERATION		0x050E
#define DRIME4_NOTI_FOCUS_POS			0x0510
#define DRIME4_NOTI_IRIS_POS			0x0511
#define DRIME4_NOTI_ZOOM_POS			0x0512
#define DRIME4_NOTI_F_NUM_RANGE			0x0513
#define DRIME4_NOTI_LENS_K_MOUNT		0x0515
#define DRIME4_NOTI_LENS_NO_LENS		0x0516
#define DRIME4_NOTI_LENS_MODE_CHANGE_DONE 0x0517
#define DRIME4_NOTI_LENS_FW_UPDATE_PROGRESSING		0x0518

/* ---------------------------------- */

/* DRIME4 Liveview Movie Mode */
#define DRIME4_LIVE_MOV_OFF				0x0
#define DRIME4_LIVEVIEW_MODE			0x1
#define DRIME4_MOVIE_MODE				0x2

/* DRIME4 AF Area */
#define DRIME4_AF_AREA_SEL				0x0
#define DRIME4_AF_AREA_MULTI			0x1
#define DRIME4_AF_AREA_FD				0x2
#define DRIME4_AF_AREA_SELF				0x3


/* ---------------------------------- */

#define DRIME4_IMG_OUTPUT	0x0902
#define DRIME4_HDR_OUTPUT	0x0008
#define DRIME4_YUV_OUTPUT	0x0009
#define DRIME4_INTERLEAVED_OUTPUT	0x000D
#define DRIME4_HYBRID_OUTPUT	0x0016

#define DRIME4_STILL_PRE_FLASH			0x0A00
#define DRIME4_STILL_PRE_FLASH_FIRE		0x0000
#define DRIME4_STILL_PRE_FLASH_NON_FIRED	0x0000
#define DRIME4_STILL_PRE_FLASH_FIRED		0x0001

#define DRIME4_STILL_MAIN_FLASH		0x0A02
#define DRIME4_STILL_MAIN_FLASH_CANCEL		0x0001
#define DRIME4_STILL_MAIN_FLASH_FIRE		0x0002


#define DRIME4_ZOOM_STEP	0x0B00


#define DRIME4_IMAGE_EFFECT		0x0B0A
#define DRIME4_IMAGE_EFFECT_NONE	0x0001
#define DRIME4_IMAGE_EFFECT_NEGATIVE	0x0002
#define DRIME4_IMAGE_EFFECT_AQUA	0x0003
#define DRIME4_IMAGE_EFFECT_SEPIA	0x0004
#define DRIME4_IMAGE_EFFECT_MONO	0x0005
#define DRIME4_IMAGE_EFFECT_SKETCH	0x0006
#define DRIME4_IMAGE_EFFECT_WASHED	0x0007
#if 0
#define DRIME4_IMAGE_EFFECT_VINTAGE_WARM	0x0008
#define DRIME4_IMAGE_EFFECT_VINTAGE_COLD	0x0009
#endif
#define DRIME4_IMAGE_EFFECT_SOLARIZE	0x000A
#if 0
#define DRIME4_IMAGE_EFFECT_POSTERIZE	0x000B
#endif
/* U-camera test */
#define DRIME4_IMAGE_EFFECT_VINTAGE_WARM	0x05
#define DRIME4_IMAGE_EFFECT_VINTAGE_COLD	0x02
#define DRIME4_IMAGE_EFFECT_POSTERIZE	0x01
/* */

#define DRIME4_IMAGE_EFFECT_POINT_BLUE	0x000C
#define DRIME4_IMAGE_EFFECT_POINT_RED_YELLOW	0x000D
#define DRIME4_IMAGE_EFFECT_POINT_COLOR_3	0x000E
#define DRIME4_IMAGE_EFFECT_POINT_GREEN	0x000F
#define DRIME4_IMAGE_EFFECT_CARTOONIZE	0x001A

#define DRIME4_IMAGE_QUALITY		0x0B0C
#define DRIME4_IMAGE_QUALITY_SUPERFINE	0x0000
#define DRIME4_IMAGE_QUALITY_FINE	0x0001
#define DRIME4_IMAGE_QUALITY_NORMAL	0x0002


#define DRIME4_FLASH_MODE		0x0B0E
#define DRIME4_FLASH_MODE_OFF		0x0000
#define DRIME4_FLASH_MODE_ON		0x0001
#define DRIME4_FLASH_MODE_AUTO		0x0002

#define DRIME4_FLASH_TORCH		0x0B12
#define DRIME4_FLASH_TORCH_OFF		0x0000
#define DRIME4_FLASH_TORCH_ON		0x0001

#define DRIME4_AE_ISNEEDFLASH		0x0CBA
#define DRIME4_AE_ISNEEDFLASH_OFF	0x0000
#define DRIME4_AE_ISNEEDFLASH_ON	0x0001


#define DRIME4_CHG_MODE		0x0B10
#define DRIME4_DEFAULT_MODE		0x8000
#define DRIME4_FAST_MODE_SUBSAMPLING_HALF	0xA000
#define DRIME4_FAST_MODE_SUBSAMPLING_QUARTER	0xC000

#define DRIME4_AF_CON			0x0E00
#define DRIME4_AF_CON_STOP		0x0000
#define DRIME4_AF_CON_SCAN		0x0001/*AF_SCAN:Full Search*/
#define DRIME4_AF_CON_START	0x0002/*AF_START:Fast Search*/

#define DRIME4_AF_STATUS		0x5E80

#define DRIME4_AF_TOUCH_AF		0x0E0A

#define DRIME4_AF_CAL			0x0E06

#define DRIME4_CAF_STATUS_FIND_SEARCHING_DIR	0x0001
#define DRIME4_CAF_STATUS_FOCUSING	0x0002
#define DRIME4_CAF_STATUS_FOCUSED	0x0003
#define DRIME4_CAF_STATUS_UNFOCUSED	0x0004

#define DRIME4_AF_STATUS_INVALID	0x0010
#define DRIME4_AF_STATUS_FOCUSING	0x0020
#define DRIME4_AF_STATUS_FOCUSED	0x0030/*SUCCESS*/
#define DRIME4_AF_STATUS_UNFOCUSED	0x0040/*FAIL*/

#define DRIME4_AF_TOUCH_POSITION	0x5E8E

#define DRIME4_AF_FACE_ZOOM	0x0E10

#define DRIME4_AF_MODE			0x0E02
#define DRIME4_AF_MODE_NORMAL		0x0000
#define DRIME4_AF_MODE_MACRO		0x0001
#define DRIME4_AF_MODE_MOVIE_CAF_START	0x0002
#define DRIME4_AF_MODE_MOVIE_CAF_STOP		0x0003
#define DRIME4_AF_MODE_PREVIEW_CAF_START	0x0004
#define DRIME4_AF_MODE_PREVIEW_CAF_STOP	0x0005

#define DRIME4_AF_SOFTLANDING		0x0E16
#define DRIME4_AF_SOFTLANDING_ON	0x0000

#define DRIME4_FACE_DET		0x0E0C
#define DRIME4_FACE_DET_OFF		0x0000
#define DRIME4_FACE_DET_ON		0x0001

#define DRIME4_FACE_DET_OSD		0x0E0E
#define DRIME4_FACE_DET_OSD_OFF	0x0000
#define DRIME4_FACE_DET_OSD_ON		0x0001

#define DRIME4_AE_CON		0x0C00
#define DRIME4_AE_STOP		0x0000/*LOCK*/
#define DRIME4_AE_START	0x0001/*UNLOCK*/

#define DRIME4_ISO		0x0C02
#define DRIME4_ISO_AUTO	0x0000
#define DRIME4_ISO_100		0x0001
#define DRIME4_ISO_200		0x0002
#define DRIME4_ISO_400		0x0003
#define DRIME4_ISO_800		0x0004
#define DRIME4_ISO_SPORTS	0x0005
#define DRIME4_ISO_NIGHT	0x0006
#define DRIME4_ISO_INDOOR	0x0007

#define DRIME4_EV		0x0C04
#define DRIME4_EV_M20		0x0000
#define DRIME4_EV_M15		0x0001
#define DRIME4_EV_M10		0x0002
#define DRIME4_EV_M05		0x0003
#define DRIME4_EV_ZERO		0x0004
#define DRIME4_EV_P05		0x0005
#define DRIME4_EV_P10		0x0006
#define DRIME4_EV_P15		0x0007
#define DRIME4_EV_P20		0x0008

#define DRIME4_METER		0x0C06
#define DRIME4_METER_CENTER	0x0000
#define DRIME4_METER_SPOT	0x0001
#define DRIME4_METER_AVERAGE	0x0002
#define DRIME4_METER_SMART	0x0003

#define DRIME4_WDR		0x0C08
#define DRIME4_WDR_OFF		0x0000
#define DRIME4_WDR_ON		0x0001

#define DRIME4_FLICKER_MODE	0x0C12
#define DRIME4_FLICKER_NONE	0x0000
#define DRIME4_FLICKER_MANUAL_50HZ	0x0001
#define DRIME4_FLICKER_MANUAL_60HZ	0x0002
#define DRIME4_FLICKER_AUTO	0x0003
#define DRIME4_FLICKER_AUTO_50HZ	0x0004
#define DRIME4_FLICKER_AUTO_60HZ	0x0005

#define DRIME4_AE_MODE	0x0C1E
#define DRIME4_AUTO_MODE_AE_SET	0x0000
#define DRIME4_FIXED_30FPS	0x0002
#define DRIME4_FIXED_20FPS	0x0003
#define DRIME4_FIXED_15FPS	0x0004
#define DRIME4_FIXED_60FPS	0x0007
#define DRIME4_FIXED_120FPS	0x0008
#define DRIME4_FIXED_7FPS	0x0009
#define DRIME4_FIXED_10FPS	0x000A
#define DRIME4_FIXED_90FPS	0x000B
#define DRIME4_ANTI_SHAKE	0x0013

#define DRIME4_SHARPNESS	0x0C14
#define DRIME4_SHARPNESS_0	0x0000
#define DRIME4_SHARPNESS_1	0x0001
#define DRIME4_SHARPNESS_2	0x0002
#define DRIME4_SHARPNESS_M1	0x0003
#define DRIME4_SHARPNESS_M2	0x0004

#define DRIME4_SATURATION	0x0C16
#define DRIME4_SATURATION_0	0x0000
#define DRIME4_SATURATION_1	0x0001
#define DRIME4_SATURATION_2	0x0002
#define DRIME4_SATURATION_M1	0x0003
#define DRIME4_SATURATION_M2	0x0004

#define DRIME4_CONTRAST	0x0C18
#define DRIME4_CONTRAST_0	0x0000
#define DRIME4_CONTRAST_1	0x0001
#define DRIME4_CONTRAST_2	0x0002
#define DRIME4_CONTRAST_M1	0x0003
#define DRIME4_CONTRAST_M2	0x0004

#define DRIME4_SCENE_MODE		0x0C1A
#define DRIME4_SCENE_MODE_NONE		0x0000
#define DRIME4_SCENE_MODE_PORTRAIT	0x0001
#define DRIME4_SCENE_MODE_LANDSCAPE	0x0002
#define DRIME4_SCENE_MODE_SPORTS	0x0003
#define DRIME4_SCENE_MODE_INDOOR	0x0004
#define DRIME4_SCENE_MODE_BEACH	0x0005
#define DRIME4_SCENE_MODE_SUNSET	0x0006
#define DRIME4_SCENE_MODE_DAWN		0x0007
#define DRIME4_SCENE_MODE_FALL		0x0008
#define DRIME4_SCENE_MODE_NIGHT	0x0009
#define DRIME4_SCENE_MODE_AGAINSTLIGHT	0x000A
#define DRIME4_SCENE_MODE_FIRE		0x000B
#define DRIME4_SCENE_MODE_TEXT		0x000C
#define DRIME4_SCENE_MODE_CANDLE	0x000D
#define DRIME4_SCENE_MODE_LOW_LIGHT	0x0020

#define DRIME4_FIREWORK_CAPTURE 0x0C20
#define DRIME4_NIGHTSHOT_CAPTURE 0x0C22

#define DRIME4_AE_LOW_LIGHT_MODE 0x0C2C

#define DRIME4_AE_AUTO_BRAKET		0x0B14
#define DRIME4_AE_AUTO_BRAKET_EV05	0x0080
#define DRIME4_AE_AUTO_BRAKET_EV10	0x0100
#define DRIME4_AE_AUTO_BRAKET_EV15	0x0180
#define DRIME4_AE_AUTO_BRAKET_EV20	0x0200

#define DRIME4_SENSOR_STREAMING	0x090A
#define DRIME4_SENSOR_STREAMING_OFF	0x0000
#define DRIME4_SENSOR_STREAMING_ON	0x0001

#define DRIME4_AWB_MODE		0x0D02
#define DRIME4_AWB_MODE_INCANDESCENT	0x0000
#define DRIME4_AWB_MODE_FLUORESCENT1	0x0001
#define DRIME4_AWB_MODE_FLUORESCENT2	0x0002
#define DRIME4_AWB_MODE_DAYLIGHT	0x0003
#define DRIME4_AWB_MODE_CLOUDY		0x0004
#define DRIME4_AWB_MODE_AUTO		0x0005

#define DRIME4_AWB_CON			0x0D00
#define DRIME4_AWB_STOP		0x0000/*LOCK*/
#define DRIME4_AWB_START		0x0001/*UNLOCK*/

#define DRIME4_HYBRID_CAPTURE	0x0996

#define DRIME4_STATUS			0x5080
#define BOOT_SUB_MAIN_ENTER		0xFF01
#define BOOT_SRAM_TIMING_OK		0xFF02
#define BOOT_INTERRUPTS_ENABLE		0xFF03
#define BOOT_R_PLL_DONE			0xFF04
#define BOOT_R_PLL_LOCKTIME_DONE	0xFF05
#define BOOT_DELAY_COUNT_DONE		0xFF06
#define BOOT_I_PLL_DONE			0xFF07
#define BOOT_I_PLL_LOCKTIME_DONE	0xFF08
#define BOOT_PLL_INIT_OK		0xFF09
#define BOOT_SENSOR_INIT_OK		0xFF0A
#define BOOT_GPIO_SETTING_OK		0xFF0B
#define BOOT_READ_CAL_DATA_OK		0xFF0C
#define BOOT_STABLE_AE_AWB_OK		0xFF0D
#define EXCEPTION_OCCURED		0xDEAD

#define DRIME4_I2C_SEQ_STATUS	0x59A6
#define SEQ_END_PLL		(1<<0x0)
#define SEQ_END_SENSOR		(1<<0x1)
#define SEQ_END_GPIO		(1<<0x2)
#define SEQ_END_FROM		(1<<0x3)
#define SEQ_END_STABLE_AE_AWB	(1<<0x4)
#define SEQ_END_READY_I2C_CMD	(1<<0x5)

#define DRIME4_I2C_ERR_STATUS		0x599E
#define ERR_STATUS_CIS_I2C		(1<<0x0)
#define ERR_STATUS_AF_INIT		(1<<0x1)
#define ERR_STATUS_CAL_DATA		(1<<0x2)
#define ERR_STATUS_FRAME_COUNT		(1<<0x3)
#define ERR_STATUS_FROM_INIT		(1<<0x4)
#define ERR_STATUS_I2C_CIS_STREAM_OFF	(1<<0x5)
#define ERR_STATUS_I2C_N_CMD_OVER	(1<<0x6)
#define ERROR_STATUS_I2C_N_CMD_MISMATCH    (1<<0x7)
#define ERROR_STATUS_CHECK_BIN_CRC    (1<<0x8)
#define ERROR_STATUS_EXCEPTION    (1<<0x9)
#define ERROR_STATUS_INIF_INIT_STATE    (0x8)

#ifdef CONFIG_VIDEO_DRIME4_SPI
extern int drime4_spi_write(const u8 *addr, const int len, const int txSize);
extern int drime4_spi_com_write(u8 len, u8 cate, u8 byte, long long val);
extern int drime4_spi_read(u8 *buf, size_t len, const int rxSize);
extern int drime4_spi_write_packet(struct v4l2_subdev *sd,
	u8 packet_id, u32 len, u8 *payload, int log);
extern int drime4_spi_write_burst(u8 packet_id, u32 len,
	const u8 *payload, int log);
extern int drime4_spi_write_burst_only_data(u32 len,
	const u8 *payload, int log);
extern int drime4_spi_read_res(u8 *buf, int noti, int log);
extern int drime4_spi_read_length(int size);
extern int drime4_spi_read_burst(u8 *buf, int size);
extern int drime4_spi_get_write_status(void);
extern int drime4_spi_get_noti_ctrl_pin(void);
extern int drime4_spi_get_cmd_ctrl_pin(void);
extern int drime4_spi_get_noti_check_pin(void);
extern int drime4_spi_set_noti_ctrl_pin(int val);
extern int drime4_spi_set_cmd_ctrl_pin(int val);
#endif

#endif /* __DRIME4_H */
