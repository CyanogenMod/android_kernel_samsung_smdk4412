/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-common-errorcode.h
 *  [Description] : The Header file defines Error Codes
 *		to be used at sub-modules of S3C6400HCD.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file.
 *      (2) 2008/08/18   by SeungSoo Yang ( ss1.yang@samsung.com )
 *          - add HCD error code
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

#ifndef _ERRORCODE_H
#define _ERRORCODE_H


/* General USB Error Code.*/
#define	USB_ERR_SUCCESS		 0
#define USB_ERR_FAIL		(-1)

#define	USB_ERR_NO		 1

#define USB_ERR_NO_ENTITY	(-2)

/* S3CTransfer Error Code */
#define	USB_ERR_NODEV		(-ENODEV)
#define	USB_ERR_NOMEM		(-ENOMEM)
#define	USB_ERR_NOSPACE		(-ENOSPC)
#define   USB_ERR_NOIO          (-EIO)

/* OTG-HCD error code */
#define	USB_ERR_NOELEMENT		(-ENOENT)
#define	USB_ERR_ESHUTDOWN		(-ESHUTDOWN)	/* unplug */
#define	USB_ERR_DEQUEUED		(-ECONNRESET)     /* unlink */


/* S3CScheduler Error Code */
#define USB_ERR_ALREADY_EXIST		(-1)
#define USB_ERR_NO_RESOURCE		(-2)
#define USB_ERR_NO_CHANNEL		(-3)
#define USB_ERR_NO_BANDWIDTH		(-4)
#define USB_ERR_ALL_RESROUCE		(-5)

/************************************************
 *Defines the USB Error Status Code of USB Transfer.
 ************************************************/

#define USB_ERR_STATUS_COMPLETE		0
#define USB_ERR_STATUS_INPROGRESS	(-EINPROGRESS)
#define USB_ERR_STATUS_CRC			(-EILSEQ)
#define USB_ERR_STATUS_XACTERR		(-EPROTO)
#define USB_ERR_STATUS_STALL			(-EPIPE)
#define USB_ERR_STATUS_BBLERR			(-EOVERFLOW)
#define USB_ERR_STATUS_AHBERR		(-EIO)
#define USB_ERR_STATUS_FRMOVRUN_OUT		(-ENOSR)
#define USB_ERR_STATUS_FRMOVRUN_IN		(-ECOMM)
#define USB_ERR_STATUS_SHORTREAD		(-EREMOTEIO)


#endif

