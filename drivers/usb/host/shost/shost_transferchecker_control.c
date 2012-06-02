/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : ControlTransferChecker.c
 *  [Description] : The Source file implements the external
 *		and internal functions of ControlTransferChecker.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2009/02/10
 *  [Revision History]
 *      (1) 2008/06/13   by Yang Soon Yeal { syatom.yang@samsung.com }
 *	- Created this file and implements functions of ControlTransferChecker
 *      (2) 2008/06/18   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Completed to implement ControlTransferChecker.c v1.0
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
 * @name	u8	process_xfercompl_on_control(struct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the xfercompl event
 *	the procedure of this function is as following
 *	1. clears all bits of the channel' HCINT
 *		by using clear_ch_intr() of S3CIsr.
 *	2. masks some bit of HCINTMSK
 *	3. updates the result_td fields
 *		err_cnt/u8/standard_dev_req_info.
 *	4. updates the result_td->parent_ed_p->ed_status.
 *		control_data_tgl.
 *	5. calculates the tranferred size
 *		by calling calc_transferred_size() on DATA_STAGE.
 *
 * @param	[IN]	result_td
 *	-indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 *	-indicates the interrupt information of the Channel to be interrupted
 *
 * @return	USB_ERR_SUCCESS
 */
/******************************************************************************/
static u8 process_xfercompl_on_control(struct td *result_td, struct hc_info *hcinfo)
{
	u8 ret_val = 0;

	result_td->err_cnt = 0;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ALL);

	/* Mask ack Interrupt.. */
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->parent_ed_p->ed_status.is_ping_enable = false;

	switch (result_td->standard_dev_req_info.conrol_transfer_stage) {

	case SETUP_STAGE:
		if (result_td->standard_dev_req_info.is_data_stage)
			result_td->standard_dev_req_info.
				conrol_transfer_stage = DATA_STAGE;
		else
			result_td->standard_dev_req_info.
				conrol_transfer_stage = STATUS_STAGE;

		ret_val = RE_TRANSMIT;

		break;

	case DATA_STAGE:

		result_td->transferred_szie +=
			calc_transferred_size(true, result_td, hcinfo);

		/* at IN Transfer, short transfer is accepted. */
		if (result_td->transferred_szie == result_td->buf_size) {
			result_td->standard_dev_req_info.
				conrol_transfer_stage = STATUS_STAGE;
			result_td->error_code = USB_ERR_STATUS_COMPLETE;

		} else {
			if (result_td->parent_ed_p->ed_desc.is_ep_in &&
					hcinfo->hc_size.b.xfersize) {

				if (result_td->transfer_flag &
						USB_TRANS_FLAG_NOT_SHORT) {

					result_td->error_code =
						USB_ERR_STATUS_SHORTREAD;
					result_td->standard_dev_req_info.
						conrol_transfer_stage =
						STATUS_STAGE;

				} else {
					result_td->error_code =
						USB_ERR_STATUS_COMPLETE;
					result_td->standard_dev_req_info.
						conrol_transfer_stage =
						STATUS_STAGE;
				}

			} else {
				/* the Data Stage is not completed.
				   So we need to continue Data Stage. */
				result_td->standard_dev_req_info.
					conrol_transfer_stage = DATA_STAGE;
				update_data_toggle(result_td,
						hcinfo->hc_size.b.pid);
			}
		}

		if (hcinfo->hc_int.b.nyet) {
			/* at OUT Transfer, we must re-transmit. */
			if (result_td->parent_ed_p->ed_desc.is_ep_in == false) {

				if (result_td->parent_ed_p->ed_desc.
						dev_speed == HIGH_SPEED_OTG)
					result_td->parent_ed_p->ed_status.
						is_ping_enable = true;
				else
					result_td->parent_ed_p->ed_status.
						is_ping_enable = false;
			}
		}
		ret_val = RE_TRANSMIT;
		break;

	case STATUS_STAGE:
		result_td->standard_dev_req_info.
			conrol_transfer_stage = COMPLETE_STAGE;

		if (hcinfo->hc_int.b.nyet) {
			/* at OUT Transfer, we must re-transmit. */
			if (result_td->parent_ed_p->ed_desc.is_ep_in == false) {

				if (result_td->parent_ed_p->ed_desc.
						dev_speed == HIGH_SPEED_OTG)
					result_td->parent_ed_p->ed_status.
						is_ping_enable = true;
				else
					result_td->parent_ed_p->ed_status.
						is_ping_enable = false;
			}
		}

		ret_val = DE_ALLOCATE;
		break;

	default:
		break;
	}

	return ret_val;
}

/******************************************************************************/
/*!
 * @name	u8	process_ahb_on_control(ruct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with theAHB Errorl event
 *			this function stop the channel to be executed
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 */
/******************************************************************************/

