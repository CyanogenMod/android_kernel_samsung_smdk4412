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
 * sshinetbits.c
 *
 * Implementation of inet API IP address bit manipulation functions.
 *
 */

#include "sshincludes.h"
#include "sshinet.h"

/* Sets all rightmost bits after keeping `keep_bits' bits on the left to
   the value specified by `value'. */

void ssh_ipaddr_set_bits(SshIpAddr result, SshIpAddr ip,
                         unsigned int keep_bits, unsigned int value)
{
  size_t len;
  unsigned int i;

  len = SSH_IP_IS6(ip) ? 16 : 4;

  *result = *ip;
  for (i = keep_bits / 8; i < len; i++)
    {
      if (8 * i >= keep_bits)
        result->addr_data[i] = value ? 0xff : 0;
      else
        {
          SSH_ASSERT(keep_bits - 8 * i < 8);
          result->addr_data[i] &= (0xff << (8 - (keep_bits - 8 * i)));
          if (value)
            result->addr_data[i] |= (0xff >> (keep_bits - 8 * i));
        }
    }
}
