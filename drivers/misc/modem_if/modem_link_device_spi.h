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
#ifndef __MODEM_LINK_DEVICE_SPI_H__
#define __MODEM_LINK_DEVICE_SPI_H__

#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/platform_data/modem.h>


#define SPI_TIMER_TX_WAIT_TIME 60 /* ms */
#define SPI_TIMER_RX_WAIT_TIME 500 /* ms */

#define SPI_MAX_PACKET_SIZE (2048 * 6)
#define SPI_TASK_EVENT_COUNT	64
#define SPI_DATA_BOF			0x7F
#define SPI_DATA_EOF			0x7E
#define SPI_DATA_FF_PADDING_HEADER 0xFFFFFFFF

#define SPI_DATA_MUX_NORMAL_MASK	0x0F
#define SPI_DATA_MUX_MORE_H			0x10
#define SPI_DATA_MUX_MORE_M			0x20
#define SPI_DATA_MUX_MORE_T			0x30

#define SPI_DATA_MUX_SIZE		2
#define SPI_DATA_LENGTH_SIZE	4
#define SPI_DATA_BOF_SIZE		1
#define SPI_DATA_INNER_LENGTH_SIZE	4
#define SPI_DATA_IPC_INNER_LENGTH_SIZE	2
#define SPI_DATA_IPC_INNER_CONTROL_SIZE	1
#define SPI_DATA_EOF_SIZE		1
#define SPI_DATA_HEADER_SIZE	(SPI_DATA_MUX_SIZE+ \
		SPI_DATA_LENGTH_SIZE+SPI_DATA_BOF_SIZE+SPI_DATA_EOF_SIZE)
#define SPI_DATA_HEADER_SIZE_FRONT	(SPI_DATA_MUX_SIZE+ \
		SPI_DATA_LENGTH_SIZE+SPI_DATA_BOF_SIZE)
#define SPI_DATA_IPC_INNER_HEADER_SIZE	\
		(SPI_DATA_IPC_INNER_LENGTH_SIZE+ \
		SPI_DATA_IPC_INNER_CONTROL_SIZE)
#define SPI_DATA_SIZE(X)				(SPI_DATA_HEADER_SIZE+X)

#define SPI_DATA_LENGTH_OFFSET	SPI_DATA_MUX_SIZE
#define SPI_DATA_BOF_OFFSET		(SPI_DATA_LENGTH_OFFSET+ \
		SPI_DATA_LENGTH_SIZE)
#define SPI_DATA_DATA_OFFSET		(SPI_DATA_BOF_OFFSET+ \
		SPI_DATA_BOF_SIZE)
#define SPI_DATA_EOF_OFFSET(X)		(SPI_DATA_DATA_OFFSET+X)

#define SPI_DATA_PACKET_HEADER_SIZE	                 4
#define SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE		 4
#define SPI_DATA_PACKET_MAX_PACKET_BODY_SIZE \
		(SPI_MAX_PACKET_SIZE-SPI_DATA_PACKET_HEADER_SIZE- \
		SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE)

#define SPI_DATA_MIN_SIZE		(SPI_DATA_HEADER_SIZE*2)
#define SPI_DATA_MAX_SIZE_PER_PACKET (SPI_MAX_PACKET_SIZE- \
		SPI_DATA_PACKET_HEADER_SIZE-SPI_DATA_HEADER_SIZE- \
		SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE)

#define SPI_DATA_DIVIDE_BUFFER_SIZE SPI_MAX_PACKET_SIZE

struct spi_work_type {
	struct work_struct work;
	int signal_code;
};

enum spi_msg_t {
	SPI_WORK_SEND,
	SPI_WORK_RECEIVE
};

enum spi_state_t {
	SPI_STATE_START,		/* before init complete */
	SPI_STATE_INIT,		/* initialising */
	SPI_STATE_IDLE,		/* suspend. Waiting for event */
	/* state to start tx. Become from idle */
	SPI_STATE_TX_START,
	SPI_STATE_TX_CONFIRM,
	/* (in case of master) */
	/* wait srdy rising interrupt to check slave preparing */
	/* (in case of slave) */
	/* wait submrdy rising interrupt to check sync complete */
	SPI_STATE_TX_WAIT,
	SPI_STATE_TX_SENDING,	/* tx data sending  */
	SPI_STATE_TX_TERMINATE,
	SPI_STATE_TX_MORE,

	/* in case of slave, wait submrdy rising interrupt to */
	SPI_STATE_RX_WAIT,

