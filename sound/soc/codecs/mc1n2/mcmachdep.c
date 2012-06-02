/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcmachdep.c
 *
 *		Description	: machine dependent part for MC Driver
 *
 *		Version		: 1.0.0 	2010.09.10
 *
 ****************************************************************************/

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include "mcmachdep.h"
#include "mc1n2_priv.h"
#if (MCDRV_DEBUG_LEVEL>=4)
#include "mcdebuglog.h"
#endif

static struct mutex McDrv_Mutex;
static struct i2c_client *McDrv_I2C_Client;

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
void	machdep_SystemInit
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_SystemInit");
#endif

	/* Please implement system initialization procedure if need */
	mutex_init(&McDrv_Mutex);
	McDrv_I2C_Client = mc1n2_get_i2c_client();

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	machdep_SystemTerm
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_SystemTerm");
#endif

	/* Please implement system termination procedure if need */
	mutex_destroy(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	machdep_ClockStart
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_ClockStart");
#endif

	/* Please implement clock start procedure if need */

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	machdep_ClockStop
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_ClockStop");
#endif

	/* Please implement clock stop procedure if need */

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("machdep_ClockStop", 0);
#endif
}

/***************************************************************************
 *	machdep_WriteI2C
 *
 *	Function:
 *			Write a byte data to the register.
 *	Arguments:
 *			bSlaveAdr	slave address
 *			pbData		byte data for write
 *			dSize		byte data length
 *	Return:
 *			None
 *
 ****************************************************************************/
void	machdep_WriteI2C
(
	UINT8	bSlaveAdr,
	const UINT8*	pbData,
	UINT32	dSize
)
{
	int count;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_WriteI2C");
#endif

	/* Please implement register write procedure */
	count = i2c_master_send(McDrv_I2C_Client, pbData, dSize);
	if (count != dSize) {
		dev_err(&McDrv_I2C_Client->dev, "I2C write error\n");
	}

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("machdep_WriteI2C", 0);
#endif
}

/***************************************************************************
 *	machdep_ReadI2C
 *
 *	Function:
 *			Read a byte data from the register.
 *	Arguments:
 *			bSlaveAdr	slave address
 *			dAddress	address of register
 *	Return:
 *			read data
 *
 ****************************************************************************/
UINT8	machdep_ReadI2C
(
	UINT8	bSlaveAdr,
	UINT32	dAddress
)
{
	UINT8	bData;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("machdep_ReadI2C");
#endif

	/* Please implement register read procedure */
 	bData = mc1n2_i2c_read_byte(McDrv_I2C_Client, dAddress);

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bData;
	McDebugLog_FuncOut("machdep_ReadI2C", &sdRet);
#endif

	return bData;
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
void	machdep_Sleep
(
	UINT32	dSleepTime
)
{
	unsigned long ms = dSleepTime / 1000;
	unsigned long us = dSleepTime % 1000;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_Sleep");
#endif

	/* Please implement sleep procedure */
	if (us)
		udelay(us);
	if (ms)
		msleep(ms);

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	machdep_Lock
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_Lock");
#endif

	/* Please implement lock procedure */
	mutex_lock(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL>=4)
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
void	machdep_Unlock
(
	void
)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("machdep_Unlock");
#endif

	/* Please implement unlock procedure */
	mutex_unlock(&McDrv_Mutex);

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("machdep_Unlock", 0);
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
void	machdep_DebugPrint
(
	UINT8*	pbLogString
)
{
	/* Please implement debug output procedure */
	pr_debug("MCDRV: %s\n", pbLogString);
}

