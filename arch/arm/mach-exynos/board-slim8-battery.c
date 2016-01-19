/* arch/arm/mach-exynos/board-slim8-battery.c
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

#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/battery/charger/max77693_charger_bat.h>
#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <plat/gpio-cfg.h>
#include <mach/usb_switch.h>

#include "board-exynos4212.h"

#if defined(CONFIG_TOUCHSCREEN_MMS152_TAB3) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
#include <mach/tab3-input.h>
#endif

#include <linux/usb/android_composite.h>
#include <plat/devs.h>

#define SEC_CHARGER_I2C_ID	13
#define SEC_FUELGAUGE_I2C_ID	14

#define SEC_BATTERY_PMIC_NAME ""
/* cable state */
/* bool is_cable_attached; */

/* Get LP charging mode state */

int extended_cable_type;

unsigned int lpcharge;

static struct s3c_adc_client *temp_adc_client;
static struct power_supply *charger_supply;
static bool is_jig_on;

static sec_charging_current_t charging_current_table[] = {
        {0,     0,      0,      0},     	/* POWER_SUPPLY_TYPE_UNKNOWN */
        {0,     0,      0,      0},     	/* POWER_SUPPLY_TYPE_BATTERY */
        {0,     0,      0,      0},     	/* POWER_SUPPLY_TYPE_UPS */
        {1800,  1800,   250,    40*60},     	/* POWER_SUPPLY_TYPE_MAINS */
        {500,   500,    250,    40*60},     	/* POWER_SUPPLY_TYPE_USB */
        {500,   500,    250,    40*60},     	/* POWER_SUPPLY_TYPE_USB_DCP */
        {500,   500,    250,    40*60},     	/* POWER_SUPPLY_TYPE_USB_CDP */
        {500,   500,    250,    40*60},     	/* POWER_SUPPLY_TYPE_USB_ACA */
        {1800,  1800,   250,    40*60},     	/* POWER_SUPPLY_TYPE_MISC */
        {0,     0,      0,      0},     	/* POWER_SUPPLY_TYPE_CARDOCK */
        {500,   500,    250,    40*60},     	/* POWER_SUPPLY_TYPE_WIRELESS */
        {1800,  1800,   250,    40*60},     	/* POWER_SUPPLY_TYPE_UARTOFF */
	{500,   500,    250,    40*60},		/* POWER_SUPPLY_TYPE_OTG */
};

static bool sec_bat_gpio_init(void) 	{ return true; }
static bool sec_fg_gpio_init(void)	{ return true; }
static bool sec_chg_gpio_init(void)	{ return true; }

