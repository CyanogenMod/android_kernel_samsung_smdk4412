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

#ifndef __MODEM_PRJ_H__
#define __MODEM_PRJ_H__

#include <linux/wait.h>
#include <linux/miscdevice.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/wakelock.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/types.h>

#define MAX_CPINFO_SIZE		512

#define MAX_LINK_DEVTYPE	3

#define MAX_FMT_DEVS	10
#define MAX_RAW_DEVS	32
#define MAX_RFS_DEVS	10
#define MAX_NUM_IO_DEV	(MAX_FMT_DEVS + MAX_RAW_DEVS + MAX_RFS_DEVS)

#define IOCTL_MODEM_ON			_IO('o', 0x19)
#define IOCTL_MODEM_OFF			_IO('o', 0x20)
#define IOCTL_MODEM_RESET		_IO('o', 0x21)
#define IOCTL_MODEM_BOOT_ON		_IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF		_IO('o', 0x23)
#define IOCTL_MODEM_BOOT_DONE		_IO('o', 0x24)

#define IOCTL_MODEM_PROTOCOL_SUSPEND	_IO('o', 0x25)
#define IOCTL_MODEM_PROTOCOL_RESUME	_IO('o', 0x26)

#define IOCTL_MODEM_STATUS		_IO('o', 0x27)
#define IOCTL_MODEM_DL_START		_IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE		_IO('o', 0x29)

#define IOCTL_MODEM_NET_SUSPEND		_IO('o', 0x30)
#define IOCTL_MODEM_NET_RESUME		_IO('o', 0x31)

#define IOCTL_MODEM_DUMP_START		_IO('o', 0x32)
#define IOCTL_MODEM_DUMP_UPDATE		_IO('o', 0x33)
#define IOCTL_MODEM_FORCE_CRASH_EXIT	_IO('o', 0x34)
#define IOCTL_MODEM_CP_UPLOAD		_IO('o', 0x35)
#define IOCTL_MODEM_DUMP_RESET		_IO('o', 0x36)

#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
#define IOCTL_MODEM_SWITCH_MODEM	_IO('o', 0x37)
#endif

#define IOCTL_DPRAM_SEND_BOOT		_IO('o', 0x40)
#define IOCTL_DPRAM_INIT_STATUS		_IO('o', 0x43)

/* ioctl command definitions. */
#define IOCTL_DPRAM_PHONE_POWON		_IO('o', 0xd0)
#define IOCTL_DPRAM_PHONEIMG_LOAD	_IO('o', 0xd1)
#define IOCTL_DPRAM_NVDATA_LOAD		_IO('o', 0xd2)
#define IOCTL_DPRAM_PHONE_BOOTSTART	_IO('o', 0xd3)

#define IOCTL_DPRAM_PHONE_UPLOAD_STEP1	_IO('o', 0xde)
#define IOCTL_DPRAM_PHONE_UPLOAD_STEP2	_IO('o', 0xdf)

/* ioctl command for IPC Logger */
#define IOCTL_MIF_LOG_DUMP		_IO('o', 0x51)
#define IOCTL_MIF_DPRAM_DUMP		_IO('o', 0x52)

/* modem status */
#define MODEM_OFF		0
#define MODEM_CRASHED		1
#define MODEM_RAMDUMP		2
#define MODEM_POWER_ON		3
#define MODEM_BOOTING_NORMAL	4
#define MODEM_BOOTING_RAMDUMP	5
#define MODEM_DUMPING		6
#define MODEM_RUNNING		7

#define HDLC_HEADER_MAX_SIZE	6 /* fmt 3, raw 6, rfs 6 */

#define PSD_DATA_CHID_BEGIN	0x2A
#define PSD_DATA_CHID_END	0x38

#define PS_DATA_CH_0	10
#define PS_DATA_CH_LAST	24

#define IP6VERSION		6

#define SOURCE_MAC_ADDR		{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}

/* loopback: CP -> AP -> CP */
#define CP2AP_LOOPBACK_CHANNEL	30

/* ip loopback */
#define RMNET0_CH_ID		10
#define DATA_LOOPBACK_CHANNEL	31

/* Debugging features */
#define MAX_MIF_LOG_PATH_LEN	128
#define MAX_MIF_LOG_FILE_SIZE	0x800000	/* 8 MB */

#define MAX_MIF_EVT_BUFF_SIZE	256
#define MAX_MIF_TIME_LEN	32
#define MAX_MIF_NAME_LEN	16
#define MAX_MIF_STR_LEN		127
#define MAX_MIF_LOG_LEN		128

