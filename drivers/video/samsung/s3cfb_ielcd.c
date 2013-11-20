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
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <plat/clock.h>
#include <plat/regs-fb-s5p.h>

#include "s3cfb.h"
#include "s3cfb_mdnie.h"
#include "s3cfb_ielcd.h"

#define FIMD_REG_BASE			S5P_PA_FIMD0
#ifdef CONFIG_FB_EXYNOS_FIMD_V8
#define FIMD_MAP_SIZE			SZ_256K
#else
#define FIMD_MAP_SIZE			SZ_32K
#endif

#define s3c_ielcd_readl(addr)		readl(s3c_ielcd_base + addr)
#define s3c_ielcd_writel(addr, val)	writel(val, s3c_ielcd_base + addr)

static void __iomem *fimd_reg;
static struct resource *s3c_ielcd_mem;
static void __iomem *s3c_ielcd_base;

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

	fimd_reg = ioremap(FIMD_REG_BASE, FIMD_MAP_SIZE);
	if (fimd_reg == NULL) {
		pr_err("%s: fail to ioremap - fimd\n", __func__);
		return -ENOENT;
	}

	return 0;
}

static void get_lcd_size(unsigned int *xres, unsigned int *yres)
{
	unsigned int cfg;
	void __iomem *base_reg = fimd_reg;

	cfg = readl(base_reg + S3C_VIDTCON2);
	*xres = ((cfg & S3C_VIDTCON2_HOZVAL_MASK) >> S3C_VIDTCON2_HOZVAL_SHIFT) + 1;
	*yres = ((cfg & S3C_VIDTCON2_LINEVAL_MASK) >> S3C_VIDTCON2_LINEVAL_SHIFT) + 1;
}

static int ielcd_logic_start(void)
{
	s3c_ielcd_writel(IELCD_GPOUTCON0, IELCD_MAGIC_KEY);

	return 0;
}

static void ielcd_set_polarity(void)
{
	unsigned int cfg;
	void __iomem *base_reg = fimd_reg;

	cfg = readl(base_reg + S3C_VIDCON1);
	cfg &= (S3C_VIDCON1_IVCLK_RISING_EDGE | S3C_VIDCON1_IHSYNC_INVERT |
		S3C_VIDCON1_IVSYNC_INVERT | S3C_VIDCON1_IVDEN_INVERT);
	s3c_ielcd_writel(IELCD_VIDCON1, cfg);
}

static void ielcd_set_timing(void)
{
	unsigned int cfg;
	void __iomem *base_reg = fimd_reg;

	/*LCD verical porch setting*/
	cfg = readl(base_reg + S3C_VIDTCON0);
	s3c_ielcd_writel(IELCD_VIDTCON0, cfg);

	/*LCD horizontal porch setting*/
	cfg = readl(base_reg + S3C_VIDTCON1);
	s3c_ielcd_writel(IELCD_VIDTCON1, cfg);
}

/* Warning: FIMD / IELCD can have different BIT length register map.
So You MUST check register map.
If map is different you should extract register value from FIMD, and then setting to IELCD */
static void ielcd_set_lcd_size(unsigned int xres, unsigned int yres)
{
	unsigned int cfg;
	void __iomem *base_reg = fimd_reg;

	cfg = readl(base_reg + S3C_VIDTCON2);
	s3c_ielcd_writel(IELCD_VIDTCON2, cfg);
}

static void ielcd_window_on(void)
{
	unsigned int cfg;

	cfg = s3c_ielcd_readl(IELCD_WINSHMAP);
	cfg |= S3C_WINSHMAP_CH_ENABLE(0);
	s3c_ielcd_writel(IELCD_WINSHMAP, cfg);

	cfg = s3c_ielcd_readl(S3C_WINCON(0));
	cfg |= S3C_WINCON_ENWIN_ENABLE;
	s3c_ielcd_writel(S3C_WINCON(0), cfg);
}

int ielcd_display_on(void)
{
	unsigned int cfg;

	cfg = s3c_ielcd_readl(IELCD_VIDCON0);
	cfg |= (S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE);
	s3c_ielcd_writel(IELCD_VIDCON0, cfg);

	return 0;
}

int ielcd_display_off(void)
{
	unsigned int cfg, ielcd_count = 0;

	cfg = s3c_ielcd_readl(IELCD_VIDCON0);
	cfg |= S3C_VIDCON0_ENVID_ENABLE;
	cfg &= ~(S3C_VIDCON0_ENVID_F_ENABLE);

	s3c_ielcd_writel(IELCD_VIDCON0, cfg);

	do {
		if (++ielcd_count > 2000000) {
			pr_err("ielcd off fail\n");
			return 1;
		}

		if (!(s3c_ielcd_readl(IELCD_VIDCON1) & S3C_VIDCON1_LINECNT_MASK))
			return 0;
	} while (1);
}

void ielcd_init_global(void)
{
	unsigned int cfg, xres, yres;
	void __iomem *base_reg = fimd_reg;

	get_lcd_size(&xres, &yres);

	ielcd_logic_start();

	ielcd_set_polarity();
	ielcd_set_timing();
	ielcd_set_lcd_size(xres, yres);

	/* vclock divider setting , same as FIMD */
	cfg = readl(base_reg + S3C_VIDCON0);
	cfg &= ~(S3C_VIDCON0_VIDOUT_MASK | S3C_VIDCON0_VCLKEN_MASK);
	cfg |= S3C_VIDCON0_VIDOUT_RGB;
	cfg |= S3C_VIDCON0_VCLKEN_NORMAL;
	s3c_ielcd_writel(IELCD_VIDCON0, cfg);

	/* window0 position setting , fixed */
	s3c_ielcd_writel(IELCD_VIDOSD0A, 0);

	/* window0 position setting */
	cfg = S3C_VIDOSD_RIGHT_X(xres - 1);
	cfg |= S3C_VIDOSD_BOTTOM_Y(yres - 1);
	s3c_ielcd_writel(IELCD_VIDOSD0B, cfg);

	/* window0 osd size setting */
	s3c_ielcd_writel(IELCD_VIDOSD0C, xres * yres);

	/* window0 setting , fixed */
	cfg = S3C_WINCON_DATAPATH_LOCAL | S3C_WINCON_BPPMODE_32BPP | S3C_WINCON_INRGB_RGB;
	s3c_ielcd_writel(IELCD_WINCON0, cfg);

	ielcd_window_on();
}

