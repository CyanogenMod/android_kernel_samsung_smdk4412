/*
 *  atmel_maxtouch.c - Atmel maXTouch Touchscreen Controller
 *
 *  Version 0.2a
 *
 *  An early alpha version of the maXTouch Linux driver.
 *
 *
 *  Copyright (C) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
 *  Copyright (C) 2009 Ulf Samuelsson <ulf.samuelsson@atmel.com>
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
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

#define	DEBUG_INFO      1
#define	DEBUG_VERBOSE   2
#define	DEBUG_MESSAGES  5
#define	DEBUG_RAW       8
#define	DEBUG_TRACE     10

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/major.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
/*#include <linux/smp_lock.h>*/
#include <linux/semaphore.h>

#include <linux/delay.h>
#include <linux/atmel_mxt1386.h>
#include "atmel_mxt1386_cfg.h"

#include <linux/reboot.h>
#include <linux/input/mt.h>

#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
#include <mach/cpufreq.h>
#include <mach/dev.h>
#define SEC_DVFS_LOCK_TIMEOUT 3
#endif

/*
 * This is a driver for the Atmel maXTouch Object Protocol
 *
 * When the driver is loaded, mxt_init is called.
 * mxt_driver registers the "mxt_driver" structure in the i2c subsystem
 * The mxt_idtable.name string allows the board support to associate
 * the driver with its own data.
 *
 * The i2c subsystem will call the mxt_driver.probe == mxt_probe
 * to detect the device.
 * mxt_probe will reset the maXTouch device, and then
 * determine the capabilities of the I2C peripheral in the
 * host processor (needs to support BYTE transfers)
 *
 * If OK; mxt_probe will try to identify which maXTouch device it is
 * by calling mxt_identify.
 *
 * If a known device is found, a linux input device is initialized
 * the "mxt" device data structure is allocated
 * as well as an input device structure "mxt->input"
 * "mxt->client" is provided as a parameter to mxt_probe.
 *
 * mxt_read_object_table is called to determine which objects
 * are present in the device, and to determine their adresses
 *
 *
 * Addressing an object:
 *
 * The object is located at a 16 address in the object address space
 *
 * The object address can vary between revisions of the firmware
 *
 * The address is provided through an object descriptor table in the beginning
 * of the object address space.
 * It is assumed that an object type is only listed once in this table,
 * Each object type can have several instances, and the number of
 * instances is available in the object table
 *
 * The base address of the first instance of an object is stored in
 * "mxt->object_table[object_type].chip_addr",
 * This is indexed by the object type and allows direct access to the
 * first instance of an object.
 *
 * Each instance of an object is assigned a "Report Id" uniquely identifying
 * this instance. Information about this instance is available in the
 * "mxt->report_id" variable, which is a table indexed by the "Report Id".
 *
 * The maXTouch object protocol supports adding a checksum to messages.
 * By setting the most significant bit of the maXTouch address
 * an 8 bit checksum is added to all writes.
 *
 *
 * How to use driver.
 * -----------------
 * Example:
 * In arch/avr32/boards/atstk1000/atstk1002.c
 * an "i2c_board_info" descriptor is declared.
 * This contains info about which driver ("mXT224"),
 * which i2c address and which pin for CHG interrupts are used.
 *
 * In the "atstk1002_init" routine, "i2c_register_board_info" is invoked
 * with this information. Also, the I/O pins are configured, and the I2C
 * controller registered is on the application processor.
 *
 */


#if 1/*for debugging, enable DEBUG_TRACE*/
static int debug = DEBUG_INFO;
#else
static int debug = DEBUG_TRACE;  /* for debugging*/
#endif

/*
#define ITDEV
*/

#ifdef ITDEV
static int driver_paused;
static int debug_enabled;
#endif

#define MXT_MESSAGE_LENGTH 8
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
static void free_dvfs_lock(struct work_struct *work)
{
	struct mxt_data *mxt =
		container_of(work, struct mxt_data, dvfs_dwork.work);

	exynos4_busfreq_lock_free(DVFS_LOCK_ID_TSP);
	exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
	mxt->dvfs_lock_status = false;
}
static void set_dvfs_lock(struct mxt_data *mxt, bool on)
{
	if (0 == mxt->cpufreq_level)
		exynos_cpufreq_get_level(800000,
			&mxt->cpufreq_level);

	if (on) {
		cancel_delayed_work(&mxt->dvfs_dwork);
		if (!mxt->dvfs_lock_status) {
			exynos4_busfreq_lock(DVFS_LOCK_ID_TSP, BUS_L1);
			exynos_cpufreq_lock(DVFS_LOCK_ID_TSP,
				mxt->cpufreq_level);
			mxt->dvfs_lock_status = true;
		}
	} else {
		if (mxt->dvfs_lock_status)
			schedule_delayed_work(&mxt->dvfs_dwork,
				SEC_DVFS_LOCK_TIMEOUT * HZ);
	}
}
#endif

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");

#define I2C_RETRY_COUNT 5

/* Returns the start address of object in mXT memory. */
#define	MXT_BASE_ADDR(object_type) \
get_object_address(object_type, 0, mxt->object_table, mxt->device_info.num_objs)

/* If report_id (rid) == 0, then "mxt->report_id[rid].object" will be 0. */
#define	REPORT_ID_TO_OBJECT(rid) \
(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

#define	REPORT_ID_TO_OBJECT_NAME(rid) \
object_type_name[REPORT_ID_TO_OBJECT(rid)]

#define	T6_REG(x) (MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + (x))
#define	T37_REG(x) (MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37) +  (x))

#define REPORT_MT(x, y, amplitude, size) \
do {     \
	input_report_abs(mxt->input, ABS_MT_POSITION_X, x);             \
	input_report_abs(mxt->input, ABS_MT_POSITION_Y, y);             \
	input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, amplitude);         \
	input_report_abs(mxt->input, ABS_MT_WIDTH_MAJOR, size); \
} while (0)

const u8 *maxtouch_family = "maXTouch";
const u8 *mxt224_variant  = "mXT1386";

u8	*object_type_name[MXT_MAX_OBJECT_TYPES]	= {
/*	[0]	= "Reserved",	*/
/*	[2]	= "T2 - Obsolete",	*/
/*	[3]	= "T3 - Obsolete",	*/
	[5]	= "GEN_MESSAGEPROCESSOR_T5",
	[6]	= "GEN_COMMANDPROCESSOR_T6",
	[7]	= "GEN_POWERCONFIG_T7",
	[8]	= "GEN_ACQUIRECONFIG_T8",
	[9]	= "TOUCH_MULTITOUCHSCREEN_T9",
	[15]	= "TOUCH_KEYARRAY_T15",
	[18]	= "SPT_COMMSCONFIG_T18",
/*	[19]	= "T19 - Obsolete",*/
	[22]	= "PROCG_NOISESUPPRESSION_T22",
/*	[23]	= "T23 - Obsolete",*/
	[24]	= "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
	[25]	= "SPT_SELFTEST_T25",
/*	[26]	= "T26 - Obsolete",*/
	[27]	= "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",
	[28]	= "SPT_CTECONFIG_T28",
	[37]	= "DEBUG_DIAGNOSTICS_T37",
	[38]	= "USER_DATA_T38",
	[40]	= "PROCI_GRIPSUPPRESSION_T40",
	[41]	= "PROCI_PALMSUPPRESSION_T41",
	[43]	= "SPT_DIGITIZER_T43",
	[44]	= "SPT_MESSAGECOUNT_T44",
};

int backup_to_nv(struct mxt_data *mxt)
{
	/* backs up settings to the non-volatile memory */
	return mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) +
	       MXT_ADR_T6_BACKUPNV,
	       0x55);
}

int reset_chip(struct mxt_data *mxt, u8 mode)
{
	u8 data;
	printk(KERN_DEBUG "[TSP] Reset chip Reset mode (%d)", mode);
	if (mode == RESET_TO_NORMAL)
		data = 0x1;/* non-zero value*/
	else if (mode == RESET_TO_BOOTLOADER)
		data = 0xA5;
	else {
		pr_err("Invalid reset mode(%d)", mode);
		return -1;
	}

	/* Any non-zero value written to reset reg will reset the chip */
	return mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) +
				MXT_ADR_T6_RESET,
				data);
}

#ifdef MXT_ERROR_WORKAROUND
static void mxt_forced_release(struct mxt_data *mxt)
{
	int i;
	printk(KERN_DEBUG "[TSP] %s has been called", __func__);
	for (i = 0; i < MXT_MAX_NUM_TOUCHES; ++i) {
		input_mt_slot(mxt->input, i);
		input_mt_report_slot_state(mxt->input,
			MT_TOOL_FINGER, 0);

		mxt->mtouch_info[i].status = TSP_STATE_INACTIVE;
	}
	input_sync(mxt->input);
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	set_dvfs_lock(mxt, false);
#endif
}

static void mxt_force_reset(struct mxt_data *mxt)
{
	if (mxt->pdata->suspend_platform_hw && mxt->pdata->resume_platform_hw) {
		mxt->pdata->suspend_platform_hw();
		msleep(400);
		mxt->pdata->resume_platform_hw();
	}
	mxt_forced_release(mxt);
}
#endif

