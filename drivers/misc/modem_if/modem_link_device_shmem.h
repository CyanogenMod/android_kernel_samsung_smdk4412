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

#ifndef __MODEM_LINK_DEVICE_SHMEM_H__
#define __MODEM_LINK_DEVICE_SHMEM_H__

#include "modem_utils.h"
#include "modem_link_device_memory.h"

#define SHM_BOOT_MAGIC		0x424F4F54
#define SHM_DUMP_MAGIC		0x44554D50
#define SHM_IPC_MAGIC		0xAA
#define SHM_PM_MAGIC		0x5F

#define SHM_4M_RESERVED_SZ	4056
#define SHM_4M_FMT_TX_BUFF_SZ	4096
#define SHM_4M_FMT_RX_BUFF_SZ	4096
#define SHM_4M_RAW_TX_BUFF_SZ	2084864
#define SHM_4M_RAW_RX_BUFF_SZ	2097152

struct shmem_4mb_phys_map {
	u32 magic;
	u32 access;

	u32 fmt_tx_head;
	u32 fmt_tx_tail;

	u32 fmt_rx_head;
	u32 fmt_rx_tail;

	u32 raw_tx_head;
	u32 raw_tx_tail;

	u32 raw_rx_head;
	u32 raw_rx_tail;

	u8 reserved[SHM_4M_RESERVED_SZ];

	u8 fmt_tx_buff[SHM_4M_FMT_TX_BUFF_SZ];
	u8 fmt_rx_buff[SHM_4M_FMT_RX_BUFF_SZ];

	u8 raw_tx_buff[SHM_4M_RAW_TX_BUFF_SZ];
	u8 raw_rx_buff[SHM_4M_RAW_RX_BUFF_SZ];
} __packed;

struct shmem_circ {
	u32 __iomem *head;
	u32 __iomem *tail;
	u8  __iomem *buff;
	u32          size;
};

struct shmem_ipc_device {
	char name[16];
	int  id;

	struct shmem_circ txq;
	struct shmem_circ rxq;

	u16 mask_req_ack;
	u16 mask_res_ack;
	u16 mask_send;

	int req_ack_rcvd;
};

struct shmem_ipc_map {
	u32 __iomem *magic;
	u32 __iomem *access;

	struct shmem_ipc_device dev[MAX_SIPC5_DEV];

	u32 __iomem *mbx2ap;
	u32 __iomem *mbx2cp;
};

struct shmem_link_device {
	struct link_device ld;

	enum shmem_type type;

	/* SHMEM (SHARED MEMORY) address and size */
	u32 start;		/* physical "start" address of SHMEM */
	u32 size;		/* size of SHMEM */
	u8 __iomem *base;	/* virtual address of the "start" */

	/* SHMEM GPIO & IRQ */
	unsigned gpio_pda_active;

	unsigned gpio_ap_wakeup;
	int irq_ap_wakeup;
	unsigned gpio_ap_status;

	unsigned gpio_cp_wakeup;
	unsigned gpio_cp_status;
	int irq_cp_status;

	/* IPC device map */
	struct shmem_ipc_map ipc_map;

	/* Pointers (aliases) to IPC device map */
	u32 __iomem *magic;
	u32 __iomem *access;
	struct shmem_ipc_device *dev[MAX_SIPC5_DEV];
	u32 __iomem *mbx2ap;
	u32 __iomem *mbx2cp;

	/* Wakelock for SHMEM device */
	struct wake_lock wlock;
	char wlock_name[MIF_MAX_NAME_LEN];
	struct wake_lock ap_wlock;
	char ap_wlock_name[MIF_MAX_NAME_LEN];
	struct wake_lock cp_wlock;
	char cp_wlock_name[MIF_MAX_NAME_LEN];

	/* for UDL */
	struct completion udl_cmpl;
	struct std_dload_info dl_info;

	/* for CP crash dump */
	bool forced_cp_crash;
	struct timer_list crash_ack_timer;

	/* for locking TX process */
	spinlock_t tx_lock[MAX_SIPC5_DEV];

	/* for retransmission under SHMEM flow control after TXQ full state */
	atomic_t res_required[MAX_SIPC5_DEV];
	struct completion req_ack_cmpl[MAX_SIPC5_DEV];

	/* for efficient RX process */
	struct tasklet_struct rx_tsk;
	struct delayed_work ipc_rx_dwork;
	struct delayed_work udl_rx_dwork;
	struct io_device *iod[MAX_SIPC5_DEV];

