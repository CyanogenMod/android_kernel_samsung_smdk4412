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
 * kernel_alloc.h
 *
 * Kernel memory allocation API.
 *
 */

#ifndef KERNEL_ALLOC_H
#define KERNEL_ALLOC_H

/* Allocate 'size' amount of memory, with the 'flag'
   parameters. Returns a NULL value if the allocation request cannot
   be satisfied for some reason.

   Notice: 'flag' is nothing more than a hint to the allocator. The
   allocator is free to ignore 'flag'. The allocatee is free to
   specify flag as ssh_rand() number, and the returned memory must still
   have the same semantics as any other memory block allocated. */
void *ssh_kernel_alloc(size_t size, SshUInt32 flag);

/* Flag is or-ed together of the following flags. */
#define SSH_KERNEL_ALLOC_NOWAIT 0x0000 /* allocation/use atomic. */
#define SSH_KERNEL_ALLOC_WAIT   0x0001 /* allow sleeping alloc/use. */
#define SSH_KERNEL_ALLOC_DMA    0x0002 /* allow DMA use. */
/* Other bits are usable for other purposes? */

/* Frees a previously allocated block of memory. */
void ssh_kernel_free(void *ptr);

#ifdef DEBUG_LIGHT
#define KERNEL_ALLOC_USE_FUNCTIONS
#endif /* DEBUG_LIGHT */

#endif /* KERNEL_ALLOC_H */
