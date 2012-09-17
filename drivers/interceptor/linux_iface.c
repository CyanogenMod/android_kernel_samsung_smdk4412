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
 * linux_iface.c
 *
 * Linux interceptor network interface handling.
 *
 */

#include "linux_internal.h"

extern SshInterceptor ssh_interceptor_context;

/* Protect access to net_device list in kernel. */
#define SSH_LOCK_LINUX_DEV_LIST() read_lock(&dev_base_lock)
#define SSH_UNLOCK_LINUX_DEV_LIST() read_unlock(&dev_base_lock);

/**************************** Notification handlers *************************/

/* Interface notifier callback. This will notify when there has been a
   change in the interface list; device is added or removed, or an
   IPv4/IPv6 address has been added or removed from an device. */

int
ssh_interceptor_notifier_callback(struct notifier_block *block,
                                  unsigned long type, void *arg)
{
  if (ssh_interceptor_context != NULL)
    ssh_interceptor_receive_ifaces(ssh_interceptor_context);

  return NOTIFY_OK;
}

/* Internal mapping from ifnum to net_device.
   This function will assert that 'ifnum' is a valid
   SshInterceptorIfnum and that the corresponding net_device
   exists in the interface hashtable. This function will dev_hold()
   the net_device. The caller of this function must release it by
   calling ssh_interceptor_release_netdev(). If `context_return' is
   not NULL then this sets it to point to the interface context. */
inline struct net_device *
ssh_interceptor_ifnum_to_netdev_ctx(SshInterceptor interceptor,
				    SshUInt32 ifnum,
				    void **context_return)
{
  SshInterceptorInternalInterface iface;
  struct net_device *dev = NULL;

  SSH_LINUX_ASSERT_VALID_IFNUM(ifnum);

  read_lock(&interceptor->if_table_lock);
  for (iface = interceptor->if_hash[ifnum % SSH_LINUX_IFACE_HASH_SIZE];
       iface && iface->ifindex != ifnum;
       iface = iface->next)
    ;
  if (iface)
    {
      SSH_ASSERT(iface->generation == interceptor->if_generation);
      dev = iface->dev;
      SSH_ASSERT(dev != NULL);
      dev_hold(dev);
      if (context_return != NULL)
	*context_return = iface->context;
    }
  read_unlock(&interceptor->if_table_lock);

  return dev;
}

inline struct net_device *
ssh_interceptor_ifnum_to_netdev(SshInterceptor interceptor,
				SshUInt32 ifnum)
{
  return ssh_interceptor_ifnum_to_netdev_ctx(interceptor, ifnum, NULL);
}

/* Decrement the reference count of net_device 'dev'. */
inline void
ssh_interceptor_release_netdev(struct net_device *dev)
{
  SSH_ASSERT(dev != NULL);
  dev_put(dev);
}

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
/* Map Linux media types (actually extensions of ARP hardware
   address types) to platform independent denominations. */
static SshInterceptorMedia
ssh_interceptor_media_type(unsigned short type)
{
  switch (type)
    {
    case ARPHRD_ETHER:
      return SSH_INTERCEPTOR_MEDIA_ETHERNET;
    case ARPHRD_IEEE802:
      return SSH_INTERCEPTOR_MEDIA_FDDI;
    default:
      return SSH_INTERCEPTOR_MEDIA_PLAIN;
    }
  SSH_NOTREACHED;
  return SSH_INTERCEPTOR_MEDIA_PLAIN;
}
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */


