/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : Scheduler.c
 *  [Description] : The source file implements the internal
 *		functions of Scheduler.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/04
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of Scheduler
 *	 -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com)
 *				: Optimizing for performance
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

/* the max periodic bus time is 80%*125us on High Speed Mode */
static	const	u32	perio_highbustime_threshold	= 100;

/* the max periodic bus time is 90%*1000us(1ms) on Full/Low Speed Mode. */
static	const	u32	perio_fullbustime_threshold	= 900;

static	const	u8	perio_chnum_threshold	= 14;
/* static	const	u8	total_chnum_threshold	= 16; */
static	u8		total_chnum_threshold	= 16;

static	u32	perio_used_bustime;
static	u8	perio_used_chnum;
static	u8	nonperio_used_chnum;
static	u8	total_used_chnum;
static	u32	transferring_td_array[16];


static int inc_perio_bus_time(u32 bus_time, u8 dev_speed)
{
	switch (dev_speed) {

	case HIGH_SPEED_OTG:
		if ((bus_time + perio_used_bustime) <=
				perio_highbustime_threshold) {

			perio_used_bustime += bus_time;
			return USB_ERR_SUCCESS;
		} else
			return USB_ERR_FAIL;

	case LOW_SPEED_OTG:
	case FULL_SPEED_OTG:
		if ((bus_time + perio_used_bustime) <=
				perio_fullbustime_threshold) {

			perio_used_bustime += bus_time;
			return USB_ERR_SUCCESS;
		} else
			return USB_ERR_FAIL;

	case SUPER_SPEED_OTG:
		break;
	default:
		break;
	}
	return USB_ERR_FAIL;
}

static int dec_perio_bus_time(u32 bus_time)
{
	if (perio_used_bustime >= bus_time) {
		perio_used_bustime -= bus_time;
		return USB_ERR_SUCCESS;
	} else
		return USB_ERR_FAIL;
}

static int inc_perio_chnum(void)
{
	if (perio_used_chnum < perio_chnum_threshold) {
		if (total_used_chnum < total_chnum_threshold) {
			perio_used_chnum++;
			total_used_chnum++;
			return USB_ERR_SUCCESS;
		}
	}
	return USB_ERR_FAIL;
}

static u8 get_avail_chnum(void)
{
	return total_chnum_threshold - total_used_chnum;
}

static int dec_perio_chnum(void)
{
	if (perio_used_chnum > 0) {
		if (total_used_chnum > 0) {
			perio_used_chnum--;
			total_used_chnum--;
			return USB_ERR_SUCCESS;
		}
	}
	return USB_ERR_FAIL;
}

static int inc_non_perio_chnum(void)
{
	if (nonperio_used_chnum < total_chnum_threshold) {
		if (total_used_chnum < total_chnum_threshold) {
			nonperio_used_chnum++;
			total_used_chnum++;
			return USB_ERR_SUCCESS;
		}
	}
	return USB_ERR_FAIL;
}

static int dec_nonperio_chnum(void)
{
	if (nonperio_used_chnum > 0) {
		if (total_used_chnum > 0) {
			nonperio_used_chnum--;
			total_used_chnum--;
			return USB_ERR_SUCCESS;
		}
	}
	return USB_ERR_FAIL;
}

static int get_transferring_td_array(u8 chnum, unsigned int *td_addr)
{
	if (transferring_td_array[chnum] != 0) {
		*td_addr = transferring_td_array[chnum];
		return USB_ERR_SUCCESS;
	}
	return USB_ERR_FAIL;
}

static int set_transferring_td_array(u8 chnum, u32 td_addr)
{
	if (td_addr == 0) {
		transferring_td_array[chnum] = td_addr;
		return USB_ERR_SUCCESS;
	}

	if (transferring_td_array[chnum] == 0) {
		transferring_td_array[chnum] = td_addr;
		return USB_ERR_SUCCESS;
	} else
		return USB_ERR_FAIL;
}

/******************************************************************************/
/*!
 * @name	int	do_periodic_schedule(struct sec_otghost *otghost)
 *
 * @brief this function schedules PeriodicTransferReadyQ.
 *	this function checks whether PeriodicTransferReadyQ has some ed_t.
 *	if there are some ed_t on PeriodicTransferReadyQ
 *	, this function request to start USB Trasnfer to S3C6400OCI.
 *
 *
 * @param	void
 *
 * @return	void
 */
