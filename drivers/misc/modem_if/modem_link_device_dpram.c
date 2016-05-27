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

/*
** Function prototypes for basic DPRAM operations
*/
static inline void clear_intr(struct dpram_link_device *dpld);
static inline u16 recv_intr(struct dpram_link_device *dpld);
static inline void send_intr(struct dpram_link_device *dpld, u16 mask);

static inline u16 get_magic(struct dpram_link_device *dpld);
static inline void set_magic(struct dpram_link_device *dpld, u16 val);
static inline u16 get_access(struct dpram_link_device *dpld);
static inline void set_access(struct dpram_link_device *dpld, u16 val);

static inline u32 get_tx_head(struct dpram_link_device *dpld, int id);
static inline u32 get_tx_tail(struct dpram_link_device *dpld, int id);
static inline void set_tx_head(struct dpram_link_device *dpld, int id, u32 in);
static inline void set_tx_tail(struct dpram_link_device *dpld, int id, u32 out);
static inline u8 *get_tx_buff(struct dpram_link_device *dpld, int id);
static inline u32 get_tx_buff_size(struct dpram_link_device *dpld, int id);

static inline u32 get_rx_head(struct dpram_link_device *dpld, int id);
static inline u32 get_rx_tail(struct dpram_link_device *dpld, int id);
static inline void set_rx_head(struct dpram_link_device *dpld, int id, u32 in);
static inline void set_rx_tail(struct dpram_link_device *dpld, int id, u32 out);
static inline u8 *get_rx_buff(struct dpram_link_device *dpld, int id);
static inline u32 get_rx_buff_size(struct dpram_link_device *dpld, int id);

static inline u16 get_mask_req_ack(struct dpram_link_device *dpld, int id);
static inline u16 get_mask_res_ack(struct dpram_link_device *dpld, int id);
static inline u16 get_mask_send(struct dpram_link_device *dpld, int id);

static inline bool dpram_circ_valid(u32 size, u32 in, u32 out);

static void handle_cp_crash(struct dpram_link_device *dpld);
static int trigger_force_cp_crash(struct dpram_link_device *dpld);
static void dpram_dump_memory(struct link_device *ld, char *buff);

/*
** Functions for debugging
*/
static inline void log_dpram_status(struct dpram_link_device *dpld)
{
	pr_info("mif: %s: {M:0x%X A:%d} {FMT TI:%u TO:%u RI:%u RO:%u} "
		"{RAW TI:%u TO:%u RI:%u RO:%u} {INT:0x%X}\n",
		dpld->ld.mc->name,
		get_magic(dpld), get_access(dpld),
		get_tx_head(dpld, IPC_FMT), get_tx_tail(dpld, IPC_FMT),
		get_rx_head(dpld, IPC_FMT), get_rx_tail(dpld, IPC_FMT),
		get_tx_head(dpld, IPC_RAW), get_tx_tail(dpld, IPC_RAW),
		get_rx_head(dpld, IPC_RAW), get_rx_tail(dpld, IPC_RAW),
		recv_intr(dpld));
}

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

/*
** RXB (DPRAM RX buffer) functions
*/
static struct dpram_rxb *rxbq_create_pool(unsigned size, int count)
{
	struct dpram_rxb *rxb;
	u8 *buff;
	int i;

	rxb = kzalloc(sizeof(struct dpram_rxb) * count, GFP_KERNEL);
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

static inline bool rxbq_empty(struct dpram_rxb_queue *rxbq)
{
	return (rxbq->in == rxbq->out) ? true : false;
}

static inline int rxbq_free_size(struct dpram_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline struct dpram_rxb *rxbq_get_free_rxb(struct dpram_rxb_queue *rxbq)
{
	struct dpram_rxb *rxb = NULL;

	if (likely(rxbq_free_size(rxbq) > 0)) {
		rxb = &rxbq->rxb[rxbq->in];
		rxbq->in++;
		if (rxbq->in >= rxbq->size)
			rxbq->in -= rxbq->size;
		rxb->data = rxb->buff;
	}

	return rxb;
}

static inline int rxbq_size(struct dpram_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in >= out) ? (in - out) : (qsize - out + in);
}

static inline struct dpram_rxb *rxbq_get_data_rxb(struct dpram_rxb_queue *rxbq)
{
	struct dpram_rxb *rxb = NULL;

	if (likely(!rxbq_empty(rxbq))) {
		rxb = &rxbq->rxb[rxbq->out];
		rxbq->out++;
		if (rxbq->out >= rxbq->size)
			rxbq->out -= rxbq->size;
	}

	return rxb;
}

static inline u8 *rxb_put(struct dpram_rxb *rxb, unsigned len)
{
	rxb->len = len;
	return rxb->data;
}

static inline void rxb_clear(struct dpram_rxb *rxb)
{
	rxb->data = NULL;
	rxb->len = 0;
}

/*
** DPRAM operations
*/
static int dpram_register_isr(unsigned irq, irqreturn_t (*isr)(int, void*),
				unsigned long flag, const char *name,
				struct dpram_link_device *dpld)
{
	int ret;

