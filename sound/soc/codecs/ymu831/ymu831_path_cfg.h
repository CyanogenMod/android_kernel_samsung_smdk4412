/*
 * YMU831 ASoC codec driver
 *
 * Copyright (c) 2012-2013 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.	In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef YMU831_PATH_CFG_H
#define YMU831_PATH_CFG_H

#include "mcdriver.h"

#define PRESET_PATH_N	(89)
/* ========================================
	Preset Path settings
	========================================*/
static const struct MCDRV_PATH_INFO	stPresetPathInfo[PRESET_PATH_N] = {
	/* playback:off, capture:off */
	{
		{{0x00AAAAAA}, {0x00AAAAAA} } ,	/* asMusicOut	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asExtOut	*/
		{{0x00AAAAAA} } ,		/* asHifiOut	*/
		{{0x00AAAAAA}, {0x00AAAAAA},
		 {0x00AAAAAA}, {0x00AAAAAA} },	/* asVboxMixIn	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAe0	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAe1	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAe2	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAe3	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asDac0	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asDac1	*/
		{{0x00AAAAAA} },		/* asVoiceOut	*/
		{{0x00AAAAAA} },		/* asVboxIoIn	*/
		{{0x00AAAAAA} },		/* asVboxHostIn	*/
		{{0x00AAAAAA} },		/* asHostOut	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAdif0	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAdif1	*/
		{{0x00AAAAAA}, {0x00AAAAAA} },	/* asAdif2	*/
		{{0x002AAAAA}, {0x002AAAAA} },	/* asAdc0	*/
		{{0x002AAAAA} },		/* asAdc1	*/
		{{0x002AAAAA}, {0x002AAAAA} },	/* asSp		*/
		{{0x002AAAAA}, {0x002AAAAA} },	/* asHp		*/
		{{0x002AAAAA} },		/* asRc		*/
		{{0x002AAAAA}, {0x002AAAAA} },	/* asLout1	*/
		{{0x002AAAAA}, {0x002AAAAA} },	/* asLout2	*/
		{{0x002AAAAA}, {0x002AAAAA},
		 {0x002AAAAA}, {0x002AAAAA} }	/* asBias	*/
	},
	/* playback:audio, capture:off (analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
#ifdef CONFIG_MACH_V1
		 {MCDRV_ASRC_DAC1_R_ON} },		/* asSp		*/
#else
		 {0x00000000} },		/* asSp		*/
