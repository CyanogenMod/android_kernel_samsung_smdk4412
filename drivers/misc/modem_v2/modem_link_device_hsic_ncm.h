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

#ifndef __MODEM_LINK_DEVICE_USB_H__
#define __MODEM_LINK_DEVICE_USB_H__

/* FOR XMM6360 */
enum {
	IF_USB_BOOT_EP = 0,
	IF_USB_FMT_EP = 0,
	IF_USB_RAW_EP,
	IF_USB_RFS_EP,
	IF_USB_CMD_EP,
	_IF_USB_ACMNUM_MAX,
};

#define PS_DATA_CH_01	0xa
#define RX_POOL_SIZE 5
#define MULTI_URB 4

#define IOCTL_LINK_CONTROL_ENABLE	_IO('o', 0x30)
#define IOCTL_LINK_CONTROL_ACTIVE	_IO('o', 0x31)
#define IOCTL_LINK_GET_HOSTWAKE		_IO('o', 0x32)
#define IOCTL_LINK_CONNECTED		_IO('o', 0x33)
#define IOCTL_LINK_SET_BIAS_CLEAR	_IO('o', 0x34)
#define IOCTL_LINK_GET_PHONEACTIVE	_IO('o', 0x35)
#define IOCTL_LINK_DISABLE_RECOVERY	_IO('o', 0x36)

enum {
	BOOT_DOWN = 0,
	IPC_CHANNEL
};

enum ch_state {
	STATE_SUSPENDED,
	STATE_RESUMED,
};

#define HOSTWAKE_TRIGLEVEL	0

struct link_pm_info {
	struct usb_link_device *usb_ld;
};

struct if_usb_devdata;
struct usb_id_info {
	int intf_id;
	int urb_cnt;
	struct usb_link_device *usb_ld;

	char *description;
	int (*bind)(struct if_usb_devdata *, struct usb_interface *,
			struct usb_link_device *);
	void (*unbind)(struct if_usb_devdata *, struct usb_interface *);
	int (*rx_fixup)(struct if_usb_devdata *, struct sk_buff *skb);
	struct sk_buff *(*tx_fixup)(struct if_usb_devdata *dev,
			struct sk_buff *skb, gfp_t flags);
	void (*intr_complete)(struct urb *urb);
};

struct link_pm_data {
	struct miscdevice miscdev;
	struct usb_link_device *usb_ld;
	unsigned irq_link_hostwake;
	int (*link_ldo_enable)(bool);
	unsigned gpio_link_enable;
	unsigned gpio_link_active;
	unsigned gpio_link_hostwake;
	unsigned gpio_link_slavewake;
	int (*link_reconnect)(void);
	int link_reconnect_cnt;
	void (*wait_cp_connect)(int);

	struct workqueue_struct *wq;
	struct delayed_work link_pm_work;
	struct delayed_work link_pm_start;
	struct delayed_work link_reconnect_work;
	bool resume_requested;
	bool link_pm_active;
	int resume_retry_cnt;

	struct wake_lock l2_wake;
	struct wake_lock rpm_wake;
	struct notifier_block usb_notifier;
	struct notifier_block pm_notifier;
	bool dpm_suspending;

	/* for SE interface test */
	bool disable_recovery;
	bool roothub_resume_req;
	bool apinit_l0_req;

	struct list_head link;
	spinlock_t lock;
	struct usb_device *udev;
	struct usb_device *hdev;

	int rpm_suspending_cnt;
};

struct mif_skb_pool;

#define MIF_NET_SUSPEND_RF_STOP         (0x1<<0)
#define MIF_NET_SUSPEND_LINK_WAKE	(0x1<<1)
#define MIF_NET_SUSPEND_GET_TX_BUF	(0x1<<2)
#define MIF_NET_SUSPEND_TX_RETRY	(0x1<<3)

struct if_usb_devdata {
	struct usb_interface *data_intf;
	struct usb_link_device *usb_ld;
	struct usb_device *usbdev;
	unsigned int tx_pipe;
	unsigned int rx_pipe;
	struct usb_host_endpoint *status;
	u8 disconnected;

	int format;
	int idx;

	/* Multi-URB style*/
	struct usb_anchor urbs;
	struct usb_anchor reading;

	struct urb *intr_urb;
	unsigned int rx_buf_size;
	enum ch_state state;

	struct usb_id_info *info;
	/* SubClass expend data - optional */
	void *sedata;
	struct io_device *iod;
	unsigned long flags;

	struct sk_buff_head free_rx_q;
	struct sk_buff_head sk_tx_q;
	unsigned tx_pend;
	struct timespec txpend_ts;
	struct rtc_time txpend_tm;
	struct usb_anchor tx_deferd_urbs;
	struct net_device *ndev;
	int net_suspend;
	bool net_connected;

	bool defered_rx;
	struct delayed_work rx_defered_work;

	struct mif_skb_pool *ntb_pool;
};

struct usb_link_device {
	/*COMMON LINK DEVICE*/
	struct link_device ld;

	/*USB SPECIFIC LINK DEVICE*/
	struct usb_device	*usbdev;
	int max_link_ch;
	int max_acm_ch;
	int acm_cnt;
	int ncm_cnt;
	struct if_usb_devdata *devdata;
	unsigned int		suspended;
	int if_usb_connected;

	/* LINK PM DEVICE DATA */
	struct link_pm_data *link_pm_data;

	struct mif_skb_pool skb_pool;
	struct delayed_work link_event;
	unsigned long events;

	/* for debug */
	unsigned debug_pending;
	unsigned tx_cnt;
	unsigned rx_cnt;
	unsigned tx_err;
	unsigned rx_err;
};

enum bit_link_events {
	LINK_EVENT_RECOVERY,
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_usb_link_device(linkdev) \
			container_of(linkdev, struct usb_link_device, ld)

#ifdef FOR_TEGRA
extern void tegra_ehci_txfilltuning(void);
#define ehci_vendor_txfilltuning tegra_ehci_txfilltuning
#else
#define ehci_vendor_txfilltuning()
#endif

int usb_tx_skb(struct if_usb_devdata *pipe_data, struct sk_buff *skb);
extern int usb_resume(struct device *dev, pm_message_t msg);
#endif
