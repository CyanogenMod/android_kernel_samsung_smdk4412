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
 * linux_packet.c
 *
 * Linux interceptor implementation of interceptor API packet functions.
 *
 */

#include "linux_internal.h"
#include "linux_packet_internal.h"

/* Packet context freelist head pointer array. There is a freelist entry for
   each CPU and one freelist entry (with index SSH_LINUX_INTERCEPTOR_NR_CPUS)
   shared among all CPU's. When getting a packet, the current CPU freelist is
   searched, if that is empty the shared freelist is searched (which requires
   taking a lock). When returning a packet, if the CPU is the same as when
   the packet was allocated, the packet is returned to the CPU freelist, if
   not it is returned to the shared freelist (which again requires taking
   a lock). */
struct SshInterceptorInternalPacketRec
*ssh_packet_freelist_head[SSH_LINUX_INTERCEPTOR_NR_CPUS + 1] = {NULL};

extern SshInterceptor ssh_interceptor_context;

static inline SshInterceptorInternalPacket
ssh_freelist_packet_get(SshInterceptor interceptor,
			Boolean may_borrow)
{
  SshInterceptorInternalPacket p;
  unsigned int cpu;

  icept_preempt_disable();

  cpu = smp_processor_id();

  p = ssh_packet_freelist_head[cpu];
  if (p)
    {
      p->cpu = cpu;

      ssh_packet_freelist_head[p->cpu] =
	(SshInterceptorInternalPacket) p->packet.next;
    }
  else
    {
      /* Try getting a packet from the shared freelist */
      ssh_kernel_mutex_lock(interceptor->packet_lock);

      p = ssh_packet_freelist_head[SSH_LINUX_INTERCEPTOR_NR_CPUS];
      if (p)
	{
	  p->cpu = cpu;

	  ssh_packet_freelist_head[SSH_LINUX_INTERCEPTOR_NR_CPUS] =
	    (SshInterceptorInternalPacket) p->packet.next;

	  ssh_kernel_mutex_unlock(interceptor->packet_lock);
	  goto done;
	}
      ssh_kernel_mutex_unlock(interceptor->packet_lock);

      p = ssh_malloc(sizeof(*p));
      if (!p)
	goto done;

      p->cpu = cpu;
    }

 done:
  icept_preempt_enable();

  return p;
}

static inline void
ssh_freelist_packet_put(SshInterceptor interceptor,
                        SshInterceptorInternalPacket p)
{
  unsigned int cpu;

  icept_preempt_disable();

  cpu = p->cpu;

  SSH_ASSERT(cpu < SSH_LINUX_INTERCEPTOR_NR_CPUS);

#ifdef DEBUG_LIGHT
  memset(p, 'F', sizeof(*p));
#endif /* DEBUG_LIGHT */

  if (likely(cpu == smp_processor_id()))
    {
      p->packet.next =
	(SshInterceptorPacket) ssh_packet_freelist_head[cpu];
      ssh_packet_freelist_head[cpu] = p;
    }
  else
    {
      cpu = SSH_LINUX_INTERCEPTOR_NR_CPUS;

      /* The executing CPU is not the same as when the packet was
	 allocated. Put the packet back to the shared freelist */
      ssh_kernel_mutex_lock(interceptor->packet_lock);

      p->packet.next =
	(SshInterceptorPacket) ssh_packet_freelist_head[cpu];
      ssh_packet_freelist_head[cpu] = p;

      ssh_kernel_mutex_unlock(interceptor->packet_lock);
    }

  icept_preempt_enable();
}


Boolean
ssh_interceptor_packet_freelist_init(SshInterceptor interceptor)
{
  unsigned int i;

  for (i = 0; i < SSH_LINUX_INTERCEPTOR_NR_CPUS + 1; i++)
    ssh_packet_freelist_head[i] = NULL;

  return TRUE;
}

void
ssh_interceptor_packet_freelist_uninit(SshInterceptor interceptor)
{
  SshInterceptorInternalPacket p;
  unsigned int i;

  ssh_kernel_mutex_lock(interceptor->packet_lock);

  for (i = 0; i < SSH_LINUX_INTERCEPTOR_NR_CPUS + 1; i++)
    {
      /* Traverse freelist and free allocated all packets. */
      p = ssh_packet_freelist_head[i];
      while (p != NULL)
	{
	  ssh_packet_freelist_head[i] =
	    (SshInterceptorInternalPacket) p->packet.next;

	  ssh_free(p);

	  p = ssh_packet_freelist_head[i];
	}
    }

  ssh_kernel_mutex_unlock(interceptor->packet_lock);
}

