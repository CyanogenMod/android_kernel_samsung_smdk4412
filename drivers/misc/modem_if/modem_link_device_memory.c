/*
 * Copyright (C) 2011 Samsung Electronics.
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
#include "modem_link_device_memory.h"
#ifdef CONFIG_LINK_DEVICE_DPRAM
#include "modem_link_device_dpram.h"
#endif

/**
 * msq_get_free_slot
 * @trq : pointer to an instance of mem_status_queue structure
 *
 * Succeeds always by dropping the oldest slot if a "msq" is full.
 */
struct mem_status *msq_get_free_slot(struct mem_status_queue *msq)
{
	int qsize = MAX_MEM_LOG_CNT;
	int in;
	int out;
	unsigned long flags;
	struct mem_status *stat;

	spin_lock_irqsave(&msq->lock, flags);

	in = msq->in;
	out = msq->out;

	if (circ_get_space(qsize, in, out) < 1) {
		/* Make the oldest slot empty */
		out++;
		msq->out = (out == qsize) ? 0 : out;
	}

	/* Get a free slot */
	stat = &msq->stat[in];

	/* Make it as "data" slot */
	in++;
	msq->in = (in == qsize) ? 0 : in;

	spin_unlock_irqrestore(&msq->lock, flags);

	memset(stat, 0, sizeof(struct mem_status));

	return stat;
}

struct mem_status *msq_get_data_slot(struct mem_status_queue *msq)
{
	int qsize = MAX_MEM_LOG_CNT;
	int in;
	int out;
	unsigned long flags;
	struct mem_status *stat;

	spin_lock_irqsave(&msq->lock, flags);

	in = msq->in;
	out = msq->out;

	if (in == out) {
		stat = NULL;
		goto exit;
	}

	/* Get a data slot */
	stat = &msq->stat[out];

	/* Make it "free" slot */
	out++;
	msq->out = (out == qsize) ? 0 : out;

exit:
	spin_unlock_irqrestore(&msq->lock, flags);
	return stat;
}

/**
 * memcpy16_from_io
 * @to: pointer to "real" memory
 * @from: pointer to IO memory
 * @count: data length in bytes to be copied
 *
 * Copies data from IO memory space to "real" memory space.
 */
void memcpy16_from_io(const void *to, const void __iomem *from, u32 count)
{
	u16 *d = (u16 *)to;
	u16 *s = (u16 *)from;
	u32 words = count >> 1;
	while (words--)
		*d++ = ioread16(s++);
}

/**
 * memcpy16_to_io
 * @to: pointer to IO memory
 * @from: pointer to "real" memory
 * @count: data length in bytes to be copied
 *
 * Copies data from "real" memory space to IO memory space.
 */
void memcpy16_to_io(const void __iomem *to, const void *from, u32 count)
{
	u16 *d = (u16 *)to;
	u16 *s = (u16 *)from;
	u32 words = count >> 1;
	while (words--)
		iowrite16(*s++, d++);
}

/**
 * memcmp16_to_io
 * @to: pointer to IO memory
 * @from: pointer to "real" memory
 * @count: data length in bytes to be compared
 *
 * Compares data from "real" memory space to IO memory space.
 */
int memcmp16_to_io(const void __iomem *to, const void *from, u32 count)
{
	u16 *d = (u16 *)to;
	u16 *s = (u16 *)from;
	int words = count >> 1;
	int diff = 0;
	int i;
	u16 d1;
	u16 s1;

	for (i = 0; i < words; i++) {
		d1 = ioread16(d);
		s1 = *s;
		if (d1 != s1) {
			diff++;
			mif_err("ERR! [%d] d:0x%04X != s:0x%04X\n", i, d1, s1);
		}
		d++;
		s++;
	}

	return diff;
}

/**
 * circ_read16_from_io
 * @dst: start address of the destination buffer
 * @src: start address of the buffer in a circular queue
 * @qsize: size of the circular queue
 * @out: offset to read
 * @len: length of data to be read
 *
 * Should be invoked after checking data length
 */
void circ_read16_from_io(void *dst, void *src, u32 qsize, u32 out, u32 len)
{
	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		memcpy16_from_io(dst, (src + out), len);
	} else {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		unsigned len1 = qsize - out;

		/* 1) data start (out) ~ buffer end */
		memcpy16_from_io(dst, (src + out), len1);

		/* 2) buffer start ~ data end (in - 1) */
		memcpy16_from_io((dst + len1), src, (len - len1));
	}
}

