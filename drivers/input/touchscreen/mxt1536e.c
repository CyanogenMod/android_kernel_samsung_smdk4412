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
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt1536e.h>
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
#define SUPPRESS_MSG_MASK		0x02

/* Version */
#define MXT224_VER_20			20
#define MXT224_VER_21			21
#define MXT224_VER_22			22

/* Slave addresses */
#define MXT_APP_LOW		0x4a
#define MXT_APP_HIGH		0x4b
#define MXT_BOOT_LOW		0x24
#define MXT_BOOT_HIGH		0x25

#define MXT1536E_APP_LOW		0x4c
#define MXT1536E_APP_HIGH		0x4d //0x4d
#define MXT1536E_BOOT_LOW		0x26
#define MXT1536E_BOOT_HIGH		0x27 //0x27


/* FIRMWARE NAME */
#define MXT224_ECHO_FW_NAME	    "mXT224E.fw"
#define MXT224_FW_NAME		    "mXT224.fw"
#define MXT768E_FW_NAME			"mXT768e.fw"

#define MXT_FWRESET_TIME		175	/* msec */
#define MXT_RESET_TIME		1000	/* msec */

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

/*#define DRIVER_FILTER*/

#define MXT_STATE_INACTIVE		-1
#define MXT_STATE_RELEASE		0
#define MXT_STATE_PRESS		1
#define MXT_STATE_MOVE		2

#define MAX_USING_FINGER_NUM 10

/* touch booster */
#define TOUCH_BOOSTER			0 // 1
#define TOUCH_BOOSTER_TIME		3000
#define SYSFS	0
#define BRINGUP  0
#define UPDATE   1
#if TOUCH_BOOSTER
#include <mach/cpufreq.h>
#endif

/* Add for debugging */
/*####################*/
/*#define FOR_DEBUGGING_TEST*/
/*#define FOR_DEBUGGING_TEST_DOWNLOADFW_BIN*/
/*#define ITDEV //hmink*/
#define SHOW_COORDINATE
/*####################*/

#ifdef ITDEV
static int driver_paused;//itdev
static int debug_enabled;//itdev
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
	s8 state;
	int16_t component;
	u16 mcount;	/*add for debug*/
};

typedef struct
{
    u8 object_type;     /*!< Object type. */
    u8 instance;        /*!< Instance number. */
} report_id_map_t;

u8 max_report_id;
report_id_map_t *rid_map;
static bool rid_map_alloc;

struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct early_suspend early_suspend;
	u8 family_id;
	u32 finger_mask;
	int gpio_read_done;
	struct object_t *objects;
	u8 objects_len;
	u8 tsp_version;
	u8 tsp_build;
	u8 tsp_variant;
	const u8 *power_cfg;
	u8 finger_type;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;
	u8 chrgtime_batt;
	u8 chrgtime_charging;
	u8 atchcalst;
	u8 atchcalsthr;
	u8 tchthr_batt;
	u8 tchthr_charging;
	u8 calcfg_batt_e;
	u8 calcfg_charging_e;
	u8 atchcalsthr_e;
	u8 atchfrccalthr_e;
	u8 atchfrccalratio_e;
	u8 idlesyncsperx_batt;
	u8 idlesyncsperx_charging;
	u8 actvsyncsperx_batt;
	u8 actvsyncsperx_charging;
	u8 idleacqint_batt;
	u8 idleacqint_charging;
	u8 actacqint_batt;
	u8 actacqint_charging;
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
	u8 tchhyst_batt;
	u8 tchhyst_charging;
	const u8 *t48_config_batt_e;
	const u8 *t48_config_chrg_e;
	struct delayed_work config_dwork;
	struct delayed_work overflow_dwork;
	struct delayed_work check_tchpress_dwork;
	struct delayed_work check_median_error_dwork;
#if TOUCH_BOOSTER
	struct delayed_work dvfs_dwork;
#endif
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(void *);
	void (*read_ta_status)(bool *);
	int num_fingers;
#ifdef ITDEV
	u16 last_read_addr;
	u16 msg_proc_addr;
#endif
	struct finger_info fingers[];
};

struct mxt_data *copy_data;
extern struct class *sec_class;
int touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);
static int mxt_enabled;
static bool g_debug_switch;
static u8 threshold;
static int firm_status_data;
static bool deepsleep;
void Mxt_force_released(void);
static void mxt_ta_probe(int ta_status);

/* add for protection code +*/
static bool overflow_deteted;
static bool first_touch_detected;
static int config_dwork_flag;
static int treat_median_error_status;
static int tchcount_aft_median_error;

#define CNTLMTTCH_AFT_MEDIAN_ERROR	10

#define TIME_FOR_OVERFLOWCLEAR		2	/*2 sec */
#define TIME_FOR_RECALIBRATION		3	/*3 sec */
#define TIME_FOR_RECONFIG				10	/*10 sec*/
#define TIME_FOR_RECONFIG_ON_REPET	7	/*7 sec */
#define TIME_FOR_RECONFIG_ON_BOOT	30	/*30 sec*/
#define TIME_FOR_CHECK_MEDIAN_ERROR	2	/*2 sec*/

#define CAL_FROM_BOOTUP			0
#define CAL_FROM_RESUME			1
#define CAL_BEF_WORK_CALLED		2
#define CAL_REP_WORK_CALLED		3
#define CAL_AFT_WORK_CALLED		4
/* add for protection code -*/

#if TOUCH_BOOSTER
static bool tsp_press_status;
static bool touch_cpu_lock_status;
static int cpu_lv = -1;
#endif

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

	if (ret < 0){
		printk(KERN_ERR"[TSP]i2c failed ret = %d\n",ret);
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
	/*
	if (reg==(data->cmd_proc + CMD_CALIBRATE_OFFSET))
		printk(KERN_ERR"[TSP] write calibrate_command ret is %d, size is %d\n",ret,sizeof(tmp));
	*/

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

	if(size ==0 && address == 0) return 0;
	else return write_mem(data, address, size, cfg);
}

static int check_instance(struct mxt_data *data, u8 object_type)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			return (data->objects[i].instances);
		}
	}
	return 0;
}

static int init_write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address = 0;
	u16 size = 0;
#if 0
	u8 *temp;
#endif
	ret = get_object_info(data, type, &size, &address);

	//printk("[TSP]%s:type=%d,size=%d,adress=%d\n",__func__,type,size,address);
	if((size == 0) || (address == 0)) return 0;

	ret = write_mem(data, address, size, cfg);
#if 0
	if (check_instance(data, type)) {
		printk("[TSP] exist instance1 object (%d)\n", type);
		temp = kmalloc(size * sizeof(u8), GFP_KERNEL);
		memset(temp, 0, size);
		ret |= write_mem(data, address+size, size, temp);
		kfree(temp);
	}
#endif
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
	if (!ret) {
		printk(KERN_DEBUG "[TSP] calibration success!!!\n");
	}
	return ret;
}

static unsigned int mxt_time_point;
static unsigned int mxt_time_diff;
static unsigned int mxt_timer_state;
static unsigned int good_check_flag;
static u8 cal_check_flag;

uint8_t calibrate_chip(void)
{
	u8 cal_data = 1;
	int ret = 0;
	u8 atchcalst_tmp, atchcalsthr_tmp;
	u16 obj_address = 0;
	u16 size = 1;
	int ret1 = 0;

/*	printk(KERN_ERR"[TSP]ret is %d,ret1 is %d\n",ret,ret1); */

	if (cal_check_flag == 0) {

		ret = get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		size = 1;

		/* change calibration suspend settings to zero until calibration confirmed good */
		/* store normal settings */
		/* read_mem(copy_data, obj_address+6, (u8)size, &atchcalst); */
		/* read_mem(copy_data, obj_address+7, (u8)size, &atchcalsthr); */

		/* resume calibration must be performed with zero settings */
		atchcalst_tmp = 0;
		atchcalsthr_tmp = 0;

		ret = write_mem(copy_data, obj_address+6, size, &atchcalst_tmp);
		ret1 = write_mem(copy_data, obj_address+7, size, &atchcalsthr_tmp);

		/* Write temporary acquisition config to chip. */
		/*
		if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
			"Acquisition config write failed!\n"
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			ret = WRITE_MEM_FAILED; calling function should retry calibration call
		}*/

		/* restore settings to the local structure so that when we confirm the
		 * cal is good we can correct them in the chip
		 * this must be done before returning */
		/*
		printk(KERN_ERR"[TSP] acquition restore! atchcalst=%d, atchcalsthr=%d\n", atchcalst, atchcalsthr );
		write_mem(copy_data, obj_address+6, size, &atchcalst);
		write_mem(copy_data, obj_address+7, size, &atchcalsthr);
		*/
	}

	/* send calibration command to the chip */
	if (!ret && !ret1 /*&& !Doing_calibration_falg*/) {
		/* change calibration suspend settings to zero until calibration confirmed good */
		ret = write_mem(copy_data, copy_data->cmd_proc + CMD_CALIBRATE_OFFSET, 1, &cal_data);
		/* msleep(5); */

		/*read_mem(copy_data, copy_data->cmd_proc+2, (u8)size, &value);
		printk(KERN_ERR"[TSP] calibration data is %d\n",value);*/

		/* set flag for calibration lockup recovery if cal command was successful */
		if (!ret) {
			/* set flag to show we must still confirm if calibration was good or bad */
			cal_check_flag = 1u;
			/* Doing_calibration_falg = 1; */
			printk(KERN_ERR "[TSP] calibration success!!!\n");
		}

	}
	return ret;
}

#if 0
static void treat_force_reset(struct mxt_data *data)
{
	int i;
	bool ta_status;
	int count = 0;

	printk(KERN_ERR "[TSP] %s\n", __func__);

	disable_irq(data->client->irq);
	mxt_internal_suspend(data);

	msleep(50);
	data->power_on();

	mxt_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_DEBUG "[TSP] ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}

	config_dwork_flag = CAL_FROM_RESUME;
	overflow_deteted = 0;
	first_touch_detected = 0;
	treat_median_error_status = 0;
	calibrate_chip_e();

	enable_irq(data->client->irq);
}
#endif

static void treat_error_status_e(void)
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
		printk(KERN_ERR "[TSP] Error status already treated\n");
#if 0
		treat_median_error_status++;
		if (treat_median_error_status > 2) {
			treat_median_error_status = 0;
			calibrate_chip_e;
		}
