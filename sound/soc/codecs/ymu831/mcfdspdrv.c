/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcfdspdrv.c
 *
 *		Description	: MC Fdsp Driver
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
#include "mcfdspdrv.h"


/* inside definition */

#define IRQ_NO_ERROR			(0)
#define IRQ_ERROR			(1)

#define FDSP_STATUS_IDLE		(0)
#define FDSP_STATUS_INITED		(1)

#define CHUNK_SIZE			(8)

#define AEC_FULL_FW			(1)
#define AEC_APP_FW			(2)
#define AEC_APP_COEF_FW			(3)
#define AEC_APP_REG_FW			(4)

#define COEF_DMA_TRANS			(0)
#define COEF_DSP_TRANS			(1)
#define COEF_DSP_TRANS_MAX		(512)

#define AEC_FDSP_TAG_FWCTRL		(0x00000100)
#define FWCTRL_SIZE			(27)
#define AEC_FDSP_TAG_CHSEL		(0x00000200)
#define CHSEL_SIZE			(33)
#define AEC_FDSP_TAG_TOP_ENV		(0x00000300)
#define TOP_ENV_SIZE			(29)
#define AEC_FDSP_TAG_TOP_COEF		(0x00000400)
#define TOP_COEF_FIX_SIZE		(8)
#define AEC_FDSP_TAG_TOP_INS		(0x00000500)
#define TOP_INS_FIX_SIZE		(8)
#define AEC_FDSP_TAG_APP_MASK		(0xFFFFFF00)
#define AEC_FDSP_TAG_APPNO_MASK		(0xFF)
#define AEC_FDSP_TAG_APP_ENV		(0x00000600)
#define APP_ENV_FIX_SIZE		(38)
#define AEC_FDSP_TAG_APP_COEF		(0x00000700)
#define APP_COEF_FIX_SIZE		(9)
#define AEC_FDSP_TAG_APP_CONST		(0x00000800)
#define AEC_FDSP_TAG_APP_INS		(0x00000900)
#define APP_INS_FIX_SIZE		(8)
#define AEC_FDSP_TAG_APP_REG		(0x00000A00)
#define APP_REG_FIX_SIZE		(8)
#define AEC_FDSP_TAG_EX_INFO_1		(0x00000B00)
#define EX_INFO_1_SIZE			(4)

#define FWCTL_FWMOD			(0)
#define FWCTL_FS			(1)
#define FWCTL_APPEXEC			(2)
#define FWCTL_FADECODE			(26)

#define CHSEL_BYPASS			(0)
#define CHSEL_INFDSPSRC			(1)
#define CHSEL_OUTAUDIOSRC		(17)

#define TOP_ENV_ID			(0)
#define TOP_ENV_INSTBASE		(1)
#define TOP_ENV_MBLKSIZE		(5)
#define TOP_ENV_MBUFSIZE		(6)
#define TOP_ENV_MOCHCNFG		(7)
#define TOP_ENV_MICHCNFG		(8)
#define TOP_ENV_SSTARTCH		(9)
#define TOP_ENV_SNUMOFOCH		(10)
#define TOP_ENV_SNUMOFICH		(11)
#define TOP_ENV_SAVEBUFSIZE1		(12)
#define TOP_ENV_SAVEBUFSIZE0		(16)
#define TOP_ENV_LIMITWORKSIZE		(20)
#define TOP_ENV_WORKBASE		(24)
#define TOP_ENV_TOPBASE0		(28)

#define TOP_COEF_ADR			(0)
#define TOP_COEF_SIZE			(4)
#define TOP_COEF_DATA			(8)

#define TOP_INST_ADR			(0)
#define TOP_INST_SIZE			(4)
#define TOP_INST_DATA			(8)

#define APP_ENV_ID			(0)
#define APP_ENV_EXCECPROCESS		(1)
#define APP_ENV_INSTBASE		(2)
#define APP_ENV_REGBASE			(6)
#define APP_ENV_EXECFS			(10)
#define APP_ENV_EXECCH			(14)
#define APP_ENV_WORKBASE		(18)
#define APP_ENV_APPBASE0		(22)
#define APP_ENV_APPBASE1		(26)
#define APP_ENV_APPBASE2		(30)
#define APP_ENV_APPBASE3		(34)
#define APP_ENV_APPFADE			(38)
#define APP_ENV_APPIRQ			(39)

#define APP_COEF_ADR			(0)
#define APP_COEF_SIZE			(5)
#define APP_COEF_DATA			(9)

#define APP_INST_ADR			(0)
#define APP_INST_SIZE			(4)
#define APP_INST_DATA			(8)

#define APP_REG_ADR			(0)
#define APP_REG_SIZE			(4)
#define APP_REG_DATA			(8)

#define APP_PARAM_ON			(1)

#define FDSP_FREQ_MIN			(1)
#define FDSP_FREQ_MAX			(5)

#define MULTI_CHUNK_TOP_COEF		(1)
#define MULTI_CHUNK_APP_COEF		(2)
#define MULTI_CHUNK_APP_REG		(3)

#define RAM_UNIT_SIZE			(4UL)
#define DXRAM_RANGE_MIN			(0x0UL)
#define DXRAM_RANGE_MAX_1		(0x07FFFUL)
#define DXRAM_RANGE_MAX_2		(0x08FFFUL)
#define DYRAM_RANGE_MIN			(0x0UL)
#define DYRAM_RANGE_MAX			(0x017FFUL)
#define IRAM_RANGE_MIN			(0x10000UL)
#define IRAM_RANGE_MAX_1		(0x13FFFUL)
#define IRAM_RANGE_MAX_2		(0x12FFFUL)

#define STOP_KIND_NONE			(0x00)
#define STOP_KIND_FDSP			(0x01)
#define STOP_KIND_APP_EXEC		(0x02)
#define STOP_KIND_APP			(0x04)
#define STOP_KIND_WAIT			(0x08)

#define RESTART_OFF			(0)
#define RESTART_ON			(1)
#define RESTART_BYPASS			(2)

#define APP_VOL_ID			(0x07)
#define FIX_APP_VOL_FREE		(0)
#define FIX_APP_VOL_EXIST		(1)
#define FIX_APP_VOL_NO			(22)
#define FIX_APP_VOL_ACT			MCI_APPACT0
#define FIX_APP_VOL_BIT			MCB_APPACT22
#define FIX_APP_VOL_APPEXEC		(0x400000)
#define FIX_APP_VOL_REG_BASE		(0x4F)
#define FIX_APP_VOL_REG_FADE_CTRL	(FIX_APP_VOL_REG_BASE + 1)
#define FIX_APP_VOL_REG_FADE_CTRL_BIT	(0x01)
#define FIX_APP_VOL_REG_FADE_STE	(FIX_APP_VOL_REG_BASE + 2)
#define FIX_APP_VOL_REG_FADE_STE_BIT	(0x01)
#define FIX_APP_FADE_WAIT_TIME_US	(10000)

#define DEF_FDSP_STOP_WAIT_TIME_US	(15000)

struct MCDRV_FDSP_INFO {
	/* state */
	UINT32				dStatus;
	/* info */
	struct MCDRV_FDSP_INIT		sInit;
	SINT32(*cbfunc)(SINT32, UINT32, UINT32);
	UINT32				dAppStop;
	UINT32				dAppIEnb;
	UINT32				dAppFade;
	UINT8				bDSPBypass;
	UINT8				bFixAppVol;
	/* register */
	UINT8				bADIMute0;
	UINT8				bADIMute1;
	UINT8				bADOMute0;
	UINT8				bADOMute1;
	UINT8				bDSPCtl;
	UINT8				bAppExec0;
	UINT8				bAppExec1;
	UINT8				bAppExec2;
	UINT8				bAppIEnb0;
	UINT8				bAppIEnb1;
	UINT8				bAppIEnb2;
};

struct MCDRV_FDSP_AEC_FDSP_INFO {
	UINT8				*pbChunkData;
	UINT32				dwSize;
	UINT8				bMustStop;
	UINT8				*pbFwctrl;
	UINT8				*pbChSel;
	UINT8				*pbTopEnv;
	UINT32				dTopCoefCnt;
	UINT8				*pbTopInst;
	UINT32				dTargetApp;
	UINT32				dAppEnvCnt;
	UINT8				*apbAppEnv[FDSP_APP_NUM];
	UINT8				bCoefTrans;
	UINT32				dAppCoefCnt;
	UINT32				dAppCnstCnt;
	UINT32				dAppMaxCoefCnstNum;
	UINT32				dAppInstCnt;
	UINT8				*apbAppInst[FDSP_APP_NUM];
	UINT32				dAppRegCnt;
	UINT8				*pbExInfo1;
	UINT32				dAppExec;
};

struct MCDRV_FDSP_EXEC_INFO {
	UINT32				dAppExec;
	UINT8				bRestart;
};

static struct MCDRV_FDSP_INFO gsFdspInfo = {
	FDSP_STATUS_IDLE,
	{
		{0},
		{0},
		DEF_FDSP_STOP_WAIT_TIME_US
	},
	NULL,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

/****************************************************************************
 *	GetAppAct
 *
 *	Description:
 *			APP operation state acquisition
 *	Arguments:
 *			none
 *	Return:
 *			APP State
 *
 ****************************************************************************/
static UINT32 GetAppAct(void)
{
	UINT32 dAppAct;
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT0), &bData, 1);
	dAppAct = ((UINT32)bData << 16);

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT1), &bData, 1);
	dAppAct |= ((UINT32)bData << 8);

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT2), &bData, 1);
	dAppAct |= (UINT32)bData;

	return dAppAct;
}

/****************************************************************************
 *	GetChMute
 *
 *	Description:
 *			Specified channel Mute information acquisition
 *	Arguments:
 *			bADMute		mute information
 *			bChMuteBit	Specified channel
 *	Return:
 *			Mute ON/OFF information
 *
 ****************************************************************************/
static UINT8 GetChMute(UINT8 bADMute, UINT8 bChMuteBit)
{
	if ((bADMute & bChMuteBit) == bChMuteBit)
		return FDSP_MUTE_OFF;

	return FDSP_MUTE_ON;
}

/****************************************************************************
 *	CreateMuteReg
 *
 *	Description:
 *			Mute register value making
 *	Arguments:
 *			bADMute		mute register value
 *			bMute		mute specification
 *			bChMuteBit	specified channel
 *	Return:
 *			mute register value
 *
 ****************************************************************************/
