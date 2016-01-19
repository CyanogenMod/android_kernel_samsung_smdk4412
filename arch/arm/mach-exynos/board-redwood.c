/*
 * linux/arch/arm/mach-exynos/board-redwood.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/lcd.h>
#include <linux/lcd-property.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/lsm330dlc_accel.h>
#include <linux/sensor/lsm330dlc_gyro.h>
#include <linux/sensor/ak8975.h>
#include <linux/sensor/ak8963.h>
#include <linux/sensor/cm36651.h>
#include <linux/sensor/gp2a.h>
#include <linux/cma.h>
#include <linux/jack.h>
#include <linux/uart_select.h>
#include <linux/utsname.h>
#include <linux/mfd/max77686.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/leds-max77693.h>
#include <linux/battery/max17047_fuelgauge.h>
#include <linux/power/charger-manager.h>
#include <linux/devfreq/exynos4_bus.h>
#include <linux/pm_qos_params.h>
#include <drm/exynos_drm.h>
#include <linux/printk.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#include <plat/s3c64xx-spi.h>
#include <plat/csis.h>
#include <plat/udc-hs.h>
#include <plat/media.h>
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
#include <media/exynos_fimc_is.h>
#endif
#include <plat/regs-fb.h>
#include <plat/fb-core.h>
#include <plat/mipi_dsim2.h>
#include <plat/fimd_lite_ext.h>
#include <plat/hdmi.h>
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif
#include <plat/fb-s5p.h>

#ifdef CONFIG_I2C_SI4705
#include <linux/si4705_pdata.h>
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#include <mach/map.h>
#include <mach/spi-clocks.h>
#include <mach/sec_debug.h>

#ifdef CONFIG_SND_SOC_WM8994
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#endif

#include <mach/midas-power.h>
#ifdef CONFIG_TOUCHSCREEN_MELFAS_MMS
#include <linux/melfas_mms_ts.h>
#endif
#include <mach/dwmci.h>

#include <mach/bcm47511.h>
#include <mach/bcm4752.h>

#include <mach/regs-pmu.h>

#include <mach/dev-sysmmu.h>

#include "board-mobile.h"

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#ifdef CONFIG_BUSFREQ_OPP
#include <mach/dev.h>
#include <mach/ppmu.h>
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>
#endif
#include <mach/midas-thermistor.h>

#include <linux/host_notify.h>

#ifdef CONFIG_INPUT_SECBRIDGE
#include <linux/input/sec-input-bridge.h>
#endif

enum gpio_i2c {
	I2C_LAST_HW	= 8, /* I2C0~8 are reserved */
	I2C_CODEC	= 9, /* I2C9 is reserved for CODEC (hardcoded) */
	I2C_NFC,
	I2C_3_TOUCH,
	I2C_FUEL,
	I2C_BSENSE,
	I2C_MSENSE,
	I2C_MHL		= 15, /* 15 is hardcoded from midas-mhl.c */
	I2C_MHL_D	= 16, /* 16 is hardcoded from midas-mhl.c */
	I2C_PSENSE,
	I2C_IF_PMIC,
	I2C_FM_RADIO	= 19, /* refer from midas */
};

enum board_rev {
	REDWOOD_REV0_1_0425 = 0xc,
	REDWOOD_REV0_1_0704 = 0x4,
};

static int hwrevision(int rev)
{
	switch (rev) {
	case 0: return (system_rev == 0x3);
	case 1: return (system_rev == 0x0);
	}
	return 0;
}

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SLP_MIDAS_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SLP_MIDAS_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SLP_MIDAS_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg slp_midas_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SLP_MIDAS_UCON_DEFAULT,
		.ulcon		= SLP_MIDAS_ULCON_DEFAULT,
		.ufcon		= SLP_MIDAS_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SLP_MIDAS_UCON_DEFAULT,
		.ulcon		= SLP_MIDAS_ULCON_DEFAULT,
		.ufcon		= SLP_MIDAS_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SLP_MIDAS_UCON_DEFAULT,
		.ulcon		= SLP_MIDAS_ULCON_DEFAULT,
		.ufcon		= SLP_MIDAS_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SLP_MIDAS_UCON_DEFAULT,
		.ulcon		= SLP_MIDAS_ULCON_DEFAULT,
		.ufcon		= SLP_MIDAS_UFCON_DEFAULT,
	},
};

#if defined(CONFIG_S3C64XX_DEV_SPI)
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "s5c73m3_spi",
		.platform_data = NULL,
		.max_speed_hz = 50000000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
#define DIV_FSYS3	(S5P_VA_CMU + 0x0C54C)
static void exynos_dwmci_cfg_gpio(int width)
{
	unsigned int gpio;

	for (gpio = EXYNOS4_GPK0(0); gpio < EXYNOS4_GPK0(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
	}

	width = (1 << width);

	switch (width) {
	case 8:
		for (gpio = EXYNOS4_GPK1(3); gpio <= EXYNOS4_GPK1(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(4));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		}
	case 4:
		for (gpio = EXYNOS4_GPK0(3); gpio <= EXYNOS4_GPK0(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		}
		break;
	case 1:
		gpio = EXYNOS4_GPK0(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);

		/* Workaround
		 * NOTE : In future, should be modified bootloader
		 * Set this value when 1-bit buswidth(it's initial time)*/
		__raw_writel(0x1, DIV_FSYS3);
	default:
		break;
	}
}

/*
 * block setting of dwmci
 * max_segs = PAGE_SIZE / size of IDMAC desc,
 * max_blk_size = 512,
 * max_blk_count = 65536,
 * max_seg_size = PAGE_SIZE,
 * max_req_size = max_seg_size * max_blk_count
 **/
static struct block_settings exynos_dwmci_blk_setting = {
	.max_segs		= 0x1000,
	.max_blk_size		= 0x200,
	.max_blk_count		= 0x10000,
	.max_seg_size		= 0x1000,
	.max_req_size		= 0x1000 * 0x10000,
};

static struct dw_mci_board exynos_dwmci_pdata __initdata = {
	.num_slots		= 1,
	.quirks			= DW_MCI_QUIRK_BROKEN_CARD_DETECTION |
				DW_MCI_QUIRK_HIGHSPEED,
	.bus_hz			= 100 * 1000 * 1000,
	.caps			= MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR |
				MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	.caps2			= MMC_CAP2_PACKED_CMD,
	.detect_delay_ms	= 200,
	.hclk_name		= "dwmci",
	.cclk_name		= "sclk_dwmci",
	.cfg_gpio		= exynos_dwmci_cfg_gpio,
	.blk_settings		= &exynos_dwmci_blk_setting,
	.buf_size		= PAGE_SIZE << 4,
};
#else
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
	.cd_type                = S3C_MSHCI_CD_PERMANENT,
	.fifo_depth		= 0x80,
	.max_width              = 8,
	.host_caps              = MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
				  MMC_CAP_UHS_DDR50 | MMC_CAP_CMD23,
	.host_caps2		= MMC_CAP2_PACKED_CMD,
};
#endif

static struct s3c_sdhci_platdata slp_midas_hsmmc2_pdata __initdata = {
	.cd_type                = S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio            = EXYNOS4_GPX3(4),
	.ext_cd_gpio_invert	= true,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA,
	.vmmc_name		= "vtf_2.8v",
};

static DEFINE_MUTEX(notify_lock);

/* FIXME: For coexistence of both slp-pq and redwood board. Need to fix */
#define DEFINE_MMC_CARD_NOTIFIER(num) \
extern void (*hsmmc##num##_notify_func)(struct platform_device *, int state); \
static int ext_cd_init_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func); \
	hsmmc##num##_notify_func = notify_func; \
	mutex_unlock(&notify_lock); \
	return 0; \
} \
static int ext_cd_cleanup_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func != notify_func); \
	hsmmc##num##_notify_func = NULL; \
	mutex_unlock(&notify_lock); \
	return 0; \
}

DEFINE_MMC_CARD_NOTIFIER(3)

/*
 * call this when you need sd stack to recognize insertion or removal of card
 * that can't be told by SDHCI regs
 */

static struct s3c_sdhci_platdata slp_midas_hsmmc3_pdata __initdata = {
/* new code for brm4334 */
	.cd_type	= S3C_SDHCI_CD_EXTERNAL,
	.clk_type	= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags	= S3C_SDHCI_PM_IGNORE_SUSPEND_RESUME,
	.ext_cd_init	= ext_cd_init_hsmmc3,
	.ext_cd_cleanup	= ext_cd_cleanup_hsmmc3,
};

