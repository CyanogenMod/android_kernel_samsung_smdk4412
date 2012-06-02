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

const inline char *get_dev_name(int dev)
{
	if (dev == IPC_FMT)
		return "FMT";
	else if (dev == IPC_RAW)
		return "RAW";
	else if (dev == IPC_RFS)
		return "RFS";
	else
		return "NONE";
}

static void log_dpram_irq(struct dpram_link_device *dpld, u16 int2ap)
{
	struct sk_buff *skb;
	struct mif_event_buff *evtb;
	struct dpram_irq_buff *irqb;
	struct link_device *ld = &dpld->ld;

	skb = alloc_skb(MAX_MIF_EVT_BUFF_SIZE, GFP_ATOMIC);
	if (!skb)
		return;

	evtb = (struct mif_event_buff *)skb_put(skb, MAX_MIF_EVT_BUFF_SIZE);
	memset(evtb, 0, MAX_MIF_EVT_BUFF_SIZE);

	do_gettimeofday(&evtb->tv);
	evtb->evt = MIF_IRQ_EVT;

	strncpy(evtb->mc, ld->mc->name, MAX_MIF_NAME_LEN);
	strncpy(evtb->ld, ld->name, MAX_MIF_NAME_LEN);
	evtb->link_type = ld->link_type;

	irqb = &evtb->dpram_irqb;

	irqb->magic = dpld->dpctl->get_magic();
	irqb->access = dpld->dpctl->get_access();

	irqb->qsp[IPC_FMT].txq.in = dpld->dpctl->get_tx_head(IPC_FMT);
	irqb->qsp[IPC_FMT].txq.out = dpld->dpctl->get_tx_tail(IPC_FMT);
	irqb->qsp[IPC_FMT].rxq.in = dpld->dpctl->get_rx_head(IPC_FMT);
	irqb->qsp[IPC_FMT].rxq.out = dpld->dpctl->get_rx_tail(IPC_FMT);

	irqb->qsp[IPC_RAW].txq.in = dpld->dpctl->get_tx_head(IPC_RAW);
	irqb->qsp[IPC_RAW].txq.out = dpld->dpctl->get_tx_tail(IPC_RAW);
	irqb->qsp[IPC_RAW].rxq.in = dpld->dpctl->get_rx_head(IPC_RAW);
	irqb->qsp[IPC_RAW].rxq.out = dpld->dpctl->get_rx_tail(IPC_RAW);

	irqb->int2ap = int2ap;

	evtb->rcvd = sizeof(struct dpram_irq_buff);
	evtb->len = sizeof(struct dpram_irq_buff);

	mif_irq_log(ld->mc, skb);
	mif_flush_logs(ld->mc);
}

static int memcmp16_to_io(const void __iomem *to, void *from, int size)
{
	u16 *d = (u16 *)to;
	u16 *s = (u16 *)from;
	int count = size >> 1;
	int diff = 0;
	int i;
	u16 d1;
	u16 s1;

	for (i = 0; i < count; i++) {
		d1 = ioread16(d);
		s1 = *s;
		if (d1 != s1) {
			diff++;
			mif_info("ERR! [%d] d:0x%04X != s:0x%04X\n", i, d1, s1);
		}
		d++;
		s++;
	}

	return diff;
}

static int test_dpram(char *dp_name, u8 __iomem *start, u32 size)
{
	u8 __iomem *dst;
	int i;
	u16 val;

	mif_info("%s: start = 0x%p, size = %d\n", dp_name, start, size);

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		iowrite16((i & 0xFFFF), dst);
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		val = ioread16(dst);
		if (val != (i & 0xFFFF)) {
			mif_info("%s: ERR! dst[%d] 0x%04X != 0x%04X\n",
				dp_name, i, val, (i & 0xFFFF));
			return -EINVAL;
		}
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		iowrite16(0x00FF, dst);
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		val = ioread16(dst);
		if (val != 0x00FF) {
			mif_info("%s: ERR! dst[%d] 0x%04X != 0x00FF\n",
				dp_name, i, val);
			return -EINVAL;
		}
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		iowrite16(0x0FF0, dst);
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		val = ioread16(dst);
		if (val != 0x0FF0) {
			mif_info("%s: ERR! dst[%d] 0x%04X != 0x0FF0\n",
				dp_name, i, val);
			return -EINVAL;
		}
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		iowrite16(0xFF00, dst);
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		val = ioread16(dst);
		if (val != 0xFF00) {
			mif_info("%s: ERR! dst[%d] 0x%04X != 0xFF00\n",
				dp_name, i, val);
			return -EINVAL;
		}
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		iowrite16(0, dst);
		dst += 2;
	}

	dst = start;
	for (i = 0; i < (size >> 1); i++) {
		val = ioread16(dst);
		if (val != 0) {
			mif_info("%s: ERR! dst[%d] 0x%04X != 0\n",
				dp_name, i, val);
			return -EINVAL;
		}
		dst += 2;
	}

	mif_info("%s: PASS!!!\n", dp_name);
	return 0;
}

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

