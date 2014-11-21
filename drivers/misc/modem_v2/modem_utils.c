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

#include <linux/netdevice.h>
#include <linux/platform_data/modem_v2.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <net/ip.h>

#include "modem_prj.h"
#include "modem_utils.h"

#define CMD_SUSPEND	((unsigned short)(0x00CA))
#define CMD_RESUME	((unsigned short)(0x00CB))

#define TX_SEPARATOR	"mif: >>>>>>>>>> Outgoing packet "
#define RX_SEPARATOR	"mif: Incoming packet <<<<<<<<<<"
#define LINE_SEPARATOR	\
	"mif: ------------------------------------------------------------"
#define LINE_BUFF_SIZE	80

#ifdef CONFIG_LINK_DEVICE_DPRAM
#include "modem_link_device_dpram.h"
int mif_dump_dpram(struct io_device *iod)
{
	struct link_device *ld = get_current_link(iod);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u32 size = dpld->dp_size;
	unsigned long read_len = 0;
	struct sk_buff *skb;
	char *buff;

	buff = kzalloc(size, GFP_ATOMIC);
	if (!buff) {
		pr_err("[MIF] <%s> alloc dpram buff  failed\n", __func__);
		return -ENOMEM;
	} else {
		dpld->dpram_dump(ld, buff);
	}

	while (read_len < size) {
		skb = alloc_skb(MAX_IPC_SKB_SIZE, GFP_ATOMIC);
		if (!skb) {
			pr_err("[MIF] <%s> alloc skb failed\n", __func__);
			kfree(buff);
			return -ENOMEM;
		}
		memcpy(skb_put(skb, MAX_IPC_SKB_SIZE),
			buff + read_len, MAX_IPC_SKB_SIZE);
		skb_queue_tail(&iod->sk_rx_q, skb);
		read_len += MAX_IPC_SKB_SIZE;
		wake_up(&iod->wq);
	}
	kfree(buff);
	return 0;
}
#endif

int mif_dump_log(struct modem_shared *msd, struct io_device *iod)
{
	struct sk_buff *skb;
	unsigned long read_len = 0;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);
	while (read_len < MAX_MIF_BUFF_SIZE) {
		skb = alloc_skb(MAX_IPC_SKB_SIZE, GFP_ATOMIC);
		if (!skb) {
			pr_err("[MIF] <%s> alloc skb failed\n", __func__);
			spin_unlock_irqrestore(&msd->lock, flags);
			return -ENOMEM;
		}
		memcpy(skb_put(skb, MAX_IPC_SKB_SIZE),
			msd->storage.addr + read_len, MAX_IPC_SKB_SIZE);
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = get_current_link(iod);
		skb_queue_tail(&iod->sk_rx_q, skb);
		read_len += MAX_IPC_SKB_SIZE;
		wake_up(&iod->wq);
	}
	spin_unlock_irqrestore(&msd->lock, flags);
	return 0;
}

static unsigned long long get_kernel_time(void)
{
	int this_cpu;
	unsigned long flags;
	unsigned long long time;

	local_irq_save(flags);

	this_cpu = smp_processor_id();
	time = cpu_clock(this_cpu);

	local_irq_restore(flags);

	return time;
}

void mif_ipc_log(enum mif_log_id id,
	struct modem_shared *msd, const char *data, size_t len)
{
	struct mif_ipc_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_ipc_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	block->len = (len > MAX_IPC_LOG_SIZE) ? MAX_IPC_LOG_SIZE : len;
	memcpy(block->buff, data, block->len);
}

void _mif_irq_log(enum mif_log_id id, struct modem_shared *msd,
	struct mif_irq_map map, const char *data, size_t len)
{
	struct mif_irq_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_irq_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	memcpy(&(block->map), &map, sizeof(struct mif_irq_map));
	if (data)
		memcpy(block->buff, data,
			(len > MAX_IRQ_LOG_SIZE) ? MAX_IRQ_LOG_SIZE : len);
}

void _mif_com_log(enum mif_log_id id,
	struct modem_shared *msd, const char *format, ...)
{
	struct mif_common_block *block;
	unsigned long int flags;
	va_list args;
	int ret;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_common_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();

	va_start(args, format);
	ret = vsnprintf(block->buff, MAX_COM_LOG_SIZE, format, args);
	va_end(args);
}

