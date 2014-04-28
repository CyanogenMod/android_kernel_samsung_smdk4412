/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdevprof.h
 *
 *	Description	: MC Driver device profile header
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

#ifndef _MCDEVPROF_H_
#define _MCDEVPROF_H_

#include "mctypedef.h"
#include "mcdriver.h"
#include "mcresctrl.h"

enum MCDRV_DEV_ID {
	eMCDRV_DEV_ID_80_90H	= 0,
	eMCDRV_DEV_ID_81_91H,
	eMCDRV_DEV_ID_81_92H
};

enum MCDRV_SLAVE_ADDR_KIND {
	eMCDRV_SLAVE_ADDR_DIG	= 0,
	eMCDRV_SLAVE_ADDR_ANA
};



void	McDevProf_SetDevId(enum MCDRV_DEV_ID eDevId);
enum MCDRV_DEV_ID	McDevProf_GetDevId(void);

UINT8	McDevProf_GetSlaveAddr(enum MCDRV_SLAVE_ADDR_KIND eSlaveAddrKind);

enum MCDRV_REG_ACCSESS	McDevProf_GetRegAccess(
				const struct MCDRV_REG_INFO *psRegInfo);



#endif	/*	_MCDEVPROF_H_	*/