static UINT8 CreateMuteReg(UINT8 bADMute, UINT8 bMute, UINT8 bChMuteBit)
{
	switch (bMute) {
	case FDSP_MUTE_ON:
		bADMute &= (UINT8)~bChMuteBit;
		break;

	case FDSP_MUTE_OFF:
		bADMute |= bChMuteBit;
		break;

	default:
		break;
	}

	return bADMute;
}

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

/****************************************************************************
 *	SetAudioIF
 *
 *	Description:
 *			Setting of Audio I/F
 *	Arguments:
 *			psFdspInit	MCDRV_FDSP_AUDIO_INIT structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetAudioIF(struct MCDRV_FDSP_INIT *psFdspInit)
{
	/* ADIDFmt0 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_F
				| (UINT32)MCI_ADIDFMT0),
				(UINT8)(psFdspInit->sInput.wADDFmt & 0xFF));
	/* ADIDFmt1 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_F
			| (UINT32)MCI_ADIDFMT1),
			(UINT8)((psFdspInit->sInput.wADDFmt >> 8) & 0xFF));


	/* ADODFmt0 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_F
				| (UINT32)MCI_ADODFMT0),
				(UINT8)(psFdspInit->sOutput.wADDFmt & 0xFF));
	/* ADODFmt1 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_F
			| (UINT32)MCI_ADODFMT1),
			(UINT8)((psFdspInit->sOutput.wADDFmt >> 8) & 0xFF));

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	InitCore
 *
 *	Description:
 *			Init F-DSP
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void InitCore(void)
{
	gsFdspInfo.dAppStop = 0UL;
	gsFdspInfo.dAppIEnb = 0UL;
	gsFdspInfo.dAppFade = 0UL;
	gsFdspInfo.bDSPBypass = 0UL;
	gsFdspInfo.bFixAppVol = FIX_APP_VOL_FREE;

	gsFdspInfo.bDSPCtl = MCI_DSPCTRL_DEF;
	gsFdspInfo.bAppExec0 = MCI_APPEXEC0_DEF;
	gsFdspInfo.bAppExec1 = MCI_APPEXEC1_DEF;
	gsFdspInfo.bAppExec2 = MCI_APPEXEC2_DEF;
	gsFdspInfo.bAppIEnb0 = MCI_APPIENB0_DEF;
	gsFdspInfo.bAppIEnb1 = MCI_APPIENB1_DEF;
	gsFdspInfo.bAppIEnb2 = MCI_APPIENB2_DEF;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_DSPCTRL),
						gsFdspInfo.bDSPCtl);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC0),
						gsFdspInfo.bAppExec0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC1),
						gsFdspInfo.bAppExec1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC2),
						gsFdspInfo.bAppExec2);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB0),
						gsFdspInfo.bAppIEnb0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB1),
						gsFdspInfo.bAppIEnb1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB2),
						gsFdspInfo.bAppIEnb2);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE0),
						gsFdspInfo.bADIMute0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE1),
						gsFdspInfo.bADIMute1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE2),
						MCB_ADIMTSET);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE0),
						gsFdspInfo.bADOMute0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE1),
						gsFdspInfo.bADOMute1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE2),
						MCB_ADOMTSET);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE0),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE1),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE2),
						0x00);
	McDevIf_ExecutePacket();

	SetAudioIF(&gsFdspInfo.sInit);
}

/****************************************************************************
 *	GetFDSPChunk
 *
 *	Description:
 *			Get FDSP chunk
 *	Arguments:
 *			psPrm		MCDRV_AEC_INFO structure pointer
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 GetFDSPChunk(struct MCDRV_AEC_INFO *psPrm,
				struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	psFdspInfo->pbChunkData = NULL;
	psFdspInfo->dwSize = 0;

	switch (psPrm->sAecConfig.bFDspLocate) {
	case FDSP_LOCATE_AUDIOENGINE:
		if (psPrm->sAecAudioengine.bEnable == AEC_AUDIOENGINE_ENABLE) {

			if (psPrm->sAecAudioengine.bFDspOnOff == FDSP_OFF)
				return MCDRV_SUCCESS;

			psFdspInfo->pbChunkData =
				psPrm->sAecAudioengine.sAecFDsp.pbChunkData;
			psFdspInfo->dwSize =
					psPrm->sAecAudioengine.sAecFDsp.dwSize;
		}
		break;

	case FDSP_LOCATE_V_BOX:
		if (psPrm->sAecVBox.bEnable == AEC_AUDIOENGINE_ENABLE) {

			if (psPrm->sAecVBox.bFDspOnOff == FDSP_OFF)
				return MCDRV_SUCCESS;

			psFdspInfo->pbChunkData =
					psPrm->sAecVBox.sAecFDsp.pbChunkData;
			psFdspInfo->dwSize =
					psPrm->sAecVBox.sAecFDsp.dwSize;
		}
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	if (psFdspInfo->pbChunkData != NULL)
		psFdspInfo->pbChunkData = &psFdspInfo->pbChunkData[CHUNK_SIZE];

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	FwctrlChunkAnalyze
 *
 *	Description:
 *			FWCTRL chunk analysis
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *			pbPrm		FWCTRL chunk pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 FwctrlChunkAnalyze(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo,
								UINT8 *pbPrm)
{
	UINT32 i;

	if (pbPrm[FWCTL_FWMOD] != 0x00)
		return MCDRV_ERROR_ARGUMENT;

	if ((pbPrm[FWCTL_FS] != 0xFF) || (pbPrm[FWCTL_FADECODE] != 0xFF))
		psFdspInfo->bMustStop |= STOP_KIND_FDSP;

	for (i = 0; i < (UINT32)FDSP_APP_NUM; ++i)
		if ((pbPrm[FWCTL_APPEXEC + i] != FDSP_APP_EXEC_DONTCARE) &&
			(i != FIX_APP_VOL_NO)) {
			psFdspInfo->bMustStop |= STOP_KIND_APP_EXEC;
			break;
		}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	FixAppCheck
 *
 *	Description:
 *			Check Fix APP
 *	Arguments:
 *			dAppNo		App No
 *			pbAppEnv	App Env pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 FixAppCheck(UINT32 dAppNo, UINT8 *pbAppEnv)
{
	if (dAppNo != FIX_APP_VOL_NO)
		return MCDRV_SUCCESS;

	if (pbAppEnv[APP_ENV_ID] == 0x00)
		return MCDRV_SUCCESS;

	if (pbAppEnv[APP_ENV_ID] != APP_VOL_ID)
		return MCDRV_ERROR_ARGUMENT;

	if ((UINT8)(pbAppEnv[APP_ENV_REGBASE + 3] & 0x7F) !=
							FIX_APP_VOL_REG_BASE)
		return MCDRV_ERROR_ARGUMENT;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	AppChunkAnalyze
 *
 *	Description:
 *			APP chunk analysis
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *			dChunkId	chunk id
 *			dChunkSize	chunk size
 *			dChunkTop	chunk top position
 *			pbPrm		chunk pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 AppChunkAnalyze(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo,
					UINT32 dChunkId, UINT32 dChunkSize,
					UINT32 dChunkTop, UINT8 *pbPrm)
{
	UINT32 dAppNo;
	UINT32 dTemp;

	switch ((dChunkId & (UINT32)AEC_FDSP_TAG_APP_MASK)) {
	case AEC_FDSP_TAG_APP_ENV:
		dAppNo = (dChunkId & (UINT32)AEC_FDSP_TAG_APPNO_MASK);
		if (dAppNo < (UINT32)FDSP_APP_NUM) {
			if (dChunkSize < (UINT32)APP_ENV_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->apbAppEnv[dAppNo] != NULL)
				return MCDRV_ERROR_ARGUMENT;

			if (FixAppCheck(dAppNo, &pbPrm[dChunkTop])
							!= MCDRV_SUCCESS)
				return MCDRV_ERROR_ARGUMENT;

			psFdspInfo->apbAppEnv[dAppNo] = &pbPrm[dChunkTop];
			++(psFdspInfo->dAppEnvCnt);
			psFdspInfo->dTargetApp |= (0x01UL << dAppNo);
			psFdspInfo->bCoefTrans = COEF_DMA_TRANS;

			psFdspInfo->bMustStop |= STOP_KIND_APP;
		}
		break;

	case AEC_FDSP_TAG_APP_COEF:
	case AEC_FDSP_TAG_APP_CONST:
		dAppNo = (dChunkId & (UINT32)AEC_FDSP_TAG_APPNO_MASK);
		if (dAppNo < (UINT32)FDSP_APP_NUM) {
			if (dChunkSize < (UINT32)APP_COEF_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			dTemp = CreateUINT32(pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL],
						pbPrm[dChunkTop + 8UL]);
			if (dTemp == 0UL)
				break;
			if ((dTemp & 0x03UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)APP_COEF_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;

			if (COEF_DSP_TRANS != pbPrm[dChunkTop + 4UL]) {
				psFdspInfo->bCoefTrans = COEF_DMA_TRANS;
				psFdspInfo->bMustStop |= STOP_KIND_APP;
			}

			if ((UINT32)AEC_FDSP_TAG_APP_COEF ==
				(dChunkId & (UINT32)AEC_FDSP_TAG_APP_MASK))
				++(psFdspInfo->dAppCoefCnt);
			else
				++(psFdspInfo->dAppCnstCnt);

			psFdspInfo->dTargetApp |= (0x01UL << dAppNo);
			if (psFdspInfo->dAppMaxCoefCnstNum < dTemp)
				psFdspInfo->dAppMaxCoefCnstNum = dTemp;

			psFdspInfo->bMustStop |= STOP_KIND_WAIT;
		}
		break;

	case AEC_FDSP_TAG_APP_INS:
		dAppNo = (dChunkId & (UINT32)AEC_FDSP_TAG_APPNO_MASK);
		if (dAppNo < (UINT32)FDSP_APP_NUM) {
			if (dChunkSize < (UINT32)APP_INS_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->apbAppInst[dAppNo] != NULL)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);
			if (dTemp == 0UL)
				break;
			if ((dTemp & 0x03UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)APP_INS_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;

			psFdspInfo->apbAppInst[dAppNo] = &pbPrm[dChunkTop];
			++(psFdspInfo->dAppInstCnt);
			psFdspInfo->dTargetApp |= (0x01UL << dAppNo);
			psFdspInfo->bCoefTrans = COEF_DMA_TRANS;

			psFdspInfo->bMustStop |= STOP_KIND_APP;
		}
		break;

	case AEC_FDSP_TAG_APP_REG:
		dAppNo = (dChunkId & (UINT32)AEC_FDSP_TAG_APPNO_MASK);
		if (dAppNo < (UINT32)FDSP_APP_NUM) {
			if (dChunkSize < (UINT32)APP_REG_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			dTemp = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);
			if (dTemp == 0UL)
				break;
			if (dChunkSize < (dTemp + (UINT32)APP_REG_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			++(psFdspInfo->dAppRegCnt);

			psFdspInfo->bMustStop |= STOP_KIND_WAIT;
		}
		break;

	default:
		/* unknown */
		break;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CoefTransCheck
 *
 *	Description:
 *			Coefficient transfer check
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void CoefTransCheck(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	if (psFdspInfo->bCoefTrans != COEF_DSP_TRANS)
		return;

	if ((psFdspInfo->dAppCoefCnt == 0UL) &&
		(psFdspInfo->dAppCnstCnt == 0UL))
		return;

	if (((UINT32)COEF_DSP_TRANS_MAX < psFdspInfo->dAppMaxCoefCnstNum) ||
		(1UL < (psFdspInfo->dAppCoefCnt + psFdspInfo->dAppCnstCnt)))
		psFdspInfo->bCoefTrans = COEF_DMA_TRANS;

	if (psFdspInfo->bCoefTrans == COEF_DMA_TRANS)
		psFdspInfo->bMustStop |= STOP_KIND_APP;
}

