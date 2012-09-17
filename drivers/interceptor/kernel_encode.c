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
 * kernel_encode.c
 *
 * Encode/decode API implementation for kernel space.
 *
 */

#include "sshincludes.h"
#include "sshgetput.h"
#include "sshencode.h"

/* Encodes data into the buffer.  Returns the number of bytes added, or
   0 if the buffer is too small.  The data to be encoded is specified by the
   variable-length argument list.  Each element must start with a
   SshEncodingFormat type, be followed by arguments of the appropriate
   type, and the list must end with SSH_FORMAT_END. */
size_t ssh_encode_array_va(unsigned char *buf, size_t bufsize, va_list ap)
{
  SshEncodingFormat format;
  unsigned int intvalue;
  SshUInt16 u16;
  SshUInt32 u32;
  SshUInt64 u64;
  size_t len, offset;
  Boolean b;
  const unsigned char *p;

  offset = 0;
  for (;;)
    {
      format = va_arg(ap, SshEncodingFormat);
      switch (format)
        {
        case SSH_FORMAT_UINT32_STR:
          p = va_arg(ap, unsigned char *);
          len = va_arg(ap, size_t);
          if (bufsize - offset < 4 + len)
            return 0;
          SSH_PUT_32BIT(buf + offset, len);
          memcpy(buf + offset + 4, p, len);
          offset += 4 + len;
          break;

        case SSH_FORMAT_BOOLEAN:
          b = va_arg(ap, Boolean);
          if (bufsize - offset < 1)
            return 0;
          buf[offset++] = (unsigned char)b;
          break;

        case SSH_FORMAT_UINT32:
          u32 = va_arg(ap, SshUInt32);
          if (bufsize - offset < 4)
            return 0;
          SSH_PUT_32BIT(buf + offset, u32);
          offset += 4;
          break;

        case SSH_FORMAT_UINT16:
          intvalue = va_arg(ap, unsigned int);
		  u16 = (SshUInt16) intvalue;
          if (bufsize - offset < 2)
            return 0;
          SSH_PUT_16BIT(buf + offset, u16);
          offset += 2;
          break;

        case SSH_FORMAT_CHAR:
          intvalue = va_arg(ap, unsigned int);
          if (bufsize - offset < 1)
            return 0;
          buf[offset++] = (unsigned char)intvalue;
          break;

        case SSH_FORMAT_DATA:
          p = va_arg(ap, unsigned char *);
          len = va_arg(ap, size_t);
          if (bufsize - offset < len)
            return 0;
          memcpy(buf + offset, p, len);
          offset += len;
          break;

        case SSH_FORMAT_UINT64:
          u64 = va_arg(ap, SshUInt64);
          if (bufsize - offset < 8)
            return 0;
          SSH_PUT_64BIT(buf + offset, u64);
          offset += 8;
          break;

	case SSH_FORMAT_SPECIAL:
	  {
	    SshEncodeDatum fn;
	    void *datum;
	    size_t space, size;

	    fn = va_arg(ap, SshEncodeDatum);
	    datum = va_arg(ap, void *);

	    space = bufsize - offset;
	    size  = (*fn)(buf, space, datum);
	    if (size > space)
	      return 0;

	    offset += size;
	  }
	  break;

        case SSH_FORMAT_END:
          /* Return the number of bytes added. */
          return offset;

        default:
          SSH_FATAL("ssh_encode_array_va: invalid format code %d "
                    "(check arguments and SSH_FORMAT_END)",
                    (int)format);
        }
    }
}

/* Appends data at the end of the buffer as specified by the variable-length
   argument list.  Each element must start with a SshEncodingFormat type,
   be followed by arguments of the appropriate type, and the list must end
   with SSH_FORMAT_END.  This returns the number of bytes added to the
   buffer, or 0 if the buffer is too small. */
size_t ssh_encode_array(unsigned char *buf, size_t bufsize, ...)
{
  size_t bytes;
  va_list ap;

  va_start(ap, bufsize);
  bytes = ssh_encode_array_va(buf, bufsize, ap);
  va_end(ap);
  return bytes;
}

