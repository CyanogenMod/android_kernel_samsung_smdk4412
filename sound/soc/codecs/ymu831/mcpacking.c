/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcpacking.c
 *
 *	Description	: MC Driver packet packing control
 *
 *	Version		: 1.0.6	2013.01.25
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


#include "mcpacking.h"
#include "mcdevif.h"
#include "mcresctrl.h"
#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcservice.h"
#include "mcmachdep.h"
#include "mccdspdrv.h"
#include "mcbdspdrv.h"
#include "mcedspdrv.h"
#include "mcfdspdrv.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif




#define	MCDRV_TCXO_WAIT_TIME		((UINT32)2000)
#define	MCDRV_PLL_WAIT_TIME		((UINT32)2000)
#define	MCDRV_LDO_WAIT_TIME		((UINT32)1000)
#define	MCDRV_VREF_WAIT_TIME_ES1	((UINT32)2000)
#define	MCDRV_OSC_WAIT_TIME		((UINT32)10)
#define	MCDRV_CP_WAIT_TIME		((UINT32)2000)
#define	MCDRV_WAIT_TIME_500US		((UINT32)500)
#define	MCDRV_WAIT_TIME_350US		((UINT32)350)
#define	MCDRV_OFC_WAIT_TIME		(3000UL)
#define	MCDRV_DP_DAC_WAIT_TIME		(21UL)

#define	DSF_PRE_INPUT_ADC_L		(0)
#define	DSF_PRE_INPUT_ADC_R		(1)
#define	DSF_PRE_INPUT_ADC_M		(2)
#define	DSF_PRE_INPUT_PDM0_L		(3)
#define	DSF_PRE_INPUT_PDM0_R		(4)
#define	DSF_PRE_INPUT_PDM1_L		(5)
#define	DSF_PRE_INPUT_PDM1_R		(6)

#define	IN_MIX_DIFI_0			(1<<0)
#define	IN_MIX_DIFI_1			(1<<1)
#define	IN_MIX_DIFI_2			(1<<2)
#define	IN_MIX_DIFI_3			(1<<3)
#define	IN_MIX_ADI_0			(1<<4)
#define	IN_MIX_ADI_1			(1<<5)
#define	IN_MIX_ADI_2			(1<<6)

#define	OUT_MIX_DIFI_0			(1<<0)
#define	OUT_MIX_DIFI_1			(1<<1)
#define	OUT_MIX_DIFI_2			(1<<2)
#define	OUT_MIX_DIFI_3			(1<<3)
#define	OUT_MIX_ADI_0			(1<<4)
#define	OUT_MIX_ADI_1			(1<<5)
#define	OUT_MIX_ADI_2			(1<<6)
#define	OUT_MIX_AEO_0			(1<<7)
#define	OUT_MIX_AEO_1			(1<<8)
#define	OUT_MIX_AEO_2			(1<<9)
#define	OUT_MIX_AEO_3			(1<<10)

#define	DSF_PREINPUT_ADC0_L		(0)
#define	DSF_PREINPUT_ADC0_R		(1<<4)
#define	DSF_PREINPUT_ADC1_L		(2)
#define	DSF_PREINPUT_ADC1_R		(2<<4)
#define	DSF_PREINPUT_PDM0_L		(3)
#define	DSF_PREINPUT_PDM0_R		(4<<4)
#define	DSF_PREINPUT_PDM1_L		(5)
#define	DSF_PREINPUT_PDM1_R		(6<<4)

#define	DTH				(3)
#define	EN_CP_ILMT_N			(1)
#define	HP_IDLE				(0)
#define	HP_IBST				(3)
#define	HP_MIDBST			(1)
#define	OP_DAC_HP			(0x40)
#define	OP_DAC				(0x30)
#define	CP_88OFF			(1)
#define	CD_39				(0x21)
#define	T_CPMODE_OFFCAN_BEFORE		(0)
#define	T_CPMODE_OFFCAN_AFTER		(2)
#define	E_99				(0x9F)
#define	E_100				(0x01)
#define	ANA_114				(0x31)
#define	ANA_115				(0x8A)

static void	AddInitDigtalIO(void);
static void	AddInitGPIO(void);
static SINT32	InitMBlock(void);
static void	SetupEReg(void);
static SINT32	Offsetcancel(void);

static void	AddDIPad(void);
static void	GetDIState(UINT8 bPhysPort,
			UINT8 *pbIsUsedDIR,
			UINT8 *pbIsUsedDIT,
			UINT8 *pbLPort);
static UINT8	GetLPort(UINT8 bPhysPort);
static void	AddPAD(void);
static void	AddSource(void);
static void	AddInMixSource(enum MCDRV_DST_TYPE eDstType,
				UINT8 bRegAddr0,
				UINT8 bRegAddr1,
				UINT8 bSep);
static UINT8	GetPreInput(UINT32 dSrcOnOff);
static UINT8	GetInMixReg(UINT32 dSrcOnOff);
static void	AddOutMixSource(enum MCDRV_DST_TYPE eDstType,
				UINT8 bRegAddr0,
				UINT8 bRegAddr1,
				UINT8 bSep);
static void	AddOut2MixSource(void);
static UINT16	GetOutMixReg(UINT32 dSrcOnOff);
static void	AddMixSet(void);
static UINT8	GetMicMixBit(UINT32 dSrcOnOff);
static void	AddDacStop(void);
static UINT8	GetLP2Start(void);
static UINT8	GetLP2Fs(UINT8 *bThru);
static void	AddDIStart(void);
static void	AddDacStart(void);
static UINT8	GetSPath(void);

static void	AddDigVolPacket(UINT8 bVolL,
				UINT8 bVolR,
				UINT32 dRegType,
				UINT8 bVolLAddr,
				UINT8 bVSEP,
				UINT8 bVolRAddr,
				enum MCDRV_VOLUPDATE_MODE eMode);

static void	AddDIOCommon(enum MCDRV_DIO_PORT_NO ePort);
static void	AddDIODIR(enum MCDRV_DIO_PORT_NO ePort);
static void	AddDIODIT(enum MCDRV_DIO_PORT_NO ePort);

#define	MCDRV_DPB_KEEP	0
#define	MCDRV_DPB_UP	1

static SINT32	DSPCallback(SINT32 sdHd, UINT32 dEvtType, UINT32 dEvtPrm);
static UINT32	GetMaxWait(UINT8 bRegChange);

/****************************************************************************
 *	McPacket_AddInit
 *
 *	Description:
 *			Add initialize packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddInit(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddInit");
#endif


	/*	CD_RST	*/
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_CD_RST,
			MCI_CD_RST_DEF);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_CD_RST,
			0);

	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		bReg	= sInitInfo.bPpdRc<<4
			| sInitInfo.bPpdHp<<3
			| sInitInfo.bPpdSp<<2
			| sInitInfo.bPpdLineOut2<<1
			| sInitInfo.bPpdLineOut1;
	} else {
		bReg	= sInitInfo.bPpdSp<<2;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_PPD,
			bReg);

	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF) {
		bReg	= MCB_APMOFF;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_APM,
				bReg);
	}

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		bReg	= (UINT8)(sInit2Info.bOption[12]<<4);
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_81_91H) {
			;
			bReg	|= 0x06;
		} else {
			bReg	|= (sInit2Info.bOption[13] & 0x0F);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)10,
				bReg);
	}

	bReg	= sInitInfo.bMbSel4<<6
		| sInitInfo.bMbSel3<<4
		| sInitInfo.bMbSel2<<2
		| sInitInfo.bMbSel1;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_MBSEL,
			bReg);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_KDSET,
			sInitInfo.bMbsDisch<<4);

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)15,
				sInit2Info.bOption[11]);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	= (UINT8)(sInitInfo.bNonClip<<7);
		bReg	|= DTH;
	} else {
		bReg	= (UINT8)(sInitInfo.bNonClip<<7);
		bReg	|= sInit2Info.bOption[3];
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_NONCLIP,
			bReg);

	bReg	= sInitInfo.bLineOut2Dif<<5
		| sInitInfo.bLineOut1Dif<<4
		| sInitInfo.bLineIn1Dif;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_DIF,
			bReg);

	bReg	= sInitInfo.bSvolHp<<7
		| sInitInfo.bSvolSp<<6
		| sInitInfo.bSvolRc<<5
		| sInitInfo.bSvolLineOut2<<4
		| sInitInfo.bSvolLineOut1<<3
		| MCB_SVOL_HPDET;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_SVOL,
			bReg);

	bReg	= sInitInfo.bHpHiz<<6
		| sInitInfo.bSpHiz<<4
		| sInitInfo.bRcHiz<<3;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		bReg	|= sInit2Info.bOption[2];
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_HIZ,
			bReg);
	bReg	= sInitInfo.bLineOut2Hiz<<6
		| sInitInfo.bLineOut1Hiz<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_LO_HIZ,
			bReg);

	bReg	= sInitInfo.bMic4Sng<<7
		| sInitInfo.bMic3Sng<<6
		| sInitInfo.bMic2Sng<<5
		| sInitInfo.bMic1Sng<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_MCSNG,
			bReg);

	bReg	= sInitInfo.bZcHp<<7
		| sInitInfo.bZcSp<<6
		| sInitInfo.bZcRc<<5
		| sInitInfo.bZcLineOut2<<4
		| sInitInfo.bZcLineOut1<<3;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_ZCOFF,
			bReg);

	bReg	= sInitInfo.bCpMod;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		bReg	|= (UINT8)(EN_CP_ILMT_N<<2);
	} else {
		bReg	|= MCB_HIP_DISABLE;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_CPMOD,
			bReg);

	McResCtrl_GetAecInfo(&sAecInfo);
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	= (UINT8)(sAecInfo.sOutput.bDng_Release<<4)
			| sAecInfo.sOutput.bDng_Attack;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_ES1,
				bReg);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_HP_ES1,
				sAecInfo.sOutput.bDng_Target[0]);
	} else {
		bReg	= (UINT8)(sAecInfo.sOutput.bDng_Release<<4)
			| sAecInfo.sOutput.bDng_Attack;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG,
				bReg);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_HP,
				sAecInfo.sOutput.bDng_Target[0]);
	}
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		bReg	= sInitInfo.bRbSel<<7;
	} else {
		bReg	= 0;
	}
	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF)
		bReg	|= MCB_OFC_MAN;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_RBSEL,
			bReg);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_STDPLUGSEL,
				sInitInfo.bPlugSel<<7);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)18,
				0x20);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)114,
				ANA_114);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)115,
				ANA_115);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_SCKMSKON_R,
				sInit2Info.bOption[1]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)39,
				sInit2Info.bOption[6]);
	}

	McPacket_AddHSDet();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddInit", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AddInitDigtalIO
 *
 *	Description:
 *			Add ditigal IO initialize packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddInitDigtalIO(
	void
)
{
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddInitDigtalIO");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetAecInfo(&sAecInfo);

	/*	DIO0	*/
	if ((sInitInfo.bPowerMode == MCDRV_POWMODE_CDSPDEBUG)
	|| (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn == 1)) {
		bReg	= 0x01;
	} else {
		bReg	= MCI_DO0_DRV_DEF;
		if (sInitInfo.bDio0SdoHiz == MCDRV_DAHIZ_LOW)
			bReg |= MCB_SDO0_DDR;
		if (sInitInfo.bDio0ClkHiz == MCDRV_DAHIZ_LOW) {
			bReg |= MCB_BCLK0_DDR;
			bReg |= MCB_LRCK0_DDR;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO0_DRV,
			bReg);

	/*	DIO1	*/
	if ((sInitInfo.bPowerMode == MCDRV_POWMODE_CDSPDEBUG)
	|| (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn == 1)) {
		bReg	= 0x22;
	} else {
		bReg	= MCI_DO1_DRV_DEF;
		if (sInitInfo.bDio1SdoHiz == MCDRV_DAHIZ_LOW)
			bReg |= MCB_SDO1_DDR;
		if (sInitInfo.bDio1ClkHiz == MCDRV_DAHIZ_LOW) {
			bReg |= MCB_BCLK1_DDR;
			bReg |= MCB_LRCK1_DDR;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO1_DRV,
			bReg);

	/*	DIO2	*/
	bReg	= MCI_DO2_DRV_DEF;
	if (sInitInfo.bDio2SdoHiz == MCDRV_DAHIZ_LOW)
		bReg |= MCB_SDO2_DDR;
	if (sInitInfo.bDio2ClkHiz == MCDRV_DAHIZ_LOW) {
		bReg |= MCB_BCLK2_DDR;
		bReg |= MCB_LRCK2_DDR;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO2_DRV,
			bReg);

	bReg	= 0;
	bReg	|= sAecInfo.sPdm.bPdm1_Data_Delay<<5;
	bReg	|= sAecInfo.sPdm.bPdm0_Data_Delay<<4;
	if (sInitInfo.bDio0PcmHiz == MCDRV_PCMHIZ_HIZ)
		bReg	|= MCB_PCMOUT0_HIZ;
	if (sInitInfo.bDio1PcmHiz == MCDRV_PCMHIZ_HIZ)
		bReg	|= MCB_PCMOUT1_HIZ;
	if (sInitInfo.bDio2PcmHiz == MCDRV_PCMHIZ_HIZ)
		bReg	|= MCB_PCMOUT2_HIZ;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PCMOUT_HIZ,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddInitDigtalIO", NULL);
#endif
}

/****************************************************************************
 *	AddInitGPIO
 *
 *	Description:
 *			Add GPIO initialize packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddInitGPIO(
	void
)
{
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_GP_MODE	sGPMode;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddInitGPIO");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetGPMode(&sGPMode);

	bReg	= MCI_PA0_DEF;
	if (sInitInfo.bPa0Func == MCDRV_PA_PDMCK) {
		bReg	|= MCB_PA0_OUT;
		bReg	|= MCB_PA0_DDR;
	} else if (sInitInfo.bPa0Func == MCDRV_PA_GPIO) {
		if (sGPMode.abGpDdr[MCDRV_GP_PAD0] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA0_DDR;
		else
			bReg	|= MCB_PA0_DDR;
		if (sGPMode.abGpHost[MCDRV_GP_PAD0] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA0_OUTSEL;
		else
			bReg	|= MCB_PA0_OUTSEL;
		if (sGPMode.abGpInvert[MCDRV_GP_PAD0] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA0_INV;
		else
			bReg	|= MCB_PA0_INV;
	}
	if (McResCtrl_GetGPMask(MCDRV_GP_PAD0) == MCDRV_GPMASK_OFF)
		bReg	&= (UINT8)~MCB_PA0_MSK;
	else
		bReg	|= MCB_PA0_MSK;
	if ((sInitInfo.bPa0Func == MCDRV_PA_GPIO)
	&& (sGPMode.abGpDdr[MCDRV_GP_PAD0] == MCDRV_GPDDR_OUT)
	&& (sGPMode.abGpHost[MCDRV_GP_PAD0] == MCDRV_GPHOST_CPU))
		bReg	|= McResCtrl_GetGPPad(MCDRV_GP_PAD0)<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PA0,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA1);
	if (sInitInfo.bPa1Func == MCDRV_PA_GPIO) {
		if (sGPMode.abGpDdr[MCDRV_GP_PAD1] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA1_DDR;
		else
			bReg	|= MCB_PA1_DDR;
		if (sGPMode.abGpHost[MCDRV_GP_PAD1] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA1_OUTSEL;
		else
			bReg	|= MCB_PA1_OUTSEL;
		if (sGPMode.abGpInvert[MCDRV_GP_PAD1] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA1_INV;
		else
			bReg	|= MCB_PA1_INV;
	}
	if (McResCtrl_GetGPMask(MCDRV_GP_PAD1) == MCDRV_GPMASK_OFF)
		bReg	&= (UINT8)~MCB_PA1_MSK;
	else
		bReg	|= MCB_PA1_MSK;
	if ((sInitInfo.bPa1Func == MCDRV_PA_GPIO)
	&& (sGPMode.abGpDdr[MCDRV_GP_PAD1] == MCDRV_GPDDR_OUT)
	&& (sGPMode.abGpHost[MCDRV_GP_PAD1] == MCDRV_GPHOST_CPU))
		bReg	|= McResCtrl_GetGPPad(MCDRV_GP_PAD1)<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PA1,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA2);
	if (sInitInfo.bPa2Func == MCDRV_PA_GPIO) {
		if (sGPMode.abGpDdr[MCDRV_GP_PAD2] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA2_DDR;
		else
			bReg	|= MCB_PA2_DDR;
		if (sGPMode.abGpHost[MCDRV_GP_PAD2] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA2_OUTSEL;
		else
			bReg	|= MCB_PA2_OUTSEL;
		if (sGPMode.abGpInvert[MCDRV_GP_PAD2] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA2_INV;
		else
			bReg	|= MCB_PA2_INV;
	}
	if (McResCtrl_GetGPMask(MCDRV_GP_PAD2) == MCDRV_GPMASK_OFF)
		bReg	&= (UINT8)~MCB_PA2_MSK;
	else
		bReg	|= MCB_PA2_MSK;
	if ((sInitInfo.bPa2Func == MCDRV_PA_GPIO)
	&& (sGPMode.abGpDdr[MCDRV_GP_PAD2] == MCDRV_GPDDR_OUT)
	&& (sGPMode.abGpHost[MCDRV_GP_PAD2] == MCDRV_GPHOST_CPU))
		bReg	|= McResCtrl_GetGPPad(MCDRV_GP_PAD2)<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PA2,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddInitGPIO", NULL);
#endif
}

/****************************************************************************
 *	InitMBlock
 *
 *	Description:
 *			Initialize M Block.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static SINT32	InitMBlock(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dUpdateFlg	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("InitMBlock");
#endif

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_CLK_SEL,
			McResCtrl_GetClkSel());
	}
	dUpdateFlg	= MCDRV_MUSIC_COM_UPDATE_FLAG
			 |MCDRV_MUSIC_DIR_UPDATE_FLAG
			 |MCDRV_MUSIC_DIT_UPDATE_FLAG
			 |MCDRV_EXT_COM_UPDATE_FLAG
			 |MCDRV_EXT_DIR_UPDATE_FLAG
			 |MCDRV_EXT_DIT_UPDATE_FLAG
			 |MCDRV_VOICE_COM_UPDATE_FLAG
			 |MCDRV_VOICE_DIR_UPDATE_FLAG
			 |MCDRV_VOICE_DIT_UPDATE_FLAG
			 |MCDRV_HIFI_COM_UPDATE_FLAG
			 |MCDRV_HIFI_DIR_UPDATE_FLAG
			 |MCDRV_HIFI_DIT_UPDATE_FLAG;
	McPacket_AddDigitalIO(dUpdateFlg);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet == MCDRV_SUCCESS) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP0_FP,
				MCDRV_PHYSPORT_NONE);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP1_FP,
				MCDRV_PHYSPORT_NONE);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP2_FP,
				MCDRV_PHYSPORT_NONE);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP0_FP,
				MCDRV_PHYSPORT_NONE);
		McPacket_AddDigitalIOPath();
		sdRet	= McDevIf_ExecutePacket();
	}
	if (sdRet == MCDRV_SUCCESS) {
		dUpdateFlg	= MCDRV_SWAP_ADIF0_UPDATE_FLAG
				 |MCDRV_SWAP_ADIF1_UPDATE_FLAG
				 |MCDRV_SWAP_ADIF2_UPDATE_FLAG
				 |MCDRV_SWAP_DAC0_UPDATE_FLAG
				 |MCDRV_SWAP_DAC1_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICIN0_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICIN1_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICIN2_UPDATE_FLAG
				 |MCDRV_SWAP_EXTIN_UPDATE_FLAG
				 |MCDRV_SWAP_VOICEIN_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICOUT0_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICOUT1_UPDATE_FLAG
				 |MCDRV_SWAP_MUSICOUT2_UPDATE_FLAG
				 |MCDRV_SWAP_EXTOUT_UPDATE_FLAG
				 |MCDRV_SWAP_VOICEOUT_UPDATE_FLAG;
		McPacket_AddSwap(dUpdateFlg);
		sdRet	= McDevIf_ExecutePacket();
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("InitMBlock", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	SetupEReg
 *
 *	Description:
 *			Setup E_REG.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetupEReg(
	void
)
{
	struct MCDRV_AEC_INFO	sInfo;
	UINT8	bReg;
	UINT8	abSysEq[2][45];
	UINT8	bNonClip;
	int	i;
	UINT8	bMSB, bLSB;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetupEReg");
#endif
	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	if ((bReg & (MCB_PSW_M|MCB_RST_M)) != 0)
		goto exit;

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	if ((bReg & MCB_PE_CLK_PD) != 0)
		goto exit;

	McResCtrl_GetAecInfo(&sInfo);

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ECLK_SEL,
			McResCtrl_GetEClkSel());
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_LPF_THR);
	bReg	&= (MCB_OSF1_MN|MCB_OSF0_MN|MCB_OSF1_ENB|MCB_OSF0_ENB);
	bReg	|= (UINT8)(sInfo.sOutput.bLpf_Post_Thru[1]<<7);
	bReg	|= (UINT8)(sInfo.sOutput.bLpf_Post_Thru[0]<<6);
	bReg	|= (UINT8)(sInfo.sOutput.bLpf_Pre_Thru[1]<<5);
	bReg	|= (UINT8)(sInfo.sOutput.bLpf_Pre_Thru[0]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_LPF_THR,/*2*/
			bReg);

	bReg	= (UINT8)(sInfo.sOutput.bDcc_Sel[1]<<2)
			| sInfo.sOutput.bDcc_Sel[0];
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DAC_DCC_SEL,/*3*/
			bReg);

	bReg	= (UINT8)(sInfo.sOutput.bPow_Det_Lvl[1]<<6)
		| (UINT8)(sInfo.sOutput.bPow_Det_Lvl[0]<<4)
		| sInfo.sOutput.bSig_Det_Lvl;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DET_LVL,/*4*/
			bReg);

	bReg	= (UINT8)(sInfo.sOutput.bOsf_Sel[1]<<2)
			| sInfo.sOutput.bOsf_Sel[0];
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_OSF_SEL,/*8*/
			bReg);

	for (i = 0; i < MCDRV_AEC_OUTPUT_N; i++) {
		McSrv_MemCopy(sInfo.sOutput.bSys_Eq_Coef_A0[i],
				&abSysEq[i][0],
				3);
		McSrv_MemCopy(sInfo.sOutput.bSys_Eq_Coef_A1[i],
				&abSysEq[i][3],
				3);
		McSrv_MemCopy(sInfo.sOutput.bSys_Eq_Coef_A2[i],
				&abSysEq[i][6],
				3);
		McSrv_MemCopy(sInfo.sOutput.bSys_Eq_Coef_B1[i],
				&abSysEq[i][9],
				3);
		McSrv_MemCopy(sInfo.sOutput.bSys_Eq_Coef_B2[i],
				&abSysEq[i][12],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A0,
				&abSysEq[i][15],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A1,
				&abSysEq[i][18],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_A2,
				&abSysEq[i][21],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B1,
				&abSysEq[i][24],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[0].bCoef_B2,
				&abSysEq[i][27],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A0,
				&abSysEq[i][30],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A1,
				&abSysEq[i][33],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_A2,
				&abSysEq[i][36],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B1,
				&abSysEq[i][39],
				3);
		McSrv_MemCopy(sInfo.sOutput.sSysEqEx[i].sBand[1].bCoef_B2,
				&abSysEq[i][42],
				3);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		bReg	= (UINT8)(sInfo.sOutput.bSys_Eq_Enb[1]<<1)
				| sInfo.sOutput.bSys_Eq_Enb[0];
	} else {
		bReg	= (UINT8)(sInfo.sOutput.bSys_Eq_Enb[1]<<4)
				| sInfo.sOutput.bSys_Eq_Enb[0];
	}
	McEdsp_E1Download(abSysEq[0], abSysEq[1], bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_SYSEQ,/*9*/
			bReg);

	bNonClip	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
						MCI_NONCLIP);
	if ((sInfo.sOutput.bClip_Md[1] & 0x6) != 0)
		bNonClip	|= 1<<7;
	else
		bNonClip	&= 0x7F;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_NONCLIP,
			bNonClip);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_CLIP_MD,/*10*/
			sInfo.sOutput.bClip_Md[1]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_CLIP_ATT,/*11*/
			sInfo.sOutput.bClip_Att[1]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_CLIP_REL,/*12*/
			sInfo.sOutput.bClip_Rel[1]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_CLIP_G,/*13*/
			sInfo.sOutput.bClip_G[1]);

	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_OSF_GAIN0_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_OSF_GAIN0_7_0);
	if ((bMSB != sInfo.sOutput.bOsf_Gain[0][0])
	|| (bLSB != sInfo.sOutput.bOsf_Gain[0][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_OSF_GAIN0_15_8,/*14*/
				sInfo.sOutput.bOsf_Gain[0][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_OSF_GAIN0_7_0,
				sInfo.sOutput.bOsf_Gain[0][1]);
	}
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_OSF_GAIN1_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_OSF_GAIN1_7_0);
	if ((bMSB != sInfo.sOutput.bOsf_Gain[1][0])
	|| (bLSB != sInfo.sOutput.bOsf_Gain[1][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_OSF_GAIN1_15_8,
				sInfo.sOutput.bOsf_Gain[1][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_OSF_GAIN1_7_0,
				sInfo.sOutput.bOsf_Gain[1][1]);
	}

	bReg	= (UINT8)(sInfo.sOutput.bDcl_OnOff[1]<<7)
		| (UINT8)(sInfo.sOutput.bDcl_Gain[1]<<4)
		| (UINT8)(sInfo.sOutput.bDcl_OnOff[0]<<3)
		| sInfo.sOutput.bDcl_Gain[0];
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DCL_GAIN,/*18*/
			bReg);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_DCL0_LMT_14_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_DCL0_LMT_7_0);
	if ((bMSB != sInfo.sOutput.bDcl_Limit[0][0])
	|| (bLSB != sInfo.sOutput.bDcl_Limit[0][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DCL0_LMT_14_8,/*19*/
				sInfo.sOutput.bDcl_Limit[0][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DCL0_LMT_7_0,
				sInfo.sOutput.bDcl_Limit[0][1]);
	}

	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_DCL1_LMT_14_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_DCL1_LMT_7_0);
	if ((bMSB != sInfo.sOutput.bDcl_Limit[1][0])
	|| (bLSB != sInfo.sOutput.bDcl_Limit[1][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DCL1_LMT_14_8,/*21*/
				sInfo.sOutput.bDcl_Limit[1][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DCL1_LMT_7_0,
				sInfo.sOutput.bDcl_Limit[1][1]);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	= (UINT8)(sInfo.sOutput.bDc_Dither_Level[0]<<4)
			| (UINT8)(sInfo.sOutput.bRandom_Dither_OnOff[0]<<3)
			| (UINT8)(sInfo.sOutput.bRandom_Dither_POS[0]<<2)
			| sInfo.sOutput.bRandom_Dither_Level[0];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER0,/*23*/
				bReg);

		bReg	= (UINT8)(sInfo.sOutput.bDc_Dither_Level[1]<<4)
			| (UINT8)(sInfo.sOutput.bRandom_Dither_OnOff[1]<<3)
			| (UINT8)(sInfo.sOutput.bRandom_Dither_POS[1]<<2)
			| sInfo.sOutput.bRandom_Dither_Level[1];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER1,
				bReg);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER0,/*23*/
				sInit2Info.bOption[4]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER1,
				sInit2Info.bOption[5]);
	}

	bReg	= (UINT8)(sInfo.sOutput.bDng_Fw[0]<<7)
		| (UINT8)(sInfo.sOutput.bDng_Time[0]<<5)
		| sInfo.sOutput.bDng_Zero[0];
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG0_ES1,/*25*/
				bReg);
		bReg	= (UINT8)(sInfo.sOutput.bDng_Fw[1]<<7)
			| (UINT8)(sInfo.sOutput.bDng_Time[1]<<5)
			| sInfo.sOutput.bDng_Zero[1];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG1_ES1,
				bReg);

		bReg	= (UINT8)(sInfo.sOutput.bDng_On[1]<<1)
				| sInfo.sOutput.bDng_On[0];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG_ON_ES1,/*27*/
				bReg);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG0,/*96*/
				bReg);
		bReg	= (UINT8)(sInfo.sOutput.bDng_Fw[1]<<7)
			| (UINT8)(sInfo.sOutput.bDng_Time[1]<<5)
			| sInfo.sOutput.bDng_Zero[1];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG1,
				bReg);

		bReg	= (UINT8)(sInfo.sOutput.bDng_On[1]<<1)
				| sInfo.sOutput.bDng_On[0];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DNG_ON,/*98*/
				bReg);
	}

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADJ_HOLD,/*35*/
			sInfo.sAdj.bHold);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADJ_CNT,
			sInfo.sAdj.bCnt);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADJ_MAX_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADJ_MAX_7_0);
	if ((bMSB != sInfo.sAdj.bMax[0])
	|| (bLSB != sInfo.sAdj.bMax[1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADJ_MAX_15_8,
				sInfo.sAdj.bMax[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADJ_MAX_7_0,
				sInfo.sAdj.bMax[1]);
	}

	bReg	= (UINT8)(sInfo.sOutput.bDither_Type[1]<<5)
		| (UINT8)(sInfo.sOutput.bDither_Type[0]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)40,/*40*/
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_DSF0_FLT_TYPE);
	bReg	&= (MCB_DSF0_MN|MCB_DSF0ENB);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_R_Type[0]<<6);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_L_Type[0]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF0_FLT_TYPE,/*41*/
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_DSF1_FLT_TYPE);
	bReg	&= (MCB_DSF1_MN|MCB_DSF1ENB);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_R_Type[1]<<6);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_L_Type[1]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF1_FLT_TYPE,/*43*/
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_DSF2_FLT_TYPE);
	bReg	&= (MCB_DSF2REFSEL|MCB_DSF2REFBACK|MCB_DSF2_MN|MCB_DSF2ENB);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_R_Type[2]<<6);
	bReg	|= (UINT8)(sInfo.sInput.bDsf32_L_Type[2]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF2_FLT_TYPE,/*45*/
			bReg);

	bReg	= (UINT8)(sInfo.sInput.bDsf4_Sel[2]<<2)
		| (UINT8)(sInfo.sInput.bDsf4_Sel[1]<<1)
		| (UINT8)(sInfo.sInput.bDsf4_Sel[0]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF_SEL,/*47*/
			bReg);
	bReg	= (UINT8)(sInfo.sInput.bDcc_Sel[2]<<4)
		| (UINT8)(sInfo.sInput.bDcc_Sel[1]<<2)
		| (UINT8)(sInfo.sInput.bDcc_Sel[0]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DCC_SEL,/*48*/
			bReg);

	bReg	= (UINT8)(sInfo.sInput.bDng_On[2]<<2)
		| (UINT8)(sInfo.sInput.bDng_On[1]<<1)
		| (UINT8)(sInfo.sInput.bDng_On[0]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG_ON,/*49*/
			bReg);

	bReg	= (UINT8)(sInfo.sInput.bDng_Fw[0]<<4)
		| (UINT8)(sInfo.sInput.bDng_Rel[0]<<2)
		| (UINT8)(sInfo.sInput.bDng_Att[0]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG0_FW,/*50*/
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG0_TIM,
			sInfo.sInput.bDng_Tim[0]);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG0_ZERO_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG0_ZERO_7_0);
	if ((bMSB != sInfo.sInput.bDng_Zero[0][0])
	|| (bLSB != sInfo.sInput.bDng_Zero[0][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG0_ZERO_15_8,
				sInfo.sInput.bDng_Zero[0][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG0_ZERO_7_0,
				sInfo.sInput.bDng_Zero[0][1]);
	}
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG0_TGT_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG0_TGT_7_0);
	if ((bMSB != sInfo.sInput.bDng_Tgt[0][0])
	|| (bLSB != sInfo.sInput.bDng_Tgt[0][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG0_TGT_15_8,
				sInfo.sInput.bDng_Tgt[0][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG0_TGT_7_0,
				sInfo.sInput.bDng_Tgt[0][1]);
	}

	bReg	= (UINT8)(sInfo.sInput.bDng_Fw[1]<<4)
		| (UINT8)(sInfo.sInput.bDng_Rel[1]<<2)
		| (UINT8)(sInfo.sInput.bDng_Att[1]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG1_FW,/*56*/
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG1_TIM,
			sInfo.sInput.bDng_Tim[1]);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG1_ZERO_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG1_ZERO_7_0);
	if ((bMSB != sInfo.sInput.bDng_Zero[1][0])
	|| (bLSB != sInfo.sInput.bDng_Zero[1][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG1_ZERO_15_8,
				sInfo.sInput.bDng_Zero[1][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG1_ZERO_7_0,
				sInfo.sInput.bDng_Zero[1][1]);
	}
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG1_TGT_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG1_TGT_7_0);
	if ((bMSB != sInfo.sInput.bDng_Tgt[1][0])
	|| (bLSB != sInfo.sInput.bDng_Tgt[1][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG1_TGT_15_8,
				sInfo.sInput.bDng_Tgt[1][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG1_TGT_7_0,
				sInfo.sInput.bDng_Tgt[1][1]);
	}

	bReg	= (UINT8)(sInfo.sInput.bDng_Fw[2]<<4)
		| (UINT8)(sInfo.sInput.bDng_Rel[2]<<2)
		| (UINT8)(sInfo.sInput.bDng_Att[2]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG2_FW,/*62*/
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADC_DNG2_TIM,
			sInfo.sInput.bDng_Tim[2]);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG2_ZERO_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG2_ZERO_7_0);
	if ((bMSB != sInfo.sInput.bDng_Zero[2][0])
	|| (bLSB != sInfo.sInput.bDng_Zero[2][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG2_ZERO_15_8,
				sInfo.sInput.bDng_Zero[2][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG2_ZERO_7_0,
				sInfo.sInput.bDng_Zero[2][1]);
	}
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG2_TGT_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADC_DNG2_TGT_7_0);
	if ((bMSB != sInfo.sInput.bDng_Tgt[2][0])
	|| (bLSB != sInfo.sInput.bDng_Tgt[2][1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG2_TGT_15_8,
				sInfo.sInput.bDng_Tgt[2][0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADC_DNG2_TGT_7_0,
				sInfo.sInput.bDng_Tgt[2][1]);
	}

	bReg	= (UINT8)(sInfo.sInput.bDepop_Wait[0]<<2)
		| (UINT8)(sInfo.sInput.bDepop_Att[0]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DEPOP0,/*68*/
			bReg);
	bReg	= (UINT8)(sInfo.sInput.bDepop_Wait[1]<<2)
		| (UINT8)(sInfo.sInput.bDepop_Att[1]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DEPOP1,
			bReg);
	bReg	= (UINT8)(sInfo.sInput.bDepop_Wait[2]<<2)
		| (UINT8)(sInfo.sInput.bDepop_Att[2]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DEPOP2,
			bReg);

	bReg	= (UINT8)(sInfo.sPdm.bStWait<<2)
		| sInfo.sPdm.bMode;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM_MODE,/*71*/
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_PDM_LOAD_TIM);
	bReg	&= (MCB_PDM1_START|MCB_PDM0_START);
	bReg	|= (UINT8)(sInfo.sPdm.bPdm1_LoadTim<<4);
	bReg	|= sInfo.sPdm.bPdm0_LoadTim;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM_LOAD_TIM,/*72*/
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM0L_FINE_DLY,
			sInfo.sPdm.bPdm0_LFineDly);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM0R_FINE_DLY,
			sInfo.sPdm.bPdm0_RFineDly);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM1L_FINE_DLY,
			sInfo.sPdm.bPdm1_LFineDly);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM1R_FINE_DLY,
			sInfo.sPdm.bPdm1_RFineDly);

	bReg	= 0;
	bReg	|= (UINT8)(sInfo.sEDspMisc.bChSel<<6);
	bReg	|= (UINT8)(sInfo.sEDspMisc.bI2SOut_Enb<<5);
	bReg	|= sInfo.sEDspMisc.bLoopBack;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_CH_SEL,
			bReg);

	bReg	= (sInfo.sE2.bE2_Da_Sel<<3) | sInfo.sE2.bE2_Ad_Sel;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E2_SEL,
			bReg);

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)99,
				E_99);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)100,
				E_100);
	}

