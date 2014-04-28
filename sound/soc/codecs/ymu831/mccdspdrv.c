/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mccdspdrv.c
 *
 *		Description	: CDSP Driver
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
#include "mccdspdrv.h"
#include "mccdspos.h"



/* macro */

#ifndef	LOBYTE
#define	LOBYTE(w)			(UINT8)((UINT32)(w) & 0xFFUL)
#endif	/* LOBYTE */
#ifndef	HIBYTE
#define	HIBYTE(w)			(UINT8)((UINT32)(w) >> 8)
#endif	/* HIBYTE */
#ifndef	MAKEWORD
#define	MAKEWORD(l, h)			((UINT16)((UINT32)(l) & 0xff) \
					| (UINT16)(((UINT32)(h) & 0xff) << 8))
#endif	/* LOBYTE */
#ifndef	MAKEBYTE
#define	MAKEBYTE(l, h)			(UINT8)(((l) & 0x0f) \
					| (((h) << 4) & 0xf0))
#endif	/* LOBYTE */

/* definition */

#define AEC_VBOX_DISABLE		(0)
#define AEC_VBOX_ENABLE			(1)

#define CDSP_FUNC_OFF			(0)
#define CDSP_FUNC_ON			(1)

#define MCDRV_CDSP_EVT_ERROR		(11)
#define MCDRV_CDSP_EVT_PARAM		(12)
#define MCDRV_CDSP_EVT_END		(13)
#define MCDRV_CDSP_EVT_FIFO		(14)
#define MCDRV_CDSP_EVT_TIMER		(15)


#define CODER_DEC			(0)	/* FuncA */
#define CODER_ENC			(1)	/* FuncB */
#define CODER_NUM			(2)

/* state */
#define STATE_NOTINIT			(0)
#define STATE_INIT			(1)
#define STATE_READY_SETUP		(2)
#define STATE_READY			(3)
#define STATE_PLAYING			(4)

/* call back */

#define CALLBACK_HOSTCOMMAND		(0)	/* Host command */
#define CALLBACK_END_OF_SEQUENCE	(1)	/* End */
#define CALLBACK_DFIFOPOINT		(2)	/* DEC_FIFO IRQ point */
#define CALLBACK_RFIFOPOINT		(3)	/* REC_FIFO IRQ point */
#define CALLBACK_DFIFOEMPTY		(4)	/* DEC_FIFO empty */
#define CALLBACK_RECBUF_OVF		(5)	/* Record buffer over flow */
#define CALLBACK_TIMER			(6)	/* Timer */
#define CALLBACK_PLAY_ERROR1		(7)	/* Play error */
#define CALLBACK_PLAY_ERROR2		(8)	/* Play error */
#define CALLBACK_HW_ERROR		(9)	/* Hardware error */
#define CALLBACK_COUNT			(10)
#define CALLBACK_OFF			(0)
#define CALLBACK_ON			(1)

/* callback position */

#define CBPOS_DFIFO_NONE		(-1)
#define CBPOS_DFIFO_DEF			(2048)
#define CBPOS_DFIFO_MIN			(0)
#define CBPOS_DFIFO_MAX			(4096)

#define CBPOS_RFIFO_NONE		(-1)
#define CBPOS_RFIFO_DEF			(2048)
#define CBPOS_RFIFO_MIN			(0)
#define CBPOS_RFIFO_MAX			(4096)

/* Program download flag */

#define PROGRAM_NO_DOWNLOAD		(0)
#define PROGRAM_DOWNLOAD		(1)

/* Program parameter */

#define PRG_DESC_VENDER_ID		(0)
#define PRG_DESC_FUNCTION_ID		(2)
#define PRG_DESC_PRG_TYPE		(4)
#define PRG_DESC_OUTPUT_TYPE		(6)
#define PRG_DESC_PRG_SCRAMBLE		(8)
#define PRG_DESC_DATA_SCRAMBLE		(10)
#define PRG_DESC_ENTRY_ADR		(12)
#define PRG_DESC_PRG_LOAD_ADR		(14)
#define PRG_DESC_PRG_SIZE		(16)
#define PRG_DESC_DATA_LOAD_ADR		(18)
#define PRG_DESC_DATA_SIZE		(20)
#define PRG_DESC_WORK_BEGIN_ADR		(22)
#define PRG_DESC_WORK_SIZE		(24)
#define PRG_DESC_STACK_BEGIN_ADR	(26)
#define PRG_DESC_STACK_SIZE		(28)
#define PRG_DESC_OUTSTARTMODE		(30)
#define PRG_DESC_RESOURCE_FLAG		(32)
#define PRG_DESC_MAX_LOAD		(34)
#define PRG_DESC_PROGRAM		(36)

/* Program data parameter */

#define	PRG_PRM_TYPE_TASK0		(0x0001)
#define	PRG_PRM_TYPE_TASK1		(0x0002)
#define	PRG_PRM_SCRMBL_DISABLE		(0x0000)
#define	PRG_PRM_SCRMBL_ENABLE		(0x0001)
#define	PRG_PRM_IOTYPE_IN_MASK		(0xFF00)
#define	PRG_PRM_IOTYPE_IN_PCM		(0x0000)
#define	PRG_PRM_IOTYPE_IN_NOPCM		(0x0100)
#define	PRG_PRM_IOTYPE_OUT_MASK		(0x00FF)
#define	PRG_PRM_IOTYPE_OUT_PCM		(0x0000)
#define	PRG_PRM_IOTYPE_OUT_NOPCM	(0x0001)

/* OS parameter */

#define ADR_OS_PROG_L			(0x00)
#define ADR_OS_PROG_H			(0x00)
#define ADR_OS_DATA_L			(0x00)
#define ADR_OS_DATA_H			(0x00)

/* CDSP MSEL */

#define MSEL_PROG			(0x00)
#define MSEL_DATA			(0x01)

/* FIFO size */

#define FIFOSIZE_DFIFO			(4096)
#define FIFOSIZE_OFIFO			(8192)
#define FIFOSIZE_EFIFO			(8192)
#define FIFOSIZE_RFIFO			(4096)
#define FIFOSIZE_FFIFO			(1024)

#define DFIFO_DUMMY_SPACE		(2)

/* FIFO ID */

#define FIFO_NONE			(0x00000000L)
#define FIFO_DFIFO_MASK			(0x000000FFL)
#define FIFO_DFIFO			(0x00000001L)
#define FIFO_EFIFO_MASK			(0x0000FF00L)
#define FIFO_EFIFO			(0x00000100L)
#define FIFO_OFIFO_MASK			(0x00FF0000L)
#define FIFO_OFIFO			(0x00010000L)
#define FIFO_RFIFO_MASK			(0xFF000000L)
#define FIFO_RFIFO			(0x01000000L)

#define PORT_SEL_NONE			(0)
#define PORT_SEL_NORMAL			(1)
#define PORT_SEL_REF			(2)

#define RFIFO_CH_NUM			(2)
#define RFIFO_BIT_WIDTH			(16)

/* format */

#define CODER_FMT_FS_48000		(0)
#define CODER_FMT_FS_44100		(1)
#define CODER_FMT_FS_32000		(2)
#define CODER_FMT_FS_24000		(4)
#define CODER_FMT_FS_22050		(5)
#define CODER_FMT_FS_16000		(6)
#define CODER_FMT_FS_12000		(8)
#define CODER_FMT_FS_11025		(9)
#define CODER_FMT_FS_8000		(10)

#define CODER_FMT_ETOBUF_LRMIX		(0)
#define CODER_FMT_ETOBUF_LCH		(1)
#define CODER_FMT_ETOBUF_RCH		(2)

#define CODER_FMT_BUFTOO_NONE		(0)
#define CODER_FMT_BUFTOO_CONV		(1)

/* InputDataEnd Command */

#define INPUTDATAEND_EMPTY		(0)
#define INPUTDATAEND_WRITE		(1)

/* TimerReste Command */

#define TIMERRESET_RESET		(0)
#define TIMERRESET_OFF			(1)

/* dual mono */

#define CODER_DUALMONO_LR		(0)
#define CODER_DUALMONO_L		(1)
#define CODER_DUALMONO_R		(2)

/* Fs */

#define OUTPUT_FS_8000			(8000)
#define OUTPUT_FS_11025			(11025)
#define OUTPUT_FS_12000			(12000)
#define OUTPUT_FS_16000			(16000)
#define OUTPUT_FS_22050			(22050)
#define OUTPUT_FS_24000			(24000)
#define OUTPUT_FS_32000			(32000)
#define OUTPUT_FS_44100			(44100)
#define OUTPUT_FS_48000			(48000)
#define OUTPUT_FS_MIN			OUTPUT_FS_8000
#define OUTPUT_FS_MAX			OUTPUT_FS_48000
#define OUTPUT_FS_DEF			OUTPUT_FS_48000

/* Start Sample */

#define OFIFO_BUF_SAMPLE_MIN		(0)
#define OFIFO_BUF_SAMPLE_MAX		(1024)
#define OFIFO_BUF_SAMPLE_DEF		(500)

#define RFIFO_BUF_SAMPLE_MIN		(0)
#define RFIFO_BUF_SAMPLE_MAX		(512)
#define RFIFO_BUF_SAMPLE_DEF		(500)

/* Start Flag */

#define OUTPUT_START_OFF		(0)
#define OUTPUT_START_ON			(1)

/* data end */

#define INPUT_DATAEND_RELEASE		(0)
#define INPUT_DATAEND_SET		(1)

/* Stop: verify stop completion */

#define MADEVCDSP_VERIFY_COMP_OFF	(0)
#define MADEVCDSP_VERIFY_COMP_ON	(1)

#define CHANGE_OUTPUT_FS_OFF		(0)
#define CHANGE_OUTPUT_FS_ON		(1)

#define FORMAT_PROPAGATE_OFF		(0)
#define FORMAT_PROPAGATE_ON		(1)

/* EVT */

#define EVENT_TIMER			(0x01)
#define EVENT_CLEAR			(0x00)

/* Error code: CDSP */

#define	CDSP_ERR_NO_ERROR		(0x0000)
#define	CDSP_ERR_MEM_PROTECTION		(0xFFF1)
#define	CDSP_ERR_WDT			(0xFFF2)
#define	CDSP_ERR_PROG_DOWNLOAD		(0xFFFF)

/* Error code: DEC/ENC */

#define	DEC_ERR_NO_ERROR		(0x00)
#define	DEC_ERR_PROG_SPECIFIC_MIN	(0x01)
#define	DEC_ERR_PROG_SPECIFIC_MAX	(0xEF)
#define	DEC_ERR_NOT_READY		(0xF0)
#define	DEC_ERR_MEM_PROTECTION		(0xF1)
#define	DEC_ERR_WDT			(0xF2)

/* c-dsp chunk */
#define CHUNK_SIZE			(8)

#define CDSP_FUNC_NUMBER		(0)

#define AEC_CDSP_TAG_PROG		(0x00001000)
#define PROG_FIX_SIZE			(4)
#define AEC_CDSP_TAG_PRM		(0x00001100)
#define PRM_FIX_SIZE			(4)
#define PRM_UNIT_SIZE			(17)
#define AEC_CDSP_TAG_FIFO		(0x00001200)
#define FIFO_FIX_SIZE			(26)
#define AEC_CDSP_TAG_EXT		(0x00001300)
#define EXT_FIX_SIZE			(5)

#define ROUTE_OUT0L_SEL			(0)
#define ROUTE_OUT0R_SEL			(1)
#define ROUTE_OUT1L_SEL			(2)
#define ROUTE_OUT1R_SEL			(3)
#define ROUTE_OUT2L_SEL			(4)
#define ROUTE_OUT2R_SEL			(5)
#define ROUTE_EFIFO0_SEL		(6)
#define ROUTE_EFIFO1_SEL		(7)
#define ROUTE_EFIFO2_SEL		(8)
#define ROUTE_EFIFO3_SEL		(9)
#define CDSP_EFIFO_CH			(10)
#define CDSP_EFIFO_BIT_WIDTH		(11)
#define CDSP_EFIFO_E2BUF_MODE		(12)
#define CDSP_OFIFO_CH			(13)
#define CDSP_OFIFO_BIT_WIDTH		(14)
#define CDSP_DFIFO_BIT_WIDTH		(15)
#define CDSP_RFIFO_BIT_WIDTH		(16)
#define CDSP_USE_FIFO			(17)
#define CDSP_DFIFO_CB_POINT		(18)	/* 2Byte */
#define CDSP_RFIFO_CB_POINT		(20)	/* 2Byte */
#define CDSP_OFIFO_BUFFERING		(22)	/* 2Byte */
#define CDSP_RFIFO_BUFFERING		(24)	/* 2Byte */

#define OUT_LOOPBACK_L			(4)
#define OUT_LOOPBACK_R			(5)

#define CDSP_FIFO_MASK			0x0F
#define CDSP_FIFO_EFIFO_BIT		0x01
#define CDSP_FIFO_OFIFO_BIT		0x02
#define CDSP_FIFO_DFIFO_BIT		0x04
#define CDSP_FIFO_RFIFO_BIT		0x08
#define CDSP_FIFO_OTHER_MASK		0xF0
#define CDSP_FIFO_OTHER_OUTBUF_BIT	0x10
#define CDSP_FIFO_OTHER_INBUF_BIT	0x20

#define CDSP_FIFO_DONTCARE		0xFF
#define CDSP_FIFO_DONTCARE_W		0xFFFF
#define CDSP_FIFO_DONTCARE_CB		0xFFFE
#define CDSP_FIFO_NOT_CB		0xFFFF

#define CDSP_PRM_CMD			(0)
#define CDSP_PRM_PRM0			(1)

#define AEC_FUNC_INFO_A			(0)
#define AEC_FUNC_INFO_B			(1)
#define AEC_FUNC_INFO_NUM		(2)

#define EXT_COMMAND			(0)
#define EXT_COMMAND_CLEAR		(1)

/* struct */

struct FSQ_DATA_INFO {
	const UINT8			*pbData;
	UINT16				wSize;
	UINT16				wLoadAddr;
	UINT16				wScramble;
	UINT8				bMsel;
};

struct FIFO_INFO {
	SINT32				sdDFifoCbPos;		/* DFIFO */
	SINT32				sdRFifoCbPos;		/* RFIFO */
	UINT32				dOFifoBufSample;	/* OFIFO */
	UINT8				bOFifoOutStart;
	UINT32				dDFifoWriteSize;
	UINT32				dRFifoBufSample;
	UINT8				bRFifoOutStart;

	UINT8				bOut0Sel;
	UINT8				bOut1Sel;
	UINT8				bOut2Sel;
	UINT8				bRDFifoBitSel;
	UINT8				bEFifo01Sel;
	UINT8				bEFifo23Sel;
};

struct VERSION_INFO {
	UINT16	wVersionH;
	UINT16	wVersionL;
};

struct CALLBACK_INFO {
	SINT32(*pcbFunc)(SINT32, UINT32, UINT32);	/* Callback function */
	UINT8				abCbOn[CALLBACK_COUNT];
	UINT32				adCbExInfo[CALLBACK_COUNT];
};

struct PROGRAM_INFO {
	UINT16				wVendorId;
	UINT16				wFunctionId;
	UINT16				wProgType;
	UINT16				wInOutType;
	UINT16				wProgScramble;
	UINT16				wDataScramble;
	UINT16				wEntryAddress;
	UINT16				wProgLoadAddr;
	UINT16				wProgSize;
	UINT16				wDataLoadAddr;
	UINT16				wDataSize;
	UINT16				wWorkBeginAddr;
	UINT16				wWorkSize;
	UINT16				wStackBeginAddr;
	UINT16				wStackSize;
	UINT16				wOutStartMode;
	UINT16				wResourceFlag;
	UINT16				wMaxLoad;
};

struct FORMAT_INFO {
	UINT8				bFs;
	UINT8				bE2BufMode;
};

struct CONNECTION_INFO {
	UINT8				bInSource;
	UINT8				bOutDest;
};

struct CONNECTION_EX_INFO {
	UINT8				bEfifoCh;
	UINT8				bOfifoCh;
};

struct BIT_WIDTH_INFO {
	UINT8				bEfifoBit;
	UINT8				bOfifoBit;
};

struct DEC_INFO {
	UINT32				dCoderId;	/* ID */
	UINT32				dState;		/* State */
	UINT8				bPreInputDataEnd;
	UINT8				bInputDataEnd;	/* input date end */
	UINT8				bChangeOutputFs;
	UINT8				bFmtPropagate;
	UINT32				dInPosSup;	/* Input Pos */
	UINT16				wErrorCode;	/* Task's error code */
	struct CALLBACK_INFO		sCbInfo;
	struct VERSION_INFO		sProgVer;	/* Program version */
	struct PROGRAM_INFO		sProgInfo;	/* Program infor */
	struct FORMAT_INFO		sFormat;	/* Format */
	struct CONNECTION_INFO		sConnect;	/* Connection */
	struct CONNECTION_EX_INFO	sConnectEx;	/* ConnectionEx */
	struct BIT_WIDTH_INFO		sBitWidth;	/* Bit Width */
	struct MC_CODER_PARAMS		sParams;	/* C-DSP Cmd Prm */
};

struct CDSP_INFO {
	UINT16				wHwErrorCode;	/* Hw's error code */
	struct VERSION_INFO		sOsVer;		/* OS version */
};

struct AEC_CDSP_FUNC_INFO {
	UINT32				dCoderId;
	UINT8				bFuncOnOff;
	UINT8				*pbChunk;
	UINT32				dChunkSize;
	UINT8				*pbFifo;
	UINT8				*pbProg;
	UINT32				dProgSize;
	UINT8				*pbParam;
	UINT32				dParamNum;
	UINT8				*pbExt;

	struct FORMAT_INFO		sFormat;
	struct CONNECTION_INFO		sConnect;
	struct CONNECTION_EX_INFO	sConnectEx;
	struct BIT_WIDTH_INFO		sBitWidth;
	UINT16				wInOutType;
};

struct MCDRV_CDSP_AEC_CDSP_INFO {
	struct AEC_CDSP_FUNC_INFO	asFuncInfo[AEC_FUNC_INFO_NUM];

	SINT32				sdDFifoCbPos;
	SINT32				sdRFifoCbPos;
	UINT32				dOFifoBufSample;
	UINT32				dRFifoBufSample;
	UINT8				bOut0Sel;
	UINT8				bOut1Sel;
	UINT8				bOut2Sel;
	UINT8				bRDFifoBitSel;
	UINT8				bEFifo01Sel;
	UINT8				bEFifo23Sel;
};

static struct CDSP_INFO gsCdspInfo = {
	CDSP_ERR_NO_ERROR,				/* wHwErrorCode */
	{						/* sOsVer */
		0,					/* wVersionH */
		0					/* wVersionL */
	}
};

static struct FIFO_INFO gsFifoInfo = {
	0,						/* sdDFifoCbPos */
	0,						/* sdRFifoCbPos */
	0,						/* dOFifoBufSample */
	0,						/* bOFifoOutStart */
	0,						/* dDFifoWriteSize */
	0,
	0,
	0,
	0,
	0,
	0
};

static struct DEC_INFO gsDecInfo = {
	CODER_DEC,					/* dCoderId */
	STATE_NOTINIT,					/* dState */
	INPUT_DATAEND_RELEASE,				/* bPreInputDataEnd */
	INPUT_DATAEND_RELEASE,				/* bInputDataEnd */
	CHANGE_OUTPUT_FS_OFF,				/* bChangeOutputFs */
	FORMAT_PROPAGATE_OFF,				/* bFmtPropagate */
	0,						/* dInPosSup */
	0,						/* wErrorCode */
	{						/* sCbInfo */
		NULL,					/* pcbFunc */
		{					/* abCbOn */
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF
		},
		{					/* adCbExInfo */
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0
		},
	},
	{						/* sProgVer */
		0,					/* wVersionH */
		0					/* wVersionL */
	},
	{						/* sProgInfo */
		0,					/* wVendorId */
		0,					/* wFunctionId */
		0,					/* wProgType */
		0,					/* wInOutType */
		0,					/* wProgScramble */
		0,					/* wDataScramblev*/
		0,					/* wEntryAddress */
		0,					/* wProgLoadAddr */
		0,					/* wProgSize */
		0,					/* wDataLoadAddr */
		0,					/* wDataSize */
		0,					/* wWorkBeginAddr */
		0,					/* wWorkSize */
		0,					/* wStackBeginAddr */
		0,					/* wStackSize */
		0,					/* wOutStartMode */
		0,					/* wResourceFlag */
		0					/* wMaxLoad */
	},
	{						/* sFormat */
		CODER_FMT_FS_48000,			/* bFs */
		CODER_FMT_ETOBUF_LRMIX			/* bE2BufMode */
	},
	{						/* sConnect */
		CDSP_IN_SOURCE_NONE,			/* bInSource */
		CDSP_OUT_DEST_NONE			/* bOutDest */
	},
	{						/* sConnectEx */
		2,					/* bEfifoCh */
		2					/* bOfifoCh */
	},
	{						/* sBitWidth */
		16,					/* bEfifoBit */
		16					/* bOfifoBit */
	},
	{						/* sParams */
		0,					/* bCommandId */
		{					/* abParam */
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
			0
		},
	}
};

static struct DEC_INFO gsEncInfo = {
	CODER_ENC,					/* dCoderId */
	STATE_NOTINIT,					/* dState */
	INPUT_DATAEND_RELEASE,				/* bPreInputDataEnd */
	INPUT_DATAEND_RELEASE,				/* bInputDataEnd */
	CHANGE_OUTPUT_FS_OFF,				/* bChangeOutputFs */
	FORMAT_PROPAGATE_OFF,				/* bFmtPropagate */
	0,						/* dInPosSup */
	0,						/* wErrorCode */
	{						/* sCbInfo */
		NULL,					/* pcbFunc */
		{					/* abCbOn */
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF,
			CALLBACK_OFF
		},
		{					/* adCbExInfo */
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0
		},
	},
	{						/* sProgVer */
		0,					/* wVersionH */
		0					/* wVersionL */
	},
	{						/* sProgInfo */
		0,					/* wVendorId */
		0,					/* wFunctionId */
		0,					/* wProgType */
		0,					/* wInOutType */
		0,					/* wProgScramble */
		0,					/* wDataScramblev*/
		0,					/* wEntryAddress */
		0,					/* wProgLoadAddr */
		0,					/* wProgSize */
		0,					/* wDataLoadAddr */
		0,					/* wDataSize */
		0,					/* wWorkBeginAddr */
		0,					/* wWorkSize */
		0,					/* wStackBeginAddr */
		0,					/* wStackSize */
		0,					/* wOutStartMode */
		0,					/* wResourceFlag */
		0					/* wMaxLoad */
	},
	{						/* sFormat */
		CODER_FMT_FS_48000,			/* bFs */
		CODER_FMT_ETOBUF_LRMIX			/* bE2BufMode */
	},
	{						/* sConnect */
		CDSP_IN_SOURCE_NONE,			/* bInSource */
		CDSP_OUT_DEST_NONE			/* bOutDest */
	},
	{						/* sConnectEx */
		2,					/* bEfifoCh */
		2					/* bOfifoCh */
	},
	{						/* sBitWidth */
		16,					/* bEfifoBit */
		16					/* bOfifoBit */
	},
	{						/* sParams */
		0,					/* bCommandId */
		{					/* abParam */
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
			0
		},
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
 *	InitializeRegister
 *
 *	Function:
 *			Initialize CDSP registers.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InitializeRegister(void)
{
	SINT32	i;

	/* CDSP SRST */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
			| MCDRV_PACKET_REGTYPE_C
			| (UINT32)MCI_CDSP_RESET),
			(MCB_CDSP_DMODE | MCB_CDSP_FSQ_SRST | MCB_CDSP_SRST));

	/* Disable interrupt */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					0x00);

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FLG),
					MCB_DEC_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_FLG),
					MCB_ENC_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_FLG),
					MCB_DFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_FLG),
					MCB_OFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_FLG),
					MCB_EFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_FLG),
					MCB_RFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_FLG),
					MCB_FFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_FLG),
					MCB_CDSP_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_CDSP_ALL);

	/* Other registers */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START2),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					0x00);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_PWM_DIGITAL_CDSP),
					0x00);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_POS4),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_POS4),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_IRQ_PNT_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_IRQ_PNT_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_IRQ_PNT_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_IRQ_PNT_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_IRQ_PNT_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_IRQ_PNT_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_IRQ_PNT_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_IRQ_PNT_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_MAR_H),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_MAR_L),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_GPR_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_SFR1),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_SFR0),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_GPR_ENABLE),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_SFR1),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_SFR0),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_EVT),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_EVT),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_CH),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT0_SEL),
					MCI_OUT0_SEL_DEF);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT1_SEL),
					MCI_OUT1_SEL_DEF);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT2_SEL),
					MCI_OUT2_SEL_DEF);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO01_SEL),
					MCI_EFIFO01_SEL_DEF);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO23_SEL),
					MCI_EFIFO23_SEL_DEF);
	for (i = 0; i < (SINT32)CDSP_CMD_PARAM_NUM; i++) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| ((UINT32)MCI_DEC_GPR15 + (UINT32)i)),
					0x00);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| ((UINT32)MCI_ENC_GPR15 + (UINT32)i)),
					 0x00);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| ((UINT32)MCI_DEC_CTL15 + (UINT32)i)),
					0x00);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| ((UINT32)MCI_ENC_CTL15 + (UINT32)i)),
					0x00);
	}
	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	DownloadOS
 *
 *	Function:
 *			Download CDSP OS.
 *	Arguments:
 *			pbFirmware	OS data
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DownloadOS(const UINT8 *pbFirmware)
{
	UINT8		bData;
	UINT8		abMsel[2];
	UINT8		abAdrH[2];
	UINT8		abAdrL[2];
	UINT16		awOsSize[2];
	SINT32		i;
	UINT32		j;
	const UINT8	*apbOsProg[2];

	abMsel[0]	= (UINT8)MCB_CDSP_MSEL_PROG;
	abAdrH[0]	= (UINT8)ADR_OS_PROG_H;
	abAdrL[0]	= (UINT8)ADR_OS_PROG_L;
	awOsSize[0]	= MAKEWORD(pbFirmware[0], pbFirmware[1]);
	apbOsProg[0]	= &pbFirmware[4];

	abMsel[1]	= (UINT8)MCB_CDSP_MSEL_DATA;
	abAdrH[1]	= (UINT8)ADR_OS_DATA_H;
	abAdrL[1]	= (UINT8)ADR_OS_DATA_L;
	awOsSize[1]	= MAKEWORD(pbFirmware[2], pbFirmware[3]);
	apbOsProg[1]	= &apbOsProg[0][((UINT32)awOsSize[0] * 2UL)];

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						&bData, 1);

	/* CDSP_SRST Set : CDSP OS stop */
	bData &= ~MCB_CDSP_FMODE;
	bData |= MCB_CDSP_SRST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);

	/* Program & Data Write */
	for (i = 0; i < 2L; i++) {
		/* CDSP_MSEL Set */
		bData &= ~MCB_CDSP_MSEL;
		bData |= abMsel[i];
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);

		/* CDSP_MAR Set */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_MAR_H),
						abAdrH[i]);
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_MAR_L),
						abAdrL[i]);

		McDevIf_ExecutePacket();

		/* FSQ_FIFO Write */
		for (j = 0; j < ((UINT32)awOsSize[i] * 2UL); ++j)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_FSQ_FFIFO),
						apbOsProg[i][j]);

		McDevIf_ExecutePacket();

	}

	/* CDSP_SRST Release : CDSP OS start */
	bData &= ~MCB_CDSP_SRST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);

	/* 100 ns wait */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_TIMWAIT | 1UL), 0x00);

	McDevIf_ExecutePacket();

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CommandWriteComplete
 *
 *	Function:
 *			The completion of the command is notified.
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Command ID and result data
 *	Return:
 *			None
 *
 ****************************************************************************/
