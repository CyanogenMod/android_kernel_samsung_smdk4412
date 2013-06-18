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
#include "modem_link_device_pld.h"
#include "modem_utils.h"


/*
** Function prototypes for basic DPRAM operations
*/
static inline void clear_intr(struct pld_link_device *pld);
static inline u16 recv_intr(struct pld_link_device *pld);
static inline void send_intr(struct pld_link_device *pld, u16 mask);

static inline u16 get_magic(struct pld_link_device *pld);
static inline void set_magic(struct pld_link_device *pld, u16 val);
static inline u16 get_access(struct pld_link_device *pld);
static inline void set_access(struct pld_link_device *pld, u16 val);

static inline u32 get_tx_head(struct pld_link_device *pld, int id);
static inline u32 get_tx_tail(struct pld_link_device *pld, int id);
static inline void set_tx_head(struct pld_link_device *pld, int id, u32 in);
static inline void set_tx_tail(struct pld_link_device *pld, int id, u32 out);
static inline u8 *get_tx_buff(struct pld_link_device *pld, int id);
static inline u32 get_tx_buff_size(struct pld_link_device *pld, int id);

static inline u32 get_rx_head(struct pld_link_device *pld, int id);
static inline u32 get_rx_tail(struct pld_link_device *pld, int id);
static inline void set_rx_head(struct pld_link_device *pld, int id, u32 in);
static inline void set_rx_tail(struct pld_link_device *pld, int id, u32 out);
static inline u8 *get_rx_buff(struct pld_link_device *pld, int id);
static inline u32 get_rx_buff_size(struct pld_link_device *pld, int id);

static inline u16 get_mask_req_ack(struct pld_link_device *pld, int id);
static inline u16 get_mask_res_ack(struct pld_link_device *pld, int id);
static inline u16 get_mask_send(struct pld_link_device *pld, int id);

static void handle_cp_crash(struct pld_link_device *pld);
static int trigger_force_cp_crash(struct pld_link_device *pld);

/*
** Functions for debugging
*/
static void set_dpram_map(struct pld_link_device *pld,
			struct mif_irq_map *map)
{
	map->magic = get_magic(pld);
	map->access = get_access(pld);

	map->fmt_tx_in = get_tx_head(pld, IPC_FMT);
	map->fmt_tx_out = get_tx_tail(pld, IPC_FMT);
	map->fmt_rx_in = get_rx_head(pld, IPC_FMT);
	map->fmt_rx_out = get_rx_tail(pld, IPC_FMT);
	map->raw_tx_in = get_tx_head(pld, IPC_RAW);
	map->raw_tx_out = get_tx_tail(pld, IPC_RAW);
	map->raw_rx_in = get_rx_head(pld, IPC_RAW);
	map->raw_rx_out = get_rx_tail(pld, IPC_RAW);

	map->cp2ap = recv_intr(pld);
}

/*
** DPRAM operations
*/
static int pld_register_isr(unsigned irq, irqreturn_t (*isr)(int, void*),
				unsigned long flag, const char *name,
				struct pld_link_device *pld)
{
	int ret = 0;

	ret = request_irq(irq, isr, flag, name, pld);
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

static inline void clear_intr(struct pld_link_device *pld)
{
	if (pld->ext_op && pld->ext_op->clear_intr)
		pld->ext_op->clear_intr(pld);
}

static inline u16 recv_intr(struct pld_link_device *pld)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->mbx2ap[0]),
				pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&pld->mbx2ap[0]),
				pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: intr1(%d) != intr1(%d)\n", val1, val2);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);

	return val1;
}

static inline void send_intr(struct pld_link_device *pld, u16 mask)
{
	int cnt = 3;
	u32 val = 0;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	iowrite16(PLD_ADDR_MASK(&pld->mbx2cp[0]),
				pld->address_buffer);
	iowrite16((u16)mask, pld->base);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->mbx2cp[0]),
					pld->address_buffer);
		val =  ioread16(pld->base);

		if (likely(val == mask))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return;
		}

		mif_err("ERR: intr1(%d) != intr2(%d)\n", val, mask);
		udelay(100);

		/* Write head value again */
		iowrite16(PLD_ADDR_MASK(&pld->mbx2cp[0]),
					pld->address_buffer);
		iowrite16((u16)mask, pld->base);
	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);

	return;
}

static inline u16 get_magic(struct pld_link_device *pld)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->magic_ap2cp[0]),
				pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&pld->magic_ap2cp[0]),
				pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: txq.head(%d) != in(%d)\n", val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;

}

static inline void set_magic(struct pld_link_device *pld, u16 in)
{
	int cnt = 3;
	u32 val = 0;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	iowrite16(PLD_ADDR_MASK(&pld->magic_ap2cp[0]),
				pld->address_buffer);
	iowrite16((u16)in, pld->base);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->magic_ap2cp[0]),
					pld->address_buffer);
		val =  ioread16(pld->base);

		if (likely(val == in))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return;
		}

		mif_err("ERR: magic1(%d) != magic2(%d)\n", val, in);
		udelay(100);

		/* Write head value again */
		iowrite16(PLD_ADDR_MASK(&pld->magic_ap2cp[0]),
					pld->address_buffer);
		iowrite16((u16)in, pld->base);
	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return;
}

