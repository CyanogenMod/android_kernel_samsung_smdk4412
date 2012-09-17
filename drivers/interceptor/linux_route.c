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
 * linux_route.c
 *
 * Linux interceptor implementation of interceptor API routing functions.
 *
 */

#include "linux_internal.h"

/****************** Internal data types and declarations ********************/

typedef struct SshInterceptorRouteResultRec
{
  SshIpAddrStruct gw[1];
  SshInterceptorIfnum ifnum;
  SshUInt16 mtu;
} SshInterceptorRouteResultStruct, *SshInterceptorRouteResult;

/****************** Internal Utility Functions ******************************/

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#if (defined LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING) \
  || (defined LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING)

/* Create a child dst_entry with locked interface MTU, and attach it to `dst'.
   This is needed on newer linux kernels and IP_ONLY_INTERCEPTOR builds,
   where the IP stack fragments packets to path MTU after ssh_interceptor_send.
*/
static struct dst_entry *
interceptor_route_create_child_dst(struct dst_entry *dst, Boolean ipv6)
{
  struct dst_entry *child;
#ifdef LINUX_HAS_DST_COPY_METRICS
  SshUInt32 set;
  struct rt6_info *rt6;
  struct rtable *rt;
#endif /* LINUX_HAS_DST_COPY_METRICS */

  /* Allocate a dst_entry and copy relevant fields from dst. */
  child = SSH_DST_ALLOC(dst);
  if (child == NULL)
    return NULL;

  child->input = dst->input;
  child->output = dst->output;

  /* Child is not added to dst hash, and linux native IPsec is disabled. */
  child->flags |= (DST_NOHASH | DST_NOPOLICY | DST_NOXFRM);

  /* Copy route metrics and lock MTU to interface MTU. */
#ifdef LINUX_HAS_DST_COPY_METRICS
  if (ipv6 == TRUE)
    {
      rt6 = (struct rt6_info *)child;
      memset(&rt6->rt6i_table, 0, sizeof(*rt6) - sizeof(struct dst_entry));
    }
  else
    {
      rt = (struct rtable *)child;
      memset(&SSH_RTABLE_FIRST_MEMBER(rt), 0,
	     sizeof(*rt) - sizeof(struct dst_entry));
    }

  dst_copy_metrics(child, dst);
  set = dst_metric(child, RTAX_LOCK);
  set |= 1 << RTAX_MTU;
  dst_metric_set(child, RTAX_LOCK, set);
#else  /* LINUX_HAS_DST_COPY_METRICS */
  memcpy(child->metrics, dst->metrics, sizeof(child->metrics));
  child->metrics[RTAX_LOCK-1] |= 1 << RTAX_MTU;
#endif /* LINUX_HAS_DST_COPY_METRICS */

#ifdef CONFIG_NET_CLS_ROUTE
  child->tclassid = dst->tclassid;
#endif /* CONFIG_NET_CLS_ROUTE */

#ifdef CONFIG_XFRM
  child->xfrm = NULL;
#endif /* CONFIG_XFRM */

#ifdef LINUX_HAS_HH_CACHE
  if (dst->hh)
    {
      atomic_inc(&dst->hh->hh_refcnt);
      child->hh = dst->hh;
    }
#endif /* LINUX_HAS_HH_CACHE */

#ifdef LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS
  if (dst_get_neighbour(dst) != NULL)
    dst_set_neighbour(child, neigh_clone(dst_get_neighbour(dst)));
#else  /* LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS */
  if (dst->neighbour != NULL)
    child->neighbour = neigh_clone(dst->neighbour);
#endif /* LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS */

  if (dst->dev)
    {
      dev_hold(dst->dev);
      child->dev = dst->dev;
    }

  SSH_ASSERT(dst->child == NULL);
  dst->child = dst_clone(child);

  SSH_DEBUG(SSH_D_MIDOK, ("Allocated child %p dst_entry for dst %p mtu %d",
			  child, dst, dst_mtu(dst)));

  return child;
}

#endif /* LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING
	  || LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

static inline int interceptor_ip4_route_output(struct rtable **rt,
					       struct flowi *fli)
{
  int rval = 0;
#ifdef LINUX_USE_NF_FOR_ROUTE_OUTPUT
  struct dst_entry *dst = NULL;
  const struct nf_afinfo *afinfo = NULL;
  unsigned short family = AF_INET;

  rcu_read_lock();
  afinfo = nf_get_afinfo(family);
  if (!afinfo)
    {
      rcu_read_unlock();
      SSH_DEBUG(SSH_D_FAIL, ("Failed to get nf afinfo"));
      return -1;
    }

  rval = afinfo->route(&init_net, &dst, fli, TRUE);
  rcu_read_unlock();

