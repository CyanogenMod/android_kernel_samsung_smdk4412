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
 * linux_ip_glue.c
 *
 * Linux interceptor packet interception using Netfilter hooks.
 *
 */

#include "linux_internal.h"

extern SshInterceptor ssh_interceptor_context;

/********************* Prototypes for packet handling hooks *****************/

static unsigned int
ssh_interceptor_packet_in_ipv4(unsigned int hooknum,
			       SshHookSkb *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn) (struct sk_buff *));

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static unsigned int
ssh_interceptor_packet_in_ipv6(unsigned int hooknum,
			       SshHookSkb *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn) (struct sk_buff *));
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
static unsigned int
ssh_interceptor_packet_in_arp(unsigned int hooknum,
			      SshHookSkb *skb,
			      const struct net_device *in,
			      const struct net_device *out,
			      int (*okfn) (struct sk_buff *));
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

static unsigned int
ssh_interceptor_packet_out(int pf,
                           unsigned int hooknum,
                           SshHookSkb *skb,
                           const struct net_device *in,
                           const struct net_device *out,
                           int (*okfn) (struct sk_buff *));

static unsigned int
ssh_interceptor_packet_out_ipv4(unsigned int hooknum,
                           SshHookSkb *skb,
                           const struct net_device *in,
                           const struct net_device *out,
                           int (*okfn) (struct sk_buff *));

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static unsigned int
ssh_interceptor_packet_out_ipv6(unsigned int hooknum,
                                SshHookSkb *skb,
                                const struct net_device *in,
                                const struct net_device *out,
                                int (*okfn) (struct sk_buff *));
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */


/********** Definition of netfilter hooks to register **********************/

struct SshLinuxHooksRec
{
  const char *name;        /* Name of hook */
  Boolean is_registered;   /* Has this hook been registered? */
  Boolean is_mandatory;    /* If the registration fails,
			      abort initialization? */
  int pf;                  /* Protocol family */
  int hooknum;             /* Hook id */
  int priority;            /* Netfilter priority of hook */

  /* Actual hook function */
  unsigned int (*hookfn)(unsigned int hooknum,
                         SshHookSkb *skb,
                         const struct net_device *in,
                         const struct net_device *out,
                         int (*okfn)(struct sk_buff *));

  struct nf_hook_ops *ops; /* Pointer to storage for nf_hook_ops
			      to store the netfilter hook configuration
			      and state */
};

struct nf_hook_ops ssh_nf_in4, ssh_nf_out4;

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
struct nf_hook_ops ssh_nf_in_arp;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
struct nf_hook_ops ssh_nf_in6, ssh_nf_out6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

static struct SshLinuxHooksRec ssh_nf_hooks[] =
{
  { "ipv4 in",
    FALSE, TRUE, PF_INET, SSH_NF_IP_PRE_ROUTING,  SSH_NF_IP_PRI_FIRST,
    ssh_interceptor_packet_in_ipv4,  &ssh_nf_in4 },
  { "ipv4 out",
    FALSE, TRUE, PF_INET, SSH_NF_IP_POST_ROUTING, SSH_NF_IP_PRI_FIRST,
    ssh_interceptor_packet_out_ipv4, &ssh_nf_out4 },

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  { "arp in",
    FALSE, TRUE, SSH_NFPROTO_ARP, NF_ARP_IN, 1,
    ssh_interceptor_packet_in_arp, &ssh_nf_in_arp },
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  { "ipv6 in",
    FALSE, TRUE, PF_INET6, SSH_NF_IP6_PRE_ROUTING, SSH_NF_IP6_PRI_FIRST,
    ssh_interceptor_packet_in_ipv6, &ssh_nf_in6 },
  { "ipv6 out",
    FALSE, TRUE, PF_INET6, SSH_NF_IP6_POST_ROUTING, SSH_NF_IP6_PRI_FIRST,
    ssh_interceptor_packet_out_ipv6, &ssh_nf_out6 },
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  { NULL,
    0, 0, 0, 0, 0,
    NULL_FNPTR, NULL },
};


/******************************** Module parameters *************************/

/* Module parameters. Default values. These can be overrided at the
   loading of the module from the command line. These set the priority
   for netfilter hooks. */

static int in_priority = SSH_NF_IP_PRI_FIRST;
static int out_priority = SSH_NF_IP_PRI_FIRST;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static int in6_priority = SSH_NF_IP6_PRI_FIRST;
static int out6_priority = SSH_NF_IP6_PRI_FIRST;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

MODULE_PARM_DESC(in_priority, "Netfilter hook priority at IPv4 PREROUTING");
MODULE_PARM_DESC(out_priority, "Netfilter hook priority at IPv4 POSTROUTING");
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
MODULE_PARM_DESC(in6_priority, "Netfilter hook priority at IPv6 PREROUTING");
MODULE_PARM_DESC(out6_priority, "Netfilter hook priority at IPv6 POSTROUTING");
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

module_param(in_priority, int, 0444);
module_param(out_priority, int, 0444);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
module_param(in6_priority, int, 0444);
module_param(out6_priority, int, 0444);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */


/************* Utility functions ********************************************/

/* Map a SshInterceptorProtocol to a skbuff protocol id */
static unsigned short
ssh_proto_to_skb_proto(SshInterceptorProtocol protocol)
{
  /* If support for other than IPv6, IPv4 and ARP
     inside the engine on Linux are to be supported, their
     protocol types must be added here. */
  switch (protocol)
    {
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case SSH_PROTOCOL_IP6:
      return __constant_htons(ETH_P_IPV6);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    case SSH_PROTOCOL_ARP:
      return __constant_htons(ETH_P_ARP);

    case SSH_PROTOCOL_IP4:
      return __constant_htons(ETH_P_IP);

    default:
      SSH_DEBUG(SSH_D_ERROR, ("Unknown protocol %d", protocol));
      return 0;
    }
}

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR

/* Map ethernet type to a skbuff protocol id */
static unsigned short
ssh_ethertype_to_skb_proto(SshInterceptorProtocol protocol,
			   size_t media_header_len,
			   unsigned char *media_header)
{
  SshUInt16 ethertype;

  if (protocol != SSH_PROTOCOL_ETHERNET)
    return ssh_proto_to_skb_proto(protocol);

  SSH_ASSERT(media_header_len >= SSH_ETHERH_HDRLEN);
  ethertype = SSH_GET_16BIT(media_header + SSH_ETHERH_OFS_TYPE);

  /* If support for other than IPv6, IPv4 and ARP
     inside the engine on Linux are to be supported, their
     ethernet types must be added here. */
  switch (ethertype)
    {
    case SSH_ETHERTYPE_IPv6:
      return __constant_htons(ETH_P_IPV6);

    case SSH_ETHERTYPE_ARP:
      return __constant_htons(ETH_P_ARP);

    case SSH_ETHERTYPE_IP:
      return __constant_htons(ETH_P_IP);

    default:
      SSH_DEBUG(SSH_D_ERROR, ("Unknown ethertype 0x%x", ethertype));
      return 0;
    }
}

/* Return the pointer to start of ethernet header */
static struct ethhdr *ssh_get_eth_hdr(const struct sk_buff *skb)
{
#ifdef LINUX_HAS_ETH_HDR
  return eth_hdr(skb);
#else /* LINUX_HAS_ETH_HDR */
  return skb->mac.ethernet;
#endif /* LINUX_HAS_ETH_HDR */
}

#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */


/**************** Packet reception and sending *****************************/

/**************** Inbound packet interception ******************************/


/* Common code for ssh_interceptor_packet_in_finish_ipv4()
   and ssh_interceptor_packet_in_finish_ipv6().

   If SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE is set, then
   this function is called as the okfn() from the netfilter
   infrastructure after inbound netfilter hook iteration.
   Otherwise this function is called directly from the inbound
   netfilter hookfn().
*/