	/* for logging SHMEM status */
	struct mem_status_queue stat_list;

	/* for logging SHMEM dump */
	struct trace_data_queue trace_list;
#ifdef DEBUG_MODEM_IF
	struct delayed_work dump_dwork;
	char dump_path[MIF_MAX_PATH_LEN];
#endif

	/* to hold/release "cp_wakeup" for PM (power-management) */
	struct delayed_work cp_sleep_dwork;
	struct delayed_work link_off_dwork;
	atomic_t ref_cnt;
	spinlock_t pm_lock;
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_shmem_link_device(linkdev) \
		container_of(linkdev, struct shmem_link_device, ld)

#if 1
#endif

/**
 * get_magic
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Returns the value of the "magic code" field.
 */
static inline u32 get_magic(struct shmem_link_device *shmd)
{
	return ioread32(shmd->magic);
}

/**
 * get_access
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Returns the value of the "access enable" field.
 */
static inline u32 get_access(struct shmem_link_device *shmd)
{
	return ioread32(shmd->access);
}

/**
 * set_magic
 * @shmd: pointer to an instance of shmem_link_device structure
 * @val: value to be written to the "magic code" field
 */
static inline void set_magic(struct shmem_link_device *shmd, u32 val)
{
	iowrite32(val, shmd->magic);
}

/**
 * set_access
 * @shmd: pointer to an instance of shmem_link_device structure
 * @val: value to be written to the "access enable" field
 */
static inline void set_access(struct shmem_link_device *shmd, u32 val)
{
	iowrite32(val, shmd->access);
}

/**
 * get_txq_head
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in a TX queue.
 */
static inline u32 get_txq_head(struct shmem_link_device *shmd, int id)
{
	return ioread32(shmd->dev[id]->txq.head);
}

/**
 * get_txq_tail
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (out) pointer in a TX queue.
 *
 * It is useless for an AP to read a tail pointer in a TX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_txq_tail(struct shmem_link_device *shmd, int id)
{
	return ioread32(shmd->dev[id]->txq.tail);
}

/**
 * get_txq_buff
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in a TXQ.
 */
static inline u8 *get_txq_buff(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->txq.buff;
}

/**
 * get_txq_buff_size
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in a TXQ.
 */
static inline u32 get_txq_buff_size(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->txq.size;
}

/**
 * get_rxq_head
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in an RX queue.
 *
 * It is useless for an AP to read a head pointer in an RX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_rxq_head(struct shmem_link_device *shmd, int id)
{
	return ioread32(shmd->dev[id]->rxq.head);
}

/**
 * get_rxq_tail
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (in) pointer in an RX queue.
 */
static inline u32 get_rxq_tail(struct shmem_link_device *shmd, int id)
{
	return ioread32(shmd->dev[id]->rxq.tail);
}

/**
 * get_rxq_buff
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in an RXQ.
 */
static inline u8 *get_rxq_buff(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->rxq.buff;
}

/**
 * get_rxq_buff_size
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in an RXQ.
 */
static inline u32 get_rxq_buff_size(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->rxq.size;
}

/**
 * set_txq_head
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in a TXQ
 */
static inline void set_txq_head(struct shmem_link_device *shmd, int id, u32 in)
{
	iowrite32(in, shmd->dev[id]->txq.head);
}

/**
 * set_txq_tail
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in a TXQ
 */
static inline void set_txq_tail(struct shmem_link_device *shmd, int id, u32 out)
{
	iowrite32(out, shmd->dev[id]->txq.tail);
}

/**
 * set_rxq_head
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in an RXQ
 */
static inline void set_rxq_head(struct shmem_link_device *shmd, int id, u32 in)
{
	iowrite32(in, shmd->dev[id]->rxq.head);
}

/**
 * set_rxq_tail
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in an RXQ
 */
static inline void set_rxq_tail(struct shmem_link_device *shmd, int id, u32 out)
{
	iowrite32(out, shmd->dev[id]->rxq.tail);
}

/**
 * get_mask_req_ack
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the REQ_ACK mask value for the IPC device.
 */
static inline u16 get_mask_req_ack(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->mask_req_ack;
}

/**
 * get_mask_res_ack
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the RES_ACK mask value for the IPC device.
 */
static inline u16 get_mask_res_ack(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->mask_res_ack;
}

/**
 * get_mask_send
 * @shmd: pointer to an instance of shmem_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the SEND mask value for the IPC device.
 */
static inline u16 get_mask_send(struct shmem_link_device *shmd, int id)
{
	return shmd->dev[id]->mask_send;
}

#ifndef CONFIG_LINK_DEVICE_C2C
/**
 * read_int2cp
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Returns the value of the AP-to-CP interrupt register.
 */
static inline u16 read_int2cp(struct shmem_link_device *shmd)
{
	if (shmd->mbx2cp)
		return ioread16(shmd->mbx2cp);
	else
		return 0;
}
#endif

/**
 * reset_txq_circ
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Empties a TXQ by resetting the head (in) pointer with the value in the tail
 * (out) pointer.
 */
static inline void reset_txq_circ(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	u32 head = get_txq_head(shmd, dev);
	u32 tail = get_txq_tail(shmd, dev);

	mif_err("%s: %s_TXQ: HEAD[%u] <== TAIL[%u]\n",
		ld->name, get_dev_name(dev), head, tail);

	set_txq_head(shmd, dev, tail);
}

/**
 * reset_rxq_circ
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Empties an RXQ by resetting the tail (out) pointer with the value in the head
 * (in) pointer.
 */
static inline void reset_rxq_circ(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	u32 head = get_rxq_head(shmd, dev);
	u32 tail = get_rxq_tail(shmd, dev);

	mif_err("%s: %s_RXQ: TAIL[%u] <== HEAD[%u]\n",
		ld->name, get_dev_name(dev), tail, head);

	set_rxq_tail(shmd, dev, head);
}

/**
 * ipc_active
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Returns whether or not IPC via the shmem_link_device instance is possible.
 */
static bool ipc_active(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	u32 magic = get_magic(shmd);
	u32 access = get_access(shmd);

	/* Check link mode */
	if (unlikely(ld->mode != LINK_MODE_IPC)) {
		mif_err("%s: <by %pf> ERR! ld->mode != LINK_MODE_IPC\n",
			ld->name, CALLER);
		return false;
	}

	/* Check "magic code" and "access enable" values */
	if (unlikely(magic != SHM_IPC_MAGIC || access != 1)) {
		mif_err("%s: <by %pf> ERR! magic:0x%X access:%d\n",
			ld->name, CALLER, magic, access);
		return false;
	}

	return true;
}

/**
 * get_rxq_rcvd
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 * OUT @circ: pointer to an instance of circ_status structure
 *
 * Stores {start address of the buffer in a RXQ, size of the buffer, in & out
 * pointer values, size of received data} into the 'circ' instance.
 *
 * Returns an error code.
 */
static int get_rxq_rcvd(struct shmem_link_device *shmd, int dev,
			struct mem_status *mst, struct circ_status *circ)
{
	struct link_device *ld = &shmd->ld;

