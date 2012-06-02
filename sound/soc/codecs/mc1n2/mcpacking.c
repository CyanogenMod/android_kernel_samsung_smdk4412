/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcpacking.c
 *
 *		Description	: MC Driver packet packing control
 *
 *		Version		: 1.0.0 	2010.09.10
 *
 ****************************************************************************/


#include "mcpacking.h"
#include "mcdevif.h"
#include "mcresctrl.h"
#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcservice.h"
#include "mcmachdep.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif



#define	MCDRV_TCXO_WAIT_TIME	((UINT32)2000)
#define	MCDRV_PLRST_WAIT_TIME	((UINT32)2000)
#define	MCDRV_LDOA_WAIT_TIME	((UINT32)1000)
#define	MCDRV_SP_WAIT_TIME		((UINT32)150)
#define	MCDRV_HP_WAIT_TIME		((UINT32)300)
#define	MCDRV_RC_WAIT_TIME		((UINT32)150)

/* SrcRate default */
#define	MCDRV_DIR_SRCRATE_48000	(32768)
#define	MCDRV_DIR_SRCRATE_44100	(30106)
#define	MCDRV_DIR_SRCRATE_32000	(21845)
#define	MCDRV_DIR_SRCRATE_24000	(16384)
#define	MCDRV_DIR_SRCRATE_22050	(15053)
#define	MCDRV_DIR_SRCRATE_16000	(10923)
#define	MCDRV_DIR_SRCRATE_12000	(8192)
#define	MCDRV_DIR_SRCRATE_11025	(7526)
#define	MCDRV_DIR_SRCRATE_8000	(5461)

#define	MCDRV_DIT_SRCRATE_48000	(4096)
#define	MCDRV_DIT_SRCRATE_44100	(4458)
#define	MCDRV_DIT_SRCRATE_32000	(6144)
#define	MCDRV_DIT_SRCRATE_24000	(8192)
#define	MCDRV_DIT_SRCRATE_22050	(8916)
#define	MCDRV_DIT_SRCRATE_16000	(12288)
#define	MCDRV_DIT_SRCRATE_12000	(16384)
#define	MCDRV_DIT_SRCRATE_11025	(17833)
#define	MCDRV_DIT_SRCRATE_8000	(24576)

static SINT32	AddAnalogPowerUpAuto(const MCDRV_POWER_INFO* psPowerInfo, const MCDRV_POWER_UPDATE* psPowerUpdate);

static void		AddDIPad(void);
static void		AddPAD(void);
static SINT32	AddSource(void);
static void		AddDIStart(void);
static UINT8	GetMicMixBit(const MCDRV_CHANNEL* psChannel);
static void		AddDigVolPacket(UINT8 bVolL, UINT8 bVolR, UINT8 bVolLAddr, UINT8 bLAT, UINT8 bVolRAddr, MCDRV_VOLUPDATE_MODE eMode);
static void		AddStopADC(void);
static void		AddStopPDM(void);
static UINT8	IsModifiedDIO(UINT32 dUpdateInfo);
static UINT8	IsModifiedDIOCommon(MCDRV_DIO_PORT_NO ePort);
static UINT8	IsModifiedDIODIR(MCDRV_DIO_PORT_NO ePort);
static UINT8	IsModifiedDIODIT(MCDRV_DIO_PORT_NO ePort);
static void		AddDIOCommon(MCDRV_DIO_PORT_NO ePort);
static void		AddDIODIR(MCDRV_DIO_PORT_NO ePort);
static void		AddDIODIT(MCDRV_DIO_PORT_NO ePort);

#define	MCDRV_DPB_KEEP	0
#define	MCDRV_DPB_UP	1
static SINT32	PowerUpDig(UINT8 bDPBUp);

static UINT32	GetMaxWait(UINT8 bRegChange);

/****************************************************************************
 *	McPacket_AddInit
 *
 *	Description:
 *			Add initialize packet.
 *	Arguments:
 *			psInitInfo		information for initialization
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddInit
(
	const MCDRV_INIT_INFO*	psInitInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddInit");
#endif


	/*	RSTA	*/
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_RST, MCB_RST);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_RST, 0);

	/*	ANA_RST	*/
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ANA_RST, MCI_ANA_RST_DEF);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ANA_RST, 0);

	/*	SDIN_MSK*, SDO_DDR*	*/
	bReg	= MCB_SDIN_MSK2;
	if(psInitInfo->bDioSdo2Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_SDO_DDR2;
	}
	bReg |= MCB_SDIN_MSK1;
	if(psInitInfo->bDioSdo1Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_SDO_DDR1;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_SD_MSK, bReg);

	bReg	= MCB_SDIN_MSK0;
	if(psInitInfo->bDioSdo0Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_SDO_DDR0;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_SD_MSK_1, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*, PCM_HIZ*	*/
	bReg	= 0;
	bReg |= MCB_BCLK_MSK2;
	bReg |= MCB_LRCK_MSK2;
	if(psInitInfo->bDioClk2Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_BCLK_DDR2;
		bReg |= MCB_LRCK_DDR2;
	}
	bReg |= MCB_BCLK_MSK1;
	bReg |= MCB_LRCK_MSK1;
	if(psInitInfo->bDioClk1Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_BCLK_DDR1;
		bReg |= MCB_LRCK_DDR1;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_BCLK_MSK, bReg);

	bReg	= 0;
	bReg |= MCB_BCLK_MSK0;
	bReg |= MCB_LRCK_MSK0;
	if(psInitInfo->bDioClk0Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_BCLK_DDR0;
		bReg |= MCB_LRCK_DDR0;
	}
	if(psInitInfo->bPcmHiz == MCDRV_PCMHIZ_HIZ)
	{
		bReg |= (MCB_PCMOUT_HIZ2 | MCB_PCMOUT_HIZ1 | MCB_PCMOUT_HIZ0);
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_BCLK_MSK_1, bReg);

	/*	DI*_BCKP	*/

	/*	PA*_MSK, PA*_DDR	*/
	bReg	= MCI_PA_MSK_DEF;
	if(psInitInfo->bPad0Func == MCDRV_PAD_PDMCK)
	{
		bReg	|= MCB_PA0_DDR;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_MSK, bReg);

	/*	PA0_OUT	*/
	if(psInitInfo->bPad0Func == MCDRV_PAD_PDMCK)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_OUT, MCB_PA_OUT);
	}

	/*	SCU_PA*	*/

	/*	CKSEL	*/
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_CKSEL, psInitInfo->bCkSel);

	/*	DIVR0, DIVF0	*/
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVR0, psInitInfo->bDivR0);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVF0, psInitInfo->bDivF0);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVR1, psInitInfo->bDivR1);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVF1, psInitInfo->bDivF1);

	/*	Clock Start	*/
	sdRet	= McDevIf_ExecutePacket();
	if(sdRet == MCDRV_SUCCESS)
	{
		McSrv_ClockStart();

		/*	DP0	*/
		bReg	= MCI_DPADIF_DEF & (UINT8)~MCB_DP0_CLKI0;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
		if(psInitInfo->bCkSel != MCDRV_CKSEL_CMOS)
		{
			/*	2ms wait	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_TCXO_WAIT_TIME, 0);
		}

		/*	PLLRST0	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_PLL_RST, 0);
		/*	2ms wait	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_PLRST_WAIT_TIME, 0);
		/*	DP1/DP2 up	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL, 0);
		/*	RSTB	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_RSTB, 0);
		/*	DPB	*/
		bReg	= MCB_PWM_DPPDM|MCB_PWM_DPDI2|MCB_PWM_DPDI1|MCB_PWM_DPDI0;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_1, bReg);

		/*	DCL_GAIN, DCL_LMT	*/
		bReg	= (psInitInfo->bDclGain<<4) | psInitInfo->bDclLimit;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DCL, bReg);

		/*	DIF_LI, DIF_LO*	*/
		bReg	= (psInitInfo->bLineOut2Dif<<5) | (psInitInfo->bLineOut1Dif<<4) | psInitInfo->bLineIn1Dif;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_DIF_LINE, bReg);

		/*	SP*_HIZ, SPMN	*/
		bReg	= (psInitInfo->bSpmn << 1);
		if(MCDRV_SPHIZ_HIZ == psInitInfo->bSpHiz)
		{
			bReg	|= (MCB_SPL_HIZ|MCB_SPR_HIZ);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SP_MODE, bReg);

		/*	MC*SNG	*/
		bReg	= (psInitInfo->bMic2Sng<<6) | (psInitInfo->bMic1Sng<<2);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC_GAIN, bReg);
		bReg	= (psInitInfo->bMic3Sng<<2);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC3_GAIN, bReg);

		/*	CPMOD	*/
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_CPMOD, psInitInfo->bCpMod);

		/*	AVDDLEV, VREFLEV	*/
		bReg	= 0x20 | psInitInfo->bAvddLev;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LEV, bReg);

		if(((psInitInfo->bPowerMode & MCDRV_POWMODE_VREFON) != 0) || (McResCtrl_GetAPMode() == eMCDRV_APM_OFF))
		{
			bReg	= MCI_PWM_ANALOG_0_DEF;
			if(psInitInfo->bLdo == MCDRV_LDO_ON)
			{
				/*	AP_LDOA	*/
				bReg	&= (UINT8)~MCB_PWM_LDOA;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_LDOA_WAIT_TIME, 0);
			}
			else
			{
				bReg	&= (UINT8)~MCB_PWM_REFA;
			}
			/*	AP_VR up	*/
			bReg	&= (UINT8)~MCB_PWM_VR;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
			sdRet	= McDevIf_ExecutePacket();
			if(sdRet == MCDRV_SUCCESS)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | psInitInfo->sWaitTime.dVrefRdy2, 0);
			}
		}
	}

	if(MCDRV_SUCCESS == sdRet)
	{
		if(McResCtrl_GetAPMode() == eMCDRV_APM_OFF)
		{
			bReg	= MCB_APMOFF_SP|MCB_APMOFF_HP|MCB_APMOFF_RC;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_APMOFF, bReg);
		}
		sdRet	= McDevIf_ExecutePacket();
	}
	if(MCDRV_SUCCESS == sdRet)
	{
		/*	unused path power down	*/
		McResCtrl_GetPowerInfo(&sPowerInfo);
		sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
		sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
		sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
		sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
		sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
		sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
		sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddInit", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPowerUp
 *
 *	Description:
 *			Add powerup packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerUp
(
	const MCDRV_POWER_INFO*		psPowerInfo,
	const MCDRV_POWER_UPDATE*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegDPADIF;
	UINT8	bRegCur;
	UINT8	bRegAna1;
	UINT8	bRegAna2;
	UINT32	dUpdate;
	UINT8	bRegChange;
	UINT8	bSpHizReg;
	UINT32	dWaitTime;
	UINT32	dWaitTimeDone	= 0;
	UINT8	bWaitVREFRDY	= 0;
	UINT8	bWaitHPVolUp	= 0;
	UINT8	bWaitSPVolUp	= 0;
	MCDRV_INIT_INFO		sInitInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddPowerUp");
#endif

	bRegDPADIF	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DPADIF);


	McResCtrl_GetInitInfo(&sInitInfo);

	/*	Digital Power	*/
	dUpdate	= ~psPowerInfo->dDigital & psPowerUpdate->dDigital;
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DP0) != 0UL)
	{
		if((bRegDPADIF & (MCB_DP0_CLKI1|MCB_DP0_CLKI0)) == (MCB_DP0_CLKI1|MCB_DP0_CLKI0))
		{/*	DP0 changed	*/
			/*	CKSEL	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_CKSEL, sInitInfo.bCkSel);
			/*	DIVR0, DIVF0	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVR0, sInitInfo.bDivR0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVF0, sInitInfo.bDivF0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVR1, sInitInfo.bDivR1);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DIVF1, sInitInfo.bDivF1);
			sdRet	= McDevIf_ExecutePacket();
			if(sdRet == MCDRV_SUCCESS)
			{
				/*	Clock Start	*/
				McSrv_ClockStart();
				/*	DP0 up	*/
				if((((bRegDPADIF & MCB_CLKSRC) == 0) && ((bRegDPADIF & MCB_CLKINPUT) == 0))
				|| (((bRegDPADIF & MCB_CLKSRC) != 0) && ((bRegDPADIF & MCB_CLKINPUT) != 0)))
				{
					bRegDPADIF	&= (UINT8)~MCB_DP0_CLKI0;
				}
				else
				{
					bRegDPADIF	&= (UINT8)~MCB_DP0_CLKI1;
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bRegDPADIF);
				if(sInitInfo.bCkSel != MCDRV_CKSEL_CMOS)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_TCXO_WAIT_TIME, 0);
				}
				/*	PLLRST0 up	*/
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_PLL_RST, 0);
				/*	2ms wait	*/
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_PLRST_WAIT_TIME, 0);
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					/*	DP1/DP2 up	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL, 0);
					/*	DPB/DPDI* up	*/
					bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PWM_DIGITAL_1);
					if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0) != 0UL)
					{
						bReg	&= (UINT8)~MCB_PWM_DPDI0;
					}
					if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1) != 0UL)
					{
						bReg	&= (UINT8)~MCB_PWM_DPDI1;
					}
					if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2) != 0UL)
					{
						bReg	&= (UINT8)~MCB_PWM_DPDI2;
					}
					if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM) != 0UL)
					{
						bReg	&= (UINT8)~MCB_PWM_DPPDM;
					}
					if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB) != 0UL)
					{
						bReg	&= (UINT8)~MCB_PWM_DPB;
					}
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_1, bReg);
				}
			}
		}
		else if((dUpdate & MCDRV_POWINFO_DIGITAL_DP2) != 0UL)
		{
			/*	DP1/DP2 up	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL, 0);
			/*	DPB/DPDI* up	*/
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PWM_DIGITAL_1);
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0) != 0UL)
			{
				bReg	&= (UINT8)~MCB_PWM_DPDI0;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1) != 0UL)
			{
				bReg	&= (UINT8)~MCB_PWM_DPDI1;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2) != 0UL)
			{
				bReg	&= (UINT8)~MCB_PWM_DPDI2;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM) != 0UL)
			{
				bReg	&= (UINT8)~MCB_PWM_DPPDM;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB) != 0UL)
			{
				bReg	&= (UINT8)~MCB_PWM_DPB;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_1, bReg);
		}
		else
		{
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			/*	DPBDSP	*/
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPBDSP) != 0UL)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_BDSP, 0);
			}
			/*	DPADIF	*/
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPADIF) != 0UL)
			{
				bRegDPADIF	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DPADIF);
				bRegDPADIF	&= (UINT8)~MCB_DPADIF;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bRegDPADIF);
			}
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		if(McResCtrl_GetAPMode() == eMCDRV_APM_ON)
		{
			sdRet	= AddAnalogPowerUpAuto(psPowerInfo, psPowerUpdate);
		}
		else
		{
			/*	Analog Power	*/
			dUpdate	= (UINT32)~psPowerInfo->abAnalog[0] & (UINT32)psPowerUpdate->abAnalog[0];
			if((dUpdate & (UINT32)MCB_PWM_VR) != 0UL)
			{
				if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0) & MCB_PWM_VR) != 0)
				{/*	AP_VR changed	*/
					bReg	= MCI_PWM_ANALOG_0_DEF;
					if(sInitInfo.bLdo == MCDRV_LDO_ON)
					{
						/*	AP_LDOA	*/
						bReg	&= (UINT8)~MCB_PWM_LDOA;
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_LDOA_WAIT_TIME, 0);
					}
					else
					{
						bReg	&= (UINT8)~MCB_PWM_REFA;
					}
					/*	AP_VR up	*/
					bReg	&= (UINT8)~MCB_PWM_VR;
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
					dWaitTimeDone	= sInitInfo.sWaitTime.dVrefRdy1;
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTimeDone, 0);
					bWaitVREFRDY	= 1;
				}

				bReg	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1));
				/*	SP_HIZ control	*/
				if(MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz)
				{
					bSpHizReg	= 0;
					if((bReg & (MCB_PWM_SPL1 | MCB_PWM_SPL2)) != 0)
					{
						bSpHizReg |= MCB_SPL_HIZ;
					}

					if((bReg & (MCB_PWM_SPR1 | MCB_PWM_SPR2)) != 0)
					{
						bSpHizReg |= MCB_SPR_HIZ;
					}

					bSpHizReg |= (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SP_MODE) & (MCB_SPMN | MCB_SP_SWAP));
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SP_MODE, bSpHizReg);
				}

				bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_3);
				bReg	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) & bRegCur);
				bRegChange	= bReg ^ bRegCur;
				/*	set DACON and NSMUTE before setting 0 to AP_DA	*/
				if(((bRegChange & (MCB_PWM_DAR|MCB_PWM_DAL)) != 0) && ((psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & (MCB_PWM_DAR|MCB_PWM_DAL)) != (MCB_PWM_DAR|MCB_PWM_DAL)))
				{
					if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DAC_CONFIG) & MCB_DACON) == 0)
					{
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, MCB_NSMUTE);
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, (MCB_DACON | MCB_NSMUTE));
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, MCB_DACON);
					}
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_3, bReg);
				bRegChange	&= (MCB_PWM_MB1|MCB_PWM_MB2|MCB_PWM_MB3);

				bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_4);
				bReg	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) & bRegCur);
				bRegChange	|= (bReg ^ bRegCur);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_4, bReg);

				if(bWaitVREFRDY != 0)
				{
					/*	wait VREF_RDY	*/
					dWaitTimeDone	= sInitInfo.sWaitTime.dVrefRdy2 - dWaitTimeDone;
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTimeDone, 0);
				}

				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0);
					bReg	= (UINT8)(~dUpdate & bRegCur);
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
					if(((bRegCur & MCB_PWM_CP) != 0) && ((bReg & MCB_PWM_CP) == 0))
					{/*	AP_CP up	*/
						dWaitTime		= MCDRV_HP_WAIT_TIME;
					}
					else
					{
						dWaitTime	= 0;
					}

					bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1);
					bRegAna1	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & bRegCur);
					if((((bRegCur & MCB_PWM_SPL1) != 0) && ((bRegAna1 & MCB_PWM_SPL1) == 0))
					|| (((bRegCur & MCB_PWM_SPR1) != 0) && ((bRegAna1 & MCB_PWM_SPR1) == 0)))
					{/*	AP_SP* up	*/
						bReg	= bRegAna1|(bRegCur&(UINT8)~(MCB_PWM_SPL1|MCB_PWM_SPR1));
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_1, bReg);
						if(dWaitTime == (UINT32)0)
						{
							dWaitTime		= MCDRV_SP_WAIT_TIME;
							bWaitSPVolUp	= 1;
						}
					}
					if((((bRegCur & MCB_PWM_HPL) != 0) && ((bRegAna1 & MCB_PWM_HPL) == 0))
					|| (((bRegCur & MCB_PWM_HPR) != 0) && ((bRegAna1 & MCB_PWM_HPR) == 0)))
					{
						bWaitHPVolUp	= 1;
					}

					bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_2);
					bRegAna2	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) & bRegCur);
					if(((bRegCur & MCB_PWM_RC1) != 0) && ((bRegAna2 & MCB_PWM_RC1) == 0))
					{/*	AP_RC up	*/
						bReg	= bRegAna2|(bRegCur&(UINT8)~MCB_PWM_RC1);
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_2, bReg);
						if(dWaitTime == (UINT32)0)
						{
							dWaitTime	= MCDRV_RC_WAIT_TIME;
						}
					}
					if(dWaitTime > (UINT32)0)
					{
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
						dWaitTimeDone	+= dWaitTime;
					}

					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_1, bRegAna1);
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_2, bRegAna2);

					/*	time wait	*/
					dWaitTime	= GetMaxWait(bRegChange);
					if(dWaitTime > dWaitTimeDone)
					{
						dWaitTime	= dWaitTime - dWaitTimeDone;
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
						dWaitTimeDone	+= dWaitTime;
					}

					if((bWaitSPVolUp != 0) && (sInitInfo.sWaitTime.dSpRdy > dWaitTimeDone))
					{
						dWaitTime	= sInitInfo.sWaitTime.dSpRdy - dWaitTimeDone;
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
						dWaitTimeDone	+= dWaitTime;
					}
					if((bWaitHPVolUp != 0) && (sInitInfo.sWaitTime.dHpRdy > dWaitTimeDone))
					{
						dWaitTime	= sInitInfo.sWaitTime.dHpRdy - dWaitTimeDone;
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
						dWaitTimeDone	+= dWaitTime;
					}
				}
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddPowerUp", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AddAnalogPowerUpAuto
 *
 *	Description:
 *			Add analog auto powerup packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	AddAnalogPowerUpAuto
