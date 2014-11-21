/*
 * linux/arch/arm/mach-exynos/board-slp-naples.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Based on mach-exynos/board-slp-pq.c
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
#include <linux/i2c/mms114.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/lcd.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/sensor/lsm330dlc_accel.h>
#include <linux/sensor/lsm330dlc_gyro.h>
#include <linux/sensor/ak8975.h>
#ifdef CONFIG_SENSORS_TAOS
#include <linux/sensor/taos.h>
#endif
#ifdef CONFIG_SENSORS_GP2A
#include <linux/sensor/gp2a.h>
#endif
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
#include <linux/sensor/lps331ap.h>
#include <linux/devfreq/exynos4_bus.h>
#include <linux/extcon.h>
#include <drm/exynos_drm.h>

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
#ifdef CONFIG_EXYNOS4_DEV_FIMC_IS
#include <media/exynos_fimc_is.h>
#endif
#include <plat/regs-fb.h>
#include <plat/fb-core.h>
#include <plat/mipi_dsim2.h>
#include <plat/fimd_lite_ext.h>
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif
#include <plat/fb-s5p.h>

#include <mach/map.h>
#include <mach/spi-clocks.h>

#ifdef CONFIG_SND_SOC_WM8994
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#endif

#include <mach/midas-power.h>
#include <mach/naples-tsp.h>
#include <mach/midas-tsp.h>
#include <mach/dwmci.h>

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT224
#include <linux/i2c/mxt224.h>
#endif
#ifdef CONFIG_I2C_SI4705
#include <linux/si4705_pdata.h>
#endif

#include <mach/regs-pmu.h>

#include <mach/dev-sysmmu.h>

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#include <linux/host_notify.h>

#include "board-mobile.h"

#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif

enum gpio_i2c {
	I2C_LAST_HW	= 8, /* I2C0~8 are reserved */
	I2C_FM_RADIO	= 9, /* I2C9 is reserved for CODEC (hardcoded) */
	I2C_NFC,
	I2C_3_TOUCH,
	I2C_FUEL,
	I2C_BSENSE,
	I2C_MSENSE,
	I2C_MHL		= 15, /* 15 is hardcoded from midas-mhl.c */
	I2C_MHL_D	= 16, /* 16 is hardcoded from midas-mhl.c */
	I2C_PSENSE,
	I2C_IF_PMIC,
	I2C_GSENSE,
	I2C_VTCAM,
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