	circ->buff = get_rxq_buff(shmd, dev);
	circ->qsize = get_rxq_buff_size(shmd, dev);
	circ->in = mst->head[dev][RX];
	circ->out = mst->tail[dev][RX];
	circ->size = circ_get_usage(circ->qsize, circ->in, circ->out);

	if (circ_valid(circ->qsize, circ->in, circ->out)) {
		mif_debug("%s: %s_RXQ qsize[%u] in[%u] out[%u] rcvd[%u]\n",
			ld->name, get_dev_name(dev), circ->qsize, circ->in,
			circ->out, circ->size);
		return 0;
	} else {
		mif_err("%s: ERR! %s_RXQ invalid (qsize[%d] in[%d] out[%d])\n",
			ld->name, get_dev_name(dev), circ->qsize, circ->in,
			circ->out);
		return -EIO;
	}
}

/**
 * get_txq_space
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * OUT @circ: pointer to an instance of circ_status structure
 *
 * Stores {start address of the buffer in a TXQ, size of the buffer, in & out
 * pointer values, size of free space} into the 'circ' instance.
 *
 * Returns the size of free space in the buffer or an error code.
 */
static int get_txq_space(struct shmem_link_device *shmd, int dev,
			struct circ_status *circ)
{
	struct link_device *ld = &shmd->ld;
	int cnt = 0;
	u32 qsize;
	u32 head;
	u32 tail;
	int space;

