/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcresctrl.c
 *
 *	Description	: MC Driver resource control
 *
 *	Version		: 1.0.6	2013.01.24
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
 *
 ****************************************************************************/


#include "mcresctrl.h"
#include "mcdevif.h"
#include "mcservice.h"
#include "mcdriver.h"
#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcmachdep.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif



#ifndef GET_ARRAY_SIZE
#define	GET_ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))
#endif

#define	D1SRC_ALL_OFF	(MCDRV_D1SRC_MUSICIN_OFF	\
			| MCDRV_D1SRC_EXTIN_OFF		\
			| MCDRV_D1SRC_VBOXOUT_OFF	\
			| MCDRV_D1SRC_VBOXREFOUT_OFF	\
			| MCDRV_D1SRC_AE0_OFF		\
			| MCDRV_D1SRC_AE1_OFF		\
			| MCDRV_D1SRC_AE2_OFF		\
			| MCDRV_D1SRC_AE3_OFF		\
			| MCDRV_D1SRC_ADIF0_OFF		\
			| MCDRV_D1SRC_ADIF1_OFF		\
			| MCDRV_D1SRC_ADIF2_OFF		\
			| MCDRV_D1SRC_HIFIIN_OFF)
#define	D2SRC_ALL_OFF	(MCDRV_D2SRC_VOICEIN_OFF	\
			| MCDRV_D2SRC_VBOXIOOUT_OFF	\
			| MCDRV_D2SRC_VBOXHOSTOUT_OFF	\
			| MCDRV_D2SRC_ADC0_L_OFF	\
			| MCDRV_D2SRC_ADC0_R_OFF	\
			| MCDRV_D2SRC_ADC1_OFF		\
			| MCDRV_D2SRC_PDM0_L_OFF	\
			| MCDRV_D2SRC_PDM0_R_OFF	\
			| MCDRV_D2SRC_PDM1_L_OFF	\
			| MCDRV_D2SRC_PDM1_R_OFF	\
			| MCDRV_D2SRC_DAC0REF_OFF	\
			| MCDRV_D2SRC_DAC1REF_OFF)

static enum MCDRV_STATE	geState	= eMCDRV_STATE_NOTINIT;

static struct MCDRV_GLOBAL_INFO	gsGlobalInfo;
static struct MCDRV_PACKET	gasPacket[MCDRV_MAX_PACKETS+1];

/* register next address */
static const UINT16	gawNextAddr[256]	= {
	0,	1,	2,	3,	4,	5,	6,	7,
	8,	9,	10,	11,	12,	13,	14,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	33,	34,	35,	36,	37,	38,	39,
	40,	41,	42,	43,	44,	45,	46,	47,
	48,	49,	50,	51,	52,	53,	54,	55,
	56,	57,	58,	59,	60,	61,	62,	63,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,	101,	102,	103,
	104,	105,	106,	107,	108,	109,	110,	111,
	112,	113,	114,	115,	116,	117,	118,	119,
	120,	121,	122,	123,	124,	125,	126,	127,
	128,	129,	130,	131,	132,	133,	134,	135,
	136,	137,	138,	139,	140,	141,	142,	143,
	144,	145,	146,	147,	148,	149,	150,	151,
	152,	153,	154,	155,	156,	157,	158,	159,
	160,	161,	162,	163,	164,	165,	166,	167,
	168,	169,	170,	171,	172,	173,	174,	175,
	176,	177,	178,	179,	180,	181,	182,	183,
	184,	185,	186,	187,	188,	189,	190,	191,
	192,	193,	194,	195,	196,	197,	198,	199,
	200,	201,	202,	203,	204,	205,	206,	207,
	208,	209,	210,	211,	212,	213,	214,	215,
	216,	217,	218,	219,	220,	221,	222,	223,
	224,	225,	226,	227,	228,	229,	230,	231,
	232,	233,	234,	235,	236,	237,	238,	239,
	240,	241,	242,	243,	244,	245,	246,	247,
	248,	249,	250,	251,	252,	253,	254,	255
};

static const UINT16	gawNextAddrAInc[256]	= {
	1,	2,	3,	4,	5,	6,	7,	8,
	9,	10,	11,	12,	13,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,	24,
	25,	26,	27,	28,	29,	30,	31,	32,
	33,	34,	35,	36,	37,	38,	39,	40,
	41,	42,	43,	44,	45,	46,	47,	48,
	49,	50,	51,	52,	53,	54,	55,	56,
	57,	58,	59,	60,	61,	62,	63,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
	73,	74,	75,	76,	77,	78,	79,	80,
	81,	82,	83,	84,	85,	86,	87,	88,
	89,	90,	91,	92,	93,	94,	95,	96,
	97,	98,	99,	100,	101,	102,	103,	104,
	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,
	121,	122,	123,	124,	125,	126,	127,	128,
	129,	130,	131,	132,	133,	134,	135,	136,
	137,	138,	139,	140,	141,	142,	143,	144,
	145,	146,	147,	148,	149,	150,	151,	152,
	153,	154,	155,	156,	157,	158,	159,	160,
	161,	162,	163,	164,	165,	166,	167,	168,
	169,	170,	171,	172,	173,	174,	175,	176,
	177,	178,	179,	180,	181,	182,	183,	184,
	185,	186,	187,	188,	189,	190,	191,	192,
	193,	194,	195,	196,	197,	198,	199,	200,
	201,	202,	203,	204,	205,	206,	207,	208,
	209,	210,	211,	212,	213,	214,	215,	216,
	217,	218,	219,	220,	221,	222,	223,	224,
	225,	226,	227,	228,	229,	230,	231,	232,
	233,	234,	235,	236,	237,	238,	239,	240,
	241,	242,	243,	244,	245,	246,	247,	248,
	249,	250,	251,	252,	253,	254,	255,	0xFFFF
};

static void	SetRegDefault(void);
static void	InitClockSw(void);
static void	InitPathInfo(void);
static void	InitVolInfo(void);
static void	InitDioInfo(void);
static void	InitDioPathInfo(void);
static void	InitSwap(void);
static void	InitHSDet(void);
static void	InitGpMode(void);
static void	InitGpMask(void);
static void	InitAecInfo(void);

static SINT32	CheckLpFp(void);
static SINT32	IsValidPath(void);
static UINT8	ValidateADC(void);
static UINT8	ValidateDAC(void);

static void	SetD1SourceOnOff(const struct MCDRV_D1_CHANNEL *psSetDChannel,
				struct MCDRV_D1_CHANNEL *psDstDChannel,
				UINT8	bChannels);
static void	SetD2SourceOnOff(const struct MCDRV_D2_CHANNEL *psSetDChannel,
				struct MCDRV_D2_CHANNEL *psDstDChannel,
				UINT8	bChannels);
static void	SetASourceOnOff(const struct MCDRV_A_CHANNEL *psSetAChannel,
				struct MCDRV_A_CHANNEL *psDstAChannel,
				UINT8 bChannels);
static void	SetBiasSourceOnOff(const struct MCDRV_PATH_INFO *psPathInfo);

static void	ClearD1SourceOnOff(UINT32 *pdSrcOnOff);
static void	ClearD2SourceOnOff(UINT32 *pdSrcOnOff);
static void	ClearASourceOnOff(UINT32 *pdSrcOnOff);
static void	SetSourceOnOff(UINT32 dSrcOnOff,
				UINT32 *pdDstOnOff,
				UINT32 dOn,
				UINT32 dOff);
static UINT32	GetD1Source(struct MCDRV_D1_CHANNEL *psD,
				enum MCDRV_DST_CH eCh);
static UINT32	GetD2Source(struct MCDRV_D2_CHANNEL *psD,
				enum MCDRV_DST_CH eCh);

static void	SetDIOCommon(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);
static void	SetDIODIR(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);
static void	SetDIODIT(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);

static SINT16	GetAnaInVolReg(SINT16 swVol);
static SINT16	GetAnaOutVolReg(SINT16 swVol);
static SINT16	GetSpVolReg(SINT16 swVol);
static SINT16	GetLOutVolReg(SINT16 swVol);
static SINT16	GetHpVolReg(SINT16 swVol);

static SINT32	WaitBitSet(UINT8 bSlaveAddr,
				UINT16 wRegAddr,
				UINT8 bBit,
				UINT32 dCycleTime,
				UINT32 dTimeOut);
static SINT32	WaitBitRelease(UINT8 bSlaveAddr,
				UINT16 wRegAddr,
				UINT8 bBit,
				UINT32 dCycleTime,
				UINT32 dTimeOut);
static SINT32	WaitDSPBitSet(UINT8 bSlaveAddr,
				UINT8 bRegAddr_A,
				UINT8 bRegAddr_D,
				UINT16 wRegAddr,
				UINT8 bBit,
				UINT32 dCycleTime,
				UINT32 dTimeOut);
static SINT32	WaitDSPBitRelease(UINT8 bSlaveAddr,
				UINT8 bRegAddr_A,
				UINT8 bRegAddr_D,
				UINT16 wRegAddr,
				UINT8 bBit,
				UINT32 dCycleTime,
				UINT32 dTimeOut);

/****************************************************************************
 *	McResCtrl_SetHwId
 *
 *	Description:
 *			Set hardware ID.
 *	Arguments:
 *			bHwId_dig	digital block hardware ID
 *			bHwId_ana	analog block hardware ID
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_INIT
 *
 ****************************************************************************/
SINT32	McResCtrl_SetHwId(
	UINT8	bHwId_dig,
	UINT8	bHwId_ana
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetHwId");
#endif

	if (((bHwId_dig&MCDRV_DEVID_MASK) == MCDRV_DEVID_DIG)
	&& ((bHwId_ana&MCDRV_DEVID_MASK) == MCDRV_DEVID_ANA)) {
		if ((bHwId_dig&MCDRV_VERID_MASK) == 1) {
			if ((bHwId_ana&MCDRV_VERID_MASK) == 2)
				McDevProf_SetDevId(eMCDRV_DEV_ID_81_92H);
			else
				McDevProf_SetDevId(eMCDRV_DEV_ID_81_91H);
		} else {
			McDevProf_SetDevId(eMCDRV_DEV_ID_80_90H);
		}
		gsGlobalInfo.bHwId	= bHwId_dig;
		SetRegDefault();
		gsGlobalInfo.abRegValA[MCI_A_DEV_ID]	= gsGlobalInfo.bHwId;
	} else {
		sdRet	= MCDRV_ERROR_INIT;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetHwId", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McResCtrl_Init
 *
 *	Description:
 *			initialize the resource controller.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_Init(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_Init");
#endif

	InitPathInfo();
	InitVolInfo();
	InitDioInfo();
	InitDioPathInfo();
	InitClockSw();
	InitSwap();
	InitHSDet();
	InitGpMode();
	InitGpMask();
	InitAecInfo();

	gsGlobalInfo.bClkSel	= 0;
	gsGlobalInfo.bEClkSel	= 0;
	gsGlobalInfo.bCClkSel	= 0;
	gsGlobalInfo.bFClkSel	= 0;
	gsGlobalInfo.bPlugDetDB	= 0;

	gsGlobalInfo.ePacketBufAlloc	= eMCDRV_PACKETBUF_FREE;
	gsGlobalInfo.pcbfunc		= NULL;

	McResCtrl_InitRegUpdate();

	gsGlobalInfo.eAPMode	= eMCDRV_APM_ON;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_Init", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_InitABlockReg
 *
 *	Description:
 *			Initialize the A Block virtual registers.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_InitABlockReg(
	void
)
{
	UINT16	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_InitABlockReg");
#endif

	for (i = 0; i < MCDRV_REG_NUM_A; i++)
		gsGlobalInfo.abRegValA[i]	= 0;

	gsGlobalInfo.abRegValA[MCI_A_DEV_ID]	= gsGlobalInfo.bHwId;
	gsGlobalInfo.abRegValA[MCI_CLK_MSK]	= MCI_CLK_MSK_DEF;
	gsGlobalInfo.abRegValA[MCI_PD]		= MCI_PD_DEF;
	gsGlobalInfo.abRegValA[MCI_DO0_DRV]	= MCI_DO0_DRV_DEF;
	gsGlobalInfo.abRegValA[MCI_DO1_DRV]	= MCI_DO1_DRV_DEF;
	gsGlobalInfo.abRegValA[MCI_DO2_DRV]	= MCI_DO2_DRV_DEF;
	gsGlobalInfo.abRegValA[MCI_PA0]		= MCI_PA0_DEF;
	gsGlobalInfo.abRegValA[MCI_PA1]		= MCI_PA1_DEF;
	gsGlobalInfo.abRegValA[MCI_PA2]		= MCI_PA2_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValA[MCI_DOA_DRV]	= MCI_DOA_DRV_DEF;
	}
	gsGlobalInfo.abRegValA[MCI_LP1_FP]	= MCI_LP1_FP_DEF;
	gsGlobalInfo.abRegValA[MCI_LP2_FP]	= MCI_LP2_FP_DEF;
	gsGlobalInfo.abRegValA[MCI_LP3_FP]	= MCI_LP3_FP_DEF;
	gsGlobalInfo.abRegValA[MCI_CLKSRC]	= MCI_CLKSRC_DEF;
	gsGlobalInfo.abRegValA[MCI_FREQ73M]	= MCI_FREQ73M_DEF;
	gsGlobalInfo.abRegValA[MCI_PLL_MODE_A]	= MCI_PLL_MODE_A_DEF;
	gsGlobalInfo.abRegValA[MCI_PLL_MODE_B]	= MCI_PLL_MODE_B_DEF;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_InitABlockReg", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_InitMBlockReg
 *
 *	Description:
 *			Initialize the Block M virtual registers.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_InitMBlockReg(
	void
)
{
	UINT16	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_InitMBlockReg");
#endif

	for (i = 0; i < MCDRV_REG_NUM_MA; i++)
		gsGlobalInfo.abRegValMA[i]	= 0;

	gsGlobalInfo.abRegValMA[MCI_I_VINTP]	= MCI_I_VINTP_DEF;
	gsGlobalInfo.abRegValMA[MCI_O_VINTP]	= MCI_O_VINTP_DEF;

	for (i = 0; i < MCDRV_REG_NUM_MB; i++)
		gsGlobalInfo.abRegValMB[i]	= 0;

	gsGlobalInfo.abRegValMB[MCI_LP0_MODE]	= MCI_LP0_MODE_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPR0_SLOT]	= MCI_LPR0_SLOT_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPT0_SLOT]	= MCI_LPT0_SLOT_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPR0_PCM]	= MCI_LPR0_PCM_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPT0_PCM]	= MCI_LPT0_PCM_DEF;
	gsGlobalInfo.abRegValMB[MCI_LP1_MODE]	= MCI_LP1_MODE_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPR1_PCM]	= MCI_LPR1_PCM_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPT1_PCM]	= MCI_LPT1_PCM_DEF;
	gsGlobalInfo.abRegValMB[MCI_LP2_MODE]	= MCI_LP2_MODE_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPR2_PCM]	= MCI_LPR2_PCM_DEF;
	gsGlobalInfo.abRegValMB[MCI_LPT2_PCM]	= MCI_LPT2_PCM_DEF;

	for (i = 0; i < MCDRV_REG_NUM_B; i++)
		gsGlobalInfo.abRegValB[i]	= 0;

	McResCtrl_InitEReg();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_InitMBlockReg", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_InitEReg
 *
 *	Description:
 *			Initialize the E virtual registers.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_InitEReg(
	void
)
{
	UINT16	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_InitEReg");
#endif

	for (i = 0; i < MCDRV_REG_NUM_E; i++)
		gsGlobalInfo.abRegValE[i]	= 0;

	gsGlobalInfo.abRegValE[MCI_E1DSP_CTRL]	= MCI_E1DSP_CTRL_DEF;
	gsGlobalInfo.abRegValE[MCI_LPF_THR]	= MCI_LPF_THR_DEF;
	gsGlobalInfo.abRegValE[MCI_DAC_DCC_SEL]	= MCI_DAC_DCC_SEL_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValE[MCI_OSF_SEL]	= MCI_OSF_SEL_DEF;
	}
	gsGlobalInfo.abRegValE[MCI_OSF_GAIN0_15_8]
		= MCI_OSF_GAIN0_15_8_DEF;
	gsGlobalInfo.abRegValE[MCI_OSF_GAIN0_7_0]
		= MCI_OSF_GAIN0_7_0_DEF;
	gsGlobalInfo.abRegValE[MCI_OSF_GAIN1_15_8]
		= MCI_OSF_GAIN1_15_8_DEF;
	gsGlobalInfo.abRegValE[MCI_OSF_GAIN1_7_0]
		= MCI_OSF_GAIN1_7_0_DEF;
	gsGlobalInfo.abRegValE[MCI_DCL0_LMT_14_8]
		= MCI_DCL0_LMT_14_8_DEF;
	gsGlobalInfo.abRegValE[MCI_DCL0_LMT_7_0]
		= MCI_DCL0_LMT_7_0_DEF;
	gsGlobalInfo.abRegValE[MCI_DCL1_LMT_14_8]
		= MCI_DCL1_LMT_14_8_DEF;
	gsGlobalInfo.abRegValE[MCI_DCL1_LMT_7_0]
		= MCI_DCL1_LMT_7_0_DEF;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValE[MCI_DITHER0]	= MCI_DITHER0_DEF;
		gsGlobalInfo.abRegValE[MCI_DITHER1]	= MCI_DITHER1_DEF;
	} else {
		gsGlobalInfo.abRegValE[MCI_DITHER0]	= 0x30;
		gsGlobalInfo.abRegValE[MCI_DITHER1]	= 0x30;
	}
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValE[MCI_DNG0_ES1]	= MCI_DNG0_DEF_ES1;
		gsGlobalInfo.abRegValE[MCI_DNG1_ES1]	= MCI_DNG1_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValE[MCI_DNG0]	= MCI_DNG0_DEF;
		gsGlobalInfo.abRegValE[MCI_DNG1]	= MCI_DNG1_DEF;
	}
	gsGlobalInfo.abRegValE[MCI_DPATH_DA_V]	= MCI_DPATH_DA_V_DEF;
	gsGlobalInfo.abRegValE[MCI_DPATH_AD_V]	= MCI_DPATH_AD_V_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValE[39]	= 0xF4;
	}
	gsGlobalInfo.abRegValE[MCI_DSF0_PRE_INPUT]
		= MCI_DSF0_PRE_INPUT_DEF;
	gsGlobalInfo.abRegValE[MCI_DSF1_FLT_TYPE]
		= MCI_DSF1_FLT_TYPE_DEF;
	gsGlobalInfo.abRegValE[MCI_DSF1_PRE_INPUT]
		= MCI_DSF1_PRE_INPUT_DEF;
	gsGlobalInfo.abRegValE[MCI_DSF2_PRE_INPUT]
		= MCI_DSF2_PRE_INPUT_DEF;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValE[MCI_ADC_DCC_SEL]
						= MCI_ADC_DCC_SEL_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValE[MCI_ADC_DCC_SEL]	= MCI_ADC_DCC_SEL_DEF;
	}
	gsGlobalInfo.abRegValE[MCI_ADC_DNG0_FW]	= MCI_ADC_DNG0_FW_DEF;
	gsGlobalInfo.abRegValE[MCI_ADC_DNG1_FW]	= MCI_ADC_DNG1_FW_DEF;
	gsGlobalInfo.abRegValE[MCI_ADC_DNG2_FW]	= MCI_ADC_DNG2_FW_DEF;
	gsGlobalInfo.abRegValE[MCI_DEPOP0]	= MCI_DEPOP0_DEF;
	gsGlobalInfo.abRegValE[MCI_DEPOP1]	= MCI_DEPOP1_DEF;
	gsGlobalInfo.abRegValE[MCI_DEPOP2]	= MCI_DEPOP2_DEF;
	gsGlobalInfo.abRegValE[MCI_PDM_MODE]	= MCI_PDM_MODE_DEF;
	gsGlobalInfo.abRegValE[MCI_E2DSP]	= MCI_E2DSP_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValE[99]	= 0xC9;
		gsGlobalInfo.abRegValE[100]	= 0x01;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_InitEReg", 0);
#endif
}

