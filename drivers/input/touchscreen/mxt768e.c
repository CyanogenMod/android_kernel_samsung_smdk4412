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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt768e.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/string.h>

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
#define AMPLITUDE_MSG_MASK	0x04
#define SUPPRESS_MSG_MASK		0x02

/* Slave addresses */
#define MXT_APP_LOW		0x4c
#define MXT_APP_HIGH		0x4d
#define MXT_BOOT_LOW		0x26
#define MXT_BOOT_HIGH		0x27

/* FIRMWARE NAME */
#define MXT_FW_NAME			"tsp_atmel/mXT768E.fw"
#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE		0x55

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL		0x03
#define MXT_FRAME_CRC_PASS		0x04
#define MXT_APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB		0xaa
#define MXT_UNLOCK_CMD_LSB		0xdc

#define ID_BLOCK_SIZE			7

#define MXT_STATE_INACTIVE		-1
#define MXT_STATE_RELEASE		0
#define MXT_STATE_PRESS		1
#define MXT_STATE_MOVE		2

#define MAX_USING_FINGER_NUM 10

/* Feature */
/*#######################################*/
#define TOUCH_BOOSTER				1
#define USE_SUMSIZE					1
#define SYSFS	1
#define FOR_BRINGUP  0
#define UPDATE_ON_PROBE   1
#define READ_FW_FROM_HEADER	0
#define FOR_DEBUGGING_TEST_DOWNLOADFW_BIN 0
#define ITDEV	0
#define SHOW_COORDINATE	0
#define DEBUG_INFO	0
#define USE_ADJUST_FRCCALRATIO	0
/*#######################################*/

/* touch booster */
#if TOUCH_BOOSTER
#include <mach/cpufreq.h>
#define TOUCH_BOOSTER_TIME		3000
#define TOUCH_BOOSTER_LIMIT_CLK	500000

static bool tsp_press_status;
static bool touch_cpu_lock_status;
static int cpu_lv = -1;
#endif

/* use sumsize in T57 register */
#if USE_SUMSIZE
#define MAX_SUMSIZE	40
#define MAX_SINGLE_SUMSIZE		100

static uint16_t sum_size;
#endif

/* Firmware */
#if READ_FW_FROM_HEADER
static u8 firmware_mXT[] = {

};
#endif

#if USE_ADJUST_FRCCALRATIO
#define FRCCALRATIO_RECOVERY_VAL		136	/* -60% */
#define FRCCALRATIO_NORMAL_VAL		0	/* 0% */
#define FRCCALRATIO_INC_VAL			10
#define FRCCALRATIO_VALUE_INITIAL		FRCCALRATIO_RECOVERY_VAL
#define FRCCALRATIO_VALUE_MAX			256
#define FRCCALRATIO_VALUE_MIN			136
#define FRCCALRATIO_TIME				100	/* 100 msec */

static u16 frccalratio_value;
#endif

#if ITDEV
static int driver_paused;
static int debug_enabled;
#endif

/* add for protection code */
/*#######################################*/
#define CNTLMTTCH_AFT_MEDIAN_ERROR	5
#define TIME_FOR_RECONFIG				7	/*7 sec*/
#define TIME_FOR_RECONFIG_ON_BOOT	30	/*30 sec*/
#define TIME_FOR_CHECK_MEDIAN_ERROR	2	/*2 sec*/
#define TIME_FOR_RECAL_ABNMAL_PALM	5	/*5 sec*/

#define CAL_FROM_BOOTUP			0
#define CAL_FROM_RESUME			1
#define CAL_BEF_WORK_CALLED		2
#define CAL_REP_WORK_CALLED		3
#define CAL_AFT_WORK_CALLED		4
/*#######################################*/

/* variable related protection code */
/*#######################################*/
static int treat_median_error_status;
static bool first_touch_aftcal_detected;
static int config_dwork_flag;
static int tchcount_aft_median_error;
/*#######################################*/

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
	s8 state;
	int16_t component;
	u16 mcount;	/*add for debug*/
};

struct report_id_map_t {
	u8 object_type;     /*!< Object type. */
	u8 instance;        /*!< Instance number. */
};

u8 max_report_id;
struct report_id_map_t *rid_map;
static bool rid_map_alloc;

struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct mxt_platform_data *pdata;
	struct early_suspend early_suspend;
	u8 family_id;
	u32 finger_mask;
	int gpio_read_done;
	struct object_t *objects;
	u8 objects_len;
	u8 tsp_version;
	u8 tsp_build;
	u8 tsp_variant;
	u8 finger_type;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;
	u8 tchthr_batt;
	u8 tchthr_charging;
	u8 calcfg_batt;
	u8 calcfg_charging;
	u8 idlesyncsperx_batt;
	u8 idlesyncsperx_charging;
	u8 actvsyncsperx_batt;
	u8 actvsyncsperx_charging;
	u8 xloclip_batt;
	u8 xloclip_charging;
	u8 xhiclip_batt;
	u8 xhiclip_charging;
	u8 yloclip_batt;
	u8 yloclip_charging;
	u8 yhiclip_batt;
	u8 yhiclip_charging;
	u8 xedgectrl_batt;
	u8 xedgectrl_charging;
	u8 xedgedist_batt;
	u8 xedgedist_charging;
	u8 yedgectrl_batt;
	u8 yedgectrl_charging;
	u8 yedgedist_batt;
	u8 yedgedist_charging;
	const u8 *t48_config_batt;
	const u8 *t48_config_chrg;
	struct delayed_work config_dwork;
	struct delayed_work check_median_error_dwork;
	struct delayed_work check_abnormal_palm_dwork;
#if USE_ADJUST_FRCCALRATIO
	struct delayed_work chk_and_adjust_frccalratio_dwork;
#endif
#if TOUCH_BOOSTER
	struct delayed_work dvfs_dwork;
#endif
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(void *);
	void (*read_ta_status)(bool *);
	int num_fingers;
#if ITDEV
	u16 last_read_addr;
	u16 msg_proc_addr;
#endif
	struct finger_info fingers[];
};

struct mxt_data *copy_data;

int touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);
static int mxt_enabled;
static bool g_debug_switch;
static u8 threshold;
static int firm_status_data;

static u8 firmware_latest[] = {0x20, 0xAA};	/* version, build_version */

#if DEBUG_INFO
static u8	*object_type_name[60] = {
/*	[0]	= "Reserved",	*/
/*	[2]	= "T2 - Obsolete",	*/
/*	[3]	= "T3 - Obsolete",	*/
	[5]	= "GEN_MESSAGEPROCESSOR_T5",
	[6]	= "GEN_COMMANDPROCESSOR_T6",
	[7]	= "GEN_POWERCONFIG_T7",
	[8]	= "GEN_ACQUIRECONFIG_T8",
	[9]	= "TOUCH_MULTITOUCHSCREEN_T9",
	[15]	= "TOUCH_KEYARRAY_T15",
	[18]	= "SPT_COMCONFIG_T18",
/*	[22]	= "PROCG_NOISESUPPRESSION_T22",*/
/*	[23]	= "TOUCH_PROXIMITY_T23",*/
/*	[24]	= "PROCI_ONETOUCHGESTUREPROCESSOR_T24",*/
	[25]	= "SPT_SELFTEST_T25",
/*	[26]	= "T26 - Obsolete",*/
/*	[27]	= "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",*/
/*	[28]	= "SPT_CTECONFIG_T28",*/
	[37]	= "DEBUG_DIAGNOSTICS_T37",
	[38]	= "USER_DATA_T38",
	[40]	= "PROCI_GRIPSUPPRESSION_T40",
	[42]	= "PROCI_TOUCHSUPPRESSION_T42",
	[43]	= "SPT_DIGITIZER_T43",
	[46]	= "SPT_CTECONFIG_T46",
	[48]	= "PROCG_NOISESUPPRESSION_T48",
	[56]	= "PROCI_SHIELDLESS_T56",
	[57]	= "SPT_GENERICDATA_T57",
};
#endif

/* declare function proto type */
static void mxt_ta_probe(int ta_status);
static void report_input_data(struct mxt_data *data);

static int read_mem(struct mxt_data *data, u16 reg, u8 len, u8 *buf)
{
	int ret;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	ret = i2c_transfer(data->client->adapter, msg, 2);

	if (ret < 0) {
		pr_err("i2c failed ret = %d\n", ret);
		return ret;
	}
	return ret == 2 ? 0 : -EIO;
}

static int write_mem(struct mxt_data *data, u16 reg, u8 len, const u8 *buf)
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

static int __devinit mxt_reset(struct mxt_data *data)
{
	u8 buf = 1u;
	return write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}

static int __devinit mxt_backup(struct mxt_data *data)
{
	u8 buf = 0x55u;
	return write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
}

static int get_object_info(struct mxt_data *data, u8 object_type, u16 *size,
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

static int write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address = 0;
	u16 size = 0;

	ret = get_object_info(data, type, &size, &address);

	if (size == 0 && address == 0)
		return 0;
	else
		return write_mem(data, address, size, cfg);
}

static int check_instance(struct mxt_data *data, u8 object_type)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type)
			return data->objects[i].instances;
	}
	return 0;
}

