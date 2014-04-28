/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcfdspdrv.h
 *
 *		Description	: MC Fdsp Driver header
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
#ifndef	_MCFDSPDRV_H_
#define	_MCFDSPDRV_H_

/* definition */

#define	MCDRV_PROCESSING		((SINT32)1)

#define AEC_AUDIOENGINE_ENABLE		(1)
#define AEC_AUDIOENGINE_DISABLE		(0)

#define FDSP_OFF			(0)
#define FDSP_ON				(1)

#define FDSP_LOCATE_AUDIOENGINE		(0)
#define FDSP_LOCATE_V_BOX		(1)

#define FDSP_DRV_SUCCESS_STOPED		(1)

#define FDSP_PRC_FDSP_STOP		(0x1000000)
#define FDSP_PRC_COEF_TRS		(0x2000000)

#define FDSP_MUTE_NUM			(8)
#define FDSP_MUTE_NOTCHANGE		(0)
#define FDSP_MUTE_ON			(1)
#define FDSP_MUTE_OFF			(2)

#define FDSP_APP_NUM			(24)
#define FDSP_APP_EXEC_NOTCHANGE		(0)
#define FDSP_APP_EXEC_STOP		(0)
#define FDSP_APP_EXEC_START		(1)
#define FDSP_APP_EXEC_DONTCARE		(2)
#define FDSP_APP_FADE_NON		(0)
#define FDSP_APP_FADE_SHRT		(1)
#define FDSP_APP_FADE_MID		(2)
#define FDSP_APP_FADE_LONG		(3)

#define FDSP_AE_FW_DXRAM		(1)
#define FDSP_AE_FW_DYRAM		(2)
#define FDSP_AE_FW_IRAM			(3)

#define FDSP_CB_FDSP_STOP		(1)
#define FDSP_CB_APP_STOP		(2)
#define FDSP_CB_APP_REQ			(3)
#define FDSP_CB_COEF_DONE		(4)
#define FDSP_CB_ERR			(5)
#define FDSP_CB_FREQ			(100)
#define FDSP_CB_RESET			(101)

/* outside Typedef Struct */

struct MCDRV_FDSP_AUDIO_IO {
	UINT16				wADDFmt;
};

struct MCDRV_FDSP_INIT {
	struct MCDRV_FDSP_AUDIO_IO	sInput;
	struct MCDRV_FDSP_AUDIO_IO	sOutput;
	UINT32				dWaitTime;
};

struct MCDRV_FDSP_MUTE {
	UINT8				abInMute[FDSP_MUTE_NUM];
	UINT8				abOutMute[FDSP_MUTE_NUM];
};

#if defined(__cplusplus)
extern "C" {
#endif

SINT32 McFdsp_Init(struct MCDRV_FDSP_INIT *psPrm);
SINT32 McFdsp_Term(void);
void McFdsp_IrqProc(void);
SINT32 McFdsp_SetCBFunc(SINT32 (*cbfunc)(SINT32, UINT32, UINT32));
SINT32 McFdsp_GetTransition(void);
SINT32 McFdsp_GetDSP(UINT32 dTarget, UINT32 dAddress,
						UINT8 *pvData, UINT32 dSize);
SINT32 McFdsp_SetDSPCheck(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McFdsp_SetDSP(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McFdsp_Start(void);
SINT32 McFdsp_Stop(void);
SINT32 McFdsp_GetMute(struct MCDRV_FDSP_MUTE *psPrm);
SINT32 McFdsp_SetMute(struct MCDRV_FDSP_MUTE *psPrm);

#if defined(__cplusplus)
}
#endif

#endif /* _MCFDSPDRV_H_ */
