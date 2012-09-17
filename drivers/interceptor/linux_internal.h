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
 * linux_internal.h
 *
 * Internal header file for the linux interceptor.
 *
 */

#ifndef LINUX_INTERNAL_H
#define LINUX_INTERNAL_H

#include "sshincludes.h"

/* Parameters used to tune the interceptor. */
#include "linux_versions.h"
#include "linux_params.h"

#include <linux/types.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/pkt_sched.h>

#include <linux/interrupt.h>
#include <linux/inetdevice.h>

#include <net/ip.h>
#include <net/inet_common.h>

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
#include <net/ipv6.h>
#include <net/addrconf.h>
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
#include <linux/if_arp.h>
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_bridge.h>
#include <linux/netfilter_arp.h>

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cpumask.h>
#include <linux/rcupdate.h>

#ifdef LINUX_NEED_IF_ADDR_H
#include <linux/if_addr.h>
#endif /* LINUX_NEED_IF_ADDR_H */

#include <net/ip.h>
#include <net/route.h>
#include <net/inet_common.h>

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
#include <net/ipv6.h>
#include <net/addrconf.h>
#include <net/ip6_fib.h>
#include <net/ip6_route.h>
#include <net/flow.h>
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#include <linux/threads.h>

#include "engine.h"
#include "kernel_includes.h"
#include "kernel_mutex.h"
#include "interceptor.h"
#include "sshinet.h"
#include "sshdebug.h"

#include "linux_packet_internal.h"
#include "linux_mutex_internal.h"
#include "linux_virtual_adapter_internal.h"

/****************************** Sanity checks ********************************/

#ifndef MODULE
#error "VPNClient can only be compiled as a MODULE"
#endif /* MODULE */

#ifndef CONFIG_NETFILTER
#error "Kernel is not compiled with CONFIG_NETFILTER"
#endif /* CONFIG_NETFILTER */

/* Check that SSH_LINUX_FWMARK_EXTENSION_SELECTOR is in range. */
#ifdef SSH_LINUX_FWMARK_EXTENSION_SELECTOR
#if (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0)
#if (SSH_LINUX_FWMARK_EXTENSION_SELECTOR >= \
     SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS)
#error "Invalid value specified for SSH_LINUX_FWMARK_EXTENSION_SELECTOR"
#endif
#endif /* (SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS > 0) */
#endif /* SSH_LINUX_FWMARK_EXTENSION_SELECTOR */


/****************************** Internal defines *****************************/

#define SSH_LINUX_INTERCEPTOR_NR_CPUS NR_CPUS

/* Flags for ssh_engine_start */
#define SSH_LINUX_ENGINE_FLAGS 0

#define SSH_LINUX_INTERCEPTOR_MODULE_DESCRIPTION "VPNClient"

/********************** Kernel version specific wrapper macros ***************/

#ifdef LINUX_HAS_DEV_GET_FLAGS
#define SSH_LINUX_DEV_GET_FLAGS(dev) dev_get_flags(dev)
#else /* LINUX_HAS_DEV_GET_FLAGS */
#define SSH_LINUX_DEV_GET_FLAGS(dev) ((dev)->flags)
#endif /* LINUX_HAS_DEV_GET_FLAGS */

#ifdef LINUX_NF_HOOK_SKB_IS_POINTER
typedef struct sk_buff SshHookSkb;
#define SSH_HOOK_SKB_PTR(_skb) _skb
#else  /* LINUX_NF_HOOK_SKB_IS_POINTER */
typedef struct sk_buff *SshHookSkb;
#define SSH_HOOK_SKB_PTR(_skb) *_skb
#endif /* LINUX_NF_HOOK_SKB_IS_POINTER */

#ifdef LINUX_HAS_SKB_MARK
#define SSH_SKB_MARK(__skb) ((__skb)->mark)
#else /* LINUX_HAS_SKB_MARK */
#define SSH_SKB_MARK(__skb) ((__skb)->nfmark)
#endif /* LINUX_HAS_SKB_MARK */

#ifdef LINUX_HAS_DST_MTU
#define SSH_DST_MTU(__dst) (dst_mtu((__dst)))
#else /* LINUX_HAS_DST_MTU */
#define SSH_DST_MTU(__dst) (dst_pmtu((__dst)))
#endif /* LINUX_HAS_DST_MTU */