static inline int
ssh_interceptor_packet_in_finish(struct sk_buff *skbp,
				 SshInterceptorProtocol protocol)
{
  SshInterceptorInternalPacket packet;
  SshInterceptor interceptor;
  int ifnum_in;

  interceptor = ssh_interceptor_context;

  SSH_ASSERT(skbp->dev != NULL);
  ifnum_in = skbp->dev->ifindex;

  SSH_DEBUG(SSH_D_HIGHSTART,
            ("incoming packet 0x%p, len %d proto 0x%x [%s] iface %d [%s]",
             skbp, skbp->len, ntohs(skbp->protocol),
             (protocol == SSH_PROTOCOL_IP4 ? "ipv4" :
	      (protocol == SSH_PROTOCOL_IP6 ? "ipv6" :
	       (protocol == SSH_PROTOCOL_ETHERNET ? "ethernet" : "unknown"))),
             ifnum_in, skbp->dev->name));

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  if (unlikely(protocol == SSH_PROTOCOL_ETHERNET))
    {
      /* Unwrap ethernet header. This is needed by the engine currently
         due to some sillyness in the ARP handling. */
      if (SSH_SKB_GET_MACHDR(skbp))
	{
	  size_t media_header_len = skbp->data - SSH_SKB_GET_MACHDR(skbp);
	  if (media_header_len)
	    skb_push(skbp, media_header_len);
	}
    }
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /* Encapsulate the skb into a new packet. Note that ip_rcv()
     performs IP header checksum computation, so we do not
     need to. */
  packet = ssh_interceptor_packet_alloc_header(interceptor,
                                               SSH_PACKET_FROMADAPTER
                                               |SSH_PACKET_IP4HDRCKSUMOK,
                                               protocol,
                                               ifnum_in,
					       SSH_INTERCEPTOR_INVALID_IFNUM,
                                               skbp,
                                               FALSE, TRUE, TRUE);

  if (unlikely(packet == NULL))
    {
      SSH_DEBUG(SSH_D_FAIL, ("encapsulation failed, packet dropped"));
      /* Free sk_buff and return error */
      dev_kfree_skb_any(skbp);
      SSH_LINUX_STATISTICS(interceptor, { interceptor->stats.num_errors++; });
      return -EPERM;
    }

#ifdef DEBUG_LIGHT
  packet->skb->dev = NULL;
#endif /* DEBUG_LIGHT */

  SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP,
                    ("incoming packet, len=%d flags=0x%08lx%s",
                     packet->skb->len,
		     (unsigned long) packet->packet.flags,
		     ((packet->packet.flags & SSH_PACKET_HWCKSUM) ?
		      " [hwcsum]" : "")),
                    packet->skb->data, packet->skb->len);

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.num_fastpath_bytes_in += (SshUInt64) packet->skb->len;
    interceptor->stats.num_fastpath_packets_in++;
  });

  /* Pass the packet to then engine. Which eventually will call
     ssh_interceptor_send. */
  SSH_LINUX_INTERCEPTOR_PACKET_CALLBACK(interceptor,
					(SshInterceptorPacket) packet);

  /* Return ok */
  return 0;
}


#ifdef SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE

static inline int
ssh_interceptor_packet_in_finish_ipv4(struct sk_buff *skbp)
{
  return ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_IP4);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static inline int
ssh_interceptor_packet_in_finish_ipv6(struct sk_buff *skbp)
{
  return ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_IP6);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#endif /* SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE */


/* ssh_interceptor_packet_in() is the common code for
   inbound netfilter hooks ssh_interceptor_packet_in_ipv4(),
   ssh_interceptor_packet_in_ipv6(), and ssh_interceptor_packet_in_arp().

   This function must only be called from softirq context, or
   with softirqs disabled. This function MUST NOT be called
   from a hardirq (as then it could pre-empt itself on the same CPU). */

static inline unsigned int
ssh_interceptor_packet_in(int pf,
			  unsigned int hooknum,
			  SshHookSkb *skb,
                          const struct net_device *in,
                          const struct net_device *out,
                          int (*okfn) (struct sk_buff *))
{
  SshInterceptor interceptor;
  struct sk_buff *skbp = SSH_HOOK_SKB_PTR(skb);
#ifdef DEBUG_LIGHT
  struct iphdr *iph = (struct iphdr *) SSH_SKB_GET_NETHDR(skbp);
#endif /* DEBUG_LIGHT */

  SSH_DEBUG(SSH_D_PCKDMP,
	    ("IN 0x%04x/0x%x (%s[%d]->%s[%d]) len %d "
	     "type=0x%08x %08x -> %08x dst 0x%p dev (%s[%d])",
	     htons(skbp->protocol),
	     pf,
	     (in ? in->name : "<none>"),
	     (in ? in->ifindex : -1),
	     (out ? out->name : "<none>"),
	     (out ? out->ifindex : -1),
	     skbp->len,
	     skbp->pkt_type,
	     iph->saddr, iph->daddr,
	     SSH_SKB_DST(skbp),
	     (skbp->dev ? skbp->dev->name : "<none>"),
	     (skbp->dev ? skbp->dev->ifindex : -1)
	     ));

  interceptor = ssh_interceptor_context;

  if (interceptor->enable_interception == FALSE)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      SSH_DEBUG(11, ("packet passed through"));
      return NF_ACCEPT;
    }

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.num_bytes_in += (SshUInt64) skbp->len;
    interceptor->stats.num_packets_in++;
  });

  /* If the device is to loopback, pass the packet through. */
  if (in->flags & IFF_LOOPBACK)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }

  /* The linux stack makes a copy of each locally generated
     broadcast / multicast packet. The original packet will
     be sent to network as any packet. The copy will be marked
     as PACKET_LOOPBACK and looped back to local stack.
     So we let the copy continue back to local stack. */
  if (skbp->pkt_type == PACKET_LOOPBACK)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }

  /* VPNClient code relies on skb->dev to be set.
     If packet has been processed by AF_PACKET (what tcpdump uses),
     then skb->dev has been cleared, and we must reset it here. */
  SSH_ASSERT(skbp->dev == NULL || skbp->dev == in);
  if (skbp->dev == NULL)
    {
      skbp->dev = (struct net_device *) in;
      /* Increment refcount of skbp->dev. */
      dev_hold(skbp->dev);
    }

#ifdef SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE

  /* Traverse lower priority netfilter hooks. */
  switch (pf)
    {
    case PF_INET:
      SSH_ASSERT(hooknum == SSH_NF_IP_PRE_ROUTING);
      NF_HOOK_THRESH(PF_INET, SSH_NF_IP_PRE_ROUTING, skbp,
		     (struct net_device *) in, (struct net_device *) out,
		     ssh_interceptor_packet_in_finish_ipv4,
		     ssh_nf_in4.priority + 1);
      return NF_STOLEN;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      SSH_ASSERT(hooknum == SSH_NF_IP6_PRE_ROUTING);
      NF_HOOK_THRESH(PF_INET6, SSH_NF_IP6_PRE_ROUTING, skbp,
		     (struct net_device *) in, (struct net_device *) out,
		     ssh_interceptor_packet_in_finish_ipv6,
		     ssh_nf_in6.priority + 1);
      return NF_STOLEN;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
    case SSH_NFPROTO_ARP:
      /* There is no point in looping ARP packets,
	 just continue packet processing, and return NF_STOLEN. */
      ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_ETHERNET);
      return NF_STOLEN;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

    default:
      SSH_NOTREACHED;
      return NF_DROP;
    }

#else /* SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE */

  /* Continue packet processing ssh_interceptor_packet_in_finish() */
  switch (pf)
    {
    case PF_INET:
      ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_IP4);
      return NF_STOLEN;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_IP6);
      return NF_STOLEN;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
    case SSH_NFPROTO_ARP:
      ssh_interceptor_packet_in_finish(skbp, SSH_PROTOCOL_ETHERNET);
      return NF_STOLEN;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

    default:
      break;
    }

#endif /* SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE */

  SSH_NOTREACHED;
  return NF_DROP;
}