exit:;
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetupEReg", NULL);
#endif
}

/****************************************************************************
 *	Offsetcancel
 *
 *	Description:
 *			do offsetcancel.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *			MCDRV_ERROR
 *
 ****************************************************************************/
static SINT32	Offsetcancel(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg,
		bEIRQHS,
		bAP,
		bAPDA0,
		bAPDA1;
	struct MCDRV_AEC_INFO	sAecInfo;
	UINT8	bSlaveAddrA;
	UINT8	abData[2];
	UINT8	bMSB, bLSB;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("Offsetcancel");
#endif
	bSlaveAddrA	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
#ifndef _FOR_FPGA
	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);
	McResCtrl_GetAecInfo(&sAecInfo);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E1DSP_CTRL),
			(UINT8)0x00);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_LPF_THR),
			(UINT8)MCI_LPF_THR_DEF|MCB_OSF1_ENB|MCB_OSF0_ENB);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
			| MCDRV_DP_DAC_WAIT_TIME,
			0);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DP,
			0);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF0_FLT_TYPE),
			(UINT8)MCB_DSF0ENB);

	bEIRQHS	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_IRQHS);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_IRQHS,
			0);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	= (UINT8)(sAecInfo.sOutput.bDc_Dither_Level[0]<<4)
			| (UINT8)(sAecInfo.sOutput.bRandom_Dither_OnOff[0]<<3)
			| (UINT8)(sAecInfo.sOutput.bRandom_Dither_POS[0]<<2)
			| sAecInfo.sOutput.bRandom_Dither_Level[0];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER0,/*23*/
				bReg);
		bReg	= (UINT8)(sAecInfo.sOutput.bDc_Dither_Level[1]<<4)
			| (UINT8)(sAecInfo.sOutput.bRandom_Dither_OnOff[1]<<3)
			| (UINT8)(sAecInfo.sOutput.bRandom_Dither_POS[1]<<2)
			| sAecInfo.sOutput.bRandom_Dither_Level[1];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DITHER1,
				bReg);
	}

	bReg	= (UINT8)(sAecInfo.sOutput.bDither_Type[1]<<5)
		| (UINT8)(sAecInfo.sOutput.bDither_Type[0]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)40,/*40*/
			bReg);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADJ_HOLD,/*35*/
			sAecInfo.sAdj.bHold);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_ADJ_CNT,
			sAecInfo.sAdj.bCnt);
	bMSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADJ_MAX_15_8);
	bLSB	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
			MCI_ADJ_MAX_7_0);
	if ((bMSB != sAecInfo.sAdj.bMax[0])
	|| (bLSB != sAecInfo.sAdj.bMax[1])) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADJ_MAX_15_8,
				sAecInfo.sAdj.bMax[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_ADJ_MAX_7_0,
				sAecInfo.sAdj.bMax[1]);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)22,
				CP_88OFF<<1);

		if ((McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH1) != 0))
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)19,
					OP_DAC_HP);
		else
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)19,
					OP_DAC);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)17,
				(UINT8)(HP_IBST<<1)|HP_MIDBST);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)15,
				(UINT8)(HP_IDLE<<5));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)39,
				CD_39);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)31,
				0xB5);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)30,
				0xD6);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)78,
				T_CPMODE_OFFCAN_BEFORE);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)22,
				CP_88OFF<<1);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)19,
				sInit2Info.bOption[7]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)62,
				sInit2Info.bOption[8]);
	}

	bReg	= E1COMMAND_OFFSET_CANCEL;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E1COMMAND,
			bReg);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		abData[0]	= MCI_ANA_REG_A<<1;
		abData[1]	= 78;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_ANA_REG_D);
		if (bReg != T_CPMODE_OFFCAN_BEFORE) {
			sdRet	= MCDRV_ERROR;
			goto exit;
		}
	}

	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
				| MCDRV_OFC_WAIT_TIME,
				0);

		bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_AP);
		bAPDA0	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_AP_DA0);
		bAPDA1	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_AP_DA1);

		bReg	= bAPDA1;
		bReg	&= (UINT8)~(MCB_AP_SPR1|MCB_AP_SPL1);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA1,
				bReg);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
				|MCDRV_WAIT_TIME_350US,
				0);
		bReg	&= (UINT8)~(MCB_AP_SPR2|MCB_AP_SPL2);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA1,
				bReg);

		bReg	= bAPDA0;
		bReg	&= (UINT8)~MCB_AP_RC;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA0,
				bReg);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_ANA_RDY
				| ((UINT32)MCI_RDY1 << 8)
				| (UINT32)MCB_SPDY_R|MCB_SPDY_L,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_ANA_RDY
				| ((UINT32)MCI_RDY2 << 8)
				| (UINT32)MCB_RCRDY,
				0);

		bReg	= bAP;
		bReg	&= (UINT8)~MCB_AP_HPDET;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP,
				bReg);
		bReg	= bAPDA0;
		bReg	&= (UINT8)~(MCB_AP_HPR
					|MCB_AP_HPL
					|MCB_AP_LO1R
					|MCB_AP_LO1L);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA0,
				bReg);
		bReg	= bAPDA1;
		bReg	&= (UINT8)~(MCB_AP_LO2R|MCB_AP_LO2L);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA1,
				bReg);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_ANA_RDY
				| ((UINT32)MCI_RDY1 << 8)
				| (UINT32)MCB_HPDY_R
				| MCB_HPDY_L
				| MCB_HPDET_RDY,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_ANA_RDY
				| ((UINT32)MCI_RDY2 << 8)
				| (UINT32)MCB_LO2RDY_R
				| MCB_LO2RDY_L
				| MCB_LO1RDY_R
				| MCB_LO1RDY_L,
				0);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP,
				bAP);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA0,
				bAPDA0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA1,
				bAPDA1);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_OFFCAN_BSY_RESET,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_SSENSEFIN,
				MCB_SOFFCANFIN);
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF0_FLT_TYPE),
				0);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQHS,
				bEIRQHS);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)78,
					T_CPMODE_OFFCAN_AFTER);
	}

	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		abData[0]	= MCI_ANA_REG_A<<1;
		abData[1]	= 78;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_ANA_REG_D);
		if (bReg != T_CPMODE_OFFCAN_AFTER) {
			sdRet	= MCDRV_ERROR;
			goto exit;
		}
	}
