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
 * linux_main.c
 *
 * Linux interceptor kernel module main.
 *
 */

#include "linux_internal.h"
#include "sshinet.h"

#include <linux/kernel.h>

#ifdef DEBUG_LIGHT
unsigned int ssh_debug_level = 0;
MODULE_PARM_DESC(ssh_debug_level, "Debug level");
module_param(ssh_debug_level, uint, 0444);
#endif /* DEBUG_LIGHT */

/* Global interceptor object */
SshInterceptor ssh_interceptor_context = NULL;

/* Preallocated interceptor object */
static SshInterceptorStruct interceptor_struct;

/******************************** Utility functions *************************/

void
ssh_interceptor_notify_ipm_open(SshInterceptor interceptor)
{
  local_bh_disable();

  /* Tell engine the PM connection is open. */
  ssh_engine_notify_ipm_open(interceptor->engine);

  local_bh_enable();
}

void
ssh_interceptor_notify_ipm_close(SshInterceptor interceptor)
{
  local_bh_disable();

  /* Tell engine the PM connection is closed. */
  ssh_engine_notify_ipm_close(interceptor->engine);

  /* Disable packet interception now that ipm has disconnected. */
  interceptor->enable_interception = FALSE;
  ssh_interceptor_dst_entry_cache_flush(interceptor);

  local_bh_enable();
}

/******************************* Interceptor API ****************************/

/* Opens the packet interceptor.  This must be called before using any
   other interceptor functions.  This registers the callbacks that the
   interceptor will use to notify the higher levels of received packets
   or changes in the interface list.  The interface callback will be called
   once either during this call or soon after this has returned.
   The `packet_cb' callback will be called whenever a packet is received
   from either a network adapter or a protocol stack.  It is guaranteed that
   this will not be called until from the bottom of the event loop after the
   open call has returned.

   The `interfaces_cb' callback will be called once soon after opening the
   interceptor, however earliest from the bottom of the event loop after the
   open call has returned.  From then on, it will be called whenever there is
   a change in the interface list (e.g., the IP address of an interface is
   changed, or a PPP interface goes up or down).

   The `callback_context' argument is passed to the callbacks. */

Boolean
ssh_interceptor_open(void *machine_context,
                     SshInterceptorPacketCB packet_cb,
                     SshInterceptorInterfacesCB interfaces_cb,
                     SshInterceptorRouteChangeCB route_cb,
                     void *callback_context,
                     SshInterceptor * interceptor_return)
{
  SshInterceptor interceptor;

  interceptor = ssh_interceptor_context;
  if (interceptor->engine_open)
    return FALSE;

  local_bh_disable();

  SSH_DEBUG(2, ("interceptor opened"));

  interceptor->engine_open = TRUE;
  interceptor->packet_callback = packet_cb;
  interceptor->interfaces_callback = interfaces_cb;
  interceptor->route_callback = route_cb;
  interceptor->callback_context = callback_context;

  /* Return the global interceptor object. */
  *interceptor_return = (void *) interceptor;

  local_bh_enable();
  return TRUE;
}

/* Closes the packet interceptor.  No more packet or interface callbacks
   will be received from the interceptor after this returns.  Destructors
   may still get called even after this has returned.

   It is illegal to call any packet interceptor functions (other than
   ssh_interceptor_open) after this call.  It is, however, legal to call
   destructors for any previously returned packets even after calling this.
   Destructors for any packets previously supplied to one of the send
   functions will get called before this function returns. */

void
ssh_interceptor_close(SshInterceptor interceptor)
{
  /* all closing is done in ssh_interceptor_uninit() */
  interceptor->engine_open = FALSE;
  return;
}

/* Dummy function callback after interceptor has been stopped */
static void
ssh_interceptor_dummy_interface_cb(SshUInt32 num_interfaces,
                                   SshInterceptorInterface *ifs,
                                   void *context)
{
  /* Do nothing */
  return;
}