static int init_write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address = 0;
	u16 size = 0;
	u8 *temp;

	ret = get_object_info(data, type, &size, &address);

	if ((size == 0) || (address == 0)) {
		pr_err("%s error object_type(%d)\n", __func__, type);
		return -ENODEV;
	}

	ret = write_mem(data, address, size, cfg);

	if (check_instance(data, type)) {
#if DEBUG_INFO
		pr_info("exist instance1 object (%d)\n", type);
#endif
		temp = kmalloc(size * sizeof(u8), GFP_KERNEL);
		memset(temp, 0, size);
		ret |= write_mem(data, address+size, size, temp);
		kfree(temp);
	}

	return ret;
}

static int change_config(struct mxt_data *data,
			u16 reg, u8 offeset, u8 change_value)
{
	u8 value = 0;

	value = change_value;
	return write_mem(data, reg+offeset, 1, &value);
}

static u32 __devinit crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int __devinit calculate_infoblock_crc(struct mxt_data *data,
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

uint8_t calibrate_chip_e(void)
{
	u8 cal_data = 1;
	int ret = 0;
	/* send calibration command to the chip */
	ret = write_mem(copy_data,
		copy_data->cmd_proc + CMD_CALIBRATE_OFFSET,
		1, &cal_data);
	/* set flag for calibration lockup
	recovery if cal command was successful */
	if (!ret)
		pr_info("sucess sending calibration cmd!!!\n");
	return ret;
}

static void mxt_set_normal_config(struct mxt_data *data)
{
	u16 size;
	u16 obj_address = 0;
	int error = 0;

	pr_info("%s\n", __func__);

	get_object_info(data,
		GEN_POWERCONFIG_T7, &size, &obj_address);
	/* 2 :ACTV2IDLETO*/
	error |= change_config(data, obj_address, 2, 25);

#if !(USE_ADJUST_FRCCALRATIO)
	get_object_info(data,
		GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
	/* 8:ATCHFRCCALTHR*/
	error |= change_config(data, obj_address, 8, 50);
	/* 9:ATCHFRCCALRATIO*/
	error |= change_config(data, obj_address, 9, 0);
#endif


	if (error < 0)
		pr_err("%s write config Error!!\n"
					, __func__);
}

static void mxt_set_recovery_config(struct mxt_data *data)
{
	u16 size;
	u16 obj_address = 0;
	int error = 0;

	pr_info("%s\n", __func__);

	get_object_info(data,
		GEN_POWERCONFIG_T7, &size, &obj_address);
	/* 2 :ACTV2IDLETO*/
	error |= change_config(data, obj_address, 2, 7);
#if !(USE_ADJUST_FRCCALRATIO)
	get_object_info(data,
		GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
	/* 8:ATCHFRCCALTHR*/
	error |= change_config(data, obj_address, 8, 8);
	/* 9:ATCHFRCCALRATIO*/
	error |= change_config(data, obj_address, 9, 136);
#endif

	if (error < 0)
		pr_err("%s write config Error!!\n"
					, __func__);
}

static void treat_error_status(void)
{
	bool ta_status = 0;
	u16 size;
	u16 obj_address = 0;
	int error = 0;
	struct mxt_data *data = copy_data;

	data->read_ta_status(&ta_status);

	tchcount_aft_median_error = 0;
	cancel_delayed_work(&data->check_median_error_dwork);
	schedule_delayed_work(&data->check_median_error_dwork
		, HZ*TIME_FOR_CHECK_MEDIAN_ERROR);

	if (treat_median_error_status) {
		pr_err("Error status already treated\n");
		return;
	} else
		treat_median_error_status = 1;

	pr_info("Error status TA is[%d]\n", ta_status);

	if (ta_status) {
		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
#if !(USE_ADJUST_FRCCALRATIO)
		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);
#endif

		get_object_info(data,
			PROCI_TOUCHSUPPRESSION_T42, &size, &obj_address);
		/* 0:CTRL */
		error |= change_config(data, obj_address, 0, 3);

		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address, 2, 48);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address, 3, 48);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 100);
		/* 22:MFINVLDDIFFTHR */
		error |= change_config(data, obj_address, 22, 30);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 50);
		/* 39:MOVFILTER[0] */
		error |= change_config(data, obj_address, 39, 65);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 15);

		if (error < 0)
			pr_err("failed to write error status\n");
	} else {
		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
#if !(USE_ADJUST_FRCCALRATIO)
		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);
#endif
		get_object_info(data,
			TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		/* 31:TCHHYST */
		error |= change_config(data, obj_address, 31, 15);

		get_object_info(data,
			PROCI_TOUCHSUPPRESSION_T42, &size, &obj_address);
		/* 0:CTRL */
		error |= change_config(data, obj_address, 0, 3);

		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address, 2, 32);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address, 3, 32);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 64);
		/* 22:MFINVLDDIFFTHR */
		error |= change_config(data, obj_address, 22, 100);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 45);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 12);

		if (error < 0)
			pr_err("failed to write error status\n");
	}
}

/* this function is called in irq routine */
static void treat_calibration_state(struct mxt_data *data)
{
	first_touch_aftcal_detected = 0;

	cancel_delayed_work(&data->check_abnormal_palm_dwork);

	 if (config_dwork_flag
			== CAL_FROM_RESUME) {
		config_dwork_flag = CAL_BEF_WORK_CALLED;
	} else if ((config_dwork_flag
				== CAL_BEF_WORK_CALLED)
			|| (config_dwork_flag
				== CAL_REP_WORK_CALLED)) {
		cancel_delayed_work(&data->config_dwork);
		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG);
	} else if (config_dwork_flag
				== CAL_AFT_WORK_CALLED) {
		mxt_set_recovery_config(data);
		config_dwork_flag = CAL_BEF_WORK_CALLED;
		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG);
	}
}

/* this function is called in irq routine */
static void treat_first_touch_aftcal(struct mxt_data *data)
{
#if USE_ADJUST_FRCCALRATIO
	int error = 0;
	u16 size;
	u16 obj_address = 0;
#endif
	first_touch_aftcal_detected = 1;
#if USE_ADJUST_FRCCALRATIO
	cancel_delayed_work(&data->chk_and_adjust_frccalratio_dwork);
	frccalratio_value = FRCCALRATIO_VALUE_INITIAL;
	get_object_info(data,
		GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
	/* 9:ATCHFRCCALRATIO*/
	error = change_config(data, obj_address, 9, frccalratio_value);

	pr_info("%s frccalratio_value = %d\n",
		__func__, frccalratio_value);
	schedule_delayed_work(&data->chk_and_adjust_frccalratio_dwork,
	msecs_to_jiffies(FRCCALRATIO_TIME));
#endif
}

static void mxt_reconfigration_normal(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, config_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);

		if (first_touch_aftcal_detected) {
			mxt_set_normal_config(data);
			config_dwork_flag = CAL_AFT_WORK_CALLED;
		} else {
			cancel_delayed_work(&data->config_dwork);
			schedule_delayed_work(&data->config_dwork
				, HZ*TIME_FOR_RECONFIG);
			config_dwork_flag = CAL_REP_WORK_CALLED;
		}

		enable_irq(data->client->irq);
	}
	return;
}

static void mxt_check_medianfilter_error(struct work_struct *work)
{
	pr_info("%s [%d]\n",
		__func__, tchcount_aft_median_error);

	if (tchcount_aft_median_error
		>= CNTLMTTCH_AFT_MEDIAN_ERROR) {
		/*calibrate_chip_e();*/
	}
	tchcount_aft_median_error = 0;
}

static void mxt_check_abnormal_palm(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work,
		struct mxt_data, check_abnormal_palm_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);

		pr_info(
			"force calibration for palm\n");
		calibrate_chip_e();

		enable_irq(data->client->irq);
	}
}

#if USE_ADJUST_FRCCALRATIO
static void mxt_adjust_frccalratio(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work,
	struct mxt_data, chk_and_adjust_frccalratio_dwork.work);

	u16 size;
	u16 obj_address = 0;
	int error = 0;
	bool ta_status = 0;

	get_object_info(data,
		GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);

	if (touch_is_pressed) {
		frccalratio_value += FRCCALRATIO_INC_VAL;
		if (frccalratio_value
			>= FRCCALRATIO_VALUE_MAX) {
			frccalratio_value = FRCCALRATIO_VALUE_MAX;
			error = change_config(data,
				obj_address, 9, FRCCALRATIO_NORMAL_VAL);
			pr_info("%s frccalratio_value = %d\n",
				__func__, frccalratio_value);
			return;
		}
		error = change_config(data,
			obj_address, 9, frccalratio_value);
	} else {
		frccalratio_value -= FRCCALRATIO_INC_VAL;
		if (frccalratio_value <=
			FRCCALRATIO_VALUE_MIN) {
			frccalratio_value = FRCCALRATIO_VALUE_MIN;
			error = change_config(data,
				obj_address, 9, FRCCALRATIO_RECOVERY_VAL);
			pr_info("%s frccalratio_value = %d\n",
				__func__, frccalratio_value);
			return;
		}
		error = change_config(data,
			obj_address, 9, frccalratio_value);
	}
	schedule_delayed_work(&data->chk_and_adjust_frccalratio_dwork,
			msecs_to_jiffies(FRCCALRATIO_TIME));
	pr_info("%s frccalratio_value = %d\n",
			__func__, frccalratio_value);
}
#endif

#if TOUCH_BOOSTER
static void mxt_set_dvfs_off(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, dvfs_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);
		if (touch_cpu_lock_status
			&& !tsp_press_status){
			exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
			touch_cpu_lock_status = 0;
		}
		enable_irq(data->client->irq);
	}
}

