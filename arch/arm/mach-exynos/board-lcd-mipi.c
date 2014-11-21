/*
 * board-lcd-mipi.c - lcd driver of MIDAS Project
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
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/lcd.h>

#include <plat/devs.h>
#include <plat/fb-s5p.h>
#include <plat/gpio-cfg.h>
#include <plat/pd.h>
#include <plat/map-base.h>
#include <plat/map-s5p.h>

#ifdef CONFIG_FB_S5P_MIPI_DSIM
#include <mach/mipi_ddi.h>
#include <mach/dsim.h>
#endif

#ifdef CONFIG_FB_S5P_MDNIE
#include <linux/mdnie.h>
#endif

#ifdef CONFIG_BACKLIGHT_LP855X
#include <linux/platform_data/lp855x.h>
#endif

#ifdef GPIO_LCD_22V_EN_00
 #define GPIO_LCD_POWER_EN GPIO_LCD_22V_EN_00
#elif defined(GPIO_LCD_EN)
 #define GPIO_LCD_POWER_EN GPIO_LCD_EN
#endif

#ifdef CONFIG_FB_S5P_S6E63M0
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_2
 /* 320Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	80
 #define DPHY_PLL_S	1
#elif defined(CONFIG_FB_S5P_LMS501XX)
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_2
 /* 428Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	107
 #define DPHY_PLL_S	1
#elif defined(CONFIG_MACH_GC1_USA_VZW)
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_4
 /* 480Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	120
 #define DPHY_PLL_S	1
#elif defined(CONFIG_FB_S5P_S6E88A0)
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_2
 /* 489Mbps */
 #define DPHY_PLL_P	4
 #define DPHY_PLL_M	163
 #define DPHY_PLL_S	1
#elif defined(CONFIG_FB_S5P_HX8369B)
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_2
 /* 492Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	123
 #define DPHY_PLL_S	1
#elif defined(CONFIG_FB_S5P_NT71391)
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_4
 /* 230Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	115
 #define DPHY_PLL_S	1
#else
 #define DSIM_NO_DATA_LANE	DSIM_DATA_LANE_4
 /* 500Mbps */
 #define DPHY_PLL_P	3
 #define DPHY_PLL_M	125
 #define DPHY_PLL_S	1
#endif

static const char * const lcd_regulator_arr[] = {
#if defined(CONFIG_FB_S5P_LMS501XX)
	"vlcd_3.3v",
#elif defined(CONFIG_S6E8AA0_AMS465XX)
	"vlcd_3.1v",
#elif defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_IPCAM)
	"vcc_1.8v_lcd",
	"vlcd_3.3v",
#elif defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
	"lcd_io_1.8v",
#elif defined(CONFIG_MACH_GD2)
	"vcc_1.8v_lcd"
#elif defined(CONFIG_MACH_ZEST)
	"vcc_1.8v_lcd",
	"vcc_3.0v_lcd",
#elif defined(CONFIG_FB_S5P_NT71391) || defined(CONFIG_FB_S5P_S6D7AA0)
	/* Regulator empty*/
#elif defined(CONFIG_MACH_WATCH)
	"vcc_3.3v_lcd",
#else
	"vlcd_3.3v",
#endif
};

#ifdef CONFIG_BACKLIGHT_LP855X
static int lcd_bl_init(void);
#endif

#ifdef CONFIG_FB_S5P_MDNIE
static struct platform_mdnie_data mdnie_data;
#endif
struct s3c_platform_fb fb_platform_data;
unsigned int lcdtype;
static int __init lcdtype_setup(char *str)
{
	get_option(&str, &lcdtype);
#ifdef CONFIG_FB_S5P_S6D7AA0
	mdnie_data.display_type = lcdtype & 0xff;
#endif

#if defined(CONFIG_MACH_KONA)
/*
	* LP8556 ISET electric resistance  Change
	* KONA 3G, WIFI: gpio >= 4
	* KONA LTE : gpio >=3
	* lcdtype : 1 => BOE
	* lcdtype : 2 => SDC
	*/
	if (lcdtype == 1) { /* BOE LCD */
		mdnie_data.display_type = 0;
#if defined(CONFIG_MACH_KONA_EUR_LTE)
		if (system_rev >= 3)
			mdnie_data.display_type = 1;
#else
		if (system_rev >= 4)
			mdnie_data.display_type = 1;
#endif
	} else { /* SDC LCD */
		mdnie_data.display_type = 2;
#if defined(CONFIG_MACH_KONA_EUR_LTE)
		if (system_rev >= 3)
			mdnie_data.display_type = 3;
#else
		if (system_rev >= 4)
			mdnie_data.display_type = 3;
#endif
	}
#endif

	return 1;
}
__setup("lcdtype=", lcdtype_setup);