/****************************************************************************
 *	SetRegDefault
 *
 *	Description:
 *			Initialize the virtual registers.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetRegDefault(
	void
)
{
	UINT16	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetRegDefault");
#endif

	for (i = 0; i < MCDRV_REG_NUM_IF; i++)
		gsGlobalInfo.abRegValIF[i]	= 0;

	gsGlobalInfo.abRegValIF[MCI_RST_A]	= MCI_RST_A_DEF;
	gsGlobalInfo.abRegValIF[MCI_RST]	= MCI_RST_DEF;

	McResCtrl_InitABlockReg();

	McResCtrl_InitMBlockReg();

	for (i = 0; i < MCDRV_REG_NUM_C; i++)
		gsGlobalInfo.abRegValC[i]	= 0;

	for (i = 0; i < MCDRV_REG_NUM_F; i++)
		gsGlobalInfo.abRegValF[i]	= 0;

	for (i = 0; i < MCDRV_REG_NUM_ANA; i++)
		gsGlobalInfo.abRegValANA[i]	= 0;

	gsGlobalInfo.abRegValANA[MCI_ANA_ID]	= MCI_ANA_ID_DEF;
	gsGlobalInfo.abRegValANA[MCI_ANA_RST]	= MCI_ANA_RST_DEF;
	gsGlobalInfo.abRegValANA[MCI_AP]	= MCI_AP_DEF;
	gsGlobalInfo.abRegValANA[MCI_AP_DA0]	= MCI_AP_DA0_DEF;
	gsGlobalInfo.abRegValANA[MCI_AP_DA1]	= MCI_AP_DA1_DEF;
	gsGlobalInfo.abRegValANA[MCI_AP_MIC]	= MCI_AP_MIC_DEF;
	gsGlobalInfo.abRegValANA[MCI_AP_AD]	= MCI_AP_AD_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValANA[10]	= 0x06;
		gsGlobalInfo.abRegValANA[MCI_NONCLIP]	= MCI_NONCLIP_DEF;
	}
	gsGlobalInfo.abRegValANA[MCI_SVOL]	= MCI_SVOL_DEF;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValANA[MCI_HIZ]	= MCI_HIZ_DEF;
		gsGlobalInfo.abRegValANA[MCI_LO_HIZ]	= MCI_LO_HIZ_DEF;
	}
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValANA[MCI_ZCOFF]	= MCI_ZCOFF_DEF_ES1;
	}
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValANA[MCI_CPMOD]	= MCI_CPMOD_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_ES1]	= MCI_DNG_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_HP_ES1]
							= MCI_DNG_HP_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_SP_ES1]
							= MCI_DNG_SP_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_RC_ES1]
							= MCI_DNG_RC_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_LO1_ES1]
							= MCI_DNG_LO1_DEF_ES1;
		gsGlobalInfo.abRegValANA[MCI_DNG_LO2_ES1]
							= MCI_DNG_LO2_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValANA[MCI_CPMOD]	= MCI_CPMOD_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG]	= MCI_DNG_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG_HP]	= MCI_DNG_HP_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG_SP]	= MCI_DNG_SP_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG_RC]	= MCI_DNG_RC_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG_LO1]	= MCI_DNG_LO1_DEF;
		gsGlobalInfo.abRegValANA[MCI_DNG_LO2]	= MCI_DNG_LO2_DEF;
		gsGlobalInfo.abRegValANA[114]		= 0x31;
		gsGlobalInfo.abRegValANA[115]		= 0x8B;
	}

	for (i = 0; i < MCDRV_REG_NUM_CD; i++)
		gsGlobalInfo.abRegValCD[i]	= 0;

	gsGlobalInfo.abRegValCD[MCI_HW_ID]	= MCI_HW_ID_DEF;
	gsGlobalInfo.abRegValCD[MCI_CD_RST]	= MCI_CD_RST_DEF;
	gsGlobalInfo.abRegValCD[MCI_DP]		= MCI_DP_DEF;
	gsGlobalInfo.abRegValCD[MCI_DP_OSC]	= MCI_DP_OSC_DEF;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValCD[MCI_CKSEL]	= MCI_CKSEL_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValCD[MCI_CKSEL]	= MCI_CKSEL_DEF;
	}
	gsGlobalInfo.abRegValCD[MCI_MICDET]	= MCI_MICDET_DEF;
	gsGlobalInfo.abRegValCD[MCI_HSDETEN]	= MCI_HSDETEN_DEF;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValCD[MCI_DETIN_INV]	= MCI_DETIN_INV_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValCD[MCI_IRQTYPE]	= MCI_IRQTYPE_DEF;
		gsGlobalInfo.abRegValCD[MCI_DETIN_INV]	= MCI_DETIN_INV_DEF;
	}
	gsGlobalInfo.abRegValCD[MCI_HSDETMODE]	= MCI_HSDETMODE_DEF;
	gsGlobalInfo.abRegValCD[MCI_DBNC_PERIOD]	= MCI_DBNC_PERIOD_DEF;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.abRegValCD[MCI_DBNC_NUM]	= MCI_DBNC_NUM_DEF_ES1;
	} else {
		gsGlobalInfo.abRegValCD[MCI_DBNC_NUM]	= MCI_DBNC_NUM_DEF;
		gsGlobalInfo.abRegValCD[MCI_KEY_MTIM]	= MCI_KEY_MTIM_DEF;
	}
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		gsGlobalInfo.abRegValCD[39]	= 0x21;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetRegDefault", 0);
#endif
}

/****************************************************************************
 *	InitClockSw
 *
 *	Description:
 *			Initialize switch clock info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitClockSw(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitClockSw");
#endif

	gsGlobalInfo.sClockSwInfo.bClkSrc	= MCDRV_CLKSW_CLKA;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitClockSw", 0);
#endif
}

/****************************************************************************
 *	InitPathInfo
 *
 *	Description:
 *			Initialize path info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitPathInfo(
	void
)
{
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitPathInfo");
#endif

	for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff);

	for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff);

	for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff);

	for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff);

	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff);
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff);
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff);
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff);
	}
	for (bCh = 0; bCh < DAC0_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff);

	for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++)
		ClearD1SourceOnOff(
			&gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff);

	for (bCh = 0; bCh < VOICEOUT_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asVoiceOut[bCh].dSrcOnOff);

	for (bCh = 0; bCh < VBOXIOIN_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asVboxIoIn[bCh].dSrcOnOff);

	for (bCh = 0; bCh < VBOXHOSTIN_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asVboxHostIn[bCh].dSrcOnOff);

	for (bCh = 0; bCh < HOSTOUT_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asHostOut[bCh].dSrcOnOff);

	for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff);

	for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff);

	for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++)
		ClearD2SourceOnOff(
			&gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff);

	for (bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asAdc0[bCh].dSrcOnOff);

	for (bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asAdc1[bCh].dSrcOnOff);

	for (bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff);

	for (bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asHp[bCh].dSrcOnOff);

	for (bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asRc[bCh].dSrcOnOff);

	for (bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asLout1[bCh].dSrcOnOff);

	for (bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff);

	for (bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++)
		ClearASourceOnOff(
			&gsGlobalInfo.sPathInfo.asBias[bCh].dSrcOnOff);

	gsGlobalInfo.sPathInfoVirtual	= gsGlobalInfo.sPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitPathInfo", 0);
#endif
}

/****************************************************************************
 *	InitVolInfo
 *
 *	Description:
 *			Initialize volume info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitVolInfo(
	void
)
{
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitVolInfo");
#endif

	for (bCh = 0; bCh < MUSICIN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_MusicIn[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < EXTIN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_ExtIn[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < VOICEIN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_VoiceIn[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < REFIN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_RefIn[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < ADIF0IN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_Adif0In[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < ADIF1IN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_Adif1In[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < ADIF2IN_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_Adif2In[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < MUSICOUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_MusicOut[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < EXTOUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_ExtOut[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < VOICEOUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_VoiceOut[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < REFOUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_RefOut[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < DAC0OUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_Dac0Out[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < DAC1OUT_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswD_Dac1Out[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < DPATH_VOL_CHANNELS; bCh++) {
		gsGlobalInfo.sVolInfo.aswD_DpathDa[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;
		gsGlobalInfo.sVolInfo.aswD_DpathAd[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;
	}

	for (bCh = 0; bCh < LINEIN1_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_LineIn1[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Mic1[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Mic2[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Mic3[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < MIC4_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Mic4[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < HP_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Hp[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < SP_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Sp[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < RC_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_Rc[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < LINEOUT1_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_LineOut1[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < LINEOUT2_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_LineOut2[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

	for (bCh = 0; bCh < HPDET_VOL_CHANNELS; bCh++)
		gsGlobalInfo.sVolInfo.aswA_HpDet[bCh]	=
			MCDRV_LOGICAL_VOL_MUTE;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitVolInfo", 0);
#endif
}

/****************************************************************************
 *	InitDioInfo
 *
 *	Description:
 *			Initialize Digital I/O info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitDioInfo(
	void
)
{
	UINT8	bPort, bPortNum;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitDioInfo");
#endif
	bPortNum	= GET_ARRAY_SIZE(gsGlobalInfo.sDioInfo.asPortInfo);

	for (bPort = 0; bPort < bPortNum; bPort++) {
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bMasterSlave
			= MCDRV_DIO_SLAVE;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bAutoFs
			= MCDRV_AUTOFS_ON;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bFs
			= MCDRV_FS_48000;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bBckFs
			= MCDRV_BCKFS_64;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface
			= MCDRV_DIO_DA;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bBckInvert
			= MCDRV_BCLK_NORMAL;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bSrcThru
			= MCDRV_SRC_NOT_THRU;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmHizTim
			= MCDRV_PCMHIZTIM_FALLING;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmFrame
			= MCDRV_PCM_SHORTFRAME;
		gsGlobalInfo.sDioInfo.asPortInfo[
					bPort].sDioCommon.bPcmHighPeriod
			= 0;

		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bBitSel
			= MCDRV_BITSEL_16;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bMode
			= MCDRV_DAMODE_HEADALIGN;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bMono
			= MCDRV_PCM_MONO;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bOrder
			= MCDRV_PCM_MSB_FIRST;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bLaw
			= MCDRV_PCM_LINEAR;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bBitSel
			= MCDRV_PCM_BITSEL_8;

		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.bStMode
			= MCDRV_STMODE_ZERO;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.bEdge
			= MCDRV_SDOUT_NORMAL;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bBitSel
			= MCDRV_BITSEL_16;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bMode
			= MCDRV_DAMODE_HEADALIGN;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bMono
			= MCDRV_PCM_MONO;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bOrder
			= MCDRV_PCM_MSB_FIRST;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bLaw
			= MCDRV_PCM_LINEAR;
		gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bBitSel
			= MCDRV_PCM_BITSEL_8;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitDioInfo", 0);
#endif
}

/****************************************************************************
 *	InitDioPathInfo
 *
 *	Description:
 *			Initialize Digital I/O path info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitDioPathInfo(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitDioPathInfo");
#endif

	gsGlobalInfo.sDioPathInfo.abPhysPort[0]	= MCDRV_PHYSPORT_DIO0;
	gsGlobalInfo.sDioPathInfo.abPhysPort[1]	= MCDRV_PHYSPORT_DIO1;
	gsGlobalInfo.sDioPathInfo.abPhysPort[2]	= MCDRV_PHYSPORT_DIO2;
	gsGlobalInfo.sDioPathInfo.abPhysPort[3]	= MCDRV_PHYSPORT_NONE;
	gsGlobalInfo.sDioPathInfo.bMusicCh	= MCDRV_MUSIC_2CH;
	gsGlobalInfo.sDioPathInfo.abMusicRSlot[0]	= 0;
	gsGlobalInfo.sDioPathInfo.abMusicRSlot[1]	= 1;
	gsGlobalInfo.sDioPathInfo.abMusicRSlot[2]	= 2;
	gsGlobalInfo.sDioPathInfo.abMusicTSlot[0]	= 0;
	gsGlobalInfo.sDioPathInfo.abMusicTSlot[1]	= 1;
	gsGlobalInfo.sDioPathInfo.abMusicTSlot[2]	= 2;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitDioPathInfo", 0);
#endif
}

/****************************************************************************
 *	InitSwap
 *
 *	Description:
 *			Initialize Swap info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitSwap(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitSwap");
#endif

	gsGlobalInfo.sSwapInfo.bAdif0		= MCDRV_SWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bAdif1		= MCDRV_SWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bAdif2		= MCDRV_SWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bDac0		= MCDRV_SWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bDac1		= MCDRV_SWAP_NORMAL;

	gsGlobalInfo.sSwapInfo.bMusicIn0	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bMusicIn1	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bMusicIn2	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bExtIn		= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bVoiceIn		= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bHifiIn		= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bMusicOut0	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bMusicOut1	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bMusicOut2	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bExtOut		= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bVoiceOut	= MCDRV_SWSWAP_NORMAL;
	gsGlobalInfo.sSwapInfo.bHifiOut		= MCDRV_SWSWAP_NORMAL;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitSwap", 0);
#endif
}

/****************************************************************************
 *	InitHSDet
 *
 *	Description:
 *			Initialize Headset Det info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitHSDet(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitHSDet");
#endif

	gsGlobalInfo.sHSDetInfo.bEnPlugDet	= MCDRV_PLUGDET_DISABLE;
	gsGlobalInfo.sHSDetInfo.bEnPlugDetDb	= MCDRV_PLUGDETDB_DISABLE;
	gsGlobalInfo.sHSDetInfo.bEnDlyKeyOff	= MCDRV_KEYEN_D_D_D;
	gsGlobalInfo.sHSDetInfo.bEnDlyKeyOn	= MCDRV_KEYEN_D_D_D;
	gsGlobalInfo.sHSDetInfo.bEnMicDet	= MCDRV_MICDET_DISABLE;
	gsGlobalInfo.sHSDetInfo.bEnKeyOff	= MCDRV_KEYEN_D_D_D;
	gsGlobalInfo.sHSDetInfo.bEnKeyOn	= MCDRV_KEYEN_D_D_D;
	gsGlobalInfo.sHSDetInfo.bHsDetDbnc	= MCDRV_DETDBNC_875;
	gsGlobalInfo.sHSDetInfo.bKeyOffMtim	= MCDRV_KEYOFF_MTIM_63;
	gsGlobalInfo.sHSDetInfo.bKeyOnMtim	= MCDRV_KEYON_MTIM_63;
	gsGlobalInfo.sHSDetInfo.bKey0OffDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey1OffDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey2OffDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey0OnDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey1OnDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey2OnDlyTim	= 0;
	gsGlobalInfo.sHSDetInfo.bKey0OnDlyTim2	= 0;
	gsGlobalInfo.sHSDetInfo.bKey1OnDlyTim2	= 0;
	gsGlobalInfo.sHSDetInfo.bKey2OnDlyTim2	= 0;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H)
		gsGlobalInfo.sHSDetInfo.bIrqType	= MCDRV_IRQTYPE_NORMAL;
	else
		gsGlobalInfo.sHSDetInfo.bIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDetInfo.bDetInInv	= MCDRV_DET_IN_INV;
	gsGlobalInfo.sHSDetInfo.bHsDetMode	= MCDRV_HSDET_MODE_DETIN_A;
	gsGlobalInfo.sHSDetInfo.bSperiod	= MCDRV_SPERIOD_3906;
	gsGlobalInfo.sHSDetInfo.bLperiod	= MCDRV_LPERIOD_125000;
	gsGlobalInfo.sHSDetInfo.bDbncNumPlug	= MCDRV_DBNC_NUM_7;
	gsGlobalInfo.sHSDetInfo.bDbncNumMic	= MCDRV_DBNC_NUM_7;
	gsGlobalInfo.sHSDetInfo.bDbncNumKey	= MCDRV_DBNC_NUM_7;
	gsGlobalInfo.sHSDetInfo.bSgnlPeriod	= MCDRV_SGNLPERIOD_97;
	gsGlobalInfo.sHSDetInfo.bSgnlNum	= MCDRV_SGNLNUM_8;
	gsGlobalInfo.sHSDetInfo.bSgnlPeak	= MCDRV_SGNLPEAK_1182;
	gsGlobalInfo.sHSDetInfo.bImpSel		= MCDRV_IMPSEL_MOSTFREQ;
	gsGlobalInfo.sHSDetInfo.bDlyIrqStop	= MCDRV_DLYIRQ_DONTCARE;
	gsGlobalInfo.sHSDetInfo.cbfunc		= NULL;

	gsGlobalInfo.sHSDet2Info.bPlugDetDbIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bPlugUndetDbIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bMicDetIrqType		= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bPlugDetIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey0OnIrqType		= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey1OnIrqType		= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey2OnIrqType		= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey0OffIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey1OffIrqType	= MCDRV_IRQTYPE_REF;
	gsGlobalInfo.sHSDet2Info.bKey2OffIrqType	= MCDRV_IRQTYPE_REF;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitHSDet", 0);
#endif
}


/****************************************************************************
 *	InitGpMode
 *
 *	Description:
 *			Initialize Gp mode.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitGpMode(
	void
)
{
	UINT8	bGpioIdx;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitGpMode");
#endif

	for (bGpioIdx = 0; bGpioIdx < 3; bGpioIdx++) {
		gsGlobalInfo.sGpMode.abGpDdr[bGpioIdx]
			= MCDRV_GPDDR_IN;
		gsGlobalInfo.sGpMode.abGpHost[bGpioIdx]
			= MCDRV_GPHOST_CPU;
		gsGlobalInfo.sGpMode.abGpInvert[bGpioIdx]
			= MCDRV_GPINV_NORMAL;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitGpMode", 0);
#endif
}

/****************************************************************************
 *	InitGpMask
 *
 *	Description:
 *			Initialize Gp mask.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitGpMask(
	void
)
{
	UINT8	bGpioIdx;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitGpMask");
#endif

	for (bGpioIdx = 0; bGpioIdx < 3; bGpioIdx++)
		gsGlobalInfo.abGpMask[bGpioIdx]	= MCDRV_GPMASK_ON;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitGpMask", 0);
#endif
}

/****************************************************************************
 *	InitAecInfo
 *
 *	Description:
 *			Initialize Aec Info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitAecInfo(
	void
)
{
	UINT8	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitAecInfo");
#endif

	gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate			= 0;

	gsGlobalInfo.sAecInfo.sAecAudioengine.bEnable			= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bAEOnOff			= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bFDspOnOff		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bBDspAE0Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bBDspAE1Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn0Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn1Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn2Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn3Src		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecBDsp.pbChunkData	= NULL;
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecBDsp.dwSize		= 0;
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecFDsp.pbChunkData	= NULL;
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecFDsp.dwSize		= 0;

	gsGlobalInfo.sAecInfo.sAecVBox.bEnable				= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bFdsp_Po_Source			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bISrc2_VSource			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bISrc2_Ch1_VSource		= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bISrc3_VSource			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_VSource			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_Mix_VolI			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_Mix_VolO			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Ctrl			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bSrc2_Fs				= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bSrc2_Thru			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Fs				= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Thru			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspA.pbChunkData		= NULL;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspA.dwSize			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspB.pbChunkData		= NULL;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspB.dwSize			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecFDsp.pbChunkData		= NULL;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecFDsp.dwSize			= 0;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn		= 0;

	gsGlobalInfo.sAecInfo.sOutput.bLpf_Pre_Thru[0]			= 1;
	gsGlobalInfo.sAecInfo.sOutput.bLpf_Post_Thru[0]			= 1;
	gsGlobalInfo.sAecInfo.sOutput.bLpf_Pre_Thru[1]			= 0;
	gsGlobalInfo.sAecInfo.sOutput.bLpf_Post_Thru[1]			= 0;
	for (i = 0; i < MCDRV_AEC_OUTPUT_N; i++) {
		gsGlobalInfo.sAecInfo.sOutput.bDcc_Sel[i]		= 2;
		gsGlobalInfo.sAecInfo.sOutput.bPow_Det_Lvl[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bOsf_Sel[i]		= 3;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Enb[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][0]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][1]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][2]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][0]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][1]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][2]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][0]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][1]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][2]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][0]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][1]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][2]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][0]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][1]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][2]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bClip_Md[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bClip_Att[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bClip_Rel[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bClip_G[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDcl_OnOff[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDcl_Gain[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDcl_Limit[i][0]		= 0x7F;
		gsGlobalInfo.sAecInfo.sOutput.bDcl_Limit[i][1]		= 0xFF;
		gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_OnOff[i]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_Level[i]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_POS[i]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDc_Dither_OnOff[i]	= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDc_Dither_Level[i]	= 3;
		gsGlobalInfo.sAecInfo.sOutput.bDither_Type[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDng_On[i]		= 0;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Fw[i]		= 1;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[2]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[0]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[1]
									= 0;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[2]
									= 0;
	}
	gsGlobalInfo.sAecInfo.sOutput.bDng_Zero[0]			= 31;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Zero[1]			= 9;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Time[0]			= 0;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Time[1]			= 2;
	gsGlobalInfo.sAecInfo.sOutput.bSig_Det_Lvl			= 0;
	gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[0][0]			=
							MCI_OSF_GAIN0_15_8_DEF;
	gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[0][1]			=
							MCI_OSF_GAIN0_7_0_DEF;
	gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[1][0]			=
							MCI_OSF_GAIN1_15_8_DEF;
	gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[1][1]			=
							MCI_OSF_GAIN1_7_0_DEF;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Attack			= 1;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Release			= 3;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Target[0]			= 0x84;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target[1]
							= MCI_DNG_SP_DEF_ES1;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_LineOut[0]
							= MCI_DNG_LO1_DEF_ES1;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_LineOut[1]
							= MCI_DNG_LO2_DEF_ES1;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_Rc
							= MCI_DNG_RC_DEF_ES1;
	} else {
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target[1]
							= MCI_DNG_SP_DEF;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_LineOut[0]
							= MCI_DNG_LO1_DEF;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_LineOut[1]
							= MCI_DNG_LO2_DEF;
		gsGlobalInfo.sAecInfo.sOutput.bDng_Target_Rc
							= MCI_DNG_RC_DEF;
	}

	for (i = 0; i < MCDRV_AEC_INPUT_N; i++) {
		gsGlobalInfo.sAecInfo.sInput.bDsf32_L_Type[i]		= 1;
		gsGlobalInfo.sAecInfo.sInput.bDsf32_R_Type[i]		= 1;
		gsGlobalInfo.sAecInfo.sInput.bDsf4_Sel[i]		= 1;
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			;
			gsGlobalInfo.sAecInfo.sInput.bDcc_Sel[i]	= 2;
		} else {
			gsGlobalInfo.sAecInfo.sInput.bDcc_Sel[i]	= 1;
		}
		gsGlobalInfo.sAecInfo.sInput.bDng_On[i]			= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Att[i]		= 3;
		gsGlobalInfo.sAecInfo.sInput.bDng_Rel[i]		= 2;
		gsGlobalInfo.sAecInfo.sInput.bDng_Fw[i]			= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Tim[i]		= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Zero[i][0]		= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Zero[i][1]		= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Tgt[i][0]		= 0;
		gsGlobalInfo.sAecInfo.sInput.bDng_Tgt[i][1]		= 0;
		gsGlobalInfo.sAecInfo.sInput.bDepop_Att[i]		= 2;
		gsGlobalInfo.sAecInfo.sInput.bDepop_Wait[i]		= 2;
	}
	gsGlobalInfo.sAecInfo.sInput.bRef_Sel				= 0;

	gsGlobalInfo.sAecInfo.sPdm.bMode				= 0;
	gsGlobalInfo.sAecInfo.sPdm.bStWait				= 2;
	gsGlobalInfo.sAecInfo.sPdm.bPdm0_LoadTim			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm0_LFineDly			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm0_RFineDly			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm1_LoadTim			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm1_LFineDly			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm1_RFineDly			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm0_Data_Delay			= 0;
	gsGlobalInfo.sAecInfo.sPdm.bPdm1_Data_Delay			= 0;

	gsGlobalInfo.sAecInfo.sE2.bEnable				= 0;
	gsGlobalInfo.sAecInfo.sE2.bE2_Da_Sel				= 0;
	gsGlobalInfo.sAecInfo.sE2.bE2_Ad_Sel				= 0;
	gsGlobalInfo.sAecInfo.sE2.bE2OnOff				= 0;
	gsGlobalInfo.sAecInfo.sE2.sE2Config.pbChunkData			= NULL;
	gsGlobalInfo.sAecInfo.sE2.sE2Config.dwSize			= 0;

	gsGlobalInfo.sAecInfo.sAdj.bHold				= 24;
	gsGlobalInfo.sAecInfo.sAdj.bCnt					= 10;
	gsGlobalInfo.sAecInfo.sAdj.bMax[0]				= 2;
	gsGlobalInfo.sAecInfo.sAdj.bMax[1]				= 0;

	gsGlobalInfo.sAecInfo.sEDspMisc.bI2SOut_Enb			= 0;
	gsGlobalInfo.sAecInfo.sEDspMisc.bChSel				= 0;
	gsGlobalInfo.sAecInfo.sEDspMisc.bLoopBack			= 0;

	gsGlobalInfo.sAecInfo.sControl.bCommand				= 0;
	gsGlobalInfo.sAecInfo.sControl.bParam[0]			= 0;
	gsGlobalInfo.sAecInfo.sControl.bParam[1]			= 0;
	gsGlobalInfo.sAecInfo.sControl.bParam[2]			= 0;
	gsGlobalInfo.sAecInfo.sControl.bParam[3]			= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitAecInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_UpdateState
 *
 *	Description:
 *			update state.
 *	Arguments:
 *			eState	state
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_UpdateState(
	enum MCDRV_STATE	eState
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_UpdateState");
#endif

	geState = eState;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_UpdateState", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetState
 *
 *	Description:
 *			Get state.
 *	Arguments:
 *			none
 *	Return:
 *			current state
 *
 ****************************************************************************/
enum MCDRV_STATE	McResCtrl_GetState(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= geState;
	McDebugLog_FuncIn("McResCtrl_GetState");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetState", &sdRet);
#endif

	return geState;
}

/****************************************************************************
 *	McResCtrl_GetRegVal
 *
 *	Description:
 *			Get register value.
 *	Arguments:
 *			wRegType	register type
 *			wRegAddr	address
 *	Return:
 *			register value
 *
 ****************************************************************************/
