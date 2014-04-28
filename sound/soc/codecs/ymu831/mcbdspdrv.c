/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcbdspdrv.c
 *
 *		Description	: MC Bdsp Driver
 *
 *		Version		: 1.0.0	2012.12.13
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
#include "mctypedef.h"
#include "mcdevif.h"
#include "mcdefs.h"
#include "mcresctrl.h"
#include "mcbdspdrv.h"
#include "mcservice.h"


/* inside definition */
#define BDSP_STATUS_IDLE		(0)
#define BDSP_STATUS_INITED		(1)

#define CHUNK_SIZE			(8)

#define AEC_BDSP_TAG_FWCTRL_B		(0x00000C00)
#define FWCTRL_B_SIZE			(14)
#define AEC_BDSP_TAG_APP_COEF		(0x00000700)
#define APP_COEF_FIX_SIZE		(9)
#define AEC_BDSP_TAG_APP_REG		(0x00000A00)
#define APP_REG_FIX_SIZE		(8)
#define AEC_BDSP_TAG_APP_MASK		(0xFFFFFF00)
#define AEC_BDSP_TAG_APPNO_MASK		(0x000000FF)

#define FWCTRL_B_BYPASS			(0)
#define FWCTRL_B_AE0_BYP		(1)
#define FWCTRL_B_AE1_BYP		(2)
#define FWCTRL_B_WIDE			(3)
#define FWCTRL_B_HEX			(4)
#define FWCTRL_B_EQ3_0A			(5)
#define FWCTRL_B_DRC3			(6)
#define FWCTRL_B_DRC_0			(7)
#define FWCTRL_B_EQ3_0B			(8)
#define FWCTRL_B_GEN			(9)
#define FWCTRL_B_EQ3_1A			(10)
#define FWCTRL_B_AGC			(11)
#define FWCTRL_B_DRC_1			(12)
#define FWCTRL_B_EQ3_1B			(13)

#define APP_NO_GEN			0
#define APP_NO_SWAP_0			1
#define APP_NO_WIDE			2
#define APP_NO_HEX			3
#define APP_NO_EQ3_0A			4
#define APP_NO_DRC3			5
#define APP_NO_DRC_0			6
#define APP_NO_EQ3_0B			7
#define APP_NO_MIX_0			8
#define APP_NO_SWAP_1			9
#define APP_NO_EQ3_1A			10
#define APP_NO_AGC			11
#define APP_NO_DRC_1			12
#define APP_NO_EQ3_1B			13
#define APP_NO_MIX_1			14

#define APP_COEF_ADR			(0)
#define APP_COEF_SIZE			(5)
#define APP_COEF_DATA			(9)

#define APP_REG_FLG			(0)
#define APP_REG_FLG_FWCTL0		(0x01)
#define APP_REG_FLG_FWCTL1		(0x02)
#define APP_REG_FLG_FWCTL2		(0x04)
#define APP_REG_FLG_FWCTL3		(0x08)
#define APP_REG_FLG_FWCTL4		(0x10)
#define APP_REG_FLG_FWCTL5		(0x20)
#define APP_REG_FLG_FWCTL6		(0x40)
#define APP_REG_FWCTL0			(1)
#define APP_REG_FWCTL1			(2)
#define APP_REG_FWCTL2			(3)
#define APP_REG_FWCTL3			(4)
#define APP_REG_FWCTL4			(5)
#define APP_REG_FWCTL5			(6)
#define APP_REG_FWCTL6			(7)

#define COEF_DMA_TRANS			(0)
#define COEF_DSP_TRANS			(1)
#define COEF_DSP_TRANS_MAX		(40)

#define TARGET_NONE			(0x00000000)
#define TARGET_AE0_MASK			(0x00003F00)
#define TARGET_AE0_EQ3_0B		(0x00002000)
#define TARGET_AE0_DRC_0		(0x00001000)
#define TARGET_AE0_DRC3			(0x00000800)
#define TARGET_AE0_EQ3_0A		(0x00000400)
#define TARGET_AE0_HEX			(0x00000200)
#define TARGET_AE0_WIDE			(0x00000100)
#define TARGET_AE1_MASK			(0x0000003C)
#define TARGET_AE1_EQ3_1B		(0x00000020)
#define TARGET_AE1_DRC_1		(0x00000010)
#define TARGET_AE1_AGC			(0x00000008)
#define TARGET_AE1_EQ3_1A		(0x00000004)
#define TARGET_GEN_MASK			(0x00008000)
#define TARGET_GEN_GEN			(0x00008000)
#define TARGET_BASE_MASK		(0xF0000000)
#define TARGET_BASE_SWAP_0		(0x10000000)
#define TARGET_BASE_SWAP_1		(0x20000000)
#define TARGET_BASE_MIX_0		(0x40000000)
#define TARGET_BASE_MIX_1		(0x80000000)

#define REG_APP_NO_STOP			(0)
#define REG_APP_STOP			(1)

#define MULTI_CHUNK_APP_COEF		(1)
#define MULTI_CHUNK_APP_REG		(2)

#define RAM_UNIT_SIZE_32		(4UL)
#define RAM_UNIT_SIZE_64		(8UL)
#define DXRAM_RANGE_MIN			(0x0UL)
#define DXRAM_RANGE_MAX			(0x017FUL)
#define DYRAM_RANGE_MIN			(0x0UL)
#define DYRAM_RANGE_MAX			(0x01FFUL)

#define WAIT_NONE			(0x00)
#define CROSS_FADE_WAIT			(0x01)
#define SIN_OUT_WAIT			(0x02)

#define BDSP_PATH_NORMAL		(0)
#define BDSP_PATH_BYPASS		(1)
#define BDSP_PATH_DONTCARE		(2)

#define BDSP_BYPASS_FADE_TIME		(10000)

#define BDSP_SIN_CTRL_REG		(0)
#define BDSP_SIN_CTRL_GPIO		(1)

#define GEN_SIN_CTL_SEL_ADD		(0x001D9)

/* inside Struct */

struct MCDRV_BDSP_AEC_BDSP_INFO {
	UINT8				bBDspOnOff;
	UINT8				*pbChunkData;
	UINT32				dwSize;
	UINT8				*pbFwctrlB;
	UINT8				bAe0AppOnOff;
	UINT8				bAe1AppOnOff;
	UINT8				bAppGen;
	UINT32				dAppCoefCnt;
	UINT8				bCoefTrans;
	UINT32				dCoefTarget;
	UINT8				bSinCtrlSel;
	UINT32				dAppRegCnt;
	UINT32				dRegAppStopTarget;
	UINT8				bStoppedAppExec0;
	UINT8				bStoppedAppExec1;
	UINT8				bStoppedSinOut;
	UINT8				bStoppedBypass;
};