/* Before 2.6.22 kernels, the net devices were accessed
   using directly global variables.
   2.6.22 -> 2.6.23 introduced new functions accessing
   the net device list.
   2.6.24 -> these new functions started taking new
   arguments. */
#ifndef LINUX_HAS_NETDEVICE_ACCESSORS
/* For 2.4.x -> 2.6.21 kernels */
#define SSH_FIRST_NET_DEVICE()     dev_base
#define SSH_NEXT_NET_DEVICE(_dev)  _dev->next

#else /* LINUX_HAS_NETDEVICE_ACCESSORS */
#ifndef LINUX_NET_DEVICE_HAS_ARGUMENT
/* For 2.6.22 -> 2.6.23 kernels */
#define SSH_FIRST_NET_DEVICE()     first_net_device()
#define SSH_NEXT_NET_DEVICE(_dev)  next_net_device(_dev)

#else /* LINUX_NET_DEVICE_HAS_ARGUMENT */
/* For 2.6.24 -> kernels */
#define SSH_FIRST_NET_DEVICE()     first_net_device(&init_net)
#define SSH_NEXT_NET_DEVICE(_dev)  next_net_device(_dev)

#endif /* LINUX_NET_DEVICE_HAS_ARGUMENT */
#endif /* LINUX_HAS_NETDEVICE_ACCESSORS */

/* This HAVE_NET_DEVICE_OPS was removed in 3.1.x */
#ifdef LINUX_HAS_NET_DEVICE_OPS
#ifndef HAVE_NET_DEVICE_OPS
#define HAVE_NET_DEVICE_OPS 1
#endif /* HAVE_NET_DEVICE_OPS */
#endif /* LINUX_HAS_NET_DEVICE_OPS */

#ifdef LINUX_NET_DEVICE_HAS_ARGUMENT
#define SSH_DEV_GET_BY_INDEX(_i) dev_get_by_index(&init_net, (_i))
#else /* LINUX_NET_DEVICE_HAS_ARGUMENT */
#define SSH_DEV_GET_BY_INDEX(_i) dev_get_by_index((_i))
#endif /* LINUX_NET_DEVICE_HAS_ARGUMENT */

#ifdef LINUX_HAS_SKB_DATA_ACCESSORS
/* On new kernel versions the skb->end, skb->tail, skb->network_header,
   skb->mac_header, and skb->transport_header are either pointers to
   skb->data (on 32bit platforms) or offsets from skb->data
   (on 64bit platforms). */

#define SSH_SKB_GET_END(__skb) (skb_end_pointer((__skb)))

#define SSH_SKB_GET_TAIL(__skb) (skb_tail_pointer((__skb)))
#define SSH_SKB_SET_TAIL(__skb, __ptr) \
   (skb_set_tail_pointer((__skb), (__ptr) - (__skb)->data))
#define SSH_SKB_RESET_TAIL(__skb) (skb_reset_tail_pointer((__skb)))

#define SSH_SKB_GET_NETHDR(__skb) (skb_network_header((__skb)))
#define SSH_SKB_SET_NETHDR(__skb, __ptr) \
   (skb_set_network_header((__skb), (__ptr) - (__skb)->data))
#define SSH_SKB_RESET_NETHDR(__skb) (skb_reset_network_header((__skb)))

#define SSH_SKB_GET_MACHDR(__skb) (skb_mac_header((__skb)))
#define SSH_SKB_SET_MACHDR(__skb, __ptr) \
   (skb_set_mac_header((__skb), (__ptr) - (__skb)->data))
#define SSH_SKB_RESET_MACHDR(__skb) (skb_reset_mac_header((__skb)))

#define SSH_SKB_GET_TRHDR(__skb) (skb_transport_header((__skb)))
#define SSH_SKB_SET_TRHDR(__skb, __ptr) \
   (skb_set_transport_header((__skb), (__ptr) - (__skb)->data))
#define SSH_SKB_RESET_TRHDR(__skb) (skb_reset_transport_header((__skb)))

#else  /* LINUX_HAS_SKB_DATA_ACCESSORS */

