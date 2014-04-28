/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcparser.c
 *
 *	Description	: MC Driver parse control
 *
 *	Version		: 1.0.4	2013.01.17
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
#include "mcparser.h"
#include "mcmachdep.h"
#include "mcdevprof.h"
#include "mcbdspdrv.h"
#include "mccdspdrv.h"
#include "mcedspdrv.h"
#include "mcfdspdrv.h"

#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif




#define AEC_MIN_BYTES			(12)
#define	AEC_REVISION			(5)
#define	AEC_TARGET			(253)
#define AEC_D7_MIN_BYTES		(6)
#define	AEC_D7_CHUNK_CONFIG		(0x01000000)
#define	AEC_D7_CHUNK_AE			(0x02000000)
#define	AEC_D7_CHUNK_BDSP_ES1		(0x00010000)
#define	AEC_D7_CHUNK_BDSP		(0x00010001)
#define	AEC_D7_CHUNK_FDSP_ES1		(0x00020000)
#define	AEC_D7_CHUNK_FDSP		(0x00020001)
#define	AEC_D7_CHUNK_VBOX		(0x03000000)
#define	AEC_D7_CHUNK_CDSPA		(0x00030000)
#define	AEC_D7_CHUNK_CDSPB		(0x00030001)
#define	AEC_D7_CHUNK_CDSP_DEBUG		(0x00050000)
#define	AEC_D7_CHUNK_OUTPUT0_ES1	(0x04000000)
#define	AEC_D7_CHUNK_OUTPUT1_ES1	(0x04000001)
#define	AEC_D7_CHUNK_OUTPUT0		(0x04000002)
#define	AEC_D7_CHUNK_OUTPUT1		(0x04000003)
#define	AEC_D7_CHUNK_SYSEQ_EX		(0x00060000)
#define	AEC_D7_CHUNK_INPUT0		(0x05000000)
#define	AEC_D7_CHUNK_INPUT1		(0x05000001)
#define	AEC_D7_CHUNK_INPUT2		(0x05000002)
#define	AEC_D7_CHUNK_PDM		(0x06000000)
#define	AEC_D7_CHUNK_E2			(0x07000000)
#define	AEC_D7_CHUNK_E2_CONFIG		(0x00040000)
#define	AEC_D7_CHUNK_ADJ		(0x08000000)
#define	AEC_D7_CHUNK_EDSP_MISC		(0x09000000)
#define	AEC_D7_CHUNK_CONTROL		(0x0A000000)


static SINT32	AnalyzeAESubChunk(
				const UINT8 *pbPrm,
				UINT32 *pdPos,
				UINT32 *pdSubChunkSize,
				struct MCDRV_AEC_INFO *psAECInfo);
static SINT32	AnalyzeVBoxSubChunk(
				const UINT8 *pbPrm,
				UINT32 *pdPos,
				UINT32 *pdSubChunkSize,
				struct MCDRV_AEC_INFO *psAECInfo);
static SINT32	AnalyzeSysEqExSubChunk(
				const UINT8 *pbPrm,
				UINT32 *pdPos,
				UINT32 *pdSubChunkSize,
				struct MCDRV_AEC_SYSEQ_EX *psSysEqEx);
static SINT32	AnalyzeEDSPSubChunk(
				const UINT8 *pbPrm,
				UINT32 *pdPos,
				UINT32 *pdSubChunkSize,
				struct MCDRV_AEC_INFO *psAECInfo);

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
static UINT32	CreateUINT32(UINT8 b0, UINT8 b1, UINT8 b2, UINT8 b3)
{
	return ((UINT32)b0 << 24) | ((UINT32)b1 << 16)
					| ((UINT32)b2 << 8) | (UINT32)b3;
}