(
	const MCDRV_POWER_INFO*		psPowerInfo,
	const MCDRV_POWER_UPDATE*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegCur;
	UINT32	dUpdate;
	UINT8	bRegChange;
	UINT8	bSpHizReg;
	MCDRV_INIT_INFO	sInitInfo;
	UINT32	dWaitTime	= 0;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddAnalogPowerUpAuto");
#endif


	McResCtrl_GetInitInfo(&sInitInfo);

	/*	Analog Power	*/
	dUpdate	= (UINT32)~psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0];
	if((dUpdate & (UINT32)MCB_PWM_VR) != 0UL)
	{
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0) & MCB_PWM_VR) != 0)
		{/*	AP_VR changed	*/
			/*	AP_VR up	*/
			bReg	= MCI_PWM_ANALOG_0_DEF;
			if(sInitInfo.bLdo == MCDRV_LDO_ON)
			{
				/*	AP_LDOA	*/
				bReg	&= (UINT8)~MCB_PWM_LDOA;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | MCDRV_LDOA_WAIT_TIME, 0);
			}
			else
			{
				bReg	&= (UINT8)~MCB_PWM_REFA;
			}
			bReg	&= (UINT8)~MCB_PWM_VR;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | sInitInfo.sWaitTime.dVrefRdy1, 0);
			if(sInitInfo.sWaitTime.dVrefRdy2 > sInitInfo.sWaitTime.dVrefRdy1)
			{
				dWaitTime	= sInitInfo.sWaitTime.dVrefRdy2 - sInitInfo.sWaitTime.dVrefRdy1;
			}
		}

		bReg	= (UINT8)((UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1));
		/*	SP_HIZ control	*/
		if(MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz)
		{
			bSpHizReg	= 0;
			if((bReg & (MCB_PWM_SPL1 | MCB_PWM_SPL2)) != 0)
			{
				bSpHizReg |= MCB_SPL_HIZ;
			}

			if((bReg & (MCB_PWM_SPR1 | MCB_PWM_SPR2)) != 0)
			{
				bSpHizReg |= MCB_SPR_HIZ;
			}

			bSpHizReg |= (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SP_MODE) & (MCB_SPMN | MCB_SP_SWAP));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SP_MODE, bSpHizReg);
		}

		bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_3);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) & bRegCur;
		bRegChange	= bReg ^ bRegCur;
		/*	set DACON and NSMUTE before setting 0 to AP_DA	*/
		if(((bRegChange & (MCB_PWM_DAR|MCB_PWM_DAL)) != 0) && ((psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & (MCB_PWM_DAR|MCB_PWM_DAL)) != (MCB_PWM_DAR|MCB_PWM_DAL)))
		{
			if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DAC_CONFIG) & MCB_DACON) == 0)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, MCB_NSMUTE);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, (MCB_DACON | MCB_NSMUTE));
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, MCB_DACON);
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_3, bReg);
		bRegChange	&= (MCB_PWM_MB1|MCB_PWM_MB2|MCB_PWM_MB3);

		bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_4);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) & bRegCur;
		bRegChange	|= (bReg ^ bRegCur);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_4, bReg);

		if(dWaitTime > (UINT32)0)
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
		}

		sdRet	= McDevIf_ExecutePacket();
		if(sdRet == MCDRV_SUCCESS)
		{
			bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0);
			bReg	= (UINT8)~dUpdate & bRegCur;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);

			bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1);
			bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & bRegCur;
			if((bRegCur & (MCB_PWM_ADL|MCB_PWM_ADR)) != (bReg & (MCB_PWM_ADL|MCB_PWM_ADR)))
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_1, bReg);
			}
			else
			{
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1, bReg);
				}
			}
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_2);
			bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) & bRegCur;
			if((bRegCur & (MCB_PWM_LO1L|MCB_PWM_LO1R|MCB_PWM_LO2L|MCB_PWM_LO2R)) != (bReg & (MCB_PWM_LO1L|MCB_PWM_LO1R|MCB_PWM_LO2L|MCB_PWM_LO2R)))
			{
				bReg	= bReg|(bRegCur&(MCB_PWM_RC1|MCB_PWM_RC2));
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_2, bReg);
			}
			else
			{
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_2, bReg);
				}
			}
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			/*	time wait	*/
			if(dWaitTime < GetMaxWait(bRegChange))
			{
				dWaitTime	= GetMaxWait(bRegChange) - dWaitTime;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT | dWaitTime, 0);
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddAnalogPowerUpAuto", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPowerDown
 *
 *	Description:
 *			Add powerdown packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerDown
