/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdriver.c
 *
 *	Description	: MC Driver
 *
 *	Version		: 1.0.6	2013.02.04
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


#include "mcdriver.h"
#include "mcservice.h"
#include "mcdevif.h"
#include "mcresctrl.h"
#include "mcparser.h"
#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcmachdep.h"
#include "mcbdspdrv.h"
#include "mccdspdrv.h"
#include "mcedspdrv.h"
#include "mcfdspdrv.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif




#define	MCDRV_MAX_WAIT_TIME		(268435455UL)
#define	MCDRV_LDO_WAIT_TIME		(1000UL)
#define	MCDRV_DAC_MUTE_WAIT_TIME	(20000UL)
#define	MCDRV_VREF_WAIT_TIME_ES1	(2000UL)
#define	MCDRV_VREF_WAIT_TIME		(30000UL)
#define	MCDRV_MB4_WAIT_TIME		(8000UL)
#define	MCDRV_SP_WAIT_TIME		(200UL)

#define	T_CPMODE_IMPSENSE_BEFORE	(0)
#define	T_CPMODE_IMPSENSE_AFTER		(2)

static const struct MCDRV_PATH_INFO	gsPathInfoAllOff	= {
	{{0x00AAAAAA}, {0x00AAAAAA} },	/* asMusicOut	*/
	{{0x00AAAAAA}, {0x00AAAAAA} },	/* asExtOut	*/
	{{0x00AAAAAA} },		/* asHifiOut	*/
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
};

static const struct MCDRV_PATH_INFO	gsPathInfoAllZero	= {
	{{0x00000000}, {0x00000000} },	/* asMusicOut	*/
	{{0x00000000}, {0x00000000} },	/* asExtOut	*/
	{{0x00000000} },		/* asHifiOut	*/
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
};

static SINT32	init(const struct MCDRV_INIT_INFO *psInitInfo,
			const struct MCDRV_INIT2_INFO *psInit2Info);
static SINT32	term(void);

static SINT32	read_reg(struct MCDRV_REG_INFO *psRegInfo);
static SINT32	write_reg(const struct MCDRV_REG_INFO *psRegInfo);

static SINT32	get_clocksw(struct MCDRV_CLOCKSW_INFO *psClockSwInfo);
static SINT32	set_clocksw(const struct MCDRV_CLOCKSW_INFO *psClockSwInfo);

static SINT32	get_path(struct MCDRV_PATH_INFO *psPathInfo);
static SINT32	set_path(const struct MCDRV_PATH_INFO *psPathInfo);

static SINT32	get_volume(struct MCDRV_VOL_INFO *psVolInfo);
static SINT32	set_volume(const struct MCDRV_VOL_INFO *psVolInfo);

static SINT32	get_digitalio(struct MCDRV_DIO_INFO *psDioInfo);
static SINT32	set_digitalio(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT32 dUpdateInfo);

static SINT32	get_digitalio_path(struct MCDRV_DIOPATH_INFO *psDioPathInfo);
static SINT32	set_digitalio_path(
				const struct MCDRV_DIOPATH_INFO *psDioPathInfo,
				UINT32 dUpdateInfo);

static SINT32	get_swap(struct MCDRV_SWAP_INFO *psSwapInfo);
static SINT32	set_swap(const struct MCDRV_SWAP_INFO *psSwapInfo,
				UINT32 dUpdateInfo);

static	UINT8	IsPathAllOff(void);
static SINT32	set_dsp(const UINT8 *pbPrm, UINT32 dSize);
static SINT32	get_dsp(struct MCDRV_DSP_PARAM *psDspParam,
				void *pvData,
				UINT32 dSize);
static SINT32	get_dsp_data(UINT8 *pbData, UINT32 dSize);
static SINT32	set_dsp_data(const UINT8 *pbData, UINT32 dSize);
static SINT32	register_dsp_cb(SINT32 (*pcbfunc)(SINT32, UINT32, UINT32));
static SINT32	get_dsp_transition(UINT32 dDspType);

static SINT32	get_hsdet(struct MCDRV_HSDET_INFO *psHSDet,
				struct MCDRV_HSDET2_INFO *psHSDet2);
static SINT32	set_hsdet(const struct MCDRV_HSDET_INFO *psHSDet,
				const struct MCDRV_HSDET2_INFO *psHSDet2,
				UINT32 dUpdateInfo);
static SINT32	config_gp(const struct MCDRV_GP_MODE *psGpMode);
static SINT32	mask_gp(UINT8 *pbMask, UINT32 dPadNo);
static SINT32	getset_gp(UINT8 *pbGpio, UINT32 dPadNo);

static SINT32	irq_proc(void);
static SINT32	BeginImpSense(UINT8 *bOP_DAC);

static UINT8	IsLDOAOn(void);
static UINT8	IsValidInitParam(const struct MCDRV_INIT_INFO *psInitInfo,
				const struct MCDRV_INIT2_INFO *psInit2Info);
static UINT8	IsValidClockSwParam(
			const struct MCDRV_CLOCKSW_INFO *psClockSwInfo);
static UINT8	IsValidReadRegParam(const struct MCDRV_REG_INFO *psRegInfo);
static UINT8	IsValidWriteRegParam(const struct MCDRV_REG_INFO *psRegInfo);
static void	MaskIrregularPath(struct MCDRV_PATH_INFO *psPathInfo);
static UINT8	IsValidDioParam(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT32 dUpdateInfo);
static UINT8	IsValidDioPathParam(
				const struct MCDRV_DIOPATH_INFO *psDioPathInfo,
				UINT32 dUpdateInfo);
static UINT8	IsValidSwapParam(const struct MCDRV_SWAP_INFO *psSwapInfo,
				UINT32 dUpdateInfo);
static UINT8	IsValidDspParam(const struct MCDRV_AEC_INFO *psAECInfo);
static UINT8	IsValidHSDetParam(const struct MCDRV_HSDET_INFO *psHSDetInfo,
				const struct MCDRV_HSDET2_INFO *psHSDet2Info,
				UINT32 dUpdateInfo);
static UINT8	IsValidGpParam(const struct MCDRV_GP_MODE *psGpMode);
static UINT8	IsValidMaskGp(UINT8 bMask, UINT32 dPadNo);

static UINT8	CheckDIOCommon(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);
static UINT8	CheckDaFormat(const struct MCDRV_DA_FORMAT *psDaFormat);
static UINT8	CheckPcmFormat(const struct MCDRV_PCM_FORMAT *psPcmFormat);
static UINT8	CheckDIODIR(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);
static UINT8	CheckDIODIT(const struct MCDRV_DIO_INFO *psDioInfo,
				UINT8 bPort);

static SINT32	SetVol(UINT32 dUpdate,
				enum MCDRV_VOLUPDATE_MODE eMode,
				UINT32 *pdSVolDoneParam);
static void	GetMuteParam(UINT8 *pbDIRMuteParam,
				UINT8 *pbADCMuteParam,
				UINT8 *pbDITMuteParam,
				UINT8 *pbDACMuteParam);
static SINT32	SavePower(void);

