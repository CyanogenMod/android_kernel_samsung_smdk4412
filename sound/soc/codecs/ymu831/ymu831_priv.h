/*
 * YMU831 ASoC codec driver - private header
 *
 * Copyright (c) 2012-2013 Yamaha Corporation
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

#ifndef YMU831_PRIV_H
#define YMU831_PRIV_H

#include <sound/soc.h>
#include "mcdriver.h"

#undef	MC_ASOC_TEST

/*
 * Virtual registers
 */
enum {
	MC_ASOC_DVOL_MUSICIN,
	MC_ASOC_DVOL_EXTIN,
	MC_ASOC_DVOL_VOICEIN,
	MC_ASOC_DVOL_REFIN,
	MC_ASOC_DVOL_ADIF0IN,
	MC_ASOC_DVOL_ADIF1IN,
	MC_ASOC_DVOL_ADIF2IN,
	MC_ASOC_DVOL_MUSICOUT,
	MC_ASOC_DVOL_EXTOUT,
	MC_ASOC_DVOL_VOICEOUT,
	MC_ASOC_DVOL_REFOUT,
	MC_ASOC_DVOL_DAC0OUT,
	MC_ASOC_DVOL_DAC1OUT,
	MC_ASOC_DVOL_DPATHDA,
	MC_ASOC_DVOL_DPATHAD,
	MC_ASOC_AVOL_LINEIN1,
	MC_ASOC_AVOL_MIC1,
	MC_ASOC_AVOL_MIC2,
	MC_ASOC_AVOL_MIC3,
	MC_ASOC_AVOL_MIC4,
	MC_ASOC_AVOL_HP,
	MC_ASOC_AVOL_SP,
	MC_ASOC_AVOL_RC,
	MC_ASOC_AVOL_LINEOUT1,
	MC_ASOC_AVOL_LINEOUT2,
	MC_ASOC_AVOL_SP_GAIN,

	MC_ASOC_DVOL_MASTER,
	MC_ASOC_DVOL_VOICE,
	MC_ASOC_DVOL_APLAY_A,
	MC_ASOC_DVOL_APLAY_D,

	MC_ASOC_VOICE_RECORDING,

	MC_ASOC_AUDIO_MODE_PLAY,
	MC_ASOC_AUDIO_MODE_CAP,
	MC_ASOC_OUTPUT_PATH,
	MC_ASOC_INPUT_PATH,
	MC_ASOC_INCALL_MIC_SP,
	MC_ASOC_INCALL_MIC_RC,
	MC_ASOC_INCALL_MIC_HP,
	MC_ASOC_INCALL_MIC_LO1,
	MC_ASOC_INCALL_MIC_LO2,

	MC_ASOC_MAINMIC_PLAYBACK_PATH,
	MC_ASOC_SUBMIC_PLAYBACK_PATH,
	MC_ASOC_2MIC_PLAYBACK_PATH,
	MC_ASOC_HSMIC_PLAYBACK_PATH,
	MC_ASOC_BTMIC_PLAYBACK_PATH,
	MC_ASOC_LIN1_PLAYBACK_PATH,

	MC_ASOC_DTMF_CONTROL,
	MC_ASOC_DTMF_OUTPUT,

	MC_ASOC_SWITCH_CLOCK,

	MC_ASOC_EXT_MASTERSLAVE,
	MC_ASOC_EXT_RATE,
	MC_ASOC_EXT_BITCLOCK_RATE,
	MC_ASOC_EXT_INTERFACE,
	MC_ASOC_EXT_BITCLOCK_INVERT,
	MC_ASOC_EXT_INPUT_DA_BIT_WIDTH,
	MC_ASOC_EXT_INPUT_DA_FORMAT,
	MC_ASOC_EXT_INPUT_PCM_MONOSTEREO,
	MC_ASOC_EXT_INPUT_PCM_BIT_ORDER,
	MC_ASOC_EXT_INPUT_PCM_FORMAT,
	MC_ASOC_EXT_INPUT_PCM_BIT_WIDTH,
	MC_ASOC_EXT_OUTPUT_DA_BIT_WIDTH,
	MC_ASOC_EXT_OUTPUT_DA_FORMAT,
	MC_ASOC_EXT_OUTPUT_PCM_MONOSTEREO,
	MC_ASOC_EXT_OUTPUT_PCM_BIT_ORDER,
	MC_ASOC_EXT_OUTPUT_PCM_FORMAT,
	MC_ASOC_EXT_OUTPUT_PCM_BIT_WIDTH,

