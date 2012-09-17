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
 * kernel_alloc.
 *
 * Engine memory allocation API implementation for kernel space.
 *
 */

#include "sshincludes.h"
#include "kernel_alloc.h"
#include "kernel_mutex.h"

void *
ssh_malloc_flags(size_t size, SshUInt32 flags)
{
  return ssh_kernel_alloc(size, flags);
}

void *
ssh_malloc(size_t size)
{
  return ssh_malloc_flags(size, SSH_KERNEL_ALLOC_NOWAIT);
}

void *
ssh_realloc_flags(void *oldptr, size_t oldsize, size_t newsize,
                  SshUInt32 flags)
{
  void * newptr;

  if (oldptr == NULL)
    return ssh_kernel_alloc(newsize, flags);

  if (newsize <= oldsize)
    return oldptr;

  if ((newptr = ssh_kernel_alloc(newsize, flags)) == NULL)
      return NULL;

  /* newsize > oldsize, see above */
  if (oldsize > 0)
    memcpy(newptr, oldptr, oldsize);

  /* Success, thus we can release the old memory */
  ssh_kernel_free(oldptr);

  return newptr;
}

void *
ssh_realloc(void * oldptr, size_t oldsize, size_t newsize)
{
  return ssh_realloc_flags(oldptr, oldsize, newsize, SSH_KERNEL_ALLOC_NOWAIT);
}

/* coverity[ -tainted_data_sink : arg-0 ] */
void ssh_free (void * ptr)
{
  if (ptr != NULL)
    ssh_kernel_free(ptr);
}

void*
ssh_calloc_flags (size_t nitems, size_t isize, SshUInt32 flags)
{
  void                * ptr;
  unsigned long       size;

  size = isize * nitems;

  if ((ptr = ssh_malloc_flags(size ? size : 1, flags)) == NULL)
    return NULL;

  if (size > 0)
    memset(ptr, 0, size);

  return ptr;
}

void *
ssh_calloc(size_t nitems, size_t isize)
{
  return ssh_calloc_flags(nitems, isize, SSH_KERNEL_ALLOC_NOWAIT);
}

void *ssh_strdup (const void * p)
{
  const char  * str;
  char        * cp;

  SSH_PRECOND(p != NULL);

  str = (const char *) p;

  if ((cp = (char *) ssh_malloc(strlen(str) + 1)) == NULL)
    return NULL;

  strcpy(cp, str);

  return (void *) cp;
}

void *ssh_memdup(const void * p, size_t len)
{
  void        * cp;

  if ((cp = ssh_malloc(len + 1)) == NULL)
    return NULL;

  memcpy(cp, p, (size_t)len);

  ((unsigned char *) cp)[len] = '\0';

  return cp;
}
