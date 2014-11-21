/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : CommonTransferChecker.c
 *  [Description] : The Source file implements the external
 *		and internal functions of CommonTransferChecker.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2009/01/12
 *  [Revision History]
 *      (1) 2008/06/12   by Yang Soon Yeal { syatom.yang@samsung.com }
 *	- Created this file and implements functions of CommonTransferChecker
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

#include "shost.h"

/**
 * void mask_channel_interrupt(u32 ch_num, u32 mask_info)
 *
 * @brief Mask specific channel interrupt
 *
 * @param [IN] chnum : channel number for masking
 *	   [IN] mask_info : mask information to write register
 *
 * @return None
 *
 * @remark
 *
 */
static void mask_channel_interrupt(u32 ch_num, u32 mask_info)
{
	clear_reg_32(HCINTMSK(ch_num), mask_info);
}

/**
 * void unmask_channel_interrupt(u32 ch_num, u32 mask_info)
 *
 * @brief Unmask specific channel interrupt
 *
 * @param [IN] chnum : channel number for unmasking
 *	   [IN] mask_info : mask information to write register
 *
 * @return None
 *
 * @remark
 *
 */
static void unmask_channel_interrupt(u32 ch_num, u32 mask_info)
{
	update_reg_32(HCINTMSK(ch_num), mask_info);
}

/**
 * int get_ch_info(struct hc_info * hc_reg, u8 ch_num)
 *
 * @brief Get current channel information about specific channel
 *
 * @param [OUT] hc_reg : structure to write channel inforamtion value
 *	   [IN] ch_num : channel number for unmasking
 *
 * @return None
 *
 * @remark
 *
 */
static int get_ch_info(struct hc_info *hc_reg, u8 ch_num)
{
	if (hc_reg != NULL) {
		hc_reg->hc_int_msk.d32	= read_reg_32(HCINTMSK(ch_num));
		hc_reg->hc_int.d32	= read_reg_32(HCINT(ch_num));
		hc_reg->dma_addr	= read_reg_32(HCDMA(ch_num));
		hc_reg->hc_char.d32	= read_reg_32(HCCHAR(ch_num));
		hc_reg->hc_size.d32	= read_reg_32(HCTSIZ(ch_num));

		return USB_ERR_SUCCESS;
	}
	return USB_ERR_FAIL;
}

/**
 * void get_intr_ch(u32* haint, u32* haintmsk)
 *
 * @brief Get Channel Interrupt Information in HAINT, HAINTMSK register
 *
 * @param [OUT] haint : HAINT register value
 *	   [OUT] haintmsk : HAINTMSK register value
 *
 * @return None
 *
 * @remark
 *
 */
static void get_intr_ch(u32 *haint, u32 *haintmsk)
{
	*haint = read_reg_32(HAINT);
	*haintmsk = read_reg_32(HAINTMSK);
}

/**
 * void clear_ch_intr(u8 ch_num, u32 clear_bit)
 *
 * @brief Get Channel Interrupt Information in HAINT, HAINTMSK register
 *
 * @param [IN] haint : HAINT register value
 *	   [IN] haintmsk : HAINTMSK register value
 *
 * @return None
 *
 * @remark
 *
 */
static void clear_ch_intr(u8 ch_num, u32 clear_bit)
{
	update_reg_32(HCINT(ch_num), clear_bit);
}


static int
release_trans_resource(struct sec_otghost *otghost,
		struct td *done_td)
{
	/* remove the pDeallocateTD from parent_ed_p. */
	otg_list_pop(&done_td->td_list_entry);
	done_td->parent_ed_p->num_td--;

	/* Call deallocate to release the channel
	 and bandwidth resource of S3CScheduler. */
	deallocate(done_td);
	delete_td(otghost, done_td);
	return USB_ERR_SUCCESS;
}

