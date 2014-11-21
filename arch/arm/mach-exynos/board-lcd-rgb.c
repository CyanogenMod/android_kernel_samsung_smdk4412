/*
 * board-lcd-rgb.c - lcd driver of MIDAS Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/lcd.h>

#include <plat/devs.h>
#include <plat/fb-s5p.h>
#include <plat/gpio-cfg.h>
#include <plat/pd.h>
#include <plat/map-base.h>
#include <plat/map-s5p.h>

#ifdef CONFIG_FB_S5P_LD9040
#include <linux/ld9040.h>
#endif

#ifdef CONFIG_FB_S5P_MDNIE
#include <linux/mdnie.h>
#endif

struct s3c_platform_fb fb_platform_data;
static struct platform_mdnie_data mdnie_data;

unsigned int lcdtype;
static int __init lcdtype_setup(char *str)
{
	get_option(&str, &lcdtype);
#if defined(CONFIG_FB_S5P_S6C1372)
	mdnie_data.display_type = lcdtype;
#endif
	return 1;
}
__setup("lcdtype=", lcdtype_setup);


#ifdef CONFIG_FB_S5P
#ifdef CONFIG_FB_S5P_LD9040
static int lcd_cfg_gpio(void)
{
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data line (32bit) */
		s3c_gpio_cfgpin(EXYNOS4_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF0(i), S3C_GPIO_PULL_NONE);
	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* MLCD_RST */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(3), S3C_GPIO_PULL_NONE);

	/* LCD_SCLK */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(1), S3C_GPIO_PULL_NONE);

	/* LCD_SDI */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static int lcd_power_on(struct lcd_device *ld, int enable)
{
	struct regulator *regulator;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		return 0;
	}

	if (enable) {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;

		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vlcd_3.0v");

	if (IS_ERR(regulator))
		return 0;

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);

		regulator_put(regulator);
	}

	return 1;
}

static int reset_lcd(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = EXYNOS4_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
			"lcd reset control\n");
		return err;
	}

	gpio_request(reset_gpio, "MLCD_RST");

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);
	mdelay(10);
	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);

	return 1;
}

static int lcd_gpio_cfg_earlysuspend(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = EXYNOS4_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
			"lcd reset control\n");
		return err;
	}

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);

	gpio_free(reset_gpio);

	return 0;
}

static int lcd_gpio_cfg_lateresume(struct lcd_device *ld)
{
	/* MLCD_RST */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(3), S3C_GPIO_PULL_NONE);

	/* LCD_SCLK */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(1), S3C_GPIO_PULL_NONE);

	/* LCD_SDI */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static struct s3cfb_lcd panel_data = {
	.width = 480,
	.height = 800,
	.p_width = 56,
	.p_height = 93,
	.bpp = 24,

	.freq = 60,
	.timing = {
		.h_fp = 16,
		.h_bp = 14,
		.h_sw = 2,
		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 4,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

struct ld9040_panel_data s2plus_panel_data;
static struct lcd_platform_data ld9040_platform_data = {
	.reset = reset_lcd,
	.power_on = lcd_power_on,
	.gpio_cfg_earlysuspend = lcd_gpio_cfg_earlysuspend,
	.gpio_cfg_lateresume = lcd_gpio_cfg_lateresume,
		 /* it indicates whether lcd panel is enabled from u-boot. */
	.lcd_enabled = 0,
	.reset_delay = 20,  /* 20ms */
	.power_on_delay = 20,	 /* 20ms */
	.power_off_delay = 200, /* 200ms */
	.sleep_in_delay = 160,
	.pdata = &s2plus_panel_data,
};

#define LCD_BUS_NUM	3
#define DISPLAY_CS	EXYNOS4_GPY4(3)
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.max_speed_hz = 1200000,
		.bus_num = LCD_BUS_NUM,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#define DISPLAY_CLK	EXYNOS4_GPY3(1)
#define DISPLAY_SI	EXYNOS4_GPY3(3)
static struct spi_gpio_platform_data lcd_spi_gpio_data = {
	.sck = DISPLAY_CLK,
	.mosi = DISPLAY_SI,
	.miso = SPI_GPIO_NO_MISO,
	.num_chipselect = 1,
};

static struct platform_device ld9040_spi_gpio = {
	.name = "spi_gpio",
	.id = LCD_BUS_NUM,
	.dev = {
		.parent = &s3c_device_fb.dev,
		.platform_data = &lcd_spi_gpio_data,
	},
};

/* reading with 3-WIRE SPI with GPIO */
static inline void setcs(u8 is_on)
{
	gpio_set_value(DISPLAY_CS, is_on);
}

static inline void setsck(u8 is_on)
{
	gpio_set_value(DISPLAY_CLK, is_on);
}

static inline void setmosi(u8 is_on)
{
	gpio_set_value(DISPLAY_SI, is_on);
}

static inline unsigned int getmiso(void)
{
	return !!gpio_get_value(DISPLAY_SI);
}

static inline void setmosi2miso(u8 is_on)
{
	if (is_on)
		s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_INPUT);
	else
		s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_OUTPUT);
}

