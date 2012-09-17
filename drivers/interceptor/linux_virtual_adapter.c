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
 * linux_virtual_adapter.c
 *
 */

#include "linux_internal.h"

#include "virtual_adapter.h"
#include "linux_virtual_adapter_internal.h"
#include "sshencode.h"
#include "sshinetencode.h"

extern SshInterceptor ssh_interceptor_context;

/* ************************ Types and definitions ***************************/

#define SSH_DEBUG_MODULE "SshInterceptorVirtualAdapter"

static int num_virtual_adapters = 1; /* Default to 1 virtual adapter. */
MODULE_PARM_DESC(num_virtual_adapters, "Number of virtual adapters to create");
module_param(num_virtual_adapters, int, 0444);


#ifdef LINUX_HAS_NET_DEVICE_PRIV
#define SSH_LINUX_NET_DEVICE_PRIV(netdev) ((netdev)->priv)
#else
#define SSH_LINUX_NET_DEVICE_PRIV(netdev) (*((void **) netdev_priv(netdev)))
#endif


/* *********************** Internal Utility Functions ***********************/

static inline SshVirtualAdapter
ssh_virtual_adapter_ifnum_to_adapter(SshInterceptor interceptor,
				     SshInterceptorIfnum adapter_ifnum)
{
  SshVirtualAdapter adapter = NULL;
  SshUInt32 i;

  ssh_kernel_mutex_assert_is_locked(interceptor->interceptor_lock);

  for (i = 0; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
    {
      adapter = interceptor->virtual_adapters[i];
      if (adapter && adapter->dev->ifindex == adapter_ifnum)
	return adapter;
    }
  return NULL;
}


/* ******************* Public Virtual Adapter Operations *********************/

void ssh_virtual_adapter_get_status(SshInterceptor interceptor,
				    SshInterceptorIfnum adapter_ifnum,
				    SshVirtualAdapterStatusCB status_cb,
				    void *context)
{
  SshVirtualAdapter adapter;
  unsigned char adapter_name[SSH_INTERCEPTOR_IFNAME_SIZE];
  SshVirtualAdapterState adapter_state;
  unsigned int dev_flags;
  void *adapter_context;
  SshUInt32 i;

  if (adapter_ifnum == SSH_INTERCEPTOR_INVALID_IFNUM)
    {
      i = 0;
    restart:
      local_bh_disable();
      ssh_kernel_mutex_lock(interceptor->interceptor_lock);
      for (; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
	{
	  adapter = interceptor->virtual_adapters[i];
	  if (adapter == NULL)
	    continue;

	  SSH_ASSERT(adapter->dev != NULL);
	  adapter_ifnum = (SshInterceptorIfnum) adapter->dev->ifindex;
	  snprintf(adapter_name, SSH_INTERCEPTOR_IFNAME_SIZE,
		   "%s", adapter->dev->name);
	  dev_flags = SSH_LINUX_DEV_GET_FLAGS(adapter->dev);
	  adapter_state = SSH_VIRTUAL_ADAPTER_STATE_DOWN;
	  if (dev_flags & IFF_UP)
	    adapter_state = SSH_VIRTUAL_ADAPTER_STATE_UP;
	  adapter_context = adapter->adapter_context;
	  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	  local_bh_enable();
	  (*status_cb)(SSH_VIRTUAL_ADAPTER_ERROR_OK_MORE,
		       adapter_ifnum, adapter_name, adapter_state,
		       adapter_context, context);
	  i++;
	  goto restart;
	}
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      (*status_cb)(SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT,
		   SSH_INTERCEPTOR_INVALID_IFNUM, NULL,
		   SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED,
		   NULL, context);
      return;
    }
  else
    {
      local_bh_disable();
      ssh_kernel_mutex_lock(interceptor->interceptor_lock);
      adapter = ssh_virtual_adapter_ifnum_to_adapter(interceptor,
						     adapter_ifnum);
      if (adapter == NULL)
	{
	  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	  local_bh_enable();
	  (*status_cb)(SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT,
		       adapter_ifnum, NULL,
		       SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED,
		       NULL, context);
	  return;
	}

      SSH_ASSERT(adapter->dev != NULL);
      snprintf(adapter_name, SSH_INTERCEPTOR_IFNAME_SIZE,
	       "%s", adapter->dev->name);
      dev_flags = SSH_LINUX_DEV_GET_FLAGS(adapter->dev);
      adapter_state = SSH_VIRTUAL_ADAPTER_STATE_DOWN;
      if (dev_flags & IFF_UP)
	adapter_state = SSH_VIRTUAL_ADAPTER_STATE_UP;
      adapter_context = adapter->adapter_context;
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      (*status_cb)(SSH_VIRTUAL_ADAPTER_ERROR_OK,
		   adapter_ifnum, adapter_name, adapter_state,
		   adapter_context, context);
      return;
    }
}

void ssh_virtual_adapter_attach(SshInterceptor interceptor,
                                SshInterceptorIfnum adapter_ifnum,
                                SshVirtualAdapterPacketCB packet_cb,
                                SshVirtualAdapterDetachCB detach_cb,
                                void *adapter_context,
                                SshVirtualAdapterStatusCB callback,
                                void *context)
{
  SshVirtualAdapter adapter;
  unsigned char adapter_name[SSH_INTERCEPTOR_IFNAME_SIZE];
  SshVirtualAdapterState adapter_state = SSH_VIRTUAL_ADAPTER_STATE_DOWN;
  unsigned int dev_flags;
  SshVirtualAdapterDetachCB old_detach_cb;
  void *old_adapter_context;

 restart:
  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  adapter = ssh_virtual_adapter_ifnum_to_adapter(interceptor, adapter_ifnum);
  if (adapter == NULL)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      SSH_DEBUG(SSH_D_ERROR, ("Attach failed for virtual adapter %d",
			      (int) adapter_ifnum));
      if (callback)
	(*callback)(SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT,
		    adapter_ifnum, NULL, SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED,
		    NULL, context);
      return;
    }

  /* Destroy old adapter_context. */
  if (adapter->detach_cb != NULL)
    {
      old_detach_cb = adapter->detach_cb;
      old_adapter_context = adapter->adapter_context;
      adapter->detach_cb = NULL;
      adapter->adapter_context = NULL;
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      (*old_detach_cb)(old_adapter_context);
      goto restart;
    }

  /* Fill in new adapter_context and callbacks. */
  adapter->adapter_context = adapter_context;
  adapter->packet_cb = packet_cb;
  adapter->detach_cb = detach_cb;
  adapter->attached = 1;

  /* Gather info for callback. */
  if (callback)
    {
      snprintf(adapter_name, SSH_INTERCEPTOR_IFNAME_SIZE,
	       "%s", adapter->dev->name);
      dev_flags = SSH_LINUX_DEV_GET_FLAGS(adapter->dev);
      if (dev_flags & IFF_UP)
	adapter_state = SSH_VIRTUAL_ADAPTER_STATE_UP;
    }

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();

  if (callback)
    (*callback)(SSH_VIRTUAL_ADAPTER_ERROR_OK,
		adapter_ifnum, adapter_name, adapter_state,
		adapter_context, context);
}

/* Clear virtual adapter callbacks, context, and configured IPv6 addresses. */
static void ssh_virtual_adapter_clear(SshVirtualAdapter adapter)
{
  /* Clear adapter_context and callbacks. */
  adapter->detach_cb = NULL;
  adapter->adapter_context = NULL;
  adapter->packet_cb = NULL;
}

void ssh_virtual_adapter_detach(SshInterceptor interceptor,
                                SshInterceptorIfnum adapter_ifnum,
                                SshVirtualAdapterStatusCB callback,
                                void *context)
{
  SshVirtualAdapter adapter;
  unsigned char adapter_name[SSH_INTERCEPTOR_IFNAME_SIZE];
  SshVirtualAdapterDetachCB detach_cb = NULL;
  void *adapter_context = NULL;

  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  adapter = ssh_virtual_adapter_ifnum_to_adapter(interceptor, adapter_ifnum);
  if (adapter == NULL)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      if (callback)
	(*callback)(SSH_VIRTUAL_ADAPTER_ERROR_NONEXISTENT,
		    adapter_ifnum, NULL, SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED,
		    NULL, context);
      return;
    }

  if (adapter->detach_cb)
    {
      detach_cb = adapter->detach_cb;
      adapter_context = adapter->adapter_context;
    }

  ssh_virtual_adapter_clear(adapter);
  adapter->attached = 0;

  /* Gather info for callback. */
  if (callback)
    {
      snprintf(adapter_name, SSH_INTERCEPTOR_IFNAME_SIZE,
	       "%s", adapter->dev->name);
    }

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();

  /* Destroy adapter_context. */
  if (detach_cb)
    (*detach_cb)(adapter_context);

  if (callback)
    (*callback)(SSH_VIRTUAL_ADAPTER_ERROR_OK,
		adapter_ifnum, adapter_name,
		SSH_VIRTUAL_ADAPTER_STATE_UNDEFINED,
		NULL, context);
}

