/* linux/arch/arm/mach-exynos/dev-c2c.c
 *
 * Copyright (c) 2012 Samsung Electronics
 *
 * Base EXYNOS C2C resource and device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/cpu.h>

#include <mach/gpio-exynos4.h>
#include <mach/map.h>
#include <mach/regs-pmu.h>
#include <mach/pmu.h>
#include <mach/c2c.h>

static struct resource exynos_c2c_resource[] = {
	[0] = {
		.start  = EXYNOS_PA_C2C,
		.end    = EXYNOS_PA_C2C + SZ_64K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = EXYNOS_PA_C2C_CP,
		.end    = EXYNOS_PA_C2C_CP + SZ_64K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = IRQ_C2C_SSCM0,
		.end    = IRQ_C2C_SSCM0,
		.flags  = IORESOURCE_IRQ,
	},
	[3] = {
		.start  = IRQ_C2C_SSCM1,
		.end    = IRQ_C2C_SSCM1,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 exynos_c2c_dma_mask = DMA_BIT_MASK(32);

struct platform_device exynos_device_c2c = {
	.name = "samsung-c2c",
	.id = -1,
	.num_resources = ARRAY_SIZE(exynos_c2c_resource),
	.resource = exynos_c2c_resource,
	.dev = {
		.dma_mask = &exynos_c2c_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

#ifdef CONFIG_C2C_IPC_ENABLE
struct exynos_c2c_platdata smdk4412_c2c_pdata = {
	.setup_gpio = NULL,
	.shdmem_addr = 0,
#ifdef CONFIG_C2C_IPC_ONLY
	.shdmem_size = C2C_MEMSIZE_4,
#else
	.shdmem_size = C2C_MEMSIZE_64,
#endif
	.ap_sscm_addr = NULL,
	.cp_sscm_addr = NULL,
	.rx_width = C2C_BUSWIDTH_8,
	.tx_width = C2C_BUSWIDTH_8,
	.clk_opp100 = 133,
	.clk_opp50 = 66,
	.clk_opp25 = 33,
	.default_opp_mode = C2C_OPP100,
	.get_c2c_state = NULL,
	.c2c_sysreg = S5P_VA_CMU + 0x12000,
};
#else /*!CONFIG_C2C_IPC_ENABLE*/
struct exynos_c2c_platdata smdk4412_c2c_pdata = {
	.setup_gpio = NULL,
	.shdmem_addr = C2C_SHAREDMEM_BASE,
	.shdmem_size = C2C_MEMSIZE_64,
	.ap_sscm_addr = NULL,
	.cp_sscm_addr = NULL,
	.rx_width = C2C_BUSWIDTH_16,
	.tx_width = C2C_BUSWIDTH_16,
	.clk_opp100 = 400,
	.clk_opp50 = 266,
	.clk_opp25 = 0,
	.default_opp_mode = C2C_OPP50,
	.get_c2c_state	= NULL,
	.c2c_sysreg = S5P_VA_CMU + 0x12000,
};
#endif /*CONFIG_C2C_IPC_ENABLE*/

struct exynos_c2c_platdata smdk5250_c2c_pdata = {
	.setup_gpio = NULL,
	.shdmem_addr = C2C_SHAREDMEM_BASE,
	.shdmem_size = C2C_MEMSIZE_64,
	.ap_sscm_addr = NULL,
	.cp_sscm_addr = NULL,
	.rx_width = C2C_BUSWIDTH_16,
	.tx_width = C2C_BUSWIDTH_16,
	.clk_opp100 = 400,
	.clk_opp50 = 200,
	.clk_opp25 = 100,
	.default_opp_mode = C2C_OPP25,
	.get_c2c_state	= NULL,
	.c2c_sysreg = S5P_VA_CMU + 0x6000,
};

static void exynos4_c2c_config_gpio(enum c2c_buswidth rx_width,
			enum c2c_buswidth tx_width, void __iomem *etc8drv_addr)
{
	int i;
	s5p_gpio_pd_cfg_t pd_cfg = S5P_GPIO_PD_PREV_STATE;
	s5p_gpio_pd_pull_t pd_pull = S5P_GPIO_PD_DOWN_ENABLE;
	s5p_gpio_drvstr_t lv1 = S5P_GPIO_DRVSTR_LV1;
	s5p_gpio_drvstr_t lv3 = S5P_GPIO_DRVSTR_LV3;

