/*
 * max17047_fuelgauge.c
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * based on max17040_battery.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>
#include <linux/battery/max17047_fuelgauge_c.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/rtc.h>
#if defined(CONFIG_TARGET_LOCALE_KOR)
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#endif

#define CONFIG_FG_SYSFS

#if defined(CONFIG_S3C_ADC)
#include <plat/adc.h>
#endif

struct platform_device *pdev_fuelgauge_adc;
/* Battery parameters depends on the type */
static int battery_type = UNKNOWN_TYPE;

/* MAX17047 Registers. */
#define MAX17047_REG_STATUS			0x00
#define MAX17047_REG_VALRT_TH		0x01
#define MAX17047_REG_TALRT_TH		0x02
#define MAX17047_REG_SALRT_TH		0x03
#define MAX17047_REG_REMCAP_REP		0x05
#define MAX17047_SOCREP				0x06
#define MAX17047_REG_VCELL			0x09
#define MAX17047_REG_SOCMIX			0x0D
#define MAX17047_REG_SOCAV			0x0E
#define MAX17047_REG_REMCAP_MIX		0x0F
#define MAX17047_REG_FULLCAP		0x10
#define MAX17047_REG_TEMPERATURE	0x08
#define MAX17047_REG_CYCLES			0x17
#define MAX17047_REG_DESIGNCAP_REG	0x18
#define MAX17047_REG_AVGVCELL		0x19
#define MAX17047_REG_CONFIG			0x1D
#define MAX17047_REG_VERSION		0x21
#define MAX17047_REG_FULLCAP_NOM 	0x23
#define MAX17047_REG_LEARNCFG		0x28
#define MAX17047_REG_FILTERCFG		0x29
#define MAX17047_REG_MISCCFG		0x2B
#define MAX17047_REG_CGAIN			0x2E
#define MAX17047_REG_RCOMP			0x38
#define MAX17047_REG_TEMPCO			0x39
#define MAX17047_REG_VFOCV			0xFB
#define MAX17047_REG_SOC_VF			0xFF
#define MAX17047_REG_FULLCAP		0x10
#define MAX17047_REG_FULLCAPNOM		0x23
#define MAX17047_REG_CURRENT		0x0A
#define MAX17047_REG_AVG_CURRENT	0x0B
#define MAX17047_REG_DQACC			0x45
#define MAX17047_REG_DPACC			0x46
#define MAX17047_REG_VFSOC			0xFF

#define FULL_SOC_DEFAULT		9900
#define FULL_SOC_LOW			9700
#define FULL_SOC_HIGH			10000
#define KEEP_FULL_SOC			100 // /* 1.0% */

/* Polling work */
#undef	DEBUG_FUELGAUGE_POLLING
#define MAX17047_POLLING_INTERVAL	10000

struct max17047_fuelgauge_data {
	struct i2c_client		*client;
	struct max17047_platform_data	*pdata;

	struct power_supply		fuelgauge;

	/* workqueue */
	struct delayed_work		update_work;
#ifdef DEBUG_FUELGAUGE_POLLING
	struct delayed_work		polling_work;
#endif
	struct delayed_work		full_comp_work;

	/* mutex */
	struct mutex			irq_lock;

	/* wakelock */
	struct wake_lock		update_wake_lock;

	unsigned int			irq;

	unsigned int			vcell;
	unsigned int			avgvcell;
	unsigned int			vfocv;
	unsigned int			soc;
	unsigned int			rawsoc;
	unsigned int			temperature;

/*#if defined(CONFIG_FUELGAUGE_MAX17047_COULOMB_COUNTING)*/
	u32						previous_fullcap;
	u32						previous_vffullcap;

	/* low battery comp */
	int low_batt_comp_cnt[LOW_BATT_COMP_RANGE_NUM][LOW_BATT_COMP_LEVEL_NUM];

	/* low battery boot */
	int low_batt_boot_flag;
	bool is_low_batt_alarm;

	/* miscellaneous */
	unsigned long			fullcap_check_interval;
	int 					full_check_flag;
	bool					is_first_check;
/*#endif*/

	/* adjust full soc */
	int				full_soc;

#if defined(CONFIG_MACH_GC1)
	int				prev_status;
#endif

#if defined(CONFIG_MACH_KONA)
	unsigned int is_comp_3;
	unsigned int is_comp_1;
#endif


#ifdef USE_TRIM_ERROR_DETECTION
	/* trim error state */
	bool				trim_err;
#endif

#ifdef CONFIG_DEBUG_FS
	struct dentry			*fg_debugfs_dir;
#endif

#ifdef CONFIG_HIBERNATION
	u8 *reg_dump;
#endif
#if defined(CONFIG_S3C_ADC)
	struct s3c_adc_client			*adc_client;
#endif
};

static struct battery_data_t fg_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x2530,
		.low_battery_comp_voltage = 3500,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000, 0,		0}, 	/* dummy for top limit */
			{-1250, 0,		3320},
			{-750, 97,		3451},
			{-100, 96,		3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0, 	0}, 	/* dummy for top limit */
		},
		.type_str = "SDI",
	},
	/* BYD battery data */
	{
		.Capacity = 0x2326,
		.low_battery_comp_voltage = 3500,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000, 0,		0}, 	/* dummy for top limit */
			{-1250, 0,		3320},
			{-750, 97,		3451},
			{-100, 96,		3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0, 	0}, 	/* dummy for top limit */
		},
		.type_str = "BYD",
	},
	/* LIS battery data */
	{
		.Capacity = 0x2414,
		.low_battery_comp_voltage = 3500,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000, 0,		0}, 	/* dummy for top limit */
			{-1250, 0,		3320},
			{-750, 97,		3451},
			{-100, 96,		3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0, 	0}, 	/* dummy for top limit */
		},
		.type_str = "LIS",
	}
};

#define MAX17047_CAPACITY	0x2530

static int max17047_i2c_read(struct i2c_client *client, int reg, u8 *buf)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
	if (ret < 0)
		pr_err("%s: err %d, reg: 0x%02x\n", __func__, ret, reg);

	return ret;
}

static int max17047_i2c_write(struct i2c_client *client, int reg, u8 *buf)
{
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);
	if (ret < 0)
		pr_err("%s: err %d, reg: 0x%02x, data: 0x%x%x\n", __func__,
				ret, reg, buf[0], buf[1]);

	return ret;
}

static int fg_read_register(struct i2c_client *client,
				u8 addr)
{
	u8 data[2];

	if (max17047_i2c_read(client, addr, data) < 0) {
		dev_err(&client->dev, "%s: Failed to read addr(0x%x)\n",
			__func__, addr);
		return -1;
	}

	return (data[1] << 8) | data[0];
}

static int fg_write_register(struct i2c_client *client,
				u8 addr, u16 w_data)
{
	u8 data[2];

	data[0] = w_data & 0xFF;
	data[1] = w_data >> 8;

	pr_info("%s: write addr(0x%x) data: (0x%x%x)\n", __func__, addr, data[0] , data[1]);

	if (max17047_i2c_write(client, addr, data) < 0) {
		dev_err(&client->dev, "%s: Failed to write addr(0x%x)\n",
			__func__, addr);
		return -1;
	}

	return 0;
}

static void max17047_test_read(struct max17047_fuelgauge_data *fg_data)
{
	int reg;
	u8 data[2];
	int i;
	u8 buf[673];

	struct timespec ts;
	struct rtc_time tm;
	pr_info("%s\n", __func__);

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

	pr_info("%s: %d/%d/%d %02d:%02d\n", __func__,
					tm.tm_mday,
					tm.tm_mon + 1,
					tm.tm_year + 1900,
					tm.tm_hour,
					tm.tm_min);

	i = 0;
	for (reg = 0; reg < 0x50; reg++) {
		if (!(reg & 0xf))
			i += sprintf(buf + i, "\n%02x| ", reg);
		max17047_i2c_read(fg_data->client, reg, data);
		i += sprintf(buf + i, "%02x%02x ", data[1], data[0]);
	}
	for (reg = 0xe0; reg < 0x100; reg++) {
		if (!(reg & 0xf))
			i += sprintf(buf + i, "\n%02x| ", reg);
		max17047_i2c_read(fg_data->client, reg, data);
		i += sprintf(buf + i, "%02x%02x ", data[1], data[0]);
	}

	pr_info("    0    1    2    3    4    5    6    7");
	pr_cont("    8    9    a    b    c    d    e    f");
	pr_cont("%s\n", buf);
}

static int fg_check_battery_present(struct i2c_client *client)
{
	u8 status_data[2];
	int ret = 1;

	/* 1. Check Bst bit */
	if (max17047_i2c_read(client, MAX17047_REG_STATUS, status_data) < 0) {
		dev_err(&client->dev,
			"%s: Failed to read STATUS_REG\n", __func__);
		return 0;
	}

	if (status_data[0] & (0x1 << 3)) {
		dev_info(&client->dev,
			"%s: addr(0x01), data(0x%04x)\n", __func__,
			(status_data[1]<<8) | status_data[0]);
		dev_info(&client->dev, "%s: battery is absent!!\n", __func__);
		ret = 0;
	}

	return ret;
}

