/*
## cyasdiagnostics.h - Linux Antioch device driver file
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor
## Boston, MA  02110-1301, USA.
## ===========================
*/

#define __CYAS_SYSFS_FOR_DIAGNOSTICS__
typedef enum cy_as_diag_cmd_type {
	CY_AS_DIAG_GET_VERSION 	= 1,
	CY_AS_DIAG_DISABLE_MSM_SDIO = 2,
	CY_AS_DIAG_ENABLE_MSM_SDIO	= 3,
	CY_AS_DIAG_LEAVE_STANDBY	= 4,
	CY_AS_DIAG_ENTER_STANDBY	= 5,
	CY_AS_DIAG_CREATE_BLKDEV	= 6,
	CY_AS_DIAG_DESTROY_BLKDEV	= 7,
	CY_AS_DIAG_SD_MOUNT	= 11,
	CY_AS_DIAG_SD_READ	= 12,
	CY_AS_DIAG_SD_WRITE	= 13,
	CY_AS_DIAG_SD_UNMOUNT = 14,
	CY_AS_DIAG_CONNECT_UMS	= 21,
	CY_AS_DIAG_DISCONNECT_UMS = 22,
	CY_AS_DIAG_CONNECT_MTP	= 31,
	CY_AS_DIAG_DISCONNECT_MTP	= 32,
	CY_AS_DIAG_TEST_RESET_LOW 		= 40,
	CY_AS_DIAG_TEST_RESET_HIGH	= 41
} cy_as_diag_cmd_type;

/*[]*/