(
	const MCDRV_POWER_INFO*		psPowerInfo,
	const MCDRV_POWER_UPDATE*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegDPADIF;
	UINT8	bRegCur;
	UINT32	dUpdate	= psPowerInfo->dDigital & psPowerUpdate->dDigital;
	UINT32	dAPMDoneParam;
	UINT32	dAnaRdyParam;
	UINT8	bSpHizReg;
	MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddPowerDown");
#endif

	bRegDPADIF	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DPADIF);


	McResCtrl_GetInitInfo(&sInitInfo);

	if(McResCtrl_GetAPMode() == eMCDRV_APM_ON)
	{
		if((((psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0] & MCB_PWM_VR) != 0)
			&& (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0) & MCB_PWM_VR) == 0))
		{
			/*	wait AP_XX_A	*/
			dAPMDoneParam	= ((MCB_AP_CP_A|MCB_AP_HPL_A|MCB_AP_HPR_A)<<8)
							| (MCB_AP_RC1_A|MCB_AP_RC2_A|MCB_AP_SPL1_A|MCB_AP_SPR1_A|MCB_AP_SPL2_A|MCB_AP_SPR2_A);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_APM_DONE | dAPMDoneParam, 0);
		}
	}

	if(((dUpdate & MCDRV_POWINFO_DIGITAL_DP0) != 0UL)
	&& ((bRegDPADIF & (MCB_DP0_CLKI1|MCB_DP0_CLKI0)) != (MCB_DP0_CLKI1|MCB_DP0_CLKI0)))
	{
		/*	wait mute complete	*/
		sdRet	= McDevIf_ExecutePacket();
		if(sdRet == MCDRV_SUCCESS)
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_ALLMUTE, 0);
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		/*	Analog Power	*/
		bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1);
		bReg	= (psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) | bRegCur;
		if(((psPowerUpdate->abAnalog[1] & MCDRV_POWUPDATE_ANALOG1_OUT) != 0) && (MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz))
		{
			/*	SP_HIZ control	*/
			bSpHizReg	= 0;
			if((bReg & (MCB_PWM_SPL1 | MCB_PWM_SPL2)) != 0)
			{
				bSpHizReg |= MCB_SPL_HIZ;
			}

			if((bReg & (MCB_PWM_SPR1 | MCB_PWM_SPR2)) != 0)
			{
				bSpHizReg |= MCB_SPR_HIZ;
			}

			bSpHizReg |= (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SP_MODE) & (MCB_SPMN | MCB_SP_SWAP));
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SP_MODE, bSpHizReg);
		}

		if(McResCtrl_GetAPMode() == eMCDRV_APM_ON)
		{
			if((bRegCur & (MCB_PWM_ADL|MCB_PWM_ADR)) != (bReg & (MCB_PWM_ADL|MCB_PWM_ADR)))
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_1, bReg);
			}
			else
			{
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_1, bReg);
				}
			}
		}
		else
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_1, bReg);
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_2);
		bReg	= (psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) | bRegCur;
		if(McResCtrl_GetAPMode() == eMCDRV_APM_ON)
		{
			if((bRegCur & (MCB_PWM_LO1L|MCB_PWM_LO1R|MCB_PWM_LO2L|MCB_PWM_LO2R)) != (bReg & (MCB_PWM_LO1L|MCB_PWM_LO1R|MCB_PWM_LO2L|MCB_PWM_LO2R)))
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_2, bReg);
			}
			else
			{
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_2, bReg);
				}
			}
		}
		else
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_2, bReg);
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		bReg	= (UINT8)((psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) | McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_3));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_3, bReg);
		bReg	= (UINT8)((psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) | McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_4));
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_4, bReg);

		/*	set DACON and NSMUTE after setting 1 to AP_DA	*/
		if((psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & (MCB_PWM_DAR|MCB_PWM_DAL)) == (MCB_PWM_DAR|MCB_PWM_DAL))
		{
			if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DAC_CONFIG) & MCB_DACON) == MCB_DACON)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_DACMUTE | (UINT32)((MCB_DAC_FLAGL<<8)|MCB_DAC_FLAGR), 0);
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DAC_CONFIG, MCB_NSMUTE);
		}

		bRegCur	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_PWM_ANALOG_0);
		bReg	= psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0];
		if(McResCtrl_GetAPMode() == eMCDRV_APM_OFF)
		{
			/*	wait CPPDRDY	*/
			dAnaRdyParam	= 0;
			if(((bRegCur & MCB_PWM_CP) == 0) && ((bReg & MCB_PWM_CP) == MCB_PWM_CP))
			{
				dAnaRdyParam	= MCB_CPPDRDY;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_ANA_RDY | dAnaRdyParam, 0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, (bRegCur|MCB_PWM_CP));
			}
		}

		if(((bReg & MCB_PWM_VR) != 0) && ((bRegCur & MCB_PWM_VR) == 0))
		{/*	AP_VR changed	*/
			/*	AP_xx down	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, MCI_PWM_ANALOG_0_DEF);
		}
		else
		{
			bReg	|= bRegCur;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_PWM_ANALOG_0, bReg);
		}


		/*	Digital Power	*/
		if(((dUpdate & MCDRV_POWINFO_DIGITAL_DPADIF) != 0UL)
		&& (bRegDPADIF != MCB_DPADIF))
		{
			/*	DPADIF	*/
			bRegDPADIF	|= MCB_DPADIF;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bRegDPADIF);
		}

		if((dUpdate & MCDRV_POWINFO_DIGITAL_DPBDSP) != 0UL)
		{
			/*	DPBDSP	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_BDSP, MCB_PWM_DPBDSP);
		}

		/*	DPDI*, DPPDM	*/
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PWM_DIGITAL_1);
		if(((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0) != 0UL) || ((dUpdate & MCDRV_POWINFO_DIGITAL_DP2) != 0UL))
		{
			bReg |= MCB_PWM_DPDI0;
		}
		if(((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1) != 0UL) || ((dUpdate & MCDRV_POWINFO_DIGITAL_DP2) != 0UL))
		{
			bReg |= MCB_PWM_DPDI1;
		}
		if(((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2) != 0UL) || ((dUpdate & MCDRV_POWINFO_DIGITAL_DP2) != 0UL))
		{
			bReg |= MCB_PWM_DPDI2;
		}
		if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM) != 0UL)
		{
			bReg |= MCB_PWM_DPPDM;
		}
		if(bReg != 0)
		{
			/*	cannot set DP* & DPB at the same time	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_1, bReg);
		}
		/*	DPB	*/
		if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB) != 0UL)
		{
			bReg |= MCB_PWM_DPB;
		}
		if(bReg != 0)
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL_1, bReg);
		}

		if(((dUpdate & MCDRV_POWINFO_DIGITAL_DP2) != 0UL)
		&& ((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PWM_DIGITAL) & MCB_PWM_DP2) == 0))
		{
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DP0) != 0UL)
			{
				/*	DP2, DP1	*/
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL, (MCB_PWM_DP2 | MCB_PWM_DP1));
				if((dUpdate & MCDRV_POWINFO_DIGITAL_PLLRST0) != 0UL)
				{
					/*	PLLRST0	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_PLL_RST, MCB_PLLRST0);
				}
				/*	DP0	*/
				bRegDPADIF	|= (MCB_DP0_CLKI1 | MCB_DP0_CLKI0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bRegDPADIF);
				sdRet	= McDevIf_ExecutePacket();
				if(sdRet == MCDRV_SUCCESS)
				{
					McSrv_ClockStop();
				}
			}
			else
			{
				/*	DP2	*/
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PWM_DIGITAL, MCB_PWM_DP2);
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
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
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPathSet
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddPathSet");
#endif

	/*	DI Pad	*/
	AddDIPad();

	/*	PAD(PDM)	*/
	AddPAD();

	/*	Digital Mixer Source	*/
	sdRet	= AddSource();
	if(sdRet == MCDRV_SUCCESS)
	{
		/*	DIR*_START, DIT*_START	*/
		AddDIStart();
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddPathSet", &sdRet);
#endif
	return sdRet;
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
static void	AddDIPad
(
	void
)
{
	UINT8	bReg;
	UINT8	bIsUsedDIR[3];
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddDIPad");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetDioInfo(&sDioInfo);

	/*	SDIN_MSK2/1	*/
	bReg	= 0;
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR2) == 0)
	{
		bReg |= MCB_SDIN_MSK2;
		bIsUsedDIR[2]	= 0;
	}
	else
	{
		bIsUsedDIR[2]	= 1;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR1) == 0)
	{
		bReg |= MCB_SDIN_MSK1;
		bIsUsedDIR[1]	= 0;
	}
	else
	{
		bIsUsedDIR[1]	= 1;
	}
	/*	SDO_DDR2/1	*/
	if(McResCtrl_IsDstUsed(eMCDRV_DST_DIT2, eMCDRV_DST_CH0) == 0)
	{
		if(sInitInfo.bDioSdo2Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_SDO_DDR2;
		}
	}
	else
	{
		bReg |= MCB_SDO_DDR2;
	}
	if(McResCtrl_IsDstUsed(eMCDRV_DST_DIT1, eMCDRV_DST_CH0) == 0)
	{
		if(sInitInfo.bDioSdo1Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_SDO_DDR1;
		}
	}
	else
	{
		bReg |= MCB_SDO_DDR1;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_SD_MSK, bReg);

	/*	SDIN_MSK0	*/
	bReg	= 0;
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR0) == 0)
	{
		bReg |= MCB_SDIN_MSK0;
		bIsUsedDIR[0]	= 0;
	}
	else
	{
		bIsUsedDIR[0]	= 1;
	}
	/*	SDO_DDR0	*/
	if(McResCtrl_IsDstUsed(eMCDRV_DST_DIT0, eMCDRV_DST_CH0) == 0)
	{
		if(sInitInfo.bDioSdo0Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_SDO_DDR0;
		}
	}
	else
	{
		bReg |= MCB_SDO_DDR0;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_SD_MSK_1, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*	*/
	bReg	= 0;
	if((bIsUsedDIR[2] == 0) && (McResCtrl_GetDITSource(eMCDRV_DIO_2) == eMCDRV_SRC_NONE))
	{
		bReg |= MCB_BCLK_MSK2;
		bReg |= MCB_LRCK_MSK2;
		if(sInitInfo.bDioClk2Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_BCLK_DDR2;
			bReg |= MCB_LRCK_DDR2;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_BCLK_DDR2;
			bReg |= MCB_LRCK_DDR2;
		}
	}
	if((bIsUsedDIR[1] == 0) && (McResCtrl_GetDITSource(eMCDRV_DIO_1) == eMCDRV_SRC_NONE))
	{
		bReg |= MCB_BCLK_MSK1;
		bReg |= MCB_LRCK_MSK1;
		if(sInitInfo.bDioClk1Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_BCLK_DDR1;
			bReg |= MCB_LRCK_DDR1;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_BCLK_DDR1;
			bReg |= MCB_LRCK_DDR1;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_BCLK_MSK, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*, PCM_HIZ*	*/
	bReg	= 0;
	if((bIsUsedDIR[0] == 0) && (McResCtrl_GetDITSource(eMCDRV_DIO_0) == eMCDRV_SRC_NONE))
	{
		bReg |= MCB_BCLK_MSK0;
		bReg |= MCB_LRCK_MSK0;
		if(sInitInfo.bDioClk0Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_BCLK_DDR0;
			bReg |= MCB_LRCK_DDR0;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_BCLK_DDR0;
			bReg |= MCB_LRCK_DDR0;
		}
	}
	if(sInitInfo.bPcmHiz == MCDRV_PCMHIZ_HIZ)
	{
		bReg |= (MCB_PCMOUT_HIZ2 | MCB_PCMOUT_HIZ1 | MCB_PCMOUT_HIZ0);
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_BCLK_MSK_1, bReg);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddDIPad", 0);
#endif
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
static void	AddPAD
(
	void
)
{
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddPAD");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);

	/*	PA*_MSK	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PA_MSK);
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_PDM) == 0)
	{
		bReg	|= MCB_PA0_MSK;
		if(sInitInfo.bPad1Func == MCDRV_PAD_PDMDI)
		{
			bReg	|= MCB_PA1_MSK;
		}
	}
	else
	{
		bReg	&= (UINT8)~MCB_PA0_MSK;
		if(sInitInfo.bPad1Func == MCDRV_PAD_PDMDI)
		{
			bReg	&= (UINT8)~MCB_PA1_MSK;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_MSK, bReg);

#if (MCDRV_DEBUG_LEVEL>=4)
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
static SINT32	AddSource
(
	void
)
{
	SINT32	sdRet			= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bAEng6;
	UINT8	bRegAESource	= 0;
	UINT8	bAESourceChange	= 0;
	UINT32	dXFadeParam		= 0;
	MCDRV_SRC_TYPE	eAESource;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DAC_INFO	sDacInfo;
	MCDRV_AE_INFO	sAeInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddSource");
#endif

	bAEng6		= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_AENG6_SOURCE);
	eAESource	= McResCtrl_GetAESource();

	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetAeInfo(&sAeInfo);

	switch(eAESource)
	{
	case	eMCDRV_SRC_PDM:		bRegAESource	= MCB_AE_SOURCE_AD;		bAEng6	= MCB_AENG6_PDM;	break;
	case	eMCDRV_SRC_ADC0:	bRegAESource	= MCB_AE_SOURCE_AD;		bAEng6	= MCB_AENG6_ADC0;	break;
	case	eMCDRV_SRC_DIR0:	bRegAESource	= MCB_AE_SOURCE_DIR0;	break;
	case	eMCDRV_SRC_DIR1:	bRegAESource	= MCB_AE_SOURCE_DIR1;	break;
	case	eMCDRV_SRC_DIR2:	bRegAESource	= MCB_AE_SOURCE_DIR2;	break;
	case	eMCDRV_SRC_MIX:		bRegAESource	= MCB_AE_SOURCE_MIX;	break;
	default:					bRegAESource	= 0;
	}
	if(bRegAESource != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_SRC_SOURCE_1)&0xF0))
	{
		/*	xxx_INS	*/
		dXFadeParam	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DAC_INS);
		dXFadeParam <<= 8;
		dXFadeParam	|= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_INS);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DAC_INS, 0);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, 0);
		bAESourceChange	= 1;
		sdRet	= McDevIf_ExecutePacket();
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		McResCtrl_GetDacInfo(&sDacInfo);

		/*	DAC_SOURCE/VOICE_SOURCE	*/
		bReg	= 0;
		switch(McResCtrl_GetDACSource(eMCDRV_DAC_MASTER))
		{
		case	eMCDRV_SRC_PDM:
			bReg |= MCB_DAC_SOURCE_AD;
			bAEng6	= MCB_AENG6_PDM;
			break;
		case	eMCDRV_SRC_ADC0:
			bReg |= MCB_DAC_SOURCE_AD;
			bAEng6	= MCB_AENG6_ADC0;
			break;
		case	eMCDRV_SRC_DIR0:
			bReg |= MCB_DAC_SOURCE_DIR0;
			break;
		case	eMCDRV_SRC_DIR1:
			bReg |= MCB_DAC_SOURCE_DIR1;
			break;
		case	eMCDRV_SRC_DIR2:
			bReg |= MCB_DAC_SOURCE_DIR2;
			break;
		case	eMCDRV_SRC_MIX:
			bReg |= MCB_DAC_SOURCE_MIX;
			break;
		default:
			break;
		}
		switch(McResCtrl_GetDACSource(eMCDRV_DAC_VOICE))
		{
		case	eMCDRV_SRC_PDM:
			bReg |= MCB_VOICE_SOURCE_AD;
			bAEng6	= MCB_AENG6_PDM;
			break;
		case	eMCDRV_SRC_ADC0:
			bReg |= MCB_VOICE_SOURCE_AD;
			bAEng6	= MCB_AENG6_ADC0;
			break;
		case	eMCDRV_SRC_DIR0:
			bReg |= MCB_VOICE_SOURCE_DIR0;
			break;
		case	eMCDRV_SRC_DIR1:
			bReg |= MCB_VOICE_SOURCE_DIR1;
			break;
		case	eMCDRV_SRC_DIR2:
			bReg |= MCB_VOICE_SOURCE_DIR2;
			break;
		case	eMCDRV_SRC_MIX:
			bReg |= MCB_VOICE_SOURCE_MIX;
			break;
		default:
			break;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_SOURCE, bReg);

		/*	SWP/VOICE_SWP	*/
		bReg	= (sDacInfo.bMasterSwap << 4) | sDacInfo.bVoiceSwap;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_SWP, bReg);

		/*	DIT0SRC_SOURCE/DIT1SRC_SOURCE	*/
		bReg	= 0;
		switch(McResCtrl_GetDITSource(eMCDRV_DIO_0))
		{
		case	eMCDRV_SRC_PDM:
			bReg |= MCB_DIT0_SOURCE_AD;
				bAEng6	= MCB_AENG6_PDM;
			break;
		case	eMCDRV_SRC_ADC0:
			bReg |= MCB_DIT0_SOURCE_AD;
				bAEng6	= MCB_AENG6_ADC0;
			break;
		case	eMCDRV_SRC_DIR0:
			bReg |= MCB_DIT0_SOURCE_DIR0;
			break;
		case	eMCDRV_SRC_DIR1:
			bReg |= MCB_DIT0_SOURCE_DIR1;
			break;
		case	eMCDRV_SRC_DIR2:
			bReg |= MCB_DIT0_SOURCE_DIR2;
			break;
		case	eMCDRV_SRC_MIX:
			bReg |= MCB_DIT0_SOURCE_MIX;
			break;
		default:
			break;
		}
		switch(McResCtrl_GetDITSource(eMCDRV_DIO_1))
		{
		case	eMCDRV_SRC_PDM:
			bReg |= MCB_DIT1_SOURCE_AD;
				bAEng6	= MCB_AENG6_PDM;
			break;
		case	eMCDRV_SRC_ADC0:
			bReg |= MCB_DIT1_SOURCE_AD;
				bAEng6	= MCB_AENG6_ADC0;
			break;
		case	eMCDRV_SRC_DIR0:
			bReg |= MCB_DIT1_SOURCE_DIR0;
			break;
		case	eMCDRV_SRC_DIR1:
			bReg |= MCB_DIT1_SOURCE_DIR1;
			break;
		case	eMCDRV_SRC_DIR2:
			bReg |= MCB_DIT1_SOURCE_DIR2;
			break;
		case	eMCDRV_SRC_MIX:
			bReg |= MCB_DIT1_SOURCE_MIX;
			break;
		default:
			break;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_SRC_SOURCE, bReg);

		/*	AE_SOURCE/DIT2SRC_SOURCE	*/
		bReg	= bRegAESource;
		switch(McResCtrl_GetDITSource(eMCDRV_DIO_2))
		{
		case	eMCDRV_SRC_PDM:
			bReg |= MCB_DIT2_SOURCE_AD;
				bAEng6	= MCB_AENG6_PDM;
			break;
		case	eMCDRV_SRC_ADC0:
			bReg |= MCB_DIT2_SOURCE_AD;
				bAEng6	= MCB_AENG6_ADC0;
			break;
		case	eMCDRV_SRC_DIR0:
			bReg |= MCB_DIT2_SOURCE_DIR0;
			break;
		case	eMCDRV_SRC_DIR1:
			bReg |= MCB_DIT2_SOURCE_DIR1;
			break;
		case	eMCDRV_SRC_DIR2:
			bReg |= MCB_DIT2_SOURCE_DIR2;
			break;
		case	eMCDRV_SRC_MIX:
			bReg |= MCB_DIT2_SOURCE_MIX;
			break;
		default:
			break;
		}
		if(bAESourceChange != 0)
		{
			/*	wait xfade complete	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_INSFLG | dXFadeParam, 0);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_SRC_SOURCE_1, bReg);

		/*	BDSP_ST	*/
		if(McResCtrl_GetAESource() == eMCDRV_SRC_NONE)
		{/*	AE is unused	*/
			/*	BDSP stop & reset	*/
			if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_BDSP_ST)&MCB_BDSP_ST) != 0)
			{
				bReg	= 0;
				if((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0)
				{
					bReg |= MCB_EQ5ON;
				}
				if((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0)
				{
					bReg |= MCB_DRCON;
				}
				if((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0)
				{
					bReg |= MCB_EQ3ON;
				}
				if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
				{
					if((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0)
					{
						bReg |= MCB_DBEXON;
					}
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_ST, bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_RST, MCB_TRAM_RST);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_RST, 0);
			}
		}
		else
		{/*	AE is used	*/
			bReg	= 0;
			if((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0)
			{
				bReg |= MCB_EQ5ON;
			}
			if((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0)
			{
				bReg |= MCB_DRCON;
				bReg |= MCB_BDSP_ST;
			}
			if((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0)
			{
				bReg |= MCB_EQ3ON;
			}
			if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
			{
				if((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0)
				{
					bReg |= MCB_DBEXON;
					bReg |= MCB_BDSP_ST;
				}
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_ST, bReg);
		}

		/*	check MIX SOURCE for AENG6_SOURCE	*/
		if(McResCtrl_IsSrcUsed(eMCDRV_SRC_PDM) != 0)
		{
			bAEng6	= MCB_AENG6_PDM;
		}
		else if(McResCtrl_IsSrcUsed(eMCDRV_SRC_ADC0) != 0)
		{
			bAEng6	= MCB_AENG6_ADC0;
		}
		else
		{
		}

		/*	AENG6_SOURCE	*/
		if(bAEng6 != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_AENG6_SOURCE))
		{
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START);
			if((bReg & MCB_AD_START) != 0)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_START, bReg&(UINT8)~MCB_AD_START);
			}
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START);
			if((bReg & MCB_PDM_START) != 0)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_START, bReg&(UINT8)~MCB_PDM_START);
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_AENG6_SOURCE, bAEng6);

		/*	xxx_INS	*/
		if(McResCtrl_IsSrcUsed(eMCDRV_SRC_AE) != 0)
		{
			switch(eAESource)
			{
			case	eMCDRV_SRC_PDM:
			case	eMCDRV_SRC_ADC0:
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, MCB_ADC_INS);
				break;
			case	eMCDRV_SRC_DIR0:
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, MCB_DIR0_INS);
				break;
			case	eMCDRV_SRC_DIR1:
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, MCB_DIR1_INS);
				break;
			case	eMCDRV_SRC_DIR2:
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, MCB_DIR2_INS);
				break;
			case	eMCDRV_SRC_MIX:
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DAC_INS, MCB_DAC_INS);
				break;
			default:
				break;
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddSource", &sdRet);
#endif

	return sdRet;
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
static void	AddDIStart
(
	void
)
{
	UINT8	bReg;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddDIStart");
#endif

	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetDioInfo(&sDioInfo);

	/*	DIR*_START, DIT*_START	*/
	bReg	= 0;
	if(McResCtrl_GetDITSource(eMCDRV_DIO_0) != eMCDRV_SRC_NONE)
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM0_START;
		}
		bReg |= MCB_DIT0_SRC_START;
		bReg |= MCB_DIT0_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR0) != 0)
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM0_START;
		}
		bReg |= MCB_DIR0_SRC_START;
		bReg |= MCB_DIR0_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX0_START, bReg);

	bReg	= 0;
	if(McResCtrl_GetDITSource(eMCDRV_DIO_1) != eMCDRV_SRC_NONE)
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM1_START;
		}
		bReg |= MCB_DIT1_SRC_START;
		bReg |= MCB_DIT1_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR1) != 0)
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM1_START;
		}
		bReg |= MCB_DIR1_SRC_START;
		bReg |= MCB_DIR1_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX1_START, bReg);

	bReg	= 0;
	if(McResCtrl_GetDITSource(eMCDRV_DIO_2) != eMCDRV_SRC_NONE)
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM2_START;
		}
		bReg |= MCB_DIT2_SRC_START;
		bReg |= MCB_DIT2_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR2) != 0)
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_DITIM2_START;
		}
		bReg |= MCB_DIR2_SRC_START;
		bReg |= MCB_DIR2_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX2_START, bReg);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddDIStart", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddMixSet
 *
 *	Description:
 *			Add analog mixer set packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddMixSet
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddMixSet");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetPathInfo(&sPathInfo);

	/*	ADL_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asAdc0[0]);
	if(((sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADL_MIX, bReg);
	/*	ADL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADL_MONO, bReg);

	/*	ADR_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asAdc0[1]);
	if(((sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	|| ((sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADR_MIX, bReg);
	/*	ADR_MONO	*/
	bReg	= 0;
	if((sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADR_MONO, bReg);

	/*	L1L_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asLout1[0]);
	if(((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	if(((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	|| ((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON))
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO1L_MIX, bReg);
	/*	L1L_MONO	*/
	bReg	= 0;
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_MONO_DA;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO1L_MONO, bReg);

	/*	L1R_MIX	*/
	if(sInitInfo.bLineOut1Dif != MCDRV_LINE_DIF)
	{
		bReg	= GetMicMixBit(&sPathInfo.asLout1[1]);
		if((sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
		{
			bReg |= MCB_LI1MIX;
		}
		if((sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
		{
			bReg |= MCB_DAMIX;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO1R_MIX, bReg);
	}

	/*	L2L_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asLout2[0]);
	if(((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	if(((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	|| ((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON))
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO2L_MIX, bReg);
	/*	L2L_MONO	*/
	bReg	= 0;
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_MONO_DA;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO2L_MONO, bReg);

	/*	L2R_MIX	*/
	if(sInitInfo.bLineOut2Dif != MCDRV_LINE_DIF)
	{
		bReg	= GetMicMixBit(&sPathInfo.asLout2[1]);
		if((sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
		{
			bReg |= MCB_LI1MIX;
		}
		if((sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
		{
			bReg |= MCB_DAMIX;
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO2R_MIX, bReg);
	}

	/*	HPL_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asHpOut[0]);
	if(((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	if(((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	|| ((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON))
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HPL_MIX, bReg);
	/*	HPL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_MONO_DA;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HPL_MONO, bReg);

	/*	HPR_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asHpOut[1]);
	if((sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	{
		bReg |= MCB_LI1MIX;
	}
	if((sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HPR_MIX, bReg);

	/*	SPL_MIX	*/
	bReg	= 0;
	if(((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	if(((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	|| ((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON))
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPL_MIX, bReg);
	/*	SPL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_MONO_DA;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPL_MONO, bReg);

	/*	SPR_MIX	*/
	bReg	= 0;
	if(((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	|| ((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		bReg |= MCB_LI1MIX;
	}
	if(((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
	|| ((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON))
	{
		bReg |= MCB_DAMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPR_MIX, bReg);
	/*	SPR_MONO	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_MONO_LI1;
	}
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_MONO_DA;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPR_MONO, bReg);

	/*	RCV_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asRcOut[0]);
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_LI1MIX;
	}
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	{
		bReg |= MCB_DALMIX;
	}
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
	{
		bReg |= MCB_DARMIX;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_RC_MIX, bReg);


#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddMixSet", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	GetMicMixBit
 *
 *	Description:
 *			Get mic mixer bit.
 *	Arguments:
 *			source info
 *	Return:
 *			mic mixer bit
 *
 ****************************************************************************/
static UINT8	GetMicMixBit
(
	const MCDRV_CHANNEL* psChannel
)
{
	UINT8	bMicMix	= 0;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetMicMixBit");
#endif

	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON)
	{
		bMicMix |= MCB_M1MIX;
	}
	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON)
	{
		bMicMix |= MCB_M2MIX;
	}
	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON)
	{
		bMicMix |= MCB_M3MIX;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bMicMix;
	McDebugLog_FuncOut("GetMicMixBit", &sdRet);
#endif
	return bMicMix;
}

/****************************************************************************
 *	McPacket_AddStart
 *
 *	Description:
 *			Add start packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddStart
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_ADC_INFO	sAdcInfo;
	MCDRV_PDM_INFO	sPdmInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddStart");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetAdcInfo(&sAdcInfo);
	McResCtrl_GetPdmInfo(&sPdmInfo);

	if((McResCtrl_IsDstUsed(eMCDRV_DST_ADC0, eMCDRV_DST_CH0) == 1)
	|| (McResCtrl_IsDstUsed(eMCDRV_DST_ADC0, eMCDRV_DST_CH1) == 1))
	{/*	ADC0 source is used	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START) & MCB_AD_START) == 0)
		{
			bReg	= (sAdcInfo.bAgcOn << 2) | sAdcInfo.bAgcAdjust;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_AGC, bReg);
			bReg	= (sAdcInfo.bMono << 1) | MCB_AD_START;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_START, bReg);
		}
	}
	else if(McResCtrl_IsSrcUsed(eMCDRV_SRC_PDM) != 0)
	{/*	PDM is used	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START) & MCB_PDM_START) == 0)
		{
			bReg	= (sPdmInfo.bAgcOn << 2) | sPdmInfo.bAgcAdjust;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_AGC, bReg);
			bReg	= (sPdmInfo.bMono << 1) | MCB_PDM_START;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_START, bReg);
		}
	}
	else
	{
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddStart", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddStop
 *
 *	Description:
 *			Add stop packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddStop
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddStop");
#endif

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX0_START);
	if(McResCtrl_GetDITSource(eMCDRV_DIO_0) == eMCDRV_SRC_NONE)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_DIT0_SRC_START;
		bReg &= (UINT8)~MCB_DIT0_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR0) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_DIR0_SRC_START;
		bReg &= (UINT8)~MCB_DIR0_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_DITIM0_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX0_START, bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX1_START);
	if(McResCtrl_GetDITSource(eMCDRV_DIO_1) == eMCDRV_SRC_NONE)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_DIT1_SRC_START;
		bReg &= (UINT8)~MCB_DIT1_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR1) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_DIR1_SRC_START;
		bReg &= (UINT8)~MCB_DIR1_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_DITIM1_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX1_START, bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX2_START);
	if(McResCtrl_GetDITSource(eMCDRV_DIO_2) == eMCDRV_SRC_NONE)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_DIT2_SRC_START;
		bReg &= (UINT8)~MCB_DIT2_START;
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_DIR2) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_DIR2_SRC_START;
		bReg &= (UINT8)~MCB_DIR2_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_DITIM2_START;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIX2_START, bReg);

	if((McResCtrl_IsDstUsed(eMCDRV_DST_ADC0, eMCDRV_DST_CH0) == 0)
	&& (McResCtrl_IsDstUsed(eMCDRV_DST_ADC0, eMCDRV_DST_CH1) == 0))
	{/*	ADC0 source is unused	*/
		AddStopADC();
	}
	if(McResCtrl_IsSrcUsed(eMCDRV_SRC_PDM) == 0)
	{/*	PDM is unused	*/
		AddStopPDM();
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddStop", &sdRet);
#endif

	return sdRet;
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
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddVol
(
	UINT32					dUpdate,
	MCDRV_VOLUPDATE_MODE	eMode,
	UINT32*					pdSVolDoneParam
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	UINT8			bVolL;
	UINT8			bVolR;
	UINT8			bLAT;
	UINT8			bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_VOL_INFO	sVolInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddVol");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetVolReg(&sVolInfo);

	if((dUpdate & MCDRV_VOLUPDATE_ANAOUT_ALL) != (UINT32)0)
	{
		*pdSVolDoneParam	= 0;

		bVolL	= (UINT8)sVolInfo.aswA_Hp[0]&MCB_HPVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Hp[1]&MCB_HPVOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_HPVOL_L) & MCB_HPVOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_HPVOL_R)))
				{
					bLAT	= MCB_ALAT_HP;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|MCB_SVOL_HP|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HPVOL_L, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					*pdSVolDoneParam	|= (MCB_HPL_BUSY<<8);
				}
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			if((bVolR == MCDRV_REG_MUTE) && (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_HPVOL_R) != 0))
			{
				*pdSVolDoneParam	|= (UINT8)MCB_HPR_BUSY;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HPVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Sp[0]&MCB_SPVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Sp[1]&MCB_SPVOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SPVOL_L) & MCB_SPVOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SPVOL_R)))
				{
					bLAT	= MCB_ALAT_SP;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|MCB_SVOL_SP|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPVOL_L, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					*pdSVolDoneParam	|= (MCB_SPL_BUSY<<8);
				}
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			if((bVolR == MCDRV_REG_MUTE) && (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SPVOL_R) != 0))
			{
				*pdSVolDoneParam	|= (UINT8)MCB_SPR_BUSY;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SPVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Rc[0]&MCB_RCVOL;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_RCVOL) & MCB_RCVOL))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				bReg	= MCB_SVOL_RC|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_RCVOL, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					*pdSVolDoneParam	|= (MCB_RC_BUSY<<8);
				}
			}
		}

		bVolL	= (UINT8)sVolInfo.aswA_Lout1[0]&MCB_LO1VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lout1[1]&MCB_LO1VOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LO1VOL_L) & MCB_LO1VOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LO1VOL_R)))
				{
					bLAT	= MCB_ALAT_LO1;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO1VOL_L, bReg);
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO1VOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Lout2[0]&MCB_LO2VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lout2[1]&MCB_LO2VOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LO2VOL_L) & MCB_LO2VOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LO2VOL_R)))
				{
					bLAT	= MCB_ALAT_LO2;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO2VOL_L, bReg);
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LO2VOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_HpGain[0];
		if(bVolL != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_HP_GAIN))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_HP_GAIN, bVolL);
			}
		}
	}
	if((dUpdate & ~MCDRV_VOLUPDATE_ANAOUT_ALL) != (UINT32)0)
	{
		bVolL	= (UINT8)sVolInfo.aswA_Lin1[0]&MCB_LI1VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lin1[1]&MCB_LI1VOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LI1VOL_L) & MCB_LI1VOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LI1VOL_R)))
				{
					bLAT	= MCB_ALAT_LI1;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LI1VOL_L, bReg);
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LI1VOL_R, bVolR);
		}

		if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1)
		{
			bVolL	= (UINT8)sVolInfo.aswA_Lin2[0]&MCB_LI2VOL_L;
			bVolR	= (UINT8)sVolInfo.aswA_Lin2[1]&MCB_LI2VOL_R;
			if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LI2VOL_L) & MCB_LI2VOL_L))
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
				{
					if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
					&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_LI2VOL_R)))
					{
						bLAT	= MCB_ALAT_LI2;
					}
					else
					{
						bLAT	= 0;
					}
					bReg	= bLAT|bVolL;
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LI2VOL_L, bReg);
				}
			}
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_LI2VOL_R, bVolR);
			}
		}

		bVolL	= (UINT8)sVolInfo.aswA_Mic1[0]&MCB_MC1VOL;
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC1VOL, bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic2[0]&MCB_MC2VOL;
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC2VOL, bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic3[0]&MCB_MC3VOL;
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC3VOL, bVolL);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Ad0[0]&MCB_ADVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Ad0[1]&MCB_ADVOL_R;
		if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_ADVOL_L) & MCB_ADVOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_ADVOL_R)))
				{
					bLAT	= MCB_ALAT_AD;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADVOL_L, bReg);
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_ADVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Mic2Gain[0]&0x03;
		bVolL	= (UINT8)((bVolL << 4) & MCB_MC2GAIN) | ((UINT8)sVolInfo.aswA_Mic1Gain[0]&MCB_MC1GAIN);
		bVolL |= ((sInitInfo.bMic2Sng << 6) & MCB_MC2SNG);
		bVolL |= ((sInitInfo.bMic1Sng << 2) & MCB_MC1SNG);
		if(eMode == eMCDRV_VOLUPDATE_MUTE)
		{
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_MC_GAIN);
			if(((bReg & MCB_MC2GAIN) == 0) && ((bReg & MCB_MC1GAIN) == 0))
			{
				;
			}
			else
			{
				if((bReg & MCB_MC2GAIN) == 0)
				{
					bVolL &= (UINT8)~MCB_MC2GAIN;
				}
				else if((bReg & MCB_MC1GAIN) == 0)
				{
					bVolL &= (UINT8)~MCB_MC1GAIN;
				}
				else
				{
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC_GAIN, bVolL);
			}
		}
		else
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC_GAIN, bVolL);
		}

		bVolL	= ((UINT8)sVolInfo.aswA_Mic3Gain[0]&MCB_MC3GAIN) | ((sInitInfo.bMic3Sng << 2) & MCB_MC3SNG);
		if(eMode == eMCDRV_VOLUPDATE_MUTE)
		{
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_MC3_GAIN);
			if((bReg & MCB_MC3GAIN) == 0)
			{
				;
			}
			else
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC3_GAIN, bVolL);
			}
		}
		else
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_MC3_GAIN, bVolL);
		}

		/*	DIT0_INVOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dit0[0]&MCB_DIT0_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit0[1]&MCB_DIT0_INVOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIT0_INVOLL, MCB_DIT0_INLAT, MCI_DIT0_INVOLR, eMode);

		/*	DIT1_INVOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dit1[0]&MCB_DIT1_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit1[1]&MCB_DIT1_INVOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIT1_INVOLL, MCB_DIT1_INLAT, MCI_DIT1_INVOLR, eMode);

		/*	DIT2_INVOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dit2[0]&MCB_DIT2_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit2[1]&MCB_DIT2_INVOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIT2_INVOLL, MCB_DIT2_INLAT, MCI_DIT2_INVOLR, eMode);

		/*	PDM0_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Pdm[0]&MCB_PDM0_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Pdm[1]&MCB_PDM0_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_PDM0_VOLL, MCB_PDM0_LAT, MCI_PDM0_VOLR, eMode);

		/*	DIR0_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir0[0]&MCB_DIR0_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir0[1]&MCB_DIR0_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR0_VOLL, MCB_DIR0_LAT, MCI_DIR0_VOLR, eMode);

		/*	DIR1_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir1[0]&MCB_DIR1_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir1[1]&MCB_DIR1_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR1_VOLL, MCB_DIR1_LAT, MCI_DIR1_VOLR, eMode);

		/*	DIR2_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir2[0]&MCB_DIR2_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir2[1]&MCB_DIR2_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR2_VOLL, MCB_DIR2_LAT, MCI_DIR2_VOLR, eMode);

		/*	ADC_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Ad0[0]&MCB_ADC_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Ad0[1]&MCB_ADC_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_ADC_VOLL, MCB_ADC_LAT, MCI_ADC_VOLR, eMode);

		if(McDevProf_IsValid(eMCDRV_FUNC_ADC1) == 1)
		{
#if 0
			bVolL	= (UINT8)sVolInfo.aswD_Ad1[0]&MCB_ADC1_VOLL;
			bVolR	= (UINT8)sVolInfo.aswD_Ad1[1]&MCB_ADC1_VOLR;
			AddDigVolPacket(bVolL, bVolR, MCI_ADC1_VOLL, MCB_ADC1_LAT, MCI_ADC1_VOLR, eMode);
#endif
		}

		/*	AENG6_VOL	*/
		bVolL	= (UINT8)sVolInfo.aswD_Aeng6[0]&MCB_AENG6_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Aeng6[1]&MCB_AENG6_VOLR;
		AddDigVolPacket(bVolL, bVolR, MCI_AENG6_VOLL, MCB_AENG6_LAT, MCI_AENG6_VOLR, eMode);

		/*	ADC_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_Ad0Att[0]&MCB_ADC_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Ad0Att[1]&MCB_ADC_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_ADC_ATTL, MCB_ADC_ALAT, MCI_ADC_ATTR, eMode);

		/*	DIR0_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir0Att[0]&MCB_DIR0_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir0Att[1]&MCB_DIR0_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR0_ATTL, MCB_DIR0_ALAT, MCI_DIR0_ATTR, eMode);

		/*	DIR1_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir1Att[0]&MCB_DIR1_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir1Att[1]&MCB_DIR1_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR1_ATTL, MCB_DIR1_ALAT, MCI_DIR1_ATTR, eMode);

		/*	DIR2_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_Dir2Att[0]&MCB_DIR2_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir2Att[1]&MCB_DIR2_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_DIR2_ATTL, MCB_DIR2_ALAT, MCI_DIR2_ATTR, eMode);

		/*	ST_VOL	*/
		if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_AENG6_SOURCE) == MCB_AENG6_PDM)
		{
			bVolL	= (UINT8)sVolInfo.aswD_SideTone[0]&MCB_ST_VOLL;
			bVolR	= (UINT8)sVolInfo.aswD_SideTone[1]&MCB_ST_VOLR;
			AddDigVolPacket(bVolL, bVolR, MCI_ST_VOLL, MCB_ST_LAT, MCI_ST_VOLR, eMode);
		}

		/*	MASTER_OUT	*/
		bVolL	= (UINT8)sVolInfo.aswD_DacMaster[0]&MCB_MASTER_OUTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacMaster[1]&MCB_MASTER_OUTR;
		AddDigVolPacket(bVolL, bVolR, MCI_MASTER_OUTL, MCB_MASTER_OLAT, MCI_MASTER_OUTR, eMode);

		/*	VOICE_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_DacVoice[0]&MCB_VOICE_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacVoice[1]&MCB_VOICE_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_VOICE_ATTL, MCB_VOICE_LAT, MCI_VOICE_ATTR, eMode);

		/*	DAC_ATT	*/
		bVolL	= (UINT8)sVolInfo.aswD_DacAtt[0]&MCB_DAC_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacAtt[1]&MCB_DAC_ATTR;
		AddDigVolPacket(bVolL, bVolR, MCI_DAC_ATTL, MCB_DAC_LAT, MCI_DAC_ATTR, eMode);
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddVol", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	AddDigVolPacket
 *
 *	Description:
 *			Add digital vol setup packet.
 *	Arguments:
 *				bVolL		Left volume
 *				bVolR		Right volume
 *				bVolLAddr	Left volume register address
 *				bLAT		LAT
 *				bVolRAddr	Right volume register address
 *				eMode		update mode
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDigVolPacket
(
	UINT8	bVolL,
	UINT8	bVolR,
	UINT8	bVolLAddr,
	UINT8	bLAT,
	UINT8	bVolRAddr,
	MCDRV_VOLUPDATE_MODE	eMode
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddDigVolPacket");
#endif

	if(bVolL != (McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, bVolLAddr) & (UINT8)~bLAT))
	{
		if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolL == MCDRV_REG_MUTE))
		{
			if(((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
			&& (bVolR != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, bVolRAddr)))
			{
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)bVolLAddr, bLAT|bVolL);
		}
	}
	if((eMode != eMCDRV_VOLUPDATE_MUTE) || (bVolR == MCDRV_REG_MUTE))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)bVolRAddr, bVolR);
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddDigVolPacket", 0);
#endif
}