static int battery_get_lpm_state(char *str)
{
	if (strncmp(str, "charger", 7) == 0)
	lpcharge = 1;
	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("androidboot.mode=", battery_get_lpm_state);

/* For JB bootloader compatibility */
static int bootloader_get_lpm_state(char *str)
{
	if (strncmp(str, "1", 1) == 0)
		lpcharge = 1;

	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("lpcharge=", bootloader_get_lpm_state);

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
	return (current_cable_type == POWER_SUPPLY_TYPE_UARTOFF);
}

static bool isInitial = true;
static void sec_bat_initial_check(void)
{
	union power_supply_propval value;

        if (POWER_SUPPLY_TYPE_BATTERY < current_cable_type) {
                value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
                psy_do_property("battery", set,
                        POWER_SUPPLY_PROP_ONLINE, value);
        } else {
                psy_do_property("sec-charger", get,
                                POWER_SUPPLY_PROP_ONLINE, value);
                if (value.intval == POWER_SUPPLY_TYPE_WIRELESS) {
                        value.intval =
                                POWER_SUPPLY_TYPE_WIRELESS<<ONLINE_TYPE_MAIN_SHIFT;
                        psy_do_property("battery", set,
                                POWER_SUPPLY_PROP_ONLINE, value);
                }
        }
}

static int sec_bat_check_cable_callback(void)
{
        return current_cable_type;
}

static bool sec_bat_check_cable_result_callback(int cable_type)
{
        current_cable_type = cable_type;
        pr_info("%s : Cable Type(%d)\n", __func__, cable_type);
        return true;
}

static int sec_bat_get_cable_from_extended_cable_type(
        int input_extended_cable_type)
{
        int cable_main, cable_sub, cable_power;
        int cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
        union power_supply_propval value;
        int charge_current_max = 0, charge_current = 0;

        cable_main = GET_MAIN_CABLE_TYPE(input_extended_cable_type);
        if (cable_main != POWER_SUPPLY_TYPE_UNKNOWN)
                extended_cable_type = (extended_cable_type &
                        ~(int)ONLINE_TYPE_MAIN_MASK) |
                        (cable_main << ONLINE_TYPE_MAIN_SHIFT);
        cable_sub = GET_SUB_CABLE_TYPE(input_extended_cable_type);
        if (cable_sub != ONLINE_SUB_TYPE_UNKNOWN)
                extended_cable_type = (extended_cable_type &
                        ~(int)ONLINE_TYPE_SUB_MASK) |
                        (cable_sub << ONLINE_TYPE_SUB_SHIFT);
        cable_power = GET_POWER_CABLE_TYPE(input_extended_cable_type);
        if (cable_power != ONLINE_POWER_TYPE_UNKNOWN)
                extended_cable_type = (extended_cable_type &
                        ~(int)ONLINE_TYPE_PWR_MASK) |
                        (cable_power << ONLINE_TYPE_PWR_SHIFT);

        switch (cable_main) {
        case POWER_SUPPLY_TYPE_CARDOCK:
                switch (cable_power) {
                case ONLINE_POWER_TYPE_BATTERY:
                        cable_type = POWER_SUPPLY_TYPE_BATTERY;
                        break;
                case ONLINE_POWER_TYPE_TA:
                        switch (cable_sub) {
                        case ONLINE_SUB_TYPE_MHL:
                                cable_type = POWER_SUPPLY_TYPE_USB;
                                break;
                        case ONLINE_SUB_TYPE_AUDIO:
                        case ONLINE_SUB_TYPE_DESK:
                        case ONLINE_SUB_TYPE_SMART_NOTG:
                        case ONLINE_SUB_TYPE_KBD:
                                cable_type = POWER_SUPPLY_TYPE_MAINS;
                                break;
                        case ONLINE_SUB_TYPE_SMART_OTG:
                                cable_type = POWER_SUPPLY_TYPE_CARDOCK;
                                break;
                        }
                        break;
		case ONLINE_POWER_TYPE_USB:
                        cable_type = POWER_SUPPLY_TYPE_USB;
                        break;
                default:
                        cable_type = current_cable_type;
                        break;
                }
                break;
        case POWER_SUPPLY_TYPE_MISC:
                switch (cable_sub) {
                case ONLINE_SUB_TYPE_MHL:
                        switch (cable_power) {
                        case ONLINE_POWER_TYPE_BATTERY:
                                cable_type = POWER_SUPPLY_TYPE_BATTERY;
                                break;
                        case ONLINE_POWER_TYPE_MHL_500:
                                cable_type = POWER_SUPPLY_TYPE_MISC;
                                charge_current_max = 400;
                                charge_current = 400;
                                break;
                        case ONLINE_POWER_TYPE_MHL_900:
                                cable_type = POWER_SUPPLY_TYPE_MISC;
                                charge_current_max = 700;
                                charge_current = 700;
                                break;
                        case ONLINE_POWER_TYPE_MHL_1500:
                                cable_type = POWER_SUPPLY_TYPE_MISC;
                                charge_current_max = 1300;
                                charge_current = 1300;
                                break;
                        case ONLINE_POWER_TYPE_USB:
                                cable_type = POWER_SUPPLY_TYPE_USB;
                                charge_current_max = 300;
                                charge_current = 300;
                                break;
                        }
                        break;
                default:
                        cable_type = cable_main;
                        break;
                }
                break;
        default:
                cable_type = cable_main;
                break;
        }

        if (charge_current_max == 0) {
                charge_current_max = charging_current_table[cable_type].
					input_current_limit;
                charge_current = charging_current_table[cable_type].
                        		fast_charging_current;
        }
        value.intval = charge_current_max;
        psy_do_property("sec-charger", set,
                        POWER_SUPPLY_PROP_CURRENT_MAX, value);
        value.intval = charge_current;
        psy_do_property("sec-charger", set,
                        POWER_SUPPLY_PROP_CURRENT_AVG, value);

	pr_info("%s : cable(0x%02x) - main(0x%02x), sub(0x%02x), main(0x%02x)\n",
		__func__, input_extended_cable_type, cable_main, cable_sub, cable_power);
	pr_info("%s : cable(0x%02x) - input cur(%dmA), charge cur(%dmA)\n",
		__func__, cable_type, charge_current_max, charge_current);
        return cable_type;
}

static bool sec_bat_switch_to_check(void) {return true; }
static bool sec_bat_switch_to_normal(void) {return true; }

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

/* unit: seconds */
static int polling_time_table[] = {
	60,     /* BASIC */
	60,     /* CHARGING */
	60,     /* DISCHARGING */
	60,     /* NOT_CHARGING */
	1800,	/* SLEEP */
};

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

static bool sec_bat_adc_none_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_none_exit(void) { return true; }
static int sec_bat_adc_none_read(unsigned int channel) { return 0; }

static bool sec_bat_adc_ap_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ap_exit(void) { return true; }
static int sec_bat_adc_ap_read(unsigned int channel) { return 0; }

static bool sec_bat_adc_ic_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ic_exit(void) { return true; }
static int sec_bat_adc_ic_read(unsigned int channel) { return 0; }

sec_battery_platform_data_t sec_battery_pdata = {
	/* NO NEED TO BE CHANGED */
	.initial_check = sec_bat_initial_check,
	.bat_gpio_init = sec_bat_gpio_init,
	.fg_gpio_init = sec_fg_gpio_init,
	.chg_gpio_init = sec_chg_gpio_init,

	.is_lpm = sec_bat_is_lpm,
	.check_jig_status = sec_bat_check_jig_status,
	.check_cable_callback =				/* No Use */
		sec_bat_check_cable_callback,
	.check_cable_result_callback =			/* No Use */
		sec_bat_check_cable_result_callback,
	.get_cable_from_extended_cable_type =
                sec_bat_get_cable_from_extended_cable_type,
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

	 .adc_type = {
                SEC_BATTERY_ADC_TYPE_NONE,      /* CABLE_CHECK */
                SEC_BATTERY_ADC_TYPE_NONE,      /* BAT_CHECK */
                SEC_BATTERY_ADC_TYPE_NONE,     	/* TEMP */
                SEC_BATTERY_ADC_TYPE_NONE,  	/* TEMP_AMB */
                SEC_BATTERY_ADC_TYPE_NONE,      /* FULL_CHECK */
        },
	
	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.battery_data = (void *)tab3_battery_data,
	.bat_polarity_ta_nconnected = 1,        /* active HIGH */
	.bat_irq_attr = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE |
		SEC_BATTERY_CABLE_CHECK_INT,
	
	.cable_source_type =
		SEC_BATTERY_CABLE_SOURCE_EXTERNAL |
		SEC_BATTERY_CABLE_SOURCE_EXTENDED,

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

	/* Temperature check */
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_FG,
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
	
	.full_check_type = SEC_BATTERY_FULLCHARGED_CHGPSY,
	.full_check_type_2nd = SEC_BATTERY_FULLCHARGED_TIME,

	.full_check_count = 1,
	/* .full_check_adc_1st = 26500, */
	/*.full_check_adc_2nd = 25800, */
	.chg_polarity_full_check = 1,
	.full_condition_type =
		SEC_BATTERY_FULL_CONDITION_NOTIMEFULL |
		SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_VCELL,
	.full_condition_soc = 97,
	.full_condition_vcell = 4300,

	.recharge_check_count = 1,
	.recharge_condition_type =
                SEC_BATTERY_RECHARGE_CONDITION_SOC |
                SEC_BATTERY_RECHARGE_CONDITION_VCELL,

	.recharge_condition_soc = 98,
	.recharge_condition_avgvcell = 4300,
	.recharge_condition_vcell = 4300,

	.charging_total_time = 10 * 60 * 60,
        .recharging_total_time = 90 * 60,
        .charging_reset_time = 0,

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

	.chg_float_voltage = 4350,
};

/* set MAX17050 Fuel Gauge gpio i2c */
static struct i2c_gpio_platform_data tab3_gpio_i2c14_pdata = {
        .sda_pin = (GPIO_IF_FUEL_SDA),
        .scl_pin = (GPIO_IF_FUEL_SCL),
        .udelay = 10,
        .timeout = 0,
};

static struct platform_device sec_device_fuelgauge = {
        .name = "i2c-gpio",
        .id = SEC_FUELGAUGE_I2C_ID,
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
                I2C_BOARD_INFO("sec-chager",
                                MAX77693_CHARGER_I2C_ADDR),
                .platform_data  = &sec_battery_pdata,
        },
};