  *rt = (struct rtable *)dst;

#else /* LINUX_USE_NF_FOR_ROUTE_OUTPUT */

#ifdef LINUX_IP_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT
  rval = ip_route_output_key(&init_net, rt, fli);
#else /* LINUX_IP_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT */
  rval = ip_route_output_key(rt, fli);
#endif /* LINUX_IP_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT */
#endif /* LINUX_USE_NF_FOR_ROUTE_OUTPUT */

  if (rval > 0)
    rval = -rval;

  return rval;
}

static inline struct dst_entry *interceptor_ip6_route_output(struct flowi *fli)
{
  struct dst_entry *dst = NULL;

  /* Perform route lookup */
  /* we do not need a socket, only fake flow */
#ifdef LINUX_USE_NF_FOR_ROUTE_OUTPUT
  const struct nf_afinfo *afinfo = NULL;
  unsigned short family = AF_INET6;
  int rval = 0;

  rcu_read_lock();
  afinfo = nf_get_afinfo(family);
  if (!afinfo)
    {
      rcu_read_unlock();
      SSH_DEBUG(SSH_D_FAIL, ("Failed to get nf afinfo"));
      return NULL;
    }

  rval = afinfo->route(&init_net, &dst, fli, FALSE);
  rcu_read_unlock();
  if (rval != 0)
    {
      SSH_DEBUG(SSH_D_FAIL, ("Failed to get route from IPv6 NF"));
      return NULL;
    }
#else /* LINUX_USE_NF_FOR_ROUTE_OUTPUT */

#ifdef LINUX_IP6_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT
  dst = ip6_route_output(&init_net, NULL, fli);
#else /* LINUX_IP6_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT */
  dst = ip6_route_output(NULL, fli);
#endif /* LINUX_IP6_ROUTE_OUTPUT_KEY_HAS_NET_ARGUMENT */
#endif /* LINUX_USE_NF_FOR_ROUTE_OUTPUT */

  return dst;
}

/****************** Implementation of ssh_interceptor_route *****************/

/* Perform route lookup using linux ip_route_output_key.

   The route lookup will use the following selectors:
   dst, src, outbound ifnum, ip protocol, tos, and fwmark.
   The source address is expected to be local or undefined.

   The following selectors are ignored:
   dst port, src port, icmp type, icmp code, ipsec spi. */
Boolean
ssh_interceptor_route_output_ipv4(SshInterceptor interceptor,
				  SshInterceptorRouteKey key,
				  SshUInt16 selector,
				  SshInterceptorRouteResult result)
{
  u32 daddr;
  struct rtable *rt;
  int rval;
  struct flowi rt_key;
  u16 rt_type;
#ifdef DEBUG_LIGHT
  unsigned char *rt_type_str;
  u32 fwmark = 0;
  unsigned char dst_buf[SSH_IP_ADDR_STRING_SIZE];
  unsigned char src_buf[SSH_IP_ADDR_STRING_SIZE];
#endif /* DEBUG_LIGHT */

  SSH_IP4_ENCODE(&key->dst, (unsigned char *) &daddr);

  /* Initialize rt_key with zero values */
  memset(&rt_key, 0, sizeof(rt_key));

  rt_key.fl4_dst = daddr;
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
    SSH_IP4_ENCODE(&key->src, (unsigned char *) &rt_key.fl4_src);
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM)
    {
      SSH_LINUX_ASSERT_VALID_IFNUM(key->ifnum);
      rt_key.oif = key->ifnum;
    }
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO)
    rt_key.proto = key->ipproto;
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS)
    rt_key.fl4_tos = key->nh.ip4.tos;
  rt_key.fl4_scope = RT_SCOPE_UNIVERSE;

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR

  /* Use linux fw_mark in routing */
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION)
    {
#ifdef LINUX_HAS_SKB_MARK
      rt_key.mark = key->extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR];
#else /* LINUX_HAS_SKB_MARK */
#ifdef CONFIG_IP_ROUTE_FWMARK
      rt_key.fl4_fwmark =
	key->extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR];
#endif /* CONFIG_IP_ROUTE_FWMARK */
#endif /* LINUX_HAS_SKB_MARK */
#ifdef DEBUG_LIGHT
      fwmark = key->extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR];
#endif /* DEBUG_LIGHT */
    }

#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route lookup: "
	     "dst %s src %s ifnum %d ipproto %d tos 0x%02x fwmark 0x%lx",
	     ssh_ipaddr_print(&key->dst, dst_buf, sizeof(dst_buf)),
	     ((selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) ?
	      ssh_ipaddr_print(&key->src, src_buf, sizeof(src_buf)) : NULL),
	     (int) ((selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM)?
		    key->ifnum : -1),
	     (int) ((selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO) ?
		    key->ipproto : -1),
	     ((selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS) ?
	      key->nh.ip4.tos : 0),
	     (unsigned long) ((selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION)?
	      fwmark : 0)));

  /* Perform route lookup */

  rval = interceptor_ip4_route_output(&rt, &rt_key);
  if (rval < 0)
    {
      goto fail;
    }

  /* Get the gateway, mtu and ifnum */

  SSH_IP4_DECODE(result->gw, &rt->rt_gateway);
  result->mtu = SSH_DST_MTU(&SSH_RT_DST(rt));
  result->ifnum = SSH_RT_DST(rt).dev->ifindex;
  rt_type = rt->rt_type;

