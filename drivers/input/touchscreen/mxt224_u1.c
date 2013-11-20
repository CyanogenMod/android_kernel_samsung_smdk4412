/*
*  Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
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
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt224_u1.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <mach/cpufreq.h>
#include <linux/input/mt.h>

#include "../keyboard/cypress/cypress-touchkey.h"

#define OBJECT_TABLE_START_ADDRESS	7
#define OBJECT_TABLE_ELEMENT_SIZE	6

#define CMD_RESET_OFFSET		0
#define CMD_BACKUP_OFFSET		1
#define CMD_CALIBRATE_OFFSET    2
#define CMD_REPORTATLL_OFFSET   3
#define CMD_DEBUG_CTRL_OFFSET   4
#define CMD_DIAGNOSTIC_OFFSET   5

#define DETECT_MSG_MASK			0x80
#define PRESS_MSG_MASK			0x40
#define RELEASE_MSG_MASK		0x20
#define MOVE_MSG_MASK			0x10
#define SUPPRESS_MSG_MASK		0x02

/* Version */
#define QT602240_VER_20			20
#define QT602240_VER_21			21
#define QT602240_VER_22			22

/* Slave addresses */
#define QT602240_APP_LOW		0x4a
#define QT602240_APP_HIGH		0x4b
#define QT602240_BOOT_LOW		0x24
#define QT602240_BOOT_HIGH		0x25

/*FIRMWARE NAME */
#define MXT224_ECHO_FW_NAME	    "mXT224e.fw"
#define MXT224_FW_NAME		    "qt602240.fw"

#define QT602240_FWRESET_TIME		175	/* msec */
#define QT602240_RESET_TIME		65	/* msec */

#define QT602240_BOOT_VALUE		0xa5
#define QT602240_BACKUP_VALUE		0x55

/* Bootloader mode status */
#define QT602240_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define QT602240_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define QT602240_FRAME_CRC_CHECK	0x02
#define QT602240_FRAME_CRC_FAIL		0x03
#define QT602240_FRAME_CRC_PASS		0x04
#define QT602240_APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define QT602240_BOOT_STATUS_MASK	0x3f

/* Command to unlock bootloader */
#define QT602240_UNLOCK_CMD_MSB		0xaa
#define QT602240_UNLOCK_CMD_LSB		0xdc

/* TSP state */
#define TSP_STATE_INACTIVE		-1
#define TSP_STATE_RELEASE		0
#define TSP_STATE_PRESS		1
#define TSP_STATE_MOVE		2

#define ID_BLOCK_SIZE			7

#define DRIVER_FILTER
#define U1_EUR_TARGET

#define MAX_USING_FINGER_NUM 10

#define MXT224_AUTOCAL_WAIT_TIME		2000

#if defined(U1_EUR_TARGET)
static bool gbfilter;
#endif

struct object_t {
	u8 object_type;
	u16 i2c_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;
} __packed;

struct finger_info {
	s16 x;
	s16 y;
	s16 z;
	u16 w;
	int16_t component;
};

struct mxt224_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct early_suspend early_suspend;
	u8 family_id;
	u32 finger_mask;
	int gpio_read_done;
	struct object_t *objects;
	u8 objects_len;
	u8 tsp_version;
	const u8 *power_cfg;
	const u8 *noise_suppression_cfg_ta;
	const u8 *noise_suppression_cfg;
	u8 finger_type;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;

	u8 atchcalst;
	u8 atchcalsthr;
	u8 tchthr_batt;
	u8 tchthr_charging;
	u8 tchthr_batt_init;
	u8 noisethr_batt;
	u8 noisethr_charging;
	u8 movfilter_batt;
	u8 movfilter_charging;
	u8 atchcalst_e;
	u8 atchcalsthr_e;
	u8 tchthr_batt_e;
	u8 tchthr_charging_e;
	u8 calcfg_batt_e;
	u8 calcfg_charging_e;
	u8 atchfrccalthr_e;
	u8 atchfrccalratio_e;
	u8 chrgtime_batt_e;
	u8 chrgtime_charging_e;
	u8 blen_batt_e;
	u8 blen_charging_e;
	u8 movfilter_batt_e;
	u8 movfilter_charging_e;
	u8 actvsyncsperx_e;
	u8 nexttchdi_e;

	void (*power_on) (void);
	void (*power_off) (void);
	void (*register_cb) (void *);
	void (*read_ta_status) (void *);
	int num_fingers;
	struct finger_info fingers[MXT224_MAX_MT_FINGERS];
	struct t22_freq_table_config_t freq_table;
	struct t48_median_config_t noise_median;

	int mxt224_enabled;
	bool g_debug_switch;
	u8 mxt_version_disp;
	u8 tsp_version_disp;
	int optiacl_gain;
	int firm_status_data;
	bool lock_status;
	int touch_state;		/* 1:release, 2:press, 3:others */
	int palm_chk_flag;
	bool ta_status_pre;
	int gErrCondition;
	int threshold;
	int threshold_e;
	bool boot_or_resume;	/*1: boot_or_resume,0: others */
	bool sleep_mode_flag;
	char *tsp_config_version;
	bool median_err_flag;
	int touch_is_pressed_arr[MAX_USING_FINGER_NUM];

#if defined(CONFIG_TARGET_LOCALE_NAATT) || defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	bool gain_change_flag;
	int gain_ta;
#endif
#ifdef CONFIG_TARGET_LOCALE_KOR
	unsigned char is_inputmethod;
#endif
	struct completion	init_done;
	struct mutex	lock;
	bool enabled;
};

#define CLEAR_MEDIAN_FILTER_ERROR
struct mxt224_data *copy_data;
int touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);

static void mxt224_optical_gain(uint16_t dbg_mode);

static int read_mem(struct mxt224_data *data, u16 reg, u8 len, u8 * buf)
{
	int ret;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
		 .addr = data->client->addr,
		 .flags = 0,
		 .len = 2,
		 .buf = (u8 *) &le_reg,
		 },
		{
		 .addr = data->client->addr,
		 .flags = I2C_M_RD,
		 .len = len,
		 .buf = buf,
		 },
	};

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0)
		return ret;

	return ret == 2 ? 0 : -EIO;
}

static int write_mem(struct mxt224_data *data, u16 reg, u8 len, const u8 * buf)
{
	int ret;
	u8 tmp[len + 2];

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);

	ret = i2c_master_send(data->client, tmp, sizeof(tmp));

	if (ret < 0)
		return ret;

	return ret == sizeof(tmp) ? 0 : -EIO;
}

static int __devinit mxt224_reset(struct mxt224_data *data)
{
	u8 buf = 1u;
	return write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}

static int __devinit mxt224_backup(struct mxt224_data *data)
{
	u8 buf = 0x55u;
	return write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
}

static int get_object_info(struct mxt224_data *data, u8 object_type, u16 *size,
			   u16 *address)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			*size = data->objects[i].size + 1;
			*address = data->objects[i].i2c_address;
			return 0;
		}
	}

	return -ENODEV;
}

static int write_config(struct mxt224_data *data, u8 type, const u8 * cfg)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);

	if (ret)
		return ret;

	return write_mem(data, address, size, cfg);
}

static u32 __devinit crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16) byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32) data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int __devinit calculate_infoblock_crc(struct mxt224_data *data,
					     u32 *crc_pointer)
{
	u32 crc = 0;
	u8 mem[7 + data->objects_len * 6];
	int status;
	int i;

	status = read_mem(data, 0, sizeof(mem), mem);

	if (status)
		return status;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

static unsigned int qt_time_point_freq;
static unsigned int qt_time_diff_freq;
static unsigned int qt_time_point;
static unsigned int qt_time_diff;
static unsigned int qt_timer_state;
static unsigned int good_check_flag;
static unsigned int not_yet_count;

static u8 cal_check_flag;

static u8 Doing_calibration_flag;

uint8_t calibrate_chip(void)
{
	u8 cal_data = 1;
	int ret = 0;
	u8 atchcalst_tmp, atchcalsthr_tmp;
	u16 obj_address = 0;
	u16 size = 1;
	int ret1 = 0;

	if (cal_check_flag == 0) {
		cal_check_flag = 1u;

		ret =
		    get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8, &size,
				    &obj_address);

		/* resume calibration must be performed with zero settings */
		atchcalst_tmp = 0;
		atchcalsthr_tmp = 0;

		/* atchcalst */
		ret = write_mem(copy_data, obj_address + 6, 1, &atchcalst_tmp);
		/*atchcalsthr */
		ret1 = write_mem(copy_data, obj_address + 7, 1,
				 &atchcalsthr_tmp);

		if (copy_data->family_id == 0x81) {	/* mxT224E */
			/* forced cal thr  */
			ret = write_mem(copy_data, obj_address + 8, 1,
					&atchcalst_tmp);
			/* forced cal thr ratio */
			ret1 = write_mem(copy_data, obj_address + 9, 1,
					 &atchcalsthr_tmp);
		}
	}

	/* send calibration command to the chip */
	if (!ret && !ret1 && !Doing_calibration_flag) {
		/* change calibration suspend settings to zero
		until calibration confirmed good */
		ret =
		    write_mem(copy_data,
			      copy_data->cmd_proc + CMD_CALIBRATE_OFFSET, 1,
			      &cal_data);

		/* set flag for calibration lockup recovery
		if cal command was successful */
		if (!ret) {
			/* set flag to show we must still confirm
			if calibration was good or bad */
			Doing_calibration_flag = 1;
			printk(KERN_ERR "[TSP] calibration success!!!\n");
		}

	}
	return ret;
}

static int check_abs_time(void)
{
	if (!qt_time_point)
		return 0;

	qt_time_diff = jiffies_to_msecs(jiffies) - qt_time_point;

	if (qt_time_diff > 0)
		return 1;
	else
		return 0;

}

static int check_abs_time_freq_err(void)
{
	if (!qt_time_point_freq)
		return 0;

	qt_time_diff_freq = jiffies_to_msecs(jiffies) - qt_time_point_freq;

	if (qt_time_diff_freq > 0)
		return 1;
	else
		return 0;

}

static void mxt224_ta_probe(bool ta_status)
{
	u16 obj_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	u8 val = 0;
	unsigned int register_address = 7;
	/*u8 calcfg; */
	u8 noise_threshold;
	u8 movfilter;
	u8 calcfg_dis;
	u8 calcfg_en;
	u8 charge_time;

	printk(KERN_ERR "[TSP] mxt224_ta_probe\n");
	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR "[TSP] copy_data->mxt224_enabled is 0\n");
		return;
	}

	if (ta_status) {
		copy_data->threshold = copy_data->tchthr_charging;
		calcfg_dis = copy_data->calcfg_charging_e;
		calcfg_en = copy_data->calcfg_charging_e | 0x20;
		noise_threshold = copy_data->noisethr_charging;
		charge_time = copy_data->chrgtime_charging_e;
#ifdef CLEAR_MEDIAN_FILTER_ERROR
		copy_data->gErrCondition = ERR_RTN_CONDITION_MAX;
		copy_data->noise_median.mferr_setting = false;
#endif
	} else {
		if (copy_data->boot_or_resume == 1) {
			copy_data->threshold = copy_data->tchthr_batt_init;
			calcfg_dis = copy_data->calcfg_batt_e;
		} else {
			copy_data->threshold = copy_data->tchthr_batt;
			calcfg_dis = copy_data->calcfg_batt_e ^ 0x20;
		}
		copy_data->threshold_e = copy_data->tchthr_batt_e;

		calcfg_en = copy_data->calcfg_batt_e | 0x20;
		noise_threshold = copy_data->noisethr_batt;
		charge_time = copy_data->chrgtime_batt_e;
#ifdef CLEAR_MEDIAN_FILTER_ERROR
		copy_data->gErrCondition = ERR_RTN_CONDITION_IDLE;
		copy_data->noise_median.mferr_count = 0;
		copy_data->noise_median.mferr_setting = false;
		copy_data->noise_median.median_on_flag = false;
#endif
	}

	if (copy_data->family_id == 0x81) {
#ifdef CLEAR_MEDIAN_FILTER_ERROR
		if (!ta_status) {
			ret =
			    get_object_info(copy_data,
					    TOUCH_MULTITOUCHSCREEN_T9,
					    &size_one, &obj_address);
			size_one = 1;
			/*blen */
			value = copy_data->blen_batt_e;
			register_address = 6;
			write_mem(copy_data,
				  obj_address + (u16) register_address,
				  size_one, &value);
			/*threshold */
			value = copy_data->threshold_e;
			register_address = 7;
			write_mem(copy_data,
				  obj_address + (u16) register_address,
				  size_one, &value);
			/*move Filter */
			value = copy_data->movfilter_batt_e;
			register_address = 13;
			write_mem(copy_data,
				  obj_address + (u16) register_address,
				  size_one, &value);
			/*nexttchdi*/
			value = copy_data->nexttchdi_e;
			register_address = 34;
			write_mem(copy_data,
				  obj_address + (u16) register_address,
				  size_one, &value);
		}
#endif

		value = copy_data->actvsyncsperx_e;
		ret =
		    get_object_info(copy_data, SPT_CTECONFIG_T46, &size_one,
				    &obj_address);
		write_mem(copy_data, obj_address + 3, 1, &value);

		ret =
		    get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8,
				    &size_one, &obj_address);
		size_one = 1;
		value = charge_time;
		write_mem(copy_data, obj_address, size_one, &value);

		ret =
		    get_object_info(copy_data, PROCG_NOISESUPPRESSION_T48,
				    &size_one, &obj_address);
