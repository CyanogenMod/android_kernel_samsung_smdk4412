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
#include <linux/wakelock.h>

#ifndef __MODEM_LINK_DEVICE_DPRAM_H__
#define __MODEM_LINK_DEVICE_DPRAM_H__

#define FMT_IDX			0
#define RAW_IDX			1
#define MAX_IDX			2
#define DP_FMT_OUT_BUFF_SIZE		2044
#define DP_RAW_OUT_BUFF_SIZE		6128
#define DP_FMT_IN_BUFF_SIZE		2044
#define DP_RAW_IN_BUFF_SIZE		6128
struct dpram_circ {
	u16 head;
	u16 tail;
};

struct dpram_ota_header {
	u8 start_index;
	u16 nframes;
	u16 curframe;
	u16 len;

} __packed;

struct dpram_map {
	u16	magic;
	u16	enable;

	struct dpram_circ fmt_out;
	u8	fmt_out_buff[DP_FMT_OUT_BUFF_SIZE];

	struct dpram_circ raw_out;
	u8	raw_out_buff[DP_RAW_OUT_BUFF_SIZE];

	struct dpram_circ fmt_in;
	u8	fmt_in_buff[DP_FMT_IN_BUFF_SIZE];

	struct dpram_circ raw_in;
	u8	raw_in_buff[DP_RAW_IN_BUFF_SIZE];

	u8	padding[16];
#ifdef CONFIG_INTERNAL_MODEM_IF
	u16	mbx_ap2cp;
	u16	mbx_cp2ap;
#else
	u16	mbx_cp2ap;
	u16	mbx_ap2cp;
#endif
} __packed;

struct dpram_device {
	struct dpram_circ __iomem *in;
	u8 __iomem	*in_buff_addr;
	int		in_buff_size;

	struct dpram_circ __iomem *out;
	u8 __iomem	*out_buff_addr;
	int		out_buff_size;

	u16            mask_req_ack;
	u16            mask_res_ack;
	u16            mask_send;
};

struct ul_header {
#ifdef CONFIG_INTERNAL_MODEM_IF
	u16 bop;
	u16 total_frame;
	u16 curr_frame;
	u16 len;
#else
	u8 bop;
	u16 total_frame;
	u16 curr_frame;
	u16 len;
#endif
};

#ifdef CONFIG_INTERNAL_MODEM_IF
enum idpram_link_pm_states {
	IDPRAM_PM_SUSPEND_PREPARE,
	IDPRAM_PM_DPRAM_POWER_DOWN,
	IDPRAM_PM_SUSPEND_START,
	IDPRAM_PM_RESUME_START,
	IDPRAM_PM_ACTIVE,
};

struct idpram_link_pm_data {
	struct dpram_link_device *dpld;
	struct modem_data *mdata;

	unsigned last_pm_mailbox; /* 0xCC or 0x CA*/
	unsigned resume_retry;
	unsigned pm_states;

	int idpram_wpend;

	struct completion idpram_down;

	struct delayed_work resume_work;

	atomic_t read_lock;
	atomic_t write_lock;

	struct wake_lock host_wakeup_wlock;
	struct wake_lock rd_wlock;
	struct wake_lock hold_wlock;
	struct wake_lock wakeup_wlock;
};
#endif


struct dpram_link_device {
	struct link_device ld;

	/* maybe -list of io devices for the link device to use
	 * to find where to send incoming packets to */
	struct list_head list_of_io_devices;

	atomic_t raw_txq_req_ack_rcvd;
	atomic_t fmt_txq_req_ack_rcvd;

	struct dpram_map __iomem *dpram;
	struct dpram_device dev_map[MAX_IDX];

	struct wake_lock dpram_wake_lock;

	struct completion dpram_init_cmd;
	struct completion modem_pif_init_done;
	struct completion gota_download_start_complete;
	struct completion gota_send_done;
	struct completion gota_update_done;
	struct completion dump_receive_done;

	int irq;
#ifdef CONFIG_INTERNAL_MODEM_IF
	void (*clear_interrupt)(void);
	int (*board_ota_reset) (void);
	struct idpram_link_pm_data *link_pm_data;
	int (*init_magic_num)(struct dpram_link_device *dpld);
#else
	void (*clear_interrupt)(struct dpram_link_device *);
#endif
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_dpram_link_device(linkdev) \
			container_of(linkdev, struct dpram_link_device, ld)

#endif
