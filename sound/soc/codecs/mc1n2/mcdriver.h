/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdriver.h
 *
 *		Description	: MC Driver header
 *
 *		Version		: 1.0.0 	2010.07.05
 *
 ****************************************************************************/

#ifndef _MCDRIVER_H_
#define _MCDRIVER_H_

#include "mctypedef.h"



signed long	McDrv_Ctrl( UINT32 dCmd, void* pvPrm, UINT32 dPrm );




/*	return value	*/
#define	MCDRV_SUCCESS				((SINT32)0)
#define	MCDRV_ERROR_ARGUMENT		(-1)
#define	MCDRV_ERROR_TIMEOUT			(-2)
#define	MCDRV_ERROR_INIT			(-3)
#define	MCDRV_ERROR_RESOURCEOVER	(-4)
#define	MCDRV_ERROR_STATE			(-5)

#define	MCDRV_ERROR					(-10)


/*	dCmd	*/
#define	MCDRV_INIT					(0)
#define	MCDRV_TERM					(1)
#define	MCDRV_READ_REG				(2)
#define	MCDRV_WRITE_REG				(3)
#define	MCDRV_GET_PATH				(4)
#define	MCDRV_SET_PATH				(5)
#define	MCDRV_GET_VOLUME			(6)
#define	MCDRV_SET_VOLUME			(7)
#define	MCDRV_GET_DIGITALIO			(8)
#define	MCDRV_SET_DIGITALIO			(9)
#define	MCDRV_GET_DAC				(10)
#define	MCDRV_SET_DAC				(11)
#define	MCDRV_GET_ADC				(12)
#define	MCDRV_SET_ADC				(13)
#define	MCDRV_GET_SP				(14)
#define	MCDRV_SET_SP				(15)
#define	MCDRV_GET_DNG				(16)
#define	MCDRV_SET_DNG				(17)
#define	MCDRV_SET_AUDIOENGINE		(18)
#define	MCDRV_SET_AUDIOENGINE_EX	(19)
#define	MCDRV_SET_CDSP				(20)
#define	MCDRV_GET_CDSP_PARAM		(21)
#define	MCDRV_SET_CDSP_PARAM		(22)
#define	MCDRV_REGISTER_CDSP_CB		(23)
#define	MCDRV_GET_PDM				(24)
#define	MCDRV_SET_PDM				(25)
#define	MCDRV_SET_DTMF				(26)
#define	MCDRV_CONFIG_GP				(27)
#define	MCDRV_MASK_GP				(28)
#define	MCDRV_GETSET_GP				(29)
#define	MCDRV_GET_PEAK				(30)
#define	MCDRV_IRQ					(31)
#define	MCDRV_UPDATE_CLOCK			(32)
#define	MCDRV_SWITCH_CLOCK			(33)
#define	MCDRV_GET_SYSEQ				(34)
#define	MCDRV_SET_SYSEQ				(35)

/*	pvPrm	*/
/*	init	*/
/*	MCDRV_INIT_INFO bCkSel setting	*/
#define	MCDRV_CKSEL_CMOS			(0x00)
#define	MCDRV_CKSEL_TCXO			(0xC0)
#define	MCDRV_CKSEL_CMOS_TCXO		(0x80)
#define	MCDRV_CKSEL_TCXO_CMOS		(0x40)

/*	MCDRV_INIT_INFO bXXXHiz setting	*/
#define	MCDRV_DAHIZ_LOW				(0)
#define	MCDRV_DAHIZ_HIZ				(1)

/*	CDRV_INIT_INFO bPcmHiz setting	*/
#define	MCDRV_PCMHIZ_LOW			(0)
#define	MCDRV_PCMHIZ_HIZ			(1)

/*	MCDRV_INIT_INFO bSvolStep setting	*/
#define	MCDRV_SVOLSTEP_0137			(0)
#define	MCDRV_SVOLSTEP_0274			(1)
#define	MCDRV_SVOLSTEP_0548			(2)
#define	MCDRV_SVOLSTEP_1096			(3)

/*	MCDRV_INIT_INFO bLinexxDif setting	*/
#define	MCDRV_LINE_STEREO			(0)
#define	MCDRV_LINE_DIF				(1)

/*	MCDRV_INIT_INFO bSpmn setting	*/
#define	MCDRV_SPMN_ON				(0)
#define	MCDRV_SPMN_OFF				(1)

/*	MCDRV_INIT_INFO bMicxSng setting	*/
#define	MCDRV_MIC_DIF				(0)
#define	MCDRV_MIC_SINGLE			(1)

/*	MCDRV_INIT_INFO bPowerMode setting	*/
#define	MCDRV_POWMODE_NORMAL		(0)
#define	MCDRV_POWMODE_CLKON			(1)
#define	MCDRV_POWMODE_VREFON		(2)
#define	MCDRV_POWMODE_CLKVREFON		(3)
#define	MCDRV_POWMODE_FULL			(4)

/* bSpHiz setting */
#define	MCDRV_SPHIZ_PULLDOWN		(0)
#define	MCDRV_SPHIZ_HIZ				(1)

/*	MCDRV_INIT_INFO bLdo setting	*/
#define	MCDRV_LDO_OFF				(0)
#define	MCDRV_LDO_ON				(1)

/*	MCDRV_INIT_INFO bPadxFunc setting	*/
#define	MCDRV_PAD_GPIO				(0)
#define	MCDRV_PAD_PDMCK				(1)
#define	MCDRV_PAD_PDMDI				(2)
#define	MCDRV_PAD_IRQ				(3)