#ifndef CONFIG_TARGET_LOCALE_NAATT_TEMP
		value = calcfg_dis;
		register_address = 2;
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
#if !defined(PRODUCT_SHIP)
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "[TSP]TA_probe MXT224E T%d Byte%d is %d\n", 48,
		       register_address, val);
#endif
#endif
		if (ta_status)
			write_config(copy_data, PROCG_NOISESUPPRESSION_T48,
				     copy_data->noise_suppression_cfg_ta);
		else
			write_config(copy_data, PROCG_NOISESUPPRESSION_T48,
				     copy_data->noise_suppression_cfg);
#if defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
		if ((ta_status) && (copy_data->gain_change_flag == 1)) {
			value = copy_data->blen_charging_e;
			write_mem(copy_data, obj_address + 34, size_one,
				  &value);
#if !defined(PRODUCT_SHIP)
			read_mem(copy_data, obj_address + 34, (u8) size_one,
				 &val);
			printk(KERN_DEBUG
			       "[TSP]TA_probe MXT224E T%d Byte%d is %d\n", 48,
			       34, val);
#endif

			value = 40;
			write_mem(copy_data, obj_address + 35, size_one,
				  &value);
#if !defined(PRODUCT_SHIP)
			read_mem(copy_data, obj_address + 35, (u8) size_one,
				 &val);
			printk(KERN_DEBUG
			       "[TSP]TA_probe MXT224E T%d Byte%d is %d\n", 48,
			       35, val);
#endif

			value = 240;
			write_mem(copy_data, obj_address + 43, size_one,
				  &value);

			value = 245;
			write_mem(copy_data, obj_address + 44, size_one,
				  &value);

			value = 148;
			write_mem(copy_data, obj_address + 47, size_one,
				  &value);

			value = 50;
			write_mem(copy_data, obj_address + 48, size_one,
				  &value);
		}
#endif
#ifndef CONFIG_TARGET_LOCALE_NAATT_TEMP
		value = calcfg_en;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
#if !defined(PRODUCT_SHIP)
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "[TSP]TA_probe MXT224E T%d Byte%d is %d\n", 48,
		       register_address, val);
#endif
#endif
	} else {
		if (copy_data->freq_table.fherr_setting >= 1) {
			ret = get_object_info(copy_data, GEN_POWERCONFIG_T7,
				&size_one, &obj_address);
			value = 48;
			write_mem(copy_data, obj_address, 1, &value);

			ret = get_object_info(copy_data,
				TOUCH_MULTITOUCHSCREEN_T9,
				&size_one, &obj_address);
			value = 32;
			write_mem(copy_data, obj_address + 6, 1, &value);

			ret = get_object_info(copy_data,
				PROCG_NOISESUPPRESSION_T22,
				&size_one, &obj_address);
			value = 143;
			write_mem(copy_data, obj_address, 1, &value);

			value = 0;
			write_mem(copy_data, obj_address + 10, 1, &value);

			write_mem(copy_data, obj_address + 11, 5,
				&copy_data->freq_table.freq_for_fherr1[0]);

			ret = get_object_info(copy_data, SPT_CTECONFIG_T28,
				&size_one, &obj_address);
			value = 19;
			write_mem(copy_data, obj_address + 4, 1, &value);

			copy_data->freq_table.fherr_cnt = 0;
			copy_data->freq_table.fherr_num = 1;
			copy_data->freq_table.fherr_setting = 0;
			copy_data->freq_table.t9_blen_for_fherr_cnt = 0;
		}
		ret =
		    get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9,
				    &size_one, &obj_address);
		size_one = 1;
		value = (u8) copy_data->threshold;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
#if !defined(PRODUCT_SHIP)
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "[TSP]TA_probe MXT224 T%d Byte%d is %d\n", 9,
		       register_address, val);
#endif

		value = noise_threshold;
		register_address = 8;
		ret =
		    get_object_info(copy_data, PROCG_NOISESUPPRESSION_T22,
				    &size_one, &obj_address);
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
	}
	copy_data->ta_status_pre = (bool) ta_status;
};

void check_chip_calibration(unsigned char one_touch_input_flag)
{
	u8 data_buffer[100] = { 0 };
	u8 try_ctr = 0;
	u8 data_byte = 0xF3;	/* dianostic command to get touch flags */
	u8 tch_ch = 0, atch_ch = 0;
	u8 check_mask;
	u8 i, j = 0;
	u8 x_line_limit;
	int ret;
	u16 size;
	u16 object_address = 0;
	bool ta_status_check;

	/* we have had the first touchscreen or face suppression message
	 * after a calibration - check the sensor state and try to confirm if
	 * cal was good or bad */

	/* get touch flags from the chip using the diagnostic object */
	/* write command to command processor to get touch flags
	- 0xF3 Command required to do this */
	write_mem(copy_data, copy_data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1,
		  &data_byte);

	/* get the address of the diagnostic object
	so we can get the data we need */
	/* diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0); */
	ret =
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size,
			    &object_address);

	usleep_range(10000, 10000);

	/* read touch flags from the diagnostic object
	- clear buffer so the while loop can run first time */
	memset(data_buffer, 0xFF, sizeof(data_buffer));

	/* wait for diagnostic object to update */
	while (!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))) {
		/* wait for data to be valid  */
		if (try_ctr > 10) {	/* 0318 hugh 100-> 10 */

			/* Failed! */
			printk(KERN_ERR
			       "[TSP] Diagnostic Data did not update!!\n");
			qt_timer_state = 0;	/* 0430 hugh */
			break;
		}

		usleep_range(2000, 2000);	/* 0318 hugh  3-> 2 */
		try_ctr++;	/* timeout counter */
		/* read_mem(diag_address, 2,data_buffer); */

		read_mem(copy_data, object_address, 2, data_buffer);
	}

	/* data is ready - read the detection flags */
	/* read_mem(diag_address, 82,data_buffer); */
	read_mem(copy_data, object_address, 82, data_buffer);

	/* data array is 20 x 16 bits for each set of flags, 2 byte header,
	   40 bytes for touch flags 40 bytes for antitouch flags */

	/* count up the channels/bits if we recived the data properly */
	if ((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)) {

		/* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4. */
		/* x_line_limit = 16 + cte_config.mode; */
		x_line_limit = 16 + 3;

		if (x_line_limit > 20) {
			/* hard limit at 20 so we don't over-index the array */
			x_line_limit = 20;
		}

		/* double the limit as the array is in bytes not words */
		x_line_limit = x_line_limit << 1;

		/* count the channels and print the flags to the log */
		/* check X lines
		- data is in words so increment 2 at a time */
		for (i = 0; i < x_line_limit; i += 2) {
			/* print the flags to the log
			- only really needed for debugging */

			/* count how many bits set for this row */
			for (j = 0; j < 8; j++) {
				/* create a bit mask to check against */
				check_mask = 1 << j;

				/* check detect flags */
				if (data_buffer[2 + i] & check_mask)
					tch_ch++;

				if (data_buffer[3 + i] & check_mask)
					tch_ch++;

				/* check anti-detect flags */
				if (data_buffer[42 + i] & check_mask)
					atch_ch++;

				if (data_buffer[43 + i] & check_mask)
					atch_ch++;

			}
		}

		pr_debug("[TSP] t: %d, a: %d\n", tch_ch, atch_ch);

		/* send page up command so we can detect
		when data updates next time, page byte will sit at 1
		until we next send F3 command */
		data_byte = 0x01;

		write_mem(copy_data,
			  copy_data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1,
			  &data_byte);

		if ((tch_ch + atch_ch) > 21) {
			printk(KERN_ERR "[TSP]touch ch + anti ch > 21\n");
			calibrate_chip();
			qt_timer_state = 0;
			qt_time_point = jiffies_to_msecs(jiffies);
			not_yet_count = 0;
		} else if (tch_ch > 17) {
			printk(KERN_ERR "[TSP]touch ch > 17\n");
			calibrate_chip();
			qt_timer_state = 0;
			qt_time_point = jiffies_to_msecs(jiffies);
			not_yet_count = 0;
		} else if ((tch_ch > 0) && (atch_ch == 0)) {
			/* cal was good - don't need to check any more */
			not_yet_count = 0;

			/* original:qt_time_diff = 501 */
			if (!check_abs_time())
				qt_time_diff = 301;

			if (qt_timer_state == 1) {
				/* originalqt_time_diff = 500 */
				if (qt_time_diff > 300) {
					printk(KERN_ERR
					       "[TSP] calibration was good\n");
					cal_check_flag = 0;
					good_check_flag = 0;
					qt_timer_state = 0;
					qt_time_point =
					    jiffies_to_msecs(jiffies);

					ret =
					    get_object_info(copy_data,
						    GEN_ACQUISITIONCONFIG_T8,
						    &size,
						    &object_address);

				/* change calibration suspend settings to zero
				until calibration confirmed good */
				/* store normal settings */
					size = 1;
					copy_data->palm_chk_flag = 2;

					if (copy_data->family_id == 0x80) {
						write_mem(copy_data,
						  object_address + 6, 1,
						  &copy_data->atchcalst);
						write_mem(copy_data,
						  object_address + 7, 1,
						  &copy_data->atchcalsthr);

					} else
					if (copy_data->family_id == 0x81) {
						write_mem(copy_data,
						  object_address + 6, 1,
						  &copy_data->atchcalst_e);
						write_mem(copy_data,
						  object_address + 7, 1,
						  &copy_data->atchcalsthr_e);
						write_mem(copy_data,
						  object_address + 8, 1,
						  &copy_data->atchfrccalthr_e);
						write_mem(copy_data,
						  object_address + 9, 1,
					  &copy_data->atchfrccalratio_e);
					}

					if ((copy_data->read_ta_status) &&
					(copy_data->boot_or_resume == 1)) {
						copy_data->boot_or_resume = 0;
						copy_data->read_ta_status
						    (&ta_status_check);
						printk(KERN_ERR
						       "[TSP] ta_status is %d",
						       ta_status_check);

						if ((ta_status_check == 0)
					      && (copy_data->family_id == 0x80)
					      && (copy_data->\
					      freq_table.fherr_setting == 0))
							mxt224_ta_probe
							    (ta_status_check);
					}
				} else {
					cal_check_flag = 1;
				}
			} else {
				qt_timer_state = 1;
				qt_time_point = jiffies_to_msecs(jiffies);
				cal_check_flag = 1;
			}

		} else if (atch_ch >= 5) {
			printk(KERN_ERR "[TSP] calibration was bad\n");
			calibrate_chip();
			qt_timer_state = 0;
			not_yet_count = 0;
			qt_time_point = jiffies_to_msecs(jiffies);
		} else {
			/* we cannot confirm if good or bad
			- we must wait for next touch  message to confirm */
			printk(KERN_ERR
			       "[TSP] calibration was not decided yet\n");
			cal_check_flag = 1u;
			/* Reset the 100ms timer */
			qt_timer_state = 0;
			qt_time_point = jiffies_to_msecs(jiffies);

			not_yet_count++;
			if (not_yet_count > 10) {
				not_yet_count = 0;
				calibrate_chip();
			}
		}
	}
}