	MC_ASOC_VOICE_MASTERSLAVE,
	MC_ASOC_VOICE_RATE,
	MC_ASOC_VOICE_BITCLOCK_RATE,
	MC_ASOC_VOICE_INTERFACE,
	MC_ASOC_VOICE_BITCLOCK_INVERT,
	MC_ASOC_VOICE_INPUT_DA_BIT_WIDTH,
	MC_ASOC_VOICE_INPUT_DA_FORMAT,
	MC_ASOC_VOICE_INPUT_PCM_MONOSTEREO,
	MC_ASOC_VOICE_INPUT_PCM_BIT_ORDER,
	MC_ASOC_VOICE_INPUT_PCM_FORMAT,
	MC_ASOC_VOICE_INPUT_PCM_BIT_WIDTH,
	MC_ASOC_VOICE_OUTPUT_DA_BIT_WIDTH,
	MC_ASOC_VOICE_OUTPUT_DA_FORMAT,
	MC_ASOC_VOICE_OUTPUT_PCM_MONOSTEREO,
	MC_ASOC_VOICE_OUTPUT_PCM_BIT_ORDER,
	MC_ASOC_VOICE_OUTPUT_PCM_FORMAT,
	MC_ASOC_VOICE_OUTPUT_PCM_BIT_WIDTH,

	MC_ASOC_MUSIC_PHYSICAL_PORT,
	MC_ASOC_EXT_PHYSICAL_PORT,
	MC_ASOC_VOICE_PHYSICAL_PORT,
	MC_ASOC_HIFI_PHYSICAL_PORT,

	MC_ASOC_ADIF0_SWAP,
	MC_ASOC_ADIF1_SWAP,
	MC_ASOC_ADIF2_SWAP,

	MC_ASOC_DAC0_SWAP,
	MC_ASOC_DAC1_SWAP,

	MC_ASOC_MUSIC_OUT0_SWAP,

	MC_ASOC_MUSIC_IN0_SWAP,
	MC_ASOC_MUSIC_IN1_SWAP,
	MC_ASOC_MUSIC_IN2_SWAP,

	MC_ASOC_EXT_IN_SWAP,

	MC_ASOC_VOICE_IN_SWAP,

	MC_ASOC_MUSIC_OUT1_SWAP,
	MC_ASOC_MUSIC_OUT2_SWAP,

	MC_ASOC_EXT_OUT_SWAP,

	MC_ASOC_VOICE_OUT_SWAP,

	MC_ASOC_ADIF0_SOURCE,
	MC_ASOC_ADIF1_SOURCE,
	MC_ASOC_ADIF2_SOURCE,

	MC_ASOC_CLEAR_DSP_PARAM,

	MC_ASOC_PARAMETER_SETTING,

	MC_ASOC_MAIN_MIC,
	MC_ASOC_SUB_MIC,
	MC_ASOC_HS_MIC,
#ifdef MC_ASOC_TEST
	MC_ASOC_MIC1_BIAS,
	MC_ASOC_MIC2_BIAS,
	MC_ASOC_MIC3_BIAS,
	MC_ASOC_MIC4_BIAS,
#endif

	MC_ASOC_N_REG
};
#define MC_ASOC_N_VOL_REG			(MC_ASOC_DVOL_APLAY_D+1)


