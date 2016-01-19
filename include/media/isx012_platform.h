/*
 * Driver for ISX012 (5MP camera) from Sony
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/sec_camera_platform.h>

#define ISX012_DEVICE_NAME	"ISX012"
#define ISX012_STREAMOFF_DELAY	(DEFAULT_STREAMOFF_DELAY + 50)

enum {
	ISX012_FLASH_MODE_NORMAL,
	ISX012_FLASH_MODE_MOVIE,
	ISX012_FLASH_MODE_MAX,
};

enum {
	ISX012_FLASH_OFF = 0,
	ISX012_FLASH_ON = 1,
};

struct isx012_platform_data {
	struct sec_camera_platform_data common;
	int (*is_flash_on)(void);
	int (*stby_on)(bool);
};

