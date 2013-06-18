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
#include <linux/platform_data/modem.h>

#include "modem_prj.h"
#include "modem_link_device_dpram.h"
#include "modem_utils.h"

static void handle_cp_crash(struct dpram_link_device *dpld);
static void trigger_force_cp_crash(struct dpram_link_device *dpld);

/**
 * set_circ_pointer
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @dir: direction of communication (TX or RX)
 * @ptr: type of the queue pointer (HEAD or TAIL)
 * @addr: address of the queue pointer
 * @val: value to be written to the queue pointer
 *
 * Writes a value to a pointer in a circular queue with verification
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
			trigger_force_cp_crash(dpld);
			break;
		}

		udelay(100);

		/* Write the value again */
		iowrite16(val, addr);
	}
}

/**
 * register_isr
 * @irq: IRQ number for a DPRAM interrupt
 * @isr: function pointer to an interrupt service routine
 * @flags: set of interrupt flags
 * @name: name of the interrupt
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Registers the ISR for the IRQ number
 */
static int register_isr(unsigned int irq, irqreturn_t (*isr)(int, void*),
			unsigned long flags, const char *name,
			struct dpram_link_device *dpld)
{
	int ret;

	ret = request_irq(irq, isr, flags, name, dpld);
	if (ret) {
		mif_info("%s: ERR! request_irq fail (err %d)\n", name, ret);
		return ret;
	}

	ret = enable_irq_wake(irq);
	if (ret)
		mif_info("%s: ERR! enable_irq_wake fail (err %d)\n", name, ret);

	mif_info("%s (#%d) handler registered\n", name, irq);

	return 0;
}

/**
 * clear_intr
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Clears the CP-to-AP interrupt register in a DPRAM
 */
static inline void clear_intr(struct dpram_link_device *dpld)
{
	if (likely(dpld->need_intr_clear))
		dpld->ext_op->clear_intr(dpld);
}

/**
 * recv_intr
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the CP-to-AP interrupt register in a DPRAM
 */
static inline u16 recv_intr(struct dpram_link_device *dpld)
{
	return ioread16(dpld->mbx2ap);
}

/**
 * send_intr
 * @dpld: pointer to an instance of dpram_link_device structure
 * @mask: value to be written to the AP-to-CP interrupt register in a DPRAM
 */
static inline void send_intr(struct dpram_link_device *dpld, u16 mask)
{
	iowrite16(mask, dpld->mbx2cp);
}

/**
 * get_magic
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns the value of the "magic code" field in a DPRAM
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
 * Returns the value of the "access enable" field in a DPRAM
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
 * get_tx_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in a TX queue
 */
static inline u32 get_tx_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.head);
}

/**
 * get_tx_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (out) pointer in a TX queue
 *
 * It is useless for an AP to read a tail pointer in a TX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_tx_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.tail);
}

/**
 * set_tx_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in a TXQ
 */
static inline void set_tx_head(struct dpram_link_device *dpld, int id, u32 in)
{
	set_circ_pointer(dpld, id, TX, HEAD, dpld->dev[id]->txq.head, in);
}

/**
 * set_tx_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in a TXQ
 */
static inline void set_tx_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	set_circ_pointer(dpld, id, TX, TAIL, dpld->dev[id]->txq.tail, out);
}

/**
 * get_tx_buff
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in a TXQ
 */
static inline u8 *get_tx_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.buff;
}

/**
 * get_tx_buff_size
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in a TXQ
 */
static inline u32 get_tx_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.size;
}

/**
 * get_rx_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a head (in) pointer in an RX queue
 *
 * It is useless for an AP to read a head pointer in an RX queue twice to verify
 * whether or not the value in the pointer is valid, because it can already have
 * been updated by a CP after the first access from the AP.
 */
static inline u32 get_rx_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.head);
}

/**
 * get_rx_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the value of a tail (in) pointer in an RX queue
 */
static inline u32 get_rx_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.tail);
}

/**
 * set_rx_head
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @in: value to be written to the head pointer in an RXQ
 */
static inline void set_rx_head(struct dpram_link_device *dpld, int id, u32 in)
{
	set_circ_pointer(dpld, id, TX, HEAD, dpld->dev[id]->rxq.head, in);
}

/**
 * set_rx_tail
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @out: value to be written to the tail pointer in an RXQ
 */
static inline void set_rx_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	set_circ_pointer(dpld, id, TX, TAIL, dpld->dev[id]->rxq.tail, out);
}

/**
 * get_rx_buff
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the start address of the buffer in an RXQ
 */
static inline u8 *get_rx_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.buff;
}

/**
 * get_rx_buff_size
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the size of the buffer in an RXQ
 */
static inline u32 get_rx_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.size;
}

/**
 * get_mask_req_ack
 * @dpld: pointer to an instance of dpram_link_device structure
 * @id: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns the REQ_ACK mask value for the IPC device
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
 * Returns the RES_ACK mask value for the IPC device
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
 * Returns the SEND mask value for the IPC device
 */
static inline u16 get_mask_send(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_send;
}

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

	map->fmt_tx_in = get_tx_head(dpld, IPC_FMT);
	map->fmt_tx_out = get_tx_tail(dpld, IPC_FMT);
	map->fmt_rx_in = get_rx_head(dpld, IPC_FMT);
	map->fmt_rx_out = get_rx_tail(dpld, IPC_FMT);
	map->raw_tx_in = get_tx_head(dpld, IPC_RAW);
	map->raw_tx_out = get_tx_tail(dpld, IPC_RAW);
	map->raw_rx_in = get_rx_head(dpld, IPC_RAW);
	map->raw_rx_out = get_rx_tail(dpld, IPC_RAW);

	map->cp2ap = recv_intr(dpld);
}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
/**
 * log_dpram_status
 * @dpld: pointer to an instance of dpram_link_device structure
 * @str: pointer to a string that will be printed with a DPRAM status log
 *
 * Prints current DPRAM status with a string to a kernel log.
 */
