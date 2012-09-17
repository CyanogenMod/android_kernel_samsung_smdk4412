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
 * sshconf.h
 *
 * Generic configuration defines.
 *
 */

#ifndef SSHCONF_H
#define SSHCONF_H

/* Light debugging */
#define DEBUG_LIGHT 1

/* Interceptor has its own version of ssh_interceptor_packet_copy */
/* #undef INTERCEPTOR_HAS_PACKET_COPY */

/* Interceptor has its own version of ssh_interceptor_packet_copyin */
#define INTERCEPTOR_HAS_PACKET_COPYIN 1

/* Interceptor has its own version of ssh_interceptor_packet_copyout */
#define INTERCEPTOR_HAS_PACKET_COPYOUT 1

/* Interceptor has its own versions of
   ssh_interceptor_export_internal_data and
   ssh_interceptor_import_internal_data */
#define INTERCEPTOR_HAS_PACKET_INTERNAL_DATA_ROUTINES 1

/* Interceptor has "platform_interceptor.h" include file
   to be included by interceptor.h. */
#define INTERCEPTOR_HAS_PLATFORM_INCLUDE 1

/* Should the interceptor align the IP header of packets to word boundary
   when sending to the network or stack? */
/* #undef INTERCEPTOR_IP_ALIGNS_PACKETS */

/* Define this if the interceptor operates at IP level (that is, no
   interface supplies or requires packets at ethernet level, or
   generally media level).  Generally there is no much difference in
   performance whether the interceptor operates at ethernet level or
   at IP level; however, some functionality (particularity the ability
   to proxy arp so that the same subnet can be shared for both
   external and DMZ interfaces) is not available without an ethernet
   level interceptor. */
#define SSH_IPSEC_IP_ONLY_INTERCEPTOR 1

#define KERNEL_SIZEOF_SHORT 2
#define KERNEL_SIZEOF_INT 4
#define KERNEL_SIZEOF_LONG_LONG 8

#ifdef __LP64__
#define KERNEL_SIZEOF_LONG 8
#define KERNEL_SIZEOF_SIZE_T 8
#define KERNEL_SIZEOF_VOID_P 8
#else
#define KERNEL_SIZEOF_LONG 4
#define KERNEL_SIZEOF_SIZE_T 4
#define KERNEL_SIZEOF_VOID_P 4
#endif

#include <asm/byteorder.h>

#if defined(__BIG_ENDIAN)
#define WORDS_BIGENDIAN
#elif !defined(__LITTLE_ENDIAN)
#error cannot determine byte order
#endif

/* "Have" for kernel basic types */
#define HAVE_KERNEL_INT 1
#define HAVE_KERNEL_LONG 1
#define HAVE_KERNEL_LONG_LONG 1
#define HAVE_KERNEL_SHORT 1
#define HAVE_KERNEL_VOID_P 1

/* Define this to enable IPv6 support. */
#define WITH_IPV6 1

#endif /* SSHCONF_H */