UINT8	McResCtrl_GetRegVal(
	UINT16	wRegType,
	UINT16	wRegAddr
)
{
	UINT8	bVal	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetRegVal");
#endif

	switch (wRegType) {
	case	MCDRV_PACKET_REGTYPE_IF:
		bVal	= gsGlobalInfo.abRegValIF[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_A:
		bVal	= gsGlobalInfo.abRegValA[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_MA:
		bVal	= gsGlobalInfo.abRegValMA[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_MB:
		bVal	= gsGlobalInfo.abRegValMB[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_E:
		bVal	= gsGlobalInfo.abRegValE[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_ANA:
		bVal	= gsGlobalInfo.abRegValANA[wRegAddr];
		break;
	case	MCDRV_PACKET_REGTYPE_CD:
		bVal	= gsGlobalInfo.abRegValCD[wRegAddr];
		break;

	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bVal;
	McDebugLog_FuncOut("McResCtrl_GetRegVal", &sdRet);
#endif
	return bVal;
}

/****************************************************************************
 *	McResCtrl_SetRegVal
 *
 *	Description:
 *			Set register value.
 *	Arguments:
 *			wRegType	register type
 *			wRegAddr	address
 *			bRegVal		register value
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetRegVal(
	UINT16	wRegType,
	UINT16	wRegAddr,
	UINT8	bRegVal
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetRegVal");
#endif

	switch (wRegType) {
	case	MCDRV_PACKET_REGTYPE_IF:
		gsGlobalInfo.abRegValIF[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_A:
		gsGlobalInfo.abRegValA[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_MA:
		gsGlobalInfo.abRegValMA[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_MB:
		gsGlobalInfo.abRegValMB[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_E:
		gsGlobalInfo.abRegValE[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_ANA:
		gsGlobalInfo.abRegValANA[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_CD:
		gsGlobalInfo.abRegValCD[wRegAddr]	= bRegVal;
		break;
	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetRegVal", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetInitInfo
 *
 *	Description:
 *			Set Initialize information.
 *	Arguments:
 *			psInitInfo	Initialize information
 *			psInit2Info	Initialize information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetInitInfo(
	const struct MCDRV_INIT_INFO	*psInitInfo,
	const struct MCDRV_INIT2_INFO	*psInit2Info
)
{
	int	i;
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetInitInfo");
#endif

	gsGlobalInfo.sInitInfo	= *psInitInfo;
	if (psInit2Info != NULL) {
		gsGlobalInfo.sInit2Info	= *psInit2Info;
	} else {
		gsGlobalInfo.sInit2Info.bOption[0]	= MCDRV_DOA_DRV_HIGH;
		gsGlobalInfo.sInit2Info.bOption[1]	= MCDRV_SCKMSK_OFF;
		gsGlobalInfo.sInit2Info.bOption[2]	= MCDRV_SPMN_8_9_10;
		for (i = 3;
		i < GET_ARRAY_SIZE(gsGlobalInfo.sInit2Info.bOption); i++) {
			;
			gsGlobalInfo.sInit2Info.bOption[i]	= 0;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetInitInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetInitInfo
 *
 *	Description:
 *			Get Initialize information.
 *	Arguments:
 *			psInitInfo	Initialize information
 *			psInit2Info	Initialize information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetInitInfo(
	struct MCDRV_INIT_INFO	*psInitInfo,
	struct MCDRV_INIT2_INFO	*psInit2Info
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetInitInfo");
#endif

	*psInitInfo	= gsGlobalInfo.sInitInfo;
	if (psInit2Info != NULL) {
		;
		*psInit2Info	= gsGlobalInfo.sInit2Info;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetInitInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetClockSwInfo
 *
 *	Description:
 *			Set switch clock info.
 *	Arguments:
 *			psClockSwInfo	pointer to MCDRV_CLOCKSW_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetClockSwInfo(
	const struct MCDRV_CLOCKSW_INFO	*psClockSwInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetClockSw");
#endif

	gsGlobalInfo.sClockSwInfo	= *psClockSwInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetClockSw", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetClockSwInfo
 *
 *	Description:
 *			Get switch clock info.
 *	Arguments:
 *			psClockSwInfo	pointer to MCDRV_CLOCKSW_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetClockSwInfo(
	struct MCDRV_CLOCKSW_INFO	*psClockSwInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetClockSw");
#endif

	*psClockSwInfo	= gsGlobalInfo.sClockSwInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetClockSw", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetPathInfo
 *
 *	Description:
 *			Set path information.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32	McResCtrl_SetPathInfo(
	const struct MCDRV_PATH_INFO	*psPathInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	struct MCDRV_PATH_INFO	sCurPathInfo	= gsGlobalInfo.sPathInfo;
	int	i;
	UINT8	bDone;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetPathInfo");
#endif


	gsGlobalInfo.sPathInfo	= gsGlobalInfo.sPathInfoVirtual;

	/*	MusicOut source on/off	*/
	SetD1SourceOnOff(psPathInfo->asMusicOut,
			gsGlobalInfo.sPathInfo.asMusicOut,
			MUSICOUT_PATH_CHANNELS);
	/*	ExtOut source on/off	*/
	SetD1SourceOnOff(psPathInfo->asExtOut,
			gsGlobalInfo.sPathInfo.asExtOut,
			EXTOUT_PATH_CHANNELS);
	/*	HifiOut source on/off	*/
	SetD1SourceOnOff(psPathInfo->asHifiOut,
			gsGlobalInfo.sPathInfo.asHifiOut,
			HIFIOUT_PATH_CHANNELS);
	/*	V-Box source on/off	*/
	SetD1SourceOnOff(psPathInfo->asVboxMixIn,
			gsGlobalInfo.sPathInfo.asVboxMixIn,
			VBOXMIXIN_PATH_CHANNELS);
	/*	AE source on/off	*/
	SetD1SourceOnOff(psPathInfo->asAe0,
			gsGlobalInfo.sPathInfo.asAe0,
			AE_PATH_CHANNELS);
	SetD1SourceOnOff(psPathInfo->asAe1,
			gsGlobalInfo.sPathInfo.asAe1,
			AE_PATH_CHANNELS);
	SetD1SourceOnOff(psPathInfo->asAe2,
			gsGlobalInfo.sPathInfo.asAe2,
			AE_PATH_CHANNELS);
	SetD1SourceOnOff(psPathInfo->asAe3,
			gsGlobalInfo.sPathInfo.asAe3,
			AE_PATH_CHANNELS);
	/*	DAC source on/off	*/
	SetD1SourceOnOff(psPathInfo->asDac0,
			gsGlobalInfo.sPathInfo.asDac0,
			DAC0_PATH_CHANNELS);
	SetD1SourceOnOff(psPathInfo->asDac1,
			gsGlobalInfo.sPathInfo.asDac1,
			DAC1_PATH_CHANNELS);
	/*	VoiceOut source on/off	*/
	SetD2SourceOnOff(psPathInfo->asVoiceOut,
			gsGlobalInfo.sPathInfo.asVoiceOut,
			VOICEOUT_PATH_CHANNELS);
	/*	VboxIoIn source on/off	*/
	SetD2SourceOnOff(psPathInfo->asVboxIoIn,
			gsGlobalInfo.sPathInfo.asVboxIoIn,
			VBOXIOIN_PATH_CHANNELS);
	/*	VboxHostIn source on/off	*/
	SetD2SourceOnOff(psPathInfo->asVboxHostIn,
			gsGlobalInfo.sPathInfo.asVboxHostIn,
			VBOXHOSTIN_PATH_CHANNELS);
	/*	HostOut source on/off	*/
	SetD2SourceOnOff(psPathInfo->asHostOut,
			gsGlobalInfo.sPathInfo.asHostOut,
			HOSTOUT_PATH_CHANNELS);
	/*	Adif source on/off	*/
	SetD2SourceOnOff(psPathInfo->asAdif0,
			gsGlobalInfo.sPathInfo.asAdif0,
			ADIF0_PATH_CHANNELS);
	SetD2SourceOnOff(psPathInfo->asAdif1,
			gsGlobalInfo.sPathInfo.asAdif1,
			ADIF1_PATH_CHANNELS);
	SetD2SourceOnOff(psPathInfo->asAdif2,
			gsGlobalInfo.sPathInfo.asAdif2,
			ADIF2_PATH_CHANNELS);

	/*	ADC0 source on/off	*/
	SetASourceOnOff(psPathInfo->asAdc0,
			gsGlobalInfo.sPathInfo.asAdc0,
			ADC0_PATH_CHANNELS);
	/*	ADC1 source on/off	*/
	SetASourceOnOff(psPathInfo->asAdc1,
			gsGlobalInfo.sPathInfo.asAdc1,
			ADC1_PATH_CHANNELS);
	/*	SP source on/off	*/
	SetASourceOnOff(psPathInfo->asSp,
			gsGlobalInfo.sPathInfo.asSp,
			SP_PATH_CHANNELS);
	/*	HP source on/off	*/
	SetASourceOnOff(psPathInfo->asHp,
			gsGlobalInfo.sPathInfo.asHp,
			HP_PATH_CHANNELS);
	/*	RCV source on/off	*/
	SetASourceOnOff(psPathInfo->asRc,
			gsGlobalInfo.sPathInfo.asRc,
			RC_PATH_CHANNELS);
	/*	LOut1 source on/off	*/
	SetASourceOnOff(psPathInfo->asLout1,
			gsGlobalInfo.sPathInfo.asLout1,
			LOUT1_PATH_CHANNELS);
	/*	LOut2 source on/off	*/
	SetASourceOnOff(psPathInfo->asLout2,
			gsGlobalInfo.sPathInfo.asLout2,
			LOUT2_PATH_CHANNELS);
	/*	Bias source on/off	*/
	SetBiasSourceOnOff(psPathInfo);

	sdRet	= IsValidPath();
	if (sdRet != MCDRV_SUCCESS) {
		gsGlobalInfo.sPathInfo	= sCurPathInfo;
	} else {
		sdRet	= CheckLpFp();
		if (sdRet != MCDRV_SUCCESS) {
			gsGlobalInfo.sPathInfo	= sCurPathInfo;
		} else {
			gsGlobalInfo.sPathInfoVirtual
				= gsGlobalInfo.sPathInfo;
			for (i = 0, bDone = 1; i < 3 && bDone != 0; i++) {
				bDone	= ValidateADC();
				bDone	|= ValidateDAC();
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetPathInfo", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	CheckLpFp
 *
 *	Description:
 *			Check port.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckLpFp(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	bFP[4];
	int	i, j;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("CheckLpFp");
#endif

	bFP[0]	=
	bFP[1]	=
	bFP[2]	=
	bFP[3]	= MCDRV_PHYSPORT_NONE;

	if ((McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) != 0)) {
		bFP[0]	= gsGlobalInfo.sDioPathInfo.abPhysPort[0];
		if (bFP[0] == MCDRV_PHYSPORT_NONE) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) != 0)) {
		bFP[1]	= gsGlobalInfo.sDioPathInfo.abPhysPort[1];
		if (bFP[1] == MCDRV_PHYSPORT_NONE) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXIOIN, eMCDRV_DST_CH0) != 0)) {
		bFP[2]	= gsGlobalInfo.sDioPathInfo.abPhysPort[2];
		if (bFP[2] == MCDRV_PHYSPORT_NONE) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) != 0)) {
		bFP[3]	= gsGlobalInfo.sDioPathInfo.abPhysPort[3];
		if (bFP[3] == MCDRV_PHYSPORT_NONE) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}

	for (i = 0; i < 4 && sdRet == MCDRV_SUCCESS; i++) {
		if (bFP[i] == MCDRV_PHYSPORT_NONE)
			continue;
		for (j = i+1; j < 4 && sdRet == MCDRV_SUCCESS; j++) {
			if (bFP[i] == bFP[j])
				sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("CheckLpFp", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	IsValidPath
 *
 *	Description:
 *			Check path is valid.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	IsValidPath(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dDacSrc[2];
	UINT8	bCh;
	UINT32	dSrc;
	UINT32	dAdifSrc[2];
	UINT8	bUsed;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("IsValidPath");
#endif

	dDacSrc[0]	= gsGlobalInfo.sPathInfo.asDac0[0].dSrcOnOff;
	dDacSrc[1]	= gsGlobalInfo.sPathInfo.asDac0[1].dSrcOnOff;

	if (((dDacSrc[0] & MCDRV_D1SRC_HIFIIN_ON) != 0)
	|| ((dDacSrc[1] & MCDRV_D1SRC_HIFIIN_ON) != 0)) {
		if (((dDacSrc[0] & MCDRV_D1SRC_MUSICIN_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_EXTIN_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE0_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE1_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE2_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE3_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF0_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF1_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF2_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_MUSICIN_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_EXTIN_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE0_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE1_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE2_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE3_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF0_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF1_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF2_ON) != 0)) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++) {
				dSrc	=
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
			}
			for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++) {
				dSrc	=
				gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
			}
			for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++) {
				dSrc	=
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
			}
			for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
				dSrc	=
				gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
				dSrc	=
				gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
				dSrc	=
				gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
				dSrc	=
				gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
			}
			for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++) {
				dSrc	=
				gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff;
				if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					goto exit;
				}
			}
		}
	}

	dDacSrc[0]	= gsGlobalInfo.sPathInfo.asDac1[0].dSrcOnOff;
	dDacSrc[1]	= gsGlobalInfo.sPathInfo.asDac1[1].dSrcOnOff;
	if (((dDacSrc[0] & MCDRV_D1SRC_HIFIIN_ON) != 0)
	|| ((dDacSrc[1] & MCDRV_D1SRC_HIFIIN_ON) != 0)) {
		if (((dDacSrc[0] & MCDRV_D1SRC_MUSICIN_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_EXTIN_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE0_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE1_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE2_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_AE3_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF0_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF1_ON) != 0)
		|| ((dDacSrc[0] & MCDRV_D1SRC_ADIF2_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_MUSICIN_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_EXTIN_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE0_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE1_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE2_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_AE3_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF0_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF1_ON) != 0)
		|| ((dDacSrc[1] & MCDRV_D1SRC_ADIF2_ON) != 0)) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}

	for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++) {
		bUsed	= 0;
		dAdifSrc[bCh]	= gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC1_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_R_ON) != 0)
			bUsed++;
		if (bUsed > 1) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}
	if ((dAdifSrc[0] == D2SRC_ALL_OFF)
	&& (dAdifSrc[1] != D2SRC_ALL_OFF)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++) {
		bUsed	= 0;
		dAdifSrc[bCh]	= gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC1_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_R_ON) != 0)
			bUsed++;
		if (bUsed > 1) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}
	if ((dAdifSrc[0] == D2SRC_ALL_OFF)
	&& (dAdifSrc[1] != D2SRC_ALL_OFF)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
		bUsed	= 0;
		if (bCh == 0)
			dAdifSrc[bCh]	=
			McResCtrl_GetSource(eMCDRV_DST_ADIF2,
						eMCDRV_DST_CH0);
		else
			dAdifSrc[bCh]	=
			McResCtrl_GetSource(eMCDRV_DST_ADIF2,
						eMCDRV_DST_CH1);
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_ADC1_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM0_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_L_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_PDM1_R_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_DAC0REF_ON) != 0)
			bUsed++;
		if ((dAdifSrc[bCh] & MCDRV_D2SRC_DAC1REF_ON) != 0)
			bUsed++;
		if (bUsed > 1) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}
	if (((dAdifSrc[0] & ~D2SRC_ALL_OFF) == 0)
	&& ((dAdifSrc[1] & ~D2SRC_ALL_OFF) != 0)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}
	if (((dAdifSrc[0] & MCDRV_D2SRC_DAC0REF_ON) != 0)
	&& ((dAdifSrc[1] & ~MCDRV_D2SRC_DAC0REF_ON) != 0)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}
	if (((dAdifSrc[1] & MCDRV_D2SRC_DAC0REF_ON) != 0)
	&& ((dAdifSrc[0] & ~MCDRV_D2SRC_DAC0REF_ON) != 0)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}
	if (((dAdifSrc[0] & MCDRV_D2SRC_DAC1REF_ON) != 0)
	&& ((dAdifSrc[1] & ~MCDRV_D2SRC_DAC1REF_ON) != 0)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}
	if (((dAdifSrc[1] & MCDRV_D2SRC_DAC1REF_ON) != 0)
	&& ((dAdifSrc[0] & ~MCDRV_D2SRC_DAC1REF_ON) != 0)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	for (bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++) {
		bUsed	= 0;
		dSrc	= gsGlobalInfo.sPathInfo.asAdc0[bCh].dSrcOnOff;
		if ((dSrc & MCDRV_ASRC_LINEIN1_L_ON) != 0)
			bUsed++;
		if ((dSrc & MCDRV_ASRC_LINEIN1_R_ON) != 0)
			bUsed++;
		if ((dSrc & MCDRV_ASRC_LINEIN1_M_ON) != 0)
			bUsed++;
		if (bUsed > 1) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			goto exit;
		}
	}
exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("IsValidPath", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	ValidateADC
 *
 *	Description:
 *			Validate ADC setting.
 *	Arguments:
 *			none
 *	Return:
 *			0:not changed, 1:changed
 *
 ****************************************************************************/
static UINT8	ValidateADC(
	void
)
{
	UINT8	bRet	= 0;
	UINT8	bCh;
	UINT8	bHasSrc	= 0;
	UINT32	dHifiSrc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("ValidateADC");
#endif

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) != 0))
		bHasSrc	= 1;
	else
		bHasSrc	= 0;

	dHifiSrc	= gsGlobalInfo.sPathInfo.asHifiOut[0].dSrcOnOff;

	if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF0_ON) == 0)
	&& ((dHifiSrc & MCDRV_D1SRC_ADIF0_ON) == 0)) {
		;
	} else if (bHasSrc == 0) {
		for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < DAC0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF0_ON;
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF0_OFF;
		}
		bRet	= 1;
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) != 0))
		bHasSrc	= 1;
	else
		bHasSrc	= 0;

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF1_ON) == 0) {
		;
	} else if (bHasSrc == 0) {
		for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < DAC0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF1_ON;
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF1_OFF;
		}
		bRet	= 1;
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1) != 0))
		bHasSrc	= 1;
	else
		bHasSrc	= 0;

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF2_ON) == 0) {
		;
	} else if (bHasSrc == 0) {
		for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asHifiOut[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < DAC0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				&= ~MCDRV_D1SRC_ADIF2_ON;
			gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
				|= MCDRV_D1SRC_ADIF2_OFF;
		}
		bRet	= 1;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH0) != 0)
		bHasSrc	= 1;
	else
		bHasSrc	= 0;
	if (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_L_ON) == 0) {
		if (bHasSrc != 0) {
			gsGlobalInfo.sPathInfo.asAdc0[0].dSrcOnOff
					= D2SRC_ALL_OFF;
			bRet	= 1;
		}
	} else if (bHasSrc == 0) {
		for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_L_ON;
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_L_OFF;
		}
		for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_L_ON;
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_L_OFF;
		}
		for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_L_ON;
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_L_OFF;
		}
		bRet	= 1;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH1) != 0)
		bHasSrc	= 1;
	else
		bHasSrc	= 0;
	if (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_R_ON) == 0) {
		if (bHasSrc != 0) {
			gsGlobalInfo.sPathInfo.asAdc0[1].dSrcOnOff
					= D2SRC_ALL_OFF;
			bRet	= 1;
		}
	} else if (bHasSrc == 0) {
		for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_R_ON;
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_R_OFF;
		}
		for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_R_ON;
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_R_OFF;
		}
		for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC0_R_ON;
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC0_R_OFF;
		}
		bRet	= 1;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_ADC1, eMCDRV_DST_CH0) != 0)
		bHasSrc	= 1;
	else
		bHasSrc	= 0;
	if (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC1_ON) == 0) {
		if (bHasSrc != 0) {
			for (bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
				gsGlobalInfo.sPathInfo.asAdc1[bCh].dSrcOnOff
					= D2SRC_ALL_OFF;
			bRet	= 1;
		}
	} else if (bHasSrc == 0) {
		bRet	= 1;
		for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC1_ON;
			gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC1_OFF;
		}
		for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC1_ON;
			gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC1_OFF;
		}
		for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				&= ~MCDRV_D2SRC_ADC1_ON;
			gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				|= MCDRV_D2SRC_ADC1_OFF;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("ValidateADC", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	ValidateDAC
 *
 *	Description:
 *			Validate DAC setting.
 *	Arguments:
 *			none
 *	Return:
 *			0:not changed, 1:changed
 *
 ****************************************************************************/
static UINT8	ValidateDAC(
	void
)
{
	UINT8	bRet	= 0;
	UINT8	bCh;
	UINT8	bHasSrc	= 0;
	UINT8	bUsed	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("ValidateDAC");
#endif

	if ((McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) != 0))
		bHasSrc	= 1;
	else
		bHasSrc	= 0;

	if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_R_ON) != 0))
		bUsed	= 1;
	else {
		bUsed	= 0;
		for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
			if ((gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				& MCDRV_D2SRC_DAC1REF_ON) != 0) {
				bUsed	= 1;
			}
		}
	}

	if (bHasSrc == 0) {
		if (bUsed == 1) {
			bRet	= 1;
			for (bCh = 0; bCh < SP_PATH_CHANNELS; bCh++) {
				gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff
					&= ~MCDRV_ASRC_DAC1_L_ON;
				gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff
					|= MCDRV_ASRC_DAC1_L_OFF;
				gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff
					&= ~MCDRV_ASRC_DAC1_R_ON;
				gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff
					|= MCDRV_ASRC_DAC1_R_OFF;
			}
			for (bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++) {
				gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff
					&= ~MCDRV_ASRC_DAC1_L_ON;
				gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff
					|= MCDRV_ASRC_DAC1_L_OFF;
				gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff
					&= ~MCDRV_ASRC_DAC1_R_ON;
				gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff
					|= MCDRV_ASRC_DAC1_R_OFF;
			}
		}
		for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
			if ((gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
				& MCDRV_D2SRC_DAC1REF_ON) != 0) {
				gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
					&= ~MCDRV_D2SRC_DAC1REF_ON;
				bRet	= 1;
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("ValidateDAC", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	SetD1SourceOnOff
 *
 *	Description:
 *			Set digital output source On/Off.
 *	Arguments:
 *			psSetDChannel	set On/Off info
 *			psDstDChannel	destination On/Off setting
 *			bChannels	number of channels
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetD1SourceOnOff(
	const struct MCDRV_D1_CHANNEL	*psSetDChannel,
	struct MCDRV_D1_CHANNEL		*psDstDChannel,
	UINT8	bChannels
)
{
	UINT8	i, bCh;
	UINT32	dOn, dOff;
	struct {
		UINT32	dOn;
		UINT32	dOff;
	} sBlockInfo[]	= {
		{MCDRV_D1SRC_MUSICIN_ON,	MCDRV_D1SRC_MUSICIN_OFF},
		{MCDRV_D1SRC_EXTIN_ON,		MCDRV_D1SRC_EXTIN_OFF},
		{MCDRV_D1SRC_VBOXOUT_ON,	MCDRV_D1SRC_VBOXOUT_OFF},
		{MCDRV_D1SRC_VBOXREFOUT_ON,	MCDRV_D1SRC_VBOXREFOUT_OFF},
		{MCDRV_D1SRC_AE0_ON,		MCDRV_D1SRC_AE0_OFF},
		{MCDRV_D1SRC_AE1_ON,		MCDRV_D1SRC_AE1_OFF},
		{MCDRV_D1SRC_AE2_ON,		MCDRV_D1SRC_AE2_OFF},
		{MCDRV_D1SRC_AE3_ON,		MCDRV_D1SRC_AE3_OFF},
		{MCDRV_D1SRC_ADIF0_ON,		MCDRV_D1SRC_ADIF0_OFF},
		{MCDRV_D1SRC_ADIF1_ON,		MCDRV_D1SRC_ADIF1_OFF},
		{MCDRV_D1SRC_ADIF2_ON,		MCDRV_D1SRC_ADIF2_OFF},
		{MCDRV_D1SRC_HIFIIN_ON,		MCDRV_D1SRC_HIFIIN_OFF},
	};

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetD1SourceOnOff");
#endif

	for (bCh = 0; bCh < bChannels; bCh++) {
		for (i = 0; i < GET_ARRAY_SIZE(sBlockInfo); i++) {
			dOn	= sBlockInfo[i].dOn;
			dOff	= sBlockInfo[i].dOff;
			SetSourceOnOff(
				psSetDChannel[bCh].dSrcOnOff,
				&psDstDChannel[bCh].dSrcOnOff,
				dOn,
				dOff);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetD1SourceOnOff", 0);
#endif
}

/****************************************************************************
 *	SetD2SourceOnOff
 *
 *	Description:
 *			Set digital output source On/Off.
 *	Arguments:
 *			psSetDChannel	set On/Off info
 *			psDstDChannel	destination On/Off setting
 *			bChannels	number of channels
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetD2SourceOnOff(
	const struct MCDRV_D2_CHANNEL	*psSetDChannel,
	struct MCDRV_D2_CHANNEL		*psDstDChannel,
	UINT8	bChannels
)
{
	UINT8	i, bCh;
	UINT32	dOn, dOff;
	struct {
		UINT32	dOn;
		UINT32	dOff;
	} sBlockInfo[]	= {
		{MCDRV_D2SRC_VOICEIN_ON,	MCDRV_D2SRC_VOICEIN_OFF},
		{MCDRV_D2SRC_VBOXIOOUT_ON,	MCDRV_D2SRC_VBOXIOOUT_OFF},
		{MCDRV_D2SRC_VBOXHOSTOUT_ON,	MCDRV_D2SRC_VBOXHOSTOUT_OFF},
		{MCDRV_D2SRC_ADC0_L_ON,		MCDRV_D2SRC_ADC0_L_OFF},
		{MCDRV_D2SRC_ADC0_R_ON,		MCDRV_D2SRC_ADC0_R_OFF},
		{MCDRV_D2SRC_ADC1_ON,		MCDRV_D2SRC_ADC1_OFF},
		{MCDRV_D2SRC_PDM0_L_ON,		MCDRV_D2SRC_PDM0_L_OFF},
		{MCDRV_D2SRC_PDM0_R_ON,		MCDRV_D2SRC_PDM0_R_OFF},
		{MCDRV_D2SRC_PDM1_L_ON,		MCDRV_D2SRC_PDM1_L_OFF},
		{MCDRV_D2SRC_PDM1_R_ON,		MCDRV_D2SRC_PDM1_R_OFF},
		{MCDRV_D2SRC_DAC0REF_ON,	MCDRV_D2SRC_DAC0REF_OFF},
		{MCDRV_D2SRC_DAC1REF_ON,	MCDRV_D2SRC_DAC1REF_OFF}
	};

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetD2SourceOnOff");
#endif

	for (bCh = 0; bCh < bChannels; bCh++) {
		for (i = 0; i < GET_ARRAY_SIZE(sBlockInfo); i++) {
			dOn	= sBlockInfo[i].dOn;
			dOff	= sBlockInfo[i].dOff;
			if ((psSetDChannel[bCh].dSrcOnOff & dOn) != 0) {
				if ((dOn == MCDRV_D2SRC_PDM0_L_ON)
				|| (dOn == MCDRV_D2SRC_PDM0_R_ON)
				|| (dOn == MCDRV_D2SRC_PDM1_L_ON)
				|| (dOn == MCDRV_D2SRC_PDM1_R_ON)) {
					if (gsGlobalInfo.sInitInfo.bPa0Func
						!= MCDRV_PA_PDMCK) {
						break;
					}
				}
			}
			SetSourceOnOff(
				psSetDChannel[bCh].dSrcOnOff,
				&psDstDChannel[bCh].dSrcOnOff,
				dOn,
				dOff);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetD2SourceOnOff", 0);
#endif
}

/****************************************************************************
 *	SetASourceOnOff
 *
 *	Description:
 *			Set analog output source On/Off.
 *	Arguments:
 *			psSetDChannel	set On/Off info
 *			psDstDChannel	destination On/Off setting
 *			bChannels	number of channels
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetASourceOnOff(
	const struct MCDRV_A_CHANNEL	*psSetAChannel,
	struct MCDRV_A_CHANNEL		*psDstAChannel,
	UINT8	bChannels
)
{
	UINT8	i, bCh;
	struct {
		UINT32	dOn;
		UINT32	dOff;
	} sBlockInfo[]	= {
		{MCDRV_ASRC_DAC0_L_ON,		MCDRV_ASRC_DAC0_L_OFF},
		{MCDRV_ASRC_DAC0_R_ON,		MCDRV_ASRC_DAC0_R_OFF},
		{MCDRV_ASRC_DAC1_L_ON,		MCDRV_ASRC_DAC1_L_OFF},
		{MCDRV_ASRC_DAC1_R_ON,		MCDRV_ASRC_DAC1_R_OFF},
		{MCDRV_ASRC_MIC1_ON,		MCDRV_ASRC_MIC1_OFF},
		{MCDRV_ASRC_MIC2_ON,		MCDRV_ASRC_MIC2_OFF},
		{MCDRV_ASRC_MIC3_ON,		MCDRV_ASRC_MIC3_OFF},
		{MCDRV_ASRC_MIC4_ON,		MCDRV_ASRC_MIC4_OFF},
		{MCDRV_ASRC_LINEIN1_L_ON,	MCDRV_ASRC_LINEIN1_L_OFF},
		{MCDRV_ASRC_LINEIN1_R_ON,	MCDRV_ASRC_LINEIN1_R_OFF},
		{MCDRV_ASRC_LINEIN1_M_ON,	MCDRV_ASRC_LINEIN1_M_OFF},
	};

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetASourceOnOff");
#endif

	for (bCh = 0; bCh < bChannels; bCh++) {
		for (i = 0; i < GET_ARRAY_SIZE(sBlockInfo); i++) {
			SetSourceOnOff(
				psSetAChannel[bCh].dSrcOnOff,
				&psDstAChannel[bCh].dSrcOnOff,
				sBlockInfo[i].dOn,
				sBlockInfo[i].dOff);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetASourceOnOff", 0);
#endif
}

/****************************************************************************
 *	SetBiasSourceOnOff
 *
 *	Description:
 *			Set Bias source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetBiasSourceOnOff(
	const struct MCDRV_PATH_INFO	*psPathInfo
)
{
	UINT8	bBlock, bCh;
	struct {
		UINT8	bBlock;
		UINT32	dOn;
		UINT32	dOff;
	} sBlockInfo[]	= {
		{0, MCDRV_ASRC_MIC1_ON,	MCDRV_ASRC_MIC1_OFF},
		{0, MCDRV_ASRC_MIC2_ON,	MCDRV_ASRC_MIC2_OFF},
		{0, MCDRV_ASRC_MIC3_ON,	MCDRV_ASRC_MIC3_OFF},
		{0, MCDRV_ASRC_MIC4_ON,	MCDRV_ASRC_MIC4_OFF}
	};

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetBiasSourceOnOff");
#endif

	for (bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++) {
		for (bBlock = 0; bBlock < GET_ARRAY_SIZE(sBlockInfo); bBlock++
		) {
			;
			SetSourceOnOff(psPathInfo->asBias[bCh].dSrcOnOff,
				&gsGlobalInfo.sPathInfo.asBias[bCh].dSrcOnOff,
				sBlockInfo[bBlock].dOn,
				sBlockInfo[bBlock].dOff);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetBiasSourceOnOff", 0);
#endif
}

/****************************************************************************
 *	ClearD1SourceOnOff
 *
 *	Description:
 *			Clear digial output source On/Off.
 *	Arguments:
 *			pdSrcOnOff	source On/Off info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	ClearD1SourceOnOff(
	UINT32	*pdSrcOnOff
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("ClearD1SourceOnOff");
#endif

	if (pdSrcOnOff == NULL) {
			;
	} else {
		;
		*pdSrcOnOff	= D1SRC_ALL_OFF;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("ClearD1SourceOnOff", 0);
#endif
}

/****************************************************************************
 *	ClearD2SourceOnOff
 *
 *	Description:
 *			Clear digial output source On/Off.
 *	Arguments:
 *			pdSrcOnOff	source On/Off info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	ClearD2SourceOnOff(
	UINT32	*pdSrcOnOff
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("ClearD2SourceOnOff");
#endif

	if (pdSrcOnOff == NULL) {
			;
	} else {
		;
		*pdSrcOnOff	= D2SRC_ALL_OFF;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("ClearD2SourceOnOff", 0);
#endif
}

/****************************************************************************
 *	ClearASourceOnOff
 *
 *	Description:
 *			Clear analog output source On/Off.
 *	Arguments:
 *			pdSrcOnOff	source On/Off info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	ClearASourceOnOff(
	UINT32	*pdSrcOnOff
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("ClearASourceOnOff");
#endif

	if (pdSrcOnOff == NULL) {
			;
	} else {
		;
		pdSrcOnOff[0]	= MCDRV_ASRC_DAC0_L_OFF
				| MCDRV_ASRC_DAC0_R_OFF
				| MCDRV_ASRC_DAC1_L_OFF
				| MCDRV_ASRC_DAC1_R_OFF
				| MCDRV_ASRC_MIC1_OFF
				| MCDRV_ASRC_MIC2_OFF
				| MCDRV_ASRC_MIC3_OFF
				| MCDRV_ASRC_MIC4_OFF
				| MCDRV_ASRC_LINEIN1_L_OFF
				| MCDRV_ASRC_LINEIN1_R_OFF
				| MCDRV_ASRC_LINEIN1_M_OFF;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("ClearASourceOnOff", 0);
#endif
}

/****************************************************************************
 *	SetSourceOnOff
 *
 *	Description:
 *			Set source On/Off.
 *	Arguments:
 *			pdSrcOnOff	source On/Off info
 *			pdDstOnOff	destination
 *			bBlock		Block
 *			bOn			On bit
 *			bOff		Off bit
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetSourceOnOff(
	UINT32	dSrcOnOff,
	UINT32	*pdDstOnOff,
	UINT32	dOn,
	UINT32	dOff
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetSourceOnOff");
#endif

	if (pdDstOnOff == NULL) {
			;
	} else {
		if ((dSrcOnOff & dOn) != 0) {
			*pdDstOnOff	&= ~dOff;
			*pdDstOnOff	|= dOn;
		} else if ((dSrcOnOff & (dOn|dOff)) == dOff) {
			*pdDstOnOff	&= ~dOn;
			*pdDstOnOff	|= dOff;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetSourceOnOff", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetPathInfo
 *
 *	Description:
 *			Get path information.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPathInfo(
	struct MCDRV_PATH_INFO	*psPathInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetPathInfo");
#endif

	*psPathInfo	= gsGlobalInfo.sPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetPathInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetPathInfoVirtual
 *
 *	Description:
 *			Get virtaul path information.
 *	Arguments:
 *			psPathInfo	virtaul path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPathInfoVirtual(
	struct MCDRV_PATH_INFO	*psPathInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetPathInfoVirtual");
#endif

	*psPathInfo	= gsGlobalInfo.sPathInfoVirtual;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetPathInfoVirtual", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetVolInfo
 *
 *	Description:
 *			Update volume.
 *	Arguments:
 *			psVolInfo		volume setting
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetVolInfo(
	const struct MCDRV_VOL_INFO	*psVolInfo
)
{
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetVolInfo");
#endif


	for (bCh = 0; bCh < MUSICIN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_MusicIn[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_MusicIn[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_MusicIn[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < EXTIN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_ExtIn[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_ExtIn[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_ExtIn[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < VOICEIN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_VoiceIn[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_VoiceIn[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_VoiceIn[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < REFIN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_RefIn[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_RefIn[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_RefIn[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < ADIF0IN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_Adif0In[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_Adif0In[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_Adif0In[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < ADIF1IN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_Adif1In[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_Adif1In[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_Adif1In[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < ADIF2IN_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_Adif2In[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_Adif2In[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_Adif2In[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < MUSICOUT_VOL_CHANNELS ; bCh++) {
		if (((UINT16)psVolInfo->aswD_MusicOut[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_MusicOut[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_MusicOut[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < EXTOUT_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_ExtOut[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_ExtOut[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_ExtOut[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < VOICEOUT_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_VoiceOut[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_VoiceOut[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_VoiceOut[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < REFOUT_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_RefOut[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_RefOut[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_RefOut[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < DAC0OUT_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_Dac0Out[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_Dac0Out[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_Dac0Out[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < DAC1OUT_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_Dac1Out[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_Dac1Out[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_Dac1Out[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < DPATH_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswD_DpathDa[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_DpathDa[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_DpathDa[bCh]
					& 0xFFFE);

		if (((UINT16)psVolInfo->aswD_DpathAd[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswD_DpathAd[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswD_DpathAd[bCh]
					& 0xFFFE);
	}

	for (bCh = 0; bCh < LINEIN1_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_LineIn1[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_LineIn1[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_LineIn1[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Mic1[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Mic1[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Mic1[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Mic2[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Mic2[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Mic2[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Mic3[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Mic3[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Mic3[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < MIC4_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Mic4[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Mic4[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Mic4[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < HP_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Hp[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Hp[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Hp[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < SP_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Sp[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Sp[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Sp[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < RC_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_Rc[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_Rc[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_Rc[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < LINEOUT1_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_LineOut1[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_LineOut1[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_LineOut1[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < LINEOUT2_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_LineOut2[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_LineOut2[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_LineOut2[bCh]
					& 0xFFFE);
	}
	for (bCh = 0; bCh < HPDET_VOL_CHANNELS; bCh++) {
		if (((UINT16)psVolInfo->aswA_HpDet[bCh] & 0x01) != 0)
			gsGlobalInfo.sVolInfo.aswA_HpDet[bCh]	=
				(SINT16)((UINT16)psVolInfo->aswA_HpDet[bCh]
					& 0xFFFE);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetVolInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetVolInfo
 *
 *	Description:
 *			Get volume setting.
 *	Arguments:
 *			psVolInfo		volume setting
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetVolInfo(
	struct MCDRV_VOL_INFO	*psVolInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetVolInfo");
#endif

	*psVolInfo	= gsGlobalInfo.sVolInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetVolInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetDioInfo
 *
 *	Description:
 *			Set digital io information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDioInfo(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT32	dUpdateInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetDioInfo");
#endif


	if ((dUpdateInfo & MCDRV_MUSIC_COM_UPDATE_FLAG) != 0UL)
		SetDIOCommon(psDioInfo, 0);
	if ((dUpdateInfo & MCDRV_EXT_COM_UPDATE_FLAG) != 0UL)
		SetDIOCommon(psDioInfo, 1);
	if ((dUpdateInfo & MCDRV_VOICE_COM_UPDATE_FLAG) != 0UL)
		SetDIOCommon(psDioInfo, 2);
	if ((dUpdateInfo & MCDRV_HIFI_COM_UPDATE_FLAG) != 0UL)
		SetDIOCommon(psDioInfo, 3);

	if ((dUpdateInfo & MCDRV_MUSIC_DIR_UPDATE_FLAG) != 0UL)
		SetDIODIR(psDioInfo, 0);
	if ((dUpdateInfo & MCDRV_EXT_DIR_UPDATE_FLAG) != 0UL)
		SetDIODIR(psDioInfo, 1);
	if ((dUpdateInfo & MCDRV_VOICE_DIR_UPDATE_FLAG) != 0UL)
		SetDIODIR(psDioInfo, 2);
	if ((dUpdateInfo & MCDRV_HIFI_DIR_UPDATE_FLAG) != 0UL)
		SetDIODIR(psDioInfo, 3);

	if ((dUpdateInfo & MCDRV_MUSIC_DIT_UPDATE_FLAG) != 0UL)
		SetDIODIT(psDioInfo, 0);
	if ((dUpdateInfo & MCDRV_EXT_DIT_UPDATE_FLAG) != 0UL)
		SetDIODIT(psDioInfo, 1);
	if ((dUpdateInfo & MCDRV_VOICE_DIT_UPDATE_FLAG) != 0UL)
		SetDIODIT(psDioInfo, 2);
	if ((dUpdateInfo & MCDRV_HIFI_DIT_UPDATE_FLAG) != 0UL)
		SetDIODIT(psDioInfo, 3);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetDioInfo", 0);
#endif
}

/****************************************************************************
 *	SetDIOCommon
 *
 *	Description:
 *			Set digital io common information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIOCommon(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetDIOCommon");
#endif

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bMasterSlave
		= psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bAutoFs
		= psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bFs
		= psDioInfo->asPortInfo[bPort].sDioCommon.bFs;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bBckFs
		= psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface
		= psDioInfo->asPortInfo[bPort].sDioCommon.bInterface;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bBckInvert
		= psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bSrcThru
		= psDioInfo->asPortInfo[bPort].sDioCommon.bSrcThru;

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmHizTim
		= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmFrame
		= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmHighPeriod
		= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHighPeriod;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetDIOCommon", 0);
#endif
}

/****************************************************************************
 *	SetDIODIR
 *
 *	Description:
 *			Set digital io dir information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIODIR(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetDIODIR");
#endif

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bBitSel
		= psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bMode
		= psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bMode;

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bMono
		= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bMono;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bOrder
		= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bLaw
		= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bBitSel
		= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetDIODIR", 0);
#endif
}

/****************************************************************************
 *	SetDIODIT
 *
 *	Description:
 *			Set digital io dit information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIODIT(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetDIODIT");
#endif

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.bStMode
		= psDioInfo->asPortInfo[bPort].sDit.bStMode;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.bEdge
		= psDioInfo->asPortInfo[bPort].sDit.bEdge;

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bBitSel
		= psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bMode
		= psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bMode;

	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bMono
		= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bMono;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bOrder
		= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bLaw
		= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw;
	gsGlobalInfo.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bBitSel
		= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetDIODIT", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetDioInfo
 *
 *	Description:
 *			Get digital io information.
 *	Arguments:
 *			psDioInfo	digital io information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDioInfo(
	struct MCDRV_DIO_INFO	*psDioInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetDioInfo");
#endif

	*psDioInfo	= gsGlobalInfo.sDioInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetDioInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetDioPathInfo
 *
 *	Description:
 *			Set digital io path information.
 *	Arguments:
 *			psDioPathInfo	digital io path information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDioPathInfo(
	const struct MCDRV_DIOPATH_INFO	*psDioPathInfo,
	UINT32	dUpdateInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetDioPathInfo");
#endif


	if ((dUpdateInfo & MCDRV_MUSICNUM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.bMusicCh
				= psDioPathInfo->bMusicCh;

	if ((dUpdateInfo & MCDRV_PHYS0_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abPhysPort[0]
				= psDioPathInfo->abPhysPort[0];

	if ((dUpdateInfo & MCDRV_PHYS1_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abPhysPort[1]
				= psDioPathInfo->abPhysPort[1];

	if ((dUpdateInfo & MCDRV_PHYS2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abPhysPort[2]
				= psDioPathInfo->abPhysPort[2];

	if ((dUpdateInfo & MCDRV_PHYS3_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abPhysPort[3]
				= psDioPathInfo->abPhysPort[3];


	if ((dUpdateInfo & MCDRV_DIR0SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicRSlot[0]
				= psDioPathInfo->abMusicRSlot[0];

	if ((dUpdateInfo & MCDRV_DIR1SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicRSlot[1]
				= psDioPathInfo->abMusicRSlot[1];

	if ((dUpdateInfo & MCDRV_DIR2SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicRSlot[2]
				= psDioPathInfo->abMusicRSlot[2];


	if ((dUpdateInfo & MCDRV_DIT0SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicTSlot[0]
				= psDioPathInfo->abMusicTSlot[0];

	if ((dUpdateInfo & MCDRV_DIT1SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicTSlot[1]
				= psDioPathInfo->abMusicTSlot[1];

	if ((dUpdateInfo & MCDRV_DIT2SLOT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sDioPathInfo.abMusicTSlot[2]
				= psDioPathInfo->abMusicTSlot[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetDioPathInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetDioPathInfo
 *
 *	Description:
 *			Get digital io path information.
 *	Arguments:
 *			psDioPathInfo	digital io path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDioPathInfo(
	struct MCDRV_DIOPATH_INFO	*psDioPathInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetDioPathInfo");
#endif

	*psDioPathInfo	= gsGlobalInfo.sDioPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetDioPathInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetSwap
 *
 *	Description:
 *			Get Swap info.
 *	Arguments:
 *			psSwapInfo	pointer to struct MCDRV_SWAP_INFO struct
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetSwap(
	const struct MCDRV_SWAP_INFO	*psSwapInfo,
	UINT32	dUpdateInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetSwap");
#endif


	if ((dUpdateInfo & MCDRV_SWAP_ADIF0_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bAdif0	= psSwapInfo->bAdif0;

	if ((dUpdateInfo & MCDRV_SWAP_ADIF1_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bAdif1	= psSwapInfo->bAdif1;

	if ((dUpdateInfo & MCDRV_SWAP_ADIF2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bAdif2	= psSwapInfo->bAdif2;

	if ((dUpdateInfo & MCDRV_SWAP_DAC0_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bDac0	= psSwapInfo->bDac0;

	if ((dUpdateInfo & MCDRV_SWAP_DAC1_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bDac1	= psSwapInfo->bDac1;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN0_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicIn0	= psSwapInfo->bMusicIn0;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN1_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicIn1	= psSwapInfo->bMusicIn1;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicIn2	= psSwapInfo->bMusicIn2;

	if ((dUpdateInfo & MCDRV_SWAP_EXTIN_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bExtIn	= psSwapInfo->bExtIn;

	if ((dUpdateInfo & MCDRV_SWAP_VOICEIN_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bVoiceIn	= psSwapInfo->bVoiceIn;

	if ((dUpdateInfo & MCDRV_SWAP_HIFIIN_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bHifiIn	= psSwapInfo->bHifiIn;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT0_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicOut0
			= psSwapInfo->bMusicOut0;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT1_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicOut1
			= psSwapInfo->bMusicOut1;

	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bMusicOut2
			= psSwapInfo->bMusicOut2;

	if ((dUpdateInfo & MCDRV_SWAP_EXTOUT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bExtOut	= psSwapInfo->bExtOut;

	if ((dUpdateInfo & MCDRV_SWAP_VOICEOUT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bVoiceOut	= psSwapInfo->bVoiceOut;

	if ((dUpdateInfo & MCDRV_SWAP_HIFIOUT_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sSwapInfo.bHifiOut	= psSwapInfo->bHifiOut;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetSwap", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetSwap
 *
 *	Description:
 *			Get Swap info.
 *	Arguments:
 *			psSwapInfo	pointer to struct MCDRV_SWAP_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetSwap(
	struct MCDRV_SWAP_INFO	*psSwapInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetSwap");
#endif

	*psSwapInfo	= gsGlobalInfo.sSwapInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetSwap", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetHSDet
 *
 *	Description:
 *			Get Headset Det info.
 *	Arguments:
 *			psHSDetInfo	pointer to MCDRV_HSDET_INFO struct
 *			psHSDet2Info	pointer to MCDRV_HSDET2_INFO struct
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetHSDet(
	const struct MCDRV_HSDET_INFO	*psHSDetInfo,
	const struct MCDRV_HSDET2_INFO	*psHSDet2Info,
	UINT32	dUpdateInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetHSDet");
#endif


	if ((dUpdateInfo & MCDRV_ENPLUGDET_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnPlugDet
			= psHSDetInfo->bEnPlugDet;

	if ((dUpdateInfo & MCDRV_ENPLUGDETDB_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnPlugDetDb
			= psHSDetInfo->bEnPlugDetDb;

	if ((dUpdateInfo & MCDRV_ENDLYKEYOFF_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnDlyKeyOff
			= psHSDetInfo->bEnDlyKeyOff;

	if ((dUpdateInfo & MCDRV_ENDLYKEYON_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnDlyKeyOn
			= psHSDetInfo->bEnDlyKeyOn;

	if ((dUpdateInfo & MCDRV_ENMICDET_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnMicDet
			= psHSDetInfo->bEnMicDet;

	if ((dUpdateInfo & MCDRV_ENKEYOFF_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnKeyOff
			= psHSDetInfo->bEnKeyOff;

	if ((dUpdateInfo & MCDRV_ENKEYON_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bEnKeyOn
			= psHSDetInfo->bEnKeyOn;

	if ((dUpdateInfo & MCDRV_HSDETDBNC_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bHsDetDbnc
			= psHSDetInfo->bHsDetDbnc;

	if ((dUpdateInfo & MCDRV_KEYOFFMTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKeyOffMtim
			= psHSDetInfo->bKeyOffMtim;

	if ((dUpdateInfo & MCDRV_KEYONMTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKeyOnMtim
			= psHSDetInfo->bKeyOnMtim;

	if ((dUpdateInfo & MCDRV_KEY0OFFDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey0OffDlyTim
			= psHSDetInfo->bKey0OffDlyTim;

	if ((dUpdateInfo & MCDRV_KEY1OFFDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey1OffDlyTim
			= psHSDetInfo->bKey1OffDlyTim;

	if ((dUpdateInfo & MCDRV_KEY2OFFDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey2OffDlyTim
			= psHSDetInfo->bKey2OffDlyTim;

	if ((dUpdateInfo & MCDRV_KEY0ONDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey0OnDlyTim
			= psHSDetInfo->bKey0OnDlyTim;

	if ((dUpdateInfo & MCDRV_KEY1ONDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey1OnDlyTim
			= psHSDetInfo->bKey1OnDlyTim;

	if ((dUpdateInfo & MCDRV_KEY2ONDLYTIM_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey2OnDlyTim
			= psHSDetInfo->bKey2OnDlyTim;

	if ((dUpdateInfo & MCDRV_KEY0ONDLYTIM2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey0OnDlyTim2
			= psHSDetInfo->bKey0OnDlyTim2;

	if ((dUpdateInfo & MCDRV_KEY1ONDLYTIM2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey1OnDlyTim2
			= psHSDetInfo->bKey1OnDlyTim2;

	if ((dUpdateInfo & MCDRV_KEY2ONDLYTIM2_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bKey2OnDlyTim2
			= psHSDetInfo->bKey2OnDlyTim2;

	if ((dUpdateInfo & MCDRV_IRQTYPE_UPDATE_FLAG) != 0UL) {
		gsGlobalInfo.sHSDetInfo.bIrqType
			= psHSDetInfo->bIrqType;
		if (psHSDet2Info != NULL) {
			gsGlobalInfo.sHSDet2Info.bPlugDetDbIrqType
				= psHSDet2Info->bPlugDetDbIrqType;
			gsGlobalInfo.sHSDet2Info.bPlugUndetDbIrqType
				= psHSDet2Info->bPlugUndetDbIrqType;
			gsGlobalInfo.sHSDet2Info.bMicDetIrqType
				= psHSDet2Info->bMicDetIrqType;
			gsGlobalInfo.sHSDet2Info.bPlugDetIrqType
				= psHSDet2Info->bPlugDetIrqType;
			gsGlobalInfo.sHSDet2Info.bKey0OnIrqType
				= psHSDet2Info->bKey0OnIrqType;
			gsGlobalInfo.sHSDet2Info.bKey1OnIrqType
				= psHSDet2Info->bKey1OnIrqType;
			gsGlobalInfo.sHSDet2Info.bKey2OnIrqType
				= psHSDet2Info->bKey2OnIrqType;
			gsGlobalInfo.sHSDet2Info.bKey0OffIrqType
				= psHSDet2Info->bKey0OffIrqType;
			gsGlobalInfo.sHSDet2Info.bKey1OffIrqType
				= psHSDet2Info->bKey1OffIrqType;
			gsGlobalInfo.sHSDet2Info.bKey2OffIrqType
				= psHSDet2Info->bKey2OffIrqType;
		}
	}

	if ((dUpdateInfo & MCDRV_DETINV_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bDetInInv
			= psHSDetInfo->bDetInInv;

	if ((dUpdateInfo & MCDRV_HSDETMODE_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bHsDetMode
			= psHSDetInfo->bHsDetMode;

	if ((dUpdateInfo & MCDRV_SPERIOD_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bSperiod
			= psHSDetInfo->bSperiod;

	if ((dUpdateInfo & MCDRV_LPERIOD_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bLperiod
			= psHSDetInfo->bLperiod;

	if ((dUpdateInfo & MCDRV_DBNCNUMPLUG_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bDbncNumPlug
			= psHSDetInfo->bDbncNumPlug;

	if ((dUpdateInfo & MCDRV_DBNCNUMMIC_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bDbncNumMic
			= psHSDetInfo->bDbncNumMic;

	if ((dUpdateInfo & MCDRV_DBNCNUMKEY_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bDbncNumKey
			= psHSDetInfo->bDbncNumKey;

	if ((dUpdateInfo & MCDRV_SGNL_UPDATE_FLAG) != 0UL) {
		gsGlobalInfo.sHSDetInfo.bSgnlPeriod
			= psHSDetInfo->bSgnlPeriod;
		gsGlobalInfo.sHSDetInfo.bSgnlNum
			= psHSDetInfo->bSgnlNum;
		gsGlobalInfo.sHSDetInfo.bSgnlPeak
			= psHSDetInfo->bSgnlPeak;
	}

	if ((dUpdateInfo & MCDRV_IMPSEL_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bImpSel
			= psHSDetInfo->bImpSel;

	if ((dUpdateInfo & MCDRV_DLYIRQSTOP_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.bDlyIrqStop
			= psHSDetInfo->bDlyIrqStop;

	if ((dUpdateInfo & MCDRV_CBFUNC_UPDATE_FLAG) != 0UL)
		gsGlobalInfo.sHSDetInfo.cbfunc
			= psHSDetInfo->cbfunc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetHSDet", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetHSDet
 *
 *	Description:
 *			Get Headset Det info.
 *	Arguments:
 *			psHSDetInfo	pointer to MCDRV_HSDET_INFO struct
 *			psHSDet2Info	pointer to MCDRV_HSDET2_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetHSDet(
	struct MCDRV_HSDET_INFO	*psHSDetInfo,
	struct MCDRV_HSDET2_INFO	*psHSDet2Info
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetHSDet");
#endif

	*psHSDetInfo	= gsGlobalInfo.sHSDetInfo;
	if (psHSDet2Info != NULL) {
		;
		*psHSDet2Info	= gsGlobalInfo.sHSDet2Info;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetHSDet", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetAecInfo
 *
 *	Description:
 *			Set AEC info.
 *	Arguments:
 *			psAecInfo	pointer to MCDRV_AEC_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetAecInfo(
	const struct MCDRV_AEC_INFO	*psAecInfo
)
{
	int	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetAecInfo");
#endif


	if (psAecInfo->sAecConfig.bFDspLocate != 0xFF)
		gsGlobalInfo.sAecInfo.sAecConfig
			= psAecInfo->sAecConfig;

	gsGlobalInfo.sAecInfo.sAecAudioengine.bEnable
		= psAecInfo->sAecAudioengine.bEnable;
	if (psAecInfo->sAecAudioengine.bEnable != 0) {
		if (psAecInfo->sAecAudioengine.bAEOnOff != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bAEOnOff
				= psAecInfo->sAecAudioengine.bAEOnOff;
		if (psAecInfo->sAecAudioengine.bFDspOnOff != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bFDspOnOff
				= psAecInfo->sAecAudioengine.bFDspOnOff;
		if (psAecInfo->sAecAudioengine.bBDspAE0Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bBDspAE0Src
				= psAecInfo->sAecAudioengine.bBDspAE0Src;
		if (psAecInfo->sAecAudioengine.bBDspAE1Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bBDspAE1Src
				= psAecInfo->sAecAudioengine.bBDspAE1Src;
		if (psAecInfo->sAecAudioengine.bMixerIn0Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn0Src
				= psAecInfo->sAecAudioengine.bMixerIn0Src;
		if (psAecInfo->sAecAudioengine.bMixerIn1Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn1Src
				= psAecInfo->sAecAudioengine.bMixerIn1Src;
		if (psAecInfo->sAecAudioengine.bMixerIn2Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn2Src
				= psAecInfo->sAecAudioengine.bMixerIn2Src;
		if (psAecInfo->sAecAudioengine.bMixerIn3Src != 2)
			gsGlobalInfo.sAecInfo.sAecAudioengine.bMixerIn3Src
				= psAecInfo->sAecAudioengine.bMixerIn3Src;
	}
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecBDsp
		= psAecInfo->sAecAudioengine.sAecBDsp;
	gsGlobalInfo.sAecInfo.sAecAudioengine.sAecFDsp
		= psAecInfo->sAecAudioengine.sAecFDsp;

	gsGlobalInfo.sAecInfo.sAecVBox.bEnable
		= psAecInfo->sAecVBox.bEnable;
	if (psAecInfo->sAecVBox.bEnable != 0) {
		if (psAecInfo->sAecVBox.bCDspFuncAOnOff != 2)
			gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff
				= psAecInfo->sAecVBox.bCDspFuncAOnOff;
		if (psAecInfo->sAecVBox.bCDspFuncBOnOff != 2)
			gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff
				= psAecInfo->sAecVBox.bCDspFuncBOnOff;
		if (psAecInfo->sAecVBox.bFDspOnOff != 2)
			gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff
				= psAecInfo->sAecVBox.bFDspOnOff;
		if (psAecInfo->sAecVBox.bFdsp_Po_Source != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bFdsp_Po_Source
				= psAecInfo->sAecVBox.bFdsp_Po_Source;
		if (psAecInfo->sAecVBox.bISrc2_VSource != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bISrc2_VSource
				= psAecInfo->sAecVBox.bISrc2_VSource;
		if (psAecInfo->sAecVBox.bISrc2_Ch1_VSource != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bISrc2_Ch1_VSource
				= psAecInfo->sAecVBox.bISrc2_Ch1_VSource;
		if (psAecInfo->sAecVBox.bISrc3_VSource != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bISrc3_VSource
				= psAecInfo->sAecVBox.bISrc3_VSource;
		if (psAecInfo->sAecVBox.bLPt2_VSource != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_VSource
				= psAecInfo->sAecVBox.bLPt2_VSource;
		if (psAecInfo->sAecVBox.bLPt2_Mix_VolO != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_Mix_VolO
				= psAecInfo->sAecVBox.bLPt2_Mix_VolO;
		if (psAecInfo->sAecVBox.bLPt2_Mix_VolI != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_Mix_VolI
				= psAecInfo->sAecVBox.bLPt2_Mix_VolI;
		if (psAecInfo->sAecVBox.bSrc3_Ctrl != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Ctrl
				= psAecInfo->sAecVBox.bSrc3_Ctrl;
		if (psAecInfo->sAecVBox.bSrc2_Fs != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bSrc2_Fs
				= psAecInfo->sAecVBox.bSrc2_Fs;
		if (psAecInfo->sAecVBox.bSrc2_Thru != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bSrc2_Thru
				= psAecInfo->sAecVBox.bSrc2_Thru;
		if (psAecInfo->sAecVBox.bSrc3_Fs != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Fs
				= psAecInfo->sAecVBox.bSrc3_Fs;
		if (psAecInfo->sAecVBox.bSrc3_Thru != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.bSrc3_Thru
				= psAecInfo->sAecVBox.bSrc3_Thru;
		if (psAecInfo->sAecVBox.sAecCDspDbg.bJtagOn != 0xFF)
			gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn
				= psAecInfo->sAecVBox.sAecCDspDbg.bJtagOn;
	}
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspA
		= psAecInfo->sAecVBox.sAecCDspA;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspB
		= psAecInfo->sAecVBox.sAecCDspB;
	gsGlobalInfo.sAecInfo.sAecVBox.sAecFDsp
		= psAecInfo->sAecVBox.sAecFDsp;

	for (i = 0; i < MCDRV_AEC_OUTPUT_N; i++) {
		if (psAecInfo->sOutput.bLpf_Pre_Thru[i] != 0xFF) {
			gsGlobalInfo.sAecInfo.sOutput.bLpf_Pre_Thru[i]
				= psAecInfo->sOutput.bLpf_Pre_Thru[i];
			gsGlobalInfo.sAecInfo.sOutput.bLpf_Post_Thru[i]
				= psAecInfo->sOutput.bLpf_Post_Thru[i];
			gsGlobalInfo.sAecInfo.sOutput.bDcc_Sel[i]
				= psAecInfo->sOutput.bDcc_Sel[i];
			gsGlobalInfo.sAecInfo.sOutput.bPow_Det_Lvl[i]
				= psAecInfo->sOutput.bPow_Det_Lvl[i];
			gsGlobalInfo.sAecInfo.sOutput.bOsf_Sel[i]
				= psAecInfo->sOutput.bOsf_Sel[i];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Enb[i]
				= psAecInfo->sOutput.bSys_Eq_Enb[i];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][0]
				= psAecInfo->sOutput.bSys_Eq_Coef_A0[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][1]
				= psAecInfo->sOutput.bSys_Eq_Coef_A0[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A0[i][2]
				= psAecInfo->sOutput.bSys_Eq_Coef_A0[i][2];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][0]
				= psAecInfo->sOutput.bSys_Eq_Coef_A1[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][1]
				= psAecInfo->sOutput.bSys_Eq_Coef_A1[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A1[i][2]
				= psAecInfo->sOutput.bSys_Eq_Coef_A1[i][2];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][0]
				= psAecInfo->sOutput.bSys_Eq_Coef_A2[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][1]
				= psAecInfo->sOutput.bSys_Eq_Coef_A2[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_A2[i][2]
				= psAecInfo->sOutput.bSys_Eq_Coef_A2[i][2];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][0]
				= psAecInfo->sOutput.bSys_Eq_Coef_B1[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][1]
				= psAecInfo->sOutput.bSys_Eq_Coef_B1[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B1[i][2]
				= psAecInfo->sOutput.bSys_Eq_Coef_B1[i][2];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][0]
				= psAecInfo->sOutput.bSys_Eq_Coef_B2[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][1]
				= psAecInfo->sOutput.bSys_Eq_Coef_B2[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bSys_Eq_Coef_B2[i][2]
				= psAecInfo->sOutput.bSys_Eq_Coef_B2[i][2];
			gsGlobalInfo.sAecInfo.sOutput.bClip_Md[i]
				= psAecInfo->sOutput.bClip_Md[i];
			gsGlobalInfo.sAecInfo.sOutput.bClip_Att[i]
				= psAecInfo->sOutput.bClip_Att[i];
			gsGlobalInfo.sAecInfo.sOutput.bClip_Rel[i]
				= psAecInfo->sOutput.bClip_Rel[i];
			gsGlobalInfo.sAecInfo.sOutput.bClip_G[i]
				= psAecInfo->sOutput.bClip_G[i];
			gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[i][0]
				= psAecInfo->sOutput.bOsf_Gain[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bOsf_Gain[i][1]
				= psAecInfo->sOutput.bOsf_Gain[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bDcl_OnOff[i]
				= psAecInfo->sOutput.bDcl_OnOff[i];
			gsGlobalInfo.sAecInfo.sOutput.bDcl_Gain[i]
				= psAecInfo->sOutput.bDcl_Gain[i];
			gsGlobalInfo.sAecInfo.sOutput.bDcl_Limit[i][0]
				= psAecInfo->sOutput.bDcl_Limit[i][0];
			gsGlobalInfo.sAecInfo.sOutput.bDcl_Limit[i][1]
				= psAecInfo->sOutput.bDcl_Limit[i][1];
			gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_OnOff[i]
				= psAecInfo->sOutput.bRandom_Dither_OnOff[i];
			gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_Level[i]
				= psAecInfo->sOutput.bRandom_Dither_Level[i];
			gsGlobalInfo.sAecInfo.sOutput.bRandom_Dither_POS[i]
				= psAecInfo->sOutput.bRandom_Dither_POS[i];
			gsGlobalInfo.sAecInfo.sOutput.bDc_Dither_OnOff[i]
				= psAecInfo->sOutput.bDc_Dither_OnOff[i];
			gsGlobalInfo.sAecInfo.sOutput.bDc_Dither_Level[i]
				= psAecInfo->sOutput.bDc_Dither_Level[i];
			gsGlobalInfo.sAecInfo.sOutput.bDither_Type[i]
				= psAecInfo->sOutput.bDither_Type[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_On[i]
				= psAecInfo->sOutput.bDng_On[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_Zero[i]
				= psAecInfo->sOutput.bDng_Zero[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_Time[i]
				= psAecInfo->sOutput.bDng_Time[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_Fw[i]
				= psAecInfo->sOutput.bDng_Fw[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_Target[i]
				= psAecInfo->sOutput.bDng_Target[i];
			gsGlobalInfo.sAecInfo.sOutput.bDng_Target_LineOut[i]
				= psAecInfo->sOutput.bDng_Target_LineOut[i];
		}
		if (psAecInfo->sOutput.sSysEqEx[i].bEnable == 0)
			continue;
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A0[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A0[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A0[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A1[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A1[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A1[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A2[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A2[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_A2[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B1[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B1[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B1[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B2[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B2[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[0].bCoef_B2[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A0[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A0[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A0[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A1[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A1[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A1[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A2[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A2[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_A2[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B1[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B1[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B1[2];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[0]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B2[0];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[1]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B2[1];
		gsGlobalInfo.sAecInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2[2]
			= psAecInfo->sOutput.sSysEqEx[i].sBand[1].bCoef_B2[2];
	}
	gsGlobalInfo.sAecInfo.sOutput.bSig_Det_Lvl
		= psAecInfo->sOutput.bSig_Det_Lvl;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Attack
		= psAecInfo->sOutput.bDng_Attack;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Release
		= psAecInfo->sOutput.bDng_Release;
	gsGlobalInfo.sAecInfo.sOutput.bDng_Target_Rc
		= psAecInfo->sOutput.bDng_Target_Rc;

	for (i = 0; i < MCDRV_AEC_INPUT_N; i++) {
		if (psAecInfo->sInput.bDsf32_L_Type[i] != 0xFF) {
			gsGlobalInfo.sAecInfo.sInput.bDsf32_L_Type[i]
				= psAecInfo->sInput.bDsf32_L_Type[i];
			gsGlobalInfo.sAecInfo.sInput.bDsf32_R_Type[i]
				= psAecInfo->sInput.bDsf32_R_Type[i];
			gsGlobalInfo.sAecInfo.sInput.bDsf4_Sel[i]
				= psAecInfo->sInput.bDsf4_Sel[i];
			gsGlobalInfo.sAecInfo.sInput.bDcc_Sel[i]
				= psAecInfo->sInput.bDcc_Sel[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_On[i]
				= psAecInfo->sInput.bDng_On[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_Att[i]
				= psAecInfo->sInput.bDng_Att[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_Rel[i]
				= psAecInfo->sInput.bDng_Rel[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_Fw[i]
				= psAecInfo->sInput.bDng_Fw[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_Tim[i]
				= psAecInfo->sInput.bDng_Tim[i];
			gsGlobalInfo.sAecInfo.sInput.bDng_Zero[i][0]
				= psAecInfo->sInput.bDng_Zero[i][0];
			gsGlobalInfo.sAecInfo.sInput.bDng_Zero[i][1]
				= psAecInfo->sInput.bDng_Zero[i][1];
			gsGlobalInfo.sAecInfo.sInput.bDng_Tgt[i][0]
				= psAecInfo->sInput.bDng_Tgt[i][0];
			gsGlobalInfo.sAecInfo.sInput.bDng_Tgt[i][1]
				= psAecInfo->sInput.bDng_Tgt[i][1];
			gsGlobalInfo.sAecInfo.sInput.bDepop_Att[i]
				= psAecInfo->sInput.bDepop_Att[i];
			gsGlobalInfo.sAecInfo.sInput.bDepop_Wait[i]
				= psAecInfo->sInput.bDepop_Wait[i];
		}
	}
	gsGlobalInfo.sAecInfo.sInput.bRef_Sel	= psAecInfo->sInput.bRef_Sel;

	if (psAecInfo->sPdm.bMode != 0xFF)
		gsGlobalInfo.sAecInfo.sPdm	= psAecInfo->sPdm;

	gsGlobalInfo.sAecInfo.sE2.bEnable
		= psAecInfo->sE2.bEnable;
	if (psAecInfo->sE2.bEnable != 0) {
		if (psAecInfo->sE2.bE2_Da_Sel != 4)
			gsGlobalInfo.sAecInfo.sE2.bE2_Da_Sel
				= psAecInfo->sE2.bE2_Da_Sel;
		if (psAecInfo->sE2.bE2_Ad_Sel != 8)
			gsGlobalInfo.sAecInfo.sE2.bE2_Ad_Sel
				= psAecInfo->sE2.bE2_Ad_Sel;
		if (psAecInfo->sE2.bE2OnOff != 2)
			gsGlobalInfo.sAecInfo.sE2.bE2OnOff
				= psAecInfo->sE2.bE2OnOff;
	}
	gsGlobalInfo.sAecInfo.sE2.sE2Config
		= psAecInfo->sE2.sE2Config;

	if (psAecInfo->sAdj.bHold != 0xFF)
		gsGlobalInfo.sAecInfo.sAdj
			= psAecInfo->sAdj;

	if (psAecInfo->sEDspMisc.bI2SOut_Enb != 0xFF)
		gsGlobalInfo.sAecInfo.sEDspMisc
			= psAecInfo->sEDspMisc;

	if (psAecInfo->sControl.bCommand == 0xFF) {
		gsGlobalInfo.sAecInfo.sControl.bCommand		= 0;
		gsGlobalInfo.sAecInfo.sControl.bParam[0]	= 0;
		gsGlobalInfo.sAecInfo.sControl.bParam[1]	= 0;
		gsGlobalInfo.sAecInfo.sControl.bParam[2]	= 0;
		gsGlobalInfo.sAecInfo.sControl.bParam[3]	= 0;
	} else {
		gsGlobalInfo.sAecInfo.sControl
			= psAecInfo->sControl;
	}

	if (gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate == 0)
		gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff	= 0;
	else
		gsGlobalInfo.sAecInfo.sAecAudioengine.bFDspOnOff	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetAecInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_ReplaceAecInfo
 *
 *	Description:
 *			Replace AEC info.
 *	Arguments:
 *			psAecInfo	pointer to MCDRV_AEC_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_ReplaceAecInfo(
	const struct MCDRV_AEC_INFO	*psAecInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_ReplaceAecInfo");
#endif


	gsGlobalInfo.sAecInfo	= *psAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_ReplaceAecInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetAecInfo
 *
 *	Description:
 *			Get AEC info.
 *	Arguments:
 *			psAecInfo	pointer to MCDRV_AEC_INFO struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetAecInfo(
	struct MCDRV_AEC_INFO	*psAecInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetAecInfo");
#endif


	*psAecInfo	= gsGlobalInfo.sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetAec", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetDSPCBFunc
 *
 *	Description:
 *			Set DSP callback function.
 *	Arguments:
 *			pcbfunc		pointer to callback function
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDSPCBFunc(
	SINT32 (*pcbfunc)(SINT32, UINT32, UINT32)
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetDSPCBFunc");
#endif
	gsGlobalInfo.pcbfunc	= pcbfunc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetDSPCBFunc", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetDSPCBFunc
 *
 *	Description:
 *			Get DSP callback function.
 *	Arguments:
 *			pcbfunc		pointer to callback function
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDSPCBFunc(
	SINT32 (**pcbfunc)(SINT32, UINT32, UINT32)
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetDSPCBFunc");
#endif
	*pcbfunc	= gsGlobalInfo.pcbfunc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetDSPCBFunc", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetGPMode
 *
 *	Description:
 *			Set GP mode.
 *	Arguments:
 *			psGpMode	GP mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetGPMode(
	const struct MCDRV_GP_MODE	*psGpMode
)
{
	UINT8	bPad;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetGPMode");
#endif


	for (bPad = 0; bPad < 3; bPad++) {
		if ((psGpMode->abGpDdr[bPad] == MCDRV_GPDDR_IN)
		|| (psGpMode->abGpDdr[bPad] == MCDRV_GPDDR_OUT)) {
			gsGlobalInfo.sGpMode.abGpDdr[bPad]
				= psGpMode->abGpDdr[bPad];
			if (psGpMode->abGpDdr[bPad] == MCDRV_GPDDR_IN)
				gsGlobalInfo.abGpPad[bPad]
					= 0;
		}
		if ((psGpMode->abGpHost[bPad] == MCDRV_GPHOST_CPU)
		|| (psGpMode->abGpHost[bPad] == MCDRV_GPHOST_CDSP))
			gsGlobalInfo.sGpMode.abGpHost[bPad]
				= psGpMode->abGpHost[bPad];

		if ((psGpMode->abGpInvert[bPad] == MCDRV_GPINV_NORMAL)
		|| (psGpMode->abGpInvert[bPad] == MCDRV_GPINV_INVERT))
			gsGlobalInfo.sGpMode.abGpInvert[bPad]
				= psGpMode->abGpInvert[bPad];
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetGPMode", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetGPMode
 *
 *	Description:
 *			Get GP mode.
 *	Arguments:
 *			psGpMode	GP mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetGPMode(
	struct MCDRV_GP_MODE	*psGpMode
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetGPMode");
#endif

	*psGpMode	= gsGlobalInfo.sGpMode;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetGPMode", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_SetGPMask
 *
 *	Description:
 *			Set GP mask.
 *	Arguments:
 *			bMask	GP mask
 *			dPadNo	PAD Number
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetGPMask(
	UINT8	bMask,
	UINT32	dPadNo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetGPMask");
#endif

	gsGlobalInfo.abGpMask[dPadNo]	= bMask;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetGPMask", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetGPMask
 *
 *	Description:
 *			Get GP mask.
 *	Arguments:
 *			dPadNo	PAD Number
 *	Return:
 *			GP mask
 *
 ****************************************************************************/
UINT8	McResCtrl_GetGPMask(
	UINT32	dPadNo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= gsGlobalInfo.abGpMask[dPadNo];
	McDebugLog_FuncIn("McResCtrl_GetGPMask");
	McDebugLog_FuncOut("McResCtrl_GetGPMask", &sdRet);
#endif
	return	gsGlobalInfo.abGpMask[dPadNo];
}

/****************************************************************************
 *	McResCtrl_SetGPPad
 *
 *	Description:
 *			Set GP pad.
 *	Arguments:
 *			bPad	GP pad value
 *			dPadNo	PAD Number
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetGPPad(
	UINT8	bPad,
	UINT32	dPadNo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetGPPad");
#endif

	gsGlobalInfo.abGpPad[dPadNo]	= bPad;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetGPPad", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetGPPad
 *
 *	Description:
 *			Get GP pad value.
 *	Arguments:
 *			dPadNo	PAD Number
 *	Return:
 *			GP pad value
 *
 ****************************************************************************/
UINT8	McResCtrl_GetGPPad(
	UINT32	dPadNo
)
{
	UINT8	bRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetGPPad");
#endif

	bRet	= gsGlobalInfo.abGpPad[dPadNo];

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("McResCtrl_GetGPPad", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	McResCtrl_SetClkSel
 *
 *	Description:
 *			Set CLK_SEL.
 *	Arguments:
 *			bClkSel	CLK_SEL
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetClkSel(
	UINT8	bClkSel
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetClkSel");
#endif

	gsGlobalInfo.bClkSel	= bClkSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetClkSel", 0);
#endif
}
/****************************************************************************
 *	McResCtrl_GetClkSel
 *
 *	Description:
 *			Get CLK_SEL.
 *	Arguments:
 *			none
 *	Return:
 *			CLK_SEL
 *
 ****************************************************************************/
UINT8	McResCtrl_GetClkSel(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetClkSel");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= gsGlobalInfo.bClkSel;
	McDebugLog_FuncOut("McResCtrl_SetClkSel", &sdRet);
#endif
	return gsGlobalInfo.bClkSel;
}
/****************************************************************************
 *	McResCtrl_SetEClkSel
 *
 *	Description:
 *			Set ECLK_SEL.
 *	Arguments:
 *			bEClkSel	ECLK_SEL
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetEClkSel(
	UINT8	bEClkSel
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetEClkSel");
#endif

	gsGlobalInfo.bEClkSel	= bEClkSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetEClkSel", 0);
#endif
}
/****************************************************************************
 *	McResCtrl_GetEClkSel
 *
 *	Description:
 *			Get ECLK_SEL.
 *	Arguments:
 *			none
 *	Return:
 *			ECLK_SEL
 *
 ****************************************************************************/
UINT8	McResCtrl_GetEClkSel(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetEClkSel");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= gsGlobalInfo.bEClkSel;
	McDebugLog_FuncOut("McResCtrl_GetEClkSel", &sdRet);
#endif
	return gsGlobalInfo.bEClkSel;
}

/****************************************************************************
 *	McResCtrl_SetCClkSel
 *
 *	Description:
 *			Set CDSP_DIVR.
 *	Arguments:
 *			bCClkSel	CDSP_DIVR
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetCClkSel(
	UINT8	bCClkSel
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetCClkSel");
#endif

	gsGlobalInfo.bCClkSel	= bCClkSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetCClkSel", 0);
#endif
}
/****************************************************************************
 *	McResCtrl_GetCClkSel
 *
 *	Description:
 *			Get CDSP_DIVR.
 *	Arguments:
 *			none
 *	Return:
 *			CDSP_DIVR
 *
 ****************************************************************************/
UINT8	McResCtrl_GetCClkSel(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetCClkSel");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= gsGlobalInfo.bCClkSel;
	McDebugLog_FuncOut("McResCtrl_GetCClkSel", &sdRet);
#endif
	return gsGlobalInfo.bCClkSel;
}
/****************************************************************************
 *	McResCtrl_SetFClkSel
 *
 *	Description:
 *			Set FDSP_DIVR.
 *	Arguments:
 *			bFClkSel	FDSP_DIVR
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetFClkSel(
	UINT8	bFClkSel
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetFClkSel");
#endif

	gsGlobalInfo.bFClkSel	= bFClkSel;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetFClkSel", 0);
#endif
}
/****************************************************************************
 *	McResCtrl_GetFClkSel
 *
 *	Description:
 *			Get FDSP_DIVR.
 *	Arguments:
 *			none
 *	Return:
 *			FDSP_DIVR
 *
 ****************************************************************************/
UINT8	McResCtrl_GetFClkSel(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetFClkSel");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= gsGlobalInfo.bFClkSel;
	McDebugLog_FuncOut("McResCtrl_GetFClkSel", &sdRet);
#endif
	return gsGlobalInfo.bFClkSel;
}


/****************************************************************************
 *	McResCtrl_SetPlugDetDB
 *
 *	Description:
 *			Set PlugDetDB.
 *	Arguments:
 *			bPlugDetDB	PlugDetDB
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetPlugDetDB(
	UINT8	bPlugDetDB
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_SetPlugDetDB");
#endif

	gsGlobalInfo.bPlugDetDB	= bPlugDetDB;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_SetPlugDetDB", 0);
#endif
}
/****************************************************************************
 *	McResCtrl_GetPlugDetDB
 *
 *	Description:
 *			Get bPlugDetDB.
 *	Arguments:
 *			none
 *	Return:
 *			bPlugDetDB
 *
 ****************************************************************************/
UINT8	McResCtrl_GetPlugDetDB(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetPlugDetDB");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= gsGlobalInfo.bPlugDetDB;
	McDebugLog_FuncOut("McResCtrl_GetPlugDetDB", &sdRet);
#endif
	return gsGlobalInfo.bPlugDetDB;
}


/****************************************************************************
 *	McResCtrl_GetVolReg
 *
 *	Description:
 *			Get value of volume registers.
 *	Arguments:
 *			psVolInfo	volume information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetVolReg(
	struct MCDRV_VOL_INFO	*psVolInfo
)
{
	int	iSrc;
	UINT8	bCh;
	enum MCDRV_DST_CH	abDSTCh[]	= {
		eMCDRV_DST_CH0, eMCDRV_DST_CH1};

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetVolReg");
#endif


	*psVolInfo	= gsGlobalInfo.sVolInfo;

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) == 0) {
		psVolInfo->aswD_MusicIn[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_MusicIn[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_MusicIn[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_MusicIn[0]);
		psVolInfo->aswD_MusicIn[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_MusicIn[1]);
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) == 0) {
		psVolInfo->aswD_ExtIn[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_ExtIn[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_ExtIn[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_ExtIn[0]);
		psVolInfo->aswD_ExtIn[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_ExtIn[1]);
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) == 0) {
		psVolInfo->aswD_VoiceIn[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_VoiceIn[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_VoiceIn[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_VoiceIn[0]);
		psVolInfo->aswD_VoiceIn[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_VoiceIn[1]);
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) == 0) {
		psVolInfo->aswD_RefIn[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_RefIn[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_RefIn[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_RefIn[0]);
		psVolInfo->aswD_RefIn[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_RefIn[1]);
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF0_ON) == 0) {
		psVolInfo->aswD_Adif0In[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_Adif0In[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_Adif0In[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif0In[0]);
		psVolInfo->aswD_Adif0In[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif0In[1]);
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF1_ON) == 0) {
		psVolInfo->aswD_Adif1In[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_Adif1In[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_Adif1In[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif1In[0]);
		psVolInfo->aswD_Adif1In[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif1In[1]);
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF2_ON) == 0) {
		psVolInfo->aswD_Adif2In[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_Adif2In[1]	= MCDRV_REG_MUTE;
	} else {
		psVolInfo->aswD_Adif2In[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif2In[0]);
		psVolInfo->aswD_Adif2In[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Adif2In[1]);
	}

	for (bCh = 0; bCh < GET_ARRAY_SIZE(abDSTCh); bCh++) {
		if (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, abDSTCh[bCh]) == 0)
			psVolInfo->aswD_MusicOut[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswD_MusicOut[bCh]	=
		McResCtrl_GetDigitalVolReg(psVolInfo->aswD_MusicOut[bCh]);

		if (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, abDSTCh[bCh]) == 0)
			psVolInfo->aswD_ExtOut[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswD_ExtOut[bCh]	=
		McResCtrl_GetDigitalVolReg(psVolInfo->aswD_ExtOut[bCh]);
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) == 0)
		psVolInfo->aswD_VoiceOut[0]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_VoiceOut[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_VoiceOut[0]);

	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) == 0)
		psVolInfo->aswD_VoiceOut[1]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_VoiceOut[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_VoiceOut[1]);

	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) == 0)
		psVolInfo->aswD_RefOut[0]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_RefOut[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_RefOut[0]);

	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) == 0)
		psVolInfo->aswD_RefOut[1]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_RefOut[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_RefOut[1]);

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) == 0) {
		psVolInfo->aswD_DpathDa[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_DpathDa[1]	= MCDRV_REG_MUTE;
	}

	iSrc	= GetD1Source(gsGlobalInfo.sPathInfo.asDac0, eMCDRV_DST_CH0);
	if ((iSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
		psVolInfo->aswD_Dac0Out[0]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_Dac0Out[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Dac0Out[0]);
	if ((iSrc & MCDRV_D1SRC_HIFIIN_ON) != 0)
		psVolInfo->aswD_DpathDa[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathDa[0]);

	iSrc	= GetD1Source(gsGlobalInfo.sPathInfo.asDac0, eMCDRV_DST_CH1);
	if ((iSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
		psVolInfo->aswD_Dac0Out[1]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_Dac0Out[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Dac0Out[1]);
	if ((iSrc & MCDRV_D1SRC_HIFIIN_ON) != 0)
		psVolInfo->aswD_DpathDa[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathDa[1]);

	iSrc	= GetD1Source(gsGlobalInfo.sPathInfo.asDac1, eMCDRV_DST_CH0);
	if ((iSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
		psVolInfo->aswD_Dac1Out[0]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_Dac1Out[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Dac1Out[0]);
	if ((iSrc & MCDRV_D1SRC_HIFIIN_ON) != 0)
		psVolInfo->aswD_DpathDa[0]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathDa[0]);

	iSrc	= GetD1Source(gsGlobalInfo.sPathInfo.asDac1, eMCDRV_DST_CH1);
	if ((iSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
		psVolInfo->aswD_Dac1Out[1]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswD_Dac1Out[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_Dac1Out[1]);
	if ((iSrc & MCDRV_D1SRC_HIFIIN_ON) != 0)
		psVolInfo->aswD_DpathDa[1]
		= McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathDa[1]);

	iSrc	=
		GetD1Source(gsGlobalInfo.sPathInfo.asHifiOut, eMCDRV_DST_CH0);
	if (iSrc == 0) {
		psVolInfo->aswD_DpathAd[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswD_DpathAd[1]	= MCDRV_REG_MUTE;
	} else {
		if (((iSrc&(MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)) != 0)
		|| ((iSrc&(MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)) != 0)
		) {
			psVolInfo->aswD_DpathAd[0]	=
			McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathAd[0]);
			psVolInfo->aswD_DpathAd[1]	=
			McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathAd[1]);
		} else {
			if (((iSrc&MCDRV_D2SRC_ADC1_ON) != 0)
			||
			(McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH0)
				!= 0))
				psVolInfo->aswD_DpathAd[0]	=
			McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathAd[0]);
			else
				psVolInfo->aswD_DpathAd[0]	=
					MCDRV_REG_MUTE;

			if (((iSrc&
				(MCDRV_D2SRC_ADC0_L_ON|MCDRV_D2SRC_ADC0_R_ON))
				 != 0)
			&&
			(McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH1)
				!= 0))
				psVolInfo->aswD_DpathAd[1]	=
			McResCtrl_GetDigitalVolReg(psVolInfo->aswD_DpathAd[1]);
			else
				psVolInfo->aswD_DpathAd[1]	=
					MCDRV_REG_MUTE;
		}
	}

	if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_M_ON) != 0))
		psVolInfo->aswA_LineIn1[0]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_LineIn1[0]);
	else
		psVolInfo->aswA_LineIn1[0]	= MCDRV_REG_MUTE;
	if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_R_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_M_ON) != 0))
		psVolInfo->aswA_LineIn1[1]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_LineIn1[1]);
	else
		psVolInfo->aswA_LineIn1[1]	= MCDRV_REG_MUTE;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC1_ON) != 0)
		psVolInfo->aswA_Mic1[0]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_Mic1[0]);
	else
		psVolInfo->aswA_Mic1[0]	= MCDRV_REG_MUTE;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC2_ON) != 0)
		psVolInfo->aswA_Mic2[0]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_Mic2[0]);
	else
		psVolInfo->aswA_Mic2[0]	= MCDRV_REG_MUTE;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC3_ON) != 0)
		psVolInfo->aswA_Mic3[0]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_Mic3[0]);
	else
		psVolInfo->aswA_Mic3[0]	= MCDRV_REG_MUTE;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC4_ON) != 0)
		psVolInfo->aswA_Mic4[0]	=
			GetAnaInVolReg(gsGlobalInfo.sVolInfo.aswA_Mic4[0]);
	else
		psVolInfo->aswA_Mic4[0]	= MCDRV_REG_MUTE;

	for (bCh = 0; bCh < GET_ARRAY_SIZE(abDSTCh); bCh++) {
		if (McResCtrl_HasSrc(eMCDRV_DST_HP, abDSTCh[bCh]) == 0)
			psVolInfo->aswA_Hp[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswA_Hp[bCh]	=
				GetHpVolReg(
					gsGlobalInfo.sVolInfo.aswA_Hp[bCh]);

		if (McResCtrl_HasSrc(eMCDRV_DST_SP, abDSTCh[bCh]) == 0)
			psVolInfo->aswA_Sp[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswA_Sp[bCh]	=
				GetSpVolReg(
					gsGlobalInfo.sVolInfo.aswA_Sp[bCh]);

		if (McResCtrl_HasSrc(eMCDRV_DST_LOUT1, abDSTCh[bCh]) == 0)
			psVolInfo->aswA_LineOut1[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswA_LineOut1[bCh]	=
			GetLOutVolReg(
				gsGlobalInfo.sVolInfo.aswA_LineOut1[bCh]);

		if (McResCtrl_HasSrc(eMCDRV_DST_LOUT2, abDSTCh[bCh]) == 0)
			psVolInfo->aswA_LineOut2[bCh]	= MCDRV_REG_MUTE;
		else
			psVolInfo->aswA_LineOut2[bCh]	=
			GetLOutVolReg(
				gsGlobalInfo.sVolInfo.aswA_LineOut2[bCh]);
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_RCV, eMCDRV_DST_CH0) == 0)
		psVolInfo->aswA_Rc[0]	= MCDRV_REG_MUTE;
	else
		psVolInfo->aswA_Rc[0]	=
			GetAnaOutVolReg(gsGlobalInfo.sVolInfo.aswA_Rc[0]);

	psVolInfo->aswA_HpDet[0]	=
		GetAnaOutVolReg(gsGlobalInfo.sVolInfo.aswA_HpDet[0]);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetVolReg", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetDigitalVolReg
 *
 *	Description:
 *			Get value of digital volume registers.
 *	Arguments:
 *			sdVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
SINT16	McResCtrl_GetDigitalVolReg(
	SINT32	sdVol
)
{
	SINT32	sdRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetDigitalVolReg");
#endif

	if (sdVol < (-95*256))
		sdRet	= 0;
	else if (sdVol < 0L)
		sdRet	= 96L + (sdVol-128L)/256L;
	else
		sdRet	= 96L + (sdVol+128L)/256L;

	if (sdRet > 114L)
		sdRet	= 114;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetDigitalVolReg", &sdRet);
#endif

	return (SINT16)sdRet;
}


/****************************************************************************
 *	GetAnaInVolReg
 *
 *	Description:
 *			Get update value of analog input volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetAnaInVolReg(
	SINT16	swVol
)
{
	SINT16	swRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetAnaInVolReg");
#endif

	if (swVol < (-30*256))
		swRet	= 0;
	else if (swVol < 0)
		swRet	= 0x21 + (swVol-128) / 256;
	else if (swVol <= (21*256))
		swRet	= 0x21 + (swVol+128) / 256;
	else if (swVol < (6080))	/*	6080:23.75*256	*/
		swRet	= 0x36 + ((swVol+64) / 128 - (21*2));
	else
		swRet	= 0x3C + (swVol/256 - 23) / 2;

	if (swRet < 0)
		swRet	= 0;
	else if (swRet > 0x3F)
		swRet	= 0x3F;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= swRet;
	McDebugLog_FuncOut("GetAnaInVolReg", &sdRet);
#endif
	return swRet;
}

/****************************************************************************
 *	GetAnaOutVolReg
 *
 *	Description:
 *			Get update value of analog output volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetAnaOutVolReg(
	SINT16	swVol
)
{
	SINT16	swRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetAnaOutVolReg");
#endif

	if (swVol < (-36*256))
		swRet	= 0;
	else if (swVol < (-4032))	/*	-4032:-15.75*256	*/
		swRet	= 0x42 + (swVol-128) / 256 + 17;
	else if (swVol < (-1504))	/*	-1504:-5.875*256	*/
		swRet	= 0x57 + ((swVol-64) / 128 + 6*2);
	else
		swRet	= 0x6F + ((swVol-32) / 64);

	if (swRet < 0)
		swRet	= 0;
	else if (swRet > 0x6F)
		swRet	= 0x6F;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= swRet;
	McDebugLog_FuncOut("GetAnaOutVolReg", &sdRet);
#endif
	return swRet;
}

/****************************************************************************
 *	GetSpVolReg
 *
 *	Description:
 *			Get update value of analog output volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetSpVolReg(
	SINT16	swVol
)
{
	SINT16	swRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetSpVolReg");
#endif

	if (swVol < (-36*256))
		swRet	= 0;
	else if (swVol < (-4032))	/*	-4032:-15.75*256	*/
		swRet	= 0x42 + (swVol-128) / 256 + 17;
	else if (swVol < (-1504))	/*	-1504:-5.875*256	*/
		swRet	= 0x57 + ((swVol-64) / 128 + 6*2);
	else if (swVol < 32)
		swRet	= 0x6F + ((swVol-32) / 64);
	else
		swRet	= 0x70 + ((swVol-32) / 64);

	if (swRet < 0)
		swRet	= 0;
	else if (swRet > 0x7F)
		swRet	= 0x7F;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= swRet;
	McDebugLog_FuncOut("GetSpVolReg", &sdRet);
#endif
	return swRet;
}

/****************************************************************************
 *	GetLOutVolReg
 *
 *	Description:
 *			Get update value of analog output volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetLOutVolReg(
	SINT16	swVol
)
{
	SINT16	swRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetLOutVolReg");
#endif

	if (swVol < (-38*256))
		swRet	= 0;
	else if (swVol < -9344)	/*	-9344:-36.5*256	*/
		swRet	= 0x2E;
	else if (swVol < -4032)	/*	-4032:-15.75*256	*/
		swRet	= 0x42 + (swVol-128) / 256 + 17;
	else if (swVol < -1504)	/*	-1504:-5.875*256	*/
		swRet	= 0x57 + ((swVol-64) / 128 + 6*2);
	else if (swVol < 32)
		swRet	= 0x6F + ((swVol-32) / 64);
	else
		swRet	= 0x70 + ((swVol-32) / 64);

	if (swRet < 0)
		swRet	= 0;
	else if (swRet > 0x77)
		swRet	= 0x77;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= swRet;
	McDebugLog_FuncOut("GetLOutVolReg", &sdRet);
#endif
	return swRet;
}

/****************************************************************************
 *	GetHpVolReg
 *
 *	Description:
 *			Get update value of analog Hp volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetHpVolReg(
	SINT16	swVol
)
{
	SINT16	swRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetHpVolReg");
#endif

	if (swVol < (-36*256))
		swRet	= 0;
	else if (swVol < (-4032))	/*	-4032:-15.75*256	*/
		swRet	= 0x43 + (swVol-128) / 256 + 16;
	else if (swVol < (-1504))	/*	-1504:-5.875*256	*/
		swRet	= 0x43 + ((swVol-64) / 128 + 16*2);
	else if (swVol < 0)
		swRet	= 0x57 + ((swVol-32) / 64 + 6*4);
	else
		swRet	= 0x6F + ((swVol+32) / 64);

	if (swRet < 0)
		swRet	= 0;
	else if (swRet > 0x7F)
		swRet	= 0x7F;



#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= swRet;
	McDebugLog_FuncOut("GetHpVolReg", &sdRet);
#endif

	return swRet;
}

/****************************************************************************
 *	McResCtrl_GetPowerInfo
 *
 *	Description:
 *			Get power information.
 *	Arguments:
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPowerInfo(
	struct MCDRV_POWER_INFO	*psPowerInfo
)
{
	UINT32	dHifiSrc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetPowerInfo");
#endif


	psPowerInfo->bDigital	= MCDRV_POWINFO_D_PLL_PD
				| MCDRV_POWINFO_D_PE_CLK_PD
				| MCDRV_POWINFO_D_PB_CLK_PD
				| MCDRV_POWINFO_D_PM_CLK_PD
				| MCDRV_POWINFO_D_PF_CLK_PD
				| MCDRV_POWINFO_D_PC_CLK_PD;

	dHifiSrc	= gsGlobalInfo.sPathInfo.asHifiOut[0].dSrcOnOff;

	if ((gsGlobalInfo.sAecInfo.sE2.bE2OnOff == 1)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_L_ON|MCDRV_D2SRC_ADC0_R_ON)
		!= 0)
	|| ((dHifiSrc & MCDRV_D1SRC_ADIF0_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC1_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)
		!= 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)
		!= 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) != 0)) {
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((gsGlobalInfo.sAecInfo.sAecAudioengine.bAEOnOff == 1)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_AE0_ON
				|MCDRV_D1SRC_AE1_ON
				|MCDRV_D1SRC_AE2_ON
				|MCDRV_D1SRC_AE3_ON) != 0)) {
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PB_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((gsGlobalInfo.sInitInfo.bPowerMode == MCDRV_POWMODE_CDSPDEBUG)
	|| (gsGlobalInfo.sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn != 0)
	|| (gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff != 0)
	|| (gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff != 0)) {
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PC_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate == 0)
	&& (gsGlobalInfo.sAecInfo.sAecAudioengine.bFDspOnOff == 1)) {
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PF_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate != 0)
	&& (gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff == 1)) {
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PF_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((psPowerInfo->bDigital & MCDRV_POWINFO_D_PM_CLK_PD) != 0) {
		if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) != 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) != 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) != 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_VOICEIN_ON) != 0)
		|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_VBOXIOOUT_ON) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) != 0)
		) {
			psPowerInfo->bDigital	&=
				(UINT8)~MCDRV_POWINFO_D_PE_CLK_PD;
			psPowerInfo->bDigital	&=
				(UINT8)~MCDRV_POWINFO_D_PM_CLK_PD;
			psPowerInfo->bDigital	&=
				(UINT8)~MCDRV_POWINFO_D_PLL_PD;
		}
	}

	/*	Analog power	*/
	psPowerInfo->abAnalog[0]	= MCI_AP_DEF;
	psPowerInfo->abAnalog[1]	= MCI_AP_DA0_DEF;
	psPowerInfo->abAnalog[2]	= MCI_AP_DA1_DEF;
	psPowerInfo->abAnalog[3]	= MCI_AP_MIC_DEF;
	psPowerInfo->abAnalog[4]	= MCI_AP_AD_DEF;

	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_L_ON) != 0) {
		;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_DA0L;
	}
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_R_ON) != 0) {
		;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_DA0R;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH0) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_HPL;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH1) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[1] &=	(UINT8)~MCB_AP_HPR;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_RCV, eMCDRV_DST_CH0) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_RC;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_LOUT1, eMCDRV_DST_CH0) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_LO1L;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_LOUT1, eMCDRV_DST_CH1) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[1] &= (UINT8)~MCB_AP_LO1R;
	}

	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_L_ON) != 0) {
		;
		psPowerInfo->abAnalog[2] &= (UINT8)~MCB_AP_DA1L;
	}
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_R_ON) != 0) {
		;
		psPowerInfo->abAnalog[2] &= (UINT8)~MCB_AP_DA1R;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_SP, eMCDRV_DST_CH0) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[2] &=
			(UINT8)~(MCB_AP_SPL2|MCB_AP_SPL1);
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_SP, eMCDRV_DST_CH1) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[2] &=
			(UINT8)~(MCB_AP_SPR2|MCB_AP_SPR1);
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_LOUT2, eMCDRV_DST_CH0) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[2] &= (UINT8)~MCB_AP_LO2L;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_LOUT2, eMCDRV_DST_CH1) != 0) {
		if (gsGlobalInfo.eAPMode == eMCDRV_APM_OFF)
			psPowerInfo->abAnalog[0] &= (UINT8)~MCB_AP_CP;
		psPowerInfo->abAnalog[2] &= (UINT8)~MCB_AP_LO2R;
	}

	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC1_ON) != 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MC1;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC2_ON) != 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MC2;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC3_ON) != 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MC3;
	if (McResCtrl_IsASrcUsed(MCDRV_ASRC_MIC4_ON) != 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MC4;

	if ((gsGlobalInfo.sPathInfo.asBias[0].dSrcOnOff & MCDRV_ASRC_MIC1_ON)
									!= 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MB1;
	if ((gsGlobalInfo.sPathInfo.asBias[1].dSrcOnOff & MCDRV_ASRC_MIC2_ON)
									!= 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MB2;
	if ((gsGlobalInfo.sPathInfo.asBias[2].dSrcOnOff & MCDRV_ASRC_MIC3_ON)
									!= 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MB3;
	if ((gsGlobalInfo.sPathInfo.asBias[3].dSrcOnOff & MCDRV_ASRC_MIC4_ON)
									!= 0)
		psPowerInfo->abAnalog[3] &= (UINT8)~MCB_MB4;

	if (McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH0) != 0)
		psPowerInfo->abAnalog[4] &= (UINT8)~MCB_AP_ADL;
	if (McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH1) != 0)
		psPowerInfo->abAnalog[4] &= (UINT8)~MCB_AP_ADR;
	if (McResCtrl_HasSrc(eMCDRV_DST_ADC1, eMCDRV_DST_CH0) != 0)
		psPowerInfo->abAnalog[4] &= (UINT8)~MCB_AP_ADM;

	if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_M_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_LINEIN1_R_ON) != 0))
		psPowerInfo->abAnalog[4] &= (UINT8)~MCB_AP_LI;

	if ((psPowerInfo->abAnalog[1] != MCI_AP_DA0_DEF)
	|| (psPowerInfo->abAnalog[2] != MCI_AP_DA1_DEF)
	|| (psPowerInfo->abAnalog[3] != MCI_AP_MIC_DEF)
	|| (psPowerInfo->abAnalog[4] != MCI_AP_AD_DEF)) {
		psPowerInfo->abAnalog[0] &=
			(UINT8)~(MCB_AP_LDOA|MCB_AP_BGR|MCB_AP_VR);
		psPowerInfo->bDigital	&= (UINT8)~MCDRV_POWINFO_D_PLL_PD;
	}

	if ((psPowerInfo->bDigital&MCDRV_POWINFO_D_PLL_PD) == 0) {
		;
		psPowerInfo->abAnalog[0] &= (UINT8)~(MCB_AP_LDOD|MCB_AP_BGR);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetPowerInfo", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetPowerInfoRegAccess
 *
 *	Description:
 *			Get power information to access register.
 *	Arguments:
 *			psRegInfo	register information
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPowerInfoRegAccess(
	const struct MCDRV_REG_INFO	*psRegInfo,
	struct MCDRV_POWER_INFO	*psPowerInfo
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetPowerInfoRegAccess");
#endif


	McResCtrl_GetPowerInfo(psPowerInfo);
	switch (psRegInfo->bRegType) {
	case	MCDRV_REGTYPE_IF:
	case	MCDRV_REGTYPE_A:
	case	MCDRV_REGTYPE_MA:
	case	MCDRV_REGTYPE_MB:
	case	MCDRV_REGTYPE_B:
	case	MCDRV_REGTYPE_E:
	case	MCDRV_REGTYPE_C:
	case	MCDRV_REGTYPE_F:
	case	MCDRV_REGTYPE_ANA:
	case	MCDRV_REGTYPE_CD:
	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetPowerInfoRegAccess", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_GetCurPowerInfo
 *
 *	Description:
 *			Get current power setting.
 *	Arguments:
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetCurPowerInfo(
	struct MCDRV_POWER_INFO	*psPowerInfo
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_GetCurPowerInfo");
#endif


	psPowerInfo->bDigital	= 0;
	bReg	= gsGlobalInfo.abRegValA[MCI_PD];
	if ((bReg & MCB_PLL_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PLL_PD;
	if ((bReg & MCB_PE_CLK_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PE_CLK_PD;
	if ((bReg & MCB_PB_CLK_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PB_CLK_PD;
	if ((bReg & MCB_PM_CLK_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PM_CLK_PD;
	if ((bReg & MCB_PF_CLK_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PF_CLK_PD;
	if ((bReg & MCB_PC_CLK_PD) != 0)
		psPowerInfo->bDigital	|= MCDRV_POWINFO_D_PC_CLK_PD;

	psPowerInfo->abAnalog[0]	= gsGlobalInfo.abRegValANA[MCI_AP];
	psPowerInfo->abAnalog[1]	= gsGlobalInfo.abRegValANA[MCI_AP_DA0];
	psPowerInfo->abAnalog[2]	= gsGlobalInfo.abRegValANA[MCI_AP_DA1];
	psPowerInfo->abAnalog[3]	= gsGlobalInfo.abRegValANA[MCI_AP_MIC];
	psPowerInfo->abAnalog[4]	= gsGlobalInfo.abRegValANA[MCI_AP_AD];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetCurPowerInfo", 0);
#endif
}

/****************************************************************************
 *	GetD1Source
 *
 *	Description:
 *			Get digital source information.
 *	Arguments:
 *			pasDChannel	digital source setting
 *			eCh		channel
 *	Return:
 *			path source
 *
 ****************************************************************************/
static UINT32	GetD1Source(
	struct MCDRV_D1_CHANNEL	*pasD,
	enum MCDRV_DST_CH	eCh
)
{
	UINT32	dSrc	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetD1Source");
#endif

	if (pasD != NULL) {
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_MUSICIN_ON) != 0)
			dSrc	|= MCDRV_D1SRC_MUSICIN_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_EXTIN_ON) != 0)
			dSrc	|= MCDRV_D1SRC_EXTIN_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_VBOXOUT_ON) != 0)
			dSrc	|= MCDRV_D1SRC_VBOXOUT_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
			dSrc	|= MCDRV_D1SRC_VBOXREFOUT_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_AE0_ON) != 0)
			dSrc	|= MCDRV_D1SRC_AE0_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_AE1_ON) != 0)
			dSrc	|= MCDRV_D1SRC_AE1_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_AE2_ON) != 0)
			dSrc	|= MCDRV_D1SRC_AE2_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_AE3_ON) != 0)
			dSrc	|= MCDRV_D1SRC_AE3_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_ADIF0_ON) != 0)
			dSrc	|= MCDRV_D1SRC_ADIF0_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_ADIF1_ON) != 0)
			dSrc	|= MCDRV_D1SRC_ADIF1_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_ADIF2_ON) != 0)
			dSrc	|= MCDRV_D1SRC_ADIF2_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D1SRC_HIFIIN_ON) != 0)
			dSrc	|= MCDRV_D1SRC_HIFIIN_ON;
	}
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= dSrc;
	McDebugLog_FuncOut("GetD1Source", &sdRet);