static void mxt_set_dvfs_on(struct mxt_data *data)
{
	cancel_delayed_work(&data->dvfs_dwork);
	if (cpu_lv < 0)
		exynos_cpufreq_get_level(TOUCH_BOOSTER_LIMIT_CLK, &cpu_lv);
	exynos_cpufreq_lock(DVFS_LOCK_ID_TSP, cpu_lv);
	touch_cpu_lock_status = 1;
}
#endif

static void mxt_ta_probe(int ta_status)
{
	u16 obj_address = 0;
	u16 size;
	int error;

	struct mxt_data *data = copy_data;

	if (!mxt_enabled) {
		pr_err("%s mxt_enabled is 0\n", __func__);
		return;
	}

	if (treat_median_error_status) {
		treat_median_error_status = 0;
		tchcount_aft_median_error = 0;
	}

	if (ta_status) {
		get_object_info(data,
			TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		error = change_config(data, obj_address,
					7, data->tchthr_charging);
		error |= change_config(data, obj_address,
					22, data->xloclip_charging);
		error |= change_config(data, obj_address,
					23, data->xhiclip_charging);
		error |= change_config(data, obj_address,
					24, data->yloclip_charging);
		error |= change_config(data, obj_address,
					25, data->yhiclip_charging);
		error |= change_config(data, obj_address,
					26, data->xedgectrl_charging);
		error |= change_config(data, obj_address,
					27, data->xedgedist_charging);
		error |= change_config(data, obj_address,
					28, data->yedgectrl_charging);
		error |= change_config(data, obj_address,
					29, data->yedgedist_charging);

		error |= write_config(data, data->t48_config_chrg[0],
					data->t48_config_chrg + 1);
		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address,
				2, data->idlesyncsperx_charging);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address,
				3, data->actvsyncsperx_charging);
		threshold = data->tchthr_charging;

		if (error < 0)
			pr_err("ta_probe write config Error!!\n");
	} else {
		get_object_info(data,
			TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		error = change_config(data, obj_address,
					7, data->tchthr_batt);
		error |= change_config(data, obj_address,
					22, data->xloclip_batt);
		error |= change_config(data, obj_address,
					23, data->xhiclip_batt);
		error |= change_config(data, obj_address,
					24, data->yloclip_batt);
		error |= change_config(data, obj_address,
					25, data->yhiclip_batt);
		error |= change_config(data, obj_address,
					26, data->xedgectrl_batt);
		error |= change_config(data, obj_address,
					27, data->xedgedist_batt);
		error |= change_config(data, obj_address,
					28, data->yedgectrl_batt);
		error |= change_config(data, obj_address,
					29, data->yedgedist_batt);

		error |= write_config(data, data->t48_config_batt[0],
					data->t48_config_batt + 1);
		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address,
					2, data->idlesyncsperx_batt);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address,
					3, data->actvsyncsperx_batt);
		threshold = data->tchthr_batt;

		if (error < 0)
			pr_err("%s write config Error!!\n", __func__);
	}
	pr_info("%s : threshold[%d]\n", __func__, threshold);
};

uint8_t reportid_to_type(struct mxt_data *data, u8 report_id, u8 *instance)
{
	struct report_id_map_t *report_id_map;
	report_id_map = rid_map;

	if (report_id <= max_report_id) {
		*instance = report_id_map[report_id].instance;
		return report_id_map[report_id].object_type;
	} else
		return 0;
}

static int __devinit mxt_init_touch_driver(struct mxt_data *data)
{
	struct object_t *object_table;
	struct report_id_map_t *report_id_map_t;
	u32 read_crc = 0;
	u32 calc_crc;
	u16 crc_address;
	u16 dummy;
	int i, j;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;
	int current_report_id, start_report_id;

	ret = read_mem(data, 0, sizeof(id), id);
	if (ret)
		return ret;

	pr_info("family = %#02x, variant = %#02x, version "
			"= %#02x, build = %#02x, "
			"matrix X,Y size = %d,%d\n"
			, id[0], id[1], id[2], id[3], id[4], id[5]);

	data->family_id = id[0];
	data->tsp_variant = id[1];
	data->tsp_version = id[2];
	data->tsp_build = id[3];
	data->objects_len = id[6];

	object_table = kmalloc(data->objects_len * sizeof(*object_table),
				GFP_KERNEL);
	if (!object_table)
		return -ENOMEM;

	ret = read_mem(data, OBJECT_TABLE_START_ADDRESS,
			data->objects_len * sizeof(*object_table),
			(u8 *)object_table);
	if (ret)
		goto err;

	max_report_id = 0;

	for (i = 0; i < data->objects_len; i++) {
		object_table[i].i2c_address =
			le16_to_cpu(object_table[i].i2c_address);
		max_report_id += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		tmp = 0;
		if (object_table[i].num_report_ids) {
			tmp = type_count + 1;
			type_count += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		}
		switch (object_table[i].object_type) {
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_type = tmp;
			pr_info("Finger type = %d\n",
						data->finger_type);
			break;
		case GEN_MESSAGEPROCESSOR_T5:
#if ITDEV
			data->msg_proc_addr = object_table[i].i2c_address;
#endif
			data->msg_object_size = object_table[i].size + 1;
			break;
		}
	}
	if (rid_map_alloc) {
		rid_map_alloc = false;
		kfree(rid_map);
	}
	rid_map = kmalloc((sizeof(report_id_map_t) * max_report_id + 1),
					GFP_KERNEL);

	if (!rid_map) {
		kfree(object_table);
		return -ENOMEM;
	}
	rid_map_alloc = true;
	rid_map[0].instance = 0;
	rid_map[0].object_type = 0;
	current_report_id = 1;

	for (i = 0; i < data->objects_len; i++) {
		if (object_table[i].num_report_ids != 0) {
			for (j = 0; j <= object_table[i].instances; j++) {
				for (start_report_id = current_report_id;
				     current_report_id <
				     (start_report_id +
				      object_table[i].num_report_ids);
				     current_report_id++) {
					rid_map[current_report_id].instance = j;
					rid_map[current_report_id].object_type =
					    object_table[i].object_type;
				}
			}
		}
	}
	data->objects = object_table;

	/* Verify CRC */
	crc_address = OBJECT_TABLE_START_ADDRESS +
			data->objects_len * OBJECT_TABLE_ELEMENT_SIZE;

#ifdef __BIG_ENDIAN
#error The following code will likely break on a big endian machine
#endif
	ret = read_mem(data, crc_address, 3, (u8 *)&read_crc);
	if (ret)
		goto err;

	read_crc = le32_to_cpu(read_crc);

	ret = calculate_infoblock_crc(data, &calc_crc);
	if (ret)
		goto err;

	if (read_crc != calc_crc) {
		pr_err("CRC error\n");
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

#if DEBUG_INFO
	pr_info("maXTouch: %d Objects\n",
			data->objects_len);

	for (i = 0; i < data->objects_len; i++) {
		pr_info("Type:\t\t\t[%d]: %s\n",
			 object_table[i].object_type,
			 object_type_name[object_table[i].object_type]);
		pr_info("\tAddress:\t0x%04X\n",
			 object_table[i].i2c_address);
		pr_info("\tSize:\t\t%d Bytes\n",
			 object_table[i].size);
		pr_info("\tInstances:\t%d\n",
			 object_table[i].instances);
		pr_info("\tReport Id's:\t%d\n",
			 object_table[i].num_report_ids);
	}
#endif

	return 0;

err:
	kfree(object_table);
	return ret;
}

static void report_input_data(struct mxt_data *data)
{
	int i;
	int count = 0;
	int report_count = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					MT_TOOL_FINGER, false);
		} else {
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
		}
		#ifdef _SUPPORT_SHAPE_TOUCH_
		input_report_abs(data->input_dev, ABS_MT_COMPONENT,
			data->fingers[i].component);
		#endif

		report_count++;

#if SHOW_COORDINATE
		switch (data->fingers[i].state) {
		case MXT_STATE_PRESS:
			pr_info("P: "
				"id[%d],x=%d,y=%d,w=%d,ss=%d\n",
				i, data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].w, sum_size);
			break;
		case MXT_STATE_RELEASE:
			pr_info("R: "
				"id[%d],mc=%d\n",
				i, data->fingers[i].mcount);
			break;
		default:
			break;
		}
#else
		if (data->fingers[i].state == MXT_STATE_PRESS)
			pr_info("P: id[%d],w=%d,ss=%d\n"
				, i, data->fingers[i].w, sum_size);
		else if (data->fingers[i].state == MXT_STATE_RELEASE)
			pr_info("R: id[%d],mc=%d\n"
				, i, data->fingers[i].mcount);
#endif
		if (treat_median_error_status) {
			if (data->fingers[i].state == MXT_STATE_RELEASE
				|| data->fingers[i].state == MXT_STATE_PRESS) {
				tchcount_aft_median_error++;
				if (tchcount_aft_median_error > 100)
					tchcount_aft_median_error = 0;
			}
		} else
			tchcount_aft_median_error = 0;

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			data->fingers[i].state = MXT_STATE_INACTIVE;
			data->fingers[i].mcount = 0;
		} else {
			data->fingers[i].state = MXT_STATE_MOVE;
			count++;
		}
	}
	if (report_count > 0) {
#if ITDEV
		if (!driver_paused)
#endif
			input_sync(data->input_dev);
	}

	if (count)
		touch_is_pressed = 1;
	else
		touch_is_pressed = 0;

