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
#include <linux/i2c/mxt224e.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <mach/cpufreq.h>
#include <linux/uaccess.h>
#include <linux/input/mt.h>

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

/* Slave addresses */
#define MXT224_APP_LOW		0x4a
#define MXT224_APP_HIGH		0x4b
#define MXT224_BOOT_LOW		0x24
#define MXT224_BOOT_HIGH		0x25

/*FIRMWARE NAME */
#define MXT224E_FW_NAME	    "mXT224e.fw"
#define MXT_FIRMWARE_INKERNEL_PATH	"tsp_atmel/"
#define MXT_MAX_FW_PATH			30
#define MXT_FIRMWARE_UPDATE_TYPE	true
#define MXT224E_FW_VER		0x10
#define MXT224E_BUILD_VER	0xaa

#define MXT_RESET_VALUE		0x01
#define MXT_BOOT_VALUE		0xa5

#define TSP_BUF_SIZE		1024

#define NODE_PER_PAGE		64
#define DATA_PER_NODE		2

#define REF_OFFSET_VALUE	16384
#define REF_MIN_VALUE		(19744 - REF_OFFSET_VALUE)
#define REF_MAX_VALUE		(28884 - REF_OFFSET_VALUE)

/* Diagnostic command defines  */
#define MXT_DIAG_PAGE_UP		0x01
#define MXT_DIAG_PAGE_DOWN		0x02
#define MXT_DIAG_DELTA_MODE		0x10
#define MXT_DIAG_REFERENCE_MODE		0x11
#define MXT_DIAG_CTE_MODE		0x31
#define MXT_DIAG_IDENTIFICATION_MODE	0x80
#define MXT_DIAG_TOCH_THRESHOLD_MODE	0xF4

#define MXT_DIAG_MODE_MASK		0xFC
#define MXT_DIAGNOSTIC_MODE		0
#define MXT_DIAGNOSTIC_PAGE		1

#define MXT_CONFIG_VERSION_LENGTH	30

enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

enum {
	MXT_FW_FROM_BUILT_IN = 0,
	MXT_FW_FROM_UMS,
	MXT_FW_FROM_REQ_FW,
};

#define MXT224_FWRESET_TIME		175	/* msec */
#define MXT224_RESET_TIME		65	/* msec */

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK		0x02
#define MXT_FRAME_CRC_FAIL		0x03
#define MXT_FRAME_CRC_PASS		0x04
#define MXT_APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB		0xaa
#define MXT_UNLOCK_CMD_LSB		0xdc

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

static bool gbfilter;

struct object_t {
	u8 object_type;
	u16 i2c_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;
} __packed;

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

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
	const struct mxt224_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mxt_fac_data *fdata;
	struct mxt_info info;
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

	int (*power_on) (void);
	int (*power_off) (void);
	void (*register_cb) (void *);
	void (*read_ta_status) (void *);
	int num_fingers;
	struct finger_info fingers[MXT224_MAX_MT_FINGERS];
	struct t48_median_config_t noise_median;

	int mxt224_enabled;
	bool g_debug_switch;
	u8 mxt_version_disp;
	u8 tsp_version_disp;
	int optiacl_gain;
	int firm_status_data;
	bool lock_status;
	int touch_state;		/* 1:release, 2:press, 3:others */
	bool ta_status_pre;
	int gErrCondition;
	int threshold;
	int threshold_e;
	bool boot_or_resume;	/*1: boot_or_resume,0: others */
	bool sleep_mode_flag;
	char *tsp_config_version;
	bool median_err_flag;
	int touch_is_pressed_arr[MAX_USING_FINGER_NUM];

	struct completion init_done;
	struct mutex lock;
	bool enabled;
};

struct mxt_fw_info {
	u8 fw_ver;
	u8 build_ver;
	u32 fw_len;
	const u8 *fw_raw_data;	/* start address of firmware data */
	struct mxt224_data *data;
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

static int mxt_read_object(struct mxt224_data *data,
				u8 type, u8 offset, u8 *val)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);
	if (ret) {
		printk(KERN_ERR"[TSP] get_object_info fail\n"); 
		return ret;
	}

	ret = read_mem(data, address + offset, 1, val);
	if (ret)
		printk(KERN_ERR"[TSP] Error to read T[%d] offset[%d] val[%d]\n",
			type, offset, *val);
	return ret;
}

static int mxt_write_object(struct mxt224_data *data,
				 u8 type, u8 offset, u8 val)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);
	if (ret) {
		printk(KERN_ERR"[TSP] get_object_info fail\n"); 
		return ret;
	}

	if (offset >= size) {
		printk(KERN_ERR"[TSP] Tried to write outside object T%d offset:%d, size:%d\n",
			type, offset, size);
		return -EINVAL;
	}

	ret = write_mem(data, address + offset, 1, &val);
	if (ret)
		printk(KERN_ERR"[TSP] Error to write T[%d] offset[%d] val[%d]\n",
			type, offset, val);

	return ret;
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

static unsigned int qt_time_point;
static unsigned int qt_time_diff;
static unsigned int qt_timer_state;
static unsigned int good_check_flag;
static unsigned int not_yet_count;

static u8 cal_check_flag;

static u8 Doing_calibration_flag;