/*	MCDRV_INIT_INFO bAvddLev/bVrefLev setting	*/
#define	MCDRV_OUTLEV_0				(0)
#define	MCDRV_OUTLEV_1				(1)
#define	MCDRV_OUTLEV_2				(2)
#define	MCDRV_OUTLEV_3				(3)
#define	MCDRV_OUTLEV_4				(4)
#define	MCDRV_OUTLEV_5				(5)
#define	MCDRV_OUTLEV_6				(6)
#define	MCDRV_OUTLEV_7				(7)

/*	MCDRV_INIT_INFO bDclGain setting	*/
#define	MCDRV_DCLGAIN_0				(0)
#define	MCDRV_DCLGAIN_6				(1)
#define	MCDRV_DCLGAIN_12			(2)
#define	MCDRV_DCLGAIN_18			(3)

/*	MCDRV_INIT_INFO bDclLimit setting	*/
#define	MCDRV_DCLLIMIT_0			(0)
#define	MCDRV_DCLLIMIT_116			(1)
#define	MCDRV_DCLLIMIT_250			(2)
#define	MCDRV_DCLLIMIT_602			(3)

/*	MCDRV_INIT_INFO bCpMod setting	*/
#define	MCDRV_CPMOD_ON				(0)
#define	MCDRV_CPMOD_OFF				(1)

typedef struct
{
	UINT32	dAdHpf;
	UINT32	dMic1Cin;
	UINT32	dMic2Cin;
	UINT32	dMic3Cin;
	UINT32	dLine1Cin;
	UINT32	dLine2Cin;
	UINT32	dVrefRdy1;
	UINT32	dVrefRdy2;
	UINT32	dHpRdy;
	UINT32	dSpRdy;
	UINT32	dPdm;
	UINT32	dAnaRdyInterval;
	UINT32	dSvolInterval;
	UINT32	dAnaRdyTimeOut;
	UINT32	dSvolTimeOut;
} MCDRV_WAIT_TIME;

typedef struct
{
	UINT8			bCkSel;
	UINT8			bDivR0;
	UINT8			bDivF0;
	UINT8			bDivR1;
	UINT8			bDivF1;
	UINT8			bRange0;
	UINT8			bRange1;
	UINT8			bBypass;
	UINT8			bDioSdo0Hiz;
	UINT8			bDioSdo1Hiz;
	UINT8			bDioSdo2Hiz;
	UINT8			bDioClk0Hiz;
	UINT8			bDioClk1Hiz;
	UINT8			bDioClk2Hiz;
	UINT8			bPcmHiz;
	UINT8			bLineIn1Dif;
	UINT8			bLineIn2Dif;
	UINT8			bLineOut1Dif;
	UINT8			bLineOut2Dif;
	UINT8			bSpmn;
	UINT8			bMic1Sng;
	UINT8			bMic2Sng;
	UINT8			bMic3Sng;
	UINT8			bPowerMode;
	UINT8			bSpHiz;
	UINT8			bLdo;
	UINT8			bPad0Func;
	UINT8			bPad1Func;
	UINT8			bPad2Func;
	UINT8			bAvddLev;
	UINT8			bVrefLev;
	UINT8			bDclGain;
	UINT8			bDclLimit;
	UINT8			bCpMod;
	UINT8			bReserved1;
	UINT8			bReserved2;
	UINT8			bReserved3;
	UINT8			bReserved4;
	UINT8			bReserved5;
	MCDRV_WAIT_TIME	sWaitTime;
} MCDRV_INIT_INFO;

/*	update clock	*/
typedef struct
{
	UINT8	bCkSel;
	UINT8	bDivR0;
	UINT8	bDivF0;
	UINT8	bDivR1;
	UINT8	bDivF1;
	UINT8	bRange0;
	UINT8	bRange1;
	UINT8	bBypass;
} MCDRV_CLOCK_INFO;

/*	switch clock	*/
/*	MCDRV_CLKSW_INFO bClkSrc setting	*/
#define	MCDRV_CLKSW_CLKI0	(0x00)
#define	MCDRV_CLKSW_CLKI1	(0x01)

typedef struct
{
	UINT8	bClkSrc;
} MCDRV_CLKSW_INFO;

/*	set/get path	*/
#define	SOURCE_BLOCK_NUM			(7)
#define	MCDRV_SRC_MIC1_BLOCK		(0)
#define	MCDRV_SRC_MIC2_BLOCK		(0)
#define	MCDRV_SRC_MIC3_BLOCK		(0)
#define	MCDRV_SRC_LINE1_L_BLOCK		(1)
#define	MCDRV_SRC_LINE1_R_BLOCK		(1)
#define	MCDRV_SRC_LINE1_M_BLOCK		(1)
#define	MCDRV_SRC_LINE2_L_BLOCK		(2)
#define	MCDRV_SRC_LINE2_R_BLOCK		(2)
#define	MCDRV_SRC_LINE2_M_BLOCK		(2)
#define	MCDRV_SRC_DIR0_BLOCK		(3)
#define	MCDRV_SRC_DIR1_BLOCK		(3)
#define	MCDRV_SRC_DIR2_BLOCK		(3)
#define	MCDRV_SRC_DIR2_DIRECT_BLOCK	(3)
#define	MCDRV_SRC_DTMF_BLOCK		(4)
#define	MCDRV_SRC_PDM_BLOCK			(4)
#define	MCDRV_SRC_ADC0_BLOCK		(4)
#define	MCDRV_SRC_ADC1_BLOCK		(4)
#define	MCDRV_SRC_DAC_L_BLOCK		(5)
#define	MCDRV_SRC_DAC_R_BLOCK		(5)
#define	MCDRV_SRC_DAC_M_BLOCK		(5)
#define	MCDRV_SRC_MIX_BLOCK			(6)
#define	MCDRV_SRC_AE_BLOCK			(6)
#define	MCDRV_SRC_CDSP_BLOCK		(6)
#define	MCDRV_SRC_CDSP_DIRECT_BLOCK	(6)