	ret = request_irq(irq, isr, flag, name, dpld);
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

static inline void clear_intr(struct dpram_link_device *dpld)
{
	if (likely(dpld->dpctl->clear_intr))
		dpld->dpctl->clear_intr();
}

static inline u16 recv_intr(struct dpram_link_device *dpld)
{
	if (likely(dpld->dpctl->recv_intr))
		return dpld->dpctl->recv_intr();
	else
		return ioread16(dpld->mbx2ap);
}

static inline void send_intr(struct dpram_link_device *dpld, u16 mask)
{
	if (likely(dpld->dpctl->send_intr))
		dpld->dpctl->send_intr(mask);
	else
		iowrite16(mask, dpld->mbx2cp);
}

static inline u16 get_magic(struct dpram_link_device *dpld)
{
	return ioread16(dpld->magic);
}

static inline void set_magic(struct dpram_link_device *dpld, u16 val)
{
	iowrite16(val, dpld->magic);
}

static inline u16 get_access(struct dpram_link_device *dpld)
{
	return ioread16(dpld->access);
}

static inline void set_access(struct dpram_link_device *dpld, u16 val)
{
	iowrite16(val, dpld->access);
}

static inline u32 get_tx_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.head);
}

static inline u32 get_tx_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->txq.tail);
}

static inline void set_tx_head(struct dpram_link_device *dpld, int id, u32 in)
{
	int cnt = 3;
	u32 val = 0;

	iowrite16((u16)in, dpld->dev[id]->txq.head);

	do {
		/* Check head value written */
		val = ioread16(dpld->dev[id]->txq.head);
		if (likely(val == in))
			return;

		mif_err("ERR: %s txq.head(%d) != in(%d)\n",
			get_dev_name(id), val, in);
		udelay(100);

		/* Write head value again */
		iowrite16((u16)in, dpld->dev[id]->txq.head);
	} while (cnt--);

	trigger_force_cp_crash(dpld);
}

static inline void set_tx_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	int cnt = 3;
	u32 val = 0;

	iowrite16((u16)out, dpld->dev[id]->txq.tail);

	do {
		/* Check tail value written */
		val = ioread16(dpld->dev[id]->txq.tail);
		if (likely(val == out))
			return;

		mif_err("ERR: %s txq.tail(%d) != out(%d)\n",
			get_dev_name(id), val, out);
		udelay(100);

		/* Write tail value again */
		iowrite16((u16)out, dpld->dev[id]->txq.tail);
	} while (cnt--);

	trigger_force_cp_crash(dpld);
}

static inline u8 *get_tx_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.buff;
}

static inline u32 get_tx_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->txq.size;
}

static inline u32 get_rx_head(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.head);
}

static inline u32 get_rx_tail(struct dpram_link_device *dpld, int id)
{
	return ioread16(dpld->dev[id]->rxq.tail);
}

static inline void set_rx_head(struct dpram_link_device *dpld, int id, u32 in)
{
	int cnt = 3;
	u32 val = 0;

	iowrite16((u16)in, dpld->dev[id]->rxq.head);

	do {
		/* Check head value written */
		val = ioread16(dpld->dev[id]->rxq.head);
		if (val == in)
			return;

		mif_err("ERR: %s rxq.head(%d) != in(%d)\n",
			get_dev_name(id), val, in);
		udelay(100);

		/* Write head value again */
		iowrite16((u16)in, dpld->dev[id]->rxq.head);
	} while (cnt--);

	trigger_force_cp_crash(dpld);
}

static inline void set_rx_tail(struct dpram_link_device *dpld, int id, u32 out)
{
	int cnt = 3;
	u32 val = 0;

	iowrite16((u16)out, dpld->dev[id]->rxq.tail);

	do {
		/* Check tail value written */
		val = ioread16(dpld->dev[id]->rxq.tail);
		if (val == out)
			return;

		mif_err("ERR: %s rxq.tail(%d) != out(%d)\n",
			get_dev_name(id), val, out);
		udelay(100);

		/* Write tail value again */
		iowrite16((u16)out, dpld->dev[id]->rxq.tail);
	} while (cnt--);

	trigger_force_cp_crash(dpld);
}

static inline u8 *get_rx_buff(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.buff;
}

static inline u32 get_rx_buff_size(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->rxq.size;
}

static inline u16 get_mask_req_ack(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_req_ack;
}

static inline u16 get_mask_res_ack(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_res_ack;
}

static inline u16 get_mask_send(struct dpram_link_device *dpld, int id)
{
	return dpld->dev[id]->mask_send;
}

static inline bool dpram_circ_valid(u32 size, u32 in, u32 out)
{
	if (in >= size)
		return false;

	if (out >= size)
		return false;

	return true;
}

