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

enum pld_init_status {
	PLD_INIT_STATE_NONE,
	PLD_INIT_STATE_READY,
};

#if 1
#define MAX_RXBQ_SIZE	256

struct mif_rxb {
	u8 *buff;
	unsigned size;

	u8 *data;
	unsigned len;
};

struct mif_rxb_queue {
	int size;
	int in;
	int out;
	struct mif_rxb *rxb;
};

/*
** RXB (DPRAM RX buffer) functions
*/
static inline struct mif_rxb *rxbq_create_pool(unsigned size, int count)
{
	struct mif_rxb *rxb;
	u8 *buff;
	int i;

	rxb = kzalloc(sizeof(struct mif_rxb) * count, GFP_KERNEL);
	if (!rxb) {
		mif_info("ERR! kzalloc rxb fail\n");
		return NULL;
	}

	buff = kzalloc((size * count), GFP_KERNEL|GFP_DMA);
	if (!buff) {
		mif_info("ERR! kzalloc buff fail\n");
		kfree(rxb);
		return NULL;
	}

	for (i = 0; i < count; i++) {
		rxb[i].buff = buff;
		rxb[i].size = size;
		buff += size;
	}

	return rxb;
}

static inline unsigned rxbq_get_page_size(unsigned len)
{
	return ((len + PAGE_SIZE - 1) >> PAGE_SHIFT) << PAGE_SHIFT;
}

static inline bool rxbq_empty(struct mif_rxb_queue *rxbq)
{
	return (rxbq->in == rxbq->out) ? true : false;
}

static inline int rxbq_free_size(struct mif_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline struct mif_rxb *rxbq_get_free_rxb(struct mif_rxb_queue *rxbq)
{
	struct mif_rxb *rxb = NULL;

	if (likely(rxbq_free_size(rxbq) > 0)) {
		rxb = &rxbq->rxb[rxbq->in];
		rxbq->in++;
		if (rxbq->in >= rxbq->size)
			rxbq->in -= rxbq->size;
		rxb->data = rxb->buff;
	}

	return rxb;
}

static inline int rxbq_size(struct mif_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in >= out) ? (in - out) : (qsize - out + in);
}

static inline struct mif_rxb *rxbq_get_data_rxb(struct mif_rxb_queue *rxbq)
{
	struct mif_rxb *rxb = NULL;

	if (likely(!rxbq_empty(rxbq))) {
		rxb = &rxbq->rxb[rxbq->out];
		rxbq->out++;
		if (rxbq->out >= rxbq->size)
			rxbq->out -= rxbq->size;
	}

	return rxb;
}

static inline u8 *rxb_put(struct mif_rxb *rxb, unsigned len)
{
	rxb->len = len;
	return rxb->data;
}

static inline void rxb_clear(struct mif_rxb *rxb)
{
	rxb->data = NULL;
	rxb->len = 0;
}
#endif

struct qc_dpram_boot_map {
	u8 __iomem *buff;
	u16 __iomem *frame_size;
	u16 __iomem *tag;
	u16 __iomem *count;
};

struct qc_dpram_udl_param {
	unsigned char *addr;
	unsigned int size;
	unsigned int count;
	unsigned int tag;
};

struct qc_dpram_udl_check {
	unsigned int total_size;
	unsigned int rest_size;
	unsigned int send_size;
	unsigned int copy_start;
	unsigned int copy_complete;
	unsigned int boot_complete;
};

struct pld_ext_op;

struct pld_link_device {
	struct link_device ld;

	/* DPRAM address and size */
	enum dpram_type type;	/* DPRAM type			*/
	u8 __iomem *base;	/* DPRAM base virtual address	*/
	u32 size;		/* DPRAM size			*/

	/* DPRAM IRQ GPIO# */
	unsigned gpio_ipc_int2ap;

	/* DPRAM IRQ from CP */
	int irq;
	unsigned long irq_flags;
	char irq_name[MIF_MAX_NAME_LEN];

	/* Link to DPRAM control functions dependent on each platform */
	struct modemlink_dpram_data *dpram;

	/* Physical configuration -> logical configuration */
	union {
	struct memif_boot_map bt_map;
		struct qc_dpram_boot_map qc_bt_map;
	};

	struct memif_dload_map dl_map;
	struct memif_uload_map ul_map;

	/* IPC device map */
	struct pld_ipc_map ipc_map;

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
	struct tasklet_struct dl_tsk;
	struct completion udl_start_complete;
	struct completion udl_cmd_complete;
	struct qc_dpram_udl_check qc_udl_check;
	struct qc_dpram_udl_param qc_udl_param;

	/* For CP crash dump */
	struct timer_list crash_ack_timer;
	struct completion crash_start_complete;
	struct completion crash_recv_done;

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
