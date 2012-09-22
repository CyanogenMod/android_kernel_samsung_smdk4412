/*****************************************************************************
 Copyright(c) 2010 FCI Inc. All Rights Reserved

 File name : fci_ringbuffer.h

 Description :

 History :
 ----------------------------------------------------------------------
 2010/11/25	aslan.cho	initial
*******************************************************************************/

#ifndef __FCI_RINGBUFFER_H__
#define __FCI_RINGBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/spinlock.h>
#include <linux/wait.h>

struct fci_ringbuffer {
	u8               *data;
	ssize_t           size;
	ssize_t           pread;
	ssize_t           pwrite;
	int               error;

	wait_queue_head_t queue;
	spinlock_t        lock;
};

#define FCI_RINGBUFFER_PKTHDRSIZE 3

extern void fci_ringbuffer_init(struct fci_ringbuffer *rbuf
	, void *data, size_t len);

extern int fci_ringbuffer_empty(struct fci_ringbuffer *rbuf);

extern ssize_t fci_ringbuffer_free(struct fci_ringbuffer *rbuf);

extern ssize_t fci_ringbuffer_avail(struct fci_ringbuffer *rbuf);

extern void fci_ringbuffer_reset(struct fci_ringbuffer *rbuf);

extern void fci_ringbuffer_flush(struct fci_ringbuffer *rbuf);

extern void fci_ringbuffer_flush_spinlock_wakeup(struct fci_ringbuffer *rbuf);

#define FCI_RINGBUFFER_PEEK(rbuf, offs)	\
	((rbuf)->data[((rbuf)->pread+(offs)) % (rbuf)->size])

#define FCI_RINGBUFFER_SKIP(rbuf, num)	\
	((rbuf)->pread = ((rbuf)->pread+(num)) % (rbuf)->size)

extern ssize_t fci_ringbuffer_read_user(struct fci_ringbuffer *rbuf
, u8 __user *buf, size_t len);

extern void fci_ringbuffer_read(struct fci_ringbuffer *rbuf
	, u8 *buf, size_t len);


#define FCI_RINGBUFFER_WRITE_BYTE(rbuf, byte)	\
		{ (rbuf)->data[(rbuf)->pwrite] = (byte); \
			(rbuf)->pwrite = ((rbuf)->pwrite + 1) % (rbuf)->size; }

extern ssize_t fci_ringbuffer_write(struct fci_ringbuffer *rbuf
	, const u8 *buf, size_t len);

extern ssize_t fci_ringbuffer_pkt_write(struct fci_ringbuffer *rbuf
	, u8 *buf, size_t len);

extern ssize_t fci_ringbuffer_pkt_read_user(struct fci_ringbuffer *rbuf
	, size_t idx, int offset, u8 __user *buf, size_t len);

extern ssize_t fci_ringbuffer_pkt_read(struct fci_ringbuffer *rbuf
	, size_t idx, int offset, u8 *buf, size_t len);

extern void fci_ringbuffer_pkt_dispose(struct fci_ringbuffer *rbuf, size_t idx);

extern ssize_t fci_ringbuffer_pkt_next(struct fci_ringbuffer *rbuf
	, size_t idx, size_t *pktlen);

#ifdef __cplusplus
}
#endif

#endif