/****************************************************************************
 *	FdspChunkAnalyze
 *
 *	Description:
 *			FDSP chunk analysis
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 FdspChunkAnalyze(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT8 *pbPrm;
	UINT32 dSize;
	UINT32 dTemp;
	UINT32 i;

	psFdspInfo->bMustStop = STOP_KIND_NONE;
	psFdspInfo->pbFwctrl = NULL;
	psFdspInfo->pbChSel = NULL;
	psFdspInfo->pbTopEnv = NULL;
	psFdspInfo->dTopCoefCnt = 0UL;
	psFdspInfo->pbTopInst = NULL;
	psFdspInfo->dTargetApp = 0UL;
	psFdspInfo->dAppEnvCnt = 0UL;
	psFdspInfo->bCoefTrans = COEF_DSP_TRANS;
	psFdspInfo->dAppCoefCnt = 0UL;
	psFdspInfo->dAppCnstCnt = 0UL;
	psFdspInfo->dAppMaxCoefCnstNum = 0UL;
	psFdspInfo->dAppInstCnt = 0UL;
	psFdspInfo->dAppRegCnt = 0UL;
	psFdspInfo->pbExInfo1 = NULL;
	for (i = 0UL; i < (UINT32)FDSP_APP_NUM; ++i) {
		psFdspInfo->apbAppEnv[i] = NULL;
		psFdspInfo->apbAppInst[i] = NULL;
	}

	if ((psFdspInfo->pbChunkData == NULL) || (psFdspInfo->dwSize == 0))
		return MCDRV_SUCCESS;

	pbPrm = psFdspInfo->pbChunkData;
	dSize = psFdspInfo->dwSize;
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
		case AEC_FDSP_TAG_FWCTRL:
			if (dChunkSize < (UINT32)FWCTRL_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->pbFwctrl != NULL)
				return MCDRV_ERROR_ARGUMENT;

			if (FwctrlChunkAnalyze(psFdspInfo,
				&pbPrm[dChunkTop]) != MCDRV_SUCCESS)
				return MCDRV_ERROR_ARGUMENT;

			psFdspInfo->pbFwctrl = &pbPrm[dChunkTop];
			break;

		case AEC_FDSP_TAG_CHSEL:
			if (dChunkSize < (UINT32)CHSEL_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->pbChSel != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFdspInfo->pbChSel = &pbPrm[dChunkTop];
			psFdspInfo->bMustStop |= STOP_KIND_FDSP;
			break;

		case AEC_FDSP_TAG_TOP_ENV:
			if (dChunkSize < (UINT32)TOP_ENV_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->pbTopEnv != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFdspInfo->pbTopEnv = &pbPrm[dChunkTop];
			psFdspInfo->bMustStop |= STOP_KIND_FDSP;
			break;

		case AEC_FDSP_TAG_TOP_COEF:
			if (dChunkSize < (UINT32)TOP_COEF_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			dTemp = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);
			if ((dTemp & 0x03UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)TOP_COEF_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;

			++(psFdspInfo->dTopCoefCnt);
			psFdspInfo->bMustStop |= STOP_KIND_FDSP;
			break;

		case AEC_FDSP_TAG_TOP_INS:
			if (dChunkSize < (UINT32)TOP_INS_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			if (psFdspInfo->pbTopInst != NULL)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 4UL],
						pbPrm[dChunkTop + 5UL],
						pbPrm[dChunkTop + 6UL],
						pbPrm[dChunkTop + 7UL]);
			if (dTemp == 0UL)
				break;
			if ((dTemp & 0x03UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)TOP_INS_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			psFdspInfo->pbTopInst = &pbPrm[dChunkTop];
			psFdspInfo->bMustStop |= STOP_KIND_FDSP;
			break;

		default:
			if (AppChunkAnalyze(psFdspInfo,
					dChunkId, dChunkSize, dChunkTop, pbPrm)
							!= MCDRV_SUCCESS)
				return MCDRV_ERROR_ARGUMENT;
		}
		dChunkTop += dChunkSize;
	}

	CoefTransCheck(psFdspInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetAppIrq
 *
 *	Description:
 *			App interrupt setting
 *	Arguments:
 *			dAppIEnb	interrupt setting
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetAppIrq(UINT32 dAppIEnb)
{
	UINT8 bAppIEnb0;
	UINT8 bAppIEnb1;
	UINT8 bAppIEnb2;
	UINT8 bData;

	bAppIEnb0 = (UINT8)((dAppIEnb >> 16) & 0xFFUL);
	bAppIEnb1 = (UINT8)((dAppIEnb >> 8) & 0xFFUL);
	bAppIEnb2 = (UINT8)(dAppIEnb & 0xFFUL);

	if (gsFdspInfo.bAppIEnb0 != bAppIEnb0) {
		bData = (UINT8)(bAppIEnb0 & (UINT8)~gsFdspInfo.bAppIEnb0);
		if (bData != 0x00)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP0),
						bData);

		gsFdspInfo.bAppIEnb0 = bAppIEnb0;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB0),
						gsFdspInfo.bAppIEnb0);

		McDevIf_ExecutePacket();
	}

	if (gsFdspInfo.bAppIEnb1 != bAppIEnb1) {
		bData = (UINT8)(bAppIEnb1 & (UINT8)~gsFdspInfo.bAppIEnb1);
		if (bData != 0x00)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP1),
						bData);

		gsFdspInfo.bAppIEnb1 = bAppIEnb1;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB1),
						gsFdspInfo.bAppIEnb1);

		McDevIf_ExecutePacket();
	}

	if (gsFdspInfo.bAppIEnb2 != bAppIEnb2) {
		bData = (UINT8)(bAppIEnb2 & (UINT8)~gsFdspInfo.bAppIEnb2);
		if (bData != 0x00)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP2),
						bData);

		gsFdspInfo.bAppIEnb2 = bAppIEnb2;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPIENB2),
						gsFdspInfo.bAppIEnb2);

		McDevIf_ExecutePacket();
	}
}

/****************************************************************************
 *	DisableIrq
 *
 *	Description:
 *			Interrupt disable setting
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void DisableIrq(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) != 0) {
		/* IEnb */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR),
					MCI_IESERR_DEF);

		/* TopIEnb */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_TOPIENB),
					MCI_TOPIENB_DEF);

		McDevIf_ExecutePacket();

		SetAppIrq(0x00);

	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP) != 0)
		SetAppIrq((gsFdspInfo.dAppIEnb & ~psFdspInfo->dTargetApp));
}

/****************************************************************************
 *	FixAppVolFadeOut
 *
 *	Description:
 *			Fixed VOL fadeout
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 FixAppVolFadeOut(void)
{
	UINT8 bAct;

	if (gsFdspInfo.bFixAppVol != FIX_APP_VOL_EXIST)
		return MCDRV_SUCCESS;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
				| (UINT32)FIX_APP_VOL_ACT), &bAct, 1);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_IREQAPP0),
					MCB_IRAPP22);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)FIX_APP_VOL_REG_FADE_CTRL),
					0x00);

	McDevIf_ExecutePacket();

	if ((bAct & FIX_APP_VOL_BIT) == 0)
		return MCDRV_SUCCESS;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_TIMWAIT
				| (UINT32)FIX_APP_FADE_WAIT_TIME_US),
				0x00);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	DSPTransWait
 *
 *	Description:
 *			DSP forwarding waiting
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 DSPTransWait(void)
{
	SINT32 sdResult;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_IF_REG_FLAG_RESET
					| (((UINT32)MCI_FDSPTREQ) << 8)
					| (UINT32)MCB_FDSPTREQ),
					0);

	sdResult = McDevIf_ExecutePacket();

	return sdResult;
}

/****************************************************************************
 *	SetFdspCtrl
 *
 *	Description:
 *			Execution and stop settings of F-DSP
 *	Arguments:
 *			bDSPCtl		DSPCtl register value
 *	Return:
 *			MCDRV_SUCCESS
 *			FDSP_DRV_SUCCESS_STOPED
 *
 ****************************************************************************/
static SINT32 SetFdspCtrl(UINT8 bDSPCtl)
{
	UINT8 bData;
	SINT32 sdResult;

	sdResult = MCDRV_SUCCESS;

	if ((bDSPCtl & MCB_DSPSTART) != MCB_DSPSTART) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_DSPSTATE),
						&bData, 1);
		if ((bData & MCB_DSPACT) != MCB_DSPACT)
			sdResult = FDSP_DRV_SUCCESS_STOPED;

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FDSPTREQ),
					MCI_FDSPTREQ_DEF);
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_DSPCTRL),
						bDSPCtl);

	McDevIf_ExecutePacket();

	gsFdspInfo.bDSPCtl = bDSPCtl;

	return sdResult;
}

