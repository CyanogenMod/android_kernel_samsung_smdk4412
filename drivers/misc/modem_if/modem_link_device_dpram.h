/*
 * Copyright (C) 2011 Google, Inc.
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
#ifndef __MODEM_LINK_DEVICE_DPRAM_H__
#define __MODEM_LINK_DEVICE_DPRAM_H__

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

#define INT_MASK_REQ_ACK_RFS	0x0400 /* Request RES_ACK_RFS		*/
#define INT_MASK_RES_ACK_RFS	0x0200 /* Response of REQ_ACK_RFS	*/
#define INT_MASK_SEND_RFS	0x0100 /* Indicate sending RFS data	*/

#define INT_MASK_REQ_ACK_F	0x0020
#define INT_MASK_REQ_ACK_R	0x0010
#define INT_MASK_RES_ACK_F	0x0008
#define INT_MASK_RES_ACK_R	0x0004
#define INT_MASK_SEND_F		0x0002
#define INT_MASK_SEND_R		0x0001

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

enum host_boot_mode {
	HOST_BOOT_MODE_NORMAL,
	HOST_BOOT_MODE_DUMP,
};

enum dpram_init_status {
	DPRAM_INIT_STATE_NONE,
	DPRAM_INIT_STATE_READY,
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

#define DP_BOOT_BUFF_OFFSET	4
#define DP_DLOAD_BUFF_OFFSET	4
#define DP_ULOAD_BUFF_OFFSET	4
#define DP_BOOT_REQ_OFFSET	0
#define DP_BOOT_RESP_OFFSET	8

#define MAX_WQ_NAME_LENGTH	64

#define DPRAM_MAX_RXBQ_SIZE	256

struct dpram_rxb {
	u8 *buff;
	unsigned size;