static int dpram_register_isr(unsigned irq, irqreturn_t (*isr)(int, void*),
		unsigned long flag, const char *name, struct link_device *ld)
{
	int ret = 0;

	ret = request_irq(irq, isr, flag, name, ld);
	if (ret) {
		mif_info("%s: ERR! request_irq fail (err %d)\n", name, ret);
		return ret;
	}

	ret = enable_irq_wake(irq);
	if (ret)
		mif_info("%s: ERR! enable_irq_wake fail (err %d)\n", name, ret);

	mif_info("%s: IRQ#%d handler registered\n", name, irq);

	return 0;
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
		mif_info("%s: ERR! <%pF> DPRAM wakeup fail\n",
			ld->name, __builtin_return_address(0));
		return -EACCES;
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
	u16 magic = dpld->dpctl->get_magic();
	u16 access = dpld->dpctl->get_access();

	if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
		return 0;

	for (i = 1; i <= 10; i++) {
		mif_info("%s: ERR! magic:%X access:%X -> retry:%d\n",
			ld->name, magic, access, i);
		mdelay(1);

		magic = dpld->dpctl->get_magic();
		access = dpld->dpctl->get_access();
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
		mif_info("%s: ERR! <%pF> ld->mode != LINK_MODE_IPC\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	if (dpram_check_access(dpld) < 0) {
		mif_info("%s: ERR! <%pF> dpram_check_access fail\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	return true;
}

static inline bool dpram_circ_valid(u32 size, u32 in, u32 out)
{
	if (in >= size)
		return false;

	if (out >= size)
		return false;

	return true;
}

/* get the size of the TXQ */
static inline int dpram_get_txq_size(struct dpram_link_device *dpld, int dev)
{
	return dpld->dpctl->get_tx_buff_size(dev);
}

/* get in & out pointers of the TXQ */
static inline void dpram_get_txq_ptrs(struct dpram_link_device *dpld, int dev,
		u32 *in, u32 *out)
{
	*in = dpld->dpctl->get_tx_head(dev);
	*out = dpld->dpctl->get_tx_tail(dev);
}

/* get free space in the TXQ as well as in & out pointers */
static inline int dpram_get_txq_space(struct dpram_link_device *dpld, int dev,
		u32 qsize, u32 *in, u32 *out)
{
	struct link_device *ld = &dpld->ld;

	*in = dpld->dpctl->get_tx_head(dev);
	*out = dpld->dpctl->get_tx_tail(dev);

	if (!dpram_circ_valid(qsize, *in, *out)) {
		mif_info("%s: ERR! <%pF> "
			"%s_TXQ invalid (size:%d in:%d out:%d)\n",
			ld->name, __builtin_return_address(0),
			get_dev_name(dev), qsize, *in, *out);
		dpld->dpctl->set_tx_head(dev, 0);
		dpld->dpctl->set_tx_tail(dev, 0);
		*in = 0;
		*out = 0;
		return -EINVAL;
	}

	return (*in < *out) ? (*out - *in - 1) : (qsize + *out - *in - 1);
}

static void dpram_ipc_write(struct dpram_link_device *dpld, int dev,
		u32 qsize, u32 in, u32 out, struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	struct io_device *iod = skbpriv(skb)->iod;
	u8 __iomem *dst = dpctl->get_tx_buff(dev);
	u8 *src = skb->data;
	u32 len = skb->len;

	/* check queue status */
	mif_debug("%s: {FMT %u %u %u %u} {RAW %u %u %u %u} ...\n", ld->name,
		dpctl->get_tx_head(IPC_FMT), dpctl->get_tx_tail(IPC_FMT),
		dpctl->get_rx_head(IPC_FMT), dpctl->get_rx_tail(IPC_FMT),
		dpctl->get_tx_head(IPC_RAW), dpctl->get_tx_tail(IPC_RAW),
		dpctl->get_rx_head(IPC_RAW), dpctl->get_rx_tail(IPC_RAW));

	if (dev == IPC_FMT) {
		mif_ipc_log(ld->mc, MIF_LNK_TX_EVT, iod, ld, src, len);
		mif_flush_logs(ld->mc);
	}

	if (in < out) {
		/* +++++++++ in ---------- out ++++++++++ */
		memcpy((dst + in), src, len);
	} else {
		/* ------ out +++++++++++ in ------------ */
		u32 space = qsize - in;

		/* 1) in -> buffer end */
		memcpy((dst + in), src, ((len > space) ? space : len));

		/* 2) buffer start -> out */
		if (len > space)
			memcpy(dst, (src + space), (len - space));
	}

	/* update new in pointer */
	in += len;
	if (in >= qsize)
		in -= qsize;
	dpctl->set_tx_head(dev, in);
}

static int dpram_try_ipc_tx(struct dpram_link_device *dpld, int dev)
{
	struct link_device *ld = &dpld->ld;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	u32 qsize = dpram_get_txq_size(dpld, dev);
	u32 in;
	u32 out;
	int space;
	int copied = 0;
	u16 mask = 0;
	unsigned long int flags;

	while (1) {
		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		space = dpram_get_txq_space(dpld, dev, qsize, &in, &out);
		if (unlikely(space < 0)) {
			skb_queue_head(txq, skb);
			return -ENOSPC;
		}

		if (unlikely(space < skb->len)) {
			atomic_set(&dpld->res_required[dev], 1);
			skb_queue_head(txq, skb);
			mask = dpctl->get_mask_req_ack(dev);
			mif_info("%s: %s "
				"qsize[%u] in[%u] out[%u] free[%u] < len[%u]\n",
				ld->name, get_dev_name(dev),
				qsize, in, out, space, skb->len);
			break;
		}

		/* TX if there is enough room in the queue
		*/
		mif_debug("%s: %s "
			"qsize[%u] in[%u] out[%u] free[%u] >= len[%u]\n",
			ld->name, get_dev_name(dev),
			qsize, in, out, space, skb->len);

		spin_lock_irqsave(&dpld->tx_lock, flags);
		dpram_ipc_write(dpld, dev, qsize, in, out, skb);
		spin_unlock_irqrestore(&dpld->tx_lock, flags);

		copied += skb->len;

		dev_kfree_skb_any(skb);
	}

	if (mask)
		return -ENOSPC;
	else
		return copied;
}

static void dpram_trigger_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;
	int i;

	for (i = 0; i < dpld->max_ipc_dev; i++) {
		mif_info("%s: purging %s_skb_txq\b", ld->name, get_dev_name(i));
		skb_queue_purge(ld->skb_txq[i]);
	}

	ld->mode = LINK_MODE_ULOAD;

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_channel(ld, PS_DATA_CH_0);
	if (iod)
		iodevs_for_each(&iod->mc->commons, iodev_netif_stop, 0);
}

static int dpram_trigger_force_cp_crash(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	int cnt = 5000;

	mif_info("%s\n", ld->name);

	dpld->dpctl->send_intr(INT_CMD(INT_CMD_CRASH_EXIT));

	while (cnt--) {
		ret = try_wait_for_completion(&dpld->crash_start_complete);
		if (ret)
			break;
		udelay(1000);
	}

	if (!ret) {
		mif_info("%s: ERR! No CRASH_EXIT ACK from CP\n", ld->name);
		dpram_trigger_crash(dpld);
	}

	return 0;
}

static void dpram_ipc_rx_task(unsigned long data)
{
	struct link_device *ld;
	struct dpram_link_device *dpld;
	struct dpram_rxb *rxb;
	struct io_device *iod;
	u32 qlen;
	int i;

	dpld = (struct dpram_link_device *)data;
	ld = &dpld->ld;

	for (i = 0; i < dpld->max_ipc_dev; i++) {
		if (i == IPC_RAW)
			iod = link_get_iod_with_format(ld, IPC_MULTI_RAW);
		else
			iod = link_get_iod_with_format(ld, i);

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
static int dpram_ipc_recv_data(struct dpram_link_device *dpld, int dev,
				u16 non_cmd)
{
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	struct link_device *ld = &dpld->ld;
	struct dpram_rxb *rxb;
	u8 __iomem *src = dpctl->get_rx_buff(dev);
	u32 in = dpctl->get_rx_head(dev);
	u32 out = dpctl->get_rx_tail(dev);
	u32 qsize = dpctl->get_rx_buff_size(dev);
	u32 rcvd = 0;

	if (in == out)
		return 0;

	if (dev == IPC_FMT)
		log_dpram_irq(dpld, non_cmd);

	/* Get data length in DPRAM*/
	rcvd = (in > out) ? (in - out) : (qsize - out + in);

	mif_debug("%s: %s qsize[%u] in[%u] out[%u] rcvd[%u]\n",
		ld->name, get_dev_name(dev), qsize, in, out, rcvd);

	/* Check each queue */
	if (!dpram_circ_valid(qsize, in, out)) {
		mif_info("%s: ERR! %s_RXQ invalid (size:%d in:%d out:%d)\n",
			ld->name, get_dev_name(dev), qsize, in, out);
		dpctl->set_rx_head(dev, 0);
		dpctl->set_rx_tail(dev, 0);
		return -EINVAL;
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
	dpctl->set_rx_tail(dev, out);

	return rcvd;
}

static void dpram_purge_rx_circ(struct dpram_link_device *dpld, int dev)
{
	u32 in = dpld->dpctl->get_rx_head(dev);
	dpld->dpctl->set_rx_tail(dev, in);
}

static void non_command_handler(struct dpram_link_device *dpld, u16 non_cmd)
{
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	struct link_device *ld = &dpld->ld;
	struct sk_buff_head *txq;
	struct sk_buff *skb;
	int i;
	int ret = 0;
	int copied = 0;
	u32 in;
	u32 out;
	u16 mask = 0;
	u16 req_mask = 0;
	u16 tx_mask = 0;

	if (!dpram_ipc_active(dpld))
		return;

	/* Read data from DPRAM */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		ret = dpram_ipc_recv_data(dpld, i, non_cmd);
		if (ret < 0)
			dpram_purge_rx_circ(dpld, i);

		/* Check and process REQ_ACK (at this time, in == out) */
		if (non_cmd & dpctl->get_mask_req_ack(i)) {
			mif_debug("%s: send %s_RES_ACK\n",
				ld->name, get_dev_name(i));
			mask = dpctl->get_mask_res_ack(i);
			dpctl->send_intr(INT_NON_CMD(mask));
		}
	}

	/* Schedule soft IRQ for RX */
	tasklet_hi_schedule(&dpld->rx_tsk);

	/* Try TX via DPRAM */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		if (atomic_read(&dpld->res_required[i]) > 0) {
			dpram_get_txq_ptrs(dpld, i, &in, &out);
			if (likely(in == out)) {
				ret = dpram_try_ipc_tx(dpld, i);
				if (ret > 0) {
					atomic_set(&dpld->res_required[i], 0);
					tx_mask |= dpctl->get_mask_send(i);
				} else {
					req_mask |= dpctl->get_mask_req_ack(i);
				}
			} else {
				req_mask |= dpctl->get_mask_req_ack(i);
			}
		}
	}

	if (req_mask || tx_mask) {
		tx_mask |= req_mask;
		dpctl->send_intr(INT_NON_CMD(tx_mask));
		mif_debug("%s: send intr 0x%04X\n", ld->name, tx_mask);
	}
}

static int dpram_init_ipc(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	int i;

	if (ld->mode == LINK_MODE_IPC &&
	    dpctl->get_magic() == DPRAM_MAGIC_CODE &&
	    dpctl->get_access() == 1)
		mif_info("%s: IPC already initialized\n", ld->name);

	/* Clear pointers in every circular queue */
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		dpctl->set_tx_head(i, 0);
		dpctl->set_tx_tail(i, 0);
		dpctl->set_rx_head(i, 0);
		dpctl->set_rx_tail(i, 0);
	}

	/* Enable IPC */
	dpctl->set_magic(DPRAM_MAGIC_CODE);
	dpctl->set_access(1);
	if (dpctl->get_magic() != DPRAM_MAGIC_CODE || dpctl->get_access() != 1)
		return -EACCES;

	ld->mode = LINK_MODE_IPC;

	for (i = 0; i < dpld->max_ipc_dev; i++)
		atomic_set(&dpld->res_required[i], 0);

	atomic_set(&dpld->accessing, 0);

	return 0;
}

static void cmd_req_active_handler(struct dpram_link_device *dpld)
{
	dpld->dpctl->send_intr(INT_CMD(INT_CMD_RES_ACTIVE));
}

static void cmd_crash_reset_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = NULL;

	mif_info("%s: Recv 0xC7 (CRASH_RESET)\n", ld->name);

	ld->mode = LINK_MODE_ULOAD;

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);
}