/****************************************************************************
 *	FdspStop
 *
 *	Description:
 *			Stop F-DSP
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			FDSP_DRV_SUCCESS_STOPED
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 FdspStop(void)
{
	SINT32 sdResult;

	sdResult = SetFdspCtrl(MCI_DSPCTRL_DEF);
	if (sdResult != (SINT32)FDSP_DRV_SUCCESS_STOPED) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_TIMWAIT
				| gsFdspInfo.sInit.dWaitTime),
				0x00);

		McDevIf_ExecutePacket();

		if (gsFdspInfo.cbfunc != NULL) {
			gsFdspInfo.cbfunc(0, FDSP_CB_RESET, 0);
			InitCore();
		}
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	AppFade
 *
 *	Description:
 *			APP Fade setting
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppFade(void)
{
	UINT8 bFade0;
	UINT8 bFade1;
	UINT8 bFade2;

	bFade0 = (UINT8)((gsFdspInfo.dAppFade >> 16) & 0xFFUL);
	bFade1 = (UINT8)((gsFdspInfo.dAppFade >> 8) & 0xFFUL);
	bFade2 = (UINT8)(gsFdspInfo.dAppFade & 0xFFUL);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPFADE0), bFade0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPFADE1), bFade1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPFADE2), bFade2);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	SetAppExec
 *
 *	Description:
 *			APP execution setting
 *	Arguments:
 *			bAppExec0	App23-16
 *			bAppExec1	App15-8
 *			bAppExec2	App7-0
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetAppExec(UINT8 bAppExec0, UINT8 bAppExec1, UINT8 bAppExec2)
{
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC0),
						bAppExec0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC1),
						bAppExec1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPEXEC2),
						bAppExec2);
	McDevIf_ExecutePacket();

	gsFdspInfo.bAppExec0 = bAppExec0;
	gsFdspInfo.bAppExec1 = bAppExec1;
	gsFdspInfo.bAppExec2 = bAppExec2;
}

/****************************************************************************
 *	AppStop
 *
 *	Description:
 *			APP Stop
 *	Arguments:
 *			dAppStop	Stopping APP
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 AppStop(UINT32 dAppStop)
{
	UINT8 bAppStop0;
	UINT8 bAppStop1;
	UINT8 bAppStop2;
	UINT8 bAppStop0Org;
	UINT8 bAppStop1Org;
	UINT8 bAppStop2Org;
	UINT8 bAppExec0;
	UINT8 bAppExec1;
	UINT8 bAppExec2;
	SINT32 sdResult;

	sdResult = (SINT32)MCDRV_SUCCESS;

	if (dAppStop == 0UL)
		return sdResult;

	bAppStop0 = (UINT8)((dAppStop >> 16) & (UINT32)MCB_APPEXEC0);
	bAppStop1 = (UINT8)((dAppStop >> 8) & (UINT32)MCB_APPEXEC1);
	bAppStop2 = (UINT8)(dAppStop & (UINT32)MCB_APPEXEC2);
	bAppStop0Org = bAppStop0;
	bAppStop1Org = bAppStop1;
	bAppStop2Org = bAppStop2;

	bAppStop0 = (UINT8)(gsFdspInfo.bAppExec0 & bAppStop0);
	bAppStop1 = (UINT8)(gsFdspInfo.bAppExec1 & bAppStop1);
	bAppStop2 = (UINT8)(gsFdspInfo.bAppExec2 & bAppStop2);

	bAppExec0 = (UINT8)(gsFdspInfo.bAppExec0 & (UINT8)~bAppStop0);
	bAppExec1 = (UINT8)(gsFdspInfo.bAppExec1 & (UINT8)~bAppStop1);
	bAppExec2 = (UINT8)(gsFdspInfo.bAppExec2 & (UINT8)~bAppStop2);
	SetAppExec(bAppExec0, bAppExec1, bAppExec2);

	if (bAppStop0Org != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_F_REG_FLAG_RESET
					| (((UINT32)MCI_APPACT0) << 8)
					| (UINT32)bAppStop0Org),  0);

		sdResult = McDevIf_ExecutePacket();
		if (sdResult < MCDRV_SUCCESS)
			return sdResult;
	}
	if (bAppStop1Org != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_F_REG_FLAG_RESET
					| (((UINT32)MCI_APPACT1) << 8)
					| (UINT32)bAppStop1Org),  0);

		sdResult = McDevIf_ExecutePacket();
		if (sdResult < MCDRV_SUCCESS)
			return sdResult;
	}
	if (bAppStop2Org != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_F_REG_FLAG_RESET
					| (((UINT32)MCI_APPACT2) << 8)
					| (UINT32)bAppStop2Org),  0);

		sdResult = McDevIf_ExecutePacket();
		if (sdResult < MCDRV_SUCCESS)
			return sdResult;
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_IREQAPP0), bAppStop0Org);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_IREQAPP1), bAppStop1Org);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_IREQAPP2), bAppStop2Org);

	McDevIf_ExecutePacket();

	return sdResult;
}

/****************************************************************************
 *	Stop
 *
 *	Description:
 *			Stop of F-DSP and APP
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 Stop(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	SINT32 sdResult;

	sdResult = (SINT32)MCDRV_SUCCESS;

	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) != 0) {
		if ((gsFdspInfo.bDSPCtl & (UINT8)MCB_DSPSTART) != 0) {
			sdResult = FixAppVolFadeOut();
			if (sdResult < MCDRV_SUCCESS)
				return sdResult;

			sdResult = DSPTransWait();
			if (sdResult < MCDRV_SUCCESS)
				return sdResult;

		}
		sdResult = FdspStop();
	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP) != 0) {
		sdResult = DSPTransWait();
		if (sdResult < MCDRV_SUCCESS)
			return sdResult;

		AppFade();
		sdResult = AppStop(psFdspInfo->dTargetApp);

	} else if ((psFdspInfo->bMustStop & STOP_KIND_WAIT) != 0)
		sdResult = DSPTransWait();

	return sdResult;
}

/****************************************************************************
 *	FwCtlDL
 *
 *	Description:
 *			FWCTL Setting
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void FwCtlDL(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	UINT8 bData;
	UINT8 *pbFwCtl;
	UINT32 i;

	pbFwCtl = psFdspInfo->pbFwctrl;

	/* AppExec */
	for (i = 0; i < (UINT32)FDSP_APP_NUM; ++i) {
		if (i != FIX_APP_VOL_NO) {
			switch (pbFwCtl[FWCTL_APPEXEC + i]) {
			case FDSP_APP_EXEC_STOP:
				psFdspInfo->dAppExec &= ~(0x01UL << i);
				break;

			case FDSP_APP_EXEC_START:
				psFdspInfo->dAppExec |= (0x01UL << i);
				break;

			default:
				break;
			}
		}
	}

	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) == 0)
		return;

	/* FWMod */
	bData = (UINT8)((pbFwCtl[FWCTL_FWMOD] << 4) & MCB_FWMOD);
	/* Fs */
	if (pbFwCtl[FWCTL_FS] != 0xFF) {
		bData |= (UINT8)(pbFwCtl[FWCTL_FS] & MCB_FS);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_FWMOD),
					bData);
	}

	/* FadeCode */
	if (pbFwCtl[FWCTL_FADECODE] != 0xFF) {
		bData = (pbFwCtl[FWCTL_FADECODE] & MCB_FADECODE);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_F
			| (UINT32)MCI_FADECODE),
			bData);
	}

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	ChSelDL
 *
 *	Description:
 *			CHSEL Setting
 *	Arguments:
 *			pbChSel			CHSEL chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ChSelDL(UINT8 *pbChSel)
{
	UINT8 bData;

	/* Bypass */
	gsFdspInfo.bDSPBypass =
			(UINT8)((pbChSel[CHSEL_BYPASS] << 7) & MCB_DSPBYPASS);

	/* InFdspSrc */
	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 1] << 4) & MCB_ADI01CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 0] & MCB_ADI00CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL0),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 3] << 4) & MCB_ADI03CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 2] & MCB_ADI02CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL1),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 5] << 4) & MCB_ADI05CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 4] & MCB_ADI04CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL2),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 7] << 4) & MCB_ADI07CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 6] & MCB_ADI06CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL3),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 9] << 4) & MCB_ADI09CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 8] & MCB_ADI08CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL4),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 11] << 4) & MCB_ADI11CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 10] & MCB_ADI10CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL5),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 13] << 4) & MCB_ADI13CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 12] & MCB_ADI12CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL6),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_INFDSPSRC + 15] << 4) & MCB_ADI15CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_INFDSPSRC + 14] & MCB_ADI14CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADICSEL7),
						bData);

	/* OutAudioSrc */
	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 1] << 4) & MCB_ADO01CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 0] & MCB_ADO00CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL0),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 3] << 4) & MCB_ADO03CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 2] & MCB_ADO02CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL1),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 5] << 4) & MCB_ADO05CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 4] & MCB_ADO04CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL2),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 7] << 4) & MCB_ADO07CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 6] & MCB_ADO06CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL3),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 9] << 4) & MCB_ADO09CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 8] & MCB_ADO08CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL4),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 11] << 4)
							& MCB_ADO11CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 10] & MCB_ADO10CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL5),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 13] << 4)
							& MCB_ADO13CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 12] & MCB_ADO12CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL6),
						bData);

	bData = (UINT8)((pbChSel[CHSEL_OUTAUDIOSRC + 15] << 4)
							& MCB_ADO15CSEL);
	bData |= (UINT8)(pbChSel[CHSEL_OUTAUDIOSRC + 14] & MCB_ADO14CSEL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOCSEL7),
						bData);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	TopEnvDL
 *
 *	Description:
 *			TOP environmental setting
 *	Arguments:
 *			pbTopEnv	TOP environmental chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void TopEnvDL(UINT8 *pbTopEnv)
{
	UINT8 bData;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_19_16),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_15_8),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_7_0),
						0x08);

	/* OMACtl */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FDSPTINI), MCI_FDSPTINI_DEF);
	McDevIf_ExecutePacket();

	/* 0x00008 */
	/* ID */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_FMAD),
			pbTopEnv[TOP_ENV_ID]);
	/* InstBase */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbTopEnv[TOP_ENV_INSTBASE + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_INSTBASE + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_INSTBASE + 3]);

	/* 0x00009 */
	/* MBlkSize */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAD),
				(UINT8)(pbTopEnv[TOP_ENV_MBLKSIZE] & 0x1F));
	/* MBufSize */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAD),
				(UINT8)(pbTopEnv[TOP_ENV_MBUFSIZE] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* MOChCnfg */
	bData = ((pbTopEnv[TOP_ENV_MOCHCNFG] << 4) & 0xF0);
	/* MIChCnfg */
	bData |= (pbTopEnv[TOP_ENV_MICHCNFG] & 0x0F);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						bData);

	/* 0x0000A */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* SStartCh */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAD),
				(UINT8)(pbTopEnv[TOP_ENV_SSTARTCH] & 0x0F));
	/* SNumOfOCh */
	bData = ((pbTopEnv[TOP_ENV_SNUMOFOCH] << 4) & 0xF0);
	/* SNumOfICh */
	bData |= (pbTopEnv[TOP_ENV_SNUMOFICH] & 0x0F);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						bData);

	/* 0x0000B */
	/* SaveBufSize1 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_SAVEBUFSIZE1 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_SAVEBUFSIZE1 + 3]);
	/* SaveBufSize0 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_SAVEBUFSIZE0 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_SAVEBUFSIZE0 + 3]);

	/* 0x0000C */
	/* LimitWorkSize */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbTopEnv[TOP_ENV_LIMITWORKSIZE + 1] & 0x0F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_LIMITWORKSIZE + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_LIMITWORKSIZE + 3]);

	/* 0x0000D */
	/* WorkBase */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbTopEnv[TOP_ENV_WORKBASE + 1] & 0x0F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_WORKBASE + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_WORKBASE + 3]);

	/* 0x0000E */
	/* TopBase0 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbTopEnv[TOP_ENV_TOPBASE0 + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_TOPBASE0 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbTopEnv[TOP_ENV_TOPBASE0 + 3]);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	TopCoefDL
 *
 *	Description:
 *			TOP coefficient setting
 *	Arguments:
 *			pbTopCoef	TOP coefficient chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void TopCoefDL(UINT8 *pbTopCoef)
{
	UINT32 dSize;
	UINT32 i;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_19_16),
			(UINT8)(pbTopCoef[TOP_COEF_ADR + 1] & MCB_FMAA_19_16));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_15_8),
			(UINT8)(pbTopCoef[TOP_COEF_ADR + 2] & MCB_FMAA_15_8));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_7_0),
			(UINT8)(pbTopCoef[TOP_COEF_ADR + 3] & MCB_FMAA_7_0));

	/* OMACtl */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTINI),
						MCI_FDSPTINI_DEF);

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbTopCoef[TOP_COEF_SIZE + 0],
					pbTopCoef[TOP_COEF_SIZE + 1],
					pbTopCoef[TOP_COEF_SIZE + 2],
					pbTopCoef[TOP_COEF_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbTopCoef[TOP_COEF_DATA + i]);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	AppCoefDL
 *
 *	Description:
 *			APP coefficient setting
 *	Arguments:
 *			pbAppCoef	APP coefficient chunk pointer
 *			bCoefTrans
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
			| (UINT32)MCI_FMAA_19_16),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 1] & MCB_FMAA_19_16));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_15_8),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 2] & MCB_FMAA_15_8));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_7_0),
			(UINT8)(pbAppCoef[APP_COEF_ADR + 3] & MCB_FMAA_7_0));

	/* OMACtl */
	if (bCoefTrans == COEF_DSP_TRANS)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FDSPTINI),
					(UINT8)(MCB_DSPTINI | MCB_FMAMOD_DSP));
	else
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTINI),
						MCI_FDSPTINI_DEF);

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbAppCoef[APP_COEF_SIZE + 0],
						pbAppCoef[APP_COEF_SIZE + 1],
						pbAppCoef[APP_COEF_SIZE + 2],
						pbAppCoef[APP_COEF_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppCoef[APP_COEF_DATA + i]);
	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	AppRegDL
 *
 *	Description:
 *			APP register setting
 *	Arguments:
 *			pbAppReg	APP register chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppRegDL(UINT8 *pbAppReg)
{
	UINT32 dSize;
	UINT32 i;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_F_REG_A),
			(UINT8)(pbAppReg[APP_REG_ADR + 3] | MCB_F_REG_AINC));

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbAppReg[APP_REG_SIZE + 0],
						pbAppReg[APP_REG_SIZE + 1],
						pbAppReg[APP_REG_SIZE + 2],
						pbAppReg[APP_REG_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_F_REG_D),
						pbAppReg[APP_REG_DATA + i]);
	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	MultiChunkDL
 *
 *	Description:
 *			Download of multidata input possible chunk
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *			dTargetChunk	target chunk
 *	Return:
 *			none
 *
 ****************************************************************************/