static int max17047_get_temperature(struct i2c_client *client)
{
#if defined(CONFIG_MACH_KONA)
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2] = {0x00, 0x00};
	int temper = 0;

	if (fg_check_battery_present(client)) {
		if (max17047_i2c_read(client, MAX17047_REG_TEMPERATURE, data) < 0) {
			dev_err(&client->dev,
				"%s: Failed to read TEMPERATURE_REG\n",
				__func__);
			return -1;
		}

		if (data[1]&(0x1 << 7)) {
			temper = ((~(data[1]))&0xFF)+1;
			temper *= (-1000);
			temper -= ((~((int)data[0]))+1) * 39 / 10;
		} else {
			temper = data[1] & 0x7f;
			temper *= 1000;
			temper += data[0] * 39 / 10;
		}
	} else
		temper = 20000;

	dev_info(&client->dev, "%s: TEMPERATURE(%d), data(0x%04x)\n",
		__func__, temper/100, (data[1]<<8) | data[0]);

	return temper/100;
#else
	return 300;
#endif
}

/* max17047_get_XXX(); Return current value and update data value */
/* mV */
static int max17047_get_vfocv(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2];
	int ret;
	u32 vfocv;
	pr_debug("%s\n", __func__);

	ret = max17047_i2c_read(client, MAX17047_REG_VFOCV, data);
	if (ret < 0)
		return ret;

	vfocv = ((data[0] >> 3) + (data[1] << 5)) * 625;
	fg_data->vfocv = vfocv / 1000;	

	pr_debug("%s: VFOCV(0x%02x%02x, %d)\n", __func__,
		 data[1], data[0], fg_data->vfocv);
	return fg_data->vfocv;
}

/* mV */
static int max17047_get_vcell(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2];
	int ret;
	u32 vcell;
	pr_debug("%s\n", __func__);

	ret = max17047_i2c_read(client, MAX17047_REG_VCELL, data);
	if (ret < 0)
		return ret;

	vcell = ((data[0] >> 3) + (data[1] << 5)) * 625;
	fg_data->vcell = vcell / 1000;

	pr_debug("%s: VCELL(0x%02x%02x, %d)\n", __func__,
		 data[1], data[0], fg_data->vcell);
	return fg_data->vcell;
}

static int max17047_get_avgvcell(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2];
	int ret;
	u32 avgvcell;
	pr_debug("%s\n", __func__);

	ret = max17047_i2c_read(client, MAX17047_REG_AVGVCELL, data);
	if (ret < 0)
		return ret;

	avgvcell = ((data[0] >> 3) + (data[1] << 5)) * 625;
	fg_data->avgvcell = avgvcell / 1000;

	pr_debug("%s: AVGVCELL(0x%02x%02x, %d)\n", __func__,
		 data[1], data[0], fg_data->avgvcell);
	return fg_data->avgvcell;
}

static int max17047_get_rawsoc(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2];
	int soc;

	if (max17047_i2c_read(client, MAX17047_SOCREP, data) < 0) {
		dev_err(&client->dev, "%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	soc = (data[1] * 100) + (data[0] * 100 / 256);

	dev_dbg(&client->dev, "%s: raw capacity (0.01%%) (%d)\n",
		__func__, soc);

	dev_dbg(&client->dev, "%s: raw capacity (%d), data(0x%04x)\n",
			__func__, soc, (data[1]<<8) | data[0]);

	fg_data->rawsoc = min(soc, 10000);
	return fg_data->rawsoc;
}

/* soc should be 1% unit */
static int max17047_get_soc(struct i2c_client *client)
{
	u8 data[2];
	int soc;

	if (max17047_i2c_read(client, MAX17047_SOCREP, data) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 100;

	pr_info("%s: capacity (%d), data(0x%04x)\n", __func__, soc, (data[1]<<8) | data[0]);

	return min(soc, 100);
}

static int fg_reset_capacity_by_jig_connection(struct i2c_client *client)
{
	pr_info("%s: DesignCap = Capacity - 1 (Jig Connection)\n", __func__);

	return fg_write_register(client, MAX17047_REG_DESIGNCAP_REG,
		fg_battery_data[battery_type].Capacity - 1);
}

#if defined(CONFIG_MACH_KONA)
/* For using JIG detach */
struct i2c_client *fg_client;

void fg_reset_capacity_by_jig_connection_ex(void)
{

	pr_info("%s: DesignCap = Capacity - 1 (Jig Connection)\n", __func__);
	printk("[BAT] %s call!!\n", __func__);


	fg_write_register(fg_client, MAX17047_REG_DESIGNCAP_REG,
		fg_battery_data[battery_type].Capacity - 1);
}
EXPORT_SYMBOL(fg_reset_capacity_by_jig_connection_ex);
#endif

static void fg_periodic_read(struct i2c_client *client)
{
	u8 reg;
	int i;
	int data[0x10];
	char *str = NULL;

	str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
	if (!str)
		return;

	for (i = 0; i < 16; i++) {
		for (reg = 0; reg < 0x10; reg++)
			data[reg] = fg_read_register(client, reg + i * 0x10);

		sprintf(str+strlen(str),
			"%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,",
			data[0x00], data[0x01], data[0x02], data[0x03],
			data[0x04], data[0x05], data[0x06], data[0x07]);
		sprintf(str+strlen(str),
			"%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,",
			data[0x08], data[0x09], data[0x0a], data[0x0b],
			data[0x0c], data[0x0d], data[0x0e], data[0x0f]);
		if (i == 4)
			i = 13;
	}

	dev_info(&client->dev, "maxim check %s", str);

	kfree(str);
}

static int fg_read_current(struct i2c_client *client)
{
	u8 data1[2], data2[2];
	u32 temp, sign;
	s32 i_current;
	s32 avg_current;

	if (max17047_i2c_read(client, MAX17047_REG_CURRENT, data1) < 0) {
		pr_err("%s: Failed to read CURRENT\n",
			__func__);
		return -1;
	}

	if (max17047_i2c_read(client, MAX17047_REG_AVG_CURRENT, data2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n",
			__func__);
		return -1;
	}

	temp = ((data1[1]<<8) | data1[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	/* 1.5625uV/0.01Ohm(Rsense) = 156.25uA */
	i_current = temp * 15625 / 100000;
	if (sign)
		i_current *= -1;

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	/* 1.5625uV/0.01Ohm(Rsense) = 156.25uA */
	avg_current = temp * 15625 / 100000;
	if (sign)
		avg_current *= -1;

	pr_info("%s: CURRENT(%dmA), AVG_CURRENT(%dmA)\n",
		__func__, i_current, avg_current);

	fg_periodic_read(client);

	return i_current;
}

static int fg_read_avg_current(struct i2c_client *client)
{
	u8	data2[2];
	u32 temp, sign;
	s32 avg_current;

	if (max17047_i2c_read(client, MAX17047_REG_AVG_CURRENT, data2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n",
			__func__);
		return -1;
	}

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	/* 1.5625uV/0.01Ohm(Rsense) = 156.25uA */
	avg_current = temp * 15625 / 100000;

	if (sign)
		avg_current *= -1;

	return avg_current;
}

/* soc should be 0.1% unit */
static int fg_read_vfsoc(struct i2c_client *client)
{
	u8 data[2];
	int soc;

	if (max17047_i2c_read(client, MAX17047_REG_VFSOC, data) < 0) {
		pr_err("%s: Failed to read VFSOC\n", __func__);
		return -1;
	}

	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 100;

	return min(soc, 100);
}

static int fg_check_status_reg(struct i2c_client *client)
{
	u8 status_data[2];
	int ret = 0;

	/* 1. Check Smn was generatedread */
	if (max17047_i2c_read(client, MAX17047_REG_STATUS, status_data) < 0) {
		dev_err(&client->dev, "%s: Failed to read STATUS_REG\n",
			__func__);
		return -1;
	}
	dev_info(&client->dev, "%s: addr(0x00), data(0x%04x)\n", __func__,
		(status_data[1]<<8) | status_data[0]);

	if (status_data[1] & (0x1 << 2))
		ret = 1;

	/* 2. clear Status reg */
	status_data[1] = 0;
	if (max17047_i2c_write(client, MAX17047_REG_STATUS, status_data) < 0) {
		dev_info(&client->dev, "%s: Failed to write STATUS_REG\n",
			__func__);
		return -1;
	}

	return ret;
}


int get_fuelgauge_value(struct i2c_client *client, int data)
{
	int ret = 0;

	switch (data) {
	case FG_LEVEL:
		/*ret = fg_read_soc(client);*/
		ret = max17047_get_soc(client);
		break;

	case FG_TEMPERATURE:
		/*ret = fg_read_temp(client);*/
		break;

	case FG_VOLTAGE:
		ret = max17047_get_vcell(client);
		break;

	case FG_CURRENT:
		ret = fg_read_current(client);
		break;

	case FG_CURRENT_AVG:
		ret = fg_read_avg_current(client);
		break;

	case FG_CHECK_STATUS:
		ret = fg_check_status_reg(client);
		break;

	case FG_RAW_SOC:
		ret = max17047_get_rawsoc(client);
		break;

	case FG_VF_SOC:
		ret = fg_read_vfsoc(client);
		break;

	case FG_AV_SOC:
		/*ret = fg_read_avsoc(client);*/
		break;

	case FG_FULLCAP:
		/*ret = fg_read_fullcap(client);*/
		break;

	case FG_MIXCAP:
		/*ret = fg_read_mixcap(client);*/
		break;

	case FG_AVCAP:
		/*ret = fg_read_avcap(client);*/
		break;

	case FG_REPCAP:
		/*ret = fg_read_repcap(client);*/
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}


void fg_check_vf_fullcap_range(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);

	static int new_vffullcap;
	bool is_vffullcap_changed = true;

	if (is_jig_attached == JIG_ON)
		fg_reset_capacity_by_jig_connection(client);

	new_vffullcap = fg_read_register(client, MAX17047_REG_FULLCAP_NOM);
	if (new_vffullcap < 0)
		new_vffullcap = fg_battery_data[battery_type].Capacity;

	/* compare with initial capacity */
	if (new_vffullcap >
		(fg_battery_data[battery_type].Capacity * 110 / 100)) {
		pr_info("%s: [Case 1] capacity = 0x%04x, NewVfFullCap = 0x%04x\n",
			__func__, fg_battery_data[battery_type].Capacity,
			new_vffullcap);

		new_vffullcap =
			(fg_battery_data[battery_type].Capacity * 110) / 100;

		fg_write_register(client, MAX17047_REG_DQACC,
			(u16)(new_vffullcap / 4));
		fg_write_register(client, MAX17047_REG_DPACC, (u16)0x3200);
	} else if (new_vffullcap <
		(fg_battery_data[battery_type].Capacity * 50 / 100)) {
		pr_info("%s: [Case 5] capacity = 0x%04x, NewVfFullCap = 0x%04x\n",
			__func__, fg_battery_data[battery_type].Capacity, new_vffullcap);

		new_vffullcap =
			(fg_battery_data[battery_type].Capacity * 50) / 100;

		fg_write_register(client, MAX17047_REG_DQACC,
			(u16)(new_vffullcap / 4));
		fg_write_register(client, MAX17047_REG_DPACC, (u16)0x3200);
	} else {
	/* compare with previous capacity */
		if (new_vffullcap >
			(fg_data->previous_vffullcap * 110 / 100)) {
			pr_info("%s: [Case 2] previous_vffullcap = 0x%04x, NewVfFullCap = 0x%04x\n",
				__func__, fg_data->previous_vffullcap,
				new_vffullcap);

			new_vffullcap =
				(fg_data->previous_vffullcap * 110) /
				100;

			fg_write_register(client, MAX17047_REG_DQACC,
				(u16)(new_vffullcap / 4));
			fg_write_register(client, MAX17047_REG_DPACC, (u16)0x3200);
		} else if (new_vffullcap <
			(fg_data->previous_vffullcap * 90 / 100)) {
			pr_info("%s: [Case 3] previous_vffullcap = 0x%04x, NewVfFullCap = 0x%04x\n",
				__func__, fg_data->previous_vffullcap, new_vffullcap);

			new_vffullcap =
				(fg_data->previous_vffullcap * 90) / 100;

			fg_write_register(client, MAX17047_REG_DQACC,
				(u16)(new_vffullcap / 4));
			fg_write_register(client, MAX17047_REG_DPACC, (u16)0x3200);
		} else {
			pr_info("%s: [Case 4] previous_vffullcap = 0x%04x, NewVfFullCap = 0x%04x\n",
				__func__, fg_data->previous_vffullcap,
				new_vffullcap);
			is_vffullcap_changed = false;
		}
	}

	/* delay for register setting (dQacc, dPacc) */
	if (is_vffullcap_changed)
		msleep(300);

	fg_data->previous_vffullcap =
		fg_read_register(client, MAX17047_REG_FULLCAP_NOM);

	if (is_vffullcap_changed)
		pr_info("%s : VfFullCap(0x%04x), dQacc(0x%04x), dPacc(0x%04x)\n",
			__func__,
			fg_read_register(client, MAX17047_REG_FULLCAP_NOM),
			fg_read_register(client, MAX17047_REG_DQACC),
			fg_read_register(client, MAX17047_REG_DPACC));

}

void fg_set_full_charged(struct i2c_client *client)
{
	pr_info("[FG_Set_Full] (B) FullCAP(%d), RemCAP(%d)\n",
		(fg_read_register(client, MAX17047_REG_FULLCAP)/2),
		(fg_read_register(client, MAX17047_REG_REMCAP_REP)/2));

	fg_write_register(client, MAX17047_REG_FULLCAP,
		(u16)fg_read_register(client, MAX17047_REG_REMCAP_REP));

	pr_info("[FG_Set_Full] (A) FullCAP(%d), RemCAP(%d)\n",
		(fg_read_register(client, MAX17047_REG_FULLCAP)/2),
		(fg_read_register(client, MAX17047_REG_REMCAP_REP)/2));
}

static void add_low_batt_comp_cnt(struct i2c_client *client,
				int range, int level)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);
	int i;
	int j;

	/* Increase the requested count value, and reset others. */
	fg_data->low_batt_comp_cnt[range-1][level/2]++;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (i == range-1 && j == level/2)
				continue;
			else
				fg_data->low_batt_comp_cnt[i][j] = 0;
		}
	}
}


