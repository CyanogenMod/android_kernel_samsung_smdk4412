/*
 * MC-1N2 ASoC codec driver
 *
 * Copyright (c) 2010-2011 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <sound/hwdep.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include "mc1n2.h"
#include "mc1n2_priv.h"

#include <plat/gpio-cfg.h>
#include <mach/gpio.h>

#ifdef CONFIG_TARGET_LOCALE_NAATT_TEMP
/* CONFIG_TARGET_LOCALE_NAATT_TEMP is intentionally introduced temporarily*/

#include "mc1n2_cfg_gsm.h"
#elif defined(CONFIG_MACH_Q1_BD)
#include "mc1n2_cfg_q1.h"
#elif defined(CONFIG_MACH_U1_KOR_LGT)
#include "mc1n2_cfg_lgt.h"
#elif defined(CONFIG_MACH_PX)
#include "mc1n2_cfg_px.h"
#else
#include "mc1n2_cfg.h"
#endif

extern int mc1n2_set_mclk_source(bool on);
static int audio_ctrl_mic_bias_gpio(struct mc1n2_platform_data *pdata, int mic, bool on);

#define MC1N2_DRIVER_VERSION "1.0.4"

#define MC1N2_NAME "mc1n2"

#define MC1N2_I2S_RATE (SNDRV_PCM_RATE_8000_48000)
#define MC1N2_I2S_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
	 SNDRV_PCM_FMTBIT_S24_3LE)

#define MC1N2_PCM_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)
#define MC1N2_PCM_FORMATS \
	(SNDRV_PCM_FMTBIT_S8 | \
	 SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_A_LAW | SNDRV_PCM_FMTBIT_MU_LAW)

#define MC1N2_HWDEP_ID "mc1n2"

#define MC1N2_HW_ID_AA 0x78
#define MC1N2_HW_ID_AB 0x79

#define MC1N2_WAITTIME_MICIN	100

#ifdef ALSA_VER_ANDROID_3_0
static struct i2c_client *mc1n2_i2c;
#endif

/*
 * Driver private data structure
 */
static UINT8 mc1n2_hwid;

static size_t mc1n2_path_channel_tbl[] = {
	offsetof(MCDRV_PATH_INFO, asDit0[0]),
	offsetof(MCDRV_PATH_INFO, asDit1[0]),
	offsetof(MCDRV_PATH_INFO, asDit2[0]),
	offsetof(MCDRV_PATH_INFO, asHpOut[0]),
	offsetof(MCDRV_PATH_INFO, asHpOut[1]),
	offsetof(MCDRV_PATH_INFO, asSpOut[0]),
	offsetof(MCDRV_PATH_INFO, asSpOut[1]),
	offsetof(MCDRV_PATH_INFO, asRcOut[0]),
	offsetof(MCDRV_PATH_INFO, asLout1[0]),
	offsetof(MCDRV_PATH_INFO, asLout1[1]),
	offsetof(MCDRV_PATH_INFO, asLout2[0]),
	offsetof(MCDRV_PATH_INFO, asLout2[1]),
	offsetof(MCDRV_PATH_INFO, asDac[0]),
	offsetof(MCDRV_PATH_INFO, asDac[1]),
	offsetof(MCDRV_PATH_INFO, asAe[0]),
	offsetof(MCDRV_PATH_INFO, asAdc0[0]),
	offsetof(MCDRV_PATH_INFO, asAdc0[1]),
	offsetof(MCDRV_PATH_INFO, asMix[0]),
	offsetof(MCDRV_PATH_INFO, asBias[0]),
};
#define MC1N2_N_PATH_CHANNELS (sizeof(mc1n2_path_channel_tbl) / sizeof(size_t))

struct mc1n2_port_params {
	UINT8 rate;
	UINT8 bits[SNDRV_PCM_STREAM_LAST+1];
	UINT8 pcm_mono[SNDRV_PCM_STREAM_LAST+1];
	UINT8 pcm_order[SNDRV_PCM_STREAM_LAST+1];
	UINT8 pcm_law[SNDRV_PCM_STREAM_LAST+1];
	UINT8 master;
	UINT8 inv;
	UINT8 format;
	UINT8 bckfs;
	UINT8 pcm_clkdown;
	UINT8 channels;
	UINT8 stream;                     /* bit0: Playback, bit1: Capture */
	UINT8 dir[MC1N2_N_PATH_CHANNELS]; /* path settings for DIR */
	MCDRV_CHANNEL dit;                /* path settings for DIT */
};

struct mc1n2_data {
	struct mutex mutex;
	struct mc1n2_setup setup;
	struct mc1n2_port_params port[IOPORT_NUM];
	struct snd_hwdep *hwdep;
	struct mc1n2_platform_data *pdata;
	int clk_update;
	MCDRV_PATH_INFO path_store;
	MCDRV_VOL_INFO vol_store;
	MCDRV_DIO_INFO dio_store;
	MCDRV_DAC_INFO dac_store;
	MCDRV_ADC_INFO adc_store;
	MCDRV_SP_INFO sp_store;
	MCDRV_DNG_INFO dng_store;
	MCDRV_SYSEQ_INFO syseq_store;
	MCDRV_AE_INFO ae_store;
	MCDRV_PDM_INFO pdm_store;
	UINT32 hdmicount;
	UINT32 delay_mic1in;
	UINT32 lineoutenable;
};

struct mc1n2_info_store {
	UINT32 get;
	UINT32 set;
	size_t offset;
	UINT32 flags;
};

struct mc1n2_info_store mc1n2_info_store_tbl[] = {
	{MCDRV_GET_DIGITALIO, MCDRV_SET_DIGITALIO,
	 offsetof(struct mc1n2_data, dio_store), 0x1ff},
	{MCDRV_GET_DAC, MCDRV_SET_DAC,
	 offsetof(struct mc1n2_data, dac_store), 0x7},
	{MCDRV_GET_ADC, MCDRV_SET_ADC,
	 offsetof(struct mc1n2_data, adc_store), 0x7},
	{MCDRV_GET_SP, MCDRV_SET_SP,
	 offsetof(struct mc1n2_data, sp_store), 0},
	{MCDRV_GET_DNG, MCDRV_SET_DNG,
	 offsetof(struct mc1n2_data, dng_store), 0x3f3f3f},
	{MCDRV_GET_SYSEQ, MCDRV_SET_SYSEQ,
	 offsetof(struct mc1n2_data, syseq_store), 0x3},
	{0, MCDRV_SET_AUDIOENGINE,
	 offsetof(struct mc1n2_data, ae_store), 0x1ff},
	{MCDRV_GET_PDM, MCDRV_SET_PDM,
	 offsetof(struct mc1n2_data, pdm_store), 0x7f},
	{MCDRV_GET_PATH, MCDRV_SET_PATH,
	 offsetof(struct mc1n2_data, path_store), 0},
	{MCDRV_GET_VOLUME, MCDRV_SET_VOLUME,
	 offsetof(struct mc1n2_data, vol_store), 0},
};
#define MC1N2_N_INFO_STORE (sizeof(mc1n2_info_store_tbl) / sizeof(struct mc1n2_info_store))

#define mc1n2_is_in_playback(p) ((p)->stream & (1 << SNDRV_PCM_STREAM_PLAYBACK))
#define mc1n2_is_in_capture(p)  ((p)->stream & (1 << SNDRV_PCM_STREAM_CAPTURE))
#define get_port_id(id) (id-1)

static int mc1n2_current_mode;

#ifndef ALSA_VER_ANDROID_3_0
static struct snd_soc_codec *mc1n2_codec;
#endif

#ifndef ALSA_VER_ANDROID_3_0
static struct snd_soc_codec *mc1n2_get_codec_data(void)
{
	return mc1n2_codec;
}

static void mc1n2_set_codec_data(struct snd_soc_codec *codec)
{
	mc1n2_codec = codec;
}
#endif

/* deliver i2c access to machdep */
struct i2c_client *mc1n2_get_i2c_client(void)
{
#ifdef ALSA_VER_ANDROID_3_0
	return mc1n2_i2c;
#else
	return mc1n2_codec->control_data;
#endif
}

static int audio_ctrl_mic_bias_gpio(struct mc1n2_platform_data *pdata, int mic, bool on)
{
	if (!pdata) {
		pr_err("failed to control mic bias\n");
		return -EINVAL;
	}

	if ((mic & MAIN_MIC) && (pdata->set_main_mic_bias != NULL))
		pdata->set_main_mic_bias(on);

	if ((mic & SUB_MIC) && (pdata->set_sub_mic_bias != NULL))
		pdata->set_sub_mic_bias(on);

	return 0;
}

/*
 * DAI (PCM interface)
 */
/* SRC_RATE settings @ 73728000Hz (ideal PLL output) */
static int mc1n2_src_rate[][SNDRV_PCM_STREAM_LAST+1] = {
	/* DIR, DIT */
	{32768, 4096},                  /* MCDRV_FS_48000 */
	{30106, 4458},                  /* MCDRV_FS_44100 */
	{21845, 6144},                  /* MCDRV_FS_32000 */
	{0, 0},                         /* N/A */
	{0, 0},                         /* N/A */
	{15053, 8916},                  /* MCDRV_FS_22050 */
	{10923, 12288},                 /* MCDRV_FS_16000 */
	{0, 0},                         /* N/A */
	{0, 0},                         /* N/A */
	{7526, 17833},                  /* MCDRV_FS_11025 */
	{5461, 24576},                  /* MCDRV_FS_8000 */
};

#define mc1n2_fs_to_srcrate(rate,dir) mc1n2_src_rate[(rate)][(dir)];

static int mc1n2_setup_dai(struct mc1n2_data *mc1n2, int id, int mode, int dir)
{
	MCDRV_DIO_INFO dio;
	MCDRV_DIO_PORT *port = &dio.asPortInfo[id];
	struct mc1n2_setup *setup = &mc1n2->setup;
	struct mc1n2_port_params *par = &mc1n2->port[id];
	UINT32 update = 0;
	int i;

	memset(&dio, 0, sizeof(MCDRV_DIO_INFO));

	if (par->stream == 0) {
		port->sDioCommon.bMasterSlave = par->master;
		port->sDioCommon.bAutoFs = MCDRV_AUTOFS_OFF;
		port->sDioCommon.bFs = par->rate;
		port->sDioCommon.bBckFs = par->bckfs;
		port->sDioCommon.bInterface = mode;
		port->sDioCommon.bBckInvert = par->inv;
		if (mode == MCDRV_DIO_PCM) {
			port->sDioCommon.bPcmHizTim = setup->pcm_hiz_redge[id];
			port->sDioCommon.bPcmClkDown = par->pcm_clkdown;
			port->sDioCommon.bPcmFrame = par->format;
			port->sDioCommon.bPcmHighPeriod = setup->pcm_hperiod[id];
		}
		update |= MCDRV_DIO0_COM_UPDATE_FLAG;
	}

	if (dir == SNDRV_PCM_STREAM_PLAYBACK) {
		port->sDir.wSrcRate = mc1n2_fs_to_srcrate(par->rate, dir);
		if (mode == MCDRV_DIO_DA) {
			port->sDir.sDaFormat.bBitSel = par->bits[dir];
			port->sDir.sDaFormat.bMode = par->format;
		} else {
			port->sDir.sPcmFormat.bMono = par->pcm_mono[dir];
			port->sDir.sPcmFormat.bOrder = par->pcm_order[dir];
			if (setup->pcm_extend[id]) {
				port->sDir.sPcmFormat.bOrder |=
					(1 << setup->pcm_extend[id]);
			}
			port->sDir.sPcmFormat.bLaw = par->pcm_law[dir];
			port->sDir.sPcmFormat.bBitSel = par->bits[dir];
		}
		for (i = 0; i < DIO_CHANNELS; i++) {
			if (i && par->channels == 1) {
				port->sDir.abSlot[i] = port->sDir.abSlot[i-1];
			} else {
				port->sDir.abSlot[i] = setup->slot[id][dir][i];
			}

		}
		update |= MCDRV_DIO0_DIR_UPDATE_FLAG;
	}

	if (dir == SNDRV_PCM_STREAM_CAPTURE) {
		port->sDit.wSrcRate = mc1n2_fs_to_srcrate(par->rate, dir);
		if (mode == MCDRV_DIO_DA) {
			port->sDit.sDaFormat.bBitSel = par->bits[dir];
			port->sDit.sDaFormat.bMode = par->format;
		} else {
			port->sDit.sPcmFormat.bMono = par->pcm_mono[dir];
			port->sDit.sPcmFormat.bOrder = par->pcm_order[dir];
			if (setup->pcm_extend[id]) {
				port->sDit.sPcmFormat.bOrder |=
					(1 << setup->pcm_extend[id]);
			}
			port->sDit.sPcmFormat.bLaw = par->pcm_law[dir];
			port->sDit.sPcmFormat.bBitSel = par->bits[dir];
		}
		for (i = 0; i < DIO_CHANNELS; i++) {
			port->sDit.abSlot[i] = setup->slot[id][dir][i];
		}
		update |= MCDRV_DIO0_DIT_UPDATE_FLAG;
	}

	return _McDrv_Ctrl(MCDRV_SET_DIGITALIO, &dio, update << (id*3));
}

static int mc1n2_control_dir(struct mc1n2_data *mc1n2, int id, int enable)
{
	MCDRV_PATH_INFO info;
	MCDRV_CHANNEL *ch;
	int activate;
	int i;

	memset(&info, 0, sizeof(MCDRV_PATH_INFO));

	for (i = 0; i < MC1N2_N_PATH_CHANNELS; i++) {
		ch = (MCDRV_CHANNEL *)((void *)&info + mc1n2_path_channel_tbl[i]);

		switch (i) {
		case 0:
#ifdef DIO0_DAI_ENABLE
			activate = enable && mc1n2_is_in_capture(&mc1n2->port[0]);
#else
			activate = enable;
#endif
			break;

		case 1:
#ifdef DIO1_DAI_ENABLE
			activate = enable && mc1n2_is_in_capture(&mc1n2->port[1]);
#else
			activate = enable;
#endif
			break;
		case 2:
#ifdef DIO2_DAI_ENABLE
			activate = enable && mc1n2_is_in_capture(&mc1n2->port[2]);
#else
			activate = enable;
#endif
			break;
		default:
			activate = enable;
			break;
		}

		if (mc1n2->port[id].dir[i]) {
			ch->abSrcOnOff[3] = 0x1 << (id * 2 + !activate);
		}
	}

	return _McDrv_Ctrl(MCDRV_SET_PATH, &info, 0);
}

static int mc1n2_control_dit(struct mc1n2_data *mc1n2, int id, int enable)
{
	MCDRV_PATH_INFO info;
	MCDRV_CHANNEL *ch = info.asDit0 + id;
	int stream;
	int i;

	memset(&info, 0, sizeof(MCDRV_PATH_INFO));

	for (i = 0; i < SOURCE_BLOCK_NUM; i++) {
		if (i == 3) {
			stream = 0;

#ifdef DIO0_DAI_ENABLE
			stream |= mc1n2_is_in_playback(&mc1n2->port[0]);
#endif
#ifdef DIO1_DAI_ENABLE
			stream |= mc1n2_is_in_playback(&mc1n2->port[1]) << 2;
#endif
#ifdef DIO2_DAI_ENABLE
			stream |= mc1n2_is_in_playback(&mc1n2->port[2]) << 4;
#endif

			ch->abSrcOnOff[3] = (stream & mc1n2->port[id].dit.abSrcOnOff[3]) << !enable;
		} else {
			ch->abSrcOnOff[i] = mc1n2->port[id].dit.abSrcOnOff[i] << !enable;
		}
	}

	return _McDrv_Ctrl(MCDRV_SET_PATH, &info, 0);
}

static int mc1n2_update_clock(struct mc1n2_data *mc1n2)
{
	MCDRV_CLOCK_INFO info;

	memset(&info, 0, sizeof(MCDRV_CLOCK_INFO));
	info.bCkSel = mc1n2->setup.init.bCkSel;
	info.bDivR0 = mc1n2->setup.init.bDivR0;
	info.bDivF0 = mc1n2->setup.init.bDivF0;
	info.bDivR1 = mc1n2->setup.init.bDivR1;
	info.bDivF1 = mc1n2->setup.init.bDivF1;

	return _McDrv_Ctrl(MCDRV_UPDATE_CLOCK, &info, 0);
}