#define DEFINE_MMC_CARD_NOTIFIER(num) \
static void (*hsmmc##num##_notify_func)(struct platform_device *, int state); \
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

void mmc_force_presence_change(struct platform_device *pdev)
{
	void (*notify_func)(struct platform_device *, int state) = NULL;
	mutex_lock(&notify_lock);
	if (pdev == &s3c_device_hsmmc3)
		notify_func = hsmmc3_notify_func;

	if (notify_func)
		notify_func(pdev, 1);
	else
		pr_warn("%s: called for device with no notifier\n", __func__);
	mutex_unlock(&notify_lock);
}
EXPORT_SYMBOL_GPL(mmc_force_presence_change);

static struct s3c_sdhci_platdata slp_midas_hsmmc3_pdata __initdata = {
/* new code for brm4334 */
	.cd_type	= S3C_SDHCI_CD_EXTERNAL,
	.clk_type	= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags	= S3C_SDHCI_PM_IGNORE_SUSPEND_RESUME,
	.ext_cd_init	= ext_cd_init_hsmmc3,
	.ext_cd_cleanup	= ext_cd_cleanup_hsmmc3,
};

enum fixed_regulator_id {
	FIXED_REG_ID_LCD = 0,
};

static struct regulator_consumer_supply lcd_supplies[] = {
	REGULATOR_SUPPLY("VDD3", "s6e8aa0"),
};

static struct regulator_init_data lcd_fixed_reg_initdata = {
	.num_consumer_supplies = ARRAY_SIZE(lcd_supplies),
	.consumer_supplies = lcd_supplies,
};

static struct fixed_voltage_config lcd_config = {
	.init_data = &lcd_fixed_reg_initdata,
	.microvolts = 2200000,
	.gpio = GPIO_LCD_22V_EN_00,
};

static struct platform_device lcd_fixed_reg_device = {
	.name = "reg-fixed-voltage",
	.id = FIXED_REG_ID_LCD,
	.dev = {
		.platform_data = &lcd_config,
	},
};

static void lcd_cfg_gpio(void)
{
	int reg;
	reg = __raw_readl(S3C_VA_SYS + 0x210);
	reg |= 1 << 1;
	__raw_writel(reg, S3C_VA_SYS + 0x210);

	if (hwrevision(1)) {
		/* LCD_EN */
		s3c_gpio_cfgpin(GPIO_LCD_22V_EN_00, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_LCD_22V_EN_00, S3C_GPIO_PULL_NONE);
	}

	return;
}

static int reset_lcd(struct lcd_device *ld)
{
	static unsigned int first = 1;

	if (first) {
		gpio_request(GPIO_MLCD_RST, "MLCD_RST");
		first = 0;
	}

	mdelay(10);
	gpio_direction_output(GPIO_MLCD_RST, 0);
	mdelay(10);
	gpio_direction_output(GPIO_MLCD_RST, 1);

	dev_info(&ld->dev, "reset completed.\n");

	return 0;
}

static struct lcd_platform_data s6e39a0x02_pd = {
	.reset			= reset_lcd,
	.reset_delay		= 25,
	.power_off_delay	= 120,
	.power_on_delay		= 120,
	.lcd_enabled		= 1,
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

#ifdef CONFIG_S5P_MIPI_DSI2
static struct mipi_dsim_config dsim_config = {
	.e_interface		= DSIM_COMMAND,
	.e_virtual_ch		= DSIM_VIRTUAL_CH_0,
	.e_pixel_format		= DSIM_24BPP_888,
	.e_burst_mode	= DSIM_NON_VIDEO_MODE,
	.e_no_data_lane	= DSIM_DATA_LANE_2,
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

	.p			= 3,
	.m			= 120,
	.s			= 1,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time	= 500,

	/* escape clk : 10MHz */
	.esc_clk		= 20 * 1000000,

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
	.name			= "s6e39a0x02",
	.id			= -1,
	.bus_id			= 0,

	.platform_data		= (void *)&s6e39a0x02_pd,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_MELFAS
static struct melfas_tsi_platform_data melfas_tsp_pdata = {
	.x_size = 720,
	.y_size = 1280,
	.gpio_int = GPIO_TSP_INT,
	.power = melfas_power,
	.mt_protocol_b = true,
	.enable_btn_touch = true,
	.set_touch_i2c = melfas_set_touch_i2c,
	.set_touch_i2c_to_gpio = melfas_set_touch_i2c_to_gpio,
	.input_event = midas_tsp_request_qos,
};
#endif
static struct i2c_board_info i2c_devs0[] __initdata = {
	/*
	 * GPD1(0, 1) / XI2C0SDA/SCL
	 * PQ_LTE: 8M_CAM, PQ(proxima): NC
	 */
};

#ifdef CONFIG_SENSORS_LSM330DLC
static struct i2c_board_info i2c_devs1[] __initdata = {
	/* PQ_LTE/PQ both use GSENSE_SCL/SDA */
	{
		I2C_BOARD_INFO("lsm330dlc_accel", (0x32 >> 1)),
	},
	{
		I2C_BOARD_INFO("lsm330dlc_gyro", (0xD6 >> 1)),
	},
};

static void lsm331dlc_gpio_init(void)
{
	int ret = gpio_request(GPIO_GYRO_INT, "lsm330dlc_gyro_irq");

	pr_info("%s\n", __func__);

	if (ret)
		pr_err("Failed to request gpio lsm330dlc_gyro_irq\n");

	ret = gpio_request(GPIO_GYRO_DE, "lsm330dlc_gyro_data_enable");

	if (ret)
		pr_err("Failed to request gpio lsm330dlc_gyro_data_enable\n");

	ret = gpio_request(GPIO_ACC_INT, "lsm330dlc_accel_irq");

	if (ret)
		pr_err("Failed to request gpio lsm330dlc_accel_irq\n");

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
#endif

#ifdef CONFIG_VIBETONZ
static struct max77693_haptic_platform_data max77693_haptic_pdata = {
	.max_timeout = 10000,
	.duty = 44000,
	.period = 44642,
	.reg2 = MOTOR_LRA | EXT_PWM | DIVIDER_128,
	.init_hw = NULL,
	.motor_en = NULL,
	.pwm_id = 0,
	.regulator_name = "vmotor",
};
#endif

#ifdef CONFIG_LEDS_MAX77693
static struct max77693_led_platform_data max77693_led_pdata = {
	.num_leds = 2,

	.leds[0].name = "leds-sec",
	.leds[0].id = MAX77693_FLASH_LED_1,
	.leds[0].timer = MAX77693_FLASH_TIME_1000MS,
	.leds[0].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = MAX_FLASH_DRV_LEVEL,

	.leds[1].name = "torch-sec",
	.leds[1].id = MAX77693_TORCH_LED_1,
	.leds[1].timer = MAX77693_DIS_TORCH_TMR,
	.leds[1].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[1].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = MAX_TORCH_DRV_LEVEL,
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
		 * fast-charge timer : 5hr
		 * charger restart threshold : disabled
		 * low-battery prequalification mode : enabled
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_01,
		.data = (0x1 << 7) | (0x3 << 4) | 0x2,
	}, {
		/*
		 * CHGIN output current limit in OTG mode : 900mA
		 * fast-charge current : 500mA
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_02,
		.data = (1 << 7) | 0xf,
	}, {
		/*
		 * TOP off timer setting : 0min
		 * TOP off current threshold : 250mA
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_03,
		.data = 0x3,
	}, {
		/*
		 * minimum system regulation voltage : 3.0V
		 * primary charge termination voltage : 4.2V
		 */
		.addr = MAX77693_CHG_REG_CHG_CNFG_04,
		.data = 0x16,
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

#if defined(CONFIG_REGULATOR_S5M8767)
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
		.platform_data = &exynos4_s5m8767_info,
		.irq	= IRQ_EINT(7),
	}
};
#endif


#if defined(CONFIG_REGULATOR_MAX77686)
/* Both PQ/PQ_LTE use I2C7 (XPWMTOUT_2/3) for MAX77686 */
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	}
};
#endif

#ifdef CONFIG_I2C_SI4705
static void naples_si4705_reset(int enable)
{
	pr_info("%s: enable is %d", __func__, enable);
	if (enable)
		gpio_set_value(GPIO_FM_RST, 1);
	else
		gpio_set_value(GPIO_FM_RST, 0);
}

static void naples_si4705_init(void)
{
	gpio_request(GPIO_FM_RST, "fmradio_reset");
	s3c_gpio_cfgpin(GPIO_FM_RST, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_FM_RST, 0);
}

static struct i2c_gpio_platform_data gpio_i2c_fm_radio = {
	.sda_pin = EXYNOS4_GPY0(3),
	.scl_pin = EXYNOS4_GPY0(2),
};

static struct platform_device device_i2c_fm_radio = {
	.name = "i2c-gpio",
	.id = I2C_FM_RADIO,
	.dev.platform_data = &gpio_i2c_fm_radio,
};

static struct si4705_pdata naples_fm_radio_info = {
	.reset = naples_si4705_reset,
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
		.platform_data = &naples_fm_radio_info,
		.irq = IRQ_EINT(11),
	},
};
#endif

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