void _mif_time_log(enum mif_log_id id, struct modem_shared *msd,
	struct timespec epoch, const char *data, size_t len)
{
	struct mif_time_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_time_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	memcpy(&block->epoch, &epoch, sizeof(struct timespec));

	if (data)
		memcpy(block->buff, data,
			(len > MAX_IRQ_LOG_SIZE) ? MAX_IRQ_LOG_SIZE : len);
}

/* dump2hex
 * dump data to hex as fast as possible.
 * the length of @buf must be greater than "@len * 3"
 * it need 3 bytes per one data byte to print.
 */
static inline int dump2hex(char *buf, const char *data, size_t len)
{
	static const char *hex = "0123456789abcdef";
	char *dest = buf;
	int i;

	for (i = 0; i < len; i++) {
		*dest++ = hex[(data[i] >> 4) & 0xf];
		*dest++ = hex[data[i] & 0xf];
		*dest++ = ' ';
	}
	if (likely(len > 0))
		dest--; /* last space will be overwrited with null */

	*dest = '\0';

	return dest - buf;
}

int pr_ipc(const char *str, const char *data, size_t len)
{
	struct timeval tv;
	struct tm date;
	unsigned char hexstr[128];

	do_gettimeofday(&tv);
	time_to_tm((tv.tv_sec - sys_tz.tz_minuteswest * 60), 0, &date);

	dump2hex(hexstr, data, (len > 40 ? 40 : len));

	return pr_info("mif: %s: [%02d-%02d %02d:%02d:%02d.%03ld] %s\n",
			str, date.tm_mon + 1, date.tm_mday,
			date.tm_hour, date.tm_min, date.tm_sec,
			(tv.tv_usec > 0 ? tv.tv_usec / 1000 : 0), hexstr);
}

/* print buffer as hex string */
int pr_buffer(const char *tag, const char *data, size_t data_len,
							size_t max_len)
{
	size_t len = min(data_len, max_len);
	unsigned char hexstr[len ? len * 3 : 1]; /* 1 <= sizeof <= max_len*3 */
	dump2hex(hexstr, data, len);

	/* don't change this printk to mif_debug for print this as level7 */
	return printk(KERN_INFO "mif: %s(%u): %s%s\n", tag, data_len, hexstr,
			len == data_len ? "" : " ...");
}

/* flow control CM from CP, it use in serial devices */
int link_rx_flowctl_cmd(struct link_device *ld, const char *data, size_t len)
{
	struct modem_shared *msd = ld->msd;
	unsigned short *cmd, *end = (unsigned short *)(data + len);

	mif_debug("flow control cmd: size=%d\n", len);

	for (cmd = (unsigned short *)data; cmd < end; cmd++) {
		switch (*cmd) {
		case CMD_SUSPEND:
			iodevs_for_each(msd, iodev_netif_stop, 0);
			ld->raw_tx_suspended = true;
			mif_info("flowctl CMD_SUSPEND(%04X)\n", *cmd);
			break;

		case CMD_RESUME:
			iodevs_for_each(msd, iodev_netif_wake, 0);
			ld->raw_tx_suspended = false;
			complete_all(&ld->raw_tx_resumed_by_cp);
			mif_info("flowctl CMD_RESUME(%04X)\n", *cmd);
			break;

		default:
			mif_err("flowctl BACMD: %04X\n", *cmd);
			break;
		}
	}

	return 0;
}

struct io_device *get_iod_with_channel(struct modem_shared *msd,
					unsigned channel)
{
	struct rb_node *n = msd->iodevs_tree_chan.rb_node;
	struct io_device *iodev;
	while (n) {
		iodev = rb_entry(n, struct io_device, node_chan);
		if (channel < iodev->id)
			n = n->rb_left;
		else if (channel > iodev->id)
			n = n->rb_right;
		else
			return iodev;
	}
	return NULL;
}

struct io_device *get_iod_with_format(struct modem_shared *msd,
			enum dev_format format)
{
	struct rb_node *n = msd->iodevs_tree_fmt.rb_node;
	struct io_device *iodev;
	while (n) {
		iodev = rb_entry(n, struct io_device, node_fmt);
		if (format < iodev->format)
			n = n->rb_left;
		else if (format > iodev->format)
			n = n->rb_right;
		else
			return iodev;
	}
	return NULL;
}

