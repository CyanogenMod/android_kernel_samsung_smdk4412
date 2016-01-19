/*
 * Driver for LSI S5C73M3 (ISP for 8MP Camera)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SLP_S5C73M3_H
#define __SLP_S5C73M3_H

#define CONFIG_CAM_DEBUG	1

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
			printk(KERN_DEBUG "%s: " fmt, \
					__func__, ##__VA_ARGS__); \
	} while (0)

#define cam_trace(fmt, ...)	\
	do { \
		if (to_state(sd)->dbg_level & CAM_TRACE) \
			printk(KERN_DEBUG "%s: " fmt, \
					__func__, ##__VA_ARGS__); \
	} while (0)

#define cam_i2c_dbg(fmt, ...)	\
	do { \
		if (to_state(sd)->dbg_level & CAM_I2C) \
			printk(KERN_DEBUG "%s: " fmt, \
					__func__, ##__VA_ARGS__); \
	} while (0)
#else
#define cam_dbg(fmt, ...)
#define cam_trace(fmt, ...)
#define cam_i2c_dbg(fmt, ...)
#endif

enum s5c73m3_prev_frmsize {
	S5C73M3_PREVIEW_QCIF,
	S5C73M3_PREVIEW_QCIF2,
	S5C73M3_PREVIEW_QVGA,
	S5C73M3_PREVIEW_CIF,
	S5C73M3_PREVIEW_VGA,
	S5C73M3_PREVIEW_D1,
	S5C73M3_PREVIEW_WVGA,
	S5C73M3_PREVIEW_880X720,
	S5C73M3_PREVIEW_960X720,
	S5C73M3_PREVIEW_1008X672,
	S5C73M3_PREVIEW_1056X704,
	S5C73M3_PREVIEW_1184X666,
	S5C73M3_PREVIEW_720P,
	S5C73M3_VDIS_720P,
	S5C73M3_PREVIEW_1080P,
	S5C73M3_VDIS_1080P,
	S5C73M3_PREVIEW_HDR,
};

enum s5c73m3_cap_frmsize {
	S5C73M3_CAPTURE_VGA,	/* 640 x 480 */
	S5C73M3_CAPTURE_WVGA,	/* 800 x 480 */
	S5C73M3_CAPTURE_1024X768,	/* 1024 x 768 */
	S5C73M3_CAPTURE_HD,	/* 1280 x 720 */
	S5C73M3_CAPTURE_W1MP,	/* 1600 x 960 */
	S5C73M3_CAPTURE_2MP,	/* UXGA - 1600 x 1200 */
	S5C73M3_CAPTURE_W2MP,	/* 2048 x 1232 */
	S5C73M3_CAPTURE_3MP,	/* QXGA - 2048 x 1536 */
	S5C73M3_CAPTURE_W4MP,	/* WQXGA - 2560 x 1440 */
	S5C73M3_CAPTURE_5MP,	/* 2560 x 1920 */
	S5C73M3_CAPTURE_W6MP,	/* 3072 x 1856 */
	S5C73M3_CAPTURE_7MP,	/* 3072 x 2304 */
	S5C73M3_CAPTURE_W7MP,	/* WQXGA - 2560 x 1536 */
	S5C73M3_CAPTURE_3264X2176,	/* 3264 x 2176 */
	S5C73M3_CAPTURE_8MP,	/* 3264 x 2448 */
};

enum s5c73m3_isneed_flash_tristate {
	S5C73M3_ISNEED_FLASH_OFF = 0x00,
	S5C73M3_ISNEED_FLASH_ON = 0x01,
	S5C73M3_ISNEED_FLASH_UNDEFINED = 0x02,
};

struct s5c73m3_control {
	u32 id;
	s32 value;
	s32 minimum;		/* Note signedness */
	s32 maximum;
	s32 step;
	s32 default_value;
};

struct s5c73m3_frmsizeenum {
	unsigned int index;
	unsigned int width;
	unsigned int height;
	u8 reg_val;		/* a value for category parameter */
};

struct s5c73m3_isp {
	wait_queue_head_t wait;
	unsigned int irq;	/* irq issued by ISP */
	unsigned int issued;
	unsigned int int_factor;
	unsigned int bad_fw:1;
};