#endif

	return dSrc;
}

/****************************************************************************
 *	GetD2Source
 *
 *	Description:
 *			Get digital source information.
 *	Arguments:
 *			pasDChannel	digital source setting
 *			eCh		channel
 *	Return:
 *			path source
 *
 ****************************************************************************/
static UINT32	GetD2Source(
	struct MCDRV_D2_CHANNEL	*pasD,
	enum MCDRV_DST_CH	eCh
)
{
	UINT32	dSrc	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetD2Source");
#endif

	if (pasD != NULL) {
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_VOICEIN_ON) != 0)
			dSrc	|= MCDRV_D2SRC_VOICEIN_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_VBOXIOOUT_ON) != 0)
			dSrc	|= MCDRV_D2SRC_VBOXIOOUT_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_VBOXHOSTOUT_ON) != 0)
			dSrc	|= MCDRV_D2SRC_VBOXHOSTOUT_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_ADC0_L_ON) != 0)
			dSrc	|= MCDRV_D2SRC_ADC0_L_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_ADC0_R_ON) != 0)
			dSrc	|= MCDRV_D2SRC_ADC0_R_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_ADC1_ON) != 0)
			dSrc	|= MCDRV_D2SRC_ADC1_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_PDM0_L_ON) != 0)
			dSrc	|= MCDRV_D2SRC_PDM0_L_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_PDM0_R_ON) != 0)
			dSrc	|= MCDRV_D2SRC_PDM0_R_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_PDM1_L_ON) != 0)
			dSrc	|= MCDRV_D2SRC_PDM1_L_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_PDM1_R_ON) != 0)
			dSrc	|= MCDRV_D2SRC_PDM1_R_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_DAC0REF_ON) != 0)
			dSrc	|= MCDRV_D2SRC_DAC0REF_ON;
		if ((pasD[eCh].dSrcOnOff & MCDRV_D2SRC_DAC1REF_ON) != 0)
			dSrc	|= MCDRV_D2SRC_DAC1REF_ON;
	}
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= dSrc;
	McDebugLog_FuncOut("GetD2Source", &sdRet);
