/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : Commons3c-otg-transfer-transfer.h
 *  [Description] : This source file implements the functions
 *		to be defined at CommonTransfer Module.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements some functions of CommonTransfer.
 *		(2) 2008/07/15   by SeungSoo Yang ( ss1.yang@samsung.com )
 *	    - Optimizing for performance
 *      (3) 2008/08/18   by SeungSoo Yang ( ss1.yang@samsung.com )
 *          - Modifying for successful rmmod & disconnecting
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

#include "shost_transfer.h"

/* the header pointer to indicate the ED_list
 * to manage the struct ed to be created and initiated.
 */
static otg_list_head ed_list_head;
static u32 ref_periodic_transfer;

/**
 * void enable_sof(void)
 *
 * @brief Generate SOF Interrupt.
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */
static void enable_sof(void)
{
	gintmsk_t gintmsk = {.d32 = 0};
	gintmsk.b.sofintr = 1;
	update_reg_32(GINTMSK, gintmsk.d32);
}

/**
 *  void disable_sof(void)
 *
 * @brief Stop to generage SOF interrupt
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */
static void disable_sof(void)
{
	gintmsk_t gintmsk = {.d32 = 0};
	gintmsk.b.sofintr = 1;
	clear_reg_32(GINTMSK, gintmsk.d32);
}



/******************************************************************************/
/*!
 * @name int cancel_transfer(struct sec_otghost *otghost,
 * struct ed	*parent_ed,
 * struct td	*cancel_td)
 *
 * @brief this function cancels to transfer USB Transfer of cancel_td.
 *	this function firstly check whether this cancel_td
 *	is transferring or not. if the cancel_td is transferring, the this
 *	function requests to cancel the USB Transfer
 *	to S3CScheduler. if the parent_ed is for Periodic Transfer, and
 *	there is not any struct td at parent_ed, then this function requests
 *	to release some usb resources for the struct ed to S3CScheduler.
 *	finally this function deletes the cancel_td.
 *
 * @param [IN] pUpdateTD = indicates the pointer ot the struct td
 *	to have STransfer to be updated.
 *
 * @return	USB_ERR_SUCCESS	- if success to update the STranfer of pUpdateTD.
 *	USB_ERR_FAIL - if fail to update the STranfer of pUpdateTD.
 */
/******************************************************************************/
static int cancel_transfer(struct sec_otghost *otghost,
		struct ed *parent_ed, struct td *cancel_td)
{
	otg_list_head *tmp_list_p, *tmp_list2_p;
	int err = USB_ERR_DEQUEUED;
	bool cond_found = false;

	if (parent_ed == NULL || cancel_td == NULL) {
		otg_err(1, "%s is null.\n", parent_ed ?
				"cancel_td" : "parent_ed");
		return USB_ERR_NOELEMENT;
	}

	otg_list_for_each_safe(tmp_list_p, tmp_list2_p,
			&parent_ed->td_list_entry) {

		if (&cancel_td->td_list_entry == tmp_list_p) {
			cond_found = true;
			break;
		}
	}

	if (cond_found != true) {
		otg_dbg(OTG_DBG_TRANSFER, "cond_found != true\n");
		cancel_td->error_code = USB_ERR_NOELEMENT;
		otg_usbcore_giveback(cancel_td);
		return cancel_td->error_code;
	}


	if (cancel_td->is_transferring) {
		if (!parent_ed->ed_status.is_in_transfer_ready_q) {
			err = cancel_to_transfer_td(otghost, cancel_td);

			parent_ed->ed_status.in_transferring_td = 0;

			if (err != USB_ERR_SUCCESS) {
				otg_dbg(OTG_DBG_TRANSFER,
						"cancel_to_transfer_td\n");
				cancel_td->error_code = err;
				otg_usbcore_giveback(cancel_td);
				goto ErrorStatus;
			}

			otg_list_pop(&cancel_td->td_list_entry);
			parent_ed->num_td--;
		}

	} else {

		otg_list_pop(&cancel_td->td_list_entry);
		parent_ed->num_td--;

		if (parent_ed->num_td == 0) {
			remove_ed_from_scheduler(parent_ed);
			parent_ed->is_need_to_insert_scheduler = true;
		}
	}