struct MCDRV_BDSP_INFO {
	/* state */
	UINT32				dStatus;
	UINT8				bDSPBypass;
	UINT8				bSinCtrlSel;
	/* register */
	UINT8				bDSPCtl;
	UINT8				bAppExec0;
	UINT8				bAppExec1;
	UINT8				bAEBypass;
};

static struct MCDRV_BDSP_INFO gsBdspInfo = {
	BDSP_STATUS_IDLE,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};

/****************************************************************************
 *	CreateUINT32
 *
 *	Description:
 *			Create UINT32 Value
 *	Arguments:
 *			b0	31-24bit value
 *			b1	23-16bit value
 *			b2	15-8bit value
 *			b3	7-0bit value
 *	Return:
 *			UINT32 Value
 *
 ****************************************************************************/
static UINT32 CreateUINT32(UINT8 b0, UINT8 b1, UINT8 b2, UINT8 b3)
{
	return ((UINT32)b0 << 24) | ((UINT32)b1 << 16)
					| ((UINT32)b2 << 8) | (UINT32)b3;
}

/***************************************************************************
 *	GetBDSPChunk
 *
 *	Function:
 *			Get BDSP chunk
 *	Arguments:
 *			pbPrm		MCDRV_AEC_INFO structure pointer
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void GetBDSPChunk(struct MCDRV_AEC_INFO *psPrm,
				struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	psBdspInfo->bBDspOnOff = BDSP_OFF;
	psBdspInfo->pbChunkData = NULL;
	psBdspInfo->dwSize = 0;

	if (psPrm->sAecAudioengine.bEnable == AEC_AUDIOENGINE_ENABLE) {
		psBdspInfo->bBDspOnOff = psPrm->sAecAudioengine.bAEOnOff;

		if (psBdspInfo->bBDspOnOff == BDSP_OFF)
			return;

		psBdspInfo->pbChunkData =
				psPrm->sAecAudioengine.sAecBDsp.pbChunkData;
		psBdspInfo->dwSize =
				psPrm->sAecAudioengine.sAecBDsp.dwSize;
	}

	if (psBdspInfo->pbChunkData != NULL)
		psBdspInfo->pbChunkData = &psBdspInfo->pbChunkData[CHUNK_SIZE];
}

/***************************************************************************
 *	AppOnOff
 *
 *	Function:
 *			change decision App On/Off
 *	Arguments:
 *			bOnOff		On/Off
 *			bApp		App bit
 *			bAppExec	App exec value
 *	Return:
 *			App bit
 *
 ****************************************************************************/
static UINT8 AppOnOff(UINT8 bOnOff, UINT8 bApp, UINT8 bAppExec)
{
	switch (bOnOff) {
	case BDSP_APP_EXEC_STOP:
		if ((bAppExec & bApp) == 0)
			bApp = 0x00;
		break;

	case BDSP_APP_EXEC_START:
		if ((bAppExec & bApp) != 0)
			bApp = 0x00;
		break;

	case BDSP_APP_EXEC_DONTCARE:
	default:
		bApp = 0x00;
		break;
	}

	return bApp;
}

/***************************************************************************
 *	CheckAppOnOff
 *
 *	Function:
 *			
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			Target Number
 *
 ****************************************************************************/
static void CheckAppOnOff(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 *pbPrm;

	pbPrm = psBdspInfo->pbFwctrlB;

	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_WIDE],
				MCB_AEEXEC0_WIDE, gsBdspInfo.bAppExec0);
	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_HEX],
				MCB_AEEXEC0_HEX, gsBdspInfo.bAppExec0);
	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_EQ3_0A],
				MCB_AEEXEC0_EQ3_0A, gsBdspInfo.bAppExec0);
	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_DRC3],
				MCB_AEEXEC0_DRC3, gsBdspInfo.bAppExec0);
	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_DRC_0],
				MCB_AEEXEC0_DRC_0, gsBdspInfo.bAppExec0);
	psBdspInfo->bAe0AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_EQ3_0B],
				MCB_AEEXEC0_EQ3_0B, gsBdspInfo.bAppExec0);

	psBdspInfo->bAe1AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_EQ3_1A],
				MCB_AEEXEC1_EQ3_1A, gsBdspInfo.bAppExec1);
	psBdspInfo->bAe1AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_AGC],
				MCB_AEEXEC1_AGC, gsBdspInfo.bAppExec1);
	psBdspInfo->bAe1AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_DRC_1],
				MCB_AEEXEC1_DRC_1, gsBdspInfo.bAppExec1);
	psBdspInfo->bAe1AppOnOff |= AppOnOff(pbPrm[FWCTRL_B_EQ3_1B],
				MCB_AEEXEC1_EQ3_1B, gsBdspInfo.bAppExec1);

	if ((BDSP_APP_EXEC_STOP == pbPrm[FWCTRL_B_GEN]) &&
		(gsBdspInfo.bAppExec0 & MCB_AEEXEC0_GEN) != 0)
		psBdspInfo->bAppGen |= MCB_AEEXEC0_GEN;
}

/***************************************************************************
 *	GetAppNoTarget
 *
 *	Function:
 *			Get app target
 *	Arguments:
 *			dAppNo		App Number
 *	Return:
 *			Target Number
 *
 ****************************************************************************/
static UINT32 GetAppNoTarget(UINT32 dAppNo)
{
	switch (dAppNo) {
	case APP_NO_GEN:
		return TARGET_GEN_GEN;
	case APP_NO_SWAP_0:
		return TARGET_BASE_SWAP_0;
	case APP_NO_WIDE:
		return TARGET_AE0_WIDE;
	case APP_NO_HEX:
		return TARGET_AE0_HEX;
	case APP_NO_EQ3_0A:
		return TARGET_AE0_EQ3_0A;
	case APP_NO_DRC3:
		return TARGET_AE0_DRC3;
	case APP_NO_DRC_0:
		return TARGET_AE0_DRC_0;
	case APP_NO_EQ3_0B:
		return TARGET_AE0_EQ3_0B;
	case APP_NO_MIX_0:
		return TARGET_BASE_MIX_0;
	case APP_NO_SWAP_1:
		return TARGET_BASE_SWAP_1;
	case APP_NO_EQ3_1A:
		return TARGET_AE1_EQ3_1A;
	case APP_NO_AGC:
		return TARGET_AE1_AGC;
	case APP_NO_DRC_1:
		return TARGET_AE1_DRC_1;
	case APP_NO_EQ3_1B:
		return TARGET_AE1_EQ3_1B;
	case APP_NO_MIX_1:
		return TARGET_BASE_MIX_1;
	default:
		break;
	}

	return TARGET_NONE;
}

