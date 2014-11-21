/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-kal.h
 * @brief  header of s3c-otg-hcdi-kal \n
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 *  -# Aug 18,2008 v1.3 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Modifying for successful rmmod & disconnecting \n
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

#ifndef _SHOST_KAL_H_
#define _SHOST_KAL_H_

#include <linux/io.h>		/* for readl, writel */
#include <linux/usb/ch9.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <mach/map.h>
#include <plat/regs-otg.h>

extern volatile		u8			*g_pUDCBase;
extern struct		usb_hcd		*g_pUsbHcd;

#define	ctrlr_base_reg_addr(offset) \
			((volatile unsigned int *)((g_pUDCBase) + (offset)))
/**
 * otg_kal_make_ep_null
 *
 * @brief make ep->hcpriv NULL
 *
 * @param [in] pdelete_ed : pointer of ed
 *
 * @return void \n
 */
static	inline	void
otg_kal_make_ep_null(struct ed *pdelete_ed)
{
	((struct usb_host_endpoint *)(pdelete_ed->ed_private))->hcpriv = NULL;
}

/**
 * otg_kal_is_ep_null
 *
 * @brief check ep->hcpriv is NULL or not
 *
 * @param [in] pdelete_ed : pointer of ed
 *
 * @return bool \n
 */
static	inline	bool
otg_kal_is_ep_null(struct ed *pdelete_ed)
{
	if (((struct usb_host_endpoint *)
				(pdelete_ed->ed_private))->hcpriv == NULL)
		return true;
	else
		return false;
}


/**
 * int	otg_usbcore_get_calc_bustime()
 *
 * @brief get bus time of usbcore
 *
 * @param [in] speed : usb speed
 *		  [in] is_input : input or not
 *		  [in] is_isoch : isochronous or not
 *		  [in] byte_count : bytes
 *
 * @return bus time of usbcore \n
 */
static	inline	int
otg_usbcore_get_calc_bustime(u8 speed, bool is_input,
		bool is_isoch, unsigned int byte_count)
{
	unsigned int convert_speed = 0;

	otg_dbg(OTG_DBG_OTGHCDI_KAL, "otg_usbcore_get_calc_bustime\n");
/*	enum usb_device_speed {
		USB_SPEED_UNKNOWN = 0,
		USB_SPEED_LOW, USB_SPEED_FULL,
		USB_SPEED_HIGH,
		USB_SPEED_VARIABLE, };*/
	switch (speed) {
	case HIGH_SPEED_OTG:
		convert_speed = USB_SPEED_HIGH; break;
	case FULL_SPEED_OTG:
		convert_speed = USB_SPEED_FULL; break;
	case LOW_SPEED_OTG:
		convert_speed = USB_SPEED_LOW; break;
	default:
		convert_speed = USB_SPEED_UNKNOWN; break;
	}
	return usb_calc_bus_time(convert_speed, is_input,
			(unsigned int)is_isoch, byte_count);
}


/**
 * void	otg_usbcore_giveback(struct td td_p)
 *
 * @brief give-back a td as urb
 *
 * @param [in] td_p : pointer of struct td to give back
 *
 * @return void \n
 */
static	inline	void
otg_usbcore_giveback(struct td *td_p)
{
	struct urb *urb_p = NULL;

	otg_dbg(OTG_DBG_OTGHCDI_KAL, "otg_usbcore_giveback\n");

	if (td_p->td_private == NULL) {
		otg_err(OTG_DBG_OTGHCDI_KAL,
			"td_p->td_private == NULL\n");
		return;
	}

	urb_p = (struct urb *)td_p->td_private;

	urb_p->actual_length	= (int)(td_p->transferred_szie);
	urb_p->status			= (int)(td_p->error_code);
	urb_p->error_count		= (int)(td_p->err_cnt);
	urb_p->hcpriv			= NULL;

	usb_hcd_giveback_urb(g_pUsbHcd, urb_p, urb_p->status);
}

/**
 * void otg_usbcore_hc_died(void)
 *
 * @brief inform usbcore of hc die
 *
 * @return void \n
 */
static	inline	void
otg_usbcore_hc_died(void)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL, "otg_usbcore_hc_died\n");
	usb_hc_died(g_pUsbHcd);
}

/**
 * void	otg_usbcore_poll_rh_status(void)
 *
 * @brief invoke usbcore's usb_hcd_poll_rh_status
 *
 * @param void
 *
 * @return void \n
 */
static	inline	void
otg_usbcore_poll_rh_status(void)
{
	usb_hcd_poll_rh_status(g_pUsbHcd);
}

/**
 * void	otg_usbcore_resume_roothub(void)
 *
 * @brief invoke usbcore's usb_hcd_resume_root_hub
 *
 * @param void
 *
 * @return void \n
 */