/******************************************************************************/
static void do_periodic_schedule(struct sec_otghost *otghost)
{
	struct ed *scheduling_ed = NULL;
	int	err_sched = USB_ERR_SUCCESS;
	u32	sched_cnt = 0;

	otg_list_head	*td_list_entry;
	struct td		*td;
	u32		cur_frame_num = 0;
	u8 alloc_ch;

	otg_dbg(OTG_DBG_SCHEDULE, "***** Start periodicSchedul *****\n");

	sched_cnt = get_periodic_ready_q_entity_num();

	while (sched_cnt) {
		/* in periodic transfser,
		   the channel resource was already reserved.
		   So, we don't need this routine... */

start_sched_perio_transfer:
		if (!sched_cnt)
			goto end_sched_perio_transfer;

		otg_dbg(OTG_DBG_SCHEDULE, "sched_cnt : %d\n", sched_cnt);

		err_sched = get_ed_from_ready_q(&scheduling_ed, true);

		if (err_sched != USB_ERR_SUCCESS) {
			/* there is no ED on PeriodicTransferQ.
			   So we finish scheduling.*/
			otg_dbg(OTG_DBG_SCHEDULE, "no ed\n");
			goto end_sched_perio_transfer;
		}

		cur_frame_num = 0;

		otg_dbg(OTG_DBG_SCHEDULE,
			"the %d ed to be scheduled\n", (int)scheduling_ed);

		sched_cnt--;
		td_list_entry = scheduling_ed->td_list_entry.next;

		if (td_list_entry == &scheduling_ed->td_list_entry) {
			/* scheduling_ed has no struct td.
			   so we schedules another ed on
			   PeriodicTransferReadyQ. */
			goto start_sched_perio_transfer;
		}

		if (scheduling_ed->ed_status.is_in_transferring) {
			/* scheduling_ed is already Scheduled.
			   so we schedules another ed on
			   PeriodicTransferReadyQ. */
			goto start_sched_perio_transfer;
		}

		cur_frame_num = oci_get_frame_num();

		if (((cur_frame_num - scheduling_ed->ed_desc.sched_frame) &
			HFNUM_MAX_FRNUM) > (HFNUM_MAX_FRNUM >> 1)) {

			insert_ed_to_ready_q(scheduling_ed, false);
			goto start_sched_perio_transfer;
		}

		td	= otg_list_get_node(td_list_entry,
				struct td, td_list_entry);

		if ((td->is_transferring) || (td->is_transfer_done)) {
			/* the selected struct td was already transferring
			   or completed to transfer.
			   we should decide how to control this case.*/
			goto end_sched_perio_transfer;
		}

		otg_dbg(OTG_DBG_SCHEDULE,
				"the struct td to be scheduled :%d", (int)td);

		alloc_ch = oci_start_transfer(otghost, &td->cur_stransfer);

		if (alloc_ch < total_chnum_threshold) {

			td->cur_stransfer.alloc_chnum = alloc_ch;
			transferring_td_array[alloc_ch] = (u32)td;

			scheduling_ed->ed_status.is_in_transferring = true;
			scheduling_ed->ed_status.is_in_transfer_ready_q = false;
			scheduling_ed->ed_status.in_transferring_td = (u32)td;

			td->is_transferring = true;

		} else {
			/* we should insert the ed_t to TransferReadyQ,
			   because the USB Transfer of current ed is failed.*/
			scheduling_ed->ed_status.is_in_transferring = false;
			scheduling_ed->ed_status.is_in_transfer_ready_q = true;
			scheduling_ed->ed_status.in_transferring_td = 0;

			insert_ed_to_ready_q(scheduling_ed, true);

			scheduling_ed->is_need_to_insert_scheduler = false;
			goto end_sched_perio_transfer;
		}

	}

end_sched_perio_transfer:

	return;
}


/******************************************************************************/
/*!
 * @name	int	do_nonperiodic_schedule(struct sec_otghost *otghost)
 *
 * @brief this function start to schedule thie NonPeriodicTransferReadyQ.
 *
 *	this function checks whether NonPeriodicTransferReadyQ has some ed.
 *	if there are some ed on NonPeriodicTransferReadyQ,
 *	this function request to start USB Trasnfer to S3C6400OCI.
 *
 * @param	void
 * @return	void
 */
