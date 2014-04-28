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

#ifndef YMU831_CFG_H
#define YMU831_CFG_H
#include <linux/input.h>
#include <linux/irq.h>

#include "mcdriver.h"
#include "ymu831_priv.h"

/*
 * ALSA Version
 */
#define	KERNEL_3_4

#define	HSDET_WHILE_SUSPEND

#define	MAX_YMS_CTRL_PARAM_SIZE	(524288UL)

#define	BUS_SEL_I2C		(0)
#define	BUS_SEL_SPI		(1)
#define	BUS_SEL_SLIM		(2)
#define	BUS_SELECT		BUS_SEL_SPI

#define	CAPTURE_PORT_MUSIC	(0)
#define	CAPTURE_PORT_EXT	(1)
#define	CAPTURE_PORT		CAPTURE_PORT_MUSIC

#define	MC_ASOC_PHYS_DIO0	MCDRV_PHYSPORT_DIO0
#define	MC_ASOC_PHYS_DIO1	MCDRV_PHYSPORT_DIO1
#define	MC_ASOC_PHYS_DIO2	MCDRV_PHYSPORT_DIO2
#define	MC_ASOC_PHYS_NONE	MCDRV_PHYSPORT_NONE
#define	MC_ASOC_PHYS_SLIM0	MCDRV_PHYSPORT_SLIM0
#define	MC_ASOC_PHYS_SLIM1	MCDRV_PHYSPORT_SLIM1
#define	MC_ASOC_PHYS_SLIM2	MCDRV_PHYSPORT_SLIM2
#define	MUSIC_PHYSICAL_PORT	MC_ASOC_PHYS_DIO0
#define	EXT_PHYSICAL_PORT	MC_ASOC_PHYS_DIO2
#define	VOICE_PHYSICAL_PORT	MC_ASOC_PHYS_DIO1
#define	HIFI_PHYSICAL_PORT	MC_ASOC_PHYS_DIO0

#define	VOICE_RECORDING_UNMUTE	(1)

#define	INCALL_MIC_SP		MC_ASOC_INCALL_MIC_MAINMIC
#define	INCALL_MIC_RC		MC_ASOC_INCALL_MIC_MAINMIC
#define	INCALL_MIC_HP		MC_ASOC_INCALL_MIC_MAINMIC
#define	INCALL_MIC_LO1		MC_ASOC_INCALL_MIC_MAINMIC
#define	INCALL_MIC_LO2		MC_ASOC_INCALL_MIC_MAINMIC

#define	MIC_NONE		(0)
#define	MIC_1			(1)
#define	MIC_2			(2)
#define	MIC_3			(3)
#define	MIC_4			(4)
#define	MIC_PDM0		(5)
#define	MIC_PDM1		(6)

#define	MAIN_MIC		MIC_1
#define	SUB_MIC			MIC_2
#define	HEADSET_MIC		MIC_4

#define	BIAS_OFF		(0)
#define	BIAS_ON_ALWAYS		(1)
#define	BIAS_SYNC_MIC		(2)
#define	MIC1_BIAS		BIAS_OFF
#define	MIC2_BIAS		BIAS_SYNC_MIC
#define	MIC3_BIAS		BIAS_SYNC_MIC
#define	MIC4_BIAS		BIAS_SYNC_MIC

#define	IRQ_TYPE		IRQ_TYPE_EDGE_FALLING

#define	AUTO_POWEROFF_OFF	(0)
#define	AUTO_POWEROFF_ON	(1)
#define	AUTO_POWEROFF		AUTO_POWEROFF_ON

