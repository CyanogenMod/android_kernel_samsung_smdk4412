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

#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/suspend.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_dpram.h"

static void trigger_forced_cp_crash(struct dpram_link_device *dpld);

/**
 * set_circ_pointer
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @dir: direction of communication (TX or RX)
 * @ptr: type of the queue pointer (HEAD or TAIL)
 * @addr: address of the queue pointer
 * @val: value to be written to the queue pointer
 *
 * Writes a value to a pointer in a circular queue with verification.
 */
static inline void set_circ_pointer(struct dpram_link_device *dpld, int id,
				int dir, int ptr, void __iomem *addr, u16 val)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 0;
	u16 saved = 0;

	iowrite16(val, addr);

	while (1) {
		/* Check the value written to the address */
		saved = ioread16(addr);
		if (likely(saved == val))
			break;

		cnt++;
		mif_err("%s: ERR! %s_%s.%s saved(%d) != val(%d), count %d\n",
			ld->name, get_dev_name(id), circ_dir(dir),
			circ_ptr(ptr), saved, val, cnt);
		if (cnt >= MAX_RETRY_CNT) {
			trigger_forced_cp_crash(dpld);
			break;
		}

		udelay(100);

		/* Write the value again */
		iowrite16(val, addr);
	}
}

/**
 * recv_int2ap
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the CP-to-AP interrupt register in a DPRAM.
 */
static inline u16 recv_int2ap(struct dpram_link_device *dpld)
{
	return ioread16(dpld->mbx2ap);
}

/**
 * send_int2cp
 * @dpld: pointer to an instance of dpram_link_device structure
 * @mask: value to be written to the AP-to-CP interrupt register in a DPRAM
 */
static inline void send_int2cp(struct dpram_link_device *dpld, u16 mask)
{
	struct idpram_pm_op *pm_op = dpld->pm_op;

	if (pm_op && pm_op->int2cp_possible) {
		if (!pm_op->int2cp_possible(dpld))
			return;
	}

	iowrite16(mask, dpld->mbx2cp);
}

/**
 * read_int2cp
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the AP-to-CP interrupt register in a DPRAM.
 */
static inline u16 read_int2cp(struct dpram_link_device *dpld)
{
	return ioread16(dpld->mbx2cp);
}

/**
 * get_magic
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the "magic code" field in a DPRAM.
 */
static inline u16 get_magic(struct dpram_link_device *dpld)
{
	return ioread16(dpld->magic);
}

/**
 * set_magic
 * @dpld: pointer to an instance of dpram_link_device structure
 * @val: value to be written to the "magic code" field in a DPRAM
 */
static inline void set_magic(struct dpram_link_device *dpld, u16 val)
{
	iowrite16(val, dpld->magic);
}

/**
 * get_access
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the "access enable" field in a DPRAM.
 */
static inline u16 get_access(struct dpram_link_device *dpld)
{
	return ioread16(dpld->access);
}

/**
 * set_access
 * @dpld: pointer to an instance of dpram_link_device structure
 * @val: value to be written to the "access enable" field in a DPRAM
 */
static inline void set_access(struct dpram_link_device *dpld, u16 val)
{
	iowrite16(val, dpld->access);
}

/**
 * get_txq_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in a TX queue.
 */
static inline u32 get_txq_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.head);
}

/**
 * get_txq_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (out) pointer in a TX queue.
 *
 * It is useless for an AP to read a tail pointer in a TX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_txq_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.tail);
}

/**
 * get_txq_buff
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in a TXQ.
 */
static inline u8 *get_txq_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.buff;
}

/**
 * get_txq_buff_size
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in a TXQ.
 */
static inline u32 get_txq_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.size;
}

/**
 * get_rxq_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in an RX queue.
 *
 * It is useless for an AP to read a head pointer in an RX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_rxq_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.head);
}

/**
 * get_rxq_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (in) pointer in an RX queue.
 */
static inline u32 get_rxq_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.tail);
}

/**
 * get_rxq_buff
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in an RXQ.
 */
static inline u8 *get_rxq_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.buff;
}

/**
 * get_rxq_buff_size
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in an RXQ.
 */
static inline u32 get_rxq_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.size;
}

/**
 * set_txq_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in a TXQ
 */
static inline void set_txq_head(struct dpram_link_device *dpld, int id, u32 in)
{
	set_circ_pointer(dpld, id, TX, HEAD, dpld->dev[id]->txq.head, in);
}

/**
 * set_txq_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in a TXQ
 */
static inline void set_txq_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	set_circ_pointer(dpld, id, TX, TAIL, dpld->dev[id]->txq.tail, out);
}

/**
 * set_rxq_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in an RXQ
 */
static inline void set_rxq_head(struct dpram_link_device *dpld, int id, u32 in)
{
	set_circ_pointer(dpld, id, RX, HEAD, dpld->dev[id]->rxq.head, in);
}

/**
 * set_rxq_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in an RXQ
 */
static inline void set_rxq_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	set_circ_pointer(dpld, id, RX, TAIL, dpld->dev[id]->rxq.tail, out);
}

/**
 * get_mask_req_ack
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the REQ_ACK mask value for the IPC device.
 */
static inline u16 get_mask_req_ack(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_req_ack;
}

/**
 * get_mask_res_ack
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the RES_ACK mask value for the IPC device.
 */
static inline u16 get_mask_res_ack(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_res_ack;
}

/**
 * get_mask_send
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the SEND mask value for the IPC device.
 */
static inline u16 get_mask_send(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_send;
}

/**
 * reset_txq_circ
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Empties a TXQ by resetting the head (in) pointer with the value in the tail
 * (out) pointer.
 */
static inline void reset_txq_circ(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	u32 head = get_txq_head(dpld, dev);
	u32 tail = get_txq_tail(dpld, dev);

	mif_info("%s: %s_TXQ: HEAD[%u] <== TAIL[%u]\n",
		ld->name, get_dev_name(dev), head, tail);

	set_txq_head(dpld, dev, tail);
}

/**
 * reset_rxq_circ
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Empties an RXQ by resetting the tail (out) pointer with the value in the head
 * (in) pointer.
 */
static inline void reset_rxq_circ(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	u32 head = get_rxq_head(dpld, dev);
	u32 tail = get_rxq_tail(dpld, dev);

	mif_info("%s: %s_RXQ: TAIL[%u] <== HEAD[%u]\n",
		ld->name, get_dev_name(dev), tail, head);

	set_rxq_tail(dpld, dev, head);
}

/**
 * check_magic_access
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns 0 if the "magic code" and "access enable" values are valid, otherwise
 * returns -EACCES.
 */
static int check_magic_access(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int i;
	u16 magic = get_magic(dpld);
	u16 access = get_access(dpld);

	/* Returns 0 if the "magic code" and "access enable" are valid */
	if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
		return 0;

	/* Retry up to 100 times with 100 us delay per each retry */
	for (i = 1; i <= 100; i++) {
		mif_info("%s: magic:%X access:%X -> retry:%d\n",
			ld->name, magic, access, i);
		udelay(100);

		magic = get_magic(dpld);
		access = get_access(dpld);
		if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
			return 0;
	}

	mif_info("%s: !CRISIS! magic:%X access:%X\n", ld->name, magic, access);
	return -EACCES;
}

/**
 * ipc_active
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns whether or not IPC via the dpram_link_device instance is possible.
 */
static bool ipc_active(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	/* Check DPRAM mode */
	if (ld->mode != LINK_MODE_IPC) {
		mif_err("%s: <called by %pf> ERR! ld->mode != LINK_MODE_IPC\n",
			ld->name, CALLER);
		return false;
	}

	/* Check "magic code" and "access enable" values */
	if (check_magic_access(dpld) < 0) {
		mif_err("%s: <called by %pf> ERR! check_magic_access fail\n",
			ld->name, CALLER);
		return false;
	}

	return true;
}

/**
 * get_dpram_status
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dir: direction of communication (TX or RX)
 * @stat: pointer to an instance of mem_status structure
 *
 * Takes a snapshot of the current status of a DPRAM.
 */
static void get_dpram_status(struct dpram_link_device *dpld,
			enum circ_dir_type dir, struct mem_status *stat)
{
#ifdef DEBUG_MODEM_IF
	getnstimeofday(&stat->ts);
#endif

	stat->dir = dir;
	stat->magic = get_magic(dpld);
	stat->access = get_access(dpld);
	stat->head[IPC_FMT][TX] = get_txq_head(dpld, IPC_FMT);
	stat->tail[IPC_FMT][TX] = get_txq_tail(dpld, IPC_FMT);
	stat->head[IPC_FMT][RX] = get_rxq_head(dpld, IPC_FMT);
	stat->tail[IPC_FMT][RX] = get_rxq_tail(dpld, IPC_FMT);
	stat->head[IPC_RAW][TX] = get_txq_head(dpld, IPC_RAW);
	stat->tail[IPC_RAW][TX] = get_txq_tail(dpld, IPC_RAW);
	stat->head[IPC_RAW][RX] = get_rxq_head(dpld, IPC_RAW);
	stat->tail[IPC_RAW][RX] = get_rxq_tail(dpld, IPC_RAW);
	stat->int2ap = recv_int2ap(dpld);
	stat->int2cp = read_int2cp(dpld);
}

#if 0
/**
 * save_ipc_trace_work
 * @work: pointer to an instance of work_struct structure
 *
 * Performs actual file operation for saving RX IPC trace.
 */
