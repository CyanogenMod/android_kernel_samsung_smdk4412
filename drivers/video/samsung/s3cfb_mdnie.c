/* linux/drivers/video/samsung/s3cfb_mdnie.c
 *
 * Register interface file for Samsung mDNIe driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <plat/regs-fb-s5p.h>
#include <mach/map.h>

#include "s3cfb.h"
#include "s3cfb_ielcd.h"
#include "s3cfb_mdnie.h"
#include "mdnie.h"

#define FIMD_REG_BASE			S5P_PA_FIMD0
#define FIMD_MAP_SIZE			SZ_32K

static void __iomem *fimd_reg;
static struct resource *s3c_mdnie_mem;
static void __iomem *s3c_mdnie_base;

#define s3c_mdnie_read(addr)		readl(s3c_mdnie_base + addr*4)
#define s3c_mdnie_write(addr, val)	writel(val, s3c_mdnie_base + addr*4)

int mdnie_write(unsigned int addr, unsigned int val)
{
	s3c_mdnie_write(addr, val);

	return 0;
}

int mdnie_mask(void)
{
	s3c_mdnie_write(MDNIE_REG_MASK, 0x9FFF);

	return 0;
}

int mdnie_unmask(void)
{
	s3c_mdnie_write(MDNIE_REG_MASK, 0);

	return 0;
}

static int mdnie_hw_init(void)
{
	s3c_mdnie_mem = request_mem_region(S3C_MDNIE_PHY_BASE, S3C_MDNIE_MAP_SIZE, "mdnie");
	if (IS_ERR_OR_NULL(s3c_mdnie_mem)) {
		pr_err("%s: fail to request_mem_region\n", __func__);
		return -ENOENT;
	}

	s3c_mdnie_base = ioremap(S3C_MDNIE_PHY_BASE, S3C_MDNIE_MAP_SIZE);
	if (IS_ERR_OR_NULL(s3c_mdnie_base)) {
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

static int mdnie_set_size(void)
{
	unsigned int cfg, xres, yres;

	get_lcd_size(&xres, &yres);

	/* Bank0 Select : DO NOT REMOVE THIS LINE */
	s3c_mdnie_write(MDNIE_REG_BANK_SEL, 0);

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	/* Input Data Unmask */
	cfg = s3c_mdnie_read(S3C_MDNIE_rR1);
	cfg &= ~S3C_MDNIE_INPUT_DATA_ENABLE;
	s3c_mdnie_write(S3C_MDNIE_rR1, cfg);
#endif

	/* LCD width */
	cfg = s3c_mdnie_read(MDNIE_REG_WIDTH);
	cfg &= ~S3C_MDNIE_SIZE_MASK;
	cfg |= S3C_MDNIE_HSIZE(xres);
	s3c_mdnie_write(MDNIE_REG_WIDTH, cfg);

	/* LCD height */
	cfg = s3c_mdnie_read(MDNIE_REG_HEIGHT);
	cfg &= ~S3C_MDNIE_SIZE_MASK;
	cfg |= S3C_MDNIE_VSIZE(yres);
	s3c_mdnie_write(MDNIE_REG_HEIGHT, cfg);

	mdnie_unmask();

	return 0;
}

int mdnie_display_on(void)
{
	mdnie_set_size();

	ielcd_init_global();

	ielcd_display_on();

	if (!IS_ERR_OR_NULL(g_mdnie))
		g_mdnie->enable = TRUE;

	return 0;
}

int mdnie_display_off(void)
{
	if (!IS_ERR_OR_NULL(g_mdnie))
		g_mdnie->enable = FALSE;

	return ielcd_display_off();
}

void mdnie_setup(void)
{
	mdnie_hw_init();
	ielcd_hw_init();
}

MODULE_DESCRIPTION("EXYNOS mDNIe Device Driver");
MODULE_LICENSE("GPL");
