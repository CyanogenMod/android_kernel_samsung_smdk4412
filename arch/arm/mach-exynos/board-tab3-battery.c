/* arch/arm/mach-exynos/board-tab3-battery.c
 *
 * Copyright (C) 2012 Samsung Electronics Co, Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/switch.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/machine.h>
#include <linux/platform_device.h>
#include <plat/adc.h>

#include <mach/gpio-rev00-tab3.h>

#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <plat/gpio-cfg.h>
#include <linux/stmpe811-adc.h>
#include <mach/usb_switch.h>

#if defined(CONFIG_TOUCHSCREEN_MMS152_TAB3) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
#include <mach/tab3-input.h>
#endif

#include <linux/usb/android_composite.h>
#include <plat/devs.h>

#if defined(CONFIG_TARGET_TAB3_3G8) ||\
		defined(CONFIG_TARGET_TAB3_WIFI8) ||\
		defined(CONFIG_TARGET_TAB3_LTE8)

void __init exynos_tab3_charger_init(void)
{

}

extern bool is_cable_attached;

#else

#define SEC_CHARGER_I2C_ID	13
#define SEC_FUELGAUGE_I2C_ID	14
#define P30_USB 7

#define SEC_BATTERY_PMIC_NAME ""

#define TA_ADC_LOW              800
#define TA_ADC_HIGH             2200

/* cable state */
extern bool is_cable_attached;
static struct s3c_adc_client *temp_adc_client;

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
		defined(CONFIG_TARGET_TAB3_WIFI10) ||\
		defined(CONFIG_TARGET_TAB3_LTE10)

/* Santos 10' Temperature ADC Table */
#else
/* Santos 7' Temperature ADC Table */
static sec_bat_adc_table_data_t tab3_temp_table[] = {
        {404, 600},
        {548, 500},
        {696, 420},
        {725, 400},
        {927, 300},
        {1144, 200},
        {1377, 100},
        {1460, 50},
        {1582, 0},
        {1647, -50},
        {1738, -100},
};
#endif

static struct power_supply *charger_supply;
static bool is_jig_on;

static bool sec_bat_gpio_init(void)
{
	return true;
}

static bool sec_fg_gpio_init(void)
{
	return true;
}

static bool sec_chg_gpio_init(void)
{
	return true;
}

/* Get LP charging mode state */
unsigned int lpcharge;
static int battery_get_lpm_state(char *str)
{
	get_option(&str, &lpcharge);
	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("lpcharge=", battery_get_lpm_state);

static bool sec_bat_is_lpm(void)
{
	return lpcharge == 1 ? true : false;
}

void check_jig_status(int status)
{
	if (status) {
		pr_info("%s: JIG On so reset fuel gauge capacity\n", __func__);
		is_jig_on = true;
	}

}

static bool sec_bat_check_jig_status(void)
{
	return is_jig_on;
}

static int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

static int sec_bat_check_cable_callback(void)
{
	struct usb_gadget *gadget =
			platform_get_drvdata(&s3c_device_usbgadget);
	bool attach = true;
	int ta_nconnected;
	int adc = 0;

	if (!charger_supply) {
		charger_supply = power_supply_get_by_name("sec-charger");

		if (!charger_supply)
			pr_err("%s: failed to get power supplies\n", __func__);
	}

	/* delay 100ms */
	msleep(100);

	usb_switch_lock();
	usb_switch_set_path(USB_PATH_ADCCHECK);

	adc = stmpe811_get_adc_data(6);

	usb_switch_clr_path(USB_PATH_ADCCHECK);
	usb_switch_unlock();

	ta_nconnected = gpio_get_value(GPIO_TA_nCONNECTED);

	/* temp : set cable type always TA */
	current_cable_type = !ta_nconnected ?
				POWER_SUPPLY_TYPE_BATTERY :
				(adc > TA_ADC_LOW && adc < TA_ADC_HIGH) ?
				POWER_SUPPLY_TYPE_MAINS :
				POWER_SUPPLY_TYPE_USB;

	attach = ta_nconnected ? true : false;

	is_cable_attached = attach;

	/* temp code : only set vbus enable when usb attaced */
	if (gadget) {
		if (attach && current_cable_type == POWER_SUPPLY_TYPE_USB)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	} else {
		pr_err("%s: usb gadget drive not exist\n", __func__);
	}

	pr_info("%s: Cable type(%s), Attach(%d), Adc(%d)\n", __func__,
		current_cable_type == POWER_SUPPLY_TYPE_BATTERY ?
		"Battery" : current_cable_type == POWER_SUPPLY_TYPE_USB ?
		"USB" : "TA", attach, adc);

#if defined(CONFIG_TOUCHSCREEN_MMS152_TAB3) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
	tsp_charger_infom(attach);
#endif

	return current_cable_type;
}

static void sec_bat_initial_check(void)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct power_supply *chg_psy = power_supply_get_by_name("sec-charger");
	union power_supply_propval value;
	int ta_nconnected = 0;
	int ret = 0;

	value.intval = sec_bat_check_cable_callback();
	pr_info("%s: %d\n", __func__, value.intval);

	ta_nconnected = gpio_get_value(GPIO_TA_nCONNECTED);

	if (ta_nconnected &&
			value.intval != POWER_SUPPLY_TYPE_BATTERY) {
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
		if (ret)
			pr_err("%s: fail to set power_suppy ONLINE property\n",
					__func__);
	} else {
		if (chg_psy) {
			ret = chg_psy->set_property(chg_psy,
					POWER_SUPPLY_PROP_ONLINE, &value);
			if (ret)
				pr_err("%s: fail to set ONLINE property\n",
						__func__);
		}
	}
}

