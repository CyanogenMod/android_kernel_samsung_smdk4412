/*
 *  wacom_i2c_firm.c - Wacom G5 Digitizer Controller (I2C bus)
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
#include <linux/kernel.h>
unsigned char *Binary;

#if defined(CONFIG_MACH_P4NOTE)
const unsigned int Binary_nLength = 0xBFFF;
const unsigned char Mpu_type = 0x22;
const unsigned int Firmware_version_of_file = 0x200;
unsigned char *firmware_name = "";
const char Firmware_checksum[] = { 0x1F, 0x4d, 0x9d, 0x2f, 0x36, };

#include "wacom_i2c_firm_p4.h"
#elif defined(CONFIG_MACH_Q1_BD)
const unsigned int Binary_nLength = 0x7FFF;
const unsigned char Mpu_type = 0x26;
unsigned int Firmware_version_of_file = 0x340;
unsigned char *firmware_name = "epen/W8501.bin";

/* checksum for 0x340 */
const char Firmware_checksum[] = { 0x1F, 0xee, 0x06, 0x4b, 0xdd, };

#endif

void wacom_i2c_set_firm_data(unsigned char *Binary_new)
{
	if (Binary_new == NULL) {
#if defined(CONFIG_MACH_P4NOTE)
		Binary = (unsigned char *)Binary_48;
#elif defined(CONFIG_MACH_Q1_BD)
		Binary = NULL;
#endif
		return;
	}

	Binary = (unsigned char *)Binary_new;
}

void wacom_i2c_init_firm_data(void)
{
#if defined(CONFIG_MACH_P4NOTE)
	Binary = (unsigned char *)Binary_48;

#elif defined(CONFIG_MACH_Q1_BD)
	Binary = NULL;
	/* Separate board revision */
	if (system_rev < 6) {
		firmware_name = "epen/W8501_P48.bin";
		Firmware_version_of_file = 0x20A;
		printk(KERN_DEBUG
		       "[E-PEN] Wacom driver is working for 4.8mm pitch pad.\n");
	} else
		printk(KERN_DEBUG
		       "[E-PEN] Wacom driver is working for 4.4mm pitch pad.\n");
#endif
}