static inline void log_dpram_status(struct dpram_link_device *dpld, char *str)
{
	struct utc_time utc;

	get_utc_time(&utc);

	pr_info("%s: %s: %s: [%02d:%02d:%02d.%03d] "
		"ACC{%X %d} FMT{TI:%u TO:%u RI:%u RO:%u} "
		"RAW{TI:%u TO:%u RI:%u RO:%u} INTR{0x%X}\n",
		MIF_TAG, dpld->ld.mc->name, str,
		utc.hour, utc.min, utc.sec, utc.msec,
		get_magic(dpld), get_access(dpld),
		get_tx_head(dpld, IPC_FMT), get_tx_tail(dpld, IPC_FMT),
		get_rx_head(dpld, IPC_FMT), get_rx_tail(dpld, IPC_FMT),
		get_tx_head(dpld, IPC_RAW), get_tx_tail(dpld, IPC_RAW),
		get_rx_head(dpld, IPC_RAW), get_rx_tail(dpld, IPC_RAW),
		recv_intr(dpld));
}

/**
 * save_dpram_dump_work
 * @work: pointer to an instance of work_struct structure
 *
 * Performs actual file operation for saving a DPRAM dump.
 */
static void save_dpram_dump_work(struct work_struct *work)
{
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct trace_queue *trq;
	struct trace_data *trd;
	struct file *fp;
	struct timespec *ts;
	u8 *dump;
	int rcvd;
	char *path;
	struct utc_time utc;

	dpld = container_of(work, struct dpram_link_device, dump_dwork.work);
	ld = &dpld->ld;
	trq = &dpld->dump_list;
	path = dpld->dump_path;

	while (1) {
		trd = trq_get_data_slot(trq);
		if (!trd)
			break;

		ts = &trd->ts;
		dump = trd->data;
		rcvd = trd->size;

		ts2utc(ts, &utc);
		snprintf(path, MIF_MAX_PATH_LEN,
			"%s/%s_dump_%d%02d%02d-%02d%02d%02d",
			MIF_LOG_DIR, ld->name, utc.year, utc.mon, utc.day,
			utc.hour, utc.min, utc.sec);

		fp = mif_open_file(path);
		if (fp) {
			mif_save_file(fp, dump, rcvd);
			mif_close_file(fp);
		} else {
			mif_err("%s: ERR! %s open fail\n", ld->name, path);
			mif_print_dump(dump, rcvd, 16);
		}

		kfree(dump);
	}
}

/**
 * save_dpram_dump
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Saves a current DPRAM dump.
 *
 * Actual file operation (save) will be performed by save_dpram_dump_work() that
 * is invoked by a delayed work.
 */
static void save_dpram_dump(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct trace_data *trd;
	u8 *buff;
	struct timespec ts;

	buff = kzalloc(dpld->size, GFP_ATOMIC);
	if (!buff) {
		mif_err("%s: ERR! kzalloc fail\n", ld->name);
		return;
	}

	getnstimeofday(&ts);

	memcpy(buff, dpld->base, dpld->size);

	trd = trq_get_free_slot(&dpld->dump_list);
	if (!trd) {
		mif_err("%s: ERR! trq_get_free_slot fail\n", ld->name);
		mif_print_dump(buff, dpld->size, 16);
		kfree(buff);
		return;
	}

	memcpy(&trd->ts, &ts, sizeof(struct timespec));
	trd->data = buff;
	trd->size = dpld->size;

	queue_delayed_work(system_nrt_wq, &dpld->dump_dwork, 0);
}

/**
 * pr_trace
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @ts: pointer to an instance of timespec structure
 * @buff: start address of a buffer into which RX IPC messages were copied
 * @rcvd: size of data in the buffer
 *
 * Prints IPC messages in a local memory buffer to a kernel log.
 */
static void pr_trace(struct dpram_link_device *dpld, int dev,
			struct timespec *ts, u8 *buff, u32 rcvd)
{
	struct link_device *ld = &dpld->ld;
	struct utc_time utc;

	ts2utc(ts, &utc);

	pr_info("%s: [%d-%02d-%02d %02d:%02d:%02d.%03d] %s trace (%s)\n",
		MIF_TAG, utc.year, utc.mon, utc.day, utc.hour, utc.min, utc.sec,
		utc.msec, get_dev_name(dev), ld->name);

	mif_print_dump(buff, rcvd, 4);

	return;
}

/**
 * print_ipc_trace
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @stat: pointer to an instance of dpram_circ_status structure
 *
 * Prints IPC messages currently in an RX circular queue to a kernel log.
 */
static void print_ipc_trace(struct dpram_link_device *dpld, int dev,
			struct dpram_circ_status *stat)
{
	u8 *buff = dpld->buff;
	struct timespec ts;

	getnstimeofday(&ts);

	/* Copy IPC messages from a DPRAM RXQ to a local buffer */
	memset(buff, 0, dpld->size);
	circ_read(buff, stat->buff, stat->qsize, stat->out, stat->size);

	/* Print IPC messages in the local buffer to a kernel log */
	pr_trace(dpld, dev, &ts, buff, stat->size);
}

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
	struct trace_queue *trq;
	struct trace_data *trd;
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
			dump = trd->data;
			rcvd = trd->size;
			pr_trace(dpld, dev, ts, dump, rcvd);

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
		dump = trd->data;
		rcvd = trd->size;

		ts2utc(ts, &utc);
		snprintf(path, MIF_MAX_PATH_LEN,
			"%s/%s_%s_%d%02d%02d-%02d%02d%02d",
			MIF_LOG_DIR, ld->name, get_dev_name(dev),
			utc.year, utc.mon, utc.day, utc.hour, utc.min, utc.sec);

		fp = mif_open_file(path);
		if (fp) {
			int len;

			snprintf(buff, MIF_MAX_PATH_LEN,
				"[%d-%02d-%02d %02d:%02d:%02d.%03d]\n",
				utc.year, utc.mon, utc.day, utc.hour, utc.min,
				utc.sec, utc.msec);
			len = strlen(buff);
			mif_dump2format4(dump, rcvd, (buff + len), NULL);
			strcat(buff, "\n");
			len = strlen(buff);

			mif_save_file(fp, buff, len);

			memset(buff, 0, len);
			mif_close_file(fp);
		} else {
			mif_err("%s: %s open fail\n", ld->name, path);
			pr_trace(dpld, dev, ts, dump, rcvd);
		}

		kfree(dump);
	}

	kfree(buff);
}

/**
 * save_ipc_trace
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @stat: pointer to an instance of dpram_circ_status structure
 *
 * Saves IPC messages currently in an RX circular queue.
 *
 * Actual file operation (save) will be performed by save_ipc_trace_work() that
 * is invoked by a delayed work.
 */
static void save_ipc_trace(struct dpram_link_device *dpld, int dev,
			struct dpram_circ_status *stat)
{
	struct link_device *ld = &dpld->ld;
	struct trace_data *trd;
	u8 *buff;
	struct timespec ts;