static u32 calc_transferred_size(bool f_is_complete,
		struct td *td, struct hc_info *hc_info)
{
	if (f_is_complete) {
		if (td->parent_ed_p->ed_desc.is_ep_in) {
			return td->cur_stransfer.buf_size -
				hc_info->hc_size.b.xfersize;

		} else
			return	td->cur_stransfer.buf_size;

	} else {
		return	(td->cur_stransfer.packet_cnt -
				hc_info->hc_size.b.pktcnt)*td->
				parent_ed_p->ed_desc.max_packet_size;
	}
}

static void update_frame_number(struct td *pResultTD)
{
	u32 cur_frame_num = 0;

	if (pResultTD->parent_ed_p->ed_desc.
			endpoint_type == CONTROL_TRANSFER ||
		pResultTD->parent_ed_p->ed_desc.
			endpoint_type == BULK_TRANSFER) {
		return;
	}

	pResultTD->parent_ed_p->ed_desc.sched_frame +=
		pResultTD->parent_ed_p->ed_desc.interval;
	pResultTD->parent_ed_p->ed_desc.sched_frame &=
		HFNUM_MAX_FRNUM;

	cur_frame_num = oci_get_frame_num();

	if (((cur_frame_num - pResultTD->parent_ed_p->ed_desc.sched_frame)
		&HFNUM_MAX_FRNUM) <= (HFNUM_MAX_FRNUM >> 1)) {
		pResultTD->parent_ed_p->ed_desc.sched_frame = cur_frame_num;
	}
}

static void update_data_toggle(struct td *td, u8 toggle)
{
	switch (td->parent_ed_p->ed_desc.endpoint_type) {
	case CONTROL_TRANSFER:
		if (td->standard_dev_req_info.conrol_transfer_stage
				== DATA_STAGE) {
			td->parent_ed_p->ed_status.
				control_data_toggle.data = toggle;
		}
		break;
	case BULK_TRANSFER:
	case INT_TRANSFER:
		td->parent_ed_p->ed_status.data_toggle = toggle;
		break;

	case ISOCH_TRANSFER:
		break;
	default:
		break;
	}
}

/******************************************************************************/
/*!
 * @name	void	update_perio_stransfer(struct td *parent_td)
 *
 * @brief		this function updates the parent_td->cur_stransfer
 *	to be used by oci. the STransfer of parent_td is for Periodic Transfer.
 *
 * @param	[IN/OUT]parent_td	= indicates the pointer of struct td
 *	to store the STranser to be updated.
 *
 * @return	USB_ERR_SUCCESS	-if success to update the parent_td->cur_stransfer.
 *	USB_ERR_FAIL -if fail to update the parent_td->cur_stransfer.
 */
/******************************************************************************/
static void update_perio_stransfer(struct td *parent_td)
{
	switch (parent_td->parent_ed_p->ed_desc.endpoint_type) {
	case INT_TRANSFER:
		parent_td->cur_stransfer.phy_addr =
			parent_td->phy_buf_addr +
			parent_td->transferred_szie;

		parent_td->cur_stransfer.vir_addr =
			parent_td->vir_buf_addr +
			parent_td->transferred_szie;

		parent_td->cur_stransfer.buf_size =
			(parent_td->buf_size > MAX_CH_TRANSFER_SIZE)
			? MAX_CH_TRANSFER_SIZE : parent_td->buf_size;
		break;

	case ISOCH_TRANSFER:
		parent_td->cur_stransfer.phy_addr =
			parent_td->phy_buf_addr +
			parent_td->isoch_packet_desc_p[parent_td->
				isoch_packet_index].
				isoch_packiet_start_addr +
			parent_td->isoch_packet_position;

		parent_td->cur_stransfer.vir_addr =
			parent_td->vir_buf_addr +
			parent_td->isoch_packet_desc_p[parent_td->
				isoch_packet_index].
				isoch_packiet_start_addr +
				parent_td->isoch_packet_position;

		parent_td->cur_stransfer.buf_size =
			(parent_td->isoch_packet_desc_p
			 [parent_td->isoch_packet_index].buf_size -
			parent_td->isoch_packet_position) >
			MAX_CH_TRANSFER_SIZE
			? MAX_CH_TRANSFER_SIZE
			: parent_td->isoch_packet_desc_p
				[parent_td->isoch_packet_index].buf_size -
				parent_td->isoch_packet_position;

		break;

	default:
		break;
	}
}