/**
 * circ_write16_to_io
 * @dst: pointer to the start of the circular queue
 * @src: pointer to the source
 * @qsize: size of the circular queue
 * @in: offset to write
 * @len: length of data to be written
 *
 * Should be invoked after checking free space
 */
void circ_write16_to_io(void *dst, void *src, u32 qsize, u32 in, u32 len)
{
	u32 space;

	if ((in + len) < qsize) {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		memcpy16_to_io((dst + in), src, len);
	} else {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */

		/* 1) space start (in) ~ buffer end */
		space = qsize - in;
		memcpy16_to_io((dst + in), src, ((len > space) ? space : len));

		/* 2) buffer start ~ data end */
		if (len > space)
			memcpy16_to_io(dst, (src + space), (len - space));
	}
}

/**
 * copy_circ_to_user
 * @dst: start address of the destination buffer
 * @src: start address of the buffer in a circular queue
 * @qsize: size of the circular queue
 * @out: offset to read
 * @len: length of data to be read
 *
 * Should be invoked after checking data length
 */
int copy_circ_to_user(void __user *dst, void *src, u32 qsize, u32 out, u32 len)
{
	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		if (copy_to_user(dst, (src + out), len)) {
			mif_err("ERR! <called by %pf> copy_to_user fail\n",
				CALLER);
			return -EFAULT;
		}
	} else {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		unsigned len1 = qsize - out;

		/* 1) data start (out) ~ buffer end */
		if (copy_to_user(dst, (src + out), len1)) {
			mif_err("ERR! <called by %pf> copy_to_user fail\n",
				CALLER);
			return -EFAULT;
		}

		/* 2) buffer start ~ data end (in?) */
		if (copy_to_user((dst + len1), src, (len - len1))) {
			mif_err("ERR! <called by %pf> copy_to_user fail\n",
				CALLER);
			return -EFAULT;
		}
	}

	return 0;
}

/**
 * copy_user_to_circ
 * @dst: pointer to the start of the circular queue
 * @src: pointer to the source
 * @qsize: size of the circular queue
 * @in: offset to write
 * @len: length of data to be written
 *
 * Should be invoked after checking free space
 */
int copy_user_to_circ(void *dst, void __user *src, u32 qsize, u32 in, u32 len)
{
	u32 space;
	u32 len1;

	if ((in + len) < qsize) {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		if (copy_from_user((dst + in), src, len)) {
			mif_err("ERR! <called by %pf> copy_from_user fail\n",
				CALLER);
			return -EFAULT;
		}
	} else {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */

		/* 1) space start (in) ~ buffer end */
		space = qsize - in;
		len1 = (len > space) ? space : len;
		if (copy_from_user((dst + in), src, len1)) {
			mif_err("ERR! <called by %pf> copy_from_user fail\n",
				CALLER);
			return -EFAULT;
		}

		/* 2) buffer start ~ data end */
		if (len > len1) {
			if (copy_from_user(dst, (src + space), (len - len1))) {
				mif_err("ERR! <called by %pf> copy_from_user "
					"fail\n", CALLER);
				return -EFAULT;
			}
		}
	}

	return 0;
}

/**
 * print_mem_status
 * @ld: pointer to an instance of link_device structure
 * @mst: pointer to an instance of mem_status structure
 *
 * Prints a snapshot of the status of a SHM.
 */
void print_mem_status(struct link_device *ld, struct mem_status *mst)
{
	struct utc_time utc;
	int us = ns2us(mst->ts.tv_nsec);

	ts2utc(&mst->ts, &utc);
	pr_info("%s: %s: [%02d:%02d:%02d.%06d] "
		"[%s] ACC{%X %d} "
		"FMT{TI:%u TO:%u RI:%u RO:%u} "
		"RAW{TI:%u TO:%u RI:%u RO:%u} "
		"INTR{RX:0x%X TX:0x%X}\n",
		MIF_TAG, ld->name, utc.hour, utc.min, utc.sec, us,
		get_dir_str(mst->dir), mst->magic, mst->access,
		mst->head[IPC_FMT][TX], mst->tail[IPC_FMT][TX],
		mst->head[IPC_FMT][RX], mst->tail[IPC_FMT][RX],
		mst->head[IPC_RAW][TX], mst->tail[IPC_RAW][TX],
		mst->head[IPC_RAW][RX], mst->tail[IPC_RAW][RX],
		mst->int2ap, mst->int2cp);
}