static const struct mc_asoc_setup mc_asoc_cfg_setup = {
	/*	init	*/
	{
		MCDRV_CKSEL_CMOS_CMOS,		/*	bCkSel		*/
		MCDRV_CKINPUT_CLKI0_CLKI0,	/*	bCkInput	*/
		0x04,				/*	bPllModeA	*/
		0x16,				/*	bPllPrevDivA	*/
		0x0087,				/*	wPllFbDivA	*/
		0x2B02,				/*	wPllFracA	*/
		1,				/*	bPllFreqA	*/
		0x04,				/*	bPllModeB	*/
		0x16,				/*	bPllPrevDivB	*/
		0x0043,				/*	wPllFbDivB	*/
		0x9581,				/*	wPllFracB	*/
		0,				/*	bPllFreqB	*/
		0,				/*	bHsdetClk	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio0SdoHiz	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio1SdoHiz	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio2SdoHiz	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio0ClkHiz	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio1ClkHiz	*/
		MCDRV_DAHIZ_HIZ,		/*	bDio2ClkHiz	*/
		MCDRV_PCMHIZ_LOW,		/*	bDio0PcmHiz	*/
		MCDRV_PCMHIZ_LOW,		/*	bDio1PcmHiz	*/
		MCDRV_PCMHIZ_LOW,		/*	bDio2PcmHiz	*/
		MCDRV_PA_GPIO,			/*	bPa0Func	*/
		MCDRV_PA_GPIO,			/*	bPa1Func	*/
		MCDRV_PA_GPIO,			/*	bPa2Func	*/
		MCDRV_POWMODE_FULL,		/*	bPowerMode	*/
		MCDRV_MBSEL_22,			/*	bMbSel1		*/
		MCDRV_MBSEL_22,			/*	bMbSel2		*/
		MCDRV_MBSEL_22,			/*	bMbSel3		*/
		MCDRV_MBSEL_22,			/*	bMbSel4		*/
		MCDRV_MBSDISCH_1000,		/*	bMbsDisch	*/
		MCDRV_NONCLIP_OFF,		/*	bNonClip	*/
		MCDRV_LINE_STEREO,		/*	bLineIn1Dif	*/
		MCDRV_LINE_STEREO,		/*	bLineOut1Dif	*/
		MCDRV_LINE_STEREO,		/*	bLineOut2Dif	*/
		MCDRV_MIC_DIF,			/*	bMic1Sng	*/
		MCDRV_MIC_DIF,			/*	bMic2Sng	*/
		MCDRV_MIC_DIF,			/*	bMic3Sng	*/
		MCDRV_MIC_DIF,			/*	bMic4Sng	*/
		MCDRV_ZC_OFF,			/*	bZcLineOut1	*/
		MCDRV_ZC_OFF,			/*	bZcLineOut2	*/
		MCDRV_ZC_ON,			/*	bZcRc		*/
		MCDRV_ZC_ON,			/*	bZcSp		*/
		MCDRV_ZC_OFF,			/*	bZcHp		*/
		MCDRV_SVOL_ON,			/*	bSvolLineOut1	*/
		MCDRV_SVOL_ON,			/*	bSvolLineOut2	*/
		MCDRV_SVOL_ON,			/*	bSvolRc		*/
		MCDRV_SVOL_ON,			/*	bSvolSp		*/
		MCDRV_SVOL_ON,			/*	bSvolHp		*/
		MCDRV_RCIMP_FIXLOW,		/*	bRcHiz		*/
		MCDRV_WL_LOFF_ROFF,		/*	bSpHiz		*/
		MCDRV_IMP_LFIXLOW_RFIXLOW,	/*	bHpHiz		*/
		MCDRV_IMP_LFIXLOW_RFIXLOW,	/*	bLineOut1Hiz	*/
		MCDRV_IMP_LFIXLOW_RFIXLOW,	/*	bLineOut2Hiz	*/
		MCDRV_CPMOD_MID,		/*	bCpMod		*/
		MCDRV_RBSEL_2_2K,		/*	bRbSel		*/
		MCDRV_PLUG_LRGM,		/*	bPlugSel	*/
		MCDRV_GNDDET_OFF,		/*	bGndDet		*/
		MCDRV_PPD_OFF,			/*	bPpdRc		*/
		MCDRV_PPD_OFF,			/*	bPpdSp		*/
		MCDRV_PPD_OFF,			/*	bPpdHp		*/
		MCDRV_PPD_OFF,			/*	bPpdLineOut1	*/
		MCDRV_PPD_OFF,			/*	bPpdLineOut2	*/
		{
			/*	dWaitTime	*/
			{
			5000, 5000, 5000, 5000, 25000, 15000, 2000, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			},
			/*	dPollInterval	*/
			{
			1000, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			},
			/*	dPollTimeOut	*/
			{
			1000, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			}
		},
	},
	/*	init2	*/
	{
		{
			MCDRV_DOA_DRV_HIGH,
			MCDRV_SCKMSK_OFF,
			MCDRV_SPMN_OFF_9,
			0x03, 0x30, 0x30, 0x21, 0x03, 0xC0, 0x6B, 0x00,
			0xC0, 0x01, 0x0F, 0, 0, 0, 0, 0, 0
		}
	},
	/*	rslot	*/
	{
		0, 1, 2
	},
	/*	tslot	*/
	{
		0, 1, 2
	},
};