static inline	void
otg_usbcore_resume_roothub(void)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL,
		 "otg_usbcore_resume_roothub\n");
	usb_hcd_resume_root_hub(g_pUsbHcd);
};

/**
 * int	otg_usbcore_inc_usb_bandwidth(u32 band_width)
 *
 * @brief	increase bandwidth of usb bus
 *
 * @param  [in] band_width : bandwidth to be increased
 *
 * @return USB_ERR_SUCCESS \n
 */
static	inline	int
otg_usbcore_inc_usb_bandwidth(u32 band_width)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL,
		"otg_usbcore_inc_usb_bandwidth\n");
	hcd_to_bus(g_pUsbHcd)->bandwidth_allocated += band_width;
	return USB_ERR_SUCCESS;
}

/**
 * int otg_usbcore_des_usb_bandwidth(u32 uiBandwidth)
 *
 * @brief	decrease bandwidth of usb bus
 *
 * @param  [in] band_width : bandwidth to be decreased
 *
 * @return USB_ERR_SUCCESS \n
 */
static	inline	int
otg_usbcore_des_usb_bandwidth(u32 band_width)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL,
		"otg_usbcore_des_usb_bandwidth\n");
	hcd_to_bus(g_pUsbHcd)->bandwidth_allocated -= band_width;
	return USB_ERR_SUCCESS;
}

/**
 * int otg_usbcore_inc_periodic_transfer_cnt(u8 transfer_type)
 *
 * @brief	increase count of periodic transfer
 *
 * @param  [in] transfer_type : type of transfer
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_usbcore_inc_periodic_transfer_cnt(u8 transfer_type)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL,
		"otg_usbcore_inc_periodic_transfer_cnt\n");

	switch (transfer_type) {
	case INT_TRANSFER:
		hcd_to_bus(g_pUsbHcd)->bandwidth_int_reqs++;
		break;
	case ISOCH_TRANSFER:
		hcd_to_bus(g_pUsbHcd)->bandwidth_isoc_reqs++;
		break;
	default:
		otg_err(OTG_DBG_OTGHCDI_KAL,
			"not proper TransferType\n");
		return USB_ERR_FAIL;
	}
	return USB_ERR_SUCCESS;
}

/**
 * int otg_usbcore_des_periodic_transfer_cnt(u8 transfer_type)
 *
 * @brief	decrease count of periodic transfer
 *
 * @param  [in] transfer_type : type of transfer
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 */
static	inline	int
otg_usbcore_des_periodic_transfer_cnt(u8 transfer_type)
{
	otg_dbg(OTG_DBG_OTGHCDI_KAL,
		"otg_usbcore_des_periodic_transfer_cnt\n");

	switch (transfer_type) {
	case INT_TRANSFER:
		hcd_to_bus(g_pUsbHcd)->bandwidth_int_reqs--;
		break;
	case ISOCH_TRANSFER:
		hcd_to_bus(g_pUsbHcd)->bandwidth_isoc_reqs--;
		break;
	default:
		otg_err(OTG_DBG_OTGHCDI_KAL,
			"not proper TransferType\n");
		return USB_ERR_FAIL;
	}
	return USB_ERR_SUCCESS;
}

/**
 * u32 read_reg_32(u32 offset)
 *
 * @brief Reads the content of a register.
 *
 * @param [in] offset : offset of address of register to read.
 *
 * @return contents of the register. \n
 * @remark call readl()
 */
static	inline	u32 read_reg_32(u32 offset)
{
	volatile unsigned int *reg_addr_p = ctrlr_base_reg_addr(offset);

	return *reg_addr_p;
	/* return readl(reg_addr_p); */
};

/**
 * void write_reg_32( u32 offset, const u32 value)
 *
 * @brief Writes a register with a 32 bit value.
 *
 * @param [in] offset : offset of address of register to write.
 * @param [in] value : value to write
 *
 * @remark call writel()
 */
static inline void write_reg_32(u32 offset, const u32 value)
{
	volatile unsigned int *reg_addr_p = ctrlr_base_reg_addr(offset);

	*reg_addr_p = value;
	/* writel( value, reg_addr_p ); */
};

/**
 * void	update_reg_32(u32 offset, u32 value)
 *
 * @brief logic or operation
 *
 * @param [in] offset : offset of address of register to write.
 * @param [in] value : value to or
 *
 */
static	inline	void update_reg_32(u32 offset, u32 value)
{
	write_reg_32(offset, (read_reg_32(offset) | value));
}

/**
 * void	clear_reg_32(u32	 offset, u32 value)
 *
 * @brief logic not operation
 *
 * @param [in] offset : offset of address of register to write.
 * @param [in] value : value to not
 *
 */
static	inline	void clear_reg_32(u32	 offset, u32 value)
{
	write_reg_32(offset, (read_reg_32(offset) & ~value));
}

#endif /* _S3C_OTG_HCDI_KAL_H_ */

