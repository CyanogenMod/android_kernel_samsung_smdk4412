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
 * linux_ipm.c
 *
 * Linux interceptor kernel to user space messaging.
 *
 */

#include "linux_internal.h"

extern SshInterceptor ssh_interceptor_context;

/************************ Internal utility functions ************************/

/* Use printk instead of SSH_DEBUG macros. */
#ifdef DEBUG_LIGHT
#define SSH_LINUX_IPM_DEBUG(x...) if (net_ratelimit()) printk(KERN_INFO x)
#define SSH_LINUX_IPM_WARN(x...) panic(x)
#endif /* DEBUG_LIGHT */

#ifndef SSH_LINUX_IPM_DEBUG
#define SSH_LINUX_IPM_DEBUG(x...)
#define SSH_LINUX_IPM_WARN(x...) printk(KERN_CRIT x)
#endif /* SSH_LINUX_IPM_DEBUG */

/************************* Ipm message alloc / free *************************/

static void interceptor_ipm_message_free_internal(SshInterceptor interceptor,
						  SshInterceptorIpmMsg msg)
{
  SSH_ASSERT(interceptor != NULL);
  SSH_ASSERT(msg != NULL);

  if (msg->buf)
    ssh_free(msg->buf);
  msg->buf = NULL;

  SSH_LINUX_STATISTICS(interceptor,
  {
    interceptor->stats.ipm_send_queue_len--;
    interceptor->stats.ipm_send_queue_bytes -= (SshUInt64) msg->len;
  });

  if (msg->emergency_mallocated)
    {
      ssh_free(msg);
    }
  else
    {
      msg->next = interceptor->ipm.msg_freelist;
      interceptor->ipm.msg_freelist = msg;
      msg->prev = NULL;
    }
}

void interceptor_ipm_message_free(SshInterceptor interceptor,
				  SshInterceptorIpmMsg msg)
{
  local_bh_disable();
  write_lock(&interceptor->ipm.lock);
  interceptor_ipm_message_free_internal(interceptor, msg);
  write_unlock(&interceptor->ipm.lock);
  local_bh_enable();
}

static SshInterceptorIpmMsg
interceptor_ipm_message_alloc(SshInterceptor interceptor,
			      Boolean reliable,
			      size_t len)
{
  SshInterceptorIpmMsg msg = NULL;

  SSH_ASSERT(interceptor != NULL);

  /* Try to take a message from freelist. */
  if (interceptor->ipm.msg_freelist)
    {
      msg = interceptor->ipm.msg_freelist;
      interceptor->ipm.msg_freelist = msg->next;
    }

#if SSH_LINUX_MAX_IPM_MESSAGES < 2000
#error "SSH_LINUX_MAX_IPM_MESSAGES is too low"
#endif /* SSH_LINUX_MAX_IPM_MESSAGES */

  /* Try to allocate a new message. */
  else if (interceptor->ipm.msg_allocated < SSH_LINUX_MAX_IPM_MESSAGES)
    {
      msg = ssh_calloc(1, sizeof(*msg));
      if (msg != NULL)
	{
	  interceptor->ipm.msg_allocated++;
	}
    }

  /* Try to reuse last unreliable message in send queue. */
  if (msg == NULL && reliable == TRUE)
    {
      /* This is a reliable message, reuse last unreliable message. */
      if (interceptor->ipm.send_queue_num_unreliable > 0)
        {
          for (msg = interceptor->ipm.send_queue_tail;
               msg != NULL;
               msg = msg->prev)
            {
              if (msg->reliable == 0)
                {
                  if (msg->next != NULL)
                    msg->next->prev = msg->prev;

                  if (msg->prev != NULL)
                    msg->prev->next = msg->next;

                  if (msg == interceptor->ipm.send_queue)
                    interceptor->ipm.send_queue = msg->next;

                  if (msg == interceptor->ipm.send_queue_tail)
                    interceptor->ipm.send_queue_tail = msg->prev;

                  SSH_ASSERT(interceptor->ipm.send_queue_num_unreliable > 0);
                  interceptor->ipm.send_queue_num_unreliable--;

                  SSH_LINUX_STATISTICS(interceptor,
	          {
                    interceptor->stats.ipm_send_queue_len--;
                    interceptor->stats.ipm_send_queue_bytes
                      -= (SshUInt64) msg->len;
                  });

                  ssh_free(msg->buf);
                  break;
                }
            }
        }

      /* Last resort, malloc message. */
      if (msg == NULL)
	{
	  msg = ssh_calloc(1, sizeof(*msg));
	  if (msg != NULL)
	    msg->emergency_mallocated = 1;
	}
    }

  if (msg)
    SSH_LINUX_STATISTICS(interceptor,
    {
      interceptor->stats.ipm_send_queue_len++;
      interceptor->stats.ipm_send_queue_bytes += (SshUInt64) len;
    });

  return msg;
}

void interceptor_ipm_message_freelist_uninit(SshInterceptor interceptor)
{
  SshInterceptorIpmMsg msg;
  int freelist_len;

  local_bh_disable();
  write_lock(&interceptor->ipm.lock);

  SSH_ASSERT(atomic_read(&interceptor->ipm.open) == 0);

  while (interceptor->ipm.msg_freelist != NULL)
    {
      msg = interceptor->ipm.msg_freelist;
      interceptor->ipm.msg_freelist = msg->next;
      SSH_ASSERT(msg->buf == NULL);
      ssh_free(msg);
      interceptor->ipm.msg_allocated--;
    }

  freelist_len = interceptor->ipm.msg_allocated;

  write_unlock(&interceptor->ipm.lock);
  local_bh_enable();

  if (freelist_len)
    SSH_LINUX_IPM_WARN("Memory leak detected: %d ipm messages leaked!\n",
		       freelist_len);
}


