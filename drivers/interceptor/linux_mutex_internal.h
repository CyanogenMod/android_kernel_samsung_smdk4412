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
 * linux_mutex_internal.h
 *
 * Linux interceptor internal defines for kernel mutex API.
 *
 */

#ifndef LINUX_MUTEX_INTERNAL_H
#define LINUX_MUTEX_INTERNAL_H

#include <linux/spinlock.h>
#include <asm/current.h>

typedef struct SshKernelMutexRec
{
  spinlock_t lock;
  unsigned long flags;

#ifdef DEBUG_LIGHT
  Boolean taken;
  unsigned long jiffies;
#endif
} SshKernelMutexStruct;

#ifdef CONFIG_PREEMPT

#include <linux/preempt.h>

#define icept_preempt_enable()  preempt_enable()
#define icept_preempt_disable() preempt_disable()

#else /* CONFIG_PREEMPT */

#define icept_preempt_enable()  do {;} while(0)
#define icept_preempt_disable() do {;} while(0)

#endif /* CONFIG_PREEMPT */

#endif /* LINUX_MUTEX_INTERNAL_H */
