/*
 * driver for DMC DRIMe4 (Digital Real Image & Movie Engine)
 *
 * Copyright (c) 2012, Samsung Electronics. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/videodev2.h>
#include <linux/unistd.h>
#include <linux/sched.h>

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#define DRIME4_BUSFREQ_OPP
#define DRIME4_F_NUMBER_VALUE
#define DRIME4_F_NUMBER_VALUE_LONG

#ifdef DRIME4_BUSFREQ_OPP
#include <mach/dev.h>
#include <plat/cpu.h>
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_exynos_media.h>
#include <linux/videodev2_exynos_camera.h>
#endif

#include <linux/regulator/machine.h>

#ifdef CONFIG_LEDS_AAT1290A
#include <linux/leds-aat1290a.h>
#endif

#include <media/drime4_platform.h>
#include "drime4.h"

#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif

#define DRIME4_DRIVER_NAME	"DRIME4"

int g_drime4_boot_error;

extern struct class *camera_class; /*sys/class/camera*/
struct device *drime4_dev; /*sys/class/camera/rear*/
struct v4l2_subdev *sd_internal;
int mmc_fw;

#ifdef CONFIG_SWITCH
/* Hotshoe detection */
static struct switch_dev android_hotshoe = {
	.name = "hotshoe_mic",
};
#endif

#ifdef DRIME4_BUSFREQ_OPP
struct device *bus_dev;
#endif

#define DRIME4_LENSFW_PATH				"/sdcard/lens.bin"
#define DRIME4_LENSFW_PATH_EXT		"/storage/extSdCard/lens.bin"
#define DRIME4_LENS_FW_VER_OFFSET		256
#define DRIME4_LENS_MODEL_OFFSET			0
#define DRIME4_LENS_FW_VER_LEN			20
#define DRIME4_LENS_MODEL_LEN		20

/*#define DRIME4_FROM_BOOTING*/
#define DRIME4_NOTI_TIMEOUT		200
#define DRIME4_ISP_TIMEOUT		5000
#define DRIME4_ISP_BOOT_TIMEOUT	15000
#define DRIME4_CORE_VDD	"/data/ISP_CV"
#if 1
#define DRIME4_FW_PATH		"/sdcard/camera.bin"
#define DRIME4_FW_MMC_PATH		"/dev/block/mmcblk0p17"
#define FW_BIN_SIZE		85000000
#else
#define DRIME4_FW_PATH		"/storage/extSdCard/camera.bin"
#endif

#define DRIME4_ISP_LOG_LENGTH_BUF_SIZE	4
#define DRIME4_ISP_LOG_END_PACKET_SIZE	7
#define DRIME4_ISP_LOG_FILE_PATH	"/sdcard/ISP_dump.txt"
#define DRIME4_ISP_LOG_CHECK_PATH "/efs/di_factory/get_isp_log.dat"

#define DRIME4_FACTORY_CSV_PATH	"/sdcard/FACTORY_CSV_RAW.bin"
#define DRIME4_FACTORY_PCDATA_PATH	"/sdcard/data_raw.bin"

#define DRIME4_FW_VER_LEN		11
#define DRIME4_FW_VER_FILE_CUR	28
#define DRIME4_FW_BUF_SIZE		(60*1024*8)  /* 60KB unit */

#define DRIME4_I2C_RETRY		5

#define FAST_CAPTURE

#define INT_DIGITS 19

#define CHECK_ERR(x)	if ((x) < 0) { \
				cam_err("error check failed, err %d\n", x); \
				return x; \
			}

#define BURST_BOOT_PARAM

struct drime4_fw_version camfw_info[DRIME4_PATH_MAX];

static const struct drime4_frmsizeenum preview_frmsizes[] = {
	{ DRIME4_PREVIEW_720P,		1280,	720,	0x00 },	/* 16:9 */
	{ DRIME4_PREVIEW_1056X704,	1056,	704,	0x01 },	/* 3:2 */
	{ DRIME4_PREVIEW_960X720,	960,	720,	0x02 },	/* 4:3 */
	{ DRIME4_PREVIEW_704X704,	704,	704,	0x03 },	/* 1:1 */
	{ DRIME4_PREVIEW_VGA,		640,	480,	0x04 },	/* 4:3 */
};

#if 1	/* YUV capture 2 line gap */
/* MIPI setting Size */
static const struct drime4_frmsizeenum capture_frmsizes[] = {
	{ 0x00,	5472,	3080,	0x10 },	/* 16.9MW default */
	{ 0x01,	2048,	3292,	0x10 },
	{ 0x02,	2048,	2470,	0x10 },
	{ 0x03,	2048,	2058,	0x10 },
	{ 0x04,	2048,	8230,	0x10 },
	{ 0x10,	3712,	2088,	0x11 },	/* 7.8MW */
	{ 0x11,	2048,	1514,	0x11 },
	{ 0x12,	2048,	1136,	0x11 },
	{ 0x13,	2048,	 948,	0x11 },
	{ 0x14,	2048,	3790,	0x11 },
	{ 0x20,	2944,	1656,	0x12 },	/* 4.9MW */
	{ 0x21,	2048,	 954,	0x12 },
	{ 0x22,	2048,	 716,	0x12 },
	{ 0x23,	2048,	 596,	0x12 },
	{ 0x24,	2048,	2384,	0x12 },
	{ 0x30,	1920,	1080,	0x13 },	/* 2.1MW */
	{ 0x31,	2048,	 406,	0x13 },
	{ 0x32,	2048,	 304,	0x13 },
	{ 0x33,	2048,	 254,	0x13 },
	{ 0x34,	2048,	1016,	0x13 },
	{ 0x40,	5472,	3648,	0x00 },	/* 20MP */
	{ 0x41,	2048,	3900,	0x00 },
	{ 0x42,	2048,	2926,	0x00 },
	{ 0x43,	2048,	2438,	0x00 },
	{ 0x44,	2048,	9748,	0x00 },
	{ 0x45,	2048,	7680,	0x00 },
	{ 0x46,	2048,	11580,	0x00 },
	{ 0x47,	2048,	10606,	0x00 },
	{ 0x48,	2048,	10118,	0x00 },
	{ 0x50,	3888,	2592,	0x01 },	/* 10.1MP */
	{ 0x51,	2048,	1970,	0x01 },
	{ 0x52,	2048,	1478,	0x01 },
	{ 0x53,	2048,	1232,	0x01 },
	{ 0x54,	2048,	4926,	0x01 },
	{ 0x60,	2976,	1984,	0x02 },	/* 5.9MP */
	{ 0x61,	2048,	1154,	0x02 },
	{ 0x62,	2048,	 866,	0x02 },
	{ 0x63,	2048,	 722,	0x02 },
	{ 0x64,	2048,	2886,	0x02 },
	{ 0x70,	1728,	1152,	0x03 },	/* 2MP */
	{ 0x71,	2048,	 390,	0x03 },
	{ 0x72,	2048,	 292,	0x03 },
	{ 0x73,	2048,	 244,	0x03 },
	{ 0x74,	2048,	 974,	0x03 },
	{ 0x80,	3648,	3648,	0x20 },	/* 13.3MS */
	{ 0x81,	2048,	2600,	0x20 },
	{ 0x82,	2048,	1950,	0x20 },
	{ 0x83,	2048,	1626,	0x20 },
	{ 0x84,	2048,	6502,	0x20 },
	{ 0x90,	2640,	2640,	0x21 },	/* 7MS */
	{ 0x91,	2048,	1362,	0x21 },
	{ 0x92,	2048,	1022,	0x21 },
	{ 0x93,	2048,	 852,	0x21 },
	{ 0x94,	2048,	3406,	0x21 },
	{ 0xA0,	2000,	2000,	0x22 },	/* 4MS */
	{ 0xA1,	2048,	 782,	0x22 },
	{ 0xA2,	2048,	 586,	0x22 },
	{ 0xA3,	2048,	 490,	0x22 },
	{ 0xA4,	2048,	1956,	0x22 },
	{ 0xB0,	1024,	1024,	0x23 },	/* 1MS */
	{ 0xB1,	2048,	 206,	0x23 },
	{ 0xB2,	2048,	 154,	0x23 },
	{ 0xB3,	2048,	 128,	0x23 },
	{ 0xB4,	2048,	 514,	0x23 },
	{ 0xE0,	2736,	1824,	0x30 },	/* Burst 5MP */
	{ 0xE1,	2048,	 976,	0x30 },
	{ 0xE2,	2048,	 732,	0x30 },
	{ 0xE3,	2048,	 610,	0x30 },
	{ 0xC0,	2668,	1512,	0x31 },	/* 3D 4.1MW */
	{ 0xC1,	2048,	1576,	0x31 },
	{ 0xC2,	2048,	1182,	0x31 },
	{ 0xC3,	2048,	 986,	0x31 },
	{ 0xD0,	1920,	1080,	0x32 },	/* 3D 2.1MW */
	{ 0xD1,	2048,	 810,	0x32 },
	{ 0xD2,	2048,	 608,	0x32 },
	{ 0xD3,	2048,	 508,	0x32 },
	{ 0xF0,	 640,	 480,	0x33 },	/* VGA */
	{ 0xF1,	2048,	   60,	0x33 },
	{ 0xF2,	2048,	   46,	0x33 },
	{ 0xF3,	2048,	   38,	0x33 },
	{ 0xFF,	2048,	3200,	0x10 },	/* TEMP JPEG */
	{ 0xFE,	2048,	7680,	0x01 },	/* TEMP RAW */
	{ 0xF00,	5600,	3714,	0x50 },	/* 39.7 Still */
	{ 0xF01,	2048,	10230,	0x50 },
	{ 0xF02,	2800,	1864,	0x51 },	/* 10 Burst */
	{ 0xF03,	2048,	2549,	0x51 },
	{ 0xF04,	2800,	1576,	0x52 },	/* 8.5 Full HD */
	{ 0xF05,	2048,	2155,	0x52 },
	{ 0xF06,	2800,	1206,	0x53 },	/* 6.5 Cinema */
	{ 0xF07,	2048,	1649,	0x53 },
	{ 0xF08,	1864,	1064,	0x54 },	/* 3.8 HD */
	{ 0xF09,	2048,	973,		0x54 },
	{ 0xF0A,	1864,	1248,	0x55 },	/* 4.5 Liveview */
	{ 0xF0B,	2048,	1141,	0x55 },
	{ 0xF0C,	1864,	1248,	0x56 },	/* 4.5 MF */
	{ 0xF0D,	2048,	1141,	0x56 },
	{ 0xF0E,	1864,	544,		0x57 },	/* 2 Fast AF*/
	{ 0xF0F,	2048,	498,		0x57 },
	{ 0xF10,	3364,	1248,	0x58 },	/* 8.1 PAF Liveview */
	{ 0xF11,	2048,	2050,	0x58 },
};
#else
/* MIPI setting Size */
static const struct drime4_frmsizeenum capture_frmsizes[] = {
	{ 0x00,	5472,	3080,	0x10 },	/* 16.9MW default */
	{ 0x01,	2048,	3292,	0x10 },
	{ 0x02,	2048,	2470,	0x10 },
	{ 0x03,	2048,	2058,	0x10 },
	{ 0x04,	2048,	8230,	0x10 },
	{ 0x10,	3712,	2088,	0x11 },	/* 7.8MW */
	{ 0x11,	2048,	1514,	0x11 },
	{ 0x12,	2048,	1136,	0x11 },
	{ 0x13,	2048,	 948,	0x11 },
	{ 0x14,	2048,	3786,	0x11 },
	{ 0x20,	2944,	1656,	0x12 },	/* 4.9MW */
	{ 0x21,	2048,	 954,	0x12 },
	{ 0x22,	2048,	 716,	0x12 },
	{ 0x23,	2048,	 596,	0x12 },
	{ 0x24,	2048,	2382,	0x12 },
	{ 0x30,	1920,	1080,	0x13 },	/* 2.1MW */
	{ 0x31,	2048,	 406,	0x13 },
	{ 0x32,	2048,	 304,	0x13 },
	{ 0x33,	2048,	 254,	0x13 },
	{ 0x34,	2048,	1014,	0x13 },
	{ 0x40,	5472,	3648,	0x00 },	/* 20MP */
	{ 0x41,	2048,	3900,	0x00 },
	{ 0x42,	2048,	2926,	0x00 },
	{ 0x43,	2048,	2438,	0x00 },
	{ 0x44,	2048,	9748,	0x00 },
	{ 0x45,	2048,	7680,	0x00 },
	{ 0x46,	2048,	11580,	0x00 },
	{ 0x47,	2048,	10606,	0x00 },
	{ 0x48,	2048,	10118,	0x00 },
	{ 0x50,	3888,	2592,	0x01 },	/* 10.1MP */
	{ 0x51,	2048,	1970,	0x01 },
	{ 0x52,	2048,	1478,	0x01 },
	{ 0x53,	2048,	1232,	0x01 },
	{ 0x54,	2048,	4922,	0x01 },
	{ 0x60,	2976,	1984,	0x02 },	/* 5.9MP */
	{ 0x61,	2048,	1154,	0x02 },
	{ 0x62,	2048,	 866,	0x02 },
	{ 0x63,	2048,	 722,	0x02 },
	{ 0x64,	2048,	2884,	0x02 },
	{ 0x70,	1728,	1152,	0x03 },	/* 2MP */
	{ 0x71,	2048,	 390,	0x03 },
	{ 0x72,	2048,	 292,	0x03 },
	{ 0x73,	2048,	 244,	0x03 },
	{ 0x74,	2048,	 972,	0x03 },
	{ 0x80,	3648,	3648,	0x20 },	/* 13.3MS */
	{ 0x81,	2048,	2600,	0x20 },
	{ 0x82,	2048,	1950,	0x20 },
	{ 0x83,	2048,	1626,	0x20 },
	{ 0x84,	2048,	6498,	0x20 },
	{ 0x90,	2640,	2640,	0x21 },	/* 7MS */
	{ 0x91,	2048,	1362,	0x21 },
	{ 0x92,	2048,	1022,	0x21 },
	{ 0x93,	2048,	 852,	0x21 },
	{ 0x94,	2048,	3404,	0x21 },
	{ 0xA0,	2000,	2000,	0x22 },	/* 4MS */
	{ 0xA1,	2048,	 782,	0x22 },
	{ 0xA2,	2048,	 586,	0x22 },
	{ 0xA3,	2048,	 490,	0x22 },
	{ 0xA4,	2048,	1954,	0x22 },
	{ 0xB0,	1024,	1024,	0x23 },	/* 1MS */
	{ 0xB1,	2048,	 206,	0x23 },
	{ 0xB2,	2048,	 154,	0x23 },
	{ 0xB3,	2048,	 128,	0x23 },
	{ 0xB4,	2048,	 512,	0x23 },
	{ 0xE0,	2736,	1824,	0x30 },	/* Burst 5MP */
	{ 0xE1,	2048,	 976,	0x30 },
	{ 0xE2,	2048,	 732,	0x30 },
	{ 0xE3,	2048,	 610,	0x30 },
	{ 0xC0,	2668,	1512,	0x31 },	/* 3D 4.1MW */
	{ 0xC1,	2048,	1576,	0x31 },
	{ 0xC2,	2048,	1182,	0x31 },
	{ 0xC3,	2048,	 986,	0x31 },
	{ 0xD0,	1920,	1080,	0x32 },	/* 3D 2.1MW */
	{ 0xD1,	2048,	 810,	0x32 },
	{ 0xD2,	2048,	 608,	0x32 },
	{ 0xD3,	2048,	 508,	0x32 },
	{ 0xFF,	2048,	 3200,	0x10 },	/* TEMP JPEG */
	{ 0xFE,	2048,	 7680,	0x01 },	/* TEMP RAW */
};
#endif

/* ISP setting Size */
static const struct drime4_frmsizeenum photo_frmsizes[] = {
	{ DRIME4_CAPTURE_16p9MW,	5472,	3080,	0x10 },	/* default */
	{ DRIME4_CAPTURE_7p8MW,		3712,	2088,	0x11 },
	{ DRIME4_CAPTURE_4p9MW,		2944,	1656,	0x12 },
	{ DRIME4_CAPTURE_2p1MW,		1920,	1080,	0x13 },
	{ DRIME4_CAPTURE_20MP,		5472,	3648,	0x00 },
	{ DRIME4_CAPTURE_10p1MP,	3888,	2592,	0x01 },
	{ DRIME4_CAPTURE_5p9MP,		2976,	1984,	0x02 },
	{ DRIME4_CAPTURE_2MP,		1728,	1152,	0x03 },
	{ DRIME4_CAPTURE_13p3MS,	3648,	3648,	0x20 },
	{ DRIME4_CAPTURE_7MS,		2640,	2640,	0x21 },
	{ DRIME4_CAPTURE_4MS,		2000,	2000,	0x22 },
	{ DRIME4_CAPTURE_1p1MS,		1024,	1024,	0x23 },
	{ DRIME4_CAPTURE_15MP,		2048,	7680,	0x24 },
	{ DRIME4_CAPTURE_5MP,		2736,	1824,	0x30 },	/* Burst */
	{ DRIME4_CAPTURE_4p1MW_3D,	2668,	1512,	0x31 },	/* 3D */
	{ DRIME4_CAPTURE_2p1MW_3D,	1920,	1080,	0x32 },	/* 3D */
	{ DRIME4_CAPTURE_VGA,		 640,	 480,	0x33 },	/* VGA */
	{ DRIME4_CT3_MODE_STILL,	5600,	3714,	0x50 },
	{ DRIME4_CT3_MODE_BURST,	2800,	1864,	0x51 },
	{ DRIME4_CT3_MODE_FULLHD,	2800,	1576,	0x52 },
#if 0
	{ DRIME4_CT3_MODE_CINEMA,	2800,	1206,	0x52 },
#endif
	{ DRIME4_CT3_MODE_HD,		1864,	1064,	0x53 },
	{ DRIME4_CT3_MODE_LIVEVIEW,	1864,	1248,	0x54 },
	{ DRIME4_CT3_MODE_MF,		1864,	1248,	0x54 },
	{ DRIME4_CT3_MODE_FASTAF,	1864,	544,	0x55 },
	{ DRIME4_CT3_MODE_PAFLIVEVIEW,	3364,	1248,	0x56 },
};

static const struct drime4_frmsizeenum movie_frmsizes[] = {
	{ DRIME4_MOVIE_FHD,		1920,	1080,	0x01 },	/* default */
	{ DRIME4_MOVIE_CINE,	1920,	810,	0x02 },
	{ DRIME4_MOVIE_HD,		1280,	720,	0x03 },
	{ DRIME4_MOVIE_960_720,	960,	720,	0x04 },
	{ DRIME4_MOVIE_704_576,	704,	576,	0x05 },
	{ DRIME4_MOVIE_VGA,		640,	480,	0x06 },
	{ DRIME4_MOVIE_QVGA,	320,	240,	0x07 },
};

static const struct drime4_effectenum drime4_effects[] = {
	{IMAGE_EFFECT_NONE, DRIME4_IMAGE_EFFECT_NONE},
	{IMAGE_EFFECT_NEGATIVE, DRIME4_IMAGE_EFFECT_NEGATIVE},
	{IMAGE_EFFECT_AQUA, DRIME4_IMAGE_EFFECT_AQUA},
	{IMAGE_EFFECT_SEPIA, DRIME4_IMAGE_EFFECT_SEPIA},
	{IMAGE_EFFECT_BNW, DRIME4_IMAGE_EFFECT_MONO},
	{IMAGE_EFFECT_SKETCH, DRIME4_IMAGE_EFFECT_SKETCH},
	{IMAGE_EFFECT_WASHED, DRIME4_IMAGE_EFFECT_WASHED},
	{IMAGE_EFFECT_VINTAGE_WARM, DRIME4_IMAGE_EFFECT_VINTAGE_WARM},
	{IMAGE_EFFECT_VINTAGE_COLD, DRIME4_IMAGE_EFFECT_VINTAGE_COLD},
	{IMAGE_EFFECT_SOLARIZE, DRIME4_IMAGE_EFFECT_SOLARIZE},
	{IMAGE_EFFECT_POSTERIZE, DRIME4_IMAGE_EFFECT_POSTERIZE},
	{IMAGE_EFFECT_POINT_BLUE, DRIME4_IMAGE_EFFECT_POINT_BLUE},
	{IMAGE_EFFECT_POINT_RED_YELLOW, DRIME4_IMAGE_EFFECT_POINT_RED_YELLOW},
	{IMAGE_EFFECT_POINT_COLOR_3, DRIME4_IMAGE_EFFECT_POINT_COLOR_3},
	{IMAGE_EFFECT_POINT_GREEN, DRIME4_IMAGE_EFFECT_POINT_GREEN},
	{IMAGE_EFFECT_CARTOONIZE, DRIME4_IMAGE_EFFECT_CARTOONIZE},
};

/* Shutter Speed : Auto, Bulb, 30, ... */
static const struct drime4_fraction ss_val[] = {
	{1, 250}, {0, 0},
	{300, 10}, {250, 10}, {200, 10}, {150, 10}, {130, 10},
	{100, 10}, {80, 10}, {60, 10}, {50, 10}, {40, 10},
	{30, 10}, {25, 10}, {20, 10}, {16, 10}, {13, 10},
	{10, 10}, {8, 10}, {6, 10}, {5, 10}, {4, 10},
	{3, 10}, {1, 4}, {1, 5}, {1, 6}, {1, 8},
	{1, 10}, {1, 13}, {1, 15}, {1, 20}, {1, 25},
	{1, 30}, {1, 40}, {1, 50}, {1, 60}, {1, 80},
	{1, 100}, {1, 125}, {1, 160}, {1, 180}, {1, 200},
	{1, 250}, {1, 320}, {1, 400}, {1, 500}, {1, 640},
	{1, 800}, {1, 1000}, {1, 1250}, {1, 1600}, {1, 2000},
	{1, 2500}, {1, 3200}, {1, 4000}, {1, 5000}, {1, 6000},
};

#ifndef DRIME4_F_NUMBER_VALUE
/* F number */
static const struct drime4_fraction fn_val[] = {
	{0, 0},
	{10, 10}, {11, 10}, {12, 10}, {14, 10}, {16, 10},
	{18, 10}, {20, 10}, {22, 10}, {25, 10}, {28, 10},
	{32, 10}, {35, 10}, {40, 10}, {45, 10}, {50, 10},
	{56, 10}, {63, 10}, {71, 10}, {80, 10}, {90, 10},
	{100, 10}, {110, 10}, {130, 10}, {140, 10}, {160, 10},
	{180, 10}, {200, 10}, {220, 10}, {250, 10}, {290, 10},
	{320, 10}, {360, 10}, {400, 10}, {450, 10}, {500, 10},
	{570, 10}, {640, 10}, {720, 10}, {800, 10}, {900, 10},
};
#endif

static struct drime4_control drime4_ctrls[] = {
	{
		.id = V4L2_CID_CAMERA_ISO,
		.minimum = ISO_AUTO,
		.maximum = ISO_25600,
		.step = 1,
		.value = ISO_AUTO,
		.default_value = ISO_AUTO,
	}, {
		.id = V4L2_CID_CAMERA_ISO_AUTO_MAX_LEVEL,
		.minimum = ISO_100,
		.maximum = ISO_25600,
		.step = 1,
		.value = ISO_3200,
		.default_value = ISO_3200,
	}, {
		.id = V4L2_CID_CAMERA_BRIGHTNESS,
		.minimum = EV_MINUS_9,
		.maximum = EV_MAX - 1,
		.step = 1,
		.value = EV_DEFAULT,
		.default_value = EV_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_CONTRAST,
		.minimum = CONTRAST_EXT_MINUS_4,
		.maximum = CONTRAST_EXT_MAX - 1,
		.step = 1,
		.value = CONTRAST_EXT_DEFAULT,
		.default_value = CONTRAST_EXT_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_SATURATION,
		.minimum = SATURATION_EXT_MINUS_4,
		.maximum = SATURATION_EXT_MAX - 1,
		.step = 1,
		.value = SATURATION_EXT_DEFAULT,
		.default_value = SATURATION_EXT_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_SHARPNESS,
		.minimum = SHARPNESS_EXT_MINUS_4,
		.maximum = SHARPNESS_EXT_MAX - 1,
		.step = 1,
		.value = SHARPNESS_EXT_DEFAULT,
		.default_value = SHARPNESS_EXT_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_COLOR,
		.minimum = 0,
		.maximum = 21,
		.step = 1,
		.value = 0,
		.default_value = 0,
	}, {
		.id = V4L2_CID_CAMERA_ZOOM,
		.minimum = ZOOM_LEVEL_0,
		.maximum = ZOOM_LEVEL_MAX - 1,
		.step = 1,
		.value = ZOOM_LEVEL_0,
		.default_value = ZOOM_LEVEL_0,
	}, {
		.id = V4L2_CID_CAM_JPEG_QUALITY,
		.minimum = 1,
		.maximum = 100,
		.step = 1,
		.value = 100,
		.default_value = 100,
	},
};

static u8 sysfs_sensor_fw[20] = {0,};
static u8 sysfs_phone_fw[20] = {0,};
static u8 sysfs_isp_core[20] = {0,};
static u8 sysfs_lens_fw[20] = {0,};
static u8 sysfs_lens_binary_fw[20] = {0,};
static u8 sysfs_hotshoe_status[2] = {0,};
static u8 data_memory[500000] = {0,};
static int copied_fw_binary;
static int fw_update_result;
#ifdef BURST_BOOT_PARAM
#define MAX_PARAM_SIZE 1000
static u8 boot_param[MAX_PARAM_SIZE] = {0,};
static int param_index;
#endif

static int makeISPpinsLow(void);
static int drime4_init(struct v4l2_subdev *sd, u32 val);

static int drime4_s_stream_sensor(struct v4l2_subdev *sd, int onoff);
static int drime4_set_touch_auto_focus(struct v4l2_subdev *sd);
static const struct drime4_frmsizeenum *drime4_get_frmsize
	(const struct drime4_frmsizeenum *frmsizes, int num_entries, int index);