static int mc1n2_set_clkdiv_common(struct mc1n2_data *mc1n2, int div_id, int div)
{
	struct mc1n2_setup *setup = &mc1n2->setup;

	switch (div_id) {
	case MC1N2_CKSEL:
		switch (div) {
		case 0:
			setup->init.bCkSel = MCDRV_CKSEL_CMOS;
			break;
		case 1:
			setup->init.bCkSel = MCDRV_CKSEL_TCXO;
			break;
		case 2:
			setup->init.bCkSel = MCDRV_CKSEL_CMOS_TCXO;
			break;
		case 3:
			setup->init.bCkSel = MCDRV_CKSEL_TCXO_CMOS;
			break;
		default:
			return -EINVAL;
		}
		break;
	case MC1N2_DIVR0:
		if ((div < 1) || (div > 127)) {
			return -EINVAL;
		}
		setup->init.bDivR0 = div;
		break;
	case MC1N2_DIVF0:
		if ((div < 1) || (div > 255)) {
			return -EINVAL;
		}
		setup->init.bDivF0 = div;
		break;
	case MC1N2_DIVR1:
		if ((div < 1) || (div > 127)) {
			return -EINVAL;
		}
		setup->init.bDivR1 = div;
		break;
	case MC1N2_DIVF1:
		if ((div < 1) || (div > 255)) {
			return -EINVAL;
		}
		setup->init.bDivF1 = div;
		break;
	default:
		return -EINVAL;
	}

	mc1n2->clk_update = 1;

	return 0;
}

static int mc1n2_set_fmt_common(struct mc1n2_port_params *port, unsigned int fmt)
{
	/* master */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		port->master = MCDRV_DIO_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		port->master = MCDRV_DIO_SLAVE;
		break;
	default:
		return -EINVAL;
	}

	/* inv */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		port->inv = MCDRV_BCLK_NORMAL;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		port->inv = MCDRV_BCLK_INVERT;
		break;
	default:
		return -EINVAL;
	}

#ifdef ALSA_VER_1_0_19
	/* clock */
	switch (fmt & SND_SOC_DAIFMT_CLOCK_MASK) {
	case SND_SOC_DAIFMT_SYNC:
		/* just validating */
		break;
	default:
		return -EINVAL;
	}
#endif

	return 0;
}

static int mc1n2_i2s_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct snd_soc_codec *codec = dai->codec;
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];

	switch (div_id) {
	case MC1N2_BCLK_MULT:
		switch (div) {
		case MC1N2_LRCK_X32:
			port->bckfs = MCDRV_BCKFS_32;
			break;
		case MC1N2_LRCK_X48:
			port->bckfs = MCDRV_BCKFS_48;
			break;
		case MC1N2_LRCK_X64:
			port->bckfs = MCDRV_BCKFS_64;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return mc1n2_set_clkdiv_common(mc1n2, div_id, div);
	}

	return 0;
}

static int mc1n2_i2s_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];

	/* format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		port->format = MCDRV_DAMODE_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		port->format = MCDRV_DAMODE_TAILALIGN;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		port->format = MCDRV_DAMODE_HEADALIGN;
		break;
	default:
		return -EINVAL;
	}

	return mc1n2_set_fmt_common(port, fmt);
}

static int mc1n2_i2s_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
#ifdef ALSA_VER_ANDROID_3_0
	struct snd_soc_codec *codec = dai->codec;
#else
	struct snd_soc_pcm_runtime *runtime = snd_pcm_substream_chip(substream);
#ifdef ALSA_VER_1_0_19
	struct snd_soc_codec *codec = runtime->socdev->codec;
#else
	struct snd_soc_codec *codec = runtime->socdev->card->codec;
#endif
#endif
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];
	int dir = substream->stream;
	int rate;
	int err = 0;

	dbg_info("hw_params: [%d] name=%s, dir=%d, rate=%d, bits=%d, ch=%d\n",
		 get_port_id(dai->id), substream->name, dir,
		 params_rate(params), params_format(params), params_channels(params));

	/* format (bits) */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		port->bits[dir] = MCDRV_BITSEL_16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		port->bits[dir] = MCDRV_BITSEL_20;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		port->bits[dir] = MCDRV_BITSEL_24;
		break;
	default:
		return -EINVAL;
	}

	/* rate */
	switch (params_rate(params)) {
	case 8000:
		rate = MCDRV_FS_8000;
		break;
	case 11025:
		rate = MCDRV_FS_11025;
		break;
	case 16000:
		rate = MCDRV_FS_16000;
		break;
	case 22050:
		rate = MCDRV_FS_22050;
		break;
	case 32000:
		rate = MCDRV_FS_32000;
		break;
	case 44100:
		rate = MCDRV_FS_44100;
		break;
	case 48000:
		rate = MCDRV_FS_48000;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&mc1n2->mutex);

	if ((port->stream & ~(1 << dir)) && (rate != port->rate)) {
		err = -EBUSY;
		goto error;
	}

#ifdef CONFIG_SND_SAMSUNG_RP
	if ((dir == SNDRV_PCM_STREAM_PLAYBACK) && (get_port_id(dai->id) == 0)
		&& (port->stream & (1 << dir)) && (rate == port->rate)) {
		/* During ULP Audio, DAI should not be touched
		   if i2s port already opened. */
		err = 0;
		goto error;
	}
#endif

/* Because of line out pop up noise issue, i2s port already opend */
	if ((mc1n2->lineoutenable == 1) && (port->stream & (1 << dir))) {
		err = 0;
		goto error;
	}

	port->rate = rate;
	port->channels = params_channels(params);

	err = mc1n2_update_clock(mc1n2);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_update_clock\n", err);
		err = -EIO;
		goto error;
	}

	err = mc1n2_setup_dai(mc1n2, get_port_id(dai->id), MCDRV_DIO_DA, dir);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_setup_dai\n", err);
		err = -EIO;
		goto error;
	}

	if (dir == SNDRV_PCM_STREAM_PLAYBACK) {
		err = mc1n2_control_dir(mc1n2, get_port_id(dai->id), 1);
	} else {
		err = mc1n2_control_dit(mc1n2, get_port_id(dai->id), 1);
	}
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_control_dir/dit\n", err);
		err = -EIO;
		goto error;
	}

	port->stream |= (1 << dir);

error:
	mutex_unlock(&mc1n2->mutex);

	return err;
}

static int mc1n2_hw_free(struct snd_pcm_substream *substream,
			 struct snd_soc_dai *dai)
{
#ifdef ALSA_VER_ANDROID_3_0
	struct snd_soc_codec *codec = dai->codec;
#else
	struct snd_soc_pcm_runtime *runtime = snd_pcm_substream_chip(substream);
#ifdef ALSA_VER_1_0_19
	struct snd_soc_codec *codec = runtime->socdev->codec;
#else
	struct snd_soc_codec *codec = runtime->socdev->card->codec;
#endif
#endif
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];
	int dir = substream->stream;
	int err;

	mutex_lock(&mc1n2->mutex);

	if (!(port->stream & (1 << dir))) {
		err = 0;
		goto error;
	}

#ifdef CONFIG_SND_SAMSUNG_RP
	if ((dir == SNDRV_PCM_STREAM_PLAYBACK) && (get_port_id(dai->id) == 0)) {
		/* Leave codec opened during ULP Audio */
		err = 0;
		goto error;
	}
#endif

/* Because of line out pop up noise, leave codec opened */
	if (mc1n2->lineoutenable == 1) {
		err = 0;
		goto error;
	}

	if (dir == SNDRV_PCM_STREAM_PLAYBACK) {
		err = mc1n2_control_dir(mc1n2, get_port_id(dai->id), 0);
	} else {
		err = mc1n2_control_dit(mc1n2, get_port_id(dai->id), 0);
	}
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_control_dir/dit\n", err);
		err = -EIO;
		goto error;
	}

	port->stream &= ~(1 << dir);

error:
	mutex_unlock(&mc1n2->mutex);

	return err;
}

static int mc1n2_pcm_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct snd_soc_codec *codec = dai->codec;
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];

	switch (div_id) {
	case MC1N2_BCLK_MULT:
		switch (div) {
		case MC1N2_LRCK_X8:
			port->bckfs = MCDRV_BCKFS_16;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_HALF;
			break;
		case MC1N2_LRCK_X16:
			port->bckfs = MCDRV_BCKFS_16;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X24:
			port->bckfs = MCDRV_BCKFS_48;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_HALF;
			break;
		case MC1N2_LRCK_X32:
			port->bckfs = MCDRV_BCKFS_32;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X48:
			port->bckfs = MCDRV_BCKFS_48;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X64:
			port->bckfs = MCDRV_BCKFS_64;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X128:
			port->bckfs = MCDRV_BCKFS_128;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X256:
			port->bckfs = MCDRV_BCKFS_256;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		case MC1N2_LRCK_X512:
			port->bckfs = MCDRV_BCKFS_512;
			port->pcm_clkdown = MCDRV_PCM_CLKDOWN_OFF;
			break;
		}
		break;
	default:
		return mc1n2_set_clkdiv_common(mc1n2, div_id, div);
	}

	return 0;
}

static int mc1n2_pcm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];

	/* format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		port->format = MCDRV_PCM_SHORTFRAME;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		port->format = MCDRV_PCM_LONGFRAME;
		break;
	default:
		return -EINVAL;
	}

	return mc1n2_set_fmt_common(port, fmt);
}

static int mc1n2_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
#ifdef ALSA_VER_ANDROID_3_0
	struct snd_soc_codec *codec = dai->codec;
#else
	struct snd_soc_pcm_runtime *runtime = snd_pcm_substream_chip(substream);
#ifdef ALSA_VER_1_0_19
	struct snd_soc_codec *codec = runtime->socdev->codec;
#else
	struct snd_soc_codec *codec = runtime->socdev->card->codec;
#endif
#endif
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_port_params *port = &mc1n2->port[get_port_id(dai->id)];
	int dir = substream->stream;
	int rate;
	int err;

	dbg_info("hw_params: [%d] name=%s, dir=%d, rate=%d, bits=%d, ch=%d\n",
		 get_port_id(dai->id), substream->name, dir,
		 params_rate(params), params_format(params), params_channels(params));

	/* channels */
	switch (params_channels(params)) {
	case 1:
		port->pcm_mono[dir] = MCDRV_PCM_MONO;
		break;
	case 2:
		port->pcm_mono[dir] = MCDRV_PCM_STEREO;
		break;
	default:
		return -EINVAL;
	}

	/* format (bits) */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		port->bits[dir] = MCDRV_PCM_BITSEL_8;
		port->pcm_order[dir] = MCDRV_PCM_MSB_FIRST;
		port->pcm_law[dir] = MCDRV_PCM_LINEAR;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		port->bits[dir] = MCDRV_PCM_BITSEL_16;
		port->pcm_order[dir] = MCDRV_PCM_LSB_FIRST;
		port->pcm_law[dir] = MCDRV_PCM_LINEAR;
		break;
	case SNDRV_PCM_FORMAT_S16_BE:
		port->bits[dir] = MCDRV_PCM_BITSEL_16;
		port->pcm_order[dir] = MCDRV_PCM_MSB_FIRST;
		port->pcm_law[dir] = MCDRV_PCM_LINEAR;
		break;
	case SNDRV_PCM_FORMAT_A_LAW:
		port->bits[dir] = MCDRV_PCM_BITSEL_8;
		port->pcm_order[dir] = MCDRV_PCM_MSB_FIRST;
		port->pcm_law[dir] = MCDRV_PCM_ALAW;
		break;
	case SNDRV_PCM_FORMAT_MU_LAW:
		port->bits[dir] = MCDRV_PCM_BITSEL_8;
		port->pcm_order[dir] = MCDRV_PCM_MSB_FIRST;
		port->pcm_law[dir] = MCDRV_PCM_MULAW;
		break;
	default:
		return -EINVAL;
	}

	/* rate */
	switch (params_rate(params)) {
	case 8000:
		rate = MCDRV_FS_8000;
		break;
	case 16000:
		rate = MCDRV_FS_16000;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&mc1n2->mutex);

	if ((port->stream & ~(1 << dir)) && (rate != port->rate)) {
		err = -EBUSY;
		goto error;
	}

	port->rate = rate;
	port->channels = params_channels(params);

	err = mc1n2_update_clock(mc1n2);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_update_clock\n", err);
		err = -EIO;
		goto error;
	}

	err = mc1n2_setup_dai(mc1n2, get_port_id(dai->id), MCDRV_DIO_PCM, dir);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_setup_dai\n", err);
		err = -EIO;
		goto error;
	}

	if (dir == SNDRV_PCM_STREAM_PLAYBACK) {
		err = mc1n2_control_dir(mc1n2, get_port_id(dai->id), 1);
	} else {
		err = mc1n2_control_dit(mc1n2, get_port_id(dai->id), 1);
	}
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in mc1n2_control_dir/dit\n", err);
		err = -EIO;
		goto error;
	}

	port->stream |= (1 << dir);

error:
	mutex_unlock(&mc1n2->mutex);

	return err;
}

#ifndef ALSA_VER_1_0_19
static struct snd_soc_dai_ops mc1n2_dai_ops[] = {
	{
		.set_clkdiv = mc1n2_i2s_set_clkdiv,
		.set_fmt = mc1n2_i2s_set_fmt,
		.hw_params = mc1n2_i2s_hw_params,
		.hw_free = mc1n2_hw_free,
	},
	{
		.set_clkdiv = mc1n2_pcm_set_clkdiv,
		.set_fmt = mc1n2_pcm_set_fmt,
		.hw_params = mc1n2_pcm_hw_params,
		.hw_free = mc1n2_hw_free,
	},
	{
		.set_clkdiv = mc1n2_i2s_set_clkdiv,
		.set_fmt = mc1n2_i2s_set_fmt,
		.hw_params = mc1n2_i2s_hw_params,
		.hw_free = mc1n2_hw_free,
	},
	{
		.set_clkdiv = mc1n2_pcm_set_clkdiv,
		.set_fmt = mc1n2_pcm_set_fmt,
		.hw_params = mc1n2_pcm_hw_params,
		.hw_free = mc1n2_hw_free,
	},
	{
		.set_clkdiv = mc1n2_i2s_set_clkdiv,
		.set_fmt = mc1n2_i2s_set_fmt,
		.hw_params = mc1n2_i2s_hw_params,
		.hw_free = mc1n2_hw_free,
	},
	{
		.set_clkdiv = mc1n2_pcm_set_clkdiv,
		.set_fmt = mc1n2_pcm_set_fmt,
		.hw_params = mc1n2_pcm_hw_params,
		.hw_free = mc1n2_hw_free,
	}
};
#endif


#ifdef ALSA_VER_ANDROID_3_0
struct snd_soc_dai_driver mc1n2_dai[] = {
#else
struct snd_soc_dai mc1n2_dai[] = {
#endif
	{
		.name = MC1N2_NAME "-da0i",
		.id = 1,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_i2s_set_clkdiv,
			.set_fmt = mc1n2_i2s_set_fmt,
			.hw_params = mc1n2_i2s_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[0]
#endif
	},
	{
		.name = MC1N2_NAME "-da0p",
		.id = 1,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_pcm_set_clkdiv,
			.set_fmt = mc1n2_pcm_set_fmt,
			.hw_params = mc1n2_pcm_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[1]
#endif
	},
	{
		.name = MC1N2_NAME "-da1i",
		.id = 2,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_i2s_set_clkdiv,
			.set_fmt = mc1n2_i2s_set_fmt,
			.hw_params = mc1n2_i2s_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[2]
#endif
	},
	{
		.name = MC1N2_NAME "-da1p",
		.id = 2,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_pcm_set_clkdiv,
			.set_fmt = mc1n2_pcm_set_fmt,
			.hw_params = mc1n2_pcm_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[3]
#endif
	},
	{
		.name = MC1N2_NAME "-da2i",
		.id = 3,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_I2S_RATE,
			.formats = MC1N2_I2S_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_i2s_set_clkdiv,
			.set_fmt = mc1n2_i2s_set_fmt,
			.hw_params = mc1n2_i2s_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[4]
#endif
	},
	{
		.name = MC1N2_NAME "-da2p",
		.id = 3,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MC1N2_PCM_RATE,
			.formats = MC1N2_PCM_FORMATS,
		},
#ifdef ALSA_VER_1_0_19
		.ops = {
			.set_clkdiv = mc1n2_pcm_set_clkdiv,
			.set_fmt = mc1n2_pcm_set_fmt,
			.hw_params = mc1n2_pcm_hw_params,
			.hw_free = mc1n2_hw_free,
		}
#else
		.ops = &mc1n2_dai_ops[5]
#endif
	},
};
#ifndef ALSA_VER_ANDROID_3_0
EXPORT_SYMBOL_GPL(mc1n2_dai);
#endif

/*
 * Control interface
 */
/*
 * Virtual register
 *
 * 16bit software registers are implemented for volumes and mute
 * switches (as an exception, no mute switches for MIC and HP gain).
 * Register contents are stored in codec's register cache.
 *
 *   15  14                       8   7   6                       0
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *  |swR|          volume-R         |swL|          volume-L         |
 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 */
struct mc1n2_vreg_info {
	size_t offset;
	SINT16 *volmap;
};