/******************************************************************************/
static void do_nonperiodic_schedule(struct sec_otghost *otghost)
{
	struct ed *scheduling_ed;
	int	err_sched;

	otg_list_head *td_list_entry;
	struct td *td;
	u8	alloc_ch;

	if (total_used_chnum >= total_chnum_threshold) {
		pr_info("shost nonperiodic: used_chnum >= total_chnum\n");
		return;
	}

	while (1) {

start_nonperiodic:
		/* check there is available channel resource
		   for Non-Periodic Transfer. */
		if (total_used_chnum == total_chnum_threshold) {
			pr_info("shost nonperiodic: used_chnum == total_chnum\n");
			goto end_nonperiodic;
		}

		err_sched = get_ed_from_ready_q(&scheduling_ed, false);

		if (err_sched != USB_ERR_SUCCESS) {
			/* pr_info("shost: non: nothing to schedule\n"); */
			goto end_nonperiodic;
		}

		td_list_entry =	scheduling_ed->td_list_entry.next;

		if (otg_list_empty(&scheduling_ed->td_list_entry)) {
			/* scheduling_ed has no td.
			   so we schedules another ed
			   on PeriodicTransferReadyQ.*/
			goto start_nonperiodic;
		}

		if (scheduling_ed->ed_status.is_in_transferring) {
			/* scheduling_ed is already Scheduled.
			   so we schedules another ed
			   on PeriodicTransferReadyQ.*/
			goto start_nonperiodic;
		}

		td = otg_list_get_node(td_list_entry, struct td, td_list_entry);

		if ((td->is_transferring) || (td->is_transfer_done)) {
			pr_info("shost nonperiodic: td is busy\n");
			goto end_nonperiodic;
		}

		alloc_ch = oci_start_transfer(otghost, &td->cur_stransfer);

		if (alloc_ch < total_chnum_threshold) {

			td->cur_stransfer.alloc_chnum = alloc_ch;
			transferring_td_array[alloc_ch]	= (u32)td;

			inc_non_perio_chnum();

			scheduling_ed->ed_status.is_in_transferring = true;
			scheduling_ed->ed_status.is_in_transfer_ready_q = false;
			scheduling_ed->ed_status.in_transferring_td = (u32)td;
			td->is_transferring = true;

		} else {
			/* we must insert the ed to queue,
			   because the USB Transfer of the ed is failed.*/
			scheduling_ed->ed_status.is_in_transferring = false;
			scheduling_ed->ed_status.in_transferring_td = 0;
			insert_ed_to_ready_q(scheduling_ed, true);
			scheduling_ed->ed_status.is_in_transfer_ready_q = true;

			goto end_nonperiodic;
		}
	} /* while */

end_nonperiodic:
	return;
}

/******************************************************************************/
/*!
 * @name int insert_ed_to_scheduler(struct sec_otghost *otghost,
 *			ed_t *insert_ed)
 *
 * @brief this function transfers the insert_ed to S3C6400Scheduler,
 *	and after that, the insert_ed is inserted to TransferReadyQ
 *	and scheduled by Scheduler.
 *
 * @param	[IN]	insert_ed
 * = indicates pointer of ed_t to be inserted to TransferReadyQ.
 *
 * @return
 * USB_ERR_ALREADY_EXIST - if the insert_ed is already existed.
 * USB_ERR_SUCCESS - if success to insert insert_ed to S3CScheduler.
 */
/******************************************************************************/
int insert_ed_to_scheduler(struct sec_otghost *otghost, struct ed *insert_ed)
{
	if (!insert_ed->is_need_to_insert_scheduler)
		return USB_ERR_ALREADY_EXIST;

	otg_dbg(OTG_DBG_SCHEDULE_ED, "ed_id %d, td %d\n",
			insert_ed->ed_id, insert_ed->num_td);

	insert_ed_to_ready_q(insert_ed, false);
	insert_ed->is_need_to_insert_scheduler = false;
	insert_ed->ed_status.is_in_transfer_ready_q = true;

	do_periodic_schedule(otghost);
	do_nonperiodic_schedule(otghost);

	return USB_ERR_SUCCESS;
}