static void CommandWriteComplete(UINT32 dCoderId,
				struct MC_CODER_PARAMS *psParam)
{
	UINT8 bAddSfr;
	UINT8 bAddCtl;
	UINT32 dCount;
	UINT32 i;

	if (CODER_DEC == dCoderId) {
		bAddSfr = MCI_DEC_SFR1;
		bAddCtl = MCI_DEC_CTL0;
	} else {
		bAddSfr = MCI_ENC_SFR1;
		bAddCtl = MCI_ENC_CTL0;
	}

	/* Write result */
	dCount = (UINT32)(CDSP_CMD_PARAM_RESULT_00 +
						CDSP_CMD_PARAM_RESULT_NUM);
	for (i = (UINT32)CDSP_CMD_PARAM_RESULT_00; i < dCount; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| ((UINT32)bAddCtl - i)),
					psParam->abParam[i]);

	/* Write complete command */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)bAddSfr),
					(UINT8)(psParam->bCommandId
					| CDSP_CMD_OS2HOST_COMPLETION));

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	CommandWaitComplete
 *
 *	Function:
 *			It waits until the command transmission is completed.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			>= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CommandWaitComplete(UINT32 dCoderId)
{
	SINT32 sdRet;
	UINT8 bAddSfr;

	if (CODER_DEC == dCoderId)
		bAddSfr = MCI_DEC_SFR0;
	else
		bAddSfr = MCI_ENC_SFR0;

	/* Polling */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_C_REG_FLAG_SET
					| (((UINT32)bAddSfr) << 8)
					| (UINT32)CDSP_CMD_HOST2OS_COMPLETION),
					0);

	sdRet = McDevIf_ExecutePacket();
	if ((SINT32)MCDRV_SUCCESS > sdRet) {
		/* Time out */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)bAddSfr),
					CDSP_CMD_HOST2OS_COMPLETION);

		McDevIf_ExecutePacket();
	}

	return sdRet;
}

/****************************************************************************
 *	CommandInitialize
 *
 *	Function:
 *			Initialize register of command sending and receiving.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			>= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CommandInitialize(UINT32 dCoderId)
{
	SINT32 sdRet;
	struct MC_CODER_PARAMS sParam;

	sParam.bCommandId = (UINT8)CDSP_CMD_OS2HOST_CMN_NONE;
	sParam.abParam[CDSP_CMD_PARAM_RESULT_00] = 0;
	sParam.abParam[CDSP_CMD_PARAM_RESULT_01] = 0;
	sParam.abParam[CDSP_CMD_PARAM_RESULT_02] = 0;
	sParam.abParam[CDSP_CMD_PARAM_RESULT_03] = 0;

	CommandWriteComplete(dCoderId, &sParam);
	sdRet = CommandWaitComplete(dCoderId);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CommandWriteHost2Os
 *
 *	Function:
 *			Write command (Host -> OS).
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Command ID and parameter data
 *	Return:
 *			>= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CommandWriteHost2Os(UINT32 dCoderId,
					struct MC_CODER_PARAMS *psParam)
{
	UINT8	bData;
	UINT32	dAddSfr;
	UINT32	dAddCtl;
	UINT32	dAddGpr;
	UINT32	dCount;
	UINT32	i;
	SINT32	sdRet;

	if (CODER_DEC == dCoderId) {
		dAddSfr = MCI_DEC_SFR0;
		dAddCtl = MCI_DEC_CTL0;
		dAddGpr = MCI_DEC_GPR0;
	} else {
		dAddSfr = MCI_ENC_SFR0;
		dAddCtl = MCI_ENC_CTL0;
		dAddGpr = MCI_ENC_GPR0;
	}

	/* Polling */
	sdRet = CommandWaitComplete(dCoderId);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Write parameter */
	switch (psParam->bCommandId) {
	case CDSP_CMD_HOST2OS_CMN_NONE:
	case CDSP_CMD_HOST2OS_CMN_RESET:
	case CDSP_CMD_HOST2OS_CMN_CLEAR:
	case CDSP_CMD_HOST2OS_CMN_STANDBY:
	case CDSP_CMD_HOST2OS_CMN_GET_PRG_VER:
	case CDSP_CMD_HOST2OS_SYS_GET_OS_VER:
	case CDSP_CMD_HOST2OS_SYS_VERIFY_STOP_COMP:
	case CDSP_CMD_HOST2OS_SYS_CLEAR_INPUT_DATA_END:
	case CDSP_CMD_HOST2OS_SYS_TERMINATE:
	case CDSP_CMD_HOST2OS_SYS_GET_INPUT_POS:
	case CDSP_CMD_HOST2OS_SYS_RESET_INPUT_POS:
	case CDSP_CMD_HOST2OS_SYS_HALT:
		dCount = 0;
		break;
	case CDSP_CMD_HOST2OS_SYS_INPUT_DATA_END:
	case CDSP_CMD_HOST2OS_SYS_TIMER_RESET:
	case CDSP_CMD_HOST2OS_SYS_SET_DUAL_MONO:
	case CDSP_CMD_HOST2OS_SYS_SET_CLOCK_SOURCE:
		dCount = 1;
		break;
	case CDSP_CMD_HOST2OS_SYS_SET_PRG_INFO:
		dCount = 11;
		break;
	case CDSP_CMD_HOST2OS_SYS_SET_FORMAT:
		dCount = 2;
		break;
	case CDSP_CMD_HOST2OS_SYS_SET_CONNECTION:
		dCount = 2;
		break;
	case CDSP_CMD_HOST2OS_SYS_SET_TIMER:
		dCount = 4;
		break;
	default:
		/* Program dependence command */
		dCount = (UINT32)CDSP_CMD_PARAM_ARGUMENT_NUM;
		break;
	}

	for (i = 0; i < dCount; i++)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| ((UINT32)dAddCtl - i)),
						psParam->abParam[i]);

	/* Write command */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)dAddSfr),
						psParam->bCommandId);

	McDevIf_ExecutePacket();

	/* Polling */
	sdRet = CommandWaitComplete(dCoderId);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Error check */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)dAddSfr),
								&bData, 1);
	if (0xF0 <= bData)
		switch (bData) {
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
			return MCDRV_ERROR_ARGUMENT;
		case 0xF7:
			return MCDRV_ERROR_STATE;
		default:
			return MCDRV_ERROR;
		}

	/* Get result */
	dCount = (UINT32)(CDSP_CMD_PARAM_RESULT_00 +
					CDSP_CMD_PARAM_RESULT_NUM);
	for (i = (UINT32)CDSP_CMD_PARAM_RESULT_00; i < dCount; i++) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)(dAddGpr - i)),
						&bData,
						1);
		psParam->abParam[i] = bData;
	}

	return (SINT32)bData;
}

/***************************************************************************
 *	InitializeOS
 *
 *	Function:
 *			Initialize CDSP OS.
 *	Arguments:
 *			None
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 InitializeOS(void)
{
	SINT32 sdRet;
	struct MC_CODER_PARAMS sParam;

	/* CDSP_ERR/WDT_FLG Flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_FLG),
						MCB_CDSP_FLG_ALL);

	/* IRQ Flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_CDSP_ALL);

	/* ECDSP_ERR/WDT Enable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_ENABLE),
						MCB_CDSP_ENABLE_ALL);

	/* IRQ: ECDSP=Enable, other=Disable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					MCB_ECDSP);

	McDevIf_ExecutePacket();

	/* Command register Initialize */
	sdRet = CommandInitialize(CODER_DEC);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	sdRet = CommandInitialize(CODER_ENC);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* READY Polling */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_C_REG_FLAG_SET
					| (((UINT32)MCI_CDSP_POWER_MODE) << 8)
					| (UINT32)MCB_CDSP_SLEEP),
					0);

	sdRet = McDevIf_ExecutePacket();
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* GetOsVersion command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_GET_OS_VER;
	sdRet = CommandWriteHost2Os(CODER_DEC, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	gsCdspInfo.sOsVer.wVersionH = MAKEWORD(
				sParam.abParam[CDSP_CMD_PARAM_RESULT_00],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_01]);
	gsCdspInfo.sOsVer.wVersionL = MAKEWORD(
				sParam.abParam[CDSP_CMD_PARAM_RESULT_02],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_03]);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	InitDecInfo
 *
 *	Function:
 *			Initialize DEC_INFO
 *	Arguments:
 *			psDecInfo	Pointer of Dec info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InitDecInfo(struct DEC_INFO *psDecInfo)
{
	SINT32 i;

	psDecInfo->dState			= STATE_INIT;
	psDecInfo->bPreInputDataEnd		= INPUT_DATAEND_RELEASE;
	psDecInfo->bInputDataEnd		= INPUT_DATAEND_RELEASE;
	psDecInfo->bChangeOutputFs		= CHANGE_OUTPUT_FS_OFF;
	psDecInfo->bFmtPropagate		= FORMAT_PROPAGATE_OFF;
	psDecInfo->dInPosSup			= 0;
	psDecInfo->wErrorCode			= DEC_ERR_NO_ERROR;
	psDecInfo->sCbInfo.pcbFunc		= NULL;
	for (i = 0; i < (SINT32)CALLBACK_COUNT; i++) {
		psDecInfo->sCbInfo.abCbOn[i]	= CALLBACK_OFF;
		psDecInfo->sCbInfo.adCbExInfo[i] = 0;
	}
	psDecInfo->sProgVer.wVersionH		= 0;
	psDecInfo->sProgVer.wVersionL		= 0;
	psDecInfo->sProgInfo.wVendorId		= 0;
	psDecInfo->sProgInfo.wFunctionId	= 0;
	psDecInfo->sProgInfo.wProgType		= 0;
	psDecInfo->sProgInfo.wInOutType =
			PRG_PRM_IOTYPE_IN_PCM | PRG_PRM_IOTYPE_OUT_PCM;
	psDecInfo->sProgInfo.wProgScramble	= 0;
	psDecInfo->sProgInfo.wDataScramble	= 0;
	psDecInfo->sProgInfo.wEntryAddress	= 0;
	psDecInfo->sProgInfo.wProgLoadAddr	= 0;
	psDecInfo->sProgInfo.wProgSize		= 0;
	psDecInfo->sProgInfo.wDataLoadAddr	= 0;
	psDecInfo->sProgInfo.wDataSize		= 0;
	psDecInfo->sProgInfo.wWorkBeginAddr	= 0;
	psDecInfo->sProgInfo.wWorkSize		= 0;
	psDecInfo->sProgInfo.wStackBeginAddr	= 0;
	psDecInfo->sProgInfo.wStackSize		= 0;
	psDecInfo->sProgInfo.wOutStartMode	= 0;
	psDecInfo->sProgInfo.wResourceFlag	= 0;
	psDecInfo->sProgInfo.wMaxLoad		= 0;
	psDecInfo->sFormat.bFs			= CODER_FMT_FS_48000;
	psDecInfo->sFormat.bE2BufMode		= CODER_FMT_ETOBUF_LRMIX;
	psDecInfo->sConnect.bInSource		= CDSP_IN_SOURCE_NONE;
	psDecInfo->sConnect.bOutDest		= CDSP_OUT_DEST_NONE;
	psDecInfo->sConnectEx.bEfifoCh		= 2;
	psDecInfo->sConnectEx.bOfifoCh		= 2;
	psDecInfo->sBitWidth.bEfifoBit		= 16;
	psDecInfo->sBitWidth.bOfifoBit		= 16;
	psDecInfo->sParams.bCommandId		= 0;
	for (i = 0; i < 16L; i++)
		psDecInfo->sParams.abParam[i]	= 0;
}

/****************************************************************************
 *	Initialize
 *
 *	Function:
 *			Initialize CDSP and SMW.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static SINT32 Initialize(void)
{
	const UINT8 *pbFirmware;

	if ((STATE_PLAYING == gsDecInfo.dState)
		|| (STATE_PLAYING == gsEncInfo.dState))
		return MCDRV_ERROR_STATE;

	/* Global Initialize */
	gsCdspInfo.wHwErrorCode				= CDSP_ERR_NO_ERROR;
	gsCdspInfo.sOsVer.wVersionH			= 0;
	gsCdspInfo.sOsVer.wVersionL			= 0;
	gsFifoInfo.sdDFifoCbPos				= CBPOS_DFIFO_DEF;
	gsFifoInfo.sdRFifoCbPos				= CBPOS_RFIFO_DEF;
	gsFifoInfo.dOFifoBufSample			= OFIFO_BUF_SAMPLE_DEF;
	gsFifoInfo.bOFifoOutStart			= OUTPUT_START_OFF;
	gsFifoInfo.dDFifoWriteSize			= 0;
	gsFifoInfo.dRFifoBufSample			= RFIFO_BUF_SAMPLE_DEF;
	gsFifoInfo.bRFifoOutStart			= OUTPUT_START_OFF;
	gsFifoInfo.bOut0Sel				= MCI_OUT0_SEL_DEF;
	gsFifoInfo.bOut1Sel				= MCI_OUT1_SEL_DEF;
	gsFifoInfo.bOut2Sel				= MCI_OUT2_SEL_DEF;
	gsFifoInfo.bRDFifoBitSel			= 0;
	gsFifoInfo.bEFifo01Sel				= MCI_EFIFO01_SEL_DEF;
	gsFifoInfo.bEFifo23Sel				= MCI_EFIFO23_SEL_DEF;

	InitDecInfo(&gsDecInfo);
	InitDecInfo(&gsEncInfo);

	InitializeRegister();

	/* CDSP OS Download */
	pbFirmware = gabCdspOs;

	DownloadOS(pbFirmware);

	/* CDSP OS initialize */
	InitializeOS();

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	GetDecInfo
 *
 *	Function:
 *			Get CDSP decoder/encoder management
 *			module's information.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			DEC_INFO*
 *
 ****************************************************************************/
static struct DEC_INFO *GetDecInfo(UINT32 dCoderId)
{
	if (dCoderId == CODER_DEC)
		return &gsDecInfo;

	return &gsEncInfo;
}

/***************************************************************************
 *	GetOtherDecInfo
 *
 *	Function:
 *			Get CDSP decoder/encoder management
 *			module's information.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			DEC_INFO*
 *
 ****************************************************************************/
static struct DEC_INFO *GetOtherDecInfo(UINT32 dCoderId)
{
	if (dCoderId == CODER_DEC)
		return &gsEncInfo;

	return &gsDecInfo;
}

/****************************************************************************
 *	CommandReadOs2Host
 *
 *	Function:
 *			Read command (Host <- OS).
 *	Arguments:
 *			eCoderId	Coder ID
 *			psParam		Command ID and parameter data
 *	Return:
 *			>= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static void CommandReadOs2Host(UINT32 dCoderId,
					struct MC_CODER_PARAMS *psParam)
{
	UINT8 bAdd;
	UINT8 bData;
	UINT32 dGpr;
	UINT32 dCount;
	UINT32 i;

	if (CODER_DEC == dCoderId) {
		bAdd = MCI_DEC_SFR1;
		dGpr = (UINT32)MCI_DEC_GPR0;
	} else {
		bAdd = MCI_ENC_SFR1;
		dGpr = (UINT32)MCI_ENC_GPR0;
	}

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)bAdd), &bData, 1);
	psParam->bCommandId = bData;

	switch (psParam->bCommandId) {
	case CDSP_CMD_OS2HOST_CMN_NONE:
		dCount = 0;
		break;

	case CDSP_CMD_OS2HOST_CMN_NOTIFY_OUT_FORMAT:
		dCount = 4;
		break;

	default:
		/* Program dependence command */
		dCount = (UINT32)CDSP_CMD_PARAM_ARGUMENT_NUM;
		break;
	}

	for (i = 0; i < dCount; i++) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (dGpr - i)),
								&bData, 1);
		psParam->abParam[i] = bData;
	}
}

/****************************************************************************
 *	GetRFifoSel
 *
 *	Function:
 *			RFIFO setting acquisition of sel
 *	Arguments:
 *			None
 *	Return:
 *			Port/Host
 *
 ****************************************************************************/
static UINT8 GetRFifoSel(void)
{
	if ((gsFifoInfo.bRDFifoBitSel & MCB_RFIFO_SEL_HOST) != 0)
		return CDSP_FIFO_SEL_HOST;

	return CDSP_FIFO_SEL_PORT;
}

/****************************************************************************
 *	GetDFifoSel
 *
 *	Function:
 *			DFIFO setting acquisition of sel
 *	Arguments:
 *			None
 *	Return:
 *			Port/Host
 *
 ****************************************************************************/
static UINT8 GetDFifoSel(void)
{
	if ((gsFifoInfo.bRDFifoBitSel & MCB_DFIFO_SEL_HOST) != 0)
		return CDSP_FIFO_SEL_HOST;

	return CDSP_FIFO_SEL_PORT;
}

/***************************************************************************
 *	CommandInputDataEnd
 *
 *	Function:
 *			Set Input data end.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CommandInputDataEnd(UINT32 dCoderId)
{
	UINT8 bInSource;
	UINT8 bOutDest;
	UINT8 bData;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);
	bInSource = psDecInfo->sConnect.bInSource;

	if ((CDSP_IN_SOURCE_DFIFO == bInSource)
		|| (CDSP_IN_SOURCE_DFIFO_EFIFO == bInSource)) {
		if (0x01L == (gsFifoInfo.dDFifoWriteSize & 0x01L)) {
			bData = 0x00;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_DEC_FIFO),
						bData);

			gsFifoInfo.dDFifoWriteSize = 0;
		}
	}

	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = (UINT8)INPUTDATAEND_EMPTY;
	bOutDest = psDecInfo->sConnect.bOutDest;
	if ((GetRFifoSel() == CDSP_FIFO_SEL_HOST)
		&& ((CDSP_OUT_DEST_RFIFO == bOutDest)
		|| (CDSP_OUT_DEST_OFIFO_RFIFO == bOutDest)))
		sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] =
						(UINT8)INPUTDATAEND_WRITE;

	/* InputDataEnd command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_INPUT_DATA_END;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	OFifoStartReal
 *
 *	Function:
 *			Start OFIFO.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void OFifoStartReal(void)
{
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_START), &bData, 1);

	bData |= MCB_DEC_OUT_START;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_START), bData);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	RFifoStartReal
 *
 *	Function:
 *			Start RFIFO.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void RFifoStartReal(void)
{
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_START2), &bData, 1);

	bData |= MCB_RFIFO_START;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_START2), bData);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	GetFifoIdCore
 *
 *	Function:
 *			Get input/output FIFO ID from path.
 *	Arguments:
 *			psConnect	CONNECTION_INFO structure pointer
 *	Return:
 *			Fifo Id
 *
 ****************************************************************************/
static UINT32 GetFifoIdCore(struct CONNECTION_INFO *psConnect)
{
	UINT32 dFifoId;

	dFifoId = 0;

	switch (psConnect->bInSource) {
	case CDSP_IN_SOURCE_DFIFO:
		dFifoId |= FIFO_DFIFO;
		break;

	case CDSP_IN_SOURCE_EFIFO:
		dFifoId |= FIFO_EFIFO;
		break;

	case CDSP_IN_SOURCE_DFIFO_EFIFO:
		dFifoId |= FIFO_DFIFO;
		dFifoId |= FIFO_EFIFO;
		break;

	case CDSP_IN_SOURCE_OTHER_OUTBUF:
	case CDSP_IN_SOURCE_NONE:
	default:
		break;
	}

	switch (psConnect->bOutDest) {
	case CDSP_OUT_DEST_OFIFO:
		dFifoId |= FIFO_OFIFO;
		break;

	case CDSP_OUT_DEST_RFIFO:
		dFifoId |= FIFO_RFIFO;
		break;

	case CDSP_OUT_DEST_OFIFO_RFIFO:
		dFifoId |= FIFO_OFIFO;
		dFifoId |= FIFO_RFIFO;
		break;

	case CDSP_OUT_DEST_OTHER_INBUF:
	case CDSP_OUT_DEST_NONE:
	default:
		break;
	}

	return dFifoId;
}

/****************************************************************************
 *	GetFifoId
 *
 *	Function:
 *			Get input/output FIFO ID from path.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			Fifo Id
 *
 ****************************************************************************/
static UINT32 GetFifoId(UINT32 dCoderId)
{
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	dFifoId = GetFifoIdCore(&(psDecInfo->sConnect));

	return dFifoId;
}

/****************************************************************************
 *	NotifyOutFormatOs2Host
 *
 *	Function:
 *			Decoder/Encoder SFR Interrupt (NotifyOutputFormat).
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void NotifyOutFormatOs2Host(UINT32 dCoderId)
{
	UINT32 dFlagOtherFifo;
	SINT32 sdRet;
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *psOtherDecInfo;

	psDecInfo = GetDecInfo(dCoderId);
	psOtherDecInfo = GetOtherDecInfo(dCoderId);

	/* Fs */
	psDecInfo->sFormat.bFs =
			psDecInfo->sParams.abParam[CDSP_CMD_PARAM_ARGUMENT_00];

	dFlagOtherFifo = 0;
	if (psDecInfo->sConnect.bOutDest == CDSP_OUT_DEST_OTHER_INBUF) {
		dFifoId = GetFifoIdCore(&(psOtherDecInfo->sConnect));
		if ((dFifoId & FIFO_OFIFO_MASK) != 0)
			dFlagOtherFifo |= FIFO_OFIFO;

		if (((dFifoId & FIFO_RFIFO_MASK) != 0)
			&& (GetRFifoSel() == CDSP_FIFO_SEL_PORT))
			dFlagOtherFifo |= FIFO_RFIFO;
	} else {
		dFifoId = GetFifoIdCore(&(psDecInfo->sConnect));
		dFifoId &= ~(FIFO_DFIFO_MASK | FIFO_EFIFO_MASK);
		if (GetRFifoSel() != CDSP_FIFO_SEL_PORT)
			dFifoId &= ~FIFO_RFIFO_MASK;

		if (FIFO_NONE != dFifoId) {
			if ((dFifoId & FIFO_OFIFO_MASK) != 0)
				/* Wait OFIFO_EMPTY */
				McDevIf_AddPacket(
					(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_FLG),
					MCB_OFIFO_FLG_OEMP);

			if ((dFifoId & FIFO_RFIFO_MASK) != 0)
				/* Wait RFIFO_EMPTY */
				McDevIf_AddPacket(
					(MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_FLG),
					MCB_RFIFO_FLG_REMP);

			if ((dFifoId & FIFO_OFIFO_MASK) != 0)
				McDevIf_AddPacket(
					(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_C_REG_FLAG_SET
					| (((UINT32)MCI_OFIFO_FLG) << 8)
					| (UINT32)MCB_OFIFO_FLG_OEMP),
					0);

			if ((dFifoId & FIFO_RFIFO_MASK) != 0)
				McDevIf_AddPacket(
					(MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_C_REG_FLAG_SET
					| (((UINT32)MCI_RFIFO_FLG) << 8)
					| (UINT32)MCB_RFIFO_FLG_REMP),
					0);

			sdRet = McDevIf_ExecutePacket();

			if ((dFifoId & FIFO_OFIFO_MASK) != 0)
				/* Clear OFIFO_EMPTY */
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_OFIFO_FLG),
						MCB_OFIFO_FLG_ALL);

			if ((dFifoId & FIFO_RFIFO_MASK) != 0)
				/* Clear RFIFO_EMPTY */
				McDevIf_AddPacket(
						(MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_RFIFO_FLG),
						MCB_RFIFO_FLG_ALL);

			McDevIf_ExecutePacket();
		}
	}

	if ((FIFO_NONE != dFlagOtherFifo)
		&& (psOtherDecInfo->bInputDataEnd == INPUT_DATAEND_RELEASE)) {
		psOtherDecInfo->bPreInputDataEnd = INPUT_DATAEND_RELEASE;

		/* InputDataEnd command */
		sdRet = CommandInputDataEnd(psOtherDecInfo->dCoderId);

		/* FormatPropagate state - set */
		psOtherDecInfo->bFmtPropagate = FORMAT_PROPAGATE_ON;

		if ((dFlagOtherFifo & FIFO_OFIFO_MASK) != 0)
			/* OUT Start */
			OFifoStartReal();

		if ((dFlagOtherFifo & FIFO_RFIFO_MASK) != 0)
			/* OUT Start */
			RFifoStartReal();

	} else
		/* Callback (HOST COMMAND) */
		psDecInfo->sCbInfo.abCbOn[CALLBACK_HOSTCOMMAND] = CALLBACK_ON;
}

/****************************************************************************
 *	InterruptProcDecSfr
 *
 *	Function:
 *			Decoder/Encoder SFR Interrupt.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDecSfr(UINT32 dCoderId)
{
	UINT8 bCmdId;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* Read SFR data */
	CommandReadOs2Host(dCoderId, &psDecInfo->sParams);
	bCmdId = psDecInfo->sParams.bCommandId;

	if ((UINT8)CDSP_CMD_OS2HOST_READY_MIN <= bCmdId)
		/* Ready */
		return;

	if ((UINT8)CDSP_CMD_OS2HOST_CMN_MAX >= bCmdId)
		/* Common Command */
		switch (bCmdId) {
		case CDSP_CMD_OS2HOST_CMN_NOTIFY_OUT_FORMAT:
			NotifyOutFormatOs2Host(dCoderId);
			break;

		default:
			/* DEC/ENC_SFR1 Write complete */
			psDecInfo->sParams.abParam[CDSP_CMD_PARAM_RESULT_00]
									= 0;
			psDecInfo->sParams.abParam[CDSP_CMD_PARAM_RESULT_01]
									= 0;
			psDecInfo->sParams.abParam[CDSP_CMD_PARAM_RESULT_02]
									= 0;
			psDecInfo->sParams.abParam[CDSP_CMD_PARAM_RESULT_03]
									= 0;
			CommandWriteComplete(dCoderId, &psDecInfo->sParams);
			break;
		}
	else if ((UINT8)CDSP_CMD_OS2HOST_PRG_MAX >= bCmdId)
		/* Callback (HOST COMMAND) */
		psDecInfo->sCbInfo.abCbOn[CALLBACK_HOSTCOMMAND] = CALLBACK_ON;
}

/****************************************************************************
 *	InterruptProcDecEvt
 *
 *	Function:
 *			Decoder/Encoder EVT Interrupt.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDecEvt(UINT32 dCoderId)
{
	UINT8 bAdd;
	UINT8 bEvtData;
	struct DEC_INFO *pDecInfo;

	/* Read EVT data */
	if (CODER_DEC == dCoderId)
		bAdd = MCI_DEC_EVT;
	else
		bAdd = MCI_ENC_EVT;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)bAdd), &bEvtData, 1);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)bAdd),
						0x00);

	McDevIf_ExecutePacket();

	/* Timer Event */
	if (0 != ((UINT8)EVENT_TIMER & bEvtData)) {
		/* Callback (TIMER) */
		pDecInfo = GetDecInfo(dCoderId);
		pDecInfo->sCbInfo.abCbOn[CALLBACK_TIMER] = CALLBACK_ON;
	}
}

