/*
 * Driver for DRIME4
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

struct drime4_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in Hz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;

	int (*is_isp_reset)(void);
	int (*power_on_off)(int);
	int (*config_isp_ap_irq)(void);
	int (*avs_test_mode)(void);
//	int (*config_isp_d4_irq)(int onoff);
	int ap_irq;
	int d4_irq;
};
