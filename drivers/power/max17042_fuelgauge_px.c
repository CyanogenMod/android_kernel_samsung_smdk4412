/*
 *  max17042_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/irq.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/wakelock.h>
#include <linux/blkdev.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/rtc.h>
#include <mach/gpio.h>
#if defined(CONFIG_TARGET_LOCALE_KOR)
#include <linux/power_supply.h>
#endif /* CONFIG_TARGET_LOCALE_KOR */
#include <linux/power/sec_battery_px.h>
#include <linux/power/max17042_fuelgauge_px.h>

#define PRINT_COUNT	1

static struct i2c_client *fg_i2c_client;
struct max17042_chip *max17042_chip_data;

static int fg_get_battery_type(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);

	return chip->info.battery_type;
}

static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	s32 value;

	value = i2c_smbus_read_i2c_block_data(client, reg, length, data);
	if (value < 0 || value != length) {
		pr_err("%s: Failed to fg_i2c_read. status = %d\n",
				__func__, value);
		return -1;
	}

	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	s32 value;

	value = i2c_smbus_write_i2c_block_data(client, reg, length, data);
	if (value < 0) {
		pr_err("%s: Failed to fg_i2c_write, error code=%d\n",
				__func__, value);
		return -1;
	}

	return 0;
}

static int fg_read_register(u8 addr)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];

	if (fg_i2c_read(client, addr, data, 2) < 0) {
		pr_err("%s: Failed to read addr(0x%x)\n", __func__, addr);
		return -1;
	}

	return (data[1] << 8) | data[0];
}

static int fg_write_register(u8 addr, u16 w_data)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];

	data[0] = w_data & 0xFF;
	data[1] = w_data >> 8;

	if (fg_i2c_write(client, addr, data, 2) < 0) {
		pr_err("%s: Failed to write addr(0x%x)\n", __func__, addr);
		return -1;
	}

	return 0;
}

static int fg_read_16register(u8 addr, u16 *r_data)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[32];
	int i = 0;

	if (fg_i2c_read(client, addr, data, 32) < 0) {
		pr_err("%s: Failed to read addr(0x%x)\n", __func__, addr);
		return -1;
	}

	for (i = 0; i < 16; i++)
		r_data[i] = (data[2 * i + 1] << 8) | data[2 * i];

	return 0;
}

void fg_write_and_verify_register(u8 addr, u16 w_data)
{
	u16 r_data;
	u8 retry_cnt = 2;

	while (retry_cnt) {
		fg_write_register(addr, w_data);
		r_data = fg_read_register(addr);

		if (r_data != w_data) {
			pr_err("%s: verification failed (addr : 0x%x, w_data : 0x%x, r_data : 0x%x)\n",
				__func__, addr, w_data, r_data);
			retry_cnt--;
		} else
			break;
	}
}

