/*
 *  trats_mc1n2.c
 *
 *  Copyright (c) 2009 Samsung Electronics Co. Ltd
 *  Author: aitdark.park  <aitdark.park@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or  modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#ifdef SND_SOC_MC1N2_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <linux/input.h>

#include <sound/jack.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/regs-clock.h>
#include <mach/pmu.h>
#include "../codecs/mc1n2/mc1n2.h"
#include "slp_jack.h"

static struct platform_device *trats_snd_device;

#define SEC_JACK_ADC_CH		3

static struct s3c_adc_client *padc;

static struct slp_jack_zone slp_jack_zones[] = {
	{
		/* adc < 50, unstable zone, default to 3pole if it stays
		* in this range for a half second (20ms delays, 25 samples)
		*/
		.adc_high = 0,
		.delay_ms = 15,
		.check_count = 20,
		.jack_type = SND_JACK_HEADPHONE,
	},
	{
		/* 50 < adc <= 490, unstable zone, default to 3pole if it stays
		* in this range for a second (10ms delays, 100 samples)
		*/
		.adc_high = 1200,
		.delay_ms = 10,
		.check_count = 80,
		.jack_type = SND_JACK_HEADPHONE,
	},
	{
		/* 490 < adc <= 900, unstable zone, default to 4pole if it
		* stays in this range for a second (10ms delays, 100 samples)
		*/
		.adc_high = 2600,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SND_JACK_HEADSET,
	},
	{
		/* 900 < adc <= 1500, 4 pole zone, default to 4pole if it
		* stays in this range for 200ms (20ms delays, 10 samples)
		*/
		.adc_high = 3800,
		.delay_ms = 10,
		.check_count = 15,
		.jack_type = SND_JACK_HEADSET,
	},
	{
		/* adc > 1500, unstable zone, default to 3pole if it stays
		* in this range for a second (10ms delays, 100 samples)
		*/
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 100,
		.jack_type = SND_JACK_HEADPHONE,
	},
};

/* To support 3-buttons earjack */
static struct slp_jack_buttons_zone slp_jack_buttons_zones[] = {
	{
		/* 0 <= adc <= 93, stable zone */
		.code           = KEY_MEDIA,
		.adc_low        = 0,
		.adc_high       = 105,
	},
	{
		/* 94 <= adc <= 167, stable zone */
		.code           = KEY_PREVIOUSSONG,
		.adc_low        = 106,
		.adc_high       = 167,
	},
	{
		/* 168 <= adc <= 370, stable zone */
		.code           = KEY_NEXTSONG,
		.adc_low        = 168,
		.adc_high       = 370,
	},
};

void trats_jack_set_micbias(bool on)
{
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	gpio_set_value(GPIO_EAR_MIC_BIAS_EN, on);
#endif /* #ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS */
};

int trats_jack_get_adc(void)
{
	int adc;

	adc = s3c_adc_read(padc, SEC_JACK_ADC_CH);

	return adc;
};

void trats_jack_init(void)
{
	padc = s3c_adc_register(trats_snd_device, NULL, NULL, 0);
};

struct slp_jack_platform_data trats_jack_pdata = {
	.set_micbias_state = trats_jack_set_micbias,
	.get_adc_value = trats_jack_get_adc,
	.zones = slp_jack_zones,
	.num_zones = ARRAY_SIZE(slp_jack_zones),
	.buttons_zones = slp_jack_buttons_zones,
	.num_buttons_zones = ARRAY_SIZE(slp_jack_buttons_zones),
	.det_gpio = GPIO_DET_35,
	.send_end_gpio = GPIO_EAR_SEND_END,
	.jack_mach_init = trats_jack_init,
};

static bool xclkout_enabled;

int mc1n2_set_mclk_source(bool on)
{
	if (on) {
		exynos4_pmu_xclkout_set(1, XCLKOUT_XUSBXTI);
		xclkout_enabled = true;
	} else {
		exynos4_pmu_xclkout_set(0, XCLKOUT_XUSBXTI);
		xclkout_enabled = false;
	}

	mdelay(10);

	return 0;
}
EXPORT_SYMBOL(mc1n2_set_mclk_source);

static int trats_hifi_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;

	/* Set the codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* Set the cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;
/* check later
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_I2SV2_CLKSRC_CDCLK,
				0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,
				0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
*/

	ret = snd_soc_dai_set_clkdiv(codec_dai,	MC1N2_BCLK_MULT,
					MC1N2_LRCK_X32);

	if (ret < 0)
		return ret;

	return 0;
}