	buff = kzalloc(stat->size, GFP_ATOMIC);
	if (!buff) {
		mif_err("%s: %s: ERR! kzalloc fail\n",
			ld->name, get_dev_name(dev));
		print_ipc_trace(dpld, dev, stat);
		return;
	}

	getnstimeofday(&ts);

	circ_read(buff, stat->buff, stat->qsize, stat->out, stat->size);

	trd = trq_get_free_slot(&dpld->trace_list);
	if (!trd) {
		mif_err("%s: %s: ERR! trq_get_free_slot fail\n",
			ld->name, get_dev_name(dev));
		pr_trace(dpld, dev, &ts, buff, stat->size);
		kfree(buff);
		return;
	}

	memcpy(&trd->ts, &ts, sizeof(struct timespec));
	trd->dev = dev;
	trd->data = buff;
	trd->size = stat->size;

	queue_delayed_work(system_nrt_wq, &dpld->trace_dwork, 0);
}
#endif

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
		mif_err("%s: ERR! <%pf> ld->mode != LINK_MODE_IPC\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	/* Check "magic code" and "access enable" values */
	if (check_magic_access(dpld) < 0) {
		mif_err("%s: ERR! <%pf> check_magic_access fail\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	return true;
}

/**
 * dpram_can_sleep
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Returns tha value of the "need_wake_up" variable in a dpram_link_device
 * instance that is set in dpram_create_link_device().
 */
static inline bool dpram_can_sleep(struct dpram_link_device *dpld)
{
	return dpld->need_wake_up;
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

	if (unlikely(!dpram_can_sleep(dpld)))
		return 0;

	if (dpld->ext_op->wakeup(dpld) < 0) {
		mif_err("%s: ERR! <%pf> wakeup fail, once\n",
			ld->name, __builtin_return_address(0));

		dpld->ext_op->sleep(dpld);

		udelay(10);

		if (dpld->ext_op->wakeup(dpld) < 0) {
			mif_err("%s: ERR! <%pf> wakeup fail, twice\n",
				ld->name, __builtin_return_address(0));
			return -EACCES;
		}
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

	if (unlikely(!dpram_can_sleep(dpld)))
		return;

	if (atomic_dec_return(&dpld->accessing) <= 0) {
		dpld->ext_op->sleep(dpld);
		atomic_set(&dpld->accessing, 0);
		mif_debug("%s: DPRAM sleep possible\n", ld->name);
	}
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
 * trigger_force_cp_crash
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Triggers an enforced CP crash.
 */
static void trigger_force_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (ld->mode == LINK_MODE_ULOAD) {
		mif_err("%s: CP crash is already in progress\n", ld->mc->name);
		return;
	}

	disable_irq_nosync(dpld->irq);

	ld->mode = LINK_MODE_ULOAD;
	mif_info("%s: called by %pf\n", ld->name, __builtin_return_address(0));

	dpram_wake_up(dpld);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	/* Take a DPRAM dump */
	save_dpram_dump(dpld);
#endif

	enable_irq(dpld->irq);

	/* Send CRASH_EXIT command to a CP */
	send_intr(dpld, INT_CMD(INT_CMD_CRASH_EXIT));

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
 * Processes an extended command from a CP
 */
static void ext_command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;
	u16 resp;

	switch (EXT_CMD_MASK(cmd)) {
	case EXT_CMD_SET_SPEED_LOW:
		if (dpld->dpctl->setup_speed) {
			dpld->dpctl->setup_speed(DPRAM_SPEED_LOW);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_LOW);
			send_intr(dpld, resp);
		}
		break;

	case EXT_CMD_SET_SPEED_MID:
		if (dpld->dpctl->setup_speed) {
			dpld->dpctl->setup_speed(DPRAM_SPEED_MID);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_MID);
			send_intr(dpld, resp);
		}
		break;

	case EXT_CMD_SET_SPEED_HIGH:
		if (dpld->dpctl->setup_speed) {
			dpld->dpctl->setup_speed(DPRAM_SPEED_HIGH);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_HIGH);
			send_intr(dpld, resp);
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
 * Processes a command for upload/download from a CP
 */
static void udl_command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;

	if (cmd & UDL_RESULT_FAIL) {
		mif_info("%s: ERR! Command failed: %04x\n", ld->name, cmd);
		return;
	}

	switch (UDL_CMD_MASK(cmd)) {
	case UDL_CMD_RECV_READY:
		mif_debug("%s: Send CP-->AP RECEIVE_READY\n", ld->name);
		send_intr(dpld, CMD_IMG_START_REQ);
		break;
	default:
		complete_all(&dpld->udl_cmd_complete);
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
	send_intr(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
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

	/* Stop network interfaces */
	mif_netif_stop(ld);

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	for (i = 0; i < ld->max_ipc_dev; i++)
		skb_queue_purge(ld->skb_txq[i]);

	/* Change the modem state to STATE_CRASH_EXIT for the FMT IO device */
	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	/* Change the modem state to STATE_CRASH_EXIT for the BOOT IO device */
	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);
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

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	del_timer(&dpld->crash_ack_timer);

	dpram_wake_up(dpld);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	save_dpram_dump(dpld);
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
		set_tx_head(dpld, i, 0);
		set_tx_tail(dpld, i, 0);
		set_rx_head(dpld, i, 0);
		set_rx_tail(dpld, i, 0);
	}

	/* Initialize variables for efficient TX/RX processing */
	for (i = 0; i < ld->max_ipc_dev; i++)
		dpld->iod[i] = link_get_iod_with_format(ld, i);
	dpld->iod[IPC_RAW] = link_get_iod_with_format(ld, IPC_MULTI_RAW);

	if (dpld->iod[IPC_RAW]->recv_skb)
		dpld->rx_with_skb = true;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		spin_lock_init(&dpld->tx_lock[i]);
		atomic_set(&dpld->res_required[i], 0);
	}

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
 * cmd_phone_start_handler
 * @dpld: pointer to an instance of dpram_link_device structure
 *
 * Handles the PHONE_START command from a CP.
 */
static void cmd_phone_start_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = NULL;

	mif_info("%s: Recv 0xC8 (CP_START)\n", ld->name);

	init_dpram_ipc(dpld);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (!iod) {
		mif_info("%s: ERR! no iod\n", ld->name);
		return;
	}

	if (dpld->ext_op && dpld->ext_op->cp_start_handler)
		dpld->ext_op->cp_start_handler(dpld);

	if (ld->mc->phone_state != STATE_ONLINE) {
		mif_info("%s: phone_state: %d -> ONLINE\n",
			ld->name, ld->mc->phone_state);
		iod->modem_state_changed(iod, STATE_ONLINE);
	}

	mif_info("%s: Send 0xC2 (INIT_END)\n", ld->name);
	send_intr(dpld, INT_CMD(INT_CMD_INIT_END));
}

/**
 * command_handler: processes a DPRAM command from a CP
 * @dpld: pointer to an instance of dpram_link_device structure
 * @cmd: DPRAM command from a CP
 */
static void command_handler(struct dpram_link_device *dpld, u16 cmd)
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
		complete_all(&dpld->dpram_init_cmd);
		break;

	case INT_CMD_NV_REBUILDING:
		mif_info("%s: NV_REBUILDING\n", ld->name);
		break;

	case INT_CMD_PIF_INIT_DONE:
		complete_all(&dpld->modem_pif_init_done);
		break;

	case INT_CMD_SILENT_NV_REBUILDING:
		mif_info("%s: SILENT_NV_REBUILDING\n", ld->name);
		break;

	case INT_CMD_NORMAL_PWR_OFF:
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
 * ipc_rx_task
 * @data: pointer to an instance of dpram_link_device structure
 *
 * Invokes the recv method in the io_device instance to perform receiving IPC
 * messages from each mif_rxb.
 */
static void ipc_rx_task(unsigned long data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;
	struct mif_rxb *rxb;
	int qlen;
	int i;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		iod = dpld->iod[i];
		qlen = rxbq_size(&dpld->rxbq[i]);
		while (qlen > 0) {
			rxb = rxbq_get_data_rxb(&dpld->rxbq[i]);
			iod->recv(iod, ld, rxb->data, rxb->len);
			rxb_clear(rxb);
			qlen--;
		}
	}
}

