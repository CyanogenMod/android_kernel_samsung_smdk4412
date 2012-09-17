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
 * linux_hook_magic.c
 *
 * Linux netfilter hook probing.
 *
 */

#include "linux_internal.h"

#include <net/checksum.h>
#include <linux/udp.h>

extern SshInterceptor ssh_interceptor_context;

/***************************** Probe hooks ***********************************/

#define SSH_LINUX_HOOK_MAGIC_NUM_PROBES 10

typedef struct SshInterceptorHookMagicOpsRec
{
  struct nf_hook_ops ops;
  struct sk_buff *skbp[SSH_LINUX_HOOK_MAGIC_NUM_PROBES];
  int probe_count;
} SshInterceptorHookMagicOpsStruct;

static SshInterceptorHookMagicOpsStruct hook_magic_in4;
static SshInterceptorHookMagicOpsStruct hook_magic_out4;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static SshInterceptorHookMagicOpsStruct hook_magic_in6;
static SshInterceptorHookMagicOpsStruct hook_magic_out6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
static SshInterceptorHookMagicOpsStruct hook_magic_arp_in;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */


static unsigned int
ssh_interceptor_hook_magic_hookfn(int pf,
				  unsigned int hooknum,
				  SshHookSkb *skb,
				  const struct net_device *in,
				  const struct net_device *out,
				  int (*okfn) (struct sk_buff *))
{
  struct sk_buff *skbp = SSH_HOOK_SKB_PTR(skb);
  int i;

  /* Grab lock */
  local_bh_disable();
  ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);

  switch (pf)
    {
    case PF_INET:
      switch (hooknum)
	{
	case SSH_NF_IP_PRE_ROUTING:
	  /* Check if this is our hook_magic probe */
	  for (i = 0; i < hook_magic_in4.probe_count; i++)
	    {
	      if (hook_magic_in4.skbp[i] == skbp)
		{
		  SSH_ASSERT(okfn != NULL);

		  /* Check if we have probed this hook */
		  if (ssh_interceptor_context->linux_fn.ip_rcv_finish == NULL)
		    {
		      /* Make sure the assignments do not get reordered */
		      barrier();

		      ssh_interceptor_context->linux_fn.ip_rcv_finish = okfn;
		      SSH_DEBUG(SSH_D_LOWOK, ("ip_rcv_finish 0x%p", okfn));
		    }
		  else
		    {
		      SSH_DEBUG(SSH_D_LOWOK,
				("Double probe received for "
				 "NF_IP_PRE_ROUTING"));
		      SSH_ASSERT(ssh_interceptor_context->
				 linux_fn.ip_rcv_finish == okfn);
		    }

		  /* Mark packet as being handled */
		  hook_magic_in4.skbp[i] = NULL;
		  goto drop;
		}
	    }

	  /* Not our probe packet, let through */
	  SSH_DEBUG(10, ("Not our skbp 0x%p", skbp));
	  goto accept;

	case SSH_NF_IP_POST_ROUTING:
	  /* Check if this is our hook_magic probe */
	  for (i = 0; i < hook_magic_out4.probe_count; i++)
	    {
	      if (hook_magic_out4.skbp[i] == skbp)
		{
		  SSH_ASSERT(okfn != NULL);

		  /* Check if we have probed this hook */
		  if (ssh_interceptor_context->linux_fn.ip_finish_output
		      == NULL)
		    {
		      /* Make sure the assignments do not get reordered */
		      barrier();

		      ssh_interceptor_context->linux_fn.ip_finish_output = okfn;
		      SSH_DEBUG(SSH_D_LOWOK, ("ip_finish_output 0x%p", okfn));
		    }
		  else
		    {
		      SSH_DEBUG(SSH_D_LOWOK,
				("Double probe received for "
				 "NF_IP_POST_ROUTING"));
		      SSH_ASSERT(ssh_interceptor_context->
				 linux_fn.ip_finish_output == okfn);
		    }

		  /* Mark packet as being handled */
		  hook_magic_out4.skbp[i] = NULL;
		  goto drop;
		}
	    }

	  /* Not our probe packet, let through */
	  SSH_DEBUG(10, ("Not our skbp 0x%p", skbp));
	  goto accept;

	default:
	  SSH_NOTREACHED;
	  break;
	}
      break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:
      switch (hooknum)
	{
	case SSH_NF_IP6_PRE_ROUTING:
	  /* Check if this is our hook_magic probe */
	  for (i = 0; i < hook_magic_in6.probe_count; i++)
	    {
	      if (hook_magic_in6.skbp[i] == skbp)
		{
		  SSH_ASSERT(okfn != NULL);

		  /* Check if we have probed this hook */
		  if (ssh_interceptor_context->linux_fn.ip6_rcv_finish == NULL)
		    {
		      /* Make sure the assignments do not get reordered */
		      barrier();

		      ssh_interceptor_context->linux_fn.ip6_rcv_finish = okfn;
		      SSH_DEBUG(SSH_D_LOWOK, ("ip6_rcv_finish 0x%p", okfn));
		    }
		  else
		    {
		      SSH_DEBUG(SSH_D_LOWOK,
				("Double probe received for "
				 "NF_IP6_PRE_ROUTING"));
		      SSH_ASSERT(ssh_interceptor_context->
				 linux_fn.ip6_rcv_finish == okfn);
		    }

		  /* Mark packet as being handled */
		  hook_magic_in6.skbp[i] = NULL;
		  goto drop;
		}
	    }

	  /* Not our probe packet, let through */
	  SSH_DEBUG(10, ("Not our skbp 0x%p", skbp));
	  goto accept;

	case SSH_NF_IP6_POST_ROUTING:
	  /* Check if this is our hook_magic probe */
	  for (i = 0; i < hook_magic_out6.probe_count; i++)
	    {
	      if (hook_magic_out6.skbp[i] == skbp)
		{
		  SSH_ASSERT(okfn != NULL);

		  /* Check if we have not yet probed this hook */
		  if (ssh_interceptor_context->linux_fn.ip6_output_finish
		      == NULL)
		    {
		      /* Make sure the assignments do not get reordered */
		      barrier();

		      ssh_interceptor_context->linux_fn.ip6_output_finish =
			okfn;
		      SSH_DEBUG(SSH_D_LOWOK, ("ip6_output_finish 0x%p", okfn));
		    }
		  else
		    {
		      SSH_DEBUG(SSH_D_LOWOK,
				("Double probe received for "
				 "NF_IP6_POST_ROUTING"));
		      SSH_ASSERT(ssh_interceptor_context->
				 linux_fn.ip6_output_finish == okfn);
		    }

		  /* Mark packet as being handled */
		  hook_magic_out6.skbp[i] = NULL;
		  goto drop;
		}
	    }

	  /* Not our probe packet, let through */
	  SSH_DEBUG(10, ("Not our skbp 0x%p", skbp));
	  goto accept;

	default:
	  SSH_NOTREACHED;
	  break;
	}
      break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
    case SSH_NFPROTO_ARP:
      /* Check if this is our hook_magic probe */
      for (i = 0; i < hook_magic_arp_in.probe_count; i++)
	{
	  if (hook_magic_arp_in.skbp[i] == skbp)
	    {
	      SSH_ASSERT(hooknum == NF_ARP_IN);
	      SSH_ASSERT(okfn != NULL);

	      /* Check if we have probed this hook */
	      if (ssh_interceptor_context->linux_fn.arp_process == NULL)
		{
		  /* Make sure the assignments do not get reordered */
		  barrier();

		  ssh_interceptor_context->linux_fn.arp_process = okfn;
		  SSH_DEBUG(SSH_D_LOWOK, ("arp_process 0x%p", okfn));
		}
	      else
		{
		  SSH_DEBUG(SSH_D_LOWOK,
			    ("Double probe received for NF_ARP_IN"));
		  SSH_ASSERT(ssh_interceptor_context->linux_fn.arp_process
			     == okfn);
		}

	      /* Mark packet as being handled */
	      hook_magic_arp_in.skbp[i] = NULL;
	      goto drop;
	    }
	}

      /* Not our probe packet, let through */
      SSH_DEBUG(10, ("Not our skbp 0x%p", skbp));
      goto accept;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

    default:
      SSH_NOTREACHED;
      break;
    }

  /* Unlock and drop the packet */
 drop:
  ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
  local_bh_enable();
  return NF_DROP;

 accept:
  /* Unlock and let packet continue */
  ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
  local_bh_enable();
  return NF_ACCEPT;
}

