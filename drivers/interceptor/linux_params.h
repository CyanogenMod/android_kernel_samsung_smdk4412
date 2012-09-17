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
 * linux_params.h
 *
 * Linux interceptor tunable parameters.
 *
 */

#ifndef LINUX_PARAMS_H
#define LINUX_PARAMS_H

/* Netfilter interoperability flag. This flags specifies the extension
   selector slot which is used for storing the Linux 'skb->nfmark' firewall
   mark.  Note that in kernel versions before 2.6.20 the linux kernel
   CONFIG_IP_ROUTE_FWMARK must be enabled if you wish to use `skb->nfmark'
   in route lookups.  This define is not used if
   SSH_INTERCEPTOR_NUM_EXTENSION_SELECTORS is 0. */
/* #define SSH_LINUX_FWMARK_EXTENSION_SELECTOR 0 */

/* The upper treshold of queued messages from the engine to the policymanager.
   If this treshold is passed, then "unreliable" messages (messages not
   necessary for the correct operation of the engine/policymanager), are
   discarded. Both existing queued messages or new messages can be
   discarded. */
#define SSH_LINUX_MAX_IPM_MESSAGES 2000

/* Disable IPV6 support in the interceptor here, if explicitly desired.
   Undefining SSH_LINUX_INTERCEPTOR_IPV6 results into excluding IPv6
   specific code in the interceptor. The define does not affect the
   size of any common data structures.
   Currently it is disabled by default if IPv6 is not available in the
   kernel. */
#if defined (WITH_IPV6)
#define SSH_LINUX_INTERCEPTOR_IPV6 1
#ifndef CONFIG_IPV6
#ifndef CONFIG_IPV6_MODULE
#undef SSH_LINUX_INTERCEPTOR_IPV6
#endif /* !CONFIG_IPV6_MODULE */
#endif /* !CONFIG_IPV6 */
#endif /* WITH_IPV6 */

/* Netfilter interoperability flag. If this flag is set, then packets
   intercepted at the PRE_ROUTING hook are passed to other netfilter modules
   before the packet is given to the engine for processing. */
/* #define SSH_LINUX_NF_PRE_ROUTING_BEFORE_ENGINE 1 */

/* Netfilter interoperability flag. If this flag is set, then packets
   sent to host stack are passed to other netfilter modules in the PRE_ROUTING
   hook after the packet has been processed in the engine. */
/* #define SSH_LINUX_NF_PRE_ROUTING_AFTER_ENGINE 1 */

/* Netfilter interoperability flag. If this flag is set, then packets
   intercepted at the POST_ROUTING hook are passed to other netfilter
   modules before the packet is given to the engine for processing. */
/* #define SSH_LINUX_NF_POST_ROUTING_BEFORE_ENGINE 1 */

/* Netfilter interoperability flag. If this flag is set, then packets
   sent to network are passed to other netfilter modules in the POST_ROUTING
   hook after the packet has been processed in the engine. This flag is usable
   only if SSH_IPSEC_IP_ONLY_INTERCEPTOR is defined. */
#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
/* #define SSH_LINUX_NF_POST_ROUTING_AFTER_ENGINE 1 */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

/* Netfilter interoperability flag. If this flags is set, then forwarded
   packets are passed to netfilter modules in the FORWARD hook after
   the packet has been processed in the engine.  This flag is usable
   only if SSH_IPSEC_IP_ONLY_INTERCEPTOR defined, and if the engine performs
   packet forwarding (that is, SSH_ENGINE_FLAGS does not include
   SSH_ENGINE_NO_FORWARDING). */
#ifdef SSH_IPSEC_IP_ONLY_INTERCEPTOR
/* #define SSH_LINUX_NF_FORWARD_AFTER_ENGINE 1 */
#endif /* SSH_IPSEC_IP_ONLY_INTERCEPTOR */

#endif /* LINUX_PARAMS_H */
