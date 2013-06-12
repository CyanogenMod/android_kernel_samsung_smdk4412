/*
 * control.c
 *
 * send and receive control packet and handle it
 */
#include "headers.h"

u_int control_init(struct net_adapter *adapter)
{
	queue_init_list(adapter->ctl.q_received.head);
	spin_lock_init(&adapter->ctl.q_received.lock);

	queue_init_list(adapter->ctl.apps.process_list);
	spin_lock_init(&adapter->ctl.apps.lock);

	adapter->ctl.apps.ready = TRUE;

	return STATUS_SUCCESS;
}

void control_remove(struct net_adapter *adapter)
{
	struct buffer_descriptor	*dsc;
	struct process_descriptor	*process;
	unsigned long			flags;
	/* Free the received control packets queue */
	while (!queue_empty(adapter->ctl.q_received.head)) {
		/* queue is not empty */
		dump_debug("Freeing Control Receive Queue");
		dsc = (struct buffer_descriptor *)
			queue_get_head(adapter->ctl.q_received.head);
		if (!dsc) {
			dump_debug("Fail...node is null");
			continue;
		}
		queue_remove_head(adapter->ctl.q_received.head);
		if (dsc->buffer)
			kfree(dsc->buffer);
		if (dsc)
			kfree(dsc);
	}

	/* process list */
	if (adapter->ctl.apps.ready) {
		if (!queue_empty(adapter->ctl.apps.process_list)) {
			/* first time gethead needed to get the dsc nodes */
			process = (struct process_descriptor *)queue_get_head(adapter->ctl.apps.process_list);
			spin_lock_irqsave(&adapter->ctl.apps.lock, flags);
			while (process != NULL) {
			if (process->irp) {
				process->irp = FALSE;
				wake_up_interruptible(&process->read_wait);
			}
				process = (struct process_descriptor *)process->node.next;
			dump_debug("sangam dbg : waking processes");
			}
			spin_unlock_irqrestore(&adapter->ctl.apps.lock, flags);
			/* delay for the process release */
			msleep(100);
		}
	}
	adapter->ctl.apps.ready = FALSE;
}

/* add received packet to pending list */
static void control_enqueue_buffer(struct net_adapter *adapter,
		u_short type, void *buffer, u_long length)
{
	struct buffer_descriptor	*dsc;
	struct process_descriptor	*process;

	/* Queue and wake read only if process exist. */
	process = process_by_type(adapter, type);
	if (process) {
		dsc = (struct buffer_descriptor *)
			kmalloc(sizeof(struct buffer_descriptor),
					 GFP_ATOMIC);
		if (dsc == NULL) {
			dump_debug("dsc Memory Alloc Failure *****");
			return;
		}
		dsc->buffer = kmalloc(length, GFP_ATOMIC);
		if (dsc->buffer == NULL) {
			kfree(dsc);
			dump_debug("dsc->buffer Memory Alloc Failure *****");
			return;
		}

		memcpy(dsc->buffer, buffer, length);
		/* fill out descriptor */
		dsc->length = length;
		dsc->type = type;
		/* add to pending list */
		queue_put_tail(adapter->ctl.q_received.head, dsc->node)

			if (process->irp) {
				process->irp = FALSE;
				wake_up_interruptible(&process->read_wait);
			}
	} else
		dump_debug("Waiting process not found skip the packet");
}

/* receive control data */
void control_recv(struct net_adapter *adapter, void *buffer, u_long length)
{
	struct eth_header	*hdr;

	/* dump rx control frame */
	if (adapter->pdata->g_cfg->enable_dump_msg == 1)
		dump_buffer("Control Rx", (u_char *)buffer + 12, length - 12);

	/* check halt flag */
	if (adapter->halted)
		return;

	hdr = (struct eth_header *)buffer;

	/* not found, add to pending buffers list */
	spin_lock(&adapter->ctl.q_received.lock);
	control_enqueue_buffer(adapter, hdr->type, buffer, length);
	spin_unlock(&adapter->ctl.q_received.lock);
}