void ssh_virtual_adapter_detach_all(SshInterceptor interceptor)
{
  SshVirtualAdapter adapter;
  SshUInt32 i = 0;
  SshVirtualAdapterDetachCB detach_cb;
  void *adapter_context;

 restart:
  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  for (; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
    {
      adapter = interceptor->virtual_adapters[i];
      if (adapter == NULL)
	continue;

      detach_cb = NULL;
      adapter_context = NULL;
      if (adapter->detach_cb != NULL)
	{
	  detach_cb = adapter->detach_cb;
	  adapter_context = adapter->adapter_context;
	}

      ssh_virtual_adapter_clear(adapter);
      adapter->attached = 0;

      if (detach_cb != NULL)
	{
	  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	  local_bh_enable();

	  /* Destroy adapter_context. */
	  (*detach_cb)(adapter_context);

	  goto restart;
	}
    }
  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();
}

/* ****************************** Platform stuff *****************************/

/* Netdevice low level statistics callback function. */
static struct net_device_stats *
ssh_virtual_adapter_get_stats(struct net_device *dev)
{
  SshVirtualAdapter adapter =
    (SshVirtualAdapter) SSH_LINUX_NET_DEVICE_PRIV(dev);

  if (adapter)
    return &adapter->low_level_stats;

  return NULL;
}

/* Netdevice low level transmit callback function. */
static int
ssh_virtual_adapter_xmit(struct sk_buff *skbp,
			 struct net_device *dev)
{
  struct net_device_stats *stats;
  SshInterceptorInternalPacket ipp;
  SshInterceptor interceptor;
  SshVirtualAdapter adapter;
  SshInterceptorIfnum ifnum_in;
  SshVirtualAdapterPacketCB packet_cb;
  void *adapter_context;

  SSH_ASSERT(skbp != NULL && dev != NULL);

  interceptor = ssh_interceptor_context;

  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  adapter = (SshVirtualAdapter) SSH_LINUX_NET_DEVICE_PRIV(dev);
  if (adapter == NULL)
    {
      /* Virtual adapter is not attached. */
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("Device %d [%s] is not attached to a SshVirtualAdapter",
		 dev->ifindex, dev->name));
    discard:
      /* Silently discard the packet. */
      dev_kfree_skb_any(skbp);
      return NET_XMIT_SUCCESS;
    }

  /* Update statistics */
  stats = ssh_virtual_adapter_get_stats(dev);
  SSH_ASSERT(stats != NULL);
  stats->tx_packets++;

  if (!adapter->initialized || !adapter->packet_cb)
    {
      /* This is not very uncommon. Packets end up here if the virtual
	 adapter is set up when policymanager is not running. We discard
	 the packets silently, as otherwise the stack will attempt to
	 transmit IPv6 IGMP messages indefinitely. */
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      SSH_DEBUG(SSH_D_LOWOK,
		("Virtual adapter %d [%s] not initialized / no cb.",
		 adapter->dev->ifindex, adapter->dev->name));
      goto discard;
    }

  ifnum_in = (SshInterceptorIfnum) dev->ifindex;
  SSH_LINUX_ASSERT_VALID_IFNUM(ifnum_in);

  /* Pass the packet to the packet callback. */
  ipp = ssh_interceptor_packet_alloc_header(interceptor,
				 SSH_PACKET_FROMPROTOCOL,
				 SSH_PROTOCOL_ETHERNET,
				 ifnum_in,
				 SSH_INTERCEPTOR_INVALID_IFNUM,
				 skbp,
                                 FALSE, FALSE, TRUE);
  if (ipp == NULL)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("Could not allocate packet header, virtual adapter %d [%s]",
		 adapter->dev->ifindex, adapter->dev->name));
      stats->tx_errors++;
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      return NET_XMIT_DROP;
    }

  stats->tx_bytes += ipp->skb->len;

  packet_cb = adapter->packet_cb;
  adapter_context = adapter->adapter_context;

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);

  SSH_DEBUG(SSH_D_NICETOKNOW,
	    ("Passing skb %p from virtual adapter %d [%s] to engine",
	     ipp->skb, (int)ifnum_in, dev->name));

  /* Call the callback.  This will eventually free `pp'. */
  (*packet_cb)(interceptor, (SshInterceptorPacket) ipp, adapter_context);

  return NET_XMIT_SUCCESS;
}

