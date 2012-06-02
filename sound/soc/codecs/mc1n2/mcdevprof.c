/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdevprof.c
 *
 *		Description	: MC Driver device profile
 *
 *		Version		: 1.0.0 	2010.09.10
 *
 ****************************************************************************/


#include "mcdevprof.h"
#include "mcmachdep.h"
#if MCDRV_DEBUG_LEVEL
#include "mcdebuglog.h"
#endif


static	MCDRV_DEV_ID	geDevID	= eMCDRV_DEV_ID_1N2;

static	UINT8	gabValid[][3]	=
{
/*	MC-1N2	MC-2N	MC-3N	*/
	{0,		0,		1},		/*	DI2		*/
	{0,		0,		1},		/*	RANGE	*/
	{0,		0,		1},		/*	BYPASS	*/
	{0,		0,		1},		/*	ADC1	*/
	{0,		0,		0},		/*	PAD2	*/
	{0,		1,		1},		/*	DBEX	*/
	{0,		0,		1},		/*	GPMODE	*/
	{0,		0,		1},		/*	DTMF	*/
	{0,		0,		1},		/*	IRQ		*/
};

static UINT8	gabSlaveAddr[3][2]	=
{
/*	Digital Analog	*/
	{0x3A,	0x3A},	/*	MC1N2	*/
	{0x00,	0x00},	/*	MC2N	*/
	{0x00,	0x00}	/*	MC3N	*/
};

/****************************************************************************
 *	McDevProf_SetDevId
 *
 *	Description:
 *			Set device ID.
 *	Arguments:
 *			eDevId	device ID
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McDevProf_SetDevId(MCDRV_DEV_ID eDevId)
{
#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncIn("McDevProf_SetDevId");
#endif

	geDevID	= eDevId;

#if (MCDRV_DEBUG_LEVEL>=4)
	McDebugLog_FuncOut("McDevProf_SetDevId", 0);
#endif
}

/****************************************************************************
 *	McDevProf_IsValid
 *
 *	Description:
 *			Validity function.
 *	Arguments:
 *			function kind
 *	Return:
 *			0:Invalid/1:Valid
 *
 ****************************************************************************/
UINT8	McDevProf_IsValid
(
	MCDRV_FUNC_KIND eFuncKind
)
{
	UINT8	bData;

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McDevProf_IsValid");
#endif

	bData	= gabValid[eFuncKind][geDevID];

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bData;
	McDebugLog_FuncOut("McDevProf_IsValid", &sdRet);
#endif
	return bData;
}

/****************************************************************************
 *	McDevProf_GetSlaveAddr
 *
 *	Description:
 *			get slave address.
 *	Arguments:
 *			eSlaveAddrKind	slave address kind
 *	Return:
 *			slave address
 *
 ****************************************************************************/
UINT8	McDevProf_GetSlaveAddr
(
	MCDRV_SLAVE_ADDR_KIND eSlaveAddrKind
)
{
	UINT8	bData	= gabSlaveAddr[geDevID][eSlaveAddrKind];

#if (MCDRV_DEBUG_LEVEL>=4)
	SINT32	sdRet;
	McDebugLog_FuncIn("McDevProf_GetSlaveAddr");
#endif

#if (MCDRV_DEBUG_LEVEL>=4)
	sdRet	= (SINT32)bData;
	McDebugLog_FuncOut("McDevProf_GetSlaveAddr", &sdRet);
#endif

	return bData;
}