/* Netfilter hookfn() wrapper function for IPv4 packets. */
static unsigned int
ssh_interceptor_packet_in_ipv4(unsigned int hooknum,
			       SshHookSkb *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_packet_in(PF_INET, hooknum, skb, in, out, okfn);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
/* Netfilter nf_hookfn() wrapper function for IPv6 packets. */
static unsigned int
ssh_interceptor_packet_in_ipv6(unsigned int hooknum,
			       SshHookSkb *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_packet_in(PF_INET6, hooknum, skb, in, out, okfn);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
/* Netfilter nf_hookfn() wrapper function for ARP packets. */
static unsigned int
ssh_interceptor_packet_in_arp(unsigned int hooknum,
			      SshHookSkb *skb,
			      const struct net_device *in,
			      const struct net_device *out,
			      int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_packet_in(SSH_NFPROTO_ARP, hooknum, skb,
				   in, out, okfn);
}
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */


/**************** Outbound packet interception *****************************/


/* Common code for ssh_interceptor_packet_out_finish_ipv4()
   and ssh_interceptor_packet_out_finish_ipv6().

   If SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE is set, then
   this function is called as the okfn() function from the
   netfilter infrastructure after the outbound hook iteration.
   Otherwise, this function is called directly from the outbound
   netfilter hookfn().

   This function must only be called from softirq context or
   from an exception. It will disable softirqs for the engine
   processing. This function MUST NOT be called
   from a hardirq (as then it could pre-empt itself
   on the same CPU). */
static inline int
ssh_interceptor_packet_out_finish(struct sk_buff *skbp,
				  SshInterceptorProtocol protocol)
{
  SshInterceptorInternalPacket packet;
  SshInterceptor interceptor;
  int ifnum_in;
  SshUInt32 flags;

  SSH_ASSERT(skbp->dev != NULL);
  ifnum_in = skbp->dev->ifindex;

  interceptor = ssh_interceptor_context;

  SSH_DEBUG(SSH_D_HIGHSTART,
	    ("outgoing packet 0x%p, len %d proto 0x%x [%s] iface %d [%s]",
	     skbp, skbp->len, ntohs(skbp->protocol),
             (protocol == SSH_PROTOCOL_IP4 ? "ipv4" :
	      (protocol == SSH_PROTOCOL_IP6 ? "ipv6" :
	       (protocol == SSH_PROTOCOL_ETHERNET ? "ethernet" : "unknown"))),
	     ifnum_in, skbp->dev->name));

  local_bh_disable();

  flags = SSH_PACKET_FROMPROTOCOL;

#ifdef LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING
  /* Is this a local packet which is allowed to be fragmented? */
  if (protocol == SSH_PROTOCOL_IP6 && skbp->local_df == 1)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW, ("Local packet, fragmentation allowed."));
      flags |= SSH_PACKET_FRAGMENTATION_ALLOWED;
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING */

#ifdef LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING
  /* Is this a local packet which is allowed to be fragmented? */
  if (protocol == SSH_PROTOCOL_IP4 && skbp->local_df == 1)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW, ("Local packet, fragmentation allowed."));
      flags |= SSH_PACKET_FRAGMENTATION_ALLOWED;
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING */

  /* Encapsulate the skb into a new packet. This function
     holds packet_lock during freelist manipulation. */
  packet = ssh_interceptor_packet_alloc_header(interceptor,
                                               flags,
                                               protocol,
                                               ifnum_in,
					       SSH_INTERCEPTOR_INVALID_IFNUM,
                                               skbp,
                                               FALSE, TRUE, TRUE);
  if (unlikely(packet == NULL))
    {
      SSH_DEBUG(SSH_D_FAIL, ("encapsulation failed, packet dropped"));
      local_bh_enable();
      /* Free sk_buff and return error */
      dev_kfree_skb_any(skbp);
      SSH_LINUX_STATISTICS(interceptor, { interceptor->stats.num_errors++; });
      return -EPERM;
    }

#ifdef DEBUG_LIGHT
  packet->skb->dev = NULL;
#endif /* DEBUG_LIGHT */

  SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP,
		    ("outgoing packet, len=%d flags=0x%08lx%s",
		     packet->skb->len,
		     (unsigned long) packet->packet.flags,
		     ((packet->packet.flags & SSH_PACKET_HWCKSUM) ?
		      " [hwcsum]" : "")),
                    packet->skb->data, packet->skb->len);

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.num_fastpath_bytes_out += (SshUInt64) packet->skb->len;
    interceptor->stats.num_fastpath_packets_out++;
  });

  /* Pass the packet to engine. */
  SSH_LINUX_INTERCEPTOR_PACKET_CALLBACK(interceptor,
					(SshInterceptorPacket) packet);

  local_bh_enable();

  /* Return ok */
  return 0;
}

#ifdef SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE

static inline int
ssh_interceptor_packet_out_finish_ipv4(struct sk_buff *skbp)
{
  return ssh_interceptor_packet_out_finish(skbp, SSH_PROTOCOL_IP4);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static inline int
ssh_interceptor_packet_out_finish_ipv6(struct sk_buff *skbp)
{
  return ssh_interceptor_packet_out_finish(skbp, SSH_PROTOCOL_IP6);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
#endif /* SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE */

/* ssh_interceptor_packet_out() is the common code for
   outbound netfilter hook ssh_interceptor_packet_out_ipv4()
   and ssh_interceptor_packet_out_ipv6().

   Netfilter does not provide a clean way of intercepting ALL packets
   being sent via an output chain after all other filters are processed.
   Therefore this hook is registered first, and then if
   SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE is set the packet is
   sent back to SSH_NF_IP_POST_ROUTING hook with (*okfn)()
   pointing to the actual interception function. If
   SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE is not set, then
   all following netfilter hook functions in SSH_NF_IP_POST_ROUTING hook
   are skipped.

   This function must only be called from softirq context or
   from an exception. It will disable softirqs for the engine
   processing. This function MUST NOT be called
   from a hardirq (as then it could pre-empt itself
   on the same CPU). */

static inline unsigned int
ssh_interceptor_packet_out(int pf,
                           unsigned int hooknum,
                           SshHookSkb *skb,
                           const struct net_device *in,
                           const struct net_device *out,
                           int (*okfn) (struct sk_buff *))
{
  SshInterceptor interceptor;
  struct sk_buff *skbp = SSH_HOOK_SKB_PTR(skb);
#ifdef DEBUG_LIGHT
  struct iphdr *iph = (struct iphdr *) SSH_SKB_GET_NETHDR(skbp);
#endif /* DEBUG_LIGHT */

  SSH_DEBUG(SSH_D_PCKDMP,
	    ("OUT 0x%04x/0x%x (%s[%d]->%s[%d]) len %d "
	     "type=0x%08x %08x -> %08x dst 0x%p dev (%s[%d])",
	     htons(skbp->protocol),
	     pf,
	     (in ? in->name : "<none>"),
	     (in ? in->ifindex : -1),
	     (out ? out->name : "<none>"),
	     (out ? out->ifindex : -1),
	     skbp->len,
	     skbp->pkt_type,
	     iph->saddr, iph->daddr,
	     SSH_SKB_DST(skbp),
	     (skbp->dev ? skbp->dev->name : "<none>"),
	     (skbp->dev ? skbp->dev->ifindex : -1)
	     ));

  interceptor = ssh_interceptor_context;

  if (interceptor->enable_interception == FALSE)
    {
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      SSH_DEBUG(11, ("packet passed through"));
      return NF_ACCEPT;
    }

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.num_packets_out++;
    interceptor->stats.num_bytes_out += (SshUInt64) skbp->len;
  });

  /* If the device is to loopback, pass the packet through. */
  if (out->flags & IFF_LOOPBACK)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW, ("loopback packet passed through"));
      SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP, ("len=%d", skbp->len),
			skbp->data, skbp->len);
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }

  /* Linux network stack creates a copy of locally generated broadcast
     and multicast packets, and sends the copies to local stack using
     'ip_dev_loopback_xmit' or 'ip6_dev_loopback_xmit' as the NFHOOK
     okfn. Intercept the original packets and let the local copies go
     through. */
  if (pf == PF_INET &&
      okfn != interceptor->linux_fn.ip_finish_output)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("local IPv4 broadcast loopback packet passed through"));
      SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP, ("len=%d", skbp->len),
			skbp->data, skbp->len);
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  if (pf == PF_INET6 &&
      okfn != interceptor->linux_fn.ip6_output_finish)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("local IPv6 broadcast loopback packet passed through"));
      SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP, ("len=%d", skbp->len),
			skbp->data, skbp->len);
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_IP_ONLY_PASSTHROUGH_NDISC
  if (pf == PF_INET6 &&
      skbp->sk == dev_net(skbp->dev)->ipv6.ndisc_sk)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("Neighbour discovery packet passed through"));
      SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP,
			("length %d dumping %d bytes",
			 (int) skbp->len, (int) skb_headlen(skbp)),
			skbp->data, skb_headlen(skbp));
      SSH_LINUX_STATISTICS(interceptor,
      { interceptor->stats.num_passthrough++; });
      return NF_ACCEPT;
    }
