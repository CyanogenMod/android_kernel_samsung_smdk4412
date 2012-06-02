/* linux/arch/arm/mach-xxxx/board-c1ctc-modems.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/if_arp.h>

/* inlcude platform specific file */
#include <linux/platform_data/modem.h>
#include <mach/sec_modem.h>
#include <mach/gpio.h>
#include <mach/gpio-exynos4.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-mem.h>
#include <plat/regs-srom.h>

#ifdef CONFIG_USBHUB_USB3503
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/platform_data/usb3503.h>
#endif
#include <plat/devs.h>
#include <plat/ehci.h>


#define SROM_CS0_BASE		0x04000000
#define SROM_WIDTH		0x01000000
#define SROM_NUM_ADDR_BITS	14

/*
 * For SROMC Configuration:
 * SROMC_ADDR_BYTE enable for byte access
 */
#define SROMC_DATA_16		0x1
#define SROMC_ADDR_BYTE		0x2
#define SROMC_WAIT_EN		0x4
#define SROMC_BYTE_EN		0x8
#define SROMC_MASK		0xF

/* Memory attributes */
enum sromc_attr {
	MEM_DATA_BUS_16BIT = 0x00000001,
	MEM_BYTE_ADDRESSABLE = 0x00000002,
	MEM_WAIT_EN = 0x00000004,
	MEM_BYTE_EN = 0x00000008,

};

/* DPRAM configuration */
struct sromc_cfg {
	enum sromc_attr attr;
	unsigned size;
	unsigned csn;		/* CSn #			*/
	unsigned addr;		/* Start address (physical)	*/
	unsigned end;		/* End address (physical)	*/
};

/* DPRAM access timing configuration */
struct sromc_access_cfg {
	u32 tacs;		/* Address set-up before CSn		*/
	u32 tcos;		/* Chip selection set-up before OEn	*/
	u32 tacc;		/* Access cycle				*/
	u32 tcoh;		/* Chip selection hold on OEn		*/
	u32 tcah;		/* Address holding time after CSn	*/
	u32 tacp;		/* Page mode access cycle at Page mode	*/
	u32 pmc;		/* Page Mode config			*/
};

/* For MDM6600 EDPRAM (External DPRAM) */
#define MSM_EDPRAM_SIZE		0x4000	/* 16 KB */


#define INT_MASK_REQ_ACK_F	0x0020
#define INT_MASK_REQ_ACK_R	0x0010
#define INT_MASK_RES_ACK_F	0x0008
#define INT_MASK_RES_ACK_R	0x0004
#define INT_MASK_SEND_F		0x0002
#define INT_MASK_SEND_R		0x0001

#define INT_MASK_REQ_ACK_RFS	0x0400 /* Request RES_ACK_RFS		*/
#define INT_MASK_RES_ACK_RFS	0x0200 /* Response of REQ_ACK_RFS	*/
#define INT_MASK_SEND_RFS	0x0100 /* Indicate sending RFS data	*/


/* Function prototypes */
static void config_dpram_port_gpio(void);
static void init_sromc(void);
static void setup_sromc(unsigned csn, struct sromc_cfg *cfg,
		struct sromc_access_cfg *acc_cfg);
static void setup_dpram_speed(unsigned csn, struct sromc_access_cfg *acc_cfg);
static int __init init_modem(void);

#ifdef CONFIG_USBHUB_USB3503
static int host_port_enable(int port, int enable);
#else
static int host_port_enable(int port, int enable)
{
	return s5p_ehci_port_control(&s5p_device_ehci, port, enable);
}
#endif

static struct sromc_cfg msm_edpram_cfg = {
	.attr = (MEM_DATA_BUS_16BIT | MEM_WAIT_EN | MEM_BYTE_EN),
	.size = MSM_EDPRAM_SIZE,
};

static struct sromc_access_cfg msm_edpram_access_cfg[] = {
	[DPRAM_SPEED_LOW] = {
		.tacs = 0x2 << 28,
		.tcos = 0x2 << 24,
		.tacc = 0x3 << 16,
		.tcoh = 0x2 << 12,
		.tcah = 0x2 << 8,
		.tacp = 0x2 << 4,
		.pmc  = 0x0 << 0,
	},
};

/*
	magic_code +
	access_enable +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +
	raw_rx_head + raw_rx_tail + raw_rx_buff +
	padding +
	mbx_cp2ap +
	mbx_ap2cp
 =	2 +
	2 +
	2 + 2 + 2044 +
	2 + 2 + 6128 +
	2 + 2 + 2044 +
	2 + 2 + 6128 +
	16 +
	2 +
	2
 =	16384
*/

#define MSM_DP_FMT_TX_BUFF_SZ	2044
#define MSM_DP_RAW_TX_BUFF_SZ	6128
#define MSM_DP_FMT_RX_BUFF_SZ	2044
#define MSM_DP_RAW_RX_BUFF_SZ	6128

#define MAX_MSM_EDPRAM_IPC_DEV	2	/* FMT, RAW */