#define	MCDRV_SRC0_MIC1_ON			(0x01)
#define	MCDRV_SRC0_MIC1_OFF			(0x02)
#define	MCDRV_SRC0_MIC2_ON			(0x04)
#define	MCDRV_SRC0_MIC2_OFF			(0x08)
#define	MCDRV_SRC0_MIC3_ON			(0x10)
#define	MCDRV_SRC0_MIC3_OFF			(0x20)
#define	MCDRV_SRC1_LINE1_L_ON		(0x01)
#define	MCDRV_SRC1_LINE1_L_OFF		(0x02)
#define	MCDRV_SRC1_LINE1_R_ON		(0x04)
#define	MCDRV_SRC1_LINE1_R_OFF		(0x08)
#define	MCDRV_SRC1_LINE1_M_ON		(0x10)
#define	MCDRV_SRC1_LINE1_M_OFF		(0x20)
#define	MCDRV_SRC2_LINE2_L_ON		(0x01)
#define	MCDRV_SRC2_LINE2_L_OFF		(0x02)
#define	MCDRV_SRC2_LINE2_R_ON		(0x04)
#define	MCDRV_SRC2_LINE2_R_OFF		(0x08)
#define	MCDRV_SRC2_LINE2_M_ON		(0x10)
#define	MCDRV_SRC2_LINE2_M_OFF		(0x20)
#define	MCDRV_SRC3_DIR0_ON			(0x01)
#define	MCDRV_SRC3_DIR0_OFF			(0x02)
#define	MCDRV_SRC3_DIR1_ON			(0x04)
#define	MCDRV_SRC3_DIR1_OFF			(0x08)
#define	MCDRV_SRC3_DIR2_ON			(0x10)
#define	MCDRV_SRC3_DIR2_OFF			(0x20)
#define	MCDRV_SRC3_DIR2_DIRECT_ON	(0x40)
#define	MCDRV_SRC3_DIR2_DIRECT_OFF	(0x80)
#define	MCDRV_SRC4_DTMF_ON			(0x01)
#define	MCDRV_SRC4_DTMF_OFF			(0x02)
#define	MCDRV_SRC4_PDM_ON			(0x04)
#define	MCDRV_SRC4_PDM_OFF			(0x08)
#define	MCDRV_SRC4_ADC0_ON			(0x10)
#define	MCDRV_SRC4_ADC0_OFF			(0x20)
#define	MCDRV_SRC4_ADC1_ON			(0x40)
#define	MCDRV_SRC4_ADC1_OFF			(0x80)
#define	MCDRV_SRC5_DAC_L_ON			(0x01)
#define	MCDRV_SRC5_DAC_L_OFF		(0x02)
#define	MCDRV_SRC5_DAC_R_ON			(0x04)
#define	MCDRV_SRC5_DAC_R_OFF		(0x08)
#define	MCDRV_SRC5_DAC_M_ON			(0x10)
#define	MCDRV_SRC5_DAC_M_OFF		(0x20)
#define	MCDRV_SRC6_MIX_ON			(0x01)
#define	MCDRV_SRC6_MIX_OFF			(0x02)
#define	MCDRV_SRC6_AE_ON			(0x04)
#define	MCDRV_SRC6_AE_OFF			(0x08)
#define	MCDRV_SRC6_CDSP_ON			(0x10)
#define	MCDRV_SRC6_CDSP_OFF			(0x20)
#define	MCDRV_SRC6_CDSP_DIRECT_ON	(0x40)
#define	MCDRV_SRC6_CDSP_DIRECT_OFF	(0x80)

typedef struct
{
	UINT8	abSrcOnOff[SOURCE_BLOCK_NUM];
} MCDRV_CHANNEL;

#define	HP_PATH_CHANNELS		(2)
#define	SP_PATH_CHANNELS		(2)
#define	RC_PATH_CHANNELS		(1)
#define	LOUT1_PATH_CHANNELS		(2)
#define	LOUT2_PATH_CHANNELS		(2)
#define	PEAK_PATH_CHANNELS		(1)
#define	DIT0_PATH_CHANNELS		(1)
#define	DIT1_PATH_CHANNELS		(1)
#define	DIT2_PATH_CHANNELS		(1)
#define	DAC_PATH_CHANNELS		(2)
#define	AE_PATH_CHANNELS		(1)
#define	CDSP_PATH_CHANNELS		(4)
#define	ADC0_PATH_CHANNELS		(2)
#define	ADC1_PATH_CHANNELS		(1)
#define	MIX_PATH_CHANNELS		(1)
#define	BIAS_PATH_CHANNELS		(1)