static void otg_accessory_power(int enable)
{
	u8 on = (u8)!!enable;

	gpio_request(GPIO_OTG_EN, "USB_OTG_EN");
	gpio_direction_output(GPIO_OTG_EN, on);
	gpio_free(GPIO_OTG_EN);
	pr_info("%s: otg accessory power = %d\n", __func__, on);
}

static struct host_notifier_platform_data host_notifier_pdata = {
	.ndev.name	= "usb_otg",
	.booster	= otg_accessory_power,
	.thread_enable	= 0,
};

struct platform_device host_notifier_device = {
	.name = "host_notifier",
	.dev.platform_data = &host_notifier_pdata,
};

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


#ifdef CONFIG_DRM_EXYNOS_FIMD
#define VIDCON0_DSI_ENABLE			(1 << 30)

static struct exynos_drm_fimd_pdata drm_fimd_pdata = {
	.panel = {
		.timing	= {
			.xres		= 540,
			.yres		= 960,
			.hsync_len	= 0,
			.left_margin	= 0,
			.right_margin	= 0,
			.vsync_len	= 1,
			.upper_margin	= 0x4D,
			.lower_margin	= 0x2,
			.refresh	= 60,
		},
		.width_mm	= 53,
		.height_mm	= 95,
	},
	.vidcon0	= VIDCON0_DSI_ENABLE | VIDCON0_VIDOUT_I80_LDI0
			 | VIDCON0_PNRMODE_RGB, /* MIPI COMMAND MODE */
	.default_win	= 3,
	.bpp		= 32,
	.dynamic_refresh	= 1,
	.high_freq		= 1,
};
#endif

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

static struct mdnie_platform_data exynos4_mdnie_pd = {
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
		.platform_data	= &exynos4_mdnie_pd,
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
			.enable = EXYNOS4212_GPJ0(4),
			.init_data = &wm1811_ldo1_initdata,
		},
		{
			.init_data = &wm1811_ldo2_initdata,
		},
	},

	/* Regulated mode at highest output voltage */
	.micbias = {0x3f, 0x3e},

	.micd_lvl_sel = 0xFF,

	.ldo_ena_always_driven = true,

	/* external cap for mic bias2 */
	.jd_ext_cap = 1,
};
#endif