#define SSH_SKB_GET_END(__skb) ((__skb)->end)

#define SSH_SKB_GET_TAIL(__skb) ((__skb)->tail)
#define SSH_SKB_SET_TAIL(__skb, __ptr) ((__skb)->tail = (__ptr))
#define SSH_SKB_RESET_TAIL(__skb) ((__skb)->tail = NULL)

#define SSH_SKB_GET_NETHDR(__skb) ((__skb)->nh.raw)
#define SSH_SKB_SET_NETHDR(__skb, __ptr) ((__skb)->nh.raw = (__ptr))
#define SSH_SKB_RESET_NETHDR(__skb) ((__skb)->nh.raw = NULL)

#define SSH_SKB_GET_MACHDR(__skb) ((__skb)->mac.raw)
#define SSH_SKB_SET_MACHDR(__skb, __ptr) ((__skb)->mac.raw = (__ptr))
#define SSH_SKB_RESET_MACHDR(__skb) ((__skb)->mac.raw = NULL)

#define SSH_SKB_GET_TRHDR(__skb) ((__skb)->h.raw)
#define SSH_SKB_SET_TRHDR(__skb, __ptr) ((__skb)->h.raw = (__ptr))
#define SSH_SKB_RESET_TRHDR(__skb) ((__skb)->h.raw = NULL)

#endif /* LINUX_HAS_SKB_DATA_ACCESSORS */

#ifdef LINUX_HAS_SKB_CSUM_OFFSET
/* On linux-2.6.20 and later skb->csum is split into
   a union of csum and csum_offset. */
#define SSH_SKB_CSUM_OFFSET(__skb) ((__skb)->csum_offset)
#define SSH_SKB_CSUM(__skb) ((__skb)->csum)
#else /* LINUX_HAS_SKB_CSUM_OFFSET */
#define SSH_SKB_CSUM_OFFSET(__skb) ((__skb)->csum)
#define SSH_SKB_CSUM(__skb) ((__skb)->csum)
#endif /* LINUX_HAS_SKB_CSUM_OFFSET */

#ifdef LINUX_NF_INET_HOOKNUMS

#define SSH_NF_IP_PRE_ROUTING NF_INET_PRE_ROUTING
#define SSH_NF_IP_LOCAL_IN NF_INET_LOCAL_IN
#define SSH_NF_IP_FORWARD NF_INET_FORWARD
#define SSH_NF_IP_LOCAL_OUT NF_INET_LOCAL_OUT
#define SSH_NF_IP_POST_ROUTING NF_INET_POST_ROUTING
#define SSH_NF_IP_PRI_FIRST INT_MIN

#define SSH_NF_IP6_PRE_ROUTING NF_INET_PRE_ROUTING
#define SSH_NF_IP6_LOCAL_IN NF_INET_LOCAL_IN
#define SSH_NF_IP6_FORWARD NF_INET_FORWARD
#define SSH_NF_IP6_LOCAL_OUT NF_INET_LOCAL_OUT
#define SSH_NF_IP6_POST_ROUTING NF_INET_POST_ROUTING
#define SSH_NF_IP6_PRI_FIRST INT_MIN

#else /* LINUX_UNIFIED_NETFILTER_IP_HOOKNUMS */

#define SSH_NF_IP_PRE_ROUTING NF_IP_PRE_ROUTING
#define SSH_NF_IP_LOCAL_IN NF_IP_LOCAL_IN
#define SSH_NF_IP_FORWARD NF_IP_FORWARD
#define SSH_NF_IP_LOCAL_OUT NF_IP_LOCAL_OUT
#define SSH_NF_IP_POST_ROUTING NF_IP_POST_ROUTING
#define SSH_NF_IP_PRI_FIRST NF_IP_PRI_FIRST

#define SSH_NF_IP6_PRE_ROUTING NF_IP6_PRE_ROUTING
#define SSH_NF_IP6_LOCAL_IN NF_IP6_LOCAL_IN
#define SSH_NF_IP6_FORWARD NF_IP6_FORWARD
#define SSH_NF_IP6_LOCAL_OUT NF_IP6_LOCAL_OUT
#define SSH_NF_IP6_POST_ROUTING NF_IP6_POST_ROUTING
#define SSH_NF_IP6_PRI_FIRST NF_IP6_PRI_FIRST