static int trats_voice_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	if (params_rate(params) != 8000)
		return -EINVAL;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
				SND_SOC_DAIFMT_IB_NF |
				SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set the codec clock divide */
	ret = snd_soc_dai_set_clkdiv(codec_dai,
				MC1N2_BCLK_MULT, MCDRV_BCKFS_32);
	if (ret < 0)
		return ret;

	return 0;
}

static int trats_bt_voice_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	if (params_rate(params) != 8000)
		return -EINVAL;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
				SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set the codec clock divide */
	ret = snd_soc_dai_set_clkdiv(codec_dai,
				MC1N2_BCLK_MULT, MCDRV_BCKFS_32);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct snd_soc_dapm_widget trats_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("SPOUTLN"),
	SND_SOC_DAPM_OUTPUT("SPOUTLP"),

	SND_SOC_DAPM_OUTPUT("RCOUTN"),
	SND_SOC_DAPM_OUTPUT("RCOUTP"),

	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),

	SND_SOC_DAPM_SPK("SPK", NULL),
	SND_SOC_DAPM_SPK("RCV", NULL),

	SND_SOC_DAPM_HP("HP", NULL),

	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_MIC("Sub Mic", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_LINE("FM In", NULL),
};

static const struct snd_soc_dapm_route trats_dapm_routes[] = {
	/* mono speaker */
	{"SPK", NULL, "SPOUTLN"},
	{"SPK", NULL, "SPOUTLP"},
	{"SPOUTLN", NULL, "SPL MIXER"},
	{"SPOUTLP", NULL, "SPL MIXER"},
	/* receiver */
	{"RCV", NULL, "RCOUTN"},
	{"RCV", NULL, "RCOUTP"},
	{"RCOUTN", NULL, "RC MIXER"},
	{"RCOUTP", NULL, "RC MIXER"},
	/* headset */
	{"HP", NULL, "HPOUTL"},
	{"HP", NULL, "HPOUTR"},
	{"HPOUTL", NULL, "HPL MIXER"},
	{"HPOUTR", NULL, "HPR MIXER"},
	/* main mic */
	{"MIC1", NULL, "Main Mic"},
	/* sub mic */
	{"MIC3", NULL, "Sub Mic"},
	/* headset mic */
	{"MIC2", NULL, "Headset Mic"},
	/* linein fmradio */
	{"LINEIN", NULL, "FM In"},
};

static int trats_get_enable_codec(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int suspended;

	suspended = pm_runtime_suspended(codec->dev);
	ucontrol->value.integer.value[0] = !suspended;
#endif
	return 0;
}

static int trats_set_enable_codec(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	pm_runtime_get_sync(codec->dev);
#endif

	return 0;
}

static int trats_get_disable_codec(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int suspended;

	suspended = pm_runtime_suspended(codec->dev);
	ucontrol->value.integer.value[0] = !suspended;
#endif

	return 0;
}

static int trats_set_disable_codec(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (pm_runtime_suspended(codec->dev)) {
		dev_err(codec->dev, "%s: Codec has already suspended\n",
								__func__);
		return -EPERM;
	}

	pm_runtime_put(codec->dev);

#endif
	return 0;
}

static const struct snd_kcontrol_new trats_pm_controls[] = {
	SOC_SINGLE_BOOL_EXT("Enable Codec", 0,
			trats_get_enable_codec,
			trats_set_enable_codec),
	SOC_SINGLE_BOOL_EXT("Disable Codec", 0,
			trats_get_disable_codec,
			trats_set_disable_codec),
};

static int trats_startup(struct snd_pcm_substream *substream)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	pm_runtime_get_sync(codec->dev);
	dev_info(codec->dev, "%s: %d\n", __func__,
				atomic_read(&codec->dev->power.usage_count));
#endif
	return 0;
}

static void trats_shutdown(struct snd_pcm_substream *substream)
{
#ifdef SND_SOC_MC1N2_PM_RUNTIME
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	if (pm_runtime_suspend(codec->dev)) {
		dev_warn(codec->dev,
			"%s: The power state of sound is suspended", __func__);
		return;
	}
	pm_runtime_put(codec->dev);
	dev_info(codec->dev, "%s: %d\n", __func__,
				atomic_read(&codec->dev->power.usage_count));
#endif
}

static int trats_hifiaudio_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	int ret;

	/* add codec power control */
	ret = snd_soc_add_controls(codec, trats_pm_controls,
					ARRAY_SIZE(trats_pm_controls));
	if (ret < 0) {
		pr_err("%s: add control failed\n", __func__);
		goto exit;
	}

	snd_soc_dapm_new_controls(&codec->dapm, trats_dapm_widgets,
						ARRAY_SIZE(trats_dapm_widgets));

	snd_soc_dapm_add_routes(&codec->dapm, trats_dapm_routes,
						ARRAY_SIZE(trats_dapm_routes));

	snd_soc_dapm_sync(&codec->dapm);

	slp_jack_init(codec->card->snd_card, "Headset Jack",
			SND_JACK_HEADSET | SND_JACK_AVOUT, &trats_jack_pdata);