#if TOUCH_BOOSTER
	if (count == 0) {
		if (touch_cpu_lock_status) {
			cancel_delayed_work(&data->dvfs_dwork);
			schedule_delayed_work(&data->dvfs_dwork,
				msecs_to_jiffies(TOUCH_BOOSTER_TIME));
		}
		tsp_press_status = 0;
	} else
		tsp_press_status = 1;
#endif
	data->finger_mask = 0;
}

static irqreturn_t mxt_irq_thread(int irq, void *ptr)
{
	struct mxt_data *data = ptr;
	int id;
	u8 msg[data->msg_object_size];
	u8 touch_message_flag = 0;
	u16 obj_address = 0;
	u16 size;
	u8 value;
	int error;
	u8 object_type, instance;

	do {
		touch_message_flag = 0;
		if (read_mem(data, data->msg_proc, sizeof(msg), msg)) {
#if TOUCH_BOOSTER
			if (touch_cpu_lock_status) {
				exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
				touch_cpu_lock_status = 0;
			}
#endif
			return IRQ_HANDLED;
		}
#if ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_INFO, "MXT MSG:",
			DUMP_PREFIX_NONE, 16, 1, msg, sizeof(msg), false);
#endif

		object_type = reportid_to_type(data, msg[0] , &instance);

		if (object_type == RESERVED_T0)
			continue;

		switch (object_type) {
		case GEN_COMMANDPROCESSOR_T6:
		{
			if (msg[1] == 0x00) /* normal mode */
				pr_info("normal mode\n");
			if ((msg[1]&0x04) == 0x04) /* I2C checksum error */
				pr_info("I2C checksum error\n");
			if ((msg[1]&0x08) == 0x08) /* config error */
				pr_info("config error\n");
			if ((msg[1]&0x10) == 0x10)	{ /* calibration */
				pr_info("calibration is"
					" on going !!\n");
				treat_calibration_state(data);
			}
			if ((msg[1]&0x20) == 0x20) /* signal error */
				pr_info("signal error\n");
			if ((msg[1]&0x40) == 0x40) /* overflow */
				pr_info("overflow detected\n");
			if ((msg[1]&0x80) == 0x80) /* reset */
				pr_info("reset is ongoing\n");
		}
			break;
		case TOUCH_MULTITOUCHSCREEN_T9:
		{
			id = msg[0] - data->finger_type;

			/* If not a touch event, then keep going */
			if (id < 0 || id >= data->num_fingers)
				continue;

			if (data->finger_mask & (1U << id))
				report_input_data(data);

			if (msg[1] & RELEASE_MSG_MASK) {
				data->fingers[id].z = 0;
				data->fingers[id].w = msg[5];
				data->finger_mask |= 1U << id;
				data->fingers[id].state = MXT_STATE_RELEASE;
			} else if ((msg[1] & DETECT_MSG_MASK) && (msg[1] &
					(PRESS_MSG_MASK | MOVE_MSG_MASK))) {
#if TOUCH_BOOSTER
				if (!touch_cpu_lock_status)
					mxt_set_dvfs_on(data);
#endif
				touch_message_flag = 1;
				data->fingers[id].z = msg[6];
				data->fingers[id].w = msg[5];
				data->fingers[id].x =
					(((msg[2] << 4) | (msg[4] >> 4))
					>> data->x_dropbits);
				data->fingers[id].y =
					(((msg[3] << 4) | (msg[4] & 0xF))
					>> data->y_dropbits);

				data->finger_mask |= 1U << id;

				if (msg[1] & PRESS_MSG_MASK) {
					data->fingers[id].state =
						MXT_STATE_PRESS;
					data->fingers[id].mcount = 0;

					if (!first_touch_aftcal_detected)
						treat_first_touch_aftcal(data);
				} else if (msg[1] & MOVE_MSG_MASK)
					data->fingers[id].mcount += 1;

				#ifdef _SUPPORT_SHAPE_TOUCH_
				data->fingers[id].component = msg[7];
				#endif

			} else if ((msg[1] & SUPPRESS_MSG_MASK)
			&& (data->fingers[id].state != MXT_STATE_INACTIVE)) {
				data->fingers[id].z = 0;
				data->fingers[id].w = msg[5];
				data->fingers[id].state = MXT_STATE_RELEASE;
				data->finger_mask |= 1U << id;
			} else {
				/* ignore changed amplitude message */
				if (!((msg[1] & DETECT_MSG_MASK)
					&& (msg[1] & AMPLITUDE_MSG_MASK)))
					pr_err("Unknown state %#02x %#02x\n",
						msg[0], msg[1]);
				continue;
			}
		}
			break;
		case PROCI_TOUCHSUPPRESSION_T42:
		{
			get_object_info(data, GEN_ACQUISITIONCONFIG_T8,
							&size, &obj_address);
			if ((msg[1] & 0x01) == 0x00) {
				/* Palm release */
				pr_info("palm touch released\n");

				cancel_delayed_work(
					&data->check_abnormal_palm_dwork);
				touch_is_pressed = 0;
			} else if ((msg[1] & 0x01) == 0x01) {
				/* Palm Press */
				pr_info("palm touch detected\n");

				cancel_delayed_work(
					&data->check_abnormal_palm_dwork);
				schedule_delayed_work(
					&data->check_abnormal_palm_dwork,
					HZ*TIME_FOR_RECAL_ABNMAL_PALM);
				touch_is_pressed = 1;
				touch_message_flag = 1;
			}
		}
			break;
		case PROCG_NOISESUPPRESSION_T48:
		{
			/* pr_info("T48 [STATUS]:%#02x"
				"[ADCSPERX]:%#02x[FRQ]:%#02x"
				"[STATE]:%#02x[NLEVEL]:%#02x\n"
				, msg[1], msg[2], msg[3], msg[4], msg[5]);*/

			if (msg[4] == 5) { /* Median filter error */
				pr_info("Median filter error\n");
				if ((data->family_id == 0xA1)
					&& ((data->tsp_version == 0x13)
					|| (data->tsp_version == 0x20))) {
					if (data->read_ta_status)
						treat_error_status();
				} else {
					get_object_info(data,
						PROCG_NOISESUPPRESSION_T48,
						&size, &obj_address);
					value = data->calcfg_batt;
					error = write_mem(data,
						obj_address+2, 1, &value);
					msleep(20);
					value |= 0x20;
					error |= write_mem(data,
						obj_address+2, 1, &value);
					if (error)
						pr_err("failed to"
							"reenable CHRGON\n");
				}
			}
		}
			break;
#if USE_SUMSIZE
		case SPT_GENERICDATA_T57:
		{
			sum_size = msg[1];
			sum_size +=  (msg[2]<<8);
		}
			break;
#endif
		default:
			pr_info("Untreated Report ID[%d], %#02x, %#02x\n",
				object_type, msg[0], msg[1]);
			break;
		}
	} while (!gpio_get_value(data->gpio_read_done));

	if (data->finger_mask) {
#if USE_SUMSIZE
		if (sum_size > 0) {
			/* case of normal configuration */
			u8 num_finger = 0;
			u8 i;
			uint16_t t9_sum_size = 0;

			for (i = 0; i < data->num_fingers; i++) {
				if (data->fingers[i].state == MXT_STATE_INACTIVE
				|| data->fingers[i].state == MXT_STATE_RELEASE)
					continue;
				t9_sum_size += data->fingers[i].w;

				num_finger++;
			}

			if ((num_finger == 1)
				&& ((sum_size-t9_sum_size) >= MAX_SUMSIZE)) {
				pr_info(
					"recalibrate for max sumsize[%d]"
					"t9_sum_size[%d]\n"
					, sum_size, t9_sum_size);
				calibrate_chip_e();
			} else {
				report_input_data(data);
			}

			t9_sum_size = 0;
			sum_size = 0;
		} else {
			report_input_data(data);
		}
#else
		report_input_data(data);
#endif
	}

	return IRQ_HANDLED;
}

static int mxt_internal_suspend(struct mxt_data *data)
{
	int i;
	int count = 0;

	cancel_delayed_work(&data->config_dwork);
	cancel_delayed_work(&data->check_median_error_dwork);
	cancel_delayed_work(&data->check_abnormal_palm_dwork);
#if USE_ADJUST_FRCCALRATIO
	cancel_delayed_work(&data->chk_and_adjust_frccalratio_dwork);
#endif

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
		count++;
	}
	if (count)
		report_input_data(data);

#if TOUCH_BOOSTER
	cancel_delayed_work(&data->dvfs_dwork);
	tsp_press_status = 0;
	if (touch_cpu_lock_status) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		touch_cpu_lock_status = 0;
	}
#endif
	data->power_off();

	return 0;
}

static int mxt_internal_resume(struct mxt_data *data)
{
	data->power_on();

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt_suspend	NULL
#define mxt_resume	NULL

static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);

	if (mxt_enabled == 1) {
		pr_info("%s\n", __func__);
		mxt_enabled = 0;
		touch_is_pressed = 0;
		disable_irq(data->client->irq);
		mxt_internal_suspend(data);
	} else
		pr_err("%s. but touch already off\n", __func__);
}

static void mxt_late_resume(struct early_suspend *h)
{
	bool ta_status = 0;
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);
	if (mxt_enabled == 0) {
		pr_info("%s\n", __func__);
		mxt_internal_resume(data);

		mxt_enabled = 1;

		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}

		config_dwork_flag = CAL_FROM_RESUME;
		first_touch_aftcal_detected = 0;
		treat_median_error_status = 0;
		tchcount_aft_median_error = 0;

		enable_irq(data->client->irq);
	} else
		pr_err("%s. but touch already on\n", __func__);
}
#else
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mxt_enabled = 0;
	touch_is_pressed = 0;