/****************************************************************************
 *	AddStopADC
 *
 *	Description:
 *			Add stop ADC packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddStopADC
(
	void
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddStopADC");
#endif

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START);

	if((bReg & MCB_AD_START) != 0)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_START, bReg&(UINT8)~MCB_AD_START);
	}

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_ADCMUTE, 0);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddStopADC", 0);
#endif
}

/****************************************************************************
 *	AddStopPDM
 *
 *	Description:
 *			Add stop PDM packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddStopPDM
(
	void
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("AddStopPDM");
#endif

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START);

	if((bReg & MCB_PDM_START) != 0)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_START, bReg&(UINT8)~MCB_PDM_START);
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddStopPDM", 0);
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
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddDigitalIO
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;
	MCDRV_DIO_INFO		sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddDigitalIO");
#endif

	if(IsModifiedDIO(dUpdateInfo) != 0)
	{
		McResCtrl_GetCurPowerInfo(&sPowerInfo);
		sdRet	= PowerUpDig(MCDRV_DPB_UP);
		if(sdRet == MCDRV_SUCCESS)
		{
			if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIOCommon(eMCDRV_DIO_0);
			}
			if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIOCommon(eMCDRV_DIO_1);
			}
			if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIOCommon(eMCDRV_DIO_2);
			}

			/*	DI*_BCKP	*/
			if(((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0))
			{
				McResCtrl_GetDioInfo(&sDioInfo);
				bReg	= 0;
				if(sDioInfo.asPortInfo[0].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
				{
					bReg |= MCB_DI0_BCKP;
				}
				if(sDioInfo.asPortInfo[1].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
				{
					bReg |= MCB_DI1_BCKP;
				}
				if(sDioInfo.asPortInfo[2].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
				{
					bReg |= MCB_DI2_BCKP;
				}
				if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_BCKP))
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_BCKP, bReg);
				}
			}

			if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIR(eMCDRV_DIO_0);
			}
			if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIR(eMCDRV_DIO_1);
			}
			if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIR(eMCDRV_DIO_2);
			}

			if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIT(eMCDRV_DIO_0);
			}
			if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIT(eMCDRV_DIO_1);
			}
			if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != (UINT32)0)
			{
				AddDIODIT(eMCDRV_DIO_2);
			}

			/*	unused path power down	*/
			sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
			sPowerUpdate.abAnalog[0]	= 
			sPowerUpdate.abAnalog[1]	= 
			sPowerUpdate.abAnalog[2]	= 
			sPowerUpdate.abAnalog[3]	= 
			sPowerUpdate.abAnalog[4]	= 0;
			sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddDigitalIO", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	IsModifiedDIO
 *
 *	Description:
 *			Is modified DigitalIO.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIO
