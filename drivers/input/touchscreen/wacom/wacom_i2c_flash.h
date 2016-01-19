/*
 *  wacom_i2c_flash.h - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_WACOM_I2C_FLASH_H
#define _LINUX_WACOM_I2C_FLASH_H

#define FLASH_START0 'f'
#define FLASH_START1 'l'
#define FLASH_START2 'a'
#define FLASH_START3 's'
#define FLASH_START4 'h'
#define FLASH_START5 '\r'
#define FLASH_ACK    0x06

#define PANA_QUERY   0x11

#define FLASH_END         0x80
#define FLASH_VERIFY      0x81
#define FLASH_WRITE       0x82
#define FLASH_READ        0x83
#define FLASH_ERASE       0x84
#define FLASH_SET_INFO    0x85
#define FLASH_END_TO_BOOT 0x87
#define FLASH_BAUDRATE    0x88

#define FLASH_QUERY    0xE0
#define FLASH_BLVER    0xE1
#define FLASH_UNITID   0xE2
#define FLASH_GET_INFO 0xE3
#define FLASH_FWVER    0xE4
#define FLASH_MPU      0xE8

#define WRITE_BUFF       300
#define BLOCK_SIZE_W     128
#define NUM_BLOCK_2WRITE 16
#define BANK             0
#define START_ADDR       0x1000
#define END_BLOCK        4

#define MAX_BLOCK_W8501  31
#define MPUVER_W8501     0x26
#define BLVER_W8501      0x41
#define MAX_ADDR_W8501   0x7FFF

#define MAX_BLOCK_514    47
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE)
#define MPUVER_514       0x22
#define BLVER_514        0x70
#else
#define MPUVER_514       0x27
#define BLVER_514        0x50
#endif
#define MAX_ADDR_514     0xBFFF

#define MPUVER_505             0x28
#define MAX_BLOCK_505          59
#define MAX_ADDR_505           0xEFFF
#define BLVER_505              0xFF

#define RETRY            1

#define ERR_FAILED_ENTER -1
#define ERR_UNSENT       -2
#define ERR_NOT_FLASH    -3
#define ERR_FAILED_EXIT  -4

#define PEN_QUERY '*'

extern int wacom_i2c_flash(struct wacom_i2c *wac_i2c);

#endif /* _LINUX_WACOM_I2C_FLASH_H */