uint8_t calibrate_chip(struct mxt224_data *data)
{
	u8 cal_data = 1;
	int ret = 0;
	u8 atchcalst_tmp, atchcalsthr_tmp;
	u16 obj_address = 0;
	u16 size = 1;

	if (cal_check_flag == 0) {
		cal_check_flag = 1u;

		ret =
		    get_object_info(data, GEN_ACQUISITIONCONFIG_T8, &size,
				    &obj_address);

		/* resume calibration must be performed with zero settings */
		atchcalst_tmp = 0;
		atchcalsthr_tmp = 0;

		/* atchcalst */
		ret = write_mem(data, obj_address + 6, 1,
				&atchcalst_tmp);
		/*atchcalsthr */
		ret |= write_mem(data, obj_address + 7, 1,
				&atchcalsthr_tmp);
		/* forced cal thr  */
		ret |= write_mem(data, obj_address + 8, 1,
				&atchcalst_tmp);
		/* forced cal thr ratio */
		ret |= write_mem(data, obj_address + 9, 1,
				&atchcalsthr_tmp);
	}

	/* send calibration command to the chip */
	if (!ret && !Doing_calibration_flag) {
		/* change calibration suspend settings to zero
		until calibration confirmed good */
		ret =
		    write_mem(data,
			      data->cmd_proc + CMD_CALIBRATE_OFFSET, 1,
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

static void mxt224_ta_probe(bool ta_status)
{
	u16 obj_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	u8 val = 0;
	unsigned int register_address = 7;
	u8 calcfg_en;
	u8 active_perX;

	printk(KERN_ERR "[TSP] mxt224_ta_probe\n");
	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR "[TSP] copy_data->mxt224_enabled is 0\n");
		return;
	}

	if (ta_status) {
		calcfg_en = copy_data->calcfg_charging_e | 0x20;

#ifdef CLEAR_MEDIAN_FILTER_ERROR
		copy_data->gErrCondition = ERR_RTN_CONDITION_MAX;
		copy_data->noise_median.mferr_setting = false;
#endif
		active_perX = 26;
	} else {
		calcfg_en = copy_data->calcfg_batt_e | 0x20;

#ifdef CLEAR_MEDIAN_FILTER_ERROR
		copy_data->gErrCondition = ERR_RTN_CONDITION_IDLE;
		copy_data->noise_median.mferr_count = 0;
		copy_data->noise_median.mferr_setting = false;
		copy_data->noise_median.median_on_flag = false;
#endif

#ifdef CLEAR_MEDIAN_FILTER_ERROR
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
#endif
		active_perX = 22;
	}

	/* value = copy_data->actvsyncsperx_e; */
	value = active_perX;
	ret =
	    get_object_info(copy_data, SPT_CTECONFIG_T46, &size_one,
			    &obj_address);
	write_mem(copy_data, obj_address + 3, 1, &value);

	ret =
	    get_object_info(copy_data, PROCG_NOISESUPPRESSION_T48,
			    &size_one, &obj_address);

	register_address = 2;
	size_one = 1;

	if (ta_status)
		write_config(copy_data, PROCG_NOISESUPPRESSION_T48,
			     copy_data->noise_suppression_cfg_ta);
	else
		write_config(copy_data, PROCG_NOISESUPPRESSION_T48,
			     copy_data->noise_suppression_cfg);
	value = calcfg_en;
	write_mem(copy_data, obj_address + (u16) register_address,
		  size_one, &value);
#if !defined(PRODUCT_SHIP)
	read_mem(copy_data, obj_address + (u16) register_address,
		 (u8) size_one, &val);
	printk(KERN_ERR "[TSP]TA_probe MXT224E T%d Byte%d is %d\n", 48,
	       register_address, val);
#endif
	copy_data->ta_status_pre = (bool) ta_status;
};

void check_chip_calibration(struct mxt224_data *data,
			unsigned char one_touch_input_flag)
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
	write_mem(data, data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1,
		  &data_byte);

	/* get the address of the diagnostic object
	so we can get the data we need */
	/* diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0); */
	ret =
	    get_object_info(data, DEBUG_DIAGNOSTIC_T37, &size,
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

		read_mem(data, object_address, 2, data_buffer);
	}

	/* data is ready - read the detection flags */
	/* read_mem(diag_address, 82,data_buffer); */
	read_mem(data, object_address, 82, data_buffer);

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

		printk(KERN_ERR "[TSP] t: %d, a: %d\n", tch_ch, atch_ch);

		/* send page up command so we can detect
		when data updates next time, page byte will sit at 1
		until we next send F3 command */
		data_byte = 0x01;

		write_mem(data,
			  data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1,
			  &data_byte);

		if ((tch_ch + atch_ch) > 21) {
			printk(KERN_ERR "[TSP]touch ch + anti ch > 21\n");
			calibrate_chip(data);
			qt_timer_state = 0;
			qt_time_point = jiffies_to_msecs(jiffies);
			not_yet_count = 0;
		} else if (tch_ch > 17) {
			printk(KERN_ERR "[TSP]touch ch > 17\n");
			calibrate_chip(data);
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
					    get_object_info(data,
						    GEN_ACQUISITIONCONFIG_T8,
						    &size,
						    &object_address);

				/* change calibration suspend settings to zero
				until calibration confirmed good */
				/* store normal settings */
					size = 1;

					write_mem(data,
					  object_address + 6, 1,
					  &data->atchcalst_e);
					write_mem(data,
					  object_address + 7, 1,
					  &data->atchcalsthr_e);
					write_mem(data,
					  object_address + 8, 1,
					  &data->atchfrccalthr_e);
					write_mem(data,
					  object_address + 9, 1,
					  &data->atchfrccalratio_e);

					if ((data->read_ta_status) &&
					(data->boot_or_resume == 1)) {
						data->boot_or_resume = 0;
						data->read_ta_status
						    (&ta_status_check);
						printk(KERN_ERR
						       "[TSP] ta_status is %d",
						       ta_status_check);
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
			calibrate_chip(data);
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
				calibrate_chip(data);
			}
		}
	}
}

#if defined(DRIVER_FILTER)
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

	data->info.family_id = id[0];
	data->info.variant_id = id[1];
	data->info.version = id[2];
	data->info.build = id[3];
	data->info.matrix_xsize = id[4];
	data->info.matrix_ysize = id[5];
	data->info.object_num = id[6];

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
			printk(KERN_ERR "[TSP] Up[%d]\n", i);
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

		if (data->touch_is_pressed_arr[i] == 1)
			check_press = true;

		if (data->g_debug_switch)
			printk(KERN_ERR "[TSP] ID-%d, %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);

		if (data->touch_is_pressed_arr[i] != 0)
			touch_is_pressed = 1;

		/* logging */
#ifdef __TSP_DEBUG
		if (data->touch_is_pressed_arr[i] == 1)
			printk(KERN_ERR "[TSP] Dn[%d] %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);
		if (data->touch_is_pressed_arr[i] == 2)
			printk(KERN_ERR "[TSP] Mv[%d] %4d,%4d\n", i,
			       data->fingers[i].x, data->fingers[i].y);
#else
		if (data->touch_is_pressed_arr[i] == 1) {
			printk(KERN_ERR "[TSP] Dn[%d]\n", i);
			data->touch_is_pressed_arr[i] = 2;
		}
#endif
	}
	data->finger_mask = 0;
	data->touch_state = 0;
	input_sync(data->input_dev);

	for (i = 0; i < data->num_fingers; i++) {
		if (TSP_STATE_INACTIVE != data->fingers[i].z) {
			tsp_state = true;
			break;
		}
	}

	if (!tsp_state && data->lock_status) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		data->lock_status = 0;
	} else if ((data->lock_status == 0) && check_press) {
		if (level != ~0) {
			exynos_cpufreq_lock(
				DVFS_LOCK_ID_TSP,
				level);
			data->lock_status = 1;
		}
	}
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
				value = 32;
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
				value = 100;	/* mfinvlddiffthr */
				write_mem(copy_data, obj_address + 22, 1,
					  &value);
				value = 100;	/* mferrorthr */
				write_mem(copy_data, obj_address + 25, 1,
					  &value);
				value = 40;	/* thr */
				write_mem(copy_data, obj_address + 35, 1,
					  &value);
				value = 65;	/* movfilter */
				write_mem(copy_data, obj_address + 39, 1,
					  &value);
				ret |=
				    get_object_info(copy_data,
						    SPT_CTECONFIG_T46,
						    &size_one, &obj_address);
				value = 48;	/* actvsyncsperx */
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
		}