#define MC_ASOC_AUDIO_MODE_OFF			(0)
#define MC_ASOC_AUDIO_MODE_AUDIO		(1)
#define MC_ASOC_AUDIO_MODE_INCALL		(2)
#define MC_ASOC_AUDIO_MODE_AUDIO_INCALL		(3)
#define MC_ASOC_AUDIO_MODE_INCOMM		(4)
#define MC_ASOC_AUDIO_MODE_KARAOKE		(5)
#define MC_ASOC_AUDIO_MODE_INCALL2		(6)
#define MC_ASOC_AUDIO_MODE_AUDIO_INCALL2	(7)
#define MC_ASOC_AUDIO_MODE_INCOMM2		(8)
#define MC_ASOC_AUDIO_MODE_INCALL3		(9)
#define MC_ASOC_AUDIO_MODE_AUDIO_INCALL3	(10)
#define MC_ASOC_AUDIO_MODE_INCALL4		(11)
#define MC_ASOC_AUDIO_MODE_AUDIO_INCALL4	(12)

#define MC_ASOC_AUDIO_MODE_AUDIOEX		(5)
#define MC_ASOC_AUDIO_MODE_AUDIOVR		(6)

#define MC_ASOC_OUTPUT_PATH_SP			(0)
#define MC_ASOC_OUTPUT_PATH_RC			(1)
#define MC_ASOC_OUTPUT_PATH_HP			(2)
#define MC_ASOC_OUTPUT_PATH_HS			(3)
#define MC_ASOC_OUTPUT_PATH_LO1			(4)
#define MC_ASOC_OUTPUT_PATH_LO2			(5)
#define MC_ASOC_OUTPUT_PATH_BT			(6)
#define MC_ASOC_OUTPUT_PATH_SP_RC		(7)
#define MC_ASOC_OUTPUT_PATH_SP_HP		(8)
#define MC_ASOC_OUTPUT_PATH_SP_LO1		(9)
#define MC_ASOC_OUTPUT_PATH_SP_LO2		(10)
#define MC_ASOC_OUTPUT_PATH_SP_BT		(11)
#define MC_ASOC_OUTPUT_PATH_LO1_RC		(12)
#define MC_ASOC_OUTPUT_PATH_LO1_HP		(13)
#define MC_ASOC_OUTPUT_PATH_LO1_BT		(14)
#define MC_ASOC_OUTPUT_PATH_LO2_RC		(15)
#define MC_ASOC_OUTPUT_PATH_LO2_HP		(16)
#define MC_ASOC_OUTPUT_PATH_LO2_BT		(17)
#define MC_ASOC_OUTPUT_PATH_LO1_LO2		(18)
#define MC_ASOC_OUTPUT_PATH_LO2_LO1		(19)

#define MC_ASOC_INPUT_PATH_MAINMIC		(0)
#define MC_ASOC_INPUT_PATH_SUBMIC		(1)
#define MC_ASOC_INPUT_PATH_2MIC			(2)
#define MC_ASOC_INPUT_PATH_HS			(3)
#define MC_ASOC_INPUT_PATH_BT			(4)
#define MC_ASOC_INPUT_PATH_VOICECALL		(5)
#define MC_ASOC_INPUT_PATH_VOICEUPLINK		(6)
#define MC_ASOC_INPUT_PATH_VOICEDOWNLINK	(7)
#define MC_ASOC_INPUT_PATH_LIN1			(8)

#define MC_ASOC_INCALL_MIC_MAINMIC		(0)
#define MC_ASOC_INCALL_MIC_SUBMIC		(1)
#define MC_ASOC_INCALL_MIC_2MIC			(2)

#define MC_ASOC_DTMF_OUTPUT_SP			(0)
#define MC_ASOC_DTMF_OUTPUT_NORMAL		(1)


/*
 * Driver private data structure
 */
struct mc_asoc_setup {
	struct MCDRV_INIT_INFO	init;
	struct MCDRV_INIT2_INFO	init2;
	unsigned char	rslot[3];
	unsigned char	tslot[3];
};