struct s5c73m3_jpeg {
	int quality;
	unsigned int main_size;	/* Main JPEG file size */
	unsigned int thumb_size;	/* Thumbnail file size */
	unsigned int main_offset;
	unsigned int thumb_offset;
	unsigned int postview_offset;
};

struct s5c73m3_focus {
	unsigned int mode;
	unsigned int lock;
	unsigned int status;
	unsigned int touch;
	unsigned int pos_x;
	unsigned int pos_y;

	/* rectangle type values */
	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;
};

struct s5c73m3_exif {
	char unique_id[7];
	u32 exptime;		/* us */
	u16 flash;
	u16 iso;
	int tv;			/* shutter speed */
	int bv;			/* brightness */
	int ebv;		/* exposure bias */
	int effect;		/* effect(colorfx) */
	int saturation;
	int sharpness;
	int metering;
	int wdr;
};

struct s5c73m3_state {
	struct s5c73m3_platform_data *pdata;
	struct v4l2_subdev sd;

	struct s5c73m3_isp isp;

	const struct s5c73m3_frmsizeenum *preview;
	const struct s5c73m3_frmsizeenum *capture;

	enum v4l2_pix_format_mode format_mode;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_flash_mode flash_mode;
	enum v4l2_wb_mode wb_mode;
	enum v4l2_scene_mode scene_mode;
	int vt_mode;
	int beauty_mode;
	int hdr_mode;
	int hybrid_mode;
	int zoom;
	int stream_enable;
	int ae_lock;
	int awb_lock;

	int cal_device;
	int cal_dll;

	unsigned int fps;
	struct s5c73m3_focus focus;
	int caf_mode;
	char isflash;

	struct s5c73m3_jpeg jpeg;
	struct s5c73m3_exif exif;

	int check_dataline;
	char *fw_version;

	u32 jpeg_width;
	u32 jpeg_height;

	u8 sensor_fw[10];
	u8 phone_fw[10];

	u8 sensor_type[15];

#ifdef CONFIG_CAM_DEBUG
	u8 dbg_level;
#endif
};

#define S5C73M3_IMG_OUTPUT	0x0902
#define S5C73M3_HDR_OUTPUT	0x0008
#define S5C73M3_YUV_OUTPUT	0x0009
#define S5C73M3_INTERLEAVED_OUTPUT	0x000D
#define S5C73M3_HYBRID_OUTPUT	0x0016

#define S5C73M3_STILL_PRE_FLASH			0x0A00
#define S5C73M3_STILL_PRE_FLASH_FIRE		0x0000
#define S5C73M3_STILL_PRE_FLASH_NON_FIRED	0x0000
#define S5C73M3_STILL_PRE_FLASH_FIRED		0x0001

#define S5C73M3_STILL_MAIN_FLASH		0x0A02
#define S5C73M3_STILL_MAIN_FLASH_CANCEL		0x0001
#define S5C73M3_STILL_MAIN_FLASH_FIRE		0x0002


#define S5C73M3_ZOOM_STEP	0x0B00


#define S5C73M3_IMAGE_EFFECT		0x0B0A
#define S5C73M3_IMAGE_EFFECT_NONE	0x0001
#define S5C73M3_IMAGE_EFFECT_NEGATIVE	0x0002
#define S5C73M3_IMAGE_EFFECT_AQUA	0x0003
#define S5C73M3_IMAGE_EFFECT_SEPIA	0x0004
#define S5C73M3_IMAGE_EFFECT_MONO	0x0005

#define S5C73M3_IMAGE_QUALITY		0x0B0C
#define S5C73M3_IMAGE_QUALITY_SUPERFINE	0x0000
#define S5C73M3_IMAGE_QUALITY_FINE	0x0001
#define S5C73M3_IMAGE_QUALITY_NORMAL	0x0002


#define S5C73M3_FLASH_MODE		0x0B0E
#define S5C73M3_FLASH_MODE_OFF		0x0000
#define S5C73M3_FLASH_MODE_ON		0x0001
#define S5C73M3_FLASH_MODE_AUTO		0x0002

#define S5C73M3_FLASH_TORCH		0x0B12
#define S5C73M3_FLASH_TORCH_OFF		0x0000
#define S5C73M3_FLASH_TORCH_ON		0x0001

