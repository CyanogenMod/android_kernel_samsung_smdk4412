/*
 * tcbd_feature.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TCBD_API_FEATURE_H__
#define __TCBD_API_FEATURE_H__

#define __AGC_TABLE_IN_DSP__

#define __CSPI_ONLY__
#undef __I2C_STS__

#undef __ALWAYS_FIC_ON__
#undef __CALLBACK_BUFFER_HEADER__

#if defined(__I2C_STS__)
#define __STATUS_IN_REGISTER__
#else /*__I2C_STS__*/
#undef __STATUS_IN_REGISTER__
#define __STATUS_IN_STREAM__
#endif /*!__I2C_STS__*/

#if defined(__CSPI_ONLY__)
#define __READ_FIXED_LENGTH__
#undef __READ_VARIABLE_LENGTH__
#endif /*__CSPI_ONLY__*/

#undef __DEBUG_DSP_ROM__

#define TCBD_MAX_NUM_SERVICE 6

#define TCBD_DEF_BANDWIDTH	(1500)
#define TCBD_STATUS_SIZE	(32)
#define TCBD_FIC_SIZE		(388)
#define TCBD_TS_SIZE		(188)
#define TCBD_OP_HEADER_SIZE	(4)
#define TCBD_MAX_FIFO_SIZE	(1024*16)
#define TCBD_CHIPID_VALUE	(0x37)

#if defined(__STATUS_IN_REGISTER__)
#if defined(__CSPI_ONLY__)
#define TCBD_THRESHOLD_FIC\
	(TCBD_FIC_SIZE + TCBD_STATUS_SIZE + TCBD_OP_HEADER_SIZE*2)
#elif defined(__I2C_STS__)
#define TCBD_THRESHOLD_FIC	(TCBD_FIC_SIZE)
#else /*__I2C_STS__*/
#error "you must define __I2C_STS__ or __CSPI_ONLY__"
#endif /*!__CSPI_ONLY__ && !__I2C_STS__*/
#else   /*  __STATUS_IN_REGISTER__ */
#define TCBD_THRESHOLD_FIC\
	(TCBD_FIC_SIZE+TCBD_STATUS_SIZE+TCBD_OP_HEADER_SIZE*2)
#endif  /* !__STATUS_IN_REGISTER__ */

#if defined(__CSPI_ONLY__)
#define TCBD_BUFFER_A_SIZE	(TCBD_MAX_FIFO_SIZE)
#define TCBD_BUFFER_B_SIZE	(0x0)
#define TCBD_BUFFER_C_SIZE	(0x0)
#define TCBD_BUFFER_D_SIZE	(0x0)

#define TCBD_MAX_THRESHOLD	(((1024*7)>>2)<<2)

#elif defined(__I2C_STS__)
#define TCBD_BUFFER_A_SIZE	(TCBD_THRESHOLD_FIC)
#define TCBD_BUFFER_B_SIZE	\
			(((TCBD_MAX_FIFO_SIZE-TCBD_BUFFER_A_SIZE)>>2)<<2)
#define TCBD_BUFFER_C_SIZE	(0x0)
#define TCBD_BUFFER_D_SIZE	(0x0)

#define TCBD_MAX_THRESHOLD	(((TCBD_BUFFER_B_SIZE>>1)>>2)<<2)
#endif	 /* __I2C_STS__ */

#define PHY_BASE_ADDR		(0x80000000)
#define PHY_MEM_FIFO_START_ADDR	(0x00000000)
#define PHY_MEM_ADDR_A_START	(PHY_BASE_ADDR + 0xa000)
#define PHY_MEM_ADDR_A_END\
	(PHY_MEM_ADDR_A_START+TCBD_BUFFER_A_SIZE-1)
#define PHY_MEM_ADDR_B_START\
	(PHY_MEM_ADDR_A_END+1)
#define PHY_MEM_ADDR_B_END\
	(PHY_MEM_ADDR_B_START+TCBD_BUFFER_B_SIZE-1)
#define PHY_MEM_ADDR_C_START\
	(PHY_MEM_ADDR_B_END+1)
#define PHY_MEM_ADDR_C_END\
	(PHY_MEM_ADDR_C_START+TCBD_BUFFER_C_SIZE-1)
#define PHY_MEM_ADDR_D_START\
	(PHY_MEM_ADDR_C_END+1)
#define PHY_MEM_ADDR_D_END\
	(PHY_MEM_ADDR_D_START+TCBD_BUFFER_D_SIZE-1)

/* CODE Memory Setting */
#define START_PC        (0x0000)
#define START_PC_OFFSET	(0x8000)
#define CODE_MEM_BASE	(PHY_BASE_ADDR+START_PC_OFFSET)
#define CODE_TABLEBASE_RAND      (0xF0020000)
#define CODE_TABLEBASE_DINT      (0xF0024000)
#define CODE_TABLEBASE_DAGU      (0xF0028000)
#define CODE_TABLEBASE_COL_ORDER (0xF002C000)

/* lock check time definition */
#define TDMB_OFDMDETECT_LOCK	(100)
#define TDMB_OFDMDETECT_RETRY	(2)
#define TDMB_CTO_LOCK		(100)
#define TDMB_CTO_RETRY		(3)
#define TDMB_CFO_LOCK		(20)
#define TDMB_CFO_RETRY		(3)

#endif /*__TCBD_API_FEATURE_H__*/