/***************************************************************************
 *	CheckSinCtrlSel
 *
 *	Function:
 *			check sin control
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *			pbPrm		chunk data pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void CheckSinCtrlSel(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo,
								UINT8 *pbPrm)
{
	UINT32 dAdd;
	UINT32 dSize;

	dAdd = CreateUINT32(pbPrm[0UL], pbPrm[1UL], pbPrm[2UL], pbPrm[3UL]);
	dSize = CreateUINT32(pbPrm[5UL], pbPrm[6UL], pbPrm[7UL], pbPrm[8UL]);
	pbPrm = &pbPrm[9UL];

	if (GEN_SIN_CTL_SEL_ADD < dAdd)
		return;

	if ((dAdd + (dSize / 4UL)) < GEN_SIN_CTL_SEL_ADD)
		return;

	dAdd = GEN_SIN_CTL_SEL_ADD - dAdd;

	if ((pbPrm[(dAdd * 4) + 3] & BDSP_SIN_CTRL_GPIO) != 0)
		psBdspInfo->bSinCtrlSel = BDSP_SIN_CTRL_GPIO;
	else
		psBdspInfo->bSinCtrlSel = BDSP_SIN_CTRL_REG;
}

/***************************************************************************
 *	AppChunkAnalyze
 *
 *	Function:
 *			App analysis chunk
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *			dChunkId	chunk id
 *			dChunkSize	chunk size
 *			dChunkTop	chunk positon
 *			pbPrm		chunk data pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 AppChunkAnalyze(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo,
					UINT32 dChunkId, UINT32 dChunkSize,
					UINT32 dChunkTop, UINT8 *pbPrm)
{
	UINT32 dTag;
	UINT32 dAppNo;
	UINT32 dTarget;
	UINT32 dTemp;

	dTag = (dChunkId & (UINT32)AEC_BDSP_TAG_APP_MASK);
	if ((dTag != AEC_BDSP_TAG_APP_COEF) && (dTag != AEC_BDSP_TAG_APP_REG))
		return MCDRV_SUCCESS;

	dAppNo = (dChunkId & (UINT32)AEC_BDSP_TAG_APPNO_MASK);
	dTarget = GetAppNoTarget(dAppNo);

	if (dTag == AEC_BDSP_TAG_APP_COEF) {
		if (dChunkSize < (UINT32)APP_COEF_FIX_SIZE)
			return MCDRV_ERROR_ARGUMENT;
		dTemp = CreateUINT32(pbPrm[dChunkTop + 5UL],
					pbPrm[dChunkTop + 6UL],
					pbPrm[dChunkTop + 7UL],
					pbPrm[dChunkTop + 8UL]);
		if (dTemp == 0UL)
			return MCDRV_SUCCESS;
		if ((dTemp & 0x03UL) != 0UL)
			return MCDRV_ERROR_ARGUMENT;
		if (dChunkSize < (dTemp + (UINT32)APP_COEF_FIX_SIZE))
			return MCDRV_ERROR_ARGUMENT;

		++(psBdspInfo->dAppCoefCnt);

		if ((COEF_DSP_TRANS != pbPrm[dChunkTop + 4UL]) ||
			((UINT32)COEF_DSP_TRANS_MAX < dTemp) ||
			(1UL < psBdspInfo->dAppCoefCnt))
			psBdspInfo->bCoefTrans = COEF_DMA_TRANS;

		psBdspInfo->dCoefTarget |= dTarget;

		if (dTarget == TARGET_GEN_GEN)
			CheckSinCtrlSel(psBdspInfo, &pbPrm[dChunkTop]);
	} else {
		if (dChunkSize < (UINT32)APP_REG_FIX_SIZE)
			return MCDRV_ERROR_ARGUMENT;

		if (TARGET_GEN_GEN != dTarget)
			return MCDRV_SUCCESS;

		++(psBdspInfo->dAppRegCnt);

		if ((pbPrm[dChunkTop + APP_REG_FLG] & APP_REG_FLG_FWCTL4) != 0)
			psBdspInfo->dRegAppStopTarget |= TARGET_GEN_GEN;
	}

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	BdspChunkAnalyze
 *
 *	Function:
 *			Bdsp analysis chunk
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 BdspChunkAnalyze(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT8 *pbPrm;
	UINT32 dSize;

	psBdspInfo->pbFwctrlB = NULL;
	psBdspInfo->dAppCoefCnt = 0UL;
	psBdspInfo->bCoefTrans = COEF_DSP_TRANS;
	psBdspInfo->bAe0AppOnOff = 0;
	psBdspInfo->bAe1AppOnOff = 0;
	psBdspInfo->bAppGen = 0;
	psBdspInfo->dCoefTarget = TARGET_NONE;
	psBdspInfo->bSinCtrlSel = gsBdspInfo.bSinCtrlSel;
	psBdspInfo->dAppRegCnt = 0UL;
	psBdspInfo->dRegAppStopTarget = TARGET_NONE;
	psBdspInfo->bStoppedAppExec0 = 0;
	psBdspInfo->bStoppedAppExec1 = 0;
	psBdspInfo->bStoppedSinOut = 0;
	psBdspInfo->bStoppedBypass = 0;

	if ((psBdspInfo->pbChunkData == NULL) || (psBdspInfo->dwSize == 0))
		return MCDRV_SUCCESS;

	pbPrm = psBdspInfo->pbChunkData;
	dSize = psBdspInfo->dwSize;
	dChunkTop = 0UL;
	while (dChunkTop < dSize) {
		if (dSize < (dChunkTop + (UINT32)CHUNK_SIZE))
			return MCDRV_ERROR_ARGUMENT;

		dChunkId = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);
		dChunkSize = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);
		if (dSize < (dChunkTop + (UINT32)CHUNK_SIZE + dChunkSize))
			return MCDRV_ERROR_ARGUMENT;

		dChunkTop += (UINT32)CHUNK_SIZE;
		switch (dChunkId) {
		case AEC_BDSP_TAG_FWCTRL_B:
			if (dChunkSize < (UINT32)FWCTRL_B_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psBdspInfo->pbFwctrlB != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psBdspInfo->pbFwctrlB = &pbPrm[dChunkTop];

			CheckAppOnOff(psBdspInfo);
			break;

		default:
			if (AppChunkAnalyze(psBdspInfo,
					dChunkId, dChunkSize, dChunkTop, pbPrm)
							!= MCDRV_SUCCESS)
				return MCDRV_ERROR_ARGUMENT;
		}
		dChunkTop += dChunkSize;
	}

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	DSPTransWait
 *
 *	Function:
 *			Wait for DSP transfer
 *	Arguments:
 *			none
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
static SINT32 DSPTransWait(void)
{
	SINT32 sdResult;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_IF_REG_FLAG_RESET
					| (((UINT32)MCI_BDSPTREQ) << 8)
					| (UINT32)MCB_BDSPTREQ),
					0);

	sdResult = McDevIf_ExecutePacket();

	return sdResult;
}

/***************************************************************************
 *	CrossFadeWait
 *
 *	Function:
 *			Cross fade wait
 *	Arguments:
 *			none
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
static SINT32 CrossFadeWait(void)
{
	SINT32 sdResult;

	McSrv_Sleep(BDSP_BYPASS_FADE_TIME);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_B_REG_FLAG_RESET
				| (((UINT32)MCI_AEFADE) << 8)
				| (UINT32)(MCB_AEFADE_AE1 | MCB_AEFADE_AE0)),
				0);

	sdResult = McDevIf_ExecutePacket();

	return sdResult;
}

/***************************************************************************
 *	SinOutWait
 *
 *	Function:
 *			Sin out wait
 *	Arguments:
 *			none
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
static SINT32 SinOutWait(void)
{
	SINT32 sdResult;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_B_REG_FLAG_RESET
					| (((UINT32)MCI_SINOUTFLG) << 8)
					| (UINT32)(MCB_SINOFLG)),
					0);

	sdResult = McDevIf_ExecutePacket();

	return sdResult;
}

/***************************************************************************
 *	BypassPath
 *
 *	Function:
 *			Bypass configuration path
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			Wait info
 *
 ****************************************************************************/