/*mode 1 = Charger connected */
/*mode 0 = Charger disconnected*/
static void mxt_inform_charger_connection(struct mxt_callbacks *cb, int mode)
{
	struct mxt_data *mxt = container_of(cb, struct mxt_data, callbacks);

	mxt->set_mode_for_ta = !!mode;
	if (mxt->enabled && !work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
}

static void mxt_ta_worker(struct work_struct *work)
{
	struct mxt_data *mxt = container_of(work, struct mxt_data, ta_work);
	static u8 blen;
	static u8 tchthr;
	static u8 noisethr;
	static u8 idlegcafdepth;
	static u8 movefilter;
	static u8 idleacqint;
	static u8 freqscale;
	static u8 jumplimit;
	int i = 0;
	u8 fhe_cnt = 0;
	u8 freq[5];
	u16 setting_bit = 0;

	printk(KERN_DEBUG "[TSP] TA/USB is%sconnected.\n",
		mxt->set_mode_for_ta ? " " : " dis");

	if (0 == mxt->pdata->fherr_cnt) {
		tchthr = mxt->pdata->touchscreen_config.tchthr;
		noisethr = mxt->pdata->noise_suppression_config.noisethr;
		idlegcafdepth = mxt->pdata->cte_config.idlegcafdepth;
		idleacqint = mxt->pdata->power_config.idleacqint;
		movefilter = mxt->pdata->touchscreen_config.movfilter;
		blen = mxt->pdata->touchscreen_config.blen;
		jumplimit = mxt->pdata->touchscreen_config.jumplimit;
		freqscale = mxt->pdata->noise_suppression_config.freqhopscale;
		if (!mxt->set_mode_for_ta)
			return ;
	}

	for (i = 0; i < 5; i++)
		freq[i] = mxt->pdata->noise_suppression_config.freq[i];

	if (mxt->set_mode_for_ta) {
		if (mxt->pdata->fherr_cnt >= mxt->pdata->fherr_chg_cnt) {
			blen = mxt->pdata->tch_blen_for_fherr;
			SET_BIT(setting_bit, TSP_SETTING_BLEN);

			tchthr = mxt->pdata->tchthr_for_fherr;
			SET_BIT(setting_bit, TSP_SETTING_TCHTHR);

			noisethr = mxt->pdata->noisethr_for_fherr;
			SET_BIT(setting_bit, TSP_SETTING_NOISETHR);

			movefilter = mxt->pdata->movefilter_for_fherr;
			SET_BIT(setting_bit, TSP_SETTING_MOVEFILTER);

			if (jumplimit != mxt->pdata->jumplimit_for_fherr) {
				jumplimit = mxt->pdata->jumplimit_for_fherr;
				SET_BIT(setting_bit, TSP_SETTING_JUMPLIMIT);
			}

			if (freqscale != mxt->pdata->freqhopscale_for_fherr) {
				freqscale = mxt->pdata->freqhopscale_for_fherr;
				SET_BIT(setting_bit, TSP_SETTING_FREQ_SCALE);
			}
			fhe_cnt = (mxt->pdata->fherr_cnt /
				mxt->pdata->fherr_chg_cnt) % 4;
			SET_BIT(setting_bit, TSP_SETTING_FREQUENCY);
			for (i = 0; i < 5; i++) {
				switch (fhe_cnt) {
				case 1:
					freq[i] =
						mxt->pdata->freq_for_fherr1[i];
					break;
				case 2:
					freq[i] =
						mxt->pdata->freq_for_fherr2[i];
					break;
				case 3:
					freq[i] =
						mxt->pdata->freq_for_fherr3[i];
					break;
				case 0:
				default:
					break;
				}
			}
		} else {
			tchthr = mxt->pdata->tchthr_for_ta_connect;
			SET_BIT(setting_bit, TSP_SETTING_TCHTHR);

			noisethr = mxt->pdata->noisethr_for_ta_connect;
			SET_BIT(setting_bit, TSP_SETTING_NOISETHR);

			if (idlegcafdepth !=
				mxt->pdata->idlegcafdepth_ta_connect) {
				idlegcafdepth =
					mxt->pdata->idlegcafdepth_ta_connect;
				SET_BIT(setting_bit, TSP_SETTING_IDLEDEPTH);
			}

			if (idleacqint !=
				mxt->pdata->idleacqint_for_ta_connect) {
				idleacqint =
					mxt->pdata->idleacqint_for_ta_connect;
				SET_BIT(setting_bit, TSP_SETTING_IDLEACQINT);
			}
		}

	} else {
		mxt->pdata->fherr_cnt = 0;

		tchthr = mxt->pdata->touchscreen_config.tchthr;
		SET_BIT(setting_bit, TSP_SETTING_TCHTHR);

		noisethr = mxt->pdata->noise_suppression_config.
			noisethr;
		SET_BIT(setting_bit, TSP_SETTING_NOISETHR);

		movefilter = mxt->pdata->touchscreen_config.movfilter;
		SET_BIT(setting_bit, TSP_SETTING_MOVEFILTER);

		blen = mxt->pdata->touchscreen_config.blen;
		SET_BIT(setting_bit, TSP_SETTING_BLEN);

		if (idlegcafdepth != mxt->pdata->cte_config.idlegcafdepth) {
			idlegcafdepth = mxt->pdata->cte_config.idlegcafdepth;
			SET_BIT(setting_bit, TSP_SETTING_IDLEDEPTH);
		}

		if (idleacqint != mxt->pdata->power_config.idleacqint) {
			idleacqint = mxt->pdata->power_config.idleacqint;
			SET_BIT(setting_bit, TSP_SETTING_IDLEACQINT);
		}

		if (jumplimit != mxt->pdata->touchscreen_config.jumplimit) {
			jumplimit = mxt->pdata->touchscreen_config.jumplimit;
			SET_BIT(setting_bit, TSP_SETTING_JUMPLIMIT);
		}

		if (freqscale != mxt->pdata->noise_suppression_config.
			freqhopscale) {
			freqscale = mxt->pdata->noise_suppression_config.
				freqhopscale;
			SET_BIT(setting_bit, TSP_SETTING_FREQ_SCALE);
		}
	}

	printk(KERN_DEBUG "[TSP] setting_bit : %x\n", setting_bit);

	for (i = 0; i < 5; i++)
		printk(KERN_DEBUG "[TSP] frequency[%d] : %u\n",
			i, freq[i]);

	disable_irq(mxt->client->irq);

	if (setting_bit & (0x1 << TSP_SETTING_BLEN))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_BLEN, blen);

	/* change to ta_connect config*/
	/* tchthr change*/
	if (setting_bit & (0x1 << TSP_SETTING_TCHTHR))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_TCHTHR, tchthr);

	/* noisethr change*/
	if (setting_bit & (0x1 << TSP_SETTING_NOISETHR))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
			+ MXT_ADR_T22_NOISETHR, noisethr);

	/* freq change*/
	if (setting_bit & (0x1 << TSP_SETTING_FREQUENCY)) {
		mxt_write_block(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
			+ MXT_ADR_T22_FREQ, 5, freq);
			printk(KERN_DEBUG "[TSP] frequency table chage : %u\n",
				mxt->pdata->fherr_cnt);
	}

	/* frequency scale */
	if (setting_bit & (0x1 << TSP_SETTING_FREQ_SCALE))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
			+ MXT_ADR_T22_FREQHOPSCALE, freqscale);

	/* idlegcafdepth change*/
	if (setting_bit & (0x1 << TSP_SETTING_IDLEDEPTH))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T28)
			+ MXT_ADR_T28_IDLEGCAFDEPTH,
			idlegcafdepth);

	/* idleacqint change*/
	if (setting_bit & (0x1 << TSP_SETTING_IDLEACQINT))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7)
			+ MXT_ADR_T7_IDLEACQINT,
			idleacqint);

	/* move filter change */
	if (setting_bit & (0x1 << TSP_SETTING_MOVEFILTER))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_MOVFILTER,
			movefilter);

	/* move filter change */
	if (setting_bit & (0x1 << TSP_SETTING_JUMPLIMIT))
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ 30,
			jumplimit);

#if 0
	/* mxt_calibrate : non-zero value*/
	mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
		+ MXT_ADR_T6_CALIBRATE,
	       0x1);
#endif
	enable_irq(mxt->client->irq);
}

static void mxt_fhe_worker(struct work_struct *work)
{
	struct mxt_data *mxt = container_of(work, struct mxt_data, fhe_work);

	printk(KERN_DEBUG "[TSP] fherr_no_ta : %u\n",
		mxt->pdata->fherr_cnt_no_ta);

	if (mxt->pdata->fherr_cnt_no_ta ==
		mxt->pdata->fherr_chg_cnt_no_ta) {

		int ret = 0;

		disable_irq(mxt->client->irq);

		/* blen */
		ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_BLEN,
			mxt->pdata->tch_blen_for_fherr_no_ta);

		printk(KERN_DEBUG "[TSP] tch_blen_for_fherr_no_ta : %u - (%d)\n",
			mxt->pdata->tch_blen_for_fherr_no_ta, ret);

		/* tchthr change*/
		ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_TCHTHR,
			mxt->pdata->tchthr_for_fherr_no_ta);

		printk(KERN_DEBUG "[TSP] tchthr_for_fherr_no_ta : %u - (%d)\n",
			mxt->pdata->tchthr_for_fherr_no_ta, ret);

		/* move filter change */
		ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_MOVFILTER,
			mxt->pdata->movfilter_fherr_no_ta);

		printk(KERN_DEBUG "[TSP] movfilter_fherr_no_ta : %u - (%d)\n",
			mxt->pdata->movfilter_fherr_no_ta, ret);

		/* noisethr change*/
		ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
			+ MXT_ADR_T22_NOISETHR,
			mxt->pdata->noisethr_for_fherr_no_ta);

		printk(KERN_DEBUG "[TSP] noisethr_for_fherr_no_ta : %u - (%d)\n",
			mxt->pdata->noisethr_for_fherr_no_ta, ret);

		enable_irq(mxt->client->irq);
	} else if (mxt->pdata->fherr_cnt_no_ta % 5)
		mxt->fherr_cnt_no_ta_calready = 1;
}

#ifdef MXT_CALIBRATE_WORKAROUND
static void mxt_calibrate_worker(struct work_struct *work)
{
	struct	mxt_data *mxt;
	u8		buf[4];
	int error;
	mxt = container_of(work, struct mxt_data, calibrate_dwork.work);

	if (mxt->enabled == true) {
		disable_irq(mxt->client->irq);
		memcpy(buf,  &mxt->pdata->atchcalst_idle, sizeof(buf));

		printk(KERN_DEBUG "[TSP] %s\n", __func__);

		/* change auto calibration config*/
		/* from atchcalst to atchcalfrcratio change*/
		error = mxt_write_block(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8)
			+ MXT_ADR_T8_ATCHCALST,
			4,
			(u8 *) buf);
		if (error < 0)
			pr_err("[TSP] error %s: write_object : from atchcalst to atchcalfrcratio\n",
					__func__);
		enable_irq(mxt->client->irq);
	}
}
#endif

/* Calculates the 24-bit CRC sum. */

static u32 mxt_CRC_24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 result;
	u16 data_word;

	data_word = (u16) ((u16) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (u32) data_word);
	if (result & 0x1000000)
		result ^= crcpoly;
	return result;
}

/* Returns object address in mXT chip, or zero if object is not found */
u16 get_object_address(uint8_t object_type,
		       uint8_t instance,
		       struct mxt_object *object_table,
		       int max_objs)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = 0;

	struct mxt_object obj;

	while ((object_table_index < max_objs) && !address_found) {
		obj = object_table[object_table_index];
		if (obj.type == object_type) {
			address_found = 1;
			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance)
				address = obj.chip_addr +
					  (obj.size + 1) * instance;
			else
				return 0;
		}
		object_table_index++;
	}

	return address;
}

/* Returns object size in mXT chip, or zero if object is not found */
u16 get_object_size(uint8_t object_type,
				struct mxt_object *object_table,
				int max_objs)
{
	uint8_t object_table_index = 0;
	struct mxt_object obj;

	while (object_table_index < max_objs) {
		obj = object_table[object_table_index];
		if (obj.type == object_type)
			return obj.size;
		object_table_index++;
	}
	return 0;
}

/*
 * Reads one byte from given address from mXT chip (which requires
 * writing the 16-bit address pointer first).
 */

int mxt_read_byte(struct i2c_client *client, u16 addr, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr = cpu_to_le16(addr);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);


	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = 1;
	msg[1].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 2) == 2) {
		mxt->last_read_addr = addr;
		return 0;
	} else {
		/*
		 * In case the transfer failed, set last read addr to invalid
		 * address, so that the next reads won't get confused.
		 */
		mxt->last_read_addr = -1;
		return -EIO;
	}
}

/*
 * Reads a block of bytes from given address from mXT chip. If we are
 * reading from message window, and previous read was from message window,
 * there's no need to write the address pointer: the mXT chip will
 * automatically set the address pointer back to message window start.
 */

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
				if (debug_enabled)
					print_hex_dump(KERN_DEBUG,
						"MXT RX:", DUMP_PREFIX_NONE,
						16, 1, value, length, false);
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
	if  (i2c_transfer(adapter, msg, 2) == 2) {
#ifdef ITDEV
		if (debug_enabled) {
			print_hex_dump(KERN_DEBUG,
				"MXT TX:", DUMP_PREFIX_NONE,
				16, 1, msg[0].buf, msg[0].len, false);
			print_hex_dump(KERN_DEBUG,
				"MXT RX:", DUMP_PREFIX_NONE,
				16, 1, msg[1].buf, msg[1].len, false);
		}
#endif
		return 0;
	} else
		return -EIO;
}

/* Reads a block of bytes from current address from mXT chip. */
static int mxt_read_block_wo_addr(struct i2c_client *client,
			   u16 length,
			   u8 *value)
{
	if  (i2c_master_recv(client, value, length) == length) {
		printk(KERN_DEBUG "read ok\n");
#ifdef ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_DEBUG,
				"MXT RX:", DUMP_PREFIX_NONE,
				16, 1, value, length, false);
#endif
		return length;
	} else {
		printk(KERN_DEBUG "read failed\n");
		return -EIO;
	}
}

/* Writes one byte to given address in mXT chip. */
int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
	struct {
		__le16 le_addr;
		u8 data;

	} i2c_byte_transfer;

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;


	if  (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3) {
#ifdef ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_DEBUG,
				"MXT TX:", DUMP_PREFIX_NONE,
				16, 1, &i2c_byte_transfer, 3, false);
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
#ifdef ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_DEBUG,
				"MXT TX:", DUMP_PREFIX_NONE,
				16, 1, &i2c_block_transfer, length+2, false);
#endif
		return length;
	} else
		return -EIO;
}

/* TODO: make all other access block until the read has been done? Otherwise
an arriving message for example could set the ap to message window, and then
the read would be done from wrong address! */

/* Writes the address pointer (to set up following reads). */
static int mxt_write_ap(struct i2c_client *client, u16 ap)
{

	__le16	le_ap = cpu_to_le16(ap);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	printk(KERN_DEBUG "Address pointer set to %d\n", ap);

	if (i2c_master_send(client, (u8 *) &le_ap, 2) == 2) {
#ifdef ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_DEBUG,
			"MXT TX:", DUMP_PREFIX_NONE, 16, 1, &le_ap, 2, false);
#endif
		return 0;
	} else
		return -EIO;
}

