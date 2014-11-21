/*
 * Driver for S5C73M3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

struct s5c73m3_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in Hz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;

	void (*set_vdd_core)(int);
	bool (*is_vdd_core_set)(void);
	int (*is_isp_reset)(void);
	int (*power_on_off)(int);
};