#endif
exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("Offsetcancel", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPowerUp
 *
 *	Description:
 *			Add powerup packet.
 *	Arguments:
 *			psPowerInfo	power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerUp(
	const struct MCDRV_POWER_INFO	*psPowerInfo,
	const struct MCDRV_POWER_UPDATE	*psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg,
		bRegPD, bCurRegPD,
		bRegRst, bPSW, bRST,
		bAP,
		bRegAPDA0, bRegAPDA1, bCurRegAPDA0, bCurRegAPDA1;
	UINT8	bRegChange;
	UINT8	bDUpdate;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;
	struct MCDRV_CLOCKSW_INFO	sClockSwInfo;
	struct MCDRV_AEC_INFO	sAecInfo;
	UINT32	dWaitTime	= 0;
	UINT8	bOfc	= 0;
	struct MCDRV_FDSP_INIT	stFDSPInit;
	struct MCDRV_CDSP_INIT	stCDSPInit;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddPowerUp");
#endif


	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);
	McResCtrl_GetClockSwInfo(&sClockSwInfo);
	McResCtrl_GetAecInfo(&sAecInfo);

	bRegRst	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	bPSW	= bRegRst & (MCB_PSW_M|MCB_PSW_F|MCB_PSW_C);
	bRST	= bRegRst & (MCB_RST_M|MCB_RST_F|MCB_RST_C);
	bRegPD	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	bCurRegPD	= bRegPD;
	bDUpdate	= ~psPowerInfo->bDigital & psPowerUpdate->bDigital;
	bAP		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	bCurRegAPDA0	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA0);
	bCurRegAPDA1	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA1);
	bRegAPDA0	=
		(UINT8)~(bCurRegAPDA0 & psPowerUpdate->abAnalog[1]);
	bRegAPDA0	|= psPowerInfo->abAnalog[1];
	bRegAPDA0	&= bCurRegAPDA0;
	bRegAPDA1	=
		(UINT8)~(bCurRegAPDA1 & psPowerUpdate->abAnalog[2]);
	bRegAPDA1	|= psPowerInfo->abAnalog[2];
	bRegAPDA1	&= bCurRegAPDA1;

	if (((bDUpdate & MCDRV_POWINFO_D_PLL_PD) != 0UL)
	&& ((bRegPD&MCB_PLL_PD) != 0)) {
		if ((bAP&MCB_AP_LDOD) != 0) {
			bAP	&= (UINT8)~(MCB_AP_LDOD|MCB_AP_BGR);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_AP,
					bAP);

			McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
					| MCDRV_LDO_WAIT_TIME,
					0);
		}

		/*	RSTA	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_RST_A,
				MCI_RST_A_DEF);
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_RST_A,
				MCI_RST_A_DEF&~MCB_RST_A);

		McResCtrl_InitABlockReg();

		/*	JTAGSEL	*/
		if ((sInitInfo.bPowerMode == MCDRV_POWMODE_CDSPDEBUG)
		|| (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn == 1))
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_JTAGSEL,
					MCB_JTAGSEL);

		AddInitDigtalIO();
		AddInitGPIO();

		/*	CKSEL	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_CK_TCX0,
				sInitInfo.bCkSel);
		/*	PLL	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_MODE_A,
				sInitInfo.bPllModeA);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_PREDIV_A,
				sInitInfo.bPllPrevDivA);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FBDIV_A_12_8,
				(UINT8)(sInitInfo.wPllFbDivA>>8));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FBDIV_A_7_0,
				(UINT8)(sInitInfo.wPllFbDivA&0xFF));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FRAC_A_15_8,
				(UINT8)(sInitInfo.wPllFracA>>8));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FRAC_A_7_0,
				(UINT8)(sInitInfo.wPllFracA&0xFF));
		bReg	= (sInitInfo.bPllFreqA<<1);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FOUT_A,
				bReg);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_MODE_B,
				sInitInfo.bPllModeB);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_PREDIV_B,
				sInitInfo.bPllPrevDivB);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FBDIV_B_12_8,
				(UINT8)(sInitInfo.wPllFbDivB>>8));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FBDIV_B_7_0,
				(UINT8)(sInitInfo.wPllFbDivB&0xFF));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FRAC_B_15_8,
				(UINT8)(sInitInfo.wPllFracB>>8));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FRAC_B_7_0,
				(UINT8)(sInitInfo.wPllFracB&0xFF));
		bReg	= (sInitInfo.bPllFreqB<<1);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PLL_FOUT_B,
				bReg);

		bReg	= (UINT8)(McResCtrl_GetFClkSel()<<6)
						| McResCtrl_GetCClkSel();
		if (sClockSwInfo.bClkSrc == MCDRV_CLKSW_CLKA)
			bReg	|= (sInitInfo.bPllFreqA<<4);
		else
			bReg	|= (sInitInfo.bPllFreqB<<4);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_FREQ73M,
				bReg);

		bReg	= sInitInfo.bCkInput;
		if (sClockSwInfo.bClkSrc == MCDRV_CLKSW_CLKB)
			bReg	|= MCB_CLK_INPUT;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_CLKSRC,
				bReg);

		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_DOA_DRV,
					sInit2Info.bOption[0]<<7);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_SCKMSKON_B,
					sInit2Info.bOption[1]);
		}

		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;

		/*	Clock Start	*/
		McSrv_ClockStart();

		bReg	= MCI_CLK_MSK_DEF;
		if (sClockSwInfo.bClkSrc == MCDRV_CLKSW_CLKA) {
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI1)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_RTCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_SBCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI0)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_RTCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_SBCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
		} else {
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI1_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_RTC_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_SBCK_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI0_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_RTC_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_SBCK_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_CLK_MSK,
				bReg);
		if ((bReg & MCB_CLKI0_MSK) == 0) {
			if ((sInitInfo.bCkSel == MCDRV_CKSEL_TCXO_TCXO)
			|| (sInitInfo.bCkSel == MCDRV_CKSEL_TCXO_CMOS)) {
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_TCXO_WAIT_TIME, 0);
			}
		} else if ((bReg & MCB_CLKI1_MSK) == 0) {
			if ((sInitInfo.bCkSel == MCDRV_CKSEL_TCXO_TCXO)
			|| (sInitInfo.bCkSel == MCDRV_CKSEL_CMOS_TCXO)) {
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_TCXO_WAIT_TIME, 0);
			}
		}
		if ((bReg&MCB_RTCI_MSK) == 0) {
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD,
							MCI_CKSEL);
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				;
				bReg	&= (UINT8)~MCB_HSDET;
			} else {
				bReg	&= (UINT8)~MCB_CRTC;
			}
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_CKSEL,
				bReg);
		}

		bRegPD	&= (UINT8)~MCB_PLL_PD;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PD,
				bRegPD);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
				| MCDRV_PLL_WAIT_TIME, 0);
		bRegPD	&= (UINT8)~MCB_VCOOUT_PD;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PD,
				bRegPD);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_IRQ,
				MCB_EIRQ);
		bOfc	= 1;
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PE_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PE_CLK_PD) != 0)) {
		;
		bOfc	= 1;
	}

	/*	ANACLK_PD	*/
	if ((((psPowerUpdate->abAnalog[0] & MCB_AP_LDOA) != 0)
	&& ((psPowerInfo->abAnalog[0] & MCB_AP_LDOA) == 0))
	|| (bOfc == 1)) {
		bRegPD	&= (UINT8)~MCB_ANACLK_PD;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PD,
				bRegPD);
	}

	if (bOfc == 1) {
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			if ((McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH0)
				!= 0)
			|| (McResCtrl_HasSrc(eMCDRV_DST_HP, eMCDRV_DST_CH1)
				!= 0))
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)19,
						OP_DAC_HP);
			else
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)19,
						OP_DAC);
		}
	}

	/*	AP_VR	*/
	if ((bAP & MCB_AP_VR) != 0) {
		if (((((psPowerUpdate->abAnalog[0] & MCB_AP_LDOA) != 0)
		&& ((psPowerInfo->abAnalog[0] & MCB_AP_LDOA) == 0))
		|| (bOfc == 1))) {
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				bAP	&= (UINT8)~MCB_AP_VR;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_VREF_WAIT_TIME_ES1,
						0);
				/*	AP_LDOA/AP_BGR	*/
				bAP	&= (UINT8)~(MCB_AP_LDOA|MCB_AP_BGR);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_LDO_WAIT_TIME,
						0);
			} else {
				/*	AP_BGR	*/
				bAP	&= (UINT8)~(MCB_AP_BGR);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);

				/*	AP_LDOA	*/
				bAP	&= (UINT8)~(MCB_AP_LDOA);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_LDO_WAIT_TIME,
						0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)62,
						sInit2Info.bOption[8]|0x20);
				bAP	&= (UINT8)~MCB_AP_VR;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
					| sInitInfo.sWaitTime.dWaitTime[6],
						0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)62,
						sInit2Info.bOption[8]);
			}
		}
	}

	if ((bOfc == 1)
	|| (((bDUpdate & MCDRV_POWINFO_D_PM_CLK_PD) != 0UL)
		&& ((bRegPD&MCB_PM_CLK_PD) != 0))) {
		bPSW	&= (UINT8)~MCB_PSW_M;
		bRST	&= (UINT8)~MCB_RST_M;
		bRegPD	&= (UINT8)~MCB_PM_CLK_PD;
		McResCtrl_InitMBlockReg();
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PB_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PB_CLK_PD) != 0)) {
		bRegPD	&= (UINT8)~MCB_PB_CLK_PD;
	}

	if ((bOfc == 1)
	|| (((bDUpdate & MCDRV_POWINFO_D_PE_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PE_CLK_PD) != 0))) {
		bRegPD	&= (UINT8)~MCB_PE_CLK_PD;
		McResCtrl_InitEReg();
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PF_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PF_CLK_PD) != 0)) {
		bPSW	&= (UINT8)~MCB_PSW_F;
		bRST	&= (UINT8)~MCB_RST_F;
		bRegPD	&= (UINT8)~MCB_PF_CLK_PD;
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PC_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PC_CLK_PD) != 0)) {
		bPSW	&= (UINT8)~MCB_PSW_C;
		bRST	&= (UINT8)~MCB_RST_C;
		bRegPD	&= (UINT8)~MCB_PC_CLK_PD;
	}

	bRegRst &= (UINT8)~(MCB_PSW_M|MCB_PSW_F|MCB_PSW_C);
	bRegRst	|= bPSW;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_RST,
			bRegRst);
	if ((bAP&MCB_AP_LDOD) == 0)
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_PSW_RESET
				| (UINT32)MCI_RST<<8
				| (~bPSW&(MCB_PSW_M|MCB_PSW_F|MCB_PSW_C)),
				0);
	bRegRst	&= (UINT8)~(MCB_RST_M|MCB_RST_F|MCB_RST_C);
	bRegRst	|= bRST;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_RST,
			bRegRst);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PD,
			bRegPD);

	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;
	if (((bDUpdate & MCDRV_POWINFO_D_PM_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PM_CLK_PD) != 0)
	&& ((bRegPD&MCB_PM_CLK_PD) == 0))
		InitMBlock();

	if (((bDUpdate & MCDRV_POWINFO_D_PB_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PB_CLK_PD) != 0)
	&& ((bRegPD&MCB_PB_CLK_PD) == 0)) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McBdsp_Init();
	}

	if ((bOfc != 0)
	|| (((bDUpdate & MCDRV_POWINFO_D_PE_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PE_CLK_PD) != 0)
	&& ((bRegPD&MCB_PE_CLK_PD) == 0))) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McEdsp_Init();
		McEdsp_SetCBFunc(DSPCallback);
		SetupEReg();
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		if (bOfc != 0) {
			sdRet	= Offsetcancel();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			SetupEReg();
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
		}
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PF_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PF_CLK_PD) != 0)
	&& ((bRegPD&MCB_PF_CLK_PD) == 0)) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		if (sAecInfo.sAecConfig.bFDspLocate == 0) {
			stFDSPInit.sInput.wADDFmt	= 0xF0FF;
			stFDSPInit.sOutput.wADDFmt	= 0xF0FF;
		} else {
			stFDSPInit.sInput.wADDFmt	= 0xF000;
			stFDSPInit.sOutput.wADDFmt	= 0xF000;
		}
		stFDSPInit.dWaitTime	= sInitInfo.sWaitTime.dWaitTime[5];
		McFdsp_Init(&stFDSPInit);
		McFdsp_SetCBFunc(DSPCallback);
		bReg	= MCB_IESERR
			| MCB_IEAMTBEG
			| MCB_IEAMTEND
			| MCB_IEFW;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_IESERR,
				bReg);
	}

	if (((bDUpdate & MCDRV_POWINFO_D_PC_CLK_PD) != 0UL)
	&& ((bCurRegPD&MCB_PC_CLK_PD) != 0)
	&& ((bRegPD&MCB_PC_CLK_PD) == 0)) {
		/*	JTAGSEL	*/
		if (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn == 1) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_JTAGSEL,
					MCB_JTAGSEL);
		}
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		stCDSPInit.bJtag	= sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn;
		/*	JTAGSEL	*/
		if (sInitInfo.bPowerMode == MCDRV_POWMODE_CDSPDEBUG) {
			;
			stCDSPInit.bJtag	= 1;
		}
		McCdsp_Init(&stCDSPInit);
		McCdsp_SetCBFunc(eMC_PLAYER_CODER_A, DSPCallback);
		McCdsp_SetCBFunc(eMC_PLAYER_CODER_B, DSPCallback);
		bReg	= MCB_ECDSP
			| MCB_EFFIFO
			| MCB_ERFIFO
			| MCB_EEFIFO
			| MCB_EOFIFO
			| MCB_EDFIFO
			| MCB_EENC
			| MCB_EDEC;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_ECDSP,
				bReg);
	}

	/*	DP_ADC/DP_DAC*	*/
	AddDacStart();
	if (bOfc == 0) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_DP);
		if ((psPowerUpdate->abAnalog[1] & (MCB_AP_DA0R|MCB_AP_DA0L))
			!= 0) {
			if (((psPowerInfo->abAnalog[1] & MCB_AP_DA0R) == 0)
			|| ((psPowerInfo->abAnalog[1] & MCB_AP_DA0L) == 0)) {
				bReg	&=
				(UINT8)~(MCB_DP_DAC1|MCB_DP_DAC0|
					MCB_DP_PDMCK|MCB_DP_PDMDAC);
				if (McDevProf_GetDevId()
					== eMCDRV_DEV_ID_80_90H) {
					;
					bReg	|= MCB_DP_DAC1;
				}
			}
		}
		if ((psPowerUpdate->abAnalog[1] & (MCB_AP_DA1R|MCB_AP_DA1L))
			!= 0)
			if (((psPowerInfo->abAnalog[2] & MCB_AP_DA1R) == 0)
			|| ((psPowerInfo->abAnalog[2] & MCB_AP_DA1L) == 0))
				bReg	&=
				(UINT8)~(MCB_DP_DAC1|
					MCB_DP_PDMCK|MCB_DP_PDMDAC);
		if ((psPowerUpdate->abAnalog[4] &
			(MCB_AP_ADM|MCB_AP_ADR|MCB_AP_ADL)) != 0)
			if (((psPowerInfo->abAnalog[4] & MCB_AP_ADM) == 0)
			|| ((psPowerInfo->abAnalog[4] & MCB_AP_ADR) == 0)
			|| ((psPowerInfo->abAnalog[4] & MCB_AP_ADL) == 0))
				bReg	&=
				(UINT8)~(MCB_DP_ADC|
					MCB_DP_PDMCK|MCB_DP_PDMADC);
		if ((bRegAPDA1&
			(MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1))
			!= (MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1))
			bReg	&= (UINT8)~(MCB_DP_ADC|MCB_DP_PDMADC);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_DP,
				bReg);
	}

	/*	AP_MC/AP_MB	*/
	bReg	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_MIC);
	bRegChange	= (UINT8)~(bReg & psPowerUpdate->abAnalog[3]);
	bRegChange	|= psPowerInfo->abAnalog[3];
	bRegChange	&= bReg;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP_MIC,
			bRegChange);
	bRegChange	= ~bRegChange & bReg;
	dWaitTime	= GetMaxWait(bRegChange);

	/*	AP_LI/AP_AD	*/
	bReg	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_AD);
	bRegChange	= (UINT8)~(bReg & psPowerUpdate->abAnalog[4]);
	bRegChange	|= psPowerInfo->abAnalog[4];
	bRegChange	&= bReg;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP_AD,
			bRegChange);
	if (((bReg&MCB_AP_LI) != 0)
	&& ((bRegChange&MCB_AP_LI) == 0))
		dWaitTime	= sInitInfo.sWaitTime.dWaitTime[4];

	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF) {
		if (((psPowerUpdate->abAnalog[0] & MCB_AP_CP) != 0)
		&& ((psPowerInfo->abAnalog[0] & MCB_AP_CP) == 0)) {
			if ((bAP&MCB_AP_CP) != 0) {
				bAP	&= (UINT8)~MCB_AP_CP;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bAP);
				if (McDevProf_GetDevId()
					== eMCDRV_DEV_ID_80_90H) {
					if (dWaitTime > MCDRV_CP_WAIT_TIME) {
						;
						dWaitTime -=
							MCDRV_CP_WAIT_TIME;
					} else {
						dWaitTime	= 0;
					}
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_CP_WAIT_TIME,
						0);
				}
			}
		}
		if (bRegAPDA0 != bCurRegAPDA0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_AP_DA0,
					bRegAPDA0);
			if (dWaitTime < MCDRV_WAIT_TIME_500US)
				dWaitTime	= MCDRV_WAIT_TIME_500US;
		}
		if (bRegAPDA1 != bCurRegAPDA1) {
			if ((bRegAPDA1 &
			(MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1))
			!=
			(bCurRegAPDA1 &
			(MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1))) {
				bReg	= bRegAPDA1 |
				(bCurRegAPDA1&(MCB_AP_SPR2|MCB_AP_SPL2));
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP_DA1,
						bReg);
				McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_TIMWAIT|MCDRV_WAIT_TIME_350US,
				0);
				if (dWaitTime > MCDRV_WAIT_TIME_350US)
					dWaitTime	-=
							MCDRV_WAIT_TIME_350US;
				else
					dWaitTime	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_AP_DA1,
					bRegAPDA1);
			if (dWaitTime < MCDRV_WAIT_TIME_500US)
				dWaitTime	= MCDRV_WAIT_TIME_500US;
		}
	} else {
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA0,
			bRegAPDA0);
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA1,
			bRegAPDA1);
	}
	if (dWaitTime > 0)
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddPowerUp", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPowerDown
 *
 *	Description:
 *			Add powerdown packet.
 *	Arguments:
 *			psPowerInfo	power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerDown(
	const struct MCDRV_POWER_INFO	*psPowerInfo,
	const struct MCDRV_POWER_UPDATE	*psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bUpdate;
	UINT8	bReg;
	UINT8	bRegPD, bRegRst, bPSW = 0, bRST = 0,
		bAP,
		bRegAPDA0, bRegAPDA1;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;
	UINT8	bMKDetEn	= 0;
	UINT8	bAddMKDetEn	= 0;
	UINT8	bPlugDetDB	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddPowerDown");
#endif


	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);

	bUpdate	= psPowerInfo->bDigital & psPowerUpdate->bDigital;
	bRegRst	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	bRegPD	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	bPlugDetDB	= McResCtrl_GetPlugDetDB();

	if (MCDRV_POWMODE_FULL == sInitInfo.bPowerMode)
		if (((bUpdate & MCDRV_POWINFO_D_PLL_PD) != 0UL)
		&& ((bRegPD&(MCB_PLL_PD|MCB_PM_CLK_PD)) == 0))
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_ALLMUTE,
					0);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA0);
	bRegAPDA0	= bReg |
			(psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]);
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_DA1);
	bRegAPDA1	= bReg |
			(psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]);
	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF) {
		/*	AP_DA0/AP_DA1	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA0,
				bRegAPDA0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP_DA1,
				bRegAPDA1);
	} else {
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_AP_DA0, bRegAPDA0);
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_AP_DA1, bRegAPDA1);
	}

	/*	AP_MC/AP_MB	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_MIC);
	bReg	|= (psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP_MIC,
			bReg);

	/*	AP_LI/AP_AD	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP_AD);
	bReg	|= (psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP_AD,
			bReg);

	if ((bUpdate & MCDRV_POWINFO_D_PC_CLK_PD) != 0UL) {
		if ((bRegPD&MCB_PC_CLK_PD) == 0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			McCdsp_Term();
			bRegPD	|= MCB_PC_CLK_PD;
		}
		bRST	|= MCB_RST_C;
		bPSW	|= MCB_PSW_C;
	}

	if ((bUpdate & MCDRV_POWINFO_D_PF_CLK_PD) != 0UL) {
		if ((bRegPD&MCB_PF_CLK_PD) == 0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			McFdsp_Term();
			bRegPD	|= MCB_PF_CLK_PD;
		}
		bRST	|= MCB_RST_F;
		bPSW	|= MCB_PSW_F;
	}

	if (((bUpdate & MCDRV_POWINFO_D_PE_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PE_CLK_PD) == 0)) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McEdsp_Term();
		bRegPD	|= MCB_PE_CLK_PD;
	}

	if (((bUpdate & MCDRV_POWINFO_D_PB_CLK_PD) != 0UL)
	&& ((bRegPD&MCB_PB_CLK_PD) == 0)) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McBdsp_Term();
		bRegPD	|= MCB_PB_CLK_PD;
	}

	if ((bUpdate & MCDRV_POWINFO_D_PM_CLK_PD) != 0UL) {
		if ((bRegPD&MCB_PM_CLK_PD) == 0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_DSP_START,
					0);
			bRegPD	|= MCB_PM_CLK_PD;
		}
		bRST	|= MCB_RST_M;
		bPSW	|= MCB_PSW_M;
	}

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PD,
			bRegPD);
	bRegRst	|= bRST;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_RST,
			bRegRst);
	bRegRst	|= bPSW;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_RST,
			bRegRst);

	/*	Analog Power Save	*/
	bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_HSDETEN);
	if (((psPowerUpdate->abAnalog[0] & MCB_AP_LDOA) != 0)
	&& ((psPowerInfo->abAnalog[0] & MCB_AP_LDOA) != 0)
	&& ((bAP & MCB_AP_LDOA) == 0)) {
		if ((bReg & MCB_MKDETEN) != 0) {
			bMKDetEn	= bReg & MCB_MKDETEN;
			/*	MKDETEN	*/
			bReg	&= (UINT8)~MCB_MKDETEN;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_CD
					| (UINT32)MCI_HSDETEN,
					bReg);
		}
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_AP_CP_A_SET
					| ((UINT32)91 << 8)
					| (UINT32)0x02,
					0);
		}
		/*	ANACLK_PD	*/
		bRegPD	|= MCB_ANACLK_PD;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PD,
				bRegPD);
	}

	/*	DP_ADC/DP_DAC*	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_DP);
	if ((psPowerUpdate->abAnalog[1] & (MCB_AP_DA0R|MCB_AP_DA0L)) != 0)
		if (((psPowerInfo->abAnalog[1] & MCB_AP_DA0R) != 0)
		&& ((psPowerInfo->abAnalog[1] & MCB_AP_DA0L) != 0))
			bReg	|= MCB_DP_DAC0;
	if ((psPowerUpdate->abAnalog[1] & (MCB_AP_DA1R|MCB_AP_DA1L)) != 0) {
		if (((psPowerInfo->abAnalog[2] & MCB_AP_DA1R) != 0)
		&& ((psPowerInfo->abAnalog[2] & MCB_AP_DA1L) != 0)) {
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				bReg	|= MCB_DP_DAC1;
			} else {
				if ((bReg&MCB_DP_DAC0) != 0)
					bReg	|= MCB_DP_DAC1;
			}
		}
	}
	if ((psPowerUpdate->abAnalog[4] & (MCB_AP_ADM|MCB_AP_ADR|MCB_AP_ADL))
		!= 0) {
		;
		if (((psPowerInfo->abAnalog[4] & MCB_AP_ADM) != 0)
		&& ((psPowerInfo->abAnalog[4] & MCB_AP_ADR) != 0)
		&& ((psPowerInfo->abAnalog[4] & MCB_AP_ADL) != 0)
		&& ((bRegAPDA1
			&(MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1))
			== (MCB_AP_SPR2|MCB_AP_SPR1|MCB_AP_SPL2|MCB_AP_SPL1)))
				bReg	|= (MCB_DP_ADC|MCB_DP_PDMADC);
	}
	if ((bReg&(MCB_DP_DAC0|MCB_DP_DAC1)) == (MCB_DP_DAC0|MCB_DP_DAC1)) {
		bReg	|= MCB_DP_PDMDAC;
		if ((bReg&MCB_DP_PDMADC) != 0)
			bReg	= MCI_DP_DEF;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DP,
			bReg);
	AddDacStop();

	if (MCDRV_POWMODE_FULL == sInitInfo.bPowerMode) {
		if (((bUpdate & MCDRV_POWINFO_D_PLL_PD) != 0UL)
		&& ((bRegPD&MCB_PLL_PD) == 0)) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRQ,
					0);
			bRegPD	|= MCB_VCOOUT_PD;
			bRegPD	|= MCB_PLL_PD;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_PD,
					bRegPD);
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
							MCI_CLK_MSK);
			if ((bReg&MCB_RTCI_MSK) == 0) {
				bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_CD,
						MCI_CKSEL);
				if (McDevProf_GetDevId()
					== eMCDRV_DEV_ID_80_90H) {
					;
					bReg	|= MCB_HSDET;
				} else {
					bReg	|= MCB_CRTC;
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_CKSEL,
						bReg);
			}
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			/*	Clock Stop	*/
			McSrv_ClockStop();
			/*	RST_A	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST_A,
					MCI_RST_A_DEF);
			McResCtrl_InitABlockReg();
		}
		if (sInit2Info.bOption[19] == 0) {
			;
			if ((bRegPD&MCB_PLL_PD) != 0) {
				;
				bAP	|= MCB_AP_LDOD;
			}
		}
	}

	if (((psPowerUpdate->abAnalog[0] & MCB_AP_LDOA) != 0)
	&& ((psPowerInfo->abAnalog[0] & MCB_AP_LDOA) != 0)
	&& ((bAP & MCB_AP_LDOA) == 0)) {
		bAddMKDetEn	= bMKDetEn;
		bAP	|= (MCB_AP_LDOA|MCB_AP_VR);
	} else
		bAddMKDetEn	= 0;

	if (((psPowerUpdate->abAnalog[0] & MCB_AP_HPDET) != 0)
	&& ((psPowerInfo->abAnalog[0] & MCB_AP_HPDET) != 0))
		bAP	|= MCB_AP_HPDET;

	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_81_92H) {
		if ((bAP & (MCB_AP_LDOA|MCB_AP_LDOD)) ==
			(MCB_AP_LDOA|MCB_AP_LDOD)
		&& ((bAP & MCB_AP_BGR) == 0)
		&& ((bPlugDetDB&MCB_RPLUGDET_DB) == 0)) {
			if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
				bReg	= sInitInfo.bHpHiz<<6
					| sInitInfo.bSpHiz<<4
					| sInitInfo.bRcHiz<<3
					| sInit2Info.bOption[2];
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_HIZ,
						bReg);
				bReg	= sInitInfo.bLineOut2Hiz<<6
					| sInitInfo.bLineOut1Hiz<<4;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_LO_HIZ,
						bReg);
			}
			bAP	|= MCB_AP_BGR;
		}
	} else {
		if ((bAP & (MCB_AP_LDOA|MCB_AP_LDOD)) ==
						(MCB_AP_LDOA|MCB_AP_LDOD)) {
			bAP	|= MCB_AP_BGR;
			if ((bPlugDetDB&MCB_RPLUGDET_DB) == 0) {
				bReg	= sInitInfo.bHpHiz<<6
					| sInitInfo.bSpHiz<<4
					| sInitInfo.bRcHiz<<3
					| sInit2Info.bOption[2];
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_HIZ,
						bReg);
			}
		}
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	if (McResCtrl_GetAPMode() == eMCDRV_APM_OFF) {
		if (((psPowerUpdate->abAnalog[0] & MCB_AP_CP) != 0)
		&& ((psPowerInfo->abAnalog[0] & MCB_AP_CP) != 0))
			bAP	|= MCB_AP_CP;
		if (((bAP&MCB_AP_CP) != 0)
		&& ((bReg&MCB_AP_CP) == 0)) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_ANA_RDY
					| ((UINT32)MCI_RDY1 << 8)
					| (UINT32)MCB_CPPDRDY,
					0);
			bReg	|= MCB_AP_CP;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_AP,
					bReg);
		}
	}

	if (((bAP&MCB_AP_HPDET) != 0)
	&& ((bReg&MCB_AP_HPDET) == 0)) {
		bReg	|= MCB_AP_HPDET;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP,
				bReg);
	}
	if (((bAP&MCB_AP_LDOA) != 0)
	&& ((bReg&MCB_AP_LDOA) == 0)) {
		bReg	|= MCB_AP_LDOA;
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H)
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_AP,
					bReg);
		bReg	|= MCB_AP_VR;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP,
				bReg);
	}
	if (((bAP&MCB_AP_LDOD) != 0)
	&& ((bReg&MCB_AP_LDOD) == 0)) {
		bReg	|= MCB_AP_LDOD;
		if (((bAP&MCB_AP_BGR) != 0)
		&& ((bReg&MCB_AP_BGR) == 0)
		&& (bMKDetEn == 0))
			bReg	|= MCB_AP_BGR;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_AP,
				bReg);
	}
	if (((bReg&MCB_AP_LDOA) != 0)
	&& ((bReg&MCB_AP_LDOD) != 0)
	&& ((bAP&MCB_AP_BGR) != 0)
	&& ((bReg&MCB_AP_BGR) == 0)
	&& ((bPlugDetDB&MCB_RPLUGDET_DB) == 0))
		bReg	|= MCB_AP_BGR;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP,
			bReg);

	if (bAddMKDetEn != 0) {
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
		McPacket_AddMKDetEnable(0);
	}

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddPowerDown", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPathSet
 *
 *	Description:
 *			Add path update packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddPathSet(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddPathSet");
#endif

	if ((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST_A) &
							MCB_RST_A) == 0) {
		/*	DI Pad	*/
		AddDIPad();
		/*	PAD(PDM)	*/
		AddPAD();
		/*	Digital Mixer Source	*/
		AddSource();
		/*	Analog Mixer Source	*/
		AddMixSet();
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddPathSet", NULL);
#endif
}