static void save_ipc_trace_work(struct work_struct *work)
{
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct trace_data_queue *trq;
	struct trace_data *trd;
	struct circ_status *stat;
	struct file *fp;
	struct timespec *ts;
	int dev;
	u8 *dump;
	int rcvd;
	u8 *buff;
	char *path;
	struct utc_time utc;

	dpld = container_of(work, struct dpram_link_device, trace_dwork.work);
	ld = &dpld->ld;
	trq = &dpld->trace_list;
	path = dpld->trace_path;

	buff = kzalloc(dpld->size << 3, GFP_KERNEL);
	if (!buff) {
		while (1) {
			trd = trq_get_data_slot(trq);
			if (!trd)
				break;

			ts = &trd->ts;
			dev = trd->dev;
			stat = &trd->circ_stat;
			dump = trd->data;
			rcvd = trd->size;
			print_ipc_trace(ld, dev, stat, ts, dump, rcvd);

			kfree(dump);
		}
		return;
	}

	while (1) {
		trd = trq_get_data_slot(trq);
		if (!trd)
			break;

		ts = &trd->ts;
		dev = trd->dev;
		stat = &trd->circ_stat;
		dump = trd->data;
		rcvd = trd->size;

		ts2utc(ts, &utc);
		snprintf(path, MIF_MAX_PATH_LEN,
			"%s/%s_%s_%d%02d%02d-%02d%02d%02d.lst",
			MIF_LOG_DIR, ld->name, get_dev_name(dev),
			utc.year, utc.mon, utc.day, utc.hour, utc.min, utc.sec);

		fp = mif_open_file(path);
		if (fp) {
			int len;

			snprintf(buff, MIF_MAX_PATH_LEN,
				"[%d-%02d-%02d %02d:%02d:%02d.%03d] "
				"%s %s_RXQ {IN:%u OUT:%u LEN:%d}\n",
				utc.year, utc.mon, utc.day, utc.hour, utc.min,
				utc.sec, utc.msec, ld->name, get_dev_name(dev),
				stat->in, stat->out, stat->size);
			len = strlen(buff);
			mif_dump2format4(dump, rcvd, (buff + len), NULL);
			strcat(buff, "\n");
			len = strlen(buff);

			mif_save_file(fp, buff, len);

			memset(buff, 0, len);
			mif_close_file(fp);
		} else {
			mif_err("%s: %s open fail\n", ld->name, path);
			print_ipc_trace(ld, dev, stat, ts, dump, rcvd);
		}

		kfree(dump);
	}

	kfree(buff);
}
#endif

/**
 * set_dpram_map
 * @dpld: pointer to an instance of dpram_link_device structure
 * @map: pointer to an instance of mif_irq_map structure
 *
 * Sets variables in an mif_irq_map instance as current DPRAM status for IPC
 * logging.
 */
static void set_dpram_map(struct dpram_link_device *dpld,
			struct mif_irq_map *map)
{
	map->magic = get_magic(dpld);
	map->access = get_access(dpld);

	map->fmt_tx_in = get_txq_head(dpld, IPC_FMT);
	map->fmt_tx_out = get_txq_tail(dpld, IPC_FMT);
	map->fmt_rx_in = get_rxq_head(dpld, IPC_FMT);
	map->fmt_rx_out = get_rxq_tail(dpld, IPC_FMT);
	map->raw_tx_in = get_txq_head(dpld, IPC_RAW);
	map->raw_tx_out = get_txq_tail(dpld, IPC_RAW);
	map->raw_rx_in = get_rxq_head(dpld, IPC_RAW);
	map->raw_rx_out = get_rxq_tail(dpld, IPC_RAW);

	map->cp2ap = recv_int2ap(dpld);
}

/**
 * dpram_wake_up
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Wakes up a DPRAM if it can sleep and increases the "accessing" counter in the
 * dpram_link_device instance.
 *
 * CAUTION!!! dpram_allow_sleep() MUST be invoked after dpram_wake_up() success
 * to decrease the "accessing" counter.
 */
static int dpram_wake_up(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (unlikely(!dpld->need_wake_up))
		return 0;

	if (dpld->ext_op->wakeup(dpld) < 0) {
		mif_err("%s: <called by %pf> ERR! wakeup fail\n",
			ld->name, CALLER);
		return -EACCES;
	}

	atomic_inc(&dpld->accessing);

	return 0;
}

/**
 * dpram_allow_sleep
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Decreases the "accessing" counter in the dpram_link_device instance if it can
 * sleep and allows the DPRAM to sleep only if the value of "accessing" counter
 * is less than or equal to 0.
 *
 * MUST be invoked after dpram_wake_up() success to decrease the "accessing"
 * counter.
 */
static void dpram_allow_sleep(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (unlikely(!dpld->need_wake_up))
		return;

	if (atomic_dec_return(&dpld->accessing) <= 0) {
		dpld->ext_op->sleep(dpld);
		atomic_set(&dpld->accessing, 0);
		mif_debug("%s: DPRAM sleep possible\n", ld->name);
	}
}

static int capture_dpram_snapshot(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct sk_buff *skb;
	u32 size = dpld->size;
	u32 copied = 0;
	u8 *dump;

	dpram_wake_up(dpld);
	dump = capture_mem_dump(ld, dpld->base, dpld->size);
	dpram_allow_sleep(dpld);

	if (!dump)
		return -ENOMEM;

	while (copied < size) {
		skb = alloc_skb(MAX_DUMP_SKB_SIZE, GFP_ATOMIC);
		if (!skb) {
			mif_err("ERR! alloc_skb fail\n");
			kfree(dump);
			return -ENOMEM;
		}

		skb_put(skb, MAX_DUMP_SKB_SIZE);
		memcpy(skb->data, (dump + copied), MAX_DUMP_SKB_SIZE);
		copied += MAX_DUMP_SKB_SIZE;

		skb_queue_tail(&iod->sk_rx_q, skb);
		wake_up(&iod->wq);
	}

	kfree(dump);
	return 0;
}

/**
 * handle_cp_crash
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Actual handler for the CRASH_EXIT command from a CP.
 */
static void handle_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;
	int i;

	if (dpld->forced_cp_crash)
		dpld->forced_cp_crash = false;

	/* Stop network interfaces */
	mif_netif_stop(ld);

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	for (i = 0; i < ld->max_ipc_dev; i++)
		skb_queue_purge(ld->skb_txq[i]);

	/* Change the modem state to STATE_CRASH_EXIT for the FMT IO device */
	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	/* Change the modem state to STATE_CRASH_EXIT for the BOOT IO device */
	iod = link_get_iod_with_format(ld, IPC_BOOT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);
}

/**
 * handle_no_cp_crash_ack
 * @arg: pointer to an instance of dpram_link_device structure
 *
 * Invokes handle_cp_crash() to enter the CRASH_EXIT state if there was no
 * CRASH_ACK from a CP in FORCE_CRASH_ACK_TIMEOUT.
 */
static void handle_no_cp_crash_ack(unsigned long arg)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)arg;
	struct link_device *ld = &dpld->ld;

	mif_err("%s: ERR! No CRASH_EXIT ACK from CP\n", ld->mc->name);

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	handle_cp_crash(dpld);
}

/**
 * trigger_forced_cp_crash
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Triggers an enforced CP crash.
 */
static void trigger_forced_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
#ifdef DEBUG_MODEM_IF
	struct trace_data *trd;
	u8 *dump;
	struct timespec ts;
	getnstimeofday(&ts);
#endif

	if (ld->mode == LINK_MODE_ULOAD) {
		mif_err("%s: <called by %pf> ALREADY in progress\n",
			ld->name, CALLER);
		return;
	}

	ld->mode = LINK_MODE_ULOAD;
	dpld->forced_cp_crash = true;

	disable_irq_nosync(dpld->irq);

	dpram_wake_up(dpld);

#ifdef DEBUG_MODEM_IF
	dump = capture_mem_dump(ld, dpld->base, dpld->size);
	if (dump) {
		trd = trq_get_free_slot(&dpld->trace_list);
		memcpy(&trd->ts, &ts, sizeof(struct timespec));
		trd->dev = IPC_DEBUG;
		trd->data = dump;
		trd->size = dpld->size;
	}
#endif

	enable_irq(dpld->irq);

	mif_err("%s: <called by %pf>\n", ld->name, CALLER);

	/* Send CRASH_EXIT command to a CP */
	send_int2cp(dpld, INT_CMD(INT_CMD_CRASH_EXIT));
	get_dpram_status(dpld, TX, msq_get_free_slot(&dpld->stat_list));

	/* If there is no CRASH_ACK from a CP in FORCE_CRASH_ACK_TIMEOUT,
	   handle_no_cp_crash_ack() will be executed. */
	mif_add_timer(&dpld->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			handle_no_cp_crash_ack, (unsigned long)dpld);

	return;
}

/**
 * ext_command_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 * @cmd: extended DPRAM command from a CP
 *
 * Processes an extended command from a CP.
 */
static void ext_command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;
	u16 resp;

	switch (EXT_CMD_MASK(cmd)) {
	case EXT_CMD_SET_SPEED_LOW:
		if (dpld->dpram->setup_speed) {
			dpld->dpram->setup_speed(DPRAM_SPEED_LOW);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_LOW);
			send_int2cp(dpld, resp);
		}
		break;

	case EXT_CMD_SET_SPEED_MID:
		if (dpld->dpram->setup_speed) {
			dpld->dpram->setup_speed(DPRAM_SPEED_MID);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_MID);
			send_int2cp(dpld, resp);
		}
		break;

	case EXT_CMD_SET_SPEED_HIGH:
		if (dpld->dpram->setup_speed) {
			dpld->dpram->setup_speed(DPRAM_SPEED_HIGH);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_HIGH);
			send_int2cp(dpld, resp);
		}
		break;

	default:
		mif_info("%s: unknown command 0x%04X\n", ld->name, cmd);
		break;
	}
}

/**
 * udl_command_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 * @cmd: DPRAM upload/download command from a CP
 *
 * Processes a command for upload/download from a CP.
 */
