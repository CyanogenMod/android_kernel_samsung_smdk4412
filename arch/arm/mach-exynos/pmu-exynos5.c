/* linux/arch/arm/mach-exynos/pmu-exynos5.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS5 - CPU PMU(Power Management Unit) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>

#include <mach/regs-clock.h>
#include <mach/regs-pmu5.h>
#include <mach/pmu.h>

#include <plat/cpu.h>

static struct exynos4_pmu_conf *exynos5_pmu_config;

static unsigned int entry_cnt;

static struct exynos4_pmu_conf exynos52xx_pmu_config[] = {
	/* { .reg = address, .val = { AFTR, LPA, SLEEP } */
	{ EXYNOS5_ARM_CORE0_SYS_PWR_REG,			{ 0x0, 0x0, 0x2} },
	{ EXYNOS5_DIS_IRQ_ARM_CORE0_LOCAL_SYS_PWR_REG,		{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_DIS_IRQ_ARM_CORE0_CENTRAL_SYS_PWR_REG,	{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_ARM_CORE1_SYS_PWR_REG,			{ 0x0, 0x0, 0x2} },
	{ EXYNOS5_DIS_IRQ_ARM_CORE1_LOCAL_SYS_PWR_REG,		{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_DIS_IRQ_ARM_CORE1_CENTRAL_SYS_PWR_REG,	{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_FSYS_ARM_SYS_PWR_REG,				{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_DIS_IRQ_FSYS_ARM_CENTRAL_SYS_PWR_REG,		{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_ISP_ARM_SYS_PWR_REG,				{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_DIS_IRQ_ISP_ARM_LOCAL_SYS_PWR_REG,		{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_DIS_IRQ_ISP_ARM_CENTRAL_SYS_PWR_REG,		{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_ARM_COMMON_SYS_PWR_REG,			{ 0x0, 0x0, 0x2} },
	{ EXYNOS5_ARM_L2_SYS_PWR_REG,				{ 0x0, 0x0, 0x3} },
	{ EXYNOS5_CMU_ACLKSTOP_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SCLKSTOP_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_CMU_ACLKSTOP_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SCLKSTOP_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_SYSMEM_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_DRAM_FREQ_DOWN_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_DDRPHY_DLLOFF_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_DDRPHY_DLLLOCK_SYS_PWR_REG,			{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_APLL_SYSCLK_SYS_PWR_REG,			{ 0x0, 0x0, 0x0} },
	{ EXYNOS5_MPLL_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_VPLL_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_EPLL_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_BPLL_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CPLL_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_MPLLUSER_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_BPLLUSER_SYSCLK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_TOP_BUS_SYS_PWR_REG,				{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_TOP_RETENTION_SYS_PWR_REG,			{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_TOP_PWR_SYS_PWR_REG,				{ 0x3, 0x0, 0x3} },
	{ EXYNOS5_TOP_BUS_SYSMEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_TOP_RETENTION_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_TOP_PWR_SYSMEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x3} },
	{ EXYNOS5_LOGIC_RESET_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_OSCCLK_GATE_SYS_PWR_REG,			{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_LOGIC_RESET_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_OSCCLK_GATE_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_USBOTG_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_G2D_MEM_SYS_PWR_REG,				{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_USBDRD_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_SDMMC_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_CSSYS_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_SECSS_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_ROTATOR_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_INTRAM_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_INTROM_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_JPEG_MEM_SYS_PWR_REG,				{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_HSI_MEM_SYS_PWR_REG,				{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_MCUIOP_MEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_SATA_MEM_SYS_PWR_REG,				{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_DRAM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_MAU_SYS_PWR_REG,		{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_PAD_RETENTION_GPIO_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_UART_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_MMCA_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_MMCB_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_EBIA_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_EBIB_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_SPI_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_RETENTION_GPIO_SYSMEM_SYS_PWR_REG,	{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_ISOLATION_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_ISOLATION_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_PAD_ALV_SEL_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_EXT_REGULATOR_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_GPIO_MODE_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_GPIO_MODE_SYSMEM_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_GPIO_MODE_MAU_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_TOP_ASB_RESET_SYS_PWR_REG,			{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_TOP_ASB_ISOLATION_SYS_PWR_REG,		{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_GSCL_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_ISP_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_MFC_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_G3D_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_DISP1_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_MAU_SYS_PWR_REG,				{ 0x7, 0x7, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_GSCL_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_ISP_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_MFC_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_G3D_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_DISP1_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_MAU_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_GSCL_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_ISP_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_MFC_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_G3D_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_DISP1_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_MAU_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_CMU_RESET_GSCL_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_ISP_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_MFC_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_G3D_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_DISP1_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_MAU_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_XXTI_SYS_PWR_REG,				{ 0x1, 0x1, 0x0} },
};

static struct exynos4_pmu_conf exynos52xx_pmu_config_gps[] = {
	/* { .reg = address, .val = { AFTR, LPA, SLEEP } */
	{ EXYNOS5_CMU_ACLKSTOP_SYS_PWR_REG,			{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_CMU_SCLKSTOP_SYS_PWR_REG,			{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_CMU_ACLKSTOP_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_CMU_SCLKSTOP_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x1} },
	{ EXYNOS5_DRAM_FREQ_DOWN_SYS_PWR_REG,			{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_DDRPHY_DLLOFF_SYS_PWR_REG,			{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_GPS_SYS_PWR_REG,				{ 0x7, 0x0, 0x0} },
	{ EXYNOS5_CMU_CLKSTOP_GPS_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_SYSCLK_GPS_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_CMU_RESET_GPS_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_XXTI_SYS_PWR_REG,				{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_ARM_L2_SYS_PWR_REG,				{ 0x3, 0x3, 0x3} },
};

static struct exynos4_pmu_conf exynos52xx_pmu_c2c_config[] = {
	/* { .reg = address, .val = { AFTR, LPA, SLEEP } */
	{ EXYNOS5_CMU_RESET_SYSMEM_SYS_PWR_REG,			{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_DDRPHY_DLLLOCK_SYS_PWR_REG,			{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_TOP_RETENTION_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_TOP_PWR_SYSMEM_SYS_PWR_REG,			{ 0x3, 0x0, 0x0} },
	{ EXYNOS5_LOGIC_RESET_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x1, 0x1} },
	{ EXYNOS5_OSCCLK_GATE_SYSMEM_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
	{ EXYNOS5_TOP_ASB_RESET_SYS_PWR_REG,			{ 0x1, 0x1, 0x0} },
	{ EXYNOS5_TOP_ASB_ISOLATION_SYS_PWR_REG,		{ 0x1, 0x0, 0x0} },
};

void __iomem *list_both_cnt_feed[] = {
	EXYNOS5_ARM_CORE0_OPTION,
	EXYNOS5_ARM_CORE1_OPTION,
	EXYNOS5_ARM_COMMON_OPTION,
	EXYNOS5_GSCL_OPTION,
	EXYNOS5_ISP_OPTION,
	EXYNOS5_MFC_OPTION,
	EXYNOS5_G3D_OPTION,
	EXYNOS5_DISP1_OPTION,
	EXYNOS5_MAU_OPTION,
	EXYNOS5_TOP_PWR_OPTION,
	EXYNOS5_TOP_PWR_SYSMEM_OPTION,
};

void __iomem *list_diable_wfi_wfe[] = {
	EXYNOS5_ARM_CORE1_OPTION,
	EXYNOS5_FSYS_ARM_OPTION,
	EXYNOS5_ISP_ARM_OPTION,
};

static void exynos5_init_pmu(void)
{
	unsigned int i;
	unsigned int tmp;

	/*
	 * Enable both SC_FEEDBACK and SC_COUNTER
	 */
	for (i = 0 ; i < ARRAY_SIZE(list_both_cnt_feed) ; i++) {
		tmp = __raw_readl(list_both_cnt_feed[i]);
		tmp |= (EXYNOS5_USE_SC_FEEDBACK |
			EXYNOS5_USE_SC_COUNTER);
		__raw_writel(tmp, list_both_cnt_feed[i]);
	}

	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		tmp = __raw_readl(EXYNOS5_GPS_OPTION);
		tmp |= (EXYNOS5_USE_SC_FEEDBACK |
			EXYNOS5_USE_SC_COUNTER);
		__raw_writel(tmp, EXYNOS5_GPS_OPTION);
	}

	/*
	 * SKIP_DEACTIVATE_ACEACP_IN_PWDN_BITFIELD Enable
	 * MANUAL_L2RSTDISABLE_CONTROL_BITFIELD Enable
	 */
	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		tmp = __raw_readl(EXYNOS5_ARM_COMMON_OPTION);
		tmp |= (EXYNOS5_MANUAL_L2RSTDISABLE_CONTROL |
			EXYNOS5_SKIP_DEACTIVATE_ACEACP_IN_PWDN);
		__raw_writel(tmp, EXYNOS5_ARM_COMMON_OPTION);
	} else {
		tmp = __raw_readl(EXYNOS5_ARM_COMMON_OPTION);
		tmp |= EXYNOS5_SKIP_DEACTIVATE_ACEACP_IN_PWDN;
		__raw_writel(tmp, EXYNOS5_ARM_COMMON_OPTION);
	}

	/*
	 * Disable WFI/WFE on XXX_OPTION
	 */
	for (i = 0 ; i < ARRAY_SIZE(list_diable_wfi_wfe) ; i++) {
		tmp = __raw_readl(list_diable_wfi_wfe[i]);
		tmp &= ~(EXYNOS5_OPTION_USE_STANDBYWFE |
			 EXYNOS5_OPTION_USE_STANDBYWFI);
		__raw_writel(tmp, list_diable_wfi_wfe[i]);
	}
}

void exynos5_pmu_xclkout_set(unsigned int enable, enum xclkout_select source)
{
	unsigned int tmp;

	if (enable) {
		tmp = __raw_readl(S5P_PMU_DEBUG);
		/* CLKOUT enable */
		tmp &= ~ (0xF << S5P_PMU_CLKOUT_SEL_SHIFT | S5P_CLKOUT_DISABLE);
		tmp |= (source << S5P_PMU_CLKOUT_SEL_SHIFT);
		__raw_writel(tmp, S5P_PMU_DEBUG);
	} else {
		tmp = __raw_readl(S5P_PMU_DEBUG);
		/* CLKOUT disable */
		tmp |= S5P_CLKOUT_DISABLE;
		__raw_writel(tmp, S5P_PMU_DEBUG);
	}

	printk(KERN_DEBUG "pmu_debug: 0x%08x\n", __raw_readl(S5P_PMU_DEBUG));
}
EXPORT_SYMBOL_GPL(exynos5_pmu_xclkout_set);

void exynos5_sys_powerdown_xxti_control(unsigned int enable)
{
	unsigned int count = entry_cnt;

	if (enable)
		exynos5_pmu_config[count - 1].val[SYS_SLEEP] = 0x1;
	else
		exynos5_pmu_config[count - 1].val[SYS_SLEEP] = 0x0;

	printk(KERN_DEBUG "xxti_control: %ld\n",
			exynos5_pmu_config[count - 1].val[SYS_SLEEP]);
}
EXPORT_SYMBOL_GPL(exynos5_sys_powerdown_xxti_control);


void exynos5_sys_powerdown_conf(enum sys_powerdown mode)
{
	unsigned int count = entry_cnt;
	unsigned int i;

	exynos5_init_pmu();

	for (; count > 0; count--)
		__raw_writel(exynos5_pmu_config[count - 1].val[mode],
				exynos5_pmu_config[count - 1].reg);

	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		for (i = 0; i < ARRAY_SIZE(exynos52xx_pmu_config_gps); i++) {
			__raw_writel(exynos52xx_pmu_config_gps[i].val[mode],
					exynos52xx_pmu_config_gps[i].reg);
		}

	}

	if ((mode != SYS_AFTR) && (exynos4_is_c2c_use())) {
		pr_info("%s power mode enter with C2C Enabling\n"
				, (mode == SYS_LPA) ? "LPA" : "SLEEP");

		for (i = 0; i < ARRAY_SIZE(exynos52xx_pmu_c2c_config); i++) {
			__raw_writel(exynos52xx_pmu_c2c_config[i].val[mode],
					exynos52xx_pmu_c2c_config[i].reg);
		}
	}
}

static int __init exynos5_pmu_init(void)
{
	exynos5_pmu_config = exynos52xx_pmu_config;
	entry_cnt = ARRAY_SIZE(exynos52xx_pmu_config);
	printk(KERN_INFO "%s: PMU supports 52XX(%d)\n"
				, __func__, entry_cnt);

	return 0;
}
arch_initcall(exynos5_pmu_init);
