/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>

#include <mach/gpio-p10.h>
#include <mach/regs-pmu.h>	/* S5P_INFORMX */

#include <plat/gpio-cfg.h>

#ifdef CONFIG_STMPE811_ADC
#include <linux/stmpe811-adc.h>
#endif

#if defined(CONFIG_BATTERY_SAMSUNG_P1X)
#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>

#define SEC_BATTERY_PMIC_NAME ""
#define SEC_FUELGAUGE_I2C_ID 9
#define SEC_CHARGER_I2C_ID 10

static bool sec_bat_adc_none_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_none_exit(void) { return true; }
static int sec_bat_adc_none_read(unsigned int channel) { return 0; }

static bool sec_bat_adc_ap_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ap_exit(void) { return true; }
static int sec_bat_adc_ap_read(unsigned int channel) { return 0; }

/* CHECK ME */
#define SMTPE811_CHANNEL_ADC_CHECK_1	6
#define SMTPE811_CHANNEL_VICHG		4	/* Not supported in P10 */

static bool sec_bat_adc_ic_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ic_exit(void) { return true; }
static int sec_bat_adc_ic_read(unsigned int channel)
{
	int data = 0;
	int max_voltage = 3300;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_CABLE_CHECK:
		data = stmpe811_get_adc_data(SMTPE811_CHANNEL_ADC_CHECK_1);
		data = data * max_voltage / 4095;	/* 4096 ? */
		break;
	}

	return data;
}

static bool sec_bat_gpio_init(void)
{
#if defined(CONFIG_MACH_P10_LTE_00_BD) || defined(CONFIG_MACH_P10_WIFI_00_BD)
	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);
#else
	/* IRQ to detect cable insertion and removal */
	s3c_gpio_cfgpin(GPIO_TA_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_INT, S3C_GPIO_PULL_NONE);
#endif

	return true;
}

static bool sec_fg_gpio_init(void)
{
	/* IRQ to detect low battery from fuel gauge */
	s3c_gpio_cfgpin(GPIO_FUEL_ALERT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_FUEL_ALERT, S3C_GPIO_PULL_UP);

	return true;
}

static bool sec_chg_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_TA_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TA_EN, S3C_GPIO_PULL_UP);
/*	gpio_set_value(GPIO_TA_EN, 1); */

	s3c_gpio_cfgpin(GPIO_TA_nCHG, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCHG, S3C_GPIO_PULL_UP);

#if defined(CONFIG_MACH_P10_LTE_00_BD) || defined(CONFIG_MACH_P10_WIFI_00_BD)
	/* GPIO_CHG_INT not supported */
#else
	/* IRQ to detect charger status change */
	s3c_gpio_cfgpin(GPIO_CHG_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CHG_INT, S3C_GPIO_PULL_UP);
#endif

	return true;
}

static bool sec_bat_is_lpm(void)
{
	u32 val = __raw_readl(S5P_INFORM2);

	pr_info("%s: LP charging: (INFORM2) 0x%x\n", __func__, val);

	if (val == 0x1)
		return true;

	return false;
}

static void sec_bat_initial_check(void)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	int ret = 0;

	value.intval = gpio_get_value(GPIO_TA_nCONNECTED);
	pr_debug("%s: %d\n", __func__, value.intval);

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static bool sec_bat_check_jig_status(void)
{
	/* TODO: */
	return false;
}

static void sec_bat_switch_to_check(void)
{
	pr_debug("%s\n", __func__);

	s3c_gpio_cfgpin(GPIO_USB_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_USB_SEL1, 0);

	mdelay(300);
}

static void sec_bat_switch_to_normal(void)
{
	pr_debug("%s\n", __func__);

	s3c_gpio_cfgpin(GPIO_USB_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_USB_SEL1, 1);
}

static int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

static int sec_bat_check_cable_callback(void)
{
	return current_cable_type;
}

