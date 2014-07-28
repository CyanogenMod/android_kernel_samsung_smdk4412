/*
 * board-lcd-evf.c - lcd driver of GD2 Project
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
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <plat/devs.h>
#include <plat/fb-s5p.h>
#include <plat/gpio-cfg.h>
#include <plat/pd.h>
#include <plat/map-base.h>
#include <plat/map-s5p.h>

#ifdef CONFIG_S3C64XX_DEV_SPI
#include <plat/s3c64xx-spi.h>
#endif

static struct s3c_platform_fb fb_platform_data;
struct s3c_platform_fb *evf_fb_platdata_addr;
struct s3c_platform_fb *mipi_fb_platdata_addr;

#ifdef CONFIG_FB_S5P
#if defined(CONFIG_FB_S5P_GD2EVF)
static int lcd_power_on(struct lcd_device *ld, int enable)
{
	struct regulator *regulator;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		return 0;
	}

	if (enable) {
		regulator = regulator_get(NULL, "evf_1.8v");
		if (IS_ERR(regulator))
			return 0;

		regulator_enable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "evf_3.3v");
		if (IS_ERR(regulator))
			return 0;

		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "evf_3.3v");

		if (IS_ERR(regulator))
			return 0;

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);

		regulator_put(regulator);

		regulator = regulator_get(NULL, "evf_1.8v");

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
	int reset_gpio;
	int err;

	reset_gpio = GPIO_EVF_RESET;

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
		       "lcd reset control\n");
		return err;
	}

	s3c_gpio_cfgpin(reset_gpio, S3C_GPIO_OUTPUT);
	gpio_set_value(reset_gpio, GPIO_LEVEL_LOW);
	mdelay(10);
	gpio_set_value(reset_gpio, GPIO_LEVEL_HIGH);

	gpio_free(reset_gpio);

	return 1;
}

static void lcd_gpio_setup(unsigned int start, unsigned int size,
		unsigned int cfg, s5p_gpio_drvstr_t drvstr)
{
	s3c_gpio_cfgrange_nopull(start, size, cfg);

	for (; size > 0; size--, start++)
		s5p_gpio_set_drvstr(start, drvstr);
}

static void lcd_gpio_cfg(int onoff)
{
	if (onoff) {
		lcd_gpio_setup(EXYNOS4_GPF0(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV2);
		lcd_gpio_setup(EXYNOS4_GPF1(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV2);
		lcd_gpio_setup(EXYNOS4_GPF2(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV2);
		lcd_gpio_setup(EXYNOS4_GPF3(0), 4, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV2);
	}
	else {
		s3c_gpio_cfgall_range(EXYNOS4_GPF0(0), 8, S3C_GPIO_INPUT, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgall_range(EXYNOS4_GPF1(0), 8, S3C_GPIO_INPUT, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgall_range(EXYNOS4_GPF2(0), 8, S3C_GPIO_INPUT, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgall_range(EXYNOS4_GPF3(0), 4, S3C_GPIO_INPUT, S3C_GPIO_PULL_DOWN);
	}
}
static int lcd_gpio_cfg_earlysuspend(struct lcd_device *ld)
{
	/* MLCD_RST */
	s3c_gpio_cfgpin(GPIO_EVF_RESET, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_EVF_RESET, S3C_GPIO_PULL_DOWN);
	/* LCD_nCS */
	s3c_gpio_cfgpin(GPIO_LCD_nCS, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_LCD_nCS, S3C_GPIO_PULL_DOWN);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(GPIO_LCD_SCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_LCD_SCLK, S3C_GPIO_PULL_DOWN);
	/* LCD_SDI */
	s3c_gpio_cfgpin(GPIO_LCD_SDI, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_LCD_SDI, S3C_GPIO_PULL_DOWN);

	lcd_gpio_cfg(0);

	return 0;
}

static int lcd_gpio_cfg_lateresume(struct lcd_device *ld)
{
	lcd_gpio_cfg(1);

	/* MLCD_RST */
	s3c_gpio_cfgpin(GPIO_EVF_RESET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_EVF_RESET, S3C_GPIO_PULL_NONE);
	/* LCD_nCS */
	s3c_gpio_cfgpin(GPIO_LCD_nCS, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_nCS, S3C_GPIO_PULL_NONE);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(GPIO_LCD_SCLK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_SCLK, S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	s3c_gpio_cfgpin(GPIO_LCD_SDI, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_SDI, S3C_GPIO_PULL_NONE);

	return 0;
}

static struct s3cfb_lcd panel_data = {
	.width = 800,
	.height = 600,
	.p_width = 56,
	.p_height = 93,
	.bpp = 24,

	.freq = 60,
	.timing = {
		.h_fp = 33,
		.h_bp = 92,
		.h_sw = 64,
		.v_fp = 42,
		.v_fpe = 1,
		.v_bp = 11,
		.v_bpe = 1,
		.v_sw = 12,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

static struct lcd_platform_data gd2evf_platform_data = {
	.reset = reset_lcd,
	.power_on = lcd_power_on,
	.gpio_cfg_earlysuspend = lcd_gpio_cfg_earlysuspend,
	.gpio_cfg_lateresume = lcd_gpio_cfg_lateresume,
	/* it indicates whether lcd panel is enabled from u-boot. */
	.lcd_enabled = 0,
	.reset_delay = 180,	/* 180ms */
	.power_on_delay = 2,	/* 2ms */
	.power_off_delay = 2,	/* 2ms */
	.pdata = NULL,
};

#define LCD_BUS_NUM	3
static struct spi_board_info spi3_board_info[] __initdata = {
	{
		.modalias = "gd2evf",
		.platform_data = &gd2evf_platform_data,
		.max_speed_hz = 1200000,
		.bus_num = LCD_BUS_NUM,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = (void *)GPIO_LCD_nCS,
	},
};

static struct spi_gpio_platform_data lcd_spi_gpio_data = {
	.sck = GPIO_LCD_SCLK,
	.mosi = GPIO_LCD_SDI,
	.miso = SPI_GPIO_NO_MISO,
	.num_chipselect = 1,
};

struct platform_device gd2evf_spi_gpio = {
	.name = "spi_gpio",
	.id = LCD_BUS_NUM,
	.dev = {
		.parent = &s3c_device_fb.dev,
		.platform_data = &lcd_spi_gpio_data,
	},
};

void __init gd2evf_fb_init(void)
{
	spi_register_board_info(spi3_board_info, ARRAY_SIZE(spi3_board_info));

	s3cfb_set_platdata(&fb_platform_data);
	evf_fb_platdata_addr = s3c_device_fb.dev.platform_data;
}
#endif

static struct s3c_platform_fb fb_platform_data __initdata = {
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
