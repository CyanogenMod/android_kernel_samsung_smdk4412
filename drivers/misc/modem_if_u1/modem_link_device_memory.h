/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MODEM_LINK_DEVICE_MEMORY_H__
#define __MODEM_LINK_DEVICE_MEMORY_H__

#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/platform_data/modem.h>

#include "modem_prj.h"

#define DPRAM_MAGIC_CODE	0xAA

/* interrupt masks.*/
#define INT_MASK_VALID		0x0080
#define INT_MASK_CMD		0x0040
#define INT_VALID(x)		((x) & INT_MASK_VALID)
#define INT_CMD_VALID(x)	((x) & INT_MASK_CMD)
#define INT_NON_CMD(x)		(INT_MASK_VALID | (x))
#define INT_CMD(x)		(INT_MASK_VALID | INT_MASK_CMD | (x))

#define EXT_UDL_MASK		0xF000
#define EXT_UDL_CMD(x)		((x) & EXT_UDL_MASK)
#define EXT_INT_VALID_MASK	0x8000
#define EXT_CMD_VALID_MASK	0x4000
#define UDL_CMD_VALID_MASK	0x2000
#define EXT_INT_VALID(x)	((x) & EXT_INT_VALID_MASK)
#define EXT_CMD_VALID(x)	((x) & EXT_CMD_VALID_MASK)
#define UDL_CMD_VALID(x)	((x) & UDL_CMD_VALID_MASK)
#define INT_EXT_CMD(x)		(EXT_INT_VALID_MASK | EXT_CMD_VALID_MASK | (x))

#define EXT_CMD_MASK(x)		((x) & 0x0FFF)
#define EXT_CMD_SET_SPEED_LOW	0x0011
#define EXT_CMD_SET_SPEED_MID	0x0012
#define EXT_CMD_SET_SPEED_HIGH	0x0013

#define UDL_RESULT_SUCCESS	0x1
#define UDL_RESULT_FAIL		0x2

#define UDL_CMD_MASK(x)		(((x) >> 8) & 0xF)
#define UDL_CMD_RECV_READY	0x1
#define UDL_CMD_DL_START_REQ	0x2
#define UDL_CMD_DL_START_RESP	0x3
#define UDL_CMD_IMAGE_SEND_REQ	0x4
#define UDL_CMD_SEND_DONE_RESP	0x5
#define UDL_CMD_SEND_DONE_REQ	0x6
#define UDL_CMD_UPDATE_DONE	0x7
#define UDL_CMD_STATUS_UPDATE	0x8
#define UDL_CMD_IMAGE_SEND_RESP	0x9
#define UDL_CMD_EFS_CLEAR_RESP	0xB
#define UDL_CMD_ALARM_BOOT_OK	0xC
#define UDL_CMD_ALARM_BOOT_FAIL	0xD

#define CMD_IMG_START_REQ	0x9200
#define CMD_IMG_SEND_REQ	0x9400
#define CMD_DL_SEND_DONE_REQ	0x9600
#define CMD_UL_RECV_RESP	0x9601
#define CMD_UL_RECV_DONE_RESP	0x9801

/* special interrupt cmd indicating modem boot failure. */
#define INT_POWERSAFE_FAIL	0xDEAD

#define INT_MASK_REQ_ACK_F	0x0020
#define INT_MASK_REQ_ACK_R	0x0010
#define INT_MASK_RES_ACK_F	0x0008
#define INT_MASK_RES_ACK_R	0x0004
#define INT_MASK_SEND_F		0x0002
#define INT_MASK_SEND_R		0x0001

#define INT_MASK_REQ_ACK_RFS	0x0400 /* Request RES_ACK_RFS		*/
#define INT_MASK_RES_ACK_RFS	0x0200 /* Response of REQ_ACK_RFS	*/
#define INT_MASK_SEND_RFS	0x0100 /* Indicate sending RFS data	*/

#define INT_MASK_RES_ACK_SET \
	(INT_MASK_RES_ACK_F | INT_MASK_RES_ACK_R | INT_MASK_RES_ACK_RFS)

