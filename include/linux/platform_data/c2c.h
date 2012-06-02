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

#include <linux/delay.h>

#define C2C_CP_RGN_ADDR		0x60000000
#define C2C_CP_RGN_SIZE		0x03800000	// 56 MB
#define C2C_SH_RGN_ADDR		(C2C_CP_RGN_ADDR + C2C_SH_RGN_ADDR)
#define C2C_SH_RGN_SIZE		0x00800000	// 8 MB


#define DPRAM_INTR_PORT_SIZE		2
#define DPRAM_MAGIC_CODE_SIZE		2
#define DPRAM_ACCESS_CODE_SIZE	2
#define DP_HEAD_SIZE	0x2
#define DP_TAIL_SIZE	0x2
#define DP_MAGIC_CODE	0xAA
/*
  Total size	=	magic_code + access_enable +
			fmt_tx_head + fmt_tx_tail + fmt_tx_buff +
			raw_tx_head + raw_tx_tail + raw_tx_buff +
			fmt_rx_head + fmt_rx_tail + fmt_rx_buff +
			raw_rx_head + raw_rx_tail + raw_rx_buff +
			padding +
			mbx_ap2cp + mbx_cp2ap
		=	2 + 2 +
			2 + 2 + 2044 +
			2 + 2 + 6128 +
			2 + 2 + 2044 +
			2 + 2 + 6128 +
			16 +
			2 + 2
		= 16384
*/

#define TRUE	1
#define FALSE	0

/* interrupt masks.*/
#define INT_MASK_VALID			0x0080
#define INT_MASK_CMD			0x0040
#define INT_MASK_REQ_ACK_F		0x0020
#define INT_MASK_REQ_ACK_R		0x0010
#define INT_MASK_RES_ACK_F		0x0008
#define INT_MASK_RES_ACK_R		0x0004
#define INT_MASK_SEND_F			0x0002
#define INT_MASK_SEND_R			0x0001

#define INT_CMD_INIT_START		0x0001
#define INT_CMD_INIT_END		0x0002
#define INT_CMD_REQ_ACTIVE		0x0003
#define INT_CMD_RES_ACTIVE		0x0004
#define INT_CMD_REQ_TIME_SYNC		0x0005
#define INT_CMD_PHONE_START		0x0008
#define INT_CMD_ERR_DISPLAY		0x0009
#define INT_CMD_PHONE_DEEP_SLEEP	0x000A
#define INT_CMD_NV_REBUILDING		0x000B
#define INT_CMD_EMER_DOWN		0x000C
#define INT_CMD_PIF_INIT_DONE		0x000D
#define INT_CMD_SILENT_NV_REBUILDING	0x000E
#define INT_CMD_NORMAL_POWER_OFF	0x000F

#define INT_CMD(x)			(INT_MASK_VALID | INT_MASK_CMD | x)
#define INT_NON_CMD(x)			(INT_MASK_VALID | x)

/* special interrupt cmd indicating modem boot failure. */
#define INT_POWERSAFE_FAIL		0xDEAD

#define FMT_IDX			0
#define RAW_IDX			1
#define MAX_IDX			2

#define GPIO_DPRAM_INT_N	62
#define IRQ_DPRAM_INT_N		gpio_to_irq(GPIO_DPRAM_INT_N)

#define GPIO_PHONE_ACTIVE 120

#define HDLC_START	0x7F
#define HDLC_END	0x7E
#define SIZE_OF_HDLC_START		1
#define SIZE_OF_HDLC_END		1

/* ioctl command definitions. */
#define IOC_MZ_MAGIC		('o')
#define DPRAM_PHONE_POWON	_IO(IOC_MZ_MAGIC, 0xd0)
#define DPRAM_PHONEIMG_LOAD	_IO(IOC_MZ_MAGIC, 0xd1)
#define DPRAM_NVDATA_LOAD	_IO(IOC_MZ_MAGIC, 0xd2)
#define DPRAM_PHONE_BOOTSTART	_IO(IOC_MZ_MAGIC, 0xd3)

/*related GPMC*/
#define OMAP44XX_GPMC_CS1_SIZE 0xC  /* 64M */
#define OMAP44XX_GPMC_CS1_MAP 0x04000000
#define DPRAM_GPMC_CONFIG1  0x00001201
#define DPRAM_GPMC_CONFIG2  0x000f1200
#define DPRAM_GPMC_CONFIG3  0x44040400
#define DPRAM_GPMC_CONFIG4  0x0e05f155
#define DPRAM_GPMC_CONFIG5  0x000e1016
#define DPRAM_GPMC_CONFIG6  0x060603c3
#define DPRAM_GPMC_CONFIG7  0x00000F44

#define GPMC_CONFIG1		(0x00)
#define GPMC_CONFIG2		(0x04)
#define GPMC_CONFIG3		(0x08)
#define GPMC_CONFIG4		(0x0C)
#define GPMC_CONFIG5		(0x10)
#define GPMC_CONFIG6		(0x14)
#define GPMC_CONFIG7		(0x18)
#define GPMC_CONFIG_CS1		(OMAP44XX_GPMC_BASE+0x90)
#define GPMC_CONFIG_WIDTH	(0x30)

#define REG32(A) (*(volatile unsigned long  *)(A))
#define GPMC_CONTROL_BASE_ADDR		0x50000000
#define GPMC_CONFIG1_1		(GPMC_CONTROL_BASE_ADDR + 0x90)
#define GPMC_CONFIG2_1		(GPMC_CONTROL_BASE_ADDR + 0x94)
#define GPMC_CONFIG3_1		(GPMC_CONTROL_BASE_ADDR + 0x98)
#define GPMC_CONFIG4_1		(GPMC_CONTROL_BASE_ADDR + 0x9C)
#define GPMC_CONFIG5_1		(GPMC_CONTROL_BASE_ADDR + 0xA0)
#define GPMC_CONFIG6_1		(GPMC_CONTROL_BASE_ADDR + 0xA4)
#define GPMC_CONFIG7_1		(GPMC_CONTROL_BASE_ADDR + 0xA8)