/* Sends the updated interface information to the engine. This is called
   at the initialization of the interceptor and whenever the status of any
   interface changes.

   This function grabs 'if_table_lock' (for reading) and 'interceptor_lock'.
*/
static void
ssh_interceptor_send_interfaces(SshInterceptor interceptor)
{
  SshInterceptorInternalInterface iface;
  SshInterceptorInterface *ifarray;
  SshInterceptorInterfacesCB interfaces_callback;
  struct net_device *dev;
  struct in_device *inet_dev = NULL;
  struct in_ifaddr *addr;
  int num, n, count, i, hashvalue;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  struct inet6_dev *inet6_dev = NULL;
  struct inet6_ifaddr *addr6, *next_addr6;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  /* Grab 'if_table_lock' for reading the interface table. */
  read_lock(&interceptor->if_table_lock);

  num = interceptor->if_table_size;
  if (!num)
    {
      read_unlock(&interceptor->if_table_lock);
      SSH_DEBUG(4, ("No interfaces to report."));
      return;
    }

  /* Allocate temporary memory for the table that is
     passed to the engine. This is mallocated, since
     we want to minimise stack usage. */
  ifarray = ssh_malloc(sizeof(*ifarray) * num);
  if (ifarray == NULL)
    {
      read_unlock(&interceptor->if_table_lock);
      return;
    }
  memset(ifarray, 0, sizeof(*ifarray) * num);

  /* Iterate over the slots of the iface hashtable. */
  n = 0;
  for (hashvalue = 0; hashvalue < SSH_LINUX_IFACE_HASH_SIZE; hashvalue++)
    {

      /* Iterate over the chain of iface entries in a hashtable slot. */
      for (iface = interceptor->if_hash[hashvalue];
	   iface != NULL;
	   iface = iface->next)
	{
	  /* Ignore devices that are not up. */
	  dev = iface->dev;
	  if (dev == NULL || !(dev->flags & IFF_UP))
	    continue;

	  /* Disable net_device features that vpnclient does not support */
	  if (dev->features & NETIF_F_TSO)
	    dev->features &= ~NETIF_F_TSO;

#ifdef LINUX_HAS_NETIF_F_GSO
	  /* Disable net_device features that vpnclient does not support */
	  if (dev->features & NETIF_F_GSO)
	    {
	      SSH_DEBUG(2, ("Warning: Interface %lu [%s], dropping unsupported "
			    "feature NETIF_F_GSO",
			    (unsigned long) iface->ifindex,
			    (dev->name ? dev->name : "<none>")));
	      dev->features &= ~NETIF_F_GSO;
	    }
#endif /* LINUX_HAS_NETIF_F_GSO */

#ifdef LINUX_HAS_NETIF_F_TSO6
	  /* Disable net_device features that vpnclient does not support */
	  if (dev->features & NETIF_F_TSO6)
	    {
	      SSH_DEBUG(2, ("Warning: Interface %lu [%s], dropping unsupported "
			    "feature NETIF_F_TSO6",
			    (unsigned long) iface->ifindex,
			    (dev->name ? dev->name : "<none>")));
	      dev->features &= ~NETIF_F_TSO6;
	    }
#endif /* LINUX_HAS_NETIF_F_TSO6 */

#ifdef LINUX_HAS_NETIF_F_TSO_ECN
	  /* Disable net_device features that vpnclient does not support */
	  if (dev->features & NETIF_F_TSO_ECN)
	    {
	      SSH_DEBUG(2, ("Warning: Interface %lu [%s], dropping unsupported "
			    "feature NETIF_F_TSO_ECN",
			    (unsigned long) iface->ifindex,
			    (dev->name ? dev->name : "<none>")));
	      dev->features &= ~NETIF_F_TSO_ECN;
	    }
#endif /* LINUX_HAS_NETIF_F_TSO_ECN */

#ifdef LINUX_HAS_NETIF_F_GSO_ROBUST
	  /* Disable net_device features that vpnclient does not support */
	  if (dev->features & NETIF_F_GSO_ROBUST)
	    {
	      SSH_DEBUG(2, ("Warning: Interface %lu [%s], dropping unsupported "
			    "feature NETIF_F_GSO_ROBUST",
			    (unsigned long) iface->ifindex,
			    (dev->name ? dev->name : "<none>")));
	      dev->features &= ~NETIF_F_GSO_ROBUST;
	    }
#endif /* LINUX_HAS_NETIF_F_GSO_ROBUST */

#ifdef LINUX_HAS_NETIF_F_UFO
	  if (dev->features & NETIF_F_UFO)
	    {
	      SSH_DEBUG(2, ("Warning: Interface %lu [%s], dropping unsupported "
			    "feature NETIF_F_UFO",
			    (unsigned long) iface->ifindex,
			    (dev->name ? dev->name : "<none>")));
	      dev->features &= ~NETIF_F_UFO;
	    }
#endif /* LINUX_HAS_NETIF_F_UFO */

	  /* Count addresses */
	  count = 0;

	  /* Increment refcount to make sure the device does not disappear. */
	  inet_dev = in_dev_get(dev);
	  if (inet_dev)
	    {
	      /* Count the device's IPv4 addresses */
	      for (addr = inet_dev->ifa_list; addr != NULL;
		   addr = addr->ifa_next)
		{
		  count++;
		}
	    }

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	  /* Increment refcount to make sure the device does not disappear. */
	  inet6_dev = in6_dev_get(dev);
	  if (inet6_dev)
	    {
	      /* Count the device's IPv6 addresses */
	      SSH_INET6_IFADDR_LIST_FOR_EACH(addr6,
					     next_addr6,
					     inet6_dev->addr_list)
		{
		  count++;
		}
	    }
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

	  /* Fill interface entry. */
	  ifarray[n].ifnum = iface->ifindex;
	  ifarray[n].to_protocol.flags =
	    SSH_INTERCEPTOR_MEDIA_INFO_NO_FRAGMENT;
	  ifarray[n].to_protocol.mtu_ipv4 = dev->mtu;
	  ifarray[n].to_adapter.flags = 0;
	  ifarray[n].to_adapter.mtu_ipv4 = dev->mtu;

#ifdef WITH_IPV6
	  ifarray[n].to_adapter.mtu_ipv6 = dev->mtu;
	  ifarray[n].to_protocol.mtu_ipv6 = dev->mtu;
#endif /* WITH_IPV6 */

#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	  ifarray[n].to_adapter.media = ssh_interceptor_media_type(dev->type);
	  ifarray[n].to_protocol.media = ssh_interceptor_media_type(dev->type);
#else /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
	  ifarray[n].to_adapter.media = SSH_INTERCEPTOR_MEDIA_PLAIN;
	  ifarray[n].to_protocol.media = SSH_INTERCEPTOR_MEDIA_PLAIN;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	  strncpy(ifarray[n].name, dev->name, 15);

	  /* Set interface type and link status. */
	  if (dev->flags & IFF_POINTOPOINT)
	    ifarray[n].flags |= SSH_INTERFACE_FLAG_POINTOPOINT;
	  if (dev->flags & IFF_BROADCAST)
	    ifarray[n].flags |= SSH_INTERFACE_FLAG_BROADCAST;
	  if (!netif_carrier_ok(dev))
	    ifarray[n].flags |= SSH_INTERFACE_FLAG_LINK_DOWN;

	  ifarray[n].num_addrs = count;
	  ifarray[n].addrs = NULL;

	  /* Add addresses to interface entry. */
	  if (count > 0)
	    {
	      ifarray[n].addrs = ssh_malloc(sizeof(*ifarray[n].addrs) * count);

	      if (ifarray[n].addrs == NULL)
		{
		  /* Release INET/INET6 devices */
		  if (inet_dev)
		    in_dev_put(inet_dev);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
		  if (inet6_dev)
		    in6_dev_put(inet6_dev);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
		  read_unlock(&interceptor->if_table_lock);
		  goto out;
		}
	      count = 0;
	      if (inet_dev)
		{
		  /* Put the IPv4 addresses */
		  for (addr = inet_dev->ifa_list;
		       addr != NULL;
		       addr = addr->ifa_next)
		    {
		      ifarray[n].addrs[count].protocol = SSH_PROTOCOL_IP4;
		      SSH_IP4_DECODE(&ifarray[n].addrs[count].addr.ip.ip,
				     &addr->ifa_local);
		      SSH_IP4_DECODE(&ifarray[n].addrs[count].addr.ip.mask,
				     &addr->ifa_mask);

#if 0



		      if (dev->flags & IFF_POINTOPOINT)
			SSH_IP4_DECODE(&ifarray[n].addrs[count].
				       addr.ip.broadcast,
				       &addr->ifa_address);
		      else
#endif /* 0 */
		      if (dev->flags & IFF_BROADCAST)
			SSH_IP4_DECODE(&ifarray[n].addrs[count].
				       addr.ip.broadcast,
				       &addr->ifa_broadcast);
		      else
			SSH_IP_UNDEFINE(&ifarray[n].addrs[count].
					addr.ip.broadcast);
		      count++;
		    }
		}

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	      if (inet6_dev)
		{
		  /* Put the IPv6 addresses */
		  SSH_INET6_IFADDR_LIST_FOR_EACH(addr6,
						 next_addr6,
						 inet6_dev->addr_list)
		    {
		      ifarray[n].addrs[count].protocol = SSH_PROTOCOL_IP6;

		      SSH_IP6_DECODE(&ifarray[n].addrs[count].addr.ip.ip,
				     &addr6->addr);

		      /* Generate mask from prefix length and IPv6 addr */
		      SSH_IP6_DECODE(&ifarray[n].addrs[count].addr.ip.mask,
				     "\xff\xff\xff\xff\xff\xff\xff\xff"
				     "\xff\xff\xff\xff\xff\xff\xff\xff");
		      ssh_ipaddr_set_bits(&ifarray[n].addrs[count].
					  addr.ip.mask,
					  &ifarray[n].addrs[count].
					  addr.ip.mask,
					  addr6->prefix_len, 0);

		      /* Set the broadcast address to the IPv6
			 undefined address */
		      SSH_IP6_DECODE(&ifarray[n].addrs[count].
				     addr.ip.broadcast,
				     "\x00\x00\x00\x00\x00\x00\x00\x00"
				     "\x00\x00\x00\x00\x00\x00\x00\x00");

                      /* Copy the ifnum in case of ipv6 to scope_id, since
                         in linux scope_id == ifnum. */
                      ifarray[n].addrs[count].addr.ip.ip.
                        scope_id.scope_id_union.ui32 = ifarray[n].ifnum;
		      count++;
		    }
		}
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */
	    }
#ifndef SSH_IPSEC_IP_ONLY_INTERCEPTOR
	  /* Grab the MAC address */
	  ifarray[n].media_addr_len = dev->addr_len;
	  SSH_ASSERT(dev->addr_len <= sizeof(ifarray[n].media_addr));
	  memcpy(&ifarray[n].media_addr[0], dev->dev_addr, dev->addr_len);
#else /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */
	  ifarray[n].media_addr[0] = 0;
	  ifarray[n].media_addr_len = 0;
#endif /* !SSH_IPSEC_IP_ONLY_INTERCEPTOR */

	  /* Release INET/INET6 devices */
	  if (inet_dev)
	    in_dev_put(inet_dev);
	  inet_dev = NULL;
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
	  if (inet6_dev)
	    in6_dev_put(inet6_dev);
	  inet6_dev = NULL;
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

          ssh_kernel_mutex_lock(interceptor->interceptor_lock);
          for (i = 0; i < SSH_LINUX_MAX_VIRTUAL_ADAPTERS; i++)
            {
              SshVirtualAdapter adapter = interceptor->virtual_adapters[i];
              if (adapter && adapter->dev->ifindex == ifarray[n].ifnum)
                {
                  ifarray[n].flags |= SSH_INTERFACE_FLAG_VIP;
                  break;
                }
            }
          ssh_kernel_mutex_unlock(interceptor->interceptor_lock);

	  /* Done with the interface entry,
	     increase ifarray index and continue with next iface. */
	  n++;

	} /* for (iface entry chain iteration) */
    } /* for (hashtable iteration) */

  /* Release if_table lock. */
  read_unlock(&interceptor->if_table_lock);

  SSH_ASSERT(n <= num);

  /* 'interceptor_lock' protects 'num_interface_callbacks' and
     'interfaces_callback'. */
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  interceptor->num_interface_callbacks++;
  interfaces_callback = interceptor->interfaces_callback;
  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);

  /* Call the interface callback. */
  (interfaces_callback)(n, ifarray, interceptor->callback_context);

  ssh_kernel_mutex_lock(interceptor->interceptor_lock);
  interceptor->num_interface_callbacks--;
  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);