#define S5C73M3_AE_ISNEEDFLASH		0x0CBA
#define S5C73M3_AE_ISNEEDFLASH_OFF	0x0000
#define S5C73M3_AE_ISNEEDFLASH_ON	0x0001


#define S5C73M3_CHG_MODE		0x0B10
#define S5C73M3_YUV_MODE		0x8000
#define S5C73M3_INTERLEAVED_MODE	0x8000
#define S5C73M3_CHG_MODE_YUV_320_240	0x8001
#define S5C73M3_CHG_MODE_YUV_400_300	0x8002
#define S5C73M3_CHG_MODE_YUV_640_480	0x8003
#define S5C73M3_CHG_MODE_YUV_800_600	0x8004
#define S5C73M3_CHG_MODE_YUV_960_720	0x8005
#define S5C73M3_CHG_MODE_YUV_1280_720	0x8006
#define S5C73M3_CHG_MODE_YUV_1280_960	0x8007
#define S5C73M3_CHG_MODE_YUV_1600_1200	0x8008
#define S5C73M3_CHG_MODE_YUV_1632_1224	0x8009
#define S5C73M3_CHG_MODE_YUV_1920_1080	0x800A
#define S5C73M3_CHG_MODE_YUV_1920_1440	0x800B
#define S5C73M3_CHG_MODE_YUV_2304_1296	0x800C
#define S5C73M3_CHG_MODE_YUV_2304_1728	0x800D
#define S5C73M3_CHG_MODE_JPEG_640_480	0x0010
#define S5C73M3_CHG_MODE_JPEG_800_450	0x0020
#define S5C73M3_CHG_MODE_JPEG_800_600	0x0030
#define S5C73M3_CHG_MODE_JPEG_1600_960	0x0040
#define S5C73M3_CHG_MODE_JPEG_1600_1200	0x0050
#define S5C73M3_CHG_MODE_JPEG_2048_1152	0x0060
#define S5C73M3_CHG_MODE_JPEG_2048_1536	0x0070
#define S5C73M3_CHG_MODE_JPEG_2560_1440	0x0080
#define S5C73M3_CHG_MODE_JPEG_2560_1920	0x0090
#define S5C73M3_CHG_MODE_JPEG_3072_1728	0x00A0
#define S5C73M3_CHG_MODE_JPEG_3264_2304	0x00B0
#define S5C73M3_CHG_MODE_JPEG_3264_1836	0x00C0
#define S5C73M3_CHG_MODE_JPEG_3264_2448	0x00D0


#define S5C73M3_AF_CON			0x0E00
#define S5C73M3_AF_CON_STOP		0x0000
#define S5C73M3_AF_CON_SCAN		0x0001/*AF_SCAN:Full Search*/
#define S5C73M3_AF_CON_START	0x0002/*AF_START:Fast Search*/

#define S5C73M3_AF_STATUS		0x5E80

#define S5C73M3_AF_TOUCH_AF		0x0E0A

#define S5C73M3_AF_CAL			0x0E06

#define S5C73M3_CAF_STATUS_FIND_SEARCHING_DIR	0x0001
#define S5C73M3_CAF_STATUS_FOCUSING	0x0002
#define S5C73M3_CAF_STATUS_FOCUSED	0x0003
#define S5C73M3_CAF_STATUS_UNFOCUSED	0x0004

#define S5C73M3_AF_STATUS_INVALID	0x0010
#define S5C73M3_AF_STATUS_FOCUSING	0x0020
#define S5C73M3_AF_STATUS_FOCUSED	0x0030/*SUCCESS*/
#define S5C73M3_AF_STATUS_UNFOCUSED	0x0040/*FAIL*/

#define S5C73M3_AF_TOUCH_POSITION	0x5E8E

#define S5C73M3_AF_FACE_ZOOM	0x0E10

#define S5C73M3_AF_MODE			0x0E02
#define S5C73M3_AF_MODE_NORMAL		0x0000
#define S5C73M3_AF_MODE_MACRO		0x0001
#define S5C73M3_AF_MODE_MOVIE_CAF_START	0x0002
#define S5C73M3_AF_MODE_MOVIE_CAF_STOP		0x0003
#define S5C73M3_AF_MODE_PREVIEW_CAF_START	0x0004
#define S5C73M3_AF_MODE_PREVIEW_CAF_STOP	0x0005