static bool sec_bat_check_cable_result_callback(
				int cable_type)
{
	current_cable_type = cable_type;

	switch (cable_type) {
	case POWER_SUPPLY_TYPE_USB:
		pr_info("%s set vbus applied\n",
			__func__);
		break;
	case POWER_SUPPLY_TYPE_BATTERY:
		pr_info("%s set vbus cut\n",
			__func__);
		break;
	case POWER_SUPPLY_TYPE_MAINS:
	default:
		pr_err("%s cable type (%d)\n",
			__func__, cable_type);
		return false;
	}

	return true;
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
static bool sec_bat_check_callback(void) { return true; }
static bool sec_bat_check_result_callback(void) { return true; }

/* callback for OVP/UVLO check
 * return : int
 * battery health
 */
static int sec_bat_ovp_uvlo_callback(void)
{
	int health;
	health = POWER_SUPPLY_HEALTH_GOOD;

	return health;
}

static bool sec_bat_ovp_uvlo_result_callback(int health) { return true; }

/*
 * val.intval : temperature
 */
static bool sec_bat_get_temperature_callback(
		enum power_supply_property psp,
		union power_supply_propval *val) { return true; }

static bool sec_fg_fuelalert_process(bool is_fuel_alerted) { return true; }

/* ADC region should be exclusive */
static sec_bat_adc_region_t cable_adc_value_table[] = {
	{ 0,	500 },	/* POWER_SUPPLY_TYPE_BATTERY */
	{ 0,	0 },	/* POWER_SUPPLY_TYPE_UPS */
	{ 1000,	1500 },	/* POWER_SUPPLY_TYPE_MAINS */
	{ 0,	0 },	/* POWER_SUPPLY_TYPE_USB */
	{ 0,	0 },	/* POWER_SUPPLY_TYPE_OTG */
	{ 0,	0 },	/* POWER_SUPPLY_TYPE_DOCK */
	{ 0,	0 },	/* POWER_SUPPLY_TYPE_MISC */
};

/* charging current (mA, 0 - NOT supported) */
/* matching with power_supply_type in power_supply.h */
static sec_charging_current_t charging_current_table[] = {
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_BATTERY */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_UPS */
	{2000,	2000,	256,	0},	/* POWER_SUPPLY_TYPE_MAINS */
	{500,	500,	256,	0},	/* POWER_SUPPLY_TYPE_USB */
	{500,	500,	256,	0},	/* POWER_SUPPLY_TYPE_USB_DCP */
	{500,	500,	256,	0},	/* POWER_SUPPLY_TYPE_USB_CDP */
	{500,	500,	256,	0},	/* POWER_SUPPLY_TYPE_USB_ACA */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_OTG */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_DOCK */
	{500,	500,	256,	0},	/* POWER_SUPPLY_TYPE_MISC */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_WIRELESS */
};

/* unit: seconds */
static int polling_time_table[] = {
	10,	/* BASIC */
	30,	/* CHARGING */
	30,	/* DISCHARGING */
	30,	/* NOT_CHARGING */
	300,	/* SLEEP */
};

/* for MAX17050, MAX17047 */
static struct battery_data_t p10_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x2008,
		.low_battery_comp_voltage = 3600,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000,	0,	0},	/* dummy for top limit */
			{-1250, 0,	3320},
			{-750, 97,	3451},
			{-100, 96,	3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0,	0},	/* dummy for top limit */
		},
		.type_str = "SDI",
	}
};

