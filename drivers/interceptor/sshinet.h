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
 * sshinet.h
 *
 * Inet API.
 *
 */

#ifndef SSHINET_H
#define SSHINET_H

#include "sshgetput.h"

/*************************** Ethernet definitions ***************************/

/* Etherenet header things we need */
#define SSH_ETHERH_HDRLEN       14
#define SSH_ETHERH_OFS_DST      0
#define SSH_ETHERH_OFS_SRC      6
#define SSH_ETHERH_OFS_TYPE     12
#define SSH_ETHERH_ADDRLEN      6

/* Known values for the ethernet type field.  The same values are used for
   both ethernet (rfc894) and IEEE 802 encapsulation (the type will just
   be in a different position in the header). */
#define SSH_ETHERTYPE_IP        0x0800 /* IPv4, as per rfc894 */
#define SSH_ETHERTYPE_ARP       0x0806 /* ARP, as per rfc826 */
#define SSH_ETHERTYPE_IPv6      0x86dd /* IPv6, as per rfc1972 */


/***************************** SshIpAddr stuff ******************************/

typedef enum {
    SSH_IP_TYPE_NONE = 0,
    SSH_IP_TYPE_IPV4 = 1,
    SSH_IP_TYPE_IPV6 = 2
} SshIpAddrType;

#if defined(WITH_IPV6)
/* An IPv6 link-local address scope ID. */
struct SshScopeIdRec
{
  union
  {
    SshUInt32 ui32;
  } scope_id_union;
};

typedef struct SshScopeIdRec SshScopeIdStruct;
typedef struct SshScopeIdRec *SshScopeId;

#endif /* WITH_IPV6 */

#if !defined(WITH_IPV6)
#define SSH_IP_ADDR_SIZE 4
#define SSH_IP_ADDR_STRING_SIZE 32
#else /* WITH_IPV6 */
#define SSH_IP_ADDR_SIZE 16
#define SSH_IP_ADDR_STRING_SIZE 64
#endif /* !WITH_IPV6 */

typedef struct SshIpAddrRec
{
  /* Note: All fields of this data structure are private, and should
     not be accessed except using the macros and functions defined in
     this header.  They should never be accessed directly; the
     internal definition of this structure is subject to change
     without notice. */
  SshUInt8 type; /* KEEP type first if changing rest of the contents */
  SshUInt8 mask_len;

  /* There is a hole of 16 bits here */

  /* For optimised mask comparison routine _addr_data has to be 32-bit
     aligned so it can be read as words on machines requiring
     alignment */
  union {
    unsigned char _addr_data[SSH_IP_ADDR_SIZE];
    SshUInt32 _addr_align;
  } addr_union;

#define addr_data addr_union._addr_data

#if defined(WITH_IPV6)
  SshScopeIdStruct scope_id;
#endif /* WITH_IPV6 */

} *SshIpAddr, SshIpAddrStruct;

#define SSH_IP_DEFINED(ip_addr) ((ip_addr)->type != SSH_IP_TYPE_NONE)
#define SSH_IP_IS4(ip_addr)     ((ip_addr)->type == SSH_IP_TYPE_IPV4)
#define SSH_IP_IS6(ip_addr)     ((ip_addr)->type == SSH_IP_TYPE_IPV6)

#define SSH_IP_ADDR_LEN(ip_addr)        \
  (SSH_PREDICT_TRUE(SSH_IP_IS4(ip_addr))\
   ? (4)                                \
   : (SSH_IP_IS6(ip_addr)               \
      ? (16)                            \
      : 0))

/* Make given IP address undefined. */
#define SSH_IP_UNDEFINE(IPADDR)         \
do {                                    \
  (IPADDR)->type = SSH_IP_TYPE_NONE;    \
} while (0)


#if defined(WITH_IPV6)
/* Decode, that is fill given 'ipaddr', with given 'type', 'bytes' and
   'masklen' information. */
#define __SSH_IP_MASK_DECODE(IPADDR,TYPE,BYTES,BYTELEN,MASKLEN) \
  do {                                                          \
    (IPADDR)->type = (TYPE);                                    \
    memmove((IPADDR)->addr_data, (BYTES), (BYTELEN));           \
    memset(&(IPADDR)->scope_id, 0, sizeof((IPADDR)->scope_id)); \
    (IPADDR)->mask_len = (MASKLEN);                             \
  } while (0)
#else /* WITH_IPV6 */
#define __SSH_IP_MASK_DECODE(IPADDR,TYPE,BYTES,BYTELEN,MASKLEN) \
  do {                                                          \
    (IPADDR)->type = (TYPE);                                    \
    memmove((IPADDR)->addr_data, (BYTES), (BYTELEN));           \
    (IPADDR)->mask_len = (MASKLEN);                             \
  } while (0)
#endif /* WITH_IPV6 */

/* Encode, that is copy from 'ipaddr' into 'bytes' and 'maskptr'.  The
   input 'ipaddr' needs to be of given 'type'. It is an fatal error
   to call this for invalid address type. */