/****************************************************************************
 *	init
 *
 *	Description:
 *			Initialize.
 *	Arguments:
 *			psInitInfo	initialize information
 *			psInit2Info	initialize information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	init
(
	const struct MCDRV_INIT_INFO	*psInitInfo,
	const struct MCDRV_INIT2_INFO	*psInit2Info
)
{
	SINT32	sdRet;
	UINT8	abData[4];
	UINT8	bHwId_dig,
		bHwId_ana	= 0;
	UINT8	bAP		= MCI_AP_DEF;
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;

	if (NULL == psInitInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY == eState)
		return MCDRV_ERROR_STATE;

	McSrv_SystemInit();
	McSrv_Lock();
	McResCtrl_Init();

	machdep_PreLDODStart();

	if (bHwId_ana == 0) {
		abData[0]	= MCI_ANA_REG_A<<1;
		abData[1]	= MCI_ANA_ID;
		McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 2);
		bHwId_ana	= McSrv_ReadReg(MCDRV_SLAVEADDR_ANA,
						(UINT32)MCI_ANA_REG_D);
		if ((bHwId_ana&MCDRV_DEVID_MASK) == MCDRV_DEVID_ANA) {
			abData[0]	= MCI_ANA_REG_A<<1;
			abData[1]	= MCI_ANA_RST;
			abData[2]	= MCI_ANA_REG_D<<1;
			abData[3]	= MCI_ANA_RST_DEF;
			McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
			abData[3]	= 0;
			McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);

			if ((bHwId_ana&MCDRV_VERID_MASK) == 0) {
				abData[1]	= MCI_AP;
				abData[3]	= MCI_AP_DEF;
				abData[3]	&=
					(UINT8)~MCB_AP_VR;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				McSrv_Sleep(MCDRV_VREF_WAIT_TIME_ES1);
				abData[3]	&=
					(UINT8)~(MCB_AP_LDOA|MCB_AP_BGR);
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				McSrv_Sleep(MCDRV_LDO_WAIT_TIME);
				bAP	= abData[3];
			} else {
				abData[1]	= MCI_HIZ;
				abData[3]	= 0;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				abData[1]	= MCI_LO_HIZ;
				abData[3]	= 0;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				abData[1]	= MCI_AP;
				bAP	&= (UINT8)~(MCB_AP_LDOA|MCB_AP_BGR);
				abData[3]	= bAP;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				McSrv_Sleep(MCDRV_LDO_WAIT_TIME);
				abData[1]	= 62;
				abData[3]	= 0x20;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				abData[1]	= MCI_AP;
				bAP	&= (UINT8)~MCB_AP_VR;
				abData[3]	= bAP;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
				McSrv_Sleep(MCDRV_VREF_WAIT_TIME);
				abData[1]	= 62;
				abData[3]	= 0;
				McSrv_WriteReg(MCDRV_SLAVEADDR_ANA, abData, 4);
			}
		}
	}
	if ((bHwId_ana&MCDRV_DEVID_MASK) == MCDRV_DEVID_ANA) {
		if ((bHwId_ana&MCDRV_VERID_MASK) == 0)
			bHwId_dig	= 0x80;
		else
			bHwId_dig	= 0x81;
		sdRet	= McResCtrl_SetHwId(bHwId_dig, bHwId_ana);
		if (sdRet == MCDRV_SUCCESS) {
			if (IsValidInitParam(psInitInfo, psInit2Info) == 0)
				sdRet	= MCDRV_ERROR_ARGUMENT;
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP,
						bAP);
		}
		if (sdRet == MCDRV_SUCCESS) {
			McResCtrl_SetInitInfo(psInitInfo, psInit2Info);
			sdRet	= McDevIf_AllocPacketBuf();
		}
		if (sdRet == MCDRV_SUCCESS)
			sdRet	= McPacket_AddInit();

		if (sdRet == MCDRV_SUCCESS)
			sdRet	= McDevIf_ExecutePacket();

		if (sdRet == MCDRV_SUCCESS) {
			if (psInitInfo->bPowerMode == MCDRV_POWMODE_CDSPDEBUG) {
				McResCtrl_GetPowerInfo(&sPowerInfo);
				/*	used path power up	*/
				sPowerUpdate.bDigital	= MCDRV_POWUPDATE_D_ALL;
				sPowerUpdate.abAnalog[0]	= MCB_AP_LDOD;
				sPowerUpdate.abAnalog[1]	=
				sPowerUpdate.abAnalog[2]	=
				sPowerUpdate.abAnalog[3]	=
				sPowerUpdate.abAnalog[4]	= 0;
				sdRet	=
				McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
			} else {
				if ((psInit2Info != NULL)
				&& (psInit2Info->bOption[19] == 1)) {
					/*	LDOD always on	*/
					McResCtrl_GetPowerInfo(&sPowerInfo);
					sPowerInfo.bDigital	&=
						(UINT8)~MCDRV_POWINFO_D_PLL_PD;
					sPowerInfo.abAnalog[0]	&=
							(UINT8)~MCB_AP_LDOD;
					/*	used path power up	*/
					sPowerUpdate.bDigital	=
							MCDRV_POWUPDATE_D_ALL;
					sPowerUpdate.abAnalog[0]	=
								MCB_AP_LDOD;
					sPowerUpdate.abAnalog[1]	=
					sPowerUpdate.abAnalog[2]	=
					sPowerUpdate.abAnalog[3]	=
					sPowerUpdate.abAnalog[4]	= 0;
					sdRet	=
					McPacket_AddPowerUp(&sPowerInfo,
								&sPowerUpdate);
				} else {
					abData[0]	= MCI_ANA_REG_A<<1;
					abData[1]	= MCI_AP;
					abData[2]	= MCI_ANA_REG_D<<1;
					bAP	&=	(UINT8)~MCB_AP_LDOD;
					abData[3]	= bAP;
					McSrv_WriteReg(MCDRV_SLAVEADDR_ANA,
								abData, 4);
					McResCtrl_SetRegVal(
						MCDRV_PACKET_REGTYPE_ANA,
						MCI_AP, bAP);
					abData[0]	= MCI_RST_A<<1;
					abData[1]	= MCI_RST_A_DEF;
					McSrv_WriteReg(MCDRV_SLAVEADDR_DIG,
								abData, 2);
					abData[1]	=
						MCI_RST_A_DEF&~MCB_RST_A;
					McSrv_WriteReg(MCDRV_SLAVEADDR_DIG,
								abData, 2);
				}
			}
		}

		if (sdRet == MCDRV_SUCCESS)
			sdRet	= McDevIf_ExecutePacket();

		if (sdRet == MCDRV_SUCCESS) {
			abData[0]	= MCI_A_REG_A<<1;
			abData[1]	= MCI_A_DEV_ID;
			McSrv_WriteReg(MCDRV_SLAVEADDR_DIG, abData, 2);
			abData[3]	= McSrv_ReadReg(MCDRV_SLAVEADDR_DIG,
						(UINT32)MCI_A_REG_D);
			if (abData[3] != bHwId_dig)
				sdRet	= MCDRV_ERROR_INIT;
		}

		if (sdRet == MCDRV_SUCCESS) {
			McResCtrl_UpdateState(eMCDRV_STATE_READY);
			SavePower();
		} else
			McDevIf_ReleasePacketBuf();
	} else
		sdRet	= MCDRV_ERROR_INIT;

	bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	if ((bAP&MCB_AP_LDOD) != 0) {
		;
		machdep_PostLDODStart();
	}

	McSrv_Unlock();

	if (sdRet != MCDRV_SUCCESS)
		McSrv_SystemTerm();

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
static SINT32	term
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;
	UINT32			dUpdateFlg;
	struct MCDRV_HSDET_INFO	sHSDet;
	UINT8	bAP;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	McSrv_Lock();

	bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
	if ((bAP&MCB_AP_LDOD) != 0) {
		;
		machdep_PreLDODStart();
	}

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	sInitInfo.bPowerMode	= MCDRV_POWMODE_FULL;
	McResCtrl_SetInitInfo(&sInitInfo, NULL);

	sdRet	= set_path(&gsPathInfoAllOff);
	if (sdRet == MCDRV_SUCCESS) {
		sPowerInfo.bDigital	= 0xFF;
		sPowerInfo.abAnalog[0]	=
		sPowerInfo.abAnalog[1]	=
		sPowerInfo.abAnalog[2]	=
		sPowerInfo.abAnalog[3]	=
		sPowerInfo.abAnalog[4]	= (UINT8)0xFF;
		sPowerUpdate.bDigital	= MCDRV_POWUPDATE_D_ALL;
		sPowerUpdate.abAnalog[0]	=
						(UINT8)MCDRV_POWUPDATE_AP;
		sPowerUpdate.abAnalog[1]	=
						(UINT8)MCDRV_POWUPDATE_AP_OUT0;
		sPowerUpdate.abAnalog[2]	=
						(UINT8)MCDRV_POWUPDATE_AP_OUT1;
		sPowerUpdate.abAnalog[3]	=
						(UINT8)MCDRV_POWUPDATE_AP_MC;
		sPowerUpdate.abAnalog[4]	=
						(UINT8)MCDRV_POWUPDATE_AP_IN;
		sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
		if (sdRet == MCDRV_SUCCESS)
			sdRet	= McDevIf_ExecutePacket();
	}

	sHSDet.bEnPlugDet	= MCDRV_PLUGDET_DISABLE;
	sHSDet.bEnPlugDetDb	= MCDRV_PLUGDETDB_DISABLE;
	sHSDet.bEnDlyKeyOff	= MCDRV_KEYEN_D_D_D;
	sHSDet.bEnDlyKeyOn	= MCDRV_KEYEN_D_D_D;
	sHSDet.bEnMicDet	= MCDRV_MICDET_DISABLE;
	sHSDet.bEnKeyOff	= MCDRV_KEYEN_D_D_D;
	sHSDet.bEnKeyOn		= MCDRV_KEYEN_D_D_D;
	dUpdateFlg		= MCDRV_ENPLUGDET_UPDATE_FLAG
					|MCDRV_ENPLUGDETDB_UPDATE_FLAG
					|MCDRV_ENDLYKEYOFF_UPDATE_FLAG
					|MCDRV_ENDLYKEYON_UPDATE_FLAG
					|MCDRV_ENMICDET_UPDATE_FLAG
					|MCDRV_ENKEYOFF_UPDATE_FLAG
					|MCDRV_ENKEYON_UPDATE_FLAG;
	sdRet	= set_hsdet(&sHSDet, NULL, dUpdateFlg);

	McDevIf_ReleasePacketBuf();

	McResCtrl_UpdateState(eMCDRV_STATE_NOTINIT);

	if ((bAP&MCB_AP_LDOD) != 0) {
		;
		machdep_PostLDODStart();
	}
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
static SINT32	read_reg
(
	struct MCDRV_REG_INFO	*psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bSlaveAddr;
	UINT8	bAddr;
	UINT8	abData[2];
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_INFO	sCurPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;

	if (NULL == psRegInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	if (IsValidReadRegParam(psRegInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess(psRegInfo, &sPowerInfo);
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if (MCDRV_SUCCESS != sdRet)
		return sdRet;
	sdRet	= McDevIf_ExecutePacket();
	if (MCDRV_SUCCESS != sdRet)
		return sdRet;

	bAddr	= psRegInfo->bAddress;

	if (psRegInfo->bRegType == MCDRV_REGTYPE_IF) {
		if ((psRegInfo->bAddress == MCI_ANA_REG_A)
		|| (psRegInfo->bAddress == MCI_ANA_REG_D))
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		else
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
	} else {
		switch (psRegInfo->bRegType) {
		case	MCDRV_REGTYPE_A:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_A_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_A_REG_D;
			break;

		case	MCDRV_REGTYPE_MA:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_MA_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_MA_REG_D;
			break;

		case	MCDRV_REGTYPE_MB:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_MB_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_MB_REG_D;
			break;

		case	MCDRV_REGTYPE_B:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_B_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_B_REG_D;
			break;

		case	MCDRV_REGTYPE_E:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_E_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_E_REG_D;
			break;

		case	MCDRV_REGTYPE_C:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_C_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_C_REG_D;
			break;

		case	MCDRV_REGTYPE_F:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_F_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_F_REG_D;
			break;

		case	MCDRV_REGTYPE_ANA:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
			abData[0]	= MCI_ANA_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_ANA_REG_D;
			break;

		case	MCDRV_REGTYPE_CD:
			bSlaveAddr	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
			abData[0]	= MCI_CD_REG_A<<1;
			abData[1]	= bAddr;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			bAddr	= MCI_CD_REG_D;
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	read register	*/
	psRegInfo->bData	= McSrv_ReadReg(bSlaveAddr, bAddr);

	/*	restore power	*/
	sdRet	= McPacket_AddPowerDown(&sCurPowerInfo, &sPowerUpdate);
	if (MCDRV_SUCCESS != sdRet)
		return sdRet;
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
static SINT32	write_reg
(
	const struct MCDRV_REG_INFO	*psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_INFO	sCurPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;
	UINT8	abData[2];

	if (NULL == psRegInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	if (IsValidWriteRegParam(psRegInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess(psRegInfo, &sPowerInfo);
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	switch (psRegInfo->bRegType) {
	case	MCDRV_REGTYPE_IF:
		if ((psRegInfo->bAddress == MCI_ANA_REG_A)
		|| (psRegInfo->bAddress == MCI_ANA_REG_D)
		|| (psRegInfo->bAddress == MCI_CD_REG_A)
		|| (psRegInfo->bAddress == MCI_CD_REG_D)) {
			abData[0]	= psRegInfo->bAddress<<1;
			abData[1]	= psRegInfo->bData;
			McSrv_WriteReg(
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA),
				abData, 2);
		} else {
			;
			McDevIf_AddPacket(
				(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| psRegInfo->bAddress),
				psRegInfo->bData);
		}
		break;

	case	MCDRV_REGTYPE_A:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_MA:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_MA
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_MB:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_MB
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_E:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_C:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_F:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_F
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_ANA:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_CD:
		McDevIf_AddPacket(
			(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| psRegInfo->bAddress),
			psRegInfo->bData);
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	restore power	*/
	switch (psRegInfo->bRegType) {
	case	MCDRV_REGTYPE_A:
		sCurPowerInfo.bDigital &= ~MCDRV_POWINFO_D_PLL_PD;
		if (psRegInfo->bAddress == MCI_PD) {
			if (((psRegInfo->bData&MCB_PE_CLK_PD) == 0)
			&& (sCurPowerInfo.bDigital&MCDRV_POWINFO_D_PM_CLK_PD)
				== 0)
				sCurPowerInfo.bDigital
					&= ~MCDRV_POWINFO_D_PE_CLK_PD;
			if (((psRegInfo->bData&MCB_PB_CLK_PD) == 0)
			&& (sCurPowerInfo.bDigital&MCDRV_POWINFO_D_PM_CLK_PD)
				== 0)
				sCurPowerInfo.bDigital
					&= ~MCDRV_POWINFO_D_PB_CLK_PD;
		}
		break;
	case	MCDRV_REGTYPE_IF:
		if (psRegInfo->bAddress == MCI_RST) {
			if ((psRegInfo->bData&MCB_PSW_M) == 0)
				sCurPowerInfo.bDigital
					&= ~MCDRV_POWINFO_D_PM_CLK_PD;
			if ((psRegInfo->bData&MCB_PSW_F) == 0)
				sCurPowerInfo.bDigital
					&= ~MCDRV_POWINFO_D_PF_CLK_PD;
			if ((psRegInfo->bData&MCB_PSW_C) == 0)
				sCurPowerInfo.bDigital
					&= ~MCDRV_POWINFO_D_PC_CLK_PD;
		}
		break;

	case	MCDRV_REGTYPE_ANA:
		if (psRegInfo->bAddress == MCI_AP)
			sCurPowerInfo.abAnalog[0]	= psRegInfo->bData;
		else if (psRegInfo->bAddress == MCI_AP_DA0)
			sCurPowerInfo.abAnalog[1]	= psRegInfo->bData;
		else if (psRegInfo->bAddress == MCI_AP_DA1)
			sCurPowerInfo.abAnalog[2]	= psRegInfo->bData;
		else if (psRegInfo->bAddress == MCI_AP_MIC)
			sCurPowerInfo.abAnalog[3]	= psRegInfo->bData;
		else if (psRegInfo->bAddress == MCI_AP_AD)
			sCurPowerInfo.abAnalog[4]	= psRegInfo->bData;
		break;

	default:
		break;
	}
	sdRet	= McPacket_AddPowerDown(&sCurPowerInfo, &sPowerUpdate);
	if (MCDRV_SUCCESS == sdRet)
		return sdRet;
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_clocksw
 *
 *	Description:
 *			Get clock switch setting.
 *	Arguments:
 *			psClockSwInfo	clock switch information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	get_clocksw
(
	struct MCDRV_CLOCKSW_INFO	*psClockSwInfo
)
{
	if (NULL == psClockSwInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	McResCtrl_GetClockSwInfo(psClockSwInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_clocksw
 *
 *	Description:
 *			Set clock switch.
 *	Arguments:
 *			psClockSwInfo	clock switch information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	set_clocksw
(
	const struct MCDRV_CLOCKSW_INFO	*psClockSwInfo
)
{
	struct MCDRV_CLOCKSW_INFO	sCurClockSwInfo;

	if (NULL == psClockSwInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	if (IsValidClockSwParam(psClockSwInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_GetClockSwInfo(&sCurClockSwInfo);
	if (sCurClockSwInfo.bClkSrc == psClockSwInfo->bClkSrc)
		return MCDRV_SUCCESS;

	McResCtrl_SetClockSwInfo(psClockSwInfo);
	McPacket_AddClockSw();
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
static SINT32	get_path
(
	struct MCDRV_PATH_INFO	*psPathInfo
)
{
	if (NULL == psPathInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

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
static SINT32	set_path
(
	const struct MCDRV_PATH_INFO	*psPathInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dSVolDoneParam	= 0;
	UINT8	bDIRMuteParam	= 0;
	UINT8	bADCMuteParam	= 0;
	UINT8	bDITMuteParam	= 0;
	UINT8	bDACMuteParam	= 0;
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_PATH_INFO	sPathInfo;
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;
	UINT8	bDSPStarted	= McResCtrl_GetDspStart();
	UINT8	bHPVolL, bHPVolR;
	UINT8	bSPVolL, bSPVolR;
	UINT8	bReg;

	if (NULL == psPathInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	sPathInfo	= *psPathInfo;
	MaskIrregularPath(&sPathInfo);

	sdRet	= McResCtrl_SetPathInfo(&sPathInfo);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	bHPVolL	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_HPVOL_L);
	bHPVolL	&= (UINT8)~MCB_ALAT_HP;
	bHPVolR	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_HPVOL_R);
	bSPVolL	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_SPVOL_L);
	bSPVolL	&= (UINT8)~MCB_ALAT_SP;
	bSPVolR	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_SPVOL_R);

	/*	unused analog out volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ANA_OUT,
			eMCDRV_VOLUPDATE_MUTE,
			&dSVolDoneParam);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	if (dSVolDoneParam != 0UL)
		McDevIf_AddPacket(
			MCDRV_PACKET_TYPE_EVTWAIT
			| MCDRV_EVT_SVOL_DONE
			| dSVolDoneParam,
			0);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	if ((bSPVolL != 0)
	|| (bSPVolR != 0)) {
		bSPVolL	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_SPVOL_L);
		bSPVolL	&= (UINT8)~MCB_ALAT_SP;
		bSPVolR	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_SPVOL_R);
		if ((bSPVolL == 0)
		&& (bSPVolR == 0)) {
			;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
					| MCDRV_SP_WAIT_TIME, 0);
		}
	}

	GetMuteParam(&bDIRMuteParam,
			&bADCMuteParam,
			&bDITMuteParam,
			&bDACMuteParam);

	/*	unused volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_DIG,
			eMCDRV_VOLUPDATE_MUTE,
			NULL);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	set volume	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ANA_IN,
			eMCDRV_VOLUPDATE_ALL,
			NULL);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	wait VFLAG	*/
	if (bDIRMuteParam != 0)
		McDevIf_AddPacket(
			MCDRV_PACKET_TYPE_EVTWAIT
			| MCDRV_EVT_DIRMUTE
			| bDIRMuteParam,
			0);

	if (bADCMuteParam != 0)
		McDevIf_AddPacket(
			MCDRV_PACKET_TYPE_EVTWAIT
			| MCDRV_EVT_ADCMUTE
			| bADCMuteParam,
			0);

	if (bDITMuteParam != 0)
		McDevIf_AddPacket(
			MCDRV_PACKET_TYPE_EVTWAIT
			| MCDRV_EVT_DITMUTE
			| bDITMuteParam,
			0);

	if (bDACMuteParam != 0)
		McDevIf_AddPacket(
			MCDRV_PACKET_TYPE_EVTWAIT
			| MCDRV_EVT_DACMUTE
			| bDACMuteParam,
			0);

	/*	F-DSP stop	*/
	McPacket_AddFDSPStop(bDSPStarted);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	stop unused path	*/
	McPacket_AddStop();
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	McResCtrl_GetPowerInfo(&sPowerInfo);

	/*	unused analog out path power down	*/
	sPowerUpdate.bDigital		= 0;
	sPowerUpdate.abAnalog[0]	= (UINT8)0;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)0;
	sPowerUpdate.abAnalog[4]	= (UINT8)0;
	sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	used path power up	*/
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	set mixer	*/
	McPacket_AddPathSet();
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	DSP start	*/
	McPacket_AddDSPStartStop(bDSPStarted);
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	if ((bHPVolL != 0)
	|| (bHPVolR != 0)) {
		if ((bHPVolL == 0)
		|| ((dSVolDoneParam&(MCB_HPL_BUSY<<8)) != 0)) {
			if ((bHPVolR == 0)
			|| ((dSVolDoneParam&(MCB_HPR_BUSY<<8)) != 0)) {
				if ((sPowerInfo.abAnalog[3] & MCB_MB4) != 0) {
					bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_ANA,
						MCI_AP_MIC);
					if ((bReg&MCB_MB4) == 0) {
						;
						McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_TIMWAIT
						| MCDRV_MB4_WAIT_TIME,
						0);
					}
				}
			}
		}
	}

	/*	unused path power down	*/
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	start	*/
	McPacket_AddStart();
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	/*	set volume	*/
	sdRet = SetVol(MCDRV_VOLUPDATE_DIG,
			eMCDRV_VOLUPDATE_ALL,
			NULL);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;
	sdRet	= SetVol(MCDRV_VOLUPDATE_ANA_OUT,
			eMCDRV_VOLUPDATE_ALL,
			&dSVolDoneParam);
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	return sdRet;
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
static SINT32	get_volume
(
	struct MCDRV_VOL_INFO	*psVolInfo
)
{
	if (NULL == psVolInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

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
static SINT32	set_volume
(
	const struct MCDRV_VOL_INFO	*psVolInfo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_PATH_INFO	sPathInfo;

	if (NULL == psVolInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

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
static SINT32	get_digitalio
(
	struct MCDRV_DIO_INFO	*psDioInfo
)
{
	if (NULL == psDioInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

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
static SINT32	set_digitalio
(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT32	dUpdateInfo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (NULL == psDioInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (IsValidDioParam(psDioInfo, dUpdateInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_SetDioInfo(psDioInfo, dUpdateInfo);

	 McPacket_AddDigitalIO(dUpdateInfo);
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_digitalio_path
 *
 *	Description:
 *			Get current digital IO path setting.
 *	Arguments:
 *			psDioInfoPath	digital IO path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	get_digitalio_path
(
	struct MCDRV_DIOPATH_INFO	*psDioPathInfo
)
{
	if (NULL == psDioPathInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != McResCtrl_GetState())
		return MCDRV_ERROR_STATE;

	McResCtrl_GetDioPathInfo(psDioPathInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_digitalio_path
 *
 *	Description:
 *			Update digital IO path configuration.
 *	Arguments:
 *			psDioInfoPath	digital IO path configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	set_digitalio_path
(
	const struct MCDRV_DIOPATH_INFO	*psDioPathInfo,
	UINT32	dUpdateInfo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (NULL == psDioPathInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (IsValidDioPathParam(psDioPathInfo, dUpdateInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_SetDioPathInfo(psDioPathInfo, dUpdateInfo);

	McPacket_AddDigitalIOPath();
	return McDevIf_ExecutePacket();
}

/****************************************************************************
 *	get_swap
 *
 *	Description:
 *			Get Swap setting.
 *	Arguments:
 *			psSwapInfo	pointer to MCDRV_SWAP_INFO struct
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	get_swap
(
	struct MCDRV_SWAP_INFO	*psSwapInfo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == psSwapInfo)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_GetSwap(psSwapInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_swap
 *
 *	Description:
 *			Set Swap setting.
 *	Arguments:
 *			psSwapInfo	pointer to MCDRV_SWAP_INFO struct
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	set_swap
(
	const struct MCDRV_SWAP_INFO	*psSwapInfo,
	UINT32	dUpdateInfo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == psSwapInfo)
		return MCDRV_ERROR_ARGUMENT;

	if (IsValidSwapParam(psSwapInfo, dUpdateInfo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_SetSwap(psSwapInfo, dUpdateInfo);
	McPacket_AddSwap(dUpdateInfo);
	return McDevIf_ExecutePacket();
}


/****************************************************************************
 *	IsPathAllOff
 *
 *	Description:
 *			Is Path All Off.
 *	Arguments:
 *			none
 *	Return:
 *			0:not All Off, 1:All Off
 *
 ****************************************************************************/
static	UINT8	IsPathAllOff
(
	void
)
{
	UINT8	bRet	= 1;
	struct MCDRV_PATH_INFO	sCurPathInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsPathAllOff");
#endif
	McResCtrl_GetPathInfoVirtual(&sCurPathInfo);
	if ((sCurPathInfo.asMusicOut[0].dSrcOnOff
		!= gsPathInfoAllOff.asMusicOut[0].dSrcOnOff)
	|| (sCurPathInfo.asMusicOut[1].dSrcOnOff
		!= gsPathInfoAllOff.asMusicOut[1].dSrcOnOff)
	|| (sCurPathInfo.asExtOut[0].dSrcOnOff
		!= gsPathInfoAllOff.asExtOut[0].dSrcOnOff)
	|| (sCurPathInfo.asExtOut[1].dSrcOnOff
		!= gsPathInfoAllOff.asExtOut[1].dSrcOnOff)
	|| (sCurPathInfo.asHifiOut[0].dSrcOnOff
		!= gsPathInfoAllOff.asHifiOut[0].dSrcOnOff)
	|| (sCurPathInfo.asVboxMixIn[0].dSrcOnOff
		!= gsPathInfoAllOff.asVboxMixIn[0].dSrcOnOff)
	|| (sCurPathInfo.asVboxMixIn[1].dSrcOnOff
		!= gsPathInfoAllOff.asVboxMixIn[1].dSrcOnOff)
	|| (sCurPathInfo.asVboxMixIn[2].dSrcOnOff
		!= gsPathInfoAllOff.asVboxMixIn[2].dSrcOnOff)
	|| (sCurPathInfo.asVboxMixIn[3].dSrcOnOff
		!= gsPathInfoAllOff.asVboxMixIn[3].dSrcOnOff)
	|| (sCurPathInfo.asAe0[0].dSrcOnOff
		!= gsPathInfoAllOff.asAe0[0].dSrcOnOff)
	|| (sCurPathInfo.asAe0[1].dSrcOnOff
		!= gsPathInfoAllOff.asAe0[1].dSrcOnOff)
	|| (sCurPathInfo.asAe1[0].dSrcOnOff
		!= gsPathInfoAllOff.asAe1[0].dSrcOnOff)
	|| (sCurPathInfo.asAe1[1].dSrcOnOff
		!= gsPathInfoAllOff.asAe1[1].dSrcOnOff)
	|| (sCurPathInfo.asAe2[0].dSrcOnOff
		!= gsPathInfoAllOff.asAe2[0].dSrcOnOff)
	|| (sCurPathInfo.asAe2[1].dSrcOnOff
		!= gsPathInfoAllOff.asAe2[1].dSrcOnOff)
	|| (sCurPathInfo.asAe3[0].dSrcOnOff
		!= gsPathInfoAllOff.asAe3[0].dSrcOnOff)
	|| (sCurPathInfo.asAe3[1].dSrcOnOff
		!= gsPathInfoAllOff.asAe3[1].dSrcOnOff)
	|| (sCurPathInfo.asDac0[0].dSrcOnOff
		!= gsPathInfoAllOff.asDac0[0].dSrcOnOff)
	|| (sCurPathInfo.asDac0[1].dSrcOnOff
		!= gsPathInfoAllOff.asDac0[1].dSrcOnOff)
	|| (sCurPathInfo.asDac1[0].dSrcOnOff
		!= gsPathInfoAllOff.asDac1[0].dSrcOnOff)
	|| (sCurPathInfo.asDac1[1].dSrcOnOff
		!= gsPathInfoAllOff.asDac1[1].dSrcOnOff)
	|| (sCurPathInfo.asVoiceOut[0].dSrcOnOff
		!= gsPathInfoAllOff.asVoiceOut[0].dSrcOnOff)
	|| (sCurPathInfo.asVboxIoIn[0].dSrcOnOff
		!= gsPathInfoAllOff.asVboxIoIn[0].dSrcOnOff)
	|| (sCurPathInfo.asVboxHostIn[0].dSrcOnOff
		!= gsPathInfoAllOff.asVboxHostIn[0].dSrcOnOff)
	|| (sCurPathInfo.asHostOut[0].dSrcOnOff
		!= gsPathInfoAllOff.asHostOut[0].dSrcOnOff)
	|| (sCurPathInfo.asAdif0[0].dSrcOnOff
		!= gsPathInfoAllOff.asAdif0[0].dSrcOnOff)
	|| (sCurPathInfo.asAdif0[1].dSrcOnOff
		!= gsPathInfoAllOff.asAdif0[1].dSrcOnOff)
	|| (sCurPathInfo.asAdif1[0].dSrcOnOff
		!= gsPathInfoAllOff.asAdif1[0].dSrcOnOff)
	|| (sCurPathInfo.asAdif1[1].dSrcOnOff
		!= gsPathInfoAllOff.asAdif1[1].dSrcOnOff)
	|| (sCurPathInfo.asAdif2[0].dSrcOnOff
		!= gsPathInfoAllOff.asAdif2[0].dSrcOnOff)
	|| (sCurPathInfo.asAdif2[1].dSrcOnOff
		!= gsPathInfoAllOff.asAdif2[1].dSrcOnOff)
	|| (sCurPathInfo.asAdc0[0].dSrcOnOff
		!= gsPathInfoAllOff.asAdc0[0].dSrcOnOff)
	|| (sCurPathInfo.asAdc0[1].dSrcOnOff
		!= gsPathInfoAllOff.asAdc0[1].dSrcOnOff)
	|| (sCurPathInfo.asAdc1[0].dSrcOnOff
		!= gsPathInfoAllOff.asAdc1[0].dSrcOnOff)
	|| (sCurPathInfo.asSp[0].dSrcOnOff
		!= gsPathInfoAllOff.asSp[0].dSrcOnOff)
	|| (sCurPathInfo.asSp[1].dSrcOnOff
		!= gsPathInfoAllOff.asSp[1].dSrcOnOff)
	|| (sCurPathInfo.asHp[0].dSrcOnOff
		!= gsPathInfoAllOff.asHp[0].dSrcOnOff)
	|| (sCurPathInfo.asHp[1].dSrcOnOff
		!= gsPathInfoAllOff.asHp[1].dSrcOnOff)
	|| (sCurPathInfo.asRc[0].dSrcOnOff
		!= gsPathInfoAllOff.asRc[0].dSrcOnOff)
	|| (sCurPathInfo.asLout1[0].dSrcOnOff
		!= gsPathInfoAllOff.asLout1[0].dSrcOnOff)
	|| (sCurPathInfo.asLout1[1].dSrcOnOff
		!= gsPathInfoAllOff.asLout1[1].dSrcOnOff)
	|| (sCurPathInfo.asLout2[0].dSrcOnOff
		!= gsPathInfoAllOff.asLout2[0].dSrcOnOff)
	|| (sCurPathInfo.asLout2[1].dSrcOnOff
		!= gsPathInfoAllOff.asLout2[1].dSrcOnOff)
	|| (sCurPathInfo.asBias[0].dSrcOnOff
		!= gsPathInfoAllOff.asBias[0].dSrcOnOff)
	|| (sCurPathInfo.asBias[1].dSrcOnOff
		!= gsPathInfoAllOff.asBias[1].dSrcOnOff)
	|| (sCurPathInfo.asBias[2].dSrcOnOff
		!= gsPathInfoAllOff.asBias[2].dSrcOnOff)
	|| (sCurPathInfo.asBias[3].dSrcOnOff
		!= gsPathInfoAllOff.asBias[3].dSrcOnOff)
	)
		bRet	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsPathAllOff", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	set_dsp
 *
 *	Description:
 *			Set DSP parameter.
 *	Arguments:
 *			pbPrm	pointer to AEC parameter
 *			dSize	data byte size.
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	set_dsp
(
	const UINT8	*pbPrm,
	UINT32	dSize
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_AEC_D7_INFO	sD7Info;
	struct MCDRV_AEC_INFO		sAECInfo, sCurAECInfo;
	struct MCDRV_POWER_INFO		sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;
	static struct MCDRV_PATH_INFO	sPathInfo;
	static UINT8	bLP2_START, bSRC3_START;
	UINT32	dSVolDoneParam	= 0;
	UINT32	dMuteFlg;
	UINT8	bReg;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == pbPrm)
		return MCDRV_ERROR_ARGUMENT;

	if (dSize == 0)
		return MCDRV_SUCCESS;

	McResCtrl_GetAecInfo(&sAECInfo);
	sCurAECInfo	= sAECInfo;

	sdRet	= McParser_GetD7Chunk(pbPrm, dSize, &sD7Info);
	if (sdRet < MCDRV_SUCCESS)
		return sdRet;

	sdRet	= McParser_AnalyzeD7Chunk(&sD7Info, &sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		return sdRet;

	if (IsValidDspParam(&sAECInfo) == 0) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		return sdRet;
	}

	McResCtrl_SetAecInfo(&sAECInfo);
	McResCtrl_GetAecInfo(&sAECInfo);

	sAECInfo.sAecVBox.bCDspFuncAOnOff	&= 0x01;
	sAECInfo.sAecVBox.bCDspFuncBOnOff	&= 0x01;
	if (sAECInfo.sControl.bCommand == 1) {
		if (sAECInfo.sControl.bParam[0] == 0) {
			if (IsPathAllOff() == 0) {
				McResCtrl_GetPathInfoVirtual(&sPathInfo);
				sdRet	= set_path(&gsPathInfoAllOff);
				if (sdRet != MCDRV_SUCCESS) {
					;
					goto exit;
				}
			}
		}
	} else if (sAECInfo.sControl.bCommand == 2) {
		if (sAECInfo.sControl.bParam[0] == 0) {
			bLP2_START	=
				McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB,
							MCI_LP2_START);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_LP2_START,
					0);
			bSRC3_START	=
				McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB,
							MCI_SRC3_START);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_SRC3_START,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
		}
	} else if (sAECInfo.sControl.bCommand == 3) {
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McPacket_AddDOutMute();
			dMuteFlg	= MCB_DIFO3_VFLAG1
					| MCB_DIFO3_VFLAG0
					| MCB_DIFO2_VFLAG1
					| MCB_DIFO2_VFLAG0
					| MCB_DIFO1_VFLAG1
					| MCB_DIFO1_VFLAG0
					| MCB_DIFO0_VFLAG1
					| MCB_DIFO0_VFLAG0;
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_DITMUTE
				| dMuteFlg,
				0);
			dMuteFlg	= MCB_DAO1_VFLAG1
					| MCB_DAO1_VFLAG0
					| MCB_DAO0_VFLAG1
					| MCB_DAO0_VFLAG0;
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_DACMUTE
				| dMuteFlg,
				0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MA
					| (UINT32)MCI_CLK_SEL,
					sAECInfo.sControl.bParam[0]);
			McResCtrl_SetClkSel(sAECInfo.sControl.bParam[0]);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			sdRet	= SetVol(MCDRV_VOLUPDATE_DOUT,
					eMCDRV_VOLUPDATE_ALL,
					NULL);
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
		}
	} else if (sAECInfo.sControl.bCommand == 4) {
		if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
			McPacket_AddDac0Mute();
			McPacket_AddDac1Mute();
			McPacket_AddAdifMute();
			McPacket_AddDPathDAMute();
			dMuteFlg	= MCB_ADI2_VFLAG1
					| MCB_ADI2_VFLAG0
					| MCB_ADI1_VFLAG1
					| MCB_ADI1_VFLAG0
					| MCB_ADI0_VFLAG1
					| MCB_ADI0_VFLAG0;
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_ADCMUTE
				| dMuteFlg,
				0);
			dMuteFlg	= MCB_SPR_BUSY<<8
					| MCB_SPL_BUSY<<8
					| MCB_HPR_BUSY<<8
					| MCB_HPL_BUSY<<8
					| MCB_LO2R_BUSY
					| MCB_LO2L_BUSY
					| MCB_LO1R_BUSY
					| MCB_LO1L_BUSY
					| MCB_RC_BUSY;
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_SVOL_DONE
				| dMuteFlg,
				0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_ECLK_SEL,
					sAECInfo.sControl.bParam[0]);
			McResCtrl_SetEClkSel(sAECInfo.sControl.bParam[0]);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			sdRet	= SetVol(MCDRV_VOLUPDATE_ANA_OUT
					| MCDRV_VOLUPDATE_ADIF0
					| MCDRV_VOLUPDATE_ADIF1
					| MCDRV_VOLUPDATE_ADIF2
					| MCDRV_VOLUPDATE_DPATHDA,
					eMCDRV_VOLUPDATE_ALL,
					&dSVolDoneParam);
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
		}
	} else if (sAECInfo.sControl.bCommand == 5) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A,
								MCI_FREQ73M);
		if ((sAECInfo.sControl.bParam[0] != 0xFF)) {
			bReg	&= 0xF8;
			bReg	|= sAECInfo.sControl.bParam[0];
			McResCtrl_SetCClkSel(sAECInfo.sControl.bParam[0]);
		}
		if ((sAECInfo.sControl.bParam[1] != 0xFF)) {
			bReg	&= 0x3F;
			bReg	|= (sAECInfo.sControl.bParam[1]<<6);
			McResCtrl_SetFClkSel(sAECInfo.sControl.bParam[1]);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_A
				| (UINT32)MCI_FREQ73M,
				bReg);
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
	}

	sdRet	= McBdsp_SetDSPCheck(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McCdsp_SetDSPCheck(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McEdsp_SetDSPCheck(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McFdsp_SetDSPCheck(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;

	McResCtrl_GetPowerInfo(&sPowerInfo);
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		goto exit;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	McPacket_AddAEC();
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	sdRet	= McBdsp_SetDSP(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McCdsp_SetDSP(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McEdsp_SetDSP(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;
	sdRet	= McFdsp_SetDSP(&sAECInfo);
	if (sdRet < MCDRV_SUCCESS)
		goto exit;

	if (sAECInfo.sControl.bCommand == 1) {
		if (sAECInfo.sControl.bParam[0] == 1) {
			sdRet	= set_path(&sPathInfo);
			sPathInfo	= gsPathInfoAllZero;
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
		}
	} else if (sAECInfo.sControl.bCommand == 2) {
		if (sAECInfo.sControl.bParam[0] == 1) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_LP2_START,
					bLP2_START);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_MB
					| (UINT32)MCI_SRC3_START,
					bSRC3_START);
		}
	}

	McResCtrl_GetPowerInfo(&sPowerInfo);
	sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		goto exit;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet == MCDRV_SUCCESS)
		return sdRet;

exit:
	McResCtrl_ReplaceAecInfo(&sCurAECInfo);
	SavePower();
	return sdRet;
}

/****************************************************************************
 *	get_dsp
 *
 *	Description:
 *			Get DSP parameter.
 *	Arguments:
 *			psDspParam	pointer to parameter
 *			pvData		pointer to read data buffer
 *			dSize		data buffer size
 *	Return:
 *			0<=	Get data size
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	get_dsp
(
	struct MCDRV_DSP_PARAM	*psDspParam,
	void	*pvData,
	UINT32	dSize
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == psDspParam)
		return MCDRV_ERROR_ARGUMENT;

	if (NULL == pvData)
		return MCDRV_ERROR_ARGUMENT;

	switch (psDspParam->dType) {
	case	MCDRV_DSP_PARAM_CDSP_INPOS:
		return McCdsp_GetDSP(CDSP_INPOS, pvData, dSize);
	case	MCDRV_DSP_PARAM_CDSP_OUTPOS:
		return McCdsp_GetDSP(CDSP_OUTPOS, pvData, dSize);
	case	MCDRV_DSP_PARAM_CDSP_DFIFO_REMAIN:
		return McCdsp_GetDSP(CDSP_DFIFO_REMAIN, pvData, dSize);
	case	MCDRV_DSP_PARAM_CDSP_RFIFO_REMAIN:
		return McCdsp_GetDSP(CDSP_RFIFO_REMAIN, pvData, dSize);
	case	MCDRV_DSP_PARAM_FDSP_DXRAM:
		return McFdsp_GetDSP(FDSP_AE_FW_DXRAM, psDspParam->dInfo,
						(UINT8 *)pvData, dSize);
	case	MCDRV_DSP_PARAM_FDSP_DYRAM:
		return McFdsp_GetDSP(FDSP_AE_FW_DYRAM, psDspParam->dInfo,
						(UINT8 *)pvData, dSize);
	case	MCDRV_DSP_PARAM_FDSP_IRAM:
		return McFdsp_GetDSP(FDSP_AE_FW_IRAM, psDspParam->dInfo,
						(UINT8 *)pvData, dSize);
	case	MCDRV_DSP_PARAM_EDSP_E2RES:
		return McEdsp_GetDSP((UINT8 *)pvData);
	default:
		return MCDRV_ERROR_ARGUMENT;
	}
}

/****************************************************************************
 *	get_dsp_data
 *
 *	Description:
 *			Get DSP data.
 *	Arguments:
 *			pbData	pointer to data
 *			dSize	data byte size
 *	Return:
 *			0<=	size of got data
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	get_dsp_data
(
	UINT8	*pbData,
	UINT32	dSize
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == pbData)
		return MCDRV_ERROR_ARGUMENT;

	return McCdsp_ReadData(pbData, dSize);
}

/****************************************************************************
 *	set_dsp_data
 *
 *	Description:
 *			Write data to DSP.
 *	Arguments:
 *			pbData	pointer to data
 *			dSize	data byte size
 *	Return:
 *			0<=	size of write data
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	set_dsp_data
(
	const UINT8	*pbData,
	UINT32	dSize
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == pbData)
		return MCDRV_ERROR_ARGUMENT;

	return McCdsp_WriteData(pbData, dSize);
}

/****************************************************************************
 *	register_dsp_cb
 *
 *	Description:
 *			Callback function setting
 *	Arguments:
 *			pcbfunc	Pointer to Callback function
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	register_dsp_cb
(
	SINT32 (*pcbfunc)(SINT32, UINT32, UINT32)
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	McResCtrl_SetDSPCBFunc(pcbfunc);
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	get_dsp_transition
 *
 *	Description:
 *			It judges while processing the DSP control
 *	Arguments:
 *			dDspType	DSP type
 *	Return:
 *			0	has processed
 *			1<=	processing
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	get_dsp_transition
(
	UINT32	dDspType
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (dDspType == MCDRV_DSPTYPE_FDSP) {
		;
		return McFdsp_GetTransition();
	} else if (dDspType == MCDRV_DSPTYPE_BDSP) {
		return McBdsp_GetTransition();
	}

	return MCDRV_ERROR_ARGUMENT;
}

/****************************************************************************
 *	get_hsdet
 *
 *	Description:
 *			Get Headset Det.
 *	Arguments:
 *			psHSDet	pointer to MCDRV_HSDET_INFO struct
 *			psHSDet	pointer to MCDRV_HSDET2_INFO struct
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	get_hsdet
(
	struct MCDRV_HSDET_INFO	*psHSDet,
	struct MCDRV_HSDET2_INFO	*psHSDet2
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == psHSDet)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_GetHSDet(psHSDet, psHSDet2);
	psHSDet->bDlyIrqStop	= 0;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_hsdet
 *
 *	Description:
 *			Set Headset Det.
 *	Arguments:
 *			psHSDet	pointer to MCDRV_HSDET_INFO struct
 *			psHSDet	pointer to MCDRV_HSDET2_INFO struct
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static SINT32	set_hsdet
(
	const struct MCDRV_HSDET_INFO	*psHSDet,
	const struct MCDRV_HSDET2_INFO	*psHSDet2,
	UINT32	dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_HSDET_INFO	sHSDetInfo;
	UINT8	bReg;
	struct MCDRV_PATH_INFO	sCurPathInfo, sTmpPathInfo;
	struct MCDRV_HSDET_RES	sHSDetRes;
	UINT8	bSlaveAddrA;
	UINT8	abData[2];
	UINT8	bPlugDetDB;
	struct MCDRV_HSDET_INFO	sHSDet;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (NULL == psHSDet)
		return MCDRV_ERROR_ARGUMENT;

	sHSDet	= *psHSDet;
	if (IsValidHSDetParam(&sHSDet, psHSDet2, dUpdateInfo) == 0) {
		;
		return MCDRV_ERROR_ARGUMENT;
	}
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		sHSDet.bDetInInv	&= 1;
	}

	McResCtrl_SetHSDet(&sHSDet, psHSDet2, dUpdateInfo);
	McPacket_AddHSDet();
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		return sdRet;

	McResCtrl_GetHSDet(&sHSDetInfo, NULL);
	if ((sHSDetInfo.bEnPlugDetDb != MCDRV_PLUGDETDB_DISABLE)
	|| (sHSDetInfo.bEnDlyKeyOff != MCDRV_KEYEN_D_D_D)
	|| (sHSDetInfo.bEnDlyKeyOn != MCDRV_KEYEN_D_D_D)
	|| (sHSDetInfo.bEnMicDet != MCDRV_MICDET_DISABLE)
	|| (sHSDetInfo.bEnKeyOff != MCDRV_KEYEN_D_D_D)
	|| (sHSDetInfo.bEnKeyOn != MCDRV_KEYEN_D_D_D)) {
		bPlugDetDB	= McResCtrl_GetPlugDetDB();
		bReg	= McResCtrl_GetRegVal(
				MCDRV_PACKET_REGTYPE_CD,
				MCI_HSDETEN);
		if ((bReg&MCB_MKDETEN) == 0) {
			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_HSDETEN,
				(UINT8)(MCB_HSDETEN | sHSDetInfo.bHsDetDbnc));

			if (((bPlugDetDB&MCB_RPLUGDET_DB) != 0)
			&& ((sHSDetInfo.bEnDlyKeyOff != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnDlyKeyOn != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnMicDet != MCDRV_MICDET_DISABLE)
			|| (sHSDetInfo.bEnKeyOff != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnKeyOn != MCDRV_KEYEN_D_D_D))) {
				if (IsLDOAOn() != 0) {
					McResCtrl_GetPathInfoVirtual(
								&sCurPathInfo);
					sTmpPathInfo	= sCurPathInfo;
					sTmpPathInfo.asAdc0[0].dSrcOnOff
						= 0x00AAAAAA;
					sTmpPathInfo.asAdc0[1].dSrcOnOff
						= 0x00AAAAAA;

					sTmpPathInfo.asAdc1[0].dSrcOnOff
						= 0x00AAAAAA;

					sTmpPathInfo.asSp[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asSp[1].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asHp[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asHp[1].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asRc[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asLout1[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asLout1[1].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asLout2[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asLout2[1].dSrcOnOff
						= 0x002AAAAA;

					sTmpPathInfo.asBias[0].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asBias[1].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asBias[2].dSrcOnOff
						= 0x002AAAAA;
					sTmpPathInfo.asBias[3].dSrcOnOff
						= 0x002AAAAA;
					sdRet	= set_path(&sTmpPathInfo);
					if (sdRet != MCDRV_SUCCESS)
						return sdRet;
					McPacket_AddMKDetEnable(1);
					sdRet	= set_path(&sCurPathInfo);
				} else {
					McPacket_AddMKDetEnable(1);
				}
			}
		} else {
			bReg	= (UINT8)(MCB_HSDETEN | sHSDetInfo.bHsDetDbnc);
			if ((sHSDetInfo.bEnDlyKeyOff != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnDlyKeyOn != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnMicDet != MCDRV_MICDET_DISABLE)
			|| (sHSDetInfo.bEnKeyOff != MCDRV_KEYEN_D_D_D)
			|| (sHSDetInfo.bEnKeyOn != MCDRV_KEYEN_D_D_D)) {
				if ((bPlugDetDB&MCB_RPLUGDET_DB) != 0) {
					;
					bReg	|= MCB_MKDETEN;
				}
			}

			McDevIf_AddPacket(
				MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_CD
				| (UINT32)MCI_HSDETEN,
				bReg);

			if ((bReg&MCB_MKDETEN) == 0) {
				bReg	=
				McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_KDSET);
				bReg	&= (UINT8)~(MCB_KDSET2|MCB_KDSET1);
				McDevIf_AddPacket(
					MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)MCI_KDSET,
					bReg);
			}
		}
	}
	if (sdRet == MCDRV_SUCCESS)
		sdRet	= McDevIf_ExecutePacket();
	if (sdRet == MCDRV_SUCCESS) {
		if (((dUpdateInfo & MCDRV_ENPLUGDETDB_UPDATE_FLAG) != 0)
		&& ((sHSDetInfo.bEnPlugDetDb & MCDRV_PLUGDETDB_UNDET_ENABLE)
			!= 0)
		&& (sHSDetInfo.cbfunc != 0)) {
			bSlaveAddrA	=
				McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
#if 0
			McSrv_Sleep(sHSDetInfo.bHsDetDbnc*1000);
			abData[0]	= MCI_CD_REG_A<<1;
			abData[1]	= MCI_PLUGDET_DB;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
			if ((bReg & MCB_PLUGUNDET_DB) != 0) {
#else
			abData[0]	= MCI_CD_REG_A<<1;
			abData[1]	= MCI_PLUGDET;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
			if ((bReg & MCB_PLUGDET) == 0) {
#endif
				McSrv_Unlock();
				sHSDetRes.bKeyCnt0	= 0;
				sHSDetRes.bKeyCnt1	= 0;
				sHSDetRes.bKeyCnt2	= 0;
				(*sHSDetInfo.cbfunc)(
					MCDRV_HSDET_EVT_PLUGUNDET_DB_FLAG,
					&sHSDetRes);
				McSrv_Lock();
			}
		}
		sdRet	= SavePower();
	}
	return sdRet;
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
static SINT32	config_gp
(
	const struct MCDRV_GP_MODE	*psGpMode
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();

	if (NULL == psGpMode)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (IsValidGpParam(psGpMode) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_SetGPMode(psGpMode);

	McPacket_AddGPMode();
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
static SINT32	mask_gp
(
	UINT8	*pbMask,
	UINT32	dPadNo
)
{
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_INIT_INFO	sInitInfo;

	if (NULL == pbMask)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	if (IsValidMaskGp(*pbMask, dPadNo) == 0)
		return MCDRV_ERROR_ARGUMENT;

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	if (((dPadNo == 0) && (sInitInfo.bPa0Func != MCDRV_PA_GPIO))
	     || ((dPadNo == 1) && (sInitInfo.bPa1Func != MCDRV_PA_GPIO))
	     || ((dPadNo == 2) && (sInitInfo.bPa2Func != MCDRV_PA_GPIO)))
		return MCDRV_SUCCESS;

	McResCtrl_SetGPMask(*pbMask, dPadNo);

	McPacket_AddGPMask(dPadNo);
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
static SINT32	getset_gp
(
	UINT8	*pbGpio,
	UINT32	dPadNo
)
{
	UINT8		bSlaveAddr;
	UINT8		abData[2];
	UINT8		bRegData;
	enum MCDRV_STATE	eState	= McResCtrl_GetState();
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_GP_MODE	sGPMode;

	if (NULL == pbGpio)
		return MCDRV_ERROR_ARGUMENT;

	if (eMCDRV_STATE_READY != eState)
		return MCDRV_ERROR_STATE;

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetGPMode(&sGPMode);

	if ((dPadNo != MCDRV_GP_PAD0)
	&& (dPadNo != MCDRV_GP_PAD1)
	&& (dPadNo != MCDRV_GP_PAD2))
		return MCDRV_ERROR_ARGUMENT;

	if (((dPadNo == MCDRV_GP_PAD0)
		&& (sInitInfo.bPa0Func != MCDRV_PA_GPIO))
	|| ((dPadNo == MCDRV_GP_PAD1)
		&& (sInitInfo.bPa1Func != MCDRV_PA_GPIO))
	|| ((dPadNo == MCDRV_GP_PAD2)
		&& (sInitInfo.bPa2Func != MCDRV_PA_GPIO)))
		return MCDRV_ERROR;

	if (sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN) {
		bSlaveAddr	=
			McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abData[0]	= MCI_A_REG_A<<1;
		abData[1]	= (UINT8)(MCI_PA0+dPadNo);
		McSrv_WriteReg(bSlaveAddr, abData, 2);
		bRegData	= McSrv_ReadReg(bSlaveAddr, MCI_A_REG_D);
		*pbGpio		= (UINT8)((bRegData & MCB_PA0_DATA) >> 4);
	} else {
		if (sGPMode.abGpHost[dPadNo] != MCDRV_GPHOST_CDSP) {
			if ((*pbGpio != MCDRV_GP_LOW)
			&& (*pbGpio != MCDRV_GP_HIGH))
				return MCDRV_ERROR_ARGUMENT;

			McResCtrl_SetGPPad(*pbGpio, dPadNo);
			McPacket_AddGPSet(dPadNo);
			return McDevIf_ExecutePacket();
		}
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	irq_proc
 *
 *	Description:
 *			Clear interrupt flag.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static SINT32	irq_proc
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bEIRQ	= 0;
	UINT8	bSlaveAddrD, bSlaveAddrA;
	UINT8	bReg, bSPlugDet, bPlugDet;
	struct MCDRV_HSDET_INFO	sHSDetInfo;
	UINT8	abData[4];
	UINT32	dFlg_DET	= 0;
	UINT8	bFlg_DLYKEY	= 0,
		bFlg_KEY	= 0;
	struct MCDRV_HSDET_RES	sHSDetRes;
	UINT8	bSENSEFIN;
	struct MCDRV_AEC_INFO	sAecInfo;
	struct MCDRV_PATH_INFO	sCurPathInfo;
	static UINT8	bOP_DAC;
	static UINT8	bHpDet;
	UINT32	dCycles, dInterval, dTimeOut;
	UINT8	bAP;

	bSlaveAddrD	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
	bSlaveAddrA	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);

	McResCtrl_GetHSDet(&sHSDetInfo, NULL);

	bEIRQ	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_IRQ);
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_EIRQSENSE);
	if ((bReg & MCB_EIRQSENSE) == 0) {
		McSrv_Lock();
		bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
		if ((bAP&MCB_AP_LDOD) != 0) {
			;
			machdep_PreLDODStart();
		}
	} else {/*	Sensing	*/
		/*	Disable	*/
		bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);
		if ((bAP&MCB_AP_LDOD) != 0) {
			;
			machdep_PreLDODStart();
		}
		abData[0]	= MCI_IRQR<<1;
		abData[1]	= 0;
		McSrv_WriteReg(bSlaveAddrD, abData, 2);

		abData[0]	= MCI_CD_REG_A<<1;
		abData[1]	= MCI_SSENSEFIN;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bSENSEFIN	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		if ((bSENSEFIN&MCB_SSENSEFIN) != 0) {
			abData[1]	= MCI_EIRQSENSE;
			abData[2]	= MCI_CD_REG_D<<1;
			abData[3]	= 0;
			McSrv_WriteReg(bSlaveAddrA, abData, 4);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_CD,
						MCI_EIRQSENSE, 0);
			abData[1]	= MCI_SSENSEFIN;
			abData[3]	= bReg;
			McSrv_WriteReg(bSlaveAddrA, abData, 4);
		} else {
			/*	Enable	*/
			abData[0]	= MCI_IRQR<<1;
			abData[1]	= MCB_EIRQR;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			return MCDRV_SUCCESS;
		}

		/*	PLUGDET, PLUGUNDETDB, PLUGDETDB	*/
		abData[0]	= MCI_CD_REG_A<<1;
		abData[1]	= MCI_PLUGDET;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		dFlg_DET	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		/*	clear	*/
		abData[2]	= MCI_CD_REG_D<<1;
		abData[3]	= (UINT8)dFlg_DET;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		/*	set reference	*/
		abData[1]	= MCI_PLUGDET_DB;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		McResCtrl_SetPlugDetDB(bReg);
		abData[1]	= MCI_RPLUGDET;
		abData[3]	= ((UINT8)dFlg_DET&MCB_PLUGDET) | bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);
		dFlg_DET	= abData[3];

		abData[1]	= MCI_MICDET;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		bFlg_KEY	= bReg &
				(sHSDetInfo.bEnMicDet == MCDRV_MICDET_ENABLE ?
							MCB_MICDET : 0);
		abData[1]	= MCI_RMICDET;
		abData[3]	= bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		abData[1]	= MCI_SMICDET;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		abData[3]	= bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		sHSDetRes.bKeyCnt0	= 0;
		sHSDetRes.bKeyCnt1	= 0;
		sHSDetRes.bKeyCnt2	= 0;

		abData[0]	= MCI_E_REG_A<<1;
		abData[1]	= MCI_PLUG_REV;
		McSrv_WriteReg(bSlaveAddrD, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_E_REG_D);
		sHSDetRes.bPlugRev	= bReg>>7;
		sHSDetRes.bHpImpClass	= bReg&0x07;
		abData[1]	= MCI_HPIMP_15_8;
		McSrv_WriteReg(bSlaveAddrD, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_E_REG_D);
		sHSDetRes.wHpImp	= bReg<<8;
		abData[1]	= MCI_HPIMP_7_0;
		McSrv_WriteReg(bSlaveAddrD, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_E_REG_D);
		sHSDetRes.wHpImp	|= bReg;

		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_LPF_THR);
		bReg	&= (MCB_OSF1_MN|MCB_OSF0_MN|MCB_OSF1_ENB|MCB_OSF0_ENB);
		McResCtrl_GetAecInfo(&sAecInfo);
		bReg	|= (UINT8)(sAecInfo.sOutput.bLpf_Post_Thru[1]<<7);
		bReg	|= (UINT8)(sAecInfo.sOutput.bLpf_Post_Thru[0]<<6);
		bReg	|= (UINT8)(sAecInfo.sOutput.bLpf_Pre_Thru[1]<<5);
		bReg	|= (UINT8)(sAecInfo.sOutput.bLpf_Pre_Thru[0]<<4);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_LPF_THR,
				bReg);
		bReg	= (UINT8)(sAecInfo.sOutput.bDcl_OnOff[1]<<7)
			| (UINT8)(sAecInfo.sOutput.bDcl_Gain[1]<<4)
			| (UINT8)(sAecInfo.sOutput.bDcl_OnOff[0]<<3)
			| sAecInfo.sOutput.bDcl_Gain[0];
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DCL_GAIN,/*18*/
				bReg);
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)19,
					bOP_DAC);
		}
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_HPDETVOL,
				bHpDet);
		bReg	= (sAecInfo.sE2.bE2_Da_Sel<<3)
						| sAecInfo.sE2.bE2_Ad_Sel;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_E2_SEL,
				bReg);
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)78,
					T_CPMODE_IMPSENSE_AFTER);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)87,
					0);
			abData[0]	= MCI_ANA_REG_A<<1;
			abData[1]	= 78;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_ANA_REG_D);
		} else {
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_ANA
					| (UINT32)87,
					0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)31,
					0);
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)30,
					0);
		}

		McResCtrl_GetPathInfoVirtual(&sCurPathInfo);
		sdRet	= set_path(&sCurPathInfo);
		if ((bAP&MCB_AP_LDOD) != 0) {
			;
			machdep_PostLDODStart();
		}
		McSrv_Unlock();
		if (sHSDetInfo.cbfunc != NULL) {
			if ((dFlg_DET & MCB_SPLUGUNDET_DB) != 0) {
				;
				dFlg_DET	= MCB_PLUGUNDET_DB;
			} else {
				dFlg_DET	|=
						MCDRV_HSDET_EVT_SENSEFIN_FLAG;
			}
			(*sHSDetInfo.cbfunc)(dFlg_DET
						|((UINT32)bFlg_KEY<<16),
						&sHSDetRes);
		}
		/*	Enable IRQ	*/
		McSrv_Lock();
		bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_AP);
		if ((bAP&MCB_AP_LDOD) != 0) {
			;
			machdep_PreLDODStart();
		}
		abData[0]	= MCI_CD_REG_A<<1;
		abData[1]	= MCI_IRQHS;
		abData[2]	= MCI_CD_REG_D<<1;
		abData[3]	= MCB_EIRQHS;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_CD,
					MCI_IRQHS, MCB_EIRQHS);
		abData[0]	= MCI_IRQR<<1;
		abData[1]	= MCB_EIRQR;
		McSrv_WriteReg(bSlaveAddrD, abData, 2);
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF,
						MCI_RST_A);
		if (bReg == 0) {
			abData[0]	= MCI_IRQ<<1;
			abData[1]	= MCB_EIRQ;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_IF,
						MCI_IRQ, MCB_EIRQ);
		}
		if ((bAP&MCB_AP_LDOD) != 0) {
			;
			machdep_PostLDODStart();
		}
		McSrv_Unlock();
		return MCDRV_SUCCESS;
	}

	if (eMCDRV_STATE_READY != McResCtrl_GetState()) {
		sdRet	= MCDRV_ERROR_STATE;
		goto exit;
	}

	if ((bEIRQ&MCB_EIRQ) != 0) {
		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_EDSP);
		if (bReg != 0) {
			/*	Disable IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EEDSP,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;

			if ((bReg & MCB_E2DSP_STA) != 0) {
				;
				McEdsp_IrqProc();
			}
			/*	Clear IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP,
					bReg);
			/*	Enable IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EEDSP,
					MCB_EE2DSP);
		}

		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_CDSP);
		if (bReg != 0) {
			/*	Disable IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			McCdsp_IrqProc();
			/*	Clear IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP,
					bReg);
			/*	Enable IRQ	*/
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

		bReg	= McSrv_ReadReg(bSlaveAddrD, MCI_IRSERR);
		if (bReg != 0) {
			/*	Disable IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR,
					0);
			sdRet	= McDevIf_ExecutePacket();
			if (sdRet != MCDRV_SUCCESS)
				goto exit;
			McFdsp_IrqProc();
			/*	Clear IRQ	*/
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR,
					bReg);
			/*	Enable IRQ	*/
			bReg	= MCB_IESERR
				| MCB_IEAMTBEG
				| MCB_IEAMTEND
				| MCB_IEFW;
			McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR,
					bReg);
		}
		sdRet	= McDevIf_ExecutePacket();
		if (sdRet != MCDRV_SUCCESS)
			goto exit;
	}

	abData[0]	= MCI_CD_REG_A<<1;
	abData[1]	= MCI_IRQHS;
	McSrv_WriteReg(bSlaveAddrA, abData, 2);
	bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
	if (bReg == (MCB_EIRQHS|MCB_IRQHS)) {
		/*	Disable EIRQHS	*/
		abData[2]	= MCI_CD_REG_D<<1;
		abData[3]	= 0;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_CD,
					MCI_IRQHS, 0);

		/*	PLUGDET, SPLUGUNDETDB, SPLUGDETDB	*/
		abData[1]	= MCI_PLUGDET;/*10*/
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bSPlugDet	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		dFlg_DET	= (bSPlugDet&MCB_PLUGDET);

		abData[1]	= MCI_PLUGDET_DB;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bPlugDet	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		dFlg_DET	|= bPlugDet;

		if ((dFlg_DET&MCB_PLUGUNDET_DB) != 0) {
			;
			McResCtrl_SetPlugDetDB(0);
		}
		if ((sHSDetInfo.bEnPlugDetDb & MCDRV_PLUGDETDB_DET_ENABLE) != 0
		) {
			if (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H) {
				abData[1]	= MCI_RPLUGDET;
				McSrv_WriteReg(bSlaveAddrA, abData, 2);
				bReg	= (UINT8)~McSrv_ReadReg(bSlaveAddrA,
							MCI_CD_REG_D);
				bReg	&= bPlugDet;
			} else {
				bReg	= bSPlugDet;
			}
			if ((bReg & MCB_SPLUGDET_DB) != 0) {
				McResCtrl_SetPlugDetDB(bReg);
				if (McDevProf_GetDevId() !=
							eMCDRV_DEV_ID_81_92H) {
					bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_ANA,
						MCI_AP);
					bReg	&= (UINT8)~MCB_AP_BGR;
					McDevIf_AddPacket(
						MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_AP,
						bReg);
				}
				bReg	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_ANA,
						MCI_HIZ);
				bReg	|= (MCB_HPR_HIZ|MCB_HPL_HIZ);
				McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_ANA
						| (UINT32)MCI_HIZ,
						bReg);
				McResCtrl_GetPathInfoVirtual(&sCurPathInfo);
				sCurPathInfo.asBias[3].dSrcOnOff	|=
							MCDRV_ASRC_MIC4_ON;
				sdRet	= set_path(&sCurPathInfo);
				if (sdRet != MCDRV_SUCCESS) {
					/*	Enable IRQ	*/
					abData[1]	= MCI_IRQHS;
					abData[3]	= MCB_EIRQHS;
					McSrv_WriteReg(bSlaveAddrA, abData, 4);
					McResCtrl_SetRegVal(
						MCDRV_PACKET_REGTYPE_CD,
						MCI_IRQHS, MCB_EIRQHS);
					goto exit;
				}
				if ((sHSDetInfo.bEnDlyKeyOff !=
							MCDRV_KEYEN_D_D_D)
				|| (sHSDetInfo.bEnDlyKeyOn !=
							MCDRV_KEYEN_D_D_D)
				|| (sHSDetInfo.bEnMicDet !=
							MCDRV_MICDET_DISABLE)
				|| (sHSDetInfo.bEnKeyOff !=
							MCDRV_KEYEN_D_D_D)
				|| (sHSDetInfo.bEnKeyOn !=
							MCDRV_KEYEN_D_D_D)) {
					;
					McPacket_AddMKDetEnable(0);
				}
				sdRet	= McDevIf_ExecutePacket();
				if (sdRet != MCDRV_SUCCESS) {
					/*	Enable IRQ	*/
					abData[1]	= MCI_IRQHS;
					abData[3]	= MCB_EIRQHS;
					McSrv_WriteReg(bSlaveAddrA, abData, 4);
					McResCtrl_SetRegVal(
						MCDRV_PACKET_REGTYPE_CD,
						MCI_IRQHS, MCB_EIRQHS);
					goto exit;
				}
				if (sHSDetInfo.bSgnlNum != MCDRV_SGNLNUM_NONE
				) {
					bHpDet	= McResCtrl_GetRegVal(
						MCDRV_PACKET_REGTYPE_ANA,
						MCI_HPDETVOL);
					sdRet	= BeginImpSense(&bOP_DAC);
					if (sdRet == MCDRV_SUCCESS) {
						if ((bAP&MCB_AP_LDOD) != 0) {
							;
						machdep_PostLDODStart();
						}
						return sdRet;
					} else {
						/*	Enable IRQ	*/
						abData[1]	= MCI_IRQHS;
						abData[3]	= MCB_EIRQHS;
						McSrv_WriteReg(bSlaveAddrA,
								abData, 4);
						McResCtrl_SetRegVal(
							MCDRV_PACKET_REGTYPE_CD,
							MCI_IRQHS, MCB_EIRQHS);
						goto exit;
					}
				}
			}
		}

		/*	clear	*/
		abData[1]	= MCI_PLUGDET;/*10*/
		abData[3]	= bSPlugDet;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		/*	set reference	*/
		abData[1]	= MCI_RPLUGDET;
		abData[3]	= (UINT8)dFlg_DET;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		/*	DLYKEYON, DLYKEYOFF	*/
		abData[1]	= MCI_SDLYKEY;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg		= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		bFlg_DLYKEY	= bReg;
		/*	clear	*/
		abData[1]	= MCI_SDLYKEY;
		abData[3]	= bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		/*	MICDET, KEYON, KEYOFF	*/
		abData[1]	= MCI_SMICDET;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg		= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		bFlg_KEY	= bReg & (UINT8)~MCB_SMICDET;
		/*	clear	*/
		abData[1]	= MCI_SMICDET;
		abData[3]	= bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		/*	set reference	*/
		abData[1]	= MCI_MICDET;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		if (sHSDetInfo.bSgnlNum == MCDRV_SGNLNUM_NONE) {
			if ((dFlg_DET & MCB_PLUGDET_DB) != 0) {
				dCycles	= 0;
				switch (sHSDetInfo.bSperiod) {
				default:
				case	MCDRV_SPERIOD_244:
					dInterval	= 244;
					break;
				case	MCDRV_SPERIOD_488:
					dInterval	= 488;
					break;
				case	MCDRV_SPERIOD_977:
					dInterval	= 977;
					break;
				case	MCDRV_SPERIOD_1953:
					dInterval	= 1953;
					break;
				case	MCDRV_SPERIOD_3906:
					dInterval	= 3906;
					break;
				case	MCDRV_SPERIOD_7813:
					dInterval	= 7813;
					break;
				case	MCDRV_SPERIOD_15625:
					dInterval	= 15625;
					break;
				case	MCDRV_SPERIOD_31250:
					dInterval	= 31250;
					break;
				}
				switch (sHSDetInfo.bDbncNumMic) {
				default:
				case	MCDRV_DBNC_NUM_2:
					dTimeOut	= 1;
					break;
				case	MCDRV_DBNC_NUM_3:
					dTimeOut	= 4;
					break;
				case	MCDRV_DBNC_NUM_4:
					dTimeOut	= 5;
					break;
				case	MCDRV_DBNC_NUM_7:
					dTimeOut	= 8;
					break;
				}

				while (dCycles < dTimeOut) {
					bReg	= McSrv_ReadReg(bSlaveAddrA,
								MCI_CD_REG_D);
					if ((bReg & (MCB_MICDET|0x07)) != 0) {
						;
						break;
					}
					McSrv_Sleep(dInterval);
					dCycles++;
				}
			}
		} else {
			bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_CD_REG_D);
		}
		bFlg_KEY	|= bReg &
				(sHSDetInfo.bEnMicDet == MCDRV_MICDET_ENABLE ?
							MCB_MICDET : 0);
		abData[1]	= MCI_RMICDET;
		abData[3]	= bReg;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);

		if (sHSDetInfo.cbfunc != NULL) {
			/*	KeyCnt0	*/
			abData[1]	= MCI_KEYCNTCLR0;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			sHSDetRes.bKeyCnt0	= McSrv_ReadReg(bSlaveAddrA,
							MCI_CD_REG_D);
			abData[1]	= MCI_KEYCNTCLR0;
			abData[3]	= MCB_KEYCNTCLR0;
			McSrv_WriteReg(bSlaveAddrA, abData, 4);

			/*	KeyCnt1	*/
			abData[1]	= MCI_KEYCNTCLR1;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			sHSDetRes.bKeyCnt1	= McSrv_ReadReg(bSlaveAddrA,
							MCI_CD_REG_D);
			abData[1]	= MCI_KEYCNTCLR1;
			abData[3]	= MCB_KEYCNTCLR1;
			McSrv_WriteReg(bSlaveAddrA, abData, 4);

			/*	KeyCnt2	*/
			abData[1]	= MCI_KEYCNTCLR2;
			McSrv_WriteReg(bSlaveAddrA, abData, 2);
			sHSDetRes.bKeyCnt2	= McSrv_ReadReg(bSlaveAddrA,
							MCI_CD_REG_D);
			abData[1]	= MCI_KEYCNTCLR2;
			abData[3]	= MCB_KEYCNTCLR2;
			McSrv_WriteReg(bSlaveAddrA, abData, 4);

			abData[0]	= MCI_IRQ<<1;
			abData[1]	= 0;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_IRQ,
						0);
			abData[0]	= MCI_IRQR<<1;
			abData[1]	= 0;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			McSrv_Unlock();
			(*sHSDetInfo.cbfunc)(
				dFlg_DET
				|((UINT32)bFlg_DLYKEY<<8)
				|((UINT32)bFlg_KEY<<16),
				&sHSDetRes);
			McSrv_Lock();
			bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA,
								MCI_AP);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			abData[0]	= MCI_IRQR<<1;
			abData[1]	= MCB_EIRQR;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
		}
		/*	Enable IRQ	*/
		abData[0]	= MCI_CD_REG_A<<1;
		abData[1]	= MCI_IRQHS;
		abData[2]	= MCI_CD_REG_D<<1;
		abData[3]	= MCB_EIRQHS;
		McSrv_WriteReg(bSlaveAddrA, abData, 4);
		McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_CD,
					MCI_IRQHS, MCB_EIRQHS);
	}