/* Get free space in the TXQ as well as in & out pointers */
static inline int dpram_get_txq_space(struct dpram_link_device *dpld, int dev,
		u32 qsize, u32 *in, u32 *out)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 3;
	u32 head;
	u32 tail;
	int space;

	do {
		head = get_tx_head(dpld, dev);
		tail = get_tx_tail(dpld, dev);

		space = (head < tail) ? (tail - head - 1) :
			(qsize + tail - head - 1);
		mif_debug("%s: %s_TXQ qsize[%u] in[%u] out[%u] space[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, space);

		if (dpram_circ_valid(qsize, head, tail)) {
			*in = head;
			*out = tail;
			return space;
		}

		mif_info("%s: CAUTION! <%pf> "
			"%s_TXQ invalid (size:%d in:%d out:%d)\n",
			ld->name, __builtin_return_address(0),
			get_dev_name(dev), qsize, head, tail);

		udelay(100);
	} while (cnt--);

	*in = 0;
	*out = 0;
	return -EINVAL;
}

static void dpram_reset_tx_circ(struct dpram_link_device *dpld, int dev)
{
	set_tx_head(dpld, dev, 0);
	set_tx_tail(dpld, dev, 0);
	if (dev == IPC_FMT)
		trigger_force_cp_crash(dpld);
}