/******************************************* General packet allocation stuff */

/* Allocates new packet skb with copied data from original
   + the extra free space reserved for extensions. */
struct sk_buff *
ssh_interceptor_packet_skb_dup(SshInterceptor interceptor,
			       struct sk_buff *skb,
                               size_t addbytes_active_ofs,
                               size_t addbytes_active)
{
  struct sk_buff *new_skb;
  ssize_t offset;
  size_t addbytes_spare_start = SSH_INTERCEPTOR_PACKET_HEAD_ROOM;
  size_t addbytes_spare_end = SSH_INTERCEPTOR_PACKET_TAIL_ROOM;
  unsigned char *ptr;

  SSH_DEBUG(SSH_D_LOWOK,
	    ("skb dup: len %d extra %d offset %d headroom %d tailroom %d",
	     skb->len, (int) addbytes_active, (int) addbytes_active_ofs,
	     (int) addbytes_spare_start, (int) addbytes_spare_end));

  /* Create new skb */
  new_skb = alloc_skb(skb->len + addbytes_active +
                      addbytes_spare_start +
                      addbytes_spare_end,
                      SSH_LINUX_ALLOC_SKB_GFP_MASK);
  if (!new_skb)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_failed_allocs++; });
      return NULL;
    }

  /* Set the fields */
  new_skb->len = skb->len + addbytes_active;
  new_skb->data = new_skb->head + addbytes_spare_start;
  SSH_SKB_SET_TAIL(new_skb, new_skb->data + new_skb->len);
  new_skb->protocol = skb->protocol;
  new_skb->dev = skb->dev;

  new_skb->pkt_type = skb->pkt_type;
#ifdef LINUX_HAS_SKB_STAMP
  new_skb->stamp = skb->stamp;
#endif /* LINUX_HAS_SKB_STAMP */
  new_skb->destructor = NULL_FNPTR;

  /* Set transport header offset. TX Checksum offloading relies this to be
     set in the case that the checksum has to be calculated in software
     in dev_queue_xmit(). */
  ptr = SSH_SKB_GET_TRHDR(skb);
  if (ptr != NULL)
    {
      offset = ptr - skb->data;
      if (offset > addbytes_active_ofs)
	offset += addbytes_active;
      SSH_SKB_SET_TRHDR(new_skb, new_skb->data + offset);
    }

  /* Set mac header offset. This is set for convinience. Note that if
     mac header has already been removed from the sk_buff then the mac
     header data is not copied to the duplicate. */
  ptr = SSH_SKB_GET_MACHDR(skb);
  if (ptr != NULL)
    {
      offset = ptr - skb->data;
      if (offset > addbytes_active_ofs)
	offset += addbytes_active;
      SSH_SKB_SET_MACHDR(new_skb, new_skb->data + offset);
    }

  /* Set network header offset. This one is maintained out of convenience
     (so we need not do setting by hand unless we definitely want to,
     i.e. sending packet out). */
  ptr = SSH_SKB_GET_NETHDR(skb);
  if (ptr != NULL)
    {
      offset = ptr - skb->data;
      if (offset > addbytes_active_ofs)
	offset += addbytes_active;
      SSH_SKB_SET_NETHDR(new_skb, new_skb->data + offset);
    }

  /* not used by old interceptor. */
  new_skb->sk = NULL; /* kernel does this.. copying might make more sense? */

  /* not needed according to kernel (alloc_skb does this) */
  atomic_set(&new_skb->users, 1);

  /* Set csum fields. */
  new_skb->ip_summed = skb->ip_summed;
  if (
#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
      new_skb->ip_summed == CHECKSUM_COMPLETE
#else /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
      new_skb->ip_summed == CHECKSUM_HW
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
      )
    {
      SSH_SKB_CSUM(new_skb) = SSH_SKB_CSUM(skb);
    }
