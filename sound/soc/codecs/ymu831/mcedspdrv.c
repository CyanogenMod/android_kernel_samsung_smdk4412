/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcedspdrv.c
 *
 *		Description	: MC Edsp Driver
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
#include "mcedspdrv.h"


/* inside definition */
#define EDSP_STATUS_IDLE		(0)
#define EDSP_STATUS_INITED		(1)

#define CHUNK_SIZE			(8)

#define AEC_EDSP_TAG_IMEM		(0x00002000)
#define AEC_EDSP_TAG_XMEM		(0x00002100)
#define AEC_EDSP_TAG_YMEM		(0x00002200)
#define AEC_EDSP_TAG_REG		(0x00002300)

#define MEM_FIX_SIZE			(4)

#define REG_UNIT_NUM			(9)
#define REG_FIX_SIZE			(4)
#define REG_COMMAND			(0)
#define REG_REQ_0			(1)
#define REG_REQ_7			(8)
#define REG_REQ_NUM			(8)
#define REG_STATUS			(0)
#define REG_RES_NUM			(8)

#define E1_COMMAND_RUN_MODE		(0x40)
#define E1_COMMAND_SLEEP_MODE		(0x00)
#define E1_COMMAND_OFFSET_START		(0x02)
#define E1_COMMAND_OFFSET_STOP		(0x00)
#define E1_COMMAND_IMPEDANCE_START	(0x81)
#define E1_COMMAND_IMPEDANCE_STOP	(0x80)
#define OFFSET_START			(1)
#define OFFSET_STOP			(0)
#define IMPEDANCE_START			(1)
#define IMPEDANCE_STOP			(0)

#define SYSEQ_NUM			(15)
#define BAND_NUM			(3)
#define SYSEQ_MODIFY_0			(1)
#define SYSEQ_MODIFY_1			(2)
#define SYSEQ_MODIFY_2			(4)
#define SYSEQ_NO_MODIFY			(0)

/* inside Typedef Struct */
struct MCDRV_EDSP_AEC_E_INFO {
	UINT8				bOnOff;
	UINT8				*pbChunkData;
	UINT32				dwSize;

	UINT8				*pbIMem;
	UINT32				dIMemSize;
	UINT8				*pbXMem;
	UINT32				dXMemSize;
	UINT8				*pbYMem;
	UINT32				dYMemSize;
	UINT8				*pbReg;
	UINT32				dCmdCount;
};

struct MCDRV_EDSP_AEC_EDSP_INFO {
	struct MCDRV_EDSP_AEC_E_INFO	sE2;
};

struct MCDRV_EDSP_INFO {
	UINT32				dStatus;
	SINT32(*cbfuncE2)(SINT32, UINT32, UINT32);
	UINT8				bSysEq0Modify;
	UINT8				abSysEq0[SYSEQ_NUM * BAND_NUM];
	UINT8				bSysEq1Modify;
	UINT8				abSysEq1[SYSEQ_NUM * BAND_NUM];
};

static struct MCDRV_EDSP_INFO gsEdspInfo = {
	EDSP_STATUS_IDLE,
	NULL,
	(SYSEQ_MODIFY_0 | SYSEQ_MODIFY_1 | SYSEQ_MODIFY_2),
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	},
	(SYSEQ_MODIFY_0 | SYSEQ_MODIFY_1 | SYSEQ_MODIFY_2),
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	}
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
 *	GetEDSPChunk
 *
 *	Function:
 *			Get E2 chunk
 *	Arguments:
 *			psPrm		Pointer of aec info
 *			psEdspInfo	Pointer of aec edsp info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void GetEDSPChunk(struct MCDRV_AEC_INFO *psPrm,
				struct MCDRV_EDSP_AEC_EDSP_INFO *psEdspInfo)
{
	psEdspInfo->sE2.bOnOff = EDSP_E_OFF;
	psEdspInfo->sE2.pbChunkData = NULL;
	psEdspInfo->sE2.dwSize = 0;