/****************************************************************************/
/*!
 * @name	void	update_nonperio_stransfer(struct td *parent_td)
 *
 * @brief	this function updates the parent_td->cur_stransfer
 *		to be used by S3COCI.
 *
 * @param	[IN/OUT]parent_td = indicates the pointer of struct td
 *	to store the STranser to be updated.
 *
 * @return	USB_ERR_SUCCESS	-if success to update the parent_td->cur_stransfer.
 *	USB_ERR_FAIL	-if fail to update the parent_td->cur_stransfer.
 */
/****************************************************************************/
static void	update_nonperio_stransfer(struct td *parent_td)
{
	switch (parent_td->parent_ed_p->ed_desc.endpoint_type) {

	case BULK_TRANSFER:
		parent_td->cur_stransfer.phy_addr =
			parent_td->phy_buf_addr +
			parent_td->transferred_szie;

		parent_td->cur_stransfer.vir_addr	=
			parent_td->vir_buf_addr +
			parent_td->transferred_szie;

		parent_td->cur_stransfer.buf_size =
			((parent_td->buf_size - parent_td->transferred_szie)
			 > MAX_CH_TRANSFER_SIZE)
			? MAX_CH_TRANSFER_SIZE
			: parent_td->buf_size -
			parent_td->transferred_szie;
		break;

	case CONTROL_TRANSFER:
		if (parent_td->standard_dev_req_info.
				conrol_transfer_stage == SETUP_STAGE) {
			/* but, this case will not be occured...... */
			parent_td->cur_stransfer.phy_addr	=
				parent_td->standard_dev_req_info.
				phy_standard_dev_req_addr;
			parent_td->cur_stransfer.vir_addr	=
				parent_td->standard_dev_req_info.
				vir_standard_dev_req_addr;
			parent_td->cur_stransfer.buf_size	= 8;

		} else if (parent_td->standard_dev_req_info.
				conrol_transfer_stage == DATA_STAGE) {

			parent_td->cur_stransfer.phy_addr	=
				parent_td->phy_buf_addr +
				parent_td->transferred_szie;

			parent_td->cur_stransfer.vir_addr	=
				parent_td->vir_buf_addr +
				parent_td->transferred_szie;

			parent_td->cur_stransfer.buf_size	=
				((parent_td->buf_size - parent_td->
				  transferred_szie) > MAX_CH_TRANSFER_SIZE)
				? MAX_CH_TRANSFER_SIZE
				: parent_td->buf_size -
				parent_td->transferred_szie;

		} else {
			parent_td->cur_stransfer.phy_addr	= 0;
			parent_td->cur_stransfer.vir_addr	= 0;
			parent_td->cur_stransfer.buf_size		= 0;
		}
		break;
	default:
		break;
	}

	parent_td->cur_stransfer.packet_cnt =
		calc_packet_cnt(parent_td->cur_stransfer.buf_size,
			parent_td->parent_ed_p->ed_desc.max_packet_size);

}

#include "shost_transferchecker_control.c"
#include "shost_transferchecker_interrupt.c"
#include "shost_transferchecker_bulk.c"

/******************************************************************************/
/*!
 * @name	void	do_transfer_checker(struct sec_otghost *otghost)
 *
 * @brief		this function processes the result of USB Transfer.
 *		So, do_transfer_checker fistly
 *		check which channel occurs OTG Interrupt and gets the status
 *		information of the channel.
 *		do_transfer_checker requests the information of td to scheduler.
 *		To process the interrupt of the channel, do_transfer_checker
 *		calls the sub-modules of
 *		S3CDoneTransferChecker, for example,
 *		ControlTransferChecker, BulkTransferChecker.
 *		according to the process result of the channel interrupt,
 *		do_transfer_checker decides
 *		the USB Transfer will be done or retransmitted.
 *
 *
 * @param	void
 *
 * @return	void
 */