#if TOUCH_BOOSTER
	tsp_press_status = 0;
#endif
	return mxt_internal_suspend(data);
}

static int mxt_resume(struct device *dev)
{
	int ret = 0;
	bool ta_status = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	ret = mxt_internal_resume(data);

	mxt_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		pr_info("ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}
	return ret;
}
#endif

void Mxt_force_released(void)
{
	struct mxt_data *data = copy_data;
	int i;

	if (!mxt_enabled) {
		pr_err("mxt_enabled is 0\n");
		return;
	}

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
	}
	report_input_data(data);

	calibrate_chip_e();
};

#if SYSFS
static ssize_t mxt_debug_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	g_debug_switch = !g_debug_switch;
	return 0;
}

static ssize_t mxt_object_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	unsigned int object_register;
	unsigned int register_value;
	u8 value;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	sscanf(buf, "%u%u%u", &object_type, &object_register, &register_value);
	pr_info("object type T%d", object_type);
	pr_info("object register ->Byte%d\n", object_register);
	pr_info("register value %d\n", register_value);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return count;
	}

	size = 1;
	value = (u8)register_value;
	write_mem(data, address+(u16)object_register, size, &value);
	read_mem(data, address+(u16)object_register, (u8)size, &val);

	pr_info("T%d Byte%d is %d\n",
		object_type, object_register, val);
	return count;
}

static ssize_t mxt_object_show(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	u16 i;
	sscanf(buf, "%u", &object_type);
	pr_info("object type T%d\n", object_type);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return count;
	}
	for (i = 0; i < size; i++) {
		read_mem(data, address+i, 1, &val);
		pr_info("Byte %u --> %u\n", i, val);
	}
	return count;
}

struct device *sec_touchscreen;
struct device *mxt_noise_test;
/*
	top_left, top_right, center, bottom_left, bottom_right
*/
unsigned int test_node[5] = {642, 98, 367, 668, 124};

uint16_t qt_refrence_node[768] = { 0 };
uint16_t qt_delta_node[768] = { 0 };

void diagnostic_chip(u8 mode)
{
	int error;
	u16 t6_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	u16 t37_address = 0;

	ret = get_object_info(copy_data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &t6_address);

	size_one = 1;
	error = write_mem(copy_data, t6_address+5, (u8)size_one, &mode);
	/* QT602240_COMMAND_DIAGNOSTIC, mode); */
	if (error < 0) {
		pr_err("error %s: write_object\n", __func__);
	} else {
		get_object_info(copy_data,
			DEBUG_DIAGNOSTIC_T37, &size_one, &t37_address);
		size_one = 1;
		/* pr_info("diagnostic_chip setting success\n"); */
		read_mem(copy_data, t37_address, (u8)size_one, &value);
		/* pr_info("dianostic_chip mode is %d\n",value); */
	}
}

uint8_t read_uint16_t(struct mxt_data *data, uint16_t address, uint16_t *buf)
{
	uint8_t status;
	uint8_t temp[2];

	status = read_mem(data, address, 2, temp);
	*buf = ((uint16_t)temp[1]<<8) + (uint16_t)temp[0];

	return status;
}

void read_dbg_data(uint8_t dbg_mode , uint16_t node, uint16_t *dbg_data)
{
	u8 read_page, read_point;
	uint8_t mode, page;
	u16 size;
	u16 diagnostic_addr = 0;

	if (!mxt_enabled) {
		pr_err("read_dbg_data. mxt_enabled is 0\n");
		return;
	}

	get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &diagnostic_addr);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(20);

	do {
		if (read_mem(copy_data, diagnostic_addr, 1, &mode)) {
			pr_info("READ_MEM_FAILED\n");
			return;
		}
	} while (mode != MXT_CTE_MODE);

	diagnostic_chip(dbg_mode);
	msleep(20);

	do {
		if (read_mem(copy_data, diagnostic_addr, 1, &mode)) {
			pr_info("READ_MEM_FAILED\n");
			return;
		}
	} while (mode != dbg_mode);

	for (page = 1; page <= read_page; page++) {
		diagnostic_chip(MXT_PAGE_UP);
		msleep(20);
		do {
			if (read_mem(copy_data,
				diagnostic_addr + 1, 1, &mode)) {
				pr_info("READ_MEM_FAILED\n");
				return;
			}
		} while (mode != page);
	}

	if (read_uint16_t(copy_data, diagnostic_addr + read_point, dbg_data)) {
		pr_info("READ_MEM_FAILED\n");
		return;
	}
}

#define MIN_VALUE 19744
#define MAX_VALUE 28884
#define MIN_VALUE_TA_ERROR_MODE	19125

#define T48_CALCFG_CHRGON		0x20

/* caution : should check the sensor level
this value is depend on tsp tunning state */
#define SENSOR_GAIN_TAERROR	5

int read_all_data(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 max_value = MIN_VALUE, min_value = MAX_VALUE;
	u16 ref_max_value = MAX_VALUE;
	u16 ref_min_value = MIN_VALUE;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;
	u8 val = 0;
	u8 sensor_gain = 0;
	bool ta_status = 0;

	if (!mxt_enabled) {
		pr_err("%s : mxt_enabled is 0\n", __func__);
		return 1;
	}

	if (copy_data->read_ta_status) {
		copy_data->read_ta_status(&ta_status);
		pr_info("ta_status is %d\n", ta_status);
	}

	/* check the CHRG_ON bit is set or not */
	/* when CHRG_ON is setted dual x is on so skip read last line*/
	get_object_info(copy_data,
		PROCG_NOISESUPPRESSION_T48, &size, &object_address);
	ret = read_mem(copy_data, object_address+2 , 1, &val);
	if (ret < 0)
		pr_err("TSP read fail : %s", __func__);

	pr_info("%s CALCFG[%#02x]\n", __func__, val);
	val = val & T48_CALCFG_CHRGON;

	/* read sensor gain to check reference value */
	if (val == T48_CALCFG_CHRGON) {

		ret = read_mem(copy_data, object_address+34 , 1, &sensor_gain);
		if (ret < 0)
			pr_err("TSP read fail : %s", __func__);
		sensor_gain = (sensor_gain&0xF0)>>4;

		pr_info("%s BLEN[%d]\n", __func__, sensor_gain);
		if (sensor_gain == SENSOR_GAIN_TAERROR)
			ref_min_value = MIN_VALUE_TA_ERROR_MODE;
	}

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);

	msleep(50); /* msleep(20);  */

	/* 768/64 */
	for (read_page = 0 ; read_page < 12; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data,
				object_address+(u16)read_point, 2, data_buffer);
			qt_refrence_node[num] = ((uint16_t)data_buffer[1]<<8)
				+ (uint16_t)data_buffer[0];

			/* last X line has 1/2 reference during
				TA mode So do not check min/max value */
			if ((val != T48_CALCFG_CHRGON)
				|| (val == T48_CALCFG_CHRGON && (num < 736))) {
				if ((qt_refrence_node[num] < ref_min_value)
				|| (qt_refrence_node[num] > ref_max_value)) {
					if (sensor_gain == SENSOR_GAIN_TAERROR)
						state = 2;
					else
						state = 1;
					pr_err(
						"qt_refrence_node[%3d] = %5d\n"
						, num, qt_refrence_node[num]);
				}

				if (data_buffer[0] != 0) {
					if (qt_refrence_node[num] > max_value)
						max_value =
							qt_refrence_node[num];
					if (qt_refrence_node[num] < min_value)
						min_value =
							qt_refrence_node[num];
				}
			}

			num++;
			/* all node => 24 * 32 = 768 => (12page * 64) */
			/* if ((read_page == 11) && (node == 64))
				break; */
			if (qt_refrence_node[num-1] == 0)
				pr_err("qt_refrence_node"
					"[%d] = 0\n", num);
			if (num == 768)
				break;
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
		if (num == 768)
			break;
	}

	if ((max_value - min_value) > 4500) {
		pr_err("diff = %d, max_value = %d, min_value = %d\n",
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

	if (!mxt_enabled) {
		pr_err("%s : mxt_enabled is 0\n", __func__);
		return 1;
	}

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);
/*jerry no need to leave it */
#if 0
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(50); /* msleep(20);  */
#endif

	/* 768/64 */
	for (read_page = 0 ; read_page < 12; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data,
				object_address+(u16)read_point, 2, data_buffer);
				qt_delta_node[num] =
					((uint16_t)data_buffer[1]<<8)
					+ (uint16_t)data_buffer[0];

			num++;

			/* all node => 24 * 32 = 768 => (12page * 64) */
			/*if ((read_page == 11) && (node == 64))
				break;*/
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
	}

	return state;
}

int find_channel(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;
	u16 delta_val = 0;
	u16 max_val = 0;

	if (!mxt_enabled) {
		pr_err("%s : mxt_enabled is 0\n", __func__);
		return 1;
	}

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);
/*jerry no need to leave it */
#if 0
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
#else
	msleep(50); /* msleep(20);  */
#endif

	/* 768/64 */
	for (read_page = 0 ; read_page < 12; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data,
				object_address+(u16)read_point, 2, data_buffer);
				delta_val = ((uint16_t)data_buffer[1]<<8)
					+ (uint16_t)data_buffer[0];

				if (delta_val > 32767)
					delta_val = 65535 - delta_val;
				if (delta_val > max_val) {
					max_val = delta_val;
					state = (read_point - 2)/2 +
						(read_page * 64);
				}

			num++;

			/* all node => 24 * 32 = 768 => (12page * 64) */
			/*if ((read_page == 11) && (node == 64))
				break;*/
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
	}

	return state;
}

