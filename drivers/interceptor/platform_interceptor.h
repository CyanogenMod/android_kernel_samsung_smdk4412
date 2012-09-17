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
 * platform_interceptor.h
 *
 * Linux interceptor specific defines for the Interceptor API.
 *
 */

#ifndef SSH_PLATFORM_INTERCEPTOR_H

#define SSH_PLATFORM_INTERCEPTOR_H 1

#ifdef KERNEL
#ifndef KERNEL_INTERCEPTOR_USE_FUNCTIONS

#define ssh_interceptor_packet_len(pp) \
  ((size_t)((SshInterceptorInternalPacket)(pp))->skb->len)

#include "linux_params.h"
#include "linux_packet_internal.h"

#endif /* KERNEL_INTERCEPTOR_USE_FUNCTIONS */
#endif /* KERNEL */

#endif /* SSH_PLATFORM_INTERCEPTOR_H */