/*****************************************************************************/
void do_transfer_checker(struct sec_otghost *otghost)
__releases(&otghost->lock)
__acquires(&otghost->lock)
{
	u32	hc_intr = 0;
	u32	hc_intr_msk = 0;
	u8	do_try_cnt = 0;

	struct hc_info	ch_info;
	struct td	*done_td = {0};

	u32	td_addr = 0;
	u8	proc_result = 0;

	otg_mem_set((void *)&ch_info, 0, sizeof(struct hc_info));

	/* Get value of HAINT... */
	get_intr_ch(&hc_intr, &hc_intr_msk);

start_do_transfer_checker:

	while (do_try_cnt < MAX_CH_NUMBER) {
		/* checks the channel number to be masked or not. */
		if (!(hc_intr & hc_intr_msk & (1 << do_try_cnt))) {
			do_try_cnt++;
			goto start_do_transfer_checker;
		}

		/* Gets the address of the struct td
		to have the channel to be interrupted. */
		if (!(get_td_info(do_try_cnt, &td_addr))) {

			done_td = (struct td *)td_addr;

			if (do_try_cnt != done_td->cur_stransfer.alloc_chnum) {
				do_try_cnt++;
				goto start_do_transfer_checker;
			}

		} else {
			do_try_cnt++;
			goto start_do_transfer_checker;
		}

		/* Gets the informationof channel to be interrupted. */
		get_ch_info(&ch_info, do_try_cnt);

		switch (done_td->parent_ed_p->ed_desc.endpoint_type) {
		case CONTROL_TRANSFER:
			proc_result =
				process_control_transfer(done_td, &ch_info);
			break;
		case BULK_TRANSFER:
			proc_result = process_bulk_transfer(done_td, &ch_info);
			break;
		case INT_TRANSFER:
			proc_result = process_intr_transfer(done_td, &ch_info);
			break;
		case ISOCH_TRANSFER:
			/* proc_result =
			   ProcessIsochTransfer(done_td, &ch_info); */
			break;
		default:
			break;
		}

		if ((proc_result == RE_TRANSMIT) ||
			(proc_result == RE_SCHEDULE)) {

			done_td->parent_ed_p->ed_status.
				is_in_transferring = false;
			done_td->is_transfer_done = false;
			done_td->is_transferring = false;

			if (done_td->parent_ed_p->ed_desc.endpoint_type
					== CONTROL_TRANSFER ||
				done_td->parent_ed_p->ed_desc.endpoint_type
					== BULK_TRANSFER) {

				update_nonperio_stransfer(done_td);

			} else
				update_perio_stransfer(done_td);

			if (proc_result == RE_TRANSMIT)
				retransmit(otghost, done_td);
			else
				reschedule(done_td);
		}

		else if (proc_result == DE_ALLOCATE) {

			done_td->parent_ed_p->ed_status.
				is_in_transferring = false;
			done_td->parent_ed_p->ed_status.
				in_transferring_td = 0;
			done_td->is_transfer_done = true;
			done_td->is_transferring = false;

			spin_unlock(&otghost->lock);
			otg_usbcore_giveback(done_td);
			spin_lock(&otghost->lock);
			release_trans_resource(otghost, done_td);

		} else {	/* NO_ACTION.... */
			done_td->parent_ed_p->ed_status.
				is_in_transferring = true;
			done_td->parent_ed_p->ed_status.
				in_transferring_td = (u32)done_td;
			done_td->is_transfer_done = false;
			done_td->is_transferring = true;
		}
		do_try_cnt++;
	}
	/* Complete to process the Channel Interrupt.
	 So. we now start to scheduler of S3CScheduler. */
	do_schedule(otghost);
}