#ifdef DEBUG_LIGHT
  switch (rt_type)
    {
    case RTN_UNSPEC:
      rt_type_str = "unspec";
      break;
    case RTN_UNICAST:
      rt_type_str = "unicast";
      break;
    case RTN_LOCAL:
      rt_type_str = "local";
      break;
    case RTN_BROADCAST:
      rt_type_str = "broadcast";
      break;
    case RTN_ANYCAST:
      rt_type_str = "anycast";
      break;
    case RTN_MULTICAST:
      rt_type_str = "multicast";
      break;
    case RTN_BLACKHOLE:
      rt_type_str = "blackhole";
      break;
    case RTN_UNREACHABLE:
      rt_type_str = "unreachable";
      break;
    case RTN_PROHIBIT:
      rt_type_str = "prohibit";
      break;
    case RTN_THROW:
      rt_type_str = "throw";
      break;
    case RTN_NAT:
      rt_type_str = "nat";
      break;
    case RTN_XRESOLVE:
      rt_type_str = "xresolve";
      break;
    default:
      rt_type_str = "unknown";
    }
#endif /* DEBUG_LIGHT */

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route result: dst %s via %s ifnum %d[%s] mtu %d type %s [%d]",
	     dst_buf, ssh_ipaddr_print(result->gw, src_buf, sizeof(src_buf)),
	     (int) result->ifnum,
	     (SSH_RT_DST(rt).dev->name ? SSH_RT_DST(rt).dev->name : "none"),
	     result->mtu, rt_type_str, rt_type));

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING
  /* Check if need to create a child dst_entry with interface MTU. */
  if ((selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED)
      && SSH_RT_DST(rt).child == NULL)
    {
      if (interceptor_route_create_child_dst(&SSH_RT_DST(rt), FALSE) == NULL)
	SSH_DEBUG(SSH_D_FAIL, ("Could not create child dst_entry for dst %p",
			       &SSH_RT_DST(rt)));
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /* Release the routing table entry ; otherwise a memory leak occurs
     in the route entry table. */
  ip_rt_put(rt);

  /* Assert that ifnum fits into the SshInterceptorIfnum data type. */
  SSH_LINUX_ASSERT_IFNUM(result->ifnum);

  /* Check that ifnum does not collide with SSH_INTERCEPTOR_INVALID_IFNUM. */
  if (result->ifnum == SSH_INTERCEPTOR_INVALID_IFNUM)
    goto fail;

  /* Accept only unicast, broadcast, anycast,  multicast and local routes */
  if (rt_type == RTN_UNICAST
      || rt_type == RTN_BROADCAST
      || rt_type == RTN_ANYCAST
      || rt_type == RTN_MULTICAST
      || rt_type == RTN_LOCAL)
    {
      SSH_LINUX_ASSERT_VALID_IFNUM(result->ifnum);
      return TRUE;
    }

 fail:
  /* Fail route lookup for other route types */
  SSH_DEBUG(SSH_D_FAIL, ("Route lookup for %s failed with code %d",
			 dst_buf, rval));

  return FALSE;
}


/* Perform route lookup using linux ip_route_input.

   The route lookup will use the following selectors:
   dst, src, inbound ifnum, ip protocol, tos, and fwmark.
   The source address is expected to be non-local and it must be defined.

   The following selectors are ignored:
   dst port, src port, icmp type, icmp code, ipsec spi. */
