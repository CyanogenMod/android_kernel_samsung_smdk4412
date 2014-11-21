/*
 *  adc_fuelgauge.c
 *  Samsung ADC Fuel Gauge Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/battery/sec_fuelgauge.h>
#if 0
static int adc_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int adc_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int adc_read_word(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}
#endif

static int adc_get_adc_data(struct sec_fuelgauge_info *fuelgauge,
		int adc_ch, int count)
{
	int adc_data;
	int adc_max;
	int adc_min;
	int adc_total;
	int i;

	adc_data = 0;
	adc_max = 0;
	adc_min = 0;
	adc_total = 0;

	for (i = 0; i < count; i++) {
		mutex_lock(&fuelgauge->info.adclock);
		adc_data = adc_read(fuelgauge->pdata, adc_ch);
		mutex_unlock(&fuelgauge->info.adclock);

		if (adc_data < 0)
			goto err;

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (count - 2);
err:
	return adc_data;
}

static int adc_get_adc_value(
		struct sec_fuelgauge_info *fuelgauge, int channel)
{
	int adc;

	adc = adc_get_adc_data(fuelgauge, channel,
		get_battery_data(fuelgauge).adc_check_count);

	if (adc < 0) {
		dev_err(&fuelgauge->client->dev,
			"%s: Error in ADC\n", __func__);
		return adc;
	}

	return adc;
}

static unsigned long adc_calculate_average(
		struct sec_fuelgauge_info *fuelgauge,
		int channel, int adc)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;

	cnt = fuelgauge->info.adc_sample[channel].cnt;
	total_adc = fuelgauge->info.adc_sample[channel].total_adc;

	if (adc < 0) {
		dev_err(&fuelgauge->client->dev,
			"%s : Invalid ADC : %d\n", __func__, adc);
		adc = fuelgauge->info.adc_sample[channel].average_adc;
	}

	if (cnt < ADC_HISTORY_SIZE) {
		fuelgauge->info.adc_sample[channel].adc_arr[cnt] =
			adc;
		fuelgauge->info.adc_sample[channel].index = cnt;
		fuelgauge->info.adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {
		index = fuelgauge->info.adc_sample[channel].index;
		if (++index >= ADC_HISTORY_SIZE)
			index = 0;

		total_adc = total_adc -
			fuelgauge->info.adc_sample[channel].adc_arr[index] +
			adc;
		average_adc = total_adc / ADC_HISTORY_SIZE;

		fuelgauge->info.adc_sample[channel].adc_arr[index] = adc;
		fuelgauge->info.adc_sample[channel].index = index;
	}

	fuelgauge->info.adc_sample[channel].total_adc = total_adc;
	fuelgauge->info.adc_sample[channel].average_adc =
		average_adc;

#if defined(DEBUG)
	{
		int i;
		printk("%s : ", __func__);
		for(i=0;i<ADC_HISTORY_SIZE;i++)
			printk("%d, ", fuelgauge->info.
				adc_sample[channel].adc_arr[i]);
		printk("\n");
	}
#endif

	return average_adc;
}

static int adc_get_data_by_adc(
		struct sec_fuelgauge_info *fuelgauge,
		const sec_bat_adc_table_data_t *adc_table,
		unsigned int adc_table_size, int adc)
{
	int data = 0;
	int low = 0;
	int high = 0;
	int mid = 0;

	if (adc_table[0].adc >= adc) {
		data = adc_table[0].data;
		goto data_by_adc_goto;
	} else if (adc_table[adc_table_size-1].adc <= adc) {
		data = adc_table[adc_table_size-1].data;
		goto data_by_adc_goto;
	}

	high = adc_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (adc_table[mid].adc > adc)
			high = mid - 1;
		else if (adc_table[mid].adc < adc)
			low = mid + 1;
		else {
			data = adc_table[mid].data;
			goto data_by_adc_goto;
		}
	}

	data = adc_table[low].data;
	data += ((adc_table[high].data - adc_table[low].data) *
		(adc - adc_table[low].adc)) /
		(adc_table[high].adc - adc_table[low].adc);

data_by_adc_goto:
	dev_dbg(&fuelgauge->client->dev,
		"%s: adc(%d), data(%d), high(%d), low(%d)\n",
		__func__, adc, data, high, low);

	return data;
}

static int adc_get_vcell(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	int vcell;

	vcell = adc_get_data_by_adc(fuelgauge,
		get_battery_data(fuelgauge).adc2vcell_table,
		get_battery_data(fuelgauge).adc2vcell_table_size,
		adc_get_adc_value(fuelgauge, SEC_BAT_ADC_CHANNEL_VOLTAGE_NOW));

	return vcell; 
}

static int adc_get_avg_vcell(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	int voltage_avg;

	voltage_avg = (int)adc_calculate_average(fuelgauge,
		ADC_CHANNEL_VOLTAGE_AVG, fuelgauge->info.voltage_now);

	return voltage_avg;
}

static int adc_get_ocv(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	int voltage_ocv;

	voltage_ocv = (int)adc_calculate_average(fuelgauge,
		ADC_CHANNEL_VOLTAGE_OCV, fuelgauge->info.voltage_avg);

	return voltage_ocv;
}

/* soc should be 0.01% unit */
static int adc_get_soc(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	int soc;

	soc = adc_get_data_by_adc(fuelgauge,
		get_battery_data(fuelgauge).ocv2soc_table,
		get_battery_data(fuelgauge).ocv2soc_table_size,
		fuelgauge->info.voltage_ocv);

	return soc; 
}