#ifdef CONFIG_DRM_EXYNOS
static struct resource exynos_drm_resource[] = {
	[0] = {
		.start = IRQ_FIMD0_VSYNC,
		.end   = IRQ_FIMD0_VSYNC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device exynos_drm_device = {
	.name	= "exynos-drm",
	.id	= -1,
	.num_resources	  = ARRAY_SIZE(exynos_drm_resource),
	.resource	  = exynos_drm_resource,
	.dev	= {
		.dma_mask = &exynos_drm_device.dev.coherent_dma_mask,
		.coherent_dma_mask = 0xffffffffUL,
	}
};
#endif

enum fixed_regulator_id {
	FIXED_REG_ID_LCD = 0,
	FIXED_REG_ID_HDMI = 1,
};

#ifdef CONFIG_DRM_EXYNOS_FIMD
static struct exynos_drm_fimd_pdata drm_fimd_pdata = {
	.panel = {
		.timing	= {
			.xres		= 720,
			.yres		= 1280,
			.hsync_len	= 3,
			.left_margin	= 15,
			.right_margin	= 50,
			.vsync_len	= 2,
			.upper_margin	= 2,
			.lower_margin	= 3,
			.refresh	= 60,
		},
		.width_mm	= 63,	/* 63.2 mm */
		.height_mm	= 114,	/* 114.19 mm */
	},
	.vidcon0		= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1		= VIDCON1_INV_VCLK,
	.default_win		= 3,
	.bpp			= 32,
	.dynamic_refresh	= 0,
	.high_freq		= 1,
};

#ifdef CONFIG_MDNIE_SUPPORT
static struct resource exynos4_fimd_lite_resource[] = {
	[0] = {
		.start	= EXYNOS4_PA_LCD_LITE0,
		.end	= EXYNOS4_PA_LCD_LITE0 + S5P_SZ_LCD_LITE0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_LCD_LITE0,
		.end	= IRQ_LCD_LITE0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource exynos4_mdnie_resource[] = {
	[0] = {
		.start	= EXYNOS4_PA_MDNIE0,
		.end	= EXYNOS4_PA_MDNIE0 + S5P_SZ_MDNIE0 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct mdnie_platform_data exynos4_mdnie_pdata = {
	.width			= 720,
	.height			= 1280,
};

static struct s5p_fimd_ext_device exynos4_fimd_lite_device = {
	.name			= "fimd_lite",
	.id			= -1,
	.num_resources		= ARRAY_SIZE(exynos4_fimd_lite_resource),
	.resource		= exynos4_fimd_lite_resource,
	.dev			= {
		.platform_data	= &drm_fimd_pdata,
	},
};

static struct s5p_fimd_ext_device exynos4_mdnie_device = {
	.name			= "mdnie",
	.id			= -1,
	.num_resources		= ARRAY_SIZE(exynos4_mdnie_resource),
	.resource		= exynos4_mdnie_resource,
	.dev			= {
		.platform_data	= &exynos4_mdnie_pdata,
	},
};

/* FIXME:!! why init at this point ? */
static int exynos4_common_setup_clock(const char *sclk_name,
	const char *pclk_name, unsigned long rate, unsigned int rate_set)
{
	struct clk *sclk = NULL;
	struct clk *pclk = NULL;

	sclk = clk_get(NULL, sclk_name);
	if (IS_ERR(sclk)) {
		printk(KERN_ERR "failed to get %s clock.\n", sclk_name);
		goto err_clk;
	}

	pclk = clk_get(NULL, pclk_name);
	if (IS_ERR(pclk)) {
		printk(KERN_ERR "failed to get %s clock.\n", pclk_name);
		goto err_clk;
	}

	clk_set_parent(sclk, pclk);

	printk(KERN_INFO "set parent clock of %s to %s\n", sclk_name,
			pclk_name);
	if (!rate_set)
		goto set_end;

	if (!rate)
		rate = 200 * MHZ;

	clk_set_rate(sclk, rate);

set_end:
	clk_put(sclk);
	clk_put(pclk);

	return 0;

err_clk:
	clk_put(sclk);
	clk_put(pclk);

	return -EINVAL;

}
#endif

static struct regulator_consumer_supply lcd_supplies[] = {
	REGULATOR_SUPPLY("VDD", "s6d6aa1"),
};

static struct regulator_init_data lcd_fixed_reg_initdata = {
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(lcd_supplies),
	.consumer_supplies = lcd_supplies,
};

#define GPIO_LCD_VDD_EN_5V	EXYNOS4212_GPM0(0)
static struct fixed_voltage_config lcd_config = {
	.init_data = &lcd_fixed_reg_initdata,
	.microvolts = 5000000,
	.supply_name = "VDD",
	.gpio = GPIO_LCD_VDD_EN_5V,
	.enable_high = 1,
	.enabled_at_boot = 1,
};

static struct platform_device lcd_fixed_reg_device = {
	.name = "reg-fixed-voltage",
	.id = FIXED_REG_ID_LCD,
	.dev = {
		.platform_data = &lcd_config,
	},
};

static int reset_lcd(struct lcd_device *ld)
{
	static unsigned int first = 1;
	int reset_gpio = -1;

	reset_gpio = EXYNOS4_GPF2(1);

	if (first) {
		gpio_request(reset_gpio, "MLCD_RST");
		first = 0;
	}

	gpio_direction_output(reset_gpio, 1);
	usleep_range(1000, 2000);
	gpio_direction_output(reset_gpio, 0);
		usleep_range(1000, 2000);
	gpio_direction_output(reset_gpio, 1);

	dev_info(&ld->dev, "reset completed.\n");

	return 0;
}

static struct lcd_property s6d6aa1_property = {
	.flip = LCD_PROPERTY_FLIP_VERTICAL |
		LCD_PROPERTY_FLIP_HORIZONTAL,
	.dynamic_refresh = false,
};

static struct lcd_platform_data s6d6aa1_pdata = {
	.reset			= reset_lcd,
	.reset_delay		= 1,
	.power_off_delay	= 130,
	.power_on_delay		= 10,
	.lcd_enabled		= 1,
	.pdata	= &s6d6aa1_property,
};

static void lcd_cfg_gpio(void)
{
	int reg;
	int reset_gpio = EXYNOS4_GPF2(1);

	/* MLCD_RST */
	s3c_gpio_cfgpin(reset_gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(reset_gpio, S3C_GPIO_PULL_NONE);

	/* LCD_EN */
	s3c_gpio_cfgpin(GPIO_LCD_VDD_EN_5V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_VDD_EN_5V, S3C_GPIO_PULL_NONE);

	reg = __raw_readl(S3C_VA_SYS + 0x210);
	reg |= 1 << 1;
	__raw_writel(reg, S3C_VA_SYS + 0x210);

	return;
}

#ifdef CONFIG_S5P_MIPI_DSI2
static struct mipi_dsim_config dsim_config = {
	.e_interface		= DSIM_VIDEO,
	.e_virtual_ch		= DSIM_VIRTUAL_CH_0,
	.e_pixel_format		= DSIM_24BPP_888,
	.e_burst_mode		= DSIM_BURST_SYNC_EVENT,
	.e_no_data_lane		= DSIM_DATA_LANE_4,
	.e_byte_clk		= DSIM_PLL_OUT_DIV8,
	.cmd_allow		= 0xf,

	/*
	 * ===========================================
	 * |    P    |    M    |    S    |    MHz    |
	 * -------------------------------------------
	 * |    3    |   100   |    3    |    100    |
	 * |    3    |   100   |    2    |    200    |
	 * |    3    |    63   |    1    |    252    |
	 * |    4    |   100   |    1    |    300    |
	 * |    4    |   110   |    1    |    330    |
	 * |   12    |   350   |    1    |    350    |
	 * |    3    |   100   |    1    |    400    |
	 * |    4    |   150   |    1    |    450    |
	 * |    3    |   120   |    1    |    480    |
	 * |   12    |   250   |    0    |    500    |
	 * |    4    |   100   |    0    |    600    |
	 * |    3    |    81   |    0    |    648    |
	 * |    3    |    88   |    0    |    704    |
	 * |    3    |    90   |    0    |    720    |
	 * |    3    |   100   |    0    |    800    |
	 * |   12    |   425   |    0    |    850    |
	 * |    4    |   150   |    0    |    900    |
	 * |   12    |   475   |    0    |    950    |
	 * |    6    |   250   |    0    |   1000    |
	 * -------------------------------------------
	 */

	.p			= 12,
	.m			= 250,
	.s			= 0,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time	= 500,

	/* escape clk : 10MHz */
	.esc_clk		= 10 * 1000000,

	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.stop_holding_cnt	= 0x7ff,
	/* bta timeout 0 ~ 0xff */
	.bta_timeout		= 0xff,
	/* lp rx timeout 0 ~ 0xffff */
	.rx_timeout		= 0xffff,
};

static struct s5p_platform_mipi_dsim dsim_platform_data = {
	/* already enabled at boot loader. FIXME!!! */
	.enabled		= true,
	.phy_enable		= s5p_dsim_phy_enable,
	.dsim_config		= &dsim_config,
};

static struct mipi_dsim_lcd_device mipi_lcd_device = {
	.name			= "s6d6aa1",
	.id			= -1,
	.bus_id			= 0,

	.platform_data		= (void *)&s6d6aa1_pdata,
};
#endif

static void __init redwood_fb_init(void)
{
#ifdef CONFIG_S5P_MIPI_DSI2
	struct s5p_platform_mipi_dsim *dsim_pdata;

	dsim_pdata = (struct s5p_platform_mipi_dsim *)&dsim_platform_data;
	strcpy(dsim_pdata->lcd_panel_name, "s6d6aa1");
	dsim_pdata->lcd_panel_info = (void *)&drm_fimd_pdata.panel.timing;

	s5p_mipi_dsi_register_lcd_device(&mipi_lcd_device);
	platform_device_register(&lcd_fixed_reg_device);
#ifdef CONFIG_MDNIE_SUPPORT
	s5p_fimd_ext_device_register(&exynos4_mdnie_device);
	s5p_fimd_ext_device_register(&exynos4_fimd_lite_device);
	exynos4_common_setup_clock("sclk_mdnie", "mout_mpll_user",
				400 * MHZ, 1);
#endif
	s5p_device_mipi_dsim0.dev.platform_data = (void *)&dsim_platform_data;
	platform_device_register(&s5p_device_mipi_dsim0);
#endif

	s5p_device_fimd0.dev.platform_data = &drm_fimd_pdata;
	lcd_cfg_gpio();
}

static unsigned long fbmem_start;
static int __init early_fbmem(char *p)
{
	char *endp;
	unsigned long size;

	if (!p)
		return -EINVAL;

	size = memparse(p, &endp);
	if (*endp == '@')
		fbmem_start = memparse(endp + 1, &endp);

	return endp > p ? 0 : -EINVAL;
}
early_param("fbmem", early_fbmem);
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
/* I2C HDMIPHY */
static struct s3c2410_platform_i2c hdmiphy_i2c_data __initdata = {
	.bus_num	= 8,
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
};

static struct i2c_board_info i2c_hdmiphy_devs[] __initdata = {
	{
		/* hdmiphy */
		I2C_BOARD_INFO("s5p_hdmiphy", (0x70 >> 1)),
	},
};

static struct exynos_drm_hdmi_pdata drm_hdmi_pdata = {
	.cfg_hpd	= s5p_hdmi_cfg_hpd,
	.get_hpd	= s5p_hdmi_get_hpd,
};

static struct exynos_drm_common_hdmi_pd drm_common_hdmi_pdata = {
	.hdmi_dev	= &s5p_device_hdmi.dev,
	.mixer_dev	= &s5p_device_mixer.dev,
};

static struct platform_device exynos_drm_hdmi_device = {
	.name	= "exynos-drm-hdmi",
	.dev	= {
		.platform_data = &drm_common_hdmi_pdata,
	},
};

static void redwood_tv_init(void)
{
	/* HDMI PHY */
	s5p_i2c_hdmiphy_set_platdata(&hdmiphy_i2c_data);
	i2c_register_board_info(8, i2c_hdmiphy_devs,
				ARRAY_SIZE(i2c_hdmiphy_devs));

	gpio_request(GPIO_HDMI_HPD, "HDMI_HPD");
	gpio_direction_input(GPIO_HDMI_HPD);
	s3c_gpio_cfgpin(GPIO_HDMI_HPD, S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(GPIO_HDMI_HPD, S3C_GPIO_PULL_DOWN);

#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_hdmi.dev.parent = &exynos4_device_pd[PD_TV].dev;
	s5p_device_mixer.dev.parent = &exynos4_device_pd[PD_TV].dev;
#endif
	s5p_device_hdmi.dev.platform_data = &drm_hdmi_pdata;
}
#endif

static struct platform_device exynos_drm_vidi_device = {
	.name	= "exynos-drm-vidi",
};

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
static void flexrate_work(struct work_struct *work)
{
	cpufreq_ondemand_flexrate_request(10000, 10);
}
static DECLARE_WORK(flex_work, flexrate_work);
#endif

#include <linux/pm_qos_params.h>
static struct pm_qos_request_list busfreq_qos;
static void flexrate_qos_cancel(struct work_struct *work)
{
	pm_qos_update_request(&busfreq_qos, 0);
}

static DECLARE_DELAYED_WORK(busqos_work, flexrate_qos_cancel);

static void tsp_request_qos(void *data)
{
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
	if (!work_pending(&flex_work))
		schedule_work_on(0, &flex_work);
#endif
	/* Guarantee that the bus runs at >= 266MHz */
	if (!pm_qos_request_active(&busfreq_qos))
		pm_qos_add_request(&busfreq_qos, PM_QOS_BUS_DMA_THROUGHPUT,
				   266000);
	else {
		cancel_delayed_work_sync(&busqos_work);
		pm_qos_update_request(&busfreq_qos, 266000);
	}

	/* Cancel the QoS request after 1/10 sec */
	schedule_delayed_work_on(0, &busqos_work, HZ / 5);
}

#ifdef CONFIG_TOUCHSCREEN_MELFAS_MMS
/* MELFAS TSP */
static bool enabled;
static int melfas_mms_power(int on)
{
	struct regulator *regulator_avdd;
	struct regulator *regulator_iovdd;

	if (enabled == on)
		return 0;

	/* Analog-Panel Power: 3.3V */
	regulator_avdd = regulator_get(NULL, "touch");
	if (IS_ERR(regulator_avdd))
		return PTR_ERR(regulator_avdd);

	/* IO Logic Power: 1.8V */
	regulator_iovdd = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator_iovdd)) {
		regulator_put(regulator_avdd);
		return PTR_ERR(regulator_iovdd);
	}

	printk(KERN_DEBUG "[TSP] %s %s\n", __func__, on ? "on" : "off");

	if (on) {
		regulator_enable(regulator_avdd);
		regulator_enable(regulator_iovdd);
	} else {
		if (regulator_is_enabled(regulator_iovdd))
			regulator_disable(regulator_iovdd);
		if (regulator_is_enabled(regulator_avdd))
			regulator_disable(regulator_avdd);
	}

	enabled = on;
	regulator_put(regulator_avdd);
	regulator_put(regulator_iovdd);

	return 0;
}

static int is_melfas_mms_vdd_on(void)
{
	int ret;
	/* 3.3V */
	static struct regulator *regulator;

	if (!regulator) {
		regulator = regulator_get(NULL, "touch");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

static int melfas_mms_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		/*s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT); */
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

static struct tsp_callbacks *charger_callbacks;
static struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

static void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}

static struct melfas_mms_platform_data mms_ts_pdata = {
	.max_x = 720,
	.max_y = 1280,
	.invert_x = 0,
	.invert_y = 0,
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_mms_power,
	.mux_fw_flash = melfas_mms_mux_fw_flash,
	.is_vdd_on = is_melfas_mms_vdd_on,
	.input_event = tsp_request_qos,
	.register_cb = melfas_register_callback,
};

static struct melfas_mms_platform_data mms_ts_pdata_rotate = {
	.max_x = 720,
	.max_y = 1280,
	.invert_x = 720,
	.invert_y = 1280,
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_mms_power,
	.mux_fw_flash = melfas_mms_mux_fw_flash,
	.is_vdd_on = is_melfas_mms_vdd_on,
	.input_event = tsp_request_qos,
	.register_cb = melfas_register_callback,
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

static void __init redwood_tsp_set_platdata(struct melfas_mms_platform_data
	*pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

static void __init redwood_tsp_init(void)
{
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}
#endif

static struct i2c_board_info i2c_devs0[] __initdata = {
	/*
	 * GPD1(0, 1) / XI2C0SDA/SCL
	 * PQ_LTE: 8M_CAM, PQ(proxima): NC
	 */
};

#ifdef CONFIG_VIBETONZ
static struct max77693_haptic_platform_data max77693_haptic_pdata = {
	.max_timeout = 10000,
	.duty = 37641,
	.period = 38022,
	.reg2 = MOTOR_LRA | EXT_PWM | DIVIDER_128,
	.init_hw = NULL,
	.motor_en = NULL,
	.pwm_id = 0,
	.regulator_name = "vmotor",
};
#endif

#ifdef CONFIG_LEDS_MAX77693
static struct max77693_led_platform_data max77693_led_pdata = {
	.num_leds = 4,

	.leds[0].name = "leds-sec1",
	.leds[0].id = MAX77693_FLASH_LED_1,
	.leds[0].timer = MAX77693_FLASH_TIME_500MS,
	.leds[0].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = 0x1F,

	.leds[1].name = "leds-sec2",
	.leds[1].id = MAX77693_FLASH_LED_2,
	.leds[1].timer = MAX77693_FLASH_TIME_500MS,
	.leds[1].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[1].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = 0x1F,

	.leds[2].name = "torch-sec1",
	.leds[2].id = MAX77693_TORCH_LED_1,
	.leds[2].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[2].brightness = 0x0F,

	.leds[3].name = "torch-sec2",
	.leds[3].id = MAX77693_TORCH_LED_2,
	.leds[3].cntrl_mode = MAX77693_LED_CTRL_BY_I2C,
	.leds[3].brightness = 0x0F,

};
#endif

static struct max77693_charger_reg_data max77693_charger_regs[] = {
	{
		/*
		 * charger setting unlock
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_06,
		.data = 0x3 << 2,
	}, {
		/*
		 * fast-charge timer : 10hr
		 * charger restart threshold : disabled
		 * low-battery prequalification mode : enabled
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_01,
		.data = (0x4 << 0) | (0x3 << 4),
	}, {
		/*
		 * CHGIN output current limit in OTG mode : 900mA
		 * fast-charge current : 466mA
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_02,
		.data = (1 << 7) | 0xf,
	}, {
		/*
		 * TOP off timer setting : 0min
		 * TOP off current threshold : 100mA
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_03,
		.data = 0x0,
	}, {
		/*
		* minimum system regulation voltage : 3.6V
		* primary charge termination voltage : 4.2V
		*/
		.addr = MAX77693_CHG_REG_CHG_CNFG_04,
		.data = 0xd6,
	}, {
		/*
		 * maximum input current limit : 600mA
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_09,
		.data = 0x1e,
	}, {
		/*
		 * VBYPSET 5V for USB HOST
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_11,
		.data = 0x50,
	},
};

static struct max77693_charger_platform_data max77693_charger_pdata = {
	.init_data = max77693_charger_regs,
	.num_init_data = ARRAY_SIZE(max77693_charger_regs),
};

static void max77693_change_top_off_vol(void)
{
	int i = 0;

	/*
	* minimum system regulation voltage : 3.6V
	* primary charge termination voltage : 4.35V
	*/
	for (i = 0; i < max77693_charger_pdata.num_init_data; i++) {
		if (max77693_charger_pdata.init_data[i].addr ==
				MAX77693_CHG_REG_CHG_CNFG_04)
			max77693_charger_pdata.init_data[i].data = 0xdd;
	}

	return ;
}

static struct max77693_platform_data midas_max77693_info = {
	.irq_base	= IRQ_BOARD_IFIC_START,
	.irq_gpio	= GPIO_IF_PMIC_IRQ,
	.wakeup		= 1,
	.muic = &max77693_muic,
	.regulators = &max77693_regulators,
	.num_regulators = MAX77693_REG_MAX,
#ifdef CONFIG_VIBETONZ
	.haptic_data = &max77693_haptic_pdata,
#endif
#ifdef CONFIG_LEDS_MAX77693
	.led_data = &max77693_led_pdata,
#endif
	.charger_data = &max77693_charger_pdata,
};

/* I2C GPIO: PQ/PQ_LTE use GPM2[0,1] for MAX77693 */
static struct i2c_gpio_platform_data gpio_i2c_if_pmic = {
	/* PQ/PQLTE use GPF1(4, 5) */
	.sda_pin = GPIO_IF_PMIC_SDA,
	.scl_pin = GPIO_IF_PMIC_SCL,
};

static struct platform_device device_i2c_if_pmic = {
	.name = "i2c-gpio",
	.id = I2C_IF_PMIC,
	.dev.platform_data = &gpio_i2c_if_pmic,
};

static struct i2c_board_info i2c_devs_if_pmic[] __initdata = {
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data = &midas_max77693_info,
	},
};

/* Both PQ/PQ_LTE use I2C7 (XPWMTOUT_2/3) for MAX77686 */
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	}
};

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk4212_ehci_pdata;

static void __init smdk4212_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4212_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata smdk4212_ohci_pdata;

static void __init smdk4212_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk4212_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
#endif


/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata smdk4212_usbgadget_pdata;

static void __init smdk4212_usbgadget_init(void)
{
	struct s5p_usbgadget_platdata *pdata = &smdk4212_usbgadget_pdata;

	s5p_usbgadget_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_G_SLP
#include <linux/usb/slp_multi.h>
static struct slp_multi_func_data midas_slp_multi_funcs[] = {
	{
		.name = "mtp",
		.usb_config_id = USB_CONFIGURATION_DUAL,
	}, {
		.name = "acm",
		.usb_config_id = USB_CONFIGURATION_2,
	}, {
		.name = "sdb",
		.usb_config_id = USB_CONFIGURATION_2,
	}, {
		.name = "mass_storage",
		.usb_config_id = USB_CONFIGURATION_1,
	}, {
		.name = "rndis",
		.usb_config_id = USB_CONFIGURATION_1,
	},
};

static struct slp_multi_platform_data midas_slp_multi_pdata = {
	.nluns	= 2,
	.funcs = midas_slp_multi_funcs,
	.nfuncs = ARRAY_SIZE(midas_slp_multi_funcs),
};

static struct platform_device midas_slp_usb_multi = {
	.name		= "slp_multi",
	.id			= -1,
	.dev		= {
		.platform_data = &midas_slp_multi_pdata,
	},
};
#endif

#ifdef CONFIG_SND_SOC_WM8994
/* vbatt device (for WM8994) */
static struct regulator_consumer_supply vbatt_supplies[] = {
	REGULATOR_SUPPLY("LDO1VDD", NULL),
	REGULATOR_SUPPLY("SPKVDD1", NULL),
	REGULATOR_SUPPLY("SPKVDD2", NULL),
};

static struct regulator_init_data vbatt_initdata = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(vbatt_supplies),
	.consumer_supplies = vbatt_supplies,
};

static struct fixed_voltage_config vbatt_config = {
	.init_data = &vbatt_initdata,
	.microvolts = 5000000,
	.supply_name = "VBATT",
	.gpio = -EINVAL,
};

static struct platform_device vbatt_device = {
	.name = "reg-fixed-voltage",
	.id = -1,
	.dev = {
		.platform_data = &vbatt_config,
	},
};

/* I2C GPIO: GPF0(0/1) for CODEC_SDA/SCL */
static struct regulator_consumer_supply wm1811_ldo1_supplies[] = {
	REGULATOR_SUPPLY("AVDD1", NULL),
};

static struct regulator_init_data wm1811_ldo1_initdata = {
	.constraints = {
		.name = "WM1811 LDO1",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo1_supplies),
	.consumer_supplies = wm1811_ldo1_supplies,
};

static struct regulator_consumer_supply wm1811_ldo2_supplies[] = {
	REGULATOR_SUPPLY("DCVDD", NULL),
};

static struct regulator_init_data wm1811_ldo2_initdata = {
	.constraints = {
		.name = "WM1811 LDO2",
		.always_on = true, /* Actually status changed by LDO1 */
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo2_supplies),
	.consumer_supplies = wm1811_ldo2_supplies,
};

static struct wm8994_pdata wm1811_pdata = {
	.gpio_defaults = {
		[0] = WM8994_GP_FN_IRQ,   /* GPIO1 IRQ output, CMOS mode */
		[7] = WM8994_GPN_DIR | WM8994_GP_FN_PIN_SPECIFIC, /* DACDAT3 */
		[8] = WM8994_CONFIGURE_GPIO |
			  WM8994_GP_FN_PIN_SPECIFIC, /* ADCDAT3 */
		[9] = WM8994_CONFIGURE_GPIO |\
			  WM8994_GP_FN_PIN_SPECIFIC, /* LRCLK3 */
		[10] = WM8994_CONFIGURE_GPIO |\
			   WM8994_GP_FN_PIN_SPECIFIC, /* BCLK3 */
	},

	.irq_base = IRQ_BOARD_CODEC_START,

	/* The enable is shared but assign it to LDO1 for software */
	.ldo = {
		{
			.enable = GPIO_WM8994_LDO,
			.init_data = &wm1811_ldo1_initdata,
		},
		{
			.init_data = &wm1811_ldo2_initdata,
		},
	},

	/* Support external capacitors */
	/* This works on wm1811a only (board REV06 or above) */
	.jd_ext_cap = 1,

	/* Regulated mode at highest output voltage */
	/* 2.0V for micbias2 */
	.micbias = {0x2f, 0x27},

	.micd_lvl_sel = 0xFF,

	.ldo_ena_always_driven = true,
	.ldo_ena_delay = 30000,

	/* Disable ground loop noise feedback on lineout1 - NC - */
	.lineout1fb = 0,
	/* Enable ground loop noise feedback on lineout2 dock audio */
	.lineout2fb = 1,
};
#endif

/* I2C4's GPIO: Redwood CODEC */
static struct i2c_board_info i2c_devs4[] __initdata = {
#if defined(CONFIG_SND_SOC_WM8994)
	{
		I2C_BOARD_INFO("wm1811", (0x34 >> 1)),	/* Audio CODEC */
		.platform_data = &wm1811_pdata,
		.irq = IRQ_EINT(30),
	},
#endif
};

/* I2C GPIO: NFC */
static struct i2c_gpio_platform_data gpio_i2c_nfc = {
#ifdef CONFIG_MACH_REDWOOD
	.sda_pin = GPIO_NFC_SDA_18V,
	.scl_pin = GPIO_NFC_SCL_18V,
#elif defined(CONFIG_MACH_SLP_PQ_LTE)
	.sda_pin = EXYNOS4212_GPM4(1),
	.scl_pin = EXYNOS4212_GPM4(0),
#endif
};

static struct platform_device device_i2c_nfc = {
	.name = "i2c-gpio",
	.id = I2C_NFC,
	.dev.platform_data = &gpio_i2c_nfc,
};

/* Bluetooth */
static struct platform_device bcm4334_bluetooth_device = {
	.name = "bcm4334_bluetooth",
	.id = -1,
};

#ifdef CONFIG_MACH_REDWOOD
/* BCM47511 GPS */
static struct bcm47511_platform_data midas_bcm47511_data = {
	.regpu		= GPIO_GPS_PWR_EN,	/* XM0DATA[15] */
	.nrst		= GPIO_GPS_nRST,	/* XM0DATA[14] */
	.uart_rxd	= GPIO_GPS_RXD,		/* XURXD[1] */
	.gps_cntl	= -1,	/* GPS_CNTL - XM0ADDR[6] */
	.reg32khz	= "lpo_in",
};

static struct platform_device midas_bcm47511 = {
	.name	= "bcm47511",
	.id	= -1,
	.dev	= {
		.platform_data	= &midas_bcm47511_data,
	},
};

/* BCM4752 GPS */
static struct bcm4752_platform_data midas_bcm4752_data = {
	.regpu		= GPIO_GPS_PWR_EN,	/* XM0DATA[15] */
	.uart_rxd	= GPIO_GPS_RXD,		/* XURXD[1] */
	.gps_cntl	= -1,	/* GPS_CNTL - XM0ADDR[6] */
	.reg32khz	= "lpo_in",
};

static struct platform_device midas_bcm4752 = {
	.name	= "bcm4752",
	.id	= -1,
	.dev	= {
		.platform_data	= &midas_bcm4752_data,
	},
};
#endif

/* I2C GPIO: 3_TOUCH */
#ifdef CONFIG_MACH_SLP_PQ_LTE
static struct i2c_gpio_platform_data gpio_i2c_3_touch = {
	.sda_pin = GPIO_3_TOUCH_SDA,
	.scl_pin = GPIO_3_TOUCH_SCL,
};

static struct platform_device device_i2c_3_touch = {
	.name = "i2c-gpio",
	.id = I2C_3_TOUCH,
	.dev.platform_data = &gpio_i2c_3_touch,
};

static struct i2c_board_info i2c_devs_3_touch[] __initdata = {
	{
		I2C_BOARD_INFO("melfas-touchkey", 0x20),
	},
};
#endif

#define GPIO_KEYS(_code, _gpio, _active_low, _iswake, _hook)		\
{					\
	.code = _code,			\
	.gpio = _gpio,	\
	.active_low = _active_low,		\
	.type = EV_KEY,			\
	.wakeup = _iswake,		\
	.debounce_interval = 10,	\
	.isr_hook = _hook,			\
	.value = 1 \
}


static struct gpio_keys_button redwood_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_MENU, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
};

static struct gpio_keys_platform_data redwood_gpiokeys_platform_data = {
	.buttons = redwood_buttons,
	.nbuttons = ARRAY_SIZE(redwood_buttons),
};

static struct platform_device redwood_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &redwood_gpiokeys_platform_data,
	},
};

#ifdef CONFIG_I2C_SI4705
static void pq_si4705_reset(int enable)
{
	pr_info("%s: enable is %d", __func__, enable);
	if (enable)
		gpio_set_value(GPIO_FM_RST, 1);
	else
		gpio_set_value(GPIO_FM_RST, 0);
}

static void pq_si4705_init(void)
{
	gpio_request(GPIO_FM_RST, "fmradio_reset");
	s3c_gpio_cfgpin(GPIO_FM_RST, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_FM_RST, 0);
}

static struct i2c_gpio_platform_data gpio_i2c_fm_radio = {
	.sda_pin	= EXYNOS4_GPY0(3),
	.scl_pin	= EXYNOS4_GPY0(2),
};

static struct platform_device device_i2c_fm_radio = {
	.name = "i2c-gpio",
	.id = I2C_FM_RADIO,
	.dev.platform_data = &gpio_i2c_fm_radio,
};

static struct si4705_pdata pq_fm_radio_info = {
	.reset = pq_si4705_reset,
	.pdata_values = (SI4705_PDATA_BIT_VOL_STEPS |
			 SI4705_PDATA_BIT_VOL_TABLE |
			 SI4705_PDATA_BIT_RSSI_THRESHOLD |
			 SI4705_PDATA_BIT_SNR_THRESHOLD),
	.rx_vol_steps = 16,
	.rx_vol_table = {	0x0, 0x13, 0x16, 0x19,
				0x1C, 0x1F, 0x22, 0x25,
				0x28, 0x2B, 0x2E, 0x31,
				0x34, 0x37, 0x3A, 0x3D	},
	.rx_seek_tune_rssi_threshold = 0x00,
	.rx_seek_tune_snr_threshold = 0x01,
};

static struct i2c_board_info i2c_devs_fm_radio[] __initdata = {
	{
		I2C_BOARD_INFO("si4705", 0x22>>1),
		.platform_data = &pq_fm_radio_info,
		.irq = IRQ_EINT(11),
	}
};
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
static struct samsung_battery_platform_data samsung_battery_pdata = {
	.charger_name	= "max77693-charger",
	.fuelgauge_name	= "max17047-fuelgauge",
	.voltage_max = 4200000,
	.voltage_min = 3400000,

	.in_curr_limit = 1000,
	.chg_curr_ta = 1000,

	.chg_curr_usb = 475,
	.chg_curr_cdp = 1000,
	.chg_curr_wpc = 475,
	.chg_curr_dock = 1000,
	.chg_curr_etc = 475,

	.chng_interval = 30,
	.chng_susp_interval = 60,
	.norm_interval = 120,
	.norm_susp_interval = 7200,
	.emer_lv1_interval = 30,
	.emer_lv2_interval = 10,

	.recharge_voltage = 4150000,	/* it will be cacaluated in probe */

	.abstimer_charge_duration = 6 * 60 * 60,
	.abstimer_charge_duration_wpc = 8 * 60 * 60,
	.abstimer_recharge_duration = 1.5 * 60 * 60,

	.cb_det_src = CABLE_DET_CHARGER,
	.overheat_stop_temp = 600,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,

	/* CTIA spec */
	.ctia_spec  = false,

	/* CTIA temperature spec */
	.event_time = 10 * 60,
	.event_overheat_stop_temp = 600,
	.event_overheat_recovery_temp = 400,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 480,
	.lpm_overheat_recovery_temp = 450,
	.lpm_freeze_stop_temp = -50,
	.lpm_freeze_recovery_temp = 0,

	.temper_src = TEMPER_AP_ADC,
	.temper_ch = 2,
#ifdef CONFIG_S3C_ADC
	/* s3c adc driver does not convert raw adc data.
	 * so, register convert function.
	 */
	.covert_adc = convert_adc,
#endif

	.vf_det_src = VF_DET_CHARGER,
	.vf_det_ch = 0,	/* if src == VF_DET_ADC */
	.vf_det_th_l = 500,
	.vf_det_th_h = 1500,

	.suspend_chging = true,

	.led_indicator = false,

	.battery_standever = false,
};

static struct platform_device samsung_device_battery = {
	.name	= "samsung-battery",
	.id	= -1,
	.dev.platform_data = &samsung_battery_pdata,
};
#endif

/* I2C GPIO: Fuel Gauge */
static struct i2c_gpio_platform_data gpio_i2c_fuel = {
	/* PQ/PQLTE use GPF1(4, 5) */
	.sda_pin = GPIO_FUEL_SDA,
	.scl_pin = GPIO_FUEL_SCL,
};

static struct platform_device device_i2c_fuel = {
	.name = "i2c-gpio",
	.id = I2C_FUEL,
	.dev.platform_data = &gpio_i2c_fuel,
};

static struct max17047_platform_data max17047_pdata = {
	.irq_gpio = GPIO_FUEL_ALERT,
};

static struct i2c_board_info i2c_devs_fuel[] __initdata = {
	{
		I2C_BOARD_INFO("max17047-fuelgauge", 0x36),
		.platform_data = &max17047_pdata,
	},
};

#ifdef CONFIG_BUSFREQ_OPP
static struct device_domain busfreq;

static struct platform_device exynos4_busfreq = {
	.id = -1,
	.name = "exynos-busfreq",
};
#endif

#ifdef CONFIG_DISPFREQ_OPP
static struct platform_device exynos4_dispfreq = {
	.name	= "exynos4-dispfreq",
	.id		= -1,
};
#endif

/* I2C GPIO: MHL */
static struct i2c_gpio_platform_data gpio_i2c_mhl = {
	.sda_pin = GPIO_MHL_SDA_1_8V,
	.scl_pin = GPIO_MHL_SCL_1_8V,
	.udelay = 3,
};

static struct platform_device device_i2c_mhl = {
	.name = "i2c-gpio",
	.id = I2C_MHL,
	.dev.platform_data = &gpio_i2c_mhl,
};

/* I2C GPIO: MHL_D */
static struct i2c_gpio_platform_data gpio_i2c_mhl_d = {
	.sda_pin = GPIO_MHL_DSDA_2_8V,
	.scl_pin = GPIO_MHL_DSCL_2_8V,
};

static struct platform_device device_i2c_mhl_d = {
	.name = "i2c-gpio",
	.id = I2C_MHL_D,
	.dev.platform_data = &gpio_i2c_mhl_d,
};

/* Sensor */
/* Accelometer & Gyro Sensor */
static int lsm330dlc_accel_get_position(void)
{
	int position = 0;

	return position;
}

static struct accel_platform_data lsm330dlc_accel_pdata = {
	.accel_get_position = lsm330dlc_accel_get_position,
	.axis_adjust = true,
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	/* PQ_LTE/PQ both use GSENSE_SCL/SDA */
	{
		I2C_BOARD_INFO("lsm330dlc_accel", (0x32 >> 1)),
		.platform_data = &lsm330dlc_accel_pdata,
	},
	{
		I2C_BOARD_INFO("lsm330dlc_gyro", (0xD6 >> 1)),
	},
};

static void lsm331dlc_gpio_init(void)
{
	int ret = gpio_request(GPIO_GYRO_INT, "lsm330dlc_gyro_irq");

	printk(KERN_INFO "%s\n", __func__);

	if (ret)
		printk(KERN_ERR "Failed to request gpio lsm330dlc_gyro_irq\n");

	ret = gpio_request(GPIO_GYRO_DE, "lsm330dlc_gyro_data_enable");

	if (ret)
		printk(KERN_ERR "Failed to request gpio lsm330dlc_gyro_data_enable\n");

	ret = gpio_request(GPIO_ACC_INT, "lsm330dlc_accel_irq");

	if (ret)
		printk(KERN_ERR "Failed to request gpio lsm330dlc_accel_irq\n");

	/* Accelerometer sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_ACC_INT, 2);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_ACC_INT, S5P_GPIO_DRVSTR_LV1);
	i2c_devs1[0].irq = gpio_to_irq(GPIO_ACC_INT);

	/* Gyro sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_GYRO_INT, 2);
	s3c_gpio_setpull(GPIO_GYRO_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_GYRO_INT, S5P_GPIO_DRVSTR_LV1);
	i2c_devs1[1].irq = gpio_to_irq(GPIO_GYRO_INT);

	/* Gyro sensor data enable pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_DE, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_GYRO_DE, 0);
	s3c_gpio_setpull(GPIO_GYRO_DE, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_DE, S5P_GPIO_DRVSTR_LV1);
}

/* Magnetic (MSEMSE) Sensor */
static struct i2c_gpio_platform_data gpio_i2c_msense = {
	.sda_pin = GPIO_MSENSOR_SDA_18V,
	.scl_pin = GPIO_MSENSOR_SCL_18V,
	.udelay = 2, /* 250KHz */
};

static struct platform_device device_i2c_msense = {
	.name = "i2c-gpio",
	.id = I2C_MSENSE,
	.dev.platform_data = &gpio_i2c_msense,
};

static struct akm8975_platform_data akm8975_pdata = {
	.gpio_data_ready_int = GPIO_MSENSOR_INT,
};

static struct i2c_board_info i2c_devs_msense_ak8975[] __initdata = {
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
	},
};

static void ak8975c_gpio_init(void)
{
	int ret = gpio_request(GPIO_MSENSOR_INT, "gpio_akm_int");

	printk(KERN_INFO "%s\n", __func__);

	if (ret)
		printk(KERN_ERR "Failed to request gpio akm_int.\n");

	s5p_register_gpio_interrupt(GPIO_MSENSOR_INT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_SFN(0xF));
	i2c_devs_msense_ak8975[0].irq = gpio_to_irq(GPIO_MSENSOR_INT);
}

static struct akm8963_platform_data akm8963_pdata = {
	.gpio_data_ready_int = GPIO_MSENSOR_INT,
	.layout = 1,
	.outbit = 1,
	.gpio_RST = GPIO_MSENSE_RST_N,
};

static struct i2c_board_info i2c_devs_msense_ak8963[] __initdata = {
	{
		I2C_BOARD_INFO("ak8963", 0x0C),
		.platform_data = &akm8963_pdata,
	},
};

static int ak8963c_gpio_init(void)
{
	int ret = gpio_request(GPIO_MSENSOR_INT, "gpio_akm_int");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio akm_int.(%d)\n",
			__func__, ret);
		return ret;
	}

	s3c_gpio_cfgpin(GPIO_MSENSE_RST_N, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_MSENSE_RST_N, 1);
	s3c_gpio_setpull(GPIO_MSENSE_RST_N, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(GPIO_MSENSOR_INT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_SFN(0xF));
	i2c_devs_msense_ak8963[0].irq = gpio_to_irq(GPIO_MSENSOR_INT);

	return ret;
}

/* Proximity & Light Sensor */
static int proximity_leda_on(bool enable)
{
	struct regulator *regulator;
	DEFINE_MUTEX(lock);
	int ret = 0;

	pr_info("%s, enable = %d\n", __func__, enable);
	mutex_lock(&lock);

	regulator = regulator_get(NULL, "led_a_2.8v");
	WARN(IS_ERR_OR_NULL(regulator), "%s cannot get regulator\n", __func__);
	if (IS_ERR_OR_NULL(regulator)) {
		regulator = NULL;
		ret = -ENODEV;
		goto leda_out;
	}

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);

	if (enable)
		regulator_enable(regulator);

leda_out:
	if (regulator)
		regulator_put(regulator);
	mutex_unlock(&lock);
	return ret;
}


/* I2C GPIO: PS_ALS (PSENSE) */
static struct i2c_gpio_platform_data gpio_i2c_psense_cm36651 = {
	.sda_pin = GPIO_RGB_SDA_1_8V,
	.scl_pin = GPIO_RGB_SCL_1_8V,
	.udelay = 2, /* 250KHz */
};

static struct platform_device device_i2c_psense_cm36651 = {
	.name = "i2c-gpio",
	.id = I2C_PSENSE,
	.dev.platform_data = &gpio_i2c_psense_cm36651,
};

static u8 proximity_get_threshold(void)
{
	u8 new_threshold = 9;

	if (system_rev == 0x0c)
		new_threshold = 75;

	return new_threshold;
}

static struct cm36651_platform_data cm36651_pdata = {
	.cm36651_led_on = proximity_leda_on,
	.cm36651_get_threshold = proximity_get_threshold,
	.irq = GPIO_PS_ALS_INT,
};

static struct i2c_board_info i2c_devs_psense_cm36651[] __initdata = {
	{
		I2C_BOARD_INFO("cm36651", (0x30 >> 1)),
		.platform_data = &cm36651_pdata,
	},
};

/* I2C GPIO: PS_ALS (PSENSE) */
static struct i2c_gpio_platform_data gpio_i2c_psense_gp2a = {
	.sda_pin = EXYNOS4_GPF0(0),	/* GPIO_PS_ALS_SDA_28V, */
	.scl_pin = EXYNOS4_GPF0(1),	/* GPIO_PS_ALS_SCL_28V, */
	.udelay = 2, /* 250KHz */
};

static struct platform_device device_i2c_psense_gp2a = {
	.name = "i2c-gpio",
	.id = I2C_PSENSE,
	.dev.platform_data = &gpio_i2c_psense_gp2a,
};

static unsigned long gp2a_get_threshold(void)
{
	int new_threshold = 7; /* LTH value */

	if (system_rev != 0x0c)
		new_threshold = 260;

	return new_threshold;
}

static struct gp2a_platform_data gp2a_pdata = {
	.gp2a_led_on	= proximity_leda_on,
	.gp2a_get_threshold = gp2a_get_threshold,
	.p_out = GPIO_PS_ALS_INT,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev = {
		.platform_data = &gp2a_pdata,
	},
};

static struct i2c_board_info i2c_devs_psense_gp2a[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x72 >> 1)),
	},
};

