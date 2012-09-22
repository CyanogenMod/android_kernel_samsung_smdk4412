/*
 * Copyright (C) 2011 Samsung Electronics.
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
#ifndef _WIMAX_SDIO_H
#define _WIMAX_SDIO_H

#include <linux/miscdevice.h>
#include <linux/mmc/sdio_func.h>
#include <linux/netdevice.h>
#include <linux/completion.h>
#include <linux/firmware.h>
#include <asm/byteorder.h>
#define WIMAX_CON0_POLL

/* Macro definition for defining IOCTL */
#define CTL_CODE(DeviceType, Function, Method, Access)	\
(	\
	((DeviceType) << 16) | ((Access) << 14) |	\
	((Function) << 2) | (Method) \
)
/* Define the method codes for how buffers are passed for I/O and FS controls */
#define METHOD_BUFFERED			0

/*
*	Define the access check value for any access

*	The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
*	ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
*	constants *MUST* always be in sync.
*/
#define FILE_ANY_ACCESS			0

#define CONTROL_ETH_TYPE_WCM		0x0015
#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89
#endif
#define	CONTROL_IOCTL_WRITE_REQUEST		\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_POWER_CTL	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_MODE_CHANGE	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x839, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CONTROL_IOCTL_WIMAX_WRITE_REV \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83B, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CONTROL_IOCTL_WIMAX_CHECK_CERT \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83C, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CONTROL_IOCTL_WIMAX_CHECK_CAL \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ETHERNET_ADDRESS_LENGTH     6

/* eth types for control message */
enum {
	ETHERTYPE_HIM	= 0x1500,
	ETHERTYPE_MC	= 0x1501,
	ETHERTYPE_DM	= 0x1502,
	ETHERTYPE_CT	= 0x1503,
	ETHERTYPE_DL	= 0x1504,
	ETHERTYPE_VSP	= 0x1510,
	ETHERTYPE_AUTH	= 0x1521
};

#define WIMAX_IMAGE_PATH	"wimaxfw.bin"
#define WIMAX_LOADER_PATH	"wimaxloader.bin"

/*Time(ms) taken for CMC bootloader to be ready for firmware download*/
#define CMC_BOOTLOAD_TIME 250

 /*Time(ms) to wait for SDIO probe after SDIO force detect call*/
#define CMC_PROBE_TIMEOUT 1000

 /*Time(ms) to wait the firmware to complete downloading*/
#define CMC_FIRMWARE_DOWNLOAD_TIMEOUT 2000

/*maximum time(ms) spent for MAC request */
#define CMC_MAC_TIMEOUT 9000

#define MODEM_RESP_RETRY 10
#define ADAPTER_TIMEOUT				(HZ * 10)

#define CMC732_RAM_START		0xC0000000
#define CMC732_WIMAX_ADDRESS		CMC732_RAM_START

#define CMD_MSG_TOTAL_LENGTH		12
#define IMAGE_INFO_MSG_TOTAL_LENGTH	28
#define CMD_MSG_LENGTH			0
#define IMAGE_INFO_MSG_LENGTH		16
#define MAX_IMAGE_DATA_LENGTH		3564
#define MAX_IMAGE_DATA_MSG_LENGTH	4096


/*
* SDIO general defines
* size of a bank in cmc732's rx and tx buffers
*/
#define CMC732_SDIO_BANK_SIZE			4096
#define CMC732_PACKET_LENGTH_SIZE	4
#define CMC732_MAX_PACKET_SIZE (CMC732_SDIO_BANK_SIZE \
	- CMC732_PACKET_LENGTH_SIZE)
#define CMC_BLOCK_SIZE				512

#define WIMAX_MTU_SIZE				1400
#define WIMAX_MAX_FRAMESIZE		1500
#define WIMAX_HEADER_SIZE			14
#define WIMAX_MAX_TOTAL_SIZE	(WIMAX_MAX_FRAMESIZE + WIMAX_HEADER_SIZE)
/* maximum allocated data size,  mtu 1400 so 3 blocks max 1536 */
#define BUFFER_DATA_SIZE			1600