static int adc_get_current(struct i2c_client *client)
{
	union power_supply_propval value;

	psy_do_property("sec-charger", get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	return value.intval;
}

/* judge power off or not by current_avg */
static int adc_get_current_average(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	union power_supply_propval value_bat;
	union power_supply_propval value_chg;
	int vcell, soc, curr_avg;

	psy_do_property("sec-charger", get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value_chg);
	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_HEALTH, value_bat);
	vcell = adc_get_vcell(client);
	soc = adc_get_soc(client) / 100;

	/* if 0% && under 3.4v && low power charging(1000mA), power off */
	if (!fuelgauge->pdata->is_lpm() && (soc <= 0) && (vcell < 3400) &&
			((value_chg.intval < 1000) ||
			((value_bat.intval == POWER_SUPPLY_HEALTH_OVERHEAT) ||
			(value_bat.intval == POWER_SUPPLY_HEALTH_COLD)))) {
		dev_info(&client->dev, "%s: SOC(%d), Vnow(%d), Inow(%d)\n",
			__func__, soc, vcell, value_chg.intval);
		curr_avg = -1;
	} else {
		curr_avg = value_chg.intval;
	}

	return curr_avg;
}

static void adc_reset(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	int i;

	for (i = 0; i < ADC_CHANNEL_NUM; i++) {
		fuelgauge->info.adc_sample[i].cnt = 0;
		fuelgauge->info.adc_sample[i].total_adc = 0;
	}

	cancel_delayed_work(&fuelgauge->info.monitor_work);
	schedule_delayed_work(&fuelgauge->info.monitor_work, 0);
}

static void adc_monitor_work(struct work_struct *work)
{
	struct sec_fuelgauge_info *fuelgauge =
		container_of(work, struct sec_fuelgauge_info,
		info.monitor_work.work);

	fuelgauge->info.voltage_now = adc_get_vcell(fuelgauge->client);
	fuelgauge->info.voltage_avg = adc_get_avg_vcell(fuelgauge->client);
	fuelgauge->info.voltage_ocv = adc_get_ocv(fuelgauge->client);
	fuelgauge->info.current_now = adc_get_current(fuelgauge->client);
	fuelgauge->info.current_avg =
		adc_get_current_average(fuelgauge->client);
	fuelgauge->info.capacity = adc_get_soc(fuelgauge->client);

	dev_info(&fuelgauge->client->dev,
		"%s:Vnow(%dmV),Vavg(%dmV),Vocv(%dmV),"
		"Inow(%dmA),Iavg(%dmA),SOC(%d%%)\n", __func__,
		fuelgauge->info.voltage_now, fuelgauge->info.voltage_avg,
		fuelgauge->info.voltage_ocv, fuelgauge->info.current_now,
		fuelgauge->info.current_avg, fuelgauge->info.capacity);

	schedule_delayed_work(&fuelgauge->info.monitor_work,
		HZ * get_battery_data(fuelgauge).monitor_polling_time);
}

bool sec_hal_fg_init(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
			i2c_get_clientdata(client);

	mutex_init(&fuelgauge->info.adclock);
	INIT_DELAYED_WORK_DEFERRABLE(&fuelgauge->info.monitor_work,
		adc_monitor_work);

	schedule_delayed_work(&fuelgauge->info.monitor_work, HZ);
	return true;
}

bool sec_hal_fg_suspend(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_resume(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_fuelalert_init(struct i2c_client *client, int soc)
{
	return true;
}

bool sec_hal_fg_is_fuelalerted(struct i2c_client *client)
{
	return false;
}

bool sec_hal_fg_fuelalert_process(void *irq_data, bool is_fuel_alerted)
{
	return true;
}

bool sec_hal_fg_full_charged(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_reset(struct i2c_client *client)
{
	adc_reset(client);
	return true;
}

bool sec_hal_fg_get_property(struct i2c_client *client,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	struct sec_fuelgauge_info *fuelgauge =
			i2c_get_clientdata(client);

	switch (psp) {
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = fuelgauge->info.voltage_now;
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case SEC_BATTEY_VOLTAGE_AVERAGE:
			val->intval = fuelgauge->info.voltage_avg;
			break;
		case SEC_BATTEY_VOLTAGE_OCV:
			val->intval = fuelgauge->info.voltage_ocv;
			break;
		}
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = fuelgauge->info.current_now;
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = fuelgauge->info.current_avg;
		break;
		/* SOC (%) */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW)
			val->intval = fuelgauge->info.capacity;
		else
			val->intval = fuelgauge->info.capacity / 10;
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;

	default:
		return false;
	}
	return true;
}

bool sec_hal_fg_set_property(struct i2c_client *client,
			     enum power_supply_property psp,
			     const union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		break;
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;
	default:
		return false;
	}
	return true;
}

ssize_t sec_hal_fg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	int i = 0;

	switch (offset) {
	case FG_DATA:
	case FG_REGS:
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sec_hal_fg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count)
{
	int ret = 0;

	switch (offset) {
	case FG_REG:
	case FG_DATA:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