static UINT32 BypassPath(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bData;
	UINT8 bStopped;
	UINT32 dWait;

	bData = gsBdspInfo.bAEBypass;
	bStopped = 0;
	dWait = WAIT_NONE;

	if (psBdspInfo->bAe0AppOnOff != 0) {
		bData |= MCB_AEBYPASS_AE0;
		bStopped |= MCB_AEBYPASS_AE0;
	}

	if (psBdspInfo->bAe1AppOnOff != 0) {
		bData |= MCB_AEBYPASS_AE1;
		bStopped |= MCB_AEBYPASS_AE1;
	}

	if (psBdspInfo->bCoefTrans == COEF_DMA_TRANS) {
		if ((psBdspInfo->dCoefTarget & TARGET_AE0_MASK) != 0) {
			bData |= MCB_AEBYPASS_AE0;
			bStopped |= MCB_AEBYPASS_AE0;
		}

		if ((psBdspInfo->dCoefTarget & TARGET_AE1_MASK) != 0) {
			bData |= MCB_AEBYPASS_AE1;
			bStopped |= MCB_AEBYPASS_AE1;
		}
	}

	if (bData != gsBdspInfo.bAEBypass) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_AEBYPASS),
						bData);
		McDevIf_ExecutePacket();

		dWait = CROSS_FADE_WAIT;
		psBdspInfo->bStoppedBypass = bStopped;
		gsBdspInfo.bAEBypass = bData;
	}

	return dWait;
}

/***************************************************************************
 *	SinStop
 *
 *	Function:
 *			Sin stop
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			Wait info
 *
 ****************************************************************************/
static UINT32 SinStop(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bData;
	UINT32 dWait;

	dWait = WAIT_NONE;

	if (gsBdspInfo.bSinCtrlSel == BDSP_SIN_CTRL_GPIO)
		return dWait;

	if ((gsBdspInfo.bAppExec0 & MCB_AEEXEC0_GEN) == 0)
		return dWait;

	if (((psBdspInfo->bAppGen != 0) ||
		(psBdspInfo->dRegAppStopTarget & TARGET_GEN_MASK) != 0) ||
		((psBdspInfo->bCoefTrans == COEF_DMA_TRANS) &&
		((psBdspInfo->dCoefTarget & TARGET_GEN_MASK) != 0))) {

		dWait = SIN_OUT_WAIT;

		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_SINOUT),
						&bData, 1);
		if ((bData & MCB_SINOUT) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_SINOUT),
						MCI_SINOUT_DEF);
			McDevIf_ExecutePacket();

			psBdspInfo->bStoppedSinOut = MCB_SINOUT;
		}
	}

	return dWait;
}