	if (psPrm->sE2.bEnable == AEC_EDSP_ENABLE) {
		psEdspInfo->sE2.bOnOff = psPrm->sE2.bE2OnOff;

		if (psEdspInfo->sE2.bOnOff == EDSP_E_OFF)
			return;

		psEdspInfo->sE2.pbChunkData =
					psPrm->sE2.sE2Config.pbChunkData;
		psEdspInfo->sE2.dwSize = psPrm->sE2.sE2Config.dwSize;
	}

	if (psEdspInfo->sE2.pbChunkData != NULL)
		psEdspInfo->sE2.pbChunkData =
			&psEdspInfo->sE2.pbChunkData[CHUNK_SIZE];

	return;
}

/***************************************************************************
 *	EdspChunkAnalyze
 *
 *	Function:
 *			Analyze e2 chunk
 *	Arguments:
 *			psEdspInfo	Pointer of aec info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32 EdspChunkAnalyze(struct MCDRV_EDSP_AEC_EDSP_INFO *psEdspInfo)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT8 *pbPrm;
	UINT32 dSize;
	UINT32 dTemp;
	struct MCDRV_EDSP_AEC_E_INFO *psEInfo;

	psEInfo = &psEdspInfo->sE2;
	if (psEInfo->bOnOff == EDSP_E_OFF)
		return MCDRV_SUCCESS;

	psEInfo->pbIMem = NULL;
	psEInfo->dIMemSize = 0;
	psEInfo->pbXMem = NULL;
	psEInfo->dXMemSize = 0;
	psEInfo->pbYMem = NULL;
	psEInfo->dYMemSize = 0;
	psEInfo->pbReg = NULL;
	psEInfo->dCmdCount = 0;

	if ((psEInfo->pbChunkData == NULL) || (psEInfo->dwSize == 0))
		return MCDRV_SUCCESS;

	pbPrm = psEInfo->pbChunkData;
	dSize = psEInfo->dwSize;

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
		case AEC_EDSP_TAG_IMEM:
			if (dSize < MEM_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);

			if (dTemp == 0UL)
				break;
			if ((dTemp % 3UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)MEM_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			if (psEInfo->pbIMem != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psEInfo->pbIMem = &pbPrm[dChunkTop + MEM_FIX_SIZE];
			psEInfo->dIMemSize = dTemp;
			break;

		case AEC_EDSP_TAG_XMEM:
			if (dSize < MEM_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);

			if (dTemp == 0UL)
				break;
			if ((dTemp & 0x07UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)MEM_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			if (psEInfo->pbXMem != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psEInfo->pbXMem = &pbPrm[dChunkTop + MEM_FIX_SIZE];
			psEInfo->dXMemSize = dTemp;
			break;

		case AEC_EDSP_TAG_YMEM:
			if (dSize < MEM_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);

			if (dTemp == 0UL)
				break;
			if ((dTemp % 3UL) != 0UL)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)MEM_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;

			if (psEInfo->pbYMem != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psEInfo->pbYMem = &pbPrm[dChunkTop + MEM_FIX_SIZE];
			psEInfo->dYMemSize = dTemp;
			break;

		case AEC_EDSP_TAG_REG:
			if (dSize < REG_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);

			if (dTemp == 0UL)
				break;
			if (dTemp != REG_UNIT_NUM)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)REG_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			if (psEInfo->pbReg != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psEInfo->pbReg = &pbPrm[dChunkTop + MEM_FIX_SIZE];
			psEInfo->dCmdCount = 1;
			break;

		default:
			break;
		}
		dChunkTop += dChunkSize;
	}

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	SetFirmware
 *
 *	Function:
 *			data download
 *	Arguments:
 *			psEdspInfo	Pointer of aec info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetFirmware(struct MCDRV_EDSP_AEC_EDSP_INFO *psEdspInfo)
{
	UINT8 bData;
	UINT32 i;
	struct MCDRV_EDSP_AEC_E_INFO *psEInfo;

	psEInfo = &psEdspInfo->sE2;
	if (psEInfo->bOnOff == EDSP_E_OFF)
		return;

	if ((psEInfo->pbIMem == NULL) &&
		(psEInfo->pbXMem == NULL) &&
		(psEInfo->pbYMem == NULL))
		return;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E2DSP),
						(UINT8)MCB_E2DSP_RST);

	/* E2IMEM */
	if (psEInfo->pbIMem != NULL) {
		bData = MCB_EDSP_FW_PAGE_E2IMEM;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)bData);

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_A),
						(UINT8)0x00);

		for (i = 0; i < psEInfo->dIMemSize; ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						psEInfo->pbIMem[i]);

		McDevIf_ExecutePacket();
	}

	/* E2YMEM */
	if (psEInfo->pbYMem != NULL) {
		bData = MCB_EDSP_FW_PAGE_E2YMEM;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)bData);

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_A),
						(UINT8)0x00);

		for (i = 0; i < psEInfo->dYMemSize; ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						psEInfo->pbYMem[i]);

		McDevIf_ExecutePacket();
	}


	/* E2XMEM */
	if (psEInfo->pbXMem != NULL) {
		bData = MCB_EDSP_FW_PAGE_E2XMEM;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)bData);

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_A),
						(UINT8)0x00);

		for (i = 0; i < psEInfo->dXMemSize; ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						psEInfo->pbXMem[i]);

		McDevIf_ExecutePacket();
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_PAGE),
						(UINT8)MCI_EDSP_FW_PAGE_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E2DSP),
						(UINT8)0x00);
	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	WriteCommandE2
 *
 *	Function:
 *			Write e2 command
 *	Arguments:
 *			pbPrm		Pointer of e2 command data
 *	Return:
 *			none
 *
 ****************************************************************************/