static bool sec_bat_check_cable_result_callback(int cable_type)
{
	bool ret = true;
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
		break;
	default:
		pr_err("%s cable type (%d)\n",
				__func__, cable_type);
		ret = false;
		break;
	}
	/* omap4_tab3_tsp_ta_detect(cable_type); */

	return ret;
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

static sec_bat_adc_region_t cable_adc_value_table[] = {
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_UNKNOWN */
	{ 0,    500 },  /* POWER_SUPPLY_TYPE_BATTERY */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_UPS */
	{ 1000, 1500 }, /* POWER_SUPPLY_TYPE_MAINS */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_USB */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_OTG */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_DOCK */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_MISC */
};

static sec_charging_current_t charging_current_table[] = {
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_UNKNOWN */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_BATTERY */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_UPS */
	{1800,  2000,   200,    0},     /* POWER_SUPPLY_TYPE_MAINS */
	{500,   500,    200,    0},     /* POWER_SUPPLY_TYPE_USB */
	{500,   500,    200,    0},     /* POWER_SUPPLY_TYPE_USB_DCP */
	{500,   500,    200,    0},     /* POWER_SUPPLY_TYPE_USB_CDP */
	{500,   500,    200,    0},     /* POWER_SUPPLY_TYPE_USB_ACA */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_OTG */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_DOCK */
	{500,   500,    200,    0},     /* POWER_SUPPLY_TYPE_MISC */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_WIRELESS */
};

/* unit: seconds */
static int polling_time_table[] = {
	10,     /* BASIC */
	30,     /* CHARGING */
	30,     /* DISCHARGING */
	30,     /* NOT_CHARGING */
	1800,    /* SLEEP */
};

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
                defined(CONFIG_TARGET_TAB3_WIFI10) ||\
                defined(CONFIG_TARGET_TAB3_LTE10)
static struct battery_data_t tab3_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x36B0,
		.low_battery_comp_voltage = 3600,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000, 0,      0},     /* dummy for top limit */
			{-1250, 0,      3320},
			{-750, 97,      3451},
			{-100, 96,      3461},
			{0, 0,  3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,    8950},
			{60000, 200,    51000},
			{100000, 0,     0},     /* dummy for top limit */
		},
		.type_str = "SDI",
	}
};
#else
static struct battery_data_t tab3_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x21FC,
		.low_battery_comp_voltage = 3600,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000, 0,      0},     /* dummy for top limit */
			{-1250, 0,      3320},
			{-750, 97,      3451},
			{-100, 96,      3461},
			{0, 0,  3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,    8950},
			{60000, 200,    51000},
			{100000, 0,     0},     /* dummy for top limit */
		},
		.type_str = "SDI",
	}
};
#endif

static bool sec_bat_adc_none_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_none_exit(void) { return true; }
static int sec_bat_adc_none_read(unsigned int channel) { return 0; }