static sec_battery_platform_data_t sec_battery_pdata = {
	/* NO NEED TO BE CHANGED */
	.initial_check = sec_bat_initial_check,
	.bat_gpio_init = sec_bat_gpio_init,
	.fg_gpio_init = sec_fg_gpio_init,
	.chg_gpio_init = sec_chg_gpio_init,

	.is_lpm = sec_bat_is_lpm,
	.check_jig_status = sec_bat_check_jig_status,
	.check_cable_callback =
		sec_bat_check_cable_callback,
	.cable_switch_check = sec_bat_switch_to_check,
	.cable_switch_normal = sec_bat_switch_to_normal,
	.check_cable_result_callback =
		sec_bat_check_cable_result_callback,
	.check_battery_callback =
		sec_bat_check_callback,
	.check_battery_result_callback =
		sec_bat_check_result_callback,
	.ovp_uvlo_callback = sec_bat_ovp_uvlo_callback,
	.ovp_uvlo_result_callback =
		sec_bat_ovp_uvlo_result_callback,
	.fuelalert_process = sec_fg_fuelalert_process,
	.get_temperature_callback =
		sec_bat_get_temperature_callback,

	.adc_api[SEC_BATTERY_ADC_TYPE_NONE] = {
		.init = sec_bat_adc_none_init,
		.exit = sec_bat_adc_none_exit,
		.read = sec_bat_adc_none_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_AP] = {
		.init = sec_bat_adc_ap_init,
		.exit = sec_bat_adc_ap_exit,
		.read = sec_bat_adc_ap_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_IC] = {
		.init = sec_bat_adc_ic_init,
		.exit = sec_bat_adc_ic_exit,
		.read = sec_bat_adc_ic_read
		},
	.cable_adc_value = cable_adc_value_table,
	.charging_current = charging_current_table,
	.polling_time = polling_time_table,
	/* NO NEED TO BE CHANGED */

	.pmic_name = SEC_BATTERY_PMIC_NAME,

	.adc_check_count = 7,
	.adc_type = {
		SEC_BATTERY_ADC_TYPE_IC,	/* CABLE_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,	/* BAT_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP_AMB */
		SEC_BATTERY_ADC_TYPE_NONE,	/* FULL_CHECK */
	},

	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.battery_data = (void *)p10_battery_data,
	.bat_gpio_ta_nconnected = GPIO_TA_nCONNECTED,
	.bat_polarity_ta_nconnected = 1,	/* active HIGH */
	.bat_irq = IRQ_EINT(0),	/* GPIO_TA_INT */
	.bat_irq_attr =
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE |
		SEC_BATTERY_CABLE_CHECK_INT,
	.cable_source_type = SEC_BATTERY_CABLE_SOURCE_ADC,

	.event_check = false,
	.event_waiting_time = 60,

	/* Monitor setting */
	.polling_type = SEC_BATTERY_MONITOR_ALARM,
	.monitor_initial_count = 3,

	/* Battery check */
	.battery_check_type = SEC_BATTERY_CHECK_NONE,
	.check_count = 3,

	/* Battery check by ADC */
	.check_adc_max = 0,
	.check_adc_min = 0,

	/* OVP/UVLO check */
	.ovp_uvlo_check_type = SEC_BATTERY_OVP_UVLO_CHGINT,

	/* Temperature check */
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_FG,

	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,
	.temp_check_count = 3,
	.temp_high_threshold_event = 650,
	.temp_high_recovery_event = 450,
	.temp_low_threshold_event = 0,
	.temp_low_recovery_event = -50,
	.temp_high_threshold_normal = 470,
	.temp_high_recovery_normal = 400,
	.temp_low_threshold_normal = 0,
	.temp_low_recovery_normal = -30,
	.temp_high_threshold_lpm = 600,
	.temp_high_recovery_lpm = 420,
	.temp_low_threshold_lpm = 2,
	.temp_low_recovery_lpm = -30,

	.full_check_type = SEC_BATTERY_FULLCHARGED_CHGGPIO,
	.full_check_count = 3,
	.full_check_adc_1st = 26500,	/* CHECK ME */
	.full_check_adc_2nd = 25800,	/* CHECK ME */
	.chg_gpio_full_check = GPIO_TA_nCHG,	/* STAT of bq24191 */
	.chg_polarity_full_check = 1,
	.full_condition_type =
		SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_OCV,
	.full_condition_soc = 99,
	.full_condition_ocv = 4170,

	.recharge_condition_type =
		SEC_BATTERY_RECHARGE_CONDITION_SOC |
		SEC_BATTERY_RECHARGE_CONDITION_VCELL,
	.recharge_condition_soc = 98,
	.recharge_condition_avgvcell = 4150,
	.recharge_condition_vcell = 4150,

	.charging_total_time = 6 * 60 * 60,
	.recharging_total_time = 90 * 60,
	.charging_reset_time = 10 * 60,

	/* Fuel Gauge */
	.fg_irq = IRQ_EINT(19),	/* GPIO_FUEL_ALERT */
	.fg_irq_attr = IRQF_TRIGGER_LOW | IRQF_ONESHOT,
	.fuel_alert_soc = 1,
	.repeated_fuelalert = false,
	.capacity_calculation_type =
		SEC_FUELGAUGE_CAPACITY_TYPE_RAW,
		/* SEC_FUELGAUGE_CAPACITY_TYPE_SCALE | */
		/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC, */
	.capacity_max = 1000,
	.capacity_min = 0,

	/* Charger */
	.chg_gpio_en = GPIO_TA_EN,
	.chg_polarity_en = 0,	/* active LOW charge enable */
	.chg_gpio_status = GPIO_TA_nCHG,
	.chg_polarity_status = 0,
#if defined(CONFIG_MACH_P10_LTE_00_BD) || defined(CONFIG_MACH_P10_WIFI_00_BD)
	.chg_irq = 0,
	.chg_irq_attr = 0,
#else
	.chg_irq = IRQ_EINT(4),	/* GPIO_CHG_INT */
	.chg_irq_attr = IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
#endif
	.chg_float_voltage = 4200,
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_battery_pdata,
};