/* Calculates the CRC value for mXT infoblock. */
static int calculate_infoblock_crc(struct mxt_data *mxt, u32 *crc_result)
{
	u32 crc = 0;
	u16 crc_area_size;
	u8 *mem;
	int i;

	int error;
	struct i2c_client *client;

	client = mxt->client;

	crc_area_size = MXT_ID_BLOCK_SIZE +
		mxt->device_info.num_objs * MXT_OBJECT_TABLE_ELEMENT_SIZE;

	mem = kmalloc(crc_area_size, GFP_KERNEL);

	if (mem == NULL) {
		dev_err(&client->dev, "Error allocating memory\n");
		return -ENOMEM;
	}

	error = mxt_read_block(client, 0, crc_area_size, mem);
	if (error < 0) {
		kfree(mem);
		return error;
	}

	for (i = 0; i < (crc_area_size - 1); i = i + 2)
		crc = mxt_CRC_24(crc, *(mem + i), *(mem + i + 1));

	/* If uneven size, pad with zero */
	if (crc_area_size & 0x0001)
		crc = mxt_CRC_24(crc, *(mem + i), 0);

	kfree(mem);

	/* Return only 24 bits of CRC. */
	*crc_result = (crc & 0x00FFFFFF);
	return 1;

}

#ifdef ITDEV
/* Functions for mem_access interface */
struct bin_attribute mem_access_attr;
static ssize_t mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t off, size_t count)
{
	int ret = 0;
	struct i2c_client *client;

	printk(KERN_DEBUG "mem_access_read p=%p off=%lli c=%zi\n",
		buf, off, count);

	if (off >= 32768)
		return -EIO;

	if (off + count > 32768)
		count = 32768 - off;

	if (count > 256)
		count = 256;

	if (count > 0) {
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

	printk(KERN_DEBUG "mem_access_write p=%p off=%lli c=%zi\n",
		buf, off, count);

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
#endif

static void process_T9_message(struct mxt_data *mxt, u8 *message)
{
	struct	input_dev *input;
	u8  status;
	u8 report_id;
	u8 touch_id;  /* to identify each touches. starts from 0 to 15*/
	u8 pressed_or_released = 0;
	u8 anytouch_pressed = 0;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	static int prev_touch_id = -1;
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	bool tsp_boost = false;
#endif

	input = mxt->input;
	status = message[MXT_MSG_T9_STATUS];
	report_id = message[0];
	touch_id = report_id - 2;

	if (touch_id >= MXT_MAX_NUM_TOUCHES) {
		pr_err("[TSP] Invalid touch_id (toud_id=%d)", touch_id);
		return;
	}

	/* Put together the 10-/12-bit coordinate values. */
	xpos = message[MXT_MSG_T9_XPOSMSB] * 16 +
		((message[MXT_MSG_T9_XYPOSLSB] >> 4) & 0xF);
	ypos = message[MXT_MSG_T9_YPOSMSB] * 16 +
		((message[MXT_MSG_T9_XYPOSLSB] >> 0) & 0xF);

	if (mxt->pdata->max_x < 1024)
		xpos >>= 2;
	if (mxt->pdata->max_y < 1024)
		ypos >>= 2;

	mxt->mtouch_info[touch_id].size = message[MXT_MSG_T9_TCHAREA];

	if (status & MXT_MSGB_T9_DETECT) {  /* case 1: detected */
		 /* touch amplitude */
		mxt->mtouch_info[touch_id].pressure
					= message[MXT_MSG_T9_TCHAMPLITUDE];
		mxt->mtouch_info[touch_id].x = (int16_t)xpos;
		mxt->mtouch_info[touch_id].y = (int16_t)ypos;
		pressed_or_released = 1;
		if (status & MXT_MSGB_T9_PRESS) {
			mxt->mtouch_info[touch_id].status = TSP_STATE_PRESS;
			printk(KERN_DEBUG "mxt %d p\n", touch_id);
		}
	} else if (status & MXT_MSGB_T9_RELEASE) {  /* case 2: released */
		pressed_or_released = 1;
		mxt->mtouch_info[touch_id].status = TSP_STATE_RELEASE;
		mxt->mtouch_info[touch_id].pressure = 0;
		printk(KERN_DEBUG "mxt %d r\n", touch_id);
	} else if (status & MXT_MSGB_T9_SUPPRESS) {  /* case 3: suppressed */
		/*
		 * Atmel's recommendation:
		 * In the case of supression,
		 * mxt1386 chip doesn't make a release event.
		 * So we need to release them forcibly.
		 */
		mxt_forced_release(mxt);
	} else
		pr_err("[TSP] Unknown status (0x%x)", status);

	if (pressed_or_released) {
		input_mt_slot(mxt->input, touch_id);
		input_mt_report_slot_state(mxt->input,
			MT_TOOL_FINGER,
			!!mxt->mtouch_info[touch_id].pressure);

		if (TSP_STATE_RELEASE == mxt->mtouch_info[touch_id].status)
			mxt->mtouch_info[touch_id].status = TSP_STATE_INACTIVE;
		else {
			REPORT_MT(
				mxt->mtouch_info[touch_id].x,
				mxt->mtouch_info[touch_id].y,
				mxt->mtouch_info[touch_id].pressure,
				mxt->mtouch_info[touch_id].size);
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
			tsp_boost = true;
			anytouch_pressed++;
#endif
		}

		if (mxt->fherr_cnt_no_ta_calready && (!anytouch_pressed)) {
			mxt->fherr_cnt_no_ta_calready = 0;
			/* mxt_calibrate : non-zero value*/
			mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
			+ MXT_ADR_T6_CALIBRATE,
		       0x1);
		}

		input_sync(input);
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
		set_dvfs_lock(mxt, tsp_boost);
#endif
	}
	prev_touch_id = touch_id;

	if (debug >= DEBUG_TRACE) {
		char msg[64] = {0};
		char info[64] = {0};

		if (status & MXT_MSGB_T9_SUPPRESS) {
			strcpy(msg, "Suppress: ");
		} else {
			if (status & MXT_MSGB_T9_DETECT) {
				strcpy(msg, "Detect(");
				if (status & MXT_MSGB_T9_PRESS)
					strcat(msg, "P");
				if (status & MXT_MSGB_T9_MOVE)
					strcat(msg, "M");
				if (status & MXT_MSGB_T9_AMP)
					strcat(msg, "A");
				if (status & MXT_MSGB_T9_VECTOR)
					strcat(msg, "V");
				strcat(msg, "): ");
			} else if (status & MXT_MSGB_T9_RELEASE) {
				strcpy(msg, "Release: ");
			} else {
				strcpy(msg, "[!] Unknown status: ");
			}
		}
		sprintf(info, "(%d,%d) amp=%d, size=%d", xpos, ypos,
			message[MXT_MSG_T9_TCHAMPLITUDE],
			message[MXT_MSG_T9_TCHAREA]);
		strcat(msg, info);
		printk(KERN_DEBUG "%s\n", msg);
	}

	return;
}

int process_message(struct mxt_data *mxt, u8 *message, u8 object)
{

	struct i2c_client *client;

	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  event;
	u8  length;
	u8  report_id;

	client = mxt->client;
	length = mxt->message_size;
	report_id = message[0];

	switch (object) {
	case MXT_GEN_COMMANDPROCESSOR_T6:
		status = message[1];
		if (status & MXT_MSGB_T6_COMSERR)
			printk(KERN_ERR "[TSP] maXTouch checksum error\n");

		if (status & MXT_MSGB_T6_CFGERR)
			printk(KERN_ERR "[TSP] maXTouch configuration error\n");

		if (status & MXT_MSGB_T6_CAL)
			printk(KERN_DEBUG "[TSP] maXTouch calibration in progress\n");

		if (status & MXT_MSGB_T6_SIGERR) {
			printk(KERN_ERR "[TSP] maXTouch acquisition error\n");
#ifdef MXT_ERROR_WORKAROUND
			mxt_force_reset(mxt);
#endif
		}
		if (status & MXT_MSGB_T6_OFL) {
			printk(KERN_ERR "[TSP] maXTouch cycle overflow\n");
#ifdef MXT_ERROR_WORKAROUND
			/* soft reset */
			/*typical atmel spec. value is 250ms,
			but it sometimes fails to recover so it needs more*/
			reset_chip(mxt, RESET_TO_NORMAL);
			msleep(300);
#endif
		}
		if (status & MXT_MSGB_T6_RESET)
			printk(KERN_DEBUG "[TSP] maXTouch chip reset\n");

		if (status == 0) {
			printk(KERN_DEBUG "[TSP] maXTouch status normal\n");
#if defined(MXT_FACTORY_TEST)
				/*check if firmware started*/
				if (mxt->firm_status_data == 1) {
					printk(KERN_DEBUG "[TSP] maXTouch mxt->firm_normal_status_ack after firm up\n");
					/*got normal status ack*/
					mxt->firm_normal_status_ack = 1;
				}
#endif
		}
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
#ifdef ITDEV
		if (!driver_paused)
#endif
			process_T9_message(mxt, message);
		break;

	case MXT_PROCG_NOISESUPPRESSION_T22:
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Receiving noise suppression msg\n");
		status = message[MXT_MSG_T22_STATUS];
		if (status & MXT_MSGB_T22_FHCHG) {
			if (debug >= DEBUG_TRACE)
				printk(KERN_DEBUG "[TSP] maXTouch: Freq changed\n");
		}
		if (status & MXT_MSGB_T22_GCAFERR) {
			if (debug >= DEBUG_TRACE)
				printk(KERN_DEBUG "[TSP] maXTouch: High noise "
					"level\n");
		}
		if (status & MXT_MSGB_T22_FHERR) {
			mxt->pdata->fherr_cnt++;
			printk(KERN_DEBUG "[TSP] frequency hopping err : %u\n",
				mxt->pdata->fherr_cnt);

			if (mxt->set_mode_for_ta) {
				printk(KERN_DEBUG "[TSP] fherr : %u\n",
						++mxt->pdata->fherr_cnt);
				if (0 == (mxt->pdata->fherr_cnt %
					mxt->pdata->fherr_chg_cnt))
					if (!work_pending(&mxt->ta_work))
						schedule_work(&mxt->ta_work);
			} else {
				mxt->pdata->fherr_cnt_no_ta++;
				if (!work_pending(&mxt->fhe_work))
					schedule_work(&mxt->fhe_work);
			}

		}
		break;

	case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Receiving one-touch gesture msg\n");

		event = message[MXT_MSG_T24_STATUS] & 0x0F;
		xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
		xpos >>= 2;
		ypos >>= 2;

		switch (event) {
		case	MT_GESTURE_RESERVED:
			break;
		case	MT_GESTURE_PRESS:
			break;
		case	MT_GESTURE_RELEASE:
			break;
		case	MT_GESTURE_TAP:
			break;
		case	MT_GESTURE_DOUBLE_TAP:
			break;
		case	MT_GESTURE_FLICK:
			break;
		case	MT_GESTURE_DRAG:
			break;
		case	MT_GESTURE_SHORT_PRESS:
			break;
		case	MT_GESTURE_LONG_PRESS:
			break;
		case	MT_GESTURE_REPEAT_PRESS:
			break;
		case	MT_GESTURE_TAP_AND_PRESS:
			break;
		case	MT_GESTURE_THROW:
			break;
		default:
			break;
		}
		break;

	case MXT_SPT_SELFTEST_T25:
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Receiving Self-Test msg\n");

		if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) {
			if (debug >= DEBUG_TRACE)
				printk(KERN_DEBUG "[TSP] maXTouch: Self-Test OK\n");

		} else  {
			printk(KERN_DEBUG "[TSP] maXTouch: Self-Test Failed [%02x]:"
				"{%02x,%02x,%02x,%02x,%02x}\n",
				message[MXT_MSG_T25_STATUS],
				message[MXT_MSG_T25_STATUS + 0],
				message[MXT_MSG_T25_STATUS + 1],
				message[MXT_MSG_T25_STATUS + 2],
				message[MXT_MSG_T25_STATUS + 3],
				message[MXT_MSG_T25_STATUS + 4]
				);
		}
		break;

	case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Receiving 2-touch gesture message\n");
		break;

	case MXT_SPT_CTECONFIG_T28:
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Receiving CTE message...\n");
		status = message[MXT_MSG_T28_STATUS];
		if (status & MXT_MSGB_T28_CHKERR)
			printk(KERN_DEBUG "[TSP] maXTouch: Power-Up CRC failure\n");

		break;
	default:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"maXTouch: Unknown message!\n");

		break;
	}
	return 0;
}

