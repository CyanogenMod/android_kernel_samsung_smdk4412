/* Netfilter Driver for IPSec VPN Client
 *
 * Copyright(c)   2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * linux_packet_internal.h
 *
 * Linux interceptor specific defines for the packet portion of the
 * Interceptor API.
 *
 */

#ifndef LINUX_PACKET_INTERNAL_H
#define LINUX_PACKET_INTERNAL_H

#include "kernel_includes.h"

/* Internal packet structure, used to encapsulate the kernel structure
   for the generic packet processing engine. */

typedef struct SshInterceptorInternalPacketRec
{
  /* Generic packet structure */
  struct SshInterceptorPacketRec packet;

  /* Backpointer to interceptor */
  SshInterceptor interceptor;

  /* Kernel skb structure. */
  struct sk_buff *skb;

  /* The processor from which this packet was allocated from the freelist */
  unsigned int cpu;

  size_t iteration_offset;
  size_t iteration_bytes;

  /* These are SshUInt32's for export/import */
  SshUInt32 original_ifnum;

  SshUInt16 borrowed : 1; /* From spare resource, free after use */
} *SshInterceptorInternalPacket;


/* Typical needed tailroom: ESP trailer (worstcase ~27B).
   Typical needed headroom: media, IPIP, ESP (~60B for IPv4, ~80B for IPv6)
   Worstcase headroom:      media, UDP(8), NAT-T(12), IPIP(~20), ESP(22)  */

/* The amount of headroom reserved for network interface processing. The
   interceptor ensures that all packets passed to NIC driver will have atleast
   this much headroom. */
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
/* With media level interceptor the SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM
   includes the media header length. Let us use up the full skb if necessary.
   This is important for reducing overhead in the forwarding case. */
#define SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM 0
#else
/* Ensure that packet has always enough headroom for an aligned
   ethernet header. */
#define SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM 16
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

/* Amount of head- and tailroom to reserve when allocating or duplicating
   a packet. These values are optimised for IPsec processing. */
#define SSH_INTERCEPTOR_PACKET_HEAD_ROOM      (80)
#define SSH_INTERCEPTOR_PACKET_TAIL_ROOM      (30)

#define SSH_LINUX_ALLOC_SKB_GFP_MASK (GFP_ATOMIC)

#ifdef LINUX_HAS_SKB_CLONE_WRITABLE
#define SSH_SKB_WRITABLE(_skb, _len)				\
  (!skb_cloned((_skb)) || skb_clone_writable((_skb), (_len)))
#else /* LINUX_HAS_SKB_CLONE_WRITABLE */
#define SSH_SKB_WRITABLE(_skb, _len)		\
  (!skb_cloned((_skb)))
#endif /* LINUX_HAS_SKB_CLONE_WRITABLE */

#endif /* LINUX_PACKET_INTERNAL_H */
