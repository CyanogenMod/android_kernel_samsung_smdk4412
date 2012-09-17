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
 * sshgetput.h
 *
 * Macros for enconding/decoding of integers to character data.
 *
 */

#ifndef SSHGETPUT_H
#define SSHGETPUT_H


#define SSH_GET_8BIT(cp) (*(unsigned char *)(cp))
#define SSH_PUT_8BIT(cp, value) (*(unsigned char *)(cp)) = \
  (unsigned char)(value)

#define SSH_GET_4BIT_LOW(cp) (*(unsigned char *)(cp) & 0x0f)
#define SSH_GET_4BIT_HIGH(cp) ((*(unsigned char *)(cp) >> 4) & 0x0f)
#define SSH_PUT_4BIT_LOW(cp, value) (*(unsigned char *)(cp) = \
  (unsigned char)((*(unsigned char *)(cp) & 0xf0) | ((value) & 0x0f)))
#define SSH_PUT_4BIT_HIGH(cp, value) (*(unsigned char *)(cp) = \
  (unsigned char)((*(unsigned char *)(cp) & 0x0f) | (((value) & 0x0f) << 4)))

#ifdef SSHUINT64_IS_64BITS

#define SSH_GET_64BIT(cp) (((SshUInt64)SSH_GET_32BIT((cp)) << 32) | \
                           ((SshUInt64)SSH_GET_32BIT((cp) + 4)))
#define SSH_PUT_64BIT(cp, value) do { \
  SSH_PUT_32BIT((cp), (SshUInt32)((SshUInt64)(value) >> 32)); \
  SSH_PUT_32BIT((cp) + 4, (SshUInt32)(value)); } while (0)

#else /* SSHUINT64_IS_64BITS */

#define SSH_GET_64BIT(cp) ((SshUInt64)SSH_GET_32BIT((cp) + 4))
#define SSH_PUT_64BIT(cp, value) do { \
  SSH_PUT_32BIT((cp), 0L); \
  SSH_PUT_32BIT((cp) + 4, (SshUInt32)(value)); } while (0)
#define SSH_GET_64BIT_LSB_FIRST(cp) ((SshUInt64)SSH_GET_32BIT((cp)))
#define SSH_PUT_64BIT_LSB_FIRST(cp, value) do { \
  SSH_PUT_32BIT_LSB_FIRST((cp), (SshUInt32)(value)); \
  SSH_PUT_32BIT_LSB_FIRST((cp) + 4, 0L); } while (0)

#define SSH_GET_40BIT(cp) ((SshUInt64)SSH_GET_32BIT((cp) + 1))
#define SSH_PUT_40BIT(cp, value) do { \
  SSH_PUT_8BIT((cp), 0); \
  SSH_PUT_32BIT((cp) + 1, (SshUInt32)(value)); } while (0)
#define SSH_GET_40BIT_LSB_FIRST(cp) ((SshUInt64)SSH_GET_32BIT_LSB_FIRST((cp)))
#define SSH_PUT_40BIT_LSB_FIRST(cp, value) do { \
  SSH_PUT_32BIT_LSB_FIRST((cp), (SshUInt32)(value)); \
  SSH_PUT_8BIT((cp) + 4, 0); } while (0)

#endif /* SSHUINT64_IS_64BITS */


/*------------ macros for storing/extracting msb first words -------------*/

#define SSH_GET_32BIT(cp) \
  ((((unsigned long)((unsigned char *)(cp))[0]) << 24) | \
   (((unsigned long)((unsigned char *)(cp))[1]) << 16) | \
   (((unsigned long)((unsigned char *)(cp))[2]) << 8) | \
   ((unsigned long)((unsigned char *)(cp))[3]))

#define SSH_GET_16BIT(cp) \
     ((SshUInt16) ((((unsigned long)((unsigned char *)(cp))[0]) << 8) | \
      ((unsigned long)((unsigned char *)(cp))[1])))

#define SSH_PUT_32BIT(cp, value) do { \
  ((unsigned char *)(cp))[0] = (unsigned char)((value) >> 24); \
  ((unsigned char *)(cp))[1] = (unsigned char)((value) >> 16); \
  ((unsigned char *)(cp))[2] = (unsigned char)((value) >> 8); \
  ((unsigned char *)(cp))[3] = (unsigned char)(value); } while (0)

#define SSH_PUT_16BIT(cp, value) do { \
  ((unsigned char *)(cp))[0] = (unsigned char)((value) >> 8); \
  ((unsigned char *)(cp))[1] = (unsigned char)(value); } while (0)

/*------------ macros for storing/extracting lsb first words -------------*/

#define SSH_GET_32BIT_LSB_FIRST(cp) \
  (((unsigned long)((unsigned char *)(cp))[0]) | \
  (((unsigned long)((unsigned char *)(cp))[1]) << 8) | \
  (((unsigned long)((unsigned char *)(cp))[2]) << 16) | \
  (((unsigned long)((unsigned char *)(cp))[3]) << 24))

#define SSH_GET_16BIT_LSB_FIRST(cp) \
  ((SshUInt16) (((unsigned long)((unsigned char *)(cp))[0]) | \
  (((unsigned long)((unsigned char *)(cp))[1]) << 8)))

#define SSH_PUT_32BIT_LSB_FIRST(cp, value) do { \
  ((unsigned char *)(cp))[0] = (unsigned char)(value); \
  ((unsigned char *)(cp))[1] = (unsigned char)((value) >> 8); \
  ((unsigned char *)(cp))[2] = (unsigned char)((value) >> 16); \
  ((unsigned char *)(cp))[3] = (unsigned char)((value) >> 24); } while (0)

#define SSH_PUT_16BIT_LSB_FIRST(cp, value) do { \
  ((unsigned char *)(cp))[0] = (unsigned char)(value); \
  ((unsigned char *)(cp))[1] = (unsigned char)((value) >> 8); } while (0)

#endif /* GETPUT_H */
