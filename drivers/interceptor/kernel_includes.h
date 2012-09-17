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
 * kernel_includes.h
 *
 * Common include file for kernel space components.
 *
 */

#ifndef KERNEL_INCLUDES_H
#define KERNEL_INCLUDES_H

#ifdef KERNEL
# undef _KERNEL
# define _KERNEL
#endif /* KERNEL */

#include "sshconf.h"

/* Set SIZEOF_* defines to point to kernel definitions of those. */
#ifndef SIZEOF_INT
# define SIZEOF_INT      KERNEL_SIZEOF_INT
#endif /* SIZEOF_INT */

#ifndef SIZEOF_LONG
# define SIZEOF_LONG     KERNEL_SIZEOF_LONG
#endif /* SIZEOF_LONG */

#ifndef SIZEOF_LONG_LONG
# define SIZEOF_LONG_LONG KERNEL_SIZEOF_LONG_LONG
#endif /* SIZEOF_LONG_LONG */

#ifndef SIZEOF_SHORT
# define SIZEOF_SHORT    KERNEL_SIZEOF_SHORT
#endif /* SIZEOF_SHORT */

#ifndef SIZEOF_VOID_P
# define SIZEOF_VOID_P    KERNEL_SIZEOF_VOID_P
#endif /* SIZEOF_VOID_P */

/* Set HAVE_ */
#ifdef HAVE_KERNEL_SHORT
# define HAVE_SHORT
#endif
#ifdef HAVE_KERNEL_INT
# define HAVE_INT
#endif
#ifdef HAVE_KERNEL_LONG
# define HAVE_LONG
#endif
#ifdef HAVE_KERNEL_LONG_LONG
# define HAVE_LONG_LONG
#endif
#ifdef HAVE_KERNEL_VOID_P
# define HAVE_VOID_P
#endif

typedef unsigned char SshUInt8;         /* At least 8 bits. */
typedef signed char SshInt8;            /* At least 8 bits. */

typedef unsigned short SshUInt16;       /* At least 16 bits. */
typedef short SshInt16;                 /* At least 16 bits. */

#if SIZEOF_LONG == 4
typedef unsigned long SshUInt32;        /* At least 32 bits. */
typedef long SshInt32;                  /* At least 32 bits. */
#else /* SIZEOF_LONG != 4 */
#if SIZEOF_INT == 4
typedef unsigned int SshUInt32;         /* At least 32 bits. */
typedef int SshInt32;                   /* At least 32 bits. */
#else /* SIZEOF_INT != 4 */
#if SIZEOF_SHORT >= 4
typedef unsigned short SshUInt32;       /* At least 32 bits. */
typedef short SshInt32;                 /* At least 32 bits. */
#else /* SIZEOF_SHORT < 4 */
#   error "Autoconfig error, your compiler doesn't support any 32 bit type"
#endif /* SIZEOF_SHORT < 4 */
#endif /* SIZEOF_INT != 4 */
#endif /* SIZEOF_LONG != 4 */

#if SIZEOF_LONG >= 8
typedef unsigned long SshUInt64;
typedef long SshInt64;
# define SSHUINT64_IS_64BITS
# define SSH_C64(x) (x##lu)
# define SSH_S64(x) (x##l)
#else /* SIZEOF_LONG < 8 */
#if SIZEOF_LONG_LONG >= 8
typedef unsigned long long SshUInt64;
typedef long long SshInt64;
#  define SSHUINT64_IS_64BITS
#  define SSH_C64(x) (x##llu)
#  define SSH_S64(x) (x##ll)
#else /* SIZEOF_LONG_LONG < 8 */
/* No 64 bit type; SshUInt64 and SshInt64 will be 32 bits. */
typedef unsigned long SshUInt64;
typedef long SshInt64;
#  define SSH_C64(x) SSH_FATAL(ERROR_NO_64_BIT_ON_THIS_SYSTEM)
#endif /* SIZEOF_LONG_LONG < 8 */
#endif /* SIZEOF_LONG < 8 */

typedef SshInt64 SshTime;

#include <linux/types.h>

#ifdef HAVE_MACHINE_ENDIAN_H
# include <sys/param.h>
# include <machine/endian.h>
#endif

#include <stddef.h>

#include <stdarg.h>

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

typedef unsigned int Boolean;

/* Platform-specific kernel-mode definitions follow. */

/******************************   LINUX       *****************************/
/* Sanity checks about module support and that we are supporting it really  */
#ifndef MODULE
#    error  "MODULE must be defined when compiling for Linux"
#endif

#include <linux/string.h>

/* Including linux/skbuff.h here allows us to cleanly make
   ssh_interceptor_packet*() macros for it. It really ought to
   be in the the "platform_interceptor.h" header file, but unfortunately
   this would require generic engine code to set SSH_ALLOW_SYSTEM_SPRINTFS,
   which is why it is here. */
#include "linux/skbuff.h"

/******************************   LINUX (END) *****************************/

#endif /* KERNEL_INCLUDES_H */
