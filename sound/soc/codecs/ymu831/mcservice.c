/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcservice.c
 *
 *	Description	: MC Driver service routine
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


#include "mcdefs.h"
#include "mcservice.h"
#include "mcmachdep.h"
#include "mcresctrl.h"
#include "mcdevprof.h"
#if (MCDRV_DEBUG_LEVEL >= 4)
#include "mcdebuglog.h"
#endif


/****************************************************************************
 *	McSrv_SystemInit
 *
 *	Description:
 *			Initialize the system.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_SystemInit(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_SystemInit");
#endif

	machdep_SystemInit();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_SystemInit", 0);
#endif
}

/****************************************************************************
 *	McSrv_SystemTerm
 *
 *	Description:
 *			Terminate the system.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_SystemTerm(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_SystemTerm");
#endif

	machdep_SystemTerm();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_SystemTerm", 0);
#endif
}

/****************************************************************************
 *	McSrv_ClockStart
 *
 *	Description:
 *			Start clock.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_ClockStart(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_ClockStart");
#endif

	machdep_ClockStart();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_ClockStart", 0);
#endif
}

/****************************************************************************
 *	McSrv_ClockStop
 *
 *	Description:
 *			Stop clock.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_ClockStop(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_ClockStop");
#endif

	machdep_ClockStop();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_ClockStop", 0);
#endif
}

/****************************************************************************
 *	McSrv_WriteReg
 *
 *	Description:
 *			Write data to register.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			pbData		data
 *			dSize		data size
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_WriteReg(
	UINT8	bSlaveAddr,
	UINT8	*pbData,
	UINT32	dSize
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_WriteReg");
#endif

	McSrv_DisableIrq();
	machdep_WriteReg(bSlaveAddr, pbData, dSize);
	McSrv_EnableIrq();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_WriteReg", 0);
#endif
}

/****************************************************************************
 *	McSrv_ReadReg
 *
 *	Function:
 *			Read a byte data from the register.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			dRegAddr	address of register
 *	Return:
 *			read data
 *
 ****************************************************************************/
UINT8	McSrv_ReadReg(
	UINT8	bSlaveAddr,
	UINT32	dRegAddr
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McSrv_ReadReg");
#endif

	McSrv_DisableIrq();
	machdep_ReadReg(bSlaveAddr, dRegAddr, &bReg, 1);
	McSrv_EnableIrq();

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)bReg;
	McDebugLog_FuncOut("McSrv_ReadReg", &sdRet);
#endif

	return bReg;
}

/****************************************************************************
 *	McSrv_ReadRegN
 *
 *	Function:
 *			Read a byte data from the register.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			dRegAddr	address of register
 *			pbData		pointer to read data buffer
 *			dSize		read count
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_ReadRegN(
	UINT8	bSlaveAddr,
	UINT32	dRegAddr,
	UINT8	*pbData,
	UINT32	dSize
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McSrv_ReadRegN");
#endif
	McSrv_DisableIrq();
	machdep_ReadReg(bSlaveAddr, dRegAddr, pbData, dSize);
	McSrv_EnableIrq();

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)dSize;
	McDebugLog_FuncOut("McSrv_ReadRegN", &sdRet);
#endif
}

/***************************************************************************
 *	McSrv_Sleep
 *
 *	Function:
 *			Sleep for a specified interval.
 *	Arguments:
 *			dSleepTime	sleep time [us]
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_Sleep(
	UINT32	dSleepTime
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_Sleep");
#endif

	machdep_Sleep(dSleepTime);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_Sleep", 0);
#endif
}

/****************************************************************************
 *	McSrv_Lock
 *
 *	Description:
 *			Lock a call of the driver.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_Lock(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_Lock");
#endif

	machdep_Lock();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_Lock", 0);
#endif
}

/***************************************************************************
 *	McSrv_Unlock
 *
 *	Function:
 *			Unlock a call of the driver.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_Unlock(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_Unlock");
#endif

	machdep_Unlock();

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_Unlock", 0);
#endif
}

/***************************************************************************
 *	McSrv_MemCopy
 *
 *	Function:
 *			Copy memory.
 *	Arguments:
 *			pbSrc	copy source
 *			pbDest	copy destination
 *			dSize	size
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_MemCopy(
	const UINT8	*pbSrc,
	UINT8		*pbDest,
	UINT32		dSize
)
{
	UINT32	i;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_MemCopy");
#endif

	for (i = 0; i < dSize; i++) {
		;
		pbDest[i] = pbSrc[i];
	}

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_MemCopy", 0);
#endif
}

/***************************************************************************
 *	McSrv_DisableIrq
 *
 *	Function:
 *			Disable interrupt.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_DisableIrq(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_DisableIrq");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_DisableIrq", 0);
#endif
}

/***************************************************************************
 *	McSrv_EnableIrq
 *
 *	Function:
 *			Enable interrupt.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McSrv_EnableIrq(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("McSrv_EnableIrq");
#endif

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("McSrv_EnableIrq", 0);
#endif
}