/****************************************************************************
 *	AddDIPad
 *
 *	Description:
 *			Add DI Pad setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIPad(
	void
)
{
	UINT8	bReg;
	UINT8	bIsUsedDIR	= 0,
		bIsUsedDIT	= 0;
	UINT8	bLPort;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_PATH_INFO	sPathInfo;
	struct MCDRV_DIO_INFO	sDioInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddDIPad");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetDioInfo(&sDioInfo);
	McResCtrl_GetAecInfo(&sAecInfo);

	bReg	= 0;
	GetDIState(MCDRV_PHYSPORT_DIO0, &bIsUsedDIR, &bIsUsedDIT, &bLPort);
	if (bIsUsedDIR == 0)
		bReg |= MCB_SDIN0_MSK;
	if (bIsUsedDIT == 0) {
		if (sInitInfo.bDio0SdoHiz == MCDRV_DAHIZ_LOW)
			bReg |= MCB_SDO0_DDR;
	} else {
		bReg |= MCB_SDO0_DDR;
	}
	if ((bIsUsedDIR == 0) && (bIsUsedDIT == 0)) {
		bReg |= MCB_BCLK0_MSK;
		bReg |= MCB_LRCK0_MSK;
		if (sInitInfo.bDio0ClkHiz == MCDRV_DAHIZ_LOW) {
			bReg |= MCB_BCLK0_DDR;
			bReg |= MCB_LRCK0_DDR;
		}
	} else {
		if ((bLPort == 3)
		|| ((bLPort <= 2)
			&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bMasterSlave
							== MCDRV_DIO_MASTER))
		) {
			bReg |= MCB_BCLK0_DDR;
			bReg |= MCB_LRCK0_DDR;
		}
	}
	if ((bLPort <= 3)
	&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT))
		bReg	|= MCB_BCLK0_INV;
	if ((sInitInfo.bPowerMode != MCDRV_POWMODE_CDSPDEBUG)
	&& (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn != 1)) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO0_DRV,
			bReg);
	}

	bReg	= 0;
	GetDIState(MCDRV_PHYSPORT_DIO1, &bIsUsedDIR, &bIsUsedDIT, &bLPort);
	if (bIsUsedDIR == 0)
		bReg |= MCB_SDIN1_MSK;
	if (bIsUsedDIT == 0) {
		if (sInitInfo.bDio1SdoHiz == MCDRV_DAHIZ_LOW)
			bReg |= MCB_SDO1_DDR;
	} else {
		bReg |= MCB_SDO1_DDR;
	}
	if ((bIsUsedDIR == 0) && (bIsUsedDIT == 0)) {
		bReg |= MCB_BCLK1_MSK;
		bReg |= MCB_LRCK1_MSK;
		if (sInitInfo.bDio1ClkHiz == MCDRV_DAHIZ_LOW) {
			bReg |= MCB_BCLK1_DDR;
			bReg |= MCB_LRCK1_DDR;
		}
	} else {
		if ((bLPort == 3)
		|| ((bLPort <= 2)
			&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bMasterSlave
							== MCDRV_DIO_MASTER))
		) {
			bReg |= MCB_BCLK1_DDR;
			bReg |= MCB_LRCK1_DDR;
		}
	}
	if ((bLPort <= 3)
	&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT))
		bReg	|= MCB_BCLK1_INV;
	if ((sInitInfo.bPowerMode != MCDRV_POWMODE_CDSPDEBUG)
	&& (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn != 1)) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO1_DRV,
			bReg);
	}

	bReg	= 0;
	GetDIState(MCDRV_PHYSPORT_DIO2, &bIsUsedDIR, &bIsUsedDIT, &bLPort);
	if (bIsUsedDIR == 0)
		bReg |= MCB_SDIN2_MSK;
	if (bIsUsedDIT == 0) {
		if (sInitInfo.bDio2SdoHiz == MCDRV_DAHIZ_LOW)
			bReg |= MCB_SDO2_DDR;
	} else {
		bReg |= MCB_SDO2_DDR;
	}
	if ((bIsUsedDIR == 0)
	&& (bIsUsedDIT == 0)
	&& (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) == 0)) {
		bReg |= MCB_BCLK2_MSK;
		bReg |= MCB_LRCK2_MSK;
		if (sInitInfo.bDio2ClkHiz == MCDRV_DAHIZ_LOW) {
			bReg |= MCB_BCLK2_DDR;
			bReg |= MCB_LRCK2_DDR;
		}
	} else {
		if ((bLPort == 3)
		|| ((bLPort <= 2)
			&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bMasterSlave
							== MCDRV_DIO_MASTER))
		) {
			bReg |= MCB_BCLK2_DDR;
			bReg |= MCB_LRCK2_DDR;
		}
	}
	if ((bLPort <= 3)
	&& (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT))
		bReg	|= MCB_BCLK2_INV;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO2_DRV,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDIPad", 0);
#endif
}

/****************************************************************************
 *	GetDIState
 *
 *	Description:
 *			Get DI Pad state.
 *	Arguments:
 *			bPhysPort	Physical Port
 *			pbIsUsedDIR	DI Rx State
 *			pbIsUsedDIT	DI Tx State
 *			pbLPort		Logical PortID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	GetDIState
(
	UINT8	bPhysPort,
	UINT8	*pbIsUsedDIR,
	UINT8	*pbIsUsedDIT,
	UINT8	*pbLPort
)
{
	struct MCDRV_DIOPATH_INFO	sDioPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetDIState");
#endif

	McResCtrl_GetDioPathInfo(&sDioPathInfo);

	*pbIsUsedDIR	= 0;
	*pbIsUsedDIT	= 0;
	*pbLPort	= 0xFF;

	if (sDioPathInfo.abPhysPort[0] == bPhysPort) {
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) == 0) {
			;
		} else {
			*pbIsUsedDIR	= 1;
			*pbLPort	= 0;
		}
		if ((McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0)
			== 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1)
			== 0)) {
			;
		} else {
			*pbIsUsedDIT	= 1;
			*pbLPort	= 0;
		}
	}
	if (sDioPathInfo.abPhysPort[1] == bPhysPort) {
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) == 0) {
			;
		} else {
			*pbIsUsedDIR	= 1;
			*pbLPort	= 1;
		}
		if ((McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0)
			== 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1)
			== 0)) {
			;
		} else {
			*pbIsUsedDIT	= 1;
			*pbLPort	= 1;
		}
	}
	if (sDioPathInfo.abPhysPort[2] == bPhysPort) {
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXIOIN, eMCDRV_DST_CH0)
								== 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXHOSTIN, eMCDRV_DST_CH0)
								== 0)) {
			;
		} else {
			*pbIsUsedDIR	= 1;
			*pbLPort	= 2;
		}
		if (McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0)
			== 0) {
			;
		} else {
			*pbIsUsedDIT	= 1;
			*pbLPort	= 2;
		}
	}
	if (sDioPathInfo.abPhysPort[3] == bPhysPort) {
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) == 0) {
			;
		} else {
			*pbIsUsedDIR	= 1;
			*pbLPort	= 3;
		}
		if (McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0)
			== 0) {
			;
		} else {
			*pbIsUsedDIT	= 1;
			*pbLPort	= 3;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= *pbIsUsedDIT<<8 | *pbIsUsedDIR;
	McDebugLog_FuncOut("GetDIState", &sdRet);
#endif
}

/****************************************************************************
 *	GetLPort
 *
 *	Description:
 *			Get Logical Port ID.
 *	Arguments:
 *			bPhysPort	Physical Port
 *	Return:
 *			Logical Port ID
 *
 ****************************************************************************/
static UINT8	GetLPort(
	UINT8	bPhysPort
)
{
	UINT8	bIsUsedDIR, bIsUsedDIT;
	UINT8	bLPort	= 0xFF;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetLPort");
#endif

	GetDIState(bPhysPort, &bIsUsedDIR, &bIsUsedDIT, &bLPort);

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bLPort;
	McDebugLog_FuncOut("GetLPort", &sdRet);
#endif
	return bLPort;
}

/****************************************************************************
 *	AddPAD
 *
 *	Description:
 *			Add PAD setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddPAD(
	void
)
{
	UINT8	bReg;
	UINT32	dHifiSrc;
	struct MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddPAD");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);

	dHifiSrc	=
		McResCtrl_GetSource(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0);

	if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON) == 0)
	&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_R_ON) == 0)
	&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON) == 0)
	&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_R_ON) == 0)
	&& ((dHifiSrc & (MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)) == 0)
	&& ((dHifiSrc & (MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)) == 0)) {
		if (sInitInfo.bPa0Func == MCDRV_PA_PDMCK) {
			bReg	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA0);
			bReg	|= MCB_PA0_MSK;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_PA0,
					bReg);
		}
	}
	if (sInitInfo.bPa1Func == MCDRV_PA_PDMDI) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA1);
		if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON) == 0)
		&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_R_ON) == 0)
		&& ((dHifiSrc & (MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON))
								== 0)) {
			;
			bReg	|= MCB_PA1_MSK;
		} else {
			bReg	&= (UINT8)~MCB_PA1_MSK;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA1,
				bReg);
	}
	if (sInitInfo.bPa2Func == MCDRV_PA_PDMDI) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA2);
		if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON) == 0)
		&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_R_ON) == 0)
		&& ((dHifiSrc & (MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON))
								== 0)) {
			;
			bReg	|= MCB_PA2_MSK;
		} else {
			bReg	&= (UINT8)~MCB_PA2_MSK;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA2,
				bReg);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddPAD", 0);
#endif
}

/****************************************************************************
 *	AddSource
 *
 *	Description:
 *			Add source setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static void	AddSource(
	void
)
{
	UINT8	bReg;
	UINT8	bDsf0PreInput, bDsf1PreInput, bDsf2PreInput;
	UINT32	dHifiSrc;
	UINT32	dSrc;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddSource");
#endif

	AddInMixSource(eMCDRV_DST_AE0,
			MCI_IN0_MIX0,
			MCI_IN0_MIX1,
			MCB_IN0_MSEP);
	AddInMixSource(eMCDRV_DST_AE1,
			MCI_IN1_MIX0,
			MCI_IN1_MIX1,
			MCB_IN1_MSEP);
	AddInMixSource(eMCDRV_DST_AE2,
			MCI_IN2_MIX0,
			MCI_IN2_MIX1,
			MCB_IN2_MSEP);
	AddInMixSource(eMCDRV_DST_AE3,
			MCI_IN3_MIX0,
			MCI_IN3_MIX1,
			MCB_IN3_MSEP);

	AddOutMixSource(eMCDRV_DST_MUSICOUT,
			MCI_OUT0_MIX0_10_8,
			MCI_OUT0_MIX1_10_8,
			MCB_OUT0_MSEP);
	AddOutMixSource(eMCDRV_DST_EXTOUT,
			MCI_OUT1_MIX0_10_8,
			MCI_OUT1_MIX1_10_8,
			MCB_OUT1_MSEP);
	AddOut2MixSource();
	AddOutMixSource(eMCDRV_DST_DAC0,
			MCI_OUT4_MIX0_10_8,
			MCI_OUT4_MIX1_10_8,
			MCB_OUT4_MSEP);
	AddOutMixSource(eMCDRV_DST_DAC1,
			MCI_OUT5_MIX0_10_8,
			MCI_OUT5_MIX1_10_8,
			MCB_OUT5_MSEP);

	McResCtrl_GetAecInfo(&sAecInfo);

	bDsf0PreInput	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF0_PRE_INPUT);
	bDsf1PreInput	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF1_PRE_INPUT);
	bDsf2PreInput	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF2_PRE_INPUT);

	dHifiSrc	=
		McResCtrl_GetSource(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0);

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) != 0)) {
		bReg	= bDsf0PreInput;
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0);
		if (dSrc != 0) {
			bReg	&= ~0x0F;
			bReg	|= GetPreInput(dSrc);
		}
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1);
		if (dSrc != 0) {
			bReg	&= ~0xF0;
			bReg	|= (UINT8)(GetPreInput(dSrc)<<4);
		}
		if (bReg != bDsf0PreInput)
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_DSF0_PRE_INPUT,
					bReg);
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) != 0)) {
		bReg	= bDsf1PreInput;
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0);
		if (dSrc != 0) {
			bReg	&= ~0x0F;
			bReg	|= GetPreInput(dSrc);
		}
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1);
		if (dSrc != 0) {
			bReg	&= ~0xF0;
			bReg	|= (UINT8)(GetPreInput(dSrc)<<4);
		}
		if (bReg != bDsf1PreInput)
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_DSF1_PRE_INPUT,
					bReg);
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1) != 0)) {
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0);
		if ((dSrc != MCDRV_D2SRC_DAC0REF_ON)
		&& (dSrc != MCDRV_D2SRC_DAC1REF_ON)) {
			bReg	= bDsf2PreInput;
			if (dSrc != 0) {
				bReg	&= ~0x0F;
				bReg	|= GetPreInput(dSrc);
			}
			dSrc	=
			McResCtrl_GetSource(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1);
			if (dSrc != 0) {
				bReg	&= ~0xF0;
				bReg	|= (UINT8)(GetPreInput(dSrc)<<4);
			}
			if (bReg != bDsf2PreInput)
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_DSF2_PRE_INPUT,
						bReg);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddSource", NULL);
#endif
}

/****************************************************************************
 *	GetPreInput
 *
 *	Description:
 *			Get DSF*_PRE_INPUT register setting.
 *	Arguments:
 *			dSrc	source info
 *	Return:
 *			register setting
 *
 ****************************************************************************/
static UINT8	GetPreInput(
	UINT32	dSrcOnOff
)
{
	UINT8	bReg	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetPreInput");
#endif

	switch (dSrcOnOff) {
	case	MCDRV_D2SRC_ADC0_L_ON:
		bReg	= DSF_PRE_INPUT_ADC_L;
		break;
	case	MCDRV_D2SRC_ADC0_R_ON:
		bReg	= DSF_PRE_INPUT_ADC_R;
		break;
	case	MCDRV_D2SRC_ADC1_ON:
		bReg	= DSF_PRE_INPUT_ADC_M;
		break;
	case	MCDRV_D2SRC_PDM0_L_ON:
		bReg	= DSF_PRE_INPUT_PDM0_L;
		break;
	case	MCDRV_D2SRC_PDM0_R_ON:
		bReg	= DSF_PRE_INPUT_PDM0_R;
		break;
	case	MCDRV_D2SRC_PDM1_L_ON:
		bReg	= DSF_PRE_INPUT_PDM1_L;
		break;
	case	MCDRV_D2SRC_PDM1_R_ON:
		bReg	= DSF_PRE_INPUT_PDM1_R;
		break;
	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bReg;
	McDebugLog_FuncOut("GetPreInput", &sdRet);
#endif
	return bReg;
}

/****************************************************************************
 *	AddInMixSource
 *
 *	Description:
 *			Add INx_MIX source setup packet.
 *	Arguments:
 *			eDstType	destination type
 *			bRegAddr0	IN*_MIX0 address
 *			bRegAddr1	IN*_MIX1 address
 *			bSep		IN*_MSEP bit
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddInMixSource(
	enum MCDRV_DST_TYPE	eDstType,
	UINT8	bRegAddr0,
	UINT8	bRegAddr1,
	UINT8	bSep
)
{
	UINT32	dSrc;
	UINT8	bReg0, bReg1, bCurReg0, bCurReg1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddInMixSource");
#endif

	bCurReg0
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr0);
	bCurReg0	&= (UINT8)~bSep;
	bCurReg1
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr1);
	dSrc	= McResCtrl_GetSource(eDstType, eMCDRV_DST_CH0);
	bReg0	= GetInMixReg(dSrc);
	dSrc	= McResCtrl_GetSource(eDstType, eMCDRV_DST_CH1);
	bReg1	= GetInMixReg(dSrc);
	if ((bReg0 != bCurReg0)
	|| (bReg1 != bCurReg1)) {
		if (bReg0 == bReg1) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr0,
					bReg0);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
						bRegAddr1, bReg1);
		} else {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr0,
					bReg0 | bSep);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr1,
					bReg1);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddInMixSource", NULL);
#endif
}

/****************************************************************************
 *	GetInMixReg
 *
 *	Description:
 *			Get IN*_MIX register setting.
 *	Arguments:
 *			dSrc	source info
 *	Return:
 *			IN*_MIX register setting
 *
 ****************************************************************************/
static UINT8	GetInMixReg
(
	UINT32	dSrcOnOff
)
{
	UINT8	bReg	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetInMixReg");
#endif

	if ((dSrcOnOff & MCDRV_D1SRC_MUSICIN_ON) != 0)
		bReg	|= IN_MIX_DIFI_0;

	if ((dSrcOnOff & MCDRV_D1SRC_EXTIN_ON) != 0)
		bReg	|= IN_MIX_DIFI_1;

	if ((dSrcOnOff & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		bReg	|= IN_MIX_DIFI_2;

	if ((dSrcOnOff & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		bReg	|= IN_MIX_DIFI_3;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF0_ON) != 0)
		bReg	|= IN_MIX_ADI_0;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF1_ON) != 0)
		bReg	|= IN_MIX_ADI_1;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF2_ON) != 0)
		bReg	|= IN_MIX_ADI_2;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bReg;
	McDebugLog_FuncOut("GetInMixReg", &sdRet);
#endif
	return bReg;
}

/****************************************************************************
 *	AddOutMixSource
 *
 *	Description:
 *			Add OUTx_MIX source setup packet.
 *	Arguments:
 *			eDstType	destination type
 *			bRegAddr0	IN*_MIX0 address
 *			bRegAddr1	IN*_MIX1 address
 *			bSep		IN*_MSEP bit
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddOutMixSource(
	enum MCDRV_DST_TYPE	eDstType,
	UINT8	bRegAddr0,
	UINT8	bRegAddr1,
	UINT8	bSep
)
{
	UINT32	dSrc;
	UINT16	wReg0, wReg1, wCurReg0, wCurReg1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddOutMixSource");
#endif

	wCurReg0
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr0)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr0+1);
	wCurReg0	&= ~(bSep<<8);
	wCurReg1
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr1)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, bRegAddr1+1);
	dSrc	= McResCtrl_GetSource(eDstType, eMCDRV_DST_CH0);
	wReg0	= GetOutMixReg(dSrc);
	dSrc	= McResCtrl_GetSource(eDstType, eMCDRV_DST_CH1);
	wReg1	= GetOutMixReg(dSrc);
	if ((wReg0 != wCurReg0)
	|| (wReg1 != wCurReg1)) {
		if (wReg0 == wReg1) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr0,
					(UINT8)(wReg0>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| ((UINT32)bRegAddr0+1),
					(UINT8)wReg0);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
						bRegAddr1, (UINT8)(wReg1>>8));
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
						bRegAddr1+1, (UINT8)wReg1);
		} else {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr0,
					(UINT8)(wReg0>>8) | bSep);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| ((UINT32)bRegAddr0+1),
					(UINT8)wReg0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)bRegAddr1,
					(UINT8)(wReg1>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| ((UINT32)bRegAddr1+1),
					(UINT8)wReg1);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddOutMixSource", NULL);
#endif
}

/****************************************************************************
 *	AddOut2MixSource
 *
 *	Description:
 *			Add OUT23_MIX source setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddOut2MixSource(
	void
)
{
	UINT32	dSrc;
	UINT16	wReg0, wReg1, wCurReg0, wCurReg1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddOut2MixSource");
#endif

	wCurReg0
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX0_10_8)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX0_7_0);
	wCurReg0	&= ~(MCB_OUT2_MSEP<<8);
	wCurReg1
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX1_10_8)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX1_7_0);
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0);
	wReg0	= GetOutMixReg(dSrc);
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1);
	wReg1	= GetOutMixReg(dSrc);
	if ((wReg0 != wCurReg0)
	|| (wReg1 != wCurReg1)) {
		if (wReg0 == wReg1) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX0_10_8,
					(UINT8)(wReg0>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX0_7_0,
					(UINT8)wReg0);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX1_10_8, (UINT8)(wReg1>>8));
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT2_MIX1_7_0, (UINT8)wReg1);
		} else {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX0_10_8,
					(UINT8)(wReg0>>8) | MCB_OUT2_MSEP);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX0_7_0,
					(UINT8)wReg0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX1_10_8,
					(UINT8)(wReg1>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT2_MIX1_7_0,
					(UINT8)wReg1);
		}
	}

	wCurReg0
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX0_10_8)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX0_7_0);
	wCurReg0	&= ~(MCB_OUT3_MSEP<<8);
	wCurReg1
		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX1_10_8)<<8
		| McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX1_7_0);
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2);
	wReg0	= GetOutMixReg(dSrc);
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3);
	wReg1	= GetOutMixReg(dSrc);
	if ((wReg0 != wCurReg0)
	|| (wReg1 != wCurReg1)) {
		if (wReg0 == wReg1) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX0_10_8,
					(UINT8)(wReg0>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX0_7_0,
					(UINT8)wReg0);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX1_10_8, (UINT8)(wReg1>>8));
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_MA,
					MCI_OUT3_MIX1_7_0, (UINT8)wReg1);
		} else {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX0_10_8,
					(UINT8)(wReg0>>8) | MCB_OUT3_MSEP);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX0_7_0,
					(UINT8)wReg0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX1_10_8,
					(UINT8)(wReg1>>8));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_OUT3_MIX1_7_0,
					(UINT8)wReg1);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddOut2MixSource", NULL);
#endif
}

/****************************************************************************
 *	GetOutMixReg
 *
 *	Description:
 *			Get OUT*_MIX register setting.
 *	Arguments:
 *			dSrcOnOff	source info
 *	Return:
 *			OUT*_MIX register setting
 *
 ****************************************************************************/