#endif /* LINUX_NF_INET_HOOKNUMS */

#ifdef LINUX_HAS_NFPROTO_ARP
#define SSH_NFPROTO_ARP NFPROTO_ARP
#else /* LINUX_HAS_NFPROTO_ARP */
#define SSH_NFPROTO_ARP NF_ARP
#endif /* LINUX_HAS_NFPROTO_ARP */

/*
  Since 2.6.31 there is now skb->dst pointer and
  functions skb_dst() and skb_dst_set() should be used.

  The code is modified to use the functions. For older
  version corresponding macros are defined.
 */
#ifdef LINUX_HAS_SKB_DST_FUNCTIONS
#define SSH_SKB_DST(__skb) skb_dst((__skb))
#define SSH_SKB_DST_SET(__skb, __dst) skb_dst_set((__skb), (__dst))
#else /* LINUX_HAS_SKB_DST_FUNCTIONS */
#define SSH_SKB_DST(__skb) ((__skb)->dst)
#define SSH_SKB_DST_SET(__skb, __dst) ((void)((__skb)->dst = (__dst)))
#endif /* LINUX_HAS_SKB_DST_FUNCTIONS */

#ifdef IP6CB
#define SSH_LINUX_IP6CB(skbp) IP6CB(skbp)
#else /* IP6CB */
#define SSH_LINUX_IP6CB(skbp) ((struct inet6_skb_parm *) ((skbp)->cb))
#endif /* IP6CB */

/* Stating from linux 2.6.35 the IPv6 address list needs to be iterated
   using the list_for_each_* macros. */
#ifdef LINUX_RT_DST_IS_NOT_IN_UNION
#define SSH_RT_DST(_rt) ((_rt)->dst)
#else /* LINUX_RT_DST_IS_NOT_IN_UNION */
#define SSH_RT_DST(_rt) ((_rt)->u.dst)
#endif /* LINUX_RT_DST_IS_NOT_IN_UNION */

/* Starting from linux 2.6.35 the IPv6 address list needs to be iterated
   using the list_for_each_* macros. */
#ifdef LINUX_HAS_INET6_IFADDR_LIST_HEAD
#define SSH_INET6_IFADDR_LIST_FOR_EACH(item, next, list)	        \
  list_for_each_entry_safe((item), (next), &(list), if_list)
#else /* LINUX_HAS_INET6_IFADDR_LIST_HEAD */
#define SSH_INET6_IFADDR_LIST_FOR_EACH(item, next, list)		\
  for ((item) = (list), (next) = NULL;					\
       (item) != NULL;							\
       (item) = (item)->if_next)
#endif /* LINUX_HAS_INET6_IFADDR_LIST_HEAD */

#if defined(LINUX_DST_ALLOC_HAS_MANY_ARGS)
#define SSH_DST_ALLOC(_dst) dst_alloc((_dst)->ops, NULL, 0, 0, 0)
#elif defined(LINUX_DST_ALLOC_HAS_REFCOUNT)
#define SSH_DST_ALLOC(_dst) dst_alloc((_dst)->ops, 0)
#else /* defined(LINUX_DST_ALLOC_HAS_REFCOUNT) */
#define SSH_DST_ALLOC(_dst) dst_alloc((_dst)->ops)
#endif /* defined(LINUX_DST_ALLOC_HAS_REFCOUNT) */

#if defined(LINUX_SSH_RTABLE_FIRST_ELEMENT_NEEDED)
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,38)
#define SSH_RTABLE_FIRST_MEMBER(_rt) ((_rt)->fl)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)
#define SSH_RTABLE_FIRST_MEMBER(_rt) ((_rt)->rt_key_dst)
#endif /* LINUX_VERSION_CODE */
#endif /* defined(LINUX_SSH_RTABLE_FIRST_ELEMENT_NEEDED) */

/* Linux 2.6.39 removed fl4_dst and fl6_dst defines. We like to use
   those so redefinig those for our purposes. */
