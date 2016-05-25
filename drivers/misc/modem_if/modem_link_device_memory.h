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
#include <linux/notifier.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#elif defined(CONFIG_FB)
#include <linux/fb.h>
#endif

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

#define CMD_DL_READY		0xA100
#define CMD_DL_START_REQ	0x9200
#define CMD_DL_START_RESP	0xA301
#define CMD_DL_SEND_REQ		0x9400
#define CMD_DL_SEND_RESP	0xA501
#define CMD_DL_DONE_REQ		0x9600
#define CMD_DL_DONE_RESP	0xA701

#define CMD_UL_RECV_RESP	0x9601
#define CMD_UL_RECV_DONE_REQ	0xA700
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

#define INT_MASK_REQ_ACK_SET \
	(INT_MASK_REQ_ACK_F | INT_MASK_REQ_ACK_R | INT_MASK_REQ_ACK_RFS)

#define INT_MASK_RES_ACK_SET \
	(INT_MASK_RES_ACK_F | INT_MASK_RES_ACK_R | INT_MASK_RES_ACK_RFS)

#define INT_CMD_MASK(x)			((x) & 0xF)
#define INT_CMD_INIT_START		0x1
#define INT_CMD_INIT_END		0x2
#define INT_CMD_REQ_ACTIVE		0x3
#define INT_CMD_RES_ACTIVE		0x4
#define INT_CMD_REQ_TIME_SYNC		0x5
#define INT_CMD_CRASH_RESET		0x7
#define INT_CMD_PHONE_START		0x8
#define INT_CMD_ERR_DISPLAY		0x9
#define INT_CMD_CRASH_EXIT		0x9
#define INT_CMD_CP_DEEP_SLEEP		0xA
#define INT_CMD_NV_REBUILDING		0xB
#define INT_CMD_EMER_DOWN		0xC
#define INT_CMD_PIF_INIT_DONE		0xD
#define INT_CMD_SILENT_NV_REBUILDING	0xE
#define INT_CMD_NORMAL_POWER_OFF	0xF

/* AP_IDPRAM PM control command with QSC6085 */
#define INT_CMD_IDPRAM_SUSPEND_REQ	0xD
#define INT_CMD_IDPRAM_SUSPEND_ACK	0xB
#define INT_CMD_IDPRAM_WAKEUP_START	0xE
#define INT_CMD_IDPRAM_RESUME_REQ	0xC

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

#define IDPRAM_SUSPEND_REQ_TIMEOUT	(50 * HZ)

#define RES_ACK_WAIT_TIMEOUT	10		/* 10 ms */
#define REQ_ACK_DELAY		10		/* 10 ms */

#ifdef DEBUG_MODEM_IF
#define MAX_RETRY_CNT	1
#else
#define MAX_RETRY_CNT	3
#endif

#define MAX_SKB_TXQ_DEPTH	1024

struct memif_boot_map {
	u32 __iomem *magic;
	u8  __iomem *buff;
	u32 __iomem *req;
	u32 __iomem *resp;
	u32          space;
};

struct memif_dload_map {
	u32 __iomem *magic;
	u8  __iomem *buff;
	u32 space;
};

struct memif_uload_map {
	u32 __iomem *magic;
	u8  __iomem *cmd;
	u32 cmd_size;
	u8  __iomem *buff;
	u32 space;
};

#define DP_BOOT_BUFF_OFFSET	4
#define DP_DLOAD_MAGIC_SIZE	4
#define DP_DLOAD_BUFF_OFFSET	4
#define DP_ULOAD_MAGIC_SIZE	4
#define DP_ULOAD_BUFF_OFFSET	4
#define DP_BOOT_REQ_OFFSET	0
#define DP_BOOT_RESP_OFFSET	8
#define DP_MBX_SET_SIZE		4
#define DP_MAX_PAYLOAD_SIZE	0x2000

enum circ_dir_type {
	TX,
	RX,
	MAX_DIR,
};

enum circ_ptr_type {
	HEAD,
	TAIL,
};

static inline bool circ_valid(u32 qsize, u32 in, u32 out)
{
	if (in >= qsize)
		return false;

	if (out >= qsize)
		return false;

	return true;
}

static inline u32 circ_get_space(u32 qsize, u32 in, u32 out)
{
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline u32 circ_get_usage(u32 qsize, u32 in, u32 out)
{
	return (in >= out) ? (in - out) : (qsize - out + in);
}

static inline u32 circ_new_pointer(u32 qsize, u32 p, u32 len)
{
	p += len;
	return (p < qsize) ? p : (p - qsize);
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
static inline void circ_read(void *dst, void *src, u32 qsize, u32 out, u32 len)
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
static inline void circ_write(void *dst, void *src, u32 qsize, u32 in, u32 len)
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

/**
 * get_dir_str
 * @dir: communication direction (enum circ_dir_type)
 *
 * Returns the direction of a circular queue
 *
 */
static const inline char *get_dir_str(enum circ_dir_type dir)
{
	if (dir == TX)
		return "AP->CP";
	else
		return "CP->AP";
}

void memcpy16_from_io(const void *to, const void __iomem *from, u32 count);
void memcpy16_to_io(const void __iomem *to, const void *from, u32 count);
int memcmp16_to_io(const void __iomem *to, const void *from, u32 count);
void circ_read16_from_io(void *dst, void *src, u32 qsize, u32 out, u32 len);
void circ_write16_to_io(void *dst, void *src, u32 qsize, u32 in, u32 len);
int copy_circ_to_user(void __user *dst, void *src, u32 qsize, u32 out, u32 len);
int copy_user_to_circ(void *dst, void __user *src, u32 qsize, u32 in, u32 len);

#define MAX_MEM_LOG_CNT	8192
#define MAX_TRACE_SIZE	1024

struct mem_status {
	/* Timestamp */
	struct timespec ts;

	/* Direction (TX or RX) */
	enum circ_dir_type dir;

	/* The status of memory interface at the time */
	u32 magic;
	u32 access;

	u32 head[MAX_IPC_DEV][MAX_DIR];
	u32 tail[MAX_IPC_DEV][MAX_DIR];

	u16 int2ap;
	u16 int2cp;
};

struct mem_status_queue {
	spinlock_t lock;
	u32 in;
	u32 out;
	struct mem_status stat[MAX_MEM_LOG_CNT];
};

struct circ_status {
	u8 *buff;
	u32 qsize;	/* the size of a circular buffer */
	u32 in;
	u32 out;
	u32 size;	/* the size of free space or received data */
};

struct trace_data {
	struct timespec ts;
	enum dev_format dev;
	struct circ_status circ_stat;
	u8 *data;
	u32 size;
};

struct trace_data_queue {
	spinlock_t lock;
	u32 in;
	u32 out;
	struct trace_data trd[MAX_TRACE_SIZE];
};

struct mem_status *msq_get_free_slot(struct mem_status_queue *msq);
struct mem_status *msq_get_data_slot(struct mem_status_queue *msq);

void print_mem_status(struct link_device *ld, struct mem_status *mst);
void print_circ_status(struct link_device *ld, int dev, struct mem_status *mst);
void print_ipc_trace(struct link_device *ld, int dev, struct circ_status *stat,
			struct timespec *ts, u8 *buff, u32 rcvd);

u8 *capture_mem_dump(struct link_device *ld, u8 *base, u32 size);
struct trace_data *trq_get_free_slot(struct trace_data_queue *trq);
struct trace_data *trq_get_data_slot(struct trace_data_queue *trq);

#endif