#define INT_MASK_SEND_SET \
	(INT_MASK_SEND_F | INT_MASK_SEND_R | INT_MASK_SEND_RFS)

#define INT_CMD_MASK(x)		((x) & 0xF)
#define INT_CMD_INIT_START	0x1
#define INT_CMD_INIT_END	0x2
#define INT_CMD_REQ_ACTIVE	0x3
#define INT_CMD_RES_ACTIVE	0x4
#define INT_CMD_REQ_TIME_SYNC	0x5
#define INT_CMD_CRASH_RESET	0x7
#define INT_CMD_PHONE_START	0x8
#define INT_CMD_ERR_DISPLAY	0x9
#define INT_CMD_CRASH_EXIT	0x9
#define INT_CMD_CP_DEEP_SLEEP	0xA
#define INT_CMD_NV_REBUILDING	0xB
#define INT_CMD_EMER_DOWN	0xC
#define INT_CMD_PIF_INIT_DONE	0xD
#define INT_CMD_SILENT_NV_REBUILDING	0xE
#define INT_CMD_NORMAL_PWR_OFF	0xF

#define START_FLAG		0x7F
#define END_FLAG		0x7E

#define DP_MAGIC_DMDL		0x4445444C
#define DP_MAGIC_UMDL		0x4445444D
#define DP_DPRAM_SIZE		0x4000
#define DP_DEFAULT_WRITE_LEN	8168
#define DP_DEFAULT_DUMP_LEN	16128
#define DP_DUMP_HEADER_SIZE	7

#define UDL_TIMEOUT		(50 * HZ)
#define UDL_SEND_TIMEOUT	(200 * HZ)
#define FORCE_CRASH_ACK_TIMEOUT	(5 * HZ)
#define DUMP_TIMEOUT		(30 * HZ)
#define DUMP_START_TIMEOUT	(100 * HZ)
#define DUMP_WAIT_TIMEOUT	(HZ >> 10)	/* 1/1024 second */
#define RES_ACK_WAIT_TIMEOUT	(HZ >> 8)	/* 1/256 second */
#define REQ_ACK_DELAY		(HZ >> 7)	/* 1/128 second */

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
#define MAX_RETRY_CNT	1
#else
#define MAX_RETRY_CNT	3
#endif

#define MAX_SKB_TXQ_DEPTH	1024

enum host_boot_mode {
	HOST_BOOT_MODE_NORMAL,
	HOST_BOOT_MODE_DUMP,
};

enum dpram_init_status {
	DPRAM_INIT_STATE_NONE,
	DPRAM_INIT_STATE_READY,
};

enum circ_dir_type {
	TX,
	RX,
};

enum circ_ptr_type {
	HEAD,
	TAIL,
};

struct dpram_boot_img {
	char *addr;
	int size;
	enum host_boot_mode mode;
	unsigned req;
	unsigned resp;
};

#define MAX_PAYLOAD_SIZE 0x2000
struct dpram_boot_frame {
	unsigned req;		/* AP->CP request		*/
	unsigned resp;		/* response expected by AP	*/
	ssize_t len;		/* data size in the buffer	*/
	unsigned offset;	/* offset to write into DPRAM	*/
	char data[MAX_PAYLOAD_SIZE];
};

/* buffer type for modem image */
struct dpram_dump_arg {
	char *buff;		/* pointer to the buffer	*/
	int buff_size;		/* buffer size			*/
	unsigned req;		/* AP->CP request		*/
	unsigned resp;		/* CP->AP response		*/
	bool cmd;		/* AP->CP command		*/
};

struct dpram_boot_map {
	u32 __iomem *magic;
	u8  __iomem *buff;
	u32 __iomem *req;
	u32 __iomem *resp;
	u32          size;
};

struct qc_dpram_boot_map {
	u8 __iomem *buff;
	u16 __iomem *frame_size;
	u16 __iomem *tag;
	u16 __iomem *count;
};

struct dpram_dload_map {
	u32 __iomem *magic;
	u8  __iomem *buff;
};

