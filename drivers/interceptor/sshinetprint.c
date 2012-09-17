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
 * sshinetprint.c
 *
 * Implementation of inet API IP address printing functions.
 *
 */

#include "sshincludes.h"
#include "sshinet.h"

/* Prints the IP address into the buffer in string format.  If the buffer
   is too short, the address is truncated.  This returns `buf'. */

void ssh_ipaddr_ipv4_print(const unsigned char *data,
			   unsigned char *buf, size_t buflen)
{
  snprintf(buf, buflen, "%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
}

void ssh_ipaddr_ipv6_print(const unsigned char *data,
			   unsigned char *buf, size_t buflen,
			   SshUInt32 scope)
{
  int i, j;
  unsigned char *cp;
  int opt_start = 8;
  int opt_len = 0, n, l;
  SshUInt16 val;

  /* Optimize the largest zero-block from the address. */
  for (i = 0; i < 8; i++)
    if (SSH_GET_16BIT(data + i * 2) == 0)
      {
        for (j = i + 1; j < 8; j++)
          {
            val = SSH_GET_16BIT(data + j * 2);
            if (val != 0)
              break;
          }
        if (j - i > opt_len)
          {
            opt_start = i;
            opt_len = j - i;
          }
        i = j;
      }

  if (opt_len <= 1)
    /* Disable optimization. */
    opt_start = 8;

  cp = buf;
  n = buflen;

  /* Format the result. */
  for (i = 0; i < 8; i++)
    {
      if (i == opt_start)
        {
          if (i == 0)
	    {
	      *cp++ = ':';
	      n -= 1;
	      if (n <= 1)
		break;
	    }

          *cp++ = ':';
	  n -= 1;
	  if (n <= 1)
	    break;

          i += opt_len - 1;
        }
      else
        {
          l = snprintf(cp, n, "%x",
		       (unsigned int) SSH_GET_16BIT(data + i * 2));
	  if (l == -1)
	    {
	      *cp = '\0';
	      return;
	    }

          cp += l;
	  n -= l;
	  if (n <= 1)
	    break;

          if (i + 1 < 8)
            {
              *cp = ':';
              cp++;
	      n -= 1;
	      if (n <= 1)
		break;
            }
        }
    }

  /* Put scope id there, if stored. */
  if (scope != 0)
    {
      l = snprintf(cp, n, "%%%u", (unsigned int) scope);

      if (l > 0)
	cp += l;
    }

  *cp = '\0';
}

unsigned char *ssh_ipaddr_print(const SshIpAddr ip, unsigned char *buf,
                                size_t buflen)
{
  if (SSH_IP_IS4(ip))
    ssh_ipaddr_ipv4_print(ip->addr_data, buf, buflen);
#if defined(WITH_IPV6)
  else if (SSH_IP_IS6(ip))
    ssh_ipaddr_ipv6_print(ip->addr_data, buf, buflen, SSH_IP6_SCOPE_ID(ip));
#endif /* WITH_IPV6 */
  else if (buflen > 0)
    buf[0] = '\0';

  return buf;
}
