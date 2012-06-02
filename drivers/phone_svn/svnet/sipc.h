/**
 * SAMSUNG MODEM IPC header
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __SAMSUNG_IPC_H__
#define __SAMSUNG_IPC_H__

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

extern const char *sipc_version;

/*
#if 1
#  include "sipc4.h"
#else
#  error "Unknown version"
#endif
*/

#define SIPC_RESET_MB 0xFFFFFF7E /* -2 & ~(INT_VALID) */
#define SIPC_EXIT_MB 0xFFFFFF7F /* -1 & ~(INT_VALID) */

struct sipc;

extern struct sipc *sipc_open(void (*queue)(u32 mailbox, void *data),
		struct net_device *ndev);
extern void sipc_close(struct sipc **);

extern void sipc_exit(void);

extern int sipc_write(struct sipc *, struct sk_buff_head *);
extern int sipc_read(struct sipc *, u32 mailbox, int *cond);
extern int sipc_rx(struct sipc *);


/* TODO: use PN_CMD ?? */
extern int sipc_check_skb(struct sipc *, struct sk_buff *skb);
extern int sipc_do_cmd(struct sipc *, struct sk_buff *skb);

extern ssize_t sipc_debug_show(struct sipc *, char *);
extern int sipc_debug(struct sipc *, const char *);
extern int sipc_whitelist(struct sipc *si, const char *buf, size_t count);

extern void sipc_ramdump(struct sipc *);

#endif /* __SAMSUNG_IPC_H__ */