#ifdef LINUX_FLOWI_NO_FL4_ACCESSORS
#define fl4_dst u.ip4.daddr
#define fl4_src	u.ip4.saddr
#define fl4_tos	u.ip4.__fl_common.flowic_tos
#define oif u.ip4.__fl_common.flowic_oif
#define proto u.ip4.__fl_common.flowic_proto
#define fl4_scope u.ip4.__fl_common.flowic_scope
#endif /* LINUX_FLOWI_NO_FL4_ACCESSORS */

#ifdef LINUX_FLOWI_NO_FL6_ACCESSORS
#define fl6_dst u.ip6.daddr
#define fl6_src	u.ip6.saddr
#endif /* LINUX_FLOWI_NO_FL6_ACCESSORS */

/****************************** Statistics helper macros *********************/

#ifdef DEBUG_LIGHT

#define SSH_LINUX_STATISTICS(interceptor, block)         \
do                                                       \
  {                                                      \
    if ((interceptor))                                   \
      {                                                  \
        spin_lock_bh(&(interceptor)->statistics_lock);   \
        block;                                           \
        spin_unlock_bh(&(interceptor)->statistics_lock); \
      }                                                  \
  }                                                      \
while (0)

#else /* DEBUG_LIGHT */

#define SSH_LINUX_STATISTICS(interceptor, block)

#endif /* DEBUG_LIGHT */

/****************************** Interface handling ***************************/

/* Sanity check that the interface index 'ifnum' fits into
   the SshInterceptorIfnum data type. 'ifnum' may be equal to
   SSH_INTERCEPTOR_INVALID_IFNUM. */
#define SSH_LINUX_ASSERT_IFNUM(ifnum) \
SSH_ASSERT(((SshUInt32) (ifnum)) < ((SshUInt32) SSH_INTERCEPTOR_MAX_IFNUM) \
|| ((SshUInt32) (ifnum)) == ((SshUInt32) SSH_INTERCEPTOR_INVALID_IFNUM))

/* Sanity check that the interface index 'ifnum' is a valid
   SshInterceptorIfnum. */
#define SSH_LINUX_ASSERT_VALID_IFNUM(ifnum) \
SSH_ASSERT(((SshUInt32) (ifnum)) < ((SshUInt32) SSH_INTERCEPTOR_MAX_IFNUM) \
&& ((SshUInt32) (ifnum)) != ((SshUInt32) SSH_INTERCEPTOR_INVALID_IFNUM))

/* Interface structure for caching "ifindex->dev" mapping. */
typedef struct SshInterceptorInternalInterfaceRec
*SshInterceptorInternalInterface;

struct SshInterceptorInternalInterfaceRec
{
  /* Next entry in the hashtable chain */
  SshInterceptorInternalInterface next;
  /* Interface index */
  SshUInt32 ifindex;
  /* Linux net_device structure */
  struct net_device *dev;

  /* This field is used to mark existing interfaces,
     and to remove disappeared interfaces from the hashtable. */
  SshUInt8 generation;

  /* Pointer to private data. This is currently used only by Octeon. */
  void *context;
};

/* Number of hashtable slots in the interface hashtable. */
#define SSH_LINUX_IFACE_HASH_SIZE 256

/* Maximum number of entries in the interface hashtable.
   Currently equal to maximum interface number. */
#define SSH_LINUX_IFACE_TABLE_SIZE SSH_INTERCEPTOR_MAX_IFNUM

/****************************** Proc entries *********************************/

#define SSH_PROC_ROOT "vpnclient"
#define SSH_PROC_ENGINE "engine"
#define SSH_PROC_VERSION "version"

/* Ipm receive buffer size. This must be big enough to fit a maximum sized
   IP packet + internal packet data + ipm message header. There is only
   one receive buffer. */
#define SSH_LINUX_IPM_RECV_BUFFER_SIZE 66000

/* Ipm channel message structure. These structures are used for queueing
   messages from kernel to userspace. The maximum number of allocated messages
   is limited by SSH_LINUX_MAX_IPM_MESSAGES (in linux_params.h). */
typedef struct SshInterceptorIpmMsgRec
SshInterceptorIpmMsgStruct, *SshInterceptorIpmMsg;

struct SshInterceptorIpmMsgRec
{
  /* Send queue is doubly linked, freelist uses only `next'. */
  SshInterceptorIpmMsg next;
  SshInterceptorIpmMsg prev;