/* Doubles the size of a buffer. Copies the contents from the old one
   to the new one and frees the old one. Returns new buffer size.
   If realloc fails, frees the old buffer and returns 0. */
size_t ssh_encode_array_enlarge_buffer(unsigned char **buf, size_t bufsize)
{
  unsigned int newsize;
  unsigned char *newbuf;

  SSH_ASSERT(buf != NULL);

  newsize = bufsize * 2;
  SSH_ASSERT(newsize < 10000000);

  if (newsize == 0)
    newsize = 100;

  newbuf = ssh_realloc(*buf, bufsize, newsize);
  if (newbuf == NULL)
    {
      ssh_free(*buf);
      return 0;
    }
  *buf = newbuf;

  return newsize;
}

/* Encodes the given data.  Returns the length of encoded data in bytes, and
   if `buf_return' is non-NULL, it is set to a memory area allocated by
   ssh_malloc that contains the data.  The caller should free the data when
   no longer needed.

   Duplicates functionality in ssh_encode_array_va, could combine
   some. Note that cycling va_list with va_arg is not supported in all
   environments, so repeatedly trying ssh_encode_array_va with bigger
   buffers is not possible. */
size_t ssh_encode_array_alloc_va(unsigned char **buf_return, va_list ap)
{
  size_t bufsize;
  unsigned char *buf = NULL;
  SshEncodingFormat format;
  unsigned int intvalue;
  SshUInt16 u16;
  SshUInt32 u32;
  SshUInt64 u64;
  size_t len, offset;
  Boolean b;
  const unsigned char *p;

  /* Prepare return value in case of later failure. */
  if (buf_return != NULL)
    *buf_return = NULL;

  /* Allocate new buffer. */
  bufsize = ssh_encode_array_enlarge_buffer(&buf, 0);

  offset = 0;
  for (;;)
    {
      format = va_arg(ap, SshEncodingFormat);
      switch (format)
        {
        case SSH_FORMAT_UINT32_STR:
          p = va_arg(ap, unsigned char *);
          len = va_arg(ap, size_t);
          while (bufsize - offset < 4 + len)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          SSH_PUT_32BIT(buf + offset, len);
          memcpy(buf + offset + 4, p, len);
          offset += 4 + len;
          break;

        case SSH_FORMAT_BOOLEAN:
          b = va_arg(ap, Boolean);
          while (bufsize - offset < 1)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          buf[offset++] = (unsigned char)b;
          break;

        case SSH_FORMAT_UINT32:
          u32 = va_arg(ap, SshUInt32);
          while (bufsize - offset < 4)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          SSH_PUT_32BIT(buf + offset, u32);
          offset += 4;
          break;

        case SSH_FORMAT_UINT16:
          intvalue = va_arg(ap, unsigned int);
		  u16 = (SshUInt16) intvalue;
          while (bufsize - offset < 2)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          SSH_PUT_16BIT(buf + offset, u16);
          offset += 2;
          break;

        case SSH_FORMAT_CHAR:
          intvalue = va_arg(ap, unsigned int);
          while (bufsize - offset < 1)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          buf[offset++] = (unsigned char)intvalue;
          break;

        case SSH_FORMAT_DATA:
          p = va_arg(ap, unsigned char *);
          len = va_arg(ap, size_t);
          while (bufsize - offset < len)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          memcpy(buf + offset, p, len);
          offset += len;
          break;

        case SSH_FORMAT_UINT64:
          u64 = va_arg(ap, SshUInt64);
          while (bufsize - offset < 8)
            {
              if (!(bufsize = ssh_encode_array_enlarge_buffer(&buf, bufsize)))
                return 0;
            }
          SSH_PUT_64BIT(buf + offset, u64);
          offset += 8;
          break;

        case SSH_FORMAT_END:
          if (buf_return != NULL)
            *buf_return = buf;
          else
            /* They only want to know the size of the buffer */
            ssh_free(buf);

          /* Return the number of bytes added. */
          return offset;

        default:
          SSH_FATAL("ssh_encode_array_alloc_va: invalid format code %d "
                    "(check arguments and SSH_FORMAT_END)",
                    (int)format);
        }
    }
}

