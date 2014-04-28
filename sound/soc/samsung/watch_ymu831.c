/*
 *  watch_ymu831.c
 *
 *  Copyright (c) 2013 Samsung Electronics Co. Ltd
 *
 *  This program is free software; you can redistribute  it and/or  modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#include <mach/midas-sound.h>
#include <mach/pmu.h>
#include <mach/regs-clock.h>
#include <mach/watch-sound.h>

#include <plat/gpio-cfg.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "../codecs/ymu831/ymu831.h"

struct watch_machine_priv {
	int (*set_mic_bias) (int on);
	int micbias_status;
};

static int watch_hifi_hw_params(struct snd_pcm_substream *substream,
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

	ret = snd_soc_dai_set_clkdiv(codec_dai, MC_ASOC_BCLK_MULT,
				MC_ASOC_LRCK_X32);

	if (ret < 0)
		return ret;

	return 0;
}


/*
 * WATCH YMU831 DAI operations.
 */
static struct snd_soc_ops watch_hifi_ops = {
	.hw_params = watch_hifi_hw_params,
};

const char *micbias_control_text[] = {
	"Off", "On"
};

static const struct soc_enum micbias_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(micbias_control_text), micbias_control_text),
};

static int get_micbias_switch(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct watch_machine_priv *machine_watch
		= snd_soc_card_get_drvdata(codec->card);

	ucontrol->value.integer.value[0] = machine_watch->micbias_status;
	return 0;
}

static int set_micbias_switch(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct watch_machine_priv *machine_watch
		= snd_soc_card_get_drvdata(codec->card);
	int status = ucontrol->value.integer.value[0];

	machine_watch->set_mic_bias(status);
	machine_watch->micbias_status = status;
	return 0;
}

static const struct snd_kcontrol_new watch_controls[] = {
	SOC_ENUM_EXT("MICBIAS SWITCH", micbias_control_enum[0],
		get_micbias_switch, set_micbias_switch),
};

static struct snd_soc_dai_link watch_dai[] = {
	{ /* Primary DAI i/f */
		.name = "YMU831 AIF1",
		.stream_name = "Pri_Dai",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "ymu831-da0",
		.platform_name = "samsung-audio",
		.codec_name = "spi0.0", /* SPI bus:0, chipselect:0 */
		.ops = &watch_hifi_ops,
	},
};

static struct snd_soc_card ymu831_snd_card = {
	.name = "WATCH-YMU831",
	.dai_link = watch_dai,
	.num_links = ARRAY_SIZE(watch_dai),

	.controls = watch_controls,
	.num_controls = ARRAY_SIZE(watch_controls),
};

static struct platform_device *watch_snd_device;

static int __devinit snd_watch_probe(struct platform_device *pdev)
{
	int ret;
	struct watch_machine_priv *machine_watch;
	struct watch_audio_platform_data *audio_pdata =
					pdev->dev.platform_data;

	machine_watch = kzalloc(sizeof *machine_watch, GFP_KERNEL);
	if (!machine_watch) {
		pr_err("Failed to allocate memory\n");
		return -ENOMEM;
	}
	machine_watch->set_mic_bias = audio_pdata->set_micbias;

	snd_soc_card_set_drvdata(&ymu831_snd_card, machine_watch);

	midas_snd_set_mclk(1, 0);

	watch_snd_device = platform_device_alloc("soc-audio", -1);
	if (!watch_snd_device) {
		ret = -ENOMEM;
		goto err_device_alloc;
	}

	platform_set_drvdata(watch_snd_device, &ymu831_snd_card);

	ret = platform_device_add(watch_snd_device);
	if (ret) {
		platform_device_put(watch_snd_device);
	}

	return ret;

err_device_alloc:
	kfree(machine_watch);
	return ret;
}

static int __devexit snd_watch_remove(struct platform_device *pdev)
{
	struct watch_machine_priv *machine_watch = snd_soc_card_get_drvdata(&ymu831_snd_card);
	platform_device_unregister(watch_snd_device);
	kfree(machine_watch);

	return 0;
}


static struct platform_driver snd_watch_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ymu831-card",
	},
	.probe = snd_watch_probe,
	.remove = __devexit_p(snd_watch_remove),
};


static int __init watch_audio_init(void)
{
	return platform_driver_register(&snd_watch_driver);
}
module_init(watch_audio_init);

static void __exit watch_audio_exit(void)
{
	platform_driver_unregister(&snd_watch_driver);
}

module_exit(watch_audio_exit);

MODULE_DESCRIPTION("ALSA SoC WATCH YMU831");
MODULE_LICENSE("GPL");
