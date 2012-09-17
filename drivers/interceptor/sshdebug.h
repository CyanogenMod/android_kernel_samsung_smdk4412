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
 * sshdebug.h
 *
 * Debug macros and assertions.
 *
 */

#ifndef SSHDEBUG_H
#define SSHDEBUG_H

#ifdef DEBUG_LIGHT

/* Debug level */
extern unsigned int ssh_debug_level;

#define SSH_FATAL(_fmt...) panic(_fmt)

#define SSH_NOTREACHED							   \
  panic("%s:%d: Unreachable code reached!", __FILE__, __LINE__)

#define SSH_DEBUG(level, varcall)					   \
  do {									   \
    if ((level) <= ssh_debug_level)					   \
      {									   \
	printk("%s:%d: ", __FILE__, __LINE__);				   \
	printk varcall;							   \
	printk("\n");							   \
      }									   \
  } while (0)

#define SSH_ASSERT(cond)                                                   \
  do {                                                                     \
    if (!(cond))                                                           \
      panic("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, "#cond"); \
  } while (0)
#define SSH_VERIFY(cond)  SSH_ASSERT(cond)
#define SSH_PRECOND(cond) SSH_ASSERT(cond)

static inline void
ssh_debug_hexdump(const unsigned char *buf, const size_t len)
{
  size_t i;

  for (i = 0; (i + 16) < len; i += 16)
    printk("%08x: %02x%02x %02x%02x %02x%02x %02x%02x "
	   "%02x%02x %02x%02x %02x%02x %02x%02x\n",
	   (unsigned int) i,
	   (unsigned int) buf[i+0], (unsigned int) buf[i+1],
	   (unsigned int) buf[i+2], (unsigned int) buf[i+3],
	   (unsigned int) buf[i+4], (unsigned int) buf[i+5],
	   (unsigned int) buf[i+6], (unsigned int) buf[i+7],
	   (unsigned int) buf[i+8], (unsigned int) buf[i+9],
	   (unsigned int) buf[i+10], (unsigned int) buf[i+11],
	   (unsigned int) buf[i+12], (unsigned int) buf[i+13],
	   (unsigned int) buf[i+14], (unsigned int) buf[i+15]);
  if (i >= len)
    return;

  printk("%08x: ", (unsigned int) i);
  for (; i < len; i++)
    printk("%02x%s", (unsigned int) buf[i], ((i % 2) == 1 ? " " : ""));
  printk("\n");
}

#define SSH_DEBUG_HEXDUMP(level, varcall, buf, len)	\
  do {						        \
    if ((level) <= ssh_debug_level)			\
      {							\
	printk("%s:%d: ", __FILE__, __LINE__);		\
	printk varcall;					\
	printk("\n");					\
	ssh_debug_hexdump((buf), (len));		\
      }							\
  } while (0)

#else /* !DEBUG_LIGHT */

#define SSH_FATAL(_fmt...) panic(_fmt)

#define SSH_NOTREACHED                                                     \
  panic("%s:%d: Unreachable code reached!", __FILE__, __LINE__)

#define SSH_DEBUG(level, varcall)

#define SSH_VERIFY(cond)                                                   \
  do {                                                                     \
    if (!(cond))                                                           \
      panic("%s:%d: Verify failed: %s\n", __FILE__, __LINE__, "#cond");    \
  } while (0)

#ifdef __COVERITY__
#define SSH_ASSERT(cond)                                                   \
  do {                                                                     \
    if (!(cond))                                                           \
      panic("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, "#cond"); \
  } while (0)
#else /* __COVERITY__ */
#define SSH_ASSERT(cond)
#endif /* __COVERITY__ */

#define SSH_PRECOND(cond) SSH_ASSERT(cond)

#define SSH_DEBUG_HEXDUMP(level, varcall, buf, len)

#endif /* DEBUG_LIGHT */



/* *********************************************************************
 *   DEBUG LEVELS
 * *********************************************************************/

/* *********************************************************************
 * Debug type definitions for debug level mapping
 * *********************************************************************/

/* Use debug code definitions below, not the debug level numbers
   (except 11-15). */

/** Software malfunction. */
#define SSH_D_ERROR  0

/** Software failure, but caused by a packet coming from network. */
#define SSH_D_NETFAULT 3

/** Data formatted incorrectly coming from a network or other outside source.*/
#define SSH_D_NETGARB 3

/** Nonfatal failure in a high or middle-level operation. */
#define SSH_D_FAIL 3

/** Uncommon situation. */
#define SSH_D_UNCOMMON 6

/** Success in a high-level operation. */
#define SSH_D_HIGHOK 4

/** Success in a middle-level operation. */
#define SSH_D_MIDOK 7

/** Success in a low-level operation. */
#define SSH_D_LOWOK 9

/** Start of a high-level operation. */
#define SSH_D_HIGHSTART 5

/** Start of a middle-level operation. */
#define SSH_D_MIDSTART 8

/** Start of a low-level operation. */
#define SSH_D_LOWSTART 10

/** Nice-to-know information. */
#define SSH_D_NICETOKNOW 7

/** Data block dump. */
#define SSH_D_DATADUMP 8

/** Packet dump. */
#define SSH_D_PCKDMP 9

/** Middle result of an operation, loop-internal information. */
#define SSH_D_MIDRESULT 10

#endif /* SSHDEBUG_H */