static void cmd_crash_exit_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;

	mif_info("%s: Recv 0xC9 (CRASH_EXIT)\n", ld->name);

	ld->mode = LINK_MODE_ULOAD;

	complete_all(&dpld->crash_start_complete);

	if (ld->mdm_data->modem_type == QC_MDM6600) {
		if (dpld->dpctl->log_disp)
			dpld->dpctl->log_disp(dpld->dpctl);
	}

	dpram_trigger_crash(dpld);
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

	if (ld->mdm_data->modem_type == SEC_CMC221) {
		if (ld->mc->phone_state != STATE_ONLINE) {
			mif_info("%s: phone_state: %d -> ONLINE\n",
				ld->name, ld->mc->phone_state);
			iod->modem_state_changed(iod, STATE_ONLINE);
		}
	} else if (ld->mdm_data->modem_type == QC_MDM6600) {
		if (dpld->dpctl->phone_boot_start_handler)
			dpld->dpctl->phone_boot_start_handler(dpld->dpctl);
	}

	mif_info("%s: Send 0xC2 (INIT_END)\n", ld->name);
	dpld->dpctl->send_intr(INT_CMD(INT_CMD_INIT_END));
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
			dpld->dpctl->send_intr(resp);
		}
		break;

	case EXT_CMD_SET_SPEED_MID:
		if (dpld->dpctl->setup_speed) {
			dpld->dpctl->setup_speed(DPRAM_SPEED_MID);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_MID);
			dpld->dpctl->send_intr(resp);
		}
		break;

	case EXT_CMD_SET_SPEED_HIGH:
		if (dpld->dpctl->setup_speed) {
			dpld->dpctl->setup_speed(DPRAM_SPEED_HIGH);
			resp = INT_EXT_CMD(EXT_CMD_SET_SPEED_HIGH);
			dpld->dpctl->send_intr(resp);
		}
		break;

	default:
		mif_info("%s: unknown command 0x%04X\n", ld->name, cmd);
		break;
	}
}