u_int control_send(struct net_adapter *adapter, void *buffer, u_long length)
{
	if ((length + sizeof(struct hw_packet_header)) >= WIMAX_MAX_TOTAL_SIZE)
		return STATUS_RESOURCES;
		/* changed from SUCCESS return status */

	return hw_send_data(adapter, buffer, length, CONTROL_PACKET);

}

struct process_descriptor *process_by_id(struct net_adapter *adapter, u_int id)
{
	struct process_descriptor	*process;

	if (queue_empty(adapter->ctl.apps.process_list)) {
		dump_debug("process_by_id: Empty process list");
		return NULL;
	}

	/* first time gethead needed to get the dsc nodes */
	process = (struct process_descriptor *)
			queue_get_head(adapter->ctl.apps.process_list);
	while (process != NULL)	{
		if (process->id == id)	/* process found */
			return process;
		process = (struct process_descriptor *)process->node.next;
	}
	dump_debug("process_by_id: process not found");

	return NULL;
}

struct process_descriptor *process_by_type(struct net_adapter *adapter,
					 u_short type)
{
	struct process_descriptor	*process;

	if (queue_empty(adapter->ctl.apps.process_list)) {
		dump_debug("process_by_type: Empty process list");
		return NULL;
	}

	/* first time gethead needed to get the dsc nodes */
	process = (struct process_descriptor *)
			queue_get_head(adapter->ctl.apps.process_list);
	while (process != NULL)	{
		if (process->type == type)	/* process found */
			return process;
		process = (struct process_descriptor *)process->node.next;
	}
	dump_debug("process_by_type: process not found");

	return NULL;
}

void remove_process(struct net_adapter *adapter, u_int id)
{
	struct process_descriptor	*curElem;
	struct process_descriptor	*prevElem = NULL;

	if (queue_empty(adapter->ctl.apps.process_list))
		return;

	/* first time get head needed to get the dsc nodes */
	curElem = (struct process_descriptor *)
			queue_get_head(adapter->ctl.apps.process_list);

	for ( ; curElem != NULL; prevElem = curElem,
		 curElem  = (struct process_descriptor *)
			curElem->node.next) {
		if (curElem->id == id) {	/* process found */
			if (prevElem == NULL) {
				/* only one node present */
			(adapter->ctl.apps.process_list).next =
				 (((struct list_head *)curElem)->next);
			if (!((adapter->ctl.apps.process_list).next)) {
				/*rechain list pointer to next link*/
			dump_debug("sangam dbg first and only process delete");
				(adapter->ctl.apps.process_list).prev = NULL;
			}
			} else if (((struct list_head *)curElem)->next ==
							NULL) {
				/* last node */
				dump_debug("sangam dbg only last packet");
				((struct list_head *)prevElem)->next = NULL;
				(adapter->ctl.apps.process_list).prev =
					 (struct list_head *)(prevElem);
			} else {
				/* middle node */
				dump_debug("sangam dbg middle node");
				((struct list_head *)prevElem)->next =
					 ((struct list_head *)curElem)->next;
			}

			kfree(curElem);
			break;
		}
	}
}

/* find buffer by buffer type */
struct buffer_descriptor *buffer_by_type(struct list_head ListHead,
				 u_short type)
{
	struct buffer_descriptor	*dsc;

	if (queue_empty(ListHead))
		return NULL;

	/* first time gethead needed to get the dsc nodes */
	dsc = (struct buffer_descriptor *)queue_get_head(ListHead);
	while (dsc != NULL) {
		if (dsc->type == type)	/* process found */
			return dsc;
		dsc = (struct buffer_descriptor *)dsc->node.next;
	}

	return NULL;
}

void dump_buffer(const char *desc, u_char *buffer, u_int len)
{
	char	print_buf[256] = {0};
	char	chr[8] = {0};
	int	i;

	/* dump packets */
	u_char  *b = buffer;
	dump_debug("%s (%d) =>", desc, len);

	for (i = 0; i < len; i++) {
		sprintf(chr, " %02x", b[i]);
		strcat(print_buf, chr);
		if (((i + 1) != len) && (i % 16 == 15)) {
			dump_debug(print_buf);
			memset(print_buf, 0x0, 256);
		}
	}
	dump_debug(print_buf);
}