	if (parent_ed->num_td) {
		parent_ed->is_need_to_insert_scheduler = true;
		insert_ed_to_scheduler(otghost, parent_ed);

	} else {

		if (parent_ed->ed_desc.endpoint_type == INT_TRANSFER ||
			parent_ed->ed_desc.endpoint_type == ISOCH_TRANSFER) {

			/* Release channel and usb bus resource for
			 * this struct ed. but, not release memory for this ed.
			 */
			free_usb_resource_for_periodic(
				parent_ed->ed_desc.used_bus_time,
				cancel_td->cur_stransfer.alloc_chnum,
				cancel_td->parent_ed_p->ed_desc.endpoint_type);

			parent_ed->ed_status.is_alloc_resource_for_ed = false;
		}
	}
	/* the caller of this functions should call
	   otg_usbcore_giveback(cancel_td); */
	cancel_td->error_code = USB_ERR_DEQUEUED;
	otg_usbcore_giveback(cancel_td);

	/* TODO: recursive call occured. FIX */
	delete_td(otghost, cancel_td);

ErrorStatus:

	return err;
}




/******************************************************************************/
/*!
 * @name int cancel_all_td(struct sec_otghost *otghost, struct ed *parent_ed)
 *
 * @brief this function cancels all Transfer which parent_ed manages.
 *
 * @param [IN] parent_ed = indicates the pointer ot the struct ed
 *	to manage TD_ts to be canceled.
 *
 * @return
 *	USB_ERR_SUCCESS	- if success to cancel all TD_ts of pParentsED.
 *	USB_ERR_FAIL - if fail to cancel all TD_ts of pParentsED.
 */
/******************************************************************************/
static int cancel_all_td(struct sec_otghost *otghost, struct ed *parent_ed)
{
	otg_list_head	*cancel_td_list_entry;
	struct td		*cancel_td;

	otg_dbg(OTG_DBG_OTGHCDI_HCD, "cancel_all_td\n");
	do {
		cancel_td_list_entry = parent_ed->td_list_entry.next;

		cancel_td = otg_list_get_node(cancel_td_list_entry,
				struct td, td_list_entry);

		cancel_transfer(otghost, parent_ed, cancel_td);

	} while (parent_ed->num_td);

	return USB_ERR_SUCCESS;
}




/******************************************************************************/
/*!
 * @name int delete_ed(struct ed *delete_ed)
 *
 * @brief this function delete the delete_ed.
 *	if there is some available  TD_ts on delete_ed,
 *	then this function also deletes these struct td
 *
 * @param	[IN]	delete_ed = indicates the address of struct ed to be deleted.
 *
 * @return	USB_ERR_SUCCESS	-if successes to delete the struct ed.
 *		USB_ERR_FAILl		-if fails to delete the ed.
 */
/******************************************************************************/
static int delete_ed(struct sec_otghost *otghost, struct ed *delete_ed)
{
	otg_kal_make_ep_null(delete_ed);

	if (delete_ed->num_td) {
		cancel_all_td(otghost, delete_ed);
	/**
	 * need to giveback of td's urb with considering life-cycle of
	 * TD, ED, urb->hcpriv, td->private, ep->hcpriv, td->parentED
	 * (commented by ss1.yang)
	 */
	}

	otg_list_pop(&delete_ed->ed_list_entry);

	if (delete_ed->ed_desc.endpoint_type == INT_TRANSFER ||
		delete_ed->ed_desc.endpoint_type == ISOCH_TRANSFER) {
		ref_periodic_transfer--;
	}

	if (ref_periodic_transfer == 0)
		disable_sof();

	otg_mem_free(delete_ed);

	return USB_ERR_SUCCESS;
}

/******************************************************************************/
/*!
 * @name void init_transfer(void)
 *
 * @brief this function initiates the S3CTranfer module.
 *	that is, this functions initiates
 *	the ED_list_head OTG List which manages the all ed to be existed.
 *
 * @param	void
 * @return	void
 */
/******************************************************************************/

static void init_transfer(void)
{
	otg_dbg(OTG_DBG_TRANSFER, "start to init_transfer\n");
	otg_list_init(&ed_list_head);
	ref_periodic_transfer = 0;
}


/******************************************************************************/
/*!
 * @name void DeInitTransfer(void)
 *
 * @brief this function Deinitiates the S3CTranfer module.
 *	this functions check which there are
 *	some ed on ED_list_head. if some ed exists,
 *	deinit_transfer() deletes the ed.
 *
 * @param	void
 * @return	void
 */