/****************************************************************************
 *	DecStop
 *
 *	Function:
 *			Stop decoder/encoder.
 *	Arguments:
 *			dCoderId	Coder ID
 *			bVerifyComp	Verify Stop Complete command flag
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DecStop(UINT32 dCoderId, UINT8 bVerifyComp)
{
	UINT8 bData;
	UINT8 bIntData;
	UINT8 bAddInt;
	UINT32 dFifoId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	if (STATE_PLAYING != psDecInfo->dState)
		return MCDRV_ERROR_STATE;

	dFifoId = GetFifoIdCore(&psDecInfo->sConnect);
	if ((FIFO_EFIFO & dFifoId) != FIFO_NONE) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_CH), &bData, 1);
		bData &= ~MCB_DEC_EFIFO_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_CH),
					bData);
		McDevIf_ExecutePacket();
	}
	if (((FIFO_DFIFO & dFifoId) != FIFO_NONE)
		&& (GetDFifoSel() == CDSP_FIFO_SEL_PORT)) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL), &bData,
					1);
		bData &= ~MCB_RDFIFO_DFIFO_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					bData);
		McDevIf_ExecutePacket();
	}

	/* DEC/ENC Stop */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_START), &bData, 1);
	if (CODER_DEC == dCoderId) {
		bData &= ~MCB_DEC_DEC_START;
		bAddInt = MCI_DEC_ENABLE;
	} else {
		bData &= ~MCB_DEC_ENC_START;
		bAddInt = MCI_ENC_ENABLE;
	}
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), bData);

	McDevIf_ExecutePacket();

	/* VerifyStopCompletion command */
	if (MADEVCDSP_VERIFY_COMP_ON == bVerifyComp) {
		sParam.bCommandId =
				(UINT8)CDSP_CMD_HOST2OS_SYS_VERIFY_STOP_COMP;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);
		if ((SINT32)MCDRV_SUCCESS <= sdRet)
			sdRet = MCDRV_SUCCESS;
	} else
		sdRet = MCDRV_SUCCESS;

	/* DEC/ENC END,ERR Interrupt Disable */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)bAddInt), &bIntData, 1);
	bIntData &= ~(MCB_EDEC_END | MCB_EDEC_ERR);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)bAddInt),
						bIntData);

	McDevIf_ExecutePacket();

	return sdRet;
}

/***************************************************************************
 *	FifoStop
 *
 *	Function:
 *			Stop FIFO.
 *	Arguments:
 *			dFifoId		FIFO ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 FifoStop(UINT32 dCoderId)
{
	UINT8 bData;
	UINT32 dFifoId;

	dFifoId = GetFifoId(dCoderId);

	if (FIFO_NONE != (FIFO_DFIFO_MASK & dFifoId)) {
		/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt Disable */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_ENABLE),
					0x00);

		McDevIf_ExecutePacket();

		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
		bData &= ~MCB_EDFIFO;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

		McDevIf_ExecutePacket();
	}

	if (FIFO_NONE != (FIFO_EFIFO_MASK & dFifoId)) {
		/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt Disable */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_ENABLE),
					0x00);

		McDevIf_ExecutePacket();

		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
		bData &= ~MCB_EEFIFO;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

		McDevIf_ExecutePacket();
	}

	if (FIFO_NONE != (FIFO_OFIFO_MASK & dFifoId)) {
		/* OUT Stop */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_START),
						&bData, 1);

		bData &= ~MCB_DEC_OUT_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), bData);

		/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt Disable */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_ENABLE),
					0x00);

		McDevIf_ExecutePacket();

		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
		bData &= ~MCB_EOFIFO;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

		McDevIf_ExecutePacket();
	}

	if (FIFO_NONE != (FIFO_RFIFO_MASK & dFifoId)) {
		if (GetRFifoSel() == CDSP_FIFO_SEL_PORT) {
			/* OUT Stop */
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_START2),
						&bData, 1);

			bData &= ~MCB_RFIFO_START;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START2), bData);
		}

		/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt Disable */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE),
					0x00);

		McDevIf_ExecutePacket();

		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
		bData &= ~MCB_ERFIFO;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

		McDevIf_ExecutePacket();
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ConvertSamplesToTime
 *
 *	Function:
 *			Convert sample count to milli-second.
 *	Arguments:
 *			bFs		Fs ID
 *			dSamples	Sample count
 *	Return:
 *			milli-second
 *
 ****************************************************************************/
static UINT32 ConvertSamplesToTime(UINT8 bFs, UINT32 dSamples)
{
	UINT32 dFs;
	UINT32 dTime;

	switch (bFs) {
	case CODER_FMT_FS_48000:
		dFs = OUTPUT_FS_48000;
		break;

	case CODER_FMT_FS_44100:
		dFs = OUTPUT_FS_44100;
		break;

	case CODER_FMT_FS_32000:
		dFs = OUTPUT_FS_32000;
		break;

	case CODER_FMT_FS_24000:
		dFs = OUTPUT_FS_24000;
		break;

	case CODER_FMT_FS_22050:
		dFs = OUTPUT_FS_22050;
		break;

	case CODER_FMT_FS_16000:
		dFs = OUTPUT_FS_16000;
		break;

	case CODER_FMT_FS_12000:
		dFs = OUTPUT_FS_12000;
		break;

	case CODER_FMT_FS_11025:
		dFs = OUTPUT_FS_11025;
		break;

	case CODER_FMT_FS_8000:
		dFs = OUTPUT_FS_8000;
		break;

	default:
		dFs = OUTPUT_FS_DEF;
		break;
	}

	dTime = (dSamples / dFs) * 1000UL;
	dTime += (((dSamples % dFs) * 1000UL) + (dFs - 1UL)) / dFs;

	return dTime;
}

/****************************************************************************
 *	GetOutputPosition
 *
 *	Function:
 *			Get present output position (unit of ms). 
 *	Arguments:
 *			dCoderId	Coder ID
 *			pdPos		Pointer of output position
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 GetOutputPosition(UINT32 dCoderId, UINT32 *pdPos)
{
	UINT32 dOutputPos;
	UINT8 abOutputPos[4];
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* Path check */
	switch (psDecInfo->sConnect.bOutDest) {
	case CDSP_OUT_DEST_RFIFO:
	case CDSP_OUT_DEST_OTHER_INBUF:
	case CDSP_OUT_DEST_NONE:
		return MCDRV_ERROR;

	default:
		break;
	}

	/* DEC_POS Read */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_POS1), &abOutputPos[3], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_POS2), &abOutputPos[2], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_POS3), &abOutputPos[1], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_POS4), &abOutputPos[0], 1);

	dOutputPos = ((UINT32)abOutputPos[3] << 24)
				| ((UINT32)abOutputPos[2] << 16)
				| ((UINT32)abOutputPos[1] << 8)
				| (UINT32)abOutputPos[0];

	/* sample -> msec */
	*pdPos = ConvertSamplesToTime(psDecInfo->sFormat.bFs, dOutputPos);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	InterruptProcDecErr
 *
 *	Function:
 *			Decoder/Encoder ERR Interrupt.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDecErr(UINT32 dCoderId)
{
	UINT8 bAdd;
	UINT8 bData;
	SINT32 sdRet;
	UINT32 dPos;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *psOtherDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);
	psOtherDecInfo = GetOtherDecInfo(dCoderId);

	/* Read ERR data */
	if (CODER_DEC == dCoderId)
		bAdd = MCI_DEC_ERROR;
	else
		bAdd = MCI_ENC_ERROR;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)bAdd), &bData, 1);

	psDecInfo->wErrorCode = (UINT16)bData;

	dPos = 0;
	GetOutputPosition(dCoderId, &dPos);

	if (DEC_ERR_PROG_SPECIFIC_MAX < psDecInfo->wErrorCode) {
		/* Stop */
		FifoStop(dCoderId);
		sdRet = DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_OFF);

		/* Reset command */
		sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_RESET;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);

		/* Command register Initialize */
		CommandInitialize(dCoderId);

		/* EDEC/EENC_SFR Disable */
		if (CODER_DEC == dCoderId) {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_ENABLE),
						&bData, 1);
			bData &= ~MCB_EDEC_SFR;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_ENABLE),
						bData);
		} else {
			McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_ENC_ENABLE),
						&bData, 1);
			bData &= ~MCB_EENC_SFR;
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_ENC_ENABLE),
						bData);
		}
		McDevIf_ExecutePacket();

		/* EDEC/EENC Disable */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
							| (UINT32)MCI_ECDSP),
							&bData, 1);
		if (CODER_DEC == dCoderId)
			bData &= ~MCB_EDEC;
		else
			bData &= ~MCB_EENC;

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						bData);
		McDevIf_ExecutePacket();

		/* Parameter Initialize */
		psDecInfo->sProgInfo.wVendorId = 0;
		psDecInfo->sProgInfo.wFunctionId = 0;

		psDecInfo->dState = STATE_READY_SETUP;

		/* Callback (ERROR) */
		psDecInfo->sCbInfo.abCbOn[CALLBACK_PLAY_ERROR2] = CALLBACK_ON;
		psDecInfo->sCbInfo.adCbExInfo[CALLBACK_PLAY_ERROR2] =
			(dPos << 8) | (UINT32)(psDecInfo->wErrorCode & 0xFF);
	} else {
		FifoStop(dCoderId);
		DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_OFF);

		psDecInfo->dState = STATE_READY;

		/* Callback (ERROR) */
		psDecInfo->sCbInfo.abCbOn[CALLBACK_PLAY_ERROR1] = CALLBACK_ON;
		psDecInfo->sCbInfo.adCbExInfo[CALLBACK_PLAY_ERROR1] =
			(dPos << 8) | (UINT32)(psDecInfo->wErrorCode & 0xFF);
	}
}

/***************************************************************************
 *	OutputStart
 *
 *	Function:
 *			Start OFIFO.
 *	Arguments:
 *			psDecInfo	CDSP decoder/encoder
 *					management module's information
 *	Return:
 *			None
 *
 ****************************************************************************/
static void OutputStart(struct DEC_INFO *psDecInfo)
{
	UINT8 bOutDest;

	bOutDest = psDecInfo->sConnect.bOutDest;
	switch (bOutDest) {
	case CDSP_OUT_DEST_OFIFO:
		break;

	case CDSP_OUT_DEST_OFIFO_RFIFO:
		if (GetRFifoSel() != CDSP_FIFO_SEL_PORT)
			bOutDest = CDSP_OUT_DEST_OFIFO;
		break;

	case CDSP_OUT_DEST_RFIFO:
		if (GetRFifoSel() != CDSP_FIFO_SEL_PORT)
			return;
		break;

	default:
		return;
	}

	if ((psDecInfo->sProgInfo.wOutStartMode != 0)
		&& (psDecInfo->bChangeOutputFs == CHANGE_OUTPUT_FS_OFF))
		return;

	switch (bOutDest) {
	case CDSP_OUT_DEST_OFIFO:
		if (gsFifoInfo.bOFifoOutStart != OUTPUT_START_ON)
			return;

		OFifoStartReal();
		break;

	case CDSP_OUT_DEST_OFIFO_RFIFO:
		if ((gsFifoInfo.bOFifoOutStart != OUTPUT_START_ON)
			|| (gsFifoInfo.bRFifoOutStart != OUTPUT_START_ON))
			return;

		OFifoStartReal();
		RFifoStartReal();
		break;

	case CDSP_OUT_DEST_RFIFO:
		if (gsFifoInfo.bRFifoOutStart != OUTPUT_START_ON)
			return;

		RFifoStartReal();
		break;

	default:
		break;
	}
}

/****************************************************************************
 *	InterruptProcDecEnd
 *
 *	Function:
 *			Decoder/Encoder END Interrupt.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDecEnd(UINT32 dCoderId)
{
	UINT8 bData;
	UINT32 dInputPos;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *psOtherDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);
	psOtherDecInfo = GetOtherDecInfo(dCoderId);

	if (psDecInfo->bFmtPropagate == FORMAT_PROPAGATE_ON) {
		psDecInfo->sFormat.bFs = psOtherDecInfo->sFormat.bFs;

		/* DEC/ENC Stop */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), &bData, 1);
		if (CODER_DEC == psDecInfo->dCoderId)
			bData &= ~MCB_DEC_DEC_START;
		else
			bData &= ~MCB_DEC_ENC_START;

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), bData);
		McDevIf_ExecutePacket();

		/* VerifyStopCompletion command */
		sParam.bCommandId =
				(UINT8)CDSP_CMD_HOST2OS_SYS_VERIFY_STOP_COMP;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);

		/* GetInputPos command */
		sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_GET_INPUT_POS;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);

		/* InputPos */
		dInputPos = CreateUINT32(
				sParam.abParam[CDSP_CMD_PARAM_RESULT_03],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_02],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_01],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_00]);

		psDecInfo->dInPosSup += dInputPos;

		/* Clear command */
		sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_CLEAR;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);

		/* DEC/ENC Start */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), &bData, 1);
		if (CODER_DEC == psDecInfo->dCoderId)
			bData |= MCB_DEC_DEC_START;
		else
			bData |= MCB_DEC_ENC_START;

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_START),
						bData);
		McDevIf_ExecutePacket();

		psDecInfo->bFmtPropagate = FORMAT_PROPAGATE_OFF;

		/* Callback (HOST Command) */
		psOtherDecInfo->sCbInfo.abCbOn[CALLBACK_HOSTCOMMAND] =
								CALLBACK_ON;
	} else {
		switch (psOtherDecInfo->dState) {
		case STATE_READY_SETUP:
			psOtherDecInfo->bPreInputDataEnd = INPUT_DATAEND_SET;
			break;

		case STATE_READY:
		case STATE_PLAYING:
			if (CDSP_IN_SOURCE_OTHER_OUTBUF ==
					psOtherDecInfo->sConnect.bInSource) {
				/* InputDataEnd command */
				CommandInputDataEnd(psOtherDecInfo->dCoderId);
				/* Input data end flag - set */
				psOtherDecInfo->bInputDataEnd =
							INPUT_DATAEND_SET;
				/* OutputStart */
				OutputStart(psOtherDecInfo);
			}
			break;

		default:
			break;
		}

		/* ClearInputDataEnd command */
		sParam.bCommandId =
			(UINT8)CDSP_CMD_HOST2OS_SYS_CLEAR_INPUT_DATA_END;
		sdRet = CommandWriteHost2Os(dCoderId, &sParam);

		/* Input data end flag - release */
		psDecInfo->bInputDataEnd = INPUT_DATAEND_RELEASE;

		FifoStop(dCoderId);
		DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_ON);

		psDecInfo->dState = STATE_READY;

		/* Callback (DEC/ENC END) */
		psDecInfo->sCbInfo.abCbOn[CALLBACK_END_OF_SEQUENCE] =
								CALLBACK_ON;
	}
}

/****************************************************************************
 *	InterruptProcDec
 *
 *	Function:
 *			Decoder/Encoder interrupt function.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDec(UINT32 dCoderId)
{
	UINT8 bData;
	UINT8 bClear;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* Read interrupt flag */
	if (CODER_DEC == dCoderId) {
		McDevIf_ReadDirect(((UINT32)MCI_DEC_FLG
					| (UINT32)MCDRV_PACKET_REGTYPE_C),
					&bData, 1);
		bClear = 0;
		if (0 != (MCB_DEC_FLG_SFR & bData)) {
			InterruptProcDecSfr(dCoderId);
			bClear |= MCB_DEC_FLG_SFR;
		}
		if (0 != (MCB_DEC_FLG_ERR & bData)) {
			InterruptProcDecErr(dCoderId);
			bClear |= MCB_DEC_FLG_ERR;
		}
		if ((0 != (MCB_DEC_FLG_END & bData))
			&& (STATE_PLAYING == psDecInfo->dState)) {
			InterruptProcDecEnd(dCoderId);
			bClear |= MCB_DEC_FLG_END;
		}
		if (0 != (MCB_DEC_EVT_FLG & bData)) {
			InterruptProcDecEvt(dCoderId);
			bClear |= MCB_DEC_EVT_FLG;
		}
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FLG),
						bClear);

		McDevIf_ExecutePacket();
	} else {
		McDevIf_ReadDirect(((UINT32)MCI_ENC_FLG
					| (UINT32)MCDRV_PACKET_REGTYPE_C),
					&bData, 1);
		bClear = 0;
		if (0 != (MCB_ENC_FLG_SFR & bData)) {
			InterruptProcDecSfr(dCoderId);
			bClear |= MCB_ENC_FLG_SFR;
		}
		if (0 != (MCB_ENC_FLG_ERR & bData)) {
			InterruptProcDecErr(dCoderId);
			bClear |= MCB_ENC_FLG_ERR;
		}
		if ((0 != (MCB_ENC_FLG_END & bData))
			&& (STATE_PLAYING == psDecInfo->dState)) {
			InterruptProcDecEnd(dCoderId);
			bClear |= MCB_ENC_FLG_END;
		}
		if (0 != (MCB_ENC_EVT_FLG & bData)) {
			InterruptProcDecEvt(dCoderId);
			bClear |= MCB_ENC_EVT_FLG;
		}
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_ENC_FLG),
						bClear);

		McDevIf_ExecutePacket();
	}
}

/***************************************************************************
 *	InterruptProcDFifo
 *
 *	Function:
 *			DFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcDFifo(void)
{
	UINT8 bIntFlg;
	UINT8 bIntCtrl;
	UINT32 i;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *pasDecInfo[CODER_NUM];

	/* Read interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_FLG), &bIntFlg, 1);

	/* Interrupt process */
	/* EDPNT, EDEMP Read */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_ENABLE), &bIntCtrl, 1);

	psDecInfo = NULL;
	pasDecInfo[CODER_DEC] = &gsDecInfo;
	pasDecInfo[CODER_ENC] = &gsEncInfo;
	for (i = 0; i < (UINT32)CODER_NUM; i++) {
		switch (pasDecInfo[i]->sConnect.bInSource) {
		case CDSP_IN_SOURCE_DFIFO:
		case CDSP_IN_SOURCE_DFIFO_EFIFO:
			if ((UINT32)STATE_INIT < pasDecInfo[i]->dState)
				psDecInfo = pasDecInfo[i];
			break;

		default:
			break;
		}

		if (NULL != psDecInfo)
			break;
	}

	/* DPNT */
	if ((MCB_DFIFO_FLG_DPNT == (bIntFlg & MCB_DFIFO_FLG_DPNT))
		&& (MCB_DFIFO_EDPNT == (bIntCtrl & MCB_DFIFO_EDPNT))) {
		/* EDPNT Disable */
		bIntCtrl &= ~MCB_DFIFO_EDPNT;

		/* Callback (DFIFO POINT) */
		if (NULL != psDecInfo)
			psDecInfo->sCbInfo.abCbOn[CALLBACK_DFIFOPOINT] =
								CALLBACK_ON;
	}

	/* DEMP */
	if ((MCB_DFIFO_FLG_DEMP == (bIntFlg & MCB_DFIFO_FLG_DEMP))
		&& (MCB_DFIFO_EDEMP == (bIntCtrl & MCB_DFIFO_EDEMP))) {
		/* EDEMP Disable */
		bIntCtrl &= ~MCB_DFIFO_EDEMP;

		/* Callback (DFIFO EMPTY) */
		if (NULL != psDecInfo)
			psDecInfo->sCbInfo.abCbOn[CALLBACK_DFIFOEMPTY] =
								CALLBACK_ON;
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DFIFO_ENABLE),
						bIntCtrl);

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DFIFO_FLG),
						bIntFlg);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	InterruptProcOFifoCore
 *
 *	Function:
 *			OFIFO interrupt function Core.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcOFifoCore(void)
{
	UINT8 bData;
	UINT32 i;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *pasDecInfo[CODER_NUM];

	/* EOPNT Disable */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_OFIFO_ENABLE), &bData, 1);
	bData &= ~MCB_OFIFO_EOPNT;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_OFIFO_ENABLE),
						bData);

	/* EOFIFO Disable */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						&bData, 1);
	bData &= ~MCB_EOFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						bData);

	McDevIf_ExecutePacket();

	/* OUT START */
	gsFifoInfo.bOFifoOutStart = OUTPUT_START_ON;

	psDecInfo = NULL;
	pasDecInfo[CODER_DEC] = &gsDecInfo;
	pasDecInfo[CODER_ENC] = &gsEncInfo;
	for (i = 0; i < (UINT32)CODER_NUM; i++) {
		switch (pasDecInfo[i]->sConnect.bOutDest) {
		case CDSP_OUT_DEST_OFIFO:
		case CDSP_OUT_DEST_OFIFO_RFIFO:
			if ((UINT32)STATE_INIT < pasDecInfo[i]->dState)
				psDecInfo = pasDecInfo[i];
			break;

		default:
			break;
		}

		if (NULL != psDecInfo)
			break;
	}

	/* OUT_START Set */
	if (NULL != psDecInfo)
		OutputStart(psDecInfo);
}

/***************************************************************************
 *	InterruptProcOFifo
 *
 *	Function:
 *			OFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcOFifo(void)
{
	UINT8 bIntFlg;

	/* Read interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)MCI_OFIFO_FLG),
								&bIntFlg, 1);

	/* Interrupt process */
	if (MCB_OFIFO_FLG_OPNT == (MCB_OFIFO_FLG_OPNT & bIntFlg))
		InterruptProcOFifoCore();

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_OFIFO_FLG),
						bIntFlg);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	InterruptProcRFifoPortCore
 *
 *	Function:
 *			RFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcRFifoPortCore(void)
{
	UINT8 bData;
	UINT32 i;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *pasDecInfo[CODER_NUM];

	/* ERPNT Disable */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_RFIFO_ENABLE), &bData, 1);
	bData &= ~MCB_RFIFO_ERPNT;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_RFIFO_ENABLE),
						bData);

	/* ERFIFO Disable */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						&bData, 1);
	bData &= ~MCB_ERFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						bData);

	McDevIf_ExecutePacket();

	/* OUT START */
	gsFifoInfo.bRFifoOutStart = OUTPUT_START_ON;

	psDecInfo = NULL;
	pasDecInfo[CODER_DEC] = &gsDecInfo;
	pasDecInfo[CODER_ENC] = &gsEncInfo;
	for (i = 0; i < (UINT32)CODER_NUM; i++) {
		switch (pasDecInfo[i]->sConnect.bOutDest) {
		case CDSP_OUT_DEST_RFIFO:
		case CDSP_OUT_DEST_OFIFO_RFIFO:
			if ((UINT32)STATE_INIT < pasDecInfo[i]->dState)
				psDecInfo = pasDecInfo[i];
			break;

		default:
			break;
		}

		if (NULL != psDecInfo)
			break;
	}

	/* OUT_START Set */
	if (NULL != psDecInfo)
		OutputStart(psDecInfo);
}

/***************************************************************************
 *	InterruptProcRFifoPort
 *
 *	Function:
 *			RFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcRFifoPort(void)
{
	UINT8 bIntFlg;

	/* Read interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)MCI_RFIFO_FLG),
								&bIntFlg, 1);

	/* Interrupt process */
	if (MCB_OFIFO_FLG_OPNT == (MCB_OFIFO_FLG_OPNT & bIntFlg))
		InterruptProcRFifoPortCore();

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_RFIFO_FLG),
						bIntFlg);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	InterruptProcRFifoHost
 *
 *	Function:
 *			RFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcRFifoHost(void)
{
	UINT8 bIntFlg;
	UINT8 bIntCtrl;
	UINT32 i;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *pasDecInfo[CODER_NUM];

	/* Read interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)MCI_RFIFO_FLG),
								&bIntFlg, 1);

	/* Interrupt process */

	/* ERxxx Read */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)MCI_RFIFO_ENABLE),
								&bIntCtrl, 1);

	if (MCB_RFIFO_FLG_RPNT == (MCB_RFIFO_FLG_RPNT & bIntFlg)) {
		psDecInfo = NULL;
		pasDecInfo[CODER_DEC] = &gsDecInfo;
		pasDecInfo[CODER_ENC] = &gsEncInfo;

		for (i = 0; i < (UINT32)CODER_NUM; i++) {
			switch (pasDecInfo[i]->sConnect.bOutDest) {
			case CDSP_OUT_DEST_RFIFO:
			case CDSP_OUT_DEST_OFIFO_RFIFO:
				if ((UINT32)STATE_INIT < pasDecInfo[i]->dState)
					psDecInfo = pasDecInfo[i];
				break;

			default:
				break;
			}

			if (NULL != psDecInfo)
				break;
		}

		if (MCB_RFIFO_ERPNT == (MCB_RFIFO_ERPNT & bIntCtrl)) {
			/* ERPNT Disable */
			bIntCtrl &= ~MCB_RFIFO_ERPNT;

			/* Callback (RFIFO POINT) */
			if (NULL != psDecInfo)
				psDecInfo->sCbInfo.abCbOn[CALLBACK_RFIFOPOINT]
					= CALLBACK_ON;
		}

		if ((MCB_RFIFO_FLG_ROVF == (MCB_RFIFO_FLG_ROVF & bIntFlg))
			&& (MCB_RFIFO_EROVF == (MCB_RFIFO_EROVF & bIntCtrl))) {
			/* EROVF Disable */
			bIntCtrl &= ~MCB_RFIFO_EROVF;

			/* Callback (RecBuf Overflow) */
			if (NULL != psDecInfo)
				psDecInfo->sCbInfo.abCbOn[CALLBACK_RECBUF_OVF]
					= CALLBACK_ON;
		}

		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE), bIntCtrl);
	}

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_RFIFO_FLG), bIntFlg);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	InterruptProcRFifo
 *
 *	Function:
 *			RFIFO interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcRFifo(void)
{
	if (GetRFifoSel() == CDSP_FIFO_SEL_PORT)
		InterruptProcRFifoPort();
	else
		InterruptProcRFifoHost();
}

/****************************************************************************
 *	InterruptProcCDsp
 *
 *	Function:
 *			CDSP interrupt function.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void InterruptProcCDsp(void)
{
	UINT8 bDatErr;
	UINT8 bData;

	/* Read interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C | (UINT32)MCI_CDSP_FLG),
								&bData, 1);

	/* Interrupt process */
	if (MCB_CDSP_FLG_ERR == (MCB_CDSP_FLG_ERR & bData)) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_ERR),
						&bDatErr, 1);

		gsCdspInfo.wHwErrorCode = (UINT16)bDatErr;
	} else if (MCB_CDSP_FLG_WDT == (MCB_CDSP_FLG_WDT & bData))
		gsCdspInfo.wHwErrorCode = CDSP_ERR_WDT;

	/* Interrupt ALL disable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_ENC_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DFIFO_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_OFIFO_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_EFIFO_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_RFIFO_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_FFIFO_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_ENABLE),
						0x00);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_IF
						| (UINT32)MCI_ECDSP),
						0x00);

	/* State update */
	gsDecInfo.dState = STATE_NOTINIT;
	gsEncInfo.dState = STATE_NOTINIT;

	/* Callback (HW ERROR) */
	gsDecInfo.sCbInfo.abCbOn[CALLBACK_HW_ERROR] = CALLBACK_ON;
	gsDecInfo.sCbInfo.adCbExInfo[CALLBACK_HW_ERROR] =
				(UINT32)(gsCdspInfo.wHwErrorCode & 0xFF);
	gsEncInfo.sCbInfo.abCbOn[CALLBACK_HW_ERROR] = CALLBACK_ON;
	gsEncInfo.sCbInfo.adCbExInfo[CALLBACK_HW_ERROR] =
				(UINT32)(gsCdspInfo.wHwErrorCode & 0xFF);

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_FLG),
						bData);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	CallbackProcCore
 *
 *	Function:
 *			Callback process.
 *	Arguments:
 *			psCbInfo	Pointer of Callback info
 *			sdHd		Callback handle
 *	Return:
 *			None
 *
 ****************************************************************************/
static void CallbackProcCore(struct CALLBACK_INFO *psCbInfo, SINT32 sdHd)
{
	if (psCbInfo->pcbFunc != NULL) {
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_HOSTCOMMAND])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_PARAM, 0);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_END_OF_SEQUENCE])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_END, 0);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_DFIFOPOINT])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_FIFO, 0);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_RFIFOPOINT])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_FIFO, 1);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_DFIFOEMPTY])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_FIFO, 2);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_RECBUF_OVF])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_FIFO, 3);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_TIMER])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_TIMER, 0);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_PLAY_ERROR1])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_ERROR,
				psCbInfo->adCbExInfo[CALLBACK_PLAY_ERROR1]);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_PLAY_ERROR2])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_ERROR,
				psCbInfo->adCbExInfo[CALLBACK_PLAY_ERROR2]);
		if (CALLBACK_ON == psCbInfo->abCbOn[CALLBACK_HW_ERROR])
			psCbInfo->pcbFunc(sdHd, MCDRV_CDSP_EVT_ERROR,
				psCbInfo->adCbExInfo[CALLBACK_HW_ERROR]);
	}

	psCbInfo->abCbOn[CALLBACK_HOSTCOMMAND] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_END_OF_SEQUENCE] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_DFIFOPOINT] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_RFIFOPOINT] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_DFIFOEMPTY] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_RECBUF_OVF] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_TIMER] = CALLBACK_OFF;
	psCbInfo->abCbOn[CALLBACK_PLAY_ERROR1] = CALLBACK_OFF;
	psCbInfo->adCbExInfo[CALLBACK_PLAY_ERROR1] = 0;
	psCbInfo->abCbOn[CALLBACK_PLAY_ERROR2] = CALLBACK_OFF;
	psCbInfo->adCbExInfo[CALLBACK_PLAY_ERROR2] = 0;
	psCbInfo->abCbOn[CALLBACK_HW_ERROR] = CALLBACK_OFF;
	psCbInfo->adCbExInfo[CALLBACK_HW_ERROR] = 0;

}

