/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : IntTransferChecker.c
 *  [Description] : The Source file implements the external
 *		and internal functions of IntTransferChecker
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/19
 *  [Revision History]
 *      (1) 2008/06/18   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of IntTransferChecker
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



/******************************************************************************/
/*!
 * @name	u8 process_xfercompl_on_intr(	struct td *result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the xfercompl event
 *			the procedure of this function is as following
 *	1. clears all bits of the channel' HCINT
 *		by using clear_ch_intr() of S3CIsr.
 *	2. masks ack/nak(?)/datatglerr(?) bit of HCINTMSK
 *	3. Resets the err_cnt of result_td.
 *	4. updates the result_td->parent_ed_p->ed_status.
 *		IntDataTgl.
 *	5. calculates the tranferred size by calling
 *	calc_transferred_size() on DATA_STAGE.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE	-if USB Transfer is completed.
 *		RE_TRANSMIT	-if need to retransmit the result_td.
 */
/******************************************************************************/
static u8 process_xfercompl_on_intr(struct td *result_td,
					struct hc_info *hcinfo)
{
	u8	ret_val = 0;

	result_td->err_cnt = 0;
	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_ALL);

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->transferred_szie +=
		calc_transferred_size(true, result_td, hcinfo);

	if (result_td->transferred_szie == result_td->buf_size)	{
		/* at IN Transfer, short transfer is accepted. */
		result_td->error_code = USB_ERR_STATUS_COMPLETE;
		ret_val = DE_ALLOCATE;

	} else {

		if (result_td->parent_ed_p->ed_desc.is_ep_in &&
			hcinfo->hc_size.b.xfersize) {

			if (result_td->transfer_flag & USB_TRANS_FLAG_NOT_SHORT)
				result_td->error_code =
					USB_ERR_STATUS_SHORTREAD;
			else
				result_td->error_code = USB_ERR_STATUS_COMPLETE;
			ret_val = DE_ALLOCATE;

		} else {
			update_data_toggle(result_td, hcinfo->hc_size.b.pid);
			ret_val = RE_TRANSMIT;
		}
	}

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	return ret_val;

}

/******************************************************************************/
/*!
 * @name	u8 process_ahb_on_intr(struct td *result_td,
 *					struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with theAHB Errorl event
 *			this function stop the channel to be executed
 *
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 */
/******************************************************************************/
/* TODO: not used. removed */
#if 0
static u8 process_ahb_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	result_td->err_cnt		=	0;
	result_td->error_code	=	USB_ERR_STATUS_AHBERR;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_AHBErr);

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);
	result_td->parent_ed_p->ed_status.is_ping_enable = false;

	return DE_ALLOCATE;

}
#endif

/******************************************************************************/
/*!
 * @name	u8 process_stall_on_intr(struct td *result_td,
 *					struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the Stall event
 * when Stall is occured at Int Transfer, we should reset the PID as DATA0
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 */
/******************************************************************************/
static u8 process_stall_on_intr(struct td *result_td,
			 struct hc_info *hcinfo)
{
	result_td->err_cnt		=	0;
	result_td->error_code	=	USB_ERR_STATUS_STALL;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_ALL);

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);

	update_data_toggle(result_td, DATA0);

	return DE_ALLOCATE;
}

/******************************************************************************/
/*!
 * @name	u8 process_nak_on_intr(struct td *result_td,
 *					struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the nak event
 *	nak is occured at OUT/IN Transaction of Interrupt Transfer.
 *	we can't use ping protocol on Interrupt Transfer.
 *	and Syonopsys OTG IP occures
 *	chhltd interrupt on nak of IN/OUT Transaction.
 *	So we should retransmit the transfer on IN Transfer.
 *	If nak is occured at IN Transaction,
 *	this function processes this interrupt as following.
 *	1. resets the result_td->err_cnt.
 *	2. masks ack/nak/DaaTglErr bit of HCINTMSK.
 *	3. clears the nak bit of HCINT
 *	4. calculates frame number to retransmit this Interrupt Transfer.
 *
 *	If nak is occured at OUT Transaction,
 *	this function processes this interrupt as following.
 *	1. all procedures of IN Transaction are executed.
 *	2. calculates the size of the transferred data.
 *	3. if the speed of USB Device is High-Speed, sets the ping protocol.
 *	4. update the Toggle
 *	at OUT Transaction, this function check whether	the speed of
 *	USB Device is High-Speed or not.
 *		if USB Device is High-Speed, then
 *		this function sets the ping protocol.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_SCHEDULE	-if the direction of the Transfer is OUT
 *		NO_ACTION	-if the direction of the Transfer is IN
 */