#if defined(DRIVER_FILTER)
#if defined(U1_EUR_TARGET)
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MAX_USING_FINGER_NUM] = { 0, };
	static u16 pre_x[MAX_USING_FINGER_NUM][4] = { {0}, };
	static u16 pre_y[MAX_USING_FINGER_NUM][4] = { {0}, };
	int coff[4] = { 0, };
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	pre_x[id][tcount[id] % 4] = *px;
	pre_y[id][tcount[id] % 4] = *py;

	if (gbfilter) {
		if (tcount[id] > 3) {
			*px =
			    (u16) ((*px + pre_x[id][(tcount[id] - 1) % 4] +
				    pre_x[id][(tcount[id] - 2) % 4] +
				    pre_x[id][(tcount[id] - 3) % 4]) / 4);
			*py =
			    (u16) ((*py + pre_y[id][(tcount[id] - 1) % 4] +
				    pre_y[id][(tcount[id] - 2) % 4] +
				    pre_y[id][(tcount[id] - 3) % 4]) / 4);
		} else {
			switch (tcount[id]) {
			case 2:
				{
					*px =
					    (u16) ((*px +
						    pre_x[id][(tcount[id] -
							       1) % 4]) >> 1);
					*py =
					    (u16) ((*py +
						    pre_y[id][(tcount[id] -
							       1) % 4]) >> 1);
					break;
				}

			case 3:
				{
					*px =
					    (u16) ((*px +
						    pre_x[id][(tcount[id] -
							       1) % 4] +
						    pre_x[id][(tcount[id] -
							       2) % 4]) / 3);
					*py =
					    (u16) ((*py +
						    pre_y[id][(tcount[id] -
							       1) % 4] +
						    pre_y[id][(tcount[id] -
							       2) % 4]) / 3);
					break;
				}

			default:
				break;
			}
		}

	} else if (tcount[id] > 3) {
		{
			distance =
			    abs(pre_x[id][(tcount[id] - 1) % 4] - *px) +
			    abs(pre_y[id][(tcount[id] - 1) % 4] - *py);

			coff[0] = (u8) (2 + distance / 5);
			if (coff[0] < 8) {
				coff[0] = max(2, coff[0]);
				coff[1] =
				    min((8 - coff[0]), (coff[0] >> 1) + 1);
				coff[2] =
				    min((8 - coff[0] - coff[1]),
					(coff[1] >> 1) + 1);
				coff[3] = 8 - coff[0] - coff[1] - coff[2];

				*px =
				    (u16) ((*px * (coff[0]) +
					    pre_x[id][(tcount[id] -
						       1) % 4] * (coff[1])
					    +
					    pre_x[id][(tcount[id] -
						       2) % 4] * (coff[2]) +
					    pre_x[id][(tcount[id] -
						       3) % 4] * (coff[3])) /
					   8);
				*py =
				    (u16) ((*py * (coff[0]) +
					    pre_y[id][(tcount[id] -
						       1) % 4] * (coff[1])
					    +
					    pre_y[id][(tcount[id] -
						       2) % 4] * (coff[2]) +
					    pre_y[id][(tcount[id] -
						       3) % 4] * (coff[3])) /
					   8);
			} else {
				*px =
				    (u16) ((*px * 4 +
					    pre_x[id][(tcount[id] -
						       1) % 4]) / 5);
				*py =
				    (u16) ((*py * 4 +
					    pre_y[id][(tcount[id] -
						       1) % 4]) / 5);
			}
		}
	}
	tcount[id]++;
}

#else				/* CONFIG_TARGET_LOCALE_KOR */
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MAX_USING_FINGER_NUM] = { 0, };
	static u16 pre_x[MAX_USING_FINGER_NUM][4] = { {0}, };
	static u16 pre_y[MAX_USING_FINGER_NUM][4] = { {0}, };
	int coff[4] = { 0, };
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	pre_x[id][tcount[id] % 4] = *px;
	pre_y[id][tcount[id] % 4] = *py;

	if (tcount[id] > 3) {
		distance =
		    abs(pre_x[id][(tcount[id] - 1) % 4] - *px) +
		    abs(pre_y[id][(tcount[id] - 1) % 4] - *py);

		coff[0] = (u8) (4 + distance / 5);
		if (coff[0] < 8) {
			coff[0] = max(4, coff[0]);
			coff[1] = min((10 - coff[0]), (coff[0] >> 1) + 1);
			coff[2] =
			    min((10 - coff[0] - coff[1]), (coff[1] >> 1) + 1);
			coff[3] = 10 - coff[0] - coff[1] - coff[2];

			*px =
			    (u16) ((*px * (coff[0]) +
				    pre_x[id][(tcount[id] - 1) % 4] * (coff[1])
				    +
				    pre_x[id][(tcount[id] -
					       2) % 4] * (coff[2]) +
				    pre_x[id][(tcount[id] -
					       3) % 4] * (coff[3])) / 10);
			*py =
			    (u16) ((*py * (coff[0]) +
				    pre_y[id][(tcount[id] - 1) % 4] * (coff[1])
				    +
				    pre_y[id][(tcount[id] -
					       2) % 4] * (coff[2]) +
				    pre_y[id][(tcount[id] -
					       3) % 4] * (coff[3])) / 10);
		} else {
			*px =
			    (u16) ((*px * 4 +
				    pre_x[id][(tcount[id] - 1) % 4]) / 5);
			*py =
			    (u16) ((*py * 4 +
				    pre_y[id][(tcount[id] - 1) % 4]) / 5);
		}
	}
#if 0
	else {
		switch (tcount[id]) {
		case 2:
			{
				*px =
				    (u16) ((*px +
					    pre_x[id][(tcount[id] -
						       1) % 4]) >> 1);
				*py =
				    (u16) ((*py +
					    pre_y[id][(tcount[id] -
						       1) % 4]) >> 1);
				break;
			}

		case 3:
			{
				*px =
				    (u16) ((*px +
					    pre_x[id][(tcount[id] - 1) % 4] +
					    pre_x[id][(tcount[id] -
						       2) % 4]) / 3);
				*py =
				    (u16) ((*py +
					    pre_y[id][(tcount[id] - 1) % 4] +
					    pre_y[id][(tcount[id] -
						       2) % 4]) / 3);
				break;
			}

		default:
			break;
		}
	}
#endif

	tcount[id]++;
}
#endif
#endif				/* DRIVER_FILTER */

static int __devinit mxt224_init_touch_driver(struct mxt224_data *data)
{
	struct object_t *object_table;
	u32 read_crc = 0;
	u32 calc_crc;
	u16 crc_address;
	u16 dummy;
	int i;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;

	ret = read_mem(data, 0, sizeof(id), id);
	if (ret)
		return ret;

	dev_info(&data->client->dev, "family = %#02x, variant = %#02x, version"
		 "= %#02x, build = %d\n", id[0], id[1], id[2], id[3]);
	printk(KERN_ERR "[TSP] family = %#02x, variant = %#02x, version "
	       "= %#02x, build = %d\n", id[0], id[1], id[2], id[3]);
	dev_dbg(&data->client->dev, "matrix X size = %d\n", id[4]);
	dev_dbg(&data->client->dev, "matrix Y size = %d\n", id[5]);

	data->family_id = id[0];
	data->tsp_version = id[2];
	data->objects_len = id[6];

	data->mxt_version_disp = data->family_id;
	data->tsp_version_disp = data->tsp_version;

	object_table = kmalloc(data->objects_len * sizeof(*object_table),
			       GFP_KERNEL);
	if (!object_table)
		return -ENOMEM;

	ret = read_mem(data, OBJECT_TABLE_START_ADDRESS,
		       data->objects_len * sizeof(*object_table),
		       (u8 *) object_table);
	if (ret)
		goto err;

	for (i = 0; i < data->objects_len; i++) {
		object_table[i].i2c_address =
		    le16_to_cpu(object_table[i].i2c_address);
		tmp = 0;
		if (object_table[i].num_report_ids) {
			tmp = type_count + 1;
			type_count += object_table[i].num_report_ids *
			    (object_table[i].instances + 1);
		}
		switch (object_table[i].object_type) {
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_type = tmp;
			dev_dbg(&data->client->dev, "Finger type = %d\n",
				data->finger_type);
			break;
		case GEN_MESSAGEPROCESSOR_T5:
			data->msg_object_size = object_table[i].size + 1;
			dev_dbg(&data->client->dev, "Message object size = "
				"%d\n", data->msg_object_size);
			break;
		}
	}

	data->objects = object_table;

	/* Verify CRC */
	crc_address = OBJECT_TABLE_START_ADDRESS +
	    data->objects_len * OBJECT_TABLE_ELEMENT_SIZE;

#ifdef __BIG_ENDIAN
#error The following code will likely break on a big endian machine
#endif
	ret = read_mem(data, crc_address, 3, (u8 *) &read_crc);
	if (ret)
		goto err;

	read_crc = le32_to_cpu(read_crc);

	ret = calculate_infoblock_crc(data, &calc_crc);
	if (ret)
		goto err;

	if (read_crc != calc_crc) {
		dev_err(&data->client->dev, "CRC error\n");
		ret = -EFAULT;
		goto err;
	}

	ret = get_object_info(data, GEN_MESSAGEPROCESSOR_T5, &dummy,
			      &data->msg_proc);
	if (ret)
		goto err;

	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &dummy,
			      &data->cmd_proc);
	if (ret)
		goto err;

	return 0;

 err:
	kfree(object_table);
	return ret;
}

static void report_input_data(struct mxt224_data *data)
{
	int i;
	static unsigned int level = ~0;
	bool tsp_state = false;
	bool check_press = false;
	u16 object_address = 0;
	u16 size = 1;
	u8 value;

	touch_is_pressed = 0;

	if (level == ~0)
		exynos_cpufreq_get_level(500000, &level);

	for (i = 0; i < data->num_fingers; i++) {
		if (TSP_STATE_INACTIVE == data->fingers[i].z)
			continue;

		/* for release */
		if (data->fingers[i].z == TSP_STATE_RELEASE) {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
				MT_TOOL_FINGER, false);
			data->fingers[i].z = TSP_STATE_INACTIVE;
		/* logging */
#ifdef __TSP_DEBUG
			printk(KERN_ERR "[TSP] Up[%d] %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);
#else
			pr_debug("[TSP] Up[%d]\n", i);
#endif

			continue;
		}

		input_mt_slot(data->input_dev, i);
		input_mt_report_slot_state(data->input_dev,
			MT_TOOL_FINGER, true);

		input_report_abs(data->input_dev, ABS_MT_POSITION_X,
				 data->fingers[i].x);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
				 data->fingers[i].y);
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
				 data->fingers[i].z);
		input_report_abs(data->input_dev, ABS_MT_PRESSURE,
				 data->fingers[i].w);

#ifdef _SUPPORT_SHAPE_TOUCH_
		input_report_abs(data->input_dev, ABS_MT_COMPONENT,
				 data->fingers[i].component);
#endif

		if (copy_data->touch_is_pressed_arr[i] == 1)
			check_press = true;

		if (copy_data->g_debug_switch)
			printk(KERN_ERR "[TSP] ID-%d, %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);

		if (copy_data->touch_is_pressed_arr[i] != 0)
			touch_is_pressed = 1;

		/* logging */
#ifdef __TSP_DEBUG
		if (copy_data->touch_is_pressed_arr[i] == 1)
			printk(KERN_ERR "[TSP] Dn[%d] %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);
		if (copy_data->touch_is_pressed_arr[i] == 2)
			printk(KERN_ERR "[TSP] Mv[%d] %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);
#else
		if (copy_data->touch_is_pressed_arr[i] == 1) {
			pr_debug("[TSP] Dn[%d]\n", i);
			copy_data->touch_is_pressed_arr[i] = 2;
		}
#endif
	}
	data->finger_mask = 0;
	copy_data->touch_state = 0;
	input_sync(data->input_dev);

	if ((touch_is_pressed == 0) &&
		(copy_data->freq_table.fherr_setting >= 2)) {
		if (!check_abs_time_freq_err())
			qt_time_diff_freq = 5001;

		if (qt_time_diff_freq > 5000) {
			get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8,
				&size, &object_address);
			value = 0;
			/* TCHAUTOCAL disable */
			write_mem(copy_data, object_address + 4, 1, &value);
			copy_data->freq_table.fherr_setting = 1;
			printk(KERN_ERR"[TSP] auto cal disable\n");
			get_object_info(copy_data, PROCG_NOISESUPPRESSION_T22,
				&size, &object_address);
			value = 60;
			write_mem(copy_data, object_address + 8, 1, &value);
		}
		if (copy_data->freq_table.fherr_setting == 2) {
			write_mem(copy_data,
				copy_data->cmd_proc + CMD_CALIBRATE_OFFSET,
				1, &copy_data->freq_table.fherr_num);
			copy_data->freq_table.fherr_setting = 3;
		}
	}

	for (i = 0; i < data->num_fingers; i++) {
		if (TSP_STATE_INACTIVE != data->fingers[i].z) {
			tsp_state = true;
			break;
		}
	}

	if (!tsp_state && copy_data->lock_status) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		copy_data->lock_status = 0;
	} else if ((copy_data->lock_status == 0) && check_press) {
		if (level != ~0) {
			exynos_cpufreq_lock(
				DVFS_LOCK_ID_TSP,
				level);
			copy_data->lock_status = 1;
		}
	}

    /* tell cypress keypad we had finger activity */
    touchscreen_state_report(touch_is_pressed);

}

