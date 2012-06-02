/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-list.h
 * @brief  list functions for otg
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com)
 *	  : Creating the initial version of this code
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com)
 *	  : Optimizing for performance
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

#ifndef _S3C_OTG_HCDI_LIST_H_
#define _S3C_OTG_HCDI_LIST_H_

#include <linux/list.h>

typedef   struct  list_head    otg_list_head;

#define	otg_list_get_node(ptr, type, member) container_of(ptr, type, member)


#define	otg_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


/**
 * void	otg_list_push_next(otg_list_head *n, otg_list_head *list)
 *
 * @brief push a list node into the next of head
 *
 * @param [in] n : node to be pushed
 * @param [in] otg_list_head : target list head
 *
 * @return void \n
 */

static	inline
void otg_list_push_next(otg_list_head *n, otg_list_head *list)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_push_next\n");
	list_add(n, list);
}

/**
 * void	otg_list_push_prev(otg_list_head *n, otg_list_head *list)
 *
 * @brief push a list node into the previous of head
 *
 * @param [in] n : node to be pushed
 * @param [in] otg_list_head : target list head
 *
 * @return void \n
 */
static	inline
void otg_list_push_prev(otg_list_head *n, otg_list_head *list)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_push_prev\n");
	list_add_tail(n, list);
}

/**
 * void	otg_list_pop(otg_list_head *list_entity_p)
 *
 * @brief pop a list node
 *
 * @param [in] n : node to be poped
 * @param [in] otg_list_head : target list head
 *
 * @return void \n
 */
static	inline
void otg_list_pop(otg_list_head *list_entity_p)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_pop\n");

	if (list_entity_p->prev && list_entity_p->next)
		list_del(list_entity_p);
	else
		pr_info("usb: otg_list_pop error\n");
}

/**
 * void	otg_list_move_next(otg_list_head *node_p, otg_list_head *list)
 *
 * @brief move a list to next of head
 *
 * @param [in] n : node to be moved
 * @param [in] otg_list_head : target list head
 *
 * @return void \n
 */
static	inline
void otg_list_move_next(otg_list_head *node_p, otg_list_head *list)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_move_next\n");
	list_move(node_p, list);
}

/**
 * void	otg_list_move_prev(otg_list_head *node_p, otg_list_head *list)
 *
 * @brief move a list to previous of head
 *
 * @param [in] n : node to be moved
 * @param [in] otg_list_head : target list head
 *
 * @return void \n
 */
static	inline
void otg_list_move_prev(otg_list_head *node_p, otg_list_head *list)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_move_prev\n");
	list_move_tail(node_p, list);
}

/**
 * bool	otg_list_empty(otg_list_head *list)
 *
 * @brief check a list empty or not
 *
 * @param [in] list : node to check
 *
 * @return true : empty list \n
 *	false : not empty list
 */
static	inline
bool otg_list_empty(otg_list_head *list)
{
	/* otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_empty\n"); */
	if (list_empty(list))
		return true;
	return false;
}

/**
 * void	otg_list_merge(otg_list_head *list_p, otg_list_head *head_p)
 *
 * @brief merge two list
 *
 * @param [in] list_p : a head
 * @param [in] head_p : target list head
 *
 * @return void \n
 */
static	inline
void otg_list_merge(otg_list_head *list_p, otg_list_head *head_p)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_merge\n");
	list_splice(list_p, head_p);
}

/**
 * void    otg_list_init(otg_list_head *list_p)
 *
 * @brief initialize a list
 *
 * @param [in] list_p : node to be initialized
 *
 * @return void \n
 */
static	inline
void    otg_list_init(otg_list_head *list_p)
{
	otg_dbg(OTG_DBG_OTGHCDI_LIST, "otg_list_init\n");
	list_p->next = list_p;
	list_p->prev = list_p;
}

#endif /* _S3C_OTG_HCDI_LIST_H_ */

