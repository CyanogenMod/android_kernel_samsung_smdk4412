/****************************************************************************
 *
 *	Copyright(c) 2012-2013 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcmachdep.c
 *
 *	Description	: machine dependent part for MC Driver
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

#include <linux/delay.h>
#include <linux/mutex.h>
#include "ymu831_priv.h"

#include "mcmachdep.h"
#if (MCDRV_DEBUG_LEVEL >= 4)
#include "mcdebuglog.h"
#endif

static struct mutex McDrv_Mutex;

/****************************************************************************
 *	machdep_SystemInit
 *
 *	Description:
 *			Initialize the system.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_SystemInit(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_SystemInit");
#endif

	/* Please implement system initialization procedure if need */
	mutex_init(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_SystemInit", 0);
#endif
}

/****************************************************************************
 *	machdep_SystemTerm
 *
 *	Description:
 *			Terminate the system.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_SystemTerm(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_SystemTerm");
#endif

	/* Please implement system termination procedure if need */
	mutex_destroy(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_SystemTerm", 0);
#endif
}

/****************************************************************************
 *	machdep_ClockStart
 *
 *	Description:
 *			Start clock.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_ClockStart(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_ClockStart");
#endif

	/* Please implement clock start procedure if need */

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_ClockStart", 0);
#endif
}

/****************************************************************************
 *	machdep_ClockStop
 *
 *	Description:
 *			Stop clock.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_ClockStop(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_ClockStop");
#endif

	/* Please implement clock stop procedure if need */

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_ClockStop", 0);
#endif
}

/***************************************************************************
 *	machdep_WriteReg
 *
 *	Function:
 *			Write data to the register.
 *	Arguments:
 *			bSlaveAdr	slave address
 *			pbData		byte data for write
 *			dSize		byte data length
 *	Return:
 *			None
 *
 ****************************************************************************/
void	machdep_WriteReg(
	UINT8	bSlaveAdr,
	const UINT8	*pbData,
	UINT32	dSize
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_WriteReg");
#endif

	/* Please implement register write procedure */
	mc_asoc_write_data(bSlaveAdr, pbData, dSize);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_WriteReg", 0);
#endif
}

/***************************************************************************
 *	machdep_ReadReg
 *
 *	Function:
 *			Read a byte data from the register.
 *	Arguments:
 *			bSlaveAdr	slave address
 *			dAddress	address of register
 *			pbData		pointer to read data buffer
 *			dSize		read count
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_ReadReg(
	UINT8	bSlaveAdr,
	UINT32	dAddress,
	UINT8	*pbData,
	UINT32	dSize
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	SINT32	sdRet;
	McDebugLog_FuncIn("machdep_ReadReg");
#endif

	/* Please implement register read procedure */
	mc_asoc_read_data(bSlaveAdr, dAddress, pbData, dSize);

#if (MCDRV_DEBUG_LEVEL >= 4)
	sdRet	= (SINT32)dSize;
	McDebugLog_FuncOut("machdep_ReadReg", &sdRet);
#endif
}

/****************************************************************************
 *	machdep_Sleep
 *
 *	Function:
 *			Sleep for a specified interval.
 *	Arguments:
 *			dSleepTime	sleep time [us]
 *	Return:
 *			None
 *
 ****************************************************************************/
void	machdep_Sleep(
	UINT32	dSleepTime
)
{
	unsigned long ms = dSleepTime / 1000;
	unsigned long us = dSleepTime % 1000;

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_Sleep");
#endif

	/* Please implement sleep procedure */
	if (us)
		udelay(us);
	if (ms)
		msleep(ms);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_Sleep", 0);
#endif
}

/***************************************************************************
 *	machdep_Lock
 *
 *	Function:
 *			Lock a call of the driver.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_Lock(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_Lock");
#endif

	/* Please implement lock procedure */
	mutex_lock(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_Lock", 0);
#endif
}

/***************************************************************************
 *	machdep_Unlock
 *
 *	Function:
 *			Unlock a call of the driver.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_Unlock(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_Unlock");
#endif

	/* Please implement unlock procedure */
	mutex_unlock(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_Unlock", 0);
#endif
}

/***************************************************************************
 *	machdep_PreLDODStart
 *
 *	Function:
 *			.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_PreLDODStart(
	void
)
{
	UINT8	bData[2];

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_PreLDODStart");
#endif

	/* Please implement procedure */
	mc_asoc_set_codec_ldod(1);
	bData[0]	= 0x04;
	bData[1]	= 0x01;
	mc_asoc_write_data(0, bData, 2);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_PreLDODStart", 0);
#endif
}

/***************************************************************************
 *	machdep_PostLDODStart
 *
 *	Function:
 *			.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_PostLDODStart(
	void
)
{
#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncIn("machdep_PostLDODStart");
#endif

	/* Please implement procedure */
	mc_asoc_set_codec_ldod(0);

#if (MCDRV_DEBUG_LEVEL >= 4)
	McDebugLog_FuncOut("machdep_PostLDODStart", 0);
#endif
}

/***************************************************************************
 *	machdep_DebugPrint
 *
 *	Function:
 *			Output debug log.
 *	Arguments:
 *			pbLogString	log string buffer pointer
 *	Return:
 *			none
 *
 ****************************************************************************/
void	machdep_DebugPrint(
	UINT8	*pbLogString
)
{
	/* Please implement debug output procedure */
	pr_debug("MCDRV: %s\n", pbLogString);
}