/* Encodes the given data.  Returns the length of encoded data in bytes, and
   if `buf_return' is non-NULL, it is set to a memory area allocated by
   ssh_malloc that contains the data.  The caller should free the data when
   no longer needed.  It is an error to call this with an argument list that
   would result in zero bytes being encoded. */
size_t ssh_encode_array_alloc(unsigned char **buf_return, ...)
{
  size_t bytes;
  va_list ap;

  va_start(ap, buf_return);
  bytes = ssh_encode_array_alloc_va(buf_return, ap);
  va_end(ap);

  return bytes;
}

/* Allocates a buffer of the given size with ssh_malloc.  However,
   the buffer is also recorded in *num_allocs_p and *allocs_p, so that
   they can all be easily freed later if necessary. */
static unsigned char *ssh_decode_array_alloc(unsigned int *num_allocs_p,
                                             unsigned char ***allocsp,
                                             size_t size)
{
  unsigned char *p;

  /* Check if we need to enlarge the pointer array.  We enlarge it in chunks
     of 16 pointers. */
  if (*num_allocs_p == 0)
    *allocsp = NULL;

  /* This will also match *num_allocs_p == 0, and it is valid to pass
     NULL to krealloc, so this works. */
  if (*num_allocs_p % 16 == 0)
    {
      unsigned char ** nallocsp;

      nallocsp = ssh_realloc(*allocsp,
                              *num_allocs_p
                              * sizeof(unsigned char *),
                              (*num_allocs_p + 16)
                              * sizeof(unsigned char *));

      /* If we fail in allocation, return NULL but leave *allocsp intact. */
      if (!nallocsp)
        return NULL;

      *allocsp = nallocsp;
    }

  /* Allocate the memory block. */
  if (!(p = ssh_malloc(size)))
      return NULL;

  /* Store it in the array. */
  (*allocsp)[*num_allocs_p] = p;
  (*num_allocs_p)++;

  return p;
}

/* Decodes data from the given byte array as specified by the
   variable-length argument list.  If all specified arguments could be
   successfully parsed, returns the number of bytes parsed (any
   remaining data can be parsed by first skipping this many bytes).
   If parsing any element results in an error, this returns 0 (and
   frees any already allocated data).  Zero is also returned if the
   specified length would be exceeded. */