#endif /* LINUX_IP_ONLY_PASSTHROUGH_NDISC */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  /* Assert that we are about to intercept the packet from
     the correct netfilter hook on the correct path. */
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  SSH_ASSERT(okfn == interceptor->linux_fn.ip_finish_output ||
	     okfn == interceptor->linux_fn.ip6_output_finish);
#else /* SSH_LINUX_INTERCEPTOR_IPV6 */
  SSH_ASSERT(okfn == interceptor->linux_fn.ip_finish_output);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifdef SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE

  /* Traverse lower priority netfilter hooks. */
  switch (pf)
    {
    case PF_INET:
      SSH_ASSERT(hooknum == SSH_NF_IP_POST_ROUTING);
      NF_HOOK_THRESH(PF_INET, SSH_NF_IP_POST_ROUTING, skbp,
		     (struct net_device *) in, (struct net_device *) out,
		     ssh_interceptor_packet_out_finish_ipv4,
		     ssh_nf_out4.priority + 1);
      return NF_STOLEN;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      SSH_ASSERT(hooknum == SSH_NF_IP6_POST_ROUTING);
      NF_HOOK_THRESH(PF_INET6, SSH_NF_IP6_POST_ROUTING, skbp,
		     (struct net_device *) in, (struct net_device *) out,
		     ssh_interceptor_packet_out_finish_ipv6,
		     ssh_nf_out6.priority + 1);
      return NF_STOLEN;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    default:
      SSH_NOTREACHED;
      return NF_DROP;
    }

#else /* SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE */

  /* Continue packet processing in ssh_interceptor_packet_out_finish() */
  switch (pf)
    {
    case PF_INET:
      SSH_ASSERT(hooknum == SSH_NF_IP_POST_ROUTING);
      ssh_interceptor_packet_out_finish(skbp, SSH_PROTOCOL_IP4);
      return NF_STOLEN;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      SSH_ASSERT(hooknum == SSH_NF_IP6_POST_ROUTING);
      ssh_interceptor_packet_out_finish(skbp, SSH_PROTOCOL_IP6);
      return NF_STOLEN;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    default:
      break;
    }
#endif /* SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE */

  SSH_NOTREACHED;
  return NF_DROP;
}