static struct jack_platform_data midas_jack_data = {
	.usb_online		= 0,
	.charger_online	= 0,
	.hdmi_online	= 0,
	.earjack_online	= 0,
	.earkey_online	= 0,
	.ums_online		= -1,
	.cdrom_online	= -1,
	.jig_online		= -1,
	.host_online	= 0,
};

static struct platform_device midas_jack = {
	.name		= "jack",
	.id			= -1,
	.dev		= {
		.platform_data = &midas_jack_data,
	},
};

#if defined(CONFIG_ARM_EXYNOS4_BUS_DEVFREQ)
static struct exynos4_bus_platdata devfreq_bus_pdata = {
	.threshold = {
		.upthreshold = 90,
		.downdifferential = 10,
	},
	.polling_ms = 50,
};
static struct platform_device devfreq_busfreq = {
	.name		= "exynos4412-busfreq",
	.id		= -1,
	.dev		= {
		.platform_data = &devfreq_bus_pdata,
	},
};
#endif

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ)
static struct platform_device devfreq_display = {
	.name		= "exynos4412-display",
	.id		= -1,
};
#endif

/* Uart Select */
static void midas_set_uart_switch(int path)
{
	int gpio;

	gpio = EXYNOS4_GPF2(3);
	gpio_request(gpio, "UART_SEL");

	/* gpio_high == AP */
	if (path == UART_SW_PATH_AP)
		gpio_set_value(gpio, GPIO_LEVEL_HIGH);
	else if (path == UART_SW_PATH_CP)
		gpio_set_value(gpio, GPIO_LEVEL_LOW);

	gpio_free(gpio);
	return;
}