/* Dummy function which packets get routed to after ssh_interceptor_stop()
   has been called. */
static void
ssh_interceptor_dummy_packet_cb(SshInterceptorPacket pp, void *ctx)
{
  ssh_interceptor_packet_free(pp);
}

/* Stops the packet interceptor.  After this call has returned, no new
   calls to the packet and interfaces callbacks will be made.  The
   interceptor keeps track of how many threads are processing packet,
   interface, or have pending route callbacks, and this function
   returns TRUE if there are no callbacks/pending calls to those functions.
   This returns FALSE if threads are still executing in those callbacks
   or routing callbacks are pending.

   After calling this function, the higher-level code should wait for
   packet processing to continue, free all packet structures received
   from that interceptor, and then close ssh_interceptor_close.  It is
   not an error to call this multiple times (the latter calls are
   ignored). */

Boolean
ssh_interceptor_stop(SshInterceptor interceptor)
{
  SSH_DEBUG(2, ("interceptor stopping"));

  /* 'interceptor_lock protects the 'interfaces_callback'
     and 'num_interface_callbacks'. */
  ssh_kernel_mutex_lock(interceptor->interceptor_lock);

  if (interceptor->num_interface_callbacks)
    {
      ssh_kernel_mutex_unlock(interceptor->interceptor_lock);
      SSH_DEBUG(SSH_D_ERROR,
                ("%d interface callbacks pending, can't stop",
                 interceptor->num_interface_callbacks));
      return FALSE;
    }

  /* No more interfaces are delivered to the engine after this. */
  interceptor->interfaces_callback = ssh_interceptor_dummy_interface_cb;

  /* Route callback is currently not used. */
  interceptor->route_callback = NULL_FNPTR;

  ssh_kernel_mutex_unlock(interceptor->interceptor_lock);

  /* After this the engine will receive no more packets from
     the interceptor, although the netfilter hooks are still
     installed. */

  /* Set packet_callback to point to our dummy_db */
  rcu_assign_pointer(interceptor->packet_callback,
		     ssh_interceptor_dummy_packet_cb);

  /* Wait for state synchronization. */
  local_bh_enable();
  synchronize_rcu();
  local_bh_disable();

  /* Callback context can now be safely zeroed, as both
     the interface_callback and the packet_callback point to
     our dummy_cb, and all kernel threads have returned from
     the engine. */
  interceptor->callback_context = NULL;

  SSH_DEBUG(2, ("interceptor stopped"));

  return TRUE;
}

/************** Interceptor uninitialization break-down. *******************/

static void
ssh_interceptor_uninit_external_interfaces(SshInterceptor interceptor)
{
  ssh_interceptor_virtual_adapter_uninit(interceptor);

  /* Remove netfilter hooks */
  ssh_interceptor_ip_glue_uninit(interceptor);
}

static void
ssh_interceptor_uninit_engine(SshInterceptor interceptor)
{
  /* Stop packet processing engine */
  if (interceptor->engine != NULL)
    {
      while (ssh_engine_stop(interceptor->engine) == FALSE)
	{
	  local_bh_enable();
	  schedule();
	  mdelay(300);
	  local_bh_disable();
	}
      interceptor->engine = NULL;
    }

  /* Free packet data structure */
  ssh_interceptor_packet_freelist_uninit(interceptor);
}

static void
ssh_interceptor_uninit_kernel_services(void)
{
  /* Remove interface event handlers and free interface table. */
  ssh_interceptor_iface_uninit(ssh_interceptor_context);

  ssh_interceptor_dst_entry_cache_uninit(ssh_interceptor_context);

  /* Uninitialize ipm channel */
  ssh_interceptor_ipm_uninit(ssh_interceptor_context);

  /* Free locks */
  ssh_kernel_mutex_free(ssh_interceptor_context->interceptor_lock);
  ssh_interceptor_context->interceptor_lock = NULL;
  ssh_kernel_mutex_free(ssh_interceptor_context->packet_lock);
  ssh_interceptor_context->packet_lock = NULL;

  ssh_interceptor_context = NULL;
}

