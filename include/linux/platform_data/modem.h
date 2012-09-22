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
	IMC_XMM6262,
	VIA_CBP71,
	VIA_CBP72,
	SEC_CMC221,
	QC_MDM6600,
	QC_ESC6270,
	SPRD_SC8803,
	DUMMY,
	MAX_MODEM_TYPE
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_CMD,
	IPC_BOOT,
	IPC_MULTI_RAW,
	IPC_RAMDUMP,
	MAX_DEV_FORMAT,
};
#define MAX_IPC_DEV	(IPC_RFS + 1)	/* FMT, RAW, RFS */
#define MAX_SIPC5_DEV	(IPC_RAW + 1)	/* FMT, RAW */

enum modem_io {
	IODEV_MISC,
	IODEV_NET,
	IODEV_DUMMY,
};

enum modem_link {
	LINKDEV_UNDEFINED,
	LINKDEV_MIPI,
	LINKDEV_DPRAM,
	LINKDEV_SPI,
	LINKDEV_USB,
	LINKDEV_HSIC,
	LINKDEV_C2C,
	LINKDEV_PLD,
	LINKDEV_MAX,
};
#define LINKTYPE(modem_link) (1u << (modem_link))

enum modem_network {
	UMTS_NETWORK,
	CDMA_NETWORK,
	TDSCDMA_NETWORK,
	LTE_NETWORK,
};

enum sipc_ver {
	NO_SIPC_VER = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41 = 41,
	SIPC_VER_42 = 42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER,
};

/**
 * struct modem_io_t - declaration for io_device
 * @name:	device name
 * @id:		contain format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 *
 * This structure is used in board-*-modem.c
 */
struct modem_io_t {
	char *name;
	int   id;
	enum dev_format format;
	enum modem_io io_type;
	enum modem_link links;
	enum modem_link tx_link;
	bool rx_gather;
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

	/* usb hub only */
	int (*port_enable)(int, int);
	int (*hub_standby)(void *);
	void *hub_pm_data;
	bool has_usbhub;

	/* cpu/bus frequency lock */
	atomic_t freqlock;
	int (*freq_lock)(struct device *dev);
	int (*freq_unlock)(struct device *dev);

	int autosuspend_delay_ms; /* if zero, the default value is used */
	void (*ehci_reg_dump)(struct device *);
};

struct modemlink_pm_link_activectl {
	int gpio_initialized;
	int gpio_request_host_active;
};

#define RES_CP_ACTIVE_IRQ_ID	0
#define RES_DPRAM_MEM_ID	1
#define RES_DPRAM_IRQ_ID	2
#define RES_DPRAM_SFR_ID	3

enum dpram_type {
	EXT_DPRAM,
	AP_IDPRAM,
	CP_IDPRAM,
	SHM_DPRAM,
	MAX_DPRAM_TYPE
};

#define DPRAM_SIZE_8KB		0x02000
#define DPRAM_SIZE_16KB		0x04000
#define DPRAM_SIZE_32KB		0x08000
#define DPRAM_SIZE_64KB		0x10000
#define DPRAM_SIZE_128KB	0x20000

enum dpram_speed {
	DPRAM_SPEED_LOW,
	DPRAM_SPEED_MID,
	DPRAM_SPEED_HIGH,
	MAX_DPRAM_SPEED
};

struct dpram_circ {
	u16 __iomem *head;
	u16 __iomem *tail;
	u8  __iomem *buff;
	u32          size;
};

struct dpram_ipc_device {
	char name[16];
	int  id;

	struct dpram_circ txq;
	struct dpram_circ rxq;

	u16 mask_req_ack;
	u16 mask_res_ack;
	u16 mask_send;
};

struct dpram_ipc_map {
#if defined(CONFIG_LINK_DEVICE_PLD)
	u16 __iomem *mbx_ap2cp;
	u16 __iomem *magic_ap2cp;
	u16 __iomem *access_ap2cp;

	u16 __iomem *mbx_cp2ap;
	u16 __iomem *magic_cp2ap;
	u16 __iomem *access_cp2ap;

	struct dpram_ipc_device dev[MAX_IPC_DEV];