static void WriteCommandE2(UINT8 *pbPrm)
{
	UINT32 i;
	for (i = 0; i < REG_REQ_NUM; ++i)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| ((UINT32)MCI_E2REQ_0 + i)),
						(UINT8)pbPrm[REG_REQ_0 + i]);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E2COMMAND),
						(UINT8)pbPrm[REG_COMMAND]);
	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	SetCommand
 *
 *	Function:
 *			Set command
 *	Arguments:
 *			psEdspInfo	Pointer of aec info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetCommand(struct MCDRV_EDSP_AEC_EDSP_INFO *psEdspInfo)
{
	UINT8 *pData;

	if ((psEdspInfo->sE2.bOnOff != EDSP_E_OFF) &&
		(psEdspInfo->sE2.pbReg != NULL) &&
		(0 < psEdspInfo->sE2.dCmdCount)) {

		pData = psEdspInfo->sE2.pbReg;
		WriteCommandE2(&pData[0]);
	}
}

/***************************************************************************
 *	SetE
 *
 *	Function:
 *			Set EDSP
 *	Arguments:
 *			psEdspInfo	Pointer of aec info
 *	Return:
 *			none
 *
 ****************************************************************************/
static void SetE(struct MCDRV_EDSP_AEC_EDSP_INFO *psEdspInfo)
{
	SetFirmware(psEdspInfo);

	SetCommand(psEdspInfo);
}