#endif
		return;
	} else
		treat_median_error_status = 1;

	if (ta_status) {
		printk(KERN_ERR "[TSP] Error status TA is[%d]\n", ta_status);

		get_object_info(data,
			GEN_POWERCONFIG_T7, &size, &obj_address);
		/* 1:ACTVACQINT */
		error = change_config(data, obj_address, 1, 255);

		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		/* 0:CHRGTIME */
		error |= change_config(data, obj_address, 0, 64);
		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);

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
		/* 2:CALCFG */
		error |= change_config(data, obj_address, 2, 114);
		/* 3:BASEFREQ */
		error |= change_config(data, obj_address, 3, 15);
		/* 8:MFFREQ[0] */
		error |= change_config(data, obj_address, 8, 3);
		/* 9:MFFREQ[1] */
		error |= change_config(data, obj_address, 9, 5);
		/* 10:NLGAIN*/
		error |= change_config(data, obj_address, 10, 96);
		/* 11:NLTHR*/
		error |= change_config(data, obj_address, 11, 30);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 100);
		/* 34:BLEN[0] */
		error |= change_config(data, obj_address, 34, 80);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 40);
		/* 36:TCHDI[0] */
		error |= change_config(data, obj_address, 36, 2);
		/* 39:MOVFILTER[0] */
		error |= change_config(data, obj_address, 39, 65);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 43:XLOCLIP[0] */
		error |= change_config(data, obj_address, 43, 5);
		/* 44:XHICLIP[0] */
		error |= change_config(data, obj_address, 44, 5);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 15);

		if (error < 0)
			printk(KERN_ERR "[TSP] failed to write error status\n");
	} else {
		printk(KERN_ERR "[TSP] Error status TA is[%d]\n", ta_status);

		get_object_info(data,
			GEN_POWERCONFIG_T7, &size, &obj_address);
		/* 1:ACTVACQINT */
		error = change_config(data, obj_address, 1, 255);

		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		/* 0:CHRGTIME */
		error |= change_config(data, obj_address, 0, 64);
		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);

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
		error |= change_config(data, obj_address, 2, 48);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address, 3, 48);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 2:CALCFG */
		error |= change_config(data, obj_address, 2, 242);
		/* 3:BASEFREQ */
		error |= change_config(data, obj_address, 3, 15);
		/* 8:MFFREQ[0] */
		error |= change_config(data, obj_address, 8, 3);
		/* 9:MFFREQ[1] */
		error |= change_config(data, obj_address, 9, 5);
		/* 10:NLGAIN*/
		error |= change_config(data, obj_address, 10, 112);
		/* 11:NLTHR*/
		error |= change_config(data, obj_address, 11, 25);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 100);
		/* 34:BLEN[0] */
		error |= change_config(data, obj_address, 34, 112);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 40);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 15);

		if (error < 0)
			printk(KERN_ERR "[TSP] failed to write error status\n");
	}
}

/* this function is called in irq routine */
static void treat_calibration_state(struct mxt_data *data)
{
	u16 size;
	u16 obj_address = 0;
	int error = 0;

	first_touch_detected = 0;
	overflow_deteted = 0;
	cancel_delayed_work(&data->check_tchpress_dwork);
	cancel_delayed_work(&data->overflow_dwork);

	 if (config_dwork_flag
			== CAL_FROM_RESUME) {
		config_dwork_flag = CAL_BEF_WORK_CALLED;
	} else if (config_dwork_flag
				== CAL_BEF_WORK_CALLED) {
		cancel_delayed_work(&data->config_dwork);
		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG);
	} else if (config_dwork_flag
				== CAL_REP_WORK_CALLED) {
		cancel_delayed_work(&data->config_dwork);
		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG_ON_REPET);
	} else if (config_dwork_flag
				== CAL_AFT_WORK_CALLED) {

		get_object_info(data,
			GEN_POWERCONFIG_T7, &size, &obj_address);
		/* 1 :ACTVACQINT*/
		error = change_config(data, obj_address, 1, 16);
		/* 2 :ACTV2IDLETO*/
		error |= change_config(data, obj_address, 2, 7);

		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 8);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 136);

		get_object_info(data,
			TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		/* 31:TCHHYST */
		error |= change_config(data, obj_address, 31, 0);

		get_object_info(data,
			PROCI_TOUCHSUPPRESSION_T42, &size, &obj_address);
		/* 0:CTRL */
		error |= change_config(data, obj_address, 0, 51);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 5);

		if (error < 0)
			printk(KERN_ERR "[TSP] %s write config Error!!\n"
						, __func__);
		config_dwork_flag = CAL_BEF_WORK_CALLED;

		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG);
	}
}

/* mxt224E reconfigration */
static void mxt_reconfigration_normal(struct work_struct *work)
{
	u16 size;
	int error = 0;

	struct mxt_data *data =
		container_of(work, struct mxt_data, config_dwork.work);
	u16 obj_address = 0;

	if (mxt_enabled) {
		disable_irq(data->client->irq);

		if (first_touch_detected) {
			get_object_info(data,
				GEN_POWERCONFIG_T7, &size, &obj_address);
			/* 1 :ACTVACQINT*/
			error = change_config(data, obj_address, 1, 255);
			/* 2 :ACTV2IDLETO*/
			error |= change_config(data, obj_address, 2, 25);

			get_object_info(data,
				GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
			/* 8:ATCHFRCCALTHR*/
			error |= change_config(data, obj_address, 8, 50);
			/* 9:ATCHFRCCALRATIO*/
			error |= change_config(data, obj_address, 9, 0);

			get_object_info(data,
				TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
			/* 31:TCHHYST */
			error |= change_config(data, obj_address, 31, 15);

			get_object_info(data,
				PROCI_TOUCHSUPPRESSION_T42,
				&size, &obj_address);
			/* 0:CTRL */
			error |= change_config(data, obj_address, 0, 3);

			get_object_info(data,
				PROCG_NOISESUPPRESSION_T48,
				&size, &obj_address);
			/* 52:TCHHYST[0] */
			error |= change_config(data, obj_address, 52, 15);

			if (error < 0)
				printk(KERN_ERR "[TSP] ta_probe write config Error!!\n");

			printk(KERN_DEBUG "[TSP] %s execute !!\n", __func__);

			config_dwork_flag = CAL_AFT_WORK_CALLED;
		} else {
			cancel_delayed_work(&data->config_dwork);
			schedule_delayed_work(&data->config_dwork
				, HZ*TIME_FOR_RECONFIG_ON_REPET);
			config_dwork_flag = CAL_REP_WORK_CALLED;
		}

		enable_irq(data->client->irq);
	}
	return;
}

static void mxt_calbration_by_overflowed(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, overflow_dwork.work);
	u8 id;

	if (mxt_enabled) {
		disable_irq(data->client->irq);
		printk(KERN_DEBUG "[TSP] %s execute [%s]\n",
			__func__ , (overflow_deteted == 1) ? "T" : "F");
		if (overflow_deteted) {
			for (id = 0 ; id < MAX_USING_FINGER_NUM ; ++id) {
				if (data->fingers[id].state
					== MXT_STATE_INACTIVE)
					continue;

				if (data->fingers[id].mcount > 10) {
					enable_irq(data->client->irq);
					return;
				}
			}
			calibrate_chip_e();
		}
		enable_irq(data->client->irq);
	}
}

static void mxt_calibration_by_notch_after_overflowed(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, check_tchpress_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);
		printk(KERN_DEBUG "[TSP] %s execute!![%s]\n",
			__func__ , (first_touch_detected == 1) ? "F" : "T");
		if (!first_touch_detected)
			calibrate_chip_e();
		first_touch_detected = 0;
		enable_irq(data->client->irq);
	}
}

static void mxt_check_medianfilter_error(struct work_struct *work)
{
	printk(KERN_DEBUG "[TSP] %s [%d]\n",
		__func__, tchcount_aft_median_error);

	if (tchcount_aft_median_error
		>= CNTLMTTCH_AFT_MEDIAN_ERROR) {
		calibrate_chip_e();
	}
	tchcount_aft_median_error = 0;
}

#if TOUCH_BOOSTER
static void mxt_set_dvfs_off(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, dvfs_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);
		if (touch_cpu_lock_status
			&& !tsp_press_status){
			s5pv310_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
			touch_cpu_lock_status = 0;
		}
		enable_irq(data->client->irq);
	}
}

static void mxt_set_dvfs_on(struct mxt_data *data)
{
	cancel_delayed_work(&data->dvfs_dwork);
	if (cpu_lv < 0)
		cpu_lv =	s5pv310_cpufreq_round_idx(CPUFREQ_500MHZ);
	s5pv310_cpufreq_lock(DVFS_LOCK_ID_TSP, cpu_lv);
	touch_cpu_lock_status = 1;
}
#endif

static int check_abs_time(void)
{
	if (!mxt_time_point)
		return 0;

	mxt_time_diff = jiffies_to_msecs(jiffies) - mxt_time_point;

	if (mxt_time_diff > 0)
		return 1;
	else
		return 0;
}

