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
 * sshinetencode.h
 *
 * Inet API: IP address encoding and decoding functions.
 *
 */

#ifndef SSHINETENCODE_H
#define SSHINETENCODE_H

#include "sshinet.h"

/* Decode IP-address from array. */
int ssh_decode_ipaddr_array(const unsigned char *buf, size_t bufsize,
			    void *ip);

/* Encode IP-address to array. Return 0 in case it does not fit to the buffer.
   NOTE, this is NOT a SshEncodeDatum Encoder, as the return values are
   different. */
size_t ssh_encode_ipaddr_array(unsigned char *buf, size_t bufsize,
			       const SshIpAddr ip);
size_t ssh_encode_ipaddr_array_alloc(unsigned char **buf_return,
				     const SshIpAddr ip);

#ifdef WITH_IPV6
/* type+mask+scopeid+content */
#define SSH_MAX_IPADDR_ENCODED_LENGTH (1+4+4+16)
#else  /* WITH_IPV6 */
/* type+mask+content */
#define SSH_MAX_IPADDR_ENCODED_LENGTH (1+4+16)
#endif /* WITH_IPV6 */

#endif /* SSHINETENCODE_H */