#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
  else if (new_skb->ip_summed == CHECKSUM_PARTIAL)
    {
      SSH_SKB_CSUM_OFFSET(new_skb) = SSH_SKB_CSUM_OFFSET(skb);
#ifdef LINUX_HAS_SKB_CSUM_START
      /* Set csum_start. */
      offset = (skb->head + skb->csum_start) - skb->data;
      if (offset > addbytes_active_ofs)
	offset += addbytes_active;
      new_skb->csum_start = (new_skb->data + offset) - new_skb->head;
#endif /* LINUX_HAS_SKB_CSUM_START */
    }
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
  else
    {
      SSH_SKB_CSUM(new_skb) = 0;
    }

  new_skb->priority = skb->priority;
  SSH_SKB_DST_SET(new_skb, dst_clone(SSH_SKB_DST(skb)));
  memcpy(new_skb->cb, skb->cb, sizeof(skb->cb));

#ifdef LINUX_HAS_SKB_SECURITY
  new_skb->security = skb->security;
#endif /* LINUX_HAS_SKB_SECURITY */

#ifdef CONFIG_NETFILTER
  SSH_SKB_MARK(new_skb) = SSH_SKB_MARK(skb);
#ifdef LINUX_HAS_SKB_NFCACHE
  new_skb->nfcache = NFC_UNKNOWN;
#endif /* LINUX_HAS_SKB_NFCACHE */
#ifdef CONFIG_NETFILTER_DEBUG
#ifdef LINUX_HAS_SKB_NFDEBUG
  new_skb->nf_debug = skb->nf_debug;
#endif /* LINUX_HAS_SKB_NFDEBUG */
#endif /* CONFIG_NETFILTER_DEBUG */
#endif /* CONFIG_NETFILTER */

  SSH_LINUX_STATISTICS(interceptor,
  { interceptor->stats.num_copied_packets++; });

  /* Copy data from active_ofs+ => active_ofs+addbytes+ */
  if ((skb->len - addbytes_active_ofs) > 0)
    {
      memcpy(new_skb->data + addbytes_active_ofs + addbytes_active,
             skb->data + addbytes_active_ofs,
             skb->len - addbytes_active_ofs);
    }

  /* Copy the 0+ => 0+ where value < ofs (header part that is left alone). */
  if (addbytes_active_ofs > 0)
    {
      SSH_ASSERT(addbytes_active_ofs <= skb->len);
      memcpy(new_skb->data,
             skb->data,
             addbytes_active_ofs);
    }

  return new_skb;
}

/* Allocates a packet header wrapping the given skbuff.
   Packet headers can be allocated only using this function.

   Note that the actual packet->skb is NULL after packet has been
   returned.

   This function returns NULL if the packet header cannot be
   allocated. */

SshInterceptorInternalPacket
ssh_interceptor_packet_alloc_header(SshInterceptor interceptor,
                                    SshUInt32 flags,
                                    SshInterceptorProtocol protocol,
                                    SshUInt32 ifnum_in,
                                    SshUInt32 ifnum_out,
				    struct sk_buff *skb,
                                    Boolean force_copy_skbuff,
                                    Boolean free_original_on_copy,
				    Boolean packet_from_system)
{
  SshInterceptorInternalPacket p;

  /* Linearize the packet in case it isn't already. */
#ifdef LINUX_SKB_LINEARIZE_NEEDS_FLAGS
  if (skb && skb_is_nonlinear(skb) && skb_linearize(skb, GFP_ATOMIC) != 0)
    return NULL;
#else /* LINUX_SKB_LINEARIZE_NEEDS_FLAGS */
  if (skb && skb_is_nonlinear(skb) && skb_linearize(skb) != 0)
    return NULL;
#endif /* LINUX_SKB_LINEARIZE_NEEDS_FLAGS */

  /* Allocate a wrapper structure */
  p = ssh_freelist_packet_get(interceptor, !packet_from_system);
  if (p == NULL)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_failed_allocs++; });
      return NULL;
    }

  /* Initialize all the fields */
  p->packet.flags = flags;

  /* Assert that the interface number fits into SshInterceptorIfnum.
     Note that both interface numbers may be equal to
     SSH_INTERCEPTOR_INVALID_IFNUM. */
  SSH_LINUX_ASSERT_IFNUM(ifnum_in);
  SSH_LINUX_ASSERT_IFNUM(ifnum_out);

  p->packet.ifnum_in = ifnum_in;
  p->packet.ifnum_out = ifnum_out;
  p->original_ifnum = ifnum_in;

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  p->packet.route_selector = 0;
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  p->packet.pmtu = 0;
  p->packet.protocol = protocol;
