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

#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_T0_KOR_SKT)
unsigned int Firmware_version_of_file = 0x305;
unsigned char *firmware_name = "epen/W9001_B746S.bin";
char Firmware_checksum[] = { 0x1F, 0xE4, 0xB4, 0x71, 0x60, };
#elif defined(CONFIG_MACH_T0_KOR_KT)
unsigned int Firmware_version_of_file = 0x305;
unsigned char *firmware_name = "epen/W9001_B746K.bin";
char Firmware_checksum[] = { 0x1F, 0xE4, 0xB4, 0x71, 0x60, };
#elif defined(CONFIG_MACH_T0_KOR_LGT)
unsigned int Firmware_version_of_file = 0x404;
unsigned char *firmware_name = "epen/W9001_B746L.bin";
char Firmware_checksum[] = { 0x1F, 0x23, 0xD7, 0xCF, 0xDF, };
#endif
#elif defined(CONFIG_MACH_T0_JPN_LTE_DCM)
unsigned int Firmware_version_of_file = 0x304;
unsigned char *firmware_name = "epen/W9001_B746JD.bin";
char Firmware_checksum[] = { 0x1F, 0xA6, 0xFB, 0xBF, 0x11, };
#else
unsigned int Firmware_version_of_file = 0x25F;
unsigned char *firmware_name = "epen/W9001_B746.bin";

char Firmware_checksum[] = { 0x1F, 0x27, 0x85, 0x8B, 0xFB, };
#endif
/*checksum for 0x13D*/
const char B713X_checksum[] = { 0x1F, 0xB5, 0x84, 0x38, 0x34, };
/*checksum for 0x16*/
const char B660X_checksum[] = { 0x1F, 0x83, 0x88, 0xD4, 0x67, };
#elif defined(CONFIG_MACH_KONA)
const unsigned int Binary_nLength = 0xCBCB;
const unsigned char Mpu_type = 0x00;
unsigned int Firmware_version_of_file = 0x65D;
unsigned char *firmware_name = "epen/W9002_B781.bin";

char Firmware_checksum[] = { 0x1F, 0x72, 0xCD, 0x6E, 0xE3, };
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
