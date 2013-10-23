/*
 *  kona_wm1811.c
 *
 *  Copyright (c) 2011 Samsung Electronics Co. Ltd
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/jack.h>

#include <mach/regs-clock.h>
#include <mach/pmu.h>
#include <mach/midas-sound.h>

#include <linux/mfd/wm8994/core.h>
#include <linux/mfd/wm8994/registers.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
#include <linux/exynos_audio.h>
#endif

#ifdef CONFIG_USE_ADC_DET
#include <plat/adc.h>
#endif

#if defined(CONFIG_STMPE811_ADC)
#include <linux/stmpe811-adc.h>
#endif

#include "i2s.h"
#include "s3c-i2s-v2.h"
#include "../codecs/wm8994.h"

#if defined(CONFIG_STMPE811_ADC)
#define SEC_JACK_ADC_CH		4
#else
#define SEC_JACK_ADC_CH		3
#endif

#define MIDAS_DEFAULT_MCLK1	24000000
#define MIDAS_DEFAULT_MCLK2	32768
#define MIDAS_DEFAULT_SYNC_CLK	11289600

#define WM1811_JACKDET_MODE_NONE  0x0000
#define WM1811_JACKDET_MODE_JACK  0x0100
#define WM1811_JACKDET_MODE_MIC   0x0080
#define WM1811_JACKDET_MODE_AUDIO 0x0180

#define WM1811_JACKDET_BTN0	0x04
#define WM1811_JACKDET_BTN1	0x10
#define WM1811_JACKDET_BTN2	0x08

#define JACK_ADC_CH		2
#define JACK_SAMPLE_SIZE	5

#define MAX_ZONE_LIMIT		10
/* keep this value if you support double-pressed concept */
#define WAKE_LOCK_TIME		(HZ * 5)	/* 5 sec */
#define EAR_CHECK_LOOP_CNT	10

struct wm1811_machine_priv {
	struct snd_soc_jack jack;
	struct snd_soc_codec *codec;
	struct wake_lock jackdet_wake_lock;
	void (*lineout_switch_f) (int on);
	void (*set_main_mic_f) (int on);
	void (*set_sub_mic_f) (int on);
	int (*get_g_det_value_f) (void);
	int (*get_g_det_irq_num_f) (void);
#ifdef CONFIG_USE_ADC_DET
	struct s3c_adc_client *padc;
	struct jack_zone *zones;
	int	num_zones;
	int	use_jackdet_type;
#endif
};

enum {
	SEC_JACK_NO_DEVICE		= 0x0,
	SEC_HEADSET_4POLE		= 0x01 << 0,
	SEC_HEADSET_3POLE		= 0x01 << 1,
	SEC_TTY_DEVICE			= 0x01 << 2,
	SEC_FM_HEADSET			= 0x01 << 3,
	SEC_FM_SPEAKER			= 0x01 << 4,
	SEC_TVOUT_DEVICE		= 0x01 << 5,
	SEC_EXTRA_DOCK_SPEAKER		= 0x01 << 6,
	SEC_EXTRA_CAR_DOCK_SPEAKER	= 0x01 << 7,
	SEC_UNKNOWN_DEVICE		= 0x01 << 8,
};

#ifdef CONFIG_USE_ADC_DET
static bool recheck_jack;
static int jack_get_adc_data(struct s3c_adc_client *padc);
static void jack_set_type(struct wm1811_machine_priv *wm1811, int jack_type);
#endif

static struct wm8958_micd_rate kona_det_rates[] = {
	{ MIDAS_DEFAULT_MCLK2,     true,  0,  0 },
	{ MIDAS_DEFAULT_MCLK2,    false,  0,  0 },
	{ MIDAS_DEFAULT_SYNC_CLK,  true,  7,  7 },
	{ MIDAS_DEFAULT_SYNC_CLK, false,  7,  7 },
};

static struct wm8958_micd_rate kona_jackdet_rates[] = {
	{ MIDAS_DEFAULT_MCLK2,     true,  0,  0 },
	{ MIDAS_DEFAULT_MCLK2,    false,  0,  0 },
	{ MIDAS_DEFAULT_SYNC_CLK,  true, 12, 12 },
	{ MIDAS_DEFAULT_SYNC_CLK, false,  7,  8 },
};

static int aif2_mode;
const char *aif2_mode_text[] = {
	"Slave", "Master"
};

static int input_clamp;
const char *input_clamp_text[] = {
	"Off", "On"
};

static int lineout_mode;
const char *lineout_mode_text[] = {
	"Off", "On"
};

static int aif2_digital_mute;
const char *switch_mode_text[] = {
	"Off", "On"
};

static int aif1_ADC_Data;
const char *mic_mode_text[] = {
	"Off", "On"
};

static int aif1_loopback_data;
const char *aif1_loopback_text[] = {
	"Off", "On"
};

#ifndef CONFIG_SEC_DEV_JACK
/* To support PBA function test */
static struct class *jack_class;
static struct device *jack_dev;
#endif

static struct platform_device *kona_snd_device;