static void MultiChunkDL(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo,
							UINT32 dTargetChunk)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT32 dAppNo;
	UINT8 *pbPrm;
	UINT32 dSize;

	pbPrm = psFdspInfo->pbChunkData;
	dSize = psFdspInfo->dwSize;

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

		dAppNo = (dChunkId & (UINT32)AEC_FDSP_TAG_APPNO_MASK);
		switch ((dChunkId & (UINT32)AEC_FDSP_TAG_APP_MASK)) {
		case AEC_FDSP_TAG_APP_COEF:
		case AEC_FDSP_TAG_APP_CONST:
			if (dTargetChunk == (UINT32)MULTI_CHUNK_APP_COEF)
				if (dAppNo < (UINT32)FDSP_APP_NUM)
					AppCoefDL(&pbPrm[dChunkTop],
						psFdspInfo->bCoefTrans);
			break;

		case AEC_FDSP_TAG_APP_REG:
			if (dTargetChunk == (UINT32)MULTI_CHUNK_APP_REG)
				if (dAppNo < (UINT32)FDSP_APP_NUM)
					AppRegDL(&pbPrm[dChunkTop]);
			break;

		default:
			if ((dChunkId == AEC_FDSP_TAG_TOP_COEF) &&
				(dTargetChunk ==
					(UINT32)MULTI_CHUNK_TOP_COEF))
				TopCoefDL(&pbPrm[dChunkTop]);
			break;
		}
		dChunkTop += dChunkSize;
	}
}

/****************************************************************************
 *	TopInstDL
 *
 *	Description:
 *			TOP instruction setting
 *	Arguments:
 *			pbTopInst	TOP instruction chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void TopInstDL(UINT8 *pbTopInst)
{
	UINT32 dSize;
	UINT32 i;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_19_16),
			(UINT8)(pbTopInst[TOP_INST_ADR + 1] & MCB_FMAA_19_16));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_15_8),
			(UINT8)(pbTopInst[TOP_INST_ADR + 2] & MCB_FMAA_15_8));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_7_0),
			(UINT8)(pbTopInst[TOP_INST_ADR + 3] & MCB_FMAA_7_0));

	/* OMACtl */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTINI),
						MCB_FMABUS_I);

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbTopInst[TOP_INST_SIZE + 0],
						pbTopInst[TOP_INST_SIZE + 1],
						pbTopInst[TOP_INST_SIZE + 2],
						pbTopInst[TOP_INST_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbTopInst[TOP_INST_DATA + i]);
	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	FixAppEnvDL
 *
 *	Description:
 *			Fix APP setting
 *	Arguments:
 *			dAppNo		APP number
 *			pbAppEnv	APP environmental chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetFixApp(UINT32 dAppNo, UINT8 *pbAppEnv)
{
	if (dAppNo != FIX_APP_VOL_NO)
		return;

	if (pbAppEnv[APP_ENV_ID] == APP_VOL_ID)
		gsFdspInfo.bFixAppVol = FIX_APP_VOL_EXIST;
	else
		gsFdspInfo.bFixAppVol = FIX_APP_VOL_FREE;
}

/****************************************************************************
 *	AppEnvDL
 *
 *	Description:
 *			APP environmental setting
 *	Arguments:
 *			dAppNo		APP number
 *			pbAppEnv	APP environmental chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppEnvDL(UINT32 dAppNo, UINT8 *pbAppEnv)
{
	UINT32 dBase;
	UINT8 bData;

	dBase = 0x00010UL + (dAppNo * 8UL);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAA_19_16),
				(UINT8)((dBase >> 16)
				& (UINT32)MCB_FMAA_19_16));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAA_15_8),
				(UINT8)((dBase >> 8) & (UINT32)MCB_FMAA_15_8));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAA_7_0),
					(UINT8)(dBase & (UINT32)MCB_FMAA_7_0));

	/* OMACtl */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTINI),
						MCI_FDSPTINI_DEF);

	/* Base + 0x00000 */
	/* ID */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppEnv[APP_ENV_ID]);
	/* ExcecProcess */
	bData = ((pbAppEnv[APP_ENV_EXCECPROCESS] << 7) & 0x80);
	/* InstBase */
	bData |= (pbAppEnv[APP_ENV_INSTBASE + 1] & 0x1F);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						bData);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_INSTBASE + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_INSTBASE + 3]);

	/* Base + 0x00001 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* RegBase */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAD),
				(UINT8)(pbAppEnv[APP_ENV_REGBASE + 3] & 0x7F));
	/* ExecFs */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FMAD),
				(UINT8)(pbAppEnv[APP_ENV_EXECFS + 2] & 0x7F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppEnv[APP_ENV_EXECFS + 3]);

	/* Base + 0x00002 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* ExecCh */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppEnv[APP_ENV_EXECCH + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppEnv[APP_ENV_EXECCH + 3]);

	/* Base + 0x00003 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* WorkBase */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbAppEnv[APP_ENV_WORKBASE + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_WORKBASE + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_WORKBASE + 3]);

	/* Base + 0x00004 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* AppBase0 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbAppEnv[APP_ENV_APPBASE0 + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE0 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE0 + 3]);

	/* Base + 0x00005 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* AppBase1 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbAppEnv[APP_ENV_APPBASE1 + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE1 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE1 + 3]);

	/* Base + 0x00006 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* AppBase2 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbAppEnv[APP_ENV_APPBASE2 + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE2 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE2 + 3]);

	/* Base + 0x00007 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						0x00);
	/* AppBase3 */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAD),
			(UINT8)(pbAppEnv[APP_ENV_APPBASE3 + 1] & 0x1F));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE3 + 2]);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FMAD),
					pbAppEnv[APP_ENV_APPBASE3 + 3]);

	McDevIf_ExecutePacket();

	/* register */
	/* Fade */
	if ((pbAppEnv[APP_ENV_APPFADE] & APP_PARAM_ON) == APP_PARAM_ON)
		gsFdspInfo.dAppFade |= (UINT32)(0x01UL << dAppNo);
	else
		gsFdspInfo.dAppFade &= (UINT32)~(0x01UL << dAppNo);

	/* irq */
	if ((pbAppEnv[APP_ENV_APPIRQ] & APP_PARAM_ON) == APP_PARAM_ON)
		gsFdspInfo.dAppIEnb |= (UINT32)(0x01UL << dAppNo);
	else
		gsFdspInfo.dAppIEnb &= (UINT32)~(0x01UL << dAppNo);
}