out:
  /* Free the array. */
  for (i = 0; i < n; i++)
    {
      if (ifarray[i].addrs != NULL)
        ssh_free(ifarray[i].addrs);
    }
  ssh_free(ifarray);

  return;
}

/* Caller must hold 'if_table_lock' for writing */
static SshInterceptorInternalInterface
ssh_interceptor_alloc_iface(SshInterceptor interceptor)
{
  SshInterceptorInternalInterface iface, new_table;
  SshUInt32 i, n, new_table_size, hashvalue;

  iface = NULL;

 relookup:
  /* Find empty slot in the if_table */
  for (i = 0; i < interceptor->if_table_size; i++)
    {
      iface = &interceptor->if_table[i];
      if (iface->generation == 0)
	{
#ifdef DEBUG_LIGHT
	  iface->dev = NULL;
	  iface->ifindex = SSH_INTERCEPTOR_INVALID_IFNUM;
	  iface->next = NULL;
#endif /* DEBUG_LIGHT */
	  iface->context = NULL;
	  return iface;
	}
    }

  /* No free slots, reallocate if_table */
  if (interceptor->if_table_size >= SSH_LINUX_IFACE_TABLE_SIZE)
    return NULL; /* Maximum table size reached. */

  new_table_size = interceptor->if_table_size + 10;
  if (new_table_size > SSH_LINUX_IFACE_TABLE_SIZE)
    new_table_size = SSH_LINUX_IFACE_TABLE_SIZE;

  new_table = ssh_malloc(sizeof(*new_table) * new_table_size);
  if (new_table == NULL)
    return NULL;

  /* Copy existing entries from if_hash */
  n = 0;
  for (i = 0; i < SSH_LINUX_IFACE_HASH_SIZE; i++)
    {
      for (iface = interceptor->if_hash[i]; iface; iface = iface->next)
	{
	  new_table[n].next = NULL;
	  new_table[n].ifindex = iface->ifindex;
	  new_table[n].dev = iface->dev;
	  new_table[n].generation = iface->generation;
	  n++;
	}
    }

  /* Initialize the rest of the new interface table */
  for (; n < new_table_size; n++)
    {
      new_table[n].generation = 0;
#ifdef DEBUG_LIGHT
      new_table[n].dev = NULL;
      new_table[n].ifindex = SSH_INTERCEPTOR_INVALID_IFNUM;
      new_table[n].next = NULL;
#endif /* DEBUG_LIGHT */
      new_table[n].context = NULL;
    }

  /* Rebuild hashtable */
  memset(interceptor->if_hash, 0, sizeof(interceptor->if_hash));
  for (i = 0; i < new_table_size; i++)
    {
      if (new_table[i].generation == 0)
	continue;

      hashvalue = new_table[i].ifindex % SSH_LINUX_IFACE_HASH_SIZE;
      new_table[i].next = interceptor->if_hash[hashvalue];
      interceptor->if_hash[hashvalue] = &new_table[i];
    }

  /* Free old if_table */
  ssh_free(interceptor->if_table);
  wmb();
  interceptor->if_table = new_table;
  interceptor->if_table_size = new_table_size;

  goto relookup;
}