/* TODO: not used. remove */
#if 0
static u8 process_ahb_on_control(struct td *result_td,
		struct hc_info *hcinfo)
{
	result_td->err_cnt = 0;
	result_td->error_code = USB_ERR_STATUS_AHBERR;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_AHBErr);

	/* Mask ack Interrupt.. */
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	/* we just calculate the size of the transferred data
	   on Data Stage of Control Transfer. */
	if (result_td->standard_dev_req_info.
			conrol_transfer_stage == DATA_STAGE)
		result_td->transferred_szie +=
			calc_transferred_size(false, result_td, hcinfo);

	return DE_ALLOCATE;

}
#endif

/******************************************************************************/
/*!
 * @name	u8	process_stall_on_control(struct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with theStall event
 *	but USB2.0 Spec don't permit the Stall
 *	on Setup Stage of Control Transfer.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 */
/******************************************************************************/
static u8 process_stall_on_control(struct td *result_td,
		struct hc_info *hcinfo)
{
	result_td->err_cnt = 0;
	result_td->error_code = USB_ERR_STATUS_STALL;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_ALL);

	/* Mask ack Interrupt.. */
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->parent_ed_p->ed_status.is_ping_enable = false;

	/* we just calculate the size of the transferred data
	   on Data Stage of Control Transfer. */
	if (result_td->standard_dev_req_info.
			conrol_transfer_stage == DATA_STAGE)
		result_td->transferred_szie +=
			calc_transferred_size(false, result_td, hcinfo);

	return DE_ALLOCATE;
}

/******************************************************************************/
/*!
 * @name	u8	process_nak_on_control(ruct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the nak event
 *	nak is occured at OUT/IN Transaction of Data/Status Stage,
 *	and is not occured at Setup Stage.
 *	If nak is occured at IN Transaction,
 *	this function processes this interrupt as following.
 *	1. resets the result_td->err_cnt.
 *	2. masks ack/nak/DaaTglErr bit of HCINTMSK.
 *	3. clears the nak bit of HCINT
 *	4. be careful, nak of IN Transaction don't require re-transmit.
 *
 *	If nak is occured at OUT Transaction,
 *	this function processes this interrupt as following.
 *	1. all procedures of IN Transaction are executed.
 *	2. calculates the size of the transferred data.
 *	3. if the speed of USB Device is High-Speed, sets the ping protocol.
 *	4. update the Toggle
 *	at OUT Transaction, this function check whether	the speed of
 *	USB Device is High-Speed or not.
 *	if USB Device is High-Speed, then
 *	this function sets the ping protocol.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		 [IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_SCHEDULE
 */
/******************************************************************************/
static u8 process_nak_on_control(struct td *result_td,
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

	/* at OUT Transfer, we must re-transmit. */
	if (result_td->parent_ed_p->ed_desc.is_ep_in == false) {
		result_td->transferred_szie +=
			calc_transferred_size(false, result_td, hcinfo);

		update_data_toggle(result_td, hcinfo->hc_size.b.pid);
	}

	if (result_td->parent_ed_p->ed_desc.dev_speed == HIGH_SPEED_OTG) {

		if (result_td->standard_dev_req_info.
				conrol_transfer_stage == DATA_STAGE) {
			if (result_td->parent_ed_p->ed_desc.is_ep_in == false)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;
		}

		else if (result_td->standard_dev_req_info.
				conrol_transfer_stage == STATUS_STAGE) {

			if (result_td->parent_ed_p->ed_desc.is_ep_in == true)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;

		} else
			result_td->parent_ed_p->ed_status.
				is_ping_enable = false;
	}

	return RE_SCHEDULE;
}

/******************************************************************************/
/*!
 * @name	u8	process_ack_on_control(ruct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the ack event
 *			ack of IN/OUT Transaction don't need any retransmit.
 *			this function just resets result_td->err_cnt
 *	and masks ack/nak/DataTgl of HCINTMSK.
 *			finally, this function clears ack bit of HCINT.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	NO_ACTION
 */
/******************************************************************************/
static u8 process_ack_on_control(struct td *result_td,
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

	result_td->parent_ed_p->ed_status.is_ping_enable = false;

	return NO_ACTION;

}

/******************************************************************************/
/*!
 * @name	u8	process_nyet_on_control(struct td	*result_td,
 *					struct hc_info	*hcinfo)
 *
 * @brief		this function deals with the nyet event
 *	nyet is occured at OUT Transaction of Data/Status Stage,
 *	and is not occured at Setup Stage.
 *	If nyet is occured at OUT Transaction,
 *	this function processes this interrupt as following.
 *	1. resets the result_td->err_cnt.
 *	2. masks ack/nak/datatglerr bit of HCINTMSK.
 *	3. clears the nyet bit of HCINT
 *	4. calculates the size of the transferred data.
 *	5. if the speed of USB Device is High-Speed, sets the ping protocol.
 *	6. update the Data Toggle.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_SCHEDULE
 */