struct io_device *insert_iod_with_channel(struct modem_shared *msd,
		unsigned channel, struct io_device *iod)
{
	struct rb_node **p = &msd->iodevs_tree_chan.rb_node;
	struct rb_node *parent = NULL;
	struct io_device *iodev;
	while (*p) {
		parent = *p;
		iodev = rb_entry(parent, struct io_device, node_chan);
		if (channel < iodev->id)
			p = &(*p)->rb_left;
		else if (channel > iodev->id)
			p = &(*p)->rb_right;
		else
			return iodev;
	}
	rb_link_node(&iod->node_chan, parent, p);
	rb_insert_color(&iod->node_chan, &msd->iodevs_tree_chan);
	return NULL;
}

struct io_device *insert_iod_with_format(struct modem_shared *msd,
		enum dev_format format, struct io_device *iod)
{
	struct rb_node **p = &msd->iodevs_tree_fmt.rb_node;
	struct rb_node *parent = NULL;
	struct io_device *iodev;
	while (*p) {
		parent = *p;
		iodev = rb_entry(parent, struct io_device, node_fmt);
		if (format < iodev->format)
			p = &(*p)->rb_left;
		else if (format > iodev->format)
			p = &(*p)->rb_right;
		else
			return iodev;
	}
	rb_link_node(&iod->node_fmt, parent, p);
	rb_insert_color(&iod->node_fmt, &msd->iodevs_tree_fmt);
	return NULL;
}

void iodevs_for_each(struct modem_shared *msd, action_fn action, void *args)
{
	struct io_device *iod;
	struct rb_node *node = rb_first(&msd->iodevs_tree_chan);
	for (; node; node = rb_next(node)) {
		iod = rb_entry(node, struct io_device, node_chan);
		action(iod, args);
	}
}

void iodev_netif_wake(struct io_device *iod, void *args)
{
	if (iod->io_typ == IODEV_NET && iod->ndev) {
		netif_wake_queue(iod->ndev);
		mif_info("%s\n", iod->name);
	}
}

void iodev_netif_stop(struct io_device *iod, void *args)
{
	if (iod->io_typ == IODEV_NET && iod->ndev) {
		netif_stop_queue(iod->ndev);
		mif_info("%s\n", iod->name);
	}
}

static void iodev_set_tx_link(struct io_device *iod, void *args)
{
	struct link_device *ld = (struct link_device *)args;
	if (iod->format == IPC_RAW && IS_CONNECTED(iod, ld)) {
		set_current_link(iod, ld);
		mif_err("%s -> %s\n", iod->name, ld->name);
	}
}

void rawdevs_set_tx_link(struct modem_shared *msd, enum modem_link link_type)
{
	struct link_device *ld = find_linkdev(msd, link_type);
	if (ld)
		iodevs_for_each(msd, iodev_set_tx_link, ld);
}

/**
 * ipv4str_to_be32 - ipv4 string to be32 (big endian 32bits integer)
 * @return: return zero when errors occurred
 */
__be32 ipv4str_to_be32(const char *ipv4str, size_t count)
{
	unsigned char ip[4];
	char ipstr[16]; /* == strlen("xxx.xxx.xxx.xxx") + 1 */
	char *next = ipstr;
	char *p;
	int i;

	strncpy(ipstr, ipv4str, ARRAY_SIZE(ipstr));

	for (i = 0; i < 4; i++) {
		p = strsep(&next, ".");
		if (kstrtou8(p, 10, &ip[i]) < 0)
			return 0; /* == 0.0.0.0 */
	}

	return *((__be32 *)ip);
}

void mif_add_timer(struct timer_list *timer, unsigned long expire,
		void (*function)(unsigned long), unsigned long data)
{
	if (timer_pending(timer))
		return;

	init_timer(timer);
	timer->expires = get_jiffies_64() + expire;
	timer->function = function;
	timer->data = data;
	add_timer(timer);
}