/* I2C4's GPIO: PQ_LTE(CMC_CS) / PQ(NC) / PQ Rev01 (codec) */
static struct i2c_board_info i2c_devs4[] __initdata = {
#ifdef CONFIG_SND_SOC_WM8994
	{
		I2C_BOARD_INFO("wm1811", (0x34 >> 1)),	/* Audio CODEC */
		.platform_data = &wm1811_pdata,
		.irq = IRQ_EINT(30),
	},
#endif
};

static void naples_audio_init(void)
{
	int err;

	/* Main Microphone BIAS */
	err = gpio_request(GPIO_MIC_BIAS_EN_00, "MICBIAS_EN");
	if (err) {
		pr_err("%s: MIC_BIAS_EN GPIO set error", __func__);
		return;
	}
	gpio_direction_output(GPIO_MIC_BIAS_EN_00, 0);
	gpio_free(GPIO_MIC_BIAS_EN_00);

	/* Sub Microphone BIAS */
	err = gpio_request(GPIO_SUB_MIC_BIAS_EN_00, "SUB_MICBIAS_EN");
	if (err) {
		pr_err("%s: SUB_MIC_BIAS_EN GPIO set error", __func__);
		return;
	}
	gpio_direction_output(GPIO_SUB_MIC_BIAS_EN_00, 0);
	gpio_free(GPIO_SUB_MIC_BIAS_EN_00);
}

/* I2C GPIO: NFC */
static struct i2c_gpio_platform_data gpio_i2c_nfc = {
	.sda_pin = GPIO_NFC_SDA_18V,
	.scl_pin = GPIO_NFC_SCL_18V,
};

static struct platform_device device_i2c_nfc = {
	.name = "i2c-gpio",
	.id = I2C_NFC,
	.dev.platform_data = &gpio_i2c_nfc,
};

/* Bluetooth */
#ifdef CONFIG_BT_BCM4334
static struct platform_device bcm4334_bluetooth_device = {
	.name = "bcm4334_bluetooth",
	.id = -1,
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

static struct gpio_keys_button midas_buttons[] = {
#ifdef CONFIG_SEC_DEBUG
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_MENU, GPIO_OK_KEY,
			  1, 1, NULL),
#else
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, NULL),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, NULL),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, NULL),
	GPIO_KEYS(KEY_MENU, GPIO_OK_KEY,
		  1, 1, NULL),
#endif

};

static struct gpio_keys_platform_data midas_gpiokeys_platform_data = {
	.buttons = midas_buttons,
	.nbuttons = ARRAY_SIZE(midas_buttons),
};

static struct platform_device midas_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &midas_gpiokeys_platform_data,
	},
};

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