struct dpram_uload_map {
	u32 __iomem *magic;
	u8  __iomem *buff;
};

struct ul_header {
	u8  bop;
	u16 total_frame;
	u16 curr_frame;
	u16 len;
} __packed;

struct dpram_udl_param {
	unsigned char *addr;
	unsigned int size;
	unsigned int count;
	unsigned int tag;
};

struct dpram_udl_check {
	unsigned int total_size;
	unsigned int rest_size;
	unsigned int send_size;
	unsigned int copy_start;
	unsigned int copy_complete;
	unsigned int boot_complete;
};

struct dpram_circ_status {
	u8 *buff;
	unsigned int qsize;	/* the size of a circular buffer */
	unsigned int in;
	unsigned int out;
	int size;		/* the size of free space or received data */
};

#define DP_BOOT_BUFF_OFFSET	4
#define DP_DLOAD_BUFF_OFFSET	4
#define DP_ULOAD_BUFF_OFFSET	4
#define DP_BOOT_REQ_OFFSET	0
#define DP_BOOT_RESP_OFFSET	8

static inline bool circ_valid(u32 qsize, u32 in, u32 out)
{
	if (in >= qsize)
		return false;

	if (out >= qsize)
		return false;

	return true;
}

static inline int circ_get_space(int qsize, int in, int out)
{
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline int circ_get_usage(int qsize, int in, int out)
{
	return (in >= out) ? (in - out) : (qsize - out + in);
}

/**
 * circ_read
 * @dst: start address of the destination buffer
 * @src: start address of the buffer in a circular queue
 * @qsize: size of the circular queue
 * @out: offset to read
 * @len: length of data to be read
 *
 * Should be invoked after checking data length
 */
static inline void circ_read(u8 *dst, u8 __iomem *src, u32 qsize, u32 out,
				u32 len)
{
	unsigned len1;

	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		memcpy(dst, (src + out), len);
	} else {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */

		/* 1) data start (out) ~ buffer end */
		len1 = qsize - out;
		memcpy(dst, (src + out), len1);

		/* 2) buffer start ~ data end (in?) */
		memcpy((dst + len1), src, (len - len1));
	}
}

/**
 * circ_write
 * @dst: pointer to the start of the circular queue
 * @src: pointer to the source
 * @qsize: size of the circular queue
 * @in: offset to write
 * @len: length of data to be written
 *
 * Should be invoked after checking free space
 */
static inline void circ_write(u8 __iomem *dst, u8 *src, u32 qsize, u32 in,
				u32 len)
{
	u32 space;

	if ((in + len) < qsize) {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		memcpy((dst + in), src, len);
	} else {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */

		/* 1) space start (in) ~ buffer end */
		space = qsize - in;
		memcpy((dst + in), src, ((len > space) ? space : len));

		/* 2) buffer start ~ data end */
		if (len > space)
			memcpy(dst, (src + space), (len - space));
	}
}

/**
 * circ_dir
 * @dir: communication direction (enum circ_dir_type)
 *
 * Returns the direction of a circular queue
 *
 */
static const inline char *circ_dir(enum circ_dir_type dir)
{
	if (dir == TX)
		return "TXQ";
	else
		return "RXQ";
}

/**
 * circ_ptr
 * @ptr: circular queue pointer (enum circ_ptr_type)
 *
 * Returns the name of a circular queue pointer
 *
 */
static const inline char *circ_ptr(enum circ_ptr_type ptr)
{
	if (ptr == HEAD)
		return "head";
	else
		return "tail";
}

#if 1
#define DPRAM_MAX_RXBQ_SIZE	256

struct mif_rxb {
	u8 *buff;
	unsigned size;

	u8 *data;
	unsigned len;
};

struct mif_rxb_queue {
	int size;
	int in;
	int out;
	struct mif_rxb *rxb;
};