void palm_recovery(void)
{
	int ret = 0;
	u8 atchcalst_tmp = 0, atchcalsthr_tmp = 0;
	u16 obj_address = 0;
	u16 size = 1;
	int ret1 = 0;

	if (copy_data->palm_chk_flag == 2) {
		copy_data->palm_chk_flag = 0;
		ret =
		    get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8, &size,
				    &obj_address);
		size = 1;

		/* TCHAUTOCAL Disable */
		ret = write_mem(copy_data, obj_address + 4, 1,
		&atchcalst_tmp);	/* TCHAUTOCAL */
		printk(KERN_DEBUG "[TSP] auto calibration disable!!!\n");

	} else {
		if (cal_check_flag == 0) {
			ret =
			    get_object_info(copy_data,
				GEN_ACQUISITIONCONFIG_T8,
					    &size, &obj_address);

			/* resume calibration must be
			performed with zero settings */
			atchcalst_tmp = 0;
			atchcalsthr_tmp = 0;

			ret = write_mem(copy_data, obj_address + 4, 1,
				&atchcalst_tmp);	/* TCHAUTOCAL */

			ret =
			    write_mem(copy_data, obj_address + 6, 1,
				      &atchcalst_tmp);
			ret1 =
			    write_mem(copy_data, obj_address + 7, 1,
				      &atchcalsthr_tmp);

			if (copy_data->family_id == 0x81) {	/* mxT224E */
				ret = write_mem(copy_data,
				obj_address + 8, 1,
				&atchcalst_tmp);	/* forced cal thr  */
				ret1 = write_mem(copy_data,
				obj_address + 9, 1,
				&atchcalsthr_tmp);	/* forced cal ratio */
			}
		}
	}
}

static int freq_hop_err_setting(int state)
{
	uint16_t object_address = 0;
	u8 value, ret;
	u16 size_one = 1;

	printk(KERN_DEBUG"[TSP] freq_hop_err_setting\n");
	copy_data->freq_table.fherr_num = 30;
	ret =
		get_object_info(copy_data, GEN_POWERCONFIG_T7,
		&size_one, &object_address);
	value = 255;
	write_mem(copy_data, object_address, 1, &value);

	cal_check_flag = 0;
	good_check_flag = 0;
	qt_timer_state = 0;

	ret =
		get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8,
		&size_one, &object_address);
	value = 5;
	/* TCHAUTOCAL 1sec */
	write_mem(copy_data, object_address + 4, 1, &value);

	qt_time_point_freq = jiffies_to_msecs(jiffies);
	copy_data->freq_table.fherr_setting = 2;
	write_mem(copy_data, object_address + 6, 1, &copy_data->atchcalst);
	write_mem(copy_data, object_address + 7, 1, &copy_data->atchcalsthr);

	ret =
		get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9,
		&size_one, &object_address);
	value = copy_data->freq_table.t9_blen_for_fherr;
	write_mem(copy_data, object_address + 6, 1, &value);

	value = copy_data->freq_table.t9_thr_for_fherr;
	write_mem(copy_data, object_address + 7, 1, &value);

	value = copy_data->freq_table.t9_movfilter_for_fherr;
	write_mem(copy_data, object_address + 13, 1, &value);

	ret =
		get_object_info(copy_data, PROCG_NOISESUPPRESSION_T22,
		&size_one, &object_address);
	value = 135;
	write_mem(copy_data, object_address + 0, 1, &value);

	value = copy_data->freq_table.t22_noisethr_for_fherr;
	write_mem(copy_data, object_address + 8, 1, &value);

	value = copy_data->freq_table.t22_freqscale_for_fherr;
	write_mem(copy_data, object_address + 10, 1, &value);

	if (state == 1) {
		write_mem(copy_data, object_address + 11, 5,
			&copy_data->freq_table.freq_for_fherr1[0]);
	} else if (state == 2) {
		write_mem(copy_data, object_address + 11, 5,
			&copy_data->freq_table.freq_for_fherr2[0]);
		copy_data->freq_table.fherr_num = 1;
		copy_data->freq_table.fherr_cnt = 2;
	} else if (state == 3) {
		write_mem(copy_data, object_address + 11, 5,
			&copy_data->freq_table.freq_for_fherr3[0]);
		copy_data->freq_table.fherr_num = 1;
	} else if (state == 4) {
		write_mem(copy_data, object_address + 11, 5,
			&copy_data->freq_table.freq_for_fherr4[0]);
		copy_data->freq_table.fherr_num = 1;
		copy_data->freq_table.fherr_cnt = 0;
	}

	ret =
		get_object_info(copy_data, SPT_CTECONFIG_T28,
		&size_one, &object_address);
	value = 48;
	write_mem(copy_data, object_address + 4, 1, &value);

	return 0;
}

#ifdef CLEAR_MEDIAN_FILTER_ERROR
static int Check_Err_Condition(void)
{
	int rtn = ERR_RTN_CONDITION_IDLE;

	switch (copy_data->gErrCondition) {
	case ERR_RTN_CONDITION_IDLE:
	default:
			rtn = ERR_RTN_CONDITION_T9;
			break;
	}
	return rtn;
}

static void median_err_setting(void)
{
	u16 obj_address;
	u16 size_one;
	u8 value, state;
	bool ta_status_check;
	int ret = 0;

	if (cal_check_flag == 1) {
		printk(KERN_ERR"[TSP] calibration was forcely good\n");
		cal_check_flag = 0;
		good_check_flag = 0;
		qt_timer_state = 0;
		qt_time_point = jiffies_to_msecs(jiffies);
		copy_data->palm_chk_flag = 2;

		ret = get_object_info(copy_data,
			GEN_ACQUISITIONCONFIG_T8,
			&size_one, &obj_address);
		write_mem(copy_data,
		  obj_address + 6, 1,
		  &copy_data->atchcalst_e);
		write_mem(copy_data,
		  obj_address + 7, 1,
		  &copy_data->atchcalsthr_e);
		write_mem(copy_data,
		  obj_address + 8, 1,
		  &copy_data->atchfrccalthr_e);
		write_mem(copy_data,
		  obj_address + 9, 1,
		  &copy_data->atchfrccalratio_e);
	}

	copy_data->read_ta_status(&ta_status_check);
	if (!ta_status_check) {
		copy_data->gErrCondition = Check_Err_Condition();
		switch (copy_data->gErrCondition) {
		case ERR_RTN_CONDITION_T9:
			{
#ifdef CONFIG_TARGET_LOCALE_NAATT_TEMP
				ret =
				get_object_info(copy_data,
				PROCG_NOISESUPPRESSION_T48,
				&size_one, &obj_address);
				value = 100;
				write_mem(copy_data, obj_address+22, 1,
				&value);
				value = 100;
				write_mem(copy_data, obj_address+25, 1,
				&value);
				value = 81;
				write_mem(copy_data, obj_address+39, 1,
				&value);
#else
#if 0
				ret =
				    get_object_info(copy_data,
						    TOUCH_MULTITOUCHSCREEN_T9,
						    &size_one, &obj_address);
				value = 16;
				write_mem(copy_data, obj_address + 6, 1,
					  &value);
				value = 40;
				write_mem(copy_data, obj_address + 7, 1,
					  &value);
				value = 80;
				write_mem(copy_data, obj_address + 13, 1,
					  &value);
#endif
				ret |=
				    get_object_info(copy_data,
						    SPT_CTECONFIG_T46,
						    &size_one, &obj_address);
				value = 48;  /* 32;*/
				write_mem(copy_data, obj_address + 3, 1,
					  &value);
				ret |=
				    get_object_info(copy_data,
						    PROCG_NOISESUPPRESSION_T48,
						    &size_one, &obj_address);
				value = 20; /*29;*/
				write_mem(copy_data, obj_address + 3, 1,
					  &value);
				value = 1;
				write_mem(copy_data, obj_address + 8, 1,
					  &value);
				value = 2;
				write_mem(copy_data, obj_address + 9, 1,
					  &value);
				value = 64; /*100;*/
				write_mem(copy_data, obj_address + 17, 1,
					  &value);
				value = 64;
				write_mem(copy_data, obj_address + 19, 1,
					  &value);
				value = 100; /*20;*/
				write_mem(copy_data, obj_address + 22, 1,
					  &value);
				value = 100; /*38;*/
				write_mem(copy_data, obj_address + 25, 1,
					  &value);
#if 0
				value = 16;
				write_mem(copy_data, obj_address + 34, 1,
					  &value);
#endif
				value = 40;
				write_mem(copy_data, obj_address + 35, 1,
					  &value);
				value = 81; /*80;*/
				write_mem(copy_data, obj_address + 39, 1,
					  &value);
#endif
			}
			break;

		default:
			break;
		}
	} else {
		value = 1;
		if (copy_data->noise_median.mferr_count < 3)
			copy_data->noise_median.mferr_count++;

		if (!(copy_data->noise_median.mferr_count % value)
		    && (copy_data->noise_median.mferr_count < 3)) {
			printk(KERN_DEBUG
			       "[TSP] median thr noise level too high. %d\n",
			       copy_data->noise_median.mferr_count / value);
			state = copy_data->noise_median.mferr_count / value;

			ret |=
			    get_object_info(copy_data,
					    PROCG_NOISESUPPRESSION_T48,
					    &size_one, &obj_address);
			if (state == 1) {
				value =
			copy_data->noise_median.t48_mfinvlddiffthr_for_mferr;
				write_mem(copy_data, obj_address + 22, 1,
					  &value);
				value =
			copy_data->noise_median.t48_mferrorthr_for_mferr;
				write_mem(copy_data, obj_address + 25, 1,
					  &value);
				value =
				copy_data->noise_median.t48_thr_for_mferr;
				write_mem(copy_data, obj_address + 35, 1,
					  &value);
				value =
			copy_data->noise_median.t48_movfilter_for_mferr;
				write_mem(copy_data, obj_address + 39, 1,
					  &value);
				ret |=
				    get_object_info(copy_data,
						    SPT_CTECONFIG_T46,
						    &size_one, &obj_address);
				value =
			copy_data->noise_median.t46_actvsyncsperx_for_mferr;
				write_mem(copy_data, obj_address + 3, 1,
					  &value);
	} else if (state >= 2) {
		value = 10;
				write_mem(copy_data, obj_address + 3, 1,
					  &value);
				value = 0;	/* secondmf */
				write_mem(copy_data, obj_address + 8, 1,
					  &value);
				value = 0;	/* thirdmf */
				write_mem(copy_data, obj_address + 9, 1,
					  &value);
				value = 20;	/* mfinvlddiffthr */
				write_mem(copy_data, obj_address + 22, 1,
					  &value);
				value = 38;	/* mferrorthr */
				write_mem(copy_data, obj_address + 25, 1,
					  &value);
				value = 45;	/* thr */
				write_mem(copy_data, obj_address + 35, 1,
					  &value);
				value = 65;	/* movfilter */
				write_mem(copy_data, obj_address + 39, 1,
					  &value);
				ret |=
				    get_object_info(copy_data,
						    SPT_CTECONFIG_T46,
						    &size_one, &obj_address);
				value = 53;	/* actvsyncsperx */
				write_mem(copy_data, obj_address + 3, 1,
					  &value);
			}
		}
	}
	copy_data->noise_median.mferr_setting = true;
}
#endif