static int get_low_batt_threshold(struct i2c_client *client,
				int range, int nCurrent, int level)
{
	int ret = 0;

	ret = fg_battery_data[battery_type].low_battery_table[range][OFFSET] +
		((nCurrent *
		fg_battery_data[battery_type].low_battery_table[range][SLOPE]) /
		1000);

	return ret;
}

void reset_low_batt_comp_cnt(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);

	memset(fg_data->low_batt_comp_cnt, 0,
		sizeof(fg_data->low_batt_comp_cnt));
}

static void display_low_batt_comp_cnt(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);

	pr_info("Check Array(%s): [%d, %d], [%d, %d], ",
			fg_battery_data[battery_type].type_str,
			fg_data->low_batt_comp_cnt[0][0],
			fg_data->low_batt_comp_cnt[0][1],
			fg_data->low_batt_comp_cnt[1][0],
			fg_data->low_batt_comp_cnt[1][1]);
	pr_info("[%d, %d], [%d, %d], [%d, %d]\n",
			fg_data->low_batt_comp_cnt[2][0],
			fg_data->low_batt_comp_cnt[2][1],
			fg_data->low_batt_comp_cnt[3][0],
			fg_data->low_batt_comp_cnt[3][1],
			fg_data->low_batt_comp_cnt[4][0],
			fg_data->low_batt_comp_cnt[4][1]);
}


static int check_low_batt_comp_condition(
				struct i2c_client *client, int *nLevel)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);
	int i;
	int j;
	int ret = 0;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (fg_data->low_batt_comp_cnt[i][j] >=
				MAX_LOW_BATT_CHECK_CNT) {
				display_low_batt_comp_cnt(client);
				ret = 1;
				*nLevel = j*2 + 1;
				break;
			}
		}
	}

	return ret;
}

void fg_low_batt_compensation(struct i2c_client *client, u32 level)
{
#if defined(CONFIG_MACH_KONA)
	struct max17047_fuelgauge_data *fg_data =
					i2c_get_clientdata(client);
#endif
	int read_val;
	u32 temp;

	pr_info("%s: Adjust SOCrep to %d!!\n",
		__func__, level);

	read_val = fg_read_register(client, MAX17047_REG_FULLCAP);
	if (read_val < 0)
		return;
	
#if defined(CONFIG_MACH_KONA)
	if (level > 2) { /* 3% compensation */
		/* RemCapREP (05h) = FullCap(10h) x 0.0301 */
		temp = read_val * (level*100 + 1) / 10000;

		/* Display conpensation 3% value for debug screen */
		fg_data->is_comp_3 = 1;

	} else {				/* 1% compensation */
		/* RemCapREP (05h) = FullCap(10h) x 0.0090 */
		temp = read_val * (level*90) / 10000;

		/* Display conpensation 1% value for debug screen */
		fg_data->is_comp_1 = 1;
	}
#else
	if (level > 2)	/* 3% compensation */
		/* RemCapREP (05h) = FullCap(10h) x 0.0301 */
		temp = read_val * (level*100 + 1) / 10000;
	else				/* 1% compensation */
		/* RemCapREP (05h) = FullCap(10h) x 0.0090 */
		temp = read_val * (level*90) / 10000;
#endif

	pr_info("%s: read_val=(%d), level=(%d) temp=(0x%04x)\n", __func__,
			read_val, level, temp);

	
	fg_write_register(client, MAX17047_REG_REMCAP_REP, (u16)temp);
}

void prevent_early_poweroff(struct i2c_client *client,
	int vcell, int *fg_soc)
{
	int soc = 0;
	int read_val;

	soc = get_fuelgauge_value(client, FG_LEVEL);

	if (soc > POWER_OFF_SOC_HIGH_MARGIN)
		return;

	pr_info("%s: soc=%d%%, vcell=%d\n", __func__,
		soc, vcell);

	if (vcell > POWER_OFF_VOLTAGE_HIGH_MARGIN) {
		read_val = fg_read_register(client, MAX17047_REG_FULLCAP);
		/* FullCAP * 0.013 */
		fg_write_register(client, MAX17047_REG_REMCAP_REP,
		(u16)(read_val * 13 / 1000));
		msleep(200);
		*fg_soc = max17047_get_soc(client);
		dev_info(&client->dev, "%s : new soc=%d, vcell=%d\n",
			__func__, *fg_soc, vcell);
	}
}

