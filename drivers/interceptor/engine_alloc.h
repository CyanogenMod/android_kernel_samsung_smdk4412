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
 * engine_alloc.h
 *
 * Engine memory allocation API.
 *
 */

#ifndef ENGINE_ALLOC_H
#define ENGINE_ALLOC_H

void *ssh_malloc(size_t size);
void *ssh_malloc_flags(size_t size, SshUInt32 flags);
void *ssh_realloc(void *ptr, size_t old_size, size_t new_size);
void *ssh_realloc_flags(void *ptr, size_t old_size, size_t new_size,
                        SshUInt32 flags);
void *ssh_calloc(size_t nitems, size_t size);
void *ssh_calloc_flags(size_t nitems, size_t size, SshUInt32 flags);
void *ssh_strdup(const void *p);
void *ssh_memdup(const void *p, size_t len);
void ssh_free(void *ptr);

#endif /* ENGINE_ALLOC_H */
