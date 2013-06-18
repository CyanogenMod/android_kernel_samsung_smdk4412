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
#ifndef __MODEM_LINK_DEVICE_DPRAM_H__
#define __MODEM_LINK_DEVICE_DPRAM_H__

#include "modem_link_device_memory.h"

/*
	magic_code +
	access_enable +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +
	raw_rx_head + raw_rx_tail + raw_rx_buff +
	mbx_cp2ap +
	mbx_ap2cp
 =	2 +
	2 +
	2 + 2 + 1336 +
	2 + 2 + 4564 +
	2 + 2 + 1336 +
	2 + 2 + 9124 +
	2 +
	2
 =	16384
*/
#define DP_16K_FMT_TX_BUFF_SZ	1336
#define DP_16K_RAW_TX_BUFF_SZ	4564
#define DP_16K_FMT_RX_BUFF_SZ	1336
#define DP_16K_RAW_RX_BUFF_SZ	9124

struct dpram_ipc_16k_map {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8  fmt_tx_buff[DP_16K_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8  raw_tx_buff[DP_16K_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8  fmt_rx_buff[DP_16K_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8  raw_rx_buff[DP_16K_RAW_RX_BUFF_SZ];

	u16 mbx_cp2ap;
	u16 mbx_ap2cp;
};

struct dpram_sfr {
	u16 __iomem *int2cp;
	u16 __iomem *int2ap;
	u16 __iomem *clr_int2ap;
	u16 __iomem *reset;
	u16 __iomem *msg2cp;
	u16 __iomem *msg2ap;
};

struct dpram_ext_op;

struct dpram_link_device {
	struct link_device ld;

	/* DPRAM address and size */
	enum dpram_type type;	/* DPRAM type			*/
	u8 __iomem *base;	/* Virtual address of DPRAM	*/
	u32 size;		/* DPRAM size			*/

	/* Whether or not this DPRAM can go asleep */
	bool need_wake_up;

	/* Whether or not this DPRAM needs interrupt clearing */
	bool need_intr_clear;

	/* DPRAM SFR */
	u8 __iomem *sfr_base;	/* Virtual address of SFR	*/
	struct dpram_sfr sfr;

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
	u16 __iomem *magic;
	u16 __iomem *access;
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
	spinlock_t tx_lock[MAX_IPC_DEV];

	/* For TX under DPRAM flow control */
	struct completion req_ack_cmpl[MAX_IPC_DEV];

	/* For efficient RX process */
	struct tasklet_struct rx_tsk;
	struct mif_rxb_queue rxbq[MAX_IPC_DEV];
	struct io_device *iod[MAX_IPC_DEV];
	bool rx_with_skb;

	/* For retransmission after buffer full state */
	atomic_t res_required[MAX_IPC_DEV];

	/* For wake-up/sleep control */
	atomic_t accessing;

	/* Multi-purpose miscellaneous buffer */
	u8 *buff;

	/* DPRAM IPC initialization status */
	int init_status;

	/* Alias to device-specific IOCTL function */
	int (*ext_ioctl)(struct dpram_link_device *dpld, struct io_device *iod,
			unsigned int cmd, unsigned long arg);

	/* Alias to DPRAM IRQ handler */
	irqreturn_t (*irq_handler)(int irq, void *data);

	/* For DPRAM dump */
	void (*dpram_dump)(struct link_device *ld, char *buff);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	char dump_path[MIF_MAX_PATH_LEN];
	char trace_path[MIF_MAX_PATH_LEN];
	struct trace_queue dump_list;
	struct trace_queue trace_list;
	struct delayed_work dump_dwork;
	struct delayed_work trace_dwork;
#endif

	/* Common operations for each DPRAM */
	u16 (*recv_intr)(struct dpram_link_device *dpld);
	void (*send_intr)(struct dpram_link_device *dpld, u16 mask);
	u16 (*get_magic)(struct dpram_link_device *dpld);
	void (*set_magic)(struct dpram_link_device *dpld, u16 value);
	u16 (*get_access)(struct dpram_link_device *dpld);
	void (*set_access)(struct dpram_link_device *dpld, u16 value);
	u32 (*get_tx_head)(struct dpram_link_device *dpld, int id);
	u32 (*get_tx_tail)(struct dpram_link_device *dpld, int id);
	void (*set_tx_head)(struct dpram_link_device *dpld, int id, u32 head);
	void (*set_tx_tail)(struct dpram_link_device *dpld, int id, u32 tail);
	u8 *(*get_tx_buff)(struct dpram_link_device *dpld, int id);
	u32 (*get_tx_buff_size)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_head)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_tail)(struct dpram_link_device *dpld, int id);
	void (*set_rx_head)(struct dpram_link_device *dpld, int id, u32 head);
	void (*set_rx_tail)(struct dpram_link_device *dpld, int id, u32 tail);
	u8 *(*get_rx_buff)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_buff_size)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_req_ack)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_res_ack)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_send)(struct dpram_link_device *dpld, int id);
	void (*ipc_rx_handler)(struct dpram_link_device *dpld, u16 int2ap);

	/* Extended operations for various modems */
	struct dpram_ext_op *ext_op;
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_dpram_link_device(linkdev) \
		container_of(linkdev, struct dpram_link_device, ld)

struct dpram_ext_op {
	int exist;

	void (*init_boot_map)(struct dpram_link_device *dpld);
	void (*init_dl_map)(struct dpram_link_device *dpld);
	void (*init_ul_map)(struct dpram_link_device *dpld);
	void (*init_ipc_map)(struct dpram_link_device *dpld);

	int (*download_binary)(struct dpram_link_device *dpld,
			struct sk_buff *skb);

	void (*cp_start_handler)(struct dpram_link_device *dpld);

	void (*crash_log)(struct dpram_link_device *dpld);
	int (*dump_start)(struct dpram_link_device *dpld);
	int (*dump_update)(struct dpram_link_device *dpld, void *arg);

	int (*ioctl)(struct dpram_link_device *dpld, struct io_device *iod,
			unsigned int cmd, unsigned long arg);

	irqreturn_t (*irq_handler)(int irq, void *data);
	void (*clear_intr)(struct dpram_link_device *dpld);

	int (*wakeup)(struct dpram_link_device *dpld);
	void (*sleep)(struct dpram_link_device *dpld);
};

struct dpram_ext_op *dpram_get_ext_op(enum modem_t modem);

#endif