/* The interceptor_update_interfaces() function traverses the kernels
   list of interfaces, grabs a refcnt for each one, and updates the
   interceptors 'ifnum->net_device' cache (optimizing away the need to
   grab a lock, traverse dev_base linked list, unlock, for each
   packet).

   This function grabs 'if_table_lock' (for writing) and dev_base lock. */
static void
ssh_interceptor_update_interfaces(SshInterceptor interceptor)
{
  SshInterceptorInternalInterface iface, iface_prev, iface_next;
  struct net_device *dev;
  SshUInt32 i, hashvalue, ifindex;

  /* WARNING: TWO LOCKS HELD AT THE SAME TIME. BE CAREFUL!
     dev_base_lock MUST be held to ensure integrity during traversal
     of list of interfaces in kernel. */
  SSH_LOCK_LINUX_DEV_LIST();

  /* Grab 'if_table_lock' for modifying the interface table. */
  write_lock(&interceptor->if_table_lock);

  /* Increment 'if_generation' */
  interceptor->if_generation++;
  if (interceptor->if_generation == 0)
    interceptor->if_generation++; /* Handle wrapping */

  /* Traverse net_device list, add new interfaces to hashtable,
     and mark existing entries up-to-date. */
  for (dev = SSH_FIRST_NET_DEVICE();
       dev != NULL;
       dev = SSH_NEXT_NET_DEVICE(dev))
    {
      ifindex = (SshUInt32) dev->ifindex;

      /* Ignore the loopback device. */
      if (dev->flags & IFF_LOOPBACK)
	continue;

      /* Ignore interfaces that collide with SSH_INTERCEPTOR_INVALID_IFNUM */
      if (ifindex == SSH_INTERCEPTOR_INVALID_IFNUM)
	{
	  SSH_DEBUG(2, ("Interface index collides with "
			"SSH_INTERCEPTOR_INVALID_IFNUM, "
			"ignoring interface %lu[%s]",
			(unsigned long) ifindex,
			(dev->name ? dev->name : "<none>")));
	  continue;
	}

      /* Assert that 'dev->ifindex' is otherwise valid. */
      SSH_LINUX_ASSERT_VALID_IFNUM(ifindex);

      /* Lookup interface from the hashtable. */
      for (iface =
	     interceptor->if_hash[ifindex % SSH_LINUX_IFACE_HASH_SIZE];
	   iface != NULL && iface->ifindex != ifindex;
	   iface = iface->next)
	;

      /* Interface found */
      if (iface)
	{
	  if (iface->dev == dev)
	    {
	      SSH_DEBUG(SSH_D_NICETOKNOW,
			("Old interface %lu[%s]",
			 (unsigned long) ifindex,
			 (dev->name ? dev->name : "<none>")));

	      /* Mark up-to-date. */
	      iface->generation = interceptor->if_generation;
	    }

	  /* Interface index matches, but net_device has changed. */
	  else
	    {
	      SSH_DEBUG(SSH_D_NICETOKNOW,
			("Changed interface %lu[%s] (from %lu[%s])",
			 (unsigned long) ifindex,
			 (dev->name ? dev->name : "<none>"),
			 (unsigned long) iface->ifindex,
			 (iface->dev->name ? iface->dev->name : "<none>")));

	      /* Release old net_device. */
	      SSH_ASSERT(iface->dev != NULL);
	      dev_put(iface->dev);
	      /* Hold new net_device. */
	      dev_hold(dev);
	      wmb(); /* Make sure assignments are not reordered. */
	      iface->dev = dev;
	      iface->ifindex = ifindex;
	      iface->context = NULL;
	      /* Mark up-to-date. */
	      iface->generation = interceptor->if_generation;
	    }
	}

      /* Interface not found */
      else
	{
	  SSH_DEBUG(SSH_D_NICETOKNOW,
		    ("New interface %lu[%s]",
		     (unsigned long) ifindex,
		     (dev->name ? dev->name : "<none>")));

	  /* Allocate new interface entry */
	  iface = ssh_interceptor_alloc_iface(interceptor);
	  if (iface)
	    {
	      /* Hold new net_device. */
	      dev_hold(dev);
	      /* Fill interface entry. */
	      iface->ifindex = ifindex;
	      iface->dev = dev;
	      iface->context = NULL;
	      /* Mark up-to-date */
	      iface->generation = interceptor->if_generation;
	      /* Add entry to hashtable. */
	      hashvalue = iface->ifindex % SSH_LINUX_IFACE_HASH_SIZE;
	      iface->next = interceptor->if_hash[hashvalue];
	      wmb();
	      interceptor->if_hash[hashvalue] = iface;
	    }
	  else
	    {
	      SSH_DEBUG(SSH_D_FAIL,
			("Could not allocate memory for new interface %lu[%s]",
			 (unsigned long) ifindex,
			 (dev->name ? dev->name : "<none>")));
	    }
	}
    }

  /* Remove old interfaces from the table */
  for (i = 0; i < SSH_LINUX_IFACE_HASH_SIZE; i++)
    {
      iface_prev = NULL;
      for (iface = interceptor->if_hash[i];
	   iface != NULL;
	   iface = iface_next)
	{
	  if (iface->generation != 0 &&
	      iface->generation != interceptor->if_generation)
	    {
	      SSH_DEBUG(SSH_D_NICETOKNOW,
			("Disappeared interface %lu[%s]",
			 (unsigned long) iface->ifindex,
			 (iface->dev->name ? iface->dev->name : "<none>")));

	      /* Release old netdevice */
	      SSH_ASSERT(iface->dev != NULL);
	      dev_put(iface->dev);

#ifdef DEBUG_LIGHT
	      wmb();
	      iface->dev = NULL;
	      iface->ifindex = SSH_INTERCEPTOR_INVALID_IFNUM;
#endif /* DEBUG_LIGHT */

	      /* Mark entry freed. */
	      iface->generation = 0;

	      /* Remove entry from hashtable. */
	      if (iface_prev)
		iface_prev->next = iface->next;
	      else
		interceptor->if_hash[i] = iface->next;
	      iface_next = iface->next;
	      iface->next = NULL;
	    }
	  else
	    {
	      iface_prev = iface;
	      iface_next = iface->next;
	    }
	}
    }
  /* Unlock if_table_lock */
  write_unlock(&interceptor->if_table_lock);

  /* Release dev_base_lock. */
  SSH_UNLOCK_LINUX_DEV_LIST();

  /* Notify changes to engine */
  if (interceptor->engine != NULL && interceptor->engine_open == TRUE)
    {
      local_bh_disable();
      ssh_interceptor_send_interfaces(interceptor);
      local_bh_enable();
    }

  return;
}


