/*
 * tcbd_error.h
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TCBD_ERROR_H__
#define __TCBD_ERROR_H__

/**
 * @defgroup ListOfErrorCode
 * If API fails then above error code will be returned.
 * Error codes can be combined with each other.
 */
/** @{*/
#define TCERR_SUCCESS				0x00000000
#define TCERR_INVALID_ARG			0x00000001
#define TCERR_BROKEN_MAIL_HEADER	0x00000002
#define TCERR_UNKNOWN_MAIL_STATUS	0x00000004
#define TCERR_WAIT_MAIL_TIMEOUT		0x00000008
#define TCERR_CRC_FAIL				0x00000010
#define TCERR_ACK_FAIL				0x00000020
#define TCERR_OS_DRIVER_FAIL		0x00000040
#define TCERR_CANNOT_ACCESS_MAIL	0x00000080
#define TCERR_UNKNOWN_BAND			0x00000100
#define TCERR_TUNE_FAILED			0x00000200
#define TCERR_MAX_NUM_SERVICE		0x00000400
#define TCERR_SERVICE_NOT_FOUND		0x00000800
#define TCERR_NO_FIC_DATA			0x00001000
#define TCERR_IO_NOT_INITIALIZED	0x00002000
#define TCERR_WARMBOOT_FAIL			0x00004000
#define TCERR_AREADY_REGISTERED		0x00008000
#define TCERR_FATAL_ERROR           0x10000000
/**@}*/


#endif /*__TCBD_ERROR_H__*/
