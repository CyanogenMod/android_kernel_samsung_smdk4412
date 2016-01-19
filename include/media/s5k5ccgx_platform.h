/*
 * Driver for S5K5CCGX (3MP camera) from LSI
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/sec_camera_platform.h>

#define S5K5CCGX_DEVICE_NAME	"S5K5CCGX"

#if defined(CONFIG_MACH_TAB3)
#define S5K5CCGX_I2C_ADDR	(0x5A >> 1)
#define S5K5CCGX_STREAMOFF_DELAY	(DEFAULT_STREAMOFF_DELAY + 50)
#define S5K5CCGX_STREAMOFF_NIGHT_DELAY	(500 + 30)
#else
#define S5K5CCGX_I2C_ADDR	(0x78 >> 1)
#define S5K5CCGX_STREAMOFF_DELAY	(DEFAULT_STREAMOFF_DELAY + 20)
#endif

enum {
	S5K5CCGX_FLASH_MODE_NORMAL,
	S5K5CCGX_FLASH_MODE_MOVIE,
	S5K5CCGX_FLASH_MODE_MAX,
};

enum {
	S5K5CCGX_FLASH_OFF = 0,
	S5K5CCGX_FLASH_ON = 1,
};

struct s5k5ccgx_platform_data {
	struct sec_camera_platform_data common;
	int (*is_flash_on)(void);
};