static irqreturn_t mxt224_irq_thread(int irq, void *ptr)
{
	struct mxt224_data *data = ptr;
	int id;
	u8 msg[data->msg_object_size];
	u8 touch_message_flag = 0;
	u8 value, ret;
	u16 size_one;
	u16 obj_address = 0;
	int ta_status_check;

	if ((copy_data->palm_chk_flag == 2) && (copy_data->family_id == 0x80))
		palm_recovery();

	do {
		touch_message_flag = 0;

		if (read_mem(data, data->msg_proc, sizeof(msg), msg))
			return IRQ_HANDLED;

		if ((msg[0] == 0x1) &&
			((msg[1] & 0x10) == 0x10)) {	/* caliration */
			printk(KERN_ERR "[TSP] Calibration!!!!!!");
			Doing_calibration_flag = 1;
		} else if ((msg[0] == 0x1) &&
			((msg[1] & 0x40) == 0x40)) { /* overflow */
			printk(KERN_ERR "[TSP] Overflow!!!!!!");
		} else if ((msg[0] == 0x1) &&
			((msg[1] & 0x10) == 0x00)) {	/* caliration */
			printk(KERN_ERR "[TSP] Calibration End!!!!!!");

			Doing_calibration_flag = 0;
			if (cal_check_flag == 1) {
				qt_timer_state = 0;
				qt_time_point = jiffies_to_msecs(jiffies);
			}

			if ((cal_check_flag == 0)
			    && (copy_data->family_id == 0x80)
			    && (copy_data->freq_table.fherr_setting == 0)) {
				palm_recovery();
				cal_check_flag = 1u;
			}
		}

		if ((msg[0] == 14) && (copy_data->family_id == 0x80)) {
			if ((msg[1] & 0x01) == 0x00) {/* Palm release */
				printk(KERN_ERR "[TSP] Palm release");
				touch_is_pressed = 0;
			} else if ((msg[1] & 0x01) == 0x01) {/* Palm Press */
				printk(KERN_ERR "[TSP] Palm Press");
				touch_is_pressed = 1;
				touch_message_flag = 1;
			} else {
				/* printk(KERN_ERR
				"[TSP] palm error msg[1] is %d!!!\n",
				msg[1]); */
			}
		}

		if ((msg[0] == 0xf) && (copy_data->family_id == 0x80)) {
			if ((msg[1]&0x08) == 0x08) {
				copy_data->freq_table.fherr_cnt++;
				if (copy_data->freq_table.fherr_cnt >
					(copy_data->freq_table.fherr_num * 4))
					copy_data->freq_table.fherr_cnt = 1;

				if (!(copy_data->freq_table.fherr_cnt%
					copy_data->freq_table.fherr_num)) {
					printk(KERN_DEBUG"[TSP] freq changed."
					   "noise level too high.(%d)\n",
					   copy_data->freq_table.fherr_cnt/
					   copy_data->freq_table.fherr_num);
					freq_hop_err_setting(
					   copy_data->freq_table.fherr_cnt/
					   copy_data->freq_table.fherr_num);
				}
			}
		}
#ifdef CLEAR_MEDIAN_FILTER_ERROR
		if ((msg[0] == 18) && (data->family_id == 0x81)) {
			if ((msg[4] & 0x5) == 0x5) {
				printk(KERN_ERR
				       "[TSP] median filter state error!!!\n");
				median_err_setting();
			} else if ((msg[4] & 0x4) == 0x4) {
				copy_data->read_ta_status(&ta_status_check);
				if ((!ta_status_check)
				    && (copy_data->noise_median.mferr_setting
				    == false)
				    && (copy_data->noise_median.median_on_flag
				    == false)) {
					printk(KERN_ERR
					       "[TSP] median filter ON!!!\n");
					ret =
					    get_object_info(copy_data,
						    TOUCH_MULTITOUCHSCREEN_T9,
							    &size_one,
							    &obj_address);
					value = 0;
					write_mem(copy_data, obj_address + 34,
						  1, &value);
					copy_data->noise_median.median_on_flag
						= true;
				}
			}
		}
#endif
		if (msg[0] > 1 && msg[0] < 12) {

			if ((copy_data->touch_is_pressed_arr[msg[0] - 2] == 1)
			    && (msg[1] & 0x40)) {
				printk(KERN_ERR
				       "[TSP] Calibrate on Ghost touch");
				calibrate_chip();
			copy_data->touch_is_pressed_arr[msg[0] - 2] = 0;
			}

			if ((msg[1] & 0x20) == 0x20) {	/* Release */
			/* touch_is_pressed = 0; */
			/* copy_data->touch_is_pressed_arr[msg[0]-2] = 0; */

			} else if ((msg[1] & 0x90) == 0x90) {/*Detect & Move*/
				touch_message_flag = 1;
			} else if ((msg[1] & 0xC0) == 0xC0) {/*Detect & Press*/
				touch_message_flag = 1;
			}

			id = msg[0] - data->finger_type;

			if (msg[1] & RELEASE_MSG_MASK) {
				data->fingers[id].z = TSP_STATE_RELEASE;
				data->fingers[id].w = msg[5];
				data->finger_mask |= 1U << id;
			copy_data->touch_is_pressed_arr[msg[0] - 2] = 0;
				copy_data->touch_state = 1;
			} else if ((msg[1] & DETECT_MSG_MASK)
				   && (msg[1] &
				       (PRESS_MSG_MASK | MOVE_MSG_MASK))) {
				if (msg[1] & PRESS_MSG_MASK)
					copy_data->touch_is_pressed_arr[id] =
					TSP_STATE_PRESS;
				else if (msg[1] & MOVE_MSG_MASK)
					copy_data->touch_is_pressed_arr[id] =
					TSP_STATE_MOVE;

				data->fingers[id].z = msg[6];
				data->fingers[id].w = msg[5];
				data->fingers[id].x =
				    ((msg[2] << 4) | (msg[4] >> 4)) >>
				    data->x_dropbits;
				data->fingers[id].y =
				    ((msg[3] << 4) | (msg[4] & 0xF)) >>
				    data->y_dropbits;
				data->finger_mask |= 1U << id;
#if defined(DRIVER_FILTER)
				if (msg[1] & PRESS_MSG_MASK) {
					equalize_coordinate(1, id,
							    &data->
							    fingers[id].x,
							    &data->
							    fingers[id].y);
				} else if (msg[1] & MOVE_MSG_MASK) {
					equalize_coordinate(0, id,
							    &data->
							    fingers[id].x,
							    &data->
							    fingers[id].y);
				}
#endif
#ifdef _SUPPORT_SHAPE_TOUCH_
				data->fingers[id].component = msg[7];
#endif

			} else if ((msg[1] & SUPPRESS_MSG_MASK)
				   && (data->fingers[id].z != -1)) {
				data->fingers[id].z = TSP_STATE_RELEASE;
				data->fingers[id].w = msg[5];
				data->finger_mask |= 1U << id;
				copy_data->touch_is_pressed_arr[id] =
					TSP_STATE_RELEASE;
			} else {
				dev_dbg(&data->client->dev,
					"Unknown state %#02x %#02x", msg[0],
					msg[1]);
				continue;
			}
		}
		if (data->finger_mask)
			report_input_data(data);

		if (touch_message_flag && (cal_check_flag)
			&& !Doing_calibration_flag)
			check_chip_calibration(1);
	} while (!gpio_get_value(data->gpio_read_done));

	if ((!copy_data->optiacl_gain) && (data->family_id != 0x81)) {
		mxt224_optical_gain(QT_REFERENCE_MODE);
		copy_data->optiacl_gain = 1;
	}

	return IRQ_HANDLED;
}

static int mxt224_internal_suspend(struct mxt224_data *data)
{
	int i;
	int ret = 0;
#ifdef CONFIG_TARGET_LOCALE_NA
	static const u8 sleep_power_cfg[3];
	bool ta_status = 0;
	data->read_ta_status(&ta_status);

	if (ta_status) {
		printk(KERN_ERR "[TSP] sleep\n");
		for (i = 0; i < data->num_fingers; i++) {
			if (data->fingers[i].z == -1)
				continue;

			copy_data->touch_is_pressed_arr[i] = 0;
			data->fingers[i].z = 0;
		}
		report_input_data(data);

		ret = write_config(data, GEN_POWERCONFIG_T7, sleep_power_cfg);
		copy_data->sleep_mode_flag = 1;
	} else {
#endif	/* CONFIG_TARGET_LOCALE_NA */
		for (i = 0; i < data->num_fingers; i++) {
			if (data->fingers[i].z == -1)
				continue;

			copy_data->touch_is_pressed_arr[i] = 0;
			data->fingers[i].z = 0;
		}
		report_input_data(data);

		data->power_off();
#ifdef CONFIG_TARGET_LOCALE_NA
	}
#endif	/* CONFIG_TARGET_LOCALE_NA */
	return ret;
}

static int mxt224_internal_resume(struct mxt224_data *data)
{
	int ret = 0;
#ifdef CONFIG_TARGET_LOCALE_NA
	int i = 0;
	bool ta_status = 0;
	data->read_ta_status(&ta_status);
	if (copy_data->sleep_mode_flag) {
		printk(KERN_ERR "[TSP] sleep wake up\n");
		do {
			ret =
			    write_config(data, GEN_POWERCONFIG_T7,
					 data->power_cfg);
			msleep(20);
			i++;
		} while (ret && i < 10);
		msleep(20);
		copy_data->boot_or_resume = 1;
		copy_data->sleep_mode_flag = 0;
	} else {
#endif	/* CONFIG_TARGET_LOCALE_NA */
		data->power_on();
		copy_data->boot_or_resume = 1;
#ifdef CONFIG_TARGET_LOCALE_NA
	}
#endif	/* CONFIG_TARGET_LOCALE_NA */
	data->noise_median.mferr_count = 0;
	data->noise_median.mferr_setting = false;
	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt224_suspend	NULL
#define mxt224_resume	NULL

static void mxt224_early_suspend(struct early_suspend *h)
{
	struct mxt224_data *data = container_of(h, struct mxt224_data,
						early_suspend);

	copy_data->mxt224_enabled = 0;
	touch_is_pressed = 0;
	qt_timer_state = 0;
	not_yet_count = 0;
	Doing_calibration_flag = 0;
	copy_data->freq_table.fherr_cnt = 0;
	copy_data->freq_table.fherr_num = 1;

	disable_irq(data->client->irq);
	mxt224_internal_suspend(data);
}

static void mxt224_late_resume(struct early_suspend *h)
{
	struct mxt224_data *data = container_of(h, struct mxt224_data,
						early_suspend);
	bool ta_status = 0;

	mxt224_internal_resume(data);
	enable_irq(data->client->irq);

	copy_data->mxt224_enabled = 1;
#ifdef CONFIG_TARGET_LOCALE_KOR
	copy_data->is_inputmethod = 0;
#endif
#ifdef CLEAR_MEDIAN_FILTER_ERROR
	copy_data->noise_median.mferr_count = 0;
	copy_data->noise_median.mferr_setting = false;
	copy_data->noise_median.median_on_flag = false;
#endif
	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
	calibrate_chip();
}
#else
static int mxt224_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);

	copy_data->mxt224_enabled = 0;
	touch_is_pressed = 0;
	/* Doing_calibration_falg = 0; */
	return mxt224_internal_suspend(data);
}

static int mxt224_resume(struct device *dev)
{
	int ret = 0;
	bool ta_status = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);

	ret = mxt224_internal_resume(data);

	copy_data->mxt224_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
	return ret;
}
#endif

void Mxt224_force_released(void)
{
	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR "[TSP] copy_data->mxt224_enabled is 0\n");
		return;
	}

	calibrate_chip();

	touch_is_pressed = 0;
};
EXPORT_SYMBOL(Mxt224_force_released);

static ssize_t mxt224_debug_setting(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	copy_data->g_debug_switch = !copy_data->g_debug_switch;
	return 0;
}

static ssize_t qt602240_object_setting(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct mxt224_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	unsigned int object_register;
	unsigned int register_value;
	u8 value;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	sscanf(buf, "%u%u%u", &object_type, &object_register, &register_value);
	printk(KERN_ERR "[TSP] object type T%d", object_type);
	printk(KERN_ERR "[TSP] object register ->Byte%d\n", object_register);
	printk(KERN_ERR "[TSP] register value %d\n", register_value);
	ret = get_object_info(data, (u8) object_type, &size, &address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		return count;
	}

	size = 1;
	value = (u8) register_value;
	write_mem(data, address + (u16) object_register, size, &value);
	read_mem(data, address + (u16) object_register, (u8) size, &val);

	printk(KERN_ERR "[TSP] T%d Byte%d is %d\n", object_type,
	       object_register, val);

	return count;

}

static ssize_t qt602240_object_show(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct mxt224_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	u16 i;
	sscanf(buf, "%u", &object_type);
	printk(KERN_ERR "[TSP] object type T%d\n", object_type);
	ret = get_object_info(data, (u8) object_type, &size, &address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		return count;
	}
	for (i = 0; i < size; i++) {
		read_mem(data, address + i, 1, &val);
		printk(KERN_ERR "[TSP] Byte %u --> %u\n", i, val);
	}
	return count;
}

int get_tsp_status(void)
{
	return touch_is_pressed;
}

#define ENABLE_NOISE_TEST_MODE	1
#ifdef ENABLE_NOISE_TEST_MODE
struct device *sec_touchscreen;
struct device *tsp_noise_test;
/*
	botton_right, botton_left, center, top_right, top_left
*/
unsigned char test_node[5] = { 12, 20, 104, 188, 196 };

uint16_t qt_refrence_node[209] = { 0 };
uint16_t qt_delta_node[209] = { 0 };