static struct i2c_gpio_platform_data gpio_i2c_data_fuelgauge = {
	.sda_pin = GPIO_FUEL_SDA_18V,
	.scl_pin = GPIO_FUEL_SCL_18V,
};

struct platform_device sec_device_fuelgauge = {
	.name = "i2c-gpio",
	.id = SEC_FUELGAUGE_I2C_ID,
	.dev.platform_data = &gpio_i2c_data_fuelgauge,
};

static struct i2c_board_info sec_brdinfo_fuelgauge[] __initdata = {
	{
		I2C_BOARD_INFO("sec-fuelgauge",
			SEC_FUELGAUGE_I2C_SLAVEADDR),
		.platform_data	= &sec_battery_pdata,
	},
};

static struct i2c_gpio_platform_data gpio_i2c_data_charger = {
	.sda_pin = GPIO_CHG_SDA_18V,
	.scl_pin = GPIO_CHG_SCL_18V,
};

struct platform_device sec_device_charger = {
	.name = "i2c-gpio",
	.id = SEC_CHARGER_I2C_ID,
	.dev.platform_data = &gpio_i2c_data_charger,
};

static struct i2c_board_info sec_brdinfo_charger[] __initdata = {
	{
		I2C_BOARD_INFO("sec-charger",
			SEC_CHARGER_I2C_SLAVEADDR),
		.platform_data	= &sec_battery_pdata,
	},
};

static struct platform_device *sec_battery_devices[] __initdata = {
	&sec_device_charger,
	&sec_device_fuelgauge,
	&sec_device_battery,
};

void __init p10_battery_init(void)
{
	platform_add_devices(
		sec_battery_devices,
		ARRAY_SIZE(sec_battery_devices));

	i2c_register_board_info(
		SEC_CHARGER_I2C_ID,
		sec_brdinfo_charger,
		ARRAY_SIZE(sec_brdinfo_charger));

	i2c_register_board_info(
		SEC_FUELGAUGE_I2C_ID,
		sec_brdinfo_fuelgauge,
		ARRAY_SIZE(sec_brdinfo_fuelgauge));
}
#endif	/* CONFIG_BATTERY_SAMSUNG_P1X */