void check_chip_calibration(void)
{
	u8 data_buffer[100] = { 0 };
	u8 try_ctr = 0;
	u8 data_byte = 0xF3; /* dianostic command to get touch flags */
	u8 tch_ch = 0, atch_ch = 0;
	/* u8 atchcalst, atchcalsthr; */
	u8 check_mask;
	u8 i, j = 0;
	u8 x_line_limit;
	int ret;
	u16 size;
	u16 object_address = 0;


	/* we have had the first touchscreen or face suppression message
	* after a calibration - check the sensor state and try to confirm if
	* cal was good or bad */

	/* get touch flags from the chip using the diagnostic object */
	/* write command to command processor to get touch flags - 0xF3 Command required to do this */
	/* write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte); */
	write_mem(copy_data, copy_data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1, &data_byte);


	/* get the address of the diagnostic object so we can get the data we need */
	/* diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0); */
	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);

	msleep(10);

	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	memset(data_buffer , 0xFF, sizeof(data_buffer));

	/* wait for diagnostic object to update */
	while (!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))) {
		/* wait for data to be valid  */
		if (try_ctr > 10) {

			/* Failed! */
			printk(KERN_ERR"[TSP] Diagnostic Data did not update!!\n");
			mxt_timer_state = 0;
			break;
		}

		mdelay(2);
		try_ctr++; /* timeout counter */
		/* read_mem(diag_address, 2,data_buffer); */

		read_mem(copy_data, object_address, 2, data_buffer);
	}


	/* data is ready - read the detection flags */
	/* read_mem(diag_address, 82,data_buffer); */
	read_mem(copy_data, object_address, 82, data_buffer);


	/* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/

	/* count up the channels/bits if we recived the data properly */
	if ((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)) {

		/* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4.*/
		/* x_line_limit = 16 + cte_config.mode; */
		x_line_limit = 16 + 3;

		if (x_line_limit > 20) {
			/* hard limit at 20 so we don't over-index the array */
			x_line_limit = 20;
		}

		/* double the limit as the array is in bytes not words */
		x_line_limit = x_line_limit << 1;

		/* count the channels and print the flags to the log */
		for (i = 0; i < x_line_limit; i += 2) { /* check X lines - data is in words so increment 2 at a time */

			/* print the flags to the log - only really needed for debugging */

			/* count how many bits set for this row */
			for (j = 0; j < 8; j++) {
				/* create a bit mask to check against */
				check_mask = 1 << j;

				/* check detect flags */
				if (data_buffer[2+i] & check_mask)
					tch_ch++;

				if (data_buffer[3+i] & check_mask)
					tch_ch++;

				/* check anti-detect flags */
				if (data_buffer[42+i] & check_mask)
					atch_ch++;

				if (data_buffer[43+i] & check_mask)
					atch_ch++;

			}
		}

		printk(KERN_ERR"[TSP] t: %d, a: %d\n", tch_ch, atch_ch);

		/* send page up command so we can detect when data updates next time,
		* page byte will sit at 1 until we next send F3 command */
		data_byte = 0x01;

		/* write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte); */
		write_mem(copy_data, copy_data->cmd_proc + CMD_DIAGNOSTIC_OFFSET, 1, &data_byte);


		/* process counters and decide if we must re-calibrate or if cal was good */
		if ((tch_ch > 0) && (atch_ch == 0)) {  /* jwlee change. */
			/* cal was good - don't need to check any more */
			if (!check_abs_time())
				mxt_time_diff = 501;

			if (mxt_timer_state == 1) {
				if (mxt_time_diff > 500) {
					printk(KERN_ERR"[TSP] calibration was good\n");
					cal_check_flag = 0;
					good_check_flag = 0;
					mxt_timer_state = 0;
					mxt_time_point = jiffies_to_msecs(jiffies);

					ret = get_object_info(copy_data, GEN_ACQUISITIONCONFIG_T8, &size, &object_address);

					/* change calibration suspend settings to zero until calibration confirmed good */
					/* store normal settings */
					size = 1;
					write_mem(copy_data, object_address+6, size, &copy_data->atchcalst);
					write_mem(copy_data, object_address+7, size, &copy_data->atchcalsthr);


					/* dprintk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n", acquisition_config.atchcalst, acquisition_config.atchcalsthr ); */

					/*
					Write normal acquisition config back to the chip.
					if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
						"Acquisition config write failed!\n"
						printk(KERN_DEBUG "\n[TSP][ERROR] line : %d\n", __LINE__);
						MUST be fixed
					}
					*/

				} else  {
					cal_check_flag = 1;
				}
			} else {
				mxt_timer_state = 1;
				mxt_time_point = jiffies_to_msecs(jiffies);
				cal_check_flag = 1;
			}

		} else if (atch_ch >= 5) {
			/* cal was bad - must recalibrate and check afterwards */
			printk(KERN_ERR"[TSP] calibration was bad\n");
			calibrate_chip();
			mxt_timer_state = 0;
			mxt_time_point = jiffies_to_msecs(jiffies);
		} else {
			/* we cannot confirm if good or bad - we must wait for next touch  message to confirm */
			printk(KERN_ERR"[TSP] calibration was not decided yet\n");
			cal_check_flag = 1u;
			mxt_timer_state = 0;
			mxt_time_point = jiffies_to_msecs(jiffies);
		}
	}
}