static const struct MCDRV_DIO_PORT	stMusicPort_Default = {
	/*	sDioCommon	*/
	{
		/*	bMasterSlave : Master / Slave Setting	*/
		/*	 MCDRV_DIO_SLAVE	(0): Slave	*/
		/*	 MCDRV_DIO_MASTER	(1): Master	*/
		MCDRV_DIO_MASTER,

		/*	bAutoFs : Sampling frequency automatic measurement
				ON/OFF Setting in slave mode	*/
		/*	 MCDRV_AUTOFS_OFF	(0): OFF	*/
		/*	 MCDRV_AUTOFS_ON	(1): ON		*/
		MCDRV_AUTOFS_ON ,

		/*	bFs : Sampling Rate Setting		*/
		/*	 MCDRV_FS_48000	(0): 48kHz		*/
		/*	 MCDRV_FS_44100	(1): 44.1kHz		*/
		/*	 MCDRV_FS_32000	(2): 32kHz		*/
		/*	 MCDRV_FS_24000	(4): 24kHz		*/
		/*	 MCDRV_FS_22050	(5): 22.05kHz		*/
		/*	 MCDRV_FS_16000	(6): 16kHz		*/
		/*	 MCDRV_FS_12000	(8): 12kHz		*/
		/*	 MCDRV_FS_11025	(9): 11.025kHz		*/
		/*	 MCDRV_FS_8000	(10): 8kHz		*/
		MCDRV_FS_48000,

		/*	bBckFs : Bit Clock Frequency Setting	*/
		/*	 MCDRV_BCKFS_64		(0): LRCK x 64	*/
		/*	 MCDRV_BCKFS_48		(1): LRCK x 48	*/
		/*	 MCDRV_BCKFS_32		(2): LRCK x 32	*/
		/*	 MCDRV_BCKFS_512	(4): LRCK x 512	*/
		/*	 MCDRV_BCKFS_256	(5): LRCK x 256	*/
		/*	 MCDRV_BCKFS_192	(6): LRCK x 192	*/
		/*	 MCDRV_BCKFS_128	(7): LRCK x 128	*/
		/*	 MCDRV_BCKFS_96		(8): LRCK x 96	*/
		/*	 MCDRV_BCKFS_24		(9): LRCK x 24	*/
		/*	 MCDRV_BCKFS_16		(10): LRCK x 16	*/
		/*	 MCDRV_BCKFS_8		(11): LRCK x 8	*/
		/*	 MCDRV_BCKFS_SLAVE	(15): PCM I/F SLAVE	*/
		MCDRV_BCKFS_32,

		/*	bInterface : Interface Selection	*/
		/*	 MCDRV_DIO_DA	(0): Digital Audio	*/
		/*	 MCDRV_DIO_PCM	(1): PCM		*/
		MCDRV_DIO_DA,

		/*	bBckInvert : Bit Clock Inversion Setting	*/
		/*	 MCDRV_BCLK_NORMAL	(0): Normal Operation	*/
		/*	 MCDRV_BCLK_INVERT	(1): Clock Inverted	*/
		MCDRV_BCLK_NORMAL,

		/*	MCDRV_SRC_NOT_THRU	(0)	*/
		/*	MCDRV_SRC_THRU		(1)	*/
		MCDRV_SRC_NOT_THRU,

		/*	bPcmHizTim : High Impedance transition timing
			after transmitting the last PCM I/F data	*/
		/*	 MCDRV_PCMHIZTIM_FALLING	(0):
						BCLK#* Falling Edge	*/
		/*	 MCDRV_PCMHIZTIM_RISING		(1):
						BCLK#* Rising Edge	*/
		MCDRV_PCMHIZTIM_FALLING,

		/*	bPcmFrame : Frame Mode Setting with PCM interface*/
		/*	 MCDRV_PCM_SHORTFRAME	(0): Short Frame	*/
		/*	 MCDRV_PCM_LONGFRAME	(1): Long Frame		*/
		MCDRV_PCM_SHORTFRAME,

		/*	bPcmHighPeriod :
			LR clock High time setting with PCM selected
			and Master selected	*/
		/*	 0 to 31:
			High level keeps during the period of time of
			(setting value + 1) of the bit clock.		*/
		0,
	},
	/*	sDir	*/
	{
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_8
		}
	},
	/*	sDit	*/
	{
		MCDRV_STMODE_ZERO,	/*	bStMode	*/
		MCDRV_SDOUT_NORMAL,	/*	bEdge	*/
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		}
	}
};

