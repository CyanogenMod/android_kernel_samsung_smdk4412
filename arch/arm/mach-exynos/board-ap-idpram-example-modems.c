/* linux/arch/arm/mach-xxxx/board-u1-spr-modem.c
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
#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/s5p-idpram.h>
#include <linux/platform_data/modem.h>

static int __init init_modem(void);

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
		.name = "cdma_rfs0",
		.id = 0x33,		/* 0x13 (ch.id) | 0x20 (mask) */
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[3] = {
		.name = "cdma_multipdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[4] = {
		.name = "cdma_rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[5] = {
		.name = "cdma_rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[6] = {
		.name = "cdma_rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[7] = {
		.name = "cdma_rmnet3",
		.id = 0x2D,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[8] = {
		.name = "cdma_rmnet4",
		.id = 0x27,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[9] = {
		.name = "cdma_rmnet5", /* DM Port IO device */
		.id = 0x3A,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[10] = {
		.name = "cdma_rmnet6", /* AT CMD IO device */
		.id = 0x31,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[11] = {
		.name = "cdma_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
};

/*
	magic_code +
	access_enable +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +
	raw_rx_head + raw_rx_tail + raw_rx_buff +
	mbx_ap2cp +
	mbx_cp2ap
 =	2 +
	2 +
	2 + 2 + 1020 +
	2 + 2 + 7160 +
	2 + 2 + 1020 +
	2 + 2 + 7160 +
	2 +
	2
 =	16384
*/

#define DP_FMT_TX_BUFF_SZ	1020
#define DP_RAW_TX_BUFF_SZ	7160
#define DP_FMT_RX_BUFF_SZ	1020
#define DP_RAW_RX_BUFF_SZ	7160

#define INT_MASK_REQ_ACK_F	0x0020
#define INT_MASK_REQ_ACK_R	0x0010
#define INT_MASK_RES_ACK_F	0x0008
#define INT_MASK_RES_ACK_R	0x0004
#define INT_MASK_SEND_F		0x0002
#define INT_MASK_SEND_R		0x0001

struct s5p_idpram_ipc_cfg {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8 fmt_tx_buff[DP_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8 raw_tx_buff[DP_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8 fmt_rx_buff[DP_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8 raw_rx_buff[DP_RAW_RX_BUFF_SZ];

	u16 mbx_ap2cp;
	u16 mbx_cp2ap;
} __packed;

static struct dpram_ipc_map s5p_idpram_ipc_map;

static struct modemlink_dpram_data s5p_idpram = {
	.type = AP_IDPRAM,
	.ap = S5P,
	.aligned = 1,
	.ipc_map = &s5p_idpram_ipc_map,
	.clear_int2ap = idpram_clr_intr,
};

static struct resource cdma_modem_res[] = {
	[RES_DPRAM_MEM_ID] = {
		.name = STR_DPRAM_BASE,
		.start = IDPRAM_PHYS_ADDR,
		.end = IDPRAM_PHYS_END,
		.flags = IORESOURCE_MEM,
	},
};

/* To get modem state, register phone active irq using resource */
static struct modem_data cdma_modem_data = {
	.name = "qsc6085",

	.gpio_cp_on = GPIO_QSC_PHONE_ON,
	.gpio_cp_reset = GPIO_QSC_PHONE_RST,

	.gpio_pda_active = GPIO_PDA_ACTIVE,

	.gpio_phone_active = GPIO_QSC_PHONE_ACTIVE,
	.irq_phone_active = IRQ_QSC_PHONE_ACTIVE;

	.gpio_ipc_int2ap = 0,
	.irq_ipc_int2ap = IRQ_MODEM_IF,

	.gpio_ipc_int2cp = GPIO_DPRAM_INT_CP_N,

	.gpio_ap_wakeup = GPIO_C210_DPRAM_INT_N,

	.gpio_cp_dump_int = GPIO_CP_DUMP_INT,

	.modem_net = CDMA_NETWORK,
	.modem_type = QC_QSC6085,

	.link_types = LINKTYPE(LINKDEV_DPRAM),
	.link_name = "s5p_idpram",
	.dpram = &s5p_idpram,

	.num_iodevs = ARRAY_SIZE(cdma_io_devices),
	.iodevs = cdma_io_devices,

	.max_ipc_dev = (IPC_RAW + 1),
};

/* if use more than one modem device, then set id num */
static struct platform_device cdma_modem = {
	.name = "modem_if",
	.id = 1,
	.num_resources = ARRAY_SIZE(cdma_modem_res),
	.resource = cdma_modem_res,
	.dev = {
		.platform_data = &cdma_modem_data,
	},
};

static void config_cdma_modem_gpio(void)
{
	int err;
	unsigned gpio_cp_on = cdma_modem_data.gpio_cp_on;
	unsigned gpio_cp_rst = cdma_modem_data.gpio_cp_reset;
	unsigned gpio_pda_active = cdma_modem_data.gpio_pda_active;
	unsigned gpio_phone_active = cdma_modem_data.gpio_phone_active;
	unsigned gpio_ap_wakeup = cdma_modem_data.gpio_ap_wakeup;
	unsigned gpio_cp_dump_int = cdma_modem_data.gpio_cp_dump_int;
	unsigned gpio_ipc_int2cp = cdma_modem_data.gpio_ipc_int2cp;

	pr_info("MIF: <%s>\n", __func__);

	if (gpio_cp_on) {
		err = gpio_request(gpio_cp_on, "QSC_ON");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n", "QSC_ON");
		} else {
			gpio_direction_output(gpio_cp_on, 0);
			s3c_gpio_setpull(gpio_cp_on, S3C_GPIO_PULL_NONE);
		}
	}

	if (gpio_cp_rst) {
		err = gpio_request(gpio_cp_rst, "QSC_RST");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n", "QSC_RST");
		} else {
			gpio_direction_output(gpio_cp_rst, 0);
			s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
			s5p_gpio_set_drvstr(gpio_cp_rst, S5P_GPIO_DRVSTR_LV4);
		}
	}

	if (gpio_pda_active) {
		err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n", "PDA_ACTIVE");
		} else {
			gpio_direction_output(gpio_pda_active, 1);
			s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
		}
	}

	if (gpio_phone_active) {
		err = gpio_request(gpio_phone_active, "PHONE_ACTIVE");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n",
				"PHONE_ACTIVE");
		} else {
			s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_SFN(0xF));
			s3c_gpio_setpull(gpio_phone_active,
				S3C_GPIO_PULL_NONE);
		}
	}

	if (gpio_ap_wakeup) {
		err = gpio_request(gpio_ap_wakeup, "HOST_WAKEUP");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n",
				"HOST_WAKEUP");
		} else {
			s3c_gpio_cfgpin(gpio_ap_wakeup, S3C_GPIO_SFN(0xF));
			s3c_gpio_setpull(gpio_ap_wakeup, S3C_GPIO_PULL_NONE);
		}
	}

	if (gpio_cp_dump_int) {
		err = gpio_request(gpio_cp_dump_int, "CP_DUMP_INT");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n",
				"CP_DUMP_INT");
		} else {
			s3c_gpio_cfgpin(gpio_cp_dump_int, S3C_GPIO_SFN(0xF));
			s3c_gpio_setpull(gpio_cp_dump_int, S3C_GPIO_PULL_DOWN);
		}
	}

	if (gpio_ipc_int2cp) {
		err = gpio_request(gpio_ipc_int2cp, "DPRAM_INT2CP");
		if (err) {
			pr_err("MIF: fail to request gpio %s\n",
				"DPRAM_INT2CP");
		} else {
			s3c_gpio_cfgpin(gpio_ipc_int2cp, S3C_GPIO_SFN(0xF));
			gpio_direction_output(gpio_ipc_int2cp, 1);
			s3c_gpio_setpull(gpio_ipc_int2cp, S3C_GPIO_PULL_UP);
		}
	}
}