  SshUInt8 reliable : 1; /* message is reliable. */
  SshUInt8 emergency_mallocated : 1; /* message is allocated from heap */

  /* Offset for partially sent message */
  size_t offset;

  /* Message length and data. */
  size_t len;
  unsigned char *buf;
};

/* Ipm structure */
typedef struct SshInterceptorIpmRec
{
  /* RW lock for protecting the send message queue and the message freelist. */
  rwlock_t lock;

  /* Is ipm channel open */
  atomic_t open;

  /* Message freelist and number of allocated messages. */
  SshInterceptorIpmMsg msg_freelist;
  SshUInt32 msg_allocated;

  /* Output message queue */
  SshInterceptorIpmMsg send_queue;
  SshInterceptorIpmMsg send_queue_tail;

  /* Number of unreliable messages in the sed queue. */
  SshUInt32 send_queue_num_unreliable;

} SshInterceptorIpmStruct, *SshInterceptorIpm;


/* Structure for ipm channel /proc entry. */
typedef struct SshInterceptorIpmProcEntryRec
{
  /* /proc filesystem inode */
  struct proc_dir_entry *entry;

  /* RW lock for protecting the proc entry */
  rwlock_t lock;

  /* Is an userspace application using this entry */
  Boolean open;

  /* Is another read ongoing? When this is TRUE
     then `send_msg' is owned by the reader. */
  Boolean read_active;

  /* Is another write ongoing? When this is TRUE
     then `recv_buf' is owned by the writer. */
  Boolean write_active;

  /* Wait queue for blocking mode reads and writes. */
  wait_queue_head_t wait_queue;

  /* Output message under processing. */
  SshInterceptorIpmMsg send_msg;

  /* Input message length */
  size_t recv_len;

  /* Input message buffer */
  size_t recv_buf_size;
  unsigned char *recv_buf;

} SshInterceptorIpmProcEntryStruct, *SshInterceptorIpmProcEntry;


/* Structure for other /proc entries. */
typedef struct SshInterceptorProcEntryRec
{
  /* /proc filesystem entry */
  struct proc_dir_entry *entry;

  /* RW lock for protecting the proc entry */
  rwlock_t lock;

  /* Is an userspace application using this entry */
  Boolean open;

  /* Is another read or write ongoing? When this is TRUE
     then `buf' is owned by the reader/writer. */
  Boolean active;

  /* Number of bytes returned to the userpace application */
  size_t buf_len;

  /* Preallocated buffer for read and write operations. */
  char buf[1024];

} SshInterceptorProcEntryStruct, *SshInterceptorProcEntry;

/****************************** Dst entry cache ******************************/
#define SSH_DST_ENTRY_TBL_SIZE 128
typedef struct SshDstEntryRec
{
  struct dst_entry *dst_entry;

  unsigned long allocation_time;
  SshUInt32 dst_entry_id;

  struct SshDstEntryRec *next;
} *SshDstEntry, SshDstEntryStruct;


/****************************** Interceptor Object ***************************/

struct SshInterceptorRec
{
  /* Function pointers to netfilter infrastructure */
  struct
  {
    int (*ip_rcv_finish) (struct sk_buff *);
    int (*ip_finish_output) (struct sk_buff *);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
    int (*ip6_rcv_finish) (struct sk_buff *);
    int (*ip6_output_finish) (struct sk_buff *);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
    int (*arp_process) (struct sk_buff *);
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
  } linux_fn;

  SshVirtualAdapter virtual_adapters[SSH_LINUX_MAX_VIRTUAL_ADAPTERS];

  Boolean hooks_installed;

  /* Interface information used in ssh_interceptor_send()
     (and elsewhere obviously, but the aforementioned
     is the reason it is here). 'if_hash', 'if_table_size',
     and 'if_generation' are protected by 'if_table_lock' rwlock. */
  SshInterceptorInternalInterface if_hash[SSH_LINUX_IFACE_HASH_SIZE];

  SshInterceptorInternalInterface if_table;
  SshUInt32 if_table_size;
  SshUInt8 if_generation;

  /* Protected by interceptor_lock */
  int num_interface_callbacks;

  /* Notifiers, notifies when interfaces change. */
  Boolean iface_notifiers_installed;