/* Netfilter nf_hookfn() wrapper function for IPv4 packets. */
static unsigned int
ssh_interceptor_packet_out_ipv4(unsigned int hooknum,
                                SshHookSkb *skb,
                                const struct net_device *in,
                                const struct net_device *out,
                                int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_packet_out(PF_INET, hooknum, skb, in, out, okfn);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
/* Netfilter nf_hookfn() wrapper function for IPv6 packets. */
static unsigned int
ssh_interceptor_packet_out_ipv6(unsigned int hooknum,
                                SshHookSkb *skb,
                                const struct net_device *in,
                                const struct net_device *out,
                                int (*okfn) (struct sk_buff *))
{
  struct sk_buff *skbp = SSH_HOOK_SKB_PTR(skb);
  if (skbp->dev == NULL)
    skbp->dev = SSH_SKB_DST(skbp)->dev;

  return ssh_interceptor_packet_out(PF_INET6, hooknum, skb, in, out, okfn);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */


/**************** Sending packets ******************************************/

/* Netfilter okfn() for sending packets to network after
   SSH_NF_IP_FORWARD hook traversal. */

static inline int
ssh_interceptor_send_to_network(int pf, struct sk_buff *skbp)
{
  skbp->pkt_type = PACKET_OUTGOING;

#ifdef CONFIG_NETFILTER_DEBUG
#ifdef LINUX_HAS_SKB_NFDEBUG
  if (pf == PF_INET)
    {
      /* Mark SSH_NF_IP_LOCAL_OUT chains visited */
      if (skbp->sk)
	skbp->nf_debug = ((1 << SSH_NF_IP_LOCAL_OUT)
			  | (1 << SSH_NF_IP_POST_ROUTING));

      /* skbp is unowned, netfilter thinks this is a forwarded skb.
	 Mark SSH_NF_IP_PRE_ROUTING, SSH_NF_IP_FORWARD,
	 and SSH_NF_IP_POST_ROUTING
	 chains visited */
      else
	skbp->nf_debug = ((1 << SSH_NF_IP_PRE_ROUTING)
			  | (1 << SSH_NF_IP_FORWARD)
			  | (1 << SSH_NF_IP_POST_ROUTING));
    }
#endif /* LINUX_HAS_SKB_NFDEBUG */
#endif /* CONFIG_NETFILTER_DEBUG */

  SSH_LINUX_STATISTICS(ssh_interceptor_context,
  {
    ssh_interceptor_context->stats.num_packets_sent++;
    ssh_interceptor_context->stats.num_bytes_sent += (SshUInt64) skbp->len;
  });

#ifdef SSH_LINUX_NF_POST_ROUTING_AFTER_ENGINE
  /* Traverse lower priority netfilter hooks. */
  switch (pf)
    {
    case PF_INET:
      return NF_HOOK_THRESH(PF_INET, SSH_NF_IP_POST_ROUTING, skbp,
			    NULL, skbp->dev,
			    ssh_interceptor_context->
			    linux_fn.ip_finish_output,
			    ssh_nf_out4.priority + 1);

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      return NF_HOOK_THRESH(PF_INET6, SSH_NF_IP6_POST_ROUTING, skbp,
			    NULL, skbp->dev,
			    ssh_interceptor_context->
			    linux_fn.ip6_output_finish,
			    ssh_nf_out6.priority + 1);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    default:
      break;
    }

#else /* SSH_LINUX_NF_POST_ROUTING_AFTER_ENGINE */
  /* Pass packet to output path. */
  switch (pf)
    {
    case PF_INET:
      return (*ssh_interceptor_context->linux_fn.ip_finish_output)(skbp);

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      return (*ssh_interceptor_context->linux_fn.ip6_output_finish)(skbp);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

    default:
      break;
    }
#endif /* SSH_LINUX_NF_POST_ROUTING_AFTER_ENGINE */

  SSH_NOTREACHED;
  dev_kfree_skb_any(skbp);
  return -EPERM;
}

static inline int
ssh_interceptor_send_to_network_ipv4(struct sk_buff *skbp)
{
  return ssh_interceptor_send_to_network(PF_INET, skbp);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static inline int
ssh_interceptor_send_to_network_ipv6(struct sk_buff *skbp)
{
  return ssh_interceptor_send_to_network(PF_INET6, skbp);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

/* ssh_interceptor_send() sends a packet to the network or to the
   protocol stacks.  This will eventually free the packet by calling
   ssh_interceptor_packet_free.  The packet header should not be
   touched once this function has been called.

   ssh_interceptor_send() function for both media level and IP level
   interceptor. This grabs a packet with media layer headers attached
   and sends it to the interface defined by 'pp->ifnum_out'. */
void
ssh_interceptor_send(SshInterceptor interceptor,
                     SshInterceptorPacket pp,
                     size_t media_header_len)
{
  SshInterceptorInternalPacket ipp = (SshInterceptorInternalPacket) pp;
  struct net_device *dev;
#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef SSH_LINUX_NF_FORWARD_AFTER_ENGINE
  struct net_device *in_dev = NULL;
#endif /* SSH_LINUX_NF_FORWARD_AFTER_ENGINE */
#endif /*SSH_IPSEC_IP_ONLY_INTERCEPTOR */
  unsigned char *neth;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  size_t offset;
  SshUInt8 ipproto;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  SSH_DEBUG(SSH_D_HIGHSTART,
	    ("sending packet to %s, "
	     "len=%d flags=0x%08lx ifnum_out=%lu protocol=%s[0x%x]",
	     ((pp->flags & SSH_PACKET_FROMPROTOCOL) ? "network" :
	      ((pp->flags & SSH_PACKET_FROMADAPTER) ? "stack" :
	       "nowhere")),
	     ipp->skb->len, (unsigned long) pp->flags,
	     (unsigned long) pp->ifnum_out,
	     (pp->protocol == SSH_PROTOCOL_IP4 ? "ipv4" :
	      (pp->protocol == SSH_PROTOCOL_IP6 ? "ipv6" :
	       (pp->protocol == SSH_PROTOCOL_ARP ? "arp" :
		(pp->protocol == SSH_PROTOCOL_ETHERNET ? "ethernet" :
		 "unknown")))),
	     pp->protocol));

  SSH_DEBUG_HEXDUMP(SSH_D_PCKDMP, ("packet, len %d", ipp->skb->len),
		    ipp->skb->data, ipp->skb->len);

  /* Require that any references to previous devices
     were released by the entrypoint hooks. */
  SSH_ASSERT(ipp->skb->dev == NULL);

  /* Map 'pp->ifnum_out' to a net_device.
     This will dev_hold() the net_device. */
  dev = ssh_interceptor_ifnum_to_netdev(interceptor, pp->ifnum_out);
  if (dev == NULL)
    {
      SSH_DEBUG(SSH_D_UNCOMMON,
		("Interface %lu has disappeared, dropping packet 0x%p",
		 (unsigned long) pp->ifnum_out, ipp->skb));
      goto error;
    }
  ipp->skb->dev = dev;

  /* Verify that packet has enough headroom to be sent out via `skb->dev'. */
  ipp->skb =
    ssh_interceptor_packet_verify_headroom(ipp->skb, media_header_len);
  if (ipp->skb == NULL)
    {
      SSH_DEBUG(SSH_D_UNCOMMON,
		("Could not add headroom to skbp, dropping packet 0x%p",
		 ipp->skb));
      goto error;
    }

#ifdef INTERCEPTOR_IP_ALIGNS_PACKETS
  /* Align IP header to word boundary. */
  if (!ssh_interceptor_packet_align(pp, media_header_len))
    {
      pp = NULL;
      goto error;
    }
#endif /* INTERCEPTOR_IP_ALIGNS_PACKETS */

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
  /* Copy the linux nfmark from the extension slot indexed by
     SSH_LINUX_FWMARK_EXTENSION_SELECTOR. */
  SSH_SKB_MARK(ipp->skb) = pp->extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR];
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

  /* Check if the engine has cleared the SSH_PACKET_HWCKSUM flag. */
  if ((pp->flags & SSH_PACKET_HWCKSUM) == 0)
    ipp->skb->ip_summed = CHECKSUM_NONE;

  /* Clear control buffer, as packet contents might have changed. */
  if ((pp->flags & SSH_PACKET_UNMODIFIED) == 0)
    memset(ipp->skb->cb, 0, sizeof(ipp->skb->cb));

  /* Send to network */
  if (pp->flags & SSH_PACKET_FROMPROTOCOL)
    {
      /* Network header pointer is required by tcpdump. */
      SSH_SKB_SET_NETHDR(ipp->skb, ipp->skb->data + media_header_len);

      /* Let unmodified packets pass on as if they were never intercepted.
	 Note that this expects that skb->dst has not been cleared or modified
	 during Engine processing. */
      if (pp->flags & SSH_PACKET_UNMODIFIED)
	{
	  SSH_DEBUG(SSH_D_HIGHOK, ("Passing unmodified packet to network"));

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	  /* Remove the media header that was prepended to the packet
	     in the inbound netfilter hook. Update skb->protocol and
	     pp->protocol. */
	  if (media_header_len > 0)
	    {
	      SSH_ASSERT(ipp->skb->len >= media_header_len);
	      SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);
	      ipp->skb->protocol = ssh_ethertype_to_skb_proto(pp->protocol,
							      media_header_len,
							      ipp->skb->data);
	      skb_pull(ipp->skb, media_header_len);
	      if (ntohs(ipp->skb->protocol) == ETH_P_IP)
		pp->protocol = SSH_PROTOCOL_IP4;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	      else if (ntohs(ipp->skb->protocol) == ETH_P_IPV6)
		pp->protocol = SSH_PROTOCOL_IP6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
	      else
		{
		  SSH_DEBUG(SSH_D_FAIL,
			    ("Invalid skb protocol %d, dropping packet",
			     ntohs(ipp->skb->protocol)));
		  goto error;
		}
	    }
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	  if (SSH_SKB_DST(ipp->skb) == NULL)
	    {
	      SSH_DEBUG(SSH_D_FAIL, ("Invalid skb->dst, dropping packet"));
	      goto error;
	    }

	  switch (pp->protocol)
	    {
	    case SSH_PROTOCOL_IP4:
	      SSH_LINUX_STATISTICS(interceptor,
				   { interceptor->stats.num_passthrough++; });
	      ssh_interceptor_send_to_network_ipv4(ipp->skb);
	      break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	    case SSH_PROTOCOL_IP6:
	      SSH_LINUX_STATISTICS(interceptor,
				   { interceptor->stats.num_passthrough++; });
	      ssh_interceptor_send_to_network_ipv6(ipp->skb);
	      break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

	    default:
	      SSH_DEBUG(SSH_D_ERROR,
			("pp->protocol 0x%x ipp->skb->protocol 0x%x",
			 pp->protocol, ipp->skb->protocol));
	      SSH_NOTREACHED;
	      goto error;
	    }

	  /* All done. */
	  goto sent;
	}

#ifdef DEBUG_LIGHT
      if (
#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
	  ipp->skb->ip_summed == CHECKSUM_PARTIAL
#else /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
	  ipp->skb->ip_summed == CHECKSUM_HW
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
	  )
	SSH_DEBUG(SSH_D_LOWOK, ("Hardware performs checksumming."));
      else if (ipp->skb->ip_summed == CHECKSUM_NONE)
	SSH_DEBUG(SSH_D_LOWOK, ("Checksum calculated in software."));
      else
	SSH_DEBUG(SSH_D_LOWOK, ("No checksumming required."));
#endif /* DEBUG_LIGHT */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR

      /* Media level */
      SSH_ASSERT(media_header_len <= ipp->skb->len);
      SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);

      /* Set ipp->skb->protocol */
      SSH_ASSERT(skb_headlen(ipp->skb) >= media_header_len);
      ipp->skb->protocol = ssh_ethertype_to_skb_proto(pp->protocol,
						      media_header_len,
						      ipp->skb->data);

      SSH_LINUX_STATISTICS(interceptor,
      {
	interceptor->stats.num_packets_sent++;
	interceptor->stats.num_bytes_sent += (SshUInt64) ipp->skb->len;
      });

      /* Pass packet to network device driver. */
      dev_queue_xmit(ipp->skb);

      /* All done. */
      goto sent;

#else /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

      /* IP level */

      /* Set ipp->skb->protocol */
      ipp->skb->protocol = ssh_proto_to_skb_proto(pp->protocol);

#ifdef SSH_LINUX_NF_FORWARD_AFTER_ENGINE

      /* Prepare to pass forwarded packets through
	 SSH_NF_IP_FORWARD netfilter hook. */
      if (pp->flags & SSH_PACKET_FORWARDED)
	{
	  /* Map 'pp->ifnum_in' to a net_device. */
	  in_dev = ssh_interceptor_ifnum_to_netdev(interceptor, pp->ifnum_in);

	  SSH_DEBUG(SSH_D_PCKDMP,
		    ("FWD 0x%04x/%d (%s[%d]->%s[%d]) len %d "
		     "type=0x%08x dst 0x%08x",
		     ntohs(ipp->skb->protocol), pp->protocol,
		     (in_dev ? in_dev->name : "<none>"),
		     (in_dev ? in_dev->ifindex : -1),
		     (ipp->skb->dev ? ipp->skb->dev->name : "<none>"),
		     (ipp->skb->dev ? ipp->skb->dev->ifindex : -1),
		     ipp->skb->len, ipp->skb->pkt_type, SSH_SKB_DST(ipp->skb)));

	  SSH_DEBUG(SSH_D_HIGHSTART,
		    ("forwarding packet 0x%08x, len %d proto 0x%x [%s]",
		     ipp->skb, ipp->skb->len, ntohs(ipp->skb->protocol),
		     (pp->protocol == SSH_PROTOCOL_IP4 ? "ipv4" :
		      (pp->protocol == SSH_PROTOCOL_IP6 ? "ipv6" :
		       (pp->protocol == SSH_PROTOCOL_ARP ? "arp" :
			"unknown")))));

	  /* Change pkt_type to PACKET_HOST, which is expected
	     in the SSH_NF_IP_FORWARD hook. It is set to PACKET_OUTGOING
	     in ssh_interceptor_send_to_network_*() */
	  ipp->skb->pkt_type = PACKET_HOST;
	}
#endif /* SSH_LINUX_NF_FORWARD_AFTER_ENGINE */

      SSH_ASSERT(media_header_len == 0);

      switch (pp->protocol)
	{
	case SSH_PROTOCOL_IP4:
	  /* Set ipp->skb->dst */
	  if (!ssh_interceptor_reroute_skb_ipv4(interceptor,
						ipp->skb,
						pp->route_selector,
						pp->ifnum_in))
	    {
	      SSH_DEBUG(SSH_D_UNCOMMON,
			("Unable to reroute skb 0x%p", ipp->skb));
	      goto error;
	    }
	  SSH_ASSERT(SSH_SKB_DST(ipp->skb) != NULL);
#ifdef SSH_LINUX_NF_FORWARD_AFTER_ENGINE
	  /* Pass forwarded packets to SSH_NF_IP_FORWARD netfilter hook */
	  if (pp->flags & SSH_PACKET_FORWARDED)
	    {
	      NF_HOOK(PF_INET, SSH_NF_IP_FORWARD, ipp->skb,
		      in_dev, ipp->skb->dev,
		      ssh_interceptor_send_to_network_ipv4);
	    }
	  /* Send local packets directly to network. */
	  else
#endif /* SSH_LINUX_NF_FORWARD_AFTER_ENGINE */
	    ssh_interceptor_send_to_network_ipv4(ipp->skb);
	  break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	case SSH_PROTOCOL_IP6:
	  /* Set ipp->skb->dst */
	  if (!ssh_interceptor_reroute_skb_ipv6(interceptor,
						ipp->skb,
						pp->route_selector,
						pp->ifnum_in))
	    {
	      SSH_DEBUG(SSH_D_UNCOMMON,
			("Unable to reroute skb 0x%p", ipp->skb));
	      goto error;
	    }
	  SSH_ASSERT(SSH_SKB_DST(ipp->skb) != NULL);
#ifdef SSH_LINUX_NF_FORWARD_AFTER_ENGINE
	  /* Pass forwarded packets to SSH_NF_IP6_FORWARD netfilter hook */
	  if (pp->flags & SSH_PACKET_FORWARDED)
	    {
	      NF_HOOK(PF_INET6, SSH_NF_IP6_FORWARD, ipp->skb,
		      in_dev, ipp->skb->dev,
		      ssh_interceptor_send_to_network_ipv6);
	    }
	  /* Send local packets directly to network. */
	  else
#endif /* SSH_LINUX_NF_FORWARD_AFTER_ENGINE */
	    ssh_interceptor_send_to_network_ipv6(ipp->skb);
	  break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

	default:
	  SSH_DEBUG(SSH_D_ERROR,
		    ("pp->protocol 0x%x ipp->skb->protocol 0x%x",
		     pp->protocol, ipp->skb->protocol));
	  SSH_NOTREACHED;
	  goto error;
	}

      /* All done. */
      goto sent;

#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */
    }

  /* Send to stack */
  else if (pp->flags & SSH_PACKET_FROMADAPTER)
    {
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
      SshUInt32 pkt_len4;
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      SshUInt32 pkt_len6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

      /* Network header pointer is required by tcpdump. */
      SSH_SKB_SET_NETHDR(ipp->skb, ipp->skb->data + media_header_len);

      /* Let unmodified packets pass on as if they were never intercepted.
	 Note that this expects that SSH_PACKET_UNMODIFIED packets are either
	 IPv4 or IPv6. Currently there is no handling for ARP, as the Engine
	 never sets SSH_PACKET_UNMODIFIED for ARP packets. */
      if (pp->flags & SSH_PACKET_UNMODIFIED)
	{
	  SSH_DEBUG(SSH_D_HIGHOK, ("Passing unmodified packet to stack"));

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	  /* Remove the media header that was prepended to the packet
	     in the inbound netfilter hook. Update skb->protocol and
	     pp->protocol. */
	  if (media_header_len > 0)
	    {
	      SSH_ASSERT(ipp->skb->len >= media_header_len);
	      SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);
	      ipp->skb->protocol = ssh_ethertype_to_skb_proto(pp->protocol,
							      media_header_len,
							      ipp->skb->data);
	      skb_pull(ipp->skb, media_header_len);
	      if (ntohs(ipp->skb->protocol) == ETH_P_IP)
		pp->protocol = SSH_PROTOCOL_IP4;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	      else if (ntohs(ipp->skb->protocol) == ETH_P_IPV6)
		pp->protocol = SSH_PROTOCOL_IP6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
	      else
		{
		  SSH_DEBUG(SSH_D_FAIL,
			    ("Invalid skb protocol %d, dropping packet",
			     ntohs(ipp->skb->protocol)));
		  goto error;
		}
	    }
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	  switch (pp->protocol)
	    {
	    case SSH_PROTOCOL_IP4:
	      SSH_LINUX_STATISTICS(interceptor,
				   { interceptor->stats.num_passthrough++; });
#ifdef SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE
	      SSH_DEBUG(SSH_D_LOWOK, ("Passing skb 0x%p to NF_IP_PRE_ROUTING",
				      ipp->skb));
	      NF_HOOK_THRESH(PF_INET, SSH_NF_IP_PRE_ROUTING,
			     ipp->skb, ipp->skb->dev, NULL,
			     interceptor->nf->linux_fn.ip_rcv_finish,
			     ssh_nf_in4.priority + 1);
#else /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	      /* Call SSH_NF_IP_PREROUTING okfn() directly */
	      SSH_DEBUG(SSH_D_LOWOK, ("Passing skb 0x%p to ip_rcv_finish",
				      ipp->skb));
	      (*interceptor->linux_fn.ip_rcv_finish)(ipp->skb);
#endif /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	      break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	    case SSH_PROTOCOL_IP6:
	      SSH_LINUX_STATISTICS(interceptor,
				   { interceptor->stats.num_passthrough++; });
#ifdef SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE
	      SSH_DEBUG(SSH_D_LOWOK, ("Passing skb 0x%p to NF_IP6_PRE_ROUTING",
				      ipp->skb));
	      NF_HOOK_THRESH(PF_INET6, SSH_NF_IP6_PRE_ROUTING, ipp->skb,
			     ipp->skb->dev, NULL,
			     interceptor->nf->linux_fn.ip6_rcv_finish,
			     ssh_nf_out6.priority + 1);
#else /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	      /* Call SSH_NF_IP6_PREROUTING okfn() directly */
	      SSH_DEBUG(SSH_D_LOWOK, ("Passing skb 0x%p to ip6_rcv_finish",
				      ipp->skb));
	      (*interceptor->linux_fn.ip6_rcv_finish)(ipp->skb);
#endif /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	      break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

	    default:
	      SSH_DEBUG(SSH_D_ERROR,
			("pp->protocol 0x%x ipp->skb->protocol 0x%x",
			 pp->protocol, ipp->skb->protocol));
	      SSH_NOTREACHED;
	      goto error;
	    }

	  /* All done. */
	  goto sent;
	}

      /* If we do not wish to keep the broadcast state of
         the packet, then reset the pkt_type to PACKET_HOST. */
      if (!((ipp->skb->pkt_type == PACKET_MULTICAST
	     || ipp->skb->pkt_type == PACKET_BROADCAST)
            && (pp->flags & SSH_PACKET_MEDIABCAST) != 0))
        ipp->skb->pkt_type = PACKET_HOST;

      /* Clear old routing decision */
      if (SSH_SKB_DST(ipp->skb))
        {
          dst_release(SSH_SKB_DST(ipp->skb));
          SSH_SKB_DST_SET(ipp->skb, NULL);
        }

      /* If the packet has an associated SKB and that SKB is associated
	 with a socket, orphan the skb from it's owner. These situations
	 may arise when sending packets towards the protocol when
	 the packet has been turned around by the engine. */
      skb_orphan(ipp->skb);

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR

      /* Media level */

      /* If the packet does not include a media level header (for
	 example in case of pppoe), calling eth_type_trans() will
	 corrupt the beginning of packet. Instead skb->protocol must
	 be set from pp->protocol. */
      if (media_header_len == 0)
	{
	  SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);
	  ipp->skb->protocol = ssh_proto_to_skb_proto(pp->protocol);
	}
      else
	{
	  /* Workaround for 802.2Q VLAN interfaces.
	     Calling eth_type_trans() would corrupt these packets,
	     as dev->hard_header_len includes the VLAN tag, but the
	     packet does not. */
	  if (ipp->skb->dev->priv_flags & IFF_802_1Q_VLAN)
	    {
	      struct ethhdr *ethernet;

	      SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);
	      ethernet = ssh_get_eth_hdr(ipp->skb);
	      ipp->skb->protocol = ethernet->h_proto;
	      skb_pull(ipp->skb, media_header_len);
	    }

	  /* For all other packets, call eth_type_trans() to
	     set the protocol and the skb pointers. */
	  else
	    ipp->skb->protocol = eth_type_trans(ipp->skb, ipp->skb->dev);
	}