static bool sec_bat_adc_ap_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ap_exit(void) { return true; }
static int sec_bat_adc_ap_read(unsigned int channel)
{
        int i;
        int ret = 0;

	ret = s3c_adc_read(temp_adc_client, 2);

	if (ret == -ETIMEDOUT) {
		for (i = 0; i < 5; i++) {
			msleep(20);
			ret = s3c_adc_read(temp_adc_client, 2);
			if (ret > 0)
				break;
		}

		if (i >= 5)
			pr_err("%s: Retry count exceeded\n", __func__);

	} else if (ret < 0) {
		pr_err("%s: Failed read adc value : %d\n",
		__func__, ret);
	}

out:
        pr_info("%s: temp acd : %d\n", __func__, ret);
        return ret;
}

static bool sec_bat_adc_ic_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ic_exit(void) { return true; }
static int sec_bat_adc_ic_read(unsigned int channel) { return 0; }

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

	.adc_check_count = 5,

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
                defined(CONFIG_TARGET_TAB3_WIFI10) ||\
                defined(CONFIG_TARGET_TAB3_LTE10)
	.adc_type = {
		SEC_BATTERY_ADC_TYPE_NONE,	/* CABLE_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,      /* BAT_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP_AMB */
		SEC_BATTERY_ADC_TYPE_NONE,      /* FULL_CHECK */
	},
#else
	 .adc_type = {
                SEC_BATTERY_ADC_TYPE_NONE,      /* CABLE_CHECK */
                SEC_BATTERY_ADC_TYPE_NONE,      /* BAT_CHECK */
                SEC_BATTERY_ADC_TYPE_AP,      	/* TEMP */
                SEC_BATTERY_ADC_TYPE_AP,      	/* TEMP_AMB */
                SEC_BATTERY_ADC_TYPE_NONE,      /* FULL_CHECK */
        },
#endif
	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.battery_data = (void *)tab3_battery_data,
	.bat_polarity_ta_nconnected = 1,        /* active HIGH */
	.bat_irq_attr = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE |
		SEC_BATTERY_CABLE_CHECK_INT,
	.cable_source_type = SEC_BATTERY_CABLE_SOURCE_CALLBACK,

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
	.ovp_uvlo_check_type = SEC_BATTERY_OVP_UVLO_CHGPOLLING,

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
                defined(CONFIG_TARGET_TAB3_WIFI10) ||\
                defined(CONFIG_TARGET_TAB3_LTE10)
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_FG,
	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,

        .temp_check_count = 1,
        .temp_high_threshold_event = 50000,
        .temp_high_recovery_event = 420,
        .temp_low_threshold_event = -50,
        .temp_low_recovery_event = 0,
        .temp_high_threshold_normal = 50000,
        .temp_high_recovery_normal = 420,
        .temp_low_threshold_normal = -50,
        .temp_low_recovery_normal = 0,
        .temp_high_threshold_lpm = 50000,
        .temp_high_recovery_lpm = 420,
        .temp_low_threshold_lpm = -50,
        .temp_low_recovery_lpm = 0,
#else
	/* Temperature check */
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_ADC,
        .temp_adc_table = tab3_temp_table,
        .temp_adc_table_size = sizeof(tab3_temp_table)
                                        /sizeof(sec_bat_adc_table_data_t),
        .temp_amb_adc_table = tab3_temp_table,
        .temp_amb_adc_table_size = sizeof(tab3_temp_table)
                                        /sizeof(sec_bat_adc_table_data_t),

	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,
	.temp_check_count = 1,
	.temp_high_threshold_event = 500,
	.temp_high_recovery_event = 420,
	.temp_low_threshold_event = -50,
	.temp_low_recovery_event = 0,
	.temp_high_threshold_normal = 500,
	.temp_high_recovery_normal = 420,
	.temp_low_threshold_normal = -50,
	.temp_low_recovery_normal = 0,
	.temp_high_threshold_lpm = 500,
	.temp_high_recovery_lpm = 420,
	.temp_low_threshold_lpm = -50,
	.temp_low_recovery_lpm = 0,
#endif
	.full_check_type = SEC_BATTERY_FULLCHARGED_CHGPSY,
	.full_check_count = 1,
	/* .full_check_adc_1st = 26500, */
	/*.full_check_adc_2nd = 25800, */
	.chg_polarity_full_check = 1,
	.full_condition_type =
		SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_OCV,
	.full_condition_soc = 99,
	.full_condition_ocv = 4170,

	.recharge_condition_type =
                SEC_BATTERY_RECHARGE_CONDITION_SOC |
                SEC_BATTERY_RECHARGE_CONDITION_VCELL,

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
                defined(CONFIG_TARGET_TAB3_WIFI10) ||\
                defined(CONFIG_TARGET_TAB3_LTE10)
	.recharge_condition_soc = 98,
	.recharge_condition_avgvcell = 4150,
	.recharge_condition_vcell = 4150,

	.charging_total_time = 10 * 60 * 60,
        .recharging_total_time = 90 * 60,
        .charging_reset_time = 10 * 60,
