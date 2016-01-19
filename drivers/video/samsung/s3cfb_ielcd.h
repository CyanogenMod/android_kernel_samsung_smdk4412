/* linux/drivers/video/samsung/s3cfb_ielcd.h
 *
 * Header file for Samsung (IELCD) driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3CFB_IELCD_H__
#define __S3CFB_IELCD_H__

#include <mach/map.h>

#define S3C_IELCD_PHY_BASE		EXYNOS4_PA_LCD_LITE0
#define S3C_IELCD_MAP_SIZE		SZ_32K

#define IELCD_VIDCON0			(0x0000)
#define IELCD_VIDCON1			(0x0004)

#define IELCD_VIDTCON0		(0x0010)
#define IELCD_VIDTCON1		(0x0014)
#define IELCD_VIDTCON2		(0x0018)

#define IELCD_WINCON0			(0x0020)
#define IELCD_WINSHMAP		(0x0034)
#define IELCD_VIDOSD0A		(0x0040)
#define IELCD_VIDOSD0B		(0x0044)
#define IELCD_VIDOSD0C		(0x0048)

#define IELCD_GPOUTCON0		(0x0278)

#define IELCD_MAGIC_KEY		0x2ff47

int ielcd_hw_init(void);
int ielcd_display_on(void);
int ielcd_display_off(void);
void ielcd_init_global(void);

#endif