/* Get data size in the RXQ as well as in & out pointers */
static inline int dpram_get_rxq_rcvd(struct dpram_link_device *dpld, int dev,
		u32 qsize, u32 *in, u32 *out)
{
	struct link_device *ld = &dpld->ld;
	int cnt = 3;
	u32 head;
	u32 tail;
	u32 rcvd;

	do {
		head = get_rx_head(dpld, dev);
		tail = get_rx_tail(dpld, dev);
		if (head == tail) {
			*in = head;
			*out = tail;
			return 0;
		}

		rcvd = (head > tail) ? (head - tail) : (qsize - tail + head);
		mif_debug("%s: %s_RXQ qsize[%u] in[%u] out[%u] rcvd[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, rcvd);

		if (dpram_circ_valid(qsize, head, tail)) {
			*in = head;
			*out = tail;
			return rcvd;
		}

		mif_info("%s: CAUTION! <%pf> "
			"%s_RXQ invalid (size:%d in:%d out:%d)\n",
			ld->name, __builtin_return_address(0),
			get_dev_name(dev), qsize, head, tail);

		udelay(100);
	} while (cnt--);

	*in = 0;
	*out = 0;
	return -EINVAL;
}

static void dpram_reset_rx_circ(struct dpram_link_device *dpld, int dev)
{
	set_rx_head(dpld, dev, 0);
	set_rx_tail(dpld, dev, 0);
	if (dev == IPC_FMT)
		trigger_force_cp_crash(dpld);
}

/*
** CAUTION : dpram_allow_sleep() MUST be invoked after dpram_wake_up() success
*/
static int dpram_wake_up(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (!dpld->dpctl->wakeup)
		return 0;

	if (dpld->dpctl->wakeup() < 0) {
		mif_err("%s: ERR! <%pf> DPRAM wakeup fail once\n",
			ld->name, __builtin_return_address(0));

		if (dpld->dpctl->sleep)
			dpld->dpctl->sleep();

		udelay(10);

		if (dpld->dpctl->wakeup() < 0) {
			mif_err("%s: ERR! <%pf> DPRAM wakeup fail twice\n",
				ld->name, __builtin_return_address(0));
			return -EACCES;
		}
	}

	atomic_inc(&dpld->accessing);
	return 0;
}

static void dpram_allow_sleep(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (!dpld->dpctl->sleep)
		return;

	if (atomic_dec_return(&dpld->accessing) <= 0) {
		dpld->dpctl->sleep();
		atomic_set(&dpld->accessing, 0);
		mif_debug("%s: DPRAM sleep possible\n", ld->name);
	}
}

static int dpram_check_access(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int i;
	u16 magic = get_magic(dpld);
	u16 access = get_access(dpld);

	if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
		return 0;

	for (i = 1; i <= 100; i++) {
		mif_info("%s: ERR! magic:%X access:%X -> retry:%d\n",
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

static bool dpram_ipc_active(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	/* Check DPRAM mode */
	if (ld->mode != LINK_MODE_IPC) {
		mif_info("%s: <%pf> ld->mode != LINK_MODE_IPC\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	if (dpram_check_access(dpld) < 0) {
		mif_info("%s: ERR! <%pf> dpram_check_access fail\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	return true;
}

static void dpram_ipc_write(struct dpram_link_device *dpld, int dev,
		u32 qsize, u32 in, u32 out, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *buff = get_tx_buff(dpld, dev);
	u8 *src = skb->data;
	u32 len = skb->len;
	u32 inp;
	struct mif_irq_map map;

	if (in < out) {
		/* +++++++++ in ---------- out ++++++++++ */
		memcpy((buff + in), src, len);
	} else {
		/* ------ out +++++++++++ in ------------ */
		u32 space = qsize - in;

		/* 1) in -> buffer end */
		memcpy((buff + in), src, ((len > space) ? space : len));

		/* 2) buffer start -> out */
		if (len > space)
			memcpy(buff, (src + space), (len - space));
	}

	/* update new in pointer */
	inp = in + len;
	if (inp >= qsize)
		inp -= qsize;
	set_tx_head(dpld, dev, inp);

	if (dev == IPC_FMT) {
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_write", sizeof("ipc_write"));
		mif_ipc_log(MIF_IPC_AP2CP, ld->mc->msd, skb->data, skb->len);
	}

	if (ld->aligned && memcmp16_to_io((buff + in), src, 4)) {
		mif_err("%s: memcmp16_to_io fail\n", ld->name);
		trigger_force_cp_crash(dpld);
	}
}

static int dpram_try_ipc_tx(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	unsigned long int flags;
	u32 qsize = get_tx_buff_size(dpld, dev);
	u32 in;
	u32 out;
	int space;
	int copied = 0;

	spin_lock_irqsave(&dpld->tx_lock[dev], flags);

	while (1) {
		space = dpram_get_txq_space(dpld, dev, qsize, &in, &out);
		if (unlikely(space < 0)) {
			spin_unlock_irqrestore(&dpld->tx_lock[dev], flags);
			dpram_reset_tx_circ(dpld, dev);
			return space;
		}

		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		if (unlikely(space < skb->len)) {
			atomic_set(&dpld->res_required[dev], 1);
			skb_queue_head(txq, skb);
			spin_unlock_irqrestore(&dpld->tx_lock[dev], flags);
			mif_info("%s: %s "
				"qsize[%u] in[%u] out[%u] free[%u] < len[%u]\n",
				ld->name, get_dev_name(dev),
				qsize, in, out, space, skb->len);
			return -ENOSPC;
		}

		/* TX if there is enough room in the queue */
		dpram_ipc_write(dpld, dev, qsize, in, out, skb);
		copied += skb->len;
		dev_kfree_skb_any(skb);
	}

	spin_unlock_irqrestore(&dpld->tx_lock[dev], flags);

	return copied;
}

static void dpram_ipc_rx_task(unsigned long data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;
	struct dpram_rxb *rxb;
	unsigned qlen;
	int i;

	for (i = 0; i < dpld->max_ipc_dev; i++) {
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

static void dpram_ipc_read(struct dpram_link_device *dpld, int dev, u8 *dst,
	u8 __iomem *src, u32 out, u32 len, u32 qsize)
{
	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		memcpy(dst, (src + out), len);
	} else {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		unsigned len1 = qsize - out;

		/* 1) out -> buffer end */
		memcpy(dst, (src + out), len1);

		/* 2) buffer start -> in */
		dst += len1;
		memcpy(dst, src, (len - len1));
	}
}

/*
  ret < 0  : error
  ret == 0 : no data
  ret > 0  : valid data
*/
static int dpram_ipc_recv_data_with_rxb(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct dpram_rxb *rxb;
	u8 __iomem *src = get_rx_buff(dpld, dev);
	u32 qsize = get_rx_buff_size(dpld, dev);
	u32 in;
	u32 out;
	u32 rcvd;
	struct mif_irq_map map;

	rcvd = dpram_get_rxq_rcvd(dpld, dev, qsize, &in, &out);
	if (rcvd <= 0)
		return rcvd;

	if (dev == IPC_FMT) {
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

	/* Allocate an rxb */
	rxb = rxbq_get_free_rxb(&dpld->rxbq[dev]);
	if (!rxb) {
		mif_info("%s: ERR! %s rxbq_get_free_rxb fail\n",
			ld->name, get_dev_name(dev));
		return -ENOMEM;
	}

	/* Read data from each DPRAM buffer */
	dpram_ipc_read(dpld, dev, rxb_put(rxb, rcvd), src, out, rcvd, qsize);

	/* Calculate and set new out */
	out += rcvd;
	if (out >= qsize)
		out -= qsize;
	set_rx_tail(dpld, dev, out);

	return rcvd;
}

/*
  ret < 0  : error
  ret == 0 : no data
  ret > 0  : valid data
*/
static int dpram_ipc_recv_data_with_skb(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = dpld->iod[dev];
	struct sk_buff *skb;
	u8 __iomem *src = get_rx_buff(dpld, dev);
	u32 qsize = get_rx_buff_size(dpld, dev);
	u32 in;
	u32 out;
	u32 rcvd;
	int rest;
	u8 *frm;
	u8 *dst;
	unsigned int len;
	unsigned int pad;
	unsigned int tot;
	struct mif_irq_map map;

	rcvd = dpram_get_rxq_rcvd(dpld, dev, qsize, &in, &out);
	if (rcvd <= 0)
		return rcvd;

	if (dev == IPC_FMT) {
		set_dpram_map(dpld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

	rest = rcvd;
	while (rest > 0) {
		frm = src + out;
		if (unlikely(!sipc5_start_valid(frm[0]))) {
			mif_err("%s: ERR! %s invalid start 0x%02X\n",
				ld->name, get_dev_name(dev), frm[0]);
			skb_queue_purge(&dpld->skb_rxq[dev]);
			return -EBADMSG;
		}

		len = sipc5_get_frame_sz16(frm);
		if (unlikely(len > rest)) {
			mif_err("%s: ERR! %s len %d > rest %d\n",
				ld->name, get_dev_name(dev), len, rest);
			skb_queue_purge(&dpld->skb_rxq[dev]);
			return -EBADMSG;
		}

		pad = sipc5_calc_padding_size(len);
		tot = len + pad;

		/* Allocate an skb */
		skb = dev_alloc_skb(tot);
		if (!skb) {
			mif_err("%s: ERR! %s dev_alloc_skb fail\n",
				ld->name, get_dev_name(dev));
			return -ENOMEM;
		}

		/* Read data from each DPRAM buffer */
		dst = skb_put(skb, tot);
		dpram_ipc_read(dpld, dev, dst, src, out, tot, qsize);
		skb_trim(skb, len);
		iod->recv_skb(iod, ld, skb);

		/* Calculate and set new out */
		rest -= tot;
		out += tot;
		if (out >= qsize)
			out -= qsize;
	}

	set_rx_tail(dpld, dev, out);

	return rcvd;
}

static void non_command_handler(struct dpram_link_device *dpld, u16 intr)
{
	struct link_device *ld = &dpld->ld;
	int i = 0;
	int ret = 0;
	u16 tx_mask = 0;

	if (!dpram_ipc_active(dpld))
		return;

	/* Read data from DPRAM */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		if (dpld->use_skb)
			ret = dpram_ipc_recv_data_with_skb(dpld, i);
		else
			ret = dpram_ipc_recv_data_with_rxb(dpld, i);
		if (ret < 0)
			dpram_reset_rx_circ(dpld, i);

		/* Check and process REQ_ACK (at this time, in == out) */
		if (intr & get_mask_req_ack(dpld, i)) {
			mif_debug("%s: send %s_RES_ACK\n",
				ld->name, get_dev_name(i));
			tx_mask |= get_mask_res_ack(dpld, i);
		}
	}

	if (!dpld->use_skb) {
		/* Schedule soft IRQ for RX */
		tasklet_hi_schedule(&dpld->rx_tsk);
	}

	/* Try TX via DPRAM */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		if (atomic_read(&dpld->res_required[i]) > 0) {
			ret = dpram_try_ipc_tx(dpld, i);
			if (ret > 0) {
				atomic_set(&dpld->res_required[i], 0);
				tx_mask |= get_mask_send(dpld, i);
			} else if (ret == -ENOSPC) {
				tx_mask |= get_mask_req_ack(dpld, i);
			}
		}
	}

	if (tx_mask) {
		send_intr(dpld, INT_NON_CMD(tx_mask));
		mif_debug("%s: send intr 0x%04X\n", ld->name, tx_mask);
	}
}

static void handle_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;
	int i;

	for (i = 0; i < dpld->max_ipc_dev; i++) {
		mif_info("%s: purging %s_skb_txq\b", ld->name, get_dev_name(i));
		skb_queue_purge(ld->skb_txq[i]);
	}

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_channel(ld, PS_DATA_CH_0);
	if (iod)
		iodevs_for_each(iod->msd, iodev_netif_stop, 0);
}

static void handle_no_crash_ack(unsigned long arg)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)arg;
	struct link_device *ld = &dpld->ld;

	mif_err("%s: ERR! No CRASH_EXIT ACK from CP\n", ld->mc->name);

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	handle_cp_crash(dpld);
}

static int trigger_force_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	if (ld->mode == LINK_MODE_ULOAD) {
		mif_err("%s: CP crash is already in progress\n", ld->mc->name);
		return 0;
	}

	ld->mode = LINK_MODE_ULOAD;
	mif_err("%s: called by %pf\n", ld->name, __builtin_return_address(0));

	if (dpld->dp_type == CP_IDPRAM)
		dpram_wake_up(dpld);

	send_intr(dpld, INT_CMD(INT_CMD_CRASH_EXIT));

	mif_add_timer(&dpld->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			handle_no_crash_ack, (unsigned long)dpld);

	return 0;
}

static int dpram_init_ipc(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int i;

	if (ld->mode == LINK_MODE_IPC &&
	    get_magic(dpld) == DPRAM_MAGIC_CODE &&
	    get_access(dpld) == 1)
		mif_info("%s: IPC already initialized\n", ld->name);

	/* Clear pointers in every circular queue */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		set_tx_head(dpld, i, 0);
		set_tx_tail(dpld, i, 0);
		set_rx_head(dpld, i, 0);
		set_rx_tail(dpld, i, 0);
	}

	/* Initialize variables for efficient TX/RX processing */
	for (i = 0; i < dpld->max_ipc_dev; i++)
		dpld->iod[i] = link_get_iod_with_format(ld, i);
	dpld->iod[IPC_RAW] = link_get_iod_with_format(ld, IPC_MULTI_RAW);

	if (dpld->iod[IPC_RAW]->recv_skb)
		dpld->use_skb = true;

	for (i = 0; i < dpld->max_ipc_dev; i++) {
		spin_lock_init(&dpld->tx_lock[i]);
		atomic_set(&dpld->res_required[i], 0);
		skb_queue_purge(&dpld->skb_rxq[i]);
	}

	/* Enable IPC */
	atomic_set(&dpld->accessing, 0);

	set_magic(dpld, DPRAM_MAGIC_CODE);
	set_access(dpld, 1);
	if (get_magic(dpld) != DPRAM_MAGIC_CODE || get_access(dpld) != 1)
		return -EACCES;

	ld->mode = LINK_MODE_IPC;

	if (wake_lock_active(&dpld->wlock))
		wake_unlock(&dpld->wlock);

	return 0;
}

static void cmd_req_active_handler(struct dpram_link_device *dpld)
{
	send_intr(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
}

static void cmd_crash_reset_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = NULL;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	mif_err("%s: Recv 0xC7 (CRASH_RESET)\n", ld->name);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);
}

static void cmd_crash_exit_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	u32 size = dpld->dpctl->dp_size;
	char *dpram_buff = NULL;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&dpld->wlock))
		wake_lock(&dpld->wlock);

	mif_err("%s: Recv 0xC9 (CRASH_EXIT)\n", ld->name);

	if (dpld->dp_type == CP_IDPRAM)
		dpram_wake_up(dpld);

	dpram_buff = kzalloc(size + (MAX_MIF_SEPA_SIZE * 2), GFP_ATOMIC);
	if (!dpram_buff) {
		mif_err("DPRAM dump failed!!\n");
	} else {
		memset(dpram_buff, 0, size + (MAX_MIF_SEPA_SIZE * 2));
		memcpy(dpram_buff, MIF_SEPARATOR_DPRAM, MAX_MIF_SEPA_SIZE);
		memcpy(dpram_buff + MAX_MIF_SEPA_SIZE, &size, sizeof(u32));
		dpram_buff += (MAX_MIF_SEPA_SIZE * 2);
		dpram_dump_memory(ld, dpram_buff);
	}

	del_timer(&dpld->crash_ack_timer);

	if (dpld->ext_op && dpld->ext_op->crash_log)
		dpld->ext_op->crash_log(dpld);

	handle_cp_crash(dpld);
}

