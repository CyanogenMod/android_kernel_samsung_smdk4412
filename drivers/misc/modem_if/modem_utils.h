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

#ifndef __MODEM_UTILS_H__
#define __MODEM_UTILS_H__

#include <linux/rbtree.h>

#define IS_CONNECTED(iod, ld) ((iod)->link_types & LINKTYPE((ld)->link_type))

/** find_linkdev - find a link device
 * @commons:	struct mif_common
 */
static inline struct link_device *find_linkdev(struct mif_common *commons,
		enum modem_link link_type)
{
	struct link_device *ld;
	list_for_each_entry(ld, &commons->link_dev_list, list) {
		if (ld->link_type == link_type)
			return ld;
	}
	return NULL;
}

/** countbits - count number of 1 bits as fastest way
 * @n: number
 */
static inline unsigned int countbits(unsigned int n)
{
	unsigned int i;
	for (i = 0; n != 0; i++)
		n &= (n - 1);
	return i;
}

/* print buffer as hex string */
int pr_buffer(const char *tag, const char *data, size_t data_len,
							size_t max_len);

/* print a sk_buff as hex string */
#define pr_skb(tag, skb) \
	pr_buffer(tag, (char *)((skb)->data), (size_t)((skb)->len), (size_t)16)

/* print a urb as hex string */
#define pr_urb(tag, urb) \
	pr_buffer(tag, (char *)((urb)->transfer_buffer), \
			(size_t)((urb)->actual_length), (size_t)16)

/* flow control CMD from CP, it use in serial devices */
int link_rx_flowctl_cmd(struct link_device *ld, const char *data, size_t len);


/* get iod from tree functions */

struct io_device *get_iod_with_format(struct mif_common *commons,
					enum dev_format format);
struct io_device *get_iod_with_channel(struct mif_common *commons,
					unsigned channel);

static inline struct io_device *link_get_iod_with_format(
			struct link_device *ld, enum dev_format format)
{
	struct io_device *iod = get_iod_with_format(&ld->mc->commons, format);
	return (iod && IS_CONNECTED(iod, ld)) ? iod : NULL;
}

static inline struct io_device *link_get_iod_with_channel(
			struct link_device *ld, unsigned channel)
{
	struct io_device *iod = get_iod_with_channel(&ld->mc->commons, channel);
	return (iod && IS_CONNECTED(iod, ld)) ? iod : NULL;
}

/* insert iod to tree functions */
struct io_device *insert_iod_with_format(struct mif_common *commons,
			enum dev_format format, struct io_device *iod);
struct io_device *insert_iod_with_channel(struct mif_common *commons,
			unsigned channel, struct io_device *iod);

/* iodev for each */
typedef void (*action_fn)(struct io_device *iod, void *args);
void iodevs_for_each(struct mif_common *commons, action_fn action, void *args);

/* netif wake/stop queue of iod */
void iodev_netif_wake(struct io_device *iod, void *args);
void iodev_netif_stop(struct io_device *iod, void *args);

/* change tx_link of raw devices */
void rawdevs_set_tx_link(struct mif_common *commons, enum modem_link link_type);

void mif_add_timer(struct timer_list *timer, unsigned long expire,
		void (*function)(unsigned long), unsigned long data);

/* debug helper functions for sipc4, sipc5 */
void mif_print_data(char *buf, int len);
void print_sipc4_hdlc_fmt_frame(const u8 *psrc);
void print_sipc4_fmt_frame(const u8 *psrc);
void print_sipc5_link_fmt_frame(const u8 *psrc);


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
#define IPV4_HDR_SIZE	20

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
#define TCP_HDR_SIZE	20

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
#define UDP_HDR_SIZE	8

void print_ip4_packet(u8 *ip_pkt);

#endif/*__MODEM_UTILS_H__*/