typedef struct
{
	MCDRV_CHANNEL	asHpOut[HP_PATH_CHANNELS];
	MCDRV_CHANNEL	asSpOut[SP_PATH_CHANNELS];
	MCDRV_CHANNEL	asRcOut[RC_PATH_CHANNELS];
	MCDRV_CHANNEL	asLout1[LOUT1_PATH_CHANNELS];
	MCDRV_CHANNEL	asLout2[LOUT2_PATH_CHANNELS];
	MCDRV_CHANNEL	asPeak[PEAK_PATH_CHANNELS];
	MCDRV_CHANNEL	asDit0[DIT0_PATH_CHANNELS];
	MCDRV_CHANNEL	asDit1[DIT1_PATH_CHANNELS];
	MCDRV_CHANNEL	asDit2[DIT2_PATH_CHANNELS];
	MCDRV_CHANNEL	asDac[DAC_PATH_CHANNELS];
	MCDRV_CHANNEL	asAe[AE_PATH_CHANNELS];
	MCDRV_CHANNEL	asCdsp[CDSP_PATH_CHANNELS];
	MCDRV_CHANNEL	asAdc0[ADC0_PATH_CHANNELS];
	MCDRV_CHANNEL	asAdc1[ADC1_PATH_CHANNELS];
	MCDRV_CHANNEL	asMix[MIX_PATH_CHANNELS];
	MCDRV_CHANNEL	asBias[BIAS_PATH_CHANNELS];
} MCDRV_PATH_INFO;

/*	set/get vol	*/
#define	MCDRV_VOL_UPDATE		(0x0001)

#define	AD0_VOL_CHANNELS		(2)
#define	AD1_VOL_CHANNELS		(1)
#define	AENG6_VOL_CHANNELS		(2)
#define	PDM_VOL_CHANNELS		(2)
#define	DTMF_VOL_CHANNELS		(2)
#define	DIO0_VOL_CHANNELS		(2)
#define	DIO1_VOL_CHANNELS		(2)
#define	DIO2_VOL_CHANNELS		(2)
#define	DTFM_VOL_CHANNELS		(2)
#define	DAC_VOL_CHANNELS		(2)

#define	LIN1_VOL_CHANNELS		(2)
#define	LIN2_VOL_CHANNELS		(2)
#define	MIC1_VOL_CHANNELS		(1)
#define	MIC2_VOL_CHANNELS		(1)
#define	MIC3_VOL_CHANNELS		(1)
#define	HP_VOL_CHANNELS			(2)
#define	SP_VOL_CHANNELS			(2)
#define	RC_VOL_CHANNELS			(1)
#define	LOUT1_VOL_CHANNELS		(2)
#define	LOUT2_VOL_CHANNELS		(2)
#define	HPGAIN_VOL_CHANNELS		(1)

typedef struct
{
	SINT16	aswD_Ad0[AD0_VOL_CHANNELS];
	SINT16	aswD_Ad1[AD1_VOL_CHANNELS];
	SINT16	aswD_Aeng6[AENG6_VOL_CHANNELS];
	SINT16	aswD_Pdm[PDM_VOL_CHANNELS];
	SINT16	aswD_Dtmfb[DTMF_VOL_CHANNELS];
	SINT16	aswD_Dir0[DIO0_VOL_CHANNELS];
	SINT16	aswD_Dir1[DIO1_VOL_CHANNELS];
	SINT16	aswD_Dir2[DIO2_VOL_CHANNELS];
	SINT16	aswD_Ad0Att[AD0_VOL_CHANNELS];
	SINT16	aswD_Ad1Att[AD1_VOL_CHANNELS];
	SINT16	aswD_Dir0Att[DIO0_VOL_CHANNELS];
	SINT16	aswD_Dir1Att[DIO1_VOL_CHANNELS];
	SINT16	aswD_Dir2Att[DIO2_VOL_CHANNELS];
	SINT16	aswD_SideTone[PDM_VOL_CHANNELS];
	SINT16	aswD_DtmfAtt[DTFM_VOL_CHANNELS];
	SINT16	aswD_DacMaster[DAC_VOL_CHANNELS];
	SINT16	aswD_DacVoice[DAC_VOL_CHANNELS];
	SINT16	aswD_DacAtt[DAC_VOL_CHANNELS];
	SINT16	aswD_Dit0[DIO0_VOL_CHANNELS];
	SINT16	aswD_Dit1[DIO1_VOL_CHANNELS];
	SINT16	aswD_Dit2[DIO2_VOL_CHANNELS];
	SINT16	aswA_Ad0[AD0_VOL_CHANNELS];
	SINT16	aswA_Ad1[AD1_VOL_CHANNELS];
	SINT16	aswA_Lin1[LIN1_VOL_CHANNELS];
	SINT16	aswA_Lin2[LIN2_VOL_CHANNELS];
	SINT16	aswA_Mic1[MIC1_VOL_CHANNELS];
	SINT16	aswA_Mic2[MIC2_VOL_CHANNELS];
	SINT16	aswA_Mic3[MIC3_VOL_CHANNELS];
	SINT16	aswA_Hp[HP_VOL_CHANNELS];
	SINT16	aswA_Sp[SP_VOL_CHANNELS];
	SINT16	aswA_Rc[RC_VOL_CHANNELS];
	SINT16	aswA_Lout1[LOUT1_VOL_CHANNELS];
	SINT16	aswA_Lout2[LOUT2_VOL_CHANNELS];
	SINT16	aswA_Mic1Gain[MIC1_VOL_CHANNELS];
	SINT16	aswA_Mic2Gain[MIC2_VOL_CHANNELS];
	SINT16	aswA_Mic3Gain[MIC3_VOL_CHANNELS];
	SINT16	aswA_HpGain[HPGAIN_VOL_CHANNELS];
} MCDRV_VOL_INFO;