static void kona_gpio_init(void)
{
	int err;
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	/* Main Microphone BIAS */
	err = gpio_request(GPIO_MIC_BIAS_EN, "MAIN MIC");
	if (err) {
		pr_err(KERN_ERR "MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_MIC_BIAS_EN, 1);
	gpio_set_value(GPIO_MIC_BIAS_EN, 0);
	gpio_free(GPIO_MIC_BIAS_EN);
#endif

#ifdef CONFIG_SND_USE_SUB_MIC
	/* Sub Microphone BIAS */
	err = gpio_request(GPIO_SUB_MIC_BIAS_EN, "SUB MIC");
	if (err) {
		pr_err(KERN_ERR "SUB_MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_SUB_MIC_BIAS_EN, 1);
	gpio_set_value(GPIO_SUB_MIC_BIAS_EN, 0);
	gpio_free(GPIO_SUB_MIC_BIAS_EN);
#endif

#ifdef CONFIG_SND_USE_LINEOUT_SWITCH
	err = gpio_request(GPIO_LINEOUT_EN, "LINEOUT_EN");
	if (err) {
		pr_err(KERN_ERR "LINEOUT_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_LINEOUT_EN, 1);
	gpio_set_value(GPIO_LINEOUT_EN, 0);
	gpio_free(GPIO_LINEOUT_EN);
#endif
}

static const struct soc_enum lineout_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(lineout_mode_text), lineout_mode_text),
};

static int get_lineout_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = lineout_mode;
	return 0;
}

static int set_lineout_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	lineout_mode = ucontrol->value.integer.value[0];

	if (lineout_mode)
		gpio_set_value(GPIO_LINEOUT_EN, 1);
	else
		gpio_set_value(GPIO_LINEOUT_EN, 0);

	dev_info(codec->dev, "set lineout mode : %s\n",
		lineout_mode_text[lineout_mode]);
	return 0;

}
static const struct soc_enum aif2_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aif2_mode_text), aif2_mode_text),
};

static int get_aif2_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = aif2_mode;
	return 0;
}

static int set_aif2_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	if (aif2_mode == ucontrol->value.integer.value[0])
		return 0;

	aif2_mode = ucontrol->value.integer.value[0];

	pr_info("set aif2 mode : %s\n", aif2_mode_text[aif2_mode]);

	return 0;
}

static const struct soc_enum input_clamp_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(input_clamp_text), input_clamp_text),
};

static int get_input_clamp(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = input_clamp;
	return 0;
}

static int set_input_clamp(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	input_clamp = ucontrol->value.integer.value[0];

	if (input_clamp) {
		snd_soc_update_bits(codec, WM8994_INPUT_MIXER_1,
				WM8994_INPUTS_CLAMP, WM8994_INPUTS_CLAMP);
		msleep(100);
	} else {
		snd_soc_update_bits(codec, WM8994_INPUT_MIXER_1,
				WM8994_INPUTS_CLAMP, 0);
	}
	pr_info("set fm input_clamp : %s\n", input_clamp_text[input_clamp]);

	return 0;
}

static const struct soc_enum switch_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(switch_mode_text), switch_mode_text),
};

static int get_aif2_mute_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = aif2_digital_mute;
	return 0;
}

static int set_aif2_mute_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg;

	aif2_digital_mute = ucontrol->value.integer.value[0];

	if (snd_soc_read(codec, WM8994_POWER_MANAGEMENT_6)
		& WM8994_AIF2_DACDAT_SRC)
		aif2_digital_mute = 0;

	if (aif2_digital_mute)
		reg = WM8994_AIF1DAC1_MUTE;
	else
		reg = 0;

	snd_soc_update_bits(codec, WM8994_AIF2_DAC_FILTERS_1,
				WM8994_AIF1DAC1_MUTE, reg);

	pr_info("set aif2_digital_mute : %s\n",
			switch_mode_text[aif2_digital_mute]);

	return 0;
}

static const struct soc_enum mic_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_mode_text), mic_mode_text),
};

static int get_aif1_ADC_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = aif1_ADC_Data;
	return 0;
}

static int set_aif1_ADC_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg;

	aif1_ADC_Data = ucontrol->value.integer.value[0];

	if (aif1_ADC_Data)
		reg = WM8994_AIF1ADCR_DAT_INV;
	else
		reg = 0;

	snd_soc_update_bits(codec, WM8994_AIF1ADC_DATA,
				WM8994_AIF1ADCR_DAT_INV, reg);

	pr_info("set aif1_ADC_Data : %s\n",
			mic_mode_text[aif1_ADC_Data]);

	return 0;
}

static const struct soc_enum aif1_loopback_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aif1_loopback_text), aif1_loopback_text),
};

static int get_aif1_loopback_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = aif1_loopback_data;
	return 0;
}

static int set_aif1_loopback_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg1, reg2;

	aif1_loopback_data = ucontrol->value.integer.value[0];

	if (aif1_loopback_data){
		reg1 = WM8994_AIF1_LOOPBACK;
		reg2 = 0;
	}else{
		reg1 = 0;
		reg2 = WM8994_AIF1DAC1_MUTE;
	}

	snd_soc_update_bits(codec, WM8994_AIF1_CONTROL_2,
				WM8994_AIF1_LOOPBACK, reg1);
	snd_soc_update_bits(codec, WM8994_AIF1_DAC1_FILTERS_1,
				WM8994_AIF1DAC1_MUTE, reg2);

	pr_info("set aif1_loopback_data : %s\n",
			aif1_loopback_text[aif1_loopback_data]);

	return 0;
}

static int set_ext_micbias(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	dev_info(codec->dev, "%s event is %02X", w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		gpio_set_value(GPIO_MIC_BIAS_EN, 1);
		msleep(100);
		break;
	case SND_SOC_DAPM_POST_PMD:
		gpio_set_value(GPIO_MIC_BIAS_EN, 0);
		break;
	}

	return 0;
}