/****************************************************************************
 *	CallbackProc
 *
 *	Function:
 *			Callback process.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void CallbackProc(void)
{
	CallbackProcCore(&(gsDecInfo.sCbInfo), 0);
	CallbackProcCore(&(gsEncInfo.sCbInfo), 1);
}

/****************************************************************************
 *	GetCDSPChunk
 *
 *	Function:
 *			Get chunk of CDSP
 *	Arguments:
 *			psPrm		Pointer of AEC Data
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void GetCDSPChunk(struct MCDRV_AEC_INFO *psPrm,
				struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	struct AEC_CDSP_FUNC_INFO *psFuncInfoA;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoB;

	psFuncInfoA = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A];
	psFuncInfoB = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B];

	psFuncInfoA->dCoderId = CODER_DEC;
	psFuncInfoA->bFuncOnOff = CDSP_FUNC_OFF;
	psFuncInfoA->pbChunk = NULL;
	psFuncInfoA->dChunkSize = 0;

	psFuncInfoB->dCoderId = CODER_ENC;
	psFuncInfoB->bFuncOnOff = CDSP_FUNC_OFF;
	psFuncInfoB->pbChunk = NULL;
	psFuncInfoB->dChunkSize = 0;

	if (psPrm->sAecVBox.bEnable == AEC_VBOX_ENABLE) {
		psFuncInfoA->bFuncOnOff = psPrm->sAecVBox.bCDspFuncAOnOff;
		psFuncInfoB->bFuncOnOff = psPrm->sAecVBox.bCDspFuncBOnOff;

		if ((psFuncInfoA->bFuncOnOff == CDSP_FUNC_OFF)
			&& (psFuncInfoB->bFuncOnOff == CDSP_FUNC_OFF))
			return;

		if (psFuncInfoA->bFuncOnOff == CDSP_FUNC_ON) {
			psFuncInfoA->pbChunk =
				psPrm->sAecVBox.sAecCDspA.pbChunkData;
			psFuncInfoA->dChunkSize =
				psPrm->sAecVBox.sAecCDspA.dwSize;
		}

		if (psFuncInfoB->bFuncOnOff == CDSP_FUNC_ON) {
			psFuncInfoB->pbChunk =
				psPrm->sAecVBox.sAecCDspB.pbChunkData;
			psFuncInfoB->dChunkSize =
				psPrm->sAecVBox.sAecCDspB.dwSize;
		}
	}

	if (psFuncInfoA->pbChunk != NULL)
		psFuncInfoA->pbChunk = &psFuncInfoA->pbChunk[CHUNK_SIZE];

	if (psFuncInfoB->pbChunk != NULL)
		psFuncInfoB->pbChunk = &psFuncInfoB->pbChunk[CHUNK_SIZE];

	return;
}

/****************************************************************************
 *	CheckFIFO
 *
 *	Function:
 *			Check fifo
 *	Arguments:
 *			dCoderId	Coder ID
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CheckFIFO(UINT32 dCoderId,
				struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	UINT32 dPos;
	UINT32 dSamples;
	UINT8 bData;
	UINT8 bFifo;
	UINT8 bInSource;
	UINT8 bOutDest;
	UINT8 bBit;
	UINT8 bCh;
	UINT8 bE2BufMode;
	UINT8 *pbPrm;
	UINT32 dFifoId;
	struct AEC_CDSP_FUNC_INFO *psFuncInfo;

	if (dCoderId == CODER_DEC)
		psFuncInfo = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A];
	else
		psFuncInfo = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B];

	pbPrm = psFuncInfo->pbFifo;

	bFifo = pbPrm[CDSP_USE_FIFO];
	if (bFifo != CDSP_FIFO_DONTCARE) {
		if ((bFifo & CDSP_FIFO_OTHER_OUTBUF_BIT) != 0)
			if (((bFifo & CDSP_FIFO_EFIFO_BIT) != 0)
				|| ((bFifo & CDSP_FIFO_DFIFO_BIT) != 0))
				return MCDRV_ERROR_ARGUMENT;

		if ((bFifo & CDSP_FIFO_OTHER_INBUF_BIT) != 0)
			if (((bFifo & CDSP_FIFO_OFIFO_BIT) != 0)
				|| ((bFifo & CDSP_FIFO_RFIFO_BIT) != 0))
				return MCDRV_ERROR_ARGUMENT;

		if ((bFifo & CDSP_FIFO_OTHER_OUTBUF_BIT) != 0)
			bInSource = CDSP_IN_SOURCE_OTHER_OUTBUF;
		else if (((bFifo & CDSP_FIFO_EFIFO_BIT) != 0)
			&& ((bFifo & CDSP_FIFO_DFIFO_BIT) != 0))
			bInSource = CDSP_IN_SOURCE_DFIFO_EFIFO;
		else if ((bFifo & CDSP_FIFO_DFIFO_BIT) != 0)
			bInSource = CDSP_IN_SOURCE_DFIFO;
		else if ((bFifo & CDSP_FIFO_EFIFO_BIT) != 0)
			bInSource = CDSP_IN_SOURCE_EFIFO;
		else
			bInSource = CDSP_IN_SOURCE_NONE;

		psFuncInfo->sConnect.bInSource = bInSource;

		if ((bFifo & CDSP_FIFO_OTHER_INBUF_BIT) != 0)
			bOutDest = CDSP_OUT_DEST_OTHER_INBUF;
		else if (((bFifo & CDSP_FIFO_OFIFO_BIT) != 0)
			&& ((bFifo & CDSP_FIFO_RFIFO_BIT) != 0))
			bOutDest = CDSP_OUT_DEST_OFIFO_RFIFO;
		else if ((bFifo & CDSP_FIFO_OFIFO_BIT) != 0)
			bOutDest = CDSP_OUT_DEST_OFIFO;
		else if ((bFifo & CDSP_FIFO_RFIFO_BIT) != 0)
			bOutDest = CDSP_OUT_DEST_RFIFO;
		else
			bOutDest = CDSP_OUT_DEST_NONE;

		psFuncInfo->sConnect.bOutDest = bOutDest;
	}

	dFifoId = GetFifoIdCore(&psFuncInfo->sConnect);

	/* EFIFO */
	if ((dFifoId & FIFO_EFIFO) != 0) {
		if (pbPrm[ROUTE_EFIFO0_SEL] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bEFifo01Sel & ~MCB_EFIFO0_SEL);
			bData |= pbPrm[ROUTE_EFIFO0_SEL] & MCB_EFIFO0_SEL;
			psCdspInfo->bEFifo01Sel = bData;
		}

		if (pbPrm[ROUTE_EFIFO1_SEL] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bEFifo01Sel & ~MCB_EFIFO1_SEL);
			bData |= (pbPrm[ROUTE_EFIFO1_SEL] << 4) &
							MCB_EFIFO1_SEL;
			psCdspInfo->bEFifo01Sel = bData;
		}

		if (pbPrm[ROUTE_EFIFO2_SEL] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bEFifo23Sel & ~MCB_EFIFO2_SEL);
			bData |= pbPrm[ROUTE_EFIFO2_SEL] & MCB_EFIFO2_SEL;
			psCdspInfo->bEFifo23Sel = bData;
		}

		if (pbPrm[ROUTE_EFIFO3_SEL] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bEFifo23Sel & ~MCB_EFIFO3_SEL);
			bData |= (pbPrm[ROUTE_EFIFO3_SEL] << 4) &
							MCB_EFIFO3_SEL;
			psCdspInfo->bEFifo23Sel = bData;
		}

		/* 0: 2ch 16bit */
		/* 1: 4ch 16bit */
		/* 2: 2ch 32bit   */
		/* 3: 4ch 32bit */
		bCh = psFuncInfo->sConnectEx.bEfifoCh;
		bBit = psFuncInfo->sBitWidth.bEfifoBit;
		bE2BufMode = psFuncInfo->sFormat.bE2BufMode;

		switch (pbPrm[CDSP_EFIFO_CH]) {
		case 2:
		case 4:
			bCh = pbPrm[CDSP_EFIFO_CH];
			break;

		case CDSP_FIFO_DONTCARE:
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}

		switch (pbPrm[CDSP_EFIFO_BIT_WIDTH]) {
		case 16:
		case 32:
			bBit = pbPrm[CDSP_EFIFO_BIT_WIDTH];
			break;

		case CDSP_FIFO_DONTCARE:
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}

		switch (pbPrm[CDSP_EFIFO_E2BUF_MODE]) {
		case CODER_FMT_ETOBUF_LRMIX:
		case CODER_FMT_ETOBUF_LCH:
		case CODER_FMT_ETOBUF_RCH:
			bE2BufMode = pbPrm[CDSP_EFIFO_E2BUF_MODE];
			break;

		case CDSP_FIFO_DONTCARE:
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}

		psFuncInfo->sConnectEx.bEfifoCh = bCh;
		psFuncInfo->sBitWidth.bEfifoBit = bBit;
		psFuncInfo->sFormat.bE2BufMode = bE2BufMode;
	}

	/* OFIFO */
	if (pbPrm[ROUTE_OUT0L_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT0L_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT0L_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut0Sel & ~MCB_OUT0L_SEL);
			bData |= (pbPrm[ROUTE_OUT0L_SEL] & MCB_OUT0L_SEL);
			psCdspInfo->bOut0Sel = bData;
		}
	}

	if (pbPrm[ROUTE_OUT0R_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT0R_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT0R_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut0Sel & ~MCB_OUT0R_SEL);
			bData |= (pbPrm[ROUTE_OUT0R_SEL] << 4) & MCB_OUT0R_SEL;
			psCdspInfo->bOut0Sel = bData;
		}
	}

	if (pbPrm[ROUTE_OUT1L_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT1L_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT1L_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut1Sel & ~MCB_OUT1L_SEL);
			bData |= (pbPrm[ROUTE_OUT1L_SEL] & MCB_OUT1L_SEL);
			psCdspInfo->bOut1Sel = bData;
		}
	}

	if (pbPrm[ROUTE_OUT1R_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT1R_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT1R_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut1Sel & ~MCB_OUT1R_SEL);
			bData |= (pbPrm[ROUTE_OUT1R_SEL] << 4) & MCB_OUT1R_SEL;
			psCdspInfo->bOut1Sel = bData;
		}
	}

	if (pbPrm[ROUTE_OUT2L_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT2L_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT2L_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut2Sel & ~MCB_OUT2L_SEL);
			bData |= (pbPrm[ROUTE_OUT2L_SEL] & MCB_OUT2L_SEL);
			psCdspInfo->bOut2Sel = bData;
		}
	}

	if (pbPrm[ROUTE_OUT2R_SEL] != CDSP_FIFO_DONTCARE) {
		if (((dFifoId & FIFO_OFIFO) != 0) ||
			(pbPrm[ROUTE_OUT2R_SEL] == OUT_LOOPBACK_L) ||
			(pbPrm[ROUTE_OUT2R_SEL] == OUT_LOOPBACK_R)) {
			bData = (psCdspInfo->bOut2Sel & ~MCB_OUT2R_SEL);
			bData |= (pbPrm[ROUTE_OUT2R_SEL] << 4) & MCB_OUT2R_SEL;
			psCdspInfo->bOut2Sel = bData;
		}
	}

	if ((dFifoId & FIFO_OFIFO) != 0) {
		/* 0: 2ch 16bit) */
		/* 1: 4ch 16bit */
		/* 2: 2ch 32bit */
		/* 3: 4ch 32bit */
		bCh = psFuncInfo->sConnectEx.bOfifoCh;
		bBit = psFuncInfo->sBitWidth.bOfifoBit;

		switch (pbPrm[CDSP_OFIFO_CH]) {
		case 2:
		case 4:
			bCh = pbPrm[CDSP_OFIFO_CH];
			break;

		case CDSP_FIFO_DONTCARE:
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}

		switch (pbPrm[CDSP_OFIFO_BIT_WIDTH]) {
		case 16:
		case 32:
			bBit = pbPrm[CDSP_OFIFO_BIT_WIDTH];
			break;

		case CDSP_FIFO_DONTCARE:
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}

		psFuncInfo->sConnectEx.bOfifoCh = bCh;
		psFuncInfo->sBitWidth.bOfifoBit = bBit;

		dSamples = ((UINT32)pbPrm[CDSP_OFIFO_BUFFERING + 0] << 8) |
				((UINT32)pbPrm[CDSP_OFIFO_BUFFERING + 1]);
		if (CDSP_FIFO_DONTCARE_W != dSamples) {
			if (OFIFO_BUF_SAMPLE_MAX < dSamples)
				return MCDRV_ERROR_ARGUMENT;
			psCdspInfo->dOFifoBufSample = dSamples;
		}
	}

	/* DFIFO */
	if ((dFifoId & FIFO_DFIFO) != 0) {
		if (pbPrm[CDSP_DFIFO_BIT_WIDTH] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bRDFifoBitSel & ~MCB_DFIFO_BIT);
			switch (pbPrm[CDSP_DFIFO_BIT_WIDTH]) {
			case 32:
				bData |= MCB_DFIFO_BIT;
				break;

			case 16:
			default:
				break;
			}
			psCdspInfo->bRDFifoBitSel = bData;
		}

		dPos = ((UINT32)pbPrm[CDSP_DFIFO_CB_POINT + 0] << 8) |
			((UINT32)pbPrm[CDSP_DFIFO_CB_POINT + 1]);
		switch (dPos) {
		case CDSP_FIFO_DONTCARE_CB:
			break;

		case CDSP_FIFO_NOT_CB:
			psCdspInfo->sdDFifoCbPos = CBPOS_DFIFO_NONE;
			break;

		default:
			if (((UINT32)CBPOS_DFIFO_MAX < dPos))
				return MCDRV_ERROR_ARGUMENT;

			psCdspInfo->sdDFifoCbPos = (SINT32)dPos;
			break;
		}
	}

	/* RFIFO */
	if ((dFifoId & FIFO_RFIFO) != 0) {
		if (pbPrm[CDSP_RFIFO_BIT_WIDTH] != CDSP_FIFO_DONTCARE) {
			bData = (psCdspInfo->bRDFifoBitSel & ~MCB_RFIFO_BIT);
			switch (pbPrm[CDSP_DFIFO_BIT_WIDTH]) {
			case 32:
				bData |= MCB_RFIFO_BIT;
				break;

			case 16:
			default:
				break;
			}
			psCdspInfo->bRDFifoBitSel = bData;
		}

		dPos = ((UINT32)pbPrm[CDSP_RFIFO_CB_POINT + 0] << 8) |
			((UINT32)pbPrm[CDSP_RFIFO_CB_POINT + 1]);
		switch (dPos) {
		case CDSP_FIFO_DONTCARE_CB:
			break;

		case CDSP_FIFO_NOT_CB:
			psCdspInfo->sdRFifoCbPos = CBPOS_RFIFO_NONE;
			break;

		default:
			if (((UINT32)CBPOS_RFIFO_MAX < dPos))
				return MCDRV_ERROR_ARGUMENT;

			psCdspInfo->sdRFifoCbPos = (SINT32)dPos; 
			break;
		}

		dSamples = ((UINT32)pbPrm[CDSP_RFIFO_BUFFERING + 0]
			<< 8) |
			((UINT32)pbPrm[CDSP_RFIFO_BUFFERING + 1]);
		if (CDSP_FIFO_DONTCARE_W != dSamples) {
			if (RFIFO_BUF_SAMPLE_MAX < dSamples)
				return MCDRV_ERROR_ARGUMENT;
			psCdspInfo->dRFifoBufSample = dSamples;
		}
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	MemoryCheck
 *
 *	Function:
 *			Check memory use area.
 *	Arguments:
 *			wStartAdr1	Program-1 start address
 *			wSize1		Program-1 size
 *			wStartAdr2	Program-2 start address
 *			wSize2		Program-2 size
 *	Return:
 *			0		OK
 *			< 0		NG
 *
 ****************************************************************************/
static SINT32 MemoryCheck(UINT16 wStartAdr1, UINT16 wSize1,
					UINT16 wStartAdr2, UINT16 wSize2)
{
	UINT16 wEndAdr1;
	UINT16 wEndAdr2;

	wEndAdr1 = wStartAdr1 + wSize1 - 1;
	wEndAdr2 = wStartAdr2 + wSize2 - 1;

	if ((wStartAdr1 <= wStartAdr2) && (wStartAdr2 <= wEndAdr1))
		return MCDRV_ERROR_RESOURCEOVER;

	if ((wStartAdr1 <= wEndAdr2) && (wEndAdr2 <= wEndAdr1))
		return MCDRV_ERROR_RESOURCEOVER;

	if ((wStartAdr2 <= wStartAdr1) && (wStartAdr1 <= wEndAdr2))
		return MCDRV_ERROR_RESOURCEOVER;

	if ((wStartAdr2 <= wEndAdr1) && (wEndAdr1 <= wEndAdr2))
		return MCDRV_ERROR_RESOURCEOVER;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckProgram
 *
 *	Function:
 *			Check program.
 *	Arguments:
 *			dCoderId	Coder ID
 *			psProgram	Program information
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CheckProgram(UINT32 dCoderId,
				const struct MC_CODER_FIRMWARE *psProgram)
{
	UINT16 wProgLoadAddr;
	UINT16 wProgSize;
	UINT16 wDataLoadAddr;
	UINT16 wDataSize;
	UINT16 wWorkBeginAddr;
	UINT16 wWorkSize;
	UINT16 wStackBeginAddr;
	UINT16 wStackSize;
	UINT16 wProgType;
	UINT32 dTotalSize;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *psOtherDecInfo;
	struct PROGRAM_INFO *psOtherProgInfo;

	psDecInfo = GetDecInfo(dCoderId);
	psOtherDecInfo = GetOtherDecInfo(dCoderId);


	/* Size Check */
	if (psProgram->dSize < (UINT32)PRG_DESC_PROGRAM)
		return MCDRV_ERROR_ARGUMENT;

	dTotalSize = (UINT32)MAKEWORD(
				psProgram->pbFirmware[PRG_DESC_PRG_SIZE],
				psProgram->pbFirmware[PRG_DESC_PRG_SIZE+1]);
	dTotalSize += (UINT32)MAKEWORD(
				psProgram->pbFirmware[PRG_DESC_DATA_SIZE],
				psProgram->pbFirmware[PRG_DESC_DATA_SIZE+1]);
	dTotalSize += (UINT32)(PRG_DESC_PROGRAM / 2);
	if ((dTotalSize * 2UL) != psProgram->dSize)
		return MCDRV_ERROR_ARGUMENT;

	/* Program Type Check */
	wProgType = MAKEWORD(
				psProgram->pbFirmware[PRG_DESC_PRG_TYPE],
				psProgram->pbFirmware[PRG_DESC_PRG_TYPE+1]);
	if ((dCoderId == CODER_DEC) && (wProgType != PRG_PRM_TYPE_TASK0))
		return MCDRV_ERROR_ARGUMENT;

	if ((dCoderId == CODER_ENC) && (wProgType != PRG_PRM_TYPE_TASK1))
		return MCDRV_ERROR_ARGUMENT;

	if ((STATE_NOTINIT == psOtherDecInfo->dState)
		|| (STATE_INIT == psOtherDecInfo->dState))
		return MCDRV_SUCCESS;

	/* RAM Check */
	wProgLoadAddr = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_PRG_LOAD_ADR],
			psProgram->pbFirmware[PRG_DESC_PRG_LOAD_ADR+1]);
	wProgSize = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_PRG_SIZE],
			psProgram->pbFirmware[PRG_DESC_PRG_SIZE+1]);
	wDataLoadAddr = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_DATA_LOAD_ADR],
			psProgram->pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);
	wDataSize = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_DATA_SIZE],
			psProgram->pbFirmware[PRG_DESC_DATA_SIZE+1]);
	wWorkBeginAddr = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_WORK_BEGIN_ADR],
			psProgram->pbFirmware[PRG_DESC_WORK_BEGIN_ADR+1]);
	wWorkSize = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_WORK_SIZE],
			psProgram->pbFirmware[PRG_DESC_WORK_SIZE+1]);
	wStackBeginAddr = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_STACK_BEGIN_ADR],
			psProgram->pbFirmware[PRG_DESC_STACK_BEGIN_ADR+1]);
	wStackSize = MAKEWORD(
			psProgram->pbFirmware[PRG_DESC_STACK_SIZE],
			psProgram->pbFirmware[PRG_DESC_STACK_SIZE+1]);
	psOtherProgInfo	= &psOtherDecInfo->sProgInfo;

	/* Program & Program */
	sdRet = MemoryCheck(wProgLoadAddr, wProgSize,
					psOtherProgInfo->wProgLoadAddr,
					psOtherProgInfo->wProgSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Data & Data */
	sdRet = MemoryCheck(wDataLoadAddr, wDataSize,
					psOtherProgInfo->wDataLoadAddr,
					psOtherProgInfo->wDataSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Data & Stack */
	sdRet = MemoryCheck(wDataLoadAddr, wDataSize,
					psOtherProgInfo->wStackBeginAddr,
					psOtherProgInfo->wStackSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	sdRet = MemoryCheck(wStackBeginAddr, wStackSize,
					psOtherProgInfo->wDataLoadAddr,
					psOtherProgInfo->wDataSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Work & Work */
	sdRet = MemoryCheck(wWorkBeginAddr, wWorkSize,
					psOtherProgInfo->wWorkBeginAddr,
					psOtherProgInfo->wWorkSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Work & Stack */
	sdRet = MemoryCheck(wWorkBeginAddr, wWorkSize,
					psOtherProgInfo->wStackBeginAddr,
					psOtherProgInfo->wStackSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	sdRet = MemoryCheck(wStackBeginAddr, wStackSize,
					psOtherProgInfo->wWorkBeginAddr,
					psOtherProgInfo->wWorkSize);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckProgramAec
 *
 *	Function:
 *			Check program.
 *	Arguments:
 *			psProgramA	Program information FuncA
 *			psProgramB	Program information FuncB
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CheckProgramAec(const struct MC_CODER_FIRMWARE *psProgramA,
				const struct MC_CODER_FIRMWARE *psProgramB)
{
	UINT16 wProgLoadAddrA;
	UINT16 wProgSizeA;
	UINT16 wDataLoadAddrA;
	UINT16 wDataSizeA;
	UINT16 wWorkBeginAddrA;
	UINT16 wWorkSizeA;
	UINT16 wStackBeginAddrA;
	UINT16 wStackSizeA;
	UINT32 dTotalSizeA;
	UINT16 wProgLoadAddrB;
	UINT16 wProgSizeB;
	UINT16 wDataLoadAddrB;
	UINT16 wDataSizeB;
	UINT16 wWorkBeginAddrB;
	UINT16 wWorkSizeB;
	UINT16 wStackBeginAddrB;
	UINT16 wStackSizeB;
	UINT32 dTotalSizeB;
	SINT32 sdRet;

	dTotalSizeA = (UINT32)MAKEWORD(
				psProgramA->pbFirmware[PRG_DESC_PRG_SIZE],
				psProgramA->pbFirmware[PRG_DESC_PRG_SIZE+1]);
	dTotalSizeA += (UINT32)MAKEWORD(
				psProgramA->pbFirmware[PRG_DESC_DATA_SIZE],
				psProgramA->pbFirmware[PRG_DESC_DATA_SIZE+1]);
	dTotalSizeA += (UINT32)(PRG_DESC_PROGRAM / 2);

	dTotalSizeB = (UINT32)MAKEWORD(
				psProgramB->pbFirmware[PRG_DESC_PRG_SIZE],
				psProgramB->pbFirmware[PRG_DESC_PRG_SIZE+1]);
	dTotalSizeB += (UINT32)MAKEWORD(
				psProgramB->pbFirmware[PRG_DESC_DATA_SIZE],
				psProgramB->pbFirmware[PRG_DESC_DATA_SIZE+1]);
	dTotalSizeB += (UINT32)(PRG_DESC_PROGRAM / 2);


	/* RAM Check */
	wProgLoadAddrA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_PRG_LOAD_ADR],
			psProgramA->pbFirmware[PRG_DESC_PRG_LOAD_ADR+1]);
	wProgLoadAddrB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_PRG_LOAD_ADR],
			psProgramB->pbFirmware[PRG_DESC_PRG_LOAD_ADR+1]);

	wProgSizeA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_PRG_SIZE],
			psProgramA->pbFirmware[PRG_DESC_PRG_SIZE+1]);
	wProgSizeB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_PRG_SIZE],
			psProgramB->pbFirmware[PRG_DESC_PRG_SIZE+1]);

	wDataLoadAddrA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_DATA_LOAD_ADR],
			psProgramA->pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);
	wDataLoadAddrB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_DATA_LOAD_ADR],
			psProgramB->pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);

	wDataSizeA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_DATA_SIZE],
			psProgramA->pbFirmware[PRG_DESC_DATA_SIZE+1]);
	wDataSizeB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_DATA_SIZE],
			psProgramB->pbFirmware[PRG_DESC_DATA_SIZE+1]);

	wWorkBeginAddrA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_WORK_BEGIN_ADR],
			psProgramA->pbFirmware[PRG_DESC_WORK_BEGIN_ADR+1]);
	wWorkBeginAddrB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_WORK_BEGIN_ADR],
			psProgramB->pbFirmware[PRG_DESC_WORK_BEGIN_ADR+1]);

	wWorkSizeA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_WORK_SIZE],
			psProgramA->pbFirmware[PRG_DESC_WORK_SIZE+1]);
	wWorkSizeB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_WORK_SIZE],
			psProgramB->pbFirmware[PRG_DESC_WORK_SIZE+1]);

	wStackBeginAddrA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_STACK_BEGIN_ADR],
			psProgramA->pbFirmware[PRG_DESC_STACK_BEGIN_ADR+1]);
	wStackBeginAddrB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_STACK_BEGIN_ADR],
			psProgramB->pbFirmware[PRG_DESC_STACK_BEGIN_ADR+1]);

	wStackSizeA = MAKEWORD(
			psProgramA->pbFirmware[PRG_DESC_STACK_SIZE],
			psProgramA->pbFirmware[PRG_DESC_STACK_SIZE+1]);
	wStackSizeB = MAKEWORD(
			psProgramB->pbFirmware[PRG_DESC_STACK_SIZE],
			psProgramB->pbFirmware[PRG_DESC_STACK_SIZE+1]);

	/* Program & Program */
	sdRet = MemoryCheck(wProgLoadAddrA, wProgSizeA,
						wProgLoadAddrB, wProgSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Data & Data */
	sdRet = MemoryCheck(wDataLoadAddrA, wDataSizeA,
						wDataLoadAddrB, wDataSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Data & Stack */
	sdRet = MemoryCheck(wDataLoadAddrA, wDataSizeA,
						wStackBeginAddrB, wStackSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	sdRet = MemoryCheck(wStackBeginAddrA, wStackSizeA,
						wDataLoadAddrB, wDataSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Work & Work */
	sdRet = MemoryCheck(wWorkBeginAddrA, wWorkSizeA,
						wWorkBeginAddrB, wWorkSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	/* Work & Stack */
	sdRet = MemoryCheck(wWorkBeginAddrA, wWorkSizeA,
						wStackBeginAddrB, wStackSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	sdRet = MemoryCheck(wStackBeginAddrA, wStackSizeA,
						wWorkBeginAddrB, wWorkSizeB);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckInOutPath
 *
 *	Function:
 *			Check path.
 *	Arguments:
 *			psDecConnect		Pointer of current Connect.
 *			psOtherDecConnect	Pointer of other Connect.
 *	Return:
 *			0			success
 *			< 0			error code
 *
 ****************************************************************************/
static SINT32 CheckInOutPath(const struct CONNECTION_INFO *psDecConnect,
			const struct CONNECTION_INFO *psOtherDecConnect)
{
	/* Input */
	switch (psDecConnect->bInSource) {
	case CDSP_IN_SOURCE_OTHER_OUTBUF:
	case CDSP_IN_SOURCE_NONE:
		break;

	/* DFIFO */
	case CDSP_IN_SOURCE_DFIFO:
		switch (psOtherDecConnect->bInSource) {
		case CDSP_IN_SOURCE_EFIFO:
		case CDSP_IN_SOURCE_OTHER_OUTBUF:
		case CDSP_IN_SOURCE_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}
		break;

	/* EFIFO */
	case CDSP_IN_SOURCE_EFIFO:
		switch (psOtherDecConnect->bInSource) {
		case CDSP_IN_SOURCE_DFIFO:
		case CDSP_IN_SOURCE_OTHER_OUTBUF:
		case CDSP_IN_SOURCE_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}
		break;

	/* DFIFO & EFIFO */
	case CDSP_IN_SOURCE_DFIFO_EFIFO:
		switch (psOtherDecConnect->bInSource) {
		case CDSP_IN_SOURCE_OTHER_OUTBUF:
		case CDSP_IN_SOURCE_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}

	default:
		return MCDRV_ERROR;
	}

	/* output */
	switch (psDecConnect->bOutDest) {
	case CDSP_OUT_DEST_OTHER_INBUF:
	case CDSP_OUT_DEST_NONE:
		break;

	/* RFIFO */
	case CDSP_OUT_DEST_RFIFO:
		switch (psOtherDecConnect->bOutDest) {
		case CDSP_OUT_DEST_OFIFO:
		case CDSP_OUT_DEST_OTHER_INBUF:
		case CDSP_OUT_DEST_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}
		break;

	/* OFIFO */
	case CDSP_OUT_DEST_OFIFO:
		switch (psOtherDecConnect->bOutDest) {
		case CDSP_OUT_DEST_RFIFO:
		case CDSP_OUT_DEST_OTHER_INBUF:
		case CDSP_OUT_DEST_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}
		break;

	/* RFIFO & OFIFO */
	case CDSP_OUT_DEST_OFIFO_RFIFO:
		switch (psOtherDecConnect->bOutDest) {
		case CDSP_OUT_DEST_OTHER_INBUF:
		case CDSP_OUT_DEST_NONE:
			break;

		default:
			return MCDRV_ERROR_RESOURCEOVER;
		}
		break;

	default:
		break;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckInOutType
 *
 *	Function:
 *			Check input output type.
 *	Arguments:
 *			psConnect	Pointer of Connect
 *			wInOutType	input output type
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CheckInOutType(struct CONNECTION_INFO *psConnect,
							UINT16 wInOutType)
{
	UINT16 wInType;
	UINT16 wOutType;

	wInType = wInOutType & PRG_PRM_IOTYPE_IN_MASK;
	wOutType = wInOutType & PRG_PRM_IOTYPE_OUT_MASK;

	/* Input type check */
	if (PRG_PRM_IOTYPE_IN_NOPCM == wInType) {
		switch (psConnect->bInSource) {
		case CDSP_IN_SOURCE_NONE:
		case CDSP_IN_SOURCE_DFIFO:
			break;
		default:
			return MCDRV_ERROR;
		}
	}

	/* Output type check */
	if (PRG_PRM_IOTYPE_OUT_NOPCM == wOutType) {
		switch (psConnect->bOutDest) {
		case CDSP_OUT_DEST_NONE:
		case CDSP_OUT_DEST_RFIFO:
			break;
		default:
			return MCDRV_ERROR;
		}
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckPath
 *
 *	Function:
 *			Check path.
 *	Arguments:
 *			dCoderId	Coder ID
 *			wInOutType	input output type
 *			psConnect	Connect information
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CheckPath(UINT32 dCoderId, UINT16 wInOutType,
					struct CONNECTION_INFO *psConnect)
{
	SINT32 sdRet;
	struct DEC_INFO *psOtherDecInfo;

	psOtherDecInfo = GetOtherDecInfo(dCoderId);

	switch (psOtherDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		/* Check Input/Output path */
		sdRet = CheckInOutPath(psConnect, &psOtherDecInfo->sConnect);
		if ((SINT32)MCDRV_SUCCESS != sdRet)
			return sdRet;
		break;

	default:
		break;
	}

	/* Check Input/Output Type */
	sdRet = CheckInOutType(psConnect, wInOutType);

	return sdRet;
}

/****************************************************************************
 *	CdspChunkAnalyzeFunc
 *
 *	Function:
 *			Analyze Func chunk
 *	Arguments:
 *			dCoderId	Coder ID
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CdspChunkAnalyzeFunc(UINT32 dCoderId,
				struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	UINT32 dChunkTop;
	UINT32 dChunkId;
	UINT32 dChunkSize;
	UINT8 *pbPrm;
	UINT32 dSize;
	UINT32 dTemp;
	SINT32 sdRet;
	struct MC_CODER_FIRMWARE sProgram;
	struct DEC_INFO *psDecInfo;
	struct AEC_CDSP_FUNC_INFO *psFuncInfo;

	if (dCoderId == CODER_DEC)
		psFuncInfo = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A];
	else
		psFuncInfo = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B];

	pbPrm = psFuncInfo->pbChunk;
	dSize = psFuncInfo->dChunkSize;
	psFuncInfo->pbFifo = NULL;
	psFuncInfo->pbProg = NULL;
	psFuncInfo->dProgSize = 0;
	psFuncInfo->pbParam = NULL;
	psFuncInfo->dParamNum = 0;
	psFuncInfo->pbExt = NULL;

	psDecInfo = GetDecInfo(psFuncInfo->dCoderId);
	psFuncInfo->sConnect.bInSource = psDecInfo->sConnect.bInSource;
	psFuncInfo->sConnect.bOutDest = psDecInfo->sConnect.bOutDest;
	psFuncInfo->sConnectEx.bEfifoCh = psDecInfo->sConnectEx.bEfifoCh;
	psFuncInfo->sConnectEx.bOfifoCh = psDecInfo->sConnectEx.bOfifoCh;
	psFuncInfo->sBitWidth.bEfifoBit = psDecInfo->sBitWidth.bEfifoBit;
	psFuncInfo->sBitWidth.bOfifoBit = psDecInfo->sBitWidth.bOfifoBit;
	psFuncInfo->sFormat.bFs = psDecInfo->sFormat.bFs;
	psFuncInfo->sFormat.bE2BufMode = psDecInfo->sFormat.bE2BufMode;
	psFuncInfo->wInOutType = psDecInfo->sProgInfo.wInOutType;

	if ((pbPrm == NULL) || (dSize == 0))
		return MCDRV_SUCCESS;

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
		case AEC_CDSP_TAG_PROG:
			if (dChunkSize < (UINT32)PROG_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);
			if (dTemp == 0UL)
				return MCDRV_SUCCESS;
			if (dChunkSize < (dTemp + (UINT32)PROG_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			if (psFuncInfo->pbProg != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFuncInfo->pbProg = &pbPrm[dChunkTop + PROG_FIX_SIZE];
			psFuncInfo->dProgSize = dTemp;
			break;

		case AEC_CDSP_TAG_PRM:
			if (dChunkSize < (UINT32)PRM_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;
			dTemp = CreateUINT32(pbPrm[dChunkTop + 0UL],
						pbPrm[dChunkTop + 1UL],
						pbPrm[dChunkTop + 2UL],
						pbPrm[dChunkTop + 3UL]);
			if (dTemp == 0UL)
				return MCDRV_SUCCESS;
			if ((dTemp % PRM_UNIT_SIZE) != 0)
				return MCDRV_ERROR_ARGUMENT;
			if (dChunkSize < (dTemp + (UINT32)PRM_FIX_SIZE))
				return MCDRV_ERROR_ARGUMENT;
			if (psFuncInfo->pbParam != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFuncInfo->pbParam = &pbPrm[dChunkTop + PRM_FIX_SIZE];
			psFuncInfo->dParamNum = (dTemp / PRM_UNIT_SIZE);
			break;

		case AEC_CDSP_TAG_FIFO:
			if (dChunkSize < (UINT32)FIFO_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			if (psFuncInfo->pbFifo != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFuncInfo->pbFifo = &pbPrm[dChunkTop];
			break;

		case AEC_CDSP_TAG_EXT:
			if (dChunkSize < (UINT32)EXT_FIX_SIZE)
				return MCDRV_ERROR_ARGUMENT;

			if (psFuncInfo->pbExt != NULL)
				return MCDRV_ERROR_ARGUMENT;

			psFuncInfo->pbExt = &pbPrm[dChunkTop];
			break;

		default:
			break;
		}
		dChunkTop += dChunkSize;
	}

	/* FIFO Check */
	if (NULL != psFuncInfo->pbFifo) {
		sdRet = CheckFIFO(dCoderId, psCdspInfo);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;
	}

	/* Program Check */
	if ((0UL != psFuncInfo->dProgSize) && (NULL != psFuncInfo->pbProg)) {
		sProgram.pbFirmware = psFuncInfo->pbProg;
		sProgram.dSize = psFuncInfo->dProgSize;

		sdRet = CheckProgram(psFuncInfo->dCoderId, &sProgram);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		psFuncInfo->wInOutType = MAKEWORD(
			sProgram.pbFirmware[PRG_DESC_OUTPUT_TYPE],
			sProgram.pbFirmware[PRG_DESC_OUTPUT_TYPE+1]);
	}

	/* Path check */
	sdRet = CheckPath(psFuncInfo->dCoderId, psFuncInfo->wInOutType,
						&psFuncInfo->sConnect);

	return sdRet;
}

/****************************************************************************
 *	CdspChunkAnalyze
 *
 *	Function:
 *			Analyze CDSP chunk
 *	Arguments:
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 CdspChunkAnalyze(struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	SINT32 sdRet;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoA;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoB;
	struct MC_CODER_FIRMWARE sProgramA;
	struct MC_CODER_FIRMWARE sProgramB;

	psFuncInfoA = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A];
	psFuncInfoB = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B];
	psCdspInfo->sdDFifoCbPos = gsFifoInfo.sdDFifoCbPos;
	psCdspInfo->sdRFifoCbPos = gsFifoInfo.sdRFifoCbPos;
	psCdspInfo->dOFifoBufSample = gsFifoInfo.dOFifoBufSample;
	psCdspInfo->dRFifoBufSample = gsFifoInfo.dRFifoBufSample;
	psCdspInfo->bOut0Sel = gsFifoInfo.bOut0Sel;
	psCdspInfo->bOut1Sel = gsFifoInfo.bOut1Sel;
	psCdspInfo->bOut2Sel = gsFifoInfo.bOut2Sel;
	psCdspInfo->bRDFifoBitSel = gsFifoInfo.bRDFifoBitSel;
	psCdspInfo->bEFifo01Sel = gsFifoInfo.bEFifo01Sel;
	psCdspInfo->bEFifo23Sel = gsFifoInfo.bEFifo23Sel;

	sdRet = CdspChunkAnalyzeFunc(CODER_DEC, psCdspInfo);
	if (sdRet < MCDRV_SUCCESS)
		return sdRet;

	sdRet = CdspChunkAnalyzeFunc(CODER_ENC, psCdspInfo);
	if (sdRet < MCDRV_SUCCESS)
		return sdRet;

	if ((NULL != psFuncInfoA->pbFifo) && (NULL != psFuncInfoB->pbFifo)) {
		sdRet = CheckInOutPath(&psFuncInfoA->sConnect,
						&psFuncInfoB->sConnect);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;
	}

	if ((0UL != psFuncInfoA->dProgSize)
		&& (NULL != psFuncInfoA->pbProg)
		&& (0UL != psFuncInfoB->dProgSize)
		&& (NULL != psFuncInfoB->pbProg)) {
		sProgramA.pbFirmware = psFuncInfoA->pbProg;
		sProgramA.dSize = psFuncInfoA->dProgSize;
		sProgramB.pbFirmware = psFuncInfoB->pbProg;
		sProgramB.dSize = psFuncInfoB->dProgSize;
		sdRet = CheckProgramAec(&sProgramA, &sProgramB);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;
	}

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	CompleteNotifyOutputFormat
 *
 *	Function:
 *			Complete Notify Output Format command.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void CompleteNotifyOutputFormat(UINT32 dCoderId)
{
	struct DEC_INFO *psOtherDecInfo;
	struct MC_CODER_PARAMS sParam;

	psOtherDecInfo = GetOtherDecInfo(dCoderId);

	/* Complete NotifyOutFormat */
	if (FORMAT_PROPAGATE_ON == psOtherDecInfo->bFmtPropagate) {
		sParam.bCommandId =
				(UINT8)CDSP_CMD_OS2HOST_CMN_NOTIFY_OUT_FORMAT;
		sParam.abParam[CDSP_CMD_PARAM_RESULT_00] = 0;
		sParam.abParam[CDSP_CMD_PARAM_RESULT_01] = 0;
		sParam.abParam[CDSP_CMD_PARAM_RESULT_02] = 0;
		sParam.abParam[CDSP_CMD_PARAM_RESULT_03] = 0;
		CommandWriteComplete(dCoderId, &sParam);

		psOtherDecInfo->bFmtPropagate = FORMAT_PROPAGATE_OFF;
	}
}

/****************************************************************************
 *	DecClose
 *
 *	Function:
 *			Open resource.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void DecClose(UINT32 dCoderId)
{
	UINT8 bAddInt;
	UINT8 bAddCtl;
	UINT8 bIntData;
	UINT8 bCtlData;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* Complete NotifyOutFormat */
	CompleteNotifyOutputFormat(dCoderId);

	/* Reset command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_RESET;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);

	/* Command register Initialize */
	CommandInitialize(psDecInfo->dCoderId);

	/* DEC/ENC SFR,EVT Disable */
	if (CODER_DEC == dCoderId) {
		bAddInt = MCI_DEC_ENABLE;
		bAddCtl = MCB_EDEC;
	} else {
		bAddInt = MCI_ENC_ENABLE;
		bAddCtl = MCB_EENC;
	}
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)bAddInt), &bIntData, 1);

	bIntData &= ~(MCB_EDEC_EVT | MCB_EDEC_SFR);

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_ECDSP), &bCtlData, 1);
	bCtlData &= ~bAddCtl;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_ECDSP), bCtlData);

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)bAddInt), bIntData);

	McDevIf_ExecutePacket();


	psDecInfo->sFormat.bFs = CODER_FMT_FS_48000;

}