enum mif_event_id {
	MIF_IRQ_EVT = 0,
	MIF_LNK_RX_EVT,
	MIF_MUX_RX_EVT,
	MIF_IOD_RX_EVT,
	MIF_IOD_TX_EVT,
	MIF_MUX_TX_EVT,
	MIF_LNK_TX_EVT,
	MAX_MIF_EVT
};

struct dpram_queue_status {
	unsigned in;
	unsigned out;
};

struct dpram_queue_status_pair {
	struct dpram_queue_status txq;
	struct dpram_queue_status rxq;
};

struct dpram_irq_buff {
	unsigned magic;
	unsigned access;
	struct dpram_queue_status_pair qsp[MAX_IPC_DEV];
	unsigned int2ap;
	unsigned int2cp;
};

/* Not use */
struct mif_event_buff {
	char time[MAX_MIF_TIME_LEN];

	struct timeval tv;
	enum mif_event_id evt;

	char mc[MAX_MIF_NAME_LEN];

	char iod[MAX_MIF_NAME_LEN];

	char ld[MAX_MIF_NAME_LEN];
	enum modem_link link_type;

	unsigned rcvd;
	unsigned len;
	union {
		u8 data[MAX_MIF_LOG_LEN];
		struct dpram_irq_buff dpram_irqb;
	};
};

#define MIF_LOG_DIR	"/sdcard"
#define MIF_LOG_LV_FILE	"/data/.mif_log_level"

/* Does modem ctl structure will use state ? or status defined below ?*/
enum modem_state {
	STATE_OFFLINE,
	STATE_CRASH_RESET, /* silent reset */
	STATE_CRASH_EXIT, /* cp ramdump */
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING, /* <= rebuilding start */
	STATE_LOADER_DONE,
	STATE_SIM_ATTACH,
	STATE_SIM_DETACH,
#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
	STATE_MODEM_SWITCH,
#endif
};

enum com_state {
	COM_NONE,
	COM_ONLINE,
	COM_HANDSHAKE,
	COM_BOOT,
	COM_CRASH,
};

enum link_mode {
	LINK_MODE_OFFLINE = 0,
	LINK_MODE_BOOT,
	LINK_MODE_IPC,
	LINK_MODE_DLOAD,
	LINK_MODE_ULOAD,
};

struct sim_state {
	bool online;	/* SIM is online? */
	bool changed;	/* online is changed? */
};

#define HDLC_START		0x7F
#define HDLC_END		0x7E
#define SIZE_OF_HDLC_START	1
#define SIZE_OF_HDLC_END	1
#define MAX_LINK_PADDING_SIZE	3

struct header_data {
	char hdr[HDLC_HEADER_MAX_SIZE];
	unsigned len;
	unsigned frag_len;
	char start; /*hdlc start header 0x7F*/
};

struct fmt_hdr {
	u16 len;
	u8 control;
} __packed;

struct raw_hdr {
	u32 len;
	u8 channel;
	u8 control;
} __packed;

struct rfs_hdr {
	u32 len;
	u8 cmd;
	u8 id;
} __packed;

struct sipc_fmt_hdr {
	u16 len;
	u8  msg_seq;
	u8  ack_seq;
	u8  main_cmd;
	u8  sub_cmd;
	u8  cmd_type;
} __packed;

#define SIPC5_START_MASK	0b11111000
#define SIPC5_CONFIG_MASK	0b00000111
#define SIPC5_EXT_FIELD_MASK	0b00000011

#define SIPC5_PADDING_EXIST	0b00000100
#define SIPC5_EXT_FIELD_EXIST	0b00000010
#define SIPC5_CTL_FIELD_EXIST	0b00000001

#define SIPC5_MAX_HEADER_SIZE	6
#define SIPC5_HEADER_SIZE_WITH_EXT_LEN	6
#define SIPC5_HEADER_SIZE_WITH_CTL_FLD	5
#define SIPC5_MIN_HEADER_SIZE	4
#define SIPC5_CONFIG_SIZE	1
#define SIPC5_CH_ID_SIZE	1

#define SIPC5_CONFIG_OFFSET	0
#define SIPC5_CH_ID_OFFSET	1
#define SIPC5_LEN_OFFSET	2
#define SIPC5_CTL_OFFSET	4

#define SIPC5_CH_ID_RAW_0	0
#define SIPC5_CH_ID_PDP_0	10
#define SIPC5_CH_ID_PDP_LAST	24
#define SIPC5_CH_ID_FMT_0	235
#define SIPC5_CH_ID_RFS_0	245
#define SIPC5_CH_ID_MAX		255