/***************************************************************************
 *	AppStop
 *
 *	Function:
 *			App stop
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppStop(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bAppExec0;
	UINT8 bAppExec1;
	UINT8 bStoppedAppExec0;
	UINT8 bStoppedAppExec1;

	bAppExec0 = gsBdspInfo.bAppExec0;
	bAppExec1 = gsBdspInfo.bAppExec1;
	bStoppedAppExec0 = 0;
	bStoppedAppExec1 = 0;

	if (psBdspInfo->bCoefTrans == COEF_DMA_TRANS) {
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_EQ3_0B) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_EQ3_0B) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_EQ3_0B;
			bStoppedAppExec0 |= MCB_AEEXEC0_EQ3_0B;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_DRC_0) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_DRC_0) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_DRC_0;
			bStoppedAppExec0 |= MCB_AEEXEC0_DRC_0;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_DRC3) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_DRC3) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_DRC3;
			bStoppedAppExec0 |= MCB_AEEXEC0_DRC3;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_EQ3_0A) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_EQ3_0A) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_EQ3_0A;
			bStoppedAppExec0 |= MCB_AEEXEC0_EQ3_0A;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_HEX) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_HEX) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_HEX;
			bStoppedAppExec0 |= MCB_AEEXEC0_HEX;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE0_WIDE) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_WIDE) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_WIDE;
			bStoppedAppExec0 |= MCB_AEEXEC0_WIDE;
		}

		if (((psBdspInfo->dCoefTarget & TARGET_AE1_EQ3_1B) != 0) &&
			((bAppExec1 & MCB_AEEXEC1_EQ3_1B) != 0)) {
			bAppExec1 &= ~MCB_AEEXEC1_EQ3_1B;
			bStoppedAppExec1 |= MCB_AEEXEC1_EQ3_1B;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE1_DRC_1) != 0) &&
			((bAppExec1 & MCB_AEEXEC1_DRC_1) != 0)) {
			bAppExec1 &= ~MCB_AEEXEC1_DRC_1;
			bStoppedAppExec1 |= MCB_AEEXEC1_DRC_1;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE1_AGC) != 0) &&
			((bAppExec1 & MCB_AEEXEC1_AGC) != 0)) {
			bAppExec1 &= ~MCB_AEEXEC1_AGC;
			bStoppedAppExec1 |= MCB_AEEXEC1_AGC;
		}
		if (((psBdspInfo->dCoefTarget & TARGET_AE1_EQ3_1A) != 0) &&
			((bAppExec1 & MCB_AEEXEC1_EQ3_1A) != 0)) {
			bAppExec1 &= ~MCB_AEEXEC1_EQ3_1A;
			bStoppedAppExec1 |= MCB_AEEXEC1_EQ3_1A;
		}

		if (((psBdspInfo->dCoefTarget & TARGET_GEN_GEN) != 0) &&
			((bAppExec0 & MCB_AEEXEC0_GEN) != 0)) {
			bAppExec0 &= ~MCB_AEEXEC0_GEN;
			bStoppedAppExec0 |= MCB_AEEXEC0_GEN;
		}
	}

	if (((psBdspInfo->dRegAppStopTarget & TARGET_GEN_GEN) != 0) &&
		((bAppExec0 & MCB_AEEXEC0_GEN) != 0)) {
		bAppExec0 &= ~MCB_AEEXEC0_GEN;
		bStoppedAppExec0 |= MCB_AEEXEC0_GEN;
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC0),
				bAppExec0);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC1),
				bAppExec1);

	psBdspInfo->bStoppedAppExec0 = bStoppedAppExec0;
	psBdspInfo->bStoppedAppExec1 = bStoppedAppExec1;

	gsBdspInfo.bAppExec0 = bAppExec0;
	gsBdspInfo.bAppExec1 = bAppExec1;
}

/***************************************************************************
 *	Stop
 *
 *	Function:
 *			Stop
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
static SINT32 Stop(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	SINT32 sdResult;
	UINT32 dWait;

	dWait = WAIT_NONE;

	sdResult = DSPTransWait();
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	dWait |= BypassPath(psBdspInfo);

	dWait |= SinStop(psBdspInfo);

	if ((dWait & CROSS_FADE_WAIT) != 0) {
		sdResult = CrossFadeWait();
		if (sdResult < (SINT32)MCDRV_SUCCESS)
			return sdResult;
	}
	if ((dWait & SIN_OUT_WAIT) != 0) {
		sdResult = SinOutWait();
		if (sdResult < (SINT32)MCDRV_SUCCESS)
			return sdResult;
	}

	/* APP Stop */
	AppStop(psBdspInfo);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	EnableAppNo
 *
 *	Function:
 *			Number determination App
 *	Arguments:
 *			dAppNo		App Number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
static SINT32 EnableAppNo(UINT32 dAppNo)
{
	switch (dAppNo) {
	case APP_NO_GEN:
	case APP_NO_SWAP_0:
	case APP_NO_WIDE:
	case APP_NO_HEX:
	case APP_NO_EQ3_0A:
	case APP_NO_DRC3:
	case APP_NO_DRC_0:
	case APP_NO_EQ3_0B:
	case APP_NO_MIX_0:
	case APP_NO_SWAP_1:
	case APP_NO_EQ3_1A:
	case APP_NO_AGC:
	case APP_NO_DRC_1:
	case APP_NO_EQ3_1B:
	case APP_NO_MIX_1:
		return MCDRV_SUCCESS;

	default:
		break;
	}

	return MCDRV_ERROR;
}

/***************************************************************************
 *	AppCoefDL
 *
 *	Function:
 *			Download App coefficient
 *	Arguments:
 *			pbAppCoef	Coefficient data pointer
 *			bCoefTrans	Transmission mode
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppCoefDL(UINT8 *pbAppCoef, UINT8 bCoefTrans)
{
	UINT32 dSize;
	UINT32 i;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_BMAA0),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 1] & MCB_BMAA0));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_BMAA1),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 2] & MCB_BMAA1));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_BMAA2),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 3] & MCB_BMAA2));

	/* BMACtl */
	if (bCoefTrans == COEF_DSP_TRANS)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_BMACTL),
				(UINT8)(MCB_BDSPTINI | MCB_BMAMOD_DSP
							| MCB_BMABUS_Y));
	else
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMACTL),
						MCB_BMABUS_Y);

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbAppCoef[APP_COEF_SIZE + 0],
						pbAppCoef[APP_COEF_SIZE + 1],
						pbAppCoef[APP_COEF_SIZE + 2],
						pbAppCoef[APP_COEF_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAD),
						pbAppCoef[APP_COEF_DATA + i]);
	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	AppRegDL
 *
 *	Function:
 *			Download App register
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *			dTarget		Target
 *			pbAppReg	Register data pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppRegDL(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo,
					UINT32 dTarget, UINT8 *pbAppReg)
{
	switch (dTarget) {
	case TARGET_GEN_GEN:
		if ((pbAppReg[APP_REG_FLG] & APP_REG_FLG_FWCTL4) != 0)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_F01SEL),
						pbAppReg[APP_REG_FWCTL4]);

		if ((pbAppReg[APP_REG_FLG] & APP_REG_FLG_FWCTL5) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_SINOUT),
						pbAppReg[APP_REG_FWCTL5]);
			if ((pbAppReg[APP_REG_FWCTL5] & MCB_SINOUT) == 0)
				psBdspInfo->bStoppedSinOut = 0;
		}
		McDevIf_ExecutePacket();
		break;

	default:
		break;
	}
}

/***************************************************************************
 *	MultiChunkDL
 *
 *	Function:
 *			Download multiple chunks
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *			dTargetChunk	Target chunk
 *	Return:
 *			none
 *
 ****************************************************************************/
