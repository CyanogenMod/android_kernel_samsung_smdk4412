/* linux/drivers/video/samsung/s3cfb_mdnie.c
 *
 * Register interface file for Samsung IELCD driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <linux/io.h>
#include <mach/map.h>
#include <plat/clock.h>
#include <plat/regs-fb-s5p.h>

#include "s3cfb.h"
#include "s3cfb_mdnie_kona.h"
#include "s3cfb_ielcd_kona.h"

static struct resource *s3c_ielcd_mem;
static void __iomem *s3c_ielcd_base;

#define s3c_ielcd_readl(addr)			__raw_readl(s3c_ielcd_base + addr)
#define s3c_ielcd_writel(addr, val)		writel(val, s3c_ielcd_base + addr)

static struct s3cfb_global ielcd_fb;
static struct s3cfb_global *ielcd_fbdev;

int ielcd_hw_init(void)
{
	s3c_ielcd_mem = request_mem_region(S3C_IELCD_PHY_BASE, S3C_IELCD_MAP_SIZE, "ielcd");
	if (IS_ERR_OR_NULL(s3c_ielcd_mem)) {
		pr_err("%s: fail to request_mem_region\n", __func__);
		return -ENOENT;
	}

	s3c_ielcd_base = ioremap(S3C_IELCD_PHY_BASE, S3C_IELCD_MAP_SIZE);
	if (IS_ERR_OR_NULL(s3c_ielcd_base)) {
		pr_err("%s: fail to ioremap\n", __func__);
		return -ENOENT;
	}

	ielcd_fbdev = &ielcd_fb;

	return 0;
}

int ielcd_display_on(void)
{
	unsigned int cfg;

	cfg = s3c_ielcd_readl(S3C_VIDCON0);
	cfg |= (S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE);
	s3c_ielcd_writel(S3C_VIDCON0, cfg);

	return 0;
}

int ielcd_display_off(void)
{
	unsigned int cfg, ielcd_count = 0;

	cfg = s3c_ielcd_readl(S3C_VIDCON0);
	cfg |= S3C_VIDCON0_ENVID_ENABLE;
	cfg &= ~(S3C_VIDCON0_ENVID_F_ENABLE);

	s3c_ielcd_writel(S3C_VIDCON0, cfg);

	do {
		if (++ielcd_count > 2000000) {
			printk(KERN_ERR "ielcd off fail\n");
			return 1;
		}

		if (!(s3c_ielcd_readl(S3C_VIDCON1) & 0xffff0000))
			return 0;
	} while (1);
}

void ielcd_init_global(struct s3cfb_global *ctrl)
{
	unsigned int cfg;

	*ielcd_fbdev = *ctrl;
	ctrl->ielcd_regs = ielcd_fbdev->regs = s3c_ielcd_base;

	s3c_ielcd_writel(S3C_IELCD_GPOUTCON0, S3C_IELCD_MAGIC_KEY);

	s3cfb_set_polarity_only(ielcd_fbdev);
	s3cfb_set_timing(ielcd_fbdev);
	s3cfb_set_lcd_size(ielcd_fbdev);

	/* vclock divider setting , same as FIMD */
	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~(S3C_VIDCON0_VIDOUT_MASK | S3C_VIDCON0_VCLKEN_MASK);
	cfg |= S3C_VIDCON0_VIDOUT_RGB;
	cfg |= S3C_VIDCON0_VCLKEN_NORMAL;
	s3c_ielcd_writel(S3C_VIDCON0, cfg);

	/* window0 position setting , fixed */
	s3c_ielcd_writel(S3C_VIDOSD0A, 0);

	/* window0 position setting */
	cfg = S3C_VIDOSD_RIGHT_X(ctrl->lcd->width - 1);
	cfg |= S3C_VIDOSD_BOTTOM_Y(ctrl->lcd->height - 1);
	s3c_ielcd_writel(S3C_VIDOSD0B, cfg);

	/* window0 osd size setting */
	s3c_ielcd_writel(S3C_VIDOSD0C, ctrl->lcd->width * ctrl->lcd->height);

	/* window0 setting , fixed */
	cfg = S3C_WINCON_DATAPATH_LOCAL | S3C_WINCON_BPPMODE_32BPP | S3C_WINCON_INRGB_RGB;
	s3c_ielcd_writel(S3C_WINCON0, cfg);

	s3cfb_window_on(ielcd_fbdev, 0);
}