exit:
	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_RST_A);
	if (bReg == 0) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_IF,
								MCI_IRQ);
		if (bReg != MCB_EIRQ) {
			abData[0]	= MCI_IRQ<<1;
			abData[1]	= MCB_EIRQ;
			McSrv_WriteReg(bSlaveAddrD, abData, 2);
			McResCtrl_SetRegVal(MCDRV_PACKET_REGTYPE_IF, MCI_IRQ,
								MCB_EIRQ);
		}
	}

	if ((bAP&MCB_AP_LDOD) != 0) {
		;
		machdep_PostLDODStart();
	}
	McSrv_Unlock();
	return sdRet;
}

#ifndef MCDRV_SKIP_IMPSENSE
/****************************************************************************
 *	BeginImpSense
 *
 *	Description:
 *			Begin Imp Sensing.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	BeginImpSense
(
	UINT8	*bOP_DAC
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;
	struct MCDRV_HSDET_INFO	sHSDetInfo;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_INIT2_INFO	sInit2Info;
	UINT8	bSlaveAddrA;
	UINT8	abData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("BeginImpSense");
#endif

	bSlaveAddrA	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
	McResCtrl_GetCurPowerInfo(&sPowerInfo);
	sPowerInfo.bDigital	&= ~(MCDRV_POWINFO_D_PM_CLK_PD
					|MCDRV_POWINFO_D_PE_CLK_PD
					|MCDRV_POWINFO_D_PLL_PD);
	sPowerInfo.abAnalog[0]	&=
			(UINT8)~(MCB_AP_LDOA|MCB_AP_LDOD|MCB_AP_BGR|MCB_AP_VR);

	/*	power up	*/
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= 0;
	sPowerUpdate.abAnalog[2]	= 0;
	sPowerUpdate.abAnalog[3]	= 0;
	sPowerUpdate.abAnalog[4]	= 0;
	sdRet	= McPacket_AddPowerUp(&sPowerInfo, &sPowerUpdate);
	if (sdRet != MCDRV_SUCCESS)
		goto exit;
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_IRQ,
			0);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_IRQHS,
			0);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E1DSP_CTRL),
			(UINT8)0x00);

	McPacket_AddDac0Mute();
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO0_VOL0),
			MCDRV_REG_MUTE);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_MA
			| (UINT32)MCI_DAO0_VOL1),
			MCDRV_REG_MUTE);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_LPF_THR);
	bReg	|= (MCB_OSF0_ENB|MCB_LPF0_PST_THR|MCB_LPF0_PRE_THR);
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_LPF_THR,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_CD, MCI_DP);
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	&= (UINT8)~(MCB_DP_ADC|MCB_DP_DAC0|
				MCB_DP_PDMCK|MCB_DP_PDMADC|MCB_DP_PDMDAC);
	} else {
		bReg	&= (UINT8)~(MCB_DP_ADC|MCB_DP_DAC1|MCB_DP_DAC0|
				MCB_DP_PDMCK|MCB_DP_PDMADC|MCB_DP_PDMDAC);
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_DP,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
					MCI_DCL_GAIN);
	bReg	|= MCB_DCL0_OFF;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_DCL_GAIN,
			bReg);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF0_FLT_TYPE);
		bReg	|= MCB_DSF0ENB;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF0_FLT_TYPE,
				bReg);
	} else {
		bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E,
							MCI_DSF2_FLT_TYPE);
		bReg	|= MCB_DSF2ENB;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_E
				| (UINT32)MCI_DSF2_FLT_TYPE,
				bReg);
	}

	McResCtrl_GetHSDet(&sHSDetInfo, NULL);
	bReg	= (sHSDetInfo.bSgnlPeak<<4)
		| (sHSDetInfo.bSgnlNum<<2)
		| sHSDetInfo.bSgnlPeriod;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_IMPSEL,
			bReg);

	bReg	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_E, MCI_E2_SEL);
	bReg	&= (UINT8)~0x0C;
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E2_SEL,
			0);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_IRQR,
			MCB_EIRQR);

	McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_CD
			| (UINT32)MCI_EIRQSENSE,
			MCB_EIRQSENSE);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		*bOP_DAC	= McResCtrl_GetRegVal(
					MCDRV_PACKET_REGTYPE_ANA, 19);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)19,
				0);
	}

	McResCtrl_GetInitInfo(&sInitInfo, &sInit2Info);
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		;
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_HPDETVOL,
				0x70);
	} else if (McDevProf_GetDevId() == eMCDRV_DEV_ID_81_91H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_HPDETVOL,
				0x6B);
	} else {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)MCI_HPDETVOL,
				sInit2Info.bOption[9]);
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)78,
				T_CPMODE_IMPSENSE_BEFORE);
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_ANA
				| (UINT32)87,
				0x11);
	} else {
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
				| (UINT32)87,
				sInit2Info.bOption[10]);
	}
	sdRet	= McDevIf_ExecutePacket();
	if (sdRet != MCDRV_SUCCESS)
		goto exit;
	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		abData[0]	= MCI_ANA_REG_A<<1;
		abData[1]	= 78;
		McSrv_WriteReg(bSlaveAddrA, abData, 2);
		bReg	= McSrv_ReadReg(bSlaveAddrA, MCI_ANA_REG_D);
		if (bReg != T_CPMODE_IMPSENSE_BEFORE) {
			sdRet	= MCDRV_ERROR;
			goto exit;
		}
	}

	bReg	= E1COMMAND_IMP_SENSE;
	if (sInitInfo.bGndDet == MCDRV_GNDDET_ON)
		bReg	|= E1COMMAND_GND_DET;
	if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_R_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_L_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC0_R_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_ADC1_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_L_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM0_R_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_L_ON) != 0)
	|| (McResCtrl_IsD2SrcUsed(MCDRV_D2SRC_PDM1_R_ON) != 0))
		bReg	|= E1COMMAND_ADDITIONAL;
	else if ((McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_L_ON) != 0)
	     || (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_R_ON) != 0))
		bReg	|= (E1COMMAND_ADDITIONAL|E1COMMAND_PD);

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		McDevIf_AddPacket(MCDRV_PACKET_TYPE_TIMWAIT
				| MCDRV_DAC_MUTE_WAIT_TIME,
				0);
	} else {
		bReg	|= E1COMMAND_WAIT;
	}
	McDevIf_AddPacket(MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_E
			| (UINT32)MCI_E1COMMAND,
			bReg);

	sdRet	= McDevIf_ExecutePacket();

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("BeginImpSense", &sdRet);
#endif
	return sdRet;
}
#endif