static inline u16 get_access(struct pld_link_device *pld)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->access_ap2cp[0]),
				pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&pld->access_ap2cp[0]),
				pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: access1(%d) != access2(%d)\n", val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;

}

static inline void set_access(struct pld_link_device *pld, u16 in)
{
	int cnt = 3;
	u32 val = 0;
	unsigned long int flags;

	iowrite16(PLD_ADDR_MASK(&pld->access_ap2cp[0]),
				pld->address_buffer);
	iowrite16((u16)in, pld->base);

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&pld->access_ap2cp[0]),
					pld->address_buffer);
		val =  ioread16(pld->base);

		if (likely(val == in))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return;
		}

		mif_err("ERR: access(%d) != access(%d)\n", val, in);
		udelay(100);

		/* Write head value again */
		iowrite16(PLD_ADDR_MASK(&pld->access_ap2cp[0]),
					pld->address_buffer);
		iowrite16((u16)in, pld->base);
	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return;
}

static inline u32 get_tx_head(struct pld_link_device *pld, int id)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.head)[0]),
				pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.head)[0]),
				pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: %s txq.head(%d) != in(%d)\n",
			get_dev_name(id), val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;
}

static inline u32 get_tx_tail(struct pld_link_device *pld, int id)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.tail)[0]),
					pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.tail)[0]),
					pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: %s txq.tail(%d) != in(%d)\n",
			get_dev_name(id), val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;
}

static inline void set_tx_head(struct pld_link_device *pld, int id, u32 in)
{
	int cnt = 3;
	u32 val = 0;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.head)[0]),
				pld->address_buffer);
	iowrite16((u16)in, pld->base);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.head)[0]),
					pld->address_buffer);
		val =  ioread16(pld->base);

		if (likely(val == in))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return;
		}

		mif_err("ERR: %s txq.head(%d) != in(%d)\n",
			get_dev_name(id), val, in);
		udelay(100);

		/* Write head value again */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->txq.head)[0]),
					pld->address_buffer);
		iowrite16((u16)in, pld->base);
	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return;
}

static inline void set_tx_tail(struct pld_link_device *pld, int id, u32 out)
{
	return;
}

static inline u8 *get_tx_buff(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->txq.buff;
}

static inline u32 get_tx_buff_size(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->txq.size;
}

static inline u32 get_rx_head(struct pld_link_device *pld, int id)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.head)[0]),
					pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.head)[0]),
					pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: %s rxq.head(%d) != in(%d)\n",
			get_dev_name(id), val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;
}

static inline u32 get_rx_tail(struct pld_link_device *pld, int id)
{
	u16 val1 = 0, val2 = 0, cnt = 3;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	do {
		/* Check head value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.tail)[0]),
					pld->address_buffer);
		val1 =  ioread16(pld->base);

		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.tail)[0]),
					pld->address_buffer);
		val2 =  ioread16(pld->base);

		if (likely(val1 == val2))	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return val1;
		}

		mif_err("ERR: %s rxq.tail(%d) != in(%d)\n",
		get_dev_name(id), val1, val2);
		udelay(100);

	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return val1;
}

static inline void set_rx_head(struct pld_link_device *pld, int id, u32 in)
{
	return;
}

static inline void set_rx_tail(struct pld_link_device *pld, int id, u32 out)
{
	int cnt = 3;
	u32 val = 0;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.tail)[0]),
				pld->address_buffer);
	iowrite16((u16)out, pld->base);

	do {
		/* Check tail value written */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.tail)[0]),
					pld->address_buffer);
		val =  ioread16(pld->base);

		if (val == out)	{
			spin_unlock_irqrestore(&pld->pld_lock, flags);
			return;
		}

		mif_err("ERR: %s rxq.tail(%d) != out(%d)\n",
			get_dev_name(id), val, out);
		udelay(100);

		/* Write tail value again */
		iowrite16(PLD_ADDR_MASK(&(pld->dev[id]->rxq.tail)[0]),
					pld->address_buffer);
		iowrite16((u16)out, pld->base);
	} while (cnt--);

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	return;
}

static inline u8 *get_rx_buff(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->rxq.buff;
}

static inline u32 get_rx_buff_size(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->rxq.size;
}

static inline u16 get_mask_req_ack(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->mask_req_ack;
}

static inline u16 get_mask_res_ack(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->mask_res_ack;
}

static inline u16 get_mask_send(struct pld_link_device *pld, int id)
{
	return pld->dev[id]->mask_send;
}