#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops ssh_virtual_adapter_ops = {
	.ndo_start_xmit		= ssh_virtual_adapter_xmit,
        .ndo_get_stats          = ssh_virtual_adapter_get_stats,
};
#endif

/* **************** Netdevice registration / unregistration ******************/

static void
ssh_virtual_adapter_low_level_setup(struct net_device *dev)
{
  /* Empty at the moment, but some netdevice specific settings could be done
     here. */
}

static void
ssh_virtual_adapter_destructor(struct net_device *dev)
{
  /* Free netdevice object. */
  free_netdev(dev);
}

/* Attach virtual adapter to system. Returns FALSE on error, TRUE otherwise. */
static Boolean
ssh_virtual_adapter_attach_low_level(SshVirtualAdapter adapter,
				     unsigned char *adapter_name,
				     unsigned char *adapter_enaddr)
{
  int i, result;

  /*
    After priv member has disappeared from struct net_device, store
    the "priv" pointer to private data area following the structure.
   */
#ifdef LINUX_HAS_NET_DEVICE_PRIV
  const unsigned int private_data_size = 0;
#else
  const unsigned int private_data_size = sizeof(void *);
#endif

  SSH_ASSERT(adapter->dev == NULL);
  SSH_ASSERT(adapter_name != NULL);
  SSH_ASSERT(adapter_enaddr != NULL);


  /* Allocate net_device. */
  adapter->dev = alloc_netdev(private_data_size, adapter_name,
			      ssh_virtual_adapter_low_level_setup);
  if (adapter->dev == NULL)
    {
      SSH_DEBUG(SSH_D_ERROR, ("alloc_netdev failed"));
      return FALSE;
    }

  /* Copy ethernet address. */
  for (i = 0; i < ETH_ALEN; i++)
    adapter->dev->dev_addr[i] = adapter_enaddr[i];

  /* Initialize the device structure. */
  /* Set device private data to point to virtual adapter structure. */
  SSH_LINUX_NET_DEVICE_PRIV(adapter->dev) = (void *) adapter;

#ifdef HAVE_NET_DEVICE_OPS
  adapter->dev->netdev_ops = &ssh_virtual_adapter_ops;
#else
  adapter->dev->hard_start_xmit = ssh_virtual_adapter_xmit;
  adapter->dev->get_stats = ssh_virtual_adapter_get_stats;
#endif

  memset(&adapter->low_level_stats, 0, sizeof(struct net_device_stats));
  adapter->dev->tx_queue_len = 0; /* no transmit queue */
  adapter->dev->destructor = ssh_virtual_adapter_destructor;

  /* Fill in the fields of the device structure with ethernet-generic
     values. */
  ether_setup(adapter->dev);

  adapter->dev->flags |= IFF_POINTOPOINT;

  /* Set default MTU. */
  adapter->dev->mtu -= 100;

  /* Register the network device. This call assigns a valid ifindex to
     virtual adapter and it triggers an interface event. */
  result = register_netdev(adapter->dev);

  if (result != 0)
    {
      /* register_netdev() failed. Free net_device. */
      SSH_LINUX_NET_DEVICE_PRIV(adapter->dev) = NULL;
      ssh_virtual_adapter_destructor(adapter->dev);
      adapter->dev = NULL;
      SSH_DEBUG(SSH_D_ERROR, ("register_netdev failed: %d", result));
      return FALSE;
    }

  /* All ok. */
  SSH_DEBUG(SSH_D_MIDOK, ("Virtual adapter %d [%s] attached successfully",
			  adapter->dev->ifindex, adapter->dev->name));
  return TRUE;
}

