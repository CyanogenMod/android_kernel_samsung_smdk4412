/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-memory.h
 * @brief  header of s3c-otg-hcdi-memory \n
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

#ifndef _S3C_OTG_HCDI_MEMORY_H_
#define _S3C_OTG_HCDI_MEMORY_H_


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

/**
 * @enum otg_mem_alloc_flag
 *
 * @brief enumeration for flag of memory allocation
 */
enum otg_mem_type {
	USB_MEM_SYNC,
	USB_MEM_ASYNC,
	USB_MEM_DMA
};

/**
 * int	otg_mem_alloc(void ** addr_pp, u16 byte_size, u8 ubType);
 *
 * @brief allocating momory specified
 *
 * @param  [inout] addr_pp : address to be assigned
 *         [in] byte_size : size of memory
 *         [in] type : enum otg_mem_type
 *
 * @return USB_ERR_SUCCESS : If success \n
 *		   USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_mem_alloc(void **addr_pp, u16 byte_size,
		enum otg_mem_type type)
{
	gfp_t flags;
	otg_dbg(OTG_DBG_OTGHCDI_MEM, "otg_mem_alloc\n");

	switch (type) {
	case USB_MEM_SYNC:
		flags = GFP_KERNEL;
		break;
	case USB_MEM_ASYNC:
		flags = GFP_ATOMIC;
		break;
	case USB_MEM_DMA:
		flags = GFP_DMA;
		break;
	default:
		otg_err(OTG_DBG_OTGHCDI_MEM,
			"not proper enum otg_mem_type\n");
		return USB_ERR_FAIL;
	}

	*addr_pp = kmalloc((size_t)byte_size, flags);

	if (*addr_pp == 0) {
		otg_err(OTG_DBG_OTGHCDI_MEM, "kmalloc failed\n");
		return USB_ERR_FAIL;
	}
	return USB_ERR_SUCCESS;
}

/**
 * int	otg_mem_copy(void *to_addr_p, void *from_addr_p, u16 byte_size);
 *
 * @brief memory copy
 *
 * @param  [in] to_addr_p : target address
 *         [in] from_addr_p : source address
 *         [in] byte_size : size
 *
 * @return USB_ERR_SUCCESS : If success \n
 *	       USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_mem_copy(void *to_addr_p,
		void *from_addr_p, u16 byte_size)
{
	otg_dbg(OTG_DBG_OTGHCDI_MEM, "otg_mem_copy\n");

	memcpy(to_addr_p, from_addr_p, (size_t)byte_size);

	return USB_ERR_SUCCESS;
}

/**
 * int	otg_mem_free(void * addr_p);
 *
 * @brief de-allocating memory
 *
 * @param  [in] addr_p : target address to be de-allocated
 *
 * @return USB_ERR_SUCCESS : If success \n
 *	       USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_mem_free(void *addr_p)
{
	otg_dbg(OTG_DBG_OTGHCDI_MEM, "otg_mem_free\n");
	kfree(addr_p);
	return USB_ERR_SUCCESS;
}


/**
 * int	otg_mem_set(void *addr_p, char value, u16 byte_size)
 *
 * @brief writing a value to memory
 *
 * @param  [in] addr_p : target address
 *         [in] value : value to be written
 *         [in] byte_size : size
 *
 * @return USB_ERR_SUCCESS : If success \n
 *		   USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_mem_set(void *addr_p, char value, u16 byte_size)
{
	otg_dbg(OTG_DBG_OTGHCDI_MEM, "otg_mem_set\n");
	memset(addr_p, value, (size_t)byte_size);
	return USB_ERR_SUCCESS;
}


#endif /* _S3C_OTG_HCDI_MEMORY_H_ */