static const struct MCDRV_DIO_PORT	stExtPort_Default = {
	/*	sDioCommon	*/
	{
		/*	bMasterSlave : Master / Slave Setting	*/
		/*	 MCDRV_DIO_SLAVE	(0): Slave	*/
		/*	 MCDRV_DIO_MASTER	(1): Master	*/
		MCDRV_DIO_MASTER,

		/*	bAutoFs : Sampling frequency automatic measurement
				ON/OFF Setting in slave mode	*/
		/*	 MCDRV_AUTOFS_OFF	(0): OFF	*/
		/*	 MCDRV_AUTOFS_ON	(1): ON		*/
		MCDRV_AUTOFS_ON ,

		/*	bFs : Sampling Rate Setting		*/
		/*	 MCDRV_FS_48000	(0): 48kHz		*/
		/*	 MCDRV_FS_44100	(1): 44.1kHz		*/
		/*	 MCDRV_FS_32000	(2): 32kHz		*/
		/*	 MCDRV_FS_24000	(4): 24kHz		*/
		/*	 MCDRV_FS_22050	(5): 22.05kHz		*/
		/*	 MCDRV_FS_16000	(6): 16kHz		*/
		/*	 MCDRV_FS_12000	(8): 12kHz		*/
		/*	 MCDRV_FS_11025	(9): 11.025kHz		*/
		/*	 MCDRV_FS_8000	(10): 8kHz		*/
		MCDRV_FS_8000,

		/*	bBckFs : Bit Clock Frequency Setting	*/
		/*	 MCDRV_BCKFS_64		(0): LRCK x 64	*/
		/*	 MCDRV_BCKFS_48		(1): LRCK x 48	*/
		/*	 MCDRV_BCKFS_32		(2): LRCK x 32	*/
		/*	 MCDRV_BCKFS_512	(4): LRCK x 512	*/
		/*	 MCDRV_BCKFS_256	(5): LRCK x 256	*/
		/*	 MCDRV_BCKFS_192	(6): LRCK x 192	*/
		/*	 MCDRV_BCKFS_128	(7): LRCK x 128	*/
		/*	 MCDRV_BCKFS_96		(8): LRCK x 96	*/
		/*	 MCDRV_BCKFS_24		(9): LRCK x 24	*/
		/*	 MCDRV_BCKFS_16		(10): LRCK x 16	*/
		/*	 MCDRV_BCKFS_8		(11): LRCK x 8	*/
		/*	 MCDRV_BCKFS_SLAVE	(15): PCM I/F SLAVE	*/
		MCDRV_BCKFS_32,

		/*	bInterface : Interface Selection	*/
		/*	 MCDRV_DIO_DA	(0): Digital Audio	*/
		/*	 MCDRV_DIO_PCM	(1): PCM		*/
		MCDRV_DIO_DA,

		/*	bBckInvert : Bit Clock Inversion Setting	*/
		/*	 MCDRV_BCLK_NORMAL	(0): Normal Operation	*/
		/*	 MCDRV_BCLK_INVERT	(1): Clock Inverted	*/
		MCDRV_BCLK_NORMAL,

		/*	MCDRV_SRC_NOT_THRU	(0)	*/
		/*	MCDRV_SRC_THRU		(1)	*/
		MCDRV_SRC_NOT_THRU,

		/*	bPcmHizTim : High Impedance transition timing
			after transmitting the last PCM I/F data	*/
		/*	 MCDRV_PCMHIZTIM_FALLING	(0):
						BCLK#* Falling Edge	*/
		/*	 MCDRV_PCMHIZTIM_RISING		(1):
						BCLK#* Rising Edge	*/
		MCDRV_PCMHIZTIM_FALLING,

		/*	bPcmFrame : Frame Mode Setting with PCM interface*/
		/*	 MCDRV_PCM_SHORTFRAME	(0): Short Frame	*/
		/*	 MCDRV_PCM_LONGFRAME	(1): Long Frame		*/
		MCDRV_PCM_SHORTFRAME,

		/*	bPcmHighPeriod :
			LR clock High time setting with PCM selected
			and Master selected	*/
		/*	 0 to 31:
			High level keeps during the period of time of
			(setting value + 1) of the bit clock.		*/
		0,
	},
	/*	sDir	*/
	{
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_LSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		}
	},
	/*	sDit	*/
	{
		MCDRV_STMODE_ZERO,	/*	bStMode	*/
		MCDRV_SDOUT_NORMAL,	/*	bEdge	*/
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_LSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		}
	}
};