(
	UINT32		dUpdateInfo
)
{
	UINT8	bModified	= 0;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsModifiedDIO");
#endif

	if((((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIOCommon(eMCDRV_DIO_0) != 0))
	|| (((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIOCommon(eMCDRV_DIO_1) != 0))
	|| (((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIOCommon(eMCDRV_DIO_2) != 0)))
	{
		bModified	= 1;
	}
	else if((((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIR(eMCDRV_DIO_0) != 0))
	|| (((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIR(eMCDRV_DIO_1) != 0))
	|| (((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIR(eMCDRV_DIO_2) != 0)))
	{
		bModified	= 1;
	}
	else if((((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIT(eMCDRV_DIO_0) != 0))
	|| (((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIT(eMCDRV_DIO_1) != 0))
	|| (((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != (UINT32)0) && (IsModifiedDIODIT(eMCDRV_DIO_2) != 0)))
	{
		bModified	= 1;
	}
	else
	{
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bModified;
	McDebugLog_FuncOut("IsModifiedDIO", &sdRet);
#endif
	return bModified;
}

/****************************************************************************
 *	IsModifiedDIOCommon
 *
 *	Description:
 *			Is modified DigitalIO Common.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIOCommon
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT8	bModified	= 0;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsModifiedDIOCommon");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{
		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("IsModifiedDIOCommon", &sdRet);
		#endif
		return 0;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIMODE0+bRegOffset))
	{
		bModified	= 1;
	}

	bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs << 7)
			| (sDioInfo.asPortInfo[ePort].sDioCommon.bBckFs << 4)
			| sDioInfo.asPortInfo[ePort].sDioCommon.bFs;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI_FS0+bRegOffset))
	{
		bModified	= 1;
	}

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI0_SRC+bRegOffset);
	if((sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs == 0)
	&& (sDioInfo.asPortInfo[ePort].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE))
	{
		bReg |= MCB_DICOMMON_SRC_RATE_SET;
	}
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI0_SRC+bRegOffset))
	{
		bModified	= 1;
	}
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHizTim << 7)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmClkDown << 6)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmFrame << 5)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHighPeriod);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_HIZ_REDGE0+bRegOffset))
		{
			bModified	= 1;
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bModified;
	McDebugLog_FuncOut("IsModifiedDIOCommon", &sdRet);
#endif
	return bModified;
}

/****************************************************************************
 *	IsModifiedDIODIR
 *
 *	Description:
 *			Is modified DigitalIO DIR.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIODIR
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT8	bModified	= 0;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsModifiedDIODIR");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{
		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("IsModifiedDIODIR", &sdRet);
		#endif
		return 0;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	bReg	= (UINT8)(sDioInfo.asPortInfo[ePort].sDir.wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIRSRC_RATE0_MSB+bRegOffset))
	{
		bModified	= 1;
	}

	bReg	= (UINT8)sDioInfo.asPortInfo[ePort].sDir.wSrcRate;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIRSRC_RATE0_LSB+bRegOffset))
	{
		bModified	= 1;
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX0_FMT+bRegOffset))
		{
			bModified	= 1;
		}
		/*	DIR*_CH	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIR0_CH+bRegOffset))
		{
			bModified	= 1;
		}
	}
	else
	{
		/*	PCM_MONO_RX*, PCM_EXTEND_RX*, PCM_LSBON_RX*, PCM_LAW_RX*, PCM_BIT_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_RX0+bRegOffset))
		{
			bModified	= 1;
		}
		/*	PCM_CH1_RX*, PCM_CH0_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_SLOT_RX0+bRegOffset))
		{
			bModified	= 1;
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bModified;
	McDebugLog_FuncOut("IsModifiedDIODIR", &sdRet);
#endif
	return bModified;
}

/****************************************************************************
 *	IsModifiedDIODIT
 *
 *	Description:
 *			Is modified DigitalIO DIT.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIODIT
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT8	bModified	= 0;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsModifiedDIODIT");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{
		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("IsModifiedDIODIT", &sdRet);
		#endif
		return 0;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	bReg	= (UINT8)(sDioInfo.asPortInfo[ePort].sDit.wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DITSRC_RATE0_MSB+bRegOffset))
	{
		bModified	= 1;
	}
	bReg	= (UINT8)sDioInfo.asPortInfo[ePort].sDit.wSrcRate;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DITSRC_RATE0_LSB+bRegOffset))
	{
		bModified	= 1;
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX0_FMT+bRegOffset))
		{
			bModified	= 1;
		}

		/*	DIT*_SLOT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT0_SLOT+bRegOffset))
		{
			bModified	= 1;
		}
	}
	else
	{
		/*	PCM_MONO_TX*, PCM_EXTEND_TX*, PCM_LSBON_TX*, PCM_LAW_TX*, PCM_BIT_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_TX0+bRegOffset))
		{
			bModified	= 1;
		}

		/*	PCM_CH1_TX*, PCM_CH0_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_SLOT_TX0+bRegOffset))
		{
			bModified	= 1;
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bModified;
	McDebugLog_FuncOut("IsModifiedDIODIT", &sdRet);
#endif
	return bModified;
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
static void	AddDIOCommon
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIOCommon");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{

		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("AddDIOCommon", &sdRet);
		#endif
		return;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	/*	DIMODE*	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIMODE0+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIMODE0+bRegOffset),
							sDioInfo.asPortInfo[ePort].sDioCommon.bInterface);
	}

	/*	DIAUTO_FS*, DIBCK*, DIFS*	*/
	bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs << 7)
			| (sDioInfo.asPortInfo[ePort].sDioCommon.bBckFs << 4)
			| sDioInfo.asPortInfo[ePort].sDioCommon.bFs;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI_FS0+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DI_FS0+bRegOffset), bReg);
	}

	/*	DI*_SRCRATE_SET	*/
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI0_SRC+bRegOffset);
	if((sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs == 0)
	&& (sDioInfo.asPortInfo[ePort].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE))
	{
		bReg |= MCB_DICOMMON_SRC_RATE_SET;
	}
	else
	{
		bReg &= (UINT8)~MCB_DICOMMON_SRC_RATE_SET;
	}
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DI0_SRC+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DI0_SRC+bRegOffset), bReg);
	}

	/*	HIZ_REDGE*, PCM_CLKDOWN*, PCM_FRAME*, PCM_HPERIOD*	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHizTim << 7)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmClkDown << 6)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmFrame << 5)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHighPeriod);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_HIZ_REDGE0+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_HIZ_REDGE0+bRegOffset), bReg);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
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
static void	AddDIODIR
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT16	wSrcRate;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIODIR");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{
		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("AddDIODIR", &sdRet);
		#endif
		return;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	/*	DIRSRC_RATE*	*/
	wSrcRate	= sDioInfo.asPortInfo[ePort].sDir.wSrcRate;
	if(wSrcRate == 0)
	{
		switch(sDioInfo.asPortInfo[ePort].sDioCommon.bFs)
		{
		case MCDRV_FS_48000:
			wSrcRate	= MCDRV_DIR_SRCRATE_48000;
			break;
		case MCDRV_FS_44100:
			wSrcRate	= MCDRV_DIR_SRCRATE_44100;
			break;
		case MCDRV_FS_32000:
			wSrcRate	= MCDRV_DIR_SRCRATE_32000;
			break;
		case MCDRV_FS_24000:
			wSrcRate	= MCDRV_DIR_SRCRATE_24000;
			break;
		case MCDRV_FS_22050:
			wSrcRate	= MCDRV_DIR_SRCRATE_22050;
			break;
		case MCDRV_FS_16000:
			wSrcRate	= MCDRV_DIR_SRCRATE_16000;
			break;
		case MCDRV_FS_12000:
			wSrcRate	= MCDRV_DIR_SRCRATE_12000;
			break;
		case MCDRV_FS_11025:
			wSrcRate	= MCDRV_DIR_SRCRATE_11025;
			break;
		case MCDRV_FS_8000:
			wSrcRate	= MCDRV_DIR_SRCRATE_8000;
			break;
		default:
			/* unreachable */
			wSrcRate = 0;
			break;
		}
	}
	bReg	= (UINT8)(wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIRSRC_RATE0_MSB+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIRSRC_RATE0_MSB+bRegOffset), bReg);
	}
	bReg	= (UINT8)wSrcRate;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIRSRC_RATE0_LSB+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIRSRC_RATE0_LSB+bRegOffset), bReg);
	}

	/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX0_FMT+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIX0_FMT+bRegOffset), bReg);
		}
		/*	DIR*_CH	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIR0_CH+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIR0_CH+bRegOffset), bReg);
		}
	}
	else
	{
		/*	PCM_MONO_RX*, PCM_EXTEND_RX*, PCM_LSBON_RX*, PCM_LAW_RX*, PCM_BIT_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_RX0+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_PCM_RX0+bRegOffset), bReg);
		}
		/*	PCM_CH1_RX*, PCM_CH0_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_SLOT_RX0+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_PCM_SLOT_RX0+bRegOffset), bReg);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
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
static void	AddDIODIT
(
	MCDRV_DIO_PORT_NO	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT16	wSrcRate;
	MCDRV_DIO_INFO	sDioInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("AddDIODIT");
#endif

	if(ePort == eMCDRV_DIO_0)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1)
	{
		bRegOffset	= MCI_DIMODE1 - MCI_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2)
	{
		bRegOffset	= MCI_DIMODE2 - MCI_DIMODE0;
	}
	else
	{
		#if (MCDRV_DEBUG_LEVEL>=4)
			sdRet	= MCDRV_ERROR;
			McDebugLog_FuncOut("AddDIODIT", &sdRet);
		#endif
		return;
	}

	McResCtrl_GetDioInfo(&sDioInfo);

	wSrcRate	= sDioInfo.asPortInfo[ePort].sDit.wSrcRate;
	if(wSrcRate == 0)
	{
		switch(sDioInfo.asPortInfo[ePort].sDioCommon.bFs)
		{
		case MCDRV_FS_48000:
			wSrcRate	= MCDRV_DIT_SRCRATE_48000;
			break;
		case MCDRV_FS_44100:
			wSrcRate	= MCDRV_DIT_SRCRATE_44100;
			break;
		case MCDRV_FS_32000:
			wSrcRate	= MCDRV_DIT_SRCRATE_32000;
			break;
		case MCDRV_FS_24000:
			wSrcRate	= MCDRV_DIT_SRCRATE_24000;
			break;
		case MCDRV_FS_22050:
			wSrcRate	= MCDRV_DIT_SRCRATE_22050;
			break;
		case MCDRV_FS_16000:
			wSrcRate	= MCDRV_DIT_SRCRATE_16000;
			break;
		case MCDRV_FS_12000:
			wSrcRate	= MCDRV_DIT_SRCRATE_12000;
			break;
		case MCDRV_FS_11025:
			wSrcRate	= MCDRV_DIT_SRCRATE_11025;
			break;
		case MCDRV_FS_8000:
			wSrcRate	= MCDRV_DIT_SRCRATE_8000;
			break;
		default:
			/* unreachable */
			wSrcRate = 0;
			break;
		}
	}
	/*	DITSRC_RATE*	*/
	bReg	= (UINT8)(wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DITSRC_RATE0_MSB+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DITSRC_RATE0_MSB+bRegOffset), bReg);
	}
	bReg	= (UINT8)wSrcRate;
	if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DITSRC_RATE0_LSB+bRegOffset))
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DITSRC_RATE0_LSB+bRegOffset), bReg);
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIX0_FMT+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIX0_FMT+bRegOffset), bReg);
		}

		/*	DIT*_SLOT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT0_SLOT+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_DIT0_SLOT+bRegOffset), bReg);
		}
	}
	else
	{
		/*	PCM_MONO_TX*, PCM_EXTEND_TX*, PCM_LSBON_TX*, PCM_LAW_TX*, PCM_BIT_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_TX0+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_PCM_TX0+bRegOffset), bReg);
		}

		/*	PCM_CH1_TX*, PCM_CH0_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PCM_SLOT_TX0+bRegOffset))
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)(MCI_PCM_SLOT_TX0+bRegOffset), bReg);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("AddDIODIT", 0);
#endif
}

/****************************************************************************
 *	McPacket_AddDAC
 *
 *	Description:
 *			Add DAC setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddDAC
(
	UINT32			dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;
	MCDRV_DAC_INFO		sDacInfo;
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddDAC");
#endif

	McResCtrl_GetDacInfo(&sDacInfo);

	if(((dUpdateInfo & MCDRV_DAC_MSWP_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_DAC_VSWP_UPDATE_FLAG) != (UINT32)0))
	{
		bReg	= (sDacInfo.bMasterSwap<<4)|sDacInfo.bVoiceSwap;
		if(bReg == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_SWP))
		{
			dUpdateInfo	&= ~(MCDRV_DAC_MSWP_UPDATE_FLAG|MCDRV_DAC_VSWP_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_DAC_HPF_UPDATE_FLAG) != (UINT32)0)
	{
		if(sDacInfo.bDcCut == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DCCUTOFF))
		{
			dUpdateInfo	&= ~(MCDRV_DAC_HPF_UPDATE_FLAG);
		}
	}

	if(dUpdateInfo != (UINT32)0)
	{
		McResCtrl_GetCurPowerInfo(&sPowerInfo);
		sdRet	= PowerUpDig(MCDRV_DPB_UP);
		if(sdRet == MCDRV_SUCCESS)
		{
			if(((dUpdateInfo & MCDRV_DAC_MSWP_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_DAC_VSWP_UPDATE_FLAG) != (UINT32)0))
			{
				bReg	= (sDacInfo.bMasterSwap<<4)|sDacInfo.bVoiceSwap;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_SWP, bReg);
			}
			if((dUpdateInfo & MCDRV_DAC_HPF_UPDATE_FLAG) != (UINT32)0)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DCCUTOFF, sDacInfo.bDcCut);
			}

			/*	unused path power down	*/
			sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
			sPowerUpdate.abAnalog[0]	= 
			sPowerUpdate.abAnalog[1]	= 
			sPowerUpdate.abAnalog[2]	= 
			sPowerUpdate.abAnalog[3]	= 
			sPowerUpdate.abAnalog[4]	= 0;
			sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddDAC", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddADC
 *
 *	Description:
 *			Add ADC setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddADC
(
	UINT32			dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;
	MCDRV_ADC_INFO		sAdcInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddADC");
#endif

	McResCtrl_GetAdcInfo(&sAdcInfo);

	if(((dUpdateInfo & MCDRV_ADCADJ_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_ADCAGC_UPDATE_FLAG) != (UINT32)0))
	{
		bReg	= (sAdcInfo.bAgcOn<<2)|sAdcInfo.bAgcAdjust;
		if(bReg == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_AGC))
		{
			dUpdateInfo	&= ~(MCDRV_ADCADJ_UPDATE_FLAG|MCDRV_ADCAGC_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_ADCMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (UINT8)((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START) & MCB_AD_START) | (sAdcInfo.bMono << 1));
		if(bReg == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START))
		{
			dUpdateInfo	&= ~(MCDRV_ADCMONO_UPDATE_FLAG);
		}
	}

	if(dUpdateInfo != (UINT32)0)
	{
		McResCtrl_GetCurPowerInfo(&sPowerInfo);
		sdRet	= PowerUpDig(MCDRV_DPB_UP);
		if(sdRet == MCDRV_SUCCESS)
		{
			if(((dUpdateInfo & MCDRV_ADCADJ_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_ADCAGC_UPDATE_FLAG) != (UINT32)0))
			{
				bReg	= (sAdcInfo.bAgcOn<<2)|sAdcInfo.bAgcAdjust;
				if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_AGC))
				{
					AddStopADC();
					sdRet	= McDevIf_ExecutePacket();
				}
				if(MCDRV_SUCCESS == sdRet)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_AGC, bReg);
				}
			}
			if(sdRet == MCDRV_SUCCESS)
			{
				if((dUpdateInfo & MCDRV_ADCMONO_UPDATE_FLAG) != (UINT32)0)
				{
					bReg	= (UINT8)((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START) & MCB_AD_START) | (sAdcInfo.bMono << 1));
					if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_AD_START))
					{
						AddStopADC();
						sdRet	= McDevIf_ExecutePacket();
					}
					if(sdRet == MCDRV_SUCCESS)
					{
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_AD_START, (sAdcInfo.bMono << 1));
					}
				}
				if(sdRet == MCDRV_SUCCESS)
				{
					/*	unused path power down	*/
					sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
					sPowerUpdate.abAnalog[0]	= 
					sPowerUpdate.abAnalog[1]	= 
					sPowerUpdate.abAnalog[2]	= 
					sPowerUpdate.abAnalog[3]	= 
					sPowerUpdate.abAnalog[4]	= 0;
					sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
					if(sdRet == MCDRV_SUCCESS)
					{
						sdRet	= McPacket_AddStart();
					}
				}
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddADC", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddSP
 *
 *	Description:
 *			Add SP setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddSP
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_SP_INFO	sSpInfo;
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddSP");
#endif

	bReg	= (UINT8)(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_SP_MODE) & (UINT8)~MCB_SP_SWAP);

	McResCtrl_GetSpInfo(&sSpInfo);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)MCI_SP_MODE, bReg|sSpInfo.bSwap);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddSP", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddDNG
 *
 *	Description:
 *			Add Digital Noise Gate setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddDNG
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_DNG_INFO	sDngInfo;
	UINT8	bReg;
	UINT8	bRegDNGON;
	UINT8	bItem;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddDNG");
