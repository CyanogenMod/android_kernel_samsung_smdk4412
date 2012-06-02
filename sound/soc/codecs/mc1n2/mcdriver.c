/****************************************************************************
 *
 *		Copyright(c) 2010-2011 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdriver.c
 *
 *		Description	: MC Driver
 *
 *		Version		: 1.0.2		2011.04.18
 *
 ****************************************************************************/


#include "mcdriver_AA.h"
#include "mcdriver.h"
#include "mcservice.h"
#include "mcdevif.h"
#include "mcresctrl.h"
#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcmachdep.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif



#define	MCDRV_MAX_WAIT_TIME	((UINT32)0x0FFFFFFF)

static SINT32	init					(const MCDRV_INIT_INFO* psInitInfo);
static SINT32	term					(void);

static SINT32	read_reg				(MCDRV_REG_INFO* psRegInfo);
static SINT32	write_reg				(const MCDRV_REG_INFO* psRegInfo);

static SINT32	update_clock			(const MCDRV_CLOCK_INFO* psClockInfo);
static SINT32	switch_clock			(const MCDRV_CLKSW_INFO* psClockInfo);

static SINT32	get_path				(MCDRV_PATH_INFO* psPathInfo);
static SINT32	set_path				(const MCDRV_PATH_INFO* psPathInfo);

static SINT32	get_volume				(MCDRV_VOL_INFO* psVolInfo);
static SINT32	set_volume				(const MCDRV_VOL_INFO *psVolInfo);

static SINT32	get_digitalio			(MCDRV_DIO_INFO* psDioInfo);
static SINT32	set_digitalio			(const MCDRV_DIO_INFO* psDioInfo, UINT32 dUpdateInfo);

static SINT32	get_dac					(MCDRV_DAC_INFO* psDacInfo);
static SINT32	set_dac					(const MCDRV_DAC_INFO* psDacInfo, UINT32 dUpdateInfo);

static SINT32	get_adc					(MCDRV_ADC_INFO* psAdcInfo);
static SINT32	set_adc					(const MCDRV_ADC_INFO* psAdcInfo, UINT32 dUpdateInfo);

static SINT32	get_sp					(MCDRV_SP_INFO* psSpInfo);
static SINT32	set_sp					(const MCDRV_SP_INFO* psSpInfo);

static SINT32	get_dng					(MCDRV_DNG_INFO* psDngInfo);
static SINT32	set_dng					(const MCDRV_DNG_INFO* psDngInfo, UINT32 dUpdateInfo);

static SINT32	set_ae					(const MCDRV_AE_INFO* psAeInfo, UINT32 dUpdateInfo);

static SINT32	get_pdm					(MCDRV_PDM_INFO* psPdmInfo);
static SINT32	set_pdm					(const MCDRV_PDM_INFO* psPdmInfo, UINT32 dUpdateInfo);

static SINT32	config_gp				(const MCDRV_GP_MODE* psGpMode);
static SINT32	mask_gp					(UINT8* pbMask, UINT32 dPadNo);
static SINT32	getset_gp				(UINT8* pbGpio, UINT32 dPadNo);

static SINT32	get_syseq				(MCDRV_SYSEQ_INFO* psSysEq);
static SINT32	set_syseq				(const MCDRV_SYSEQ_INFO* psSysEq, UINT32 dUpdateInfo);

static SINT32	ValidateInitParam		(const MCDRV_INIT_INFO* psInitInfo);
static SINT32	ValidateClockParam		(const MCDRV_CLOCK_INFO* psClockInfo);
static SINT32	ValidateReadRegParam	(const MCDRV_REG_INFO* psRegInfo);
static SINT32	ValidateWriteRegParam	(const MCDRV_REG_INFO* psRegInfo);
static SINT32	ValidatePathParam		(const MCDRV_PATH_INFO* psPathInfo);
static SINT32	ValidateDioParam		(const MCDRV_DIO_INFO* psDioInfo, UINT32 dUpdateInfo);
static SINT32	CheckDIOCommon			(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort);
static SINT32	CheckDIODIR				(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort, UINT8 bInterface);
static SINT32	CheckDIODIT				(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort, UINT8 bInterface);

static SINT32	SetVol					(UINT32 dUpdate, MCDRV_VOLUPDATE_MODE eMode, UINT32* pdSVolDoneParam);
static	SINT32	PreUpdatePath			(UINT16* pwDACMuteParam, UINT16* pwDITMuteParam);