static void MultiChunkDL(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo,
							UINT32 dTargetChunk)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT32 dAppNo;
	UINT8 *pbPrm;
	UINT32 dSize;

	pbPrm = psBdspInfo->pbChunkData;
	dSize = psBdspInfo->dwSize;

	dChunkTop = 0;
	while (dChunkTop < dSize) {
		dChunkId = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);
		dChunkSize = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);

		dChunkTop += (UINT32)CHUNK_SIZE;

		dAppNo = (dChunkId & (UINT32)AEC_BDSP_TAG_APPNO_MASK);
		switch ((dChunkId & (UINT32)AEC_BDSP_TAG_APP_MASK)) {
		case AEC_BDSP_TAG_APP_COEF:
			if (dTargetChunk == (UINT32)MULTI_CHUNK_APP_COEF) {
				if (EnableAppNo(dAppNo) == MCDRV_SUCCESS)
					AppCoefDL(&pbPrm[dChunkTop],
						psBdspInfo->bCoefTrans);

				if (dAppNo == APP_NO_GEN)
					gsBdspInfo.bSinCtrlSel =
						psBdspInfo->bSinCtrlSel;
			}
			break;

		case AEC_BDSP_TAG_APP_REG:
			if (dTargetChunk == (UINT32)MULTI_CHUNK_APP_REG)
				AppRegDL(psBdspInfo, GetAppNoTarget(dAppNo),
							&pbPrm[dChunkTop]);
			break;

		default:
			/* unknown */
			break;
		}
		dChunkTop += dChunkSize;
	}
}

/***************************************************************************
 *	Download
 *
 *	Function:
 *			Download
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void Download(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	if (psBdspInfo->dAppCoefCnt != 0UL) {
		MultiChunkDL(psBdspInfo, MULTI_CHUNK_APP_COEF);

		if (psBdspInfo->bCoefTrans == COEF_DSP_TRANS) {
			/* DSPTReq */
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BDSPTREQ),
						MCB_BDSPTREQ);
			McDevIf_ExecutePacket();
		}
	}

	if (psBdspInfo->dAppRegCnt != 0UL)
		MultiChunkDL(psBdspInfo, MULTI_CHUNK_APP_REG);
}

/***************************************************************************
 *	CreateAppExec
 *
 *	Function:
 *			Create App Exec
 *	Arguments:
 *			bOnOff		On/Off
 *			bApp		App bit
 *			bAppExec	App exec value
 *	Return:
 *			App Exec
 *
 ****************************************************************************/
static UINT8 CreateAppExec(UINT8 bOnOff, UINT8 bApp, UINT8 bAppExec)
{
	switch (bOnOff) {
	case BDSP_APP_EXEC_STOP:
		bAppExec &= ~bApp;
		break;

	case BDSP_APP_EXEC_START:
		bAppExec |= bApp;
		break;

	case BDSP_APP_EXEC_DONTCARE:
	default:
		break;
	}

	return bAppExec;
}

/***************************************************************************
 *	NewAppOn
 *
 *	Function:
 *			Settings App On/Off
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void NewAppOn(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bAppExec0;
	UINT8 bAppExec1;
	UINT8 *pbPrm;

	pbPrm = psBdspInfo->pbFwctrlB;

	bAppExec0 = gsBdspInfo.bAppExec0;
	bAppExec1 = gsBdspInfo.bAppExec1;

	bAppExec0 |= psBdspInfo->bStoppedAppExec0;
	bAppExec1 |= psBdspInfo->bStoppedAppExec1;

	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_WIDE],
					MCB_AEEXEC0_WIDE, bAppExec0);
	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_HEX],
					MCB_AEEXEC0_HEX, bAppExec0);
	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_EQ3_0A],
					MCB_AEEXEC0_EQ3_0A, bAppExec0);
	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_DRC3],
					MCB_AEEXEC0_DRC3, bAppExec0);
	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_DRC_0],
					MCB_AEEXEC0_DRC_0, bAppExec0);
	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_EQ3_0B],
					MCB_AEEXEC0_EQ3_0B, bAppExec0);

	bAppExec0 = CreateAppExec(pbPrm[FWCTRL_B_GEN],
					MCB_AEEXEC0_GEN, bAppExec0);

	bAppExec1 = CreateAppExec(pbPrm[FWCTRL_B_EQ3_1A],
					MCB_AEEXEC1_EQ3_1A, bAppExec1);
	bAppExec1 = CreateAppExec(pbPrm[FWCTRL_B_AGC],
					MCB_AEEXEC1_AGC, bAppExec1);
	bAppExec1 = CreateAppExec(pbPrm[FWCTRL_B_DRC_1],
					MCB_AEEXEC1_DRC_1, bAppExec1);
	bAppExec1 = CreateAppExec(pbPrm[FWCTRL_B_EQ3_1B],
					MCB_AEEXEC1_EQ3_1B, bAppExec1);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC0),
				bAppExec0);


	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC1),
				bAppExec1);

	McDevIf_ExecutePacket();

	gsBdspInfo.bAppExec0 = bAppExec0;
	gsBdspInfo.bAppExec1 = bAppExec1;
}

/***************************************************************************
 *	SinStart
 *
 *	Function:
 *			Sin start
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SinStart(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	if (psBdspInfo->bStoppedSinOut == MCB_SINOUT) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_SINOUT),
						MCB_SINOUT);
		McDevIf_ExecutePacket();
	}
}

/***************************************************************************
 *	NewPath
 *
 *	Function:
 *			Path setting
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void NewPath(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bAEBypass;
	UINT8 *pbPrm;

	pbPrm = psBdspInfo->pbFwctrlB;
	if (pbPrm == NULL)
		return;

	bAEBypass = gsBdspInfo.bAEBypass;

	switch (pbPrm[FWCTRL_B_AE0_BYP]) {
	case BDSP_PATH_NORMAL:
		bAEBypass &= ~MCB_AEBYPASS_AE0;
		break;

	case BDSP_PATH_BYPASS:
		bAEBypass |= MCB_AEBYPASS_AE0;
		break;

	case BDSP_PATH_DONTCARE:
	default:
		break;
	}

	switch (pbPrm[FWCTRL_B_AE1_BYP]) {
	case BDSP_PATH_NORMAL:
		bAEBypass &= ~MCB_AEBYPASS_AE1;
		break;

	case BDSP_PATH_BYPASS:
		bAEBypass |= MCB_AEBYPASS_AE1;
		break;

	case BDSP_PATH_DONTCARE:
	default:
		break;
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEBYPASS),
				bAEBypass);

	McDevIf_ExecutePacket();

	gsBdspInfo.bAEBypass = bAEBypass;
}

/***************************************************************************
 *	AudioIFPath
 *
 *	Function:
 *			Audio IF path set
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AudioIFPath(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bDSPCtl;

	switch (psBdspInfo->pbFwctrlB[FWCTRL_B_BYPASS]) {
	case BDSP_PATH_NORMAL:
	case BDSP_PATH_BYPASS:
		gsBdspInfo.bDSPBypass =
			(UINT8)((psBdspInfo->pbFwctrlB[FWCTRL_B_BYPASS] << 7)
							& MCB_BDSPBYPASS);
		break;

	case BDSP_PATH_DONTCARE:
	default:
		break;
	};

	bDSPCtl = gsBdspInfo.bDSPCtl;
	switch (gsBdspInfo.bDSPBypass & MCB_BDSPBYPASS) {
	case MCB_BDSPBYPASS:
		if ((bDSPCtl & (UINT8)MCB_BDSPSTART) != 0)
			bDSPCtl = MCB_BDSPBYPASS;
		break;

	/*case MCI_BDSPCTRL_DEF:*/
	default:
		if ((bDSPCtl & (UINT8)MCB_BDSPBYPASS) != 0)
			bDSPCtl = MCB_BDSPSTART;
		break;
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_BDSPCTRL),
				bDSPCtl);

	McDevIf_ExecutePacket();

	gsBdspInfo.bDSPCtl = bDSPCtl;
}