/* Processes messages when the interrupt line (CHG) is asserted. */
static void mxt_threaded_irq_handler(struct mxt_data *mxt)
{
	struct	i2c_client *client;

	/*note: changed message_length to 8 in ver0.9*/
	u8	message[MXT_MESSAGE_LENGTH];
	u16	message_length;
	u16	message_addr;
	u8	report_id;
	u8	object;
	int	error;
	int	i;

	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;

	if (debug >= DEBUG_TRACE)
		dev_info(&mxt->client->dev, "maXTouch worker active:\n");

	/* Read next message */
	mxt->message_counter++;
	mxt->read_fail_counter = 0;
	/* Reread on failure! */
	for (i = 1; i < I2C_RETRY_COUNT; i++) {
		/*note: changed message_length to 8 in ver0.9*/
		error = mxt_read_block(client,
			message_addr,
			MXT_MESSAGE_LENGTH,
			message);
		if (error >= 0)
			break;
		mxt->read_fail_counter++;
		/* Register read failed */
		printk(KERN_DEBUG "[TSP] Failure reading maxTouch device\n");
	}

#ifdef MXT_ERROR_WORKAROUND
	/*reset mxt touch ic if the i2c error occurs continuously*/
	if (mxt->read_fail_counter == I2C_RETRY_COUNT - 1) {
		mxt_force_reset(mxt);
		mxt->read_fail_counter = 0;
		return;
	}
#endif
	report_id = message[0];
	if (debug >= DEBUG_RAW) {
		printk(KERN_DEBUG "%s message [%08x]:",
		       REPORT_ID_TO_OBJECT_NAME(report_id),
		       mxt->message_counter
		);

		for (i = 0; i < message_length; i++)
			printk(KERN_DEBUG "0x%02x ", message[i]);
		printk(KERN_DEBUG "\n");
	}

#ifdef ITDEV
	if (debug_enabled)
		print_hex_dump(KERN_DEBUG,
			"MXT MSG:", DUMP_PREFIX_NONE, 16, 1,
			message, message_length, false);
#endif

	if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {

		for (i = 0; i < message_length; i++)
			mxt->last_message[i] = message[i];
#if 0
		if (down_interruptible(&mxt->msg_sem)) {
			printk(KERN_DEBUG "mxt_worker Interrupted "
				"while waiting for msg_sem!\n");
			return;
		}
#endif
		mxt->new_msgs = 1;
#if 0
		up(&mxt->msg_sem);
#endif
		wake_up_interruptible(&mxt->msg_queue);
		/* Get type of object and process the message */
		object = mxt->rid_map[report_id].object;
		process_message(mxt, message, object);
	}
}

static irqreturn_t mxt_threaded_irq(int irq, void *_mxt)
{
	struct	mxt_data *mxt = _mxt;
	/*
	mxt->irq_counter++;
	printk(KERN_DEBUG "mxt_threaded_irq : irq_counter = %d",
		mxt->irq_counter);
	*/
	mxt_threaded_irq_handler(mxt);
	return IRQ_HANDLED;
}

/* Function to write a block of data to any address on touch chip. */

#define I2C_PAYLOAD_SIZE 254

static ssize_t set_config(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	int i;

	u16 address;
	int whole_blocks;
	int last_block_size;

	struct i2c_client *client  = to_i2c_client(dev);

	address = *((u16 *) buf);
	address = cpu_to_be16(address);
	buf += 2;

	whole_blocks = (count - 2) / I2C_PAYLOAD_SIZE;
	last_block_size = (count - 2) % I2C_PAYLOAD_SIZE;

	for (i = 0; i < whole_blocks; i++) {
		mxt_write_block(client, address, I2C_PAYLOAD_SIZE, (u8 *) buf);
		address += I2C_PAYLOAD_SIZE;
		buf += I2C_PAYLOAD_SIZE;
	}

	mxt_write_block(client, address, last_block_size, (u8 *) buf);

	return count;

}

static ssize_t get_config(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int i;
	struct i2c_client *client  = to_i2c_client(dev);
	struct mxt_data *mxt = i2c_get_clientdata(client);

	printk(KERN_DEBUG "Reading %d bytes from current ap\n",
		mxt->bytes_to_read);

	if (0 == mxt->bytes_to_read)
		return 0;

	i = mxt_read_block_wo_addr(client, mxt->bytes_to_read, (u8 *) buf);

	return (ssize_t) i;

}

/*
 * Sets up a read from mXT chip. If we want to read config data from user space
 * we need to use this first to tell the address and byte count, then use
 * get_config to read the data.
 */

static ssize_t set_ap(struct device *dev,
		      struct device_attribute *attr,
		      const char *buf,
		      size_t count)
{

	int i;
	struct i2c_client *client;
	struct mxt_data *mxt;
	u16 ap;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	if (count < 3) {
		/* Error, ap needs to be two bytes, plus 1 for size! */
		printk(KERN_DEBUG "set_ap needs to arguments: address pointer "
		       "and data size");
		return -EIO;
	}

	ap = (u16) *((u16 *)buf);
	i = mxt_write_ap(client, ap);
	mxt->bytes_to_read = (u16) *(buf + 2);
	return count;

}


static ssize_t show_deltas(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16     *delta;
	s16     size, read_size;
	u16     diagnostics;
	u16     debug_diagnostics;
	char    *bufp;
	int     x, y;
	int     error;
	u16     *val;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	/* Allocate buffer for delta's */
	size = mxt->device_info.num_nodes * sizeof(__u16);
	if (mxt->delta == NULL) {
		mxt->delta = kzalloc(size, GFP_KERNEL);
		if (!mxt->delta) {
			sprintf(buf, "insufficient memory\n");
			return strlen(buf);
		}
	}

	if (mxt->object_table[MXT_GEN_COMMANDPROCESSOR_T6].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T6 not found\n");
		return 0;
	}
	diagnostics =  T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	if (mxt->object_table[MXT_DEBUG_DIAGNOSTICS_T37].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T37 not found\n");
		return 0;
	}
	debug_diagnostics = T37_REG(2);

	/* Configure T37 to show deltas */
	error = mxt_write_byte(client, diagnostics, MXT_CMD_T6_DELTAS_MODE);
	if (error)
		return error;

	delta = mxt->delta;

	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
				       debug_diagnostics,
				       read_size,
				       (__u8 *) delta);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch: Error reading delta object\n");
		}
		delta += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (s16 *) mxt->delta;
	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ",
					(s16) le16_to_cpu(*val++));
		bufp -= 2;	/* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");
	return strlen(buf);
}


static ssize_t show_references(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16   *reference;
	s16   size, read_size;
	u16   diagnostics;
	u16   debug_diagnostics;
	char  *bufp;
	int   x, y;
	int   error;
	u16   *val;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	/* Allocate buffer for reference's */
	size = mxt->device_info.num_nodes * sizeof(u16);
	if (mxt->reference == NULL) {
		mxt->reference = kzalloc(size, GFP_KERNEL);
		if (!mxt->reference) {
			sprintf(buf, "insufficient memory\n");
			return strlen(buf);
		}
	}

	if (mxt->object_table[MXT_GEN_COMMANDPROCESSOR_T6].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T6 not found\n");
		return 0;
	}
	diagnostics =  T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	if (mxt->object_table[MXT_DEBUG_DIAGNOSTICS_T37].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T37 not found\n");
		return 0;
	}
	debug_diagnostics = T37_REG(2);

	/* Configure T37 to show references */
	mxt_write_byte(client, diagnostics, MXT_CMD_T6_REFERENCES_MODE);
	/* Should check for error */
	reference = mxt->reference;
	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
				       debug_diagnostics,
				       read_size,
				       (__u8 *) reference);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch: Error reading reference object\n");
		}
		reference += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (u16 *) mxt->reference;

	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ", le16_to_cpu(*val++));
		bufp -= 2; /* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");
	return strlen(buf);
}

static ssize_t show_device_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
	bufp += sprintf(bufp,
			"Family:\t\t\t[0x%02x] %s\n",
			mxt->device_info.family_id,
			mxt->device_info.family
			);
	bufp += sprintf(bufp,
			"Variant:\t\t[0x%02x] %s\n",
			mxt->device_info.variant_id,
			mxt->device_info.variant
			);
	bufp += sprintf(bufp,
			"Firmware version:\t[%d.%d], build 0x%02X\n",
			mxt->device_info.major,
			mxt->device_info.minor,
			mxt->device_info.build
			);
	bufp += sprintf(bufp,
			"%d Sensor nodes:\t[X=%d, Y=%d]\n",
			mxt->device_info.num_nodes,
			mxt->device_info.x_size,
			mxt->device_info.y_size
			);
	bufp += sprintf(bufp,
			"Reported resolution:\t[X=%d, Y=%d]\n",
			mxt->pdata->max_x+1,
			mxt->pdata->max_y+1
			);
	return strlen(buf);
}

static ssize_t show_stat(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
	bufp += sprintf(bufp,
			"Interrupts:\t[VALID=%d ; INVALID=%d]\n",
			mxt->valid_irq_counter,
			mxt->invalid_irq_counter
			);
	bufp += sprintf(bufp, "Messages:\t[%d]\n", mxt->message_counter);
	bufp += sprintf(bufp, "Read Failures:\t[%d]\n", mxt->read_fail_counter);
	return strlen(buf);
}

static ssize_t show_object_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client	*client;
	struct mxt_data		*mxt;
	char			*bufp;
	struct mxt_object	*object_table;
	int			i;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	bufp = buf;

	bufp += sprintf(bufp, "maXTouch: %d Objects\n",
			mxt->device_info.num_objs);

	for (i = 0; i < MXT_MAX_OBJECT_TYPES; i++) {
		if (object_table[i].type != 0) {
			bufp += sprintf(bufp,
					"Type:\t\t[%d]: %s\n",
					object_table[i].type,
					object_type_name[i]);
			bufp += sprintf(bufp,
					"Address:\t0x%04X\n",
					object_table[i].chip_addr);
			bufp += sprintf(bufp,
					"Size:\t\t%d Bytes\n",
					object_table[i].size);
			bufp += sprintf(bufp,
					"Instances:\t%d\n",
					object_table[i].instances
				);
			bufp += sprintf(bufp,
					"Report Id's:\t%d\n\n",
					object_table[i].num_report_ids);
		}
	}
	return strlen(buf);
}

static ssize_t show_messages(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	struct mxt_object *object_table;
	int   i;
	__u8  *message;
	__u16 message_len;
	__u16 message_addr;

	char  *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	bufp = buf;

	message = kmalloc(mxt->message_size, GFP_KERNEL);
	if (message == NULL) {
		printk(KERN_DEBUG "Error allocating memory!\n");
		return -ENOMEM;
	}

	message_addr = mxt->msg_proc_addr;
	message_len = mxt->message_size;
	bufp += sprintf(bufp,
			"Reading Message Window [0x%04x]\n",
			message_addr);

#if 0
	/* Acquire the lock. */
	if (down_interruptible(&mxt->msg_sem)) {
		printk(KERN_DEBUG "mxt: Interrupted while waiting for mutex!\n");
		kfree(message);
		return -ERESTARTSYS;
	}
#endif
	while (mxt->new_msgs == 0) {
		/* Release the lock. */
#if 0
		up(&mxt->msg_sem);
#endif
		if (wait_event_interruptible(mxt->msg_queue, mxt->new_msgs)) {
			printk(KERN_DEBUG "mxt: Interrupted while waiting for new msg!\n");
			kfree(message);
			return -ERESTARTSYS;
		}
#if 0
		/* Acquire the lock. */
		if (down_interruptible(&mxt->msg_sem)) {
			printk(KERN_DEBUG "mxt: Interrupted while waiting for mutex!\n");
			kfree(message);
			return -ERESTARTSYS;
		}
#endif
	}

	for (i = 0; i < mxt->message_size; i++)
		message[i] = mxt->last_message[i];

	mxt->new_msgs = 0;

#if 0
	/* Release the lock. */
	up(&mxt->msg_sem);
#endif

	for (i = 0; i < message_len; i++)
		bufp += sprintf(bufp, "0x%02x ", message[i]);
	bufp--;
	bufp += sprintf(bufp, "\t%s\n", REPORT_ID_TO_OBJECT_NAME(message[0]));

	kfree(message);
	return strlen(buf);
}