/* volmap for Digital Volumes */
static SINT16 mc1n2_vol_digital[] = {
	0xa000, 0xb600, 0xb700, 0xb800, 0xb900, 0xba00, 0xbb00, 0xbc00,
	0xbd00, 0xbe00, 0xbf00, 0xc000, 0xc100, 0xc200, 0xc300, 0xc400,
	0xc500, 0xc600, 0xc700, 0xc800, 0xc900, 0xca00, 0xcb00, 0xcc00,
	0xcd00, 0xce00, 0xcf00, 0xd000, 0xd100, 0xd200, 0xd300, 0xd400,
	0xd500, 0xd600, 0xd700, 0xd800, 0xd900, 0xda00, 0xdb00, 0xdc00,
	0xdd00, 0xde00, 0xdf00, 0xe000, 0xe100, 0xe200, 0xe300, 0xe400,
	0xe500, 0xe600, 0xe700, 0xe800, 0xe900, 0xea00, 0xeb00, 0xec00,
	0xed00, 0xee00, 0xef00, 0xf000, 0xf100, 0xf200, 0xf300, 0xf400,
	0xf500, 0xf600, 0xf700, 0xf800, 0xf900, 0xfa00, 0xfb00, 0xfc00,
	0xfd00, 0xfe00, 0xff00, 0x0000, 0x0100, 0x0200, 0x0300, 0x0400,
	0x0500, 0x0600, 0x0700, 0x0800, 0x0900, 0x0a00, 0x0b00, 0x0c00,
	0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100, 0x1200,
};

/* volmap for ADC Analog Volume */
static SINT16 mc1n2_vol_adc[] = {
	0xa000, 0xe500, 0xe680, 0xe800, 0xe980, 0xeb00, 0xec80, 0xee00,
	0xef80, 0xf100, 0xf280, 0xf400, 0xf580, 0xf700, 0xf880, 0xfa00,
	0xfb80, 0xfd00, 0xfe80, 0x0000, 0x0180, 0x0300, 0x0480, 0x0600,
	0x0780, 0x0900, 0x0a80, 0x0c00, 0x0d80, 0x0f00, 0x1080, 0x1200,
};

/* volmap for LINE/MIC Input Volumes */
static SINT16 mc1n2_vol_ain[] = {
	0xa000, 0xe200, 0xe380, 0xe500, 0xe680, 0xe800, 0xe980, 0xeb00,
	0xec80, 0xee00, 0xef80, 0xf100, 0xf280, 0xf400, 0xf580, 0xf700,
	0xf880, 0xfa00, 0xfb80, 0xfd00, 0xfe80, 0x0000, 0x0180, 0x0300,
	0x0480, 0x0600, 0x0780, 0x0900, 0x0a80, 0x0c00, 0x0d80, 0x0f00,
};

/* volmap for HP/SP Output Volumes */
static SINT16 mc1n2_vol_hpsp[] = {
	0xa000, 0xdc00, 0xe400, 0xe800, 0xea00, 0xec00, 0xee00, 0xf000,
	0xf100, 0xf200, 0xf300, 0xf400, 0xf500, 0xf600, 0xf700, 0xf800,
	0xf880, 0xf900, 0xf980, 0xfa00, 0xfa80, 0xfb00, 0xfb80, 0xfc00,
	0xfc80, 0xfd00, 0xfd80, 0xfe00, 0xfe80, 0xff00, 0xff80, 0x0000,
};

/* volmap for RC/LINE Output Volumes */
static SINT16 mc1n2_vol_aout[] = {
	0xa000, 0xe200, 0xe300, 0xe400, 0xe500, 0xe600, 0xe700, 0xe800,
	0xe900, 0xea00, 0xeb00, 0xec00, 0xed00, 0xee00, 0xef00, 0xf000,
	0xf100, 0xf200, 0xf300, 0xf400, 0xf500, 0xf600, 0xf700, 0xf800,
	0xf900, 0xfa00, 0xfb00, 0xfc00, 0xfd00, 0xfe00, 0xff00, 0x0000,
};

/* volmap for MIC Gain Volumes */
static SINT16 mc1n2_vol_micgain[] = {
	0x0f00, 0x1400, 0x1900, 0x1e00,
};

/* volmap for HP Gain Volume */
static SINT16 mc1n2_vol_hpgain[] = {
	0x0000, 0x0180, 0x0300, 0x0600,
};

struct mc1n2_vreg_info mc1n2_vreg_map[MC1N2_N_VOL_REG] = {
	{offsetof(MCDRV_VOL_INFO, aswD_Ad0),       mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Aeng6),     mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Pdm),       mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir0),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir1),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir2),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Ad0Att),    mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir0Att),   mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir1Att),   mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dir2Att),   mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_SideTone),  mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_DacMaster), mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_DacVoice),  mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_DacAtt),    mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dit0),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dit1),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswD_Dit2),      mc1n2_vol_digital},
	{offsetof(MCDRV_VOL_INFO, aswA_Ad0),       mc1n2_vol_adc},
	{offsetof(MCDRV_VOL_INFO, aswA_Lin1),      mc1n2_vol_ain},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic1),      mc1n2_vol_ain},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic2),      mc1n2_vol_ain},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic3),      mc1n2_vol_ain},
	{offsetof(MCDRV_VOL_INFO, aswA_Hp),        mc1n2_vol_hpsp},
	{offsetof(MCDRV_VOL_INFO, aswA_Sp),        mc1n2_vol_hpsp},
	{offsetof(MCDRV_VOL_INFO, aswA_Rc),        mc1n2_vol_hpsp},
	{offsetof(MCDRV_VOL_INFO, aswA_Lout1),     mc1n2_vol_aout},
	{offsetof(MCDRV_VOL_INFO, aswA_Lout2),     mc1n2_vol_aout},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic1Gain),  mc1n2_vol_micgain},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic2Gain),  mc1n2_vol_micgain},
	{offsetof(MCDRV_VOL_INFO, aswA_Mic3Gain),  mc1n2_vol_micgain},
	{offsetof(MCDRV_VOL_INFO, aswA_HpGain),    mc1n2_vol_hpgain},
};

#ifdef ALSA_VER_ANDROID_3_0
static int cache_read(struct snd_soc_codec *codec, unsigned int reg)
{
	int ret;
	unsigned int val;

	ret = snd_soc_cache_read(codec, reg, &val);
	if (ret != 0) {
		dev_err(codec->dev, "Cache read to %x failed: %d\n", reg, ret);
		return -EIO;
	}
	return val;
}
static int cache_write(struct snd_soc_codec *codec,
					   unsigned int reg, unsigned int value)
{
	return ((int)snd_soc_cache_write(codec, reg, value));
}
#else
static int cache_read(struct snd_soc_codec *codec, unsigned int reg)
{
	return ((u16 *)codec->reg_cache)[reg];
}
static int cache_write(struct snd_soc_codec *codec,
					   unsigned int reg, unsigned int value)
{
	u16 *cp = (u16 *)codec->reg_cache + reg;
	*cp = value;
	return 0;
}
#endif

static unsigned int mc1n2_read_reg(struct snd_soc_codec *codec, unsigned int reg)
{
	int ret;

	ret = cache_read(codec, reg);
	if (ret < 0) {
		return -EIO;
	}
	return (unsigned int)ret;
}

#ifdef ALSA_VER_ANDROID_3_0
#define REG_CACHE_READ(reg)	(mc1n2_read_reg(codec, reg))
#else
#define REG_CACHE_READ(reg)	((u16 *)codec->reg_cache)[reg]
#endif

static int write_reg_vol(struct snd_soc_codec *codec,
			   unsigned int reg, unsigned int value)
{
	MCDRV_VOL_INFO update;
	SINT16 *vp;
	int ret;
	int err, i;

	memset(&update, 0, sizeof(MCDRV_VOL_INFO));
	vp = (SINT16 *)((void *)&update + mc1n2_vreg_map[reg].offset);

	for (i = 0; i < 2; i++, vp++) {
		unsigned int v = (value >> (i*8)) & 0xff;
		unsigned int c = (mc1n2_read_reg(codec, reg) >> (i*8)) & 0xff;
		if (v != c) {
			int sw, vol;
			SINT16 db;
			sw = (reg < MC1N2_AVOL_MIC1_GAIN) ? (v & 0x80) : 1;
			vol = sw ? (v & 0x7f) : 0;
			db = mc1n2_vreg_map[reg].volmap[vol];
			*vp = db | MCDRV_VOL_UPDATE;
		}
	}

	err = _McDrv_Ctrl(MCDRV_SET_VOLUME, &update, 0);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in MCDRV_SET_VOLUME\n", err);
		return -EIO;
	}
	ret = cache_write(codec, reg, value);
	if (ret != 0) {
		dev_err(codec->dev, "Cache write to %x failed: %d\n", reg, ret);
	}

	return 0;
}

static int mc1n2_hwdep_ioctl_set_path(struct snd_soc_codec *codec,
				      void *info, unsigned int update);

