/*
 * Copyright (C) 2010 Google, Inc.
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

#ifndef __MODEM_LINK_DEVICE_MIPI_H__
#define __MODEM_LINK_DEVICE_MIPI_H__


#define HSI_MAX_CHANNELS	16
#define CHANNEL_MASK	0xFF

#define HSI_CHANNEL_TX_STATE_UNAVAIL	(1 << 0)
#define HSI_CHANNEL_TX_STATE_WRITING	(1 << 1)
#define HSI_CHANNEL_RX_STATE_UNAVAIL	(1 << 0)
#define HSI_CHANNEL_RX_STATE_READING	(1 << 1)

#define HSI_WRITE_DONE_TIMEOUT	(HZ)
#define HSI_READ_DONE_TIMEOUT	(HZ)
#define HSI_ACK_DONE_TIMEOUT	(HZ)
#define HSI_CLOSE_CONN_DONE_TIMEOUT	(HZ)
#define HSI_ACWAKE_DOWN_TIMEOUT	(HZ / 2)

#define HSI_CONTROL_CHANNEL	0
#define HSI_FLASHLESS_CHANNEL	0
#define HSI_CP_RAMDUMP_CHANNEL	0
#define HSI_FMT_CHANNEL	1
#define HSI_RAW_CHANNEL	2
#define HSI_RFS_CHANNEL	3
#define HSI_CMD_CHANNEL	4
#define HSI_NUM_OF_USE_CHANNELS	5

#define HSI_LL_INVALID_CHANNEL	0xFF

#define HSI_FLASHBOOT_ACK_LEN	16
#define DUMP_PACKET_SIZE	12289 /* 48K + 4 length, word unit */
#define DUMP_ERR_INFO_SIZE	39 /* 150 bytes + 4 length , word unit */

enum {
	HSI_LL_MSG_BREAK, /* 0x0 */
	HSI_LL_MSG_ECHO,
	HSI_LL_MSG_INFO_REQ,
	HSI_LL_MSG_INFO,
	HSI_LL_MSG_CONFIGURE,
	HSI_LL_MSG_ALLOCATE_CH,
	HSI_LL_MSG_RELEASE_CH,
	HSI_LL_MSG_OPEN_CONN,
	HSI_LL_MSG_CONN_READY,
	HSI_LL_MSG_CONN_CLOSED, /* 0x9 */
	HSI_LL_MSG_CANCEL_CONN,
	HSI_LL_MSG_ACK, /* 0xB */
	HSI_LL_MSG_NAK, /* 0xC */
	HSI_LL_MSG_CONF_RATE,
	HSI_LL_MSG_OPEN_CONN_OCTET, /* 0xE */
	HSI_LL_MSG_INVALID = 0xFF,
};

enum {
	STEP_UNDEF,
	STEP_CLOSED,
	STEP_NOT_READY,
	STEP_IDLE,
	STEP_ERROR,
	STEP_SEND_OPEN_CONN,
	STEP_SEND_ACK,
	STEP_WAIT_FOR_ACK,
	STEP_TO_ACK,
	STEP_SEND_NACK,
	STEP_GET_NACK,
	STEP_SEND_CONN_READY,
	STEP_WAIT_FOR_CONN_READY,
	STEP_SEND_CONF_RATE,
	STEP_WAIT_FOR_CONF_ACK,
	STEP_TX,
	STEP_RX,
	STEP_SEND_CONN_CLOSED,
	STEP_WAIT_FOR_CONN_CLOSED,
	STEP_SEND_BREAK,
};


struct if_hsi_channel {
	struct hsi_device *dev;
	unsigned int channel_id;

	u32 *tx_data;
	unsigned int tx_count;
	u32 *rx_data;
	unsigned int rx_count;
	unsigned int packet_size;

	unsigned int tx_state;
	unsigned int rx_state;
	spinlock_t tx_state_lock;
	spinlock_t rx_state_lock;

	unsigned int send_step;
	unsigned int recv_step;

	unsigned int got_nack;
	unsigned int acwake;
	spinlock_t acwake_lock;

	struct semaphore write_done_sem;
	struct semaphore ack_done_sem;
	struct semaphore close_conn_done_sem;

	unsigned int opened;
};

struct if_hsi_command {
	u32 command;
	struct list_head list;
};

struct mipi_link_device {
	struct link_device ld;

	/* mipi specific link data */
	struct if_hsi_channel hsi_channles[HSI_MAX_CHANNELS];
	struct list_head list_of_hsi_cmd;
	spinlock_t list_cmd_lock;

	struct workqueue_struct *mipi_wq;
	struct work_struct cmd_work;
	struct delayed_work start_work;

	struct wake_lock wlock;
	struct timer_list hsi_acwake_down_timer;
};
/* converts from struct link_device* to struct xxx_link_device* */
#define to_mipi_link_device(linkdev) \
			container_of(linkdev, struct mipi_link_device, ld)


enum {
	HSI_INIT_MODE_NORMAL,
	HSI_INIT_MODE_FLASHLESS_BOOT,
	HSI_INIT_MODE_CP_RAMDUMP,
};
static int hsi_init_handshake(struct mipi_link_device *mipi_ld, int mode);
static int if_hsi_write(struct if_hsi_channel *channel, u32 *data,
			unsigned int size);
static int if_hsi_protocol_send(struct mipi_link_device *mipi_ld, int ch,
			u32 *data, unsigned int len);
static int if_hsi_close_channel(struct if_hsi_channel *channel);

#endif