#else /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

      /* IP level */

      /* Assert that the media_header_len is always zero. */
      SSH_ASSERT(media_header_len == 0);
      SSH_SKB_SET_MACHDR(ipp->skb, ipp->skb->data);
      ipp->skb->protocol = ssh_proto_to_skb_proto(pp->protocol);

#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#ifdef DEBUG_LIGHT
      if (ipp->skb->ip_summed == CHECKSUM_NONE)
	SSH_DEBUG(SSH_D_LOWOK, ("Checksum is verified in software"));
      else if (ipp->skb->ip_summed == CHECKSUM_UNNECESSARY)
	SSH_DEBUG(SSH_D_LOWOK, ("Hardware claims to have verified checksum"));
      else if (
#ifdef LINUX_HAS_NEW_CHECKSUM_FLAGS
	       ipp->skb->ip_summed == CHECKSUM_COMPLETE
#else /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
	       ipp->skb->ip_summed == CHECKSUM_HW
#endif /* LINUX_HAS_NEW_CHECKSUM_FLAGS */
	       )
	SSH_DEBUG(SSH_D_LOWOK, ("Hardware has verified checksum, csum 0x%x",
				SSH_SKB_CSUM(ipp->skb)));
      /* ip_summed is CHECKSUM_PARTIAL, this should never happen. */
      else
	SSH_DEBUG(SSH_D_ERROR, ("Invalid HW checksum flag %d",
				ipp->skb->ip_summed));