static void cmc22x_idpram_enable_ipc(struct dpram_link_device *dpld)
{
	dpram_init_ipc(dpld);
}

static int cmc22x_idpram_wait_response(struct dpram_link_device *dpld, u32 resp)
{
	struct link_device *ld = &dpld->ld;
	int count = 50000;
	u32 rcvd = 0;

	if (resp == CMC22x_CP_REQ_NV_DATA) {
		while (1) {
			rcvd = ioread32(dpld->bt_map.resp);
			if (rcvd == resp)
				break;

			rcvd = dpld->dpctl->recv_msg();
			if (rcvd == 0x9999) {
				mif_info("%s: Invalid resp 0x%04X\n",
					ld->name, rcvd);
				panic("CP Crash ... BAD CRC in CP");
			}

			if (count-- < 0) {
				mif_info("%s: Invalid resp 0x%08X\n",
					ld->name, rcvd);
				return -EAGAIN;
			}

			udelay(100);
		}
	} else {
		while (1) {
			rcvd = dpld->dpctl->recv_msg();

			if (rcvd == resp)
				break;

			if (resp == CMC22x_CP_RECV_NV_END &&
			    rcvd == CMC22x_CP_CAL_BAD) {
				mif_info("%s: CMC22x_CP_CAL_BAD\n", ld->name);
				break;
			}

			if (count-- < 0) {
				mif_info("%s: Invalid resp 0x%04X\n",
					ld->name, rcvd);
				return -EAGAIN;
			}

			udelay(100);
		}
	}

	return rcvd;
}

static int cmc22x_idpram_send_boot(struct link_device *ld, unsigned long arg)
{
	int err = 0;
	int cnt = 0;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u8 __iomem *bt_buff = dpld->bt_map.buff;
	struct dpram_boot_img cp_img;
	u8 *img_buff = NULL;

	ld->mode = LINK_MODE_BOOT;

	dpld->dpctl->setup_speed(DPRAM_SPEED_LOW);

	/* Test memory... After testing, memory is cleared. */
	if (test_dpram(ld->name, bt_buff, dpld->bt_map.size) < 0) {
		mif_info("%s: ERR! test_dpram fail!\n", ld->name);
		ld->mode = LINK_MODE_INVALID;
		return -EIO;
	}

	/* Get information about the boot image */
	err = copy_from_user((struct dpram_boot_img *)&cp_img, (void *)arg,
			     sizeof(struct dpram_boot_img));
	mif_info("%s: CP image addr = 0x%08X, size = %d\n",
		ld->name, (int)cp_img.addr, cp_img.size);

	/* Alloc a buffer for the boot image */
	img_buff = kzalloc(dpld->bt_map.size, GFP_KERNEL);
	if (!img_buff) {
		mif_info("%s: ERR! kzalloc fail\n", ld->name);
		ld->mode = LINK_MODE_INVALID;
		return -ENOMEM;
	}

	/* Copy boot image from the user space to the image buffer */
	err = copy_from_user(img_buff, cp_img.addr, cp_img.size);

	/* Copy boot image to DPRAM and verify it */
	memcpy(bt_buff, img_buff, cp_img.size);
	if (memcmp16_to_io(bt_buff, img_buff, cp_img.size)) {
		mif_info("%s: ERR! Boot may be broken!!!\n", ld->name);
		goto err;
	}

	dpld->dpctl->reset();
	udelay(1000);

	if (cp_img.mode == CMC22x_BOOT_MODE_NORMAL) {
		mif_info("%s: CMC22x_BOOT_MODE_NORMAL\n", ld->name);
		mif_info("%s: Send req 0x%08X\n", ld->name, cp_img.req);
		iowrite32(cp_img.req, dpld->bt_map.req);

		/* Wait for cp_img.resp for 1 second */
		mif_info("%s: Wait resp 0x%08X\n", ld->name, cp_img.resp);
		while (ioread32(dpld->bt_map.resp) != cp_img.resp) {
			cnt++;
			msleep_interruptible(10);
			if (cnt > 100) {
				mif_info("%s: ERR! Invalid resp 0x%08X\n",
					ld->name, ioread32(dpld->bt_map.resp));
				goto err;
			}
		}
	} else {
		mif_info("%s: CMC22x_BOOT_MODE_DUMP\n", ld->name);
	}

	kfree(img_buff);

	mif_info("%s: Send BOOT done\n", ld->name);

	if (dpld->dpctl->setup_speed)
		dpld->dpctl->setup_speed(DPRAM_SPEED_HIGH);

	return 0;

err:
	ld->mode = LINK_MODE_INVALID;
	kfree(img_buff);

	mif_info("%s: ERR! Boot send fail!!!\n", ld->name);
	return -EIO;
}