static void udl_command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;

	if (cmd & UDL_RESULT_FAIL) {
		mif_err("%s: ERR! command fail (0x%04X)\n", ld->name, cmd);
		return;
	}

	switch (UDL_CMD_MASK(cmd)) {
	case UDL_CMD_RECV_READY:
		mif_err("%s: [CP->AP] CMD_DL_READY (0x%04X)\n", ld->name, cmd);
#ifdef CONFIG_CDMA_MODEM_CBP72
		mif_err("%s: [AP->CP] CMD_DL_START_REQ (0x%04X)\n",
			ld->name, CMD_DL_START_REQ);
		send_int2cp(dpld, CMD_DL_START_REQ);
#else
		complete(&dpld->udl_cmpl);
#endif
		break;

	default:
		complete(&dpld->udl_cmpl);
	}
}

/**
 * cmd_req_active_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Handles the REQ_ACTIVE command from a CP.
 */
static void cmd_req_active_handler(struct dpram_link_device *dpld)
{
	send_int2cp(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
}

/**
 * cmd_crash_reset_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Handles the CRASH_RESET command from a CP.
 */
static void cmd_crash_reset_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = NULL;
	int i;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	/* Stop network interfaces */
	mif_netif_stop(ld);

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	for (i = 0; i < ld->max_ipc_dev; i++)
		skb_queue_purge(ld->skb_txq[i]);

	mif_err("%s: Recv 0xC7 (CRASH_RESET)\n", ld->name);

	/* Change the modem state to STATE_CRASH_RESET for the FMT IO device */
	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);

	/* Change the modem state to STATE_CRASH_RESET for the BOOT IO device */
	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);
}

/**
 * cmd_crash_exit_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Handles the CRASH_EXIT command from a CP.
 */
static void cmd_crash_exit_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
#ifdef DEBUG_MODEM_IF
	struct trace_data *trd;
	u8 *dump;
	struct timespec ts;
	getnstimeofday(&ts);
#endif

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	del_timer(&dpld->crash_ack_timer);

	dpram_wake_up(dpld);

#ifdef DEBUG_MODEM_IF
	if (!dpld->forced_cp_crash) {
		dump = capture_mem_dump(ld, dpld->base, dpld->size);
		if (dump) {
			trd = trq_get_free_slot(&dpld->trace_list);
			memcpy(&trd->ts, &ts, sizeof(struct timespec));
			trd->dev = IPC_DEBUG;
			trd->data = dump;
			trd->size = dpld->size;
		}
	}
#endif

	if (dpld->ext_op && dpld->ext_op->crash_log)
		dpld->ext_op->crash_log(dpld);

	mif_err("%s: Recv 0xC9 (CRASH_EXIT)\n", ld->name);

	handle_cp_crash(dpld);
}

/**
 * init_dpram_ipc
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Initializes IPC via DPRAM.
 */
static int init_dpram_ipc(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int i;

	if (ld->mode == LINK_MODE_IPC &&
	    get_magic(dpld) == DPRAM_MAGIC_CODE &&
	    get_access(dpld) == 1)
		mif_info("%s: IPC already initialized\n", ld->name);

	/* Clear pointers in every circular queue */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		set_txq_head(dpld, i, 0);
		set_txq_tail(dpld, i, 0);
		set_rxq_head(dpld, i, 0);
		set_rxq_tail(dpld, i, 0);
	}

	/* Initialize variables for efficient TX/RX processing */
	for (i = 0; i < ld->max_ipc_dev; i++)
		dpld->iod[i] = link_get_iod_with_format(ld, i);
	dpld->iod[IPC_RAW] = link_get_iod_with_format(ld, IPC_MULTI_RAW);

	/* Initialize variables for TX flow control */
	for (i = 0; i < ld->max_ipc_dev; i++)
		atomic_set(&dpld->res_required[i], 0);

	/* Enable IPC */
	if (wake_lock_active(&dpld->wlock))
		wake_unlock(&dpld->wlock);

	atomic_set(&dpld->accessing, 0);

	set_magic(dpld, DPRAM_MAGIC_CODE);
	set_access(dpld, 1);
	if (get_magic(dpld) != DPRAM_MAGIC_CODE || get_access(dpld) != 1)
		return -EACCES;

	ld->mode = LINK_MODE_IPC;

	return 0;
}

/**
 * reset_dpram_ipc
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Reset DPRAM with IPC map.
 */
static void reset_dpram_ipc(struct dpram_link_device *dpld)
{
	int i;
	struct link_device *ld = &dpld->ld;

	dpld->set_access(dpld, 0);

	/* Clear pointers in every circular queue */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		dpld->set_txq_head(dpld, i, 0);
		dpld->set_txq_tail(dpld, i, 0);
		dpld->set_rxq_head(dpld, i, 0);
		dpld->set_rxq_tail(dpld, i, 0);
	}

	dpld->set_magic(dpld, DPRAM_MAGIC_CODE);
	dpld->set_access(dpld, 1);
}

/**
 * cmd_phone_start_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Handles the PHONE_START command from a CP.
 */
static void cmd_phone_start_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;

	mif_err("%s: Recv 0xC8 (CP_START)\n", ld->name);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (!iod) {
		mif_err("%s: ERR! no iod\n", ld->name);
		return;
	}

	init_dpram_ipc(dpld);

	iod->modem_state_changed(iod, STATE_ONLINE);

	if (dpld->ext_op && dpld->ext_op->cp_start_handler) {
		dpld->ext_op->cp_start_handler(dpld);
	} else {
		mif_err("%s: Send 0xC2 (INIT_END)\n", ld->name);
		send_int2cp(dpld, INT_CMD(INT_CMD_INIT_END));
	}
}

/**
 * cmd_handler: processes a DPRAM command from a CP
 * @dpld: pointer to an instance of dpram_link_device structure
 * @cmd: DPRAM command from a CP
 */
static void cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		cmd_req_active_handler(dpld);
		break;

	case INT_CMD_CRASH_RESET:
		dpld->init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_reset_handler(dpld);
		break;

	case INT_CMD_CRASH_EXIT:
		dpld->init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_exit_handler(dpld);
		break;

	case INT_CMD_PHONE_START:
		dpld->init_status = DPRAM_INIT_STATE_READY;
		cmd_phone_start_handler(dpld);
		complete_all(&ld->init_cmpl);
		break;

	case INT_CMD_NV_REBUILDING:
		mif_info("%s: NV_REBUILDING\n", ld->name);
		break;

	case INT_CMD_PIF_INIT_DONE:
		complete_all(&ld->pif_cmpl);
		break;

	case INT_CMD_SILENT_NV_REBUILDING:
		mif_info("%s: SILENT_NV_REBUILDING\n", ld->name);
		break;

	case INT_CMD_NORMAL_POWER_OFF:
		/*ToDo:*/
		/*kernel_sec_set_cp_ack()*/;
		break;

	case INT_CMD_REQ_TIME_SYNC:
	case INT_CMD_CP_DEEP_SLEEP:
	case INT_CMD_EMER_DOWN:
		break;

	default:
		mif_info("%s: unknown command 0x%04X\n", ld->name, cmd);
	}
}

/**
 * ipc_rx_work
 * @work: pointer to an instance of the work_struct structure
 *
 * Invokes the recv method in the io_device instance to perform receiving IPC
 * messages from each skb.
 */
static void ipc_rx_work(struct work_struct *work)
{
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct io_device *iod;
	struct sk_buff *skb;
	int i;

	dpld = container_of(work, struct dpram_link_device, rx_dwork.work);
	ld = &dpld->ld;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		iod = dpld->iod[i];
		while (1) {
			skb = skb_dequeue(ld->skb_rxq[i]);
			if (!skb)
				break;

			if (iod->recv_skb) {
				iod->recv_skb(iod, ld, skb);
			} else {
				iod->recv(iod, ld, skb->data, skb->len);
				dev_kfree_skb_any(skb);
			}
		}
	}
}

/**
 * get_rxq_rcvd
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 * OUT @dcst: pointer to an instance of circ_status structure
 *
 * Stores {start address of the buffer in a RXQ, size of the buffer, in & out
 * pointer values, size of received data} into the 'stat' instance.
 *
 * Returns an error code.
 */
static int get_rxq_rcvd(struct dpram_link_device *dpld, int dev,
			struct mem_status *mst, struct circ_status *dcst)
{
	struct link_device *ld = &dpld->ld;

	dcst->buff = get_rxq_buff(dpld, dev);
	dcst->qsize = get_rxq_buff_size(dpld, dev);
	dcst->in = mst->head[dev][RX];
	dcst->out = mst->tail[dev][RX];
	dcst->size = circ_get_usage(dcst->qsize, dcst->in, dcst->out);

	if (circ_valid(dcst->qsize, dcst->in, dcst->out)) {
		mif_debug("%s: %s_RXQ qsize[%u] in[%u] out[%u] rcvd[%u]\n",
			ld->name, get_dev_name(dev), dcst->qsize, dcst->in,
			dcst->out, dcst->size);
		return 0;
	} else {
		mif_err("%s: ERR! %s_RXQ invalid (qsize[%d] in[%d] out[%d])\n",
			ld->name, get_dev_name(dev), dcst->qsize, dcst->in,
			dcst->out);
		return -EIO;
	}
}

/**
 * rx_sipc4_frames
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : ILLEGAL status
 *   ret > 0  : valid data
 *
 * Must be invoked only when there is data in the corresponding RXQ.
 *
 * Requires a bottom half (e.g. ipc_rx_task) that will invoke the recv method in
 * the io_device instance.
 */
static int rx_sipc4_frames(struct dpram_link_device *dpld, int dev,
			struct mem_status *mst)
{
	struct link_device *ld = &dpld->ld;
	struct sk_buff *skb;
	u8 *dst;
	struct circ_status dcst;
	int rcvd;

	rcvd = get_rxq_rcvd(dpld, dev, mst, &dcst);
	if (unlikely(rcvd < 0)) {
#ifdef DEBUG_MODEM_IF
		trigger_forced_cp_crash(dpld);
#endif
		goto exit;
	}
	rcvd = dcst.size;

