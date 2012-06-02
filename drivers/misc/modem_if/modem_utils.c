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
#include <linux/platform_data/modem.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ip.h>

#include "modem_prj.h"
#include "modem_utils.h"

#define CMD_SUSPEND	((unsigned short)(0x00CA))
#define CMD_RESUME	((unsigned short)(0x00CB))


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

/* print buffer as hex string */
int pr_buffer(const char *tag, const char *data, size_t data_len,
							size_t max_len)
{
	size_t len = min(data_len, max_len);
	unsigned char hexstr[len ? len * 3 : 1]; /* 1 <= sizeof <= max_len*3 */
	dump2hex(hexstr, data, len);

	/* don't change this printk to mif_debug for print this as level7 */
	return printk(KERN_DEBUG "%s(%u): %s%s\n", tag, data_len, hexstr,
			len == data_len ? "" : " ...");
}

/* flow control CMfrom CP, it use in serial devices */
int link_rx_flowctl_cmd(struct link_device *ld, const char *data, size_t len)
{
	struct mif_common *commons = &ld->mc->commons;
	unsigned short *cmd, *end = (unsigned short *)(data + len);

	mif_debug("flow control cmd: size=%d\n", len);

	for (cmd = (unsigned short *)data; cmd < end; cmd++) {
		switch (*cmd) {
		case CMD_SUSPEND:
			iodevs_for_each(commons, iodev_netif_stop, 0);
			ld->raw_tx_suspended = true;
			mif_info("flowctl CMD_SUSPEND(%04X)\n", *cmd);
			break;

		case CMD_RESUME:
			iodevs_for_each(commons, iodev_netif_wake, 0);
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

struct io_device *get_iod_with_channel(struct mif_common *commons,
					unsigned channel)
{
	struct rb_node *n = commons->iodevs_tree_chan.rb_node;
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

struct io_device *get_iod_with_format(struct mif_common *commons,
			enum dev_format format)
{
	struct rb_node *n = commons->iodevs_tree_fmt.rb_node;
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

struct io_device *insert_iod_with_channel(struct mif_common *commons,
		unsigned channel, struct io_device *iod)
{
	struct rb_node **p = &commons->iodevs_tree_chan.rb_node;
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
	rb_insert_color(&iod->node_chan, &commons->iodevs_tree_chan);
	return NULL;
}

struct io_device *insert_iod_with_format(struct mif_common *commons,
		enum dev_format format, struct io_device *iod)
{
	struct rb_node **p = &commons->iodevs_tree_fmt.rb_node;
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
	rb_insert_color(&iod->node_fmt, &commons->iodevs_tree_fmt);
	return NULL;
}

void iodevs_for_each(struct mif_common *commons, action_fn action, void *args)
{
	struct io_device *iod;
	struct rb_node *node = rb_first(&commons->iodevs_tree_chan);
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
	if (iod->io_typ == IODEV_NET && IS_CONNECTED(iod, ld)) {
		set_current_link(iod, ld);
		mif_err("%s -> %s\n", iod->name, ld->name);
	}
}

void rawdevs_set_tx_link(struct mif_common *commons, enum modem_link link_type)
{
	struct link_device *ld = find_linkdev(commons, link_type);
	if (ld)
		iodevs_for_each(commons, iodev_set_tx_link, ld);
}

void mif_add_timer(struct timer_list *timer, unsigned long expire,
		void (*function)(unsigned long), unsigned long data)
{
	init_timer(timer);
	timer->expires = get_jiffies_64() + expire;
	timer->function = function;
	timer->data = data;
	add_timer(timer);
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
		mif_err("%04X: "
			"%02x %02x %02x %02x  %02x %02x %02x %02x  "
			"%02x %02x %02x %02x  %02x %02x %02x %02x\n",
			(i << 4),
			b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
			b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
	}

	/* Print the last line */
	if (residue > 0)
		mif_err("%s\n", last);
}

void print_sipc4_hdlc_fmt_frame(const u8 *psrc)
{
	u8 *frm;			/* HDLC Frame	*/
	struct fmt_hdr *hh;		/* HDLC Header	*/
	struct sipc_fmt_hdr *fh;	/* IPC Header	*/
	u16 hh_len = sizeof(struct fmt_hdr);
	u16 fh_len = sizeof(struct sipc_fmt_hdr);
	u8 *data;
	int dlen;

	/* Actual HDLC header starts from after START flag (0x7F) */
	frm = (u8 *)(psrc + 1);

	/* Point HDLC header and IPC header */
	hh = (struct fmt_hdr *)(frm);
	fh = (struct sipc_fmt_hdr *)(frm + hh_len);

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
	struct sipc_fmt_hdr *fh = (struct sipc_fmt_hdr *)psrc;
	u16 fh_len = sizeof(struct sipc_fmt_hdr);
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

static void print_tcp_header(u8 *pkt)
{
	int i;
	char tcp_flags[32];
	struct tcphdr *tcph = (struct tcphdr *)pkt;
	u8 *opt = pkt + TCP_HDR_SIZE;
	unsigned opt_len = (tcph->doff << 2) - TCP_HDR_SIZE;

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

	memset(tcp_flags, 0, sizeof(tcp_flags));
	if (tcph->cwr)
		strcat(tcp_flags, "CWR ");
	if (tcph->ece)
		strcat(tcp_flags, "EC");
	if (tcph->urg)
		strcat(tcp_flags, "URG ");
	if (tcph->ack)
		strcat(tcp_flags, "ACK ");
	if (tcph->psh)
		strcat(tcp_flags, "PSH ");
	if (tcph->rst)
		strcat(tcp_flags, "RST ");
	if (tcph->syn)
		strcat(tcp_flags, "SYN ");
	if (tcph->fin)
		strcat(tcp_flags, "FIN ");

	mif_err("TCP:: Src.Port %u, Dst.Port %u\n",
		ntohs(tcph->source), ntohs(tcph->dest));
	mif_err("TCP:: SEQ 0x%08X(%u), ACK 0x%08X(%u)\n",
		ntohs(tcph->seq), ntohs(tcph->seq),
		ntohs(tcph->ack_seq), ntohs(tcph->ack_seq));
	mif_err("TCP:: Flags {%s}\n", tcp_flags);
	mif_err("TCP:: Window %u, Checksum 0x%04X, Urg Pointer %u\n",
		ntohs(tcph->window), ntohs(tcph->check), ntohs(tcph->urg_ptr));

	if (opt_len > 0) {
		mif_err("TCP:: Options {");
		for (i = 0; i < opt_len; i++)
			mif_err("%02X ", opt[i]);
		mif_err("}\n");
	}
}

static void print_udp_header(u8 *pkt)
{
	struct udphdr *udph = (struct udphdr *)pkt;

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

	mif_err("UDP:: Src.Port %u, Dst.Port %u\n",
		ntohs(udph->source), ntohs(udph->dest));
	mif_err("UDP:: Length %u, Checksum 0x%04X\n",
		ntohs(udph->len), ntohs(udph->check));

	if (ntohs(udph->dest) == 53)
		mif_err("UDP:: DNS query!!!\n");

	if (ntohs(udph->source) == 53)
		mif_err("UDP:: DNS response!!!\n");
}

void print_ip4_packet(u8 *ip_pkt)
{
	char ip_flags[16];
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	u8 *pkt = ip_pkt + (iph->ihl << 2);
	u16 flags = (ntohs(iph->frag_off) & 0xE000);
	u16 frag_off = (ntohs(iph->frag_off) & 0x1FFF);

	mif_err("-----------------------------------------------------------\n");

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

	memset(ip_flags, 0, sizeof(ip_flags));
	if (flags & IP_CE)
		strcat(ip_flags, "C");
	if (flags & IP_DF)
		strcat(ip_flags, "D");
	if (flags & IP_MF)
		strcat(ip_flags, "M");

	mif_err("IP4:: Version %u, Header Length %u, TOS %u, Length %u\n",
		iph->version, (iph->ihl << 2), iph->tos, ntohs(iph->tot_len));
	mif_err("IP4:: I%u, Fragment Offset %u\n",
		ntohs(iph->id), frag_off);
	mif_err("IP4:: Flags {%s}\n", ip_flags);
	mif_err("IP4:: TTL %u, Protocol %u, Header Checksum 0x%04X\n",
		iph->ttl, iph->protocol, ntohs(iph->check));
	mif_err("IP4:: Src.IP %u.%u.%u.%u, Dst.IP %u.%u.%u.%u\n",
		ip_pkt[12], ip_pkt[13], ip_pkt[14], ip_pkt[15],
		ip_pkt[16], ip_pkt[17], ip_pkt[18], ip_pkt[19]);

	switch (iph->protocol) {
	case 6:
		/* TCP */
		print_tcp_header(pkt);
		break;

	case 17:
		/* UDP */
		print_udp_header(pkt);
		break;

	default:
		break;
	}

	mif_err("-----------------------------------------------------------\n");
}