/* Callback for interface and address notifier. */
void
ssh_interceptor_receive_ifaces(SshInterceptor interceptor)
{
  local_bh_disable();
  ssh_interceptor_update_interfaces(interceptor);
  local_bh_enable();

  return;
}


/* Release all refcounts and free the 'ifnum->net_device' map cache. */
void
ssh_interceptor_clear_ifaces(SshInterceptor interceptor)
{
  int i;
  SshInterceptorInternalInterface iface;

  /* At this point. All hooks must have completed running! */

  SSH_LOCK_LINUX_DEV_LIST();
  write_lock(&interceptor->if_table_lock);

  /* Release net_devices and free if_table. */
  if (interceptor->if_table != NULL)
    {
      for (i = 0; i < interceptor->if_table_size; i++)
        {
	  iface = &interceptor->if_table[i];
          if (iface->generation != 0)
            {
	      SSH_ASSERT(iface->dev != NULL);
	      dev_put(iface->dev);
#ifdef DEBUG_LIGHT
	      wmb();
	      iface->dev = NULL;
#endif /* DEBUG_LIGHT */
	      iface->generation = 0;
            }
        }

      ssh_free(interceptor->if_table);
      interceptor->if_table = NULL;
    }

  /* Clear interface hashtable. */
  memset(interceptor->if_hash, 0, sizeof(interceptor->if_hash));

  write_unlock(&interceptor->if_table_lock);
  SSH_UNLOCK_LINUX_DEV_LIST();
}