int low_batt_compensation(struct i2c_client *client,
		int fg_soc, int fg_vcell, int fg_current)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);
	int fg_avg_current = 0;
	int fg_min_current = 0;
	int new_level = 0;
	int i, table_size;

	pr_info("%s: fg_soc=(%d), fg_vcell=(%d) fg_current=(%d)\n", __func__,
			fg_soc, fg_vcell, fg_current);

	/* Not charging, Under low battery comp voltage */
	if (fg_vcell <= fg_battery_data[battery_type].low_battery_comp_voltage) {
		fg_avg_current = fg_read_avg_current(client);
		fg_min_current = min(fg_avg_current, fg_current);

		table_size =
			sizeof(fg_battery_data[battery_type].low_battery_table) /
			(sizeof(s16)*TABLE_MAX);

		for (i = 1; i < CURRENT_RANGE_MAX_NUM; i++) {
			if ((fg_min_current >= fg_battery_data[battery_type].low_battery_table[i-1][RANGE]) &&
				(fg_min_current < fg_battery_data[battery_type].low_battery_table[i][RANGE])) {
				if (fg_soc >= 2 && fg_vcell <
					get_low_batt_threshold(client,
					i, fg_min_current, 1)) {
					add_low_batt_comp_cnt(
						client, i, 1);
				} else {
					reset_low_batt_comp_cnt(client);
				}
			}
		}

		if (check_low_batt_comp_condition(client, &new_level)) {
			fg_low_batt_compensation(client, new_level);
			reset_low_batt_comp_cnt(client);

			/* Do not update soc right after
			 * low battery compensation
			 * to prevent from powering-off suddenly
			 */
			pr_info("%s: SOC is set to %d by low compensation!!\n",
				__func__, max17047_get_soc(client));
		}
	}

	/* Prevent power off over 3500mV */
	prevent_early_poweroff(client, fg_vcell, &fg_soc);

	return fg_soc;
}

int fg_adjust_capacity(struct i2c_client *client)
{
	u8 data[2];

	data[0] = 0;
	data[1] = 0;

	/* 1. Write RemCapREP(05h)=0; */
	if (max17047_i2c_write(client, MAX17047_REG_REMCAP_REP, data) < 0) {
		pr_err("%s: Failed to write RemCap_REP\n", __func__);
		return -1;
	}
	msleep(200);

	pr_info("%s: After adjust - RepSOC(%d)\n", __func__, max17047_get_soc(client));

	return 0;
}

static bool is_booted_in_low_battery(struct i2c_client *client)
{
	int fg_vcell = get_fuelgauge_value(client, FG_VOLTAGE);
	int fg_current = get_fuelgauge_value(client, FG_CURRENT);
	int threshold = 0;

	threshold = 3300 + ((fg_current * 17) / 100);

	if (fg_vcell <= threshold)
		return true;
	else
		return false;
}

static bool fuelgauge_recovery_handler(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);
	int current_soc;
	int avsoc;
	int temperature;

	if (fg_data->soc > LOW_BATTERY_SOC_REDUCE_UNIT) {
		pr_err("%s: Reduce the Reported SOC by 1%%\n",
			__func__);
		current_soc =
			get_fuelgauge_value(client, FG_LEVEL);

		if (current_soc) {
			pr_info("%s: Returning to Normal discharge path\n",
				__func__);
			pr_info("%s: Actual SOC(%d) non-zero\n",
				__func__, current_soc);
			fg_data->is_low_batt_alarm = false;
		} else {
			temperature =
				get_fuelgauge_value(client, FG_TEMPERATURE);
			avsoc =
				get_fuelgauge_value(client, FG_AV_SOC);

			if ((fg_data->soc > avsoc) ||
				(temperature < 0)) {
				fg_data->soc -=
					LOW_BATTERY_SOC_REDUCE_UNIT;
				pr_err("%s: New Reduced RepSOC (%d)\n",
					__func__, fg_data->soc);
			} else
				pr_info("%s: Waiting for recovery (AvSOC:%d)\n",
					__func__, avsoc);
		}
	}

	return fg_data->is_low_batt_alarm;
}


static int get_fuelgauge_soc(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
				i2c_get_clientdata(client);
	struct power_supply *battery_psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	int fg_soc;
	int fg_vfsoc;
	int fg_soc_raw;
	int fg_vcell;
	int fg_current;
	int avg_current;
	ktime_t current_time;
	struct timespec ts;
	int fullcap_check_interval;
	int cable_type;

	if (fg_data->is_low_batt_alarm) {
		if (fuelgauge_recovery_handler(client)) {
			fg_soc = fg_data->soc;
			goto return_soc;
		}
	}

	current_time = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(current_time);

	/* check fullcap range */
	fullcap_check_interval =
		(ts.tv_sec - fg_data->fullcap_check_interval);
	if (fullcap_check_interval >
		VFFULLCAP_CHECK_INTERVAL) {
		dev_info(&client->dev,
			"%s: check fullcap range (interval:%d)\n",
			__func__, fullcap_check_interval);
		fg_check_vf_fullcap_range(client);
		fg_data->fullcap_check_interval = ts.tv_sec;
	}

	fg_soc = get_fuelgauge_value(client, FG_LEVEL);
	if (fg_soc < 0) {
		pr_info("Can't read soc!!!");
		fg_soc = fg_data->soc;
	}

	fg_soc_raw = get_fuelgauge_value(client, FG_RAW_SOC);
	if (fg_soc_raw < 0) {
		pr_info("Can't read rawsoc!!!");
		fg_soc = fg_data->rawsoc;
	}

	if (!battery_psy) {
		pr_info("%s : battery driver didn't load yet.\n", __func__);
		goto adjust_soc;
	}

	battery_psy->get_property(battery_psy,
				POWER_SUPPLY_PROP_ONLINE,
				&value);

	cable_type = value.intval;

	if (fg_data->low_batt_boot_flag) {
		fg_soc_raw = 0;

		if (cable_type != POWER_SUPPLY_TYPE_BATTERY &&
			!is_booted_in_low_battery(client)) {
			fg_adjust_capacity(client);
			fg_data->low_batt_boot_flag = 0;
		}

		if (cable_type == POWER_SUPPLY_TYPE_BATTERY)
			fg_data->low_batt_boot_flag = 0;
	}

	fg_vcell = get_fuelgauge_value(client, FG_VOLTAGE);
	fg_current = get_fuelgauge_value(client, FG_CURRENT);
	avg_current = get_fuelgauge_value(client, FG_CURRENT_AVG);
	fg_vfsoc = get_fuelgauge_value(client, FG_VF_SOC);


	battery_psy->get_property(battery_psy,
				POWER_SUPPLY_PROP_STATUS,
				&value);

	/* Algorithm for reducing time to fully charged (from MAXIM) */
	if (value.intval != POWER_SUPPLY_STATUS_DISCHARGING &&
		value.intval != POWER_SUPPLY_STATUS_FULL &&
		cable_type != POWER_SUPPLY_TYPE_USB &&
		/* Skip when first check after boot up */
		!fg_data->is_first_check &&
		(fg_vfsoc > VFSOC_FOR_FULLCAP_LEARNING &&
		(fg_current > LOW_CURRENT_FOR_FULLCAP_LEARNING &&
		fg_current < HIGH_CURRENT_FOR_FULLCAP_LEARNING) &&
		(avg_current > LOW_AVGCURRENT_FOR_FULLCAP_LEARNING &&
		avg_current < HIGH_AVGCURRENT_FOR_FULLCAP_LEARNING))) {

		if (fg_data->full_check_flag == 2) {
			pr_info("%s: force fully charged SOC !! (%d)",
				__func__, fg_data->full_check_flag);
			fg_set_full_charged(client);
			fg_soc = get_fuelgauge_value(client, FG_LEVEL);
		} else if (fg_data->full_check_flag < 2)
			pr_info("%s: full_check_flag (%d)",
				__func__, fg_data->full_check_flag);

		/* prevent overflow */
		if (fg_data->full_check_flag++ > 10000)
			fg_data->full_check_flag = 3;
	} else
		fg_data->full_check_flag = 0;

	/*	Checks vcell level and tries to compensate SOC if needed.*/
	/*	If jig cable is connected, then skip low batt compensation check. */
	if (is_jig_attached != JIG_ON &&
		value.intval == POWER_SUPPLY_STATUS_DISCHARGING) {
		fg_soc = low_batt_compensation(
			client, fg_soc, fg_vcell, fg_current);
		fg_soc_raw = get_fuelgauge_value(client, FG_RAW_SOC);
	}

	if (fg_data->is_first_check)
		fg_data->is_first_check = false;

adjust_soc:
#if defined(CONFIG_MACH_KONA)
  	if (fg_data->full_soc <= 0)
		fg_data->full_soc = FULL_SOC_DEFAULT;
	
	fg_soc =(min((fg_soc_raw * 100 / (fg_data->full_soc)), 100));	
#else
	fg_soc = min((fg_soc_raw / 100), 100);
#endif

	fg_data->soc = fg_soc;

return_soc:
	pr_info("%s: soc(%d), low_batt_alarm(%d)\n",
		__func__, fg_data->soc,
		fg_data->is_low_batt_alarm);

	return fg_soc;
}