/****************************************************************************
 *	IsLDOAOn
 *
 *	Description:
 *			Is LDOA used.
 *	Arguments:
 *			none
 *	Return:
 *			0:unused, 1:used
 *
 ****************************************************************************/
static	UINT8	IsLDOAOn
(
	void
)
{
	UINT8	bRet	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsLDOAOn");
#endif
	if ((McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADC0, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_ADC1, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_BIAS, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_BIAS, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_BIAS, eMCDRV_DST_CH2) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_BIAS, eMCDRV_DST_CH3) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC0_R_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_L_ON) != 0)
	|| (McResCtrl_IsASrcUsed(MCDRV_ASRC_DAC1_R_ON) != 0))
		bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsLDOAOn", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidInitParam
 *
 *	Description:
 *			check init parameters.
 *	Arguments:
 *			psInitInfo	initialize information
 *			psInit2Info	initialize information
 *	Return:
 *			0:Invalid
 *			othre:Valid
 *
 ****************************************************************************/
static	UINT8	IsValidInitParam
(
	const struct MCDRV_INIT_INFO	*psInitInfo,
	const struct MCDRV_INIT2_INFO	*psInit2Info
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidInitParam");
#endif


	if ((MCDRV_CKSEL_CMOS_CMOS != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO_TCXO != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_CMOS_TCXO != psInitInfo->bCkSel)
	&& (MCDRV_CKSEL_TCXO_CMOS != psInitInfo->bCkSel))
		bRet	= 0;

	if ((MCDRV_CKINPUT_CLKI0_CLKI1 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI0_RTCK != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI0_SBCK != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI1_CLKI0 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI1_RTCK != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI1_SBCK != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_RTC_CLKI0 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_RTC_CLKI1 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_RTC_SBCK != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_SBCK_CLKI0 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_SBCK_CLKI1 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_SBCK_RTC != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI0_CLKI0 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_CLKI1_CLKI1 != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_RTC_RTC != psInitInfo->bCkInput)
	&& (MCDRV_CKINPUT_SBCK_SBCK != psInitInfo->bCkInput))
		bRet	= 0;

	if (psInitInfo->bPllModeA > 7)
		bRet	= 0;

	if (psInitInfo->bPllPrevDivA > 0x3F)
		bRet	= 0;

	if (psInitInfo->wPllFbDivA > 0x3FFF)
		bRet	= 0;

	if ((psInitInfo->bPllFreqA != MCDRV_PLLFREQ_73)
	&& (psInitInfo->bPllFreqA != MCDRV_PLLFREQ_147))
		bRet	= 0;

	if (psInitInfo->bPllModeB > 7)
		bRet	= 0;

	if (psInitInfo->bPllPrevDivB > 0x3F)
		bRet	= 0;

	if (psInitInfo->wPllFbDivB > 0x3FFF)
		bRet	= 0;

	if ((psInitInfo->bPllFreqB != MCDRV_PLLFREQ_73)
	&& (psInitInfo->bPllFreqB != MCDRV_PLLFREQ_147))
		bRet	= 0;

	if ((psInitInfo->bHsdetClk != MCDRV_HSDETCLK_RTC)
	&& (psInitInfo->bHsdetClk != MCDRV_HSDETCLK_OSC))
		bRet	= 0;

	if (((MCDRV_DAHIZ_LOW != psInitInfo->bDio0SdoHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio0SdoHiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDio1SdoHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio1SdoHiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDio2SdoHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio2SdoHiz)))
		bRet	= 0;

	if (((MCDRV_DAHIZ_LOW != psInitInfo->bDio0ClkHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio0ClkHiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDio1ClkHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio1ClkHiz))
	|| ((MCDRV_DAHIZ_LOW != psInitInfo->bDio2ClkHiz)
		&& (MCDRV_DAHIZ_HIZ != psInitInfo->bDio2ClkHiz)))
		bRet	= 0;

	if (((MCDRV_PCMHIZ_LOW != psInitInfo->bDio0PcmHiz)
		&& (MCDRV_PCMHIZ_HIZ != psInitInfo->bDio0PcmHiz))
	|| ((MCDRV_PCMHIZ_LOW != psInitInfo->bDio1PcmHiz)
		&& (MCDRV_PCMHIZ_HIZ != psInitInfo->bDio1PcmHiz))
	|| ((MCDRV_PCMHIZ_LOW != psInitInfo->bDio2PcmHiz)
		&& (MCDRV_PCMHIZ_HIZ != psInitInfo->bDio2PcmHiz)))
		bRet	= 0;

	if ((MCDRV_PA_GPIO != psInitInfo->bPa0Func)
	&& (MCDRV_PA_PDMCK != psInitInfo->bPa0Func))
		bRet	= 0;

	if ((MCDRV_PA_GPIO != psInitInfo->bPa1Func)
	&& (MCDRV_PA_PDMDI != psInitInfo->bPa1Func))
		bRet	= 0;

	if ((MCDRV_PA_GPIO != psInitInfo->bPa2Func)
	&& (MCDRV_PA_PDMDI != psInitInfo->bPa2Func))
		bRet	= 0;

	if ((MCDRV_POWMODE_FULL != psInitInfo->bPowerMode)
	&& (MCDRV_POWMODE_CDSPDEBUG != psInitInfo->bPowerMode))
		bRet	= 0;

	if ((psInitInfo->bMbSel1 != MCDRV_MBSEL_20)
	&& (psInitInfo->bMbSel1 != MCDRV_MBSEL_21)
	&& (psInitInfo->bMbSel1 != MCDRV_MBSEL_22)
	&& (psInitInfo->bMbSel1 != MCDRV_MBSEL_23))
		bRet	= 0;
	if ((psInitInfo->bMbSel2 != MCDRV_MBSEL_20)
	&& (psInitInfo->bMbSel2 != MCDRV_MBSEL_21)
	&& (psInitInfo->bMbSel2 != MCDRV_MBSEL_22)
	&& (psInitInfo->bMbSel2 != MCDRV_MBSEL_23))
		bRet	= 0;
	if ((psInitInfo->bMbSel3 != MCDRV_MBSEL_20)
	&& (psInitInfo->bMbSel3 != MCDRV_MBSEL_21)
	&& (psInitInfo->bMbSel3 != MCDRV_MBSEL_22)
	&& (psInitInfo->bMbSel3 != MCDRV_MBSEL_23))
		bRet	= 0;
	if ((psInitInfo->bMbSel4 != MCDRV_MBSEL_20)
	&& (psInitInfo->bMbSel4 != MCDRV_MBSEL_21)
	&& (psInitInfo->bMbSel4 != MCDRV_MBSEL_22)
	&& (psInitInfo->bMbSel4 != MCDRV_MBSEL_23))
		bRet	= 0;

	if ((psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0000)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0001)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0010)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0011)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0100)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0101)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0110)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_0111)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1000)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1001)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1010)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1011)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1100)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1101)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1110)
	&& (psInitInfo->bMbsDisch != MCDRV_MBSDISCH_1111))
		bRet	= 0;

	if ((psInitInfo->bNonClip != MCDRV_NONCLIP_OFF)
	&& (psInitInfo->bNonClip != MCDRV_NONCLIP_ON))
		bRet	= 0;

	if ((psInitInfo->bLineIn1Dif != MCDRV_LINE_STEREO)
	&& (psInitInfo->bLineIn1Dif != MCDRV_LINE_DIF))
		bRet	= 0;

	if ((psInitInfo->bLineOut1Dif != MCDRV_LINE_STEREO)
	&& (psInitInfo->bLineOut1Dif != MCDRV_LINE_DIF))
		bRet	= 0;

	if ((psInitInfo->bLineOut2Dif != MCDRV_LINE_STEREO)
	&& (psInitInfo->bLineOut2Dif != MCDRV_LINE_DIF))
		bRet	= 0;

	if ((psInitInfo->bMic1Sng != MCDRV_MIC_DIF)
	&& (psInitInfo->bMic1Sng != MCDRV_MIC_SINGLE))
		bRet	= 0;
	if ((psInitInfo->bMic2Sng != MCDRV_MIC_DIF)
	&& (psInitInfo->bMic2Sng != MCDRV_MIC_SINGLE))
		bRet	= 0;
	if ((psInitInfo->bMic3Sng != MCDRV_MIC_DIF)
	&& (psInitInfo->bMic3Sng != MCDRV_MIC_SINGLE))
		bRet	= 0;
	if ((psInitInfo->bMic4Sng != MCDRV_MIC_DIF)
	&& (psInitInfo->bMic4Sng != MCDRV_MIC_SINGLE))
		bRet	= 0;

	if ((psInitInfo->bZcLineOut1 != MCDRV_ZC_ON)
	&& (psInitInfo->bZcLineOut1 != MCDRV_ZC_OFF))
		bRet	= 0;

	if ((psInitInfo->bZcLineOut2 != MCDRV_ZC_ON)
	&& (psInitInfo->bZcLineOut2 != MCDRV_ZC_OFF))
		bRet	= 0;

	if ((psInitInfo->bZcRc != MCDRV_ZC_ON)
	&& (psInitInfo->bZcRc != MCDRV_ZC_OFF))
		bRet	= 0;

	if ((psInitInfo->bZcSp != MCDRV_ZC_ON)
	&& (psInitInfo->bZcSp != MCDRV_ZC_OFF))
		bRet	= 0;

	if ((psInitInfo->bZcHp != MCDRV_ZC_ON)
	&& (psInitInfo->bZcHp != MCDRV_ZC_OFF))
		bRet	= 0;

	if ((psInitInfo->bSvolLineOut1 != MCDRV_SVOL_OFF)
	&& (psInitInfo->bSvolLineOut1 != MCDRV_SVOL_ON))
		bRet	= 0;

	if ((psInitInfo->bSvolLineOut2 != MCDRV_SVOL_OFF)
	&& (psInitInfo->bSvolLineOut2 != MCDRV_SVOL_ON))
		bRet	= 0;

	if ((psInitInfo->bSvolRc != MCDRV_SVOL_OFF)
	&& (psInitInfo->bSvolRc != MCDRV_SVOL_ON))
		bRet	= 0;

	if ((psInitInfo->bSvolSp != MCDRV_SVOL_OFF)
	&& (psInitInfo->bSvolSp != MCDRV_SVOL_ON))
		bRet	= 0;

	if ((psInitInfo->bSvolHp != MCDRV_SVOL_OFF)
	&& (psInitInfo->bSvolHp != MCDRV_SVOL_ON))
		bRet	= 0;

	if ((psInitInfo->bRcHiz != MCDRV_RCIMP_FIXLOW)
	&& (psInitInfo->bRcHiz != MCDRV_RCIMP_WL))
		bRet	= 0;

	if ((psInitInfo->bSpHiz != MCDRV_WL_LOFF_ROFF)
	&& (psInitInfo->bSpHiz != MCDRV_WL_LON_ROFF)
	&& (psInitInfo->bSpHiz != MCDRV_WL_LOFF_RON)
	&& (psInitInfo->bSpHiz != MCDRV_WL_LON_RON))
		bRet	= 0;

	if ((psInitInfo->bHpHiz != MCDRV_IMP_LFIXLOW_RFIXLOW)
	&& (psInitInfo->bHpHiz != MCDRV_IMP_LWL_RFIXLOW)
	&& (psInitInfo->bHpHiz != MCDRV_IMP_LFIXLOW_RWL)
	&& (psInitInfo->bHpHiz != MCDRV_IMP_LWL_RWL))
		bRet	= 0;

	if ((psInitInfo->bLineOut1Hiz != MCDRV_IMP_LFIXLOW_RFIXLOW)
	&& (psInitInfo->bLineOut1Hiz != MCDRV_IMP_LWL_RFIXLOW)
	&& (psInitInfo->bLineOut1Hiz != MCDRV_IMP_LFIXLOW_RWL)
	&& (psInitInfo->bLineOut1Hiz != MCDRV_IMP_LWL_RWL))
		bRet	= 0;

	if ((psInitInfo->bLineOut2Hiz != MCDRV_IMP_LFIXLOW_RFIXLOW)
	&& (psInitInfo->bLineOut2Hiz != MCDRV_IMP_LWL_RFIXLOW)
	&& (psInitInfo->bLineOut2Hiz != MCDRV_IMP_LFIXLOW_RWL)
	&& (psInitInfo->bLineOut2Hiz != MCDRV_IMP_LWL_RWL))
		bRet	= 0;

	if ((psInitInfo->bCpMod != MCDRV_CPMOD_HI)
	&& (psInitInfo->bCpMod != MCDRV_CPMOD_MID))
		bRet	= 0;

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		if ((psInitInfo->bRbSel != MCDRV_RBSEL_2_2K)
		&& (psInitInfo->bRbSel != MCDRV_RBSEL_50))
			bRet	= 0;
	}

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		if ((psInitInfo->bPlugSel != MCDRV_PLUG_LRGM)
		&& (psInitInfo->bPlugSel != MCDRV_PLUG_LRMG))
			bRet	= 0;
	}

	if ((psInitInfo->bGndDet != MCDRV_GNDDET_OFF)
	&& (psInitInfo->bGndDet != MCDRV_GNDDET_ON))
		bRet	= 0;

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		if ((psInitInfo->bPpdRc != MCDRV_PPD_OFF)
		&& (psInitInfo->bPpdRc != MCDRV_PPD_ON))
			bRet	= 0;
	}

	if ((psInitInfo->bPpdSp != MCDRV_PPD_OFF)
	&& (psInitInfo->bPpdSp != MCDRV_PPD_ON))
		bRet	= 0;

	if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
		if ((psInitInfo->bPpdHp != MCDRV_PPD_OFF)
		&& (psInitInfo->bPpdHp != MCDRV_PPD_ON))
			bRet	= 0;

		if ((psInitInfo->bPpdLineOut1 != MCDRV_PPD_OFF)
		&& (psInitInfo->bPpdLineOut1 != MCDRV_PPD_ON))
			bRet	= 0;

		if ((psInitInfo->bPpdLineOut2 != MCDRV_PPD_OFF)
		&& (psInitInfo->bPpdLineOut2 != MCDRV_PPD_ON))
			bRet	= 0;
	}

	if ((psInitInfo->sWaitTime.dWaitTime[0] > MCDRV_MAX_WAIT_TIME)
	|| (psInitInfo->sWaitTime.dWaitTime[1] > MCDRV_MAX_WAIT_TIME)
	|| (psInitInfo->sWaitTime.dWaitTime[2] > MCDRV_MAX_WAIT_TIME)
	|| (psInitInfo->sWaitTime.dWaitTime[3] > MCDRV_MAX_WAIT_TIME)
	|| (psInitInfo->sWaitTime.dWaitTime[4] > MCDRV_MAX_WAIT_TIME))
		bRet	= 0;

	if ((McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H)
	&& (psInit2Info != NULL)) {
		if ((psInit2Info->bOption[0] != MCDRV_DOA_DRV_LOW)
		&& (psInit2Info->bOption[0] != MCDRV_DOA_DRV_HIGH)) {
			bRet	= 0;
			goto exit;
		}
		if ((psInit2Info->bOption[1] != MCDRV_SCKMSK_OFF)
		&& (psInit2Info->bOption[1] != MCDRV_SCKMSK_ON)) {
			bRet	= 0;
			goto exit;
		}
		if ((psInit2Info->bOption[2] != MCDRV_SPMN_8_9_10)
		&& (psInit2Info->bOption[2] != MCDRV_SPMN_5_6_7)
		&& (psInit2Info->bOption[2] != MCDRV_SPMN_4_5_6)
		&& (psInit2Info->bOption[2] != MCDRV_SPMN_OFF_9)
		&& (psInit2Info->bOption[2] != MCDRV_SPMN_OFF_6)) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[3] > 0x0F) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[4] > 0xF8) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[5] > 0xF8) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[6] > 0x31) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[7] > 0x7F) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[9] > 0x7F) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[10] > 0x11) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[11] > 0xF3) {
			bRet	= 0;
			goto exit;
		}
		if (psInit2Info->bOption[12] > 0x07) {
			bRet	= 0;
			goto exit;
		}
	}
exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidInitParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidReadRegParam
 *
 *	Description:
 *			check read reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static	UINT8	IsValidReadRegParam
(
	const struct MCDRV_REG_INFO	*psRegInfo
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidReadRegParam");
#endif


	if ((McResCtrl_GetRegAccess(psRegInfo) & eMCDRV_CAN_READ) ==
		eMCDRV_ACCESS_DENY) {
		bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidReadRegParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidWriteRegParam
 *
 *	Description:
 *			check write reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static	UINT8	IsValidWriteRegParam
(
	const struct MCDRV_REG_INFO	*psRegInfo
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidWriteRegParam");
#endif


	if ((McResCtrl_GetRegAccess(psRegInfo) & eMCDRV_CAN_WRITE) ==
		eMCDRV_ACCESS_DENY) {
		bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidWriteRegParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidClockSwParam
 *
 *	Description:
 *			check clock parameters.
 *	Arguments:
 *			psClockSwInfo	clock switch information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static	UINT8	IsValidClockSwParam
(
	const struct MCDRV_CLOCKSW_INFO	*psClockSwInfo
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidClockSwParam");
#endif


	if ((MCDRV_CLKSW_CLKA != psClockSwInfo->bClkSrc)
	&& (MCDRV_CLKSW_CLKB != psClockSwInfo->bClkSrc)) {
		bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidClockSwParam", &sdRet);
#endif

	return bRet;
}

/****************************************************************************
 *	MaskIrregularPath
 *
 *	Description:
 *			mask irregular path parameters.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MaskIrregularPath
(
	struct MCDRV_PATH_INFO	*psPathInfo
)
{
	UINT8	bCh;
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("MaskIrregularPath");
#endif

	for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++)
		psPathInfo->asMusicOut[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;

	for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++)
		psPathInfo->asExtOut[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;

	for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++)
		psPathInfo->asHifiOut[bCh].dSrcOnOff	&=
			(MCDRV_D1SRC_ADIF0_ON
			|MCDRV_D1SRC_ADIF0_OFF);

	for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++)
		psPathInfo->asVboxMixIn[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;

	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		psPathInfo->asAe0[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;
		psPathInfo->asAe1[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;
		psPathInfo->asAe2[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;
		psPathInfo->asAe3[bCh].dSrcOnOff	&=
			~MCDRV_D1SRC_HIFIIN_ON;
	}

	for (bCh = 0; bCh < VOICEOUT_PATH_CHANNELS; bCh++)
		psPathInfo->asVoiceOut[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_VBOXIOOUT_ON|MCDRV_D2SRC_VBOXIOOUT_OFF);

	for (bCh = 0; bCh < VBOXIOIN_PATH_CHANNELS; bCh++)
		psPathInfo->asVboxIoIn[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_VOICEIN_ON|MCDRV_D2SRC_VOICEIN_OFF);

	for (bCh = 0; bCh < VBOXHOSTIN_PATH_CHANNELS; bCh++)
		psPathInfo->asVboxHostIn[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_VOICEIN_ON|MCDRV_D2SRC_VOICEIN_OFF);

	for (bCh = 0; bCh < HOSTOUT_PATH_CHANNELS; bCh++)
		psPathInfo->asHostOut[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_VBOXHOSTOUT_ON
			|MCDRV_D2SRC_VBOXHOSTOUT_OFF);

	for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++)
		psPathInfo->asAdif0[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_ADC0_L_ON
			|MCDRV_D2SRC_ADC0_L_OFF
			|MCDRV_D2SRC_ADC0_R_ON
			|MCDRV_D2SRC_ADC0_R_OFF
			|MCDRV_D2SRC_ADC1_ON
			|MCDRV_D2SRC_ADC1_OFF
			|MCDRV_D2SRC_PDM0_L_ON
			|MCDRV_D2SRC_PDM0_L_OFF
			|MCDRV_D2SRC_PDM0_R_ON
			|MCDRV_D2SRC_PDM0_R_OFF
			|MCDRV_D2SRC_PDM1_L_ON
			|MCDRV_D2SRC_PDM1_L_OFF
			|MCDRV_D2SRC_PDM1_R_ON
			|MCDRV_D2SRC_PDM1_R_OFF);

	for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++)
		psPathInfo->asAdif1[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_ADC0_L_ON
			|MCDRV_D2SRC_ADC0_L_OFF
			|MCDRV_D2SRC_ADC0_R_ON
			|MCDRV_D2SRC_ADC0_R_OFF
			|MCDRV_D2SRC_ADC1_ON
			|MCDRV_D2SRC_ADC1_OFF
			|MCDRV_D2SRC_PDM0_L_ON
			|MCDRV_D2SRC_PDM0_L_OFF
			|MCDRV_D2SRC_PDM0_R_ON
			|MCDRV_D2SRC_PDM0_R_OFF
			|MCDRV_D2SRC_PDM1_L_ON
			|MCDRV_D2SRC_PDM1_L_OFF
			|MCDRV_D2SRC_PDM1_R_ON
			|MCDRV_D2SRC_PDM1_R_OFF);

	for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++)
		psPathInfo->asAdif2[bCh].dSrcOnOff	&=
			(MCDRV_D2SRC_ADC0_L_ON
			|MCDRV_D2SRC_ADC0_L_OFF
			|MCDRV_D2SRC_ADC0_R_ON
			|MCDRV_D2SRC_ADC0_R_OFF
			|MCDRV_D2SRC_ADC1_ON
			|MCDRV_D2SRC_ADC1_OFF
			|MCDRV_D2SRC_PDM0_L_ON
			|MCDRV_D2SRC_PDM0_L_OFF
			|MCDRV_D2SRC_PDM0_R_ON
			|MCDRV_D2SRC_PDM0_R_OFF
			|MCDRV_D2SRC_PDM1_L_ON
			|MCDRV_D2SRC_PDM1_L_OFF
			|MCDRV_D2SRC_PDM1_R_ON
			|MCDRV_D2SRC_PDM1_R_OFF
			|MCDRV_D2SRC_DAC0REF_ON
			|MCDRV_D2SRC_DAC0REF_OFF
			|MCDRV_D2SRC_DAC1REF_ON
			|MCDRV_D2SRC_DAC1REF_OFF);

	psPathInfo->asAdc0[0].dSrcOnOff	&=
		~(MCDRV_ASRC_DAC0_L_ON|MCDRV_ASRC_DAC0_R_ON
		|MCDRV_ASRC_DAC1_L_ON|MCDRV_ASRC_DAC1_R_ON
		|MCDRV_ASRC_LINEIN1_R_ON);
	psPathInfo->asAdc0[1].dSrcOnOff	&=
		~(MCDRV_ASRC_DAC0_L_ON|MCDRV_ASRC_DAC0_R_ON
		|MCDRV_ASRC_DAC1_L_ON|MCDRV_ASRC_DAC1_R_ON
		|MCDRV_ASRC_LINEIN1_L_ON);

	for (bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
		psPathInfo->asAdc1[bCh].dSrcOnOff	&=
			~(MCDRV_ASRC_DAC0_L_ON|MCDRV_ASRC_DAC0_R_ON
			|MCDRV_ASRC_DAC1_L_ON|MCDRV_ASRC_DAC1_R_ON
			|MCDRV_ASRC_LINEIN1_L_ON|MCDRV_ASRC_LINEIN1_R_ON);

	psPathInfo->asSp[0].dSrcOnOff	&=
		(MCDRV_ASRC_DAC1_L_ON
		|MCDRV_ASRC_DAC1_L_OFF);
	psPathInfo->asSp[1].dSrcOnOff	&=
		(MCDRV_ASRC_DAC1_R_ON
		|MCDRV_ASRC_DAC1_R_OFF);

	psPathInfo->asHp[0].dSrcOnOff	&=
		(MCDRV_ASRC_DAC0_L_ON
		|MCDRV_ASRC_DAC0_L_OFF);
	psPathInfo->asHp[1].dSrcOnOff	&=
		(MCDRV_ASRC_DAC0_R_ON
		|MCDRV_ASRC_DAC0_R_OFF);

	psPathInfo->asRc[0].dSrcOnOff	&=
		(MCDRV_ASRC_DAC0_L_ON
		|MCDRV_ASRC_DAC0_L_OFF);

	psPathInfo->asLout1[0].dSrcOnOff	&=
		(MCDRV_ASRC_DAC0_L_ON
		|MCDRV_ASRC_DAC0_L_OFF);
	psPathInfo->asLout1[1].dSrcOnOff	&=
		(MCDRV_ASRC_DAC0_R_ON
		|MCDRV_ASRC_DAC0_R_OFF);

	psPathInfo->asLout2[0].dSrcOnOff	&=
		(MCDRV_ASRC_DAC1_L_ON
		|MCDRV_ASRC_DAC1_L_OFF);
	psPathInfo->asLout2[1].dSrcOnOff	&=
		(MCDRV_ASRC_DAC1_R_ON
		|MCDRV_ASRC_DAC1_R_OFF);

	psPathInfo->asBias[0].dSrcOnOff	&=
		(MCDRV_ASRC_MIC1_ON
		|MCDRV_ASRC_MIC1_OFF);
	psPathInfo->asBias[1].dSrcOnOff	&=
		(MCDRV_ASRC_MIC2_ON
		|MCDRV_ASRC_MIC2_OFF);
	psPathInfo->asBias[2].dSrcOnOff	&=
		(MCDRV_ASRC_MIC3_ON
		|MCDRV_ASRC_MIC3_OFF);
	psPathInfo->asBias[3].dSrcOnOff	&=
		(MCDRV_ASRC_MIC4_ON
		|MCDRV_ASRC_MIC4_OFF);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("MaskIrregularPath", NULL);
#endif
}

/****************************************************************************
 *	IsValidDioParam
 *
 *	Description:
 *			validate digital IO parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			dUpdateInfo	update information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidDioParam
(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT32	dUpdateInfo
)
{
	UINT8	bRet	= 1;
	struct MCDRV_DIO_INFO	sDioInfo;
	UINT8	bRegVal;
	UINT8	bLPR0_start	= 0,
		bLPT0_start	= 0,
		bLPR1_start	= 0,
		bLPT1_start	= 0,
		bLPR2_start	= 0,
		bLPT2_start	= 0,
		bLPR3_start	= 0,
		bLPT3_start	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= MCDRV_SUCCESS;
	McDebugLog_FuncIn("IsValidDioParam");
#endif


	McResCtrl_GetDioInfo(&sDioInfo);

	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP0_START);
	if ((bRegVal & MCB_LPR0_START) != 0) {
		;
		bLPR0_start	= 1;
	}
	if ((bRegVal & MCB_LPT0_START) != 0) {
		;
		bLPT0_start	= 1;
	}
	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP1_START);
	if ((bRegVal & MCB_LPR1_START) != 0) {
		;
		bLPR1_start	= 1;
	}
	if ((bRegVal & MCB_LPT1_START) != 0) {
		;
		bLPT1_start	= 1;
	}
	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP2_START);
	if ((bRegVal & MCB_LPR2_START) != 0) {
		;
		bLPR2_start	= 1;
	}
	if ((bRegVal & MCB_LPT2_START) != 0) {
		;
		bLPT2_start	= 1;
	}
	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP3_START);
	if ((bRegVal & MCB_LPR3_START) != 0) {
		;
		bLPR3_start	= 1;
	}
	if ((bRegVal & MCB_LPT3_START) != 0) {
		;
		bLPT3_start	= 1;
	}

	if (((dUpdateInfo & MCDRV_MUSIC_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_MUSIC_DIR_UPDATE_FLAG) != 0UL)) {
		if (bLPR0_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_MUSIC_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_MUSIC_DIT_UPDATE_FLAG) != 0UL)) {
		if (bLPT0_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_EXT_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_EXT_DIR_UPDATE_FLAG) != 0UL)) {
		if (bLPR1_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_EXT_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_EXT_DIT_UPDATE_FLAG) != 0UL)) {
		if (bLPT1_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_VOICE_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_VOICE_DIR_UPDATE_FLAG) != 0UL)) {
		if (bLPR2_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_VOICE_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_VOICE_DIT_UPDATE_FLAG) != 0UL)) {
		if (bLPT2_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_HIFI_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_HIFI_DIR_UPDATE_FLAG) != 0UL)) {
		if (bLPR3_start != 0) {
			;
			bRet	= 0;
		}
	}
	if (((dUpdateInfo & MCDRV_HIFI_COM_UPDATE_FLAG) != 0UL)
	|| ((dUpdateInfo & MCDRV_HIFI_DIT_UPDATE_FLAG) != 0UL)) {
		if (bLPT3_start != 0) {
			;
			bRet	= 0;
		}
	}


	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_MUSIC_COM_UPDATE_FLAG) != 0UL)) {
		bRet	= CheckDIOCommon(psDioInfo, 0);
		if (bRet != 0) {
			;
			sDioInfo.asPortInfo[0].sDioCommon.bInterface	=
				psDioInfo->asPortInfo[0].sDioCommon.bInterface;
		}
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_EXT_COM_UPDATE_FLAG) != 0UL)) {
		bRet	= CheckDIOCommon(psDioInfo, 1);
		if (bRet != 0) {
			;
			sDioInfo.asPortInfo[1].sDioCommon.bInterface	=
				psDioInfo->asPortInfo[1].sDioCommon.bInterface;
		}
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_VOICE_COM_UPDATE_FLAG) != 0UL)) {
		bRet	= CheckDIOCommon(psDioInfo, 2);
		if (bRet != 0) {
			;
			sDioInfo.asPortInfo[2].sDioCommon.bInterface	=
				psDioInfo->asPortInfo[2].sDioCommon.bInterface;
		}
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_HIFI_COM_UPDATE_FLAG) != 0UL)) {
		bRet	= CheckDIOCommon(psDioInfo, 3);
		if (bRet != 0) {
			;
			sDioInfo.asPortInfo[3].sDioCommon.bInterface	=
				psDioInfo->asPortInfo[3].sDioCommon.bInterface;
		}
	}

	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_MUSIC_DIR_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIR(psDioInfo, 0);
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_EXT_DIR_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIR(psDioInfo, 1);
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_VOICE_DIR_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIR(psDioInfo, 2);
	}

	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_MUSIC_DIT_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIT(psDioInfo, 0);
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_EXT_DIT_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIT(psDioInfo, 1);
	}
	if ((bRet != 0)
	&& ((dUpdateInfo & MCDRV_VOICE_DIT_UPDATE_FLAG) != 0UL)) {
		;
		bRet	= CheckDIODIT(psDioInfo, 2);
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidDioParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidDioPathParam
 *
 *	Description:
 *			validate digital IO path parameters.
 *	Arguments:
 *			psDioPathInfo	digital IO path information
 *			dUpdateInfo	update information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidDioPathParam
(
	const struct MCDRV_DIOPATH_INFO	*psDioPathInfo,
	UINT32	dUpdateInfo
)
{
	UINT8	bRet	= 1;
	UINT8	bRegVal;
	struct MCDRV_DIOPATH_INFO	sDioPathInfo;
	UINT8	bLP0_start	= 0,
		bLP1_start	= 0,
		bLP2_start	= 0,
		bLP3_start	= 0;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= MCDRV_SUCCESS;
	McDebugLog_FuncIn("IsValidDioPathParam");
#endif


	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP0_START);
	if ((bRegVal & MCB_LPR0_START) != 0)
		bLP0_start	= 1;
	else if ((bRegVal & MCB_LPT0_START) != 0)
		bLP0_start	= 1;

	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP1_START);
	if ((bRegVal & MCB_LPR1_START) != 0)
		bLP1_start	= 1;
	else if ((bRegVal & MCB_LPT1_START) != 0)
		bLP1_start	= 1;

	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP2_START);
	if ((bRegVal & MCB_LPR2_START) != 0)
		bLP2_start	= 1;
	else if ((bRegVal & MCB_LPT2_START) != 0)
		bLP2_start	= 1;

	bRegVal	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MB, MCI_LP3_START);
	if ((bRegVal & MCB_LPR3_START) != 0)
		bLP3_start	= 1;
	else if ((bRegVal & MCB_LPT3_START) != 0)
		bLP3_start	= 1;

	McResCtrl_GetDioPathInfo(&sDioPathInfo);

	if ((dUpdateInfo & MCDRV_MUSICNUM_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->bMusicCh != MCDRV_MUSIC_2CH)
		&& (psDioPathInfo->bMusicCh != MCDRV_MUSIC_4CH)
		&& (psDioPathInfo->bMusicCh != MCDRV_MUSIC_6CH)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->bMusicCh != sDioPathInfo.bMusicCh) {
			if ((psDioPathInfo->bMusicCh == MCDRV_MUSIC_6CH)
			|| (sDioPathInfo.bMusicCh == MCDRV_MUSIC_6CH)) {
				if (bLP2_start != 0) {
					bRet	= 0;
					goto exit;
				}
			} else {
				if (bLP0_start != 0) {
					bRet	= 0;
					goto exit;
				}
#if 0
				if (bLP1_start != 0) {
					bRet	= 0;
					goto exit;
				}
#endif
			}
		}
	}
	if ((dUpdateInfo & MCDRV_PHYS0_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_DIO0)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_DIO1)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_DIO2)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_NONE)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_SLIM0)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_SLIM1)
		&& (psDioPathInfo->abPhysPort[0] != MCDRV_PHYSPORT_SLIM2)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->abPhysPort[0] !=
			sDioPathInfo.abPhysPort[0]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
		}
	}
	if ((dUpdateInfo & MCDRV_PHYS1_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_DIO0)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_DIO1)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_DIO2)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_NONE)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_SLIM0)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_SLIM1)
		&& (psDioPathInfo->abPhysPort[1] != MCDRV_PHYSPORT_SLIM2)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->abPhysPort[1] !=
			sDioPathInfo.abPhysPort[1]) {
			if (bLP1_start != 0) {
				bRet	= 0;
				goto exit;
			}
		}
	}
	if ((dUpdateInfo & MCDRV_PHYS2_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_DIO0)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_DIO1)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_DIO2)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_NONE)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_SLIM0)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_SLIM1)
		&& (psDioPathInfo->abPhysPort[2] != MCDRV_PHYSPORT_SLIM2)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->abPhysPort[2] !=
			sDioPathInfo.abPhysPort[2]) {
			if (bLP2_start != 0) {
				bRet	= 0;
				goto exit;
			}
		}
	}

	if ((dUpdateInfo & MCDRV_PHYS3_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_DIO0)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_DIO1)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_DIO2)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_NONE)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_SLIM0)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_SLIM1)
		&& (psDioPathInfo->abPhysPort[3] != MCDRV_PHYSPORT_SLIM2)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->abPhysPort[3] !=
			sDioPathInfo.abPhysPort[3]) {
			if (bLP3_start != 0) {
				bRet	= 0;
				goto exit;
			}
		}
	}
	if ((dUpdateInfo & MCDRV_DIR0SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicRSlot[0] != 0)
		&& (psDioPathInfo->abMusicRSlot[0] != 1)
		&& (psDioPathInfo->abMusicRSlot[0] != 2)
		&& (psDioPathInfo->abMusicRSlot[0] != 3)) {
			bRet	= 0;
			goto exit;
		} else if (psDioPathInfo->abMusicRSlot[0] !=
			sDioPathInfo.abMusicRSlot[0]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
		}
	}
	if ((dUpdateInfo & MCDRV_DIR1SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicRSlot[1] != 0)
		&& (psDioPathInfo->abMusicRSlot[1] != 1)
		&& (psDioPathInfo->abMusicRSlot[1] != 2)
		&& (psDioPathInfo->abMusicRSlot[1] != 3)) {
			bRet	= 0;
		} else if (psDioPathInfo->abMusicRSlot[1] !=
			sDioPathInfo.abMusicRSlot[1]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
			/*if (bLP1_start != 0) {
				bRet	= 0;
			}*/
		}
	}
	if ((dUpdateInfo & MCDRV_DIR2SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicRSlot[2] != 0)
		&& (psDioPathInfo->abMusicRSlot[2] != 1)
		&& (psDioPathInfo->abMusicRSlot[2] != 2)
		&& (psDioPathInfo->abMusicRSlot[2] != 3)) {
			bRet	= 0;
		} else if (psDioPathInfo->abMusicRSlot[2] !=
			sDioPathInfo.abMusicRSlot[2]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
			/*if (bLP2_start != 0) {
				bRet	= 0;
			}*/
		}
	}

	if ((dUpdateInfo & MCDRV_DIT0SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicTSlot[0] != 0)
		&& (psDioPathInfo->abMusicTSlot[0] != 1)
		&& (psDioPathInfo->abMusicTSlot[0] != 2)
		&& (psDioPathInfo->abMusicTSlot[0] != 3)) {
			bRet	= 0;
		} else if (psDioPathInfo->abMusicTSlot[0] !=
			sDioPathInfo.abMusicTSlot[0]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
			/*if (bLP0_start != 0) {
				bRet	= 0;
			}*/
		}
	}
	if ((dUpdateInfo & MCDRV_DIT1SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicTSlot[1] != 0)
		&& (psDioPathInfo->abMusicTSlot[1] != 1)
		&& (psDioPathInfo->abMusicTSlot[1] != 2)
		&& (psDioPathInfo->abMusicTSlot[1] != 3)) {
			bRet	= 0;
		} else if (psDioPathInfo->abMusicTSlot[1] !=
			sDioPathInfo.abMusicTSlot[1]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
			/*if (bLP1_start != 0) {
				bRet	= 0;
			}*/
		}
	}
	if ((dUpdateInfo & MCDRV_DIT2SLOT_UPDATE_FLAG) != 0UL) {
		if ((psDioPathInfo->abMusicTSlot[2] != 0)
		&& (psDioPathInfo->abMusicTSlot[2] != 1)
		&& (psDioPathInfo->abMusicTSlot[2] != 2)
		&& (psDioPathInfo->abMusicTSlot[2] != 3)) {
			bRet	= 0;
		} else if (psDioPathInfo->abMusicTSlot[2] !=
			sDioPathInfo.abMusicTSlot[2]) {
			if (bLP0_start != 0) {
				bRet	= 0;
				goto exit;
			}
			/*if (bLP2_start != 0) {
				bRet	= 0;
			}*/
		}
	}

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidDioPathParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidSwapParam
 *
 *	Description:
 *			check Swap parameters.
 *	Arguments:
 *			psSwapInfo	Swap information
 *			dUpdateInfo	update information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidSwapParam
