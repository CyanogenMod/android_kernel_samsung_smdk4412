/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcpacking.h
 *
 *	Description	: MC Driver Packet packing header
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

#ifndef _MCPACKING_H_
#define _MCPACKING_H_

#include "mctypedef.h"
#include "mcdriver.h"

/* volume update */
enum MCDRV_VOLUPDATE_MODE {
	eMCDRV_VOLUPDATE_MUTE,
	eMCDRV_VOLUPDATE_ALL
};

#define	MCDRV_VOLUPDATE_ALL	(0xFFFFFFFFUL)
#define	MCDRV_VOLUPDATE_ANA_OUT	(0x00000001UL)
#define	MCDRV_VOLUPDATE_ANA_IN	(0x00000100UL)
#define	MCDRV_VOLUPDATE_DIN0	(0x00010000UL)
#define	MCDRV_VOLUPDATE_DIN1	(0x00020000UL)
#define	MCDRV_VOLUPDATE_DIN2	(0x00040000UL)
#define	MCDRV_VOLUPDATE_DIN3	(0x00080000UL)
#define	MCDRV_VOLUPDATE_ADIF0	(0x00100000UL)
#define	MCDRV_VOLUPDATE_ADIF1	(0x00200000UL)
#define	MCDRV_VOLUPDATE_ADIF2	(0x00400000UL)
#define	MCDRV_VOLUPDATE_DIN	(0x00FF0000UL)
#define	MCDRV_VOLUPDATE_DOUT0	(0x01000000UL)
#define	MCDRV_VOLUPDATE_DOUT1	(0x02000000UL)
#define	MCDRV_VOLUPDATE_DOUT2	(0x04000000UL)
#define	MCDRV_VOLUPDATE_DOUT3	(0x08000000UL)
#define	MCDRV_VOLUPDATE_DAC0	(0x10000000UL)
#define	MCDRV_VOLUPDATE_DAC1	(0x20000000UL)
#define	MCDRV_VOLUPDATE_DPATHDA	(0x80000000UL)
#define	MCDRV_VOLUPDATE_DOUT	(0xFF000000UL)
#define	MCDRV_VOLUPDATE_DIG	(0xFFFF0000UL)

/*	power setting	*/
struct MCDRV_POWER_INFO {
	UINT8	bDigital;
	UINT8	abAnalog[5];
};

#define	MCDRV_POWINFO_D_PLL_PD		(0x80)
#define	MCDRV_POWINFO_D_PE_CLK_PD	(0x20)
#define	MCDRV_POWINFO_D_PB_CLK_PD	(0x10)
#define	MCDRV_POWINFO_D_PM_CLK_PD	(0x08)
#define	MCDRV_POWINFO_D_PF_CLK_PD	(0x04)
#define	MCDRV_POWINFO_D_PC_CLK_PD	(0x02)

/* power update */
struct MCDRV_POWER_UPDATE {
	UINT8	bDigital;
	UINT8	abAnalog[5];
};

#define	MCDRV_POWUPDATE_D_ALL		(0xFF)
#define	MCDRV_POWUPDATE_AP		(0x3F)
#define	MCDRV_POWUPDATE_AP_OUT0		(0x7F)
#define	MCDRV_POWUPDATE_AP_OUT1		(0xFF)
#define	MCDRV_POWUPDATE_AP_MC		(0xFF)
#define	MCDRV_POWUPDATE_AP_IN		(0x87)



SINT32	McPacket_AddInit(void);
void	McPacket_AddClockSw(void);
SINT32	McPacket_AddPowerUp(const struct MCDRV_POWER_INFO *psPowerInfo,
				const struct MCDRV_POWER_UPDATE *psPowerUpdate
				);
SINT32	McPacket_AddPowerDown(const struct MCDRV_POWER_INFO *psPowerInfo,
				const struct MCDRV_POWER_UPDATE *psPowerUpdate
				);
void	McPacket_AddPathSet(void);
void	McPacket_AddStop(void);
void	McPacket_AddDSPStartStop(UINT8 bDSPStarted);
void	McPacket_AddFDSPStop(UINT8 bDSPStarted);
void	McPacket_AddStart(void);
void	McPacket_AddVol(UINT32 dUpdate,
			enum MCDRV_VOLUPDATE_MODE eMode,
			UINT32 *pdSVolDoneParam);
void	McPacket_AddDac0Mute(void);
void	McPacket_AddDac1Mute(void);
void	McPacket_AddDOutMute(void);
void	McPacket_AddAdifMute(void);
void	McPacket_AddDPathDAMute(void);
void	McPacket_AddDigitalIO(UINT32 dUpdateInfo);
void	McPacket_AddDigitalIOPath(void);
void	McPacket_AddSwap(UINT32 dUpdateInfo);
void	McPacket_AddHSDet(void);
void	McPacket_AddMKDetEnable(UINT8 bCheckPlugDetDB);
void	McPacket_AddAEC(void);
void	McPacket_AddGPMode(void);
void	McPacket_AddGPMask(UINT32 dPadNo);
void	McPacket_AddGPSet(UINT32 dPadNo);


#endif /* _MCPACKING_H_ */