/****************************************************************************
 *	McParser_GetD7Chunk
 *
 *	Description:
 *			D7 chunk acquisition
 *	Arguments:
 *			pbPrm		AEC data pointer
 *			dSize		data size
 *			psD7Info	MCDRV_AEC_D7_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32	McParser_GetD7Chunk(
	const UINT8	*pbPrm,
	UINT32	dSize,
	struct MCDRV_AEC_D7_INFO	*psD7Info
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dDataSize;
	UINT32	dChunkSize;
	UINT32	dPos;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("");
#endif
	if (dSize < (UINT32)AEC_MIN_BYTES) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* header */
	if ((pbPrm[0] != 0x41)
	|| (pbPrm[1] != 0x45)
	|| (pbPrm[2] != 0x43)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* revision */
	if (pbPrm[3] != AEC_REVISION) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* size */
	dDataSize	= CreateUINT32(pbPrm[4], pbPrm[5], pbPrm[6], pbPrm[7]);
	if (dSize != (dDataSize + 8)) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* target */
	if (pbPrm[9] != AEC_TARGET) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* Reserved */
	if (pbPrm[11] != 0x00) {
		sdRet	= MCDRV_ERROR_ARGUMENT;
		goto exit;
	}

	/* D7 Chunk Search */
	psD7Info->pbChunkTop	= NULL;
	psD7Info->dChunkSize	= 0;
	dPos	= AEC_MIN_BYTES;

	while ((dPos + AEC_D7_MIN_BYTES) < dSize) {
		if ((pbPrm[dPos + 0] == 0x44)
		&& (pbPrm[dPos + 1] == 0x37)) {
			if (psD7Info->pbChunkTop != NULL) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}

			dChunkSize	= CreateUINT32(pbPrm[dPos + 2],
							pbPrm[dPos + 3],
							pbPrm[dPos + 4],
							pbPrm[dPos + 5]);
			if ((dPos + AEC_D7_MIN_BYTES + dChunkSize) > dSize) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psD7Info->pbChunkTop
				= &pbPrm[dPos + AEC_D7_MIN_BYTES];
			psD7Info->dChunkSize	= dChunkSize;
		} else {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}

		/* Next Chunk */
		dPos += AEC_D7_MIN_BYTES;
		dPos += dChunkSize;
	}

	if (psD7Info->pbChunkTop == NULL)
		sdRet	= MCDRV_ERROR_ARGUMENT;

