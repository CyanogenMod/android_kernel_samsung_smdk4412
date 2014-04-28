/****************************************************************************
 *
 *		Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcbdspdrv.h
 *
 *		Description	: MC Bdsp Driver header
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
#ifndef	_MCBDSPDRV_H_
#define	_MCBDSPDRV_H_

/* outside definition */

#define AEC_AUDIOENGINE_ENABLE		(1)
#define AEC_AUDIOENGINE_DISABLE		(0)

#define BDSP_OFF			(0)
#define BDSP_ON				(1)

#define BDSP_APP_EXEC_STOP		(0)
#define BDSP_APP_EXEC_START		(1)
#define BDSP_APP_EXEC_DONTCARE		(2)

#define BDSP_PRC_FADE_AE0		(0x01)
#define BDSP_PRC_FADE_AE1		(0x02)
#define BDSP_PRC_SINOUT			(0x04)
#define BDSP_PRC_COEF_TRS		(0x08)

#define BDSP_NORMAL			(0)
#define BDSP_BYPASS			(1)

#define BDSP_AE_FW_DXRAM		(1)
#define BDSP_AE_FW_DYRAM		(2)

/* outside Struct */
#if defined(__cplusplus)
extern "C" {
#endif

SINT32 McBdsp_Init(void);
SINT32 McBdsp_Term(void);
SINT32 McBdsp_GetTransition(void);
SINT32 McBdsp_GetDSP(UINT32 dTarget, UINT32 dAddress,
						UINT8 *pbData, UINT32 dSize);
SINT32 McBdsp_SetDSPCheck(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McBdsp_SetDSP(struct MCDRV_AEC_INFO *pbPrm);
SINT32 McBdsp_Start(void);
SINT32 McBdsp_Stop(void);

#if defined(__cplusplus)
}
#endif

#endif /* _MCBDSPDRV_H_ */
