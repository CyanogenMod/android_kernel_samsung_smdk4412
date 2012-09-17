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
 * linux_mutex.c
 *
 * Linux interceptor kernel mutex API implementation.
 *
 */

#include "linux_internal.h"
#include "linux_mutex_internal.h"

extern SshInterceptor ssh_interceptor_context;

Boolean ssh_kernel_mutex_init(SshKernelMutex mutex)
{
  spin_lock_init(&mutex->lock);

#ifdef DEBUG_LIGHT
  mutex->taken = FALSE;
  mutex->jiffies = 0;
#endif
  return TRUE;
}

/* Allocates a simple mutex.  This should be as fast as possible, but work
   between different processors in a multiprocessor machine.  This need
   not work between different independent processes. */

SshKernelMutex
ssh_kernel_mutex_alloc(void)
{
  SshKernelMutex m;

  m = ssh_calloc(1, sizeof(struct SshKernelMutexRec));
  if (m == NULL)
    return NULL;

  if (!ssh_kernel_mutex_init(m))
    {
      ssh_free(m);
      m = NULL;
    }
  return m;
}

/* Frees the given mutex.  The mutex must not be locked when it is
   freed. */

void ssh_kernel_mutex_uninit(SshKernelMutex mutex)
{
  SSH_ASSERT(!mutex->taken);
}

void ssh_kernel_mutex_free(SshKernelMutex mutex)
{
  if (mutex)
    {
      ssh_kernel_mutex_uninit(mutex);
      ssh_free(mutex);
    }
}

#ifdef KERNEL_MUTEX_USE_FUNCTIONS
/* Locks the mutex.  Only one thread of execution can have a mutex locked
   at a time.  This will block until execution can continue.  One should
   not keep mutexes locked for extended periods of time. */
void
ssh_kernel_mutex_lock_i(SshKernelMutex mutex)
{
  SSH_LINUX_STATISTICS(ssh_interceptor_context,
  { ssh_interceptor_context->stats.num_light_locks++; });

  spin_lock(&mutex->lock);

  SSH_ASSERT(!mutex->taken);

#ifdef DEBUG_LIGHT
  mutex->taken = TRUE;
  mutex->jiffies = jiffies;
#endif /* DEBUG_LIGHT */
}

/* Unlocks the mutex.  If other threads are waiting to lock the mutex,
   one of them will get the lock and continue execution. */

void
ssh_kernel_mutex_unlock_i(SshKernelMutex mutex)
{
  SSH_ASSERT(mutex->taken);
#ifdef DEBUG_LIGHT
  mutex->taken = FALSE;
#endif /* DEBUG_LIGHT */

  spin_unlock(&mutex->lock);
}
#endif /* KERNEL_MUTEX_USE_FUNCTIONS */

#ifdef DEBUG_LIGHT
/* Check that the mutex is locked.  It is a fatal error if it is not. */

void
ssh_kernel_mutex_assert_is_locked(SshKernelMutex mutex)
{
  SSH_ASSERT(mutex->taken);
}
#endif /* DEBUG_LIGHT */