#ifdef CONFIG_FB_S5P_S6E8AA0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6e8aa0",
	.height = 1280,
#if defined(CONFIG_S6E8AA0_AMS529HA01)
	.height = 1280,
	.width = 800,
	.p_width = 64,
	.p_height = 106,
#elif 0 /*defined(CONFIG_MACH_WATCH)*/
	.height = 320,
	.width = 320,
	.p_width = 29,		/* 29.28 mm */
	.p_height = 29,
#else
	.height = 1280,
	.width = 720,
	.p_width = 60,		/* 59.76 mm */
	.p_height = 106,
#endif
	.bpp = 24,

	.freq = 60,
#if defined(CONFIG_S6E8AA0_AMS480GYXX) || defined(CONFIG_S6E8AA0_AMS465XX)
#if defined(CONFIG_MACH_M3_JPN_DCM)
	.freq_limit = 43,
#else
	.freq_limit = 40,
#endif
#endif

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},
#if defined(CONFIG_MACH_M3_JPN_DCM)
	.timing = {
		.h_fp = 15,
		.h_bp = 10,
		.h_sw = 10,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,
		.stable_vfp = 2,
	},
#else
	.timing = {
		.h_fp = 5,
		.h_bp = 5,
		.h_sw = 5,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,
		.stable_vfp = 2,
	},
#endif
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_EA8061
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "ea8061",
	.height = 1280,
	.width = 720,
	.p_width = 69,
	.p_height = 123,
	.bpp = 24,
	.freq = 58,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 52,
		.h_bp = 121,
		.h_sw = 4,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6EVR02
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6evr02",
	.height = 1280,
	.width = 720,
	.p_width = 69,
	.p_height = 123,
	.bpp = 24,
	.freq = 58,
	.freq_limit = 41,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 70,
		.h_bp = 40,
		.h_sw = 3,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 7,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

static struct s3cfb_lcd ea8061 = {
	.name = "ea8061",
	.height = 1280,
	.width = 720,
	.p_width = 69,
	.p_height = 123,
	.bpp = 24,
	.freq = 58,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 52,
		.h_bp = 121,
		.h_sw = 4,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 7,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6E63M0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6e63m0",
	.width = 480,
	.height = 800,
	.p_width = 60,
	.p_height = 106,
	.bpp = 24,

	.freq = 59,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 16,
		.h_bp = 14,
		.h_sw = 2,
		.v_fp = 28,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6E39A0
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6e8aa0",
	.width = 540,
	.height = 960,
	.p_width = 58,
	.p_height = 103,
	.bpp = 24,

	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 0x48,
		.h_bp = 12,
		.h_sw = 4,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 0x4,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6D6AA1
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6d6aa1",
	.width = 720,
	.height = 1280,
	.p_width = 63,		/* 63.2 mm */
	.p_height = 114,	/* 114.19 mm */
	.bpp = 24,

