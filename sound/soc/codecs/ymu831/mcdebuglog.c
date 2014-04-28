/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdebuglog.c
 *
 *	Description	: MC Driver debug log
 *
 *	Version		: 1.0.0	2012.12.13
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


#include "mcdebuglog.h"
#include "mcresctrl.h"

#if MCDRV_DEBUG_LEVEL

#include "mcdefs.h"
#include "mcdevprof.h"
#include "mcservice.h"
#include "mcmachdep.h"



#define	CHAR	char

static CHAR	gsbLogString[8192];
static CHAR	gsbLFCode[]	= "\n";

static const CHAR	gsbCmdName[][20]	= {
	"init",
	"term",
	"read_reg",
	"write_reg",
	"get_clocksw",
	"set_clocksw",
	"get_path",
	"set_path",
	"get_volume",
	"set_volume",
	"get_digitalio",
	"set_digitalio",
	"get_digitalio_path",
	"set_digitalio_path",
	"get_swap",
	"set_swap",
	"set_dsp",
	"get_dsp",
	"get_dsp_data",
	"set_dsp_data",
	"register_dsp_cb",
	"get_dsp_transition",
	"irq",
	"get_hsdet",
	"set_hsdet",
	"config_gp",
	"mask_gp",
	"getset_gp"
};

static void	OutputRegDump(void);
static void	GetRegDump(CHAR *psbLogString,
				UINT8 bSlaveAddr,
				UINT8 bADRAddr,
				UINT8 bWINDOWAddr,
				UINT8 bRegType);

static void	MakeInitInfoLog(const struct MCDRV_INIT_INFO *pvPrm1);
static void	MakeRegInfoLog(const struct MCDRV_REG_INFO *pvPrm1);
static void	MakeClockSwInfoLog(const struct MCDRV_CLOCKSW_INFO *pvPrm1);
static void	MakePathInfoLog(const struct MCDRV_PATH_INFO *pvPrm1);
static void	MakeVolInfoLog(const struct MCDRV_VOL_INFO *pvPrm1);
static void	MakeDIOInfoLog(const struct MCDRV_DIO_INFO *pvPrm1);
static void	MakeDIOPathInfoLog(const struct MCDRV_DIOPATH_INFO *pvPrm1);
static void	MakeSwapInfoLog(const struct MCDRV_SWAP_INFO *pvPrm1);
static void	MakeDspLog(const UINT8 *pvPrm1, UINT32 dPrm);
static void	MakeDspPrmLog(const struct MCDRV_DSP_PARAM *pvPrm1,
				const void *pvPrm2,
				UINT32 dPrm);
static void	MakeHSDETInfoLog(const struct MCDRV_HSDET_INFO *pvPrm1);
static void	MakeGPModeLog(const struct MCDRV_GP_MODE *pvPrm1);
static void	MakeGPMaskLog(const UINT8 *pvPrm1);
static void	MakeGetSetGPLog(const UINT8 *pvPrm1);

