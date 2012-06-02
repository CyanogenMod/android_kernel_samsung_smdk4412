/*
 * MC-1N2 ASoC codec driver - private header
 *
 * Copyright (c) 2010 Yamaha Corporation
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

#ifndef MC1N2_PRIV_H
#define MC1N2_PRIV_H

#include "mcdriver.h"

/*
 * Virtual registers
 */
enum {
	MC1N2_DVOL_AD0,
	MC1N2_DVOL_AENG6,
	MC1N2_DVOL_PDM,
	MC1N2_DVOL_DIR0,
	MC1N2_DVOL_DIR1,
	MC1N2_DVOL_DIR2,
	MC1N2_DVOL_AD0_ATT,
	MC1N2_DVOL_DIR0_ATT,
	MC1N2_DVOL_DIR1_ATT,
	MC1N2_DVOL_DIR2_ATT,
	MC1N2_DVOL_SIDETONE,
	MC1N2_DVOL_DAC_MASTER,
	MC1N2_DVOL_DAC_VOICE,
	MC1N2_DVOL_DAC_ATT,
	MC1N2_DVOL_DIT0,
	MC1N2_DVOL_DIT1,
	MC1N2_DVOL_DIT2,
	MC1N2_AVOL_AD0,
	MC1N2_AVOL_LIN1,
	MC1N2_AVOL_MIC1,
	MC1N2_AVOL_MIC2,
	MC1N2_AVOL_MIC3,
	MC1N2_AVOL_HP,
	MC1N2_AVOL_SP,
	MC1N2_AVOL_RC,
	MC1N2_AVOL_LOUT1,
	MC1N2_AVOL_LOUT2,
	MC1N2_AVOL_MIC1_GAIN,
	MC1N2_AVOL_MIC2_GAIN,
	MC1N2_AVOL_MIC3_GAIN,
	MC1N2_AVOL_HP_GAIN,

	MC1N2_ADCL_MIC1_SW,
	MC1N2_ADCL_MIC2_SW,
	MC1N2_ADCL_MIC3_SW,
	MC1N2_ADCL_LINE_SW,
	MC1N2_ADCR_MIC1_SW,
	MC1N2_ADCR_MIC2_SW,
	MC1N2_ADCR_MIC3_SW,
	MC1N2_ADCR_LINE_SW,

	MC1N2_HPL_MIC1_SW,
	MC1N2_HPL_MIC2_SW,
	MC1N2_HPL_MIC3_SW,
	MC1N2_HPL_LINE_SW,
	MC1N2_HPL_DAC_SW,

	MC1N2_HPR_MIC1_SW,
	MC1N2_HPR_MIC2_SW,
	MC1N2_HPR_MIC3_SW,
	MC1N2_HPR_LINER_SW,
	MC1N2_HPR_DACR_SW,

	MC1N2_SPL_LINE_SW,
	MC1N2_SPL_DAC_SW,
	MC1N2_SPR_LINE_SW,
	MC1N2_SPR_DAC_SW,

	MC1N2_RC_MIC1_SW,
	MC1N2_RC_MIC2_SW,
	MC1N2_RC_MIC3_SW,
	MC1N2_RC_LINEMONO_SW,
	MC1N2_RC_DACL_SW,
	MC1N2_RC_DACR_SW,

	MC1N2_LOUT1L_MIC1_SW,
	MC1N2_LOUT1L_MIC2_SW,
	MC1N2_LOUT1L_MIC3_SW,
	MC1N2_LOUT1L_LINE_SW,
	MC1N2_LOUT1L_DAC_SW,

	MC1N2_LOUT1R_MIC1_SW,
	MC1N2_LOUT1R_MIC2_SW,
	MC1N2_LOUT1R_MIC3_SW,
	MC1N2_LOUT1R_LINER_SW,
	MC1N2_LOUT1R_DACR_SW,

	MC1N2_LOUT2L_MIC1_SW,
	MC1N2_LOUT2L_MIC2_SW,
	MC1N2_LOUT2L_MIC3_SW,
	MC1N2_LOUT2L_LINE_SW,
	MC1N2_LOUT2L_DAC_SW,

	MC1N2_LOUT2R_MIC1_SW,
	MC1N2_LOUT2R_MIC2_SW,
	MC1N2_LOUT2R_MIC3_SW,
	MC1N2_LOUT2R_LINER_SW,
	MC1N2_LOUT2R_DACR_SW,

	MC1N2_DMIX_ADC_SW,
	MC1N2_DMIX_DIR0_SW,
	MC1N2_DMIX_DIR1_SW,
	MC1N2_DMIX_DIR2_SW,

	MC1N2_DACMAIN_SRC,
	MC1N2_DACVOICE_SRC,
	MC1N2_DIT0_SRC,
	MC1N2_DIT1_SRC,
	MC1N2_DIT2_SRC,
	MC1N2_AE_SRC,

	MC1N2_ADCL_LINE_SRC,
	MC1N2_ADCR_LINE_SRC,
	MC1N2_HPL_LINE_SRC,
	MC1N2_HPL_DAC_SRC,
	MC1N2_SPL_LINE_SRC,
	MC1N2_SPL_DAC_SRC,
	MC1N2_SPR_LINE_SRC,
	MC1N2_SPR_DAC_SRC,
	MC1N2_LOUT1L_LINE_SRC,
	MC1N2_LOUT1L_DAC_SRC,
	MC1N2_LOUT2L_LINE_SRC,
	MC1N2_LOUT2L_DAC_SRC,

	MC1N2_AE_PARAM_SEL,
	MC1N2_ADC_PDM_SEL,

	MC1N2_MICBIAS1,
	MC1N2_MICBIAS2,
	MC1N2_MICBIAS3,

	MC1N2_N_REG
};

#define MC1N2_N_VOL_REG MC1N2_ADCL_MIC1_SW	 

#define MC1N2_DSOURCE_OFF		0
#define MC1N2_DSOURCE_ADC		1
#define MC1N2_DSOURCE_DIR0		2
#define MC1N2_DSOURCE_DIR1		3
#define MC1N2_DSOURCE_DIR2		4
#define MC1N2_DSOURCE_MIX		5

#define MC1N2_AE_PARAM_1		0
#define MC1N2_AE_PARAM_2		1
#define MC1N2_AE_PARAM_3		2
#define MC1N2_AE_PARAM_4		3
#define MC1N2_AE_PARAM_5		4

#define mc1n2_i2c_read_byte(c,r) i2c_smbus_read_byte_data((c), (r)<<1)

extern struct i2c_client *mc1n2_get_i2c_client(void);

/*
 * For debugging
 */
#ifdef CONFIG_SND_SOC_MC1N2_DEBUG

#define dbg_info(format, arg...) snd_printd(KERN_INFO format, ## arg)
#define TRACE_FUNC() snd_printd(KERN_INFO "<trace> %s()\n", __FUNCTION__)


#define _McDrv_Ctrl McDrv_Ctrl_dbg
extern SINT32 McDrv_Ctrl_dbg(UINT32 dCmd, void *pvPrm, UINT32 dPrm);

#else

#define dbg_info(format, arg...)
#define TRACE_FUNC()

#define _McDrv_Ctrl McDrv_Ctrl

#endif /* CONFIG_SND_SOC_MC1N2_DEBUG */

#endif /* MC1N2_PRIV_H */
