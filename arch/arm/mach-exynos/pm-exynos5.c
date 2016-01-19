/* linux/arch/arm/mach-exynos/pm-exynos5.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS5 - Power Management support
 *
 * Based on arch/arm/mach-s3c2410/pm.c
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <asm/cacheflush.h>

#include <plat/cpu.h>
#include <plat/pm.h>
#include <plat/bts.h>

#include <mach/regs-irq.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-pmu5.h>
#include <mach/pm-core.h>
#include <mach/pmu.h>
#include <mach/smc.h>

#include <mach/map-exynos5.h>

void (*exynos5_sleep_gpio_table_set)(void);

#ifdef CONFIG_ARM_TRUSTZONE
#define REG_INFORM0            (S5P_VA_SYSRAM_NS + 0x8)
#define REG_INFORM1            (S5P_VA_SYSRAM_NS + 0xC)
#else
#define REG_INFORM0            (EXYNOS5_INFORM0)
#define REG_INFORM1            (EXYNOS5_INFORM1)
#endif

static struct sleep_save exynos5_set_clksrc[] = {
	{ .reg = EXYNOS5_CLKSRC_MASK_TOP		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_GSCL		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_DISP1_0		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_MAUDIO		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_FSYS		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_PERIC0		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_PERIC1		, .val = 0xffffffff, },
	{ .reg = EXYNOS5_CLKSRC_MASK_ISP		, .val = 0xffffffff, },
};

static struct sleep_save exynos5_core_save[] = {
	/* GIC side */
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x000),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x004),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x008),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x00C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x000),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x004),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x100),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x104),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x108),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x10C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x110),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x300),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x304),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x308),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x30C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x310),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x400),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x404),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x408),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x40C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x410),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x414),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x418),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x41C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x420),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x424),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x428),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x42C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x430),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x434),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x438),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x43C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x440),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x444),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x448),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x44C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x450),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x454),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x458),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x45C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x460),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x464),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x468),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x46C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x470),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x474),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x478),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x47C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x480),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x484),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x488),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x48C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x490),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x494),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x498),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x49C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0x800),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x804),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x808),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x80C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x810),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x814),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x818),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x81C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x820),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x824),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x828),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x82C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x830),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x834),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x838),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x83C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x840),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x844),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x848),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x84C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x850),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x854),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x858),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x85C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x860),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x864),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x868),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x86C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x870),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x874),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x878),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x87C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x880),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x884),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x888),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x88C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x890),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x894),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x898),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x89C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC00),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC04),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC08),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC0C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC10),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC14),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC18),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC1C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC20),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC24),

	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x000),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x010),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x020),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x030),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x040),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x050),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x060),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x070),

	SAVE_ITEM(S3C_VA_SYS + 0x234),
};

static struct sleep_save exynos5_regs_save[] = {
	/* Common GPIO Part1 */
        SAVE_ITEM(S5P_VA_GPIO + 0x700),
        SAVE_ITEM(S5P_VA_GPIO + 0x704),
        SAVE_ITEM(S5P_VA_GPIO + 0x708),
        SAVE_ITEM(S5P_VA_GPIO + 0x70C),
        SAVE_ITEM(S5P_VA_GPIO + 0x710),
        SAVE_ITEM(S5P_VA_GPIO + 0x714),
        SAVE_ITEM(S5P_VA_GPIO + 0x718),
        SAVE_ITEM(S5P_VA_GPIO + 0x71C),
        SAVE_ITEM(S5P_VA_GPIO + 0x720),
        SAVE_ITEM(S5P_VA_GPIO + 0x724),
        SAVE_ITEM(S5P_VA_GPIO + 0x728),
        SAVE_ITEM(S5P_VA_GPIO + 0x72C),
        SAVE_ITEM(S5P_VA_GPIO + 0x730),
        SAVE_ITEM(S5P_VA_GPIO + 0x900),
        SAVE_ITEM(S5P_VA_GPIO + 0x904),
        SAVE_ITEM(S5P_VA_GPIO + 0x908),
        SAVE_ITEM(S5P_VA_GPIO + 0x90C),
        SAVE_ITEM(S5P_VA_GPIO + 0x910),
        SAVE_ITEM(S5P_VA_GPIO + 0x914),
        SAVE_ITEM(S5P_VA_GPIO + 0x918),
        SAVE_ITEM(S5P_VA_GPIO + 0x91C),
        SAVE_ITEM(S5P_VA_GPIO + 0x920),
        SAVE_ITEM(S5P_VA_GPIO + 0x924),
        SAVE_ITEM(S5P_VA_GPIO + 0x928),
        SAVE_ITEM(S5P_VA_GPIO + 0x92C),
        SAVE_ITEM(S5P_VA_GPIO + 0x930),
	/* Common GPIO Part2 */
        SAVE_ITEM(S5P_VA_GPIO2 + 0x700),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x704),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x708),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x70C),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x710),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x714),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x718),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x71C),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x720),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x900),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x904),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x908),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x90C),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x910),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x914),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x918),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x91C),
        SAVE_ITEM(S5P_VA_GPIO2 + 0x920),
};