size_t ssh_decode_array_va(const unsigned char *buf, size_t len,
                                  va_list ap)
{
  SshEncodingFormat format;
  unsigned long longvalue;
  SshUInt16 *u16p;
  SshUInt32 *u32p;
  SshUInt64 *u64p;
  Boolean *bp;
  size_t size, *sizep;
  unsigned int *uip;
  unsigned char *p, **pp;
  const unsigned char **cpp;
  size_t offset;
  unsigned int i, num_allocs;
  unsigned char **allocs = NULL;

  offset = 0;
  num_allocs = 0;

  for (;;)
    {
      /* Get the next format code. */
      format = va_arg(ap, SshEncodingFormat);
      switch (format)
        {

        case SSH_FORMAT_UINT32_STR:
          /* Get length and data pointers. */
          pp = va_arg(ap, unsigned char **);
          sizep = va_arg(ap, size_t *);

          /* Check if the length of the string is there. */
          if (len - offset < 4)
            goto fail;

          /* Get the length of the string. */
          longvalue = SSH_GET_32BIT(buf + offset);
          offset += 4;

          /* Check that the string is all in the buffer. */
          if (longvalue > len - offset)
            goto fail;

          /* Store length if requested. */
          if (sizep != NULL)
            *sizep = longvalue;

          /* Retrieve the data if requested. */
          if (pp != NULL)
            {
              *pp = ssh_decode_array_alloc(&num_allocs, &allocs,
                                            (size_t)longvalue + 1);

              if (!*pp)
                  goto fail;

              memcpy(*pp, buf + offset, (size_t)longvalue);
              (*pp)[longvalue] = '\0';
            }

          /* Consume the data. */
          offset += longvalue;
          break;

        case SSH_FORMAT_UINT32_STR_NOCOPY:

          /* Get length and data pointers. */
          cpp = va_arg(ap, const unsigned char **);
          sizep = va_arg(ap, size_t *);

          /* Decode string length and skip the length. */

          if (len - offset < 4)
            goto fail;

          longvalue = SSH_GET_32BIT(buf + offset);
          offset += 4;

          /* Check that the string is all in the buffer. */
          if (longvalue > len - offset)
            goto fail;

          /* Store length if requested. */
          if (sizep != NULL)
            *sizep = longvalue;

          /* Retrieve the data if requested. */
          if (cpp != NULL)
            *cpp = buf + offset;

          /* Consume the data. */
          offset += longvalue;
          break;


        case SSH_FORMAT_BOOLEAN:
          bp = va_arg(ap, Boolean *);
          if (len - offset < 1)
            goto fail;
          if (bp != NULL)
            *bp = buf[offset] != 0;
          offset++;
          break;

        case SSH_FORMAT_UINT32:
          u32p = va_arg(ap, SshUInt32 *);
          if (len - offset < 4)
            goto fail;
          if (u32p)
            *u32p = SSH_GET_32BIT(buf + offset);
          offset += 4;
          break;

        case SSH_FORMAT_UINT16:
          u16p = va_arg(ap, SshUInt16 *);
          if (len - offset < 2)
            goto fail;
          if (u16p)
            *u16p = SSH_GET_16BIT(buf + offset);
          offset += 2;
          break;

        case SSH_FORMAT_CHAR:
          uip = va_arg(ap, unsigned int *);
          if (len - offset < 1)
            goto fail;
          if (uip)
            *uip = buf[offset];
          offset++;
          break;

        case SSH_FORMAT_DATA:
          p = va_arg(ap, unsigned char *);
          size = va_arg(ap, size_t);
          if (len - offset < size)
            goto fail;
          if (p)
            memcpy(p, buf + offset, size);
          offset += size;
          break;

        case SSH_FORMAT_UINT64:
          u64p = va_arg(ap, SshUInt64 *);
          if (len - offset < 8)
            goto fail;
          if (u64p)
            *u64p = SSH_GET_64BIT(buf + offset);
          offset += 8;
          break;

	case SSH_FORMAT_SPECIAL:
	  {
	    SshDecodeDatum fn;
	    void **datump;

	    fn = va_arg(ap, SshDecodeDatum);
	    datump = va_arg(ap, void **);
	    size = (*fn)(buf + offset, len - offset, datump);
	    if (size > len - offset)
	      goto fail;
	    offset += size;
	  }
	  break;

        case SSH_FORMAT_END:
          /* Free the allocs array. */
          if (num_allocs > 0)
            ssh_free(allocs);
          /* Return the number of bytes consumed. */
          return offset;

        default:
          SSH_FATAL("ssh_decode_array_va: invalid format code %d "
                    "(check arguments and SSH_FORMAT_END)",
                    (int)format);
        }
    }

fail:
  /* An error was encountered.  Free all allocated memory and return zero. */
  for (i = 0; i < num_allocs; i++)
    ssh_free(allocs[i]);
  if (i > 0)
    ssh_free(allocs);
  return 0;
}

/* Decodes data from the given byte array as specified by the
   variable-length argument list.  If all specified arguments could be
   successfully parsed, returns the number of bytes parsed (any
   remaining data can be parsed by first skipping this many bytes).
   If parsing any element results in an error, this returns 0 (and
   frees any already allocates data).  Zero is also returned if the
   specified length would be exceeded. */
size_t ssh_decode_array(const unsigned char *buf, size_t len, ...)
{
  va_list ap;
  size_t bytes;

  va_start(ap, len);
  bytes = ssh_decode_array_va(buf, len, ap);
  va_end(ap);

  return bytes;
}
