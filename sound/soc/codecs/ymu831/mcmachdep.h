/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcmachdep.h
 *
 *	Description	: MC Driver machine dependent part header
 *
 *	Version		: 1.0.5	2013.01.21
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

#ifndef _MCMACHDEP_H_
#define _MCMACHDEP_H_

#include "mctypedef.h"

#include "linux/string.h"
#include "linux/kernel.h"


#define	MCDRV_DEBUG_LEVEL	(0)


void	machdep_SystemInit(void);
void	machdep_SystemTerm(void);
void	machdep_ClockStart(void);
void	machdep_ClockStop(void);
void	machdep_WriteReg(UINT8 bSlaveAdr,
				 const UINT8 *pbData,
				 UINT32 dSize);
void	machdep_ReadReg(UINT8 bSlaveAdr,
				 UINT32 dAddress,
				 UINT8 *pbData,
				 UINT32 dSize);
void	machdep_Sleep(UINT32 dSleepTime);
void	machdep_Lock(void);
void	machdep_Unlock(void);
void	machdep_PreLDODStart(void);
void	machdep_PostLDODStart(void);
void	machdep_DebugPrint(UINT8 *pbLogString);


#endif /* _MCMACHDEP_H_ */