struct spi_ops ops = {
	.setcs = setcs,
	.setsck = setsck,
	.setmosi = setmosi,
	.setmosi2miso = setmosi2miso,
	.getmiso = getmiso,
};

void __init ld9040_fb_init(void)
{
	struct ld9040_panel_data *pdata;

	strcpy(spi_board_info[0].modalias, "ld9040");
	spi_board_info[0].platform_data = (void *)&ld9040_platform_data;

	pdata = ld9040_platform_data.pdata;
	pdata->ops = &ops;

	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));

	if (!ld9040_platform_data.lcd_enabled)
		lcd_cfg_gpio();
	/*s3cfb_set_platdata(&fb_platform_data);*/
}
#endif

#if defined(CONFIG_FB_S5P_S6C1372)
int s6c1372_panel_gpio_init(void)
{
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data line (32bit) */
		s3c_gpio_cfgpin(EXYNOS4_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	return 0;
}

static struct s3cfb_lcd panel_data = {
	.width = 1280,
	.height = 800,
	.p_width = 217,
	.p_height = 135,
	.bpp = 24,
#if defined(CONFIG_MACH_P4NOTELTE_USA_SPR) || \
	defined(CONFIG_MACH_P4NOTELTE_USA_VZW) || \
	defined(CONFIG_MACH_P4NOTELTE_USA_USCC)
	.freq = 55,
#else
	.freq = 60,
#endif
	.timing = {
		.h_fp = 18,
		.h_bp = 36,
		.h_sw = 16,
		.v_fp = 4,
		.v_fpe = 1,
		.v_bp = 16,
		.v_bpe = 1,
		.v_sw = 3,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

static int lcd_power_on(struct lcd_device *ld, int enable)
{
	if (enable) {
		/* s5c1372_ldi_enable */
		gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_HIGH);
		msleep(40);

		/* LVDS_N_SHDN to high*/
		mdelay(1);
		gpio_set_value(GPIO_LVDS_NSHDN, GPIO_LEVEL_HIGH);
		msleep(300);

		gpio_set_value(GPIO_LED_BACKLIGHT_RESET, GPIO_LEVEL_HIGH);
		mdelay(2);
	} else {
		gpio_set_value(GPIO_LED_BACKLIGHT_RESET, GPIO_LEVEL_LOW);
		msleep(200);

		/* LVDS_nSHDN low*/
		gpio_set_value(GPIO_LVDS_NSHDN, GPIO_LEVEL_LOW);
		msleep(40);

		/* s5c1372_ldi_disable */
		s3c_gpio_cfgpin(GPIO_LCD_PCLK, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_LCD_PCLK, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_LCD_PCLK, GPIO_LEVEL_LOW);

		msleep(40);
		gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_LOW);

		msleep(600);
	}

	return 0;
}

static struct lcd_platform_data panel_platform_data = {
	.power_on	= lcd_power_on,
#if defined(CONFIG_MACH_TAB3)
	.pdata		= "SMD_LTL101AL06"
#else
	.pdata		= "SEC_LTL101AL01-002/003"
#endif
};

struct platform_device lcd_s6c1372 = {
	.name   = "lvds_lcd",
	.id	= -1,
	.dev.platform_data = &panel_platform_data,
};

#endif

struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "fimd",
	.nr_wins	= 5,
#ifdef CONFIG_FB_S5P_DEFAULT_WINDOW
	.default_win	= CONFIG_FB_S5P_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
	.lcd		= &panel_data
};
#endif

#ifdef CONFIG_FB_S5P_MDNIE
static struct platform_mdnie_data mdnie_data = {
	.display_type	= 0,
#if defined(CONFIG_FB_S5P_S6C1372)
	.lcd_pd		= &panel_platform_data,
#endif
};

struct platform_device mdnie_device = {
		.name		 = "mdnie",
		.id	 = -1,
		.dev		 = {
			.parent = &exynos4_device_pd[PD_LCD0].dev,
			.platform_data = &mdnie_data,
	},
};
#endif

