/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdebuglog.h
 *
 *		Description	: MC Driver debug log header
 *
 *		Version		: 1.0.0 	2010.08.16
 *
 ****************************************************************************/

#ifndef _MCDEBUGLOB_H_
#define _MCDEBUGLOG_H_

#include "mcdriver.h"
#include "mcmachdep.h"

#if MCDRV_DEBUG_LEVEL


void		McDebugLog_CmdIn	(UINT32 dCmd, const void* pvParam, UINT32 dUpdateInfo);
void		McDebugLog_CmdOut	(UINT32 dCmd, const SINT32* psdRet, const void* pvParam);

void		McDebugLog_FuncIn	(void* pvFuncName);
void		McDebugLog_FuncOut	(void* pvFuncName, const SINT32* psdRet);


#endif	/*	MCDRV_DEBUG_LEVEL	*/

#endif /* _MCDEBUGLOG_H_ */