static ssize_t find_channel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int status = 0;

	status = find_channel(MXT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}
#endif

static int mxt_check_bootloader(struct i2c_client *client,
					unsigned int state)
{
	u8 val;
	u8 temp;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1)
		return -EIO;

	if (val & 0x20)	{
		if (i2c_master_recv(client, &temp, 1) != 1)
			return -EIO;

		if (i2c_master_recv(client, &temp, 1) != 1)
			return -EIO;
		val &= ~0x20;
	}

	if ((val & 0xF0) == MXT_APP_CRC_FAIL) {
		pr_info("MXT_APP_CRC_FAIL\n");
		if (i2c_master_recv(client, &val, 1) != 1)
			return -EIO;

		if (val & 0x20) {
			if (i2c_master_recv(client, &temp, 1) != 1)
				return -EIO;

			if (i2c_master_recv(client, &temp, 1) != 1)
				return -EIO;
			val &= ~0x20;
		}
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
		pr_err("Unvalid bootloader mode state\n");
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		pr_err("%s: i2c send failed\n",
			__func__);

		return -EIO;
	}

	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
				const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		pr_err("%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address = 0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;

	pr_info("mxt_load_fw start from header!!!\n");
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT;
	fw->size = sizeof(firmware_mXT);
#else
	const struct firmware *fw = NULL;

	pr_info("mxt_load_fw start!!!\n");
	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		pr_err("Unable to open firmware %s\n", fn);
		return ret;
	}
#endif
	/* Change to the bootloader mode */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret) {
		pr_err("fail to get object_info\n");
		release_firmware(fw);
		return ret;
	}
	size_one = 1;
	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
	msleep(MXT_SW_RESET_TIME);

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_err("check_wating_frame_data_error = %d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_err("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n",
			pos, fw->size);

		msleep(20);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}

static int mxt_load_fw_bootmode(struct device *dev, const char *fn)
{
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;
	pr_info("mxt_load_fw start from header!!!\n");
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT;
	fw->size = sizeof(firmware_mXT);
#else
	const struct firmware *fw = NULL;
	pr_info("mxt_load_fw start!!!\n");

	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		pr_err("Unable to open firmware %s\n", fn);
		return ret;
	}
#endif
	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_err("check_wating_frame_data_error = %d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_err("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n",
			pos, fw->size);

		msleep(20);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}

#if SYSFS
static ssize_t set_refer0_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference = 0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[0], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer1_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference = 0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[1], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer2_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference = 0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[2], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer3_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference = 0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[3], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer4_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference = 0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[4], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_delta0_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta = 0;
	read_dbg_data(MXT_DELTA_MODE, test_node[0], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if (mxt_delta)
		return sprintf(buf, "-%u\n", mxt_delta);
	else
		return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta1_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta = 0;
	read_dbg_data(MXT_DELTA_MODE, test_node[1], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if (mxt_delta)
		return sprintf(buf, "-%u\n", mxt_delta);
	else
		return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta2_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta = 0;
	read_dbg_data(MXT_DELTA_MODE, test_node[2], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if (mxt_delta)
		return sprintf(buf, "-%u\n", mxt_delta);
	else
		return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta3_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta = 0;
	read_dbg_data(MXT_DELTA_MODE, test_node[3], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if (mxt_delta)
		return sprintf(buf, "-%u\n", mxt_delta);
	else
		return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta4_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta = 0;
	read_dbg_data(MXT_DELTA_MODE, test_node[4], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if (mxt_delta)
		return sprintf(buf, "-%u\n", mxt_delta);
	else
		return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_threshold_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", threshold);
}

static ssize_t set_all_refer_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_data(MXT_REFERENCE_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_reference;

static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return -1;
	while (str[count] && str[count] >= '0' && str[count] <= '9') {
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}

ssize_t disp_all_refdata_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n",  qt_refrence_node[index_reference]);
}

ssize_t disp_all_refdata_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	index_reference = atoi(buf);
	return size;
}

static ssize_t set_all_delta_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_delta_data(MXT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_delta;

ssize_t disp_all_deltadata_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (qt_delta_node[index_delta] < 32767)
		return sprintf(buf, "%u\n", qt_delta_node[index_delta]);
	else
		qt_delta_node[index_delta] = 65535 - qt_delta_node[index_delta];

	return sprintf(buf, "-%u\n", qt_delta_node[index_delta]);
}

ssize_t disp_all_deltadata_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	index_delta = atoi(buf);
	return size;
}

static ssize_t set_firm_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = copy_data;

	u8 id[ID_BLOCK_SIZE];
	u8 value;
	int ret;
	u8 i;

	if (mxt_enabled == 1) {
		disable_irq(data->client->irq);
		for (i = 0; i < 4; i++) {
			ret = read_mem(copy_data, 0, sizeof(id), id);
			if (!ret)
				break;
		}
		enable_irq(data->client->irq);
		if (ret < 0) {
			pr_err("TSP read fail : %s", __func__);
			value = 0;
			return sprintf(buf, "%d\n", value);
		} else {
			pr_info("%s : %#02x\n",
				__func__, id[2]);
			return sprintf(buf, "%#02x\n", id[2]);
		}
	} else {
		pr_err("TSP power off : %s", __func__);
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
}

static ssize_t set_module_off_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxt_data *data = copy_data;
	pr_info("%s!!\n", __func__);

	if (*buf != 'S' && *buf != 'F') {
		pr_err("Invalid values\n");
		return -EINVAL;
	}
	if (mxt_enabled == 1) {
		mxt_enabled = 0;
		touch_is_pressed = 0;
#if TOUCH_BOOSTER
		tsp_press_status = 0;
#endif
		disable_irq(data->client->irq);
		mxt_internal_suspend(data);
	}
	return size;
}

static ssize_t set_module_on_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxt_data *data = copy_data;
	bool ta_status = 0;

	pr_info("%s!!\n", __func__);

	if (*buf != 'S' && *buf != 'F') {
		pr_err("Invalid values\n");
		return -EINVAL;
	}

	if (mxt_enabled == 0) {
		mxt_internal_resume(data);
		enable_irq(data->client->irq);

		mxt_enabled = 1;

		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		calibrate_chip_e();
	}

	return size;
}

#if FOR_DEBUGGING_TEST_DOWNLOADFW_BIN
#include <linux/uaccess.h>

#define MXT_FW_BIN_NAME "/sdcard/mxt.bin"

static int mxt_download(const u8 *pBianry, const u32 unLength)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address = 0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

	pr_info("mxt_download start!!!\n");

	/* Change to the bootloader mode */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return ret;
	}
	size_one = 1;

	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
	msleep(MXT_SW_RESET_TIME);

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;
	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);

	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < unLength) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_info("check_wating_frame_data_error=%d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(pBianry + pos) << 8) | *(pBianry + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, pBianry + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_info("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n", pos, unLength);

		msleep(20);
	}

out:
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}

int mxt_binfile_download(void)
{
	int nRet = 0;
	int retry_cnt = 0;
	long fw1_size = 0;
	unsigned char *fw_data1;
	struct file *filp;
	loff_t	pos;
	int	ret = 0;
	mm_segment_t oldfs;
	spinlock_t	lock;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MXT_FW_BIN_NAME, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("file open error:%d\n", (s32)filp);
		return -1;
	}

	fw1_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("Size of the file : %ld(bytes)\n", fw1_size);

	fw_data1 = kmalloc(fw1_size, GFP_KERNEL);
	memset(fw_data1, 0, fw1_size);

	pos = 0;
	memset(fw_data1, 0, fw1_size);
	ret = vfs_read(filp, (char __user *)fw_data1, fw1_size, &pos);

	if (ret != fw1_size) {
		pr_err("Failed to read file %s (ret = %d)\n",
			MXT_FW_BIN_NAME, ret);
		kfree(fw_data1);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(oldfs);

	for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
		pr_info("ADB - MASTER CHIP Firmware update! try : %d",
			retry_cnt+1);
		nRet = mxt_download((const u8 *)fw_data1, (const u32)fw1_size);
		if (nRet)
			continue;
		break;
	}

	kfree(fw_data1);
	return nRet;
}
#endif

static ssize_t set_mxt_firm_update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 **tsp_config = (u8 **)data->pdata->config;
	int i = 0;
	int error = 0;

	pr_info("set_mxt_update_show start!!\n");
	if (*buf != 'S' && *buf != 'F') {
		pr_err("Invalid values\n");
		return -EINVAL;
	}

	disable_irq(data->client->irq);
	firm_status_data = 1;
#if FOR_DEBUGGING_TEST_DOWNLOADFW_BIN
	error = mxt_binfile_download();
#else
	if (*buf != 'F' && data->tsp_version >= firmware_latest[0]
		&& data->tsp_build >= firmware_latest[1]) {
		pr_err("latest firmware\n");
		firm_status_data = 2;
		enable_irq(data->client->irq);
		return size;
	}
	pr_info("fm_update\n");
	error = mxt_load_fw(dev, MXT_FW_NAME);