#endif

	return dSrc;
}

/****************************************************************************
 *	McResCtrl_IsD1SrcUsed
 *
 *	Description:
 *			Is Src used
 *	Arguments:
 *			dSrcOnOff	path src type
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_IsD1SrcUsed(
	UINT32	dSrcOnOff
)
{
	UINT8	bUsed	= 0;
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsD1SrcUsed");
#endif

	for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asMusicOut[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asExtOut[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asVboxMixIn[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < AE_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAe0[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
		if ((gsGlobalInfo.sPathInfo.asAe1[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
		if ((gsGlobalInfo.sPathInfo.asAe2[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
		if ((gsGlobalInfo.sPathInfo.asAe3[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < DAC0_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asDac0[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < DAC1_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asDac1[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bUsed;
	McDebugLog_FuncOut("IsD1SrcUsed", &sdRet);
#endif

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_IsD2SrcUsed
 *
 *	Description:
 *			Is Src used
 *	Arguments:
 *			dSrcOnOff	path src type
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_IsD2SrcUsed(
	UINT32	dSrcOnOff
)
{
	UINT8	bUsed	= 0;
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsD2SrcUsed");
#endif

	for (bCh = 0; bCh < VOICEOUT_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asVoiceOut[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < VBOXIOIN_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asVboxIoIn[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < VBOXHOSTIN_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asVboxHostIn[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < HOSTOUT_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asHostOut[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < ADIF0_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAdif0[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < ADIF1_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAdif1[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < ADIF2_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAdif2[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bUsed;
	McDebugLog_FuncOut("IsD2SrcUsed", &sdRet);
#endif

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_IsASrcUsed
 *
 *	Description:
 *			Is Src used
 *	Arguments:
 *			dSrcOnOff	path src type
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_IsASrcUsed(
	UINT32	dSrcOnOff
)
{
	UINT8	bUsed	= 0;
	UINT8	bCh;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsASrcUsed");
#endif

	for (bCh = 0; bCh < ADC0_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAdc0[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < ADC1_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asAdc1[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < SP_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asSp[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < HP_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asHp[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < RC_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asRc[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < LOUT1_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asLout1[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}
	for (bCh = 0; bCh < LOUT2_PATH_CHANNELS && bUsed == 0; bCh++) {
		if ((gsGlobalInfo.sPathInfo.asLout2[bCh].dSrcOnOff
			& dSrcOnOff) != 0)
			bUsed	= 1;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bUsed;
	McDebugLog_FuncOut("IsASrcUsed", &sdRet);
#endif

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_HasSrc
 *
 *	Description:
 *			Is Destination used
 *	Arguments:
 *			eType	path destination
 *			eCh	channel
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_HasSrc(
	enum MCDRV_DST_TYPE	eType,
	enum MCDRV_DST_CH	eCh
)
{
	UINT8	bUsed	= 0;
	struct MCDRV_PATH_INFO *psPathInfo	= &gsGlobalInfo.sPathInfo;
	UINT32	dSrcOnOff;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_HasSrc");
#endif

	switch (eType) {
	case	eMCDRV_DST_MUSICOUT:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asMusicOut, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_EXTOUT:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asExtOut, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_HIFIOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asHifiOut, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_VBOXMIXIN:
		if (GetD1Source(psPathInfo->asVboxMixIn, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_AE0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asAe0, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_AE1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asAe1, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_AE2:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asAe2, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_AE3:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asAe3, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_DAC0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asDac0, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_DAC1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD1Source(psPathInfo->asDac1, eCh) != 0) {
			;
			bUsed	= 1;
		}
		break;
	case	eMCDRV_DST_VOICEOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asVoiceOut, eCh) != 0)
			bUsed	= 1;
		break;
	case	eMCDRV_DST_VBOXIOIN:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asVboxIoIn, eCh) != 0)
			bUsed	= 1;
		break;
	case	eMCDRV_DST_VBOXHOSTIN:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asVboxHostIn, eCh) != 0)
			bUsed	= 1;
		break;
	case	eMCDRV_DST_HOSTOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asHostOut, eCh) != 0)
			bUsed	= 1;
		break;

	case	eMCDRV_DST_ADIF0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asAdif0, eCh) != 0)
			bUsed	= 1;
		break;

	case	eMCDRV_DST_ADIF1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asAdif1, eCh) != 0)
			bUsed	= 1;
		break;

	case	eMCDRV_DST_ADIF2:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		if (GetD2Source(psPathInfo->asAdif2, eCh) != 0)
			bUsed	= 1;
		break;

	case	eMCDRV_DST_ADC0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asAdc0[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_MIC1_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC2_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC3_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC4_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_R_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_M_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_ADC1:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asAdc1[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_MIC1_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC2_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC3_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC4_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_R_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_LINEIN1_M_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_SP:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asSp[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_DAC1_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_DAC1_R_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_HP:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asHp[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_DAC0_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_DAC0_R_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_RCV:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asRc[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_DAC0_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_DAC0_R_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_LOUT1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asLout1[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_DAC0_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_DAC0_R_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_LOUT2:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asLout2[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_DAC1_L_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_DAC1_R_ON) != 0)) {
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_BIAS:
		if (eCh > eMCDRV_DST_CH3) {
			;
			break;
		}
		dSrcOnOff	= psPathInfo->asBias[eCh].dSrcOnOff;
		if (((dSrcOnOff & MCDRV_ASRC_MIC1_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC2_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC3_ON) != 0)
		|| ((dSrcOnOff & MCDRV_ASRC_MIC4_ON) != 0)) {
			;
			bUsed	= 1;
		}
		break;

	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bUsed;
	McDebugLog_FuncOut("McResCtrl_HasSrc", &sdRet);
#endif

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_GetSource
 *
 *	Description:
 *			Get source information.
 *	Arguments:
 *			eType	path destination
 *			eCh	channel
 *	Return:
 *			path source
 *
 ****************************************************************************/
