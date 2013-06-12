/*
 * buffer.h
 *
 * Used q_send for the control and data packet sending, uses the BUFFER_DESCRIPTOR
 * q_received used for the control packet receive, data packet sent directly
 */
#ifndef _WIMAX_BUFFER_H
#define _WIMAX_BUFFER_H

/* structures */
struct buffer_descriptor {
	struct list_head	node;		/* list node */
	void			*buffer;	/* allocated buffer: It is used for copying and transfer */
	u_long			length;		/* current data length */
	u_short			type;		/* buffer type (used for control buffers) */
};

struct queue_info {
	struct list_head	head;		/* list head */
	spinlock_t		lock;		/* protection */
};

/******************************************************************************
 * queue_init_list -- Macro which will initialize a queue to NULL.
 ******************************************************************************/
#define queue_init_list(_L) ((_L).next = (_L).prev = NULL)

/******************************************************************************
 * queue_empty -- Macro which checks to see if a queue is empty.
 ******************************************************************************/
#define queue_empty(_L) (queue_get_head((_L)) == NULL)

/******************************************************************************
 * queue_get_head -- Macro which returns the head of the queue, but does not
 * remove the head from the queue.
 ******************************************************************************/
#define queue_get_head(_L) ((struct list_head *)((_L).next))

/******************************************************************************
 * queue_remove_head -- Macro which removes the head of the head of queue.
 ******************************************************************************/
#define queue_remove_head(_L)	\
{	\
	struct list_head *h;	\
	if ((h = (struct list_head *)(_L).next)) {	/* then fix up our our list to point to next elem */	\
		if (!((_L).next = h->next)) /* rechain list pointer to next link */	\
			/* if the list pointer is null, null out the reverse link */	\
			(_L).prev = NULL;	\
	}	\
}

/******************************************************************************
 * queue_put_tail -- Macro which puts an element at the tail (end) of the queue.
 ******************************************************************************/
#define queue_put_tail(_L,_E) \
{	\
	if ((_L).prev) {	\
		((struct list_head *)(_L).prev)->next = (struct list_head *)(&(_E));	\
		(_L).prev = (struct list_head *)(&(_E));	\
	} else	\
		(_L).next = (_L).prev = (struct list_head *)(&(_E));	\
	(_E).next = NULL;	\
}

/******************************************************************************
 * queue_pop_head -- Macro which  will pop the head off of a queue (list), and
 *                 return it (this differs only from queueremovehead only in the 1st line)
 ******************************************************************************/
#define queue_pop_head(_L) \
{	\
	(struct list_head *) (_L).next;	\
	queue_remove_head(_L);	\
}

#endif	/* _WIMAX_BUFFER_H */
