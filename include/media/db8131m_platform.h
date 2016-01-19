/*
 * Driver for DB8131M (1.3M camera) from SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/sec_camera_platform.h>
	 
#define DB8131M_DEVICE_NAME	"DB8131A"

#define VT_DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define DEFAULT_PREVIEW_WIDTH		640
#define DEFAULT_PREVIEW_HEIGHT		480
#define DEFAULT_CAPTURE_WIDTH		1600
#define DEFAULT_CAPTURE_HEIGHT		1200

#define DB8131A_I2C_ADDR		(0x8A >> 1)

#if defined(CONFIG_MACH_TAB3) || defined(CONFIG_MACH_ZEST)
#define DB8131A_STREAMOFF_DELAY		0	/* 0ms */
#else
#define DB8131A_STREAMOFF_DELAY		200	/* 200ms */
#endif

enum {
	DB8131A_FLASH_MODE_NORMAL,
	DB8131A_FLASH_MODE_MOVIE,
	DB8131A_FLASH_MODE_MAX,
};

enum {
	DB8131A_FLASH_OFF = 0,
	DB8131A_FLASH_ON = 1,
};

struct db8131m_platform_data {
	struct sec_camera_platform_data common;
	int (*is_flash_on)(void);
};