static void max17047_adjust_fullsoc(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data =
		 i2c_get_clientdata(client);
	int prev_full_soc = fg_data->full_soc;
	int raw_soc = max17047_get_rawsoc(fg_data->client);
	int keep_soc = 0;

	if (raw_soc < 0) {
		pr_err("%s : fg data error!(%d)\n", __func__, raw_soc);
		fg_data->full_soc = FULL_SOC_DEFAULT;
		return;
	}

	if (raw_soc < FULL_SOC_LOW)
		fg_data->full_soc = FULL_SOC_LOW;
	else if (raw_soc > FULL_SOC_HIGH) {
		keep_soc = FULL_SOC_HIGH / 100;
		fg_data->full_soc = (FULL_SOC_HIGH - keep_soc);
	} else {
		keep_soc = ((raw_soc * KEEP_FULL_SOC) / 10000);
		if (raw_soc > (FULL_SOC_LOW + keep_soc))
			fg_data->full_soc = raw_soc - keep_soc;
		else
			fg_data->full_soc = FULL_SOC_LOW;
	}

	if (prev_full_soc != fg_data->full_soc)
		pr_info("%s : full_soc(%d->%d), rsoc(%d), keep(%d)\n", __func__,
			prev_full_soc, fg_data->full_soc, raw_soc, keep_soc);
}

/* SOC% alert, disabled(0xFF00) */
static void max17047_set_salrt(struct max17047_fuelgauge_data *fg_data,
								u8 min, u8 max)
{
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_info("%s: min(%d%%), max(%d%%)\n", __func__, min, max);

	i2c_data[1] = max;
	i2c_data[0] = min;
	max17047_i2c_write(client, MAX17047_REG_SALRT_TH, i2c_data);

	max17047_i2c_read(client, MAX17047_REG_SALRT_TH, i2c_data);
	if ((i2c_data[0] != min) || (i2c_data[1] != max))
		pr_err("%s: SALRT_TH is not valid (0x%02d%02d ? 0x%02d%02d)\n",
			__func__, i2c_data[1], i2c_data[0], max, min);
}

/* Temperature alert, disabled(0x7F80) */
static void max17047_set_talrt(struct max17047_fuelgauge_data *fg_data,
								u8 min, u8 max)
{
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_info("%s: min(0x%02x), max(0x%02x)\n", __func__, min, max);

	i2c_data[1] = max;
	i2c_data[0] = min;
	max17047_i2c_write(client, MAX17047_REG_TALRT_TH, i2c_data);

	max17047_i2c_read(client, MAX17047_REG_TALRT_TH, i2c_data);
	if ((i2c_data[0] != min) || (i2c_data[1] != max))
		pr_err("%s: TALRT_TH is not valid (0x%02d%02d ? 0x%02d%02d)\n",
			__func__, i2c_data[1], i2c_data[0], max, min);
}

/* Voltage alert, disabled(0xFF00) */
static void max17047_set_valrt(struct max17047_fuelgauge_data *fg_data,
								u8 min, u8 max)
{
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_info("%s: min(%dmV), max(%dmV)\n", __func__, min * 20, max * 20);

	i2c_data[1] = max;
	i2c_data[0] = min;
	max17047_i2c_write(client, MAX17047_REG_VALRT_TH, i2c_data);

	max17047_i2c_read(client, MAX17047_REG_VALRT_TH, i2c_data);
	if ((i2c_data[0] != min) || (i2c_data[1] != max))
		pr_err("%s: VALRT_TH is not valid (0x%02d%02d ? 0x%02d%02d)\n",
			__func__, i2c_data[1], i2c_data[0], max, min);
}

static void max17047_alert_init(struct max17047_fuelgauge_data *fg_data)
{
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_debug("%s\n", __func__);

	/* SALRT Threshold setting */
	/* min 1%, max disable */
	max17047_set_salrt(fg_data, 0x01, 0xFF);

	/* TALRT Threshold setting */
	/* min disable, max disable */
	max17047_set_talrt(fg_data, 0x80, 0x7F);

	/* VALRT Threshold setting */
	/* min disable, max disable */
	max17047_set_valrt(fg_data, 0x00, 0xFF);

	/* Enable SOC alerts */
	max17047_i2c_read(client, MAX17047_REG_CONFIG, i2c_data);
	i2c_data[0] |= (0x1 << 2);
	max17047_i2c_write(client, MAX17047_REG_CONFIG, i2c_data);
}

static void max17047_reg_init(struct max17047_fuelgauge_data *fg_data)
{
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_debug("%s\n", __func__);

	if (max17047_i2c_read(client, MAX17047_REG_FILTERCFG, i2c_data) < 0)
		return;

	/* Clear average vcell (12 sec) */
	i2c_data[0] &= 0x8f;

	max17047_i2c_write(client, MAX17047_REG_FILTERCFG, i2c_data);

	i2c_data[0] = 0xd9;
	i2c_data[1] = 0x35;
	max17047_i2c_write(client, MAX17047_REG_CGAIN, i2c_data);
}

static void max17047_update_work(struct work_struct *work)
{
	struct max17047_fuelgauge_data *fg_data = container_of(work,
						struct max17047_fuelgauge_data,
						update_work.work);
	struct power_supply *battery_psy;
	struct i2c_client *client = fg_data->client;
	union power_supply_propval value;
	pr_debug("%s\n", __func__);

#ifdef CONFIG_SLP
	battery_psy = &fg_data->fuelgauge;
#else
	battery_psy = power_supply_get_by_name("battery");
#endif

	max17047_get_vcell(client);
	max17047_get_vfocv(client);
	max17047_get_avgvcell(client);
	max17047_get_rawsoc(client);
	max17047_get_soc(client);

	pr_info("%s: VCELL(%d), VFOCV(%d), AVGVCELL(%d), RAWSOC(%d), SOC(%d)\n",
					__func__, fg_data->vcell,
					fg_data->vfocv, fg_data->avgvcell,
					fg_data->rawsoc, fg_data->soc);

	max17047_test_read(fg_data);

	if (!battery_psy || !battery_psy->set_property) {
		pr_err("%s: fail to get battery power supply\n", __func__);
		return;
	}

	battery_psy->set_property(battery_psy,
				POWER_SUPPLY_PROP_STATUS,
				&value);

	wake_lock_timeout(&fg_data->update_wake_lock, HZ);
}

#ifdef DEBUG_FUELGAUGE_POLLING
static void max17047_polling_work(struct work_struct *work)
{
	struct max17047_fuelgauge_data *fg_data = container_of(work,
						struct max17047_fuelgauge_data,
						polling_work.work);
	int reg;
	int i;
	u8 data[2];
	u8 buf[512];

	max17047_get_vcell(fg_data->client);
	max17047_get_vfocv(fg_data->client);
	max17047_get_avgvcell(fg_data->client);
	max17047_get_rawsoc(fg_data->client);
	max17047_get_soc(fg_data->client);

	pr_info("%s: VCELL(%d), VFOCV(%d), AVGVCELL(%d), RAWSOC(%d), SOC(%d)\n",
					__func__, fg_data->vcell,
					fg_data->vfocv, fg_data->avgvcell,
					fg_data->rawsoc, fg_data->soc);

	max17047_test_read(fg_data);

	schedule_delayed_work(&fg_data->polling_work,
		msecs_to_jiffies(MAX17047_POLLING_INTERVAL));
}
#endif

static enum power_supply_property max17047_fuelgauge_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static int max17047_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17047_fuelgauge_data *fg_data = container_of(psy,
						  struct max17047_fuelgauge_data,
						  fuelgauge);
	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		switch (val->intval) {
		case VOLTAGE_TYPE_VCELL:
			/* uV */
			val->intval = max17047_get_vcell(fg_data->client) * 1000;
			break;
		case VOLTAGE_TYPE_VFOCV:
			/* uV */
			val->intval = max17047_get_vfocv(fg_data->client) * 1000;
			break;
		default:
			/* uV */
			val->intval = max17047_get_vcell(fg_data->client) * 1000;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		/* uV */
		val->intval = max17047_get_avgvcell(fg_data->client) * 1000;
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = get_fuelgauge_value(fg_data->client, FG_CURRENT);
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = get_fuelgauge_value(fg_data->client, FG_CURRENT_AVG);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		switch (val->intval) {
		case SOC_TYPE_ADJUSTED:
			/* 1% unit */
			val->intval = get_fuelgauge_soc(fg_data->client);
			break;
		case SOC_TYPE_RAW:
			/* 0.01% unit */
			val->intval = fg_data->rawsoc;
			break;
		case SOC_TYPE_FULL:
			val->intval = fg_data->full_soc;
			break;
		default:
			/* 1% unit */
		val->intval = get_fuelgauge_soc(fg_data->client);
			break;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = max17047_get_temperature(fg_data->client);
		break;
#if defined(CONFIG_MACH_KONA)
	case POWER_SUPPLY_PROP_COMPENSATION_3:
		val->intval = fg_data->is_comp_3;
		break;
	case POWER_SUPPLY_PROP_COMPENSATION_1:	
		val->intval = fg_data->is_comp_1;
		break;			
#endif
	
	default:
		return -EINVAL;
	}

	return 0;
}