(
	const struct MCDRV_SWAP_INFO	*psSwapInfo,
	UINT32	dUpdateInfo
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidSwapParam");
#endif


	if ((dUpdateInfo & MCDRV_SWAP_ADIF0_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bAdif0 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bAdif0 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_ADIF1_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bAdif1 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bAdif1 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_ADIF2_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bAdif2 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bAdif2 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_DAC0_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bDac0 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bDac0 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_DAC1_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bDac1 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bDac1 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}

	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN0_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicIn0 != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bMusicIn0 != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bMusicIn0 != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bMusicIn0 != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN1_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicIn1 != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bMusicIn1 != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bMusicIn1 != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bMusicIn1 != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICIN2_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicIn2 != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bMusicIn2 != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bMusicIn2 != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bMusicIn2 != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_EXTIN_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bExtIn != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bExtIn != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bExtIn != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bExtIn != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_VOICEIN_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bVoiceIn != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bVoiceIn != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bVoiceIn != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bVoiceIn != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_HIFIIN_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bHifiIn != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bHifiIn != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bHifiIn != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bHifiIn != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT0_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicOut0 != MCDRV_SWAP_NORMAL)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_SWAP)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_MUTE)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_CENTER)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_MIX)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_MONOMIX)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_BOTHL)
		&& (psSwapInfo->bMusicOut0 != MCDRV_SWAP_BOTHR))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT1_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicOut1 != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bMusicOut1 != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bMusicOut1 != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bMusicOut1 != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_MUSICOUT2_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bMusicOut2 != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bMusicOut2 != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bMusicOut2 != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bMusicOut2 != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_EXTOUT_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bExtOut != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bExtOut != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bExtOut != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bExtOut != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_VOICEOUT_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bVoiceOut != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bVoiceOut != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bVoiceOut != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bVoiceOut != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SWAP_HIFIOUT_UPDATE_FLAG) != 0UL) {
		if ((psSwapInfo->bHifiOut != MCDRV_SWSWAP_NORMAL)
		&& (psSwapInfo->bHifiOut != MCDRV_SWSWAP_BOTH1)
		&& (psSwapInfo->bHifiOut != MCDRV_SWSWAP_BOTH0)
		&& (psSwapInfo->bHifiOut != MCDRV_SWSWAP_SWAP))
			bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidSwapParam", &sdRet);
#endif

	return bRet;
}

