/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcservice.h
 *
 *		Description	: MC Driver service routine header
 *
 *		Version		: 1.0.0 	2010.03.18
 *
 ****************************************************************************/

#ifndef _MCSERVICE_H_
#define _MCSERVICE_H_

#include "mctypedef.h"


void	McSrv_SystemInit	( void );
void	McSrv_SystemTerm	( void );
void	McSrv_ClockStart	( void );
void	McSrv_ClockStop		( void );
void	McSrv_WriteI2C		( UINT8	bSlaveAddr, UINT8 *pbData, UINT32 dSize );
UINT8	McSrv_ReadI2C		( UINT8	bSlaveAddr, UINT32 dRegAddr );
void	McSrv_Sleep			( UINT32 dSleepTime );
void	McSrv_Lock			( void );
void	McSrv_Unlock		( void );
void	McSrv_MemCopy		( const UINT8 *pbSrc, UINT8 *pbDest, UINT32 dSize );
void	McSrv_DisableIrq	( void );
void	McSrv_EnableIrq		( void );



#endif /* _MCSERVICE_H_ */