/****************************************************************************
 *	TerminateProgram
 *
 *	Function:
 *			Terminate program.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void TerminateProgram(UINT32 dCoderId)
{
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_TERMINATE;
	CommandWriteHost2Os(dCoderId, &sParam);

	psDecInfo->sProgInfo.wVendorId = 0;
	psDecInfo->sProgInfo.wFunctionId = 0;
}

/***************************************************************************
 *	WriteProgram
 *
 *	Function:
 *			Write CDSP program to FIFO.
 *	Arguments:
 *			psDataInfo	Program information
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 WriteProgram(struct FSQ_DATA_INFO *psDataInfo)
{
	UINT8 bData;
	UINT16 wFFifoWordSize;
	UINT16 wLoadAddr;
	UINT16 wWriteSize;
	UINT16 wCurrentPos;
	UINT16 wRemainSize;
	SINT32 sdRet;
	UINT32 i;

	sdRet = MCDRV_SUCCESS;
	wFFifoWordSize = FIFOSIZE_FFIFO / sizeof(UINT16);
	wCurrentPos = 0;
	wRemainSize = psDataInfo->wSize;
	wLoadAddr = psDataInfo->wLoadAddr;

	/* CDSP_MSEL Set */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_RESET), &bData, 1);
	if (PRG_PRM_SCRMBL_DISABLE == psDataInfo->wScramble)
		bData |= MCB_CDSP_FMODE;
	else
		bData &= ~MCB_CDSP_FMODE;

	bData &= ~MCB_CDSP_MSEL;
	if (MSEL_PROG == psDataInfo->bMsel)
		bData |= MCB_CDSP_MSEL_PROG;
	else
		bData |= MCB_CDSP_MSEL_DATA;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_RESET),
					bData);
	McDevIf_ExecutePacket();

	while ((wRemainSize > 0) && (sdRet >= (SINT32)MCDRV_SUCCESS)) {
		/* CDSP_MAR Set */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_MAR_H),
					HIBYTE(wLoadAddr));
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_MAR_L),
					LOBYTE(wLoadAddr));
		McDevIf_ExecutePacket();

		/* fill FFIFO */
		if (wRemainSize > wFFifoWordSize)
			wWriteSize = wFFifoWordSize;
		else
			wWriteSize = wRemainSize;
		for (i = 0; i < ((UINT32)wWriteSize * 2UL); ++i)
			McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_FSQ_FFIFO),
					psDataInfo->pbData[
					((UINT32)wCurrentPos * 2UL)+i]);

		McDevIf_ExecutePacket();

		wLoadAddr = wLoadAddr + wWriteSize;
		wCurrentPos = wCurrentPos + wWriteSize;
		wRemainSize = wRemainSize - wWriteSize;

		/* FFIFO_FLG Clear */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_FFIFO);

		/* FSQ_END_FLG Clear */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_FLG),
					MCB_FFIFO_FLG_FSQ_END);
		McDevIf_ExecutePacket();

		/* FSQ_START Set */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START),
					&bData,
					1);
		bData |= MCB_DEC_FSQ_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START),
					bData);
		McDevIf_ExecutePacket();

		/* FSQ_END_FLG Polling */
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
					| MCDRV_EVT_C_REG_FLAG_SET
					| (((UINT32)MCI_FFIFO_FLG) << 8)
					| (UINT32)MCB_FFIFO_FLG_FSQ_END),
					0);

		sdRet = McDevIf_ExecutePacket();
		if ((SINT32)MCDRV_SUCCESS > sdRet)
			return sdRet;

		/* FSQ_START Clear */
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), &bData, 1);
		bData &= ~MCB_DEC_FSQ_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), bData);
		McDevIf_ExecutePacket();
	}

	/* FFIFO_FLG Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_FFIFO);

	/* FSQ_END_FLG Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_FFIFO_FLG),
					MCB_FFIFO_FLG_FSQ_END);
	McDevIf_ExecutePacket();
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	DownloadProgram
 *
 *	Function:
 *			Download CDSP program.
 *	Arguments:
 *			pbFirmware	Program data
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DownloadProgram(const UINT8 *pbFirmware)
{
	UINT8 bData;
	SINT32 sdRet;
	struct FSQ_DATA_INFO asDataInfo[2];

	/* CDSP_SAVEOFF Set */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_PWM_DIGITAL_CDSP),
				MCB_PWM_CDSP_SAVEOFF);
	McDevIf_ExecutePacket();

	/* CDSP_HALT_MODE Check */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_EVTWAIT
				| MCDRV_EVT_C_REG_FLAG_RESET
				| (((UINT32)MCI_CDSP_POWER_MODE) << 8)
				| (UINT32)MCB_CDSP_HLT_MODE_SLEEP_HALT)	,
				0);

	sdRet = McDevIf_ExecutePacket();
	if (sdRet < (SINT32)MCDRV_SUCCESS) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_PWM_DIGITAL_CDSP),
					0x00);

		McDevIf_ExecutePacket();

		return sdRet;
	}

	/* FSQ_SRST */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_RESET), &bData, 1);
	bData |= MCB_CDSP_FSQ_SRST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);
	bData &= ~MCB_CDSP_FSQ_SRST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);

	/* 150ns wait */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_TIMWAIT | 1UL), 0x00);

	McDevIf_ExecutePacket();

	/* FFIFO_RST */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FIFO_RST),
						&bData, 1);
	bData |= MCB_DEC_FFIFO_RST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FIFO_RST),
						bData);
	bData &= ~MCB_DEC_FFIFO_RST;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FIFO_RST),
						bData);

	McDevIf_ExecutePacket();

	/* Transfer Program & Data */
	asDataInfo[0].pbData = &pbFirmware[PRG_DESC_PROGRAM];
	asDataInfo[0].wSize = MAKEWORD(pbFirmware[PRG_DESC_PRG_SIZE],
					pbFirmware[PRG_DESC_PRG_SIZE+1]);
	asDataInfo[0].wLoadAddr = MAKEWORD(pbFirmware[PRG_DESC_PRG_LOAD_ADR],
					pbFirmware[PRG_DESC_PRG_LOAD_ADR+1]);
	asDataInfo[0].wScramble = MAKEWORD(pbFirmware[PRG_DESC_PRG_SCRAMBLE],
					pbFirmware[PRG_DESC_PRG_SCRAMBLE+1]);
	asDataInfo[0].bMsel = (UINT8)MSEL_PROG;


	asDataInfo[1].pbData = &asDataInfo[0].pbData[
					((UINT32)asDataInfo[0].wSize * 2UL)];
	asDataInfo[1].wSize = MAKEWORD(pbFirmware[PRG_DESC_DATA_SIZE],
					pbFirmware[PRG_DESC_DATA_SIZE+1]);
	asDataInfo[1].wLoadAddr = MAKEWORD(pbFirmware[PRG_DESC_DATA_LOAD_ADR],
					pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);
	asDataInfo[1].wScramble = MAKEWORD(pbFirmware[PRG_DESC_DATA_SCRAMBLE],
					pbFirmware[PRG_DESC_DATA_SCRAMBLE+1]);
	asDataInfo[1].bMsel = (UINT8)MSEL_DATA;

	sdRet = WriteProgram(&asDataInfo[0]);
	if ((SINT32)MCDRV_SUCCESS == sdRet)
		sdRet = WriteProgram(&asDataInfo[1]);

	/* CDSP_SAVEOFF Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_PWM_DIGITAL_CDSP),
					0x00);
	McDevIf_ExecutePacket();

	return sdRet;
}

/****************************************************************************
 *	InitializeProgram
 *
 *	Function:
 *			Initialize program.
 *	Arguments:
 *			dCoderId	Coder ID
 *			psProgram	Program information
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 InitializeProgram(UINT32 dCoderId,
				const struct MC_CODER_FIRMWARE *psProgram)
{
	UINT16 wDataLoadAddr;
	UINT16 wWorkBeginAddr;
	UINT16 wDataAddr;
	UINT16 wDataSize;
	SINT32 sdRet;
	const UINT8 *pbFirmware;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	pbFirmware = psProgram->pbFirmware;
	wDataLoadAddr = MAKEWORD(pbFirmware[PRG_DESC_DATA_LOAD_ADR],
				pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);
	wWorkBeginAddr	= MAKEWORD(pbFirmware[PRG_DESC_WORK_BEGIN_ADR],
					pbFirmware[PRG_DESC_WORK_BEGIN_ADR+1]);
	if (wDataLoadAddr < wWorkBeginAddr)
		wDataAddr = wDataLoadAddr;
	else
		wDataAddr = wWorkBeginAddr;
	wDataSize = MAKEWORD(pbFirmware[PRG_DESC_DATA_SIZE],
					pbFirmware[PRG_DESC_DATA_SIZE+1]);
	wDataSize = wDataSize + (UINT16)MAKEWORD(
					pbFirmware[PRG_DESC_WORK_SIZE],
					pbFirmware[PRG_DESC_WORK_SIZE+1]);

	/* SetProgramInfo command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_SET_PRG_INFO;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = LOBYTE(wDataAddr);
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01] = HIBYTE(wDataAddr);
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_02] = LOBYTE(wDataSize);
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_03] = HIBYTE(wDataSize);
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_04] =
					pbFirmware[PRG_DESC_ENTRY_ADR];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_05] =
					pbFirmware[PRG_DESC_ENTRY_ADR+1];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_06] =
					pbFirmware[PRG_DESC_STACK_BEGIN_ADR];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_07] =
					pbFirmware[PRG_DESC_STACK_BEGIN_ADR+1];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_08] =
					pbFirmware[PRG_DESC_STACK_SIZE];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_09] =
					pbFirmware[PRG_DESC_STACK_SIZE+1];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_10] =
					pbFirmware[PRG_DESC_RESOURCE_FLAG];
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Reset command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_RESET;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* GetProgramVersion command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_GET_PRG_VER;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	psDecInfo = GetDecInfo(dCoderId);
	psDecInfo->sProgVer.wVersionH = MAKEWORD(
				sParam.abParam[CDSP_CMD_PARAM_RESULT_00],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_01]);
	psDecInfo->sProgVer.wVersionL = MAKEWORD(
				sParam.abParam[CDSP_CMD_PARAM_RESULT_02],
				sParam.abParam[CDSP_CMD_PARAM_RESULT_03]);

	psDecInfo->sProgInfo.wVendorId = MAKEWORD(
				pbFirmware[PRG_DESC_VENDER_ID],
				pbFirmware[PRG_DESC_VENDER_ID+1]);
	psDecInfo->sProgInfo.wFunctionId = MAKEWORD(
				pbFirmware[PRG_DESC_FUNCTION_ID],
				pbFirmware[PRG_DESC_FUNCTION_ID+1]);
	psDecInfo->sProgInfo.wProgType = MAKEWORD(
				pbFirmware[PRG_DESC_PRG_TYPE],
				pbFirmware[PRG_DESC_PRG_TYPE+1]);
	psDecInfo->sProgInfo.wInOutType = MAKEWORD(
				pbFirmware[PRG_DESC_OUTPUT_TYPE],
				pbFirmware[PRG_DESC_OUTPUT_TYPE+1]);
	psDecInfo->sProgInfo.wProgScramble = MAKEWORD(
				pbFirmware[PRG_DESC_PRG_SCRAMBLE],
				pbFirmware[PRG_DESC_PRG_SCRAMBLE+1]);
	psDecInfo->sProgInfo.wDataScramble = MAKEWORD(
				pbFirmware[PRG_DESC_DATA_SCRAMBLE],
				pbFirmware[PRG_DESC_DATA_SCRAMBLE+1]);
	psDecInfo->sProgInfo.wEntryAddress = MAKEWORD(
				pbFirmware[PRG_DESC_ENTRY_ADR],
				pbFirmware[PRG_DESC_ENTRY_ADR+1]);
	psDecInfo->sProgInfo.wProgLoadAddr = MAKEWORD(
				pbFirmware[PRG_DESC_PRG_LOAD_ADR],
				pbFirmware[PRG_DESC_PRG_LOAD_ADR+1]);
	psDecInfo->sProgInfo.wProgSize = MAKEWORD(
				pbFirmware[PRG_DESC_PRG_SIZE],
				pbFirmware[PRG_DESC_PRG_SIZE+1]);
	psDecInfo->sProgInfo.wDataLoadAddr = MAKEWORD(
				pbFirmware[PRG_DESC_DATA_LOAD_ADR],
				pbFirmware[PRG_DESC_DATA_LOAD_ADR+1]);
	psDecInfo->sProgInfo.wDataSize = MAKEWORD(
				pbFirmware[PRG_DESC_DATA_SIZE],
				pbFirmware[PRG_DESC_DATA_SIZE+1]);
	psDecInfo->sProgInfo.wWorkBeginAddr = MAKEWORD(
				pbFirmware[PRG_DESC_WORK_BEGIN_ADR],
				pbFirmware[PRG_DESC_WORK_BEGIN_ADR+1]);
	psDecInfo->sProgInfo.wWorkSize = MAKEWORD(
				pbFirmware[PRG_DESC_WORK_SIZE],
				pbFirmware[PRG_DESC_WORK_SIZE+1]);
	psDecInfo->sProgInfo.wStackBeginAddr = MAKEWORD(
				pbFirmware[PRG_DESC_STACK_BEGIN_ADR],
				pbFirmware[PRG_DESC_STACK_BEGIN_ADR+1]);
	psDecInfo->sProgInfo.wStackSize = MAKEWORD(
				pbFirmware[PRG_DESC_STACK_SIZE],
				pbFirmware[PRG_DESC_STACK_SIZE+1]);
	psDecInfo->sProgInfo.wOutStartMode = MAKEWORD(
				pbFirmware[PRG_DESC_OUTSTARTMODE],
				pbFirmware[PRG_DESC_OUTSTARTMODE+1]);
	psDecInfo->sProgInfo.wResourceFlag = MAKEWORD(
				pbFirmware[PRG_DESC_RESOURCE_FLAG],
				pbFirmware[PRG_DESC_RESOURCE_FLAG+1]);
	psDecInfo->sProgInfo.wMaxLoad = MAKEWORD(
				pbFirmware[PRG_DESC_MAX_LOAD],
				pbFirmware[PRG_DESC_MAX_LOAD+1]);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	FifoReset
 *
 *	Function:
 *			Reset FIFO.
 *	Arguments:
 *			dFifoId		FIFO ID
 *			dTargetFifo	Target FIFO
 *	Return:
 *			none
 *
 ****************************************************************************/
