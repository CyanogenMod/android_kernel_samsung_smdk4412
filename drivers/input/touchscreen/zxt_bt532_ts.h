/*
 *
 * Zinitix bt532 touch driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ZXT_BT532_TS
#define _ZXT_BT532_TS

#define TS_DRVIER_VERSION	"1.0.18"
#define	MULTI_PROTOCOL_TYPE_B	1
#define USE_THREADED_IRQ	1
#define TOUCH_POINT_MODE	0
#define TOUCH_I2C_REGISTER_HERE	0

/* max 10 */
#define MAX_SUPPORTED_FINGER_NUM	5

/* max 8 */
#define MAX_SUPPORTED_BUTTON_NUM    6
#define SUPPORTED_BUTTON_NUM		6


/* Upgrade Method*/
#define TOUCH_ONESHOT_UPGRADE		1
/* if you use isp mode, you must add i2c device :
name = "zinitix_isp" , addr 0x50*/

/* resolution offset */
#define ABS_PT_OFFSET				(-1)

#define TOUCH_FORCE_UPGRADE			1
#define USE_CHECKSUM				0
#define CHECK_HWID					0

#define CHIP_OFF_DELAY				50 /*ms*/
#define CHIP_ON_DELAY				15 /*ms*/
#define FIRMWARE_ON_DELAY			20 /*ms*/

#define DELAY_FOR_SIGNAL_DELAY		30 /*us*/
#define DELAY_FOR_TRANSCATION		50
#define DELAY_FOR_POST_TRANSCATION	10

enum power_control {
	POWER_OFF,
	POWER_ON,
	POWER_ON_SEQUENCE,
};

/* Key Enum */
enum key_event {
	ICON_BUTTON_UNCHANGE,
	ICON_BUTTON_DOWN,
	ICON_BUTTON_UP,
};

/* ESD Protection */
/*second : if 0, no use. if you have to use, 3 is recommended*/
#define ESD_TIMER_INTERVAL			0
#define SCAN_RATE_HZ				100
#define CHECK_ESD_TIMER				3

 /*Test Mode (Monitoring Raw Data) */
#define SEC_DND_N_COUNT				10
#define SEC_DND_FREQUENCY			110 /* 300khz */

#define MAX_RAW_DATA_SZ				576 /* 32x18 */
#define MAX_TRAW_DATA_SZ	\
	(MAX_RAW_DATA_SZ + 4*MAX_SUPPORTED_FINGER_NUM + 2)
/* preriod raw data interval */

#define RAWDATA_DELAY_FOR_HOST		100

struct raw_ioctl {
	int sz;
	u8 *buf;
};

struct reg_ioctl {
	int addr;
	int *val;
};

#define TOUCH_SEC_MODE				48
#define TOUCH_REF_MODE				10
#define TOUCH_NORMAL_MODE			5
#define TOUCH_DELTA_MODE			3
#define TOUCH_DND_MODE				6

/*  Other Things */
#define INIT_RETRY_CNT				5
#define I2C_SUCCESS					0
#define I2C_FAIL					1

/*---------------------------------------------------------------------*/

/* Register Map*/
#define ZXT_SWRESET_CMD		0x0000
#define ZXT_WAKEUP_CMD		0x0001

#define ZXT_IDLE_CMD		0x0004
#define ZXT_SLEEP_CMD		0x0005

#define ZXT_CLEAR_INT_STATUS_CMD	0x0003
#define ZXT_CALIBRATE_CMD		0x0006
#define ZXT_SAVE_STATUS_CMD		0x0007
#define ZXT_SAVE_CALIBRATION_CMD	0x0008
#define ZXT_RECALL_FACTORY_CMD	0x000f

#define ZXT_SENSITIVITY	0x0020


#define ZXT_DEBUG_REG		0x0115	//0~7

#define ZXT_TOUCH_MODE		0x0010
#define ZXT_CHIP_REVISION		0x0011
#define ZXT_FIRMWARE_VERSION	0x0012