/***************************** Process message from ipm ********************/

ssize_t ssh_interceptor_receive_from_ipm(unsigned char *data, size_t len)
{
  SshUInt32 msg_len;
  SshUInt8 msg_type;

  /* Need a complete header. */
  if (len < 5)
    return 0;

  /* Parse message header. */
  msg_len = SSH_GET_32BIT(data) - 1;
  msg_type = SSH_GET_8BIT(data + 4);

  /* Need a complete message. */
  if (msg_len > (len - 5))
    return 0;

  /* Pass message to engine. */
  local_bh_disable();

  ssh_engine_packet_from_ipm(ssh_interceptor_context->engine,
			     msg_type, data + 5, msg_len);

  local_bh_enable();

  return msg_len + 5;
}


/***************************** Send to ipm *********************************/

Boolean ssh_interceptor_send_to_ipm(unsigned char *data, size_t len,
				    Boolean reliable, void *machine_context)
{
  SshInterceptorIpmMsg msg = NULL;

  local_bh_disable();
  write_lock(&ssh_interceptor_context->ipm.lock);

  /* Check ipm channel status */
  if (atomic_read(&ssh_interceptor_context->ipm.open) == 0)
    {
      write_unlock(&ssh_interceptor_context->ipm.lock);
      local_bh_enable();
      ssh_free(data);
      SSH_LINUX_IPM_DEBUG("ipm channel closed, dropping ipm message len %d\n",
			  (int) len);
      return FALSE;
    }

  /* Allocate a message. */
  msg = interceptor_ipm_message_alloc(ssh_interceptor_context, reliable, len);
  if (msg == NULL)
    {
      write_unlock(&ssh_interceptor_context->ipm.lock);
      local_bh_enable();

      if (reliable)
	SSH_LINUX_IPM_WARN("Dropping reliable ipm message type %d len %d\n",
			   (int) (len < 5 ? -1 : data[4]), (int) len);
      else
	SSH_LINUX_IPM_DEBUG("Dropping unreliable ipm message type %d "
			    "len %d\n",
			    (int) (len < 5 ? -1 : data[4]), (int) len);
      ssh_free(data);
      return FALSE;
    }

  /* Fill message structure. */
  msg->buf = data;
  msg->len = len;
  msg->offset = 0;
  if (reliable)
    msg->reliable = 1;

  /* Append message to send queue tail. */
  msg->prev = ssh_interceptor_context->ipm.send_queue_tail;
  ssh_interceptor_context->ipm.send_queue_tail = msg;
  msg->next = NULL;
  if (msg->prev)
    msg->prev->next = msg;

  if (ssh_interceptor_context->ipm.send_queue == NULL)
    ssh_interceptor_context->ipm.send_queue = msg;

  if (msg->reliable == 0)
    {
      ssh_interceptor_context->ipm.send_queue_num_unreliable++;
      SSH_ASSERT(ssh_interceptor_context->ipm.send_queue_num_unreliable != 0);
    }

  write_unlock(&ssh_interceptor_context->ipm.lock);
  local_bh_enable();

  /* Wake up reader. */
  wake_up_interruptible(&ssh_interceptor_context->ipm_proc_entry.wait_queue);

  return TRUE;
}


/**************************** Ipm channel open / close **********************/

void interceptor_ipm_open(SshInterceptor interceptor)
{

  local_bh_disable();
  write_lock(&interceptor->ipm.lock);

  /* Assert that send queue is empty */
  SSH_ASSERT(interceptor->ipm.send_queue == NULL);

  /* Mark ipm channel open */
  atomic_set(&interceptor->ipm.open, 1);

  write_unlock(&interceptor->ipm.lock);
  local_bh_enable();
}

void interceptor_ipm_close(SshInterceptor interceptor)
{
  SshInterceptorIpmMsg msg, list;

  local_bh_disable();
  write_lock(&interceptor->ipm.lock);

  /* Mark ipm channel closed */
  atomic_set(&interceptor->ipm.open, 0);

  /* Clear send queue */
  list = interceptor->ipm.send_queue;
  interceptor->ipm.send_queue = NULL;
  interceptor->ipm.send_queue_tail = NULL;

  write_unlock(&interceptor->ipm.lock);
  local_bh_enable();

  /* Free all ipm messages from send queue. */
  while (list != NULL)
    {
      msg = list;
      list = msg->next;
      interceptor_ipm_message_free(interceptor, msg);
    }
}


/***************************** Init / uninit ********************************/

Boolean ssh_interceptor_ipm_init(SshInterceptor interceptor)
{
  /* Initialize ipm structure */
  atomic_set(&interceptor->ipm.open, 0);
  rwlock_init(&interceptor->ipm.lock);

  /* Initialize /proc interface */
  return ssh_interceptor_proc_init(interceptor);
}

void ssh_interceptor_ipm_uninit(SshInterceptor interceptor)
{
  /* Uninit /proc interface */
  ssh_interceptor_proc_uninit(interceptor);

  interceptor_ipm_close(interceptor);

  /* Free ipm messages.*/
  interceptor_ipm_message_freelist_uninit(interceptor);
}