static void
ssh_virtual_adapter_detach_low_level(SshVirtualAdapter adapter)
{
#ifdef DEBUG_LIGHT
  unsigned char adapter_name[SSH_INTERCEPTOR_IFNAME_SIZE];
  SshInterceptorIfnum adapter_ifnum;
#endif /* DEBUG_LIGHT */

  /* Virtual adapter is not attached to system. */
  if (adapter->dev == NULL)
    return;

#ifdef DEBUG_LIGHT
  memcpy(adapter_name, adapter->dev->name, IFNAMSIZ);
  adapter_ifnum = (SshInterceptorIfnum) adapter->dev->ifindex;
#endif /* DEBUG_LIGHT */

  /* Remove the network device. This call triggers an interface event. */
  unregister_netdev(adapter->dev);

  /* Unlink netdevice structure, it will be freed in the device destructor. */
  adapter->dev = NULL;

  /* All ok. */
  SSH_DEBUG(SSH_D_NICETOKNOW, ("Virtual adapter %d [%s] detached",
			       (int)adapter_ifnum, adapter_name));
}

/* *************** Creating and Destroying Virtual Adapters *****************/

/* Workhorse for destroy_all and error handler for create. `adapter' must
   not be in the adapter table when this function is called. */
static Boolean
ssh_virtual_adapter_destroy(SshInterceptor interceptor,
                            SshVirtualAdapter adapter)
{
  SSH_ASSERT(adapter != NULL);

  ssh_kernel_mutex_assert_is_locked(interceptor->interceptor_lock);

  SSH_DEBUG(SSH_D_HIGHOK,
	    ("Destroying virtual adapter %d [%s]",
	     (adapter->dev ? adapter->dev->ifindex : -1),
	     (adapter->dev ? adapter->dev->name : "unknown")));

  if (adapter->dev)
    {
      /* Detach and destroy net_device. */
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();

      /* This call will produce an interface event. */
      ssh_virtual_adapter_detach_low_level(adapter);

      local_bh_disable();
      ssh_kernel_mutex_lock(interceptor->interceptor_lock);
      adapter->dev = NULL;
    }

  /* Free adapter */
  ssh_free(adapter);

  /* All ok. */
  return TRUE;
}