static int drime4_load_fw(struct v4l2_subdev *sd);
static int drime4_load_lens_fw(struct v4l2_subdev *sd);
static int drime4_check_lens_fw(struct v4l2_subdev *sd);
static int drime4_set_face_detection(struct v4l2_subdev *sd, int val);

static inline struct drime4_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct drime4_state, sd);
}

#ifndef CONFIG_VIDEO_DRIME4_SPI
static int drime4_i2c_write(struct v4l2_subdev *sd,
	unsigned short addr, unsigned short data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char buf[4] = {0,};
	int i, err;

	if (!client->adapter)
		return -ENODEV;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(buf);
	msg.buf = buf;

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;
	buf[2] = data >> 8;
	buf[3] = data & 0xff;

	cam_i2c_dbg("addr %#x, data %#x\n", addr, data);

	for (i = DRIME4_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	return err;
}

static int drime4_i2c_read(struct v4l2_subdev *sd,
	unsigned short addr, unsigned short *data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char buf[2] = {0,};
	int i, err;

	if (!client->adapter)
		return -ENODEV;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(buf);
	msg.buf = buf;

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;

	for (i = DRIME4_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		cam_err("addr %#x\n", addr);
		return err;
	}

	msg.flags = I2C_M_RD;

	for (i = DRIME4_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		cam_err("addr %#x\n", addr);
		return err;
	}

	*data = ((buf[0] << 8) | buf[1]);

	return err;
}

int drime4_spi_write_packet(struct v4l2_subdev *sd,
	u8 packet_id, u32 len, u8 *payload, int log)
{
	return 0;
}
int drime4_spi_read_res(u8 *buf, int noti, int log)
{
	return 0;
}
#endif

#if 0
static int drime4_write(struct v4l2_subdev *sd,
	unsigned short addr1, unsigned short addr2, unsigned short data)
{
	cam_trace("E\n");

	cam_trace("X\n");

	return 0;
}

static int drime4_read(struct v4l2_subdev *sd,
	unsigned short addr1, unsigned short addr2, unsigned short *data)
{
	cam_trace("E\n");

	cam_trace("X\n");

	return 0;
}
#endif

static int drime4_write0(struct v4l2_subdev *sd,
	u8 packetID, u16 ID)
{
	int all_len = 3;
	u8 wdata[all_len];

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = 0x00;

	cam_dbg("drime4_write [0] packet 0x%02x, ID 0x%02x\n",
		packetID, ID);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_writeb(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u8 val)
{
	int param_len = 1;
	int all_len = param_len+3;
	u8 wdata[all_len];

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = param_len & 0xFF;
	wdata[3] = val & 0xFF;

	cam_dbg("drime4_write [B] packet 0x%02x, ID 0x%02x, val 0x%02x\n",
		packetID, ID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_writew(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u16 val)
{
	int param_len = 2;
	int all_len = param_len+3;
	u8 wdata[all_len];
	int i;

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = param_len & 0xFF;
	for (i = 0; i < param_len; i++)
		wdata[3+i] = (val >> (8*i)) & 0xFF;

	cam_dbg("drime4_write [W] packet 0x%02x, ID 0x%02x, val 0x%04x\n",
		packetID, ID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_writel(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u32 val)
{
	int param_len = 4;
	int all_len = param_len+3;
	u8 wdata[all_len];
	int i;

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = param_len & 0xFF;
	for (i = 0; i < param_len; i++)
		wdata[3+i] = (val >> (8*i)) & 0xFF;

	cam_dbg("drime4_write [L] packet 0x%02x, ID 0x%02x, val 0x%08x\n",
		packetID, ID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_writell(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u32 val, u32 val2)
{
	int param_len = 8;
	int all_len = param_len+3;
	u8 wdata[all_len];
	int i;

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = param_len & 0xFF;
	for (i = 0; i < 4; i++)
		wdata[3+i] = (val2 >> (8*i)) & 0xFF;
	for (i = 0; i < 4; i++)
		wdata[7+i] = (val >> (8*i)) & 0xFF;

	cam_dbg("drime4_write [LL] packet 0x%02x, ID 0x%02x, val 0x%08x%08x\n",
		packetID, ID, val, val2);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_write_buf(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u32 len, u8 *buf, int log)
{
	int all_len = len+3;
	u8 wdata[all_len];
	int i;

	wdata[0] = ID & 0xFF;
	wdata[1] = (ID >> 8) & 0xFF;
	wdata[2] = len & 0xFF;

	for (i = 0; i < len; i++)
		wdata[3+i] = *(buf + i) & 0xFF;

	cam_dbg("drime4_write [%d] packet 0x%02x, ID 0x%02x\n",
		len, packetID, ID);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], log);
}

#ifdef BURST_BOOT_PARAM
static int drime4_write_multi_buf(struct v4l2_subdev *sd,
	u8 packetID, u32 len, u8 *buf, int log)
{
	int all_len = len;
	u8 wdata[all_len];
	int i;

	for (i = 0; i < len; i++)
		wdata[i] = *(buf + i) & 0xFF;

	cam_dbg("drime4_write [%d] packet 0x%02x\n", len, packetID);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], log);
}

static int drime4_add_boot_b_param(struct v4l2_subdev *sd,
	u16 payloadID, u32 len, int val)
{
	if (len != 1) {
		cam_err("unsupported value!\n");
		return -1;
	}

	cam_dbg("payloadID = 0x%04x, len = 0x%x, val = 0x%x\n",
		payloadID, len, val);

	boot_param[param_index++] = payloadID & 0xFF;
	boot_param[param_index++] = (payloadID >> 8) & 0xFF;
	boot_param[param_index++] = len;
	boot_param[param_index++] = val & 0xFF;

	return 0;
}

static int drime4_add_boot_w_param(struct v4l2_subdev *sd,
	u16 payloadID, u32 len, int val)
{
	if (len != 2) {
		cam_err("unsupported value!\n");
		return -1;
	}

	cam_dbg("payloadID = 0x%04x, len = 0x%x, val = 0x%02x\n",
		payloadID, len, val);

	boot_param[param_index++] = payloadID & 0xFF;
	boot_param[param_index++] = (payloadID >> 8) & 0xFF;
	boot_param[param_index++] = len;
	boot_param[param_index++] = val & 0xFF;
	boot_param[param_index++] = (val >> 8) & 0xFF;

	return 0;
}

static int drime4_add_boot_l_param(struct v4l2_subdev *sd,
	u16 payloadID, u32 len, int val)
{
	if (len != 4) {
		cam_err("unsupported value!\n");
		return -1;
	}

	cam_dbg("payloadID = 0x%04x, len = 0x%x, val = 0x%04x\n",
		payloadID, len, val);

	boot_param[param_index++] = payloadID & 0xFF;
	boot_param[param_index++] = (payloadID >> 8) & 0xFF;
	boot_param[param_index++] = len;
	boot_param[param_index++] = val & 0xFF;
	boot_param[param_index++] = (val >> 8) & 0xFF;
	boot_param[param_index++] = (val >> 16) & 0xFF;
	boot_param[param_index++] = (val >> 24) & 0xFF;

	return 0;
}
#endif

static int drime4_cmdb(struct v4l2_subdev *sd,
	u8 packetID, u8 val)
{
	int all_len = 1;
	u8 wdata[all_len];

	wdata[0] = val & 0xFF;

	cam_dbg("drime4_write [B] packet 0x%02x, val 0x%02x\n",
		packetID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_cmdw(struct v4l2_subdev *sd,
	u8 packetID, u16 val)
{
	int all_len = 2;
	u8 wdata[all_len];
	int i;

	for (i = 0; i < all_len; i++)
		wdata[i] = (val >> (8*i)) & 0xFF;

	cam_dbg("drime4_write [W] packet 0x%02x, val 0x%04x\n",
		packetID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_cmdwn(struct v4l2_subdev *sd,
	u8 packetID, u16 val)
{
	int all_len = 2;
	u8 wdata[all_len];
	int i;

	for (i = 0; i < all_len; i++)
		wdata[i] = (val >> (8*i)) & 0xFF;

#if 0
	cam_dbg("drime4_write [W] packet 0x%02x, val 0x%04x\n",
		packetID, val);
#endif

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_cmdl(struct v4l2_subdev *sd,
	u8 packetID, u32 val)
{
	int all_len = 4;
	u8 wdata[all_len];
	int i;

	for (i = 0; i < all_len; i++)
		wdata[i] = (val >> (8*i)) & 0xFF;

	cam_dbg("drime4_write [L] packet 0x%02x, val 0x%08x\n",
		packetID, val);

	return drime4_spi_write_packet(sd, packetID, all_len, &wdata[0], 0);
}

static int drime4_cmd_buf(struct v4l2_subdev *sd,
	u8 packetID, u32 len, u8 *buf, int log)
{
	u8 wdata[len];
	int i;

	for (i = 0; i < len; i++)
		wdata[i] = *(buf + i) & 0xFF;

	return drime4_spi_write_packet(sd, packetID, len, &wdata[0], log);
}

static irqreturn_t drime4_isp_ap_isr(int irq, void *dev_id)
{
	struct v4l2_subdev *sd = (struct v4l2_subdev *)dev_id;
	struct drime4_state *state = to_state(sd);

#if 1
	cam_trace("**************** interrupt ****************\n");
#endif
	state->isp.ap_issued = 1;
	wake_up(&state->isp.ap_wait);

	return IRQ_HANDLED;
}

static u32 drime4_wait_ap_interrupt(struct v4l2_subdev *sd,
	unsigned int timeout)
{
	struct drime4_state *state = to_state(sd);
#if 0
	cam_trace("E\n");
#endif

#if 0
	if (wait_event_interruptible_timeout(state->isp.wait,
		state->isp.issued == 1,
		msecs_to_jiffies(timeout)) == 0) {
		cam_err("timeout ~~~~~~~~~~~~~~~~~~~~~\n");
		return 0;
	}
#else
	if (wait_event_timeout(state->isp.ap_wait,
				state->isp.ap_issued == 1,
				msecs_to_jiffies(timeout)) == 0) {
		if (timeout != 2)
			cam_err("timeout ~~~~~~~~~~~~~~~~~~~~~~~\n");
		return 0;
	}
#endif

	state->isp.ap_issued = 0;

#if 0
	cam_trace("X\n");
#endif
	return 1;
}

static irqreturn_t drime4_isp_d4_isr(int irq, void *dev_id)
{
	struct v4l2_subdev *sd = (struct v4l2_subdev *)dev_id;
	struct drime4_state *state = to_state(sd);

	cam_trace("************ noti interrupt ************ pin:%d,ctrl:%d\n",
		drime4_spi_get_cmd_ctrl_pin(), state->noti_ctrl_status);
	state->isp.d4_issued = 1;
	wake_up(&state->isp.d4_wait);

	return IRQ_HANDLED;
}

static u32 drime4_wait_d4_interrupt(struct v4l2_subdev *sd,
	unsigned int timeout)
{
	struct drime4_state *state = to_state(sd);
#if 0
	cam_trace("E\n");
#endif

#if 0
	if (wait_event_timeout(state->isp.d4_wait,
				state->isp.d4_issued == 1,
				timeout) == 0) {
		cam_err("timeout ~~~~~~~~~~~~~~~~~~~~~~~\n");
		return 0;
	}
#else
	if (wait_event_timeout(state->isp.d4_wait,
				state->isp.d4_issued == 1,
				msecs_to_jiffies(timeout)) == 0) {
#if 0
		cam_err("d4_int timeout: continuous waiting(no error)\n");
#endif
		return 0;
	}
#endif

	state->isp.d4_issued = 0;

#if 0
	cam_trace("X\n");
#endif
	return 1;
}

static int drime4_write_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID)
{
	struct drime4_state *state = to_state(sd);
	u8 rdata[500] = {0,};
	int err = 0;
	int int_factor;
	int noti_lock = 0;

	/* ISP ready wait */
	if (ID == DRIME4_CMD_SCRIPT_BYPASS) {
		int_factor = drime4_wait_ap_interrupt(sd,
			DRIME4_ISP_TIMEOUT*36);
	} else if (ID == DRIME4_CMD_WRITE_NAND_FLASH
	|| ID == DRIME4_CMD_MAKE_CSV) {
		int_factor = drime4_wait_ap_interrupt(sd,
			DRIME4_ISP_TIMEOUT*12);
	} else {
		int_factor = drime4_wait_ap_interrupt(sd,
			DRIME4_ISP_TIMEOUT);
	}
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;

	if (int_factor) {
		cam_err("drime4_write HSW int_factor = %d, noti_lock= %d\n",
			int_factor, noti_lock);
		err = drime4_spi_read_res(&rdata[0], noti_lock, 0);
		CHECK_ERR(err);
		if (rdata[2] != (packetID+1)) {
			cam_err("drime4_write res failed: packet ID err - w:0x%02x r:0x%02x\n",
				packetID, rdata[2]);
			goto exit_wr_err;
		}
		if (((rdata[3] | ((rdata[4] << 8) & 0xFF00)) & 0xFFFF) != ID) {
			cam_err("drime4_write res failed: parameter ID err - w:0x%04x r:0x%02x%02x\n",
				ID, rdata[4], rdata[3]);
			goto exit_wr_err;
		}
		if (rdata[5] != 0x01) {
			if (rdata[5] != DRIME4_PACKET_RES_OK) {
				cam_err("drime4_write res failed: 0x%02x\n",
					rdata[5]);
				goto exit_wr_err;
			}
		} else if (rdata[6] != DRIME4_PACKET_RES_OK) {
			cam_err("drime4_write res failed: 0x%02x\n",
				rdata[6]);
			goto exit_wr_err;
		}
		if (rdata[5] != 0x00) {
			cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x, val 0x%02x\n",
				rdata[5], rdata[2], rdata[4], rdata[3],
				rdata[6]);
		} else {
			cam_dbg("drime4_write res [0] packet 0x%02x, ID 0x%02x%02x, val 0x%02x\n",
				rdata[2], rdata[4], rdata[3], rdata[5]);
		}
	} else {
		cam_err("drime4_write res timeout: packet ID 0x%02x param ID 0x%02x\n",
			packetID, ID);
		return -1;
	}

exit_wr_err:
	return err;
}

static int drime4_cmd_res(struct v4l2_subdev *sd,
	u8 packetID)
{
	struct drime4_state *state = to_state(sd);
	u8 rdata[20] = {0,};
	int err = 0;
	int int_factor;
	int noti_lock = 0;

	/* ISP ready wait */
	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&rdata[0], noti_lock, 0);
		CHECK_ERR(err);
		if (rdata[2] != (packetID+1)) {
			cam_err("drime4_write res failed: packet ID err - w:0x%02x r:0x%02x\n",
				packetID, rdata[2]);
			goto exit_wr_err;
		}
		if (rdata[3] != DRIME4_PACKET_RES_OK) {
			cam_err("drime4_write res failed: 0x%02x\n",
				rdata[3]);
			goto exit_wr_err;
		}
		cam_dbg("drime4_write res [] packet 0x%02x\n",
			rdata[2]);

	} else {
		cam_err("drime4_write res timeout: packet ID 0x%02x\n",
			packetID);
		return -1;
	}

exit_wr_err:
	return err;
}

static int drime4_write0_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID)
{
	int err = 0;

	err = drime4_write0(sd, packetID, ID);
	CHECK_ERR(err);

	err = drime4_write_res(sd, packetID, ID);
	CHECK_ERR(err);

	return err;
}

static int drime4_writeb_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u8 val)
{
	int err = 0;

	err = drime4_writeb(sd, packetID, ID, val);
	CHECK_ERR(err);

	err = drime4_write_res(sd, packetID, ID);
	CHECK_ERR(err);

	return err;
}

static int drime4_writew_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u16 val)
{
	int err = 0;

	err = drime4_writew(sd, packetID, ID, val);
	CHECK_ERR(err);

	err = drime4_write_res(sd, packetID, ID);
	CHECK_ERR(err);

	return err;
}

static int drime4_writel_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u32 val)
{
	int err = 0;

	err = drime4_writel(sd, packetID, ID, val);
	CHECK_ERR(err);

	err = drime4_write_res(sd, packetID, ID);
	CHECK_ERR(err);

	return err;
}

static int drime4_write_buf_res(struct v4l2_subdev *sd,
	u8 packetID, u16 ID, u32 len, u8 *buf, int log)
{
	int err = 0;

	err = drime4_write_buf(sd, packetID, ID, len, buf, 0);
	CHECK_ERR(err);

	err = drime4_write_res(sd, packetID, ID);
	CHECK_ERR(err);

	return err;
}

#if 0
static int drime4_cmdb_res(struct v4l2_subdev *sd,
	u8 packetID, u8 val)
{
	int err;

	err = drime4_cmdb(sd, packetID, val);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, packetID);
	CHECK_ERR(err);

	return err;
}

static int drime4_cmdw_res(struct v4l2_subdev *sd,
	u8 packetID, u16 val)
{
	int err;

	err = drime4_cmdw(sd, packetID, val);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, packetID);
	CHECK_ERR(err);

	return err;
}

static int drime4_cmdl_res(struct v4l2_subdev *sd,
	u8 packetID, u32 val)
{
	int err;

	err = drime4_cmdl(sd, packetID, val);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, packetID);
	CHECK_ERR(err);

	return err;
}
#endif

#if 0
void drime4_make_CRC_table(u32 *table, u32 id)
{
	u32 i, j, k;

	for (i = 0; i < 256; ++i) {
		k = i;
		for (j = 0; j < 8; ++j) {
			if (k & 1)
				k = (k >> 1) ^ id;
			else
				k >>= 1;
		}
		table[i] = k;
	}
}
#endif

static int drime4_set_mode(struct v4l2_subdev *sd, int isp_mode)
{
	struct drime4_state *state = to_state(sd);
	int err;

	if (state->isp_mode == isp_mode) {
		cam_dbg("same mode %d\n", state->isp_mode);
		return state->isp_mode;
	}

	cam_trace("E %d -> %d\n", state->isp_mode, isp_mode);

#if 0 /* Don't live control when Golf shot */
	if (state->PASM_mode == MODE_GOLF_SHOT
		&& isp_mode == DRIME4_LIVE_MOV_OFF
		&& state->sensor_mode != SENSOR_MOVIE) {
		cam_dbg("~ return %d\n", state->sensor_mode);
		return 0;
	}
	if (state->PASM_mode == MODE_GOLF_SHOT
		&& state->isp_mode == DRIME4_LIVEVIEW_MODE
		&& isp_mode == DRIME4_LIVEVIEW_MODE) {
		state->isp_mode = isp_mode;
		cam_dbg("~~ return %d\n", state->sensor_mode);
		return 0;
	}
	if (state->PASM_mode == MODE_GOLF_SHOT
		&& state->sensor_mode == SENSOR_MOVIE
		&& state->isp_mode != DRIME4_MOVIE_MODE) {
		state->isp_mode = isp_mode;
		cam_dbg("~~~ return %d\n", state->sensor_mode);
		return 0;
	}
#endif

	switch (isp_mode) {
	case DRIME4_LIVE_MOV_OFF:
		if (state->isp_mode == DRIME4_LIVEVIEW_MODE) {
			/* Liveview Stop */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_LIVEVIEW, 0x00);
			CHECK_ERR(err);
		} else if (state->isp_mode == DRIME4_MOVIE_MODE) {
			/* Stop Movie */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_MOVIE, 0x00);
			CHECK_ERR(err);
		}
		break;

	case DRIME4_LIVEVIEW_MODE:
		if (state->isp_mode == DRIME4_MOVIE_MODE) {
			/* Stop Movie */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_MOVIE, 0x00);
			CHECK_ERR(err);
		}

		/* Liveview Start */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_LIVEVIEW, 0x01);
		CHECK_ERR(err);
		break;

	case DRIME4_MOVIE_MODE:
		if (state->isp_mode == DRIME4_LIVEVIEW_MODE) {
			/* Liveview Stop */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_LIVEVIEW, 0x00);
			CHECK_ERR(err);
		}

		/* Start Movie */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_MOVIE, 0x01);
		CHECK_ERR(err);
		break;

	default:
		cam_warn("current mode is unknown, %d\n", state->isp_mode);
		return -1;/* -EINVAL; */
	}

	state->isp_mode = isp_mode;

	cam_trace("X\n");
	return state->isp_mode;
}

/*
 * v4l2_subdev_core_ops
 */
static int drime4_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(drime4_ctrls); i++) {
		if (qc->id == drime4_ctrls[i].id) {
			qc->maximum = drime4_ctrls[i].maximum;
			qc->minimum = drime4_ctrls[i].minimum;
			qc->step = drime4_ctrls[i].step;
			qc->default_value = drime4_ctrls[i].default_value;
			return 0;
		}
	}

	return -EINVAL;
}

static int drime4_set_antibanding(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	int antibanding_mode = 0;

	switch (val) {
	case ANTI_BANDING_OFF:
		antibanding_mode = DRIME4_FLICKER_NONE;
		break;
	case ANTI_BANDING_50HZ:
		antibanding_mode = DRIME4_FLICKER_AUTO_50HZ;
		break;
	case ANTI_BANDING_60HZ:
		antibanding_mode = DRIME4_FLICKER_AUTO_60HZ;
		break;
	case ANTI_BANDING_AUTO:
	default:
		antibanding_mode = DRIME4_FLICKER_AUTO;
		break;
	}

	return err;
}

static int drime4_set_ext_mic_status(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	cam_trace("E, val %d\n", val);

	sprintf(state->hotshoe_status, "%d", val);
	memcpy(sysfs_hotshoe_status, state->hotshoe_status, 1);
	sysfs_hotshoe_status[1] = ' ';
	switch_set_state(&android_hotshoe, val);

	cam_trace("X\n");
	return 0;
}

static int drime4_dump_fw(struct v4l2_subdev *sd)
{
	return 0;
}

static int drime4_save_ISP_log(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	int noti_lock = 0, int_factor = 0, err = 0;
	int log_length = 0;
	int buf1_count = 0, buf2_size = 0;
	char read_val[20] = {0,};
	char *log_buf = NULL;
	int next_addr = 0;
	int i = 0;

	struct file *fp;
	mm_segment_t old_fs;
	char filepath[256];

	cam_dbg("E\n");

	/*check a log*/
	err = drime4_cmdb(sd, DRIME4_PACKET_ISP_LOG_CHECK, 0x00);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], noti_lock, 1);
		if (read_val[2] != DRIME4_PACKET_ISP_LOG_CHECK_RES) {
			cam_err("(check)wrong response(0x%02x)", read_val[2]);
			goto out;
		}
		if (read_val[3] != 0x00) {
			cam_err("(check) Log is empty(0x%02x)\n", read_val[3]);

			/*change mode*/
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			/*write '0' to get_isp_log.dat*/
			sprintf(filepath, DRIME4_ISP_LOG_CHECK_PATH);

			fp = filp_open(filepath,
				O_WRONLY|O_CREAT|O_TRUNC,
				S_IRUGO|S_IWUGO|S_IXUSR);
			if (IS_ERR(fp)) {
				cam_err("failed to open check file, err %ld\n",
					PTR_ERR(fp));
				err = -ENOENT;
				goto file_out;
			}
			vfs_write(fp, "0", 1, &fp->f_pos);

			goto file_out;
		}
	} else {
		err = -EIO;
		goto out;
	}

	/*request to start sending a log data from ISP*/
	err = drime4_cmdb(sd, DRIME4_PACKET_ISP_LOG_SAVE, 0x00);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		log_length =
			drime4_spi_read_length(DRIME4_ISP_LOG_LENGTH_BUF_SIZE)
			-(DRIME4_ISP_LOG_LENGTH_BUF_SIZE
			+ DRIME4_ISP_LOG_END_PACKET_SIZE);

		if (log_length <= 0) {
			cam_err("log_data_length (%d)\n", log_length);
			goto out;
		}
	} else {
		err = -EIO;
		goto out;
	}

	/*calculate a save count*/
	if (log_length >= (DRIME4_FW_BUF_SIZE/8))
		buf1_count = log_length / (DRIME4_FW_BUF_SIZE/8);
	buf2_size = log_length % (DRIME4_FW_BUF_SIZE/8);
	cam_dbg("total[%d] = 60k x count[%d] + extra[%d]\n",
		log_length, buf1_count, buf2_size);

	log_buf = vmalloc(log_length+1);
	if (buf1_count) {
		for (i = 0; i < buf1_count; i++) {
			err = drime4_spi_read_burst(&data_memory[0],
				(DRIME4_FW_BUF_SIZE/8));
			CHECK_ERR(err);

			memcpy(log_buf + next_addr,
				&data_memory[0], (DRIME4_FW_BUF_SIZE/8));
			next_addr += (DRIME4_FW_BUF_SIZE/8);
		}
	}

	/*get a remain log data*/
	if (buf2_size) {
		err = drime4_spi_read_burst(&data_memory[0], buf2_size);
		CHECK_ERR(err);

		memcpy(log_buf + next_addr, &data_memory[0], buf2_size);
		next_addr += buf2_size;
	}

	/*end of file*/
	memcpy(log_buf + next_addr, " ", 1);
#if 0
	/*check the end of a log data*/
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	err = drime4_spi_read_res(&read_val[0], noti_lock, 0);
	CHECK_ERR(err);
	if (read_val[2] != DRIME4_PACKET_ISP_LOG_SAVE_RES) {
		cam_err("(save) worng response(0x%02x)\n", read_val[2]);
		goto out;
	}
#endif

	/*change mode*/
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sprintf(filepath, DRIME4_ISP_LOG_FILE_PATH);

	fp = filp_open(filepath,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		cam_err("failed to open, err %ld\n", PTR_ERR(fp));
		err = -ENOENT;
		goto file_out;
	}

	vfs_write(fp, log_buf, log_length+1, &fp->f_pos);

	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	/*write '0' to get_isp_log.dat*/
	sprintf(filepath, DRIME4_ISP_LOG_CHECK_PATH);

	fp = filp_open(filepath,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		cam_err("failed to open check file, err %ld\n", PTR_ERR(fp));
		err = -ENOENT;
		goto file_out;
	}
	vfs_write(fp, "0", 1, &fp->f_pos);