static int midas_get_uart_switch(void)
{
	int val;
	int gpio;

	gpio = EXYNOS4_GPF2(3);
	gpio_request(gpio, "UART_SEL");
	val = gpio_get_value(gpio);
	gpio_free(gpio);

	/* gpio_high == AP */
	if (val == GPIO_LEVEL_HIGH)
		return UART_SW_PATH_AP;
	else if (val == GPIO_LEVEL_LOW)
		return UART_SW_PATH_CP;
	else
		return UART_SW_PATH_NA;
}

static struct uart_select_platform_data midas_uart_select_data = {
	.set_uart_switch	= midas_set_uart_switch,
	.get_uart_switch	= midas_get_uart_switch,
};

static struct platform_device midas_uart_select = {
	.name			= "uart-select",
	.id			= -1,
	.dev			= {
		.platform_data	= &midas_uart_select_data,
	},
};
#ifdef CONFIG_INPUT_SECBRIDGE
/*============================================================
sec-input-bridge
/*============================================================*/
static const struct sec_input_bridge_mkey redwood_appslog_mkey_map[] = {
	{ .type = EV_KEY , .code = KEY_VOLUMEUP			},
	{ .type = EV_KEY , .code = KEY_VOLUMEDOWN			},
	{ .type = EV_KEY , .code = KEY_VOLUMEUP			},
	{ .type = EV_KEY , .code = KEY_VOLUMEDOWN			},
	{ .type = EV_KEY , .code = KEY_POWER				},
	{ .type = EV_KEY , .code = KEY_VOLUMEDOWN			},
	{ .type = EV_KEY , .code = KEY_VOLUMEUP			},
	{ .type = EV_KEY , .code = KEY_POWER				},
};