	/* Allocate an skb */
	skb = dev_alloc_skb(rcvd);
	if (!skb) {
		mif_info("%s: ERR! %s dev_alloc_skb fail\n",
			ld->name, get_dev_name(dev));
		rcvd = -ENOMEM;
		goto exit;
	}

	/* Read data from the RXQ */
	dst = skb_put(skb, rcvd);
	circ_read16_from_io(dst, dcst.buff, dcst.qsize, dcst.out, rcvd);

	/* Store the skb to the corresponding skb_rxq */
	skb_queue_tail(ld->skb_rxq[dev], skb);

exit:
	/* Update tail (out) pointer to empty out the RXQ */
	set_rxq_tail(dpld, dev, dcst.in);

	return rcvd;
}

/**
 * rx_sipc5_frames
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : ILLEGAL status
 *   ret > 0  : valid data
 *
 * Must be invoked only when there is data in the corresponding RXQ.
 *
 * Requires a recv_skb method in the io_device instance, so this function must
 * be used for only SIPC5.
 */
static int rx_sipc5_frames(struct dpram_link_device *dpld, int dev,
			struct mem_status *mst)
{
	struct link_device *ld = &dpld->ld;
	struct sk_buff *skb;
	/**
	 * variables for the status of the circular queue
	 */
	u8 __iomem *src;
	u8 hdr[SIPC5_MIN_HEADER_SIZE];
	struct circ_status dcst;
	/**
	 * variables for RX processing
	 */
	int qsize;	/* size of the queue			*/
	int rcvd;	/* size of data in the RXQ or error	*/
	int rest;	/* size of the rest data		*/
	int idx;	/* index to the start of current frame	*/
	u8 *frm;	/* pointer to current frame		*/
	u8 *dst;	/* pointer to the destination buffer	*/
	int tot;	/* total length including padding data	*/
	/**
	 * variables for debug logging
	 */
	struct mif_irq_map map;

	/* Get data size in the RXQ and in/out pointer values */
	rcvd = get_rxq_rcvd(dpld, dev, mst, &dcst);
	if (unlikely(rcvd < 0)) {
		mif_err("%s: ERR! rcvd %d < 0\n", ld->name, rcvd);
		goto exit;
	}

	rcvd = dcst.size;
	src = dcst.buff;
	qsize = dcst.qsize;
	idx = dcst.out;

	if (dev == IPC_FMT) {
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

#if 0
	skb = dev_alloc_skb(rcvd);

	/*
	** If there is enough free space for an skb to store received
	** data at once,
	*/
	if (skb) {
		/* Read all data from the RXQ to the skb */
		dst = skb_put(skb, rcvd);
		if (unlikely(dpld->strict_io_access))
			circ_read16_from_io(dst, src, qsize, idx, rcvd);
		else
			circ_read(dst, src, qsize, idx, rcvd);

#ifdef DEBUG_MODEM_IF
		/* Verify data copied to the skb */
		if (ld->aligned && memcmp16_to_io((src + idx), dst, 4)) {
			mif_err("%s: memcmp16_to_io fail\n", ld->name);
			rcvd = -EIO;
			goto exit;
		}
#endif

		/* Store the skb to the corresponding skb_rxq */
		skb_queue_tail(ld->skb_rxq[dev], skb);

		goto exit;
	}

	/*
	** If there was no enough space to store received data at once,
	*/
#endif

	rest = rcvd;
	while (rest > 0) {
		/* Calculate the start of an SIPC5 frame */
		frm = src + idx;

		/* Copy the header in the frame to the header buffer */
		if (unlikely(dpld->strict_io_access))
			memcpy16_from_io(hdr, frm, SIPC5_MIN_HEADER_SIZE);
		else
			memcpy(hdr, frm, SIPC5_MIN_HEADER_SIZE);

		/* Check the config field in the header */
		if (unlikely(!sipc5_start_valid(hdr))) {
			char str[MIF_MAX_STR_LEN];
			snprintf(str, MIF_MAX_STR_LEN, "%s: BAD CONFIG",
				ld->mc->name);
			mif_err("%s: ERR! %s INVALID config 0x%02X\n",
				ld->name, get_dev_name(dev), hdr[0]);
			pr_ipc(1, str, hdr, 4);
			rcvd = -EBADMSG;
			goto exit;
		}

		/* Verify the total length of the frame (data + padding) */
		tot = sipc5_get_total_len(hdr);
		if (unlikely(tot > rest)) {
			char str[MIF_MAX_STR_LEN];
			snprintf(str, MIF_MAX_STR_LEN, "%s: BAD LENGTH",
				ld->mc->name);
			mif_err("%s: ERR! %s tot %d > rest %d\n",
				ld->name, get_dev_name(dev), tot, rest);
			pr_ipc(1, str, hdr, 4);
			rcvd = -EBADMSG;
#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT) || defined(CONFIG_MACH_C1_KOR_LGT)
			return rcvd;
#else
			goto exit;
#endif
		}

		/* Allocate an skb */
		skb = dev_alloc_skb(tot);
		if (!skb) {
			mif_err("%s: ERR! %s dev_alloc_skb fail\n",
				ld->name, get_dev_name(dev));
			rcvd = -ENOMEM;
			goto exit;
		}

		/* Set the attribute of the skb as "single frame" */
		skbpriv(skb)->single_frame = true;

		/* Read the frame from the RXQ */
		dst = skb_put(skb, tot);
		if (unlikely(dpld->strict_io_access))
			circ_read16_from_io(dst, src, qsize, idx, tot);
		else
			circ_read(dst, src, qsize, idx, tot);

#ifdef DEBUG_MODEM_IF
		/* Take a log for debugging */
		if (unlikely(dev == IPC_FMT)) {
			size_t len = (skb->len > 32) ? 32 : skb->len;
			char str[MIF_MAX_STR_LEN];
			snprintf(str, MIF_MAX_STR_LEN, "%s: CP2MIF",
				ld->mc->name);
			pr_ipc(0, str, skb->data, len);
		}
#endif

#ifdef DEBUG_MODEM_IF
		/* Verify data copied to the skb */
		if (ld->aligned && memcmp16_to_io((src + idx), dst, 4)) {
			mif_err("%s: memcmp16_to_io fail\n", ld->name);
			rcvd = -EIO;
			goto exit;
		}
#endif

		/* Store the skb to the corresponding skb_rxq */
		skb_queue_tail(ld->skb_rxq[dev], skb);

		/* Calculate new idx value */
		rest -= tot;
		idx += tot;
		if (unlikely(idx >= qsize))
			idx -= qsize;
	}

exit:
#ifdef DEBUG_MODEM_IF
	if (rcvd < 0)
		trigger_forced_cp_crash(dpld);
#endif

	/* Update tail (out) pointer to empty out the RXQ */
	set_rxq_tail(dpld, dev, dcst.in);

	return rcvd;
}

/**
 * msg_handler: receives IPC messages from every RXQ
 * @dpld: pointer to an instance of dpram_link_device structure
 * @stat: pointer to an instance of mem_status structure
 *
 * 1) Receives all IPC message frames currently in every DPRAM RXQ.
 * 2) Sends RES_ACK responses if there are REQ_ACK requests from a CP.
 * 3) Completes all threads waiting for the corresponding RES_ACK from a CP if
 *    there is any RES_ACK response.
 */
static void msg_handler(struct dpram_link_device *dpld, struct mem_status *stat)
{
	struct link_device *ld = &dpld->ld;
	int i = 0;
	int ret = 0;
	u16 mask = 0;
	u16 intr = stat->int2ap;

	if (!ipc_active(dpld))
		return;

	/* Read data from DPRAM */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		/* Invoke an RX function only when there is data in the RXQ */
		if (unlikely(stat->head[i][RX] == stat->tail[i][RX])) {
			mif_debug("%s: %s_RXQ is empty\n",
				ld->name, get_dev_name(i));
		} else {
			if (unlikely(ld->ipc_version < SIPC_VER_50))
				ret = rx_sipc4_frames(dpld, i, stat);
			else
				ret = rx_sipc5_frames(dpld, i, stat);
			if (ret < 0)
				reset_rxq_circ(dpld, i);
		}
	}

	/* Schedule soft IRQ for RX */
	queue_delayed_work(system_nrt_wq, &dpld->rx_dwork, 0);

	/* Check and process REQ_ACK (at this time, in == out) */
	if (unlikely(intr & INT_MASK_REQ_ACK_SET)) {
		for (i = 0; i < ld->max_ipc_dev; i++) {
			if (intr & get_mask_req_ack(dpld, i)) {
				mif_debug("%s: set %s_RES_ACK\n",
					ld->name, get_dev_name(i));
				mask |= get_mask_res_ack(dpld, i);
			}
		}

		send_int2cp(dpld, INT_NON_CMD(mask));
	}

	/* Check and process RES_ACK */
	if (unlikely(intr & INT_MASK_RES_ACK_SET)) {
		for (i = 0; i < ld->max_ipc_dev; i++) {
			if (intr & get_mask_res_ack(dpld, i)) {
#ifdef DEBUG_MODEM_IF
				mif_info("%s: recv %s_RES_ACK\n",
					ld->name, get_dev_name(i));
				print_circ_status(ld, i, stat);
#endif
				complete(&dpld->req_ack_cmpl[i]);
			}
		}
	}
}

/**
 * cmd_msg_handler: processes a DPRAM command or receives IPC messages
 * @dpld: pointer to an instance of dpram_link_device structure
 * @stat: pointer to an instance of mem_status structure
 *
 * Invokes cmd_handler for a DPRAM command or msg_handler for IPC messages.
 */