/****************************************************************************
 *	IsValidDspParam
 *
 *	Description:
 *			validate DSP parameters.
 *	Arguments:
 *			psAECInfo	AEC information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidDspParam
(
	const struct MCDRV_AEC_INFO	*psAECInfo
)
{
	UINT8	bRet	= 1;
	UINT8	bBdspUsed	= 0,
		bCdspUsed	= 0,
		bFdspUsed	= 0;
	struct MCDRV_AEC_INFO	sCurAECInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidDspParam");
#endif


	McResCtrl_GetAecInfo(&sCurAECInfo);

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_AE0_ON
				|MCDRV_D1SRC_AE1_ON
				|MCDRV_D1SRC_AE2_ON
				|MCDRV_D1SRC_AE3_ON) != 0) {
		bBdspUsed	= 1;
		if (sCurAECInfo.sAecConfig.bFDspLocate == 0)
			bFdspUsed	= 1;
	}
	if ((McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) != 0)
	|| (McResCtrl_HasSrc(eMCDRV_DST_VBOXHOSTIN, eMCDRV_DST_CH0) != 0)) {
		bCdspUsed	= 1;
		if (sCurAECInfo.sAecConfig.bFDspLocate == 1)
			bFdspUsed	= 1;
	}

	if ((bBdspUsed != 0)
	|| (bCdspUsed != 0)
	|| (bFdspUsed != 0))
		if (psAECInfo->sAecConfig.bFDspLocate != 0xFF)
			if (sCurAECInfo.sAecConfig.bFDspLocate !=
				psAECInfo->sAecConfig.bFDspLocate) {
				bRet	= 0;
				goto exit;
			}

	if ((bBdspUsed != 0)
	|| (bFdspUsed != 0))
		if (psAECInfo->sAecAudioengine.bEnable != 0) {
			if ((psAECInfo->sAecAudioengine.bBDspAE0Src != 2)
			&& (psAECInfo->sAecAudioengine.bBDspAE0Src !=
				sCurAECInfo.sAecAudioengine.bBDspAE0Src)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecAudioengine.bBDspAE1Src != 2)
			&& (psAECInfo->sAecAudioengine.bBDspAE1Src !=
				sCurAECInfo.sAecAudioengine.bBDspAE1Src)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecAudioengine.bMixerIn0Src != 2)
			&& (psAECInfo->sAecAudioengine.bMixerIn0Src !=
				sCurAECInfo.sAecAudioengine.bMixerIn0Src)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecAudioengine.bMixerIn1Src != 2)
			&& (psAECInfo->sAecAudioengine.bMixerIn1Src !=
				sCurAECInfo.sAecAudioengine.bMixerIn1Src)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecAudioengine.bMixerIn2Src != 2)
			&& (psAECInfo->sAecAudioengine.bMixerIn2Src !=
				sCurAECInfo.sAecAudioengine.bMixerIn2Src)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecAudioengine.bMixerIn3Src != 2)
			&& (psAECInfo->sAecAudioengine.bMixerIn3Src !=
				sCurAECInfo.sAecAudioengine.bMixerIn3Src)) {
				bRet	= 0;
				goto exit;
			}
		}

	if ((bCdspUsed != 0)
	|| (bFdspUsed != 0))
		if (psAECInfo->sAecVBox.bEnable != 0) {
			if ((psAECInfo->sAecVBox.bFdsp_Po_Source != 0xFF)
			&& (psAECInfo->sAecVBox.bFdsp_Po_Source !=
				sCurAECInfo.sAecVBox.bFdsp_Po_Source)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bISrc2_VSource != 0xFF)
			&& (psAECInfo->sAecVBox.bISrc2_VSource !=
				sCurAECInfo.sAecVBox.bISrc2_VSource)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bISrc2_Ch1_VSource != 0xFF)
			&& (psAECInfo->sAecVBox.bISrc2_Ch1_VSource !=
				sCurAECInfo.sAecVBox.bISrc2_Ch1_VSource)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bISrc3_VSource != 0xFF)
			&& (psAECInfo->sAecVBox.bISrc3_VSource !=
				sCurAECInfo.sAecVBox.bISrc3_VSource)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bLPt2_VSource != 0xFF)
			&& (psAECInfo->sAecVBox.bLPt2_VSource !=
				sCurAECInfo.sAecVBox.bLPt2_VSource)) {
				bRet	= 0;
				goto exit;
			}
		}

	if (bCdspUsed != 0)
		if (psAECInfo->sAecVBox.bEnable != 0) {
			if ((psAECInfo->sAecVBox.bSrc3_Ctrl != 0xFF)
			&& (psAECInfo->sAecVBox.bSrc3_Ctrl !=
				sCurAECInfo.sAecVBox.bSrc3_Ctrl)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bSrc2_Fs != 0xFF)
			&& (psAECInfo->sAecVBox.bSrc2_Fs !=
				sCurAECInfo.sAecVBox.bSrc2_Fs)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bSrc2_Thru != 0xFF)
			&& (psAECInfo->sAecVBox.bSrc2_Thru !=
				sCurAECInfo.sAecVBox.bSrc2_Thru)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bSrc3_Fs != 0xFF)
			&& (psAECInfo->sAecVBox.bSrc3_Fs !=
				sCurAECInfo.sAecVBox.bSrc3_Fs)) {
				bRet	= 0;
				goto exit;
			}
			if ((psAECInfo->sAecVBox.bSrc3_Thru != 0xFF)
			&& (psAECInfo->sAecVBox.bSrc3_Thru !=
				sCurAECInfo.sAecVBox.bSrc3_Thru)) {
				bRet	= 0;
				goto exit;
			}
		}

	if (psAECInfo->sControl.bCommand == 1) {
		if ((psAECInfo->sControl.bParam[0] != 0)
		&& (psAECInfo->sControl.bParam[0] != 1)) {
			bRet	= 0;
			goto exit;
		}
	} else if (psAECInfo->sControl.bCommand == 2) {
		if ((psAECInfo->sControl.bParam[0] != 0)
		&& (psAECInfo->sControl.bParam[0] != 1)) {
			bRet	= 0;
			goto exit;
		}
	} else if (psAECInfo->sControl.bCommand == 3) {
		if ((psAECInfo->sControl.bParam[0] != 0)
		&& (psAECInfo->sControl.bParam[0] != 1)
		&& (psAECInfo->sControl.bParam[0] != 2)) {
			bRet	= 0;
			goto exit;
		}
	} else if (psAECInfo->sControl.bCommand == 4) {
		if ((psAECInfo->sControl.bParam[0] != 0)
		&& (psAECInfo->sControl.bParam[0] != 1)
		&& (psAECInfo->sControl.bParam[0] != 3)) {
			bRet	= 0;
			goto exit;
		}
	} else if (psAECInfo->sControl.bCommand == 5) {
		if ((psAECInfo->sControl.bParam[0] > 6)
		&& (psAECInfo->sControl.bParam[0] < 0xFF)) {
			bRet	= 0;
			goto exit;
		}
		if ((psAECInfo->sControl.bParam[1] > 3)
		&& (psAECInfo->sControl.bParam[1] < 0xFF)) {
			bRet	= 0;
			goto exit;
		}
	}

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidDspParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidHSDetParam
 *
 *	Description:
 *			validate HSDet parameters.
 *	Arguments:
 *			psHSDetInfo	HSDet information
 *			psHSDet2Info	HSDet2 information
 *			dUpdateInfo	update information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidHSDetParam
(
	const struct MCDRV_HSDET_INFO	*psHSDetInfo,
	const struct MCDRV_HSDET2_INFO	*psHSDet2Info,
	UINT32	dUpdateInfo
)
{
	UINT8	bRet	= 1;
	struct MCDRV_HSDET_INFO	sHSDetInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidHSDetParam");
#endif


	McResCtrl_GetHSDet(&sHSDetInfo, NULL);

	if ((dUpdateInfo & MCDRV_ENPLUGDET_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnPlugDet != MCDRV_PLUGDET_DISABLE)
		&& (psHSDetInfo->bEnPlugDet != MCDRV_PLUGDET_ENABLE))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENPLUGDET_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnPlugDetDb != MCDRV_PLUGDETDB_DISABLE)
		&& (psHSDetInfo->bEnPlugDetDb != MCDRV_PLUGDETDB_DET_ENABLE)
		&& (psHSDetInfo->bEnPlugDetDb != MCDRV_PLUGDETDB_UNDET_ENABLE)
		&& (psHSDetInfo->bEnPlugDetDb != MCDRV_PLUGDETDB_BOTH_ENABLE))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENDLYKEYOFF_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_D_D_D)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_D_D_E)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_D_E_D)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_D_E_E)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_E_D_D)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_E_D_E)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_E_E_D)
		&& (psHSDetInfo->bEnDlyKeyOff != MCDRV_KEYEN_E_E_E))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENDLYKEYON_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_D_D_D)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_D_D_E)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_D_E_D)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_D_E_E)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_E_D_D)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_E_D_E)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_E_E_D)
		&& (psHSDetInfo->bEnDlyKeyOn != MCDRV_KEYEN_E_E_E))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENMICDET_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnMicDet != MCDRV_MICDET_DISABLE)
		&& (psHSDetInfo->bEnMicDet != MCDRV_MICDET_ENABLE))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENKEYOFF_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_D_D_D)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_D_D_E)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_D_E_D)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_D_E_E)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_E_D_D)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_E_D_E)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_E_E_D)
		&& (psHSDetInfo->bEnKeyOff != MCDRV_KEYEN_E_E_E))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_ENKEYON_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_D_D_D)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_D_D_E)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_D_E_D)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_D_E_E)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_E_D_D)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_E_D_E)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_E_E_D)
		&& (psHSDetInfo->bEnKeyOn != MCDRV_KEYEN_E_E_E))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_HSDETDBNC_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_27)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_55)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_109)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_219)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_438)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_875)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_1313)
		&& (psHSDetInfo->bHsDetDbnc != MCDRV_DETDBNC_1750))
			bRet	= 0;
	}

	if ((dUpdateInfo & MCDRV_KEYOFFMTIM_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bKeyOffMtim == MCDRV_KEYOFF_MTIM_63)
		|| (psHSDetInfo->bKeyOffMtim == MCDRV_KEYOFF_MTIM_16))
			sHSDetInfo.bKeyOffMtim	= psHSDetInfo->bKeyOffMtim;
		else
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_KEYONMTIM_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bKeyOnMtim == MCDRV_KEYON_MTIM_63)
		|| (psHSDetInfo->bKeyOnMtim == MCDRV_KEYON_MTIM_250))
			sHSDetInfo.bKeyOnMtim	= psHSDetInfo->bKeyOnMtim;
		else
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_KEY0OFFDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey0OffDlyTim > MC_DRV_KEYOFFDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOffMtim == MCDRV_KEYOFF_MTIM_63) {
			if (psHSDetInfo->bKey0OffDlyTim == 1)
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY1OFFDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey1OffDlyTim > MC_DRV_KEYOFFDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOffMtim == MCDRV_KEYOFF_MTIM_63) {
			if (psHSDetInfo->bKey1OffDlyTim == 1)
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY2OFFDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey2OffDlyTim > MC_DRV_KEYOFFDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOffMtim == MCDRV_KEYOFF_MTIM_63) {
			if (psHSDetInfo->bKey2OffDlyTim == 1)
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY0ONDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey0OnDlyTim > MC_DRV_KEYONDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey0OnDlyTim == 1)
			|| (psHSDetInfo->bKey0OnDlyTim == 2)
			|| (psHSDetInfo->bKey0OnDlyTim == 3))
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY1ONDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey1OnDlyTim > MC_DRV_KEYONDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey1OnDlyTim == 1)
			|| (psHSDetInfo->bKey1OnDlyTim == 2)
			|| (psHSDetInfo->bKey1OnDlyTim == 3))
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY2ONDLYTIM_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey2OnDlyTim > MC_DRV_KEYONDLYTIM_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey2OnDlyTim == 1)
			|| (psHSDetInfo->bKey2OnDlyTim == 2)
			|| (psHSDetInfo->bKey2OnDlyTim == 3))
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY0ONDLYTIM2_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey0OnDlyTim2 > MC_DRV_KEYONDLYTIM2_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey0OnDlyTim2 == 1)
			|| (psHSDetInfo->bKey0OnDlyTim2 == 2)
			|| (psHSDetInfo->bKey0OnDlyTim2 == 3))
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY1ONDLYTIM2_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey1OnDlyTim2 > MC_DRV_KEYONDLYTIM2_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey1OnDlyTim2 == 1)
			|| (psHSDetInfo->bKey1OnDlyTim2 == 2)
			|| (psHSDetInfo->bKey1OnDlyTim2 == 3))
				bRet	= 0;
		}
	}
	if ((dUpdateInfo & MCDRV_KEY2ONDLYTIM2_UPDATE_FLAG) != 0UL) {
		if (psHSDetInfo->bKey2OnDlyTim2 > MC_DRV_KEYONDLYTIM2_MAX)
			bRet	= 0;
		else if (sHSDetInfo.bKeyOnMtim == MCDRV_KEYON_MTIM_250) {
			if ((psHSDetInfo->bKey2OnDlyTim2 == 1)
			|| (psHSDetInfo->bKey2OnDlyTim2 == 2)
			|| (psHSDetInfo->bKey2OnDlyTim2 == 3))
				bRet	= 0;
		}
	}

	if ((dUpdateInfo & MCDRV_IRQTYPE_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bIrqType != MCDRV_IRQTYPE_NORMAL)
		&& (psHSDetInfo->bIrqType != MCDRV_IRQTYPE_REF)
		&& (psHSDetInfo->bIrqType != MCDRV_IRQTYPE_EX)) {
			bRet	= 0;
			goto exit;
		} else if (psHSDetInfo->bIrqType == MCDRV_IRQTYPE_EX) {
			if (psHSDet2Info != NULL) {
				if ((psHSDet2Info->bPlugDetDbIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bPlugDetDbIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bPlugUndetDbIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bPlugUndetDbIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bMicDetIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bMicDetIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bPlugDetIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bPlugDetIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey0OnIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey0OnIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey1OnIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey1OnIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey2OnIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey2OnIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey0OffIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey0OffIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey1OffIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey1OffIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
				if ((psHSDet2Info->bKey2OffIrqType !=
					MCDRV_IRQTYPE_NORMAL)
				&& (psHSDet2Info->bKey2OffIrqType !=
					MCDRV_IRQTYPE_REF)) {
					bRet	= 0;
					goto exit;
				}
			}
		}
	}
	if ((dUpdateInfo & MCDRV_DETINV_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bDetInInv != MCDRV_DET_IN_NORMAL)
		&& (psHSDetInfo->bDetInInv != MCDRV_DET_IN_INV)
		&& (psHSDetInfo->bDetInInv !=
					MCDRV_DET_IN_NORMAL_NORMAL)
		&& (psHSDetInfo->bDetInInv != MCDRV_DET_IN_INV_NORMAL)
		&& (psHSDetInfo->bDetInInv != MCDRV_DET_IN_NORMAL_INV)
		&& (psHSDetInfo->bDetInInv != MCDRV_DET_IN_INV_INV))
			bRet	= 0;
	}

	if ((dUpdateInfo & MCDRV_HSDETMODE_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bHsDetMode != MCDRV_HSDET_MODE_DETIN_A)
		&& (psHSDetInfo->bHsDetMode != MCDRV_HSDET_MODE_DETIN_B))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_SPERIOD_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bSperiod != MCDRV_SPERIOD_244)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_488)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_977)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_1953)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_3906)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_7813)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_15625)
		&& (psHSDetInfo->bSperiod != MCDRV_SPERIOD_31250))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_LPERIOD_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bLperiod != MCDRV_LPERIOD_3906)
		&& (psHSDetInfo->bLperiod != MCDRV_LPERIOD_62500)
		&& (psHSDetInfo->bLperiod != MCDRV_LPERIOD_125000)
		&& (psHSDetInfo->bLperiod != MCDRV_LPERIOD_250000))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_DBNCNUMPLUG_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bDbncNumPlug != MCDRV_DBNC_NUM_2)
		&& (psHSDetInfo->bDbncNumPlug != MCDRV_DBNC_NUM_3)
		&& (psHSDetInfo->bDbncNumPlug != MCDRV_DBNC_NUM_4)
		&& (psHSDetInfo->bDbncNumPlug != MCDRV_DBNC_NUM_7))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_DBNCNUMMIC_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bDbncNumMic != MCDRV_DBNC_NUM_2)
		&& (psHSDetInfo->bDbncNumMic != MCDRV_DBNC_NUM_3)
		&& (psHSDetInfo->bDbncNumMic != MCDRV_DBNC_NUM_4)
		&& (psHSDetInfo->bDbncNumMic != MCDRV_DBNC_NUM_7))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_DBNCNUMKEY_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bDbncNumKey != MCDRV_DBNC_NUM_2)
		&& (psHSDetInfo->bDbncNumKey != MCDRV_DBNC_NUM_3)
		&& (psHSDetInfo->bDbncNumKey != MCDRV_DBNC_NUM_4)
		&& (psHSDetInfo->bDbncNumKey != MCDRV_DBNC_NUM_7))
			bRet	= 0;
	}

	if ((dUpdateInfo & MCDRV_SGNL_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bSgnlPeriod != MCDRV_SGNLPERIOD_61)
		&& (psHSDetInfo->bSgnlPeriod != MCDRV_SGNLPERIOD_79)
		&& (psHSDetInfo->bSgnlPeriod != MCDRV_SGNLPERIOD_97)
		&& (psHSDetInfo->bSgnlPeriod != MCDRV_SGNLPERIOD_151))
			bRet	= 0;
		if ((psHSDetInfo->bSgnlNum != MCDRV_SGNLNUM_1)
		&& (psHSDetInfo->bSgnlNum != MCDRV_SGNLNUM_4)
		&& (psHSDetInfo->bSgnlNum != MCDRV_SGNLNUM_6)
		&& (psHSDetInfo->bSgnlNum != MCDRV_SGNLNUM_8)
		&& (psHSDetInfo->bSgnlNum != MCDRV_SGNLNUM_NONE))
			bRet	= 0;
		if ((psHSDetInfo->bSgnlPeak != MCDRV_SGNLPEAK_500)
		&& (psHSDetInfo->bSgnlPeak != MCDRV_SGNLPEAK_730)
		&& (psHSDetInfo->bSgnlPeak != MCDRV_SGNLPEAK_960)
		&& (psHSDetInfo->bSgnlPeak != MCDRV_SGNLPEAK_1182))
			bRet	= 0;
	}
	if ((dUpdateInfo & MCDRV_IMPSEL_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bImpSel != 0)
		&& (psHSDetInfo->bImpSel != 1))
			bRet	= 0;
	}

	if ((dUpdateInfo & MCDRV_DLYIRQSTOP_UPDATE_FLAG) != 0UL) {
		if ((psHSDetInfo->bDlyIrqStop != MCDRV_DLYIRQ_DONTCARE)
		&& (psHSDetInfo->bDlyIrqStop != MCDRV_DLYIRQ_STOP))
			bRet	= 0;
	}

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidHSDetParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidGpParam
 *
 *	Description:
 *			validate GP parameters.
 *	Arguments:
 *			psGpInfo	GP information
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidGpParam
(
	const struct MCDRV_GP_MODE	*psGpMode
)
{
	UINT8	bRet	= 1;
	UINT8	bPad;
	struct MCDRV_INIT_INFO	sInitInfo;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("IsValidGpParam");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);

	for (bPad = 0; (bPad < 3) && (bRet == 1); bPad++) {
		if (((bPad == 0) && (sInitInfo.bPa0Func != MCDRV_PA_GPIO))
		|| ((bPad == 1) && (sInitInfo.bPa1Func != MCDRV_PA_GPIO))
		|| ((bPad == 2) && (sInitInfo.bPa2Func != MCDRV_PA_GPIO)))
			continue;
		if ((psGpMode->abGpDdr[bPad] != MCDRV_GPDDR_IN)
		&& (psGpMode->abGpDdr[bPad] != MCDRV_GPDDR_OUT)) {
			bRet	= 0;
			continue;
		}
		if ((psGpMode->abGpHost[bPad] != MCDRV_GPHOST_CPU)
		&& (psGpMode->abGpHost[bPad] != MCDRV_GPHOST_CDSP)) {
			bRet	= 0;
			continue;
		}
		if ((psGpMode->abGpInvert[bPad] != MCDRV_GPINV_NORMAL)
		&& (psGpMode->abGpInvert[bPad] != MCDRV_GPINV_INVERT))
			bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidGpParam", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	IsValidMaskGp
 *
 *	Description:
 *			validate GP parameters.
 *	Arguments:
 *			bMask	MaskGP information
 *			dPadNo	PAD number
 *	Return:
 *			0:Invalid
 *			other:Valid
 *
 ****************************************************************************/