#define S5C73M3_AF_SOFTLANDING		0x0E16
#define S5C73M3_AF_SOFTLANDING_ON	0x0000

#define S5C73M3_FACE_DET		0x0E0C
#define S5C73M3_FACE_DET_OFF		0x0000
#define S5C73M3_FACE_DET_ON		0x0001

#define S5C73M3_FACE_DET_OSD		0x0E0E
#define S5C73M3_FACE_DET_OSD_OFF	0x0000
#define S5C73M3_FACE_DET_OSD_ON		0x0001

#define S5C73M3_AE_CON		0x0C00
#define S5C73M3_AE_STOP		0x0000/*LOCK*/
#define S5C73M3_AE_START	0x0001/*UNLOCK*/

#define S5C73M3_ISO		0x0C02
#define S5C73M3_ISO_AUTO	0x0000
#define S5C73M3_ISO_100		0x0001
#define S5C73M3_ISO_200		0x0002
#define S5C73M3_ISO_400		0x0003
#define S5C73M3_ISO_800		0x0004
#define S5C73M3_ISO_SPORTS	0x0005
#define S5C73M3_ISO_NIGHT	0x0006
#define S5C73M3_ISO_INDOOR	0x0007

#define S5C73M3_EV		0x0C04
#define S5C73M3_EV_M20		0x0000
#define S5C73M3_EV_M15		0x0001
#define S5C73M3_EV_M10		0x0002
#define S5C73M3_EV_M05		0x0003
#define S5C73M3_EV_ZERO		0x0004
#define S5C73M3_EV_P05		0x0005
#define S5C73M3_EV_P10		0x0006
#define S5C73M3_EV_P15		0x0007
#define S5C73M3_EV_P20		0x0008

#define S5C73M3_METER		0x0C06
#define S5C73M3_METER_CENTER	0x0000
#define S5C73M3_METER_SPOT	0x0001
#define S5C73M3_METER_AVERAGE	0x0002
#define S5C73M3_METER_SMART	0x0003

#define S5C73M3_WDR		0x0C08
#define S5C73M3_WDR_OFF		0x0000
#define S5C73M3_WDR_ON		0x0001

#define S5C73M3_AE_MODE	0x0C1E
#define S5C73M3_AUTO_MODE_AE_SET	0x0000
#define S5C73M3_FIXED_30FPS	0x0002
#define S5C73M3_FIXED_20FPS	0x0003
#define S5C73M3_FIXED_15FPS	0x0004
#define S5C73M3_FIXED_120FPS	0x0008
#define S5C73M3_FIXED_7FPS	0x0009
#define S5C73M3_ANTI_SHAKE	0x0013

#define S5C73M3_SHARPNESS	0x0C14
#define S5C73M3_SHARPNESS_0	0x0000
#define S5C73M3_SHARPNESS_1	0x0001
#define S5C73M3_SHARPNESS_2	0x0002
#define S5C73M3_SHARPNESS_M1	0x0003
#define S5C73M3_SHARPNESS_M2	0x0004

#define S5C73M3_SATURATION	0x0C16
#define S5C73M3_SATURATION_0	0x0000
#define S5C73M3_SATURATION_1	0x0001
#define S5C73M3_SATURATION_2	0x0002
#define S5C73M3_SATURATION_M1	0x0003
#define S5C73M3_SATURATION_M2	0x0004

#define S5C73M3_CONTRAST	0x0C18
#define S5C73M3_CONTRAST_0	0x0000
#define S5C73M3_CONTRAST_1	0x0001
#define S5C73M3_CONTRAST_2	0x0002
#define S5C73M3_CONTRAST_M1	0x0003
#define S5C73M3_CONTRAST_M2	0x0004