/* Get free space in the TXQ as well as in & out pointers */
static inline int get_txq_space(struct pld_link_device *pld, int dev, u32 qsize,
		u32 *in, u32 *out)
{
	struct link_device *ld = &pld->ld;
	int cnt = 3;
	u32 head;
	u32 tail;
	int space;

	do {
		head = get_tx_head(pld, dev);
		tail = get_tx_tail(pld, dev);

		space = (head < tail) ? (tail - head - 1) :
			(qsize + tail - head - 1);
		mif_debug("%s: %s_TXQ qsize[%u] in[%u] out[%u] space[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, space);

		if (circ_valid(qsize, head, tail)) {
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

static void reset_tx_circ(struct pld_link_device *pld, int dev)
{
	set_tx_head(pld, dev, 0);
	set_tx_tail(pld, dev, 0);
	if (dev == IPC_FMT)
		trigger_force_cp_crash(pld);
}

/* Get data size in the RXQ as well as in & out pointers */
static inline int get_rxq_rcvd(struct pld_link_device *pld, int dev, u32 qsize,
		u32 *in, u32 *out)
{
	struct link_device *ld = &pld->ld;
	int cnt = 3;
	u32 head;
	u32 tail;
	u32 rcvd;

	do {
		head = get_rx_head(pld, dev);
		tail = get_rx_tail(pld, dev);
		if (head == tail) {
			*in = head;
			*out = tail;
			return 0;
		}

		rcvd = (head > tail) ? (head - tail) : (qsize - tail + head);
		mif_info("%s: %s_RXQ qsize[%u] in[%u] out[%u] rcvd[%u]\n",
			ld->name, get_dev_name(dev), qsize, head, tail, rcvd);

		if (circ_valid(qsize, head, tail)) {
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

static void reset_rx_circ(struct pld_link_device *pld, int dev)
{
	set_rx_head(pld, dev, 0);
	set_rx_tail(pld, dev, 0);
	if (dev == IPC_FMT)
		trigger_force_cp_crash(pld);
}

static int check_access(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	int i;
	u16 magic = get_magic(pld);
	u16 access = get_access(pld);

	if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
		return 0;

	for (i = 1; i <= 100; i++) {
		mif_info("%s: ERR! magic:%X access:%X -> retry:%d\n",
			ld->name, magic, access, i);
		udelay(100);

		magic = get_magic(pld);
		access = get_access(pld);
		if (likely(magic == DPRAM_MAGIC_CODE && access == 1))
			return 0;
	}

	mif_info("%s: !CRISIS! magic:%X access:%X\n", ld->name, magic, access);
	return -EACCES;
}

static bool ipc_active(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;

	/* Check DPRAM mode */
	if (ld->mode != LINK_MODE_IPC) {
		mif_info("%s: <%pf> ld->mode != LINK_MODE_IPC\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	if (check_access(pld) < 0) {
		mif_info("%s: ERR! <%pf> check_access fail\n",
			ld->name, __builtin_return_address(0));
		return false;
	}

	return true;
}

static void pld_ipc_write(struct pld_link_device *pld, int dev,
		u32 qsize, u32 in, u32 out, struct sk_buff *skb)
{
	struct link_device *ld = &pld->ld;
	u8 __iomem *buff = get_tx_buff(pld, dev);
	u8 *src = skb->data;
	u32 len = skb->len;
	u32 inp;
	struct mif_irq_map map;
	unsigned long int flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	if (in < out) {
		/* +++++++++ in ---------- out ++++++++++ */
		iowrite16(PLD_ADDR_MASK(&(buff+in)[0]), pld->address_buffer);
		memcpy(pld->base, src, len);
	} else {
		/* ------ out +++++++++++ in ------------ */
		u32 space = qsize - in;

		/* 1) in -> buffer end */
		iowrite16(PLD_ADDR_MASK(&(buff+in)[0]), pld->address_buffer);
		memcpy(pld->base, src, ((len > space) ? space : len));

		if (len > space)	{
			iowrite16(PLD_ADDR_MASK(&buff[0]),
						pld->address_buffer);
			memcpy(pld->base, (src+space), (len-space));
		}
	}

	spin_unlock_irqrestore(&pld->pld_lock, flags);

	/* update new in pointer */
	inp = in + len;
	if (inp >= qsize)
		inp -= qsize;
	set_tx_head(pld, dev, inp);

	if (dev == IPC_FMT) {
		set_dpram_map(pld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_write", sizeof("ipc_write"));
		mif_ipc_log(MIF_IPC_AP2CP, ld->mc->msd, skb->data, skb->len);
	}
}

static int pld_try_ipc_tx(struct pld_link_device *pld, int dev)
{
	struct link_device *ld = &pld->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	unsigned long int flags;
	u32 qsize = get_tx_buff_size(pld, dev);
	u32 in;
	u32 out;
	int space;
	int copied = 0;

	spin_lock_irqsave(&pld->tx_rx_lock, flags);

	while (1) {
		space = get_txq_space(pld, dev, qsize, &in, &out);
		if (unlikely(space < 0)) {
			spin_unlock_irqrestore(&pld->tx_rx_lock, flags);
			reset_tx_circ(pld, dev);
			return space;
		}

		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		if (unlikely(space < skb->len)) {
			atomic_set(&pld->res_required[dev], 1);
			skb_queue_head(txq, skb);
			spin_unlock_irqrestore(&pld->tx_rx_lock, flags);
			mif_info("%s: %s "
				"qsize[%u] in[%u] out[%u] free[%u] < len[%u]\n",
				ld->name, get_dev_name(dev),
				qsize, in, out, space, skb->len);
			return -ENOSPC;
		}

		/* TX if there is enough room in the queue */
		pld_ipc_write(pld, dev, qsize, in, out, skb);
		copied += skb->len;
		dev_kfree_skb_any(skb);
	}

	spin_unlock_irqrestore(&pld->tx_rx_lock, flags);

	return copied;
}

static void pld_ipc_rx_task(unsigned long data)
{
	struct pld_link_device *pld = (struct pld_link_device *)data;
	struct link_device *ld = &pld->ld;
	struct io_device *iod;
	struct mif_rxb *rxb;
	unsigned qlen;
	int i;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		iod = pld->iod[i];
		qlen = rxbq_size(&pld->rxbq[i]);
		while (qlen > 0) {
			rxb = rxbq_get_data_rxb(&pld->rxbq[i]);
			iod->recv(iod, ld, rxb->data, rxb->len);
			rxb_clear(rxb);
			qlen--;
		}
	}
}

static void pld_ipc_read(struct pld_link_device *pld, int dev, u8 *dst,
	u8 __iomem *src, u32 out, u32 len, u32 qsize)
{
	u8 *ori_det = dst;
	unsigned long flags;

	spin_lock_irqsave(&pld->pld_lock, flags);

	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		iowrite16(PLD_ADDR_MASK(&(src+out)[0]), pld->address_buffer);
		memcpy(dst, pld->base, len);
	} else {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		unsigned len1 = qsize - out;

		/* 1) out -> buffer end */
		iowrite16(PLD_ADDR_MASK(&(src+out)[0]), pld->address_buffer);
		memcpy(dst, pld->base, len1);

		/* 2) buffer start -> in */
		dst += len1;
		iowrite16(PLD_ADDR_MASK(&src[0]), pld->address_buffer);
		memcpy(dst, pld->base, (len - len1));
	}

	spin_unlock_irqrestore(&pld->pld_lock, flags);
	if (pld->ld.mode == LINK_MODE_IPC && ori_det[0] != 0x7F)	{
		mif_info("ipc read error!! in[%d], out[%d]\n",
					get_rx_head(pld, dev),
					get_rx_tail(pld, dev));
	}

}

/*
  ret < 0  : error
  ret == 0 : no data
  ret > 0  : valid data
*/
static int pld_ipc_recv_data_with_rxb(struct pld_link_device *pld, int dev)
{
	struct link_device *ld = &pld->ld;
	struct mif_rxb *rxb;
	u8 __iomem *src = get_rx_buff(pld, dev);
	u32 qsize = get_rx_buff_size(pld, dev);
	u32 in;
	u32 out;
	u32 rcvd;
	struct mif_irq_map map;
	unsigned long int flags;

	spin_lock_irqsave(&pld->tx_rx_lock, flags);

	rcvd = get_rxq_rcvd(pld, dev, qsize, &in, &out);
	if (rcvd <= 0)	{
		spin_unlock_irqrestore(&pld->tx_rx_lock, flags);
		return rcvd;
	}

	if (dev == IPC_FMT) {
		set_dpram_map(pld, &map);
		mif_irq_log(ld->mc->msd, map, "ipc_recv", sizeof("ipc_recv"));
	}

	/* Allocate an rxb */
	rxb = rxbq_get_free_rxb(&pld->rxbq[dev]);
	if (!rxb) {
		mif_info("%s: ERR! %s rxbq_get_free_rxb fail\n",
			ld->name, get_dev_name(dev));
		spin_unlock_irqrestore(&pld->tx_rx_lock, flags);
		return -ENOMEM;
	}

	/* Read data from each DPRAM buffer */
	pld_ipc_read(pld, dev, rxb_put(rxb, rcvd), src, out, rcvd, qsize);

	/* Calculate and set new out */
	out += rcvd;
	if (out >= qsize)
		out -= qsize;
	set_rx_tail(pld, dev, out);

	spin_unlock_irqrestore(&pld->tx_rx_lock, flags);
	return rcvd;
}

static void non_command_handler(struct pld_link_device *pld, u16 non_cmd)
{
	struct link_device *ld = &pld->ld;
	int i = 0;
	int ret = 0;
	u16 mask = 0;

	if (!ipc_active(pld))
		return;

	/* Read data from DPRAM */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		ret = pld_ipc_recv_data_with_rxb(pld, i);
		if (ret < 0)
			reset_rx_circ(pld, i);

		/* Check and process REQ_ACK (at this time, in == out) */
		if (non_cmd & get_mask_req_ack(pld, i)) {
			mif_debug("%s: send %s_RES_ACK\n",
				ld->name, get_dev_name(i));
			mask |= get_mask_res_ack(pld, i);
		}
	}

	/* Schedule soft IRQ for RX */
	tasklet_hi_schedule(&pld->rx_tsk);

	/* Try TX via DPRAM */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		if (atomic_read(&pld->res_required[i]) > 0) {
			ret = pld_try_ipc_tx(pld, i);
			if (ret > 0) {
				atomic_set(&pld->res_required[i], 0);
				mask |= get_mask_send(pld, i);
			} else if (ret == -ENOSPC) {
				mask |= get_mask_req_ack(pld, i);
			}
		}
	}

	if (mask) {
		send_intr(pld, INT_NON_CMD(mask));
		mif_debug("%s: send intr 0x%04X\n", ld->name, mask);
	}
}

static void handle_cp_crash(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	struct io_device *iod;
	int i;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		mif_info("%s: purging %s_skb_txq\b", ld->name, get_dev_name(i));
		skb_queue_purge(ld->skb_txq[i]);
	}

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	iod = link_get_iod_with_channel(ld, RMNET0_CH_ID);
	if (iod)
		iodevs_for_each(iod->msd, iodev_netif_stop, 0);
}

static void handle_no_crash_ack(unsigned long arg)
{
	struct pld_link_device *pld = (struct pld_link_device *)arg;
	struct link_device *ld = &pld->ld;

	mif_err("%s: ERR! No CRASH_EXIT ACK from CP\n", ld->mc->name);

	if (!wake_lock_active(&pld->wlock))
		wake_lock(&pld->wlock);

	handle_cp_crash(pld);
}

static int trigger_force_cp_crash(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;

	if (ld->mode == LINK_MODE_ULOAD) {
		mif_err("%s: CP crash is already in progress\n", ld->mc->name);
		return 0;
	}

	ld->mode = LINK_MODE_ULOAD;
	mif_err("%s: called by %pf\n", ld->name, __builtin_return_address(0));

	send_intr(pld, INT_CMD(INT_CMD_CRASH_EXIT));

	mif_add_timer(&pld->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			handle_no_crash_ack, (unsigned long)pld);

	return 0;
}

static int pld_init_ipc(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	int i;

	if (ld->mode == LINK_MODE_IPC &&
	    get_magic(pld) == DPRAM_MAGIC_CODE &&
	    get_access(pld) == 1)
		mif_info("%s: IPC already initialized\n", ld->name);

	/* Clear pointers in every circular queue */
	for (i = 0; i < ld->max_ipc_dev; i++) {
		set_tx_head(pld, i, 0);
		set_tx_tail(pld, i, 0);
		set_rx_head(pld, i, 0);
		set_rx_tail(pld, i, 0);
	}

	/* Initialize variables for efficient TX/RX processing */
	for (i = 0; i < ld->max_ipc_dev; i++)
		pld->iod[i] = link_get_iod_with_format(ld, i);
	pld->iod[IPC_RAW] = link_get_iod_with_format(ld, IPC_MULTI_RAW);

	for (i = 0; i < ld->max_ipc_dev; i++)
		atomic_set(&pld->res_required[i], 0);

	spin_lock_init(&pld->tx_rx_lock);

	/* Enable IPC */
	atomic_set(&pld->accessing, 0);

	set_magic(pld, DPRAM_MAGIC_CODE);
	set_access(pld, 1);
	if (get_magic(pld) != DPRAM_MAGIC_CODE || get_access(pld) != 1)
		return -EACCES;

	ld->mode = LINK_MODE_IPC;

	if (wake_lock_active(&pld->wlock))
		wake_unlock(&pld->wlock);

	return 0;
}

static void cmd_req_active_handler(struct pld_link_device *pld)
{
	send_intr(pld, INT_CMD(INT_CMD_RES_ACTIVE));
}

static void cmd_crash_reset_handler(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	struct io_device *iod = NULL;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&pld->wlock))
		wake_lock(&pld->wlock);

	mif_err("%s: Recv 0xC7 (CRASH_RESET)\n", ld->name);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);

	iod = link_get_iod_with_format(ld, IPC_BOOT);
	iod->modem_state_changed(iod, STATE_CRASH_RESET);
}

static void cmd_crash_exit_handler(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&pld->wlock))
		wake_lock(&pld->wlock);

	mif_err("%s: Recv 0xC9 (CRASH_EXIT)\n", ld->name);

	del_timer(&pld->crash_ack_timer);

	if (pld->ext_op && pld->ext_op->crash_log)
		pld->ext_op->crash_log(pld);

	handle_cp_crash(pld);
}