#ifdef CLEAR_MEDIAN_FILTER_ERROR
		if (msg[0] == 18) {
			if ((msg[4] & 0x5) == 0x5) {
				printk(KERN_ERR
					"[TSP] median filter state error!!!\n");
				median_err_setting();
			} else if ((msg[4] & 0x4) == 0x4) {
				data->read_ta_status(&ta_status_check);
				if ((!ta_status_check) &&
					(data->noise_median.mferr_setting
						== false) &&
					(data->noise_median.median_on_flag
						== false)) {
					printk(KERN_ERR
						"[TSP] median filter ON!!!\n");
					ret =
						get_object_info(data,
							TOUCH_MULTITOUCHSCREEN_T9,
								&size_one,
								&obj_address);
					value = 0;
					write_mem(data, obj_address + 34,
						1, &value);
					data->noise_median.median_on_flag
						= true;
				}
			}
		}
#endif
		if (msg[0] > 1 && msg[0] < 12) {

			if ((data->touch_is_pressed_arr[msg[0] - 2] == 1)
			    && (msg[1] & 0x40)) {
				printk(KERN_ERR
				       "[TSP] Calibrate on Ghost touch");
				calibrate_chip(data);
			data->touch_is_pressed_arr[msg[0] - 2] = 0;
			}

			if ((msg[1] & 0x20) == 0x20) {	/* Release */
			/* touch_is_pressed = 0; */
			/* data->touch_is_pressed_arr[msg[0]-2] = 0; */

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
			data->touch_is_pressed_arr[msg[0] - 2] = 0;
				data->touch_state = 1;
			} else if ((msg[1] & DETECT_MSG_MASK)
				   && (msg[1] &
				       (PRESS_MSG_MASK | MOVE_MSG_MASK))) {
				if (msg[1] & PRESS_MSG_MASK)
					data->touch_is_pressed_arr[id] =
					TSP_STATE_PRESS;
				else if (msg[1] & MOVE_MSG_MASK)
					data->touch_is_pressed_arr[id] =
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
				data->touch_is_pressed_arr[id] =
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
			check_chip_calibration(data, 1);
	} while (!gpio_get_value(data->gpio_read_done));

	if ((!data->optiacl_gain) && (data->family_id != 0x81)) {
		mxt224_optical_gain(MXT_DIAG_REFERENCE_MODE);
		data->optiacl_gain = 1;
	}

	return IRQ_HANDLED;
}

static int mxt224_internal_suspend(struct mxt224_data *data)
{
	int i;
	int ret = 0;

	if (!data->mxt224_enabled) {
		printk(KERN_ERR"[TSP] already %s state.\n", __func__);
		return ret;
	}

	disable_irq(data->client->irq);
	ret = data->power_off();

	data->mxt224_enabled = 0;
	touch_is_pressed = 0;
	qt_timer_state = 0;
	not_yet_count = 0;
	Doing_calibration_flag = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].z == -1)
			continue;

		data->touch_is_pressed_arr[i] = 0;
		data->fingers[i].z = 0;
	}
	report_input_data(data);

	return ret;
}

static int mxt224_internal_resume(struct mxt224_data *data)
{
	int ret = 0;
	bool ta_status = 0;

	if (data->mxt224_enabled) {
		printk(KERN_ERR"[TSP] already %s state.\n", __func__);
		return ret;
	}

	ret = data->power_on();

	enable_irq(data->client->irq);
	data->mxt224_enabled = 1;
	data->boot_or_resume = 1;

#ifdef CLEAR_MEDIAN_FILTER_ERROR
	data->noise_median.mferr_count = 0;
	data->noise_median.mferr_setting = false;
	data->noise_median.median_on_flag = false;
#endif

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
	calibrate_chip(data);

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt224_suspend	NULL
#define mxt224_resume	NULL

static void mxt224_early_suspend(struct early_suspend *h)
{
	struct mxt224_data *data = container_of(h, struct mxt224_data,
						early_suspend);

	mxt224_internal_suspend(data);

	dev_err(&data->client->dev, "%s\n", __func__);
}

static void mxt224_late_resume(struct early_suspend *h)
{
	struct mxt224_data *data = container_of(h, struct mxt224_data,
						early_suspend);

	mxt224_internal_resume(data);

	dev_err(&data->client->dev, "%s\n", __func__);
}
#else
static int mxt224_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);
	int ret = 0;

	ret = mxt224_internal_suspend(data);

	dev_err(&client->dev, "%s\n", __func__);

	return ret;
}

static int mxt224_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);
	int ret = 0;

	ret = mxt224_internal_resume(data);

	dev_err(&client->dev, "%s\n", __func__);

	return ret;
}
#endif

