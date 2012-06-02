/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcmachdep.h
 *
 *		Description	: MC Driver machine dependent part header
 *
 *		Version		: 1.0.0 	2010.09.10
 *
 ****************************************************************************/

#ifndef _MCMACHDEP_H_
#define _MCMACHDEP_H_

#include "mctypedef.h"


#define	MCDRV_DEBUG_LEVEL	(0)


void	machdep_SystemInit	( void );
void	machdep_SystemTerm	( void );
void	machdep_ClockStart	( void );
void	machdep_ClockStop	( void );
void	machdep_WriteI2C	( UINT8 bSlaveAdr, const UINT8 *pbData, UINT32 dSize );
UINT8	machdep_ReadI2C		( UINT8 bSlaveAdr, UINT32 dAddress );
void	machdep_Sleep		( UINT32 dSleepTime );
void	machdep_Lock		( void );
void	machdep_Unlock		( void );
void	machdep_DebugPrint	( UINT8 *pbLogString );


#endif /* _MCMACHDEP_H_ */