static void cmd_phone_start_handler(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	struct io_device *iod = NULL;

	mif_info("%s: Recv 0xC8 (CP_START)\n", ld->name);

	pld_init_ipc(pld);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (!iod) {
		mif_info("%s: ERR! no iod\n", ld->name);
		return;
	}

	if (pld->ext_op && pld->ext_op->cp_start_handler)
		pld->ext_op->cp_start_handler(pld);

	if (ld->mc->phone_state != STATE_ONLINE) {
		mif_info("%s: phone_state: %d -> ONLINE\n",
			ld->name, ld->mc->phone_state);
		iod->modem_state_changed(iod, STATE_ONLINE);
	}

	mif_info("%s: Send 0xC2 (INIT_END)\n", ld->name);
	send_intr(pld, INT_CMD(INT_CMD_INIT_END));
}

static void command_handler(struct pld_link_device *pld, u16 cmd)
{
	struct link_device *ld = &pld->ld;

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		cmd_req_active_handler(pld);
		break;

	case INT_CMD_CRASH_RESET:
		pld->init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_reset_handler(pld);
		break;

	case INT_CMD_CRASH_EXIT:
		pld->init_status = DPRAM_INIT_STATE_NONE;
		cmd_crash_exit_handler(pld);
		break;

	case INT_CMD_PHONE_START:
		pld->init_status = DPRAM_INIT_STATE_READY;
		cmd_phone_start_handler(pld);
		complete_all(&pld->dpram_init_cmd);
		break;

	case INT_CMD_NV_REBUILDING:
		mif_info("%s: NV_REBUILDING\n", ld->name);
		break;

	case INT_CMD_PIF_INIT_DONE:
		complete_all(&pld->modem_pif_init_done);
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

static irqreturn_t pld_irq_handler(int irq, void *data)
{
	struct pld_link_device *pld = (struct pld_link_device *)data;
	struct link_device *ld = (struct link_device *)&pld->ld;
	u16 int2ap = 0;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	int2ap = recv_intr(pld);

	if (unlikely(int2ap == INT_POWERSAFE_FAIL)) {
		mif_info("%s: int2ap == INT_POWERSAFE_FAIL\n", ld->name);
		goto exit;
	} else if (int2ap == 0x1234 || int2ap == 0xDBAB || int2ap == 0xABCD) {
		if (pld->ext_op && pld->ext_op->dload_cmd_handler) {
			pld->ext_op->dload_cmd_handler(pld, int2ap);
			goto exit;
		}
	}

	if (likely(INT_VALID(int2ap))) {
		if (unlikely(INT_CMD_VALID(int2ap)))
			command_handler(pld, int2ap);
		else
			non_command_handler(pld, int2ap);
	} else {
		mif_info("%s: ERR! invalid intr 0x%04X\n",
			ld->name, int2ap);
	}

exit:
	clear_intr(pld);
	return IRQ_HANDLED;
}

static void pld_send_ipc(struct link_device *ld, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct pld_link_device *pld = to_pld_link_device(ld);
	struct sk_buff_head *txq = ld->skb_txq[dev];
	int ret;
	u16 mask;

	skb_queue_tail(txq, skb);
	if (txq->qlen > 1024) {
		mif_debug("%s: %s txq->qlen %d > 1024\n",
			ld->name, get_dev_name(dev), txq->qlen);
	}

	if (!ipc_active(pld))
		goto exit;

	if (atomic_read(&pld->res_required[dev]) > 0) {
		mif_debug("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		goto exit;
	}

	ret = pld_try_ipc_tx(pld, dev);
	if (ret > 0) {
		mask = get_mask_send(pld, dev);
		send_intr(pld, INT_NON_CMD(mask));
	} else if (ret == -ENOSPC) {
		mask = get_mask_req_ack(pld, dev);
		send_intr(pld, INT_NON_CMD(mask));
		mif_info("%s: Send REQ_ACK 0x%04X\n", ld->name, mask);
	} else {
		mif_info("%s: pld_try_ipc_tx fail (err %d)\n", ld->name, ret);
	}

exit:
	return;
}

static int pld_send(struct link_device *ld, struct io_device *iod,
		struct sk_buff *skb)
{
	enum dev_format dev = iod->format;
	int len = skb->len;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		if (likely(ld->mode == LINK_MODE_IPC)) {
			pld_send_ipc(ld, dev, iod, skb);
		} else {
			mif_info("%s: ld->mode != LINK_MODE_IPC\n", ld->name);
			dev_kfree_skb_any(skb);
		}
		return len;

	default:
		mif_info("%s: ERR! no TXQ for %s\n", ld->name, iod->name);
		dev_kfree_skb_any(skb);
		return -ENODEV;
	}
}

static int pld_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct pld_link_device *pld = to_pld_link_device(ld);
	trigger_force_cp_crash(pld);
	return 0;
}