void Mxt224_force_released(void)
{
	if (!copy_data->mxt224_enabled) {
		printk(KERN_ERR "[TSP] copy_data->mxt224_enabled is 0\n");
		return;
	}

	calibrate_chip(copy_data);

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

static ssize_t mxt224_object_setting(struct device *dev,
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

static ssize_t mxt224_object_show(struct device *dev,
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

struct device *sec_touchscreen;

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
	diagnostic_chip(MXT_DIAG_CTE_MODE);
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

			if (copy_data->family_id == 0x81)
				qt_refrence = qt_refrence - 16384;
			if (qt_refrence > 14500)
				reference_over = 1;

			if ((read_page == 3) && (node == 16))
				break;
		}
		diagnostic_chip(MXT_DIAG_PAGE_UP);
		msleep(20);
	}

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
	case MXT_WAITING_BOOTLOAD_CMD:
	case MXT_WAITING_FRAME_DATA:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
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

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

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

static DEVICE_ATTR(object_show, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   mxt224_object_show);
static DEVICE_ATTR(object_write, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   mxt224_object_setting);
static DEVICE_ATTR(dbg_switch, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   mxt224_debug_setting);

/* FACTORY_TEST_SUPPORT */

static void set_default_result(struct mxt_fac_data *data)
{
	char delim = ':';

	memset(data->cmd_result, 0x00, ARRAY_SIZE(data->cmd_result));
	memcpy(data->cmd_result, data->cmd, strlen(data->cmd));
	strncat(data->cmd_result, &delim, 1);
}

static void set_cmd_result(struct mxt_fac_data *data, char *buff, int len)
{
	strncat(data->cmd_result, buff, len);
}

static void not_support_cmd(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};

	set_default_result(fdata);
	sprintf(buff, "%s", "NA");
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_NOT_APPLICABLE;
	dev_info(&client->dev, "%s: \"%s(%d)\"\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static bool mxt_check_xy_range(struct mxt224_data *data, u16 node)
{
	u8 x_line = node / data->info.matrix_ysize;
	u8 y_line = node % data->info.matrix_ysize;
	return (y_line < data->fdata->num_ynode) ?
		(x_line < data->fdata->num_xnode) : false;
}

/* +  Vendor specific helper functions */
static int mxt_xy_to_node(struct mxt224_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};
	int node;

	/* cmd_param[0][1] : [x][y] */
	if (fdata->cmd_param[0] < 0
		|| fdata->cmd_param[0] >= data->fdata->num_xnode
		|| fdata->cmd_param[1] < 0
		|| fdata->cmd_param[1] >= data->fdata->num_ynode) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;

		dev_info(&client->dev, "%s: parameter error: %u,%u\n",
			__func__, fdata->cmd_param[0], fdata->cmd_param[1]);
		return -EINVAL;
	}

	/*
	 * maybe need to consider orient.
	 *   --> y number
	 *  |(0,0) (0,1)
	 *  |(1,0) (1,1)
	 *  v
	 *  x number
	 */
	node = fdata->cmd_param[0] * data->fdata->num_ynode
			+ fdata->cmd_param[1];

	dev_info(&client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static void mxt_node_to_xy(struct mxt224_data *data, u16 *x, u16 *y)
{
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	*x = fdata->delta_max_node / data->fdata->num_ynode;
	*y = fdata->delta_max_node % data->fdata->num_ynode;

	dev_info(&client->dev, "%s: node[%d] is X,Y=%d,%d\n",
		__func__, fdata->delta_max_node, *x, *y);
}

static int mxt_set_diagnostic_mode(struct mxt224_data *data, u8 dbg_mode)
{
	struct i2c_client *client = data->client;
	u8 cur_mode;
	int ret;

	ret = mxt_write_object(data, GEN_COMMANDPROCESSOR_T6,
			CMD_DIAGNOSTIC_OFFSET, dbg_mode);

	if (ret) {
		dev_err(&client->dev, "Failed change diagnositc mode to %d\n",
			 dbg_mode);
		goto out;
	}

	if (dbg_mode & MXT_DIAG_MODE_MASK) {
		do {
			ret = mxt_read_object(data, DEBUG_DIAGNOSTIC_T37,
				MXT_DIAGNOSTIC_MODE, &cur_mode);
			if (ret) {
				dev_err(&client->dev, "Failed getting diagnositc mode\n");
				goto out;
			}
			msleep(20);
		} while (cur_mode != dbg_mode);
		dev_dbg(&client->dev, "current dianostic chip mode is %d\n",
			 cur_mode);
	}

out:
	return ret;
}

static void mxt_treat_dbg_data(struct mxt224_data *data,
	u16 address, u8 dbg_mode, u8 read_point, u16 num)
{
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
	u8 data_buffer[DATA_PER_NODE] = { 0 };
	int xnode, ynode = 0;

	xnode = num / data->fdata->num_ynode;
	ynode = num % data->fdata->num_ynode;

	if (dbg_mode == MXT_DIAG_DELTA_MODE) {
		/* read delta data */
		read_mem(data, address + read_point,
			DATA_PER_NODE, data_buffer);

		fdata->delta[num] =
			((u16)data_buffer[1]<<8) + (u16)data_buffer[0];

		dev_dbg(&client->dev, "delta[%d] = %d\n",
			num, fdata->delta[num]);

		if (abs(fdata->delta[num])
			> abs(fdata->delta_max_data)) {
			fdata->delta_max_node = num;
			fdata->delta_max_data = fdata->delta[num];
		}
	} else if (dbg_mode == MXT_DIAG_REFERENCE_MODE) {
		/* read reference data */
		read_mem(data, address + read_point,
			DATA_PER_NODE, data_buffer);

		fdata->reference[num] =
			((u16)data_buffer[1] << 8) + (u16)data_buffer[0]
			- REF_OFFSET_VALUE;

		/* check that reference is in spec or not */
		if (fdata->reference[num] < REF_MIN_VALUE
			|| fdata->reference[num] > REF_MAX_VALUE) {
			dev_err(&client->dev, "reference[%d] is out of range = %d(%d,%d)\n",
				num, fdata->reference[num], xnode, ynode);
		}
		if (fdata->reference[num] > fdata->ref_max_data)
			fdata->ref_max_data = fdata->reference[num];
		if (fdata->reference[num] < fdata->ref_min_data)
			fdata->ref_min_data =	fdata->reference[num];

		dev_dbg(&client->dev, "reference[%d] = %d\n",
				num, fdata->reference[num]);
	}
}

static int mxt_read_all_diagnostic_data(struct mxt224_data *data, u8 dbg_mode)
{
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
	u16 size;
	u16 address;
	u8 read_page, cur_page, end_page, read_point;
	u16 node, num = 0,  cnt = 0;
	int ret = 0;

	/* to make the Page Num to 0 */
	ret = mxt_set_diagnostic_mode(data, MXT_DIAG_CTE_MODE);
	if (ret)
		goto out;

	/* change the debug mode */
	ret = mxt_set_diagnostic_mode(data, dbg_mode);
	if (ret)
		goto out;

	/* get object info for diagnostic */
	ret = get_object_info(data, DEBUG_DIAGNOSTIC_T37, &size,
		&address);
	if (ret) {
		dev_err(&client->dev, "fail to get object_info\n");
		ret = -EINVAL;
		goto out;
	}

	fdata->ref_min_data = REF_MAX_VALUE;
	fdata->ref_max_data = REF_MIN_VALUE;
	fdata->delta_max_data = 0;
	fdata->delta_max_node = 0;
	end_page = (data->info.matrix_xsize * data->info.matrix_ysize)
				/ NODE_PER_PAGE + 1;

	/* read the dbg data */
	for (read_page = 0 ; read_page < end_page; read_page++) {
		for (node = 0; node < NODE_PER_PAGE; node++) {
			read_point = (node * DATA_PER_NODE) + 2;

			if (mxt_check_xy_range(data, cnt)) {
				mxt_treat_dbg_data(data, address, dbg_mode,
					read_point, num);
				num++;
			}
			if (num == 198)
				break;
			cnt++;
		}
		if (num == 198)
			break;
		ret = mxt_set_diagnostic_mode(data, MXT_DIAG_PAGE_UP);
		if (ret)
			goto out;
		do {
			msleep(20);
			ret = read_mem(data,
				address + MXT_DIAGNOSTIC_PAGE,
				1, &cur_page);
			if (ret) {
				dev_err(&client->dev,
					"%s Read fail page\n", __func__);
				goto out;
			}
		} while (cur_page != read_page + 1);
	}

	if (dbg_mode == MXT_DIAG_REFERENCE_MODE) {
		dev_info(&client->dev, "min/max reference is [%d/%d]\n",
			fdata->ref_min_data, fdata->ref_max_data);
	} else if (dbg_mode == MXT_DIAG_DELTA_MODE) {
		dev_info(&client->dev, "max delta node %d=[%d]\n",
			fdata->delta_max_node, fdata->delta_max_data);
	}
out:
	return ret;
}

/*
 * find the x,y position to use maximum delta.
 * it is diffult to map the orientation and caculate the node number
 * because layout is always different according to device
 */
static void find_delta_node(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct mxt_fac_data *fdata = data->fdata;
	char buff[16] = {0};
	u16 x, y;
	int ret;

	set_default_result(fdata);

	/* read all delta to get the maximum delta value */
	ret = mxt_read_all_diagnostic_data(data,
			MXT_DIAG_DELTA_MODE);
	if (ret) {
		fdata->cmd_state = CMD_STATUS_FAIL;
	} else {
		mxt_node_to_xy(data, &x, &y);
		snprintf(buff, sizeof(buff), "%d,%d", x, y);
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));

		fdata->cmd_state = CMD_STATUS_OK;
	}
}