exit:
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McParser_GetD7Chunk", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	McParser_AnalyzeD7Chunk
 *
 *	Description:
 *			D7 chunk analysis
 *	Arguments:
 *			psD7Info	MCDRV_AEC_D7_INFO structure pointer
 *			psAECInfo	MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32	McParser_AnalyzeD7Chunk(
	struct MCDRV_AEC_D7_INFO	*psD7Info,
	struct MCDRV_AEC_INFO		*psAECInfo
)
{
	const UINT8	*pbPrm;
	UINT32	dChunkEnd;
	UINT32	dPos;
	UINT32	dChunkId;
	UINT32	dSubChunkSize;
	SINT32	sdRet	= MCDRV_SUCCESS;
	int	n;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McParser_AnalyzeD7Chunk");
#endif
	pbPrm		= psD7Info->pbChunkTop;
	dChunkEnd	= psD7Info->dChunkSize-1;

	psAECInfo->sAecConfig.bFDspLocate		= 0xFF;
	psAECInfo->sAecAudioengine.bEnable		= 0;
	psAECInfo->sAecAudioengine.sAecBDsp.pbChunkData	= NULL;
	psAECInfo->sAecAudioengine.sAecBDsp.dwSize	= 0;
	psAECInfo->sAecAudioengine.sAecFDsp.pbChunkData	= NULL;
	psAECInfo->sAecAudioengine.sAecFDsp.dwSize	= 0;
	psAECInfo->sAecVBox.bEnable			= 0;
	psAECInfo->sAecVBox.sAecCDspA.pbChunkData	= NULL;
	psAECInfo->sAecVBox.sAecCDspA.dwSize		= 0;
	psAECInfo->sAecVBox.sAecCDspB.pbChunkData	= NULL;
	psAECInfo->sAecVBox.sAecCDspB.dwSize		= 0;
	psAECInfo->sAecVBox.sAecFDsp.pbChunkData	= NULL;
	psAECInfo->sAecVBox.sAecFDsp.dwSize		= 0;
	psAECInfo->sAecVBox.sAecCDspDbg.bJtagOn		= 0xFF;
	psAECInfo->sOutput.bLpf_Pre_Thru[0]		= 0xFF;
	psAECInfo->sOutput.bLpf_Pre_Thru[1]		= 0xFF;
	psAECInfo->sInput.bDsf32_L_Type[0]		= 0xFF;
	psAECInfo->sInput.bDsf32_L_Type[1]		= 0xFF;
	psAECInfo->sInput.bDsf32_L_Type[2]		= 0xFF;
	psAECInfo->sPdm.bMode				= 0xFF;
	psAECInfo->sE2.bEnable				= 0;
	psAECInfo->sE2.sE2Config.pbChunkData		= NULL;
	psAECInfo->sE2.sE2Config.dwSize			= 0;
	psAECInfo->sAdj.bHold				= 0xFF;
	psAECInfo->sEDspMisc.bI2SOut_Enb		= 0xFF;
	psAECInfo->sControl.bCommand			= 0xFF;

	dPos	= 0UL;
	while (dPos <= dChunkEnd && sdRet == MCDRV_SUCCESS) {
		if ((dPos + 8-1) > dChunkEnd) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}

		dChunkId	= CreateUINT32(pbPrm[dPos],
						pbPrm[dPos+1],
						pbPrm[dPos+2],
						pbPrm[dPos+3]);
		dSubChunkSize	= CreateUINT32(pbPrm[dPos+4],
						pbPrm[dPos+5],
						pbPrm[dPos+6],
						pbPrm[dPos+7]);

		dPos	+= 8;
		if ((dPos + dSubChunkSize-1) > dChunkEnd) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}

		switch (dChunkId) {
		case	AEC_D7_CHUNK_CONFIG:
			if (dSubChunkSize != 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sAecConfig.bFDspLocate != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecConfig.bFDspLocate	= pbPrm[dPos++];
			dSubChunkSize	= 0;
			break;
		case	AEC_D7_CHUNK_AE:
			if (dSubChunkSize < 8) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sAecAudioengine.bEnable != 0) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bEnable	= 1;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bAEOnOff	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bFDspOnOff	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bBDspAE0Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bBDspAE1Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bMixerIn0Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bMixerIn1Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bMixerIn2Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecAudioengine.bMixerIn3Src	= pbPrm[dPos++];
			dSubChunkSize--;
			if (dSubChunkSize > 0) {
				sdRet	= AnalyzeAESubChunk(pbPrm,
								&dPos,
								&dSubChunkSize,
								psAECInfo);
			}
			break;
		case	AEC_D7_CHUNK_VBOX:
			if (dSubChunkSize < 15) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sAecVBox.bEnable != 0) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bEnable	= 1;
			if (pbPrm[dPos] > 2) {
				if (pbPrm[dPos] != 0x11) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sAecVBox.bCDspFuncAOnOff	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				if (pbPrm[dPos] != 0x11) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sAecVBox.bCDspFuncBOnOff	= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bFDspOnOff		= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 0x3F)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bFdsp_Po_Source	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 1)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bISrc2_VSource	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 1)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bISrc2_Ch1_VSource	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 1)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bISrc3_VSource	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 1)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bLPt2_VSource	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 3)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bLPt2_Mix_VolO	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 3)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bLPt2_Mix_VolI	= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 3)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bSrc3_Ctrl		= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] == 3)
			|| (pbPrm[dPos] == 7)
			|| (pbPrm[dPos] == 11)
			|| ((pbPrm[dPos] > 13) && (pbPrm[dPos] < 0xFF))) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bSrc2_Fs		= pbPrm[dPos++];
			dSubChunkSize--;
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if ((pbPrm[dPos] > 1)
				&& (pbPrm[dPos] < 0xFF)) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sAecVBox.bSrc2_Thru	= pbPrm[dPos++];
			} else {
				dPos++;
			}
			dSubChunkSize--;
			if ((pbPrm[dPos] == 3)
			|| (pbPrm[dPos] == 7)
			|| (pbPrm[dPos] == 11)
			|| ((pbPrm[dPos] > 13) && (pbPrm[dPos] < 0xFF))) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bSrc3_Fs		= pbPrm[dPos++];
			dSubChunkSize--;
			if ((pbPrm[dPos] > 1)
			&& (pbPrm[dPos] < 0xFF)) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.bSrc3_Thru		= pbPrm[dPos++];
			dSubChunkSize--;
			if (dSubChunkSize > 0) {
				sdRet	= AnalyzeVBoxSubChunk(pbPrm,
								&dPos,
								&dSubChunkSize,
								psAECInfo);
			}
			break;

		case	AEC_D7_CHUNK_OUTPUT0_ES1:
		case	AEC_D7_CHUNK_OUTPUT1_ES1:
		case	AEC_D7_CHUNK_OUTPUT0:
		case	AEC_D7_CHUNK_OUTPUT1:
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if ((dChunkId != AEC_D7_CHUNK_OUTPUT0_ES1)
				&& (dChunkId != AEC_D7_CHUNK_OUTPUT1_ES1)) {
					;
					break;
				}
				n	= dChunkId - AEC_D7_CHUNK_OUTPUT0_ES1;
			} else {
				if ((dChunkId != AEC_D7_CHUNK_OUTPUT0)
				&& (dChunkId != AEC_D7_CHUNK_OUTPUT1)) {
					;
					break;
				}
				n	= dChunkId - AEC_D7_CHUNK_OUTPUT0;
			}
			if (dSubChunkSize < 47) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sOutput.bLpf_Pre_Thru[n] != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bLpf_Pre_Thru[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bLpf_Post_Thru[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDcc_Sel[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 5) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bSig_Det_Lvl		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bPow_Det_Lvl[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bOsf_Sel[n]		= pbPrm[dPos++];
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			} else {
				if (pbPrm[dPos] > 7) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sOutput.bSys_Eq_Enb[n]	= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A0[n][0]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A0[n][1]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A0[n][2]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A1[n][0]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A1[n][1]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A1[n][2]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A2[n][0]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A2[n][1]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_A2[n][2]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B1[n][0]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B1[n][1]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B1[n][2]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B2[n][0]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B2[n][1]
								= pbPrm[dPos++];
			psAECInfo->sOutput.bSys_Eq_Coef_B2[n][2]
								= pbPrm[dPos++];
			if (pbPrm[dPos] > 7) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bClip_Md[n]		= pbPrm[dPos++];
			psAECInfo->sOutput.bClip_Att[n]		= pbPrm[dPos++];
			psAECInfo->sOutput.bClip_Rel[n]		= pbPrm[dPos++];
			psAECInfo->sOutput.bClip_G[n]		= pbPrm[dPos++];
			psAECInfo->sOutput.bOsf_Gain[n][0]	= pbPrm[dPos++];
			psAECInfo->sOutput.bOsf_Gain[n][1]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDcl_OnOff[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDcl_Gain[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 0x7F) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDcl_Limit[n][0]	= pbPrm[dPos++];
			psAECInfo->sOutput.bDcl_Limit[n][1]	= pbPrm[dPos++];
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sOutput.bRandom_Dither_OnOff[n]
								= pbPrm[dPos++];
			} else {
				dPos++;
			}
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if (pbPrm[dPos] > 3) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sOutput.bRandom_Dither_Level[n]
								= pbPrm[dPos++];
			} else {
				dPos++;
			}
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sOutput.bRandom_Dither_POS[n]
								= pbPrm[dPos++];
			} else {
				dPos++;
			}
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDc_Dither_OnOff[n]	= pbPrm[dPos++];
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if (pbPrm[dPos] > 15) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sOutput.bDc_Dither_Level[n]
								= pbPrm[dPos++];
			} else {
				dPos++;
			}
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDither_Type[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDng_On[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 31) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDng_Zero[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDng_Time[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sOutput.bDng_Fw[n]		= pbPrm[dPos++];
			psAECInfo->sOutput.bDng_Attack		= pbPrm[dPos++];
			psAECInfo->sOutput.bDng_Release		= pbPrm[dPos++];
			psAECInfo->sOutput.bDng_Target[n]	= pbPrm[dPos++];
			psAECInfo->sOutput.bDng_Target_LineOut[n]
								= pbPrm[dPos++];
			if (n == 0)
				psAECInfo->sOutput.bDng_Target_Rc
								= pbPrm[dPos++];
			else
				dPos++;
			dSubChunkSize	-= 47;
			if (dSubChunkSize > 0) {
				sdRet	= AnalyzeSysEqExSubChunk(pbPrm,
					&dPos,
					&dSubChunkSize,
					&psAECInfo->sOutput.sSysEqEx[n]);
			}
			break;

		case	AEC_D7_CHUNK_INPUT0:
		case	AEC_D7_CHUNK_INPUT1:
		case	AEC_D7_CHUNK_INPUT2:
			n	= dChunkId - AEC_D7_CHUNK_INPUT0;
			if (dSubChunkSize < 16) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sInput.bDsf32_L_Type[n] != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (n == 0) {
				if (pbPrm[dPos] > 2) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			} else {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sInput.bDsf32_L_Type[n]	= pbPrm[dPos++];
			if (n == 0) {
				if (pbPrm[dPos] > 2) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			} else {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sInput.bDsf32_R_Type[n]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDsf4_Sel[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDcc_Sel[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDng_On[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDng_Att[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDng_Rel[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDng_Fw[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 0x3F) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDng_Tim[n]		= pbPrm[dPos++];
			psAECInfo->sInput.bDng_Zero[n][0]	= pbPrm[dPos++];
			psAECInfo->sInput.bDng_Zero[n][1]	= pbPrm[dPos++];
			psAECInfo->sInput.bDng_Tgt[n][0]	= pbPrm[dPos++];
			psAECInfo->sInput.bDng_Tgt[n][1]	= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDepop_Att[n]		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sInput.bDepop_Wait[n]	= pbPrm[dPos++];
			if (n == 2) {
				if (pbPrm[dPos] > 1) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sInput.bRef_Sel	= pbPrm[dPos++];
			} else {
				dPos++;
			}
			dSubChunkSize	= 0;
			break;

		case	AEC_D7_CHUNK_PDM:
			if (dSubChunkSize != 10) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sPdm.bMode != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bMode			= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bStWait			= pbPrm[dPos++];
			if (pbPrm[dPos] > 7) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm0_LoadTim		= pbPrm[dPos++];
			if (pbPrm[dPos] > 63) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm0_LFineDly		= pbPrm[dPos++];
			if (pbPrm[dPos] > 63) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm0_RFineDly		= pbPrm[dPos++];
			if (pbPrm[dPos] > 7) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm1_LoadTim		= pbPrm[dPos++];
			if (pbPrm[dPos] > 63) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm1_LFineDly		= pbPrm[dPos++];
			if (pbPrm[dPos] > 63) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm1_RFineDly		= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm0_Data_Delay	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sPdm.bPdm1_Data_Delay	= pbPrm[dPos++];
			dSubChunkSize	= 0;
			break;

		case	AEC_D7_CHUNK_E2:
			if (dSubChunkSize < 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sE2.bEnable != 0) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sE2.bEnable	= 1;
			if (pbPrm[dPos] > 4) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sE2.bE2_Da_Sel		= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 8) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sE2.bE2_Ad_Sel		= pbPrm[dPos++];
			dSubChunkSize--;
			if (pbPrm[dPos] > 2) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sE2.bE2OnOff			= pbPrm[dPos++];
			dSubChunkSize--;
			if (dSubChunkSize > 0) {
				sdRet	= AnalyzeEDSPSubChunk(pbPrm,
								&dPos,
								&dSubChunkSize,
								psAECInfo);
			}
			break;

		case	AEC_D7_CHUNK_ADJ:
			if (dSubChunkSize != 4) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sAdj.bHold != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[dPos] > 0x3F) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAdj.bHold		= pbPrm[dPos++];
			if (pbPrm[dPos] > 0x0F) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAdj.bCnt		= pbPrm[dPos++];
			psAECInfo->sAdj.bMax[0]		= pbPrm[dPos++];
			psAECInfo->sAdj.bMax[1]		= pbPrm[dPos++];
			dSubChunkSize	= 0;
			break;

		case	AEC_D7_CHUNK_EDSP_MISC:
			if (dSubChunkSize != 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sEDspMisc.bI2SOut_Enb != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sEDspMisc.bI2SOut_Enb	= pbPrm[dPos++];
			if (pbPrm[dPos] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sEDspMisc.bChSel		= pbPrm[dPos++];
			if (pbPrm[dPos] > 3) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sEDspMisc.bLoopBack		= pbPrm[dPos++];
			dSubChunkSize	= 0;
			break;

		case	AEC_D7_CHUNK_CONTROL:
			if (dSubChunkSize != 5) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sControl.bCommand != 0xFF) {
				/*	already done	*/
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H) {
				if ((pbPrm[dPos] < 1)
				|| (pbPrm[dPos] > 2)) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			} else {
				if ((pbPrm[dPos] < 1)
				|| (pbPrm[dPos] > 5)) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
			}
			psAECInfo->sControl.bCommand		= pbPrm[dPos++];
			psAECInfo->sControl.bParam[0]		= pbPrm[dPos++];
			psAECInfo->sControl.bParam[1]		= pbPrm[dPos++];
			psAECInfo->sControl.bParam[2]		= pbPrm[dPos++];
			psAECInfo->sControl.bParam[3]		= pbPrm[dPos++];
			dSubChunkSize	= 0;
			break;

		default:
			/* unknown */
			break;
		}
		dPos += dSubChunkSize;
	}


#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McParser_AnalyzeD7Chunk", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AnalyzeAESubChunk
 *
 *	Description:
 *			AudioEngine Sub chunk analysis
 *	Arguments:
 *			pbPrm		AEC data pointer
 *			pdPos		AEC sub chunk start pos
 *			pdSubChunkSize	AEC sub chunk byte size
 *			psAECInfo	MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	AnalyzeAESubChunk
(
	const UINT8	*pbPrm,
	UINT32		*pdPos,
	UINT32		*pdSubChunkSize,
	struct MCDRV_AEC_INFO	*psAECInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dChunkId;
	UINT32	dChunkSize;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AnalyzeAESubChunk");
#endif

	while (*pdSubChunkSize > 0) {
		if (*pdSubChunkSize < 8) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		dChunkId	= CreateUINT32(pbPrm[*pdPos],
						pbPrm[*pdPos+1],
						pbPrm[*pdPos+2],
						pbPrm[*pdPos+3]);
		dChunkSize	= CreateUINT32(pbPrm[*pdPos+4],
						pbPrm[*pdPos+5],
						pbPrm[*pdPos+6],
						pbPrm[*pdPos+7]);
		*pdSubChunkSize	-= 8;
		if (*pdSubChunkSize < dChunkSize) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		if ((dChunkId == AEC_D7_CHUNK_BDSP_ES1)
		|| (dChunkId == AEC_D7_CHUNK_BDSP)) {
			if (((dChunkId == AEC_D7_CHUNK_BDSP_ES1)
			&& (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H))
			|| ((dChunkId == AEC_D7_CHUNK_BDSP)
			&& (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H))) {
				if (
				psAECInfo->sAecAudioengine.sAecBDsp.pbChunkData
					!= NULL) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sAecAudioengine.sAecBDsp.pbChunkData
					= (UINT8 *)&pbPrm[*pdPos];
				psAECInfo->sAecAudioengine.sAecBDsp.dwSize
					= dChunkSize;
			}
		} else if ((dChunkId == AEC_D7_CHUNK_FDSP_ES1)
		|| (dChunkId == AEC_D7_CHUNK_FDSP)) {
			if (((dChunkId == AEC_D7_CHUNK_FDSP_ES1)
			&& (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H))
			|| ((dChunkId == AEC_D7_CHUNK_FDSP)
			&& (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H))) {
				if (
				psAECInfo->sAecAudioengine.sAecFDsp.pbChunkData
					!= NULL) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sAecAudioengine.sAecFDsp.pbChunkData
					= (UINT8 *)&pbPrm[*pdPos];
				psAECInfo->sAecAudioengine.sAecFDsp.dwSize
					= dChunkSize;
			}
		} else {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		*pdPos		+= (dChunkSize+8);
		*pdSubChunkSize	-= dChunkSize;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AnalyzeAESubChunk", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AnalyzeVBoxSubChunk
 *
 *	Description:
 *			V-Box Sub chunk analysis
 *	Arguments:
 *			pbPrm		AEC data pointer
 *			pdPos		AEC sub chunk start pos
 *			pdSubChunkSize	AEC sub chunk byte size
 *			psAECInfo	MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	AnalyzeVBoxSubChunk(
	const UINT8	*pbPrm,
	UINT32	*pdPos,
	UINT32	*pdSubChunkSize,
	struct MCDRV_AEC_INFO	*psAECInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dChunkId;
	UINT32	dChunkSize;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AnalyzeVBoxSubChunk");
#endif

	while (*pdSubChunkSize > 0) {
		if (*pdSubChunkSize < 8) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		dChunkId	= CreateUINT32(pbPrm[*pdPos],
						pbPrm[*pdPos+1],
						pbPrm[*pdPos+2],
						pbPrm[*pdPos+3]);
		dChunkSize	= CreateUINT32(pbPrm[*pdPos+4],
						pbPrm[*pdPos+5],
						pbPrm[*pdPos+6],
						pbPrm[*pdPos+7]);
		*pdSubChunkSize	-= 8;
		if (*pdSubChunkSize < dChunkSize) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		if (dChunkId == AEC_D7_CHUNK_CDSPA) {
			if (psAECInfo->sAecVBox.sAecCDspA.pbChunkData
				!= NULL) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.sAecCDspA.pbChunkData
				= (UINT8 *)&pbPrm[*pdPos];
			psAECInfo->sAecVBox.sAecCDspA.dwSize
				= dChunkSize;
		} else if (dChunkId == AEC_D7_CHUNK_CDSPB) {
			if (psAECInfo->sAecVBox.sAecCDspB.pbChunkData
				!= NULL) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.sAecCDspB.pbChunkData
				= (UINT8 *)&pbPrm[*pdPos];
			psAECInfo->sAecVBox.sAecCDspB.dwSize
				= dChunkSize;
		} else if ((dChunkId == AEC_D7_CHUNK_FDSP_ES1)
		|| (dChunkId == AEC_D7_CHUNK_FDSP)) {
			if (((dChunkId == AEC_D7_CHUNK_FDSP_ES1)
			&& (McDevProf_GetDevId() == eMCDRV_DEV_ID_80_90H))
			|| ((dChunkId == AEC_D7_CHUNK_FDSP)
			&& (McDevProf_GetDevId() != eMCDRV_DEV_ID_80_90H))) {
				if (psAECInfo->sAecVBox.sAecFDsp.pbChunkData
					!= NULL) {
					sdRet	= MCDRV_ERROR_ARGUMENT;
					break;
				}
				psAECInfo->sAecVBox.sAecFDsp.pbChunkData
					= (UINT8 *)&pbPrm[*pdPos];
				psAECInfo->sAecVBox.sAecFDsp.dwSize
					= dChunkSize;
			}
		} else if (dChunkId == AEC_D7_CHUNK_CDSP_DEBUG) {
			if (dChunkSize != 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psAECInfo->sAecVBox.sAecCDspDbg.bJtagOn
				!= 0xFF) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (pbPrm[*pdPos+8] > 1) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sAecVBox.sAecCDspDbg.bJtagOn
				= pbPrm[*pdPos+8];
		} else {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		*pdPos		+= (dChunkSize+8);
		*pdSubChunkSize	-= dChunkSize;
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AnalyzeVBoxSubChunk", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AnalyzeSysEqExSubChunk
 *
 *	Description:
 *			SYS EQ EX Sub chunk analysis
 *	Arguments:
 *			pbPrm		AEC data pointer
 *			pdPos		AEC sub chunk start pos
 *			pdSubChunkSize	AEC sub chunk byte size
 *			psSysEqEx	MCDRV_AEC_SYSEQ_EX structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	AnalyzeSysEqExSubChunk
(
	const UINT8	*pbPrm,
	UINT32		*pdPos,
	UINT32		*pdSubChunkSize,
	struct MCDRV_AEC_SYSEQ_EX	*psSysEqEx
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dChunkId;
	UINT32	dChunkSize;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AnalyzeSysEqExSubChunk");
#endif

	while (*pdSubChunkSize > 0) {
		if (*pdSubChunkSize < 8) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		dChunkId	= CreateUINT32(pbPrm[*pdPos],
						pbPrm[*pdPos+1],
						pbPrm[*pdPos+2],
						pbPrm[*pdPos+3]);
		dChunkSize	= CreateUINT32(pbPrm[*pdPos+4],
						pbPrm[*pdPos+5],
						pbPrm[*pdPos+6],
						pbPrm[*pdPos+7]);
		*pdPos	+= 8;
		*pdSubChunkSize	-= 8;
		if (*pdSubChunkSize < dChunkSize) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}

		switch (dChunkId) {
		case	AEC_D7_CHUNK_SYSEQ_EX:
			if (dChunkSize != 30) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			if (psSysEqEx->bEnable	!= 0) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psSysEqEx->bEnable	= 1;
			psSysEqEx->sBand[0].bCoef_A0[0]	= pbPrm[*pdPos+0];
			psSysEqEx->sBand[0].bCoef_A0[1]	= pbPrm[*pdPos+1];
			psSysEqEx->sBand[0].bCoef_A0[2]	= pbPrm[*pdPos+2];
			psSysEqEx->sBand[0].bCoef_A1[0]	= pbPrm[*pdPos+3];
			psSysEqEx->sBand[0].bCoef_A1[1]	= pbPrm[*pdPos+4];
			psSysEqEx->sBand[0].bCoef_A1[2]	= pbPrm[*pdPos+5];
			psSysEqEx->sBand[0].bCoef_A2[0]	= pbPrm[*pdPos+6];
			psSysEqEx->sBand[0].bCoef_A2[1]	= pbPrm[*pdPos+7];
			psSysEqEx->sBand[0].bCoef_A2[2]	= pbPrm[*pdPos+8];
			psSysEqEx->sBand[0].bCoef_B1[0]	= pbPrm[*pdPos+9];
			psSysEqEx->sBand[0].bCoef_B1[1]	= pbPrm[*pdPos+10];
			psSysEqEx->sBand[0].bCoef_B1[2]	= pbPrm[*pdPos+11];
			psSysEqEx->sBand[0].bCoef_B2[0]	= pbPrm[*pdPos+12];
			psSysEqEx->sBand[0].bCoef_B2[1]	= pbPrm[*pdPos+13];
			psSysEqEx->sBand[0].bCoef_B2[2]	= pbPrm[*pdPos+14];
			psSysEqEx->sBand[1].bCoef_A0[0]	= pbPrm[*pdPos+15];
			psSysEqEx->sBand[1].bCoef_A0[1]	= pbPrm[*pdPos+16];
			psSysEqEx->sBand[1].bCoef_A0[2]	= pbPrm[*pdPos+17];
			psSysEqEx->sBand[1].bCoef_A1[0]	= pbPrm[*pdPos+18];
			psSysEqEx->sBand[1].bCoef_A1[1]	= pbPrm[*pdPos+19];
			psSysEqEx->sBand[1].bCoef_A1[2]	= pbPrm[*pdPos+20];
			psSysEqEx->sBand[1].bCoef_A2[0]	= pbPrm[*pdPos+21];
			psSysEqEx->sBand[1].bCoef_A2[1]	= pbPrm[*pdPos+22];
			psSysEqEx->sBand[1].bCoef_A2[2]	= pbPrm[*pdPos+23];
			psSysEqEx->sBand[1].bCoef_B1[0]	= pbPrm[*pdPos+24];
			psSysEqEx->sBand[1].bCoef_B1[1]	= pbPrm[*pdPos+25];
			psSysEqEx->sBand[1].bCoef_B1[2]	= pbPrm[*pdPos+26];
			psSysEqEx->sBand[1].bCoef_B2[0]	= pbPrm[*pdPos+27];
			psSysEqEx->sBand[1].bCoef_B2[1]	= pbPrm[*pdPos+28];
			psSysEqEx->sBand[1].bCoef_B2[2]	= pbPrm[*pdPos+29];
			*pdPos	+= 30;
			*pdSubChunkSize	-= dChunkSize;
			break;
		default:
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AnalyzeSysEqExSubChunk", &sdRet);
#endif
	return sdRet;
}

/****************************************************************************
 *	AnalyzeEDSPSubChunk
 *
 *	Description:
 *			E-DSP Sub chunk analysis
 *	Arguments:
 *			pbPrm		AEC data pointer
 *			pdPos		AEC sub chunk start pos
 *			pdSubChunkSize	AEC sub chunk byte size
 *			psAECInfo	MCDRV_AEC_INFO structure pointer
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	AnalyzeEDSPSubChunk
(
	const UINT8	*pbPrm,
	UINT32		*pdPos,
	UINT32		*pdSubChunkSize,
	struct MCDRV_AEC_INFO	*psAECInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dChunkId;
	UINT32	dChunkSize;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("AnalyzeEDSPSubChunk");
#endif

	while (*pdSubChunkSize > 0 && sdRet == MCDRV_SUCCESS) {
		if (*pdSubChunkSize < 8) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
		dChunkId	= CreateUINT32(pbPrm[*pdPos],
						pbPrm[*pdPos+1],
						pbPrm[*pdPos+2],
						pbPrm[*pdPos+3]);
		dChunkSize	= CreateUINT32(pbPrm[*pdPos+4],
						pbPrm[*pdPos+5],
						pbPrm[*pdPos+6],
						pbPrm[*pdPos+7]);
		*pdSubChunkSize	-= 8;
		if (*pdSubChunkSize < dChunkSize) {
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}

		switch (dChunkId) {
		case	AEC_D7_CHUNK_E2_CONFIG:
			if (psAECInfo->sE2.sE2Config.pbChunkData
				!= NULL) {
				sdRet	= MCDRV_ERROR_ARGUMENT;
				break;
			}
			psAECInfo->sE2.sE2Config.pbChunkData
				= (UINT8 *)&pbPrm[*pdPos];
			psAECInfo->sE2.sE2Config.dwSize
				= dChunkSize;
			*pdPos		+= (dChunkSize+8);
			*pdSubChunkSize	-= dChunkSize;
			break;
		default:
			sdRet	= MCDRV_ERROR_ARGUMENT;
			break;
		}
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("AnalyzeEDSPSubChunk", &sdRet);
#endif
	return sdRet;
}