static void fg_debug_print(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 avg_v;
	u16 w_data;
	u32 temp;
	u32 temp2;
	u16 fullcap, remcap, mixcap, avcap;

	if (fg_i2c_read(client, AVR_VCELL_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	avg_v = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	avg_v += (temp2 << 4);

	fullcap = fg_read_register(FULLCAP_REG);
	remcap = fg_read_register(REMCAP_REP_REG);
	mixcap = fg_read_register(REMCAP_MIX_REG);
	avcap = fg_read_register(REMCAP_AV_REG);

	pr_info("avg_v(%d), fullcap(%d), remcap(%d), mixcap(%d), avcap(%d)\n",
		avg_v, (fullcap / 2), (remcap / 2), (mixcap / 2), (avcap / 2));
	msleep(20);
}

static int fg_read_voltage_now(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 voltage_now;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (fg_i2c_read(client, VCELL_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	voltage_now = temp / 1000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000;
	voltage_now += (temp2 << 4);

	return voltage_now;
}

static int fg_read_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	u32 vcell;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (fg_i2c_read(client, VCELL_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	if (!(chip->info.pr_cnt % PRINT_COUNT))
		pr_info("%s: vcell(%d, 0x%04x)\n", __func__,
				vcell, (data[1]<<8) | data[0]);

	return vcell;
}

static int fg_read_avg_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 avg_vcell;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (fg_i2c_read(client, AVR_VCELL_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	avg_vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	avg_vcell += (temp2 << 4);

	return avg_vcell;
}

static int fg_read_vfocv(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vfocv = 0;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (fg_i2c_read(client, VFOCV_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VFOCV\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vfocv = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vfocv += (temp2 << 4);

	return vfocv;
}

static int fg_check_battery_present(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 status_data[2];
	int ret = 1;

	/* 1. Check Bst bit */
	if (fg_i2c_read(client, STATUS_REG, status_data, 2) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return 0;
	}

	if (status_data[0] & (0x1 << 3)) {
		pr_info("%s - addr(0x01), data(0x%04x)\n", __func__,
				(status_data[1]<<8) | status_data[0]);
		pr_info("%s: battery is absent!!\n", __func__);
		ret = 0;
	}

	return ret;
}


static int fg_read_temp(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2] = { 0, };
	int temper = 0;

	if (fg_check_battery_present()) {
		if (fg_i2c_read(client, TEMPERATURE_REG, data, 2) < 0) {
			pr_err("%s: Failed to read TEMPERATURE_REG\n",
					__func__);
			return -1;
		}

		if (data[1]&(0x1 << 7)) {
			temper = ((~(data[1]))&0xFF)+1;
			temper *= (-1000);
		} else {
			temper = data[1] & 0x7f;
			temper *= 1000;
			temper += data[0] * 39 / 10;

			/* Adjust temperature over 47, only for SDI battery */
			if (fg_get_battery_type() == SDI_BATTERY_TYPE) {
				if (temper >= 47000 && temper < 60000) {
					temper = temper * SDI_TRIM1_1 / 100;
					temper -= SDI_TRIM1_2;
				} else if (temper >= 60000) {
					temper = temper * SDI_TRIM2_1 / 100;
					temper -= 51000;
				}
			}
		}
	} else
		temper = 20000;

	if (!(chip->info.pr_cnt % PRINT_COUNT))
		pr_info("%s: temper(%d, 0x%04x)\n", __func__,
				temper, (data[1]<<8) | data[0]);

	return temper;
}

static int fg_read_vfsoc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];

	if (fg_i2c_read(client, VFSOC_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VFSOC\n", __func__);
		return -1;
	}

	return (int)data[1];
}

static int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	u32 soc = 0;
	u32 soc_lsb = 0;
	int psoc = 0;

	if (fg_i2c_read(client, SOCREP_REG, data, 2) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	soc_lsb = (data[0] * 100) / 256;
	psoc = (data[1] * 100) + soc_lsb;
	soc = psoc / 99;
	chip->info.psoc = psoc;

	if (!(chip->info.pr_cnt % PRINT_COUNT))
		pr_info("%s: soc(%d), psoc(%d), vfsoc(%d)\n", __func__,
			soc, psoc, fg_read_vfsoc());

	return soc;
}

static int fg_read_current(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data1[2], data2[2];
	u32 temp, sign;
	s32 i_current;
	s32 avg_current;

	if (fg_i2c_read(client, CURRENT_REG, data1, 2) < 0) {
		pr_err("%s: Failed to read CURRENT\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, AVG_CURRENT_REG, data2, 2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n", __func__);
		return -1;
	}

	temp = ((data1[1]<<8) | data1[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	i_current = temp * MAX17042_CURRENT_UNIT;
	if (sign)
		i_current *= -1;

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	avg_current = temp * MAX17042_CURRENT_UNIT;
	if (sign)
		avg_current *= -1;

	if (!(chip->info.pr_cnt++ % PRINT_COUNT)) {
		pr_info("%s: curr(%d), avg_curr(%dmA)\n", __func__,
				i_current, avg_current);
		chip->info.pr_cnt = 1;

		fg_debug_print();
		/* Read max17042's all registers every 5 minute. */
		fg_periodic_read();
	}

	return i_current;
}

static int fg_read_avg_current(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8  data2[2];
	u32 temp, sign;
	s32 avg_current;

	if (fg_i2c_read(client, AVG_CURRENT_REG, data2, 2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n", __func__);
		return -1;
	}

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	avg_current = temp * MAX17042_CURRENT_UNIT;

	if (sign)
		avg_current *= -1;

	return avg_current;
}

int fg_reset_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
#if defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
	u32 fullcap;
	int vfocv = 0;
#endif

	/* delay for current stablization */
	msleep(500);

	pr_info("%s: %s - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, "Before quick-start", fg_read_vcell(),
		fg_read_vfocv(), fg_read_vfsoc(), fg_read_soc());
	pr_info("%s: Before quick-start - current(%d), avg current(%d)\n",
		__func__, fg_read_current(),
		fg_read_avg_current());

	if (!chip->pdata->check_jig_status()) {
		pr_info("%s: Return by No JIG_ON signal\n", __func__);
		return 0;
	}

	fg_write_register(CYCLES_REG, 0);

	if (fg_i2c_read(client, MISCCFG_REG, data, 2) < 0) {
		pr_err("%s: Failed to read MiscCFG\n", __func__);
		return -1;
	}

	data[1] |= (0x1 << 2);
	if (fg_i2c_write(client, MISCCFG_REG, data, 2) < 0) {
		pr_err("%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(250);
	fg_write_register(FULLCAP_REG, chip->info.capacity);
	msleep(500);

	pr_info("%s: %s - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, "After quick-start", fg_read_vcell(),
		fg_read_vfocv(), fg_read_vfsoc(), fg_read_soc());
	pr_info("%s: After quick-start - current(%d), avg current(%d)\n",
		__func__, fg_read_current(), fg_read_avg_current());
	fg_write_register(CYCLES_REG, 0x00a0);

/* P8 is not turned off by Quickstart @3.4V(It's not a problem, depend on
 * mode data. Power off for factory test(File system, etc..) */
#if defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
#define QUICKSTART_POWER_OFF_VOLTAGE	3400
	vfocv = fg_read_vfocv();
	if (vfocv < QUICKSTART_POWER_OFF_VOLTAGE) {
		pr_info("%s: Power off condition(%d)\n", __func__, vfocv);

		fullcap = fg_read_register(FULLCAP_REG);
		/* FullCAP * 0.009 */
		fg_write_register(REMCAP_REP_REG, (u16)(fullcap * 9 / 1000));
		msleep(200);
		pr_info("%s: new soc=%d, vfocv=%d\n", __func__,
				fg_read_soc(), vfocv);
	}

	pr_info("%s: Additional step - VfOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, fg_read_vfocv(), fg_read_vfsoc(), fg_read_soc());
#endif

	return 0;
}


int fg_reset_capacity(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);

	return fg_write_register(DESIGNCAP_REG, chip->info.vfcapacity-1);
}

int fg_adjust_capacity(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 data[2];

	data[0] = 0;
	data[1] = 0;

	/* 1. Write RemCapREP(05h)=0; */
	if (fg_i2c_write(client, REMCAP_REP_REG, data, 2) < 0) {
		pr_err("%s: Failed to write RemCap_REP\n", __func__);
		return -1;
	}
	msleep(200);

	pr_info("%s: After adjust - RepSOC(%d)\n", __func__, fg_read_soc());

	chip->info.soc_restart_flag = 1;

	return 0;
}

void fg_low_batt_compensation(u32 level)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	int read_val;
	u32 temp;

	pr_info("%s: Adjust SOCrep to %d!!\n", __func__, level);

	read_val = fg_read_register(FULLCAP_REG);
	if (read_val < 0)
		return;

	/* RemCapREP (05h) = FullCap(10h) x 0.0304 or 0.0104 */
	temp = read_val * (level*100 + 4) / 10000;
	fg_write_register(REMCAP_REP_REG, (u16)temp);

	/* (0x67) * 0.0039 = 0.4%*/
	temp = 0;
	temp = (u16)((level << 8) | 0x67);
	fg_write_register(SOCREP_REG, temp);

	chip->info.low_batt_comp_flag = 1;
}

void fg_periodic_read(void)
{
	u8 reg;
	int i;
	int data[0x10];
	struct timespec ts;
	struct rtc_time tm;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

	pr_info("[MAX17042] %d/%d/%d %02d:%02d,",
		tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
		tm.tm_hour, tm.tm_min);

	for (i = 0; i < 16; i++) {
		for (reg = 0; reg < 0x10; reg++)
			data[reg] = fg_read_register(reg + i * 0x10);

		pr_info("%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,\n"
			"%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,",
			data[0x00], data[0x01], data[0x02], data[0x03],
			data[0x04], data[0x05], data[0x06], data[0x07],
			data[0x08], data[0x09], data[0x0a], data[0x0b],
			data[0x0c], data[0x0d], data[0x0e], data[0x0f]);
		if (i == 4)
			i = 13;
		msleep(20);
	}
	pr_info("\n");
}

static void fg_read_model_data(void)
{
	u16 data0[16], data1[16], data2[16];
	int i;
	int relock_check;

	pr_info("[FG_Model] ");

	/* Unlock model access */
	fg_write_register(0x62, 0x0059);
	fg_write_register(0x63, 0x00C4);

	/* Read model data */
	fg_read_16register(0x80, data0);
	fg_read_16register(0x90, data1);
	fg_read_16register(0xa0, data2);

	/* Print model data */
	for (i = 0; i < 16; i++)
		pr_info("0x%04x, ", data0[i]);

	for (i = 0; i < 16; i++)
		pr_info("0x%04x, ", data1[i]);

	for (i = 0; i < 16; i++) {
		if (i == 15)
			pr_info("0x%04x", data2[i]);
		else
			pr_info("0x%04x, ", data2[i]);
	}

	do {
		relock_check = 0;
		/* Lock model access */
		fg_write_register(0x62, 0x0000);
		fg_write_register(0x63, 0x0000);

		/* Read model data again */
		fg_read_16register(0x80, data0);
		fg_read_16register(0x90, data1);
		fg_read_16register(0xa0, data2);

		for (i = 0; i < 16; i++) {
			if (data0[i] || data1[i] || data2[i]) {
				pr_info("%s: data is non-zero, lock again!\n",
						__func__);
				relock_check = 1;
			}
		}
	} while (relock_check);

}

int fg_alert_init(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 misccgf_data[2];
	u8 salrt_data[2];
	u8 config_data[2];
	u8 valrt_data[2];
	u8 talrt_data[2];
	u16 read_data = 0;

	/* Using RepSOC */
	if (fg_i2c_read(client, MISCCFG_REG, misccgf_data, 2) < 0) {
		pr_err("%s: Failed to read MISCCFG_REG\n", __func__);
		return -1;
	}
	misccgf_data[0] = misccgf_data[0] & ~(0x03);

	if (fg_i2c_write(client, MISCCFG_REG, misccgf_data, 2) < 0) {
		pr_info("%s: Failed to write MISCCFG_REG\n", __func__);
		return -1;
	}

	/* SALRT Threshold setting */
	salrt_data[1] = 0xff;
	salrt_data[0] = 0x01;
	if (fg_i2c_write(client, SALRT_THRESHOLD_REG, salrt_data, 2) < 0) {
		pr_info("%s: Failed to write SALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	/* Reset VALRT Threshold setting (disable) */
	valrt_data[1] = 0xFF;
	valrt_data[0] = 0x00;
	if (fg_i2c_write(client, VALRT_THRESHOLD_REG, valrt_data, 2) < 0) {
		pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	read_data = fg_read_register((u8)VALRT_THRESHOLD_REG);
	if (read_data != 0xff00)
		pr_err("%s: VALRT_THRESHOLD_REG is not valid (0x%x)\n",
				__func__, read_data);

	/* Reset TALRT Threshold setting (disable) */
	talrt_data[1] = 0x7F;
	talrt_data[0] = 0x80;
	if (fg_i2c_write(client, TALRT_THRESHOLD_REG, talrt_data, 2) < 0) {
		pr_info("%s: Failed to write TALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	read_data = fg_read_register((u8)TALRT_THRESHOLD_REG);
	if (read_data != 0x7f80)
		pr_err("%s: TALRT_THRESHOLD_REG is not valid (0x%x)\n",
				__func__, read_data);

	mdelay(100);

	/* Enable SOC alerts */
	if (fg_i2c_read(client, CONFIG_REG, config_data, 2) < 0) {
		pr_err("%s: Failed to read CONFIG_REG\n", __func__);
		return -1;
	}
	config_data[0] = config_data[0] | (0x1 << 2);

	if (fg_i2c_write(client, CONFIG_REG, config_data, 2) < 0) {
		pr_info("%s: Failed to write CONFIG_REG\n", __func__);
		return -1;
	}

	return 1;
}

static int fg_check_status_reg(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 status_data[2];
	int ret = 0;

	/* 1. Check Smn was generatedread */
	if (fg_i2c_read(client, STATUS_REG, status_data, 2) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return -1;
	}
	pr_info("%s - addr(0x00), data(0x%04x)\n", __func__,
			(status_data[1]<<8) | status_data[0]);

	if (status_data[1] & (0x1 << 2))
		ret = 1;

	/* 2. clear Status reg */
	status_data[1] = 0;
	if (fg_i2c_write(client, STATUS_REG, status_data, 2) < 0) {
		pr_info("%s: Failed to write STATUS_REG\n", __func__);
		return -1;
	}

	return ret;
}

void fg_fullcharged_compensation(u32 is_recharging, u32 pre_update)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	static int new_fcap;
	pr_info("%s: is_recharging(%d), pre_update(%d)\n", __func__,
					is_recharging, pre_update);

	new_fcap = fg_read_register(FULLCAP_REG);
	if (new_fcap < 0)
		new_fcap = chip->info.capacity;

	pr_info("%s: prevFCap(0x%04x), newFcap(0x%04x)\n", __func__,
			chip->info.prev_fullcap, new_fcap);
	if (new_fcap > (chip->info.capacity * 110 / 100)) {
		new_fcap = (chip->info.capacity * 110) / 100;
		pr_info("%s: init FCap +10%%: newFcap(0x%04x)\n",
						__func__, new_fcap);
		fg_write_register(REMCAP_REP_REG, (u16)(new_fcap));
		fg_write_register(FULLCAP_REG, (u16)(new_fcap));
	} else if (new_fcap < (chip->info.capacity * 50 / 100)) {
		new_fcap = (chip->info.capacity * 50) / 100;
		pr_info("%s: init FCap -50%%: newFcap(0x%04x)\n",
						__func__, new_fcap);
		fg_write_register(REMCAP_REP_REG, (u16)(new_fcap));
		fg_write_register(FULLCAP_REG, (u16)(new_fcap));
	} else {
		if (new_fcap > (chip->info.prev_fullcap * 110 / 100)) {
			new_fcap = (chip->info.prev_fullcap * 110) / 100;
			pr_info("%s: prev FCap +10%%: newFcap(0x%04x)\n",
						__func__, new_fcap);
			fg_write_register(REMCAP_REP_REG, (u16)(new_fcap));
			fg_write_register(FULLCAP_REG, (u16)(new_fcap));
		} else if (new_fcap < (chip->info.prev_fullcap * 90 / 100)) {
			new_fcap = (chip->info.prev_fullcap * 90) / 100;
			pr_info("%s: prev FCap -10%%: newFcap(0x%04x)\n",
						__func__, new_fcap);
			fg_write_register(REMCAP_REP_REG, (u16)(new_fcap));
			fg_write_register(FULLCAP_REG, (u16)(new_fcap));
		} else {
			pr_info("%s: keep FCap: newFcap(0x%04x)\n",
						__func__, new_fcap);
		}
	}

	/* 4. Write RepSOC(06h)=100%; */
	fg_write_register(SOCREP_REG, (u16)(0x64 << 8));

	/* 5. Write MixSOC(0Dh)=100%; */
	fg_write_register(SOCMIX_REG, (u16)(0x64 << 8));

	/* 6. Write AVSOC(0Eh)=100%; */
	fg_write_register(SOCAV_REG, (u16)(0x64 << 8));

	/* if pre_update case, skip updating PrevFullCAP value. */
	if (!pre_update)
		chip->info.prev_fullcap = fg_read_register(FULLCAP_REG);

	pr_info("%s: FullCap(0x%04x), RemCap(0x%04x)\n", __func__,
				fg_read_register(FULLCAP_REG),
				fg_read_register(REMCAP_REP_REG));

	fg_periodic_read();
}

void fg_check_vf_fullcap_range(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	static int new_vffcap;
	u16 print_flag = 1;
	pr_debug("%s\n", __func__);

	new_vffcap = fg_read_register(FULLCAP_NOM_REG);
	if (new_vffcap < 0)
		new_vffcap = chip->info.vfcapacity;

	pr_info("%s: prevVFFCap(0x%04x), newVFFcap(0x%04x)\n", __func__,
			chip->info.prev_vffcap, new_vffcap);
	if (new_vffcap > (chip->info.vfcapacity * 110 / 100)) {
		new_vffcap = (chip->info.vfcapacity * 110) / 100;
		pr_info("%s: init VFFCap +10%%: newVFFcap(0x%04x)\n",
						__func__, new_vffcap);
		fg_write_register(DQACC_REG, (u16)(new_vffcap / 4));
		fg_write_register(DPACC_REG, (u16)0x3200);
	} else if (new_vffcap < (chip->info.vfcapacity * 50 / 100)) {
		new_vffcap = (chip->info.vfcapacity * 50) / 100;
		pr_info("%s: init VFFCap -50%%: newVFFcap(0x%04x)\n",
						__func__, new_vffcap);
		fg_write_register(DQACC_REG, (u16)(new_vffcap / 4));
		fg_write_register(DPACC_REG, (u16)0x3200);
	} else {
		if (new_vffcap > (chip->info.prev_vffcap * 105 / 100)) {
			new_vffcap = (chip->info.prev_vffcap * 105) / 100;
			pr_info("%s: prev VFFCap +10%%: newVFFcap(0x%04x)\n",
							__func__, new_vffcap);
			fg_write_register(DQACC_REG, (u16)(new_vffcap / 4));
			fg_write_register(DPACC_REG, (u16)0x3200);
		} else if (new_vffcap < (chip->info.prev_vffcap * 90 / 100)) {
			new_vffcap = (chip->info.prev_vffcap * 90) / 100;
			pr_info("%s: prev VFFCap -10%%: newVFFcap(0x%04x)\n",
						__func__, new_vffcap);
			fg_write_register(DQACC_REG, (u16)(new_vffcap / 4));
			fg_write_register(DPACC_REG, (u16)0x3200);
		} else {
			pr_info("%s: keep VFFCap: newVFFcap(0x%04x)\n",
						__func__, new_vffcap);
			print_flag = 0;
		}
	}

	/* delay for register setting (dQacc, dPacc) */
	if (print_flag)
		msleep(300);

	chip->info.prev_vffcap = fg_read_register(FULLCAP_NOM_REG);

	if (print_flag)
		pr_info("%s: VfFullCap(0x%04x), dQacc(0x%04x), dPacc(0x%04x)\n",
			__func__,
			fg_read_register(FULLCAP_NOM_REG),
			fg_read_register(DQACC_REG),
			fg_read_register(DPACC_REG));
}

int fg_check_cap_corruption(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	int vfsoc = fg_read_vfsoc();
	int repsoc = fg_read_soc();
	int mixcap = fg_read_register(REMCAP_MIX_REG);
	int vfocv = fg_read_register(VFOCV_REG);
	int remcap = fg_read_register(REMCAP_REP_REG);
	int fullcapacity = fg_read_register(FULLCAP_REG);
	int vffullcapacity = fg_read_register(FULLCAP_NOM_REG);
	u32 temp, temp2, new_vfocv, pr_vfocv;
	int designcap;
	int ret = 0;

	/* If usgin Jig or low batt compensation flag is set,
	   then skip checking. */
	if (chip->pdata->check_jig_status()) {
		fg_write_register(DESIGNCAP_REG, chip->info.vfcapacity - 1);
		designcap = fg_read_register(DESIGNCAP_REG);
		pr_info("%s: return by jig, vfcap(0x%04x), designcap(0x%04x)\n",
				__func__, chip->info.vfcapacity, designcap);
		return 0;
	}

	if (vfsoc < 0 || repsoc < 0 || mixcap < 0 || vfocv < 0 ||
		remcap < 0 || fullcapacity < 0 || vffullcapacity < 0)
		return 0;

	/* Check full charge learning case. */
	if (((vfsoc >= 70)
		&& ((remcap >= (fullcapacity * 995 / 1000))
		&& (remcap <= (fullcapacity * 1005 / 1000))))
		|| chip->info.low_batt_comp_flag
		|| chip->info.soc_restart_flag) {

		chip->info.prev_repsoc = repsoc;
		chip->info.prev_remcap = remcap;
		chip->info.prev_fullcapacity = fullcapacity;
		if (chip->info.soc_restart_flag)
			chip->info.soc_restart_flag = 0;

		ret = 1;
	}

	/* ocv calculation for print */
	temp = (vfocv & 0xFFF) * 78125;
	pr_vfocv = temp / 1000000;

	temp = ((vfocv & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	pr_vfocv += (temp2 << 4);

	/* MixCap differ is greater than 265mAh */
	if ((((vfsoc+5) < chip->info.prev_vfsoc)
			|| (vfsoc > (chip->info.prev_vfsoc+5)))
		|| (((repsoc+5) < chip->info.prev_repsoc)
			|| (repsoc > (chip->info.prev_repsoc+5)))
		|| (((mixcap+530) < chip->info.prev_mixcap)
			|| (mixcap > (chip->info.prev_mixcap+530)))) {
		fg_periodic_read();

		pr_info("[FG_Recovery] (B) VfSOC(%d), prevVfSOC(%d),",
			vfsoc, chip->info.prev_vfsoc);
		pr_info(" RepSOC(%d), prevRepSOC(%d), MixCap(%d),",
			repsoc, chip->info.prev_repsoc, (mixcap/2));
		pr_info(" prevMixCap(%d),VfOCV(0x%04x, %d)\n",
			(chip->info.prev_mixcap/2), vfocv, pr_vfocv);

		mutex_lock(&chip->fg_lock);

		fg_write_and_verify_register(REMCAP_MIX_REG,
						chip->info.prev_mixcap);
		fg_write_register(VFOCV_REG, chip->info.prev_vfocv);
		mdelay(200);

		fg_write_and_verify_register(REMCAP_REP_REG,
						chip->info.prev_remcap);
		vfsoc = fg_read_register(VFSOC_REG);
		fg_write_register(0x60, 0x0080);
		fg_write_and_verify_register(0x48, vfsoc);
		fg_write_register(0x60, 0x0000);

		fg_write_and_verify_register(0x45,
				(chip->info.prev_vfcapacity / 4));
		fg_write_and_verify_register(0x46, 0x3200);
		fg_write_and_verify_register(FULLCAP_REG,
				chip->info.prev_fullcapacity);
		fg_write_and_verify_register(FULLCAP_NOM_REG,
				chip->info.prev_vfcapacity);

		mutex_unlock(&chip->fg_lock);

		msleep(200);

		/* ocv calculation for print */
		new_vfocv = fg_read_register(VFOCV_REG);
		temp = (new_vfocv & 0xFFF) * 78125;
		pr_vfocv = temp / 1000000;

		temp = ((new_vfocv & 0xF000) >> 4) * 78125;
		temp2 = temp / 1000000;
		pr_vfocv += (temp2 << 4);

		pr_info("[FG_Recovery] (A) newVfSOC(%d), newRepSOC(%d),",
			fg_read_vfsoc(),
			fg_read_soc());
		pr_info(" newMixCap(%d), newVfOCV(0x%04x, %d)\n",
			(fg_read_register(REMCAP_MIX_REG)/2),
			new_vfocv,
			pr_vfocv);

		fg_periodic_read();

		ret = 1;
	} else {
		chip->info.prev_vfsoc = vfsoc;
		chip->info.prev_repsoc = repsoc;
		chip->info.prev_remcap = remcap;
		chip->info.prev_mixcap = mixcap;
		chip->info.prev_fullcapacity = fullcapacity;
		chip->info.prev_vfcapacity = vffullcapacity;
		chip->info.prev_vfocv = vfocv;
	}

	return ret;
}

void fg_set_full_charged(void)
{
	pr_info("[FG_Set_Full] (B) FullCAP(%d), RemCAP(%d)\n",
		(fg_read_register(FULLCAP_REG)/2),
		(fg_read_register(REMCAP_REP_REG)/2));

	fg_write_register(FULLCAP_REG, (u16)fg_read_register(REMCAP_REP_REG));

	pr_info("[FG_Set_Full] (A) FullCAP(%d), RemCAP(%d)\n",
		(fg_read_register(FULLCAP_REG)/2),
		(fg_read_register(REMCAP_REP_REG)/2));
}

static void display_low_batt_comp_cnt(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	u8 type_str[10];

	if (fg_get_battery_type() == SDI_BATTERY_TYPE)
		sprintf(type_str, "SDI");
	else if (fg_get_battery_type() == ATL_BATTERY_TYPE)
		sprintf(type_str, "ATL");
	else if (fg_get_battery_type() == BYD_BATTERY_TYPE)
		sprintf(type_str, "BYD");
	else
		sprintf(type_str, "Unknown");

	pr_info("Check Array(%s):[%d,%d], [%d,%d], [%d,%d], [%d,%d], [%d,%d]\n",
			type_str,
			chip->info.low_batt_comp_cnt[0][0],
			chip->info.low_batt_comp_cnt[0][1],
			chip->info.low_batt_comp_cnt[1][0],
			chip->info.low_batt_comp_cnt[1][1],
			chip->info.low_batt_comp_cnt[2][0],
			chip->info.low_batt_comp_cnt[2][1],
			chip->info.low_batt_comp_cnt[3][0],
			chip->info.low_batt_comp_cnt[3][1],
			chip->info.low_batt_comp_cnt[4][0],
			chip->info.low_batt_comp_cnt[4][1]);
}

static void add_low_batt_comp_cnt(int range, int level)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	int i;
	int j;

	/* Increase the requested count value, and reset others. */
	chip->info.low_batt_comp_cnt[range-1][level/2]++;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (i == range-1 && j == level/2)
				continue;
			else
				chip->info.low_batt_comp_cnt[i][j] = 0;
		}
	}
}

#define POWER_OFF_SOC_HIGH_MARGIN	0x200
#define POWER_OFF_VOLTAGE_HIGH_MARGIN	3500
#define POWER_OFF_VOLTAGE_NOW_LOW_MARGIN	3350
#define POWER_OFF_VOLTAGE_AVG_LOW_MARGIN	3400

void prevent_early_late_poweroff(int vcell, int *fg_soc)
{
	struct i2c_client *client = fg_i2c_client;
	int repsoc, repsoc_data = 0;
	int read_val;
	int avg_vcell;
	u8 data[2];

	if (fg_i2c_read(client, SOCREP_REG, data, 2) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return;
	}
	repsoc = data[1];
	repsoc_data = ((data[1] << 8) | data[0]);

	if (repsoc_data > POWER_OFF_SOC_HIGH_MARGIN)
		return;

	avg_vcell = fg_read_avg_vcell();
	pr_info("%s: soc(%d%%(0x%04x)), v(%d), avg_v(%d)\n",
			__func__, repsoc, repsoc_data, vcell, avg_vcell);

	if (vcell > POWER_OFF_VOLTAGE_HIGH_MARGIN) {
		read_val = fg_read_register(FULLCAP_REG);
		/* FullCAP * 0.013 */
		fg_write_register(REMCAP_REP_REG, (u16)(read_val * 13 / 1000));
		msleep(200);
		*fg_soc = fg_read_soc();
		pr_info("%s: 1.3%% case: new soc(%d), v(%d), avg_v(%d)\n",
					__func__, *fg_soc, vcell, avg_vcell);
	} else if ((vcell < POWER_OFF_VOLTAGE_NOW_LOW_MARGIN) &&
		(avg_vcell < POWER_OFF_VOLTAGE_AVG_LOW_MARGIN)) {
		read_val = fg_read_register(FULLCAP_REG);
		/* FullCAP * 0.009 */
		fg_write_register(REMCAP_REP_REG, (u16)(read_val * 9 / 1000));
		msleep(200);
		*fg_soc = fg_read_soc();
		pr_info("%s: 0%% case: new soc(%d), v(%d), avg_v(%d)\n",
					__func__, *fg_soc, vcell, avg_vcell);
	}
}

void reset_low_batt_comp_cnt(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);

	memset(chip->info.low_batt_comp_cnt, 0,
			sizeof(chip->info.low_batt_comp_cnt));
}

static int check_low_batt_comp_condtion(int *nLevel)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	int i;
	int j;
	int ret = 0;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (chip->info.low_batt_comp_cnt[i][j] \
						>= MAX_LOW_BATT_CHECK_CNT) {
				display_low_batt_comp_cnt();
				ret = 1;
				*nLevel = j*2 + 1;
				break;
			}
		}
	}

	return ret;
}

static int get_low_batt_threshold(int range, int level, int nCurrent)
{
	int ret = 0;

	if (fg_get_battery_type() == SDI_BATTERY_TYPE) {
		switch (range) {
/* P4 & P8 needs one more level */
#if defined(CONFIG_MACH_P4NOTE) \
	|| defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
		case 5:
			if (level == 1)
				ret = SDI_Range5_1_Offset + \
				      ((nCurrent * SDI_Range5_1_Slope) / 1000);
			else if (level == 3)
				ret = SDI_Range5_3_Offset + \
				      ((nCurrent * SDI_Range5_3_Slope) / 1000);
			break;
#endif
		case 4:
			if (level == 1)
				ret = SDI_Range4_1_Offset + \
				      ((nCurrent * SDI_Range4_1_Slope) / 1000);
			else if (level == 3)
				ret = SDI_Range4_3_Offset + \
				      ((nCurrent * SDI_Range4_3_Slope) / 1000);
			break;

		case 3:
			if (level == 1)
				ret = SDI_Range3_1_Offset + \
				      ((nCurrent * SDI_Range3_1_Slope) / 1000);
			else if (level == 3)
				ret = SDI_Range3_3_Offset + \
				      ((nCurrent * SDI_Range3_3_Slope) / 1000);
			break;

		case 2:
			if (level == 1)
				ret = SDI_Range2_1_Offset + \
				      ((nCurrent * SDI_Range2_1_Slope) / 1000);
			else if (level == 3)
				ret = SDI_Range2_3_Offset + \
				      ((nCurrent * SDI_Range2_3_Slope) / 1000);
			break;

		case 1:
			if (level == 1)
				ret = SDI_Range1_1_Offset + \
				      ((nCurrent * SDI_Range1_1_Slope) / 1000);
			else if (level == 3)
				ret = SDI_Range1_3_Offset + \
				      ((nCurrent * SDI_Range1_3_Slope) / 1000);
			break;

		default:
			break;
		}
	}  else if (fg_get_battery_type() == ATL_BATTERY_TYPE) {
		switch (range) {
		case 4:
			if (level == 1)
				ret = ATL_Range4_1_Offset + \
				      ((nCurrent * ATL_Range4_1_Slope) / 1000);
			else if (level == 3)
				ret = ATL_Range4_3_Offset + \
				      ((nCurrent * ATL_Range4_3_Slope) / 1000);
			break;

		case 3:
			if (level == 1)
				ret = ATL_Range3_1_Offset + \
				      ((nCurrent * ATL_Range3_1_Slope) / 1000);
			else if (level == 3)
				ret = ATL_Range3_3_Offset + \
				      ((nCurrent * ATL_Range3_3_Slope) / 1000);
			break;

		case 2:
			if (level == 1)
				ret = ATL_Range2_1_Offset + \
				      ((nCurrent * ATL_Range2_1_Slope) / 1000);
			else if (level == 3)
				ret = ATL_Range2_3_Offset + \
				      ((nCurrent * ATL_Range2_3_Slope) / 1000);
			break;

		case 1:
			if (level == 1)
				ret = ATL_Range1_1_Offset + \
				      ((nCurrent * ATL_Range1_1_Slope) / 1000);
			else if (level == 3)
				ret = ATL_Range1_3_Offset + \
				      ((nCurrent * ATL_Range1_3_Slope) / 1000);
			break;

		default:
			break;
		}
	}
#if defined(CONFIG_MACH_P4NOTE)
	else if (fg_get_battery_type() == BYD_BATTERY_TYPE) {
		switch (range) {
		case 5:
			if (level == 1)
				ret = BYD_Range5_1_Offset + \
				      ((nCurrent * BYD_Range5_1_Slope) / 1000);
			else if (level == 3)
				ret = BYD_Range5_3_Offset + \
				      ((nCurrent * BYD_Range5_3_Slope) / 1000);
			break;
		case 4:
			if (level == 1)
				ret = BYD_Range4_1_Offset + \
				      ((nCurrent * BYD_Range4_1_Slope) / 1000);
			else if (level == 3)
				ret = BYD_Range4_3_Offset + \
				      ((nCurrent * BYD_Range4_3_Slope) / 1000);
			break;

		case 3:
			if (level == 1)
				ret = BYD_Range3_1_Offset + \
				      ((nCurrent * BYD_Range3_1_Slope) / 1000);
			else if (level == 3)
				ret = BYD_Range3_3_Offset + \
				      ((nCurrent * BYD_Range3_3_Slope) / 1000);
			break;

		case 2:
			if (level == 1)
				ret = BYD_Range2_1_Offset + \
				      ((nCurrent * BYD_Range2_1_Slope) / 1000);
			else if (level == 3)
				ret = BYD_Range2_3_Offset + \
				      ((nCurrent * BYD_Range2_3_Slope) / 1000);
			break;

		case 1:
			if (level == 1)
				ret = BYD_Range1_1_Offset + \
				      ((nCurrent * BYD_Range1_1_Slope) / 1000);
			else if (level == 3)
				ret = BYD_Range1_3_Offset + \
				      ((nCurrent * BYD_Range1_3_Slope) / 1000);
			break;

		default:
			break;
		}
	}
#endif
	return ret;
}

void fg_reset_fullcap_in_fullcharge(void)
{
	u32 fullCap = fg_read_register(FULLCAP_REG);
	u32 remCapRep = fg_read_register(REMCAP_REP_REG);
	if (!fullCap || !remCapRep) {
		pr_err("%s: Possible i2c failure !!!\n", __func__);
		return;
	}

	pr_info("%s: FullCap(%d), RemCapRep(%d)\n",
			__func__,
			fullCap,
			remCapRep);

	if (remCapRep < fullCap) {
		/* Usually we need a delay of 100ms here for the
		 * RemCapRep to adjust to the new FullCap but
		 * here this call is being made in an interrupt where
		 * we already follow this call with a delay
		 * of 300ms, hence avoided.
		 */
		fg_write_register(FULLCAP_REG, remCapRep);
	 }
	return;
}

int low_batt_compensation(int fg_soc, int fg_vcell, int fg_current)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);
	int fg_avg_current = 0;
	int fg_min_current = 0;
	int new_level = 0;
	int bCntReset = 0;

	/* Not charging, flag is none, Under 3.60V or 3.45V */
	if (!chip->info.low_batt_comp_flag && \
			(fg_vcell <= chip->info.check_start_vol)) {

		fg_avg_current = fg_read_avg_current();
		fg_min_current = min(fg_avg_current, fg_current);

		if (fg_min_current < CURRENT_RANGE_MAX) {
			if (fg_soc >= 2 && \
				fg_vcell < get_low_batt_threshold( \
						CURRENT_RANGE_MAX_NUM,
						1, fg_min_current))
				add_low_batt_comp_cnt(CURRENT_RANGE_MAX_NUM, 1);
			else if (fg_soc >= 4 && \
				fg_vcell < get_low_batt_threshold( \
						CURRENT_RANGE_MAX_NUM,
						3, fg_min_current))
				add_low_batt_comp_cnt(CURRENT_RANGE_MAX_NUM, 3);
			else
				bCntReset = 1;
		}
/* P4 & P8 needs more level */
#if defined(CONFIG_MACH_P4NOTE) \
	|| defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
		else if (fg_min_current >= CURRENT_RANGE5 && \
				fg_min_current < CURRENT_RANGE4) {
			if (fg_soc >= 2 && fg_vcell < \
				get_low_batt_threshold(4, 1, fg_min_current))

				add_low_batt_comp_cnt(4, 1);
			else if (fg_soc >= 4 && fg_vcell < \
				get_low_batt_threshold(4, 3, fg_min_current))

				add_low_batt_comp_cnt(4, 3);
			else
				bCntReset = 1;
		}
#endif
		else if (fg_min_current >= CURRENT_RANGE4 && \
				fg_min_current < CURRENT_RANGE3) {
			if (fg_soc >= 2 && fg_vcell < \
				get_low_batt_threshold(3, 1, fg_min_current))

				add_low_batt_comp_cnt(3, 1);
			else if (fg_soc >= 4 && fg_vcell < \
				get_low_batt_threshold(3, 3, fg_min_current))
				add_low_batt_comp_cnt(3, 3);
			else
				bCntReset = 1;
		} else if (fg_min_current >= CURRENT_RANGE3 && \
				fg_min_current < CURRENT_RANGE2) {
			if (fg_soc >= 2 && fg_vcell < \
				get_low_batt_threshold(2, 1, fg_min_current))

				add_low_batt_comp_cnt(2, 1);
			else if (fg_soc >= 4 && fg_vcell < \
				get_low_batt_threshold(2, 3, fg_min_current))

				add_low_batt_comp_cnt(2, 3);
			else
				bCntReset = 1;
		} else if (fg_min_current >= CURRENT_RANGE2 && \
				fg_min_current < CURRENT_RANGE1) {
			if (fg_soc >= 2 && fg_vcell < \
				get_low_batt_threshold(1, 1, fg_min_current))

				add_low_batt_comp_cnt(1, 1);
			else if (fg_soc >= 4 && fg_vcell < \
				get_low_batt_threshold(1, 3, fg_min_current))

				add_low_batt_comp_cnt(1, 3);
			else
				bCntReset = 1;
		}


		if (check_low_batt_comp_condtion(&new_level)) {
#if defined(CONFIG_MACH_P4NOTE) || \
	defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
			/*
			 * Disable 3% low battery compensation
			 * duplicated action with 1% low battery compensation
			 */
			if (new_level < 2)
#endif
				fg_low_batt_compensation(new_level);
			reset_low_batt_comp_cnt();
		}

		if (bCntReset)
			reset_low_batt_comp_cnt();

		/* if compensation finished, then read SOC again!!*/
		if (chip->info.low_batt_comp_flag) {
			pr_info("%s: MIN_CURRENT(%d), AVG_CURRENT(%d),",
				__func__, fg_min_current, fg_avg_current);
			pr_info(" CURRENT(%d), SOC(%d), VCELL(%d)\n",
				fg_current, fg_soc, fg_vcell);
#if defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
	/* Do not update soc right after low battery compensation */
	/* to prevent from powering-off suddenly (only for P8s) */
			pr_info("%s: SOC is set to %d\n",
				__func__, fg_read_soc());
#else
			fg_soc = fg_read_soc();
			pr_info("%s: SOC is set to %d\n", __func__, fg_soc);
#endif
		}
	}

#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_P2)
	/* Prevent power off over 3500mV */
	/* Force power off under 3400mV */
	prevent_early_late_poweroff(fg_vcell, &fg_soc);
#endif

	return fg_soc;
}

static void fg_set_battery_type(void)
{
	struct i2c_client *client = fg_i2c_client;
	struct max17042_chip *chip = i2c_get_clientdata(client);

	u16 data;
	u8 type_str[10];

	data = fg_read_register(DESIGNCAP_REG);

	if ((data == chip->pdata->sdi_vfcapacity) || \
			(data == chip->pdata->sdi_vfcapacity-1))
		chip->info.battery_type = SDI_BATTERY_TYPE;
	else if ((data == chip->pdata->atl_vfcapacity) || \
			(data == chip->pdata->atl_vfcapacity-1))
		chip->info.battery_type = ATL_BATTERY_TYPE;
	else if ((data == chip->pdata->byd_vfcapacity) || \
			(data == chip->pdata->byd_vfcapacity-1))
		chip->info.battery_type = BYD_BATTERY_TYPE;
	else {
		pr_info("%s: Unknown battery is set to SDI type.\n", __func__);
		chip->info.battery_type = SDI_BATTERY_TYPE;
	}

	if (chip->info.battery_type == SDI_BATTERY_TYPE)
		sprintf(type_str, "SDI");
	else if (chip->info.battery_type == ATL_BATTERY_TYPE)
		sprintf(type_str, "ATL");
	else if (chip->info.battery_type == BYD_BATTERY_TYPE)
		sprintf(type_str, "BYD");
	else
		sprintf(type_str, "Unknown");

	pr_info("%s: DesignCAP(0x%04x), Battery type(%s)\n",
			__func__, data, type_str);

	switch (chip->info.battery_type) {
	case ATL_BATTERY_TYPE:
		chip->info.capacity = chip->pdata->atl_capacity;
		chip->info.vfcapacity = chip->pdata->atl_vfcapacity;
		break;
	case BYD_BATTERY_TYPE:
		chip->info.capacity = chip->pdata->byd_capacity;
		chip->info.vfcapacity = chip->pdata->byd_vfcapacity;
		break;
	case SDI_BATTERY_TYPE:
	default:
		chip->info.capacity = chip->pdata->sdi_capacity;
		chip->info.vfcapacity = chip->pdata->sdi_vfcapacity;
		break;
	}

	/* If not initialized yet, then init threshold values. */
	if (!chip->info.check_start_vol) {
		if (chip->info.battery_type == SDI_BATTERY_TYPE)
			chip->info.check_start_vol = \
					chip->pdata->sdi_low_bat_comp_start_vol;
		else if (chip->info.battery_type == ATL_BATTERY_TYPE)
			chip->info.check_start_vol = \
					chip->pdata->atl_low_bat_comp_start_vol;
		else if (chip->info.battery_type == BYD_BATTERY_TYPE)
			chip->info.check_start_vol = \
					chip->pdata->byd_low_bat_comp_start_vol;
	}

}

int get_fuelgauge_capacity(enum capacity_type type)
{
	int cap = -1;
	pr_debug("%s\n", __func__);

	switch (type) {
	case CAPACITY_TYPE_FULL:
		cap = fg_read_register(FULLCAP_REG);
		break;
	case CAPACITY_TYPE_MIX:
		cap = fg_read_register(REMCAP_MIX_REG);
		break;
	case CAPACITY_TYPE_AV:
		cap = fg_read_register(REMCAP_AV_REG);
		break;
	case CAPACITY_TYPE_REP:
		cap = fg_read_register(REMCAP_REP_REG);
		break;
	default:
		pr_info("%s: invalid type(%d)\n", __func__, type);
		cap = -EINVAL;
		break;
	}

	pr_debug("%s: type(%d), cap(0x%x, %d)\n", __func__,
					type, cap, (cap / 2));
	return cap;
}

int get_fuelgauge_value(int data)
{
	int ret;

	switch (data) {
	case FG_LEVEL:
		ret = fg_read_soc();
		break;

	case FG_TEMPERATURE:
		ret = fg_read_temp();
		break;

	case FG_VOLTAGE:
		ret = fg_read_vcell();
		break;

	case FG_CURRENT:
		ret = fg_read_current();
		break;

	case FG_CURRENT_AVG:
		ret = fg_read_avg_current();
		break;

	case FG_BATTERY_TYPE:
		ret = fg_get_battery_type();
		break;

	case FG_CHECK_STATUS:
		ret = fg_check_status_reg();
		break;

	case FG_VF_SOC:
		ret = fg_read_vfsoc();
		break;

	case FG_VOLTAGE_NOW:
		ret = fg_read_voltage_now();
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}

static int fg_i2c_remove(struct i2c_client *client)
{
	struct max17042_chip *chip = i2c_get_clientdata(client);

	kfree(chip);
	return 0;
}

#if defined(CONFIG_TARGET_LOCALE_KOR)
static int max17042_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = get_fuelgauge_value(FG_VOLTAGE);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = get_fuelgauge_value(FG_LEVEL);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property max17042_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static ssize_t max17042_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t max17042_store_property(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

#define SEC_MAX17042_ATTR(_name) \
{ \
	.attr = { .name = #_name, .mode = S_IRUGO | (S_IWUSR | S_IWGRP) }, \
	.show = max17042_show_property, \
	.store = max17042_store_property, \
}

static struct device_attribute sec_max17042_attrs[] = {
	SEC_MAX17042_ATTR(fg_vfsoc),
	SEC_MAX17042_ATTR(fg_vfocv),
	SEC_MAX17042_ATTR(fg_filtercfg),
	SEC_MAX17042_ATTR(fg_cgain),
};

enum {
	MAX17042_VFSOC = 0,
	MAX17042_VFOCV,
	MAX17042_FILTERCFG,
	MAX17042_CGAIN,
};

static ssize_t max17042_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max17042_chip *chip = container_of(psy,
						  struct max17042_chip,
						  battery);
	int i = 0;
	const ptrdiff_t off = attr - sec_max17042_attrs;
	int val;

	switch (off) {
	case MAX17042_VFSOC:
		val = fg_read_register(VFSOC_REG);
		val = val >> 8; /* % */
		if (val >= 0)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		else
			i = -EINVAL;
		break;
	case MAX17042_VFOCV:
		val = fg_read_register(VFOCV_REG);
		val = ((val >> 3) * 625) / 1000; /* mV */
		if (val >= 0)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		else
			i = -EINVAL;
		break;
	case MAX17042_FILTERCFG:
		val = fg_read_register(FILTERCFG_REG);
		if (val >= 0)
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", val);
		else
			i = -EINVAL;
		break;
	case MAX17042_CGAIN:
		val = fg_read_register(CGAIN_REG);
		if (val >= 0)
			i += scnprintf(buf + i, PAGE_SIZE - i,
				"0x%x, PSOC: %d\n", val, chip->info.psoc);
		else
			i = -EINVAL;
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

static ssize_t max17042_store_property(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret = 0;
	const ptrdiff_t off = attr - sec_max17042_attrs;

	switch (off) {
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int max17042_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_max17042_attrs); i++) {
		rc = device_create_file(dev, &sec_max17042_attrs[i]);
		if (rc)
			goto max17042_attrs_failed;
	}
	goto succeed;

max17042_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_max17042_attrs[i]);
succeed:
	return rc;
}
#endif /* CONFIG_TARGET_LOCALE_KOR */

static int
fg_i2c_probe(struct i2c_client *client,  const struct i2c_device_id *id)
{
	struct max17042_chip *chip;
#if defined(CONFIG_TARGET_LOCALE_KOR)
	int ret = 0;
#endif /* CONFIG_TARGET_LOCALE_KOR */

	if (!client->dev.platform_data) {
		pr_err("%s: No platform data\n", __func__);
		return -EINVAL;
	}

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		pr_info("failed to allocate memory.\n");
		return -ENOMEM;
	}

	chip->client = client;
	chip->pdata = client->dev.platform_data;
	i2c_set_clientdata(client, chip);

	pr_info("%s: fuelgauge driver loading\n", __func__);

	max17042_chip_data = chip;
	/* rest of the initialisation goes here. */
	pr_info("Fuel gauge attach success!!!\n");

	fg_i2c_client = client;
	fg_set_battery_type();

	/* Init parameters to prevent wrong compensation. */
	chip->info.prev_fullcap = fg_read_register(FULLCAP_REG);
	chip->info.prev_vffcap = fg_read_register(FULLCAP_NOM_REG);
	/* Init FullCAP of first full charging. */
	chip->info.full_charged_cap = chip->info.prev_fullcap;

	chip->info.prev_vfsoc = fg_read_vfsoc();
	chip->info.prev_repsoc = fg_read_soc();
	chip->info.prev_remcap = fg_read_register(REMCAP_REP_REG);
	chip->info.prev_mixcap = fg_read_register(REMCAP_MIX_REG);
	chip->info.prev_vfocv = fg_read_register(VFOCV_REG);
	chip->info.prev_fullcapacity = chip->info.prev_fullcap;
	chip->info.prev_vfcapacity = chip->info.prev_vffcap;

	mutex_init(&chip->fg_lock);

	fg_alert_init();
	fg_read_model_data();
	fg_periodic_read();

#if defined(CONFIG_TARGET_LOCALE_KOR)
	chip->battery.name = "fuelgauge";
	chip->battery.type = POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property = max17042_get_property;
	chip->battery.properties = max17042_battery_props;
	chip->battery.num_properties = ARRAY_SIZE(max17042_battery_props);
	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		pr_err("%s : failed to regist fuel_gauge(ret = %d)\n",
			__func__, ret);
		kfree(chip);
		return ret;
	}

	max17042_create_attrs(chip->battery.dev);
#endif /* CONFIG_TARGET_LOCALE_KOR */
	return 0;
}

static const struct i2c_device_id fg_device_id[] = {
	{"fuelgauge", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fg_device_id);

static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "fuelgauge",
		.owner = THIS_MODULE,
	},
	.probe	= fg_i2c_probe,
	.remove	= fg_i2c_remove,
	.id_table	= fg_device_id,
};

static int __init max17042_init(void)
{
	return i2c_add_driver(&fg_i2c_driver);
}
subsys_initcall(max17042_init);

static void __exit max17042_exit(void)
{
	i2c_del_driver(&fg_i2c_driver);
}
module_exit(max17042_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17042 Fuel Gauge");
MODULE_LICENSE("GPL");