#define __SSH_IP_MASK_ENCODE(IPADDR,TYPE,BYTES,BYTELEN,MASKPTR) \
  do {                                                          \
    SSH_VERIFY((IPADDR)->type == (TYPE));                       \
    memmove((BYTES), (IPADDR)->addr_data, (BYTELEN));           \
    if (SSH_PREDICT_FALSE(MASKPTR))                             \
      *((SshUInt32 *) (MASKPTR)) = (IPADDR)->mask_len;          \
  } while (0)

/* IPv4 Address manipulation */
#define SSH_IP4_ENCODE(ip_addr,bytes) \
  __SSH_IP_MASK_ENCODE(ip_addr,SSH_IP_TYPE_IPV4,bytes,4,NULL)
#define SSH_IP4_DECODE(ip_addr,bytes) \
  __SSH_IP_MASK_DECODE(ip_addr,SSH_IP_TYPE_IPV4,bytes,4,32)

/* IPv6 address manipulation */
#define SSH_IP6_ENCODE(ip_addr,bytes) \
  __SSH_IP_MASK_ENCODE(ip_addr,SSH_IP_TYPE_IPV6,bytes,16,NULL)

#if !defined(WITH_IPV6)
#define SSH_IP6_DECODE(ip_addr,bytes) SSH_IP_UNDEFINE(ip_addr)
#else /* WITH_IPV6 */
#define SSH_IP6_DECODE(ip_addr,bytes) \
  __SSH_IP_MASK_DECODE(ip_addr,SSH_IP_TYPE_IPV6,bytes,16,128)
#endif /* !WITH_IPV6 */

#define SSH_IP_MASK_LEN(ip_addr) ((ip_addr)->mask_len)

#if defined(WITH_IPV6)
#define SSH_IP6_SCOPE_ID(ip_addr) ((ip_addr)->scope_id.scope_id_union.ui32)
#endif /* WITH_IPV6 */

/*********************** Definitions for IPv4 packets ***********************/


/* IPv4 header lengths. */
#define SSH_IPH4_HDRLEN 20
#define SSH_IPH4_HLEN(ucp) SSH_GET_4BIT_LOW(ucp)
#define SSH_IPH4_LEN(ucp) SSH_GET_16BIT((ucp) + 2)
#define SSH_IPH4_SRC(ipaddr, ucp) SSH_IP4_DECODE((ipaddr), (ucp) + 12)

/*********************** Definitions for IPv6 packets ***********************/

/* IPv6 header length. Extension headers are not counted in IPv6
   header */
#define SSH_IPH6_HDRLEN 40

#define SSH_IPH6_OFS_LEN                4
#define SSH_IPH6_OFS_NH                 6

#define SSH_IPH6_ADDRLEN               16
#define SSH_IPH6_OFS_SRC               8

#define SSH_IP6_WORD0_TO_INT(ip_addr) SSH_GET_32BIT((ip_addr)->addr_data)
#define SSH_IP6_WORD1_TO_INT(ip_addr) SSH_GET_32BIT((ip_addr)->addr_data + 4)
#define SSH_IP6_WORD2_TO_INT(ip_addr) SSH_GET_32BIT((ip_addr)->addr_data + 8)
#define SSH_IP6_WORD3_TO_INT(ip_addr) SSH_GET_32BIT((ip_addr)->addr_data + 12)

#define SSH_IPH6_LEN(ucp) SSH_GET_16BIT((ucp) + SSH_IPH6_OFS_LEN)
#define SSH_IPH6_NH(ucp) SSH_GET_8BIT((ucp) + SSH_IPH6_OFS_NH)
#define SSH_IPH6_SRC(ipaddr, ucp) SSH_IP6_DECODE((ipaddr), \
                                                 (ucp) + SSH_IPH6_OFS_SRC)

/****************** Definitions for IPv6 extension headers ******************/

#define SSH_IP6_EXT_COMMON_NH(ucp)      SSH_GET_8BIT((ucp))
#define SSH_IP6_EXT_COMMON_LEN(ucp)     SSH_GET_8BIT((ucp) + 1)
#define SSH_IP6_EXT_COMMON_LENB(ucp)		\
  ((SSH_IP6_EXT_COMMON_LEN((ucp)) + 1) << 3)

/*************************** Link definitions *******************************/

/* Reserved value for invalid interface index. */
#define SSH_INVALID_IFNUM       0xffffffff

/***************************** Helper functions *****************************/

/* Sets all rightmost bits after keeping `keep_bits' bits on the left
   to the value specified by `value'. */
void ssh_ipaddr_set_bits(SshIpAddr result, SshIpAddr ip,
                         unsigned int keep_bits, unsigned int value);

/* Prints the IP address into the buffer in string format.  If the buffer
   is too short, the address is truncated.  This returns `buf'. */
unsigned char *ssh_ipaddr_print(const SshIpAddr ip, unsigned char *buf,
                                size_t buflen);

/* Prints the IP address into the buffer in string format.  If the buffer
   is too short, the address is truncated.  This returns `buf'. */
void ssh_ipaddr_ipv4_print(const unsigned char *data,
			   unsigned char *buf, size_t buflen);
void ssh_ipaddr_ipv6_print(const unsigned char *data,
			   unsigned char *buf, size_t buflen,
			   SshUInt32 scope);

#endif /* SSHINET_H */
