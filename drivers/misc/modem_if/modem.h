/*
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

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

enum modem_t {
	IMC_XMM6260,
	IMC_XMM6262,
	VIA_CBP71,
	VIA_CBP72,
	VIA_CBP82,
	SEC_CMC220,
	SEC_CMC221,
	SEC_SS222,
	QC_MDM6600,
	QC_ESC6270,
	QC_QSC6085,
	SPRD_SC8803,
	DUMMY,
	MAX_MODEM_TYPE
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_MULTI_RAW,
	IPC_CMD,
	IPC_BOOT,
	IPC_RAMDUMP,
	IPC_DEBUG,
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
	LINKDEV_USB,
	LINKDEV_HSIC,
	LINKDEV_DPRAM,
	LINKDEV_PLD,
	LINKDEV_C2C,
	LINKDEV_SHMEM,
	LINKDEV_SPI,
	LINKDEV_MAX
};
#define LINKTYPE(modem_link) (1u << (modem_link))

enum modem_network {
	UMTS_NETWORK,
	CDMA_NETWORK,
	TDSCDMA_NETWORK,
	LTE_NETWORK,
	MAX_MODEM_NETWORK
};

enum ap_type {
	S5P,
	MAX_AP_TYPE
};

enum sipc_ver {
	NO_SIPC_VER = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41 = 41,
	SIPC_VER_42 = 42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER
};

/**
 * struct modem_io_t - declaration for io_device
 * @name:	device name
 * @id:		for SIPC4, contains format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 *		for SIPC5, contains only 8-bit channel ID
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 * @app:	the name of the application that will use this IO device
 *
 * This structure is used in board-*-modems.c
 */
struct modem_io_t {
	char *name;
	int   id;
	enum dev_format format;
	enum modem_io io_type;
	enum modem_link links;
	enum modem_link tx_link;
	char *app;
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

#ifdef CONFIG_EXYNOS4_CPUFREQ
	/* cpu/bus frequency lock */
	atomic_t freqlock;
	atomic_t freq_dpramlock;
	int (*freq_lock)(struct device *dev);
	int (*freq_unlock)(struct device *dev);
	unsigned gpio_cpufreq_lock;
#endif

	int autosuspend_delay_ms; /* if zero, the default value is used */
	void (*ehci_reg_dump)(struct device *);
};

struct modemlink_pm_link_activectl {
	int gpio_initialized;
	int gpio_request_host_active;
};

#define RES_DPRAM_MEM_ID	0
#define RES_DPRAM_SFR_ID	1

#define STR_DPRAM_BASE		"dpram_base"
#define STR_DPRAM_SFR_BASE	"dpram_sfr_base"

enum dpram_type {
	EXT_DPRAM,
	AP_IDPRAM,
	CP_IDPRAM,
	PLD_DPRAM,
	MAX_DPRAM_TYPE
};

#define DPRAM_SIZE_8KB		(8 << 10)
#define DPRAM_SIZE_16KB		(16 << 10)
#define DPRAM_SIZE_32KB		(32 << 10)
#define DPRAM_SIZE_64KB		(64 << 10)
#define DPRAM_SIZE_128KB	(128 << 10)

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
	u16 __iomem *magic;
	u16 __iomem *access;

	struct dpram_ipc_device dev[MAX_IPC_DEV];

	u16 __iomem *mbx_cp2ap;
	u16 __iomem *mbx_ap2cp;
};

struct pld_ipc_map {
	u16 __iomem *mbx_ap2cp;
	u16 __iomem *magic_ap2cp;
	u16 __iomem *access_ap2cp;

	u16 __iomem *mbx_cp2ap;
	u16 __iomem *magic_cp2ap;
	u16 __iomem *access_cp2ap;

	struct dpram_ipc_device dev[MAX_IPC_DEV];

	u16 __iomem *address_buffer;
};

struct modemlink_dpram_data {
	enum dpram_type type;	/* DPRAM type */
	enum ap_type ap;	/* AP type for AP_IDPRAM */

	/* Stirct I/O access (e.g. ioread16(), etc.) is required */
	bool strict_io_access;

	/* Aligned access is required */
	int aligned;

	/* Disabled during phone booting */
	bool disabled;