#endif

	McResCtrl_GetDngInfo(&sDngInfo);

	for(bItem = MCDRV_DNG_ITEM_HP; bItem <= MCDRV_DNG_ITEM_RC; bItem++)
	{
		bRegDNGON	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_DNGON_HP+(bItem*3));

		/*	ReleseTime, Attack Time	*/
		if(((dUpdateInfo & (MCDRV_DNGREL_HP_UPDATE_FLAG<<(8*bItem))) != 0UL) || ((dUpdateInfo & (MCDRV_DNGATK_HP_UPDATE_FLAG<<(8*bItem))) != 0UL))
		{
			bReg	= (sDngInfo.abRelease[bItem]<<4)|sDngInfo.abAttack[bItem];
			if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_DNGATRT_HP+(bItem*3)))
			{
				if((bRegDNGON&0x01) != 0)
				{/*	DNG on	*/
					/*	DNG off	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGON_HP+(bItem*3)), bRegDNGON&0xFE);
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGATRT_HP+(bItem*3)), bReg);
			}
		}

		/*	Target	*/
		if((dUpdateInfo & (MCDRV_DNGTARGET_HP_UPDATE_FLAG<<(8*bItem))) != 0UL)
		{
			bReg	= sDngInfo.abTarget[bItem]<<4;
			if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_DNGTARGET_HP+(bItem*3)))
			{
				if((bRegDNGON&0x01) != 0)
				{/*	DNG on	*/
					/*	DNG off	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGON_HP+(bItem*3)), bRegDNGON&0xFE);
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGTARGET_HP+(bItem*3)), bReg);
			}
		}

		/*	Threshold, HoldTime	*/
		if(((dUpdateInfo & (MCDRV_DNGTHRES_HP_UPDATE_FLAG<<(8*bItem))) != 0UL)
		|| ((dUpdateInfo & (MCDRV_DNGHOLD_HP_UPDATE_FLAG<<(8*bItem))) != 0UL))
		{
			bReg	= (sDngInfo.abThreshold[bItem]<<4)|(sDngInfo.abHold[bItem]<<1);
			if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_ANA, MCI_DNGON_HP+(bItem*3)))
			{
				if((bRegDNGON&0x01) != 0)
				{/*	DNG on	*/
					/*	DNG off	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGON_HP+(bItem*3)), bRegDNGON&0xFE);
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGON_HP+(bItem*3)), bReg);
			}
			bRegDNGON	= bReg | (bRegDNGON&0x01);
		}

		/*	DNGON	*/
		if((dUpdateInfo & (MCDRV_DNGSW_HP_UPDATE_FLAG<<(8*bItem))) != 0UL)
		{
			bRegDNGON	= (bRegDNGON&0xFE) | sDngInfo.abOnOff[bItem];
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | (UINT32)(MCI_DNGON_HP+(bItem*3)), bRegDNGON);
	}


#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddDNG", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddAE
 *
 *	Description:
 *			Add Audio Engine setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddAE
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	i;
	UINT32	dXFadeParam	= 0;
	MCDRV_AE_INFO	sAeInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddAE");
#endif

	McResCtrl_GetPathInfo(&sPathInfo);
	McResCtrl_GetAeInfo(&sAeInfo);

	if(McResCtrl_IsDstUsed(eMCDRV_DST_AE, eMCDRV_DST_CH0) == 1)
	{/*	AE is used	*/
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_BDSP_ST);
		if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
		{
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF) != (UINT32)0)
			{
				if((((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0) && ((bReg & MCB_DBEXON) != 0))
				|| (((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) == 0) && ((bReg & MCB_DBEXON) == 0)))
				{
					dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF;
				}
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC_ONOFF) != (UINT32)0)
		{
			if((((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0) && ((bReg & MCB_DRCON) != 0))
			|| (((sAeInfo.bOnOff & MCDRV_DRC_ON) == 0) && ((bReg & MCB_DRCON) == 0)))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_DRC_ONOFF;
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5_ONOFF) != (UINT32)0)
		{
			if((((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0) && ((bReg & MCB_EQ5ON) != 0))
			|| (((sAeInfo.bOnOff & MCDRV_EQ5_ON) == 0) && ((bReg & MCB_EQ5ON) == 0)))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_EQ5_ONOFF;
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3_ONOFF) != (UINT32)0)
		{
			if((((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0) && ((bReg & MCB_EQ3ON) != 0))
			|| (((sAeInfo.bOnOff & MCDRV_EQ3_ON) == 0) && ((bReg & MCB_EQ3ON) == 0)))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_EQ3_ONOFF;
			}
		}
		if(dUpdateInfo != (UINT32)0)
		{
			/*	on/off setting or param changed	*/
			if(((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEX) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_WIDE) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC_ONOFF) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5_ONOFF) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3_ONOFF) != (UINT32)0))
			{
				dXFadeParam	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DAC_INS);
				dXFadeParam	<<= 8;
				dXFadeParam	|= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_INS);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DAC_INS, 0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_INS, 0);
			}
			sdRet	= McDevIf_ExecutePacket();
			if(sdRet == MCDRV_SUCCESS)
			{
				/*	wait xfade complete	*/
				if(dXFadeParam != (UINT32)0)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_INSFLG | dXFadeParam, 0);
					bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_BDSP_ST);
					if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0)
					{
						bReg	&= (UINT8)~MCB_EQ5ON;
					}
					if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0)
					{
						bReg	&= (UINT8)~MCB_EQ3ON;
					}
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_ST, bReg);
				}

				bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_BDSP_ST);
				if((bReg & MCB_BDSP_ST) == MCB_BDSP_ST)
				{
					/*	Stop BDSP	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_ST, 0);
					/*	Reset TRAM	*/
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_RST, MCB_TRAM_RST);
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_RST, 0);
				}
			}
		}
	}

	if((dUpdateInfo != (UINT32)0) && (sdRet == MCDRV_SUCCESS))
	{
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC) != (UINT32)0)
		{
			McResCtrl_GetPowerInfo(&sPowerInfo);
			sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP0;
			sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP1;
			sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP2;
			sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DPBDSP;
			sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_PLLRST0;
			sPowerUpdate.dDigital	= MCDRV_POWUPDATE_DIGITAL_ALL;
			sPowerUpdate.abAnalog[0]	= 
			sPowerUpdate.abAnalog[1]	= 
			sPowerUpdate.abAnalog[2]	= 
			sPowerUpdate.abAnalog[3]	= 
			sPowerUpdate.abAnalog[4]	= 0;
			sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
			if(MCDRV_SUCCESS == sdRet)
			{
				sdRet	= McDevIf_ExecutePacket();
			}
			if(MCDRV_SUCCESS == sdRet)
			{
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_ADR, 0);
				sdRet	= McDevIf_ExecutePacket();
				if(MCDRV_SUCCESS == sdRet)
				{
					McDevIf_AddPacketRepeat(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_A | (UINT32)MCI_BDSP_WINDOW, sAeInfo.abDrc, DRC_PARAM_SIZE);
				}
			}
		}

		if(MCDRV_SUCCESS == sdRet)
		{
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0)
			{
				for(i = 0; i < EQ5_PARAM_SIZE; i++)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_AE | (UINT32)(MCI_BAND0_CEQ0+i), sAeInfo.abEq5[i]);
				}
			}
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0)
			{
				for(i = 0; i < EQ3_PARAM_SIZE; i++)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_AE | (UINT32)(MCI_BAND5_CEQ0+i), sAeInfo.abEq3[i]);
				}
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddAE", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPDM
 *
 *	Description:
 *			Add PDM setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPDM
(
	UINT32			dUpdateInfo
)
{
	SINT32				sdRet		= MCDRV_SUCCESS;
	UINT8				bReg;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;
	MCDRV_PDM_INFO		sPdmInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddPDM");
#endif

	McResCtrl_GetPdmInfo(&sPdmInfo);
	if(((dUpdateInfo & MCDRV_PDMADJ_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_PDMAGC_UPDATE_FLAG) != (UINT32)0))
	{
		bReg	= (sPdmInfo.bAgcOn<<2)|sPdmInfo.bAgcAdjust;
		if(bReg == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_AGC))
		{
			dUpdateInfo &= ~(MCDRV_PDMADJ_UPDATE_FLAG|MCDRV_PDMAGC_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_PDMMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (UINT8)(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START) & (UINT8)MCB_PDM_MN);
		if((sPdmInfo.bMono<<1) == bReg)
		{
			dUpdateInfo &= ~(MCDRV_PDMMONO_UPDATE_FLAG);
		}
	}
	if(((dUpdateInfo & MCDRV_PDMCLK_UPDATE_FLAG) != (UINT32)0)
	|| ((dUpdateInfo & MCDRV_PDMEDGE_UPDATE_FLAG) != (UINT32)0)
	|| ((dUpdateInfo & MCDRV_PDMWAIT_UPDATE_FLAG) != (UINT32)0)
	|| ((dUpdateInfo & MCDRV_PDMSEL_UPDATE_FLAG) != (UINT32)0))
	{
		bReg	= (sPdmInfo.bPdmWait<<5) | (sPdmInfo.bPdmEdge<<4) | (sPdmInfo.bPdmSel<<2) | sPdmInfo.bClk;
		if(bReg == McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_STWAIT))
		{
			dUpdateInfo &= ~(MCDRV_PDMCLK_UPDATE_FLAG|MCDRV_PDMEDGE_UPDATE_FLAG|MCDRV_PDMWAIT_UPDATE_FLAG|MCDRV_PDMSEL_UPDATE_FLAG);
		}
	}
	if(dUpdateInfo != (UINT32)0)
	{
		McResCtrl_GetCurPowerInfo(&sPowerInfo);
		sdRet	= PowerUpDig(MCDRV_DPB_UP);
		if(sdRet == MCDRV_SUCCESS)
		{
			if(((dUpdateInfo & MCDRV_PDMADJ_UPDATE_FLAG) != (UINT32)0) || ((dUpdateInfo & MCDRV_PDMAGC_UPDATE_FLAG) != (UINT32)0))
			{
				bReg	= (sPdmInfo.bAgcOn<<2)|sPdmInfo.bAgcAdjust;
				if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_AGC))
				{
					AddStopPDM();
					sdRet	= McDevIf_ExecutePacket();
				}
				if(sdRet == MCDRV_SUCCESS)
				{
					McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_AGC, bReg);
				}
			}
		}
		if(sdRet == MCDRV_SUCCESS)
		{
			if((dUpdateInfo & MCDRV_PDMMONO_UPDATE_FLAG) != (UINT32)0)
			{
				bReg	= (UINT8)((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START) & (UINT8)~MCB_PDM_MN) | (sPdmInfo.bMono<<1));
				if(bReg != McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_PDM_START))
				{
					AddStopPDM();
					sdRet	= McDevIf_ExecutePacket();
					if(MCDRV_SUCCESS == sdRet)
					{
						McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_START, (sPdmInfo.bMono<<1));
					}
				}
			}
		}
		if(sdRet == MCDRV_SUCCESS)
		{
			if(((dUpdateInfo & MCDRV_PDMCLK_UPDATE_FLAG) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_PDMEDGE_UPDATE_FLAG) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_PDMWAIT_UPDATE_FLAG) != (UINT32)0)
			|| ((dUpdateInfo & MCDRV_PDMSEL_UPDATE_FLAG) != (UINT32)0))
			{
				bReg	= (sPdmInfo.bPdmWait<<5) | (sPdmInfo.bPdmEdge<<4) | (sPdmInfo.bPdmSel<<2) | sPdmInfo.bClk;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_PDM_STWAIT, bReg);
			}

			/*	unused path power down	*/
			sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
			sPowerUpdate.abAnalog[0]	= 
			sPowerUpdate.abAnalog[1]	= 
			sPowerUpdate.abAnalog[2]	= 
			sPowerUpdate.abAnalog[3]	= 
			sPowerUpdate.abAnalog[4]	= 0;
			sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
			if(sdRet == MCDRV_SUCCESS)
			{
				sdRet	= McPacket_AddStart();
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddPDM", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddGPMode
 *
 *	Description:
 *			Add GP mode setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddGPMode
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddGPMode");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetGPMode(&sGPMode);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PA_MSK);

	if(sInitInfo.bPad0Func == MCDRV_PAD_GPIO)
	{
		if(sGPMode.abGpDdr[0] == MCDRV_GPDDR_IN)
		{
			bReg	&= (UINT8)~MCB_PA0_DDR;
		}
		else
		{
			bReg	|= MCB_PA0_DDR;
		}
	}
	if(sInitInfo.bPad1Func == MCDRV_PAD_GPIO)
	{
		if(sGPMode.abGpDdr[1] == MCDRV_GPDDR_IN)
		{
			bReg	&= (UINT8)~MCB_PA1_DDR;
		}
		else
		{
			bReg	|= MCB_PA1_DDR;
		}
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_MSK, bReg);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddGPMode", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddGPMask
 *
 *	Description:
 *			Add GP mask setup packet.
 *	Arguments:
 *			dPadNo		PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32	McPacket_AddGPMask
(
	UINT32	dPadNo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;
	UINT8	abMask[GPIO_PAD_NUM];

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddGPMask");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetGPMode(&sGPMode);
	McResCtrl_GetGPMask(abMask);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if((sInitInfo.bPad0Func == MCDRV_PAD_GPIO) && (sGPMode.abGpDdr[0] == MCDRV_GPDDR_IN))
		{
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PA_MSK);
			if(abMask[0] == MCDRV_GPMASK_OFF)
			{
				bReg	&= (UINT8)~MCB_PA0_MSK;
			}
			else
			{
				bReg	|= MCB_PA0_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_MSK, bReg);
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if((sInitInfo.bPad1Func == MCDRV_PAD_GPIO) && (sGPMode.abGpDdr[1] == MCDRV_GPDDR_IN))
		{
			bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PA_MSK);
			if(abMask[1] == MCDRV_GPMASK_OFF)
			{
				bReg	&= (UINT8)~MCB_PA1_MSK;
			}
			else
			{
				bReg	|= MCB_PA1_MSK;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_MSK, bReg);
		}
	}
	else
	{
		sdRet	= MCDRV_ERROR;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddGPMask", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddGPSet
 *
 *	Description:
 *			Add GPIO output packet.
 *	Arguments:
 *			bGpio		pin state setting
 *			dPadNo		PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32	McPacket_AddGPSet
(
	UINT8	bGpio,
	UINT32	dPadNo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddGPSet");
#endif

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_BASE, MCI_PA_SCU_PA);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if(bGpio == MCDRV_GP_LOW)
		{
			bReg	&= (UINT8)~MCB_PA_SCU_PA0;
		}
		else if(bGpio == MCDRV_GP_HIGH)
		{
			bReg	|= MCB_PA_SCU_PA0;
		}
		else
		{
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if(bGpio == MCDRV_GP_LOW)
		{
			bReg	&= (UINT8)~MCB_PA_SCU_PA1;
		}
		else if(bGpio == MCDRV_GP_HIGH)
		{
			bReg	|= MCB_PA_SCU_PA1;
		}
		else
		{
		}
	}
	else
	{
		sdRet	= MCDRV_ERROR;
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | (UINT32)MCI_PA_SCU_PA, bReg);
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddGPSet", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddSysEq
 *
 *	Description:
 *			Add GPIO output packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddSysEq
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bRegEQOn;
	UINT8	bReg;
	MCDRV_SYSEQ_INFO	sSysEq;
	UINT8	i;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddSysEq");
#endif

	bRegEQOn	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_SYSTEM_EQON);

	McResCtrl_GetSysEq(&sSysEq);

	if((dUpdateInfo & MCDRV_SYSEQ_PARAM_UPDATE_FLAG) != 0UL)
	{
		if((bRegEQOn & MCB_SYSTEM_EQON) != 0)
		{/*	EQ on	*/
			/*	EQ off	*/
			bReg	= bRegEQOn;
			bReg	&= (UINT8)~MCB_SYSTEM_EQON;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_SYSTEM_EQON, bReg);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_SYSEQ_FLAG_RESET, 0);
		}
		/*	EQ coef	*/
		for(i = 0; i < sizeof(sSysEq.abParam); i++)
		{
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)(MCI_SYS_CEQ0_19_12+i), sSysEq.abParam[i]);
		}
	}

	if((dUpdateInfo & MCDRV_SYSEQ_ONOFF_UPDATE_FLAG) != 0UL)
	{
		bRegEQOn	&= (UINT8)~MCB_SYSTEM_EQON;
		bRegEQOn	|= sSysEq.bOnOff;
	}

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_SYSTEM_EQON, bRegEQOn);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddSysEq", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddClockSwitch
 *
 *	Description:
 *			Add switch clock packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32	McPacket_AddClockSwitch
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_CLKSW_INFO	sClockInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McPacket_AddClockSwitch");
#endif

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_CODEC, MCI_DPADIF);

	McResCtrl_GetClockSwitch(&sClockInfo);

	if((bReg & (MCB_DP0_CLKI1 | MCB_DP0_CLKI0)) != (MCB_DP0_CLKI1 | MCB_DP0_CLKI0))
	{
		if(sClockInfo.bClkSrc == MCDRV_CLKSW_CLKI0)
		{
			if((bReg & MCB_DP0_CLKI1) == 0)
			{/*	CLKI1->CLKI0	*/
				bReg	&= (UINT8)~MCB_DP0_CLKI0;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
				if((bReg & MCB_CLKINPUT) != 0)
				{
					bReg	|= MCB_CLKSRC;
				}
				else
				{
					bReg	&= (UINT8)~MCB_CLKSRC;
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_CLKBUSY_RESET, 0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_CLKSRC_RESET, 0);
				bReg	|= MCB_DP0_CLKI1;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
			}
		}
		else
		{
			if((bReg & MCB_DP0_CLKI0) == 0)
			{/*	CLKI0->CLKI1	*/
				bReg	&= (UINT8)~MCB_DP0_CLKI1;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
				if((bReg & MCB_CLKINPUT) == 0)
				{
					bReg	|= MCB_CLKSRC;
				}
				else
				{
					bReg	&= (UINT8)~MCB_CLKSRC;
				}
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_CLKBUSY_RESET, 0);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_CLKSRC_SET, 0);
				bReg	|= MCB_DP0_CLKI0;
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
			}
		}
	}
	else
	{
		if(sClockInfo.bClkSrc == MCDRV_CLKSW_CLKI0)
		{
			if((bReg & MCB_CLKSRC) != 0)
			{
				bReg	|= MCB_CLKINPUT;
			}
			else
			{
				bReg	&= (UINT8)~MCB_CLKINPUT;
			}
		}
		else
		{
			if((bReg & MCB_CLKSRC) == 0)
			{
				bReg	|= MCB_CLKINPUT;
			}
			else
			{
				bReg	&= (UINT8)~MCB_CLKINPUT;
			}
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | (UINT32)MCI_DPADIF, bReg);
		sdRet	= McDevIf_ExecutePacket();
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McPacket_AddClockSwitch", &sdRet);
#endif

	return sdRet;
}


/****************************************************************************
 *	PowerUpDig
 *
 *	Description:
 *			Digital power up.
 *	Arguments:
 *			bDPBUp		1:DPB power up
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	PowerUpDig
(
	UINT8	bDPBUp
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("PowerUpDig");
#endif

	McResCtrl_GetCurPowerInfo(&sPowerInfo);
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP0;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP1;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP2;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_PLLRST0;
	if(bDPBUp == MCDRV_DPB_UP)
	{
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DPB;
	}
	sPowerUpdate.dDigital	= MCDRV_POWUPDATE_DIGITAL_ALL;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS == sdRet)
	{
		sdRet	= McDevIf_ExecutePacket();
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("PowerUpDig", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	GetMaxWait
 *
 *	Description:
 *			Get maximum wait time.
 *	Arguments:
 *			bRegChange	analog power management register update information
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
	MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("GetMaxWait");
#endif

	McResCtrl_GetInitInfo(&sInitInfo);

	if((bRegChange & MCB_PWM_LI) != 0)
	{
		if(sInitInfo.sWaitTime.dLine1Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dLine1Cin;
		}
	}
	if(((bRegChange & MCB_PWM_MB1) != 0) || ((bRegChange & MCB_PWM_MC1) != 0))
	{
		if(sInitInfo.sWaitTime.dMic1Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic1Cin;
		}
	}
	if(((bRegChange & MCB_PWM_MB2) != 0) || ((bRegChange & MCB_PWM_MC2) != 0))
	{
		if(sInitInfo.sWaitTime.dMic2Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic2Cin;
		}
	}
	if(((bRegChange & MCB_PWM_MB3) != 0) || ((bRegChange & MCB_PWM_MC3) != 0))
	{
		if(sInitInfo.sWaitTime.dMic3Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic3Cin;
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)dWaitTimeMax;
	McDebugLog_FuncOut("GetMaxWait", &sdRet);
#endif

	return dWaitTimeMax;
}