static int pld_dump_start(struct link_device *ld, struct io_device *iod)
{
	struct pld_link_device *pld = to_pld_link_device(ld);

	if (pld->ext_op && pld->ext_op->dump_start)
		return pld->ext_op->dump_start(pld);
	else
		return -ENODEV;
}

static int pld_dump_update(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct pld_link_device *pld = to_pld_link_device(ld);

	if (pld->ext_op && pld->ext_op->dump_update)
		return pld->ext_op->dump_update(pld, (void *)arg);
	else
		return -ENODEV;
}

static int pld_ioctl(struct link_device *ld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct pld_link_device *pld = to_pld_link_device(ld);
	int err = 0;

/*
	mif_info("%s: cmd 0x%08X\n", ld->name, cmd);
*/

	switch (cmd) {
	case IOCTL_DPRAM_INIT_STATUS:
		mif_debug("%s: get dpram init status\n", ld->name);
		return pld->init_status;

	default:
		if (pld->ext_ioctl) {
			err = pld->ext_ioctl(pld, iod, cmd, arg);
		} else {
			mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
			err = -EINVAL;
		}

		break;
	}

	return err;
}

static int pld_table_init(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	u8 __iomem *dp_base;
	int i;

	if (!pld->base) {
		mif_info("%s: ERR! pld->base == NULL\n", ld->name);
		return -EINVAL;
	}
	dp_base = pld->base;

	/* Map for IPC */
	if (pld->dpctl->ipc_map) {
		memcpy(&pld->ipc_map, pld->dpctl->ipc_map,
			sizeof(struct dpram_ipc_map));
	}

	pld->magic_ap2cp = pld->ipc_map.magic_ap2cp;
	pld->access_ap2cp = pld->ipc_map.access_ap2cp;

	pld->magic_cp2ap = pld->ipc_map.magic_cp2ap;
	pld->access_cp2ap = pld->ipc_map.access_cp2ap;

	pld->address_buffer = pld->ipc_map.address_buffer;

	for (i = 0; i < ld->max_ipc_dev; i++)
		pld->dev[i] = &pld->ipc_map.dev[i];
	pld->mbx2ap = pld->ipc_map.mbx_cp2ap;
	pld->mbx2cp = pld->ipc_map.mbx_ap2cp;

	/* Map for booting */
	if (pld->ext_op && pld->ext_op->init_boot_map) {
		pld->ext_op->init_boot_map(pld);
	} else {
		pld->bt_map.magic = (u32 *)(dp_base);
		pld->bt_map.buff = (u8 *)(dp_base + DP_BOOT_BUFF_OFFSET);
		pld->bt_map.size = pld->size - 8;
	}

	/* Map for download (FOTA, UDL, etc.) */
	if (pld->ext_op && pld->ext_op->init_dl_map) {
		pld->ext_op->init_dl_map(pld);
	} else {
		pld->dl_map.magic = (u32 *)(dp_base);
		pld->dl_map.buff = (u8 *)(dp_base + DP_DLOAD_BUFF_OFFSET);
	}

	/* Map for upload mode */
	if (pld->ext_op && pld->ext_op->init_ul_map) {
		pld->ext_op->init_ul_map(pld);
	} else {
		pld->ul_map.magic = (u32 *)(dp_base);
		pld->ul_map.buff = (u8 *)(dp_base + DP_ULOAD_BUFF_OFFSET);
	}

	return 0;
}

