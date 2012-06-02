/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcpacking.h
 *
 *		Description	: MC Driver Packet packing header
 *
 *		Version		: 1.0.0 	2010.08.24
 *
 ****************************************************************************/

#ifndef _MCPACKING_H_
#define _MCPACKING_H_

#include "mctypedef.h"
#include "mcdriver.h"

/* volume update */
typedef enum
{
	eMCDRV_VOLUPDATE_MUTE,
	eMCDRV_VOLUPDATE_ALL
} MCDRV_VOLUPDATE_MODE;

/*	power setting	*/
#define	MCDRV_POWINFO_DIGITAL_DP0			((UINT32)0x0001)
#define	MCDRV_POWINFO_DIGITAL_DP1			((UINT32)0x0002)
#define	MCDRV_POWINFO_DIGITAL_DP2			((UINT32)0x0004)
#define	MCDRV_POWINFO_DIGITAL_DPB			((UINT32)0x0008)
#define	MCDRV_POWINFO_DIGITAL_DPDI0			((UINT32)0x0010)
#define	MCDRV_POWINFO_DIGITAL_DPDI1			((UINT32)0x0020)
#define	MCDRV_POWINFO_DIGITAL_DPDI2			((UINT32)0x0040)
#define	MCDRV_POWINFO_DIGITAL_DPPDM			((UINT32)0x0080)
#define	MCDRV_POWINFO_DIGITAL_DPBDSP		((UINT32)0x0100)
#define	MCDRV_POWINFO_DIGITAL_DPADIF		((UINT32)0x0200)
#define	MCDRV_POWINFO_DIGITAL_PLLRST0		((UINT32)0x0400)
typedef struct
{
	UINT32	dDigital;
	UINT8	abAnalog[5];
} MCDRV_POWER_INFO;

/* power update */
typedef struct
{
	UINT32	dDigital;
	UINT8	abAnalog[5];
} MCDRV_POWER_UPDATE;

#define	MCDRV_POWUPDATE_DIGITAL_ALL			(0xFFFFFFFFUL)
#define	MCDRV_POWUPDATE_ANALOG0_ALL			(0x0F)
#define	MCDRV_POWUPDATE_ANALOG1_ALL			(0xFF)
#define	MCDRV_POWUPDATE_ANALOG2_ALL			(0x3F)
#define	MCDRV_POWUPDATE_ANALOG3_ALL			(0x1F)
#define	MCDRV_POWUPDATE_ANALOG4_ALL			(0xF0)
#define	MCDRV_POWUPDATE_ANALOG0_IN			(0x0D)
#define	MCDRV_POWUPDATE_ANALOG1_IN			(0xC0)
#define	MCDRV_POWUPDATE_ANALOG2_IN			(0x00)
#define	MCDRV_POWUPDATE_ANALOG3_IN			(0x1F)
#define	MCDRV_POWUPDATE_ANALOG4_IN			(0xF0)
#define	MCDRV_POWUPDATE_ANALOG0_OUT			(0x02)
#define	MCDRV_POWUPDATE_ANALOG1_OUT			(0x3F)
#define	MCDRV_POWUPDATE_ANALOG2_OUT			(0x3F)
#define	MCDRV_POWUPDATE_ANALOG3_OUT			(0x00)
#define	MCDRV_POWUPDATE_ANALOG4_OUT			(0x00)


SINT32		McPacket_AddInit			(const MCDRV_INIT_INFO* psInitInfo);
SINT32		McPacket_AddVol				(UINT32 dUpdate, MCDRV_VOLUPDATE_MODE eMode, UINT32* pdSVolDoneParam);
SINT32		McPacket_AddPowerUp			(const MCDRV_POWER_INFO* psPowerInfo, const MCDRV_POWER_UPDATE* psPowerUpdate);
SINT32		McPacket_AddPowerDown		(const MCDRV_POWER_INFO* psPowerInfo, const MCDRV_POWER_UPDATE* psPowerUpdate);
SINT32		McPacket_AddPathSet			(void);
SINT32		McPacket_AddMixSet			(void);
SINT32		McPacket_AddStart			(void);
SINT32		McPacket_AddStop			(void);
SINT32		McPacket_AddDigitalIO		(UINT32 dUpdateInfo);
SINT32		McPacket_AddDAC				(UINT32 dUpdateInfo);
SINT32		McPacket_AddADC				(UINT32 dUpdateInfo);
SINT32		McPacket_AddSP				(void);
SINT32		McPacket_AddDNG				(UINT32 dUpdateInfo);
SINT32		McPacket_AddAE				(UINT32 dUpdateInfo);
SINT32		McPacket_AddPDM				(UINT32 dUpdateInfo);
SINT32		McPacket_AddGPMode			(void);
SINT32		McPacket_AddGPMask			(UINT32 dPadNo);
SINT32		McPacket_AddGPSet			(UINT8 bGpio, UINT32 dPadNo);
SINT32		McPacket_AddSysEq			(UINT32 dUpdateInfo);
SINT32		McPacket_AddClockSwitch		(void);


#endif /* _MCPACKING_H_ */