/***************************************************************************
 *	EnableIrq
 *
 *	Function:
 *			Interrupt enable
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void EnableIrq(void)
{
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_EEDSP),
								&bData, 1);

	bData |= MCB_EE2DSP;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_EEDSP),
			(UINT8)bData);
}

/***************************************************************************
 *	DisableIrq
 *
 *	Function:
 *
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void DisableIrq(void)
{
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_EEDSP),
								&bData, 1);

	bData &= ~MCB_EE2DSP;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_EEDSP),
			(UINT8)bData);
}

/***************************************************************************
 *	E1Download1Band
 *
 *	Function:
 *			E1 SysEq data download
 *	Arguments:
 *			pbSysEq0	SysEq0 data
 *			pbSysEq1	SysEq1 data
 *			bSysEqEnbReg	E-DSP #9 register set
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 E1Download1Band(UINT8 *pbSysEq0, UINT8 *pbSysEq1, UINT8 bSysEqEnbReg)
{
	UINT8 bOrgSysEqEnbReg;
	UINT32 i;

	bOrgSysEqEnbReg = bSysEqEnbReg;
	if (pbSysEq0 != NULL) {
		for (i = 0; i < SYSEQ_NUM; ++i)
			if (gsEdspInfo.abSysEq0[i] != pbSysEq0[i]) {
				gsEdspInfo.abSysEq0[i] = pbSysEq0[i];
				gsEdspInfo.bSysEq0Modify = SYSEQ_MODIFY_0;
			}

		if (gsEdspInfo.bSysEq0Modify != SYSEQ_NO_MODIFY)
			bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ0_ENB;
		else
			pbSysEq0 = NULL;
		gsEdspInfo.bSysEq0Modify = SYSEQ_NO_MODIFY;
	}
	if (pbSysEq1 != NULL) {
		for (i = 0; i < SYSEQ_NUM; ++i)
			if (gsEdspInfo.abSysEq1[i] != pbSysEq1[i]) {
				gsEdspInfo.abSysEq1[i] = pbSysEq1[i];
				gsEdspInfo.bSysEq1Modify = SYSEQ_MODIFY_0;
			}

		if (gsEdspInfo.bSysEq1Modify != SYSEQ_NO_MODIFY)
			bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ1_ENB;
		else
			pbSysEq1 = NULL;
		gsEdspInfo.bSysEq1Modify = SYSEQ_NO_MODIFY;
	}

	if ((pbSysEq0 == NULL) && (pbSysEq1 == NULL))
		return MCDRV_SUCCESS;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_SYSEQ),
						(UINT8)bSysEqEnbReg);

	/* SYSEQ0 */
	if (pbSysEq0 != NULL) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ0);

		for (i = 0; i < SYSEQ_NUM; ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq0[i]);

		McDevIf_ExecutePacket();
	}

	/* E1YREG[SYSEQ1] */
	if (pbSysEq1 != NULL) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ1);

		for (i = 0; i < SYSEQ_NUM; ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq1[i]);

		McDevIf_ExecutePacket();
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_PAGE),
						(UINT8)MCI_EDSP_FW_PAGE_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_SYSEQ),
						(UINT8)bOrgSysEqEnbReg);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	E1Download3Band
 *
 *	Function:
 *			E1 SysEq data download
 *	Arguments:
 *			pbSysEq0	SysEq0 data
 *			pbSysEq1	SysEq1 data
 *			bSysEqEnbReg	E-DSP #9 register set
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 E1Download3Band(UINT8 *pbSysEq0, UINT8 *pbSysEq1, UINT8 bSysEqEnbReg)
{
	UINT8 bOrgSysEqEnbReg;
	UINT8 bSysEq0Modify;
	UINT8 bSysEq1Modify;
	UINT32 i;
	UINT32 j;
	UINT32 k;

	bOrgSysEqEnbReg = bSysEqEnbReg;
	bSysEq0Modify = SYSEQ_NO_MODIFY;
	bSysEq1Modify = SYSEQ_NO_MODIFY;
	if (pbSysEq0 != NULL) {
		for (i = 0, j = SYSEQ_NUM, k = (SYSEQ_NUM * 2);
			i < SYSEQ_NUM;
			++i, ++j, ++k) {
			if (gsEdspInfo.abSysEq0[i] != pbSysEq0[i]) {
				gsEdspInfo.abSysEq0[i] = pbSysEq0[i];
				gsEdspInfo.bSysEq0Modify |= SYSEQ_MODIFY_0;
			}
			if (gsEdspInfo.abSysEq0[j] != pbSysEq0[j]) {
				gsEdspInfo.abSysEq0[j] = pbSysEq0[j];
				gsEdspInfo.bSysEq0Modify |= SYSEQ_MODIFY_1;
			}
			if (gsEdspInfo.abSysEq0[k] != pbSysEq0[k]) {
				gsEdspInfo.abSysEq0[k] = pbSysEq0[k];
				gsEdspInfo.bSysEq0Modify |= SYSEQ_MODIFY_2;
			}
		}

		if (gsEdspInfo.bSysEq0Modify != SYSEQ_NO_MODIFY) {
			bSysEq0Modify = gsEdspInfo.bSysEq0Modify;
			if ((gsEdspInfo.bSysEq0Modify & SYSEQ_MODIFY_0) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ0_B0_ENB;
			if ((gsEdspInfo.bSysEq0Modify & SYSEQ_MODIFY_1) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ0_B1_ENB;
			if ((gsEdspInfo.bSysEq0Modify & SYSEQ_MODIFY_2) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ0_B2_ENB;
		} else
			pbSysEq0 = NULL;
		gsEdspInfo.bSysEq0Modify = SYSEQ_NO_MODIFY;
	}
	if (pbSysEq1 != NULL) {
		for (i = 0, j = SYSEQ_NUM, k = (SYSEQ_NUM * 2);
			i < SYSEQ_NUM;
			++i, ++j, ++k) {
			if (gsEdspInfo.abSysEq1[i] != pbSysEq1[i]) {
				gsEdspInfo.abSysEq1[i] = pbSysEq1[i];
				gsEdspInfo.bSysEq1Modify |= SYSEQ_MODIFY_0;
			}
			if (gsEdspInfo.abSysEq1[j] != pbSysEq1[j]) {
				gsEdspInfo.abSysEq1[j] = pbSysEq1[j];
				gsEdspInfo.bSysEq1Modify |= SYSEQ_MODIFY_1;
			}
			if (gsEdspInfo.abSysEq1[k] != pbSysEq1[k]) {
				gsEdspInfo.abSysEq1[k] = pbSysEq1[k];
				gsEdspInfo.bSysEq1Modify |= SYSEQ_MODIFY_2;
			}
		}

		if (gsEdspInfo.bSysEq1Modify != SYSEQ_NO_MODIFY) {
			bSysEq1Modify = gsEdspInfo.bSysEq1Modify;
			if ((gsEdspInfo.bSysEq1Modify & SYSEQ_MODIFY_0) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ1_B0_ENB;
			if ((gsEdspInfo.bSysEq1Modify & SYSEQ_MODIFY_1) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ1_B1_ENB;
			if ((gsEdspInfo.bSysEq1Modify & SYSEQ_MODIFY_2) != 0)
				bSysEqEnbReg &= ~MCB_SYSEQ_SYSEQ1_B2_ENB;
		} else
			pbSysEq1 = NULL;
		gsEdspInfo.bSysEq1Modify = SYSEQ_NO_MODIFY;
	}

	if ((pbSysEq0 == NULL) && (pbSysEq1 == NULL))
		return MCDRV_SUCCESS;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_SYSEQ),
						(UINT8)bSysEqEnbReg);

	/* SYSEQ0 */
	if (pbSysEq0 != NULL) {
		if ((bSysEq0Modify & SYSEQ_MODIFY_0) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ0_B0);

			for (i = 0; i < SYSEQ_NUM; ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq0[i]);
		}

		if ((bSysEq0Modify & SYSEQ_MODIFY_1) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ0_B1);

			for (i = SYSEQ_NUM; i < (SYSEQ_NUM * 2); ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq0[i]);
		}

		if ((bSysEq0Modify & SYSEQ_MODIFY_2) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ0_B2);

			for (i = (SYSEQ_NUM * 2); i < (SYSEQ_NUM * 3); ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq0[i]);
		}

		McDevIf_ExecutePacket();
	}

	/* E1YREG[SYSEQ1] */
	if (pbSysEq1 != NULL) {
		if ((bSysEq1Modify & SYSEQ_MODIFY_0) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ1_B0);

			for (i = 0; i < SYSEQ_NUM; ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq1[i]);
		}

		if ((bSysEq1Modify & SYSEQ_MODIFY_1) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ1_B1);

			for (i = SYSEQ_NUM; i < (SYSEQ_NUM * 2); ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq1[i]);
		}

		if ((bSysEq1Modify & SYSEQ_MODIFY_2) != 0) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP_FW_PAGE),
					(UINT8)MCB_EDSP_FW_PAGE_SYSEQ1_B2);

			for (i = (SYSEQ_NUM * 2); i < (SYSEQ_NUM * 3); ++i)
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_D),
						pbSysEq1[i]);
		}

		McDevIf_ExecutePacket();
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_EDSP_FW_PAGE),
						(UINT8)MCI_EDSP_FW_PAGE_DEF);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_SYSEQ),
						(UINT8)bOrgSysEqEnbReg);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_Init
 *
 *	Function:
 *			Init edsp
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_Init(void)
{
	UINT32 i;

	if (gsEdspInfo.dStatus != (UINT32)EDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	gsEdspInfo.cbfuncE2 = NULL;

	gsEdspInfo.dStatus = (UINT32)EDSP_STATUS_INITED;

	gsEdspInfo.bSysEq0Modify =
			(SYSEQ_MODIFY_0 | SYSEQ_MODIFY_1 | SYSEQ_MODIFY_2);
	gsEdspInfo.bSysEq1Modify =
			(SYSEQ_MODIFY_0 | SYSEQ_MODIFY_1 | SYSEQ_MODIFY_2);
	for (i = 0; i < (SYSEQ_NUM * BAND_NUM); ++i)
		gsEdspInfo.abSysEq0[i] = 0x00;
	for (i = 0; i < (SYSEQ_NUM * BAND_NUM); ++i)
		gsEdspInfo.abSysEq1[i] = 0x00;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_Term
 *
 *	Function:
 *			Terminate EDSP
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_Term(void)
{
	UINT8 bData;

	if (gsEdspInfo.dStatus == (UINT32)EDSP_STATUS_IDLE)
		return MCDRV_SUCCESS;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_EEDSP),
								&bData, 1);
	bData &= ~(MCB_EE2DSP_OV | MCB_EE2DSP);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EEDSP),
					bData);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_IF
			| (UINT32)MCI_EDSP),
			(UINT8)MCB_E2DSP_STA);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_E
					| (UINT32)MCI_E2DSP),
					MCI_E2DSP_DEF);

	McDevIf_ExecutePacket();

	gsEdspInfo.cbfuncE2 = NULL;
	gsEdspInfo.dStatus = (UINT32)EDSP_STATUS_IDLE;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_IrqProc
 *
 *	Function:
 *			Interrupt processing
 *	Arguments:
 *			none
 *	Return:
 *			0
 *
 ****************************************************************************/