/**
 * get_rxq_rcvd
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * OUT @stat: pointer to an instance of dpram_circ_status structure
 *
 * Stores {start address of the buffer in a RXQ, size of the buffer, in & out
 * pointer values, size of received data} into the 'stat' instance.
 *
 * Returns the size of received data in the buffer or an error code.
 */
static int get_rxq_rcvd(struct dpram_link_device *dpld, int dev,
			struct dpram_circ_status *stat)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 0;
	u32 qsize;
	u32 head;
	u32 tail;
	int rcvd;

	while (1) {
		qsize = get_rx_buff_size(dpld, dev);
		head = get_rx_head(dpld, dev);
		tail = get_rx_tail(dpld, dev);
		rcvd = circ_get_usage(qsize, head, tail);

		mif_debug("%s: %s_RXQ qsize[%u] in[%u] out[%u] rcvd[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, rcvd);

		if (circ_valid(qsize, head, tail))
			break;

		cnt++;
		mif_err("%s: ERR! <%pf> "
			"%s_RXQ invalid (qsize:%d in:%d out:%d rcvd:%d), "
			"count %d\n",
			ld->name, __builtin_return_address(0),
			get_dev_name(dev), qsize, head, tail, rcvd, cnt);
		if (cnt >= MAX_RETRY_CNT) {
			rcvd = -EIO;
			break;
		}

		udelay(100);
	}

	stat->buff = get_rx_buff(dpld, dev);
	stat->qsize = qsize;
	stat->in = head;
	stat->out = tail;
	stat->size = rcvd;

	return rcvd;
}

/**
 * recv_ipc_with_rxb: receives IPC messages from an RXQ with mif_rxb
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : no data
 *   ret > 0  : valid data
 *
 * Requires a bottom half (e.g. ipc_rx_task) that will invoke the recv method in
 * the io_device instance.
 */