/* skb pool functions */
int mif_skb_pool_init(struct mif_skb_pool *msp, size_t u_size, int p_len,
	gfp_t mask)
{
	int len;
	struct sk_buff *skb;

	/* double initialize prohibited */
	if (msp->default_len)
		return -EINVAL;

	msp->default_len = p_len;
	skb_queue_head_init(&msp->pend);
	skb_queue_head_init(&msp->free);

	for (len = 0; len <= p_len; len++) {
		skb = alloc_skb(u_size, mask);
		if (unlikely(!skb))
			break;
		skb_queue_tail(&msp->free, skb);
	}
	if (len) {
		atomic_set(&msp->pool_len, msp->free.qlen);
		msp->unit_size = skb->truesize;
	}

	mif_info("unit size = %d, free qlen = %d\n", msp->unit_size,
		msp->free.qlen);
	return msp->free.qlen;
}

struct sk_buff *mif_skb_pool_alloc(struct mif_skb_pool *msp)
{
	struct sk_buff *skb;

	if (!msp->free.qlen && msp->pend.qlen) {
		int pend_len = msp->pend.qlen;
		while (pend_len--) {
			skb = skb_dequeue(&msp->pend);
			if (unlikely(!skb)) /*unexpected*/
				break;
			if (atomic_read(&(skb_shinfo(skb)->dataref)) == 1)
				skb_queue_tail(&msp->free, skb);
			else
				skb_queue_tail(&msp->pend, skb);
		}
	}

	if (msp->free.qlen < 50 && (msp->free.qlen % 10 == 1))
		printk(KERN_DEBUG "%s: f(%d), p(%d)\n", __func__,
			msp->free.qlen,	msp->pend.qlen);

	skb = skb_dequeue(&msp->free);
	if (unlikely(!skb)) {
		mif_err("free = %d, pend = %d\n", msp->free.qlen,
			msp->pend.qlen);
		return NULL;
	}

	if (unlikely(skb->truesize != msp->unit_size)) {
		mif_err("wrong skb true size = %d, unit size = %d\n",
					skb->truesize, msp->unit_size);
		dev_kfree_skb_any(skb);
		atomic_dec(&msp->pool_len);
	}
	if (unlikely(atomic_read(&(skb_shinfo(skb)->dataref)) != 1)) {
		mif_err("wrong dataref = %d\n",
			atomic_read(&(skb_shinfo(skb)->dataref)));
		dev_kfree_skb_any(skb);
		atomic_dec(&msp->pool_len);
	}
	/* clear the sk headers */
	memset(skb, 0, offsetof(struct sk_buff, tail));
	skb->data = skb->head;
	skb_reset_tail_pointer(skb);

	return skb;
}

void mif_skb_pool_free(struct mif_skb_pool *msp, struct sk_buff *skb)
{

	if (!skb || atomic_read(&(skb_shinfo(skb)->dataref)) < 1) {
		mif_err("invalid skb\n");
		return;
	}
	if (unlikely(skb->truesize != msp->unit_size)) {
		mif_err("wrong skb true size = %d, unit size = %d\n",
					skb->truesize, msp->unit_size);
		dev_kfree_skb_any(skb);
		return;
	}

	if (likely(atomic_read(&(skb_shinfo(skb)->dataref)) == 1))
		skb_queue_tail(&msp->free, skb);
	else
		skb_queue_tail(&msp->pend, skb);

	mif_debug("free = %d, pend = %d\n", msp->free.qlen, msp->pend.qlen);
	return;
}

unsigned int mif_skb_pool_free_qlen(struct mif_skb_pool *msp)
{
	return msp->free.qlen;
}

struct sk_buff *mif_skb_pool_alloc_inc(struct mif_skb_pool *msp, gfp_t mask)
{
	struct sk_buff *skb;

	skb = mif_skb_pool_alloc(msp);
	if (unlikely(!skb)) {
		skb = alloc_skb(msp->unit_size, mask);
		if (unlikely(!skb))
			return NULL;
		atomic_inc(&msp->pool_len);
	}
	return skb;
}

void mif_skb_pool_free_dec(struct mif_skb_pool *msp, struct sk_buff *skb)
{
	if (!skb || atomic_read(&(skb_shinfo(skb)->dataref)) < 1) {
		mif_err("invalid skb\n");
		return;
	}

	if (atomic_read(&msp->pool_len) > msp->default_len) {
		atomic_dec(&msp->pool_len);
		dev_kfree_skb_any(skb);
		return;
	}
	mif_skb_pool_free(msp, skb);
	return;
}