static int set_ext_submicbias(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	dev_info(codec->dev, "%s event is %02X", w->name, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		gpio_set_value(GPIO_SUB_MIC_BIAS_EN, 1);
		msleep(100);
		break;
	case SND_SOC_DAPM_POST_PMD:
		gpio_set_value(GPIO_SUB_MIC_BIAS_EN, 0);
		break;
	}

	return 0;
}

static int set_lineout_switch(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	dev_info(codec->dev, "%s event is %02X", w->name, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		gpio_set_value(GPIO_LINEOUT_EN, 1);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		gpio_set_value(GPIO_LINEOUT_EN, 0);
		break;
	}

	return 0;
}

static void kona_micd_set_rate(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(codec);
	int best, i, sysclk, val;
	bool idle;
	const struct wm8958_micd_rate *rates = NULL;
	int num_rates = 0;

	idle = !wm8994->jack_mic;

	sysclk = snd_soc_read(codec, WM8994_CLOCKING_1);
	if (sysclk & WM8994_SYSCLK_SRC)
		sysclk = wm8994->aifclk[1];
	else
		sysclk = wm8994->aifclk[0];

	if (wm8994->jackdet) {
		rates = kona_jackdet_rates;
		num_rates = ARRAY_SIZE(kona_jackdet_rates);
		wm8994->pdata->micd_rates = kona_jackdet_rates;
		wm8994->pdata->num_micd_rates = num_rates;
	} else {
		rates = kona_det_rates;
		num_rates = ARRAY_SIZE(kona_det_rates);
		wm8994->pdata->micd_rates = kona_det_rates;
		wm8994->pdata->num_micd_rates = num_rates;
	}

	best = 0;
	for (i = 0; i < num_rates; i++) {
		if (rates[i].idle != idle)
			continue;
		if (abs(rates[i].sysclk - sysclk) <
		    abs(rates[best].sysclk - sysclk))
			best = i;
		else if (rates[best].idle != idle)
			best = i;
	}

	val = rates[best].start << WM8958_MICD_BIAS_STARTTIME_SHIFT
		| rates[best].rate << WM8958_MICD_RATE_SHIFT;

	snd_soc_update_bits(codec, WM8958_MIC_DETECT_1,
			    WM8958_MICD_BIAS_STARTTIME_MASK |
			    WM8958_MICD_RATE_MASK, val);
}

