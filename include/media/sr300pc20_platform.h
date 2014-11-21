/*
 * Driver for SR300PC20 (1.3MP camera) from siliconfile
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <media/sec_camera_platform.h>

#define SR300PC20_STREAMOFF_DELAY	(DEFAULT_STREAMOFF_DELAY + 50)

enum {
	SR300PC20_FLASH_MODE_NORMAL,
	SR300PC20_FLASH_MODE_MOVIE,
	SR300PC20_FLASH_MODE_MAX,
};

enum {
	SR300PC20_FLASH_OFF = 0,
	SR300PC20_FLASH_ON = 1,
};

struct sr300pc20_platform_data {
	struct sec_camera_platform_data common;
	int (*stby_on)(bool);
	int (*is_flash_on)(void);
};