/* If iod->id is 0, do not need to store to `iodevs_tree_fmt' in SIPC4 */
#define sipc4_is_not_reserved_channel(ch) ((ch) != 0)

/* Channel 0, 5, 6, 27, 255 are reserved in SIPC5.
 * see SIPC5 spec: 2.2.2 Channel Identification (Ch ID) Field.
 * They do not need to store in `iodevs_tree_fmt'
 */
#define sipc5_is_not_reserved_channel(ch) \
	((ch) != 0 && (ch) != 5 && (ch) != 6 && (ch) != 27 && (ch) != 255)

struct sipc5_link_hdr {
	u8 cfg;
	u8 ch;
	u16 len;
	union {
		u8 ctl;
		u16 ext_len;
	};
} __packed;

struct sipc5_frame_data {
	/* Config octet */
	u8 config;

	/* Channel ID */
	u8 ch_id;

	/* Control for multiple FMT frame */
	u8 control;

	/* Frame configuration set by header analysis */
	bool padding;
	bool ctl_fld;
	bool ext_len;

	/* Frame length calculated from the length fields */
	unsigned len;

	/* The length of link layer header */
	unsigned hdr_len;

	/* The length of received header */
	unsigned hdr_rcvd;

	/* The length of data payload */
	unsigned data_len;

	/* The length of received data */
	unsigned data_rcvd;

	/* Header buffer */
	u8 hdr[SIPC5_MAX_HEADER_SIZE];
};

struct vnet {
	struct io_device *iod;
};

/* for fragmented data from link devices */
struct fragmented_data {
	struct sk_buff *skb_recv;
	struct header_data h_data;
	struct sipc5_frame_data f_data;
	/* page alloc fail retry*/
	unsigned realloc_offset;
};
#define fragdata(iod, ld) (&(iod)->fragments[(ld)->link_type])

/** struct skbuff_priv - private data of struct sk_buff
 * this is matched to char cb[48] of struct sk_buff
 */
struct skbuff_private {
	struct io_device *iod;
	struct link_device *ld;
	struct io_device *real_iod; /* for rx multipdp */
	u8 ch_id;
	u8 control;
} __packed;

static inline struct skbuff_private *skbpriv(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(struct skbuff_private) > sizeof(skb->cb));
	return (struct skbuff_private *)&skb->cb;
}

struct io_device {
	/* rb_tree node for an io device */
	struct rb_node node_chan;
	struct rb_node node_fmt;

	/* Name of the IO device */
	char *name;

	atomic_t opened;

	/* Wait queue for the IO device */
	wait_queue_head_t wq;

	/* Misc and net device structures for the IO device */
	struct miscdevice  miscdev;
	struct net_device *ndev;

	/* ID and Format for channel on the link */
	unsigned id;
	enum modem_link link_types;
	enum dev_format format;
	enum modem_io io_typ;
	enum modem_network net_typ;

	bool use_handover;	/* handover 2+ link devices */

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Rx queue of sk_buff */
	struct sk_buff_head sk_rx_q;

	/*
	** work for each io device, when delayed work needed
	** use this for private io device rx action
	*/
	struct delayed_work rx_work;

	struct fragmented_data fragments[LINKDEV_MAX];

	/* for multi-frame */
	struct sk_buff *skb[128];

	/* called from linkdevice when a packet arrives for this iodevice */
	int (*recv)(struct io_device *iod, struct link_device *ld,
					const char *data, unsigned int len);
	int (*recv_skb)(struct io_device *iod, struct link_device *ld,
					struct sk_buff *skb);

	/* inform the IO device that the modem is now online or offline or
	 * crashing or whatever...
	 */
	void (*modem_state_changed)(struct io_device *iod, enum modem_state);

	/* inform the IO device that the SIM is not inserting or removing */
	void (*sim_state_changed)(struct io_device *iod, bool sim_online);

	struct modem_ctl *mc;
	struct modem_shared *msd;

	struct wake_lock wakelock;
	long waketime;

	/* DO NOT use __current_link directly
	 * you MUST use skbpriv(skb)->ld in mc, link, etc..
	 */
	struct link_device *__current_link;
};
#define to_io_device(misc) container_of(misc, struct io_device, miscdev)

/* get_current_link, set_current_link don't need to use locks.
 * In ARM, set_current_link and get_current_link are compiled to
 * each one instruction (str, ldr) as atomic_set, atomic_read.
 * And, the order of set_current_link and get_current_link is not important.
 */
#define get_current_link(iod) ((iod)->__current_link)
#define set_current_link(iod, ld) ((iod)->__current_link = (ld))