file_out:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);

out:
	if (log_buf != NULL)
		vfree(log_buf);

	if (!gpio_get_value(GPIO_ISP_INT))
		drime4_spi_set_cmd_ctrl_pin(1);

	cam_dbg("X\n");
	return err;
}

static int drime4_get_sensor_fw_version(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int int_factor;
	u8 read_val[20];
	int noti_lock = 0;

	drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_FW_VERSION);

	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;

	/* ISP FW version read */
	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], noti_lock, 0);
		if (err < 0) {
			sprintf(state->sensor_fw, "NULL");
			state->isp.bad_fw = 1;
			cam_dbg("sensor fw version read fail");
		} else {
			cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x\n",
				read_val[5], read_val[2], read_val[4],
				read_val[3]);
			memcpy(state->sensor_fw, &read_val[6],
				DRIME4_FW_VER_LEN);
			state->isp.bad_fw = 0;
		}
	} else {
		sprintf(state->sensor_fw, "NULL");
		state->isp.bad_fw = 1;
		cam_dbg("sensor fw version read busy fail");
		err = -1;
	}

	memcpy(sysfs_sensor_fw, state->sensor_fw,
		sizeof(state->sensor_fw));

	cam_dbg("Sensor_version = %s\n", state->sensor_fw);

	return err;
}

static int drime4_open_firmware_file(struct v4l2_subdev *sd,
	const char *filename, u8 *buf, u16 offset, u16 size) {
	struct file *fp;
	int err = 0;
	mm_segment_t old_fs;
	long nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		err = -ENOENT;
		goto out;
	} else {
		cam_dbg("%s is opened\n", filename);
	}

	err = vfs_llseek(fp, offset, SEEK_SET);
	if (err < 0) {
		cam_warn("failed to fseek, %d\n", err);
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf, size, &fp->f_pos);

	if (nread != size) {
		cam_err("failed to read firmware file, %ld Bytes\n", nread);
		err = -EIO;
		goto out;
	}
out:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);

	return err;
}

static int drime4_get_phone_fw_version(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	const struct firmware *fw = {0, };
	char fw_path_in_data[25] = {0,};
	u8 *buf = NULL;
	int err = 0;
	int retVal = 0;
	int fw_requested = 1;

	sprintf(fw_path_in_data, DRIME4_FW_PATH);

	buf = vmalloc(DRIME4_FW_VER_LEN+1);
	if (!buf) {
		cam_err("failed to allocate memory\n");
		err = -ENOMEM;
		goto out;
	}

	retVal = drime4_open_firmware_file(sd,
		DRIME4_FW_PATH,
		buf,
		DRIME4_FW_VER_FILE_CUR,
		DRIME4_FW_VER_LEN);
	if (retVal >= 0) {
		camfw_info[DRIME4_SD_CARD].opened = 1;
		memcpy(camfw_info[DRIME4_SD_CARD].ver,
			buf,
			DRIME4_FW_VER_LEN);
		camfw_info[DRIME4_SD_CARD]
			.ver[DRIME4_FW_VER_LEN+1] = ' ';
		state->fw_index = DRIME4_SD_CARD;
		fw_requested = 0;
		mmc_fw = 1;
	} else {
		cam_dbg("failed to open firmware file in sdcard. try to open from mmcblk0p17.\n");
		sprintf(fw_path_in_data, DRIME4_FW_MMC_PATH);
		retVal = drime4_open_firmware_file(sd,
				DRIME4_FW_MMC_PATH,
				buf,
				DRIME4_FW_VER_FILE_CUR,
				DRIME4_FW_VER_LEN);
		if (retVal >= 0) {
			camfw_info[DRIME4_SD_CARD].opened = 1;
			memcpy(camfw_info[DRIME4_SD_CARD].ver,
					buf,
					DRIME4_FW_VER_LEN);
			camfw_info[DRIME4_SD_CARD]
				.ver[DRIME4_FW_VER_LEN+1] = ' ';
			state->fw_index = DRIME4_SD_CARD;
			fw_requested = 0;
			mmc_fw = 2;
		} else {
			cam_dbg("failed to open firmware file in sdcard or mmcblk0p17\n");
			mmc_fw = 0;
		}
	}

	if (fw_requested) {
		sprintf(camfw_info[DRIME4_IN_DATA].ver, "NULL");
		state->fw_index = DRIME4_IN_DATA;
	}

	if (!copied_fw_binary) {
		memcpy(state->phone_fw,
			camfw_info[state->fw_index].ver,
			DRIME4_FW_VER_LEN);
		state->phone_fw[DRIME4_FW_VER_LEN+1] = ' ';
	}

	memcpy(sysfs_phone_fw,
		state->phone_fw,
		sizeof(state->phone_fw));
	cam_dbg("Phone_version = %s(index=%d)\n",
		state->phone_fw, state->fw_index);

out:
	if (buf != NULL)
		vfree(buf);

	if (fw_requested)
		release_firmware(fw);

	return err;
}

static int drime4_check_fw(struct v4l2_subdev *sd)
{
	int err;
	cam_dbg("E\n");

	err = drime4_get_phone_fw_version(sd);
	CHECK_ERR(err);

	err = drime4_get_sensor_fw_version(sd);
	CHECK_ERR(err);

	cam_info("phone ver = %s, sensor_ver = %s\n",
			sysfs_phone_fw, sysfs_sensor_fw);

	cam_dbg("X\n");

	return 0;
}

static int drime4_set_sensor_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case SENSOR_CAMERA:
		break;

	case SENSOR_MOVIE:
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = SENSOR_CAMERA;
		goto retry;
	}
	state->sensor_mode = val;

	cam_trace("X\n");
	return 0;
}

static int drime4_set_preview_size(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	const struct drime4_frmsizeenum **frmsize;
	int err;
	int i, num_entries;

	state->preview_width = (val >> 16) & 0x0FFFF;
	state->preview_height = val & 0x0FFFF;

	cam_dbg("E, preview size %d x %d\n",
		state->preview_width, state->preview_height);

	frmsize = &state->preview;
	*frmsize = NULL;

	num_entries = ARRAY_SIZE(preview_frmsizes);
	for (i = 0; i < num_entries; i++) {
		if (state->preview_width == preview_frmsizes[i].width &&
			state->preview_height == preview_frmsizes[i].height) {
			*frmsize = &preview_frmsizes[i];
			break;
		}
	}

	if (*frmsize == NULL) {
		cam_warn("invalid preview size %dx%d\n",
			state->preview_width, state->preview_height);
		*frmsize = drime4_get_frmsize(preview_frmsizes,
					num_entries, DRIME4_PREVIEW_720P);
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param) {
		err = drime4_add_boot_b_param(sd, DRIME4_SET_LIVEVIEW_SIZE,
			1, state->preview->reg_val);
		CHECK_ERR(err);
	}
#endif

	cam_trace("X, 0x%02x\n", state->preview->reg_val);

	return 0;
}

static int drime4_set_photo_size(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	const struct drime4_frmsizeenum **frmsize;
	int err;
	int i, num_entries;

	state->jpeg_width = (val >> 16) & 0x0FFFF;
	state->jpeg_height = val & 0x0FFFF;

	cam_dbg("E, jpeg size %d x %d\n",
		state->jpeg_width, state->jpeg_height);

	frmsize = &state->photo;
	*frmsize = NULL;

	num_entries = ARRAY_SIZE(photo_frmsizes);
	for (i = 0; i < num_entries; i++) {
		if (state->jpeg_width == photo_frmsizes[i].width &&
			state->jpeg_height == photo_frmsizes[i].height) {
			*frmsize = &photo_frmsizes[i];
			break;
		}
	}

	if (state->PASM_mode == MODE_3D && state->photo->reg_val == 0x13) {
		state->photo = drime4_get_frmsize(photo_frmsizes,
			num_entries, DRIME4_CAPTURE_2p1MW_3D);
	}

	if (*frmsize == NULL) {
		cam_warn("invalid frame size %dx%d\n",
			state->jpeg_width, state->jpeg_height);
		*frmsize = drime4_get_frmsize(photo_frmsizes,
			num_entries, DRIME4_CAPTURE_16p9MW);
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_CAPTURE_SIZE,
			1, state->photo->reg_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_CAPTURE_SIZE, state->photo->reg_val);
	CHECK_ERR(err);

	cam_trace("X, 0x%02x\n", state->photo->reg_val);
	return 0;
}

static int drime4_set_movie_size(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	const struct drime4_frmsizeenum **frmsize;
	int err;
	int i, num_entries;

	state->movie_width = (val >> 16) & 0x0FFFF;
	state->movie_height = val & 0x0FFFF;

	cam_dbg("E, moive size %d x %d\n",
		state->movie_width, state->movie_height);

	frmsize = &state->movie;
	*frmsize = NULL;

	num_entries = ARRAY_SIZE(movie_frmsizes);
	for (i = 0; i < num_entries; i++) {
		if (state->movie_width == movie_frmsizes[i].width &&
			state->movie_height == movie_frmsizes[i].height) {
			*frmsize = &movie_frmsizes[i];
			break;
		}
	}

	if (*frmsize == NULL) {
		cam_warn("invalid frame size %dx%d\n",
			state->movie_width, state->movie_height);
		*frmsize = drime4_get_frmsize(movie_frmsizes,
			num_entries, DRIME4_MOVIE_FHD);
	}

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_MOVIE_SIZE, state->movie->reg_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_flash_step(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;

	if (val < 0 || val > 8) {
		cam_dbg("invalid value %d\n", val);
		return 0;
	}

	cam_dbg("E, value %d\n", val);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_FLASH_STEP,
			1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_FLASH_STEP, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_flash_popup(struct v4l2_subdev *sd, int val)
{
	int err;

	cam_dbg("E, value %d\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_STROBO_POPUP, 0x01);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_flash(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err, flash_mode;
	cam_dbg("E, value %d\n", val);

	if (!state->samsung_app && val != FLASH_MODE_OFF) {
		err = drime4_set_flash_popup(sd, 1);	/* for third party */
		if (err)
			cam_err("failed to flash popup %d\n", err);
	}

retry:
	switch (val) {
	case FLASH_MODE_OFF:
		flash_mode = 0x00;
		break;

	case FLASH_MODE_SMART:
		flash_mode = 0x01;
		break;

	case FLASH_MODE_AUTO:
		flash_mode = 0x02;
		break;

	case FLASH_MODE_RED_EYE:	/* AUTO RED EYE */
		flash_mode = 0x03;
		break;

	case FLASH_MODE_ON:
	case FLASH_MODE_FILL_IN:
		flash_mode = 0x04;
		break;

	case FLASH_MODE_FILL_IN_RED_EYE:
		flash_mode = 0x05;
		break;

	case FLASH_MODE_1st_CURTAIN:
		flash_mode = 0x06;
		break;

	case FLASH_MODE_2nd_CURTAIN:
		flash_mode = 0x07;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = FLASH_MODE_OFF;
		goto retry;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_FLASH_MODE,
			1, flash_mode);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_FLASH_MODE, flash_mode);
	CHECK_ERR(err);

	state->flash_mode = val;

	cam_trace("X\n");
	return 0;
}

static int drime4_set_iso(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int err;
	cam_dbg("E, value %d\n",
		(ctrl->value > 0) ? (ctrl->value - 1) : ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		ctrl->value = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_ISO, 1,
			(ctrl->value > 0) ? ctrl->value-1 : 0);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_ISO, (ctrl->value > 0) ? ctrl->value-1 : 0);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_iso_step(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_ISO_STEP,
			1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_ISO_STEP, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_auto_iso_range(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int err;
	cam_dbg("E, value %d\n",
		(ctrl->value > 1) ? (ctrl->value - 1) : ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		ctrl->value = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_ISO_AUTO_MAX, 1,
			(ctrl->value > 1) ? ctrl->value-1 : qc.default_value);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_ISO_AUTO_MAX,
			(ctrl->value > 1) ? ctrl->value-1 : qc.default_value);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_metering(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case METERING_CENTER:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_SET_METERING,
				1, 0x01);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_METERING, 0x01);
		CHECK_ERR(err);
		break;

	case METERING_SPOT:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_SET_METERING,
				1, 0x02);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_METERING, 0x02);
		CHECK_ERR(err);
		break;

	case METERING_MATRIX:	/* MULTI */
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_SET_METERING,
				1, 0x00);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_METERING, 0x00);
		CHECK_ERR(err);
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = METERING_CENTER;
		goto retry;
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_exposure(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value;
	int set_val = val;
	int err;
	cam_dbg("E, value %d\n", ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		val = qc.default_value;
	}

	if (qc.minimum < 0)
		set_val += qc.maximum;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_EVC, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_EVC, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_contrast(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value;
	int set_val = val;
	int err;
	cam_dbg("E, value %d\n", ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

#if 0
	if (qc.minimum < 0)
		val += qc.minimum;
#else
	if (qc.minimum < 0)
		set_val += qc.maximum;
#endif

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		if (qc.minimum < 0)
			set_val = qc.default_value + qc.maximum;
		else
			set_val = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_CONTRAST, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_CONTRAST, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_sharpness(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value;
	int set_val = val;
	int err;
	cam_dbg("E, value %d\n", ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

#if 0
	if (qc.minimum < 0)
		val += qc.minimum;
#else
	if (qc.minimum < 0)
		set_val += qc.maximum;
#endif

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalid value, %d\n",  val);
		if (qc.minimum < 0)
			set_val = qc.default_value + qc.maximum;
		else
			set_val = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_SHARPNESS, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SHARPNESS, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");

	return 0;
}

static int drime4_set_saturation(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value;
	int set_val = val;
	int err;
	cam_dbg("E, value %d\n", ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

#if 0
	if (qc.minimum < 0)
		val += qc.minimum;
#else
	if (qc.minimum < 0)
		set_val += qc.maximum;
#endif

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		if (qc.minimum < 0)
			set_val = qc.default_value + qc.maximum;
		else
			set_val = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_SATURATION, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SATURATION, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");

	return 0;
}

static int drime4_set_color(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value;
	int color_val[] = { 0xFFFF, 0,
		18, 36, 54, 72, 90,
		108, 126, 144, 162, 180,
		198, 216, 234, 252, 270,
		288, 306, 324, 342, 360 };
	int err;
	cam_dbg("E, value %d\n", ctrl->value);

	qc.id = ctrl->id;
	drime4_queryctrl(sd, &qc);

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		cam_warn("invalid value, %d\n", ctrl->value);
		val = qc.default_value;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_COLOR_ADJ,
			2, color_val[val]);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_COLOR_ADJ, color_val[val]);
	CHECK_ERR(err);

	cam_trace("X\n");

	return 0;
}

static int drime4_set_wb_K(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val);

	if (val < 0x19 || val > 0x64) {
		cam_warn("invalid value, %d\n", val);
		return 0;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_WB_KELVIN, 1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_WB_KELVIN, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_wb_GBAM(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int BA_val = (val >> 8) & 0x0F, GM_val = val & 0x0F;
	cam_dbg("E, value %d\n", val);

	if (BA_val < 0 || BA_val > 0x0E || GM_val < 0 || GM_val > 0x0E) {
		cam_warn("invalid value, %d\n", val);
		return 0;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_WB_GBAM, 2, val);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_WB_GBAM, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_whitebalance(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err, wb;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case WHITE_BALANCE_AUTO:
		wb = 0x00;
		break;

	case WHITE_BALANCE_SUNNY:
		wb = 0x01;
		break;

	case WHITE_BALANCE_CLOUDY:
		wb = 0x02;
		break;

	case WHITE_BALANCE_FLUORESCENT_H:	/* FLUORESCENT_W */
		wb = 0x03;
		break;

	case WHITE_BALANCE_FLUORESCENT_L:	/* FLUORESCENT_N */
		wb = 0x04;
		break;

	case WHITE_BALANCE_FLUORESCENT:	/* FLUORESCENT_D */
		wb = 0x05;
		break;

	case WHITE_BALANCE_TUNGSTEN:
		wb = 0x06;
		break;

	case WHITE_BALANCE_FLASH:
		wb = 0x07;
		break;

	case WHITE_BALANCE_CUSTOM:
		wb = 0x08;
		break;

	case WHITE_BALANCE_K:
		wb = 0x09;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = WHITE_BALANCE_AUTO;
		goto retry;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_WB, 1, wb);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_WB, wb);
	CHECK_ERR(err);

	state->wb_mode = val;

	cam_trace("X\n");
	return 0;
}

static int drime4_set_custom_wb(struct v4l2_subdev *sd, int val)
{
	int err;

	cam_dbg("E, value %d\n", val);

	if (val) {
		/* Capture count */
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_CAP_CNT, 1);
		CHECK_ERR(err);

		/* Capture start */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_CAPTURE, DRIME4_CAP_MODE_CUST_WB);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_scene_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case SCENE_MODE_NONE:
		break;

	case SCENE_MODE_PORTRAIT:
		break;

	case SCENE_MODE_LANDSCAPE:
		break;

	case SCENE_MODE_SPORTS:
		break;

	case SCENE_MODE_PARTY_INDOOR:
		break;

	case SCENE_MODE_BEACH_SNOW:
		break;

	case SCENE_MODE_SUNSET:
		break;

	case SCENE_MODE_DUSK_DAWN:
		break;

	case SCENE_MODE_FALL_COLOR:
		break;

	case SCENE_MODE_NIGHTSHOT:
		break;

	case SCENE_MODE_BACK_LIGHT:
		break;

	case SCENE_MODE_FIREWORKS:
		break;

	case SCENE_MODE_TEXT:
		break;

	case SCENE_MODE_CANDLE_LIGHT:
		break;

	case SCENE_MODE_LOW_LIGHT:
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	state->scene_mode = val;
	cam_trace("X\n");
	return 0;
}

static int drime4_set_effect(struct v4l2_subdev *sd, int val)
{
	cam_dbg("E, value %d\n", val);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_wdr(struct v4l2_subdev *sd, int val)
{
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case WDR_OFF:
		break;

	case WDR_ON:
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = WDR_OFF;
		goto retry;
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_antishake(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 2) {
		cam_warn("invalid value, %d\n", val);
		return 0;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_OIS, 1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_OIS, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_stop_af_lens(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);

	cam_trace("X\n");
	return err;
}

static int drime4_set_3A_Lock(struct v4l2_subdev *sd, int enable)
{
	int err = 0;
	int int_factor = 0;
	u8 read_val[100] = {0,};
	cam_dbg("E, value %d\n", enable);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_3A_LOCK, enable);

	CHECK_ERR(err);

	cam_trace("X\n");

	return err;
}

static int drime4_set_af(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_info("%s, mode %#x\n", val ? "start" : "stop", state->focus.mode);

	if (val) {
		if (state->focus.touch) /* TOUCH AF */
			err = drime4_set_touch_auto_focus(sd);
		else /* S1 PUSH */
			err = drime4_set_3A_Lock(sd, 0x01);
	} else {
		err = drime4_set_3A_Lock(sd, 0x00);

		if (state->focus.touch) {
			err = drime4_write0_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_MOVE_TO_CENTER_AF);
			CHECK_ERR(err);
		}
	}

	cam_trace("X\n");
	return err;
}

#if 0
static int drime4_get_af_result(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);

	ctrl->value = state->focus.status;

	return ctrl->value;
}

static int drime4_set_af_area(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val);

	if (state->facedetect_mode == FACE_DETECTION_NORMAL
		|| state->facedetect_mode == FACE_DETECTION_ON) {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AF_AREA, DRIME4_AF_AREA_FD);
		CHECK_ERR(err);
		cam_trace("X: FD on AF Area\n");
		return 0;
	}

retry:
	switch (val) {
	case DRIME4_AF_AREA_SEL:
	case DRIME4_AF_AREA_MULTI:
	case DRIME4_AF_AREA_FD:
	case DRIME4_AF_AREA_SELF:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AF_AREA, val);
		CHECK_ERR(err);
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = DRIME4_AF_AREA_SEL;
		goto retry;
	}

	state->af_area = val;

	cam_trace("X: %d\n", state->af_area);
	return 0;

}
#endif

static int drime4_set_af_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0, af_mode = 0;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_SINGLE:
		af_mode = 0x00;
		break;

	case FOCUS_MODE_CONTINUOUS:
		af_mode = 0x01;
		break;

	case FOCUS_MODE_MANUAL:
		af_mode = 0x02;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = FOCUS_MODE_SINGLE;
		goto retry;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_AF_MODE,
			1, af_mode);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_AF_MODE, af_mode);
	CHECK_ERR(err);

	state->focus.mode = val;

	cam_trace("X\n");
	return err;
}

static int drime4_set_focus_area_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err, af_area;

#if 0
retry:
#endif
	switch (val) {
	case V4L2_FOCUS_AREA_CENTER:
		af_area = 0x00;
		break;

	case V4L2_FOCUS_AREA_MULTI:
		af_area = 0x01;
		break;

	case V4L2_FOCUS_AREA_FACE_DETECTION:
		af_area = 0x02;
		break;

	case V4L2_FOCUS_AREA_TRACKING:
		af_area = 0x03;
		break;

	case V4L2_FOCUS_AREA_ONE_TOUCH_SHOT:
		af_area = 0x04;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
#if 0
		val = V4L2_FOCUS_AREA_CENTER;
		goto retry;
#else
		goto fail;
#endif
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_AF_AREA,
			1, af_area);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_AF_AREA, af_area);
	CHECK_ERR(err);

	state->af_area = val;

#if 0	/* Change FD : Not use parameter */
	if (val == V4L2_FOCUS_AREA_FACE_DETECTION) {
		err = drime4_set_face_detection(sd,
			FACE_DETECTION_NORMAL);
		CHECK_ERR(err);
	} else {
		err = drime4_set_face_detection(sd,
			FACE_DETECTION_OFF);
		CHECK_ERR(err);
	}
#endif

fail:
	cam_trace("X val : %d\n", val);
	return 0;
}

static int drime4_set_mf_assist(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0, set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 0x01;
	else
		set_val = 0x00;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_MF_ASSIST,
			1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_MF_ASSIST, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_focus_peaking_level(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0x0000;
	cam_dbg("E, value %d\n", val);

	if (0 > val || 3 < val) {
		cam_warn("invalid value, %d\n", val);
		val = 0;
	}

	state->focus_peaking_level = val;
	set_val = (val << 8 | state->focus_peaking_color);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_FOCUS_PEAKING,
			2, set_val);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_FOCUS_PEAKING, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_focus_peaking_color(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0x0000;
	cam_dbg("E, value %d\n", val);

	if (1 > val || 3 < val) {
		cam_warn("invalid value, %d\n", val);
		val = 1;
	}

	state->focus_peaking_color = val;
	set_val = (val | state->focus_peaking_level << 8);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_FOCUS_PEAKING,
			2, set_val);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_FOCUS_PEAKING, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_link_AE_to_AF_point(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_LINK_AE_TO_AF_POINT, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_LINK_AE_TO_AF_POINT, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_object_tracking(struct v4l2_subdev *sd, int enable)
{
	struct drime4_state *state = to_state(sd);
	int err = 0, val = 0;
	int int_factor = 0;
	u8 read_val[100];
	u8 buf[4];
	unsigned int pos_x = 0, pos_y = 0;

	cam_dbg("E, value %d\n", enable);

	pos_x = state->focus.pos_x;
	pos_y = state->focus.pos_y;
	cam_dbg("pos_x, pos_y: %d, %d\n", pos_x, pos_y);

	if (enable) {
		buf[0] = pos_x & 0xFF;
		buf[1] = (pos_x >> 8) & 0xFF;
		buf[2] = pos_y & 0xFF;
		buf[3] = (pos_y >> 8) & 0xFF;

		val |= ((val & 0xFF000000) | buf[3]) << 24;
		val |= ((val & 0xFF0000) | buf[2]) << 16;
		val |= ((val & 0xFF00) | buf[1]) << 8;
		val |= (val & 0xFF) | buf[0];

		err = drime4_writel_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_TRACKING_AF, val);
	} else {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_START_STOP_TRACKING_AF, 0x00);
	}

	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_MF_enlarge(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_dbg("E, value%d\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_MF_ENLARGE, val);
	CHECK_ERR(err);

	return err;
}

static int drime4_set_touch_auto_focus(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	int err = 0, val = 0;
	int int_factor = 0;
	u8 read_val[100];
	u8 buf[4];
	unsigned int pos_x = 0, pos_y = 0;

	cam_dbg("E\n");

	pos_x = state->focus.pos_x;
	pos_y = state->focus.pos_y;
	cam_dbg("pos_x, pos_y: %d, %d\n", pos_x, pos_y);

	buf[0] = pos_x & 0xFF;
	buf[1] = (pos_x >> 8) & 0xFF;
	buf[2] = pos_y & 0xFF;
	buf[3] = (pos_y >> 8) & 0xFF;

	val |= ((val & 0xFF000000) | buf[3]) << 24;
	val |= ((val & 0xFF0000) | buf[2]) << 16;
	val |= ((val & 0xFF00) | buf[1]) << 8;
	val |= (val & 0xFF) | buf[0];

	err = drime4_writel_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_AF_WIDTH, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_zoom(struct v4l2_subdev *sd, int value)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", value);

retry:
	if (value < 0 || value > 4) {
		cam_warn("invalid value, %d\n", value);
		value = 0;
		goto retry;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_i_ZOOM, 1, value);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_i_ZOOM, value);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_jpeg_quality(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int val = ctrl->value, err;
	u8 ratio;
	cam_dbg("E, value %d\n", val);

	if (val <= 65)		/* Normal */
		ratio = 0x02;
	else if (val <= 95)	/* Fine */
		ratio = 0x01;
	else if (val <= 100)	/* Superfine */
		ratio = 0x00;
	else if (val <= 200)	/* Raw */
		ratio = 0x20;
	else if (val <= 300)	/* Superfine + Raw */
		ratio = 0x10;
	else
		ratio = 0x00;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_QUALITY, 1, ratio);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_QUALITY, ratio);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_aeawb_lock_unlock(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int ae_lock = val & 0x1;
	int awb_lock = (val & 0x2) >> 1;
	int ae_lock_changed =
		~(ae_lock & state->ae_lock) & (ae_lock | state->ae_lock);
	int awb_lock_changed =
		~(awb_lock & state->awb_lock) & (awb_lock | state->awb_lock);

	if (ae_lock_changed) {
		cam_dbg("ae lock - %s\n", ae_lock ? "true" : "false");

		state->ae_lock = ae_lock;
	}
	if (awb_lock_changed &&
		state->wb_mode == WHITE_BALANCE_AUTO) {
		cam_dbg("awb lock - %s\n", awb_lock ? "true" : "false");

		state->awb_lock = awb_lock;
	}

	return 0;
}

static int drime4_set_multi_exposure_max_cnt(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_info("E, cnt - %d\n", val);

	/* Multi Exposure Count */
	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_MULTI_EXPOSURE_CNT, val);
	CHECK_ERR(err);

	cam_info("x\n");
	return 0;
}