exit:
	return 0;
}

/*
 * U1 MC1N2 DAI operations.
 */
static struct snd_soc_ops trats_hifi_ops = {
	.startup = trats_startup,
	.shutdown = trats_shutdown,
	.hw_params = trats_hifi_hw_params,
};

static struct snd_soc_ops trats_voice_ops = {
	.startup = trats_startup,
	.shutdown = trats_shutdown,
	.hw_params = trats_voice_hw_params,
};

static struct snd_soc_ops trats_bt_voice_ops = {
	.startup = trats_startup,
	.shutdown = trats_shutdown,
	.hw_params = trats_bt_voice_hw_params,
};

#define MC1N2_PCM_RATE	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)
#define MC1N2_PCM_FORMATS \
		(SNDRV_PCM_FMTBIT_S8 | \
		 SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
		 SNDRV_PCM_FMTBIT_A_LAW | SNDRV_PCM_FMTBIT_MU_LAW)

static struct snd_soc_dai_driver trats_ext_dai[] = {
	{
		.name = "trats.cp",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
	},
	{
		.name = "trats.bt",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
	},
};

static struct snd_soc_dai_link trats_dai[] = {
	{ /* Primary DAI i/f */
		.name = "MC1N2 HiFi",
		.stream_name = "hifiaudio",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "mc1n2-da0i",
		.platform_name = "samsung-audio",
		.codec_name = "mc1n2.6-003a",
		.init = trats_hifiaudio_init,
		.ops = &trats_hifi_ops,
	},
	{
		.name = "MC1N2 Voice",
		.stream_name = "voice",
		.cpu_dai_name = "trats.cp",
		.codec_dai_name = "mc1n2-da1p",
		.platform_name = "snd-soc-dummy",
		.codec_name = "mc1n2.6-003a",
		.ops = &trats_voice_ops,
		.ignore_suspend = 1,
	},
#if defined(CONFIG_SND_SAMSUNG_LP) || defined(CONFIG_SND_SAMSUNG_ALP)
	{ /* Sec_Fifo DAI i/f */
		.name = "MC1N2 Sec_FIFO TX",
		.stream_name = "Sec_Dai",
		.cpu_dai_name = "samsung-i2s.4",
		.codec_dai_name = "mc1n2-da0i",
#ifndef CONFIG_SND_SOC_SAMSUNG_USE_DMA_WRAPPER
		.platform_name = "samsung-audio-idma",
#else
		.platform_name = "samsung-audio",
#endif
		.codec_name = "mc1n2.6-003a",
		.init = trats_hifiaudio_init,
		.ops = &trats_hifi_ops,
	},
#endif
	{
		.name = "MC1N2 Bluetooth",
		.stream_name = "bt voice",
		.cpu_dai_name = "trats.bt",
		.codec_dai_name = "mc1n2-da2p",
		.platform_name = "snd-soc-dummy",
		.codec_name = "mc1n2.6-003a",
		.ops = &trats_bt_voice_ops,
		.ignore_suspend = 1,
	},
};

static int trats_card_suspend(struct snd_soc_card *card)
{
	exynos4_sys_powerdown_xusbxti_control(xclkout_enabled ? 1 : 0);

	return 0;
}

static struct snd_soc_card trats_snd_card = {
	.name = "mc1n2",
	.dai_link = trats_dai,
	.num_links = ARRAY_SIZE(trats_dai),

	.suspend_post = trats_card_suspend,
};

static int __init trats_audio_init(void)
{
	int ret;

	mc1n2_set_mclk_source(1);

	trats_snd_device = platform_device_alloc("soc-audio", -1);
	if (!trats_snd_device)
		return -ENOMEM;

	/* register external DAI */
	ret = snd_soc_register_dais(&trats_snd_device->dev,
			trats_ext_dai, ARRAY_SIZE(trats_ext_dai));
	if (ret)
		return ret;

	platform_set_drvdata(trats_snd_device, &trats_snd_card);

	ret = platform_device_add(trats_snd_device);
	if (ret)
		platform_device_put(trats_snd_device);

	return ret;
}

static void __exit trats_audio_exit(void)
{
	platform_device_unregister(trats_snd_device);
}

module_init(trats_audio_init);

MODULE_AUTHOR("KwangHui Cho, kwanghui.cho@samsung.com");
MODULE_DESCRIPTION("ALSA SoC TRATS MC1N2");
MODULE_LICENSE("GPL");