#define ZXT_MINOR_FW_VERSION	0x0121

#define ZXT_VENDOR_ID						0x001C
#define ZXT_DATA_VERSION_REG	0x0013
#define ZXT_HW_ID			0x0014
#define ZXT_SUPPORTED_FINGER_NUM	0x0015
#define ZXT_EEPROM_INFO		0x0018
#define ZXT_INITIAL_TOUCH_MODE		0x0019


#define ZXT_TOTAL_NUMBER_OF_X	0x0060
#define ZXT_TOTAL_NUMBER_OF_Y	0x0061

#define ZXT_DELAY_RAW_FOR_HOST	0x007f

#define ZXT_BUTTON_SUPPORTED_NUM	0x00B0
#define ZXT_BUTTON_SENSITIVITY	0x00B2


#define ZXT_X_RESOLUTION		0x00C0
#define ZXT_Y_RESOLUTION		0x00C1


#define ZXT_POINT_STATUS_REG	0x0080
#define ZXT_ICON_STATUS_REG		0x00AA

#define ZXT_AFE_FREQUENCY		0x0100
#define ZXT_DND_N_COUNT			0x0122

#define ZXT_RAWDATA_REG		0x0200

#define ZXT_EEPROM_INFO_REG		0x0018

#define ZXT_INT_ENABLE_FLAG		0x00f0
#define ZXT_PERIODICAL_INTERRUPT_INTERVAL	0x00f1

#define ZXT_BTN_WIDTH		0x016d

#define ZXT_CHECKSUM_RESULT	0x012c

#define ZXT_INIT_FLASH		0x01d0
#define ZXT_WRITE_FLASH		0x01d1
#define ZXT_READ_FLASH		0x01d2


/* Interrupt & status register flag bit
-------------------------------------------------
*/
#define BIT_PT_CNT_CHANGE	0
#define BIT_DOWN			1
#define BIT_MOVE			2
#define BIT_UP				3
#define BIT_PALM			4
#define BIT_PALM_REJECT		5
#define RESERVED_0			6
#define RESERVED_1			7
#define BIT_WEIGHT_CHANGE	8
#define BIT_PT_NO_CHANGE	9
#define BIT_REJECT			10
#define BIT_PT_EXIST		11
#define RESERVED_2			12
#define BIT_MUST_ZERO		13
#define BIT_DEBUG			14
#define BIT_ICON_EVENT		15

/* button */
#define BIT_O_ICON0_DOWN	0
#define BIT_O_ICON1_DOWN	1
#define BIT_O_ICON2_DOWN	2
#define BIT_O_ICON3_DOWN	3
#define BIT_O_ICON4_DOWN	4
#define BIT_O_ICON5_DOWN	5
#define BIT_O_ICON6_DOWN	6
#define BIT_O_ICON7_DOWN	7

#define BIT_O_ICON0_UP		8
#define BIT_O_ICON1_UP		9
#define BIT_O_ICON2_UP		10
#define BIT_O_ICON3_UP		11
#define BIT_O_ICON4_UP		12
#define BIT_O_ICON5_UP		13
#define BIT_O_ICON6_UP		14
#define BIT_O_ICON7_UP		15


#define SUB_BIT_EXIST		0
#define SUB_BIT_DOWN		1
#define SUB_BIT_MOVE		2
#define SUB_BIT_UP			3
#define SUB_BIT_UPDATE		4
#define SUB_BIT_WAIT		5


#define zinitix_bit_set(val, n)		((val) &= ~(1<<(n)), (val) |= (1<<(n)))
#define zinitix_bit_clr(val, n)		((val) &= ~(1<<(n)))
#define zinitix_bit_test(val, n)	((val) & (1<<(n)))
#define zinitix_swap_v(a, b, t)		((t) = (a), (a) = (b), (b) = (t))
#define zinitix_swap_16(s)			(((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))

#endif /*_ZXT_BT532_TS*/