#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
  memset(p->packet.extension, 0, sizeof(p->packet.extension));
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
  /* Copy the linux fwmark to the extension slot indexed by
     SSH_LINUX_FWMARK_EXTENSION_SELECTOR. */
  if (skb)
    p->packet.extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR] =
      SSH_SKB_MARK(skb);
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */
  p->interceptor = interceptor;
  p->skb = skb;

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.num_allocated_packets++;
    interceptor->stats.num_allocated_packets_total++;
  });

  if (skb)
    {
      /* we have skb */
      if (force_copy_skbuff || skb_cloned(skb))
        {
          /* The skb was already cloned, so make a new copy to be modified by
           * the engine processing. */
          p->skb = ssh_interceptor_packet_skb_dup(interceptor, skb, 0, 0);
          if (p->skb == NULL)
            {
	      SSH_LINUX_STATISTICS(interceptor,
	      { interceptor->stats.num_allocated_packets--; });
	      ssh_freelist_packet_put(interceptor, p);
              return NULL;
            }

          if (free_original_on_copy)
            {
              /* Free the original buffer as we will not return it anymore */
              dev_kfree_skb_any(skb);
            }
        }
      else
        {
          /* No one else has cloned the original skb, so use it
             without copying */
          p->skb = skb;
        }

      /* If the packet is of media-broadcast persuasion, add it to the flags. */
      if (p->skb->pkt_type == PACKET_BROADCAST)
        p->packet.flags |= SSH_PACKET_MEDIABCAST;
      if (p->skb->pkt_type == PACKET_MULTICAST)
        p->packet.flags |= SSH_PACKET_MEDIABCAST;

#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
      if (p->skb->ip_summed == CHECKSUM_COMPLETE         /* inbound */
	  || p->skb->ip_summed == CHECKSUM_UNNECESSARY   /* inbound */
	  || p->skb->ip_summed == CHECKSUM_PARTIAL)      /* outbound */
	p->packet.flags |= SSH_PACKET_HWCKSUM;
#else /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
      if (p->skb->ip_summed == CHECKSUM_HW               /* inbound/outbound */
	  || p->skb->ip_summed == CHECKSUM_UNNECESSARY)  /* inbound */
	p->packet.flags |= SSH_PACKET_HWCKSUM;
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */

      SSH_DEBUG(SSH_D_LOWOK,
		("alloc header: skb len %d headroom %d tailroom %d",
		 p->skb->len, skb_headroom(p->skb), skb_tailroom(p->skb)));
    }

  return p;
}

/* Allocates a packet of at least the given size.  Packets can only be
   allocated using this function (either internally by the interceptor or
   by other code by calling this function).  This returns NULL if no more
   packets can be allocated. */

SshInterceptorPacket
ssh_interceptor_packet_alloc(SshInterceptor interceptor,
                             SshUInt32 flags,
                             SshInterceptorProtocol protocol,
                             SshInterceptorIfnum ifnum_in,
                             SshInterceptorIfnum ifnum_out,
			     size_t total_len)
{
  SshInterceptorInternalPacket packet;
  size_t len;
  struct sk_buff *skb;

  packet = (SshInterceptorInternalPacket)
    ssh_interceptor_packet_alloc_header(interceptor,
                                        flags,
                                        protocol,
					ifnum_in,
					ifnum_out,
					NULL,
					FALSE,
                                        FALSE,
					FALSE);
  if (!packet)
    return NULL;                /* header allocation failed */

  /* Allocate actual kernel packet. Note that some overhead is calculated
     so that media headers etc. fit without additional allocations or
     copying. */
  len = total_len + SSH_INTERCEPTOR_PACKET_HEAD_ROOM +
    SSH_INTERCEPTOR_PACKET_TAIL_ROOM;
  skb = alloc_skb(len, SSH_LINUX_ALLOC_SKB_GFP_MASK);
  if (skb == NULL)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_failed_allocs++; });
      ssh_freelist_packet_put(interceptor, packet);
      return NULL;
    }
  packet->skb = skb;

  /* Set data area inside the packet */
  skb->len = total_len;
  skb->data = skb->head + SSH_INTERCEPTOR_PACKET_HEAD_ROOM;
  SSH_SKB_SET_TAIL(skb, skb->data + total_len);

  /* Ensure the IP header offset is 16 byte aligned for ethernet frames. */
  if (protocol == SSH_PROTOCOL_ETHERNET)
    {
      /* Assert that SSH_INTERCEPTOR_PACKET_TAIL_ROOM is large enough */
      SSH_ASSERT(SSH_SKB_GET_END(skb) - SSH_SKB_GET_TAIL(skb) >= 2);
      skb->data += 2;
      /* This works on both pointers and offsets. */
      skb->tail += 2;
    }