/*	set/get digitalio	*/
#define	MCDRV_DIO0_COM_UPDATE_FLAG	((UINT32)0x00000001)
#define	MCDRV_DIO0_DIR_UPDATE_FLAG	((UINT32)0x00000002)
#define	MCDRV_DIO0_DIT_UPDATE_FLAG	((UINT32)0x00000004)
#define	MCDRV_DIO1_COM_UPDATE_FLAG	((UINT32)0x00000008)
#define	MCDRV_DIO1_DIR_UPDATE_FLAG	((UINT32)0x00000010)
#define	MCDRV_DIO1_DIT_UPDATE_FLAG	((UINT32)0x00000020)
#define	MCDRV_DIO2_COM_UPDATE_FLAG	((UINT32)0x00000040)
#define	MCDRV_DIO2_DIR_UPDATE_FLAG	((UINT32)0x00000080)
#define	MCDRV_DIO2_DIT_UPDATE_FLAG	((UINT32)0x00000100)

/*	MCDRV_DIO_COMMON bMasterSlave setting	*/
#define	MCDRV_DIO_SLAVE				(0)
#define	MCDRV_DIO_MASTER			(1)

/*	MCDRV_DIO_COMMON bDigitalAutoFs setting	*/
#define	MCDRV_AUTOFS_OFF			(0)
#define	MCDRV_AUTOFS_ON				(1)

/*	MCDRV_DIO_COMMON bFs setting	*/
#define	MCDRV_FS_48000				(0)
#define	MCDRV_FS_44100				(1)
#define	MCDRV_FS_32000				(2)
#define	MCDRV_FS_24000				(4)
#define	MCDRV_FS_22050				(5)
#define	MCDRV_FS_16000				(6)
#define	MCDRV_FS_12000				(8)
#define	MCDRV_FS_11025				(9)
#define	MCDRV_FS_8000				(10)

/*	MCDRV_DIO_COMMON bBckFs setting	*/
#define	MCDRV_BCKFS_64				(0)
#define	MCDRV_BCKFS_48				(1)
#define	MCDRV_BCKFS_32				(2)
#define	MCDRV_BCKFS_512				(4)
#define	MCDRV_BCKFS_256				(5)
#define	MCDRV_BCKFS_128				(6)
#define	MCDRV_BCKFS_16				(7)

/*	MCDRV_DIO_COMMON bInterface setting	*/
#define	MCDRV_DIO_DA				(0)
#define	MCDRV_DIO_PCM				(1)

/*	MCDRV_DIO_COMMON bBckInvert setting	*/
#define	MCDRV_BCLK_NORMAL			(0)
#define	MCDRV_BCLK_INVERT			(1)

/*	MCDRV_DIO_COMMON bPcmHizTim setting	*/
#define	MCDRV_PCMHIZTIM_FALLING		(0)
#define	MCDRV_PCMHIZTIM_RISING		(1)

/*	MCDRV_DIO_COMMON bPcmClkDown setting	*/
#define	MCDRV_PCM_CLKDOWN_OFF		(0)
#define	MCDRV_PCM_CLKDOWN_HALF		(1)

/*	MCDRV_DIO_COMMON bPcmFrame setting	*/
#define	MCDRV_PCM_SHORTFRAME		(0)
#define	MCDRV_PCM_LONGFRAME			(1)

typedef struct
{
	UINT8	bMasterSlave;
	UINT8	bAutoFs;
	UINT8	bFs;
	UINT8	bBckFs;
	UINT8	bInterface;
	UINT8	bBckInvert;
	UINT8	bPcmHizTim;
	UINT8	bPcmClkDown;
	UINT8	bPcmFrame;
	UINT8	bPcmHighPeriod;
} MCDRV_DIO_COMMON;

/*	MCDRV_DA_FORMAT bBitSel setting	*/
#define	MCDRV_BITSEL_16				(0)
#define	MCDRV_BITSEL_20				(1)
#define	MCDRV_BITSEL_24				(2)

/*	MCDRV_DA_FORMAT bMode setting	*/
#define	MCDRV_DAMODE_HEADALIGN		(0)
#define	MCDRV_DAMODE_I2S			(1)
#define	MCDRV_DAMODE_TAILALIGN		(2)

typedef struct
{
	UINT8	bBitSel;
	UINT8	bMode;
} MCDRV_DA_FORMAT;

/*	MCDRV_PCM_FORMAT bMono setting	*/
#define	MCDRV_PCM_STEREO			(0)
#define	MCDRV_PCM_MONO				(1)

/*	MCDRV_PCM_FORMAT bOrder setting	*/
#define	MCDRV_PCM_MSB_FIRST			(0)
#define	MCDRV_PCM_LSB_FIRST			(1)
#define	MCDRV_PCM_MSB_FIRST_SIGN	(2)
#define	MCDRV_PCM_LSB_FIRST_SIGN	(3)
#define	MCDRV_PCM_MSB_FIRST_ZERO	(4)
#define	MCDRV_PCM_LSB_FIRST_ZERO	(5)

/*	MCDRV_PCM_FORMAT bLaw setting	*/
#define	MCDRV_PCM_LINEAR			(0)
#define	MCDRV_PCM_ALAW				(1)
#define	MCDRV_PCM_MULAW				(2)

/*	MCDRV_PCM_FORMAT bBitSel setting	*/
#define	MCDRV_PCM_BITSEL_8			(0)
#define	MCDRV_PCM_BITSEL_13			(1)
#define	MCDRV_PCM_BITSEL_14			(2)
#define	MCDRV_PCM_BITSEL_16			(3)

typedef struct
{
	UINT8	bMono;
	UINT8	bOrder;
	UINT8	bLaw;
	UINT8	bBitSel;
} MCDRV_PCM_FORMAT;

#define	DIO_CHANNELS		(2)
typedef struct
{
	UINT16				wSrcRate;
	MCDRV_DA_FORMAT		sDaFormat;
	MCDRV_PCM_FORMAT	sPcmFormat;
	UINT8				abSlot[DIO_CHANNELS];
} MCDRV_DIO_DIR;