	/* Set GPIO for C2C Rx */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV0(0), 8, C2C_SFN);
	for (i = 0; i < 8; i++) {
		s5p_gpio_set_drvstr(EXYNOS4212_GPV0(i), lv1);
		s5p_gpio_set_pd_cfg(EXYNOS4212_GPV0(i), pd_cfg);
		s5p_gpio_set_pd_pull(EXYNOS4212_GPV0(i), pd_pull);
	}

	if (rx_width == C2C_BUSWIDTH_16) {
		s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV1(0), 8, C2C_SFN);
		for (i = 0; i < 8; i++) {
			s5p_gpio_set_drvstr(EXYNOS4212_GPV1(i), lv1);
			s5p_gpio_set_pd_cfg(EXYNOS4212_GPV1(i), pd_cfg);
			s5p_gpio_set_pd_pull(EXYNOS4212_GPV1(i), pd_pull);
		}
	} else if (rx_width == C2C_BUSWIDTH_10) {
		s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV1(0), 2, C2C_SFN);
		for (i = 0; i < 2; i++) {
			s5p_gpio_set_drvstr(EXYNOS4212_GPV1(i), lv1);
			s5p_gpio_set_pd_cfg(EXYNOS4212_GPV1(i), pd_cfg);
			s5p_gpio_set_pd_pull(EXYNOS4212_GPV1(i), pd_pull);
		}
	}

	/* Set GPIO for C2C Tx */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV2(0), 8, C2C_SFN);
	for (i = 0; i < 8; i++) {
		s5p_gpio_set_drvstr(EXYNOS4212_GPV2(i), lv3);
		s5p_gpio_set_pd_cfg(EXYNOS4212_GPV2(i), pd_cfg);
	}

	if (tx_width == C2C_BUSWIDTH_16) {
		s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV3(0), 8, C2C_SFN);
		for (i = 0; i < 8; i++) {
			s5p_gpio_set_drvstr(EXYNOS4212_GPV3(i), lv3);
			s5p_gpio_set_pd_cfg(EXYNOS4212_GPV3(i), pd_cfg);
		}
	} else if (tx_width == C2C_BUSWIDTH_10) {
		s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV3(0), 2, C2C_SFN);
		for (i = 0; i < 2; i++) {
			s5p_gpio_set_drvstr(EXYNOS4212_GPV3(i), lv3);
			s5p_gpio_set_pd_cfg(EXYNOS4212_GPV3(i), pd_cfg);
		}
	}

	/* Set GPIO for WakeReqOut/In */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPV4(0), 2, C2C_SFN);
	s5p_gpio_set_pd_cfg(EXYNOS4212_GPV4(0), pd_cfg);
	s5p_gpio_set_pd_pull(EXYNOS4212_GPV4(0), pd_pull);
	s5p_gpio_set_pd_cfg(EXYNOS4212_GPV4(1), pd_cfg);
	s5p_gpio_set_pd_pull(EXYNOS4212_GPV4(1), pd_pull);

	writel(0x5, etc8drv_addr);
}

static void exynos5_c2c_config_gpio(enum c2c_buswidth rx_width,
			enum c2c_buswidth tx_width, void __iomem *etc8drv_addr)
{
	int i;
	s5p_gpio_pd_cfg_t pd_cfg = S5P_GPIO_PD_PREV_STATE;
	s5p_gpio_pd_pull_t pd_pull = S5P_GPIO_PD_DOWN_ENABLE;
	s5p_gpio_drvstr_t lv1 = S5P_GPIO_DRVSTR_LV1;
	s5p_gpio_drvstr_t lv3 = S5P_GPIO_DRVSTR_LV3;