/******************************************************************************/
static void deinit_transfer(struct sec_otghost *otghost)
{
	otg_list_head	*ed_list_member;
	struct ed		*delete_ed_p;

	while (otg_list_empty(&ed_list_head) != true) {

		ed_list_member = ed_list_head.next;

		/* otg_list_pop(ed_list_member); */

		delete_ed_p = otg_list_get_node(ed_list_member,
				struct ed, ed_list_entry);

		delete_ed(otghost, delete_ed_p);
	}
}

/******************************************************************************/
/*!
 * @name int delete_td(struct sec_otghost *otghost, struct td *delete_td)
 *
 * @brief this function frees memory resource for the delete_td.
 *	and if delete_td is transferring USB Transfer, then
 *	this function request to cancel the USB Transfer to scheduler.
 *
 *
 * @param	[OUT]	new_td_p = returns the address of the new struct td.
 *
 * @return	USB_ERR_SUCCESS	-if successes to create the new struct td.
 *		USB_ERR_FAILl		-if fails to create to new struct td.
 */
/******************************************************************************/
int delete_td(struct sec_otghost *otghost, struct td *delete_td)
{
	if (delete_td->is_transferring) {
		/* at this case, we should cancel the USB Transfer. */
		cancel_to_transfer_td(otghost, delete_td);
	}

	otg_mem_free(delete_td);
	return USB_ERR_SUCCESS;
}


/******************************************************************************/
/*!
 * @name int create_ed(struct ed **new_ed)
 *
 * @brief		this function creates a new ed and returns the ed to Caller
 *
 * @param	[OUT]	new_ed = returns the address of the new ed .
 *
 * @return	USB_ERR_SUCCESS	-if successes to create the new ed.
 *		USB_ERR_FAILl		-if fails to create to new ed.
 */
/******************************************************************************/
static int create_ed(struct ed **new_ed)
{
	int err_code = USB_ERR_SUCCESS;

	err_code =  otg_mem_alloc((void **)new_ed,
			(u16)sizeof(struct ed), USB_MEM_ASYNC);
	otg_mem_set(*new_ed, 0, sizeof(struct ed));
	return err_code;
}


/******************************************************************************/
/*!
 * @name int init_ed(struct ed *init_ed,
 *	u8 dev_addr,
 *	u8 ep_num,
 *	bool f_is_ep_in,
 *	u8 dev_speed,
 *	u8 ep_type,
 *	u32	max_packet_size,
 *	u8 multi_count,
 *	u8 interval,
 *	u32	sched_frame,
 *	u8 hub_addr,
 *	u8 hub_port,
 *	bool f_is_do_split)
 *
 * @brief this function initiates the init_ed by using the another parameters.
 *
 * @param	[OUT]	init_ed = returns the struct ed to be initiated.
 * [IN]	dev_addr = inidcates the address of USB Device.
 * [IN]	ep_num = inidcates the number of the specific endpoint on USB Device.
 * [IN]	f_is_ep_in = inidcates whether the endpoint is IN or not
 * [IN]	dev_speed = inidcates the speed of USB Device.
 * [IN]	max_packet_size = inidcates the maximum packet size of a specific
 *	endpoint on USB Device.
 * [IN]	multi_count = if the endpoint supports periodic transfer
 *	, this indicates the multiple packet to be transferred on a uframe
 * [IN]	interval= if the endpoint support periodic transfer, this indicates
 *	the polling rate.
 * [IN]	sched_frame= if the endpoint supports periodic transfer, this indicates
 *	the start frame number.
 * [IN]	hub_addr= indicate the address of hub which the USB device attachs to.
 * [IN]	hub_port= inidcates the port number of the hub which the USB
 *	device attachs to.
 * [IN]	f_is_do_split= inidcates whether this tranfer is
 *	split transaction or not.
 *
 * @return	USB_ERR_SUCCESS	-if successes to initiate the ed.
 *		USB_ERR_FAILl		-if fails to initiate the ed.
 *		USB_ERR_NOSPACE	-if fails to initiate the ed
 *	because there is no USB Resource for this init_ed.
 */