#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
  if (flags & SSH_PACKET_HWCKSUM)
    {
      if (flags & SSH_PACKET_FROMADAPTER)
	skb->ip_summed = CHECKSUM_COMPLETE;
      else if (flags & SSH_PACKET_FROMPROTOCOL)
	skb->ip_summed = CHECKSUM_PARTIAL;
    }
#else /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
  if (flags & SSH_PACKET_HWCKSUM)
    skb->ip_summed = CHECKSUM_HW;
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */

  /* If support for other than IPv6, IPv4 and ARP
     inside the engine on Linux are to be supported, their
     protocol types must be added here. */
  switch(protocol)
    {
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case SSH_PROTOCOL_IP6:
      skb->protocol = __constant_htons(ETH_P_IPV6);
      break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    case SSH_PROTOCOL_ARP:
      skb->protocol = __constant_htons(ETH_P_ARP);
      break;

    case SSH_PROTOCOL_IP4:
    default:
      skb->protocol = __constant_htons(ETH_P_IP);
      break;
    }

  SSH_DEBUG(SSH_D_LOWOK,
	    ("alloc packet: skb len %d headroom %d tailroom %d",
	     packet->skb->len, skb_headroom(packet->skb),
	     skb_tailroom(packet->skb)));

  return (SshInterceptorPacket) packet;
}

/* Frees the given packet. All packets allocated by
   ssh_interceptor_packet_alloc must eventually be freed using this
   function by either calling this explicitly or by passing the packet
   to the interceptor send function. */

void
ssh_interceptor_packet_free(SshInterceptorPacket pp)
{
  SshInterceptorInternalPacket packet = (SshInterceptorInternalPacket) pp;

  /* Free the packet buffer first */
  if (packet->skb)
    {
      dev_kfree_skb_any(packet->skb);
      packet->skb = NULL;
    }

  SSH_LINUX_STATISTICS(packet->interceptor,
  { packet->interceptor->stats.num_allocated_packets--; });

  /* Free the wrapper */
  ssh_freelist_packet_put(packet->interceptor, packet);
}

#if defined(KERNEL_INTERCEPTOR_USE_FUNCTIONS) || !defined(KERNEL)
/* Returns the length of the data packet. */
size_t
ssh_interceptor_packet_len(SshInterceptorPacket pp)
{
  SshInterceptorInternalPacket packet = (SshInterceptorInternalPacket) pp;
  return packet->skb->len;
}
#endif /* defined(KERNEL_INTERCEPTOR_USE_FUNCTIONS) || !defined(KERNEL) */


/* Copies data into the packet.  Space for the new data must already have
   been allocated.  It is a fatal error to attempt to copy beyond the
   allocated packet.  Multiple threads may call this function concurrently,
   but not for the same packet.  This does not change the length of the
   packet. */

Boolean
ssh_interceptor_packet_copyin(SshInterceptorPacket pp, size_t offset,
                              const unsigned char *buf, size_t len)
{
  SshInterceptorInternalPacket packet = (SshInterceptorInternalPacket) pp;

  memmove(packet->skb->data + offset, buf, len);

  return TRUE;
}

/* Copies data out from the packet.  Space for the new data must
   already have been allocated.  It is a fatal error to attempt to
   copy beyond the allocated packet. Multiple threads may call this
   function concurrently, but not for the same packet. */

void
ssh_interceptor_packet_copyout(SshInterceptorPacket pp, size_t offset,
                               unsigned char *buf, size_t len)
{
  SshInterceptorInternalPacket packet = (SshInterceptorInternalPacket) pp;

  memmove(buf, packet->skb->data + offset, len);
}