/******************************************************************************/
static u8 process_nak_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	result_td->err_cnt = 0;

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);


	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	update_frame_number(result_td);

	return RE_SCHEDULE;


}

/******************************************************************************/
/*!
 * @name	u8 process_ack_on_intr(struct td *result_td,
 *					struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the ack event
 *	ack of IN/OUT Transaction don't need any retransmit.
 *	this function just resets result_td->err_cnt
 *	and masks ack/nak/DataTgl of HCINTMSK.
 *	finally, this function clears ack bit of HCINT
 *	and ed_status.is_ping_enable.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	NO_ACTION
 */
/******************************************************************************/
static u8 process_ack_on_intr(struct td *result_td,
			       struct hc_info *hcinfo)
{
	result_td->err_cnt = 0;

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);

	return NO_ACTION;
}

/******************************************************************************/
/*!
 * @name	u8 process_xacterr_on_intr(struct td *result_td,
 *					struct hc_info	*hcinfo)
 *
 * @brief this function deals with the xacterr event
 *	xacterr is occured at OUT/IN Transaction and we should
 *	retransmit the USB Transfer
 *	if the Error Counter is less than the RETRANSMIT_THRESHOLD.
 *	the reasons of xacterr is Timeout/CRC error/false EOP.
 *	the procedure to process xacterr is as following.
 *		1. increses the result_td->err_cnt
 *		2. check whether the result_td->err_cnt is equal to 3.
 *		3. unmasks ack/nak/datatglerr bit of HCINTMSK.
 *		4. clears the xacterr bit of HCINT
 *		5. calculates the size of the transferred data.
 *		6. update the Data Toggle.
 *		7. update the frame number to start retransmitting
 *		the Interrupt Transfer.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_SCHEDULE		-if the error count is less than 3
 *		DE_ALLOCATE		-if the error count is equal to 3
 */
/******************************************************************************/
static u8 process_xacterr_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	u8	ret_val = 0;

	if (result_td->err_cnt < RETRANSMIT_THRESHOLD) {
		result_td->cur_stransfer.hc_reg.hc_int_msk.d32 |=
			(CH_STATUS_ACK + CH_STATUS_NAK + CH_STATUS_DataTglErr);
		ret_val = RE_SCHEDULE;
		result_td->err_cnt++ ;

	} else {
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_ACK);
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_NAK);
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_DataTglErr);
		ret_val = DE_ALLOCATE;
		result_td->err_cnt = 0 ;
	}

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	if (ret_val == RE_SCHEDULE)
		update_frame_number(result_td);

	return ret_val;
}

/******************************************************************************/
/*!
 * @name	void process_bblerr_on_intr(struct td *result_td,
 *					struct hc_info *hcinfo)
 *
 * @brief	this function deals with the Babble event
 *	babble error is occured when the USB device continues to send packets
 *	althrough EOP is occured. So Babble error is only occured
 *	at IN Transfer.
 *	when Babble Error is occured, we should stop the USB Transfer,
 *	and return the fact to Application.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 */
/******************************************************************************/
static u8 process_bblerr_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	if (!result_td->parent_ed_p->ed_desc.is_ep_in)
		return NO_ACTION;

	result_td->err_cnt = 0;
	result_td->error_code = USB_ERR_STATUS_BBLERR;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_ALL);

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);
	return DE_ALLOCATE;
}

/******************************************************************************/
/*!
 * @name	u8 process_datatgl_on_intr(	struct td *result_td,
 *						struct hc_info *hcinfo)
 *
 * @brief this function deals with the datatglerr event
 *	the datatglerr event is occured at IN Transfer,
 *	and the channel is not halted.
 *	this function just resets result_td->err_cnt
 *	and masks ack/nak/DataTgl of HCINTMSK.
 *	finally, this function clears datatglerr bit of HCINT.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_SCHEDULE
 */
