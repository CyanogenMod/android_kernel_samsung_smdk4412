/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdebuglog.h
 *
 *	Description	: MC Driver debug log header
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

#ifndef _MCDEBUGLOB_H_
#define _MCDEBUGLOG_H_

#include "mcdriver.h"
#include "mcmachdep.h"

#if MCDRV_DEBUG_LEVEL


void		McDebugLog_CmdIn(UINT32 dCmd,
					const void *pvPrm1,
					const void *pvPrm2,
					UINT32 dUpdateInfo);
void		McDebugLog_CmdOut(UINT32 dCmd,
					const SINT32 *sdRet,
					const void *pvPrm1,
					const void *pvPrm2,
					UINT32 dPrm);

void		McDebugLog_FuncIn(void *pvFuncName);
void		McDebugLog_FuncOut(void *pvFuncName,
					const SINT32 *psdRet);


#endif	/*	MCDRV_DEBUG_LEVEL	*/

#endif /* _MCDEBUGLOG_H_ */