static void mxt_ta_probe(int ta_status)
{
	u16 obj_address = 0;
	u16 size;
	u8 value;
	int error;

	struct mxt_data *data = copy_data;

	if (!mxt_enabled) {
		printk(KERN_ERR"mxt_enabled is 0\n");
		return;
	}

	if (data->family_id == 0xA1) {	/* MXT-768E */

		if (treat_median_error_status) {
			treat_median_error_status = 0;
			tchcount_aft_median_error = 0;
		}
		if (ta_status) {
			get_object_info(data,
				GEN_POWERCONFIG_T7, &size, &obj_address);
			/* 0:IDLEACQINT */
			error = change_config(data, obj_address,
						0, data->idleacqint_charging);
			/* 1:ACTVACQINT */
			error |= change_config(data, obj_address,
						1, data->actacqint_charging);

			get_object_info(data,
				GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
			/* 0:CHRGTIME */
			error |= change_config(data, obj_address,
						0, data->chrgtime_charging);

			get_object_info(data,
				TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
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

			error |= write_config(data, data->t48_config_chrg_e[0],
						data->t48_config_chrg_e + 1);

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
				printk(KERN_ERR "[TSP] ta_probe write config Error!!\n");
		} else {
			get_object_info(data,
				GEN_POWERCONFIG_T7, &size, &obj_address);
			/* 0:IDLEACQINT */
			error = change_config(data, obj_address,
					0, data->idleacqint_batt);
			/* 1:ACTVACQINT */
			error |= change_config(data, obj_address,
					1, data->actacqint_batt);

			get_object_info(data,
				GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
			/* 0:CHRGTIME */
			error |= change_config(data, obj_address,
					0, data->chrgtime_batt);

			get_object_info(data,
				TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
			error |= change_config(data, obj_address,
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
			error |= change_config(data, obj_address,
						31, data->tchhyst_batt);

			error |= write_config(data, data->t48_config_batt_e[0],
						data->t48_config_batt_e + 1);

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
				printk(KERN_ERR "[TSP] ta_probe write config Error!!\n");
		}
	}
	else if (data->family_id == 0x81) {	/*  : MXT-224E */
		get_object_info(data, PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		if (ta_status) {
			error = write_config(data, data->t48_config_chrg_e[0], data->t48_config_chrg_e + 1);
			value = data->calcfg_charging_e | 0x20;
			threshold = data->tchthr_charging;
		} else {
			error = write_config(data, data->t48_config_batt_e[0], data->t48_config_batt_e + 1);
			value = data->calcfg_batt_e | 0x20;
			threshold = data->tchthr_batt;
		}
		error |= write_mem(data, obj_address+2, 1, &value);
		if (error < 0) printk(KERN_ERR "[TSP] mxt TA/USB mxt_noise_suppression_config Error!!\n");
	}
	else if (data->family_id == 0x80) {	/*  : MXT-224 */
		if (ta_status) {
			threshold = data->tchthr_charging;
		}
		else {
			threshold = data->tchthr_batt;
		}
		get_object_info(data, TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		write_mem(data, obj_address+7, 1, &threshold);
	}
	printk("[TSP] threshold : %d\n", threshold);
};

#if defined(DRIVER_FILTER)
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MAX_USING_FINGER_NUM] = { 0, };
	static u16 pre_x[MAX_USING_FINGER_NUM][4] = {{0}, };
	static u16 pre_y[MAX_USING_FINGER_NUM][4] = {{0}, };
	int coff[4] = {0,};
	int distance = 0;

	if (detect)
	{
		tcount[id] = 0;
	}

	pre_x[id][tcount[id]%4] = *px;
	pre_y[id][tcount[id]%4] = *py;

	if (tcount[id] > 3)
	{
		{
			distance = abs(pre_x[id][(tcount[id]-1)%4] - *px) + abs(pre_y[id][(tcount[id]-1)%4] - *py);

			coff[0] = (u8)(2 + distance/5);
			if (coff[0] < 8) {
				coff[0] = max(2, coff[0]);
				coff[1] = min((8 - coff[0]), (coff[0]>>1)+1);
				coff[2] = min((8 - coff[0] - coff[1]), (coff[1]>>1)+1);
				coff[3] = 8 - coff[0] - coff[1] - coff[2];

				/* printk(KERN_DEBUG "[TSP] %d, %d, %d, %d", coff[0], coff[1], coff[2], coff[3]); */

				*px = (u16)((*px*(coff[0]) + pre_x[id][(tcount[id]-1)%4]*(coff[1])
					+ pre_x[id][(tcount[id]-2)%4]*(coff[2]) + pre_x[id][(tcount[id]-3)%4]*(coff[3]))/8);
				*py = (u16)((*py*(coff[0]) + pre_y[id][(tcount[id]-1)%4]*(coff[1])
					+ pre_y[id][(tcount[id]-2)%4]*(coff[2]) + pre_y[id][(tcount[id]-3)%4]*(coff[3]))/8);
			} else {
				*px = (u16)((*px*4 + pre_x[id][(tcount[id]-1)%4])/5);
				*py = (u16)((*py*4 + pre_y[id][(tcount[id]-1)%4])/5);
			}
		}
	}
	tcount[id]++;
}
#endif  /* DRIVER_FILTER */

uint8_t reportid_to_type(struct mxt_data *data, u8 report_id, u8 *instance)
{
	report_id_map_t *report_id_map;
	report_id_map = rid_map;

	if (report_id <= max_report_id) {
		*instance = report_id_map[report_id].instance;
		return (report_id_map[report_id].object_type);
	} else
		return 0;
}

static int __devinit mxt_init_touch_driver(struct mxt_data *data)
{
	struct object_t *object_table;
#if 0
	u32 read_crc = 0;
	u32 calc_crc;
#endif
	u16 crc_address;
	u16 dummy;
	int i, j;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;
#if 0
	u16 size, obj_address;
	u8 value;
#endif
	int current_report_id, start_report_id;

	ret = read_mem(data, 0, sizeof(id), id);

	if (ret)
		return ret;

	dev_info(&data->client->dev, "family = %#02x, variant = %#02x, version "
			"= %#02x, build = %d\n", id[0], id[1], id[2], id[3]);
	printk(KERN_ERR" family = %#02x, variant = %#02x, version "
			"= %#02x, build = %d\n", id[0], id[1], id[2], id[3]);
	dev_dbg(&data->client->dev, "matrix X size = %d\n", id[4]);
	dev_dbg(&data->client->dev, "matrix Y size = %d\n", id[5]);
	printk("matrix X size = %d, Y size = %d\n", id[4],id[5]);
	
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
		object_table[i].i2c_address = le16_to_cpu(object_table[i].i2c_address);
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
			dev_dbg(&data->client->dev, "Finger type = %d\n",
						data->finger_type);

			break;
		case GEN_MESSAGEPROCESSOR_T5:
#ifdef ITDEV
			data->msg_proc_addr = object_table[i].i2c_address;
#endif
			data->msg_object_size = object_table[i].size + 1;
			dev_dbg(&data->client->dev, "Message object size = "
						"%d\n", data->msg_object_size);

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
				for (start_report_id = current_report_id; current_report_id <
						(start_report_id + object_table[i].num_report_ids);
						current_report_id++) {
					rid_map[current_report_id].instance = j;
					rid_map[current_report_id].object_type = object_table[i].object_type;
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
#if 0
	ret = read_mem(data, crc_address, 3, (u8 *)&read_crc);
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
#endif
	ret = get_object_info(data, GEN_MESSAGEPROCESSOR_T5, &dummy,
					&data->msg_proc);
	if (ret)
		goto err;

	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &dummy,
					&data->cmd_proc);
	if (ret)
		goto err;

	printk("[TSP]mxt_init_touch_driver success\n");
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
	//int press_count = 0;
	//int move_count = 0;

	for (i = 0; i < data->num_fingers; i++) {
//	for (i = 0; i < 1; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		if(data->fingers[i].state == MXT_STATE_RELEASE) {
			input_mt_slot(data->input_dev,i);
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, false);
		}
		else {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					   MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, data->fingers[i].x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, data->fingers[i].y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, data->fingers[i].z);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE, data->fingers[i].w);

#ifdef _SUPPORT_SHAPE_TOUCH_
			input_report_abs(data->input_dev, ABS_MT_COMPONENT, data->fingers[i].component);
			/* printk(KERN_ERR"the component data is %d\n",data->fingers[i].component); */
#endif
		}
		report_count++;

#ifdef SHOW_COORDINATE
		switch(data->fingers[i].state) {
		case MXT_STATE_PRESS:
			printk(KERN_ERR "[TSP] Pressed [%d] (%d, %d) ,z=%d,w=%d,mc=%d\n",
				i , data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].z, data->fingers[i].w
				, data->fingers[i].mcount);
			break;

		case MXT_STATE_MOVE:
		/*
			printk(KERN_ERR "[TSP] Moved [%d] (%d, %d) ,z=%d,w=%d,mc=%d\n",
				i , data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].z, data->fingers[i].w
				, data->fingers[i].mcount);
	*/	
			break;

		case MXT_STATE_RELEASE:
			printk(KERN_ERR "[TSP] Released [%d]\n", i);
			break;
		default :
			break;
		}
#if 0
		if (data->fingers[i].state == MXT_STATE_PRESS
			|| data->fingers[i].state == MXT_STATE_RELEASE
			|| data->fingers[i].state == MXT_STATE_MOVE) {
			printk(KERN_ERR "[TSP] id[%d],x=%d,y=%d,z=%d,w=%d,mc=%d\n",
				i , data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].z, data->fingers[i].w
				, data->fingers[i].mcount);
		}
#endif
#else
		if (data->fingers[i].state == MXT_STATE_PRESS)
			printk(KERN_ERR "[TSP] P: id[%d]\n", i);
		else if (data->fingers[i].state == MXT_STATE_RELEASE)
			printk(KERN_ERR "[TSP] R: id[%d],mc=%d\n"
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
		}
		else {
			data->fingers[i].state = MXT_STATE_MOVE;
			count++;
		}
	}
	if (report_count > 0) {
#ifdef ITDEV
		if (!driver_paused)
#endif
			input_sync(data->input_dev);
	}

	if (count) touch_is_pressed = 1;
	else touch_is_pressed = 0;

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
	//printk("[TSP]mxt_irq_thread");
	do {
		touch_message_flag = 0;
		if (read_mem(data, data->msg_proc, sizeof(msg), msg)) {
#if TOUCH_BOOSTER
			if (touch_cpu_lock_status) {
				s5pv310_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
				touch_cpu_lock_status = 0;
			}
#endif
			return IRQ_HANDLED;
		}
#ifdef ITDEV//itdev
	    if (debug_enabled)
	        print_hex_dump(KERN_DEBUG, "MXT MSG:", DUMP_PREFIX_NONE, 16, 1, msg, sizeof(msg), false);
#endif
		object_type = reportid_to_type(data, msg[0] , &instance);

		//printk(KERN_ERR"[TSP] msg[0]=0x%x,msg[1]=0x%x,objecttype=%d\n",msg[0],msg[1],object_type);

		if (object_type == GEN_COMMANDPROCESSOR_T6) {
			if (msg[1] == 0x00) /* normal mode */
			{
				printk("[TSP] normal mode\n");
#if 0
				if (overflow_deteted) {
					overflow_deteted = 0;
					first_touch_detected = 0;
					cancel_delayed_work(
						&data->check_tchpress_dwork);
					schedule_delayed_work(
						&data->check_tchpress_dwork
						, HZ*TIME_FOR_OVERFLOWCLEAR);
				}
#endif
			}
			if ((msg[1]&0x04) == 0x04) /* I2C checksum error */
			{
				printk("[TSP] I2C checksum error\n");
			}
			if ((msg[1]&0x08) == 0x08) /* config error */
			{
				printk("[TSP] config error\n");
			}
			if ((msg[1]&0x10) == 0x10) /* calibration */
			{
				printk("[TSP] calibration is"
					" on going !!\n");
#if 0
				treat_calibration_state(data);
#endif
			}
			if ((msg[1]&0x20) == 0x20) /* signal error */
			{
				printk("[TSP] signal error\n");
			}
			if ((msg[1]&0x40) == 0x40) /* overflow */
			{
				printk("[TSP] overflow detected\n");
#if 0
				overflow_deteted = 1;
				cancel_delayed_work(&data->overflow_dwork);
				cancel_delayed_work(
					&data->check_tchpress_dwork);
				schedule_delayed_work(&data->overflow_dwork
					, HZ*TIME_FOR_RECALIBRATION);
#endif
			}
			if ((msg[1]&0x80) == 0x80) /* reset */
			{
				printk("[TSP] reset is ongoing\n");
			}
		}

		if (object_type == PROCI_TOUCHSUPPRESSION_T42) {
			get_object_info(data, GEN_ACQUISITIONCONFIG_T8,
							&size, &obj_address);
			if ((msg[1] & 0x01) == 0x00) {
				/* Palm release */
				printk("[TSP] palm touch released\n");
				touch_is_pressed = 0;

			} else if ((msg[1] & 0x01) == 0x01) {
				/* Palm Press */
				printk("[TSP] palm touch detected\n");
				touch_is_pressed = 1;
				touch_message_flag = 1;
			}
		}

		if (object_type == PROCG_NOISESUPPRESSION_T48) {
			/* printk(KERN_ERR "[TSP] T48 [STATUS]:%#02x"
				"[ADCSPERX]:%#02x[FRQ]:%#02x"
				"[STATE]:%#02x[NLEVEL]:%#02x\n"
				, msg[1], msg[2], msg[3], msg[4], msg[5]);*/

			if (msg[4] == 5) { /* Median filter error */
				printk(KERN_ERR "[TSP] Median filter error\n");
				if ((data->family_id == 0xA1)
					&& ((data->tsp_version == 0x13)
					|| (data->tsp_version == 0x20))) {
					if (data->read_ta_status)
						treat_error_status_e();
				} else {
					get_object_info(data, PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
					value = data->calcfg_batt_e;
					error = write_mem(data, obj_address+2, 1, &value);
					msleep(5);
					value |= 0x20;
					error |= write_mem(data, obj_address+2, 1, &value);
					if(error) printk(KERN_ERR "[TSP] failed to reenable CHRGON\n");
				}
			}
		}

		if (object_type == TOUCH_MULTITOUCHSCREEN_T9) {
			id = msg[0] - data->finger_type;

			if (data->family_id == 0x80) {	/*  : MXT-224 */
				if ( (data->fingers[id].state >= MXT_STATE_PRESS) && msg[1] & PRESS_MSG_MASK ) {
					printk("[TSP] calibrate on ghost touch\n");
					calibrate_chip();
				}
			}

			/* If not a touch event, then keep going */
			if (id < 0 || id >= data->num_fingers){
					continue;
				}

			if (data->finger_mask & (1U << id))
				report_input_data(data);

			if (msg[1] & RELEASE_MSG_MASK) {
				data->fingers[id].z = -1; //0;
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
#if 1
				data->x_dropbits = 2;
				data->y_dropbits = 2;
				data->fingers[id].x = (((msg[2] << 4) | (msg[4] >> 4)) >> data->x_dropbits) * 2560/1024;
				data->fingers[id].y = (((msg[3] << 4) | (msg[4] & 0xF)) >> data->y_dropbits) * 1600/1024;
				data->fingers[id].y = 1600 - data->fingers[id].y;
#else
				data->fingers[id].x = (((msg[2] << 4) | (msg[4] >> 4)) /*>> data->x_dropbits*/);
				data->fingers[id].y =  ( 4095 -((msg[3] << 4) | (msg[4] & 0xF))/* >> data->y_dropbits*/);
#endif
				data->finger_mask |= 1U << id;

				if (msg[1] & PRESS_MSG_MASK) {
					data->fingers[id].state = MXT_STATE_PRESS;
					data->fingers[id].mcount = 0;

					if (!first_touch_detected)
						first_touch_detected = 1;
				}
				else if (msg[1] & MOVE_MSG_MASK) {
					data->fingers[id].mcount += 1;
				}
				#ifdef _SUPPORT_SHAPE_TOUCH_
					data->fingers[id].component= msg[7];
				#endif

			} else if ((msg[1] & SUPPRESS_MSG_MASK) && (data->fingers[id].state != MXT_STATE_INACTIVE)) {
				data->fingers[id].z = -1;
				data->fingers[id].w = msg[5];
				data->fingers[id].state = MXT_STATE_RELEASE;
				data->finger_mask |= 1U << id;
			} else {
				dev_dbg(&data->client->dev, "Unknown state %#02x %#02x\n", msg[0], msg[1]);
				continue;
			}
		}
	} while (!gpio_get_value(data->gpio_read_done));

	if (data->finger_mask)
		report_input_data(data);

	if (data->family_id == 0x80) {	/*  : MXT-224 */
		if (touch_message_flag && (cal_check_flag)) {
			check_chip_calibration();
		}
	}
	return IRQ_HANDLED;
}

static void mxt_deepsleep(struct mxt_data *data)
{
	u8 power_cfg[3] = {0, };
	write_config(data, GEN_POWERCONFIG_T7, power_cfg);
	deepsleep = 1;
}

static void mxt_wakeup(struct mxt_data *data)
{
	write_config(data, GEN_POWERCONFIG_T7, data->power_cfg);
}

static int mxt_internal_suspend(struct mxt_data *data)
{
	int i;
	int count = 0;
#if 0
	if (data->family_id == 0xA1) {	/*  : MXT-768E */
		cancel_delayed_work(&data->config_dwork);
		cancel_delayed_work(&data->overflow_dwork);
		cancel_delayed_work(&data->check_tchpress_dwork);
		cancel_delayed_work(&data->check_median_error_dwork);
	}
	else if (data->family_id == 0x81) {	/*  : MXT-224E */
		cancel_delayed_work(&data->config_dwork);
	}
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
		s5pv310_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		touch_cpu_lock_status = 0;
	}
