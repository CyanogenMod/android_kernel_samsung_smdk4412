/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MODEM_LINK_DEVICE_PLD_H__
#define __MODEM_LINK_DEVICE_PLD_H__

#include "modem_link_device_memory.h"

#define PLD_ADDR_MASK(x)	(0x00003FFF & (unsigned long)(x))

/*
	mbx_ap2cp +			0x0
	magic_code +
	access_enable +
	padding +
	mbx_cp2ap +			0x1000
	magic_code +
	access_enable +
	padding +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +		0x2000
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +		0x3000
	raw_rx_head + raw_rx_tail + raw_rx_buff +
  =	2 +
	4094 +
	2 +
	4094 +
	2 +
	2 +
	2 + 2 + 1020 +
	2 + 2 + 3064 +
	2 + 2 + 1020 +
	2 + 2 + 3064
 */

#define PLD_FMT_TX_BUFF_SZ	1024
#define PLD_RAW_TX_BUFF_SZ	3072
#define PLD_FMT_RX_BUFF_SZ	1024
#define PLD_RAW_RX_BUFF_SZ	3072

#define MAX_MSM_EDPRAM_IPC_DEV	2	/* FMT, RAW */

struct pld_ipc_map {
	u16 mbx_ap2cp;
	u16 magic_ap2cp;
	u16 access_ap2cp;
	u16 fmt_tx_head;
	u16 raw_tx_head;
	u16 fmt_rx_tail;
	u16 raw_rx_tail;
	u16 temp1;
	u8 padding1[4080];

	u16 mbx_cp2ap;
	u16 magic_cp2ap;
	u16 access_cp2ap;
	u16 fmt_tx_tail;
	u16 raw_tx_tail;
	u16 fmt_rx_head;
	u16 raw_rx_head;
	u16 temp2;
	u8 padding2[4080];

	u8 fmt_tx_buff[PLD_FMT_TX_BUFF_SZ];
	u8 raw_tx_buff[PLD_RAW_TX_BUFF_SZ];
	u8 fmt_rx_buff[PLD_RAW_TX_BUFF_SZ];
	u8 raw_rx_buff[PLD_RAW_RX_BUFF_SZ];

	u8 padding3[16384];

	u16 address_buffer;
};

struct pld_ext_op;

struct pld_link_device {
	struct link_device ld;

	/* DPRAM address and size */
	enum dpram_type type;	/* DPRAM type			*/
	u8 __iomem *base;	/* DPRAM base virtual address	*/
	u32 size;		/* DPRAM size			*/

	/* DPRAM IRQ GPIO# */
	unsigned gpio_dpram_int;

	/* DPRAM IRQ from CP */
	int irq;
	unsigned long irq_flags;
	char irq_name[MIF_MAX_NAME_LEN];

	/* Link to DPRAM control functions dependent on each platform */
	struct modemlink_dpram_control *dpctl;

	/* Physical configuration -> logical configuration */
	union {
		struct dpram_boot_map bt_map;
		struct qc_dpram_boot_map qc_bt_map;
	};

	struct dpram_dload_map dl_map;
	struct dpram_uload_map ul_map;

	/* IPC device map */
	struct dpram_ipc_map ipc_map;

	/* Pointers (aliases) to IPC device map */
	u16 __iomem *magic_ap2cp;
	u16 __iomem *access_ap2cp;
	u16 __iomem *magic_cp2ap;
	u16 __iomem *access_cp2ap;
	u16 __iomem *address_buffer;

	struct dpram_ipc_device *dev[MAX_IPC_DEV];
	u16 __iomem *mbx2ap;
	u16 __iomem *mbx2cp;

	/* Wakelock for DPRAM device */
	struct wake_lock wlock;
	char wlock_name[MIF_MAX_NAME_LEN];

	/* For booting */
	unsigned boot_start_complete;
	struct completion dpram_init_cmd;
	struct completion modem_pif_init_done;