static UINT8	IsValidMaskGp
(
	UINT8	bMask,
	UINT32	dPadNo
)
{
	UINT8	bRet	= 1;
	struct MCDRV_INIT_INFO	sInitInfo;
	struct MCDRV_GP_MODE	sGPMode;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= MCDRV_SUCCESS;
	McDebugLog_FuncIn("IsValidMaskGp");
#endif

	McResCtrl_GetInitInfo(&sInitInfo, NULL);
	McResCtrl_GetGPMode(&sGPMode);

	if ((dPadNo != 0)
	&& (dPadNo != 1)
	&& (dPadNo != 2))
		bRet	= 0;
	else if (((dPadNo == 0) && (sInitInfo.bPa0Func != MCDRV_PA_GPIO))
	|| ((dPadNo == 1) && (sInitInfo.bPa1Func != MCDRV_PA_GPIO))
	|| ((dPadNo == 2) && (sInitInfo.bPa2Func != MCDRV_PA_GPIO))) {
		;
	} else {
		if (sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT)
			bRet	= 0;
		if ((bMask != MCDRV_GPMASK_ON) && (bMask != MCDRV_GPMASK_OFF))
			bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("IsValidMaskGp", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	CheckDIOCommon
 *
 *	Description:
 *			check Digital IO Common parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *	Return:
 *			0:error
 *			other:no error
 *
 ****************************************************************************/
static UINT8	CheckDIOCommon
(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet	= MCDRV_SUCCESS;
	McDebugLog_FuncIn("CheckDIOCommon");
#endif


	if (bPort == 3) {
		if ((psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_48000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_192000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_96000))
			bRet	= 0;
		else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
				!= MCDRV_BCKFS_64)
			&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
				!= MCDRV_BCKFS_48)
			&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
				!= MCDRV_BCKFS_32))
			bRet	= 0;
		else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert
				!= MCDRV_BCLK_NORMAL)
			&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert
				!= MCDRV_BCLK_INVERT))
			bRet	= 0;
		goto exit;
	}

	if ((psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave
		!= MCDRV_DIO_SLAVE)
	&& (psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave
		!= MCDRV_DIO_MASTER))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs
			!= MCDRV_AUTOFS_OFF)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs
			!= MCDRV_AUTOFS_ON))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_48000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_44100)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_32000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_24000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_22050)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_16000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_12000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_11025)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_8000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_192000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_96000))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_64)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_48)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_32)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_512)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_256)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_192)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_128)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_96)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_24)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_16)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_8)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_SLAVE))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bInterface
			!= MCDRV_DIO_DA)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bInterface
			!= MCDRV_DIO_PCM))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert
			!= MCDRV_BCLK_NORMAL)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert
			!= MCDRV_BCLK_INVERT))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bSrcThru
			!= MCDRV_SRC_NOT_THRU)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bSrcThru
			!= MCDRV_SRC_THRU)) {
		if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
			;
			bRet	= 0;
		}
	} else {
		if ((psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim
			!= MCDRV_PCMHIZTIM_FALLING)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim
			!= MCDRV_PCMHIZTIM_RISING))
			bRet	= 0;
		else if ((psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame
				!= MCDRV_PCM_SHORTFRAME)
			&& (psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame
				!= MCDRV_PCM_LONGFRAME))
			bRet	= 0;
		else if (psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHighPeriod
				> 31)
			bRet	= 0;
	}

	if (psDioInfo->asPortInfo[bPort].sDioCommon.bInterface
		== MCDRV_DIO_PCM) {
		if ((psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_8000)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
			!= MCDRV_FS_16000))
			bRet	= 0;

		if (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			== MCDRV_BCKFS_512) {
			if (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
				!= MCDRV_FS_8000)
				bRet	= 0;
		}
		if (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			== MCDRV_BCKFS_256) {
			if ((psDioInfo->asPortInfo[bPort].sDioCommon.bFs
				!= MCDRV_FS_8000)
			&& (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
				!= MCDRV_FS_16000))
				bRet	= 0;
		}
		if (psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave
			== MCDRV_DIO_MASTER) {
			if (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
				== MCDRV_BCKFS_SLAVE)
				bRet	= 0;
		}
	} else {
		if ((bPort == 0)
		|| (bPort == 1)) {
			if ((psDioInfo->asPortInfo[bPort].sDioCommon.bFs
					== MCDRV_FS_192000)
			|| (psDioInfo->asPortInfo[bPort].sDioCommon.bFs
					== MCDRV_FS_96000))
				bRet	= 0;
		}
		if ((psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_64)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_48)
		&& (psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs
			!= MCDRV_BCKFS_32)) {
			bRet	= 0;
		}
	}

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("CheckDIOCommon", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	CheckDaFormat
 *
 *	Description:
 *			check sDaFormat parameters.
 *	Arguments:
 *			psDaFormat	MCDRV_DA_FORMAT information
 *	Return:
 *			0:error
 *			other:no error
 *
 ****************************************************************************/
static UINT8	CheckDaFormat
(
	const struct MCDRV_DA_FORMAT	*psDaFormat
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("CheckDaFormat");
#endif


	if ((psDaFormat->bBitSel != MCDRV_BITSEL_16)
	&& (psDaFormat->bBitSel != MCDRV_BITSEL_20)
	&& (psDaFormat->bBitSel != MCDRV_BITSEL_24)
	&& (psDaFormat->bBitSel != MCDRV_BITSEL_32)) {
		bRet	= 0;
	} else if ((psDaFormat->bMode != MCDRV_DAMODE_HEADALIGN)
		&& (psDaFormat->bMode != MCDRV_DAMODE_I2S)
		&& (psDaFormat->bMode != MCDRV_DAMODE_TAILALIGN)) {
		bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("CheckDaFormat", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	CheckPcmFormat
 *
 *	Description:
 *			check sPcmFormat parameters.
 *	Arguments:
 *			psPcmFormat	MCDRV_PCM_FORMAT information
 *	Return:
 *			0:error
 *			other:no error
 *
 ****************************************************************************/
static UINT8	CheckPcmFormat
(
	const struct MCDRV_PCM_FORMAT	*psPcmFormat
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("CheckPcmFormat");
#endif


	if ((psPcmFormat->bMono != MCDRV_PCM_STEREO)
	&& (psPcmFormat->bMono != MCDRV_PCM_MONO)) {
		;
		bRet	= 0;
	} else if ((psPcmFormat->bOrder != MCDRV_PCM_MSB_FIRST)
		&& (psPcmFormat->bOrder != MCDRV_PCM_LSB_FIRST)) {
		;
		bRet	= 0;
	} else if ((psPcmFormat->bLaw != MCDRV_PCM_LINEAR)
		&& (psPcmFormat->bLaw != MCDRV_PCM_ALAW)
		&& (psPcmFormat->bLaw != MCDRV_PCM_MULAW)) {
		;
		bRet	= 0;
	} else if ((psPcmFormat->bBitSel != MCDRV_PCM_BITSEL_8)
		&& (psPcmFormat->bBitSel != MCDRV_PCM_BITSEL_16)
		&& (psPcmFormat->bBitSel != MCDRV_PCM_BITSEL_24)) {
		;
		bRet	= 0;
	} else {
		if ((psPcmFormat->bLaw == MCDRV_PCM_ALAW)
		|| (psPcmFormat->bLaw == MCDRV_PCM_MULAW)) {
			if (psPcmFormat->bBitSel != MCDRV_PCM_BITSEL_8) {
				;
				bRet	= 0;
			}
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("CheckPcmFormat", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	CheckDIODIR
 *
 *	Description:
 *			validate Digital IO DIR parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *	Return:
 *			0:error
 *			other:no error
 *
 ****************************************************************************/
static UINT8	CheckDIODIR
(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("CheckDIODIR");
#endif


	bRet	= CheckPcmFormat(
				&psDioInfo->asPortInfo[bPort].sDir.sPcmFormat);
	if (bRet != 0) {
		bRet	= CheckDaFormat(
				&psDioInfo->asPortInfo[bPort].sDir.sDaFormat);
		if (psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel
			== MCDRV_BITSEL_32)
			bRet	= 0;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("CheckDIODIR", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	CheckDIODIT
 *
 *	Description:
 *			validate Digital IO DIT parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *	Return:
 *			0:error
 *			other:no error
 *
 ****************************************************************************/
static UINT8	CheckDIODIT
(
	const struct MCDRV_DIO_INFO	*psDioInfo,
	UINT8	bPort
)
{
	UINT8	bRet	= 1;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("CheckDIODIT");
#endif


	if ((psDioInfo->asPortInfo[bPort].sDit.bStMode != MCDRV_STMODE_ZERO)
	&& (psDioInfo->asPortInfo[bPort].sDit.bStMode != MCDRV_STMODE_HOLD))
		bRet	= 0;
	else if ((psDioInfo->asPortInfo[bPort].sDit.bEdge != MCDRV_SDOUT_NORMAL)
	&& (psDioInfo->asPortInfo[bPort].sDit.bEdge != MCDRV_SDOUT_AHEAD))
		bRet	= 0;
	else {
		bRet	= CheckPcmFormat(
				&psDioInfo->asPortInfo[bPort].sDit.sPcmFormat);
		if (bRet != 0) {
			bRet	= CheckDaFormat(
				&psDioInfo->asPortInfo[bPort].sDit.sDaFormat);
			if (psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel
				== MCDRV_BITSEL_32)
				if (bPort != 2)
					bRet	= 0;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= bRet;
	McDebugLog_FuncOut("CheckDIODIT", &sdRet);
#endif
	return bRet;
}

/****************************************************************************
 *	SetVol
 *
 *	Description:
 *			set volume.
 *	Arguments:
 *			dUpdate		target volume items
 *			eMode		update mode
 *			pdSVolDoneParam	wait soft volume complete flag
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static SINT32	SetVol
(
	UINT32	dUpdate,
	enum MCDRV_VOLUPDATE_MODE	eMode,
	UINT32	*pdSVolDoneParam
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SetVol");
#endif

	McPacket_AddVol(dUpdate, eMode, pdSVolDoneParam);
	sdRet	= McDevIf_ExecutePacket();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SetVol", &sdRet);
#endif

	return sdRet;
}

/****************************************************************************
 *	GetMuteParam
 *
 *	Description:
 *			Get mute complete flag.
 *	Arguments:
 *			pbDIRMuteParam	wait DIR mute complete flag
 *			pbADCMuteParam	wait ADC mute complete flag
 *			pbDITMuteParam	wait DIT mute complete flag
 *			pbDACMuteParam	wait DAC mute complete flag
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	GetMuteParam
(
	UINT8	*pbDIRMuteParam,
	UINT8	*pbADCMuteParam,
	UINT8	*pbDITMuteParam,
	UINT8	*pbDACMuteParam
)
{
	UINT8	bVol;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("GetMuteParam");
#endif

	*pbDIRMuteParam	= 0;
	*pbADCMuteParam	= 0;
	*pbDITMuteParam	= 0;
	*pbDACMuteParam	= 0;

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_MUSICIN_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI0_VOL0);
		if ((bVol & (UINT8)~MCB_DIFI0_VSEP) != 0)
			*pbDIRMuteParam	|= MCB_DIFI0_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI0_VOL1);
		if (bVol != 0)
			*pbDIRMuteParam	|= MCB_DIFI0_VFLAG1;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_EXTIN_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI1_VOL0);
		if ((bVol & (UINT8)~MCB_DIFI1_VSEP) != 0)
			*pbDIRMuteParam	|= MCB_DIFI1_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI1_VOL1);
		if (bVol != 0)
			*pbDIRMuteParam	|= MCB_DIFI1_VFLAG1;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXOUT_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI2_VOL0);
		if ((bVol & (UINT8)~MCB_DIFI2_VSEP) != 0)
			*pbDIRMuteParam	|= MCB_DIFI2_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI2_VOL1);
		if (bVol != 0)
			*pbDIRMuteParam	|= MCB_DIFI2_VFLAG1;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_VBOXREFOUT_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI3_VOL0);
		if ((bVol & (UINT8)~MCB_DIFI3_VSEP) != 0)
			*pbDIRMuteParam	|= MCB_DIFI3_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFI3_VOL1);
		if (bVol != 0)
			*pbDIRMuteParam	|= MCB_DIFI3_VFLAG1;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH0) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DAO0_VOL0);
		if ((bVol & (UINT8)~MCB_DAO0_VSEP) != 0)
			*pbDACMuteParam	|= MCB_DAO0_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC0, eMCDRV_DST_CH1) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DAO0_VOL1);
		if (bVol != 0)
			*pbDACMuteParam	|= MCB_DAO0_VFLAG1;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH0) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DAO1_VOL0);
		if ((bVol & (UINT8)~MCB_DAO1_VSEP) != 0)
			*pbDACMuteParam	|= MCB_DAO1_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_DAC1, eMCDRV_DST_CH1) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DAO1_VOL1);
		if (bVol != 0)
			*pbDACMuteParam	|= MCB_DAO1_VFLAG1;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH0) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO0_VOL0);
		if ((bVol & (UINT8)~MCB_DIFO0_VSEP) != 0)
			*pbDITMuteParam	|= MCB_DIFO0_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_MUSICOUT, eMCDRV_DST_CH1) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO0_VOL1);
		if (bVol != 0)
			*pbDITMuteParam	|= MCB_DIFO0_VFLAG1;

	}
	if (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH0) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO1_VOL0);
		if ((bVol & (UINT8)~MCB_DIFO1_VSEP) != 0)
			*pbDITMuteParam	|= MCB_DIFO1_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_EXTOUT, eMCDRV_DST_CH1) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO1_VOL1);
		if (bVol != 0)
			*pbDITMuteParam	|= MCB_DIFO1_VFLAG1;
	}

	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH0) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO2_VOL0);
		if ((bVol & (UINT8)~MCB_DIFO2_VSEP) != 0)
			*pbDITMuteParam	|= MCB_DIFO2_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH1) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO2_VOL1);
		if (bVol != 0)
			*pbDITMuteParam	|= MCB_DIFO2_VFLAG1;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH2) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO3_VOL0);
		if ((bVol & (UINT8)~MCB_DIFO2_VSEP) != 0)
			*pbDITMuteParam	|= MCB_DIFO3_VFLAG0;
	}
	if (McResCtrl_HasSrc(eMCDRV_DST_VBOXMIXIN, eMCDRV_DST_CH3) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_DIFO3_VOL1);
		if (bVol != 0)
			*pbDITMuteParam	|= MCB_DIFO3_VFLAG1;
	}

	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF0_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI0_VOL0);
		if ((bVol & (UINT8)~MCB_ADI0_VSEP) != 0)
			*pbADCMuteParam	|= MCB_ADI0_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI0_VOL1);
		if (bVol != 0)
			*pbADCMuteParam	|= MCB_ADI0_VFLAG1;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF1_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI1_VOL1);
		if ((bVol & (UINT8)~MCB_ADI1_VSEP) != 0)
			*pbADCMuteParam	|= MCB_ADI1_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI1_VOL1);
		if (bVol != 0)
			*pbADCMuteParam	|= MCB_ADI1_VFLAG1;
	}
	if (McResCtrl_IsD1SrcUsed(MCDRV_D1SRC_ADIF2_ON) == 0) {
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI2_VOL1);
		if ((bVol & (UINT8)~MCB_ADI2_VSEP) != 0)
			*pbADCMuteParam	|= MCB_ADI2_VFLAG0;
		bVol	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_MA,
				MCI_ADI2_VOL1);
		if (bVol != 0)
			*pbADCMuteParam	|= MCB_ADI2_VFLAG1;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("GetMuteParam", NULL);
#endif
}

/****************************************************************************
 *	SavePower
 *
 *	Description:
 *			Save power.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	SavePower
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	struct MCDRV_POWER_INFO	sPowerInfo;
	struct MCDRV_POWER_UPDATE	sPowerUpdate;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("SavePower");
#endif

	/*	unused path power down	*/
	McResCtrl_GetPowerInfo(&sPowerInfo);
	sPowerUpdate.bDigital		= MCDRV_POWUPDATE_D_ALL;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_AP;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_AP_OUT0;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_AP_OUT1;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_AP_MC;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_AP_IN;
	sdRet	= McPacket_AddPowerDown(&sPowerInfo, &sPowerUpdate);
	if (sdRet == MCDRV_SUCCESS)
		sdRet	= McDevIf_ExecutePacket();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("SavePower", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McDrv_Ctrl
 *
 *	Description:
 *			MC Driver I/F function.
 *	Arguments:
 *			dCmd		command #
 *			pvPrm1		parameter1
 *			pvPrm2		parameter2
 *			dPrm		update info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
SINT32	McDrv_Ctrl(
	UINT32	dCmd,
	void	*pvPrm1,
	void	*pvPrm2,
	UINT32	dPrm
)
{
	SINT32	sdRet	= MCDRV_ERROR;
	UINT8	bAP;

#if MCDRV_DEBUG_LEVEL
	McDebugLog_CmdIn(dCmd, pvPrm1, pvPrm2, dPrm);
#endif

	(void)pvPrm2;

	if ((UINT32)MCDRV_INIT == dCmd) {
		sdRet	= init((struct MCDRV_INIT_INFO *)pvPrm1,
					(struct MCDRV_INIT2_INFO *)pvPrm2);
	} else if ((UINT32)MCDRV_TERM == dCmd) {
		sdRet	= term();
	} else if ((UINT32)MCDRV_IRQ == dCmd) {
		sdRet	= irq_proc();
	} else {
		McSrv_Lock();

		bAP	= McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_ANA, MCI_AP);

		switch (dCmd) {
		case	MCDRV_GET_CLOCKSW:
			sdRet	= get_clocksw(
					(struct MCDRV_CLOCKSW_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_CLOCKSW:
			sdRet	= set_clocksw(
					(struct MCDRV_CLOCKSW_INFO *)pvPrm1);
			break;

		case	MCDRV_GET_PATH:
			sdRet	= get_path((struct MCDRV_PATH_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_PATH:
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			sdRet	= set_path((struct MCDRV_PATH_INFO *)pvPrm1);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			break;

		case	MCDRV_GET_VOLUME:
			sdRet	= get_volume((struct MCDRV_VOL_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_VOLUME:
			sdRet	= set_volume((struct MCDRV_VOL_INFO *)pvPrm1);
			break;

		case	MCDRV_GET_DIGITALIO:
			sdRet	= get_digitalio(
					(struct MCDRV_DIO_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_DIGITALIO:
			sdRet	= set_digitalio(
					(struct MCDRV_DIO_INFO *)pvPrm1, dPrm);
			break;

		case	MCDRV_GET_DIGITALIO_PATH:
			sdRet	= get_digitalio_path(
					(struct MCDRV_DIOPATH_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_DIGITALIO_PATH:
			sdRet	= set_digitalio_path(
					(struct MCDRV_DIOPATH_INFO *)pvPrm1,
					dPrm);
			break;

		case	MCDRV_GET_SWAP:
			sdRet	= get_swap((struct MCDRV_SWAP_INFO *)pvPrm1);
			break;
		case	MCDRV_SET_SWAP:
			sdRet	= set_swap((struct MCDRV_SWAP_INFO *)pvPrm1,
						dPrm);
			break;

		case	MCDRV_SET_DSP:
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			sdRet	= set_dsp((UINT8 *)pvPrm1, dPrm);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			break;
		case	MCDRV_GET_DSP:
			sdRet	= get_dsp((struct MCDRV_DSP_PARAM *)pvPrm1,
								pvPrm2, dPrm);
			break;
		case	MCDRV_GET_DSP_DATA:
			sdRet	= get_dsp_data((UINT8 *)pvPrm1, dPrm);
			break;
		case	MCDRV_SET_DSP_DATA:
			sdRet	= set_dsp_data((const UINT8 *)pvPrm1, dPrm);
			break;
		case	MCDRV_REGISTER_DSP_CB:
			sdRet	= register_dsp_cb(
				(SINT32 (*)(SINT32, UINT32, UINT32))
				pvPrm1);
			break;
		case	MCDRV_GET_DSP_TRANSITION:
			sdRet	= get_dsp_transition(dPrm);
			break;

		case	MCDRV_GET_HSDET:
			sdRet	= get_hsdet((struct MCDRV_HSDET_INFO *)pvPrm1,
					(struct MCDRV_HSDET2_INFO *)pvPrm2);
			break;
		case	MCDRV_SET_HSDET:
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			sdRet	= set_hsdet((struct MCDRV_HSDET_INFO *)pvPrm1,
					(struct MCDRV_HSDET2_INFO *)pvPrm2,
					dPrm);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			break;

		case	MCDRV_CONFIG_GP:
			sdRet	= config_gp((struct MCDRV_GP_MODE *)pvPrm1);
			break;
		case	MCDRV_MASK_GP:
			sdRet	= mask_gp((UINT8 *)pvPrm1, dPrm);
			break;
		case	MCDRV_GETSET_GP:
			sdRet	= getset_gp((UINT8 *)pvPrm1, dPrm);
			break;

		case	MCDRV_READ_REG:
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			sdRet	= read_reg((struct MCDRV_REG_INFO *)pvPrm1);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			break;
		case	MCDRV_WRITE_REG:
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PreLDODStart();
			}
			sdRet	= write_reg((struct MCDRV_REG_INFO *)pvPrm1);
			if ((bAP&MCB_AP_LDOD) != 0) {
				;
				machdep_PostLDODStart();
			}
			break;

		default:
			break;
		}

		McSrv_Unlock();
	}

#if MCDRV_DEBUG_LEVEL
	McDebugLog_CmdOut(dCmd, &sdRet, pvPrm1, pvPrm2, dPrm);
#endif
#if !defined(CONFIG_MACH_J_CHN_CTC) && !defined(CONFIG_MACH_J_CHN_CU)
	if(sdRet < 0) {
		printk("\n!!!McDrv_Ctrl failed\n\n");
	}
#endif
	return sdRet;
}

