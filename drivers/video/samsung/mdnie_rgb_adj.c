/* linux/drivers/video/samsung/mdnie_rgb_adj.c
 *
 * RGB color tuning interface for Samsung mDNIe
 *
 * Copyright (c) 2016 The CyanogenMod Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/device.h>
#include <linux/mdnie.h>
#include <linux/platform_device.h>

#include "mdnie_rgb_adj.h"
#include "s3cfb.h"
#include "s3cfb_mdnie.h"

static inline u8 mdnie_scale_value(u8 val, u8 adj) {
	int big = val * adj;
	return big / 255;
}

static const unsigned short default_scr_values[] = {
	0x00e1, 0xff00, /*SCR RrCr*/
	0x00e2, 0x00ff, /*SCR RgCg*/
	0x00e3, 0x00ff, /*SCR RbCb*/
	0x00e4, 0x00ff, /*SCR GrMr*/
	0x00e5, 0xff00, /*SCR GgMg*/
	0x00e6, 0x00ff, /*SCR GbMb*/
	0x00e7, 0x00ff, /*SCR BrYr*/
	0x00e8, 0x00ff, /*SCR BgYg*/
	0x00e9, 0xff00, /*SCR BbYb*/
	0x00ea, 0x00ff, /*SCR KrWr*/
	0x00eb, 0x00ff, /*SCR KgWg*/
	0x00ec, 0x00ff, /*SCR KbWb*/
	END_SEQ, 0x0000,
};

u16 mdnie_rgb_hook(struct mdnie_info *mdnie, int reg, int val) {
	u8 adj = 255, c1, c2;
	u16 res;

	if (!mdnie->rgb_adj_enable) {
		dev_info(mdnie->dev, "rgb adjustment disabled!\n");
		return val;
	}

	if (reg >= MDNIE_SCR_START && reg <= MDNIE_SCR_END) {
		/* adjust */
		if (IS_SCR_RED(reg)) {
			// XrYr - adjust r
			adj = mdnie->r_adj;
		} else if (IS_SCR_GREEN(reg)) {
			// XgYg - adjust g
			adj = mdnie->g_adj;
		} else if (IS_SCR_BLUE(reg)) {
			// XbYb - adjust b
			adj = mdnie->b_adj;
		}
		c1 = mdnie_scale_value(val >> 8, adj); // adj for X
		c2 = mdnie_scale_value(val, adj); // adj for Y
		res = (c1 << 8) + c2;
		return res;
	}
	return val;
}

void mdnie_send_rgb(struct mdnie_info *mdnie) {
	int i = 0;

	if (!mdnie->rgb_adj_enable) {
		dev_info(mdnie->dev, "rgb adjustment disabled!\n");
		return;
	}

	while (default_scr_values[i] != END_SEQ) {
		mdnie_write(default_scr_values[i],
				mdnie_rgb_hook(mdnie, default_scr_values[i], default_scr_values[i+1]));
		i += 2;
	}
}

unsigned short mdnie_effect_master_hook(struct mdnie_info *mdnie, unsigned short val) {
	if (mdnie->r_adj != 255 || mdnie->g_adj != 255 || mdnie->b_adj != 255) {
		val |= MDNIE_SCR_MASK;
	}

	return val;
}