/******************************************************************************/
static u8 process_datatgl_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	result_td->err_cnt = 0;

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	update_frame_number(result_td);

	return RE_SCHEDULE;
}

static u8 process_frmovrrun_on_intr(struct td *result_td,
				     struct hc_info	*hcinfo)
{

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	update_frame_number(result_td);

	return RE_TRANSMIT;
}


/******************************************************************************/
/*!
 * @name	u8 process_chhltd_on_intr(struct td *result_td,
 *					struct hc_info	*HCRegData)
 *
 *
 * @brief		this function processes Channel Halt event
 *	firstly, this function checks the reason of the Channel Halt,
 *	and according to the reason,
 *	calls the sub-functions to process the result.
 *
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_TRANSMIT	-if need to retransmit the result_td.
 *		RE_SCHEDULE	-if need to reschedule the result_td.
 *		DE_ALLOCATE	-if USB Transfer is completed.
 */
/******************************************************************************/
static u8 process_chhltd_on_intr(struct td *result_td,
				struct hc_info *hcinfo)
{
	if (result_td->parent_ed_p->ed_desc.is_ep_in) {
		if (hcinfo->hc_int.b.xfercompl)
			return process_xfercompl_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.stall)
			return process_stall_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.bblerr)
			return process_bblerr_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.nak)
			return process_nak_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.datatglerr)
			return process_datatgl_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.frmovrun)
			return process_frmovrrun_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.xacterr)
			return process_xacterr_on_intr(result_td, hcinfo);

		else {
			clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
					CH_STATUS_ALL);

			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_ACK);
			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_NAK);
			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_DataTglErr);
			return RE_TRANSMIT;
		}

	} else {
		if (hcinfo->hc_int.b.xfercompl)
			return process_xfercompl_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.stall)
			return process_stall_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.nak)
			return process_nak_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.frmovrun)
			return process_frmovrrun_on_intr(result_td, hcinfo);

		else if (hcinfo->hc_int.b.xacterr)
			return process_xacterr_on_intr(result_td, hcinfo);

		else {
			clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
					CH_STATUS_ALL);

			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_ACK);
			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_NAK);
			mask_channel_interrupt(result_td->cur_stransfer.
					alloc_chnum, CH_STATUS_DataTglErr);

			result_td->err_cnt++;

			if (result_td->err_cnt == 3) {
				result_td->error_code = USB_ERR_STATUS_XACTERR;
				result_td->err_cnt = 0;
				return DE_ALLOCATE;
			}

			return RE_TRANSMIT;
		}
	}
}



/******************************************************************************/
/*!
 * @name	u8 process_intr_transfer(struct td *result_td,
 *					struct hc_info *HCRegData)
 *
 * @brief this function processes the result of the Interrupt Transfer.
 *	firstly, this function checks the result of the Interrupt Transfer.
 *	and according to the result, calls the sub-functions
 *	to process the result.
 *
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td whose channel is interruped.
 *		[IN]	HCRegData
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_TRANSMIT	-if need to retransmit the result_td.
 *		RE_SCHEDULE	-if need to reschedule the result_td.
 *		DE_ALLOCATE	-if USB Transfer is completed.
 *		NO_ACTION	-if we don't need any action,
 */
/******************************************************************************/
static u8 process_intr_transfer(struct td *result_td,
		struct hc_info *hcinfo)
{
	hcintn_t hc_intr_info;
	u8 ret_val = 0;

	hc_intr_info.d32 = hcinfo->hc_int.d32 &
		result_td->cur_stransfer.hc_reg.hc_int_msk.d32;

	if (result_td->parent_ed_p->ed_desc.is_ep_in) {

		if (hc_intr_info.b.chhltd)
			ret_val = process_chhltd_on_intr(result_td, hcinfo);

		 else if (hc_intr_info.b.ack)
			ret_val = process_ack_on_intr(result_td, hcinfo);

	} else {
		if (hc_intr_info.b.chhltd)
			ret_val = process_chhltd_on_intr(result_td, hcinfo);

		else if (hc_intr_info.b.ack)
			ret_val = process_ack_on_intr(result_td, hcinfo);
	}

	return ret_val;
}