	/* For UDL */
	struct tasklet_struct ul_tsk;
	struct tasklet_struct dl_tsk;
	struct completion udl_start_complete;
	struct completion udl_cmd_complete;
	struct dpram_udl_check udl_check;
	struct dpram_udl_param udl_param;

	/* For CP crash dump */
	struct timer_list crash_ack_timer;
	struct completion crash_start_complete;
	struct completion crash_recv_done;
	struct timer_list crash_timer;
	int crash_rcvd;		/* Count of CP crash dump packets received */

	/* For locking TX process */
	spinlock_t tx_rx_lock;
	spinlock_t pld_lock;

	/* For efficient RX process */
	struct tasklet_struct rx_tsk;
	struct mif_rxb_queue rxbq[MAX_IPC_DEV];
	struct io_device *iod[MAX_IPC_DEV];

	/* For retransmission after buffer full state */
	atomic_t res_required[MAX_IPC_DEV];

	/* For wake-up/sleep control */
	atomic_t accessing;

	/* Multi-purpose miscellaneous buffer */
	u8 *buff;

	/* PLD IPC initialization status */
	int init_status;

	/* Alias to device-specific IOCTL function */
	int (*ext_ioctl)(struct pld_link_device *pld, struct io_device *iod,
			unsigned int cmd, unsigned long arg);

	/* Common operations for each DPRAM */
	void (*clear_intr)(struct pld_link_device *pld);
	u16 (*recv_intr)(struct pld_link_device *pld);
	void (*send_intr)(struct pld_link_device *pld, u16 mask);
	u16 (*get_magic)(struct pld_link_device *pld);
	void (*set_magic)(struct pld_link_device *pld, u16 value);
	u16 (*get_access)(struct pld_link_device *pld);
	void (*set_access)(struct pld_link_device *pld, u16 value);
	u32 (*get_tx_head)(struct pld_link_device *pld, int id);
	u32 (*get_tx_tail)(struct pld_link_device *pld, int id);
	void (*set_tx_head)(struct pld_link_device *pld, int id, u32 head);
	void (*set_tx_tail)(struct pld_link_device *pld, int id, u32 tail);
	u8 *(*get_tx_buff)(struct pld_link_device *pld, int id);
	u32 (*get_tx_buff_size)(struct pld_link_device *pld, int id);
	u32 (*get_rx_head)(struct pld_link_device *pld, int id);
	u32 (*get_rx_tail)(struct pld_link_device *pld, int id);
	void (*set_rx_head)(struct pld_link_device *pld, int id, u32 head);
	void (*set_rx_tail)(struct pld_link_device *pld, int id, u32 tail);
	u8 *(*get_rx_buff)(struct pld_link_device *pld, int id);
	u32 (*get_rx_buff_size)(struct pld_link_device *pld, int id);
	u16 (*get_mask_req_ack)(struct pld_link_device *pld, int id);
	u16 (*get_mask_res_ack)(struct pld_link_device *pld, int id);
	u16 (*get_mask_send)(struct pld_link_device *pld, int id);

	/* Extended operations for various modems */
	struct pld_ext_op *ext_op;
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_pld_link_device(linkdev) \
		container_of(linkdev, struct pld_link_device, ld)

struct pld_ext_op {
	int exist;

	void (*init_boot_map)(struct pld_link_device *pld);
	void (*init_dl_map)(struct pld_link_device *pld);
	void (*init_ul_map)(struct pld_link_device *pld);

	void (*dload_cmd_handler)(struct pld_link_device *pld, u16 cmd);

	void (*cp_start_handler)(struct pld_link_device *pld);

	void (*crash_log)(struct pld_link_device *pld);
	int (*dump_start)(struct pld_link_device *pld);
	int (*dump_update)(struct pld_link_device *pld, void *arg);

	int (*ioctl)(struct pld_link_device *pld, struct io_device *iod,
		unsigned int cmd, unsigned long arg);

	void (*clear_intr)(struct pld_link_device *pld);
};

struct pld_ext_op *pld_get_ext_op(enum modem_t modem);
#endif