#endif

	if (error) {
		firm_status_data = 3;
		pr_err("The firmware update failed(%d)\n", error);
		return error;
	} else {
		firm_status_data = 2;
		pr_info("The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_SW_RESET_TIME);

		mxt_init_touch_driver(data);
		/* mxt224_initialize(data); */
	}

	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
		error = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
		if (error) {
			pr_err("init_write_config error\n");
			firm_status_data = 3;
			return error;
		}
	}

	error = mxt_backup(data);
	if (error) {
		pr_err("mxt_backup fail!!!\n");
		return error;
	}

	/* reset the touch IC. */
	error = mxt_reset(data);
	if (error) {
		pr_err("mxt_reset fail!!!\n");
		return error;
	}

	msleep(MXT_SW_RESET_TIME);
	enable_irq(data->client->irq);

	return size;
}

static ssize_t set_mxt_firm_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	int count;
	pr_info("Enter firmware_status_show by Factory command\n");

	if (firm_status_data == 1)
		count = sprintf(buf, "DOWNLOADING\n");
	else if (firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");

	return count;
}

static ssize_t key_threshold_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", threshold);
}

static ssize_t key_threshold_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	/*TO DO IT*/
	unsigned int object_register = 7;
	u8 value;
	u8 val;
	int ret;
	u16 address = 0;
	u16 size_one;
	int num;
	if (sscanf(buf, "%d", &num) == 1) {
		threshold = num;
		pr_info("threshold value %d\n", threshold);
		ret = get_object_info(copy_data,
			TOUCH_MULTITOUCHSCREEN_T9, &size_one, &address);
		size_one = 1;
		value = (u8)threshold;
		write_mem(copy_data,
			address+(u16)object_register, size_one, &value);
		read_mem(copy_data,
			address+(u16)object_register, (u8)size_one, &val);
		pr_err("T9 Byte%d is %d\n", object_register, val);
	}
	return size;
}

static ssize_t set_mxt_firm_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("phone's version : %#02x,%#02x\n"
		, firmware_latest[0], firmware_latest[1]);
	return sprintf(buf, "%#02x\n", firmware_latest[0]);
}

static ssize_t set_mxt_firm_version_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	pr_info("phone's version : %#02x,%#02x\n"
		, data->tsp_version, data->tsp_build);
	return sprintf(buf, "%#02x\n", data->tsp_version);
}

static ssize_t mxt_touchtype_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "ATMEL,MXT768E\n");
	strcat(buf, temp);

	return strlen(buf);
}

static ssize_t x_line_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 data = 24;
	return sprintf(buf, "%d\n", data);
}

static ssize_t y_line_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 data = 32;
	return sprintf(buf, "%d\n", data);
}

#if ITDEV
/* Functions for mem_access interface */
struct bin_attribute mem_access_attr;

static int mxt_read_block(struct i2c_client *client,
		   u16 addr,
		   u16 length,
		   u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16	le_addr;
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	if (mxt != NULL) {
		if ((mxt->last_read_addr == addr) &&
			(addr == mxt->msg_proc_addr)) {
			if  (i2c_master_recv(client, value, length) == length) {
#if ITDEV
				if (debug_enabled)
					print_hex_dump(KERN_INFO, "MXT RX:",
						DUMP_PREFIX_NONE, 16, 1,
						value, length, false);
#endif
				return 0;
			} else
				return -EIO;
		} else {
			mxt->last_read_addr = addr;
		}
	}

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;
	if (i2c_transfer(adapter, msg, 2) == 2) {
#if ITDEV
		if (debug_enabled) {
			print_hex_dump(KERN_INFO, "MXT TX:", DUMP_PREFIX_NONE,
				16, 1, msg[0].buf, msg[0].len, false);
			print_hex_dump(KERN_INFO, "MXT RX:", DUMP_PREFIX_NONE,
				16, 1, msg[1].buf, msg[1].len, false);
		}
#endif
		return 0;
	} else
		return -EIO;
}

/* Writes a block of bytes (max 256) to given address in mXT chip. */

int mxt_write_block(struct i2c_client *client,
		    u16 addr,
		    u16 length,
		    u8 *value)
{
	int i;
	struct {
		__le16	le_addr;
		u8	data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	if (length > 256)
		return -EINVAL;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;

	i2c_block_transfer.le_addr = cpu_to_le16(addr);

	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

	if (i == (length + 2)) {
#if ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_INFO, "MXT TX:", DUMP_PREFIX_NONE,
				16, 1, &i2c_block_transfer, length+2, false);
#endif
		return length;
	} else
		return -EIO;
}

static ssize_t mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	int ret = 0;
	struct i2c_client *client;

	pr_info("mem_access_read p=%p off=%lli c=%zi\n", buf, off, count);

	if (off >= 32768)
		return -EIO;

	if (off + count > 32768)
		count = 32768 - off;

	if (count > 256)
		count = 256;

	if (count > 0)	{
		client = to_i2c_client(container_of(kobj, struct device, kobj));
		ret = mxt_read_block(client, off, count, buf);
	}

	return ret >= 0 ? count : ret;
}

static ssize_t mem_access_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	int ret = 0;
	struct i2c_client *client;

	pr_info("mem_access_write p=%p off=%lli c=%zi\n", buf, off, count);

	if (off >= 32768)
		return -EIO;

	if (off + count > 32768)
		count = 32768 - off;

	if (count > 256)
		count = 256;

	if (count > 0) {
		client = to_i2c_client(container_of(kobj, struct device, kobj));
		ret = mxt_write_block(client, off, count, buf);
	}

	return ret >= 0 ? count : 0;
}

static ssize_t pause_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf + count, "%d", driver_paused);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t pause_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i;
	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		driver_paused = i;

		pr_info("%s\n", i ? "paused" : "unpaused");
	} else {
		pr_info("pause_driver write error\n");
	}

	return count;
}

static ssize_t debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf + count, "%d", debug_enabled);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t debug_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i;
	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		debug_enabled = i;

		pr_info("%s\n",
			i ? "debug enabled" : "debug disabled");
	} else {
		pr_info("debug_enabled write error\n");
	}

	return count;
}

static ssize_t command_calibrate_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;

	ret = calibrate_chip_e();

	return (ret < 0) ? ret : count;
}

static ssize_t command_reset_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	int ret;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	ret = mxt_reset(mxt);

	return (ret < 0) ? ret : count;
}

static ssize_t command_backup_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	int ret;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

#if 0
	ret = backup_to_nv(mxt);
#else
	ret = mxt_backup(mxt);
#endif

	return (ret < 0) ? ret : count;
}
#endif

static DEVICE_ATTR(set_refer0, S_IRUGO,
	set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO,
	set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO,
	set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO,
	set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO,
	set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO,
	set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO,
	set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO,
	set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO,
	set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO,
	set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO,
	set_all_refer_mode_show, NULL);
static DEVICE_ATTR(disp_all_refdata, S_IRUGO | S_IWUSR | S_IWGRP,
	disp_all_refdata_show, disp_all_refdata_store);
static DEVICE_ATTR(set_all_delta, S_IRUGO,
	set_all_delta_mode_show, NULL);
static DEVICE_ATTR(disp_all_deltadata, S_IRUGO | S_IWUSR | S_IWGRP,
	disp_all_deltadata_show, disp_all_deltadata_store);
static DEVICE_ATTR(set_firm_version, S_IRUGO | S_IWUSR | S_IWGRP,
	set_firm_version_show, NULL);
static DEVICE_ATTR(set_module_off, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, set_module_off_store);
static DEVICE_ATTR(set_module_on, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, set_module_on_store);
static DEVICE_ATTR(mxt_touchtype, S_IRUGO | S_IWUSR | S_IWGRP,
	mxt_touchtype_show, NULL);
static DEVICE_ATTR(set_threshold, S_IRUGO,
	set_threshold_mode_show, NULL);
/* firmware update */
static DEVICE_ATTR(tsp_firm_update, S_IWUSR | S_IWGRP,
	NULL, set_mxt_firm_update_store);
/* firmware update status return */
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO,
	set_mxt_firm_status_show, NULL);
/* touch threshold return, store */
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	key_threshold_show, key_threshold_store);
/* PHONE*/	/* firmware version resturn in phone driver version */
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO,
	set_mxt_firm_version_show, NULL);
/*PART*/	/* firmware version resturn in TSP panel version */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO,
	set_mxt_firm_version_read_show, NULL);
static DEVICE_ATTR(object_show, S_IWUSR | S_IWGRP, NULL,
	mxt_object_show);
static DEVICE_ATTR(object_write, S_IWUSR | S_IWGRP, NULL,
	mxt_object_setting);
static DEVICE_ATTR(dbg_switch, S_IWUSR | S_IWGRP, NULL,
	mxt_debug_setting);
static DEVICE_ATTR(find_delta_channel, S_IRUGO,
	find_channel_show, NULL);
static DEVICE_ATTR(x_line, S_IRUGO,
	x_line_show, NULL);
static DEVICE_ATTR(y_line, S_IRUGO,
	y_line_show, NULL);
#if ITDEV
/* Sysfs files for libmaxtouch interface */
static DEVICE_ATTR(pause_driver, 0666,
	pause_show, pause_store);
static DEVICE_ATTR(debug_enable, 0666,
	debug_enable_show, debug_enable_store);
static DEVICE_ATTR(command_calibrate, 0666,
	NULL, command_calibrate_store);
static DEVICE_ATTR(command_reset, 0666,
	NULL, command_reset_store);
static DEVICE_ATTR(command_backup, 0666,
	NULL, command_backup_store);

static struct attribute *libmaxtouch_attributes[] = {
	&dev_attr_pause_driver.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_command_calibrate.attr,
	&dev_attr_command_reset.attr,
	&dev_attr_command_backup.attr,
	NULL,
};

