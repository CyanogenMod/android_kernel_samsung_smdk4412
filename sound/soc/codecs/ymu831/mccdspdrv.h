/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mccdspdrv.h
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
#ifndef	_MCCDSPDRV_H_
#define	_MCCDSPDRV_H_

/* definition */

#define CDSP_FUNC_OFF					(0)
#define CDSP_FUNC_ON					(1)

/* callback */
#define CDSP_EVT_FS_CHG					(100)
#define CDSP_EVT_FS_CHG_REF				(101)

/* Command (Host -> OS) */
#define CDSP_CMD_HOST2OS_CMN_NONE			(0x00)
#define CDSP_CMD_HOST2OS_CMN_RESET			(0x01)
#define CDSP_CMD_HOST2OS_CMN_CLEAR			(0x02)
#define CDSP_CMD_HOST2OS_CMN_STANDBY			(0x03)
#define CDSP_CMD_HOST2OS_CMN_GET_PRG_VER		(0x04)
#define CDSP_CMD_HOST2OS_SYS_GET_OS_VER			(0x20)
#define CDSP_CMD_HOST2OS_SYS_SET_PRG_INFO		(0x21)
#define CDSP_CMD_HOST2OS_SYS_SET_FORMAT			(0x22)
#define CDSP_CMD_HOST2OS_SYS_SET_CONNECTION		(0x23)
#define CDSP_CMD_HOST2OS_SYS_VERIFY_STOP_COMP		(0x24)
#define CDSP_CMD_HOST2OS_SYS_INPUT_DATA_END		(0x25)
#define CDSP_CMD_HOST2OS_SYS_CLEAR_INPUT_DATA_END	(0x26)
#define CDSP_CMD_HOST2OS_SYS_SET_TIMER			(0x27)
#define CDSP_CMD_HOST2OS_SYS_TIMER_RESET		(0x28)
#define CDSP_CMD_HOST2OS_SYS_TERMINATE			(0x29)
#define CDSP_CMD_HOST2OS_SYS_SET_DUAL_MONO		(0x2A)
#define CDSP_CMD_HOST2OS_SYS_GET_INPUT_POS		(0x2B)
#define CDSP_CMD_HOST2OS_SYS_RESET_INPUT_POS		(0x2C)
#define CDSP_CMD_HOST2OS_SYS_HALT			(0x2D)
#define CDSP_CMD_HOST2OS_SYS_SET_CLOCK_SOURCE		(0x2E)
#define CDSP_CMD_DRV_SYS_SET_CONNECTION_EX		(0x3E)
#define CDSP_CMD_DRV_SYS_SET_BIT_WIDTH			(0x3F)
#define CDSP_CMD_HOST2OS_PRG_MIN			(0x40)
#define CDSP_CMD_HOST2OS_PRG_MAX			(0x6F)

/* Command (OS -> Host) */
#define CDSP_CMD_OS2HOST_CMN_NONE			(0x00)
#define CDSP_CMD_OS2HOST_CMN_MIN			(0x01)
#define CDSP_CMD_OS2HOST_CMN_MAX			(0x3F)
#define CDSP_CMD_OS2HOST_PRG_MIN			(0x40)
#define CDSP_CMD_OS2HOST_PRG_MAX			(0x6F)
#define CDSP_CMD_OS2HOST_DISABLE_MIN			(0x70)
#define CDSP_CMD_OS2HOST_DISABLE_MAX			(0x7F)
#define CDSP_CMD_OS2HOST_READY_MIN			(0x80)
#define CDSP_CMD_OS2HOST_READY_MAX			(0xFF)
#define CDSP_CMD_OS2HOST_CMN_NOTIFY_OUT_FORMAT		(0x01)

/* Command parameter */
#define CDSP_CMD_PARAM_ARGUMENT_00			(0)
#define CDSP_CMD_PARAM_ARGUMENT_01			(1)
#define CDSP_CMD_PARAM_ARGUMENT_02			(2)
#define CDSP_CMD_PARAM_ARGUMENT_03			(3)
#define CDSP_CMD_PARAM_ARGUMENT_04			(4)
#define CDSP_CMD_PARAM_ARGUMENT_05			(5)
#define CDSP_CMD_PARAM_ARGUMENT_06			(6)
#define CDSP_CMD_PARAM_ARGUMENT_07			(7)
#define CDSP_CMD_PARAM_ARGUMENT_08			(8)
#define CDSP_CMD_PARAM_ARGUMENT_09			(9)
#define CDSP_CMD_PARAM_ARGUMENT_10			(10)
#define CDSP_CMD_PARAM_ARGUMENT_11			(11)
#define CDSP_CMD_PARAM_ARGUMENT_NUM			(12)
#define CDSP_CMD_PARAM_RESULT_00			(12)
#define CDSP_CMD_PARAM_RESULT_01			(13)	
#define CDSP_CMD_PARAM_RESULT_02			(14)
#define CDSP_CMD_PARAM_RESULT_03			(15)
#define CDSP_CMD_PARAM_RESULT_NUM			(4)
#define CDSP_CMD_PARAM_NUM				\
	(CDSP_CMD_PARAM_ARGUMENT_NUM + CDSP_CMD_PARAM_RESULT_NUM)