/******************************************************************************/
static u8	process_nyet_on_control(struct td	*result_td,
				struct hc_info		*hcinfo)
{
	result_td->err_cnt = 0;

	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_NYET);

	result_td->transferred_szie +=
		calc_transferred_size(false, result_td, hcinfo);

	if (result_td->parent_ed_p->ed_desc.dev_speed == HIGH_SPEED_OTG) {
		if (result_td->standard_dev_req_info.
				conrol_transfer_stage == DATA_STAGE) {

			if (result_td->parent_ed_p->ed_desc.is_ep_in == false)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;
		}

		else if (result_td->standard_dev_req_info.
				conrol_transfer_stage == STATUS_STAGE) {

			if (result_td->parent_ed_p->ed_desc.is_ep_in == true)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;

		} else
			result_td->parent_ed_p->ed_status.
				is_ping_enable = false;
	}

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	return RE_SCHEDULE;
}

/******************************************************************************/
/*!
 * @name	u8	process_xacterr_on_control(ruct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the xacterr event
 *	xacterr is occured at OUT/IN Transaction of Data/Status Stage,
 *	and is not occured at Setup Stage.
 *	if Timeout/CRC error/false EOP is occured, then xacterr is occured.
 *	the procedure to process xacterr is as following.
 *	1. increses the result_td->err_cnt
 *	2. check whether the result_td->err_cnt is equal to 3.
 *	3. unmasks ack/nak/datatglerr bit of HCINTMSK.
 *	4. clears the xacterr bit of HCINT
 *	5. calculates the size of the transferred data.
 *	6. if the speed of USB Device is High-Speed, sets the ping protocol.
 *	7. update the Data Toggle.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_TRANSMIT	-if the Error Counter is less than RETRANSMIT_THRESHOLD
 * DE_ALLOCATE -if the Error Counter is equal to RETRANSMIT_THRESHOLD
 */
/******************************************************************************/
static u8 process_xacterr_on_control(struct td *result_td,
		struct hc_info *hcinfo)
{
	u8	ret_val = 0;

	if (result_td->err_cnt < RETRANSMIT_THRESHOLD) {

		result_td->cur_stransfer.hc_reg.hc_int_msk.d32 |=
			(CH_STATUS_ACK + CH_STATUS_NAK + CH_STATUS_DataTglErr);

		ret_val = RE_TRANSMIT;

		unmask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_ACK);
		unmask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_NAK);
		unmask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_DataTglErr);
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
		result_td->error_code = USB_ERR_STATUS_XACTERR;
	}

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	if (result_td->standard_dev_req_info.
			conrol_transfer_stage == DATA_STAGE)
		result_td->transferred_szie +=
			calc_transferred_size(false, result_td, hcinfo);

	if (result_td->parent_ed_p->ed_desc.dev_speed == HIGH_SPEED_OTG) {

		if (result_td->standard_dev_req_info.
				conrol_transfer_stage == DATA_STAGE) {
			if (result_td->parent_ed_p->ed_desc.is_ep_in == false)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;
		}

		else if (result_td->standard_dev_req_info.
				conrol_transfer_stage == STATUS_STAGE) {

			if (result_td->parent_ed_p->ed_desc.is_ep_in == true)
				result_td->parent_ed_p->ed_status.
					is_ping_enable = true;

		} else
			result_td->parent_ed_p->ed_status.is_ping_enable =
				false;
	}

	update_data_toggle(result_td, hcinfo->hc_size.b.pid);

	return ret_val;

}

/******************************************************************************/
/*!
 * @name	void	process_bblerr_on_control(truct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the Babble event
 *			babble error can be just occured at IN Transaction.
 *	So if the direction of transfer is
 *			OUT, this function return Error Code.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	DE_ALLOCATE
 *		NO_ACTION	-if the direction is OUT
 */
/******************************************************************************/
static u8	process_bblerr_on_control(struct td	*result_td,
					struct hc_info *hcinfo)
{

	if (!result_td->parent_ed_p->ed_desc.is_ep_in)
		return NO_ACTION;

	result_td->err_cnt = 0;
	result_td->error_code = USB_ERR_STATUS_BBLERR;

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum,	CH_STATUS_ALL);

	/* Mask ack Interrupt.. */
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
			CH_STATUS_DataTglErr);

	result_td->parent_ed_p->ed_status.is_ping_enable = false;

	/* we just calculate the size of the transferred data
	   on Data Stage of Control Transfer. */
	if (result_td->standard_dev_req_info.
			conrol_transfer_stage == DATA_STAGE)
		result_td->transferred_szie +=
			calc_transferred_size(false, result_td, hcinfo);

	return DE_ALLOCATE;
}

