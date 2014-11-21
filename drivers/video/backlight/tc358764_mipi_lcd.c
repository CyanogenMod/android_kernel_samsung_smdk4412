/* linux/drivers/video/backlight/tc358764_mipi_lcd.c
 *
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
*/

#include <linux/delay.h>

#include <video/mipi_display.h>

#include <plat/dsim.h>
#include <plat/mipi_dsi.h>
#include <plat/cpu.h>

unsigned char initcode_013c[6] = {0x3c, 0x01, 0x03, 0x00, 0x02, 0x00};
unsigned char initcode_0114[6] = {0x14, 0x01, 0x02, 0x00, 0x00, 0x00};
unsigned char initcode_0164[6] = {0x64, 0x01, 0x05, 0x00, 0x00, 0x00};
unsigned char initcode_0168[6] = {0x68, 0x01, 0x05, 0x00, 0x00, 0x00};
unsigned char initcode_016c[6] = {0x6c, 0x01, 0x05, 0x00, 0x00, 0x00};
unsigned char initcode_0170[6] = {0x70, 0x01, 0x05, 0x00, 0x00, 0x00};
unsigned char initcode_0134[6] = {0x34, 0x01, 0x1f, 0x00, 0x00, 0x00};
unsigned char initcode_0210[6] = {0x10, 0x02, 0x1f, 0x00, 0x00, 0x00};
unsigned char initcode_0104[6] = {0x04, 0x01, 0x01, 0x00, 0x00, 0x00};
unsigned char initcode_0204[6] = {0x04, 0x02, 0x01, 0x00, 0x00, 0x00};
unsigned char initcode_0450[6] = {0x50, 0x04, 0x20, 0x01, 0xfa, 0x00};
unsigned char initcode_0454[6] = {0x54, 0x04, 0x20, 0x00, 0x50, 0x00};
unsigned char initcode_0458[6] = {0x58, 0x04, 0x00, 0x05, 0x30, 0x00};
unsigned char initcode_045c[6] = {0x5c, 0x04, 0x05, 0x00, 0x0a, 0x00};
unsigned char initcode_0460[6] = {0x60, 0x04, 0x20, 0x03, 0x0a, 0x00};
unsigned char initcode_0464[6] = {0x64, 0x04, 0x01, 0x00, 0x00, 0x00};
unsigned char initcode_04a0_1[6] = {0xa0, 0x04, 0x06, 0x80, 0x44, 0x00};
unsigned char initcode_04a0_2[6] = {0xa0, 0x04, 0x06, 0x80, 0x04, 0x00};
unsigned char initcode_0504[6] = {0x04, 0x05, 0x04, 0x00, 0x00, 0x00};
unsigned char initcode_049c[6] = {0x9c, 0x04, 0x0d, 0x00, 0x00, 0x00};

unsigned int *initcode[20] = {
	(unsigned int *)initcode_013c,
	(unsigned int *)initcode_0114,
	(unsigned int *)initcode_0164,
	(unsigned int *)initcode_0168,
	(unsigned int *)initcode_016c,
	(unsigned int *)initcode_0170,
	(unsigned int *)initcode_0134,
	(unsigned int *)initcode_0210,
	(unsigned int *)initcode_0104,
	(unsigned int *)initcode_0204,
	(unsigned int *)initcode_0450,
	(unsigned int *)initcode_0454,
	(unsigned int *)initcode_0458,
	(unsigned int *)initcode_045c,
	(unsigned int *)initcode_0460,
	(unsigned int *)initcode_0464,
	(unsigned int *)initcode_04a0_1,
	(unsigned int *)initcode_04a0_2,
	(unsigned int *)initcode_0504,
	(unsigned int *)initcode_049c
};

static int init_lcd(struct mipi_dsim_device *dsim)
{
	int i;

	if (soc_is_exynos5250() && samsung_rev() >= EXYNOS5250_REV_1_0) {
		for (i = 0; i <= 19; i++) {
			s5p_mipi_dsi_wr_data(dsim,
				MIPI_DSI_GENERIC_LONG_WRITE,
				(unsigned int)initcode[i], 6);
			usleep_range(6000, 7000);
		}
	} else {
		for (i = 0; i <= 19; i++) {
			if (s5p_mipi_dsi_wr_data(dsim,
				MIPI_DSI_GENERIC_LONG_WRITE,
				(unsigned int)initcode[i], 6) == -1)
				return 0;
			usleep_range(6000, 7000);
		}
	}

	msleep(800);

	return 1;
}

static int tc358764_mipi_lcd_suspend(struct mipi_dsim_device *dsim)
{
	return 0;
}

static int tc358764_mipi_lcd_displayon(struct mipi_dsim_device *dsim)
{
	return init_lcd(dsim);
}

static int tc358764_mipi_lcd_resume(struct mipi_dsim_device *dsim)
{
	return init_lcd(dsim);
}

struct mipi_dsim_lcd_driver tc358764_mipi_lcd_driver = {
	.suspend	= tc358764_mipi_lcd_suspend,
	.displayon	= tc358764_mipi_lcd_displayon,
	.resume		= tc358764_mipi_lcd_resume,
};