UINT32	McResCtrl_GetSource(
	enum MCDRV_DST_TYPE	eType,
	enum MCDRV_DST_CH	eCh
)
{
	UINT32	dSrc	= 0;
	struct MCDRV_PATH_INFO *psPathInfo	= &gsGlobalInfo.sPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetSource");
#endif

	switch (eType) {
	case	eMCDRV_DST_MUSICOUT:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asMusicOut, eCh);
		break;
	case	eMCDRV_DST_EXTOUT:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asExtOut, eCh);
		break;
	case	eMCDRV_DST_HIFIOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asHifiOut, eCh);
		break;
	case	eMCDRV_DST_VBOXMIXIN:
		dSrc	= GetD1Source(psPathInfo->asVboxMixIn, eCh);
		break;
	case	eMCDRV_DST_AE0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asAe0, eCh);
		break;
	case	eMCDRV_DST_AE1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asAe1, eCh);
		break;
	case	eMCDRV_DST_AE2:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asAe2, eCh);
		break;
	case	eMCDRV_DST_AE3:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asAe3, eCh);
		break;
	case	eMCDRV_DST_DAC0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asDac0, eCh);
		break;
	case	eMCDRV_DST_DAC1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD1Source(psPathInfo->asDac1, eCh);
		break;
	case	eMCDRV_DST_VOICEOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asVoiceOut, eCh);
		break;
	case	eMCDRV_DST_VBOXIOIN:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asVboxIoIn, eCh);
		break;
	case	eMCDRV_DST_VBOXHOSTIN:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asVboxHostIn, eCh);
		break;
	case	eMCDRV_DST_HOSTOUT:
		if (eCh > eMCDRV_DST_CH0) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asHostOut, eCh);
		break;
	case	eMCDRV_DST_ADIF0:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asAdif0, eCh);
		break;
	case	eMCDRV_DST_ADIF1:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asAdif1, eCh);
		break;
	case	eMCDRV_DST_ADIF2:
		if (eCh > eMCDRV_DST_CH1) {
			;
			break;
		}
		dSrc	= GetD2Source(psPathInfo->asAdif2, eCh);
		break;

	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= dSrc;
	McDebugLog_FuncOut("McResCtrl_GetSource", &sdRet);
#endif

	return dSrc;
}

/****************************************************************************
 *	McResCtrl_GetDspStart
 *
 *	Description:
 *			Get DSP Start/Stop setting.
 *	Arguments:
 *			none
 *	Return:
 *			bit on:DSP Start
 *
 ****************************************************************************/