void exynos5_cpu_suspend(void)
{
	unsigned int tmp;

	/* Disable wakeup by EXT_GIC */
	tmp = __raw_readl(EXYNOS5_WAKEUP_MASK);
	tmp |= EXYNOS5_DEFAULT_WAKEUP_MACK;
	__raw_writel(tmp, EXYNOS5_WAKEUP_MASK);

	/*
	 * GPS LPI mask.
	 */
	if (samsung_rev() < EXYNOS5250_REV_1_0)
		__raw_writel(0x10000, EXYNOS5_GPS_LPI);

	if (samsung_rev() >= EXYNOS5250_REV_1_0)
		exynos4_reset_assert_ctrl(0);

#ifdef CONFIG_ARM_TRUSTZONE
	exynos_smc(SMC_CMD_SLEEP, 0, 0, 0);
#else
	/* issue the standby signal into the pm unit. */
	cpu_do_idle();
#endif
}

static void exynos5_pm_prepare(void)
{
	unsigned int tmp;

	if (exynos5_sleep_gpio_table_set)
		exynos5_sleep_gpio_table_set();

	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		/* Disable USE_RETENTION of JPEG_MEM_OPTION */
		tmp = __raw_readl(EXYNOS5_JPEG_MEM_OPTION);
		tmp &= ~EXYNOS5_OPTION_USE_RETENTION;
		__raw_writel(tmp, EXYNOS5_JPEG_MEM_OPTION);
	}

	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		tmp = __raw_readl(EXYNOS5_ARM_L2_OPTION);
		tmp &= ~(1 << 4);
		__raw_writel(tmp, EXYNOS5_ARM_L2_OPTION);
	}

	/* Set value of power down register for sleep mode */
	exynos5_sys_powerdown_conf(SYS_SLEEP);
	__raw_writel(S5P_CHECK_SLEEP, REG_INFORM1);

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s3c_cpu_resume), REG_INFORM0);

	if (exynos4_is_c2c_use()) {
		tmp = __raw_readl(EXYNOS5_INTRAM_MEM_OPTION);
		tmp &= ~EXYNOS5_OPTION_USE_RETENTION;
		__raw_writel(tmp, EXYNOS5_INTRAM_MEM_OPTION);
	}

	s3c_pm_do_restore_core(exynos5_set_clksrc, ARRAY_SIZE(exynos5_set_clksrc));
}

static int exynos5_pm_add(struct sys_device *sysdev)
{
	pm_cpu_prep = exynos5_pm_prepare;
	pm_cpu_sleep = exynos5_cpu_suspend;

	return 0;
}

static struct sysdev_driver exynos5_pm_driver = {
	.add		= exynos5_pm_add,
};

static __init int exynos5_pm_drvinit(void)
{
	s3c_pm_init();

	return sysdev_driver_register(&exynos5_sysclass, &exynos5_pm_driver);
}
arch_initcall(exynos5_pm_drvinit);

static void exynos5_show_wakeup_reason_eint(void)
{
	int bit, i;
	long unsigned int ext_int_pend;
	unsigned long eint_wakeup_mask;
	bool found = 0;

	eint_wakeup_mask = __raw_readl(EXYNOS5_EINT_WAKEUP_MASK);

	for (i = 0; i <= 4; i++) {
		ext_int_pend = __raw_readl(S5P_EINT_PEND(i));

		for_each_set_bit(bit, &ext_int_pend, 8) {
			int irq = IRQ_EINT(i * 8) + bit;
			struct irq_desc *desc = irq_to_desc(irq);

			if (eint_wakeup_mask & (1 << (i * 8 + bit)))
				continue;

			if (desc && desc->action && desc->action->name)
				pr_info("Resume caused by IRQ %d, %s\n", irq,
					desc->action->name);
			else
				pr_info("Resume caused by IRQ %d\n", irq);

			found = 1;
		}
	}

	if (!found)
		pr_info("Resume caused by unknown EINT\n");
}

static void exynos5_show_wakeup_reason(void)
{
	unsigned long wakeup_stat;

	wakeup_stat = __raw_readl(EXYNOS5_WAKEUP_STAT);

	if (wakeup_stat & EXYNOS5_MASK_RTC_ALARM)
		pr_info("Resume caused by RTC alarm\n");
	else if (wakeup_stat & EXYNOS5_MASK_EINT)
		exynos5_show_wakeup_reason_eint();
	else
		pr_info("Resume caused by wakeup_stat=0x%08lx\n",
			wakeup_stat);
}

bool isp_pwr_off;

