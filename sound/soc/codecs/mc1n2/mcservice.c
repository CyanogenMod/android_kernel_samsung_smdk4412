/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcservice.c
 *
 *		Description	: MC Driver service routine
 *
 *		Version		: 1.0.0 	2010.09.10
 *
 ****************************************************************************/


#include "mcservice.h"
#include "mcmachdep.h"
#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_SystemInit
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_SystemInit");
#endif

	machdep_SystemInit();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_SystemTerm
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_SystemTerm");
#endif

	machdep_SystemTerm();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_ClockStart
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_ClockStart");
#endif

	machdep_ClockStart();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_ClockStop
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_ClockStop");
#endif

	machdep_ClockStop();

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McSrv_ClockStop", 0);
#endif
}

/****************************************************************************
 *	McSrv_WriteI2C
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
void	McSrv_WriteI2C
(
	UINT8	bSlaveAddr,
	UINT8	*pbData,
	UINT32	dSize
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_WriteI2C");
#endif

	McSrv_DisableIrq();
	machdep_WriteI2C( bSlaveAddr, pbData, dSize );
	McSrv_EnableIrq();

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McSrv_WriteI2C", 0);
#endif
}

/****************************************************************************
 *	McSrv_ReadI2C
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
UINT8	McSrv_ReadI2C
(
	UINT8	bSlaveAddr,
	UINT32	dRegAddr
)
{
	UINT8	bReg;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McSrv_ReadI2C");
#endif

	McSrv_DisableIrq();
	bReg	= machdep_ReadI2C( bSlaveAddr, dRegAddr );
	McSrv_EnableIrq();

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bReg;
	McDebugLog_FuncOut("McSrv_ReadI2C", &sdRet);
#endif

	return bReg;
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
void	McSrv_Sleep
(
	UINT32	dSleepTime
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_Sleep");
#endif

	machdep_Sleep( dSleepTime );

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_Lock
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_Lock");
#endif

	machdep_Lock();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_Unlock
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_Unlock");
#endif

	machdep_Unlock();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_MemCopy
(
	const UINT8	*pbSrc,
	UINT8		*pbDest,
	UINT32		dSize
)
{
	UINT32	i;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_MemCopy");
#endif

	for ( i = 0; i < dSize; i++ )
	{
		pbDest[i] = pbSrc[i];
	}

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_DisableIrq
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_DisableIrq");
#endif

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	McSrv_EnableIrq
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McSrv_EnableIrq");
#endif

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McSrv_EnableIrq", 0);
#endif
}