static const struct MCDRV_DIO_PORT	stVoicePort_Default = {
	/*	sDioCommon	*/
	{
		/*	bMasterSlave : Master / Slave Setting	*/
		/*	 MCDRV_DIO_SLAVE	(0): Slave	*/
		/*	 MCDRV_DIO_MASTER	(1): Master	*/
		MCDRV_DIO_SLAVE,

		/*	bAutoFs : Sampling frequency automatic measurement
				ON/OFF Setting in slave mode	*/
		/*	 MCDRV_AUTOFS_OFF	(0): OFF	*/
		/*	 MCDRV_AUTOFS_ON	(1): ON		*/
		MCDRV_AUTOFS_ON ,

		/*	bFs : Sampling Rate Setting	*/
		/*	 MCDRV_FS_48000	(0): 48kHz	*/
		/*	 MCDRV_FS_44100	(1): 44.1kHz	*/
		/*	 MCDRV_FS_32000	(2): 32kHz	*/
		/*	 MCDRV_FS_24000	(4): 24kHz	*/
		/*	 MCDRV_FS_22050	(5): 22.05kHz	*/
		/*	 MCDRV_FS_16000	(6): 16kHz	*/
		/*	 MCDRV_FS_12000	(8): 12kHz	*/
		/*	 MCDRV_FS_11025	(9): 11.025kHz	*/
		/*	 MCDRV_FS_8000	(10): 8kHz	*/
		MCDRV_FS_8000,

		/*	bBckFs : Bit Clock Frequency Setting	*/
		/*	 MCDRV_BCKFS_64		(0): LRCK x 64	*/
		/*	 MCDRV_BCKFS_48		(1): LRCK x 48	*/
		/*	 MCDRV_BCKFS_32		(2): LRCK x 32	*/
		/*	 MCDRV_BCKFS_512	(4): LRCK x 512	*/
		/*	 MCDRV_BCKFS_256	(5): LRCK x 256	*/
		/*	 MCDRV_BCKFS_192	(6): LRCK x 192	*/
		/*	 MCDRV_BCKFS_128	(7): LRCK x 128	*/
		/*	 MCDRV_BCKFS_96		(8): LRCK x 96	*/
		/*	 MCDRV_BCKFS_24		(9): LRCK x 24	*/
		/*	 MCDRV_BCKFS_16		(10): LRCK x 16	*/
		/*	 MCDRV_BCKFS_8		(11): LRCK x 8	*/
		/*	 MCDRV_BCKFS_SLAVE	(15): PCM I/F SLAVE	*/
		MCDRV_BCKFS_32,

		/*	bInterface : Interface Selection	*/
		/*	 MCDRV_DIO_DA	(0): Digital Audio	*/
		/*	 MCDRV_DIO_PCM	(1): PCM		*/
		MCDRV_DIO_PCM,

		/*	bBckInvert : Bit Clock Inversion Setting	*/
		/*	 MCDRV_BCLK_NORMAL	(0): Normal Operation	*/
		/*	 MCDRV_BCLK_INVERT	(1): Clock Inverted	*/
		MCDRV_BCLK_NORMAL,

		/*	bSrcThru	*/
		/*	MCDRV_SRC_NOT_THRU	(0)	*/
		/*	MCDRV_SRC_THRU		(1)	*/
		MCDRV_SRC_NOT_THRU,

		/*	bPcmHizTim : High Impedance transition timing
			after transmitting the last PCM I/F data	*/
		/*	 MCDRV_PCMHIZTIM_FALLING	(0):
						BCLK#* Falling Edge	*/
		/*	 MCDRV_PCMHIZTIM_RISING		(1):
						BCLK#* Rising Edge	*/
		MCDRV_PCMHIZTIM_FALLING,

		/*	bPcmFrame : Frame Mode Setting with PCM interface*/
		/*	 MCDRV_PCM_SHORTFRAME	(0): Short Frame	*/
		/*	 MCDRV_PCM_LONGFRAME	(1): Long Frame	*/
		MCDRV_PCM_SHORTFRAME,

		/*	bPcmHighPeriod :
			LR clock High time setting with PCM selected
			and Master selected	*/
		/*	0 to 31:
			High level keeps during the period of time of
			(setting value + 1) of the bit clock.	*/
		0,
	},
	/*	sDir	*/
	{
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_HEADALIGN
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_STEREO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		},
	},
	/*	sDit	*/
	{
		MCDRV_STMODE_ZERO,	/*	bStMode	*/
		MCDRV_SDOUT_NORMAL,	/*	bEdge	*/
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_HEADALIGN
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_STEREO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		}
	}
};

