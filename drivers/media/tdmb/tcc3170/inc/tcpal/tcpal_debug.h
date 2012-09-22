/*
 * tcpal_debug.h
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

#ifndef __TCPAL_DEBUG_H__
#define __TCPAL_DEBUG_H__

#define DEBUG_ERROR		 0x00000001
#define DEBUG_INFO		  0x00000002
#define DEBUG_DRV_IO		0x00000004
#define DEBUG_API_COMMON	0x00000008
#define DEBUG_TCPAL_OS	  0x00000010
#define DEBUG_TCHAL		 0x00000020
#define DEBUG_TCPAL_CSPI	0x00000040
#define DEBUG_DRV_COMP	  0x00000080
#define DEBUG_DRV_RF		0x00000100
#define DEBUG_TCPAL_I2C	 0x00000200
#define DEBUG_DRV_PERI	  0x00000400
#define DEBUG_STREAM_PARSER 0x00000800
#define DEBUG_PARSING_TIME  0x00001000
#define DEBUG_PARSING_PROC  0x00002000
#define DEBUG_INTRRUPT	  0x00004000
#define DEBUG_STREAM_READ   0x00008000
#define DEBUG_STREAM_STACK  0x00010000
#define DEBUG_STATUS        0x00020000
#define DEBUG_PARSE_HEADER  0x00040000

#define MAX_SIZE_DSP_ROM (1024*10)
#define MAX_PATH 128

s32 printk(const char *fmt, ...);

#define tcbd_debug(__class, __msg, ...)\
do {\
	if (__class&tcbd_debug_class) {\
		if (__class == DEBUG_ERROR)\
			printk(KERN_ERR"[%s:%d] " __msg,\
				__func__, __LINE__, ##__VA_ARGS__);\
		else if (__class == DEBUG_INFO)\
			printk(KERN_INFO __msg, ##__VA_ARGS__);\
		else\
			printk(KERN_INFO"[%s:%d] " __msg,\
				__func__, __LINE__, ##__VA_ARGS__);\
	} \
} while (0)

s32 tcbd_debug_spur_dbg(void);
s32 tcbd_debug_rom_from_fs(void);
s32 *tcbd_debug_spur_clk_cfg(void);
char *tcbd_debug_rom_path(void);

void tcbd_debug_mbox_rx(u16 *_cmd, s32 *_len, u32 **_data);
void tcbd_debug_mbox_tx(u16 *_cmd, s32 *_len, u32 **_data);

extern u32 tcbd_debug_class;

#endif /*__TCPAL_DEBUG_H_*/