static void pld_setup_common_op(struct pld_link_device *pld)
{
	pld->clear_intr = clear_intr;
	pld->recv_intr = recv_intr;
	pld->send_intr = send_intr;
	pld->get_magic = get_magic;
	pld->set_magic = set_magic;
	pld->get_access = get_access;
	pld->set_access = set_access;
	pld->get_tx_head = get_tx_head;
	pld->get_tx_tail = get_tx_tail;
	pld->set_tx_head = set_tx_head;
	pld->set_tx_tail = set_tx_tail;
	pld->get_tx_buff = get_tx_buff;
	pld->get_tx_buff_size = get_tx_buff_size;
	pld->get_rx_head = get_rx_head;
	pld->get_rx_tail = get_rx_tail;
	pld->set_rx_head = set_rx_head;
	pld->set_rx_tail = set_rx_tail;
	pld->get_rx_buff = get_rx_buff;
	pld->get_rx_buff_size = get_rx_buff_size;
	pld->get_mask_req_ack = get_mask_req_ack;
	pld->get_mask_res_ack = get_mask_res_ack;
	pld->get_mask_send = get_mask_send;
}

static int pld_link_init(struct link_device *ld, struct io_device *iod)
{
	return 0;
}

static void pld_link_terminate(struct link_device *ld, struct io_device *iod)
{
	return;
}

