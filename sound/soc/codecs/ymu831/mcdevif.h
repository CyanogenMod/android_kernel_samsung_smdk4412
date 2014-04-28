/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdevif.h
 *
 *	Description	: MC Driver device interface header
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

#ifndef _MCDEVIF_H_
#define _MCDEVIF_H_

#include "mctypedef.h"
#include "mcdriver.h"

/* packet */
struct MCDRV_PACKET {
	UINT32	dDesc;
	UINT8	bData;
};

#define	MCDRV_MAX_PACKETS			(255UL)

/*	packet dDesc	*/
/*	packet type	*/
#define	MCDRV_PACKET_TYPE_WRITE			(0x10000000UL)
#define	MCDRV_PACKET_TYPE_FORCE_WRITE		(0x20000000UL)
#define	MCDRV_PACKET_TYPE_TIMWAIT		(0x30000000UL)
#define	MCDRV_PACKET_TYPE_EVTWAIT		(0x40000000UL)
#define	MCDRV_PACKET_TYPE_TERMINATE		(0xF0000000UL)

#define	MCDRV_PACKET_TYPE_MASK			(0xF0000000UL)

/*	reg type	*/
#define	MCDRV_PACKET_REGTYPE_IF			(0x00000000UL)
#define	MCDRV_PACKET_REGTYPE_A			(0x00001000UL)
#define	MCDRV_PACKET_REGTYPE_MA			(0x00002000UL)
#define	MCDRV_PACKET_REGTYPE_MB			(0x00003000UL)
#define	MCDRV_PACKET_REGTYPE_B			(0x00004000UL)
#define	MCDRV_PACKET_REGTYPE_E			(0x00005000UL)
#define	MCDRV_PACKET_REGTYPE_C			(0x00006000UL)
#define	MCDRV_PACKET_REGTYPE_F			(0x00007000UL)
#define	MCDRV_PACKET_REGTYPE_ANA		(0x00008000UL)
#define	MCDRV_PACKET_REGTYPE_CD			(0x00009000UL)

#define	MCDRV_PACKET_REGTYPE_MASK		(0x0000F000UL)
#define	MCDRV_PACKET_ADR_MASK			(0x00000FFFUL)

/*	event	*/
#define	MCDRV_EVT_SVOL_DONE			(0x00010000UL)
#define	MCDRV_EVT_ALLMUTE			(0x00020000UL)
#define	MCDRV_EVT_DIRMUTE			(0x00030000UL)
#define	MCDRV_EVT_ADCMUTE			(0x00040000UL)
#define	MCDRV_EVT_DITMUTE			(0x00050000UL)
#define	MCDRV_EVT_DACMUTE			(0x00060000UL)
#define	MCDRV_EVT_PSW_RESET			(0x00070000UL)
#define	MCDRV_EVT_CLKBUSY_RESET			(0x00080000UL)
#define	MCDRV_EVT_OFFCAN_BSY_RESET		(0x00090000UL)
#define	MCDRV_EVT_ANA_RDY			(0x000A0000UL)
#define	MCDRV_EVT_AP_CP_A_SET			(0x000B0000UL)

#define	MCDRV_EVT_IF_REG_FLAG_SET		(0x01000000UL)
#define	MCDRV_EVT_IF_REG_FLAG_RESET		(0x02000000UL)
#define	MCDRV_EVT_B_REG_FLAG_SET		(0x03000000UL)
#define	MCDRV_EVT_B_REG_FLAG_RESET		(0x04000000UL)
#define	MCDRV_EVT_E_REG_FLAG_SET		(0x05000000UL)
#define	MCDRV_EVT_E_REG_FLAG_RESET		(0x06000000UL)
#define	MCDRV_EVT_C_REG_FLAG_SET		(0x07000000UL)
#define	MCDRV_EVT_C_REG_FLAG_RESET		(0x08000000UL)
#define	MCDRV_EVT_F_REG_FLAG_SET		(0x09000000UL)
#define	MCDRV_EVT_F_REG_FLAG_RESET		(0x0A000000UL)

#define	MCDRV_PACKET_EVT_MASK			(0x0FFF0000UL)
#define	MCDRV_PACKET_EVTPRM_MASK		(0x0000FFFFUL)

/*	timer	*/
#define	MCDRV_PACKET_TIME_MASK			(0x0FFFFFFFUL)



SINT32		McDevIf_AllocPacketBuf(void);
void		McDevIf_ReleasePacketBuf(void);
void		McDevIf_ClearPacket(void);
void		McDevIf_AddPacket(UINT32 dDesc, UINT8 bData);
void		McDevIf_AddPacketRepeat(UINT32 dDesc,
					const UINT8 *pbData,
					UINT32 dDataCount);
SINT32		McDevIf_ExecutePacket(void);
void		McDevIf_ReadDirect(UINT32 dDesc,
					 UINT8 *pbData,
					 UINT32 dSize);
void		McDevIf_WriteDirect(UINT8 *pbData, UINT32 dSize);


#endif /* _MCDEVIF_H_ */