static int recv_ipc_with_rxb(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct mif_rxb *rxb;
	struct dpram_circ_status stat;
	int rcvd;
	u8 *dst;
	struct mif_irq_map map;

	rcvd = get_rxq_rcvd(dpld, dev, &stat);
	if (unlikely(rcvd <= 0)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		if (rcvd < 0)
			trigger_force_cp_crash(dpld);
#endif
		goto exit;
	}

	if (dev == IPC_FMT) {
#if 0
		log_dpram_status(dpld, "CP2MIF");
#endif
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

	/* Allocate an rxb */
	rxb = rxbq_get_free_rxb(&dpld->rxbq[dev]);
	if (!rxb) {
		mif_info("%s: ERR! %s rxbq_get_free_rxb fail\n",
			ld->name, get_dev_name(dev));
		rcvd = -ENOMEM;
		goto exit;
	}

	/* Read data from the RXQ */
	dst = rxb_put(rxb, stat.size);
	circ_read(dst, stat.buff, stat.qsize, stat.out, stat.size);

exit:
	/* Update tail (out) pointer to empty out the RXQ */
	set_rx_tail(dpld, dev, stat.in);

	return rcvd;
}

/**
 * recv_ipc_with_skb: receives SIPC5 messages from an RXQ with skb
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : no data
 *   ret > 0  : valid data
 *
 * Requires a recv_skb method in the io_device instance, so this function must
 * be used for only SIPC5.
 */
static int recv_ipc_with_skb(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = dpld->iod[dev];
	struct sk_buff *skb;
	/**
	 * variables for the status of the circular queue
	 */
	u8 __iomem *src;
	struct dpram_circ_status stat;
	/**
	 * variables for RX processing
	 */
	int qsize;	/* size of the queue			*/
	int rcvd;	/* size of data in the RXQ or error	*/
	int rest;	/* size of the rest data		*/
	int idx;	/* index to the start of current frame	*/
	u8 *frm;	/* pointer to current frame		*/
	u8 *dst;	/* pointer to the destination buffer	*/
	int len;	/* length of current frame		*/
	int tot;	/* total length including padding data	*/
	/**
	 * variables for debug logging
	 */
	struct mif_irq_map map;

	/* Get data size in the RXQ and in/out pointer values */
	rcvd = get_rxq_rcvd(dpld, dev, &stat);
	if (unlikely(rcvd <= 0)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		if (rcvd < 0)
			trigger_force_cp_crash(dpld);
#endif
		goto exit;
	}

	/* Take a log for debugging */
	if (dev == IPC_FMT) {
#if 0
		log_dpram_status(dpld, "CP2MIF");
#endif
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

	src = stat.buff;
	qsize = stat.qsize;
	rest = stat.size;
	idx = stat.out;

	while (rest > 0) {
		/* Calculate the start of an SIPC5 frame */
		frm = src + idx;

		/* Check the SIPC5 frame */
		len = sipc5_check_frame_in_dev(ld, dev, frm, rest);
		if (len <= 0) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			log_dpram_status(dpld, "CP2MIF");
			save_ipc_trace(dpld, dev, &stat);
			trigger_force_cp_crash(dpld);
#endif
			rcvd = -EBADMSG;
			goto exit;
		}

		/* Calculate total length of the frame (data + padding) */
		tot = len + sipc5_calc_padding_size(len);

		/* Allocate an skb */
		skb = dev_alloc_skb(tot);
		if (!skb) {
			mif_err("%s: ERR! %s dev_alloc_skb fail\n",
				ld->name, get_dev_name(dev));
			rcvd = -ENOMEM;
			goto exit;
		}

		/* Read the frame from the RXQ */
		dst = skb_put(skb, tot);
		circ_read(dst, src, qsize, idx, tot);

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		/* Take a log for debugging */
		if (unlikely(dev == IPC_FMT)) {
			char str[MIF_MAX_STR_LEN];
			snprintf(str, MIF_MAX_STR_LEN, "%s: CP2MIF",
				ld->mc->name);
			pr_ipc(str, skb->data, (skb->len > 20 ? 20 : skb->len));
		}
#endif

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		/* Verify data copied to the skb */
		if (ld->aligned && memcmp16_to_io(frm, dst, 4)) {
			mif_err("%s: memcmp16_to_io fail\n", ld->name);
			trigger_force_cp_crash(dpld);
			rcvd = -EIO;
			goto exit;
		}
#endif

		/* Remove padding in the skb */
		skb_trim(skb, len);

		/* Pass the frame to the corresponding IO device */
		iod->recv_skb(iod, ld, skb);

		/* Calculate new idx value */
		rest -= tot;
		idx += tot;
		if (idx >= qsize)
			idx -= qsize;
	}

exit:
	/* Update tail (out) pointer to empty out the RXQ */
	set_rx_tail(dpld, dev, stat.in);

	return rcvd;
}

/**
 * recv_ipc_msg: receives IPC messages from every RXQ
 * @dpld: pointer to an instance of dpram_link_device structure
 * @intr: interrupt value from a CP
 *
 * 1) Receives all IPC message frames currently in every DPRAM RXQ.
 * 2) Sends RES_ACK responses if there are REQ_ACK requests from a CP.
 * 3) Completes all threads waiting for the corresponding RES_ACK from a CP if
 *    there is any RES_ACK response.
 */
static void recv_ipc_msg(struct dpram_link_device *dpld, u16 intr)
{
	struct link_device *ld = &dpld->ld;
	int i = 0;
	int ret = 0;
	u16 mask = 0;

	if (!ipc_active(dpld))
		return;

	/* Read data from DPRAM */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		if (dpld->rx_with_skb)
			ret = recv_ipc_with_skb(dpld, i);
		else
			ret = recv_ipc_with_rxb(dpld, i);

		/* Check and process REQ_ACK (at this time, in == out) */
		if (intr & get_mask_req_ack(dpld, i)) {
			mif_debug("%s: send %s_RES_ACK\n",
				ld->name, get_dev_name(i));
			mask |= get_mask_res_ack(dpld, i);
		}
	}

	if (!dpld->rx_with_skb) {
		/* Schedule soft IRQ for RX */
		tasklet_hi_schedule(&dpld->rx_tsk);
	}

	if (mask) {
		send_intr(dpld, INT_NON_CMD(mask));
		mif_debug("%s: send intr 0x%04X\n", ld->name, mask);
	}

	if (intr && INT_MASK_RES_ACK_SET) {
		if (intr && INT_MASK_RES_ACK_R)
			complete_all(&dpld->req_ack_cmpl[IPC_RAW]);
		else if (intr && INT_MASK_RES_ACK_F)
			complete_all(&dpld->req_ack_cmpl[IPC_FMT]);
		else
			complete_all(&dpld->req_ack_cmpl[IPC_RFS]);
	}
}

/**
 * cmd_msg_handler: processes a DPRAM command or receives IPC messages
 * @dpld: pointer to an instance of dpram_link_device structure
 * @intr: interrupt value from a CP
 *
 * Invokes command_handler for a DPRAM command or recv_ipc_msg for IPC messages.
 */
static inline void cmd_msg_handler(struct dpram_link_device *dpld, u16 intr)
{
	if (unlikely(INT_CMD_VALID(intr)))
		command_handler(dpld, intr);
	else
		recv_ipc_msg(dpld, intr);
}

/**
 * intr_handler: processes an interrupt from a CP
 * @dpld: pointer to an instance of dpram_link_device structure
 * @intr: interrupt value from a CP
 *
 * Call flow for normal interrupt handling:
 *   cmd_msg_handler -> command_handler -> cmd_xxx_handler
 *   cmd_msg_handler -> recv_ipc_msg -> recv_ipc_with_skb/recv_ipc_with_rxb ...
 */
static inline void intr_handler(struct dpram_link_device *dpld, u16 intr)
{
	char *name = dpld->ld.name;

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
			mif_info("%s: ERR! invalid intr 0x%04X\n", name, intr);
		}

		return;
	}

	if (likely(INT_VALID(intr)))
		cmd_msg_handler(dpld, intr);
	else
		mif_info("%s: ERR! invalid intr 0x%04X\n", name, intr);
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
	u16 int2ap = recv_intr(dpld);

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	intr_handler(dpld, int2ap);

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
	u16 int2ap;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	if (dpram_wake_up(dpld) < 0) {
		trigger_force_cp_crash(dpld);
		return IRQ_HANDLED;
	}

	int2ap = recv_intr(dpld);

	intr_handler(dpld, int2ap);

	clear_intr(dpld);

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
	u16 int2ap = recv_intr(dpld);

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	intr_handler(dpld, int2ap);

	return IRQ_HANDLED;
}

/**
 * get_txq_space
 * @dpld: pointer to an instance of dpram_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * OUT @stat: pointer to an instance of dpram_circ_status structure
 *
 * Stores {start address of the buffer in a TXQ, size of the buffer, in & out
 * pointer values, size of free space} into the 'stat' instance.
 *
 * Returns the size of free space in the buffer or an error code.
 */
