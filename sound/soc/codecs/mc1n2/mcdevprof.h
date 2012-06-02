/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdevprof.h
 *
 *		Description	: MC Driver device profile header
 *
 *		Version		: 1.0.0 	2010.07.05
 *
 ****************************************************************************/

#ifndef _MCDEVPROF_H_
#define _MCDEVPROF_H_

#include "mctypedef.h"

typedef enum
{
	eMCDRV_DEV_ID_1N2	= 0,
	eMCDRV_DEV_ID_2N,
	eMCDRV_DEV_ID_3N
} MCDRV_DEV_ID;

typedef enum
{
	eMCDRV_FUNC_LI2	= 0,
	eMCDRV_FUNC_RANGE,
	eMCDRV_FUNC_BYPASS,
	eMCDRV_FUNC_ADC1,
	eMCDRV_FUNC_PAD2,
	eMCDRV_FUNC_DBEX,
	eMCDRV_FUNC_GPMODE,
	eMCDRV_FUNC_DTMF,
	eMCDRV_FUNC_IRQ
} MCDRV_FUNC_KIND;

typedef enum
{
	eMCDRV_SLAVE_ADDR_DIG	= 0,
	eMCDRV_SLAVE_ADDR_ANA
} MCDRV_SLAVE_ADDR_KIND;



void	McDevProf_SetDevId(MCDRV_DEV_ID eDevId);

UINT8	McDevProf_IsValid(MCDRV_FUNC_KIND eFuncKind);
UINT8	McDevProf_GetSlaveAddr(MCDRV_SLAVE_ADDR_KIND eSlaveAddrKind);



#endif	/*	_MCDEVPROF_H_	*/