static int cmc22x_idpram_send_main(struct link_device *ld, struct sk_buff *skb)
{
	int err = 0;
	int ret = 0;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = (dpld->bt_map.buff + bf->offset);

	if ((bf->offset + bf->len) > dpld->bt_map.size) {
		mif_info("%s: ERR! Out of DPRAM boundary\n", ld->name);
		err = -EINVAL;
		goto exit;
	}

	if (bf->len)
		memcpy(buff, bf->data, bf->len);

	if (bf->request)
		dpld->dpctl->send_msg((u16)bf->request);

	if (bf->response) {
		err = cmc22x_idpram_wait_response(dpld, bf->response);
		if (err < 0)
			mif_info("%s: ERR! wait_response fail (err %d)\n",
				ld->name, err);
	}

	if (bf->request == CMC22x_CAL_NV_DOWN_END) {
		mif_info("%s: CMC22x_CAL_NV_DOWN_END\n", ld->name);
		cmc22x_idpram_enable_ipc(dpld);
	}

exit:
	if (err < 0)
		ret = err;
	else
		ret = skb->len;

	dev_kfree_skb_any(skb);

	return ret;
}

static void cmc22x_idpram_wait_dump(unsigned long arg)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)arg;
	u16 msg;

	if (!dpld) {
		mif_info("ERR! dpld == NULL\n");
		return;
	}

	msg = dpld->dpctl->recv_msg();

	if (msg == CMC22x_CP_DUMP_END) {
		complete_all(&dpld->dump_recv_done);
		return;
	}

	if (((dpld->dump_rcvd & 0x1) == 0) && (msg == CMC22x_1ST_BUFF_FULL)) {
		complete_all(&dpld->dump_recv_done);
		return;
	}

	if (((dpld->dump_rcvd & 0x1) == 1) && (msg == CMC22x_2ND_BUFF_FULL)) {
		complete_all(&dpld->dump_recv_done);
		return;
	}

	mif_add_timer(&dpld->dump_timer, CMC22x_DUMP_WAIT_TIMEOVER,
		cmc22x_idpram_wait_dump, (unsigned long)dpld);
}

static int cmc22x_idpram_upload(struct dpram_link_device *dpld,
		struct dpram_dump_arg *dumparg)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	u8 __iomem *src;
	int buff_size = CMC22x_DUMP_BUFF_SIZE;

	if ((dpld->dump_rcvd & 0x1) == 0)
		dpld->dpctl->send_msg(CMC22x_1ST_BUFF_READY);
	else
		dpld->dpctl->send_msg(CMC22x_2ND_BUFF_READY);

	init_completion(&dpld->dump_recv_done);

	mif_add_timer(&dpld->dump_timer, CMC22x_DUMP_WAIT_TIMEOVER,
		cmc22x_idpram_wait_dump, (unsigned long)dpld);

	ret = wait_for_completion_interruptible_timeout(
			&dpld->dump_recv_done, DUMP_TIMEOUT);
	if (!ret) {
		mif_info("%s: ERR! CP didn't send dump data!!!\n", ld->name);
		goto err_out;
	}

	if (dpld->dpctl->recv_msg() == CMC22x_CP_DUMP_END) {
		mif_info("%s: CMC22x_CP_DUMP_END\n", ld->name);
		wake_unlock(&dpld->dpram_wake_lock);
		return 0;
	}

	if ((dpld->dump_rcvd & 0x1) == 0)
		src = dpld->ul_map.buff;
	else
		src = dpld->ul_map.buff + CMC22x_DUMP_BUFF_SIZE;

	memcpy(dpld->buff, src, buff_size);

	ret = copy_to_user(dumparg->buff, dpld->buff, buff_size);
	if (ret < 0) {
		mif_info("%s: ERR! copy_to_user fail\n", ld->name);
		goto err_out;
	}

	dpld->dump_rcvd++;
	return buff_size;

err_out:
	wake_unlock(&dpld->dpram_wake_lock);
	return -EIO;
}

static int cbp72_edpram_wait_response(struct dpram_link_device *dpld, u32 resp)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	int int2cp;

	ret = wait_for_completion_interruptible_timeout(
			&dpld->udl_cmd_complete, UDL_TIMEOUT);
	if (!ret) {
		mif_info("%s: ERR! No UDL_CMD_RESP!!!\n", ld->name);
		return -ENXIO;
	}

	int2cp = dpld->dpctl->recv_intr();
	mif_debug("%s: int2cp = 0x%x\n", ld->name, int2cp);
	if (resp == int2cp || int2cp == 0xA700)
		return int2cp;
	else
		return -EINVAL;
}

static int cbp72_edpram_send_bin(struct link_device *ld, struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = dpld->bt_map.buff;
	int err = 0;

	if (bf->len > dpld->bt_map.size) {
		mif_info("%s: ERR! Out of DPRAM boundary\n", ld->name);
		err = -EINVAL;
		goto exit;
	}

	if (bf->len)
		memcpy(buff, bf->data, bf->len);

	init_completion(&dpld->udl_cmd_complete);

	if (bf->request)
		dpld->dpctl->send_intr((u16)bf->request);

	if (bf->response) {
		err = cbp72_edpram_wait_response(dpld, bf->response);
		if (err < 0) {
			mif_info("%s: ERR! wait_response fail (%d)\n",
				ld->name, err);
			goto exit;
		} else if (err == bf->response) {
			err = skb->len;
		}
	}

exit:
	dev_kfree_skb_any(skb);
	return err;
}

static int dpram_upload(struct dpram_link_device *dpld,
		struct dpram_dump_arg *dump, unsigned char __user *target)
{
	struct link_device *ld = &dpld->ld;
	struct ul_header header;
	u8 *dest;
	u8 *buff = vmalloc(DP_DEFAULT_DUMP_LEN);
	u16 plen = 0;
	int err = 0;
	int ret = 0;
	int buff_size = 0;

	mif_info("\n");

	wake_lock(&dpld->dpram_wake_lock);
	init_completion(&dpld->udl_cmd_complete);

	mif_info("%s: req = %x, resp =%x", ld->name, dump->req, dump->resp);

	if (dump->req)
		dpld->dpctl->send_intr((u16)dump->req);

	if (dump->resp) {
		err = cbp72_edpram_wait_response(dpld, dump->resp);
		if (err < 0) {
			mif_info("%s: ERR! wait_response fail (%d)\n",
				ld->name, err);
			goto exit;
		}
	}