static inline void cmd_msg_handler(struct dpram_link_device *dpld,
			struct mem_status *stat)
{
	struct dpram_ext_op *ext_op = dpld->ext_op;
	struct mem_status *mst = msq_get_free_slot(&dpld->stat_list);
	u16 intr = stat->int2ap;

	memcpy(mst, stat, sizeof(struct mem_status));

	if (unlikely(INT_CMD_VALID(intr))) {
		if (ext_op && ext_op->cmd_handler)
			ext_op->cmd_handler(dpld, intr);
		else
			cmd_handler(dpld, intr);
	} else {
		msg_handler(dpld, stat);
	}
}

/**
 * intr_handler: processes an interrupt from a CP
 * @dpld: pointer to an instance of dpram_link_device structure
 * @stat: pointer to an instance of mem_status structure
 *
 * Call flow for normal interrupt handling:
 *   cmd_msg_handler -> cmd_handler -> cmd_xxx_handler
 *   cmd_msg_handler -> msg_handler -> rx_sipc5_frames ->  ...
 */
static inline void intr_handler(struct dpram_link_device *dpld,
			struct mem_status *stat)
{
	char *name = dpld->ld.name;
	u16 intr = stat->int2ap;

	if (unlikely(intr == INT_POWERSAFE_FAIL)) {
		mif_info("%s: intr == INT_POWERSAFE_FAIL\n", name);
		return;
	}

	if (unlikely(EXT_UDL_CMD(intr))) {
		if (likely(EXT_INT_VALID(intr))) {
			if (UDL_CMD_VALID(intr))
				udl_command_handler(dpld, intr);
			else if (EXT_CMD_VALID(intr))
				ext_command_handler(dpld, intr);
			else
				mif_info("%s: ERR! invalid intr 0x%04X\n",
					name, intr);
		} else {
			mif_err("%s: ERR! invalid intr 0x%04X\n", name, intr);
		}

		return;
	}

	if (likely(INT_VALID(intr)))
		cmd_msg_handler(dpld, stat);
	else
		mif_err("%s: ERR! invalid intr 0x%04X\n", name, intr);
}

/**
 * ap_idpram_irq_handler: interrupt handler for an internal DPRAM in an AP
 * @irq: IRQ number
 * @data: pointer to a data
 *
 * 1) Reads the interrupt value
 * 2) Performs interrupt handling
 */
static irqreturn_t ap_idpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	struct modemlink_dpram_data *dpram = dpld->dpram;
	struct mem_status stat;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	get_dpram_status(dpld, RX, &stat);

	intr_handler(dpld, &stat);

	if (likely(dpram->clear_int2ap))
		dpram->clear_int2ap();

	return IRQ_HANDLED;
}

/**
 * cp_idpram_irq_handler: interrupt handler for an internal DPRAM in a CP
 * @irq: IRQ number
 * @data: pointer to a data
 *
 * 1) Wakes up the DPRAM
 * 2) Reads the interrupt value
 * 3) Performs interrupt handling
 * 4) Clears the interrupt port (port = memory or register)
 * 5) Allows the DPRAM to sleep
 */
static irqreturn_t cp_idpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	struct dpram_ext_op *ext_op = dpld->ext_op;
	struct mem_status stat;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE)) {
		mif_err("%s: ERR! ld->mode == LINK_MODE_OFFLINE\n", ld->name);
		get_dpram_status(dpld, RX, &stat);
#ifdef DEBUG_MODEM_IF
		print_mem_status(ld, &stat);
#endif
		return IRQ_HANDLED;
	}

	if (dpram_wake_up(dpld) < 0) {
		trigger_forced_cp_crash(dpld);
		return IRQ_HANDLED;
	}

	get_dpram_status(dpld, RX, &stat);

	intr_handler(dpld, &stat);

	if (likely(ext_op && ext_op->clear_int2ap))
		ext_op->clear_int2ap(dpld);

	dpram_allow_sleep(dpld);

	return IRQ_HANDLED;
}

/**
 * ext_dpram_irq_handler: interrupt handler for a normal external DPRAM
 * @irq: IRQ number
 * @data: pointer to a data
 *
 * 1) Reads the interrupt value
 * 2) Performs interrupt handling
 */
static irqreturn_t ext_dpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	struct mem_status stat;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	get_dpram_status(dpld, RX, &stat);

	intr_handler(dpld, &stat);

	return IRQ_HANDLED;
}

/**
 * get_txq_space
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * OUT @stat: pointer to an instance of circ_status structure
 *
 * Stores {start address of the buffer in a TXQ, size of the buffer, in & out
 * pointer values, size of free space} into the 'stat' instance.
 *
 * Returns the size of free space in the buffer or an error code.
 */
static int get_txq_space(struct dpram_link_device *dpld, int dev,
			struct circ_status *stat)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 0;
	u32 qsize;
	u32 head;
	u32 tail;
	int space;

	while (1) {
		qsize = get_txq_buff_size(dpld, dev);
		head = get_txq_head(dpld, dev);
		tail = get_txq_tail(dpld, dev);
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

	stat->buff = get_txq_buff(dpld, dev);
	stat->qsize = qsize;
	stat->in = head;
	stat->out = tail;
	stat->size = space;

	return space;
}

/**
 * write_ipc_to_txq
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @stat: pointer to an instance of circ_status structure
 * @skb: pointer to an instance of sk_buff structure
 *
 * Must be invoked only when there is enough space in the TXQ.
 */
static void write_ipc_to_txq(struct dpram_link_device *dpld, int dev,
			struct circ_status *stat, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *buff = stat->buff;
	u32 qsize = stat->qsize;
	u32 in = stat->in;
	u8 *src = skb->data;
	u32 len = skb->len;
	struct mif_irq_map map;

	/* Write data to the TXQ */
	if (unlikely(dpld->strict_io_access))
		circ_write16_to_io(buff, src, qsize, in, len);
	else
		circ_write(buff, src, qsize, in, len);

	/* Update new head (in) pointer */
	set_txq_head(dpld, dev, circ_new_pointer(qsize, in, len));

	/* Take a log for debugging */
	if (dev == IPC_FMT) {
#ifdef DEBUG_MODEM_IF
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: MIF2CP", ld->mc->name);
		pr_ipc(0, tag, src, (len > 32 ? 32 : len));
#endif
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_write", sizeof("ipc_write"));
		mif_ipc_log(MIF_IPC_AP2CP, ld->mc->msd, skb->data, skb->len);
	}

#ifdef DEBUG_MODEM_IF
	/* Verify data written to the TXQ */
	if (ld->aligned && memcmp16_to_io((buff + in), src, 4)) {
		mif_err("%s: memcmp16_to_io fail\n", ld->name);
		trigger_forced_cp_crash(dpld);
	}
#endif
}

/**
 * xmit_ipc_msg
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Tries to transmit IPC messages in the skb_txq of @dev as many as possible.
 *
 * Returns total length of IPC messages transmitted or an error code.
 */
static int xmit_ipc_msg(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	unsigned long flags;
	struct circ_status stat;
	int space;
	int copied = 0;

	/* Acquire the spin lock for a TXQ */
	spin_lock_irqsave(&dpld->tx_lock[dev], flags);

	while (1) {
		/* Get the size of free space in the TXQ */
		space = get_txq_space(dpld, dev, &stat);
		if (unlikely(space < 0)) {
#ifdef DEBUG_MODEM_IF
			/* Trigger a enforced CP crash */
			trigger_forced_cp_crash(dpld);
#endif
			/* Empty out the TXQ */
			reset_txq_circ(dpld, dev);
			copied = -EIO;
			break;
		}

		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		/* Check the free space size comparing with skb->len */
		if (unlikely(space < skb->len)) {
#ifdef DEBUG_MODEM_IF
			struct mem_status mst;
#endif
			/* Set res_required flag for the "dev" */
			atomic_set(&dpld->res_required[dev], 1);

			/* Take the skb back to the skb_txq */
			skb_queue_head(txq, skb);

			mif_info("%s: <called by %pf> NOSPC in %s_TXQ"
				"{qsize:%u in:%u out:%u}, free:%u < len:%u\n",
				ld->name, CALLER, get_dev_name(dev),
				stat.qsize, stat.in, stat.out, space, skb->len);
#ifdef DEBUG_MODEM_IF
			get_dpram_status(dpld, TX, &mst);
			print_circ_status(ld, dev, &mst);
#endif
			copied = -ENOSPC;
			break;
		}

		/* TX only when there is enough space in the TXQ */
		write_ipc_to_txq(dpld, dev, &stat, skb);
		copied += skb->len;
		dev_kfree_skb_any(skb);
	}

	/* Release the spin lock */
	spin_unlock_irqrestore(&dpld->tx_lock[dev], flags);

	return copied;
}

/**
 * wait_for_res_ack
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * 1) Sends an REQ_ACK interrupt for @dev to CP.
 * 2) Waits for the corresponding RES_ACK for @dev from CP.
 *
 * Returns the return value from wait_for_completion_interruptible_timeout().
 */
static int wait_for_res_ack(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct completion *cmpl = &dpld->req_ack_cmpl[dev];
	unsigned long timeout = msecs_to_jiffies(dpld->res_ack_wait_timeout);
	int ret;
	u16 mask;

#ifdef DEBUG_MODEM_IF
	mif_info("%s: send %s_REQ_ACK\n", ld->name, get_dev_name(dev));
#endif

	mask = get_mask_req_ack(dpld, dev);
	send_int2cp(dpld, INT_NON_CMD(mask));

	ret = wait_for_completion_interruptible_timeout(cmpl, timeout);
	/* ret == 0 on timeout, ret < 0 if interrupted */
	if (ret < 0) {
		mif_info("%s: %s: wait_for_completion interrupted! (ret %d)\n",
			ld->name, get_dev_name(dev), ret);
		goto exit;
	}

	if (ret == 0) {
		struct mem_status mst;
		get_dpram_status(dpld, TX, &mst);

		mif_info("%s: wait_for_completion TIMEOUT! (no %s_RES_ACK)\n",
			ld->name, get_dev_name(dev));

		/*
		** The TXQ must be checked whether or not it is empty, because
		** an interrupt mask can be overwritten by the next interrupt.
		*/
		if (mst.head[dev][TX] == mst.tail[dev][TX]) {
			ret = get_txq_buff_size(dpld, dev);
#ifdef DEBUG_MODEM_IF
			mif_info("%s: %s_TXQ has been emptied\n",
				ld->name, get_dev_name(dev));
			print_circ_status(ld, dev, &mst);
#endif
		}
	}

exit:
	return ret;
}