/* This function is called during module loading. */
static Boolean
ssh_virtual_adapter_create(SshInterceptor interceptor,
			   unsigned char *adapter_name)
{
  SshVirtualAdapter adapter;
  SshUInt32 i;
  Boolean error = FALSE;
  unsigned char created_adapter_name[SSH_INTERCEPTOR_IFNAME_SIZE];
  unsigned char adapter_enaddr[SSH_MAX_VIRTUAL_ADAPTER_HWADDR_LEN];

  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);

  /* Find a slot for this virtual adapter. Slot index will be used in
     adapter name and ethernet address creation. */
  for (i = 0; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
    {
      if (interceptor->virtual_adapters[i] == NULL)
	break;
    }
  if (i >= SSH_LINUX_MAX_VIRTUAL_ADAPTERS)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      SSH_DEBUG(SSH_D_ERROR, ("Maximum number of virtual adapters reached"));
      return FALSE;
    }

  /* Allocate the virtual adapter and store it in to the table. */
  adapter = ssh_calloc(1, sizeof(*adapter));
  if (adapter == NULL)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      SSH_DEBUG(SSH_D_ERROR, ("Could not allocate virtual adapter"));
      return FALSE;
    }
  interceptor->virtual_adapters[i] = adapter;

  /* Create a name for the virtual adapter. */
  if (adapter_name == NULL)
    {
      snprintf(created_adapter_name, SSH_INTERCEPTOR_IFNAME_SIZE,
	       "%s%d", SSH_ADAPTER_NAME_PREFIX, (int)i);
      adapter_name = created_adapter_name;
    }

  /* Create ethernet hardware address for virtual adapter. */
  ssh_virtual_adapter_interface_ether_address(i, adapter_enaddr);

  /* We can't have lock when attaching the adapter, as it will create an
     interface event. It is guaranteed that the adapter will not disappear
     under us, as it not yet marked initialized. */
  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();

  /* Initialize net_device structure and attach it to the system. */
  if (!ssh_virtual_adapter_attach_low_level(adapter,
					    adapter_name, adapter_enaddr))
    {
      SSH_DEBUG(SSH_D_ERROR,
		("Could not attach virtual adapter %s to system.",
		 adapter_name));
      error = TRUE;
    }
  else
    {
      /* We are attached to the system. */
      SSH_DEBUG(SSH_D_MIDOK,
		("Virtual adapter %lu [%s] created",
		 (unsigned long)adapter->dev->ifindex, adapter->dev->name));
    }

  /* Lock for finalizing adapter creation. */
  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);

  /* Mark adapter initialized. */
  adapter->initialized = 1;

  /* Adapter low level attach failed or adapter was destroyed under us.
     Free it now. */
  if (error || adapter->destroyed)
    {
      interceptor->virtual_adapters[i] = NULL;
      ssh_virtual_adapter_destroy(interceptor, adapter);
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      return FALSE;
    }

  /* All ok. */
  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();
  return TRUE;
}