Boolean
ssh_interceptor_route_input_ipv4(SshInterceptor interceptor,
				 SshInterceptorRouteKey key,
				 SshUInt16 selector,
				 SshInterceptorRouteResult result)
{
  u32 daddr, saddr;
  u8 ipproto;
  u8 tos;
  u32 fwmark;
  struct sk_buff *skbp;
  struct net_device *dev;
  struct rtable *rt;
  int rval = 0;
  u16 rt_type;
  struct iphdr *iph = NULL;
#ifdef DEBUG_LIGHT
  unsigned char *rt_type_str;
  unsigned char dst_buf[SSH_IP_ADDR_STRING_SIZE];
  unsigned char src_buf[SSH_IP_ADDR_STRING_SIZE];
#endif /* DEBUG_LIGHT */

  SSH_IP4_ENCODE(&key->dst, (unsigned char *) &daddr);

  /* Initialize */
  saddr = 0;
  ipproto = 0;
  tos = 0;
  fwmark = 0;
  dev = NULL;

  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
    SSH_IP4_ENCODE(&key->src, (unsigned char *) &saddr);
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_IN_IFNUM)
    {
      SSH_LINUX_ASSERT_VALID_IFNUM(key->ifnum);
      dev = ssh_interceptor_ifnum_to_netdev(interceptor, key->ifnum);
    }

  if (dev == NULL)
    return FALSE;

  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO)
    ipproto = key->ipproto;
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS)
    tos = key->nh.ip4.tos;

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
  /* Use linux fw_mark in routing */
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION)
    fwmark = key->extension[SSH_LINUX_FWMARK_EXTENSION_SELECTOR];
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

  /* Build dummy skb */
  skbp = alloc_skb(SSH_IPH4_HDRLEN, GFP_ATOMIC);
  if (skbp == NULL)
    goto fail;

  SSH_SKB_RESET_MACHDR(skbp);
  iph = (struct iphdr *) skb_put(skbp, SSH_IPH4_HDRLEN);
  if (iph == NULL)
    {
      dev_kfree_skb(skbp);
      goto fail;
    }
  SSH_SKB_SET_NETHDR(skbp, (unsigned char *) iph);

  SSH_SKB_DST_SET(skbp, NULL);
  skbp->protocol = __constant_htons(ETH_P_IP);
  SSH_SKB_MARK(skbp) = fwmark;
  iph->protocol = ipproto;

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route lookup: "
	     "dst %s src %s ifnum %d[%s] ipproto %d tos 0x%02x fwmark 0x%x",
	     ssh_ipaddr_print(&key->dst, dst_buf, sizeof(dst_buf)),
	     ((selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) ?
	      ssh_ipaddr_print(&key->src, src_buf, sizeof(src_buf)) : NULL),
	     dev->ifindex, dev->name,
	     ipproto, tos, fwmark));

  /* Perform route lookup */

  rval = ip_route_input(skbp, daddr, saddr, tos, dev);
  if (rval < 0 || SSH_SKB_DST(skbp) == NULL)
    {
      dev_kfree_skb(skbp);
      goto fail;
    }

  /* Get the gateway, mtu and ifnum */
  rt = (struct rtable *) SSH_SKB_DST(skbp);
  SSH_IP4_DECODE(result->gw, &rt->rt_gateway);
  result->mtu = SSH_DST_MTU(SSH_SKB_DST(skbp));
  result->ifnum = SSH_SKB_DST(skbp)->dev->ifindex;
  rt_type = rt->rt_type;

#ifdef DEBUG_LIGHT
  switch (rt_type)
    {
    case RTN_UNSPEC:
      rt_type_str = "unspec";
      break;
    case RTN_UNICAST:
      rt_type_str = "unicast";
      break;
    case RTN_LOCAL:
      rt_type_str = "local";
      break;
    case RTN_BROADCAST:
      rt_type_str = "broadcast";
      break;
    case RTN_ANYCAST:
      rt_type_str = "anycast";
      break;
    case RTN_MULTICAST:
      rt_type_str = "multicast";
      break;
    case RTN_BLACKHOLE:
      rt_type_str = "blackhole";
      break;
    case RTN_UNREACHABLE:
      rt_type_str = "unreachable";
      break;
    case RTN_PROHIBIT:
      rt_type_str = "prohibit";
      break;
    case RTN_THROW:
      rt_type_str = "throw";
      break;
    case RTN_NAT:
      rt_type_str = "nat";
      break;
    case RTN_XRESOLVE:
      rt_type_str = "xresolve";
      break;
    default:
      rt_type_str = "unknown";
    }