struct link_device *pld_create_link_device(struct platform_device *pdev)
{
	struct modem_data *mdm_data = NULL;
	struct pld_link_device *pld = NULL;
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
	pld = kzalloc(sizeof(struct pld_link_device), GFP_KERNEL);
	if (!pld) {
		mif_info("ERR! kzalloc pld fail\n");
		goto err;
	}
	ld = &pld->ld;

	/* Retrieve modem data and DPRAM control data from the modem data */
	ld->mdm_data = mdm_data;
	ld->name = mdm_data->link_name;
	ld->ipc_version = mdm_data->ipc_version;

	/* Retrieve the most basic data for IPC from the modem data */
	pld->dpctl = dpctl;
	pld->type = dpctl->dp_type;

	if (mdm_data->ipc_version < SIPC_VER_50) {
		if (!dpctl->max_ipc_dev) {
			mif_info("ERR! no max_ipc_dev\n");
			goto err;
		}

		ld->aligned = dpctl->aligned;
		ld->max_ipc_dev = dpctl->max_ipc_dev;
	} else {
		ld->aligned = 1;
		ld->max_ipc_dev = MAX_SIPC5_DEV;
	}

	/* Set attributes as a link device */
	ld->init_comm = pld_link_init;
	ld->terminate_comm = pld_link_terminate;
	ld->send = pld_send;
	ld->force_dump = pld_force_dump;
	ld->dump_start = pld_dump_start;
	ld->dump_update = pld_dump_update;
	ld->ioctl = pld_ioctl;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	skb_queue_head_init(&ld->sk_rfs_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;
	ld->skb_txq[IPC_RFS] = &ld->sk_rfs_tx_q;

	/* Set up function pointers */
	pld_setup_common_op(pld);
	pld->ext_op = pld_get_ext_op(mdm_data->modem_type);
	if (pld->ext_op && pld->ext_op->ioctl)
		pld->ext_ioctl = pld->ext_op->ioctl;

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
	pld->base = dpctl->dp_base;
	pld->size = dpctl->dp_size;

	mif_info("%s: type %d, aligned %d, base 0x%08X, size %d\n",
		ld->name, pld->type, ld->aligned, (int)pld->base, pld->size);

	/* Initialize DPRAM map (physical map -> logical map) */
	ret = pld_table_init(pld);
	if (ret < 0) {
		mif_info("%s: ERR! pld_table_init fail (err %d)\n",
			ld->name, ret);
		goto err;
	}

	spin_lock_init(&pld->pld_lock);

	/* Disable IPC */
	set_magic(pld, 0);
	set_access(pld, 0);

	pld->init_status = DPRAM_INIT_STATE_NONE;

	/* Initialize locks, completions, and bottom halves */
	snprintf(pld->wlock_name, MIF_MAX_NAME_LEN, "%s_wlock", ld->name);
	wake_lock_init(&pld->wlock, WAKE_LOCK_SUSPEND, pld->wlock_name);

	init_completion(&pld->dpram_init_cmd);
	init_completion(&pld->modem_pif_init_done);
	init_completion(&pld->udl_start_complete);
	init_completion(&pld->udl_cmd_complete);
	init_completion(&pld->crash_start_complete);
	init_completion(&pld->crash_recv_done);

	task_data = (unsigned long)pld;
	tasklet_init(&pld->rx_tsk, pld_ipc_rx_task, task_data);

	/* Prepare RXB queue */
	qsize = DPRAM_MAX_RXBQ_SIZE;
	for (i = 0; i < ld->max_ipc_dev; i++) {
		bsize = rxbq_get_page_size(get_rx_buff_size(pld, i));
		pld->rxbq[i].size = qsize;
		pld->rxbq[i].in = 0;
		pld->rxbq[i].out = 0;
		pld->rxbq[i].rxb = rxbq_create_pool(bsize, qsize);
		if (!pld->rxbq[i].rxb) {
			mif_info("%s: ERR! %s rxbq_create_pool fail\n",
				ld->name, get_dev_name(i));
			goto err;
		}
		mif_info("%s: %s rxbq_pool created (bsize:%d, qsize:%d)\n",
			ld->name, get_dev_name(i), bsize, qsize);
	}

	/* Prepare a multi-purpose miscellaneous buffer */
	pld->buff = kzalloc(pld->size, GFP_KERNEL);
	if (!pld->buff) {
		mif_info("%s: ERR! kzalloc pld->buff fail\n", ld->name);
		goto err;
	}

	/* Retrieve DPRAM IRQ GPIO# */
	pld->gpio_dpram_int = mdm_data->gpio_dpram_int;

	/* Retrieve DPRAM IRQ# */
	if (!dpctl->dpram_irq) {
		dpctl->dpram_irq = platform_get_irq_byname(pdev, "dpram_irq");
		if (dpctl->dpram_irq < 0) {
			mif_info("%s: ERR! platform_get_irq_byname fail\n",
				ld->name);
			goto err;
		}
	}
	pld->irq = dpctl->dpram_irq;

	/* Retrieve DPRAM IRQ flags */
	if (!dpctl->dpram_irq_flags)
		dpctl->dpram_irq_flags = (IRQF_NO_SUSPEND | IRQF_TRIGGER_LOW);
	pld->irq_flags = dpctl->dpram_irq_flags;

	/* Register DPRAM interrupt handler */
	snprintf(pld->irq_name, MIF_MAX_NAME_LEN, "%s_irq", ld->name);
	ret = pld_register_isr(pld->irq, pld_irq_handler, pld->irq_flags,
				pld->irq_name, pld);
	if (ret)
		goto err;

	return ld;

err:
	if (pld) {
		kfree(pld->buff);
		kfree(pld);
	}

	return NULL;
}

