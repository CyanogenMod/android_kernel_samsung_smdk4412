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
 * linux_kernel_alloc.c
 *
 * Linux interceptor implementation of kernel memory allocation API.
 *
 */
#include "linux_internal.h"
#include "kernel_alloc.h"

/* Malloc overhead:
   <uint32> size  (highest bit used for vmalloc indication)
   <uint32> magic
*/
#define SSH_MALLOC_OVERHEAD    (sizeof(SshUInt32) * 2)

#define SSH_MALLOC_FLAG_LARGE  (1 << 31)
#define SSH_MALLOC_MAGIC       0x32085242
#define SSH_MALLOC_MAGIC_FREED 0x00420042

/* The maximum allocation size of kmalloc is 128K, however because of
   memory fragmentation it's still insecure to allocate areas of many
   pages with kmalloc. So for areas bigger than one page (usually 4K)
   vmalloc() should be used. */
#define SSH_NORMAL_MALLOC_MAX  (128 << 10)

/* The maximum allocation size of __get_free_pages is 2M, that is the
   maximum order of alloc_pages() is 9 (2^9 of 4K pages), sometimes
   erroneous code can cause arbitarily long allocations so this is
   good sanity check. In general, allocations should be fairly
   small. */
#define SSH_VMALLOC_MAX        (2 << 20)

/* When do we shift to using vmalloc? In truth this should be ~equal
   to normal-alloc-max. */
#define SSH_VMALLOC_THRESHOLD  (SSH_NORMAL_MALLOC_MAX)

#ifdef DEBUG_LIGHT
extern SshInterceptor ssh_interceptor_context;
#endif /* DEBUG_LIGHT */


void *
ssh_kernel_alloc(size_t size, SshUInt32 flag)
{
  void *ptr;
  int malloc_flag;
  int total_size = size + SSH_MALLOC_OVERHEAD;
  Boolean is_vmalloc = FALSE;

  if (flag & SSH_KERNEL_ALLOC_WAIT)
    malloc_flag = GFP_KERNEL;
  else
    {
      SSH_ASSERT((flag & SSH_KERNEL_ALLOC_NOWAIT) ==
                 SSH_KERNEL_ALLOC_NOWAIT);
      malloc_flag = GFP_ATOMIC;
    }
  if (flag & SSH_KERNEL_ALLOC_DMA)
    {
      SSH_ASSERT(malloc_flag == GFP_ATOMIC);
      malloc_flag |= GFP_DMA;

      if (total_size > SSH_NORMAL_MALLOC_MAX)
	return NULL;
    }
  else
    if (total_size > SSH_VMALLOC_THRESHOLD)
      {
       if (total_size > SSH_VMALLOC_MAX)
       return NULL;

        is_vmalloc = TRUE;
      }
  if (is_vmalloc)
    {
      unsigned long order;

      /* Round up to nearest page size. The pages allocated with
	 __get_free_pages() are guaranteed to be contiguous in memory,
	 but you get one only if the requested amount of contiguous
	 pages are available. Linux doesn't currently have any
	 mechanisms for smart defragmentation of physical memory and
	 even if we had them they couldn't be fully reliable. So be
	 careful what you ask for, if you ask for too large a chunk of
	 memory you might end up not getting any at all. */
      total_size = ((total_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
      for (order = 0; ((1 << order) * PAGE_SIZE) < total_size; order++);

      if (order >= MAX_ORDER)
	return NULL;

      ptr = (char *)__get_free_pages(malloc_flag, order);
    }
  else
    ptr = kmalloc(total_size, malloc_flag);

  if (ptr == NULL)
    return NULL;

  /* Log all allocs to first entry */
  SSH_LINUX_STATISTICS(ssh_interceptor_context,
  {
    ssh_interceptor_context->stats.allocated_memory += (SshUInt64) total_size;

    if (ssh_interceptor_context->stats.allocated_memory >
	ssh_interceptor_context->stats.allocated_memory_max)
      ssh_interceptor_context->stats.allocated_memory_max =
	(SshUInt64) ssh_interceptor_context->stats.allocated_memory;

    if (is_vmalloc)
      ssh_interceptor_context->stats.num_allocations_large++;
    else
      ssh_interceptor_context->stats.num_allocations++;

    ssh_interceptor_context->stats.num_allocations_total++;
  });

  /* Fill in the size and magic */
  ((SshUInt32 *) ptr)[0] = total_size
    | (is_vmalloc ? SSH_MALLOC_FLAG_LARGE : 0);

#ifdef DEBUG_LIGHT
  ((SshUInt32 *) ptr)[1] = SSH_MALLOC_MAGIC;
#endif /* DEBUG_LIGHT */

  return ((char *) ptr) + SSH_MALLOC_OVERHEAD;
}

void
ssh_kernel_free(void *ptr)
{
  Boolean is_vmalloc;
  SshUInt32 size;

#ifdef DEBUG_LIGHT
  /* Sanity check */
  if (((SshUInt32 *) ptr)[-1] != SSH_MALLOC_MAGIC)
    SSH_FATAL("ssh_kernel_free: Malloc object corrupted (invalid magic %x)",
              ((unsigned int *) ptr)[-1]);

  /* Set this to distinctive value to make sure we detect double free. */
  ((SshUInt32 *) ptr)[-1] = SSH_MALLOC_MAGIC_FREED;
#endif /* DEBUG_LIGHT */

  size = ((SshUInt32 *) ptr)[-2];
  is_vmalloc = (size & SSH_MALLOC_FLAG_LARGE) != 0;

  if (is_vmalloc)
    size &= ~SSH_MALLOC_FLAG_LARGE;

  /* Statistics */
  SSH_LINUX_STATISTICS(ssh_interceptor_context,
  {
    ssh_interceptor_context->stats.allocated_memory -= (SshUInt64) size;
    if (is_vmalloc)
      ssh_interceptor_context->stats.num_allocations_large--;
    else
      ssh_interceptor_context->stats.num_allocations--;

    ssh_interceptor_context->stats.num_allocations_total--;
  });

  /* Free the real block */
  if (is_vmalloc)
    {
      SshUInt32 order;
      for (order = 0; ((1 << order) * PAGE_SIZE) < size; order++);
      free_pages((unsigned long)((char *) ptr - SSH_MALLOC_OVERHEAD), order);
    }
  else
    kfree((char *) ptr - SSH_MALLOC_OVERHEAD);
}