UINT8	McResCtrl_GetDspStart(
	void
)
{
	UINT8	bStart	= 0;
	UINT32	dHifiSrc;
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetDspStart");
#endif

	dHifiSrc	= gsGlobalInfo.sPathInfo.asHifiOut[0].dSrcOnOff;

	if (((dHifiSrc & MCDRV_D1SRC_ADIF0_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_L_ON|MCDRV_D2SRC_ADC0_R_ON)
		!= 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC1_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)
		!= 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)
		!= 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) != 0)) {
		bStart	|= MCDRV_DSP_START_E;
		bStart	|= MCDRV_DSP_START_M;
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_AE0_ON
				|MCDRV_D1SRC_AE1_ON
				|MCDRV_D1SRC_AE2_ON
				|MCDRV_D1SRC_AE3_ON) != 0) {
		bStart	|= MCDRV_DSP_START_M;
		bStart	|= MCDRV_DSP_START_B;
		if ((gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate == 0)
		&& (gsGlobalInfo.sAecInfo.sAecAudioengine.bFDspOnOff == 1)) {
			bStart	|= MCDRV_DSP_START_F;
			bStart	|= MCDRV_DSP_START_E;
		}
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) != 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) != 0)) {
		bStart	|= MCDRV_DSP_START_M;
		if ((gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff != 0)
		|| (gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff != 0))
			bStart	|= MCDRV_DSP_START_C;
		if ((gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate != 0)
		&& (gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff == 1))
			bStart	|= MCDRV_DSP_START_F;
	} else {
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1)
			!= 0)) {
			bStart	|= MCDRV_DSP_START_M;
			if (gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_VSource == 1
			) {
				if (
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff
					!= 0)
				||
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff
					!= 0))
					bStart	|= MCDRV_DSP_START_C;
				if (
				(gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate
					!= 0)
				&&
				(gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff
					== 1))
					bStart	|= MCDRV_DSP_START_F;
			}
		}
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) != 0) {
			bStart	|= MCDRV_DSP_START_M;
			if (gsGlobalInfo.sAecInfo.sAecVBox.bISrc2_VSource
				== 1) {
				if (
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff
					!= 0)
				||
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff
					!= 0))
					bStart	|= MCDRV_DSP_START_C;
				if (
				(gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate
					!= 0)
				&&
				(gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff
					== 1))
					bStart	|= MCDRV_DSP_START_F;
			}
		}
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXHOSTIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_HOSTOUT, eMCDRV_DST_CH0)
			!= 0)) {
			if (
			(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff != 0)
			||
			(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff != 0))
				bStart	|= MCDRV_DSP_START_C;
			if ((gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate != 0)
			&& (gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff == 1))
				bStart	|= MCDRV_DSP_START_F;
		}
		if (McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0)
			!= 0) {
			if (gsGlobalInfo.sAecInfo.sAecVBox.bLPt2_VSource
				== 1) {
				if (
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncAOnOff
					!= 0)
				||
				(gsGlobalInfo.sAecInfo.sAecVBox.bCDspFuncBOnOff
					!= 0))
					bStart	|= MCDRV_DSP_START_C;
				if (
				(gsGlobalInfo.sAecInfo.sAecConfig.bFDspLocate
					!= 0)
				&&
				(gsGlobalInfo.sAecInfo.sAecVBox.bFDspOnOff
					== 1))
					bStart	|= MCDRV_DSP_START_F;
			}
		}
	}

	if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) != 0))
		bStart	|= MCDRV_DSP_START_M;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bStart;
	McDebugLog_FuncOut("McResCtrl_GetDspStart", &sdRet);
#endif

	return bStart;
}

/****************************************************************************
 *	McResCtrl_GetRegAccess
 *
 *	Description:
 *			Get register access availability
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_REG_ACCSESS
 *
 ****************************************************************************/
enum MCDRV_REG_ACCSESS	McResCtrl_GetRegAccess(
	const struct MCDRV_REG_INFO	*psRegInfo
)
{
	enum MCDRV_REG_ACCSESS	eAccess	= eMCDRV_ACCESS_DENY;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_GetRegAccess");
#endif

	eAccess	= McDevProf_GetRegAccess(psRegInfo);

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= eAccess;
	McDebugLog_FuncOut("McResCtrl_GetRegAccess", &sdRet);
#endif

	return eAccess;
}

/****************************************************************************
 *	McResCtrl_GetAPMode
 *
 *	Description:
 *			get auto power management mode.
 *	Arguments:
 *			none
 *	Return:
 *			eMCDRV_APM_ON
 *			eMCDRV_APM_OFF
 *
 ****************************************************************************/
enum MCDRV_PMODE	McResCtrl_GetAPMode(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= gsGlobalInfo.eAPMode;
	McDebugLog_FuncIn("McResCtrl_GetAPMode");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_GetAPMode", &sdRet);
#endif

	return gsGlobalInfo.eAPMode;
}


/****************************************************************************
 *	McResCtrl_AllocPacketBuf
 *
 *	Description:
 *			allocate the buffer for register setting packets.
 *	Arguments:
 *			none
 *	Return:
 *			pointer to the area to store packets
 *
 ****************************************************************************/
struct MCDRV_PACKET	*McResCtrl_AllocPacketBuf(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_AllocPacketBuf");
#endif

	if (eMCDRV_PACKETBUF_ALLOCATED == gsGlobalInfo.ePacketBufAlloc) {
#if (MCDRV_DEBUG_LEVEL >= 4)
		sdRet	= 0;
		McDebugLog_FuncOut("McResCtrl_AllocPacketBuf", &sdRet);
#endif
		return NULL;
	}

	gsGlobalInfo.ePacketBufAlloc	= eMCDRV_PACKETBUF_ALLOCATED;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_AllocPacketBuf", 0);
#endif
	return gasPacket;
}

/****************************************************************************
 *	McResCtrl_ReleasePacketBuf
 *
 *	Description:
 *			Release the buffer for register setting packets.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_ReleasePacketBuf(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_ReleasePacketBuf");
#endif

	gsGlobalInfo.ePacketBufAlloc = eMCDRV_PACKETBUF_FREE;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_ReleasePacketBuf", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_InitRegUpdate
 *
 *	Description:
 *			Initialize the process of register update.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_InitRegUpdate(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_InitRegUpdate");
#endif

	gsGlobalInfo.sCtrlPacket.wDataNum	= 0;
	gsGlobalInfo.wCurSlaveAddr		= 0xFFFF;
	gsGlobalInfo.wCurRegType		= 0xFFFF;
	gsGlobalInfo.wCurRegAddr		= 0xFFFF;
	gsGlobalInfo.wDataContinueCount		= 0;
	gsGlobalInfo.wPrevAddrIndex		= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_InitRegUpdate", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_AddRegUpdate
 *
 *	Description:
 *			Add register update packet and save register value.
 *	Arguments:
 *			wRegType	register type
 *			wAddress	address
 *			bData		write data
 *			eUpdateMode	updete mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_AddRegUpdate(
	UINT16	wRegType,
	UINT16	wAddr,
	UINT8	bData,
	enum MCDRV_UPDATE_MODE	eUpdateMode
)
{
	UINT8		*pbRegVal;
	UINT8		bAddrADR;
	UINT8		bAddrWINDOW;
	UINT8		bAInc;
	UINT8		bAIncReg;
	UINT8		*pbCtrlData;
	UINT16		*pwCtrlDataNum;
	const UINT16	*pwNextAddr;
	UINT16		wNextAddr;
	UINT16		wSlaveAddr;
	struct MCDRV_REG_INFO	sRegInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_AddRegUpdate");
#endif

	switch (wRegType) {
	case	MCDRV_PACKET_REGTYPE_IF:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValIF;
		pwNextAddr	= gawNextAddr;
		bAInc		= 0;
		bAIncReg	= 0;
		bAddrADR	= (UINT8)wAddr;
		bAddrWINDOW	= bAddrADR;
		sRegInfo.bRegType	= MCDRV_REGTYPE_IF;
		break;

	case	MCDRV_PACKET_REGTYPE_A:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValA;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_A_REG_AINC;
		bAddrADR	= MCI_A_REG_A;
		bAddrWINDOW	= MCI_A_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_A;
		break;

	case	MCDRV_PACKET_REGTYPE_MA:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValMA;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_MA_REG_AINC;
		bAddrADR	= MCI_MA_REG_A;
		bAddrWINDOW	= MCI_MA_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_MA;
		break;

	case	MCDRV_PACKET_REGTYPE_MB:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValMB;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_MB_REG_AINC;
		bAddrADR	= MCI_MB_REG_A;
		bAddrWINDOW	= MCI_MB_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_MB;
		break;

	case	MCDRV_PACKET_REGTYPE_B:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValB;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_B_REG_AINC;
		bAddrADR	= MCI_B_REG_A;
		bAddrWINDOW	= MCI_B_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_B;
		break;

	case	MCDRV_PACKET_REGTYPE_E:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValE;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_E_REG_AINC;
		bAddrADR	= MCI_E_REG_A;
		bAddrWINDOW	= MCI_E_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_E;
		break;

	case	MCDRV_PACKET_REGTYPE_C:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValC;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= 0;
		bAddrADR	= MCI_C_REG_A;
		bAddrWINDOW	= MCI_C_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_C;
		break;

	case	MCDRV_PACKET_REGTYPE_F:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal	= gsGlobalInfo.abRegValF;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_F_REG_AINC;
		bAddrADR	= MCI_F_REG_A;
		bAddrWINDOW	= MCI_F_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_F;
		break;

	case	MCDRV_PACKET_REGTYPE_ANA:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		pbRegVal	= gsGlobalInfo.abRegValANA;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_ANA_REG_AINC;
		bAddrADR	= MCI_ANA_REG_A;
		bAddrWINDOW	= MCI_ANA_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_ANA;
		break;

	case	MCDRV_PACKET_REGTYPE_CD:
		wSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		pbRegVal	= gsGlobalInfo.abRegValCD;
		pwNextAddr	= gawNextAddrAInc;
		bAInc		= 1;
		bAIncReg	= MCB_CD_REG_AINC;
		bAddrADR	= MCI_CD_REG_A;
		bAddrWINDOW	= MCI_CD_REG_D;
		sRegInfo.bRegType	= MCDRV_REGTYPE_CD;
		break;

	default:
#if (MCDRV_DEBUG_LEVEL >= 4)
		McDebugLog_FuncOut("McResCtrl_AddRegUpdate", 0);
#endif
		return;
	}

	sRegInfo.bAddress	= (UINT8)wAddr;
	if ((McResCtrl_GetRegAccess(&sRegInfo) & eMCDRV_CAN_WRITE) == 0) {
#if (MCDRV_DEBUG_LEVEL >= 4)
		McDebugLog_FuncOut("McResCtrl_AddRegUpdate", 0);
#endif
		return;
	}

	if ((gsGlobalInfo.wCurSlaveAddr != 0xFFFF)
	&& (gsGlobalInfo.wCurSlaveAddr != wSlaveAddr)) {
		McResCtrl_ExecuteRegUpdate();
		McResCtrl_InitRegUpdate();
	}

	if ((gsGlobalInfo.wCurRegType != 0xFFFF)
	&& (gsGlobalInfo.wCurRegType != wRegType)) {
		McResCtrl_ExecuteRegUpdate();
		McResCtrl_InitRegUpdate();
	}

	if ((eMCDRV_UPDATE_FORCE == eUpdateMode)
	|| (bData != pbRegVal[wAddr])) {
		if (gsGlobalInfo.wCurRegAddr == 0xFFFF) {
			;
			gsGlobalInfo.wCurRegAddr	= wAddr;
		}

		if (eMCDRV_UPDATE_DUMMY != eUpdateMode) {
			pbCtrlData	= gsGlobalInfo.sCtrlPacket.abData;
			pwCtrlDataNum	= &(gsGlobalInfo.sCtrlPacket.wDataNum);
			wNextAddr	= pwNextAddr[gsGlobalInfo.wCurRegAddr];

			if ((wSlaveAddr == gsGlobalInfo.wCurSlaveAddr)
			&& (wRegType == gsGlobalInfo.wCurRegType)
			&& (0xFFFF != wNextAddr)
			&& (wAddr != wNextAddr)
			&& (bAInc != 0)) {
				if (pwNextAddr[wNextAddr] == wAddr) {
					if (0 ==
					gsGlobalInfo.wDataContinueCount) {
						pbCtrlData[
						gsGlobalInfo.wPrevAddrIndex]
						|= MCDRV_BURST_WRITE_ENABLE;
					}
					pbCtrlData[*pwCtrlDataNum]	=
						pbRegVal[wNextAddr];
					(*pwCtrlDataNum)++;
					gsGlobalInfo.wDataContinueCount++;
					wNextAddr	=
						pwNextAddr[wNextAddr];
				} else if ((0xFFFF != pwNextAddr[wNextAddr])
					&& (pwNextAddr[pwNextAddr[wNextAddr]]
						== wAddr)) {
					if (0 ==
					gsGlobalInfo.wDataContinueCount) {
						pbCtrlData[
						gsGlobalInfo.wPrevAddrIndex]
						|= MCDRV_BURST_WRITE_ENABLE;
					}
					pbCtrlData[*pwCtrlDataNum]	=
						pbRegVal[wNextAddr];
					(*pwCtrlDataNum)++;
					pbCtrlData[*pwCtrlDataNum]	=
					pbRegVal[pwNextAddr[wNextAddr]];
					(*pwCtrlDataNum)++;
					gsGlobalInfo.wDataContinueCount += 2;
					wNextAddr	=
					pwNextAddr[pwNextAddr[wNextAddr]];
				}
			}

			if ((0 == *pwCtrlDataNum) || (wAddr != wNextAddr)) {
				if (0 != gsGlobalInfo.wDataContinueCount) {
					McResCtrl_ExecuteRegUpdate();
					McResCtrl_InitRegUpdate();
				}

				if (MCDRV_PACKET_REGTYPE_IF == wRegType) {
					pbCtrlData[*pwCtrlDataNum]	=
						(bAddrADR << 1);
					gsGlobalInfo.wPrevAddrIndex	=
						*pwCtrlDataNum;
					(*pwCtrlDataNum)++;
				} else {
					pbCtrlData[(*pwCtrlDataNum)++]	=
						(bAddrADR << 1);
					pbCtrlData[(*pwCtrlDataNum)++]	=
						(UINT8)wAddr|bAIncReg;
					pbCtrlData[*pwCtrlDataNum]	=
						(bAddrWINDOW << 1);
					gsGlobalInfo.wPrevAddrIndex	=
						(*pwCtrlDataNum)++;
				}
			} else {
				if (0 == gsGlobalInfo.wDataContinueCount) {
					pbCtrlData[gsGlobalInfo.wPrevAddrIndex]
						|= MCDRV_BURST_WRITE_ENABLE;
				}
				gsGlobalInfo.wDataContinueCount++;
			}

			pbCtrlData[(*pwCtrlDataNum)++]	= bData;

			gsGlobalInfo.wCurSlaveAddr	= wSlaveAddr;
			gsGlobalInfo.wCurRegType	= wRegType;
			gsGlobalInfo.wCurRegAddr	= wAddr;
		}

		/* save register value */
		pbRegVal[wAddr] = bData;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_AddRegUpdate", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_ExecuteRegUpdate
 *
 *	Description:
 *			Add register update packet and save register value.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_ExecuteRegUpdate(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_ExecuteRegUpdate");
#endif

	if (0 != gsGlobalInfo.sCtrlPacket.wDataNum) {
		McSrv_WriteReg((UINT8)gsGlobalInfo.wCurSlaveAddr,
			gsGlobalInfo.sCtrlPacket.abData,
			gsGlobalInfo.sCtrlPacket.wDataNum);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_ExecuteRegUpdate", 0);
#endif
}

/****************************************************************************
 *	McResCtrl_WaitEvent
 *
 *	Description:
 *			Wait event.
 *	Arguments:
 *			dEvent	event to wait
 *			dParam	event parameter
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McResCtrl_WaitEvent(
	UINT32	dEvent,
	UINT32	dParam
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dInterval;
	UINT32	dTimeOut;
	UINT8	bSlaveAddr;
	UINT8	abWriteData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McResCtrl_WaitEvent");
#endif


	switch (dEvent) {
	case	MCDRV_EVT_SVOL_DONE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[0];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[0];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		if ((dParam>>8) != (UINT32)0) {
			abWriteData[0]	= MCI_ANA_REG_A<<1;
			abWriteData[1]	= MCI_BUSY1;
			McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_ANA_REG_D,
					(UINT8)(dParam>>8),
					dInterval, dTimeOut);
			if (MCDRV_SUCCESS != sdRet)
				break;
		}
		if ((dParam&(UINT32)0xFF) != (UINT32)0) {
			abWriteData[0]	= MCI_ANA_REG_A<<1;
			abWriteData[1]	= MCI_BUSY2;
			McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_ANA_REG_D,
					(UINT8)(dParam&(UINT8)0xFF),
					dInterval, dTimeOut);
		}
		break;

	case	MCDRV_EVT_ALLMUTE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[1];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[1];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_MA_REG_A<<1;
		abWriteData[1]	= MCI_DIFI_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(MCB_DIFI3_VFLAG1|MCB_DIFI3_VFLAG0
					|MCB_DIFI2_VFLAG1|MCB_DIFI2_VFLAG0
					|MCB_DIFI1_VFLAG1|MCB_DIFI1_VFLAG0
					|MCB_DIFI1_VFLAG0|MCB_DIFI0_VFLAG0),
					dInterval, dTimeOut);
		if (MCDRV_SUCCESS != sdRet)
			break;
		abWriteData[1]	= MCI_ADI_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(MCB_ADI2_VFLAG1|MCB_ADI2_VFLAG0
					|MCB_ADI1_VFLAG1|MCB_ADI1_VFLAG0
					|MCB_ADI0_VFLAG1|MCB_ADI0_VFLAG0),
					dInterval, dTimeOut);
		if (MCDRV_SUCCESS != sdRet)
			break;
		abWriteData[1]	= MCI_DIFO_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(MCB_DIFO3_VFLAG1|MCB_DIFO3_VFLAG0
					|MCB_DIFO2_VFLAG1|MCB_DIFO2_VFLAG0
					|MCB_DIFO1_VFLAG1|MCB_DIFO1_VFLAG0
					|MCB_DIFO1_VFLAG0|MCB_DIFO0_VFLAG0),
					dInterval, dTimeOut);
		if (MCDRV_SUCCESS != sdRet)
			break;
		abWriteData[1]	= MCI_DAO_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(MCB_DAO1_VFLAG1|MCB_DAO1_VFLAG0
					|MCB_DAO0_VFLAG1|MCB_DAO0_VFLAG0),
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_DIRMUTE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[1];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[1];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_MA_REG_A<<1;
		abWriteData[1]	= MCI_DIFI_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(UINT8)dParam,
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_ADCMUTE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[1];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[1];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_MA_REG_A<<1;
		abWriteData[1]	= MCI_ADI_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(UINT8)dParam,
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_DITMUTE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[1];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[1];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_MA_REG_A<<1;
		abWriteData[1]	= MCI_DIFO_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(UINT8)dParam,
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_DACMUTE:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[1];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[1];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_MA_REG_A<<1;
		abWriteData[1]	= MCI_DAO_VFLAG;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_MA_REG_D,
					(UINT8)dParam,
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_CLKBUSY_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[2];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[2];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_A_REG_A<<1;
		abWriteData[1]	= MCI_CLKSRC;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
					(UINT16)MCI_A_REG_D,
					MCB_CLKBUSY,
					dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_PSW_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[4];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[4];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitBitRelease(bSlaveAddr,
				(UINT16)(dParam>>8),
				(UINT8)(dParam&(UINT8)0xFF),
				dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_OFFCAN_BSY_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		abWriteData[0]	= MCI_CD_REG_A<<1;
		abWriteData[1]	= MCI_SSENSEFIN;
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr,
				(UINT16)MCI_CD_REG_D,
				MCB_OFFCAN_BSY,
				dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_ANA_RDY:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[5];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[5];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		abWriteData[0]	= MCI_ANA_REG_A<<1;
		abWriteData[1]	= (UINT8)(dParam>>8);
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitSet(bSlaveAddr,
				(UINT16)MCI_ANA_REG_D,
				(UINT8)dParam&0xFF,
				dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_AP_CP_A_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[5];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[5];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		abWriteData[0]	= MCI_ANA_REG_A<<1;
		abWriteData[1]	= (UINT8)(dParam>>8);
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitSet(bSlaveAddr,
				(UINT16)MCI_ANA_REG_D,
				(UINT8)dParam&0xFF,
				dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_IF_REG_FLAG_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitBitSet(bSlaveAddr,
				(UINT16)(dParam>>8),
				(UINT8)(dParam&(UINT8)0xFF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_IF_REG_FLAG_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitBitRelease(bSlaveAddr,
				(UINT16)(dParam>>8),
				(UINT8)(dParam&(UINT8)0xFF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_B_REG_FLAG_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitSet(bSlaveAddr,
				MCI_B_REG_A, MCI_B_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_B_REG_FLAG_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitRelease(bSlaveAddr,
				MCI_B_REG_A, MCI_B_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_E_REG_FLAG_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitSet(bSlaveAddr,
				MCI_E_REG_A, MCI_E_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_E_REG_FLAG_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitRelease(bSlaveAddr,
				MCI_E_REG_A, MCI_E_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_C_REG_FLAG_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitSet(bSlaveAddr,
				MCI_C_REG_A, MCI_C_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_C_REG_FLAG_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitRelease(bSlaveAddr,
				MCI_C_REG_A, MCI_C_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_F_REG_FLAG_SET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitSet(bSlaveAddr,
				MCI_F_REG_A, MCI_F_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;
	case	MCDRV_EVT_F_REG_FLAG_RESET:
		dInterval	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollInterval[3];
		dTimeOut	=
			gsGlobalInfo.sInitInfo.sWaitTime.dPollTimeOut[3];
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		sdRet	= WaitDSPBitRelease(bSlaveAddr,
				MCI_F_REG_A, MCI_F_REG_D,
				(UINT16)(dParam>>8), (UINT8)(dParam&0x00FF),
				dInterval, dTimeOut);
		break;

	default:
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McResCtrl_WaitEvent", &sdRet);
#endif

	if(sdRet < 0)
		printk("WaitEvent error dEvent = %lX, dParam = %lX\n", dEvent, dParam);
	
	return sdRet;
}

/****************************************************************************
 *	WaitBitSet
 *
 *	Description:
 *			Wait register bit to set.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitBitSet(
	UINT8	bSlaveAddr,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("WaitBitSet");
#endif


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	while (dCycles < dTimeOut) {
		bData	= McSrv_ReadReg(bSlaveAddr, wRegAddr);
		if ((bData & bBit) == bBit) {
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("WaitBitSet", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	WaitBitRelease
 *
 *	Description:
 *			Wait register bit to release.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitBitRelease(
	UINT8	bSlaveAddr,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("WaitBitRelease");
#endif


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	while (dCycles < dTimeOut) {
		bData	= McSrv_ReadReg(bSlaveAddr, wRegAddr);
		if (0 == (bData & bBit)) {
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("WaitBitRelease", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	WaitDSPBitSet
 *
 *	Description:
 *			Wait DSP register bit to set.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr_A	IF register A address
 *			wRegAddr_D	IF register D address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitDSPBitSet(
	UINT8	bSlaveAddr,
	UINT8	wRegAddr_A,
	UINT8	wRegAddr_D,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;
	UINT8	abWriteData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("WaitDSPBitSet");
#endif


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	abWriteData[0]	= wRegAddr_A<<1;
	abWriteData[1]	= (UINT8)wRegAddr;

	while (dCycles < dTimeOut) {
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		bData	= McSrv_ReadReg(bSlaveAddr, wRegAddr_D);
		if ((bData & bBit) == bBit) {
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("WaitDSPBitSet", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	WaitBitDSPRelease
 *
 *	Description:
 *			Wait DSP register bit to release.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr_A	IF register A address
 *			wRegAddr_D	IF register D address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitDSPBitRelease(
	UINT8	bSlaveAddr,
	UINT8	wRegAddr_A,
	UINT8	wRegAddr_D,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;
	UINT8	abWriteData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("WaitCDSPBitRelease");
#endif


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	abWriteData[0]	= wRegAddr_A<<1;
	abWriteData[1]	= (UINT8)wRegAddr;

	while (dCycles < dTimeOut) {
		McSrv_WriteReg(bSlaveAddr, abWriteData, 2);
		bData	= McSrv_ReadReg(bSlaveAddr, wRegAddr_D);
		if (0 == (bData & bBit)) {
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("WaitCDSPBitRelease", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McResCtrl_IsEnableIRQ
 *
 *	Description:
 *			Is enable IRQ.
 *	Arguments:
 *			none
 *	Return:
 *			0:Disable/1:Enable
 *
 ****************************************************************************/
UINT8	McResCtrl_IsEnableIRQ(
	void
)
{
	UINT8	bRet	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McResCtrl_IsEnableIRQ");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("McResCtrl_IsEnableIRQ", &sdRet);
#endif
	return bRet;
}