static int get_txq_space(struct dpram_link_device *dpld, int dev,
			struct dpram_circ_status *stat)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 0;
	u32 qsize;
	u32 head;
	u32 tail;
	int space;

	while (1) {
		qsize = get_tx_buff_size(dpld, dev);
		head = get_tx_head(dpld, dev);
		tail = get_tx_tail(dpld, dev);
		space = circ_get_space(qsize, head, tail);

		mif_debug("%s: %s_TXQ qsize[%u] in[%u] out[%u] space[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, space);

		if (circ_valid(qsize, head, tail))
			break;

		cnt++;
		mif_err("%s: ERR! <%pf> "
			"%s_TXQ invalid (qsize:%d in:%d out:%d space:%d), "
			"count %d\n",
			ld->name, __builtin_return_address(0),
			get_dev_name(dev), qsize, head, tail, space, cnt);
		if (cnt >= MAX_RETRY_CNT) {
			space = -EIO;
			break;
		}

		udelay(100);
	}

	stat->buff = get_tx_buff(dpld, dev);
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
 * @stat: pointer to an instance of dpram_circ_status structure
 * @skb: pointer to an instance of sk_buff structure
 *
 * Must be invoked only when there is enough space in the TXQ.
 */
static void write_ipc_to_txq(struct dpram_link_device *dpld, int dev,
			struct dpram_circ_status *stat, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *buff = stat->buff;
	u32 qsize = stat->qsize;
	u32 in = stat->in;
	u8 *src = skb->data;
	u32 len = skb->len;
	u32 inp;
	struct mif_irq_map map;

	/* Write data to the TXQ */
	circ_write(buff, src, qsize, in, len);

	/* Update new head (in) pointer */
	inp = in + len;
	if (inp >= qsize)
		inp -= qsize;
	set_tx_head(dpld, dev, inp);

	/* Take a log for debugging */
	if (dev == IPC_FMT) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: MIF2CP", ld->mc->name);
		pr_ipc(tag, src, (len > 20 ? 20 : len));
#if 0
		log_dpram_status(dpld, "MIF2CP");
#endif
#endif
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_write", sizeof("ipc_write"));
		mif_ipc_log(MIF_IPC_AP2CP, ld->mc->msd, skb->data, skb->len);
	}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	/* Verify data written to the TXQ */
	if (ld->aligned && memcmp16_to_io((buff + in), src, 4)) {
		mif_err("%s: memcmp16_to_io fail\n", ld->name);
		trigger_force_cp_crash(dpld);
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
	struct dpram_circ_status stat;
	int space;
	int copied = 0;

	/* Acquire the spin lock for a TXQ */
	spin_lock_irqsave(&dpld->tx_lock[dev], flags);

	while (1) {
		/* Get the size of free space in the TXQ */
		space = get_txq_space(dpld, dev, &stat);
		if (unlikely(space < 0)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			/* Trigger a enforced CP crash */
			trigger_force_cp_crash(dpld);
#endif
			/* Empty out the TXQ */
			set_tx_head(dpld, dev, stat.out);
			copied = -EIO;
			break;
		}

		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		/* Check the free space size comparing with skb->len */
		if (unlikely(space < skb->len)) {
			atomic_set(&dpld->res_required[dev], 1);
			/* Take the skb back to the skb_txq */
			skb_queue_head(txq, skb);
			mif_info("%s: %s qsize[%u] "
				"in[%u] out[%u] free[%u] < len[%u]\n",
				ld->name, get_dev_name(dev), stat.qsize,
				stat.in, stat.out, space, skb->len);
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
	unsigned long timeout = RES_ACK_WAIT_TIMEOUT;
	int ret;
	u16 mask;

	mask = get_mask_req_ack(dpld, dev);
	mif_info("%s: send %s_REQ_ACK\n", ld->name, get_dev_name(dev));
	send_intr(dpld, INT_NON_CMD(mask));

	ret = wait_for_completion_interruptible_timeout(cmpl, timeout);
	/* ret == 0 on timeout, ret < 0 if interrupted */
	if (ret == 0) {
		mif_info("%s: TIMEOUT! no %s_RES_ACK\n",
			ld->name, get_dev_name(dev));
	} else if (ret < 0) {
		mif_info("%s: %s: interrupted (ret %d)\n",
			ld->name, get_dev_name(dev), ret);
	}

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
	struct link_device *ld = &dpld->ld;
	int ret;
	u16 mask;

	mif_info("%s: recv %s_RES_ACK\n", ld->name, get_dev_name(dev));
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	log_dpram_status(dpld, "LATEST");
#endif

	ret = xmit_ipc_msg(dpld, dev);
	if (ret > 0) {
		mask = get_mask_send(dpld, dev);
		send_intr(dpld, INT_NON_CMD(mask));
		atomic_set(&dpld->res_required[dev], 0);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		mif_info("%s: xmit_ipc_msg done (%d bytes)\n", ld->name, ret);
#endif
		goto exit;
	}

	if (ret == 0) {
		mif_info("%s: %s skb_txq empty\n", ld->name, get_dev_name(dev));
		atomic_set(&dpld->res_required[dev], 0);
		goto exit;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC) {
		/*
		** dpld->res_required[dev] is set in xmit_ipc_msg()
		*/
		mif_info("%s: xmit_ipc_msg fail (err -ENOSPC)\n", ld->name);
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[dev], 0);
	} else {
		mif_err("%s: ERR! xmit_ipc_msg fail (err %d)\n",
			ld->name, ret);
	}

exit:
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
	unsigned long delay = REQ_ACK_DELAY;
	int ret;

	ld = container_of(work, struct link_device, fmt_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_FMT);
	/* ret == 0 on timeout, ret < 0 if interrupted */
	if (ret < 0)
		return;
	else if (ret == 0)
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_FMT], delay);
	else
		process_res_ack(dpld, IPC_FMT);
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
	unsigned long delay = REQ_ACK_DELAY;
	int ret;

	ld = container_of(work, struct link_device, raw_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_RAW);
	/* ret == 0 on timeout, ret < 0 if interrupted */
	if (ret < 0)
		return;

	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RAW], delay);
		return;
	}

	ret = process_res_ack(dpld, IPC_RAW);
	if (ret > 0)
		mif_netif_wake(ld);
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
	unsigned long delay = REQ_ACK_DELAY;
	int ret;

	ld = container_of(work, struct link_device, rfs_tx_dwork.work);
	dpld = to_dpram_link_device(ld);

	ret = wait_for_res_ack(dpld, IPC_RFS);
	/* ret == 0 on timeout, ret < 0 if interrupted */
	if (ret < 0)
		return;
	else if (ret == 0)
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RFS], delay);
	else
		process_res_ack(dpld, IPC_RFS);
}

