/* linux/arch/arm/mach-exynos/setup-dsim.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * DSIM controller configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <plat/clock.h>
#include <plat/regs-dsim.h>
#include <plat/gpio-cfg.h>
#include <mach/map.h>
#include <mach/regs-pmu.h>

#define S5P_MIPI_M_RESETN 4

static void s5p_dsim_enable_d_phy(unsigned char enable)
{
	unsigned int reg;

	reg = readl(S5P_MIPI_DPHY_CONTROL(0));
	if (enable)
		reg |= S5P_MIPI_DPHY_ENABLE;
	else if (!(reg & S5P_MIPI_DPHY_SRESETN))
		reg &= ~S5P_MIPI_DPHY_ENABLE;
	writel(reg, S5P_MIPI_DPHY_CONTROL(0));
}

static void s5p_dsim_enable_dsi_master(unsigned char enable)
{
	unsigned int reg;

	reg = (readl(S5P_MIPI_DPHY_CONTROL(0))) & ~(1 << 2);
	reg |= (enable << 2);
	writel(reg, S5P_MIPI_DPHY_CONTROL(0));
}

void s5p_dsim_enable_clk(void *d_clk, unsigned char enable)
{
	int ret = 0;
	struct clk *dsim_clk = (struct clk *) d_clk;

	if (enable) {
		ret = clk_enable(dsim_clk);
		if (ret < 0)
			printk("failed to clk_enable of dsim\n");
	} else
		clk_disable(dsim_clk);
}

void s5p_dsim_part_reset(void)
{
	writel(S5P_MIPI_M_RESETN, S5P_MIPI_DPHY_CONTROL(0));
}

void s5p_dsim_init_d_phy(unsigned int dsim_base)
{
	/* enable D-PHY */
	s5p_dsim_enable_d_phy(1);

	/* enable DSI master block */
	s5p_dsim_enable_dsi_master(1);
}

void s5p_dsim_exit_d_phy(unsigned int dsim_base)
{
	/* enable DSI master block */
	s5p_dsim_enable_dsi_master(0);

	/* enable D-PHY */
	s5p_dsim_enable_d_phy(0);
}

