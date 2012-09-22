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
#include <linux/platform_data/modem_na.h>
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

static char ip_flags[16];
static char tcp_flags[32];


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
	return printk(KERN_DEBUG "[MIF] %s(%u): %s%s\n", tag, data_len, hexstr,
			len == data_len ? "" : " ...");
}

/* flow control CMD from CP, it use in serial devices */
int link_rx_flowctl_cmd(struct link_device *ld, const char *data, size_t len)
{
	unsigned short *cmd, *end = (unsigned short *)(data + len);
	struct io_device *iod = NULL, *multi_raw_iod;
	int i;

	pr_debug("[MODEM_IF] flow control cmd: size=%d\n", len);

	multi_raw_iod = find_iodev(ld, IPC_MULTI_RAW);
	if (!multi_raw_iod || !multi_raw_iod->private_data) {
		pr_err("[MODEM_IF] %s: no multi raw device\n", __func__);
		return -ENODEV;
	}

	for (cmd = (unsigned short *)data; cmd < end; cmd++) {
		switch (*cmd) {
		case CMD_SUSPEND:
			raw_devs_for_each(multi_raw_iod, i, iod) {
				if (iod->io_typ == IODEV_NET && iod->ndev)
					netif_stop_queue(iod->ndev);
			}
			ld->raw_tx_suspended = true;
			pr_info("[MODEM_IF] flowctl CMD_SUSPEND(%04X)\n", *cmd);
			break;

		case CMD_RESUME:
			raw_devs_for_each(multi_raw_iod, i, iod) {
				if (iod->io_typ == IODEV_NET && iod->ndev)
					netif_wake_queue(iod->ndev);
			}
			ld->raw_tx_suspended = false;
			complete_all(&ld->raw_tx_resumed_by_cp);
			pr_info("[MODEM_IF] flowctl CMD_RESUME(%04X)\n", *cmd);
			break;

		default:
			pr_err("[MODEM_IF] flowctl BAD CMD: %04X\n", *cmd);
			break;
		}
	}

	return 0;
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
		printk(KERN_ERR "%04X: "
			"%02x %02x %02x %02x  %02x %02x %02x %02x  "
			"%02x %02x %02x %02x  %02x %02x %02x %02x\n",
			(i << 4),
			b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
			b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
	}

	/* Print the last line */
	if (residue > 0)
		printk(KERN_ERR "%s\n", last);
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

	pr_err("--------------------HDLC & FMT HEADER----------------------\n");

	pr_err("HDLC len = %d, HDLC control = 0x%02x\n", hh->len, hh->control);

	pr_err("(M)0x%02X, (S)0x%02X, (T)0x%02X, mseq:%d, aseq:%d, len:%d\n",
		fh->main_cmd, fh->sub_cmd, fh->cmd_type,
		fh->msg_seq, fh->ack_seq, fh->len);

	pr_err("-----------------------IPC FMT DATA------------------------\n");

	if (dlen > 0) {
		if (dlen > 64)
			dlen = 64;
		mif_print_data(data, dlen);
	}

	pr_err("-----------------------------------------------------------\n");
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

	pr_err("----------------------IPC FMT HEADER-----------------------\n");

	pr_err("(M)0x%02X, (S)0x%02X, (T)0x%02X, mseq:%d, aseq:%d, len:%d\n",
		fh->main_cmd, fh->sub_cmd, fh->cmd_type,
		fh->msg_seq, fh->ack_seq, fh->len);

	pr_err("-----------------------IPC FMT DATA------------------------\n");

	if (dlen > 0)
		mif_print_data(data, dlen);

	pr_err("-----------------------------------------------------------\n");
}

static void print_tcp_header(u8 *pkt)
{
	int i;
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
		strcat(tcp_flags, "ECE ");
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

	pr_err("TCP:: Src Port = %u, Dst Port = %u\n",
		ntohs(tcph->source), ntohs(tcph->dest));
	pr_err("TCP:: SEQ = 0x%08X(%u), ACK = 0x%08X(%u)\n",
		ntohs(tcph->seq), ntohs(tcph->seq),
		ntohs(tcph->ack_seq), ntohs(tcph->ack_seq));
	pr_err("TCP:: Flags = %s\n", tcp_flags);
	pr_err("TCP:: Window = %u, Checksum = 0x%04X, Urg Pointer = %u\n",
		ntohs(tcph->window), ntohs(tcph->check), ntohs(tcph->urg_ptr));

	if (opt_len > 0) {
		printk(KERN_ERR "TCP:: Options = ");
		for (i = 0; i < opt_len; i++)
			printk("%02X ", opt[i]);
		printk("\n");
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

	pr_err("UDP:: Src Port = %u, Dst Prt = %u\n",
		ntohs(udph->source), ntohs(udph->dest));
	pr_err("UDP:: Length = %u, Checksum = 0x%04X\n",
		ntohs(udph->len), ntohs(udph->check));

	if (ntohs(udph->dest) == 53)
		pr_err("UDP:: DNS query!!!\n");

	if (ntohs(udph->source) == 53)
		pr_err("UDP:: DNS response!!!\n");
}

void print_ip4_packet(u8 *ip_pkt)
{
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	u8 *pkt = ip_pkt + (iph->ihl << 2);
	u16 flags = (ntohs(iph->frag_off) & 0xE000);
	u16 frag_off = (ntohs(iph->frag_off) & 0x1FFF);

	pr_err("-----------------------------------------------------------\n");

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
		strcat(ip_flags, "CE ");
	if (flags & IP_DF)
		strcat(ip_flags, "DF ");
	if (flags & IP_MF)
		strcat(ip_flags, "MF ");

	pr_err("IP4:: Version = %u, Header Length = %u, TOS = %u, Length = %u\n",
		iph->version, (iph->ihl << 2), iph->tos, ntohs(iph->tot_len));
	pr_err("IP4:: ID = %u, Fragment Offset = %u\n",
		ntohs(iph->id), frag_off);
	pr_err("IP4:: Flags = %s\n", ip_flags);
	pr_err("IP4:: TTL = %u, Protocol = %u, Header Checksum = 0x%04X\n",
		iph->ttl, iph->protocol, ntohs(iph->check));
	pr_err("IP4:: Src IP Addr = %u.%u.%u.%u, Dst IP Addr = %u.%u.%u.%u\n",
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

	pr_err("-----------------------------------------------------------\n");
}