void mif_print_data(char *buf, int len)
{
	int words = len >> 4;
	int residue = len - (words << 4);
	int i;
	char *b;
	char last[80];
	char tb[8];

	/* Make the last line, if ((len % 16) > 0) */
	if (residue > 0) {
		memset(last, 0, sizeof(last));
		memset(tb, 0, sizeof(tb));
		b = buf + (words << 4);

		sprintf(last, "%04X: ", (words << 4));
		for (i = 0; i < residue; i++) {
			sprintf(tb, "%02x ", b[i]);
			strcat(last, tb);
			if ((i & 0x3) == 0x3) {
				sprintf(tb, " ");
				strcat(last, tb);
			}
		}
	}

	for (i = 0; i < words; i++) {
		b = buf + (i << 4);
		/* TODO: print buf*/
	}

	/* Print the last line */
	if (residue > 0)
		mif_err("%s\n", last);
}

void print_sipc4_hdlc_fmt_frame(const u8 *psrc)
{
	u8 *frm;			/* HDLC Frame	*/
	struct fmt_hdr *hh;		/* HDLC Header	*/
	struct sipc_main_hdr *fh;	/* IPC Header	*/
	u16 hh_len = sizeof(struct fmt_hdr);
	u16 fh_len = sizeof(struct sipc_main_hdr);
	u8 *data;
	int dlen;

	/* Actual HDLC header starts from after START flag (0x7F) */
	frm = (u8 *)(psrc + 1);

	/* Point HDLC header and IPC header */
	hh = (struct fmt_hdr *)(frm);
	fh = (struct sipc_main_hdr *)(frm + hh_len);

	/* Point IPC data */
	data = frm + (hh_len + fh_len);
	dlen = hh->len - (hh_len + fh_len);

	mif_err("--------------------HDLC & FMT HEADER----------------------\n");

	mif_err("HDLC: length %d, control 0x%02x\n", hh->len, hh->control);

	mif_err("(M)0x%02X, (S)0x%02X, (T)0x%02X, mseq %d, aseq %d, len %d\n",
		fh->main_cmd, fh->sub_cmd, fh->cmd_type,
		fh->msg_seq, fh->ack_seq, fh->len);

	mif_err("-----------------------IPC FMT DATA------------------------\n");

	if (dlen > 0) {
		if (dlen > 64)
			dlen = 64;
		mif_print_data(data, dlen);
	}

	mif_err("-----------------------------------------------------------\n");
}