#endif /* DEBUG_LIGHT */

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route result: dst %s via %s ifnum %d[%s] mtu %d type %s [%d]",
	     dst_buf, ssh_ipaddr_print(result->gw, src_buf, sizeof(src_buf)),
	     (int) result->ifnum,
	     (SSH_RT_DST(rt).dev->name ? SSH_RT_DST(rt).dev->name : "none"),
	     result->mtu, rt_type_str, rt_type));

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING
  /* Check if need to create a child dst_entry with interface MTU. */
  if ((selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED)
      && SSH_SKB_DST(skbp)->child == NULL)
    {
      if (interceptor_route_create_child_dst(SSH_SKB_DST(skbp), FALSE) == NULL)
	SSH_DEBUG(SSH_D_FAIL, ("Could not create child dst_entry for dst %p",
			       SSH_SKB_DST(skbp)));
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /* Release the routing table entry ; otherwise a memory leak occurs
     in the route entry table. */
  dst_release(SSH_SKB_DST(skbp));
  SSH_SKB_DST_SET(skbp, NULL);
  dev_kfree_skb(skbp);

  /* Assert that ifnum fits into the SshInterceptorIfnum data type. */
  SSH_LINUX_ASSERT_IFNUM(result->ifnum);

  /* Check that ifnum does not collide with SSH_INTERCEPTOR_INVALID_IFNUM. */
  if (result->ifnum == SSH_INTERCEPTOR_INVALID_IFNUM)
    goto fail;

  /* Accept only unicast, broadcast, anycast, multicast and local routes. */
  if (rt_type == RTN_UNICAST
      || rt_type == RTN_BROADCAST
      || rt_type == RTN_ANYCAST
      || rt_type == RTN_MULTICAST
      || rt_type == RTN_LOCAL)
    {
      ssh_interceptor_release_netdev(dev);
      SSH_LINUX_ASSERT_VALID_IFNUM(result->ifnum);
      return TRUE;
    }

  /* Fail route lookup for other route types. */
 fail:
  ssh_interceptor_release_netdev(dev);

  SSH_DEBUG(SSH_D_FAIL,
	    ("Route lookup for %s failed with code %d",
	     ssh_ipaddr_print(&key->dst, dst_buf, sizeof(dst_buf)), rval));

  return FALSE;
}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6

/* Perform route lookup using linux ip6_route_output.

   The route lookup will use the following selectors:
   dst, src, outbound ifnum.

   The following selectors are ignored:
   ipv6 priority, flowlabel, ip protocol, dst port, src port,
   icmp type, icmp code, ipsec spi, and fwmark. */
Boolean
ssh_interceptor_route_output_ipv6(SshInterceptor interceptor,
				  SshInterceptorRouteKey key,
				  SshUInt16 selector,
				  SshInterceptorRouteResult result)
{
  struct flowi rt_key;
  struct dst_entry *dst;
  struct rt6_info *rt;
  u32 rt6i_flags;
  int error = 0;
  struct neighbour *neigh;
#ifdef DEBUG_LIGHT
  unsigned char dst_buf[SSH_IP_ADDR_STRING_SIZE];
  unsigned char src_buf[SSH_IP_ADDR_STRING_SIZE];
#endif /* DEBUG_LIGHT */

  memset(&rt_key, 0, sizeof(rt_key));

  SSH_IP6_ENCODE(&key->dst, rt_key.fl6_dst.s6_addr);

  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
      SSH_IP6_ENCODE(&key->src, rt_key.fl6_src.s6_addr);

  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM)
    {
      SSH_LINUX_ASSERT_VALID_IFNUM(key->ifnum);
      rt_key.oif = key->ifnum;
    }

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route lookup: dst %s src %s ifnum %d",
	     ssh_ipaddr_print(&key->dst, dst_buf, sizeof(dst_buf)),
	     ((selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) ?
	      ssh_ipaddr_print(&key->src, src_buf, sizeof(src_buf)) : NULL),
	     (int) ((selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM) ?
		    key->ifnum : -1)));

  dst = interceptor_ip6_route_output(&rt_key);
  if (dst == NULL)
    {
      goto fail;
    }
  else if (dst->error != 0)
    {
      error = dst->error;
      /* Release dst_entry */
      dst_release(dst);
      goto fail;
    }

  rt = (struct rt6_info *) dst;

  /* Get the gateway, mtu and ifnum */

  /* For an example of retrieving routing information for IPv6
     within Linux kernel (2.4.19) see inet6_rtm_getroute()
     in /usr/src/linux/net/ipv6/route.c */
#ifdef LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS
  neigh = dst_get_neighbour(&rt->dst);
#else  /* LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS */
  neigh = rt->rt6i_nexthop;
#endif /* LINUX_HAS_DST_NEIGHBOUR_FUNCTIONS */

  if (neigh != NULL)
    SSH_IP6_DECODE(result->gw, &neigh->primary_key);
  else
      SSH_IP6_DECODE(result->gw, &rt_key.fl6_dst.s6_addr);

  result->mtu = SSH_DST_MTU(&SSH_RT_DST(rt));

  /* The interface number might not be ok, but that is a problem
     for the recipient of the routing information. */
  result->ifnum = dst->dev->ifindex;

  rt6i_flags = rt->rt6i_flags;

  SSH_DEBUG(SSH_D_LOWOK,
	    ("Route result: %s via %s ifnum %d[%s] mtu %d flags 0x%08x[%s%s]",
	     dst_buf, ssh_ipaddr_print(result->gw, src_buf, sizeof(src_buf)),
	     (int) result->ifnum, (dst->dev ? dst->dev->name : "none"),
	     result->mtu, rt6i_flags, ((rt6i_flags & RTF_UP) ? "up " : ""),
	     ((rt6i_flags & RTF_REJECT) ? "reject" : "")));