/**
 * dpram_send_ipc
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
static void dpram_send_ipc(struct dpram_link_device *dpld, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	int ret;
	u16 mask;

	if (unlikely(txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err("%s: %s txq->qlen %d >= %d\n", ld->name,
			get_dev_name(dev), txq->qlen, MAX_SKB_TXQ_DEPTH);
		if (iod->io_typ == IODEV_NET || iod->format == IPC_MULTI_RAW) {
			dev_kfree_skb_any(skb);
			return;
		}
	}

	skb_queue_tail(txq, skb);

	if (dpram_wake_up(dpld) < 0) {
		trigger_force_cp_crash(dpld);
		return;
	}

	if (!ipc_active(dpld)) {
		mif_info("%s: IPC is NOT active\n", ld->name);
		goto exit;
	}

	if (atomic_read(&dpld->res_required[dev]) > 0) {
		mif_info("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		goto exit;
	}

	ret = xmit_ipc_msg(dpld, dev);
	if (likely(ret > 0)) {
		mask = get_mask_send(dpld, dev);
		send_intr(dpld, INT_NON_CMD(mask));
		goto exit;
	}

	if (ret == 0) {
		mif_info("%s: %s skb_txq empty\n", ld->name, get_dev_name(dev));
		goto exit;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC) {
		/*
		** dpld->res_required[dev] is set in xmit_ipc_msg()
		*/
		if (dev == IPC_RAW)
			mif_netif_stop(ld);
		mif_info("%s->%s: xmit_ipc_msg fail (err -ENOSPC)\n",
			iod->name, ld->name);
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[dev], 0);
	} else {
		mif_err("%s->%s: ERR! xmit_ipc_msg fail (err %d)\n",
			iod->name, ld->name, ret);
	}

exit:
	dpram_allow_sleep(dpld);
}

static int dpram_send_cp_binary(struct link_device *ld, struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	if (dpld->ext_op && dpld->ext_op->download_binary)
		return dpld->ext_op->download_binary(dpld, skb);
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
 *   dpram_send_ipc -> xmit_ipc_msg -> write_ipc_to_txq
 *
 * Call flow on congestion in a DPRAM IPC TXQ:
 *   dpram_send_ipc -> xmit_ipc_msg ,,, queue_delayed_work
 *   => xxx_tx_work -> wait_for_res_ack
 *   => recv_ipc_msg
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
			dpram_send_ipc(dpld, dev, iod, skb);
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

static int dpram_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	trigger_force_cp_crash(dpld);
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
		return dpld->ext_op->dump_update(dpld, (void *)arg);
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

	default:
		if (dpld->ext_ioctl) {
			err = dpld->ext_ioctl(dpld, iod, cmd, arg);
		} else {
			mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
			err = -EINVAL;
		}

		break;
	}

	return err;
}

static void dpram_dump_memory(struct link_device *ld, char *buff)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	dpram_wake_up(dpld);
	memcpy(buff, dpld->base, dpld->size);
	dpram_allow_sleep(dpld);
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