static void FifoReset(UINT32 dCoderId, UINT32 dTargetFifo)
{
	UINT8 bData;
	UINT32 dFifoId;

	dFifoId = GetFifoId(dCoderId);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_FIFO_RST), &bData, 1);

	if ((FIFO_NONE != (FIFO_DFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_DFIFO_MASK & dFifoId))) {
		/* DFIFO Reset */
		bData |= MCB_DEC_DFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FIFO_RST),
						bData);
		bData &= ~MCB_DEC_DFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_DEC_FIFO_RST),
						bData);

		gsFifoInfo.dDFifoWriteSize = 0;
	}

	if ((FIFO_NONE != (FIFO_EFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_EFIFO_MASK & dFifoId))) {
		/* EFIFO Reset */
		bData |= MCB_DEC_EFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
		bData &= ~MCB_DEC_EFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
	}

	if ((FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_OFIFO_MASK & dFifoId))) {
		/* OFIFO Reset */
		bData |= MCB_DEC_OFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
		bData &= ~MCB_DEC_OFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
	}

	if ((FIFO_NONE != (FIFO_RFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_RFIFO_MASK & dFifoId))) {
		/* RFIFO Reset */
		bData |= MCB_DEC_RFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
		bData &= ~MCB_DEC_RFIFO_RST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_RST),
					bData);
	}

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	FifoInit
 *
 *	Function:
 *			FIFO Init.
 *	Arguments:
 *			dFifoId		FIFO ID
 *			dTargetFifo	Target FIFO
 *	Return:
 *			None
 *
 ****************************************************************************/
static void FifoInit(UINT32 dCoderId, UINT32 dTargetFifo)
{
	UINT32 dFifoId;

	dFifoId = GetFifoId(dCoderId);

	if ((FIFO_NONE != (FIFO_DFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_DFIFO_MASK & dFifoId))) {
		gsFifoInfo.sdDFifoCbPos = CBPOS_DFIFO_DEF;
		gsFifoInfo.dDFifoWriteSize = 0;
	}

	if ((FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_OFIFO_MASK & dFifoId)))
		gsFifoInfo.dOFifoBufSample = OFIFO_BUF_SAMPLE_DEF;

	if ((FIFO_NONE != (FIFO_RFIFO_MASK & dTargetFifo))
		&& (FIFO_NONE != (FIFO_RFIFO_MASK & dFifoId))) {
		gsFifoInfo.sdRFifoCbPos = CBPOS_RFIFO_DEF;
		gsFifoInfo.dRFifoBufSample = RFIFO_BUF_SAMPLE_DEF;
	}

	FifoReset(dCoderId, dTargetFifo);
}

/***************************************************************************
 *	SetFifoCh
 *
 *	Function:
 *			Set FIFO SYNC
 *	Arguments:
 *			dTargetFifo	Target FIFO
 *			bCh		FIFO_CH
 *	Return:
 *			old fifo Ch
 *
 ****************************************************************************/
static UINT8 SetFifoCh(UINT32 dTargetFifo, UINT8 bCh)
{
	UINT8 bData;
	UINT8 bOldCh;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_FIFO_CH), &bData, 1);

	bOldCh = 0;
	if (FIFO_NONE != (FIFO_EFIFO_MASK & dTargetFifo)) {
		bOldCh |= (bData & MCB_DEC_EFIFO_CH);
		bData &= ~MCB_DEC_EFIFO_CH;
		bData |= (bCh & MCB_DEC_EFIFO_CH);
	}

	if (FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo)) {
		bOldCh |= (bData & MCB_DEC_OFIFO_CH);
		bData &= ~MCB_DEC_OFIFO_CH;
		bData |= (bCh & MCB_DEC_OFIFO_CH);
	}

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_FIFO_CH), bData);

	McDevIf_ExecutePacket();

	return bOldCh;
}

/***************************************************************************
 *	GetChBit
 *
 *	Function:
 *			Acquisition channel bit width
 *	Arguments:
 *			dTargetFifo	Target fifo
 *			psConnectEx	pointer of ext connect info
 *			psBitWidth	pointer of bit width info
 *	Return:
 *			ch & bit
 *
 ****************************************************************************/
static UINT8 GetChBit(UINT32 dTargetFifo,
		struct CONNECTION_EX_INFO *psConnectEx,
		struct BIT_WIDTH_INFO *psBitWidth)
{
	UINT8 bData;

	bData = 0x00;

	if (FIFO_NONE != (FIFO_EFIFO_MASK & dTargetFifo)) {
		if (psConnectEx->bEfifoCh == 4) {
			if (psBitWidth->bEfifoBit == 32)
				bData |= MCB_DEC_EFIFO_CH_4_32;
			else
				bData |= MCB_DEC_EFIFO_CH_4_16;
		} else {
			if (psBitWidth->bEfifoBit == 32)
				bData |= MCB_DEC_EFIFO_CH_2_32;
			else
				bData |= MCB_DEC_EFIFO_CH_2_16;
		}
	}

	if (FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo)) {
		if (psConnectEx->bOfifoCh == 4) {
			if (psBitWidth->bOfifoBit == 32)
				bData |= MCB_DEC_OFIFO_CH_4_32;
			else
				bData |= MCB_DEC_OFIFO_CH_4_16;
		} else {
			if (psBitWidth->bOfifoBit == 32)
				bData |= MCB_DEC_OFIFO_CH_2_32;
			else
				bData |= MCB_DEC_OFIFO_CH_2_16;
		}
	}

	return bData;
}

/***************************************************************************
 *	ReSetFifoCh
 *
 *	Function:
 *			Reset fifo ch
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			old fifo Ch
 *
 ****************************************************************************/
static UINT8 ReSetFifoCh(UINT32 dCoderId)
{
	UINT8 bData;
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	dFifoId = GetFifoId(dCoderId);
	bData = GetChBit(dFifoId, &(psDecInfo->sConnectEx),
						&(psDecInfo->sBitWidth));

	bData = SetFifoCh(dFifoId, bData);

	return bData;
}

/****************************************************************************
 *	ReSetConnection
 *
 *	Function:
 *			SetConnection setting by saved information
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 ReSetConnection(UINT32 dCoderId)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	sParam.bCommandId = CDSP_CMD_HOST2OS_SYS_SET_CONNECTION;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] =
						psDecInfo->sConnect.bInSource;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01] =
						psDecInfo->sConnect.bOutDest;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);

	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	ReSetFifoCh(dCoderId);

	return sdRet;
}

/****************************************************************************
 *	DecOpen
 *
 *	Function:
 *			Secure resource.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DecOpen(UINT32 dCoderId)
{
	UINT8 bData;
	SINT32 i;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* Command register Initialize */
	CommandInitialize(dCoderId);

	/* TimerReset command (Off) */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_TIMER_RESET;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = (UINT8)TIMERRESET_OFF;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* DEC/ENC SFR,EVT Interrupt flag clear */
	if (CODER_DEC == dCoderId) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FLG), &bData, 1);
		bData |= MCB_ENC_FLG_SFR | MCB_DEC_EVT_FLG;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FLG), bData);
	} else {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_FLG), &bData, 1);
		bData |= MCB_ENC_FLG_SFR | MCB_ENC_EVT_FLG;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_FLG), bData);
	}
	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					&bData, 1);
	if (CODER_DEC == dCoderId)
		bData |= MCB_IRQFLAG_DEC;
	else
		bData |= MCB_IRQFLAG_ENC;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					bData);
	McDevIf_ExecutePacket();

	/* DEC/ENC SFR,EVT Interrupt Enable */
	if (CODER_DEC == dCoderId) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_ENABLE),
					&bData, 1);
		bData |= MCB_EDEC_SFR | MCB_EDEC_EVT;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_ENABLE),
					bData);
	} else {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_ENABLE), &bData, 1);
		bData |= MCB_EENC_SFR | MCB_EENC_EVT;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_ENABLE),
					bData);
	}
	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_ECDSP),
					&bData, 1);
	if (CODER_DEC == dCoderId)
		bData |= MCB_EDEC;
	else
		bData |= MCB_EENC;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);
	McDevIf_ExecutePacket();

	/* Initialize */
	psDecInfo->bChangeOutputFs = CHANGE_OUTPUT_FS_OFF;
	psDecInfo->bPreInputDataEnd = INPUT_DATAEND_RELEASE;
	psDecInfo->bInputDataEnd = INPUT_DATAEND_RELEASE;
	psDecInfo->bFmtPropagate = FORMAT_PROPAGATE_OFF;
	psDecInfo->wErrorCode = DEC_ERR_NO_ERROR;
	psDecInfo->sFormat.bFs = CODER_FMT_FS_48000;
	psDecInfo->sFormat.bE2BufMode = CODER_FMT_ETOBUF_LRMIX;
	psDecInfo->sConnect.bInSource = CDSP_IN_SOURCE_NONE;
	psDecInfo->sConnect.bOutDest = CDSP_OUT_DEST_NONE;
	psDecInfo->sConnectEx.bEfifoCh = 2;
	psDecInfo->sConnectEx.bOfifoCh = 2;
	psDecInfo->sBitWidth.bEfifoBit = 16;
	psDecInfo->sBitWidth.bOfifoBit = 16;
	psDecInfo->sParams.bCommandId = 0;
	for (i = 0; i < (SINT32)CDSP_CMD_PARAM_ARGUMENT_NUM; i++)
		psDecInfo->sParams.abParam[i] = 0;

	FifoInit(dCoderId,
			(FIFO_DFIFO | FIFO_EFIFO | FIFO_OFIFO | FIFO_RFIFO));

	sdRet = ReSetConnection(dCoderId);

	return sdRet;
}

/****************************************************************************
 *	DecReset
 *
 *	Function:
 *			Reset decoder/encoder.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DecReset(UINT32 dCoderId)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* Reset command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_RESET;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Complete NotifyOutFormat */
	CompleteNotifyOutputFormat(dCoderId);

	/* Command register Initialize */
	CommandInitialize(dCoderId);

	/* Input data end flag - release */
	psDecInfo->bPreInputDataEnd = INPUT_DATAEND_RELEASE;
	psDecInfo->bInputDataEnd = INPUT_DATAEND_RELEASE;

	psDecInfo->sFormat.bFs = CODER_FMT_FS_48000;
	psDecInfo->sFormat.bE2BufMode = CODER_FMT_ETOBUF_LRMIX;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ClearInputPosition
 *
 *	Function:
 *			Clear the input position.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ClearInputPosition(UINT32 dCoderId)
{
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	if ((CDSP_IN_SOURCE_EFIFO != psDecInfo->sConnect.bInSource) &&
		(CDSP_IN_SOURCE_DFIFO_EFIFO != psDecInfo->sConnect.bInSource))
		return;

	/* ENC_POS Write (Suitable value) */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_POS4), 0x00);

	McDevIf_ExecutePacket();

	psDecInfo->dInPosSup = 0;
}

/****************************************************************************
 *	ResetInputPos
 *
 *	Function:
 *			reset input position
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ResetInputPos(UINT32 dCoderId)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	if (CDSP_IN_SOURCE_OTHER_OUTBUF != psDecInfo->sConnect.bInSource)
		return;

	/* ResetInputPos command */
	sParam.bCommandId = CDSP_CMD_HOST2OS_SYS_RESET_INPUT_POS;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);

	psDecInfo->dInPosSup = 0;
}

/****************************************************************************
 *	ClearOutputPosition
 *
 *	Function:
 *			Clear the output position.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void ClearOutputPosition(UINT32 dCoderId)
{
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	if ((CDSP_OUT_DEST_OFIFO != psDecInfo->sConnect.bOutDest)
		&& (CDSP_OUT_DEST_OFIFO_RFIFO
		!= psDecInfo->sConnect.bOutDest))
		return;

	/* DEC_POS Write (Suitable value) */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DEC_POS4), 0x00);

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	DecClear
 *
 *	Function:
 *			Reset decoder/encoder 
 *			(Excluding the parameter setting).
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DecClear(UINT32 dCoderId)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* Clear command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_CLEAR;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Complete NotifyOutFormat */
	CompleteNotifyOutputFormat(dCoderId);

	/* TimerReset command (Reset) */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_TIMER_RESET;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = (UINT8)TIMERRESET_RESET;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* ChangeOutSamplingRate - Not complete */
	psDecInfo->bChangeOutputFs = CHANGE_OUTPUT_FS_OFF;

	/* Input data end state - release */
	psDecInfo->bPreInputDataEnd = INPUT_DATAEND_RELEASE;
	psDecInfo->bInputDataEnd = INPUT_DATAEND_RELEASE;

	/* Output Start - clear */
	switch (psDecInfo->sConnect.bOutDest) {
	case CDSP_OUT_DEST_OFIFO:
		gsFifoInfo.bOFifoOutStart = OUTPUT_START_OFF;
		break;

	case CDSP_OUT_DEST_OFIFO_RFIFO:
		gsFifoInfo.bOFifoOutStart = OUTPUT_START_OFF;
		gsFifoInfo.bRFifoOutStart = OUTPUT_START_OFF;
		break;

	case CDSP_OUT_DEST_RFIFO:
		gsFifoInfo.bRFifoOutStart = OUTPUT_START_OFF;
		break;

	default:
		break;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetFormat
 *
 *	Function:
 *			Set format.
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of send parameter.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetFormat(UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* Argument check */
	switch (psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_00]) {
	case CODER_FMT_FS_48000:
	case CODER_FMT_FS_44100:
	case CODER_FMT_FS_32000:
	case CODER_FMT_FS_24000:
	case CODER_FMT_FS_22050:
	case CODER_FMT_FS_16000:
	case CODER_FMT_FS_12000:
	case CODER_FMT_FS_11025:
	case CODER_FMT_FS_8000:
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	switch (psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_01]) {
	case CODER_FMT_ETOBUF_LRMIX:
	case CODER_FMT_ETOBUF_LCH:
	case CODER_FMT_ETOBUF_RCH:
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	sParam.bCommandId = psParam->bCommandId;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_00];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_01];
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	psDecInfo->sFormat.bFs = sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00];
	psDecInfo->sFormat.bE2BufMode =
				sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01];

	return sdRet;
}

/****************************************************************************
 *	SetInputDataEnd
 *
 *	Function:
 *			Set input data end.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetInputDataEnd(UINT32 dCoderId)
{
	SINT32 sdRet;
	UINT8 bInSource;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;
	default:
		return MCDRV_ERROR_STATE;
	}

	/* Path check */
	bInSource = psDecInfo->sConnect.bInSource;
	if ((CDSP_IN_SOURCE_DFIFO != bInSource)
		&& (CDSP_IN_SOURCE_DFIFO_EFIFO != bInSource))
		return MCDRV_ERROR;

	if (STATE_READY_SETUP == psDecInfo->dState) {
		psDecInfo->bPreInputDataEnd = INPUT_DATAEND_SET;
		return MCDRV_SUCCESS;
	}
	psDecInfo->bPreInputDataEnd = INPUT_DATAEND_RELEASE;

	/* FormatPropagate flag clear */
	psDecInfo->bFmtPropagate = FORMAT_PROPAGATE_OFF;

	/* InputDataEnd command */
	sdRet = CommandInputDataEnd(dCoderId);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Input data end state - set */
	psDecInfo->bInputDataEnd = INPUT_DATAEND_SET;

	/* Output Start */
	OutputStart(psDecInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetTimer
 *
 *	Function:
 *			Set timer. (unit: 1ms)
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of send parameter.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetTimer(UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* SetTimer command */
	sParam.bCommandId = psParam->bCommandId;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_00];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_01];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_02] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_02];
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_03] =
				psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_03];
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetDualMono
 *
 *	Function:
 *			Set dual mono playback mode.
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of send parameter.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetDualMono(UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* Argument check */
	switch (psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_00]) {
	case CODER_DUALMONO_LR:
	case CODER_DUALMONO_L:
	case CODER_DUALMONO_R:
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	/* Path check */
	switch (psDecInfo->sConnect.bOutDest) {
	case CDSP_OUT_DEST_OFIFO:
	case CDSP_OUT_DEST_OFIFO_RFIFO:
		break;

	default:
		return MCDRV_ERROR;
	}

	/* SetDualMono command */
	sParam.bCommandId = psParam->bCommandId;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00]
				= psParam->abParam[CDSP_CMD_PARAM_ARGUMENT_00];
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	GetInputPos
 *
 *	Function:
 *			get input position (unit: sample)
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of send parameter.
 *	Return:
 *			0 <=		sample
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 GetInputPos(UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	UINT32 dInputPos;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* Path check */
	if (CDSP_IN_SOURCE_OTHER_OUTBUF != psDecInfo->sConnect.bInSource)
		return MCDRV_ERROR;

	/* pcm input ? */
	if ((psDecInfo->sProgInfo.wInOutType & PRG_PRM_IOTYPE_IN_MASK)
		!= PRG_PRM_IOTYPE_IN_PCM)
		return MCDRV_ERROR;

	/* GetInputPos command */
	sParam.bCommandId = psParam->bCommandId;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* InputPos */
	dInputPos = ((UINT32)sParam.abParam[CDSP_CMD_PARAM_RESULT_03] << 24)
		| ((UINT32)sParam.abParam[CDSP_CMD_PARAM_RESULT_02] << 16)
		| ((UINT32)sParam.abParam[CDSP_CMD_PARAM_RESULT_01] << 8)
		| (UINT32)sParam.abParam[CDSP_CMD_PARAM_RESULT_00];

	dInputPos += psDecInfo->dInPosSup;

	return (SINT32)dInputPos;
}

/****************************************************************************
 *	DecStandby
 *
 *	Function:
 *			Standby decoder/encoder process.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 DecStandby(UINT32 dCoderId)
{
	UINT32 dFifoId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	psDecInfo = GetDecInfo(dCoderId);

	/* TimerReset command (Reset) */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_SYS_TIMER_RESET;
	sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = (UINT8)TIMERRESET_RESET;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* Standby command */
	sParam.bCommandId = (UINT8)CDSP_CMD_HOST2OS_CMN_STANDBY;
	sdRet = CommandWriteHost2Os(dCoderId, &sParam);
	if ((SINT32)MCDRV_SUCCESS > sdRet)
		return sdRet;

	/* ChangeOutSamplingRate - Not complete */
	psDecInfo->bChangeOutputFs = CHANGE_OUTPUT_FS_OFF;

	dFifoId = GetFifoId(dCoderId);
	if (FIFO_NONE != (FIFO_OFIFO_MASK & dFifoId))
		gsFifoInfo.bOFifoOutStart = OUTPUT_START_OFF;

	if (FIFO_NONE != (FIFO_RFIFO_MASK & dFifoId))
		gsFifoInfo.bRFifoOutStart = OUTPUT_START_OFF;

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	TermFunction
 *
 *	Function:
 *			Terminate function
 *	Arguments:
 *			dCoderId	Coder Id
 *	Return:
 *			None
 *
 ****************************************************************************/
static void TermFunction(UINT32 dCoderId)
{
	UINT32 dTargetFifo;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_PLAYING:
		DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_ON);
		FifoStop(dCoderId);

	case STATE_READY_SETUP:
	case STATE_READY:
		DecClose(dCoderId);

		/* Terminate current program */
		TerminateProgram(dCoderId);
		break;

	case STATE_NOTINIT:
	case STATE_INIT:
	default:
		return;
	}

	dTargetFifo = GetFifoId(dCoderId);
	if (FIFO_NONE != (FIFO_DFIFO_MASK & dTargetFifo))
		gsFifoInfo.sdDFifoCbPos = CBPOS_DFIFO_DEF;

	if (FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo)) {
		gsFifoInfo.dOFifoBufSample = OFIFO_BUF_SAMPLE_DEF;
		gsFifoInfo.bOFifoOutStart = OUTPUT_START_OFF;
	}

	if (FIFO_NONE != (FIFO_RFIFO_MASK & dTargetFifo)) {
		gsFifoInfo.sdRFifoCbPos = CBPOS_RFIFO_DEF;
		gsFifoInfo.dRFifoBufSample = RFIFO_BUF_SAMPLE_DEF;
		gsFifoInfo.bRFifoOutStart = OUTPUT_START_OFF;
	}

	InitDecInfo(psDecInfo);

	return;
}

