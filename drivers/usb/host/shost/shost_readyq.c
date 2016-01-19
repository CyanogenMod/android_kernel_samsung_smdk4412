/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : TransferReadyQ.c
 *  [Description] : The source file implements the internal
 *		functions of TransferReadyQ.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/04
 *  [Revision History]
 *      (1) 2008/06/04   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of TransferReadyQ.
 *	 -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *				: Optimizing for performance \n
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

static	struct trans_ready_q	periodic_trans_ready_q;
static	struct trans_ready_q	nonperiodic_trans_ready_q;

/******************************************************************************/
/*!
 * @name void		init_transfer_ready_q(void)
 *
 * @brief this function initiates PeriodicTransferReadyQ
 *	and NonPeriodicTransferReadyQ.
 *
 *
 * @param	void
 *
 * @return	void.
 */
/******************************************************************************/
static void	init_transfer_ready_q(void)
{
	otg_dbg(OTG_DBG_SCHEDULE, "start init_transfer_ready_q\n");

	otg_list_init(&periodic_trans_ready_q.entity_list);
	periodic_trans_ready_q.is_periodic = true;
	periodic_trans_ready_q.entity_num = 0;
	periodic_trans_ready_q.total_alloc_chnum = 0;
	periodic_trans_ready_q.total_perio_bus_bandwidth = 0;

	otg_list_init(&nonperiodic_trans_ready_q.entity_list);
	nonperiodic_trans_ready_q.is_periodic = false;
	nonperiodic_trans_ready_q.entity_num = 0;
	nonperiodic_trans_ready_q.total_alloc_chnum = 0;
	nonperiodic_trans_ready_q.total_perio_bus_bandwidth = 0;
}


/******************************************************************************/
/*!
 * @name int insert_ed_to_ready_q(struct ed *insert_ed,
 *					bool is_first)
 *
 * @brief this function inserts ed_t * to TransferReadyQ.
 *
 *
 * @param	[IN]	insert_ed
 * = indicates the ed_t to be inserted to TransferReadyQ.
 *		[IN]	is_first
 * = indicates whether the insert_ed is inserted
 * as first entry of TransferReadyQ.
 *
 * @return
 * USB_ERR_SUCCESS	-if successes to insert the insert_ed to TransferReadyQ.
 * USB_ERR_FAILl -if fails to insert the insert_ed to TransferReadyQ.
 */
/******************************************************************************/
static int insert_ed_to_ready_q(struct ed *insert_ed, bool is_first)
{
	otg_dbg(OTG_DBG_SCHEDULE_ED, "ed_id %d, td_id %d, %d\n",
			insert_ed->ed_id, insert_ed->num_td, is_first);

	if (insert_ed->ed_desc.endpoint_type == BULK_TRANSFER ||
		insert_ed->ed_desc.endpoint_type == CONTROL_TRANSFER) {

		if (is_first) {
			otg_list_push_next(&insert_ed->readyq_list,
					&nonperiodic_trans_ready_q.entity_list);
		} else {
			otg_list_push_prev(&insert_ed->readyq_list,
					&nonperiodic_trans_ready_q.entity_list);
		}
		nonperiodic_trans_ready_q.entity_num++;

	} else {
		if (is_first) {
			otg_list_push_next(&insert_ed->readyq_list,
					&periodic_trans_ready_q.entity_list);
		} else {
			otg_list_push_prev(&insert_ed->readyq_list,
					&periodic_trans_ready_q.entity_list);
		}
		periodic_trans_ready_q.entity_num++;
	}
	return USB_ERR_SUCCESS;
}


static u32 get_periodic_ready_q_entity_num(void)
{
	return periodic_trans_ready_q.entity_num;
}

/******************************************************************************/
/*!
 * @name int remove_ed_from_ready_q(ed_t	*remove_ed)
 *
 * @brief this function removes ed_t * from TransferReadyQ.
 *
 *
 * @param	[IN]	remove_ed
 * = indicate the ed_t to be removed from TransferReadyQ.
 *
 * @return
 * USB_ERR_SUCCESS -if successes to remove the remove_ed from TransferReadyQ.
 * USB_ERR_FAILl -if fails to remove the remove_ed from TransferReadyQ.
 */
/******************************************************************************/
static int remove_ed_from_ready_q(struct ed	*remove_ed)
{
	otg_dbg(OTG_DBG_SCHEDULE_ED, "ed_id %d, td_id %d\n",
			remove_ed->ed_id, remove_ed->num_td);

	otg_list_pop(&remove_ed->readyq_list);

	if (remove_ed->ed_desc.endpoint_type == BULK_TRANSFER ||
		remove_ed->ed_desc.endpoint_type == CONTROL_TRANSFER) {

		nonperiodic_trans_ready_q.entity_num--;

	} else
		periodic_trans_ready_q.entity_num--;

	return USB_ERR_SUCCESS;
}

/******************************************************************************/
/*!
 * @name int get_ed_from_ready_q(bool is_periodic,
 *					struct td	**get_ed)
 *
 * @brief this function returns the first entity of TransferReadyQ.
 *	if there are some ed_t on TransferReadyQ,
 *	this function pops first ed_t from TransferReadyQ.
 *	So, the TransferReadyQ don's has the poped ed_t.
 *
 * @param	[IN]	is_periodic
 * = indicate whether Periodic or not
 *		[OUT]	get_ed
 * = indicate the double pointer to store the address of first entity
 *	on TransferReadyQ.
 *
 * @return
 * USB_ERR_SUCCESS	-if successes to get frist ed_t from TransferReadyQ.
 * USB_ERR_NO_ENTITY	-if fails to get frist ed_t from TransferReadyQ
 *  because there is no entity on TransferReadyQ.
 */
/******************************************************************************/

static int get_ed_from_ready_q(struct ed **get_ed, bool	is_periodic)
{
	otg_list_head	*qlist = NULL;

	/* periodic transfer : control and bulk */
	if (is_periodic) {

		if (periodic_trans_ready_q.entity_num == 0)
			return USB_ERR_NO_ENTITY;

		qlist = periodic_trans_ready_q.entity_list.next;

		if (!otg_list_empty(&periodic_trans_ready_q.entity_list)) {

			*get_ed = otg_list_get_node(qlist,
					struct ed, readyq_list);

			if (qlist->prev == LIST_POISON2 ||
				qlist->next == LIST_POISON1) {

				printk(KERN_ERR "shost scheduler: get_ed_from_ready_q error\n");
				periodic_trans_ready_q.entity_num = 0;

			} else {
				otg_list_pop(qlist);
				periodic_trans_ready_q.entity_num--;
			}
			return USB_ERR_SUCCESS;

		} else
			return USB_ERR_NO_ENTITY;

	/* non-periodic transfer : interrupt and ischo */
	} else {

		if (nonperiodic_trans_ready_q.entity_num == 0)
			return USB_ERR_NO_ENTITY;

		qlist = nonperiodic_trans_ready_q.entity_list.next;

		if (!otg_list_empty(&nonperiodic_trans_ready_q.entity_list)) {

			*get_ed = otg_list_get_node(qlist,
					struct ed, readyq_list);

			if (qlist->prev == LIST_POISON2 ||
				qlist->next == LIST_POISON1) {

				printk(KERN_ERR "s3c-otg-scheduler get_ed_from_ready_q error\n");
				nonperiodic_trans_ready_q.entity_num = 0;

			} else {
				otg_list_pop(qlist);
				nonperiodic_trans_ready_q.entity_num--;
			}
			return USB_ERR_SUCCESS;
		} else
			return USB_ERR_NO_ENTITY;
	}
}


