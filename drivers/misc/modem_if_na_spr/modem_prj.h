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
#include <linux/wakelock.h>


#define MAX_LINK_DEVTYPE 3
#define MAX_RAW_DEVS 32
#define MAX_NUM_IO_DEV	(MAX_RAW_DEVS + 4)

#define IOCTL_MODEM_ON	_IO('o', 0x19)
#define IOCTL_MODEM_OFF	_IO('o', 0x20)
#define IOCTL_MODEM_RESET	_IO('o', 0x21)
#define IOCTL_MODEM_BOOT_ON	_IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF	_IO('o', 0x23)
#define IOCTL_MODEM_START	_IO('o', 0x24)

#define IOCTL_MODEM_SEND	_IO('o', 0x25)
#define IOCTL_MODEM_RECV	_IO('o', 0x26)

#define IOCTL_MODEM_STATUS	_IO('o', 0x27)
#define IOCTL_MODEM_GOTA_START	_IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE	_IO('o', 0x29)

#define IOCTL_MODEM_NET_SUSPEND	_IO('o', 0x30)
#define IOCTL_MODEM_NET_RESUME	_IO('o', 0x31)
#define IOCTL_MODEM_DUMP_START	_IO('o', 0x32)
#define IOCTL_MODEM_DUMP_UPDATE	_IO('o', 0x33)
#define IOCTL_MODEM_FORCE_CRASH_EXIT _IO('o', 0x34)
#define IOCTL_MODEM_DUMP_RESET _IO('o', 0x35)

#define IOCTL_MODEM_RAMDUMP_START  _IO('o', 0xCE)
#define IOCTL_MODEM_RAMDUMP_STOP  _IO('o', 0xCF)

#define IPC_HEADER_MAX_SIZE	6 /* fmt 3, raw 6, rfs 6 */

#define PSD_DATA_CHID_BEGIN	0x2A
#define PSD_DATA_CHID_END	0x38

#define IP6VERSION	6

#define SOURCE_MAC_ADDR	{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}

/* Does modem ctl structure will use state ? or status defined below ?*/
/* Be careful!! below sequence shouldn't be changed*/
enum modem_state {
	STATE_OFFLINE,
	__UNUSED__,
	STATE_CRASH_EXIT,
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING,
	STATE_LOADER_DONE,
};

enum {
	COM_NONE,
	COM_ONLINE,
	COM_HANDSHAKE,
	COM_BOOT,
	COM_CRASH,
	COM_BOOT_EBL,
};

struct header_data {
	char hdr[IPC_HEADER_MAX_SIZE];
	unsigned len;
	unsigned flag_len;
	char start; /*hdlc start header 0x7F*/
};

/* buffer type for modem image */
struct dpram_firmware {
	char *firmware;
	int size;
	int is_delta;
};


struct vnet {
	struct io_device *iod;
};

struct io_device {
	struct list_head list;
	char *name;

	wait_queue_head_t wq;

	struct miscdevice miscdev;
	struct net_device *ndev;

	/* ID and Format for channel on the link */
	unsigned id;
	enum dev_format format;
	enum modem_io io_typ;
	enum modem_network net_typ;

	u32 ramdump_size;
	u32 ramdump_addr;

	struct sk_buff_head sk_rx_q;

	/* work for each io device, when delayed work needed
	* use this for private io device rx action
	*/
	struct delayed_work rx_work;

	/* for fragmentation data from link device */
	struct sk_buff *skb_recv;
	struct header_data h_data;

	/* called from linkdevice when a packet arrives for this iodevice */
	int (*recv)(struct io_device *iod, const char *data, unsigned int len);

	/* inform the IO device that the modem is now online or offline or
	 * crashing or whatever...
	 */
	void (*modem_state_changed)(struct io_device *iod, enum modem_state);

	struct link_device *link;
	struct modem_ctl *mc;

	struct wake_lock wakelock;
	long waketime;

	void *private_data;
};
#define to_io_device(misc) container_of(misc, struct io_device, miscdev)

struct io_raw_devices {
	struct io_device *raw_devices[MAX_RAW_DEVS];
	int num_of_raw_devs;
};

struct link_device {
	char *name;

	struct sk_buff_head sk_fmt_tx_q;
	struct sk_buff_head sk_raw_tx_q;

	struct workqueue_struct *tx_wq;
	struct workqueue_struct *tx_raw_wq;
	struct work_struct tx_work;
	struct delayed_work tx_delayed_work;

	unsigned com_state;

	/* called during init to associate an io device with this link */
	int (*attach)(struct link_device *ld, struct io_device *iod);

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

	int (*gota_start)(struct link_device *ld, struct io_device *iod);
	int (*dump_start)(struct link_device *ld, struct io_device *iod);

	int (*modem_update)(
		struct link_device *ld,
		struct io_device *iod,
		unsigned long arg);
	int (*dump_update)(
		struct link_device *ld,
		struct io_device *iod,
		unsigned long arg);

	int (*start_ramdump)(struct link_device *ld, struct io_device *iod);
	int (*read_ramdump)(struct link_device *ld, struct io_device *iod);
	int (*stop_ramdump)(struct link_device *ld, struct io_device *iod);
};

struct modemctl_ops {
	int (*modem_on) (struct modem_ctl *);
	int (*modem_off) (struct modem_ctl *);
	int (*modem_reset) (struct modem_ctl *);
	int (*modem_boot_on) (struct modem_ctl *);
	int (*modem_boot_off) (struct modem_ctl *);
	int (*modem_force_crash_exit) (struct modem_ctl *);
	int (*modem_dump_reset) (struct modem_ctl *);
};

struct modem_ctl {
	struct device *dev;
	char *name;

	int phone_state;
	bool ramdump_active;
	bool power_off_in_progress;

	unsigned gpio_cp_on;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_flm_uart_sel;
	unsigned gpio_cp_warm_reset;
	unsigned gpio_cp_off;
	unsigned gpio_mbx_intr;

	int irq_phone_active;

	struct work_struct work;

#if defined(CONFIG_LTE_MODEM_CMC221) || defined(CONFIG_LTE_MODEM_CMC220)
	const struct attribute_group *group;
	unsigned gpio_slave_wakeup;
	unsigned gpio_host_wakeup;
	unsigned gpio_host_active;
	int irq_host_wakeup;
	struct delayed_work dwork;
	struct work_struct resume_work;
	int wakeup_flag; /*flag for CP boot GPIO sync flag*/
	int cpcrash_flag;
	int crash_cnt;
	struct completion *l2_done;
	int irq[3];
#endif /*CONFIG_LTE_MODEM_CMC221*/

	void (*clear_intr)(void);
	struct modemctl_ops ops;
	struct io_device *iod;
};

int init_io_device(struct io_device *iod);

#endif