/***************************************************************************
 *	SetFirmware
 *
 *	Function:
 *			Set firmware
 *	Arguments:
 *			psFuncInfo	Pointer of func info
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetFirmware(struct AEC_CDSP_FUNC_INFO *psFuncInfo)
{
	UINT8 bDownLoad;
	UINT16 wTemp;
	UINT32 dNewProgId;
	UINT32 dCurProgId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_FIRMWARE sProgram;

	if ((0UL == psFuncInfo->dProgSize) || (NULL == psFuncInfo->pbProg))
		return MCDRV_SUCCESS;

	if (CDSP_ERR_NO_ERROR != gsCdspInfo.wHwErrorCode)
		return MCDRV_ERROR_STATE;

	sProgram.pbFirmware = psFuncInfo->pbProg;
	sProgram.dSize = psFuncInfo->dProgSize;
	psDecInfo = GetDecInfo(psFuncInfo->dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_INIT:
		bDownLoad = PROGRAM_DOWNLOAD;
		break;

	case STATE_READY_SETUP:
	case STATE_READY:
		/* Check Program ID */
		dCurProgId  = psDecInfo->sProgInfo.wVendorId;
		dCurProgId |= (UINT32)psDecInfo->sProgInfo.wFunctionId << 16;

		wTemp = MAKEWORD(sProgram.pbFirmware[PRG_DESC_VENDER_ID],
				sProgram.pbFirmware[PRG_DESC_VENDER_ID+1]);
		dNewProgId  = (UINT32)wTemp;

		wTemp = MAKEWORD(sProgram.pbFirmware[PRG_DESC_FUNCTION_ID],
				sProgram.pbFirmware[PRG_DESC_FUNCTION_ID+1]);
		dNewProgId |= (UINT32)wTemp << 16;
		if (dNewProgId != dCurProgId)
			bDownLoad = PROGRAM_DOWNLOAD;
		else
			bDownLoad = PROGRAM_NO_DOWNLOAD;
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
		DecClose(psFuncInfo->dCoderId);

		/* Terminate current program */
		TerminateProgram(psFuncInfo->dCoderId);

		psDecInfo->dState = STATE_INIT;
		break;
	default:
		break;
	}

	if (PROGRAM_DOWNLOAD == bDownLoad) {
		/* Download */
		sdRet = DownloadProgram(sProgram.pbFirmware);
		if ((SINT32)MCDRV_SUCCESS != sdRet)
			return sdRet;
	}

	/* Initialize */

	sdRet = InitializeProgram(psFuncInfo->dCoderId, &sProgram);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	sdRet = DecOpen(psFuncInfo->dCoderId);
	if ((SINT32)MCDRV_SUCCESS != sdRet) {
		/* Terminate current program */
		TerminateProgram(psFuncInfo->dCoderId);
		return sdRet;
	}

	psDecInfo->dState = STATE_READY_SETUP;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetConnection
 *
 *	Function:
 *			Connect
 *	Arguments:
 *			psFuncInfo	Pointer of Func Info
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetConnection(struct AEC_CDSP_FUNC_INFO *psFuncInfo)
{
	UINT8 bData;
	SINT32 sdRet;
	UINT32 dTargetFifo;
	UINT32 dInitFifo;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;
	struct CONNECTION_INFO sConnect;
	struct CONNECTION_EX_INFO sConnectEx;
	struct BIT_WIDTH_INFO sBitWidth;

	psDecInfo = GetDecInfo(psFuncInfo->dCoderId);

	sConnect.bInSource = psFuncInfo->sConnect.bInSource;
	sConnect.bOutDest = psFuncInfo->sConnect.bOutDest;

	sConnectEx.bEfifoCh = psFuncInfo->sConnectEx.bEfifoCh;
	sConnectEx.bOfifoCh = psFuncInfo->sConnectEx.bOfifoCh;
	sBitWidth.bEfifoBit = psFuncInfo->sBitWidth.bEfifoBit;
	sBitWidth.bOfifoBit = psFuncInfo->sBitWidth.bOfifoBit;

	if ((psDecInfo->sConnect.bInSource == sConnect.bInSource)
		&& (psDecInfo->sConnect.bOutDest == sConnect.bOutDest)
		&& (psDecInfo->sBitWidth.bEfifoBit == sBitWidth.bEfifoBit)
		&& (psDecInfo->sConnectEx.bEfifoCh == sConnectEx.bEfifoCh)
		&& (psDecInfo->sBitWidth.bOfifoBit == sBitWidth.bOfifoBit)
		&& (psDecInfo->sConnectEx.bOfifoCh == sConnectEx.bOfifoCh))
		return MCDRV_SUCCESS;

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	dInitFifo = FIFO_NONE;
	if ((psDecInfo->sConnect.bInSource != sConnect.bInSource)
		|| (psDecInfo->sConnect.bOutDest != sConnect.bOutDest)) {

		switch (psDecInfo->dState) {
		case STATE_READY:
			sdRet = DecReset(psFuncInfo->dCoderId);
			if ((SINT32)MCDRV_SUCCESS > sdRet)
				return sdRet;

			ClearInputPosition(psFuncInfo->dCoderId);
			ResetInputPos(psFuncInfo->dCoderId);
			ClearOutputPosition(psFuncInfo->dCoderId);

			psDecInfo->dState = STATE_READY_SETUP;
			break;

		default:
			break;
		}

		sParam.bCommandId = CDSP_CMD_HOST2OS_SYS_SET_CONNECTION;
		sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_00] =
							sConnect.bInSource;
		sParam.abParam[CDSP_CMD_PARAM_ARGUMENT_01] = sConnect.bOutDest;
		sdRet = CommandWriteHost2Os(psFuncInfo->dCoderId, &sParam);
		if ((SINT32)MCDRV_SUCCESS > sdRet)
			return sdRet;

		dTargetFifo = 0;
		if (psDecInfo->sConnect.bInSource != sConnect.bInSource) {
			switch (sConnect.bInSource) {
			case CDSP_IN_SOURCE_DFIFO:
				if (psDecInfo->sConnect.bInSource
					!= CDSP_IN_SOURCE_DFIFO_EFIFO)
					dTargetFifo |= FIFO_DFIFO;
				break;

			case CDSP_IN_SOURCE_EFIFO:
				if (psDecInfo->sConnect.bInSource
					!= CDSP_IN_SOURCE_DFIFO_EFIFO)
					dTargetFifo |= FIFO_EFIFO;
				break;

			case CDSP_IN_SOURCE_DFIFO_EFIFO:
				if (psDecInfo->sConnect.bInSource
					!= CDSP_IN_SOURCE_DFIFO)
					dTargetFifo |= FIFO_DFIFO;

				if (psDecInfo->sConnect.bInSource
					!= CDSP_IN_SOURCE_EFIFO)
					dTargetFifo |= FIFO_EFIFO;
				break;

			default:
				break;
			}

			psDecInfo->sConnect.bInSource = sConnect.bInSource;
		}

		if (psDecInfo->sConnect.bOutDest != sConnect.bOutDest) {
			switch (sConnect.bOutDest) {
			case CDSP_OUT_DEST_OFIFO:
				if (psDecInfo->sConnect.bOutDest
					!= CDSP_OUT_DEST_OFIFO_RFIFO)
					dTargetFifo |= FIFO_OFIFO;
				break;

			case CDSP_OUT_DEST_RFIFO:
				if (psDecInfo->sConnect.bOutDest
					!= CDSP_OUT_DEST_OFIFO_RFIFO)
					dTargetFifo |= FIFO_RFIFO;
				break;

			case CDSP_OUT_DEST_OFIFO_RFIFO:
				if (psDecInfo->sConnect.bOutDest
					!= CDSP_OUT_DEST_OFIFO)
					dTargetFifo |= FIFO_OFIFO;

				if (psDecInfo->sConnect.bOutDest
					!= CDSP_OUT_DEST_RFIFO)
					dTargetFifo |= FIFO_RFIFO;
				break;

			default:
				break;
			}

			psDecInfo->sConnect.bOutDest = sConnect.bOutDest;
		}

		if (dTargetFifo != FIFO_NONE) {
			FifoInit(psFuncInfo->dCoderId, dTargetFifo);
			dInitFifo = dTargetFifo;
		}
	}


	dTargetFifo = GetFifoId(psFuncInfo->dCoderId);
	if ((psDecInfo->sBitWidth.bEfifoBit == sBitWidth.bEfifoBit)
		&& (psDecInfo->sConnectEx.bEfifoCh == sConnectEx.bEfifoCh))
		dTargetFifo &= ~FIFO_EFIFO_MASK;

	if ((psDecInfo->sBitWidth.bOfifoBit == sBitWidth.bOfifoBit)
		&& (psDecInfo->sConnectEx.bOfifoCh == sConnectEx.bOfifoCh))
		dTargetFifo &= ~FIFO_OFIFO_MASK;

	if (FIFO_NONE != ((FIFO_EFIFO_MASK | FIFO_OFIFO_MASK) & dTargetFifo)) {
		bData = GetChBit(dTargetFifo, &sConnectEx, &sBitWidth);

		if (FIFO_NONE != (FIFO_EFIFO_MASK & dTargetFifo)) {
			/* Clear position */
			ClearInputPosition(psFuncInfo->dCoderId);

			psDecInfo->sConnectEx.bEfifoCh = sConnectEx.bEfifoCh;
			psDecInfo->sBitWidth.bEfifoBit = sBitWidth.bEfifoBit;
		}

		if (FIFO_NONE != (FIFO_OFIFO_MASK & dTargetFifo)) {
			/* Clear position */
			ClearOutputPosition(psFuncInfo->dCoderId);

			psDecInfo->sConnectEx.bOfifoCh = sConnectEx.bOfifoCh;
			psDecInfo->sBitWidth.bOfifoBit = sBitWidth.bOfifoBit;
		}

		/* Clear FIFO */
		dInitFifo = (dTargetFifo & ~dInitFifo);
		if (FIFO_NONE != dInitFifo)
			FifoReset(psFuncInfo->dCoderId, dInitFifo);

		SetFifoCh(dTargetFifo, bData);
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetParamCore
 *
 *	Function:
 *			Set paramter
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of parameter
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetParamCore(UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	SINT32 i;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);
	/* State check */
	switch (psDecInfo->dState) {
	case STATE_INIT:
		if (CDSP_CMD_HOST2OS_SYS_SET_CLOCK_SOURCE
			!= psParam->bCommandId)
			return MCDRV_ERROR_STATE;
		break;

	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	if ((psParam->bCommandId & (UINT8)CDSP_CMD_HOST2OS_COMPLETION) == 0) {
		/* Command check */
		switch (psParam->bCommandId) {
		case CDSP_CMD_HOST2OS_CMN_RESET:
			/* reset */
			sdRet = DecReset(dCoderId);
			if ((SINT32)MCDRV_SUCCESS <= sdRet) {
				/* Clear position */
				ClearInputPosition(dCoderId);
				ResetInputPos(dCoderId);
				ClearOutputPosition(dCoderId);

				/* Clear FIFO */
				FifoReset(dCoderId,
					(FIFO_DFIFO | FIFO_EFIFO
					| FIFO_OFIFO | FIFO_RFIFO));

				/* ReSetConnect */
				ReSetConnection(dCoderId);

				psDecInfo->dState = STATE_READY_SETUP;
			}
			break;

		case CDSP_CMD_HOST2OS_CMN_CLEAR:
			sdRet = DecClear(dCoderId);
			if ((SINT32)MCDRV_SUCCESS <= sdRet) {
				/* Clear position */
				ClearInputPosition(dCoderId);
				ResetInputPos(dCoderId);
				ClearOutputPosition(dCoderId);

				/* Clear FIFO */
				FifoReset(dCoderId,
					(FIFO_DFIFO | FIFO_EFIFO
					| FIFO_OFIFO | FIFO_RFIFO));
			}
			break;

		case CDSP_CMD_HOST2OS_SYS_INPUT_DATA_END:
			sdRet = SetInputDataEnd(dCoderId);
			break;

		case CDSP_CMD_HOST2OS_SYS_SET_TIMER:
			sdRet = SetTimer(dCoderId, psParam);
			break;

		case CDSP_CMD_HOST2OS_SYS_SET_DUAL_MONO:
			sdRet = SetDualMono(dCoderId, psParam);
			break;

		case CDSP_CMD_HOST2OS_SYS_GET_INPUT_POS:
			sdRet = GetInputPos(dCoderId, psParam);
			break;

		default:
			if ((psParam->bCommandId < CDSP_CMD_HOST2OS_PRG_MIN)
				|| (psParam->bCommandId
				> CDSP_CMD_HOST2OS_PRG_MAX))
				return MCDRV_ERROR_ARGUMENT;

			/* Program dependence command */
			sdRet = CommandWriteHost2Os(dCoderId, psParam);
			if ((SINT32)MCDRV_SUCCESS <= sdRet)
				sdRet = MCDRV_SUCCESS;
			break;
		}
	} else {
		/* Host command notify completion */
		switch (psDecInfo->sParams.bCommandId) {
		case CDSP_CMD_OS2HOST_CMN_NONE:
			return MCDRV_ERROR;

		case CDSP_CMD_OS2HOST_CMN_NOTIFY_OUT_FORMAT:
			psDecInfo->bChangeOutputFs = CHANGE_OUTPUT_FS_ON;
			OutputStart(psDecInfo);
			break;

		default:
			break;
		}

		/* Write complete command */
		CommandWriteComplete(dCoderId, psParam);

		/* clear */
		psDecInfo->sParams.bCommandId =
					(UINT8)CDSP_CMD_OS2HOST_CMN_NONE;
		for (i = 0; i < (SINT32)CDSP_CMD_PARAM_ARGUMENT_NUM; i++)
			psDecInfo->sParams.abParam[i] = 0;

		sdRet = MCDRV_SUCCESS;
	}

	return sdRet;
}

/****************************************************************************
 *	GetParamCore
 *
 *	Function:
 *			Get parameter
 *	Arguments:
 *			dCoderId	Coder ID
 *			psParam		Pointer of get paramter
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 GetParamCore(
			UINT32 dCoderId, struct MC_CODER_PARAMS *psParam)
{
	SINT32 sdCount;
	SINT32 i;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	/* Command */
	psParam->bCommandId = psDecInfo->sParams.bCommandId;

	/* Argument */
	sdCount = (SINT32)(CDSP_CMD_PARAM_ARGUMENT_00
					+ CDSP_CMD_PARAM_ARGUMENT_NUM);
	for (i = (SINT32)CDSP_CMD_PARAM_ARGUMENT_00; i < sdCount; i++)
		psParam->abParam[i] = psDecInfo->sParams.abParam[i];

	/* Result */
	sdCount = (SINT32)(CDSP_CMD_PARAM_RESULT_00
					+ CDSP_CMD_PARAM_RESULT_NUM);
	for (i = (SINT32)CDSP_CMD_PARAM_RESULT_00; i < sdCount; i++)
		psParam->abParam[i] = 0;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetParam
 *
 *	Function:
 *			Set parameter
 *	Arguments:
 *			psFuncInfo
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetParam(struct AEC_CDSP_FUNC_INFO *psFuncInfo)
{
	UINT8 *pbPrm;
	UINT32 i;
	SINT32 j;
	SINT32 sdCount;
	SINT32 sdRet;
	struct MC_CODER_PARAMS sParam;

	if ((psFuncInfo->pbParam == NULL) || (psFuncInfo->dParamNum == 0))
		return MCDRV_SUCCESS;

	pbPrm = psFuncInfo->pbParam;

	sdCount = (SINT32)(CDSP_CMD_PARAM_ARGUMENT_00
					+ CDSP_CMD_PARAM_ARGUMENT_NUM);

	for (i = 0; i < psFuncInfo->dParamNum; i++) {
		sParam.bCommandId = pbPrm[CDSP_PRM_CMD];

		for (j = (SINT32)CDSP_CMD_PARAM_ARGUMENT_00; j < sdCount; j++)
			sParam.abParam[j] = pbPrm[CDSP_PRM_PRM0 + j];

		sdRet = SetParamCore(psFuncInfo->dCoderId, &sParam);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		pbPrm += PRM_UNIT_SIZE;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetE2BufMode
 *
 *	Function:
 *			Set E2BufMode
 *	Arguments:
 *			dCoderId	Coder ID
 *			psFuncInfo	Pointer of func info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void SetE2BufMode(UINT32 dCoderId,
				struct AEC_CDSP_FUNC_INFO *psFuncInfo)
{
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);
	psDecInfo->sFormat.bE2BufMode = psFuncInfo->sFormat.bE2BufMode;
}

/****************************************************************************
 *	SetCallbackPos
 *
 *	Function:
 *			Setting positon of the callback
 *	Arguments:
 *			dCoderId	Coder ID
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void SetCallbackPos(UINT32 dCoderId,
				struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	if ((gsFifoInfo.sdDFifoCbPos == psCdspInfo->sdDFifoCbPos) &&
		(gsFifoInfo.sdRFifoCbPos == psCdspInfo->sdRFifoCbPos))
		return;

	psDecInfo = GetDecInfo(dCoderId);
	dFifoId = GetFifoIdCore(&psDecInfo->sConnect);

	if ((dFifoId & FIFO_DFIFO_MASK) != 0)
		gsFifoInfo.sdDFifoCbPos = psCdspInfo->sdDFifoCbPos;

	if ((dFifoId & FIFO_RFIFO_MASK) != 0)
		gsFifoInfo.sdRFifoCbPos = psCdspInfo->sdRFifoCbPos;
}

/****************************************************************************
 *	SetBuffering
 *
 *	Function:
 *			Buffering positon of the callback
 *	Arguments:
 *			dCoderId	Coder ID
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void SetBuffering(UINT32 dCoderId,
				struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	if ((gsFifoInfo.dOFifoBufSample == psCdspInfo->dOFifoBufSample)
		&& (gsFifoInfo.dRFifoBufSample == psCdspInfo->dRFifoBufSample))
		return;

	psDecInfo = GetDecInfo(dCoderId);
	dFifoId = GetFifoIdCore(&psDecInfo->sConnect);

	if ((dFifoId & FIFO_OFIFO_MASK) != 0)
		gsFifoInfo.dOFifoBufSample = psCdspInfo->dOFifoBufSample;

	if ((dFifoId & FIFO_RFIFO_MASK) != 0)
		gsFifoInfo.dRFifoBufSample = psCdspInfo->dRFifoBufSample;
}

/***************************************************************************
 *	DFifoStart
 *
 *	Function:
 *			Start DFIFO.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void DFifoStart(void)
{
	UINT8 bIrqPntH;
	UINT8 bIrqPntL;
	UINT8 bIntCtl;
	UINT8 bData;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_ENABLE), &bIntCtl, 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_IRQ_PNT_H), &bIrqPntH, 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_IRQ_PNT_L), &bIrqPntL, 1);

	if ((GetDFifoSel() == CDSP_FIFO_SEL_PORT) ||
		((SINT32)CBPOS_DFIFO_NONE == gsFifoInfo.sdDFifoCbPos)) {
		bIrqPntH = 0;
		bIrqPntL = 0;
		bIntCtl &= ~MCB_DFIFO_EDPNT;
	} else {
		bIrqPntH = (UINT8)(gsFifoInfo.sdDFifoCbPos >> 8) &
							MCB_DFIFO_IRQ_PNT_H;
		bIrqPntL = (UINT8)(gsFifoInfo.sdDFifoCbPos) &
							MCB_DFIFO_IRQ_PNT_L;
		bIntCtl |= MCB_DFIFO_EDPNT;
	}

	bIntCtl |= MCB_DFIFO_EDEMP;

	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_FLG),
					MCB_DFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_DFIFO);

	/* xFIFO_IRQ_PNT Set */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_IRQ_PNT_H),
					bIrqPntH);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_IRQ_PNT_L),
					bIrqPntL);

	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt Enable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_ENABLE),
					bIntCtl);

	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
	bData |= MCB_EDFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	EFifoStart
 *
 *	Function:
 *			Start EFIFO.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void EFifoStart(void)
{
	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO_FLG),
					MCB_EFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_EFIFO);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	OFifoStart
 *
 *	Function:
 *			Start OFIFO.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void OFifoStart(UINT32 dCoderId)
{
	UINT8 bIrqPntH;
	UINT8 bIrqPntL;
	UINT8 bData;
	UINT32 dBufSample;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_FLG),
					MCB_OFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_OFIFO);

	/* xFIFO_IRQ_PNT Set */
	dBufSample = (gsFifoInfo.dOFifoBufSample *
			(UINT32)psDecInfo->sConnectEx.bOfifoCh *
			(UINT32)(psDecInfo->sBitWidth.bOfifoBit / 8)) / 4UL;
	bIrqPntH = (UINT8)(dBufSample >> 8) & MCB_OFIFO_IRQ_PNT_H;
	bIrqPntL = (UINT8)(dBufSample) & MCB_OFIFO_IRQ_PNT_L;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_IRQ_PNT_H),
					bIrqPntH);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_IRQ_PNT_L),
					bIrqPntL);

	/* xFIFO/xPNT Interrupt Enable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OFIFO_ENABLE),
					MCB_OFIFO_EOPNT);
	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
	bData |= MCB_EOFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);
	McDevIf_ExecutePacket();

	OutputStart(psDecInfo);
}

/***************************************************************************
 *	RFifoStartPort
 *
 *	Function:
 *			Start RFIFO.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void RFifoStartPort(UINT32 dCoderId)
{
	UINT8 bIrqPntH;
	UINT8 bIrqPntL;
	UINT8 bData;
	UINT32 dBufSample;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_FLG),
					MCB_RFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_RFIFO);

	/* xFIFO_IRQ_PNT Set */
	dBufSample = (gsFifoInfo.dRFifoBufSample * RFIFO_CH_NUM
						* RFIFO_BIT_WIDTH/8) / 4UL;
	bIrqPntH = (UINT8)(dBufSample >> 8) & MCB_RFIFO_IRQ_PNT_H;
	bIrqPntL = (UINT8)(dBufSample) & MCB_RFIFO_IRQ_PNT_L;

	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_H),
					bIrqPntH);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_L),
					bIrqPntL);

	/* xFIFO/xPNT/xOVF Interrupt Enable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE),
					MCB_RFIFO_ERPNT);
	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
	bData |= MCB_ERFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);
	McDevIf_ExecutePacket();

	OutputStart(psDecInfo);
}

/***************************************************************************
 *	RFifoStartHost
 *
 *	Function:
 *			Start RFIFO.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
static void RFifoStartHost(void)
{
	UINT8 bIrqPntH;
	UINT8 bIrqPntL;
	UINT8 bData;
	UINT8 bIntCtrl;

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_RFIFO_ENABLE), &bIntCtrl, 1);

	bIntCtrl |= MCB_RFIFO_EROVF;
	if ((SINT32)CBPOS_RFIFO_NONE != gsFifoInfo.sdRFifoCbPos) {
		bIrqPntH = (UINT8)(gsFifoInfo.sdRFifoCbPos >> 8) &
							MCB_RFIFO_IRQ_PNT_H;
		bIrqPntL = (UINT8)(gsFifoInfo.sdRFifoCbPos) &
							MCB_RFIFO_IRQ_PNT_L;
		bIntCtrl |= MCB_RFIFO_ERPNT;
	} else {
		bIrqPntH = 0;
		bIrqPntL = 0;
		bIntCtrl &= ~MCB_RFIFO_ERPNT;
	}

	/* xFIFO/xPNT/xEMP/xUDF/xOVF Interrupt flag clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_RFIFO_FLG),
						MCB_RFIFO_FLG_ALL);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_RFIFO);

	/* xFIFO_IRQ_PNT Set */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_H),
					bIrqPntH);
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_IRQ_PNT_L),
					bIrqPntL);

	/* xFIFO/xPNT/xOVF Interrupt Enable */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE),
					bIntCtrl);

	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					&bData, 1);
	bData |= MCB_ERFIFO;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_ECDSP),
					bData);

	McDevIf_ExecutePacket();
}

/***************************************************************************
 *	RFifoStart
 *
 *	Function:
 *			Start RFIFO.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void RFifoStart(UINT32 dCoderId)
{
	if (GetRFifoSel() == CDSP_FIFO_SEL_PORT)
		RFifoStartPort(dCoderId);
	else
		RFifoStartHost();
}

/***************************************************************************
 *	FifoStart
 *
 *	Function:
 *			Start FIFO.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			None
 *
 ****************************************************************************/
static void FifoStart(UINT32 dCoderId)
{
	UINT32 dFifoId;

	dFifoId = GetFifoId(dCoderId);
	if (FIFO_NONE != (FIFO_DFIFO_MASK & dFifoId))
		DFifoStart();

	if (FIFO_NONE != (FIFO_EFIFO_MASK & dFifoId))
		EFifoStart();

	if (FIFO_NONE != (FIFO_OFIFO_MASK & dFifoId))
		OFifoStart(dCoderId);

	if (FIFO_NONE != (FIFO_RFIFO_MASK & dFifoId))
		RFifoStart(dCoderId);
}

/****************************************************************************
 *	DecStart
 *
 *	Function:
 *			Start decoder/encoder.
 *	Arguments:
 *			dCoderId	Coder ID
 *	Return:
 *			0			success
 *			< 0			error code
 *
 ****************************************************************************/
static SINT32 DecStart(UINT32 dCoderId)
{
	UINT8 bData;
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;
	struct DEC_INFO *psOtherDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* Mutual output ? */
	if (CDSP_OUT_DEST_OTHER_INBUF == psDecInfo->sConnect.bOutDest) {
		psOtherDecInfo = GetOtherDecInfo(dCoderId);
		switch (psOtherDecInfo->dState) {
		case STATE_READY_SETUP:
		case STATE_READY:
		case STATE_PLAYING:
			break;
		default:
			return MCDRV_ERROR;
		}
	}

	FifoStart(dCoderId);

	/* DEC/ENC ERR,END Interrupt flag clear */
	if (CODER_DEC == dCoderId) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FLG), &bData, 1);
		bData |= MCB_DEC_FLG_ERR | MCB_DEC_FLG_END;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FLG), bData);
	} else {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_FLG), &bData, 1);
		bData |= MCB_ENC_FLG_ERR | MCB_ENC_FLG_END;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_FLG), bData);
	}
	McDevIf_ExecutePacket();

	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					&bData, 1);
	if (CODER_DEC == dCoderId)
		bData |= MCB_IRQFLAG_DEC;
	else
		bData |= MCB_IRQFLAG_ENC;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					bData);
	McDevIf_ExecutePacket();

	/* DEC/ENC END,ERR Interrupt Enable */
	if (CODER_DEC == dCoderId) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_ENABLE), &bData, 1);
		bData |= MCB_EDEC_ERR | MCB_EDEC_END;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_ENABLE), bData);
	} else {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_ENABLE), &bData, 1);
		bData |= MCB_EENC_ERR | MCB_EENC_END;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_ENC_ENABLE), bData);
	}
	McDevIf_ExecutePacket();

	/* DEC/ENC Start */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START), &bData, 1);
	if (CODER_DEC == psDecInfo->dCoderId)
		bData |= MCB_DEC_DEC_START;
	else
		bData |= MCB_DEC_ENC_START;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_START),
					bData);
	McDevIf_ExecutePacket();

	dFifoId = GetFifoIdCore(&psDecInfo->sConnect);
	if ((FIFO_EFIFO & dFifoId) != FIFO_NONE) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_CH), &bData, 1);
		bData |= MCB_DEC_EFIFO_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DEC_FIFO_CH),
					bData);
		McDevIf_ExecutePacket();
	}
	if (((FIFO_DFIFO & dFifoId) != FIFO_NONE)
		&& (GetDFifoSel() == CDSP_FIFO_SEL_PORT)) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL), &bData,
					1);
		bData |= MCB_RDFIFO_DFIFO_START;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					bData);
		McDevIf_ExecutePacket();
	}

	if (gsFifoInfo.dOFifoBufSample == 0)
		if ((FIFO_OFIFO & dFifoId) != FIFO_NONE)
			InterruptProcOFifoCore();

	if (gsFifoInfo.dRFifoBufSample == 0)
		if ((FIFO_RFIFO & dFifoId) != FIFO_NONE)
			if (GetRFifoSel() == CDSP_FIFO_SEL_PORT)
				InterruptProcRFifoPortCore();

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetExt
 *
 *	Function:
 *			Expand settings
 *	Arguments:
 *			dCoderId	Coder ID
 *			psFuncInfo	Pointer of func info
 *	Return:
 *			= 0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 SetExt(UINT32 dCoderId,
				struct AEC_CDSP_FUNC_INFO *psFuncInfo)
{
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParam;

	sdRet = MCDRV_SUCCESS;
	if (psFuncInfo->pbExt == NULL)
		return sdRet;

	psDecInfo = GetDecInfo(dCoderId);

	switch (psFuncInfo->pbExt[EXT_COMMAND]) {
	case EXT_COMMAND_CLEAR:
		sParam.bCommandId = CDSP_CMD_HOST2OS_CMN_CLEAR;

		/* State check */
		switch (psDecInfo->dState) {
		case STATE_PLAYING:
			sdRet = DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_ON);
			if ((SINT32)MCDRV_SUCCESS != sdRet)
				return sdRet;

			FifoStop(dCoderId);
			psDecInfo->dState = STATE_READY;

			sdRet = SetParamCore(dCoderId, &sParam);
			if (sdRet < MCDRV_SUCCESS)
				return sdRet;

			sdRet = DecStart(dCoderId);
			if ((SINT32)MCDRV_SUCCESS == sdRet)
				psDecInfo->dState = STATE_PLAYING;
			break;

		case STATE_READY_SETUP:
		case STATE_READY:
			sdRet = SetParamCore(dCoderId, &sParam);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}

	return sdRet;
}

/****************************************************************************
 *	SetRoute
 *
 *	Function:
 *			Setting route
 *	Arguments:
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			None
 *
 ****************************************************************************/
static void SetRoute(struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	if ((psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A].pbFifo == NULL)
		&& (psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B].pbFifo == NULL))
		return;

	/* OUT*R/L_SEL */
	if (psCdspInfo->bOut0Sel != gsFifoInfo.bOut0Sel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT0_SEL),
					psCdspInfo->bOut0Sel);
		gsFifoInfo.bOut0Sel = psCdspInfo->bOut0Sel;
	}

	if (psCdspInfo->bOut1Sel != gsFifoInfo.bOut1Sel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT1_SEL),
					psCdspInfo->bOut1Sel);
		gsFifoInfo.bOut1Sel = psCdspInfo->bOut1Sel;
	}

	if (psCdspInfo->bOut2Sel != gsFifoInfo.bOut2Sel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_OUT2_SEL),
					psCdspInfo->bOut2Sel);
		gsFifoInfo.bOut2Sel = psCdspInfo->bOut2Sel;
	}

	/* RFIFO_BIT/RFIFO_SEL/DFIFO_BIT/DFIFO_SEL */
	if (psCdspInfo->bRDFifoBitSel != gsFifoInfo.bRDFifoBitSel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					psCdspInfo->bRDFifoBitSel);
		gsFifoInfo.bRDFifoBitSel = psCdspInfo->bRDFifoBitSel;
	}

	/* EFIFO0*_SEL */
	if (psCdspInfo->bEFifo01Sel != gsFifoInfo.bEFifo01Sel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO01_SEL),
					psCdspInfo->bEFifo01Sel);
		gsFifoInfo.bEFifo01Sel = psCdspInfo->bEFifo01Sel;
	}

	if (psCdspInfo->bEFifo23Sel != gsFifoInfo.bEFifo23Sel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_EFIFO23_SEL),
					psCdspInfo->bEFifo23Sel);
		gsFifoInfo.bEFifo23Sel = psCdspInfo->bEFifo23Sel;
	}

	McDevIf_ExecutePacket();
}

/****************************************************************************
 *	SetDsp
 *
 *	Function:
 *			Set dsp
 *	Arguments:
 *			psCdspInfo	Pointer of AEC analyze info
 *	Return:
 *			None
 *
 ****************************************************************************/
static SINT32 SetDsp(struct MCDRV_CDSP_AEC_CDSP_INFO *psCdspInfo)
{
	SINT32 sdRet;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoA;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoB;

	psFuncInfoA = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_A];
	psFuncInfoB = &psCdspInfo->asFuncInfo[AEC_FUNC_INFO_B];

	if (psFuncInfoA->bFuncOnOff != CDSP_FUNC_OFF) {
		sdRet = SetFirmware(psFuncInfoA);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		sdRet = SetConnection(psFuncInfoA);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		sdRet = SetParam(psFuncInfoA);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		SetE2BufMode(CODER_DEC, psFuncInfoA);

		SetCallbackPos(CODER_DEC, psCdspInfo);

		SetBuffering(CODER_DEC, psCdspInfo);

		sdRet = SetExt(CODER_DEC, psFuncInfoA);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;
	}

	if (psFuncInfoB->bFuncOnOff != CDSP_FUNC_OFF) {
		sdRet = SetFirmware(psFuncInfoB);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		sdRet = SetConnection(psFuncInfoB);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		sdRet = SetParam(psFuncInfoB);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;

		SetE2BufMode(CODER_ENC, psFuncInfoB);

		SetCallbackPos(CODER_ENC, psCdspInfo);

		SetBuffering(CODER_ENC, psCdspInfo);

		sdRet = SetExt(CODER_ENC, psFuncInfoB);
		if (sdRet < MCDRV_SUCCESS)
			return sdRet;
	}

	SetRoute(psCdspInfo);

	return MCDRV_SUCCESS;
}

/***************************************************************************
 *	GetRFifoRemain
 *
 *	Function:
 *			Get readable size of output FIFO.
 *	Arguments:
 *			pdRemain	Pointer of readable size
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static void GetRFifoRemain(UINT32 *pdRemain)
{
	UINT8 bRFifoPntH;
	UINT8 bRFifoPntL;
	UINT32 dRemainSize;

	/* Read RFIFO Remain size */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_POINTER_H),
					&bRFifoPntH, 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_POINTER_L),
					&bRFifoPntL, 1);

	dRemainSize = MAKEWORD(bRFifoPntL, bRFifoPntH);

	*pdRemain = dRemainSize;
}