/* Interceptor uninitialization. Called by cleanup_module() with
   softirqs disabled. */
static int
ssh_interceptor_uninit(void)
{
  /* Uninitialize external interfaces. We leave softirqs enabled for
     this as we have to make calls into the netfilter API that will
     execute scheduling in Linux 2.6. */
  ssh_interceptor_uninit_external_interfaces(ssh_interceptor_context);

  /* Uninitialize engine. Via ssh_interceptor_stop() this
     function makes sure that no callouts to the interceptor
     are in progress after it returns. ssh_interceptor_stop()
     _WILL_ grab the interceptor_lock, so make sure that it
     is not held.*/
  local_bh_disable();
  ssh_interceptor_uninit_engine(ssh_interceptor_context);

  /* Uninitialize basic kernel services to the engine and the
     interceptor. This frees all remaining memory. Note that all locks
     are also freed here, so none of them can be held. */
  ssh_interceptor_uninit_kernel_services();
  local_bh_enable();

  return 0;
}


/************** Interceptor initialization break-down. *********************/


int
ssh_interceptor_init_kernel_services(void)
{
  /* Interceptor object is always preallocated. */
  SSH_ASSERT(ssh_interceptor_context == NULL);
  memset(&interceptor_struct, 0, sizeof(interceptor_struct));
  ssh_interceptor_context = &interceptor_struct;

#ifdef DEBUG_LIGHT
  spin_lock_init(&ssh_interceptor_context->statistics_lock);
#endif /* DEBUG_LIGHT */

  /* General init */
  ssh_interceptor_context->interceptor_lock = ssh_kernel_mutex_alloc();
  ssh_interceptor_context->packet_lock = ssh_kernel_mutex_alloc();

  if (ssh_interceptor_context->interceptor_lock == NULL
      || ssh_interceptor_context->packet_lock == NULL)
    goto error;

  rwlock_init(&ssh_interceptor_context->if_table_lock);

  /* Init packet data structure */
  if (!ssh_interceptor_packet_freelist_init(ssh_interceptor_context))
    {
      printk(KERN_ERR
             "VPNClient packet processing engine failed to start "
             "(out of memory).\n");
      goto error;
    }

  if (ssh_interceptor_dst_entry_cache_init(ssh_interceptor_context) == FALSE)
    {
      printk(KERN_ERR "VPNClient packet processing engine "
	     "failed to start, dst cache initialization failed.");
      goto error;
    }

  /* Initialize ipm channel */
  if (!ssh_interceptor_ipm_init(ssh_interceptor_context))
    {
      printk(KERN_ERR
	     "VPNClient packet processing engine failed to start "
	     "(proc filesystem initialization error)\n");
      goto error1;
    }

  return 0;

 error1:
  local_bh_disable();
  ssh_interceptor_packet_freelist_uninit(ssh_interceptor_context);
  local_bh_enable();

 error:
  ssh_interceptor_dst_entry_cache_uninit(ssh_interceptor_context);
  ssh_kernel_mutex_free(ssh_interceptor_context->interceptor_lock);
  ssh_interceptor_context->interceptor_lock = NULL;

  ssh_kernel_mutex_free(ssh_interceptor_context->packet_lock);
  ssh_interceptor_context->packet_lock = NULL;

  ssh_interceptor_context = NULL;

  return -ENOMEM;
}

int
ssh_interceptor_init_external_interfaces(SshInterceptor interceptor)
{
  /* Register interface notifiers. */
  if (!ssh_interceptor_iface_init(interceptor))
    {
      printk(KERN_ERR
             "VPNClient packet processing engine failed to start "
             "(interface notifier installation error).\n");
      goto error0;
    }

  /* Register the firewall hooks. */
  if (!ssh_interceptor_ip_glue_init(interceptor))
    {
      printk(KERN_ERR
             "VPNClient packet processing engine failed to start "
             "(firewall glue installation error).\n");
      goto error1;
    }

  return 0;

 error1:

  local_bh_disable();
  ssh_interceptor_iface_uninit(interceptor);
  local_bh_enable();
 error0:

  return -EBUSY;
}