	if (dump->cmd)
		return err;

	dest = (u8 *)dpld->ul_map.buff;

	header.bop = *(u8 *)(dest);
	header.total_frame = *(u16 *)(dest + 1);
	header.curr_frame = *(u16 *)(dest + 3);
	header.len = *(u16 *)(dest + 5);

	mif_info("%s: total frame:%d, current frame:%d, data len:%d\n",
		ld->name, header.total_frame, header.curr_frame, header.len);

	plen = min_t(u16, header.len, DP_DEFAULT_DUMP_LEN);

	memcpy(buff, dest + sizeof(struct ul_header), plen);
	ret = copy_to_user(dump->buff, buff, plen);
	if (ret < 0) {
		mif_info("%s: copy_to_user fail\n", ld->name);
		goto exit;
	}
	buff_size = plen;

	ret = copy_to_user(target + 4, &buff_size, sizeof(int));
	if (ret < 0) {
		mif_info("%s: copy_to_user fail\n", ld->name);
		goto exit;
	}

	wake_unlock(&dpld->dpram_wake_lock);

	return err;

exit:
	vfree(buff);
	iowrite32(0, dpld->ul_map.magic);
	wake_unlock(&dpld->dpram_wake_lock);
	return -EIO;
}

static void udl_cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	struct link_device *ld = &dpld->ld;

	if (cmd & UDL_RESULT_FAIL) {
		mif_info("%s: ERR! Command failed: %04x\n", ld->name, cmd);
		return;
	}

	switch (UDL_CMD_MASK(cmd)) {
	case UDL_CMD_RECEIVE_READY:
		mif_debug("%s: Send CP-->AP RECEIVE_READY\n", ld->name);
		dpld->dpctl->send_intr(CMD_IMG_START_REQ);
		break;
	default:
		complete_all(&dpld->udl_cmd_complete);
	}
}

static irqreturn_t dpram_irq_handler(int irq, void *data)
{
	struct link_device *ld = (struct link_device *)data;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u16 int2ap = 0;

	if (!ld->mc || ld->mc->phone_state == STATE_OFFLINE)
		return IRQ_HANDLED;

	if (dpram_wake_up(dpld) < 0)
		return IRQ_HANDLED;

	int2ap = dpld->dpctl->recv_intr();

	if (dpld->dpctl->clear_intr)
		dpld->dpctl->clear_intr();

	if (int2ap == INT_POWERSAFE_FAIL) {
		mif_info("%s: int2ap == INT_POWERSAFE_FAIL\n", ld->name);
		goto exit_isr;
	}

	if (ld->mdm_data->modem_type == QC_MDM6600) {
		if ((int2ap == 0x1234)|(int2ap == 0xDBAB)|(int2ap == 0xABCD)) {
			if (dpld->dpctl->dload_cmd_hdlr)
				dpld->dpctl->dload_cmd_hdlr(dpld->dpctl,
									int2ap);
			goto exit_isr;
		}
	}

	if (UDL_CMD_VALID(int2ap))
		udl_cmd_handler(dpld, int2ap);
	else if (EXT_INT_VALID(int2ap) && EXT_CMD_VALID(int2ap))
		ext_command_handler(dpld, int2ap);
	else if (INT_CMD_VALID(int2ap))
		command_handler(dpld, int2ap);
	else if (INT_VALID(int2ap))
		non_command_handler(dpld, int2ap);
	else
		mif_info("%s: ERR! invalid intr 0x%04X\n", ld->name, int2ap);

exit_isr:
	dpram_allow_sleep(dpld);
	return IRQ_HANDLED;
}

static void dpram_send_ipc(struct link_device *ld, int dev, struct sk_buff *skb)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct sk_buff_head *txq;
	int ret;
	u16 mask;

	if (unlikely(dev >= dpld->max_ipc_dev)) {
		mif_info("%s: ERR! dev %d >= max_ipc_dev(%s)\n",
			ld->name, dev, get_dev_name(dpld->max_ipc_dev));
		return;
	}

	if (dpram_wake_up(dpld) < 0)
		return;

	if (!dpram_ipc_active(dpld))
		goto exit;

	txq = ld->skb_txq[dev];
	if (txq->qlen > 1024)
		mif_info("%s: txq->qlen %d > 1024\n", ld->name, txq->qlen);

	skb_queue_tail(txq, skb);

	if (atomic_read(&dpld->res_required[dev]) > 0) {
		mif_debug("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		goto exit;
	}

	ret = dpram_try_ipc_tx(dpld, dev);
	if (ret > 0) {
		mask = dpld->dpctl->get_mask_send(dev);
		dpld->dpctl->send_intr(INT_NON_CMD(mask));
	} else {
		mask = dpld->dpctl->get_mask_req_ack(dev);
		dpld->dpctl->send_intr(INT_NON_CMD(mask));
		mif_info("%s: Send REQ_ACK 0x%04X\n", ld->name, mask);
	}

exit:
	dpram_allow_sleep(dpld);
}

static int dpram_send_binary(struct link_device *ld, struct sk_buff *skb)
{
	int err = 0;

	if (ld->mdm_data->modem_type == SEC_CMC221)
		err = cmc22x_idpram_send_main(ld, skb);
	else if (ld->mdm_data->modem_type == VIA_CBP72)
		err = cbp72_edpram_send_bin(ld, skb);

	return err;
}

static int dpram_send(struct link_device *ld, struct io_device *iod,
		struct sk_buff *skb)
{
	enum dev_format fmt = iod->format;
	int len = skb->len;

	switch (fmt) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		if (likely(ld->mc->phone_state == STATE_ONLINE))
			dpram_send_ipc(ld, fmt, skb);
		return len;

	case IPC_BOOT:
		return dpram_send_binary(ld, skb);

	default:
		mif_info("%s: ERR! no TXQ for %s\n", ld->name, iod->name);
		dev_kfree_skb_any(skb);
		return -ENODEV;
	}
}

static int dpram_set_dl_magic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	ld->mode = LINK_MODE_DLOAD;

	iowrite32(DP_MAGIC_DMDL, dpld->dl_map.magic);

	return 0;
}