/**
 * process_res_ack
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * 1) Tries to transmit IPC messages in the skb_txq by invoking xmit_ipc_msg()
 *    function.
 * 2) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 3) Restarts DPRAM flow control if xmit_ipc_msg() returns -ENOSPC.
 *
 * Returns the return value from xmit_ipc_msg().
 */
static int process_res_ack(struct dpram_link_device *dpld, int dev)
{
	int ret;
	u16 mask;

	ret = xmit_ipc_msg(dpld, dev);
	if (ret > 0) {
		mask = get_mask_send(dpld, dev);
		send_int2cp(dpld, INT_NON_CMD(mask));
		get_dpram_status(dpld, TX, msq_get_free_slot(&dpld->stat_list));
	}

	if (ret >= 0)
		atomic_set(&dpld->res_required[dev], 0);

	return ret;
}

/**
 * fmt_tx_work: performs TX for FMT IPC device under DPRAM flow control
 * @work: pointer to an instance of the work_struct structure
 *
 * 1) Starts waiting for RES_ACK of FMT IPC device.
 * 2) Returns immediately if the wait is interrupted.
 * 3) Restarts DPRAM flow control if there is a timeout from the wait.
 * 4) Otherwise, it performs processing RES_ACK for FMT IPC device.
 */
static void fmt_tx_work(struct work_struct *work)
{
	struct link_device *ld;
	struct dpram_link_device *dpld;
	unsigned long delay = 0;
	int ret;

	ld = container_of(work, struct link_device, fmt_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_FMT);
	/* ret < 0 if interrupted */
	if (ret < 0)
		return;

	/* ret == 0 on timeout */
	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_FMT], 0);
		return;
	}

	ret = process_res_ack(dpld, IPC_FMT);
	if (ret >= 0) {
		dpram_allow_sleep(dpld);
		return;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC)
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_FMT], delay);
}

/**
 * raw_tx_work: performs TX for RAW IPC device under DPRAM flow control.
 * @work: pointer to an instance of the work_struct structure
 *
 * 1) Starts waiting for RES_ACK of RAW IPC device.
 * 2) Returns immediately if the wait is interrupted.
 * 3) Restarts DPRAM flow control if there is a timeout from the wait.
 * 4) Otherwise, it performs processing RES_ACK for RAW IPC device.
 */
static void raw_tx_work(struct work_struct *work)
{
	struct link_device *ld;
	struct dpram_link_device *dpld;
	unsigned long delay = 0;
	int ret;

	ld = container_of(work, struct link_device, raw_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_RAW);
	/* ret < 0 if interrupted */
	if (ret < 0)
		return;

	/* ret == 0 on timeout */
	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RAW], 0);
		return;
	}

	ret = process_res_ack(dpld, IPC_RAW);
	if (ret >= 0) {
		dpram_allow_sleep(dpld);
		mif_netif_wake(ld);
		return;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC)
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RAW], delay);
}

/**
 * rfs_tx_work: performs TX for RFS IPC device under DPRAM flow control
 * @work: pointer to an instance of the work_struct structure
 *
 * 1) Starts waiting for RES_ACK of RFS IPC device.
 * 2) Returns immediately if the wait is interrupted.
 * 3) Restarts DPRAM flow control if there is a timeout from the wait.
 * 4) Otherwise, it performs processing RES_ACK for RFS IPC device.
 */
static void rfs_tx_work(struct work_struct *work)
{
	struct link_device *ld;
	struct dpram_link_device *dpld;
	unsigned long delay = 0;
	int ret;

	ld = container_of(work, struct link_device, rfs_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_RFS);
	/* ret < 0 if interrupted */
	if (ret < 0)
		return;

	/* ret == 0 on timeout */
	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RFS], 0);
		return;
	}

	ret = process_res_ack(dpld, IPC_RFS);
	if (ret >= 0) {
		dpram_allow_sleep(dpld);
		return;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC)
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RFS], delay);
}

/**
 * dpram_send_ipc
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * 1) Tries to transmit IPC messages in the skb_txq by invoking xmit_ipc_msg()
 *    function.
 * 2) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 3) Starts DPRAM flow control if xmit_ipc_msg() returns -ENOSPC.
 */
static int dpram_send_ipc(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	u16 mask;

	if (atomic_read(&dpld->res_required[dev]) > 0) {
		mif_info("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		return 0;
	}

	if (dpram_wake_up(dpld) < 0) {
		trigger_forced_cp_crash(dpld);
		return -EIO;
	}

	if (!ipc_active(dpld)) {
		mif_info("%s: IPC is NOT active\n", ld->name);
		ret = -EIO;
		goto exit;
	}

	ret = xmit_ipc_msg(dpld, dev);
	if (likely(ret > 0)) {
		mask = get_mask_send(dpld, dev);
		send_int2cp(dpld, INT_NON_CMD(mask));
		get_dpram_status(dpld, TX, msq_get_free_slot(&dpld->stat_list));
		goto exit;
	}

	/* If there was no TX, just exit */
	if (ret == 0)
		goto exit;

	/* At this point, ret < 0 */
	if (ret == -ENOSPC) {
		/* Prohibit DPRAM from sleeping until the TXQ buffer is empty */
		if (dpram_wake_up(dpld) < 0) {
			trigger_forced_cp_crash(dpld);
			goto exit;
		}

		/*----------------------------------------------------*/
		/* dpld->res_required[dev] was set in xmit_ipc_msg(). */
		/*----------------------------------------------------*/

		if (dev == IPC_RAW)
			mif_netif_stop(ld);

		queue_delayed_work(ld->tx_wq, ld->tx_dwork[dev], 0);
	}

exit:
	dpram_allow_sleep(dpld);
	return ret;
}

/**
 * pm_tx_work: performs TX while DPRAM PM is locked
 * @work: pointer to an instance of the work_struct structure
 */
static void pm_tx_work(struct work_struct *work)
{
	struct idpram_pm_data *pm_data;
	struct idpram_pm_op *pm_op;
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct workqueue_struct *pm_wq = system_nrt_wq;
	int i;
	int ret;
	unsigned long delay = 0;

	pm_data = container_of(work, struct idpram_pm_data, tx_dwork.work);
	dpld = container_of(pm_data, struct dpram_link_device, pm_data);
	ld = &dpld->ld;
	pm_op = dpld->pm_op;

	if (pm_op->locked(dpld)) {
		queue_delayed_work(pm_wq, &pm_data->tx_dwork, delay);
		return;
	}

	/* Here, PM is not locked. */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		ret = dpram_send_ipc(dpld, i);
		if (ret < 0) {
			struct io_device *iod = dpld->iod[i];
			mif_err("%s->%s: ERR! dpram_send_ipc fail (err %d)\n",
				iod->name, ld->name, ret);
		}
	}
}

/**
 * dpram_try_send_ipc
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @iod: pointer to an instance of the io_device structure
 * @skb: pointer to an skb that will be transmitted
 *
 * 1) Enqueues an skb to the skb_txq for @dev in the link device instance.
 * 2) Tries to transmit IPC messages in the skb_txq by invoking xmit_ipc_msg()
 *    function.
 * 3) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 4) Starts DPRAM flow control if xmit_ipc_msg() returns -ENOSPC.
 */
static void dpram_try_send_ipc(struct dpram_link_device *dpld, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	struct idpram_pm_op *pm_op = dpld->pm_op;
	struct workqueue_struct *pm_wq = system_nrt_wq;
	unsigned long delay = msecs_to_jiffies(10);
	struct sk_buff_head *txq = ld->skb_txq[dev];
	int ret;

	if (unlikely(txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_info("%s: %s txq->qlen %d >= %d\n", ld->name,
			get_dev_name(dev), txq->qlen, MAX_SKB_TXQ_DEPTH);
		dev_kfree_skb_any(skb);
		return;
	}

	skb_queue_tail(txq, skb);

	if (pm_op && pm_op->locked) {
		if (pm_op->locked(dpld)) {
			queue_delayed_work(pm_wq, &pm_data->tx_dwork, delay);
			return;
		}

		/* Here, PM is not locked. */
		if (work_pending(&pm_data->tx_dwork.work))
			cancel_delayed_work_sync(&pm_data->tx_dwork);
	}

	ret = dpram_send_ipc(dpld, dev);
	if (ret < 0) {
		mif_err("%s->%s: ERR! dpram_send_ipc fail (err %d)\n",
			iod->name, ld->name, ret);
	}
}

static int dpram_send_cp_binary(struct link_device *ld, struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->xmit_binary)
		return dpld->ext_op->xmit_binary(dpld, skb);
	else
		return -ENODEV;
}

/**
 * dpram_send
 * @ld: pointer to an instance of the link_device structure
 * @iod: pointer to an instance of the io_device structure
 * @skb: pointer to an skb that will be transmitted
 *
 * Returns the length of data transmitted or an error code.
 *
 * Normal call flow for an IPC message:
 *   dpram_try_send_ipc -> dpram_send_ipc -> xmit_ipc_msg -> write_ipc_to_txq
 *
 * Call flow on PM lock in a DPRAM IPC TXQ:
 *   dpram_try_send_ipc ,,, queue_delayed_work
 *   => pm_tx_work -> dpram_send_ipc -> xmit_ipc_msg -> write_ipc_to_txq
 *
 * Call flow on congestion in a DPRAM IPC TXQ:
 *   dpram_try_send_ipc -> xmit_ipc_msg ,,, queue_delayed_work
 *   => xxx_tx_work -> wait_for_res_ack
 *   => msg_handler
 *   => process_res_ack -> xmit_ipc_msg (,,, queue_delayed_work ...)
 */