static ssize_t show_report_id(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct i2c_client    *client;
	struct mxt_data      *mxt;
	struct report_id_map *report_id;
	int                  i;
	int                  object;
	char                 *bufp;

	client    = to_i2c_client(dev);
	mxt       = i2c_get_clientdata(client);
	report_id = mxt->rid_map;

	bufp = buf;
	for (i = 0 ; i < mxt->report_id_count ; i++) {
		object = report_id[i].object;
		bufp += sprintf(bufp, "Report Id [%03d], object [%03d], "
				"instance [%03d]:\t%s\n",
				i,
				object,
				report_id[i].instance,
				object_type_name[object]);
	}
	return strlen(buf);
}

static ssize_t set_debug(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int state;

	sscanf(buf, "%d", &state);
	if (state == 0 || state == 1) {
		if (state) {
			debug = DEBUG_TRACE;
			printk(KERN_DEBUG "touch info enabled");
		} else {
			debug = DEBUG_INFO;
			printk(KERN_DEBUG "touch info disabled");
		}
	} else {
		return -EINVAL;
	}
	return count;
}

static ssize_t show_firmware_dev(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val[7];

	mxt_read_block(mxt->client, MXT_ADDR_INFO_BLOCK, 7, (u8 *)val);
	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];

	return snprintf(buf, PAGE_SIZE,
		"ATM_%d.%dx%d\n",
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build);
}

static ssize_t store_firmware(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	int state;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (sscanf(buf, "%i", &state) != 1 || (state < 0 || state > 1))
		return -EINVAL;
	/*prevents the system from entering suspend during updating*/
	wake_lock(&mxt->wakelock);
	disable_irq(mxt->client->irq);

	mxt_load_firmware(dev, MXT1386_FIRMWARE);

	enable_irq(mxt->client->irq);
	wake_unlock(&mxt->wakelock);

	return count;
}

static ssize_t show_firmware_bin(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int ver[2];
	mxt_check_firmware(dev, ver);

	return snprintf(buf, PAGE_SIZE,
		"ATM_%d.%dx%d\n", ver[0]/16, ver[0]%16, ver[1]);
}

static int chk_obj(u8 type)
{
	switch (type) {
/*	case	MXT_GEN_MESSAGEPROCESSOR_T5:*/
/*	case	MXT_GEN_COMMANDPROCESSOR_T6:*/
	case	MXT_GEN_POWERCONFIG_T7:
	case	MXT_GEN_ACQUIRECONFIG_T8:
	case	MXT_TOUCH_MULTITOUCHSCREEN_T9:
	case	MXT_TOUCH_KEYARRAY_T15:
	case	MXT_SPT_COMMSCONFIG_T18:
	case	MXT_PROCG_NOISESUPPRESSION_T22:
	case	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
	case	MXT_SPT_SELFTEST_T25:
	case	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
	case	MXT_SPT_CTECONFIG_T28:
/*	case	MXT_DEBUG_DIAGNOSTICS_T37:*/
/*	case	MXT_USER_INFO_T38:*/
/*	case	MXT_GEN_EXTENSION_T39:*/
	case	MXT_PROCI_GRIPSUPPRESSION_T40:
	case	MXT_PROCI_PALMSUPPRESSION_T41:
	case	MXT_SPT_DIGITIZER_T43:
/*	case	MXT_MESSAGECOUNT_T44:*/
		return 0;
	default:
		return -1;
	}
}

static ssize_t show_object(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
/*	struct qt602240_data *data = dev_get_drvdata(dev);*/
/*	struct qt602240_object *object;*/
	struct mxt_data *mxt;
	struct mxt_object	*object_table;

	int count = 0;
	int i, j;
	u8 val;

	mxt = dev_get_drvdata(dev);
	object_table = mxt->object_table;

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		u8 obj_type = object_table[i].type;

		if (chk_obj(obj_type))
			continue;

		count += sprintf(buf + count, "%s: %d bytes\n",
			object_type_name[obj_type], object_table[i].size);
		for (j = 0; j < object_table[i].size; j++) {
			mxt_read_byte(mxt->client,
				MXT_BASE_ADDR(obj_type)+(u16)j,
				&val);
			count += sprintf(buf + count,
				"  Byte %2d: 0x%02x (%d)\n",
				j, val, val);
		}

		count += sprintf(buf + count, "\n");
	}

#ifdef MXT_TUNNING_ENABLE
	backup_to_nv(mxt);
#endif

	return count;
}

static ssize_t store_object(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
/*	struct qt602240_data *data = dev_get_drvdata(dev);*/
/*	struct qt602240_object *object;*/
	struct mxt_data *mxt;
/*	struct mxt_object	*object_table;*/

	unsigned int type, offset, val;
	u16	chip_addr;
	int ret;

	mxt = dev_get_drvdata(dev);

	if ((sscanf(buf, "%u %u %u", &type, &offset, &val) != 3) ||
					(type >= MXT_MAX_OBJECT_TYPES)) {
		pr_err("Invalid values");
		return -EINVAL;
	}

	printk(KERN_DEBUG "Object type: %u, Offset: %u, Value: %u\n",
		type, offset, val);

	chip_addr = get_object_address(type, 0, mxt->object_table,
				mxt->device_info.num_objs);
	if (chip_addr == 0) {
		pr_err("Invalid object type(%d)!", type);
		return -EIO;
	}

	ret = mxt_write_byte(mxt->client, chip_addr+(u16)offset, (u8)val);
	if (ret < 0)
		return ret;

	return count;
}

#ifdef ITDEV
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

		printk(KERN_DEBUG "%s\n", i ? "paused" : "unpaused");
	} else
		printk(KERN_DEBUG "pause_driver write error\n");

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

		printk(KERN_DEBUG
			"%s\n", i ? "debug enabled" : "debug disabled");
	} else
		printk(KERN_DEBUG "debug_enabled write error\n");

	return count;
}

static ssize_t command_calibrate_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	int ret;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	ret = mxt_write_byte(client,
	MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
		+ MXT_ADR_T6_CALIBRATE,
		0x1);

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

	ret = reset_chip(mxt, RESET_TO_NORMAL);

	return (ret < 0) ? ret : count;
}

static ssize_t command_backup_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	int ret;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	ret = backup_to_nv(mxt);

	return (ret < 0) ? ret : count;
}

/* Sysfs files for libmaxtouch interface */
static DEVICE_ATTR(pause_driver, 0666, pause_show, pause_store);
static DEVICE_ATTR(debug_enable, 0666, debug_enable_show, debug_enable_store);
static DEVICE_ATTR(command_calibrate, 0666, NULL, command_calibrate_store);
static DEVICE_ATTR(command_reset, 0666, NULL, command_reset_store);
static DEVICE_ATTR(command_backup, 0666, NULL, command_backup_store);

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

static int enter_debug_mode(struct mxt_data *mxt, u8 cmd)
{
	int try_cnt = 0;
	u8 mode = 0;
	u16 cmd_addr = T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	u16 diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);

	/* Page Num Clear */
	mxt_write_byte(mxt->client, cmd_addr, MXT_CMD_T6_CTE_MODE);
	msleep(20);
	mxt_write_byte(mxt->client, cmd_addr, cmd);
	msleep(20);

	/* check the mode */
	do {
		mxt_read_byte(mxt->client, diagnostic_addr, &mode);
		if (cmd == mode) {
			printk(KERN_DEBUG
				"[TSP] debug mode : %s\n",
				cmd == MXT_CMD_T6_REFERENCES_MODE ?
				"reference" : "delta");
			return 0;
		}
		try_cnt++;
		msleep(20);
	} while (try_cnt < 5);

	return -EAGAIN;
}

static int read_all_refdata(struct mxt_data *mxt)
{
	int status = -EAGAIN, try_cnt = 0;
	u8 mode = 0, read_page = 0, read_point = 0,
		max_page_slave = 0, numofch = 0;
	u8 dbg_data[MXT1386_PAGE_SIZE * 2];
	u16 max_val = MXT1386_MIN_REF_VALUE,
		min_val = MXT1386_MAX_REF_VALUE, ref_val = 0;
	const u16 cmd_addr = T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	const u16 diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);

	mxt->index = 0;

	if (enter_debug_mode(mxt, MXT_CMD_T6_REFERENCES_MODE))
		return -EAGAIN;

	max_page_slave =
		(u8)((mxt->pdata->touchscreen_config.xsize * MXT1386_PAGE_WIDTH)
		/ MXT1386_PAGE_SIZE);

	do {
		if (MXT1386_MAX_PAGE == read_page) {
			if (1 != status)
				status = 0;
			break;
		}

		if ((read_page % MXT1386_PAGE_SIZE_SLAVE) < max_page_slave)
			numofch = (u8)(MXT1386_PAGE_SIZE * 2);
		else if ((read_page % MXT1386_PAGE_SIZE_SLAVE)
			== max_page_slave)
			numofch =
				(u8)(((mxt->pdata->touchscreen_config.xsize
				* MXT1386_PAGE_WIDTH)
				- (max_page_slave * MXT1386_PAGE_SIZE)) * 2);
		else {
			read_page++;
			mxt_write_byte(mxt->client,
				cmd_addr, MXT_CMD_T6_PAGE_UP);
			msleep(20);
			continue;
		}

		mxt_read_byte(mxt->client, diagnostic_addr+1, &mode);
		if (mode == read_page) {
			mxt_read_block(mxt->client,
				diagnostic_addr + 2, numofch, dbg_data);
			for (read_point = 0; read_point < numofch;
				read_point += 2) {
				ref_val = (u16)dbg_data[read_point] |
					(u16)dbg_data[read_point+1] << 8;
				if (ref_val > MXT1386_MAX_REF_VALUE) {
					max_val = ref_val;
					status = 1;
				} else if (ref_val < MXT1386_MIN_REF_VALUE) {
					min_val = ref_val;
					status = 1;
				}
				/*
				printk(KERN_DEBUG "[TSP] page : %u, node : %u,"
					"ref : %u\n",
					mode, read_point, ref_val);
				*/

				if (ref_val > max_val)
					max_val = ref_val;
				else if (ref_val < min_val)
					min_val = ref_val;

				mxt->ref_data[mxt->index++] = ref_val;
			}

			read_page++;
			mxt_write_byte(mxt->client,
				cmd_addr,
				MXT_CMD_T6_PAGE_UP);
			msleep(20);

		} else {
			try_cnt++;
			if (mode < read_page)
				mxt_write_byte(mxt->client,
					cmd_addr,
					MXT_CMD_T6_PAGE_UP);
			msleep(20);
		}
	} while (try_cnt < 10);

	printk(KERN_DEBUG "[TSP] max_val : %d,"
		" min_val : %u  status : %u\n", max_val, min_val, status);
	return status;
}

static void  read_all_deltadata(struct mxt_data *mxt)
{
	int try_cnt = 0;
	u8 mode = 0, read_page = 0, read_point = 0,
		max_page_slave = 0, numofch = 0;
	u8 dbg_data[MXT1386_PAGE_SIZE * 2];
	u16 delta_val = 0;
	const u16 cmd_addr = T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	const u16 diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);

	mxt->index = 0;

	enter_debug_mode(mxt, MXT_CMD_T6_DELTAS_MODE);

	max_page_slave =
		(mxt->pdata->touchscreen_config.xsize * MXT1386_PAGE_WIDTH)
		/ MXT1386_PAGE_SIZE;

	do {
		if (MXT1386_MAX_PAGE == read_page)
			break;

		if ((read_page % MXT1386_PAGE_SIZE_SLAVE) < max_page_slave)
			numofch = (u8)(MXT1386_PAGE_SIZE * 2);
		else if ((read_page % MXT1386_PAGE_SIZE_SLAVE)
			== max_page_slave)
			numofch =
				(u8)(((mxt->pdata->touchscreen_config.xsize
				* MXT1386_PAGE_WIDTH)
				- (max_page_slave * MXT1386_PAGE_SIZE)) * 2);
		else {
			read_page++;
			mxt_write_byte(mxt->client,
				cmd_addr, MXT_CMD_T6_PAGE_UP);
			msleep(20);
			continue;
		}

		mxt_read_byte(mxt->client, diagnostic_addr+1, &mode);
		if (mode == read_page) {
			mxt_read_block(mxt->client,
				diagnostic_addr + 2, numofch, dbg_data);
			for (read_point = 0; read_point < numofch;
				read_point += 2) {
				delta_val = (u16)dbg_data[read_point] |
					(u16)dbg_data[read_point+1] << 8;

				mxt->delta_data[mxt->index++] = delta_val;
			}

			read_page++;
			mxt_write_byte(mxt->client,
				cmd_addr,
				MXT_CMD_T6_PAGE_UP);
			msleep(20);

		} else {
			try_cnt++;
			msleep(20);
		}
	} while (try_cnt < 10);
}

