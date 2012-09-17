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
 * sshinetencode.c
 *
 * Implementation of inet API IP address encoding and decoding functions.
 *
 */

#include "sshincludes.h"
#include "sshencode.h"
#include "sshinetencode.h"

size_t ssh_encode_ipaddr_array(unsigned char *buf, size_t bufsize,
			       const SshIpAddr ip)
{
  if (!ip || ip->type == SSH_IP_TYPE_NONE)
    return ssh_encode_array(buf, bufsize,
			    SSH_ENCODE_CHAR(SSH_IP_TYPE_NONE),
			    SSH_FORMAT_END);
  return ssh_encode_array(buf, bufsize,
			  SSH_ENCODE_CHAR(ip->type),
			  SSH_ENCODE_UINT32(ip->mask_len),
			  SSH_ENCODE_DATA(ip->addr_data,
					  SSH_IP_ADDR_LEN(ip)),
#ifdef WITH_IPV6
                          SSH_ENCODE_UINT32(ip->scope_id.scope_id_union.ui32),
#endif /* WITH_IPV6 */
			  SSH_FORMAT_END);
}

size_t ssh_encode_ipaddr_array_alloc(unsigned char **buf_return,
				     const SshIpAddr ip)
{
  size_t req, got;

  if (ip->type == SSH_IP_TYPE_NONE)
    req = 1;
  else
#ifdef WITH_IPV6
    req = 1 + 8 + SSH_IP_ADDR_LEN(ip);
#else  /* WITH_IPV6 */
    req = 1 + 4 + SSH_IP_ADDR_LEN(ip);
#endif /* WITH_IPV6 */

  if (buf_return == NULL)
    return req;

  if ((*buf_return = ssh_malloc(req)) == NULL)
    return 0;

  got = ssh_encode_ipaddr_array(*buf_return, req, ip);

  if (got != req)
    {
      ssh_free(*buf_return);
      *buf_return = NULL;
      return 0;
    }

  return got;
}

int ssh_decode_ipaddr_array(const unsigned char *buf, size_t len,
			    void * ipaddr)
{
  size_t point, got;
  SshUInt32 mask_len;
#ifdef WITH_IPV6
  SshUInt32 scope_id;
#endif /* WITH_IPV6 */
  unsigned int type;
  SshIpAddr ip = (SshIpAddr)ipaddr;
  point = 0;

  if ((got = ssh_decode_array(buf + point, len - point,
                              SSH_DECODE_CHAR(&type),
                              SSH_FORMAT_END)) != 1)
      return 0;

  /* Make sure scope-id (that is not present at the kernel) is
     zeroed */
  memset(ip, 0, sizeof(*ip));

  ip->type = (SshUInt8) type;

  point += got;

  if (ip->type == SSH_IP_TYPE_NONE)
    return point;

  if ((got = ssh_decode_array(buf + point, len - point,
                              SSH_DECODE_UINT32(&mask_len),
                              SSH_DECODE_DATA(ip->addr_data,
					      SSH_IP_ADDR_LEN(ip)),
#ifdef WITH_IPV6
                              SSH_DECODE_UINT32(&scope_id),
                              SSH_FORMAT_END)) != ((2 * sizeof(SshUInt32))
                                                   + SSH_IP_ADDR_LEN(ip)))
#else  /* WITH_IPV6 */
                              SSH_FORMAT_END)) != (4 + SSH_IP_ADDR_LEN(ip)))
#endif /* WITH_IPV6 */
      return 0;

  /* Sanity check */
  if (mask_len > 255)
	  return 0;

  ip->mask_len = (SshUInt8) mask_len;

  point += got;

#ifdef WITH_IPV6
  ip->scope_id.scope_id_union.ui32 = scope_id;
#endif /* WITH_IPV6 */

  /* Sanity check */
  if (!SSH_IP_IS4(ip) && !SSH_IP_IS6(ip))
    return 0;

  return point;
}
