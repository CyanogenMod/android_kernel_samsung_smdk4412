/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-common-datastruct.h
 *  [Description] : The Header file defines Data Structures
 *		to be used at sub-modules of S3C6400HCD.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *   (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *   - Created this file and defines Data Structure to be managed by Transfer.
 *   (2) 2008/08/18   by SeungSoo Yang ( ss1.yang@samsung.com )
 *   - modifying ED structure
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

#ifndef _DATA_STRUCT_DEF_H
#define _DATA_STRUCT_DEF_H


struct isoch_packet_desc {
	/* start address of buffer is buffer address + uiOffsert.*/
	u32	isoch_packiet_start_addr;
	u32	buf_size;
	u32	transferred_szie;
	u32	isoch_status;
};

struct  standard_dev_req_info {
	bool is_data_stage;
	u8   conrol_transfer_stage;
	u32  vir_standard_dev_req_addr;
	u32  phy_standard_dev_req_addr;
};

struct control_data_toggle {
	u8 setup;
	u8 data;
	u8 status;
};

struct ed_status {
	struct control_data_toggle		control_data_toggle;
	u8			data_toggle;
	bool		is_ping_enable;
	bool		is_in_transfer_ready_q;
	bool		is_in_transferring;
	u32			in_transferring_td;
	bool		is_alloc_resource_for_ed;
};

struct ed_desc {
	u8	device_addr;
	u8	endpoint_num;
	bool	is_ep_in;
	u8	dev_speed;
	u8	endpoint_type;
	u16	max_packet_size;
	u8	mc;
	u8	interval;
	u32         sched_frame;
	u32         used_bus_time;
	u8	hub_addr;
	u8	hub_port;
	bool        is_do_split;
};

struct hc_reg {
	hcintmsk_t	hc_int_msk;
	hcintn_t		hc_int;
	u32		dma_addr;

};

struct stransfer {
	u32		stransfer_id;
	u32		parent_td;
	struct ed_desc		*ed_desc_p;
	struct ed_status	*ed_status_p;
	u32		vir_addr;
	u32		phy_addr;
	u32		buf_size;
	u32		packet_cnt;
	u8		alloc_chnum;
	struct hc_reg		hc_reg;
};

struct ed {
	u32				ed_id;
	bool			is_halted;
	bool			is_need_to_insert_scheduler;
	struct ed_desc		ed_desc;
	struct ed_status		ed_status;
	otg_list_head	ed_list_entry;
	otg_list_head	td_list_entry;
	otg_list_head	readyq_list;
	u32				num_td;
	void			*ed_private;
};

struct td {
	u32				td_id;
	struct ed		*parent_ed_p;
	void			*call_back_func_p;
	void			*call_back_func_param_p;
	bool			is_transferring;
	bool			is_transfer_done;
	u32				transferred_szie;
	bool			is_standard_dev_req;

	struct standard_dev_req_info		standard_dev_req_info;
	u32				vir_buf_addr;
	u32				phy_buf_addr;
	u32				buf_size;
	u32				transfer_flag;

	struct stransfer			cur_stransfer;
	u32				error_code;
	u32				err_cnt;
	otg_list_head			td_list_entry;

	/* Isochronous Transfer Specific */
	u32				isoch_packet_num;
	struct isoch_packet_desc		*isoch_packet_desc_p;
	u32				isoch_packet_index;
	u32				isoch_packet_position;
	u32				sched_frame;
	u32				interval;
	u32				used_total_bus_time;

	/* the private data can be used by S3C6400Interface. */
	void				*td_private;
};

struct trans_ready_q {
	bool			is_periodic;
	otg_list_head	entity_list;
	u32				entity_num;

	/* In case of Periodic Transfer */
	u32		total_perio_bus_bandwidth;
	u8		total_alloc_chnum;
};

struct	hc_info {
	hcintmsk_t			hc_int_msk;
	hcintn_t			hc_int;
	u32					dma_addr;
	hcchar_t			hc_char;
	hctsiz_t			hc_size;
};

#ifndef USB_MAXCHILDREN
	#define USB_MAXCHILDREN (31)
#endif

/* TODO: use usb_hub_descriptor
 *  <linux/usb/ch11.h>
 */
struct hub_descriptor {
	u8  desc_length;
	u8  desc_type;
	u8  port_number;
	u16 hub_characteristics;
	u8  power_on_to_power_good;
	u8  hub_control_current;

	/* add 1 bit for hub status change; round to bytes */
	u8  DeviceRemovable[(USB_MAXCHILDREN + 1 + 7) / 8];
	u8  port_pwr_ctrl_mask[(USB_MAXCHILDREN + 1 + 7) / 8];
} __packed;

#endif
