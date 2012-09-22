/*
 * Driver for M9MO (16MP camera) from NEC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

struct m9mo_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in Hz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;

	/* ISP interrupt */
	int (*config_isp_irq)(void);
	int (*config_sambaz)(int);
	int (*af_led_power)(int);
	int irq;
};