struct mc_asoc_port_params {
	UINT8	rate;
	UINT8	bits[SNDRV_PCM_STREAM_LAST+1];
	UINT8	pcm_mono[SNDRV_PCM_STREAM_LAST+1];
	UINT8	pcm_order[SNDRV_PCM_STREAM_LAST+1];
	UINT8	pcm_law[SNDRV_PCM_STREAM_LAST+1];
	UINT8	master;
	UINT8	inv;
	UINT8	srcthru;
	UINT8	format;
	UINT8	bckfs;
	UINT8	channels;
	UINT8	stream;	/* bit0: Playback, bit1: Capture */
};

struct mc_asoc_dsp_param {
	UINT8	*pabParam;
	UINT32	dSize;
	struct mc_asoc_dsp_param	*next;
};

struct mc_asoc_jack {
	struct snd_soc_jack *hs_jack;
#ifdef CONFIG_SWITCH
	struct switch_dev *h2w_sdev;
#endif
};

struct mc_asoc_platform_data {
	void	(*set_ext_micbias)(int en);
	void	(*set_ext_sub_micbias)(int en);
	void	(*set_codec_ldod)(int status);
	void	(*set_codec_mclk)(bool enable, bool forced);
};

struct mc_asoc_data {
	struct mutex			mutex;
	struct mc_asoc_setup		setup;
	struct mc_asoc_port_params	port;
	struct snd_hwdep		*hwdep;
	struct MCDRV_CLOCKSW_INFO	clocksw_store;
	struct MCDRV_PATH_INFO		path_store;
	struct MCDRV_VOL_INFO		vol_store;
	struct MCDRV_DIO_INFO		dio_store;
	struct MCDRV_DIOPATH_INFO	diopath_store;
	struct MCDRV_SWAP_INFO		swap_store;
	struct MCDRV_HSDET_INFO		hsdet_store;
	struct MCDRV_HSDET2_INFO	hsdet2_store;
	struct mc_asoc_dsp_param	param_store[4][2];
	struct mc_asoc_jack		jack;
	struct mc_asoc_platform_data	*pdata;
};

struct mc_asoc_priv {
	struct mc_asoc_data	data;
};

extern void	mc_asoc_write_data(UINT8 bSlaveAdr,
				const UINT8 *pbData,
				UINT32 dSize);
extern void	mc_asoc_read_data(UINT8	bSlaveAdr,
				UINT32 dAddress,
				UINT8 *pbData,
				UINT32 dSize);
extern	void	mc_asoc_set_codec_ldod(int status);


/*
 * For debugging
 */
#ifdef CONFIG_SND_SOC_YAMAHA_YMU831_DEBUG

#define SHOW_REG_ACCESS
#define dbg_info(format, arg...)	snd_printd(KERN_INFO format, ## arg)
#define TRACE_FUNC()	snd_printd(KERN_INFO "<trace> %s()\n", __func__)
#define _McDrv_Ctrl	McDrv_Ctrl_dbg

extern SINT32	McDrv_Ctrl_dbg(
			UINT32 dCmd, void *pvPrm1, void *pvPrm2, UINT32 dPrm);

#else

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP

#define dbg_info(format, arg...)
#define TRACE_FUNC()
#define _McDrv_Ctrl McDrv_Ctrl

#else

#define dbg_info(format, arg...)	printk(KERN_DEBUG "ymu831 " \
								format, ## arg)
#define TRACE_FUNC()	printk(KERN_INFO "ymu831 %s\n", __func__)
#define _McDrv_Ctrl	McDrv_Ctrl_dbg_sec

extern SINT32	McDrv_Ctrl_dbg_sec(
			UINT32 dCmd, void *pvPrm1, void *pvPrm2, UINT32 dPrm);
#endif
#endif /* CONFIG_SND_SOC_YAMAHA_YMU831_DEBUG */

#endif /* YMU831_PRIV_H */