static int drime4_set_debug_raw_ssif_capture(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_info("E, ssif sensor - %d\n", val);

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_DEBUG_SSIF_RAW_CAPTURE, val);
	CHECK_ERR(err);

	cam_info("x\n");
	return 0;
}

static int drime4_set_debug_raw_pp_capture(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_info("E, pp sensor - %d\n", val);


	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_DEBUG_PP_RAW_CAPTURE, val);
	CHECK_ERR(err);

	cam_info("x\n");
	return 0;
}

static int drime4_set_fast_capture(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_info("E, %s mode - %d cnt - %d\n",
		(val ? "start" : "stop"),
		state->drive_capture_mode, state->capture_cnt);

	if (val) {
#if 0 /* use 2 lane */
		err = drime4_set_mode(sd, DRIME4_LIVE_MOV_OFF);
		CHECK_ERR(err);
#endif

		if (val == 2) {

			err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
			CHECK_ERR(err);

			err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
			CHECK_ERR(err);

			/* YUV capture on factory */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_PRODUCTION_YUV_CAPTURE, 0x00);
			CHECK_ERR(err);
		} else if (val == 3) {
			state->capturingGolfShot = true;
#if 0
			/* Capture start */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE, DRIME4_CAP_MODE_SMART);
			CHECK_ERR(err);
#endif
		} else {
			/* Capture count */
			if (state->capture_mode != RUNNING_MODE_SINGLE) {
				err = drime4_writew_res(sd,
					DRIME4_PACKET_DSP_SET,
					DRIME4_SET_CAP_CNT, state->capture_cnt);
				CHECK_ERR(err);
			}

			/* Capture start */
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE, state->drive_capture_mode);
			CHECK_ERR(err);
		}
	} else {
		/* Capture stop */
		if (state->PASM_mode != MODE_GOLF_SHOT) {
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE_STOP, 0x00);
			CHECK_ERR(err);
		}
	}

	cam_info("x\n");
	return 0;
}

static int drime4_burst_onoff(struct v4l2_subdev *sd, int onoff)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_info("E\n");

	if (onoff == V4L2_INT_STATE_BURST_START) {
#if 0
		err = drime4_set_mode(sd, DRIME4_LIVE_MOV_OFF);
		CHECK_ERR(err);
#endif

		/* Capture count */
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_CAP_CNT, state->capture_cnt);
		CHECK_ERR(err);

		/* Capture start */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE, state->drive_capture_mode);
		CHECK_ERR(err);
	} else if (onoff == V4L2_INT_STATE_BURST_STOP) {
		/* Capture stop */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE_STOP, 0x00);
		CHECK_ERR(err);
	}

	cam_info("x\n");
	return err;
}

static int drime4_start_capture(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_info("E - %d frame\n", val);

	/* Image Data Transfer Request */
	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_TRANSFER, 0x01);
	CHECK_ERR(err);

	cam_info("x\n");
	return 0;
}

static int drime4_stop_capture(struct v4l2_subdev *sd, int off)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_info("E\n");

	/* Capture stop */
	if (state->PASM_mode == MODE_GOLF_SHOT) {
		if (state->capturingGolfShot == true)
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE_STOP, off);
			CHECK_ERR(err);
			state->capturingGolfShot = false;
	} else {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_CAPTURE_STOP, off);
		CHECK_ERR(err);
	}

	cam_info("x\n");

	return err;
}

static int drime4_set_exif_system_orient(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_orient = 0x01;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case SYSTEM_ORIENT_0_DEGREE:
		set_orient = 0x01;
		break;

	case SYSTEM_ORIENT_90_DEGREE:
		set_orient = 0x06;
		break;

	case SYSTEM_ORIENT_180_DEGREE:
		set_orient = 0x03;
		break;

	case SYSTEM_ORIENT_270_DEGREE:
		set_orient = 0x08;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = SYSTEM_ORIENT_0_DEGREE;
		goto retry;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_EXIF_SYSTEM_ORIENT,
			1, set_orient);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_EXIF_SYSTEM_ORIENT, set_orient);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_exif_time_model(struct v4l2_subdev *sd,
	char *buf, u8 size)
{
	int err = 0, i = 0;

	cam_dbg("E, size = %d", size);
#if 0
	for (i; i < size; i++)
		cam_dbg("@@@ buf[%d] = 0x%02x", i, buf[i]);
#endif
	err = drime4_write_buf_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_EXIF_SYSTEM_TIME, 8, &buf[0], 0);
	CHECK_ERR(err);

	if (8 < size) {
		err = drime4_write_buf_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_EXIF_MODLE_NAME, 9, &buf[8], 0);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return err;
}

static int drime4_set_weather_info(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;

	cam_dbg("E, value %d\n", val);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_WEATHER_INFO,
			1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_WEATHER_INFO, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_exif_gps(struct v4l2_subdev *sd, char *buf, u8 size)
{
	int err = 0, i = 0;

	cam_dbg("E, size = %d", size);
#if 0
	for (i; i < size; i++)
		cam_dbg("@@@ buf[%d] = 0x%02x", i, buf[i]);
#endif
	err = drime4_write_buf_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_EXIF_GPS_INFO, size, &buf[0], 0);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

/* doesn't support a fade mode yet */
static int drime4_set_movie_fader_mode(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value 0x%02x\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_MOVIE_FADER, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_movie_fadeout(struct v4l2_subdev *sd, int val)
{
	int err;
	u8 rdata[10];
	int int_factor;

	cam_dbg("E, value 0x%02x\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_MOVIE_FADEOUT, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_system_reset_cwb(struct v4l2_subdev *sd, int val)
{
	int err;
	u8 rdata[10];
	int int_factor;

	cam_dbg("E, value 0x%02x\n", val);

	err = drime4_write0_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_SYSTEM_RESET_CWB);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_framing_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_FRAMING_MODE,
			1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_FRAMING_MODE, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_thumbnail_size(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("E, value %d\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_THUMBNAIL_SIZE, val);
	CHECK_ERR(err);

	cam_trace("X\n");

	return err;
}

static int drime4_set_frame_rate(struct v4l2_subdev *sd, int fps)
{
	int err = 0, movie_fps, liveview_fps;
	struct drime4_state *state = to_state(sd);
#if 0
	if (!state->stream_enable) {
		state->fps = fps;
		return 0;
	}
#endif

	cam_dbg("E, fps = %d movie = %d\n",
		fps, state->sensor_mode);

	switch (fps) {
	case 120:
		movie_fps = 0x08;
		liveview_fps = 0x00;
		break;

	case 100:
		movie_fps = 0x07;
		liveview_fps = 0x00;
		break;

	case 60:
		movie_fps = 0x06;
		liveview_fps = 0x01;
		break;

	case 50:
		movie_fps = 0x05;
		liveview_fps = 0x01;
		break;

	case 30:
		movie_fps = 0x04;
		liveview_fps = 0x02;
		break;

	case 25:
		movie_fps = 0x03;
		liveview_fps = 0x02;
		break;

	case 24:
		movie_fps = 0x02;
		liveview_fps = 0x00;
		break;

	case 15:
		movie_fps = 0x01;
		liveview_fps = 0x03;
		break;

	default:
		movie_fps = 0x06;
		liveview_fps = 0x00;	/* 60 fps Max */
		break;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param) {
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_MOVIE_FPS, 1, movie_fps);
		CHECK_ERR(err);

#if 1
		if (state->sensor_mode != SENSOR_MOVIE
			&& state->PASM_mode != MODE_PANORAMA
			&& state->PASM_mode != MODE_DRAMA_SHOT
			&& (fps == 0 || fps == 15)) {
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_LIVEVIEW_FPS, 1, liveview_fps);
			CHECK_ERR(err);
		}
#endif
	} else
#endif
	{
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_MOVIE_FPS, movie_fps);
		CHECK_ERR(err);

#if 1
		if (state->sensor_mode != SENSOR_MOVIE
			&& state->PASM_mode != MODE_PANORAMA
			&& state->PASM_mode != MODE_DRAMA_SHOT
			&& (fps == 0 || fps == 15)) {
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_LIVEVIEW_FPS, liveview_fps);
			CHECK_ERR(err);
		}
#endif
	}

	return err;
}

static int drime4_set_AEB_step(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_aeb_order = 0x02;
	int set_aeb = 0x0200;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 8) {
		cam_warn("invalid value, %d\n", val);
		val = 0;
	}

	set_aeb = (set_aeb_order << 8) | (val & 0xFF);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_AEB_SET,
			2, set_aeb);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AEB_SET, set_aeb);
	CHECK_ERR(err);

	cam_trace("X - 0x%02x\n", set_aeb);
	return 0;
}

static int drime4_set_WBB_step(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_wbb = 0x0000;
	cam_dbg("E, value %d\n", val);

	if (val >= 0 && val <= 2)
		set_wbb = (val << 8);
	else if (val >= 3 && val <= 5)
		set_wbb = ((val - 3) << 8) | (1 & 0xFF);
	else
		cam_warn("invalid value, %d\n", val);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_WBB_SET,
			2, set_wbb);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_WBB_SET, set_wbb);
	CHECK_ERR(err);

	cam_trace("X - 0x%02x\n", set_wbb);
	return 0;
}

static int drime4_set_face_zoom(struct v4l2_subdev *sd, int val)
{
	return 0;
}


static int drime4_set_face_detection(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;

	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case FACE_DETECTION_ON:
	case FACE_DETECTION_NORMAL:
#if 0	/* Change FD : Not use parameter */
		err = drime4_set_af_area(sd, DRIME4_AF_AREA_FD);
		CHECK_ERR(err);
#endif
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_CMD_FACEDETECT,
				1, 0x01);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_FACEDETECT, 0x01);
		CHECK_ERR(err);
		break;

	case FACE_DETECTION_OFF:
#if 0	/* Change FD : Not use parameter */
		err = drime4_set_af_area(sd, state->af_area);
		CHECK_ERR(err);
#endif
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_CMD_FACEDETECT,
				1, 0x00);
		else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_FACEDETECT, 0x00);
		CHECK_ERR(err);
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = FACE_DETECTION_OFF;
		state->facedetect_mode = val;
		goto retry;
	}

	state->facedetect_mode = val;

	cam_trace("X\n");
	return 0;
}

static int drime4_set_hybrid_capture(struct v4l2_subdev *sd)
{
	return 0;
}

static int drime4_get_lux(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	int err = 0;

	return err;
}

static int drime4_set_low_light_mode(struct v4l2_subdev *sd, int val)
{
	return 0;
}

static int drime4_set_capture_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_fps = 0;
	int set_drive = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

capture_mode_retry:
	switch (val) {
	case RUNNING_MODE_SINGLE:
		state->drive_capture_mode = DRIME4_CAP_MODE_SINGLE;
		state->capture_cnt = 1;
		set_fps = 0;
		set_drive = 0;
		break;

	case RUNNING_MODE_CONTINUOUS_9FPS:
		state->drive_capture_mode = DRIME4_CAP_MODE_CONT_9;
		set_fps = 9;
		set_drive = 1;
		break;

	case RUNNING_MODE_CONTINUOUS_5FPS:
		state->drive_capture_mode = DRIME4_CAP_MODE_CONT_5;
		set_fps = 5;
		set_drive = 1;
		break;

	case RUNNING_MODE_BURST:
		state->drive_capture_mode = DRIME4_CAP_MODE_BURST;
		set_fps = 30;
		set_drive = 2;
		break;

	case RUNNING_MODE_BURST_10FPS:
		state->drive_capture_mode = DRIME4_CAP_MODE_BURST;
		set_fps = 10;
		set_drive = 2;
		break;

	case RUNNING_MODE_BURST_15FPS:
		state->drive_capture_mode = DRIME4_CAP_MODE_BURST;
		set_fps = 15;
		set_drive = 2;
		break;

	case RUNNING_MODE_BURST_30FPS:
		state->drive_capture_mode = DRIME4_CAP_MODE_BURST;
		set_fps = 30;
		set_drive = 2;
		break;

	case RUNNING_MODE_AE_BRACKET:
		state->drive_capture_mode = DRIME4_CAP_MODE_AEB;
		state->capture_cnt = 3;
		set_fps = 0;
		set_drive = 3;
		break;

	case RUNNING_MODE_WB_BRACKET:
		state->drive_capture_mode = DRIME4_CAP_MODE_WBB;
		state->capture_cnt = 3;
		set_fps = 0;
		set_drive = 4;
		break;

	case RUNNING_MODE_PW_BRACKET:
		state->drive_capture_mode = DRIME4_CAP_MODE_PWB;
		state->capture_cnt = 3;
		set_fps = 0;
		set_drive = 5;
		break;

	case RUNNING_MODE_SMART:
		state->drive_capture_mode = DRIME4_CAP_MODE_SMART;
		set_fps = 0;
		set_drive = 0;
		break;

	case RUNNING_MODE_HDR:
		state->drive_capture_mode = DRIME4_CAP_MODE_HDR;
		state->capture_cnt = 3;
		set_fps = 0;
		set_drive = 0;
		break;

	case RUNNING_MODE_BULB:
		state->drive_capture_mode = DRIME4_CAP_MODE_BULB;
		state->capture_cnt = 1;
		set_fps = 0;
		set_drive = 0;
		break;

	case RUNNING_MODE_CUSTOM_WB:
		state->drive_capture_mode = DRIME4_CAP_MODE_CUST_WB;
		state->capture_cnt = 1;
		set_fps = 0;
		set_drive = 0;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = RUNNING_MODE_SINGLE;
		goto capture_mode_retry;
	}

	set_val = ((set_fps << 8) | set_drive) & 0x0FFFF;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_DRIVE,
			2, set_val);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_DRIVE, set_val);
	CHECK_ERR(err);

	state->capture_mode = val;

	if (state->capture_mode == RUNNING_MODE_SINGLE) {
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_w_param(sd, DRIME4_SET_CAP_CNT,
				2, state->capture_cnt);
		else
#endif
			err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_CAP_CNT, state->capture_cnt);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_smart_pro_level(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val - 1);

	if (val < V4L2_WIDGET_MODE_LEVEL_1
		|| val > V4L2_WIDGET_MODE_LEVEL_5) {
		cam_warn("invalid value, %d\n", val - 1);
		return 0;
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_SMART_PRO_LVL,
			1, val - 1);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_PRO_LVL, val - 1);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_PASM_mode(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val);

	switch (val) {
	case MODE_SMART_AUTO:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_SYSTEM_MODE, 1, 0);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_SYSTEM_MODE, 0);
		CHECK_ERR(err);
		break;

	case MODE_PROGRAM:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_SYSTEM_MODE, 1, 2);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_SYSTEM_MODE, 2);
		CHECK_ERR(err);

#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_EXPERT_SUB_MODE, 1, 0);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_EXPERT_SUB_MODE, 0);
		CHECK_ERR(err);
		break;

	case MODE_A:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_SYSTEM_MODE, 1, 2);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_SYSTEM_MODE, 2);
		CHECK_ERR(err);

#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_EXPERT_SUB_MODE, 1, 1);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_EXPERT_SUB_MODE, 1);
		CHECK_ERR(err);
		break;

	case MODE_S:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_SYSTEM_MODE, 1, 2);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_SYSTEM_MODE, 2);
		CHECK_ERR(err);

#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_EXPERT_SUB_MODE, 1, 2);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_EXPERT_SUB_MODE, 2);
		CHECK_ERR(err);
		break;

	case MODE_M:
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_SYSTEM_MODE, 1, 2);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_SYSTEM_MODE, 2);
		CHECK_ERR(err);

#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd,
				DRIME4_SET_EXPERT_SUB_MODE, 1, 3);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_EXPERT_SUB_MODE, 3);
		CHECK_ERR(err);
		break;

	case MODE_VIDEO:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 3);
		CHECK_ERR(err);
		break;

	case MODE_BEAUTY_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x00);
		CHECK_ERR(err);
		break;

	case MODE_BEST_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x01);
		CHECK_ERR(err);
		break;

	case MODE_CONTINUOUS_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x02);
		CHECK_ERR(err);
		break;

	case MODE_BEST_FACE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x03);
		CHECK_ERR(err);
		break;

	case MODE_PW_BRACKET:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x04);
		CHECK_ERR(err);
		break;

	case MODE_BABY_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x05);
		CHECK_ERR(err);
		break;

	case MODE_VIGNTTING:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x06);
		CHECK_ERR(err);
		break;

	case MODE_LANDSCAPE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x07);
		CHECK_ERR(err);
		break;

	case MODE_DAWN:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x08);
		CHECK_ERR(err);
		break;

	case MODE_SNOW:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x09);
		CHECK_ERR(err);
		break;

	case MODE_CLOSE_UP:	/* MACRO */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0A);
		CHECK_ERR(err);
		break;

	case MODE_FOOD:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0B);
		CHECK_ERR(err);
		break;

	case MODE_PARTY_INDOOR:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0C);
		CHECK_ERR(err);
		break;

	case MODE_ACTION_FREEZE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0D);
		CHECK_ERR(err);
		break;

	case MODE_RICH_TONE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0E);
		CHECK_ERR(err);
		break;

	case MODE_PANORAMA:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x0F);
		CHECK_ERR(err);
		break;

	case MODE_WATERFALL:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x10);
		CHECK_ERR(err);
		break;

	case MODE_ANIMATED_SCENE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x11);
		CHECK_ERR(err);
		break;

	case MODE_MULTI_EXPOSURE_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x12);
		CHECK_ERR(err);
		break;

	case MODE_DRAMA_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x13);
		CHECK_ERR(err);
		break;

	case MODE_ERASER:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x14);
		CHECK_ERR(err);
		break;

	case MODE_SOUND_AND_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x15);
		CHECK_ERR(err);
		break;

	case MODE_MINIATURE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x16);
		CHECK_ERR(err);
		break;

	case MODE_CREATIVE_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x17);
		CHECK_ERR(err);
		break;

	case MODE_INTERVAL_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x18);
		CHECK_ERR(err);
		break;

	case MODE_SILHOUETTE:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x19);
		CHECK_ERR(err);
		break;

	case MODE_SUNSET:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x1A);
		CHECK_ERR(err);
		break;

	case MODE_NIGHT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x1B);
		CHECK_ERR(err);
		break;

	case MODE_FIREWORKS:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x1C);
		CHECK_ERR(err);
		break;

	case MODE_LIGHT_TRAIL_SHOT:	/* LIGHT TRACE */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x1D);
		CHECK_ERR(err);
		break;

	case MODE_3D:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 0x04);
		CHECK_ERR(err);
		break;

	case MODE_GOLF_SHOT:
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 1);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SMART_SUB_MODE, 0x1F);
		CHECK_ERR(err);
		break;

	default:
		cam_err("invalid value, %d\n", val);
		break;
	}

	state->PASM_mode = val;

	if (state->photo != NULL) {
		if (state->PASM_mode == MODE_3D
			&& state->photo->reg_val == 0x13) {
			int num_entries;
			num_entries = ARRAY_SIZE(photo_frmsizes);
			state->photo = drime4_get_frmsize(photo_frmsizes,
				num_entries, DRIME4_CAPTURE_2p1MW_3D);

			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_CAPTURE_SIZE, state->photo->reg_val);
			CHECK_ERR(err);
			cam_trace("PASM mode - %d, set size 0x%02x\n",
				state->PASM_mode, state->photo->reg_val);
		} else if (state->PASM_mode != MODE_3D
			&& state->photo->reg_val == 0x32) {
			int num_entries;
			num_entries = ARRAY_SIZE(photo_frmsizes);
			state->photo = drime4_get_frmsize(photo_frmsizes,
				num_entries, DRIME4_CAPTURE_2p1MW);

			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_CAPTURE_SIZE, state->photo->reg_val);
			CHECK_ERR(err);
			cam_trace("PASM mode - %d, set size 0x%02x\n",
				state->PASM_mode, state->photo->reg_val);
		}
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_program_shift(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	int set_val = 0;

	cam_dbg("E val : %d\n", val);

	switch (val) {
	case V4L2_PROGRAM_SHIFT_RESET:
		set_val = 0x00;
		break;

	case V4L2_PROGRAM_SHIFT_UP:
		set_val = 0x01;
		break;

	case V4L2_PROGRAM_SHIFT_DOWN:
		set_val = 0x02;
		break;

	default:
		set_val = 0x00;
		break;
	}

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_PROGRAM_SHIFT, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_video_out_format(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	int set_val = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case V4L2_VIDEO_FORMAT_NTSC:
		set_val = 0x00;
		break;

	case V4L2_VIDEO_FORMAT_PAL:
		set_val = 0x01;
		break;

	default:
		val = 0x00;
		break;
	}

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_VIDEO_OUT_FORMAT, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_shutter_speed(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int numerator, denominator;
	int set_val;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 56) {
		cam_err("invalid value, %d\n", val);
		return 0;
	}

	numerator = ss_val[val].num << 16;
	denominator = ss_val[val].den & 0x0FFFF;
	set_val = numerator | denominator;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_l_param(sd, DRIME4_SET_SHUTTER_SPEED,
			4, set_val);
	else
#endif
		err = drime4_writel_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SHUTTER_SPEED, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_f_number(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int numerator, denominator;
	int set_f_val;
	cam_dbg("E, value %d\n", val);

#ifdef DRIME4_F_NUMBER_VALUE
	if (val < 0 || val > 900) {
		cam_err("invalid value, %d\n", val);
		return 0;
	}

#ifdef DRIME4_F_NUMBER_VALUE_LONG
	numerator = val << 16;
	denominator = 10 & 0x0FFFF;

	set_f_val = numerator | denominator;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_l_param(sd, DRIME4_SET_F_NUMBER,
			4, set_f_val);
	else
#endif
		err = drime4_writel_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_F_NUMBER, set_f_val);
	CHECK_ERR(err);
#else
	if (val < 100) {
		numerator = val << 8;
		denominator = 10 & 0x0FF;
	} else {
		numerator = (val / 10) << 8;
		denominator = 1 & 0x0FF;
	}
	set_f_val = numerator | denominator;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_w_param(sd, DRIME4_SET_F_NUMBER,
			2, set_f_val);
	else
#endif
		err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_F_NUMBER, set_f_val);
	CHECK_ERR(err);
#endif
#else
	if (val < 0 || val > 40) {
		cam_err("invalid value, %d\n", val);
		return 0;
	}

	numerator = fn_val[val].num << 8;
	denominator = fn_val[val].den & 0x0FF;
	set_f_val = numerator | denominator;

	err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_F_NUMBER, set_f_val);
	CHECK_ERR(err);
#endif

	cam_trace("X\n");
	return 0;
}

static int drime4_find_shutter_speed(struct v4l2_subdev *sd, int num, int den)
{
	int num_entries, i;

	num_entries = ARRAY_SIZE(ss_val);
	for (i = 2; i < num_entries; i++) {
		if ((ss_val[i].den * num) == (ss_val[i].num * den))
			return i;
	}

	cam_err("invalid value, %d / %d\n", num, den);

	return 0;
}

static int drime4_find_f_number(struct v4l2_subdev *sd, int num, int den)
{
	int i;
#ifndef DRIME4_F_NUMBER_VALUE
	int num_entries;
#endif

#ifdef DRIME4_F_NUMBER_VALUE
	if (den == 1)
		i = num * 10;
	else if (den == 10)
		i = num;
	else if (den != 0)
		i = (num * 10) / den;
	else
		goto err_f_num;
#if 0
	cam_err("F number %d, %d / %d\n", i, num, den);
#endif

	return i;
#else
	num_entries = ARRAY_SIZE(fn_val);
	for (i = 1; i < num_entries; i++) {
		if ((fn_val[i].den * num) == (fn_val[i].num * den))
			return i;
	}
#endif

err_f_num:
	cam_err("invalid value, %d / %d\n", num, den);

	return 0;
}

static int drime4_get_smart_read1(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err;
	u8 rdata[50] = {0,};
	int int_factor;
	int Tv, Av, EVC, Exp, Scene, Scene_sub, FD, OT;
	int ISO, AF, Lv;
	int smart_warning;
#ifndef DRIME4_F_NUMBER_VALUE
	u8 rdata2[20] = {0,};
	int Strobo_standby, Strobo_charging;
#endif
	int noti_lock = 0;

	if (!state->done_3a_lock) {
		err = drime4_cmdwn(sd, DRIME4_PACKET_DSP_QUERY,
			DRIME4_QRY_LIVE_3A_DATA);
		CHECK_ERR(err);
	} else {
		err = drime4_cmdwn(sd, DRIME4_PACKET_DSP_QUERY,
			DRIME4_QRY_3A_LOCK_DATA);
		CHECK_ERR(err);
	}

	/* ISP ready wait */
	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&rdata[0], noti_lock, 0);
		CHECK_ERR(err);
		if (rdata[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("drime4_write res failed: packet ID err - w:0x%02x r:0x%02x\n",
				DRIME4_PACKET_DSP_QUERY_RES, rdata[2]);
			return 0;
		}
	} else {
		cam_err("drime4_write res timeout: smart read - lock status %d\n",
			state->done_3a_lock);
		return -1;
	}