static void mxt_read_2byte(u16 Address, u16 *Data, struct mxt_data *mxt)
{
	u8 temp[2];
	mxt_read_block(mxt->client, Address, 2, temp);
	*Data = ((uint16_t)temp[1]<<8) + (uint16_t)temp[0];
}

static void check_debug_data(struct mxt_data *mxt, u16 node, u8 mode)
{
	int try_cnt = 0;
	u8 read_page = 0, read_point = 0, current_page = 0, value = 0;
	u16 debug_val = 0;
	const u16 cmd_addr = T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	const u16 diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);

	enter_debug_mode(mxt, mode);

	read_page = node / 64;
	read_point = ((node % 64) * 2) + 2;

	do {
		if (current_page == read_page)
			break;

		mxt_read_byte(mxt->client, diagnostic_addr+1, &value);

		if (current_page == value)
			current_page++;
		else {
			try_cnt++;
			msleep(20);
			continue;
		}

		mxt_write_byte(mxt->client,
			cmd_addr,
			MXT_CMD_T6_PAGE_UP);
		msleep(20);

	} while (try_cnt < 10);

	mxt_read_2byte(diagnostic_addr + read_point, &debug_val, mxt);

	if (MXT_CMD_T6_REFERENCES_MODE == mode)
		mxt->ref_data[node] = debug_val;
	else if (MXT_CMD_T6_DELTAS_MODE == mode)
		mxt->delta_data[node] = debug_val;

	printk(KERN_DEBUG "[TSP] %s[%d] : %d\n",
		mode == MXT_CMD_T6_REFERENCES_MODE ? "ref" : "delta",
		node, debug_val);
}

static int check_all_refer(struct mxt_data *mxt)
{
	int ret = 0;
	int try_cnt = 0;

	do {
		if (!mxt->set_mode_for_ta)
			mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7)
				+ MXT_ADR_T7_IDLEACQINT,
				0xff);
		msleep(20);

		ret = read_all_refdata(mxt);
		printk(KERN_DEBUG "[TSP] ret : %d\n", ret);
		if (-EAGAIN != ret) {
			printk(KERN_DEBUG "[TSP] status : %d\n", ret);
			reset_chip(mxt, RESET_TO_NORMAL);
			msleep(300);
			mxt->pdata->fherr_cnt = 0;
			mxt->pdata->fherr_cnt_no_ta = 0;
			if (!work_pending(&mxt->ta_work))
				schedule_work(&mxt->ta_work);
			return ret;
		}
		printk(KERN_DEBUG "[TSP] failed to enter the debug mode : %d\n",
				++try_cnt);
	} while (try_cnt < 5);

	if (!mxt->set_mode_for_ta)
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7)
			+ MXT_ADR_T7_IDLEACQINT,
			mxt->pdata->power_config.idleacqint);

	return 1;
}

static void check_index(struct mxt_data *mxt)
{
	if (mxt->index > MXT1386_MAX_CHANNEL)
		mxt->index = 0;
}

static void get_index(struct mxt_data *mxt, const char *str)
{
	u32 tmp;
	sscanf(str, "%u", &tmp);
	mxt->index = tmp;
	printk(KERN_DEBUG "[TSP] mxt->index : %u\n", tmp);
}

static void set_fhe_chg_cnt(struct mxt_data *mxt, const char *str)
{
	u32 tmp;
	sscanf(str, "%u", &tmp);
	mxt->pdata->fherr_chg_cnt = tmp;
	printk(KERN_DEBUG "[TSP] mxt->pdata->fherr_chg_cnt : %u\n", tmp);
}

static void set_mxt_update_exe(struct work_struct *work)
{
	struct mxt_data *mxt =
		container_of(work, struct mxt_data, firmup_dwork.work);
	int ret, cnt;
	printk(KERN_DEBUG "[TSP]%s\n", __func__);

	/*wake_lock(&mxt->wakelock); */
	disable_irq(mxt->client->irq); /*disable interrupt*/
	ret = mxt_load_firmware(&mxt->client->dev, MXT1386_FIRMWARE);
	enable_irq(mxt->client->irq);  /*enable interrupt*/
	/*wake_unlock(&mxt->wakelock);*/

	if (ret >= 0) {
		for (cnt = 10; cnt > 0; cnt--) {
			if (mxt->firm_normal_status_ack == 1) {
				/* firmware update success*/
				mxt->firm_status_data = 2;
				printk(KERN_DEBUG "[TSP]Reprogram done : Firmware update Success~~~~~~~~~~\n");
				break;
			} else {
				printk(KERN_DEBUG "[TSP]Reprogram done , but not yet normal status : 3s delay needed\n");
				msleep(3000);/*3s delay*/
			}
		}
		if (cnt == 0) {
			/* firmware update Fail */
			mxt->firm_status_data = 3;
			printk(KERN_DEBUG "[TSP]Reprogram done : Firmware update Fail ~~~~~~~~~~\n");
		}
	} else {
		/* firmware update Fail*/
		mxt->firm_status_data = 3;
		printk(KERN_DEBUG "[TSP]Reprogram done : Firmware update Fail~~~~~~~~~~\n");
	}
	mxt->firm_normal_status_ack = 0;
}
static ssize_t set_mxt_update_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	printk(KERN_DEBUG "[TSP]%s\n", __func__);
	/*start firmware updating*/
	mxt->firm_status_data = 1;

	cancel_delayed_work(&mxt->firmup_dwork);
	schedule_delayed_work(&mxt->firmup_dwork, 0);
	if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "OK\n");

	return count;
}
/*Current(Panel) Version*/
static ssize_t set_mxt_firm_version_read_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{

	struct mxt_data *mxt = dev_get_drvdata(dev);
	int error, cnt;
	u8 val[7];
	u8 fw_current_version;

	for (cnt = 10; cnt > 0; cnt--) {
		error = mxt_read_block(mxt->client, MXT_ADDR_INFO_BLOCK,
								7, (u8 *)val);
		if (error < 0) {
			printk(KERN_DEBUG "Atmel touch version read fail , it will try 2s later");
			msleep(2000);
		} else {
			break;
		}
	}
	if (cnt == 0) {
		pr_err("set_mxt_firm_version_show failed!!!");
		fw_current_version = 0;
	}
	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];
	fw_current_version = val[2];
	printk(KERN_DEBUG "[TSP] Atmel %s Firmware version [%d.%d](%d) Build %d\n",
		mxt224_variant,
		mxt->device_info.major,
		mxt->device_info.minor,
		fw_current_version,
		mxt->device_info.build);
	return sprintf(buf, "%02d\n", fw_current_version);
}

/*Last(Phone) Version*/
static ssize_t set_mxt_firm_version_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	u8 fw_latest_version;
	fw_latest_version = firmware_latest[0];
	printk(KERN_DEBUG "Atmel Last firmware version is %d\n",
		fw_latest_version);
	return sprintf(buf, "%02d\n", fw_latest_version);
}
static ssize_t set_mxt_firm_status_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{

	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);
	printk(KERN_DEBUG "Enter firmware_status_show by Factory command\n");
	if (mxt->firm_status_data == 1)
		count = sprintf(buf, "Downloading\n");
	else if (mxt->firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");
	return count;

}

static ssize_t show_threshold(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	if (mxt->set_mode_for_ta)
		return sprintf(buf, "%d\n",
		mxt->pdata->tchthr_for_ta_connect);
	else
		return sprintf(buf, "%d\n",
		mxt->pdata->touchscreen_config.tchthr);
}

static ssize_t store_threshold(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int i;
	if (sscanf(buf, "%d", &i) == 1) {
		/* prevents the system from entering suspend during updating*/
		wake_lock(&mxt->wakelock);
		disable_irq(mxt->client->irq);
		mxt->pdata->touchscreen_config.tchthr = i;/*basically,48*/
		mxt_multitouch_config(mxt);
		/* backup to nv memory */
		backup_to_nv(mxt);
		/* forces a reset of the chipset */
		reset_chip(mxt, RESET_TO_NORMAL);
		msleep(250);
		enable_irq(mxt->client->irq);
		wake_unlock(&mxt->wakelock);
		printk(KERN_DEBUG "[TSP] threshold is changed to %d\n", i);
	} else {
		pr_err("[TSP] threshold write error\n");
	}
	return size;
}

static ssize_t set_suppression_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val;

	mxt_read_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
			&val);
	return sprintf(buf, "%d\n", val);
}

static ssize_t set_suppression_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int i;
	if (sscanf(buf, "%d", &i) == 1) {
		if (0x81 == i) {
			mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
				0x0);
			printk(KERN_DEBUG "[TSP] the palm suppression field is off\n");
		} else {
			mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
				mxt->pdata->palmsupression_config.ctrl);
			printk(KERN_DEBUG "[TSP] the palm suppression field is on\n");

}
	} else {
		printk(KERN_DEBUG "[TSP] sysfs write error\n");
	}
	return size;
}

#define SET_SHOW_FN(name, fn, format, ...)	\
static ssize_t show_##name(struct device *dev,	\
				     struct device_attribute *attr,	\
				     char *buf)		\
{	\
	struct mxt_data *mxt = dev_get_drvdata(dev);	\
	if (NULL == mxt) {		\
		pr_err("[TSP] drvdata is not set\n");		\
		return sprintf(buf, "\n");	\
	}		\
	fn;	\
	return sprintf(buf, format "\n", ## __VA_ARGS__);	\
}

#define SET_STORE_FN(name, fn)		\
static ssize_t store_##name(struct device *dev,	\
				     struct device_attribute *attr,	\
				     const char *buf, size_t size)	\
{	\
	struct mxt_data *mxt = dev_get_drvdata(dev);	\
	if (NULL == mxt) {		\
		pr_err("[TSP] drvdata is not set\n");		\
		return size;		\
	}		\
	fn(mxt, buf);	\
	return size;	\
}

#define ATTR_SHOW_REF(num, node)	\
SET_SHOW_FN(set_refer##num,	\
	check_debug_data(mxt, node, MXT_CMD_T6_REFERENCES_MODE),	\
	"%u", mxt->ref_data[node])

#define ATTR_SHOW_DELTA(num, node)	\
SET_SHOW_FN(set_delta##num,	\
	check_debug_data(mxt, node, MXT_CMD_T6_DELTAS_MODE),	\
	"%u", mxt->delta_data[node])

static int check_family_id(struct mxt_data *mxt)
{
	return mxt->device_info.family_id;
}

/* sec_touchscreen */
SET_SHOW_FN(mxt_touchtype, printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"ATMEL,MXT1386");
SET_SHOW_FN(fhe_chg_cnt, printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"%d", mxt->pdata->fherr_chg_cnt);

SET_STORE_FN(fhe_chg_cnt, set_fhe_chg_cnt);

static DEVICE_ATTR(mxt_touchtype, S_IRUGO,
	show_mxt_touchtype, NULL);
static DEVICE_ATTR(fhe_chg_cnt, S_IRUGO | S_IWUSR,
	show_fhe_chg_cnt, store_fhe_chg_cnt);

/* tsp_noise_test */
SET_SHOW_FN(set_module_off, mxt->pdata->suspend_platform_hw(), "tspoff");
SET_SHOW_FN(set_module_on, mxt->pdata->resume_platform_hw(), "tspon");
SET_SHOW_FN(x_line, printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"%d", mxt->pdata->touchscreen_config.xsize);
SET_SHOW_FN(y_line, printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"%d", mxt->pdata->touchscreen_config.ysize);
SET_SHOW_FN(set_all_refer, printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"%d", check_all_refer(mxt));
SET_SHOW_FN(set_all_delta, read_all_deltadata(mxt), "set_all_delta");
SET_SHOW_FN(disp_all_refdata, check_index(mxt),
	"%u", mxt->ref_data[mxt->index]);
SET_SHOW_FN(disp_all_deltadata, check_index(mxt),
	"%u", mxt->delta_data[mxt->index]);
SET_SHOW_FN(set_firm_version,
	printk(KERN_DEBUG "[TSP] %s\n", __func__),
	"%d", check_family_id(mxt));

SET_STORE_FN(disp_all_refdata, get_index);
SET_STORE_FN(disp_all_deltadata, get_index);