/****************************************************************************
 *	McDebugLog_CmdIn
 *
 *	Description:
 *			Output Function entrance log.
 *	Arguments:
 *			dCmd		Command ID
 *			pvPrm1		pointer to parameter
 *			pvPrm2		pointer to parameter
 *			dPrm		parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McDebugLog_CmdIn(
	UINT32		dCmd,
	const void	*pvPrm1,
	const void	*pvPrm2,
	UINT32		dPrm
)
{
	CHAR	sbStr[80];
	UINT8	bLevel	= MCDRV_DEBUG_LEVEL;

	if (dCmd >= sizeof(gsbCmdName)/sizeof(gsbCmdName[0])) {
		;
		return;
	}

	strcpy(gsbLogString, gsbCmdName[dCmd]);
	strcat(gsbLogString, " In");

	if (bLevel < 2) {
		strcat(gsbLogString, gsbLFCode);
		machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
		return;
	}

	switch (dCmd) {
	case	MCDRV_INIT:
		MakeInitInfoLog((struct MCDRV_INIT_INFO *)pvPrm1);
		break;
	case	MCDRV_READ_REG:
	case	MCDRV_WRITE_REG:
		MakeRegInfoLog((struct MCDRV_REG_INFO *)pvPrm1);
		break;
	case	MCDRV_SET_CLOCKSW:
		MakeClockSwInfoLog((struct MCDRV_CLOCKSW_INFO *)pvPrm1);
		break;
	case	MCDRV_SET_PATH:
		MakePathInfoLog((struct MCDRV_PATH_INFO *)pvPrm1);
		break;
	case	MCDRV_SET_VOLUME:
		MakeVolInfoLog((struct MCDRV_VOL_INFO *)pvPrm1);
		break;
	case	MCDRV_SET_DIGITALIO:
		MakeDIOInfoLog((struct MCDRV_DIO_INFO *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_SET_DIGITALIO_PATH:
		MakeDIOPathInfoLog((struct MCDRV_DIOPATH_INFO *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_SET_SWAP:
		MakeSwapInfoLog((struct MCDRV_SWAP_INFO *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_SET_DSP:
		MakeDspLog((UINT8 *)pvPrm1, dPrm);
		break;
	case	MCDRV_GET_DSP:
		sprintf(sbStr, " dType=%08lX",
			((struct MCDRV_DSP_PARAM *)pvPrm1)->dType);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " dInfo=%08lX",
			((struct MCDRV_DSP_PARAM *)pvPrm1)->dInfo);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " pvPrm2=%p", pvPrm2);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_GET_DSP_DATA:
		sprintf(sbStr, " pvPrm1=%p", pvPrm2);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_SET_DSP_DATA:
		MakeDspLog((UINT8 *)pvPrm1, dPrm);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_GET_DSP_TRANSITION:
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_REGISTER_DSP_CB:
		sprintf(sbStr, " dPrm=%p", pvPrm1);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_SET_HSDET:
		MakeHSDETInfoLog((struct MCDRV_HSDET_INFO *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_CONFIG_GP:
		MakeGPModeLog((struct MCDRV_GP_MODE *)pvPrm1);
		break;
	case	MCDRV_MASK_GP:
		MakeGPMaskLog((UINT8 *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;
	case	MCDRV_GETSET_GP:
		MakeGetSetGPLog((UINT8 *)pvPrm1);
		sprintf(sbStr, " dPrm=%08lX", dPrm);
		strcat(gsbLogString, sbStr);
		break;

	default:
		break;
	}

	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
}

/****************************************************************************
 *	McDebugLog_CmdOut
 *
 *	Description:
 *			Output Function exit log.
 *	Arguments:
 *			dCmd		Command ID
 *			psdRet		retrun value
 *			pvPrm1		pointer to parameter
 *			pvPrm2		pointer to parameter
 *			dPrm		parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McDebugLog_CmdOut(
	UINT32		dCmd,
	const SINT32	*psdRet,
	const void	*pvPrm1,
	const void	*pvPrm2,
	UINT32		dPrm
)
{
	CHAR	sbStr[80];
	UINT8	bLevel	= MCDRV_DEBUG_LEVEL;

	if (dCmd >= sizeof(gsbCmdName)/sizeof(gsbCmdName[0])) {
		;
		return;
	}

	strcpy(gsbLogString, gsbCmdName[dCmd]);
	strcat(gsbLogString, " Out");
	if (psdRet != NULL) {
		sprintf(sbStr, " ret=%ld", *psdRet);
		strcat(gsbLogString, sbStr);
	}

	if (bLevel < 2) {
		strcat(gsbLogString, gsbLFCode);
		machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
		return;
	}

	switch (dCmd) {
	case	MCDRV_READ_REG:
		MakeRegInfoLog((struct MCDRV_REG_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_CLOCKSW:
		MakeClockSwInfoLog((struct MCDRV_CLOCKSW_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_PATH:
		MakePathInfoLog((struct MCDRV_PATH_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_VOLUME:
		MakeVolInfoLog((struct MCDRV_VOL_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_DIGITALIO:
		MakeDIOInfoLog((struct MCDRV_DIO_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_DIGITALIO_PATH:
		MakeDIOPathInfoLog((struct MCDRV_DIOPATH_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_SWAP:
		MakeSwapInfoLog((struct MCDRV_SWAP_INFO *)pvPrm1);
		break;
	case	MCDRV_GET_DSP:
		MakeDspPrmLog((struct MCDRV_DSP_PARAM *)pvPrm1, pvPrm2, dPrm);
		break;
	case	MCDRV_GET_DSP_DATA:
		MakeDspLog((UINT8 *)pvPrm1, dPrm);
		break;
	case	MCDRV_GET_HSDET:
		MakeHSDETInfoLog((struct MCDRV_HSDET_INFO *)pvPrm1);
		break;
	case	MCDRV_GETSET_GP:
		MakeGetSetGPLog((UINT8 *)pvPrm1);
		break;

	default:
		break;
	}
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	if (bLevel < 3) {
		;
		return;
	}

	OutputRegDump();
}

/****************************************************************************
 *	McDebugLog_FuncIn
 *
 *	Description:
 *			Output Function entrance log.
 *	Arguments:
 *			pbFuncName	function name
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McDebugLog_FuncIn(
	void	*pvFuncName
)
{
	strcpy(gsbLogString, (CHAR *)pvFuncName);
	strcat(gsbLogString, " In");

	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
}

/****************************************************************************
 *	McDebugLog_FuncOut
 *
 *	Description:
 *			Output Function exit log.
 *	Arguments:
 *			pbFuncName	function name
 *			psdRet		retrun value
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McDebugLog_FuncOut(
	void	*pvFuncName,
	const SINT32	*psdRet
)
{
	CHAR	sbStr[80];

	strcpy(gsbLogString, (CHAR *)pvFuncName);
	strcat(gsbLogString, " Out");
	if (psdRet != NULL) {
		sprintf(sbStr, " ret=%ld", *psdRet);
		strcat(gsbLogString, sbStr);
	}

	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
}


/****************************************************************************
 *	OutputRegDump
 *
 *	Description:
 *			Output Register dump.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	OutputRegDump
(
	void
)
{
	UINT16	i;
	CHAR	sbStr[10];
	UINT8	bSlaveAddr_dig, bSlaveAddr_ana;
	struct MCDRV_REG_INFO	sRegInfo;

	bSlaveAddr_dig	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
	bSlaveAddr_ana	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);

	/*	IF_REG	*/
	sRegInfo.bRegType	= MCDRV_REGTYPE_IF;
	strcpy(gsbLogString, "IF_REG:");
	for (i = 0; i < 256UL; i++) {
		sRegInfo.bAddress	= (UINT8)i;
		if ((McResCtrl_GetRegAccess(&sRegInfo)
			& eMCDRV_CAN_READ) != 0) {
			sprintf(sbStr, "[%d]=%02X",
				i, McSrv_ReadReg(bSlaveAddr_dig, i));
			strcat(gsbLogString, sbStr);
			if (i < 255UL) {
				;
				strcat(gsbLogString, " ");
			}
		}
	}
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	A_REG	*/
	strcpy(gsbLogString, "A_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_A_REG_A, MCI_A_REG_D, MCDRV_REGTYPE_A);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	MA_REG	*/
	strcpy(gsbLogString, "MA_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_MA_REG_A, MCI_MA_REG_D, MCDRV_REGTYPE_MA);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	MB_REG	*/
	strcpy(gsbLogString, "MB_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_MB_REG_A, MCI_MB_REG_D, MCDRV_REGTYPE_MB);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	B_REG	*/
	strcpy(gsbLogString, "B_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_B_REG_A, MCI_B_REG_D, MCDRV_REGTYPE_B);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	E_REG	*/
	strcpy(gsbLogString, "E_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_E_REG_A, MCI_E_REG_D, MCDRV_REGTYPE_E);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	C_REG	*/
	strcpy(gsbLogString, "C_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_C_REG_A, MCI_C_REG_D, MCDRV_REGTYPE_C);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	F_REG	*/
	strcpy(gsbLogString, "F_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_dig,
		MCI_F_REG_A, MCI_F_REG_D, MCDRV_REGTYPE_F);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	ANA_REG	*/
	strcpy(gsbLogString, "ANA_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_ana,
		MCI_ANA_REG_A, MCI_ANA_REG_D, MCDRV_REGTYPE_ANA);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);

	/*	CD_REG	*/
	strcpy(gsbLogString, "CD_REG:");
	GetRegDump(gsbLogString, bSlaveAddr_ana,
		MCI_CD_REG_A, MCI_CD_REG_D, MCDRV_REGTYPE_CD);
	strcat(gsbLogString, gsbLFCode);
	machdep_DebugPrint((UINT8 *)(void *)gsbLogString);
}

/****************************************************************************
 *	GetRegDump
 *
 *	Description:
 *			Get Register dump string.
 *	Arguments:
 *			psbLogString	string buffer
 *			bSlaveAddr	Slave address
 *			bADRAddr	ADR address
 *			bWINDOWAddr	WINDOW address
 *			bRegType	register type
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	GetRegDump
(
	CHAR	*psbLogString,
	UINT8	bSlaveAddr,
	UINT8	bADRAddr,
	UINT8	bWINDOWAddr,
	UINT8	bRegType
)
{
	UINT16	i;
	CHAR	sbStr[10];
	UINT8	abData[2];
	struct MCDRV_REG_INFO	sRegInfo;

	abData[0]	= bADRAddr<<1;
	sRegInfo.bRegType	= bRegType;

	for (i = 0; i < 256UL; i++) {
		sRegInfo.bAddress	= (UINT8)i;
		if ((McResCtrl_GetRegAccess(&sRegInfo)
			& eMCDRV_CAN_READ) != 0) {
			abData[1]	= (UINT8)i;
			McSrv_WriteReg(bSlaveAddr, abData, 2);
			sprintf(sbStr, "[%d]=%02X",
				i, McSrv_ReadReg(bSlaveAddr, bWINDOWAddr));
			strcat(psbLogString, sbStr);
			if (i < 255UL) {
				;
				strcat(psbLogString, " ");
			}
		}
	}
}

/****************************************************************************
 *	MakeInitInfoLog
 *
 *	Description:
 *			Make Init Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeInitInfoLog
(
	const struct MCDRV_INIT_INFO	*pvPrm1
)
{
	CHAR	sbStr[80];
	int	i;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " bCkSel=%02X", pvPrm1->bCkSel);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bCkInput=%02X", pvPrm1->bCkInput);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllModeA=%02X", pvPrm1->bPllModeA);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllPrevDivA=%02X", pvPrm1->bPllPrevDivA);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " wPllFbDivA=%04X", pvPrm1->wPllFbDivA);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " wPllFracA=%04X", pvPrm1->wPllFracA);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllFreqA=%02X", pvPrm1->bPllFreqA);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllModeB=%02X", pvPrm1->bPllModeB);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllPrevDivB=%02X", pvPrm1->bPllPrevDivB);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " wPllFbDivB=%04X", pvPrm1->wPllFbDivB);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " wPllFracB=%04X", pvPrm1->wPllFracB);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPllFreqB=%02X", pvPrm1->bPllFreqB);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bHsdetClk=%02X", pvPrm1->bHsdetClk);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio0SdoHiz=%02X", pvPrm1->bDio0SdoHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio1SdoHiz=%02X", pvPrm1->bDio1SdoHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio2SdoHiz=%02X", pvPrm1->bDio2SdoHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio0ClkHiz=%02X", pvPrm1->bDio0ClkHiz);
	strcat(gsbLogString, sbStr);			
	sprintf(sbStr, " bDio1ClkHiz=%02X", pvPrm1->bDio1ClkHiz);
	strcat(gsbLogString, sbStr);			
	sprintf(sbStr, " bDio2ClkHiz=%02X", pvPrm1->bDio2ClkHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio0PcmHiz=%02X", pvPrm1->bDio0PcmHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio1PcmHiz=%02X", pvPrm1->bDio1PcmHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bDio2PcmHiz=%02X", pvPrm1->bDio2PcmHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPa0Func=%02X", pvPrm1->bPa0Func);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPa1Func=%02X", pvPrm1->bPa1Func);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPa2Func=%02X", pvPrm1->bPa2Func);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPowerMode=%02X", pvPrm1->bPowerMode);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMbSel1=%02X", pvPrm1->bMbSel1);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMbSel2=%02X", pvPrm1->bMbSel2);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMbSel3=%02X", pvPrm1->bMbSel3);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMbSel4=%02X", pvPrm1->bMbSel4);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMbsDisch=%02X", pvPrm1->bMbsDisch);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bNonClip=%02X", pvPrm1->bNonClip);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bLineIn1Dif=%02X", pvPrm1->bLineIn1Dif);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bLineOut1Dif=%02X", pvPrm1->bLineOut1Dif);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bLineOut2Dif=%02X", pvPrm1->bLineOut2Dif);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMic1Sng=%02X", pvPrm1->bMic1Sng);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMic2Sng=%02X", pvPrm1->bMic2Sng);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMic3Sng=%02X", pvPrm1->bMic3Sng);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bMic4Sng=%02X", pvPrm1->bMic4Sng);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bZcLineOut1=%02X", pvPrm1->bZcLineOut1);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bZcLineOut2=%02X", pvPrm1->bZcLineOut2);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bZcRc=%02X", pvPrm1->bZcRc);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bZcSp=%02X", pvPrm1->bZcSp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bZcHp=%02X", pvPrm1->bZcHp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSvolLineOut1=%02X", pvPrm1->bSvolLineOut1);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSvolLineOut2=%02X", pvPrm1->bSvolLineOut2);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSvolRc=%02X", pvPrm1->bSvolRc);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSvolSp=%02X", pvPrm1->bSvolSp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSvolHp=%02X", pvPrm1->bSvolHp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bRcHiz=%02X", pvPrm1->bRcHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bSpHiz=%02X", pvPrm1->bSpHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bHpHiz=%02X", pvPrm1->bHpHiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bLineOut1Hiz=%02X", pvPrm1->bLineOut1Hiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bLineOut2Hiz=%02X", pvPrm1->bLineOut2Hiz);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bCpMod=%02X", pvPrm1->bCpMod);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bRbSel=%02X", pvPrm1->bRbSel);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPlugSel=%02X", pvPrm1->bPlugSel);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bGndDet=%02X", pvPrm1->bGndDet);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPpdRc=%02X", pvPrm1->bPpdRc);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPpdSp=%02X", pvPrm1->bPpdSp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " =bPpdHp%02X", pvPrm1->bPpdHp);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPpdLineOut1=%02X", pvPrm1->bPpdLineOut1);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bPpdLineOut2=%02X", pvPrm1->bPpdLineOut2);
	strcat(gsbLogString, sbStr);

	for (i = 0; i < 20; i++) {
		sprintf(sbStr, " dWaitTime[%02d]=%lu",
			i, pvPrm1->sWaitTime.dWaitTime[i]);
		strcat(gsbLogString, sbStr);
	}
	for (i = 0; i < 20; i++) {
		sprintf(sbStr, " dPollInterval[%02d]=%lu",
			i, pvPrm1->sWaitTime.dPollInterval[i]);
		strcat(gsbLogString, sbStr);
	}
	for (i = 0; i < 20; i++) {
		sprintf(sbStr, " dPollTimeOut[%02d]=%lu",
			i, pvPrm1->sWaitTime.dPollTimeOut[i]);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeRegInfoLog
 *
 *	Description:
 *			Make Reg Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeRegInfoLog
(
	const struct MCDRV_REG_INFO *pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " bRegType=%02X", pvPrm1->bRegType);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bAddress=%02X", pvPrm1->bAddress);
	strcat(gsbLogString, sbStr);
	sprintf(sbStr, " bData=%02X", pvPrm1->bData);
	strcat(gsbLogString, sbStr);
}


/****************************************************************************
 *	MakeClockSwInfoLog
 *
 *	Description:
 *			Make Clock Switch Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeClockSwInfoLog
(
	const struct MCDRV_CLOCKSW_INFO *pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " bClkSrc=%02X", pvPrm1->bClkSrc);
	strcat(gsbLogString, sbStr);
}

/****************************************************************************
 *	MakePathInfoLog
 *
 *	Description:
 *			Make Path Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakePathInfoLog
(
	const struct MCDRV_PATH_INFO *pvPrm1
)
{
	UINT8	bCh;
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	for (bCh = 0; bCh < MUSICOUT_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asMusicOut[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asMusicOut[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < EXTOUT_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asExtOut[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asExtOut[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < HIFIOUT_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asHifiOut[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asHifiOut[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VBOXMIXIN_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asVboxMixIn[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asVboxMixIn[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAe0[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAe0[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAe1[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAe1[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAe2[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAe2[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < AE_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAe3[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAe3[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DAC0_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asDac0[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asDac0[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DAC1_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asDac1[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asDac1[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VOICEOUT_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asVoiceOut[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asVoiceOut[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VBOXIOIN_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asVboxIoIn[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asVboxIoIn[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VBOXHOSTIN_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asVboxHostIn[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asVboxHostIn[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < HOSTOUT_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asHostOut[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asHostOut[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF0_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAdif0[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAdif0[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF1_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAdif1[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAdif1[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF2_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAdif2[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAdif2[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}

	for (bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAdc0[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAdc0[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asAdc1[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asAdc1[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < SP_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asSp[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asSp[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < HP_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asHp[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asHp[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < RC_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asRc[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asRc[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asLout1[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asLout1[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asLout2[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asLout2[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++) {
		sprintf(sbStr, " asBias[%d].dSrcOnOff=%08lX",
			bCh, pvPrm1->asBias[bCh].dSrcOnOff);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeVolInfoLog
 *
 *	Description:
 *			Make Volume Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeVolInfoLog
(
	const struct MCDRV_VOL_INFO *pvPrm1
)
{
	UINT8	bCh;
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	for (bCh = 0; bCh < MUSICIN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_MusicIn[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_MusicIn[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < EXTIN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_ExtIn[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_ExtIn[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VOICEIN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_VoiceIn[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_VoiceIn[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < REFIN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_RefIn[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_RefIn[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF0IN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_Adif0In[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_Adif0In[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF1IN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_Adif1In[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_Adif1In[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < ADIF2IN_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_Adif2In[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_Adif2In[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < MUSICOUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_MusicOut[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_MusicOut[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < EXTOUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_ExtOut[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_ExtOut[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < VOICEOUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_VoiceOut[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_VoiceOut[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < REFOUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_RefOut[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_RefOut[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DAC0OUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_Dac0Out[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_Dac0Out[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DAC1OUT_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_Dac1Out[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_Dac1Out[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DPATH_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_DpathDa[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_DpathDa[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < DPATH_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswD_DpathAd[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswD_DpathAd[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < LINEIN1_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_LineIn1[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_LineIn1[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Mic1[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Mic1[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Mic2[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Mic2[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Mic3[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Mic3[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < MIC4_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Mic4[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Mic4[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < HP_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Hp[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Hp[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < SP_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Sp[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Sp[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < RC_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_Rc[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_Rc[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < LINEOUT1_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_LineOut1[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_LineOut1[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < LINEOUT2_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_LineOut2[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_LineOut2[bCh]);
		strcat(gsbLogString, sbStr);
	}
	for (bCh = 0; bCh < HPDET_VOL_CHANNELS; bCh++) {
		sprintf(sbStr, " aswA_HpDet[%d]=%04X",
			bCh, (UINT16)pvPrm1->aswA_HpDet[bCh]);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeDIOInfoLog
 *
 *	Description:
 *			Make Digital I/O Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeDIOInfoLog
(
	const struct MCDRV_DIO_INFO *pvPrm1
)
{
	CHAR	sbStr[80];
	UINT8	bPort;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	for (bPort = 0; bPort < 4; bPort++) {
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bMasterSlave=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bMasterSlave);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bAutoFs=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bAutoFs);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bFs=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bFs);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bBckFs=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bBckFs);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bInterface=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bInterface);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bBckInvert=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bBckInvert);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bSrcThru=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bSrcThru);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bPcmHizTim=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bPcmHizTim);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDioCommon.bPcmFrame=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bPcmFrame);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr,
			" asPortInfo[%d].sDioCommon.bPcmHighPeriod=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDioCommon.bPcmHighPeriod);
		strcat(gsbLogString, sbStr);

		sprintf(sbStr, " asPortInfo[%d].sDir.sDaFormat.bBitSel=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDir.sDaFormat.bBitSel);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDir.sDaFormat.bMode=%02X",
			bPort, pvPrm1->asPortInfo[bPort].sDir.sDaFormat.bMode);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDir.sPcmFormat.bMono=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDir.sPcmFormat.bMono);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDir.sPcmFormat.bOrder=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDir.sPcmFormat.bOrder);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDir.sPcmFormat.bLaw=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDir.sPcmFormat.bLaw);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDir.sPcmFormat.bBitSel=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDir.sPcmFormat.bBitSel);
		strcat(gsbLogString, sbStr);

		sprintf(sbStr, " asPortInfo[%d].sDit.bStMode=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.bStMode);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.bEdge=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.bEdge);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sDaFormat.bBitSel=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sDaFormat.bBitSel);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sDaFormat.bMode=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sDaFormat.bMode);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sPcmFormat.bMono=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sPcmFormat.bMono);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sPcmFormat.bOrder=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sPcmFormat.bOrder);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sPcmFormat.bLaw=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sPcmFormat.bLaw);
		strcat(gsbLogString, sbStr);
		sprintf(sbStr, " asPortInfo[%d].sDit.sPcmFormat.bBitSel=%02X",
			bPort,
			pvPrm1->asPortInfo[bPort].sDit.sPcmFormat.bBitSel);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeDIOPathInfoLog
 *
 *	Description:
 *			Make Digital I/O path Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeDIOPathInfoLog
(
	const struct MCDRV_DIOPATH_INFO *pvPrm1
)
{
	CHAR	sbStr[80];
	UINT8	bPort;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	for (bPort = 0; bPort < 4; bPort++) {
		sprintf(sbStr, " abPhysPort[%d]=%02X",
			bPort, pvPrm1->abPhysPort[bPort]);
		strcat(gsbLogString, sbStr);
	}
	sprintf(sbStr, " bMusicCh=%02X", pvPrm1->bMusicCh);
	strcat(gsbLogString, sbStr);
	for (bPort = 0; bPort < 3; bPort++) {
		sprintf(sbStr, " abMusicRSlot[%d]=%02X",
			bPort, pvPrm1->abMusicRSlot[bPort]);
		strcat(gsbLogString, sbStr);
	}
	for (bPort = 0; bPort < 3; bPort++) {
		sprintf(sbStr, " abMusicTSlot[%d]=%02X",
			bPort, pvPrm1->abMusicTSlot[bPort]);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeSwapInfoLog
 *
 *	Description:
 *			Make Swap Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeSwapInfoLog
(
	const struct MCDRV_SWAP_INFO *pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " bAdif0=%02X", pvPrm1->bAdif0);
	sprintf(sbStr, " bAdif1=%02X", pvPrm1->bAdif1);
	sprintf(sbStr, " bAdif2=%02X", pvPrm1->bAdif2);
	sprintf(sbStr, " bDac0=%02X", pvPrm1->bDac0);
	sprintf(sbStr, " bDac1=%02X", pvPrm1->bDac1);
	sprintf(sbStr, " bMusicIn0=%02X", pvPrm1->bMusicIn0);
	sprintf(sbStr, " bMusicIn1=%02X", pvPrm1->bMusicIn1);
	sprintf(sbStr, " bMusicIn2=%02X", pvPrm1->bMusicIn2);
	sprintf(sbStr, " bExtIn=%02X", pvPrm1->bExtIn);
	sprintf(sbStr, " bVoiceIn=%02X", pvPrm1->bVoiceIn);
	sprintf(sbStr, " bHifiIn=%02X", pvPrm1->bHifiIn);
	sprintf(sbStr, " bMusicOut0=%02X", pvPrm1->bMusicOut0);
	sprintf(sbStr, " bMusicOut1=%02X", pvPrm1->bMusicOut1);
	sprintf(sbStr, " bMusicOut2=%02X", pvPrm1->bMusicOut2);
	sprintf(sbStr, " bExtOut=%02X", pvPrm1->bExtOut);
	sprintf(sbStr, " bVoiceOut=%02X", pvPrm1->bVoiceOut);
	sprintf(sbStr, " bHifiOut=%02X", pvPrm1->bHifiOut);
	strcat(gsbLogString, sbStr);
}

/****************************************************************************
 *	MakeDspLog
 *
 *	Description:
 *			Make DSP Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeDspLog(
	const UINT8	*pvPrm1,
	UINT32		dPrm
)
{
	CHAR	sbStr[80];
	UINT32	i;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}
	sprintf(sbStr, " param=");
	strcat(gsbLogString, sbStr);
	for (i = 0; i < dPrm; i++) {
		sprintf(sbStr, " %d", pvPrm1[i]);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeDspPrmLog
 *
 *	Description:
 *			Make DSP Param Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeDspPrmLog
(
	const struct MCDRV_DSP_PARAM	*pvPrm1,
	const void		*pvPrm2,
	UINT32			dPrm
)
{
	CHAR	sbStr[80];
	UINT32	i;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " pvPrm1=NULL");
		return;
	}
	if (pvPrm2 == NULL) {
		strcat(gsbLogString, " pvPrm2=NULL");
		return;
	}
	if ((pvPrm1->dType == MCDRV_DSP_PARAM_CDSP_INPOS)
	|| (pvPrm1->dType == MCDRV_DSP_PARAM_CDSP_OUTPOS)
	|| (pvPrm1->dType == MCDRV_DSP_PARAM_CDSP_DFIFO_REMAIN)
	|| (pvPrm1->dType == MCDRV_DSP_PARAM_CDSP_RFIFO_REMAIN)) {
		sprintf(sbStr, " param=%ld", *((UINT32 *)pvPrm2));
		strcat(gsbLogString, sbStr);
	} else {
		sprintf(sbStr, " param=");
		strcat(gsbLogString, sbStr);
		for (i = 0; i < dPrm; i++) {
			sprintf(sbStr, " %d", ((UINT8 *)pvPrm2)[i]);
			strcat(gsbLogString, sbStr);
		}
	}
}

/****************************************************************************
 *	MakeHSDETInfoLog
 *
 *	Description:
 *			Make HSDET Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeHSDETInfoLog
(
	const struct MCDRV_HSDET_INFO *pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " bEnPlugDet=%02X", pvPrm1->bEnPlugDet);
	sprintf(sbStr, " bEnPlugDetDb=%02X", pvPrm1->bEnPlugDetDb);
	sprintf(sbStr, " bEnDlyKeyOff=%02X", pvPrm1->bEnDlyKeyOff);
	sprintf(sbStr, " bEnDlyKeyOn=%02X", pvPrm1->bEnDlyKeyOn);
	sprintf(sbStr, " bEnMicDet=%02X", pvPrm1->bEnMicDet);
	sprintf(sbStr, " bEnKeyOff=%02X", pvPrm1->bEnKeyOff);
	sprintf(sbStr, " bEnKeyOn=%02X", pvPrm1->bEnKeyOn);
	sprintf(sbStr, " bHsDetDbnc=%02X", pvPrm1->bHsDetDbnc);
	sprintf(sbStr, " bKeyOffMtim=%02X", pvPrm1->bKeyOffMtim);
	sprintf(sbStr, " bKeyOnMtim=%02X", pvPrm1->bKeyOnMtim);
	sprintf(sbStr, " bKey0OffDlyTim=%02X", pvPrm1->bKey0OffDlyTim);
	sprintf(sbStr, " bKey1OffDlyTim=%02X", pvPrm1->bKey1OffDlyTim);
	sprintf(sbStr, " bKey2OffDlyTim=%02X", pvPrm1->bKey2OffDlyTim);
	sprintf(sbStr, " bKey0OnDlyTim=%02X", pvPrm1->bKey0OnDlyTim);
	sprintf(sbStr, " bKey1OnDlyTim=%02X", pvPrm1->bKey1OnDlyTim);
	sprintf(sbStr, " bKey2OnDlyTim=%02X", pvPrm1->bKey2OnDlyTim);
	sprintf(sbStr, " bKey0OnDlyTim2=%02X", pvPrm1->bKey0OnDlyTim2);
	sprintf(sbStr, " bKey1OnDlyTim2=%02X", pvPrm1->bKey1OnDlyTim2);
	sprintf(sbStr, " bKey2OnDlyTim2=%02X", pvPrm1->bKey2OnDlyTim2);
	sprintf(sbStr, " bIrqType=%02X", pvPrm1->bIrqType);
	sprintf(sbStr, " bDetInInv=%02X", pvPrm1->bDetInInv);
	sprintf(sbStr, " bHsDetMode=%02X", pvPrm1->bHsDetMode);
	sprintf(sbStr, " bSperiod=%02X", pvPrm1->bSperiod);
	sprintf(sbStr, " bLperiod=%02X", pvPrm1->bLperiod);
	sprintf(sbStr, " bDbncNumPlug=%02X", pvPrm1->bDbncNumPlug);
	sprintf(sbStr, " bDbncNumMic=%02X", pvPrm1->bDbncNumMic);
	sprintf(sbStr, " bDbncNumKey=%02X", pvPrm1->bDbncNumKey);
	sprintf(sbStr, " bSgnlPeriod=%02X", pvPrm1->bSgnlPeriod);
	sprintf(sbStr, " bSgnlNum=%02X", pvPrm1->bSgnlNum);
	sprintf(sbStr, " bSgnlPeak=%02X", pvPrm1->bSgnlPeak);
	sprintf(sbStr, " bImpSel=%02X", pvPrm1->bImpSel);
	sprintf(sbStr, " bDlyIrqStop=%02X", pvPrm1->bDlyIrqStop);
	sprintf(sbStr, " cbfunc=%p", pvPrm1->cbfunc);
	strcat(gsbLogString, sbStr);
}

/****************************************************************************
 *	MakeGPModeLog
 *
 *	Description:
 *			Make GPIO mode Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeGPModeLog
(
	const struct MCDRV_GP_MODE	*pvPrm1
)
{
	CHAR	sbStr[80];
	UINT8	bPadNo;

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	for (bPadNo = 0; bPadNo < 3; bPadNo++) {
		sprintf(sbStr, " abGpDdr[%d]=%02X",
			bPadNo, pvPrm1->abGpDdr[bPadNo]);
		strcat(gsbLogString, sbStr);
	}
	for (bPadNo = 0; bPadNo < 3; bPadNo++) {
		sprintf(sbStr, " abGpHost[%d]=%02X",
			bPadNo, pvPrm1->abGpHost[bPadNo]);
		strcat(gsbLogString, sbStr);
	}
	for (bPadNo = 0; bPadNo < 3; bPadNo++) {
		sprintf(sbStr, " abGpInvert[%d]=%02X",
			bPadNo, pvPrm1->abGpInvert[bPadNo]);
		strcat(gsbLogString, sbStr);
	}
}

/****************************************************************************
 *	MakeGPMaskLog
 *
 *	Description:
 *			Make GPIO Mask Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeGPMaskLog
(
	const UINT8	*pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " mask=%02X", *pvPrm1);
	strcat(gsbLogString, sbStr);
}

/****************************************************************************
 *	MakeGetSetGPLog
 *
 *	Description:
 *			Make Get/Set GPIO Info Parameter log.
 *	Arguments:
 *			pvPrm1	pointer to parameter
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	MakeGetSetGPLog
(
	const UINT8	*pvPrm1
)
{
	CHAR	sbStr[80];

	if (pvPrm1 == NULL) {
		strcat(gsbLogString, " param=NULL");
		return;
	}

	sprintf(sbStr, " HiLow=%02X", *pvPrm1);
	strcat(gsbLogString, sbStr);
}



#endif	/*	MCDRV_DEBUG_LEVEL	*/