static int dpram_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_info("%s\n", ld->name);

	if (dpram_wake_up(dpld) < 0)
		mif_info("%s: WARNING! dpram_wake_up fail\n", ld->name);

	dpram_trigger_force_cp_crash(dpld);

	dpram_allow_sleep(dpld);

	return 0;
}

static int dpram_set_ul_magic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u8 *dest = dpld->ul_map.buff;

	ld->mode = LINK_MODE_ULOAD;

	if (ld->mdm_data->modem_type == SEC_CMC221) {
		wake_lock(&dpld->dpram_wake_lock);
		dpld->dump_rcvd = 0;
		iowrite32(CMC22x_CP_DUMP_MAGIC, dpld->ul_map.magic);
	} else {
		iowrite32(DP_MAGIC_UMDL, dpld->ul_map.magic);

		iowrite8((u8)START_INDEX, dest + 0);
		iowrite8((u8)0x1, dest + 1);
		iowrite8((u8)0x1, dest + 2);
		iowrite8((u8)0x0, dest + 3);
		iowrite8((u8)END_INDEX, dest + 4);
	}

	init_completion(&dpld->dump_start_complete);

	return 0;
}

static int dpram_dump_update(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_dump_arg dump;
	int ret;

	ret = copy_from_user(&dump, (void __user *)arg, sizeof(dump));
	if (ret  < 0) {
		mif_info("%s: ERR! copy_from_user fail\n", ld->name);
		return ret;
	}

	if (ld->mdm_data->modem_type == SEC_CMC221)
		return cmc22x_idpram_upload(dpld, &dump);
	else
		return dpram_upload(dpld, &dump, (unsigned char __user *)arg);
}