/******************************************************************************/
/*!
 * @name	u8	process_datatgl_on_control(struct td	*result_td,
 *						struct hc_info *hcinfo)
 *
 *
 * @brief		this function deals with the datatglerr event
 *	the datatglerr event is occured at IN Transfer.
 *	this function just resets result_td->err_cnt
 *	and masks ack/nak/DataTgl of HCINTMSK.
 *	finally, this function clears datatglerr bit of HCINT.
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	NO_ACTION
 */
/******************************************************************************/
static u8 process_datatgl_on_control(struct td *result_td,
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
			CH_STATUS_DataTglErr);

	return NO_ACTION;

}


/******************************************************************************/
/*!
 * @name	u8	process_chhltd_on_control(truct td	*result_td,
 *						struct hc_info *hcinfo)
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
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_TRANSMIT	-if need to retransmit the result_td.
 *		RE_SCHEDULE	-if need to reschedule the result_td.
 *		DE_ALLOCATE	-if USB Transfer is completed.
 */
/******************************************************************************/
static u8	process_chhltd_on_control(struct td *result_td,
					struct hc_info *hcinfo)
{
	if (hcinfo->hc_int.b.xfercompl)
		return process_xfercompl_on_control(result_td, hcinfo);

	else if (hcinfo->hc_int.b.stall)
		return process_stall_on_control(result_td, hcinfo);

	else if (hcinfo->hc_int.b.bblerr)
		return process_bblerr_on_control(result_td, hcinfo);

	else if (hcinfo->hc_int.b.xacterr)
		return process_xacterr_on_control(result_td, hcinfo);

	else if (hcinfo->hc_int.b.nak)
		return process_nak_on_control(result_td, hcinfo);

	else if (hcinfo->hc_int.b.nyet)
		return process_nyet_on_control(result_td, hcinfo);

	else {

		/* Occure Error State.....*/
		clear_ch_intr(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_ALL);

		/* Mask ack Interrupt.. */
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_ACK);
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_NAK);
		mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum,
				CH_STATUS_DataTglErr);
		result_td->err_cnt++;

		if (result_td->err_cnt == 3) {

			result_td->error_code = USB_ERR_STATUS_XACTERR;
			result_td->err_cnt = 0;
			return DE_ALLOCATE;
		}
		return RE_TRANSMIT;
	}
	return USB_ERR_SUCCESS;
}


/******************************************************************************/
/*!
 * @name	u8	process_control_transfer(struct td	*result_td,
 *					     struct hc_info	*hcinfo)
 *
 * @brief		this function processes the result of the Control Transfer.
 *	firstly, this function checks the result the Control Transfer.
 *	and according to the result, calls the sub-functions
 *	to process the result.
 *
 *
 * @param	[IN]	result_td
 * -indicates  the pointer of the struct td to be mapped with the uChNum.
 *		[IN]	ubChNum
 * -indicates the number of the channel to be interrupted.
 *		[IN]	hcinfo
 * -indicates the interrupt information of the Channel to be interrupted
 *
 * @return	RE_TRANSMIT	-if need to retransmit the result_td.
 *		RE_SCHEDULE	-if need to reschedule the result_td.
 *		DE_ALLOCATE	-if USB Transfer is completed.
 */
/******************************************************************************/
static u8 process_control_transfer(struct td *result_td,
				struct hc_info *hcinfo)
{
	hcintn_t hcintr_info;
	u8 ret_val = 0;

	/* we just deal with the interrupts to be unmasked. */
	hcintr_info.d32 = hcinfo->hc_int.d32 &
		result_td->cur_stransfer.hc_reg.hc_int_msk.d32;

	if (result_td->parent_ed_p->ed_desc.is_ep_in) {
		if (hcintr_info.b.chhltd)
			ret_val = process_chhltd_on_control(result_td, hcinfo);

		else if (hcintr_info.b.ack)
			ret_val = process_ack_on_control(result_td, hcinfo);

		else if (hcintr_info.b.nak)
			ret_val = process_nak_on_control(result_td, hcinfo);

		else if (hcintr_info.b.datatglerr)
			ret_val = process_datatgl_on_control(result_td, hcinfo);

	} else {

		if (hcintr_info.b.chhltd)
			ret_val = process_chhltd_on_control(result_td, hcinfo);

		else if (hcintr_info.b.ack)
			ret_val = process_ack_on_control(result_td, hcinfo);
	}

	return ret_val;
}