static UINT16	GetOutMixReg(
	UINT32	dSrcOnOff
)
{
	UINT16	wReg	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetOutMixReg");
#endif

	if ((dSrcOnOff & MCDRV_D1SRC_MUSICIN_ON) != 0)
		wReg	|= OUT_MIX_DIFI_0;

	if ((dSrcOnOff & MCDRV_D1SRC_EXTIN_ON) != 0)
		wReg	|= OUT_MIX_DIFI_1;

	if ((dSrcOnOff & MCDRV_D1SRC_VBOXOUT_ON) != 0)
		wReg	|= OUT_MIX_DIFI_2;

	if ((dSrcOnOff & MCDRV_D1SRC_VBOXREFOUT_ON) != 0)
		wReg	|= OUT_MIX_DIFI_3;

	if ((dSrcOnOff & MCDRV_D1SRC_AE0_ON) != 0)
		wReg	|= OUT_MIX_AEO_0;

	if ((dSrcOnOff & MCDRV_D1SRC_AE1_ON) != 0)
		wReg	|= OUT_MIX_AEO_1;

	if ((dSrcOnOff & MCDRV_D1SRC_AE2_ON) != 0)
		wReg	|= OUT_MIX_AEO_2;

	if ((dSrcOnOff & MCDRV_D1SRC_AE3_ON) != 0)
		wReg	|= OUT_MIX_AEO_3;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF0_ON) != 0)
		wReg	|= OUT_MIX_ADI_0;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF1_ON) != 0)
		wReg	|= OUT_MIX_ADI_1;

	if ((dSrcOnOff & MCDRV_D1SRC_ADIF2_ON) != 0)
		wReg	|= OUT_MIX_ADI_2;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)wReg;
	McDebugLog_FuncOut("GetOutMixReg", &sdRet);
#endif
	return wReg;
}

/****************************************************************************
 *	AddMixSet
 *
 *	Description:
 *			Add analog mixer set packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
static void	AddMixSet(
	void
)
{
	UINT8	bReg;
	struct MCDRV_PATH_INFO	sPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddMixSet");
#endif

	McResCtrl_GetPathInfo(&sPathInfo);

	/*	ADL_MIX	*/
	bReg	= GetMicMixBit(sPathInfo.asAdc0[0].dSrcOnOff);
	if ((sPathInfo.asAdc0[0].dSrcOnOff & MCDRV_ASRC_LINEIN1_L_ON) != 0)
		bReg	|= MCB_AD_LIMIX;
	if ((sPathInfo.asAdc0[0].dSrcOnOff & MCDRV_ASRC_LINEIN1_M_ON) != 0)
		bReg	|= (MCB_MONO_AD_LI|MCB_AD_LIMIX);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_ADL_MIX,
			bReg);

	/*	ADR_MIX	*/
	bReg	= GetMicMixBit(sPathInfo.asAdc0[1].dSrcOnOff);
	if ((sPathInfo.asAdc0[1].dSrcOnOff & MCDRV_ASRC_LINEIN1_R_ON) != 0)
		bReg	|= MCB_AD_LIMIX;
	if ((sPathInfo.asAdc0[1].dSrcOnOff & MCDRV_ASRC_LINEIN1_M_ON) != 0)
		bReg	|= (MCB_MONO_AD_LI|MCB_AD_LIMIX);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_ADR_MIX,
			bReg);

	/*	ADM_MIX	*/
	bReg	= GetMicMixBit(sPathInfo.asAdc1[0].dSrcOnOff);
	if ((sPathInfo.asAdc1[0].dSrcOnOff & MCDRV_ASRC_LINEIN1_M_ON) != 0)
		bReg	|= MCB_AD_LIMIX;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_ADM_MIX,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddMixSet", NULL);
#endif
}

/****************************************************************************
 *	GetMicMixBit
 *
 *	Description:
 *			Get mic mixer bit.
 *	Arguments:
 *			dSrcOnOff	source info
 *	Return:
 *			mic mixer bit
 *
 ****************************************************************************/
static UINT8	GetMicMixBit(
	UINT32	dSrcOnOff
)
{
	UINT8	bMicMix	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetMicMixBit");
#endif

	if ((dSrcOnOff & MCDRV_ASRC_MIC1_ON) != 0)
		bMicMix |= MCB_AD_M1MIX;

	if ((dSrcOnOff & MCDRV_ASRC_MIC2_ON) != 0)
		bMicMix |= MCB_AD_M2MIX;

	if ((dSrcOnOff & MCDRV_ASRC_MIC3_ON) != 0)
		bMicMix |= MCB_AD_M3MIX;

	if ((dSrcOnOff & MCDRV_ASRC_MIC4_ON) != 0)
		bMicMix |= MCB_AD_M4MIX;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bMicMix;
	McDebugLog_FuncOut("GetMicMixBit", &sdRet);
#endif
	return bMicMix;
}

/****************************************************************************
 *	AddDacStop
 *
 *	Description:
 *			Add DAC stop packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDacStop(
	void
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddDacStop");
#endif
	/*	DAC Stop	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_LPF_THR);
	if ((McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH1) == 0))
		bReg	&= (UINT8)~MCB_OSF0_ENB;
	if ((McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) == 0))
		bReg	&= (UINT8)~MCB_OSF1_ENB;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_LPF_THR,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDacStop", 0);
#endif
}

/****************************************************************************
 *	GetLP2Start
 *
 *	Description:
 *			Get LP2_START register value.
 *	Arguments:
 *			none
 *	Return:
 *			LP2_START register value
 *
 ****************************************************************************/
static UINT8	GetLP2Start(
	void
)
{
	UINT8	bStart	= 0;
	struct MCDRV_DIO_INFO	sDioInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetLP2Start");
#endif
	McResCtrl_GetDioInfo(&sDioInfo);
	McResCtrl_GetAecInfo(&sAecInfo);

	if (McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0) != 0) {
		if (sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			;
			bStart |= MCB_LP2_TIM_START;
		}
		bStart |= MCB_LPT2_START_SRC;
		bStart |= MCB_LPT2_START;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXIOIN, eMCDRV_DST_CH0) != 0) {
		if (sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			;
			bStart |= MCB_LP2_TIM_START;
		}
		bStart |= MCB_LPR2_START_SRC;
		bStart |= MCB_LPR2_START;
	}
	if (bStart == 0) {	/*	LP2 not running	*/
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1)
			!= 0)) {
			bStart |= MCB_LP2_TIM_START;
			bStart |= MCB_LPT2_START_SRC;
			bStart |= MCB_LPT2_START;
		}
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) != 0) {
			bStart |= MCB_LP2_TIM_START;
			bStart |= MCB_LPR2_START_SRC;
			bStart |= MCB_LPR2_START;
		}
	} else {
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1)
			!= 0)) {
			bStart |= MCB_LPT2_START_SRC;
			bStart |= MCB_LPT2_START;
		}
		if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) != 0) {
			bStart |= MCB_LPR2_START_SRC;
			bStart |= MCB_LPR2_START;
		}
	}
	if (bStart == 0) {	/*	LP2 not running	*/
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3)
			!= 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON)
			!= 0)) {
			if ((sAecInfo.sAecVBox.bSrc3_Ctrl == 1)
			|| (sAecInfo.sAecVBox.bSrc3_Ctrl == 2)) {
				;
				bStart |= MCB_LP2_TIM_START;
			}
		}
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3)
			!= 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON)
			!= 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXHOSTIN, eMCDRV_DST_CH0)
			!= 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_HOSTOUT, eMCDRV_DST_CH0)
			!= 0)) {
			if ((sAecInfo.sAecConfig.bFDspLocate != 0)
			&& (sAecInfo.sAecVBox.bFDspOnOff == 1)) {
				;
				bStart |= MCB_LP2_TIM_START;
			}
		}
	}
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2)
		!= 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3)
		!= 0)
	|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON)
		!= 0)) {
		if ((sAecInfo.sAecVBox.bSrc3_Ctrl == 1)
		|| (sAecInfo.sAecVBox.bSrc3_Ctrl == 2)) {
			bStart |= MCB_LPT2_START_SRC;
			bStart |= MCB_LPT2_START;
			bStart |= MCB_LPR2_START_SRC;
			bStart |= MCB_LPR2_START;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bStart;
	McDebugLog_FuncOut("GetLP2Start", &sdRet);
#endif
	return bStart;
}

/****************************************************************************
 *	GetLP2Fs
 *
 *	Description:
 *			Get LP2_FS register value.
 *	Arguments:
 *			bThru	LP2_SRC_THRU register value
 *	Return:
 *			LP2_FS register value
 *
 ****************************************************************************/
static UINT8	GetLP2Fs(
	UINT8	*bThru
)
{
	UINT8	bFs	= 0xFF;
	struct MCDRV_DIO_INFO	sDioInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetLP2Fs");
#endif

	McResCtrl_GetDioInfo(&sDioInfo);
	McResCtrl_GetAecInfo(&sAecInfo);

	*bThru	= 0xFF;

	if ((McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXIOIN, eMCDRV_DST_CH0) != 0)) {
		/*	LP2 running	*/
		*bThru	= (sDioInfo.asPortInfo[2].sDioCommon.bSrcThru<<5);
		bFs	= sDioInfo.asPortInfo[2].sDioCommon.bFs;
	} else {
		if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN,
			eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN,
			eMCDRV_DST_CH1) != 0)
		|| (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON)
			!= 0)) {
			*bThru	= (sAecInfo.sAecVBox.bSrc2_Thru<<5);
			bFs	= sAecInfo.sAecVBox.bSrc2_Fs;
		}
	}
	if (sAecInfo.sAecVBox.bSrc3_Ctrl == 1) {
		if (*bThru == 0xFF) {
			*bThru	= (sAecInfo.sAecVBox.bSrc2_Thru<<5);
			bFs	= sAecInfo.sAecVBox.bSrc2_Fs;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bFs;
	McDebugLog_FuncOut("GetLP2Fs", &sdRet);
#endif
	return bFs;
}

/****************************************************************************
 *	McPacket_AddStop
 *
 *	Description:
 *			Add stop packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddStop(
	void
)
{
	UINT8	bReg, bCurReg;
	UINT32	dHifiSrc;
	UINT8	bDirect_Enb;
	UINT32	dSrc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddStop");
#endif

	/*	LP*_START	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP0_START);
	bCurReg	= bReg;
	if ((McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) == 0)) {
		bReg &= (UINT8)~MCB_LPT0_START_SRC;
		bReg &= (UINT8)~MCB_LPT0_START;
	}
	if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) == 0)) {
		bReg &= (UINT8)~MCB_LPR0_START_SRC;
		bReg &= (UINT8)~MCB_LPR0_START;
	}
	if ((bReg & 0x0F) == 0)
		bReg	= 0;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LP0_START,
			bReg);
	if ((bReg == 0)
	&& (bReg != bCurReg))
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP0_FP,
				MCDRV_PHYSPORT_NONE);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP1_START);
	bCurReg	= bReg;
	if ((McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) == 0)) {
		bReg &= (UINT8)~MCB_LPT1_START_SRC;
		bReg &= (UINT8)~MCB_LPT1_START;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) == 0) {
		bReg &= (UINT8)~MCB_LPR1_START_SRC;
		bReg &= (UINT8)~MCB_LPR1_START;
	}
	if ((bReg & 0x0F) == 0)
		bReg	= 0;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LP1_START,
			bReg);
	if ((bReg == 0)
	&& (bReg != bCurReg))
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP1_FP,
				MCDRV_PHYSPORT_NONE);

	bCurReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP2_START);
	bReg	= bCurReg & GetLP2Start();
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LP2_START,
			bReg);
	if ((bReg == 0)
	&& (bReg != bCurReg))
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP2_FP,
				MCDRV_PHYSPORT_NONE);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_SRC3_START);
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) == 0))
		bReg &= (UINT8)~MCB_OSRC3_START;
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) == 0)
		bReg &= (UINT8)~MCB_ISRC3_START;
	if ((bReg & 0x0F) == 0)
		bReg	= 0;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_SRC3_START,
			bReg);

	bDirect_Enb	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DIRECTPATH_ENB);
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP3_START);
	if (McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0) == 0) {
		bReg	&= (UINT8)~MCB_LPT3_START;
		bDirect_Enb	&= (UINT8)~MCB_DIRECT_ENB_ADC;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) == 0) {
		bReg	&= (UINT8)~MCB_LPR3_START;
		bDirect_Enb	&= (UINT8)~(MCB_DIRECT_ENB_DAC1
						|MCB_DIRECT_ENB_DAC0);
	} else {
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH1);
		if ((dSrc & MCDRV_D1SRC_HIFIIN_ON) == 0) {
			;
			bDirect_Enb	&= (UINT8)~MCB_DIRECT_ENB_DAC0;
		}
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH1);
		if ((dSrc & MCDRV_D1SRC_HIFIIN_ON) == 0) {
			;
			bDirect_Enb	&= (UINT8)~MCB_DIRECT_ENB_DAC1;
		}
	}
	if ((bReg & 0x0F) == 0)
		bReg	= 0;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LP3_START,
			bReg);
	if (bReg == 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DIRECTPATH_ENB,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP3_FP,
				MCDRV_PHYSPORT_NONE);
	} else {
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_DIRECTPATH_ENB,
					bDirect_Enb);
		}
	}

	/*	ADC Stop	*/
	dHifiSrc	=
		McResCtrl_GetSource(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0);

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) == 0)) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF0_FLT_TYPE);
		bReg	&= ~MCB_DSF0ENB;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF0_FLT_TYPE,
				bReg);
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) == 0)) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF1_FLT_TYPE);
		bReg	&= ~MCB_DSF1ENB;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF1_FLT_TYPE,
				bReg);
	}

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1) == 0)) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF2_FLT_TYPE);
		bReg	&= ~MCB_DSF2ENB;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF2_FLT_TYPE,
				bReg);
	}

	/*	PDM Stop	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_PDM_LOAD_TIM);
	if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON) == 0)
	&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_R_ON) == 0)
	&& ((dHifiSrc & (MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)) == 0))
		bReg	&= (UINT8)~MCB_PDM0_START;
	if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON) == 0)
	&& (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_R_ON) == 0)
	&& ((dHifiSrc & (MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)) == 0))
		bReg	&= (UINT8)~MCB_PDM1_START;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM_LOAD_TIM,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddStop", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddDSPStartStop
 *
 *	Description:
 *			Add DSP start/stop packet.
 *	Arguments:
 *			bStarted	DSP has started
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDSPStartStop(
	UINT8	bStarted
)
{
	UINT8	bStart	= McResCtrl_GetDspStart();
	UINT8	bDFifoSel, bRFifoSel;
	struct MCDRV_AEC_INFO	sAecInfo;
	struct MCDRV_DIO_INFO	sDioInfo;
	int	i;
	struct MCDRV_FDSP_MUTE	sMute;
	UINT8	bE1Command	= 0;
	UINT8	bReg;
	UINT8	bLP2Fs;
	UINT8	bThru;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDSPStartStop");
#endif

	McResCtrl_GetAecInfo(&sAecInfo);

	if ((bStart&MCDRV_DSP_START_E) != 0) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
				MCI_E1COMMAND);
		if ((McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0) == 0)
		&& (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1) == 0))
			bE1Command	= E1COMMAND_PD;
		else
			bE1Command	= 0;
		if ((bStarted&MCDRV_DSP_START_E) == 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_E1DSP_CTRL),
					(UINT8)0x00);
			bE1Command	|= E1COMMAND_RUN_MODE;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_E1COMMAND),
					bE1Command);

			McEdsp_Start();
		} else if ((bReg&E1COMMAND_PD) != (bE1Command&E1COMMAND_PD)) {
			bE1Command	|= E1COMMAND_ADDITIONAL;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_E1COMMAND),
					bE1Command);
		}
	} else if ((bStarted&MCDRV_DSP_START_E) != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_E1COMMAND),
				E1COMMAND_SLEEP_MODE);
		McEdsp_Stop();
	}
	if ((bStart&MCDRV_DSP_START_B) != 0) {
		if ((bStarted&MCDRV_DSP_START_B) == 0) {
			;
			McBdsp_Start();
		}
	} else if ((bStarted&MCDRV_DSP_START_B) != 0) {
		McBdsp_Stop();
	}
	if ((bStart&MCDRV_DSP_START_F) != 0) {
		if ((bStarted&MCDRV_DSP_START_F) == 0) {
			for (i = 0; i < FDSP_MUTE_NUM; i++) {
				sMute.abInMute[i]	= FDSP_MUTE_OFF;
				sMute.abOutMute[i]	= FDSP_MUTE_OFF;
			}
			McFdsp_Start();
			McFdsp_SetMute(&sMute);
		}
	}
	if ((bStart&MCDRV_DSP_START_C) != 0) {
		if (McResCtrl_HasSrc(eMCDRV_DST_VBOXHOSTIN, eMCDRV_DST_CH0)
			== 0)
			bDFifoSel	= CDSP_FIFO_SEL_PORT;
		else
			bDFifoSel	= CDSP_FIFO_SEL_HOST;
		if (McResCtrl_HasSrc(eMCDRV_DST_HOSTOUT, eMCDRV_DST_CH0) == 0)
			bRFifoSel	= CDSP_FIFO_SEL_PORT;
		else
			bRFifoSel	= CDSP_FIFO_SEL_HOST;
		McCdsp_SetDFifoSel(bDFifoSel);
		McCdsp_SetRFifoSel(bRFifoSel);
		if ((bStarted&MCDRV_DSP_START_C) == 0) {
			McResCtrl_GetDioInfo(&sDioInfo);
			bLP2Fs	= GetLP2Fs(&bThru);
			if (sAecInfo.sAecVBox.bCDspFuncAOnOff != 0) {
				if ((sAecInfo.sAecVBox.bSrc3_Ctrl == 3)
				&& (sAecInfo.sAecVBox.bCDspFuncAOnOff == 0x11))
					bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_MB,
						MCI_SRC3);
				else
					bReg	= bLP2Fs;
				McCdsp_SetFs(eMC_PLAYER_CODER_A,
					bReg&0x0F);
				McCdsp_Start(eMC_PLAYER_CODER_A);
			}
			if (sAecInfo.sAecVBox.bCDspFuncBOnOff != 0) {
				if ((sAecInfo.sAecVBox.bSrc3_Ctrl == 3)
				&& (sAecInfo.sAecVBox.bCDspFuncBOnOff == 0x11))
					bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_MB,
						MCI_SRC3);
				else
					bReg	= bLP2Fs;
				McCdsp_SetFs(eMC_PLAYER_CODER_B,
					bReg&0x0F);
				McCdsp_Start(eMC_PLAYER_CODER_B);
			}
		}
	} else if ((bStarted&MCDRV_DSP_START_C) != 0) {
		if (sAecInfo.sAecVBox.bCDspFuncAOnOff != 0)
			McCdsp_Stop(eMC_PLAYER_CODER_A);
		if (sAecInfo.sAecVBox.bCDspFuncBOnOff != 0)
			McCdsp_Stop(eMC_PLAYER_CODER_B);
	}

	if ((bStart&MCDRV_DSP_START_M) != 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MA
				| (UINT32)MCI_SPATH_ON,
				GetSPath());
		if ((bStarted&MCDRV_DSP_START_M) == 0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_DSP_START,
					MCB_DSP_START);
		}
	} else if ((bStarted&MCDRV_DSP_START_M) != 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MA
				| (UINT32)MCI_SPATH_ON,
				0);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDSPStartStop", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddFDSPStop
 *
 *	Description:
 *			Add F-DSP stop packet.
 *	Arguments:
 *			bStarted	DSP has started
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddFDSPStop(
	UINT8	bStarted
)
{
	UINT8	bStart	= McResCtrl_GetDspStart();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddFDSPStop");
#endif

	if (((bStart&MCDRV_DSP_START_F) == 0)
	&& ((bStarted&MCDRV_DSP_START_F) != 0)) {
		McFdsp_Stop();
	}
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddFDSPStop", NULL);
#endif
}

/****************************************************************************
 *	AddDIStart
 *
 *	Description:
 *			Add DIStart setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIStart(
	void
)
{
	UINT8	bStart;
	UINT8	bReg;
	UINT8	bThru	= 0xFF,
		bFs	= 0xFF;
	struct MCDRV_DIO_INFO	sDioInfo;
	struct MCDRV_DIOPATH_INFO	sDioPathInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddDIStart");
#endif

	McResCtrl_GetDioInfo(&sDioInfo);
	McResCtrl_GetAecInfo(&sAecInfo);
	McResCtrl_GetDioPathInfo(&sDioPathInfo);

	bStart	= 0;
	if ((McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) != 0)) {
		if (sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			bStart |= MCB_LP0_TIM_START;
		}
		bStart |= MCB_LPT0_START_SRC;
		bStart |= MCB_LPT0_START;
	}
	if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) != 0)) {
		if (sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			bStart |= MCB_LP0_TIM_START;
		}
		bStart |= MCB_LPR0_START_SRC;
		bStart |= MCB_LPR0_START;
	}
	if (bStart != 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP0_FP,
				sDioPathInfo.abPhysPort[0]);
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_LINK_LOCK,
					MCB_LINK_LOCK);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_T_DPLL_FAST,
					MCB_T_DPLL_FAST|MCB_VOLREL_TIME);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LP0_START,
				bStart);
	}

	bStart	= 0;
	if ((McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) != 0)) {
		if (sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			bStart |= MCB_LP1_TIM_START;
		}
		bStart |= MCB_LPT1_START_SRC;
		bStart |= MCB_LPT1_START;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) != 0) {
		if (sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave
						== MCDRV_DIO_MASTER) {
			bStart |= MCB_LP1_TIM_START;
		}
		bStart |= MCB_LPR1_START_SRC;
		bStart |= MCB_LPR1_START;
	}
	if (bStart != 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP1_FP,
				sDioPathInfo.abPhysPort[1]);
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_LINK_LOCK,
					MCB_LINK_LOCK);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_T_DPLL_FAST,
					MCB_T_DPLL_FAST|MCB_VOLREL_TIME);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LP1_START,
				bStart);
	}

	bStart	= GetLP2Start();
	if (bStart != 0) {
		bThru	= 0xFF;
		bFs	= GetLP2Fs(&bThru);
		if (bThru != 0xFF) {
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_MB,
						MCI_LP2_MODE);
				if ((bReg&MCB_LP2_SRC_THRU) != bThru) {
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_MB
						| (UINT32)MCI_LP2_START,
						0);
					bReg	&= (UINT8)~MCB_LP2_SRC_THRU;
					bReg	|= bThru;
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_MB
						| (UINT32)MCI_LP2_MODE,
						bReg);
				}
			}

			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB,
								MCI_LP2_BCK);
			if ((bReg&0x0F) != bFs) {
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_MB
						| (UINT32)MCI_LP2_START,
						0);
				bReg	&= 0xF0;
				bReg	|= bFs;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_MB
						| (UINT32)MCI_LP2_BCK,
						bReg);
			}
		}
		if ((McResCtrl_HasSrc(eMCDRV_DST_VOICEOUT, eMCDRV_DST_CH0) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXIOIN, eMCDRV_DST_CH0) != 0)
		) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_LP2_FP,
					sDioPathInfo.abPhysPort[2]);
		} else {	/*	LP2 not running	*/
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB,
							MCI_LP2_START);
			if (bReg != 0) {
				;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_LP2_FP,
					MCDRV_PHYSPORT_NONE);
			}
		}
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_LINK_LOCK,
					MCB_LINK_LOCK);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_T_DPLL_FAST,
					MCB_T_DPLL_FAST|MCB_VOLREL_TIME);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LP2_START,
				bStart);
	}

	bStart	= 0;
	bThru	= 0xFF;
	bFs	= 0xFF;
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) != 0)) {
		bStart |= MCB_SRC3_TIM_START;
		bStart |= MCB_OSRC3_START;
		bThru	= (sAecInfo.sAecVBox.bSrc3_Thru<<5);
		bFs	= sAecInfo.sAecVBox.bSrc3_Fs;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) != 0) {
		bStart |= MCB_SRC3_TIM_START;
		bStart |= MCB_ISRC3_START;
		bThru	= (sAecInfo.sAecVBox.bSrc3_Thru<<5);
		bFs	= sAecInfo.sAecVBox.bSrc3_Fs;
	}
	if ((bStart != 0)
	&& (sAecInfo.sAecVBox.bSrc3_Ctrl) == 3)
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_SRC3,
				(UINT8)(bThru<<4) | bFs);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_SRC3_START,
			bStart);

	bStart	= 0;
	if (McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0) != 0) {
		bStart |= MCB_LP3_TIM_START;
		bStart |= MCB_LPT3_START;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) != 0) {
		bStart |= MCB_LP3_TIM_START;
		bStart |= MCB_LPR3_START;
	}
	if (bStart != 0) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_LP3_FP,
				sDioPathInfo.abPhysPort[3]);
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_LINK_LOCK,
					MCB_LINK_LOCK);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_T_DPLL_FAST,
					MCB_T_DPLL_FAST|MCB_VOLREL_TIME);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LP3_START,
				bStart);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDIStart", 0);