#endif /* DEBUG_LIGHT */

      /* Set nh pointer */
      SSH_SKB_SET_NETHDR(ipp->skb, ipp->skb->data);
      switch(ntohs(ipp->skb->protocol))
	{
	case ETH_P_IP:
          neth = SSH_SKB_GET_NETHDR(ipp->skb);
	  if (neth == NULL)
	    {
	      SSH_DEBUG(SSH_D_ERROR, ("Could not access IP header"));
	      goto error;
	    }
          SSH_SKB_SET_TRHDR(ipp->skb, neth + SSH_IPH4_HLEN(neth) * 4);

#ifdef CONFIG_NETFILTER_DEBUG
#ifdef LINUX_HAS_SKB_NFDEBUG
	  /* Mark SSH_NF_IP_PRE_ROUTING visited */
	  ipp->skb->nf_debug = (1 << SSH_NF_IP_PRE_ROUTING);
#endif /* LINUX_HAS_SKB_NFDEBUG */
#endif /* CONFIG_NETFILTER_DEBUG */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	  /* Remove padding from packet. */
	  pkt_len4 = SSH_IPH4_LEN(neth);
	  SSH_ASSERT(pkt_len4 >= SSH_IPH4_HDRLEN && pkt_len4 <= 0xffff);
	  if (pkt_len4 != ipp->skb->len)
	    {
	      SSH_DEBUG(SSH_D_NICETOKNOW, ("Trimming skb down from %d to %lu",
					   ipp->skb->len,
					   (unsigned long) pkt_len4));
	      skb_trim(ipp->skb, pkt_len4);
	    }
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	  SSH_LINUX_STATISTICS(interceptor,
	  {
	    interceptor->stats.num_packets_sent++;
	    interceptor->stats.num_bytes_sent += (SshUInt64) ipp->skb->len;
	  });

#ifdef SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE
	  NF_HOOK_THRESH(PF_INET, SSH_NF_IP_PRE_ROUTING,
			 ipp->skb, ipp->skb->dev, NULL,
			 interceptor->linux_fn.ip_rcv_finish,
			 ssh_nf_in4.priority + 1);
#else /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  /* Call SSH_NF_IP_PREROUTING okfn() directly */
	  (*interceptor->linux_fn.ip_rcv_finish)(ipp->skb);
#endif /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	case ETH_P_IPV6:
          neth = SSH_SKB_GET_NETHDR(ipp->skb);
	  if (neth == NULL)
	    {
	      SSH_DEBUG(SSH_D_ERROR, ("Could not access IPv6 header"));
	      goto error;
	    }

	  ipproto = SSH_IPH6_NH(neth);
          pkt_len6 = SSH_IPH6_LEN(neth) + SSH_IPH6_HDRLEN;

	  /* Parse hop-by-hop options and update IPv6 control buffer. */
	  SSH_LINUX_IP6CB(ipp->skb)->iif = ipp->skb->dev->ifindex;
	  SSH_LINUX_IP6CB(ipp->skb)->hop = 0;
	  SSH_LINUX_IP6CB(ipp->skb)->ra = 0;
#ifdef LINUX_HAS_IP6CB_NHOFF
	  SSH_LINUX_IP6CB(ipp->skb)->nhoff = SSH_IPH6_OFS_NH;
#endif /* LINUX_HAS_IP6CB_NHOFF */

	  offset = SSH_IPH6_HDRLEN;
	  /* Ipproto == HOPOPT */
	  if (ipproto == 0)
	    {
	      unsigned char *opt_ptr = neth + offset + 2;
	      int opt_len;

	      ipproto = SSH_IP6_EXT_COMMON_NH(neth + offset);
	      offset += SSH_IP6_EXT_COMMON_LENB(neth + offset);

	      while (opt_ptr < neth + offset)
		{
		  opt_len = opt_ptr[1] + 2;
		  switch (opt_ptr[0])
		    {
		      /* PAD0 */
		    case 0:
		      opt_len = 1;
		      break;

		      /* PADN */
		    case 1:
		      break;

		      /* Jumbogram */
		    case 194:
		      /* Take packet len from option (skb->len is zero). */
		      pkt_len6 = SSH_GET_32BIT(&opt_ptr[2])
			+ sizeof(struct ipv6hdr);
		      break;

		      /* Router alert */
		    case 5:
                      SSH_LINUX_IP6CB(ipp->skb)->ra = opt_ptr - neth;
                      break;

		      /* Unknown / unsupported */
		    default:
		      /* Just skip unknown options. */
		      break;
		    }
		  opt_ptr += opt_len;
		}
	      SSH_LINUX_IP6CB(ipp->skb)->hop = sizeof(struct ipv6hdr);

#ifdef LINUX_HAS_IP6CB_NHOFF
	      SSH_LINUX_IP6CB(ipp->skb)->nhoff = sizeof(struct ipv6hdr);
#endif /* LINUX_HAS_IP6CB_NHOFF */
	    }
	  SSH_SKB_SET_TRHDR(ipp->skb, neth + offset);

	  /* Remove padding from packet. */
	  SSH_ASSERT(pkt_len6 >= sizeof(struct ipv6hdr));
	  if (pkt_len6 != ipp->skb->len)
	    {
	      SSH_DEBUG(SSH_D_NICETOKNOW, ("Trimming skb down from %d to %lu",
					   ipp->skb->len,
					   (unsigned long) pkt_len6));
	      skb_trim(ipp->skb, pkt_len6);
	    }

	  SSH_LINUX_STATISTICS(interceptor,
	  {
	    interceptor->stats.num_packets_sent++;
	    interceptor->stats.num_bytes_sent += (SshUInt64) ipp->skb->len;
	  });

#ifdef SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE
	  NF_HOOK_THRESH(PF_INET6, SSH_NF_IP6_PRE_ROUTING, ipp->skb,
			 ipp->skb->dev, NULL,
			 interceptor->linux_fn.ip6_rcv_finish,
			 ssh_nf_out6.priority + 1);
#else /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  /* Call SSH_NF_IP6_PREROUTING okfn() directly */
	  (*interceptor->linux_fn.ip6_rcv_finish)(ipp->skb);
#endif /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	case ETH_P_ARP:
	  SSH_LINUX_STATISTICS(interceptor,
	  {
	    interceptor->stats.num_packets_sent++;
	    interceptor->stats.num_bytes_sent += (SshUInt64) ipp->skb->len;
	  });
#ifdef SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE
	  NF_HOOK_THRESH(SSH_NFPROTO_ARP, NF_ARP_IN,
			 ipp->skb, ipp->skb->dev, NULL,
			 interceptor->linux_fn.arp_process,
			 ssh_nf_in_arp.priority + 1);
#else /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  /* Call NF_ARP_IN okfn() directly */
	  (*interceptor->linux_fn.arp_process)(ipp->skb);
#endif /* SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE */
	  break;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	default:
	  SSH_DEBUG(SSH_D_ERROR,
		    ("skb->protocol 0x%x", htons(ipp->skb->protocol)));
	  SSH_NOTREACHED;
	  goto error;
	}

      /* All done. */
      goto sent;
    } /* SSH_PACKET_FROMADAPTER */

  else
    {
      /* Not SSH_PACKET_FROMPROTOCOL or SSH_PACKET_FROMADAPTER. */
      SSH_DEBUG(SSH_D_ERROR, ("Invalid packet direction flags"));
      SSH_NOTREACHED;
      goto error;
    }

 sent:
  ipp->skb = NULL;

 out:
