/*
 * arch/arm/mach-exynos/s5p_idpram.h
 * (Header for internal DPRAM in S.LSI S5P-family AP)
 */

#ifndef __S5P_IDPRAM_H__
#define __S5P_IDPRAM_H__

#include <linux/clk.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-srom.h>
#include <mach/gpio.h>
#include <mach/regs-mem.h>

#define IDPRAM_SIZE		0x4000
#define IDPRAM_PHYS_ADDR	0x13A00000
#define IDPRAM_PHYS_END		(IDPRAM_PHYS_ADDR + IDPRAM_SIZE)

#define IDPRAM_SFR_SIZE		0x1C
#define IDPRAM_SFR_PHYS_ADDR	0x13A08000

/* For S5P Interanl DPRAM GPIO configuration */
#define GPIO_IDPRAM_WEn		EXYNOS4210_GPE0(0)
#define GPIO_IDPRAM_CSn		EXYNOS4210_GPE0(1)
#define GPIO_IDPRAM_REn		EXYNOS4210_GPE0(2)
#define GPIO_IDPRAM_INT2CP	EXYNOS4210_GPE0(3)
#define GPIO_IDPRAM_ADVn	EXYNOS4210_GPE0(4)	/* only for MUX mode */

#define GPIO_IDPRAM_ADDR_BUS_L	EXYNOS4210_GPE1(0)
#define GPIO_IDPRAM_ADDR_BUS_H	EXYNOS4210_GPE2(0)
#define GPIO_IDPRAM_DATA_BUS_L	EXYNOS4210_GPE3(0)
#define GPIO_IDPRAM_DATA_BUS_H	EXYNOS4210_GPE4(0)

/* S5P Interanl DPRAM SFR (Special Function Register) fields */
#define IDPRAM_MIFCON_INT2APEN		(1<<2)
#define IDPRAM_MIFCON_INT2MSMEN		(1<<3)
#define IDPRAM_MIFCON_DMATXREQEN_0	(1<<16)
#define IDPRAM_MIFCON_DMATXREQEN_1	(1<<17)
#define IDPRAM_MIFCON_DMARXREQEN_0	(1<<18)
#define IDPRAM_MIFCON_DMARXREQEN_1	(1<<19)
#define IDPRAM_MIFCON_FIXBIT		(1<<20)

#define IDPRAM_MIFPCON_ADM_MODE		(1<<6)	/* MUX/DEMUX mode */

struct s5p_idpram_sfr {
	unsigned int2ap;
	unsigned int2msm;
	unsigned mifcon;
	unsigned mifpcon;
	unsigned msmintclr;
	unsigned dma_tx_adr;
	unsigned dma_rx_adr;
};

static struct s5p_idpram_sfr __iomem *ap_idpram_sfr;

/**
 * idpram_config_demux_gpio
 *
 * Configures GPIO pins for CSn, REn, WEn, INT2CP, address bus, and data bus
 * as demux mode
 */
static void idpram_config_demux_gpio(void)
{
	/* Configure CSn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_CSn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_CSn, S3C_GPIO_PULL_NONE);

	/* Configure REn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_REn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_REn, S3C_GPIO_PULL_NONE);

	/* Configure WEn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_WEn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_WEn, S3C_GPIO_PULL_NONE);

	/* Configure INT2CP */
	s3c_gpio_cfgpin(GPIO_IDPRAM_INT2CP, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_INT2CP, S3C_GPIO_PULL_UP);

	/* Address bus (8 bits + 6 bits = 14 bits) */
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_ADDR_BUS_L, 8, GPIO_IDPRAM_SFN);
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_ADDR_BUS_H, 6, GPIO_IDPRAM_SFN);

	/* Data bus (8 bits + 8 bits = 16 bits)*/
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_DATA_BUS_L, 8, GPIO_IDPRAM_SFN);
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_DATA_BUS_H, 8, GPIO_IDPRAM_SFN);
}

/**
 * idpram_config_mux_gpio
 *
 * Configures GPIO pins for CSn, REn, WEn, INT2CP, ADVn, and ADM bus as mux mode
 */
static void idpram_config_mux_gpio(void)
{
	/* Configure CSn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_CSn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_CSn, S3C_GPIO_PULL_NONE);

	/* Configure REn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_REn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_REn, S3C_GPIO_PULL_NONE);

	/* Configure WEn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_WEn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_WEn, S3C_GPIO_PULL_NONE);

	/* Configure INT2CP */
	s3c_gpio_cfgpin(GPIO_IDPRAM_INT2CP, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_INT2CP, S3C_GPIO_PULL_UP);

	/* Configure ADVn */
	s3c_gpio_cfgpin(GPIO_IDPRAM_ADVn, GPIO_IDPRAM_SFN);
	s3c_gpio_setpull(GPIO_IDPRAM_ADVn, S3C_GPIO_PULL_NONE);

	/* Address and data mux bus (8 bits + 8 bits = 16 bits)*/
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_DATA_BUS_L, 8, GPIO_IDPRAM_SFN);
	s3c_gpio_cfgrange_nopull(GPIO_IDPRAM_DATA_BUS_H, 8, GPIO_IDPRAM_SFN);
}

static int idpram_init_sfr(void)
{
	ap_idpram_sfr = ioremap_nocache(IDPRAM_SFR_PHYS_ADDR, IDPRAM_SFR_SIZE);
	if (!ap_idpram_sfr) {
		pr_err("mif: %s: ioremap fail\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static void idpram_init_demux_mode(void)
{
	ap_idpram_sfr->mifcon = IDPRAM_MIFCON_FIXBIT | IDPRAM_MIFCON_INT2APEN |
				IDPRAM_MIFCON_INT2MSMEN;
}

static void idpram_init_mux_mode(void)
{
	ap_idpram_sfr->mifcon = IDPRAM_MIFCON_FIXBIT | IDPRAM_MIFCON_INT2APEN |
				IDPRAM_MIFCON_INT2MSMEN;
	ap_idpram_sfr->mifpcon = IDPRAM_MIFPCON_ADM_MODE;
}

static int idpram_enable(void)
{
	struct clk *clk;

	/* enable internal DPRAM clock (modem I/F) */
	clk = clk_get(NULL, "modem");
	if (!clk) {
		pr_err("mif: %s: ERR! IDPRAM clock gate fail\n", __func__);
		return -EINVAL;
	}

	clk_enable(clk);
	return 0;
}

static void idpram_clr_intr(void)
{
	ap_idpram_sfr->msmintclr = 0xFF;
}

#endif