static int mxt_load_fw_from_ums(struct mxt_fw_info *fw_info,
		const u8 *fw_data)
{
	struct mxt224_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	struct file *filp = NULL;
	struct firmware fw;
	mm_segment_t old_fs = {0};
	const char *firmware_name =
		data->pdata->firmware_name ?: MXT224E_FW_NAME;
	char *fw_path;
	int ret = 0;

	memset(&fw, 0, sizeof(struct firmware));

	fw_path = kzalloc(MXT_MAX_FW_PATH, GFP_KERNEL);
	if (fw_path == NULL) {
		dev_err(dev, "Failed to allocate firmware path.\n");
		return -ENOMEM;
	}

	snprintf(fw_path, MXT_MAX_FW_PATH, "/sdcard/%s", firmware_name);

	old_fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(fw_path, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		dev_err(dev, "Could not open firmware: %s,%d\n",
			fw_path, (s32)filp);
		ret = -ENOENT;
		goto err_open;
	}

	fw.size = filp->f_path.dentry->d_inode->i_size;

	fw_data = kzalloc(fw.size, GFP_KERNEL);
	if (!fw_data) {
		dev_err(dev, "Failed to alloc buffer for fw\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	ret = vfs_read(filp, (char __user *)fw_data, fw.size, &filp->f_pos);
	if (ret != fw.size) {
		dev_err(dev, "Failed to read file %s (ret = %d)\n",
			fw_path, ret);
		ret = -EINVAL;
		goto err_alloc;
	}
	fw.data = fw_data;
	fw_info->fw_len = fw.size;
	fw_info->fw_raw_data = fw.data;
err_alloc:
	filp_close(filp, current->files);
err_open:
	set_fs(old_fs);
	kfree(fw_path);

	return ret;
}

static int mxt_load_fw_from_req_fw(struct mxt_fw_info *fw_info,
		const struct firmware *fw)
{
	struct mxt224_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	const char *firmware_name =
		data->pdata->firmware_name ?: MXT224E_FW_NAME;
	int ret = 0;
	char fw_path[MXT_MAX_FW_PATH];

	memset(&fw_path, 0, MXT_MAX_FW_PATH);

	snprintf(fw_path, MXT_MAX_FW_PATH, "%s%s",
		MXT_FIRMWARE_INKERNEL_PATH, firmware_name);

	dev_err(dev, "%s\n", fw_path);

	ret = request_firmware(&fw, fw_path, dev);
	if (ret) {
		dev_err(dev,
			"Could not request firmware %s\n", fw_path);
		goto out;
	}
	fw_info->fw_len = fw->size;
	fw_info->fw_raw_data = fw->data;

out:
	return ret;
}

static int mxt_flash_fw(struct mxt_fw_info *fw_info)
{
	struct mxt224_data *data = fw_info->data;
	struct i2c_client *client = data->client;
	struct device *dev = &data->client->dev;
	const u8 *fw_data = fw_info->fw_raw_data;
	size_t fw_size = fw_info->fw_len;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	if (!fw_data) {
		dev_err(dev, "firmware data is Null\n");
		return -ENOMEM;
	}

	/* Change to slave address of bootloader */
	if (client->addr == MXT224_APP_LOW)
		client->addr = MXT224_BOOT_LOW;
	else
		client->addr = MXT224_BOOT_HIGH;

	ret = mxt224_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt224_unlock_bootloader(client);

	while (pos < fw_size) {
		ret = mxt224_check_bootloader(client,
				 MXT_WAITING_FRAME_DATA);
		if (ret) {
			dev_err(dev, "Fail updating firmware. wating_frame_data err\n");
			goto out;
		}

		frame_size = ((*(fw_data + pos) << 8) | *(fw_data + pos + 1));

		/*
		* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/

		frame_size += 2;

		/* Write one frame to device */
		mxt224_fw_write(client, fw_data + pos, frame_size);

		ret = mxt224_check_bootloader(client, MXT_FRAME_CRC_PASS);
		if (ret) {
			dev_err(dev, "Fail updating firmware. frame_crc err\n");
			goto out;
		}

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n",
				pos, fw_size);

		msleep(20);
	}

	/* Change to slave address of application */
	if (client->addr == MXT224_BOOT_LOW)
		client->addr = MXT224_APP_LOW;
	else
		client->addr = MXT224_APP_HIGH;

	dev_info(dev, "success updating firmware\n");
out:
	return ret;
}

static void fw_update(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct device *dev = &data->client->dev;
	struct mxt_fac_data *fdata = data->fdata;
	struct mxt_fw_info fw_info;
	const u8 *fw_data = NULL;
	const struct firmware *fw = NULL;
	int ret = 0;
	u16 size;
	u16 address;
	u8 value;
	char buff[16] = {0};

	memset(&fw_info, 0, sizeof(struct mxt_fw_info));
	fw_info.data = data;

	set_default_result(fdata);
	switch (fdata->cmd_param[0]) {

	case MXT_FW_FROM_UMS:
		ret = mxt_load_fw_from_ums(&fw_info, fw_data);
		if (ret)
			goto out;
	break;

	case MXT_FW_FROM_BUILT_IN:
	case MXT_FW_FROM_REQ_FW:
		ret = mxt_load_fw_from_req_fw(&fw_info, fw);
		if (ret)
			goto out;
	break;

	default:
		dev_err(dev, "invalid fw file type!!\n");
		ret = -EINVAL;
		goto out;
	}

	ret = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (ret <= 0) {
		dev_err(dev, "error while waiting for device to init (%d)\n",
			ret);
		ret = -EBUSY;
		goto out;
	}

	mutex_lock(&data->input_dev->mutex);
	disable_irq(data->client->irq);

	/* Change to the bootloader mode */
	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6,
			&size, &address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		goto irq_enable;
	}

	value = (u8) MXT_BOOT_VALUE;
	write_mem(data, address, 1,&value);
	msleep(MXT224_RESET_TIME);

	ret = mxt_flash_fw(&fw_info);
	if (ret) {
		dev_err(dev, "The firmware update failed(%d)\n", ret);
	} else {
		dev_info(dev, "The firmware update succeeded\n");
		kfree(data->objects);
		data->objects = NULL;

		/* Wait for reset */
		msleep(MXT224_FWRESET_TIME);
		/* initialize the TSP */
		mxt224_init_touch_driver(data);
	}

irq_enable:
	enable_irq(data->client->irq);
	mutex_unlock(&data->input_dev->mutex);

	ret = mxt224_backup(data);
	if (ret) {
		printk(KERN_ERR "[TSP] mxt224_backup fail!!!\n");
		goto out;
	}

	/* reset the touch IC. */
	ret = mxt224_reset(data);
	if (ret) {
		printk(KERN_ERR "[TSP] mxt224_reset fail!!!\n");
		goto out;
	}
	msleep(60);
out:
	release_firmware(fw);
	kfree(fw_data);

	if (ret) {
		snprintf(buff, sizeof(buff), "FAIL");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_OK;
	}

	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct device *dev = &data->client->dev;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[40] = {0};

	set_default_result(fdata);

	if (!fdata->fw_ver && !fdata->build_ver) {
		snprintf(buff, sizeof(buff), "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;
	} else {
		/* Format : IC vendor + H/W verion of IC + F/W version
		 * ex) Atmel : AT00201B
		 */
			snprintf(buff, sizeof(buff), "AT00%02x%02x",
			fdata->fw_ver, fdata->build_ver);
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_OK;
	}
	dev_info(dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[40] = {0};
	int ver, build;

	set_default_result(fdata);

	ver = data->info.version;
	build = data->info.build;

	/* Format : IC vendor + H/W verion of IC + F/W version
	 * ex) Atmel : AT00201B
	 */
	snprintf(buff, sizeof(buff), "AT00%02x%02x", ver, build);

	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;
	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
#if 0
	struct mxt_object *user_object;

	char buff[50] = {0};
	char date[10] = {0};
	int error = 0;
	u32 current_crc = 0;

	set_default_result(fdata);

	/* Get the config version from userdata */
	user_object = mxt_get_object(data, MXT_SPT_USERDATA_T38);
	if (!user_object) {
		dev_err(&client->dev, "fail to get object_info\n");
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	mxt_read_mem(data, user_object->start_address,
			MXT_CONFIG_VERSION_LENGTH, date);

	disable_irq(data->client->irq);
	/* Get config CRC from device */
	error = mxt_read_config_crc(data, &current_crc);
	if (error)
		dev_err(&client->dev, "%s Error getting configuration CRC:%d\n",
			__func__, error);
	enable_irq(data->client->irq);

	/* Model number_vendor_date_CRC value */
#if 1	/* Need to be removed hard cording in date */
	snprintf(buff, sizeof(buff), "%s_AT_1121_0x%06X",
		data->pdata->project_name, current_crc);
#else
	snprintf(buff, sizeof(buff), "%s-%s-0x%06X",
		data->pdata->project_name, date, current_crc);
#endif

#else
	char buff[50] = {0};

	set_default_result(fdata);

	snprintf(buff, sizeof(buff), "I217_AT_0308");
#endif

	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;
	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_threshold(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
	int error;

	char buff[16] = {0};
	u8 threshold = 0;

	set_default_result(fdata);

	error = mxt_read_object(data, TOUCH_MULTITOUCHSCREEN_T9,
		 7, &threshold);
	if (error) {
		dev_err(&client->dev, "Failed get the threshold\n");
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "%d", threshold);

	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;
	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void module_off_master(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[3] = {0};

	mutex_lock(&data->input_dev->mutex);

	if (mxt224_internal_suspend(data))
		snprintf(buff, sizeof(buff), "%s", "NG");
	else
		snprintf(buff, sizeof(buff), "%s", "OK");

	mutex_unlock(&data->input_dev->mutex);

	set_default_result(fdata);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		fdata->cmd_state = CMD_STATUS_OK;
	else
		fdata->cmd_state = CMD_STATUS_FAIL;

	dev_info(&client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[3] = {0};

	mutex_lock(&data->input_dev->mutex);

	if (mxt224_internal_resume(data))
		snprintf(buff, sizeof(buff), "%s", "NG");
	else
		snprintf(buff, sizeof(buff), "%s", "OK");

	mutex_unlock(&data->input_dev->mutex);

	set_default_result(fdata);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		fdata->cmd_state = CMD_STATUS_OK;
	else
		fdata->cmd_state = CMD_STATUS_FAIL;

	dev_info(&client->dev, "%s: %s\n", __func__, buff);

}

static void get_chip_vendor(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};

	set_default_result(fdata);

	snprintf(buff, sizeof(buff), "%s", "ATMEL");
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;
	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};

	set_default_result(fdata);

	snprintf(buff, sizeof(buff), "%s", MXT224E_DEV_NAME);

	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;
	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_x_num(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};
	int val;

	set_default_result(fdata);
	val = fdata->num_xnode;
	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;

		dev_info(&client->dev, "%s: fail to read num of x (%d).\n",
			__func__, val);

		return;
	}
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;

	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};
	int val;

	set_default_result(fdata);
	val = fdata->num_ynode;
	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = CMD_STATUS_FAIL;

		dev_info(&client->dev, "%s: fail to read num of y (%d).\n",
			__func__, val);

		return;
	}
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;

	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void run_reference_read(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct mxt_fac_data *fdata = data->fdata;
	int ret;
	char buff[16] = {0};

	set_default_result(fdata);

	ret = mxt_read_all_diagnostic_data(data,
			MXT_DIAG_REFERENCE_MODE);
	if (ret)
		fdata->cmd_state = CMD_STATUS_FAIL;
	else {
		snprintf(buff, sizeof(buff), "%d,%d",
			fdata->ref_min_data, fdata->ref_max_data);
		set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));

		fdata->cmd_state = CMD_STATUS_OK;
	}
}