/*maximum permitted SDIO error before wimax restart*/
#define MAX_SDIO_ERROR				10
/* SDIO function addresses  */
#define SDIO_TX_BANK_ADDR			0x1000
#define SDIO_RX_BANK_ADDR			0x10000
#define SDIO_INT_STATUS_REG		0xC0
#define SDIO_INT_STATUS_CLR_REG	0xC4

#define SDIO_C2H_WP_REG			0xE4
#define SDIO_C2H_RP_REG			0xE8
#define SDIO_H2C_WP_REG			0xEC
#define SDIO_H2C_RP_REG			0xF0

#define SDIO_INT_DATA_READY			0x01
#define SDIO_INT_ERROR				0x02


#define MAC_RETRY_COUNT			9
#define MAC_RETRY_INTERVAL			200

#define WAKEUP_MAX_TRY			60
#define WAKEUP_ASSERT_T			100

#define CMC_BLOCK_SIZE				512
#define CMC_MAX_BYTE_SIZE			(CMC_BLOCK_SIZE - 1)
/* used for host boot (firmware download) */
enum {
	MSG_DRIVER_OK_REQ	= 0x5010,
	MSG_DRIVER_OK_RESP	= 0x6010,
	MSG_IMAGE_INFO_REQ	= 0x3021,
	MSG_IMAGE_INFO_RESP	= 0x4021,
	MSG_IMAGE_DATA_REQ	= 0x3022,
	MSG_IMAGE_DATA_RESP	= 0x4022,
	MSG_RUN_REQ		= 0x5014,
	MSG_RUN_RESP		= 0x6014
};

/* private packet opcodes */
enum {
	HWCODEMACREQUEST = 0x01,
	HWCODEMACRESPONSE,
	HWCODELINKINDICATION,
	HWCODERXREADYINDICATION,
	HWCODEHALTEDINDICATION,
	HWCODEIDLENTFY,
	HWCODEWAKEUPNTFY
};

#define HW_PROT_VALUE_LINK_DOWN			0x00
#define HW_PROT_VALUE_LINK_UP			0xff

/* process element managed by control type */
struct process_descriptor {
	struct list_head	list;
	struct list_head	buffer_list;
	wait_queue_head_t	read_wait;
	u32		id;
	u16		type;
};

struct buffer_descriptor {
	struct list_head	list;		/* list node */
	u8	*buffer;			/* allocated buffer: */
	s32			length;		/* current data length */
};

struct image_data_payload {
	u32	offset;
	u32	size;
	u8	data[MAX_IMAGE_DATA_LENGTH];
};

#pragma pack(1)

/* eth header structure */
struct eth_header {
	u8		dest[ETHERNET_ADDRESS_LENGTH];
	u8		src[ETHERNET_ADDRESS_LENGTH];
	u16		type;
};

struct hw_packet_header {
	char		id0;	/* packet ID */
	char		id1;
	u16	length;	/* packet length */
};

struct hw_private_packet {
	char		id0;	/* packet ID */
	char		id1;
	u8	code;	/* command code */
	u8	value;	/* command value */
};


struct wimax_msg_header {
	u16	type;
	u16	id;
	u32	length;
};

#pragma pack()

/* network adapter structure */
struct net_adapter {
	struct completion		probe;
	struct completion		firmware_download;
	struct completion		mac;
	struct completion		remove;
	wait_queue_head_t       modem_resp_event;
	wait_queue_head_t       con0_poll;
	struct workqueue_struct		*wimax_workqueue;
	struct work_struct		rx_work;
	struct work_struct		tx_work;
	struct sdio_func		*func;
	struct net_device		*net;
	struct net_device_stats		netstats;
	struct miscdevice		 uwibro_dev;
	const struct firmware *fw;
	struct list_head	q_send;	/* send pending queue */
	spinlock_t		send_lock;
	struct list_head	control_process_list;
	struct mutex		control_lock;
	struct wimax732_platform_data	*pdata;
#ifdef WIMAX_CON0_POLL
	struct task_struct		*wtm_task;
#endif
	struct sk_buff  *rx_skb;
	u8	*receive_buffer;
	u32	buff_len;
	u32					image_offset;
	u32			msg_enable;
	s32			wake_irq;
	u32			sdio_error_count;
	u8		eth_header[ETHERNET_ADDRESS_LENGTH * 2];
	bool		modem_resp;
};

#endif	/* _WIMAX_SDIO_H */