static unsigned int
ssh_interceptor_hook_magic_hookfn_ipv4(unsigned int hooknum,
				       SshHookSkb *skb,
				       const struct net_device *in,
				       const struct net_device *out,
				       int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_hook_magic_hookfn(PF_INET, hooknum, skb,
					   in, out, okfn);
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
static unsigned int
ssh_interceptor_hook_magic_hookfn_ipv6(unsigned int hooknum,
				       SshHookSkb *skb,
				       const struct net_device *in,
				       const struct net_device *out,
				       int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_hook_magic_hookfn(PF_INET6, hooknum, skb,
					   in, out, okfn);
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
static unsigned int
ssh_interceptor_hook_magic_hookfn_arp(unsigned int hooknum,
				      SshHookSkb *skb,
				      const struct net_device *in,
				      const struct net_device *out,
				      int (*okfn) (struct sk_buff *))
{
  return ssh_interceptor_hook_magic_hookfn(SSH_NFPROTO_ARP, hooknum, skb,
					   in, out, okfn);
}
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */


/***************************** Probe packet sending **************************/

/* Caller holds 'interceptor_lock'.
   This function will release it when calling the network stack functions,
   and will grab it back after the function has returned. */

static int
ssh_interceptor_hook_magic_send(SshInterceptorHookMagicOpsStruct *hook_magic)
{
  struct sk_buff *skbp;
  struct net_device *dev;
  struct in_device *inet_dev = NULL;
  struct iphdr *iph;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  struct inet6_dev *inet6_dev = NULL;
  struct ipv6hdr *ip6h;
  struct in6_addr addr6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  unsigned char *ptr;
  struct arphdr *arph;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
  struct udphdr *udph;

  /* Assert that we have the lock */
  ssh_kernel_mutex_assert_is_locked(ssh_interceptor_context->interceptor_lock);

  /* Find a suitable device to put on the probe skb */
  read_lock(&dev_base_lock);

  for (dev = SSH_FIRST_NET_DEVICE();
       dev != NULL;
       dev = SSH_NEXT_NET_DEVICE(dev))
    {
      if (!(dev->flags & IFF_UP))
	continue;

      inet_dev = in_dev_get(dev);
      if (inet_dev == NULL || inet_dev->ifa_list == NULL)
	goto next_dev;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      /* The device must have an IPv6 address. */
      inet6_dev = in6_dev_get(dev);
      memset(&addr6, 0, sizeof(addr6));

      if (inet6_dev != NULL)
	{
	  struct inet6_ifaddr *ifaddr6, *next_ifaddr6;
	  SSH_INET6_IFADDR_LIST_FOR_EACH(ifaddr6,
					 next_ifaddr6,
					 inet6_dev->addr_list)
	    {
	      addr6 = ifaddr6->addr;
	      break;
	    }
	}

      if (inet6_dev == NULL ||
	  memcmp(addr6.s6_addr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0)
	goto next_dev;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
      /* The device must must support ARP. */
      if ((dev->flags & IFF_NOARP) || dev->addr_len != 6)
	goto next_dev;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

      /* This device is suitable for probing. */
      break;

    next_dev:
      if (inet_dev)
	in_dev_put(inet_dev);
      inet_dev = NULL;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      if (inet6_dev)
	in6_dev_put(inet6_dev);
      inet6_dev = NULL;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
    }
  read_unlock(&dev_base_lock);
  if (dev == NULL)
    {
      SSH_DEBUG(SSH_D_ERROR,
		("Hook magic failed, could not find a suitable device"));
      goto out;
    }

  /* Allocate probe skb */
  /* Maximum size: IPv6 + UDP = 48 bytes
     ( + headroom for ethernet header + 2 bytes for proper alignment) */
  skbp = alloc_skb(48 + ETH_HLEN + 2, GFP_ATOMIC);
  if (skbp == NULL)
    {
      SSH_DEBUG(SSH_D_ERROR,
		("Hook magic failed, could not allocate skbuff"));
      goto out;
    }

  /* Reserve headroom for ethernet header.
     Make sure IP header will be properly aligned. */
  skb_reserve(skbp, ETH_HLEN + 2);

  SSH_SKB_SET_MACHDR(skbp, skbp->data);

#ifdef LINUX_HAS_SKB_MAC_LEN
  skbp->mac_len = 0;
#endif /* LINUX_HAS_SKB_MAC_LEN */
  skbp->pkt_type = PACKET_HOST;

  skbp->dev = dev;
  SSH_SKB_DST_SET(skbp, NULL);

  /* Store skb pointer to a free slot. */
  SSH_ASSERT(hook_magic->probe_count < SSH_LINUX_HOOK_MAGIC_NUM_PROBES);
  hook_magic->skbp[hook_magic->probe_count] = skbp;
  hook_magic->probe_count++;

  switch (hook_magic->ops.pf)
    {
    case PF_INET:

      /* Build an IP + UDP packet for probing */
      skbp->protocol = __constant_htons(ETH_P_IP);

      iph = (struct iphdr *) skb_put(skbp, sizeof(struct iphdr));
      if (iph == NULL)
	{
	  SSH_DEBUG(SSH_D_ERROR,
		    ("Hook magic failed, no space for IPv4 header"));
	  goto out;
	}
      SSH_SKB_SET_NETHDR(skbp, (unsigned char *) iph);

      memset(iph, 0, sizeof(struct iphdr));
      iph->version = 4;
      iph->ihl = 5;
      iph->ttl = 1;
      iph->protocol = IPPROTO_UDP;
      iph->tot_len = htons(28);

      /* Use the device addresses */
      SSH_ASSERT(inet_dev != NULL && inet_dev->ifa_list != NULL);
      iph->saddr = inet_dev->ifa_list->ifa_local;
      iph->daddr = inet_dev->ifa_list->ifa_local;

      /* Add UDP header */
      udph = (struct udphdr *) skb_put(skbp, sizeof(struct udphdr));
      if (udph == NULL)
	{
	  SSH_DEBUG(SSH_D_ERROR,
		    ("Hook magic failed, no space for UDP header"));
	  goto out;
	}
      SSH_SKB_SET_TRHDR(skbp, (unsigned char *) udph);

      udph->source = htons(9);
      udph->dest = htons(9);
      udph->len = 0;
      udph->check = 0;

      /* Checksum */
      iph->check = 0;
      iph->check = ip_fast_csum((unsigned char *) iph, iph->ihl);

      switch (hook_magic->ops.hooknum)
	{
	case SSH_NF_IP_PRE_ROUTING:
	  SSH_ASSERT(ssh_interceptor_context->
		     linux_fn.ip_rcv_finish == NULL);
	  SSH_DEBUG(SSH_D_HIGHOK,
		    ("Probing SSH_NF_IP_PRE_ROUTING with skbp 0x%p", skbp));
	  /* Release lock for duration of netif_rx() */
	  ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
	  local_bh_enable();
	  netif_rx(skbp);
	  local_bh_disable();
	  ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);
	  break;

	case SSH_NF_IP_POST_ROUTING:
	  SSH_ASSERT(ssh_interceptor_context->
		     linux_fn.ip_finish_output == NULL);
	  SSH_DEBUG(SSH_D_HIGHOK,
		    ("Probing SSH_NF_IP_POST_ROUTING with skbp 0x%p", skbp));
	  if (ssh_interceptor_reroute_skb_ipv4(ssh_interceptor_context,
					       skbp, 0, 0))
	    {
	      /* Release lock for the duration of dst->output() */
	      ssh_kernel_mutex_unlock(ssh_interceptor_context->
				      interceptor_lock);
	      dst_output(skbp);
	      ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);
	    }
	  break;

	default:
	  SSH_NOTREACHED;
	  break;
	}
      break;

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    case PF_INET6:

      /* Build an IPv6 + UDP packet for probing */
      skbp->protocol = __constant_htons(ETH_P_IPV6);
      ip6h = (struct ipv6hdr *) skb_put(skbp, sizeof(struct ipv6hdr));
      if (ip6h == NULL)
	{
	  SSH_DEBUG(SSH_D_ERROR,
		    ("Hook magic failed, no space for IPv6 header"));
	  goto out;
	}
      SSH_SKB_SET_NETHDR(skbp, (unsigned char *) ip6h);

      memset(ip6h, 0, sizeof(struct ipv6hdr));
      ip6h->version = 6;
      ip6h->hop_limit = 1;
      ip6h->nexthdr = NEXTHDR_UDP;

      /* Use the device addresses */
      SSH_ASSERT(memcmp(addr6.s6_addr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16)
		 != 0);

      ip6h->saddr = addr6;
      ip6h->daddr = addr6;

      /* Add UDP header */
      udph = (struct udphdr *) skb_put(skbp, sizeof(struct udphdr));
      if (udph == NULL)
	{
	  SSH_DEBUG(SSH_D_ERROR,
		    ("Hook magic failed, no space for UDP header"));
	  goto out;
	}
      SSH_SKB_SET_TRHDR(skbp, (unsigned char *) udph);

      udph->source = htons(9);
      udph->dest = htons(9);
      udph->len = 0;
      udph->check = 0;

      switch (hook_magic->ops.hooknum)
	{
	case SSH_NF_IP6_PRE_ROUTING:
	  SSH_ASSERT(ssh_interceptor_context->
		     linux_fn.ip6_rcv_finish == NULL);
	  SSH_DEBUG(SSH_D_HIGHOK,
		    ("Probing SSH_NF_IP6_PRE_ROUTING with skbp 0x%p", skbp));
	  /* Release lock for the duration of netif_rx() */
	  ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
	  local_bh_enable();
	  netif_rx(skbp);
	  local_bh_disable();
	  ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);
	  break;

	case SSH_NF_IP6_POST_ROUTING:
	  SSH_ASSERT(ssh_interceptor_context->
		     linux_fn.ip6_output_finish == NULL);
	  SSH_DEBUG(SSH_D_HIGHOK,
		    ("Probing SSH_NF_IP6_POST_ROUTING with skbp 0x%p", skbp));
	  if (ssh_interceptor_reroute_skb_ipv6(ssh_interceptor_context,
					       skbp, 0, 0))
	    {
	      /* Release lock for the duration of dst->output() */
	      ssh_kernel_mutex_unlock(ssh_interceptor_context->
				      interceptor_lock);
	      dst_output(skbp);
	      ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);
	    }
	  break;

	default:
	  SSH_NOTREACHED;
	  break;
	}
      break;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
    case SSH_NFPROTO_ARP:
      SSH_ASSERT(hook_magic->ops.hooknum == NF_ARP_IN);
      SSH_ASSERT(ssh_interceptor_context->linux_fn.arp_process == NULL);

      /* Build an ARP REQ packet for probing */
      skbp->protocol = __constant_htons(ETH_P_ARP);

      SSH_ASSERT(dev->addr_len == 6);
      SSH_ASSERT((sizeof(struct arphdr) + (2 * dev->addr_len) + 8) <= 48);

      arph = (struct arphdr *) skb_put(skbp, sizeof(struct arphdr) +
				       (2 * dev->addr_len) + 8);
      if (arph == NULL)
	{
	  SSH_DEBUG(SSH_D_ERROR,
		    ("Hook magic failed, no space for ARP header"));
	  goto out;
	}
      SSH_SKB_SET_NETHDR(skbp, (unsigned char *) arph);

      arph->ar_hrd = htons(ARPHRD_ETHER);
      arph->ar_pro = htons(ETH_P_IP);
      arph->ar_hln = dev->addr_len;
      arph->ar_pln = 4;
      arph->ar_op = htons(ARPOP_REQUEST);

      ptr = skbp->data + sizeof(*arph);

      /* Zero sender HA */
      memset(ptr, 0, dev->addr_len);
      ptr += dev->addr_len;

      /* Take sender IP from device */
      memcpy(ptr, &inet_dev->ifa_list->ifa_local, 4);
      ptr += 4;

      /* All f's target HA */
      memset(ptr, 0xff, dev->addr_len);
      ptr += dev->addr_len;

      /* Take target IP from device */
      memcpy(ptr, &inet_dev->ifa_list->ifa_local, 4);

      SSH_DEBUG(SSH_D_HIGHOK,
		("Probing NF_ARP_IN with skbp 0x%p", skbp));
      /* Release the lock for the duration of dst->output() */
      ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
      local_bh_enable();
      netif_rx(skbp);
      local_bh_disable();
      ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);
      break;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

    default:
      SSH_NOTREACHED;
      break;
    }

 out:
  if (inet_dev)
    in_dev_put(inet_dev);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  if (inet6_dev)
    in6_dev_put(inet6_dev);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  return 1;
}