  struct notifier_block notifier_netdev;
  struct notifier_block notifier_inetaddr;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  struct notifier_block notifier_inet6addr;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  /* Reader Writer lock for interface table manipulation */
  rwlock_t if_table_lock;

  /* Registered callbacks */

  /* Interface callback. Protected by 'interceptor_lock' */
  SshInterceptorInterfacesCB interfaces_callback;

  /* Unused and unprotected */
  SshInterceptorRouteChangeCB route_callback;

  /* Callback for packet. Protected by rcu. */
  SshInterceptorPacketCB packet_callback;

  /* Context for interface, route and packet callbacks. */
  void *callback_context;

  /* Engine context */
  SshEngine engine;
  Boolean engine_open;

  /* Intercept packets. */
  Boolean enable_interception;

  /* Name of related engine-instance */
  char *name;

  /* Ipm channel */
  SshInterceptorIpmStruct ipm;

  /* /proc filesystem entries */
  struct proc_dir_entry *proc_dir;

  SshInterceptorIpmProcEntryStruct ipm_proc_entry;

  SshInterceptorProcEntryStruct version_proc_entry;

  /* Main mutex for interceptor use */
  SshKernelMutex interceptor_lock;

  /* Mutex for memory map manipulation */
  SshKernelMutex memory_lock;

  /* Mutex for packet handling */
  SshKernelMutex packet_lock;

  SshKernelMutex dst_entry_cache_lock;
  Boolean dst_entry_cache_timeout_registered;
  SshUInt32 dst_entry_id;
  struct timer_list dst_cache_timer;
  SshUInt32 dst_entry_cached_items;
  SshDstEntry dst_entry_table[SSH_DST_ENTRY_TBL_SIZE];

#ifdef DEBUG_LIGHT
  /* Statistics spin lock */
  spinlock_t statistics_lock;

  struct {
    /* Statistics */
    SshUInt64 num_packets_out;
    SshUInt64 num_packets_in;
    SshUInt64 num_bytes_out;
    SshUInt64 num_bytes_in;
    SshUInt64 num_passthrough;
    SshUInt64 num_fastpath_packets_in;
    SshUInt64 num_fastpath_packets_out;
    SshUInt64 num_fastpath_bytes_in;
    SshUInt64 num_fastpath_bytes_out;
    SshUInt64 num_errors;
    SshUInt64 num_packets_sent;
    SshUInt64 num_bytes_sent;
    SshUInt64 allocated_memory;
    SshUInt64 allocated_memory_max;
    SshUInt64 num_allocations;
    SshUInt64 num_allocations_large;
    SshUInt64 num_allocations_total;
    SshUInt64 num_allocated_packets;
    SshUInt64 num_allocated_packets_total;
    SshUInt64 num_copied_packets;
    SshUInt64 num_failed_allocs;
    SshUInt64 num_light_locks;
    SshUInt64 num_light_interceptor_locks;
    SshUInt64 num_heavy_locks;
    SshUInt64 ipm_send_queue_len;
    SshUInt64 ipm_send_queue_bytes;
  } stats;
#endif /* DEBUG_LIGHT */
};

typedef struct SshInterceptorRec SshInterceptorStruct;

/****************************** Function prototypes **************************/

/* Call packet_callback */
#define SSH_LINUX_INTERCEPTOR_PACKET_CALLBACK(interceptor, pkt)		    \
  do {									    \
    rcu_read_lock();							    \
    (interceptor)->packet_callback((pkt), (interceptor)->callback_context); \
    rcu_read_unlock();							    \
  } while (0)

/* Proc entries */
Boolean ssh_interceptor_proc_init(SshInterceptor interceptor);
void ssh_interceptor_proc_uninit(SshInterceptor interceptor);

/* Ipm channel */

/* init / uninit */
Boolean ssh_interceptor_ipm_init(SshInterceptor interceptor);
void ssh_interceptor_ipm_uninit(SshInterceptor interceptor);

/* open / close. These functions handle ipm message queue flushing. */
void interceptor_ipm_open(SshInterceptor interceptor);
void interceptor_ipm_close(SshInterceptor interceptor);