void diagnostic_chip(u8 mode)
{
	int error;
	u16 t6_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	u16 t37_address = 0;
	int retry = 3;

	ret =
	    get_object_info(copy_data, GEN_COMMANDPROCESSOR_T6, &size_one,
			    &t6_address);

	size_one = 1;
	while (retry--) {
		error =
		    write_mem(copy_data, t6_address + 5, (u8) size_one, &mode);

		if (error < 0) {
			printk(KERN_ERR "[TSP] error %s: write_object\n",
			       __func__);
		} else {
			get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37,
					&size_one, &t37_address);
			size_one = 1;
			read_mem(copy_data, t37_address, (u8)size_one, &value);
			return;
		}
	}
	printk(KERN_ERR "[TSP] error %s: write_object fail!!\n", __func__);
	mxt224_reset(copy_data);
	return;
}

void read_dbg_data(uint8_t dbg_mode, uint8_t node, uint16_t *dbg_data)
{
	u8 read_page, read_point;
	u8 data_buffer[2] = { 0 };
	int i, ret;
	u16 size;
	u16 object_address = 0;

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR
			"[TSP ]read_dbg_data. "
			"copy_data->mxt224_enabled is 0\n");
		return;
	}

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(20);

	diagnostic_chip(dbg_mode);
	msleep(20);

	ret =
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size,
			    &object_address);
#if 0
	for (i = 0; i < 5; i++) {
		/* qt602240_read_diagnostic(0, data_buffer, 1); */
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(20);
#endif
	printk(KERN_DEBUG "[TSP] page clear\n");

	for (i = 1; i <= read_page; i++) {
		diagnostic_chip(QT_PAGE_UP);
		msleep(20);
		/* qt602240_read_diagnostic(1, data_buffer, 1); */
		read_mem(copy_data, object_address + 1, 1, data_buffer);
		if (data_buffer[0] != i) {
			if (data_buffer[0] >= 0x4)
				break;
			i--;
		}
	}

	/* qt602240_read_diagnostic(read_point, data_buffer, 2); */
	read_mem(copy_data, object_address + (u16) read_point, 2, data_buffer);
	*dbg_data =
	    ((uint16_t) data_buffer[1] << 8) + (uint16_t) data_buffer[0];
}

#define MAX_VALUE 4840
#define MIN_VALUE 13500

int read_all_data(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 max_value = MAX_VALUE, min_value = MIN_VALUE;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(30);		/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);		/* msleep(20);  */

	ret =
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size,
			    &object_address);
/*jerry no need to leave it */
#if 0
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(50);		/* msleep(20);  */
#endif
	if (copy_data->family_id == 0x81) {
		max_value = max_value + 16384;
		min_value = min_value + 16384;
	}

	for (read_page = 0; read_page < 4; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data, object_address + (u16) read_point,
				 2, data_buffer);
			qt_refrence_node[num] =
			    ((uint16_t) data_buffer[1] << 8) +
			    (uint16_t) data_buffer[0];
			if (copy_data->family_id == 0x81) {
				if ((qt_refrence_node[num] > MIN_VALUE + 16384)
				    || (qt_refrence_node[num] <
					MAX_VALUE + 16384)) {
					state = 1;
					printk(KERN_ERR
				"[TSP] Mxt224-E qt_refrence_node[%3d] = %5d\n",
					num, qt_refrence_node[num]);
				}
			} else {
				if ((qt_refrence_node[num] > MIN_VALUE)
				    || (qt_refrence_node[num] < MAX_VALUE)) {
					state = 1;
					printk(KERN_ERR
				"[TSP] Mxt224 qt_refrence_node[%3d] = %5d\n",
					num, qt_refrence_node[num]);
				}
			}

			if (data_buffer[0] != 0) {
				if (qt_refrence_node[num] > max_value)
					max_value = qt_refrence_node[num];
				if (qt_refrence_node[num] < min_value)
					min_value = qt_refrence_node[num];
			}
			num = num + 1;

			/* all node => 19 * 11 = 209 => (3page * 64) + 17 */
			if ((read_page == 3) && (node == 16))
				break;

		}
		diagnostic_chip(QT_PAGE_UP);
		msleep(20);
	}

	if ((max_value - min_value) > 4500) {
		printk(KERN_ERR
		       "[TSP] diff = %d, max_value = %d, min_value = %d\n",
		       (max_value - min_value), max_value, min_value);
		state = 1;
	}

	return state;
}

int read_all_delta_data(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(30);		/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);		/* msleep(20);  */

	ret =
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size,
			    &object_address);
/*jerry no need to leave it */
#if 0
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(50);		/* msleep(20);  */
#endif

	for (read_page = 0; read_page < 4; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data, object_address + (u16) read_point,
				 2, data_buffer);
			qt_delta_node[num] =
			    ((uint16_t) data_buffer[1] << 8) +
			    (uint16_t) data_buffer[0];

			num = num + 1;

			/* all node => 19 * 11 = 209 => (3page * 64) + 17 */
			if ((read_page == 3) && (node == 16))
				break;

		}
		diagnostic_chip(QT_PAGE_UP);
		msleep(20);
	}

	return state;
}

static void mxt224_optical_gain(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	uint16_t qt_refrence;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int ret, reference_over = 0;
	u16 size;
	u16 size_one;
	u8 value;
	int gain = 0;
	u8 val = 0;
	unsigned int register_address = 6;

	printk(KERN_ERR "[TSP] +mxt224_optical_gain()\n");

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(20);

	diagnostic_chip(dbg_mode);
	msleep(20);

	ret =
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size,
			    &object_address);
/*jerry no need of it*/
#if 0
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(20);
#endif

	for (read_page = 0; read_page < 4; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data, object_address + (u16) read_point,
				 2, data_buffer);
			qt_refrence =
			    ((uint16_t) data_buffer[1] << 8) +
			    (uint16_t) data_buffer[0];

#if defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
			if (copy_data->family_id == 0x81) {
				if (qt_refrence != 0)
					qt_refrence = qt_refrence - 16384;

				if (qt_refrence > 13280) {
					reference_over = 1;
					printk(KERN_DEBUG
				"[TSP] reference value beyond 13280\n");
				}

			} else {
				if (qt_refrence > 14500)
					reference_over = 1;
			}
#else
			if (copy_data->family_id == 0x81)
				qt_refrence = qt_refrence - 16384;
			if (qt_refrence > 14500)
				reference_over = 1;
#endif
			if ((read_page == 3) && (node == 16))
				break;
		}
		diagnostic_chip(QT_PAGE_UP);
		msleep(20);
	}

#if defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	if (reference_over)
		copy_data->gain_ta = 0;
#else
	if (reference_over)
		gain = 16;
	else
		gain = 32;

	value = (u8) gain;
	ret =
	    get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9, &size_one,
			    &object_address);
	size_one = 1;
	write_mem(copy_data, object_address + (u16) register_address, size_one,
		  &value);
	read_mem(copy_data, object_address + (u16) register_address,
		 (u8) size_one, &val);

#endif
	printk(KERN_ERR "[TSP] -mxt224_optical_gain()\n");
};

static int mxt224_check_bootloader(struct i2c_client *client,
				   unsigned int state)
{
	u8 val;

 recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case QT602240_WAITING_BOOTLOAD_CMD:
	case QT602240_WAITING_FRAME_DATA:
		val &= ~QT602240_BOOT_STATUS_MASK;
		break;
	case QT602240_FRAME_CRC_PASS:
		if (val == QT602240_FRAME_CRC_CHECK)
			goto recheck;
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev, "Unvalid bootloader mode state\n");
		printk(KERN_ERR "[TSP] Unvalid bootloader mode state\n");
		return -EINVAL;
	}

	return 0;
}

static int mxt224_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = QT602240_UNLOCK_CMD_LSB;
	buf[1] = QT602240_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt224_fw_write(struct i2c_client *client,
			   const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt224_load_fw(struct device *dev, const char *fn)
{

	struct mxt224_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address = 0;
	u16 size_one;
	u8 value;
	unsigned int object_register;

	printk(KERN_ERR "[TSP] mxt224_load_fw start!!!\n");

	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		printk(KERN_ERR "[TSP] Unable to open firmware %s\n", fn);
		return ret;
	}

	/* Change to the bootloader mode */
	object_register = 0;
	value = (u8) QT602240_BOOT_VALUE;
	ret =
	    get_object_info(data, GEN_COMMANDPROCESSOR_T6, &size_one,
			    &obj_address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		return ret;
	}

	size_one = 1;
	write_mem(data, obj_address + (u16) object_register, (u8) size_one,
		  &value);
	msleep(QT602240_RESET_TIME);

	/* Change to slave address of bootloader */
	if (client->addr == QT602240_APP_LOW)
		client->addr = QT602240_BOOT_LOW;
	else
		client->addr = QT602240_BOOT_HIGH;

	ret = mxt224_check_bootloader(client, QT602240_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt224_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt224_check_bootloader(client,
					      QT602240_WAITING_FRAME_DATA);
		if (ret)
			goto out;

		frame_size =
			((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size
		as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;

		/* Write one frame to device */
		/* qt602240_fw_write(client, fw->data + pos, frame_size);  */
		mxt224_fw_write(client, fw->data + pos, frame_size);

		ret = mxt224_check_bootloader(client, QT602240_FRAME_CRC_PASS);
		if (ret)
			goto out;

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, fw->size);
		printk(KERN_ERR "[TSP] Updated %d bytes / %zd bytes\n", pos,
		       fw->size);
	}

 out:
	release_firmware(fw);

	/* Change to slave address of application */
	if (client->addr == QT602240_BOOT_LOW)
		client->addr = QT602240_APP_LOW;
	else
		client->addr = QT602240_APP_HIGH;

	return ret;
}

static ssize_t set_refer0_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_refrence = 0;
	read_dbg_data(QT_REFERENCE_MODE, test_node[0], &qt_refrence);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer1_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_refrence = 0;
	read_dbg_data(QT_REFERENCE_MODE, test_node[1], &qt_refrence);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer2_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_refrence = 0;
	read_dbg_data(QT_REFERENCE_MODE, test_node[2], &qt_refrence);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer3_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_refrence = 0;
	read_dbg_data(QT_REFERENCE_MODE, test_node[3], &qt_refrence);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer4_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_refrence = 0;
	read_dbg_data(QT_REFERENCE_MODE, test_node[4], &qt_refrence);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_delta0_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_delta = 0;
	read_dbg_data(QT_DELTA_MODE, test_node[0], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else
		qt_delta = 65535 - qt_delta;

	return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta1_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_delta = 0;
	read_dbg_data(QT_DELTA_MODE, test_node[1], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else
		qt_delta = 65535 - qt_delta;

	return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta2_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_delta = 0;
	read_dbg_data(QT_DELTA_MODE, test_node[2], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else
		qt_delta = 65535 - qt_delta;

	return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta3_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_delta = 0;
	read_dbg_data(QT_DELTA_MODE, test_node[3], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else
		qt_delta = 65535 - qt_delta;

	return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta4_mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	uint16_t qt_delta = 0;
	read_dbg_data(QT_DELTA_MODE, test_node[4], &qt_delta);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else
		qt_delta = 65535 - qt_delta;

	return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_threshold_mode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	char temp[15];
	if (copy_data->mxt_version_disp == 0x80) {
		sprintf(temp, "%u\n", copy_data->threshold);
		strcat(buf, temp);
	} else if (copy_data->mxt_version_disp == 0x81) {
		sprintf(temp, "%u\n", copy_data->threshold_e);
		strcat(buf, temp);
	} else {
		sprintf(temp, "error\n");
		strcat(buf, temp);
	}
	return strlen(buf);
}

static ssize_t set_all_refer_mode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_data(QT_REFERENCE_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_reference;

static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return -1;
	while (str[count] != '\0' && str[count] >= '0' && str[count] <= '9') {
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}

ssize_t disp_all_refdata_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{

/*	int status = 0;
	char tempStr[5*209 + 1] = { 0 };
	nt i = 0;*/
	return sprintf(buf, "%u\n", qt_refrence_node[index_reference]);
}

ssize_t disp_all_refdata_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t size)
{

	index_reference = atoi(buf);
	return size;
}

static ssize_t set_all_delta_mode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_delta_data(QT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_delta;

ssize_t disp_all_deltadata_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (qt_delta_node[index_delta] < 32767)
		return sprintf(buf, "%u\n", qt_delta_node[index_delta]);
	else
		qt_delta_node[index_delta] =
		65535 - qt_delta_node[index_delta];

	return sprintf(buf, "-%u\n", qt_delta_node[index_delta]);
}

ssize_t disp_all_deltadata_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{

	index_delta = atoi(buf);
	return size;
}

static ssize_t set_firm_version_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%#02x\n", copy_data->tsp_version_disp);

}

static ssize_t set_module_off_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt224_data *data = copy_data;
	int count;

	copy_data->mxt224_enabled = 0;
	touch_is_pressed = 0;
	disable_irq(data->client->irq);
	mxt224_internal_suspend(data);

	count = sprintf(buf, "tspoff\n");

	return count;
}

static ssize_t set_module_on_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct mxt224_data *data = copy_data;
	int count;

	bool ta_status = 0;

	mxt224_internal_resume(data);
	enable_irq(data->client->irq);

	copy_data->mxt224_enabled = 1;
#ifdef CONFIG_TARGET_LOCALE_KOR
	copy_data->is_inputmethod = 0;
#endif
	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
	calibrate_chip();

	count = sprintf(buf, "tspon\n");

	return count;
}

static ssize_t set_mxt_update_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt224_data *data = copy_data;
	int error = 0;
	int count = 0;
	printk(KERN_ERR "[TSP] set_mxt_update_show start!!\n");

	disable_irq(data->client->irq);
	copy_data->firm_status_data = 1;
	if (data->family_id == 0x80) {	/*  : MXT-224 */
		printk(KERN_ERR "[TSP] mxt224_fm_update\n");
		error = mxt224_load_fw(dev, MXT224_FW_NAME);
	} else if (data->family_id == 0x81) {	/*MXT-224E */
		printk(KERN_ERR "[TSP] mxt224E_fm_update\n");
		error = mxt224_load_fw(dev, MXT224_ECHO_FW_NAME);
	}
	/*jerry no need of it */
	/* error = mxt224_load_fw(dev, QT602240_FW_NAME);  */
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		copy_data->firm_status_data = 3;
		printk(KERN_ERR
			"[TSP The firmware update failed(%d)\n", error);
		return error;
	} else {
		dev_dbg(dev, "The firmware update succeeded\n");
		copy_data->firm_status_data = 2;
		printk(KERN_ERR "[TSP] The firmware update succeeded\n");

		/* Wait for reset */
		msleep(QT602240_FWRESET_TIME);
		/* initialize the TSP */
		mxt224_init_touch_driver(data);
		/*jerry no need of it */
		/* qt602240_initialize(data);  */
	}

	enable_irq(data->client->irq);
	error = mxt224_backup(data);
	if (error) {
		printk(KERN_ERR "[TSP] mxt224_backup fail!!!\n");
		return error;
	}

	/* reset the touch IC. */
	error = mxt224_reset(data);
	if (error) {
		printk(KERN_ERR "[TSP] mxt224_reset fail!!!\n");
		return error;
	}

	msleep(60);
	return count;
}