#ifndef DRIME4_F_NUMBER_VALUE
	err = drime4_cmdwn(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_STROBO_STATUS);
	CHECK_ERR(err);

	/* ISP ready wait */
	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&rdata2[0], 0, 0);
		CHECK_ERR(err);
		if (rdata2[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("drime4_write res failed: packet ID err - w:0x%02x r:0x%02x\n",
				DRIME4_PACKET_DSP_QUERY_RES, rdata2[2]);
			return 0;
		}
	}
	Strobo_standby = rdata2[6] & 0x1;
	Strobo_charging = 0;
#endif

	/* Lenth Check */
	if (rdata[5] < 0x0C) {
		cam_err("drime4_query res failed: lenth is wrong %02x\n",
			rdata[5]);
		state->smart_read[0] = 0;
		state->smart_read[1] = 0;
	} else if (rdata[5] > 0x0C) {
		Tv = drime4_find_shutter_speed(sd,
			((rdata[9] << 8) | rdata[8]) & 0x0FFFF,
			((rdata[7] << 8) | rdata[6]) & 0x0FFFF) & 0x3F;

		Av = drime4_find_f_number(sd,
			((rdata[13] << 8) | rdata[12]) & 0x0FFFF,
			((rdata[11] << 8) | rdata[10]) & 0x0FFFF) & 0x3FF;

		EVC = rdata[14] & 0x1F;
		Exp = rdata[15] & 0xF;
		Scene = rdata[17] & 0x1F;
		Scene_sub = rdata[16] & 0x1F;
		if (!state->done_3a_lock) {
			FD = rdata[18] & 0xF;
			OT = rdata[19] & 0x1;
			Lv = rdata[20] & 0xF;
			ISO = 0;
			AF = 0;
			smart_warning = 0;
		} else {
			FD = (state->smart_read[1] >> 20) & 0xF;
			OT = (state->smart_read[0] >> 31) & 0x1;
			Lv = (state->smart_read[0] >> 25) & 0xF;
			ISO = rdata[18] & 0x1F;
			AF = rdata[19] & 0x1;
			if (rdata[5] > 0x0E)
				smart_warning = rdata[20] & 0xF;
			else
				smart_warning = 0;
		}
#ifdef DRIME4_F_NUMBER_VALUE
		state->smart_read[0] = (OT << 31) | \
			(Lv << 25) | (Exp << 21) | \
			(EVC << 16) | (Av << 6) | Tv;

		state->smart_read[1] = 0 | \
			(FD << 20) | \
			(smart_warning << 16) | \
			(Scene_sub << 11) | (Scene << 6) | \
			(AF << 5) | ISO;
#else
		state->smart_read[0] = (OT << 31) | (FD << 30) | \
			(Scene_sub << 25) | (Scene << 20) | \
			(Exp << 16) | (EVC << 12) | (Av << 6) | Tv;

		state->smart_read[1] = (Strobo_standby << 31) | \
			(Strobo_charging << 30) | \
			(Lv << 6) | (AF << 5) | ISO;
#endif
	} else {
		Tv = drime4_find_shutter_speed(sd,
			((rdata[9] << 8) | rdata[8]) & 0x0FFFF,
			((rdata[7] << 8) | rdata[6]) & 0x0FFFF) & 0x3F;

		Av = drime4_find_f_number(sd,
			rdata[11], rdata[10]) & 0x3FF;

		EVC = rdata[12] & 0x1F;
		Exp = rdata[13] & 0xF;
		Scene = rdata[15] & 0x1F;
		Scene_sub = rdata[14] & 0x1F;
		if (rdata[5] > 0x0C)
			Lv = rdata[18] & 0xF;
		else
			Lv = 3;
		if (!state->done_3a_lock) {
			FD = rdata[16] & 0xF;
			OT = rdata[17] & 0x1;
			ISO = 0;
			AF = 0;
		} else {
			FD = (state->smart_read[1] >> 16) & 0xF;
			OT = (state->smart_read[0] >> 31) & 0x1;
			ISO = rdata[16] & 0x1F;
			AF = rdata[17] & 0x1;
		}
#ifdef DRIME4_F_NUMBER_VALUE
		state->smart_read[0] = (OT << 31) | \
			(Lv << 25) | (Exp << 21) | \
			(EVC << 16) | (Av << 6) | Tv;

		state->smart_read[1] = 0 | \
			(FD << 16) | \
			(Scene_sub << 11) | (Scene << 6) | \
			(AF << 5) | ISO;
#else
		state->smart_read[0] = (OT << 31) | (FD << 30) | \
			(Scene_sub << 25) | (Scene << 20) | \
			(Exp << 16) | (EVC << 12) | (Av << 6) | Tv;

		state->smart_read[1] = (Strobo_standby << 31) | \
			(Strobo_charging << 30) | \
			(Lv << 6) | (AF << 5) | ISO;
#endif
	}

	ctrl->value = state->smart_read[0] & 0x0FFFFFFFFLL;

	return 0;
}

static int drime4_get_smart_read2(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);

	ctrl->value = state->smart_read[1] & 0x0FFFFFFFFLL;

	return 0;
}

static char *itoa(int val)
{
	static char buf[INT_DIGITS + 2];
	char *p = buf + INT_DIGITS + 1;

	if (val >= 0) {
		do {
			*--p = '0' + (val % 10);
			val /= 10;
		} while (val != 0);
			return p;
	} else {
		do {
			*--p = '0' - (val % 10);
			val /= 10;
		} while (val != 0);

		*--p = '-';
	}
	return p;
}

static int drime4_get_distance_scale(struct v4l2_subdev *sd,
	struct v4l2_ext_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err;
	u8 rdata[100] = {0,};
	int int_factor;
	char *buf;
	int i = 0, j = 0;
	int temp = 0;
	int cnt = 0;
	int length = 0;
	int noti_lock = 0;

	err = drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_LENS_NEED_DISTANCE);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&rdata[0], noti_lock, 0);
		CHECK_ERR(err);

		if (rdata[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("drime4_write failed: packet ID err - w:0x%02x r:0x%02x\n",
			DRIME4_PACKET_DSP_QUERY_RES, rdata[2]);
			return err;
		}
		cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x\n",
			rdata[5], rdata[2], rdata[4], rdata[3]);
	} else {
		cam_err("drime4_write res timeout(distance scale): packet ID 0x%02x param ID 0x%02x\n",
			DRIME4_PACKET_DSP_QUERY, DRIME4_QRY_LENS_NEED_DISTANCE);
		return -1;
	}

	if (rdata[5] != 0x50) {
		cam_err("drime4_query res failed: lenth is wrong 0x%02x\n",
			rdata[5]);
	} else {
		state->distance_scale_enable = 0x01;
		for (i = 0; i < 20; i++) {
			temp = (rdata[(i << 2)+0+6] & 0xFF) | \
				(rdata[(i << 2)+1+6] << 8 & 0xFFFF) | \
				(rdata[(i << 2)+2+6] << 16 & 0xFFFFFF) | \
				(rdata[(i << 2)+3+6] << 24 & 0xFFFFFFFF);

			if (0 != temp)
				state->distance_scale_enable = 0x00;

			buf = itoa(temp);
			length = strlen(buf);

			for (j = 0; j < length; j++)
				state->distance_scale_data[j + cnt] = buf[j];

			state->distance_scale_data[cnt + length] = ' ';
			cnt += (length+1);
		}
	}
	return err;
}

static int drime4_get_lens_info(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err;
	u8 rdata[100] = {0,};
#if 0
	u8 rdata_scale[10] = {0,};
#endif
	int int_factor;
	int Lens_ID, AF_able, OIS_able, contrast_able, zoom_able;
	int MF_switch, OIS_switch, I_Fun_able;
	int focus_limit_sw, focus_limit_fun, FTMF_able, able_3D;
	int focus_max, focus_min, f_num_min, f_num_max;
	int LDC_status, Lens_seg, I_setting;
	int distance_scale;
	int noti_lock = 0;

	err = drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_LENS_INFO);
	CHECK_ERR(err);

	/* ISP ready wait */
	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&rdata[0], noti_lock, 0);
		CHECK_ERR(err);
		if (rdata[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("drime4_write res failed: packet ID err - w:0x%02x r:0x%02x\n",
				DRIME4_PACKET_DSP_QUERY_RES, rdata[2]);
			return 0;
		}
		cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x\n",
			rdata[5], rdata[2], rdata[4], rdata[3]);
	} else {
		cam_err("drime4_write res timeout(lens info): packet ID 0x%02x param ID 0x%02x\n",
			DRIME4_PACKET_DSP_QUERY, DRIME4_QRY_LENS_INFO);
		return -1;
	}
#if 0
	err = drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_LENS_NEED_DISTANCE);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&rdata_scale[0], 0, 1);
		CHECK_ERR(err);

		if (rdata_scale[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("drime4_write failed: packet ID err - w:0x%02x r:0x%02x\n",
			DRIME4_PACKET_DSP_QUERY_RES, rdata_scale[2]);
			return err;
		}
	}
	distance_scale = rdata_scale[6] & 0xFF;
#else
	distance_scale = 0x01;
#endif
	/* Lenth Check */
	if (rdata[5] < 0x2F) {
		cam_err("drime4_query res failed: lenth is wrong %02x\n",
			rdata[5]);
		state->smart_read[2] = 0;
		state->smart_read[3] = 0;
	} else if (rdata[6] == 0x00 || rdata[6] == 0xFF) {
		/* 0x00 : K-mount, 0xFF : no Lens */
		state->smart_read[2] = rdata[6];
		state->smart_read[3] = 0;
	} else {
		Lens_ID = rdata[6] & 0xFF;
		AF_able = rdata[7] & 0x1;
		OIS_able = rdata[8] & 0x1;
		contrast_able = rdata[9] & 0x1;
		zoom_able = rdata[10] & 0x1;
		MF_switch = rdata[11] & 0x1;
		OIS_switch = rdata[12] & 0x1;
		I_Fun_able = rdata[13] & 0x1;
		focus_limit_sw = rdata[14] & 0x1;
		focus_limit_fun = rdata[15] & 0x1;
		FTMF_able = rdata[16] & 0x1;
		able_3D = rdata[17] & 0x1;
		focus_max = ((rdata[21] << 24) | (rdata[20] << 16) | \
			(rdata[19] << 8) | rdata[18]) & 0xFFFFFFFF;
		focus_min = ((rdata[25] << 24) | (rdata[24] << 16) | \
			(rdata[23] << 8) | rdata[22]) & 0xFFFFFFFF;
		LDC_status = rdata[26] & 0x03;
		Lens_seg = rdata[27] & 0xFF;
		I_setting = rdata[28] & 0xFF;
		memcpy(sysfs_lens_fw, &rdata[29], 20);

		if (rdata[5] > 0x2F) {
			f_num_max = drime4_find_f_number(sd,
			((rdata[52] << 8) | rdata[51]) & 0x0FFFF,
			((rdata[50] << 8) | rdata[49]) & 0x0FFFF) & 0x3FF;
			f_num_min = drime4_find_f_number(sd,
			((rdata[56] << 8) | rdata[55]) & 0x0FFFF,
			((rdata[54] << 8) | rdata[53]) & 0x0FFFF) & 0x3FF;
		} else {
			f_num_max = drime4_find_f_number(sd,
				rdata[50], rdata[49]) & 0x3FF;
			f_num_min = drime4_find_f_number(sd,
				rdata[52], rdata[51]) & 0x3FF;
		}

#ifdef DRIME4_F_NUMBER_VALUE
		state->smart_read[2] =
			(state->distance_scale_enable << 13) | \
			(LDC_status << 11) | (able_3D << 10) | \
			(FTMF_able << 9) | (focus_limit_fun << 8) | \
			(focus_limit_sw << 7) | (I_Fun_able << 6) | \
			(OIS_switch << 5) | (MF_switch << 4) | \
			(zoom_able << 3) | (contrast_able << 2) | \
			(OIS_able << 1) | AF_able;
#else
		state->smart_read[2] =
			(f_num_min << 26) | (f_num_max << 20) | \
			(state->distance_scale_enable << 13) | \
			(LDC_status << 11) | (able_3D << 10) | \
			(FTMF_able << 9) | (focus_limit_fun << 8) | \
			(focus_limit_sw << 7) | (I_Fun_able << 6) | \
			(OIS_switch << 5) | (MF_switch << 4) | \
			(zoom_able << 3) | (contrast_able << 2) | \
			(OIS_able << 1) | AF_able;
#endif
		state->smart_read[3] = (f_num_min << 10) | f_num_max;
	}

	ctrl->value = state->smart_read[2] & 0x0FFFFFFFFLL;

	return 0;
}

static int drime4_set_LDC(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;

	/* Distortion Correct */
#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_LDC, 1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_LDC, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_AF_LED(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_dbg("E, value %d\n", val);

	if (!state->factory_running) {
#ifdef BURST_BOOT_PARAM
		if (state->first_boot_param)
			err = drime4_add_boot_b_param(sd, DRIME4_SET_AF_LAMP,
				1, val);
		else
#endif
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_AF_LAMP, val);
		CHECK_ERR(err);
	} else {
		err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
		CHECK_ERR(err);

		err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_SET_AF_LED, val);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return err;
}

static int drime4_set_timer_LED(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 30) {
		cam_warn("invalid value, %d\n", val);
		val = 0;	/* LED off */
	}

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_TIMER_LED, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_Drive(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_dbg("E, value %d\n", val);

	err = drime4_writew_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_DRIVE, 0);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_af_priority(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("E, value %d\n", val);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_AF_PRIORITY,
			1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AF_PRIORITY, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_af_frame_size(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_dbg("E, value %d\n", val);

	if (0 > val || val > 3) {
		cam_warn("invalid value, %d\n", val);
		val = 0;
	}

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_AF_FRAME_SIZE, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_high_iso_nr(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 3) {
		cam_warn("invalid value, %d\n", val);
		val = 0;	/* off */
	}

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_HIGH_ISO_NR,
			1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_HIGH_ISO_NR, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_long_term_nr(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_LONG_TERM_NR,
			1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_LONG_TERM_NR, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_DMF(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;
	else
		set_val = 0;
	cam_dbg("set_DMF, set_val = [%d]\n", set_val);

	/* Direct MF */
#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_DMF, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_DMF, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_color_space(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;	/* Adobe RGB */
	else
		set_val = 0;	/* SRGB */

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_COLOR_SPACE,
			1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_COLOR_SPACE, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_dynamic_range(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_dbg("E, value %d\n", val);

	if (val < 0 || val > 2) {
		cam_warn("invalid value, %d\n", val);
		val = 0;	/* off */
	}

	/* 0x00 : Off
	0x01 : Smart Range+
	0x02 : HDR (Rich tone)*/

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_DYNAMIC_RANGE,
			1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_DYNAMIC_RANGE, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_quickview_cancel(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);
	/* Quickview cancel to start liveview */
	err = drime4_write0_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_QUICKVIEW_CANCEL);
	CHECK_ERR(err);
	return err;
}

static int drime4_set_i_func_enable(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 1;	/* i Func On */
	else
		set_val = 0;	/* i Func Off */

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_i_FN_ENABLE, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_quickview(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_dbg("E, value %d\n", val);

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd, DRIME4_SET_QUICKVIEW, 1, val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
				DRIME4_SET_QUICKVIEW, val);
	CHECK_ERR(err);

	return err;
}

static int drime4_set_AE_AF_lock(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int int_factor = 0;
	u8 read_val[100];

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_AE_AF_LOCK, val);
	CHECK_ERR(err);

	cam_trace("X\n");

	return err;
}

static int drime4_set_over_exposure_guide(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 0x01;
	else
		set_val = 0x00;

#ifdef BURST_BOOT_PARAM
	if (state->first_boot_param)
		err = drime4_add_boot_b_param(sd,
			DRIME4_SET_OVER_EXPOSURE_GUIDE, 1, set_val);
	else
#endif
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_OVER_EXPOSURE_GUIDE, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_no_lens_capture(struct v4l2_subdev *sd, int val)
{
	int err;
	int set_val = 0;
	cam_dbg("E, value %d\n", val);

	if (val)
		set_val = 0x01;
	else
		set_val = 0x00;

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_NO_LENS_CAPTRUE, set_val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return 0;
}

static int drime4_set_mic_control(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_MIC_CONTROL, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_set_optical_preview(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_OPTICAL_PREVIEW, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_mode_set(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("E, value %d\n", val);

	if (val == 999) {/* live view start after set production mode */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AF_MODE, 0);
		CHECK_ERR(err);
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AF_AREA, 0);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_PRODUCTION_MODE, 1);
		CHECK_ERR(err);

		return err;
	} else if (val == 998) {/* for cold reboot */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_LIVEVIEW, 0x00);
			CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_COLD_REBOOT, 0);
		CHECK_ERR(err);

		return err;
	}

	if (!val) {/* production mode off */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_PRODUCTION_MODE, val);
		CHECK_ERR(err);
	}

	state->factory_running = val;

	cam_trace("X\n");
	return err;
}

static int drime4_factory_fw_check(struct v4l2_subdev *sd, char *buf)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("E, value %s\n", buf);

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 25);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_write_buf(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_FW_VERSION_CHECK, 16, &buf[0], 1);
	CHECK_ERR(err);

	err = drime4_write_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_FW_VERSION_CHECK);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_D4_cmd(struct v4l2_subdev *sd, char *buf, u8 size)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("E, value %s\n", buf);

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, size + 9);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_write_buf(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_SCRIPT_BYPASS, size, &buf[0], 0);
	CHECK_ERR(err);

	err = drime4_write_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_SCRIPT_BYPASS);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_send_PC_data(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize = 0, nread;
#if 0
	int i;
#endif

	cam_dbg("E\n");

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(DRIME4_FACTORY_PCDATA_PATH, O_RDONLY, 0);

	if (IS_ERR(fp)) {
		err = -ENOENT;
		goto out;
	} else
		cam_dbg("%s is opened\n", DRIME4_FACTORY_PCDATA_PATH);

	fsize = fp->f_path.dentry->d_inode->i_size;

	cam_dbg("size %ld Bytes\n", fsize);

	err = vfs_llseek(fp, 0, SEEK_SET);
	if (err < 0) {
		cam_err("failed to fseek, %d\n", err);
		goto out;
	}

	nread = vfs_read(fp, (char __user *)data_memory,
		fsize, &fp->f_pos);
	if (nread != fsize) {
		cam_err("failed to read data file\n");
		err = -EIO;
		goto out;
	}
#if 0
	for (i = 0; i < fsize; i++) {
		cam_dbg("buf[%d] = 0x%02x\n",
			i, data_memory[i]);
	}
#endif

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, fsize + 6);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_cmd_buf(sd, DRIME4_PACKET_ADJUST_PC_DATA,
		fsize, &data_memory[0], 0);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_ADJUST_PC_DATA);
	CHECK_ERR(err);

out:
	if (!IS_ERR(fp) && fp != NULL)
		filp_close(fp, current->files);

	set_fs(old_fs);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_flash_write(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_dbg("E, value %d\n", val);

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	/* Write adjust result to Nand Flash */
	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_WRITE_NAND_FLASH, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_log_write(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);

	struct file *fp;
	mm_segment_t old_fs;
	int err;
#if 0
	int i;
#endif
	int data_length;
	u8 read_val[1000] = {0,};
	u32 int_factor;
	u8 *buf = NULL;
	u8 dummy_data[2] = {0x00, 0xFF};

	cam_dbg("E, value %d\n", val);

	state->factory_log_start = 0;

	err = drime4_cmd_buf(sd, DRIME4_PACKET_LOG_WRITE, 2, &dummy_data[0], 1);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (err < 0)
			cam_err("drime4_spi_write failed\n");
		CHECK_ERR(err);
	} else {
		cam_err("timeout return\n");
		return -EBUSY;
	}

	data_length = (((read_val[1] & 0xFF) << 8) + (read_val[0] & 0xFF)) - 6;

	buf = vmalloc(data_length);
	memcpy(buf, &read_val[3], data_length);
#if 0
	for (i = 0; i < data_length; i++) {
		cam_dbg("csv buf[%d] = 0x%02x\n",
			i, buf[i]);
	}
#endif
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(DRIME4_FACTORY_CSV_PATH,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		cam_err("failed to open %s, err %ld\n",
			DRIME4_FACTORY_CSV_PATH, PTR_ERR(fp));
		err = -ENOENT;
		goto file_out;
	}

	cam_dbg("start, file path %s\n", DRIME4_FACTORY_CSV_PATH);

	vfs_write(fp, (char __user *)buf, data_length, &fp->f_pos);

file_out:
	if (buf != NULL)
		vfree(buf);

	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);

	cam_trace("X\n");

	return err;
}

static int drime4_factory_log_write_start(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);

	int err;
	u8 read_val[1000] = {0,};
	u32 int_factor;

	cam_dbg("E, value %d\n", val);

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 10);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	state->factory_log_start = 1;

	/* Write adjust result to CSV file */
	err = drime4_writeb(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_MAKE_CSV, val);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (err < 0)
			cam_err("drime4_spi_write failed\n");
		CHECK_ERR(err);
	} else {
		cam_err("timeout return\n");
		return -EBUSY;
	}

	cam_trace("X\n");

	return err;
}

static int drime4_factory_shutterspeed(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x03);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x03);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x03);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_shutterX(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x0F);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x0F);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x0F);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_cis_data(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x02);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x02);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x02);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_lens_mount(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x0D);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x0D);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x0D);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_hot_shoe(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x0C);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x0C);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x0C);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_flange_back(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x01);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x01);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x01);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_focus_position(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);

	int err = 0;
	u8 buf[5];

	cam_dbg("E, value %d, step : %d\n", val, state->af_step);

	buf[0] = val;
	buf[1] = state->af_step & 0xFF;
	buf[2] = (state->af_step & 0xFF00) >> 8;
	buf[3] = 0;
	buf[4] = 0;

	err = drime4_cmdl(sd, DRIME4_PACKET_CMD_DATA_SIZE, 14);
	CHECK_ERR(err);

	err = drime4_cmd_res(sd, DRIME4_PACKET_CMD_DATA_SIZE);
	CHECK_ERR(err);

	err = drime4_write_buf(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_SET_FOCUS_POS, 5, &buf[0], 1);
	CHECK_ERR(err);

	err = drime4_write_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_SET_FOCUS_POS);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_dust_reduction(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_trace("E\n");

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_CIS_CLEAN, 1);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_factory_liveview_iso_sens(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x04);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x04);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x04);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_capture_iso_sens(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x05);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x05);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x05);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_AWB(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x06);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x06);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x06);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_VFPN(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x07);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x07);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x07);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_defect_pixel(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x08);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x08);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x08);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_ColorShading(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x09);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x09);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x09);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_PAF(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x0A);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x0A);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x0A);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_flash(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_FLASH_WRITE:
		err = drime4_factory_flash_write(sd, 0x10);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write_start(sd, 0x10);
		CHECK_ERR(err);
		break;

	case FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, 0x10);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_FlashRom(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	switch (val) {
	case FACTORY_LOG_WRITE_START:
		err = drime4_factory_log_write(sd, 0x00);
		CHECK_ERR(err);
		break;

	default:
		break;

	}

	cam_trace("X\n");
	return err;
}

static int drime4_factory_get_isp_log(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_trace("E val : %d\n", val);

	if (val) {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_GET_ISP_LOG, 0x01);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return err;
}

static int drime4_noti_disable(struct v4l2_subdev *sd, int val)
{
	int err = 0;
	cam_trace("E val : %d\n", val);

	if (val) {
		/* Notification off */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_NOTI_DISABLE, 0x01);
		CHECK_ERR(err);
	} else {
		/* Notification start */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_NOTI_DISABLE, 0x00);
		CHECK_ERR(err);
	}

	return err;
}

#ifdef BURST_BOOT_PARAM
static int drime4_set_burst_boot_param(struct v4l2_subdev *sd, int val)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int int_factor = 0;
	u8 read_val[100];
	int i;
	int noti_lock = 0;

	cam_dbg("E, value %d\n", val);

	if (val) {
		if (state->noti_ctrl_status)
			drime4_spi_set_cmd_ctrl_pin(0);
		state->first_boot_param = 1;

		for (i = 0; i < MAX_PARAM_SIZE; i++)
			boot_param[i] = 0;

		param_index = 0;
	} else {
		state->first_boot_param = 0;

		if (state->factory_running || param_index == 0)
			return 0;

		err = drime4_write_multi_buf(sd, DRIME4_PACKET_DSP_SET,
			param_index, &boot_param[0], 0);

		CHECK_ERR(err);

		/* ISP ready wait */
		int_factor = drime4_wait_ap_interrupt(sd,
			DRIME4_ISP_TIMEOUT);
		if (!state->noti_ctrl_status)
			noti_lock = 1;
		if (int_factor) {
			err = drime4_spi_read_res(&read_val[0], noti_lock, 0);
			if (err < 0)
				cam_err("drime4_spi_read_res failed\n");
			if (read_val[6] != 0x00)
				cam_err("set_burst_boot_param failed - 0x%02x\n",
					read_val[6]);
			cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x\n",
				read_val[5], read_val[2], read_val[4],
				read_val[3]);
		} else {
			cam_err("drime4_write res timeout(boot param): packet ID 0x%02x param ID 0x%02x\n",
				DRIME4_PACKET_DSP_SET, param_index);
			return -1;
		}

		if (state->PASM_mode != MODE_PANORAMA
			&& state->PASM_mode != MODE_DRAMA_SHOT
			&& state->PASM_mode != MODE_GOLF_SHOT) {
				err = drime4_write0_res(sd,
						DRIME4_PACKET_DSP_CMD,
						DRIME4_CMD_LIVEVIEW_PREPARE);
				CHECK_ERR(err);
		}
	}

	cam_trace("X\n");
	return err;
}
#endif