static const struct MCDRV_DIO_PORT	stHifiPort_Default = {
	/*	sDioCommon	*/
	{
		/*	bMasterSlave : Master / Slave Setting	*/
		/*	 MCDRV_DIO_SLAVE	(0): Slave	*/
		/*	 MCDRV_DIO_MASTER	(1): Master	*/
		MCDRV_DIO_MASTER,

		/*	bAutoFs : Sampling frequency automatic measurement
				ON/OFF Setting in slave mode	*/
		/*	 MCDRV_AUTOFS_OFF	(0): OFF	*/
		/*	 MCDRV_AUTOFS_ON	(1): ON		*/
		MCDRV_AUTOFS_ON ,

		/*	bFs : Sampling Rate Setting		*/
		/*	 MCDRV_FS_48000	(0): 48kHz		*/
		/*	 MCDRV_FS_44100	(1): 44.1kHz		*/
		/*	 MCDRV_FS_32000	(2): 32kHz		*/
		/*	 MCDRV_FS_24000	(4): 24kHz		*/
		/*	 MCDRV_FS_22050	(5): 22.05kHz		*/
		/*	 MCDRV_FS_16000	(6): 16kHz		*/
		/*	 MCDRV_FS_12000	(8): 12kHz		*/
		/*	 MCDRV_FS_11025	(9): 11.025kHz		*/
		/*	 MCDRV_FS_8000	(10): 8kHz		*/
		MCDRV_FS_48000,

		/*	bBckFs : Bit Clock Frequency Setting	*/
		/*	 MCDRV_BCKFS_64		(0): LRCK x 64	*/
		/*	 MCDRV_BCKFS_48		(1): LRCK x 48	*/
		/*	 MCDRV_BCKFS_32		(2): LRCK x 32	*/
		/*	 MCDRV_BCKFS_512	(4): LRCK x 512	*/
		/*	 MCDRV_BCKFS_256	(5): LRCK x 256	*/
		/*	 MCDRV_BCKFS_192	(6): LRCK x 192	*/
		/*	 MCDRV_BCKFS_128	(7): LRCK x 128	*/
		/*	 MCDRV_BCKFS_96		(8): LRCK x 96	*/
		/*	 MCDRV_BCKFS_24		(9): LRCK x 24	*/
		/*	 MCDRV_BCKFS_16		(10): LRCK x 16	*/
		/*	 MCDRV_BCKFS_8		(11): LRCK x 8	*/
		/*	 MCDRV_BCKFS_SLAVE	(15): PCM I/F SLAVE	*/
		MCDRV_BCKFS_32,

		/*	bInterface : Interface Selection	*/
		/*	 MCDRV_DIO_DA	(0): Digital Audio	*/
		/*	 MCDRV_DIO_PCM	(1): PCM		*/
		MCDRV_DIO_DA,

		/*	bBckInvert : Bit Clock Inversion Setting	*/
		/*	 MCDRV_BCLK_NORMAL	(0): Normal Operation	*/
		/*	 MCDRV_BCLK_INVERT	(1): Clock Inverted	*/
		MCDRV_BCLK_NORMAL,

		/*	MCDRV_SRC_NOT_THRU	(0)	*/
		/*	MCDRV_SRC_THRU		(1)	*/
		MCDRV_SRC_NOT_THRU,

		/*	bPcmHizTim : High Impedance transition timing
			after transmitting the last PCM I/F data	*/
		/*	 MCDRV_PCMHIZTIM_FALLING	(0):
						BCLK#* Falling Edge	*/
		/*	 MCDRV_PCMHIZTIM_RISING		(1):
						BCLK#* Rising Edge	*/
		MCDRV_PCMHIZTIM_FALLING,

		/*	bPcmFrame : Frame Mode Setting with PCM interface*/
		/*	 MCDRV_PCM_SHORTFRAME	(0): Short Frame	*/
		/*	 MCDRV_PCM_LONGFRAME	(1): Long Frame		*/
		MCDRV_PCM_SHORTFRAME,

		/*	bPcmHighPeriod :
			LR clock High time setting with PCM selected
			and Master selected	*/
		/*	 0 to 31:
			High level keeps during the period of time of
			(setting value + 1) of the bit clock.		*/
		0,
	},
	/*	sDir	*/
	{
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_8
		}
	},
	/*	sDit	*/
	{
		MCDRV_STMODE_ZERO,	/*	bStMode	*/
		MCDRV_SDOUT_NORMAL,	/*	bEdge	*/
		/*	sDaFormat : Digital Audio Format Information	*/
		{
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_BITSEL_16	(0): 16bit	*/
			/*	 MCDRV_BITSEL_20	(1): 20bit	*/
			/*	 MCDRV_BITSEL_24	(2): 24bit	*/
			/*	 MCDRV_BITSEL_32	(3): 32bit	*/
			MCDRV_BITSEL_16,

			/*	bMode : Data Format Setting		*/
			/*	 MCDRV_DAMODE_HEADALIGN	(0):
						Left-justified Format	*/
			/*	 MCDRV_DAMODE_I2S	(1): I2S	*/
			/*	 MCDRV_DAMODE_TAILALIGN	(2):
						Right-justified Format	*/
			MCDRV_DAMODE_I2S
		},
		/*	sPcmFormat : PCM Format Information	*/
		{
			/*	bMono : Mono / Stereo Setting	*/
			/*	 MCDRV_PCM_STEREO	(0):Stereo	*/
			/*	 MCDRV_PCM_MONO		(1):Mono	*/
			MCDRV_PCM_MONO ,
			/*	bOrder : Bit Order Setting		*/
			/*	 MCDRV_PCM_MSB_FIRST	(0): MSB First	*/
			/*	 MCDRV_PCM_LSB_FIRST	(1): LSB First	*/
			MCDRV_PCM_MSB_FIRST,
			/*	bLaw : Data Format Setting		*/
			/*	 MCDRV_PCM_LINEAR	(0): Linear	*/
			/*	 MCDRV_PCM_ALAW		(1): A-Law	*/
			/*	 MCDRV_PCM_MULAW	(2): u-Law	*/
			MCDRV_PCM_LINEAR,
			/*	bBitSel : Bit Width Setting		*/
			/*	 MCDRV_PCM_BITSEL_8	(0):8 bits	*/
			/*	 MCDRV_PCM_BITSEL_16	(1):16 bits	*/
			/*	 MCDRV_PCM_BITSEL_24	(2):24 bits	*/
			MCDRV_PCM_BITSEL_16
		}
	}
};