#ifdef CONFIG_USE_ADC_DET
static int jack_get_adc_data(struct s3c_adc_client *padc)
{
	int adc_data;
	int adc_max = 0;
	int adc_min = 0xFFFF;
	int adc_total = 0;
	int adc_retry_cnt = 0;
	int i;

	for (i = 0; i < JACK_SAMPLE_SIZE; i++) {

#if defined(CONFIG_STMPE811_ADC)
		adc_data = stmpe811_get_adc_data(SEC_JACK_ADC_CH);
#else
		adc_data = s3c_adc_read(padc, JACK_ADC_CH);
#endif

		if (adc_data < 0) {

			adc_retry_cnt++;

			if (adc_retry_cnt > 10)
				return adc_data;
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (JACK_SAMPLE_SIZE - 2);
}

static void determine_jack_type(struct wm1811_machine_priv *wm1811)
{
	struct jack_zone *zones = wm1811->zones;
	struct snd_soc_codec *codec = wm1811->codec;
	int size = wm1811->num_zones;
	int count[MAX_ZONE_LIMIT] = {0};
	int adc;
	int i;

	/* set mic bias to enable adc */
	while (snd_soc_read(codec, WM1811_JACKDET_CTRL) & WM1811_JACKDET_LVL) {
		adc = jack_get_adc_data(wm1811->padc);

		pr_info("%s: adc = %d\n", __func__, adc);

		if (adc < 0)
			break;

		/* determine the type of headset based on the
		 * adc value.  An adc value can fall in various
		 * ranges or zones.  Within some ranges, the type
		 * can be returned immediately.  Within others, the
		 * value is considered unstable and we need to sample
		 * a few more types (up to the limit determined by
		 * the range) before we return the type for that range.
		 */
		for (i = 0; i < size; i++) {
			if (adc <= zones[i].adc_high) {
				if (++count[i] > zones[i].check_count) {
					if (recheck_jack == true && i == 4) {
						pr_info("%s : something wrong connection!\n",
								__func__);

						recheck_jack = false;
						return;
					}
					jack_set_type(wm1811,
						zones[i].jack_type);
					return;
				}
				msleep(zones[i].delay_ms);
				break;
			}
		}
	}

	recheck_jack = false;
	/* jack removed before detection complete */
	pr_debug("%s : jack removed before detection complete\n", __func__);
}

static void jack_set_type(struct wm1811_machine_priv *wm1811, int jack_type)
{
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(wm1811->codec);

	if (jack_type == SEC_HEADSET_4POLE) {
		dev_info(wm1811->codec->dev, "Detected microphone\n");

		wm8994->mic_detecting = false;
		wm8994->jack_mic = true;

		kona_micd_set_rate(wm1811->codec);

		snd_soc_jack_report(wm8994->micdet[0].jack, SND_JACK_HEADSET,
				    SND_JACK_HEADSET);

		snd_soc_update_bits(wm1811->codec, WM8958_MIC_DETECT_1,
					    WM8958_MICD_ENA, 1);
	} else {
		dev_info(wm1811->codec->dev, "Detected headphone\n");
		wm8994->mic_detecting = false;

		kona_micd_set_rate(wm1811->codec);

		snd_soc_jack_report(wm8994->micdet[0].jack, SND_JACK_HEADPHONE,
				    SND_JACK_HEADSET);

		/* If we have jackdet that will detect removal */
		if (wm8994->jackdet) {
			snd_soc_update_bits(wm1811->codec, WM8958_MIC_DETECT_1,
					    WM8958_MICD_ENA, 0);

			if (wm8994->active_refcount) {
				snd_soc_update_bits(wm1811->codec,
					WM8994_ANTIPOP_2,
					WM1811_JACKDET_MODE_MASK,
					WM1811_JACKDET_MODE_AUDIO);
			}

			if (wm8994->pdata->jd_ext_cap) {
				mutex_lock(&wm1811->codec->mutex);
				snd_soc_dapm_disable_pin(&wm1811->codec->dapm,
							 "MICBIAS2");
				snd_soc_dapm_sync(&wm1811->codec->dapm);
				mutex_unlock(&wm1811->codec->mutex);
			}
		}
	}
}

static void kona_micdet(void *data)
{
	struct wm1811_machine_priv *wm1811 = data;
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(wm1811->codec);

	struct snd_soc_codec *codec = wm1811->codec;

	pr_info("%s: detected jack\n", __func__);
	wm8994->mic_detecting = true;

	wake_lock_timeout(&wm1811->jackdet_wake_lock, 5 * HZ);

	snd_soc_update_bits(codec, WM8958_MICBIAS2,
				WM8958_MICB2_MODE, 0);
	snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				WM8994_MICB2_ENA_MASK, WM8994_MICB2_ENA);

	determine_jack_type(wm1811);
}
#endif

static void kona_mic_id(void *data, u16 status)
{
	struct wm1811_machine_priv *wm1811 = data;
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(wm1811->codec);

	pr_info("%s: detected jack\n", __func__);
	wake_lock_timeout(&wm1811->jackdet_wake_lock, 5 * HZ);

	/* Either nothing present or just starting detection */
	if (!(status & WM8958_MICD_STS)) {
		if (!wm8994->jackdet) {
			/* If nothing present then clear our statuses */
			dev_dbg(wm1811->codec->dev, "Detected open circuit\n");
			wm8994->jack_mic = false;
			wm8994->mic_detecting = true;

			kona_micd_set_rate(wm1811->codec);

			snd_soc_jack_report(wm8994->micdet[0].jack, 0,
					    wm8994->btn_mask |
					     SND_JACK_HEADSET);
		}
		/*ToDo*/
		/*return;*/
	}

	/* If the measurement is showing a high impedence we've got a
	 * microphone.
	 */
	if (wm8994->mic_detecting && (status & 0x400)) {
		dev_info(wm1811->codec->dev, "Detected microphone\n");

		wm8994->mic_detecting = false;
		wm8994->jack_mic = true;

		kona_micd_set_rate(wm1811->codec);

		snd_soc_jack_report(wm8994->micdet[0].jack, SND_JACK_HEADSET,
				    SND_JACK_HEADSET);
	}

	if (wm8994->mic_detecting && status & 0x4) {
		dev_info(wm1811->codec->dev, "Detected headphone\n");
		wm8994->mic_detecting = false;

		kona_micd_set_rate(wm1811->codec);

		snd_soc_jack_report(wm8994->micdet[0].jack, SND_JACK_HEADPHONE,
				    SND_JACK_HEADSET);

		/* If we have jackdet that will detect removal */
		if (wm8994->jackdet) {
			mutex_lock(&wm8994->accdet_lock);

			snd_soc_update_bits(wm1811->codec, WM8958_MIC_DETECT_1,
					    WM8958_MICD_ENA, 0);

			if (wm8994->active_refcount) {
				snd_soc_update_bits(wm1811->codec,
					WM8994_ANTIPOP_2,
					WM1811_JACKDET_MODE_MASK,
					WM1811_JACKDET_MODE_AUDIO);
			}

			mutex_unlock(&wm8994->accdet_lock);

			if (wm8994->pdata->jd_ext_cap) {
				mutex_lock(&wm1811->codec->mutex);
				snd_soc_dapm_disable_pin(&wm1811->codec->dapm,
							 "MICBIAS2");
				snd_soc_dapm_sync(&wm1811->codec->dapm);
				mutex_unlock(&wm1811->codec->mutex);
			}
		}
	}
}

static int kona_wm1811_aif1_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec;
	unsigned int pll_out;
	int ret;

	dev_info(codec_dai->dev, "%s ++\n", __func__);

	snd_soc_update_bits(codec, WM8994_AIF1_MASTER_SLAVE,
					WM8994_AIF1_TRI, 0);
	snd_soc_update_bits(codec, WM8994_AIF2_MASTER_SLAVE,
					WM8994_AIF2_TRI, 0);

	/* AIF1CLK should be >=3MHz for optimal performance */
	if (params_rate(params) == 8000 || params_rate(params) == 11025)
		pll_out = params_rate(params) * 512;
	else
		pll_out = params_rate(params) * 256;

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

	/* Switch the FLL */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1,
				  WM8994_FLL_SRC_MCLK1, MIDAS_DEFAULT_MCLK1,
				  pll_out);
	if (ret < 0)
		dev_err(codec_dai->dev, "Unable to start FLL1: %d\n", ret);

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
				     pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Unable to switch to FLL1: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
				     0, MOD_OPCLK_PCLK);
	if (ret < 0)
		return ret;

	dev_info(codec_dai->dev, "%s --\n", __func__);

	return 0;
}