typedef struct
{
	UINT16				wSrcRate;
	MCDRV_DA_FORMAT		sDaFormat;
	MCDRV_PCM_FORMAT	sPcmFormat;
	UINT8				abSlot[DIO_CHANNELS];
} MCDRV_DIO_DIT;

typedef struct
{
	MCDRV_DIO_COMMON	sDioCommon;
	MCDRV_DIO_DIR		sDir;
	MCDRV_DIO_DIT		sDit;
} MCDRV_DIO_PORT;

#define	IOPORT_NUM			(3)
typedef struct
{
	MCDRV_DIO_PORT	asPortInfo[IOPORT_NUM];
} MCDRV_DIO_INFO;

/*	set dac	*/
#define	MCDRV_DAC_MSWP_UPDATE_FLAG	((UINT32)0x01)
#define	MCDRV_DAC_VSWP_UPDATE_FLAG	((UINT32)0x02)
#define	MCDRV_DAC_HPF_UPDATE_FLAG	((UINT32)0x04)

/*	MCDRV_DAC_INFO bMasterSwap/bVoiceSwap setting	*/
#define	MCDRV_DSWAP_OFF			(0)
#define	MCDRV_DSWAP_SWAP		(1)
#define	MCDRV_DSWAP_MUTE		(2)
#define	MCDRV_DSWAP_RMVCENTER	(3)
#define	MCDRV_DSWAP_MONO		(4)
#define	MCDRV_DSWAP_MONOHALF	(5)
#define	MCDRV_DSWAP_BOTHL		(6)
#define	MCDRV_DSWAP_BOTHR		(7)

/*	MCDRV_DAC_INFO bDcCut setting	*/
#define	MCDRV_DCCUT_ON			(0)
#define	MCDRV_DCCUT_OFF			(1)

typedef struct
{
	UINT8	bMasterSwap;
	UINT8	bVoiceSwap;
	UINT8	bDcCut;
} MCDRV_DAC_INFO;

/*	set adc	*/
#define	MCDRV_ADCADJ_UPDATE_FLAG	((UINT32)0x00000001)
#define	MCDRV_ADCAGC_UPDATE_FLAG	((UINT32)0x00000002)
#define	MCDRV_ADCMONO_UPDATE_FLAG	((UINT32)0x00000004)

/*	MCDRV_ADC_INFO bAgcAdjust setting	*/
#define	MCDRV_AGCADJ_24			(0)
#define	MCDRV_AGCADJ_18			(1)
#define	MCDRV_AGCADJ_12			(2)
#define	MCDRV_AGCADJ_0			(3)

/*	MCDRV_ADC_INFO bAgcOn setting	*/
#define	MCDRV_AGC_OFF			(0)
#define	MCDRV_AGC_ON			(1)

/*	MCDRV_ADC_INFO bMono setting	*/
#define	MCDRV_ADC_STEREO		(0)
#define	MCDRV_ADC_MONO			(1)

typedef struct
{
	UINT8	bAgcAdjust;
	UINT8	bAgcOn;
	UINT8	bMono;
} MCDRV_ADC_INFO;

/*	set sp	*/
/*	MCDRV_SP_INFO bSwap setting	*/
#define	MCDRV_SPSWAP_OFF		(0)
#define	MCDRV_SPSWAP_SWAP		(1)

typedef struct
{
	UINT8	bSwap;
} MCDRV_SP_INFO;

/*	set dng	*/
#define	DNG_ITEM_NUM				(3)
#define	MCDRV_DNG_ITEM_HP			(0)
#define	MCDRV_DNG_ITEM_SP			(1)
#define	MCDRV_DNG_ITEM_RC			(2)

#define	MCDRV_DNGSW_HP_UPDATE_FLAG		((UINT32)0x00000001)
#define	MCDRV_DNGTHRES_HP_UPDATE_FLAG	((UINT32)0x00000002)
#define	MCDRV_DNGHOLD_HP_UPDATE_FLAG	((UINT32)0x00000004)
#define	MCDRV_DNGATK_HP_UPDATE_FLAG		((UINT32)0x00000008)
#define	MCDRV_DNGREL_HP_UPDATE_FLAG		((UINT32)0x00000010)
#define	MCDRV_DNGTARGET_HP_UPDATE_FLAG	((UINT32)0x00000020)
#define	MCDRV_DNGSW_SP_UPDATE_FLAG		((UINT32)0x00000100)
#define	MCDRV_DNGTHRES_SP_UPDATE_FLAG	((UINT32)0x00000200)
#define	MCDRV_DNGHOLD_SP_UPDATE_FLAG	((UINT32)0x00000400)
#define	MCDRV_DNGATK_SP_UPDATE_FLAG		((UINT32)0x00000800)
#define	MCDRV_DNGREL_SP_UPDATE_FLAG		((UINT32)0x00001000)
#define	MCDRV_DNGTARGET_SP_UPDATE_FLAG	((UINT32)0x00002000)
#define	MCDRV_DNGSW_RC_UPDATE_FLAG		((UINT32)0x00010000)
#define	MCDRV_DNGTHRES_RC_UPDATE_FLAG	((UINT32)0x00020000)
#define	MCDRV_DNGHOLD_RC_UPDATE_FLAG	((UINT32)0x00040000)
#define	MCDRV_DNGATK_RC_UPDATE_FLAG		((UINT32)0x00080000)
#define	MCDRV_DNGREL_RC_UPDATE_FLAG		((UINT32)0x00100000)
#define	MCDRV_DNGTARGET_RC_UPDATE_FLAG	((UINT32)0x00200000)