static struct attribute_group libmaxtouch_attr_group = {
	.attrs = libmaxtouch_attributes,
};
#endif

static struct attribute *mxt_attrs[] = {
	&dev_attr_object_show.attr,
	&dev_attr_object_write.attr,
	&dev_attr_dbg_switch.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

#endif
static int __devinit mxt_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;
	bool ta_status = 0;

	u8 **tsp_config;

	pr_info("%s +++\n", __func__);

	touch_is_pressed = 0;
#if TOUCH_BOOSTER
	tsp_press_status = 0;
#endif
	if (!pdata) {
		pr_err("missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
					sizeof(*data->fingers), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->pdata = pdata;
	data->num_fingers = pdata->max_finger_touches;
	data->power_on = pdata->power_on;
	data->power_off = pdata->power_off;
	data->register_cb = pdata->register_cb;
	data->read_ta_status = pdata->read_ta_status;

	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		pr_err("input device allocation failed\n");
		goto err_alloc_dev;
	}
	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "sec_touchscreen";

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(MT_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, MAX_USING_FINGER_NUM);

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

	copy_data = data;

	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret >= 0) {
		pr_info("boot mode. firm update excute\n");
		mxt_load_fw_bootmode(NULL, MXT_FW_NAME);
		msleep(MXT_SW_RESET_TIME);
	} else {
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
	}

	ret = mxt_init_touch_driver(data);

	data->register_cb(mxt_ta_probe);

	if (ret) {
		pr_err("chip initialization failed\n");
		goto err_init_drv;
	}

	/* tsp_family_id - 0xA1 : MXT-768E */
	if (data->family_id == 0xA1) {
		tsp_config = (u8 **)data->pdata->config;
		data->t48_config_batt = pdata->t48_config_batt;
		data->t48_config_chrg = pdata->t48_config_chrg;
		data->tchthr_batt = pdata->tchthr_batt;
		data->tchthr_charging = pdata->tchthr_charging;
		data->calcfg_batt = pdata->calcfg_batt;
		data->calcfg_charging = pdata->calcfg_charging;
		data->idlesyncsperx_batt = pdata->idlesyncsperx_batt;
		data->idlesyncsperx_charging = pdata->idlesyncsperx_charging;
		data->actvsyncsperx_batt = pdata->actvsyncsperx_batt;
		data->actvsyncsperx_charging = pdata->actvsyncsperx_charging;
		data->xloclip_batt = pdata->xloclip_batt;
		data->xloclip_charging = pdata->xloclip_charging;
		data->xhiclip_batt = pdata->xhiclip_batt;
		data->xhiclip_charging = pdata->xhiclip_charging;
		data->yloclip_batt = pdata->yloclip_batt;
		data->yloclip_charging = pdata->yloclip_charging;
		data->yhiclip_batt = pdata->yhiclip_batt;
		data->yhiclip_charging = pdata->yhiclip_charging;
		data->xedgectrl_batt = pdata->xedgectrl_batt;
		data->xedgectrl_charging = pdata->xedgectrl_charging;
		data->xedgedist_batt = pdata->xedgedist_batt;
		data->xedgedist_charging = pdata->xedgedist_charging;
		data->yedgectrl_batt = pdata->yedgectrl_batt;
		data->yedgectrl_charging = pdata->yedgectrl_charging;
		data->yedgedist_batt = pdata->yedgedist_batt;
		data->yedgedist_charging = pdata->yedgedist_charging;

#if UPDATE_ON_PROBE
#if !(FOR_DEBUGGING_TEST_DOWNLOADFW_BIN)
		if ((data->tsp_version < firmware_latest[0]
			|| (data->tsp_version == firmware_latest[0]
				&& data->tsp_build != firmware_latest[1]))
			&& (data->tsp_variant != 0)) {
			pr_info("force firmware update\n");
			if (mxt_load_fw(NULL, MXT_FW_NAME))
				goto err_config;
			else {
				msleep(MXT_SW_RESET_TIME);
				mxt_init_touch_driver(data);
			}
		}
#endif
#endif
		INIT_DELAYED_WORK(&data->config_dwork,
			mxt_reconfigration_normal);
		INIT_DELAYED_WORK(&data->check_median_error_dwork,
			mxt_check_medianfilter_error);
		INIT_DELAYED_WORK(&data->check_abnormal_palm_dwork,
			mxt_check_abnormal_palm);
#if USE_ADJUST_FRCCALRATIO
		INIT_DELAYED_WORK(&data->chk_and_adjust_frccalratio_dwork,
			mxt_adjust_frccalratio);
#endif
#if TOUCH_BOOSTER
		INIT_DELAYED_WORK(&data->dvfs_dwork,
			mxt_set_dvfs_off);
#endif
	} else {
		pr_err("ERROR : There is no valid TSP ID\n");
		goto err_config;
	}

	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
		ret = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
		if (ret)
			goto err_config;

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
	}
	ret = mxt_backup(data);
	if (ret)
		goto err_backup;

	/* reset the touch IC. */
	ret = mxt_reset(data);
	if (ret)
		goto err_reset;

	msleep(MXT_SW_RESET_TIME);

	calibrate_chip_e();

	for (i = 0; i < data->num_fingers; i++)
		data->fingers[i].state = MXT_STATE_INACTIVE;

	ret = request_threaded_irq(client->irq, NULL, mxt_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt_ts", data);

	if (ret < 0)
		goto err_irq;

#if SYSFS
	ret = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (ret)
		pr_err("sysfs_create_group()is falled\n");


#if ITDEV
	ret = sysfs_create_group(&client->dev.kobj, &libmaxtouch_attr_group);
	if (ret) {
		pr_err("Failed to create libmaxtouch sysfs group\n");
		goto err_irq;
	}

	sysfs_bin_attr_init(&mem_access_attr);
	mem_access_attr.attr.name = "mem_access";
	mem_access_attr.attr.mode = S_IRUGO | S_IWUGO;
	mem_access_attr.read = mem_access_read;
	mem_access_attr.write = mem_access_write;
	mem_access_attr.size = 65535;

	if (sysfs_create_bin_file(&client->dev.kobj, &mem_access_attr) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			mem_access_attr.attr.name);
		goto err_irq;
	}
#endif

	sec_touchscreen = device_create(sec_class,
		NULL, 0, NULL, "sec_touchscreen");
	dev_set_drvdata(sec_touchscreen, data);
	if (IS_ERR(sec_touchscreen))
		pr_err("Failed to create device(sec_touchscreen)!\n");

	if (device_create_file(sec_touchscreen,
				&dev_attr_tsp_firm_update) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_update.attr.name);

	if (device_create_file(sec_touchscreen,
				&dev_attr_tsp_firm_update_status) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_update_status.attr.name);

	if (device_create_file(sec_touchscreen,
				&dev_attr_tsp_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_threshold.attr.name);

	if (device_create_file(sec_touchscreen,
				&dev_attr_tsp_firm_version_phone) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_version_phone.attr.name);

	if (device_create_file(sec_touchscreen,
				&dev_attr_tsp_firm_version_panel) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_tsp_firm_version_panel.attr.name);

	if (device_create_file(sec_touchscreen,
				&dev_attr_mxt_touchtype) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mxt_touchtype.attr.name);

	mxt_noise_test = device_create(sec_class,
		NULL, 0, NULL, "tsp_noise_test");

	if (IS_ERR(mxt_noise_test))
		pr_err("Failed to create device(mxt_noise_test)!\n");

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer0) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_refer0.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta0) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_delta0.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer1) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_refer1.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta1) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_delta1.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer2) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_refer2.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta2) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_delta2.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer3) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_refer3.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta3) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_delta3.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer4) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_refer4.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta4) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_delta4.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_all_refer) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_all_refer.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_disp_all_refdata) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_disp_all_refdata.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_all_delta) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_all_delta.attr.name);

	if (device_create_file(mxt_noise_test,
				&dev_attr_disp_all_deltadata) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_disp_all_deltadata.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_threshold.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_firm_version) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_firm_version.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_module_off) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_module_off.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_module_on) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_set_module_on.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_x_line) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_x_line.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_y_line) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_y_line.attr.name);

	if (device_create_file(mxt_noise_test,
				&dev_attr_find_delta_channel) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_find_delta_channel.attr.name);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG_ON_BOOT);
	mxt_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		pr_info("ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}
	return 0;

err_irq:
err_reset:
err_backup:
err_config:
	kfree(data->objects);
err_init_drv:
	gpio_free(data->gpio_read_done);
/* err_gpio_req:
	data->power_off();
	input_unregister_device(input_dev); */
err_reg_dev:
err_alloc_dev:
	kfree(data);
	return ret;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

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

static struct i2c_device_id mxt_idtable[] = {
	{MXT_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend = mxt_suspend,
	.resume = mxt_resume,
};

static struct i2c_driver mxt_i2c_driver = {
	.id_table = mxt_idtable,
	.probe = mxt_probe,
	.remove = __devexit_p(mxt_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT_DEV_NAME,
		.pm	= &mxt_pm_ops,
	},
};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_i2c_driver);
}

static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_i2c_driver);
}
module_init(mxt_init);
module_exit(mxt_exit);

MODULE_DESCRIPTION("Atmel MaXTouch driver");
MODULE_AUTHOR("Heetae Ahn <heetae82.ahn@samsung.com>");
MODULE_LICENSE("GPL");