/*
 * kona WM1811 DAI operations.
 */
static struct snd_soc_ops kona_wm1811_aif1_ops = {
	.hw_params = kona_wm1811_aif1_hw_params,
};

static int kona_wm1811_aif2_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec;
	int ret;
	int prate;
	int bclk;

	dev_info(codec_dai->dev, "%s ++\n", __func__);

	snd_soc_update_bits(codec, WM8994_AIF1_MASTER_SLAVE,
					WM8994_AIF1_TRI, 0);
	snd_soc_update_bits(codec, WM8994_AIF2_MASTER_SLAVE,
					WM8994_AIF2_TRI, 0);

	prate = params_rate(params);
	switch (params_rate(params)) {
	case 8000:
	case 16000:
	       break;
	default:
		dev_warn(codec_dai->dev, "Unsupported LRCLK %d, falling back to 8000Hz\n",
				(int)params_rate(params));
		prate = 8000;
	}

	/* Set the codec DAI configuration, aif2_mode:0 is slave */
	if (aif2_mode == 0)
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					| SND_SOC_DAIFMT_NB_NF
					| SND_SOC_DAIFMT_CBS_CFS);
	else
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					| SND_SOC_DAIFMT_NB_NF
					| SND_SOC_DAIFMT_CBM_CFM);

	if (ret < 0)
		return ret;

	switch (prate) {
	case 8000:
		bclk = 256000;
		break;
	case 16000:
		bclk = 512000;
		break;
	default:
		return -EINVAL;
	}

	if (aif2_mode == 0) {
		ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL2,
					WM8994_FLL_SRC_BCLK,
					bclk, prate * 256);
	} else {
		ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL2,
					  WM8994_FLL_SRC_MCLK1,
					  MIDAS_DEFAULT_MCLK1, prate * 256);
	}

	if (ret < 0)
		dev_err(codec_dai->dev, "Unable to configure FLL2: %d\n", ret);

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL2,
				     prate * 256, SND_SOC_CLOCK_IN);
	if (ret < 0)
		dev_err(codec_dai->dev, "Unable to switch to FLL2: %d\n", ret);

	dev_info(codec_dai->dev, "%s --\n", __func__);
	return 0;
}

static struct snd_soc_ops kona_wm1811_aif2_ops = {
	.hw_params = kona_wm1811_aif2_hw_params,
};

static int kona_wm1811_aif3_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	pr_err("%s: enter\n", __func__);
	return 0;
}

static struct snd_soc_ops kona_wm1811_aif3_ops = {
	.hw_params = kona_wm1811_aif3_hw_params,
};

static const struct snd_kcontrol_new kona_controls[] = {
	SOC_DAPM_PIN_SWITCH("HP"),
	SOC_DAPM_PIN_SWITCH("SPK"),
	SOC_DAPM_PIN_SWITCH("RCV"),
	SOC_DAPM_PIN_SWITCH("LINE"),
	SOC_DAPM_PIN_SWITCH("HDMI"),
	SOC_DAPM_PIN_SWITCH("Main Mic"),
	SOC_DAPM_PIN_SWITCH("Sub Mic"),
	SOC_DAPM_PIN_SWITCH("Headset Mic"),

	SOC_ENUM_EXT("AIF2 Mode", aif2_mode_enum[0],
		get_aif2_mode, set_aif2_mode),

	SOC_ENUM_EXT("Input Clamp", input_clamp_enum[0],
		get_input_clamp, set_input_clamp),

	SOC_ENUM_EXT("LineoutSwitch Mode", lineout_mode_enum[0],
		get_lineout_mode, set_lineout_mode),

	SOC_ENUM_EXT("AIF2 digital mute", switch_mode_enum[0],
		get_aif2_mute_status, set_aif2_mute_status),

	SOC_ENUM_EXT("Submic ADC invert", mic_mode_enum[0],
		get_aif1_ADC_status, set_aif1_ADC_status),

	SOC_ENUM_EXT("AIF1 Loopback", aif1_loopback_enum[0],
		get_aif1_loopback_status, set_aif1_loopback_status),

};

const struct snd_soc_dapm_widget kona_dapm_widgets[] = {
	SND_SOC_DAPM_HP("HP", NULL),
	SND_SOC_DAPM_SPK("SPK", NULL),
	SND_SOC_DAPM_SPK("RCV", NULL),
	SND_SOC_DAPM_LINE("LINE", set_lineout_switch),
	SND_SOC_DAPM_LINE("HDMI", NULL),

	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Main Mic", set_ext_micbias),
	SND_SOC_DAPM_MIC("Sub Mic", set_ext_submicbias),
};

const struct snd_soc_dapm_route kona_dapm_routes[] = {
	{ "HP", NULL, "HPOUT1L" },
	{ "HP", NULL, "HPOUT1R" },

	{ "SPK", NULL, "SPKOUTLN" },
	{ "SPK", NULL, "SPKOUTLP" },
	{ "SPK", NULL, "SPKOUTRN" },
	{ "SPK", NULL, "SPKOUTRP" },

	{ "RCV", NULL, "HPOUT2N" },
	{ "RCV", NULL, "HPOUT2P" },

	{ "LINE", NULL, "LINEOUT2N" },
	{ "LINE", NULL, "LINEOUT2P" },

	{ "HDMI", NULL, "LINEOUT1N" },
	{ "HDMI", NULL, "LINEOUT1P" },

	{ "IN1LP", NULL, "Main Mic" },
	{ "IN1LN", NULL, "Main Mic" },

	{ "IN2RP:VXRP", NULL, "Sub Mic" },
	{ "IN2RN", NULL, "Sub Mic" },

	{ "IN1RP", NULL, "MICBIAS2" },
	{ "IN1RN", NULL, "MICBIAS2" },
	{ "MICBIAS2", NULL, "Headset Mic" },
};

