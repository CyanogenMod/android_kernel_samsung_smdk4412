/* linux/arch/arm/mach-xxxx/board-u1-lgt-modems.c
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

#include <plat/devs.h>
#include <plat/ehci.h>

#define SROM_CS0_BASE		0x04000000
#define SROM_WIDTH		0x01000000
#define SROM_NUM_ADDR_BITS	14

/* For "bus width and wait control (BW)" register */
enum sromc_attr {
	SROMC_DATA_16   = 0x1,	/* 16-bit data bus	*/
	SROMC_BYTE_ADDR = 0x2,	/* Byte base address	*/
	SROMC_WAIT_EN   = 0x4,	/* Wait enabled		*/
	SROMC_BYTE_EN   = 0x8,	/* Byte access enabled	*/
	SROMC_MASK      = 0xF
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

/* For EDPRAM (External DPRAM) */
#define MDM_EDPRAM_SIZE		0x8000	/* 32 KB */


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
static int __init init_modem(void);

static struct sromc_cfg mdm_edpram_cfg = {
	.attr = (SROMC_DATA_16 | SROMC_WAIT_EN | SROMC_BYTE_EN),
	.size = MDM_EDPRAM_SIZE,
};

static struct sromc_access_cfg mdm_edpram_access_cfg[] = {
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
	2 + 2 + 4092 +
	2 + 2 + 12272 +
	2 + 2 + 4092 +
	2 + 2 + 12272 +
	16 +
	2 +
	2
 =	32768
*/

#define MDM_DP_FMT_TX_BUFF_SZ	4092
#define MDM_DP_RAW_TX_BUFF_SZ	12272
#define MDM_DP_FMT_RX_BUFF_SZ	4092
#define MDM_DP_RAW_RX_BUFF_SZ	12272

#define MAX_MDM_EDPRAM_IPC_DEV	2	/* FMT, RAW */

struct mdm_edpram_ipc_cfg {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8  fmt_tx_buff[MDM_DP_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8  raw_tx_buff[MDM_DP_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8  fmt_rx_buff[MDM_DP_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8  raw_rx_buff[MDM_DP_RAW_RX_BUFF_SZ];

	u8  padding[16];
	u16 mbx_ap2cp;
	u16 mbx_cp2ap;
};

struct mdm_edpram_boot_map {
	u8  __iomem *buff;
	u16  __iomem *frame_size;
	u16  __iomem *tag;
	u16  __iomem *count;
};

static struct dpram_ipc_map mdm_ipc_map;

struct _param_nv {
	unsigned char *addr;
	unsigned int size;
	unsigned int count;
	unsigned int tag;
};

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
static struct mdm_edpram_boot_map mdm_edpram_bt_map;

static void mdm_vbus_on(void);
static void mdm_vbus_off(void);

static void mdm_log_disp(struct modemlink_dpram_control *dpctl);
static int mdm_uload_step1(struct modemlink_dpram_control *dpctl);
static int mdm_uload_step2(void *arg, struct modemlink_dpram_control *dpctl);
static int mdm_dload_prep(struct modemlink_dpram_control *dpctl);
static int mdm_dload(void *arg, struct modemlink_dpram_control *dpctl);
static int mdm_nv_load(void *arg, struct modemlink_dpram_control *dpctl);
static int mdm_boot_start(struct modemlink_dpram_control *dpctl);
static int mdm_boot_start_post_proc(void);
static void mdm_boot_start_handler(struct modemlink_dpram_control *dpctl);
static void mdm_dload_handler(struct modemlink_dpram_control *dpctl, u16 cmd);
static void mdm_bt_map_init(struct modemlink_dpram_control *dpctl);
static void mdm_load_init(struct modemlink_dpram_control *dpctl);

static struct modemlink_dpram_control mdm_edpram_ctrl = {
	.log_disp = mdm_log_disp,
	.cpupload_step1 = mdm_uload_step1,
	.cpupload_step2 = mdm_uload_step2,
	.cpimage_load_prepare = mdm_dload_prep,
	.cpimage_load = mdm_dload,
	.nvdata_load = mdm_nv_load,
	.phone_boot_start = mdm_boot_start,
	.phone_boot_start_post_process = mdm_boot_start_post_proc,
	.phone_boot_start_handler = mdm_boot_start_handler,
	.dload_cmd_hdlr = mdm_dload_handler,
	.bt_map_init = mdm_bt_map_init,
	.load_init = mdm_load_init,

	.dp_type = EXT_DPRAM,

	.dpram_irq = IRQ_EINT(8),
	.dpram_irq_flags = IRQF_TRIGGER_FALLING,

	.max_ipc_dev = IPC_RFS,
	.ipc_map = &mdm_ipc_map,
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
	[20] = {
		.name = "umts_loopback0",
		.id = (31|0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
};

static struct modem_data cdma_modem_data = {
	.name = "mdm6600",

	.gpio_cp_on = GPIO_PHONE_ON,
	.gpio_cp_reset = GPIO_CP_RST,
	.gpio_pda_active = GPIO_PDA_ACTIVE,
	.gpio_phone_active = GPIO_PHONE_ACTIVE,
	.gpio_cp_reset_mdm = GPIO_CP_RST_MSM,
	.gpio_boot_sw_sel = GPIO_BOOT_SW_SEL,
	.vbus_on = mdm_vbus_on,
	.vbus_off = mdm_vbus_off,
	.cp_vbus = NULL,
	.gpio_cp_dump_int   = 0,
	.gpio_cp_warm_reset = 0,

	.modem_net = CDMA_NETWORK,
	.modem_type = QC_MDM6600,
	.link_types = LINKTYPE(LINKDEV_DPRAM),
	.link_name  = "mdm6600_edpram",
	.dpram_ctl  = &mdm_edpram_ctrl,

	.ipc_version = SIPC_VER_41,

	.num_iodevs = ARRAY_SIZE(cdma_io_devices),
	.iodevs = cdma_io_devices,
};

static struct resource cdma_modem_res[] = {
	[0] = {
		.name  = "cp_active_irq",
		.start = IRQ_EINT(14),
		.end   = IRQ_EINT(14),
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

static void mdm_vbus_on(void)
{
	int err;

	if (system_rev >= 0x06) {
#ifdef GPIO_USB_BOOT_EN
		pr_info("%s : set USB_BOOT_EN\n", __func__);
		gpio_request(GPIO_USB_BOOT_EN, "USB_BOOT_EN");
		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		gpio_free(GPIO_USB_BOOT_EN);
#endif
	} else {
#ifdef GPIO_USB_OTG_EN
		gpio_request(GPIO_USB_OTG_EN, "USB_OTG_EN");
		gpio_direction_output(GPIO_USB_OTG_EN, 1);
		gpio_free(GPIO_USB_OTG_EN);
#endif
	}
	mdelay(10);

	if (!cdma_modem_data.cp_vbus) {
		cdma_modem_data.cp_vbus = regulator_get(NULL, "safeout2");
		if (IS_ERR(cdma_modem_data.cp_vbus)) {
			err = PTR_ERR(cdma_modem_data.cp_vbus);
			pr_err(" <%s> regualtor: %d\n", __func__, err);
			cdma_modem_data.cp_vbus = NULL;
		}
	}

	if (cdma_modem_data.cp_vbus) {
		pr_info("%s\n", __func__);
		regulator_enable(cdma_modem_data.cp_vbus);
	}
}

static void mdm_vbus_off(void)
{
	if (cdma_modem_data.cp_vbus) {
		pr_info("%s\n", __func__);
		regulator_disable(cdma_modem_data.cp_vbus);
	}

	if (system_rev >= 0x06) {
#ifdef GPIO_USB_BOOT_EN
		gpio_request(GPIO_USB_BOOT_EN, "USB_BOOT_EN");
		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		gpio_free(GPIO_USB_BOOT_EN);
#endif
	} else {
#ifdef GPIO_USB_OTG_EN
		gpio_request(GPIO_USB_OTG_EN, "USB_OTG_EN");
		gpio_direction_output(GPIO_USB_OTG_EN, 0);
		gpio_free(GPIO_USB_OTG_EN);
#endif
	}

}

static void mdm_log_disp(struct modemlink_dpram_control *dpctl)
{
	static unsigned char buf[151];
	u8 __iomem *tmp_buff = NULL;

	tmp_buff = dpctl->get_rx_buff(IPC_FMT);
	memcpy(buf, tmp_buff, (sizeof(buf)-1));

	pr_info("[LNK] | PHONE ERR MSG\t| CDMA Crash\n");
	pr_info("[LNK] | PHONE ERR MSG\t| %s\n", buf);
}

static int mdm_data_upload(struct _param_nv *param,
	struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	u16 in_interrupt = 0;
	int count = 0;

	while (1) {
		if (!gpio_get_value(GPIO_DPRAM_INT_N)) {
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

	param->size = ioread16(mdm_edpram_bt_map.frame_size);
	memcpy(param->addr, mdm_edpram_bt_map.buff, param->size);
	param->tag = ioread16(mdm_edpram_bt_map.tag);
	param->count = ioread16(mdm_edpram_bt_map.count);

	dpctl->clear_intr();
	dpctl->send_msg(0xDB12);

	return retval;

}

static int mdm_data_load(struct _param_nv *param,
	struct modemlink_dpram_control *dpctl)
{
	int retval = 0;

	if (param->size <= DP_BOOT_FRAME_SIZE_LIMIT) {
		memcpy(mdm_edpram_bt_map.buff, param->addr, param->size);
		iowrite16(param->size, mdm_edpram_bt_map.frame_size);
		iowrite16(param->tag, mdm_edpram_bt_map.tag);
		iowrite16(param->count, mdm_edpram_bt_map.count);

		dpctl->clear_intr();
		dpctl->send_msg(0xDB12);

	} else {
		pr_err("[LNK/E]<%s> size:0x%x\n", __func__, param->size);
	}

	return retval;
}

static int mdm_uload_step1(struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	int count = 0;
	u16 in_interrupt = 0, out_interrupt = 0;

	pr_info("[LNK] +---------------------------------------------+\n");
	pr_info("[LNK] |            UPLOAD PHONE SDRAM               |\n");
	pr_info("[LNK] +---------------------------------------------+\n");

	while (1) {
		if (!gpio_get_value(GPIO_DPRAM_INT_N)) {
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

static int mdm_uload_step2(void *arg,
					struct modemlink_dpram_control *dpctl)
{
	int retval = 0;
	struct _param_nv param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	retval = mdm_data_upload(&param, dpctl);
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	if (!(param.count % 500))
		pr_info("[LNK] [param->count]:%d\n", param.count);

	if (param.tag == 4) {
		dpctl->clear_intr();
		enable_irq(mdm_edpram_ctrl.dpram_irq);
		pr_info("[LNK] [param->tag]:%d\n", param.tag);
	}

	retval = copy_to_user((unsigned long *)arg, &param, sizeof(param));
	if (retval < 0) {
		pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
		return -1;
	}

	return retval;
}

static int mdm_dload_prep(struct modemlink_dpram_control *dpctl)
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

static int mdm_dload(void *arg, struct modemlink_dpram_control *dpctl)
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
	data_param->tag = param.tag;

	if (check_param.rest_size < DP_BOOT_FRAME_SIZE_LIMIT)
		data_param->size = check_param.rest_size;

	retval = mdm_data_load(data_param, dpctl);

	while (1) {
		if (check_param.copy_complete) {
			check_param.copy_complete = 0;

			vfree(img);
			kfree(data_param);

			break;
		}
		msleep_interruptible(10);
		count++;
		if (count > 2000) {
			pr_err("[LNK/E]<%s:%d>\n", __func__, __LINE__);
			vfree(img);
			kfree(data_param);
			return -1;
		}
	}

	return retval;

}

static int mdm_nv_load(void *arg, struct modemlink_dpram_control *dpctl)
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
	data_param->count = param.count;
	data_param->tag = param.tag;

	if (check_param.rest_size < DP_BOOT_FRAME_SIZE_LIMIT)
		data_param->size = check_param.rest_size;

	retval = mdm_data_load(data_param, dpctl);

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

static int mdm_boot_start(struct modemlink_dpram_control *dpctl)
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

		mdm_data_load(data_param, dpctl);
	} else {
		data_param->tag = 0;
		check_param.copy_complete = 1;
	}

}

static int mdm_boot_start_post_proc(void)
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

static void mdm_boot_start_handler(struct modemlink_dpram_control *dpctl)
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

static void mdm_dload_handler(struct modemlink_dpram_control *dpctl, u16 cmd)
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

static void mdm_bt_map_init(struct modemlink_dpram_control *dpctl)
{
	mdm_edpram_bt_map.buff = (u8 *)(dpctl->dp_base);
	mdm_edpram_bt_map.frame_size  =
		(u16 *)(dpctl->dp_base + DP_BOOT_SIZE_OFFSET);
	mdm_edpram_bt_map.tag =
		(u16 *)(dpctl->dp_base + DP_BOOT_TAG_OFFSET);
	mdm_edpram_bt_map.count =
		(u16 *)(dpctl->dp_base + DP_BOOT_COUNT_OFFSET);
}


static void mdm_load_init(struct modemlink_dpram_control *dpctl)
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
	unsigned gpio_cp_rst = cdma_modem_data.gpio_cp_reset;
	unsigned gpio_pda_active = cdma_modem_data.gpio_pda_active;
	unsigned gpio_phone_active = cdma_modem_data.gpio_phone_active;
	unsigned gpio_cp_reset_mdm = cdma_modem_data.gpio_cp_reset_mdm;
	unsigned gpio_boot_sw_sel = cdma_modem_data.gpio_boot_sw_sel;

	pr_info("[MDM] <%s>\n", __func__);

	if (gpio_cp_on) {
		err = gpio_request(gpio_cp_on, "CP_ON");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "CP_ON", err);
		}
		gpio_direction_output(gpio_cp_on, 0);
	}

	if (gpio_cp_rst) {
		err = gpio_request(gpio_cp_rst, "CP_RST");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "CP_RST", err);
		}
		gpio_direction_output(gpio_cp_rst, 0);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
	}

	if (gpio_cp_reset_mdm) {
		err = gpio_request(gpio_cp_reset_mdm, "CP_RST_MSM");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "CP_RST_MSM", err);
		}
		gpio_direction_output(gpio_cp_reset_mdm, 0);
		s3c_gpio_cfgpin(gpio_cp_reset_mdm, S3C_GPIO_SFN(0x1));
		s3c_gpio_setpull(gpio_cp_reset_mdm, S3C_GPIO_PULL_NONE);
	}

	if (gpio_boot_sw_sel) {
		err = gpio_request(gpio_boot_sw_sel, "BOOT_SW_SEL");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "BOOT_SW_SEL", err);
		}
		gpio_direction_output(gpio_boot_sw_sel, 0);
		s3c_gpio_setpull(gpio_boot_sw_sel, S3C_GPIO_PULL_NONE);
	}