/****************************************************************************
 *	AppInstDL
 *
 *	Description:
 *			APP instruction setting
 *	Arguments:
 *			pbAppInst	APP instruction chunk pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void AppInstDL(UINT8 *pbAppInst)
{
	UINT32 dSize;
	UINT32 i;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_19_16),
			(UINT8)(pbAppInst[APP_INST_ADR + 1] & MCB_FMAA_19_16));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_15_8),
			(UINT8)(pbAppInst[APP_INST_ADR + 2] & MCB_FMAA_15_8));
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_FMAA_7_0),
			(UINT8)(pbAppInst[APP_INST_ADR + 3] & MCB_FMAA_7_0));

	/* OMACtl */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FDSPTINI), MCB_FMABUS_I);

	McDevIf_ExecutePacket();

	dSize = CreateUINT32(pbAppInst[APP_INST_SIZE + 0],
						pbAppInst[APP_INST_SIZE + 1],
						pbAppInst[APP_INST_SIZE + 2],
						pbAppInst[APP_INST_SIZE + 3]);

	for (i = 0; i < dSize; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAD),
						pbAppInst[APP_INST_DATA + i]);
	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	Download
 *
 *	Description:
 *			Download of FW and coefficient to F-DSP
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void Download(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	UINT32 i;

	if (psFdspInfo->pbFwctrl != NULL)
		FwCtlDL(psFdspInfo);

	if (psFdspInfo->pbChSel != NULL)
		ChSelDL(psFdspInfo->pbChSel);

	if (psFdspInfo->pbTopEnv != NULL)
		TopEnvDL(psFdspInfo->pbTopEnv);

	if (psFdspInfo->dTopCoefCnt != 0UL)
		MultiChunkDL(psFdspInfo, MULTI_CHUNK_TOP_COEF);

	if (psFdspInfo->pbTopInst != NULL)
		TopInstDL(psFdspInfo->pbTopInst);

	if (psFdspInfo->dTargetApp != 0UL)
		for (i = 0; i < (UINT32)FDSP_APP_NUM; ++i)
			if (psFdspInfo->apbAppEnv[i] != NULL) {
				SetFixApp(i, psFdspInfo->apbAppEnv[i]);
				AppEnvDL(i, psFdspInfo->apbAppEnv[i]);
			}

	if ((psFdspInfo->dAppCoefCnt != 0UL) ||
		(psFdspInfo->dAppCnstCnt != 0UL)) {
		MultiChunkDL(psFdspInfo, MULTI_CHUNK_APP_COEF);

		if (psFdspInfo->bCoefTrans == COEF_DSP_TRANS) {
			/* DSPTReq */
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTREQ),
						MCB_FDSPTREQ);
			McDevIf_ExecutePacket();
		}
	}

	if (psFdspInfo->dAppInstCnt != 0UL)
		for (i = 0UL; i < (UINT32)FDSP_APP_NUM; ++i)
			if (psFdspInfo->apbAppInst[i] != NULL)
				AppInstDL(psFdspInfo->apbAppInst[i]);

	if (psFdspInfo->dAppRegCnt != 0UL)
		MultiChunkDL(psFdspInfo, MULTI_CHUNK_APP_REG);
}

/****************************************************************************
 *	EnableIrq
 *
 *	Description:
 *			Interrupt enable setting
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *			psExecInfo	MCDRV_FDSP_EXEC_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void EnableIrq(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo,
				struct MCDRV_FDSP_EXEC_INFO *psExecInfo)
{
	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) != 0) {
		/* IReqTop */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_IREQTOP), MCB_IREQTOP);

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					(UINT8)(MCB_IRSERR | MCB_IRFW));

		McDevIf_ExecutePacket();

		SetAppIrq((gsFdspInfo.dAppIEnb & psExecInfo->dAppExec));

		/* TopIEnb */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_TOPIENB), MCB_TOPIENB);

		/* IEnb */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR),
					(UINT8)(MCB_IESERR | MCB_IEFW));

		McDevIf_ExecutePacket();

	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP_EXEC) != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					MCB_IRFW_DSP);

		McDevIf_ExecutePacket();

		SetAppIrq(gsFdspInfo.dAppIEnb & psExecInfo->dAppExec);

	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP) != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					MCB_IRFW_DSP);

		McDevIf_ExecutePacket();

		SetAppIrq(gsFdspInfo.dAppIEnb & psExecInfo->dAppExec);

	} else if ((psFdspInfo->bMustStop & STOP_KIND_WAIT) != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					MCB_IRFW_DSP);

		McDevIf_ExecutePacket();
	}
}

/****************************************************************************
 *	FixAppVolFadeIn
 *
 *	Description:
 *			Fixed VOL fadein
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static void FixAppVolFadeIn(void)
{
	if (gsFdspInfo.bFixAppVol != FIX_APP_VOL_EXIST)
		return;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)FIX_APP_VOL_REG_FADE_CTRL),
					FIX_APP_VOL_REG_FADE_CTRL_BIT);
	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	ReStart
 *
 *	Description:
 *			re-execution of stop F-DSP and APP
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *			psExecInfo	MCDRV_FDSP_EXEC_INFO
 *					structure pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ReStart(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo,
				struct MCDRV_FDSP_EXEC_INFO *psExecInfo)
{
	UINT8 bAppExec0;
	UINT8 bAppExec1;
	UINT8 bAppExec2;
	UINT32 dAppStop;
	UINT32 dAppAct;

	bAppExec0 = (UINT8)((psExecInfo->dAppExec >> 16)
						& (UINT32)MCB_APPEXEC0);
	bAppExec1 = (UINT8)((psExecInfo->dAppExec >> 8)
						& (UINT32)MCB_APPEXEC1);
	bAppExec2 = (UINT8)(psExecInfo->dAppExec & (UINT32)MCB_APPEXEC2);

	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) != 0) {
		SetAppExec(bAppExec0, bAppExec1, bAppExec2);
		if (psExecInfo->bRestart == RESTART_ON) {
			gsFdspInfo.dAppStop = 0UL;
			if ((gsFdspInfo.bDSPBypass & MCB_DSPBYPASS) != 0)
				SetFdspCtrl(MCB_DSPBYPASS);
			else {
				SetFdspCtrl(MCB_DSPSTART);
				FixAppVolFadeIn();
			}
		}
	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP_EXEC) != 0) {
		dAppStop = ((UINT32)(gsFdspInfo.bAppExec0 & ~bAppExec0)) << 16;
		dAppStop |= ((UINT32)(gsFdspInfo.bAppExec1 & ~bAppExec1)) << 8;
		dAppStop |= ((UINT32)(gsFdspInfo.bAppExec2 & ~bAppExec2));

		AppFade();
		SetAppExec(bAppExec0, bAppExec1, bAppExec2);
		FixAppVolFadeIn();

		dAppAct = GetAppAct();
		if ((dAppStop & dAppAct) != dAppStop)
			dAppStop &= dAppAct;

		gsFdspInfo.dAppStop |= dAppStop;
	} else if ((psFdspInfo->bMustStop & STOP_KIND_APP) != 0) {
		AppFade();
		SetAppExec(bAppExec0, bAppExec1, bAppExec2);
		FixAppVolFadeIn();
	}
}

/****************************************************************************
 *	SetAudioEngine
 *
 *	Description:
 *			FW and the coefficient setting are set to F-DSP
 *	Arguments:
 *			psFdspInfo	MCDRV_FDSP_AEC_FDSP_INFO
 *					structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32 SetAudioEngine(struct MCDRV_FDSP_AEC_FDSP_INFO *psFdspInfo)
{
	UINT8 bData;
	SINT32 sdResult;
	struct MCDRV_FDSP_EXEC_INFO sExecInfo;

	psFdspInfo->dAppExec = CreateUINT32(0x00, gsFdspInfo.bAppExec0,
				gsFdspInfo.bAppExec1, gsFdspInfo.bAppExec2);

	if (gsFdspInfo.bDSPCtl != MCI_DSPCTRL_DEF)
		sExecInfo.bRestart = RESTART_ON;
	else
		sExecInfo.bRestart = RESTART_OFF;

	if ((gsFdspInfo.bDSPCtl & (UINT8)MCB_DSPBYPASS) != 0)
		psFdspInfo->bMustStop |= STOP_KIND_FDSP;
	else if ((gsFdspInfo.bDSPCtl & (UINT8)MCB_DSPSTART) == 0) {
		psFdspInfo->bMustStop |= STOP_KIND_FDSP;
		psFdspInfo->bCoefTrans = COEF_DMA_TRANS;
	} else {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_DSPSTATE),
						&bData, 1);
		if ((bData & MCB_DSPACT) != MCB_DSPACT)
			psFdspInfo->bCoefTrans = COEF_DMA_TRANS;
	}

	DisableIrq(psFdspInfo);

	sdResult = Stop(psFdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	if ((psFdspInfo->bMustStop & STOP_KIND_FDSP) != 0) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE0),
						0x00);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE1),
						0x00);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_APPFADE2),
						0x00);
	}

	Download(psFdspInfo);

	sExecInfo.dAppExec = psFdspInfo->dAppExec;
	if (gsFdspInfo.bFixAppVol == FIX_APP_VOL_EXIST)
		sExecInfo.dAppExec |= FIX_APP_VOL_APPEXEC;
	else
		sExecInfo.dAppExec &= ~FIX_APP_VOL_APPEXEC;

	EnableIrq(psFdspInfo, &sExecInfo);

	ReStart(psFdspInfo, &sExecInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_Init
 *
 *	Description:
 *			Initialize
 *	Arguments:
 *			psPrm	MCDRV_FDSP_INIT structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_Init(struct MCDRV_FDSP_INIT *psPrm)
{
	if (gsFdspInfo.dStatus != (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	gsFdspInfo.sInit.sInput.wADDFmt = psPrm->sInput.wADDFmt;
	gsFdspInfo.sInit.sOutput.wADDFmt = psPrm->sOutput.wADDFmt;

	if (psPrm->dWaitTime != 0)
		gsFdspInfo.sInit.dWaitTime = psPrm->dWaitTime;
	else
		gsFdspInfo.sInit.dWaitTime = DEF_FDSP_STOP_WAIT_TIME_US;

	gsFdspInfo.cbfunc = NULL;
	gsFdspInfo.bADIMute0 = MCI_ADIMUTE0_DEF;
	gsFdspInfo.bADIMute1 = MCI_ADIMUTE1_DEF;
	gsFdspInfo.bADOMute0 = MCI_ADOMUTE0_DEF;
	gsFdspInfo.bADOMute1 = MCI_ADOMUTE1_DEF;

	InitCore();

	gsFdspInfo.dStatus = (UINT32)FDSP_STATUS_INITED;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_Term
 *
 *	Description:
 *			Terminate
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McFdsp_Term(void)
{
	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_SUCCESS;

	/* IEnb */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IESERR),
					MCI_IESERR_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					(UINT8)(MCB_IRSERR | MCB_IRFW));

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_DSPCTRL),
					MCI_DSPCTRL_DEF);

	McDevIf_ExecutePacket();

	gsFdspInfo.cbfunc = NULL;
	gsFdspInfo.dStatus = (UINT32)FDSP_STATUS_IDLE;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_IrqProc
 *
 *	Description:
 *			Interrupt from F-DSP is processed
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void McFdsp_IrqProc(void)
{
	UINT8 bErr;
	UINT8 bIReq;
	UINT8 bIReqTop;
	UINT8 bIReqApp0;
	UINT8 bIReqApp1;
	UINT8 bIReqApp2;
	UINT32 dAppStop;
	UINT32 dData;

	bErr = IRQ_NO_ERROR;
	bIReqTop = 0;
	bIReqApp0 = 0;
	bIReqApp1 = 0;
	bIReqApp2 = 0;
	dAppStop = 0;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					&bIReq, 1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_IRSERR),
					bIReq);

	McDevIf_ExecutePacket();

	if (((bIReq & MCB_IRSERR) == MCB_IRSERR) ||
		((bIReq & MCB_IRFW_STRT) == MCB_IRFW_STRT))
		bErr = IRQ_ERROR;

	if ((bIReq & MCB_IRFW_TOP) == MCB_IRFW_TOP) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
							| (UINT32)MCI_IREQTOP),
							&bIReqTop, 1);

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQTOP),
						bIReqTop);

		McDevIf_ExecutePacket();

		if (((bIReqTop & MCB_IREQTOP3) == MCB_IREQTOP3) ||
			((bIReqTop & MCB_IREQTOP2) == MCB_IREQTOP2))
			bErr = IRQ_ERROR;
	}

	if ((bIReq & MCB_IRFW_APP) == MCB_IRFW_APP) {
		if (gsFdspInfo.bAppIEnb0 != 0) {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP0),
						&bIReqApp0, 1);

			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP0),
						bIReqApp0);

			McDevIf_ExecutePacket();

			bIReqApp0 = (UINT8)(bIReqApp0 & gsFdspInfo.bAppIEnb0);
		}
		if (gsFdspInfo.bAppIEnb1 != 0) {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP1),
						&bIReqApp1, 1);

			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP1),
						bIReqApp1);

			McDevIf_ExecutePacket();

			bIReqApp2 = (UINT8)(bIReqApp1 & gsFdspInfo.bAppIEnb1);
		}
		if (gsFdspInfo.bAppIEnb2 != 0) {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP2),
						&bIReqApp2, 1);

			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_IREQAPP2),
						bIReqApp2);

			McDevIf_ExecutePacket();
			bIReqApp2 = (UINT8)(bIReqApp2 & gsFdspInfo.bAppIEnb2);
		}
	}

	if ((bIReqTop & MCB_IREQTOP0) == MCB_IREQTOP0) {
		dData = GetAppAct();
		dAppStop = (gsFdspInfo.dAppStop & ~dData);
		gsFdspInfo.dAppStop &= ~dAppStop;
	}

	if (gsFdspInfo.cbfunc != NULL) {
		if (bErr == IRQ_ERROR)
			gsFdspInfo.cbfunc(0, FDSP_CB_ERR, 0);

		if ((bIReq & MCB_IRFW_DSP) == MCB_IRFW_DSP)
			gsFdspInfo.cbfunc(0, FDSP_CB_COEF_DONE, 0);

		dData = CreateUINT32(0x00, bIReqApp0, bIReqApp1, bIReqApp2);
		dData &= ~0x400000;
		if (dData != 0UL)
			gsFdspInfo.cbfunc(0, FDSP_CB_APP_REQ, dData);

		if (dAppStop != 0UL)
			gsFdspInfo.cbfunc(0, FDSP_CB_APP_STOP, dAppStop);

		if ((bIReqTop & MCB_IREQTOP1) == MCB_IREQTOP1)
			gsFdspInfo.cbfunc(0, FDSP_CB_FDSP_STOP, 0);
	}
}