static int max17047_reset_soc(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	u8 data[2];
	int vfocv, fullcap;

	/* delay for current stablization */
	msleep(500);

	dev_info(&client->dev,
		"%s: Before quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max17047_get_vcell(client), max17047_get_vfocv(client),
		fg_read_vfsoc(client), max17047_get_soc(client));
	dev_info(&client->dev,
		"%s: Before quick-start - current(%d), avg current(%d)\n",
		__func__, fg_read_current(client),
		fg_read_avg_current(client));

	if (is_jig_attached == JIG_OFF) {
		dev_info(&client->dev,
		"%s : Return by No JIG_ON signal\n", __func__);
		return 0;
	}

	fg_write_register(client, MAX17047_REG_CYCLES, 0);

	if (max17047_i2c_read(client, MAX17047_REG_MISCCFG, data) < 0) {
		dev_err(&client->dev, "%s: Failed to read MiscCFG\n", __func__);
		return -1;
	}

	data[1] |= (0x1 << 2);
	if (max17047_i2c_write(client, MAX17047_REG_MISCCFG, data) < 0) {
		dev_err(&client->dev,
			"%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(250);
	fg_write_register(client, MAX17047_REG_FULLCAP,
	fg_battery_data[battery_type].Capacity);
	msleep(500);

	dev_info(&client->dev,
		"%s: After quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max17047_get_vcell(client), max17047_get_vfocv(client),
	fg_read_vfsoc(client), max17047_get_soc(client));
	dev_info(&client->dev,
		"%s: After quick-start - current(%d), avg current(%d)\n",
		__func__, fg_read_current(client),
	fg_read_avg_current(client));
	fg_write_register(client, MAX17047_REG_CYCLES, 0x00a0);

/* P8 is not turned off by Quickstart @3.4V
 * (It's not a problem, depend on mode data)
 * Power off for factory test(File system, etc..) */
	vfocv = max17047_get_vfocv(client);
	if (vfocv < POWER_OFF_VOLTAGE_LOW_MARGIN) {
		dev_info(&client->dev, "%s: Power off condition(%d)\n",
		__func__, vfocv);

		fullcap = fg_read_register(client, MAX17047_REG_FULLCAP);
		/* FullCAP * 0.009 */
		fg_write_register(client, MAX17047_REG_REMCAP_REP,
			(u16)(fullcap * 9 / 1000));
		msleep(200);
		dev_info(&client->dev, "%s: new soc=%d, vfocv=%d\n", __func__,
		max17047_get_soc(client), vfocv);
	}

	dev_info(&client->dev,
		"%s: Additional step - VfOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max17047_get_vfocv(client),
	fg_read_vfsoc(client), max17047_get_soc(client));

	return 0;
}

void fg_fullcharged_compensation(struct i2c_client *client,
		u32 is_recharging, bool pre_update)
{
	struct max17047_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);
	static int new_fullcap_data;

	pr_info("%s: called.\n", __func__);

	dev_info(&client->dev, "%s: is_recharging(%d), pre_update(%d)\n",
		__func__, is_recharging, pre_update);

	new_fullcap_data =
		fg_read_register(client, MAX17047_REG_FULLCAP);
	if (new_fullcap_data < 0)
		new_fullcap_data = fg_battery_data[battery_type].Capacity;

	/* compare with initial capacity */
	if (new_fullcap_data >
		(fg_battery_data[battery_type].Capacity * 110 / 100)) {
		dev_info(&client->dev,
			"%s: [Case 1] capacity = 0x%04x, NewFullCap = 0x%04x\n",
			__func__, fg_battery_data[battery_type].Capacity,
			new_fullcap_data);

		new_fullcap_data =
			(fg_battery_data[battery_type].Capacity * 110) / 100;

		fg_write_register(client, MAX17047_REG_REMCAP_REP,
			(u16)(new_fullcap_data));
		fg_write_register(client, MAX17047_REG_FULLCAP,
			(u16)(new_fullcap_data));
	} else if (new_fullcap_data <
		(fg_battery_data[battery_type].Capacity * 50 / 100)) {
		dev_info(&client->dev,
			"%s: [Case 5] capacity = 0x%04x, NewFullCap = 0x%04x\n",
			__func__, fg_battery_data[battery_type].Capacity,
			new_fullcap_data);

		new_fullcap_data =
			(fg_battery_data[battery_type].Capacity * 50) / 100;

		fg_write_register(client, MAX17047_REG_REMCAP_REP,
			(u16)(new_fullcap_data));
		fg_write_register(client, MAX17047_REG_FULLCAP,
			(u16)(new_fullcap_data));
	} else {
	/* compare with previous capacity */
		if (new_fullcap_data >
			(fuelgauge->previous_fullcap * 110 / 100)) {
			dev_info(&client->dev,
				"%s: [Case 2] previous_fullcap = 0x%04x, NewFullCap = 0x%04x\n",
				__func__, fuelgauge->previous_fullcap,
				new_fullcap_data);

			new_fullcap_data =
				(fuelgauge->previous_fullcap * 110) / 100;

			fg_write_register(client, MAX17047_REG_REMCAP_REP,
				(u16)(new_fullcap_data));
			fg_write_register(client, MAX17047_REG_FULLCAP,
				(u16)(new_fullcap_data));
		} else if (new_fullcap_data <
			(fuelgauge->previous_fullcap * 90 / 100)) {
			dev_info(&client->dev,
				"%s: [Case 3] previous_fullcap = 0x%04x, NewFullCap = 0x%04x\n",
				__func__, fuelgauge->previous_fullcap,
				new_fullcap_data);

			new_fullcap_data =
				(fuelgauge->previous_fullcap * 90) / 100;

			fg_write_register(client, MAX17047_REG_REMCAP_REP,
				(u16)(new_fullcap_data));
			fg_write_register(client, MAX17047_REG_FULLCAP,
				(u16)(new_fullcap_data));
		} else {
			dev_info(&client->dev,
				"%s: [Case 4] previous_fullcap = 0x%04x, NewFullCap = 0x%04x\n",
				__func__, fuelgauge->previous_fullcap,
				new_fullcap_data);
		}
	}

	/* 4. Write RepSOC(06h)=100%; */
	fg_write_register(client, MAX17047_SOCREP, (u16)(0x64 << 8));

	/* 5. Write MixSOC(0Dh)=100%; */
	fg_write_register(client, MAX17047_REG_SOCMIX, (u16)(0x64 << 8));

	/* 6. Write AVSOC(0Eh)=100%; */
	fg_write_register(client, MAX17047_REG_SOCAV, (u16)(0x64 << 8));

	/* if pre_update case, skip updating PrevFullCAP value. */
	if (!pre_update)
		fuelgauge->previous_fullcap =
			fg_read_register(client, MAX17047_REG_FULLCAP);

	dev_info(&client->dev,
		"%s: (A) FullCap = 0x%04x, RemCap = 0x%04x\n", __func__,
		fg_read_register(client, MAX17047_REG_FULLCAP),
		fg_read_register(client, MAX17047_REG_REMCAP_REP));

	fg_periodic_read(client);
}

int check_battery_type(struct max17047_fuelgauge_data *fg_data)
{
	int type = UNKNOWN_TYPE;
	int adc_temp = 0;

	adc_temp = s3c_adc_read(fg_data->adc_client, 3);
	adc_temp /= 10;

	pr_info("%s: adc_temp = %d\n", __func__, adc_temp);

	if (adc_temp >= ADC_LIS_BATTERY_MIN && adc_temp <= ADC_LIS_BATTERY_MAX)
		type = LIS_BATTERY_TYPE;
	else if (adc_temp >= ADC_BYD_BATTERY_MIN && adc_temp <= ADC_BYD_BATTERY_MAX)
		type = BYD_BATTERY_TYPE;
	else
		type = SDI_BATTERY_TYPE;

	return type;
}

bool sec_hal_fg_full_charged(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fuelgauge =
				i2c_get_clientdata(client);
	struct power_supply *battery_psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	battery_psy->get_property(battery_psy,
				POWER_SUPPLY_PROP_STATUS,
				&value);

	/* full charge compensation algorithm by MAXIM */
	fg_fullcharged_compensation(client,
		(int)(value.intval == POWER_SUPPLY_STATUS_FULL), true);

	cancel_delayed_work(&fuelgauge->full_comp_work);
	schedule_delayed_work(&fuelgauge->full_comp_work, 100);

	return false;
}

static int max17047_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct max17047_fuelgauge_data *fg_data = container_of(psy,
					  struct max17047_fuelgauge_data,
					  fuelgauge);

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		max17047_reset_soc(fg_data->client);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval != POWER_SUPPLY_STATUS_FULL)
			return -EINVAL;
		pr_info("%s: charger full state!\n", __func__);
		/* adjust full soc */
		max17047_adjust_fullsoc(fg_data->client);
		sec_hal_fg_full_charged(fg_data->client);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void full_comp_work_handler(struct work_struct *work)
{
	struct max17047_fuelgauge_data *fg_data =
		container_of(work, struct max17047_fuelgauge_data, full_comp_work.work);
	struct power_supply *battery_psy = power_supply_get_by_name("battery");
	

	int avg_current;
	union power_supply_propval value;

	avg_current = get_fuelgauge_value(fg_data->client, FG_CURRENT_AVG);

	battery_psy->get_property(battery_psy,
				POWER_SUPPLY_PROP_STATUS,
				&value);

	pr_info("%s: avg_current(%d).\n", __func__, avg_current);

	if (avg_current >= 25) {
		cancel_delayed_work(&fg_data->full_comp_work);
		schedule_delayed_work(&fg_data->full_comp_work, 100);
	} else {
		dev_info(&fg_data->client->dev,
			"%s: full charge compensation start (avg_current %d)\n",
			__func__, avg_current);
		fg_fullcharged_compensation(fg_data->client,
			(int)(value.intval ==
			POWER_SUPPLY_STATUS_FULL), false);
	}
}


