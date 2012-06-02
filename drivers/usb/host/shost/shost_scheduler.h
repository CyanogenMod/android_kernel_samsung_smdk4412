/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : Scheduler.h
 *  [Description] : The Header file defines the external
 *		and internal functions of Scheduler.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines functions of Scheduler
 *		-# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com)
 *			: Optimizing for performance
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

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

/* hcd.c */
extern void init_scheduler(void);

extern int reserve_used_resource_for_periodic(u32 usb_time,
		u8 dev_speed, u8 trans_type);
extern int free_usb_resource_for_periodic(u32 free_usb_time,
		u8 free_chnum, u8 trans_type);
extern int remove_ed_from_scheduler(struct ed *remove_ed);
extern int cancel_to_transfer_td(struct sec_otghost *otghost,
		struct td *cancel_td);
extern int retransmit(struct sec_otghost *otghost, struct td *retransmit_td);
extern int reschedule(struct td *resched_td);
extern int deallocate(struct td *dealloc_td);
extern void do_schedule(struct sec_otghost *otghost);
extern int get_td_info(u8 chnum, unsigned int *td_addr);

/* transfer-common.c */
int insert_ed_to_scheduler(struct sec_otghost *otghost, struct ed *insert_ed);

#endif