/*	MCDRV_DNG_INFO bOnOff setting	*/
#define	MCDRV_DNG_OFF				(0)
#define	MCDRV_DNG_ON				(1)

/*	MCDRV_DNG_INFO bThreshold setting	*/
#define	MCDRV_DNG_THRES_30			(0)
#define	MCDRV_DNG_THRES_36			(1)
#define	MCDRV_DNG_THRES_42			(2)
#define	MCDRV_DNG_THRES_48			(3)
#define	MCDRV_DNG_THRES_54			(4)
#define	MCDRV_DNG_THRES_60			(5)
#define	MCDRV_DNG_THRES_66			(6)
#define	MCDRV_DNG_THRES_72			(7)
#define	MCDRV_DNG_THRES_78			(8)
#define	MCDRV_DNG_THRES_84			(9)

/*	MCDRV_DNG_INFO bHold setting	*/
#define	MCDRV_DNG_HOLD_30			(0)
#define	MCDRV_DNG_HOLD_120			(1)
#define	MCDRV_DNG_HOLD_500			(2)

/*	MCDRV_DNG_INFO bAttack setting	*/
#define	MCDRV_DNG_ATTACK_25			(0)
#define	MCDRV_DNG_ATTACK_100		(1)
#define	MCDRV_DNG_ATTACK_400		(2)
#define	MCDRV_DNG_ATTACK_800		(3)

/*	MCDRV_DNG_INFO bRelease setting	*/
#define	MCDRV_DNG_RELEASE_7950		(0)
#define	MCDRV_DNG_RELEASE_470		(1)
#define	MCDRV_DNG_RELEASE_940		(2)

/*	MCDRV_DNG_INFO bTarget setting	*/
#define	MCDRV_DNG_TARGET_6			(0)
#define	MCDRV_DNG_TARGET_9			(1)
#define	MCDRV_DNG_TARGET_12			(2)
#define	MCDRV_DNG_TARGET_15			(3)
#define	MCDRV_DNG_TARGET_18			(4)
#define	MCDRV_DNG_TARGET_MUTE		(5)

typedef struct
{
	UINT8	abOnOff[DNG_ITEM_NUM];
	UINT8	abThreshold[DNG_ITEM_NUM];
	UINT8	abHold[DNG_ITEM_NUM];
	UINT8	abAttack[DNG_ITEM_NUM];
	UINT8	abRelease[DNG_ITEM_NUM];
	UINT8	abTarget[DNG_ITEM_NUM];
}	MCDRV_DNG_INFO;

/*	set audio engine	*/
#define	MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF	((UINT32)0x00000001)
#define	MCDRV_AEUPDATE_FLAG_DRC_ONOFF		((UINT32)0x00000002)
#define	MCDRV_AEUPDATE_FLAG_EQ5_ONOFF		((UINT32)0x00000004)
#define	MCDRV_AEUPDATE_FLAG_EQ3_ONOFF		((UINT32)0x00000008)
#define	MCDRV_AEUPDATE_FLAG_BEX				((UINT32)0x00000010)
#define	MCDRV_AEUPDATE_FLAG_WIDE			((UINT32)0x00000020)
#define	MCDRV_AEUPDATE_FLAG_DRC				((UINT32)0x00000040)
#define	MCDRV_AEUPDATE_FLAG_EQ5				((UINT32)0x00000080)
#define	MCDRV_AEUPDATE_FLAG_EQ3				((UINT32)0x00000100)

/*	MCDRV_AE_INFO bOnOff setting	*/
#define	MCDRV_BEXWIDE_ON			(0x01)
#define	MCDRV_DRC_ON				(0x02)
#define	MCDRV_EQ5_ON				(0x04)
#define	MCDRV_EQ3_ON				(0x08)

#define	BEX_PARAM_SIZE				(104)
#define	WIDE_PARAM_SIZE				(20)
#define	DRC_PARAM_SIZE				(256)
#define	EQ5_PARAM_SIZE				(105)
#define	EQ3_PARAM_SIZE				(75)

typedef struct
{
	UINT8	bOnOff;
	UINT8	abBex[BEX_PARAM_SIZE];
	UINT8	abWide[WIDE_PARAM_SIZE];
	UINT8	abDrc[DRC_PARAM_SIZE];
	UINT8	abEq5[EQ5_PARAM_SIZE];
	UINT8	abEq3[EQ3_PARAM_SIZE];
}	MCDRV_AE_INFO;

/*	set cdsp param	*/
typedef struct
{
	UINT8	bId;
	UINT8	abParam[16];
} MCDRV_CDSPPARAM;

/*	register cdsp cb	*/
/*	dEvtType	*/
#define	MCDRV_CDSP_EVT_ERROR		(0)
#define	MCDRV_CDSP_EVT_PARAM		(1)

/*	dEvtPrm	*/
#define	MCDRV_CDSP_PRG_ERROR		(0)
#define	MCDRV_CDSP_PRG_ERROR_FATAL	(1)
#define	MCDRV_CDSP_SYS_ERROR		(2)

/*	set pdm	*/
#define	MCDRV_PDMCLK_UPDATE_FLAG	((UINT32)0x00000001)
#define	MCDRV_PDMADJ_UPDATE_FLAG	((UINT32)0x00000002)
#define	MCDRV_PDMAGC_UPDATE_FLAG	((UINT32)0x00000004)
#define	MCDRV_PDMEDGE_UPDATE_FLAG	((UINT32)0x00000008)
#define	MCDRV_PDMWAIT_UPDATE_FLAG	((UINT32)0x00000010)
#define	MCDRV_PDMSEL_UPDATE_FLAG	((UINT32)0x00000020)
#define	MCDRV_PDMMONO_UPDATE_FLAG	((UINT32)0x00000040)