void print_sipc4_fmt_frame(const u8 *psrc)
{
	struct sipc_main_hdr *fh = (struct sipc_main_hdr *)psrc;
	u16 fh_len = sizeof(struct sipc_main_hdr);
	u8 *data;
	int dlen;

	/* Point IPC data */
	data = (u8 *)(psrc + fh_len);
	dlen = fh->len - fh_len;

	mif_err("----------------------IPC FMT HEADER-----------------------\n");

	mif_err("(M)0x%02X, (S)0x%02X, (T)0x%02X, mseq:%d, aseq:%d, len:%d\n",
		fh->main_cmd, fh->sub_cmd, fh->cmd_type,
		fh->msg_seq, fh->ack_seq, fh->len);

	mif_err("-----------------------IPC FMT DATA------------------------\n");

	if (dlen > 0)
		mif_print_data(data, dlen);

	mif_err("-----------------------------------------------------------\n");
}
#if 0
void print_sipc5_link_fmt_frame(const u8 *psrc)
{
	u8 *lf;				/* Link Frame	*/
	struct sipc5_link_hdr *lh;	/* Link Header	*/
	struct sipc_fmt_hdr *fh;	/* IPC Header	*/
	u16 lh_len;
	u16 fh_len;
	u8 *data;
	int dlen;

	lf = (u8 *)psrc;

	/* Point HDLC header and IPC header */
	lh = (struct sipc5_link_hdr *)lf;
	if (lh->cfg & SIPC5_CTL_FIELD_EXIST)
		lh_len = SIPC5_HEADER_SIZE_WITH_CTL_FLD;
	else
		lh_len = SIPC5_MIN_HEADER_SIZE;
	fh = (struct sipc_fmt_hdr *)(lf + lh_len);
	fh_len = sizeof(struct sipc_fmt_hdr);

	/* Point IPC data */
	data = lf + (lh_len + fh_len);
	dlen = lh->len - (lh_len + fh_len);

	mif_err("--------------------LINK & FMT HEADER----------------------\n");

	mif_err("LINK: cfg 0x%02X, ch %d, len %d\n", lh->cfg, lh->ch, lh->len);

	mif_err("(M)0x%02X, (S)0x%02X, (T)0x%02X, mseq:%d, aseq:%d, len:%d\n",
		fh->main_cmd, fh->sub_cmd, fh->cmd_type,
		fh->msg_seq, fh->ack_seq, fh->len);

	mif_err("-----------------------IPC FMT DATA------------------------\n");

	if (dlen > 0) {
		if (dlen > 64)
			dlen = 64;
		mif_print_data(data, dlen);
	}

	mif_err("-----------------------------------------------------------\n");
}
#endif
static void strcat_tcp_header(char *buff, u8 *pkt)
{
	struct tcphdr *tcph = (struct tcphdr *)pkt;
	int eol;
	char line[LINE_BUFF_SIZE];
	char flag_str[32];

/*-------------------------------------------------------------------------

				TCP Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Source Port          |       Destination Port        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Sequence Number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Acknowledgment Number                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Data |       |C|E|U|A|P|R|S|F|                               |
	| Offset| Rsvd  |W|C|R|C|S|S|Y|I|            Window             |
	|       |       |R|E|G|K|H|T|N|N|                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |         Urgent Pointer        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             data                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

-------------------------------------------------------------------------*/

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: TCP:: Src.Port %u, Dst.Port %u\n",
		ntohs(tcph->source), ntohs(tcph->dest));
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: TCP:: SEQ 0x%08X(%u), ACK 0x%08X(%u)\n",
		ntohs(tcph->seq), ntohs(tcph->seq),
		ntohs(tcph->ack_seq), ntohs(tcph->ack_seq));
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	memset(flag_str, 0, sizeof(flag_str));
	if (tcph->cwr)
		strcat(flag_str, "CWR ");
	if (tcph->ece)
		strcat(flag_str, "ECE");
	if (tcph->urg)
		strcat(flag_str, "URG ");
	if (tcph->ack)
		strcat(flag_str, "ACK ");
	if (tcph->psh)
		strcat(flag_str, "PSH ");
	if (tcph->rst)
		strcat(flag_str, "RST ");
	if (tcph->syn)
		strcat(flag_str, "SYN ");
	if (tcph->fin)
		strcat(flag_str, "FIN ");
	eol = strlen(flag_str) - 1;
	if (eol > 0)
		flag_str[eol] = 0;
	snprintf(line, LINE_BUFF_SIZE, "mif: TCP:: Flags {%s}\n", flag_str);
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: TCP:: Window %u, Checksum 0x%04X, Urg Pointer %u\n",
		ntohs(tcph->window), ntohs(tcph->check), ntohs(tcph->urg_ptr));
	strcat(buff, line);
}

static void strcat_udp_header(char *buff, u8 *pkt)
{
	struct udphdr *udph = (struct udphdr *)pkt;
	char line[LINE_BUFF_SIZE];

/*-------------------------------------------------------------------------

				UDP Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Source Port          |       Destination Port        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            Length             |           Checksum            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             data                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

-------------------------------------------------------------------------*/

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: UDP:: Src.Port %u, Dst.Port %u\n",
		ntohs(udph->source), ntohs(udph->dest));
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: UDP:: Length %u, Checksum 0x%04X\n",
		ntohs(udph->len), ntohs(udph->check));
	strcat(buff, line);

	if (ntohs(udph->dest) == 53) {
		memset(line, 0, LINE_BUFF_SIZE);
		snprintf(line, LINE_BUFF_SIZE, "mif: UDP:: DNS query!!!\n");
		strcat(buff, line);
	}

	if (ntohs(udph->source) == 53) {
		memset(line, 0, LINE_BUFF_SIZE);
		snprintf(line, LINE_BUFF_SIZE, "mif: UDP:: DNS response!!!\n");
		strcat(buff, line);
	}
}

