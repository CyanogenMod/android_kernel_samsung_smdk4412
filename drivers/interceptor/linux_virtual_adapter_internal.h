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
 *
 * linux_virtual_adapter_internal.h
 *
 * Internal declarations for linux virtual adapters.
 *
 */


#ifndef LINUX_VIRTUAL_ADAPTER_INTERNAL_H
#define LINUX_VIRTUAL_ADAPTER_INTERNAL_H

#include "virtual_adapter.h"

/* **************************** Types and Definitions ************************/

/* Maximum number of virtual adapters. */
#define SSH_LINUX_MAX_VIRTUAL_ADAPTERS 16

/* Prefix for adapter names. */
#define SSH_ADAPTER_NAME_PREFIX "vip"

/* HW address lenght. */
#define SSH_MAX_VIRTUAL_ADAPTER_HWADDR_LEN 6

/* Maximum number of configured IPv6 addresses the virtual adapter saves
   and restores on ifdown / ifup. */
#define SSH_VIRTUAL_ADAPTER_MAX_IPV6_ADDRS 2

/* Context for a virtual adapter. */
typedef struct SshVirtualAdapterRec
{
  /* Is the adapter initialized.  This is 0 until the
     ssh_virtual_adapter_create() returns. */
  SshUInt8 initialized : 1;

  /* Is the adapter destroyed. */
  SshUInt8 destroyed : 1;

  /* Is the adapter attached to engine. */
  SshUInt8 attached : 1;

  /* Packet callback. */
  SshVirtualAdapterPacketCB packet_cb;

  /* Destructor for engine-level block */
  SshVirtualAdapterDetachCB detach_cb;
  void *adapter_context;

  /* The low-level implementation of a virtual adapter. */

  /* Platform dependant low-level implementation structure. */
  struct net_device *dev;
  struct net_device_stats low_level_stats;

} SshVirtualAdapterStruct, *SshVirtualAdapter;

/* ******************************* Functions *********************************/

int ssh_interceptor_virtual_adapter_init(SshInterceptor interceptor);
int ssh_interceptor_virtual_adapter_uninit(SshInterceptor interceptor);

#endif /* LINUX_VIRTUAL_ADAPTER_INTERNAL_H */