#define S5C73M3_SCENE_MODE		0x0C1A
#define S5C73M3_SCENE_MODE_NONE		0x0000
#define S5C73M3_SCENE_MODE_PORTRAIT	0x0001
#define S5C73M3_SCENE_MODE_LANDSCAPE	0x0002
#define S5C73M3_SCENE_MODE_SPORTS	0x0003
#define S5C73M3_SCENE_MODE_INDOOR	0x0004
#define S5C73M3_SCENE_MODE_BEACH	0x0005
#define S5C73M3_SCENE_MODE_SUNSET	0x0006
#define S5C73M3_SCENE_MODE_DAWN		0x0007
#define S5C73M3_SCENE_MODE_FALL		0x0008
#define S5C73M3_SCENE_MODE_NIGHT	0x0009
#define S5C73M3_SCENE_MODE_AGAINSTLIGHT	0x000A
#define S5C73M3_SCENE_MODE_FIRE		0x000B
#define S5C73M3_SCENE_MODE_TEXT		0x000C
#define S5C73M3_SCENE_MODE_CANDLE	0x000D

#define S5C73M3_FIREWORK_CAPTURE 0x0C20

#define S5C73M3_AE_AUTO_BRAKET		0x0B14
#define S5C73M3_AE_AUTO_BRAKET_EV05	0x0080
#define S5C73M3_AE_AUTO_BRAKET_EV10	0x0100
#define S5C73M3_AE_AUTO_BRAKET_EV15	0x0180
#define S5C73M3_AE_AUTO_BRAKET_EV20	0x0200

#define S5C73M3_SENSOR_STREAMING	0x090A
#define S5C73M3_SENSOR_STREAMING_OFF	0x0000
#define S5C73M3_SENSOR_STREAMING_ON	0x0001

#define S5C73M3_AWB_MODE		0x0D02
#define S5C73M3_AWB_MODE_INCANDESCENT	0x0000
#define S5C73M3_AWB_MODE_FLUORESCENT1	0x0001
#define S5C73M3_AWB_MODE_FLUORESCENT2	0x0002
#define S5C73M3_AWB_MODE_DAYLIGHT	0x0003
#define S5C73M3_AWB_MODE_CLOUDY		0x0004
#define S5C73M3_AWB_MODE_AUTO		0x0005

#define S5C73M3_AWB_CON			0x0D00
#define S5C73M3_AWB_STOP		0x0000/*LOCK*/
#define S5C73M3_AWB_START		0x0001/*UNLOCK*/

#define S5C73M3_HYBRID_CAPTURE	0x0996

/* S5C73M3 Sensor Mode */
#define S5C73M3_SYSINIT_MODE	0x0
#define S5C73M3_PARMSET_MODE	0x1
#define S5C73M3_MONITOR_MODE	0x2
#define S5C73M3_STILLCAP_MODE	0x3

/* Interrupt Factor */
#define S5C73M3_INT_SOUND		(1 << 7)
#define S5C73M3_INT_LENS_INIT	(1 << 6)
#define S5C73M3_INT_FD		(1 << 5)
#define S5C73M3_INT_FRAME_SYNC	(1 << 4)
#define S5C73M3_INT_CAPTURE	(1 << 3)
#define S5C73M3_INT_ZOOM		(1 << 2)
#define S5C73M3_INT_AF		(1 << 1)
#define S5C73M3_INT_MODE		(1 << 0)

/* ESD Interrupt */
#define S5C73M3_INT_ESD		(1 << 0)

static const u32 S5C73M3_INIT[] = {
0x00500009,
0x00545000,
0x0F140B08,
0x0F140000,
0x0F140900,
0x0F140403, /*640MHz*/
0x00545080,
0x0F140002
};

static u32 S5C73M3_OTP_CONTROL[] = {
0xFCFC3310,
0x00503800,
0x0054A004,
0x0F140000,
0x0054A000,
0x0F140004,
0x0054A0D8,
0x0F140000,
0x0054A0DC,
0x0F140004,
0x0054A0C4,
0x0F144000,
0x0054A0D4,
0x0F140015,
0x0054A000,
0x0F140001,
0x0054A0B4,
0x0F149F90,
0x0054A09C,
0x0F149A95,
};

static u32 S5C73M3_OTP_PAGE[] = {
0x0054A0C4,
0x0F144800,
0x0054A0C4,
0x0F144400,
0x0054A0C4,
0x0F144200,
0x0054A004,
0x0F1400C0,
0x0054A000,
0x0F140001,
};

#ifdef CONFIG_VIDEO_S5C73M3_SPI
extern int s5c73m3_spi_write(const u8 *addr, const int len, const int txSize);
#endif

#endif /* __SLP_S5C73M3_H */