#ifdef INTERCEPTOR_IP_ALIGNS_PACKETS
  /* pp can go NULL only with packet aligning. */

  if (pp)
#endif /* INTERCEPTOR_IP_ALIGNS_PACKETS */
    ssh_interceptor_packet_free(pp);

  /* Release net_device */
  if (dev)
    ssh_interceptor_release_netdev(dev);

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef SSH_LINUX_NF_FORWARD_AFTER_ENGINE
  /* Release inbound net_device that was used for
     FORWARD NF_HOOK traversal. */
  if (in_dev)
    ssh_interceptor_release_netdev(in_dev);
#endif /* SSH_LINUX_NF_FORWARD_AFTER_ENGINE */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  return;

 error:
  SSH_LINUX_STATISTICS(interceptor, { interceptor->stats.num_errors++; });
  goto out;
}


/******************************************************* General init/uninit */

void ssh_interceptor_enable_interception(SshInterceptor interceptor,
					 Boolean enable)
{

  SSH_DEBUG(SSH_D_LOWOK, ("%s packet interception",
			  (enable ? "Enabling" : "Disabling")));
  interceptor->enable_interception = enable;
}

/* Interceptor hook init. Utility function to initialize
   individual hooks. */
static Boolean
ssh_interceptor_hook_init(struct SshLinuxHooksRec *hook)
{
  int rval;

  SSH_ASSERT(hook->is_registered == FALSE);

  if (hook->pf == PF_INET && hook->hooknum == SSH_NF_IP_PRE_ROUTING)
    hook->priority = in_priority;

  if (hook->pf == PF_INET && hook->hooknum == SSH_NF_IP_POST_ROUTING)
    hook->priority = out_priority;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  if (hook->pf == PF_INET6 && hook->hooknum == SSH_NF_IP6_PRE_ROUTING)
    hook->priority = in6_priority;

  if (hook->pf == PF_INET6 && hook->hooknum == SSH_NF_IP6_POST_ROUTING)
    hook->priority = out6_priority;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  hook->ops->hook = hook->hookfn;
  hook->ops->pf = hook->pf;
  hook->ops->hooknum = hook->hooknum;
  hook->ops->priority = hook->priority;

  rval = nf_register_hook(hook->ops);
  if (rval < 0)
    {
      if (hook->is_mandatory)
        {
          printk(KERN_ERR
                 "VPNClient netfilter %s hook failed to install.\n",
                 hook->name);
          return FALSE;
        }
      return TRUE;
    }

  hook->is_registered=TRUE;
  return TRUE;
}

/* Utility function for uninstalling a single netfilter hook. */
static void
ssh_interceptor_hook_uninit(struct SshLinuxHooksRec *hook)
{
  if (hook->is_registered == FALSE)
    return;

  nf_unregister_hook(hook->ops);

  hook->is_registered = FALSE;
}

/* IP/Network glue initialization. This must be called only
   after the engine has "opened" the interceptor, and packet_callback()
   has been set to a valid value. */
Boolean
ssh_interceptor_ip_glue_init(SshInterceptor interceptor)
{
  int i;

  /* Verify that the hooks haven't been initialized yet. */
  if (interceptor->hooks_installed)
    {
      SSH_DEBUG(2, ("init called when hooks are initialized already.\n"));
      return TRUE;
    }

  /* Register all hooks */
  for (i = 0; ssh_nf_hooks[i].name != NULL; i++)
    {
      if (ssh_interceptor_hook_init(&ssh_nf_hooks[i]) == FALSE)
        goto fail;
    }

  interceptor->hooks_installed = TRUE;
  return TRUE;

 fail:
  for (i = 0; ssh_nf_hooks[i].name != NULL; i++)
    ssh_interceptor_hook_uninit(&ssh_nf_hooks[i]);
  return FALSE;
}

/* Uninitialization of netfilter glue. */
Boolean
ssh_interceptor_ip_glue_uninit(SshInterceptor interceptor)
{
  int i;

  /* Note that we do not perform concurrency control here!
     We expect that we are essentially running single-threaded
     in init/shutdown! */

  if (interceptor->hooks_installed == FALSE)
    return TRUE;

  /* Unregister netfilter hooks */
  for (i = 0; ssh_nf_hooks[i].name != NULL; i++)
    ssh_interceptor_hook_uninit(&ssh_nf_hooks[i]);

  interceptor->hooks_installed = FALSE;

  return TRUE;
}