static int dpram_table_init(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *dp_base;
	int i;

	if (!dpld->base) {
		mif_err("%s: ERR! dpld->base == NULL\n", ld->name);
		return -EINVAL;
	}
	dp_base = dpld->base;

	/* Map for booting */
	if (dpld->ext_op && dpld->ext_op->init_boot_map) {
		dpld->ext_op->init_boot_map(dpld);
	} else {
		dpld->bt_map.magic = (u32 *)(dp_base);
		dpld->bt_map.buff = (u8 *)(dp_base + DP_BOOT_BUFF_OFFSET);
		dpld->bt_map.size = dpld->size - 8;
	}

	/* Map for download (FOTA, UDL, etc.) */
	if (dpld->ext_op && dpld->ext_op->init_dl_map) {
		dpld->ext_op->init_dl_map(dpld);
	} else {
		dpld->dl_map.magic = (u32 *)(dp_base);
		dpld->dl_map.buff = (u8 *)(dp_base + DP_DLOAD_BUFF_OFFSET);
	}

	/* Map for upload mode */
	if (dpld->ext_op && dpld->ext_op->init_ul_map) {
		dpld->ext_op->init_ul_map(dpld);
	} else {
		dpld->ul_map.magic = (u32 *)(dp_base);
		dpld->ul_map.buff = (u8 *)(dp_base + DP_ULOAD_BUFF_OFFSET);
	}

	/* Map for IPC */
	if (dpld->ext_op && dpld->ext_op->init_ipc_map) {
		dpld->ext_op->init_ipc_map(dpld);
	} else if (dpld->dpctl->ipc_map) {
		memcpy(&dpld->ipc_map, dpld->dpctl->ipc_map,
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
	dpld->recv_intr = recv_intr;
	dpld->send_intr = send_intr;
	dpld->get_magic = get_magic;
	dpld->set_magic = set_magic;
	dpld->get_access = get_access;
	dpld->set_access = set_access;
	dpld->get_tx_head = get_tx_head;
	dpld->get_tx_tail = get_tx_tail;
	dpld->set_tx_head = set_tx_head;
	dpld->set_tx_tail = set_tx_tail;
	dpld->get_tx_buff = get_tx_buff;
	dpld->get_tx_buff_size = get_tx_buff_size;
	dpld->get_rx_head = get_rx_head;
	dpld->get_rx_tail = get_rx_tail;
	dpld->set_rx_head = set_rx_head;
	dpld->set_rx_tail = set_rx_tail;
	dpld->get_rx_buff = get_rx_buff;
	dpld->get_rx_buff_size = get_rx_buff_size;
	dpld->get_mask_req_ack = get_mask_req_ack;
	dpld->get_mask_res_ack = get_mask_res_ack;
	dpld->get_mask_send = get_mask_send;
	dpld->ipc_rx_handler = cmd_msg_handler;
}

static int dpram_link_init(struct link_device *ld, struct io_device *iod)
{
	return 0;
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
	struct modemlink_dpram_control *dpctl = NULL;
	struct resource *res = NULL;
	resource_size_t res_size;
	unsigned long task_data;
	int ret = 0;
	int i = 0;
	int bsize;
	int qsize;

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

	if (!modem->dpram_ctl) {
		mif_err("ERR! modem->dpram_ctl == NULL\n");
		goto err;
	}
	dpctl = modem->dpram_ctl;

	dpld->dpctl = dpctl;
	dpld->type = dpctl->dp_type;

	if (ld->ipc_version < SIPC_VER_50) {
		if (!dpctl->max_ipc_dev) {
			mif_err("%s: ERR! no max_ipc_dev\n", ld->name);
			goto err;
		}

		ld->aligned = dpctl->aligned;
		ld->max_ipc_dev = dpctl->max_ipc_dev;
	} else {
		ld->aligned = 1;
		ld->max_ipc_dev = MAX_SIPC5_DEV;
	}

	/*
	** Set attributes as a link device
	*/
	ld->init_comm = dpram_link_init;
	ld->terminate_comm = dpram_link_terminate;
	ld->send = dpram_send;
	ld->force_dump = dpram_force_dump;
	ld->dump_start = dpram_dump_start;
	ld->dump_update = dpram_dump_update;
	ld->ioctl = dpram_ioctl;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	skb_queue_head_init(&ld->sk_rfs_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;
	ld->skb_txq[IPC_RFS] = &ld->sk_rfs_tx_q;

	/*
	** Set up function pointers
	*/
	dpram_setup_common_op(dpld);
	dpld->dpram_dump = dpram_dump_memory;
	dpld->ext_op = dpram_get_ext_op(modem->modem_type);
	if (dpld->ext_op && dpld->ext_op->ioctl)
		dpld->ext_ioctl = dpld->ext_op->ioctl;
	if (dpld->ext_op && dpld->ext_op->wakeup && dpld->ext_op->sleep)
		dpld->need_wake_up = true;
	if (dpld->ext_op && dpld->ext_op->clear_intr)
		dpld->need_intr_clear = true;

	/*
	** Retrieve DPRAM resource
	*/
	if (!dpctl->dp_base) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						STR_DPRAM_BASE);
		if (!res) {
			mif_err("%s: ERR! no DPRAM resource\n", ld->name);
			goto err;
		}
		res_size = resource_size(res);

		dpctl->dp_base = ioremap_nocache(res->start, res_size);
		if (!dpctl->dp_base) {
			mif_err("%s: ERR! ioremap_nocache for BASE fail\n",
				ld->name);
			goto err;
		}
		dpctl->dp_size = res_size;
	}
	dpld->base = dpctl->dp_base;
	dpld->size = dpctl->dp_size;

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

	/* Initialize DPRAM map (physical map -> logical map) */
	ret = dpram_table_init(dpld);
	if (ret < 0) {
		mif_err("%s: ERR! dpram_table_init fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	/* Disable IPC */
	if (!dpctl->disabled) {
		set_magic(dpld, 0);
		set_access(dpld, 0);
	}
	dpld->init_status = DPRAM_INIT_STATE_NONE;

	/* Initialize locks, completions, and bottom halves */
	snprintf(dpld->wlock_name, MIF_MAX_NAME_LEN, "%s_wlock", ld->name);
	wake_lock_init(&dpld->wlock, WAKE_LOCK_SUSPEND, dpld->wlock_name);

	init_completion(&dpld->dpram_init_cmd);
	init_completion(&dpld->modem_pif_init_done);
	init_completion(&dpld->udl_start_complete);
	init_completion(&dpld->udl_cmd_complete);
	init_completion(&dpld->crash_start_complete);
	init_completion(&dpld->crash_recv_done);
	for (i = 0; i < ld->max_ipc_dev; i++)
		init_completion(&dpld->req_ack_cmpl[i]);

	task_data = (unsigned long)dpld;
	tasklet_init(&dpld->rx_tsk, ipc_rx_task, task_data);

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

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	INIT_DELAYED_WORK(&dpld->dump_dwork, save_dpram_dump_work);
	INIT_DELAYED_WORK(&dpld->trace_dwork, save_ipc_trace_work);
	spin_lock_init(&dpld->dump_list.lock);
	spin_lock_init(&dpld->trace_list.lock);
#endif

	/* Prepare RXB queue */
	qsize = DPRAM_MAX_RXBQ_SIZE;
	for (i = 0; i < ld->max_ipc_dev; i++) {
		bsize = rxbq_get_page_size(get_rx_buff_size(dpld, i));
		dpld->rxbq[i].size = qsize;
		dpld->rxbq[i].in = 0;
		dpld->rxbq[i].out = 0;
		dpld->rxbq[i].rxb = rxbq_create_pool(bsize, qsize);
		if (!dpld->rxbq[i].rxb) {
			mif_err("%s: ERR! %s rxbq_create_pool fail\n",
				ld->name, get_dev_name(i));
			goto err;
		}
		mif_info("%s: %s rxbq_pool created (bsize:%d, qsize:%d)\n",
			ld->name, get_dev_name(i), bsize, qsize);
	}

	/* Prepare a multi-purpose miscellaneous buffer */
	dpld->buff = kzalloc(dpld->size, GFP_KERNEL);
	if (!dpld->buff) {
		mif_err("%s: ERR! kzalloc dpld->buff fail\n", ld->name);
		goto err;
	}

	/*
	** Retrieve DPRAM IRQ GPIO#, IRQ#, and IRQ flags
	*/
	dpld->gpio_dpram_int = modem->gpio_dpram_int;

	if (dpctl->dpram_irq) {
		dpld->irq = dpctl->dpram_irq;
	} else {
		dpld->irq = platform_get_irq_byname(pdev, STR_DPRAM_IRQ);
		if (dpld->irq < 0) {
			mif_err("%s: ERR! no DPRAM IRQ resource\n", ld->name);
			goto err;
		}
	}

	if (dpctl->dpram_irq_flags)
		dpld->irq_flags = dpctl->dpram_irq_flags;
	else
		dpld->irq_flags = (IRQF_NO_SUSPEND | IRQF_TRIGGER_LOW);

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

	ret = register_isr(dpld->irq, dpld->irq_handler, dpld->irq_flags,
				dpld->irq_name, dpld);
	if (ret)
		goto err;

	return ld;

err:
	if (dpld) {
		if (dpld->buff)
			kfree(dpld->buff);
		kfree(dpld);
	}

	return NULL;
}

