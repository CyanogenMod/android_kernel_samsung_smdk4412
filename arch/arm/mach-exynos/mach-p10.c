/* linux/arch/arm/mach-exynos/mach-p10.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/mmc/host.h>
#include <linux/memblock.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include <video/platform_lcd.h>
#include <video/s5p-dp.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <media/exynos_gscaler.h>
#include <media/exynos_flite.h>
#include <media/exynos_fimc_is.h>
#include <plat/gpio-cfg.h>
#include <plat/adc.h>
#include <plat/regs-serial.h>
#include <plat/exynos5.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/hwmon.h>
#include <plat/devs.h>
#include <plat/sdhci.h>

#include <plat/fb.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/ehci.h>
#include <plat/s5p-mfc.h>
#include <plat/dp.h>
#include <plat/backlight.h>
#include <plat/usbgadget.h>
#include <plat/fimg2d.h>
#include <plat/tv-core.h>
#include <plat/s3c64xx-spi.h>

#include <plat/mipi_csis.h>
#include <mach/map.h>
#include <mach/exynos-ion.h>
#include <mach/sysmmu.h>
#include <mach/spi-clocks.h>
#include <mach/ppmu.h>
#include <mach/dev.h>
#include <mach/pmu.h>
#include <mach/regs-pmu.h>
#include <mach/dwmci.h>
#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#ifdef CONFIG_EXYNOS_C2C
#include <mach/c2c.h>
#endif

#ifdef CONFIG_VIDEO_EXYNOS_TV
#include <plat/tvout.h>
#endif

#ifdef CONFIG_MFD_MAX77686
#include <linux/mfd/max77686.h>
#endif

#ifdef CONFIG_SENSORS_BH1721FVC
#include <linux/bh1721fvc.h>
#endif

#include <mach/gpio-p10.h>
#include <mach/regs-clock.h>
#include <mach/regs-pmu5.h>
#include <mach/midas-sound.h>

#if defined(CONFIG_SEC_DEBUG)
#include <mach/sec_debug.h>
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050
#include <linux/mpu_411.h>
#endif

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif
#include <plat/media.h>

#ifdef CONFIG_BATTERY_SAMSUNG_P1X
#include <mach/p10-battery.h>
#endif

#ifdef CONFIG_STMPE811_ADC
#include <linux/stmpe811-adc.h>
struct stmpe811_platform_data stmpe811_pdata;
#endif

#include <mach/p10-input.h>
#ifdef CONFIG_LEDS_SPFCW043
#include <linux/leds-spfcw043.h>
#endif

#include "p10-wlan.h"

#ifdef CONFIG_FB_S5P_EXTDSP
struct s3cfb_extdsp_lcd {
	int	width;
	int	height;
	int	bpp;
};
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050
	struct mpu_platform_data mpu6050_data = {
	.int_config = 0x10,
	.orientation = {0, 1, 0,
			1, 0, 0,
			0, 0, -1},
	.enable_irq_handler = NULL,
	};

static struct ext_slave_platform_data mpu_ak8975_data = {
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.adapt_num	= 11,
	.orientation = {1, 0, 0,
			0, 1, 0,
			0, 0, 1},
	.address = 0x0C,
	.irq = IRQ_EINT(18),
	};

static struct i2c_gpio_platform_data gpio_i2c_data11 = {
	.sda_pin = GPIO_MSENSE_SDA,
	.scl_pin = GPIO_MSENSE_SCL,
};

struct platform_device s3c_device_i2c11 = {
	.name = "i2c-gpio",
	.id = 11,
	.dev.platform_data = &gpio_i2c_data11,
};

static struct i2c_board_info i2c_devs11[] __initdata = {
	{
		I2C_BOARD_INFO("ak8975_mod", 0x0C),
		.irq = IRQ_EINT(18),
		.platform_data = &mpu_ak8975_data,
	},
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("mpu6050",0x68),
		.irq = IRQ_EINT(12),
		.platform_data = &mpu6050_data,
	},
/*
	{
		I2C_BOARD_INFO("ak8975_mod",0x0C),
		.irq = IRQ_EINT(18),
		.platform_data = &mpu_ak8975_data,
	},
*/
};

void magnetic_init()
{
	pr_info("%s : AK8963C Init", __func__);
	s3c_gpio_cfgpin(GPIO_MSENSE_RST, S3C_GPIO_SFN(S3C_GPIO_OUTPUT));
	s3c_gpio_setpull(GPIO_MSENSE_RST, S3C_GPIO_PULL_UP);
	gpio_set_value(GPIO_MSENSE_RST, 0);
	usleep_range(20, 20);
	gpio_set_value(GPIO_MSENSE_RST, 1);
}

#else
static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("dummy", (0x10)),
	}
};
#endif /* CONFIG_MPU_SENSORS_MPU6050 */


#define REG_INFORM4	       (S5P_INFORM4)

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK5250_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDK5250_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK5250_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

#ifdef CONFIG_30PIN_CONN
#include <linux/30pin_con.h>
#endif

#ifdef CONFIG_MOTOR_DRV_ISA1200
#include <linux/isa1200_vibrator.h>
#endif

static struct s3c2410_uartcfg smdk5250_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDK5250_UCON_DEFAULT,
		.ulcon		= SMDK5250_ULCON_DEFAULT,
		.ufcon		= SMDK5250_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDK5250_UCON_DEFAULT,
		.ulcon		= SMDK5250_ULCON_DEFAULT,
		.ufcon		= SMDK5250_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDK5250_UCON_DEFAULT,
		.ulcon		= SMDK5250_ULCON_DEFAULT,
		.ufcon		= SMDK5250_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDK5250_UCON_DEFAULT,
		.ulcon		= SMDK5250_ULCON_DEFAULT,
		.ufcon		= SMDK5250_UFCON_DEFAULT,
	},
};

#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
struct platform_device exynos_device_md0 = {
	.name = "exynos-mdev",
	.id = 0,
};

struct platform_device exynos_device_md1 = {
	.name = "exynos-mdev",
	.id = 1,
};
struct platform_device exynos_device_md2 = {
	.name = "exynos-mdev",
	.id = 2,
};
#endif

#if defined(CONFIG_DP_40HZ_P10)
#define FPS 40
#elif defined(CONFIG_DP_60HZ_P10)
#define FPS 60
#endif
#ifdef CONFIG_FB_S3C
#if defined(CONFIG_MACH_P10_DP_01)