ATTR_SHOW_REF(0, 324);
ATTR_SHOW_REF(1, 45);
ATTR_SHOW_REF(2, 700);
ATTR_SHOW_REF(3, 1355);
ATTR_SHOW_REF(4, 1075);
ATTR_SHOW_DELTA(0, 324);
ATTR_SHOW_DELTA(1, 45);
ATTR_SHOW_DELTA(2, 700);
ATTR_SHOW_DELTA(3, 1355);
ATTR_SHOW_DELTA(4, 1075);

/* Register sysfs files */
static DEVICE_ATTR(deltas,      S_IRUGO, show_deltas,      NULL);
static DEVICE_ATTR(references,  S_IRUGO, show_references,  NULL);
static DEVICE_ATTR(device_info, S_IRUGO, show_device_info, NULL);
static DEVICE_ATTR(object_info, S_IRUGO, show_object_info, NULL);
static DEVICE_ATTR(messages,    S_IRUGO, show_messages,    NULL);
static DEVICE_ATTR(report_id,   S_IRUGO, show_report_id,   NULL);
static DEVICE_ATTR(stat,        S_IRUGO, show_stat,        NULL);
static DEVICE_ATTR(config,      S_IWUSR | S_IRUGO, get_config, set_config);
static DEVICE_ATTR(ap,          S_IWUSR, NULL,             set_ap);
static DEVICE_ATTR(debug, S_IWUSR, NULL, set_debug);
static DEVICE_ATTR(fw_dev, S_IWUSR | S_IRUGO,
	show_firmware_dev, store_firmware);
static DEVICE_ATTR(fw_bin, S_IRUGO, show_firmware_bin, NULL);
static DEVICE_ATTR(object, S_IWUSR | S_IRUGO, show_object, store_object);
static DEVICE_ATTR(set_module_off, S_IRUGO, show_set_module_off, NULL);
static DEVICE_ATTR(set_module_on, S_IRUGO, show_set_module_on, NULL);
static DEVICE_ATTR(x_line, S_IRUGO, show_x_line, NULL);
static DEVICE_ATTR(y_line, S_IRUGO, show_y_line, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO, show_set_all_refer, NULL);
static DEVICE_ATTR(set_all_delta, S_IRUGO, show_set_all_delta, NULL);
static DEVICE_ATTR(disp_all_refdata, S_IRUGO | S_IWUSR | S_IWGRP,
	show_disp_all_refdata, store_disp_all_refdata);
static DEVICE_ATTR(disp_all_deltadata, S_IRUGO | S_IWUSR | S_IWGRP,
	show_disp_all_deltadata, store_disp_all_deltadata);
static DEVICE_ATTR(set_refer0, S_IRUGO, show_set_refer0, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO, show_set_delta0, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO, show_set_refer1, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO, show_set_delta1, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO, show_set_refer2, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO, show_set_delta2, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO, show_set_refer3, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO, show_set_delta3, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO, show_set_refer4, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO, show_set_delta4, NULL);
static DEVICE_ATTR(tsp_firm_update, S_IRUGO, set_mxt_update_show, NULL);
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO,
	set_mxt_firm_status_show, NULL);
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR,
	show_threshold, store_threshold);
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO,
	set_mxt_firm_version_show, NULL);
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO,
	set_mxt_firm_version_read_show, NULL);
static DEVICE_ATTR(set_suppression, S_IRUGO | S_IWUSR,
	set_suppression_show, set_suppression_store);
static DEVICE_ATTR(set_firm_version, S_IRUGO,
	show_set_firm_version, NULL);

static struct attribute *maxTouch_attributes[] = {
	&dev_attr_deltas.attr,
	&dev_attr_references.attr,
	&dev_attr_device_info.attr,
	&dev_attr_object_info.attr,
	&dev_attr_messages.attr,
	&dev_attr_report_id.attr,
	&dev_attr_stat.attr,
	&dev_attr_config.attr,
	&dev_attr_ap.attr,
	&dev_attr_debug.attr,
	&dev_attr_fw_dev.attr,
	&dev_attr_fw_bin.attr,
	&dev_attr_object.attr,
	&dev_attr_mxt_touchtype.attr,
	&dev_attr_fhe_chg_cnt.attr,
	&dev_attr_set_module_off.attr,
	&dev_attr_set_module_on.attr,
	&dev_attr_x_line.attr,
	&dev_attr_y_line.attr,
	&dev_attr_set_all_refer.attr,
	&dev_attr_set_all_delta.attr,
	&dev_attr_disp_all_refdata.attr,
	&dev_attr_disp_all_deltadata.attr,
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_tsp_firm_update.attr,
	&dev_attr_tsp_firm_update_status.attr,
	&dev_attr_tsp_threshold.attr,
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,
	&dev_attr_set_suppression.attr,
	&dev_attr_set_firm_version.attr,
	NULL,
};

static struct attribute_group maxtouch_attr_group = {
	.attrs = maxTouch_attributes,
};

/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/

static int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt)
{
	u8 buf[7];
	int error;
	int identified;

	identified = 0;

retry_i2c:
	/* Read Device info to check if chip is valid */
	error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, 7, (u8 *) buf);

	if (error < 0) {
		mxt->read_fail_counter++;
		if (mxt->read_fail_counter < 3) {
			msleep(30);
			goto retry_i2c;
		}
		dev_err(&client->dev, "Failure accessing maXTouch device\n");
		return -EIO;
	}

	mxt->device_info.family_id  = buf[0];
	mxt->device_info.variant_id = buf[1];
	mxt->device_info.major	    = ((buf[2] >> 4) & 0x0F);
	mxt->device_info.minor      = (buf[2] & 0x0F);
	mxt->device_info.build	    = buf[3];
	mxt->device_info.x_size	    = buf[4];
	mxt->device_info.y_size	    = buf[5];
	mxt->device_info.num_objs   = buf[6];
	mxt->device_info.num_nodes  = mxt->device_info.x_size *
				      mxt->device_info.y_size;

	/* Check Family Info */
	if (mxt->device_info.family_id == MAXTOUCH_FAMILYID) {
		strlcpy(mxt->device_info.family,
			maxtouch_family, sizeof(mxt->device_info.family));
	} else {
		dev_err(&client->dev,
			"maXTouch Family ID [0x%x] not supported\n",
			mxt->device_info.family_id);
		identified = -ENXIO;
	}

	/* Check Variant Info */
	if ((mxt->device_info.variant_id == MXT224_CAL_VARIANTID) ||
	    (mxt->device_info.variant_id == MXT224_UNCAL_VARIANTID)) {
		strlcpy(mxt->device_info.variant,
			maxtouch_family, sizeof(mxt->device_info.variant));
	} else {
		dev_err(&client->dev,
			"maXTouch Variant ID [0x%x] not supported\n",
			mxt->device_info.variant_id);
		identified = -ENXIO;
	}
	printk(KERN_DEBUG "[TSP] Atmel %s.%s\n",
		mxt->device_info.family,
		mxt->device_info.variant
	);

	printk(KERN_DEBUG "[TSP] Firmware version [%d.%d] Build %d\n",
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build
	);
	printk(KERN_DEBUG "[TSP] Configuration [X: %d] x [Y: %d]\n",
		mxt->device_info.x_size,
		mxt->device_info.y_size
	);
	printk(KERN_DEBUG "[TSP] number of objects: %d\n",
		mxt->device_info.num_objs
	);

	return identified;
}

/*
 * Reads the object table from maXTouch chip to get object data like
 * address, size, report id.
 */
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt)
{
	u16	report_id_count;
	u8	buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
	u8	object_type;
	u16	object_address;
	u16	object_size;
	u8	object_instances;
	u8	object_report_ids;
	u16	object_info_address;
	u32	crc;
	u32 crc_calculated = 0;
	int	i;
	int	error;

	u8	object_instance;
	u8	object_report_id;
	u8	report_id;
	int     first_report_id;

	struct mxt_object *object_table;

	if (debug >= DEBUG_TRACE)
		printk(KERN_DEBUG "[TSP] maXTouch driver get configuration\n");

	object_table = kzalloc(sizeof(struct mxt_object) *
			       mxt->device_info.num_objs,
			       GFP_KERNEL);
	if (object_table == NULL) {
		pr_err("maXTouch: Memory allocation failed!\n");
		return -ENOMEM;
	}

	mxt->object_table = object_table;

	if (debug >= DEBUG_TRACE)
		printk(KERN_DEBUG "[TSP] maXTouch driver Memory allocated\n");

	object_info_address = MXT_ADDR_OBJECT_TABLE;

	report_id_count = 0;
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Reading maXTouch at [0x%04x]: ",
			       object_info_address);
retry_i2c:
		error = mxt_read_block(client, object_info_address,
				MXT_OBJECT_TABLE_ELEMENT_SIZE, (u8 *)buf);
		if (error < 0) {
			mxt->read_fail_counter++;
			if (mxt->read_fail_counter < 3)
				goto retry_i2c;
			dev_err(&client->dev,
				"maXTouch Object %d could not be read\n", i);
			return -EIO;
		}
		object_type		=  buf[0];
		object_address		= (buf[2] << 8) + buf[1];
		object_size		=  buf[3] + 1;
		object_instances	=  buf[4] + 1;
		object_report_ids	=  buf[5];
		if (debug >= DEBUG_TRACE)
			printk(KERN_DEBUG "[TSP] Type=%03d, Address=0x%04x, "
			       "Size=0x%02x, %d instances, %d report id's\n",
			       object_type,
			       object_address,
			       object_size,
			       object_instances,
			       object_report_ids
		);

		if (object_type > MXT_MAX_OBJECT_TYPES) {
			/* Unknown object type */
			dev_err(&client->dev,
				"maXTouch object type [%d] not recognized\n",
				object_type);
			return -ENXIO;

		}

		/* Save frequently needed info. */
		if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) {
			mxt->msg_proc_addr = object_address;
			/*mxt->message_size = object_size;*/
			/*note: changed message_length to 8 in ver0.9*/
			mxt->message_size = MXT_MESSAGE_LENGTH;
		}

		object_table[i].type            = object_type;
		object_table[i].chip_addr       = object_address;
		object_table[i].size            = object_size;
		object_table[i].instances       = object_instances;
		object_table[i].num_report_ids  = object_report_ids;
		report_id_count += object_instances * object_report_ids;

		object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;
	}

	mxt->rid_map =
		kzalloc(sizeof(struct report_id_map) * (report_id_count + 1),
			/* allocate for report_id 0, even if not used */
			GFP_KERNEL);
	if (mxt->rid_map == NULL) {
		pr_err("maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}

	mxt->last_message = kzalloc(mxt->message_size, GFP_KERNEL);
	if (mxt->last_message == NULL) {
		pr_err("maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}


	mxt->report_id_count = report_id_count;
	if (report_id_count > 254) {	/* 0 & 255 are reserved */
			dev_err(&client->dev,
				"Too many maXTouch report id's [%d]\n",
				report_id_count);
			return -ENXIO;
	}

	/* Create a mapping from report id to object type */
	report_id = 1; /* Start from 1, 0 is reserved. */

	/* Create table associating report id's with objects & instances */
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		for (object_instance = 0;
		     object_instance < object_table[i].instances;
		     object_instance++){
			first_report_id = report_id;
			for (object_report_id = 0;
			     object_report_id < object_table[i].num_report_ids;
			     object_report_id++) {
				mxt->rid_map[report_id].object =
					object_table[i].type;
				mxt->rid_map[report_id].instance =
					object_instance;
				mxt->rid_map[report_id].first_rid =
					first_report_id;
				report_id++;
			}
		}
	}

	/* Read 3 byte CRC */
	error = mxt_read_block(client, object_info_address, 3, buf);
	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "Error reading CRC\n");
	}

	crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

	if (calculate_infoblock_crc(mxt, &crc_calculated) < 0)
		printk(KERN_DEBUG "[TSP] Error while calculating CRC!\n");

	if (debug >= DEBUG_TRACE) {
		printk(KERN_DEBUG "[TSP] Reported info block CRC = 0x%6X\n\n",
			crc);
		printk(KERN_DEBUG "[TSP] Calculated info block CRC = 0x%6X\n\n",
		       crc_calculated);
	}

	if (crc == crc_calculated) {
		mxt->info_block_crc = crc;
	} else {
		mxt->info_block_crc = 0;
		pr_err("maXTouch: info block CRC invalid!\n");
	}

	mxt->delta	= NULL;
	mxt->reference	= NULL;
	mxt->cte	= NULL;

	if (debug >= DEBUG_VERBOSE) {

		dev_info(&client->dev, "maXTouch: %d Objects\n",
				mxt->device_info.num_objs);

		for (i = 0; i < mxt->device_info.num_objs; i++) {
			dev_info(&client->dev, "Type:\t\t\t[%d]: %s\n",
				 object_table[i].type,
				 object_type_name[object_table[i].type]);
			dev_info(&client->dev, "\tAddress:\t0x%04X\n",
				 object_table[i].chip_addr);
			dev_info(&client->dev, "\tSize:\t\t%d Bytes\n",
				 object_table[i].size);
			dev_info(&client->dev, "\tInstances:\t%d\n",
				 object_table[i].instances);
			dev_info(&client->dev, "\tReport Id's:\t%d\n",
				 object_table[i].num_report_ids);
		}
	}
	return 0;
}