	u8 *data;
	unsigned len;
};

struct dpram_rxb_queue {
	int size;
	int in;
	int out;
	struct dpram_rxb *rxb;
};

/*
	magic_code +
	access_enable +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +
	raw_rx_head + raw_rx_tail + raw_rx_buff +
	mbx_cp2ap +
	mbx_ap2cp
 =	2 +
	2 +
	2 + 2 + 1336 +
	2 + 2 + 4564 +
	2 + 2 + 1336 +
	2 + 2 + 9124 +
	2 +
	2
 =	16384
*/
#define DP_16K_FMT_TX_BUFF_SZ	1336
#define DP_16K_RAW_TX_BUFF_SZ	4564
#define DP_16K_FMT_RX_BUFF_SZ	1336
#define DP_16K_RAW_RX_BUFF_SZ	9124

struct dpram_ipc_16k_map {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8  fmt_tx_buff[DP_16K_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8  raw_tx_buff[DP_16K_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8  fmt_rx_buff[DP_16K_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8  raw_rx_buff[DP_16K_RAW_RX_BUFF_SZ];

	u16 mbx_cp2ap;
	u16 mbx_ap2cp;
};

#define DP_MAX_NAME_LEN	32

struct dpram_ext_op;

struct dpram_link_device {
	struct link_device ld;

	/* DPRAM address and size */
	u8 __iomem *dp_base;		/* DPRAM base virtual address	*/
	u32 dp_size;			/* DPRAM size			*/
	enum dpram_type dp_type;	/* DPRAM type			*/

	/* DPRAM IRQ GPIO# */
	unsigned gpio_dpram_int;

	/* DPRAM IRQ from CP */
	int irq;
	unsigned long irq_flags;
	char irq_name[DP_MAX_NAME_LEN];

	/* Link to DPRAM control functions dependent on each platform */
	int max_ipc_dev;
	struct modemlink_dpram_control *dpctl;

	/* Physical configuration -> logical configuration */
	union {
		struct dpram_boot_map bt_map;
		struct qc_dpram_boot_map qc_bt_map;
	};

	struct dpram_dload_map dl_map;
	struct dpram_uload_map ul_map;

	/* IPC device map */
	struct dpram_ipc_map ipc_map;

	/* Pointers (aliases) to IPC device map */
	u16 __iomem *magic;
	u16 __iomem *access;
	struct dpram_ipc_device *dev[MAX_IPC_DEV];
	u16 __iomem *mbx2ap;
	u16 __iomem *mbx2cp;

	/* Wakelock for DPRAM device */
	struct wake_lock wlock;
	char wlock_name[DP_MAX_NAME_LEN];

	/* For booting */
	unsigned boot_start_complete;
	struct completion dpram_init_cmd;
	struct completion modem_pif_init_done;

	/* For UDL */
	struct tasklet_struct ul_tsk;
	struct tasklet_struct dl_tsk;
	struct completion udl_start_complete;
	struct completion udl_cmd_complete;
	struct dpram_udl_check udl_check;
	struct dpram_udl_param udl_param;

	/* For CP RAM dump */
	struct timer_list crash_ack_timer;
	struct completion dump_start_complete;
	struct completion dump_recv_done;
	struct timer_list dump_timer;
	int dump_rcvd;		/* Count of dump packets received */

	/* For locking TX process */
	spinlock_t tx_lock[MAX_IPC_DEV];

	/* For efficient RX process */
	struct tasklet_struct rx_tsk;
	struct dpram_rxb_queue rxbq[MAX_IPC_DEV];
	struct io_device *iod[MAX_IPC_DEV];
	bool use_skb;
	struct sk_buff_head skb_rxq[MAX_IPC_DEV];

	/* For retransmission after buffer full state */
	atomic_t res_required[MAX_IPC_DEV];

	/* For wake-up/sleep control */
	atomic_t accessing;

	/* Multi-purpose miscellaneous buffer */
	u8 *buff;

	/* DPRAM IPC initialization status */
	int dpram_init_status;

	/* Alias to device-specific IOCTL function */
	int (*ext_ioctl)(struct dpram_link_device *dpld, struct io_device *iod,
			unsigned int cmd, unsigned long arg);

	/* Alias to DPRAM IRQ handler */
	irqreturn_t (*irq_handler)(int irq, void *data);

	/* For DPRAM dump */
	void (*dpram_dump)(struct link_device *ld, char *buff);

	/* Common operations for each DPRAM */
	void (*clear_intr)(struct dpram_link_device *dpld);
	u16 (*recv_intr)(struct dpram_link_device *dpld);
	void (*send_intr)(struct dpram_link_device *dpld, u16 mask);
	u16 (*get_magic)(struct dpram_link_device *dpld);
	void (*set_magic)(struct dpram_link_device *dpld, u16 value);
	u16 (*get_access)(struct dpram_link_device *dpld);
	void (*set_access)(struct dpram_link_device *dpld, u16 value);
	u32 (*get_tx_head)(struct dpram_link_device *dpld, int id);
	u32 (*get_tx_tail)(struct dpram_link_device *dpld, int id);
	void (*set_tx_head)(struct dpram_link_device *dpld, int id, u32 head);
	void (*set_tx_tail)(struct dpram_link_device *dpld, int id, u32 tail);
	u8 *(*get_tx_buff)(struct dpram_link_device *dpld, int id);
	u32 (*get_tx_buff_size)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_head)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_tail)(struct dpram_link_device *dpld, int id);
	void (*set_rx_head)(struct dpram_link_device *dpld, int id, u32 head);
	void (*set_rx_tail)(struct dpram_link_device *dpld, int id, u32 tail);
	u8 *(*get_rx_buff)(struct dpram_link_device *dpld, int id);
	u32 (*get_rx_buff_size)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_req_ack)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_res_ack)(struct dpram_link_device *dpld, int id);
	u16 (*get_mask_send)(struct dpram_link_device *dpld, int id);
	void (*ipc_rx_handler)(struct dpram_link_device *dpld, u16 int2ap);

	/* Extended operations for various modems */
	struct dpram_ext_op *ext_op;
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_dpram_link_device(linkdev) \
		container_of(linkdev, struct dpram_link_device, ld)

struct dpram_ext_op {
	int exist;

	void (*init_boot_map)(struct dpram_link_device *dpld);
	void (*init_dl_map)(struct dpram_link_device *dpld);
	void (*init_ul_map)(struct dpram_link_device *dpld);

	int (*download_binary)(struct dpram_link_device *dpld,
		struct sk_buff *skb);

	void (*cp_start_handler)(struct dpram_link_device *dpld);

	void (*crash_log)(struct dpram_link_device *dpld);
	int (*dump_start)(struct dpram_link_device *dpld);
	int (*dump_update)(struct dpram_link_device *dpld, void *arg);

	int (*ioctl)(struct dpram_link_device *dpld, struct io_device *iod,
		unsigned int cmd, unsigned long arg);

	irqreturn_t (*irq_handler)(int irq, void *data);
};

struct dpram_ext_op *dpram_get_ext_op(enum modem_t modem);

#endif