/* Command Completion */
#define CDSP_CMD_HOST2OS_COMPLETION			(0x80)
#define CDSP_CMD_OS2HOST_COMPLETION			(0x80)

/* Connect FIFO */
#define CDSP_IN_SOURCE_DFIFO				(0)
#define CDSP_IN_SOURCE_EFIFO				(1)
#define CDSP_IN_SOURCE_OTHER_OUTBUF			(2)
#define CDSP_IN_SOURCE_NONE				(3)
#define CDSP_IN_SOURCE_DFIFO_EFIFO			(4)
#define CDSP_OUT_DEST_OFIFO				(0)
#define CDSP_OUT_DEST_RFIFO				(1)
#define CDSP_OUT_DEST_OTHER_INBUF			(2)
#define CDSP_OUT_DEST_NONE				(3)
#define CDSP_OUT_DEST_OFIFO_RFIFO			(4)

#define CDSP_INPOS					(1)
#define CDSP_OUTPOS					(2)
#define CDSP_DFIFO_REMAIN				(3)
#define CDSP_RFIFO_REMAIN				(4)
#define CDSP_HW_ERROR_CODE				(11)
#define CDSP_FUNC_A_ERROR_CODE				(12)
#define CDSP_FUNC_B_ERROR_CODE				(13)
#define CDSP_FUNC_A_VERSION				(14)
#define CDSP_FUNC_B_VERSION				(15)

#define CDSP_FIFO_SEL_PORT				(0)
#define CDSP_FIFO_SEL_HOST				(1)

/* Enum */

enum MC_PLAYER_ID {
	eMC_PLAYER_CODER_A,
	eMC_PLAYER_CODER_B
};

enum MC_INOUT_ID {
	eMC_INPUT,
	eMC_OUTPUT
};

enum MC_PLAYREC_ID {
	eMC_PLAY,
	eMC_REC
};

/* Struct */

struct MCDRV_CDSP_INIT {
	UINT8				bJtag;
};

struct MC_CODER_FIRMWARE {
	const UINT8			*pbFirmware;
	UINT32				dSize;
	UINT8				bResFlag;
};

struct MC_CODER_PARAMS {
	UINT8				bCommandId;
	UINT8				abParam[16];
};

struct MC_CODER_VERSION {
	UINT32				dID;
	UINT16				wProgVerH;
	UINT16				wProgVerM;
	UINT16				wProgVerL;
	UINT16				wOsVerH;
	UINT16				wOsVerM;
	UINT16				wOsVerL;
};

#if defined(__cplusplus)
extern "C" {
#endif

SINT32 McCdsp_Init(struct MCDRV_CDSP_INIT *psPrm);
SINT32 McCdsp_Term(void);
void McCdsp_IrqProc(void);
SINT32 McCdsp_SetCBFunc(enum MC_PLAYER_ID ePlayerId,
				SINT32 (*pcbfunc)(SINT32, UINT32, UINT32));
SINT32 McCdsp_GetDSP(UINT32 dTarget, void *pvData, UINT32 dSize);
SINT32 McCdsp_SetDSPCheck(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McCdsp_SetDSP(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McCdsp_SetFs(enum MC_PLAYER_ID ePlayerId, UINT8 bFs);
SINT32 McCdsp_SetDFifoSel(UINT8 bSel);
SINT32 McCdsp_SetRFifoSel(UINT8 bSel);
SINT32 McCdsp_Start(enum MC_PLAYER_ID ePlayerId);
SINT32 McCdsp_Stop(enum MC_PLAYER_ID ePlayerId);
SINT32 McCdsp_GetParam(enum MC_PLAYER_ID ePlayerId,
					struct MC_CODER_PARAMS *psParam);
SINT32 McCdsp_SetParam(enum MC_PLAYER_ID ePlayerId,
					struct MC_CODER_PARAMS *psParam);

SINT32 McCdsp_ReadData(UINT8 *pbBuffer, UINT32 dSize);
SINT32 McCdsp_WriteData(const UINT8 *pbBuffer, UINT32 dSize);
#if defined(__cplusplus)
}
#endif

#endif /* _MCCDSPDRV_H_ */