#endif
}

/****************************************************************************
 *	AddDacStart
 *
 *	Description:
 *			Add DAC Start setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDacStart(
	void
)
{
	UINT8	bReg;
	UINT8	bOsf0_Enb	= 0,
		bOsf1_Enb	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddDacStart");
#endif

	/*	DAC Start	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_LPF_THR);
	bOsf0_Enb	= bReg & (MCB_OSF0_MN|MCB_OSF0_ENB);
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH0) != 0)
		bOsf0_Enb	= MCB_OSF0_ENB | MCB_OSF0_MN;
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH1) != 0)
		bOsf0_Enb	= MCB_OSF0_ENB;
	bOsf1_Enb	= bReg & (MCB_OSF1_MN|MCB_OSF1_ENB);
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) != 0)
		bOsf1_Enb	= MCB_OSF1_ENB | MCB_OSF1_MN;
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) != 0)
		bOsf1_Enb	= MCB_OSF1_ENB;
	bReg	&= (MCB_LPF1_PST_THR|MCB_LPF0_PST_THR
			|MCB_LPF1_PRE_THR|MCB_LPF0_PRE_THR);
	bReg	|= (bOsf1_Enb|bOsf0_Enb);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_LPF_THR,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDacStart", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddStart
 *
 *	Description:
 *			Add start packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddStart(
	void
)
{
	UINT8	bReg;
	UINT32	dHifiSrc;
	UINT8	bDsf0_Enb	= 0,
		bDsf1_Enb	= 0,
		bDsf2_Enb	= 0;
	UINT32	dSrc;
	UINT8	bDirect_Enb	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddStart");
#endif

	/*	DIR*_START, DIT*_START	*/
	AddDIStart();

	/*	ADC Start	*/
	bDsf0_Enb	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF0_FLT_TYPE);
	bDsf1_Enb	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF1_FLT_TYPE);
	bDsf2_Enb	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
						MCI_DSF2_FLT_TYPE);
	dHifiSrc	=
		McResCtrl_GetSource(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0);

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) != 0)) {
		bDsf0_Enb	|= MCB_DSF0ENB;
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF0_PRE_INPUT);
		if ((bReg>>4) == (bReg&0x0F)
		|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF0, eMCDRV_DST_CH1) == 0))
			bDsf0_Enb	|= MCB_DSF0_MN;
		else
			bDsf0_Enb	&= ~MCB_DSF0_MN;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF0_FLT_TYPE,
			bDsf0_Enb);

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) != 0)) {
		bDsf1_Enb	|= MCB_DSF1ENB;
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF1_PRE_INPUT);
		if ((bReg>>4) == (bReg&0x0F)
		|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF1, eMCDRV_DST_CH1) == 0))
			bDsf1_Enb	|= MCB_DSF1_MN;
		else
			bDsf1_Enb	&= ~MCB_DSF1_MN;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF1_FLT_TYPE,
			bDsf1_Enb);

	if ((McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1) != 0)) {
		bDsf2_Enb	|= MCB_DSF2ENB;
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_ADIF2, eMCDRV_DST_CH0);
		if (dSrc == MCDRV_D2SRC_DAC0REF_ON) {
			bDsf2_Enb	|= MCB_DSF2REFBACK;
			bDsf2_Enb	&= ~MCB_DSF2REFSEL;
			bDsf2_Enb	&= ~MCB_DSF2_MN;
		} else if (dSrc == MCDRV_D2SRC_DAC1REF_ON) {
			bDsf2_Enb	|= MCB_DSF2REFBACK;
			bDsf2_Enb	|= MCB_DSF2REFSEL;
			bDsf2_Enb	&= ~MCB_DSF2_MN;
		} else {
			bDsf2_Enb	&= ~MCB_DSF2REFBACK;
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF2_PRE_INPUT);
			if ((bReg>>4) == (bReg&0x0F)
			|| (McResCtrl_HasSrc(eMCDRV_DST_ADIF2, eMCDRV_DST_CH1)
									== 0))
				bDsf2_Enb	|= MCB_DSF2_MN;
			else
				bDsf2_Enb	&= ~MCB_DSF2_MN;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DSF2_FLT_TYPE,
			bDsf2_Enb);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		if ((McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_HIFIIN_ON) != 0)
		|| (McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0) != 0))
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_DIRECTPATH_ENB,
					MCB_DIRECTPATH_ENB);
	} else {
		if (McResCtrl_HasSrc(eMCDRV_DST_HIFIOUT, eMCDRV_DST_CH0) != 0)
			bDirect_Enb	|= MCB_DIRECT_ENB_ADC;
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH1);
		if ((dSrc & MCDRV_D1SRC_HIFIIN_ON) != 0) {
			;
			bDirect_Enb	|= MCB_DIRECT_ENB_DAC0;
		}
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH1);
		if ((dSrc & MCDRV_D1SRC_HIFIIN_ON) != 0) {
			;
			bDirect_Enb	|= MCB_DIRECT_ENB_DAC1;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DIRECTPATH_ENB,
				bDirect_Enb);
	}

	/*	PDM Start	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_PDM_LOAD_TIM);
	if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_R_ON) != 0)
	|| ((dHifiSrc & (MCDRV_D2SRC_PDM0_L_ON|MCDRV_D2SRC_PDM0_R_ON)) != 0))
		bReg	|= MCB_PDM0_START;
	if ((McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_R_ON) != 0)
	|| ((dHifiSrc & (MCDRV_D2SRC_PDM1_L_ON|MCDRV_D2SRC_PDM1_R_ON)) != 0))
		bReg	|= MCB_PDM1_START;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_PDM_LOAD_TIM,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddStart", NULL);
#endif
}

/****************************************************************************
 *	GetSPath
 *
 *	Description:
 *			Get SPATH_ON register setting
 *	Arguments:
 *			none
 *	Return:
 *			SPATH_ON register setting
 *
 ****************************************************************************/
static UINT8	GetSPath(
	void
)
{
	UINT8	bSPath	= 0x7F;
	UINT32	dSrc;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetSPath");
#endif
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH0);
	if ((dSrc & MCDRV_D1SRC_MUSICIN_ON) != 0)
		bSPath	|= MCB_SPATH_ON;
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH1);
	if ((dSrc & MCDRV_D1SRC_MUSICIN_ON) != 0)
		bSPath	|= MCB_SPATH_ON;
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0);
	if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0)
		bSPath	|= MCB_SPATH_ON;
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1);
	if ((dSrc & MCDRV_D1SRC_ADIF0_ON) != 0)
		bSPath	|= MCB_SPATH_ON;

	if ((McResCtrl_HasSrc(eMCDRV_DST_AE0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE0, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE1, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE2, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE2, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE3, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_AE3, eMCDRV_DST_CH1) != 0))
		bSPath	&= (UINT8)~MCB_SPATH_ON;
	else
		bSPath	&= (UINT8)~MCB_IN_MIX_ON;

	if ((McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) == 0))
		bSPath	&= (UINT8)~MCB_OUT_MIX_ON_0;
	if ((McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) == 0))
		bSPath	&= (UINT8)~MCB_OUT_MIX_ON_1;
	else
		bSPath	&= (UINT8)~MCB_SPATH_ON;
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) == 0))
		bSPath	&= (UINT8)~MCB_OUT_MIX_ON_2;
	else
		bSPath	&= (UINT8)~MCB_SPATH_ON;
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) == 0)
	&& (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) == 0))
		bSPath	&= (UINT8)~MCB_OUT_MIX_ON_3;
	else
		bSPath	&= (UINT8)~MCB_SPATH_ON;

	dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH0);
	if ((dSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0) {
		dSrc	=
		McResCtrl_GetSource(eMCDRV_DST_DAC0, eMCDRV_DST_CH1);
		if ((dSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
			bSPath	&= (UINT8)~MCB_OUT_MIX_ON_4;
	}
	dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH0);
	if ((dSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0) {
		dSrc	=
		McResCtrl_GetSource(eMCDRV_DST_DAC1, eMCDRV_DST_CH1);
		if ((dSrc & ~MCDRV_D1SRC_HIFIIN_ON) == 0)
			bSPath	&= (UINT8)~MCB_OUT_MIX_ON_5;
	}

	if (((bSPath&(MCB_OUT_MIX_ON_4|MCB_OUT_MIX_ON_0)) == 0)
	|| ((bSPath&(MCB_IN_MIX_ON|MCB_OUT_MIX_ON_5|MCB_OUT_MIX_ON_3|
			MCB_OUT_MIX_ON_2|MCB_OUT_MIX_ON_1)) != 0))
		bSPath	&= (UINT8)~MCB_SPATH_ON;

	if ((bSPath&(MCB_SPATH_ON|MCB_OUT_MIX_ON_0))
		== (MCB_SPATH_ON|MCB_OUT_MIX_ON_0)) {
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_MUSICOUT,
					eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_MUSICOUT,
					eMCDRV_DST_CH1);
		if (dSrc != MCDRV_D1SRC_ADIF0_ON) {
			;
			bSPath	&= (UINT8)~MCB_SPATH_ON;
		}
	}
	if ((bSPath&(MCB_SPATH_ON|MCB_OUT_MIX_ON_4))
		== (MCB_SPATH_ON|MCB_OUT_MIX_ON_4)) {
		dSrc	= McResCtrl_GetSource(eMCDRV_DST_DAC0,
					eMCDRV_DST_CH0);
		dSrc	|= McResCtrl_GetSource(eMCDRV_DST_DAC0,
					eMCDRV_DST_CH1);
		if (dSrc != MCDRV_D1SRC_MUSICIN_ON)
			bSPath	&= (UINT8)~MCB_SPATH_ON;
	}


#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bSPath;
	McDebugLog_FuncOut("GetSPath", &sdRet);
#endif
	return bSPath;
}

/****************************************************************************
 *	McPacket_AddVol
 *
 *	Description:
 *			Add volume mute packet.
 *	Arguments:
 *			dUpdate			target volume items
 *			eMode			update mode
 *			pdSVolDoneParam	wait soft volume complete flag
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddVol(
	UINT32			dUpdate,
	enum MCDRV_VOLUPDATE_MODE	eMode,
	UINT32			*pdSVolDoneParam
)
{
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_VOL_INFO	sVolInfo;
	UINT8			bRegL, bRegR;
	UINT8			bVolL, bVolR;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddVol");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetVolReg(&sVolInfo);

	if ((dUpdate & MCDRV_VOLUPDATE_ANA_IN) != 0UL) {
		bVolL	= (UINT8)sVolInfo.aswA_LineIn1[0];
		bVolR	= (UINT8)sVolInfo.aswA_LineIn1[1];
		bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_LIVOL_L);
		bRegL	&= ~MCB_ALAT_LI;
		bRegR	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_LIVOL_R);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE))) {
			if ((bVolR != bRegR)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE)))
				bVolL	|= MCB_ALAT_LI;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LIVOL_L,
					bVolL);
		}
		if ((bVolR != bRegR)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE))) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LIVOL_R,
					bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Mic1[0];
		bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_MC1VOL);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE))) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_MC1VOL,
				(UINT8)bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic2[0];
		bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_MC2VOL);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE))) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_MC2VOL,
				(UINT8)bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic3[0];
		bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_MC3VOL);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE))) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_MC3VOL,
				(UINT8)bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic4[0];
		bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
					MCI_MC4VOL);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE))) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_MC4VOL,
				(UINT8)bVolL);
		}
	}

	if ((dUpdate & MCDRV_VOLUPDATE_DIN0) != 0UL) {
		/*	DIT0_INVOL	*/
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_MusicIn[0]&~MCB_DIFI0_VSEP),
			(UINT8)sVolInfo.aswD_MusicIn[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFI0_VOL0,
			MCB_DIFI0_VSEP,
			MCI_DIFI0_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DIN1) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_ExtIn[0]&~MCB_DIFI1_VSEP),
			(UINT8)sVolInfo.aswD_ExtIn[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFI1_VOL0,
			MCB_DIFI1_VSEP,
			MCI_DIFI1_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DIN2) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_VoiceIn[0]&~MCB_DIFI2_VSEP),
			(UINT8)sVolInfo.aswD_VoiceIn[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFI2_VOL0,
			MCB_DIFI2_VSEP,
			MCI_DIFI2_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DIN3) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_RefIn[0]&~MCB_DIFI3_VSEP),
			(UINT8)sVolInfo.aswD_RefIn[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFI3_VOL0,
			MCB_DIFI3_VSEP,
			MCI_DIFI3_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_ADIF0) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_Adif0In[0]&~MCB_ADI0_VSEP),
			(UINT8)sVolInfo.aswD_Adif0In[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_ADI0_VOL0,
			MCB_ADI0_VSEP,
			MCI_ADI0_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_ADIF1) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_Adif1In[0]&~MCB_ADI1_VSEP),
			(UINT8)sVolInfo.aswD_Adif1In[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_ADI1_VOL0,
			MCB_ADI1_VSEP,
			MCI_ADI1_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_ADIF2) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_Adif2In[0]&~MCB_ADI2_VSEP),
			(UINT8)sVolInfo.aswD_Adif2In[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_ADI2_VOL0,
			MCB_ADI2_VSEP,
			MCI_ADI2_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DOUT0) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_MusicOut[0]&~MCB_DIFO0_VSEP),
			(UINT8)sVolInfo.aswD_MusicOut[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFO0_VOL0,
			MCB_DIFO0_VSEP,
			MCI_DIFO0_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DOUT1) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_ExtOut[0]&~MCB_DIFO1_VSEP),
			(UINT8)sVolInfo.aswD_ExtOut[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFO1_VOL0,
			MCB_DIFO1_VSEP,
			MCI_DIFO1_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DOUT2) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_VoiceOut[0]&~MCB_DIFO2_VSEP),
			(UINT8)sVolInfo.aswD_VoiceOut[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFO2_VOL0,
			MCB_DIFO2_VSEP,
			MCI_DIFO2_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DOUT3) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_RefOut[0]&~MCB_DIFO3_VSEP),
			(UINT8)sVolInfo.aswD_RefOut[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DIFO3_VOL0,
			MCB_DIFO3_VSEP,
			MCI_DIFO3_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DAC0) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_Dac0Out[0]&~MCB_DAO0_VSEP),
			(UINT8)sVolInfo.aswD_Dac0Out[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DAO0_VOL0,
			MCB_DAO0_VSEP,
			MCI_DAO0_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DAC1) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_Dac1Out[0]&~MCB_DAO1_VSEP),
			(UINT8)sVolInfo.aswD_Dac1Out[1],
			MCDRV_PACKET_REGTYPE_MA,
			MCI_DAO1_VOL0,
			MCB_DAO1_VSEP,
			MCI_DAO1_VOL1,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DPATHDA) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_DpathDa[0]&~MCB_DPATH_DA_VSEP),
			(UINT8)sVolInfo.aswD_DpathDa[1],
			MCDRV_PACKET_REGTYPE_E,
			MCI_DPATH_DA_VOL_L,
			MCB_DPATH_DA_VSEP,
			MCI_DPATH_DA_VOL_R,
			eMode);
	}
	if ((dUpdate & MCDRV_VOLUPDATE_DIN) != 0UL) {
		;
		AddDigVolPacket(
			(UINT8)(sVolInfo.aswD_DpathAd[0]&~MCB_DPATH_AD_VSEP),
			(UINT8)sVolInfo.aswD_DpathAd[1],
			MCDRV_PACKET_REGTYPE_E,
			MCI_DPATH_AD_VOL_L,
			MCB_DPATH_AD_VSEP,
			MCI_DPATH_AD_VOL_R,
			eMode);
	}

	if ((dUpdate & MCDRV_VOLUPDATE_ANA_OUT) != 0UL) {
		*pdSVolDoneParam	= 0;

		bVolL	= (UINT8)sVolInfo.aswA_Hp[0];
		bVolR	= (UINT8)sVolInfo.aswA_Hp[1];
		bRegL	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_HPVOL_L);
		bRegL	&= (UINT8)~MCB_ALAT_HP;
		bRegR	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_HPVOL_R);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolL == MCDRV_REG_MUTE))) {
			if ((bVolR != bRegR)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			 || (bVolR == MCDRV_REG_MUTE)))
				bVolL	|= MCB_ALAT_HP;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_HPVOL_L,
					bVolL);
			if (bVolL == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= (MCB_HPL_BUSY<<8);
		}
		if ((bVolR != bRegR)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolR == MCDRV_REG_MUTE))) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_HPVOL_R,
					bVolR);
			if (bVolR == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= (MCB_HPR_BUSY<<8);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Sp[0];
		bVolR	= (UINT8)sVolInfo.aswA_Sp[1];
		bRegL	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_SPVOL_L);
		bRegL	&= (UINT8)~MCB_ALAT_SP;
		bRegR	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_SPVOL_R);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolL == MCDRV_REG_MUTE))) {
			if ((bVolR != bRegR)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			 || (bVolR == MCDRV_REG_MUTE)))
				bVolL	|= MCB_ALAT_SP;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_SPVOL_L,
					bVolL);
			if (bVolL == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= (MCB_SPL_BUSY<<8);
		}
		if ((bVolR != bRegR)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE))) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_SPVOL_R,
					bVolR);
			if (bVolR == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= (MCB_SPR_BUSY<<8);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Rc[0];
		bRegL	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_RCVOL);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolL == MCDRV_REG_MUTE))) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_RCVOL,
					bVolL);
			if (bVolL == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= MCB_RC_BUSY;
		}

		bVolL	= (UINT8)sVolInfo.aswA_LineOut1[0];
		bVolR	= (UINT8)sVolInfo.aswA_LineOut1[1];
		bRegL	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_LO1VOL_L);
		bRegL	&= (UINT8)~MCB_ALAT_LO1;
		bRegR	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_LO1VOL_R);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolL == MCDRV_REG_MUTE))) {
			if ((bVolR != bRegR)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE)))
				bVolL	|= MCB_ALAT_LO1;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LO1VOL_L,
					bVolL);
			if (bVolL == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= MCB_LO1L_BUSY;
		}
		if ((bVolR != bRegR)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE))) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LO1VOL_R,
					bVolR);
			if (bVolR == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= MCB_LO1R_BUSY;
		}

		bVolL	= (UINT8)sVolInfo.aswA_LineOut2[0];
		bVolR	= (UINT8)sVolInfo.aswA_LineOut2[1];
		bRegL	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_LO2VOL_L);
		bRegL	&= (UINT8)~MCB_ALAT_LO2;
		bRegR	=
		McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_LO2VOL_R);
		if ((bVolL != bRegL)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
		 || (bVolL == MCDRV_REG_MUTE))) {
			if ((bVolR != bRegR)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE)))
				bVolL	|= MCB_ALAT_LO2;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LO2VOL_L,
					bVolL);
			if (bVolL == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= MCB_LO2L_BUSY;
		}
		if ((bVolR != bRegR)
		&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolR == MCDRV_REG_MUTE))) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_LO2VOL_R,
					bVolR);
			if (bVolR == MCDRV_REG_MUTE)
				*pdSVolDoneParam	|= MCB_LO2R_BUSY;
		}

		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			bVolL	= (UINT8)sVolInfo.aswA_HpDet[0];
			bRegL	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_HPDETVOL);
			if ((bVolL != bRegL)
			&& ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE)))
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_HPDETVOL,
						bVolL);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddVol", NULL);
#endif
}