static struct snd_soc_dai_driver kona_ext_dai[] = {
	{
		.name = "kona.cp",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 16000,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 16000,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
	{
		.name = "kona.bt",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 16000,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 16000,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
};

#ifndef CONFIG_SEC_DEV_JACK
static ssize_t earjack_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct snd_soc_codec *codec = dev_get_drvdata(dev);
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(codec);

	int report = 0;

	if ((wm8994->micdet[0].jack->status & SND_JACK_HEADPHONE) ||
		(wm8994->micdet[0].jack->status & SND_JACK_HEADSET)) {
		report = 1;
	}

	return sprintf(buf, "%d\n", report);
}

static ssize_t earjack_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}

static ssize_t earjack_key_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct snd_soc_codec *codec = dev_get_drvdata(dev);
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(codec);

	int report = 0;

	if (wm8994->micdet[0].jack->status & SND_JACK_BTN_0)
		report = 1;

	return sprintf(buf, "%d\n", report);
}

static ssize_t earjack_key_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}

static ssize_t earjack_select_jack_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s : operate nothing\n", __func__);

	return 0;
}

static ssize_t earjack_select_jack_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct snd_soc_codec *codec = dev_get_drvdata(dev);
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(codec);

	wm8994->mic_detecting = false;
	wm8994->jack_mic = true;

	kona_micd_set_rate(codec);

	if ((!size) || (buf[0] != '1')) {
		snd_soc_jack_report(wm8994->micdet[0].jack,
				    0, SND_JACK_HEADSET);
		dev_info(codec->dev, "Forced remove microphone\n");
	} else {

		snd_soc_jack_report(wm8994->micdet[0].jack,
				    SND_JACK_HEADSET, SND_JACK_HEADSET);
		dev_info(codec->dev, "Forced detect microphone\n");
	}

	return size;
}

static DEVICE_ATTR(select_jack, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_select_jack_show, earjack_select_jack_store);

static DEVICE_ATTR(key_state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_key_state_show, earjack_key_state_store);

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_state_show, earjack_state_store);
#endif

static int kona_wm1811_init_paiftx(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct wm1811_machine_priv *wm1811
		= snd_soc_card_get_drvdata(codec->card);
	struct snd_soc_dai *aif1_dai = rtd->codec_dai;
	struct wm8994 *control = codec->control_data;
	struct wm8994_priv *wm8994 = snd_soc_codec_get_drvdata(codec);
#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	const struct exynos_sound_platform_data *sound_pdata;
#endif
	int ret;
#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	sound_pdata = exynos_sound_get_platform_data();
#endif

	midas_snd_set_mclk(true, false);

	rtd->codec_dai->driver->playback.channels_max =
				rtd->cpu_dai->driver->playback.channels_max;

	ret = snd_soc_add_controls(codec, kona_controls,
					ARRAY_SIZE(kona_controls));

	ret = snd_soc_dapm_new_controls(&codec->dapm, kona_dapm_widgets,
					   ARRAY_SIZE(kona_dapm_widgets));
	if (ret != 0)
		dev_err(codec->dev, "Failed to add DAPM widgets: %d\n", ret);

	ret = snd_soc_dapm_add_routes(&codec->dapm, kona_dapm_routes,
					   ARRAY_SIZE(kona_dapm_routes));
	if (ret != 0)
		dev_err(codec->dev, "Failed to add DAPM routes: %d\n", ret);

	ret = snd_soc_dai_set_sysclk(aif1_dai, WM8994_SYSCLK_MCLK2,
				     MIDAS_DEFAULT_MCLK2, SND_SOC_CLOCK_IN);
	if (ret < 0)
		dev_err(codec->dev, "Failed to boot clocking\n");

	/* Force AIF1CLK on as it will be master for jack detection */
	if (wm8994->revision > 1) {
		ret = snd_soc_dapm_force_enable_pin(&codec->dapm, "AIF1CLK");
		if (ret < 0)
			dev_err(codec->dev, "Failed to enable AIF1CLK: %d\n",
					ret);
	}

	ret = snd_soc_dapm_disable_pin(&codec->dapm, "S5P RP");
	if (ret < 0)
		dev_err(codec->dev, "Failed to disable S5P RP: %d\n", ret);

	snd_soc_dapm_ignore_suspend(&codec->dapm, "RCV");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "SPK");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "HP");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "Headset Mic");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "Sub Mic");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "Main Mic");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1DACDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF2DACDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF3DACDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1ADCDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF2ADCDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF3ADCDAT");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "LINE");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "HDMI");

	wm1811->codec = codec;

	kona_micd_set_rate(codec);

#ifdef CONFIG_SEC_DEV_JACK
	/* By default use idle_bias_off, will override for WM8994 */
	codec->dapm.idle_bias_off = 0;