static u8 *setup_idpram_ipc_map(unsigned long addr, unsigned long size)
{
	int dp_addr = addr;
	int dp_size = size;
	u8 __iomem *dp_base = NULL;
	struct s5p_idpram_ipc_cfg *mem_cfg = NULL;
	struct dpram_ipc_device *dev = NULL;

	dp_addr = addr;
	dp_size = size;
	dp_base = (u8 *)ioremap_nocache(dp_addr, dp_size);
	if (!dp_base) {
		pr_err("mif: %s: ioremap fail\n", __func__);
		return NULL;
	}
	pr_info("mif: %s: DPRAM VA=0x%08X\n", __func__, (int)dp_base);

	s5p_idpram.base = (u8 __iomem *)dp_base;
	s5p_idpram.size = dp_size;

	/* Map for IPC */
	mem_cfg = (struct s5p_idpram_ipc_cfg *)dp_base;

	/* Magic code and access enable fields */
	s5p_idpram_ipc_map.magic = (u16 __iomem *)&mem_cfg->magic;
	s5p_idpram_ipc_map.access = (u16 __iomem *)&mem_cfg->access;

	/* FMT */
	dev = &s5p_idpram_ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)(&mem_cfg->fmt_tx_head);
	dev->txq.tail = (u16 __iomem *)(&mem_cfg->fmt_tx_tail);
	dev->txq.buff = (u8 __iomem *)(&mem_cfg->fmt_tx_buff[0]);
	dev->txq.size = DP_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)(&mem_cfg->fmt_rx_head);
	dev->rxq.tail = (u16 __iomem *)(&mem_cfg->fmt_rx_tail);
	dev->rxq.buff = (u8 __iomem *)(&mem_cfg->fmt_rx_buff[0]);
	dev->rxq.size = DP_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send = INT_MASK_SEND_F;

	/* RAW */
	dev = &s5p_idpram_ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)(&mem_cfg->raw_tx_head);
	dev->txq.tail = (u16 __iomem *)(&mem_cfg->raw_tx_tail);
	dev->txq.buff = (u8 __iomem *)(&mem_cfg->raw_tx_buff[0]);
	dev->txq.size = DP_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)(&mem_cfg->raw_rx_head);
	dev->rxq.tail = (u16 __iomem *)(&mem_cfg->raw_rx_tail);
	dev->rxq.buff = (u8 __iomem *)(&mem_cfg->raw_rx_buff[0]);
	dev->rxq.size = DP_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send = INT_MASK_SEND_R;

	/* Mailboxes */
	s5p_idpram_ipc_map.mbx_ap2cp = (u16 __iomem *)(&mem_cfg->mbx_ap2cp);
	s5p_idpram_ipc_map.mbx_cp2ap = (u16 __iomem *)(&mem_cfg->mbx_cp2ap);

	return dp_base;
}