static struct i2c_board_info sec_brdinfo_fuelgauge[] __initdata = {
	{
		I2C_BOARD_INFO("sec-fuelgauge",
				MAX77693_FUELGAUGE_I2C_ADDR),
		.platform_data  = &sec_battery_pdata,
	},
};

static struct platform_device *sec_battery_devices[] __initdata = {
	&sec_device_fuelgauge,
	&sec_device_battery,
};

static void charger_gpio_init(void)
{
#if defined(CONFIG_TARGET_TAB3_3G8) || \
        defined(CONFIG_TARGET_TAB3_WIFI8) || \
        defined(CONFIG_TARGET_TAB3_LTE8)
	s3c_gpio_cfgpin(GPIO_FUEL_ALERT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_FUEL_ALERT, S3C_GPIO_PULL_NONE);
	sec_battery_pdata.fg_irq = gpio_to_irq(GPIO_FUEL_ALERT);
#endif
}

void __init exynos_slim8_charger_init(void)
{
	pr_info("%s: tab3 charger init\n", __func__);
	charger_gpio_init();

	platform_add_devices(sec_battery_devices,
		ARRAY_SIZE(sec_battery_devices));

	i2c_register_board_info(SEC_CHARGER_I2C_ID,
			sec_brdinfo_charger,
			ARRAY_SIZE(sec_brdinfo_charger));

	i2c_register_board_info(SEC_FUELGAUGE_I2C_ID,
                        sec_brdinfo_fuelgauge,
                        ARRAY_SIZE(sec_brdinfo_fuelgauge));

	temp_adc_client = s3c_adc_register(&sec_device_battery, NULL, NULL, 0);
}