#ifdef CONFIG_VIDEO_SLP_DB8131M
/* I2C GPIO: 1.3M VT CAM */
static struct i2c_gpio_platform_data gpio_i2c_vtcam = {
	.sda_pin		= GPIO_VT_CAM_SDA_18V,
	.scl_pin		= GPIO_VT_CAM_SCL_18V,
	.udelay			= 2, /* 250 kHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device device_i2c_vtcam = {
	.name	= "i2c-gpio",
	.id	= I2C_VTCAM,
	.dev.platform_data	= &gpio_i2c_vtcam,
};

static struct i2c_board_info i2c_devs_vtcam[] __initdata = {
};
#endif

#ifdef CONFIG_SENSORS_LPS331
/* I2C GPIO: Barometer (BSENSE) */
static struct i2c_gpio_platform_data gpio_i2c_bsense = {
	.sda_pin = GPIO_BSENSE_SDA_18V,
	.scl_pin = GPIO_BENSE_SCL_18V,
};

static struct platform_device device_i2c_bsense = {
	.name = "i2c-gpio",
	.id = I2C_BSENSE,
	.dev.platform_data = &gpio_i2c_bsense,
};

static struct lps331ap_platform_data lps331_pdata = {
	.irq = GPIO_BARO_INT,
};

static struct i2c_board_info i2c_devs_bsense[] __initdata = {
	{
		I2C_BOARD_INFO(LPS331AP_PRS_DEV_NAME, LPS331AP_PRS_I2C_SAD_H),
		.platform_data = &lps331_pdata,
	},
};

static void lps331ap_gpio_init(void)
{
	int ret = gpio_request(GPIO_BARO_INT, "lps331_irq");

	pr_info("%s\n", __func__);

	if (ret)
		pr_err("Failed to request gpio lps331_irq\n");

	s3c_gpio_cfgpin(GPIO_BARO_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_BARO_INT, 2);
	s3c_gpio_setpull(GPIO_BARO_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_BARO_INT, S5P_GPIO_DRVSTR_LV1);
};
#endif

#if defined(CONFIG_SENSORS_BMA254)
static struct i2c_gpio_platform_data bma250_i2c_gpio_data = {
	.sda_pin	= GPIO_GSENSE_SDA_18V,
	.scl_pin	= GPIO_GSENSE_SCL_18V,
};

static struct platform_device bma254_i2c_gpio_device = {
	.name	= "i2c-gpio",
	.id	= I2C_GSENSE,
	.dev	= {
		.platform_data  = &bma250_i2c_gpio_data,
	},
};

static struct i2c_board_info bma254_i2c_devices[] __initdata = {
	{
		I2C_BOARD_INFO("bma254", 0x18),
		.irq = GPIO_ACC_INT,
		.platform_data = &bma250_i2c_gpio_data,
	},
};

static int __init bma254_gpio_init(void)
{
	int ret;

	pr_err("bma254_gpio_init\n");

	s3c_gpio_cfgpin(GPIO_GSENSE_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_GSENSE_SDA_18V, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(GPIO_GSENSE_SDA_18V, S5P_GPIO_DRVSTR_LV1);

	s3c_gpio_cfgpin(GPIO_GSENSE_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_GSENSE_SCL_18V, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(GPIO_GSENSE_SCL_18V, S5P_GPIO_DRVSTR_LV1);

	ret = gpio_request(GPIO_ACC_INT, "bma254_accel_irq");

	if (ret)
		pr_err("Failed to request gpio bma254_accel_irq = %d\n", ret);

	/* Accelerometer sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_ACC_INT, 2);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_ACC_INT, S5P_GPIO_DRVSTR_LV1);
	s5p_register_gpio_interrupt(GPIO_ACC_INT);
	bma254_i2c_devices[0].irq = gpio_to_irq(GPIO_ACC_INT);

	return 0;
}
#endif

/* I2C GPIO: Magnetic (MSENSE) */
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

static struct i2c_board_info i2c_devs_msense[] __initdata = {
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
	},
};

static void ak8975c_gpio_init(void)
{
	int ret = gpio_request(GPIO_MSENSOR_INT, "gpio_akm_int");

	pr_info("%s\n", __func__);

	if (ret)
		pr_err("Failed to request gpio akm_int.\n");

	s5p_register_gpio_interrupt(GPIO_MSENSOR_INT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_SFN(0xF));
	i2c_devs_msense[0].irq = gpio_to_irq(GPIO_MSENSOR_INT);
}

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

#ifdef CONFIG_SENSORS_TAOS
static struct i2c_gpio_platform_data gpio_i2c_psense = {
	.sda_pin    = GPIO_RGB_SDA_1_8V,
	.scl_pin    = GPIO_RGB_SCL_1_8V,
	.udelay		= 2,
};

static struct platform_device device_i2c_psense = {
	.name       = "i2c-gpio",
	.id     = I2C_PSENSE,
	.dev        = {
		.platform_data  = &gpio_i2c_psense,
	},
};

static struct taos_platform_data taos_pdata = {
	.p_out = GPIO_PS_ALS_INT,
};

static struct i2c_board_info taos_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("taos", 0x39),
		.platform_data	= &taos_pdata,
	},
};

static void optical_gpio_init(void)
{
	pr_info("%s\n", __func__);

	s3c_gpio_setpull(GPIO_PS_ALS_INT, S3C_GPIO_PULL_UP);
}
#endif

#ifdef CONFIG_SENSORS_GP2A
/* I2C GPIO: PS_ALS (PSENSE) */
static struct i2c_gpio_platform_data gpio_i2c_psense = {
	.sda_pin = GPIO_PS_ALS_SDA_28V,
	.scl_pin = GPIO_PS_ALS_SCL_28V,
	.udelay = 2, /* 250KHz */
};

static struct platform_device device_i2c_psense = {
	.name = "i2c-gpio",
	.id = I2C_PSENSE,
	.dev.platform_data = &gpio_i2c_psense,
};

static struct i2c_board_info i2c_devs_psense[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x72 >> 1)),
	},
};

static int proximity_leda_on(bool onoff)
{
	pr_info("%s, onoff = %d\n", __func__, onoff);

	gpio_set_value(GPIO_PS_ALS_EN, onoff);

	return 0;
}

static struct gp2a_platform_data gp2a_pdata = {
	.gp2a_led_on	= proximity_leda_on,
	.p_out = GPIO_PS_ALS_INT,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev = {
		.platform_data = &gp2a_pdata,
	},
};