#endif
	/*if (!deepsleep) data->power_off();*/
	data->power_off();

	return 0;
}

static int mxt_internal_resume(struct mxt_data *data)
{
	/*if (!deepsleep) data->power_on();
	else mxt_wakeup(data);*/
	data->power_on();
	msleep(1000);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt_suspend	NULL
#define mxt_resume	NULL

u8 **tsp_config;
static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);

	printk(KERN_ERR"[TSP] %s\n", __func__);

	if (mxt_enabled == 1) {
		printk(KERN_DEBUG"[TSP] %s\n", __func__);
		mxt_enabled = 0;
		touch_is_pressed = 0;
		disable_irq(data->client->irq);
		mxt_internal_suspend(data);
	} else
		printk(KERN_ERR"[TSP] %s. but touch already off\n", __func__);
}

static void mxt_late_resume(struct early_suspend *h)
{
	bool ta_status = 0;
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);
	int i,ret;
	printk(KERN_ERR"[TSP] %s\n", __func__);

	if (mxt_enabled == 0) {
		printk(KERN_DEBUG"[TSP] %s\n", __func__);
		mxt_internal_resume(data);
		/*if (!deepsleep) enable_irq(data->client->irq);*/

		mxt_enabled = 1;
#if 0
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			printk("[TSP] ta_status is %d\n", ta_status);
			/*if (!(deepsleep && ta_status)) mxt_ta_probe(ta_status);*/
			mxt_ta_probe(ta_status);
		}
#endif
		if (data->family_id == 0xA1) {/*  : MXT-768E */
			config_dwork_flag = CAL_FROM_RESUME;
			overflow_deteted = 0;
			first_touch_detected = 0;
			treat_median_error_status = 0;
			tchcount_aft_median_error = 0;
			calibrate_chip_e();
		}
		else if (data->family_id == 0x81) {	/*  : MXT-224E */
			calibrate_chip_e();
			schedule_delayed_work(&data->config_dwork, HZ*5);
		}
		else calibrate_chip();

		for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
			ret = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
			if (ret) {
				printk("write config error");
			}

			if (tsp_config[i][0] == GEN_POWERCONFIG_T7)
				data->power_cfg = tsp_config[i] + 1;

			if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
				/* Are x and y inverted? */
				if (tsp_config[i][10] & 0x1) {
					data->x_dropbits = (!(tsp_config[i][22] & 0xC)) << 1;
					data->y_dropbits = (!(tsp_config[i][20] & 0xC)) << 1;
				} else {
					data->x_dropbits = (!(tsp_config[i][20] & 0xC)) << 1;
					data->y_dropbits = (!(tsp_config[i][22] & 0xC)) << 1;
				}
			}
		//	msleep(10);
		}
		enable_irq(data->client->irq);
		/*if(deepsleep) deepsleep = 0;*/
	} else
		printk(KERN_ERR"[TSP] %s. but touch already on\n", __func__);
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
		printk("[TSP] ta_status is %d\n", ta_status);
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
		printk(KERN_ERR"[TSP] mxt_enabled is 0\n");
		return;
	}

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
	}
	report_input_data(data);
	if (data->family_id == 0xA1) {	/*  : MXT-768E */
		calibrate_chip_e();
	}
	else if (data->family_id == 0x81) {	/*  : MXT-224E */
		calibrate_chip_e();
	}
	else calibrate_chip();
};
EXPORT_SYMBOL(Mxt_force_released);

static u8 firmware_latest[] = {0x16, 0x5}; /* mxt224 : 0x16, mxt224E : 0x10 */
static u8 build_latest[] = {0xAB, 10};

#if 0 //SYSFS
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
	printk(KERN_ERR "[TSP] object type T%d", object_type);
	printk(KERN_ERR "[TSP] object register ->Byte%d\n", object_register);
	printk(KERN_ERR "[TSP] register value %d\n", register_value);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		return count;
	}

	size = 1;
	value = (u8)register_value;
	write_mem(data, address+(u16)object_register, size, &value);
	read_mem(data, address+(u16)object_register, (u8)size, &val);

	printk(KERN_ERR "[TSP] T%d Byte%d is %d\n", object_type, object_register, val);
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
	printk("[TSP] object type T%d\n", object_type);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		printk(KERN_ERR "[TSP] fail to get object_info\n");
		return count;
	}
	for (i = 0; i < size; i++) {
		read_mem(data, address+i, 1, &val);
		printk("[TSP] Byte %u --> %u\n", i, val);
	}
	return count;
}

struct device *sec_touchscreen;
static u8 firmware_latest[] = {0x16, 0x20}; /* mxt224 : 0x16, mxt224E : 0x10 */
static u8 build_latest[] = {0xAB, 0xAA};

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

	ret = get_object_info(copy_data, GEN_COMMANDPROCESSOR_T6, &size_one, &t6_address);

	size_one = 1;
	error = write_mem(copy_data, t6_address+5, (u8)size_one, &mode);
		/* qt602240_write_object(p_qt602240_data, QT602240_GEN_COMMAND, */
		/* QT602240_COMMAND_DIAGNOSTIC, mode); */
	if (error < 0) {
		printk(KERN_ERR "[TSP] error %s: write_object\n", __func__);
	} else {
		get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size_one, &t37_address);
		size_one = 1;
		/* printk(KERN_ERR"diagnostic_chip setting success\n"); */
		read_mem(copy_data, t37_address, (u8)size_one, &value);
		/* printk(KERN_ERR"dianostic_chip mode is %d\n",value); */
	}
}

uint8_t read_uint16_t(struct mxt_data *data, uint16_t address, uint16_t *buf )
{
	uint8_t status;
	uint8_t temp[2];

	status = read_mem(data, address, 2, temp);
	*buf= ((uint16_t)temp[1]<<8)+ (uint16_t)temp[0];

	return (status);
}

void read_dbg_data(uint8_t dbg_mode , uint16_t node, uint16_t *dbg_data)
{
	u8 read_page, read_point;
	uint8_t mode,page;
	u16 size;
	u16 diagnostic_addr = 0;

	if (!mxt_enabled) {
		printk(KERN_ERR "[TSP ]read_dbg_data. mxt_enabled is 0\n");
		return;
	}

	get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &diagnostic_addr);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(10);

	do {
		if(read_mem(copy_data, diagnostic_addr, 1, &mode))
		{
			printk(KERN_INFO "[TSP] READ_MEM_FAILED \n");
			return;
		}
	} while(mode != MXT_CTE_MODE);

	diagnostic_chip(dbg_mode);
	msleep(10);

	do {
		if(read_mem(copy_data, diagnostic_addr, 1, &mode))
		{
			printk(KERN_INFO "[TSP] READ_MEM_FAILED \n");
			return;
		}
	} while(mode != dbg_mode);

    for(page = 1; page <= read_page;page++)
	{
		diagnostic_chip(MXT_PAGE_UP);
		msleep(10);
		do {
			if(read_mem(copy_data, diagnostic_addr + 1, 1, &mode))
			{
				printk(KERN_INFO "[TSP] READ_MEM_FAILED \n");
				return;
			}
		} while(mode != page);
	}

	if(read_uint16_t(copy_data, diagnostic_addr + read_point, dbg_data))
	{
		printk(KERN_INFO "[TSP] READ_MEM_FAILED \n");
		return;
	}
}

#define MIN_VALUE 19744
#define MAX_VALUE 28864

#define T48_CALCFG_CHRGON		0x20

