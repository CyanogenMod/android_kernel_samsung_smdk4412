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

#ifndef __MODEM_IF_H__
#define __MODEM_IF_H__

enum modem_t {
	IMC_XMM6260,
	VIA_CBP71,
	SEC_CMC221,
	SEC_CMC220,
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_CMD,
	IPC_BOOT,
	IPC_MULTI_RAW,
	IPC_RAMDUMP,
};

enum modem_io {
	IODEV_MISC,
	IODEV_NET,
	IODEV_DUMMY,
};

enum modem_link {
	LINKDEV_MIPI,
	LINKDEV_DPRAM,
	LINKDEV_SPI,
	LINKDEV_USB,
	LINKDEV_MAX,
};

enum modem_network {
	UMTS_NETWORK,
	CDMA_NETWORK,
	LTE_NETWORK,
};

/* This structure is used in board-tuna-modem.c */
struct modem_io_t {
	char *name;
	int id;
	enum dev_format format;
	enum modem_io io_type;
	enum modem_link link;
};

struct modemlink_pm_data {
	char *name;
	/* link power contol 2 types : pin & regulator control */
	int (*link_ldo_enable)(bool);
	unsigned gpio_link_enable;
	unsigned gpio_link_active;
	unsigned gpio_link_hostwake;
	unsigned gpio_link_slavewake;
	int (*link_reconnect)(void);
	int (*port_enable)(int, int);
	int *p_hub_status;
	bool has_usbhub;

	atomic_t freqlock;
	int (*cpufreq_lock)(void);
	int (*cpufreq_unlock)(void);

	int autosuspend_delay_ms; /* if zero, the default value is used */
};

struct modemlink_pm_link_activectl {
	int gpio_initialized;
	int gpio_request_host_active;
};

/* dpram specific struct */
struct modemlink_shared_channel {
	char *name;
	unsigned out_offset;
	unsigned out_size;
	unsigned in_offset;
	unsigned in_size;
};

struct modemlink_memmap {
	unsigned magic_offset;
	unsigned magic_size;
	unsigned access_offset;
	unsigned access_size;
	unsigned in_box_offset;
	unsigned in_box_size;
	unsigned out_box_offset;
	unsigned out_box_size;

	unsigned num_shared_map;
	struct modemlink_shared_channel *shared_map;

	/*for interanl dpram interrupt clear*/
	void (*vendor_clear_irq)(void);
	int (*board_ota_reset)(void);
};

/* platform data */
struct modem_data {
	char *name;

	unsigned gpio_cp_on;
	unsigned gpio_cp_off;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_flm_uart_sel;
	unsigned gpio_cp_warm_reset;
	unsigned gpio_ap_wakeup;
#if defined(CONFIG_LTE_MODEM_CMC221) || defined(CONFIG_LTE_MODEM_CMC220)
	unsigned gpio_slave_wakeup;
	unsigned gpio_host_wakeup;
	unsigned gpio_host_active;
	int irq_host_wakeup;
#endif
	/* modem component */
	enum modem_t modem_type;
	enum modem_link link_type;
	enum modem_network modem_net;
	unsigned num_iodevs;
	struct modem_io_t *iodevs;
	void *modemlink_extension;
	void (*clear_intr)(void);
	int (*ota_reset)(void);
#ifdef CONFIG_INTERNAL_MODEM_IF
	void (*sfr_init)(void);
	unsigned gpio_mbx_intr;
#endif

	/* Modem link PM support */
	struct modemlink_pm_data *link_pm_data;
	/* Align mit mask used to adjust IPC RAW and MULTI_RAW HDLC
	Packets from interndal dpram */
	unsigned char align;
};

/* DEBUG */
#define LOG_TAG "mif: "

#define mif_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_trace(fmt, ...) \
	printk(KERN_DEBUG "mif: %s: %d: called(%pF): " fmt, \
		__func__, __LINE__, __builtin_return_address(0), ##__VA_ARGS__)

#endif