/***************************************************************************
 *	ResumeAppOn
 *
 *	Function:
 *			Resume App On
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ResumeAppOn(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bAppExec0;
	UINT8 bAppExec1;

	bAppExec0 = gsBdspInfo.bAppExec0;
	bAppExec1 = gsBdspInfo.bAppExec1;

	bAppExec0 |= psBdspInfo->bStoppedAppExec0;
	bAppExec1 |= psBdspInfo->bStoppedAppExec1;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC0),
				bAppExec0);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_AEEXEC1),
				bAppExec1);

	McDevIf_ExecutePacket();

	gsBdspInfo.bAppExec0 = bAppExec0;
	gsBdspInfo.bAppExec1 = bAppExec1;
}

/***************************************************************************
 *	ResumePath
 *
 *	Function:
 *			Resume path
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ResumePath(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	UINT8 bData;

	bData = gsBdspInfo.bAEBypass;
	bData &= ~psBdspInfo->bStoppedBypass;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_B
					| (UINT32)MCI_AEBYPASS),
					bData);

	McDevIf_ExecutePacket();

	gsBdspInfo.bAEBypass = bData;
}

/***************************************************************************
 *	ReStart
 *
 *	Function:
 *			Restart
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ReStart(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	if (psBdspInfo->pbFwctrlB != NULL) {
		NewAppOn(psBdspInfo);

		SinStart(psBdspInfo);

		NewPath(psBdspInfo);

		AudioIFPath(psBdspInfo);
	} else {
		ResumeAppOn(psBdspInfo);

		SinStart(psBdspInfo);

		ResumePath(psBdspInfo);
	}
}

/***************************************************************************
 *	SetAudioEngine
 *
 *	Function:
 *			AudioEngine setting
 *	Arguments:
 *			psBdspInfo	MCDRV_BDSP_AEC_BDSP_INFO
 *					structure pointer
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
static SINT32 SetAudioEngine(struct MCDRV_BDSP_AEC_BDSP_INFO *psBdspInfo)
{
	SINT32 sdResult;

	if ((gsBdspInfo.bDSPCtl & (UINT8)MCB_BDSPSTART) == 0) {
		psBdspInfo->bAe0AppOnOff = 0;
		psBdspInfo->bAe1AppOnOff = 0;
		psBdspInfo->bAppGen = 0;
		psBdspInfo->dCoefTarget = TARGET_NONE;
		psBdspInfo->bCoefTrans = COEF_DMA_TRANS;
		psBdspInfo->dRegAppStopTarget = TARGET_NONE;
	}

	sdResult = Stop(psBdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	Download(psBdspInfo);

	ReStart(psBdspInfo);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	Upload
 *
 *	Function:
 *			upload
 *	Arguments:
 *			bAdr0		address0
 *			dAddress	address
 *			bBMACtl		control register value
 *			dSize		size
 *			dUnitSize	unit size
 *			pbData		Pointer to the data get area
 *	Return:
 *			none
 *
 ****************************************************************************/
static void Upload(UINT8 bAdr0, UINT32 dAddress, UINT8 bBMACtl,
			UINT32 dSize, UINT32 dUnitSize, UINT8 *pbData)
{
	UINT8 bAdr1;
	UINT8 bAdr2;
	UINT32 i;

	bAdr1 = (UINT8)((dAddress >> 8) & (UINT32)MCB_BMAA1);
	bAdr2 = (UINT8)(dAddress & (UINT32)MCB_BMAA2);


	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAA0),
						bAdr0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAA1),
						bAdr1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAA2),
						bAdr2);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMACTL),
						bBMACtl);

	McDevIf_ExecutePacket();

	for (i = 0; i < dSize; i++) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAD),
						&pbData[(i * dUnitSize) + 0],
						1);
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAD),
						&pbData[(i * dUnitSize) + 1],
						1);
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAD),
						&pbData[(i * dUnitSize) + 2],
						1);
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAD),
						&pbData[(i * dUnitSize) + 3],
						1);
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BMAA0),
						MCI_BMAA0_DEF);
	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	McBdsp_Init
 *
 *	Function:
 *			initialize
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32 McBdsp_Init(void)
{
	static UINT8 abFaderCoef[17] = {
		0x00, 0x00, 0x00, 0x00,		/* address */
		0x00,
		0x00, 0x00, 0x00, 0x08,		/* size 8 (4 * 2)*/
		0xF8, 0x44, 0x41, 0x78,		/* CFADE_0 10ms */
		0xF8, 0x44, 0x41, 0x78		/* CFADE_1 10ms */
	};

	if (gsBdspInfo.dStatus != (UINT32)BDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	gsBdspInfo.bDSPBypass = MCB_BDSPBYPASS;
	gsBdspInfo.bSinCtrlSel = BDSP_SIN_CTRL_REG;

	gsBdspInfo.bDSPCtl = MCI_BDSPCTRL_DEF;
	gsBdspInfo.bAppExec0 = MCI_AEEXEC0_DEF;
	gsBdspInfo.bAppExec1 = MCI_AEEXEC1_DEF;
	gsBdspInfo.bAEBypass = MCI_AEBYPASS_DEF;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_BDSPCTRL),
						gsBdspInfo.bDSPCtl);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_AEEXEC0),
						gsBdspInfo.bAppExec0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_AEEXEC1),
						gsBdspInfo.bAppExec1);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_AEBYPASS),
						gsBdspInfo.bAEBypass);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_AEFADE),
						MCI_AEFADE_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_F01SEL),
						MCI_F01SEL_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_B
						| (UINT32)MCI_SINOUT),
						MCI_SINOUT_DEF);

	McDevIf_ExecutePacket();

	gsBdspInfo.dStatus = (UINT32)BDSP_STATUS_INITED;

	AppCoefDL(abFaderCoef, COEF_DMA_TRANS);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McBdsp_Term
 *
 *	Function:
 *			terminate
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McBdsp_Term(void)
{
	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return MCDRV_SUCCESS;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_B
					| (UINT32)MCI_BDSPCTRL),
					MCI_BDSPCTRL_DEF);

	McDevIf_ExecutePacket();

	gsBdspInfo.dStatus = (UINT32)BDSP_STATUS_IDLE;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McBdsp_GetTransition
 *
 *	Function:
 *			Get control state
 *	Arguments:
 *			none
 *	Return:
 *			state
 *
 ****************************************************************************/