SINT32 McEdsp_IrqProc(void)
{
	UINT8 bIReq;
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP),
					&bIReq, 1);

	bIReq &= MCB_E2DSP_STA;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_EDSP),
					bIReq);
	McDevIf_ExecutePacket();

	if ((bIReq & MCB_E2DSP_STA) != 0) {

		if (gsEdspInfo.cbfuncE2 != NULL) {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E2STATUS),
						&bData, 1);

			gsEdspInfo.cbfuncE2(0,
				EDSP_CB_CMD_STATUS,
				(UINT32)bData);
		}
	}

	return 0;
}

/***************************************************************************
 *	McEdsp_SetCBFunc
 *
 *	Function:
 *			Set a callback function
 *	Arguments:
 *			cbfunc		callback func
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_SetCBFunc(SINT32 (*cbfunc)(SINT32, UINT32, UINT32))
{
	gsEdspInfo.cbfuncE2 = cbfunc;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_GetDSP
 *
 *	Function:
 *			Get result
 *	Arguments:
 *			pbData		Pointer of data area
 *	Return:
 *			MCDRV_ERROR_ARGUMENT
 *			REG_RES_NUM
 *
 ****************************************************************************/
SINT32 McEdsp_GetDSP(UINT8 *pbData)
{
	UINT32 i;

	if (pbData == NULL)
		return MCDRV_ERROR_ARGUMENT;

	for (i = 0; i < REG_RES_NUM; ++i)
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_E
					| ((UINT32)MCI_E2RES_0 + i)),
					&pbData[i], 1);

	return REG_RES_NUM;
}

