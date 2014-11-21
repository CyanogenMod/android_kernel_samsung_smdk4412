/*
 * linux/arch/arm/mach-exynos/charger-slp.c
 * COPYRIGHT(C) 2011
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * Charger Support with Charger-Manager Framework
 *
 */

#include <linux/io.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/power/charger-manager.h>
#include <linux/hwmon.h>
#include <linux/platform_data/ntc_thermistor.h>

#include <plat/adc.h>
#include <plat/pm.h>

#include <mach/regs-pmu.h>
#include <mach/irqs.h>

#include "board-mobile.h"

#define S5P_WAKEUP_STAT_WKSRC_MASK	0x000ffe3f
#define ADC_SAMPLING_CNT 7
#define SDI_2100MA_BATT	4350000

/* Temperatures in milli-centigrade */
#define SECBATTSPEC_TEMP_HIGH		(65 * 1000)
#define SECBATTSPEC_TEMP_HIGH_REC	(43 * 1000)
#define SECBATTSPEC_TEMP_LOW		(-5 * 1000)
#define SECBATTSPEC_TEMP_LOW_REC	(0 * 1000)

#ifdef CONFIG_SENSORS_NTC_THERMISTOR
struct platform_device midas_ncp15wb473_thermistor;
static int ntc_adc_num = -EINVAL; /* Uninitialized */
static struct s3c_adc_client *ntc_adc;

int __init adc_ntc_init(int port)
{
	int err = 0;

	if (port < 0 || port > 9)
		return -EINVAL;
	ntc_adc_num = port;

	ntc_adc = s3c_adc_register(&midas_ncp15wb473_thermistor,
			NULL, NULL, 0);
	if (IS_ERR(ntc_adc)) {
		err = PTR_ERR(ntc_adc);
		ntc_adc = NULL;
		return err;
	}

	return 0;
}

static int read_thermistor_uV(void)
{
	int val, i;
	int adc_min = 0;
	int adc_max = 0;
	int adc_total = 0;

	s64 converted;

	WARN(ntc_adc == NULL || ntc_adc_num < 0,
	     "NTC-ADC is not initialized for %s.\n", __func__);

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {
		val = s3c_adc_read(ntc_adc, ntc_adc_num);
		if (val <  0) {
			pr_err("%s : read failed.(%d).\n", __func__, val);
			return -EAGAIN;
		}

		if (i != 0) {
			if (val > adc_max)
				adc_max = val;
			else if (val < adc_min)
				adc_min = val;
		} else {
			adc_max = val;
			adc_min = val;
		}

		adc_total += val;
	}

	val = (adc_total - (adc_max + adc_min)) / (ADC_SAMPLING_CNT - 2);

	/* Multiplied by maximum input voltage */
	converted = 1800000LL * (s64) val;
	/* Divided by resolution */
	converted >>= 12;

	return converted;
}

static struct ntc_thermistor_platform_data ncp15wb473_pdata = {
	.read_uV	= read_thermistor_uV,
	.pullup_uV	= 1800000, /* VCC_1.8V_AP */
	.pullup_ohm	= 100000, /* 100K */
	.pulldown_ohm	= 100000, /* 100K */
	.connect	= NTC_CONNECTED_GROUND,
};

static int __read_thermistor_mC(int *mC)
{
	int ret;
	static struct device *hwmon;
	static struct hwmon_property *entry;

	if (ntc_adc_num == -EINVAL)
		return -ENODEV;

	if (hwmon == NULL)
		hwmon = hwmon_find_device(&midas_ncp15wb473_thermistor.dev);

	if (IS_ERR_OR_NULL(hwmon)) {
		hwmon = NULL;
		return -ENODEV;
	}

	if (entry == NULL)
		entry = hwmon_get_property(hwmon, "temp1_input");
	if (IS_ERR_OR_NULL(entry)) {
		entry = NULL;
		return -ENODEV;
	}

	ret = hwmon_get_value(hwmon, entry, mC);
	if (ret < 0) {
		entry = NULL;
		return ret;
	}

	return 0;
}
#else
static int __read_thermistor_mC(int *mC)
{
	*mC = 25000;
	return 0;
}
#endif