/* open / close notifiers. These functions notify engine. */
void ssh_interceptor_notify_ipm_open(SshInterceptor interceptor);
void ssh_interceptor_notify_ipm_close(SshInterceptor interceptor);

Boolean ssh_interceptor_send_to_ipm(unsigned char *data, size_t len,
				    Boolean reliable, void *machine_context);
ssize_t ssh_interceptor_receive_from_ipm(unsigned char *data, size_t len);

void interceptor_ipm_message_free(SshInterceptor interceptor,
                                  SshInterceptorIpmMsg msg);

/* Packet access and manipulation. */

/* Header-only allocation.
   This function will assert that the interface numbers will
   fit into the data type SshInterceptorIfnum. */
SshInterceptorInternalPacket
ssh_interceptor_packet_alloc_header(SshInterceptor interceptor,
                                    SshUInt32 flags,
                                    SshInterceptorProtocol protocol,
                                    SshUInt32 ifnum_in,
                                    SshUInt32 ifnum_out,
                                    struct sk_buff *skb,
                                    Boolean force_copy_skbuff,
                                    Boolean free_original_on_copy,
				    Boolean packet_from_system);



/* Allocates new packet skb with copied data from original
   + the extra free space reserved for extensions. */
struct sk_buff *
ssh_interceptor_packet_skb_dup(SshInterceptor interceptor,
			       struct sk_buff *skb,
                               size_t addbytes_active_ofs,
                               size_t addbytes_active);

/* Align the interceptor packet at the data offset 'offset' to a word
   boundary. On failure, 'pp' is freed and returns FALSE. */
Boolean
ssh_interceptor_packet_align(SshInterceptorPacket packet, size_t offset);

/* Verify that `skbp' has enough headroom to be sent out through `skbp->dev'.
   On failure this frees `skbp' and returns NULL. */
struct sk_buff *
ssh_interceptor_packet_verify_headroom(struct sk_buff *skbp,
				       size_t media_header_len);

void
ssh_interceptor_packet_return_dst_entry(SshInterceptor interceptor,
					SshUInt32 dst_entry_id,
					SshInterceptorPacket pp,
					Boolean remove_only);
SshUInt32
ssh_interceptor_packet_cache_dst_entry(SshInterceptor interceptor,
				       SshInterceptorPacket pp);

Boolean
ssh_interceptor_dst_entry_cache_init(SshInterceptor interceptor);

void
ssh_interceptor_dst_entry_cache_flush(SshInterceptor interceptor);

void
ssh_interceptor_dst_entry_cache_uninit(SshInterceptor interceptor);

/* Packet freelist init / uninit. */
Boolean ssh_interceptor_packet_freelist_init(SshInterceptor interceptor);
void ssh_interceptor_packet_freelist_uninit(SshInterceptor interceptor);

int ssh_linux_module_inc_use_count(void);
void ssh_linux_module_dec_use_count(void);


Boolean ssh_interceptor_ip_glue_init(SshInterceptor interceptor);
Boolean ssh_interceptor_ip_glue_uninit(SshInterceptor interceptor);

int ssh_interceptor_hook_magic_init(void);

struct net_device *
ssh_interceptor_ifnum_to_netdev(SshInterceptor interceptor, SshUInt32 ifnum);
struct net_device *
ssh_interceptor_ifnum_to_netdev_ctx(SshInterceptor interceptor,
				    SshUInt32 ifnum, void **context_return);
void ssh_interceptor_release_netdev(struct net_device *dev);
void ssh_interceptor_receive_ifaces(SshInterceptor interceptor);
void ssh_interceptor_clear_ifaces(SshInterceptor interceptor);
Boolean ssh_interceptor_iface_init(SshInterceptor interceptor);
void ssh_interceptor_iface_uninit(SshInterceptor interceptor);

/* skb rerouting */
Boolean ssh_interceptor_reroute_skb_ipv4(SshInterceptor interceptor,
					 struct sk_buff *skb,
					 SshUInt16 route_selector,
					 SshUInt32 ifnum_in);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
Boolean ssh_interceptor_reroute_skb_ipv6(SshInterceptor interceptor,
					 struct sk_buff *skb,
					 SshUInt16 route_selector,
					 SshUInt32 ifnum_in);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

#endif /* LINUX_INTERNAL_H */