static void cmd_phone_start_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = NULL;

	mif_info("%s: Recv 0xC8 (CP_START)\n", ld->name);

	dpram_init_ipc(dpld);

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

static void command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		cmd_req_active_handler(dpld);
		break;

	case INT_CMD_CRASH_RESET:
		dpld->dpram_init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_reset_handler(dpld);
		break;

	case INT_CMD_CRASH_EXIT:
		dpld->dpram_init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_exit_handler(dpld);
		break;

	case INT_CMD_PHONE_START:
		dpld->dpram_init_status = DPRAM_INIT_STATE_READY;
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

static inline void dpram_ipc_rx(struct dpram_link_device *dpld, u16 intr)
{
	if (unlikely(INT_CMD_VALID(intr)))
		command_handler(dpld, intr);
	else
		non_command_handler(dpld, intr);
}

static inline void dpram_intr_handler(struct dpram_link_device *dpld, u16 intr)
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
		dpram_ipc_rx(dpld, intr);
	else
		mif_info("%s: ERR! invalid intr 0x%04X\n", name, intr);
}

static irqreturn_t ap_idpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	u16 int2ap = recv_intr(dpld);

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	dpram_intr_handler(dpld, int2ap);

	return IRQ_HANDLED;
}

static irqreturn_t cp_idpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	u16 int2ap;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	if (dpram_wake_up(dpld) < 0) {
		log_dpram_status(dpld);
		trigger_force_cp_crash(dpld);
		return IRQ_HANDLED;
	}

	int2ap = recv_intr(dpld);

	dpram_intr_handler(dpld, int2ap);

	clear_intr(dpld);

	dpram_allow_sleep(dpld);

	return IRQ_HANDLED;
}