/***************************************************************************
 *	ReadData
 *
 *	Function:
 *			Read record data.
 *	Arguments:
 *			pbData		Record data
 *			dSize		Record data size
 *	Return:
 *			0 <=		Record data Size
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 ReadData(UINT8 *pbData, UINT32 dSize)
{
	UINT8 bIntCtrl;
	UINT32 dRemainSize;

	/* Read RFIFO Remain size */
	GetRFifoRemain(&dRemainSize);

	/* Read Byte */
	if (dRemainSize > dSize)
		dRemainSize = dSize;

	/* Register Information */
	if ((SINT32)CBPOS_RFIFO_NONE != gsFifoInfo.sdRFifoCbPos)
		bIntCtrl = MCB_RFIFO_ERPNT;
	else
		bIntCtrl = 0;

	/* Read */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF | (UINT32)MCI_DEC_FIFO),
							pbData, dRemainSize);

	/* IRQ Flag Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					MCB_IRQFLAG_RFIFO);

	/* RFIFO Flag Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_FLG),
					MCB_RFIFO_FLG_ALL);

	/* ERPNT, EROVF Enable */
	bIntCtrl |= MCB_RFIFO_EROVF;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RFIFO_ENABLE),
					bIntCtrl);

	McDevIf_ExecutePacket();

	return (SINT32)dRemainSize;
}

/***************************************************************************
 *	GetDFifoRemain
 *
 *	Function:
 *			Get writable size of input FIFO.
 *	Arguments:
 *			pdRemain	Pointer of writable size
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static void GetDFifoRemain(UINT32 *pdRemain)
{
	UINT8 bDFifoPntH;
	UINT8 bDFifoPntL;
	UINT32 dRemainSize;

	/* Read DFIFO Remain size */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_POINTER_H),
					&bDFifoPntH, 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_DFIFO_POINTER_L),
					&bDFifoPntL, 1);
	dRemainSize = MAKEWORD(bDFifoPntL, bDFifoPntH);

	dRemainSize = (UINT32)(FIFOSIZE_DFIFO - dRemainSize);

	if (dRemainSize < DFIFO_DUMMY_SPACE)
		dRemainSize = 0;
	else
		dRemainSize -= DFIFO_DUMMY_SPACE;

	*pdRemain = dRemainSize;
}

/***************************************************************************
 *	WriteData
 *
 *	Function:
 *			Write play data to FIFO.
 *	Arguments:
 *			pbData		Play data
 *			dSize		Play data size
 *	Return:
 *			0 <=		Play Data Size
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 WriteData(const UINT8 *pbData, UINT32 dSize)
{
	UINT8 bIntCtrl;
	UINT8 bData;
	UINT32 dRemainSize;
	UINT32 i;

	/* Read DFIFO Remain size */
	GetDFifoRemain(&dRemainSize);

	/* Write Byte */
	if (dRemainSize < dSize)
		dSize = dRemainSize;

	/* Register Information */
	if ((SINT32)CBPOS_DFIFO_NONE != gsFifoInfo.sdDFifoCbPos)
		bIntCtrl = MCB_DFIFO_EDPNT;
	else
		bIntCtrl = 0;

	/* DMODE Set */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_RESET), &bData, 1);
	bData |= MCB_CDSP_DMODE;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_CDSP_RESET), bData);

	McDevIf_ExecutePacket();

	/* Write */
	for (i = 0; i < dSize; ++i)
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_DEC_FIFO),
					pbData[i]);

	McDevIf_ExecutePacket();

	/* IRQ Flag Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_CDSP),
				MCB_IRQFLAG_DFIFO);

	/* DFIFO Flag Clear */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_FLG),
				MCB_DFIFO_FLG_ALL);

	/* EDPNT, EDEMP Enable */
	bIntCtrl |= MCB_DFIFO_EDEMP;
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
				| MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_DFIFO_ENABLE),
				bIntCtrl);

	McDevIf_ExecutePacket();

	gsFifoInfo.dDFifoWriteSize += dSize;

	return (SINT32)dSize;
}

/****************************************************************************
 *	ReadDataCore
 *
 *	Function:
 *			Read record data
 *	Arguments:
 *			dCoderId	Coder ID
 *			pbBuffer	Pointer of record data
 *			dSize		Record data size
 *	Return:
 *			0 <=		Read Data Size
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 ReadDataCore(UINT32 dCoderId, UINT8 *pbBuffer, UINT32 dSize)
{
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	dFifoId = GetFifoId(dCoderId);
	if ((dFifoId & FIFO_RFIFO_MASK) == 0)
		return MCDRV_ERROR;

	return ReadData(pbBuffer, dSize);
}

/****************************************************************************
 *	WriteDataCore
 *
 *	Function:
 *			Write play data to FIFO
 *	Arguments:
 *			dCoderId	Coder Id
 *			pbBuffer	Play data
 *			dSize		Play data size
 *	Return:
 *			0 <=		Play Data Size
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 WriteDataCore(UINT32 dCoderId,
					const UINT8 *pbBuffer, UINT32 dSize)
{
	UINT32 dFifoId;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	dFifoId = GetFifoId(dCoderId);
	if ((dFifoId & FIFO_DFIFO_MASK) == 0)
		return MCDRV_ERROR;

	if (INPUT_DATAEND_RELEASE != psDecInfo->bInputDataEnd)
		return MCDRV_ERROR;

	return WriteData(pbBuffer, dSize);
}

/****************************************************************************
 *	GetInputPosition
 *
 *	Function:
 *			Get present input position (unit of ms). 
 *	Arguments:
 *			pdPos		Pointer of input position
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
static SINT32 GetInputPosition(UINT32 *pdPos)
{
	UINT8 abInputPos[4];
	UINT32 dInputPos;
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(CODER_DEC);
	/* Path check */
	switch (psDecInfo->sConnect.bInSource) {
	case CDSP_IN_SOURCE_DFIFO:
	case CDSP_IN_SOURCE_OTHER_OUTBUF:
	case CDSP_IN_SOURCE_NONE:
		psDecInfo = GetDecInfo(CODER_ENC);

		switch (psDecInfo->sConnect.bInSource) {
		case CDSP_IN_SOURCE_DFIFO:
		case CDSP_IN_SOURCE_OTHER_OUTBUF:
		case CDSP_IN_SOURCE_NONE:
			*pdPos = 0;
			return sizeof(*pdPos);
		}
		break;

	default:
		break;
	}

	/* pcm input ? */
	if ((psDecInfo->sProgInfo.wInOutType & PRG_PRM_IOTYPE_IN_MASK)
		!= PRG_PRM_IOTYPE_IN_PCM) {
		*pdPos = 0;
		return sizeof(*pdPos);
	}

	/* ENC_POS Read */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_ENC_POS1), &abInputPos[3], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_ENC_POS2), &abInputPos[2], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_ENC_POS3), &abInputPos[1], 1);
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
				| (UINT32)MCI_ENC_POS4), &abInputPos[0], 1);
	dInputPos = ((UINT32)abInputPos[3] << 24)
				| ((UINT32)abInputPos[2] << 16)
				| ((UINT32)abInputPos[1] << 8)
				| (UINT32)abInputPos[0];

	dInputPos += psDecInfo->dInPosSup;

	/* sample -> msec */
	if ((psDecInfo->sConnectEx.bEfifoCh == 4)
		|| ((psDecInfo->sConnectEx.bEfifoCh == 2)
		&& (psDecInfo->sBitWidth.bOfifoBit == 32)))
		dInputPos /= 2;

	*pdPos = ConvertSamplesToTime(psDecInfo->sFormat.bFs, dInputPos);

	return sizeof(*pdPos);
}

/****************************************************************************
 *	GetHwErrorCode
 *
 *	Function:
 *			Get the error code of the error notified
 *			by the callback.
 *	Arguments:
 *			pwErrorCode	Pointer in area where error code
 *					is received.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 GetHwErrorCode(UINT16 *pwErrorCode)
{
	*pwErrorCode = gsCdspInfo.wHwErrorCode;

	return sizeof(*pwErrorCode);
}

/****************************************************************************
 *	McCdsp_GetErrorCode
 *
 *	Function:
 *			Get the error code of the error notified
 *			by the callback.
 *	Arguments:
 *			ePlayerId	Player ID
 *			pwErrorCode	Pointer in area where error code
 *					is received.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 GetErrorCode(UINT32 dCoderId, UINT16 *pwErrorCode)
{
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);
	*pwErrorCode = psDecInfo->wErrorCode;

	return sizeof(*pwErrorCode);
}

/****************************************************************************
 *	GetVersion
 *
 *	Function:
 *			Get CDSP OS and program version.
 *	Arguments:
 *			psVersion	Pointer of structure
 *					that receives version.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 GetVersion(UINT32 dCoderId, struct MC_CODER_VERSION *psVersion)
{
	struct DEC_INFO *psDecInfo;

	psDecInfo = GetDecInfo(dCoderId);
	/* State check */
	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
	case STATE_READY:
	case STATE_PLAYING:
		psVersion->dID = (UINT32)psDecInfo->sProgInfo.wVendorId;
		psVersion->dID +=
				(UINT32)psDecInfo->sProgInfo.wFunctionId << 16;
		psVersion->wProgVerH = psDecInfo->sProgVer.wVersionH;
		psVersion->wProgVerM = 0;
		psVersion->wProgVerL = psDecInfo->sProgVer.wVersionL;
		psVersion->wOsVerH = gsCdspInfo.sOsVer.wVersionH;
		psVersion->wOsVerM = 0;
		psVersion->wOsVerL = gsCdspInfo.sOsVer.wVersionL;
		break;

	default:
		psVersion->dID = 0;
		psVersion->wProgVerH = 0;
		psVersion->wProgVerM = 0;
		psVersion->wProgVerL = 0;
		psVersion->wOsVerH = 0;
		psVersion->wOsVerM = 0;
		psVersion->wOsVerL = 0;
		break;
	}

	return sizeof(*psVersion);
}

/***************************************************************************
 *	McCdsp_Init
 *
 *	Function:
 *			Initialize CDSP.
 *	Arguments:
 *			psPrm		Pointer of MCDRV_CDSP_INIT
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_Init(struct MCDRV_CDSP_INIT *psPrm)
{
	SINT32 sdRet;

	(void)psPrm;
	sdRet = Initialize();

	return sdRet;
}

/***************************************************************************
 *	McCdsp_Term
 *
 *	Function:
 *			Terminate CDSP.
 *	Arguments:
 *			None
 *	Return:
 *			0		success
 *
 ****************************************************************************/
SINT32 McCdsp_Term(void)
{
	UINT8 bData;

	/* CDSP stop */
	if ((STATE_NOTINIT != gsDecInfo.dState)
				|| (STATE_NOTINIT != gsEncInfo.dState)) {
		McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						&bData, 1);
		bData |= MCB_CDSP_SRST;
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
						| MCDRV_PACKET_REGTYPE_C
						| (UINT32)MCI_CDSP_RESET),
						bData);

		McDevIf_ExecutePacket();
	}

	gsDecInfo.dState = STATE_NOTINIT;
	gsEncInfo.dState = STATE_NOTINIT;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McCdsp_IrqProc
 *
 *	Function:
 *			It is processed the interrupt generation.
 *	Arguments:
 *			None
 *	Return:
 *			None
 *
 ****************************************************************************/
void McCdsp_IrqProc(void)
{
	UINT8 bData;

	if ((STATE_NOTINIT == gsDecInfo.dState)
		|| (STATE_NOTINIT == gsEncInfo.dState))
		return;

	/* Get interrupt flag */
	McDevIf_ReadDirect((MCDRV_PACKET_REGTYPE_IF
				| (UINT32)MCI_CDSP), &bData, 1);

	/* Interrupt */
	if (MCB_IRQFLAG_DEC == (MCB_IRQFLAG_DEC & bData))
		InterruptProcDec(CODER_DEC);

	if (MCB_IRQFLAG_ENC == (MCB_IRQFLAG_ENC & bData))
		InterruptProcDec(CODER_ENC);

	if (MCB_IRQFLAG_DFIFO == (MCB_IRQFLAG_DFIFO & bData))
		InterruptProcDFifo();

	/*if (MCB_IRQFLAG_EFIFO == (MCB_IRQFLAG_EFIFO & bData))*/
		/* None */

	if (MCB_IRQFLAG_OFIFO == (MCB_IRQFLAG_OFIFO & bData))
		InterruptProcOFifo();

	if (MCB_IRQFLAG_RFIFO == (MCB_IRQFLAG_RFIFO & bData))
		InterruptProcRFifo();

	/* if (MCB_IRQFLAG_FFIFO == (MCB_IRQFLAG_FFIFO & bData))*/
		/* None */

	if (MCB_IRQFLAG_CDSP == (MCB_IRQFLAG_CDSP & bData))
		InterruptProcCDsp();

	/* Clear interrupt flag */
	McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_IF
					| (UINT32)MCI_CDSP),
					bData);

	McDevIf_ExecutePacket();

	/* Callback */
	CallbackProc();
}

/****************************************************************************
 *	McCdsp_SetCallbackFunc
 *
 *	Function:
 *			Set callback function.
 *	Arguments:
 *			ePlayerId	Player ID
 *			pcbfunc		Pointer of callback function.
 *	Return:
 *			0		success
 *
 ****************************************************************************/
SINT32 McCdsp_SetCBFunc(enum MC_PLAYER_ID ePlayerId,
				SINT32 (*pcbfunc)(SINT32, UINT32, UINT32))
{
	UINT32 dCoderId;
	struct DEC_INFO *psDecInfo;

	/* arg check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	psDecInfo = GetDecInfo(dCoderId);
	psDecInfo->sCbInfo.pcbFunc = pcbfunc;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McCdsp_GetDSP
 *
 *	Function:
 *			Get dsp
 *	Arguments:
 *			dTarget		Target
 *			pvData		Pointer of data
 *			dSize		Data Size
 *	Return:
 *			0 <=		Get data size
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_GetDSP(UINT32 dTarget, void *pvData, UINT32 dSize)
{
	SINT32 sdRet;

	(void)dSize;
	if (pvData == NULL)
		return MCDRV_ERROR_ARGUMENT;

	switch (dTarget) {
	case CDSP_INPOS:
		sdRet = GetInputPosition((UINT32 *)pvData);
		break;

	case CDSP_OUTPOS:
		sdRet = GetOutputPosition(CODER_DEC, (UINT32 *)pvData);
		if (sdRet != MCDRV_SUCCESS)
			sdRet = GetOutputPosition(CODER_ENC, (UINT32 *)pvData);

		if (sdRet != MCDRV_SUCCESS)
			*((UINT32 *)pvData) = 0;

		sdRet = sizeof(UINT32);
		break;

	case CDSP_DFIFO_REMAIN:
		GetDFifoRemain((UINT32 *)pvData);
		sdRet = sizeof(UINT32);
		break;

	case CDSP_RFIFO_REMAIN:
		GetRFifoRemain((UINT32 *)pvData);
		sdRet = sizeof(UINT32);
		break;

	case CDSP_HW_ERROR_CODE:
		sdRet = GetHwErrorCode((UINT16 *)pvData);
		break;

	case CDSP_FUNC_A_ERROR_CODE:
		sdRet = GetErrorCode(CODER_DEC, (UINT16 *)pvData);
		break;

	case CDSP_FUNC_B_ERROR_CODE:
		sdRet = GetErrorCode(CODER_ENC, (UINT16 *)pvData);
		break;

	case CDSP_FUNC_A_VERSION:
		sdRet = GetVersion(CODER_DEC,
					(struct MC_CODER_VERSION *)pvData);
		break;

	case CDSP_FUNC_B_VERSION:
		sdRet = GetVersion(CODER_ENC,
					(struct MC_CODER_VERSION *)pvData);
		break;

	default:
		sdRet = MCDRV_ERROR_ARGUMENT;
		break;
	}

	return sdRet;
}

/****************************************************************************
 *	McCdsp_SetDSPCheck
 *
 *	Function:
 *			Check dsp setting
 *	Arguments:
 *			psPrm		Pointer of Aec info
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetDSPCheck(struct MCDRV_AEC_INFO *psPrm)
{
	SINT32 sdResult;
	struct MCDRV_CDSP_AEC_CDSP_INFO sCdspInfo;

	if (psPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetCDSPChunk(psPrm, &sCdspInfo);

	sdResult = CdspChunkAnalyze(&sCdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McCdsp_SetDSP
 *
 *	Function:
 *			Set dsp
 *	Arguments:
 *			pbPrm		Pointer of Aec info
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetDSP(struct MCDRV_AEC_INFO *pbPrm)
{
	SINT32 sdResult;
	struct MCDRV_CDSP_AEC_CDSP_INFO sCdspInfo;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoA;
	struct AEC_CDSP_FUNC_INFO *psFuncInfoB;

	psFuncInfoA = &sCdspInfo.asFuncInfo[AEC_FUNC_INFO_A];
	psFuncInfoB = &sCdspInfo.asFuncInfo[AEC_FUNC_INFO_B];

	if (pbPrm == NULL)
		return MCDRV_ERROR_ARGUMENT;

	GetCDSPChunk(pbPrm, &sCdspInfo);

	sdResult = CdspChunkAnalyze(&sCdspInfo);
	if (sdResult < (SINT32)MCDRV_SUCCESS)
		return sdResult;

	if (pbPrm->sAecVBox.bEnable == AEC_VBOX_ENABLE) {
		if (psFuncInfoA->bFuncOnOff == CDSP_FUNC_OFF)
			TermFunction(CODER_DEC);

		if (psFuncInfoB->bFuncOnOff == CDSP_FUNC_OFF)
			TermFunction(CODER_ENC);
	}

	if (((psFuncInfoA->pbChunk == NULL)
		|| (psFuncInfoA->dChunkSize == 0))
		&& ((psFuncInfoB->pbChunk == NULL)
		|| (psFuncInfoB->dChunkSize == 0)))
		return MCDRV_SUCCESS;

	if ((STATE_NOTINIT == gsDecInfo.dState)
		|| (STATE_NOTINIT == gsEncInfo.dState))
		return MCDRV_ERROR_STATE;

	sdResult = SetDsp(&sCdspInfo);

	return sdResult;
}

/****************************************************************************
 *	McCdsp_SetFs
 *
 *	Function:
 *			Set fs
 *	Arguments:
 *			ePlayerId	Player ID
 *			bFs		FS
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetFs(enum MC_PLAYER_ID ePlayerId, UINT8 bFs)
{
	UINT32 dCoderId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;
	struct MC_CODER_PARAMS sParams;

	if ((STATE_NOTINIT == gsDecInfo.dState)
		|| (STATE_NOTINIT == gsEncInfo.dState))
		return MCDRV_ERROR_STATE;

	/* arg check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	psDecInfo = GetDecInfo(dCoderId);

	sParams.bCommandId = CDSP_CMD_HOST2OS_SYS_SET_FORMAT;
	sParams.abParam[CDSP_CMD_PARAM_ARGUMENT_00] = bFs;
	sParams.abParam[CDSP_CMD_PARAM_ARGUMENT_01] =
					psDecInfo->sFormat.bE2BufMode;

	sdRet = SetFormat(dCoderId, &sParams);

	return sdRet;
}

/****************************************************************************
 *	McCdsp_SetDFifoSel
 *
 *	Function:
 *			Select Port/Host dfifo
 *	Arguments:
 *			bSel		Select Port/Host
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetDFifoSel(UINT8 bSel)
{
	UINT8 bRDFifoBitSel;

	if ((bSel != CDSP_FIFO_SEL_PORT) && (bSel != CDSP_FIFO_SEL_HOST))
		return MCDRV_ERROR_ARGUMENT;

	if ((STATE_NOTINIT == gsDecInfo.dState)
		|| (STATE_NOTINIT == gsEncInfo.dState))
		return MCDRV_ERROR_STATE;

	bRDFifoBitSel = gsFifoInfo.bRDFifoBitSel;
	bRDFifoBitSel &= ~MCB_DFIFO_SEL;
	if (bSel == CDSP_FIFO_SEL_HOST)
		bRDFifoBitSel |= MCB_DFIFO_SEL_HOST;

	/* DFIFO_SEL */
	if (gsFifoInfo.bRDFifoBitSel != bRDFifoBitSel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					bRDFifoBitSel);
		McDevIf_ExecutePacket();

		gsFifoInfo.bRDFifoBitSel = bRDFifoBitSel;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McCdsp_SetRFifoSel
 *
 *	Function:
 *			Select Port/Host rfifo
 *	Arguments:
 *			bSel		Select Port/Host
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetRFifoSel(UINT8 bSel)
{
	UINT8 bRDFifoBitSel;

	if ((bSel != CDSP_FIFO_SEL_PORT) && (bSel != CDSP_FIFO_SEL_HOST))
		return MCDRV_ERROR_ARGUMENT;

	if ((STATE_NOTINIT == gsDecInfo.dState)
		|| (STATE_NOTINIT == gsEncInfo.dState))
		return MCDRV_ERROR_STATE;

	bRDFifoBitSel = gsFifoInfo.bRDFifoBitSel;
	bRDFifoBitSel &= ~MCB_RFIFO_SEL;
	if (bSel == CDSP_FIFO_SEL_HOST)
		bRDFifoBitSel |= MCB_RFIFO_SEL_HOST;

	/* RFIFO_SEL */
	if (gsFifoInfo.bRDFifoBitSel != bRDFifoBitSel) {
		McDevIf_AddPacket((MCDRV_PACKET_TYPE_FORCE_WRITE
					| MCDRV_PACKET_REGTYPE_C
					| (UINT32)MCI_RDFIFO_BIT_SEL),
					bRDFifoBitSel);

		McDevIf_ExecutePacket();

		gsFifoInfo.bRDFifoBitSel = bRDFifoBitSel;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McCdsp_Start
 *
 *	Function:
 *			Begin main process.
 *	Arguments:
 *			ePlayerId	Player ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_Start(enum MC_PLAYER_ID ePlayerId)
{
	UINT32 dCoderId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;

	/* arg check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	psDecInfo = GetDecInfo(dCoderId);

	switch (psDecInfo->dState) {
	case STATE_READY_SETUP:
		sdRet = DecStandby(dCoderId);
		if ((SINT32)MCDRV_SUCCESS != sdRet)
			return sdRet;

		FifoReset(dCoderId,
			(FIFO_DFIFO | FIFO_EFIFO | FIFO_OFIFO | FIFO_RFIFO));

		psDecInfo->dState = STATE_READY;

		if (INPUT_DATAEND_SET == psDecInfo->bPreInputDataEnd) {
			sdRet = SetInputDataEnd(dCoderId);
			if ((SINT32)MCDRV_SUCCESS != sdRet)
				return sdRet;
		}
		break;

	case STATE_READY:
		FifoReset(dCoderId, (FIFO_DFIFO | FIFO_EFIFO));
		break;

	default:
		return MCDRV_ERROR_STATE;
	}


	sdRet = DecStart(dCoderId);
	if ((SINT32)MCDRV_SUCCESS == sdRet)
		psDecInfo->dState = STATE_PLAYING;

	return sdRet;
}

/****************************************************************************
 *	McCdsp_Stop
 *
 *	Function:
 *			Stop main process.
 *	Arguments:
 *			ePlayerId	Player ID
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_Stop(enum MC_PLAYER_ID ePlayerId)
{
	UINT32 dCoderId;
	SINT32 sdRet;
	struct DEC_INFO *psDecInfo;

	/* arg check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	psDecInfo = GetDecInfo(dCoderId);

	/* State check */
	switch (psDecInfo->dState) {
	case STATE_PLAYING:
		break;

	default:
		return MCDRV_ERROR_STATE;
	}

	sdRet = DecStop(dCoderId, MADEVCDSP_VERIFY_COMP_ON);
	if ((SINT32)MCDRV_SUCCESS != sdRet)
		return sdRet;

	FifoStop(dCoderId);

	psDecInfo->dState = STATE_READY;

	return sdRet;
}

/****************************************************************************
 *	McCdsp_GetParam
 *
 *	Function:
 *			Get the parameter of the HOST command demanded
 *			by the callback.
 *	Arguments:
 *			ePlayerId	Player ID
 *			psParam		Pointer of structure
 *					that receives parameter.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_GetParam(enum MC_PLAYER_ID ePlayerId,
					struct MC_CODER_PARAMS	*psParam)
{
	UINT32 dCoderId;
	SINT32 sdRet;

	/* Argument check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	if (NULL == psParam)
		return MCDRV_ERROR_ARGUMENT;

	sdRet = GetParamCore(dCoderId, psParam);

	return sdRet;
}

/****************************************************************************
 *	McCdsp_SetParam
 *
 *	Function:
 *			Send command.
 *	Arguments:
 *			ePlayerId	Player ID
 *			psParam		Pointer of structure
 *					that sends parameter.
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_SetParam(enum MC_PLAYER_ID ePlayerId,
					struct MC_CODER_PARAMS *psParam)
{
	UINT32 dCoderId;
	SINT32 sdRet;

	/* arg check */
	switch (ePlayerId) {
	case eMC_PLAYER_CODER_A:
		dCoderId = CODER_DEC;
		break;
	case eMC_PLAYER_CODER_B:
		dCoderId = CODER_ENC;
		break;
	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	if (NULL == psParam)
		return MCDRV_ERROR_ARGUMENT;

	sdRet = SetParamCore(dCoderId, psParam);

	return sdRet;
}

/****************************************************************************
 *	McCdsp_ReadData
 *
 *	Function:
 *			Read data from FIFO.
 *	Arguments:
 *			pbBuffer	Read data buffer.
 *			dSize		Data size (unit: byte)
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_ReadData(UINT8 *pbBuffer, UINT32 dSize)
{
	UINT32 dCoderId;
	UINT32 dFifoId;
	SINT32 sdRet;

	if (NULL == pbBuffer)
		return MCDRV_ERROR_ARGUMENT;

	if (0UL == dSize)
		return MCDRV_ERROR_ARGUMENT;

	if (GetRFifoSel() != CDSP_FIFO_SEL_HOST)
		return 0;

	dCoderId = CODER_DEC;
	dFifoId = GetFifoId(dCoderId);
	if ((dFifoId & FIFO_RFIFO) == FIFO_NONE) {
		dCoderId = CODER_ENC;
		dFifoId = GetFifoId(dCoderId);
		if ((dFifoId & FIFO_RFIFO) == FIFO_NONE)
			return 0;
	}

	sdRet = ReadDataCore(dCoderId, pbBuffer, dSize);

	return sdRet;
}

/****************************************************************************
 *	McCdsp_WriteData
 *
 *	Function:
 *			Write byte-data to FIFO.
 *	Arguments:
 *			pbBuffer	Write data buffer.
 *			dSize		Data size (unit: byte)
 *	Return:
 *			0		success
 *			< 0		error code
 *
 ****************************************************************************/
SINT32 McCdsp_WriteData(const UINT8 *pbBuffer, UINT32 dSize)
{
	SINT32 sdRet;
	UINT32 dFifoId;
	UINT32 dCoderId;

	if (NULL == pbBuffer)
		return MCDRV_ERROR_ARGUMENT;

	if (0UL == dSize)
		return MCDRV_ERROR_ARGUMENT;

	if (GetDFifoSel() != CDSP_FIFO_SEL_HOST)
		return 0;

	dCoderId = CODER_DEC;
	dFifoId = GetFifoId(dCoderId);
	if ((dFifoId & FIFO_DFIFO) == FIFO_NONE) {
		dCoderId = CODER_ENC;
		dFifoId = GetFifoId(dCoderId);
		if ((dFifoId & FIFO_DFIFO) == FIFO_NONE)
			return 0;
	}

	sdRet = WriteDataCore(dCoderId, pbBuffer, dSize);

	return sdRet;
}