static ssize_t set_mxt_firm_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	int count;
	/* struct mxt_data *mxt = dev_get_drvdata(dev); */
	pr_info("Enter firmware_status_show by Factory command\n");

	if (copy_data->firm_status_data == 1)
		count = sprintf(buf, "Downloading\n");
	else if (copy_data->firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (copy_data->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");

	return count;

}

static ssize_t tsp_threshold_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	if (copy_data->mxt_version_disp == 0x80)
		return sprintf(buf, "%u\n", copy_data->threshold);
	else
		return sprintf(buf, "%u\n", copy_data->threshold_e);
}

static ssize_t tsp_threshold_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	/*TO DO IT */
	unsigned int object_register = 7;
	u8 value;
	u8 val;
	int ret;
	u16 address = 0;
	u16 size_one;
	int threshold;

	if (copy_data->mxt_version_disp == 0x80)
		threshold = copy_data->threshold;
	else
		threshold = copy_data->threshold_e;

	if (sscanf(buf, "%d", &threshold) == 1) {
		printk(KERN_ERR "[TSP] threshold value %d\n",
			threshold);
		ret =
		    get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9,
				    &size_one, &address);
		size_one = 1;
		value = (u8) threshold;
		write_mem(copy_data, address + (u16) object_register, size_one,
			  &value);
		read_mem(copy_data, address + (u16) object_register,
			 (u8) size_one, &val);

		printk(KERN_ERR "[TSP] T%d Byte%d is %d\n",
		       TOUCH_MULTITOUCHSCREEN_T9, object_register, val);
	}

	return size;
}

static ssize_t set_mxt_firm_version_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	u8 fw_latest_version;
	fw_latest_version = copy_data->tsp_version_disp;
	pr_info("Atmel Last firmware version is %d\n", fw_latest_version);
	return sprintf(buf, "%#02x\n", fw_latest_version);
}

static ssize_t set_mxt_firm_version_read_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	return sprintf(buf, "%#02x\n", copy_data->tsp_version_disp);
}

static ssize_t set_mxt_config_version_read_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	return sprintf(buf, "%s\n", copy_data->tsp_config_version);
}

#ifdef CONFIG_TARGET_LOCALE_KOR
ssize_t set_tsp_for_inputmethod_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "[TSP] %s is called.. is_inputmethod=%d\n", __func__,
	       copy_data->is_inputmethod);
	if (copy_data->is_inputmethod)
		*buf = '1';
	else
		*buf = '0';

	return 0;
}

ssize_t set_tsp_for_inputmethod_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	u16 obj_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	int jump_limit = 0;
	int mrgthr = 0;
	u8 val = 0;
	unsigned int register_address = 0;

	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR
		       "[TSP ]set_tsp_for_inputmethod_store. "
		       "copy_data->mxt224_enabled is 0\n");
		return 1;
	}

	if (*buf == '1' && (!copy_data->is_inputmethod)) {
		copy_data->is_inputmethod = 1;
		jump_limit = 10;
		mrgthr = 5;
		printk(KERN_ERR "[TSP] Set TSP inputmethod IN\n");

		ret =
		    get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9,
				    &size_one, &obj_address);
		register_address = 16;
		value = (u8) mrgthr;
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "T%d Byte%d is %d\n",
			9, register_address, val);

		register_address = 30;
		value = (u8) jump_limit;
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "T%d Byte%d is %d\n",
			9, register_address, val);
	} else if (*buf == '0' && (copy_data->is_inputmethod)) {
		copy_data->is_inputmethod = 0;
		jump_limit = 18;
		mrgthr = 40;
		printk(KERN_ERR "[TSP] Set TSP inputmethod OUT\n");

		ret =
		    get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9,
				    &size_one, &obj_address);
		register_address = 16;
		value = (u8) mrgthr;
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "T%d Byte%d is %d\n",
			9, register_address, val);

		register_address = 30;
		value = (u8) jump_limit;
		size_one = 1;
		write_mem(copy_data, obj_address + (u16) register_address,
			  size_one, &value);
		read_mem(copy_data, obj_address + (u16) register_address,
			 (u8) size_one, &val);
		printk(KERN_ERR "T%d Byte%d is %d\n",
			9, register_address, val);
	}

	return 1;
}
#endif

static ssize_t tsp_touchtype_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char temp[15];
	if (copy_data->mxt_version_disp == 0x80) {
		sprintf(temp, "TSP : MXT224\n");
		strcat(buf, temp);
	} else if (copy_data->mxt_version_disp == 0x81) {
		sprintf(temp, "TSP : MXT224E\n");
		strcat(buf, temp);
	} else {
		sprintf(temp, "error\n");
		strcat(buf, temp);
		dev_info(dev, "read mxt TSP type read failed.\n");
	}
	return strlen(buf);
}

static DEVICE_ATTR(set_refer0, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_all_refer_mode_show, NULL);
static DEVICE_ATTR(disp_all_refdata, S_IRUGO | S_IWUSR | S_IWGRP,
		   disp_all_refdata_show, disp_all_refdata_store);
static DEVICE_ATTR(set_all_delta, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_all_delta_mode_show, NULL);
static DEVICE_ATTR(disp_all_deltadata, S_IRUGO | S_IWUSR | S_IWGRP,
		   disp_all_deltadata_show, disp_all_deltadata_store);
static DEVICE_ATTR(set_threshould, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_threshold_mode_show, NULL);
static DEVICE_ATTR(set_firm_version, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_firm_version_show, NULL);
static DEVICE_ATTR(set_module_off, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_module_off_show, NULL);
static DEVICE_ATTR(set_module_on, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_module_on_show, NULL);

/*
	20110222 N1 firmware sync
*/
static DEVICE_ATTR(tsp_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
	set_mxt_update_show, NULL);/* firmware update */
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
	set_mxt_firm_status_show, NULL);/* firmware update status return */
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	tsp_threshold_show, tsp_threshold_store);/* threshold return, store */
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
	set_mxt_firm_version_show, NULL);	/* PHONE */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_mxt_firm_version_read_show, NULL);/*PART*/
static DEVICE_ATTR(tsp_config_version, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_mxt_config_version_read_show, NULL);
 /*PART*/			/* TSP config last modifying date */
#ifdef CONFIG_TARGET_LOCALE_KOR
/* For 3x4 Input Method, Jump limit changed API */
static DEVICE_ATTR(set_tsp_for_inputmethod, S_IRUGO | S_IWUSR | S_IWGRP,
	set_tsp_for_inputmethod_show, set_tsp_for_inputmethod_store);
#endif
static DEVICE_ATTR(tsp_touchtype, S_IRUGO | S_IWUSR | S_IWGRP,
		   tsp_touchtype_show, NULL);
#endif				/*ENABLE_NOISE_TEST_MODE*/

static DEVICE_ATTR(object_show, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   qt602240_object_show);
static DEVICE_ATTR(object_write, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   qt602240_object_setting);
static DEVICE_ATTR(dbg_switch, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   mxt224_debug_setting);

static int sec_touchscreen_enable(struct mxt224_data *data)
{
	mutex_lock(&data->lock);
	if (data->enabled)
		goto out;

	data->enabled = true;
	enable_irq(data->client->irq);
	printk(KERN_ERR "[TSP] %s\n", __func__);
out:
	mutex_unlock(&data->lock);
	return 0;
}

static int sec_touchscreen_disable(struct mxt224_data *data)
{
	mutex_lock(&data->lock);
	if (!data->enabled)
		goto out;

	disable_irq(data->client->irq);
	data->enabled = false;
	printk(KERN_ERR "[TSP] %s\n", __func__);
out:
	mutex_unlock(&data->lock);
	return 0;
}

static int sec_touchscreen_open(struct input_dev *dev)
{
	struct mxt224_data *data = input_get_drvdata(dev);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (ret > 0) {
		if (data->client->irq != -1)
			ret = sec_touchscreen_enable(data);
		else
			ret = -ENXIO;
	} else if (ret < 0) {
		dev_err(&dev->dev,
			"error while waiting for device to init (%d)\n", ret);
		ret = -ENXIO;
	} else if (ret == 0) {
		dev_err(&dev->dev,
			"timedout while waiting for device to init\n");
		ret = -ENXIO;
	}

	return ret;
}

static void sec_touchscreen_close(struct input_dev *dev)
{
	struct mxt224_data *data = input_get_drvdata(dev);
	sec_touchscreen_disable(data);
}

static struct attribute *qt602240_attrs[] = {
	&dev_attr_object_show.attr,
	&dev_attr_object_write.attr,
	&dev_attr_dbg_switch.attr,
	NULL
};

static const struct attribute_group qt602240_attr_group = {
	.attrs = qt602240_attrs,
};

static int __devinit mxt224_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct mxt224_platform_data *pdata = client->dev.platform_data;
	struct mxt224_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;
	bool ta_status;
	const u8 **tsp_config;
	u16 size_one;
	u8 user_info_value;
	u16 obj_address = 0;

#if defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	int gain_ta_pre;
#endif

	touch_is_pressed = 0;

	if (!pdata) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
		       sizeof(*data->fingers), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->num_fingers = pdata->max_finger_touches;
	data->power_on = pdata->power_on;
	data->power_off = pdata->power_off;
	data->register_cb = pdata->register_cb;
	data->read_ta_status = pdata->read_ta_status;

	data->client = client;
	i2c_set_clientdata(client, data);
	init_completion(&data->init_done);
	mutex_init(&data->lock);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "input device allocation failed\n");
		goto err_alloc_dev;
	}
	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "sec_touchscreen";
	input_dev->open = sec_touchscreen_open;
	input_dev->close = sec_touchscreen_close;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(MT_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, data->num_fingers);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->min_x,
			     pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->min_y,
			     pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, pdata->min_z,
			     pdata->max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, pdata->min_w,
			     pdata->max_w, 0, 0);

#ifdef _SUPPORT_SHAPE_TOUCH_
	input_set_abs_params(input_dev, ABS_MT_COMPONENT, 0, 255, 0, 0);
#endif
	ret = input_register_device(input_dev);
	if (ret) {
		input_free_device(input_dev);
		goto err_reg_dev;
	}

	data->gpio_read_done = pdata->gpio_read_done;

	data->power_on();
#if defined(CONFIG_MACH_M0_GRANDECTC)
	msleep(100);
