/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fci_oal.h

	Description : OS Adatation Layer header

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
	2009/09/13	jason		initial
*******************************************************************************/

#ifndef __FCI_OAL_H__
#define __FCI_OAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void print_log(HANDLE hDevice, char *fmt, ...);
extern void ms_wait(int ms);

#ifdef __cplusplus
}
#endif

#endif		/* __FCI_OAL_H__ */