	/* Set GPIO for C2C Rx */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPV0(0), 8, C2C_SFN);
	for (i = 0; i < 8; i++) {
		s5p_gpio_set_drvstr(EXYNOS5_GPV0(i), lv1);
		s5p_gpio_set_pd_cfg(EXYNOS5_GPV0(i), pd_cfg);
		s5p_gpio_set_pd_pull(EXYNOS5_GPV0(i), pd_pull);
	}

	if (rx_width == C2C_BUSWIDTH_16) {
		s3c_gpio_cfgrange_nopull(EXYNOS5_GPV1(0), 8, C2C_SFN);
		for (i = 0; i < 8; i++) {
			s5p_gpio_set_drvstr(EXYNOS5_GPV1(i), lv1);
			s5p_gpio_set_pd_cfg(EXYNOS5_GPV1(i), pd_cfg);
			s5p_gpio_set_pd_pull(EXYNOS5_GPV1(i), pd_pull);
		}
	} else if (rx_width == C2C_BUSWIDTH_10) {
		s3c_gpio_cfgrange_nopull(EXYNOS5_GPV1(0), 2, C2C_SFN);
		for (i = 0; i < 2; i++) {
			s5p_gpio_set_drvstr(EXYNOS5_GPV1(i), lv1);
			s5p_gpio_set_pd_cfg(EXYNOS5_GPV1(i), pd_cfg);
			s5p_gpio_set_pd_pull(EXYNOS5_GPV1(i), pd_pull);
		}
	}

	/* Set GPIO for C2C Tx */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPV2(0), 8, C2C_SFN);
	for (i = 0; i < 8; i++)
		s5p_gpio_set_drvstr(EXYNOS5_GPV2(i), lv3);

	if (tx_width == C2C_BUSWIDTH_16) {
		s3c_gpio_cfgrange_nopull(EXYNOS5_GPV3(0), 8, C2C_SFN);
		for (i = 0; i < 8; i++)
			s5p_gpio_set_drvstr(EXYNOS5_GPV3(i), lv3);
	} else if (tx_width == C2C_BUSWIDTH_10) {
		s3c_gpio_cfgrange_nopull(EXYNOS5_GPV3(0), 2, C2C_SFN);
		for (i = 0; i < 2; i++)
			s5p_gpio_set_drvstr(EXYNOS5_GPV3(i), lv3);
	}

	/* Set GPIO for WakeReqOut/In */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPV4(0), 2, C2C_SFN);
	s5p_gpio_set_pd_cfg(EXYNOS5_GPV4(0), pd_cfg);
	s5p_gpio_set_pd_pull(EXYNOS5_GPV4(0), pd_pull);
	s5p_gpio_set_pd_cfg(EXYNOS5_GPV4(1), pd_cfg);
	s5p_gpio_set_pd_pull(EXYNOS5_GPV4(1), pd_pull);

	writel(0x5, etc8drv_addr);
}

static void exynos_c2c_set_cprst(void)
{
	/* TODO */
}

static void exynos_c2c_clear_cprst(void)
{
	/* TODO */
}

static void exynos_c2c_config_gpio(enum c2c_buswidth rx_width,
				enum c2c_buswidth tx_width)
{
	void __iomem *etc8drv_addr;

	if (soc_is_exynos4212() || soc_is_exynos4412()) {
		/* ETC8DRV is used for setting Tx clock drive strength */
		etc8drv_addr = S5P_VA_GPIO4 + 0xAC;
		exynos4_c2c_config_gpio(rx_width, tx_width, etc8drv_addr);
	} else if (soc_is_exynos5250()) {
		/* ETC8DRV is used for setting Tx clock drive strength */
		etc8drv_addr = S5P_VA_GPIO3 + 0xAC;
		exynos5_c2c_config_gpio(rx_width, tx_width, etc8drv_addr);
	}
}

static int __init exynos_c2c_device_init(void)
{
	int err = 0;
	struct exynos_c2c_platdata *pd = NULL;

	if (soc_is_exynos4212() || soc_is_exynos4412())
		pd = &smdk4412_c2c_pdata;
	else if (soc_is_exynos5250())
		pd = &smdk5250_c2c_pdata;
	else
		goto exit;

	if (!pd->setup_gpio)
		pd->setup_gpio = exynos_c2c_config_gpio;

	if (!pd->set_cprst)
		pd->set_cprst = exynos_c2c_set_cprst;

	if (!pd->clear_cprst)
		pd->clear_cprst = exynos_c2c_clear_cprst;

	/* Set C2C_CTRL Register */
	writel(0x0, S5P_C2C_CTRL);

	exynos_device_c2c.dev.platform_data = pd;

	err = platform_device_register(&exynos_device_c2c);
	if (err) {
		pr_err("[C2C] ERR! %s platform_device_register fail (err %d)\n",
			exynos_device_c2c.name, err);
		goto exit;
	}

	return 0;

exit:
	pr_err("[C2C] %s: xxx\n", __func__);
	return err;
}
device_initcall(exynos_c2c_device_init);

static int __init exynos_c2c_device_register(void)
{
	int err;

	err = platform_device_register(&exynos_device_c2c);
	if (err) {
		pr_err("[C2C] ERR! %s platform_device_register fail (err %d)\n",
			exynos_device_c2c.name, err);
		goto exit;
	}

	return 0;

exit:
	pr_err("[C2C] %s: xxx\n", __func__);
	return err;
}

