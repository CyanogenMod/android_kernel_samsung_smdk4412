/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-common-const.h
 *  [Description] : The Header file defines constants
 *		to be used at sub-modules of S3C6400HCD.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created s3c-otg-common-const.h file and defines some constants.
 *
 ****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef	_CONST_TYPE_DEF_H_
#define	_CONST_TYPE_DEF_H_

/**
 * @def OTG_PORT_NUMBER
 *
 * @brief write~ description
 *
 * describe in detail
 */
#define	OTG_PORT_NUMBER				0

/* Defines Stages of Control Transfer */
#define		SETUP_STAGE				1
#define		DATA_STAGE				2
#define		STATUS_STAGE			3
#define		COMPLETE_STAGE			4

/* Defines Direction of Endpoint */
#define		EP_IN					1
#define		EP_OUT					0

/* Define speed of USB Device */
#define		LOW_SPEED_OTG			2
#define		FULL_SPEED_OTG			1
#define		HIGH_SPEED_OTG			0
#define		SUPER_SPEED_OTG			3

/* Define multiple count of packet in periodic transfer. */
#define		MULTI_COUNT_ZERO		0
#define		MULTI_COUNT_ONE			1
#define		MULTI_COUNT_TWO			2

/* Define USB Transfer Types. */
#define		CONTROL_TRANSFER		0
#define		ISOCH_TRANSFER			1
#define		BULK_TRANSFER			2
#define		INT_TRANSFER			3

#define		BULK_TIMEOUT			300

/* Defines PID */
#define		DATA0					0
#define		DATA1					2
#define		DATA2					1
#define		MDATA					3
#define		SETUP					3

/* Defines USB Transfer Request Size on USB2.0 */
#define USB_20_STAND_DEV_REQUEST_SIZE	8
/* Define Max Channel Number */
#define	MAX_CH_NUMBER			16
/* Define Channel Number */
#define	CH_0	0
#define	CH_1	1
#define	CH_2	2
#define	CH_3	3
#define	CH_4	4
#define	CH_5	5
#define	CH_6	6
#define	CH_7	7
#define	CH_8	8
#define	CH_9	9
#define	CH_10	10
#define	CH_11	11
#define	CH_12	12
#define	CH_13	13
#define	CH_14	14
#define	CH_15	15
#define CH_NONE	20

/*  define the Constant for result of processing the USB Transfer. */
#define	RE_TRANSMIT		1
#define	RE_SCHEDULE		2
#define	DE_ALLOCATE		3
#define	NO_ACTION		4

/* define	the threshold value to retransmit USB Transfer */
#define	RETRANSMIT_THRESHOLD		2

/* define the maximum size of data to be tranferred through channel. */
#define	MAX_CH_TRANSFER_SIZE		65536 /* 65535 */

/* define Max Frame Number which Synopsys OTG suppports. */
#define	MAX_FRAME_NUMBER			0x3FFF
/*  Channel Interrupt Status */
#define	CH_STATUS_DataTglErr		(0x1<<10)
#define	CH_STATUS_FrmOvrun			(0x1<<9)
#define	CH_STATUS_BblErr			(0x1<<8)
#define	CH_STATUS_XactErr			(0x1<<7)
#define	CH_STATUS_NYET				(0x1<<6)
#define	CH_STATUS_ACK				(0x1<<5)
#define	CH_STATUS_NAK				(0x1<<4)
#define	CH_STATUS_STALL				(0x1<<3)
#define	CH_STATUS_AHBErr			(0x1<<2)
#define	CH_STATUS_ChHltd			(0x1<<1)
#define	CH_STATUS_XferCompl			(0x1<<0)
#define	CH_STATUS_ALL				0x7FF

/* Define USB Transfer Flag.. */
#define	USB_TRANS_FLAG_NOT_SHORT	URB_SHORT_NOT_OK
#define	USB_TRANS_FLAG_ISO_ASYNCH	URB_ISO_ASAP

#define HFNUM_MAX_FRNUM	0x3FFF
#define SCHEDULE_SLOT	10


#endif