	.freq = 60,
	.freq_limit = 51,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 60,
		.h_bp = 60,
		.h_sw = 3,
		.v_fp = 36,
		.v_fpe = 1,
		.v_bp = 2,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_LMS501XX
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "lms501xx",
	.width = 480,
	.height = 800,
	.p_width = 66,
	.p_height = 110,
	.bpp = 24,
	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},
	.timing = {
		.h_fp = 65,
		.h_bp = 49,
		.h_sw = 17,
		.v_fp = 8,
		.v_fpe = 1,
		.v_bp = 12,
		.v_bpe = 1,
		.v_sw = 4,
		.cmd_allow_len = 7,
		.stable_vfp = 1,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_NT71391
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "nt71391",
	.width = 1280,
	.height = 800,
	.p_width = 172,
	.p_height = 108,
	.bpp = 24,
	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 25,
		.h_bp = 25,
		.h_sw = 41,
		.v_fp = 8, /* spec = 3 */
		.v_fpe = 1,
		.v_bp = 3,
		.v_bpe = 1,
		.v_sw = 6,
		.cmd_allow_len = 7,
		.stable_vfp = 1,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6D7AA0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6d7aa0",
	.width = 800,
	.height = 1280,
#if defined(CONFIG_S6D7AA0_LSL080AL02)
	.p_width = 108,
	.p_height = 173,
#else
	.p_width = 95,
	.p_height = 152,
#endif
	.bpp = 24,
	.freq = 58,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 16,
		.h_bp = 140,
		.h_sw = 4,
		.v_fp = 8,
		.v_fpe = 1,
		.v_bp = 4,
		.v_bpe = 1,
		.v_sw = 4,
		.cmd_allow_len = 7,
		.stable_vfp = 1,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6E88A0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "s6e88a0",
	.width = 540,
	.height = 960,
	.p_width = 53,
	.p_height = 95,
	.bpp = 24,
	.freq = 60,
	.freq_limit = 40,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 24,
		.h_bp = 3,
		.h_sw = 3,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 2,
		.v_bpe = 1,
		.v_sw = 1,
		.cmd_allow_len = 7,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_HX8369B
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd lcd_panel_pdata = {
	.name = "hx8369b",
	.width = 480,
	.height = 800,
	.p_width = 56,
	.p_height = 94,
	.bpp = 24,
	.freq = 59,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 150,
		.h_bp = 135,
		.h_sw = 32,
		.v_fp = 20,
		.v_fpe = 1,
		.v_bp = 22,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 7,
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

static int reset_lcd(void)
{
#if defined(GPIO_MLCD_RST)
	gpio_direction_output(GPIO_MLCD_RST, 1);
	usleep_range(5000, 5000);
	gpio_set_value(GPIO_MLCD_RST, 0);
	usleep_range(5000, 5000);
	gpio_set_value(GPIO_MLCD_RST, 1);
	usleep_range(5000, 5000);
#endif
	return 0;
}

static int lcd_cfg_gpio(void)
{
	int err;

#if defined(GPIO_LCD_POWER_EN)
	/* LCD_EN */
	err = gpio_request(GPIO_LCD_POWER_EN, "LCD_EN");
	if (err) {
		pr_err("failed to request LCD_EN gpio\n");
		return -EPERM;
	}

	s3c_gpio_cfgpin(GPIO_LCD_POWER_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_POWER_EN, S3C_GPIO_PULL_NONE);
#endif

#if defined(GPIO_MLCD_RST)
	/* MLCD_RST */
	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		pr_err("failed to request MLCD_RST gpio\n");
		return -EPERM;
	}

	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);
#endif

#if defined(CONFIG_FB_S5P_LMS501XX)
	err = gpio_request(GPIO_LCD_BL_EN, "LCD_BL_EN");
	if (err) {
		pr_err("failed to request LCD_BL_EN gpio\n");
		return -EPERM;
	}
#endif

	return 0;
}

static int lcd_power_on(void *ld, int enable)
{
	struct regulator *regulator;
	int i;

	pr_info("%s: enable=%d\n", __func__, enable);

	if (enable) {
#if defined(GPIO_LCD_POWER_EN)
		gpio_set_value(GPIO_LCD_POWER_EN, GPIO_LEVEL_HIGH);
#endif
#if defined(CONFIG_FB_S5P_LMS501XX)
		msleep(25);
#endif
		for (i = 0; i < ARRAY_SIZE(lcd_regulator_arr); i++) {
			pr_debug("%s: regulator name : %s, enable : %d\n",
				 __func__, lcd_regulator_arr[i], enable);
			regulator = regulator_get(NULL, lcd_regulator_arr[i]);
			if (IS_ERR(regulator))
				goto out;
			regulator_enable(regulator);
			regulator_put(regulator);
		}
	} else {
#if defined(CONFIG_MACH_SF2)
#if defined(GPIO_MLCD_RST)
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_LOW);
#endif
#endif
		for (i = ARRAY_SIZE(lcd_regulator_arr)-1; i >= 0; i--) {
			pr_debug("%s: regulator name : %s, enable : %d\n",
				 __func__, lcd_regulator_arr[i], enable);
			regulator = regulator_get(NULL, lcd_regulator_arr[i]);
			if (IS_ERR(regulator))
				goto out;
			if (regulator_is_enabled(regulator))
				regulator_disable(regulator);
			regulator_put(regulator);
		}
#if defined(GPIO_LCD_POWER_EN)
		gpio_set_value(GPIO_LCD_POWER_EN, GPIO_LEVEL_LOW);
#endif
#if !defined(CONFIG_MACH_SF2)
#if defined(GPIO_MLCD_RST)
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_LOW);
#endif
#endif
	}