static irqreturn_t ext_dpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	u16 int2ap = recv_intr(dpld);

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	dpram_intr_handler(dpld, int2ap);

	return IRQ_HANDLED;
}

static void dpram_send_ipc(struct link_device *ld, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct sk_buff_head *txq = ld->skb_txq[dev];
	int ret;
	u16 mask;

	skb_queue_tail(txq, skb);
	if (txq->qlen > 1024) {
		mif_debug("%s: %s txq->qlen %d > 1024\n",
			ld->name, get_dev_name(dev), txq->qlen);
	}

	if (dpld->dp_type == CP_IDPRAM) {
		if (dpram_wake_up(dpld) < 0) {
			trigger_force_cp_crash(dpld);
			return;
		}
	}

	if (!dpram_ipc_active(dpld))
		goto exit;

	if (atomic_read(&dpld->res_required[dev]) > 0) {
		mif_debug("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		goto exit;
	}

	ret = dpram_try_ipc_tx(dpld, dev);
	if (ret > 0) {
		mask = get_mask_send(dpld, dev);
		send_intr(dpld, INT_NON_CMD(mask));
	} else if (ret == -ENOSPC) {
		mask = get_mask_req_ack(dpld, dev);
		send_intr(dpld, INT_NON_CMD(mask));
		mif_info("%s: Send REQ_ACK 0x%04X\n", ld->name, mask);
	} else {
		mif_info("%s: dpram_try_ipc_tx fail (err %d)\n", ld->name, ret);
	}

exit:
	if (dpld->dp_type == CP_IDPRAM)
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

static int dpram_send(struct link_device *ld, struct io_device *iod,
		struct sk_buff *skb)
{
	enum dev_format dev = iod->format;
	int len = skb->len;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		if (likely(ld->mode == LINK_MODE_IPC)) {
			dpram_send_ipc(ld, dev, iod, skb);
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

static void dpram_dump_memory(struct link_device *ld, char *buff)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u8 __iomem *base = dpld->dpctl->dp_base;
	u32 size = dpld->dpctl->dp_size;

	if (dpld->dp_type == CP_IDPRAM)
		dpram_wake_up(dpld);

	memcpy(buff, base, size);
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
		return dpld->dpram_init_status;

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

static void dpram_remap_std_16k_region(struct dpram_link_device *dpld)
{
	struct dpram_ipc_16k_map *dpram_map;
	struct dpram_ipc_device *dev;

	dpram_map = (struct dpram_ipc_16k_map *)dpld->dp_base;

	/* magic code and access enable fields */
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

	if (!dpld->dp_base) {
		mif_info("%s: ERR! dpld->dp_base == NULL\n", ld->name);
		return -EINVAL;
	}
	dp_base = dpld->dp_base;

	/* Map for IPC */
	if (dpld->dpctl->ipc_map) {
		memcpy(&dpld->ipc_map, dpld->dpctl->ipc_map,
			sizeof(struct dpram_ipc_map));
	} else {
		if (dpld->dp_size == DPRAM_SIZE_16KB)
			dpram_remap_std_16k_region(dpld);
		else
			return -EINVAL;
	}

	dpld->magic = dpld->ipc_map.magic;
	dpld->access = dpld->ipc_map.access;
	for (i = 0; i < dpld->max_ipc_dev; i++)
		dpld->dev[i] = &dpld->ipc_map.dev[i];
	dpld->mbx2ap = dpld->ipc_map.mbx_cp2ap;
	dpld->mbx2cp = dpld->ipc_map.mbx_ap2cp;

	/* Map for booting */
	if (dpld->ext_op && dpld->ext_op->init_boot_map) {
		dpld->ext_op->init_boot_map(dpld);
	} else {
		dpld->bt_map.magic = (u32 *)(dp_base);
		dpld->bt_map.buff = (u8 *)(dp_base + DP_BOOT_BUFF_OFFSET);
		dpld->bt_map.size = dpld->dp_size - 8;
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

	return 0;
}

static void dpram_setup_common_op(struct dpram_link_device *dpld)
{
	dpld->clear_intr = clear_intr;
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
	dpld->ipc_rx_handler = dpram_ipc_rx;
}

static int dpram_link_init(struct link_device *ld, struct io_device *iod)
{
	return 0;
}

static void dpram_link_terminate(struct link_device *ld, struct io_device *iod)
{
	return;
}

struct link_device *dpram_create_link_device(struct platform_device *pdev)
{
	struct modem_data *mdm_data = NULL;
	struct dpram_link_device *dpld = NULL;
	struct link_device *ld = NULL;
	struct resource *res = NULL;
	resource_size_t res_size;
	struct modemlink_dpram_control *dpctl = NULL;
	unsigned long task_data;
	int ret = 0;
	int i = 0;
	int bsize;
	int qsize;

	/* Get the platform data */
	mdm_data = (struct modem_data *)pdev->dev.platform_data;
	if (!mdm_data) {
		mif_info("ERR! mdm_data == NULL\n");
		goto err;
	}
	mif_info("modem = %s\n", mdm_data->name);
	mif_info("link device = %s\n", mdm_data->link_name);

	if (!mdm_data->dpram_ctl) {
		mif_info("ERR! mdm_data->dpram_ctl == NULL\n");
		goto err;
	}
	dpctl = mdm_data->dpram_ctl;

	/* Alloc DPRAM link device structure */
	dpld = kzalloc(sizeof(struct dpram_link_device), GFP_KERNEL);
	if (!dpld) {
		mif_info("ERR! kzalloc dpld fail\n");
		goto err;
	}
	ld = &dpld->ld;

	/* Retrieve modem data and DPRAM control data from the modem data */
	ld->mdm_data = mdm_data;
	ld->name = mdm_data->link_name;
	ld->ipc_version = mdm_data->ipc_version;

	/* Retrieve the most basic data for IPC from the modem data */
	dpld->dpctl = dpctl;
	dpld->dp_type = dpctl->dp_type;

	if (mdm_data->ipc_version < SIPC_VER_50) {
		if (!dpctl->max_ipc_dev) {
			mif_info("ERR! no max_ipc_dev\n");
			goto err;
		}

		ld->aligned = dpctl->aligned;
		dpld->max_ipc_dev = dpctl->max_ipc_dev;
	} else {
		ld->aligned = 1;
		dpld->max_ipc_dev = MAX_SIPC5_DEV;
	}

	/* Set attributes as a link device */
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

	/* Set up function pointers */
	dpram_setup_common_op(dpld);
	dpld->dpram_dump = dpram_dump_memory;
	dpld->ext_op = dpram_get_ext_op(mdm_data->modem_type);
	if (dpld->ext_op && dpld->ext_op->ioctl)
		dpld->ext_ioctl = dpld->ext_op->ioctl;

	/* Retrieve DPRAM resource */
	if (!dpctl->dp_base) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			mif_info("%s: ERR! platform_get_resource fail\n",
				ld->name);
			goto err;
		}
		res_size = resource_size(res);

		dpctl->dp_base = ioremap_nocache(res->start, res_size);
		dpctl->dp_size = res_size;
	}
	dpld->dp_base = dpctl->dp_base;
	dpld->dp_size = dpctl->dp_size;

	mif_info("%s: dp_type %d, aligned %d, dp_base 0x%08X, dp_size %d\n",
		ld->name, dpld->dp_type, ld->aligned, (int)dpld->dp_base,
		dpld->dp_size);

	/* Initialize DPRAM map (physical map -> logical map) */
	ret = dpram_table_init(dpld);
	if (ret < 0) {
		mif_info("%s: ERR! dpram_table_init fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	/* Disable IPC */
	set_magic(dpld, 0);
	set_access(dpld, 0);
	dpld->dpram_init_status = DPRAM_INIT_STATE_NONE;

	/* Initialize locks, completions, and bottom halves */
	snprintf(dpld->wlock_name, DP_MAX_NAME_LEN, "%s_wlock", ld->name);
	wake_lock_init(&dpld->wlock, WAKE_LOCK_SUSPEND, dpld->wlock_name);

	init_completion(&dpld->dpram_init_cmd);
	init_completion(&dpld->modem_pif_init_done);
	init_completion(&dpld->udl_start_complete);
	init_completion(&dpld->udl_cmd_complete);
	init_completion(&dpld->dump_start_complete);
	init_completion(&dpld->dump_recv_done);

	task_data = (unsigned long)dpld;
	tasklet_init(&dpld->rx_tsk, dpram_ipc_rx_task, task_data);

	/* Prepare SKB queue head for RX processing */
	for (i = 0; i < dpld->max_ipc_dev; i++)
		skb_queue_head_init(&dpld->skb_rxq[i]);

	/* Prepare RXB queue */
	qsize = DPRAM_MAX_RXBQ_SIZE;
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		bsize = rxbq_get_page_size(get_rx_buff_size(dpld, i));
		dpld->rxbq[i].size = qsize;
		dpld->rxbq[i].in = 0;
		dpld->rxbq[i].out = 0;
		dpld->rxbq[i].rxb = rxbq_create_pool(bsize, qsize);
		if (!dpld->rxbq[i].rxb) {
			mif_info("%s: ERR! %s rxbq_create_pool fail\n",
				ld->name, get_dev_name(i));
			goto err;
		}
		mif_info("%s: %s rxbq_pool created (bsize:%d, qsize:%d)\n",
			ld->name, get_dev_name(i), bsize, qsize);
	}

	/* Prepare a multi-purpose miscellaneous buffer */
	dpld->buff = kzalloc(dpld->dp_size, GFP_KERNEL);
	if (!dpld->buff) {
		mif_info("%s: ERR! kzalloc dpld->buff fail\n", ld->name);
		goto err;
	}

	/* Retrieve DPRAM IRQ GPIO# */
	dpld->gpio_dpram_int = mdm_data->gpio_dpram_int;

	/* Retrieve DPRAM IRQ# */
	if (!dpctl->dpram_irq) {
		dpctl->dpram_irq = platform_get_irq_byname(pdev, "dpram_irq");
		if (dpctl->dpram_irq < 0) {
			mif_info("%s: ERR! platform_get_irq_byname fail\n",
				ld->name);
			goto err;
		}
	}
	dpld->irq = dpctl->dpram_irq;

	/* Retrieve DPRAM IRQ flags */
	if (!dpctl->dpram_irq_flags)
		dpctl->dpram_irq_flags = (IRQF_NO_SUSPEND | IRQF_TRIGGER_LOW);
	dpld->irq_flags = dpctl->dpram_irq_flags;

	/* Register DPRAM interrupt handler */
	snprintf(dpld->irq_name, DP_MAX_NAME_LEN, "%s_irq", ld->name);
	if (dpld->ext_op && dpld->ext_op->irq_handler)
		dpld->irq_handler = dpld->ext_op->irq_handler;
	else if (dpld->dp_type == CP_IDPRAM)
		dpld->irq_handler = cp_idpram_irq_handler;
	else if (dpld->dp_type == AP_IDPRAM)
		dpld->irq_handler = ap_idpram_irq_handler;
	else
		dpld->irq_handler = ext_dpram_irq_handler;

	ret = dpram_register_isr(dpld->irq, dpld->irq_handler, dpld->irq_flags,
				dpld->irq_name, dpld);
	if (ret)
		goto err;
	else
		return ld;

err:
	if (dpld) {
		if (dpld->buff)
			kfree(dpld->buff);
		kfree(dpld);
	}

	return NULL;
}