/* This function is called during module unload. */
static void
ssh_virtual_adapter_destroy_all(SshInterceptor interceptor)
{
  SshVirtualAdapter adapter;
  Boolean check_again;
  SshUInt32 i;

 restart:
  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);

  check_again = FALSE;
  for (i = 0; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
    {
      adapter = interceptor->virtual_adapters[i];
      if (adapter == NULL)
	continue;

      if (!adapter->initialized)
	{
	  /* Initialization is underway, mark adapter to be destroyed. */
	  adapter->destroyed = 1;
	  check_again = TRUE;
	  continue;
	}

      /* Remove adapter from table. */
      interceptor->virtual_adapters[i] = NULL;

      /* Detach and destroy adapter. */
      ssh_virtual_adapter_destroy(interceptor, adapter);

      /* Unlock and restart. */
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      goto restart;
    }

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();

  if (check_again)
    goto restart;
}

/* ************ Interceptor Side Virtual Adapter Init / Uninit ***************/

/* Initialize virtual adapters. This function is called from linux_main.c
   during module loading. */
int ssh_interceptor_virtual_adapter_init(SshInterceptor interceptor)
{
  SshUInt32 i;

  SSH_ASSERT(!in_softirq());

  if (num_virtual_adapters > SSH_LINUX_MAX_VIRTUAL_ADAPTERS)
    {
      SSH_DEBUG(SSH_D_NICETOKNOW,
		("Maximum value for num_virtual_adapters is %d",
		 SSH_LINUX_MAX_VIRTUAL_ADAPTERS));
      return -1;
    }

  for (i = 0; i < num_virtual_adapters; i++)
    {
      if (!ssh_virtual_adapter_create(interceptor, NULL))
	goto error;
    }

  return 0;

 error:
  ssh_virtual_adapter_destroy_all(interceptor);
  return -1;
}

/* Uninitialize virtual adapters. This function is called from linux_main.c
   during module unloading. */
int ssh_interceptor_virtual_adapter_uninit(SshInterceptor interceptor)
{
  SSH_ASSERT(!in_softirq());

  /* Detach all virtual adapters from the system. */
  ssh_virtual_adapter_destroy_all(interceptor);

  return 0;
}