enum temp_stat { TEMP_OK = 0, TEMP_HOT = 1, TEMP_COLD = -1 };

static int midas_thermistor_ck(int *mC)
{
	static enum temp_stat state = TEMP_OK;

	 __read_thermistor_mC(mC);

	switch (state) {
	case TEMP_OK:
		if (*mC >= SECBATTSPEC_TEMP_HIGH)
			state = TEMP_HOT;
		else if (*mC <= SECBATTSPEC_TEMP_LOW)
			state = TEMP_COLD;
		break;
	case TEMP_HOT:
		if (*mC <= SECBATTSPEC_TEMP_LOW)
			state = TEMP_COLD;
		else if (*mC < SECBATTSPEC_TEMP_HIGH_REC)
			state = TEMP_OK;
		break;
	case TEMP_COLD:
		if (*mC >= SECBATTSPEC_TEMP_HIGH)
			state = TEMP_HOT;
		else if (*mC > SECBATTSPEC_TEMP_LOW_REC)
			state = TEMP_OK;
	default:
		pr_err("%s has invalid state %d\n", __func__, state);
	}

	return state;
}

static bool s3c_wksrc_rtc_alarm(void)
{
	u32 reg = s3c_suspend_wakeup_stat & S5P_WAKEUP_STAT_WKSRC_MASK;

	if ((reg & S5P_WAKEUP_STAT_RTCALARM) &&
	    !(reg & ~S5P_WAKEUP_STAT_RTCALARM))
		return true; /* yes, it is */

	return false;
}

static char *midas_charger_stats[] = {
#if defined(CONFIG_BATTERY_MAX77693_CHARGER)
	"max77693-charger",
#endif
	NULL };

struct charger_cable charger_cable_vinchg1[] = {
	{
		.extcon_name	= "max77693-muic",
		.name		= "USB",
		.min_uA		= 475000,
		.max_uA		= 475000 + 25000,
	}, {
		.extcon_name	= "max77693-muic",
		.name		= "TA",
		.min_uA		= 650000,
		.max_uA		= 650000 + 25000,
	}, {
		.extcon_name	= "max77693-muic",
		.name		= "MHL",
	},
};

static struct charger_regulator midas_regulators[] = {
	{
		.regulator_name	= "vinchg1",
		.cables		= charger_cable_vinchg1,
		.num_cables	= ARRAY_SIZE(charger_cable_vinchg1),
	},
};

static struct charger_desc midas_charger_desc = {
	.psy_name		= "battery",
	.polling_interval_ms	= 30000,
	.polling_mode		= CM_POLL_EXTERNAL_POWER_ONLY,
	.fullbatt_vchkdrop_ms	= 30000,
	.fullbatt_vchkdrop_uV	= 50000,
	.fullbatt_uV		= 4200000,
	.battery_present	= CM_CHARGER_STAT,
	.psy_charger_stat	= midas_charger_stats,
	.psy_fuel_gauge		= "max17047-fuelgauge",
	.is_temperature_error	= midas_thermistor_ck,
	.measure_ambient_temp	= true,
	.measure_battery_temp	= false,
	.soc_margin		= 0,

	.charger_regulators	= midas_regulators,
	.num_charger_regulators	= ARRAY_SIZE(midas_regulators),
};

struct charger_global_desc midas_charger_g_desc = {
	.rtc = "rtc0",
	.is_rtc_only_wakeup_reason = s3c_wksrc_rtc_alarm,
	.assume_timer_stops_in_suspend	= false,
};

struct platform_device midas_charger_manager = {
	.name			= "charger-manager",
	.dev			= {
		.platform_data = &midas_charger_desc,
	},
};

#ifdef CONFIG_SENSORS_NTC_THERMISTOR
struct platform_device midas_ncp15wb473_thermistor = {
	.name			= "ncp15wb473",
	.dev			= {
		.platform_data = &ncp15wb473_pdata,
	},
};
#endif

void cm_change_fullbatt_uV(void)
{
	midas_charger_desc.fullbatt_uV =  SDI_2100MA_BATT;
}
EXPORT_SYMBOL(cm_change_fullbatt_uV);

