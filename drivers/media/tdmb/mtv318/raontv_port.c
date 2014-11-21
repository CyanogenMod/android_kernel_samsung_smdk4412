/*
*
* File name: drivers/media/tdmb/mtv318/src/raontv_port.c
*
* Description : RAONTECH TV OEM source driver.
*
* Copyright (C) (2011, RAONTECH)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "raontv.h"
#include "raontv_internal.h"
#include <linux/spi/spi.h>

#include "tdmb.h"

/* Declares a variable of gurad object if neccessry. */
#if defined(RTV_IF_SPI) || ((defined(RTV_TDMB_ENABLE) \
	|| defined(RTV_DAB_ENABLE)) && !defined(RTV_FIC_POLLING_MODE))
	#if defined(__KERNEL__)
		struct mutex raontv_guard;
    #elif defined(WINCE)
		CRITICAL_SECTION raontv_guard;
    #else
	/* non-OS and RTOS. */
	#endif
#endif

void rtvOEM_ConfigureInterrupt(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2) \
	|| ((defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)) \
	&& !defined(RTV_FIC_POLLING_MODE))

	RTV_REG_SET(0x09, 0x00);
	/* [6]INT1 [5]INT0 - 1: Input mode, 0: Output mode */
	RTV_REG_SET(0x0B, 0x00);
	/* [2]INT1 PAD disable [1]INT0 PAD disable */

	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x28, 0x01);
	/* [5:3]INT1 out sel [2:0] INI0 out sel
	- 0:Toggle 1:Level,, 2:"0", 3:"1"*/

   #if defined(RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE)
	RTV_REG_SET(0x29, 0x08);
	/*[3] Interrupt status register clear condition
	- 0:read data by memory access 1:status register access*/
   #else
	RTV_REG_SET(0x29, 0x00);
	/*[3] Interrupt status register clear condition
	- 0:read data by memory access 1:status register access*/
   #endif
	RTV_REG_SET(0x2A, 0x13);
	/* [5]INT1 pol [4]INT0 pol
	- 0:Active High, 1:Active Low [3:0] Period = (INT_TIME+1)/8MHz*/
#endif
}

#if defined(RTV_IF_SPI)
unsigned char mtv_spi_read(unsigned char reg)
{
	int ret;
	u8 out_buf[2], read_out_buf[2];
	u8 in_buf[2];
	struct spi_message msg;
	struct spi_transfer msg_xfer0 = {
		.tx_buf = out_buf,
		.len		= 2,
		.cs_change	= 1,
	};

	struct spi_transfer msg_xfer1 = {
		.tx_buf = read_out_buf,
		.rx_buf = in_buf,
		.len		= 2,
		.cs_change	= 0,
	};

	spi_message_init(&msg);

	out_buf[0] = RAONTV_CHIP_ADDR;
	out_buf[1] = reg;
	spi_message_add_tail(&msg_xfer0, &msg);

	read_out_buf[0] = RAONTV_CHIP_ADDR|0x1;
	read_out_buf[1] = 0xFF;
	spi_message_add_tail(&msg_xfer1, &msg);

	ret = spi_sync(tdmb_get_spi_handle(), &msg);
	if (ret) {
		printk(KERN_ERR "[mtv818_spi_read]  mtv818_spi_read() error: %d\n",
		ret);
		return 0xFF;
	}

	/* printk(KERN_ERR
	"[mtv818_spi_read]  in_buf[0]: 0x%02X, in_buf[1]: 0x%02X\n",
	in_buf[0], in_buf[1]);   */

	return in_buf[1];
}

void mtv_spi_read_burst(unsigned char reg, unsigned char *buf, int size)
{
	int ret;
	u8 out_buf[2], read_out_buf[2];
	struct spi_message msg;
	struct spi_transfer msg_xfer0 = {
		.tx_buf = out_buf,
		.len		= 2,
		.cs_change	= 1,
	};

	struct spi_transfer msg_xfer1 = {
		.tx_buf = read_out_buf,
		.rx_buf = buf,
		.len		= size,
		.cs_change	= 0,
	};

	spi_message_init(&msg);
	out_buf[0] = RAONTV_CHIP_ADDR;
	out_buf[1] = reg;
	spi_message_add_tail(&msg_xfer0, &msg);

	read_out_buf[0] = RAONTV_CHIP_ADDR|0x1;
	spi_message_add_tail(&msg_xfer1, &msg);

	ret = spi_sync(tdmb_get_spi_handle(), &msg);
	if (ret)
		printk(KERN_ERR "[mtv818_spi_read_burst] error: %d\n", ret);
}

void mtv_spi_write(unsigned char reg, unsigned char val)
{
	u8 out_buf[3];
	u8 in_buf[3];
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.len		= 3,
		.cs_change	= 0,
	};
	int ret;

	spi_message_init(&msg);

	out_buf[0] = 0x86;
	out_buf[1] = reg;
	out_buf[2] = val;

	msg_xfer.tx_buf = out_buf;
	msg_xfer.rx_buf = in_buf;
	spi_message_add_tail(&msg_xfer, &msg);

	ret = spi_sync(tdmb_get_spi_handle(), &msg);
	if (ret)
		printk(KERN_ERR "[mtv818_spi_write] error: %d\n", ret);
}
#endif