static int init_idpram(void)
{
	int err;
	u8 *base;

	idpram_config_demux_gpio();

	err = idpram_init_sfr();
	if (err)
		return err;

	idpram_init_demux_mode();

	err = idpram_enable();
	if (err)
		return err;

	base = setup_idpram_ipc_map(IDPRAM_PHYS_ADDR, IDPRAM_SIZE);
	if (!base)
		return -EINVAL;

	return 0;
}

static int recovery_boot;

static int __init setup_bootmode(char *str)
{
	if (!str)
		return 0;

	pr_err("%s: %s\n", __func__, str);

	pr_err("%s: old recovery_boot = %d\n", __func__, recovery_boot);

	if ((*str == '2') || (*str == '4'))
		recovery_boot = 1;

	pr_err("%s: new recovery_boot = %d\n", __func__, recovery_boot);

	return 0;
}
__setup("bootmode=", setup_bootmode);

struct platform_device sec_device_dpram_recovery = {
	.name = "dpram-recovery",
	.id = -1,
};

static int __init init_modem(void)
{
	int err = 0;

	pr_info("mif: %s+++\n", __func__);

	if (recovery_boot) {
		pr_info("mif: %s: boot mode = recovery\n", __func__);
		platform_device_register(&sec_device_dpram_recovery);
	} else {
		pr_info("mif: %s: boot mode = normal\n", __func__);
		err = init_idpram();
		if (err)
			goto exit;
		config_cdma_modem_gpio();
		platform_device_register(&cdma_modem);
	}

	pr_info("mif: %s---\n", __func__);

exit:
	return err;
}
late_initcall(init_modem);