struct link_device {
	struct list_head  list;
	char *name;

	enum modem_link link_type;
	unsigned aligned;

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Modem data */
	struct modem_data *mdm_data;

	/* Modem control */
	struct modem_ctl *mc;

	/* Modem shared data */
	struct modem_shared *msd;

	/* Operation mode of the link device */
	enum link_mode mode;

	struct io_device *fmt_iods[4];

	/* TX queue of socket buffers */
	struct sk_buff_head sk_fmt_tx_q;
	struct sk_buff_head sk_raw_tx_q;
	struct sk_buff_head sk_rfs_tx_q;

	struct sk_buff_head *skb_txq[MAX_IPC_DEV];

	bool raw_tx_suspended; /* for misc dev */
	struct completion raw_tx_resumed_by_cp;

	struct workqueue_struct *tx_wq;
	struct work_struct tx_work;
	struct delayed_work tx_delayed_work;
	struct delayed_work tx_dwork;

	struct workqueue_struct *rx_wq;
	struct work_struct rx_work;
	struct delayed_work rx_delayed_work;

	enum com_state com_state;

	/* init communication - setting link driver */
	int (*init_comm)(struct link_device *ld, struct io_device *iod);

	/* terminate communication */
	void (*terminate_comm)(struct link_device *ld, struct io_device *iod);

	/* called by an io_device when it has a packet to send over link
	 * - the io device is passed so the link device can look at id and
	 *   format fields to determine how to route/format the packet
	 */
	int (*send)(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb);

	int (*udl_start)(struct link_device *ld, struct io_device *iod);

	int (*force_dump)(struct link_device *ld, struct io_device *iod);

	int (*dump_start)(struct link_device *ld, struct io_device *iod);

	int (*modem_update)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);

	int (*dump_update)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);

	int (*ioctl)(struct link_device *ld, struct io_device *iod,
			unsigned cmd, unsigned long _arg);
};

/** rx_alloc_skb - allocate an skbuff and set skb's iod, ld
 * @length:	length to allocate
 * @iod:	struct io_device *
 * @ld:		struct link_device *
 *
 * %NULL is returned if there is no free memory.
 */
static inline struct sk_buff *rx_alloc_skb(unsigned int length,
		struct io_device *iod, struct link_device *ld)
{
	struct sk_buff *skb;

	if (iod->format == IPC_MULTI_RAW || iod->format == IPC_RAW)
		skb = dev_alloc_skb(length);
	else
		skb = alloc_skb(length, GFP_ATOMIC);

	if (likely(skb)) {
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;
	}
	return skb;
}

struct modemctl_ops {
	int (*modem_on) (struct modem_ctl *);
	int (*modem_off) (struct modem_ctl *);
	int (*modem_reset) (struct modem_ctl *);
	int (*modem_boot_on) (struct modem_ctl *);
	int (*modem_boot_off) (struct modem_ctl *);
	int (*modem_boot_done) (struct modem_ctl *);
	int (*modem_force_crash_exit) (struct modem_ctl *);
	int (*modem_dump_reset) (struct modem_ctl *);
};

/* for IPC Logger */
struct mif_storage {
	char *addr;
	unsigned int cnt;
};

/* modem_shared - shared data for all io/link devices and a modem ctl
 * msd : mc : iod : ld = 1 : 1 : M : N
 */
struct modem_shared {
	/* list of link devices */
	struct list_head link_dev_list;

	/* rb_tree root of io devices. */
	struct rb_root iodevs_tree_chan; /* group by channel */
	struct rb_root iodevs_tree_fmt; /* group by dev_format */

	/* for IPC Logger */
	struct mif_storage storage;
	spinlock_t lock;

	/* loopbacked IP address
	 * default is 0.0.0.0 (disabled)
	 * after you setted this, you can use IP packet loopback using this IP.
	 * exam: echo 1.2.3.4 > /sys/devices/virtual/misc/umts_multipdp/loopback
	 */
	__be32 loopback_ipaddr;
};

struct modem_ctl {
	struct device *dev;
	char *name;
	struct modem_data *mdm_data;

	struct modem_shared *msd;

	enum modem_state phone_state;
	struct sim_state sim_state;

	unsigned gpio_cp_on;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_ap_dump_int;
	unsigned gpio_flm_uart_sel;
#if defined(CONFIG_MACH_M0_CTC)
	unsigned gpio_flm_uart_sel_rev06;
#endif
	unsigned gpio_cp_warm_reset;
	unsigned gpio_cp_off;
	unsigned gpio_sim_detect;
	unsigned gpio_dynamic_switching;