int read_all_data(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 max_value = MIN_VALUE, min_value = MAX_VALUE;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;
	u8 val = 0;
	bool ta_status = 0;

	if (copy_data->read_ta_status) {
		copy_data->read_ta_status(&ta_status);
		printk(KERN_INFO "[TSP] ta_status is %d\n", ta_status);
	}

	/* check the CHRG_ON bit is set or not */
	/* when CHRG_ON is setted dual x is on so skip read last line*/
	get_object_info(copy_data,
		PROCG_NOISESUPPRESSION_T48, &size, &object_address);
	ret = read_mem(copy_data, object_address+2 , 1, &val);
	if (ret < 0)
		printk(KERN_ERR " TSP read fail : %s", __func__);

	printk(KERN_INFO "[TSP] %s %#02x\n", __func__, val);
	val = val & T48_CALCFG_CHRGON;

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);

	msleep(50); /* msleep(20);  */

	/* 768/64 */
	for (read_page = 0 ; read_page < 12; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data, object_address+(u16)read_point, 2, data_buffer);
			qt_refrence_node[num] = ((uint16_t)data_buffer[1]<<8)+ (uint16_t)data_buffer[0];

			/* last X line has 1/2 reference during
				TA mode So do not check min/max value */
			if ((val != T48_CALCFG_CHRGON)
				|| (val == T48_CALCFG_CHRGON && (num < 736))) {
				if ((qt_refrence_node[num] < MIN_VALUE)
				|| (qt_refrence_node[num] > MAX_VALUE)) {
					state = 1;
					printk(KERN_ERR
						"[TSP] Mxt768E qt_refrence_node[%3d] = %5d\n"
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
				printk(KERN_ERR"[TSP]qt_refrence_node[%d] = 0\n", num);
			if (num == 768)
				break;
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
		if (num == 768)
			break;
	}

	if ((max_value - min_value) > 4500) {
		printk(KERN_ERR "[TSP] diff = %d, max_value = %d, min_value = %d\n", (max_value - min_value), max_value, min_value);
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
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);
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
			read_mem(copy_data, object_address+(u16)read_point, 2, data_buffer);
				qt_delta_node[num] = ((uint16_t)data_buffer[1]<<8)+ (uint16_t)data_buffer[0];

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

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);
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
			read_mem(copy_data, object_address+(u16)read_point, 2, data_buffer);
				delta_val = ((uint16_t)data_buffer[1]<<8)+ (uint16_t)data_buffer[0];

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
	//struct mxt_data *mxt = dev_get_drvdata(dev);

	status = find_channel(MXT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}
#endif


#define READ_FW_FROM_HEADER	1
u8 firmware_mXT1536e[] = {
/*	#include "mxt1536e_v0.5a.h"*/
};

static int mxt_check_bootloader(struct i2c_client *client,
					unsigned int state)
{
	u8 val;
	u8 temp;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	if (val & 0x20)	{
		if (i2c_master_recv(client, &temp, 1) != 1) {
			dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
			return -EIO;
		}

		if (i2c_master_recv(client, &temp, 1) != 1) {
			dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
			return -EIO;
		}
		val &= ~0x20;
	}

	if ((val & 0xF0)== MXT_APP_CRC_FAIL) {
		printk("[TOUCH] MXT_APP_CRC_FAIL\n");
		if (i2c_master_recv(client, &val, 1) != 1) {
			dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
			return -EIO;
		}

		if(val & 0x20) {
			if (i2c_master_recv(client, &temp, 1) != 1) {
				dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
				return -EIO;
			}

			if (i2c_master_recv(client, &temp, 1) != 1) {
				dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
				return -EIO;
			}
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
		dev_err(&client->dev, "Unvalid bootloader mode state\n");
		printk(KERN_ERR "[TSP] Unvalid bootloader mode state\n");
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
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
				const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
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
	u16 obj_address=0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;
	u8 msg[10];

	//printk("[TSP] mxt_load_fw start!!!\n");

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;

	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT1536e;
	fw->size = sizeof(firmware_mXT1536e);
	/*pr_info("size of firmware: %d", fw->size);*/
#else
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		printk(KERN_ERR "[TSP] Unable to open firmware %s\n", fn);
		return ret;
	}
#endif
	/* Change to the bootloader mode */
	/* mxt_write_object(data, MXT_GEN_COMMAND, MXT_COMMAND_RESET, MXT_BOOT_VALUE); */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret)
	{
		printk(KERN_ERR"[TSP] fail to get object_info\n");
		release_firmware(fw);
		return ret;
	}
#if 0
	read_mem(data, 255, 8, msg);
	read_mem(data, 255, sizeof(msg), msg);
read_mem(data, 255, sizeof(msg), msg);
	read_mem(data, 255, sizeof(msg), msg);
	read_mem(data, 255, sizeof(msg), msg);
read_mem(data, 255, 8, msg);
	read_mem(data, 255, sizeof(msg), msg);
read_mem(data, 255, sizeof(msg), msg);
	read_mem(data, 255, sizeof(msg), msg);
	read_mem(data, 255, sizeof(msg), msg);
#endif
	size_one = 1;
	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
#if 0
	do {
	    printk("!");
	    msleep(10);
	}while (!gpio_get_value(data->gpio_read_done));
	
#endif	
	msleep(MXT_RESET_TIME);
#if 0
	do {
	    printk(".");
	    msleep(10);
	    	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);

	}while (gpio_get_value(data->gpio_read_done));
#endif	
	
	printk("[TSP] obj_address = %x\n",obj_address);
	/* Change to slave address of bootloader */
	if (data->family_id == 0xA1)  {	/* : MXT-768E */
		if (client->addr == MXT1536E_APP_LOW)
			client->addr = MXT1536E_BOOT_LOW;
		else
			client->addr = MXT1536E_BOOT_HIGH;
	} else {
		if (client->addr == MXT_APP_LOW)
			client->addr = MXT_BOOT_LOW;
		else
			client->addr = MXT_BOOT_HIGH;
	}
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
				printk(KERN_ERR"[TSP] firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				printk("[TSP]check_wating_frame_data_error = %d, retry\n",
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
		/* mxt_fw_write(client, fw->data + pos, frame_size); */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				printk(KERN_ERR"[TSP] firm update fail. frame_crc err\n");
				goto out;
			} else {
				printk("[TSP]check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, fw->size);
		printk("[TSP] Updated %d bytes / %zd bytes\n", pos, fw->size);

		msleep(1);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (data->family_id == 0xA1)  {	/* : MXT-1536E */
		if (client->addr == MXT1536E_BOOT_LOW)
			client->addr = MXT1536E_APP_LOW;
		else
			client->addr = MXT1536E_APP_HIGH;

	} else {
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
	}
	return ret;
}

static int mxt_load_fw_bootmode(struct device *dev, const char *fn)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	//u16 obj_address=0;
	//u16 size_one;
	//u8 value;
	//unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

	printk("[TSP] mxt_load_fw start!!!\n");
#if READ_FW_FROM_HEADER
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT1536e;
	fw->size = sizeof(firmware_mXT1536e);
	/*pr_info("size of firmware: %d", fw->size);*/
#else
//	const struct firmware *fw = NULL;
	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		printk(KERN_ERR "[TSP] Unable to open firmware %s\n", fn);
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
				printk(KERN_ERR"[TSP] firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				printk("[TSP]check_wating_frame_data_error = %d, retry\n",
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
		/* mxt_fw_write(client, fw->data + pos, frame_size); */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				printk(KERN_ERR"[TSP] firm update fail. frame_crc err\n");
				goto out;
			} else {
				printk("[TSP]check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, fw->size);
		printk("[TSP] Updated %d bytes / %zd bytes\n", pos, fw->size);

		msleep(1);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (data->family_id == 0xA1)  {	/* : MXT-768E */
		if (client->addr == MXT1536E_BOOT_LOW)
			client->addr = MXT1536E_APP_LOW;
		else
			client->addr = MXT1536E_APP_HIGH;

	} else {
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
	}
	return ret;
}

#if 0
static ssize_t set_refer0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct mxt_data *data = copy_data;
    uint16_t mxt_reference=0;
    read_dbg_data(MXT_REFERENCE_MODE, test_node[0],&mxt_reference);
    return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct mxt_data *data = copy_data;
	uint16_t mxt_reference=0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[1], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct mxt_data *data = copy_data;
	uint16_t mxt_reference=0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[2], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct mxt_data *data = copy_data;
	uint16_t mxt_reference=0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[3], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_refer4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_reference=0;
	read_dbg_data(MXT_REFERENCE_MODE, test_node[4], &mxt_reference);
	return sprintf(buf, "%u\n", mxt_reference);
}

static ssize_t set_delta0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta=0;
	read_dbg_data(MXT_DELTA_MODE, test_node[0], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if(mxt_delta) return sprintf(buf, "-%u\n", mxt_delta);
	else return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta=0;
	read_dbg_data(MXT_DELTA_MODE, test_node[1], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if(mxt_delta) return sprintf(buf, "-%u\n", mxt_delta);
	else return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta=0;
	read_dbg_data(MXT_DELTA_MODE, test_node[2], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if(mxt_delta) return sprintf(buf, "-%u\n", mxt_delta);
	else return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta=0;
	read_dbg_data(MXT_DELTA_MODE, test_node[3], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if(mxt_delta) return sprintf(buf, "-%u\n", mxt_delta);
	else return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_delta4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t mxt_delta=0;
	read_dbg_data(MXT_DELTA_MODE, test_node[4], &mxt_delta);
	if (mxt_delta < 32767)
		return sprintf(buf, "%u\n", mxt_delta);
	else
		mxt_delta = 65535 - mxt_delta;

	if(mxt_delta) return sprintf(buf, "-%u\n", mxt_delta);
	else return sprintf(buf, "%u\n", mxt_delta);
}

static ssize_t set_threshold_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", threshold);
}

static ssize_t set_all_refer_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_data(MXT_REFERENCE_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_reference;

static int atoi(char *str)
{
	int result = 0;
	int count = 0;
	if( str == NULL )
		return -1;
	while( str[count] != NULL && str[count] >= '0' && str[count] <= '9' )
	{
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}

ssize_t disp_all_refdata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n",  qt_refrence_node[index_reference]);
}

ssize_t disp_all_refdata_store(struct device *dev, struct device_attribute *attr,
								   const char *buf, size_t size)
{
	index_reference = atoi(buf);
	return size;
}

static ssize_t set_all_delta_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = read_all_delta_data(MXT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}

static int index_delta;

ssize_t disp_all_deltadata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (qt_delta_node[index_delta] < 32767)
		return sprintf(buf, "%u\n", qt_delta_node[index_delta]);
	else
		qt_delta_node[index_delta] = 65535 - qt_delta_node[index_delta];

	return sprintf(buf, "-%u\n", qt_delta_node[index_delta]);
}

ssize_t disp_all_deltadata_store(struct device *dev, struct device_attribute *attr,
								   const char *buf, size_t size)
{
	index_delta = atoi(buf);
	return size;
}

static ssize_t set_firm_version_show(struct device *dev, struct device_attribute *attr, char *buf)
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
			printk(KERN_ERR " TSP read fail : %s", __func__);
			value = 0;
			return sprintf(buf, "%d\n", value);
		} else {
			printk(KERN_DEBUG "%s : %#02x\n",
				__func__, id[2]);
			return sprintf(buf, "%#02x\n", id[2]);
		}
	} else {
		printk(KERN_ERR " TSP power off : %s", __func__);
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
}

static ssize_t set_module_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = copy_data;
	int count;

	if (mxt_enabled == 1) {
		mxt_enabled = 0;
		touch_is_pressed = 0;
#if TOUCH_BOOSTER
		tsp_press_status = 0;
#endif
		disable_irq(data->client->irq);
		mxt_internal_suspend(data);
	}

	count = sprintf(buf, "tspoff\n");

	return count;
}

static ssize_t set_module_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = copy_data;
	int count;

	bool ta_status = 0;

	if (mxt_enabled == 0) {
		mxt_internal_resume(data);
		enable_irq(data->client->irq);

		mxt_enabled = 1;

		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			printk("[TSP] ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		calibrate_chip();
	}

	count = sprintf(buf, "tspon\n");

	return count;
}

#ifdef FOR_DEBUGGING_TEST_DOWNLOADFW_BIN // add_for_bin_download
#include <linux/uaccess.h>

#define MXT768E_FW_BIN_NAME "/sdcard/mxt768e.bin"

static int mxt_download(const u8 *pBianry, const u32 unLength)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address=0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

	pr_info("[TSP] mxt_download start!!!\n");

	/* Change to the bootloader mode */
	/* mxt_write_object(data, MXT_GEN_COMMAND, MXT_COMMAND_RESET, MXT_BOOT_VALUE); */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret)
	{
		pr_err("TSP] fail to get object_info\n");
		return ret;
	}
	size_one = 1;

	pr_info("[TSP] 3\n");

	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
	msleep(MXT_RESET_TIME);

	/* Change to slave address of bootloader */
	if (data->family_id == 0xA1)  {	/* : MXT-768E */
		if (client->addr == MXT1536E_APP_LOW)
			client->addr = MXT1536E_BOOT_LOW;
		else
			client->addr = MXT1536E_BOOT_HIGH;
	} else {
		if (client->addr == MXT_APP_LOW)
			client->addr = MXT_BOOT_LOW;
		else
			client->addr = MXT_BOOT_HIGH;
	}
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
				printk(KERN_ERR"[TSP] firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				printk("[TSP]check_wating_frame_data_error = %d, retry\n",
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
		/* mxt_fw_write(client, fw->data + pos, frame_size); */
		mxt_fw_write(client, pBianry + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("[TSP] firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_info("[TSP]check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("[TSP] Updated %d bytes / %zd bytes\n", pos, unLength);

		msleep(1);
	}

out:
	/* Change to slave address of application */
	if (data->family_id == 0xA1)  {	/* : MXT-768E */
		if (client->addr == MXT1536E_BOOT_LOW)
			client->addr = MXT1536E_APP_LOW;
		else
			client->addr = MXT1536E_APP_HIGH;

	} else {
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
	}
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

	filp = filp_open(MXT768E_FW_BIN_NAME, O_RDONLY, 0);
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

	if(ret != fw1_size) {
		pr_err("Failed to read file %s (ret = %d)\n", MXT768E_FW_BIN_NAME, ret);
		kfree(fw_data1);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(oldfs);

	for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
		pr_info("[TSP] ADB - MASTER CHIP Firmware update! try : %d",retry_cnt+1);
		nRet = mxt_download( (const u8*) fw_data1, (const u32)fw1_size);
		if (nRet)
			continue;
		break;
	}

	kfree(fw_data1);
	return nRet;
}
#endif

static ssize_t set_mxt_firm_update_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error = 0;
	printk("[TSP] set_mxt_update_show start!!\n");
	if (*buf != 'S' && *buf != 'F') {
		printk(KERN_ERR"Invalid values\n");
		dev_err(dev, "Invalid values\n");
		return -EINVAL;
	}

	disable_irq(data->client->irq);
	firm_status_data = 1;
#ifdef FOR_DEBUGGING_TEST_DOWNLOADFW_BIN // add_for_bin_download
	error = mxt_binfile_download();
#else
	if (data->family_id == 0xA1)  {	/* : MXT-768E */
		if (*buf != 'F' && data->tsp_version >= firmware_latest[1] && data->tsp_build >= build_latest[1]) {
			printk(KERN_ERR"[TSP] mxt768E has latest firmware\n");
			firm_status_data =2;
			enable_irq(data->client->irq);
			return size;
		}
		printk("[TSP] mxt1536E_fm_update\n");
		error = mxt_load_fw(dev, MXT768E_FW_NAME);
	}
	else if (data->family_id == 0x80) {	/*  : MXT-224 */
		if (*buf != 'F' && data->tsp_version >= firmware_latest[0] && data->tsp_build >= build_latest[0]) {
			printk(KERN_ERR"[TSP] mxt224 has latest firmware\n");
			firm_status_data =2;
			enable_irq(data->client->irq);
			return size;
		}
		printk("[TSP] mxt224_fm_update\n");
		error = mxt_load_fw(dev, MXT224_FW_NAME);
	}
	else if (data->family_id == 0x81)  {	/* tsp_family_id - 0x81 : MXT-224E */
		if (*buf != 'F' && data->tsp_version >= firmware_latest[1] && data->tsp_build >= build_latest[1]) {
			printk(KERN_ERR"[TSP] mxt224E has latest firmware\n");
			firm_status_data =2;
			enable_irq(data->client->irq);
			return size;
		}
		printk("[TSP] mxt224E_fm_update\n");
		error = mxt_load_fw(dev, MXT224_ECHO_FW_NAME);
	}
#endif

	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		firm_status_data =3;
		printk(KERN_ERR"[TSP]The firmware update failed(%d)\n", error);
		return error;
	} else {
		dev_dbg(dev, "The firmware update succeeded\n");
		firm_status_data =2;
		printk("[TSP] The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_RESET_TIME);

		mxt_init_touch_driver(data);
		/* mxt224_initialize(data); */
	}

	enable_irq(data->client->irq);
	error = mxt_backup(data);
	if (error)
	{
		printk(KERN_ERR"[TSP]mxt_backup fail!!!\n");
		return error;
	}

	/* reset the touch IC. */
	error = mxt_reset(data);
	if (error)
	{
		printk(KERN_ERR"[TSP]mxt_reset fail!!!\n");
		return error;
	}

	msleep(MXT_RESET_TIME);
	return size;
}