#else
	.recharge_condition_soc = 98,
        .recharge_condition_avgvcell = 4300,
        .recharge_condition_vcell = 4300,

	.charging_total_time = 6 * 60 * 60,
        .recharging_total_time = 90 * 60,
        .charging_reset_time = 10 * 60,
#endif

	/* Fuel Gauge */
	.fg_irq_attr = IRQF_TRIGGER_FALLING,
	.fuel_alert_soc = 1,
	.repeated_fuelalert = false,
	.capacity_calculation_type =
		SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE,
	.capacity_max = 1000,
	.capacity_min = 0,
	.capacity_max_margin = 30,

	/* Charger */
	.chg_polarity_en = 0,   /* active LOW charge enable */
	.chg_polarity_status = 0,
	.chg_irq_attr = IRQF_TRIGGER_RISING,

#if defined(CONFIG_TARGET_TAB3_3G10) ||\
                defined(CONFIG_TARGET_TAB3_WIFI10) ||\
                defined(CONFIG_TARGET_TAB3_LTE10)
	.chg_float_voltage = 4200,
#else
	.chg_float_voltage = 4350,
#endif
};

/* set NCP1851 Charger gpio i2c */
static struct i2c_gpio_platform_data tab3_gpio_i2c13_pdata = {
	.sda_pin = (GPIO_CHG_SDA),
	.scl_pin = (GPIO_CHG_SCL),
	.udelay = 10,
	.timeout = 0,
};

static struct platform_device tab3_gpio_i2c13_device = {
	.name = "i2c-gpio",
	.id = 13,
	.dev = {
		.platform_data = &tab3_gpio_i2c13_pdata,
	},
};

/* set MAX17050 Fuel Gauge gpio i2c */
static struct i2c_gpio_platform_data tab3_gpio_i2c14_pdata = {
	.sda_pin = (GPIO_FUEL_SDA),
	.scl_pin = (GPIO_FUEL_SCL),
	.udelay = 10,
	.timeout = 0,
};

static struct platform_device tab3_gpio_i2c14_device = {
	.name = "i2c-gpio",
	.id = 14,
	.dev = {
		.platform_data = &tab3_gpio_i2c14_pdata,
	},
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_battery_pdata,
};

static struct i2c_board_info sec_brdinfo_charger[] __initdata = {
	{
		I2C_BOARD_INFO("sec-charger",
				SEC_CHARGER_I2C_SLAVEADDR),
		.platform_data = &sec_battery_pdata,
	},
};

static struct i2c_board_info sec_brdinfo_fuelgauge[] __initdata = {
	{
		I2C_BOARD_INFO("sec-fuelgauge",
				SEC_FUELGAUGE_I2C_SLAVEADDR),
		.platform_data  = &sec_battery_pdata,
	},
};

static struct platform_device *sec_battery_devices[] __initdata = {
	&tab3_gpio_i2c13_device,
	&tab3_gpio_i2c14_device,
	&sec_device_battery,
};

static void charger_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TA_nCHG, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCHG, S3C_GPIO_PULL_NONE);

	sec_battery_pdata.bat_irq = gpio_to_irq(GPIO_TA_nCONNECTED);
}

void __init exynos_tab3_charger_init(void)
{
	pr_info("%s: tab3 charger init\n", __func__);
	charger_gpio_init();

	platform_add_devices(sec_battery_devices,
		ARRAY_SIZE(sec_battery_devices));

	i2c_register_board_info(SEC_CHARGER_I2C_ID, sec_brdinfo_charger,
			ARRAY_SIZE(sec_brdinfo_charger));

	i2c_register_board_info(SEC_FUELGAUGE_I2C_ID, sec_brdinfo_fuelgauge,
			ARRAY_SIZE(sec_brdinfo_fuelgauge));

	temp_adc_client = s3c_adc_register(&sec_device_battery, NULL, NULL, 0);
}
#endif