static void optical_gpio_init(void)
{
	int ret = gpio_request(GPIO_PS_ALS_EN, "optical_power_supply_on");

	pr_info("%s\n", __func__);

	if (ret)
		pr_err("Failed to request gpio optical power supply.\n");

	/* configuring for gp2a gpio for LEDA power */
	s3c_gpio_cfgpin(GPIO_PS_ALS_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ALS_EN, 0);
	s3c_gpio_setpull(GPIO_PS_ALS_EN, S3C_GPIO_PULL_NONE);

	s3c_gpio_setpull(GPIO_PS_ALS_INT, S3C_GPIO_PULL_UP);
}
#endif

static struct jack_platform_data midas_jack_data = {
	.usb_online		= 0,
	.charger_online	= 0,
	.hdmi_online	= -1,
	.earjack_online	= 0,
	.earkey_online	= -1,
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
	.polling_ms = 5,
};
static struct platform_device devfreq_busfreq = {
	.name		= "exynos4412-busfreq",
	.id		= -1,
	.dev		= {
		.platform_data = &devfreq_bus_pdata,
	},
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

/* External connector */
static struct extcon_dev midas_usb_extcon = {
	.name			= "usb-connector",
	.supported_cable	= extcon_cable_name,
};

static void midas_extcon_init(void)
{
	int ret;

	ret = extcon_dev_register(&midas_usb_extcon, NULL);
	if (ret)
		pr_err(KERN_ERR "failed to register extcon_dev\n");
}

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
	&s3c_device_i2c0,	/* 5M CAM */
	&s3c_device_i2c1,	/* Gyro/Acc */
	/* i2c2: used by GPS UART */
	&s3c_device_i2c3,	/* Meltas TSP */
	/* i2c4: NC(PQ) / codec: wm1811 (PQ rev01) / Modem(PQ LTE) */
	&s3c_device_i2c4,
	/* i2c5: NC(PQ) / Modem(PQ LTE) */
	&s3c_device_i2c7,	/* MAX77686 PMIC */
#ifdef CONFIG_I2C_SI4705
	&device_i2c_fm_radio,
#endif
	&device_i2c_if_pmic,	/* if_pmic: max77693 */
	&device_i2c_fuel,	/* max17047-fuelgauge */
	#ifdef CONFIG_VIDEO_SLP_DB8131M
	&device_i2c_vtcam, /* I2C GPIO: 1.3M VT CAM */
#endif
#ifdef CONFIG_SENSORS_LPS331
	&device_i2c_bsense,	/* barometer lps331ap */
#endif
#if defined(CONFIG_SENSORS_BMA254)
	&bma254_i2c_gpio_device,
#endif
	&device_i2c_msense, /* magnetic ak8975c */
	&device_i2c_mhl,
#ifdef CONFIG_SENSORS_TAOS
	&device_i2c_psense,
#endif
#ifdef CONFIG_SENSORS_GP2A
	&device_i2c_psense, /* PS_ALS gp2a020 */
#endif
	/* TODO: SW I2C for 8M CAM of PQ (same gpio with PQ_LTE NFC) */
	/* TODO: SW I2C for VT_CAM (GPIO_VT_CAM_SCL/SDA) */
	/* TODO: SW I2C for ADC (GPIO_ADC_SCL/SDA) */
	/* TODO: SW I2C for LTE of PQ_LTE (F2(4) SDA, F2(5) SCL) */


#ifdef CONFIG_DRM_EXYNOS_FIMD
	&s5p_device_fimd0,
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
#ifdef CONFIG_SND_SAMSUNG_RP
	&exynos_device_srp,
#endif
#ifdef CONFIG_USB_EHCI_S5P
	&s5p_device_ehci,
#endif
#ifdef CONFIG_USB_OHCI_S5P
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
#ifdef CONFIG_DRM_EXYNOS
	&exynos_drm_device,
#endif
#ifdef CONFIG_EXYNOS4_DEV_FIMC_IS
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
	&SYSMMU_PLATDEV(mfc_l),
	&SYSMMU_PLATDEV(mfc_r),
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
#endif
	&midas_charger_manager,
	&midas_keypad,
	&midas_jack,
	&midas_uart_select,
	&bcm4334_bluetooth_device,
#if defined(CONFIG_S3C64XX_DEV_SPI)
	&exynos_device_spi1,
#endif
#if defined(CONFIG_ARM_EXYNOS4_BUS_DEVFREQ)
	&devfreq_busfreq,
#endif
#ifdef CONFIG_SENSORS_GP2A
	&opt_gp2a,
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#endif
	&host_notifier_device,

};

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
};
#endif