static struct s3c_fb_pd_win p10_fb_win0 = {
	.win_mode = {
		.refresh	= FPS,
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win p10_fb_win1 = {
	.win_mode = {
		.refresh	= FPS,
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},

	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};

static struct s3c_fb_pd_win p10_fb_win2 = {
	.win_mode = {
		.refresh	= FPS,
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},

	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};

static struct s3c_fb_pd_win p10_fb_win3 = {
	.win_mode = {
		.refresh	= FPS,
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},

	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};

static struct s3c_fb_pd_win p10_fb_win4 = {
	.win_mode = {
		.refresh	= FPS,
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},

	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};

#elif defined(CONFIG_MACH_P10_DP_00)

static struct s3c_fb_pd_win p10_fb_win0 = {
	.win_mode = {
		.refresh	= 20,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 20,
		.lower_margin	= 3,
		.hsync_len	= 16,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win p10_fb_win1 = {
	.win_mode = {
		.refresh	= 20,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 20,
		.lower_margin	= 3,
		.hsync_len	= 16,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,

	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};

static struct s3c_fb_pd_win p10_fb_win2 = {
	.win_mode = {
		.refresh	= 20,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 20,
		.lower_margin	= 3,
		.hsync_len	= 16,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,

	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,

};
#endif
static void exynos_fimd_gpio_setup_24bpp(void)
{
	unsigned int reg = 0;
	unsigned int uRead = 0;

#if defined(CONFIG_S5P_DP)
		/* Set Hotplug detect for DP */
		s3c_gpio_cfgpin(GPIO_DP_HPD, S3C_GPIO_SFN(3));
#endif
	/*
	 * Set DISP1BLK_CFG register for Display path selection
	 *
	 * FIMD of DISP1_BLK Bypass selection : DISP1BLK_CFG[15]
	 * ---------------------
	 *  0 | MIE/MDNIE
	 *  1 | FIMD : selected
	 */
	reg = __raw_readl(S3C_VA_SYS + 0x0214);
	reg &= ~(1 << 15);	/* To save other reset values */
	reg |= (1 << 15);
	__raw_writel(reg, S3C_VA_SYS + 0x0214);
#if defined(CONFIG_S5P_DP)
#if defined(CONFIG_MACH_P10_DP_01)
	// MPLL => FIMD Bus clock
	uRead = __raw_readl(EXYNOS5_CLKSRC_TOP0);
	uRead = (uRead & ~(0x3<<14)) | (0x0<<14);
	__raw_writel(uRead, EXYNOS5_CLKSRC_TOP0);

	uRead = __raw_readl(EXYNOS5_CLKDIV_TOP0);
	uRead = (uRead & ~(0x7<<28)) | (0x2<<28);
	__raw_writel(uRead,EXYNOS5_CLKDIV_TOP0);

	/* Reference clcok selection for DPTX_PHY: pad_osc_clk_24M */
	reg = __raw_readl(S3C_VA_SYS + 0x04d4);
	reg = (reg & ~(0x1 << 0)) | (0x0 << 0);
	__raw_writel(reg, S3C_VA_SYS + 0x04d4);

	/* DPTX_PHY: XXTI */
	reg = __raw_readl(S3C_VA_SYS + 0x04d8);
	reg = (reg & ~(0x1 << 3)) | (0x0 << 3);
	__raw_writel(reg, S3C_VA_SYS + 0x04d8);
#elif defined(CONFIG_MACH_P10_DP_00)

	reg = __raw_readl(S3C_VA_SYS + 0x04d4);
	reg |= (1 << 0);
	__raw_writel(reg, S3C_VA_SYS + 0x04d4);

	/* DPTX_PHY: XXTI */
	reg = __raw_readl(S3C_VA_SYS + 0x04d8);
	reg &= ~(1 << 3);
	__raw_writel(reg, S3C_VA_SYS + 0x04d8);
#endif
#endif
}

static struct s3c_fb_platdata p10_lcd1_pdata __initdata = {
	.win[0]		= &p10_fb_win0,
	.win[1]		= &p10_fb_win1,
	.win[2]		= &p10_fb_win2,
	.win[3]		= &p10_fb_win3,
	.win[4]		= &p10_fb_win4,
	.default_win	= 2,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= 0,
	.setup_gpio	= exynos_fimd_gpio_setup_24bpp,
};
#endif

#if defined CONFIG_VIDEO_EXYNOS5_FIMC_IS
static struct exynos5_platform_fimc_is exynos5_fimc_is_data;

#if defined CONFIG_VIDEO_S5K4E5
static struct exynos5_fimc_is_sensor_info s5k4e5= {
	.sensor_name = "S5K4E5",
	.sensor_id = SENSOR_NAME_S5K4E5,
#if defined CONFIG_S5K4E5_POSITION_FRONT
	.sensor_position = SENSOR_POSITION_FRONT,
#elif  defined CONFIG_S5K4E5_POSITION_REAR
	.sensor_position = SENSOR_POSITION_REAR,
#endif
#if defined CONFIG_S5K4E5_CSI_C
	.csi_id = CSI_ID_A,
	.flite_id = FLITE_ID_A,
	.i2c_channel = SENSOR_CONTROL_I2C0,
#elif  defined CONFIG_S5K4E5_CSI_D
	.csi_id = CSI_ID_B,
	.flite_id = FLITE_ID_B,
	.i2c_channel = SENSOR_CONTROL_I2C1,
#endif

	.max_width = 2560,
	.max_height = 1920,
	.max_frame_rate = 30,

	.mipi_lanes = 2,
	.mipi_settle = 12,
	.mipi_align = 24,
};
#endif

#if defined CONFIG_VIDEO_S5K6A3
static struct exynos5_fimc_is_sensor_info s5k6a3= {
	.sensor_name = "S5K6A3",
	.sensor_id = SENSOR_NAME_S5K6A3,
#if defined CONFIG_S5K6A3_POSITION_FRONT
	.sensor_position = SENSOR_POSITION_FRONT,
#elif  defined CONFIG_S5K6A3_POSITION_REAR
	.sensor_position = SENSOR_POSITION_REAR,
#endif
#if defined CONFIG_S5K6A3_CSI_C
	.csi_id = CSI_ID_A,
	.flite_id = FLITE_ID_A,
	.i2c_channel = SENSOR_CONTROL_I2C0,
#elif  defined CONFIG_S5K6A3_CSI_D
	.csi_id = CSI_ID_B,
	.flite_id = FLITE_ID_B,
	.i2c_channel = SENSOR_CONTROL_I2C1,
#endif

	.max_width = 1280,
	.max_height = 720,
	.max_frame_rate = 30,

	.mipi_lanes = 1,
	.mipi_settle = 12,
	.mipi_align = 24,
};
#endif
#endif

#ifdef CONFIG_S5P_DP
static void dp_lcd_set_power(struct plat_lcd_data *pd,
				unsigned int power)
{
	if (power) {

		/* LCD_PWM_IN_2.8V, AH21, XPWMOUT_0 => LCD_B_PWM */
#ifndef CONFIG_BACKLIGHT_PWM
		gpio_request_one(GPIO_LCD_PWM_IN_18V, GPIOF_OUT_INIT_LOW, "GPB2");
#endif

#ifdef CONFIG_MACH_P10_00_BD
		/* LCD_APS_EN_2.8V, R6, XCI1RGB_2 => GPG0_2 */
		gpio_request_one(GPIO_LCD_APS_EN_18V, GPIOF_OUT_INIT_LOW, "GPG0");
#endif
		/* LCD_EN , XMMC2CDN => GPC2_2 */
		gpio_request_one(GPIO_LCD_EN, GPIOF_OUT_INIT_LOW, "GPC2");

		/* LCD_EN , XMMC2CDN => GPC2_2 */
		gpio_set_value(GPIO_LCD_EN, 1);

#ifdef CONFIG_MACH_P10_00_BD
		/* LCD_APS_EN_2.8V, R6, XCI1RGB_2 => GPG0_2 */
		gpio_set_value(GPIO_LCD_APS_EN_18V, 1);
#endif

		udelay(1000);

		/* LCD_PWM_IN_2.8V, AH21, XPWMOUT_0=> LCD_B_PWM */
#ifndef CONFIG_BACKLIGHT_PWM
		gpio_set_value(GPIO_LCD_PWM_IN_18V, 1);
#endif
	} else {
		/* LCD_PWM_IN_2.8V, AH21, XPWMOUT_0=> LCD_B_PWM */
#ifndef CONFIG_BACKLIGHT_PWM
		gpio_set_value(GPIO_LCD_PWM_IN_18V, 0);
#endif

#ifdef CONFIG_MACH_P10_00_BD
		/* LCD_APS_EN_2.8V, R6, XCI1RGB_2 => GPG0_2 */
		gpio_set_value(GPIO_LCD_APS_EN_18V, 0);
#endif

		/* LCD_EN , XMMC2CDN => GPC2_2 */
		gpio_set_value(GPIO_LCD_EN, 0);

#ifdef CONFIG_MACH_P10_00_BD
		/* LCD_APS_EN_2.8V, R6, XCI1RGB_2 => GPG0_2 */
		gpio_free(GPIO_LCD_APS_EN_18V);
#endif

		/* LCD_EN , XMMC2CDN => GPC2_2 */
		gpio_free(GPIO_LCD_EN);

#ifndef CONFIG_BACKLIGHT_PWM
		gpio_free(GPIO_LCD_PWM_IN_18V);
#endif
	}
}

static struct plat_lcd_data p10_dp_lcd_data = {
	.set_power	= dp_lcd_set_power,
};

static struct platform_device p10_dp_lcd = {
	.name		   = "platform-lcd",
	.dev.parent	   = &s5p_device_fimd1.dev,
	.dev.platform_data = &p10_dp_lcd_data,
};

static struct video_info p10_dp_config = {
	.name			= "for p10 TEST",
#if defined(CONFIG_MACH_P10_DP_01)
	.h_sync_polarity	= 0,
	.v_sync_polarity	= 0,
	.interlaced		= 0,

	.color_space		= COLOR_RGB,
	.dynamic_range		= VESA,
	.ycbcr_coeff		= COLOR_YCBCR601,
	.color_depth		= COLOR_8,

	.link_rate		= LINK_RATE_2_70GBPS,
	.lane_count		= LANE_COUNT4,

#elif defined(CONFIG_MACH_P10_DP_00)

	.h_sync_polarity	= 0,
	.v_sync_polarity	= 0,
	.interlaced		= 0,

	.color_space		= COLOR_RGB,
	.dynamic_range		= VESA,
	.ycbcr_coeff		= COLOR_YCBCR601,
	.color_depth		= COLOR_8,

	.link_rate		= LINK_RATE_1_62GBPS,
	.lane_count		= LANE_COUNT4,
#endif
};

static void s5p_dp_backlight_on(void)
{
	/* LED_BACKLIGHT_RESET: XCI1RGB_5 => GPG0_5 */
	gpio_request_one(GPIO_LED_BACKLIGHT_RESET, GPIOF_OUT_INIT_LOW, "GPG0");

	gpio_set_value(GPIO_LED_BACKLIGHT_RESET, 1);
}

static void s5p_dp_backlight_off(void)
{
	/* LED_BACKLIGHT_RESET: XCI1RGB_5 => GPG0_5 */
	gpio_set_value(GPIO_LED_BACKLIGHT_RESET, 0);

	gpio_free(GPIO_LED_BACKLIGHT_RESET);

}

static struct s5p_dp_platdata p10_dp_data __initdata = {
	.video_info = &p10_dp_config,
	.phy_init	= s5p_dp_phy_init,
	.phy_exit	= s5p_dp_phy_exit,
	.backlight_on	= s5p_dp_backlight_on,
	.backlight_off	= s5p_dp_backlight_off,
};
#endif

/* LCD Backlight data */
#ifdef CONFIG_BACKLIGHT_PWM
static struct samsung_bl_gpio_info p10_bl_gpio_info = {
	.no = GPIO_LCD_PWM_IN_18V,
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data p10_bl_data = {
	.pwm_id = 0,
	.pwm_period_ns	= 10000,
};
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
static struct s5p_mfc_platdata smdk5250_mfc_pd = {
	.clock_rate = 333000000,
};
#endif

#ifdef CONFIG_EXYNOS_C2C
struct exynos_c2c_platdata smdk5250_c2c_pdata = {
	.setup_gpio	= NULL,
	.shdmem_addr	= C2C_SHAREDMEM_BASE,
	.shdmem_size	= C2C_MEMSIZE_64,
	.ap_sscm_addr	= NULL,
	.cp_sscm_addr	= NULL,
	.rx_width	= C2C_BUSWIDTH_16,
	.tx_width	= C2C_BUSWIDTH_16,
	.clk_opp100	= 400,
	.clk_opp50	= 200,
	.clk_opp25	= 100,
	.default_opp_mode	= C2C_OPP25,
	.get_c2c_state	= NULL,
	.c2c_sysreg	= S3C_VA_SYS + 0x0360,
};
#endif

static int exynos5_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	int mode = 0;

	if ((code == SYS_RESTART) && _cmd)
		if (!strcmp((char *)_cmd, "recovery"))
			mode = 0xf;

	__raw_writel(mode, REG_INFORM4);

	return NOTIFY_DONE;
}

static struct notifier_block exynos5_reboot_notifier = {
	.notifier_call = exynos5_notifier_call,
};

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
static void exynos_dwmci_cfg_gpio(int width)
{
	unsigned int gpio;

	for (gpio = EXYNOS5_GPC0(0); gpio < EXYNOS5_GPC0(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
	}

	switch (width) {
	case MMC_BUS_WIDTH_8:
		for (gpio = EXYNOS5_GPC1(3); gpio <= EXYNOS5_GPC1(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(4));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		}
	case MMC_BUS_WIDTH_4:
		for (gpio = EXYNOS5_GPC0(3); gpio <= EXYNOS5_GPC0(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		}
		break;
	case MMC_BUS_WIDTH_1:
		gpio = EXYNOS5_GPC0(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
	default:
		break;
	}
}

static struct dw_mci_board exynos_dwmci_pdata __initdata = {
	.num_slots		= 1,
	.quirks			= DW_MCI_QUIRK_BROKEN_CARD_DETECTION | DW_MCI_QUIRK_HIGHSPEED,
	.bus_hz			= 66 * 1000 * 1000,
	.caps			= MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR |
				  MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	.fifo_depth             = 0x80,
	.detect_delay_ms	= 200,
	.hclk_name		= "dwmci",
	.cclk_name		= "sclk_dwmci",
	.cfg_gpio		= exynos_dwmci_cfg_gpio,
};
#endif

static void exynos_dwmci0_cfg_gpio(int width)
{
	unsigned int gpio;

	for (gpio = EXYNOS5_GPC0(0); gpio < EXYNOS5_GPC0(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV4);
	}

	switch (width) {
	case MMC_BUS_WIDTH_8:
		for (gpio = EXYNOS5_GPC1(0); gpio <= EXYNOS5_GPC1(3); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV4);
		}
	case MMC_BUS_WIDTH_4:
		for (gpio = EXYNOS5_GPC0(3); gpio <= EXYNOS5_GPC0(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV4);
		}
		break;
	case MMC_BUS_WIDTH_1:
		gpio = EXYNOS5_GPC0(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV4);
	default:
		break;
	}
}

static struct dw_mci_board exynos5_dwmci0_pdata __initdata = {
	.num_slots		= 1,
	.quirks			= DW_MCI_QUIRK_BROKEN_CARD_DETECTION | DW_MCI_QUIRK_HIGHSPEED,
	.bus_hz			= 100 * 1000 * 1000,
	.caps			= MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR |
				  MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	.fifo_depth             = 0x80,
	.detect_delay_ms	= 200,
	.hclk_name		= "dwmci",
	.cclk_name		= "sclk_dwmci",
	.cfg_gpio		= exynos_dwmci0_cfg_gpio,
};

static void exynos_dwmci1_cfg_gpio(int width)
{
	unsigned int gpio;
	for (gpio = EXYNOS5_GPC2(0); gpio < EXYNOS5_GPC2(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
	}

	switch (width) {
	case MMC_BUS_WIDTH_4:
		for (gpio = EXYNOS5_GPC2(3); gpio <= EXYNOS5_GPC2(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
		}
		break;
	case MMC_BUS_WIDTH_1:
		gpio = EXYNOS5_GPC2(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
	default:
		break;
	}
}

static void (*wlan_notify_func)(struct platform_device *dev, int state);
static DEFINE_MUTEX(wlan_mutex_lock);

static int ext_cd_init_wlan(void (*notify_func)(struct platform_device *dev, int state))
{
	mutex_lock(&wlan_mutex_lock);
	WARN_ON(wlan_notify_func);

	wlan_notify_func = notify_func;

	if (wlan_notify_func) {
		if (samsung_rev() >= EXYNOS5250_REV_1_0)
			wlan_notify_func(&exynos_device_dwmci1, 1);
		else
			wlan_notify_func(&s3c_device_hsmmc3, 1);
	}

	mutex_unlock(&wlan_mutex_lock);

	return 0;
}

static int ext_cd_cleanup_wlan(void (*notify_func)(struct platform_device *dev, int state))
{
	mutex_lock(&wlan_mutex_lock);
	WARN_ON(wlan_notify_func);

	if (wlan_notify_func) {
		if (samsung_rev() >= EXYNOS5250_REV_1_0)
			wlan_notify_func(&exynos_device_dwmci1, 0);
		else
			wlan_notify_func(&s3c_device_hsmmc3, 0);
	}

	wlan_notify_func = NULL;

	mutex_unlock(&wlan_mutex_lock);

	return 0;
}

static struct dw_mci_board exynos5_dwmci1_pdata __initdata = {
	.num_slots              = 1,
	.quirks                 = DW_MCI_QUIRK_BROKEN_CARD_DETECTION | DW_MCI_QUIRK_HIGHSPEED,
	.bus_hz                 = 50 * 1000 * 1000,
	.caps                   = MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR |
				  MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	.fifo_depth             = 0x80,
	.detect_delay_ms        = 200,
	.hclk_name              = "dwmci",
	.cclk_name              = "sclk_dwmci",
	.cfg_gpio               = exynos_dwmci1_cfg_gpio,
	.ext_cd_init 		= ext_cd_init_wlan,
	.ext_cd_cleanup 	= ext_cd_cleanup_wlan,
	.cd_type 		= DW_MCI_CD_EXTERNAL,
};

void mmc_force_presence_change(struct platform_device *pdev)
{
	void (*notify_func)(struct platform_device *, int state) = NULL;
	mutex_lock(&wlan_mutex_lock);
	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		if (pdev == &exynos_device_dwmci1)
			notify_func = wlan_notify_func;
	} else {
		if (pdev == &s3c_device_hsmmc3)
			notify_func = wlan_notify_func;
	}

	if (notify_func)
		notify_func(pdev, 1);
	else
		pr_warn("%s: called for device with no notifier\n", __func__);
	mutex_unlock(&wlan_mutex_lock);
}
EXPORT_SYMBOL_GPL(mmc_force_presence_change);

static int smdk5250_dwmci_get_ro(u32 slot_id)
{
	/* smdk5250 rev1.0 did not support SD/MMC card write pritect. */
	return 0;
}

static void exynos_dwmci2_cfg_gpio(int width)
{
	unsigned int gpio;

	for (gpio = EXYNOS5_GPC3(0); gpio <= EXYNOS5_GPC3(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
	}

	switch (width) {
	case MMC_BUS_WIDTH_4:
		for (gpio = EXYNOS5_GPC3(3); gpio <= EXYNOS5_GPC3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		}
		break;
	case MMC_BUS_WIDTH_1:
		gpio = EXYNOS5_GPC3(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
	default:
		break;
	}
}

static struct dw_mci_board exynos5_dwmci2_pdata __initdata = {
	.num_slots		= 1,
	.quirks			= DW_MCI_QUIRK_HIGHSPEED,
	.bus_hz			= 22 * 1000 * 1000,
	.caps			= MMC_CAP_CMD23,
	.fifo_depth             = 0x80,
	.detect_delay_ms	= 200,
	.hclk_name		= "dwmci",
	.cclk_name		= "sclk_dwmci",
	.cfg_gpio		= exynos_dwmci2_cfg_gpio,
	.get_ro		= smdk5250_dwmci_get_ro,
};

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver		= 0x42,
	.gate_clkname	= "fimg2d",
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdk5250_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdk5250_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk5250_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.ext_cd_gpio		= GPIO_T_FLASH_DETECT,
	.ext_cd_gpio_invert	= true,
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

struct class *camera_class;
EXPORT_SYMBOL(camera_class);

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdk5250_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags		= S3C_SDHCI_PM_IGNORE_SUSPEND_RESUME,
	/* ext_cd_xxx should be used in only .cd_type = S3C_SDHCI_CD_EXTERNAL */
	.ext_cd_init		= ext_cd_init_wlan,
	.ext_cd_cleanup		= ext_cd_cleanup_wlan,
};
#endif

#ifdef CONFIG_S3C64XX_DEV_SPI
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = GPIO_5M_SPI_CS,
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#ifdef CONFIG_LEDS_SPFCW043
static int spfcw043_setGpio(void)
{
	int err;

	err = gpio_request(GPIO_CAM_FLASH_EN, "TORCH_EN");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_EN\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_CAM_FLASH_EN, 1);
	err = gpio_request(GPIO_CAM_FLASH_SET, "TORCH_SET");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_SET\n");
		gpio_free(GPIO_CAM_FLASH_EN);
		return -EPERM;
	}
	gpio_direction_output(GPIO_CAM_FLASH_SET, 1);
	gpio_set_value(GPIO_CAM_FLASH_EN, 0);
	gpio_set_value(GPIO_CAM_FLASH_SET, 0);

	return 0;
}

static int spfcw043_freeGpio(void)
{
	gpio_free(GPIO_CAM_FLASH_EN);
	gpio_free(GPIO_CAM_FLASH_SET);

	return 0;
}

static void spfcw043_torch_en(int onoff)
{
	gpio_set_value(GPIO_CAM_FLASH_EN, onoff);
}

static void spfcw043_torch_set(int onoff)
{
	gpio_set_value(GPIO_CAM_FLASH_SET, onoff);
}

static struct spfcw043_led_platform_data spfcw043_led_data = {
	.brightness = TORCH_BRIGHTNESS_50,
	.status	= STATUS_UNAVAILABLE,
	.setGpio = spfcw043_setGpio,
	.freeGpio = spfcw043_freeGpio,
	.torch_en = spfcw043_torch_en,
	.torch_set = spfcw043_torch_set,
};

static struct platform_device s3c_device_spfcw043_led = {
	.name	= "spfcw043-led",
	.id	= -1,
	.dev	= {
		.platform_data	= &spfcw043_led_data,
	},
};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
#if defined(CONFIG_ITU_A)
static int smdk5250_cam0_reset(int dummy)
{
	int err;
	/* Camera A */
	err = gpio_request(EXYNOS5_GPX1(2), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_2 ####\n");

	s3c_gpio_setpull(EXYNOS5_GPX1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS5_GPX1(2), 0);
	gpio_direction_output(EXYNOS5_GPX1(2), 1);
	gpio_free(EXYNOS5_GPX1(2));

	return 0;
}
#endif
#if defined(CONFIG_ITU_B)
static int smdk5250_cam1_reset(int dummy)
{
	int err;
	/* Camera A */
	err = gpio_request(EXYNOS5_GPX1(0), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_2 ####\n");

	s3c_gpio_setpull(EXYNOS5_GPX1(0), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS5_GPX1(0), 0);
	gpio_direction_output(EXYNOS5_GPX1(0), 1);
	gpio_free(EXYNOS5_GPX1(0));

	return 0;
}
#endif

#ifdef CONFIG_VIDEO_S5K4BA
static struct s5k4ba_mbus_platform_data s5k4ba_mbus_plat = {
	.id		= 0,
	.fmt = {
		.width	= 1600,
		.height	= 1200,
		/* .code	= V4L2_MBUS_FMT_UYVY8_2X8, */
		.code	= V4L2_MBUS_FMT_VYUY8_2X8,
	},
	.clk_rate	= 24000000UL,
#ifdef CONFIG_ITU_A
	.set_power	= smdk5250_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.set_power	= smdk5250_cam1_reset,
#endif
};

static struct i2c_board_info s5k4ba_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_mbus_plat,
};
#endif

/* 1 MIPI Cameras */
#ifdef CONFIG_VIDEO_M5MOLS
static struct m5mols_platform_data m5mols_platdata = {
#ifdef CONFIG_CSI_C
	.gpio_rst = EXYNOS5_GPX1(2), /* ISP_RESET */
#endif
#ifdef CONFIG_CSI_D
	.gpio_rst = EXYNOS5_GPX1(0), /* ISP_RESET */
#endif
	.enable_rst = true, /* positive reset */
	.irq = IRQ_EINT(22),
};

static struct i2c_board_info m5mols_board_info = {
	I2C_BOARD_INFO("M5MOLS", 0x1F),
	.platform_data = &m5mols_platdata,
};
#endif
#endif /* CONFIG_VIDEO_EXYNOS_FIMC_LITE */

#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
static struct regulator_consumer_supply mipi_csi_fixed_voltage_supplies[] = {
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.0"),
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.1"),
};

static struct regulator_init_data mipi_csi_fixed_voltage_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(mipi_csi_fixed_voltage_supplies),
	.consumer_supplies	= mipi_csi_fixed_voltage_supplies,
};

static struct fixed_voltage_config mipi_csi_fixed_voltage_config = {
	.supply_name	= "DC_5V",
	.microvolts	= 5000000,
	.gpio		= -EINVAL,
	.init_data	= &mipi_csi_fixed_voltage_init_data,
};

static struct platform_device mipi_csi_fixed_voltage = {
	.name		= "reg-fixed-voltage",
	.id		= 3,
	.dev		= {
		.platform_data	= &mipi_csi_fixed_voltage_config,
	},
};
#endif

#ifdef CONFIG_VIDEO_M5MOLS
static struct regulator_consumer_supply m5mols_fixed_voltage_supplies[] = {
	REGULATOR_SUPPLY("core", NULL),
	REGULATOR_SUPPLY("dig_18", NULL),
	REGULATOR_SUPPLY("d_sensor", NULL),
	REGULATOR_SUPPLY("dig_28", NULL),
	REGULATOR_SUPPLY("a_sensor", NULL),
	REGULATOR_SUPPLY("dig_12", NULL),
};

static struct regulator_init_data m5mols_fixed_voltage_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(m5mols_fixed_voltage_supplies),
	.consumer_supplies	= m5mols_fixed_voltage_supplies,
};

static struct fixed_voltage_config m5mols_fixed_voltage_config = {
	.supply_name	= "CAM_SENSOR",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &m5mols_fixed_voltage_init_data,
};

static struct platform_device m5mols_fixed_voltage = {
	.name		= "reg-fixed-voltage",
	.id		= 4,
	.dev		= {
		.platform_data	= &m5mols_fixed_voltage_config,
	},
};
#endif

#if defined(CONFIG_REGULATOR_MAX77686)
/* max77686 */
#ifdef CONFIG_SND_SOC_WM8994
static struct regulator_consumer_supply ldo3_supply[] = {
	REGULATOR_SUPPLY("vcc_1.8v", NULL),
	REGULATOR_SUPPLY("AVDD2", NULL),
	REGULATOR_SUPPLY("CPVDD", NULL),
	REGULATOR_SUPPLY("DBVDD1", NULL),
	REGULATOR_SUPPLY("DBVDD2", NULL),
	REGULATOR_SUPPLY("DBVDD3", NULL)
};
#else
static struct regulator_consumer_supply ldo3_supply[] = {
	REGULATOR_SUPPLY("vcc_1.8v", NULL),
};
#endif

static struct regulator_consumer_supply ldo4_supply[] = {
	REGULATOR_SUPPLY("vcc_2.8v", NULL),
};

static struct regulator_consumer_supply ldo5_supply[] = {
	REGULATOR_SUPPLY("cam_isp_mipi_1.2v", NULL),
};

static struct regulator_consumer_supply ldo8_supply[] = {
	REGULATOR_SUPPLY("vmipi_1.0v", NULL),
};

static struct regulator_consumer_supply ldo9_supply[] = {
	REGULATOR_SUPPLY("touch_vdd_1.8v", NULL),
};

static struct regulator_consumer_supply ldo10_supply[] = {
	REGULATOR_SUPPLY("vmipi_1.8v", NULL),
};

static struct regulator_consumer_supply ldo11_supply[] = {
	REGULATOR_SUPPLY("vabb1_1.9v", NULL),
};

static struct regulator_consumer_supply ldo12_supply[] = {
	REGULATOR_SUPPLY("votg_3.0v", NULL),
};

static struct regulator_consumer_supply ldo14_supply[] = {
	REGULATOR_SUPPLY("vabb02_1.8v", NULL),
};

static struct regulator_consumer_supply ldo15_supply[] = {
	REGULATOR_SUPPLY("vhsic_1.0v", NULL),
};

static struct regulator_consumer_supply ldo16_supply[] = {
	REGULATOR_SUPPLY("vhsic_1.8v", NULL),
};

#if defined(CONFIG_MACH_P10_LUNGO_01_BD) || \
	defined(CONFIG_MACH_P10_LUNGO_WIFI_01_BD)
static struct regulator_consumer_supply ldo17_supply[] = {
	REGULATOR_SUPPLY("cam_core_1.8v", NULL),
};
#endif
static struct regulator_consumer_supply ldo18_supply[] = {
	REGULATOR_SUPPLY("cam_io_from_1.8v", NULL),
};

static struct regulator_consumer_supply ldo19_supply[] = {
	REGULATOR_SUPPLY("vt_cam_1.8v", NULL),
};

static struct regulator_consumer_supply ldo20_supply[] = {
	REGULATOR_SUPPLY("vmem_vdd_1.8v", NULL),
};

static struct regulator_consumer_supply ldo21_supply[] = {
	REGULATOR_SUPPLY("vtf_2.8v", NULL),
};

#if defined(CONFIG_MACH_P10_LUNGO_01_BD) || \
	defined(CONFIG_MACH_P10_LUNGO_WIFI_01_BD)
static struct regulator_consumer_supply ldo22_supply[] = {
	REGULATOR_SUPPLY("vcc_mmc_2.8v", NULL),
};
#else
static struct regulator_consumer_supply ldo22_supply[] = {
	REGULATOR_SUPPLY("cam_core_1.8v", NULL),
};
#endif
static struct regulator_consumer_supply ldo23_supply[] = {
	REGULATOR_SUPPLY("touch_avdd", NULL),
};

static struct regulator_consumer_supply ldo24_supply[] = {
	REGULATOR_SUPPLY("cam_af_2.8v", NULL),
};
static struct regulator_consumer_supply ldo25_supply[] = {
	REGULATOR_SUPPLY("vadc_3.3v", NULL),
};

static struct regulator_consumer_supply ldo26_supply[] = {
	REGULATOR_SUPPLY("irda_3.3v", NULL),
};

static struct regulator_consumer_supply max77686_buck1 =
	REGULATOR_SUPPLY("vdd_mif", NULL);
static struct regulator_consumer_supply max77686_buck2 =
	REGULATOR_SUPPLY("vdd_arm", NULL);

static struct regulator_consumer_supply max77686_buck3 =
	REGULATOR_SUPPLY("vdd_int", NULL);

static struct regulator_consumer_supply max77686_buck4 =
	REGULATOR_SUPPLY("vdd_g3d", NULL);

static struct regulator_consumer_supply max77686_buck9 =
	REGULATOR_SUPPLY("cam_isp_core", NULL);

static struct regulator_consumer_supply max77686_enp32khz[] = {
	REGULATOR_SUPPLY("lpo_in", "bcm47511"),
	REGULATOR_SUPPLY("lpo", "bcm43241_bluetooth"),
};

#define REGULATOR_INIT(_ldo, _name, _min_uV, _max_uV, _always_on, _ops_mask,\
	_disabled) \
static struct regulator_init_data _ldo##_init_data = {		\
	.constraints = {					\
		.name	= _name,				\
		.min_uV = _min_uV,				\
		.max_uV = _max_uV,				\
		.always_on	= _always_on,			\
		.boot_on	= _always_on,			\
		.apply_uV	= 1,				\
		.valid_ops_mask = _ops_mask,			\
		.state_mem	= {				\
		.disabled	= _disabled,		\
		.enabled	= !(_disabled),		\
		}						\
	    },							    \
	    .num_consumer_supplies = ARRAY_SIZE(_ldo##_supply),	    \
	    .consumer_supplies = &_ldo##_supply[0],		    \
	       };

REGULATOR_INIT(ldo3, "VCC_1.8V_AP", 1800000, 1800000, 1,
	0, 0);
REGULATOR_INIT(ldo4, "VCC_2.8V_AP", 2800000, 2800000, 1,
	0, 0);
REGULATOR_INIT(ldo5, "CAM_ISP_MIPI_1.2V", 1200000, 1200000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo8, "VMIPI_1.0V", 1000000, 1000000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo9, "TOUCH_VDD_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo10, "VMIPI_1.8V", 1800000, 1800000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo11, "VABB1_1.8V", 1800000, 1800000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo12, "VUOTG_3.0V", 3000000, 3000000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo14, "VABB02_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo15, "VHSIC_1.0V", 1000000, 1000000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo16, "VHSIC_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
#if defined(CONFIG_MACH_P10_LUNGO_01_BD) || \
	defined(CONFIG_MACH_P10_LUNGO_WIFI_01_BD)
REGULATOR_INIT(ldo17, "CAM_CORE_1.8v", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
#endif
REGULATOR_INIT(ldo18, "CAM_IO_FROM_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo19, "VT_CAM_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo20, "VMEM_VDD_1.8V", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo21, "VTF_2.8V", 2800000, 2800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
#if defined(CONFIG_MACH_P10_LUNGO_01_BD) || \
	defined(CONFIG_MACH_P10_LUNGO_WIFI_01_BD)
REGULATOR_INIT(ldo22, "VCC_MMC_2.8v", 2800000, 2800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo22, "CAM_CORE_1.8v", 1800000, 1800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
#endif
REGULATOR_INIT(ldo23, "TSP_AVDD_2.8V", 2800000, 2800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo24, "CAM_AF_2.8V", 2800000, 2800000, 0,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo25, "VADC_3.3V", 3300000, 3300000, 1,
	REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo26, "IRDA_3.3V", 3300000, 3300000, 0,
	REGULATOR_CHANGE_STATUS, 1);

static struct regulator_init_data max77686_buck1_data = {
	.constraints = {
		.name = "vdd_mif range",
		.min_uV = 900000,
		.max_uV = 1300000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &max77686_buck1,
};

static struct regulator_init_data max77686_buck2_data = {
	.constraints = {
		.name = "vdd_arm range",
		.min_uV = 800000,
		.max_uV = 1500000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &max77686_buck2,
};

static struct regulator_init_data max77686_buck3_data = {
	.constraints = {
		.name = "vdd_int range",
		.min_uV = 900000,
		.max_uV = 1300000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &max77686_buck3,
};

static struct regulator_init_data max77686_buck4_data = {
	 .constraints = {
		 .name = "vdd_g3d range",
		 .min_uV = 700000,
		 .max_uV = 1300000,
		 .boot_on = 1,
		 .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
		 REGULATOR_CHANGE_STATUS,
	 },
	 .num_consumer_supplies = 1,
	 .consumer_supplies = &max77686_buck4,
};

static struct regulator_init_data max77686_buck9_data = {
	  .constraints = {
		.name = "cam_isp_core",
		.min_uV = 1000000,
		.max_uV = 1200000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
			 REGULATOR_CHANGE_STATUS,
		.state_mem = {
		.disabled = 1,
		},
	 },
	 .num_consumer_supplies = 1,
	 .consumer_supplies = &max77686_buck9,
};

static struct regulator_init_data max77686_enp32khz_data = {
	.constraints = {
		.name = "32KHZ_PMIC",
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.enabled	= 1,
			.disabled	= 0,
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(max77686_enp32khz),
	.consumer_supplies = max77686_enp32khz,
};

static struct max77686_regulator_data max77686_regulators[] = {
		{MAX77686_BUCK1, &max77686_buck1_data,},
		{MAX77686_BUCK2, &max77686_buck2_data,},
		{MAX77686_BUCK3, &max77686_buck3_data,},
		{MAX77686_BUCK4, &max77686_buck4_data,},
		{MAX77686_BUCK9, &max77686_buck9_data,},
		{MAX77686_LDO3, &ldo3_init_data,},
		{MAX77686_LDO4, &ldo4_init_data,},
		{MAX77686_LDO5, &ldo5_init_data,},
		{MAX77686_LDO8, &ldo8_init_data,},
		{MAX77686_LDO9, &ldo9_init_data,},
		{MAX77686_LDO10, &ldo10_init_data,},
		{MAX77686_LDO11, &ldo11_init_data,},
		{MAX77686_LDO12, &ldo12_init_data,},
		{MAX77686_LDO14, &ldo14_init_data,},
		{MAX77686_LDO15, &ldo15_init_data,},
		{MAX77686_LDO16, &ldo16_init_data,},
#if defined(CONFIG_MACH_P10_LUNGO_01_BD) || \
	defined(CONFIG_MACH_P10_LUNGO_WIFI_01_BD)
		{MAX77686_LDO17, &ldo17_init_data,},
#endif
		{MAX77686_LDO18, &ldo18_init_data,},
		{MAX77686_LDO19, &ldo19_init_data,},
		{MAX77686_LDO20, &ldo20_init_data,},
		{MAX77686_LDO21, &ldo21_init_data,},
		{MAX77686_LDO22, &ldo22_init_data,},
		{MAX77686_LDO23, &ldo23_init_data,},
		{MAX77686_LDO24, &ldo24_init_data,},
		{MAX77686_LDO25, &ldo25_init_data,},
		{MAX77686_LDO26, &ldo26_init_data,},
		{MAX77686_P32KH, &max77686_enp32khz_data,},
};

struct max77686_opmode_data max77686_opmode_data[MAX77686_REG_MAX] = {
		[MAX77686_LDO3] = {MAX77686_LDO3, MAX77686_OPMODE_NORMAL},
		[MAX77686_LDO8] = {MAX77686_LDO8, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO10] = {MAX77686_LDO10, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO11] = {MAX77686_LDO11, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO12] = {MAX77686_LDO12, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO14] = {MAX77686_LDO14, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO15] = {MAX77686_LDO15, MAX77686_OPMODE_STANDBY},
		[MAX77686_LDO16] = {MAX77686_LDO16, MAX77686_OPMODE_STANDBY},
		[MAX77686_BUCK1] = {MAX77686_BUCK1, MAX77686_OPMODE_STANDBY},
		[MAX77686_BUCK2] = {MAX77686_BUCK2, MAX77686_OPMODE_STANDBY},
		[MAX77686_BUCK3] = {MAX77686_BUCK3, MAX77686_OPMODE_STANDBY},
		[MAX77686_BUCK4] = {MAX77686_BUCK4, MAX77686_OPMODE_STANDBY},
};

static struct max77686_platform_data exynos4_max77686_info = {
		.num_regulators = ARRAY_SIZE(max77686_regulators),
		.regulators = max77686_regulators,
		.irq_gpio	= GPIO_PMIC_IRQ,
		.irq_base	= IRQ_BOARD_PMIC_START,
		.wakeup = 1,

		.opmode_data = max77686_opmode_data,
		.ramp_rate = MAX77686_RAMP_RATE_27MV,
		.has_full_constraints = 1,

		.buck234_gpio_dvs = {
			GPIO_PMIC_DVS1,
			GPIO_PMIC_DVS2,
			GPIO_PMIC_DVS3,
		},
		.buck234_gpio_selb = {
			GPIO_BUCK2_SEL,
			GPIO_BUCK3_SEL,
			GPIO_BUCK4_SEL,
		},

		/*for future work after DVS Table */
		.buck2_voltage[0] = 1100000,	/* 1.1V */
		.buck2_voltage[1] = 1100000,	/* 1.1V */
		.buck2_voltage[2] = 1100000,	/* 1.1V */
		.buck2_voltage[3] = 1100000,	/* 1.1V */
		.buck2_voltage[4] = 1100000,	/* 1.1V */
		.buck2_voltage[5] = 1100000,	/* 1.1V */
		.buck2_voltage[6] = 1100000,	/* 1.1V */
		.buck2_voltage[7] = 1100000,	/* 1.1V */

		.buck3_voltage[0] = 1100000,	/* 1.1V */
		.buck3_voltage[1] = 1100000,	/* 1.1V */
		.buck3_voltage[2] = 1100000,	/* 1.1V */
		.buck3_voltage[3] = 1100000,	/* 1.1V */
		.buck3_voltage[4] = 1100000,	/* 1.1V */
		.buck3_voltage[5] = 1100000,	/* 1.1V */
		.buck3_voltage[6] = 1100000,	/* 1.1V */
		.buck3_voltage[7] = 1100000,	/* 1.1V */

		.buck4_voltage[0] = 1100000,	/* 1.1V */
		.buck4_voltage[1] = 1100000,	/* 1.1V */
		.buck4_voltage[2] = 1100000,	/* 1.1V */
		.buck4_voltage[3] = 1100000,	/* 1.1V */
		.buck4_voltage[4] = 1100000,	/* 1.1V */
		.buck4_voltage[5] = 1100000,	/* 1.1V */
		.buck4_voltage[6] = 1100000,	/* 1.1V */
		.buck4_voltage[7] = 1100000,	/* 1.1V */
};
#endif /* CONFIG_REGULATOR_MAX77686 */

static struct i2c_board_info i2c_devs0[] __initdata = {
#ifdef CONFIG_VIDEO_EXYNOS_TV
	{
		I2C_BOARD_INFO("exynos_hdcp", (0x74 >> 1)),
	}
#endif
};

#ifdef CONFIG_S3C_DEV_HWMON
static struct s3c_hwmon_pdata smdk5250_hwmon_pdata __initdata = {
	/* Reference voltage (1.2V) */
	.in[0] = &(struct s3c_hwmon_chcfg) {
		.name		= "smdk:reference-voltage",
		.mult		= 3300,
		.div		= 4096,
	},
};
#endif

#if defined(CONFIG_REGULATOR_MAX77686)
static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	}
};
#endif

#ifdef CONFIG_SENSORS_BH1721FVC

static struct i2c_gpio_platform_data gpio_i2c_data12 = {
	.sda_pin = GPIO_PS_ALS_SDA,
	.scl_pin = GPIO_PS_ALS_SCL,
};

struct platform_device s3c_device_i2c12 = {
	.name = "i2c-gpio",
	.id = 12,
	.dev.platform_data = &gpio_i2c_data12,
};

static int light_sensor_init(void)
{
	int err;

	printk(KERN_INFO"==============================\n");
	printk(KERN_INFO"== BH1721 Light Sensor Init ==\n");
	printk(KERN_INFO"==============================\n");
	printk("%d %d\n", GPIO_PS_ALS_SDA, GPIO_PS_ALS_SCL);
	err = gpio_request(GPIO_PS_VOUT, "LIGHT_SENSOR_RESET");
	if (err) {
		printk(KERN_INFO" bh1721fvc Failed to request the light "
			" sensor gpio (%d)\n", err);
		return err;
	}

	s3c_gpio_cfgpin(GPIO_PS_VOUT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_PS_VOUT, S3C_GPIO_PULL_NONE);

	err = gpio_direction_output(GPIO_PS_VOUT, 0);
	udelay(2);
	err = gpio_direction_output(GPIO_PS_VOUT, 1);
	if (err) {
		printk(KERN_INFO" bh1721fvc Failed to make the light sensor gpio(reset)"
			" high (%d)\n", err);
		return err;
	}

	return 0;
}

static int  bh1721fvc_light_sensor_reset(void)
{
	int err;

	printk(KERN_INFO" bh1721fvc_light_sensor_reset\n");
	err = gpio_direction_output(GPIO_PS_VOUT, 0);
	if (err) {
		printk(KERN_INFO" bh1721fvc Failed to make the light sensor gpio(reset)"
			" low (%d)\n", err);
		return err;
	}

	udelay(2);

	err = gpio_direction_output(GPIO_PS_VOUT, 1);
	if (err) {
		printk(KERN_INFO" bh1721fvc Failed to make the light sensor gpio(reset)"
			" high (%d)\n", err);
		return err;
	}
	return 0;
}

static int  bh1721fvc_light_sensor_output(int value)
{
	int err;
	int gpio_vout = GPIO_PS_VOUT;

	err = gpio_direction_output(GPIO_PS_VOUT, value);
	if (err) {
		printk(KERN_INFO" bh1721fvc Failed to make the light sensor gpio(dvi)"
			" low (%d)\n", err);
		return err;
	}
	return 0;
}

static struct bh1721fvc_platform_data bh1721fvc_pdata = {
	.reset = bh1721fvc_light_sensor_reset,
	/* .output = bh1721fvc_light_sensor_output, */
};

static struct i2c_board_info i2c_bh1721_emul[] __initdata = {
	{
		I2C_BOARD_INFO("bh1721fvc", 0x23),
		.platform_data = &bh1721fvc_pdata,
	},
};
#endif

#ifdef CONFIG_SENSORS_SHT21

static struct i2c_gpio_platform_data gpio_i2c_data13 = {
	.sda_pin = GPIO_HUM_SDA,
	.scl_pin = GPIO_HUM_SCL,
};

struct platform_device s3c_device_i2c13 = {
	.name = "i2c-gpio",
	.id = 13,
	.dev.platform_data = &gpio_i2c_data13,
};

static struct i2c_board_info i2c_devs13[] __initdata = {
	{
		I2C_BOARD_INFO("sht21", 0x40),
	},
};

#endif

#if defined(CONFIG_SAMSUNG_MHL)
static struct i2c_board_info i2c_devs15_emul[] __initdata = {
};

/* i2c-gpio emulation platform_data */
static struct i2c_gpio_platform_data i2c15_platdata = {
	.sda_pin		= GPIO_MHL_SDA_18V,
	.scl_pin		= GPIO_MHL_SCL_18V,
	.udelay			= 2,	/* 250 kHz*/
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c15 = {
	.name			= "i2c-gpio",
	.id			= 15,
	.dev.platform_data	= &i2c15_platdata,
};

#endif

#ifdef CONFIG_MOTOR_DRV_ISA1200

static int isa1200_vdd_en(bool en)
{
	return gpio_direction_output(GPIO_MOTOR_EN, en);
}

static struct i2c_gpio_platform_data gpio_i2c_data17 = {
	.sda_pin = GPIO_MOTOR_SDA_18V,
	.scl_pin = GPIO_MOTOR_SCL_18V,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &gpio_i2c_data17,
};

static struct isa1200_vibrator_platform_data isa1200_vibrator_pdata = {
	.gpio_en = isa1200_vdd_en,
	.max_timeout = 10000,
	.ctrl0 = CTL0_DIVIDER128 | CTL0_PWM_INPUT,
	.ctrl1 = CTL1_DEFAULT,
	.ctrl2 = 0,
	.ctrl4 = 0,
	.pll = 0,
	.duty = 0,
	.period = 0,
	.get_clk = NULL,
	.pwm_id = 1,
	.pwm_duty = 37000,
	.pwm_period = 38675,/*38109*/
};
static struct i2c_board_info i2c_devs17[] = {
	{
		I2C_BOARD_INFO("isa1200_vibrator",  0x48),
		.platform_data = &isa1200_vibrator_pdata,
	},
};

static void isa1200_init(void)
{
	int gpio, ret;

	gpio = GPIO_MOTOR_EN;
	gpio_request(gpio, "MOTOR_EN");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);
}
#endif	/* CONFIG_MOTOR_DRV_ISA1200 */

#ifdef CONFIG_30PIN_CONN

static void __init acc_con_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_DOCK_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_DOCK_INT, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_ACCESSORY_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ACCESSORY_INT, S3C_GPIO_PULL_NONE);
}

static int acc_dock_con_state(void)
{
	/* From ACCESSROY_INT like desktop dock */

	return gpio_get_value(GPIO_ACCESSORY_INT);
}

static int acc_accessory_con_state(void)
{

	/* From ACCESSORY_ID pin */
	return gpio_get_value(GPIO_DOCK_INT);
}

struct acc_con_platform_data acc_con_pdata = {
    /*
	.otg_en =
	.acc_power =
	.usb_ldo_en =
    */
	.get_dock_state = acc_dock_con_state,
	.get_acc_state = acc_accessory_con_state,
	.accessory_irq_gpio = GPIO_ACCESSORY_INT,
	.dock_irq_gpio = GPIO_DOCK_INT,
	.mhl_irq_gpio = GPIO_MHL_INT,
	.hdmi_hpd_gpio = GPIO_HDMI_HPD,
};
struct platform_device sec_device_connector = {
	.name = "acc_con",
	.id = -1,
	.dev.platform_data = &acc_con_pdata,
};
#endif

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk5250_ehci_pdata;

static void __init smdk5250_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk5250_ehci_pdata;

#ifndef CONFIG_USB_EXYNOS_SWITCH
	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		if (gpio_request_one(EXYNOS5_GPX2(6), GPIOF_OUT_INIT_HIGH,
			"HOST_VBUS_CONTROL"))
			printk(KERN_ERR "failed to request gpio_host_vbus\n");
		else {
			s3c_gpio_setpull(EXYNOS5_GPX2(6), S3C_GPIO_PULL_NONE);
			gpio_free(EXYNOS5_GPX2(6));
		}
	}
#endif

	s5p_ehci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata smdk5250_ohci_pdata;

static void __init smdk5250_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk5250_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
#endif

#if defined(CONFIG_CPU_EXYNOS5250)
static void set_usb3_en(int enable)
{
	int err;
	/* XMMC2CDN(USB3.0_EN) for P10 H/W */
	err = gpio_request(EXYNOS5_GPC2(2), "USB3_EN");
	if (err)
		printk(KERN_ERR "usb: failed to request XMMC2CDN GPIO ####\n");

	s3c_gpio_setpull(EXYNOS5_GPC2(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS5_GPC2(2), enable);
	gpio_free(EXYNOS5_GPC2(2));
	printk(KERN_INFO "usb: set usb3_en gpio (%d)\n", enable);
}
#endif

/* USB GADGET */
#ifdef CONFIG_USB_S3C_OTGD
static struct s5p_usbgadget_platdata smdk5250_usbgadget_pdata;

static void __init smdk5250_usbgadget_init(void)
{
	struct s5p_usbgadget_platdata *pdata = &smdk5250_usbgadget_pdata;

	s5p_usbgadget_set_platdata(pdata);
}
#endif

#ifdef CONFIG_EXYNOS_DEV_SS_UDC
static struct exynos_usb3_drd_pdata smdk5250_ss_udc_pdata;

static void __init smdk5250_ss_udc_init(void)
{
	struct exynos_usb3_drd_pdata *pdata = &smdk5250_ss_udc_pdata;

	exynos_ss_udc_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_XHCI_EXYNOS
static struct exynos_usb3_drd_pdata smdk5250_xhci_pdata;

static void __init smdk5250_xhci_init(void)
{
	struct exynos_usb3_drd_pdata *pdata = &smdk5250_xhci_pdata;

	exynos_xhci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
static struct platform_device samsung_device_battery = {
	.name	= "samsung-fake-battery",
	.id	= -1,
};
#endif

#if defined(CONFIG_STMPE811_ADC)
static struct i2c_gpio_platform_data gpio_i2c_data19 = {
	.sda_pin = GPIO_ADC_SDA_18V,
	.scl_pin = GPIO_ADC_SCL_18V,
};

struct platform_device s3c_device_i2c19 = {
	.name = "i2c-gpio",
	.id = 19,
	.dev.platform_data = &gpio_i2c_data19,
};


/* I2C19 */
static struct i2c_board_info i2c_devs19_emul[] __initdata = {
	{
		I2C_BOARD_INFO("stmpe811-adc", (0x82 >> 1)),
		.platform_data	= &stmpe811_pdata,
	},
};
#endif

#ifdef CONFIG_BUSFREQ_OPP
/* BUSFREQ to control memory/bus*/
static struct device_domain busfreq;

static struct platform_device exynos5_busfreq = {
	.id = -1,
	.name = "exynos-busfreq",
};
#endif

/* Bluetooth */
#ifdef CONFIG_BT_BCM43241
static struct platform_device bcm43241_bluetooth_device = {
	.name = "bcm43241_bluetooth",
	.id = -1,
};
#endif

static struct platform_device *p10_devices[] __initdata = {
	/* Samsung Power Domain */
#ifdef CONFIG_EXYNOS_DEV_PD
	&exynos5_device_pd[PD_MFC],
	&exynos5_device_pd[PD_G3D],
	&exynos5_device_pd[PD_ISP],
	&exynos5_device_pd[PD_GSCL],
	&exynos5_device_pd[PD_DISP1],
#endif

#ifdef CONFIG_S5P_DP
	&s5p_device_dp,
#endif

#ifdef CONFIG_FB_S3C
	&s5p_device_fimd1,
#endif

#ifdef CONFIG_S5P_DP
	&p10_dp_lcd,
#endif

#ifdef CONFIG_FB_S5P_EXTDSP
	&s3c_device_extdsp,
#endif

	&s3c_device_wdt,

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
	&s5p_device_mfc,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
	&s5p_device_jpeg,
#endif

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	&exynos_device_dwmci,
#endif
	&exynos_device_dwmci0,
	&exynos_device_dwmci1,
	&exynos_device_dwmci2,
#ifdef CONFIG_ION_EXYNOS
	&exynos_device_ion,
#endif

#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif

#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
	&exynos_device_md0,
	&exynos_device_md1,
	&exynos_device_md2,
#endif

#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
	&exynos5_device_fimc_is,
#endif

#ifdef CONFIG_VIDEO_EXYNOS_GSCALER
	&exynos5_device_gsc0,
	&exynos5_device_gsc1,
	&exynos5_device_gsc2,
	&exynos5_device_gsc3,
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
	&exynos_device_flite2,
#endif

#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
	&s5p_device_mipi_csis0,
	&s5p_device_mipi_csis1,
	&mipi_csi_fixed_voltage,
#endif

#ifdef CONFIG_VIDEO_M5MOLS
	&m5mols_fixed_voltage,
#endif

	&s3c_device_rtc,

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[1],
#endif

#ifdef CONFIG_VIDEO_EXYNOS_TV
#ifdef CONFIG_VIDEO_EXYNOS_HDMI
	&s5p_device_hdmi,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_HDMIPHY
	&s5p_device_i2c_hdmiphy,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_MIXER
	&s5p_device_mixer,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
	&s5p_device_cec,
#endif
#endif
	&s3c_device_i2c0,
#ifdef CONFIG_MPU_SENSORS_MPU6050
	&s3c_device_i2c1,
#endif
	&s3c_device_i2c3,
	&s3c_device_i2c4,
	&s3c_device_i2c5,
	&s3c_device_i2c7,

#ifdef CONFIG_USB_EHCI_S5P
	&s5p_device_ehci,
#endif

#ifdef CONFIG_USB_OHCI_S5P
	&s5p_device_ohci,
#endif

#ifdef CONFIG_USB_S3C_OTGD
	&s3c_device_usbgadget,
#endif

#ifdef CONFIG_EXYNOS_DEV_SS_UDC
	&exynos_device_ss_udc,
#endif

#ifdef CONFIG_USB_XHCI_EXYNOS
	&exynos_device_xhci,
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
	&samsung_device_battery,
#endif
#ifdef CONFIG_MOTOR_DRV_ISA1200
	&s3c_device_i2c17,
#endif
#if defined(CONFIG_STMPE811_ADC)
	&s3c_device_i2c19,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos_device_i2s0,
#endif

	&samsung_asoc_dma,
	&samsung_asoc_idma,

#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos_device_pcm0,
#endif

#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos_device_srp,
#endif

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#endif

#ifdef CONFIG_S5P_DEV_ACE
	&s5p_device_ace,
#endif

#ifdef CONFIG_EXYNOS_C2C
	&exynos_device_c2c,
#endif

#ifdef CONFIG_S3C64XX_DEV_SPI
	&exynos_device_spi1,
#endif

#ifdef CONFIG_BUSFREQ_OPP
	&exynos5_busfreq,
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050
	&s3c_device_i2c11,
#endif
#ifdef CONFIG_SENSORS_BH1721FVC
	&s3c_device_i2c12,
#endif

#ifdef CONFIG_SENSORS_SHT21
	&s3c_device_i2c13,
#endif

#if defined(CONFIG_SAMSUNG_MHL)
	&s3c_device_i2c15,	/* MHL */
#endif

#ifdef CONFIG_30PIN_CONN
	&sec_device_connector,
#endif

#ifdef CONFIG_VIDEO_EXYNOS_ROTATOR
	&exynos_device_rotator,
#endif
#ifdef CONFIG_BT_BCM43241
	&bcm43241_bluetooth_device,
#endif
};

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
/* below temperature base on the celcius degree */
struct s5p_platform_tmu exynos_tmu_data __initdata = {
	.ts = {
		.stop_1st_throttle  = 78,
		.start_1st_throttle = 80,
		.stop_2nd_throttle  = 87,
		.start_2nd_throttle = 103,
		.start_tripping = 110, /* temp to do tripping */
		.start_emergency    = 120, /* To protect chip,forcely kernel panic */
		.stop_mem_throttle  = 80,
		.start_mem_throttle = 85,
	},
	.cpufreq = {
		.limit_1st_throttle  = 800000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
	},
};

#endif

#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#if defined(CONFIG_CMA)
static void __init exynos_reserve_mem(void)
{
	static struct cma_region regions[] = {
		{
			.name = "ion",
			.size = 256 * SZ_1M,
			.start = 0
		},
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC0
		{
			.name = "gsc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC0 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC1
		{
			.name = "gsc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC1 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC2
		{
			.name = "gsc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC2 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC3
		{
			.name = "gsc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GSC3 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FLITE0
	       {
		       .name = "flite0",
		       .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FLITE0 * SZ_1K,
		       .start = 0
	       },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FLITE1
	       {
		       .name = "flite1",
		       .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FLITE1 * SZ_1K,
		       .start = 0
	       },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
		{
			.name = "fimd",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		{
			.name = "srp",
			.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		{
			.name		= "fw",
			.size		= 2 << 20,
			{ .alignment	= 128 << 10 },
			.start		= 0x44000000,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV
		{
			.name = "tv",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_ROT
		{
			.name = "rot",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_ROT * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
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
#ifdef CONFIG_EXYNOS_C2C
		"samsung-c2c=c2c_shdmem;"
#endif
		"s3cfb.0=fimd;"
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		"samsung-rp=srp;"
#endif
		"exynos-gsc.0=gsc0;exynos-gsc.1=gsc1;exynos-gsc.2=gsc2;exynos-gsc.3=gsc3;"
		"exynos-fimc-lite.0=flite0;exynos-fimc-lite.1=flite1;"
		"ion-exynos=ion,gsc0,gsc1,gsc2,gsc3,flite0,flite1,fimd,fw,rot;"
		"exynos-rot=rot;"
		"s5p-mfc-v6/f=fw;"
		"s5p-mixer=tv;"
		"exynos5-fimc-is=fimc_is;";

    s5p_cma_region_reserve(regions, NULL, 0, map);
}
#else /* !CONFIG_CMA */
static inline void exynos_reserve_mem(void)
{
}
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
static void __init smdk5250_camera_gpio_cfg(void)
{
	/* CAM A port(b0010) : PCLK, VSYNC, HREF, CLK_OUT */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPH0(0), 4, S3C_GPIO_SFN(2));
	/* CAM A port(b0010) : DATA[0-7] */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPH1(0), 8, S3C_GPIO_SFN(2));
	/* CAM B port(b0010) : PCLK, BAY_RGB[0-6] */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPG0(0), 8, S3C_GPIO_SFN(2));
	/* CAM B port(b0010) : BAY_Vsync, BAY_RGB[7-13] */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPG1(0), 8, S3C_GPIO_SFN(2));
	/* CAM B port(b0010) : BAY_Hsync, BAY_MCLK */
	s3c_gpio_cfgrange_nopull(EXYNOS5_GPG2(0), 2, S3C_GPIO_SFN(2));
	/* This is externel interrupt for m5mo */
#ifdef CONFIG_VIDEO_M5MOLS
	s3c_gpio_cfgpin(EXYNOS5_GPX2(6), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(EXYNOS5_GPX2(6), S3C_GPIO_PULL_NONE);
#endif
}
#endif

#if defined(CONFIG_VIDEO_EXYNOS_GSCALER) && defined(CONFIG_VIDEO_EXYNOS_FIMC_LITE)
#if defined(CONFIG_VIDEO_S5K4BA)
static struct exynos_isp_info s5k4ba = {
	.board_info	= &s5k4ba_info,
	.cam_srclk_name	= "xxti",
	.clk_frequency	= 24000000UL,
	.bus_type	= CAM_TYPE_ITU,
#ifdef CONFIG_ITU_A
	.cam_clk_name	= "sclk_cam0",
	.i2c_bus_num	= 4,
	.cam_port	= CAM_PORT_A, /* A-Port : 0, B-Port : 1 */
#endif
#ifdef CONFIG_ITU_B
	.cam_clk_name	= "sclk_cam1",
	.i2c_bus_num	= 5,
	.cam_port	= CAM_PORT_B, /* A-Port : 0, B-Port : 1 */
#endif
	.flags		= CAM_CLK_INV_VSYNC,
};
/* This is for platdata of fimc-lite */
static struct s3c_platform_camera flite_s5k4ba = {
	.type		= CAM_TYPE_MIPI,
	.use_isp	= true,
	.inv_pclk	= 1,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
};
#endif
#if defined(CONFIG_VIDEO_M5MOLS)
static struct exynos_isp_info m5mols = {
	.board_info	= &m5mols_board_info,
	.cam_srclk_name	= "xxti",
	.clk_frequency	= 24000000UL,
	.bus_type	= CAM_TYPE_MIPI,
#ifdef CONFIG_CSI_C
	.cam_clk_name	= "sclk_cam0",
	.i2c_bus_num	= 4,
	.cam_port	= CAM_PORT_A, /* A-Port : 0, B-Port : 1 */
#endif
#ifdef CONFIG_CSI_D
	.cam_clk_name	= "sclk_cam1",
	.i2c_bus_num	= 5,
	.cam_port	= CAM_PORT_B, /* A-Port : 0, B-Port : 1 */
#endif
	.flags		= CAM_CLK_INV_PCLK | CAM_CLK_INV_VSYNC,
	.csi_data_align = 32,
};
/* This is for platdata of fimc-lite */
static struct s3c_platform_camera flite_m5mo = {
	.type		= CAM_TYPE_MIPI,
	.use_isp	= true,
	.inv_pclk	= 1,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
};
#endif

static void __set_gsc_camera_config(struct exynos_platform_gscaler *data,
					u32 active_index, u32 preview,
					u32 camcording, u32 max_cam)
{
	data->active_cam_index = active_index;
	data->cam_preview = preview;
	data->cam_camcording = camcording;
	data->num_clients = max_cam;
}

static void __set_flite_camera_config(struct exynos_platform_flite *data,
					u32 active_index, u32 max_cam)
{
	data->active_cam_index = active_index;
	data->num_clients = max_cam;
}

static void __init smdk5250_set_camera_platdata(void)
{
	int gsc_cam_index = 0;
	int flite0_cam_index = 0;
	int flite1_cam_index = 0;
#if defined(CONFIG_VIDEO_M5MOLS)
	exynos_gsc0_default_data.isp_info[gsc_cam_index++] = &m5mols;
#if defined(CONFIG_CSI_C)
	exynos_flite0_default_data.cam[flite0_cam_index] = &flite_m5mo;
	exynos_flite0_default_data.isp_info[flite0_cam_index] = &m5mols;
	flite0_cam_index++;
#endif
#if defined(CONFIG_CSI_D)
	exynos_flite1_default_data.cam[flite1_cam_index] = &flite_m5mo;
	exynos_flite1_default_data.isp_info[flite1_cam_index] = &m5mols;
	flite1_cam_index++;
#endif
#endif
	/* flite platdata register */
	__set_flite_camera_config(&exynos_flite0_default_data, 0, flite0_cam_index);
	__set_flite_camera_config(&exynos_flite1_default_data, 0, flite1_cam_index);

	/* gscaler platdata register */
	/* GSC-0 */
	__set_gsc_camera_config(&exynos_gsc0_default_data, 0, 1, 0, gsc_cam_index);

	/* GSC-1 */
	/* GSC-2 */
	/* GSC-3 */
}
#endif /* CONFIG_VIDEO_EXYNOS_GSCALER */

static void __init p10_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk5250_uartcfgs, ARRAY_SIZE(smdk5250_uartcfgs));
	exynos_reserve_mem();

#if defined(CONFIG_SEC_DEBUG)
	/* as soon as INFORM6 is visible, sec_debug is ready to run */
	sec_debug_init();
#endif
}

#ifdef CONFIG_EXYNOS_DEV_SYSMMU
static void __init exynos_sysmmu_init(void)
{
#ifdef CONFIG_VIDEO_JPEG_V2X
	platform_set_sysmmu(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
	platform_set_sysmmu(&SYSMMU_PLATDEV(mfc_lr).dev, &s5p_device_mfc.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
	platform_set_sysmmu(&SYSMMU_PLATDEV(tv).dev, &s5p_device_mixer.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_GSCALER
	platform_set_sysmmu(&SYSMMU_PLATDEV(gsc0).dev,
						&exynos5_device_gsc0.dev);
	platform_set_sysmmu(&SYSMMU_PLATDEV(gsc1).dev,
						&exynos5_device_gsc1.dev);
	platform_set_sysmmu(&SYSMMU_PLATDEV(gsc2).dev,
						&exynos5_device_gsc2.dev);
	platform_set_sysmmu(&SYSMMU_PLATDEV(gsc3).dev,
						&exynos5_device_gsc3.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	platform_set_sysmmu(&SYSMMU_PLATDEV(camif0).dev,
					   &exynos_device_flite0.dev);
	platform_set_sysmmu(&SYSMMU_PLATDEV(camif1).dev,
					   &exynos_device_flite1.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_ROTATOR
	platform_set_sysmmu(&SYSMMU_PLATDEV(rot).dev,
						&exynos_device_rotator.dev);
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	platform_set_sysmmu(&SYSMMU_PLATDEV(2d).dev, &s5p_device_fimg2d.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
	platform_set_sysmmu(&SYSMMU_PLATDEV(isp).dev,
						&exynos5_device_fimc_is.dev);
#endif
}
#else /* !CONFIG_EXYNOS_DEV_SYSMMU */
static inline void exynos_sysmmu_init(void)
{
}
#endif

static void p10_power_off(void)
{
	printk(KERN_EMERG "%s: set PS_HOLD low\n", __func__);

	writel(readl(EXYNOS5_PS_HOLD_CONTROL) & 0xFFFFFEFF, EXYNOS5_PS_HOLD_CONTROL);
	printk(KERN_EMERG "%s: Should not reach here\n", __func__);
}

#ifdef CONFIG_FB_S5P_EXTDSP
struct platform_device s3c_device_extdsp = {
	.name		= "s3cfb_extdsp",
	.id		= 0,
};

static struct s3cfb_extdsp_lcd dummy_buffer = {
	.width = 1920,
	.height = 1080,
	.bpp = 16,
};

static struct s3c_platform_fb default_extdsp_data __initdata = {
	.hw_ver		= 0x70,
	.nr_wins	= 1,
	.default_win	= 0,
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
	.lcd		= &dummy_buffer
};

void __init s3cfb_extdsp_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_extdsp_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;
		s3c_device_extdsp.dev.platform_data = npd;
	}
}
#endif

static void camera_init(void)
{
	camera_class = class_create(THIS_MODULE, "camera");

	if (IS_ERR(camera_class))
		pr_err("Failed to create class(camera)!\n");
}

static void __init p10_machine_init(void)
{
#ifdef CONFIG_S3C64XX_DEV_SPI
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &exynos_device_spi1.dev;
#endif
	pm_power_off = p10_power_off;

	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		exynos_dwmci_set_platdata(&exynos5_dwmci0_pdata, 0);
		dev_set_name(&exynos_device_dwmci0.dev, "s3c-sdhci.0");
		clk_add_alias("dwmci", "dw_mmc.0", "hsmmc",
			&exynos_device_dwmci0.dev);
		clk_add_alias("sclk_dwmci", "dw_mmc.0", "sclk_mmc",
			&exynos_device_dwmci0.dev);

		exynos_dwmci_set_platdata(&exynos5_dwmci1_pdata, 1);
		dev_set_name(&exynos_device_dwmci1.dev, "s3c-sdhci.1");
		clk_add_alias("dwmci", "dw_mmc.1", "hsmmc",
			&exynos_device_dwmci1.dev);
		clk_add_alias("sclk_dwmci", "dw_mmc.1", "sclk_mmc",
			&exynos_device_dwmci1.dev);

		exynos_dwmci_set_platdata(&exynos5_dwmci2_pdata, 2);
		dev_set_name(&exynos_device_dwmci2.dev, "s3c-sdhci.2");
		clk_add_alias("dwmci", "dw_mmc.2", "hsmmc",
			&exynos_device_dwmci2.dev);
		clk_add_alias("sclk_dwmci", "dw_mmc.2", "sclk_mmc",
			&exynos_device_dwmci2.dev);
	} else {
#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	exynos_dwmci_set_platdata(&exynos_dwmci_pdata, 0);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdk5250_hsmmc0_pdata);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdk5250_hsmmc1_pdata);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdk5250_hsmmc2_pdata);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
	s3c_sdhci3_set_platdata(&smdk5250_hsmmc3_pdata);
#endif
    }
#ifdef CONFIG_ION_EXYNOS
	exynos_ion_set_platdata();
#endif

#ifdef CONFIG_LEDS_SPFCW043
	platform_device_register(&s3c_device_spfcw043_led);
#endif

#ifdef CONFIG_FB_S3C
	dev_set_name(&s5p_device_fimd1.dev, "s3cfb.1");
	clk_add_alias("lcd", "exynos5-fb.1", "lcd", &s5p_device_fimd1.dev);
	clk_add_alias("sclk_fimd", "exynos5-fb.1", "sclk_fimd",
			&s5p_device_fimd1.dev);
	s5p_fb_setname(1, "exynos5-fb");

	s5p_fimd1_set_platdata(&p10_lcd1_pdata);
#endif

#ifdef CONFIG_FB_S5P_EXTDSP
	s3cfb_extdsp_set_platdata(&default_extdsp_data);
#endif

#ifdef CONFIG_BACKLIGHT_PWM
	samsung_bl_set(&p10_bl_gpio_info, &p10_bl_data);
#endif

#ifdef CONFIG_USB_EHCI_S5P
	smdk5250_ehci_init();
#endif

#ifdef CONFIG_USB_OHCI_S5P
	smdk5250_ohci_init();
#endif

#if defined(CONFIG_CPU_EXYNOS5250)
	set_usb3_en(1);
#endif

#ifdef CONFIG_USB_S3C_OTGD
	smdk5250_usbgadget_init();
#endif

#ifdef CONFIG_EXYNOS_DEV_SS_UDC
	smdk5250_ss_udc_init();
#endif

#ifdef CONFIG_USB_XHCI_EXYNOS
	smdk5250_xhci_init();
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#if defined(CONFIG_EXYNOS_DEV_PD)
	s5p_device_mfc.dev.parent = &exynos5_device_pd[PD_MFC].dev;
#endif
	s5p_mfc_set_platdata(&smdk5250_mfc_pd);

	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc-v6", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc-v6");
#endif

#ifdef CONFIG_FB_S3C
	s5p_device_fimd1.dev.parent = &exynos5_device_pd[PD_DISP1].dev;
#endif

#ifdef CONFIG_S5P_DP
	s5p_dp_set_platdata(&p10_dp_data);
#endif

#ifdef CONFIG_S3C_ADC
	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		platform_device_register(&s3c_device_adc);
#ifdef CONFIG_S3C_DEV_HWMON
		platform_device_register(&s3c_device_hwmon);
#endif
	}
#endif

#ifdef CONFIG_S3C_DEV_HWMON
	if (samsung_rev() >= EXYNOS5250_REV_1_0)
		s3c_hwmon_set_platdata(&smdk5250_hwmon_pdata);
#endif

#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

	exynos_sysmmu_init();

	p10_config_gpio_table();

	exynos5_sleep_gpio_table_set = p10_config_sleep_gpio_table;

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

#ifdef CONFIG_MPU_SENSORS_MPU6050
	pr_info("MPU6050 I2C-1 Init\n");
	//magnetic_init();
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif

	p10_tsp_init();
	p10_key_init();

	s3c_i2c5_set_platdata(NULL);
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));

	midas_sound_init();

#ifdef CONFIG_MPU_SENSORS_MPU6050
	magnetic_init();
	i2c_register_board_info(11, i2c_devs11, ARRAY_SIZE(i2c_devs11));
#endif

#ifdef CONFIG_SENSORS_BH1721FVC
	light_sensor_init();
	i2c_register_board_info(12, i2c_bh1721_emul, ARRAY_SIZE(i2c_bh1721_emul));
#endif

#ifdef CONFIG_SENSORS_SHT21
	i2c_register_board_info(13, i2c_devs13, ARRAY_SIZE(i2c_devs13));
#endif

#if defined(CONFIG_SAMSUNG_MHL)
	i2c_register_board_info(15, i2c_devs15_emul,
			ARRAY_SIZE(i2c_devs15_emul));
#endif

#ifdef CONFIG_MOTOR_DRV_ISA1200
	isa1200_init();
	i2c_register_board_info(17, i2c_devs17,
				ARRAY_SIZE(i2c_devs17));
#endif

#if defined(CONFIG_STMPE811_ADC)
	i2c_register_board_info(19, i2c_devs19_emul,
		ARRAY_SIZE(i2c_devs19_emul));
#endif
#if defined(CONFIG_BATTERY_SAMSUNG_P1X)
	p10_battery_init();
#endif

	platform_device_register(&vbatt_device);

	platform_add_devices(p10_devices, ARRAY_SIZE(p10_devices));

#ifdef CONFIG_FB_S3C
#if defined(CONFIG_MACH_P10_DP_01)
#if defined(CONFIG_DP_40HZ_P10)
	exynos4_fimd_setup_clock(&s5p_device_fimd1.dev, "sclk_fimd", "sclk_vpll",
			180 * MHZ);
#elif defined(CONFIG_DP_60HZ_P10)
	exynos4_fimd_setup_clock(&s5p_device_fimd1.dev, "sclk_fimd", "sclk_vpll",
		270 * MHZ);
#endif
#endif
#endif

#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
#if defined(CONFIG_EXYNOS_DEV_PD)
	s5p_device_mipi_csis0.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	s5p_device_mipi_csis1.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
#endif
	s3c_set_platdata(&s5p_mipi_csis0_default_data,
			sizeof(s5p_mipi_csis0_default_data), &s5p_device_mipi_csis0);
	s3c_set_platdata(&s5p_mipi_csis1_default_data,
			sizeof(s5p_mipi_csis1_default_data), &s5p_device_mipi_csis1);
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
#if defined(CONFIG_EXYNOS_DEV_PD)
	exynos_device_flite0.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos_device_flite1.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos_device_flite2.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
#endif
	smdk5250_camera_gpio_cfg();
	smdk5250_set_camera_platdata();
	s3c_set_platdata(&exynos_flite0_default_data,
			sizeof(exynos_flite0_default_data), &exynos_device_flite0);
	s3c_set_platdata(&exynos_flite1_default_data,
			sizeof(exynos_flite1_default_data), &exynos_device_flite1);
	s3c_set_platdata(&exynos_flite2_default_data,
			sizeof(exynos_flite2_default_data), &exynos_device_flite2);

/* In EVT0, for using camclk, gscaler clock should be enabled */
	if (samsung_rev() < EXYNOS5250_REV_1_0) {
		dev_set_name(&exynos_device_flite0.dev, "exynos-gsc.0");
		clk_add_alias("gscl", "exynos-fimc-lite.0", "gscl",
				&exynos_device_flite0.dev);
		dev_set_name(&exynos_device_flite0.dev, "exynos-fimc-lite.0");

		dev_set_name(&exynos_device_flite1.dev, "exynos-gsc.0");
		clk_add_alias("gscl", "exynos-fimc-lite.1", "gscl",
				&exynos_device_flite1.dev);
		dev_set_name(&exynos_device_flite1.dev, "exynos-fimc-lite.1");
	}
#endif

#if defined CONFIG_VIDEO_EXYNOS5_FIMC_IS
	dev_set_name(&exynos5_device_fimc_is.dev, "s5p-mipi-csis.0");
	clk_add_alias("gscl_wrap0", "exynos5-fimc-is", "gscl_wrap0", &exynos5_device_fimc_is.dev);
	clk_add_alias("sclk_gscl_wrap0", "exynos5-fimc-is", "sclk_gscl_wrap0", &exynos5_device_fimc_is.dev);
	dev_set_name(&exynos5_device_fimc_is.dev, "s5p-mipi-csis.1");
	clk_add_alias("gscl_wrap1", "exynos5-fimc-is", "gscl_wrap1", &exynos5_device_fimc_is.dev);
	clk_add_alias("sclk_gscl_wrap1", "exynos5-fimc-is", "sclk_gscl_wrap1", &exynos5_device_fimc_is.dev);
	dev_set_name(&exynos5_device_fimc_is.dev, "exynos-gsc.0");
	clk_add_alias("gscl", "exynos5-fimc-is", "gscl", &exynos5_device_fimc_is.dev);
	dev_set_name(&exynos5_device_fimc_is.dev, "exynos5-fimc-is");

#if defined CONFIG_VIDEO_S5K6A3
	exynos5_fimc_is_data.sensor_info[s5k6a3.sensor_position] = &s5k6a3;
	printk("add s5k6a3 sensor info(pos : %d)\n", s5k6a3.sensor_position);
#endif
#if defined CONFIG_VIDEO_S5K4E5
	exynos5_fimc_is_data.sensor_info[s5k4e5.sensor_position] = &s5k4e5;
	printk("add s5k4e5 sensor info(pos : %d)\n", s5k4e5.sensor_position);
#endif

	exynos5_fimc_is_set_platdata(&exynos5_fimc_is_data);
#if defined(CONFIG_EXYNOS_DEV_PD)
	exynos5_device_pd[PD_ISP].dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos5_device_fimc_is.dev.parent = &exynos5_device_pd[PD_ISP].dev;
#endif
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&exynos_tmu_data);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_GSCALER
#if defined(CONFIG_EXYNOS_DEV_PD)
	exynos5_device_gsc0.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos5_device_gsc1.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos5_device_gsc2.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
	exynos5_device_gsc3.dev.parent = &exynos5_device_pd[PD_GSCL].dev;
#endif

	if (samsung_rev() >= EXYNOS5250_REV_1_0) {
		exynos5_gsc_set_pdev_name(0, "exynos5250-gsc");
		exynos5_gsc_set_pdev_name(1, "exynos5250-gsc");
		exynos5_gsc_set_pdev_name(2, "exynos5250-gsc");
		exynos5_gsc_set_pdev_name(3, "exynos5250-gsc");
	}

	s3c_set_platdata(&exynos_gsc0_default_data, sizeof(exynos_gsc0_default_data),
			&exynos5_device_gsc0);
	s3c_set_platdata(&exynos_gsc1_default_data, sizeof(exynos_gsc1_default_data),
			&exynos5_device_gsc1);
	s3c_set_platdata(&exynos_gsc2_default_data, sizeof(exynos_gsc2_default_data),
			&exynos5_device_gsc2);
	s3c_set_platdata(&exynos_gsc3_default_data, sizeof(exynos_gsc3_default_data),
			&exynos5_device_gsc3);
	/* Gscaler can use MPLL(266MHz) or VPLL(300MHz).
	In case of P10, Gscaler should use MPLL(266MHz) because FIMD uses VPLL(86MHz).
	So mout_aclk_300_gscl_mid selects mout_mpll_user and then
	mout_aclk_300_gscl_mid is set to 267MHz
	even though the clock name(dout_aclk_300_gscl) implies and requires around 300MHz
	*/
	exynos5_gsc_set_parent_clock("mout_aclk_300_gscl_mid", "mout_mpll_user");
	exynos5_gsc_set_parent_clock("mout_aclk_300_gscl", "mout_aclk_300_gscl_mid");
	exynos5_gsc_set_parent_clock("aclk_300_gscl", "dout_aclk_300_gscl");
	exynos5_gsc_set_clock_rate("dout_aclk_300_gscl", 267000000);
#endif

#ifdef CONFIG_EXYNOS_C2C
	exynos_c2c_set_platdata(&smdk5250_c2c_pdata);
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
	exynos5_jpeg_setup_clock(&s5p_device_jpeg.dev, 150000000);
#endif

#if defined(CONFIG_VIDEO_EXYNOS_TV) && defined(CONFIG_VIDEO_EXYNOS_HDMI)
	dev_set_name(&s5p_device_hdmi.dev, "exynos5-hdmi");
	clk_add_alias("hdmi", "s5p-hdmi", "hdmi", &s5p_device_hdmi.dev);
	clk_add_alias("hdmiphy", "s5p-hdmi", "hdmiphy", &s5p_device_hdmi.dev);

	s5p_tv_setup();

/* setup dependencies between TV devices */
	/* This will be added after power domain for exynos5 is developed */
	s5p_device_hdmi.dev.parent = &exynos5_device_pd[PD_DISP1].dev;
	s5p_device_mixer.dev.parent = &exynos5_device_pd[PD_DISP1].dev;

	s5p_i2c_hdmiphy_set_platdata(NULL);
#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#endif
#endif

#ifdef CONFIG_S3C64XX_DEV_SPI
	sclk = clk_get(spi1_dev, "sclk_spi1");
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

	if (!gpio_request(GPIO_5M_SPI_CS, "SPI_CS1")) {
		gpio_direction_output(GPIO_5M_SPI_CS, 1);
		s3c_gpio_cfgpin(GPIO_5M_SPI_CS, S3C_GPIO_SFN(1));
		s3c_gpio_setpull(GPIO_5M_SPI_CS, S3C_GPIO_PULL_UP);
		exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi1_csi));
	}

	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
#endif

#ifdef CONFIG_BUSFREQ_OPP
	dev_add(&busfreq, &exynos5_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos5_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DDR_C], &exynos5_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DDR_R1], &exynos5_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DDR_L], &exynos5_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_RIGHT0_BUS], &exynos5_busfreq.dev);
#endif
#ifdef CONFIG_30PIN_CONN
	acc_con_gpio_init();
#endif

	/* for BRCM Wi-Fi */
	brcm_wlan_init();

	/* for camera*/
	camera_init();

	register_reboot_notifier(&exynos5_reboot_notifier);
}

#ifdef CONFIG_EXYNOS_C2C
static void __init exynos_c2c_reserve(void)
{
	static struct cma_region regions[] = {
		{
			.name = "c2c_shdmem",
			.size = 64 * SZ_1M,
			{ .alignment	= 64 * SZ_1M },
			.start = C2C_SHAREDMEM_BASE
		}, {
			.size = 0,
		}
	};

	s5p_cma_region_reserve(regions, NULL, 0, map);
}
#endif

#if defined(CONFIG_SEC_DEBUG)
static void __init exynos_init_reserve(void)
{
	sec_debug_magic_init();
}
#endif

MACHINE_START(SMDK5250, "SMDK5250")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos5_init_irq,
	.map_io		= p10_map_io,
	.init_machine	= p10_machine_init,
	.timer		= &exynos4_timer,
#ifdef CONFIG_EXYNOS_C2C
	.reserve	= &exynos_c2c_reserve,
#endif
#if defined(CONFIG_SEC_DEBUG)
	.init_early	= &exynos_init_reserve,
#endif
MACHINE_END