void print_ip4_packet(u8 *ip_pkt, bool tx)
{
	char *buff;
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	u8 *pkt = ip_pkt + (iph->ihl << 2);
	u16 flags = (ntohs(iph->frag_off) & 0xE000);
	u16 frag_off = (ntohs(iph->frag_off) & 0x1FFF);
	int eol;
	char line[LINE_BUFF_SIZE];
	char flag_str[16];

/*---------------------------------------------------------------------------
				IPv4 Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|Version|  IHL  |Type of Service|          Total Length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         Identification        |C|D|M|     Fragment Offset     |
	|                               |E|F|F|                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Time to Live |    Protocol   |         Header Checksum       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Source Address                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Destination Address                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	IHL - Header Length
	Flags - Consist of 3 bits
		The 1st bit is "Congestion" bit.
		The 2nd bit is "Dont Fragment" bit.
		The 3rd bit is "More Fragments" bit.

---------------------------------------------------------------------------*/

	if (iph->version != 4)
		return;

	buff = kzalloc(4096, GFP_ATOMIC);
	if (!buff)
		return;


	memset(line, 0, LINE_BUFF_SIZE);
	if (tx)
		snprintf(line, LINE_BUFF_SIZE, "\n%s\n", TX_SEPARATOR);
	else
		snprintf(line, LINE_BUFF_SIZE, "\n%s\n", RX_SEPARATOR);
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE, "%s\n", LINE_SEPARATOR);
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: IP4:: Version %u, Header Length %u, TOS %u, Length %u\n",
		iph->version, (iph->ihl << 2), iph->tos, ntohs(iph->tot_len));
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: IP4:: ID %u, Fragment Offset %u\n",
		ntohs(iph->id), frag_off);
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	memset(flag_str, 0, sizeof(flag_str));
	if (flags & IP_CE)
		strcat(flag_str, "CE ");
	if (flags & IP_DF)
		strcat(flag_str, "DF ");
	if (flags & IP_MF)
		strcat(flag_str, "MF ");
	eol = strlen(flag_str) - 1;
	if (eol > 0)
		flag_str[eol] = 0;
	snprintf(line, LINE_BUFF_SIZE, "mif: IP4:: Flags {%s}\n", flag_str);
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: IP4:: TTL %u, Protocol %u, Header Checksum 0x%04X\n",
		iph->ttl, iph->protocol, ntohs(iph->check));
	strcat(buff, line);

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE,
		"mif: IP4:: Src.IP %u.%u.%u.%u, Dst.IP %u.%u.%u.%u\n",
		ip_pkt[12], ip_pkt[13], ip_pkt[14], ip_pkt[15],
		ip_pkt[16], ip_pkt[17], ip_pkt[18], ip_pkt[19]);
	strcat(buff, line);

	switch (iph->protocol) {
	case 6: /* TCP */
		strcat_tcp_header(buff, pkt);
		break;

	case 17: /* UDP */
		strcat_udp_header(buff, pkt);
		break;

	default:
		break;
	}

	memset(line, 0, LINE_BUFF_SIZE);
	snprintf(line, LINE_BUFF_SIZE, "%s\n", LINE_SEPARATOR);
	strcat(buff, line);

	pr_info("%s", buff);

	kfree(buff);
}

bool is_dns_packet(u8 *ip_pkt)
{
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	struct udphdr *udph = (struct udphdr *)(ip_pkt + (iph->ihl << 2));

	/* If this packet is not a UDP packet, return here. */
	if (iph->protocol != 17)
		return false;

	if (ntohs(udph->dest) == 53 || ntohs(udph->source) == 53)
		return true;
	else
		return false;
}

bool is_syn_packet(u8 *ip_pkt)
{
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	struct tcphdr *tcph = (struct tcphdr *)(ip_pkt + (iph->ihl << 2));

	/* If this packet is not a TCP packet, return here. */
	if (iph->protocol != 6)
		return false;

	if (tcph->syn || tcph->fin)
		return true;
	else
		return false;
}

int memcmp16_to_io(const void __iomem *to, void *from, int size)
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
			mif_err("ERR! [%d] d:0x%04X != s:0x%04X\n", i, d1, s1);
		}
		d++;
		s++;
	}

	return diff;
}

int mif_test_dpram(char *dp_name, u8 __iomem *start, u32 size)
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