static const struct sec_input_bridge_mmap redwood_mmap[] = {
	{
		.mkey_map = redwood_appslog_mkey_map,
		.num_mkey = ARRAY_SIZE(redwood_appslog_mkey_map),
		.uevent_env_str = "APPS_LOG",
		.enable_uevent = 1,
		.uevent_action = KOBJ_CHANGE,
		.uevent_env_value = "ON",
		},
};

static struct sec_input_bridge_platform_data redwood_input_bridge_data = {
	.mmap = redwood_mmap,
	.num_map = ARRAY_SIZE(redwood_mmap),
	/* .lcd_warning_func = lcd_warning_function,*/
};

static struct platform_device redwood_input_bridge = {
	.name	= "samsung_input_bridge",
	.id	= -1,
	.dev	= {
		.platform_data = &redwood_input_bridge_data,
			},
};
#endif

static struct platform_device *slp_midas_devices[] __initdata = {
	/* Samsung Power Domain */
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_GPS],
	&exynos4_device_pd[PD_GPS_ALIVE],
	&exynos4_device_pd[PD_ISP],

	&s3c_device_wdt,
	&s3c_device_rtc,
	&s3c_device_i2c0,	/* PQ_LTE only: 8M CAM */
	&s3c_device_i2c1,	/* Gyro/Acc */
	/* i2c2: used by GPS UART */
	&s3c_device_i2c3,	/* Meltas TSP */
	/* i2c4: NC(PQ) / codec: wm1811 (PQ rev01) / Modem(PQ LTE) */
	&s3c_device_i2c4,
	/* i2c5: NC(PQ) / Modem(PQ LTE) */
	&s3c_device_i2c7,	/* MAX77686 PMIC */