#define SSH_PACKET_IDATA_IFNUM_OFFSET          0
#define SSH_PACKET_IDATA_DST_CACHE_ID_OFFSET   sizeof(SshUInt32)
#define SSH_PACKET_IDATA_PKT_TYPE_OFFSET       (2 * sizeof(SshUInt32))
#define SSH_PACKET_IDATA_TRHDR_OFFSET          (3 * sizeof(SshUInt32))
#define SSH_PACKET_IDATA_CP_OFFSET             (4 * sizeof(SshUInt32))
#define SSH_PACKET_IDATA_MINLEN                SSH_PACKET_IDATA_CP_OFFSET

Boolean ssh_interceptor_packet_export_internal_data(SshInterceptorPacket pp,
                                                    unsigned char **data_ret,
                                                    size_t *len_return)
{
  unsigned char *data;
  SshInterceptorInternalPacket ipp = (SshInterceptorInternalPacket)pp;
  SshUInt32 dst_cache_id = 0;
  SshUInt32 transport_offset = 0;

  if (ipp->skb == NULL)
    {
      SSH_DEBUG(SSH_D_FAIL,
		("Unable to export internal packet data, sk buff"));
      *data_ret = NULL;
      *len_return = 0;
      return FALSE;
    }

  data = ssh_calloc(1, SSH_PACKET_IDATA_MINLEN + sizeof(ipp->skb->cb));
  if (data == NULL)
    {
      SSH_DEBUG(SSH_D_FAIL, ("Unable to export internal packet data"));
      *data_ret = NULL;
      *len_return = 0;
      return FALSE;
    }

  SSH_PUT_32BIT(data, SSH_PACKET_IDATA_IFNUM_OFFSET);

  dst_cache_id =
    ssh_interceptor_packet_cache_dst_entry(ssh_interceptor_context, pp);
  SSH_PUT_32BIT(data + SSH_PACKET_IDATA_DST_CACHE_ID_OFFSET, dst_cache_id);

  SSH_PUT_8BIT(data + SSH_PACKET_IDATA_PKT_TYPE_OFFSET, ipp->skb->pkt_type);

  transport_offset = (SshUInt32)(SSH_SKB_GET_TRHDR(ipp->skb) - ipp->skb->data);

  SSH_PUT_32BIT(data + SSH_PACKET_IDATA_TRHDR_OFFSET, transport_offset);
  memcpy(data + SSH_PACKET_IDATA_CP_OFFSET, ipp->skb->cb,
	 sizeof(ipp->skb->cb));

  *data_ret = data;
  *len_return = SSH_PACKET_IDATA_MINLEN + sizeof(ipp->skb->cb);

  return TRUE;
}

void
ssh_interceptor_packet_discard_internal_data(unsigned char *data,
					     size_t data_len)
{
  SshUInt32 dst_cache_id;

  if (data_len == 0)
    return;

  if (data == NULL || data_len < SSH_PACKET_IDATA_MINLEN)
    {
      /* Attempt to import corrupted data. */
      SSH_DEBUG(SSH_D_FAIL, ("Unable to import internal packet data"));
      return;
    }

  dst_cache_id = SSH_GET_32BIT(data + SSH_PACKET_IDATA_DST_CACHE_ID_OFFSET);

  ssh_interceptor_packet_return_dst_entry(ssh_interceptor_context,
					  dst_cache_id, NULL, TRUE);
}