/****************************************************************************
 *	init
 *
 *	Description:
 *			Initialize.
 *	Arguments:
 *			psInitInfo	initialize information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	init
(
	const MCDRV_INIT_INFO	*psInitInfo
)
{
	SINT32	sdRet;
	UINT8	bHWID;

	if(NULL == psInitInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT != McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McSrv_SystemInit();
	McSrv_Lock();

	bHWID	= McSrv_ReadI2C(McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG), (UINT32)MCI_HW_ID);
	sdRet	= McResCtrl_SetHwId(bHWID);
	if(sdRet != MCDRV_SUCCESS)
	{
		McSrv_Unlock();
		McSrv_SystemTerm();
		return sdRet;
	}

	if(bHWID == MCDRV_HWID_AA)
	{
		McSrv_Unlock();
		McSrv_SystemTerm();
		return McDrv_Ctrl_AA(MCDRV_INIT, (MCDRV_INIT_INFO*)psInitInfo, 0);
	}
	else
	{
		sdRet	= ValidateInitParam(psInitInfo);

		if(sdRet == MCDRV_SUCCESS)
		{
			McResCtrl_Init(psInitInfo);
			sdRet	= McDevIf_AllocPacketBuf();
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			sdRet	= McPacket_AddInit(psInitInfo);
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			sdRet	= McDevIf_ExecutePacket();
		}

		if(sdRet == MCDRV_SUCCESS)
		{
			McResCtrl_UpdateState(eMCDRV_STATE_READY);
		}
		else
		{
			McDevIf_ReleasePacketBuf();
		}

		McSrv_Unlock();

		if(sdRet != MCDRV_SUCCESS)
		{
			McSrv_SystemTerm();
		}
	}

	return sdRet;
}

/****************************************************************************
 *	term
 *
 *	Description:
 *			Terminate.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	term
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bCh, bSrcIdx;
	UINT8	abOnOff[SOURCE_BLOCK_NUM];
	MCDRV_PATH_INFO		sPathInfo;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

	if(eMCDRV_STATE_READY != McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McSrv_Lock();

	abOnOff[0]	= (MCDRV_SRC0_MIC1_OFF|MCDRV_SRC0_MIC2_OFF|MCDRV_SRC0_MIC3_OFF);
	abOnOff[1]	= (MCDRV_SRC1_LINE1_L_OFF|MCDRV_SRC1_LINE1_R_OFF|MCDRV_SRC1_LINE1_M_OFF);
	abOnOff[2]	= (MCDRV_SRC2_LINE2_L_OFF|MCDRV_SRC2_LINE2_R_OFF|MCDRV_SRC2_LINE2_M_OFF);
	abOnOff[3]	= (MCDRV_SRC3_DIR0_OFF|MCDRV_SRC3_DIR1_OFF|MCDRV_SRC3_DIR2_OFF|MCDRV_SRC3_DIR2_DIRECT_OFF);
	abOnOff[4]	= (MCDRV_SRC4_DTMF_OFF|MCDRV_SRC4_PDM_OFF|MCDRV_SRC4_ADC0_OFF|MCDRV_SRC4_ADC1_OFF);
	abOnOff[5]	= (MCDRV_SRC5_DAC_L_OFF|MCDRV_SRC5_DAC_R_OFF|MCDRV_SRC5_DAC_M_OFF);
	abOnOff[6]	= (MCDRV_SRC6_MIX_OFF|MCDRV_SRC6_AE_OFF|MCDRV_SRC6_CDSP_OFF|MCDRV_SRC6_CDSP_DIRECT_OFF);

	for(bSrcIdx = 0; bSrcIdx < SOURCE_BLOCK_NUM; bSrcIdx++)
	{
		for(bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asHpOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asSpOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asRcOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asLout1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asLout2[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < PEAK_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asPeak[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT0_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit0[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT2_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit2[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDac[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < AE_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAe[bCh].abSrcOnOff[bSrcIdx]		= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < CDSP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asCdsp[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAdc0[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAdc1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < MIX_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asMix[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asBias[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
	}
	sdRet	= set_path(&sPathInfo);
	if(sdRet == MCDRV_SUCCESS)
	{
		sPowerInfo.dDigital			= (MCDRV_POWINFO_DIGITAL_DP0
									  |MCDRV_POWINFO_DIGITAL_DP1
									  |MCDRV_POWINFO_DIGITAL_DP2
									  |MCDRV_POWINFO_DIGITAL_DPB
									  |MCDRV_POWINFO_DIGITAL_DPDI0
									  |MCDRV_POWINFO_DIGITAL_DPDI1
									  |MCDRV_POWINFO_DIGITAL_DPDI2
									  |MCDRV_POWINFO_DIGITAL_DPPDM
									  |MCDRV_POWINFO_DIGITAL_DPBDSP
									  |MCDRV_POWINFO_DIGITAL_DPADIF
									  |MCDRV_POWINFO_DIGITAL_PLLRST0);
		sPowerInfo.abAnalog[0]		=
		sPowerInfo.abAnalog[1]		=
		sPowerInfo.abAnalog[2]		=
		sPowerInfo.abAnalog[3]		=
		sPowerInfo.abAnalog[4]		= (UINT8)0xFF;
		sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
		sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
		sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
		sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
		sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
		sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
		sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= McDevIf_ExecutePacket();
	}

	McDevIf_ReleasePacketBuf();

	McResCtrl_UpdateState(eMCDRV_STATE_NOTINIT);

	McSrv_Unlock();

	McSrv_SystemTerm();

	return sdRet;
}

/****************************************************************************
 *	read_reg
 *
 *	Description:
 *			read register.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	read_reg
(
	MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bSlaveAddr;
	UINT8	bAddr;
	UINT8	abData[2];
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_INFO	sCurPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

	if(NULL == psRegInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateReadRegParam(psRegInfo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess(psRegInfo, &sPowerInfo);
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	bAddr	= psRegInfo->bAddress;

	if(psRegInfo->bRegType == MCDRV_REGTYPE_A)
	{
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
	}
	else
	{
		switch(psRegInfo->bRegType)
		{
		case	MCDRV_REGTYPE_B_BASE:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_BASE_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_BASE_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_MIXER:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_MIX_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_MIX_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_AE:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AE_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AE_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_CDSP:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_CDSP_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_CDSP_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_CODEC:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_CD_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_CD_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_ANALOG:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
			abData[0]	= MCI_ANA_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_ANA_WINDOW;
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	read register	*/
	psRegInfo->bData	= McSrv_ReadI2C(bSlaveAddr, bAddr);

	/*	restore power	*/
	sdRet	= McPacket_AddPowerDown(&sCurPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	write_reg
 *
 *	Description:
 *			Write register.
 *	Arguments:
 *			psWR	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static	SINT32	write_reg
(
	const MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_INFO	sCurPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

	if(NULL == psRegInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateWriteRegParam(psRegInfo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess(psRegInfo, &sPowerInfo);
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	switch(psRegInfo->bRegType)
	{
	case	MCDRV_REGTYPE_A:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_A | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_BASE:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_BASE | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_MIXER:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_AE:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_AE | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_CDSP:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_CDSP | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_CODEC:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_CODEC | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_ANALOG:
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE | MCDRV_PACKET_REGTYPE_B_ANA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	restore power	*/
	if(psRegInfo->bRegType == MCDRV_REGTYPE_B_BASE)
	{
		if(psRegInfo->bAddress == MCI_PWM_DIGITAL)
		{
			if((psRegInfo->bData & MCB_PWM_DP1) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP1;
			}
			if((psRegInfo->bData & MCB_PWM_DP2) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP2;
			}
		}
		else if(psRegInfo->bAddress == MCI_PWM_DIGITAL_1)
		{
			if((psRegInfo->bData & MCB_PWM_DPB) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPB;
			}
			if((psRegInfo->bData & MCB_PWM_DPDI0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI0;
			}
			if((psRegInfo->bData & MCB_PWM_DPDI1) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI1;
			}
			if((psRegInfo->bData & MCB_PWM_DPDI2) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI2;
			}
			if((psRegInfo->bData & MCB_PWM_DPPDM) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPPDM;
			}
		}
		else if(psRegInfo->bAddress == MCI_PWM_DIGITAL_BDSP)
		{
			if((psRegInfo->bData & MCB_PWM_DPBDSP) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPBDSP;
			}
		}
		else
		{
		}
	}
	else if(psRegInfo->bRegType == MCDRV_REGTYPE_B_CODEC)
	{
		if(psRegInfo->bAddress == MCI_DPADIF)
		{
			if((psRegInfo->bData & MCB_DPADIF) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPADIF;
			}
			if((psRegInfo->bData & MCB_DP0_CLKI0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP0;
			}
			if((psRegInfo->bData & MCB_DP0_CLKI1) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP0;
			}
		}
		if(psRegInfo->bAddress == MCI_PLL_RST)
		{
			if((psRegInfo->bData & MCB_PLLRST0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_PLLRST0;
			}
		}
	}
	else if(psRegInfo->bRegType == MCDRV_REGTYPE_B_ANALOG)
	{
		if(psRegInfo->bAddress == MCI_PWM_ANALOG_0)
		{
			sCurPowerInfo.abAnalog[0]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_PWM_ANALOG_1)
		{
			sCurPowerInfo.abAnalog[1]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_PWM_ANALOG_2)
		{
			sCurPowerInfo.abAnalog[2]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_PWM_ANALOG_3)
		{
			sCurPowerInfo.abAnalog[3]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_PWM_ANALOG_4)
		{
			sCurPowerInfo.abAnalog[4]	= psRegInfo->bData;
		}
		else
		{
		}
	}
	else
	{
	}

	sdRet	= McPacket_AddPowerDown(&sCurPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	update_clock
 *
 *	Description:
 *			Update clock info.
 *	Arguments:
 *			psClockInfo	clock info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	update_clock
(
	const MCDRV_CLOCK_INFO*	psClockInfo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE		eState	= McResCtrl_GetState();
	MCDRV_INIT_INFO	sInitInfo;

	if(NULL == psClockInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo(&sInitInfo);
	if((sInitInfo.bPowerMode & MCDRV_POWMODE_CLKON) != 0)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	sdRet	= ValidateClockParam(psClockInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetClockInfo(psClockInfo);
	return sdRet;
}

/****************************************************************************
 *	switch_clock
 *
 *	Description:
 *			Switch clock.
 *	Arguments:
 *			psClockInfo	clock info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	switch_clock
(
	const MCDRV_CLKSW_INFO*	psClockInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	if(NULL == psClockInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if((psClockInfo->bClkSrc != MCDRV_CLKSW_CLKI0) && (psClockInfo->bClkSrc != MCDRV_CLKSW_CLKI1))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	McResCtrl_SetClockSwitch(psClockInfo);

	sdRet	= McPacket_AddClockSwitch();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_path
 *
 *	Description:
 *			Get current path setting.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_path
(
	MCDRV_PATH_INFO*	psPathInfo
)
{
	if(NULL == psPathInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetPathInfoVirtual(psPathInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_path
 *
 *	Description:
 *			Set path.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_path
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	UINT32			dSVolDoneParam	= 0;
	UINT16			wDACMuteParam	= 0;
	UINT16			wDITMuteParam	= 0;
	MCDRV_STATE		eState	= McResCtrl_GetState();
	MCDRV_POWER_INFO	sPowerInfo;
	MCDRV_POWER_UPDATE	sPowerUpdate;

	if(NULL == psPathInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidatePathParam(psPathInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetPathInfo(psPathInfo);

	/*	unused analog out volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ANAOUT_ALL, eMCDRV_VOLUPDATE_MUTE, &dSVolDoneParam);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	DAC/DIT* mute	*/
	sdRet	= PreUpdatePath(&wDACMuteParam, &wDITMuteParam);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	unused volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ALL&~MCDRV_VOLUPDATE_ANAOUT_ALL, eMCDRV_VOLUPDATE_MUTE, NULL);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	wait XX_BUSY	*/
	if(dSVolDoneParam != (UINT32)0)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_SVOL_DONE | dSVolDoneParam, 0);
	}
	if(wDACMuteParam != 0)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_DACMUTE | wDACMuteParam, 0);
	}
	if(wDITMuteParam != 0)
	{
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_EVTWAIT | MCDRV_EVT_DITMUTE | wDITMuteParam, 0);
	}

	/*	stop unused path	*/
	sdRet	= McPacket_AddStop();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_GetPowerInfo(&sPowerInfo);

	/*	used path power up	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set digital mixer	*/
	sdRet	= McPacket_AddPathSet();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set analog mixer	*/
	sdRet	= McPacket_AddMixSet();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL;
	sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	start	*/
	sdRet	= McPacket_AddStart();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set volume	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ALL&~MCDRV_VOLUPDATE_ANAOUT_ALL, eMCDRV_VOLUPDATE_ALL, NULL);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return SetVol(MCDRV_VOLUPDATE_ANAOUT_ALL, eMCDRV_VOLUPDATE_ALL, &dSVolDoneParam);
}

/****************************************************************************
 *	get_volume
 *
 *	Description:
 *			Get current volume setting.
 *	Arguments:
 *			psVolInfo	volume information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	get_volume
(
	MCDRV_VOL_INFO*	psVolInfo
)
{
	if(NULL == psVolInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetVolInfo(psVolInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_volume
 *
 *	Description:
 *			Set volume.
 *	Arguments:
 *			psVolInfo	volume update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	set_volume
(
	const MCDRV_VOL_INFO*	psVolInfo
)
{
	MCDRV_STATE	eState	= McResCtrl_GetState();
	MCDRV_PATH_INFO	sPathInfo;

	if(NULL == psVolInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetVolInfo(psVolInfo);

	McResCtrl_GetPathInfoVirtual(&sPathInfo);
	return	set_path(&sPathInfo);
}

/****************************************************************************
 *	get_digitalio
 *
 *	Description:
 *			Get current digital IO setting.
 *	Arguments:
 *			psDioInfo	digital IO information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_digitalio
(
	MCDRV_DIO_INFO*	psDioInfo
)
{
	if(NULL == psDioInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDioInfo(psDioInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_digitalio
 *
 *	Description:
 *			Update digital IO configuration.
 *	Arguments:
 *			psDioInfo	digital IO configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_digitalio
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psDioInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateDioParam(psDioInfo, dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetDioInfo(psDioInfo, dUpdateInfo);

	sdRet	= McPacket_AddDigitalIO(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_dac
 *
 *	Description:
 *			Get current DAC setting.
 *	Arguments:
 *			psDacInfo	DAC information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_dac
(
	MCDRV_DAC_INFO*	psDacInfo
)
{
	if(NULL == psDacInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDacInfo(psDacInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_dac
 *
 *	Description:
 *			Update DAC configuration.
 *	Arguments:
 *			psDacInfo	DAC configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_dac
(
	const MCDRV_DAC_INFO*	psDacInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psDacInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetDacInfo(psDacInfo, dUpdateInfo);

	sdRet	= McPacket_AddDAC(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_adc
 *
 *	Description:
 *			Get current ADC setting.
 *	Arguments:
 *			psAdcInfo	ADC information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_adc
(
	MCDRV_ADC_INFO*	psAdcInfo
)
{
	if(NULL == psAdcInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetAdcInfo(psAdcInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_adc
 *
 *	Description:
 *			Update ADC configuration.
 *	Arguments:
 *			psAdcInfo	ADC configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_adc
(
	const MCDRV_ADC_INFO*	psAdcInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psAdcInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetAdcInfo(psAdcInfo, dUpdateInfo);

	sdRet	= McPacket_AddADC(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_sp
 *
 *	Description:
 *			Get current SP setting.
 *	Arguments:
 *			psSpInfo	SP information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_sp
(
	MCDRV_SP_INFO*	psSpInfo
)
{
	if(NULL == psSpInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetSpInfo(psSpInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_sp
 *
 *	Description:
 *			Update SP configuration.
 *	Arguments:
 *			psSpInfo	SP configuration
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_sp
(
	const MCDRV_SP_INFO*	psSpInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psSpInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetSpInfo(psSpInfo);

	sdRet	= McPacket_AddSP();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_dng
 *
 *	Description:
 *			Get current Digital Noise Gate setting.
 *	Arguments:
 *			psDngInfo	DNG information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_dng
(
	MCDRV_DNG_INFO*	psDngInfo
)
{
	if(NULL == psDngInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDngInfo(psDngInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_dng
 *
 *	Description:
 *			Update Digital Noise Gate configuration.
 *	Arguments:
 *			psDngInfo	DNG configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_dng
(
	const MCDRV_DNG_INFO*	psDngInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psDngInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetDngInfo(psDngInfo, dUpdateInfo);

	sdRet	= McPacket_AddDNG(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	set_ae
 *
 *	Description:
 *			Update Audio Engine configuration.
 *	Arguments:
 *			psAeInfo	AE configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_ae
(
	const MCDRV_AE_INFO*	psAeInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();
	MCDRV_PATH_INFO	sPathInfo;

	if(NULL == psAeInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetAeInfo(psAeInfo, dUpdateInfo);

	sdRet	= McPacket_AddAE(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_GetPathInfoVirtual(&sPathInfo);
	return	set_path(&sPathInfo);
}

/****************************************************************************
 *	get_pdm
 *
 *	Description:
 *			Get current PDM setting.
 *	Arguments:
 *			psPdmInfo	PDM information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_pdm
(
	MCDRV_PDM_INFO*	psPdmInfo
)
{
	if(NULL == psPdmInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == McResCtrl_GetState())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetPdmInfo(psPdmInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_pdm
 *
 *	Description:
 *			Update PDM configuration.
 *	Arguments:
 *			psPdmInfo	PDM configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_pdm
(
	const MCDRV_PDM_INFO*	psPdmInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psPdmInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetPdmInfo(psPdmInfo, dUpdateInfo);

	sdRet	= McPacket_AddPDM(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	config_gp
 *
 *	Description:
 *			Set GPIO mode.
 *	Arguments:
 *			psGpMode	GPIO mode information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
*
 ****************************************************************************/
static	SINT32	config_gp
(
	const MCDRV_GP_MODE*	psGpMode
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(NULL == psGpMode)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetGPMode(psGpMode);

	sdRet	= McPacket_AddGPMode();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	mask_gp
 *
 *	Description:
 *			Set GPIO input mask.
 *	Arguments:
 *			pbMask	mask setting
 *			dPadNo	PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR
*
 ****************************************************************************/
static	SINT32	mask_gp
(
	UINT8*	pbMask,
	UINT32	dPadNo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

	if(NULL == pbMask)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetGPMode(&sGPMode);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if((sInitInfo.bPad0Func == MCDRV_PAD_GPIO) && (sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT))
		{
			return MCDRV_ERROR;
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if((sInitInfo.bPad1Func == MCDRV_PAD_GPIO) && (sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT))
		{
			return MCDRV_ERROR;
		}
	}
	else
	{
		return MCDRV_ERROR;
	}

	McResCtrl_SetGPMask(*pbMask, dPadNo);

	sdRet	= McPacket_AddGPMask(dPadNo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	getset_gp
 *
 *	Description:
 *			Set or get state of GPIO pin.
 *	Arguments:
 *			pbGpio	pin state
 *			dPadNo	PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static	SINT32	getset_gp
(
	UINT8*	pbGpio,
	UINT32	dPadNo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	UINT8			bSlaveAddr;
	UINT8			abData[2];
	UINT8			bRegData;
	MCDRV_STATE		eState	= McResCtrl_GetState();
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

	if(NULL == pbGpio)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo(&sInitInfo);
	McResCtrl_GetGPMode(&sGPMode);

	if(((dPadNo == MCDRV_GP_PAD0) && (sInitInfo.bPad0Func != MCDRV_PAD_GPIO))
	|| ((dPadNo == MCDRV_GP_PAD1) && (sInitInfo.bPad1Func != MCDRV_PAD_GPIO)))
	{
		return MCDRV_ERROR;
	}

	if(sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN)
	{
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abData[0]	= MCI_BASE_ADR<<1;
		abData[1]	= MCI_PA_SCU_PA;
		McSrv_WriteI2C(bSlaveAddr, abData, 2);
		bRegData	= McSrv_ReadI2C(bSlaveAddr, MCI_BASE_WINDOW);
		if(dPadNo == MCDRV_GP_PAD0)
		{
			*pbGpio	= bRegData & MCB_PA_SCU_PA0;
		}
		else if(dPadNo == MCDRV_GP_PAD1)
		{
			*pbGpio	= (bRegData & MCB_PA_SCU_PA1) >> 1;
		}
		else
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	else
	{
		sdRet	= McPacket_AddGPSet(*pbGpio, dPadNo);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		return McDevIf_ExecutePacket();
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	get_syseq
 *
 *	Description:
 *			Get System Eq.
 *	Arguments:
 *			psSysEq	pointer to MCDRV_SYSEQ_INFO struct
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static	SINT32	get_syseq
(
	MCDRV_SYSEQ_INFO*	psSysEq
)
{
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	if(NULL == psSysEq)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	McResCtrl_GetSysEq(psSysEq);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_syseq
 *
 *	Description:
 *			Set System Eq.
 *	Arguments:
 *			psSysEq		pointer to MCDRV_SYSEQ_INFO struct
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static	SINT32	set_syseq
(
	const MCDRV_SYSEQ_INFO*	psSysEq,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE	eState	= McResCtrl_GetState();

	if(eMCDRV_STATE_NOTINIT == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	if(NULL == psSysEq)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	McResCtrl_SetSysEq(psSysEq, dUpdateInfo);
	sdRet	= McPacket_AddSysEq(dUpdateInfo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket();
}


/****************************************************************************
 *	ValidateInitParam
 *
 *	Description:
 *			validate init parameters.
 *	Arguments:
 *			psInitInfo	initialize information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateInitParam
(
	const MCDRV_INIT_INFO*	psInitInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidateInitParam");
#endif


	if((MCDRV_CKSEL_CMOS != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_CMOS_TCXO != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO_CMOS != psInitInfo->bCkSel))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psInitInfo->bDivR0 == 0x00) || (psInitInfo->bDivR0 > 0x3F))
	|| ((psInitInfo->bDivR1 == 0x00) || (psInitInfo->bDivR1 > 0x3F))
	|| ((psInitInfo->bDivF0 == 0x00) || (psInitInfo->bDivF0 > 0x7F))
	|| ((psInitInfo->bDivF1 == 0x00) || (psInitInfo->bDivF1 > 0x7F)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1) && (((psInitInfo->bRange0 > 0x07) || (psInitInfo->bRange1 > 0x07))))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1) && (psInitInfo->bBypass > 0x03))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo0Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo0Hiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo1Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo1Hiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo2Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo2Hiz)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_PCMHIZ_HIZ != psInitInfo->bPcmHiz) && (MCDRV_PCMHIZ_LOW != psInitInfo->bPcmHiz))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((MCDRV_DAHIZ_LOW != psInitInfo->bDioClk0Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk0Hiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDioClk1Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk1Hiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDioClk2Hiz) && (MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk2Hiz)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_PCMHIZ_HIZ != psInitInfo->bPcmHiz) && (MCDRV_PCMHIZ_LOW != psInitInfo->bPcmHiz))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_LINE_STEREO != psInitInfo->bLineIn1Dif) && (MCDRV_LINE_DIF != psInitInfo->bLineIn1Dif))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1) && (MCDRV_LINE_STEREO != psInitInfo->bLineIn2Dif) && (MCDRV_LINE_DIF != psInitInfo->bLineIn2Dif))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((MCDRV_LINE_STEREO != psInitInfo->bLineOut1Dif) && (MCDRV_LINE_DIF != psInitInfo->bLineOut1Dif))
	|| ((MCDRV_LINE_STEREO != psInitInfo->bLineOut2Dif) && (MCDRV_LINE_DIF != psInitInfo->bLineOut2Dif)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_SPMN_ON != psInitInfo->bSpmn) && (MCDRV_SPMN_OFF != psInitInfo->bSpmn))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((MCDRV_MIC_DIF != psInitInfo->bMic1Sng) && (MCDRV_MIC_SINGLE != psInitInfo->bMic1Sng))
	|| ((MCDRV_MIC_DIF != psInitInfo->bMic2Sng) && (MCDRV_MIC_SINGLE != psInitInfo->bMic2Sng))
	|| ((MCDRV_MIC_DIF != psInitInfo->bMic3Sng) && (MCDRV_MIC_SINGLE != psInitInfo->bMic3Sng)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_POWMODE_NORMAL != psInitInfo->bPowerMode)
	&& (MCDRV_POWMODE_CLKON != psInitInfo->bPowerMode)
	&& (MCDRV_POWMODE_VREFON != psInitInfo->bPowerMode)
	&& (MCDRV_POWMODE_CLKVREFON != psInitInfo->bPowerMode)
	&& (MCDRV_POWMODE_FULL != psInitInfo->bPowerMode))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_SPHIZ_PULLDOWN != psInitInfo->bSpHiz) && (MCDRV_SPHIZ_HIZ != psInitInfo->bSpHiz))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_LDO_OFF != psInitInfo->bLdo) && (MCDRV_LDO_ON != psInitInfo->bLdo))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_PAD_GPIO != psInitInfo->bPad0Func) && (MCDRV_PAD_PDMCK != psInitInfo->bPad0Func) && (MCDRV_PAD_IRQ != psInitInfo->bPad0Func))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_IRQ) != 1) && (MCDRV_PAD_IRQ == psInitInfo->bPad0Func))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_PAD_GPIO != psInitInfo->bPad1Func) && (MCDRV_PAD_PDMDI != psInitInfo->bPad1Func) && (MCDRV_PAD_IRQ != psInitInfo->bPad1Func))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_IRQ) != 1) && (MCDRV_PAD_IRQ == psInitInfo->bPad1Func))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_PAD2) == 1)
	&& (MCDRV_PAD_GPIO != psInitInfo->bPad2Func)
	&& (MCDRV_PAD_PDMDI != psInitInfo->bPad2Func)
	&& (MCDRV_PAD_IRQ != psInitInfo->bPad2Func))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_OUTLEV_0 != psInitInfo->bAvddLev) && (MCDRV_OUTLEV_1 != psInitInfo->bAvddLev)
	&& (MCDRV_OUTLEV_2 != psInitInfo->bAvddLev) && (MCDRV_OUTLEV_3 != psInitInfo->bAvddLev)
	&& (MCDRV_OUTLEV_4 != psInitInfo->bAvddLev) && (MCDRV_OUTLEV_5 != psInitInfo->bAvddLev)
	&& (MCDRV_OUTLEV_6 != psInitInfo->bAvddLev) && (MCDRV_OUTLEV_7 != psInitInfo->bAvddLev))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_OUTLEV_0 != psInitInfo->bVrefLev) && (MCDRV_OUTLEV_1 != psInitInfo->bVrefLev)
	&& (MCDRV_OUTLEV_2 != psInitInfo->bVrefLev) && (MCDRV_OUTLEV_3 != psInitInfo->bVrefLev)
	&& (MCDRV_OUTLEV_4 != psInitInfo->bVrefLev) && (MCDRV_OUTLEV_5 != psInitInfo->bVrefLev)
	&& (MCDRV_OUTLEV_6 != psInitInfo->bVrefLev) && (MCDRV_OUTLEV_7 != psInitInfo->bVrefLev))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_DCLGAIN_0 != psInitInfo->bDclGain) && (MCDRV_DCLGAIN_6 != psInitInfo->bDclGain)
	&& (MCDRV_DCLGAIN_12!= psInitInfo->bDclGain) && (MCDRV_DCLGAIN_18!= psInitInfo->bDclGain))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_DCLLIMIT_0 != psInitInfo->bDclLimit) && (MCDRV_DCLLIMIT_116 != psInitInfo->bDclLimit)
	&& (MCDRV_DCLLIMIT_250 != psInitInfo->bDclLimit) && (MCDRV_DCLLIMIT_602 != psInitInfo->bDclLimit))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_CPMOD_ON != psInitInfo->bCpMod) && (MCDRV_CPMOD_OFF != psInitInfo->bCpMod))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if((MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dAdHpf)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dMic1Cin)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dMic2Cin)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dMic3Cin)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dLine1Cin)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dLine2Cin)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dVrefRdy1)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dVrefRdy2)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dHpRdy)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dSpRdy)
	|| (MCDRV_MAX_WAIT_TIME < psInitInfo->sWaitTime.dPdm))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidateInitParam", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	ValidateClockParam
 *
 *	Description:
 *			validate clock parameters.
 *	Arguments:
 *			psClockInfo	clock information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateClockParam
(
	const MCDRV_CLOCK_INFO*	psClockInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidateClockParam");
#endif


	if((MCDRV_CKSEL_CMOS != psClockInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO != psClockInfo->bCkSel)
	&& (MCDRV_CKSEL_CMOS_TCXO != psClockInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO_CMOS != psClockInfo->bCkSel))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((psClockInfo->bDivR0 == 0x00) || (psClockInfo->bDivR0 > 0x3F))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((psClockInfo->bDivR1 == 0x00) || (psClockInfo->bDivR1 > 0x3F))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((psClockInfo->bDivF0 == 0x00) || (psClockInfo->bDivF0 > 0x7F))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((psClockInfo->bDivF1 == 0x00) || (psClockInfo->bDivF1 > 0x7F))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1)
	&& ((psClockInfo->bRange0 > 0x07) || (psClockInfo->bRange1 > 0x07)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else if((McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1) && (psClockInfo->bBypass > 0x03))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	else
	{
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidateClockParam", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	ValidateReadRegParam
 *
 *	Description:
 *			validate read reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateReadRegParam
(
	const MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidateReadRegParam");
#endif


	if((McResCtrl_GetRegAccess(psRegInfo) & eMCDRV_READ_ONLY) == 0)
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidateReadRegParam", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	ValidateWriteRegParam
 *
 *	Description:
 *			validate write reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateWriteRegParam
(
	const MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidateWriteRegParam");
#endif


	if((McResCtrl_GetRegAccess(psRegInfo) & eMCDRV_WRITE_ONLY) == 0)
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidateWriteRegParam", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	ValidatePathParam
 *
 *	Description:
 *			validate path parameters.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32	ValidatePathParam
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bBlock;
	UINT8	bCh;
	UINT8	bPDMIsSrc	= 0;
	UINT8	bADC0IsSrc	= 0;
	MCDRV_PATH_INFO		sCurPathInfo;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidatePathParam");
#endif


	McResCtrl_GetPathInfoVirtual(&sCurPathInfo);
	/*	set off to current path info	*/
	for(bBlock = 0; bBlock < SOURCE_BLOCK_NUM; bBlock++)
	{
		for(bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT0_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT1_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT2_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < AE_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < MIX_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
	}

	if(((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	|| ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON))
	{
		bPDMIsSrc	= 1;
	}
	if(((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	|| ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON))
	{
		bADC0IsSrc	= 1;
	}

	/*	HP	*/
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	SP	*/
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		if(((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		if(((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	RCV	*/

	/*	LOUT1	*/
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	LOUT2	*/
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	PeakMeter	*/

	/*	DIT0	*/
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if(((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if(((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if(((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DIT1	*/
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if(((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if(((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if(((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DIT2	*/
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if(((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if(((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if(((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DAC	*/
	for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
	{
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			if(((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			if(((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			if(((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			if(((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			if(((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
			|| ((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			if(bADC0IsSrc == 1)
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
			else
			{
				bPDMIsSrc	= 1;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			if(bPDMIsSrc == 1)
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
			else
			{
				bADC0IsSrc	= 1;
			}
		}
	}

	/*	AE	*/
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		|| ((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	&& (((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		|| ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

	/*	CDSP	*/
	for(bCh = 0; bCh < 4; bCh++)
	{
	}

	/*	ADC0	*/
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		if(((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	ADC1	*/

	/*	MIX	*/
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		|| ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 || ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	 ))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 || ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	 ))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 || ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	 ))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	 || ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	 || ((sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	&& (((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 || ((sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	 ))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidatePathParam", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	ValidateDioParam
 *
 *	Description:
 *			validate digital IO parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	ValidateDioParam
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT32					dUpdateInfo
)
{
	SINT32				sdRet	= MCDRV_SUCCESS;
	UINT8				bPort;
	MCDRV_SRC_TYPE		aeDITSource[IOPORT_NUM];
	MCDRV_SRC_TYPE		eAESource;
	MCDRV_SRC_TYPE		eDAC0Source;
	MCDRV_SRC_TYPE		eDAC1Source;
	UINT8				bDIRUsed[IOPORT_NUM];
	MCDRV_DIO_INFO		sDioInfo;
	MCDRV_DIO_PORT_NO	aePort[IOPORT_NUM]	= {eMCDRV_DIO_0, eMCDRV_DIO_1, eMCDRV_DIO_2};

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("ValidateDioParam");
#endif

	eAESource	= McResCtrl_GetAESource();
	eDAC0Source	= McResCtrl_GetDACSource(eMCDRV_DAC_MASTER);
	eDAC1Source	= McResCtrl_GetDACSource(eMCDRV_DAC_VOICE);


	McResCtrl_GetDioInfo(&sDioInfo);

	for(bPort = 0; bPort < IOPORT_NUM; bPort++)
	{
		aeDITSource[bPort]	= McResCtrl_GetDITSource(aePort[bPort]);
		bDIRUsed[bPort]		= 0;
	}

	if(((eAESource == eMCDRV_SRC_DIR0) || (eDAC0Source == eMCDRV_SRC_DIR0) || (eDAC1Source == eMCDRV_SRC_DIR0))
	|| ((aeDITSource[0] == eMCDRV_SRC_DIR0) || (aeDITSource[1] == eMCDRV_SRC_DIR0) || (aeDITSource[2] == eMCDRV_SRC_DIR0)))
	{
		bDIRUsed[0]	= 1;
	}
	if(((eAESource == eMCDRV_SRC_DIR1) || (eDAC0Source == eMCDRV_SRC_DIR1) || (eDAC1Source == eMCDRV_SRC_DIR1))
	|| ((aeDITSource[0] == eMCDRV_SRC_DIR1) || (aeDITSource[1] == eMCDRV_SRC_DIR1) || (aeDITSource[2] == eMCDRV_SRC_DIR1)))
	{
		bDIRUsed[1]	= 1;
	}
	if(((eAESource == eMCDRV_SRC_DIR2) || (eDAC0Source == eMCDRV_SRC_DIR2) || (eDAC1Source == eMCDRV_SRC_DIR2))
	|| ((aeDITSource[0] == eMCDRV_SRC_DIR2) || (aeDITSource[1] == eMCDRV_SRC_DIR2) || (aeDITSource[2] == eMCDRV_SRC_DIR2)))
	{
		bDIRUsed[2]	= 1;
	}

	if(((bDIRUsed[0] == 1) && (((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != 0UL)))
	|| ((aeDITSource[0] != eMCDRV_SRC_NONE) && (((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != 0UL))))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((bDIRUsed[1] == 1) && (((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != 0UL)))
	|| ((aeDITSource[1] != eMCDRV_SRC_NONE) && (((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != 0UL))))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}
	if(((bDIRUsed[2] == 1) && (((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != 0UL)))
	|| ((aeDITSource[2] != eMCDRV_SRC_NONE) && (((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0UL) || ((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != 0UL))))
	{
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIOCommon(psDioInfo, 0);
			if(sdRet == MCDRV_SUCCESS)
			{
				sDioInfo.asPortInfo[0].sDioCommon.bInterface	= psDioInfo->asPortInfo[0].sDioCommon.bInterface;
			}
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIOCommon(psDioInfo, 1);
			if(sdRet == MCDRV_SUCCESS)
			{
				sDioInfo.asPortInfo[1].sDioCommon.bInterface	= psDioInfo->asPortInfo[1].sDioCommon.bInterface;
			}
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIOCommon(psDioInfo, 2);
			if(sdRet == MCDRV_SUCCESS)
			{
				sDioInfo.asPortInfo[2].sDioCommon.bInterface	= psDioInfo->asPortInfo[2].sDioCommon.bInterface;
			}
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIR(psDioInfo, 0, sDioInfo.asPortInfo[0].sDioCommon.bInterface);
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIR(psDioInfo, 1, sDioInfo.asPortInfo[1].sDioCommon.bInterface);
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIR(psDioInfo, 2, sDioInfo.asPortInfo[2].sDioCommon.bInterface);
		}
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIT(psDioInfo, 0, sDioInfo.asPortInfo[0].sDioCommon.bInterface);
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIT(psDioInfo, 1, sDioInfo.asPortInfo[1].sDioCommon.bInterface);
		}
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != 0UL)
		{
			sdRet	= CheckDIODIT(psDioInfo, 2, sDioInfo.asPortInfo[2].sDioCommon.bInterface);
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("ValidateDioParam", &sdRet);
#endif

	return sdRet;
}


/****************************************************************************
 *	CheckDIOCommon
 *
 *	Description:
 *			validate Digital IO Common parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIOCommon
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("CheckDIOCommon");
#endif


	if(psDioInfo->asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		if((psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_48000)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_44100)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_32000)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_24000)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_22050)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_12000)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_11025))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}

		if((psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_16000))
		{
			if(psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_512)
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
	}
	else
	{
		if((psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_512)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_256)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_128)
		|| (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_16))
		{
			sdRet	= MCDRV_ERROR_ARGUMENT;
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("CheckDIOCommon", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	CheckDIODIR
 *
 *	Description:
 *			validate Digital IO DIR parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *			bInterface	Interface(DA/PCM)
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIODIR
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort,
	UINT8	bInterface
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("CheckDIODIR");
#endif


	if(bInterface == MCDRV_DIO_PCM)
	{
		if((psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_ALAW)
		|| (psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_MULAW))
		{
			if((psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13)
			|| (psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14)
			|| (psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("CheckDIODIR", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	CheckDIDIT
 *
 *	Description:
 *			validate Digital IO DIT parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *			bInterface	Interface(DA/PCM)
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIODIT
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort,
	UINT8	bInterface
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("CheckDIODIT");
#endif


	if(bInterface == MCDRV_DIO_PCM)
	{
		if((psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_ALAW)
		|| (psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_MULAW))
		{
			if((psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13)
			|| (psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14)
			|| (psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16))
			{
				sdRet	= MCDRV_ERROR_ARGUMENT;
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("CheckDIODIT", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	SetVol
 *
 *	Description:
 *			set volume.
 *	Arguments:
 *			dUpdate			target volume items
 *			eMode			update mode
 *			pdSVolDoneParam	wait soft volume complete flag
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static SINT32	SetVol
(
	UINT32					dUpdate,
	MCDRV_VOLUPDATE_MODE	eMode,
	UINT32*					pdSVolDoneParam
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("SetVol");
#endif

	sdRet	= McPacket_AddVol(dUpdate, eMode, pdSVolDoneParam);
	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= McDevIf_ExecutePacket();
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("SetVol", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	PreUpdatePath
 *
 *	Description:
 *			Preprocess update path.
 *	Arguments:
 *			pwDACMuteParam	wait DAC mute complete flag
 *			pwDITMuteParam	wait DIT mute complete flag
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static	SINT32	PreUpdatePath
(
	UINT16*	pwDACMuteParam,
	UINT16*	pwDITMuteParam
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg	= 0;
	UINT8	bReadReg= 0;
	UINT8	bLAT	= 0;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("PreUpdatePath");
#endif

	*pwDACMuteParam	= 0;
	*pwDITMuteParam	= 0;

	switch(McResCtrl_GetDACSource(eMCDRV_DAC_MASTER))
	{
	case	eMCDRV_SRC_PDM:
		bReg	= MCB_DAC_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0:
		bReg	= MCB_DAC_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0:
		bReg	= MCB_DAC_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1:
		bReg	= MCB_DAC_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2:
		bReg	= MCB_DAC_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX:
		bReg	= MCB_DAC_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_SOURCE);
	if(((bReadReg & 0xF0) != 0) && (bReg != (bReadReg & 0xF0)))
	{/*	DAC source changed	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_MASTER_OUTL)&MCB_MASTER_OUTL) != 0)
		{
			if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_MASTER_OUTR) != 0)
			{
				bLAT	= MCB_MASTER_OLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_MASTER_OUTL, bLAT);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_MASTER_OUTR, 0);
		*pwDACMuteParam	|= (UINT16)(MCB_MASTER_OFLAGL<<8);
		*pwDACMuteParam	|= MCB_MASTER_OFLAGR;
	}

	switch(McResCtrl_GetDACSource(eMCDRV_DAC_VOICE))
	{
	case	eMCDRV_SRC_PDM:
		bReg	= MCB_VOICE_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0:
		bReg	= MCB_VOICE_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0:
		bReg	= MCB_VOICE_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1:
		bReg	= MCB_VOICE_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2:
		bReg	= MCB_VOICE_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX:
		bReg	= MCB_VOICE_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	if(((bReadReg & 0x0F) != 0) && (bReg != (bReadReg & 0x0F)))
	{/*	VOICE source changed	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_VOICE_ATTL)&MCB_VOICE_ATTL) != 0)
		{
			if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_VOICE_ATTR) != 0)
			{
				bLAT	= MCB_VOICE_LAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_VOICE_ATTL, bLAT);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_VOICE_ATTR, 0);
		*pwDACMuteParam	|= (UINT16)(MCB_VOICE_FLAGL<<8);
		*pwDACMuteParam	|= MCB_VOICE_FLAGR;
	}

	switch(McResCtrl_GetDITSource(eMCDRV_DIO_0))
	{
	case	eMCDRV_SRC_PDM:
		bReg	= MCB_DIT0_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0:
		bReg	= MCB_DIT0_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0:
		bReg	= MCB_DIT0_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1:
		bReg	= MCB_DIT0_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2:
		bReg	= MCB_DIT0_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX:
		bReg	= MCB_DIT0_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_SRC_SOURCE);
	if(((bReadReg & 0xF0) != 0) && (bReg != (bReadReg & 0xF0)))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT0_INVOLL)&MCB_DIT0_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT0_INVOLR) != 0)
			{
				bLAT	= MCB_DIT0_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT0_INVOLL, bLAT);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT0_INVOLR, 0);
		*pwDITMuteParam	|= (UINT16)(MCB_DIT0_INVFLAGL<<8);
		*pwDITMuteParam	|= MCB_DIT0_INVFLAGR;
	}

	switch(McResCtrl_GetDITSource(eMCDRV_DIO_1))
	{
	case	eMCDRV_SRC_PDM:
		bReg	= MCB_DIT1_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0:
		bReg	= MCB_DIT1_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0:
		bReg	= MCB_DIT1_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1:
		bReg	= MCB_DIT1_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2:
		bReg	= MCB_DIT1_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX:
		bReg	= MCB_DIT1_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	if(((bReadReg & 0x0F) != 0) && (bReg != (bReadReg & 0x0F)))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT1_INVOLL)&MCB_DIT1_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT1_INVOLR) != 0)
			{
				bLAT	= MCB_DIT1_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT1_INVOLL, bLAT);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT1_INVOLR, 0);
		*pwDITMuteParam	|= (UINT16)(MCB_DIT1_INVFLAGL<<8);
		*pwDITMuteParam	|= MCB_DIT1_INVFLAGR;
	}

	switch(McResCtrl_GetDITSource(eMCDRV_DIO_2))
	{
	case	eMCDRV_SRC_PDM:
		bReg	= MCB_DIT2_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0:
		bReg	= MCB_DIT2_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0:
		bReg	= MCB_DIT2_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1:
		bReg	= MCB_DIT2_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2:
		bReg	= MCB_DIT2_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX:
		bReg	= MCB_DIT2_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_SRC_SOURCE_1);
	if(((bReadReg & 0x0F) != 0) && (bReg != (bReadReg & 0x0F)))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT2_INVOLL)&MCB_DIT2_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_B_MIXER, MCI_DIT2_INVOLR) != 0)
			{
				bLAT	= MCB_DIT2_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT2_INVOLL, bLAT);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE | MCDRV_PACKET_REGTYPE_B_MIXER | (UINT32)MCI_DIT2_INVOLR, 0);
		*pwDITMuteParam	|= (UINT16)(MCB_DIT2_INVFLAGL<<8);
		*pwDITMuteParam	|= MCB_DIT2_INVFLAGR;
	}

	sdRet	= McDevIf_ExecutePacket();

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("PreUpdatePath", &sdRet);
#endif
	/*	do not Execute & Clear	*/
	return sdRet;
}

/****************************************************************************
 *	McDrv_Ctrl
 *
 *	Description:
 *			MC Driver I/F function.
 *	Arguments:
 *			dCmd		command #
 *			pvPrm		parameter
 *			dPrm		update info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
SINT32	McDrv_Ctrl
(
	UINT32	dCmd,
	void*	pvPrm,
	UINT32	dPrm
)
{
	SINT32	sdRet	= MCDRV_ERROR;

#if MCDRV_DEBUG_LEVEL
	McDebugLog_CmdIn(dCmd, pvPrm, dPrm);
#endif

	if((UINT32)MCDRV_INIT == dCmd)
	{
		sdRet	= init((MCDRV_INIT_INFO*)pvPrm);
	}
	else if((UINT32)MCDRV_TERM == dCmd)
	{
		if(McResCtrl_GetHwId() == MCDRV_HWID_AA)
		{
			sdRet	= McDrv_Ctrl_AA(dCmd, pvPrm, dPrm);
		}
		else
		{
			sdRet	= term();
		}
	}
	else
	{
		if(McResCtrl_GetHwId() == MCDRV_HWID_AA)
		{
			sdRet	= McDrv_Ctrl_AA(dCmd, pvPrm, dPrm);
		}
		else
		{
			McSrv_Lock();

			switch (dCmd)
			{
			case	MCDRV_UPDATE_CLOCK:
				sdRet	= update_clock((MCDRV_CLOCK_INFO*)pvPrm);
				break;
			case	MCDRV_SWITCH_CLOCK:
				sdRet	= switch_clock((MCDRV_CLKSW_INFO*)pvPrm);
				break;
			case	MCDRV_GET_PATH:
				sdRet	= get_path((MCDRV_PATH_INFO*)pvPrm);
				break;
			case	MCDRV_SET_PATH:
				sdRet	= set_path((MCDRV_PATH_INFO*)pvPrm);
				break;
			case	MCDRV_GET_VOLUME:
				sdRet	= get_volume((MCDRV_VOL_INFO*)pvPrm);
				break;
			case	MCDRV_SET_VOLUME:
				sdRet	= set_volume((MCDRV_VOL_INFO*)pvPrm);
				break;
			case	MCDRV_GET_DIGITALIO:
				sdRet	= get_digitalio((MCDRV_DIO_INFO*)pvPrm);
				break;
			case	MCDRV_SET_DIGITALIO:
				sdRet	= set_digitalio((MCDRV_DIO_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_GET_DAC:
				sdRet	= get_dac((MCDRV_DAC_INFO*)pvPrm);
				break;
			case	MCDRV_SET_DAC:
				sdRet	= set_dac((MCDRV_DAC_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_GET_ADC:
				sdRet	= get_adc((MCDRV_ADC_INFO*)pvPrm);
				break;
			case	MCDRV_SET_ADC:
				sdRet	= set_adc((MCDRV_ADC_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_GET_SP:
				sdRet	= get_sp((MCDRV_SP_INFO*)pvPrm);
				break;
			case	MCDRV_SET_SP:
				sdRet	= set_sp((MCDRV_SP_INFO*)pvPrm);
				break;
			case	MCDRV_GET_DNG:
				sdRet	= get_dng((MCDRV_DNG_INFO*)pvPrm);
				break;
			case	MCDRV_SET_DNG:
				sdRet	= set_dng((MCDRV_DNG_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_SET_AUDIOENGINE:
				sdRet	= set_ae((MCDRV_AE_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_GET_PDM:
				sdRet	= get_pdm((MCDRV_PDM_INFO*)pvPrm);
				break;
			case	MCDRV_SET_PDM:
				sdRet	= set_pdm((MCDRV_PDM_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_CONFIG_GP:
				sdRet	= config_gp((MCDRV_GP_MODE*)pvPrm);
				break;
			case	MCDRV_MASK_GP:
				sdRet	= mask_gp((UINT8*)pvPrm, dPrm);
				break;
			case	MCDRV_GETSET_GP:
				sdRet	= getset_gp((UINT8*)pvPrm, dPrm);
				break;
			case	MCDRV_GET_SYSEQ:
				sdRet	= get_syseq((MCDRV_SYSEQ_INFO*)pvPrm);
				break;
			case	MCDRV_SET_SYSEQ:
				sdRet	= set_syseq((MCDRV_SYSEQ_INFO*)pvPrm, dPrm);
				break;
			case	MCDRV_READ_REG :
				sdRet	= read_reg((MCDRV_REG_INFO*)pvPrm);
				break;
			case	MCDRV_WRITE_REG :
				sdRet	= write_reg((MCDRV_REG_INFO*)pvPrm);
				break;

			case	MCDRV_IRQ:
				break;

			default:
				break;
			}

			McSrv_Unlock();
		}
	}

#if MCDRV_DEBUG_LEVEL
	McDebugLog_CmdOut(dCmd, &sdRet, pvPrm);
#endif

	return sdRet;
}