static int dpram_link_ioctl(struct link_device *ld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	int err = -EFAULT;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_info("%s: cmd 0x%08X\n", ld->name, cmd);

	switch (cmd) {
	case IOCTL_DPRAM_SEND_BOOT:
		err = cmc22x_idpram_send_boot(ld, arg);
		if (err < 0) {
			mif_info("%s: ERR! dpram_send_boot fail\n", ld->name);
			goto exit;
		}
		break;

	case IOCTL_DPRAM_PHONE_POWON:
		if (dpld->dpctl->cpimage_load_prepare) {
			err = dpld->dpctl->cpimage_load_prepare(dpld->dpctl);
			if (err < 0) {
				mif_info("%s: ERR! cpimage_load_prepare fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_PHONEIMG_LOAD:
		if (dpld->dpctl->cpimage_load) {
			err = dpld->dpctl->cpimage_load(
					(void *)arg, dpld->dpctl);
			if (err < 0) {
				mif_info("%s: ERR! cpimage_load fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_NVDATA_LOAD:
		if (dpld->dpctl->nvdata_load) {
			err = dpld->dpctl->nvdata_load(
					(void *)arg, dpld->dpctl);
			if (err < 0) {
				mif_info("%s: ERR! nvdata_load fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_PHONE_BOOTSTART:
		if (dpld->dpctl->phone_boot_start) {
			err = dpld->dpctl->phone_boot_start(dpld->dpctl);
			if (err < 0) {
				mif_info("%s: ERR! phone_boot_start fail\n",
					ld->name);
				goto exit;
			}
		}
		if (dpld->dpctl->phone_boot_start_post_process) {
			err = dpld->dpctl->phone_boot_start_post_process();
			if (err < 0) {
				mif_info("%s: ERR! "
					"phone_boot_start_post_process fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP1:
		disable_irq_nosync(dpld->irq);

		if (dpld->dpctl->cpupload_step1) {
			err = dpld->dpctl->cpupload_step1(dpld->dpctl);
			if (err < 0) {
				dpld->dpctl->clear_intr();
				enable_irq(dpld->irq);
				mif_info("%s: ERR! cpupload_step1 fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP2:
		if (dpld->dpctl->cpupload_step2) {
			err = dpld->dpctl->cpupload_step2(
					(void *)arg, dpld->dpctl);
			if (err < 0) {
				dpld->dpctl->clear_intr();
				enable_irq(dpld->irq);
				mif_info("%s: ERR! cpupload_step2 fail\n",
					ld->name);
				goto exit;
			}
		}
		break;

	case IOCTL_DPRAM_INIT_STATUS:
		mif_debug("%s: get dpram init status\n", ld->name);
		return dpld->dpram_init_status;

	case IOCTL_MODEM_DL_START:
		err = dpram_set_dl_magic(ld, iod);
		if (err < 0) {
			mif_info("%s: ERR! dpram_set_dl_magic fail\n",
				ld->name);
			goto exit;
		}

	default:
		break;
	}

	return 0;

exit:
	return err;
}

static void dpram_table_init(struct dpram_link_device *dpld)
{
	struct link_device *ld;
	u8 __iomem *dp_base;

	if (!dpld) {
		mif_info("ERR! dpld == NULL\n");
		return;
	}
	ld = &dpld->ld;

	if (!dpld->dp_base) {
		mif_info("%s: ERR! dpld->dp_base == NULL\n", ld->name);
		return;
	}

	dp_base = dpld->dp_base;

	/* Map for booting */
	if (ld->mdm_data->modem_type == SEC_CMC221) {
		dpld->bt_map.buff = (u8 *)(dp_base);
		dpld->bt_map.req  = (u32 *)(dp_base + DP_BOOT_REQ_OFFSET);
		dpld->bt_map.resp = (u32 *)(dp_base + DP_BOOT_RESP_OFFSET);
		dpld->bt_map.size = dpld->dp_size;
	} else if (ld->mdm_data->modem_type == QC_MDM6600) {
		if (dpld->dpctl->bt_map_init)
			dpld->dpctl->bt_map_init(dpld->dpctl);
	} else if (ld->mdm_data->modem_type == VIA_CBP72) {
		dpld->bt_map.magic = (u32 *)(dp_base);
		dpld->bt_map.buff = (u8 *)(dp_base + DP_BOOT_BUFF_OFFSET);
		dpld->bt_map.size = dpld->dp_size - 4;
	} else {
		dpld->bt_map.buff = (u8 *)(dp_base);
		dpld->bt_map.req  = (u32 *)(dp_base + DP_BOOT_REQ_OFFSET);
		dpld->bt_map.resp = (u32 *)(dp_base + DP_BOOT_RESP_OFFSET);
		dpld->bt_map.size = dpld->dp_size - 4;
	}

	/* Map for download (FOTA, UDL, etc.) */
	if (ld->mdm_data->modem_type == SEC_CMC221 ||
			ld->mdm_data->modem_type == VIA_CBP72) {
		dpld->dl_map.magic = (u32 *)(dp_base);
		dpld->dl_map.buff  = (u8 *)(dp_base + DP_DLOAD_BUFF_OFFSET);
	}

	/* Map for upload mode */
	dpld->ul_map.magic = (u32 *)(dp_base);
	if (ld->mdm_data->modem_type == SEC_CMC221)
		dpld->ul_map.buff = (u8 *)(dp_base);
	else
		dpld->ul_map.buff = (u8 *)(dp_base + DP_ULOAD_BUFF_OFFSET);
}

#if defined(CONFIG_MACH_M0_CTC)
static void dpram_link_terminate(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_info("dpram_link_terminate\n");

	if (dpld->dpctl->terminate_link)
			dpld->dpctl->terminate_link(dpld->dpctl);
}
#endif

struct link_device *dpram_create_link_device(struct platform_device *pdev)
{
	struct dpram_link_device *dpld = NULL;
	struct link_device *ld = NULL;
	struct modem_data *pdata = NULL;
	struct modemlink_dpram_control *dpctl = NULL;
	int ret = 0;
	int i = 0;
	int bsize;
	int qsize;
	char wq_name[32];
	char wq_suffix[32];

	/* Get the platform data */
	pdata = (struct modem_data *)pdev->dev.platform_data;
	if (!pdata) {
		mif_info("ERR! pdata == NULL\n");
		goto err;
	}
	if (!pdata->dpram_ctl) {
		mif_info("ERR! pdata->dpram_ctl == NULL\n");
		goto err;
	}
	mif_info("link device = %s\n", pdata->link_name);
	mif_info("modem = %s\n", pdata->name);

	/* Alloc DPRAM link device structure */
	dpld = kzalloc(sizeof(struct dpram_link_device), GFP_KERNEL);
	if (!dpld) {
		mif_info("ERR! kzalloc dpld fail\n");
		goto err;
	}
	ld = &dpld->ld;

	/* Extract modem data and DPRAM control data from the platform data */
	ld->mdm_data = pdata;
	ld->name = pdata->link_name;
	ld->ipc_version = pdata->ipc_version;

	/* Set attributes as a link device */
	ld->aligned = pdata->dpram_ctl->aligned;
	if (ld->aligned)
		mif_info("%s: ld->aligned == TRUE\n", ld->name);

	ld->send = dpram_send;
	ld->force_dump = dpram_force_dump;
	ld->dump_start = dpram_set_ul_magic;
	ld->dump_update = dpram_dump_update;
	ld->ioctl = dpram_link_ioctl;

#if defined(CONFIG_MACH_M0_CTC)
	ld->terminate_comm = dpram_link_terminate;
#endif
	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	skb_queue_head_init(&ld->sk_rfs_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;
	ld->skb_txq[IPC_RFS] = &ld->sk_rfs_tx_q;

	/* Set attributes as a dpram link device */
	dpctl = pdata->dpram_ctl;
	dpld->dpctl = dpctl;

	dpld->dp_base = dpctl->dp_base;
	dpld->dp_size = dpctl->dp_size;
	dpld->dp_type = dpctl->dp_type;

	dpld->max_ipc_dev = dpctl->max_ipc_dev;

	dpld->irq = dpctl->dpram_irq;
	if (dpld->irq < 0) {
		mif_info("%s: ERR! failed to get IRQ#\n", ld->name);
		goto err;
	}
	mif_info("%s: DPRAM IRQ# = %d\n", ld->name, dpld->irq);

	wake_lock_init(&dpld->dpram_wake_lock, WAKE_LOCK_SUSPEND,
			dpctl->dpram_wlock_name);

	init_completion(&dpld->dpram_init_cmd);
	init_completion(&dpld->modem_pif_init_done);
	init_completion(&dpld->udl_start_complete);
	init_completion(&dpld->udl_cmd_complete);
	init_completion(&dpld->crash_start_complete);
	init_completion(&dpld->dump_start_complete);
	init_completion(&dpld->dump_recv_done);

	spin_lock_init(&dpld->tx_lock);

	tasklet_init(&dpld->rx_tsk, dpram_ipc_rx_task, (unsigned long)dpld);

	/* Initialize DPRAM map (physical map -> logical map) */
	dpram_table_init(dpld);

#if 0
	dpld->magic = dpctl->ipc_map->magic;
	dpld->access = dpctl->ipc_map->access;
	for (i = 0; i < dpld->max_ipc_dev; i++)
		dpld->dev[i] = &dpctl->ipc_map->dev[i];
	dpld->mbx2ap = dpctl->ipc_map->mbx_cp2ap;
	dpld->mbx2cp = dpctl->ipc_map->mbx_ap2cp;
#endif

	/* Prepare rxb queue */
	qsize = DPRAM_MAX_RXBQ_SIZE;
	for (i = 0; i < dpld->max_ipc_dev; i++) {
		bsize = rxbq_get_page_size(dpctl->get_rx_buff_size(i));
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

	/* Prepare a clean buffer */
	dpld->buff = kzalloc(dpld->dp_size, GFP_KERNEL);
	if (!dpld->buff) {
		mif_info("%s: ERR! kzalloc dpld->buff fail\n", ld->name);
		goto err;
	}

	if (ld->mdm_data->modem_type == QC_MDM6600) {
		if (dpctl->load_init)
			dpctl->load_init(dpctl);
	}

	/* Disable IPC */
	dpctl->set_magic(0);
	dpctl->set_access(0);
	dpld->dpram_init_status = DPRAM_INIT_STATE_NONE;

	/* Register DPRAM interrupt handler */
	ret = dpram_register_isr(dpld->irq, dpram_irq_handler,
			 dpctl->dpram_irq_flags, dpctl->dpram_irq_name, ld);
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