/* ========================================
	HS DET settings
	========================================*/
static const struct MCDRV_HSDET_INFO stHSDetInfo_Default = {
	/*	bEnPlugDet	*/
	MCDRV_PLUGDET_DISABLE,
	/*	bEnPlugDetDb	*/
	MCDRV_PLUGDETDB_BOTH_ENABLE,
	/*	bEnDlyKeyOff	*/
	MCDRV_KEYEN_D_D_D,
	/*	bEnDlyKeyOn	*/
	MCDRV_KEYEN_D_D_D,
	/*	bEnMicDet	*/
	MCDRV_MICDET_ENABLE,
	/*	bEnKeyOff	*/
	MCDRV_KEYEN_E_E_E,
	/*	bEnKeyOn	*/
	MCDRV_KEYEN_E_E_E,
	/*	bHsDetDbnc	*/
	MCDRV_DETDBNC_219,
	/*	bKeyOffMtim	*/
	MCDRV_KEYOFF_MTIM_63,
	/*	bKeyOnMtim	*/
	MCDRV_KEYON_MTIM_63,
	/*	bKey0OffDlyTim	*/
	8,
	/*	bKey1OffDlyTim	*/
	8,
	/*	bKey2OffDlyTim	*/
	8,
	/*	bKey0OnDlyTim	*/
	8,
	/*	bKey1OnDlyTim	*/
	8,
	/*	bKey2OnDlyTim	*/
	8,
	/*	bKey0OnDlyTim2	*/
	0,
	/*	bKey1OnDlyTim2	*/
	0,
	/*	bKey2OnDlyTim2	*/
	0,
	/*	bIrqType	*/
	MCDRV_IRQTYPE_REF,
	/*	bDetInv	*/
	MCDRV_DET_IN_INV_INV,
	/*	bHsDetMode	*/
	MCDRV_HSDET_MODE_DETIN_A,
	/*	bSperiod	*/
	MCDRV_SPERIOD_15625,
	/*	bLperiod	*/
	0,
	/*	bDbncNumPlug	*/
	MCDRV_DBNC_NUM_7,
	/*	bDbncNumMic	*/
	MCDRV_DBNC_NUM_4,
	/*	bDbncNumKey	*/
	MCDRV_DBNC_NUM_4,
	/*	bSgnlPeriod	*/
	MCDRV_SGNLPERIOD_79,
	/*	bSgnlNum	*/
	MCDRV_SGNLNUM_4,
	/*	bSgnlPeak	*/
	MCDRV_SGNLPEAK_1182,
	/*	bImpSel		*/
	0,
	/*	bDlyIrqStop	*/
	0,
	/*	cbfunc	*/
	0
};