#ifdef CONFIG_MACH_SLP_PQ_LTE
	&device_i2c_3_touch,	/* PQ_LTE only: Meltas Touchkey */
#endif
#ifdef CONFIG_I2C_SI4705
	&device_i2c_fm_radio,
#endif
	&device_i2c_if_pmic,	/* if_pmic: max77693 */
	&device_i2c_fuel,	/* max17047-fuelgauge */
	&device_i2c_msense, /* magnetic ak8975c */
	&device_i2c_mhl,
	/* TODO: SW I2C for 8M CAM of PQ (same gpio with PQ_LTE NFC) */
	/* TODO: SW I2C for VT_CAM (GPIO_VT_CAM_SCL/SDA) */
	/* TODO: SW I2C for ADC (GPIO_ADC_SCL/SDA) */
	/* TODO: SW I2C for LTE of PQ_LTE (F2(4) SDA, F2(5) SCL) */

#ifdef CONFIG_DRM_EXYNOS_FIMD
	&s5p_device_fimd0,
#endif
#ifdef CONFIG_DRM_EXYNOS_HDMI
	&s5p_device_i2c_hdmiphy,
	&s5p_device_hdmi,
	&s5p_device_mixer,
	&exynos_drm_hdmi_device,
#endif
	&exynos_drm_vidi_device,
#ifdef CONFIG_DRM_EXYNOS_G2D
	&s5p_device_fimg2d,
#endif
#ifdef CONFIG_DRM_EXYNOS
	&exynos_drm_device,
#endif
#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif

#ifdef CONFIG_SND_SOC_WM8994
	&vbatt_device,
#endif
	&samsung_asoc_dma,
#ifndef CONFIG_SND_SOC_SAMSUNG_USE_DMA_WRAPPER
	&samsung_asoc_idma,
#endif

#ifdef CONFIG_SND_SAMSUNG_AC97
	&exynos_device_ac97,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos_device_pcm0,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
	&exynos_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos_device_srp,
#endif
#if defined CONFIG_USB_EHCI_S5P && !defined CONFIG_LINK_DEVICE_HSIC
	&s5p_device_ehci,
#endif
#if defined CONFIG_USB_OHCI_S5P && !defined CONFIG_LINK_DEVICE_HSIC
	&s5p_device_ohci,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_G_SLP
	&midas_slp_usb_multi,
#endif
#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	&exynos_device_dwmci,
#else
	&s3c_device_mshci,
#endif
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_fimc_is,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_fimc3,
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc3,
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
	&s3c_device_csis0,
	&s3c_device_csis1,
#endif
#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	&s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
	&SYSMMU_PLATDEV(fimd0),
	&SYSMMU_PLATDEV(tv),
	&SYSMMU_PLATDEV(g2d_acp),
	&SYSMMU_PLATDEV(mfc_l),
	&SYSMMU_PLATDEV(mfc_r),
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
#endif
#ifdef CONFIG_CHARGER_MANAGER
	&midas_charger_manager,
#endif
#ifdef CONFIG_S3C_ADC
	&s3c_device_adc,
#endif
#if defined(CONFIG_BATTERY_SAMSUNG)
	&samsung_device_battery,
#endif
#ifdef CONFIG_SENSORS_NTC_THERMISTOR
	&midas_ncp15wb473_thermistor,
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
	&s5p_device_jpeg,
#endif
	&redwood_keypad,
	&midas_jack,
	&midas_uart_select,
	&bcm4334_bluetooth_device,
#if defined(CONFIG_S3C64XX_DEV_SPI)
	&exynos_device_spi1,
#endif
#if defined(CONFIG_ARM_EXYNOS4_BUS_DEVFREQ)
	&devfreq_busfreq,
#endif
#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ)
	&devfreq_display,
#endif

#if defined(CONFIG_BUSFREQ_OPP)
	&exynos4_busfreq,
#endif

#if defined(CONFIG_DISPFREQ_OPP)
	&exynos4_dispfreq,
#endif

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#else
	&exynos4_device_tmu,
#endif
	&host_notifier_device,
#ifdef CONFIG_INPUT_SECBRIDGE
	&redwood_input_bridge,
#endif
};