/*************************** Module init / uninit **************************/

Boolean ssh_interceptor_iface_init(SshInterceptor interceptor)
{
  /* This will register notifier that notifies about bringing the
     interface up and down. */
  interceptor->notifier_netdev.notifier_call =
    ssh_interceptor_notifier_callback;
  interceptor->notifier_netdev.priority = 1;
  interceptor->notifier_netdev.next = NULL;
  register_netdevice_notifier(&interceptor->notifier_netdev);

  /* This will register notifier that notifies when address of the
     interface changes without bringing the interface down. */
  interceptor->notifier_inetaddr.notifier_call =
    ssh_interceptor_notifier_callback;
  interceptor->notifier_inetaddr.priority = 1;
  interceptor->notifier_inetaddr.next = NULL;
  register_inetaddr_notifier(&interceptor->notifier_inetaddr);

#ifdef SSH_LINUX_INTERCEPTOR_IPV6
  interceptor->notifier_inet6addr.notifier_call =
    ssh_interceptor_notifier_callback;
  interceptor->notifier_inet6addr.priority = 1;
  interceptor->notifier_inet6addr.next = NULL;
  register_inet6addr_notifier(&interceptor->notifier_inet6addr);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

  interceptor->iface_notifiers_installed = TRUE;

  /* Send interface information to engine. This causes the interceptor
     to grab reference to each net_device. On error cases
     ssh_interceptor_clear_ifaces() or ssh_interceptor_iface_uninit()
     must be called to release the references. */
  ssh_interceptor_receive_ifaces(interceptor);

  return TRUE;
}

void ssh_interceptor_iface_uninit(SshInterceptor interceptor)
{
  if (interceptor->iface_notifiers_installed)
    {
      local_bh_enable();

      /* Unregister notifier callback */
      unregister_netdevice_notifier(&interceptor->notifier_netdev);
      unregister_inetaddr_notifier(&interceptor->notifier_inetaddr);
#ifdef SSH_LINUX_INTERCEPTOR_IPV6
      unregister_inet6addr_notifier(&interceptor->notifier_inet6addr);
#endif /* SSH_LINUX_INTERCEPTOR_IPV6 */

      local_bh_disable();
    }

  /* Clear interface table and release net_device references. */
  ssh_interceptor_clear_ifaces(interceptor);

  /* Due to lack of proper locking in linux kernel notifier code,
     the unregister_*_notifier functions might leave the notifier
     blocks out of sync, resulting in that the kernel may call an
     unregistered notifier function.

     Sleep for a while to decrease the possibility of the bug causing
     trouble (crash during module unloading). */
  mdelay(500);

  interceptor->iface_notifiers_installed = FALSE;
}