#define	HSUNDETDBNC	MCDRV_DETDBNC_109
#define	HSUNDETDBNCNUM	MCDRV_DBNC_NUM_7
#define	MSDETMB4OFF	(5000)
#define	MSMKDETENOFF	(200)

static const struct MCDRV_HSDET_INFO stHSDetInfo_Suspend = {
	/*	bEnPlugDet	*/
	MCDRV_PLUGDET_DISABLE,
	/*	bEnPlugDetDb	*/
	MCDRV_PLUGDETDB_BOTH_ENABLE,
	/*	bEnDlyKeyOff	*/
	MCDRV_KEYEN_D_D_D,
	/*	bEnDlyKeyOn	*/
	MCDRV_KEYEN_D_D_D,
	/*	bEnMicDet	*/
	MCDRV_MICDET_ENABLE,
	/*	bEnKeyOff	*/
	MCDRV_KEYEN_D_D_E,
	/*	bEnKeyOn	*/
	MCDRV_KEYEN_D_D_E,
	/*	bHsDetDbnc	*/
	MCDRV_DETDBNC_219,
	/*	bKeyOffMtim	*/
	MCDRV_KEYOFF_MTIM_63,
	/*	bKeyOnMtim	*/
	MCDRV_KEYON_MTIM_63,
	/*	bKey0OffDlyTim	*/
	8,
	/*	bKey1OffDlyTim	*/
	8,
	/*	bKey2OffDlyTim	*/
	8,
	/*	bKey0OnDlyTim	*/
	8,
	/*	bKey1OnDlyTim	*/
	8,
	/*	bKey2OnDlyTim	*/
	8,
	/*	bKey0OnDlyTim2	*/
	0,
	/*	bKey1OnDlyTim2	*/
	0,
	/*	bKey2OnDlyTim2	*/
	0,
	/*	bIrqType	*/
	MCDRV_IRQTYPE_REF,
	/*	bDetInv	*/
	MCDRV_DET_IN_INV_INV,
	/*	bHsDetMode	*/
	MCDRV_HSDET_MODE_DETIN_A,
	/*	bSperiod	*/
	MCDRV_SPERIOD_15625,
	/*	bLperiod	*/
	0,
	/*	bDbncNumPlug	*/
	MCDRV_DBNC_NUM_7,
	/*	bDbncNumMic	*/
	MCDRV_DBNC_NUM_4,
	/*	bDbncNumKey	*/
	MCDRV_DBNC_NUM_4,
	/*	bSgnlPeriod	*/
	MCDRV_SGNLPERIOD_79,
	/*	bSgnlNum	*/
	MCDRV_SGNLNUM_4,
	/*	bSgnlPeak	*/
	MCDRV_SGNLPEAK_1182,
	/*	bImpSel		*/
	0,
	/*	bDlyIrqStop	*/
	0,
	/*	cbfunc	*/
	0
};

static const struct MCDRV_HSDET2_INFO stHSDet2Info_Default = {
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF,
	MCDRV_IRQTYPE_REF
};

/* ========================================
	Key Event settings
	========================================*/
#define	MC_ASOC_EV_KEY_DELAYKEYON0	KEY_RESERVED
#define	MC_ASOC_EV_KEY_DELAYKEYON1	KEY_RESERVED
#define	MC_ASOC_EV_KEY_DELAYKEYON2	KEY_RESERVED

static const unsigned int	mc_asoc_ev_key_delaykeyoff0[8] = {
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED
};
static const unsigned int	mc_asoc_ev_key_delaykeyoff1[8] = {
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED
};
static const unsigned int	mc_asoc_ev_key_delaykeyoff2[8] = {
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED
};

#define	MC_ASOC_IMP_TBL_NUM	(8)
static const SINT16	aswHpVolImpTable[MC_ASOC_IMP_TBL_NUM] = {
	0, 0, 1, 2, 3.5, 5, 0, 0
};

static const SINT16	aswDac0VolImpTable[MC_ASOC_IMP_TBL_NUM] = {
	0, 0, 0, 0, 0, 0, 0, 0
};

#endif