SINT32 McBdsp_GetTransition(void)
{
	SINT32 sdResult;
	UINT8 bData;

	sdResult = 0;

	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return sdResult;

	/* AE1FADE,AE0FADE */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_B | (UINT32)MCI_AEFADE),
								&bData, 1);

	if ((bData & MCB_AEFADE_AE0) != 0)
		sdResult |= (SINT32)BDSP_PRC_FADE_AE0;
	if ((bData & MCB_AEFADE_AE1) != 0)
		sdResult |= (SINT32)BDSP_PRC_FADE_AE1;

	/* SINOFLG */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_B | (UINT32)MCI_SINOUTFLG),
								&bData, 1);
	if ((bData & MCB_SINOFLG) != 0)
		sdResult |= (SINT32)BDSP_PRC_SINOUT;

	/* DSP Coef Trance */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_BDSPTREQ),
						&bData, 1);
	if ((bData & MCB_BDSPTREQ) != 0)
		sdResult += (SINT32)BDSP_PRC_COEF_TRS;

	return sdResult;
}

/***************************************************************************
 *	McBdsp_GetDSP
 *
 *	Function:
 *			Get dsp data
 *	Arguments:
 *			dTarget		target
 *			dAddress	address
 *			pbData		Pointer to the data get area
 *			dSize		data size
 *	Return:
 *			
 *
 ****************************************************************************/
SINT32 McBdsp_GetDSP(UINT32 dTarget, UINT32 dAddress,
						UINT8 *pbData, UINT32 dSize)
{
	UINT8 bBMACtl;
	UINT32 dUnitSize;

	if (pbData == NULL)
		return MCDRV_ERROR_ARGUMENT;

	bBMACtl = MCB_BMADIR_UL;
	dUnitSize = 0;

	switch (dTarget) {
	case BDSP_AE_FW_DXRAM:
		if (DXRAM_RANGE_MAX < dAddress)
			return MCDRV_ERROR_ARGUMENT;

		dUnitSize = RAM_UNIT_SIZE_64;
		dSize = (dSize / dUnitSize);

		if ((DXRAM_RANGE_MAX + 1) < dSize)
			dSize = DXRAM_RANGE_MAX + 1;

		if ((DXRAM_RANGE_MAX + 1) < (dAddress + dSize))
			dSize -= ((dAddress + dSize)
						- (DXRAM_RANGE_MAX + 1));
		bBMACtl |= MCB_BMABUS_X;
		break;

	case BDSP_AE_FW_DYRAM:
		if (DYRAM_RANGE_MAX < dAddress)
			return MCDRV_ERROR_ARGUMENT;

		dUnitSize = RAM_UNIT_SIZE_32;
		dSize = (dSize / dUnitSize);

		if ((DYRAM_RANGE_MAX + 1) < dSize)
			dSize = (DYRAM_RANGE_MAX + 1);

		if ((DYRAM_RANGE_MAX + 1) < (dAddress + dSize))
			dSize -= ((dAddress + dSize)
						- (DYRAM_RANGE_MAX + 1));

		bBMACtl |= MCB_BMABUS_Y;
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	if (dSize == 0)
		return 0;

	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return 0;

	if (dTarget == BDSP_AE_FW_DXRAM) {
		Upload(MCB_BMAA0, dAddress, bBMACtl, dSize, dUnitSize, pbData);

		Upload(MCI_BMAA0_DEF, dAddress, bBMACtl, dSize, dUnitSize,
								&pbData[4]);

	} else
		Upload(MCI_BMAA0_DEF, dAddress, bBMACtl, dSize, dUnitSize,
								pbData);

	return (SINT32)(dSize * dUnitSize);
}

/***************************************************************************
 *	McBdsp_SetDSPCheck
 *
 *	Function:
 *			DSP configuration check
 *	Arguments:
 *			psPrm		MCDRV_AEC_INFO structure pointer
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
SINT32 McBdsp_SetDSPCheck(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_BDSP_AEC_BDSP_INFO sBdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetBDSPChunk(psPrm, &sBdspInfo);

	sdResult = BdspChunkAnalyze(&sBdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McBdsp_SetDSP
 *
 *	Function:
 *			DSP settings
 *	Arguments:
 *			psPrm		MCDRV_AEC_INFO structure pointer
 *	Return:
 *			0		SUCCESS
 *			< 0		ERROR
 *
 ****************************************************************************/
SINT32 McBdsp_SetDSP(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_BDSP_AEC_BDSP_INFO sBdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetBDSPChunk(psPrm, &sBdspInfo);

	sdResult = BdspChunkAnalyze(&sBdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	if ((sBdspInfo.pbChunkData == NULL) || (sBdspInfo.dwSize == 0))
		return MCDRV_SUCCESS;

	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	sdResult = SetAudioEngine(&sBdspInfo);

	return sdResult;
}

/***************************************************************************
 *	McBdsp_Start
 *
 *	Function:
 *			Dsp start
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32 McBdsp_Start(void)
{
	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	if ((gsBdspInfo.bDSPBypass & MCB_BDSPBYPASS) != 0)
		gsBdspInfo.bDSPCtl = (UINT8)MCB_BDSPBYPASS;
	else
		gsBdspInfo.bDSPCtl = (UINT8)MCB_BDSPSTART;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_B
				| (UINT32)MCI_BDSPCTRL),
				gsBdspInfo.bDSPCtl);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McBdsp_Stop
 *
 *	Function:
 *			Dsp stop
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McBdsp_Stop(void)
{
	if (gsBdspInfo.dStatus == (UINT32)BDSP_STATUS_IDLE)
		return MCDRV_SUCCESS;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_BDSPTREQ),
					MCI_BDSPTREQ_DEF);
	
	gsBdspInfo.bDSPCtl = (UINT8)MCI_BDSPCTRL_DEF;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_B
					| (UINT32)MCI_BDSPCTRL),
					gsBdspInfo.bDSPCtl);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