static int dpram_send(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	int dev = iod->format;
	int len = skb->len;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		if (likely(ld->mode == LINK_MODE_IPC)) {
			dpram_try_send_ipc(dpld, dev, iod, skb);
		} else {
			mif_info("%s: ld->mode != LINK_MODE_IPC\n", ld->name);
			dev_kfree_skb_any(skb);
		}
		return len;

	case IPC_BOOT:
		return dpram_send_cp_binary(ld, skb);

	default:
		mif_info("%s: ERR! no TXQ for %s\n", ld->name, iod->name);
		dev_kfree_skb_any(skb);
		return -ENODEV;
	}
}

static int dpram_xmit_boot(struct link_device *ld, struct io_device *iod,
			unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->xmit_boot)
		return dpld->ext_op->xmit_boot(dpld, arg);
	else
		return -ENODEV;
}

static int dpram_set_dload_magic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	ld->mode = LINK_MODE_DLOAD;

	mif_err("%s: magic = 0x%08X\n", ld->name, DP_MAGIC_DMDL);
	iowrite32(DP_MAGIC_DMDL, dpld->dl_map.magic);

	return 0;
}

static int dpram_dload_firmware(struct link_device *ld, struct io_device *iod,
			unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->firm_update)
		return dpld->ext_op->firm_update(dpld, arg);
	else
		return -ENODEV;
}

static int dpram_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	trigger_forced_cp_crash(dpld);
	return 0;
}

static int dpram_dump_start(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->dump_start)
		return dpld->ext_op->dump_start(dpld);
	else
		return -ENODEV;
}

static int dpram_dump_update(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->dump_update)
		return dpld->ext_op->dump_update(dpld, arg);
	else
		return -ENODEV;
}

static int dpram_dump_finish(struct link_device *ld, struct io_device *iod,
			unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->dump_finish)
		return dpld->ext_op->dump_finish(dpld, arg);
	else
		return -ENODEV;
}

static int dpram_ioctl(struct link_device *ld, struct io_device *iod,
			unsigned int cmd, unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	int err = 0;

	mif_info("%s: cmd 0x%08X\n", ld->name, cmd);

	switch (cmd) {
	case IOCTL_DPRAM_INIT_STATUS:
		mif_debug("%s: get dpram init status\n", ld->name);
		return dpld->init_status;

	case IOCTL_MIF_DPRAM_DUMP:
		if (copy_to_user((void __user *)arg, &dpld->size, sizeof(u32)))
			return -EFAULT;

		capture_dpram_snapshot(ld, iod);
		break;

	default:
		if (dpld->ext_ioctl)
			return dpld->ext_ioctl(dpld, iod, cmd, arg);

		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		return -EINVAL;
	}

	return err;
}

static void dpram_remap_std_16k_region(struct dpram_link_device *dpld)
{
	struct dpram_ipc_16k_map *dpram_map;
	struct dpram_ipc_device *dev;

	dpram_map = (struct dpram_ipc_16k_map *)dpld->base;

	/* "magic code" and "access enable" fields */
	dpld->ipc_map.magic = (u16 __iomem *)&dpram_map->magic;
	dpld->ipc_map.access = (u16 __iomem *)&dpram_map->access;

	/* FMT */
	dev = &dpld->ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)&dpram_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *)&dpram_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&dpram_map->fmt_tx_buff[0];
	dev->txq.size = DP_16K_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&dpram_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *)&dpram_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&dpram_map->fmt_rx_buff[0];
	dev->rxq.size = DP_16K_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send    = INT_MASK_SEND_F;

	/* RAW */
	dev = &dpld->ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)&dpram_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *)&dpram_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&dpram_map->raw_tx_buff[0];
	dev->txq.size = DP_16K_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&dpram_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *)&dpram_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&dpram_map->raw_rx_buff[0];
	dev->rxq.size = DP_16K_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send    = INT_MASK_SEND_R;

	/* interrupt ports */
	dpld->ipc_map.mbx_cp2ap = (u16 __iomem *)&dpram_map->mbx_cp2ap;
	dpld->ipc_map.mbx_ap2cp = (u16 __iomem *)&dpram_map->mbx_ap2cp;
}

static int dpram_init_boot_map(struct dpram_link_device *dpld)
{
	u8 __iomem *dp_base = dpld->base;
	u32 magic_size = DP_DLOAD_MAGIC_SIZE;
	u32 mbx_size = DP_MBX_SET_SIZE;

	if (dpld->ext_op && dpld->ext_op->init_boot_map) {
		dpld->ext_op->init_boot_map(dpld);
	} else {
		dpld->bt_map.magic = (u32 *)(dp_base);
		dpld->bt_map.buff = (u8 *)(dp_base + magic_size);
		dpld->bt_map.space = dpld->size - (magic_size + mbx_size);
	}

	return 0;
}

static int dpram_init_dload_map(struct dpram_link_device *dpld)
{
	u8 __iomem *dp_base = dpld->base;
	u32 magic_size = DP_DLOAD_MAGIC_SIZE;
	u32 mbx_size = DP_MBX_SET_SIZE;

	if (dpld->ext_op && dpld->ext_op->init_dl_map) {
		dpld->ext_op->init_dl_map(dpld);
	} else {
		dpld->dl_map.magic = (u32 *)(dp_base);
		dpld->dl_map.buff = (u8 *)(dp_base + magic_size);
		dpld->dl_map.space = dpld->size - (magic_size + mbx_size);
	}

	return 0;
}

static int dpram_init_uload_map(struct dpram_link_device *dpld)
{
	u8 __iomem *dp_base = dpld->base;
	u32 magic_size = DP_DLOAD_MAGIC_SIZE;
	u32 mbx_size = DP_MBX_SET_SIZE;

	if (dpld->ext_op && dpld->ext_op->init_ul_map) {
		dpld->ext_op->init_ul_map(dpld);
	} else {
		dpld->ul_map.magic = (u32 *)(dp_base);
		dpld->ul_map.buff = (u8 *)(dp_base + DP_ULOAD_BUFF_OFFSET);
		dpld->ul_map.space = dpld->size - (magic_size + mbx_size);
	}

	return 0;
}

static int dpram_init_ipc_map(struct dpram_link_device *dpld)
{
	int i;
	struct link_device *ld = &dpld->ld;

	if (dpld->ext_op && dpld->ext_op->init_ipc_map) {
		dpld->ext_op->init_ipc_map(dpld);
	} else if (dpld->dpram->ipc_map) {
		memcpy(&dpld->ipc_map, dpld->dpram->ipc_map,
			sizeof(struct dpram_ipc_map));
	} else {
		if (dpld->size == DPRAM_SIZE_16KB)
			dpram_remap_std_16k_region(dpld);
		else
			return -EINVAL;
	}

	dpld->magic = dpld->ipc_map.magic;
	dpld->access = dpld->ipc_map.access;
	for (i = 0; i < ld->max_ipc_dev; i++)
		dpld->dev[i] = &dpld->ipc_map.dev[i];
	dpld->mbx2ap = dpld->ipc_map.mbx_cp2ap;
	dpld->mbx2cp = dpld->ipc_map.mbx_ap2cp;

	return 0;
}

static void dpram_setup_common_op(struct dpram_link_device *dpld)
{
	dpld->recv_intr = recv_int2ap;
	dpld->send_intr = send_int2cp;
	dpld->get_magic = get_magic;
	dpld->set_magic = set_magic;
	dpld->get_access = get_access;
	dpld->set_access = set_access;
	dpld->get_txq_head = get_txq_head;
	dpld->get_txq_tail = get_txq_tail;
	dpld->set_txq_head = set_txq_head;
	dpld->set_txq_tail = set_txq_tail;
	dpld->get_txq_buff = get_txq_buff;
	dpld->get_txq_buff_size = get_txq_buff_size;
	dpld->get_rxq_head = get_rxq_head;
	dpld->get_rxq_tail = get_rxq_tail;
	dpld->set_rxq_head = set_rxq_head;
	dpld->set_rxq_tail = set_rxq_tail;
	dpld->get_rxq_buff = get_rxq_buff;
	dpld->get_rxq_buff_size = get_rxq_buff_size;
	dpld->get_mask_req_ack = get_mask_req_ack;
	dpld->get_mask_res_ack = get_mask_res_ack;
	dpld->get_mask_send = get_mask_send;
	dpld->get_dpram_status = get_dpram_status;
	dpld->ipc_rx_handler = cmd_msg_handler;
	dpld->reset_dpram_ipc = reset_dpram_ipc;
}

static void dpram_link_terminate(struct link_device *ld, struct io_device *iod)
{
	if (iod->format == IPC_FMT && ld->mode == LINK_MODE_IPC) {
		if (!atomic_read(&iod->opened)) {
			ld->mode = LINK_MODE_OFFLINE;
			mif_err("%s: %s: link mode is changed: IPC->OFFLINE\n",
				iod->name, ld->name);
		}
	}

	return;
}

struct link_device *dpram_create_link_device(struct platform_device *pdev)
{
	struct dpram_link_device *dpld = NULL;
	struct link_device *ld = NULL;
	struct modem_data *modem = NULL;
	struct modemlink_dpram_data *dpram = NULL;
	struct resource *res = NULL;
	resource_size_t res_size;
	int ret = 0;
	int i = 0;