u8 mxt_valid_interrupt(void)
{
	return 1;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h)
{
#ifndef MXT_SLEEP_POWEROFF
	u8 cmd_sleep[2] = {0};
	u16 addr;
#endif
	struct mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

	printk(KERN_DEBUG "[TSP] %s has been called!\n", __func__);
#if defined(MXT_FACTORY_TEST)
	/*start firmware updating : not yet finished*/
	while (mxt->firm_status_data == 1) {
		printk(KERN_DEBUG "[TSP] mxt firmware is Downloading : mxt suspend must be delayed!");
		msleep(1000);
	}
#endif
	disable_irq(mxt->client->irq);
	mxt->enabled = false;

	/* cancel and wait for all works to stop so they don't try to
	 * communicate with the controller after we turn it off
	 */
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->suspend_platform_hw != NULL)
		mxt->pdata->suspend_platform_hw();
#else
	/*
	  * a setting of zeros to IDLEACQINT and ACTVACQINT
	  * forces the chip set to enter Deep Sleep mode.
	  */
	addr = get_object_address(MXT_GEN_POWERCONFIG_T7, 0,
			mxt->object_table, mxt->device_info.num_objs);
	printk(KERN_DEBUG "[TSP] addr: 0x%02x, buf[0]=0x%x, buf[1]=0x%x",
				addr, cmd_sleep[0], cmd_sleep[1]);
	mxt_write_block(mxt->client, addr, 2, (u8 *)cmd_sleep);
#endif
	mxt_forced_release(mxt);
}

static void mxt_late_resume(struct early_suspend *h)
{
#ifndef MXT_SLEEP_POWEROFF
	int cnt;
#endif
	struct	mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

	printk(KERN_DEBUG "[TSP] %s has been called!\n", __func__);
#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->resume_platform_hw != NULL)
		mxt->pdata->resume_platform_hw();
#else
	for (cnt = 10; cnt > 0; cnt--) {
		if (mxt_power_config(mxt) < 0)
			continue;
		if (reset_chip(mxt, RESET_TO_NORMAL) == 0)/* soft reset*/
			break;
	}
	if (cnt == 0)
		pr_err("%s : reset_chip failed!!!\n", __func__);
	/*typical atmel spec. value is 250ms,
	but it sometimes fails to recover so it needs more*/
	msleep(300);
#endif
	mxt->pdata->fherr_cnt = 0;
	mxt->pdata->fherr_cnt_no_ta = 0;
	if (!work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
	mxt->enabled = true;
	enable_irq(mxt->client->irq);
#ifdef MXT_CALIBRATE_WORKAROUND
	schedule_delayed_work(&mxt->calibrate_dwork, msecs_to_jiffies(4000));
#endif
}
#endif


static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct mxt_data          *mxt;
	struct mxt_platform_data *pdata;
	struct input_dev         *input;
	struct device *tsp_dev;
	int error;
	int i;

	error = 0xff;
	if (debug >= DEBUG_INFO) {
		printk(KERN_DEBUG "[TSP] maXTouch driver\n");
		printk(KERN_DEBUG "[TSP] \t \"%s\"\n",
			client->name);
		printk(KERN_DEBUG "[TSP] \taddr:\t0x%04x\n",
			client->addr);
		printk(KERN_DEBUG "[TSP] \tirq:\t%d\n",
			client->irq);
		printk(KERN_DEBUG "[TSP] \tflags:\t0x%04x\n",
			client->flags);
		printk(KERN_DEBUG "[TSP] \tadapter:\"%s\"\n",
			client->adapter->name);
		printk(KERN_DEBUG "[TSP] \tdevice:\t\"%s\"\n",
			client->dev.init_name);
	}

	/* Allocate structure - we need it to identify device */
	mxt = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (!mxt) {
		dev_err(&client->dev, "insufficient memory\n");
		error = -ENOMEM;
		goto err_mxt_alloc;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "error allocating input device\n");
		error = -ENOMEM;
		goto err_input_dev_alloc;
	}

	/* Initialize Platform data */
	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&client->dev, "platform data is required!\n");
		goto err_pdata;
	}

	mxt->pdata = pdata;
	mxt->client = client;
	mxt->input  = input;
#if defined(MXT_FACTORY_TEST)
	mxt->firm_status_data = 0;
	mxt->firm_normal_status_ack = 0;
#endif
	mxt->read_fail_counter = 0;
	mxt->message_counter   = 0;
	mxt->bytes_to_read = 0;
	msleep(200);

	if (mxt_identify(client, mxt)) {
		dev_err(&client->dev, "Chip could not be identified\n");
		goto err_identify;
	}

	if (mxt_read_object_table(client, mxt)) {
		dev_err(&client->dev, "failed to read object table\n");
		goto err_read_ot;
	}

	INIT_WORK(&mxt->ta_work, mxt_ta_worker);
	INIT_WORK(&mxt->fhe_work, mxt_fhe_worker);
#ifdef MXT_FACTORY_TEST
	INIT_DELAYED_WORK(&mxt->firmup_dwork, set_mxt_update_exe);
#endif
#ifdef MXT_CALIBRATE_WORKAROUND
	INIT_DELAYED_WORK(&mxt->calibrate_dwork, mxt_calibrate_worker);
#endif
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	INIT_DELAYED_WORK(&mxt->dvfs_dwork,
		free_dvfs_lock);
	mxt->dvfs_lock_status = false;
#endif

	/* Register callbacks */
	/* To inform tsp , charger connection status*/
	mxt->callbacks.inform_charger = mxt_inform_charger_connection;
	if (mxt->pdata->register_cb)
		mxt->pdata->register_cb(&mxt->callbacks);

	init_waitqueue_head(&mxt->msg_queue);
	mutex_init(&mxt->mutex);
	spin_lock_init(&mxt->lock);
	wake_lock_init(&mxt->wakelock, WAKE_LOCK_SUSPEND, "touch");

	snprintf(
		mxt->phys_name,
		sizeof(mxt->phys_name),
		"%s/input0",
		dev_name(&client->dev)
	);
	input->name = client->driver->driver.name;
	input->phys = mxt->phys_name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	if (debug >= DEBUG_INFO) {
		printk(KERN_DEBUG "[TSP] maXTouch name: \"%s\"\n", input->name);
		printk(KERN_DEBUG "[TSP] maXTouch phys: \"%s\"\n", input->phys);
		printk(KERN_DEBUG "[TSP] maXTouch driver setting abs parameters\n");
	}

	__set_bit(EV_ABS, input->evbit);
	__set_bit(EV_KEY, input->evbit);
	__set_bit(MT_TOOL_FINGER, input->keybit);
	__set_bit(INPUT_PROP_DIRECT, input->propbit);

	input_mt_init_slots(input, MXT_MAX_NUM_TOUCHES);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0,
		mxt->pdata->max_x - 1, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0,
		mxt->pdata->max_y - 1, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);

	i2c_set_clientdata(client, mxt);
	input_set_drvdata(input, mxt);
	error = input_register_device(mxt->input);
	if (error < 0) {
		dev_err(&client->dev,
			"Failed to register input device\n");
		goto err_register_device;
	}

#ifndef MXT_TUNNING_ENABLE
	/* pre-set configuration before soft reset */
	error = mxt_config_settings(mxt);
	if (error < 0)
		goto err_after_read_ot;
#endif
	for (i = 0; i < MXT_MAX_NUM_TOUCHES ; i++)
		mxt->mtouch_info[i].status = TSP_STATE_INACTIVE;

	/* Allocate the interrupt */
	mxt->irq = client->irq;
	mxt->valid_irq_counter = 0;
	mxt->invalid_irq_counter = 0;
	mxt->irq_counter = 0;
	error = request_threaded_irq(mxt->irq,
		NULL,
		mxt_threaded_irq,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT,
		client->dev.driver->name,
		mxt);
	if (error < 0) {
		dev_err(&client->dev,
			"failed to allocate irq %d\n", mxt->irq);
		goto err_irq;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	mxt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	mxt->early_suspend.suspend = mxt_early_suspend;
	mxt->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	tsp_dev  = device_create(sec_class, NULL, 0, mxt, "sec_touchscreen");
	if (IS_ERR(tsp_dev)) {
		pr_err("Failed to create device for the sysfs\n");
		error = -ENODEV;
		goto err_sysfs_create_group;
	}

	error = sysfs_create_group(&tsp_dev->kobj, &maxtouch_attr_group);
	if (error) {
		pr_err("Failed to create sysfs group\n");
		goto err_sysfs_create_group;
	}

#ifdef ITDEV
	error = sysfs_create_group(&client->dev.kobj, &libmaxtouch_attr_group);
	if (error) {
		pr_err("Failed to create libmaxtouch sysfs group\n");
		goto err_sysfs_create_group;
	}

	sysfs_bin_attr_init(&mem_access_attr);
	mem_access_attr.attr.name = "mem_access";
	mem_access_attr.attr.mode = S_IRUGO | S_IWUGO;
	mem_access_attr.read = mem_access_read;
	mem_access_attr.write = mem_access_write;
	mem_access_attr.size = 65535;

	if (sysfs_create_bin_file(&client->dev.kobj,
		&mem_access_attr) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			mem_access_attr.attr.name);
		goto err_sysfs_create_group;
	}
#endif

#ifdef MXT_CALIBRATE_WORKAROUND
	schedule_delayed_work(&mxt->calibrate_dwork, 10 * HZ);
#endif
	mxt->enabled = true;
	return 0;

err_sysfs_create_group:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mxt->early_suspend);
#endif
	if (mxt->irq)
		free_irq(mxt->irq, mxt);
err_irq:
err_after_read_ot:
	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->delta);
		kfree(mxt->reference);
		kfree(mxt->cte);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}
	input_unregister_device(mxt->input);
	input = NULL;
err_register_device:
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
err_read_ot:
err_identify:
err_pdata:
	if (input)
		input_free_device(input);
err_input_dev_alloc:
	if (mxt->pdata->exit_platform_hw != NULL)
		mxt->pdata->exit_platform_hw();
	kfree(mxt);
err_mxt_alloc:
	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	/* Close down sysfs entries */
	sysfs_remove_group(&client->dev.kobj, &maxtouch_attr_group);

#ifdef CONFIG_HAS_EARLYSUSPEND
	wake_lock_destroy(&mxt->wakelock);
	unregister_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	/* Release IRQ so no queue will be scheduled */
	if (mxt->irq)
		free_irq(mxt->irq, mxt);

	/* Should dealloc deltas, references, CTE structures, if allocated */
	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->delta);
		kfree(mxt->reference);
		kfree(mxt->cte);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}

	input_unregister_device(mxt->input);
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
	if (mxt->pdata->exit_platform_hw != NULL)
		mxt->pdata->exit_platform_hw();
	kfree(mxt);

	i2c_set_clientdata(client, NULL);
	if (debug >= DEBUG_TRACE)
		dev_info(&client->dev, "Touchscreen unregistered\n");

	return 0;
}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static int mxt_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev))
		enable_irq_wake(mxt->irq);

	return 0;
}

static int mxt_resume(struct i2c_client *client)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev))
		disable_irq_wake(mxt->irq);

	return 0;
}
#else
#define mxt_suspend NULL
#define mxt_resume NULL
#endif

static const struct i2c_device_id mxt_idtable[] = {
	{"sec_touchscreen", 0,},
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "sec_touchscreen",
		.owner  = THIS_MODULE,
	},

	.id_table	= mxt_idtable,
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,

};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_driver);
}

static void __exit mxt_cleanup(void)
{
	i2c_del_driver(&mxt_driver);
}

module_init(mxt_init);
module_exit(mxt_cleanup);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Atmel mXT1386 Touchscreen Controller");
MODULE_LICENSE("GPL");

