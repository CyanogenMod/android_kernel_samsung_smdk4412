/*
 * Driver for S5K4ECGX (1.3M camera) from SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

#define CAM_FLASH_ON 0
#define CAM_FLASH_TORCH 1
#define CAM_FLASH_OFF 2

struct s5k4ecgx_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */
	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;

	int (*flash_ctrl)(int ctrl);
};
