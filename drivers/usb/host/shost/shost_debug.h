/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-debug.c
 * @brief  It provides debug functions for display message \n
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 * @see None
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

#ifndef _SHOST_DEBUG_H
#define _SHOST_DEBUG_H

#define OTG_DEBUG

#ifdef OTG_DEBUG

#define OTG_DBG_OTGHCDI_DRIVER	1
#define OTG_DBG_OTGHCDI_HCD		0
#define OTG_DBG_OTGHCDI_KAL		0
#define OTG_DBG_OTGHCDI_LIST	0
#define OTG_DBG_OTGHCDI_MEM		0
#define OTG_DBG_OTGHCDI_IRQ		0

#define OTG_DBG_TRANSFER		0
#define OTG_DBG_SCHEDULE		0
#define OTG_DBG_SCHEDULE_ED		0
#define OTG_DBG_OCI				0
#define OTG_DBG_DONETRASF		0
#define OTG_DBG_ISR				0
#define OTG_DBG_ROOTHUB			0


#include <linux/kernel.h>	/* for printk */

#define otg_err(is_active, msg, args...) \
	do { \
		if ((is_active) == true) {\
			pr_err("OTG_ERR %s(%d): " msg, \
					__func__ , __LINE__, ##args); \
		} \
	} while (0)

#define otg_dbg(is_active, msg, args...) \
	do { \
		if ((is_active) == true) { \
			pr_info("OTG %s(%d): " msg, \
					__func__, __LINE__, ##args); \
		} \
	} while (0)

#else /* OTG_DEBUG */

#define otg_err(is_active, msg...) do {} while (0)
#define otg_dbg(is_active, msg...) do {} while (0)

#endif


#endif /* SHOST_DEBUG_H */
