/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcedspdrv.h
 *
 *		Description	: MC Edsp Driver header
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
#ifndef	_MCEDSPDRV_H_
#define	_MCEDSPDRV_H_

/* definition */

#define AEC_EDSP_ENABLE			(1)
#define AEC_EDSP_DISABLE		(0)

#define EDSP_E_OFF			(0)
#define EDSP_E_ON			(1)

#define EDSP_CB_CMD_STATUS		(21)

/* Enum */

#if defined(__cplusplus)
extern "C" {
#endif
SINT32 McEdsp_Init(void);
SINT32 McEdsp_Term(void);
SINT32 McEdsp_IrqProc(void);
SINT32 McEdsp_SetCBFunc(SINT32 (*cbfunc)(SINT32, UINT32, UINT32));
SINT32 McEdsp_GetDSP(UINT8 *pbData);
SINT32 McEdsp_SetDSPCheck(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McEdsp_SetDSP(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McEdsp_Start(void);
SINT32 McEdsp_Stop(void);
SINT32 McEdsp_E1Download(UINT8 *pbSysEq0, UINT8 *pbSysEq1, UINT8 bSysEqEnbReg);
SINT32 McEdsp_E1WriteCommand(UINT8 bCmd);
SINT32 McEdsp_E1OffsetMode(UINT8 bStart);
SINT32 McEdsp_E1Impedance(UINT8 bStart);
#if defined(__cplusplus)
}
#endif

#endif