	/*
	** Alloc an instance of dpram_link_device structure
	*/
	dpld = kzalloc(sizeof(struct dpram_link_device), GFP_KERNEL);
	if (!dpld) {
		mif_err("ERR! kzalloc dpld fail\n");
		goto err;
	}
	ld = &dpld->ld;

	/*
	** Get the modem (platform) data
	*/
	modem = (struct modem_data *)pdev->dev.platform_data;
	if (!modem) {
		mif_err("ERR! modem == NULL\n");
		goto err;
	}
	mif_info("modem = %s\n", modem->name);
	mif_info("link device = %s\n", modem->link_name);

	/*
	** Retrieve modem data and DPRAM control data from the modem data
	*/
	ld->mdm_data = modem;
	ld->name = modem->link_name;
	ld->ipc_version = modem->ipc_version;

	if (!modem->dpram) {
		mif_err("ERR! no modem->dpram\n");
		goto err;
	}
	dpram = modem->dpram;

	dpld->dpram = dpram;
	dpld->type = dpram->type;
	dpld->ap = dpram->ap;
	dpld->strict_io_access = dpram->strict_io_access;

	if (ld->ipc_version < SIPC_VER_50) {
		if (!modem->max_ipc_dev) {
			mif_err("%s: ERR! no max_ipc_dev\n", ld->name);
			goto err;
		}

		ld->aligned = dpram->aligned;
		ld->max_ipc_dev = modem->max_ipc_dev;
	} else {
		ld->aligned = 1;
		ld->max_ipc_dev = MAX_SIPC5_DEV;
	}

	/*
	** Set attributes as a link device
	*/
	ld->terminate_comm = dpram_link_terminate;
	ld->send = dpram_send;
	ld->xmit_boot = dpram_xmit_boot;
	ld->dload_start = dpram_set_dload_magic;
	ld->firm_update = dpram_dload_firmware;
	ld->force_dump = dpram_force_dump;
	ld->dump_start = dpram_dump_start;
	ld->dump_update = dpram_dump_update;
	ld->dump_finish = dpram_dump_finish;
	/* IOCTL extension */
	ld->ioctl = dpram_ioctl;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	skb_queue_head_init(&ld->sk_rfs_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;
	ld->skb_txq[IPC_RFS] = &ld->sk_rfs_tx_q;

	skb_queue_head_init(&ld->sk_fmt_rx_q);
	skb_queue_head_init(&ld->sk_raw_rx_q);
	skb_queue_head_init(&ld->sk_rfs_rx_q);
	ld->skb_rxq[IPC_FMT] = &ld->sk_fmt_rx_q;
	ld->skb_rxq[IPC_RAW] = &ld->sk_raw_rx_q;
	ld->skb_rxq[IPC_RFS] = &ld->sk_rfs_rx_q;

	init_completion(&ld->init_cmpl);
	init_completion(&ld->pif_cmpl);

	/*
	** Set up function pointers
	*/
	dpram_setup_common_op(dpld);
	dpld->ext_op = dpram_get_ext_op(modem->modem_type);
	if (dpld->ext_op) {
		if (dpld->ext_op->ioctl)
			dpld->ext_ioctl = dpld->ext_op->ioctl;

		if (dpld->ext_op->wakeup && dpld->ext_op->sleep)
			dpld->need_wake_up = true;
	}

	/*
	** Retrieve DPRAM resource
	*/
	if (!dpram->base) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						STR_DPRAM_BASE);
		if (!res) {
			mif_err("%s: ERR! no DPRAM resource\n", ld->name);
			goto err;
		}
		res_size = resource_size(res);

		dpram->base = ioremap_nocache(res->start, res_size);
		if (!dpram->base) {
			mif_err("%s: ERR! ioremap_nocache for BASE fail\n",
				ld->name);
			goto err;
		}
		dpram->size = res_size;
	}
	dpld->base = dpram->base;
	dpld->size = dpram->size;

	mif_info("%s: type %d, aligned %d, base 0x%08X, size %d\n",
		ld->name, dpld->type, ld->aligned, (int)dpld->base, dpld->size);

	/*
	** Retrieve DPRAM SFR resource if exists
	*/
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					STR_DPRAM_SFR_BASE);
	if (res) {
		res_size = resource_size(res);
		dpld->sfr_base = ioremap_nocache(res->start, res_size);
		if (!dpld->sfr_base) {
			mif_err("%s: ERR! ioremap_nocache for SFR fail\n",
				ld->name);
			goto err;
		}
	}

	/*
	** Initialize DPRAM maps (physical map -> logical map)
	*/
	ret = dpram_init_boot_map(dpld);
	if (ret < 0) {
		mif_err("%s: ERR! dpram_init_boot_map fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	ret = dpram_init_dload_map(dpld);
	if (ret < 0) {
		mif_err("%s: ERR! dpram_init_dload_map fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	ret = dpram_init_uload_map(dpld);
	if (ret < 0) {
		mif_err("%s: ERR! dpram_init_uload_map fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	ret = dpram_init_ipc_map(dpld);
	if (ret < 0) {
		mif_err("%s: ERR! dpram_init_ipc_map fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	if (dpram->res_ack_wait_timeout > 0)
		dpld->res_ack_wait_timeout = dpram->res_ack_wait_timeout;
	else
		dpld->res_ack_wait_timeout = RES_ACK_WAIT_TIMEOUT;

	/* Disable IPC */
	if (!dpram->disabled) {
		set_magic(dpld, 0);
		set_access(dpld, 0);
	}
	dpld->init_status = DPRAM_INIT_STATE_NONE;

	/*
	** Initialize locks, completions, and bottom halves
	*/
	snprintf(dpld->wlock_name, MIF_MAX_NAME_LEN, "%s_wlock", ld->name);
	wake_lock_init(&dpld->wlock, WAKE_LOCK_SUSPEND, dpld->wlock_name);

	init_completion(&dpld->udl_cmpl);
	init_completion(&dpld->crash_cmpl);

	for (i = 0; i < ld->max_ipc_dev; i++)
		init_completion(&dpld->req_ack_cmpl[i]);

	INIT_DELAYED_WORK(&dpld->rx_dwork, ipc_rx_work);

	for (i = 0; i < ld->max_ipc_dev; i++) {
		spin_lock_init(&dpld->tx_lock[i]);
		atomic_set(&dpld->res_required[i], 0);
	}

	ld->tx_wq = create_singlethread_workqueue("dpram_tx_wq");
	if (!ld->tx_wq) {
		mif_err("%s: ERR! fail to create tx_wq\n", ld->name);
		goto err;
	}
	INIT_DELAYED_WORK(&ld->fmt_tx_dwork, fmt_tx_work);
	INIT_DELAYED_WORK(&ld->raw_tx_dwork, raw_tx_work);
	INIT_DELAYED_WORK(&ld->rfs_tx_dwork, rfs_tx_work);
	ld->tx_dwork[IPC_FMT] = &ld->fmt_tx_dwork;
	ld->tx_dwork[IPC_RAW] = &ld->raw_tx_dwork;
	ld->tx_dwork[IPC_RFS] = &ld->rfs_tx_dwork;

#ifdef DEBUG_MODEM_IF
	spin_lock_init(&dpld->stat_list.lock);
	spin_lock_init(&dpld->trace_list.lock);
#endif

	/* Prepare a multi-purpose miscellaneous buffer */
	dpld->buff = kzalloc(dpld->size, GFP_KERNEL);
	if (!dpld->buff) {
		mif_err("%s: ERR! kzalloc dpld->buff fail\n", ld->name);
		goto err;
	}

	/*
	** Retrieve DPRAM IRQ GPIO#, IRQ#, and IRQ flags
	*/
	dpld->gpio_int2ap = modem->gpio_ipc_int2ap;
	dpld->gpio_cp_status = modem->gpio_cp_status;
	dpld->gpio_cp_wakeup = modem->gpio_cp_wakeup;
	if (dpram->type == AP_IDPRAM) {
		if (!modem->gpio_ipc_int2cp) {
			mif_err("%s: ERR! no gpio_ipc_int2cp\n", ld->name);
			goto err;
		}
		dpld->gpio_int2cp = modem->gpio_ipc_int2cp;
	}

	dpld->irq = modem->irq_ipc_int2ap;

	if (modem->irqf_ipc_int2ap)
		dpld->irq_flags = modem->irqf_ipc_int2ap;
	else
		dpld->irq_flags = (IRQF_NO_SUSPEND | IRQF_TRIGGER_LOW);

	/*
	** Initialize power management (PM) for AP_IDPRAM
	*/
	if (dpld->type == AP_IDPRAM) {
		dpld->pm_op = idpram_get_pm_op(dpld->ap);
		if (!dpld->pm_op) {
			mif_err("%s: no pm_op for AP_IDPRAM\n", ld->name);
			goto err;
		}

		ret = dpld->pm_op->pm_init(dpld, modem, pm_tx_work);
		if (ret) {
			mif_err("%s: pm_init fail (err %d)\n", ld->name, ret);
			goto err;
		}
	}

	/*
	** Register DPRAM interrupt handler
	*/
	snprintf(dpld->irq_name, MIF_MAX_NAME_LEN, "%s_irq", ld->name);
	if (dpld->ext_op && dpld->ext_op->irq_handler)
		dpld->irq_handler = dpld->ext_op->irq_handler;
	else if (dpld->type == CP_IDPRAM)
		dpld->irq_handler = cp_idpram_irq_handler;
	else if (dpld->type == AP_IDPRAM)
		dpld->irq_handler = ap_idpram_irq_handler;
	else
		dpld->irq_handler = ext_dpram_irq_handler;

	ret = mif_register_isr(dpld->irq, dpld->irq_handler, dpld->irq_flags,
				dpld->irq_name, dpld);
	if (ret)
		goto err;

	return ld;

err:
	if (dpld) {
		kfree(dpld->buff);
		kfree(dpld);
	}

	return NULL;
}