#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING
  /* Check if need to create a child dst_entry with interface MTU. */
  if (selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED)
    {
      if (dst->child == NULL)
	if (interceptor_route_create_child_dst(dst, TRUE) == NULL)
	  SSH_DEBUG(SSH_D_FAIL, ("Could not create child dst_entry for dst %p",
				 dst));
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  /* Release dst_entry */
  dst_release(dst);

  /* Assert that ifnum fits into the SshInterceptorIfnum data type. */
  SSH_LINUX_ASSERT_IFNUM(result->ifnum);

  /* Check that ifnum does not collide with SSH_INTERCEPTOR_INVALID_IFNUM. */
  if (result->ifnum == SSH_INTERCEPTOR_INVALID_IFNUM)
    goto fail;

  /* Accept only valid routes */
  if ((rt6i_flags & RTF_UP)
      && (rt6i_flags & RTF_REJECT) == 0)
    {
      SSH_LINUX_ASSERT_VALID_IFNUM(result->ifnum);
      return TRUE;
    }

  /* Fail route lookup for reject and unknown routes */
 fail:
  SSH_DEBUG(SSH_D_FAIL, ("Route lookup for %s failed with code %d",
			 dst_buf, error));

  return FALSE;
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

/* Performs a route lookup for the given destination address.
   This also always calls a callback function. */

void
ssh_interceptor_route(SshInterceptor interceptor,
                      SshInterceptorRouteKey key,
                      SshInterceptorRouteCompletion callback,
                      void *cb_context)
{
  SshInterceptorRouteResultStruct result;

  /* It is a fatal error to call ssh_interceptor_route with
     a routing key that does not specify the destination address. */
  SSH_ASSERT(SSH_IP_DEFINED(&key->dst));

  if (SSH_IP_IS4(&key->dst))
    {
      /* Key specifies non-local src address and inbound ifnum
	 -> use ssh_interceptor_route_input_ipv4 */
      if ((key->selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
	  && (key->selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_LOCAL_SRC) == 0
	  && (key->selector & SSH_INTERCEPTOR_ROUTE_KEY_IN_IFNUM))
	{
	  /* Assert that all mandatory selectors are present. */
	  SSH_ASSERT(key->selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC);
	  SSH_ASSERT(SSH_IP_IS4(&key->src));
	  SSH_ASSERT(key->selector & SSH_INTERCEPTOR_ROUTE_KEY_IN_IFNUM);

	  if (!ssh_interceptor_route_input_ipv4(interceptor, key,
						key->selector, &result))
	    goto fail;
	}

      /* Otherwise use ssh_interceptor_route_output_ipv4 */
      else
	{
	  SshUInt16 selector = key->selector;

	  /* Assert that all mandatory selectors are present. */
	  SSH_ASSERT((key->selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) == 0
		     || SSH_IP_IS4(&key->src));

	  /* Key specifies non-local src address.
	     Linux ip_route_output_key will fail such route lookups,
	     so we must clear the src address selector and do the
	     route lookup as if src was one of local addresses.
	     For example, locally generated TCP RST packets are
	     such packets. */
	  if ((key->selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_LOCAL_SRC) == 0)
	    selector &= ~SSH_INTERCEPTOR_ROUTE_KEY_SRC;

	  if (!ssh_interceptor_route_output_ipv4(interceptor, key, selector,
						 &result))
	    goto fail;
	}

      (*callback)(TRUE, result.gw, result.ifnum, result.mtu, cb_context);
      return;
    }

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  if (SSH_IP_IS6(&key->dst))
    {
      /* Assert that all mandatory selectors are present. */
      SSH_ASSERT((key->selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) == 0
		 || SSH_IP_IS6(&key->src));

      /* Always use ip6_route_output for IPv6 */
      if (!ssh_interceptor_route_output_ipv6(interceptor, key, key->selector,
					     &result))
	goto fail;

      (*callback)(TRUE, result.gw, result.ifnum, result.mtu, cb_context);
      return;
    }
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  /* Fallback to error */
 fail:
  SSH_DEBUG(SSH_D_FAIL, ("Route lookup failed, unknown dst address type"));
  (*callback) (FALSE, NULL, 0, 0, cb_context);
}


/**************************** Rerouting of Packets **************************/


/* Route IPv4 packet 'skbp', using the route key selectors in
   'route_selector' and the interface number 'ifnum_in'. */
Boolean
ssh_interceptor_reroute_skb_ipv4(SshInterceptor interceptor,
				 struct sk_buff *skbp,
				 SshUInt16 route_selector,
				 SshUInt32 ifnum_in)
{
  struct iphdr *iph;
  int rval = 0;

  /* Recalculate the route info as the engine might have touched the
     destination address. This can happen for example if we are in
     tunnel mode. */

  iph = (struct iphdr *) SSH_SKB_GET_NETHDR(skbp);
  if (iph == NULL)
    {
      SSH_DEBUG(SSH_D_ERROR, ("Could not access IP header"));
      return FALSE;
    }

  /* Release old dst_entry */
  if (SSH_SKB_DST(skbp))
    dst_release(SSH_SKB_DST(skbp));

  SSH_SKB_DST_SET(skbp, NULL);

  if ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
      && (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_LOCAL_SRC) == 0
      && (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IN_IFNUM))
    {
      u32 saddr = 0;
      u8 ipproto = 0;
      u8 tos = 0;
#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
      u32 fwmark = 0;
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */
      struct net_device *dev;

      SSH_ASSERT(skbp->protocol == __constant_htons(ETH_P_IP));

      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
	saddr = iph->saddr;

      /* Map 'ifnum_in' to a net_device. */
      SSH_LINUX_ASSERT_VALID_IFNUM(ifnum_in);
      dev = ssh_interceptor_ifnum_to_netdev(interceptor, ifnum_in);
      if (dev == NULL)
        return FALSE;

      /* Clear the IP protocol, if selector does not define it. */
      if ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO) == 0)
	{
	  ipproto = iph->protocol;
	  iph->protocol = 0;
	}

      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS)
	tos = RT_TOS(iph->tos);

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
      /* Clear the nfmark, if selector does not define it. */
      if ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION) == 0)
	{
	  fwmark = SSH_SKB_MARK(skbp);
	  SSH_SKB_MARK(skbp) = 0;
	}
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

      /* Call ip_route_input */
      if (ip_route_input(skbp, iph->daddr, saddr, tos, dev) < 0)
	{
	  SSH_DEBUG(SSH_D_FAIL,
		    ("ip_route_input failed. (0x%08x -> 0x%08x)",
		     iph->saddr, iph->daddr));

	  SSH_DEBUG(SSH_D_NICETOKNOW,
		    ("dst 0x%08x src 0x%08x iif %d[%s] proto %d tos 0x%02x "
		     "fwmark 0x%x",
		     iph->daddr, saddr, (dev ? dev->ifindex : -1),
		     (dev ? dev->name : "none"), iph->protocol, tos,
		     SSH_SKB_MARK(skbp)));

	  /* Release netdev reference */
          ssh_interceptor_release_netdev(dev);

	  return FALSE;
	}

      /* Write original IP protocol back to skb */
      if (ipproto)
	iph->protocol = ipproto;

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
      /* Write original fwmark back to skb */
      if (fwmark)
	SSH_SKB_MARK(skbp) = fwmark;
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

      /* Release netdev reference */
      ssh_interceptor_release_netdev(dev);
    }

  else
    {
      struct rtable *rt;
      struct flowi rt_key;

      if ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_LOCAL_SRC) == 0)
	route_selector &= ~SSH_INTERCEPTOR_ROUTE_KEY_SRC;

      memset(&rt_key, 0, sizeof(rt_key));

      rt_key.fl4_dst = iph->daddr;
      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
	rt_key.fl4_src = iph->saddr;
      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM)
	rt_key.oif = (skbp->dev ? skbp->dev->ifindex : 0);
      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO)
	rt_key.proto = iph->protocol;
      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS)
	rt_key.fl4_tos = RT_TOS(iph->tos);
      rt_key.fl4_scope = RT_SCOPE_UNIVERSE;