static ssize_t set_mxt_firm_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int count;
	printk("Enter firmware_status_show by Factory command \n");

	if (firm_status_data == 1) {
		count = sprintf(buf, "DOWNLOADING\n");
	} else if (firm_status_data == 2) {
		count = sprintf(buf, "PASS\n");
	} else if (firm_status_data == 3) {
		count = sprintf(buf, "FAIL\n");
	} else
		count = sprintf(buf, "PASS\n");

	return count;
}

static ssize_t key_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", threshold);
}

static ssize_t key_threshold_store(struct device *dev, struct device_attribute *attr,
								   const char *buf, size_t size)
{
	/*TO DO IT*/
	unsigned int object_register=7;
	u8 value;
	u8 val;
	int ret;
	u16 address = 0;
	u16 size_one;
	int num;
	if (sscanf(buf, "%d", &num) == 1)
	{
		threshold = num;
		printk("threshold value %d\n",threshold);
		ret = get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9, &size_one, &address);
		size_one = 1;
		value = (u8)threshold;
		write_mem(copy_data, address+(u16)object_register, size_one, &value);
		read_mem(copy_data, address+(u16)object_register, (u8)size_one, &val);
		printk(KERN_ERR"T9 Byte%d is %d\n", object_register, val);
	}
	return size;
}

static ssize_t set_mxt_firm_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 fw_latest_version = 0;
	struct mxt_data *data = dev_get_drvdata(dev);
	if (data->family_id == 0xA1) fw_latest_version = firmware_latest[1];
	else if (data->family_id == 0x80) fw_latest_version = firmware_latest[0];
	else if (data->family_id == 0x81) fw_latest_version = firmware_latest[1];

	pr_info("Atmel Last firmware version is %d\n", fw_latest_version);
	return sprintf(buf, "%#02x\n", fw_latest_version);
}

static ssize_t set_mxt_firm_version_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	printk("[TSP] tsp_build = 0x%x\n", data->tsp_build);
	return sprintf(buf, "%#02x\n", data->tsp_version);
}

static ssize_t mxt_touchtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "ATMEL,MXT1536E\n");
	strcat(buf, temp);

	return strlen(buf);
}

static ssize_t x_line_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 data = 24;
	return sprintf(buf, "%d\n", data);
}

static ssize_t y_line_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 data = 32;
	return sprintf(buf, "%d\n", data);
}

#ifdef ITDEV //itdev
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
#ifdef ITDEV
				if (debug_enabled)//itdev
					print_hex_dump(KERN_DEBUG, "MXT RX:", DUMP_PREFIX_NONE, 16, 1, value, length, false);//itdev
#endif
				return 0;
			}
			else
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
	if  (i2c_transfer(adapter, msg, 2) == 2)
	{
#ifdef ITDEV
		if (debug_enabled) {//itdev
			print_hex_dump(KERN_DEBUG, "MXT TX:", DUMP_PREFIX_NONE, 16, 1, msg[0].buf, msg[0].len, false);//itdev
			print_hex_dump(KERN_DEBUG, "MXT RX:", DUMP_PREFIX_NONE, 16, 1, msg[1].buf, msg[1].len, false);//itdev
		}
#endif
		return 0;
	}
	else
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
#ifdef ITDEV
		if (debug_enabled)//itdev
			print_hex_dump(KERN_DEBUG, "MXT TX:", DUMP_PREFIX_NONE, 16, 1, &i2c_block_transfer, length+2, false);//itdev
#endif
		return length;
	}
	else
		return -EIO;
}

static ssize_t mem_access_read(struct file *filp, struct kobject *kobj,
                               struct bin_attribute *bin_attr,
                               char *buf, loff_t off, size_t count)
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
                                  struct bin_attribute *bin_attr,
                                  char *buf, loff_t off, size_t count)
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

		printk("%s\n", i ? "paused" : "unpaused");
	} else {
		printk("pause_driver write error\n");
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

		printk("%s\n", i ? "debug enabled" : "debug disabled");
	} else {
		printk("debug_enabled write error\n");
	}

	return count;
}

static ssize_t command_calibrate_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	int ret;

#if 0
	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	ret = mxt_write_byte(client,
		MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
		+ MXT_ADR_T6_CALIBRATE,
		0x1);
#else
	ret = calibrate_chip_e();
#endif

	return (ret < 0) ? ret : count;
}

static ssize_t command_reset_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	int ret;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

#if 0
	ret = reset_chip(mxt, RESET_TO_NORMAL);
#else
	ret = mxt_reset(mxt);
#endif
	return (ret < 0) ? ret : count;
}

static ssize_t command_backup_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
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
#endif //itdev

static DEVICE_ATTR(set_refer0, S_IRUGO, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO, set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO, set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO, set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO, set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO, set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO, set_all_refer_mode_show, NULL);
static DEVICE_ATTR(disp_all_refdata, S_IRUGO | S_IWUSR | S_IWGRP, disp_all_refdata_show, disp_all_refdata_store);
static DEVICE_ATTR(set_all_delta, S_IRUGO, set_all_delta_mode_show, NULL);
static DEVICE_ATTR(disp_all_deltadata, S_IRUGO | S_IWUSR | S_IWGRP, disp_all_deltadata_show, disp_all_deltadata_store);
static DEVICE_ATTR(set_firm_version, S_IRUGO | S_IWUSR | S_IWGRP, set_firm_version_show, NULL);
static DEVICE_ATTR(set_module_off, S_IRUGO | S_IWUSR | S_IWGRP, set_module_off_show, NULL);
static DEVICE_ATTR(set_module_on, S_IRUGO | S_IWUSR | S_IWGRP, set_module_on_show, NULL);
static DEVICE_ATTR(mxt_touchtype, S_IRUGO | S_IWUSR | S_IWGRP, mxt_touchtype_show, NULL);
static DEVICE_ATTR(set_threshold, S_IRUGO, set_threshold_mode_show, NULL);
static DEVICE_ATTR(tsp_firm_update, S_IWUSR | S_IWGRP, NULL, set_mxt_firm_update_store);		/* firmware update */
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO, set_mxt_firm_status_show, NULL);	/* firmware update status return */
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP, key_threshold_show, key_threshold_store);	/* touch threshold return, store */
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO, set_mxt_firm_version_show, NULL);/* PHONE*/	/* firmware version resturn in phone driver version */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO, set_mxt_firm_version_read_show, NULL);/*PART*/	/* firmware version resturn in TSP panel version */
static DEVICE_ATTR(object_show, S_IWUSR | S_IWGRP, NULL, mxt_object_show);
static DEVICE_ATTR(object_write, S_IWUSR | S_IWGRP, NULL, mxt_object_setting);
static DEVICE_ATTR(dbg_switch, S_IWUSR | S_IWGRP, NULL, mxt_debug_setting);
static DEVICE_ATTR(find_delta_channel, S_IRUGO, find_channel_show, NULL);
static DEVICE_ATTR(x_line, S_IRUGO, x_line_show, NULL);
static DEVICE_ATTR(y_line, S_IRUGO, y_line_show, NULL);
#ifdef ITDEV
/* Sysfs files for libmaxtouch interface */
static DEVICE_ATTR(pause_driver, 0666, pause_show, pause_store);//itdev
static DEVICE_ATTR(debug_enable, 0666, debug_enable_show, debug_enable_store);//itdev
static DEVICE_ATTR(command_calibrate, 0666, NULL, command_calibrate_store);//itdev
static DEVICE_ATTR(command_reset, 0666, NULL, command_reset_store);//itdev
static DEVICE_ATTR(command_backup, 0666, NULL, command_backup_store);//itdev