/****************************************************************************
 *	McFdsp_SetCBFunc
 *
 *	Description:
 *			Callback function setting
 *	Arguments:
 *			cbfunc	Callback function
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McFdsp_SetCBFunc(SINT32 (*cbfunc)(SINT32, UINT32, UINT32))
{
	gsFdspInfo.cbfunc = cbfunc;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_GetTransition
 *
 *	Description:
 *			It judges while processing the F-DSP control
 *	Arguments:
 *			none
 *	Return:
 *			0	has processed
 *			1<=	processing
 *
 ****************************************************************************/
SINT32 McFdsp_GetTransition(void)
{
	SINT32 sdResult;
	UINT32 dApp;
	UINT8 bData;

	sdResult = 0;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return sdResult;

	/* DSPSTART,DSPACT */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_DSPSTATE), &bData, 1);
	if ((gsFdspInfo.bDSPCtl & MCB_DSPSTART) == 0) {
		if ((bData & MCB_DSPACT) == MCB_DSPACT)
			sdResult += (SINT32)FDSP_PRC_FDSP_STOP;
		else
			return sdResult;
	}

	/* DSP Coef Trance */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_FDSPTREQ), &bData, 1);
	if ((bData & MCB_FDSPTREQ) == MCB_FDSPTREQ)
		sdResult += (SINT32)FDSP_PRC_COEF_TRS;

	/* APPEXEC,APPACT */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT0), &bData, 1);
	dApp = (UINT32)(bData & (UINT8)~gsFdspInfo.bAppExec0);
	dApp = (dApp << 8);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT1), &bData, 1);
	dApp |= (UINT32)(bData & (UINT8)~gsFdspInfo.bAppExec1);
	dApp = (dApp << 8);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_F
					| (UINT32)MCI_APPACT2), &bData, 1);
	dApp |= (UINT32)(bData & (UINT8)~gsFdspInfo.bAppExec2);
	sdResult += (SINT32)dApp;

	return sdResult;
}

/****************************************************************************
 *	McFdsp_GetDSP
 *
 *	Description:
 *			FW and the coefficient setting are acquired from F-DSP
 *	Arguments:
 *			psPrm	MCDRV_FDSP_AE_FW structure pointer
 *	Return:
 *			0<=	Get data size
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_GetDSP(UINT32 dTarget, UINT32 dAddress,
						UINT8 *pbData, UINT32 dSize)
{
	UINT8 bOMACtl;
	UINT8 bAdr0;
	UINT8 bAdr1;
	UINT8 bAdr2;
	UINT8 bHwId;
	UINT32 i;
	UINT32 dDxramMax;
	UINT32 dIramMax;

	if (pbData == NULL)
		return MCDRV_ERROR_ARGUMENT;

	bHwId = McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_A_DEV_ID);
	if ((bHwId & 0x07) == 0) {
		dDxramMax = DXRAM_RANGE_MAX_1;
		dIramMax = IRAM_RANGE_MAX_1;
	} else {
		dDxramMax = DXRAM_RANGE_MAX_2;
		dIramMax = IRAM_RANGE_MAX_2;
	}

	dSize = (dSize / RAM_UNIT_SIZE);

	bOMACtl = MCB_FMADIR_UL;
	switch (dTarget) {
	case FDSP_AE_FW_DXRAM:
		if (dDxramMax < dAddress)
			return MCDRV_ERROR_ARGUMENT;

		if ((dDxramMax + 1) < dSize)
			dSize = dDxramMax + 1;

		if ((dDxramMax + 1) < (dAddress + dSize))
			dSize -= ((dAddress + dSize)
						- (dDxramMax + 1));
		bOMACtl |= MCB_FMABUS_X;
		break;

	case FDSP_AE_FW_DYRAM:
		if (DYRAM_RANGE_MAX < dAddress)
			return MCDRV_ERROR_ARGUMENT;

		if ((DYRAM_RANGE_MAX + 1) < dSize)
			dSize = (DYRAM_RANGE_MAX + 1);

		if ((DYRAM_RANGE_MAX + 1) < (dAddress + dSize))
			dSize -= ((dAddress + dSize)
						- (DYRAM_RANGE_MAX + 1));
		bOMACtl |= MCB_FMABUS_Y;
		break;

	case FDSP_AE_FW_IRAM:
		if ((dAddress < IRAM_RANGE_MIN) ||
			 (dIramMax < dAddress))
			return MCDRV_ERROR_ARGUMENT;

		if ((dIramMax - IRAM_RANGE_MIN + 1) < dSize)
			dSize = (dIramMax - IRAM_RANGE_MIN + 1);

		if ((dIramMax + 1) < (dAddress + dSize))
			dSize -= ((dAddress + dSize)
						- (dIramMax + 1));
		bOMACtl |= MCB_FMABUS_I;
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	if (dSize == 0)
		return 0;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return 0;

	bAdr0 = (UINT8)((dAddress >> 16) & (UINT32)MCB_FMAA_19_16);
	bAdr1 = (UINT8)((dAddress >> 8) & (UINT32)MCB_FMAA_15_8);
	bAdr2 = (UINT8)(dAddress & (UINT32)MCB_FMAA_7_0);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_19_16),
						bAdr0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_15_8),
						bAdr1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FMAA_7_0),
						bAdr2);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FDSPTINI),
						bOMACtl);

	McDevIf_ExecutePacket();

	dSize *= RAM_UNIT_SIZE;
	for (i = 0; i < dSize; ++i)
		McDevIf_ReadDirect(
				(MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_FMAD),
				&pbData[i], 1);

	return (SINT32)dSize;
}

/****************************************************************************
 *	McFdsp_SetDSPCheck
 *
 *	Description:
 *			Check AEC info
 *	Arguments:
 *			psPrm		MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_SetDSPCheck(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_FDSP_AEC_FDSP_INFO sFdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	sdResult = GetFDSPChunk(psPrm, &sFdspInfo) ;
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return MCDRV_ERROR_ARGUMENT;

	sdResult = FdspChunkAnalyze(&sFdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_SetDSP
 *
 *	Description:
 *			FW and the coefficient setting are set to F-DSP
 *	Arguments:
 *			psPrm		MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_PROCESSING
 *			MCDRV_ERROR
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_SetDSP(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_FDSP_AEC_FDSP_INFO sFdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	sdResult = GetFDSPChunk(psPrm, &sFdspInfo) ;
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return MCDRV_ERROR_ARGUMENT;

	sdResult = FdspChunkAnalyze(&sFdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	if ((sFdspInfo.pbChunkData == NULL) ||
		(sFdspInfo.dwSize == 0))
		return MCDRV_SUCCESS;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	sdResult = SetAudioEngine(&sFdspInfo);

	return sdResult;
}

/****************************************************************************
 *	McFdsp_Start
 *
 *	Function:
 *			F-DSP start
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McFdsp_Start(void)
{
	SINT32 sdResult = MCDRV_SUCCESS;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	if ((gsFdspInfo.bDSPBypass & MCB_DSPBYPASS) != 0) {
		gsFdspInfo.dAppStop = 0UL;
		sdResult = SetFdspCtrl(MCB_DSPBYPASS);
	} else {
		if ((gsFdspInfo.bDSPCtl & (UINT8)MCB_DSPSTART) == 0) {
			gsFdspInfo.dAppStop = 0UL;
			sdResult = SetFdspCtrl(MCB_DSPSTART);
			FixAppVolFadeIn();
		}
	}

	return sdResult;
}

/****************************************************************************
 *	McFdsp_Stop
 *
 *	Function:
 *			F-DSP stop
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_PROCESSING
 *
 ****************************************************************************/
