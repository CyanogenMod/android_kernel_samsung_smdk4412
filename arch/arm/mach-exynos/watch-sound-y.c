/*
 * watch-sound-y.c - Sound Management of WATCH Project
 *
 *  Copyright (C) 2012 Samsung Electrnoics
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <mach/irqs.h>
#include <mach/pmu.h>
#include <mach/spi-clocks.h>
#include <mach/watch-sound.h>

#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/s3c64xx-spi.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio-midas.h>

#include "../../../sound/soc/codecs/ymu831/ymu831_priv.h"

static bool watch_snd_mclk_enabled;

static DEFINE_SPINLOCK(watch_snd_spinlock);

void midas_snd_set_mclk(bool on, bool forced)
{
	static int use_cnt;

	spin_lock(&watch_snd_spinlock);

	watch_snd_mclk_enabled = on;

	if (watch_snd_mclk_enabled) {
		if (use_cnt++ == 0 || forced) {
			printk(KERN_INFO "Sound: enabled mclk\n");
			exynos4_pmu_xclkout_set(watch_snd_mclk_enabled,
							XCLKOUT_XUSBXTI);
			mdelay(10);
		}
	} else {
		if ((--use_cnt <= 0) || forced) {
			printk(KERN_INFO "Sound: disabled mclk\n");
			exynos4_pmu_xclkout_set(watch_snd_mclk_enabled,
							XCLKOUT_XUSBXTI);
			use_cnt = 0;
		}
	}

	spin_unlock(&watch_snd_spinlock);

	pr_info("Sound: state: %d, use_cnt: %d\n",
					watch_snd_mclk_enabled, use_cnt);
}

bool midas_snd_get_mclk(void)
{
	return watch_snd_mclk_enabled;
}

#ifdef CONFIG_SND_USE_YMU831_LDODE_GPIO
static void ymu831_set_ldod(int status)
{
#ifdef GPIO_YMU_LDO_EN
	gpio_set_value(GPIO_YMU_LDO_EN, status);
#endif
}
#endif

static struct mc_asoc_platform_data mc_asoc_pdata = {
#ifdef CONFIG_SND_USE_YMU831_LDODE_GPIO
	.set_codec_ldod = ymu831_set_ldod,
#endif
	.set_codec_mclk = midas_snd_set_mclk,
};

static struct s3c64xx_spi_csinfo spi_audio_csi[] = {
	[0] = {
		.line		= EXYNOS4_GPB(1),
		.set_level	= gpio_set_value,
		.fb_delay	= 0x2,
	},
};

static struct spi_board_info spi0_board_info[] __initdata = {
	{
		.modalias		= "ymu831",
		.platform_data		= &mc_asoc_pdata,
		.max_speed_hz		= 10*1000*1000,
		.bus_num		= 0,
		.chip_select		= 0,
		.mode			= SPI_MODE_0,
		.controller_data	= &spi_audio_csi[0],
	}
};

static int set_watch_micbias(int on)
{
	struct regulator *micbias_ldo;
	static int enabled = 0;
	int ret = 0;

	pr_info("%s(%d)\n", __func__, on);
	if (enabled != on) {
		micbias_ldo = regulator_get(NULL, "micbias_ldo_2.8V");
		if (IS_ERR(micbias_ldo)) {
			pr_err("%s : get micbias_ldo error\n", __func__);
			return PTR_ERR(micbias_ldo);
		}

		if (on)
			regulator_enable(micbias_ldo);
		else {
			if (regulator_is_enabled(micbias_ldo))
				regulator_disable(micbias_ldo);
		}

		if (regulator_is_enabled(micbias_ldo) == !!on)
			enabled = on;
		else {
			pr_err("%s : regulator setting failed\n", __func__);
			ret = -1;
		}
	} else {
		pr_info("%s : micbias already %s\n", __func__, enabled ? "on" : "off");
	}

	return ret;
}

static struct watch_audio_platform_data watch_audio_pdata = {
	.set_micbias = set_watch_micbias,
};

static struct platform_device ymu831_snd_card_device = {
	.name	= "ymu831-card",
	.id	= -1,
	.dev = {
		.platform_data = &watch_audio_pdata,
	}
};

static void watch_audio_gpio_init(void)
{
	int err;
	unsigned int gpio;

#ifdef GPIO_YMU_LDO_EN
	err = gpio_request(GPIO_YMU_LDO_EN, "YMU_LDO_EN");
	if (err) {
		pr_err(KERN_ERR "YMU_LDO_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_YMU_LDO_EN, 1);
	gpio_set_value(GPIO_YMU_LDO_EN, 0);
	gpio_free(GPIO_YMU_LDO_EN);
#endif

#ifdef GPIO_MAIN_MIC_SW
	err = gpio_request(GPIO_MAIN_MIC_SW, "MAIN_MIC_SW");
	if (err) {
		pr_err(KERN_ERR "MAIN_MIC_SW GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_MAIN_MIC_SW, 1);
	gpio_set_value(GPIO_MAIN_MIC_SW, 1);
	gpio_free(GPIO_MAIN_MIC_SW);
#endif

	if (!gpio_request(EXYNOS4_GPB(1), "SPI_CS0")) {
		gpio_direction_output(EXYNOS4_GPB(1), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(1), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(1), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(0, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi_audio_csi));
	}
	for (gpio = EXYNOS4_GPB(0); gpio < EXYNOS4_GPB(4); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

}


static struct platform_device *watch_sound_devices[] __initdata = {
	&exynos_device_spi0,
	&ymu831_snd_card_device,
};

void __init midas_sound_init(void)
{
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi0_dev = &exynos_device_spi0.dev;
	printk(KERN_INFO "Sound: start %s\n", __func__);

	watch_audio_gpio_init();

	platform_add_devices(watch_sound_devices,
		ARRAY_SIZE(watch_sound_devices));

	sclk = clk_get(spi0_dev, "dout_spi0");
	if (IS_ERR(sclk))
		dev_err(spi0_dev, "failed to get sclk for SPI-0\n");
	prnt = clk_get(spi0_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi0_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
			   prnt->name, sclk->name);

	clk_put(sclk);
	clk_put(prnt);

	spi_register_board_info(spi0_board_info, ARRAY_SIZE(spi0_board_info));
}