static struct attribute *libmaxtouch_attributes[] = {
	&dev_attr_pause_driver.attr,//itdev
	&dev_attr_debug_enable.attr,//itdev
	&dev_attr_command_calibrate.attr,//itdev
	&dev_attr_command_reset.attr,//itdev
	&dev_attr_command_backup.attr,//itdev
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
static int __devinit mxt_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;
#if 0
	bool ta_status = 0;
#endif
	//u8 **tsp_config;

	printk(KERN_ERR"[TSP] %s +++\n", __func__);

	touch_is_pressed = 0;
#if TOUCH_BOOSTER
	tsp_press_status = 0;
#endif
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
	//data->register_cb = pdata->register_cb;
	data->read_ta_status = pdata->read_ta_status;

	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "input device allocation failed\n");
		goto err_alloc_dev;
	}

	input_mt_init_slots(input_dev, data->num_fingers);

	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "egalax_i2c";

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(MT_TOOL_FINGER,input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);


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
	msleep(3000);

	copy_data = data;
#if  UPDATE
	if (client->addr == MXT1536E_APP_LOW)
		client->addr = MXT1536E_BOOT_LOW; /*MXT_BOOT_LOW*/
	else
		client->addr = MXT1536E_BOOT_HIGH;
	printk("[TSP] mxt_probe. firm update excute clientAddr=%x\n",client->addr);
	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret >= 0) {
		printk("[TSP] boot mode. firm update excute\n");
		mxt_load_fw_bootmode(NULL, MXT768E_FW_NAME);
		msleep(MXT_RESET_TIME);
	} else {
		if (client->addr == MXT1536E_BOOT_LOW)
			client->addr = MXT1536E_APP_LOW;
		else
			client->addr = MXT1536E_APP_HIGH;
	}

#endif
	ret = mxt_init_touch_driver(data);
	if (ret) {
		dev_err(&client->dev, "chip initialization failed\n");
		goto err_init_drv;

	//data->register_cb(mxt_ta_probe);


	}
	if (data->family_id == 0xA1)  {	/* tsp_family_id - 0xA1 : MXT-1536E */
		tsp_config = (u8 **)pdata->config_e;

#if UPDATE	
		if ((data->tsp_version < firmware_latest[1]
			|| (data->tsp_version == firmware_latest[1]
				&& data->tsp_build != build_latest[1]))
			&& (data->tsp_variant != 0)) {
			printk("[TSP] mxt768E force firmware update\n");
			if (mxt_load_fw(NULL, MXT768E_FW_NAME))
				goto err_config;
			else {
				msleep(MXT_RESET_TIME);
				mxt_init_touch_driver(data);
			}
		}
#endif		
	}

#if 0
	else if (data->family_id == 0xA1)  { /* tsp_family_id - 0xA1 : MXT-768E */
		tsp_config = (u8 **)pdata->config_e;
		data->t48_config_batt_e = pdata->t48_config_batt_e;
		data->t48_config_chrg_e = pdata->t48_config_chrg_e;
		data->tchthr_batt = pdata->tchthr_batt_e;
		data->tchthr_charging = pdata->tchthr_charging_e;
		data->calcfg_batt_e = pdata->calcfg_batt_e;
		data->calcfg_charging_e = pdata->calcfg_charging_e;
		data->atchcalsthr_e = pdata->atchcalsthr_e;
		data->atchfrccalthr_e = pdata->atchfrccalthr_e;
		data->atchfrccalratio_e = pdata->atchfrccalratio_e;
		data->chrgtime_batt = pdata->chrgtime_batt;
		data->chrgtime_charging = pdata->chrgtime_charging;
		data->idlesyncsperx_batt = pdata->idlesyncsperx_batt;
		data->idlesyncsperx_charging = pdata->idlesyncsperx_charging;
		data->actvsyncsperx_batt = pdata->actvsyncsperx_batt;
		data->actvsyncsperx_charging = pdata->actvsyncsperx_charging;
		data->idleacqint_batt = pdata->idleacqint_batt;
		data->idleacqint_charging = pdata->idleacqint_charging;
		data->actacqint_batt = pdata->actacqint_batt;
		data->actacqint_charging = pdata->actacqint_charging;
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
		data->tchhyst_batt = pdata->tchhyst_batt;
		data->tchhyst_charging = pdata->tchhyst_charging;

		printk("[TSP] TSP chip is MXT768-E\n");
#if !defined(FOR_DEBUGGING_TEST_DOWNLOADFW_BIN)// add_for_bin_download
		if ((data->tsp_version < firmware_latest[1]
			|| (data->tsp_version == firmware_latest[1]
				&& data->tsp_build != build_latest[1]))
			&& (data->tsp_variant != 0)) {
			printk(KERN_INFO "[TSP] mxt768E force firmware update\n");
			if (mxt_load_fw(NULL, MXT768E_FW_NAME))
				goto err_config;
			else {
				msleep(MXT_RESET_TIME);
				mxt_init_touch_driver(data);
			}
		}
#endif
		INIT_DELAYED_WORK(&data->config_dwork,
			mxt_reconfigration_normal);
		INIT_DELAYED_WORK(&data->overflow_dwork,
			mxt_calbration_by_overflowed);
		INIT_DELAYED_WORK(&data->check_tchpress_dwork,
			mxt_calibration_by_notch_after_overflowed);
		INIT_DELAYED_WORK(&data->check_median_error_dwork,
			mxt_check_medianfilter_error);
#if TOUCH_BOOSTER
		INIT_DELAYED_WORK(&data->dvfs_dwork,
			mxt_set_dvfs_off);
#endif
	}
#endif
	
	else  {
		printk(KERN_ERR"ERROR : There is no valid TSP ID\n");
		goto err_config;
	}

	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
		ret = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
		if (ret)
			goto err_config;

		if (tsp_config[i][0] == GEN_POWERCONFIG_T7)
			data->power_cfg = tsp_config[i] + 1;

		if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
			/* Are x and y inverted? */
			if (tsp_config[i][10] & 0x1) {
				data->x_dropbits = (!(tsp_config[i][22] & 0xC)) << 1;
				data->y_dropbits = (!(tsp_config[i][20] & 0xC)) << 1;
			} else {
				data->x_dropbits = (!(tsp_config[i][20] & 0xC)) << 1;
				data->y_dropbits = (!(tsp_config[i][22] & 0xC)) << 1;
			}
		}
	//	msleep(10);
	}
#if 0 //BRINGUP

	ret = mxt_backup(data);
	if (ret)
		goto err_backup;

	/* reset the touch IC. */
	ret = mxt_reset(data);
	if (ret)
		goto err_reset;

	msleep(MXT_RESET_TIME);

	if (data->family_id == 0xA1) {	/*  : MXT-768E */
		calibrate_chip_e();
	}
	else if (data->family_id == 0x81) {	/*  : MXT-224E */
		calibrate_chip_e();
	}
	else calibrate_chip();
#else
	calibrate_chip_e();
#endif

	for (i = 0; i < data->num_fingers; i++) {
		data->fingers[i].state = MXT_STATE_INACTIVE;
	}

	ret = request_threaded_irq(client->irq, NULL, mxt_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt_ts", data);

	if (ret < 0)
		goto err_irq;
	printk(KERN_ERR"[TSP] %s ---\n", __func__);

#if 0
	ret = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (ret)
		printk(KERN_ERR"[TSP] sysfs_create_group()is falled\n");

#ifdef ITDEV //itdev
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
		pr_err("Failed to create device file(%s)!\n", mem_access_attr.attr.name);
		goto err_irq;
	}
#endif

	sec_touchscreen = device_create(sec_class, NULL, 0, NULL, "sec_touchscreen");
	dev_set_drvdata(sec_touchscreen, data);
	if (IS_ERR(sec_touchscreen))
		printk(KERN_ERR "[TSP] Failed to create device(sec_touchscreen)!\n");

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_update) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n", dev_attr_tsp_firm_update.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_update_status) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n", dev_attr_tsp_firm_update_status.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_threshold) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n", dev_attr_tsp_threshold.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_version_phone) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n", dev_attr_tsp_firm_version_phone.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_tsp_firm_version_panel) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n", dev_attr_tsp_firm_version_panel.attr.name);

	if (device_create_file(sec_touchscreen, &dev_attr_mxt_touchtype) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_mxt_touchtype.attr.name);

	mxt_noise_test = device_create(sec_class, NULL, 0, NULL, "tsp_noise_test");

	if (IS_ERR(mxt_noise_test))
		printk(KERN_ERR "Failed to create device(mxt_noise_test)!\n");

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer0) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer0.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta0) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta0.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer1) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer1.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta1) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta1.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer2) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer2.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta2) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta2.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer3) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer3.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta3) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta3.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_refer4) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer4.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_delta4) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta4.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_all_refer) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_all_refer.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_disp_all_refdata)< 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_disp_all_refdata.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_all_delta) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_all_delta.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_disp_all_deltadata)< 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_disp_all_deltadata.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_threshold) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_threshold.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_firm_version) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_firm_version.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_module_off) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_module_off.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_set_module_on) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_module_on.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_x_line) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_x_line.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_y_line) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_y_line.attr.name);

	if (device_create_file(mxt_noise_test, &dev_attr_find_delta_channel) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_find_delta_channel.attr.name);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

#if BRINGUP
	if (data->family_id == 0xA1) {	/*  : MXT-768E */
		schedule_delayed_work(&data->config_dwork
			, HZ*TIME_FOR_RECONFIG_ON_BOOT);
	}

#endif
	mxt_enabled = 1;
#if BRINGUP

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk("[TSP] ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}
#endif
	printk(KERN_ERR"%s: probe success\n",__func__);

	return 0;

err_irq:
#if 0
err_reset:
err_backup:
#endif
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

MODULE_DESCRIPTION("Atmel MaXTouch 1536E driver");
MODULE_AUTHOR("ki_won.kim<ki_won.kim@samsung.com>");
MODULE_LICENSE("GPL");