SINT32 McFdsp_Stop(void)
{
	SINT32 sdResult;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_SUCCESS;

	if ((gsFdspInfo.bDSPCtl & (UINT8)MCB_DSPSTART) != 0)
		FixAppVolFadeOut();

	sdResult = SetFdspCtrl(MCI_DSPCTRL_DEF);
	if (sdResult == FDSP_DRV_SUCCESS_STOPED)
		sdResult = MCDRV_SUCCESS;
	else
		sdResult = MCDRV_PROCESSING;

	return sdResult;
}

/****************************************************************************
 *	McFdsp_GetMute
 *
 *	Description:
 *			Get Mute Setting
 *	Arguments:
 *			psPrm		MCDRV_FDSP_MUTE structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_GetMute(struct MCDRV_FDSP_MUTE *psPrm)
{
	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	/* get Mute */
	psPrm->abInMute[0] = GetChMute(gsFdspInfo.bADIMute0,
					(UINT8)(MCB_ADI01MTN | MCB_ADI00MTN));
	psPrm->abInMute[1] = GetChMute(gsFdspInfo.bADIMute0,
					(UINT8)(MCB_ADI03MTN | MCB_ADI02MTN));
	psPrm->abInMute[2] = GetChMute(gsFdspInfo.bADIMute0,
					(UINT8)(MCB_ADI05MTN | MCB_ADI04MTN));
	psPrm->abInMute[3] = GetChMute(gsFdspInfo.bADIMute0,
					(UINT8)(MCB_ADI07MTN | MCB_ADI06MTN));
	psPrm->abInMute[4] = GetChMute(gsFdspInfo.bADIMute1,
					(UINT8)(MCB_ADI09MTN | MCB_ADI08MTN));
	psPrm->abInMute[5] = GetChMute(gsFdspInfo.bADIMute1,
					(UINT8)(MCB_ADI11MTN | MCB_ADI10MTN));
	psPrm->abInMute[6] = GetChMute(gsFdspInfo.bADIMute1,
					(UINT8)(MCB_ADI13MTN | MCB_ADI12MTN));
	psPrm->abInMute[7] = GetChMute(gsFdspInfo.bADIMute1,
					(UINT8)(MCB_ADI15MTN | MCB_ADI14MTN));

	psPrm->abOutMute[0] = GetChMute(gsFdspInfo.bADOMute0,
					(UINT8)(MCB_ADO01MTN | MCB_ADO00MTN));
	psPrm->abOutMute[1] = GetChMute(gsFdspInfo.bADOMute0,
					(UINT8)(MCB_ADO03MTN | MCB_ADO02MTN));
	psPrm->abOutMute[2] = GetChMute(gsFdspInfo.bADOMute0,
					(UINT8)(MCB_ADO05MTN | MCB_ADO04MTN));
	psPrm->abOutMute[3] = GetChMute(gsFdspInfo.bADOMute0,
					(UINT8)(MCB_ADO07MTN | MCB_ADO06MTN));
	psPrm->abOutMute[4] = GetChMute(gsFdspInfo.bADOMute1,
					(UINT8)(MCB_ADO09MTN | MCB_ADO08MTN));
	psPrm->abOutMute[5] = GetChMute(gsFdspInfo.bADOMute1,
					(UINT8)(MCB_ADO11MTN | MCB_ADO10MTN));
	psPrm->abOutMute[6] = GetChMute(gsFdspInfo.bADOMute1,
					(UINT8)(MCB_ADO13MTN | MCB_ADO12MTN));
	psPrm->abOutMute[7] = GetChMute(gsFdspInfo.bADOMute1,
					(UINT8)(MCB_ADO15MTN | MCB_ADO14MTN));

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McFdsp_SetMute
 *
 *	Description:
 *			Set Mute
 *	Arguments:
 *			psPrm		MCDRV_FDSP_MUTE structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McFdsp_SetMute(struct MCDRV_FDSP_MUTE *psPrm)
{
	UINT8 bADIMute0;
	UINT8 bADIMute1;
	UINT8 bADOMute0;
	UINT8 bADOMute1;
	UINT32 i;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	for (i = 0UL; i < (UINT32)FDSP_MUTE_NUM; ++i)
		if ((FDSP_MUTE_OFF < psPrm->abInMute[i]) ||
			(FDSP_MUTE_OFF < psPrm->abOutMute[i]))
			return MCDRV_ERROR_ARGUMENT;

	if (gsFdspInfo.dStatus == (UINT32)FDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	/* set Mute */
	bADIMute0 = gsFdspInfo.bADIMute0;
	bADIMute0 = CreateMuteReg(bADIMute0, psPrm->abInMute[0],
					(UINT8)(MCB_ADI01MTN | MCB_ADI00MTN));
	bADIMute0 = CreateMuteReg(bADIMute0, psPrm->abInMute[1],
					(UINT8)(MCB_ADI03MTN | MCB_ADI02MTN));
	bADIMute0 = CreateMuteReg(bADIMute0, psPrm->abInMute[2],
					(UINT8)(MCB_ADI05MTN | MCB_ADI04MTN));
	bADIMute0 = CreateMuteReg(bADIMute0, psPrm->abInMute[3],
					(UINT8)(MCB_ADI07MTN | MCB_ADI06MTN));
	bADIMute1 = gsFdspInfo.bADIMute1;
	bADIMute1 = CreateMuteReg(bADIMute1, psPrm->abInMute[4],
					(UINT8)(MCB_ADI09MTN | MCB_ADI08MTN));
	bADIMute1 = CreateMuteReg(bADIMute1, psPrm->abInMute[5],
					(UINT8)(MCB_ADI11MTN | MCB_ADI10MTN));
	bADIMute1 = CreateMuteReg(bADIMute1, psPrm->abInMute[6],
					(UINT8)(MCB_ADI13MTN | MCB_ADI12MTN));
	bADIMute1 = CreateMuteReg(bADIMute1, psPrm->abInMute[7],
					(UINT8)(MCB_ADI15MTN | MCB_ADI14MTN));

	bADOMute0 = gsFdspInfo.bADOMute0;
	bADOMute0 = CreateMuteReg(bADOMute0, psPrm->abOutMute[0],
					(UINT8)(MCB_ADO01MTN | MCB_ADO00MTN));
	bADOMute0 = CreateMuteReg(bADOMute0, psPrm->abOutMute[1],
					(UINT8)(MCB_ADO03MTN | MCB_ADO02MTN));
	bADOMute0 = CreateMuteReg(bADOMute0, psPrm->abOutMute[2],
					(UINT8)(MCB_ADO05MTN | MCB_ADO04MTN));
	bADOMute0 = CreateMuteReg(bADOMute0, psPrm->abOutMute[3],
					(UINT8)(MCB_ADO07MTN | MCB_ADO06MTN));
	bADOMute1 = gsFdspInfo.bADOMute1;
	bADOMute1 = CreateMuteReg(bADOMute1, psPrm->abOutMute[4],
					(UINT8)(MCB_ADO09MTN | MCB_ADO08MTN));
	bADOMute1 = CreateMuteReg(bADOMute1, psPrm->abOutMute[5],
					(UINT8)(MCB_ADO11MTN | MCB_ADO10MTN));
	bADOMute1 = CreateMuteReg(bADOMute1, psPrm->abOutMute[6],
					(UINT8)(MCB_ADO13MTN | MCB_ADO12MTN));
	bADOMute1 = CreateMuteReg(bADOMute1, psPrm->abOutMute[7],
					(UINT8)(MCB_ADO15MTN | MCB_ADO14MTN));

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE0),
						bADIMute0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE1),
						bADIMute1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADIMUTE2),
						MCB_ADIMTSET);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE0),
						bADOMute0);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE1),
						bADOMute1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_F
						| (UINT32)MCI_ADOMUTE2),
						MCB_ADOMTSET);

	McDevIf_ExecutePacket();

	gsFdspInfo.bADIMute0 = bADIMute0;
	gsFdspInfo.bADIMute1 = bADIMute1;
	gsFdspInfo.bADOMute0 = bADOMute0;
	gsFdspInfo.bADOMute1 = bADOMute1;

	return MCDRV_SUCCESS;
}