Boolean ssh_interceptor_packet_import_internal_data(SshInterceptorPacket pp,
                                                    const unsigned char *data,
                                                    size_t len)
{
  SshInterceptorInternalPacket ipp = (SshInterceptorInternalPacket)pp;
  SshUInt32 orig_ifnum;
  SshUInt32 dst_cache_id;
  SshUInt32 transport_offset;
  Boolean remove_only = FALSE;

  if (ipp->skb == NULL)
    {
      SSH_DEBUG(SSH_D_FAIL,
		("Unable to import internal packet data, no skb"));
      return FALSE;
    }

  if (len == 0)
    {
      /* No data to import, i.e. packet created by engine. */
      ipp->original_ifnum = SSH_INTERCEPTOR_INVALID_IFNUM;
      ipp->skb->pkt_type = PACKET_HOST;
      return TRUE;
    }

  if (data == NULL || len < (SSH_PACKET_IDATA_MINLEN + sizeof(ipp->skb->cb)))
    {
      /* Attempt to import corrupted data. */
      SSH_DEBUG(SSH_D_FAIL, ("Unable to import internal packet data"));
      return FALSE;
    }

  orig_ifnum = SSH_GET_32BIT(data + SSH_PACKET_IDATA_IFNUM_OFFSET);
  ipp->original_ifnum = orig_ifnum;

  dst_cache_id = SSH_GET_32BIT(data + SSH_PACKET_IDATA_DST_CACHE_ID_OFFSET);

  if (pp->flags & SSH_PACKET_UNMODIFIED)
    remove_only = FALSE;
  ssh_interceptor_packet_return_dst_entry(ssh_interceptor_context,
					  dst_cache_id, pp, remove_only);

  ipp->skb->pkt_type = SSH_GET_8BIT(data + SSH_PACKET_IDATA_PKT_TYPE_OFFSET);
  transport_offset = SSH_GET_32BIT(data + SSH_PACKET_IDATA_TRHDR_OFFSET);

  SSH_SKB_SET_TRHDR(ipp->skb, ipp->skb->data + transport_offset);

  memcpy(ipp->skb->cb, data + SSH_PACKET_IDATA_CP_OFFSET,
	 sizeof(ipp->skb->cb));

  return TRUE;
}

Boolean
ssh_interceptor_packet_align(SshInterceptorPacket pp, size_t offset)
{
  SshInterceptorInternalPacket packet = (SshInterceptorInternalPacket) pp;
  struct sk_buff *skb, *new_skb;
  unsigned long addr;
  size_t word_size, bytes;

  word_size = sizeof(int *);
  SSH_ASSERT(word_size < SSH_INTERCEPTOR_PACKET_TAIL_ROOM);

  skb = packet->skb;

  addr = (unsigned long) (skb->data + offset);

  bytes = (size_t)((((addr + word_size - 1) / word_size) * word_size) - addr);
  if (bytes == 0)
    return TRUE;

  if (SSH_SKB_GET_END(skb) - SSH_SKB_GET_TAIL(skb) >= bytes)
    {
      memmove(skb->data + bytes, skb->data, skb->len);
      skb->data += bytes;
      /* This works for both pointers and offsets. */
      skb->tail += bytes;

      SSH_DEBUG(SSH_D_LOWOK,
		("Aligning skb->data %p at offset %d to word "
		 "boundary (word_size %d), headroom %d tailroom %d",
		 skb->data, (int) offset, (int) word_size,
		 skb_headroom(skb), skb_tailroom(skb)));

      return TRUE;
    }
  else if (skb->data - skb->head >= word_size - bytes)
    {
      bytes = word_size - bytes;
      memmove(skb->data - bytes, skb->data, skb->len);
      skb->data -= bytes;
      skb->tail -= bytes;
      return TRUE;
    }
  else
    {
      /* Allocate a new packet which has enough head/tail room to allow
	 alignment. */
      new_skb = ssh_interceptor_packet_skb_dup(packet->interceptor, skb, 0, 0);

      if (!new_skb)
	{
	  ssh_interceptor_packet_free(pp);
	  return FALSE;
	}
      SSH_ASSERT(SSH_SKB_GET_END(new_skb) - SSH_SKB_GET_TAIL(new_skb)
		 >= word_size);

      /* Free the old packet */
      packet->skb = new_skb;
      dev_kfree_skb_any(skb);
      return ssh_interceptor_packet_align(pp, offset);
    }
}

struct sk_buff *
ssh_interceptor_packet_verify_headroom(struct sk_buff *skbp,
				       size_t media_header_len)
{
  SshUInt32 required_headroom;
  struct sk_buff *skbp2;

  SSH_ASSERT(skbp != NULL);
  SSH_ASSERT(skbp->dev != NULL);

  required_headroom = LL_RESERVED_SPACE(skbp->dev);
#if (SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM > 0)
  if (required_headroom < SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM)
    required_headroom = SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM;
#endif /* (SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM > 0) */

  if (unlikely(required_headroom > media_header_len &&
	       skb_headroom(skbp) < (required_headroom - media_header_len)))
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("skb does not have enough headroom for device %d, "
		 "reallocating skb headroom",
		 skbp->dev->ifindex));
      skbp2 = skb_realloc_headroom(skbp,
				   (required_headroom - media_header_len));
      dev_kfree_skb_any(skbp);

      return skbp2;
    }

  return skbp;
}