/****************************************************************************
 *	AddDigVolPacket
 *
 *	Description:
 *			Add digital vol setup packet.
 *	Arguments:
 *			bVolL		Left volume
 *			bVolR		Right volume
 *			dRegType	Register Type
 *			bVolLAddr	Left volume register address
 *			bVSEP		VSEP
 *			bVolRAddr	Right volume register address
 *			eMode		update mode
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDigVolPacket(
	UINT8	bVolL,
	UINT8	bVolR,
	UINT32	dRegType,
	UINT8	bVolLAddr,
	UINT8	bVSEP,
	UINT8	bVolRAddr,
	enum MCDRV_VOLUPDATE_MODE	eMode
)
{
	UINT8	bRegVolL, bRegVolR;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AddDigVolPacket");
#endif

	bRegVolL	= McResCtrl_GetRegVal((UINT16)dRegType, bVolLAddr);
	bRegVolL	&= (UINT8)~bVSEP;
	bRegVolR	= McResCtrl_GetRegVal((UINT16)dRegType, bVolRAddr);

	if (eMode == eMCDRV_VOLUPDATE_MUTE) {
		if (bVolL != MCDRV_REG_MUTE)
			bVolL	= bRegVolL;
		if (bVolR != MCDRV_REG_MUTE)
			bVolR	= bRegVolR;
	}

	if (bVolL == bVolR) {
		if ((bVolL != bRegVolL)
		|| (bVolR != bRegVolR)) {
			if ((eMode != eMCDRV_VOLUPDATE_MUTE)
			|| (bVolL == MCDRV_REG_MUTE)) {
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
						| dRegType
						| (UINT32)bVolLAddr,
						bVolL);
				McResCtrl_SetRegVal((UINT16)dRegType,
						bVolRAddr,
						bVolR);
			}
		}
	} else {
		if (eMode == eMCDRV_VOLUPDATE_MUTE) {
			if ((bVolL == MCDRV_REG_MUTE)
			&& (bVolL != bRegVolL)) {
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_FORCE_WRITE
					| dRegType
					| (UINT32)bVolLAddr,
					bVolL|bVSEP);
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_FORCE_WRITE
					| dRegType
					| (UINT32)bVolRAddr,
					bRegVolR);
			} else if ((bVolR == MCDRV_REG_MUTE)
				&& (bVolR != bRegVolR)) {
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_FORCE_WRITE
					| dRegType
					| (UINT32)bVolLAddr,
					bRegVolL|bVSEP);
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_WRITE
					| dRegType
					| (UINT32)bVolRAddr,
					bVolR);
			}
		} else {
			if (bVolL == bRegVolL) {
				if (bVolR != bRegVolR) {
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_FORCE_WRITE
						| dRegType
						| (UINT32)bVolLAddr,
						bRegVolL|bVSEP);
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_WRITE
						| dRegType
						| (UINT32)bVolRAddr,
						bVolR);
				}
			} else {
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_WRITE
					| dRegType
					| (UINT32)bVolLAddr,
					bVolL|bVSEP);
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_FORCE_WRITE
					| dRegType
					| (UINT32)bVolRAddr,
					bVolR);
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDigVolPacket", 0);
#endif
}


/****************************************************************************
 *	McPacket_AddDac0Mute
 *
 *	Description:
 *			Add Dac0 out path mute packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDac0Mute(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDac0Mute");
#endif
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_HPVOL_L,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_HPVOL_R,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_RCVOL,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_LO1VOL_L,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_LO1VOL_R,
			MCDRV_REG_MUTE);
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDac0Mute", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDac1Mute
 *
 *	Description:
 *			Add Dac1 out path mute packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDac1Mute(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDac1Mute");
#endif
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_SPVOL_L,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_SPVOL_R,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_LO2VOL_L,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_LO2VOL_R,
			MCDRV_REG_MUTE);
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDac1Mute", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDOutMute
 *
 *	Description:
 *			Add Digital out path mute packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDOutMute(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDOutMute");
#endif
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO0_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO0_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO1_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO1_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO2_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO2_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO3_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DIFO3_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO0_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO0_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO1_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO1_VOL1,
			MCDRV_REG_MUTE);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDOutMute", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddAdifMute
 *
 *	Description:
 *			Add Adif path mute packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddAdifMute(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddAdifMute");
#endif
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI0_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI0_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI1_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI1_VOL1,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI2_VOL0,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI2_VOL1,
			MCDRV_REG_MUTE);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddAdifMute", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDPathDAMute
 *
 *	Description:
 *			Add DirectPath DA mute packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDPathDAMute(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDPathDAMute");
#endif
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DPATH_DA_VOL_L,
			MCDRV_REG_MUTE);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DPATH_DA_VOL_R,
			MCDRV_REG_MUTE);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDPathDAMute", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDigitalIO
 *
 *	Description:
 *			Add DigitalI0 setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDigitalIO(
	UINT32	dUpdateInfo
)
{
	UINT8	bReg;
	UINT8	bLPort;
	struct MCDRV_DIO_INFO	sDioInfo;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_AEC_INFO	sAecInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDigitalIO");
#endif
	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetAecInfo(&sAecInfo);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	if ((bReg & (MCB_PSW_M|MCB_RST_M)) != 0)
		return;

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	if ((bReg & MCB_PM_CLK_PD) != 0)
		return;

	McResCtrl_GetDioInfo(&sDioInfo);
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
				MCI_DO0_DRV);
	bLPort	= GetLPort(MCDRV_PHYSPORT_DIO0);
	if (bLPort <= 3) {
		if (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT)
			bReg	|= MCB_BCLK0_INV;
		else
			bReg	&= (UINT8)~MCB_BCLK0_INV;
	}
	if ((sInitInfo.bPowerMode != MCDRV_POWMODE_CDSPDEBUG)
	&& (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn != 1)) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO0_DRV,
			bReg);
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
				MCI_DO1_DRV);
	bLPort	= GetLPort(MCDRV_PHYSPORT_DIO1);
	if (bLPort <= 3) {
		if (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT)
			bReg	|= MCB_BCLK1_INV;
		else
			bReg	&= (UINT8)~MCB_BCLK1_INV;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO1_DRV,
			bReg);

	bLPort	= GetLPort(MCDRV_PHYSPORT_DIO2);
	if (bLPort <= 3) {
		if (sDioInfo.asPortInfo[bLPort].sDioCommon.bBckInvert
						== MCDRV_BCLK_INVERT)
			bReg	|= MCB_BCLK2_INV;
		else
			bReg	&= (UINT8)~MCB_BCLK2_INV;
	}
	if ((sInitInfo.bPowerMode != MCDRV_POWMODE_CDSPDEBUG)
	&& (sAecInfo.sAecVBox.sAecCDspDbg.bJtagOn != 1)) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_DO1_DRV,
			bReg);
	}

	if ((dUpdateInfo & MCDRV_MUSIC_COM_UPDATE_FLAG) != 0UL)
		AddDIOCommon(eMCDRV_DIO_0);

	if ((dUpdateInfo & MCDRV_MUSIC_DIR_UPDATE_FLAG) != 0UL)
		AddDIODIR(eMCDRV_DIO_0);

	if ((dUpdateInfo & MCDRV_MUSIC_DIT_UPDATE_FLAG) != 0UL)
		AddDIODIT(eMCDRV_DIO_0);

	if ((dUpdateInfo & MCDRV_EXT_COM_UPDATE_FLAG) != 0UL)
		AddDIOCommon(eMCDRV_DIO_1);

	if ((dUpdateInfo & MCDRV_EXT_DIR_UPDATE_FLAG) != 0UL)
		AddDIODIR(eMCDRV_DIO_1);

	if ((dUpdateInfo & MCDRV_EXT_DIT_UPDATE_FLAG) != 0UL)
		AddDIODIT(eMCDRV_DIO_1);

	if ((dUpdateInfo & MCDRV_VOICE_COM_UPDATE_FLAG) != 0UL)
		AddDIOCommon(eMCDRV_DIO_2);

	if ((dUpdateInfo & MCDRV_VOICE_DIR_UPDATE_FLAG) != 0UL)
		AddDIODIR(eMCDRV_DIO_2);

	if ((dUpdateInfo & MCDRV_VOICE_DIT_UPDATE_FLAG) != 0UL)
		AddDIODIT(eMCDRV_DIO_2);

	if (((dUpdateInfo & MCDRV_HIFI_DIR_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_HIFI_DIT_UPDATE_FLAG) != 0UL)) {
		/*	LPT*_FMT, LPT*_BIT, LPR*_FMT, LPR*_BIT	*/
		bReg	= (UINT8)
			(sDioInfo.asPortInfo[eMCDRV_DIO_3].sDit.bStMode<<7
			|sDioInfo.asPortInfo[eMCDRV_DIO_3].sDit.bEdge<<4);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPT3_STMODE,
				bReg);
	}
	if ((dUpdateInfo & MCDRV_HIFI_COM_UPDATE_FLAG) != 0UL) {
		bReg	= (UINT8)
		(sDioInfo.asPortInfo[eMCDRV_DIO_3].sDioCommon.bBckFs << 4)
		| sDioInfo.asPortInfo[eMCDRV_DIO_3].sDioCommon.bFs;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LP3_BCK,
				bReg);
	}
	if (((dUpdateInfo & MCDRV_HIFI_DIR_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_HIFI_DIT_UPDATE_FLAG) != 0UL)) {
		/*	LPT*_FMT, LPT*_BIT, LPR*_FMT, LPR*_BIT	*/
		if (sDioInfo.asPortInfo[eMCDRV_DIO_3].sDioCommon.bInterface
			== MCDRV_DIO_DA) {
			bReg	= (UINT8)
		(sDioInfo.asPortInfo[eMCDRV_DIO_3].sDit.sDaFormat.bMode << 6)
		| (sDioInfo.asPortInfo[eMCDRV_DIO_3].sDit.sDaFormat.bBitSel<<4)
		| (sDioInfo.asPortInfo[eMCDRV_DIO_3].sDir.sDaFormat.bMode << 2)
		| (sDioInfo.asPortInfo[eMCDRV_DIO_3].sDir.sDaFormat.bBitSel);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_LP3_FMT,
					bReg);
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDigitalIO", NULL);
#endif
}

/****************************************************************************
 *	AddDIOCommon
 *
 *	Description:
 *			Add DigitalI0 Common setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIOCommon(
	enum MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	struct MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIOCommon");
#endif

	McResCtrl_GetDioInfo(&sDioInfo);

	if (ePort == eMCDRV_DIO_0) {
		bRegOffset	= 0;
	} else if (ePort == eMCDRV_DIO_1) {
		bRegOffset	= MCI_LP1_MODE - MCI_LP0_MODE;
	} else if (ePort == eMCDRV_DIO_2) {
		bRegOffset	= MCI_LP2_MODE - MCI_LP0_MODE;
	} else {
#if (MCDRV_DEBUG_LEVEL >= 4)
		sdRet	= MCDRV_ERROR;
		McDebugLog_FuncOut("AddDIOCommon", &sdRet);
#endif
		return;
	}

	/*	LP*_MODE	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB,
				MCI_LP0_MODE+bRegOffset);
	bReg	&= 0x0C;
	bReg	|= sDioInfo.asPortInfo[ePort].sDit.bStMode << 7;
	bReg	|= (sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs << 6);
	bReg	|= sDioInfo.asPortInfo[ePort].sDit.bEdge << 4;
	if ((sDioInfo.asPortInfo[ePort].sDioCommon.bMasterSlave
		== MCDRV_DIO_MASTER)
	&& (sDioInfo.asPortInfo[ePort].sDioCommon.bFs == MCDRV_FS_48000)) {
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			;
			bReg	|=
			(sDioInfo.asPortInfo[ePort].sDioCommon.bSrcThru << 5);
		}
	}
	bReg	|= sDioInfo.asPortInfo[ePort].sDioCommon.bInterface;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LP0_MODE+bRegOffset),
			bReg);

	/*	BCK, FS	*/
	bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bBckFs << 4)
		| sDioInfo.asPortInfo[ePort].sDioCommon.bFs;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LP0_BCK+bRegOffset),
			bReg);

	/*	HIZ_REDGE*, PCM_CLKDOWN*, PCM_FRAME*, PCM_HPERIOD*	*/
	if (sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_PCM) {
		bReg	=
			(sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHizTim << 7)
		| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmFrame << 5)
		| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHighPeriod);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)(MCI_LP0_PCM+bRegOffset),
				bReg);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDIOCommon", 0);
#endif
}

/****************************************************************************
 *	AddDIODIR
 *
 *	Description:
 *			Add DigitalI0 DIR setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIODIR(
	enum MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	struct MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIODIR");
#endif

	McResCtrl_GetDioInfo(&sDioInfo);

	if (ePort == eMCDRV_DIO_0) {
		bRegOffset	= 0;
	} else if (ePort == eMCDRV_DIO_1) {
		bRegOffset	= MCI_LP1_MODE - MCI_LP0_MODE;
	} else if (ePort == eMCDRV_DIO_2) {
		bRegOffset	= MCI_LP2_MODE - MCI_LP0_MODE;
	} else {
#if (MCDRV_DEBUG_LEVEL >= 4)
		sdRet	= MCDRV_ERROR;
		McDebugLog_FuncOut("AddDIODIR", &sdRet);
#endif
		return;
	}

	/*	LPT*_FMT, LPT*_BIT, LPR*_FMT, LPR*_BIT	*/
	bReg	=
	(sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
	| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
	| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
	| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LP0_FMT+bRegOffset),
			bReg);
	/*	LPR*_PCM_MONO, LPR*_PCM_EXTEND, LPR*_PCM_LSBON,
		LPR*_PCM_LAW, LPR*_PCM_BIT	*/
	bReg	=
	(sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bMono << 7)
	| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bOrder << 4)
	| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bLaw << 2)
	| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bBitSel);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LPR0_PCM+bRegOffset),
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDIODIR", 0);
#endif
}