static int exynos5_pm_suspend(void)
{
	unsigned long tmp;
	u32 timeout;

	s3c_pm_do_save(exynos5_core_save, ARRAY_SIZE(exynos5_core_save));

	s3c_pm_do_save(exynos5_regs_save, ARRAY_SIZE(exynos5_regs_save));

	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		if (!(__raw_readl(EXYNOS5_ISP_STATUS) & S5P_INT_LOCAL_PWR_EN)) {
			isp_pwr_off = true;
			/*
			 * Before enter suspend, ISP power domain should be on
			 */
			__raw_writel(S5P_INT_LOCAL_PWR_EN,
					EXYNOS5_ISP_CONFIGURATION);
			timeout = 1000;

			while (!(__raw_readl(EXYNOS5_ISP_STATUS) & S5P_INT_LOCAL_PWR_EN)) {
				if (timeout == 0) {
					printk(KERN_ERR "ISP power domain can not on\n");
				}
				timeout--;
				udelay(1);
			}
		}
	}

	tmp = __raw_readl(EXYNOS5_CENTRAL_SEQ_CONFIGURATION);
	tmp &= ~(EXYNOS5_CENTRAL_LOWPWR_CFG);
	__raw_writel(tmp, EXYNOS5_CENTRAL_SEQ_CONFIGURATION);

	tmp = __raw_readl(EXYNOS5_CENTRAL_SEQ_OPTION);

	tmp = (EXYNOS5_USE_STANDBYWFI_ARM_CORE0 |
		EXYNOS5_USE_STANDBYWFE_ARM_CORE0);

	__raw_writel(tmp, EXYNOS5_CENTRAL_SEQ_OPTION);

	return 0;
}

static void exynos5_pm_resume(void)
{
	unsigned long tmp, srctmp;
	u32 timeout;

	if (samsung_rev() >= EXYNOS5250_REV_1_0)
		exynos4_reset_assert_ctrl(1);

	/* If PMU failed while entering sleep mode, WFI will be
	 * ignored by PMU and then exiting cpu_do_idle().
	 * EXYNOS5_CENTRAL_SEQ_CONFIGURATION bit will not be set
	 * automatically in this situation.
	 */
	tmp = __raw_readl(EXYNOS5_CENTRAL_SEQ_CONFIGURATION);

	if (!(tmp & EXYNOS5_CENTRAL_LOWPWR_CFG)) {
		tmp |= EXYNOS5_CENTRAL_LOWPWR_CFG;
		__raw_writel(tmp, EXYNOS5_CENTRAL_SEQ_CONFIGURATION);
		/* No need to perform below restore code */
		goto early_wakeup;
	}

	if ((samsung_rev() < EXYNOS5250_REV_1_0) && isp_pwr_off) {
		srctmp = __raw_readl(EXYNOS5_CLKSRC_TOP3);
		/*
		 * To ISP power domain off,
		 * first, ISP_ARM power domain be off.
		 */
		if ((__raw_readl(EXYNOS5_ISP_ARM_STATUS) & 0x1)) {
			/* Disable ISP_ARM */
			timeout = __raw_readl(EXYNOS5_ISP_ARM_OPTION);
			timeout &= ~EXYNOS5_ISP_ARM_ENABLE;
			__raw_writel(timeout, EXYNOS5_ISP_ARM_OPTION);

			/* ISP_ARM power off */
			__raw_writel(0x0, EXYNOS5_ISP_ARM_CONFIGURATION);

			timeout = 1000;

			while (__raw_readl(EXYNOS5_ISP_ARM_STATUS) & 0x1) {
				if (timeout == 0) {
					printk(KERN_ERR "ISP_ARM power domain can not off\n");
					return;
				}
				timeout--;
				udelay(1);
			}
			/* CMU_RESET_ISP_ARM off */
			__raw_writel(0x0, EXYNOS5_CMU_RESET_ISP_SYS_PWR_REG);
		}

		__raw_writel(0x0, EXYNOS5_ISP_CONFIGURATION);

		/* Wait max 1ms */
		timeout = 1000;
		while (__raw_readl(EXYNOS5_ISP_STATUS) & S5P_INT_LOCAL_PWR_EN) {
			if (timeout == 0) {
				printk(KERN_ERR "Power domain ISP disable failed.\n");
				return;
			}
			timeout--;
			udelay(1);
		}

		__raw_writel(srctmp, EXYNOS5_CLKSRC_TOP3);

		isp_pwr_off = false;
	}

	/* For release retention */
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_MAU_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_GPIO_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_UART_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_MMCA_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_MMCB_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_EBIA_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_EBIB_OPTION);
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_SPI_OPTION);
	
	/* For Retention release on GPV block */
	__raw_writel((1 << 28), EXYNOS5_PAD_RETENTION_GPIO_SYSMEM_OPTION);

	/* Disable CPU_nIRQ[0:1] */
	tmp = ((0x1 << 8) | (0x1 << 0));
	__raw_writel(tmp, S5P_VA_COMBINER_BASE + 0x54);

	bts_enable(PD_TOP);

	s3c_pm_do_restore(exynos5_regs_save, ARRAY_SIZE(exynos5_regs_save));

	s3c_pm_do_restore_core(exynos5_core_save, ARRAY_SIZE(exynos5_core_save));

early_wakeup:
	__raw_writel(0x0, REG_INFORM1);
	exynos5_show_wakeup_reason();
}

static struct syscore_ops exynos5_pm_syscore_ops = {
	.suspend	= exynos5_pm_suspend,
	.resume		= exynos5_pm_resume,
};

static __init int exynos5_pm_syscore_init(void)
{
	register_syscore_ops(&exynos5_pm_syscore_ops);

	return 0;
}
arch_initcall(exynos5_pm_syscore_init);
