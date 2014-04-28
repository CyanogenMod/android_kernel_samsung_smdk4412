/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcservice.h
 *
 *	Description	: MC Driver service routine header
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

#ifndef _MCSERVICE_H_
#define _MCSERVICE_H_

#include "mctypedef.h"


void	McSrv_SystemInit(void);
void	McSrv_SystemTerm(void);
void	McSrv_ClockStart(void);
void	McSrv_ClockStop(void);
void	McSrv_WriteReg(UINT8 bSlaveAddr, UINT8 *pbData, UINT32 dSize);
UINT8	McSrv_ReadReg(UINT8 bSlaveAddr, UINT32 dRegAddr);
void	McSrv_ReadRegN(UINT8 bSlaveAddr, UINT32 dRegAddr, UINT8 *pbData,
			 UINT32 dSize);
void	McSrv_Sleep(UINT32 dSleepTime);
void	McSrv_Lock(void);
void	McSrv_Unlock(void);
void	McSrv_MemCopy(const UINT8 *pbSrc, UINT8 *pbDest, UINT32 dSize);
void	McSrv_DisableIrq(void);
void	McSrv_EnableIrq(void);



#endif /* _MCSERVICE_H_ */