static void check_hw_revision(void)
{
	unsigned int hwrev = system_rev & 0xff;

	switch (hwrev) {
	case REDWOOD_REV0_1_0425:
	case REDWOOD_REV0_1_0704:
		/*REDWOOD_REV0_1_0425:REDWOOD_UNIV_REV0.1_0425 */
		/*REDWOOD_REV0_1_0704:REDWOOD_UNIV_REV0.1_0704*/
		redwood_tsp_set_platdata(&mms_ts_pdata);
		redwood_tsp_init();
		/* VOL_UP/DOWN keys are not EXTINT. Register them. */
		s5p_register_gpio_interrupt(GPIO_VOL_UP);
		s5p_register_gpio_interrupt(GPIO_VOL_DOWN);
		s5p_register_gpio_interrupt(GPIO_OK_KEY_ANDROID);
		break;
	default:
		break;
	}
}

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
/* below temperature base on the celcius degree */
static struct s5p_platform_tmu midas_tmu_data __initdata = {
	.ts = {
		.stop_1st_throttle  = 78,
		.start_1st_throttle = 80,
		.stop_2nd_throttle  = 87,
		.start_2nd_throttle = 103,
		/* temp to do tripping */
		.start_tripping     = 110,
		/* To protect chip,forcely kernel panic */
		.start_emergency    = 120,
		.stop_mem_throttle  = 80,
		.start_mem_throttle = 85,
	},
	.cpufreq = {
		.limit_1st_throttle  = 800000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
	},
	.temp_compensate = {
		/* vdd_arm in uV for temperature compensation */
		.arm_volt = 900000,
		/* vdd_bus in uV for temperature compensation */
		.bus_volt = 900000,
		/* vdd_g3d in uV for temperature compensation */
		.g3d_volt = 900000,
	},
};
#endif

/*
 * The following initcall is already included in board_slp_pq.c
 * If the Redwood binary is seperated from PQ kernel,
 * maybe it will be enabled again.
 */
#if 0
#ifdef CONFIG_LINK_DEVICE_HSIC
static int __init s5p_hci_device_initcall(void)
{
	/*
	 * ehcd should be probed first.
	 * Unless device detected as fullspeed always.
	 */
#ifdef CONFIG_USB_EHCI_S5P
	int ret = platform_device_register(&s5p_device_ehci);
	if (ret)
		return ret;

	/*
	 * Exynos AP-EVT0 can't use both USB host and device(client)
	 * on running time, because that has critical ASIC problem
	 * about USB PHY CLOCK. That issue was already announced by
	 * S.SLI team (djkim@samsung.com) and already fixed it on
	 * the new EVT1 chip (new target, system_rev != 3).
	 * But we have many EVT0 targets (system_rev == 3)
	 * So, to using old target(EVT0) only using by usb device mode
	 * we added following unregister codes(disable USB Host)
	 * by yongsul96.oh@samsung.com 20120417-SLP
	 */
	if (system_rev == 3) {
		pr_warn("[USB-EHCI]AP is EVT0 type!!, unregister ehci!!!");
		platform_device_unregister(&s5p_device_ehci);
	}
#endif
#ifdef CONFIG_USB_OHCI_S5P
	return platform_device_register(&s5p_device_ohci);
#endif
}
late_initcall(s5p_hci_device_initcall);
#endif	/* LINK_DEVICE_HSIC */
#endif

#if defined(CONFIG_S5P_MEM_CMA)
static struct cma_region regions[] = {
	/*
	 * caution : do not allowed other region definitions above of drm.
	 * drm only using region 0 for startup screen display.
	 */
#ifdef CONFIG_DRM_EXYNOS
	{
		.name = "drm",
		.size = CONFIG_DRM_EXYNOS_MEMSIZE * SZ_1K,
		.start = 0
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_DMA
	{
		.name = "dma",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_DMA * SZ_1K,
		.start = 0
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
	{
		.name = "mfc1",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
		{
			.alignment = 1 << 17,
		},
		.start = 0,
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0
	{
		.name = "mfc0",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
		{
			.alignment = 1 << 17,
		},
		.start = 0,
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
	{
		.name = "mfc",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
		{
			.alignment = 1 << 17,
		},
		.start = 0
	},
#endif
#if !defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	{
		.name		= "b2",
		.size		= 32 << 20,
		{ .alignment	= 128 << 10 },
	},
	{
		.name		= "b1",
		.size		= 32 << 20,
		{ .alignment	= 128 << 10 },
	},
	{
		.name		= "fw",
		.size		= 1 << 20,
		{ .alignment	= 128 << 10 },
		.start		= 0x60500000,	/* FIXME */
	},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
	{
		.name = "srp",
		.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
		.start = 0,
	},
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	{
		.name = "fimc_is",
		.size = CONFIG_VIDEO_EXYNOS_MEMSIZE_FIMC_IS * SZ_1K,
		{
			.alignment = 1 << 26,
		},
		.start = 0
	},
#endif
	{
		.size = 0
	},
};

static void __init exynos4_reserve_mem(void)
{
	static const char map[] __initconst =
#ifdef CONFIG_DRM_EXYNOS
		"exynos-drm=drm;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_DMA
		"s3c-fimc.0=dma;s3c-fimc.1=dma;s3c-fimc.2=dma;s3c-fimc.3=dma;s3c-mem=dma;"
		"exynos4210-fimc.0=dma;exynos4210-fimc.1=dma;exynos4210-fimc.2=dma;exynos4210-fimc.3=dma;"
#endif
#ifdef CONFIG_VIDEO_MFC5X
		"s3c-mfc/A=mfc0,mfc-secure;"
		"s3c-mfc/B=mfc1,mfc-normal;"
		"s3c-mfc/AB=mfc;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		"s5p-mfc/f=fw;"
		"s5p-mfc/a=b1;"
		"s5p-mfc/b=b2;"
#endif
		"samsung-rp=srp;"
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		"exynos4-fimc-is=fimc_is;"
#endif
		""
	;

	if (fbmem_start) {
		int i = 0, drm = 0, dma = 0, mfc1 = 0, mfc0 = 0;
		regions[drm].start = (dma_addr_t) fbmem_start;
		for (i = 0; i < ARRAY_SIZE(regions) - 1 /* terminator */; ++i) {
			if (strncmp(regions[i].name, "dma",
				strlen(regions[i].name)) == 0) {
				dma = i;
				regions[dma].start =
					(dma_addr_t) regions[drm].start
					-regions[dma].size;
			}
			if (strncmp(regions[i].name, "mfc1",
				strlen(regions[i].name)) == 0) {
				mfc1 = i;
				regions[mfc1].start =
					(dma_addr_t) regions[dma].start
					-regions[mfc1].size;
			}
			if (strncmp(regions[i].name, "mfc0",
				strlen(regions[i].name)) == 0) {
				mfc0 = i;
				regions[mfc0].start =
					(dma_addr_t) regions[mfc1].start
					-regions[mfc0].size;
			}
		}
	}

	s5p_cma_region_reserve(regions, NULL, 0, map);
}

/*
 * The following initcall is already included in board_slp_pq.c
 * If the Redwood binary is seperated from PQ kernel,
 * maybe it will be enabled again.
 */
#if 0
#ifdef CONFIG_HIBERNATION
static int __init exynos_set_nosave_regions(void)
{
	int i;

	for (i = ARRAY_SIZE(regions) - 2; i >= 0 /* terminator */; i--) {
		/*
		 * MFC firmware region SHOULD BE saved.
		 * If the name of region is fw, don't register to nosave regions
		 */
		if (strcmp(regions[i].name, "fw")) {
			register_nosave_region_late(
					__phys_to_pfn(regions[i].start),
					__phys_to_pfn(regions[i].start +
						regions[i].size));
		}
	}
	return 0;
}
late_initcall(exynos_set_nosave_regions);
#endif /* CONFIG_HIBERNATION */
#endif
#endif /* CONFIG_S5P_MEM_CMA */

static void __init redwood_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(slp_midas_uartcfgs, ARRAY_SIZE(slp_midas_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
	exynos4_reserve_mem();
#endif

	/* as soon as INFORM6 is visible, sec_debug is ready to run */
	sec_debug_init();
}

static void __init exynos_sysmmu_init(void)
{
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
#ifdef CONFIG_DRM_EXYNOS_FIMD
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimd0).dev, &s5p_device_fimd0.dev);
#endif
#ifdef CONFIG_DRM_EXYNOS_HDMI
	sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_hdmi.dev);
#endif
#ifdef CONFIG_DRM_EXYNOS_G2D
	sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
}

/*
 * This function disable unused clocks to remove power leakage on idle state.
 */
static void midas_disable_unused_clock(void)
{
/* Following array include the unused clock list */
	struct __unused_clock_list {
		char *dev_id;
		char *con_id;
	} clock_list[] =  {
		{
			/* UART Ch 4 is only dedicated for communication
			 * with internal GPS in SoC */
			.dev_id = "s5pv210-uart.4",
			.con_id = "uart",
		}, {
			.dev_id = "s5p-qe.3",
			.con_id = "qefimc",
		}, {
			.dev_id = "s5p-qe.2",
			.con_id = "qefimc",
		}, {
			.dev_id = "s5p-qe.1",
			.con_id = "qefimc",
		},
	};
	struct device dev;
	struct clk *clk;
	char *con_id;
	int i;

	for (i = 0 ; i < ARRAY_SIZE(clock_list) ; i++) {
		dev.init_name = clock_list[i].dev_id;
		con_id = clock_list[i].con_id;

		clk = clk_get(&dev, con_id);
		if (IS_ERR(clk)) {
			printk(KERN_ERR "Failed to get %s for %s\n",
					con_id, dev.init_name);
			continue;
		}
		clk_enable(clk);
		clk_disable(clk);
		clk_put(clk);
	}
}

/* Use exynos4-display with DEVFREQ to control LCD refresh rate
 * dynamically when specific power domain is turning ON/OFF.
 */
#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
#include <linux/devfreq/exynos4_display.h>
/* EXYNOS4 series SoC use ten power domains,
   the last power domain is PD_MAUDIO */
#define PD_NUMBER	(PD_MAUDIO + 1)
static struct pm_qos_request_list exynos_display_qos[PD_NUMBER];
static int __exynos_pd_enable(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	pm_qos_update_request(&exynos_display_qos[pdev->id],
					EXYNOS4_DISPLAY_LV_HF);

	return exynos_pd_enable(dev);
}

static int __exynos_pd_disable(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	pm_qos_update_request(&exynos_display_qos[pdev->id],
					EXYNOS4_DISPLAY_LV_LF);

	return exynos_pd_disable(dev);
}
#else
static int __exynos_pd_enable(struct device *dev)
{
	return exynos_pd_enable(dev);
}
static int __exynos_pd_disable(struct device *dev)
{
	return exynos_pd_disable(dev);
}
#endif

static void midas_exynos_display_init(void)
{
	unsigned int pd_list[] = { PD_MFC, PD_G3D };
	struct device *dev;
	struct samsung_pd_info *pdata;
	int i, id;

	for (i = 0 ; i < ARRAY_SIZE(pd_list) ; i++) {
		id = pd_list[i];

		dev = &exynos4_device_pd[id].dev;
		pdata = dev->platform_data;
		pdata->enable = __exynos_pd_enable;
		pdata->disable = __exynos_pd_disable;

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
		pm_qos_add_request(&exynos_display_qos[id],
			PM_QOS_DISPLAY_FREQUENCY, EXYNOS4_DISPLAY_LV_HF);
#endif
	}
}

static void flash_gpio_init(void)
{
	/* GPIO initialize for STG5223QTR analog SW */
	gpio_request(GPIO_CAM_SW_EN, "CAM_SW_EN");
	gpio_direction_output(GPIO_CAM_SW_EN, 1);

	gpio_request(GPIO_TORCH_EN, "TORCH_EN");
	gpio_direction_output(GPIO_TORCH_EN, 0);

	gpio_request(GPIO_TORCH_SET, "TORCH_SET");
	gpio_direction_output(GPIO_TORCH_SET, 0);

}

static void __init redwood_machine_init(void)
{
#ifdef CONFIG_BUSFREQ_OPP
	struct clk *ppmu_clk = NULL;
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI)
	unsigned int gpio;
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &exynos_device_spi1.dev;
#endif
	strcpy(utsname()->nodename, machine_desc->name);