#else /* CONFIG_SEC_DEV_JACK */
	wm1811->jack.status = 0;

	ret = snd_soc_jack_new(codec, "Kona Jack",
				SND_JACK_HEADSET | SND_JACK_BTN_0 |
				SND_JACK_BTN_1 | SND_JACK_BTN_2,
				&wm1811->jack);

	if (ret < 0)
		dev_err(codec->dev, "Failed to create jack: %d\n", ret);

	ret = snd_jack_set_key(wm1811->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);

	if (ret < 0)
		dev_err(codec->dev, "Failed to set KEY_MEDIA: %d\n", ret);

	ret = snd_jack_set_key(wm1811->jack.jack, SND_JACK_BTN_1,
							KEY_VOLUMEUP);
	if (ret < 0)
		dev_err(codec->dev, "Failed to set KEY_VOLUMEUP: %d\n", ret);

	ret = snd_jack_set_key(wm1811->jack.jack, SND_JACK_BTN_2,
							KEY_VOLUMEDOWN);

	if (ret < 0)
		dev_err(codec->dev, "Failed to set KEY_VOLUMEDOWN: %d\n", ret);

	if (wm8994->revision > 1) {
		dev_info(codec->dev, "wm1811: Rev %c support mic detection\n",
			'A' + wm8994->revision);
#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
#ifdef CONFIG_USE_ADC_DET
		if (sound_pdata->use_jackdet_type) {
			ret = wm8958_mic_detect(codec, &wm1811->jack,
					kona_micdet, wm1811, NULL, NULL);
		} else {
			ret = wm8958_mic_detect(codec, &wm1811->jack, NULL,
				NULL, kona_mic_id, wm1811);
		}
#else
	ret = wm8958_mic_detect(codec, &wm1811->jack, NULL,
				NULL, kona_mic_id, wm1811);
#endif
#else
	ret = wm8958_mic_detect(codec, &wm1811->jack, NULL,
				NULL, kona_mic_id, wm1811);
#endif

		if (ret < 0)
			dev_err(codec->dev, "Failed start detection: %d\n",
				ret);
	} else {
		dev_info(codec->dev, "wm1811: Rev %c doesn't support mic detection\n",
			'A' + wm8994->revision);
		codec->dapm.idle_bias_off = 0;
	}
	/* To wakeup for earjack event in suspend mode */
	enable_irq_wake(control->irq);

	wake_lock_init(&wm1811->jackdet_wake_lock,
					WAKE_LOCK_SUSPEND, "kona_jackdet");

	/* To support PBA function test */
	jack_class = class_create(THIS_MODULE, "audio");

	if (IS_ERR(jack_class))
		pr_err("Failed to create class\n");

	jack_dev = device_create(jack_class, NULL, 0, codec, "earjack");

	if (device_create_file(jack_dev, &dev_attr_select_jack) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_select_jack.attr.name);

	if (device_create_file(jack_dev, &dev_attr_key_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_key_state.attr.name);

	if (device_create_file(jack_dev, &dev_attr_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_state.attr.name);

#endif /* CONFIG_SEC_DEV_JACK */

#ifdef CONFIG_USE_ADC_DET
	pr_info("%s: register adc client\n", __func__);
	wm1811->padc = s3c_adc_register(kona_snd_device, NULL, NULL, 0);
#endif

	return snd_soc_dapm_sync(&codec->dapm);
}

static struct snd_soc_dai_link kona_dai[] = {
	{ /* Sec_Fifo DAI i/f */
		.name = "Sec_FIFO TX",
		.stream_name = "Sec_Dai",
		.cpu_dai_name = "samsung-i2s.4",
		.codec_dai_name = "wm8994-aif1",
#ifndef CONFIG_SND_SOC_SAMSUNG_USE_DMA_WRAPPER
		.platform_name = "samsung-audio-idma",
#else
		.platform_name = "samsung-audio",
#endif
		.codec_name = "wm8994-codec",
		.init = kona_wm1811_init_paiftx,
		.ops = &kona_wm1811_aif1_ops,
	},
	{
		.name = "Kona_WM1811 Voice",
		.stream_name = "Voice Tx/Rx",
		.cpu_dai_name = "kona.cp",
		.codec_dai_name = "wm8994-aif2",
		.platform_name = "snd-soc-dummy",
		.codec_name = "wm8994-codec",
		.ops = &kona_wm1811_aif2_ops,
		.ignore_suspend = 1,
	},
	{
		.name = "Kona_WM1811 BT",
		.stream_name = "BT Tx/Rx",
		.cpu_dai_name = "kona.bt",
		.codec_dai_name = "wm8994-aif3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "wm8994-codec",
		.ops = &kona_wm1811_aif3_ops,
		.ignore_suspend = 1,
	},
	{ /* Primary DAI i/f */
		.name = "WM8994 AIF1",
		.stream_name = "Pri_Dai",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "wm8994-aif1",
		.platform_name = "samsung-audio",
		.codec_name = "wm8994-codec",
		.ops = &kona_wm1811_aif1_ops,
	},
};

static int kona_card_suspend_post(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	struct snd_soc_dai *aif1_dai = card->rtd[0].codec_dai;
	struct snd_soc_dai *aif2_dai = card->rtd[1].codec_dai;
	int ret, reg;

	if (!codec->active) {
		ret = snd_soc_dai_set_sysclk(aif2_dai,
					     WM8994_SYSCLK_MCLK2,
					     MIDAS_DEFAULT_MCLK2,
					     SND_SOC_CLOCK_IN);

		if (ret < 0)
			dev_err(codec->dev, "Unable to switch to MCLK2: %d\n",
				ret);

		ret = snd_soc_dai_set_pll(aif2_dai, WM8994_FLL2, 0, 0, 0);

		if (ret < 0)
			dev_err(codec->dev, "Unable to stop FLL2\n");

		ret = snd_soc_dai_set_sysclk(aif1_dai,
					     WM8994_SYSCLK_MCLK2,
					     MIDAS_DEFAULT_MCLK2,
					     SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(codec->dev, "Unable to switch to MCLK2\n");

		ret = snd_soc_dai_set_pll(aif1_dai, WM8994_FLL1, 0, 0, 0);

		if (ret < 0)
			dev_err(codec->dev, "Unable to stop FLL1\n");

		midas_snd_set_mclk(false, true);

		reg = WM8994_AIF1_TRI;
		snd_soc_update_bits(codec, WM8994_AIF1_MASTER_SLAVE,
						WM8994_AIF1_TRI, reg);

		reg = WM8994_AIF2_TRI;
		snd_soc_update_bits(codec, WM8994_AIF2_MASTER_SLAVE,
						WM8994_AIF2_TRI, reg);
	}

	exynos4_sys_powerdown_xusbxti_control(midas_snd_get_mclk() ? 1 : 0);

	return 0;
}

static int kona_card_resume_pre(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	struct snd_soc_dai *aif1_dai = card->rtd[0].codec_dai;
	int ret, reg;

	midas_snd_set_mclk(true, false);

	/* Switch the FLL */
	ret = snd_soc_dai_set_pll(aif1_dai, WM8994_FLL1,
				  WM8994_FLL_SRC_MCLK1,
				  MIDAS_DEFAULT_MCLK1,
				  MIDAS_DEFAULT_SYNC_CLK);

	if (ret < 0)
		dev_err(aif1_dai->dev, "Unable to start FLL1: %d\n", ret);

	/* Then switch AIF1CLK to it */
	ret = snd_soc_dai_set_sysclk(aif1_dai,
				     WM8994_SYSCLK_FLL1,
				     MIDAS_DEFAULT_SYNC_CLK,
				     SND_SOC_CLOCK_IN);

	if (ret < 0)
		dev_err(aif1_dai->dev, "Unable to switch to FLL1: %d\n", ret);

	/* workaround for jack detection
	* sometimes WM8994_GPIO_1 type changed wrong function type
	* so if type mismatched, update to IRQ type
	*/
	reg = snd_soc_read(codec, WM8994_GPIO_1);
	if ((reg & WM8994_GPN_FN_MASK) != WM8994_GP_FN_IRQ) {
		dev_err(codec->dev, "%s: GPIO1 type 0x%x\n", __func__, reg);
		snd_soc_write(codec, WM8994_GPIO_1, WM8994_GP_FN_IRQ);
	}

	return 0;
}

static struct snd_soc_card kona = {
	.name = "Kona_WM1811",
	.dai_link = kona_dai,
	.num_links = ARRAY_SIZE(kona_dai),
	.suspend_post = kona_card_suspend_post,
	.resume_pre = kona_card_resume_pre,
};

static int __init kona_audio_init(void)
{
	struct wm1811_machine_priv *wm1811;
#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	const struct exynos_sound_platform_data *sound_pdata;
#endif
	int ret;

	wm1811 = kzalloc(sizeof *wm1811, GFP_KERNEL);
	if (!wm1811) {
		pr_err("Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_kzalloc;
	}
	snd_soc_card_set_drvdata(&kona, wm1811);

	kona_snd_device = platform_device_alloc("soc-audio", -1);
	if (!kona_snd_device) {
		ret = -ENOMEM;
		goto err_device_alloc;
	}

	ret = snd_soc_register_dais(&kona_snd_device->dev, kona_ext_dai,
						ARRAY_SIZE(kona_ext_dai));
	if (ret != 0)
		pr_err("Failed to register external DAIs: %d\n", ret);

	platform_set_drvdata(kona_snd_device, &kona);

	ret = platform_device_add(kona_snd_device);
	if (ret)
		platform_device_put(kona_snd_device);

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	sound_pdata = exynos_sound_get_platform_data();
	if (!sound_pdata)
		pr_info("%s: don't use sound pdata\n", __func__);
#ifdef CONFIG_USE_ADC_DET
	if (sound_pdata->zones) {
		wm1811->zones = sound_pdata->zones;
		wm1811->num_zones = sound_pdata->num_zones;
	}
	pr_info("%s:use_jackdet_type = %d\n", __func__,
			sound_pdata->use_jackdet_type);
	wm1811->use_jackdet_type = sound_pdata->use_jackdet_type;
#endif
#endif

	kona_gpio_init();

	return ret;

err_device_alloc:
	kfree(wm1811);
err_kzalloc:
	return ret;
}
module_init(kona_audio_init);

static void __exit kona_audio_exit(void)
{
	struct snd_soc_card *card = &kona;
	struct wm1811_machine_priv *wm1811 = snd_soc_card_get_drvdata(card);
#ifdef CONFIG_USE_ADC_DET
	s3c_adc_release(wm1811->padc);
#endif
	platform_device_unregister(kona_snd_device);
	kfree(wm1811);
}
module_exit(kona_audio_exit);

MODULE_AUTHOR("Quartz.Jang <quartz.jang@samsung.com");
MODULE_DESCRIPTION("ALSA SoC Kona WM1811");
MODULE_LICENSE("GPL");