#endif
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:off (BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:off (analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audio (analog input) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audio (BT input) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (analog input, analog output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (BT input, analog output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (analog input, BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (BT input, BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (analog input, analog+BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audio (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:incall (analog input, analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
#ifdef CONFIG_MACH_V1
		{MCDRV_ASRC_DAC1_R_ON} },		/* asSp 	*/
#else
		{0x00000000} },		/* asSp 	*/
#endif
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:incall (BT input, BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:incall (BT input, analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:incall (analog input analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:incall (BT input BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:incall (BT input analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:audio+incall
						(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall, capture:audio+incall (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:audio+incall
						(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication (analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio (HiFi), capture:off */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_HIFIIN_ON},
		 {MCDRV_D1SRC_HIFIIN_ON} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audio (HiFi) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{MCDRV_D1SRC_ADIF0_ON} },	/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio (HiFi), capture:audio (HiFi) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{MCDRV_D1SRC_ADIF0_ON} },	/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_HIFIIN_ON},
		 {MCDRV_D1SRC_HIFIIN_ON} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audioex (analog input) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audioex (BT input) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (BT input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (analog input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (analog input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audioex (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audiovr (analog input) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF0_ON},
		 {MCDRV_D1SRC_ADIF0_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{MCDRV_D2SRC_ADC1_ON},
		 {MCDRV_D2SRC_ADC1_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_M_ON} },	/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:off, capture:audiovr (BT input) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (analog input, analog output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF0_ON},
		 {MCDRV_D1SRC_ADIF0_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{MCDRV_D2SRC_ADC1_ON},
		 {MCDRV_D2SRC_ADC1_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_M_ON} },	/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (BT input, analog output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (analog input, BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF0_ON},
		 {MCDRV_D1SRC_ADIF0_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{MCDRV_D2SRC_ADC1_ON},
		 {MCDRV_D2SRC_ADC1_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_M_ON} },	/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (BT input, BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (analog input, analog+BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF0_ON},
		 {MCDRV_D1SRC_ADIF0_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{MCDRV_D2SRC_ADC1_ON},
		 {MCDRV_D2SRC_ADC1_ON} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_M_ON} },	/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio, capture:audiovr (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:karaoke, capture:off (analog input) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		 |MCDRV_ASRC_MIC2_ON
		 |MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		 |MCDRV_ASRC_MIC2_ON
		 |MCDRV_ASRC_MIC4_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:karaoke, capture:off (BT input) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:karaoke, capture:audio (analog input) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		 |MCDRV_ASRC_MIC2_ON
		 |MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		 |MCDRV_ASRC_MIC2_ON
		 |MCDRV_ASRC_MIC4_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:karaoke, capture:audio (BT input) */
	{
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON
		 |MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:incall (analog input, analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:incall (BT input, BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:incall (BT input, analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:incall
						(analog input analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:incall (BT input BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:incall
						(BT input analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:audio+incall
					(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall2, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:audio+incall
					(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:audio+incall
						(BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall2, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },		/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication2 (analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{0x00000000}, {0x00000000} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication2 (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* incommunication2 (BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:incall (analog input, analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:incall (BT input, BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:incall (BT input, analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:incall
						(analog input analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:incall (BT input BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:incall (BT input analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:audio+incall
						(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall3, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:audio+incall
						(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall3, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:incall (analog input, analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:incall (BT input, BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:incall (BT input, analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:incall
						(analog input analog output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:incall (BT input BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:incall
						(BT input analog+BT output) */
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:audio+incall
					(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:audio+incall (BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:incall4, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:audio+incall
					(analog input, analog output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_AE1_ON},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON} },		/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:audio+incall
						(BT input, BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_VBOXOUT_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{0x00000000}, {0x00000000} },	/* asDac0	*/
		{{0x00000000}, {0x00000000} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{0x00000000}, {0x00000000} },	/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{0x00000000}, {0x00000000} },	/* asLout1	*/
		{{0x00000000}, {0x00000000} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
	/* playback:audio+incall4, capture:audio+incall
						(BT input, analog+BT output) */
	{
		{{MCDRV_D1SRC_VBOXREFOUT_ON},
		 {MCDRV_D1SRC_VBOXREFOUT_ON} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_VBOXOUT_ON|MCDRV_D1SRC_MUSICIN_ON} },
						/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{MCDRV_D1SRC_AE1_ON},
		 {0x00000000},
		 {MCDRV_D1SRC_ADIF2_ON},
		 {0x00000000} },		/* asVboxMixIn	*/
		{{0x00000000},
		 {0x00000000} },	/* asAe0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_MUSICIN_ON},
		 {MCDRV_D1SRC_MUSICIN_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asDac1	*/
		{{MCDRV_D2SRC_VBOXIOOUT_ON} },	/* asVoiceOut	*/
		{{MCDRV_D2SRC_VOICEIN_ON} },	/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{MCDRV_D2SRC_VBOXHOSTOUT_ON} },/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON},
		 {MCDRV_D2SRC_DAC0REF_ON|MCDRV_D2SRC_DAC1REF_ON} },
						/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{0x00000000}, {0x00000000} },	/* asHp		*/
		{{0x00000000} },		/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
};


static const struct MCDRV_PATH_INFO	AnalogInputPath[]	= {
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_ADIF1_ON},
		 {MCDRV_D1SRC_ADIF1_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{MCDRV_D2SRC_ADC0_L_ON},
		 {MCDRV_D2SRC_ADC0_R_ON} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_L_ON},
		 {MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_LINEIN1_R_ON} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	},
};
static const int	AnalogPathMapping[PRESET_PATH_N]	= {
	0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};
static const struct MCDRV_PATH_INFO	BtInputPath[]	= {
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{MCDRV_D1SRC_EXTIN_ON},
		 {MCDRV_D1SRC_EXTIN_ON} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	}
};
static const int	BtPathMapping[PRESET_PATH_N]		= {
	0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};
static const struct MCDRV_PATH_INFO	DtmfPath[]	= {
	{
		{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asExtOut	*/
		{{0x00000000} },		/* asHifiOut	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} },	/* asVboxMixIn	*/
		{{0x00000000}, {0x00000000} },	/* asAe0	*/
		{{0x00000000}, {0x00000000} },	/* asAe1	*/
		{{0x00000000}, {0x00000000} },	/* asAe2	*/
		{{0x00000000}, {0x00000000} },	/* asAe3	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac0	*/
		{{MCDRV_D1SRC_AE0_ON},
		 {MCDRV_D1SRC_AE0_ON} },	/* asDac1	*/
		{{0x00000000} },		/* asVoiceOut	*/
		{{0x00000000} },		/* asVboxIoIn	*/
		{{0x00000000} },		/* asVboxHostIn	*/
		{{0x00000000} },		/* asHostOut	*/
		{{0x00000000}, {0x00000000} },	/* asAdif0	*/
		{{0x00000000}, {0x00000000} },	/* asAdif1	*/
		{{0x00000000}, {0x00000000} },	/* asAdif2	*/
		{{0x00000000}, {0x00000000} },	/* asAdc0	*/
		{{0x00000000} },		/* asAdc1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {0x00000000} },		/* asSp		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asHp		*/
		{{MCDRV_ASRC_DAC0_L_ON} },	/* asRc		*/
		{{MCDRV_ASRC_DAC0_L_ON},
		 {MCDRV_ASRC_DAC0_R_ON} },	/* asLout1	*/
		{{MCDRV_ASRC_DAC1_L_ON},
		 {MCDRV_ASRC_DAC1_R_ON} },	/* asLout2	*/
		{{0x00000000}, {0x00000000},
		 {0x00000000}, {0x00000000} }	/* asBias	*/
	}
};
static const int	DtmfPathMapping[PRESET_PATH_N]	= {
	0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

#endif