	while (1) {
		qsize = get_txq_buff_size(shmd, dev);
		head = get_txq_head(shmd, dev);
		tail = get_txq_tail(shmd, dev);
		space = circ_get_space(qsize, head, tail);

		mif_debug("%s: %s_TXQ{qsize:%u in:%u out:%u space:%u}\n",
			ld->name, get_dev_name(dev), qsize, head, tail, space);

		if (circ_valid(qsize, head, tail))
			break;

		cnt++;
		mif_err("%s: ERR! invalid %s_TXQ{qsize:%d in:%d out:%d "
			"space:%d}, count %d\n",
			ld->name, get_dev_name(dev), qsize, head, tail,
			space, cnt);
		if (cnt >= MAX_RETRY_CNT) {
			space = -EIO;
			break;
		}

		udelay(100);
	}

	circ->buff = get_txq_buff(shmd, dev);
	circ->qsize = qsize;
	circ->in = head;
	circ->out = tail;
	circ->size = space;

	return space;
}

/**
 * get_txq_saved
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 * OUT @circ: pointer to an instance of circ_status structure
 *
 * Stores {start address of the buffer in a TXQ, size of the buffer, in & out
 * pointer values, size of stored data} into the 'circ' instance.
 *
 * Returns an error code.
 */
static int get_txq_saved(struct shmem_link_device *shmd, int dev,
			struct circ_status *circ)
{
	struct link_device *ld = &shmd->ld;
	int cnt = 0;
	u32 qsize;
	u32 head;
	u32 tail;
	int saved;

	while (1) {
		qsize = get_txq_buff_size(shmd, dev);
		head = get_txq_head(shmd, dev);
		tail = get_txq_tail(shmd, dev);
		saved = circ_get_usage(qsize, head, tail);

		mif_debug("%s: %s_TXQ{qsize:%u in:%u out:%u saved:%u}\n",
			ld->name, get_dev_name(dev), qsize, head, tail, saved);

		if (circ_valid(qsize, head, tail))
			break;

		cnt++;
		mif_err("%s: ERR! invalid %s_TXQ{qsize:%d in:%d out:%d "
			"saved:%d}, count %d\n",
			ld->name, get_dev_name(dev), qsize, head, tail,
			saved, cnt);
		if (cnt >= MAX_RETRY_CNT) {
			saved = -EIO;
			break;
		}

		udelay(100);
	}

	circ->buff = get_txq_buff(shmd, dev);
	circ->qsize = qsize;
	circ->in = head;
	circ->out = tail;
	circ->size = saved;

	return saved;
}

/**
 * clear_shmem_map
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Clears all pointers in every circular queue.
 */
static void clear_shmem_map(struct shmem_link_device *shmd)
{
	set_txq_head(shmd, IPC_FMT, 0);
	set_txq_tail(shmd, IPC_FMT, 0);
	set_rxq_head(shmd, IPC_FMT, 0);
	set_rxq_tail(shmd, IPC_FMT, 0);

	set_txq_head(shmd, IPC_RAW, 0);
	set_txq_tail(shmd, IPC_RAW, 0);
	set_rxq_head(shmd, IPC_RAW, 0);
	set_rxq_tail(shmd, IPC_RAW, 0);
}

/**
 * reset_shmem_ipc
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Reset SHMEM with IPC map.
 */
static void reset_shmem_ipc(struct shmem_link_device *shmd)
{
	set_access(shmd, 0);

	clear_shmem_map(shmd);

	atomic_set(&shmd->res_required[IPC_FMT], 0);
	atomic_set(&shmd->res_required[IPC_RAW], 0);

	atomic_set(&shmd->ref_cnt, 0);

	set_magic(shmd, SHM_IPC_MAGIC);
	set_access(shmd, 1);
}

/**
 * init_shmem_ipc
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Initializes IPC via SHMEM.
 */
static int init_shmem_ipc(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;

	if (ld->mode == LINK_MODE_IPC &&
	    get_magic(shmd) == SHM_IPC_MAGIC &&
	    get_access(shmd) == 1) {
		mif_err("%s: IPC already initialized\n", ld->name);
		return 0;
	}

	/* Initialize variables for efficient TX/RX processing */
	shmd->iod[IPC_FMT] = link_get_iod_with_format(ld, IPC_FMT);
	shmd->iod[IPC_RAW] = link_get_iod_with_format(ld, IPC_MULTI_RAW);

	reset_shmem_ipc(shmd);

	if (get_magic(shmd) != SHM_IPC_MAGIC || get_access(shmd) != 1)
		return -EACCES;

	return 0;
}

#endif