static void
ssh_interceptor_hook_magic_uninit(void)
{
  /* Unregister probe hooks */

  if (hook_magic_in4.ops.pf == PF_INET)
    {
      nf_unregister_hook(&hook_magic_in4.ops);
      hook_magic_in4.ops.pf = 0;
    }
  if (hook_magic_out4.ops.pf == PF_INET)
    {
      nf_unregister_hook(&hook_magic_out4.ops);
      hook_magic_out4.ops.pf = 0;
    }

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  if (hook_magic_in6.ops.pf == PF_INET6)
    {
      nf_unregister_hook(&hook_magic_in6.ops);
      hook_magic_in6.ops.pf = 0;
    }
  if (hook_magic_out6.ops.pf == PF_INET6)
    {
      nf_unregister_hook(&hook_magic_out6.ops);
      hook_magic_out6.ops.pf = 0;
    }
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  if (hook_magic_arp_in.ops.pf == SSH_NFPROTO_ARP)
    {
      nf_unregister_hook(&hook_magic_arp_in.ops);
      hook_magic_arp_in.ops.pf = 0;
    }
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
}

int
ssh_interceptor_hook_magic_init()
{
  int ret;
  int i;

  /* Intialize and register probe hooks */

  /* SSH_NF_IP_PRE_ROUTING */
  ssh_interceptor_context->linux_fn.ip_rcv_finish = NULL;
  memset(&hook_magic_in4, 0, sizeof(hook_magic_in4));
  hook_magic_in4.ops.hook = ssh_interceptor_hook_magic_hookfn_ipv4;
  hook_magic_in4.ops.pf = PF_INET;
  hook_magic_in4.ops.hooknum = SSH_NF_IP_PRE_ROUTING;
  hook_magic_in4.ops.priority = SSH_NF_IP_PRI_FIRST;
  nf_register_hook(&hook_magic_in4.ops);

  /* SSH_NF_IP_POST_ROUTING */
  ssh_interceptor_context->linux_fn.ip_finish_output = NULL;
  memset(&hook_magic_out4, 0, sizeof(hook_magic_out4));
  hook_magic_out4.ops.hook = ssh_interceptor_hook_magic_hookfn_ipv4;
  hook_magic_out4.ops.pf = PF_INET;
  hook_magic_out4.ops.hooknum = SSH_NF_IP_POST_ROUTING;
  hook_magic_out4.ops.priority = SSH_NF_IP_PRI_FIRST;
  nf_register_hook(&hook_magic_out4.ops);

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  /* SSH_NF_IP6_PRE_ROUTING */
  ssh_interceptor_context->linux_fn.ip6_rcv_finish = NULL;
  memset(&hook_magic_in6, 0, sizeof(hook_magic_in6));
  hook_magic_in6.ops.hook = ssh_interceptor_hook_magic_hookfn_ipv6;
  hook_magic_in6.ops.pf = PF_INET6;
  hook_magic_in6.ops.hooknum = SSH_NF_IP6_PRE_ROUTING;
  hook_magic_in6.ops.priority = SSH_NF_IP6_PRI_FIRST;
  nf_register_hook(&hook_magic_in6.ops);

  /* SSH_NF_IP6_POST_ROUTING */
  ssh_interceptor_context->linux_fn.ip6_output_finish = NULL;
  memset(&hook_magic_out6, 0, sizeof(hook_magic_out6));
  hook_magic_out6.ops.hook = ssh_interceptor_hook_magic_hookfn_ipv6;
  hook_magic_out6.ops.pf = PF_INET6;
  hook_magic_out6.ops.hooknum = SSH_NF_IP6_POST_ROUTING;
  hook_magic_out6.ops.priority = SSH_NF_IP6_PRI_FIRST;
  nf_register_hook(&hook_magic_out6.ops);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
  /* NF_ARP_IN */
  ssh_interceptor_context->linux_fn.arp_process = NULL;
  memset(&hook_magic_arp_in, 0, sizeof(hook_magic_arp_in));
  hook_magic_arp_in.ops.hook = ssh_interceptor_hook_magic_hookfn_arp;
  hook_magic_arp_in.ops.pf = SSH_NFPROTO_ARP;
  hook_magic_arp_in.ops.hooknum = NF_ARP_IN;
  hook_magic_arp_in.ops.priority = 1;
  nf_register_hook(&hook_magic_arp_in.ops);
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /* Send probe packets */

  /* Try maximum SSH_LINUX_HOOK_MAGIC_NUM_PROBES times, and give up */
  for (i = 0; i < SSH_LINUX_HOOK_MAGIC_NUM_PROBES; i++)
    {
      ret = 0;

      local_bh_disable();
      ssh_kernel_mutex_lock(ssh_interceptor_context->interceptor_lock);

      if (ssh_interceptor_context->linux_fn.ip_rcv_finish == NULL)
	ret += ssh_interceptor_hook_magic_send(&hook_magic_in4);
      if (ssh_interceptor_context->linux_fn.ip_finish_output == NULL)
	ret += ssh_interceptor_hook_magic_send(&hook_magic_out4);

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      if (ssh_interceptor_context->linux_fn.ip6_rcv_finish == NULL)
	ret += ssh_interceptor_hook_magic_send(&hook_magic_in6);
      if (ssh_interceptor_context->linux_fn.ip6_output_finish == NULL)
	ret += ssh_interceptor_hook_magic_send(&hook_magic_out6);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
      if (ssh_interceptor_context->linux_fn.arp_process == NULL)
	ret += ssh_interceptor_hook_magic_send(&hook_magic_arp_in);
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

      ssh_kernel_mutex_unlock(ssh_interceptor_context->interceptor_lock);
      local_bh_enable();

      if (ret == 0)
	break;

      set_current_state(TASK_UNINTERRUPTIBLE);
      if (i < 5)
	schedule_timeout((i + 5) * HZ / 10);
      else
	schedule_timeout(HZ);
    }

  /* Remove probe hooks */
  ssh_interceptor_hook_magic_uninit();

#ifdef DEBUG_LIGHT
  if (ret)
    {
      SSH_DEBUG(SSH_D_ERROR,
		("ssh_hook_magic_init exits with return value %d", -ret));
    }
  else
    {
      /* Assert that all function pointers have been resolved. */
      SSH_ASSERT(ssh_interceptor_context->linux_fn.ip_rcv_finish != NULL);
      SSH_ASSERT(ssh_interceptor_context->linux_fn.ip_finish_output != NULL);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      SSH_ASSERT(ssh_interceptor_context->linux_fn.ip6_rcv_finish != NULL);
      SSH_ASSERT(ssh_interceptor_context->linux_fn.ip6_output_finish != NULL);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
      SSH_ASSERT(ssh_interceptor_context->linux_fn.arp_process != NULL);
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
    }

#endif /* DEBUG_LIGHT */

  return -ret;
}