#if defined(CONFIG_FB_S5P_LMS501XX)
	gpio_set_value(GPIO_LCD_BL_EN, enable);
#endif

out:
	return 0;
}

static void s5p_dsim_mipi_power_control(int enable)
{
	struct regulator *regulator;

	pr_info("%s: enable=%d\n", __func__, enable);

	if (enable) {
		regulator = regulator_get(NULL, "vmipi_1.0v");
		if (IS_ERR(regulator))
			goto out;
		regulator_enable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "vmipi_1.8v");
		if (IS_ERR(regulator))
			goto out;
		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vmipi_1.8v");
		if (IS_ERR(regulator))
			goto out;
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "vmipi_1.0v");
		if (IS_ERR(regulator))
			goto out;
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
		regulator_put(regulator);
	}
out:
	return;
}

void __init mipi_fb_init(void)
{
	struct s5p_platform_dsim *dsim_pd = NULL;
	struct mipi_ddi_platform_data *mipi_ddi_pd = NULL;
	struct dsim_lcd_config *dsim_lcd_info = NULL;

	/* gpio pad configuration */
	lcd_cfg_gpio();

	/* set platform data */
	dsim_pd = s5p_device_dsim.dev.platform_data;

	dsim_pd->platform_rev = 1;
	dsim_pd->mipi_power = s5p_dsim_mipi_power_control;

	dsim_lcd_info = dsim_pd->dsim_lcd_info;

	dsim_lcd_info->lcd_panel_info = (void *)&lcd_panel_pdata;

#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_IPCAM)
	if (!gpio_get_value(GPIO_OLED_ID))	/* for EA8061 DDI */
		memcpy(&lcd_panel_pdata, &ea8061, sizeof(struct s3cfb_lcd));
#endif

	dsim_pd->dsim_info->e_no_data_lane = DSIM_NO_DATA_LANE;
	dsim_pd->dsim_info->p = DPHY_PLL_P;
	dsim_pd->dsim_info->m = DPHY_PLL_M;
	dsim_pd->dsim_info->s = DPHY_PLL_S;

	mipi_ddi_pd = dsim_lcd_info->mipi_ddi_pd;
	mipi_ddi_pd->lcd_reset = reset_lcd;
	mipi_ddi_pd->lcd_power_on = lcd_power_on;

	platform_device_register(&s5p_device_dsim);

#ifdef CONFIG_BACKLIGHT_LP855X
	lcd_bl_init();
#endif
}

struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "fimd",
	.nr_wins	= 5,
#ifdef CONFIG_FB_S5P_DEFAULT_WINDOW
	.default_win	= CONFIG_FB_S5P_DEFAULT_WINDOW,
#endif
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
	.lcd		= &lcd_panel_pdata
};