	u16 __iomem *address_buffer;
#else
	u16 __iomem *magic;
	u16 __iomem *access;

	struct dpram_ipc_device dev[MAX_IPC_DEV];

	u16 __iomem *mbx_cp2ap;
	u16 __iomem *mbx_ap2cp;
#endif
};

struct modemlink_dpram_control {
	void (*reset)(void);
	void (*clear_intr)(void);
	u16 (*recv_intr)(void);
	void (*send_intr)(u16);
	u16 (*recv_msg)(void);
	void (*send_msg)(u16);

	int (*wakeup)(void);
	void (*sleep)(void);

	void (*setup_speed)(enum dpram_speed);

	enum dpram_type dp_type;	/* DPRAM type */
	int aligned;			/* aligned access is required */
	u8 __iomem *dp_base;
	u32 dp_size;

	int dpram_irq;
	unsigned long dpram_irq_flags;

	int max_ipc_dev;
	struct dpram_ipc_map *ipc_map;

	unsigned boot_size_offset;
	unsigned boot_tag_offset;
	unsigned boot_count_offset;
	unsigned max_boot_frame_size;
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
	unsigned gpio_ap_dump_int;
	unsigned gpio_flm_uart_sel;
#if defined(CONFIG_MACH_M0_CTC)
	unsigned gpio_flm_uart_sel_rev06;
	unsigned gpio_host_wakeup;
#endif
	unsigned gpio_cp_warm_reset;
	unsigned gpio_sim_detect;
#if defined(CONFIG_LINK_DEVICE_DPRAM) || defined(CONFIG_LINK_DEVICE_PLD)
	unsigned gpio_dpram_int;
#endif

#ifdef CONFIG_LINK_DEVICE_PLD
	unsigned gpio_fpga1_creset;
	unsigned gpio_fpga1_cdone;
	unsigned gpio_fpga1_rst_n;
	unsigned gpio_fpga1_cs_n;

	unsigned gpio_fpga2_creset;
	unsigned gpio_fpga2_cdone;
	unsigned gpio_fpga2_rst_n;
	unsigned gpio_fpga2_cs_n;
#endif

#ifdef CONFIG_LTE_MODEM_CMC221
	unsigned gpio_dpram_status;
	unsigned gpio_dpram_wakeup;
	unsigned gpio_slave_wakeup;
	unsigned gpio_host_active;
	unsigned gpio_host_wakeup;
	int      irq_host_wakeup;
#endif
#ifdef CONFIG_MACH_U1_KOR_LGT
	unsigned gpio_cp_reset_msm;
	unsigned gpio_boot_sw_sel;
	void (*vbus_on)(void);
	void (*vbus_off)(void);
	struct regulator *cp_vbus;
#endif

#ifdef CONFIG_TDSCDMA_MODEM_SPRD8803
	unsigned gpio_ipc_mrdy;
	unsigned gpio_ipc_srdy;
	unsigned gpio_ipc_sub_mrdy;
	unsigned gpio_ipc_sub_srdy;
	unsigned gpio_ap_cp_int1;
	unsigned gpio_ap_cp_int2;
#endif

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	unsigned gpio_sim_io_sel;
	unsigned gpio_cp_ctrl1;
	unsigned gpio_cp_ctrl2;
#endif

	/* Switch with 2 links in a modem */
	unsigned gpio_dynamic_switching;

	/* Modem component */
	enum modem_network  modem_net;
	enum modem_t        modem_type;
	enum modem_link     link_types;
	char               *link_name;
#if defined(CONFIG_LINK_DEVICE_DPRAM) || defined(CONFIG_LINK_DEVICE_PLD)
	/* Link to DPRAM control functions dependent on each platform */
	struct modemlink_dpram_control *dpram_ctl;
#endif

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Information of IO devices */
	unsigned            num_iodevs;
	struct modem_io_t  *iodevs;

	/* Modem link PM support */
	struct modemlink_pm_data *link_pm_data;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);

	/* Handover with 2+ modems */
	bool use_handover;

	/* Debugging option */
	bool use_mif_log;
	/* SIM Detect polarity */
	bool sim_polarity;
};

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