/**
 * print_circ_status
 * @ld: pointer to an instance of link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 *
 * Prints a snapshot of the status of a memory
 */
void print_circ_status(struct link_device *ld, int dev, struct mem_status *mst)
{
	struct utc_time utc;
	int us = ns2us(mst->ts.tv_nsec);

	if (dev > IPC_RAW)
		return;

	ts2utc(&mst->ts, &utc);
	pr_info("%s: %s: [%02d:%02d:%02d.%06d] "
		"[%s] %s | TXQ{in:%u out:%u} RXQ{in:%u out:%u}\n",
		MIF_TAG, ld->name, utc.hour, utc.min, utc.sec, us,
		get_dir_str(mst->dir), get_dev_name(dev),
		mst->head[dev][TX], mst->tail[dev][TX],
		mst->head[dev][RX], mst->tail[dev][RX]);
}

/**
 * print_ipc_trace
 * @ld: pointer to an instance of link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @stat: pointer to an instance of circ_status structure
 * @ts: pointer to an instance of timespec structure
 * @buff: start address of a buffer into which RX IPC messages were copied
 * @rcvd: size of data in the buffer
 *
 * Prints IPC messages in a local memory buffer to a kernel log.
 */
void print_ipc_trace(struct link_device *ld, int dev, struct circ_status *stat,
			struct timespec *ts, u8 *buff, u32 rcvd)
{
	struct utc_time utc;

	ts2utc(ts, &utc);

	pr_info("%s: [%d-%02d-%02d %02d:%02d:%02d.%03d] "
		"%s %s_RXQ {IN:%u OUT:%u LEN:%d}\n",
		MIF_TAG, utc.year, utc.mon, utc.day, utc.hour, utc.min, utc.sec,
		utc.msec, ld->name, get_dev_name(dev), stat->in, stat->out,
		stat->size);

	mif_print_dump(buff, rcvd, 4);
}

/**
 * capture_mem_dump
 * @ld: pointer to an instance of link_device structure
 * @base: base virtual address to a memory interface medium
 * @size: size of the memory interface medium
 *
 * Captures a dump for a memory interface medium.
 *
 * Returns the pointer to a memory dump buffer.
 */
u8 *capture_mem_dump(struct link_device *ld, u8 *base, u32 size)
{
	u8 *buff = kzalloc(size, GFP_ATOMIC);
	if (!buff) {
		mif_err("%s: ERR! kzalloc(%d) fail\n", ld->name, size);
		return NULL;
	} else {
		memcpy16_from_io(buff, base, size);
		return buff;
	}
}

/**
 * trq_get_free_slot
 * @trq : pointer to an instance of trace_data_queue structure
 *
 * Succeeds always by dropping the oldest slot if a "trq" is full.
 */
struct trace_data *trq_get_free_slot(struct trace_data_queue *trq)
{
	int qsize = MAX_TRACE_SIZE;
	int in;
	int out;
	unsigned long flags;
	struct trace_data *trd;

	spin_lock_irqsave(&trq->lock, flags);

	in = trq->in;
	out = trq->out;

	/* The oldest slot can be dropped. */
	if (circ_get_space(qsize, in, out) < 1) {
		/* Free the data buffer in the oldest slot */
		trd = &trq->trd[out];
		kfree(trd->data);

		/* Make the oldest slot empty */
		out++;
		trq->out = (out == qsize) ? 0 : out;
	}

	/* Get a free slot and make it occupied */
	trd = &trq->trd[in++];
	trq->in = (in == qsize) ? 0 : in;

	spin_unlock_irqrestore(&trq->lock, flags);

	memset(trd, 0, sizeof(struct trace_data));

	return trd;
}

struct trace_data *trq_get_data_slot(struct trace_data_queue *trq)
{
	int qsize = MAX_TRACE_SIZE;
	int in;
	int out;
	unsigned long flags;
	struct trace_data *trd;

	spin_lock_irqsave(&trq->lock, flags);

	in = trq->in;
	out = trq->out;

	if (circ_get_usage(qsize, in, out) < 1) {
		spin_unlock_irqrestore(&trq->lock, flags);
		return NULL;
	}

	/* Get a data slot and make it empty */
	trd = &trq->trd[out++];
	trq->out = (out == qsize) ? 0 : out;

	spin_unlock_irqrestore(&trq->lock, flags);

	return trd;
}