Boolean
ssh_virtual_adapter_param_decode(SshVirtualAdapterParams params,
				 const unsigned char *data, size_t len)
{
  unsigned char *dns;
  size_t dns_len;
  unsigned char *wins;
  size_t wins_len;
  unsigned char *win_domain;
  size_t win_domain_len;
  SshUInt32 netbios_node_type;
  SshUInt32 i;
  size_t decode_len;

  SSH_ASSERT(params != NULL);
  SSH_ASSERT(data != NULL);
  SSH_ASSERT(len > 0);

  memset(params, 0, sizeof(*params));

  if (ssh_decode_array(data, len,
		       SSH_DECODE_UINT32(&params->mtu),
		       SSH_DECODE_UINT32(&params->dns_ip_count),
		       SSH_DECODE_UINT32_STR_NOCOPY(&dns, &dns_len),
		       SSH_DECODE_UINT32(&params->wins_ip_count),
		       SSH_DECODE_UINT32_STR_NOCOPY(&wins, &wins_len),
		       SSH_DECODE_UINT32_STR_NOCOPY(
		       &win_domain, &win_domain_len),
		       SSH_DECODE_UINT32(&netbios_node_type),
		       SSH_FORMAT_END) != len)
    return FALSE;

  /* DNS. */
  if (params->dns_ip_count)
    {
      params->dns_ip = ssh_calloc(params->dns_ip_count,
				  sizeof(*params->dns_ip));
      if (params->dns_ip == NULL)
	goto error;

      for (i = 0; i < params->dns_ip_count; i++)
	{
	  decode_len = ssh_decode_ipaddr_array(dns, dns_len,
					       &params->dns_ip[i]);
	  if (decode_len == 0)
	    goto error;
	  dns += decode_len;
	  dns_len -= decode_len;
	}
    }

      /* WINS. */
  if (params->wins_ip_count)
    {
      params->wins_ip = ssh_calloc(params->wins_ip_count,
				   sizeof(*params->wins_ip));
      if (params->wins_ip == NULL)
	goto error;

      for (i = 0; i < params->wins_ip_count; i++)
	{
	  decode_len = ssh_decode_ipaddr_array(wins, wins_len,
					       &params->wins_ip[i]);
	  if (decode_len == 0)
	    goto error;
	  wins += decode_len;
	  wins_len -= decode_len;
	}
    }

  if (win_domain_len)
    {
      params->win_domain = ssh_memdup(win_domain, win_domain_len);
      if (params->win_domain == NULL)
	goto error;
    }

  params->netbios_node_type = (SshUInt8) netbios_node_type;

  return TRUE;

 error:
  ssh_free(params->dns_ip);
  ssh_free(params->wins_ip);
  ssh_free(params->win_domain);
  memset(params, 0, sizeof(*params));
  return FALSE;
}

void
ssh_virtual_adapter_interface_ether_address(SshInterceptorIfnum adapter_ifnum,
                                            unsigned char *buffer)
{
  memset(buffer, 0, SSH_ETHERH_ADDRLEN);
  buffer[1] = 1;
  SSH_PUT_32BIT(buffer + 2, (SshUInt32) adapter_ifnum + 1);
}

void
ssh_virtual_adapter_ip_ether_address(SshIpAddr ip, unsigned char *buffer)
{
  memset(buffer, 0, SSH_ETHERH_ADDRLEN);

  if (SSH_IP_IS4(ip))
    {
      buffer[1] = 2;
      SSH_IP4_ENCODE(ip, buffer + 2);
    }
#if defined (WITH_IPV6)
  else
    {
      SshUInt32 value;

      value = SSH_IP6_WORD0_TO_INT(ip);
      value ^= SSH_IP6_WORD1_TO_INT(ip);
      value ^= SSH_IP6_WORD2_TO_INT(ip);
      value ^= SSH_IP6_WORD3_TO_INT(ip);

      buffer[1] = 2;
      SSH_PUT_32BIT(buffer + 2, value);
    }
#endif /* WITH_IPV6 */
}


/* ****************** Sending Packets to Local Stack ************************/