/*
** RXB (DPRAM RX buffer) functions
*/
static inline struct mif_rxb *rxbq_create_pool(unsigned size, int count)
{
	struct mif_rxb *rxb;
	u8 *buff;
	int i;

	rxb = kzalloc(sizeof(struct mif_rxb) * count, GFP_KERNEL);
	if (!rxb) {
		mif_info("ERR! kzalloc rxb fail\n");
		return NULL;
	}

	buff = kzalloc((size * count), GFP_KERNEL|GFP_DMA);
	if (!buff) {
		mif_info("ERR! kzalloc buff fail\n");
		kfree(rxb);
		return NULL;
	}

	for (i = 0; i < count; i++) {
		rxb[i].buff = buff;
		rxb[i].size = size;
		buff += size;
	}

	return rxb;
}

static inline unsigned rxbq_get_page_size(unsigned len)
{
	return ((len + PAGE_SIZE - 1) >> PAGE_SHIFT) << PAGE_SHIFT;
}

static inline bool rxbq_empty(struct mif_rxb_queue *rxbq)
{
	return (rxbq->in == rxbq->out) ? true : false;
}

static inline int rxbq_free_size(struct mif_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline struct mif_rxb *rxbq_get_free_rxb(struct mif_rxb_queue *rxbq)
{
	struct mif_rxb *rxb = NULL;

	if (likely(rxbq_free_size(rxbq) > 0)) {
		rxb = &rxbq->rxb[rxbq->in];
		rxbq->in++;
		if (rxbq->in >= rxbq->size)
			rxbq->in -= rxbq->size;
		rxb->data = rxb->buff;
	}

	return rxb;
}

static inline int rxbq_size(struct mif_rxb_queue *rxbq)
{
	int in = rxbq->in;
	int out = rxbq->out;
	int qsize = rxbq->size;
	return (in >= out) ? (in - out) : (qsize - out + in);
}

static inline struct mif_rxb *rxbq_get_data_rxb(struct mif_rxb_queue *rxbq)
{
	struct mif_rxb *rxb = NULL;

	if (likely(!rxbq_empty(rxbq))) {
		rxb = &rxbq->rxb[rxbq->out];
		rxbq->out++;
		if (rxbq->out >= rxbq->size)
			rxbq->out -= rxbq->size;
	}

	return rxb;
}

static inline u8 *rxb_put(struct mif_rxb *rxb, unsigned len)
{
	rxb->len = len;
	return rxb->data;
}

static inline void rxb_clear(struct mif_rxb *rxb)
{
	rxb->data = NULL;
	rxb->len = 0;
}
#endif

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
#define MAX_TRACE_SIZE	1024

struct trace_data {
	struct timespec ts;
	enum dev_format dev;
	u8 *data;
	int size;
};

struct trace_queue {
	spinlock_t lock;
	int in;
	int out;
	struct trace_data trd[MAX_TRACE_SIZE];
};

static inline struct trace_data *trq_get_free_slot(struct trace_queue *trq)
{
	int in = trq->in;
	int out = trq->out;
	int qsize = MAX_TRACE_SIZE;
	struct trace_data *trd = NULL;
	unsigned long int flags;

	spin_lock_irqsave(&trq->lock, flags);

	if (circ_get_space(qsize, in, out) < 1) {
		spin_unlock_irqrestore(&trq->lock, flags);
		return NULL;
	}

	trd = &trq->trd[in];

	in++;
	if (in == qsize)
		trq->in = 0;
	else
		trq->in = in;

	spin_unlock_irqrestore(&trq->lock, flags);

	return trd;
}

static inline struct trace_data *trq_get_data_slot(struct trace_queue *trq)
{
	int in = trq->in;
	int out = trq->out;
	int qsize = MAX_TRACE_SIZE;
	struct trace_data *trd = NULL;
	unsigned long int flags;

	spin_lock_irqsave(&trq->lock, flags);

	if (circ_get_usage(qsize, in, out) < 1) {
		spin_unlock_irqrestore(&trq->lock, flags);
		return NULL;
	}

	trd = &trq->trd[out];

	out++;
	if (out == qsize)
		trq->out = 0;
	else
		trq->out = out;

	spin_unlock_irqrestore(&trq->lock, flags);

	return trd;
}
#endif

#endif