int
ssh_interceptor_init_engine(SshInterceptor interceptor)
{
  int start_cnt;

  /* Initialize the IPsec engine */

  interceptor->engine = NULL;
  for (start_cnt = 0;
       start_cnt < 3 && interceptor->engine == NULL;
       start_cnt++)
    {
      /* In theory, it would be nice and proper to disable softirqs
	 here and enable them after we exit engine_start(), but then
	 we could not allocate memory without GFP_ATOMIC in the
	 engine initialization, which would not be nice. Therefore
	 we leave softirqs open here, and disable them for the
	 duration of ssh_interceptor_open(). */
      interceptor->engine = ssh_engine_start(ssh_interceptor_send_to_ipm,
					     interceptor,
					     SSH_LINUX_ENGINE_FLAGS);
      if (interceptor->engine == NULL)
	{
	  schedule();
	  mdelay(500);
	}
    }

  if (interceptor->engine == NULL)
    {
      printk(KERN_ERR
	     "VPNClient packet processing engine failed to start "
	     "(engine start error).\n");
      goto error;
    }

  return 0;

 error:
  if (interceptor->engine != NULL)
    {
      local_bh_disable();
      while (ssh_engine_stop(interceptor->engine) == FALSE)
	{
	  local_bh_enable();
	  schedule();
	  mdelay(300);
	  local_bh_disable();
	}
      local_bh_enable();
      interceptor->engine = NULL;
    }

  return -EBUSY;
}

/* Interceptor initialization. Called by init_module(). */
int ssh_interceptor_init(void)
{
  int ret;

  /* Print version info for log files */
  printk(KERN_INFO "VPNClient built on " __DATE__ " " __TIME__ "\n");

  ret = ssh_interceptor_init_kernel_services();
  if (ret != 0)
    goto error0;

  SSH_ASSERT(ssh_interceptor_context != NULL);

  ret = ssh_interceptor_hook_magic_init();
  if (ret != 0)
    goto error1;

  ret = ssh_interceptor_virtual_adapter_init(ssh_interceptor_context);
  if (ret != 0)
    goto error1;

  ret = ssh_interceptor_init_engine(ssh_interceptor_context);
  if (ret != 0)
    goto error4;

  ret = ssh_interceptor_init_external_interfaces(ssh_interceptor_context);
  if (ret != 0)
    goto error5;

  return 0;

 error5:
  local_bh_disable();
  ssh_interceptor_uninit_engine(ssh_interceptor_context);
  local_bh_enable();

 error4:
  ssh_interceptor_clear_ifaces(ssh_interceptor_context);
  ssh_interceptor_virtual_adapter_uninit(ssh_interceptor_context);

 error1:
  local_bh_disable();
  ssh_interceptor_uninit_kernel_services();
  local_bh_enable();

 error0:
  return ret;
}

MODULE_DESCRIPTION(SSH_LINUX_INTERCEPTOR_MODULE_DESCRIPTION);

int __init ssh_init_module(void)
{
  if (ssh_interceptor_init() != 0)
    return -EIO;
  return 0;
}

void __exit ssh_cleanup_module(void)
{
  if (ssh_interceptor_uninit() != 0)
    {
      printk("ssh_interceptor: module can't be removed.");
      return;
    }
}

void ssh_linux_module_dec_use_count()
{
  module_put(THIS_MODULE);
}

int
ssh_linux_module_inc_use_count()
{
  return try_module_get(THIS_MODULE);
}

MODULE_LICENSE("GPL");
module_init(ssh_init_module);
module_exit(ssh_cleanup_module);