#endif
	ret = mxt224_init_touch_driver(data);
	if (ret) {
		dev_err(&client->dev, "chip initialization failed\n");
		goto err_init_drv;
	}

	data->register_cb(mxt224_ta_probe);

	data->boot_or_resume = 1;
	data->gErrCondition = ERR_RTN_CONDITION_IDLE;
	data->ta_status_pre = 0;
	data->sleep_mode_flag = 0;

	/* config tunning date */
	data->tsp_config_version = "20111215";

	copy_data = data;

	if (data->family_id == 0x80) {	/*MXT-224 */
		tsp_config = pdata->config;
		data->atchcalst = pdata->atchcalst;
		data->atchcalsthr = pdata->atchcalsthr;
		data->tchthr_batt = pdata->tchthr_batt;
		data->tchthr_batt_init = pdata->tchthr_batt_init;
		data->tchthr_charging = pdata->tchthr_charging;
		data->noisethr_batt = pdata->noisethr_batt;
		data->noisethr_charging = pdata->noisethr_charging;
		data->movfilter_batt = pdata->movfilter_batt;
		data->movfilter_charging = pdata->movfilter_charging;
		data->threshold = pdata->tchthr_charging;

		printk(KERN_ERR "[TSP] TSP chip is MXT224\n");
	} else if (data->family_id == 0x81) {	/* MXT-224E */
		tsp_config = pdata->config_e;
		data->atchcalst_e = pdata->atchcalst_e;
		data->atchcalsthr_e = pdata->atchcalsthr_e;
		data->noise_suppression_cfg = pdata->t48_config_batt_e + 1;
		data->noise_suppression_cfg_ta = pdata->t48_config_chrg_e + 1;
		data->tchthr_batt_e = pdata->tchthr_batt_e;
		data->tchthr_charging_e = pdata->tchthr_charging_e;
		data->calcfg_batt_e = pdata->calcfg_batt_e;
		data->calcfg_charging_e = pdata->calcfg_charging_e;
		data->atchfrccalthr_e = pdata->atchfrccalthr_e;
		data->atchfrccalratio_e = pdata->atchfrccalratio_e;
		data->chrgtime_batt_e = pdata->chrgtime_batt_e;
		data->chrgtime_charging_e = pdata->chrgtime_charging_e;
		data->blen_batt_e = pdata->blen_batt_e;
		data->blen_charging_e = pdata->blen_charging_e;
		data->movfilter_batt_e = pdata->movfilter_batt_e;
		data->movfilter_charging_e = pdata->movfilter_charging_e;
		data->actvsyncsperx_e = pdata->actvsyncsperx_e;
		data->nexttchdi_e = pdata->nexttchdi_e;
		data->threshold_e = pdata->tchthr_batt_e;

		printk(KERN_ERR "[TSP] TSP chip is MXT224-E\n");
		get_object_info(data, SPT_USERDATA_T38, &size_one,
				&obj_address);
		size_one = 1;
		read_mem(data, obj_address, (u8) size_one, &user_info_value);
		printk(KERN_ERR "[TSP]user_info_value is %d\n",
		       user_info_value);
	} else {
		printk(KERN_ERR "[TSP] ERROR : There is no valid TSP ID\n");
		goto err_config;
	}

	if ((data->family_id == 0x81) && (user_info_value == 165)) {
		for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
			if (tsp_config[i][0] == GEN_POWERCONFIG_T7)
				data->power_cfg = tsp_config[i] + 1;

			if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
				/* Are x and y inverted? */
				if (tsp_config[i][10] & 0x1) {
					data->x_dropbits =
					    (!(tsp_config[i][22] & 0xC)) << 1;
					data->y_dropbits =
					    (!(tsp_config[i][20] & 0xC)) << 1;
				} else {
					data->x_dropbits =
					    (!(tsp_config[i][20] & 0xC)) << 1;
					data->y_dropbits =
					    (!(tsp_config[i][22] & 0xC)) << 1;
				}
			}
			if (tsp_config[i][0] == PROCG_NOISESUPPRESSION_T48)
				data->noise_suppression_cfg =
					tsp_config[i] + 1;
		}
	} else {
		for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
			ret = write_config(data, tsp_config[i][0],
					   tsp_config[i] + 1);
			if (ret)
				goto err_config;

			if (tsp_config[i][0] == GEN_POWERCONFIG_T7)
				data->power_cfg = tsp_config[i] + 1;

			if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
				/* Are x and y inverted? */
				if (tsp_config[i][10] & 0x1) {
					data->x_dropbits =
					    (!(tsp_config[i][22] & 0xC)) << 1;
					data->y_dropbits =
					    (!(tsp_config[i][20] & 0xC)) << 1;
				} else {
					data->x_dropbits =
					    (!(tsp_config[i][20] & 0xC)) << 1;
					data->y_dropbits =
					    (!(tsp_config[i][22] & 0xC)) << 1;
				}
			}
			if (tsp_config[i][0] == PROCG_NOISESUPPRESSION_T48)
				data->noise_suppression_cfg =
					tsp_config[i] + 1;
		}
	}

	ret = mxt224_backup(data);
	if (ret)
		goto err_backup;

	/* reset the touch IC. */
	ret = mxt224_reset(data);
	if (ret)
		goto err_reset;

	msleep(60);
	copy_data->mxt224_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}

	calibrate_chip();

	for (i = 0; i < data->num_fingers; i++)
		data->fingers[i].z = TSP_STATE_INACTIVE;
#ifdef CLEAR_MEDIAN_FILTER_ERROR
	copy_data->noise_median.median_on_flag = false;
	copy_data->noise_median.mferr_setting = false;
	copy_data->noise_median.mferr_count = 0;
	copy_data->noise_median.t46_actvsyncsperx_for_mferr = 38;
	copy_data->noise_median.t48_mfinvlddiffthr_for_mferr = 12;
	copy_data->noise_median.t48_mferrorthr_for_mferr = 19;
	copy_data->noise_median.t48_thr_for_mferr = 45;
	copy_data->noise_median.t48_movfilter_for_mferr = 80;
#endif

	copy_data->freq_table.fherr_setting = 0;
	copy_data->freq_table.fherr_cnt = 0;
	copy_data->freq_table.fherr_num = 1;
	copy_data->freq_table.t9_blen_for_fherr = 16;
	copy_data->freq_table.t9_blen_for_fherr_cnt = 0;
	copy_data->freq_table.t9_thr_for_fherr = 60;
	copy_data->freq_table.t9_movfilter_for_fherr = 80;
	copy_data->freq_table.t22_noisethr_for_fherr = 30;
	copy_data->freq_table.t22_freqscale_for_fherr = 1;

	copy_data->freq_table.freq_for_fherr1[0] = 10;
	copy_data->freq_table.freq_for_fherr1[1] = 12;
	copy_data->freq_table.freq_for_fherr1[2] = 18;
	copy_data->freq_table.freq_for_fherr1[3] = 20;
	copy_data->freq_table.freq_for_fherr1[4] = 29;
	copy_data->freq_table.freq_for_fherr2[0] = 45;
	copy_data->freq_table.freq_for_fherr2[1] = 49;
	copy_data->freq_table.freq_for_fherr2[2] = 55;
	copy_data->freq_table.freq_for_fherr2[3] = 59;
	copy_data->freq_table.freq_for_fherr2[4] = 63;
	copy_data->freq_table.freq_for_fherr3[0] = 7;
	copy_data->freq_table.freq_for_fherr3[1] = 33;
	copy_data->freq_table.freq_for_fherr3[2] = 39;
	copy_data->freq_table.freq_for_fherr3[3] = 52;
	copy_data->freq_table.freq_for_fherr3[4] = 64;
	copy_data->freq_table.freq_for_fherr4[0] = 29;
	copy_data->freq_table.freq_for_fherr4[1] = 34;
	copy_data->freq_table.freq_for_fherr4[2] = 39;
	copy_data->freq_table.freq_for_fherr4[3] = 49;
	copy_data->freq_table.freq_for_fherr4[4] = 58;

	ret = request_threaded_irq(client->irq, NULL, mxt224_irq_thread,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt224_ts",
			data);

	if (ret < 0)
		goto err_irq;

	disable_irq(client->irq);
	complete_all(&data->init_done);

	ret = sysfs_create_group(&client->dev.kobj, &qt602240_attr_group);
	if (ret)
		printk(KERN_ERR "[TSP] sysfs_create_group()is falled\n");

#ifdef ENABLE_NOISE_TEST_MODE
/*
	20110222 N1_firmware_sync
*/
	sec_touchscreen =
	    device_create(sec_class, NULL, 0, NULL, "sec_touchscreen");

	if (IS_ERR(sec_touchscreen))
		printk(KERN_ERR
		       "[TSP] Failed to create device(sec_touchscreen)!\n");

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_update) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n",
		       dev_attr_tsp_firm_update.attr.name);

	if (device_create_file
	    (sec_touchscreen, &dev_attr_tsp_firm_update_status) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_firm_update_status.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_threshold) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_threshold.attr.name);

	if (device_create_file
	    (sec_touchscreen, &dev_attr_tsp_firm_version_phone) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_firm_version_phone.attr.name);

	if (device_create_file
	    (sec_touchscreen, &dev_attr_tsp_firm_version_panel) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_firm_version_panel.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_config_version) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_config_version.attr.name);

#ifdef CONFIG_TARGET_LOCALE_KOR
	if (device_create_file
	    (sec_touchscreen, &dev_attr_set_tsp_for_inputmethod) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_tsp_for_inputmethod.attr.name);
#endif
	if (device_create_file(sec_touchscreen, &dev_attr_tsp_touchtype) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_tsp_touchtype.attr.name);

/*
	end N1_firmware_sync
*/
	tsp_noise_test =
	    device_create(sec_class, NULL, 0, NULL, "tsp_noise_test");

	if (IS_ERR(tsp_noise_test))
		printk(KERN_ERR
		       "Failed to create device(tsp_noise_test)!\n");

	if (device_create_file(tsp_noise_test, &dev_attr_set_refer0) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_refer0.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_delta0) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_delta0.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_refer1) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_refer1.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_delta1) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_delta1.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_refer2) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_refer2.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_delta2) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_delta2.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_refer3) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_refer3.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_delta3) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_delta3.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_refer4) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_refer4.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_delta4) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_delta4.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_all_refer) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_all_refer.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_disp_all_refdata)
	    < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_disp_all_refdata.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_all_delta) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_all_delta.attr.name);

	if (device_create_file
	    (tsp_noise_test, &dev_attr_disp_all_deltadata) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_disp_all_deltadata.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_threshould) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_threshould.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_firm_version)
	    < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_firm_version.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_module_off) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_threshould.attr.name);

	if (device_create_file(tsp_noise_test, &dev_attr_set_module_on) <
	    0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_set_firm_version.attr.name);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#if defined(CONFIG_TARGET_LOCALE_NA) || defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
#elif defined(CONFIG_TARGET_LOCALE_KOR)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 3;
#else
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
#endif
	data->early_suspend.suspend = mxt224_early_suspend;
	data->early_suspend.resume = mxt224_late_resume;
	register_early_suspend(&data->early_suspend);
#endif


	return 0;

 err_irq:
 err_reset:
 err_backup:
 err_config:
	kfree(data->objects);
 err_init_drv:
	input_unregister_device(input_dev);
	gpio_free(data->gpio_read_done);
 err_reg_dev:
 err_alloc_dev:
	kfree(data);
	return ret;
}

static int __devexit mxt224_remove(struct i2c_client *client)
{
	struct mxt224_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	kfree(data->objects);
	gpio_free(data->gpio_read_done);
	data->power_off();
	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt224_idtable[] = {
	{MXT224_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt224_idtable);

static const struct dev_pm_ops mxt224_pm_ops = {
	.suspend = mxt224_suspend,
	.resume = mxt224_resume,
};

static struct i2c_driver mxt224_i2c_driver = {
	.id_table = mxt224_idtable,
	.probe = mxt224_probe,
	.remove = __devexit_p(mxt224_remove),
	.driver = {
		   .owner = THIS_MODULE,
		   .name = MXT224_DEV_NAME,
		   .pm = &mxt224_pm_ops,
		   },
};

static int __init mxt224_init(void)
{
	return i2c_add_driver(&mxt224_i2c_driver);
}

static void __exit mxt224_exit(void)
{
	i2c_del_driver(&mxt224_i2c_driver);
}

module_init(mxt224_init);
module_exit(mxt224_exit);

MODULE_DESCRIPTION("Atmel MaXTouch 224 driver");
MODULE_AUTHOR("Rom Lemarchand <rlemarchand@sta.samsung.com>");
MODULE_LICENSE("GPL");