/******************************************************************************/
static int init_ed(struct ed *init_ed,
	u8 dev_addr,
	u8 ep_num,
	bool f_is_ep_in,
	u8 dev_speed,
	u8 ep_type,
	u16 max_packet_size,
	u8 multi_count,
	u8 interval,
	u32 sched_frame,
	u8 hub_addr,
	u8 hub_port,
	bool f_is_do_split,
	void *ep)
{
	init_ed->is_halted = false;
	init_ed->is_need_to_insert_scheduler = true;
	init_ed->ed_id = (u32)init_ed;
	init_ed->num_td = 0;
	init_ed->ed_private = ep;

	otg_list_init(&init_ed->td_list_entry);

	/* start to initiate struct ed_desc.... */
	init_ed->ed_desc.is_do_split	= f_is_do_split;
	init_ed->ed_desc.is_ep_in		= f_is_ep_in;
	init_ed->ed_desc.dev_speed	= dev_speed;
	init_ed->ed_desc.hub_addr		= hub_addr;
	init_ed->ed_desc.hub_port		= hub_port;
	init_ed->ed_desc.mc		= multi_count;
	init_ed->ed_desc.device_addr	= dev_addr;
	init_ed->ed_desc.endpoint_num	= ep_num;
	init_ed->ed_desc.endpoint_type	= ep_type;
	init_ed->ed_desc.max_packet_size	= max_packet_size;
	init_ed->ed_desc.sched_frame	= sched_frame;

	if (init_ed->ed_desc.endpoint_type == INT_TRANSFER) {

		if (init_ed->ed_desc.dev_speed == LOW_SPEED_OTG ||
			init_ed->ed_desc.dev_speed == FULL_SPEED_OTG) {

			init_ed->ed_desc.interval = interval;

		} else if (init_ed->ed_desc.dev_speed == HIGH_SPEED_OTG) {

			u8	count = 0;
			u8	cal_interval = 1;

			for (count = 0;
				count < (init_ed->ed_desc.interval-1); count++)
				cal_interval *= 2;

			init_ed->ed_desc.interval = cal_interval;

		} else {
			otg_dbg(OTG_DBG_TRANSFER,
					"Super-Speed is not supported\n");
		}

		init_ed->ed_desc.sched_frame =
			(SCHEDULE_SLOT+oci_get_frame_num())&HFNUM_MAX_FRNUM;
		ref_periodic_transfer++;
	}

	if (init_ed->ed_desc.endpoint_type == ISOCH_TRANSFER) {
		u8	count = 0;
		u8	cal_interval = 1;

		for (count = 0; count < (init_ed->ed_desc.interval-1); count++)
			cal_interval *= 2;

		init_ed->ed_desc.interval = cal_interval;
		init_ed->ed_desc.sched_frame =
			(SCHEDULE_SLOT+oci_get_frame_num())&HFNUM_MAX_FRNUM;
		ref_periodic_transfer++;
	}

	/* start to initiate struct ed_status.... */

	/* initiates PID */
	switch (ep_type) {
	case BULK_TRANSFER:
	case INT_TRANSFER:
		init_ed->ed_status.data_toggle	= DATA0;
		break;

	case CONTROL_TRANSFER:
		init_ed->ed_status.control_data_toggle.setup	= SETUP;
		init_ed->ed_status.control_data_toggle.data		= DATA1;
		init_ed->ed_status.control_data_toggle.status	= DATA1;
		break;

	case ISOCH_TRANSFER:
		if (f_is_ep_in) {
			switch (multi_count) {
			case MULTI_COUNT_ZERO:
				init_ed->ed_status.data_toggle = DATA0;
				break;
			case MULTI_COUNT_ONE:
				init_ed->ed_status.data_toggle = DATA1;
				break;
			case MULTI_COUNT_TWO:
				init_ed->ed_status.data_toggle = DATA2;
				break;
			default:
				break;
			}

		} else {
			switch (multi_count) {
			case MULTI_COUNT_ZERO:
				init_ed->ed_status.data_toggle = DATA0;
				break;
			case MULTI_COUNT_ONE:
				init_ed->ed_status.data_toggle = MDATA;
				break;
			case MULTI_COUNT_TWO:
				init_ed->ed_status.data_toggle = MDATA;
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	if (init_ed->ed_desc.endpoint_type == INT_TRANSFER ||
		init_ed->ed_desc.endpoint_type == ISOCH_TRANSFER) {

		u32 usb_time = 0, byte_count = 0;

		/* calculates the bytes to be transferred
		   at one (uframe)frame.*/
		byte_count = (init_ed->ed_desc.mc+1) *
			init_ed->ed_desc.max_packet_size;

		usb_time =	(u32)otg_usbcore_get_calc_bustime(
				init_ed->ed_desc.dev_speed,
				init_ed->ed_desc.is_ep_in,
				(init_ed->ed_desc.endpoint_type ==
				 ISOCH_TRANSFER ? true : false), byte_count);

		usb_time /= 1000;	/* convert nanosec unit to usec unit */

		if (reserve_used_resource_for_periodic(usb_time,
					init_ed->ed_desc.dev_speed,
					init_ed->ed_desc.endpoint_type)
			!= USB_ERR_SUCCESS) {
			return USB_ERR_NOSPACE;
		}

		init_ed->ed_status.is_alloc_resource_for_ed	= true;
		init_ed->ed_desc.used_bus_time = usb_time;
		init_ed->ed_desc.mc	= multi_count+1;
	}

	init_ed->ed_status.is_in_transfer_ready_q	= false;
	init_ed->ed_status.is_in_transferring		= false;
	init_ed->ed_status.is_ping_enable			= false;
	init_ed->ed_status.in_transferring_td		= 0;

	/* push the ed to ED_list. */
	otg_list_push_prev(&init_ed->ed_list_entry, &ed_list_head);

	if (ref_periodic_transfer)
		enable_sof();

	return USB_ERR_SUCCESS;
}


/*****************************************************************************/
/*!
 * @name int create_td(struct td **new_td)
 *
 * @brief this function creates a new struct td and returns the struct td to Caller
 *
 *
 * @param	[OUT]	new_td = returns the address of the new struct td .
 *
 * @return	USB_ERR_SUCCESS	-if successes to create the new struct td.
 *		USB_ERR_FAILl		-if fails to create to new struct td.
 */
/*****************************************************************************/
static int create_td(struct td **new_td)
{
	int	err_code = USB_ERR_SUCCESS;

	err_code =  otg_mem_alloc((void **)new_td,
			(u16)sizeof(struct td), USB_MEM_ASYNC);
	otg_mem_set(*new_td, 0, sizeof(struct td));
	return err_code;
}


/*****************************************************************************/
/*!
 * @name int init_perio_stransfer( bool f_is_isoch_transfer,
 *		struct td *parent_td)
 *
 * @brief this function initiates the parent_td->cur_stransfer for Periodic
 * Transfer and inserts this init_td_p to init_td_p->parent_ed_p.
 *
 * @param [IN] f_is_isoch_transfer	= indicates whether this transfer
 *	is Isochronous or not.
 * [IN] parent_td	= indicates the address of struct td to be initiated.
 *
 * @return	USB_ERR_SUCCESS	-if success to update the STranfer of pUpdateTD.
 *		USB_ERR_FAIL -if fail to update the STranfer of pUpdateTD.
 */
/*****************************************************************************/
static int init_perio_stransfer(bool f_is_isoch_transfer,
				struct td *parent_td)
{
	parent_td->cur_stransfer.ed_desc_p =
		&parent_td->parent_ed_p->ed_desc;
	parent_td->cur_stransfer.ed_status_p =
		&parent_td->parent_ed_p->ed_status;
	parent_td->cur_stransfer.alloc_chnum = CH_NONE;
	parent_td->cur_stransfer.parent_td =
		(u32)parent_td;
	parent_td->cur_stransfer.stransfer_id =
		(u32)&parent_td->cur_stransfer;

	otg_mem_set(&parent_td->cur_stransfer.hc_reg, 0, sizeof(struct hc_reg));

	parent_td->cur_stransfer.hc_reg.hc_int_msk.b.chhltd = 1;

	if (f_is_isoch_transfer) {
		/*  initiates the STransfer usinb the IsochPacketDesc[0]. */
		parent_td->cur_stransfer.buf_size =
			parent_td->isoch_packet_desc_p[0].buf_size;

		parent_td->cur_stransfer.phy_addr =
			parent_td->phy_buf_addr +
			parent_td->isoch_packet_desc_p[0].
			isoch_packiet_start_addr;

		parent_td->cur_stransfer.vir_addr =
			parent_td->vir_buf_addr +
			parent_td->isoch_packet_desc_p[0].
			isoch_packiet_start_addr;

	} else {
		parent_td->cur_stransfer.buf_size =
			(parent_td->buf_size > MAX_CH_TRANSFER_SIZE) ?
			MAX_CH_TRANSFER_SIZE : parent_td->buf_size;

		parent_td->cur_stransfer.phy_addr = parent_td->phy_buf_addr;
		parent_td->cur_stransfer.vir_addr = parent_td->vir_buf_addr;
	}

	parent_td->cur_stransfer.packet_cnt =
		calc_packet_cnt(parent_td->cur_stransfer.buf_size,
			parent_td->parent_ed_p->ed_desc.max_packet_size);

	return USB_ERR_SUCCESS;
}


/*****************************************************************************/
/*!
 * @name int init_nonperio_stransfer(bool f_is_standard_dev_req,
 *	struct td *parent_td)
 *
 * @brief this function initiates the parent_td->cur_stransfer
 *	for NonPeriodic Transfer and inserts this init_td_p
 *	to init_td_p->parent_ed_p.
 *
 * @param [IN] f_is_standard_dev_req- indicates whether this is Control or not.
 * [IN] parent_td- indicates the address of struct td to be initiated.
 *
 * @return	USB_ERR_SUCCESS	- if success to update the STranfer of pUpdateTD.
 *	USB_ERR_FAIL - if fail to update the STranfer of pUpdateTD.
 */
/******************************************************************************/
static int init_nonperio_stransfer(bool f_is_standard_dev_req,
				struct td *parent_td)
{
	parent_td->cur_stransfer.ed_desc_p		=
		&parent_td->parent_ed_p->ed_desc;
	parent_td->cur_stransfer.ed_status_p	=
		&parent_td->parent_ed_p->ed_status;
	parent_td->cur_stransfer.alloc_chnum	= CH_NONE;
	parent_td->cur_stransfer.parent_td		= (u32)parent_td;
	parent_td->cur_stransfer.stransfer_id	=
		(u32)&parent_td->cur_stransfer;

	otg_mem_set(&(parent_td->cur_stransfer.hc_reg),
		0, sizeof(struct hc_reg));

	parent_td->cur_stransfer.hc_reg.hc_int_msk.b.chhltd = 1;

	if (f_is_standard_dev_req) {
		parent_td->cur_stransfer.buf_size =
			USB_20_STAND_DEV_REQUEST_SIZE;
		parent_td->cur_stransfer.phy_addr =
			parent_td->standard_dev_req_info.
			phy_standard_dev_req_addr;
		parent_td->cur_stransfer.vir_addr =
			parent_td->standard_dev_req_info.
			vir_standard_dev_req_addr;
	} else {
		parent_td->cur_stransfer.buf_size
			= (parent_td->buf_size > MAX_CH_TRANSFER_SIZE)
			? MAX_CH_TRANSFER_SIZE
			: parent_td->buf_size;

		parent_td->cur_stransfer.phy_addr
			= parent_td->phy_buf_addr;
		parent_td->cur_stransfer.vir_addr
			= parent_td->vir_buf_addr;
	}

	parent_td->cur_stransfer.packet_cnt =
		calc_packet_cnt(parent_td->cur_stransfer.buf_size,
			parent_td->parent_ed_p->ed_desc.max_packet_size);

	return USB_ERR_SUCCESS;
}

/******************************************************************************/
/*!
 * @name int init_td(struct td *init_td,
 *	struct ed *parent_ed,
 *	void *call_back_fun,
 *	void *call_back_param,
 *	u32 transfer_flag,
 *	bool f_is_standard_dev_req,
 *	u32 phy_setup,
 *	u32 vir_setup,
 *	u32 vir_buf_addr,
 *	u32 phy_buf_addr,
 *	u32 buf_size,
 *	u32 isoch_start_frame,
 *	isoch_packet_desc_t *isoch_packet_desc,
 *	u32 isoch_packet_num,
 *	void *td_priv)
 *
 * @brief this function initiates the init_td by using another parameter.
 *
 *
 * @param [IN] init_td- indicate the struct td to be initiated.
 * [IN]	parent_ed- indicate the ed to manage this init_td
 * [IN]	call_back_func- indicate the call-back function of application.
 * [IN]	call_back_param- indicate the parameter of the call-back function.
 * [IN]	transfer_flag- indicate the transfer flag.
 * [IN]	f_is_standard_dev_req- indicates the issue transfer request is Request
 * [IN]	phy_setup- the physical address of buffer to store the Request.
 * [IN]	vir_setup- the virtual address of buffer to store the Request.
 * [IN]	vir_buf_addr- the virtual address of buffer to store the data
 *	to be transferred or received.
 * [IN]	phy_buf_addr- the physical address of buffer to store the data
 *	to be transferred or received.
 * [IN]	buf_size- indicates the buffer size.
 * [IN]	isoch_start_frame- if this usb transfer is isochronous transfer
 *	, this indicates the start frame to start the usb transfer.
 * [IN]	isoch_packet_desc- if the usb transfer is isochronous transfer
 *	, this indicates the structure to describe the isochronous transfer.
 * [IN]	isoch_packet_num- if the usb transfer is isochronous transfer
 *	, this indicates the number of packet to consist of the usb transfer.
 * [IN]	td_priv- indicate the private data to be delivered from usb core.
 *	td_priv stores the urb of linux.
 *
 * @return	USB_ERR_SUCCESS -if successes to initiate the new struct td.
 *		USB_ERR_FAILl	  -if fails to create to new struct td.
 */
/******************************************************************************/
static int init_td(struct td	*init_td,
	struct ed					*parent_ed,
	void						*call_back_fun,
	void						*call_back_param,
	u32							transfer_flag,
	bool						f_is_standard_dev_req,
	u32							phy_setup,
	u32							vir_setup,
	u32							vir_buf_addr,
	u32							phy_buf_addr,
	u32							buf_size,
	u32						isoch_start_frame,
	struct isoch_packet_desc	*isoch_packet_desc,
	u32						isoch_packet_num,
	void						*td_priv)
{
	if (f_is_standard_dev_req) {

		if ((phy_buf_addr > 0) && (buf_size > 0))
			init_td->standard_dev_req_info.is_data_stage = true;
		else
			init_td->standard_dev_req_info.is_data_stage = false;

		init_td->standard_dev_req_info.conrol_transfer_stage
			= SETUP_STAGE;
		init_td->standard_dev_req_info.phy_standard_dev_req_addr
			= phy_setup;
		init_td->standard_dev_req_info.vir_standard_dev_req_addr
			= vir_setup;
	}

	init_td->call_back_func_p		= call_back_fun;
	init_td->call_back_func_param_p	= call_back_param;
	init_td->error_code				= USB_ERR_SUCCESS;
	init_td->is_standard_dev_req	= f_is_standard_dev_req;
	init_td->is_transfer_done		= false;
	init_td->is_transferring		= false;
	init_td->td_private				= td_priv;
	init_td->err_cnt				= 0;
	init_td->parent_ed_p			= parent_ed;
	init_td->phy_buf_addr			= phy_buf_addr;
	init_td->vir_buf_addr			= vir_buf_addr;
	init_td->buf_size				= buf_size;
	init_td->isoch_packet_desc_p	= isoch_packet_desc;
	init_td->isoch_packet_num		= isoch_packet_num;
	init_td->isoch_packet_index		= 0;
	init_td->isoch_packet_position	= 0;
	init_td->sched_frame			= isoch_start_frame;
	init_td->used_total_bus_time	= parent_ed->ed_desc.used_bus_time;
	init_td->td_id					= (u32)init_td;
	init_td->transfer_flag			= transfer_flag;
	init_td->transferred_szie		= 0;

	switch (parent_ed->ed_desc.endpoint_type) {
	case CONTROL_TRANSFER:
		init_nonperio_stransfer(true, init_td);
		break;

	case BULK_TRANSFER:
		init_nonperio_stransfer(false, init_td);
		break;

	case INT_TRANSFER:
		init_perio_stransfer(false, init_td);
		break;

	case ISOCH_TRANSFER:
		init_perio_stransfer(true, init_td);
		break;

	default:
		return USB_ERR_FAIL;
	}

	/* insert the struct td to parent_ed->td_list_entry. */
	otg_list_push_prev(&init_td->td_list_entry, &parent_ed->td_list_entry);
	parent_ed->num_td++;

	return USB_ERR_SUCCESS;
}

/******************************************************************************/
/*!
 * @name int issue_transfer(struct sec_otghost *otghost,
 *	struct ed *parent_ed,
 *	void *call_back_func,
 *	void *call_back_param,
 *	u32 transfer_flag,
 *	bool f_is_standard_dev_req,
 *	u32 setup_vir_addr,
 *	u32 setup_phy_addr,
 *	u32 vir_buf_addr,
 *	u32 phy_buf_addr,
 *	u32 buf_size,
 *	u32 start_frame,
 *	u32 isoch_packet_num,
 *	isoch_packet_desc_t *isoch_packet_desc,
 *	void *td_priv,
 *	unsigned int *return_td_addr)
 *
 * @brief		this function start USB Transfer
 *
 *
 * @param [IN] parent_ed - indicate the ed to manage this issue transfer.
 * [IN] call_back_func - indicate the call-back function of application.
 * [IN] call_back_param - indicate the parameter of the call-back function.
 * [IN] transfer_flag - indicate the transfer flag.
 * [IN] f_is_standard_dev_req - indicates the issue transfer request
 *		is USB Standard Request
 * [IN] setup_vir_addr	- the virtual address of buffer to store the Request.
 * [IN] setup_phy_addr - the physical address of buffer to store the Request.
 * [IN] vir_buf_addr - the virtual address of buffer to store the data
 *	to be transferred or received.
 * [IN] phy_buf_addr - the physical address of buffer to store the data
 *	to be transferred or received.
 * [IN] buf_siz- indicates the buffer size.
 * [IN] start_frame - if this usb transfer is isochronous transfer,
 *	this indicates the start frame to start the usb transfer.
 * [IN] isoch_packet_num - if the usb transfer is isochronous transfer,
 *	this indicates the number of packet to consist of the usb transfer.
 * [IN] isoch_packet_desc - if the usb transfer is isochronous transfer
 *	this indicates the structure to describe the isochronous transfer.
 * [IN] td_priv - indicate the private data to be delivered from usb core.
 *		td_priv stores the urb of linux.
 * [OUT] return_td_addr - indicates the variable address
 *		to store the new struct td for this transfer
 *
 * @return	USB_ERR_SUCCESS - if successes to initiate the new struct td.
 *		USB_ERR_FAILl - if fails to create to new struct td.
 */
/******************************************************************************/
static int issue_transfer(struct sec_otghost *otghost,
	struct ed					*parent_ed,
	void						*call_back_func,
	void						*call_back_param,
	u32							transfer_flag,
	bool						f_is_standard_dev_req,
	u32							setup_vir_addr,
	u32							setup_phy_addr,
	u32							vir_buf_addr,
	u32							phy_buf_addr,
	u32							buf_size,
	u32							start_frame,
	u32						isoch_packet_num,
	struct isoch_packet_desc	*isoch_packet_desc,
	void						*td_priv,
	unsigned int				*return_td_addr)
{
	struct td *new_td_p = NULL;
	int err = USB_ERR_SUCCESS;

	if (create_td(&new_td_p) == USB_ERR_SUCCESS) {
		err = init_td(new_td_p,
				parent_ed,
				call_back_func,
				call_back_param,
				transfer_flag,
				f_is_standard_dev_req,
				setup_phy_addr,
				setup_vir_addr,
				vir_buf_addr,
				phy_buf_addr,
				buf_size,
				start_frame,
				isoch_packet_desc,
				isoch_packet_num,
				td_priv);

		if (err != USB_ERR_SUCCESS)
			return USB_ERR_NOMEM;

		if (parent_ed->is_need_to_insert_scheduler)
			insert_ed_to_scheduler(otghost, parent_ed);

		*return_td_addr = (u32)new_td_p;

		return USB_ERR_SUCCESS;
	} else
		return USB_ERR_NOMEM;
}


/* TODO: not used. removed */
#if 0
static int create_isoch_packet_desc(isoch_packet_desc_t **new_isoch_packet_desc,
						u32 isoch_packet_num)
{
	return otg_mem_alloc((void **)new_isoch_packet_desc,
			(u16)sizeof(isoch_packet_desc_t)*isoch_packet_num,
			USB_MEM_SYNC);
}

static int delete_isoch_packet_desc(isoch_packet_desc_t *del_isoch_packet_desc,
						u32 isoch_packet_num)
{
	return otg_mem_free(del_isoch_packet_desc);
}

/******************************************************************************/
/*!
 * @name
 * void	init_isoch_packet_desc(isoch_packet_desc_t *init_isoch_packet_desc,
 *	u32 isoch_packet_start_addr,
 *	u32 isoch_packet_size,
 *	u32	index)
 *
 * @brief		this function initiates the isoch_packet_desc_t[index].
 *
 * @param
 * [OUT] init_isoch_packet_desc = indicates the pointer of
 *	IsochPackDesc_t to be initiated.
 * [IN]	isoch_packet_start_addr	= indicates the start address of the buffer
 *	to be used at USB Isochronous Transfer.
 * [IN] isoch_packet_size = indicates the size of Isochronous packet.
 * [IN] index = indicates the index to be mapped with this.
 *
 * @return	void
 */
/******************************************************************************/
static void init_isoch_packet_desc(isoch_packet_desc_t *init_isoch_packet_desc,
					u32 isoch_packet_start_addr,
					u32 isoch_packet_size,
					u32 index)
{
	init_isoch_packet_desc[index].buf_size = isoch_packet_size;
	init_isoch_packet_desc[index].isoch_packiet_start_addr
		= isoch_packet_start_addr;
	init_isoch_packet_desc[index].isoch_status		= 0;
	init_isoch_packet_desc[index].transferred_szie	= 0;
}
#endif