static int write_reg_path(struct snd_soc_codec *codec,
			   unsigned int reg, unsigned int value)
{
	MCDRV_PATH_INFO update;
	MCDRV_CHANNEL *pch;
	MCDRV_AE_INFO *pae;
	int ret = 0;
	int err;

	memset(&update, 0, sizeof(MCDRV_PATH_INFO));

	ret = cache_write(codec, reg, value);
	if (ret != 0) {
		dev_err(codec->dev, "Cache write to %x failed: %d\n",reg, ret);
	}

	switch (reg) {
	case MC1N2_ADCL_MIC1_SW:
		if (value) {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_ADCL_MIC2_SW:
		if (value) {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_ADCL_MIC3_SW:
		if (value) {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asAdc0[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_ADCL_LINE_SW:
	case MC1N2_ADCL_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_ADCL_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_ADCL_LINE_SW)) {
				update.asAdc0[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_ON;
			}
			else {
				update.asAdc0[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_ADCL_LINE_SW)) {
				update.asAdc0[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asAdc0[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_ADCR_MIC1_SW:
		if (value) {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_ADCR_MIC2_SW:
		if (value) {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_ADCR_MIC3_SW:
		if (value) {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asAdc0[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_ADCR_LINE_SW:
	case MC1N2_ADCR_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_ADCR_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_ADCR_LINE_SW)) {
				update.asAdc0[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_ON;
			}
			else {
				update.asAdc0[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_ADCR_LINE_SW)) {
				update.asAdc0[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asAdc0[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_HPL_MIC1_SW:
		if (value) {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_HPL_MIC2_SW:
		if (value) {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_HPL_MIC3_SW:
		if (value) {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asHpOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_HPL_LINE_SW:
	case MC1N2_HPL_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_HPL_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_HPL_LINE_SW)) {
				update.asHpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_ON;
			}
			else {
				update.asHpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_HPL_LINE_SW)) {
				update.asHpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asHpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_HPL_DAC_SW:
	case MC1N2_HPL_DAC_SRC:
		if (REG_CACHE_READ(MC1N2_HPL_DAC_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_HPL_DAC_SW)) {
				update.asHpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_ON;
			}
			else {
				update.asHpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_HPL_DAC_SW)) {
				update.asHpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_ON;
			}
			else {
				update.asHpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_OFF;
			}
		}
		break;
	case MC1N2_HPR_MIC1_SW:
		if (value) {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_HPR_MIC2_SW:
		if (value) {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_HPR_MIC3_SW:
		if (value) {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asHpOut[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_HPR_LINER_SW:
		if (value) {
			update.asHpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_ON;
		}
		else {
			update.asHpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_OFF;
		}
		break;
	case MC1N2_HPR_DACR_SW:
		if (value) {
			update.asHpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_ON;
		}
		else {
			update.asHpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_OFF;
		}
		break;
	case MC1N2_SPL_LINE_SW:
	case MC1N2_SPL_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_SPL_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_SPL_LINE_SW)) {
				update.asSpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_ON;
			}
			else {
				update.asSpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_SPL_LINE_SW)) {
				update.asSpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asSpOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_SPL_DAC_SW:
	case MC1N2_SPL_DAC_SRC:
		if (REG_CACHE_READ(MC1N2_SPL_DAC_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_SPL_DAC_SW)) {
				update.asSpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_ON;
			}
			else {
				update.asSpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_SPL_DAC_SW)) {
				update.asSpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_ON;
			}
			else {
				update.asSpOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_OFF;
			}
		}
		break;
	case MC1N2_SPR_LINE_SW:
	case MC1N2_SPR_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_SPR_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_SPR_LINE_SW)) {
				update.asSpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_ON;
			}
			else {
				update.asSpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_SPR_LINE_SW)) {
				update.asSpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asSpOut[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_SPR_DAC_SW:
	case MC1N2_SPR_DAC_SRC:
		if (REG_CACHE_READ(MC1N2_SPR_DAC_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_SPR_DAC_SW)) {
				update.asSpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_ON;
			}
			else {
				update.asSpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_SPR_DAC_SW)) {
				update.asSpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_ON;
			}
			else {
				update.asSpOut[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_OFF;
			}
		}
		break;
	case MC1N2_RC_MIC1_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_RC_MIC2_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_RC_MIC3_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_RC_LINEMONO_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
		}
		break;
	case MC1N2_RC_DACL_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_OFF;
		}
		break;
	case MC1N2_RC_DACR_SW:
		if (value) {
			update.asRcOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_ON;
		}
		else {
			update.asRcOut[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_OFF;
		}
		break;
	case MC1N2_LOUT1L_MIC1_SW:
		if (value) {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_LOUT1L_MIC2_SW:
		if (value) {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_LOUT1L_MIC3_SW:
		if (value) {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asLout1[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_LOUT1L_LINE_SW:
	case MC1N2_LOUT1L_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_LOUT1L_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_LOUT1L_LINE_SW)) {
				update.asLout1[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_ON;
			}
			else {
				update.asLout1[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_LOUT1L_LINE_SW)) {
				update.asLout1[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asLout1[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_LOUT1L_DAC_SW:
	case MC1N2_LOUT1L_DAC_SRC:
		if (REG_CACHE_READ(MC1N2_LOUT1L_DAC_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_LOUT1L_DAC_SW)) {
				update.asLout1[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_ON;
			}
			else {
				update.asLout1[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_OFF;
			}
		}
		else {
			if (value) {
				update.asLout1[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_ON;
			}
			else {
				update.asLout1[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_OFF;
			}
		}
		break;
	case MC1N2_LOUT1R_MIC1_SW:
		if (value) {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_LOUT1R_MIC2_SW:
		if (value) {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_LOUT1R_MIC3_SW:
		if (value) {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asLout1[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_LOUT1R_LINER_SW:
		if (value) {
			update.asLout1[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_ON;
		}
		else {
			update.asLout1[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_OFF;
		}
		break;
	case MC1N2_LOUT1R_DACR_SW:
		if (value) {
			update.asLout1[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_ON;
		}
		else {
			update.asLout1[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_OFF;
		}
		break;
	case MC1N2_LOUT2L_MIC1_SW:
		if (value) {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_LOUT2L_MIC2_SW:
		if (value) {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_LOUT2L_MIC3_SW:
		if (value) {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asLout2[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_LOUT2L_LINE_SW:
	case MC1N2_LOUT2L_LINE_SRC:
		if (REG_CACHE_READ(MC1N2_LOUT2L_LINE_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_LOUT2L_LINE_SW)) {
				update.asLout2[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_ON;
			}
			else {
				update.asLout2[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_LOUT2L_LINE_SW)) {
				update.asLout2[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_ON;
			}
			else {
				update.asLout2[0].abSrcOnOff[1] = MCDRV_SRC1_LINE1_M_OFF;
			}
		}
		break;
	case MC1N2_LOUT2L_DAC_SW:
	case MC1N2_LOUT2L_DAC_SRC:
		if (REG_CACHE_READ(MC1N2_LOUT2L_DAC_SRC) == 0) {
			if (REG_CACHE_READ(MC1N2_LOUT2L_DAC_SW)) {
				update.asLout2[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_ON;
			}
			else {
				update.asLout2[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_L_OFF;
			}
		}
		else {
			if (REG_CACHE_READ(MC1N2_LOUT2L_DAC_SW)) {
				update.asLout2[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_ON;
			}
			else {
				update.asLout2[0].abSrcOnOff[5] = MCDRV_SRC5_DAC_M_OFF;
			}
		}
		break;
	case MC1N2_LOUT2R_MIC1_SW:
		if (value) {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_LOUT2R_MIC2_SW:
		if (value) {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_LOUT2R_MIC3_SW:
		if (value) {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asLout2[1].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	case MC1N2_LOUT2R_LINER_SW:
		if (value) {
			update.asLout2[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_ON;
		}
		else {
			update.asLout2[1].abSrcOnOff[1] = MCDRV_SRC1_LINE1_R_OFF;
		}
		break;
	case MC1N2_LOUT2R_DACR_SW:
		if (value) {
			update.asLout2[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_ON;
		}
		else {
			update.asLout2[1].abSrcOnOff[5] = MCDRV_SRC5_DAC_R_OFF;
		}
		break;
	case MC1N2_DACMAIN_SRC:
	case MC1N2_DACVOICE_SRC:
	case MC1N2_DIT0_SRC:
	case MC1N2_DIT1_SRC:
	case MC1N2_DIT2_SRC:
		if (reg == MC1N2_DACMAIN_SRC) {
			pch = &update.asDac[0];
		}
		else if (reg == MC1N2_DACVOICE_SRC) {
			pch = &update.asDac[1];
		}
		else if (reg == MC1N2_DIT0_SRC) {
			pch = &update.asDit0[0];
		}
		else if (reg == MC1N2_DIT1_SRC) {
			pch = &update.asDit1[0];
		}
		else if (reg == MC1N2_DIT2_SRC) {
			pch = &update.asDit2[0];
		}

		switch (value) {
		case MC1N2_DSOURCE_OFF:
			pch->abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			break;
		case MC1N2_DSOURCE_ADC: /* ADC */
			pch->abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				pch->abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			pch->abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			pch->abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				pch->abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}
		break;
	case MC1N2_AE_SRC:
	case MC1N2_ADC_PDM_SEL:
		switch (REG_CACHE_READ(MC1N2_AE_SRC)) {
		case MC1N2_DSOURCE_OFF:
			update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF;
			break;
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
				update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
			}
			else {
				update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
			}
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF;
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF;
			break;
		case MC1N2_DSOURCE_DIR1:/* DIR1 */
			update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF;
			break;
		case MC1N2_DSOURCE_DIR2:/* DIR2 */
			update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF;
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			update.asAe[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
			update.asAe[0].abSrcOnOff[3] =
				MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
			update.asAe[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON;
			break;
		}

		switch (REG_CACHE_READ(MC1N2_DACMAIN_SRC)) {
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				update.asDac[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					update.asDac[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					update.asDac[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}

		switch (REG_CACHE_READ(MC1N2_DACVOICE_SRC)) {
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				update.asDac[1].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					update.asDac[1].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					update.asDac[1].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[1].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDac[1].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}

		switch (REG_CACHE_READ(MC1N2_DIT0_SRC)) {
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				update.asDit0[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					update.asDit0[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					update.asDit0[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit0[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit0[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}

		switch (REG_CACHE_READ(MC1N2_DIT1_SRC)) {
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				update.asDit1[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					update.asDit1[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					update.asDit1[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit1[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit1[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}

		switch (REG_CACHE_READ(MC1N2_DIT2_SRC)) {
		case MC1N2_DSOURCE_ADC: /* ADC */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_ADC) {
				update.asDit2[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
					update.asDit2[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
				}
				else {
					update.asDit2[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
				}
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR0: /* DIR0 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR0) {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_ON | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR1: /* DIR1 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR1) {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_ON | MCDRV_SRC3_DIR2_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_DIR2: /* DIR2 */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_DIR2) {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_OFF;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit2[0].abSrcOnOff[3] =
					MCDRV_SRC3_DIR0_OFF | MCDRV_SRC3_DIR1_OFF | MCDRV_SRC3_DIR2_ON;
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_OFF;
			}
			break;
		case MC1N2_DSOURCE_MIX: /* MIX */
			if (REG_CACHE_READ(MC1N2_AE_SRC) == MC1N2_DSOURCE_MIX) {
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_OFF | MCDRV_SRC6_AE_ON;
			}
			else {
				update.asDit2[0].abSrcOnOff[6] = MCDRV_SRC6_MIX_ON | MCDRV_SRC6_AE_OFF;
			}
			break;
		}

		break;
	case MC1N2_DMIX_ADC_SW:
		if (value) {
			if (REG_CACHE_READ(MC1N2_ADC_PDM_SEL)) {
				update.asMix[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_ON;
			}
			else {
				update.asMix[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_ON | MCDRV_SRC4_PDM_OFF;
			}
		}
		else {
			update.asMix[0].abSrcOnOff[4] = MCDRV_SRC4_ADC0_OFF | MCDRV_SRC4_PDM_OFF;
		}
		break;
	case MC1N2_DMIX_DIR0_SW:
		if (value) {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_ON;
		}
		else {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
		}
		break;
	case MC1N2_DMIX_DIR1_SW:
		if (value) {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR1_ON;
		}
		else {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR1_OFF;
		}
		break;
	case MC1N2_DMIX_DIR2_SW:
		if (value) {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR2_ON;
		}
		else {
			update.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR2_OFF;
		}
		break;
	case MC1N2_AE_PARAM_SEL:
		switch (value) {
		case MC1N2_AE_PARAM_1:
			pae = &sAeInfo_1;
			break;
		case MC1N2_AE_PARAM_2:
			pae = &sAeInfo_2;
			break;
		case MC1N2_AE_PARAM_3:
			pae = &sAeInfo_3;
			break;
		case MC1N2_AE_PARAM_4:
			pae = &sAeInfo_4;
			break;
		case MC1N2_AE_PARAM_5:
			pae = &sAeInfo_5;
			break;
		default:
			pae = NULL;
			break;
		}
		err = _McDrv_Ctrl(MCDRV_SET_AUDIOENGINE, pae, 0x1FF);
		return err;
		break;
	case MC1N2_MICBIAS1:
		if (value) {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_ON;
		}
		else {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC1_OFF;
		}
		break;
	case MC1N2_MICBIAS2:
		if (value) {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_ON;
		}
		else {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC2_OFF;
		}
		break;
	case MC1N2_MICBIAS3:
		if (value) {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_ON;
		}
		else {
			update.asBias[0].abSrcOnOff[0] = MCDRV_SRC0_MIC3_OFF;
		}
		break;
	}

	mc1n2_hwdep_ioctl_set_path(codec, &update, 0);
	err = _McDrv_Ctrl(MCDRV_SET_PATH, &update, 0);

	return err;
}

static int mc1n2_write_reg(struct snd_soc_codec *codec,
			   unsigned int reg, unsigned int value)
{
	int err;

	if (reg < MC1N2_N_VOL_REG) {
		err = write_reg_vol(codec, reg, value);
	}
	else {
		err = write_reg_path(codec, reg, value);
	}

	return err;
}

static int mc1n2_get_codec_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{

	return 0;
}

static int mc1n2_set_codec_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
	SINT16 *vol = (SINT16 *)&mc1n2->vol_store;

	int control_data = ucontrol->value.integer.value[0];
	int err, i;

	dev_info(codec->dev, "%s: Recovery [%d]\n", __func__, control_data);

	switch(control_data)
	{
	case CMD_CODEC_EMERGENCY_RECOVERY:
		mutex_lock(&mc1n2->mutex);

		mc1n2_set_mclk_source(1);

		/* store parameters */
		for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
			struct mc1n2_info_store *store
						= &mc1n2_info_store_tbl[i];
			if (store->get) {
				err = _McDrv_Ctrl(store->get,
					(void *)mc1n2 + store->offset, 0);
				if (err != MCDRV_SUCCESS) {
					dev_err(codec->dev,
						"%d: Error in MCDRV_GET\n",
						err);
				}
			}
		}

		err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
		if (err != MCDRV_SUCCESS)
			dev_err(codec->dev, "%d: Error in MCDRV_TERM\n", err);

		err = _McDrv_Ctrl(MCDRV_INIT, &mc1n2->setup.init, 0);
		if (err != MCDRV_SUCCESS)
			dev_err(codec->dev, "%d: Error in MCDRV_INIT\n", err);

		/* restore parameters */
		for (i = 0; i < sizeof(MCDRV_VOL_INFO)/sizeof(SINT16);
								i++, vol++) {
			*vol |= 0x0001;
		}

		for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
			struct mc1n2_info_store *store =
						&mc1n2_info_store_tbl[i];
			if (store->set) {
				err = _McDrv_Ctrl(store->set,
					(void *) mc1n2 + store->offset,
					 store->flags);

				if (err != MCDRV_SUCCESS) {
					dev_err(codec->dev,
						"%d: Error in MCDRV_Set\n",
						err);
				}
			}
		}

		err = mc1n2_update_clock(mc1n2);
		if (err != MCDRV_SUCCESS) {
			dev_err(codec->dev,
				"%d: Error in mc1n2_update_clock\n", err);
		}

		mutex_unlock(&mc1n2->mutex);

		dev_info(codec->dev, "%s: Recovery Done\n", __func__);
		break;

	default:
		break;
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(mc1n2_tlv_digital, -7500, 100, 1);
static const DECLARE_TLV_DB_SCALE(mc1n2_tlv_adc, -2850, 150, 1);
static const DECLARE_TLV_DB_SCALE(mc1n2_tlv_ain, -3150, 150, 1);
static const DECLARE_TLV_DB_SCALE(mc1n2_tlv_aout, -3100, 100, 1);
static const DECLARE_TLV_DB_SCALE(mc1n2_tlv_micgain, 1500, 500, 0);

static unsigned int mc1n2_tlv_hpsp[] = {
	TLV_DB_RANGE_HEAD(5),
	0, 2, TLV_DB_SCALE_ITEM(-4400, 800, 1),
	2, 3, TLV_DB_SCALE_ITEM(-2800, 400, 0),
	3, 7, TLV_DB_SCALE_ITEM(-2400, 200, 0),
	7, 15, TLV_DB_SCALE_ITEM(-1600, 100, 0),
	15, 31, TLV_DB_SCALE_ITEM(-800, 50, 0),
};

static unsigned int mc1n2_tlv_hpgain[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 2, TLV_DB_SCALE_ITEM(0, 150, 0),
	2, 3, TLV_DB_SCALE_ITEM(300, 300, 0),
};

static const char *codec_status_control[] = {
        "REC_OFF", "REC_ON", "RESET_ON", "RESET_OFF"
};

static const struct soc_enum path_control_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(codec_status_control), codec_status_control),
};

static const struct snd_kcontrol_new mc1n2_snd_controls[] = {
	/*
	 * digital volumes and mute switches
	 */
	SOC_DOUBLE_TLV("AD Digital Volume",
		       MC1N2_DVOL_AD0, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("AD Digital Switch",
		   MC1N2_DVOL_AD0, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("AENG6 Volume",
		       MC1N2_DVOL_AENG6, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("AENG6 Switch",
		   MC1N2_DVOL_AENG6, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("PDM Volume",
		       MC1N2_DVOL_PDM, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("PDM Switch",
		   MC1N2_DVOL_PDM, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#0 Volume",
		       MC1N2_DVOL_DIR0, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#0 Switch",
		   MC1N2_DVOL_DIR0, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#1 Volume",
		       MC1N2_DVOL_DIR1, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#1 Switch",
		   MC1N2_DVOL_DIR1, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#2 Volume",
		       MC1N2_DVOL_DIR2, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#2 Switch",
		   MC1N2_DVOL_DIR2, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("AD ATT Volume",
		       MC1N2_DVOL_AD0_ATT, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("AD ATT Switch",
		   MC1N2_DVOL_AD0_ATT, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#0 ATT Volume",
		       MC1N2_DVOL_DIR0_ATT, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#0 ATT Switch",
		   MC1N2_DVOL_DIR0_ATT, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#1 ATT Volume",
		       MC1N2_DVOL_DIR1_ATT, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#1 ATT Switch",
		   MC1N2_DVOL_DIR1_ATT, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIR#2 ATT Volume",
		       MC1N2_DVOL_DIR2_ATT, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIR#2 ATT Switch",
		   MC1N2_DVOL_DIR2_ATT, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Side Tone Playback Volume",
		       MC1N2_DVOL_SIDETONE, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("Side Tone Playback Switch",
		   MC1N2_DVOL_SIDETONE, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Master Playback Volume",
		       MC1N2_DVOL_DAC_MASTER, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("Master Playback Switch",
		   MC1N2_DVOL_DAC_MASTER, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Voice Playback Volume",
		       MC1N2_DVOL_DAC_VOICE, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("Voice Playback Switch",
		   MC1N2_DVOL_DAC_VOICE, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DAC Playback Volume",
		       MC1N2_DVOL_DAC_ATT, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DAC Playback Switch",
		   MC1N2_DVOL_DAC_ATT, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIT#0 Capture Volume",
		       MC1N2_DVOL_DIT0, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIT#0 Capture Switch",
		   MC1N2_DVOL_DIT0, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIT#1 Capture Volume",
		       MC1N2_DVOL_DIT1, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIT#1 Capture Switch",
		   MC1N2_DVOL_DIT1, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("DIT#2 Capture Volume",
		       MC1N2_DVOL_DIT2, 0, 8, 93, 0, mc1n2_tlv_digital),
	SOC_DOUBLE("DIT#2 Capture Switch",
		   MC1N2_DVOL_DIT2, 7, 15, 1, 0),

	/*
	 * analog volumes and mute switches
	 */
	SOC_DOUBLE_TLV("AD Analog Volume",
		       MC1N2_AVOL_AD0, 0, 8, 31, 0, mc1n2_tlv_adc),
	SOC_DOUBLE("AD Analog Switch",
		   MC1N2_AVOL_AD0, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Line Bypass Playback Volume",
		       MC1N2_AVOL_LIN1, 0, 8, 31, 0, mc1n2_tlv_ain),
	SOC_DOUBLE("Line Bypass Playback Switch",
		   MC1N2_AVOL_LIN1, 7, 15, 1, 0),

	SOC_SINGLE_TLV("Mic 1 Bypass Playback Volume",
		       MC1N2_AVOL_MIC1, 0, 31, 0, mc1n2_tlv_ain),
	SOC_SINGLE("Mic 1 Bypass Playback Switch",
		   MC1N2_AVOL_MIC1, 7, 1, 0),

	SOC_SINGLE_TLV("Mic 2 Bypass Playback Volume",
		       MC1N2_AVOL_MIC2, 0, 31, 0, mc1n2_tlv_ain),
	SOC_SINGLE("Mic 2 Bypass Playback Switch",
		   MC1N2_AVOL_MIC2, 7, 1, 0),

	SOC_SINGLE_TLV("Mic 3 Bypass Playback Volume",
		       MC1N2_AVOL_MIC3, 0, 31, 0, mc1n2_tlv_ain),
	SOC_SINGLE("Mic 3 Bypass Playback Switch",
		   MC1N2_AVOL_MIC3, 7, 1, 0),

	SOC_DOUBLE_TLV("Headphone Playback Volume",
		       MC1N2_AVOL_HP, 0, 8, 31, 0, mc1n2_tlv_hpsp),
	SOC_DOUBLE("Headphone Playback Switch",
		   MC1N2_AVOL_HP, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Speaker Playback Volume",
		       MC1N2_AVOL_SP, 0, 8, 31, 0, mc1n2_tlv_hpsp),
	SOC_DOUBLE("Speaker Playback Switch",
		   MC1N2_AVOL_SP, 7, 15, 1, 0),

	SOC_SINGLE_TLV("Receiver Playback Volume",
		       MC1N2_AVOL_RC, 0, 31, 0, mc1n2_tlv_hpsp),
	SOC_SINGLE("Receiver Playback Switch",
		   MC1N2_AVOL_RC, 7, 1, 0),

	SOC_DOUBLE_TLV("Line 1 Playback Volume",
		       MC1N2_AVOL_LOUT1, 0, 8, 31, 0, mc1n2_tlv_aout),
	SOC_DOUBLE("Line 1 Playback Switch",
		   MC1N2_AVOL_LOUT1, 7, 15, 1, 0),

	SOC_DOUBLE_TLV("Line 2 Playback Volume",
		       MC1N2_AVOL_LOUT2, 0, 8, 31, 0, mc1n2_tlv_aout),
	SOC_DOUBLE("Line 2 Playback Switch",
		   MC1N2_AVOL_LOUT2, 7, 15, 1, 0),

	SOC_SINGLE_TLV("Mic 1 Gain Volume",
		       MC1N2_AVOL_MIC1_GAIN, 0, 3, 0, mc1n2_tlv_micgain),

	SOC_SINGLE_TLV("Mic 2 Gain Volume",
		       MC1N2_AVOL_MIC2_GAIN, 0, 3, 0, mc1n2_tlv_micgain),

	SOC_SINGLE_TLV("Mic 3 Gain Volume",
		       MC1N2_AVOL_MIC3_GAIN, 0, 3, 0, mc1n2_tlv_micgain),

	SOC_SINGLE_TLV("HP Gain Playback Volume",
		       MC1N2_AVOL_HP_GAIN, 0, 3, 0, mc1n2_tlv_hpgain),

	SOC_ENUM_EXT("Codec Status", path_control_enum[0],
				mc1n2_get_codec_status, mc1n2_set_codec_status),
};

/*
 * Same as snd_soc_add_controls supported in alsa-driver 1.0.19 or later.
 * This function is implimented for compatibility with linux 2.6.29.
 */
static int mc1n2_add_controls(struct snd_soc_codec *codec,
			      const struct snd_kcontrol_new *controls, int n)
{
	int err, i;

	for (i = 0; i < n; i++, controls++) {
#ifdef ALSA_VER_ANDROID_3_0
		if ((err = snd_ctl_add((struct snd_card *)codec->card->snd_card,
					snd_soc_cnew(controls, codec, NULL, NULL))) < 0) {
			return err;
		}
#else
		if ((err = snd_ctl_add(codec->card,
				       snd_soc_cnew(controls, codec, NULL))) < 0) {
			return err;
		}
#endif
	}

	return 0;
}

static const struct snd_kcontrol_new adcl_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_ADCL_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_ADCL_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_ADCL_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Line Switch", MC1N2_ADCL_LINE_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new adcr_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_ADCR_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_ADCR_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_ADCR_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Line Switch", MC1N2_ADCR_LINE_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new hpl_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_HPL_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_HPL_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_HPL_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Line Switch", MC1N2_HPL_LINE_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dac Switch", MC1N2_HPL_DAC_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new hpr_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_HPR_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_HPR_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_HPR_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("LineR Switch", MC1N2_HPR_LINER_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("DacR Switch", MC1N2_HPR_DACR_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new spl_mix[] = {
	SOC_DAPM_SINGLE("Line Switch", MC1N2_SPL_LINE_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dac Switch", MC1N2_SPL_DAC_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new spr_mix[] = {
	SOC_DAPM_SINGLE("Line Switch", MC1N2_SPR_LINE_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dac Switch", MC1N2_SPR_DAC_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new rc_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_RC_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_RC_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_RC_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("LineMono Switch", MC1N2_RC_LINEMONO_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("DacL Switch", MC1N2_RC_DACL_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("DacR Switch", MC1N2_RC_DACR_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new lout1l_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_LOUT1L_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_LOUT1L_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_LOUT1L_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Line Switch", MC1N2_LOUT1L_LINE_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dac Switch", MC1N2_LOUT1L_DAC_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new lout1r_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_LOUT1R_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_LOUT1R_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_LOUT1R_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("LineR Switch", MC1N2_LOUT1R_LINER_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("DacR Switch", MC1N2_LOUT1R_DACR_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new lout2l_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_LOUT2L_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_LOUT2L_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_LOUT2L_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Line Switch", MC1N2_LOUT2L_LINE_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dac Switch", MC1N2_LOUT2L_DAC_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new lout2r_mix[] = {
	SOC_DAPM_SINGLE("Mic1 Switch", MC1N2_LOUT2R_MIC1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic2 Switch", MC1N2_LOUT2R_MIC2_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Mic3 Switch", MC1N2_LOUT2R_MIC3_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("LineR Switch", MC1N2_LOUT2R_LINER_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("DacR Switch", MC1N2_LOUT2R_DACR_SW, 0, 1, 0),
};

static const struct snd_kcontrol_new digital_mix[] = {
	SOC_DAPM_SINGLE("Adc Switch", MC1N2_DMIX_ADC_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dir0 Switch", MC1N2_DMIX_DIR0_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dir1 Switch", MC1N2_DMIX_DIR1_SW, 0, 1, 0),
	SOC_DAPM_SINGLE("Dir2 Switch", MC1N2_DMIX_DIR2_SW, 0, 1, 0),
};

static const char *dsource_text[] = {
	"OFF", "ADC", "DIR0", "DIR1", "DIR2", "MIX",
};

static const char *linel_mode_text[] = {
	"LINEL", "LINEMONO",
};

static const char *liner_mode_text[] = {
	"LINER", "LINEMONO",
};

static const char *dacl_mode_text[] = {
	"DACL", "DACMONO",
};

static const char *dacr_mode_text[] = {
	"DACR", "DACMONO",
};

static const char *ae_param_text[] = {
	"PARAM1", "PARAM2", "PARAM3", "PARAM4", "PARAM5",
};

static const char *adc_pdm_text[] = {
	"ADC", "PDM",
};

static const struct soc_enum dacmain_source_enum =
	SOC_ENUM_SINGLE(MC1N2_DACMAIN_SRC, 0, 6, dsource_text);

static const struct soc_enum dacvoice_source_enum =
	SOC_ENUM_SINGLE(MC1N2_DACVOICE_SRC, 0, 6, dsource_text);

static const struct soc_enum dit0_source_enum =
	SOC_ENUM_SINGLE(MC1N2_DIT0_SRC, 0, 6, dsource_text);

static const struct soc_enum dit1_source_enum =
	SOC_ENUM_SINGLE(MC1N2_DIT1_SRC, 0, 6, dsource_text);

static const struct soc_enum dit2_source_enum =
	SOC_ENUM_SINGLE(MC1N2_DIT2_SRC, 0, 6, dsource_text);

static const struct soc_enum ae_source_enum =
	SOC_ENUM_SINGLE(MC1N2_AE_SRC, 0, 6, dsource_text);

static const struct soc_enum adcl_line_enum =
	SOC_ENUM_SINGLE(MC1N2_ADCL_LINE_SRC, 0, 2, linel_mode_text);

static const struct soc_enum adcr_line_enum =
	SOC_ENUM_SINGLE(MC1N2_ADCR_LINE_SRC, 0, 2, liner_mode_text);

static const struct soc_enum hpl_line_enum =
	SOC_ENUM_SINGLE(MC1N2_HPL_LINE_SRC, 0, 2, linel_mode_text);

static const struct soc_enum hpl_dac_enum =
	SOC_ENUM_SINGLE(MC1N2_HPL_DAC_SRC, 0, 2, dacl_mode_text);

static const struct soc_enum spl_line_enum =
	SOC_ENUM_SINGLE(MC1N2_SPL_LINE_SRC, 0, 2, linel_mode_text);

static const struct soc_enum spl_dac_enum =
	SOC_ENUM_SINGLE(MC1N2_SPL_DAC_SRC, 0, 2, dacl_mode_text);

static const struct soc_enum spr_line_enum =
	SOC_ENUM_SINGLE(MC1N2_SPR_LINE_SRC, 0, 2, liner_mode_text);

static const struct soc_enum spr_dac_enum =
	SOC_ENUM_SINGLE(MC1N2_SPR_DAC_SRC, 0, 2, dacr_mode_text);

static const struct soc_enum lout1l_line_enum =
	SOC_ENUM_SINGLE(MC1N2_LOUT1L_LINE_SRC, 0, 2, linel_mode_text);

static const struct soc_enum lout1l_dac_enum =
	SOC_ENUM_SINGLE(MC1N2_LOUT1L_DAC_SRC, 0, 2, dacl_mode_text);

static const struct soc_enum lout2l_line_enum =
	SOC_ENUM_SINGLE(MC1N2_LOUT2L_LINE_SRC, 0, 2, linel_mode_text);

static const struct soc_enum lout2l_dac_enum =
	SOC_ENUM_SINGLE(MC1N2_LOUT2L_DAC_SRC, 0, 2, dacl_mode_text);

static const struct soc_enum ae_param_enum =
	SOC_ENUM_SINGLE(MC1N2_AE_PARAM_SEL, 0, 5, ae_param_text);

static const struct soc_enum adc_pdm_enum =
	SOC_ENUM_SINGLE(MC1N2_ADC_PDM_SEL, 0, 2, adc_pdm_text);

static const struct snd_kcontrol_new dacmain_mux =
	SOC_DAPM_ENUM("DACMAIN SRC MUX", dacmain_source_enum);

static const struct snd_kcontrol_new dacvoice_mux =
	SOC_DAPM_ENUM("DACVOICE SRC MUX", dacvoice_source_enum);

static const struct snd_kcontrol_new dit0_mux =
	SOC_DAPM_ENUM("DIT0 SRC MUX", dit0_source_enum);

static const struct snd_kcontrol_new dit1_mux =
	SOC_DAPM_ENUM("DIT1 SRC MUX", dit1_source_enum);

static const struct snd_kcontrol_new dit2_mux =
	SOC_DAPM_ENUM("DIT2 SRC MUX", dit2_source_enum);

static const struct snd_kcontrol_new ae_mux =
	SOC_DAPM_ENUM("AE SRC MUX", ae_source_enum);

static const struct snd_kcontrol_new adcl_line_mux =
	SOC_DAPM_ENUM("ADCL LINE MUX", adcl_line_enum);

static const struct snd_kcontrol_new adcr_line_mux =
	SOC_DAPM_ENUM("ADCR LINE MUX", adcr_line_enum);

static const struct snd_kcontrol_new hpl_line_mux =
	SOC_DAPM_ENUM("HPL LINE MUX", hpl_line_enum);

static const struct snd_kcontrol_new hpl_dac_mux =
	SOC_DAPM_ENUM("HPL DAC MUX", hpl_dac_enum);

static const struct snd_kcontrol_new spl_line_mux =
	SOC_DAPM_ENUM("SPL LINE MUX", spl_line_enum);

static const struct snd_kcontrol_new spl_dac_mux =
	SOC_DAPM_ENUM("SPL DAC MUX", spl_dac_enum);

static const struct snd_kcontrol_new spr_line_mux =
	SOC_DAPM_ENUM("SPR LINE MUX", spr_line_enum);

static const struct snd_kcontrol_new spr_dac_mux =
	SOC_DAPM_ENUM("SPR DAC MUX", spr_dac_enum);

static const struct snd_kcontrol_new lout1l_line_mux =
	SOC_DAPM_ENUM("LOUT1L LINE MUX", lout1l_line_enum);

static const struct snd_kcontrol_new lout1l_dac_mux =
	SOC_DAPM_ENUM("LOUT1L DAC MUX", lout1l_dac_enum);

static const struct snd_kcontrol_new lout2l_line_mux =
	SOC_DAPM_ENUM("LOUT2L LINE MUX", lout2l_line_enum);

static const struct snd_kcontrol_new lout2l_dac_mux =
	SOC_DAPM_ENUM("LOUT2L DAC MUX", lout2l_dac_enum);

static const struct snd_kcontrol_new ae_param_mux =
	SOC_DAPM_ENUM("AE PARAMETER", ae_param_enum);

static const struct snd_kcontrol_new adc_pdm_mux =
	SOC_DAPM_ENUM("ADC PDM MUX", adc_pdm_enum);

static const struct snd_kcontrol_new bias1_sw =
	SOC_DAPM_SINGLE("Switch", MC1N2_MICBIAS1, 0, 1, 0);

static const struct snd_kcontrol_new bias2_sw =
	SOC_DAPM_SINGLE("Switch", MC1N2_MICBIAS2, 0, 1, 0);

static const struct snd_kcontrol_new bias3_sw =
	SOC_DAPM_SINGLE("Switch", MC1N2_MICBIAS3, 0, 1, 0);

static const struct snd_soc_dapm_widget mc1n2_widgets[] = {
SND_SOC_DAPM_INPUT("MIC1"),
SND_SOC_DAPM_INPUT("MIC2"),
SND_SOC_DAPM_INPUT("MIC3"),
SND_SOC_DAPM_INPUT("LINEIN"),

SND_SOC_DAPM_INPUT("DIR0"),
SND_SOC_DAPM_INPUT("DIR1"),
SND_SOC_DAPM_INPUT("DIR2"),

SND_SOC_DAPM_INPUT("PDM"),

SND_SOC_DAPM_INPUT("AE1"),
SND_SOC_DAPM_INPUT("AE2"),
SND_SOC_DAPM_INPUT("AE3"),
SND_SOC_DAPM_INPUT("AE4"),
SND_SOC_DAPM_INPUT("AE5"),

SND_SOC_DAPM_INPUT("BIAS1"),
SND_SOC_DAPM_INPUT("BIAS2"),
SND_SOC_DAPM_INPUT("BIAS3"),

SND_SOC_DAPM_OUTPUT("DIT0"),
SND_SOC_DAPM_OUTPUT("DIT1"),
SND_SOC_DAPM_OUTPUT("DIT2"),

SND_SOC_DAPM_ADC("ADC", NULL, SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_DAC("DAC", NULL, SND_SOC_NOPM, 0, 0),

SND_SOC_DAPM_MIXER("ADCL MIXER", SND_SOC_NOPM, 0, 0, adcl_mix, ARRAY_SIZE(adcl_mix)),
SND_SOC_DAPM_MIXER("ADCR MIXER", SND_SOC_NOPM, 0, 0, adcr_mix, ARRAY_SIZE(adcr_mix)),
SND_SOC_DAPM_MIXER("HPL MIXER", SND_SOC_NOPM, 0, 0, hpl_mix, ARRAY_SIZE(hpl_mix)),
SND_SOC_DAPM_MIXER("HPR MIXER", SND_SOC_NOPM, 0, 0, hpr_mix, ARRAY_SIZE(hpr_mix)),
SND_SOC_DAPM_MIXER("SPL MIXER", SND_SOC_NOPM, 0, 0, spl_mix, ARRAY_SIZE(spl_mix)),
SND_SOC_DAPM_MIXER("SPR MIXER", SND_SOC_NOPM, 0, 0, spr_mix, ARRAY_SIZE(spr_mix)),
SND_SOC_DAPM_MIXER("RC MIXER", SND_SOC_NOPM, 0, 0, rc_mix, ARRAY_SIZE(rc_mix)),
SND_SOC_DAPM_MIXER("LINEOUT1L MIXER", SND_SOC_NOPM, 0, 0, lout1l_mix, ARRAY_SIZE(lout1l_mix)),
SND_SOC_DAPM_MIXER("LINEOUT1R MIXER", SND_SOC_NOPM, 0, 0, lout1r_mix, ARRAY_SIZE(lout1r_mix)),
SND_SOC_DAPM_MIXER("LINEOUT2L MIXER", SND_SOC_NOPM, 0, 0, lout2l_mix, ARRAY_SIZE(lout2l_mix)),
SND_SOC_DAPM_MIXER("LINEOUT2R MIXER", SND_SOC_NOPM, 0, 0, lout2r_mix, ARRAY_SIZE(lout2r_mix)),

SND_SOC_DAPM_MIXER("DIGITAL MIXER", SND_SOC_NOPM, 0, 0, digital_mix, ARRAY_SIZE(digital_mix)),

SND_SOC_DAPM_MUX("DACMAIN SRC", SND_SOC_NOPM, 0, 0, &dacmain_mux),
SND_SOC_DAPM_MUX("DACVOICE SRC", SND_SOC_NOPM, 0, 0, &dacvoice_mux),
SND_SOC_DAPM_MUX("DIT0 SRC", SND_SOC_NOPM, 0, 0, &dit0_mux),
SND_SOC_DAPM_MUX("DIT1 SRC", SND_SOC_NOPM, 0, 0, &dit1_mux),
SND_SOC_DAPM_MUX("DIT2 SRC", SND_SOC_NOPM, 0, 0, &dit2_mux),

SND_SOC_DAPM_MUX("AE SRC", SND_SOC_NOPM, 0, 0, &ae_mux),

SND_SOC_DAPM_MUX("ADCL LINE MIXMODE", SND_SOC_NOPM, 0, 0, &adcl_line_mux),
SND_SOC_DAPM_MUX("ADCR LINE MIXMODE", SND_SOC_NOPM, 0, 0, &adcr_line_mux),

SND_SOC_DAPM_MUX("HPL LINE MIXMODE", SND_SOC_NOPM, 0, 0, &hpl_line_mux),
SND_SOC_DAPM_MUX("HPL DAC MIXMODE", SND_SOC_NOPM, 0, 0, &hpl_dac_mux),

SND_SOC_DAPM_MUX("SPL LINE MIXMODE", SND_SOC_NOPM, 0, 0, &spl_line_mux),
SND_SOC_DAPM_MUX("SPL DAC MIXMODE", SND_SOC_NOPM, 0, 0, &spl_dac_mux),
SND_SOC_DAPM_MUX("SPR LINE MIXMODE", SND_SOC_NOPM, 0, 0, &spr_line_mux),
SND_SOC_DAPM_MUX("SPR DAC MIXMODE", SND_SOC_NOPM, 0, 0, &spr_dac_mux),

SND_SOC_DAPM_MUX("LINEOUT1L LINE MIXMODE", SND_SOC_NOPM, 0, 0, &lout1l_line_mux),
SND_SOC_DAPM_MUX("LINEOUT1L DAC MIXMODE", SND_SOC_NOPM, 0, 0, &lout1l_dac_mux),

SND_SOC_DAPM_MUX("LINEOUT2L LINE MIXMODE", SND_SOC_NOPM, 0, 0, &lout2l_line_mux),
SND_SOC_DAPM_MUX("LINEOUT2L DAC MIXMODE", SND_SOC_NOPM, 0, 0, &lout2l_dac_mux),

SND_SOC_DAPM_MUX("AE PARAMETER SEL", SND_SOC_NOPM, 0, 0, &ae_param_mux),

SND_SOC_DAPM_MUX("ADC PDM SEL", SND_SOC_NOPM, 0, 0, &adc_pdm_mux),

SND_SOC_DAPM_SWITCH("MB1", SND_SOC_NOPM, 0, 0, &bias1_sw),
SND_SOC_DAPM_SWITCH("MB2", SND_SOC_NOPM, 0, 0, &bias2_sw),
SND_SOC_DAPM_SWITCH("MB3", SND_SOC_NOPM, 0, 0, &bias3_sw),
};

static const struct snd_soc_dapm_route intercon[] = {
	{"ADCL LINE MIXMODE", "LINEL", "LINEIN"},
	{"ADCL LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"ADCR LINE MIXMODE", "LINER", "LINEIN"},
	{"ADCR LINE MIXMODE", "LINEMONO", "LINEIN"},

	{"HPL LINE MIXMODE", "LINEL", "LINEIN"},
	{"HPL LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"HPL DAC MIXMODE", "DACL", "DAC"},
	{"HPL DAC MIXMODE", "DACMONO", "DAC"},

	{"SPL LINE MIXMODE", "LINEL", "LINEIN"},
	{"SPL LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"SPL DAC MIXMODE", "DACL", "DAC"},
	{"SPL DAC MIXMODE", "DACMONO", "DAC"},

	{"SPR LINE MIXMODE", "LINER", "LINEIN"},
	{"SPR LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"SPR DAC MIXMODE", "DACR", "DAC"},
	{"SPR DAC MIXMODE", "DACMONO", "DAC"},

	{"LINEOUT1L LINE MIXMODE", "LINEL", "LINEIN"},
	{"LINEOUT1L LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"LINEOUT1L DAC MIXMODE", "DACL", "DAC"},
	{"LINEOUT1L DAC MIXMODE", "DACMONO", "DAC"},

	{"LINEOUT2L LINE MIXMODE", "LINEL", "LINEIN"},
	{"LINEOUT2L LINE MIXMODE", "LINEMONO", "LINEIN"},
	{"LINEOUT2L DAC MIXMODE", "DACL", "DAC"},
	{"LINEOUT2L DAC MIXMODE", "DACMONO", "DAC"},

	{"ADCL MIXER", "Mic1 Switch", "MIC1"},
	{"ADCL MIXER", "Mic2 Switch", "MIC2"},
	{"ADCL MIXER", "Mic3 Switch", "MIC3"},
	{"ADCL MIXER", "Line Switch", "ADCL LINE MIXMODE"},

	{"ADCR MIXER", "Mic1 Switch", "MIC1"},
	{"ADCR MIXER", "Mic2 Switch", "MIC2"},
	{"ADCR MIXER", "Mic3 Switch", "MIC3"},
	{"ADCR MIXER", "Line Switch", "ADCR LINE MIXMODE"},

	{"HPL MIXER", "Mic1 Switch", "MIC1"},
	{"HPL MIXER", "Mic2 Switch", "MIC2"},
	{"HPL MIXER", "Mic3 Switch", "MIC3"},
	{"HPL MIXER", "Line Switch", "HPL LINE MIXMODE"},
	{"HPL MIXER", "Dac Switch", "HPL DAC MIXMODE"},

	{"HPR MIXER", "Mic1 Switch", "MIC1"},
	{"HPR MIXER", "Mic2 Switch", "MIC2"},
	{"HPR MIXER", "Mic3 Switch", "MIC3"},
	{"HPR MIXER", "LineR Switch", "LINEIN"},
	{"HPR MIXER", "DacR Switch", "DAC"},

	{"SPL MIXER", "Line Switch", "SPL LINE MIXMODE"},
	{"SPL MIXER", "Dac Switch", "SPL DAC MIXMODE"},

	{"SPR MIXER", "Line Switch", "SPR LINE MIXMODE"},
	{"SPR MIXER", "Dac Switch", "SPR DAC MIXMODE"},

	{"RC MIXER", "Mic1 Switch", "MIC1"},
	{"RC MIXER", "Mic2 Switch", "MIC2"},
	{"RC MIXER", "Mic3 Switch", "MIC3"},
	{"RC MIXER", "LineMono Switch", "LINEIN"},
	{"RC MIXER", "DacL Switch", "DAC"},
	{"RC MIXER", "DacR Switch", "DAC"},

	{"LINEOUT1L MIXER", "Mic1 Switch", "MIC1"},
	{"LINEOUT1L MIXER", "Mic2 Switch", "MIC2"},
	{"LINEOUT1L MIXER", "Mic3 Switch", "MIC3"},
	{"LINEOUT1L MIXER", "Line Switch", "LINEOUT1L LINE MIXMODE"},
	{"LINEOUT1L MIXER", "Dac Switch", "LINEOUT1L DAC MIXMODE"},

	{"LINEOUT1R MIXER", "Mic1 Switch", "MIC1"},
	{"LINEOUT1R MIXER", "Mic2 Switch", "MIC2"},
	{"LINEOUT1R MIXER", "Mic3 Switch", "MIC3"},
	{"LINEOUT1R MIXER", "LineR Switch", "LINEIN"},
	{"LINEOUT1R MIXER", "DacR Switch", "DAC"},

	{"LINEOUT2L MIXER", "Mic1 Switch", "MIC1"},
	{"LINEOUT2L MIXER", "Mic2 Switch", "MIC2"},
	{"LINEOUT2L MIXER", "Mic3 Switch", "MIC3"},

	{"LINEOUT2L MIXER", "Line Switch", "LINEOUT2L LINE MIXMODE"},
	{"LINEOUT2L MIXER", "Dac Switch", "LINEOUT2L DAC MIXMODE"},

	{"LINEOUT2R MIXER", "Mic1 Switch", "MIC1"},
	{"LINEOUT2R MIXER", "Mic2 Switch", "MIC2"},
	{"LINEOUT2R MIXER", "Mic3 Switch", "MIC3"},
	{"LINEOUT2R MIXER", "LineR Switch", "LINEIN"},
	{"LINEOUT2R MIXER", "DacR Switch", "DAC"},

	{"ADC", NULL, "ADCL MIXER"},
	{"ADC", NULL, "ADCR MIXER"},

	{"DIGITAL MIXER", "Adc Switch", "ADC PDM SEL"},
	{"DIGITAL MIXER", "Dir0 Switch", "DIR0"},
	{"DIGITAL MIXER", "Dir1 Switch", "DIR1"},
	{"DIGITAL MIXER", "Dir2 Switch", "DIR2"},

	{"AE SRC", "ADC", "ADCL MIXER"},
	{"AE SRC", "ADC", "ADCR MIXER"},
	{"AE SRC", "DIR0", "DIR0"},
	{"AE SRC", "DIR1", "DIR1"},
	{"AE SRC", "DIR2", "DIR2"},
	{"AE SRC", "MIX", "DIGITAL MIXER"},

	{"DACMAIN SRC", "ADC", "ADC PDM SEL"},
	{"DACMAIN SRC", "DIR0", "DIR0"},
	{"DACMAIN SRC", "DIR1", "DIR1"},
	{"DACMAIN SRC", "DIR2", "DIR2"},
	{"DACMAIN SRC", "MIX", "DIGITAL MIXER"},

	{"DACVOICE SRC", "ADC", "ADC PDM SEL"},
	{"DACVOICE SRC", "DIR0", "DIR0"},
	{"DACVOICE SRC", "DIR1", "DIR1"},
	{"DACVOICE SRC", "DIR2", "DIR2"},
	{"DACVOICE SRC", "MIX", "DIGITAL MIXER"},

	{"DIT0 SRC", "ADC", "ADC PDM SEL"},
	{"DIT0 SRC", "DIR0", "DIR0"},
	{"DIT0 SRC", "DIR1", "DIR1"},
	{"DIT0 SRC", "DIR2", "DIR2"},
	{"DIT0 SRC", "MIX", "DIGITAL MIXER"},

	{"DIT1 SRC", "ADC", "ADC PDM SEL"},
	{"DIT1 SRC", "DIR0", "DIR0"},
	{"DIT1 SRC", "DIR1", "DIR1"},
	{"DIT1 SRC", "DIR2", "DIR2"},
	{"DIT1 SRC", "MIX", "DIGITAL MIXER"},

	{"DIT2 SRC", "ADC", "ADC PDM SEL"},
	{"DIT2 SRC", "DIR0", "DIR0"},
	{"DIT2 SRC", "DIR1", "DIR1"},
	{"DIT2 SRC", "DIR2", "DIR2"},
	{"DIT2 SRC", "MIX", "DIGITAL MIXER"},

	{"AE PARAMETER SEL", "PARAM1", "AE1"},
	{"AE PARAMETER SEL", "PARAM2", "AE2"},
	{"AE PARAMETER SEL", "PARAM3", "AE3"},
	{"AE PARAMETER SEL", "PARAM4", "AE4"},
	{"AE PARAMETER SEL", "PARAM5", "AE5"},

	{"ADC PDM SEL", "ADC", "ADC"},
	{"ADC PDM SEL", "PDM", "PDM"},

	{"MB1", "Switch", "BIAS1"},
	{"MB2", "Switch", "BIAS2"},
	{"MB3", "Switch", "BIAS3"},

	{"MIC1", NULL, "MB1"},
	{"MIC2", NULL, "MB2"},
	{"MIC3", NULL, "MB3"},
};

#ifdef ALSA_VER_ANDROID_3_0
static int mc1n2_add_widgets(struct snd_soc_codec *codec)
{
	int err;

	err = snd_soc_dapm_new_controls(&codec->dapm, mc1n2_widgets,
				  ARRAY_SIZE(mc1n2_widgets));
	if(err < 0) {
		return err;
	}

	err = snd_soc_dapm_add_routes(&codec->dapm, intercon, ARRAY_SIZE(intercon));
	if(err < 0) {
		return err;
	}

	err = snd_soc_dapm_new_widgets(&codec->dapm);
	if(err < 0) {
		return err;
	}

	return 0;
}
#else
static int mc1n2_add_widgets(struct snd_soc_codec *codec)
{
	int err;

	err = snd_soc_dapm_new_controls(codec, mc1n2_widgets,
				  ARRAY_SIZE(mc1n2_widgets));
	if(err < 0) {
		return err;
	}

	err = snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));
	if(err < 0) {
		return err;
	}

	err = snd_soc_dapm_new_widgets(codec);
	if(err < 0) {
		return err;
	}

	return 0;
}
#endif

/*
 * Hwdep interface
 */
static int mc1n2_hwdep_open(struct snd_hwdep * hw, struct file *file)
{
	/* Nothing to do */
	return 0;
}

static int mc1n2_hwdep_release(struct snd_hwdep *hw, struct file *file)
{
	/* Nothing to do */
	return 0;
}

static int mc1n2_hwdep_map_error(int err)
{
	switch (err) {
	case MCDRV_SUCCESS:
		return 0;
	case MCDRV_ERROR_ARGUMENT:
		return -EINVAL;
	case MCDRV_ERROR_STATE:
		return -EBUSY;
	case MCDRV_ERROR_TIMEOUT:
		return -EIO;
	default:
		/* internal error */
		return -EIO;
	}
}

static int mc1n2_hwdep_ioctl_set_path(struct snd_soc_codec *codec,
				      void *info, unsigned int update)
{
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	MCDRV_CHANNEL *ch;
	int i, j;
	MCDRV_PATH_INFO *path = (MCDRV_PATH_INFO *) info;

	mutex_lock(&mc1n2->mutex);

	/* preserve DIR settings */
	for (i = 0; i < MC1N2_N_PATH_CHANNELS; i++) {
		ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[i]);
#ifdef DIO0_DAI_ENABLE
		switch ((ch->abSrcOnOff[3]) & 0x3) {
		case 1:
			mc1n2->port[0].dir[i] = 1;
			break;
		case 2:
			mc1n2->port[0].dir[i] = 0;
			break;
		}
#endif
#ifdef DIO1_DAI_ENABLE
		switch ((ch->abSrcOnOff[3] >> 2) & 0x3) {
		case 1:
			mc1n2->port[1].dir[i] = 1;
			break;
		case 2:
			mc1n2->port[1].dir[i] = 0;
			break;
		}
#endif
#ifdef DIO2_DAI_ENABLE
		switch ((ch->abSrcOnOff[3] >> 4) & 0x3) {
		case 1:
			mc1n2->port[2].dir[i] = 1;
			break;
		case 2:
			mc1n2->port[2].dir[i] = 0;
			break;
		}
#endif
	}

	/* preserve DIT settings */
#ifdef DIO0_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[0]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		mc1n2->port[0].dit.abSrcOnOff[j] |=
			ch->abSrcOnOff[j] & 0x55;
		mc1n2->port[0].dit.abSrcOnOff[j] &=
			~((ch->abSrcOnOff[j] & 0xaa) >> 1);
	}
#endif
#ifdef DIO1_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[1]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		mc1n2->port[1].dit.abSrcOnOff[j] |=
			ch->abSrcOnOff[j] & 0x55;
		mc1n2->port[1].dit.abSrcOnOff[j] &=
			~((ch->abSrcOnOff[j] & 0xaa) >> 1);
	}
#endif
#ifdef DIO2_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[2]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		mc1n2->port[2].dit.abSrcOnOff[j] |=
			ch->abSrcOnOff[j] & 0x55;
		mc1n2->port[2].dit.abSrcOnOff[j] &=
			~((ch->abSrcOnOff[j] & 0xaa) >> 1);
	}
#endif

	/* modify path */
	for (i = 0; i < MC1N2_N_PATH_CHANNELS; i++) {
		ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[i]);

#ifdef DIO0_DAI_ENABLE
		if (!mc1n2_is_in_playback(&mc1n2->port[0])) {
			ch->abSrcOnOff[3] &= ~(0x3);
		}
#endif
#ifdef DIO1_DAI_ENABLE
		if (!mc1n2_is_in_playback(&mc1n2->port[1])) {
			ch->abSrcOnOff[3] &= ~(0x3 << 2);
		}
#endif
#ifdef DIO2_DAI_ENABLE
		if (!mc1n2_is_in_playback(&mc1n2->port[2])) {
			ch->abSrcOnOff[3] &= ~(0x3 << 4);
		}
#endif
	}

#ifdef DIO0_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[0]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		if (!mc1n2_is_in_capture(&mc1n2->port[0])) {
			ch->abSrcOnOff[j] = 0;
		}
	}
#endif
#ifdef DIO1_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[1]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		if (!mc1n2_is_in_capture(&mc1n2->port[1])) {
			ch->abSrcOnOff[j] = 0;
		}
	}
#endif
#ifdef DIO2_DAI_ENABLE
	ch = (MCDRV_CHANNEL *)(info + mc1n2_path_channel_tbl[2]);
	for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
		if (!mc1n2_is_in_capture(&mc1n2->port[2])) {
			ch->abSrcOnOff[j] = 0;
		}
	}
#endif

	/* select mic path */
	if ((path->asAdc0[0].abSrcOnOff[0] & MCDRV_SRC0_MIC1_OFF) && (path->asAdc0[1].abSrcOnOff[0] & MCDRV_SRC0_MIC1_OFF)) {
		audio_ctrl_mic_bias_gpio(mc1n2->pdata, MAIN_MIC, 0);
	} else {
		audio_ctrl_mic_bias_gpio(mc1n2->pdata, MAIN_MIC, 1);
		mdelay(mc1n2->delay_mic1in);
	}

	if ((path->asAdc0[0].abSrcOnOff[0] & MCDRV_SRC0_MIC3_OFF) && (path->asAdc0[1].abSrcOnOff[0] & MCDRV_SRC0_MIC3_OFF)) {
		audio_ctrl_mic_bias_gpio(mc1n2->pdata, SUB_MIC, 0);
	} else
		audio_ctrl_mic_bias_gpio(mc1n2->pdata, SUB_MIC, 1);

	mutex_unlock(&mc1n2->mutex);

	return 0;
}

static int mc1n2_hwdep_ioctl_set_ae(struct snd_soc_codec *codec,
				    void *info, unsigned int update)
{
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	UINT8 onoff = ((MCDRV_AE_INFO *)info)->bOnOff;
	unsigned int mask = update & 0x0f;      /* bit mask for bOnOff */
	int i;

	struct mc1n2_ae_copy {
		UINT32 flag;
		size_t offset;
		size_t size;
	};

	struct mc1n2_ae_copy tbl[] = {
		{MCDRV_AEUPDATE_FLAG_BEX,
		 offsetof(MCDRV_AE_INFO, abBex), BEX_PARAM_SIZE},
		{MCDRV_AEUPDATE_FLAG_WIDE,
		 offsetof(MCDRV_AE_INFO, abWide), WIDE_PARAM_SIZE},
		{MCDRV_AEUPDATE_FLAG_DRC,
		 offsetof(MCDRV_AE_INFO, abDrc), DRC_PARAM_SIZE},
		{MCDRV_AEUPDATE_FLAG_EQ5,
		 offsetof(MCDRV_AE_INFO, abEq5), EQ5_PARAM_SIZE},
		{MCDRV_AEUPDATE_FLAG_EQ3,
		 offsetof(MCDRV_AE_INFO, abEq3), EQ3_PARAM_SIZE},
	};

	mutex_lock(&mc1n2->mutex);

	mc1n2->ae_store.bOnOff = (mc1n2->ae_store.bOnOff & ~mask) | onoff;

	for (i = 0; i < sizeof(tbl)/sizeof(struct mc1n2_ae_copy); i++) {
		if (update & tbl[i].flag) {
			memcpy((void *)&mc1n2->ae_store + tbl[i].offset,
			       info + tbl[i].offset, tbl[i].size);
		}
	}

	mutex_unlock(&mc1n2->mutex);

	return 0;
}

struct mc1n2_hwdep_func {
	int cmd;
	size_t size;
	int (*callback)(struct snd_soc_codec *, void *, unsigned int);
};

struct mc1n2_hwdep_func mc1n2_hwdep_func_map[] = {
	{0, 0, NULL},                                         /* INIT */
	{0, 0, NULL},                                         /* TERM */
	{MC1N2_IOCTL_NR_BOTH, sizeof(MCDRV_REG_INFO), NULL},  /* READ_REG */
	{0, 0, NULL},                                         /* WRITE_REG */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_PATH_INFO), NULL},  /* GET_PATH */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_PATH_INFO),
	 mc1n2_hwdep_ioctl_set_path},                         /* SET_PATH */
	{0, 0, NULL},                                         /* GET_VOLUME */
	{0, 0, NULL},                                         /* SET_VOLUME */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_DIO_INFO), NULL},   /* GET_DIGITALIO */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_DIO_INFO), NULL},   /* SET_DIGITALIO */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_DAC_INFO), NULL},   /* GET_DAC */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_DAC_INFO), NULL},   /* SET_DAC */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_ADC_INFO), NULL},   /* GET_ADC */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_ADC_INFO), NULL},   /* SET_ADC */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_SP_INFO), NULL},    /* GET_SP */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_SP_INFO), NULL},    /* SET_SP */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_DNG_INFO), NULL},   /* GET_DNG */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_DNG_INFO), NULL},   /* SET_DNG */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_AE_INFO),
	 mc1n2_hwdep_ioctl_set_ae},                           /* SET_AE */
	{0, 0, NULL},                                         /* SET_AE_EX */
	{0, 0, NULL},                                         /* SET_CDSP */
	{0, 0, NULL},                                         /* GET_CDSP_PARAM */
	{0, 0, NULL},                                         /* SET_CDSP_PARAM */
	{0, 0, NULL},                                         /* REG_CDSP_CB */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_PDM_INFO), NULL},   /* GET PDM */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_PDM_INFO), NULL},   /* SET_PDM */
	{0, 0, NULL},                                         /* SET_DTMF */
	{0, 0, NULL},                                         /* CONFIG_GP */
	{0, 0, NULL},                                         /* MASK_GP */
	{0, 0, NULL},                                         /* GETSET_GP */
	{0, 0, NULL},                                         /* GET_PEAK */
	{0, 0, NULL},                                         /* IRQ */
	{0, 0, NULL},                                         /* UPDATE_CLOCK */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_CLKSW_INFO), NULL}, /* SWITCH_CLOCK */
	{MC1N2_IOCTL_NR_GET, sizeof(MCDRV_SYSEQ_INFO), NULL}, /* GET SYSEQ */
	{MC1N2_IOCTL_NR_SET, sizeof(MCDRV_SYSEQ_INFO), NULL}, /* SET_SYSEQ */
};
#define MC1N2_HWDEP_N_FUNC_MAP \
	(sizeof(mc1n2_hwdep_func_map)/sizeof(struct mc1n2_hwdep_func))

static int mc1n2_hwdep_ioctl_get_ctrl(struct snd_soc_codec *codec,
				      struct mc1n2_ctrl_args *args)
{
	struct mc1n2_hwdep_func *func = &mc1n2_hwdep_func_map[args->dCmd];
	void *info;
	int err;

	if (func->cmd != MC1N2_IOCTL_NR_GET) {
		return -EINVAL;
	}

	if (!access_ok(VERIFY_WRITE, args->pvPrm, func->size)) {
		return -EFAULT;
	}

	if (!(info = kzalloc(func->size, GFP_KERNEL))) {
		return -ENOMEM;
	}

	err = _McDrv_Ctrl(args->dCmd, info, args->dPrm);
	err = mc1n2_hwdep_map_error(err);
	if (err < 0) {
		goto error;
	}

	if (func->callback) {                   /* call post-process */
		func->callback(codec, info, args->dPrm);
	}

	if (copy_to_user(args->pvPrm, info, func->size) != 0) {
		err = -EFAULT;
		goto error;
	}

error:
	kfree(info);
	return err;
}

static int mc1n2_hwdep_ioctl_set_ctrl(struct snd_soc_codec *codec,
				      struct mc1n2_ctrl_args *args)
{
	struct mc1n2_hwdep_func *func = &mc1n2_hwdep_func_map[args->dCmd];
	void *info;
	int err;

	if (func->cmd != MC1N2_IOCTL_NR_SET) {
		return -EINVAL;
	}

	if (!access_ok(VERIFY_READ, args->pvPrm, func->size)) {
		return -EFAULT;
	}

	if (!(info = kzalloc(func->size, GFP_KERNEL))) {
		return -ENOMEM;
	}

	if (copy_from_user(info, args->pvPrm, func->size) != 0) {
		kfree(info);
		return -EFAULT;
	}

	if (func->callback) {                   /* call pre-process */
		func->callback(codec, info, args->dPrm);
	}

	if (args->dCmd == MCDRV_SET_DIGITALIO) {
#ifdef DIO0_DAI_ENABLE
		args->dPrm &= ~(MCDRV_DIO0_COM_UPDATE_FLAG | MCDRV_DIO0_DIR_UPDATE_FLAG | MCDRV_DIO0_DIT_UPDATE_FLAG);
#endif
#ifdef DIO1_DAI_ENABLE
		args->dPrm &= ~(MCDRV_DIO1_COM_UPDATE_FLAG | MCDRV_DIO1_DIR_UPDATE_FLAG | MCDRV_DIO1_DIT_UPDATE_FLAG);
#endif
#ifdef DIO2_DAI_ENABLE
		args->dPrm &= ~(MCDRV_DIO2_COM_UPDATE_FLAG | MCDRV_DIO2_DIR_UPDATE_FLAG | MCDRV_DIO2_DIT_UPDATE_FLAG);
#endif
	}

	err = _McDrv_Ctrl(args->dCmd, info, args->dPrm);

	kfree(info);

	return mc1n2_hwdep_map_error(err);
}

static int mc1n2_hwdep_ioctl_read_reg(struct mc1n2_ctrl_args *args)
{
	struct mc1n2_hwdep_func *func = &mc1n2_hwdep_func_map[args->dCmd];
	MCDRV_REG_INFO info;
	int err;

	if (func->cmd != MC1N2_IOCTL_NR_BOTH) {
		return -EINVAL;
	}

	if (!access_ok(VERIFY_WRITE, args->pvPrm, sizeof(MCDRV_REG_INFO))) {
		return -EFAULT;
	}

	if (copy_from_user(&info, args->pvPrm, sizeof(MCDRV_REG_INFO)) != 0) {
		return -EFAULT;
	}

	err = _McDrv_Ctrl(args->dCmd, &info, args->dPrm);
	if (err != MCDRV_SUCCESS) {
		return mc1n2_hwdep_map_error(err);
	}

	if (copy_to_user(args->pvPrm, &info, sizeof(MCDRV_REG_INFO)) != 0) {
		return -EFAULT;
	}

	return 0;
}

static int mc1n2_hwdep_ioctl_notify(struct snd_soc_codec *codec,
				      struct mc1n2_ctrl_args *args)
{
	MCDRV_PATH_INFO path;
	int err;

#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif

	switch (args->dCmd) {
	case MCDRV_NOTIFY_CALL_START:
	case MCDRV_NOTIFY_2MIC_CALL_START:
		mc1n2_current_mode |= MC1N2_MODE_CALL_ON;
		err = mc1n2->pdata->set_adc_power_constraints(0);
		if (err < 0) {
			dev_err(codec->dev,
				"%s:%d:Error VADC_3.3V[On]\n", __func__, err);
		}
		break;
	case MCDRV_NOTIFY_CALL_STOP:
		mc1n2_current_mode &= ~MC1N2_MODE_CALL_ON;
		err = mc1n2->pdata->set_adc_power_constraints(1);
		if (err < 0) {
			dev_err(codec->dev,
				"%s:%d:Error VADC_3.3V[Off]\n", __func__, err);
		}
		break;
	case MCDRV_NOTIFY_MEDIA_PLAY_START:
		break;
	case MCDRV_NOTIFY_MEDIA_PLAY_STOP:
		break;
	case MCDRV_NOTIFY_FM_PLAY_START:
		mc1n2_current_mode |= MC1N2_MODE_FM_ON;
		break;
	case MCDRV_NOTIFY_FM_PLAY_STOP:
		mc1n2_current_mode &= ~MC1N2_MODE_FM_ON;
		break;
	case MCDRV_NOTIFY_BT_SCO_ENABLE:
		break;
	case MCDRV_NOTIFY_BT_SCO_DISABLE:
		break;
	case MCDRV_NOTIFY_VOICE_REC_START:
		mc1n2->delay_mic1in = MC1N2_WAITTIME_MICIN;
		break;
	case MCDRV_NOTIFY_VOICE_REC_STOP:
		mc1n2->delay_mic1in = 0;
		break;
	case MCDRV_NOTIFY_HDMI_START:
		if (mc1n2->hdmicount == 0) {
			memset(&path, 0, sizeof(path));
			path.asDit0[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asDit1[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asDit2[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asDac[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asDac[1].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asAe[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			path.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
			_McDrv_Ctrl(MCDRV_SET_PATH, &path, 0);
		}

		(mc1n2->hdmicount)++;
		break;
	case MCDRV_NOTIFY_HDMI_STOP:
		if (mc1n2->hdmicount != 0) {
			if (mc1n2->hdmicount == 1) {
				memset(&path, 0, sizeof(path));
				path.asDit0[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asDit1[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asDit2[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asDac[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asDac[1].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asAe[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				path.asMix[0].abSrcOnOff[3] = MCDRV_SRC3_DIR0_OFF;
				_McDrv_Ctrl(MCDRV_SET_PATH, &path, 0);
			}

			(mc1n2->hdmicount)--;
		}
		break;
	case MCDRV_NOTIFY_LINEOUT_START:
		mc1n2->lineoutenable = 1;
		break;
	case MCDRV_NOTIFY_LINEOUT_STOP:
		mc1n2->lineoutenable = 0;
		break;
	case MCDRV_NOTIFY_RECOVER:
		{
			int err, i;
			SINT16 *vol = (SINT16 *)&mc1n2->vol_store;

			mutex_lock(&mc1n2->mutex);

			/* store parameters */
			for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
				struct mc1n2_info_store *store = &mc1n2_info_store_tbl[i];
				if (store->get) {
					err = _McDrv_Ctrl(store->get, (void *)mc1n2 + store->offset, 0);
					if (err != MCDRV_SUCCESS) {
						dev_err(codec->dev,
							"%d: Error in MCDRV_GET_xxx\n", err);
						err = -EIO;
						goto error_recover;
					} else {
						err = 0;
					}
				}
			}

			err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
			if (err != MCDRV_SUCCESS) {
				dev_err(codec->dev, "%d: Error in MCDRV_TERM\n", err);
				err = -EIO;
			} else {
				err = 0;
			}

			err = _McDrv_Ctrl(MCDRV_INIT, &mc1n2->setup.init, 0);
			if (err != MCDRV_SUCCESS) {
				dev_err(codec->dev, "%d: Error in MCDRV_INIT\n", err);
				err = -EIO;
				goto error_recover;
			} else {
				err = 0;
			}

			/* restore parameters */
			for (i = 0; i < sizeof(MCDRV_VOL_INFO)/sizeof(SINT16); i++, vol++) {
				*vol |= 0x0001;
			}

			for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
				struct mc1n2_info_store *store = &mc1n2_info_store_tbl[i];
				if (store->set) {
					err = _McDrv_Ctrl(store->set, (void *)mc1n2 + store->offset,
							  store->flags);
					if (err != MCDRV_SUCCESS) {
						dev_err(codec->dev,
							"%d: Error in MCDRV_SET_xxx\n", err);
						err = -EIO;
						goto error_recover;
					} else {
						err = 0;
					}
				}
			}

error_recover:
			mutex_unlock(&mc1n2->mutex);
			return err;
			break;
		}
	}

	return 0;
}

static int mc1n2_hwdep_ioctl(struct snd_hwdep *hw, struct file *file,
			     unsigned int cmd, unsigned long arg)
{
	struct mc1n2_ctrl_args ctrl_args;
	struct snd_soc_codec *codec = hw->private_data;
	int err;

	if (!access_ok(VERIFY_READ, (struct mc1n2_ctrl_args *)arg,
		       sizeof(struct mc1n2_ctrl_args))) {
		return -EFAULT;
	}

	if (copy_from_user(&ctrl_args, (struct mc1n2_ctrl_args *)arg,
			   sizeof(struct mc1n2_ctrl_args)) != 0) {
		return -EFAULT;
	}

	if (cmd == MC1N2_IOCTL_NOTIFY) {
		return mc1n2_hwdep_ioctl_notify(codec, &ctrl_args);
	}

	if (ctrl_args.dCmd >= MC1N2_HWDEP_N_FUNC_MAP) {
		return -EINVAL;
	}

	switch (cmd) {
	case MC1N2_IOCTL_GET_CTRL:
		err = mc1n2_hwdep_ioctl_get_ctrl(codec, &ctrl_args);
		break;
	case MC1N2_IOCTL_SET_CTRL:
		err = mc1n2_hwdep_ioctl_set_ctrl(codec, &ctrl_args);
		break;
	case MC1N2_IOCTL_READ_REG:
		err = mc1n2_hwdep_ioctl_read_reg(&ctrl_args);
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static int mc1n2_add_hwdep(struct snd_soc_codec *codec)
{
	struct snd_hwdep *hw;
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	int err;

#ifdef ALSA_VER_ANDROID_3_0
	err = snd_hwdep_new((struct snd_card *)codec->card->snd_card,
						MC1N2_HWDEP_ID, 0, &hw);
#else
	err = snd_hwdep_new(codec->card, MC1N2_HWDEP_ID, 0, &hw);
#endif
	if (err < 0) {
		return err;
	}

	hw->iface = SNDRV_HWDEP_IFACE_MC1N2;
	hw->private_data = codec;
	hw->ops.open = mc1n2_hwdep_open;
	hw->ops.release = mc1n2_hwdep_release;
	hw->ops.ioctl = mc1n2_hwdep_ioctl;
	hw->exclusive = 1;
	strcpy(hw->name, MC1N2_HWDEP_ID);
	mc1n2->hwdep = hw;

	return 0;
}

/*
 * Codec device
 */
#ifdef ALSA_VER_ANDROID_3_0
static int mc1n2_probe(struct snd_soc_codec *codec)
#else
static int mc1n2_probe(struct platform_device *pdev)
#endif
{
#ifdef ALSA_VER_ANDROID_3_0
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
	struct device *dev = codec->dev;
	struct mc1n2_setup *setup = &mc1n2_cfg_setup;
#else
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = mc1n2_get_codec_data();
#ifdef ALSA_VER_ANDROID_2_6_35
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
	struct mc1n2_setup *setup = socdev->codec_data;
	struct device *dev = socdev->dev;
#endif
	int err;
	UINT32 update = 0;

	TRACE_FUNC();

	if (!codec) {
		dev_err(dev, "I2C bus is not probed successfully\n");
		err = -ENODEV;
		goto error_codec_data;
	}
#ifndef ALSA_VER_ANDROID_3_0
#ifdef ALSA_VER_1_0_19
	socdev->codec = codec;
#else
	socdev->card->codec = codec;
#endif
#endif

	/* init hardware */
	if (!setup) {
		dev_err(dev, "No initialization parameters given\n");
		err = -EINVAL;
		goto error_init_hw;
	}
	memcpy(&mc1n2->setup, setup, sizeof(struct mc1n2_setup));
	err = _McDrv_Ctrl(MCDRV_INIT, &mc1n2->setup.init, 0);
	if (err != MCDRV_SUCCESS) {
		dev_err(dev, "%d: Error in MCDRV_INIT\n", err);
		err = -EIO;
		goto error_init_hw;
	}

	/* pcm */
#ifndef ALSA_VER_ANDROID_3_0
	err = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (err < 0) {
		dev_err(dev, "%d: Error in snd_soc_new_pcms\n", err);
		goto error_new_pcm;
	}
#endif

	/* controls */
	err = mc1n2_add_controls(codec, mc1n2_snd_controls,
				 ARRAY_SIZE(mc1n2_snd_controls));
	if (err < 0) {
		dev_err(dev, "%d: Error in mc1n2_add_controls\n", err);
		goto error_add_ctl;
	}

	err = mc1n2_add_widgets(codec);
	if (err < 0) {
		dev_err(dev, "%d: Error in mc1n2_add_widgets\n", err);
		goto error_add_ctl;
	}

	/* hwdep */
	err = mc1n2_add_hwdep(codec);
	if (err < 0) {
		dev_err(dev, "%d: Error in mc1n2_add_hwdep\n", err);
		goto error_add_hwdep;
	}

#if (defined ALSA_VER_1_0_19) || (defined ALSA_VER_1_0_21)
	err = snd_soc_init_card(socdev);
	if (err < 0) {
		dev_err(dev, "%d: Error in snd_soc_init_card\n", err);
		goto error_init_card;
	}
#endif

#ifndef DIO0_DAI_ENABLE
	update |= (MCDRV_DIO0_COM_UPDATE_FLAG | MCDRV_DIO0_DIR_UPDATE_FLAG | MCDRV_DIO0_DIT_UPDATE_FLAG);
#endif

#ifndef DIO1_DAI_ENABLE
	update |= (MCDRV_DIO1_COM_UPDATE_FLAG | MCDRV_DIO1_DIR_UPDATE_FLAG | MCDRV_DIO1_DIT_UPDATE_FLAG);
#endif

#ifndef DIO2_DAI_ENABLE
	update |= (MCDRV_DIO2_COM_UPDATE_FLAG | MCDRV_DIO2_DIR_UPDATE_FLAG | MCDRV_DIO2_DIT_UPDATE_FLAG);
#endif

	err = _McDrv_Ctrl(MCDRV_SET_DIGITALIO, (void *)&stDioInfo_Default, update);
	if (err < 0) {
		dev_err(dev, "%d: Error in MCDRV_SET_DIGITALIO\n", err);
		goto error_set_mode;
	}

	err = _McDrv_Ctrl(MCDRV_SET_DAC, (void *)&stDacInfo_Default, 0x7);
	if (err < 0) {
		dev_err(dev, "%d: Error in MCDRV_SET_DAC\n", err);
		goto error_set_mode;
	}

	err = _McDrv_Ctrl(MCDRV_SET_ADC, (void *)&stAdcInfo_Default, 0x7);
	if (err < 0) {
		dev_err(dev, "%d: Error in MCDRV_SET_ADC\n", err);
		goto error_set_mode;
	}

	err = _McDrv_Ctrl(MCDRV_SET_SP, (void *)&stSpInfo_Default, 0);
	if (err < 0) {
		dev_err(dev, "%d: Error in MCDRV_SET_SP\n", err);
		goto error_set_mode;
	}

	err = _McDrv_Ctrl(MCDRV_SET_DNG, (void *)&stDngInfo_Default, 0x3F3F3F);
	if (err < 0) {
		dev_err(dev, "%d: Error in MCDRV_SET_DNG\n", err);
		goto error_set_mode;
	}

	if (mc1n2_hwid == MC1N2_HW_ID_AB) {
		err = _McDrv_Ctrl(MCDRV_SET_SYSEQ, (void *)&stSyseqInfo_Default, 0x3);

		if (err < 0) {
			dev_err(dev, "%d: Error in MCDRV_SET_SYSEQ\n", err);
			goto error_set_mode;
		}
	}

	return 0;

error_set_mode:
#if (defined ALSA_VER_1_0_19) || (defined ALSA_VER_1_0_21)
error_init_card:
#endif
error_add_hwdep:
error_add_ctl:
#ifndef ALSA_VER_ANDROID_3_0
	snd_soc_free_pcms(socdev);
error_new_pcm:
#endif
	_McDrv_Ctrl(MCDRV_TERM, NULL, 0);
error_init_hw:
#ifndef ALSA_VER_ANDROID_3_0
#ifdef ALSA_VER_1_0_19
	socdev->codec = NULL;
#else
	socdev->card->codec = NULL;
#endif
#endif
error_codec_data:
	return err;
}

#ifdef ALSA_VER_ANDROID_3_0
static int mc1n2_remove(struct snd_soc_codec *codec)
{
	int err;

	TRACE_FUNC();

	if (codec) {
		err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
		if (err != MCDRV_SUCCESS) {
			dev_err(codec->dev, "%d: Error in MCDRV_TERM\n", err);
			return -EIO;
		}
	}
	return 0;
}
#else
static int mc1n2_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	int err;

	TRACE_FUNC();

#ifdef ALSA_VER_1_0_19
	if (socdev->codec) {
#else
	if (socdev->card->codec) {
#endif
		snd_soc_free_pcms(socdev);

		err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
		if (err != MCDRV_SUCCESS) {
			dev_err(socdev->dev, "%d: Error in MCDRV_TERM\n", err);
			return -EIO;
		}
	}

	return 0;
}
#endif

#ifdef ALSA_VER_ANDROID_3_0
static int mc1n2_suspend(struct snd_soc_codec *codec, pm_message_t state)
#else
static int mc1n2_suspend(struct platform_device *pdev, pm_message_t state)
#endif
{
#ifdef ALSA_VER_ANDROID_3_0
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
#ifdef ALSA_VER_1_0_19
	struct snd_soc_codec *codec = socdev->codec;
#else
	struct snd_soc_codec *codec = socdev->card->codec;
#endif
#ifdef ALSA_VER_ANDROID_2_6_35
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
#endif
	int err, i;

	TRACE_FUNC();

	mutex_lock(&mc1n2->mutex);

	/* store parameters */
	for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
		struct mc1n2_info_store *store = &mc1n2_info_store_tbl[i];
		if (store->get) {
			err = _McDrv_Ctrl(store->get, (void *)mc1n2 + store->offset, 0);
			if (err != MCDRV_SUCCESS) {
				dev_err(codec->dev,
					"%d: Error in mc1n2_suspend\n", err);
				err = -EIO;
				goto error;
			} else {
				err = 0;
			}
		}
	}

	/* Do not enter suspend mode for voice call */
	if(mc1n2_current_mode != MC1N2_MODE_IDLE) {
		err = 0;
		goto error;
	}

	err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in MCDRV_TERM\n", err);
		err = -EIO;
	} else {
		err = 0;
	}

	/* Suepend MCLK */
	mc1n2_set_mclk_source(0);

error:
	mutex_unlock(&mc1n2->mutex);

	return err;
}

#ifdef ALSA_VER_ANDROID_3_0
static int mc1n2_resume(struct snd_soc_codec *codec)
#else
static int mc1n2_resume(struct platform_device *pdev)
#endif
{
#ifdef ALSA_VER_ANDROID_3_0
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
#ifdef ALSA_VER_1_0_19
	struct snd_soc_codec *codec = socdev->codec;
#else
	struct snd_soc_codec *codec = socdev->card->codec;
#endif
#ifdef ALSA_VER_ANDROID_2_6_35
	struct mc1n2_data *mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	struct mc1n2_data *mc1n2 = codec->private_data;
#endif
#endif
	SINT16 *vol = (SINT16 *)&mc1n2->vol_store;
	int err, i;

	TRACE_FUNC();

	mutex_lock(&mc1n2->mutex);

	/* Resume MCLK */
	mc1n2_set_mclk_source(1);

	err = _McDrv_Ctrl(MCDRV_INIT, &mc1n2->setup.init, 0);
	if (err != MCDRV_SUCCESS) {
		dev_err(codec->dev, "%d: Error in MCDRV_INIT\n", err);
		err = -EIO;
		goto error;
	} else {
		err = 0;
	}

	/* restore parameters */
	for (i = 0; i < sizeof(MCDRV_VOL_INFO)/sizeof(SINT16); i++, vol++) {
		*vol |= 0x0001;
	}

	for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
		struct mc1n2_info_store *store = &mc1n2_info_store_tbl[i];
		if (store->set) {
			err = _McDrv_Ctrl(store->set, (void *)mc1n2 + store->offset,
					  store->flags);
			if (err != MCDRV_SUCCESS) {
				dev_err(codec->dev,
					"%d: Error in mc1n2_resume\n", err);
				err = -EIO;
				goto error;
			} else {
				err = 0;
			}
		}
	}

error:
	mutex_unlock(&mc1n2->mutex);

	return err;
}

#ifdef ALSA_VER_ANDROID_3_0
struct snd_soc_codec_driver soc_codec_dev_mc1n2 = {
	.probe = mc1n2_probe,
	.remove = mc1n2_remove,
	.suspend = mc1n2_suspend,
	.resume = mc1n2_resume,
	.read = mc1n2_read_reg,
	.write = mc1n2_write_reg,
	.reg_cache_size = MC1N2_N_REG,
	.reg_word_size = sizeof(u16),
	.reg_cache_step = 1
};
#else
struct snd_soc_codec_device soc_codec_dev_mc1n2 = {
	.probe = mc1n2_probe,
	.remove = mc1n2_remove,
	.suspend = mc1n2_suspend,
	.resume = mc1n2_resume
};
EXPORT_SYMBOL_GPL(soc_codec_dev_mc1n2);
#endif

/*
 * I2C client
 */
static int mc1n2_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	UINT8	bHwid = mc1n2_i2c_read_byte(client, 8);

	if (bHwid != MC1N2_HW_ID_AB && bHwid != MC1N2_HW_ID_AA) {
		return -ENODEV;
	}
	mc1n2_hwid = bHwid;

	return 0;
}

static int mc1n2_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct snd_soc_codec *codec;
	struct mc1n2_data *mc1n2;
	int err;

	TRACE_FUNC();

	/* setup codec data */
	if (!(codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL))) {
		err = -ENOMEM;
		goto err_alloc_codec;
	}
	codec->name = MC1N2_NAME;
//	codec->owner = THIS_MODULE;
	mutex_init(&codec->mutex);
	codec->dev = &client->dev;

	if (!(mc1n2 = kzalloc(sizeof(struct mc1n2_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto err_alloc_data;
	}
	mutex_init(&mc1n2->mutex);
#if (defined ALSA_VER_ANDROID_2_6_35) || (defined ALSA_VER_ANDROID_3_0)
	snd_soc_codec_set_drvdata(codec, mc1n2);
#else
	codec->private_data = mc1n2;
#endif

	mc1n2->hdmicount = 0;
	mc1n2->lineoutenable = 0;
	mc1n2->pdata = client->dev.platform_data;

	/* setup i2c client data */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto err_i2c;
	}

	if ((err = mc1n2_i2c_detect(client, NULL)) < 0) {
		goto err_i2c;
	}


#ifdef ALSA_VER_ANDROID_3_0
	i2c_set_clientdata(client, mc1n2);
#else
	i2c_set_clientdata(client, codec);

	codec->control_data = client;
	codec->read = mc1n2_read_reg;
	codec->write = mc1n2_write_reg;
	codec->hw_write = NULL;
	codec->hw_read = NULL;
	codec->reg_cache = kzalloc(sizeof(u16) * MC1N2_N_REG, GFP_KERNEL);
	if (codec->reg_cache == NULL) {
		err = -ENOMEM;
		goto err_alloc_cache;
	}
	codec->reg_cache_size = MC1N2_N_REG;
	codec->reg_cache_step = 1;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	codec->dai = mc1n2_dai;
	codec->num_dai = ARRAY_SIZE(mc1n2_dai);
	mc1n2_set_codec_data(codec);
#endif

#ifdef ALSA_VER_ANDROID_3_0
	if ((err = snd_soc_register_codec(&client->dev, &soc_codec_dev_mc1n2,
									  mc1n2_dai, ARRAY_SIZE(mc1n2_dai))) < 0) {
		goto err_reg_codec;
	}

	mc1n2_i2c = client;
#else
	if ((err = snd_soc_register_codec(codec)) < 0) {
		goto err_reg_codec;
	}

	/* setup DAI data */
	for (i = 0; i < ARRAY_SIZE(mc1n2_dai); i++) {
		mc1n2_dai[i].dev = &client->dev;
	}
        if ((err = snd_soc_register_dais(mc1n2_dai, ARRAY_SIZE(mc1n2_dai))) < 0) {
		goto err_reg_dai;
	}
#endif

	return 0;

#ifndef ALSA_VER_ANDROID_3_0
err_reg_dai:
	snd_soc_unregister_codec(codec);
#endif
err_reg_codec:
#ifndef ALSA_VER_ANDROID_3_0
	kfree(codec->reg_cache);
err_alloc_cache:
#endif
	i2c_set_clientdata(client, NULL);
err_i2c:
	kfree(mc1n2);
err_alloc_data:
	kfree(codec);
err_alloc_codec:
	dev_err(&client->dev, "err=%d: failed to probe MC-1N2\n", err);
	return err;
}

static int mc1n2_i2c_remove(struct i2c_client *client)
{
#ifndef ALSA_VER_ANDROID_3_0
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
#endif
	struct mc1n2_data *mc1n2;

	TRACE_FUNC();

#ifdef ALSA_VER_ANDROID_3_0
	mc1n2 = (struct mc1n2_data*)(i2c_get_clientdata(client));
	mutex_destroy(&mc1n2->mutex);
	snd_soc_unregister_codec(&client->dev);
#else
	if (codec) {
#ifdef ALSA_VER_ANDROID_2_6_35
		mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
		mc1n2 = codec->private_data;
#endif
		snd_soc_unregister_dais(mc1n2_dai, ARRAY_SIZE(mc1n2_dai));
		snd_soc_unregister_codec(codec);

		mutex_destroy(&mc1n2->mutex);
		kfree(mc1n2);

		mutex_destroy(&codec->mutex);
		kfree(codec);
	}
#endif

	return 0;
}

/*
 * Function to prevent tick-noise when reboot menu selected.
 * if you have Power-Off sound and same problem, use this function
 */
static void mc1n2_i2c_shutdown(struct i2c_client *client)
{
#ifndef ALSA_VER_ANDROID_3_0
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
#endif
	struct mc1n2_data *mc1n2;
	int err, i;

	pr_info("%s\n", __func__);

	TRACE_FUNC();

#ifdef ALSA_VER_ANDROID_3_0
	mc1n2 = (struct mc1n2_data *)(i2c_get_clientdata(client));
#else
#ifdef ALSA_VER_ANDROID_2_6_35
	mc1n2 = snd_soc_codec_get_drvdata(codec);
#else
	mc1n2 = codec->private_data;
#endif
#endif

	mutex_lock(&mc1n2->mutex);

	/* store parameters */
	for (i = 0; i < MC1N2_N_INFO_STORE; i++) {
		struct mc1n2_info_store *store = &mc1n2_info_store_tbl[i];
		if (store->get) {
			err = _McDrv_Ctrl(store->get,
				(void *)mc1n2 + store->offset, 0);
			if (err != MCDRV_SUCCESS) {
				pr_err("%d: Error in mc1n2_suspend\n", err);
				err = -EIO;
				goto error;
			} else {
				err = 0;
			}
		}
	}

	/* Do not enter suspend mode for voice call */
	if (mc1n2_current_mode != MC1N2_MODE_IDLE) {
		err = 0;
		goto error;
	}

	err = _McDrv_Ctrl(MCDRV_TERM, NULL, 0);
	if (err != MCDRV_SUCCESS) {
		pr_err("%d: Error in MCDRV_TERM\n", err);
		err = -EIO;
	} else {
		err = 0;
	}

	/* Suepend MCLK */
	mc1n2_set_mclk_source(0);

	pr_info("%s done\n", __func__);

error:
	mutex_unlock(&mc1n2->mutex);

	if (err != 0)
		pr_err("%s: err = %d\n", __func__, err);

	return;
}

static const struct i2c_device_id mc1n2_i2c_id[] = {
	{MC1N2_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, mc1n2_i2c_id);

static struct i2c_driver mc1n2_i2c_driver = {
	.driver = {
		.name = MC1N2_NAME,
		.owner = THIS_MODULE,
	},
	.probe = mc1n2_i2c_probe,
	.remove = mc1n2_i2c_remove,
	.shutdown = mc1n2_i2c_shutdown,
	.id_table = mc1n2_i2c_id,
};

/*
 * Module init and exit
 */
static int __init mc1n2_init(void)
{
	return i2c_add_driver(&mc1n2_i2c_driver);
}
module_init(mc1n2_init);

static void __exit mc1n2_exit(void)
{
	i2c_del_driver(&mc1n2_i2c_driver);
}
module_exit(mc1n2_exit);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_DESCRIPTION("Yamaha MC-1N2 ALSA SoC codec driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(MC1N2_DRIVER_VERSION);