static irqreturn_t max17047_fuelgauge_isr(int irq, void *data)
{
	struct max17047_fuelgauge_data *fg_data = data;
	struct i2c_client *client = fg_data->client;
	u8 i2c_data[2];
	pr_info("%s: irq(%d)\n", __func__, irq);
	mutex_lock(&fg_data->irq_lock);

	max17047_i2c_read(client, MAX17047_REG_STATUS, i2c_data);
	pr_info("%s: MAX17047_REG_STATUS(0x%02x%02x)\n", __func__,
					i2c_data[1], i2c_data[0]);

	cancel_delayed_work(&fg_data->update_work);
	wake_lock(&fg_data->update_wake_lock);
	schedule_delayed_work(&fg_data->update_work, msecs_to_jiffies(1000));

	mutex_unlock(&fg_data->irq_lock);
	return IRQ_HANDLED;
}

#if defined(CONFIG_TARGET_LOCALE_KOR)
#ifdef CONFIG_DEBUG_FS
static int max17047_debugfs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t max17047_debugfs_read_registers(struct file *filp,
	char __user *buffer, size_t count, loff_t *ppos)
{
	struct max17047_fuelgauge_data *fg_data = filp->private_data;
	struct i2c_client *client = NULL;
	u8 i2c_data[2];
	int reg = 0;
	char *buf;
	size_t len = 0;
	ssize_t ret;

	if (!fg_data) {
		pr_err("%s : fg_data is null\n", __func__);
		return 0;
	}

	client = fg_data->client;

	if (*ppos != 0)
		return 0;

	if (count < sizeof(buf))
		return -ENOSPC;

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	reg = MAX17047_REG_STATUS;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"status(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_CONFIG;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"config(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_RCOMP;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"rcomp(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_CGAIN;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"cgain(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_SALRT_TH;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"salrt(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_MISCCFG;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"misc(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_TEMPCO;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"tempc0(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_REMCAP_MIX;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"remCapMix(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	reg = MAX17047_REG_REMCAP_REP;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"remCapRep(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);	

	reg = MAX17047_REG_FULLCAP;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"fullCap(0x%x)=%02x%02x ", reg, i2c_data[1], i2c_data[0]);

	len += snprintf(buf + len, PAGE_SIZE - len,
		"SOCRep(0x%x)=%d ", reg, max17047_get_soc(client));

	len += snprintf(buf + len, PAGE_SIZE - len,
		"SOCVf(0x%x)=%d ", reg, fg_read_vfsoc(client));

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	ret = simple_read_from_buffer(buffer, len, ppos, buf, PAGE_SIZE);
	kfree(buf);

	return ret;
}

static const struct file_operations max17047_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = max17047_debugfs_open,
	.read = max17047_debugfs_read_registers,
};

static ssize_t max17047_debugfs_read_defaultdata(struct file *filp,
	char __user *buffer, size_t count, loff_t *ppos)
{
	struct max17047_fuelgauge_data *fg_data = filp->private_data;
	struct i2c_client *client = NULL;
	u8 i2c_data[2];
	int reg = 0;
	char *buf;
	size_t len = 0;
	ssize_t ret;

	if (!fg_data) {
		pr_err("%s : fg_data is null\n", __func__);
		return 0;
	}

	client = fg_data->client;

	if (*ppos != 0)
		return 0;

	if (count < sizeof(buf))
		return -ENOSPC;

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	reg = MAX17047_REG_RCOMP;
	max17047_i2c_read(client, reg, i2c_data);
	len += snprintf(buf + len, PAGE_SIZE - len,
		"rcomp=%02x%02x ", i2c_data[1], i2c_data[0]);

	len += snprintf(buf + len, PAGE_SIZE - len,
		"fsoc=%d", fg_data->full_soc);

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	ret = simple_read_from_buffer(buffer, len, ppos, buf, PAGE_SIZE);
	kfree(buf);

	return ret;
}

static const struct file_operations max17047_debugfs_fops2 = {
	.owner = THIS_MODULE,
	.open = max17047_debugfs_open,
	.read = max17047_debugfs_read_defaultdata,
};
#endif
#endif

#ifdef CONFIG_FG_SYSFS
static ssize_t max17047_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf);
static ssize_t max17047_fg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);
#define MAX17047_FG_ATTR(_name)			\
{						\
	.attr = { .name = #_name,		\
		  .mode = 0664 },		\
	.show = max17047_fg_show_property,		\
	.store = max17047_fg_store,			\
}

static struct device_attribute max17047_fg_attrs[] = {
	MAX17047_FG_ATTR(fg_curr_ua),
	MAX17047_FG_ATTR(fg_avg_curr_ua),
};

enum {
	FG_CURR_UA = 0,
	FG_AVG_CURR_UA,
};

static ssize_t max17047_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct max17047_fuelgauge_data *fg_data = dev_get_drvdata(dev->parent);
	int i = 0;
	const ptrdiff_t off = attr - max17047_fg_attrs;

	switch (off) {
	case FG_CURR_UA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			get_fuelgauge_value(fg_data->client, FG_CURRENT));
		break;
	case FG_AVG_CURR_UA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			get_fuelgauge_value(fg_data->client, FG_CURRENT_AVG));
		break;
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t max17047_fg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret = 0;
	const ptrdiff_t off = attr - max17047_fg_attrs;

	switch (off) {
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int max17047_fg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(max17047_fg_attrs); i++) {
		rc = device_create_file(dev, &max17047_fg_attrs[i]);
		if (rc)
			goto fg_attrs_failed;
	}
	goto succeed;

fg_attrs_failed:
	while (i--)
		device_remove_file(dev, &max17047_fg_attrs[i]);
succeed:
	return rc;
}
#endif