	/* check sync complete then it starts to read buffer */
	SPI_STATE_RX_MORE,
	SPI_STATE_RX_TERMINATE,
	SPI_STATE_END			/* spi task is stopped */
};

enum spi_timer_state_t {
	SPI_STATE_TIME_START,
	SPI_STATE_TIME_OVER
};

enum spi_gpiolevel_t {
	SPI_GPIOLEVEL_LOW	= 0,
	SPI_GPIOLEVEL_HIGH
};

enum spi_data_type_t {
	SPI_DATA_MUX_IPC			= 0x01,
	SPI_DATA_MUX_RAW			= 0x02,
	SPI_DATA_MUX_RFS			= 0x03,
	SPI_DATA_MUX_CMD			= 0x04,
};

struct spi_data_packet_header {
	/* 12bit : packet size less than SPI_DEV_PACKET_SIZE */
	unsigned long current_data_size:31;
	/* 1bit : packet division flag */
	unsigned long more:1;
};

struct spi_link_device {
	struct link_device ld;

	/* Link to SPI control functions dependent on each platform */
	int max_ipc_dev;

	/* Wakelock for SPI device */
	struct wake_lock spi_wake_lock;
	/* Workqueue for modem bin transfers */
	struct workqueue_struct *ipc_spi_wq;

	/* SPI state */
	int spi_state;
	int spi_is_restart;

	/* SPI Timer state */
	int spi_timer_tx_state;
	int spi_timer_rx_state;

	/* SPI GPIO pins */
	unsigned int gpio_ipc_mrdy;
	unsigned int gpio_ipc_srdy;
	unsigned int gpio_ipc_sub_mrdy;
	unsigned int gpio_ipc_sub_srdy;

	unsigned int gpio_modem_bin_srdy;

	unsigned int gpio_ap_cp_int1;
	unsigned int gpio_ap_cp_int2;

	/* value for checking spi pin status */
	unsigned long mrdy_low_time_save;
	unsigned long mrdy_high_time_save;

	/* SPI Wait Timer */
	struct timer_list spi_tx_timer;
	struct timer_list spi_rx_timer;

	struct workqueue_struct *spi_wq;
	struct spi_work_type spi_work;

	/* For send modem bin */
	struct work_struct send_modem_w;

	struct io_device    *iod[MAX_DEV_FORMAT];
	struct sk_buff_head  skb_rxq[MAX_DEV_FORMAT];

	/* Multi-purpose miscellaneous buffer */
	u8 *buff;
	u8 *sync_buff;

	struct completion ril_init;
	struct semaphore srdy_sem;

	int send_modem_spi;
	int is_cp_reset;
	int boot_done;
	int ril_send_modem_img;
	unsigned long ril_send_cnt;

	void __iomem *p_virtual_buff;
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_spi_link_device(linkdev) \
			container_of(linkdev, struct spi_link_device, ld)

extern unsigned int lpcharge;
extern int get_console_suspended(void);
static void spi_work(struct work_struct *work);

/* Send SPRD main image through SPI */
#define SPRD_BLOCK_SIZE	32768

enum image_type {
	MODEM_MAIN,
	MODEM_DSP,
	MODEM_NV,
	MODEM_EFS,
	MODEM_RUN,
};

struct image_buf {
	unsigned int length;
	unsigned int offset;
	unsigned int address;
	unsigned char *buf;
};

struct sprd_image_buf {
	u8 *tx_b;
	u8 *rx_b;
	u8 *encoded_tx_b;
	u8 *decoded_rx_b;

	int tx_size;
	int rx_size;
	int encoded_tx_size;
	int decoded_rx_size;
};

/* CRC */
#define CRC_16_L_OK	0x0
#define HDLC_FLAG	0x7E
#define HDLC_ESCAPE	0x7D
#define HDLC_ESCAPE_MASK 0x20
#define CRC_CHECK_SIZE	0x02

#define M_32_SWAP(a) {					\
		u32 _tmp;				\
		_tmp = a;					\
		((u8 *)&a)[0] = ((u8 *)&_tmp)[3]; \
		((u8 *)&a)[1] = ((u8 *)&_tmp)[2]; \
		((u8 *)&a)[2] = ((u8 *)&_tmp)[1]; \
		((u8 *)&a)[3] = ((u8 *)&_tmp)[0]; \
		}

#define M_16_SWAP(a) {					\
		u16 _tmp;					\
		_tmp = (u16)a;			\
		((u8 *)&a)[0] = ((u8 *)&_tmp)[1];	\
		((u8 *)&a)[1] = ((u8 *)&_tmp)[0];	\
		}

#endif
