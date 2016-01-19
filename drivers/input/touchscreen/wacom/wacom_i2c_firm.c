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

#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE)
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

/*KOR*/
#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_T0_KOR_SKT) || defined(CONFIG_MACH_T0_KOR_KT)
unsigned int Firmware_version_of_file = 0x312;
unsigned char *firmware_name = "epen/W9001_B746S.bin";
char Firmware_checksum[] = { 0x1F, 0x37, 0x28, 0xC8, 0x7C, };
#elif defined(CONFIG_MACH_T0_KOR_LGT)
unsigned int Firmware_version_of_file = 0x40A;
unsigned char *firmware_name = "epen/W9001_B746L.bin";
char Firmware_checksum[] = { 0x1F, 0xAA, 0x40, 0x78, 0x48, };
#endif

/*JPN*/
#elif defined(CONFIG_MACH_T0_JPN_LTE_DCM)
unsigned int Firmware_version_of_file = 0x310;
unsigned char *firmware_name = "epen/W9001_B746JD.bin";
char Firmware_checksum[] = { 0x1F, 0x81, 0x72, 0xDC, 0x5E, };

/*USA*/
#elif defined(CONFIG_MACH_T0_USA_VZW) \
	|| defined(CONFIG_MACH_T0_USA_SPR)
unsigned int Firmware_version_of_file = 0x602;
unsigned char *firmware_name = "epen/W9001_B746VZW.bin";

char Firmware_checksum[] = { 0x1F, 0x46, 0xB1, 0x68, 0x88, };

/*USA*/
#elif defined(CONFIG_MACH_T0_USA_USCC)
unsigned int Firmware_version_of_file = 0x1004;
unsigned char *firmware_name = "epen/W9001_B746USC.bin";

char Firmware_checksum[] = { 0x1F, 0x85, 0x70, 0x07, 0xF9, };

/*CHN*/
#elif defined(CONFIG_MACH_T0_CHN_CTC)
unsigned int Firmware_version_of_file = 0x700;
unsigned char *firmware_name = "epen/W9001_0700.bin";
char Firmware_checksum[] = { 0x1F, 0xD4, 0xD1, 0x5A, 0x91, };

/*EUR3G/EURLTE/ATT/TMO/CMCC*/
#else
unsigned int Firmware_version_of_file = 0x268;
unsigned char *firmware_name = "epen/W9001_B746.bin";

char Firmware_checksum[] = { 0x1F, 0x78, 0xB1, 0xAB, 0x78, };
#endif
/*checksum for 0x13D*/
const char B713X_checksum[] = { 0x1F, 0xB5, 0x84, 0x38, 0x34, };
/*checksum for 0x16*/
const char B660X_checksum[] = { 0x1F, 0x83, 0x88, 0xD4, 0x67, };
#elif defined(CONFIG_MACH_KONA)
const unsigned int Binary_nLength = 0xCD6A;
const unsigned char Mpu_type = 0x00;
unsigned int Firmware_version_of_file = 0x65D;
unsigned char *firmware_name = "epen/W9002_B781.bin";

char Firmware_checksum[] = { 0x1F, 0x72, 0xCD, 0x6E, 0xE3, };
#endif

void wacom_i2c_set_firm_data(unsigned char *Binary_new)
{
	if (Binary_new == NULL) {
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE)
		Binary = (unsigned char *)Binary_48;
		ums_binary = false;
#else
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
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE)
	Binary = (unsigned char *)Binary_48;

#elif defined(CONFIG_MACH_Q1_BD)
	Binary = NULL;
	/* Separate board revision */
	if (system_rev < 6) {
		firmware_name = "epen/W8501_P48.bin";
		Firmware_version_of_file = 0x20A;
		printk(KERN_DEBUG
		       "epen:Wacom driver is working for 4.8mm pitch pad.\n");
	} else
		printk(KERN_DEBUG
		       "epen:Wacom driver is working for 4.4mm pitch pad.\n");
#elif defined(CONFIG_MACH_T0)
	int type;
	int i;

	type = wacom_i2c_get_digitizer_type();

	if (type == EPEN_DTYPE_B746) {
		printk(KERN_DEBUG
			"epen:Digitizer type is B746\n");
	} else if (type == EPEN_DTYPE_B713) {
		printk(KERN_DEBUG
			"epen:Digitizer type is B713\n");
		firmware_name = "epen/W9001_B713.bin";
		Firmware_version_of_file = 0x13D;
		memcpy(Firmware_checksum, B713X_checksum,
			sizeof(Firmware_checksum));
	} else {
		printk(KERN_DEBUG
			"epen:Digitizer type is B660\n");
		firmware_name = "epen/W9001_B660.bin";
		Firmware_version_of_file = 0x16;
		memcpy(Firmware_checksum, B660X_checksum,
			sizeof(Firmware_checksum));
	}
	Binary = NULL;
#endif
}