static int drime4_set_mf_focus_position(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_dbg("E, value %d\n", val);

	err = drime4_writel_res(sd, DRIME4_PACKET_DSP_CMD,
		DRIME4_CMD_MF_POSITION_SET, val);
	CHECK_ERR(err);

	cam_trace("X\n");
	return err;
}

static int drime4_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_dbg("id %d, value %d\n",
		ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);

	mutex_lock(&state->drime4_lock);

	if (unlikely(state->isp.bad_fw && ctrl->id != V4L2_CID_CAM_UPDATE_FW)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_FRAME_RATE:
		err = drime4_set_frame_rate(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACE_DETECTION:
		err = drime4_set_face_detection(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACE_ZOOM:
		err = drime4_set_face_zoom(sd, ctrl->value);
		break;

	case V4L2_CID_CAM_UPDATE_FW:
		if (ctrl->value == FW_MODE_DUMP)
			err = drime4_dump_fw(sd);
		else if (ctrl->value == FW_MODE_UPDATE)
			err = drime4_load_fw(sd);
		else if (ctrl->value == FW_MODE_VERSION)
			err = drime4_check_fw(sd);
		else
			err = 0;
		break;

	case V4L2_CID_LENS_UPDATE_FW:
		if (ctrl->value == CAM_LENS_FW_MODE_VERSION)
			err = drime4_check_lens_fw(sd);
		else if (ctrl->value == CAM_LENS_FW_MODE_UPDATE)
			err = drime4_load_lens_fw(sd);
		else
			err = 0;
		break;

	case V4L2_CID_AEAF_LOCK_UNLOCK:
		err = drime4_set_AE_AF_lock(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = drime4_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CAPTURE_MODE:
		err = drime4_set_capture_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = drime4_set_flash(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_FLASH_EVC_STEP:
		err = drime4_set_flash_step(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_POPUP:
		err = drime4_set_flash_popup(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		err = drime4_set_antibanding(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO:
		err = drime4_set_iso(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_METERING:
		err = drime4_set_metering(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = drime4_set_exposure(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		err = drime4_set_contrast(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SHARPNESS:
		err = drime4_set_sharpness(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SATURATION:
		err = drime4_set_saturation(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_COLOR:
		err = drime4_set_color(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = drime4_set_whitebalance(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_K_VALUE:
		err = drime4_set_wb_K(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_GBAM:
		err = drime4_set_wb_GBAM(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WB_CUSTOM_X:
		err = drime4_set_custom_wb(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = drime4_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = drime4_set_effect(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WDR:
		err = drime4_set_wdr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_IMAGE_STABILIZER:
		if (state->sensor_mode == SENSOR_CAMERA)
			err = drime4_set_antishake(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_DEFAULT_FOCUS_POSITION:
		/*err = drime4_set_af_mode(sd, state->focus.mode);*/
		err = 0;
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = drime4_set_af_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_AREA_MODE:
		err = drime4_set_focus_area_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP:
		err = drime4_set_object_tracking(sd, ctrl->value);
		break;
	case V4L2_CID_CAMERA_MF_ENLARGE:
		err = drime4_set_MF_enlarge(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
#if 0
		state->real_preview_width = ((u32)ctrl->value >> 20) & 0xFFF;
		state->real_preview_height = ((u32)ctrl->value >> 8) & 0xFFF;
#endif
		err = drime4_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		if (ctrl->value) {
			state->focus.touch = 1;
		} else {
			err = drime4_set_af(sd, ctrl->value);
			state->focus.touch = 0;
		}
		break;

	case V4L2_CID_CAMERA_MF_ASSIST:
		err = drime4_set_mf_assist(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_PEAKING_LEVEL:
		err = drime4_set_focus_peaking_level(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_PEAKING_COLOR:
		err = drime4_set_focus_peaking_color(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_LINK_AE_TO_AF_POINT:
		err = drime4_set_link_AE_to_AF_point(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FRAMING_MODE:
		err = drime4_set_framing_mode(sd, ctrl->value);
		break;

	case V4L2_CID_THUMBNAIL_SIZE:
		err = drime4_set_thumbnail_size(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ZOOM:
		err = drime4_set_zoom(sd, ctrl->value);
		break;

	case V4L2_CID_CAM_JPEG_QUALITY:
		err = drime4_set_jpeg_quality(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_CAPTURE:
		err = drime4_start_capture(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_RAW_SSIF_CAPTURE:
		err = drime4_set_debug_raw_ssif_capture(sd, ctrl->value);
		break;

	case  V4L2_CID_FACTORY_RAW_PP_CAPTURE:
		err = drime4_set_debug_raw_pp_capture(sd, ctrl->value);
		break;

#ifdef FAST_CAPTURE
	case V4L2_CID_CAMERA_FAST_CAPTURE:
		err = drime4_set_fast_capture(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FAST_CAPTURE_STOP:
		err = drime4_stop_capture(sd, ctrl->value);
		break;
#endif

	case V4L2_CID_BURSTSHOT_PROC:
		err = drime4_burst_onoff(sd, ctrl->value);
		break;

	case V4L2_CID_BURSTSHOT_STOP:
		err = drime4_stop_capture(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CAPTURE_CNT:
		state->capture_cnt = ctrl->value;
		break;

	case V4L2_CID_MULTI_EXPOSURE_CNT:
		err = drime4_set_multi_exposure_max_cnt(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_HDR:
		state->hdr_mode = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_HYBRID:
		state->hybrid_mode = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_FAST_MODE:
		state->fast_mode = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_YUV_SNAPSHOT:
		state->yuv_snapshot = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_HYBRID_CAPTURE:
		err = drime4_set_hybrid_capture(sd);
		break;

	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		break;

	case V4L2_CID_CAMERA_LIVEVIEW_RESOLUTION:
		err = drime4_set_preview_size(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_JPEG_RESOLUTION:
		err = drime4_set_photo_size(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_MOVIE_RESOLUTION:
		err = drime4_set_movie_size(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK:
		err = drime4_aeawb_lock_unlock(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CAF_START_STOP:
		err = drime4_stop_af_lens(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_LOW_LIGHT_MODE:
		err = drime4_set_low_light_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_INIT:
		cam_trace("MANUAL INIT launched.");
		err = drime4_init(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_PASM_MODE:
		err = drime4_set_PASM_mode(sd, ctrl->value);
		break;

	case V4L2_CID_PROGRAM_SHIFT:
		err = drime4_set_program_shift(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_VIDEO_OUT_FORMAT:
		err = drime4_set_video_out_format(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WIDGET_MODE_LEVEL:
		err = drime4_set_smart_pro_level(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AF_LED:
		err = drime4_set_AF_LED(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SHUTTER_SPEED:
		err = drime4_set_shutter_speed(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_LDC:
		err = drime4_set_LDC(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_HIGH_ISO_NR:
		err = drime4_set_high_iso_nr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_LONG_TERM_NR:
		err = drime4_set_long_term_nr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_DMF:
		err = drime4_set_DMF(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_COLOR_SPACE:
		err = drime4_set_color_space(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_DYNAMIC_RANGE:
		err = drime4_set_dynamic_range(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_F_NUMBER:
	case V4L2_CID_CAM_APERTURE_CAPTURE:
	case V4L2_CID_CAM_APERTURE_PREVIEW:
		err = drime4_set_f_number(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_TIMER_LED:
		err = drime4_set_timer_LED(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_DRIVE_DIAL:
		err = drime4_set_Drive(sd, ctrl->value);
		break;

	case V4L2_CID_SYSTEM_ORIENT:
		err = drime4_set_exif_system_orient(sd, ctrl->value);
		break;

	case V4L2_CID_WEATHER_INFO:
		err = drime4_set_weather_info(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_MOVIE_FADER_MODE:
		err = drime4_set_movie_fader_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_MOVIE_FADEOUT:
		err = drime4_set_movie_fadeout(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SYSTEM_RESET_CWB:
		err = drime4_set_system_reset_cwb(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRACKET_AEB:
		err = drime4_set_AEB_step(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRACKET_WBB:
		err = drime4_set_WBB_step(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AF_PRIORITY:
		err = drime4_set_af_priority(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AF_FRAME_SIZE:
		err = drime4_set_af_frame_size(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_QUICKVIEW_CANCEL:
		err = drime4_quickview_cancel(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_i_FUNC_ENABLE:
		err = drime4_set_i_func_enable(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_QUICKVIEW:
		err = drime4_set_quickview(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO_STEP:
		err = drime4_set_iso_step(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO_AUTO_MAX_LEVEL:
		err = drime4_set_auto_iso_range(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_OVER_EXPOSURE_GUIDE:
		err = drime4_set_over_exposure_guide(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_NO_LENS_CAPTURE:
		err = drime4_set_no_lens_capture(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_SHUTTERSPPED:
		err = drime4_factory_shutterspeed(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACTORY_TEST_NUMBER:
		err = drime4_factory_mode_set(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACTORY_AF_POSITION:
		err = drime4_factory_focus_position(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACTORY_AF_STEP_SET:
		state->af_step = ctrl->value;
		break;

	case V4L2_CID_CAMERA_POWER_OFF:
		err = drime4_noti_disable(sd, ctrl->value);
		if (ctrl->value) {
			state->hal_power_off = 200;
			state->isp.d4_issued = 1;
			wake_up(&state->isp.d4_wait);
		}
		break;

	case V4L2_CID_CAMERA_SAMSUNG_APP:
		state->samsung_app = ctrl->value;
		break;

	case V4L2_CID_FACTORY_CIS_DATA:
		err = drime4_factory_cis_data(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_LENS_MOUNT:
		err = drime4_factory_lens_mount(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_HOT_SHOE:
		err = drime4_factory_hot_shoe(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_FLANGE_BACK:
		err = drime4_factory_flange_back(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_SEND_PC_DATA:
		err = drime4_factory_send_PC_data(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_DUST_REDUCTION:
		err = drime4_factory_dust_reduction(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_SHUTTER_X:
		err = drime4_factory_shutterX(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_LIVEVIEW_ISO_SENS:
		err = drime4_factory_liveview_iso_sens(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_CAPTURE_ISO_SENS:
		err = drime4_factory_capture_iso_sens(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_AWB:
		err = drime4_factory_AWB(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_VFPN:
		err = drime4_factory_VFPN(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACTORY_DEFECTPIXEL:
		err = drime4_factory_defect_pixel(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_COLORSHADING:
		err = drime4_factory_ColorShading(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_PAF:
		err = drime4_factory_PAF(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FACTORY_FLASH:
		err = drime4_factory_flash(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_FLASHROM:
		err = drime4_factory_FlashRom(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_GET_ISP_LOG:
		err = drime4_factory_get_isp_log(sd, ctrl->value);
		break;

	case V4L2_CID_FACTORY_LOG_WRITE:
		err = drime4_factory_log_write(sd, ctrl->value);
		break;

#ifdef BURST_BOOT_PARAM
	case V4L2_CID_CAMERA_BURST_BOOT_PARAM:
		err = drime4_set_burst_boot_param(sd, ctrl->value);
		break;
#endif

	case V4L2_CID_CAMERA_MIC_CONTROL:
		err = drime4_set_mic_control(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_INTERVAL_SHOT_MANUAL_FOCUS:
		err = drime4_set_mf_focus_position(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OPTICAL_PREVIEW:
		err = drime4_set_optical_preview(sd, ctrl->value);
		break;

	default:
		cam_err("no such control id %d, value %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
		/*err = -ENOIOCTLCMD;*/
		err = 0;
		break;
	}

	mutex_unlock(&state->drime4_lock);
	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d, value %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
	return err;
}

static int drime4_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	mutex_lock(&state->drime4_lock);
	switch (ctrl->id) {
	case V4L2_CID_CAMERA_CAPTURE:
		break;
#if 0
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		err = drime4_get_af_result(sd, ctrl);
		break;
#endif
	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = 0xC80000;
		break;

	case V4L2_CID_CAMERA_GET_LUX:
		err = drime4_get_lux(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SMART_READ1:
		err = drime4_get_smart_read1(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SMART_READ2:
		err = drime4_get_smart_read2(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_LENS_STATUS:
		err = drime4_get_lens_info(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_F_NUMBER:
		ctrl->value = state->smart_read[3] & 0x0FFFFFFFFLL;
		break;

	case V4L2_CID_CAMERA_FW_CHECKSUM_VAL:
		if (fw_update_result)
			ctrl->value = 1;
		else
			ctrl->value = 0;

	default:
		cam_err("no such control id %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE);
		/*err = -ENOIOCTLCMD*/
		err = 0;
		break;
	}

	mutex_unlock(&state->drime4_lock);
	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d\n", ctrl->id - V4L2_CID_PRIVATE_BASE);

	return err;
}

static int drime4_get_noti(struct v4l2_subdev *sd,
	struct v4l2_noti_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int int_factor;
	u8 read_val[300] = {0,};
	int noti_ID = 0;
	int err;
#if 0
	int i;
	int retry_cnt;
#endif
	int noti_gpio_val = 0;
	int Av_min, Av_max;
	__s64 af_result = 0;
	int isp_debug_len = 0;

#if 0
	cam_err("%s E\n", __func__);
#endif

#if 0
	retry_cnt = 3;
d4_retry:
#endif
	if (state->hal_power_off == 200) {
		cam_dbg("%s : hal_power_off = %d, d4_int not waiting", __func__,
				state->hal_power_off);
		ctrl->value = 200;
		return ctrl->value;
	} else if (state->hal_power_off == 0) {
		state->hal_power_off = 1;
#if 0
		ctrl->value = 103;
		return ctrl->value;
#endif
	}
	int_factor = drime4_wait_d4_interrupt(sd, DRIME4_NOTI_TIMEOUT);
	if (state->hal_power_off == 200) {
		cam_dbg("%s : d4_interrupt is forced woked up for power off.",
				__func__);
		ctrl->value = 200;
		return ctrl->value;
	}
	if (int_factor) {
#if 1
		noti_gpio_val = drime4_spi_get_noti_check_pin();
		if (!noti_gpio_val)
			cam_err("Notification gpio read failed.\n");
#endif
		err = drime4_spi_read_res(&read_val[0], 3, 0);	/* HSW */
		if (read_val[2] != DRIME4_PACKET_DSP_NOTI)
			cam_err("Notification data read failed. - %02x\n",
				read_val[2]);
		else {
			cam_dbg("Notification ID = %02x%02x\n",
				read_val[4], read_val[3]);
			noti_ID = ((read_val[4] & 0xFF) << 8) + \
				(read_val[3] & 0xFF);
		}
	} else {
#if 1
		ctrl->value = 0;
		return ctrl->value;
#else
		retry_cnt--;
		if (retry_cnt <= 0) {
			ctrl->value = 0;
			return ctrl->value;
		} else {
			goto d4_retry;
		}
#endif
	}

	switch (noti_ID) {
	case DRIME4_NOTI_CAPTURE_START:
		ctrl->value = 1;
		ctrl->value64 = 1;
		break;

	case DRIME4_NOTI_IMG_READY:
		ctrl->value = 4;
		break;

	case DRIME4_NOTI_NEXT_SHOT_READY:
		ctrl->value = 5;
		break;

	case DRIME4_NOTI_PROCESSING:
		ctrl->value = 7;
		break;

	case DRIME4_NOTI_LIVEVIEW_START_DONE:
		ctrl->value = 8;
		ctrl->value64 = 1;
		/* noti callback 9992(noti CB) after liveview start done */
		if (state->factory_running)
			ctrl->value64 = 2;
		break;

	case DRIME4_NOTI_LIVEVIEW_STOT_DONE:
		ctrl->value = 8;
		ctrl->value64 = 0;
		break;

	case DRIME4_NOTI_AF_SCAN_DONE:
		/* multi af peak 5 */
		af_result |= (af_result | read_val[6]) << 56;

		/* multi af peak 4 */
		af_result |= (af_result | read_val[7]) << 48;

		/* multi af peak 3 */
		af_result |= (af_result | read_val[8]) << 40;

		/* multi af peak 2 */
		af_result |= (af_result | read_val[9]) << 32;

		/* multi af peak 1 */
		af_result |= ((af_result & 0xFF000000) | read_val[10]) << 24;

		/* af extend area */
		af_result |= ((af_result & 0xFF0000) | read_val[14]) << 16;

		/* af area mode */
		af_result |= ((af_result & 0xFF00) | read_val[17]) << 8;

		/* af result */
		af_result |= (af_result & 0xFF) | read_val[16];

		ctrl->value = 10;
		ctrl->value64 = af_result;
		break;

	case DRIME4_NOTI_3A_LOCK_DONE:
		state->done_3a_lock = 1;
		ctrl->value = 11;
		break;

	case DRIME4_NOTI_3A_UNLOCK_DONE:
		state->done_3a_lock = 0;
		ctrl->value = 12;
		break;

	case DRIME4_NOTI_TRACKING_OBJECT_AF_LOCK:
		ctrl->value = 13;
		ctrl->value64 = 1;
		break;

	case DRIME4_NOTI_TRACKING_OBJECT_AF_FAIL:
		ctrl->value = 13;
		ctrl->value64 = 0;
		break;

	case DRIME4_NOTI_TRACKING_OBJECT_AF_UNLOCK:
		ctrl->value = 13;
		ctrl->value64 = 2;
		break;

	case DRIME4_NOTI_CAF_RESTART_DONE:
		ctrl->value = 14;
		break;

	case DRIME4_NOTI_ISP_DEBUG_STRING:
		isp_debug_len = read_val[5];
		cam_err("Noti error[%d] : %s",
			isp_debug_len, &read_val[6]);
		break;

	case DRIME4_NOTI_CIS_CLEAN_DONE:
		ctrl->value = 20;
		break;

	case DRIME4_NOTI_STROBO_OPEN:
		ctrl->value = 21;
		break;

	case DRIME4_NOTI_STROBO_CLOSE:
		ctrl->value = 22;
		break;

	case DRIME4_NOTI_STROBO_CHARGE_DONE:
		if (read_val[6])
			ctrl->value = 23;
		else
			ctrl->value = 24;
		break;

	case DRIME4_NOTI_EXT_STROBO_ATTACH:
		if (read_val[6])
			ctrl->value = 33;
		else
			ctrl->value = 31;
		break;

	case DRIME4_NOTI_EXT_STROBO_DETACH:
		ctrl->value = 32;
		break;

	case DRIME4_NOTI_MOVIE_FADEOUT_DONE:
		ctrl->value = 40;
		ctrl->value64 = read_val[6];
		cam_err("Fade-Out Done = 0x%02x", read_val[6]);
		break;

	case DRIME4_NOTI_3D_MOVIE_LOW_LIGHT:
		ctrl->value = 41;
		break;

	case DRIME4_NOTI_EXT_MIC_ATTACH:
		/*ctrl->value = 41;*/
		drime4_set_ext_mic_status(sd, 1);
		break;

	case DRIME4_NOTI_EXT_MIC_DETACH:
		/*ctrl->value = 42;*/
		drime4_set_ext_mic_status(sd, 0);
		break;

	case DRIME4_NOTI_LENS_DETECT:
		if (read_val[6])
			ctrl->value = 101;
		else {
			ctrl->value = 102;
			memset(sysfs_lens_fw, 0, sizeof(sysfs_lens_fw));
		}
		break;

	case DRIME4_NOTI_LENS_READY:
		ctrl->value = 103;
		break;

	case DRIME4_NOTI_LENS_K_MOUNT:
		ctrl->value = 109;
		break;

	case DRIME4_NOTI_LENS_NO_LENS:
		ctrl->value = 102;
		break;

	case DRIME4_NOTI_LENS_FATAL_ERR:
		ctrl->value = 104;
		break;

	case DRIME4_NOTI_LENS_COMMU_ERR:
		ctrl->value = 105;
		break;

	case DRIME4_NOTI_LENS_LOCK:
		if (read_val[6])
			ctrl->value = 106;
		else
			ctrl->value = 107;
		break;

	case DRIME4_NOTI_F_NUM_RANGE:
		ctrl->value = 108;
		if (read_val[5] > 4) {
			Av_max = drime4_find_f_number(sd,
			((read_val[9] << 8) | read_val[8]) & 0x0FFFF,
			((read_val[7] << 8) | read_val[6]) & 0x0FFFF) & 0x3FF;
			Av_min = drime4_find_f_number(sd,
			((read_val[13] << 8) | read_val[12]) & 0x0FFFF,
			((read_val[11] << 8) | read_val[10]) & 0x0FFFF) & 0x3FF;
		} else {
			Av_max = drime4_find_f_number(sd,
				read_val[7], read_val[6]) & 0x3FF;
			Av_min = drime4_find_f_number(sd,
				read_val[9], read_val[8]) & 0x3FF;
		}

#ifdef DRIME4_F_NUMBER_VALUE
		ctrl->value64 = ((Av_min << 10) | Av_max) & 0xFFFFFLL;
#else
		ctrl->value64 = ((Av_min << 6) | Av_max) & 0xFFF;
#endif
		break;

	case DRIME4_NOTI_I_FUNCTION_KEY:
		ctrl->value = 110;
		break;

	case DRIME4_NOTI_MF_SWITCH:
		if (read_val[6])
			ctrl->value = 111;
		else
			ctrl->value = 112;
		break;

	case DRIME4_NOTI_OIS_SWITCH:
		if (read_val[6])
			ctrl->value = 113;
		else
			ctrl->value = 114;
		break;

	case DRIME4_NOTI_3D_SWITCH:
		if (read_val[6])
			ctrl->value = 115;
		else
			ctrl->value = 116;
		break;

	case DRIME4_NOTI_QUICKVIEW_DONE:
		ctrl->value = 117;
		break;

	case DRIME4_NOTI_LIVEVIEW_RESTART:
		state->done_3a_lock = 0;
		ctrl->value = 118;
		break;

	case DRIME4_NOTI_FOCUS_POS:
		ctrl->value = 121;
		ctrl->value64 = read_val[6] & 0xFF;
		ctrl->value64 |= (read_val[7] << 8) & 0xFFFF;
		ctrl->value64 |= (read_val[8] << 16) & 0xFFFFFF;
		ctrl->value64 |= (read_val[9] << 24) & 0xFFFFFFFF;
		break;

	case DRIME4_NOTI_MF_OPERATION:
		ctrl->value = 122;
		ctrl->value64 = read_val[6] & 0x1;
		break;

	case DRIME4_NOTI_LENS_FW_UPDATE_PROGRESSING:
		ctrl->value = 123;
		ctrl->value64 = read_val[6];
		break;

	case DRIME4_NOTI_LENS_MODE_CHANGE_DONE:
		ctrl->value = 119;
		break;

	case 0xAA01:
		if (!state->factory_log_start) {
			if (read_val[6])
				ctrl->value = 9997;
			else
				ctrl->value = 9998;
		} else {
			cam_dbg("~~~ log write ready !!! ~~~\n");
			ctrl->value = 9988;
		}
		break;

	case 0xAA02:
		ctrl->value = 9999;
		if (read_val[7] == 0xFF)
			read_val[7] = 0x05;
		memcpy(state->OSD_data, &read_val[6], read_val[5]);
#if 0
		for (i = 0; i < read_val[5]; i++) {
			cam_trace("OSD_data buf[%d] = 0x%02x\n",
				i, state->OSD_data[i]);
		}
#endif
		break;

	case 0xAA03:
		cam_dbg("~~~ factory cmd start !!! ~~~\n");
		ctrl->value = 9994;
		break;

	default:
		cam_warn("invalid ID, %d\n", noti_ID);
	}

	cam_err("X %x, %04llx\n", ctrl->value, ctrl->value64);
	return ctrl->value;
}

static int drime4_noti_ctrl(struct v4l2_subdev *sd,
	struct v4l2_noti_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
#if 0
	cam_err("noti_ctrl is called %d\n",
			ctrl->id - V4L2_CID_PRIVATE_BASE);
#endif

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_NOTI:
#if 0	/* HSW */
		if (!drime4_spi_get_write_status())
			drime4_spi_set_noti_ctrl_pin(1);
#endif
		state->noti_ctrl_status = 1;
		err = drime4_get_noti(sd, ctrl);
		state->noti_ctrl_status = 0;
#if 0	/* HSW */
		drime4_spi_set_noti_ctrl_pin(0);
#endif
		break;

	default:
		cam_err("no such control id %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE);
		/*err = -ENOIOCTLCMD*/
		err = 0;
		break;
	}

	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d\n", ctrl->id - V4L2_CID_PRIVATE_BASE);

	return err;
}


static int drime4_g_ext_ctrl(struct v4l2_subdev *sd,
	struct v4l2_ext_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	char *buf = NULL;

	switch (ctrl->id) {
	case V4L2_CID_CAM_SENSOR_FW_VER:
		strcpy(ctrl->string, state->phone_fw);
		break;

	case V4L2_CID_NOTI_DATA_GET:
		state->OSD_length = strlen(state->OSD_data);

		buf = kmalloc(ctrl->size, GFP_KERNEL);

		memcpy(buf, &state->OSD_data[0], state->OSD_length);

		if (copy_to_user((void __user *)ctrl->string,
					buf, state->OSD_length))
			err = -1;

		kfree(buf);
		break;
#if 1
	case V4L2_CID_CAMERA_DISTANCE_SCALE:
		mutex_lock(&state->drime4_lock);
		drime4_get_distance_scale(sd, ctrl);
		mutex_unlock(&state->drime4_lock);

		state->distance_scale_length =
			sizeof(state->distance_scale_data)
			/sizeof(*state->distance_scale_data);

		buf = kmalloc(ctrl->size, GFP_KERNEL);

		memcpy(buf, &state->distance_scale_data[0],
			state->distance_scale_length);

		if (copy_to_user((void __user *)ctrl->string,
					buf, state->distance_scale_length))
			err = -1;

		kfree(buf);
		break;
#endif
	default:
		cam_err("no such control id %d\n",
			ctrl->id - V4L2_CID_CAMERA_CLASS_BASE);
		/*err = -ENOIOCTLCMD*/
		break;
	}

	return err;
}

static int drime4_g_ext_ctrls(struct v4l2_subdev *sd,
	struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int i, err = 0;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		err = drime4_g_ext_ctrl(sd, ctrl);
		if (err) {
			ctrls->error_idx = i;
			break;
		}
	}
	return err;
}

static int drime4_s_ext_ctrl(struct v4l2_subdev *sd,
	struct v4l2_ext_control *ctrl)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	int size = 0;
	char *buf = NULL;
	int i = 0;

	size = ctrl->size;

	cam_dbg("E %s, size : %d\n", ctrl->string, size);

	switch (ctrl->id) {
	case V4L2_CID_FACTORY_FW_CHECK:
		buf = kmalloc(ctrl->size, GFP_KERNEL);
		if (copy_from_user(buf,  (void __user *)ctrl->string, size)) {
			err = -1;
			break;
		}

		for (i = 0; i < size; i++)
			cam_dbg("buf[%d] = %c\n", i, buf[i]);

		mutex_lock(&state->drime4_lock);
		err = drime4_factory_fw_check(sd, buf);
		mutex_unlock(&state->drime4_lock);

		kfree(buf);
		break;

	case V4L2_CID_FACTORY_D4_CMD:
		buf = kmalloc(ctrl->size+1, GFP_KERNEL);
		if (copy_from_user(buf,  (void __user *)ctrl->string, size)) {
			err = -1;
			break;
		}

		buf[size] = 0x00;

		mutex_lock(&state->drime4_lock);
		err = drime4_factory_D4_cmd(sd, buf, size+1);
		mutex_unlock(&state->drime4_lock);

		kfree(buf);
		break;

	case V4L2_CID_EXIF_TIME_MODEL:
		buf = kmalloc(ctrl->size+1, GFP_KERNEL);
		if (copy_from_user(buf,  (void __user *)ctrl->string, size)) {
			err = -1;
			break;
		}

		mutex_lock(&state->drime4_lock);
		err = drime4_set_exif_time_model(sd, buf, size);
		mutex_unlock(&state->drime4_lock);

		kfree(buf);
		break;

	case V4L2_CID_EXIF_GPS:
		buf = kmalloc(ctrl->size+1, GFP_KERNEL);
		if (copy_from_user(buf,  (void __user *)ctrl->string, size)) {
			err = -1;
			break;
		}

		mutex_lock(&state->drime4_lock);
		err = drime4_set_exif_gps(sd, buf, size);
		mutex_unlock(&state->drime4_lock);

		kfree(buf);
		break;

	default:
		cam_err("no such control id %d\n",
			ctrl->id - V4L2_CID_CAMERA_CLASS_BASE);
		/*err = -ENOIOCTLCMD*/
		break;
	}

	return err;
}

static int drime4_s_ext_ctrls(struct v4l2_subdev *sd,
	struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int i, err = 0;

	cam_dbg("E\n");

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		err = drime4_s_ext_ctrl(sd, ctrl);
		if (err) {
			ctrls->error_idx = i;
			break;
		}
	}
	return err;
}

static int drime4_load_fw(struct v4l2_subdev *sd)
{
	struct device *dev = sd->v4l2_dev->dev;
	struct drime4_state *state = to_state(sd);
	const struct firmware *fw;
	char fw_path[20] = {0,};
	char fw_path_in_data[25] = {0,};
	u8 *buf = NULL;
	int err = 0;
	int txSize = 0;
	u8 read_val[512] = {0,};
	u32 int_factor;
	int retry_cnt = 3;
	int buf1_count = 0, buf2_size = 0, lseek_offset = 0, j = 0;

	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize = 0, nread;

	cam_dbg("E\n");

	fw_update_result = 0;
	if (mmc_fw == 1)
		sprintf(fw_path_in_data, DRIME4_FW_PATH);
	else
		sprintf(fw_path_in_data, DRIME4_FW_MMC_PATH);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	if (state->fw_index == DRIME4_SD_CARD ||
		state->fw_index == DRIME4_IN_DATA) {

		if (state->fw_index == DRIME4_SD_CARD) {
			if (mmc_fw == 1)
				fp = filp_open(DRIME4_FW_PATH, O_RDONLY, 0);
			else
				fp = filp_open(DRIME4_FW_MMC_PATH, O_RDONLY, 0);
		} else
			fp = filp_open(fw_path_in_data, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			err = -ENOENT;
			goto out;
		} else {
			if (mmc_fw == 1)
				cam_dbg("%s is opened\n",
					state->fw_index == DRIME4_SD_CARD ?
					DRIME4_FW_PATH : fw_path_in_data);
			else
				cam_dbg("%s is opened\n",
					state->fw_index == DRIME4_SD_CARD ?
					DRIME4_FW_MMC_PATH : fw_path_in_data);
		}

		if (mmc_fw == 1)
			fsize = fp->f_path.dentry->d_inode->i_size;
		else
			fsize = FW_BIN_SIZE;

		cam_dbg("size %ld Bytes\n", fsize);

		if (fsize >= DRIME4_FW_BUF_SIZE)
			buf1_count = fsize / DRIME4_FW_BUF_SIZE;

		buf2_size = fsize % DRIME4_FW_BUF_SIZE;
	} else {
		set_fs(old_fs);
		err = request_firmware(&fw, fw_path, dev);
		if (err != 0) {
			cam_err("request_firmware falied\n");
			err = -EINVAL;
			goto out;
		}

		cam_dbg("start, size %d Bytes\n", fw->size);
		buf = (u8 *)fw->data;
		fsize = fw->size;
	}

	txSize = 60*1024; /*60KB*/

	/* firmware update start */
	err = drime4_cmdl(sd, DRIME4_PACKET_FWUP_START, fsize);
	CHECK_ERR(err);

	/* ISP ready wait */
	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_BOOT_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (err < 0)
			cam_err("drime4_spi_read falied\n");
		if (read_val[2] != DRIME4_PACKET_FWUP_READY)
			cam_err("fw update packet ID falied -0x%02x\n",
					read_val[2]);
		if (read_val[3] != 0x00)
			cam_err("fw update start falied -%d\n", retry_cnt);
	} else {
		err = -EIO;
		goto out;
	}

	if (buf1_count) {
		for (j = 0; j < buf1_count; j++) {
			err = vfs_llseek(fp, lseek_offset, SEEK_SET);
			if (err < 0) {
				cam_err("failed to fseek, %d\n", err);
				goto out;
			}

			lseek_offset += DRIME4_FW_BUF_SIZE;

			nread = vfs_read(fp, (char __user *)data_memory,
				DRIME4_FW_BUF_SIZE, &fp->f_pos);
			if (nread != DRIME4_FW_BUF_SIZE) {
				cam_err("failed to read firmware file_%d\n", j);
				err = -EIO;
				goto out;
			}

			err = drime4_spi_write_burst(DRIME4_PACKET_FWUP_DATA,
				DRIME4_FW_BUF_SIZE, &data_memory[0], 0);
			CHECK_ERR(err);
			cam_dbg("writing... %d / %d\n", j+1, buf1_count);
		}
	}

	if (buf2_size) {
		if (buf1_count == 0)
			lseek_offset = 0;

		err = vfs_llseek(fp, lseek_offset, SEEK_SET);
		if (err < 0) {
			cam_err("failed to fseek, %d\n", err);
			goto out;
		}

		nread = vfs_read(fp, (char __user *)data_memory,
			buf2_size, &fp->f_pos);
		if (nread != buf2_size) {
			cam_err("failed to read firmware file_2\n");
			err = -EIO;
			goto out;
		}

		/* send firmware */
		err = drime4_spi_write_burst(DRIME4_PACKET_FWUP_DATA,
			buf2_size, &data_memory[0], 0);
		CHECK_ERR(err);
	}

	cam_dbg("writing... end\n");

	set_fs(old_fs);

	/* send data complete */
	err = drime4_cmdb(sd, DRIME4_PACKET_FWUP_DATA_COM, 0x00);
	CHECK_ERR(err);

	/* ISP recv wait */
	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_BOOT_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (read_val[3] != 0x00)
			cam_err("fw send data complete falied -%d\n",
				retry_cnt);
	} else {
		err = -EIO;
		goto out;
	}

	/* ISP update complete */
	int_factor = drime4_wait_ap_interrupt(sd, 2*60*1000);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (read_val[3] != 0x00)
			cam_err("fw update complete falied -%d\n",
				retry_cnt);
		msleep(300);
		err = drime4_init(sd, 1);
		fw_update_result = 1;
#if 0
	} else {
		err = -EIO;
		goto out;
#endif
	}

out:
	if (state->fw_index == DRIME4_SD_CARD ||
		state->fw_index == DRIME4_IN_DATA) {
		if (!IS_ERR(fp) && fp != NULL)
			filp_close(fp, current->files);

		set_fs(old_fs);
	} else {
		release_firmware(fw);
	}

	mmc_fw = 0;

	cam_trace("X\n");

	return err;
}

static int drime4_check_lens_fw(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	char fw_lens_path[30] = {0,};
	char fw_model_lens_body[26] = {0, };
	char fw_model_binary[20] = {0, };
	char fw_ver_binary[20] = {0, };
	int int_factor;
	int err;
	int result = 0;

#if 0
	return 0;
#endif

#if 1
	/* get Model name from Lens Body */
	err = drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_LENS_MODEL);
	cam_dbg("err value = %d\n", err);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_TIMEOUT);
	if (int_factor) {
		err = drime4_spi_read_res(&fw_model_lens_body[0], 0, 0);
		cam_dbg("err value : %d\n", err);
		CHECK_ERR(err);

		if (fw_model_lens_body[2] != DRIME4_PACKET_DSP_QUERY_RES) {
			cam_err("wirte failed: packet ID err - w:0x%02x r:0x%02x\n",
			DRIME4_PACKET_DSP_QUERY_RES,
			fw_model_lens_body[2]);
			return err;
		}
		cam_dbg("drime4_write res [%d] packet 0x%02x, ID 0x%02x%02x\n",
			fw_model_lens_body[5], fw_model_lens_body[2],
			fw_model_lens_body[4], fw_model_lens_body[3]);
	}
#if 0
	/* Packet error check */
	if (fw_model_lens_body[0] == 0) {
		cam_dbg("error value\n");
		return err;
		}
#endif
	/* here */
	cam_dbg("Lens Model(body) = %s\n", &fw_model_lens_body[6]);

#endif
	/* get Model name from Binary */
	memset(sysfs_lens_binary_fw, 0, sizeof(sysfs_lens_binary_fw));
	sprintf(fw_lens_path, DRIME4_LENSFW_PATH);
	result = drime4_open_firmware_file(sd,
		fw_lens_path,
		fw_model_binary,
		DRIME4_LENS_MODEL_OFFSET,
		DRIME4_LENS_MODEL_LEN);
	cam_dbg("Lens Model(binary_sdcard) = %s\n", fw_model_binary);
	state->lens_fw_path = DRIME4_LENS_SD_CARD;

	/* None-Binary File : No Write */
	if (0 != result) {
		/* get Model name from Binary */
		sprintf(fw_lens_path, DRIME4_LENSFW_PATH_EXT);
		result = drime4_open_firmware_file(sd,
			fw_lens_path,
			fw_model_binary,
			DRIME4_LENS_MODEL_OFFSET,
			DRIME4_LENS_MODEL_LEN);
		cam_dbg("Lens Model(binary_ext) = %s\n", fw_model_binary);
		state->lens_fw_path = DRIME4_LENS_EXT_SD_CARD;

		if (0 != result) {
			cam_dbg("Binary file open failed!\n");
			return 0;
		}
	}
#if 1
	/* Model Name Compare */
	result = strcmp(&fw_model_lens_body[6], fw_model_binary);
	cam_dbg("compare result value = %d\n", result);
	if (0 != result) {
		cam_dbg("model name discord!\n");
		/*memcpy(sysfs_lens_binary_fw, " ", 1);*/
		return 0;
	}
#endif

	/* get Model version from Binary */
	drime4_open_firmware_file(sd,
		fw_lens_path,
		fw_ver_binary,
		DRIME4_LENS_FW_VER_OFFSET,
		DRIME4_LENS_FW_VER_LEN);

	if (0 != result) {
		cam_dbg("Binary file open failed!!\n");
		return 0;
	}

	/* get Model version from Lens Body */
	memcpy(sysfs_lens_binary_fw, fw_ver_binary, sizeof(fw_ver_binary));
	cam_dbg("sysfs_lens_binary_fw = %s\n", sysfs_lens_binary_fw);

	return 0;
}

static int drime4_load_lens_fw(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	char fw_lens_path[20] = {0,};
	u8 read_val[512] = {0,};
	int err = 0;
	int int_factor;
	int buf1_count = 0;
	int buf2_size = 0;
	int lseek_offset = 0;
	int retry_cnt = 0;
	int j;
	long fsize;
	long nread;
	int noti_lock = 0;

	struct file *fp = NULL;
	mm_segment_t old_fs;

#if 0
	return 0;
#endif

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	cam_dbg("lens_fw_path = %d", state->lens_fw_path);
	if (state->lens_fw_path == DRIME4_LENS_SD_CARD)
		sprintf(fw_lens_path, DRIME4_LENSFW_PATH);
	else
		sprintf(fw_lens_path, DRIME4_LENSFW_PATH_EXT);

	fp = filp_open(fw_lens_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		err = -ENOENT;
		goto out;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	cam_dbg("size %ld Bytes\n", fsize);

	if (fsize >= (DRIME4_FW_BUF_SIZE))
		buf1_count = fsize / (DRIME4_FW_BUF_SIZE);
	buf2_size = fsize % (DRIME4_FW_BUF_SIZE);
	cam_dbg("buf1_count = [%d], buf2_size = [%d]\n", buf1_count, buf2_size);

	err = drime4_cmdl(sd, DRIME4_PACKET_LENS_FWUP_START, fsize);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd,
		DRIME4_ISP_BOOT_TIMEOUT);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], noti_lock, 1);
		if (err < 0)
			cam_err("drime4_spi_read falied\n");
		if (read_val[2] != DRIME4_PACKET_LENS_FWUP_READY)
			cam_err("fw update packet ID falied -0x%02x\n",
					read_val[2]);
		if (read_val[3] != 0x00)
			cam_err("fw update start falied -%d\n", retry_cnt);
	} else {
		err = -EIO;
		goto out;
	}

	if (buf1_count) {
		for (j = 0; j < buf1_count; j++) {
			err = vfs_llseek(fp, lseek_offset, SEEK_SET);
			if (err < 0) {
				cam_err("failed to fseek, %d\n", err);
				goto out;
			}

			lseek_offset += DRIME4_FW_BUF_SIZE;

			nread = vfs_read(fp, (char __user *)data_memory,
				DRIME4_FW_BUF_SIZE, &fp->f_pos);
			if (nread != DRIME4_FW_BUF_SIZE) {
				cam_err("failed to read firmware file_%d\n", j);
				err = -EIO;
				goto out;
			}
#if 0
			err = drime4_spi_write_burst(DRIME4_PACKET_FWUP_DATA,
				DRIME4_FW_BUF_SIZE, &data_memory[0], 1);
#else
/*send only payload(not add length, Packet ID, end, cs)*/
			cam_dbg("main DRIME4_FW_BUF_SIZE = %d\n",
				DRIME4_FW_BUF_SIZE);

			err = drime4_spi_write_burst_only_data(
				DRIME4_FW_BUF_SIZE, &data_memory[0], 1);
#endif
			CHECK_ERR(err);
			cam_dbg("writing... %d / %d\n", j+1, buf1_count);
		}
	}

	if (buf2_size) {
		if (buf1_count == 0)
			lseek_offset = 0;

		err = vfs_llseek(fp, lseek_offset, SEEK_SET);
		if (err < 0) {
			cam_err("failed to fseek, %d\n", err);
			goto out;
		}
		nread = vfs_read(fp, (char __user *)data_memory,
			buf2_size, &fp->f_pos);
		if (nread != buf2_size) {
			cam_err("failed to read firmware file_2\n");
			err = -EIO;
			goto out;
		}
#if 0
		err = drime4_spi_write_burst(DRIME4_PACKET_FWUP_DATA,
			buf2_size, &data_memory[0], 1);
#else
/*send only payload(not add length, Packet ID, end, cs)*/
		cam_dbg("extar buf2_size = %d\n", buf2_size);
		err = drime4_spi_write_burst_only_data(buf2_size,
			&data_memory[0], 1);
#endif
		CHECK_ERR(err);
		cam_dbg("writing extar done");
	}

	set_fs(old_fs);

	err = drime4_cmdb(sd, DRIME4_PACKET_FWUP_DATA_COM, 0x00);
	CHECK_ERR(err);
	cam_dbg(" err = [%d]\n", err);

	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_BOOT_TIMEOUT);
	cam_dbg("int_factor(one) = [%d]\n", int_factor);
	if (state->first_boot_param || !state->noti_ctrl_status)
		noti_lock = 1;
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], noti_lock, 1);
		if (read_val[3] != 0x00)
			cam_err("fw send data complete falied -%d\n",
				retry_cnt);
	} else {
		err = -EIO;
		goto out;
	}

	int_factor = drime4_wait_ap_interrupt(sd, 2*60*1000);
	cam_dbg("int_factor(two) = [%d]\n", int_factor);
	if (int_factor) {
		err = drime4_spi_read_res(&read_val[0], 0, 1);
		if (read_val[3] != 0x00)
			cam_err("fw update complete falied -%d\n",
				retry_cnt);
#if 0
	} else {
		err = -EIO;
		goto out;
#endif
	}

out:

	if (!IS_ERR(fp) && fp != NULL)
		filp_close(fp, current->files);
	set_fs(old_fs);

	cam_trace("X\n");

	return err;
}

/*
 * v4l2_subdev_video_ops
 */
static const struct drime4_frmsizeenum *drime4_get_frmsize
	(const struct drime4_frmsizeenum *frmsizes, int num_entries, int index)
{
	int i;

	for (i = 0; i < num_entries; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

static int drime4_set_frmsize(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	int err;
	int return_liveview = 0;
	cam_trace("E, %dx%d\n", state->preview->width,
		state->preview->height);

	if (state->isp_mode == DRIME4_LIVEVIEW_MODE)
		return_liveview = 1;

	err = drime4_set_mode(sd, DRIME4_LIVE_MOV_OFF);
	CHECK_ERR(err);

	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
		DRIME4_SET_LIVEVIEW_SIZE,
		state->preview->reg_val);
	CHECK_ERR(err);

	if (state->sensor_mode == SENSOR_MOVIE) {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_MOVIE_SIZE, state->movie->reg_val);
		CHECK_ERR(err);
	}

	if (return_liveview == 1) {
		err = drime4_set_mode(sd, DRIME4_LIVEVIEW_MODE);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_s_fmt(struct v4l2_subdev *sd,
	struct v4l2_mbus_framefmt *ffmt)
{
	struct drime4_state *state = to_state(sd);
	const struct drime4_frmsizeenum **frmsize;

	u32 width = ffmt->width;
	u32 height = ffmt->height;
#if 0
	u32 tmp_width;
#endif
	u32 old_index;
	int i, num_entries;
	cam_dbg("E\n");

	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}
#if 0
	if (ffmt->width < ffmt->height) {
		tmp_width = ffmt->height;
		height = ffmt->width;
		width = tmp_width;
	}
#endif

#if 0
	if (ffmt->colorspace == V4L2_COLORSPACE_JPEG)
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;
	else
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
#endif

	state->format_mode = ffmt->field;
	state->pixelformat = ffmt->colorspace;

	/*set frame size for preview(yuv)*/
	if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
		cam_dbg("FMT_MODE_PREVIEW\n");
		frmsize = &state->preview;
	} else {
		cam_dbg("FMT_MODE_CAPTURE\n");
		frmsize = &state->capture;
	}

	old_index = *frmsize ? (*frmsize)->index : -1;
	*frmsize = NULL;

	if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
		cam_dbg("find preview frmsize %dx%d\n", width, height);
		num_entries = ARRAY_SIZE(preview_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width == preview_frmsizes[i].width &&
				height == preview_frmsizes[i].height) {
				*frmsize = &preview_frmsizes[i];
				break;
			}
		}

		if (*frmsize == NULL || state->sensor_mode == SENSOR_MOVIE) {
			cam_dbg("searching List of movie sizes\n");
			num_entries = ARRAY_SIZE(movie_frmsizes);
			for (i = 0; i < num_entries; i++) {
				if (width == movie_frmsizes[i].width &&
					height == movie_frmsizes[i].height) {
					*frmsize = &movie_frmsizes[i];
					state->movie = &movie_frmsizes[i];
					break;
				}
			}
		}
	} else {
		cam_dbg("find capture frmsize\n");
		num_entries = ARRAY_SIZE(capture_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width == capture_frmsizes[i].width &&
				height == capture_frmsizes[i].height) {
				*frmsize = &capture_frmsizes[i];
				break;
			}
		}
	}

	if (*frmsize == NULL) {
		cam_warn("invalid frame size %dx%d\n", width, height);
		if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW)
			*frmsize = drime4_get_frmsize(preview_frmsizes,
					num_entries, DRIME4_PREVIEW_720P);
		else if (state->pixelformat == V4L2_COLORSPACE_JPEG)
			*frmsize = drime4_get_frmsize(capture_frmsizes,
					num_entries, 0x01);
	}

	cam_dbg("s_fmt size %dx%d\n", (*frmsize)->width, (*frmsize)->height);
	mutex_lock(&state->drime4_lock);
	drime4_set_frmsize(sd);
	mutex_unlock(&state->drime4_lock);

	cam_trace("X\n");
	return 0;
}

static int drime4_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct drime4_state *state = to_state(sd);

	a->parm.capture.timeperframe.numerator = 1;
	a->parm.capture.timeperframe.denominator = state->fps;

	return 0;
}

static int drime4_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct drime4_state *state = to_state(sd);
#if 0
	int err = 0;

	u32 fps = a->parm.capture.timeperframe.denominator /
					a->parm.capture.timeperframe.numerator;
#endif

	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}
#if 0
	if (fps != state->fps) {
		if (fps <= 0 || fps > 30) {
			cam_err("invalid frame rate %d\n", fps);
			fps = 30;
		}
		state->fps = fps;
	}
	cam_err("Frame rate = %d(%d)\n", fps, state->fps);

	err = drime4_set_frame_rate(sd, state->fps);
	CHECK_ERR(err);
#endif

	return 0;
}

static int drime4_enum_framesizes(struct v4l2_subdev *sd,
	struct v4l2_frmsizeenum *fsize)
{
	struct drime4_state *state = to_state(sd);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
#if 0
	if (state->preview == NULL)
		return -EINVAL;
#endif

	if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
		if (state->preview == NULL
				/* FIXME || state->preview->index < 0 */)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview->width;
		fsize->discrete.height = state->preview->height;
	} else {
		if (state->capture == NULL
				/* FIXME || state->capture->index < 0 */)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture->width;
		fsize->discrete.height = state->capture->height;
	}

#if 0
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	if (state->hdr_mode || state->yuv_snapshot) {
		fsize->discrete.width = state->capture->width;
		fsize->discrete.height = state->capture->height;
	} else {
		fsize->discrete.width = state->preview->width;
		fsize->discrete.height = state->preview->height;
	}
#endif
	return 0;
}

static int drime4_s_stream_sensor(struct v4l2_subdev *sd, int onoff)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;

	cam_info("onoff=%d\n", onoff);

	return err;
}

#if 0
static int drime4_s_stream_hdr(struct v4l2_subdev *sd, int enable)
{
	struct drime4_state *state = to_state(sd);
	int err = 0;
	cam_trace("E\n");

	cam_trace("X\n");
	return 0;
}
#endif

static int drime4_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct drime4_state *state = to_state(sd);
	int err;

	cam_trace("E stream %s\n", enable == STREAM_MODE_CAM_ON ?
			"on" : "off");

	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	mutex_lock(&state->drime4_lock);

	switch (enable) {
	case STREAM_MODE_CAM_ON:
		switch (state->format_mode) {
		case V4L2_PIX_FMT_MODE_CAPTURE:
			err = drime4_s_stream_sensor(sd, enable);
			if (err < 0) {
				cam_err("drime4 s_stream failed\n");
				goto stream_out;
			}
			break;
		default:
		if (state->sensor_mode == SENSOR_MOVIE) {
			if (state->PASM_mode == MODE_GOLF_SHOT &&
				state->capturingGolfShot == true) {
				err = drime4_set_mode(sd, DRIME4_LIVEVIEW_MODE);
				if (err < 0) {
					cam_err("drime4 set_mode failed\n");
					goto stream_out;
				}
				/* StreamOn For GolfShot */
				err = drime4_writeb_res(sd,
					DRIME4_PACKET_DSP_CMD,
					DRIME4_CMD_CAPTURE,
					DRIME4_CAP_MODE_SMART);
				CHECK_ERR(err);
			} else {
				err = drime4_set_mode(sd, DRIME4_MOVIE_MODE);
				if (err < 0) {
					cam_err("drime4 set_mode failed\n");
					goto stream_out;
				}
			}
		} else {
			err = drime4_set_mode(sd, DRIME4_LIVEVIEW_MODE);
			if (err < 0) {
				cam_err("drime4 set_mode failed\n");
				goto stream_out;
			}
		}
		break;
		}
		break;

	case STREAM_MODE_CAM_OFF:
#if 1
#if 0 /* golf shot test */
		if (state->PASM_mode == MODE_GOLF_SHOT
			&& state->capturingGolfShot == true) {
			err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_CAPTURE_STOP, 0x00);
			CHECK_ERR(err);
			state->capturingGolfShot = false;
		}
#endif
		err = drime4_set_mode(sd, DRIME4_LIVE_MOV_OFF);
		if (err < 0) {
			cam_err("drime4 set_mode failed\n");
			goto stream_out;
		}
#endif
		break;

	default:
		cam_err("invalid stream option, %d\n", enable);
		break;
	}

stream_out:
	mutex_unlock(&state->drime4_lock);

#if 0
	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	switch (enable) {
	case STREAM_MODE_CAM_ON:
	case STREAM_MODE_CAM_OFF:
		switch (state->format_mode) {
		case V4L2_PIX_FMT_MODE_CAPTURE:
			cam_info("capture %s",
				enable == STREAM_MODE_CAM_ON ? "on" : "off");

			drime4_s_stream_sensor(sd, enable);
			if (enable == STREAM_MODE_CAM_ON &&
				(state->focus.mode >=
					FOCUS_MODE_CONTINOUS &&
				state->focus.mode <=
					FOCUS_MODE_CONTINOUS_VIDEO)) {
				drime4_set_af_mode(sd,
					state->focus.mode);
			}
			break;

		default:
			cam_info("preview %s",
				enable == STREAM_MODE_CAM_ON ? "on" : "off");

			if (state->hdr_mode) {
				err = drime4_set_flash(sd, FLASH_MODE_OFF, 0);
				err = drime4_s_stream_hdr(sd, enable);
			} else {
				err = drime4_s_stream_sensor(sd, enable);
				if (enable == STREAM_MODE_CAM_ON &&
					(state->focus.mode >=
						FOCUS_MODE_CONTINOUS &&
					state->focus.mode <=
						FOCUS_MODE_CONTINOUS_VIDEO)) {
					drime4_set_af_mode(sd,
						state->focus.mode);
				}
			}
			break;
		}
		break;

	case STREAM_MODE_MOVIE_ON:
		if (state->flash_mode != FLASH_MODE_OFF)
			err = drime4_set_flash(sd, state->flash_mode, 1);

		if (state->preview->index == DRIME4_PREVIEW_720P ||
				state->preview->index == DRIME4_PREVIEW_1080P)
			err = drime4_set_af(sd, 1);
		break;

	case STREAM_MODE_MOVIE_OFF:
		if (state->preview->index == DRIME4_PREVIEW_720P ||
				state->preview->index == DRIME4_PREVIEW_1080P)
			err = drime4_set_af(sd, 0);

		drime4_set_flash(sd, FLASH_MODE_OFF, 1);
		break;

	default:
		cam_err("invalid stream option, %d\n", enable);
		break;
	}

	state->stream_enable = enable;
	if (state->stream_enable && state->hdr_mode == 0) {
		if (state->fps)
			drime4_set_frame_rate(sd, state->fps);
	}
#endif

	cam_trace("X\n");
	return 0;
}

static int drime4_init_param(struct v4l2_subdev *sd)
{
	struct drime4_state *state = to_state(sd);
	int err;
	cam_dbg("E\n");

	if (!state->samsung_app) {
		/* Must Set JPEG Quality(default) for 3d party */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_QUALITY, 0x00);
		CHECK_ERR(err);

		/* Set default P Mode for 3d party */
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_SYSTEM_MODE, 2);
		CHECK_ERR(err);

		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_EXPERT_SUB_MODE, 0);
		CHECK_ERR(err);
	}

	cam_trace("X\n");
	return 0;
}

static int drime4_set_avs(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	const struct drime4_platform_data *pdata = client->dev.platform_data;

	int err = 0;
	int int_factor;
	u8 read_val[20] = {0,};
	int gpio_val = 0;
	int retry_cnt = 0;
	u8 avs_value;
	int noti_lock = 0;
	cam_dbg("E\n");

	err = drime4_cmdw(sd, DRIME4_PACKET_DSP_QUERY,
		DRIME4_QRY_CHECK_AVS);
	CHECK_ERR(err);

	int_factor = drime4_wait_ap_interrupt(sd, DRIME4_ISP_TIMEOUT);
	if (int_factor) {
#ifndef FEATURE_SPI_PROTOCOL_TRI_PIN
		noti_lock = 1;
#endif
		err = drime4_spi_read_res(&read_val[0], noti_lock, 0);
		if (err < 0) {
			cam_dbg("AVS response read fail\n");
		} else {
			cam_dbg("AVS Request Response == 0x%02x\n",
				read_val[6]);
			cam_dbg("AVS check : %s\n",
				read_val[6] == 0xaa ? "NO." : "YES");
		}
	} else {
		cam_dbg("AVS response read busy fail\n");
		return -1;
	}

	if (read_val[6] == 0xaa) {
#ifdef FEATURE_COMMAND_POWER_OFF
		cam_dbg("start power off command\n");
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
				DRIME4_CMD_POWER, 0x01);
		CHECK_ERR(err);
#else
		pdata->power_on_off(0);
#endif

		msleep(2500);
		/* AVS Test mode */
		avs_value = pdata->avs_test_mode();

boot_check_retry:
		gpio_val = gpio_get_value(GPIO_ISP_INT);
		if (!gpio_val && (retry_cnt < 300)) {
			msleep(50);
			retry_cnt++;
			cam_dbg("ISP Boot not complete retry again. gpio = %d\n",
					gpio_val);
			goto boot_check_retry;
		}
		if (!gpio_val) {
			cam_err("ISP Boot failed. gpio = %d\n", gpio_val);
			return -1;
		} else {
			cam_dbg("ISP Boot check complete. gpio =  %d\n",
					gpio_val);
			retry_cnt = 0;
		}

		/* Send AVS value to ISP */
		if (avs_value > 0x01 && avs_value < 0x0f) {
			cam_dbg("Send AVS value = 0x%02x\n", avs_value);
		} else {
			if (avs_value > 0x0e) {
				cam_err("AVS value 0x%02x : Malfunctioning Chip.\n",
					avs_value);
			} else {
				cam_err("AVS value 0x%02x : Invalid Value.\n",
					avs_value);
			}
		}
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_SET,
			DRIME4_SET_AVS_VALUE, avs_value);
		CHECK_ERR(err);
	}

	return err;
}
static int makeISPpinsLow(void)
{
	int ret = 0;

	gpio_free(GPIO_CAM_SPI_SSN);
	gpio_free(GPIO_CAM_SPI_MOSI);
	gpio_free(GPIO_CAM_SPI_MISO);
	if (system_rev > 7) {
		/*gpio_free(GPIO_CAM_SCL); */
		/* gpio_free(GPIO_CAM_SDA); */
	} else {
		/*gpio_free(GPIO_GSENSE_SCL_18V); */
		/* gpio_free(GPIO_GSENSE_SDA_18V); */
	}

	s3c_gpio_cfgpin(GPIO_CAM_SPI_SSN, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(GPIO_CAM_SPI_MOSI, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(GPIO_CAM_SPI_MISO, S3C_GPIO_OUTPUT);
	if (system_rev > 7) {
		s3c_gpio_cfgpin(GPIO_CAM_SCL, S3C_GPIO_OUTPUT);
		s3c_gpio_cfgpin(GPIO_CAM_SDA, S3C_GPIO_OUTPUT);
	} else {
		s3c_gpio_cfgpin(GPIO_GSENSE_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_cfgpin(GPIO_GSENSE_SDA_18V, S3C_GPIO_OUTPUT);
	}

	printk(KERN_DEBUG "%s  Start Set the CS, MOSI, MISO, SCL to LOW output(GPIO)\n",
		__func__);

	/* Set the CS, MOSI, MISO, SCL to LOW output(GPIO) */
	/* 1. gpio_request */
	ret = gpio_request(GPIO_CAM_SPI_SSN, "SSN");
	if (ret) {
		printk(KERN_DEBUG "%s  failed to request gpio(GPIO_CAM_SPI_SSN)\n",
			__func__);
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SPI_MOSI, "MOSI");
	if (ret) {
		printk(KERN_DEBUG "%s  failed to request gpio(GPIO_CAM_SPI_MOSI)\n",
			__func__);
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SPI_MISO, "MISO");
	if (ret) {
		printk(KERN_DEBUG "%s  failed to request gpio(GPIO_CAM_SPI_MISO)\n",
			__func__);
		return ret;
	}
#if 0
	if (system_rev > 7) {
		ret = gpio_request(GPIO_CAM_SCL, "SCL");
		if (ret) {
			printk(KERN_DEBUG "%s  failed to request gpio(GPIO_CAM_SCL)\n",
				__func__);
			return ret;
		}
	} else {
		ret = gpio_request(GPIO_GSENSE_SCL_18V, "SCL18V");
		if (ret) {
			printk(KERN_DEBUG "%s  failed to request gpio(GPIO_GSENSE_SCL_18V)\n",
				__func__);
			return ret;
		}
	}
	if (system_rev > 7) {
		ret = gpio_request(GPIO_CAM_SDA, "SDA");
		if (ret) {
			printk(KERN_DEBUG "%s  failed to request gpio(GPIO_CAM_SCL)\n",
				__func__);
			return ret;
		}
	} else {
		ret = gpio_request(GPIO_GSENSE_SDA_18V, "SDA18V");
		if (ret) {
			printk(KERN_DEBUG "%s  failed to request gpio(GPIO_GSENSE_SCL_18V)\n",
				__func__);
			return ret;
		}
	}
#endif
	/* 2. set output LOW */
	ret = gpio_direction_output(GPIO_CAM_SPI_SSN, 0);
	ret = gpio_direction_output(GPIO_CAM_SPI_MOSI, 0);
	ret = gpio_direction_output(GPIO_CAM_SPI_MISO, 0);
	if (system_rev > 7) {
		ret = gpio_direction_output(GPIO_CAM_SCL, 0);
		ret = gpio_direction_output(GPIO_CAM_SDA, 0);
	} else {
		ret = gpio_direction_output(GPIO_GSENSE_SCL_18V, 0);
		ret = gpio_direction_output(GPIO_GSENSE_SDA_18V, 0);
	}

	/*3. gpio_free*/
	gpio_free(GPIO_CAM_SPI_SSN);
	gpio_free(GPIO_CAM_SPI_MOSI);
	gpio_free(GPIO_CAM_SPI_MISO);
	if (system_rev > 7) {
		/* gpio_free(GPIO_CAM_SCL); */
		/* gpio_free(GPIO_CAM_SDA); */
	} else {
		/* gpio_free(GPIO_GSENSE_SCL_18V); */
		/* gpio_free(GPIO_GSENSE_SDA_18V); */
	}

	printk(KERN_DEBUG "%s  Set SPI PIN to output LOW!!!!\n",
		__func__);
	msleep(80);
	return ret;
}

static int drime4_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	const struct drime4_platform_data *pdata = client->dev.platform_data;
	struct drime4_state *state = to_state(sd);
	u32 int_factor;
	int retry_cnt = 0;
	int ap_int_val = 0;
	int noti_state_val = 0;
	int err = 0;
	int read_gpio = 0;

	sd_internal = sd;
	g_drime4_boot_error = 0;

	cam_dbg("start, val = %d", val);
	/* Default state values */
	state->isp.bad_fw = 1;

	state->preview = NULL;
	state->capture = NULL;
	state->photo = NULL;
	state->movie = NULL;
	state->fw_index = DRIME4_PATH_MAX;
	state->capturingGolfShot = false;
	state->capture_mode = RUNNING_MODE_SINGLE;
	state->drive_capture_mode = 0;
	state->capture_cnt = 1;
	state->PASM_mode = 0;
	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->sensor_mode = SENSOR_CAMERA;
	state->lens_fw_path = DRIME4_LENS_PATH_MAX;
	state->flash_mode = FLASH_MODE_OFF;
	state->wb_mode = WHITE_BALANCE_AUTO;
	state->focus.mode = FOCUS_MODE_SINGLE;
	state->focus.touch = 0;
	state->done_3a_lock = 0;
	state->isp.ap_issued = 0;
	state->isp.d4_issued = 0;
	state->isp_mode = DRIME4_LIVE_MOV_OFF;
	state->facedetect_mode = FACE_DETECTION_OFF;
	state->af_area = DRIME4_AF_AREA_SEL;
	state->preview_width = preview_frmsizes[0].width;
	state->preview_height = preview_frmsizes[0].height;
	state->jpeg_width = capture_frmsizes[0].width;
	state->jpeg_height = capture_frmsizes[0].height;
	state->movie_width = movie_frmsizes[0].width;
	state->movie_height = movie_frmsizes[0].height;
	state->hal_power_off = 0;
	state->factory_log_start = 0;
	state->focus_peaking_level = 0x00;
	state->focus_peaking_color = 0x01;
	state->noti_ctrl_status = 0;

	state->fps = 0;			/* auto */
#ifdef BURST_BOOT_PARAM
	state->first_boot_param = 0;
#endif
	if (val)
		state->samsung_app = 1;
	else
		state->samsung_app = 0;

	memset(&state->focus, 0, sizeof(state->focus));

#if 1
boot_start_check_retry:
	noti_state_val = drime4_spi_get_noti_check_pin();
	if (!noti_state_val && (retry_cnt < 100)) {
		msleep(50);
		retry_cnt++;
		goto boot_start_check_retry;
	}
	if (!noti_state_val) {
		ap_int_val = gpio_get_value(GPIO_ISP_INT);
		cam_err("ISP Boot start failed. I2C SDA = %s, AP INT = %s\n",
			noti_state_val ? "High" : "Low",
			ap_int_val ? "High" : "Low");

		g_drime4_boot_error = 1;
		return -1;
	}
	if (retry_cnt) {
		cam_dbg("ISP Boot start: off delay, retry cnt = %d\n",
			retry_cnt);
		msleep(40);
	}
	retry_cnt = 0;
#endif

#if 1	/* Boot Fail TEST */
	makeISPpinsLow();
#endif	/* #if 1 *//* Boot Fail TEST */

	pdata->power_on_off(1);

	if (system_rev > 7)
		s3c_gpio_cfgpin(GPIO_CAM_SDA, S3C_GPIO_INPUT);
	else
		s3c_gpio_cfgpin(GPIO_GSENSE_SDA_18V, S3C_GPIO_INPUT);

#if 0
	int_factor = drime4_wait_interrupt(sd, DRIME4_ISP_BOOT_TIMEOUT);
#else
boot_check_retry:
	ap_int_val = gpio_get_value(GPIO_ISP_INT);
#if 0
	noti_state_val = drime4_spi_get_noti_check_pin();
#else
	noti_state_val = 1;
#endif
	if ((!ap_int_val && noti_state_val) && (retry_cnt < 300)) {

#if 0	/* Boot Fail TEST */
		if (/*retry_cnt % 50 == 0 && */retry_cnt != 0) {
			makeISPpinsLow();
			pdata->power_on_off(1);
			cam_err("ISP Boot failed. retry_cnt = %d\n", retry_cnt);
		}

		if (retry_cnt == 0)
			msleep(40);
#else
		msleep(40);
#endif	/* #if 1 *//* Boot Fail TEST */
		retry_cnt++;
		goto boot_check_retry;
	}
	if (!ap_int_val && noti_state_val) {
#if 0
		cam_err("ISP Boot failed. AP INT = %s, Noti(SDA) = %s\n",
			ap_int_val ? "High" : "Low",
			noti_state_val ? "High" : "Low");
#else
		cam_err("ISP Boot failed. AP INT = %s\n",
			ap_int_val ? "High" : "Low");
#endif
		return -1;
	}
	if (retry_cnt) {
		cam_dbg("ISP Boot start: not complete retry cnt = %d\n",
			retry_cnt);
	}

	int_factor = drime4_wait_ap_interrupt(sd, 2);
	if (int_factor)
		cam_dbg("no effect ap interrupt occurred\n");
	int_factor = drime4_wait_d4_interrupt(sd, 2);
	if (int_factor)
		cam_dbg("no effect d4 interrupt occurred\n");

#if 0
	cam_dbg("ISP Boot check complete. gpio =  %d\n",
			gpio_val);
	retry_cnt = 0;
#endif
	err = drime4_set_avs(sd);
	CHECK_ERR(err);
#endif

	err = drime4_check_fw(sd);
	CHECK_ERR(err);

	if (val == 3) {
		err = drime4_save_ISP_log(sd);
		CHECK_ERR(err);
	}

	err = drime4_init_param(sd);
	CHECK_ERR(err);

	return 0;
}

static const struct v4l2_subdev_core_ops drime4_core_ops = {
	.init = drime4_init,		/* initializing API */
	.load_fw = drime4_load_fw,
	.queryctrl = drime4_queryctrl,
	.g_ctrl = drime4_g_ctrl,
	.s_ctrl = drime4_s_ctrl,
	.g_ext_ctrls = drime4_g_ext_ctrls,
	.s_ext_ctrls = drime4_s_ext_ctrls,
	.noti_ctrl = drime4_noti_ctrl,
};

static const struct v4l2_subdev_video_ops drime4_video_ops = {
	.s_mbus_fmt = drime4_s_fmt,
	.g_parm = drime4_g_parm,
	.s_parm = drime4_s_parm,
	.enum_framesizes = drime4_enum_framesizes,
	.s_stream = drime4_s_stream,
};

static const struct v4l2_subdev_ops drime4_ops = {
	.core = &drime4_core_ops,
	.video = &drime4_video_ops,
};

static ssize_t drime4_camera_rear_camtype_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char type[] = "CML0801";
	return sprintf(buf, "%s\n", type);
}

static ssize_t drime4_camera_rear_camfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", sysfs_phone_fw, sysfs_sensor_fw);
}

static ssize_t drime4_camera_rear_lensfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", sysfs_lens_fw, sysfs_lens_binary_fw);
}

static ssize_t drime4_camera_rear_flash(struct device *dev,
	struct device_attribute *attr, const char *buf,
	size_t count)
{
#ifdef CONFIG_LEDS_AAT1290A
	return aat1290a_power(dev, attr, buf, count);
#else
	return count;
#endif
}

static ssize_t drime4_camera_isp_core_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char core[20] = {0,};

	strcpy(core, sysfs_isp_core);
	return sprintf(buf, "%s\n", core);
}

static DEVICE_ATTR(rear_camtype, S_IRUGO,
		drime4_camera_rear_camtype_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO, drime4_camera_rear_camfw_show, NULL);
static DEVICE_ATTR(rear_flash, S_IWUSR|S_IWGRP|S_IROTH,
	NULL, drime4_camera_rear_flash);
static DEVICE_ATTR(isp_core, S_IRUGO, drime4_camera_isp_core_show, NULL);
static DEVICE_ATTR(rear_lensfw, S_IRUGO, drime4_camera_rear_lensfw_show, NULL);

/*
 * drime4_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int __devinit drime4_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct drime4_state *state;
	struct v4l2_subdev *sd;
	struct drime4_platform_data *pdata = client->dev.platform_data;
	int err = 0;

	state = kzalloc(sizeof(struct drime4_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, DRIME4_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &drime4_ops);

#ifdef CAM_DEBUG
	state->dbg_level = CAM_DEBUG | CAM_TRACE | CAM_I2C;
#endif

#ifdef DRIME4_BUSFREQ_OPP
	/* lock bus frequency */
	if (samsung_rev() >= EXYNOS4412_REV_2_0)
		dev_lock(bus_dev, drime4_dev, 440220);
	else
		dev_lock(bus_dev, drime4_dev, 400200);
#endif

	if (drime4_dev)
		dev_set_drvdata(drime4_dev, state);

	/* set ap_int to irq */
	if (pdata->config_isp_ap_irq)
		pdata->config_isp_ap_irq();

	mutex_init(&state->drime4_lock);

	/* wait queue initialize */
	init_waitqueue_head(&state->isp.ap_wait);

	err = request_irq(pdata->ap_irq,
		drime4_isp_ap_isr, IRQF_TRIGGER_FALLING, "drime4 ap isp", sd);
	if (err) {
		cam_err("failed to request ap_irq ~~~~~~~~~~~~~\n");
		return err;
	}
	state->isp.ap_irq = pdata->ap_irq;
	state->isp.ap_issued = 0;

	/* set d4_int to irq */
	pdata->d4_irq = gpio_to_irq(GPIO_RESERVED_1_INT);

	cam_err("%s: d4_irq = %d\n", __func__, pdata->d4_irq);
	/* wait queue initialize */
	init_waitqueue_head(&state->isp.d4_wait);

	err = request_irq(pdata->d4_irq,
		drime4_isp_d4_isr, IRQF_TRIGGER_RISING, "drime4 d4 isp", sd);
	if (err) {
		cam_err("failed to request d4_irq. %d\n", err);
		return err;
	}
	state->isp.d4_irq = pdata->d4_irq;
	state->isp.d4_issued = 0;

	if (system_rev > 7) {
		err = gpio_request(GPIO_CAM_SCL, "CAM_I2C_SCL_2");
		err = gpio_request(GPIO_CAM_SDA, "CAM_I2C_SDA_2");
		gpio_direction_input(GPIO_CAM_SDA);
	} else {
		err = gpio_request(GPIO_GSENSE_SCL_18V, "CAM_I2C_SDA_2");
		err = gpio_request(GPIO_GSENSE_SDA_18V, "CAM_I2C_SCL_2");
		gpio_direction_input(GPIO_GSENSE_SDA_18V);
	}

	drime4_spi_set_cmd_ctrl_pin(0);

	printk(KERN_DEBUG "%s, system_rev %d\n", __func__, system_rev);

	return 0;
}

static int __devexit drime4_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct drime4_state *state = to_state(sd);
	struct drime4_platform_data *pdata = client->dev.platform_data;
	int int_factor;
	int err = 0;

	cam_dbg("E\n");

	if (unlikely(state->isp.bad_fw))
		cam_err("camera is not ready!!\n");

	drime4_set_ext_mic_status(sd, 0);

#if 1 /* golf shot test */
	if (state->PASM_mode == MODE_GOLF_SHOT
		&& state->capturingGolfShot == true) {
		err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_CAPTURE_STOP, 0x00);
		CHECK_ERR(err);
		state->capturingGolfShot = false;
	}
#endif

#ifdef FEATURE_COMMAND_POWER_OFF	/* GPIO Power Off */
	err = drime4_set_mode(sd, DRIME4_LIVE_MOV_OFF);
	if (err)
		cam_err("failed to Liveview Stop %d\n", err);

	cam_dbg("start power off command\n");
#if 0
	err = drime4_writeb_res(sd, DRIME4_PACKET_DSP_CMD,
			DRIME4_CMD_POWER, 0x01);
	if (err)
		cam_err("failed to power off %d\n", err);
#else
	err = drime4_writeb(sd, DRIME4_PACKET_DSP_CMD, DRIME4_CMD_POWER, 0x01);
	if (err)
		cam_err("failed to power off %d\n", err);
	int_factor = drime4_wait_ap_interrupt(sd,
			DRIME4_ISP_TIMEOUT);
#endif
#endif

	cam_dbg("disconnect ap irq\n");
	if (state->isp.ap_irq > 0) {
		disable_irq(state->isp.ap_irq);
		free_irq(state->isp.ap_irq, sd);
	}

	cam_dbg("disconnect d4 irq\n");
	if (state->isp.d4_irq > 0) {
		disable_irq(state->isp.d4_irq);
		free_irq(state->isp.d4_irq, sd);
	}

	pdata->power_on_off(0);

	if (system_rev > 7) {
		gpio_direction_input(GPIO_CAM_SCL);
		gpio_direction_input(GPIO_CAM_SDA);
		gpio_free(GPIO_CAM_SCL);
		gpio_free(GPIO_CAM_SDA);
	} else {
		gpio_direction_input(GPIO_GSENSE_SCL_18V);
		gpio_direction_input(GPIO_GSENSE_SDA_18V);
		gpio_free(GPIO_GSENSE_SCL_18V);
		gpio_free(GPIO_GSENSE_SDA_18V);
	}

	mutex_destroy(&state->drime4_lock);

	v4l2_device_unregister_subdev(sd);

#ifdef DRIME4_BUSFREQ_OPP
	/* Unlock bus frequency */
	dev_unlock(bus_dev, drime4_dev);
#endif

	kfree(state);

	return 0;
}

static const struct i2c_device_id drime4_id[] = {
	{ DRIME4_DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, drime4_id);

static struct i2c_driver drime4_i2c_driver = {
	.driver = {
		.name	= DRIME4_DRIVER_NAME,
	},
	.probe		= drime4_probe,
	.remove		= __devexit_p(drime4_remove),
	.id_table	= drime4_id,
};

static int __init drime4_mod_init(void)
{
#ifdef DRIME4_BUSFREQ_OPP
	/* To lock bus frequency in OPP mode */
	bus_dev = dev_get("exynos-busfreq");
#endif

	if (!drime4_dev) {
		drime4_dev = device_create(camera_class,
				NULL, 0, NULL, "rear");
		if (IS_ERR(drime4_dev)) {
			cam_warn("failed to create device!\n");
			return 0;
		}

		if (device_create_file(drime4_dev, &dev_attr_rear_camtype)
				< 0) {
			cam_warn("failed to create device file, %s\n",
					dev_attr_rear_camtype.attr.name);
		}

		if (device_create_file(drime4_dev, &dev_attr_rear_camfw) < 0) {
			cam_warn("failed to create device file, %s\n",
					dev_attr_rear_camfw.attr.name);
		}

		if (device_create_file(drime4_dev, &dev_attr_rear_flash) < 0) {
			cam_warn("failed to create device file, %s\n",
					dev_attr_rear_flash.attr.name);
		}

		if (device_create_file(drime4_dev, &dev_attr_isp_core) < 0) {
			cam_warn("failed to create device file, %s\n",
					dev_attr_isp_core.attr.name);
		}

		if (device_create_file(drime4_dev, &dev_attr_rear_lensfw) < 0) {
			cam_warn("failed to create device file, %s\n",
					dev_attr_rear_lensfw.attr.name);
		}
	}
#ifdef CONFIG_SWITCH
	switch_dev_register(&android_hotshoe);
#endif

	return i2c_add_driver(&drime4_i2c_driver);
}

static void __exit drime4_mod_exit(void)
{
	i2c_del_driver(&drime4_i2c_driver);
#ifdef CONFIG_SWITCH
	switch_dev_unregister(&android_hotshoe);
#endif
}
module_init(drime4_mod_init);
module_exit(drime4_mod_exit);


MODULE_DESCRIPTION("driver for DMC DRIMe4");
MODULE_LICENSE("GPL");
