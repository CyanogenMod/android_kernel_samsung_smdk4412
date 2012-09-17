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
 * kernel_mutex.h
 *
 * Kernel mutex API.
 *
 */

#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H

typedef struct SshKernelMutexRec *SshKernelMutex;

/* Allocates and initializes a simple mutex.  This should be as fast as
   possible, but work between different processors in a multiprocessor
   machine. Also, it is a fatal error for a thread to attempt to lock
   this twice (i.e., this need not check whether it is actually held
   by the same thread).  The recommended implementation is a spinlock. */
SshKernelMutex ssh_kernel_mutex_alloc(void);

/* Initializes a mutex allocated from the stack. Returns TRUE on success
   and FALSE on failure. */
Boolean ssh_kernel_mutex_init(SshKernelMutex mutex);

/* Frees the given mutex.  The mutex must not be locked when it is freed. */
void ssh_kernel_mutex_free(SshKernelMutex mutex);

/* Uninitializes the given mutex.  The mutex must not be locked when it is
   uninitialized. */
void ssh_kernel_mutex_uninit(SshKernelMutex mutex);

/* Locks the mutex. Only one thread of execution can have a mutex locked
   at a time.  This will block until execution can continue.  One should
   not keep mutexes locked for extended periods of time. */
void ssh_kernel_mutex_lock(SshKernelMutex mutex);

/* Unlocks the mutex. The mutex must be unlocked from the same thread
   from which it was locked. If other threads are waiting to lock the mutex,
   one of them will get the lock and continue execution. */
void ssh_kernel_mutex_unlock(SshKernelMutex mutex);

#ifdef DEBUG_LIGHT
/* Check that the mutex is locked.  It is a fatal error if it is not. */
void ssh_kernel_mutex_assert_is_locked(SshKernelMutex mutex);
#else /* DEBUG_LIGHT */
#define ssh_kernel_mutex_assert_is_locked(mutex)
#endif /* DEBUG_LIGHT */

#ifdef DEBUG_LIGHT
#define KERNEL_MUTEX_USE_FUNCTIONS
#endif /* DEBUG_LIGHT */

/* This must be in the -I path of the machine-dependent interceptor
   dir. It defines any platform-dependent things (such as the inline
   functions, if KERNEL_MUTEX_USE_FUNCTIONS is not defined). */
#include "platform_kernel_mutex.h"

#endif /* KERNEL_MUTEX_H */
