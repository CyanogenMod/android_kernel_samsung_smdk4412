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


#include "s3cfb.h"
#include "s3cfb_ielcd_kona.h"
#include "s3cfb_mdnie_kona.h"
#include "mdnie_kona.h"


#define s3c_mdnie_read(addr)		__raw_readl(s3c_mdnie_base + addr*4)
#define s3c_mdnie_write(addr, val)	__raw_writel(val, s3c_mdnie_base + addr*4)


static struct resource *s3c_mdnie_mem;
static void __iomem *s3c_mdnie_base;


int mdnie_write(unsigned int addr, unsigned int val)
{
	return s3c_mdnie_write(addr, val);
}

int mdnie_mask(void)
{
	return s3c_mdnie_write(MDNIE_REG_MASK, 0x9FFF);
}

int mdnie_unmask(void)
{
	return s3c_mdnie_write(MDNIE_REG_MASK, 0);
}

int mdnie_set_size(unsigned int hsize, unsigned int vsize)
{
	unsigned int reg;

	/* Bank0 Select : DO NOT REMOVE THIS LINE */
	s3c_mdnie_write(MDNIE_REG_BANK_SEL, 0);

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	/* Input Data Unmask */
	reg = s3c_mdnie_read(S3C_MDNIE_rR1);
	reg &= ~S3C_MDNIE_INPUT_DATA_ENABLE;
	s3c_mdnie_write(S3C_MDNIE_rR1, reg);
#endif

	/* LCD width */
	reg = s3c_mdnie_read(MDNIE_REG_WIDTH);
	reg &= ~S3C_MDNIE_SIZE_MASK;
	reg |= S3C_MDNIE_HSIZE(hsize);
	s3c_mdnie_write(MDNIE_REG_WIDTH, reg);

	/* LCD height */
	reg = s3c_mdnie_read(MDNIE_REG_HEIGHT);
	reg &= ~S3C_MDNIE_SIZE_MASK;
	reg |= S3C_MDNIE_VSIZE(vsize);
	s3c_mdnie_write(MDNIE_REG_HEIGHT, reg);

	mdnie_unmask();

	return 0;
}

int mdnie_display_on(struct s3cfb_global *ctrl)
{
	mdnie_set_size(ctrl->lcd->width, ctrl->lcd->height);

	ielcd_init_global(ctrl);

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

	return 0;
}

void mdnie_setup(void)
{
	mdnie_hw_init();
	ielcd_hw_init();
}

MODULE_DESCRIPTION("EXYNOS mDNIe Device Driver");
MODULE_LICENSE("GPL");
