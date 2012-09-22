/*
 * soc-util.c  --  ALSA SoC Audio Layer utility functions
 *
 * Copyright 2009 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *         Liam Girdwood <lrg@slimlogic.co.uk>
 *         
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#ifdef CONFIG_SLP_WIP
#include <linux/slab.h>
#endif

int snd_soc_calc_frame_size(int sample_size, int channels, int tdm_slots)
{
	return sample_size * channels * tdm_slots;
}
EXPORT_SYMBOL_GPL(snd_soc_calc_frame_size);

int snd_soc_params_to_frame_size(struct snd_pcm_hw_params *params)
{
	int sample_size;

	sample_size = snd_pcm_format_width(params_format(params));
	if (sample_size < 0)
		return sample_size;

	return snd_soc_calc_frame_size(sample_size, params_channels(params),
				       1);
}
EXPORT_SYMBOL_GPL(snd_soc_params_to_frame_size);

int snd_soc_calc_bclk(int fs, int sample_size, int channels, int tdm_slots)
{
	return fs * snd_soc_calc_frame_size(sample_size, channels, tdm_slots);
}
EXPORT_SYMBOL_GPL(snd_soc_calc_bclk);

int snd_soc_params_to_bclk(struct snd_pcm_hw_params *params)
{
	int ret;

	ret = snd_soc_params_to_frame_size(params);

	if (ret > 0)
		return ret * params_rate(params);
	else
		return ret;
}
EXPORT_SYMBOL_GPL(snd_soc_params_to_bclk);

static const struct snd_pcm_hardware dummy_dma_hardware = {
	.formats		= 0xffffffff,
	.channels_min		= 1,
	.channels_max		= UINT_MAX,

	/* Random values to keep userspace happy when checking constraints */
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= PAGE_SIZE,
	.period_bytes_max	= PAGE_SIZE*2,
	.periods_min		= 2,
	.periods_max		= 128,
};

#ifdef CONFIG_SLP_WIP
struct dma_dummy {
	unsigned long base_time;
	unsigned long pos;
	unsigned long max;
};
#endif

static int dummy_dma_open(struct snd_pcm_substream *substream)
{
#ifdef CONFIG_SLP_WIP
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_dummy *pdma;

	pdma = kzalloc(sizeof(struct dma_dummy), GFP_KERNEL);
	if (!pdma)
		return -ENOMEM;

	runtime->private_data = pdma;
#endif
	snd_soc_set_runtime_hwparams(substream, &dummy_dma_hardware);

	return 0;
}

#ifdef CONFIG_SLP_WIP
static int dummy_dma_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_dummy *pdma = runtime->private_data;

	kfree(pdma);

	return 0;
}

static int dummy_dma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_dummy *pdma = runtime->private_data;

	pdma->base_time = jiffies;
	pdma->pos = 0;
	pdma->max = runtime->buffer_size * HZ;

	return 0;
}

static int dummy_dma_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	/* do nothing */
	return 0;
}

static snd_pcm_uframes_t
dummy_dma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_dummy *pdma = runtime->private_data;
	unsigned long delta;
	unsigned long pos;

	delta = jiffies - pdma->base_time;
	if (!delta)
		return pdma->pos;

	/* update base_time */
	pdma->base_time += delta;

	/* calc */
	delta *= runtime->rate;
	pdma->pos += delta;
	while (pdma->pos >= pdma->max)
		pdma->pos -= pdma->max;

	pos = pdma->pos / HZ;

	return pos;
}

static int dummy_dma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_dummy *pdma = runtime->private_data;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		pdma->base_time = jiffies;
		break;
	default:
		break;
	}

	return 0;
}
#endif

static struct snd_pcm_ops dummy_dma_ops = {
	.open		= dummy_dma_open,
	.ioctl		= snd_pcm_lib_ioctl,
#ifdef CONFIG_SLP_WIP
	.close		= dummy_dma_close,
	.copy		= dummy_dma_copy,
	.pointer	= dummy_dma_pointer,
	.prepare	= dummy_dma_prepare,
	.trigger	= dummy_dma_trigger,
#endif
};

static struct snd_soc_platform_driver dummy_platform = {
	.ops = &dummy_dma_ops,
};

static __devinit int snd_soc_dummy_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &dummy_platform);
}

static __devexit int snd_soc_dummy_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);

	return 0;
}

static struct platform_driver soc_dummy_driver = {
	.driver = {
		.name = "snd-soc-dummy",
		.owner = THIS_MODULE,
	},
	.probe = snd_soc_dummy_probe,
	.remove = __devexit_p(snd_soc_dummy_remove),
};

static struct platform_device *soc_dummy_dev;

int __init snd_soc_util_init(void)
{
	int ret;

	soc_dummy_dev = platform_device_alloc("snd-soc-dummy", -1);
	if (!soc_dummy_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_dummy_dev);
	if (ret != 0) {
		platform_device_put(soc_dummy_dev);
		return ret;
	}

	ret = platform_driver_register(&soc_dummy_driver);
	if (ret != 0)
		platform_device_unregister(soc_dummy_dev);

	return ret;
}

void __exit snd_soc_util_exit(void)
{
	platform_device_unregister(soc_dummy_dev);
	platform_driver_unregister(&soc_dummy_driver);
}