#if defined(CONFIG_S5P_MEM_CMA)
static void __init exynos4_reserve_mem(void)
{
	static struct cma_region regions[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1)
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
		{
			.name = "fimc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
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
		},
#endif
#ifdef CONFIG_DRM_EXYNOS
		{
			.name = "drm",
			.size = CONFIG_DRM_EXYNOS_MEMSIZE * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_EXYNOS4_DEV_FIMC_IS
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

	static const char map[] __initconst =
		"s3cfb.0=fimd;"
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
		"exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_VIDEO_MFC5X
		"s3c-mfc=mfc,mfc0,mfc1;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		"s5p-mfc/f=fw;"
		"s5p-mfc/a=b1;"
		"s5p-mfc/b=b2;"
#endif
#ifdef CONFIG_EXYNOS4_DEV_FIMC_IS
		"exynos4-fimc-is=fimc_is;"
#endif
#ifdef CONFIG_DRM_EXYNOS
		"exynos-drm=drm"
#endif
		""
	;

	cma_set_defaults(regions, map);
	cma_early_regions_reserve(NULL);
}
#endif

static void __init midas_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(slp_midas_uartcfgs, ARRAY_SIZE(slp_midas_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
	exynos4_reserve_mem();
#endif

#ifdef CONFIG_SEC_DEBUG
	/* as soon as INFORM6 is visible, sec_debug is ready to run */
	sec_debug_init();
#endif

}

#ifdef CONFIG_MDNIE_SUPPORT
static int exynos4_common_setup_clock(const char *sclk_name,
	const char *pclk_name, unsigned long rate, unsigned int rate_set)
{
	struct clk *sclk = NULL;
	struct clk *pclk = NULL;

	sclk = clk_get(NULL, sclk_name);
	if (IS_ERR(sclk)) {
		pr_err("failed to get %s clock.\n", sclk_name);
		goto err_clk;
	}

	pclk = clk_get(NULL, pclk_name);
	if (IS_ERR(pclk)) {
		pr_err("failed to get %s clock.\n", pclk_name);
		goto err_clk;
	}

	clk_set_parent(sclk, pclk);

	pr_info("set parent clock of %s to %s\n", sclk_name,
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

static void __init midas_fb_init(void)
{
#ifdef CONFIG_S5P_MIPI_DSI2
	struct s5p_platform_mipi_dsim *dsim_pd;

	s5p_device_mipi_dsim0.dev.platform_data = (void *)&dsim_platform_data;
	dsim_pd = (struct s5p_platform_mipi_dsim *)&dsim_platform_data;

	strcpy(dsim_pd->lcd_panel_name, "s6e39a0x02");
	dsim_pd->lcd_panel_info = (void *)&drm_fimd_pdata.panel.timing;

	s5p_mipi_dsi_register_lcd_device(&mipi_lcd_device);
	if (hwrevision(1))
		platform_device_register(&lcd_fixed_reg_device);
#ifdef CONFIG_MDNIE_SUPPORT
	s5p_fimd_ext_device_register(&exynos4_mdnie_device);
	s5p_fimd_ext_device_register(&exynos4_fimd_lite_device);
#endif
	platform_device_register(&s5p_device_mipi_dsim0);
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMD
	s5p_device_fimd0.dev.platform_data = &drm_fimd_pdata;
#endif
	lcd_cfg_gpio();
}

static void __init exynos_sysmmu_init(void)
{
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
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
			pr_err("Failed to get %s for %s\n",
					con_id, dev.init_name);
			continue;
		}
		clk_enable(clk);
		clk_disable(clk);
		clk_put(clk);
	}
}

static void __init midas_machine_init(void)
{
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
/*
#ifndef NAPLES_BT_BRING_UP
	naples_config_init_bt_gpio_table();
#endif
*/
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

#ifdef CONFIG_SENSORS_LSM330DLC
	/* LSM330DLC (Gyro & Accelerometer Sensor) */
	s3c_i2c1_set_platdata(NULL);
	lsm331dlc_gpio_init();
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif

	s3c_i2c3_set_platdata(NULL);

#ifdef CONFIG_TOUCHSCREEN_MELFAS
	midas_tsp_set_platdata(&melfas_tsp_pdata);
	midas_tsp_init();
#endif
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT224
	naples_tsp_init();
#endif

#ifdef CONFIG_I2C_SI4705
	GPIO_I2C_PIN_SETUP(fm_radio);
	naples_si4705_init();
	i2c_register_board_info(I2C_FM_RADIO, i2c_devs_fm_radio,
				ARRAY_SIZE(i2c_devs_fm_radio));
#endif

	/* Audio Codec WM1811 */
	naples_audio_init();
	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));

	s3c_i2c7_set_platdata(NULL);
	s3c_i2c7_set_platdata(NULL);

#if defined(CONFIG_REGULATOR_MAX77686)
	/* Workaround for repeated interrupts from MAX77686 during sleep */
	exynos4_max77686_info.wakeup = 0;
#endif
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

	GPIO_I2C_PIN_SETUP(if_pmic);
	midas_power_set_muic_pdata(NULL, EXYNOS4_GPX0(7));
	i2c_register_board_info(I2C_IF_PMIC, i2c_devs_if_pmic,
				ARRAY_SIZE(i2c_devs_if_pmic));

	/* NFC */
	GPIO_I2C_PIN_SETUP(nfc);
	platform_device_register(&device_i2c_nfc);
	midas_nfc_init(device_i2c_nfc.id);

	/* MHL / MHL_D */
	GPIO_I2C_PIN_SETUP(mhl);

	GPIO_I2C_PIN_SETUP(mhl_d);
	platform_device_register(&device_i2c_mhl_d);

#ifdef CONFIG_SENSORS_LPS331
	lps331ap_gpio_init();
	GPIO_I2C_PIN_SETUP(bsense);
	i2c_register_board_info(I2C_BSENSE, i2c_devs_bsense,
				ARRAY_SIZE(i2c_devs_bsense));
#endif

#ifdef CONFIG_SENSORS_BMA254
	bma254_gpio_init();
	i2c_register_board_info(I2C_GSENSE, bma254_i2c_devices,
			ARRAY_SIZE(bma254_i2c_devices));
#endif

	ak8975c_gpio_init();

	GPIO_I2C_PIN_SETUP(msense);
	i2c_register_board_info(I2C_MSENSE, i2c_devs_msense,
				ARRAY_SIZE(i2c_devs_msense));

#ifdef CONFIG_SENSORS_TAOS
	optical_gpio_init();
	GPIO_I2C_PIN_SETUP(psense);
	i2c_register_board_info(I2C_PSENSE, taos_i2c_board_info,
				ARRAY_SIZE(taos_i2c_board_info));
#endif
#ifdef CONFIG_SENSORS_GP2A
	optical_gpio_init();
	GPIO_I2C_PIN_SETUP(psense);
	i2c_register_board_info(I2C_PSENSE, i2c_devs_psense,
				ARRAY_SIZE(i2c_devs_psense));
#endif

#ifdef CONFIG_USB_EHCI_S5P
	smdk4212_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
	smdk4212_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
	smdk4212_usbgadget_init();
#endif

	/* home key pullup */
	s3c_gpio_cfgpin(EXYNOS4_GPX2(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(EXYNOS4_GPX2(0), S3C_GPIO_PULL_UP);

	GPIO_I2C_PIN_SETUP(fuel);
	i2c_register_board_info(I2C_FUEL, i2c_devs_fuel,
				ARRAY_SIZE(i2c_devs_fuel));

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	exynos_dwmci_set_platdata(&exynos_dwmci_pdata, 0);
#else
	s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
	s3c_sdhci2_set_platdata(&slp_midas_hsmmc2_pdata);
	s3c_sdhci3_set_platdata(&slp_midas_hsmmc3_pdata);
#ifdef CONFIG_EXYNOS4_DEV_FIMC_IS
	exynos4_fimc_is_set_platdata(NULL);
	exynos4_device_fimc_is.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif
	/* Init for Naples camera(FIMC) */
	midas_camera_init();
#ifdef CONFIG_VIDEO_SLP_DB8131M
	i2c_register_board_info(I2C_VTCAM, i2c_devs_vtcam,
				ARRAY_SIZE(i2c_devs_vtcam));
#endif
#ifdef CONFIG_DRM_EXYNOS_FIMD
	/*
	 * platform device name for fimd driver should be changed
	 * because we can get source clock with this name.
	 *
	 * P.S. refer to sclk_fimd definition of clock-exynos4.c
	 */
	s5p_fb_setname(0, "s3cfb");
	s5p_device_fimd0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif

	/* VOL_UP/DOWN keys are not EXTINT. Register them. */
	s5p_register_gpio_interrupt(GPIO_VOL_UP);
	s5p_register_gpio_interrupt(GPIO_VOL_DOWN);

	setup_charger_manager(&midas_charger_g_desc);

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&midas_tmu_data);
#endif

#ifdef CONFIG_S5P_MIPI_DSI2
	s5p_device_mipi_dsim0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
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

	platform_add_devices(slp_midas_devices, ARRAY_SIZE(slp_midas_devices));

	/* Extcon */
	midas_extcon_init();

	midas_fb_init();
#ifdef CONFIG_MDNIE_SUPPORT
	exynos4_common_setup_clock("sclk_mdnie", "mout_mpll_user",
				400 * MHZ, 1);
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
		pr_err("Unable to set parent %s of clock %s.\n",
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
}

MACHINE_START(SLP_PQ, "SLP_NAPLES")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= midas_map_io,
	.init_machine	= midas_machine_init,
	.timer		= &exynos4_timer,
MACHINE_END