#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
      if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION)
	{
#ifdef LINUX_HAS_SKB_MARK
	  rt_key.mark = SSH_SKB_MARK(skbp);
#else /* LINUX_HAS_SKB_MARK */
#ifdef CONFIG_IP_ROUTE_FWMARK
	  rt_key.fl4_fwmark = SSH_SKB_MARK(skbp);
#endif /* CONFIG_IP_ROUTE_FWMARK */
#endif /* LINUX_HAS_SKB_MARK */
	}
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */

      rval = interceptor_ip4_route_output(&rt, &rt_key);
      if (rval < 0)
	{
	  SSH_DEBUG(SSH_D_FAIL,
		    ("ip_route_output_key failed (0x%08x -> 0x%08x): %d",
		     iph->saddr, iph->daddr, rval));

	  SSH_DEBUG(SSH_D_NICETOKNOW,
		    ("dst 0x%08x src 0x%08x oif %d[%s] proto %d tos 0x%02x"
		     "fwmark 0x%x",
		     iph->daddr,
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC) ?
		      iph->saddr : 0),
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM) ?
		      rt_key.oif : -1),
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM) ?
		      (skbp->dev ? skbp->dev->name : "none") : "none"),
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IPPROTO) ?
		      iph->protocol : -1),
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_IP4_TOS) ?
		      iph->tos : 0),
		     ((route_selector & SSH_INTERCEPTOR_ROUTE_KEY_EXTENSION) ?
		      SSH_SKB_MARK(skbp) : 0)));

	  return FALSE;
	}

      /* Make a new dst because we just rechecked the route. */
      SSH_SKB_DST_SET(skbp, dst_clone(&SSH_RT_DST(rt)));

      /* Release the routing table entry ; otherwise a memory leak occurs
	 in the route entry table. */
      ip_rt_put(rt);
    }

  SSH_ASSERT(SSH_SKB_DST(skbp) != NULL);

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING
  if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED)
    {
      /* Check if need to create a child dst_entry with interface MTU. */
      if (SSH_SKB_DST(skbp)->child == NULL)
	{
          if (interceptor_route_create_child_dst(SSH_SKB_DST(skbp), FALSE)
	      == NULL)
	    {
	      SSH_DEBUG(SSH_D_ERROR,
			("Could not create child dst_entry for dst %p",
			 SSH_SKB_DST(skbp)));
	      return FALSE;
	    }
	}

      /* Pop dst stack and use the child entry with interface MTU
	 for sending the packet. */
#ifdef LINUX_DST_POP_IS_SKB_DST_POP
      SSH_SKB_DST_SET(skbp, dst_clone(skb_dst_pop(skbp)));
#else /* LINUX_DST_POP_IS_SKB_DST_POP */
      SSH_SKB_DST_SET(skbp, dst_pop(SSH_SKB_DST(skbp)));
#endif /*LINUX_DST_POP_IS_SKB_DST_POP */
    }