	/* Virtual base address and size */
	u8 __iomem *base;
	u32 size;

	/* Pointer to an IPC map (DPRAM or PLD) */
	void *ipc_map;

	/* Timeout of waiting for RES_ACK from CP (in msec) */
	unsigned long res_ack_wait_timeout;

	unsigned boot_size_offset;
	unsigned boot_tag_offset;
	unsigned boot_count_offset;
	unsigned max_boot_frame_size;

	void (*setup_speed)(enum dpram_speed);
	void (*clear_int2ap)(void);
};

enum shmem_type {
	REAL_SHMEM,
	C2C_SHMEM,
	MAX_SHMEM_TYPE
};

#define STR_SHMEM_BASE		"shmem_base"

#define SHMEM_SIZE_1MB		(1 << 20)	/* 1 MB */
#define SHMEM_SIZE_2MB		(2 << 20)	/* 2 MB */
#define SHMEM_SIZE_4MB		(4 << 20)	/* 4 MB */

/* platform data */
struct modem_data {
	char *name;

	unsigned gpio_cp_on;
	unsigned gpio_cp_off;
	unsigned gpio_reset_req_n;
	unsigned gpio_cp_reset;

	/* for broadcasting AP's PM state (active or sleep) */
	unsigned gpio_pda_active;

	/* for checking aliveness of CP */
	unsigned gpio_phone_active;
	int irq_phone_active;

	/* for AP-CP IPC interrupt */
	unsigned gpio_ipc_int2ap;
	int irq_ipc_int2ap;
	unsigned long irqf_ipc_int2ap;	/* IRQ flags */
	unsigned gpio_ipc_int2cp;

	/* for AP-CP power management (PM) handshaking */
	unsigned gpio_ap_wakeup;
	int irq_ap_wakeup;
	unsigned gpio_ap_status;
	unsigned gpio_cp_wakeup;
	unsigned gpio_cp_status;
	int irq_cp_status;

	/* for USB/HSIC PM */
	unsigned gpio_host_wakeup;
	int irq_host_wakeup;
	unsigned gpio_host_active;
	unsigned gpio_slave_wakeup;
	unsigned gpio_hub_suspend;

	unsigned gpio_cp_dump_int;
	unsigned gpio_ap_dump_int;
	unsigned gpio_flm_uart_sel;
	unsigned gpio_cp_warm_reset;
#if defined(CONFIG_MACH_M0_CTC)
	unsigned gpio_flm_uart_sel_rev06;
#endif

	unsigned gpio_sim_detect;
	int irq_sim_detect;

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
	unsigned gpio_link_switch;

#ifdef CONFIG_EXYNOS4_CPUFREQ
	/* cpu/bus frequency lock */
	unsigned gpio_cpufreq_lock;
#endif

	/* Modem component */
	enum modem_network modem_net;
	enum modem_t modem_type;
	enum modem_link link_types;
	char *link_name;

	/* Link to DPRAM control functions dependent on each platform */
	struct modemlink_dpram_data *dpram;

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* the number of real IPC devices -> (IPC_RAW + 1) or (IPC_RFS + 1) */
	int max_ipc_dev;

	/* Information of IO devices */
	unsigned num_iodevs;
	struct modem_io_t *iodevs;

	/* Modem link PM support */
	struct modemlink_pm_data *link_pm_data;

	/* Handover with 2+ modems */
	bool use_handover;

	/* SIM Detect polarity */
	bool sim_polarity;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);
};

#define MODEM_BOOT_DEV_SPI "modem_boot_spi"

struct modem_boot_spi_platform_data {
	const char *name;
	unsigned int gpio_cp_status;
};

struct modem_boot_spi {
	struct miscdevice dev;
	struct spi_device *spi_dev;
	struct mutex lock;
	unsigned gpio_cp_status;
};
#define to_modem_boot_spi(misc)	container_of(misc, struct modem_boot_spi, dev);

struct utc_time {
	u16 year;
	u8 mon:4,
	   day:4;
	u8 hour;
	u8 min;
	u8 sec;
	u16 msec;
} __packed;

extern void get_utc_time(struct utc_time *utc);

#define LOG_TAG	"mif: "
#define CALLER	(__builtin_return_address(0))

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