static void get_reference(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
	char buff[16] = {0};
	int node;

	set_default_result(fdata);
	/* add read function */

	node = mxt_xy_to_node(data);
	if (node < 0) {
		fdata->cmd_state = CMD_STATUS_FAIL;
		return;
	}
	snprintf(buff, sizeof(buff), "%u", fdata->reference[node]);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;

	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

static void run_delta_read(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct mxt_fac_data *fdata = data->fdata;
	int ret;

	set_default_result(fdata);

	ret = mxt_read_all_diagnostic_data(data,
			MXT_DIAG_DELTA_MODE);
	if (ret)
		fdata->cmd_state = CMD_STATUS_FAIL;
	else
		fdata->cmd_state = CMD_STATUS_OK;
}

static void get_delta(void *device_data)
{
	struct mxt224_data *data = (struct mxt224_data *)device_data;
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;
	char buff[16] = {0};
	int node;

	set_default_result(fdata);
	/* add read function */

	node = mxt_xy_to_node(data);
	if (node < 0) {
		fdata->cmd_state = CMD_STATUS_FAIL;
		return;
	}
	snprintf(buff, sizeof(buff), "%d", fdata->delta[node]);
	set_cmd_result(fdata, buff, strnlen(buff, sizeof(buff)));
	fdata->cmd_state = CMD_STATUS_OK;

	dev_info(&client->dev, "%s: %s(%d)\n",
		__func__, buff, strnlen(buff, sizeof(buff)));
}

#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

struct tsp_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

static struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", not_support_cmd),},
	{TSP_CMD("module_on_slave", not_support_cmd),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("run_delta_read", run_delta_read),},
	{TSP_CMD("get_delta", get_delta),},
	{TSP_CMD("find_delta", find_delta_node),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};

/* Functions related to basic interface */
static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct mxt224_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	struct mxt_fac_data *fdata = data->fdata;

	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (fdata->cmd_is_running == true) {
		dev_err(&client->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&fdata->cmd_lock);
	fdata->cmd_is_running = true;
	mutex_unlock(&fdata->cmd_lock);

	fdata->cmd_state = CMD_STATUS_RUNNING;

	for (i = 0; i < ARRAY_SIZE(fdata->cmd_param); i++)
		fdata->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(fdata->cmd, 0x00, ARRAY_SIZE(fdata->cmd));
	memcpy(fdata->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &fdata->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr,
			&fdata->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				ret = kstrtoint(buff, 10,\
					fdata->cmd_param + param_cnt);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}

	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n",
			i, fdata->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(data);
err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mxt224_data *data = dev_get_drvdata(dev);
	struct mxt_fac_data *fdata = data->fdata;

	char buff[16] = {0};

	dev_info(&data->client->dev, "tsp cmd: status:%d\n",
			fdata->cmd_state);

	if (fdata->cmd_state == CMD_STATUS_WAITING)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (fdata->cmd_state == CMD_STATUS_RUNNING)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (fdata->cmd_state == CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");

	else if (fdata->cmd_state == CMD_STATUS_FAIL)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (fdata->cmd_state == CMD_STATUS_NOT_APPLICABLE)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct mxt224_data *data = dev_get_drvdata(dev);
	struct mxt_fac_data *fdata = data->fdata;

	dev_info(&data->client->dev, "tsp cmd: result: %s\n",
		fdata->cmd_result);

	mutex_lock(&fdata->cmd_lock);
	fdata->cmd_is_running = false;
	mutex_unlock(&fdata->cmd_lock);

	fdata->cmd_state = CMD_STATUS_WAITING;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", fdata->cmd_result);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);

static struct attribute *touchscreen_factory_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group touchscreen_factory_attr_group = {
	.attrs = touchscreen_factory_attributes,
};

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
	int ret = 0;
#if 1
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
#else
	mutex_lock(&data->input_dev->mutex);

	ret = mxt224_internal_resume(data);

	mutex_unlock(&data->input_dev->mutex);
#endif

	return ret;
}

static void sec_touchscreen_close(struct input_dev *dev)
{
	struct mxt224_data *data = input_get_drvdata(dev);
#if 1
	sec_touchscreen_disable(data);
#else
	mutex_lock(&data->input_dev->mutex);

	mxt224_internal_suspend(data);

	mutex_unlock(&data->input_dev->mutex);
#endif
}

static struct attribute *mxt224_attrs[] = {
	&dev_attr_object_show.attr,
	&dev_attr_object_write.attr,
	&dev_attr_dbg_switch.attr,
	NULL
};

static const struct attribute_group mxt224_attr_group = {
	.attrs = mxt224_attrs,
};

static void mxt_deallocate_factory(struct mxt224_data *data)
{
	struct mxt_fac_data *fdata = data->fdata;

	kfree(fdata->delta);
	kfree(fdata->reference);
	kfree(fdata);
}

static int mxt_allocate_factory(struct mxt224_data *data)
{
	const struct mxt224_platform_data *pdata = data->pdata;
	struct mxt_fac_data *fdata = NULL;
	struct device *dev = &data->client->dev;
	int error = 0;
	int i;

	fdata = kzalloc(sizeof(struct mxt_fac_data), GFP_KERNEL);
	if (fdata == NULL) {
		dev_err(dev, "Fail to allocate sysfs data.\n");
		return -ENOMEM;
	}

	if (!pdata->num_xnode || !pdata->num_ynode) {
		dev_err(dev, "%s num of x,y is 0\n", __func__);
		error =  -EINVAL;
		goto err_get_num_node;
	}

	fdata->fw_ver = MXT224E_FW_VER;
	fdata->build_ver = MXT224E_BUILD_VER;

	fdata->num_xnode = pdata->num_xnode;
	fdata->num_ynode = pdata->num_ynode;
	fdata->num_nodes = fdata->num_xnode * fdata->num_ynode;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	dev_info(dev, "%s: x=%d, y=%d, total=%d\n",
		__func__, fdata->num_xnode,
		fdata->num_ynode, fdata->num_nodes);
#endif

	fdata->reference = kzalloc(fdata->num_nodes * sizeof(u16),
				   GFP_KERNEL);
	if (!fdata->reference) {
		dev_err(dev, "Fail to alloc reference of fdata\n");
		error = -ENOMEM;
		goto err_alloc_reference;
	}

	fdata->delta = kzalloc(fdata->num_nodes * sizeof(s16), GFP_KERNEL);
	if (!fdata->delta) {
		dev_err(dev, "Fail to alloc delta of fdata\n");
		error = -ENOMEM;
		goto err_alloc_delta;
	}

	INIT_LIST_HEAD(&fdata->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &fdata->cmd_list_head);

	mutex_init(&fdata->cmd_lock);
	fdata->cmd_is_running = false;

	data->fdata = fdata;

	return error;

err_alloc_delta:
err_alloc_reference:
err_get_num_node:

	mxt_deallocate_factory(data);

	return error;
}

static int mxt_init_factory(struct mxt224_data *data)
{
	struct device *dev = &data->client->dev;
	int error;

	error = mxt_allocate_factory(data);
	if (error)
		return -ENOMEM;

	data->fdata->fac_dev_ts = device_create(sec_class,
			NULL, 0, data, "tsp");
	if (IS_ERR(data->fdata->fac_dev_ts)) {
		dev_err(dev, "Failed to create device for the sysfs\n");
		error = IS_ERR(data->fdata->fac_dev_ts);
		goto err_create_device;
	}

	error = sysfs_create_group(&data->fdata->fac_dev_ts->kobj,
		&touchscreen_factory_attr_group);
	if (error) {
		dev_err(dev, "Failed to create touchscreen sysfs group\n");
		goto err_create_group;
	}

	return 0;

err_create_group:
err_create_device:
	mxt_deallocate_factory(data);

	return error;
}

static void mxt_exit_factory(struct mxt224_data *data)
{
	struct mxt_fac_data *fdata = data->fdata;

	sysfs_remove_group(&fdata->fac_dev_ts->kobj,
		&touchscreen_factory_attr_group);
	mxt_deallocate_factory(data);
}

static int mxt_sysfs_init(struct i2c_client *client)
{
	struct mxt224_data *data = i2c_get_clientdata(client);
	int ret;

	sec_touchscreen =
	    device_create(sec_class, NULL, 0, NULL, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen))
		printk(KERN_ERR
		       "[TSP] Failed to create device(sec_touchscreen)!\n");

	ret = sysfs_create_group(&client->dev.kobj, &mxt224_attr_group);
	if (ret)
		printk(KERN_ERR "[TSP] sysfs_create_group is falled\n");

	ret = mxt_init_factory(data);
	if (ret)
		printk(KERN_ERR "[TSP] mxt_init_factory is falled\n");

	return 0;
}

static void  mxt_sysfs_remove(struct mxt224_data *data)
{
	sysfs_remove_group(&data->client->dev.kobj, &mxt224_attr_group);

	mxt_exit_factory(data);
}


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
	data->pdata = pdata;
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
	data->tsp_config_version = "20130220";

	copy_data = data;

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

	calibrate_chip(data);

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

	ret = request_threaded_irq(client->irq, NULL, mxt224_irq_thread,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt224_ts",
			data);

	if (ret < 0)
		goto err_irq;

	disable_irq(client->irq);
	complete_all(&data->init_done);

	ret = mxt_sysfs_init(client);
	if (ret < 0)
		goto err_sysfs_init;

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt224_early_suspend;
	data->early_suspend.resume = mxt224_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	return 0;

 err_sysfs_init:
	free_irq(client->irq, data); 
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
	input_unregister_device(data->input_dev);
	mxt_sysfs_remove(data);
	gpio_free(data->gpio_read_done);
	data->power_off();
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt224_idtable[] = {
	{MXT224E_DEV_NAME, 0},
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
		   .name = MXT224E_DEV_NAME,
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

MODULE_DESCRIPTION("Atmel MaXTouch 224e driver");
MODULE_LICENSE("GPL");
