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
#include <linux/wacom_i2c.h>

unsigned char *Binary;
bool ums_binary;

#if defined(CONFIG_MACH_P4NOTE)
const unsigned int Binary_nLength = 0xBFFF;
const unsigned char Mpu_type = 0x22;
const unsigned int Firmware_version_of_file = 0x22F;
unsigned char *firmware_name = "";
const char Firmware_checksum[] = { 0x1F, 0xF1, 0x76, 0x81, 0x71, };

#include "wacom_i2c_firm_p4.h"
#elif defined(CONFIG_MACH_Q1_BD)
const unsigned int Binary_nLength = 0x7FFF;
const unsigned char Mpu_type = 0x26;
unsigned int Firmware_version_of_file = 0x340;
unsigned char *firmware_name = "epen/W8501.bin";

/* checksum for 0x340 */
const char Firmware_checksum[] = { 0x1F, 0xee, 0x06, 0x4b, 0xdd, };

#elif defined(CONFIG_MACH_T0)
const unsigned int Binary_nLength = 0xEFFF;
const unsigned char Mpu_type = 0x28;
unsigned int Firmware_version_of_file = 0x24F;
unsigned char *firmware_name = "epen/W9001_B746.bin";

char Firmware_checksum[] = { 0x1F, 0xDD, 0x87, 0x97, 0xFF, };
/*checksum for 0x13D*/
const char B713X_checksum[] = { 0x1F, 0xB5, 0x84, 0x38, 0x34, };
/*checksum for 0x16*/
const char B660X_checksum[] = { 0x1F, 0x83, 0x88, 0xD4, 0x67, };

#endif

void wacom_i2c_set_firm_data(unsigned char *Binary_new)
{
	if (Binary_new == NULL) {
#if defined(CONFIG_MACH_P4NOTE)
		Binary = (unsigned char *)Binary_48;
		ums_binary = false;
#elif defined(CONFIG_MACH_Q1_BD) || defined(CONFIG_MACH_T0)
		Binary = NULL;
#endif
		return;
	}

	Binary = (unsigned char *)Binary_new;
	ums_binary = true;
}

#ifdef CONFIG_MACH_T0
/*Return digitizer type according to board rev*/
int wacom_i2c_get_digitizer_type(void)
{
	if (system_rev >= WACOM_DTYPE_B746_HWID)
		return EPEN_DTYPE_B746;
	else if (system_rev >= WACOM_DTYPE_B713_HWID)
		return EPEN_DTYPE_B713;
	else
		return EPEN_DTYPE_B660;
}
#endif

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
#elif defined(CONFIG_MACH_T0)
	int type;
	int i;

	type = wacom_i2c_get_digitizer_type();

	if (type == EPEN_DTYPE_B746) {
		printk(KERN_DEBUG
			"[E-PEN] Digitizer type is B746\n");
	} else if (type == EPEN_DTYPE_B713) {
		printk(KERN_DEBUG
			"[E-PEN] Digitizer type is B713\n");
		firmware_name = "epen/W9001_B713.bin";
		Firmware_version_of_file = 0x13D;
		memcpy(Firmware_checksum, B713X_checksum,
			sizeof(Firmware_checksum));
	} else {
		printk(KERN_DEBUG
			"[E-PEN] Digitizer type is B660\n");
		firmware_name = "epen/W9001_B660.bin";
		Firmware_version_of_file = 0x16;
		memcpy(Firmware_checksum, B660X_checksum,
			sizeof(Firmware_checksum));
	}
	Binary = NULL;
#endif
}