	/* Workaround: bootloader needs to set GPX*PUD registers */
	s3c_gpio_setpull(EXYNOS4_GPX2(7), S3C_GPIO_PULL_NONE);

#if defined(CONFIG_EXYNOS_DEV_PD) && defined(CONFIG_PM_RUNTIME)
	exynos_pd_disable(&exynos4_device_pd[PD_MFC].dev);

	/*
	 * FIXME: now runtime pm of mali driver isn't worked yet.
	 * if the runtime pm is worked fine, then remove this call.
	 */
	exynos_pd_enable(&exynos4_device_pd[PD_G3D].dev);

	/* PD_LCD0 : The child devie control LCD0 power domain
	 * because LCD should be always enabled during kernel booting.
	 * So, LCD power domain can't turn off when machine initialization.*/
	exynos_pd_disable(&exynos4_device_pd[PD_CAM].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_TV].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_GPS].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_ISP].dev);
#elif defined(CONFIG_EXYNOS_DEV_PD)
	/*
	 * These power domains should be always on
	 * without runtime pm support.
	 */
	exynos_pd_enable(&exynos4_device_pd[PD_MFC].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_G3D].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_LCD0].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_CAM].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_TV].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_GPS].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_ISP].dev);
#endif

	/* initialise the gpios */
	midas_config_gpio_table();
	exynos4_sleep_gpio_table_set = midas_config_sleep_gpio_table;

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

	/* LSM330DLC (Gyro & Accelerometer Sensor) */
	s3c_i2c1_set_platdata(NULL);
	lsm331dlc_gpio_init();
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	s3c_i2c3_set_platdata(NULL);

	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));

	s3c_i2c7_set_platdata(NULL);
	s3c_i2c7_set_platdata(NULL);

	/* Workaround for repeated interrupts from MAX77686 during sleep */
	if (hwrevision(0))
		exynos4_max77686_info.wakeup = 0;

	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

	GPIO_I2C_PIN_SETUP(if_pmic);
	midas_power_set_muic_pdata(NULL, EXYNOS4_GPX0(7));
	i2c_register_board_info(I2C_IF_PMIC, i2c_devs_if_pmic,
				ARRAY_SIZE(i2c_devs_if_pmic));

	/* NFC */
#ifdef CONFIG_MACH_REDWOOD
	if (hwrevision(1) || system_rev >= 0x7) {
		s3c_i2c5_set_platdata(NULL);
		platform_device_register(&s3c_device_i2c5);
		midas_nfc_init(s3c_device_i2c5.id);
	} else {
		GPIO_I2C_PIN_SETUP(nfc);
		platform_device_register(&device_i2c_nfc);
		midas_nfc_init(device_i2c_nfc.id);
	}
#else
	/* CONFIG_MACH_SLP_PQ_LTE */
	GPIO_I2C_PIN_SETUP(nfc);
	platform_device_register(&device_i2c_nfc);
	midas_nfc_init(device_i2c_nfc.id);
#endif

	/* MHL / MHL_D */
	GPIO_I2C_PIN_SETUP(mhl);

#ifdef CONFIG_MACH_REDWOOD
	if (hwrevision(0)) {
		GPIO_I2C_PIN_SETUP(mhl_d);
		platform_device_register(&device_i2c_mhl_d);
	} else {
		/* nothing */
	}
#else
	GPIO_I2C_PIN_SETUP(mhl_d);
	platform_device_register(&device_i2c_mhl_d);
#endif

	/* Magnetic Sensor */
	if (system_rev == 0x0c) {
		ak8975c_gpio_init();

		GPIO_I2C_PIN_SETUP(msense);
		i2c_register_board_info(I2C_MSENSE, i2c_devs_msense_ak8975,
					ARRAY_SIZE(i2c_devs_msense_ak8975));
	} else {
		ak8963c_gpio_init();

		GPIO_I2C_PIN_SETUP(msense);
		i2c_register_board_info(I2C_MSENSE, i2c_devs_msense_ak8963,
					ARRAY_SIZE(i2c_devs_msense_ak8963));
	}

	/* Proximity & Light Sensor */
	if (system_rev == 0x0C) {
		GPIO_I2C_PIN_SETUP(psense_cm36651);
		i2c_register_board_info(I2C_PSENSE, i2c_devs_psense_cm36651,
					ARRAY_SIZE(i2c_devs_psense_cm36651));

		platform_device_register(&device_i2c_psense_cm36651);
	} else {
		GPIO_I2C_PIN_SETUP(psense_gp2a);
		i2c_register_board_info(I2C_PSENSE, i2c_devs_psense_gp2a,
					ARRAY_SIZE(i2c_devs_psense_gp2a));

		platform_device_register(&device_i2c_psense_gp2a);
		platform_device_register(&opt_gp2a);
	}

	/* GPS HW driver */
	if (system_rev == REDWOOD_REV0_1_0425)
		platform_device_register(&midas_bcm47511);
	else
		platform_device_register(&midas_bcm4752);

#ifdef CONFIG_USB_EHCI_S5P
	smdk4212_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
	smdk4212_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
	smdk4212_usbgadget_init();
#endif

#ifdef CONFIG_MACH_SLP_PQ_LTE
	GPIO_I2C_PIN_SETUP(3_touch);
	gpio_request(GPIO_3_TOUCH_INT, "3_TOUCH_INT");
	s5p_register_gpio_interrupt(GPIO_3_TOUCH_INT);
	i2c_register_board_info(I2C_3_TOUCH, i2c_devs_3_touch,
				ARRAY_SIZE(i2c_devs_3_touch));
#endif

	GPIO_I2C_PIN_SETUP(fuel);
	i2c_register_board_info(I2C_FUEL, i2c_devs_fuel,
				ARRAY_SIZE(i2c_devs_fuel));

#ifdef CONFIG_I2C_SI4705
	GPIO_I2C_PIN_SETUP(fm_radio);
	pq_si4705_init();
	i2c_register_board_info(I2C_FM_RADIO, i2c_devs_fm_radio,
				ARRAY_SIZE(i2c_devs_fm_radio));
#endif
#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	exynos_dwmci_set_platdata(&exynos_dwmci_pdata, 0);
#else
	s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
	s3c_sdhci2_set_platdata(&slp_midas_hsmmc2_pdata);
	s3c_sdhci3_set_platdata(&slp_midas_hsmmc3_pdata);

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	exynos4_fimc_is_set_platdata(NULL);
	exynos4_device_fimc_is.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif

	/* FIMC */
	midas_camera_init();

#ifdef CONFIG_DRM_EXYNOS_FIMD
	/*
	 * platform device name for fimd driver should be changed
	 * because we can get source clock with this name.
	 *
	 * P.S. refer to sclk_fimd definition of clock-exynos4.c
	 */
	s5p_fb_setname(0, "s3cfb");
	s5p_device_fimd0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#ifdef CONFIG_S5P_MIPI_DSI2
	s5p_device_mipi_dsim0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

	setup_charger_manager(&midas_charger_g_desc);

#ifdef CONFIG_VIDEO_JPEG_V2X
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&midas_tmu_data);
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc");
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
	exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 267 * MHZ);
#endif

	exynos_sysmmu_init();

	/* Disable unused clocks to remove power leakage on idle state */
	midas_disable_unused_clock();

	/* exynos4-display with DEVFREQ */
	midas_exynos_display_init();

#ifdef CONFIG_SENSORS_NTC_THERMISTOR
	/* PQ Rev00 doesn't have ntc on board */
	if (!hwrevision(0))
		adc_ntc_init(2); /* Channel 2 */
#endif

	platform_add_devices(slp_midas_devices, ARRAY_SIZE(slp_midas_devices));

#ifdef CONFIG_DRM_EXYNOS_FIMD
	redwood_fb_init();
#endif
#ifdef CONFIG_DRM_EXYNOS_HDMI
	redwood_tv_init();
#endif

	brcm_wlan_init();
#if defined(CONFIG_S3C64XX_DEV_SPI)
	sclk = clk_get(spi1_dev, "dout_spi1");
	if (IS_ERR(sclk))
		dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi1_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi1_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
		       prnt->name, sclk->name);

	clk_set_rate(sclk, 800 * 1000 * 1000);
	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPB(5), "SPI_CS1")) {
		gpio_direction_output(EXYNOS4_GPB(5), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(5), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				     ARRAY_SIZE(spi1_csi));
	}

	for (gpio = EXYNOS4_GPB(4); gpio < EXYNOS4_GPB(8); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
#endif
#ifdef CONFIG_BUSFREQ_OPP
	dev_add(&busfreq, &exynos4_busfreq.dev);

	/* PPMUs using for cpufreq get clk from clk_list */
	ppmu_clk = clk_get(NULL, "ppmudmc0");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc0\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmudmc1");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc1\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmucpu");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_cpu\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif

	flash_gpio_init();

	check_hw_revision();
}

MACHINE_START(REDWOOD, "REDWOOD")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io = redwood_map_io,
	.init_machine	= redwood_machine_init,
	.timer		= &exynos4_timer,
MACHINE_END