	int irq_phone_active;
	int irq_sim_detect;

#ifdef CONFIG_LTE_MODEM_CMC221
	const struct attribute_group *group;
	unsigned gpio_slave_wakeup;
	unsigned gpio_host_wakeup;
	unsigned gpio_host_active;
	int      irq_host_wakeup;

	struct delayed_work dwork;
#endif /*CONFIG_LTE_MODEM_CMC221*/

	struct work_struct work;

#if defined(CONFIG_MACH_U1_KOR_LGT)
	unsigned gpio_cp_reset_msm;
	unsigned gpio_boot_sw_sel;
	void (*vbus_on)(void);
	void (*vbus_off)(void);
	bool usb_boot;
#endif

#ifdef CONFIG_TDSCDMA_MODEM_SPRD8803
	unsigned gpio_ap_cp_int1;
	unsigned gpio_ap_cp_int2;
#endif

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	unsigned gpio_sim_io_sel;
	unsigned gpio_cp_ctrl1;
	unsigned gpio_cp_ctrl2;
#endif

#ifdef CONFIG_LINK_DEVICE_PLD
	unsigned gpio_fpga_cs_n;
#endif

	struct modemctl_ops ops;
	struct io_device *iod;
	struct io_device *bootd;

	/* Wakelock for modem_ctl */
	struct wake_lock mc_wake_lock;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);

	bool need_switch_to_usb;
	bool sim_polarity;
};

int sipc4_init_io_device(struct io_device *iod);
int sipc5_init_io_device(struct io_device *iod);

/**
 * sipc5_start_valid
 * @cfg: configuration field of an SIPC5 link frame
 *
 * Returns TRUE if the start (configuration field) of an SIPC5 link frame
 * is valid or returns FALSE if it is not valid.
 *
 */
static inline int sipc5_start_valid(u8 cfg)
{
	return (cfg & SIPC5_START_MASK) == SIPC5_START_MASK;
}

/**
 * sipc5_get_hdr_len
 * @cfg: configuration field of an SIPC5 link frame
 *
 * Returns the length of SIPC5 link layer header in an SIPC5 link frame
 *
 */
static inline unsigned sipc5_get_hdr_len(u8 cfg)
{
	if (cfg & SIPC5_EXT_FIELD_EXIST) {
		if (cfg & SIPC5_CTL_FIELD_EXIST)
			return SIPC5_HEADER_SIZE_WITH_CTL_FLD;
		else
			return SIPC5_HEADER_SIZE_WITH_EXT_LEN;
	} else {
		return SIPC5_MIN_HEADER_SIZE;
	}
}

/**
 * sipc5_get_ch_id
 * @frm: pointer to an SIPC5 frame
 *
 * Returns the channel ID in an SIPC5 link frame
 *
 */
static inline u8 sipc5_get_ch_id(u8 *frm)
{
	return *(frm + SIPC5_CH_ID_OFFSET);
}

/**
 * sipc5_get_frame_sz16
 * @frm: pointer to an SIPC5 link frame
 *
 * Returns the length of an SIPC5 link frame without the extended length field
 *
 */
static inline unsigned sipc5_get_frame_sz16(u8 *frm)
{
	return *((u16 *)(frm + SIPC5_LEN_OFFSET));
}

/**
 * sipc5_get_frame_sz32
 * @frm: pointer to an SIPC5 frame
 *
 * Returns the length of an SIPC5 link frame with the extended length field
 *
 */
static inline unsigned sipc5_get_frame_sz32(u8 *frm)
{
	return *((u32 *)(frm + SIPC5_LEN_OFFSET));
}

/**
 * sipc5_calc_padding_size
 * @len: length of an SIPC5 link frame
 *
 * Returns the padding size for an SIPC5 link frame
 *
 */
static inline unsigned sipc5_calc_padding_size(unsigned len)
{
	unsigned residue = len & 0x3;
	return residue ? (4 - residue) : 0;
}

extern void set_sromc_access(bool access);

#if defined(CONFIG_TDSCDMA_MODEM_SPRD8803) && defined(CONFIG_LINK_DEVICE_SPI)
extern int spi_sema_init(void);
extern int sprd_boot_done;
struct ipc_spi {
	struct class *class;
	struct device *dev;
	struct cdev cdev;
	dev_t devid;

	wait_queue_head_t waitq;
	struct fasync_struct *async_queue;
	u32 mailbox;

	unsigned long base;
	unsigned long size;
	void __iomem *mmio;

	int irq;

	struct completion comp;
	atomic_t ref_sem;
	unsigned long flags;

	const struct attribute_group *group;
};
#endif

#endif