void
ssh_virtual_adapter_send(SshInterceptor interceptor,
			 SshInterceptorPacket pp)
{
  SshVirtualAdapter adapter;
  SshInterceptorInternalPacket ipp = (SshInterceptorInternalPacket) pp;
  struct net_device_stats *stats;
  struct sk_buff *skb;

  local_bh_disable();
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  adapter = ssh_virtual_adapter_ifnum_to_adapter(interceptor, pp->ifnum_out);
  if (adapter == NULL)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      SSH_DEBUG(SSH_D_ERROR,
                ("Virtual adapter %d does not exist", (int)pp->ifnum_out));
      goto error;
    }

  /* Check the type of the source packet. */
  if (pp->protocol == SSH_PROTOCOL_ETHERNET)
    {
      /* We can send this directly. */
    }
  else if (pp->protocol == SSH_PROTOCOL_IP4
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
           || pp->protocol == SSH_PROTOCOL_IP6
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
	   )
    {
      unsigned char ether_hdr[SSH_ETHERH_HDRLEN];
      SshIpAddrStruct src;
      SshUInt16 ethertype = SSH_ETHERTYPE_IP;
      unsigned char *cp = NULL;
      size_t packet_len;

      /* Add ethernet framing. */

      /* Destination is virtual adapter's ethernet address. */
      memcpy(ether_hdr + SSH_ETHERH_OFS_DST, adapter->dev->dev_addr,
             SSH_ETHERH_ADDRLEN);

      /* Resolve packet's source and the ethernet type to use. */
      packet_len = ssh_interceptor_packet_len(pp);

      /* IPv4 */
      if (pp->protocol == SSH_PROTOCOL_IP4)
        {
          if (packet_len < SSH_IPH4_HDRLEN)
            {
	      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	      local_bh_enable();
              SSH_DEBUG(SSH_D_ERROR,
                        ("Packet is too short to contain IPv4 header"));
              goto error;
            }

	  /* Pullup requests data from the header of a writable skb. */
	  if (likely(skb_headlen(ipp->skb) >= SSH_IPH4_HDRLEN
		     && !skb_shared(ipp->skb) &&
		     SSH_SKB_WRITABLE(ipp->skb, SSH_IPH4_HDRLEN)))
	    cp = ipp->skb->data;

          if (cp == NULL)
	    {
	      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	      local_bh_enable();
	      goto error_already_freed;
	    }

          SSH_IPH4_SRC(&src, cp);
        }

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      /* IPv6 */
      else
        {
          if (packet_len < SSH_IPH6_HDRLEN)
            {
	      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	      local_bh_enable();
              SSH_DEBUG(SSH_D_ERROR,
                        ("Packet too short to contain IPv6 header"));
              goto error;
            }

	  if (likely(skb_headlen(ipp->skb) >= SSH_IPH6_HDRLEN
		     && !skb_shared(ipp->skb) &&
		     SSH_SKB_WRITABLE(ipp->skb, SSH_IPH6_HDRLEN)))
	    cp = ipp->skb->data;

          if (cp == NULL)
	    {
	      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	      local_bh_enable();
	      goto error_already_freed;
	    }

          SSH_IPH6_SRC(&src, cp);
          ethertype = SSH_ETHERTYPE_IPv6;
        }
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

      /* Finalize ethernet header. */
      ssh_virtual_adapter_ip_ether_address(&src,
                                           ether_hdr + SSH_ETHERH_OFS_SRC);
      SSH_PUT_16BIT(ether_hdr + SSH_ETHERH_OFS_TYPE, ethertype);

      /* Insert header to the packet. */
      cp = NULL;
      if (likely((skb_headroom(ipp->skb) >=
		  (SSH_ETHERH_HDRLEN + SSH_INTERCEPTOR_PACKET_HARD_HEAD_ROOM))
		 && !skb_shared(ipp->skb) && SSH_SKB_WRITABLE(ipp->skb, 0)))
	cp = skb_push(ipp->skb, SSH_ETHERH_HDRLEN);

      if (cp == NULL)
	{
	  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
	  goto error_already_freed;
	}
      memcpy(cp, ether_hdr, SSH_ETHERH_HDRLEN);

      /* Just to be pedantic. */
      pp->protocol = SSH_PROTOCOL_ETHERNET;
    }
  else
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      local_bh_enable();
      SSH_DEBUG(SSH_D_ERROR, ("Can not handle protocol %d", pp->protocol));
      goto error;
    }

  /* Tear off the internal packet from the generic SshInterceptorPacket. */
  skb = ipp->skb;
  ipp->skb = NULL;

  /* (re-)receive the packet via the interface; this should
     make the packet go back up the stack */
  skb->protocol = eth_type_trans(skb, adapter->dev);
  skb->dev = adapter->dev;

  /* Update per virtual adapter statistics. */
  stats = &adapter->low_level_stats;
  stats->rx_packets++;
  stats->rx_bytes += skb->len;

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
  local_bh_enable();

  /* Send the skb up towards stack. If it is IP (or ARP), it will be
     intercepted by ssh_interceptor_packet_in. */
  netif_rx(skb);

  /* Put the packet header on freelist. */
  ssh_interceptor_packet_free((SshInterceptorPacket) ipp);
  return;

 error:
  ssh_interceptor_packet_free(pp);

 error_already_freed:
  return;
}