#endif /* LINUX_FRAGMENTATION_AFTER_NF_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

  return TRUE;
}


#ifdef SSH_LINUX_INTERCEPTOR_IPV6

/* Route IPv6 packet 'skbp', using the route key selectors in
   'route_selector' and the interface number 'ifnum_in'. */
Boolean
ssh_interceptor_reroute_skb_ipv6(SshInterceptor interceptor,
				 struct sk_buff *skbp,
				 SshUInt16 route_selector,
				 SshUInt32 ifnum_in)
{
  /* we do not need a socket, only fake flow */
  struct flowi rt_key;
  struct dst_entry *dst;
  struct ipv6hdr *iph6;

  iph6 = (struct ipv6hdr *) SSH_SKB_GET_NETHDR(skbp);
  if (iph6 == NULL)
    {
      SSH_DEBUG(SSH_D_ERROR, ("Could not access IPv6 header"));
      return FALSE;
    }

  memset(&rt_key, 0, sizeof(rt_key));

  rt_key.fl6_dst = iph6->daddr;

  if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_SRC)
    rt_key.fl6_src = iph6->saddr;

  if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_OUT_IFNUM)
    {
      rt_key.oif = (skbp->dev ? skbp->dev->ifindex : 0);
      SSH_LINUX_ASSERT_IFNUM(rt_key.oif);
    }

  dst = interceptor_ip6_route_output(&rt_key);
  if (dst == NULL || dst->error != 0)
    {
      if (dst != NULL)
        dst_release(dst);

      SSH_DEBUG(SSH_D_FAIL,
		("ip6_route_output failed."));

      SSH_DEBUG_HEXDUMP(SSH_D_NICETOKNOW,
			("dst "),
			(unsigned char *) &iph6->daddr, sizeof(iph6->daddr));
      SSH_DEBUG_HEXDUMP(SSH_D_NICETOKNOW,
			("src "),
			(unsigned char *) &iph6->saddr, sizeof(iph6->saddr));
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("oif %d[%s]",
		 (skbp->dev ? skbp->dev->ifindex : -1),
		 (skbp->dev ? skbp->dev->name : "none")));
      return FALSE;
    }
  if (SSH_SKB_DST(skbp))
    dst_release(SSH_SKB_DST(skbp));

#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#ifdef LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING
  if (route_selector & SSH_INTERCEPTOR_ROUTE_KEY_FLAG_TRANSFORM_APPLIED)
    {
      SSH_DEBUG(SSH_D_LOWOK,
		("Creating a new child entry for dst %p, child %p %lu",
		 dst, dst->child, skbp->_skb_refdst));

      /* Check if need to create a child dst_entry with interface MTU. */
      if (dst->child == NULL)
	{
          if (interceptor_route_create_child_dst(dst, TRUE) == NULL)
	    {
	      SSH_DEBUG(SSH_D_ERROR,
			("Could not create child dst_entry for dst %p",
			 dst));

	      dst_release(dst);
	      return FALSE;
	    }
	}

      SSH_SKB_DST_SET(skbp, dst_clone(dst->child));
      dst_release(dst);
    }
  else
#endif /* LINUX_FRAGMENTATION_AFTER_NF6_POST_ROUTING */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */
    {
      /* No need to clone the dst as ip6_route_output has already taken
	 one for us. */
      SSH_SKB_DST_SET(skbp, dst);
    }

  return TRUE;
}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