/****************************************************************************
 *	AddDIODIT
 *
 *	Description:
 *			Add DigitalI0 DIT setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIODIT(
	enum MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	struct MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIODIT");
#endif

	McResCtrl_GetDioInfo(&sDioInfo);

	if (ePort == eMCDRV_DIO_0) {
		bRegOffset	= 0;
	} else if (ePort == eMCDRV_DIO_1) {
		bRegOffset	= MCI_LP1_MODE - MCI_LP0_MODE;
	} else if (ePort == eMCDRV_DIO_2) {
		bRegOffset	= MCI_LP2_MODE - MCI_LP0_MODE;
	} else {
#if (MCDRV_DEBUG_LEVEL >= 4)
		sdRet	= MCDRV_ERROR;
		McDebugLog_FuncOut("AddDIODIT", &sdRet);
#endif
		return;
	}

	/*	LPT*_FMT, LPT*_BIT	*/
	bReg	=
	(sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
	| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
	| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
	| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LP0_FMT+bRegOffset),
			bReg);
	/*	LPT*_PCM_MONO, LPT*_PCM_EXTEND, LPT*_PCM_LSBON,
		LPT*_PCM_LAW, LPT*_PCM_BIT	*/
	bReg	=
	(sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bMono << 7)
	| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bOrder << 4)
	| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bLaw << 2)
	| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bBitSel);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)(MCI_LPT0_PCM+bRegOffset),
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AddDIODIT", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDigitalIOPath
 *
 *	Description:
 *			Add DigitalI0 path setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddDigitalIOPath(
	void
)
{
	UINT8	bReg;
	struct MCDRV_DIOPATH_INFO	sDioPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddDigitalIOPath");
#endif
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	if ((bReg & (MCB_PSW_M|MCB_RST_M)) != 0)
		return;

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	if ((bReg & MCB_PM_CLK_PD) != 0)
		return;

	McResCtrl_GetDioPathInfo(&sDioPathInfo);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	if ((sDioPathInfo.abPhysPort[0] >= 4)
	|| (sDioPathInfo.abPhysPort[1] >= 4)
	|| (sDioPathInfo.abPhysPort[2] >= 4)
	|| (sDioPathInfo.abPhysPort[3] >= 4)) {
		if ((bReg & MCB_PSW_S) != 0) {
			bReg	&= MCB_PSW_S;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					bReg);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_PSW_RESET
					| (UINT32)MCI_RST<<8
					| MCB_PSW_S,
					0);
			bReg	&= (UINT8)~MCB_RST_S;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					bReg);
		}
	} else {
		if ((bReg & MCB_PSW_S) == 0) {
			bReg	|= MCB_RST_S;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					bReg);
			bReg	|= MCB_PSW_S;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					bReg);
		}
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP0_MODE);
	bReg	&= (MCB_LP0_STMODE
			|MCB_LP0_AUTO_FS
			|MCB_LP0_SRC_THRU
			|MCB_LPT0_EDGE
			|MCB_LP0_MODE);
	bReg	|= (sDioPathInfo.bMusicCh<<2);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LP0_MODE,
			bReg);

	bReg	= 0;
	bReg	|= sDioPathInfo.abMusicRSlot[0];
	bReg	|= (sDioPathInfo.abMusicRSlot[1]<<2);
	bReg	|= (sDioPathInfo.abMusicRSlot[2]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LPR0_SLOT,
			bReg);

	bReg	= 0;
	bReg	|= sDioPathInfo.abMusicTSlot[0];
	bReg	|= (sDioPathInfo.abMusicTSlot[1]<<2);
	bReg	|= (sDioPathInfo.abMusicTSlot[2]<<4);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LPT0_SLOT,
			bReg);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddDigitalIOPath", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddClockSw
 *
 *	Description:
 *			Add switch clock packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddClockSw(
	void
)
{
	UINT8	bReg;
	struct MCDRV_CLOCKSW_INFO	sClockSwInfo;
	struct MCDRV_HSDET_INFO	sHSDetInfo;
	struct MCDRV_INIT_INFO	sInitInfo;
	UINT8	bClkMsk;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddClockSw");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetClockSwInfo(&sClockSwInfo);
	McResCtrl_GetHSDet(&sHSDetInfo, NULL);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	if ((bReg & MCB_PLL_PD) != MCB_PLL_PD) {
		/*	sync	*/
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
						MCI_CLK_MSK);
		if (sClockSwInfo.bClkSrc == MCDRV_CLKSW_CLKA) {
			/*	CLKB->CLKA	*/
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI1)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_RTCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_SBCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI0)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_RTCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_SBCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLK_MSK,
					bReg);

			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
							MCI_CLKSRC);
			if ((bReg & MCB_CLK_INPUT) != 0)
				bReg	|= MCB_CLKSRC;
			else
				bReg	&= (UINT8)~MCB_CLKSRC;

			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLKSRC,
					bReg);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_CLKBUSY_RESET,
					0);

			bReg	= MCI_CLK_MSK_DEF;
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI1)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_RTCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_SBCK)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI0)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_RTCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_SBCK)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLK_MSK,
					bReg);
			bClkMsk	= bReg;
		} else {
			/*	CLKA->CLKB	*/
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI1_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_RTC_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_SBCK_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI0_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_RTC_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_SBCK_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLK_MSK,
					bReg);

			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
							MCI_CLKSRC);
			if ((bReg & MCB_CLK_INPUT) == 0)
				bReg	|= MCB_CLKSRC;
			else
				bReg	&= (UINT8)~MCB_CLKSRC;

			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLKSRC,
					bReg);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_CLKBUSY_RESET,
					0);

			bReg	= MCI_CLK_MSK_DEF;
			if ((sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI1_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_RTC_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_SBCK_CLKI0)
			|| (sInitInfo.bCkInput == MCDRV_CKINPUT_CLKI0_CLKI0)) {
				bReg	&= (UINT8)~MCB_CLKI0_MSK;
			} else if ((sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI0_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_RTC_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_SBCK_CLKI1)
				|| (sInitInfo.bCkInput
					== MCDRV_CKINPUT_CLKI1_CLKI1)) {
				bReg	&= (UINT8)~MCB_CLKI1_MSK;
			} else {
				bReg	&= (UINT8)~MCB_RTCI_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_CLK_MSK,
					bReg);
			bClkMsk	= bReg;
		}
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD,
						MCI_CKSEL);
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			if ((bClkMsk&MCB_RTCI_MSK) == 0) {
				;
				bReg	&= (UINT8)~MCB_HSDET;
			} else if ((sHSDetInfo.bEnPlugDetDb ==
						MCDRV_PLUGDETDB_DISABLE)
			&& (sHSDetInfo.bEnDlyKeyOff == MCDRV_KEYEN_D_D_D)
			&& (sHSDetInfo.bEnDlyKeyOn == MCDRV_KEYEN_D_D_D)
			&& (sHSDetInfo.bEnMicDet == MCDRV_MICDET_DISABLE)
			&& (sHSDetInfo.bEnKeyOff == MCDRV_KEYEN_D_D_D)
			&& (sHSDetInfo.bEnKeyOn == MCDRV_KEYEN_D_D_D)) {
				bReg	|= MCB_HSDET;
			}
		} else {
			if ((bClkMsk&MCB_RTCI_MSK) == 0) {
				;
				bReg	&= (UINT8)~MCB_CRTC;
			} else {
				bReg	|= MCB_CRTC;
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_CKSEL,
				bReg);
	} else {
		/*	async	*/
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
						MCI_CLKSRC);
		if (sClockSwInfo.bClkSrc == MCDRV_CLKSW_CLKA) {
			if ((bReg & MCB_CLKSRC) != 0) {
				;
				bReg	|= MCB_CLK_INPUT;
			} else {
				bReg	&= (UINT8)~MCB_CLK_INPUT;
			}
		} else {
			if ((bReg & MCB_CLKSRC) == 0) {
				;
				bReg	|= MCB_CLK_INPUT;
			} else {
				bReg	&= (UINT8)~MCB_CLK_INPUT;
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_CLKSRC,
				bReg);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddClockSwitch", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddSwap
 *
 *	Description:
 *			Add Swap packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddSwap(
	UINT32	dUpdateInfo
)
{
	struct MCDRV_SWAP_INFO	sSwapInfo;
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddSwap");
#endif
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST);
	if ((bReg & (MCB_PSW_M|MCB_RST_M)) != 0)
		return;

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PD);
	if ((bReg & MCB_PM_CLK_PD) != 0)
		return;

	McResCtrl_GetSwap(&sSwapInfo);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, MCI_ADI_SWAP);
	if ((dUpdateInfo & MCDRV_SWAP_ADIF0_UPDATE_FLAG) != 0) {
		bReg	&= 0xF0;
		bReg	|= sSwapInfo.bAdif0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_ADIF1_UPDATE_FLAG) != 0) {
		bReg	&= 0x0F;
		bReg	|= sSwapInfo.bAdif1<<4;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_ADI_SWAP,
			bReg);

	if ((dUpdateInfo & MCDRV_SWAP_ADIF2_UPDATE_FLAG) != 0) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MA
				| (UINT32)MCI_ADI2_SWAP,
				sSwapInfo.bAdif2);
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA, MCI_DAO_SWAP);
	if ((dUpdateInfo & MCDRV_SWAP_DAC0_UPDATE_FLAG) != 0) {
		bReg	&= 0xF0;
		bReg	|= sSwapInfo.bDac0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_DAC1_UPDATE_FLAG) != 0) {
		bReg	&= 0x0F;
		bReg	|= sSwapInfo.bDac1<<4;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO_SWAP,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LPR0_SWAP);
	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN0_UPDATE_FLAG) != 0) {
		bReg	&= ~0x03;
		bReg	|= sSwapInfo.bMusicIn0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN1_UPDATE_FLAG) != 0) {
		bReg	&= ~0x0C;
		bReg	|= sSwapInfo.bMusicIn1<<2;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN2_UPDATE_FLAG) != 0) {
		bReg	&= ~0x30;
		bReg	|= sSwapInfo.bMusicIn2<<4;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LPR0_SWAP,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LPT0_SWAP);
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT0_UPDATE_FLAG) != 0) {
		bReg	&= ~0x07;
		bReg	|= sSwapInfo.bMusicOut0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT1_UPDATE_FLAG) != 0) {
		bReg	&= ~0x18;
		bReg	|= sSwapInfo.bMusicOut1<<3;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT2_UPDATE_FLAG) != 0) {
		bReg	&= ~0x60;
		bReg	|= sSwapInfo.bMusicOut2<<5;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MB
			| (UINT32)MCI_LPT0_SWAP,
			bReg);

	if ((dUpdateInfo & MCDRV_SWAP_EXTIN_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bExtIn;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPR1_SWAP,
				bReg);
	}

	if ((dUpdateInfo & MCDRV_SWAP_EXTOUT_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bExtOut;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPT1_SWAP,
				bReg);
	}

	if ((dUpdateInfo & MCDRV_SWAP_VOICEIN_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bVoiceIn;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPR2_SWAP,
				bReg);
	}

	if ((dUpdateInfo & MCDRV_SWAP_VOICEOUT_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bVoiceOut;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPT2_SWAP,
				bReg);
	}

	if ((dUpdateInfo & MCDRV_SWAP_HIFIIN_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bHifiIn;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPR3_SWAP,
				bReg);
	}

	if ((dUpdateInfo & MCDRV_SWAP_HIFIOUT_UPDATE_FLAG) != 0) {
		bReg	= sSwapInfo.bHifiOut;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| (UINT32)MCI_LPT3_SWAP,
				bReg);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddSwap", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddHSDet
 *
 *	Description:
 *			Add Headset Det packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddHSDet(
	void
)
{
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_HSDET_INFO	sHSDetInfo;
	struct MCDRV_HSDET2_INFO	sHSDet2Info;
	UINT8	bClkMsk;
	UINT8	abData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddHSDet");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetHSDet(&sHSDetInfo, &sHSDet2Info);

	if ((sHSDetInfo.bEnPlugDetDb == MCDRV_PLUGDETDB_DISABLE)
	&& (sHSDetInfo.bEnDlyKeyOff == MCDRV_KEYEN_D_D_D)
	&& (sHSDetInfo.bEnDlyKeyOn == MCDRV_KEYEN_D_D_D)
	&& (sHSDetInfo.bEnMicDet == MCDRV_MICDET_DISABLE)
	&& (sHSDetInfo.bEnKeyOff == MCDRV_KEYEN_D_D_D)
	&& (sHSDetInfo.bEnKeyOn == MCDRV_KEYEN_D_D_D)) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQHS,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EPLUGDET,
				(sHSDetInfo.bEnPlugDet<<7));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EDLYKEY,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EMICDET,
				0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_HSDETEN,
				sHSDetInfo.bHsDetDbnc);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_KDSET,
				sInitInfo.bMbsDisch<<4);

		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD,
						MCI_CKSEL);
		bReg	&= (UINT8)~MCB_CKSEL0;
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			bClkMsk	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
							MCI_CLK_MSK);
			if ((bClkMsk&MCB_RTCI_MSK) != 0) {
				;
				bReg	|= MCB_HSDET;
			}
		} else {
			bReg	|= MCB_HSDET;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_CKSEL,
				bReg);
		McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_DP_OSC,
				MCB_DP_OSC);
	} else {
		if ((sHSDetInfo.bEnPlugDetDb != MCDRV_PLUGDETDB_DISABLE)
		|| (sHSDetInfo.bEnDlyKeyOff != MCDRV_KEYEN_D_D_D)
		|| (sHSDetInfo.bEnDlyKeyOn != MCDRV_KEYEN_D_D_D)
		|| (sHSDetInfo.bEnMicDet != MCDRV_MICDET_DISABLE)
		|| (sHSDetInfo.bEnKeyOff != MCDRV_KEYEN_D_D_D)
		|| (sHSDetInfo.bEnKeyOn != MCDRV_KEYEN_D_D_D)) {
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD,
							MCI_HSDETEN);
			if ((bReg&MCB_HSDETEN) == 0) {
				if (sInitInfo.bHsdetClk == MCDRV_HSDETCLK_OSC) {
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_DP_OSC,
						0);
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_OSC_WAIT_TIME, 0);
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_HSDETEN,
						sHSDetInfo.bHsDetDbnc);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_HSDETMODE,
						sHSDetInfo.bHsDetMode);
				bReg	= (UINT8)(sHSDetInfo.bSperiod<<3)
					| MCB_DBNC_LPERIOD;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_DBNC_PERIOD,
						bReg);
				bReg	= (UINT8)((sHSDetInfo.bDbncNumPlug<<4)
					| (sHSDetInfo.bDbncNumMic<<2)
					| sHSDetInfo.bDbncNumKey);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_DBNC_NUM,
						bReg);
				bReg	= (UINT8)((sHSDetInfo.bKeyOffMtim<<1)
					| sHSDetInfo.bKeyOnMtim);
				if (McDevProf_GetDevId() !=
							eMCDRV_DEV_ID_80_90H) {
					;
					bReg	|= MCI_KEY_MTIM_DEF;
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEY_MTIM,
						bReg);
				bReg	= (UINT8)((sHSDetInfo.bKey0OnDlyTim2<<4)
					| sHSDetInfo.bKey0OnDlyTim);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYONDLYTIM_K0,
						bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYOFFDLYTIM_K0,
						sHSDetInfo.bKey0OffDlyTim);
				bReg	= (UINT8)((sHSDetInfo.bKey1OnDlyTim2<<4)
					| sHSDetInfo.bKey1OnDlyTim);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYONDLYTIM_K1,
						bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYOFFDLYTIM_K1,
						sHSDetInfo.bKey1OffDlyTim);
				bReg	= (UINT8)((sHSDetInfo.bKey2OnDlyTim2<<4)
					| sHSDetInfo.bKey2OnDlyTim);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYONDLYTIM_K2,
						bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_CD
						| (UINT32)MCI_KEYOFFDLYTIM_K2,
						sHSDetInfo.bKey2OffDlyTim);

				bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_CD,
						MCI_CKSEL);
				bReg	&= (UINT8)~MCB_HSDET;
				if (sInitInfo.bHsdetClk == MCDRV_HSDETCLK_OSC
				) {
					bReg	|= MCB_CKSEL0;
				}
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_CD
					| (UINT32)MCI_CKSEL,
					bReg);
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQHS,
				MCB_EIRQHS);
		bReg	= (sHSDetInfo.bEnPlugDet<<7) | sHSDetInfo.bEnPlugDetDb;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EPLUGDET,
				bReg);
		bReg	= (UINT8)((sHSDetInfo.bEnDlyKeyOff<<3)
			| sHSDetInfo.bEnDlyKeyOn);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EDLYKEY,
				bReg);
		bReg	= (UINT8)((sHSDetInfo.bEnMicDet<<6)
			| (sHSDetInfo.bEnKeyOff<<3)
			| sHSDetInfo.bEnKeyOn);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_EMICDET,
				bReg);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_IRQR,
				MCB_EIRQR);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQTYPE,
				sHSDetInfo.bIrqType);
	} else {
		if (sHSDetInfo.bIrqType == MCDRV_IRQTYPE_NORMAL) {
			bReg	= 0;
		} else if (sHSDetInfo.bIrqType == MCDRV_IRQTYPE_REF) {
			bReg	= MCI_IRQTYPE_DEF;
		} else {
			bReg	= (UINT8)(sHSDet2Info.bPlugDetIrqType << 7)
				| (sHSDet2Info.bMicDetIrqType << 6)
				| (sHSDet2Info.bPlugUndetDbIrqType << 1)
				| sHSDet2Info.bPlugDetDbIrqType;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQTYPE,
				bReg);
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DLYIRQSTOP,
			sHSDetInfo.bDlyIrqStop);
	bReg	= sHSDetInfo.bDetInInv;
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		if (sHSDetInfo.bIrqType == MCDRV_IRQTYPE_EX) {
			bReg	|= (UINT8)(sHSDet2Info.bKey2OnIrqType << 7);
			bReg	|= (UINT8)(sHSDet2Info.bKey1OnIrqType << 6);
			bReg	|= (UINT8)(sHSDet2Info.bKey0OnIrqType << 5);
		} else {
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 7);
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 6);
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 5);
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DETIN_INV,
			bReg);
	if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
		abData[0]	= MCI_CD_REG_A<<1;
		abData[1]	= MCI_IRQM_KEYOFF;
		McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 2);
		bReg	= McSrv_ReadReg(MCDRV_SLAVEADDR_ANA,
						(UINT32)MCI_CD_REG_D);
		bReg	&= 1;
		if (sHSDetInfo.bIrqType == MCDRV_IRQTYPE_EX) {
			bReg	|= (UINT8)(sHSDet2Info.bKey2OffIrqType << 7);
			bReg	|= (UINT8)(sHSDet2Info.bKey1OffIrqType << 6);
			bReg	|= (UINT8)(sHSDet2Info.bKey0OffIrqType << 5);
		} else {
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 7);
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 6);
			bReg	|= (UINT8)(sHSDetInfo.bIrqType << 5);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_IRQM_KEYOFF,
				bReg);
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_HSDETMODE,
			sHSDetInfo.bHsDetMode);
	bReg	= (UINT8)(sHSDetInfo.bSperiod<<3)
		| MCB_DBNC_LPERIOD;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DBNC_PERIOD,
			bReg);
	bReg	= (UINT8)((sHSDetInfo.bDbncNumPlug<<4)
		| (sHSDetInfo.bDbncNumMic<<2)
		| sHSDetInfo.bDbncNumKey);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DBNC_NUM,
			bReg);
	bReg	= (UINT8)((sHSDetInfo.bKeyOffMtim<<1)
		| sHSDetInfo.bKeyOnMtim);
	if (McDevProf_GetDevId() !=
				eMCDRV_DEV_ID_80_90H) {
		;
		bReg	|= MCI_KEY_MTIM_DEF;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEY_MTIM,
			bReg);
	bReg	= (UINT8)((sHSDetInfo.bKey0OnDlyTim2<<4)
		| sHSDetInfo.bKey0OnDlyTim);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYONDLYTIM_K0,
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYOFFDLYTIM_K0,
			sHSDetInfo.bKey0OffDlyTim);
	bReg	= (UINT8)((sHSDetInfo.bKey1OnDlyTim2<<4)
		| sHSDetInfo.bKey1OnDlyTim);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYONDLYTIM_K1,
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYOFFDLYTIM_K1,
			sHSDetInfo.bKey1OffDlyTim);
	bReg	= (UINT8)((sHSDetInfo.bKey2OnDlyTim2<<4)
		| sHSDetInfo.bKey2OnDlyTim);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYONDLYTIM_K2,
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_KEYOFFDLYTIM_K2,
			sHSDetInfo.bKey2OffDlyTim);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddHSDet", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddMKDetEnable
 *
 *	Description:
 *			Add Mic/Key Det Enable packet.
 *	Arguments:
 *			bCheckPlugDetDB	1:check PlugDetDB
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddMKDetEnable(
	UINT8	bCheckPlugDetDB
)
{
	UINT8	bReg;
	struct MCDRV_HSDET_INFO	sHSDetInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddMKDetEnable");
#endif
	if (bCheckPlugDetDB != 0) {
		bReg	= McResCtrl_GetPlugDetDB();
		if ((bReg&MCB_RPLUGDET_DB) == 0) {
			;
			goto exit;
		}
	}

	McResCtrl_GetHSDet(&sHSDetInfo, NULL);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	bReg	&= (UINT8)~MCB_AP_BGR;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_AP,
			bReg);

	bReg	=
	McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_KDSET);
	bReg	&= ~MCB_KDSET2;
	bReg	|= MCB_KDSET1;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_KDSET,
			bReg);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
			| 2000UL,
			0);

	bReg	&= ~MCB_KDSET1;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_KDSET,
			bReg);

	bReg	|= MCB_KDSET2;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_ANA
			| (UINT32)MCI_KDSET,
			bReg);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
			| 4000UL,
			0);

	bReg	= (UINT8)(MCB_HSDETEN | MCB_MKDETEN | sHSDetInfo.bHsDetDbnc);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_HSDETEN,
			bReg);

exit:;
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddMKDetEnable", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddAEC
 *
 *	Description:
 *			Add AEC packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddAEC(
	void
)
{
	UINT8	bReg;
	struct MCDRV_AEC_INFO	sInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddAEC");
#endif

	McResCtrl_GetAecInfo(&sInfo);

	if (sInfo.sAecVBox.sAecCDspDbg.bJtagOn == 1) {
		bReg	= 0x01;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_DO0_DRV,
				bReg);
		bReg	= 0x22;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_DO1_DRV,
				bReg);
	}

	if (sInfo.sAecConfig.bFDspLocate == 0)
		bReg	= MCB_FDSP_PI_SOURCE_AE;
	else
		bReg	= MCB_FDSP_EX_SYNC | MCB_FDSP_PI_SOURCE_VDSP_Ex;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_FDSP_PI_SOURCE,
			bReg);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_FDSP_PO_SOURCE,
			sInfo.sAecVBox.bFdsp_Po_Source);

	bReg	= (UINT8)(sInfo.sAecAudioengine.bMixerIn3Src<<7)
		| (UINT8)(sInfo.sAecAudioengine.bMixerIn2Src<<6)
		| (UINT8)(sInfo.sAecAudioengine.bMixerIn1Src<<5)
		| (UINT8)(sInfo.sAecAudioengine.bMixerIn0Src<<4)
		| (UINT8)(sInfo.sAecAudioengine.bBDspAE1Src<<1)
		| sInfo.sAecAudioengine.bBDspAE0Src;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_BDSP_SOURCE,
			bReg);

	bReg	= (UINT8)(sInfo.sAecVBox.bLPt2_VSource<<5)
		| (UINT8)(sInfo.sAecVBox.bISrc3_VSource<<4)
		| (UINT8)(sInfo.sAecVBox.bISrc2_Ch1_VSource<<3)
		| (UINT8)(sInfo.sAecVBox.bISrc2_VSource<<2)
		| sInfo.sAecVBox.bSrc3_Ctrl;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_SRC_VSOURCE,
			bReg);
	bReg	= (UINT8)(sInfo.sAecVBox.bLPt2_Mix_VolO<<4)
		| sInfo.sAecVBox.bLPt2_Mix_VolI;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_LPT2_MIX_VOL,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PCMOUT_HIZ);
	bReg	&= (MCB_PCMOUT2_HIZ|MCB_PCMOUT1_HIZ|MCB_PCMOUT0_HIZ);
	bReg	|= sInfo.sPdm.bPdm1_Data_Delay<<5;
	bReg	|= sInfo.sPdm.bPdm0_Data_Delay<<4;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_A
			| (UINT32)MCI_PCMOUT_HIZ,
			bReg);
	SetupEReg();

	bReg	= (UINT8)(sInfo.sOutput.bDng_Release<<4)
		| sInfo.sOutput.bDng_Attack;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_ES1,
				bReg);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_HP_ES1,
				sInfo.sOutput.bDng_Target[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_SP_ES1,
				sInfo.sOutput.bDng_Target[1]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_RC_ES1,
				sInfo.sOutput.bDng_Target_Rc);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_LO1_ES1,
				sInfo.sOutput.bDng_Target_LineOut[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_LO2_ES1,
				sInfo.sOutput.bDng_Target_LineOut[1]);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG,
				bReg);

		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_HP,
				sInfo.sOutput.bDng_Target[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_SP,
				sInfo.sOutput.bDng_Target[1]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_RC,
				sInfo.sOutput.bDng_Target_Rc);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_LO1,
				sInfo.sOutput.bDng_Target_LineOut[0]);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_DNG_LO2,
				sInfo.sOutput.bDng_Target_LineOut[1]);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddAEC", NULL);
#endif
}


/****************************************************************************
 *	McPacket_AddGPMode
 *
 *	Description:
 *			Add GP mode setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McPacket_AddGPMode(
	void
)
{
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_GP_MODE	sGPMode;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddGPMode");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetGPMode(&sGPMode);

	if (sInitInfo.bPa0Func == MCDRV_PA_GPIO) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA0);
		if (sGPMode.abGpDdr[0] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA0_DDR;
		else
			bReg	|= MCB_PA0_DDR;
		if (sGPMode.abGpHost[0] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA0_OUTSEL;
		else
			bReg	|= MCB_PA0_OUTSEL;
		if (sGPMode.abGpInvert[0] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA0_INV;
		else
			bReg	|= MCB_PA0_INV;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA0,
				bReg);
	}
	if (sInitInfo.bPa1Func == MCDRV_PA_GPIO) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA1);
		if (sGPMode.abGpDdr[1] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA1_DDR;
		else
			bReg	|= MCB_PA1_DDR;
		if (sGPMode.abGpHost[1] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA1_OUTSEL;
		else
			bReg	|= MCB_PA1_OUTSEL;
		if (sGPMode.abGpInvert[1] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA1_INV;
		else
			bReg	|= MCB_PA1_INV;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA1,
				bReg);
	}
	if (sInitInfo.bPa2Func == MCDRV_PA_GPIO) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA2);
		if (sGPMode.abGpDdr[2] == MCDRV_GPDDR_IN)
			bReg	&= (UINT8)~MCB_PA2_DDR;
		else
			bReg	|= MCB_PA2_DDR;
		if (sGPMode.abGpHost[2] == MCDRV_GPHOST_CPU)
			bReg	&= (UINT8)~MCB_PA2_OUTSEL;
		else
			bReg	|= MCB_PA2_OUTSEL;
		if (sGPMode.abGpInvert[2] == MCDRV_GPINV_NORMAL)
			bReg	&= (UINT8)~MCB_PA2_INV;
		else
			bReg	|= MCB_PA2_INV;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA2,
				bReg);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddGPMode", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddGPMask
 *
 *	Description:
 *			Add GP mask setup packet.
 *	Arguments:
 *			dPadNo	PAD Number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
void	McPacket_AddGPMask(
	UINT32	dPadNo
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddGPMask");
#endif

	switch (dPadNo) {
	case	0:
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA0);
		if (McResCtrl_GetGPMask(dPadNo) == MCDRV_GPMASK_OFF)
			bReg	&= (UINT8)~MCB_PA0_MSK;
		else
			bReg	|= MCB_PA0_MSK;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA0,
				bReg);
		break;
	case	1:
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA1);
		if (McResCtrl_GetGPMask(dPadNo) == MCDRV_GPMASK_OFF)
			bReg	&= (UINT8)~MCB_PA1_MSK;
		else
			bReg	|= MCB_PA1_MSK;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA1,
				bReg);
		break;
	case	2:
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA2);
		if (McResCtrl_GetGPMask(dPadNo) == MCDRV_GPMASK_OFF)
			bReg	&= (UINT8)~MCB_PA2_MSK;
		else
			bReg	|= MCB_PA2_MSK;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_PA2,
				bReg);
		break;
	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddGPMask", NULL);
#endif
}

/****************************************************************************
 *	McPacket_AddGPSet
 *
 *	Description:
 *			Add GPIO output packet.
 *	Arguments:
 *			dPadNo	PAD Number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
void	McPacket_AddGPSet(
	UINT32	dPadNo
)
{
	UINT8	bReg;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_GP_MODE	sGPMode;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McPacket_AddGPSet");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetGPMode(&sGPMode);

	switch (dPadNo) {
	case	0:
		if ((sInitInfo.bPa0Func == MCDRV_PA_GPIO)
		&& (sGPMode.abGpDdr[0] == MCDRV_GPDDR_OUT)
		&& (sGPMode.abGpHost[0] == MCDRV_GPHOST_CPU)) {
			bReg	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA0);
			bReg	&= (UINT8)~MCB_PA0_DATA;
			bReg	|= McResCtrl_GetGPPad(dPadNo)<<4;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_PA0,
					bReg);
		}
		break;
	case	1:
		if ((sInitInfo.bPa1Func == MCDRV_PA_GPIO)
		&& (sGPMode.abGpDdr[1] == MCDRV_GPDDR_OUT)
		&& (sGPMode.abGpHost[1] == MCDRV_GPHOST_CPU)) {
			bReg	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA1);
			bReg	&= (UINT8)~MCB_PA1_DATA;
			bReg	|= McResCtrl_GetGPPad(dPadNo)<<4;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_PA1,
					bReg);
		}
		break;
	case	2:
		if ((sInitInfo.bPa2Func == MCDRV_PA_GPIO)
		&& (sGPMode.abGpDdr[2] == MCDRV_GPDDR_OUT)
		&& (sGPMode.abGpHost[2] == MCDRV_GPHOST_CPU)) {
			bReg	=
			McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_PA2);
			bReg	&= (UINT8)~MCB_PA2_DATA;
			bReg	|= McResCtrl_GetGPPad(dPadNo)<<4;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_A
					| (UINT32)MCI_PA2,
					bReg);
		}
		break;
	default:
		break;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McPacket_AddGPSet", NULL);
#endif
}

/****************************************************************************
 *	DSPCallback
 *
 *	Description:
 *			DSP callback function.
 *	Arguments:
 *			sdHd		function no.
 *			dEvtType	event type
 *			dEvtPrm		event param
 *	Return:
 *			0
 *
 ****************************************************************************/
static SINT32	DSPCallback(
	SINT32	sdHd,
	UINT32	dEvtType,
	UINT32	dEvtPrm
)
{
	SINT32	sdRet	= MCDRV_ERROR;
	UINT8	bReg;
	SINT32 (*pcbfunc)(SINT32, UINT32, UINT32);

	McSrv_Unlock();
	if (dEvtType == FDSP_CB_RESET) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF,
						MCI_RST);
		if ((bReg&MCB_PSW_F) == 0) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					(UINT8)(bReg|MCB_RST_F));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_RST,
					bReg);
			sdRet	= McDevIf_ExecutePacket();
		} else {
			sdRet	= MCDRV_SUCCESS;
		}
		goto exit;
	}

	McResCtrl_GetDSPCBFunc(&pcbfunc);
	if (pcbfunc != NULL) {
		;
		sdRet	= (pcbfunc)(sdHd, dEvtType, dEvtPrm);
	}
exit:
	McSrv_Lock();
	return sdRet;
}

/****************************************************************************
 *	GetMaxWait
 *
 *	Description:
 *			Get maximum wait time.
 *	Arguments:
 *			bRegChange	analog power management
 *					register update information
 *	Return:
 *			wait time
 *
 ****************************************************************************/
static UINT32	GetMaxWait
(
	UINT8	bRegChange
)
{
	UINT32	dWaitTimeMax	= 0;
	struct MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetMaxWait");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);

	if (((bRegChange & MCB_MC1) != 0)
	|| ((bRegChange & MCB_MB1) != 0))
		if (sInitInfo.sWaitTime.dWaitTime[0] > dWaitTimeMax)
			dWaitTimeMax	= sInitInfo.sWaitTime.dWaitTime[0];

	if (((bRegChange & MCB_MC2) != 0)
	|| ((bRegChange & MCB_MB2) != 0))
		if (sInitInfo.sWaitTime.dWaitTime[1] > dWaitTimeMax)
			dWaitTimeMax	= sInitInfo.sWaitTime.dWaitTime[1];

	if (((bRegChange & MCB_MC3) != 0)
	|| ((bRegChange & MCB_MB3) != 0))
		if (sInitInfo.sWaitTime.dWaitTime[2] > dWaitTimeMax)
			dWaitTimeMax	= sInitInfo.sWaitTime.dWaitTime[2];

	if (((bRegChange & MCB_MC4) != 0)
	|| ((bRegChange & MCB_MB4) != 0))
		if (sInitInfo.sWaitTime.dWaitTime[3] > dWaitTimeMax)
			dWaitTimeMax	= sInitInfo.sWaitTime.dWaitTime[3];

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)dWaitTimeMax;
	McDebugLog_FuncOut("GetMaxWait", &sdRet);
#endif

	return dWaitTimeMax;
}