	if (gpio_pda_active) {
		err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "PDA_ACTIVE", err);
		}
		gpio_direction_output(gpio_pda_active, 0);
	}

	if (gpio_phone_active) {
		err = gpio_request(gpio_phone_active, "PHONE_ACTIVE");
		if (err) {
			printk(KERN_ERR "fail to request gpio %s : %d\n",
				   "PHONE_ACTIVE", err);
		}
		gpio_direction_input(gpio_phone_active);
		s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
	}

	printk(KERN_INFO "<%s> done\n", __func__);
}


static u8 *mdm_edpram_remap_mem_region(struct sromc_cfg *cfg)
{
	int			      dp_addr = 0;
	int			      dp_size = 0;
	u8 __iomem                   *dp_base = NULL;
	struct mdm_edpram_ipc_cfg    *ipc_map = NULL;
	struct dpram_ipc_device *dev = NULL;

	dp_addr = cfg->addr;
	dp_size = cfg->size;
	dp_base = (u8 *)ioremap_nocache(dp_addr, dp_size);
	if (!dp_base) {
		pr_err("[MDM] <%s> dpram base ioremap fail\n", __func__);
		return NULL;
	}
	pr_info("[MDM] <%s> DPRAM VA=0x%08X\n", __func__, (int)dp_base);

	mdm_edpram_ctrl.dp_base = (u8 __iomem *)dp_base;
	mdm_edpram_ctrl.dp_size = dp_size;

	/* Map for IPC */
	ipc_map = (struct mdm_edpram_ipc_cfg *)dp_base;

	/* Magic code and access enable fields */
	mdm_ipc_map.magic  = (u16 __iomem *)&ipc_map->magic;
	mdm_ipc_map.access = (u16 __iomem *)&ipc_map->access;

	/* FMT */
	dev = &mdm_ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)&ipc_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->fmt_tx_buff[0];
	dev->txq.size = MDM_DP_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->fmt_rx_buff[0];
	dev->rxq.size = MDM_DP_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send    = INT_MASK_SEND_F;

	/* RAW */
	dev = &mdm_ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)&ipc_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->raw_tx_buff[0];
	dev->txq.size = MDM_DP_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->raw_rx_buff[0];
	dev->rxq.size = MDM_DP_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send    = INT_MASK_SEND_R;

	/* Mailboxes */
	mdm_ipc_map.mbx_ap2cp = (u16 __iomem *)&ipc_map->mbx_ap2cp;
	mdm_ipc_map.mbx_cp2ap = (u16 __iomem *)&ipc_map->mbx_cp2ap;

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
	bw &= ~(SROMC_MASK << (csn << 2));
	bw |= (cfg->attr << (csn << 2));
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

static int __init init_modem(void)
{
	struct sromc_cfg *cfg = NULL;
	struct sromc_access_cfg *acc_cfg = NULL;

	mdm_edpram_cfg.csn = 0;
	mdm_edpram_ctrl.dpram_irq = IRQ_EINT(8);
	mdm_edpram_cfg.addr = SROM_CS0_BASE + (SROM_WIDTH * mdm_edpram_cfg.csn);
	mdm_edpram_cfg.end  = mdm_edpram_cfg.addr + mdm_edpram_cfg.size - 1;

	config_dpram_port_gpio();
	config_cdma_modem_gpio();

	init_sromc();

	cfg = &mdm_edpram_cfg;
	acc_cfg = &mdm_edpram_access_cfg[DPRAM_SPEED_LOW];
	setup_sromc(cfg->csn, cfg, acc_cfg);

	if (!mdm_edpram_remap_mem_region(&mdm_edpram_cfg))
		return -1;
	platform_device_register(&cdma_modem);

	return 0;
}
late_initcall(init_modem);
/*device_initcall(init_modem);*/