static int __devinit max17047_fuelgauge_i2c_probe(struct i2c_client *client,
						  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17047_fuelgauge_data *fg_data;
	struct max17047_platform_data *pdata = client->dev.platform_data;
	int ret = -ENODEV;
	int rawsoc, firstsoc;
	ktime_t	current_time;
	struct timespec ts;

	pr_info("%s: fuelgauge init\n", __func__);

	if (!pdata) {
		pr_err("%s: no platform data\n", __func__);
		return -ENODEV;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	fg_data = kzalloc(sizeof(struct max17047_fuelgauge_data), GFP_KERNEL);
	if (!fg_data)
		return -ENOMEM;

	fg_data->client = client;
	fg_data->pdata = pdata;
	fg_client = client;

	i2c_set_clientdata(client, fg_data);

	mutex_init(&fg_data->irq_lock);

	wake_lock_init(&fg_data->update_wake_lock, WAKE_LOCK_SUSPEND,
							       "fuel-update");

	/* Initialize full_soc, set this before fisrt SOC reading */
	fg_data->full_soc = FULL_SOC_DEFAULT;
	/* first full_soc update */
	rawsoc = max17047_get_rawsoc(fg_data->client);
	if (rawsoc > FULL_SOC_DEFAULT)
		max17047_adjust_fullsoc(client);
	firstsoc = max17047_get_soc(client);
	pr_info("%s: rsoc=%d, fsoc=%d, soc=%d\n", __func__,
			rawsoc, fg_data->full_soc, firstsoc);

	if (fg_data->pdata->psy_name)
		fg_data->fuelgauge.name =
			fg_data->pdata->psy_name;
	else
		fg_data->fuelgauge.name = "max17047-fuelgauge";

	fg_data->fuelgauge.type = POWER_SUPPLY_TYPE_UNKNOWN;
	fg_data->fuelgauge.properties = max17047_fuelgauge_props;
	fg_data->fuelgauge.num_properties =
				ARRAY_SIZE(max17047_fuelgauge_props);
	fg_data->fuelgauge.get_property = max17047_get_property;
	fg_data->fuelgauge.set_property = max17047_set_property;

	ret = power_supply_register(&client->dev, &fg_data->fuelgauge);
	if (ret) {
		pr_err("%s: failed power supply register\n", __func__);
		goto err_psy_reg_fg;
	}

	current_time = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(current_time);

	fg_data->fullcap_check_interval = ts.tv_sec;
	fg_data->is_first_check = true;
	fg_data->low_batt_boot_flag = false;

	/* Init parameters to prevent wrong compensation */
	fg_data->previous_fullcap =
			fg_read_register(fg_data->client, MAX17047_REG_FULLCAP);

	fg_data->previous_vffullcap =
			fg_read_register(fg_data->client, MAX17047_REG_FULLCAPNOM);

	max17047_test_read(fg_data);

	if (is_jig_attached == JIG_ON)
		fg_reset_capacity_by_jig_connection(fg_data->client);

	/* Initialize fuelgauge alert */
	max17047_alert_init(fg_data);

	INIT_DELAYED_WORK_DEFERRABLE(&fg_data->update_work,
					max17047_update_work);

	INIT_DELAYED_WORK(&fg_data->full_comp_work,
		full_comp_work_handler);

	/* Request IRQ */
	fg_data->irq = gpio_to_irq(fg_data->pdata->irq_gpio);
	ret = gpio_request(fg_data->pdata->irq_gpio, "fuelgauge-irq");
	if (ret) {
		pr_err("%s: failed requesting gpio %d\n", __func__,
				fg_data->pdata->irq_gpio);
		goto err_irq;
	}
	gpio_direction_input(fg_data->pdata->irq_gpio);
	gpio_free(fg_data->pdata->irq_gpio);

	ret = request_threaded_irq(fg_data->irq, NULL,
				max17047_fuelgauge_isr, IRQF_TRIGGER_FALLING,
				"max17047-alert", fg_data);
	if (ret < 0) {
		pr_err("%s: fail to request max17047 irq: %d: %d\n",
				__func__, fg_data->irq, ret);
		goto err_irq;
	}

	ret = enable_irq_wake(fg_data->irq);
	if (ret < 0) {
		pr_err("%s: failed enable irq wake %d\n", __func__,
						fg_data->irq);
		goto err_enable_irq;
	}

	/* alloc platform device for adc client */
	pdev_fuelgauge_adc = platform_device_alloc("fuelgauge-adc", -1);
	if (!pdev_fuelgauge_adc) {
		pr_err("%s: could not allocation pdev_fuelgauge_adc.\n", __func__);
		ret = -ENOMEM;
		goto err_platform_allocate_device_adc;
	}

	/* Register adc client */
	fg_data->adc_client= s3c_adc_register(pdev_fuelgauge_adc, NULL, NULL, 0);

	if (IS_ERR(fg_data->adc_client)) {
		dev_err(&pdev_fuelgauge_adc->dev, "cannot register adc\n");
		ret = PTR_ERR(fg_data->adc_client);
		goto err_platform_register_device_adc;
	}

	/* Check battery type*/
	battery_type = check_battery_type(fg_data);

	pr_info("[BAT] battery type : %d\n", battery_type);

#ifdef DEBUG_FUELGAUGE_POLLING
	INIT_DELAYED_WORK_DEFERRABLE(&fg_data->polling_work,
					max17047_polling_work);
	schedule_delayed_work(&fg_data->polling_work, 0);
#else
	max17047_test_read(fg_data);
#endif

	pr_info("%s: probe complete\n", __func__);

#if defined(CONFIG_TARGET_LOCALE_KOR)
#ifdef CONFIG_DEBUG_FS
	fg_data->fg_debugfs_dir =
		debugfs_create_dir("fg_debug", NULL);
	if (fg_data->fg_debugfs_dir) {
		if (!debugfs_create_file("max17047_regs", 0644,
			fg_data->fg_debugfs_dir,
			fg_data, &max17047_debugfs_fops))
			pr_err("%s : debugfs_create_file, error\n", __func__);
		if (!debugfs_create_file("default_data", 0644,
			fg_data->fg_debugfs_dir,
			fg_data, &max17047_debugfs_fops2))
			pr_err("%s : debugfs_create_file2, error\n", __func__);
	} else
		pr_err("%s : debugfs_create_dir, error\n", __func__);
#endif
#endif

#ifdef CONFIG_FG_SYSFS
	max17047_fg_create_attrs(fg_data->fuelgauge.dev);
#endif

	return 0;


err_platform_allocate_device_adc:
	platform_device_unregister(pdev_fuelgauge_adc);
err_platform_register_device_adc:
	s3c_adc_release(fg_data->adc_client);
err_enable_irq:
	free_irq(fg_data->irq, fg_data);
err_irq:
	power_supply_unregister(&fg_data->fuelgauge);
err_psy_reg_fg:
	wake_lock_destroy(&fg_data->update_wake_lock);
	mutex_destroy(&fg_data->irq_lock);
	kfree(fg_data);
	return ret;
}

static int __devexit max17047_fuelgauge_remove(struct i2c_client *client)
{
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);

	wake_lock_destroy(&fg_data->update_wake_lock);
	free_irq(fg_data->irq, fg_data);
	power_supply_unregister(&fg_data->fuelgauge);
#ifdef DEBUG_FUELGAUGE_POLLING
	cancel_delayed_work(&fg_data->polling_work);
#endif
	cancel_delayed_work(&fg_data->update_work);
	mutex_destroy(&fg_data->irq_lock);
	kfree(fg_data);

	return 0;
}

#ifdef CONFIG_PM
static int max17047_fuelgauge_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	int charge_state, voltage_max, voltage_min;
	int valrt_vol;
	pr_info("%s\n", __func__);

#ifdef DEBUG_FUELGAUGE_POLLING
	cancel_delayed_work(&fg_data->polling_work);
#endif

#if !defined(CONFIG_SLP)
	/* default disable */
	valrt_vol = 0;

	/* voltage alert recharge voltage */
	if (!psy) {
		pr_err("%s: fail to get battery psy\n", __func__);
		return 0;
	}
	psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
	charge_state = value.intval;

	if (charge_state == POWER_SUPPLY_STATUS_FULL) {
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
									&value);
		voltage_max = value.intval;

		/* valrt voltage set as recharge voltage */
		valrt_vol = voltage_max - RECHG_DROP_VALUE;
	} else {
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
									&value);
		voltage_min = value.intval;

		/* valrt voltage set as min voltage - 50mV */
		valrt_vol = voltage_min - 50000;
	}

	pr_info("%s: charge state(%d), vcell(%d), valrt(%d)\n", __func__,
				charge_state, fg_data->vcell, valrt_vol);

	/* set voltage alert */
	max17047_set_valrt(fg_data, (valrt_vol / 1000 / 20), 0xFF);
#endif

	return 0;
}

static int max17047_fuelgauge_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct max17047_fuelgauge_data *fg_data = i2c_get_clientdata(client);
	pr_info("%s\n", __func__);

#if !defined(CONFIG_SLP)
	/* min disable, max disable */
	max17047_set_valrt(fg_data, 0x00, 0xFF);
#endif

#ifdef DEBUG_FUELGAUGE_POLLING
	schedule_delayed_work(&fg_data->polling_work, 0);
#endif

	return 0;
}
#else
#define max17047_fuelgauge_suspend NULL
#define max17047_fuelgauge_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id max17047_fuelgauge_id[] = {
	{"max17047-fuelgauge", 0},
	{}
};

#ifdef CONFIG_HIBERNATION
static const u16 save_addr[] = {
	MAX17047_REG_VALRT_TH,
	MAX17047_REG_TALRT_TH,
	MAX17047_REG_SALRT_TH,

	MAX17047_REG_TEMPERATURE,
	MAX17047_REG_CONFIG,

	MAX17047_REG_LEARNCFG,
	MAX17047_REG_FILTERCFG,
	MAX17047_REG_MISCCFG,
	MAX17047_REG_CGAIN,
	MAX17047_REG_RCOMP,
	MAX17047_REG_SOC_VF,
};


static int max17047_freeze(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct max17047_fuelgauge_data *fg_data
						= i2c_get_clientdata(client);
	int i, j;

	if (fg_data->reg_dump) {
		dev_err(dev, "Register dump is not clean.\n");
		return -EINVAL;
	}

	fg_data->reg_dump = kzalloc(sizeof(u16) * ARRAY_SIZE(save_addr),
				 GFP_KERNEL);
	if (!fg_data->reg_dump) {
		dev_err(dev, "Cannot allocate memory for hibernation dump.\n");
		return -ENOMEM;
	}

	for (i = 0, j = 0; i < ARRAY_SIZE(save_addr); i++, j += 2)
		max17047_i2c_read(client, save_addr[i]
					, &(fg_data->reg_dump[j]));

	return 0;
}

static int max17047_restore(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct max17047_fuelgauge_data *fg_data
						= i2c_get_clientdata(client);
	int i, j;

	if (!fg_data->reg_dump) {
		dev_err(dev, "Cannot allocate memory for hibernation dump.\n");
		return -ENOMEM;
	}

	for (i = 0, j = 0; i < ARRAY_SIZE(save_addr); i++, j += 2)
		max17047_i2c_write(client, save_addr[i]
					, &(fg_data->reg_dump[j]));

	kfree(fg_data->reg_dump);
	fg_data->reg_dump = NULL;

	return 0;
}
#endif



#ifdef CONFIG_PM
const struct dev_pm_ops max17047_pm = {
	.suspend = max17047_fuelgauge_suspend,
	.resume = max17047_fuelgauge_resume,
#ifdef CONFIG_HIBERNATION
	.freeze = max17047_freeze,
	.thaw = max17047_restore,
	.restore = max17047_restore,
#endif
};
#endif


MODULE_DEVICE_TABLE(i2c, max17047_fuelgauge_id);

static struct i2c_driver max17047_i2c_driver = {
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "max17047-fuelgauge",
		.pm = &max17047_pm,

	},
	.probe		= max17047_fuelgauge_i2c_probe,
	.remove		= __devexit_p(max17047_fuelgauge_remove),
	.id_table	= max17047_fuelgauge_id,
};

static int __init max17047_fuelgauge_init(void)
{
	return i2c_add_driver(&max17047_i2c_driver);
}

static void __exit max17047_fuelgauge_exit(void)
{
	i2c_del_driver(&max17047_i2c_driver);
}

module_init(max17047_fuelgauge_init);
module_exit(max17047_fuelgauge_exit);

MODULE_AUTHOR("SangYoung Son <hello.son@samsung.com>");
MODULE_DESCRIPTION("max17047 Fuel gauge driver");
MODULE_LICENSE("GPL");