struct msm_edpram_ipc_cfg {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8  fmt_tx_buff[MSM_DP_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8  raw_tx_buff[MSM_DP_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8  fmt_rx_buff[MSM_DP_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8  raw_rx_buff[MSM_DP_RAW_RX_BUFF_SZ];

	u8  padding[16];
	u16 mbx_ap2cp;
	u16 mbx_cp2ap;
};

struct msm_edpram_circ {
	u16 __iomem *head;
	u16 __iomem *tail;
	u8  __iomem *buff;
	u32          size;
};

struct msm_edpram_ipc_device {
	char name[16];
	int  id;

	struct msm_edpram_circ txq;
	struct msm_edpram_circ rxq;

	u16 mask_req_ack;
	u16 mask_res_ack;
	u16 mask_send;
};

struct msm_edpram_ipc_map {
	u16 __iomem *magic;
	u16 __iomem *access;

	struct msm_edpram_ipc_device dev[MAX_MSM_EDPRAM_IPC_DEV];

	u16 __iomem *mbx_ap2cp;
	u16 __iomem *mbx_cp2ap;
};


struct msm_edpram_boot_map {
	u8  __iomem *buff;
	u16  __iomem *frame_size;
	u16  __iomem *tag;
	u16  __iomem *count;
};

static struct msm_edpram_ipc_map msm_ipc_map;

struct _param_nv {
	unsigned char *addr;
	unsigned int size;
	unsigned int count;
	unsigned int tag;
};


#if (MSM_EDPRAM_SIZE == 0x4000)
/*
------------------
Buffer : 15KByte
------------------
Reserved: 1014Byte
------------------
SIZE: 2Byte
------------------
TAG: 2Byte
------------------
COUNT: 2Byte
------------------
AP -> CP Intr : 2Byte
------------------
CP -> AP Intr : 2Byte
------------------
*/
#define DP_BOOT_CLEAR_OFFSET	4
#define DP_BOOT_RSRVD_OFFSET	0x3C00
#define DP_BOOT_SIZE_OFFSET	    0x3FF6
#define DP_BOOT_TAG_OFFSET	    0x3FF8
#define DP_BOOT_COUNT_OFFSET	0x3FFA


#define DP_BOOT_FRAME_SIZE_LIMIT     0x3C00 /* 15KB = 15360byte = 0x3C00 */
#else
/*
------------------
Buffer : 31KByte
------------------
Reserved: 1014Byte
------------------
SIZE: 2Byte
------------------
TAG: 2Byte
------------------
COUNT: 2Byte
------------------
AP -> CP Intr : 2Byte
------------------
CP -> AP Intr : 2Byte
------------------
*/
#define DP_BOOT_CLEAR_OFFSET	4
#define DP_BOOT_RSRVD_OFFSET	0x7C00
#define DP_BOOT_SIZE_OFFSET	    0x7FF6
#define DP_BOOT_TAG_OFFSET	    0x7FF8
#define DP_BOOT_COUNT_OFFSET	0x7FFA


#define DP_BOOT_FRAME_SIZE_LIMIT     0x7C00 /* 31KB = 31744byte = 0x7C00 */
#endif

struct _param_check {
	unsigned int total_size;
	unsigned int rest_size;
	unsigned int send_size;
	unsigned int copy_start;
	unsigned int copy_complete;
	unsigned int boot_complete;
};

static struct _param_nv *data_param;
static struct _param_check check_param;

static unsigned int boot_start_complete;
static struct msm_edpram_boot_map msm_edpram_bt_map;
static struct msm_edpram_ipc_map msm_ipc_map;

static void msm_edpram_reset(void);
static void msm_edpram_clr_intr(void);
static u16  msm_edpram_recv_intr(void);
static void msm_edpram_send_intr(u16 irq_mask);
static u16  msm_edpram_recv_msg(void);
static void msm_edpram_send_msg(u16 msg);

static u16  msm_edpram_get_magic(void);
static void msm_edpram_set_magic(u16 value);
static u16  msm_edpram_get_access(void);
static void msm_edpram_set_access(u16 value);

static u32  msm_edpram_get_tx_head(int dev_id);
static u32  msm_edpram_get_tx_tail(int dev_id);
static void msm_edpram_set_tx_head(int dev_id, u32 head);
static void msm_edpram_set_tx_tail(int dev_id, u32 tail);
static u8 __iomem *msm_edpram_get_tx_buff(int dev_id);
static u32  msm_edpram_get_tx_buff_size(int dev_id);

static u32  msm_edpram_get_rx_head(int dev_id);
static u32  msm_edpram_get_rx_tail(int dev_id);
static void msm_edpram_set_rx_head(int dev_id, u32 head);
static void msm_edpram_set_rx_tail(int dev_id, u32 tail);
static u8 __iomem *msm_edpram_get_rx_buff(int dev_id);
static u32  msm_edpram_get_rx_buff_size(int dev_id);

static u16  msm_edpram_get_mask_req_ack(int dev_id);
static u16  msm_edpram_get_mask_res_ack(int dev_id);
static u16  msm_edpram_get_mask_send(int dev_id);

static void msm_log_disp(struct modemlink_dpram_control *dpctl);
static int msm_uload_step1(struct modemlink_dpram_control *dpctl);
static int msm_uload_step2(void *arg, struct modemlink_dpram_control *dpctl);
static int msm_dload_prep(struct modemlink_dpram_control *dpctl);
static int msm_dload(void *arg, struct modemlink_dpram_control *dpctl);
static int msm_nv_load(void *arg, struct modemlink_dpram_control *dpctl);
static int msm_boot_start(struct modemlink_dpram_control *dpctl);
static int msm_boot_start_post_proc(void);
static void msm_boot_start_handler(struct modemlink_dpram_control *dpctl);
static void msm_dload_handler(struct modemlink_dpram_control *dpctl, u16 cmd);
static void msm_bt_map_init(struct modemlink_dpram_control *dpctl);
static void msm_load_init(struct modemlink_dpram_control *dpctl);

static struct modemlink_dpram_control msm_edpram_ctrl = {
	.reset      = msm_edpram_reset,

	.clear_intr = msm_edpram_clr_intr,
	.recv_intr  = msm_edpram_recv_intr,
	.send_intr  = msm_edpram_send_intr,
	.recv_msg   = msm_edpram_recv_msg,
	.send_msg   = msm_edpram_send_msg,

	.get_magic  = msm_edpram_get_magic,
	.set_magic  = msm_edpram_set_magic,
	.get_access = msm_edpram_get_access,
	.set_access = msm_edpram_set_access,

	.get_tx_head = msm_edpram_get_tx_head,
	.get_tx_tail = msm_edpram_get_tx_tail,
	.set_tx_head = msm_edpram_set_tx_head,
	.set_tx_tail = msm_edpram_set_tx_tail,
	.get_tx_buff = msm_edpram_get_tx_buff,
	.get_tx_buff_size = msm_edpram_get_tx_buff_size,

	.get_rx_head = msm_edpram_get_rx_head,
	.get_rx_tail = msm_edpram_get_rx_tail,
	.set_rx_head = msm_edpram_set_rx_head,
	.set_rx_tail = msm_edpram_set_rx_tail,
	.get_rx_buff = msm_edpram_get_rx_buff,
	.get_rx_buff_size = msm_edpram_get_rx_buff_size,

	.get_mask_req_ack = msm_edpram_get_mask_req_ack,
	.get_mask_res_ack = msm_edpram_get_mask_res_ack,
	.get_mask_send    = msm_edpram_get_mask_send,

	.log_disp			= msm_log_disp,
	.cpupload_step1		= msm_uload_step1,
	.cpupload_step2		= msm_uload_step2,
	.cpimage_load		= msm_dload,
	.nvdata_load		= msm_nv_load,
	.phone_boot_start	= msm_boot_start,
	.dload_cmd_hdlr		= msm_dload_handler,
	.bt_map_init		= msm_bt_map_init,
	.load_init			= msm_load_init,
	.cpimage_load_prepare			= msm_dload_prep,
	.phone_boot_start_post_process	= msm_boot_start_post_proc,
	.phone_boot_start_handler		= msm_boot_start_handler,

	.dp_base = NULL,
	.dp_size = 0,
	.dp_type = EXT_DPRAM,

	.dpram_irq        = MSM_DPRAM_INT_IRQ,
	.dpram_irq_flags  = IRQF_TRIGGER_FALLING,
	.dpram_irq_name   = "MDM6600_EDPRAM_IRQ",
	.dpram_wlock_name = "MDM6600_EDPRAM_WLOCK",

	.max_ipc_dev = IPC_RFS,
};

/*
** CDMA target platform data
*/
static struct modem_io_t cdma_io_devices[] = {
	[0] = {
		.name = "cdma_boot0",
		.id = 0x1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[1] = {
		.name = "cdma_ipc0",
		.id = 0x1,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[2] = {
		.name = "cdma_multipdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[3] = {
		.name = "cdma_CSD",
		.id = (1|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[4] = {
		.name = "cdma_FOTA",
		.id = (2|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[5] = {
		.name = "cdma_GPS",
		.id = (5|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[6] = {
		.name = "cdma_XTRA",
		.id = (6|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[7] = {
		.name = "cdma_CDMA",
		.id = (7|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[8] = {
		.name = "cdma_EFS",
		.id = (8|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[9] = {
		.name = "cdma_TRFB",
		.id = (9|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[10] = {
		.name = "rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[11] = {
		.name = "rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[12] = {
		.name = "rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[13] = {
		.name = "rmnet3",
		.id = 0x2D,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[14] = {
		.name = "cdma_SMD",
		.id = (25|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[15] = {
		.name = "cdma_VTVD",
		.id = (26|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[16] = {
		.name = "cdma_VTAD",
		.id = (27|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[17] = {
		.name = "cdma_VTCTRL",
		.id = (28|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[18] = {
		.name = "cdma_VTENT",
		.id = (29|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[19] = {
		.name = "cdma_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
};

static struct modem_data cdma_modem_data = {
	.name = "mdm6600",

	.gpio_cp_on        = GPIO_CP_MSM_PWRON,
	.gpio_cp_off       = 0,
	.gpio_reset_req_n  = GPIO_CP_MSM_PMU_RST,
	.gpio_cp_reset     = GPIO_CP_MSM_RST,
	.gpio_pda_active   = GPIO_PDA_ACTIVE,
	.gpio_phone_active = GPIO_MSM_PHONE_ACTIVE,
	.gpio_flm_uart_sel = GPIO_BOOT_SW_SEL,

	.gpio_cp_dump_int   = 0,
	.gpio_cp_warm_reset = 0,

	.use_handover = false,

	.modem_net  = CDMA_NETWORK,
	.modem_type = QC_MDM6600,
	.link_types = LINKTYPE(LINKDEV_DPRAM),
	.link_name  = "mdm6600_edpram",
	.dpram_ctl  = &msm_edpram_ctrl,

	.num_iodevs = ARRAY_SIZE(cdma_io_devices),
	.iodevs     = cdma_io_devices,
};

static struct resource cdma_modem_res[] = {
	[0] = {
		.name  = "cp_active_irq",
		.start = MSM_PHONE_ACTIVE_IRQ,
		.end   = MSM_PHONE_ACTIVE_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device cdma_modem = {
	.name = "modem_if",
	.id = 2,
	.num_resources = ARRAY_SIZE(cdma_modem_res),
	.resource = cdma_modem_res,
	.dev = {
		.platform_data = &cdma_modem_data,
	},
};

static void msm_edpram_reset(void)
{
	return;
}

static void msm_edpram_clr_intr(void)
{
	ioread16(msm_ipc_map.mbx_cp2ap);
}

static u16 msm_edpram_recv_intr(void)
{
	return ioread16(msm_ipc_map.mbx_cp2ap);
}

static void msm_edpram_send_intr(u16 irq_mask)
{
	iowrite16(irq_mask, msm_ipc_map.mbx_ap2cp);
}

static u16 msm_edpram_recv_msg(void)
{
	return ioread16(msm_ipc_map.mbx_cp2ap);
}

static void msm_edpram_send_msg(u16 msg)
{
	iowrite16(msg, msm_ipc_map.mbx_ap2cp);
}

static u16 msm_edpram_get_magic(void)
{
	return ioread16(msm_ipc_map.magic);
}

static void msm_edpram_set_magic(u16 value)
{
	iowrite16(value, msm_ipc_map.magic);
}

static u16 msm_edpram_get_access(void)
{
	return ioread16(msm_ipc_map.access);
}

static void msm_edpram_set_access(u16 value)
{
	iowrite16(value, msm_ipc_map.access);
}

static u32 msm_edpram_get_tx_head(int dev_id)
{
	return ioread16(msm_ipc_map.dev[dev_id].txq.head);
}

static u32 msm_edpram_get_tx_tail(int dev_id)
{
	return ioread16(msm_ipc_map.dev[dev_id].txq.tail);
}

static void msm_edpram_set_tx_head(int dev_id, u32 head)
{
	iowrite16((u16)head, msm_ipc_map.dev[dev_id].txq.head);
}

static void msm_edpram_set_tx_tail(int dev_id, u32 tail)
{
	iowrite16((u16)tail, msm_ipc_map.dev[dev_id].txq.tail);
}

static u8 __iomem *msm_edpram_get_tx_buff(int dev_id)
{
	return msm_ipc_map.dev[dev_id].txq.buff;
}

static u32 msm_edpram_get_tx_buff_size(int dev_id)
{
	return msm_ipc_map.dev[dev_id].txq.size;
}

static u32 msm_edpram_get_rx_head(int dev_id)
{
	return ioread16(msm_ipc_map.dev[dev_id].rxq.head);
}

static u32 msm_edpram_get_rx_tail(int dev_id)
{
	return ioread16(msm_ipc_map.dev[dev_id].rxq.tail);
}

static void msm_edpram_set_rx_head(int dev_id, u32 head)
{
	return iowrite16((u16)head, msm_ipc_map.dev[dev_id].rxq.head);
}

static void msm_edpram_set_rx_tail(int dev_id, u32 tail)
{
	return iowrite16((u16)tail, msm_ipc_map.dev[dev_id].rxq.tail);
}

static u8 __iomem *msm_edpram_get_rx_buff(int dev_id)
{
	return msm_ipc_map.dev[dev_id].rxq.buff;
}

static u32 msm_edpram_get_rx_buff_size(int dev_id)
{
	return msm_ipc_map.dev[dev_id].rxq.size;
}

static u16 msm_edpram_get_mask_req_ack(int dev_id)
{
	return msm_ipc_map.dev[dev_id].mask_req_ack;
}

static u16 msm_edpram_get_mask_res_ack(int dev_id)
{
	return msm_ipc_map.dev[dev_id].mask_res_ack;
}

static u16 msm_edpram_get_mask_send(int dev_id)
{
	return msm_ipc_map.dev[dev_id].mask_send;
}

static void msm_log_disp(struct modemlink_dpram_control *dpctl)
{
	static unsigned char buf[151];
	u8 __iomem *tmp_buff = NULL;

	tmp_buff = dpctl->get_rx_buff(IPC_FMT);
	memcpy(buf, tmp_buff, (sizeof(buf)-1));

	pr_info("[LNK] | PHONE ERR MSG\t| CDMA Crash\n");
	pr_info("[LNK] | PHONE ERR MSG\t| %s\n", buf);
}

static int msm_data_upload(struct _param_nv *param,
	struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	u16 in_interrupt = 0;
	int count = 0;

	while (1) {
		if (!gpio_get_value(GPIO_MSM_DPRAM_INT)) {
			in_interrupt = dpctl->recv_msg();
			if (in_interrupt == 0xDBAB) {
				break;
			} else {
				pr_err("[LNK][intr]:0x%08x\n", in_interrupt);
				pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
				return -1;
			}
		}
		msleep_interruptible(1);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	param->size = ioread16(msm_edpram_bt_map.frame_size);
	memcpy(param->addr, msm_edpram_bt_map.buff, param->size);
	param->tag = ioread16(msm_edpram_bt_map.tag);
	param->count = ioread16(msm_edpram_bt_map.count);

	dpctl->clear_intr();
	dpctl->send_msg(0xDB12);

	return retval;

}

static int msm_data_load(struct _param_nv *param,
	struct modemlink_dpram_control *dpctl)
{
	int retval = 0;

	if (param->size <= DP_BOOT_FRAME_SIZE_LIMIT) {
		memcpy(msm_edpram_bt_map.buff, param->addr, param->size);
		iowrite16(param->size, msm_edpram_bt_map.frame_size);
		iowrite16(param->tag, msm_edpram_bt_map.tag);
		iowrite16(param->count, msm_edpram_bt_map.count);

		dpctl->clear_intr();
		dpctl->send_msg(0xDB12);

	} else {
		pr_err("[LNK/E]<%s> size:0x%x\n", __func__, param->size);
	}

	return retval;
}

static int msm_uload_step1(struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	int count = 0;
	u16 in_interrupt = 0, out_interrupt = 0;

	pr_info("[LNK] +---------------------------------------------+\n");
	pr_info("[LNK] |            UPLOAD PHONE SDRAM               |\n");
	pr_info("[LNK] +---------------------------------------------+\n");

	while (1) {
		if (!gpio_get_value(GPIO_MSM_DPRAM_INT)) {
			in_interrupt = dpctl->recv_msg();
			pr_info("[LNK] [in_interrupt] 0x%04x\n", in_interrupt);
			if (in_interrupt == 0x1234) {
				break;
			} else {
				pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
				return -1;
			}
		}
		msleep_interruptible(1);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			in_interrupt = dpctl->recv_msg();
			if (in_interrupt == 0x1234) {
				pr_info("[LNK] [in_interrupt]: 0x%04x\n",
							in_interrupt);
				break;
			}
			return -1;
		}
	}
	out_interrupt = 0xDEAD;
	dpctl->send_msg(out_interrupt);

	return retval;
}

static int msm_uload_step2(void *arg,
					struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	struct _param_nv param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	retval = msm_data_upload(&param, dpctl);
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	if (!(param.count % 500))
		pr_info("[LNK] [param->count]:%d\n", param.count);

	if (param.tag == 4) {
		dpctl->clear_intr();
		enable_irq(msm_edpram_ctrl.dpram_irq);
		pr_info("[LNK] [param->tag]:%d\n", param.tag);
	}

	retval = copy_to_user((unsigned long *)arg, &param, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	return retval;
}

static int msm_dload_prep(struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	int count = 0;

	while (1) {
		if (check_param.copy_start) {
			check_param.copy_start = 0;
			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	return retval;
}

static int msm_dload(void *arg, struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	int count = 0;
	unsigned char *img = NULL;
	struct _param_nv param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	img = vmalloc(param.size);
	if (img == NULL) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}
	memset(img, 0, param.size);
	memcpy(img, param.addr, param.size);

	data_param = kzalloc(sizeof(struct _param_nv), GFP_KERNEL);
	if (data_param == NULL)	{
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		vfree(img);
		return -1;
	}

	check_param.total_size = param.size;
	check_param.rest_size = param.size;
	check_param.send_size = 0;
	check_param.copy_complete = 0;

	data_param->addr = img;
	data_param->size = DP_BOOT_FRAME_SIZE_LIMIT;
	data_param->count = param.count;

	data_param->tag = 0x0001;

	if (check_param.rest_size < DP_BOOT_FRAME_SIZE_LIMIT)
		data_param->size = check_param.rest_size;

	retval = msm_data_load(data_param, dpctl);

	while (1) {
		if (check_param.copy_complete) {
			check_param.copy_complete = 0;

			vfree(img);
			kfree(data_param);

			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 1000) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			vfree(img);
			kfree(data_param);
			return -1;
		}
	}

	return retval;

}

static int msm_nv_load(void *arg, struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	int count = 0;
	unsigned char *img = NULL;
	struct _param_nv param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	img = vmalloc(param.size);
	if (img == NULL) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}
	memset(img, 0, param.size);
	memcpy(img, param.addr, param.size);

	data_param = kzalloc(sizeof(struct _param_nv), GFP_KERNEL);
	if (data_param == NULL) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		vfree(img);
		return -1;
	}

	check_param.total_size = param.size;
	check_param.rest_size = param.size;
	check_param.send_size = 0;
	check_param.copy_complete = 0;

	data_param->addr = img;
	data_param->size = DP_BOOT_FRAME_SIZE_LIMIT;
	data_param->count = 1;
	data_param->tag = 0x0002;

	if (check_param.rest_size < DP_BOOT_FRAME_SIZE_LIMIT)
		data_param->size = check_param.rest_size;

	retval = msm_data_load(data_param, dpctl);

	while (1) {
		if (check_param.copy_complete) {
			check_param.copy_complete = 0;

			vfree(img);
			kfree(data_param);

			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			vfree(img);
			kfree(data_param);
			return -1;
		}
	}

	return retval;

}

static int msm_boot_start(struct modemlink_dpram_control *dpctl)
{

	u16 out_interrupt = 0;
	int count = 0;

	/* Send interrupt -> '0x4567' */
	out_interrupt = 0x4567;
	dpctl->send_msg(out_interrupt);

	while (1) {
		if (check_param.boot_complete) {
			check_param.boot_complete = 0;
			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

static struct modemlink_dpram_control *tasklet_dpctl;

static void interruptable_load_tasklet_handler(unsigned long data);

static DECLARE_TASKLET(interruptable_load_tasklet,
	interruptable_load_tasklet_handler, (unsigned long) &tasklet_dpctl);

static void interruptable_load_tasklet_handler(unsigned long data)
{
	struct modemlink_dpram_control *dpctl =
		(struct modemlink_dpram_control *)
		(*((struct modemlink_dpram_control **) data));

	if (data_param == NULL) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return;
	}

	check_param.send_size += data_param->size;
	check_param.rest_size -= data_param->size;
	data_param->addr += data_param->size;

	if (check_param.send_size < check_param.total_size) {

		if (check_param.rest_size < DP_BOOT_FRAME_SIZE_LIMIT)
			data_param->size = check_param.rest_size;


		data_param->count += 1;

		msm_data_load(data_param, dpctl);
	} else {
		data_param->tag = 0;
		check_param.copy_complete = 1;
	}

}

static int msm_boot_start_post_proc(void)
{
	int count = 0;

	while (1) {
		if (boot_start_complete) {
			boot_start_complete = 0;
			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 200) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

static void msm_boot_start_handler(struct modemlink_dpram_control *dpctl)
{
	boot_start_complete = 1;

	/* Send INIT_END code to CP */
	pr_info("[LNK] <%s> Send 0x11C2 (INIT_END)\n", __func__);

	/*
	 * INT_MASK_VALID|INT_MASK_CMD|INT_MASK_CP_AIRPLANE_BOOT|
	 * INT_MASK_CP_AP_ANDROID|INT_MASK_CMD_INIT_END
	 */
	dpctl->send_intr((0x0080|0x0040|0x1000|0x0100|0x0002));
}

static void msm_dload_handler(struct modemlink_dpram_control *dpctl, u16 cmd)
{
	switch (cmd) {
	case 0x1234:
		check_param.copy_start = 1;
		break;

	case 0xDBAB:
		tasklet_schedule(&interruptable_load_tasklet);
		break;

	case 0xABCD:
		check_param.boot_complete = 1;
		break;

	default:
		pr_err("[LNK/Err] <%s> Unknown command.. %x\n", __func__, cmd);
	}
}

static void msm_bt_map_init(struct modemlink_dpram_control *dpctl)
{
	msm_edpram_bt_map.buff = (u8 *)(dpctl->dp_base);
	msm_edpram_bt_map.frame_size  =
		(u16 *)(dpctl->dp_base + DP_BOOT_SIZE_OFFSET);
	msm_edpram_bt_map.tag =
		(u16 *)(dpctl->dp_base + DP_BOOT_TAG_OFFSET);
	msm_edpram_bt_map.count =
		(u16 *)(dpctl->dp_base + DP_BOOT_COUNT_OFFSET);
}


static void msm_load_init(struct modemlink_dpram_control *dpctl)
{
	tasklet_dpctl = dpctl;
	if (tasklet_dpctl == NULL)
		pr_err("[LNK/Err] failed tasklet_dpctl remap\n");

	check_param.total_size = 0;
	check_param.rest_size = 0;
	check_param.send_size = 0;
	check_param.copy_start = 0;
	check_param.copy_complete = 0;
	check_param.boot_complete = 0;

	dpctl->clear_intr();
}

static void config_cdma_modem_gpio(void)
{
	int err;
	unsigned gpio_cp_on = cdma_modem_data.gpio_cp_on;
	unsigned gpio_cp_off = cdma_modem_data.gpio_cp_off;
	unsigned gpio_rst_req_n = cdma_modem_data.gpio_reset_req_n;
	unsigned gpio_cp_rst = cdma_modem_data.gpio_cp_reset;
	unsigned gpio_pda_active = cdma_modem_data.gpio_pda_active;
	unsigned gpio_phone_active = cdma_modem_data.gpio_phone_active;
	unsigned gpio_flm_uart_sel = cdma_modem_data.gpio_flm_uart_sel;

	pr_info("[MODEMS] <%s>\n", __func__);

	if (gpio_pda_active) {
		err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
		if (err) {
			pr_err("fail to request gpio %s\n", "PDA_ACTIVE");
		} else {
			gpio_direction_output(gpio_pda_active, 1);
			s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
			gpio_set_value(gpio_pda_active, 0);
		}
	}

	if (gpio_phone_active) {
		err = gpio_request(gpio_phone_active, "MSM_ACTIVE");
		if (err) {
			pr_err("fail to request gpio %s\n", "MSM_ACTIVE");
		} else {
			s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_SFN(0xF));
			s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
			irq_set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);
		}
	}

	if (gpio_flm_uart_sel) {
		err = gpio_request(gpio_flm_uart_sel, "BOOT_SW_SEL");
		if (err) {
			pr_err("fail to request gpio %s\n", "BOOT_SW_SEL");
		} else {
			gpio_direction_output(gpio_flm_uart_sel, 1);
			s3c_gpio_setpull(gpio_flm_uart_sel, S3C_GPIO_PULL_NONE);
			gpio_set_value(gpio_flm_uart_sel, 1);
		}
	}

	if (gpio_cp_on) {
		err = gpio_request(gpio_cp_on, "MSM_ON");
		if (err) {
			pr_err("fail to request gpio %s\n", "MSM_ON");
		} else {
			gpio_direction_output(gpio_cp_on, 1);
			s3c_gpio_setpull(gpio_cp_on, S3C_GPIO_PULL_NONE);
			gpio_set_value(gpio_cp_on, 0);
		}
	}

	if (gpio_cp_off) {
		err = gpio_request(gpio_cp_off, "MSM_OFF");
		if (err) {
			pr_err("fail to request gpio %s\n", "MSM_OFF");
		} else {
			gpio_direction_output(gpio_cp_off, 1);
			s3c_gpio_setpull(gpio_cp_off, S3C_GPIO_PULL_NONE);
			gpio_set_value(gpio_cp_off, 1);
		}
	}

	if (gpio_rst_req_n) {
		err = gpio_request(gpio_rst_req_n, "MSM_RST_REQ");
		if (err) {
			pr_err("fail to request gpio %s\n", "MSM_RST_REQ");
		} else {
			gpio_direction_output(gpio_rst_req_n, 1);
			s3c_gpio_setpull(gpio_rst_req_n, S3C_GPIO_PULL_NONE);
		}
		gpio_set_value(gpio_rst_req_n, 0);
	}

	if (gpio_cp_rst) {
		err = gpio_request(gpio_cp_rst, "MSM_RST");
		if (err) {
			pr_err("fail to request gpio %s\n", "MSM_RST");
		} else {
			gpio_direction_output(gpio_cp_rst, 1);
			s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
		}
		gpio_set_value(gpio_cp_rst, 0);
	}
}

static u8 *msm_edpram_remap_mem_region(struct sromc_cfg *cfg)
{
	int			      dp_addr = 0;
	int			      dp_size = 0;
	u8 __iomem                   *dp_base = NULL;
	struct msm_edpram_ipc_cfg    *ipc_map = NULL;
	struct msm_edpram_ipc_device *dev = NULL;

	dp_addr = cfg->addr;
	dp_size = cfg->size;
	dp_base = (u8 *)ioremap_nocache(dp_addr, dp_size);
	if (!dp_base) {
		pr_err("[MDM] <%s> dpram base ioremap fail\n", __func__);
		return NULL;
	}
	pr_info("[MDM] <%s> DPRAM VA=0x%08X\n", __func__, (int)dp_base);

	msm_edpram_ctrl.dp_base = (u8 __iomem *)dp_base;
	msm_edpram_ctrl.dp_size = dp_size;

	/* Map for IPC */
	ipc_map = (struct msm_edpram_ipc_cfg *)dp_base;

	/* Magic code and access enable fields */
	msm_ipc_map.magic  = (u16 __iomem *)&ipc_map->magic;
	msm_ipc_map.access = (u16 __iomem *)&ipc_map->access;

	/* FMT */
	dev = &msm_ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)&ipc_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->fmt_tx_buff[0];
	dev->txq.size = MSM_DP_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->fmt_rx_buff[0];
	dev->rxq.size = MSM_DP_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send    = INT_MASK_SEND_F;

	/* RAW */
	dev = &msm_ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)&ipc_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->raw_tx_buff[0];
	dev->txq.size = MSM_DP_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->raw_rx_buff[0];
	dev->rxq.size = MSM_DP_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send    = INT_MASK_SEND_R;

#if 0
	/* RFS */
	dev = &msm_ipc_map.dev[IPC_RFS];

	strcpy(dev->name, "RFS");
	dev->id = IPC_RFS;

	dev->txq.head = (u16 __iomem *)&ipc_map->rfs_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->rfs_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->rfs_tx_buff[0];
	dev->txq.size = MSM_DP_RFS_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->rfs_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->rfs_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->rfs_rx_buff[0];
	dev->rxq.size = MSM_DP_RFS_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_RFS;
	dev->mask_res_ack = INT_MASK_RES_ACK_RFS;
	dev->mask_send    = INT_MASK_SEND_RFS;
#endif

	/* Mailboxes */
	msm_ipc_map.mbx_ap2cp = (u16 __iomem *)&ipc_map->mbx_ap2cp;
	msm_ipc_map.mbx_cp2ap = (u16 __iomem *)&ipc_map->mbx_cp2ap;

	return dp_base;
}

/**
 *	DPRAM GPIO settings
 *
 *	SROM_NUM_ADDR_BITS value indicate the address line number or
 *	the mux/demux dpram type. if you want to set mux mode, define the
 *	SROM_NUM_ADDR_BITS to zero.
 *
 *	for CMC22x
 *	CMC22x has 16KB + a SFR register address.
 *	It used 14 bits (13bits for 16KB word address and 1 bit for SFR
 *	register)
 */
static void config_dpram_port_gpio(void)
{
	int addr_bits = SROM_NUM_ADDR_BITS;

	pr_info("[MDM] <%s> address line = %d bits\n", __func__, addr_bits);

	/*
	** Config DPRAM address/data GPIO pins
	*/

	/* Set GPIO for dpram address */
	switch (addr_bits) {
	case 0:
		break;

	case 13 ... 14:
		s3c_gpio_cfgrange_nopull(EXYNOS4_GPY3(0), EXYNOS4_GPIO_Y3_NR,
			S3C_GPIO_SFN(2));
		s3c_gpio_cfgrange_nopull(EXYNOS4_GPY4(0),
			addr_bits - EXYNOS4_GPIO_Y3_NR, S3C_GPIO_SFN(2));
		pr_info("[MDM] <%s> last data gpio EXYNOS4_GPY4(0) ~ %d\n",
			__func__, addr_bits - EXYNOS4_GPIO_Y3_NR);
		break;

	default:
		pr_err("[MDM/E] <%s> Invalid addr_bits!!!\n", __func__);
		return;
	}

	/* Set GPIO for dpram data - 16bit */
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPY5(0), 8, S3C_GPIO_SFN(2));
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPY6(0), 8, S3C_GPIO_SFN(2));

	/* Setup SROMC CSn pins */
	s3c_gpio_cfgpin(GPIO_DPRAM_CSN0, S3C_GPIO_SFN(2));

	/* Config OEn, WEn */
	s3c_gpio_cfgrange_nopull(GPIO_DPRAM_REN, 2, S3C_GPIO_SFN(2));

	/* Config LBn, UBn */
	s3c_gpio_cfgrange_nopull(GPIO_DPRAM_LBN, 2, S3C_GPIO_SFN(2));

	/* Config BUSY */
	s3c_gpio_cfgpin(GPIO_DPRAM_BUSY, S3C_GPIO_SFN(2));
}

static void init_sromc(void)
{
	struct clk *clk = NULL;

	/* SROMC clk enable */
	clk = clk_get(NULL, "sromc");
	if (!clk) {
		pr_err("[MDM/E] <%s> SROMC clock gate fail\n", __func__);
		return;
	}
	clk_enable(clk);
}

static void setup_sromc
(
	unsigned csn,
	struct sromc_cfg *cfg,
	struct sromc_access_cfg *acc_cfg
)
{
	unsigned bw = 0;
	unsigned bc = 0;
	void __iomem *bank_sfr = S5P_SROM_BC0 + (4 * csn);

	pr_err("[MDM] <%s> SROMC settings for CS%d...\n", __func__, csn);

	bw = __raw_readl(S5P_SROM_BW);
	bc = __raw_readl(bank_sfr);
	pr_err("[MDM] <%s> Old SROMC settings = BW(0x%08X), BC%d(0x%08X)\n",
		__func__, bw, csn, bc);

	/* Set the BW control field for the CSn */
	bw &= ~(SROMC_MASK << (csn * 4));

	if (cfg->attr | MEM_DATA_BUS_16BIT)
		bw |= (SROMC_DATA_16 << (csn * 4));

	if (cfg->attr | MEM_WAIT_EN)
		bw |= (SROMC_WAIT_EN << (csn * 4));

	if (cfg->attr | MEM_BYTE_EN)
		bw |= (SROMC_BYTE_EN << (csn * 4));

	writel(bw, S5P_SROM_BW);

	/* Set SROMC memory access timing for the CSn */
	bc = acc_cfg->tacs | acc_cfg->tcos | acc_cfg->tacc |
	     acc_cfg->tcoh | acc_cfg->tcah | acc_cfg->tacp | acc_cfg->pmc;

	writel(bc, bank_sfr);

	/* Verify SROMC settings */
	bw = __raw_readl(S5P_SROM_BW);
	bc = __raw_readl(bank_sfr);
	pr_err("[MDM] <%s> New SROMC settings = BW(0x%08X), BC%d(0x%08X)\n",
		__func__, bw, csn, bc);
}

static void setup_dpram_speed(unsigned csn, struct sromc_access_cfg *acc_cfg)
{
	void __iomem *bank_sfr = S5P_SROM_BC0 + (4 * csn);
	unsigned bc = 0;

	bc = __raw_readl(bank_sfr);
	pr_info("[MDM] <%s> Old CS%d setting = 0x%08X\n", __func__, csn, bc);

	/* SROMC memory access timing setting */
	bc = acc_cfg->tacs | acc_cfg->tcos | acc_cfg->tacc |
	     acc_cfg->tcoh | acc_cfg->tcah | acc_cfg->tacp | acc_cfg->pmc;
	writel(bc, bank_sfr);

	bc = __raw_readl(bank_sfr);
	pr_err("[MDM] <%s> New CS%d setting = 0x%08X\n", __func__, csn, bc);
}

static int __init init_modem(void)
{
	struct sromc_cfg *cfg = NULL;
	struct sromc_access_cfg *acc_cfg = NULL;

	msm_edpram_cfg.csn	= 0;
	msm_edpram_cfg.addr = SROM_CS0_BASE + (SROM_WIDTH * msm_edpram_cfg.csn);
	msm_edpram_cfg.end	= msm_edpram_cfg.addr + msm_edpram_cfg.size - 1;

	config_dpram_port_gpio();
	config_cdma_modem_gpio();

	init_sromc();
	cfg = &msm_edpram_cfg;
	acc_cfg = &msm_edpram_access_cfg[DPRAM_SPEED_LOW];
	setup_sromc(cfg->csn, cfg, acc_cfg);

	if (!msm_edpram_remap_mem_region(&msm_edpram_cfg))
		return -1;
	platform_device_register(&cdma_modem);

	return 0;
}
late_initcall(init_modem);
/*device_initcall(init_modem);*/

#ifdef CONFIG_USBHUB_USB3503
static int (*usbhub_set_mode)(struct usb3503_hubctl *, int);
static struct usb3503_hubctl *usbhub_ctl;

void set_host_states(struct platform_device *pdev, int type)
{
}

static int usb3503_hub_handler(void (*set_mode)(void), void *ctl)
{
	if (!set_mode || !ctl)
		return -EINVAL;

	usbhub_set_mode = (int (*)(struct usb3503_hubctl *, int))set_mode;
	usbhub_ctl = (struct usb3503_hubctl *)ctl;

	pr_info("[MDM] <%s> set_mode(%pF)\n", __func__, set_mode);

	return 0;
}

static int usb3503_hw_config(void)
{
	int err;

	err = gpio_request(GPIO_USB_HUB_CONNECT, "HUB_CONNECT");
	if (err) {
		pr_err("fail to request gpio %s\n", "HUB_CONNECT");
	} else {
		gpio_direction_output(GPIO_USB_HUB_CONNECT, 0);
		s3c_gpio_setpull(GPIO_USB_HUB_CONNECT, S3C_GPIO_PULL_NONE);
	}
	s5p_gpio_set_drvstr(GPIO_USB_HUB_CONNECT, S5P_GPIO_DRVSTR_LV1);

	err = gpio_request(GPIO_USB_BOOT_EN, "USB_BOOT_EN");
	if (err) {
		pr_err("fail to request gpio %s\n", "USB_BOOT_EN");
	} else {
		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
	}
	msleep(100);

	err = gpio_request(GPIO_USB_HUB_RST, "HUB_RST");
	if (err) {
		pr_err("fail to request gpio %s\n", "HUB_RST");
	} else {
		gpio_direction_output(GPIO_USB_HUB_RST, 0);
		s3c_gpio_setpull(GPIO_USB_HUB_RST, S3C_GPIO_PULL_NONE);
	}
	s5p_gpio_set_drvstr(GPIO_USB_HUB_RST, S5P_GPIO_DRVSTR_LV1);
	/* need to check drvstr 1 or 2 */

	/* for USB3503 26Mhz Reference clock setting */
	err = gpio_request(GPIO_USB_HUB_INT, "HUB_INT");
	if (err) {
		pr_err("fail to request gpio %s\n", "HUB_INT");
	} else {
		gpio_direction_output(GPIO_USB_HUB_INT, 1);
		s3c_gpio_setpull(GPIO_USB_HUB_INT, S3C_GPIO_PULL_NONE);
	}

	return 0;
}

static int usb3503_reset_n(int val)
{
	gpio_set_value(GPIO_USB_HUB_RST, 0);
	msleep(20);
	pr_info("[MDM] <%s> val = %d\n", __func__,
		gpio_get_value(GPIO_USB_HUB_RST));
	gpio_set_value(GPIO_USB_HUB_RST, !!val);

	pr_info("[MDM] <%s> val = %d\n", __func__,
		gpio_get_value(GPIO_USB_HUB_RST));

	udelay(5); /* need it ?*/
	return 0;
}

static struct usb3503_platform_data usb3503_pdata = {
	.initial_mode = USB3503_MODE_STANDBY,
	.reset_n = usb3503_reset_n,
	.register_hub_handler = usb3503_hub_handler,
	.port_enable = host_port_enable,
};

static struct i2c_board_info i2c_devs20_emul[] __initdata = {
	{
		I2C_BOARD_INFO(USB3503_I2C_NAME, 0x08),
		.platform_data = &usb3503_pdata,
	},
};

/* I2C20_EMUL */
static struct i2c_gpio_platform_data i2c20_platdata = {
	.sda_pin = GPIO_USB_HUB_SDA,
	.scl_pin = GPIO_USB_HUB_SCL,
	/*FIXME: need to timming tunning...  */
	.udelay	= 20,
};

static struct platform_device s3c_device_i2c20 = {
	.name = "i2c-gpio",
	.id = 20,
	.dev.platform_data = &i2c20_platdata,
};

static int __init init_usbhub(void)
{
	usb3503_hw_config();
	i2c_register_board_info(20, i2c_devs20_emul,
				ARRAY_SIZE(i2c_devs20_emul));

	platform_device_register(&s3c_device_i2c20);
	return 0;
}

device_initcall(init_usbhub);

static int host_port_enable(int port, int enable)
{
	int err, retry = 30;

	pr_info("[MDM] <%s> port(%d) control(%d)\n", __func__, port, enable);

	if (enable) {
		err = usbhub_set_mode(usbhub_ctl, USB3503_MODE_HUB);
		if (err < 0) {
			pr_err("[MDM] <%s> hub on fail\n", __func__);
			goto exit;
		}
		err = s5p_ehci_port_control(&s5p_device_ehci, port, 1);
		if (err < 0) {
			pr_err("[MDM] <%s> port(%d) enable fail\n", __func__,
				port);
			goto exit;
		}
#ifdef CONFIG_LTE_MODEM_CMC221
		msleep(20);
		err = gpio_direction_output(umts_modem_data.gpio_slave_wakeup,
			1);
		pr_err("[MDM] <%s> slave wakeup err(%d), en(%d), level(%d)\n",
			__func__, err, 1,
			gpio_get_value(umts_modem_data.gpio_slave_wakeup));

		while (!gpio_get_value(umts_modem_data.gpio_slave_wakeup)
			&& retry--)
			msleep(20);
		pr_err("[MDM] <%s> Host wakeup (%d) retry(%d)\n", __func__,
			gpio_get_value(umts_modem_data.gpio_host_wakeup),
			retry);
		err = gpio_direction_output(umts_modem_data.gpio_slave_wakeup,
			0);
		pr_err("[MDM] <%s> slave wakeup err(%d), en(%d), level(%d)\n",
			__func__, err, 0,
			gpio_get_value(umts_modem_data.gpio_slave_wakeup));
#endif
	} else {
		err = s5p_ehci_port_control(&s5p_device_ehci, port, 0);
		if (err < 0) {
			pr_err("[MDM] <%s> port(%d) enable fail\n", __func__,
				port);
			goto exit;
		}
		err = usbhub_set_mode(usbhub_ctl, USB3503_MODE_STANDBY);
		if (err < 0) {
			pr_err("[MDM] <%s> hub off fail\n", __func__);
			goto exit;
		}
	}

#ifdef CONFIG_LTE_MODEM_CMC221
	err = gpio_direction_output(umts_modem_data.gpio_host_active, enable);
	pr_info("[MDM] <%s> active state err(%d), en(%d), level(%d)\n",
		__func__, err, enable,
		gpio_get_value(umts_modem_data.gpio_host_active));
#endif

exit:
	return err;
}
#else
void set_host_states(struct platform_device *pdev, int type)
{
	if (active_ctl.gpio_initialized) {
		pr_err(" [MODEM_IF] Active States =%d, %s\n", type, pdev->name);
		gpio_direction_output(modem_link_pm_data.gpio_link_active,
			type);
	} else
		active_ctl.gpio_request_host_active = 1;
}
#endif

