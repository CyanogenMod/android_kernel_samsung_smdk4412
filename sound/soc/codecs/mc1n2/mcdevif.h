/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdevif.h
 *
 *		Description	: MC Driver device interface header
 *
 *		Version		: 1.0.0 	2010.07.05
 *
 ****************************************************************************/

#ifndef _MCDEVIF_H_
#define _MCDEVIF_H_

#include "mctypedef.h"
#include "mcdriver.h"

/* packet */
typedef struct
{
	UINT32	dDesc;
	UINT8	bData;
} MCDRV_PACKET;

#define	MCDRV_MAX_PACKETS					(256UL)

/*	packet dDesc	*/
/*	packet type	*/
#define	MCDRV_PACKET_TYPE_WRITE				(0x10000000UL)
#define	MCDRV_PACKET_TYPE_FORCE_WRITE		(0x20000000UL)
#define	MCDRV_PACKET_TYPE_TIMWAIT			(0x30000000UL)
#define	MCDRV_PACKET_TYPE_EVTWAIT			(0x40000000UL)
#define	MCDRV_PACKET_TYPE_TERMINATE			(0xF0000000UL)

#define	MCDRV_PACKET_TYPE_MASK				(0xF0000000UL)

/*	reg type	*/
#define	MCDRV_PACKET_REGTYPE_A				(0x00000000UL)
#define	MCDRV_PACKET_REGTYPE_B_BASE			(0x00001000UL)
#define	MCDRV_PACKET_REGTYPE_B_MIXER		(0x00002000UL)
#define	MCDRV_PACKET_REGTYPE_B_AE			(0x00003000UL)
#define	MCDRV_PACKET_REGTYPE_B_CDSP			(0x00004000UL)
#define	MCDRV_PACKET_REGTYPE_B_CODEC		(0x00005000UL)
#define	MCDRV_PACKET_REGTYPE_B_ANA			(0x00006000UL)

#define	MCDRV_PACKET_REGTYPE_MASK			(0x0000F000UL)
#define	MCDRV_PACKET_ADR_MASK				(0x00000FFFUL)

/*	event	*/
#define	MCDRV_EVT_INSFLG					(0x00010000UL)
#define	MCDRV_EVT_ALLMUTE					(0x00020000UL)
#define	MCDRV_EVT_DACMUTE					(0x00030000UL)
#define	MCDRV_EVT_DITMUTE					(0x00040000UL)
#define	MCDRV_EVT_SVOL_DONE					(0x00050000UL)
#define	MCDRV_EVT_APM_DONE					(0x00060000UL)
#define	MCDRV_EVT_ANA_RDY					(0x00070000UL)
#define	MCDRV_EVT_SYSEQ_FLAG_RESET			(0x00080000UL)
#define	MCDRV_EVT_CLKBUSY_RESET				(0x00090000UL)
#define	MCDRV_EVT_CLKSRC_SET				(0x000A0000UL)
#define	MCDRV_EVT_CLKSRC_RESET				(0x000B0000UL)
#define MCDRV_EVT_ADCMUTE					(0x000C0000UL)


#define	MCDRV_PACKET_EVT_MASK				(0x0FFF0000UL)
#define	MCDRV_PACKET_EVTPRM_MASK			(0x0000FFFFUL)

/*	timer	*/
#define	MCDRV_PACKET_TIME_MASK				(0x0FFFFFFFUL)



SINT32		McDevIf_AllocPacketBuf		(void);
void		McDevIf_ReleasePacketBuf	(void);
void		McDevIf_ClearPacket			(void);
void		McDevIf_AddPacket			(UINT32 dDesc, UINT8 bData);
void		McDevIf_AddPacketRepeat		(UINT32 dDesc, const UINT8* pbData, UINT16 wDataCount);
SINT32		McDevIf_ExecutePacket		(void);


#endif /* _MCDEVIF_H_ */