/*	MCDRV_PDM_INFO bClk setting	*/
#define	MCDRV_PDM_CLK_128			(1)
#define	MCDRV_PDM_CLK_64			(2)
#define	MCDRV_PDM_CLK_32			(3)

/*	MCDRV_PDM_INFO bPdmEdge setting	*/
#define	MCDRV_PDMEDGE_LH			(0)
#define	MCDRV_PDMEDGE_HL			(1)

/*	MCDRV_PDM_INFO bPdmWait setting	*/
#define	MCDRV_PDMWAIT_0				(0)
#define	MCDRV_PDMWAIT_1				(1)
#define	MCDRV_PDMWAIT_10			(2)
#define	MCDRV_PDMWAIT_20			(3)

/*	MCDRV_PDM_INFO bPdmSel setting	*/
#define	MCDRV_PDMSEL_L1R2			(0)
#define	MCDRV_PDMSEL_L2R1			(1)
#define	MCDRV_PDMSEL_L1R1			(2)
#define	MCDRV_PDMSEL_L2R2			(3)

/*	MCDRV_PDM_INFO bMono setting	*/
#define	MCDRV_PDM_STEREO			(0)
#define	MCDRV_PDM_MONO				(1)

typedef struct
{
	UINT8	bClk;
	UINT8	bAgcAdjust;
	UINT8	bAgcOn;
	UINT8	bPdmEdge;
	UINT8	bPdmWait;
	UINT8	bPdmSel;
	UINT8	bMono;
} MCDRV_PDM_INFO;

/*	set dtmf	*/
typedef struct
{
	UINT8	bSinGen0Vol;
	UINT8	bSinGen1Vol;
	UINT16	wSinGen0Freq;
	UINT16	wSinGen1Freq;
	UINT8	bSinGenGate;
	UINT8	bSinGenMode;
	UINT8	bSinGenLoop;
} MCDRV_DTMF_PARAM;

/*	MCDRV_DTMF_INFO bOnOff setting	*/
#define	MCDRV_DTMF_ON				(0)
#define	MCDRV_DTMF_OFF				(1)

typedef struct
{
	UINT8				bOnOff;
	MCDRV_DTMF_PARAM	sParam;
} MCDRV_DTMF_INFO;

/*	config gp	*/
#define	GPIO_PAD_NUM				(2)

/*	MCDRV_GP_MODE abGpDdr setting	*/
#define	MCDRV_GPDDR_IN				(0)
#define	MCDRV_GPDDR_OUT				(1)

/*	MCDRV_GP_MODE abGpMode setting	*/
#define	MCDRV_GPMODE_RISING			(0)
#define	MCDRV_GPMODE_FALLING		(1)
#define	MCDRV_GPMODE_BOTH			(2)

/*	MCDRV_GP_MODE abGpHost setting	*/
#define	MCDRV_GPHOST_SCU			(0)
#define	MCDRV_GPHOST_CDSP			(1)

/*	MCDRV_GP_MODE abGpInvert setting	*/
#define	MCDRV_GPINV_NORMAL			(0)
#define	MCDRV_GPINV_INVERT			(1)

typedef struct
{
	UINT8	abGpDdr[GPIO_PAD_NUM];
	UINT8	abGpMode[GPIO_PAD_NUM];
	UINT8	abGpHost[GPIO_PAD_NUM];
	UINT8	abGpInvert[GPIO_PAD_NUM];
} MCDRV_GP_MODE;

/*	mask gp	*/
#define	MCDRV_GPMASK_OFF			(0)
#define	MCDRV_GPMASK_ON				(1)

#define	MCDRV_GP_PAD0				((UINT32)0)
#define	MCDRV_GP_PAD1				((UINT32)1)

/*	getset gp	*/
#define	MCDRV_GP_LOW				(0)
#define	MCDRV_GP_HIGH				(1)

/*	get peak	*/
#define	PEAK_CHANNELS				(2)
typedef struct
{
	SINT16	aswPeak[PEAK_CHANNELS];
} MCDRV_PEAK;

/*	set/get syseq	*/
#define	MCDRV_SYSEQ_ONOFF_UPDATE_FLAG	((UINT32)0x00000001)
#define	MCDRV_SYSEQ_PARAM_UPDATE_FLAG	((UINT32)0x00000002)

/*	MCDRV_SYSEQ_INFO bOnOff setting	*/
#define	MCDRV_SYSEQ_OFF				(0)
#define	MCDRV_SYSEQ_ON				(1)

typedef struct
{
	UINT8	bOnOff;
	UINT8	abParam[15];
} MCDRV_SYSEQ_INFO;

/*	read_reg, write_reg	*/
#define	MCDRV_REGTYPE_A				(0)
#define	MCDRV_REGTYPE_B_BASE		(1)
#define	MCDRV_REGTYPE_B_MIXER		(2)
#define	MCDRV_REGTYPE_B_AE			(3)
#define	MCDRV_REGTYPE_B_CDSP		(4)
#define	MCDRV_REGTYPE_B_CODEC		(5)
#define	MCDRV_REGTYPE_B_ANALOG		(6)
typedef struct
{
	UINT8	bRegType;
	UINT8	bAddress;
	UINT8	bData;
} MCDRV_REG_INFO;


#endif /* _MCDRIVER_H_ */