/***************************************************************************
 *	McEdsp_SetDSPCheck
 *
 *	Function:
 *			Check dsp configuration
 *	Arguments:
 *			psPrm		Pointer of aec info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32 McEdsp_SetDSPCheck(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_EDSP_AEC_EDSP_INFO sEdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetEDSPChunk(psPrm, &sEdspInfo);

	sdResult = EdspChunkAnalyze(&sEdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_SetDSP
 *
 *	Function:
 *			Set dsp
 *	Arguments:
 *			psPrm		Pointe of aec info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32 McEdsp_SetDSP(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_EDSP_AEC_EDSP_INFO sEdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetEDSPChunk(psPrm, &sEdspInfo);

	sdResult = EdspChunkAnalyze(&sEdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	if (psPrm->sE2.bEnable == AEC_EDSP_ENABLE) {
		if (sEdspInfo.sE2.bOnOff == EDSP_E_OFF) {
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E2DSP),
						(UINT8)MCB_E2DSP_RST);
			McDevIf_ExecutePacket();
		}
	}

	if ((sEdspInfo.sE2.pbChunkData == NULL) || (sEdspInfo.sE2.dwSize == 0))
		return MCDRV_SUCCESS;

	if (gsEdspInfo.dStatus == (UINT32)EDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	SetE(&sEdspInfo);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_Start
 *
 *	Function:
 *			Start edsp
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32 McEdsp_Start(void)
{
	if (gsEdspInfo.dStatus == (UINT32)EDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	EnableIrq();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_Stop
 *
 *	Function:
 *			Stop edsp
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32 McEdsp_Stop(void)
{
	if (gsEdspInfo.dStatus == (UINT32)EDSP_STATUS_IDLE)
		return MCDRV_ERROR;

	DisableIrq();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_E1Download
 *
 *	Function:
 *			E1 SysEq data download
 *	Arguments:
 *			pbSysEq0	SysEq0 data
 *			pbSysEq1	SysEq1 data
 *			bSysEqEnbReg	E-DSP #9 register set
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_E1Download(UINT8 *pbSysEq0, UINT8 *pbSysEq1, UINT8 bSysEqEnbReg)
{
	UINT8 bHwId;
	SINT32 sdRet;

	bHwId = McResCtrl_GetRegVal(MCDRV_PACKET_REGTYPE_A, MCI_A_DEV_ID);
	if ((bHwId & 0x07) == 0)
		sdRet = E1Download1Band(pbSysEq0, pbSysEq1, bSysEqEnbReg);
	else
		sdRet = E1Download3Band(pbSysEq0, pbSysEq1, bSysEqEnbReg);

	return sdRet;
}

/***************************************************************************
 *	McEdsp_E1WriteCommand
 *
 *	Function:
 *			E1 write command
 *	Arguments:
 *			bCmd		command
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_E1WriteCommand(UINT8 bCmd)
{
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_E
						| (UINT32)MCI_E1COMMAND),
						bCmd);
	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_E1OffsetMode
 *
 *	Function:
 *			Set E1 offset mode
 *	Arguments:
 *			bStart		Offset Start/Stop
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_E1OffsetMode(UINT8 bStart)
{
	UINT8 bCmd;

	if (bStart == OFFSET_START)
		bCmd = (E1_COMMAND_OFFSET_START | E1_COMMAND_RUN_MODE);
	else
		bCmd = (E1_COMMAND_OFFSET_STOP | E1_COMMAND_RUN_MODE);

	McEdsp_E1WriteCommand(bCmd);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	McEdsp_E1Impedance
 *
 *	Function:
 *			E1 Impedance detection
 *	Arguments:
 *			bStart		Start/stop detection
 *	Return:
 *			MCDRV_SUCCESS
 *
 ****************************************************************************/
SINT32 McEdsp_E1Impedance(UINT8 bStart)
{
	UINT8 bCmd;

	if (bStart == IMPEDANCE_START)
		bCmd = E1_COMMAND_IMPEDANCE_START;
	else
		bCmd = E1_COMMAND_IMPEDANCE_STOP;

	McEdsp_E1WriteCommand(bCmd);

	return MCDRV_SUCCESS;
}