#ifdef CONFIG_FB_S5P_MDNIE
static struct platform_mdnie_data mdnie_data = {
	.display_type	= 0,
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

#ifdef CONFIG_BACKLIGHT_LP855X
#if defined(CONFIG_MACH_KONA)
#define EPROM_CFG5_ADDR 0xA5
#define EPROM_A5_VAL    0xA0 /* PWM_DIRECT(7)=1, PS_MODE(6:4)=4drivers*/
#define EPROM_A5_MASK   0x0F /* PWM_FREQ(3:0) : mask */

static struct lp855x_rom_data lp8556_eprom_arr[] = {
    {EPROM_CFG5_ADDR, EPROM_A5_VAL, EPROM_A5_MASK},
};

static struct lp855x_platform_data lp8856_bl_pdata = {
    .mode       = PWM_BASED,
    .device_control = PWM_CONFIG(LP8556),
    .load_new_rom_data = 1,
    .size_program   = ARRAY_SIZE(lp8556_eprom_arr),
    .rom_data   = lp8556_eprom_arr,
    .use_gpio_en    = 1,
    .gpio_en    = GPIO_LED_BACKLIGHT_RESET,
    .power_on_udelay = 1000,
};

static struct i2c_board_info i2c_devs24_emul[] __initdata = {
    {
        I2C_BOARD_INFO("lp8556", (0x58 >> 1)),
        .platform_data  = &lp8856_bl_pdata,
    },
};
static int lcd_bl_init(void)
{
    i2c_register_board_info(24, i2c_devs24_emul,
        ARRAY_SIZE(i2c_devs24_emul));

    return 0;
}
#else
#define EPROM_CFG3_ADDR		0xA3
#define EPROM_CFG5_ADDR		0xA5
#define EPROM_CFG7_ADDR		0xA7
#define EPROM_CFG9E_ADDR	0x9E

/* DEVICE CONTROL : PWM MODE, SET FAST */
#define DEVICE_CONTROL_VAL	(PWM_CONFIG(LP8556) | 0x80)

static struct lp855x_rom_data lp8556_eprom_arr[] = {
	/* CFG3: SCURVE_EN is linear transitions, SLOPE = 200ms,
	 * FILTER = heavy smoothing,
	 * PWM_INPUT_HYSTERESIS = 1-bit hysteresis with 12-bit resolution
	 */
	{EPROM_CFG3_ADDR, 0x5E, 0x00},
	/* CFG5: No PWM_DIRECT, PS_MODE = 4drivers, PWM_FREQ = 9616Hz */
	{EPROM_CFG5_ADDR, 0x24, 0x00},
#ifdef CONFIG_FB_S5P_S6D7AA0
	/* CFG7: EN_DRV3 = EN, EN_DRV2 = EN, IBOOST_LIM = 1.5A */
	{EPROM_CFG7_ADDR, 0xFA, 0x00},
	/* CFG9E : VBOOST_RANGE= 7v~34v */
	{EPROM_CFG9E_ADDR, 0x00, 0x00},
#endif
};

static struct lp855x_rom_data lp8556_eprom_3drv_arr[] = {
	/* CFG3: SCURVE_EN is linear transitions, SLOPE = 200ms,
	 * FILTER = heavy smoothing,
	 * PWM_INPUT_HYSTERESIS = 1-bit hysteresis with 12-bit resolution
	 */
	{EPROM_CFG3_ADDR, 0x5E, 0x00},
	/* CFG5: No PWM_DIRECT, PS_MODE = 3drivers, PWM_FREQ = 9616Hz */
	{EPROM_CFG5_ADDR, 0x34, 0x00},
	/* CFG7: EN_DRV3 = EN, EN_DRV2 = EN, IBOOST_LIM = 1.5A */
	{EPROM_CFG7_ADDR, 0xFA, 0x00},
};

static struct lp855x_platform_data lp8856_bl_pdata = {
	.mode		= PWM_BASED,
	.device_control	= DEVICE_CONTROL_VAL,
	.initial_brightness = 120,
	.load_new_rom_data = 1,
	.size_program	= ARRAY_SIZE(lp8556_eprom_arr),
	.rom_data	= lp8556_eprom_arr,
	.use_gpio_en	= 1,
	.gpio_en	= GPIO_LED_BACKLIGHT_RESET,
	.power_on_udelay = 1000,
};

static struct i2c_board_info i2c_devs24_emul[] __initdata = {
	{
		I2C_BOARD_INFO("lp8556", (0x58 >> 1)),
		.platform_data	= &lp8856_bl_pdata,
	},
};

static int lcd_bl_init(void)
{
#ifdef CONFIG_FB_S5P_S6D7AA0
	if (0x800 == (lcdtype & 0xff00)) {
		lp8856_bl_pdata.rom_data = lp8556_eprom_3drv_arr;
		lp8856_bl_pdata.size_program = ARRAY_SIZE(lp8556_eprom_3drv_arr);
	}
#endif

	i2c_register_board_info(24, i2c_devs24_emul,
				ARRAY_SIZE(i2c_devs24_emul));

	return 0;
}
#endif
#endif

#if defined(CONFIG_LCD_FREQ_SWITCH)
struct platform_device lcdfreq_device = {
		.name		= "lcdfreq",
		.id		= -1,
		.dev		= {
			.parent = &s3c_device_fb.dev,
			.platform_data = &lcd_panel_pdata.freq_limit
	},
};
#endif

