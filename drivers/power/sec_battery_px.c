/*
 *  sec_battery.c
 *  charger systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/android_alarm.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <linux/power/sec_battery_px.h>
#include <linux/power/max17042_fuelgauge_px.h>
#include <linux/mfd/max8997.h>
#if defined(CONFIG_TARGET_LOCALE_KOR)
#include <linux/proc_fs.h>
#endif /* CONFIG_TARGET_LOCALE_KOR */
#include <plat/adc.h>
#include <mach/usb_switch.h>

#define FAST_POLL	40	/* 40 sec */
#define SLOW_POLL	(30*60)	/* 30 min */

/* cut off voltage */
#define MAX_CUT_OFF_VOL	3500

/* SIOP */
#define CHARGING_CURRENT_HIGH_LOW_STANDARD	450
#define CHARGING_CURRENT_USB			500
#define SIOP_ACTIVE_CHARGE_CURRENT		450
#define SIOP_DEACTIVE_CHARGE_CURRENT		1500

enum {
	SIOP_DEACTIVE = 0,
	SIOP_ACTIVE,
};

#if defined(CONFIG_MACH_P2)
#define P2_CHARGING_FEATURE_02	/* SMB136 + MAX17042, Cable detect by TA_nCon */
#endif

#if defined(CONFIG_MACH_P4NOTE)
#define P4_CHARGING_FEATURE_01	/* SMB347 + MAX17042, use TA_nCON */
#endif

#if defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
#define P8_CHARGING_FEATURE_01   /* MAX8903 + MAX17042, use TA_nCON */
#endif

static char *supply_list[] = {
	"battery",
};

/* Get LP charging mode state */
unsigned int lpcharge;

static enum power_supply_property sec_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property sec_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	s32 batt_vol;		/* Battery voltage from ADC */
	s32 batt_temp;		/* Battery Temperature (C) from ADC */
	s32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 charging_current;	/* Charging current */
	u32 batt_full_count;	/* full checked count */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;       /* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	u32 batt_improper_ta;	/* 1: improper ta */
	u32 abstimer_is_active;	/* 0 : Not active 1: Active */
	u32 siop_activated;	/* 0 : Not active 1: Active */
#if defined(CONFIG_TARGET_LOCALE_KOR)
	u32 batt_vfsoc;
	u32 batt_soc;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	u32 batt_current_avg;
};

struct battery_data {
	struct device		*dev;
	struct sec_battery_platform_data *pdata;
	struct battery_info	info;
	struct power_supply	psy_battery;
	struct power_supply	psy_usb;
	struct power_supply	psy_ac;
	struct workqueue_struct	*sec_TA_workqueue;
	struct work_struct	battery_work;
	struct work_struct	cable_work;
	struct delayed_work	TA_work;
	struct delayed_work	fuelgauge_work;
	struct delayed_work	fuelgauge_recovery_work;
	struct delayed_work	fullcharging_work;
	struct delayed_work	full_comp_work;
	struct alarm		alarm;
	struct mutex		work_lock;
	struct wake_lock	vbus_wake_lock;
	struct wake_lock	cable_wake_lock;
	struct wake_lock	work_wake_lock;
	struct wake_lock	fullcharge_wake_lock;
	struct s3c_adc_client *padc;

#ifdef __TEST_DEVICE_DRIVER__
	struct wake_lock	wake_lock_for_dev;
#endif /* __TEST_DEVICE_DRIVER__ */
#if defined(CONFIG_TARGET_LOCALE_KOR)
	struct proc_dir_entry *entry;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	enum charger_type	current_cable_status;
	enum charger_type	previous_cable_status;
	int		present;
	unsigned int	device_state;
	unsigned int	charging_mode_booting;
	int		sec_battery_initial;
	int		low_batt_boot_flag;
	u32		full_charge_comp_recharge_info;
	unsigned long	charging_start_time;
	int		connect_irq;
	int		charging_irq;
	bool		slow_poll;
	ktime_t		last_poll;
	int fg_skip;
	int fg_skip_cnt;
	int fg_chk_cnt;
	int recharging_cnt;
	int previous_charging_status;
	int full_check_flag;
	bool is_first_check;
	/* 0:Default, 1:Only charger, 2:Only PMIC */
	int cable_detect_source;
	int pmic_cable_state;
	int dock_type;
	bool is_low_batt_alarm;
};

struct battery_data *debug_batterydata;

static void sec_cable_charging(struct battery_data *battery);
static void sec_set_chg_en(struct battery_data *battery, int enable);
static void sec_cable_changed(struct battery_data *battery);
static ssize_t sec_bat_show_property(struct device *dev,
			struct device_attribute *attr,
			char *buf);
static ssize_t sec_bat_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count);

#ifdef __TEST_DEVICE_DRIVER__
static ssize_t sec_batt_test_show_property(struct device *dev,
						struct device_attribute *attr,
						char *buf);
static ssize_t sec_batt_test_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count);

static int bat_temp_force_state;
#endif /* __TEST_DEVICE_DRIVER__ */

static bool check_UV_charging_case(void);
static void sec_bat_status_update(struct power_supply *bat_ps);
static void fullcharge_discharge_comp(struct battery_data *battery);

static int get_cached_charging_status(struct battery_data *battery)
{
	struct power_supply *psy = power_supply_get_by_name("max8997-charger");
	union power_supply_propval value;
	u32 ta_con = 0;

	if (!psy) {
		pr_err("%s: fail to get max8997-charger ps\n", __func__);
		return 0;
	}

	if (psy->get_property) {
		psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
		if (value.intval == POWER_SUPPLY_STATUS_CHARGING)
			ta_con = 1;
	}

	pr_info("PMIC detect : %s\n", ta_con ? "TA connected" :
						"TA NOT connected");
	return ta_con;
}

static int check_ta_conn(struct battery_data *battery)
{
	u32 value;

	if (battery->cable_detect_source == 2) {
		value = get_cached_charging_status(battery);
		pr_info("Cable detect from PMIC instead of TA_nConnected(%d)\n",
				value);
		return value;
	}
	value = gpio_get_value(battery->pdata->charger.connect_line);

#if defined(P4_CHARGING_FEATURE_01)
#if defined(CONFIG_MACH_P4NOTE)
	value = !value;
#else
	/* P4C H/W rev0.2, 0.3, 0.4 : High active */
	if ((system_rev >= 2) && (system_rev <= 5))
		value = !value;
#endif
#endif
	pr_debug("Charger detect : %s\n", value ? "TA NOT connected" :
							"TA connected");
	return !value;
}

#ifdef CONFIG_SAMSUNG_LPM_MODE
static void lpm_mode_check(struct battery_data *battery)
{
	battery->charging_mode_booting = lpcharge =
				battery->pdata->check_lp_charging_boot();
	pr_info("%s : charging_mode_booting(%d)\n", __func__,
			battery->charging_mode_booting);
}
#endif

#if defined(P4_CHARGING_FEATURE_01) || defined(P8_CHARGING_FEATURE_01)
#define CHARGING_CURRENT_HIGH	1500
#define CHARGING_CURRENT_LOW	CHARGING_CURRENT_USB
static void sec_set_charging(struct battery_data *battery, int charger_type)
{
	switch (charger_type) {
	case CHARGER_AC:
		gpio_set_value(battery->pdata->charger.currentset_line, 1);
		battery->info.charging_current = CHARGING_CURRENT_HIGH;
		break;
	default:
		/* For P8 Audio station, the charging current driven by TA
		path is not as per the required spec hence we drive the
		input charging current by USB path for Sound Station.*/
		gpio_set_value(battery->pdata->charger.currentset_line, 0);
		battery->info.charging_current = CHARGING_CURRENT_LOW;
		break;
	}
	pr_info("%s: Set charging current as %dmA.\n", __func__,
			battery->info.charging_current);
}
#endif

static void sec_battery_alarm(struct alarm *alarm)
{
	struct battery_data *battery =
			container_of(alarm, struct battery_data, alarm);

	pr_debug("%s : sec_battery_alarm.....\n", __func__);
	wake_lock(&battery->work_wake_lock);
	schedule_work(&battery->battery_work);
}

static void sec_program_alarm(struct battery_data *battery, int seconds)
{
	ktime_t low_interval = ktime_set(seconds - 10, 0);
	ktime_t slack = ktime_set(20, 0);
	ktime_t next;

	pr_debug("%s : sec_program_alarm.....\n", __func__);
	next = ktime_add(battery->last_poll, low_interval);
	alarm_start_range(&battery->alarm, next, ktime_add(next, slack));
}

static
enum charger_type sec_get_dedicted_charger_type(struct battery_data *battery)
{
	/* N.B. If check_ta_conn() is true something valid is
	connceted to the device for charging.
	By default this something is considered to be USB.*/
	enum charger_type result = CHARGER_USB;
	int avg_vol = 0;
	int adc_1, adc_2;
	int vol_1, vol_2;
	int accessory_line;

	mutex_lock(&battery->work_lock);

	/* ADC check margin (300~500ms) */
	msleep(300);

	usb_switch_lock();
	usb_switch_set_path(USB_PATH_ADCCHECK);

#if defined(CONFIG_STMPE811_ADC)
	vol_1 = stmpe811_get_adc_data(6);
	vol_2 = stmpe811_get_adc_data(6);
#else
	/* ADC values Update Margin */
	msleep(30);

	adc_1 = s3c_adc_read(battery->padc, 5);
	adc_2 = s3c_adc_read(battery->padc, 5);

	vol_1 = (adc_1 * 3300) / 4095;
	vol_2 = (adc_2 * 3300) / 4095;
#endif
	avg_vol = (vol_1 + vol_2)/2;

	if ((avg_vol > 800) && (avg_vol < 1800)) {
#if defined(P4_CHARGING_FEATURE_01)
		accessory_line = gpio_get_value(
			battery->pdata->charger.accessory_line);
		pr_info("%s: accessory line(%d)\n", __func__, accessory_line);

		if (battery->dock_type == DOCK_DESK) {
			pr_info("%s: deskdock(%d) charging\n",
					__func__, battery->dock_type);
			result = CHARGER_DOCK;
		} else {
			result = CHARGER_AC;
		}
#else
		result = CHARGER_AC;                      /* TA connected. */
#endif
	} else if ((avg_vol > 550) && (avg_vol < 700))
		result = CHARGER_MISC;  /* Samsung Audio Station connected.*/
	else
		result = CHARGER_USB;

	usb_switch_clr_path(USB_PATH_ADCCHECK);
	usb_switch_unlock();

	mutex_unlock(&battery->work_lock);

	pr_info("%s : result(%d), avg_vol(%d)\n", __func__, result, avg_vol);
	return result;
}

static void sec_get_cable_status(struct battery_data *battery)
{
	if (check_ta_conn(battery)) {
		battery->current_cable_status =
			sec_get_dedicted_charger_type(battery);
		battery->info.batt_full_count = 0;
	} else {
		battery->current_cable_status = CHARGER_BATTERY;
		battery->info.batt_improper_ta = 0;
	}

	if (battery->pdata->inform_charger_connection)
		battery->pdata->inform_charger_connection(
					battery->current_cable_status);

	pr_info("%s: current_status : %d\n",
		__func__, battery->current_cable_status);
}

/* Processes when the interrupt line is asserted. */
#define TA_DISCONNECT_RECHECK_TIME	300
static void sec_TA_work_handler(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, TA_work.work);
	int ta_state = 0;

	/* Prevent unstable VBUS signal from PC */
	ta_state =  check_ta_conn(battery);
	if (ta_state == 0) {
		/* Check for immediate discharge after fullcharge */
		fullcharge_discharge_comp(battery);

		msleep(TA_DISCONNECT_RECHECK_TIME);
		if (ta_state != check_ta_conn(battery)) {
			pr_info("%s: unstable ta_state(%d), ignore it.\n",
					__func__, ta_state);
			enable_irq(battery->connect_irq);
			return;
		}
	}
	pr_info("%s: ta_state(%d)\n", __func__, ta_state);

	sec_get_cable_status(battery);
	sec_cable_changed(battery);

	enable_irq(battery->connect_irq);
}

static irqreturn_t sec_TA_nCHG_interrupt_handler(int irq, void *arg)
{
	struct battery_data *battery = (struct battery_data *)arg;
	pr_info("%s(%d)\n", __func__,
		gpio_get_value(battery->pdata->charger.fullcharge_line));

	disable_irq_nosync(irq);
	cancel_delayed_work(&battery->fullcharging_work);
	schedule_delayed_work(&battery->fullcharging_work,
			msecs_to_jiffies(300));
	wake_lock_timeout(&battery->fullcharge_wake_lock, HZ * 30);

	return IRQ_HANDLED;
}

static irqreturn_t sec_TA_nCon_interrupt_handler(int irq, void *arg)
{
	struct battery_data *battery = (struct battery_data *)arg;
	pr_info("%s(%d)\n", __func__,
		gpio_get_value(battery->pdata->charger.connect_line));

	disable_irq_nosync(irq);
	cancel_delayed_work(&battery->TA_work);
	queue_delayed_work(battery->sec_TA_workqueue, &battery->TA_work, 10);
	wake_lock_timeout(&battery->cable_wake_lock, HZ * 2);

	return IRQ_HANDLED;
}

static u32 get_charger_status(struct battery_data *battery)
{
	if (check_ta_conn(battery)) {
		if (battery->current_cable_status == CHARGER_AC)
			pr_info("Charger Status : AC\n");
		else if (battery->current_cable_status == CHARGER_DOCK)
			pr_info("Charger Status : HDMI_DOCK\n");
		else if (battery->current_cable_status == CHARGER_MISC)
			pr_info("Charger Status : MISC\n");
		else
			pr_info("Charger Status : USB\n");
		return 1;
	} else {
		pr_info("Charger Status : Battery\n");
		return 0;
	}
}

static int is_over_abs_time(struct battery_data *battery)
{
	unsigned int total_time;
	ktime_t ktime;
	struct timespec cur_time;

	if (!battery->charging_start_time)
		return 0;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

	if (battery->info.batt_is_recharging)
		total_time = battery->pdata->recharge_duration;
	else
		total_time = battery->pdata->charge_duration;

	if (battery->charging_start_time + total_time < cur_time.tv_sec) {
		pr_info("Charging time out");
		return 1;
	} else
		return 0;
}

/* fullcharge_discharge_comp: During the small window between
 *  Full Charge and Recharge if the cable is connected , we always show
 *  100% SOC to the UI , evven though in background its discharging.
 * If the user pulls out the TA in between this small window he should
 * see a sudden drop from 100% to a disacharged state of 95% - 98%.
 * We fake the SOC on Fulll cap and let FG manage the differences on
 * long run.
 * Also during the start of recharging phase if the TA is disconnected SOC
 * can sudden;y drop down to a lower value. In that case also the user
 * expects to see a 100% so we update Full Cap again.
*/
static void fullcharge_discharge_comp(struct battery_data *battery)
{
	int fg_vcell = get_fuelgauge_value(FG_VOLTAGE);

	if (battery->info.batt_is_full) {
		pr_info("%s: Resetting FULLCAP !!\n", __func__);
		fg_reset_fullcap_in_fullcharge();
	}

	return;
}

static int sec_get_bat_level(struct power_supply *bat_ps)
{
	struct battery_data *battery = container_of(bat_ps,
				struct battery_data, psy_battery);
	int fg_soc;
	int fg_vfsoc;
	int fg_vcell;
	int fg_current;
	int avg_current;
	int recover_flag = 0;

	recover_flag = fg_check_cap_corruption();

	/* check VFcapacity every five minutes */
	if (!(battery->fg_chk_cnt++ % 10)) {
		fg_check_vf_fullcap_range();
		battery->fg_chk_cnt = 1;
	}

	fg_soc = get_fuelgauge_value(FG_LEVEL);
#if defined(CONFIG_TARGET_LOCALE_KOR)
	battery->info.batt_soc = fg_soc;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	if (fg_soc < 0) {
		pr_info("Can't read soc!!!");
		fg_soc = battery->info.level;
	}

	if (!battery->pdata->check_jig_status() && \
			!max17042_chip_data->info.low_batt_comp_flag) {
		if (((fg_soc+5) < max17042_chip_data->info.prev_repsoc) ||
			(fg_soc > (max17042_chip_data->info.prev_repsoc+5)))
			battery->fg_skip = 1;
	}

	/* skip one time (maximum 30 seconds) because of corruption. */
	if (battery->fg_skip) {
		pr_info("%s: skip update until corruption check "
			"is done (fg_skip_cnt:%d)\n",
			__func__, ++battery->fg_skip_cnt);
		fg_soc = battery->info.level;
		if (recover_flag || battery->fg_skip_cnt > 10) {
			battery->fg_skip = 0;
			battery->fg_skip_cnt = 0;
		}
	}

	if (battery->low_batt_boot_flag) {
		fg_soc = 0;

		if (check_ta_conn(battery) && !check_UV_charging_case()) {
			fg_adjust_capacity();
			battery->low_batt_boot_flag = 0;
		}

		if (!check_ta_conn(battery))
			battery->low_batt_boot_flag = 0;
	}

	fg_vcell = get_fuelgauge_value(FG_VOLTAGE);
	if (fg_vcell < 0) {
		pr_info("Can't read vcell!!!");
		fg_vcell = battery->info.batt_vol;
	} else
		battery->info.batt_vol = fg_vcell;

	fg_current = get_fuelgauge_value(FG_CURRENT);
	battery->info.batt_current = fg_current;
	avg_current = get_fuelgauge_value(FG_CURRENT_AVG);
	fg_vfsoc = get_fuelgauge_value(FG_VF_SOC);
#if defined(CONFIG_TARGET_LOCALE_KOR)
	battery->info.batt_vfsoc = fg_vfsoc;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	battery->info.batt_current_avg = avg_current;

/* P4-Creative does not set full flag by force */
#if !defined(CONFIG_MACH_P4NOTE)
	/* Algorithm for reducing time to fully charged (from MAXIM) */
	if (battery->info.charging_enabled &&	/* Charging is enabled */
		!battery->info.batt_is_recharging &&	/* Not Recharging */
		((battery->info.charging_source == CHARGER_AC) ||
		(battery->info.charging_source == CHARGER_MISC)) &&
		!battery->is_first_check &&	/* Skip the first check */
		(fg_vfsoc > 70 && (fg_current > 20 && fg_current < 250) &&
		(avg_current > 20 && avg_current < 260))) {

		if (battery->full_check_flag == 2) {
			pr_info("%s: force fully charged SOC !! (%d)", __func__,
					battery->full_check_flag);
			fg_set_full_charged();
			fg_soc = get_fuelgauge_value(FG_LEVEL);
		} else if (battery->full_check_flag < 2)
			pr_info("%s: full_check_flag (%d)", __func__,
					battery->full_check_flag);

		/* prevent overflow */
		if (battery->full_check_flag++ > 10000)
			battery->full_check_flag = 3;
	} else
		battery->full_check_flag = 0;
#endif

	if (battery->info.charging_source == CHARGER_AC &&
		battery->info.batt_improper_ta == 0) {
		if (is_over_abs_time(battery)) {
			battery->info.abstimer_is_active = 1;
			pr_info("%s: Charging time is over. Active abs timer.",
				__func__);
			pr_info(" VCELL(%d), SOC(%d)\n", fg_vcell, fg_soc);
			sec_set_chg_en(battery, 0);
			goto __end__;
		}
	} else {
		if (battery->info.abstimer_is_active) {
			battery->info.abstimer_is_active = 0;
			pr_info("%s: Inactive abs timer.\n", __func__);
		}
	}

	if ((fg_vcell <= battery->pdata->recharge_voltage) ||
		(fg_vcell <= 4000)) {
		if (battery->info.batt_is_full
			|| (battery->info.abstimer_is_active
			&& !battery->info.charging_enabled)) {
			if (++battery->recharging_cnt > 1) {
				pr_info("%s: Recharging start(%d ? %d).\n",
					__func__,
					fg_vcell,
					battery->pdata->recharge_voltage);
				battery->info.batt_is_recharging = 1;
				sec_set_chg_en(battery, 1);
				battery->recharging_cnt = 0;
			}
		} else
			battery->recharging_cnt = 0;
	} else
		battery->recharging_cnt = 0;

	if (fg_soc > 100)
		fg_soc = 100;

	/*  Checks vcell level and tries to compensate SOC if needed.*/
	/*  If jig cable is connected, then skip low batt compensation check. */
	if (!battery->pdata->check_jig_status() && \
			!battery->info.charging_enabled)
		fg_soc = low_batt_compensation(fg_soc, fg_vcell, fg_current);

	/* Compensation for Charging cut off current in P8.
	* On P8 we increase the charging current to 1.8A
	* to complete charging in spec time.
	* This causes the cut-off/ termination current to be high and
	* hence to set the charging cut -off current to expected value
	* we control it through ADC channel 6 of AP.
	* Due to some prob in HW sometimes the CURRENT_MEA goes
	* below V_TOP_OFF even for very low SOCs hence
	* check for CURRENT_MEA signal only at
	* a very later stage of charging process.
	* For now this code is enabled only for P8-LTE.
	* Enable it for other P8 Models only if HW is updated.
	*/
#ifdef CONFIG_MACH_P8LTE
	/* Charging is done using TA only. */
	/* TA should be a proper connection. */
	/* Charging should be Enabled. */
	/* Should not be in Recharging Path. */
	/* VCELL > 4.19V. */
	/* VFSOC > 72. 72 * 1.333 = 95.976 */
	if ((battery->info.charging_source == CHARGER_AC) &&
		(battery->info.batt_improper_ta == 0) &&
		(battery->info.charging_enabled) &&
		(battery->info.batt_is_recharging == 0) &&
		(fg_vcell > 4190) &&
		(fg_vfsoc > 72)) {
		/* Read ADC Voltage. */
		if (battery->padc) {
			int Viset = s3c_adc_read(battery->padc,
				SEC_CURR_MEA_ADC_CH);

			if (!Viset) {
				/* Connecting TA after boot shows latency
				* in ADC update for the first time.
				* Wait for ~700ms the ADC value to be updated.
				* This is just a conservative number.
				*/
				pr_info(
				"%s: Waiting ADC on Ch6 to be updated\n",
				__func__);
				mdelay(700);
				Viset = s3c_adc_read(battery->padc,
					SEC_CURR_MEA_ADC_CH);
			}

			if (Viset < V_TOP_OFF) {
				if (battery->info.batt_full_count++ <
					COUNT_TOP_OFF) {
					pr_info(
					"%s: Charging current < top-off\n",
					__func__);
				} else {
					/* Disable the charging process. */
					pr_err(
					"%s: Charging Disabled: V-topoff(%d)",
					__func__, V_TOP_OFF);
					pr_err(" is more than V-Iset(%d)\n",
					Viset);

					battery->info.batt_full_count = 0;
					sec_cable_charging(battery);
				}
			} else
				battery->info.batt_full_count = 0;
		} else {
			pr_err(
			"%s: Can't read the ADC regsiter\n",
			__func__);
		}
	}
#endif

__end__:
	pr_debug("fg_vcell = %d, fg_soc = %d, is_full = %d, full_count = %d",
		fg_vcell, fg_soc,
		battery->info.batt_is_full,
		battery->info.batt_full_count);

	if (battery->is_first_check)
		battery->is_first_check = false;

	if (battery->info.batt_is_full &&
		(battery->info.charging_source != CHARGER_USB))
		fg_soc = 100;

	return fg_soc;
}

static int sec_get_bat_vol(struct power_supply *bat_ps)
{
	struct battery_data *battery = container_of(bat_ps,
				struct battery_data, psy_battery);
	return battery->info.batt_vol;
}

static bool check_UV_charging_case(void)
{
	int fg_vcell = get_fuelgauge_value(FG_VOLTAGE);
	int fg_current = get_fuelgauge_value(FG_CURRENT);
	int threshold = 0;

	if (get_fuelgauge_value(FG_BATTERY_TYPE) == SDI_BATTERY_TYPE)
		threshold = 3300 + ((fg_current * 17) / 100);
	else if (get_fuelgauge_value(FG_BATTERY_TYPE) == ATL_BATTERY_TYPE)
		threshold = 3300 + ((fg_current * 13) / 100);

	if (fg_vcell <= threshold)
		return 1;
	else
		return 0;
}

static void sec_set_time_for_charging(struct battery_data *battery, int mode)
{
	if (mode) {
		ktime_t ktime;
		struct timespec cur_time;

		ktime = alarm_get_elapsed_realtime();
		cur_time = ktime_to_timespec(ktime);

		/* record start time for abs timer */
		battery->charging_start_time = cur_time.tv_sec;
	} else {
		/* initialize start time for abs timer */
		battery->charging_start_time = 0;
	}
}

static void disable_internal_charger(void)
{
	struct power_supply *internal_psy = \
				power_supply_get_by_name("max8997-charger");
	union power_supply_propval value;
	int ret;

	/* Disable inner charger(MAX8997) */
	if (internal_psy) {
		value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = internal_psy->set_property(internal_psy, \
					POWER_SUPPLY_PROP_STATUS, &value);
		if (ret)
			pr_err("%s: fail to set %s discharging status(%d)\n",
					__func__, internal_psy->name, ret);
	}
}
static void sec_set_chg_current(struct battery_data *battery, int set_current)
{
#if defined(P8_CHARGING_FEATURE_01)
	if ((set_current > CHARGING_CURRENT_HIGH_LOW_STANDARD) && \
			(battery->current_cable_status == CHARGER_AC))
		sec_set_charging(battery, CHARGER_AC);
	else if (battery->current_cable_status == CHARGER_MISC)
		sec_set_charging(battery, CHARGER_MISC);
	else
		sec_set_charging(battery, CHARGER_USB);
#else	/* P4C H/W rev0.0 does not support yet */
	if (battery->current_cable_status == CHARGER_AC) {
		if (battery->pdata->set_charging_current)
			battery->pdata->set_charging_current((int)set_current);

		if (battery->pdata->get_charging_current)
			battery->info.charging_current = \
					battery->pdata->get_charging_current();
	} else {
		battery->info.charging_current = CHARGING_CURRENT_USB;
	}
#endif
}

static void sec_set_chg_en(struct battery_data *battery, int enable)
{
#if defined(P2_CHARGING_FEATURE_02) || defined(P4_CHARGING_FEATURE_02)
	/* Disable internal charger(MAX8997) */
	/* because of using external charger */
	disable_internal_charger();

	if (battery->pdata->set_charging_state) {
		if (battery->current_cable_status == CHARGER_AC)
			battery->pdata->set_charging_state(enable,
						CABLE_TYPE_TA);
		else if (battery->current_cable_status == CHARGER_MISC)
			battery->pdata->set_charging_state(enable,
						CABLE_TYPE_STATION);
		else
			battery->pdata->set_charging_state(enable,
						CABLE_TYPE_USB);
	}

	if ((enable) || (battery->pdata->get_charging_current))
		battery->info.charging_current = \
				battery->pdata->get_charging_current();

	sec_set_time_for_charging(battery, enable);

	if (!enable)
		battery->info.batt_is_recharging = 0;

	pr_info("%s: External charger is %s", __func__, (enable ? "enabled." : \
								"disabled."));
#elif defined(P4_CHARGING_FEATURE_01)
	int charger_enable_line = battery->pdata->charger.enable_line;

	/* Disable internal charger(MAX8997) */
	/* because of using external charger */
	disable_internal_charger();

	/* In case of HDMI connecting, set charging current 1.5A */
#if defined(CONFIG_MACH_P4NOTE)
	if (battery->pdata->set_charging_state) {
		if (battery->current_cable_status == CHARGER_AC)
			battery->pdata->set_charging_state(enable,
			CABLE_TYPE_TA);
		else if (battery->current_cable_status == CHARGER_MISC)
			battery->pdata->set_charging_state(enable,
			CABLE_TYPE_STATION);
		else if (battery->current_cable_status == CHARGER_DOCK)
			battery->pdata->set_charging_state(enable,
			CABLE_TYPE_DESKDOCK);
		else
			battery->pdata->set_charging_state(enable,
			CABLE_TYPE_USB);
	}

	if (battery->pdata->get_charging_current)
		battery->info.charging_current = \
				battery->pdata->get_charging_current();

	sec_set_time_for_charging(battery, enable);

	if (!enable)
		battery->info.batt_is_recharging = 0;
#else
	if (system_rev >= 2) {
		if (battery->pdata->set_charging_state) {
			if (battery->current_cable_status == CHARGER_AC)
				battery->pdata->set_charging_state(enable,
				CABLE_TYPE_TA);
			else if (battery->current_cable_status == CHARGER_MISC)
				battery->pdata->set_charging_state(enable,
				CABLE_TYPE_STATION);
			else if (battery->current_cable_status == CHARGER_DOCK)
				battery->pdata->set_charging_state(enable,
				CABLE_TYPE_DESKDOCK);
			else
				battery->pdata->set_charging_state(enable,
				CABLE_TYPE_USB);
		}

		if (battery->pdata->get_charging_current)
			battery->info.charging_current = \
					battery->pdata->get_charging_current();

		sec_set_time_for_charging(battery, enable);

		if (!enable)
			battery->info.batt_is_recharging = 0;
	} else {
		if (enable) {
			sec_set_charging(battery,
					battery->current_cable_status);
			gpio_set_value(charger_enable_line, 0);

			pr_info("%s: Enabling external charger ", __func__);
			sec_set_time_for_charging(battery, 1);
		} else {
			gpio_set_value(charger_enable_line, 1);
			pr_info("%s: Disabling external charger ", __func__);

			sec_set_time_for_charging(battery, 0);
			battery->info.batt_is_recharging = 0;
		}
	}
#endif
#else
	int charger_enable_line = battery->pdata->charger.enable_line;

	/* Disable internal charger(MAX8997) */
	/* because of using external charger */
	disable_internal_charger();

	if (enable) {
		sec_set_charging(battery, battery->current_cable_status);
		gpio_set_value(charger_enable_line, 0);

		pr_info("%s: Enabling the external charger ", __func__);
		sec_set_time_for_charging(battery, 1);
	} else {
		gpio_set_value(charger_enable_line, 1);
		pr_info("%s: Disabling the external charger ", __func__);

		sec_set_time_for_charging(battery, 0);
		battery->info.batt_is_recharging = 0;
	}
#endif
	battery->info.charging_enabled = enable;
}

static void sec_temp_control(struct battery_data *battery, int mode)
{
	switch (mode) {
	case POWER_SUPPLY_HEALTH_GOOD:
		pr_debug("GOOD");
		battery->info.batt_health = mode;
		break;
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		pr_debug("OVERHEAT");
		battery->info.batt_health = mode;
		break;
	case POWER_SUPPLY_HEALTH_COLD:
		pr_debug("FREEZE");
		battery->info.batt_health = mode;
		break;
	default:
		break;
	}
	schedule_work(&battery->cable_work);
}

static int sec_get_bat_temp(struct power_supply *bat_ps)
{
	struct battery_data *battery = container_of(bat_ps,
				struct battery_data, psy_battery);
	int health = battery->info.batt_health;
	int update = 0;
	int battery_temp;
	int temp_high_threshold;
	int temp_high_recovery;
	int temp_low_threshold;
	int temp_low_recovery;

	battery_temp = get_fuelgauge_value(FG_TEMPERATURE);

	temp_high_threshold = battery->pdata->temp_high_threshold;
	temp_high_recovery = battery->pdata->temp_high_recovery;
	temp_low_threshold = battery->pdata->temp_low_threshold;
	temp_low_recovery = battery->pdata->temp_low_recovery;

#ifdef __TEST_DEVICE_DRIVER__
	switch (bat_temp_force_state) {
	case 0:
		break;
	case 1:
		battery_temp = temp_high_threshold + 1;
		break;
	case 2:
		battery_temp = temp_low_threshold - 1;
		break;
	default:
		break;
	}
#endif /* __TEST_DEVICE_DRIVER__ */

	if (battery_temp >= temp_high_threshold) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
			pr_info("battery overheat(%d>=%d),charging unavailable",
				battery_temp, temp_high_threshold);
			sec_temp_control(battery, POWER_SUPPLY_HEALTH_OVERHEAT);
			update = 1;
		}
	} else if (battery_temp <= temp_high_recovery &&
			battery_temp >= temp_low_recovery) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				health == POWER_SUPPLY_HEALTH_COLD) {
			pr_info("battery recovery(%d,%d~%d),charging available",
				battery_temp, temp_low_recovery,
				temp_high_recovery);
			sec_temp_control(battery, POWER_SUPPLY_HEALTH_GOOD);
			update = 1;
		}
	} else if (battery_temp <= temp_low_threshold) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
			pr_info("battery cold(%d <= %d), charging unavailable.",
				battery_temp, temp_low_threshold);
			sec_temp_control(battery, POWER_SUPPLY_HEALTH_COLD);
			update = 1;
		}
	}

	if (update)
		pr_info("sec_get_bat_temp = %d.", battery_temp);

	return battery_temp/100;
}

static int sec_bat_get_charging_status(struct battery_data *battery)
{
	switch (battery->info.charging_source) {
	case CHARGER_BATTERY:
	//case CHARGER_USB:
		return POWER_SUPPLY_STATUS_DISCHARGING;
	case CHARGER_AC:
	case CHARGER_MISC:
	case CHARGER_DOCK:
	case CHARGER_USB:
		if (battery->info.batt_is_full)
			return POWER_SUPPLY_STATUS_FULL;
		else if (battery->info.batt_improper_ta)
			return POWER_SUPPLY_STATUS_DISCHARGING;
		else
			return POWER_SUPPLY_STATUS_CHARGING;
	case CHARGER_DISCHARGE:
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	default:
		return POWER_SUPPLY_STATUS_UNKNOWN;
	}
}

static int sec_bat_set_property(struct power_supply *ps,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct battery_data *battery = container_of(ps,
				struct battery_data, psy_battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
#if defined(CONFIG_MACH_P4NOTE)
		battery->dock_type = val->intval;
		pr_info("dock type(%d)\n", battery->dock_type);
		/* if need current update state, set */
		if (battery->dock_type == DOCK_DESK) {
			sec_get_cable_status(battery);
			sec_cable_changed(battery);
		}
#else
		battery->pmic_cable_state = val->intval;
		pr_info("PMIC cable state: %d\n", battery->pmic_cable_state);
		if (battery->cable_detect_source == 2) {
			/* cable is attached or detached.
			   called by USB switch(MUIC) */
			sec_get_cable_status(battery);
			sec_cable_changed(battery);
		}
		battery->cable_detect_source = 0;
#endif
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_bat_get_property(struct power_supply *bat_ps,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct battery_data *battery = container_of(bat_ps,
				struct battery_data, psy_battery);

	pr_debug("psp = %d", psp);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = sec_bat_get_charging_status(battery);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = battery->info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery->present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = battery->info.batt_current;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = battery->info.batt_current_avg;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
#if defined(CONFIG_MACH_P4NOTE)
		if ((battery->info.level == 0) &&
			(battery->info.batt_vol > MAX_CUT_OFF_VOL)) {
			pr_info("%s: mismatch power off soc(%d) and vol(%d)\n",
			__func__, battery->info.level, battery->info.batt_vol);
			val->intval = battery->info.level = 1;
		} else {
			val->intval = battery->info.level;
		}
#else
		val->intval = battery->info.level;
#endif
		pr_info("level = %d\n", val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = battery->info.batt_temp;
		pr_debug("temp = %d\n", val->intval);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_usb_get_property(struct power_supply *usb_ps,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct battery_data *battery = container_of(usb_ps,
				struct battery_data, psy_usb);

	enum charger_type charger = battery->info.charging_source;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (charger == CHARGER_USB ? 1 : 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_ac_get_property(struct power_supply *ac_ps,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct battery_data *battery = container_of(ac_ps,
				struct battery_data, psy_ac);

	enum charger_type charger = battery->info.charging_source;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((charger == CHARGER_AC) || (charger == CHARGER_MISC)
			|| (charger == CHARGER_DOCK))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define SEC_BATTERY_ATTR(_name)\
{\
	.attr = { .name = #_name, .mode = S_IRUGO | (S_IWUSR | S_IWGRP) },\
	.show = sec_bat_show_property,\
	.store = sec_bat_store,\
}

static struct device_attribute sec_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_vol),
	SEC_BATTERY_ATTR(temp),
#ifdef CONFIG_MACH_SAMSUNG_P5
	SEC_BATTERY_ATTR(batt_temp_cels),
#endif
	SEC_BATTERY_ATTR(batt_charging_source),
	SEC_BATTERY_ATTR(fg_soc),
#if defined(CONFIG_MACH_P4NOTE)
	SEC_BATTERY_ATTR(batt_reset_soc),
#else
	SEC_BATTERY_ATTR(reset_soc),
#endif
	SEC_BATTERY_ATTR(fg_reset_cap),
	SEC_BATTERY_ATTR(fg_reg),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_full_check),
	SEC_BATTERY_ATTR(batt_current_now),
	SEC_BATTERY_ATTR(siop_activated),
#ifdef CONFIG_SAMSUNG_LPM_MODE
	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(voltage_now),
#endif
	SEC_BATTERY_ATTR(jig_on),
	SEC_BATTERY_ATTR(fg_capacity),
	SEC_BATTERY_ATTR(update),
#if defined(CONFIG_TARGET_LOCALE_KOR)
	SEC_BATTERY_ATTR(batt_sysrev),
	SEC_BATTERY_ATTR(batt_temp_adc_spec),
	SEC_BATTERY_ATTR(batt_current_adc),
	SEC_BATTERY_ATTR(batt_current_avg),
#endif /* CONFIG_TARGET_LOCALE_KOR */
};

enum {
	BATT_VOL = 0,
	BATT_TEMP,
#ifdef CONFIG_MACH_SAMSUNG_P5
	BATT_TEMP_CELS,
#endif
	BATT_CHARGING_SOURCE,
	BATT_FG_SOC,
	BATT_RESET_SOC,
	BATT_RESET_CAP,
	BATT_FG_REG,
	BATT_BATT_TYPE,
	BATT_TEMP_CHECK,
	BATT_FULL_CHECK,
	BATT_CURRENT_NOW,
	SIOP_ACTIVATED,
#ifdef CONFIG_SAMSUNG_LPM_MODE
	BATT_LP_CHARGING,
	VOLTAGE_NOW,
#endif
	JIG_ON,
	FG_CAPACITY,
	UPDATE,
#if defined(CONFIG_TARGET_LOCALE_KOR)
	BATT_SYSTEM_REVISION,
	BATT_TEMP_ADC_SPEC,
	BATT_CURRENT_ADC,
	BATT_CURRENT_AVG,
#endif /* CONFIG_TARGET_LOCALE_KOR */
};

static int sec_bat_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
		rc = device_create_file(dev, &sec_battery_attrs[i]);
		if (rc)
			goto sec_attrs_failed;
	}
	return 0;

sec_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_battery_attrs[i]);
	return rc;
}

static ssize_t sec_bat_show_property(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	int i = 0;
#ifdef CONFIG_MACH_SAMSUNG_P5
	s32 temp = 0;
#endif
	u8 batt_str[5];
	const ptrdiff_t off = attr - sec_battery_attrs;

	switch (off) {
	case BATT_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		get_fuelgauge_value(FG_VOLTAGE));
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.batt_temp);
		break;
#ifdef CONFIG_MACH_SAMSUNG_P5
	case BATT_TEMP_CELS:
			temp = ((debug_batterydata->info.batt_temp) / 10);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			temp);
			break;
#endif
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.charging_source);
		break;
	case BATT_FG_SOC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		get_fuelgauge_value(FG_LEVEL));
		break;
	case BATT_BATT_TYPE:
		if (get_fuelgauge_value(FG_BATTERY_TYPE) == SDI_BATTERY_TYPE)
			sprintf(batt_str, "SDI");
		else if (get_fuelgauge_value(FG_BATTERY_TYPE) ==
			ATL_BATTERY_TYPE)
			sprintf(batt_str, "ATL");
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s_%s\n",
		batt_str, batt_str);
		break;
	case BATT_TEMP_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.batt_health);
		break;
	case BATT_FULL_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.batt_is_full);
		break;
	case BATT_CURRENT_NOW:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.charging_current);
		break;
	case SIOP_ACTIVATED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		debug_batterydata->info.siop_activated);
		break;
#ifdef CONFIG_SAMSUNG_LPM_MODE
	case BATT_LP_CHARGING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			debug_batterydata->charging_mode_booting);
		break;
	case VOLTAGE_NOW:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			get_fuelgauge_value(FG_VOLTAGE_NOW));
		break;
#endif
	case JIG_ON:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			debug_batterydata->pdata->check_jig_status());
		break;
	case FG_CAPACITY:
		i += scnprintf(buf + i, PAGE_SIZE - i,
				"0x%04x 0x%04x 0x%04x 0x%04x\n",
				get_fuelgauge_capacity(CAPACITY_TYPE_FULL),
				get_fuelgauge_capacity(CAPACITY_TYPE_MIX),
				get_fuelgauge_capacity(CAPACITY_TYPE_AV),
				get_fuelgauge_capacity(CAPACITY_TYPE_REP));
		break;
#if defined(CONFIG_TARGET_LOCALE_KOR)
	case BATT_SYSTEM_REVISION:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			system_rev);
		break;
	case BATT_TEMP_ADC_SPEC:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"(HIGH: %d / %d,\tLOW: %d / %d)\n",
			debug_batterydata->pdata->temp_high_threshold / 100,
			debug_batterydata->pdata->temp_high_recovery / 100,
			debug_batterydata->pdata->temp_low_threshold / 100,
			debug_batterydata->pdata->temp_low_recovery / 100);
		break;
	case BATT_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			get_fuelgauge_value(FG_CURRENT));
		break;
	case BATT_CURRENT_AVG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			get_fuelgauge_value(FG_CURRENT_AVG));
		break;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - sec_battery_attrs;

	switch (off) {
	case BATT_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1) {
				if (!check_ta_conn(debug_batterydata))
					fg_reset_soc();
			}
			ret = count;
		}
		sec_bat_status_update(&debug_batterydata->psy_battery);
		pr_debug("Reset SOC:%d ", x);
		break;
	case BATT_RESET_CAP:
		if (sscanf(buf, "%d\n", &x) == 1 ||
			x == 2 || x == 3 || x == 4) {
			if (x == 1 || x == 2 ||
				x == 3 || x == 4)
				fg_reset_capacity();
			ret = count;
		}
		pr_debug("%s: Reset CAP:%d\n", __func__, x);
		break;
	case BATT_FG_REG:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1)
				fg_periodic_read();
			ret = count;
		}
		pr_debug("%s: FG Register:%d\n", __func__, x);
		break;
	case BATT_CURRENT_NOW:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_set_chg_current(debug_batterydata, (int)x);
			ret = count;
		}
		pr_debug("%s: BATT_CURRENT_NOW :%d\n", __func__, x);
		break;
	case SIOP_ACTIVATED:
		if (sscanf(buf, "%d\n", &x) == 1) {
			debug_batterydata->info.siop_activated = (int)x;
			if (debug_batterydata->info.siop_activated ==
								SIOP_ACTIVE)
				sec_set_chg_current(debug_batterydata,
						SIOP_ACTIVE_CHARGE_CURRENT);
			else
				sec_set_chg_current(debug_batterydata,
						SIOP_DEACTIVE_CHARGE_CURRENT);
			ret = count;
		}
		pr_info("%s: SIOP_ACTIVATED :%d\n", __func__, x);
		break;
	case UPDATE:
		pr_info("%s: battery update\n", __func__);
		wake_lock(&debug_batterydata->work_wake_lock);
		schedule_work(&debug_batterydata->battery_work);
		ret = count;
		break;
#ifdef CONFIG_SAMSUNG_LPM_MODE
	case BATT_LP_CHARGING:
		if (sscanf(buf, "%d\n", &x) == 1) {
			debug_batterydata->charging_mode_booting = x;
			ret = count;
		}
		break;
#endif
	default:
		ret = -EINVAL;
	}
	return ret;
}

#ifdef __TEST_DEVICE_DRIVER__
#define SEC_TEST_ATTR(_name)\
{\
	.attr = { .name = #_name, .mode = S_IRUGO | (S_IWUSR | S_IWGRP) },\
	.show = sec_batt_test_show_property,\
	.store = sec_batt_test_store,\
}

static struct device_attribute sec_batt_test_attrs[] = {
	SEC_TEST_ATTR(suspend_lock),
	SEC_TEST_ATTR(control_temp),
};

enum {
	SUSPEND_LOCK = 0,
	CTRL_TEMP,
};

static int sec_batt_test_create_attrs(struct device *dev)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(sec_batt_test_attrs); i++) {
		ret = device_create_file(dev, &sec_batt_test_attrs[i]);
		if (ret)
			goto sec_attrs_failed;
	}
	goto succeed;

sec_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_batt_test_attrs[i]);
succeed:
	return ret;
}

static ssize_t sec_batt_test_show_property(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - sec_batt_test_attrs;

	switch (off) {
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_batt_test_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int mode = 0;
	int ret = 0;
	const ptrdiff_t off = attr - sec_batt_test_attrs;

	switch (off) {
	case SUSPEND_LOCK:
		if (sscanf(buf, "%d\n", &mode) != 1)
			break;

		dev_dbg(dev, "%s: suspend lock (%d)\n", __func__, mode);
		if (mode)
			wake_lock(&debug_batterydata->wake_lock_for_dev);
		else
			wake_lock_timeout(&debug_batterydata->wake_lock_for_dev,
					HZ / 2);
		ret = count;
		break;

	case CTRL_TEMP:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			dev_info(dev, "%s: control temp (%d)\n", __func__,
					mode);
			bat_temp_force_state = mode;
			ret = count;
		}
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}
#endif /* __TEST_DEVICE_DRIVER__ */

int check_usb_status;
static int sec_cable_status_update(struct battery_data *battery, int status)
{
	int ret = 0;
	enum charger_type source = CHARGER_BATTERY;

	pr_debug("Update cable status ");

	if (!battery->sec_battery_initial)
		return -EPERM;

	switch (status) {
	case CHARGER_BATTERY:
		pr_info("cable NOT PRESENT ");
		battery->info.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		pr_info("cable USB");
		battery->info.charging_source = CHARGER_USB;
		break;
	case CHARGER_AC:
		pr_info("cable AC");
		battery->info.charging_source = CHARGER_AC;
		break;
	case CHARGER_DOCK:
		pr_info("cable DOCK");
		battery->info.charging_source = CHARGER_DOCK;
		break;
	case CHARGER_MISC:
		pr_info("cable MISC");
		battery->info.charging_source = CHARGER_AC;
#if defined(CONFIG_MACH_P8LTE)  || defined(CONFIG_MACH_P8)
		battery->info.charging_source = CHARGER_MISC;
#endif
		break;
	case CHARGER_DISCHARGE:
		pr_info("Discharge");
		battery->info.charging_source = CHARGER_DISCHARGE;
		break;
	default:
		pr_info("Not supported status");
		ret = -EINVAL;
	}
	check_usb_status = source = battery->info.charging_source;

	if (source == CHARGER_USB || source == CHARGER_AC || \
		source == CHARGER_MISC  || source == CHARGER_DOCK) {
		wake_lock(&battery->vbus_wake_lock);
	} else {
		/* give userspace some time to see the uevent and update
		* LED state or whatnot...
		*/
		if (!get_charger_status(battery)) {
			if (battery->charging_mode_booting)
				wake_lock_timeout(&battery->vbus_wake_lock,
						5 * HZ);
			else
				wake_lock_timeout(&battery->vbus_wake_lock,
						HZ / 2);
		}
	}

	wake_lock(&battery->work_wake_lock);
	schedule_work(&battery->battery_work);

	return ret;
}

static void sec_bat_status_update(struct power_supply *bat_ps)
{
	struct battery_data *battery = container_of(bat_ps,
				struct battery_data, psy_battery);

	int old_level, old_temp, old_health, old_is_full;

	if (!battery->sec_battery_initial)
		return;

	mutex_lock(&battery->work_lock);
	old_temp = battery->info.batt_temp;
	old_health = battery->info.batt_health;
	old_level = battery->info.level;
	old_is_full = battery->info.batt_is_full;

	battery->info.batt_temp = sec_get_bat_temp(bat_ps);
	if (!battery->is_low_batt_alarm)
		battery->info.level = sec_get_bat_level(bat_ps);

	if (!battery->info.charging_enabled &&
			!battery->info.batt_is_full &&
			!battery->pdata->check_jig_status()) {
		if (battery->info.level > old_level)
			battery->info.level = old_level;
	}

	battery->info.batt_vol = sec_get_bat_vol(bat_ps);

	if (battery->pdata->get_charging_state)
		battery->pdata->get_charging_state();

	power_supply_changed(bat_ps);
	pr_debug("call power_supply_changed");

	pr_info("BAT : soc(%d), vcell(%dmV), curr(%dmA), "
		"avg curr(%dmA), temp(%d.%d), chg(%d)",
		battery->info.level,
		battery->info.batt_vol,
		battery->info.batt_current,
		battery->info.batt_current_avg,
		battery->info.batt_temp/10,
		battery->info.batt_temp%10,
		battery->info.charging_enabled);
	pr_info(", full(%d), rechg(%d), lowbat(%d), cable(%d)\n",
		battery->info.batt_is_full,
		battery->info.batt_is_recharging,
		battery->is_low_batt_alarm,
		battery->current_cable_status);

	mutex_unlock(&battery->work_lock);
}

static void sec_cable_check_status(struct battery_data *battery)
{
	enum charger_type status = 0;

	mutex_lock(&battery->work_lock);

	if (get_charger_status(battery)) {
		pr_info("%s: Returning to Normal discharge path.\n",
			__func__);
		cancel_delayed_work(&battery->fuelgauge_recovery_work);
		battery->is_low_batt_alarm = false;

		if (battery->info.batt_health != POWER_SUPPLY_HEALTH_GOOD) {
			pr_info("Unhealth battery state! ");
			status = CHARGER_DISCHARGE;
			sec_set_chg_en(battery, 0);
			goto __end__;
		}

		status = battery->current_cable_status;

		sec_set_chg_en(battery, 1);

		max17042_chip_data->info.low_batt_comp_flag = 0;
		reset_low_batt_comp_cnt();

	} else {
		status = CHARGER_BATTERY;
		sec_set_chg_en(battery, 0);
	}
__end__:
	sec_cable_status_update(battery, status);
	mutex_unlock(&battery->work_lock);
}

static void sec_bat_work(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, battery_work);
	unsigned long flags;
	pr_debug("%s\n", __func__);

	sec_bat_status_update(&battery->psy_battery);
	battery->last_poll = alarm_get_elapsed_realtime();

	/* prevent suspend before starting the alarm */
	local_irq_save(flags);
	wake_unlock(&battery->work_wake_lock);
	sec_program_alarm(battery, FAST_POLL);
	local_irq_restore(flags);
}

static void sec_cable_work(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, cable_work);
	pr_debug("%s\n", __func__);

	sec_cable_check_status(battery);
}

static int sec_bat_suspend(struct device *dev)
{
	struct battery_data *battery = dev_get_drvdata(dev);
	pr_info("%s start\n", __func__);

	if (!get_charger_status(battery)) {
		sec_program_alarm(battery, SLOW_POLL);
		battery->slow_poll = 1;
	}

	pr_info("%s end\n", __func__);
	return 0;
}

static void sec_bat_resume(struct device *dev)
{
	struct battery_data *battery = dev_get_drvdata(dev);
	pr_info("%s start\n", __func__);

	irq_set_irq_type(battery->charging_irq
		, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING);

	if (battery->slow_poll) {
		sec_program_alarm(battery, FAST_POLL);
		battery->slow_poll = 0;
	}

	wake_lock(&battery->work_wake_lock);
	schedule_work(&battery->battery_work);

	pr_info("%s end\n", __func__);
}

static void sec_cable_changed(struct battery_data *battery)
{
	pr_debug("charger changed ");

	if (!battery->sec_battery_initial)
		return;

	if (!battery->charging_mode_booting)
		battery->info.batt_is_full = 0;

	battery->info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

	schedule_work(&battery->cable_work);

	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */

	battery->last_poll = alarm_get_elapsed_realtime();
	sec_program_alarm(battery, FAST_POLL);
}

void sec_cable_charging(struct battery_data *battery)
{
	battery->full_charge_comp_recharge_info =
		battery->info.batt_is_recharging;

	if (!battery->sec_battery_initial)
		return;

	if (battery->info.charging_enabled &&
		battery->info.batt_health == POWER_SUPPLY_HEALTH_GOOD) {
		sec_set_chg_en(battery, 0);
		battery->info.batt_is_full = 1;
		/* full charge compensation algorithm by MAXIM */
		fg_fullcharged_compensation(
			battery->full_charge_comp_recharge_info, 1);

		cancel_delayed_work(&battery->full_comp_work);
		schedule_delayed_work(&battery->full_comp_work, 100);

		pr_info("battery is full charged ");
	}

	wake_lock(&battery->work_wake_lock);
	schedule_work(&battery->battery_work);
}

static irqreturn_t low_battery_isr(int irq, void *arg)
{
	struct battery_data *battery = (struct battery_data *)arg;
	pr_debug("%s(%d)", __func__,
		gpio_get_value(max17042_chip_data->pdata->fuel_alert_line));

	cancel_delayed_work(&battery->fuelgauge_work);
	schedule_delayed_work(&battery->fuelgauge_work, 0);

	return IRQ_HANDLED;
}

void fuelgauge_recovery_handler(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data,
		fuelgauge_recovery_work.work);
	int current_soc;

	if (battery->info.level > 0) {
		pr_err("%s: Reduce the Reported SOC by 1 unit, wait for 30s\n",
								__func__);
		if (!battery->info.charging_enabled)
			wake_lock_timeout(&battery->vbus_wake_lock, HZ);
		current_soc = get_fuelgauge_value(FG_LEVEL);
		if (current_soc) {
			pr_info("%s: Returning to Normal discharge path.\n",
				__func__);
			pr_info(" Actual SOC(%d) non-zero.\n", current_soc);
			battery->is_low_batt_alarm = false;
		} else {
			battery->info.level--;
			pr_err("%s: New Reduced Reported SOC  (%d).\n",
				__func__, battery->info.level);
			power_supply_changed(&battery->psy_battery);
			queue_delayed_work(battery->sec_TA_workqueue,
				&battery->fuelgauge_recovery_work,
				msecs_to_jiffies(30000));
		}
	} else {
		if (!get_charger_status(battery)) {
			pr_err("%s: 0%% wo/ charging, will be power off\n",
								__func__);
			battery->info.level = 0;
			wake_lock_timeout(&battery->vbus_wake_lock, HZ);
			power_supply_changed(&battery->psy_battery);
		} else {
			pr_info("%s: 0%% w/ charging, exit low bat alarm\n",
								__func__);
			/* finish low battery alarm state */
			battery->is_low_batt_alarm = false;
		}
	}

	pr_info("%s: low batt alarm(%d)\n", __func__,
				battery->is_low_batt_alarm);
	return;
}

#define STABLE_LOW_BATTERY_DIFF	3
#define STABLE_LOW_BATTERY_DIFF_LOWBATT	1
int _low_battery_alarm_(struct battery_data *battery)
{
	int overcurrent_limit_in_soc;
	int current_soc = get_fuelgauge_value(FG_LEVEL);
	pr_info("%s\n", __func__);

	if (battery->info.level <= STABLE_LOW_BATTERY_DIFF)
		overcurrent_limit_in_soc = STABLE_LOW_BATTERY_DIFF_LOWBATT;
	else
		overcurrent_limit_in_soc = STABLE_LOW_BATTERY_DIFF;

	if ((battery->info.level - current_soc) > overcurrent_limit_in_soc) {
		pr_info("%s: Abnormal Current Consumption jump by %d units.\n",
			__func__, ((battery->info.level - current_soc)));
		pr_info("Last Reported SOC (%d).\n",
				battery->info.level);

		battery->is_low_batt_alarm = true;

		if (battery->info.level > 0) {
			queue_delayed_work(battery->sec_TA_workqueue,
				&battery->fuelgauge_recovery_work, 0);
			return 0;
		}
	}

	if (!get_charger_status(battery)) {
		pr_err("Set battery level as 0, power off.\n");
		battery->info.level = 0;
		wake_lock_timeout(&battery->vbus_wake_lock, HZ);
		power_supply_changed(&battery->psy_battery);
	}

	return 0;
}

static void fuelgauge_work_handler(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, fuelgauge_work.work);
	pr_info("low battery alert!");
	if (get_fuelgauge_value(FG_CHECK_STATUS))
		_low_battery_alarm_(battery);
}

static void full_comp_work_handler(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, full_comp_work.work);
	int avg_current = get_fuelgauge_value(FG_CURRENT_AVG);

	if (avg_current >= 25) {
		cancel_delayed_work(&battery->full_comp_work);
		schedule_delayed_work(&battery->full_comp_work, 100);
	} else {
		pr_info("%s: full charge compensation start (avg_current %d)\n",
			__func__, avg_current);
		fg_fullcharged_compensation(
			battery->full_charge_comp_recharge_info, 0);
	}
}

#define BATTERY_FULL_THRESHOLD_SOC	95	/* 95 / 1.333 = 71.27 */
#if defined(P4_CHARGING_FEATURE_01)
#define BATTERY_FULL_THRESHOLD_VFSOC	60	/* 60 * 1.333 = 79.98 */
#else
#define BATTERY_FULL_THRESHOLD_VFSOC	71	/* 71 * 1.333 = 94.64 */
#endif

static void fullcharging_work_handler(struct work_struct *work)
{
	struct battery_data *battery =
		container_of(work, struct battery_data, fullcharging_work.work);
	int check_charger_state = 1;
	int fg_soc, fg_vfsoc;
	pr_info("%s : nCHG intr!!, fullcharge_line=%d",
		__func__,
		gpio_get_value(battery->pdata->charger.fullcharge_line));

	if (gpio_get_value(battery->pdata->charger.fullcharge_line) == 1) {
#if defined(P2_CHARGING_FEATURE_02)
		/* Check charger state */
		if (battery->pdata->get_charging_state) {
			if (battery->pdata->get_charging_state() == \
						POWER_SUPPLY_STATUS_UNKNOWN) {
				pr_info("Disconnect cable and check charger\n");
				if (battery->pmic_cable_state == 0) {
					battery->cable_detect_source = 2;
					sec_get_cable_status(battery);
					sec_cable_changed(battery);
					check_charger_state = 0;
				}
			}
		}
#endif

#if defined(P4_CHARGING_FEATURE_01)
		fg_vfsoc = get_fuelgauge_value(FG_VF_SOC);
		check_charger_state = battery->pdata->get_charger_is_full();
		/* Check full charged state */
		if (get_charger_status(battery) != 0
			&& fg_vfsoc > BATTERY_FULL_THRESHOLD_VFSOC
			&& check_charger_state == POWER_SUPPLY_STATUS_FULL) {
			pr_info("Battery is full\n");
			sec_cable_charging(battery);
		}
#else
		fg_soc = get_fuelgauge_value(FG_LEVEL);
		fg_vfsoc = get_fuelgauge_value(FG_VF_SOC);
		/* Check full charged state */
		if (get_charger_status(battery) != 0
			&& fg_soc > BATTERY_FULL_THRESHOLD_SOC
			&& fg_vfsoc > BATTERY_FULL_THRESHOLD_VFSOC
			&& check_charger_state == 1) {
			pr_info("Battery is full\n");
			sec_cable_charging(battery);
		}
#endif
	} else {
		pr_info("Charger is working. Cable state will be updated.\n");
		/* Cable detect from default */
		battery->cable_detect_source = 0;
	}

	enable_irq(battery->charging_irq);
}

#if defined(CONFIG_TARGET_LOCALE_KOR)
static int sec_bat_read_proc(char *buf, char **start,
			     off_t offset, int count, int *eof, void *data)
{
	struct battery_data *battery = data;
	struct timespec cur_time;
	ktime_t ktime;
	int len = 0;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

	len = sprintf(buf,
		"%lu\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t0x%04x\t0x%04x\n",
		cur_time.tv_sec,
		battery->info.batt_vol, battery->info.level,
		battery->info.batt_soc, battery->info.batt_vfsoc,
		max17042_chip_data->info.psoc,
		battery->info.batt_temp,
		battery->info.batt_current, battery->info.batt_current_avg,
		battery->info.charging_source, battery->info.batt_improper_ta,
		battery->info.charging_enabled, battery->info.charging_current,
		sec_bat_get_charging_status(battery), battery->info.batt_health,
		battery->info.batt_is_full, battery->info.batt_is_recharging,
		battery->info.abstimer_is_active, battery->info.siop_activated,
		get_fuelgauge_capacity(CAPACITY_TYPE_FULL),
		get_fuelgauge_capacity(CAPACITY_TYPE_REP)
		);

	return len;
}
#endif /* CONFIG_TARGET_LOCALE_KOR */

static int __devinit sec_bat_probe(struct platform_device *pdev)
{
	struct sec_battery_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct battery_data *battery;
	int ret;
	unsigned long trigger;
	int irq_num;

	pr_info("%s : SEC Battery Driver Loading\n", __func__);

	battery = kzalloc(sizeof(*battery), GFP_KERNEL);
	if (!battery)
		return -ENOMEM;

	battery->pdata = pdata;
	if (!battery->pdata) {
		pr_err("%s : No platform data\n", __func__);
		ret = -EINVAL;
		goto err_pdata;
	}

	battery->pdata->init_charger_gpio();

	platform_set_drvdata(pdev, battery);
	debug_batterydata = battery;

	battery->present = 1;
	battery->info.level = 100;
	battery->info.charging_source = CHARGER_BATTERY;
	battery->info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	battery->info.abstimer_is_active = 0;
	battery->is_first_check = true;
	battery->is_low_batt_alarm = false;

	battery->psy_battery.name = "battery";
	battery->psy_battery.type = POWER_SUPPLY_TYPE_BATTERY;
	battery->psy_battery.properties = sec_battery_properties;
	battery->psy_battery.num_properties = \
					ARRAY_SIZE(sec_battery_properties);
	battery->psy_battery.get_property = sec_bat_get_property;
	battery->psy_battery.set_property = sec_bat_set_property;

	battery->psy_usb.name = "usb";
	battery->psy_usb.type = POWER_SUPPLY_TYPE_USB;
	battery->psy_usb.supplied_to = supply_list;
	battery->psy_usb.num_supplicants = ARRAY_SIZE(supply_list);
	battery->psy_usb.properties = sec_power_properties;
	battery->psy_usb.num_properties = ARRAY_SIZE(sec_power_properties);
	battery->psy_usb.get_property = sec_usb_get_property;

	battery->psy_ac.name = "ac";
	battery->psy_ac.type = POWER_SUPPLY_TYPE_MAINS;
	battery->psy_ac.supplied_to = supply_list;
	battery->psy_ac.num_supplicants = ARRAY_SIZE(supply_list);
	battery->psy_ac.properties = sec_power_properties;
	battery->psy_ac.num_properties = ARRAY_SIZE(sec_power_properties);
	battery->psy_ac.get_property = sec_ac_get_property;

	mutex_init(&battery->work_lock);

	wake_lock_init(&battery->vbus_wake_lock, WAKE_LOCK_SUSPEND,
		"vbus wake lock");
	wake_lock_init(&battery->work_wake_lock, WAKE_LOCK_SUSPEND,
		"batt_work wake lock");
	wake_lock_init(&battery->cable_wake_lock, WAKE_LOCK_SUSPEND,
		"temp wake lock");
	wake_lock_init(&battery->fullcharge_wake_lock, WAKE_LOCK_SUSPEND,
		"fullcharge wake lock");
#ifdef __TEST_DEVICE_DRIVER__
	wake_lock_init(&battery->wake_lock_for_dev, WAKE_LOCK_SUSPEND,
		"test mode wake lock");
#endif /* __TEST_DEVICE_DRIVER__ */

	INIT_WORK(&battery->battery_work, sec_bat_work);
	INIT_WORK(&battery->cable_work, sec_cable_work);
	INIT_DELAYED_WORK(&battery->fuelgauge_work, fuelgauge_work_handler);
	INIT_DELAYED_WORK(&battery->fuelgauge_recovery_work,
			fuelgauge_recovery_handler);
	INIT_DELAYED_WORK(&battery->fullcharging_work,
			fullcharging_work_handler);
	INIT_DELAYED_WORK(&battery->full_comp_work, full_comp_work_handler);
	INIT_DELAYED_WORK(&battery->TA_work, sec_TA_work_handler);

	battery->sec_TA_workqueue = create_singlethread_workqueue(
		"sec_TA_workqueue");
	if (!battery->sec_TA_workqueue) {
		pr_err("Failed to create single workqueue\n");
		ret = -ENOMEM;
		goto err_workqueue_init;
	}

	battery->padc = s3c_adc_register(pdev, NULL, NULL, 0);

	battery->last_poll = alarm_get_elapsed_realtime();
	alarm_init(&battery->alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
		sec_battery_alarm);

	ret = power_supply_register(&pdev->dev, &battery->psy_battery);
	if (ret) {
		pr_err("Failed to register battery power supply.\n");
		goto err_battery_psy_register;
	}

	ret = power_supply_register(&pdev->dev, &battery->psy_usb);
	if (ret) {
		pr_err("Failed to register USB power supply.\n");
		goto err_usb_psy_register;
	}

	ret = power_supply_register(&pdev->dev, &battery->psy_ac);
	if (ret) {
		pr_err("Failed to register AC power supply.\n");
		goto err_ac_psy_register;
	}

	/* create sec detail attributes */
	sec_bat_create_attrs(battery->psy_battery.dev);

#ifdef __TEST_DEVICE_DRIVER__
	sec_batt_test_create_attrs(battery->psy_ac.dev);
#endif /* __TEST_DEVICE_DRIVER__ */

	battery->sec_battery_initial = 1;
	battery->low_batt_boot_flag = 0;

	/* Get initial cable status */
	sec_get_cable_status(battery);
	battery->previous_cable_status = battery->current_cable_status;
	sec_cable_check_status(battery);

	/* TA_nCHG irq */
	battery->charging_irq = gpio_to_irq(pdata->charger.fullcharge_line);
	trigger = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	if (request_threaded_irq
	    (battery->charging_irq, NULL, sec_TA_nCHG_interrupt_handler,
	     trigger, "TA_nCHG intr", battery)) {
		pr_err("sec_TA_nCHG_interrupt_handler register failed!\n");
		goto err_charger_irq;
	}

	/* TA_nConnected irq */
	battery->connect_irq = gpio_to_irq(pdata->charger.connect_line);
	trigger = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	if (request_threaded_irq
	    (battery->connect_irq, NULL, sec_TA_nCon_interrupt_handler, trigger,
	     "TA_nConnected intr", battery)) {
		pr_err("sec_TA_nCon_interrupt_handler register failed!\n");
		goto err_charger_irq;
	}

	if (enable_irq_wake(battery->connect_irq))
		pr_err("TA_nConnected enable_irq_wake fail!\n");

	if (check_ta_conn(battery) && check_UV_charging_case())
		battery->low_batt_boot_flag = 1;

	mutex_lock(&battery->work_lock);
	fg_alert_init();
	mutex_unlock(&battery->work_lock);

	/* before enable fullcharge interrupt, check fullcharge */
	if (((battery->info.charging_source == CHARGER_AC) ||
		(battery->info.charging_source == CHARGER_MISC) ||
		(battery->info.charging_source == CHARGER_DOCK))
		&& battery->info.charging_enabled
		&& gpio_get_value(pdata->charger.fullcharge_line) == 1)
		sec_cable_charging(battery);

	/* Request IRQ */
	irq_num = gpio_to_irq(max17042_chip_data->pdata->fuel_alert_line);
	if (request_threaded_irq(irq_num, NULL, low_battery_isr,
				 IRQF_TRIGGER_FALLING, "FUEL_ALRT irq",
				 battery))
		pr_err("Can NOT request irq 'IRQ_FUEL_ALERT' %d ", irq_num);

	if (enable_irq_wake(irq_num))
		pr_err("FUEL_ALERT enable_irq_wake fail!\n");

#ifdef CONFIG_SAMSUNG_LPM_MODE
	lpm_mode_check(battery);
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR)
	battery->entry = create_proc_entry("batt_info_proc", S_IRUGO, NULL);
	if (!battery->entry) {
		pr_err("%s : failed to create proc_entry!\n", __func__);
	} else {
		battery->entry->read_proc = sec_bat_read_proc;
		battery->entry->data = battery;
	}
#endif /* CONFIG_TARGET_LOCALE_KOR */

	return 0;

err_charger_irq:
	alarm_cancel(&battery->alarm);
	power_supply_unregister(&battery->psy_ac);
err_ac_psy_register:
	power_supply_unregister(&battery->psy_usb);
err_usb_psy_register:
	power_supply_unregister(&battery->psy_battery);
err_battery_psy_register:
	destroy_workqueue(battery->sec_TA_workqueue);
err_workqueue_init:
	wake_lock_destroy(&battery->vbus_wake_lock);
	wake_lock_destroy(&battery->work_wake_lock);
	wake_lock_destroy(&battery->cable_wake_lock);
	wake_lock_destroy(&battery->fullcharge_wake_lock);
	mutex_destroy(&battery->work_lock);
err_pdata:
	kfree(battery);

	return ret;
}

static int __devexit sec_bat_remove(struct platform_device *pdev)
{
	struct battery_data *battery = platform_get_drvdata(pdev);

	free_irq(gpio_to_irq(max17042_chip_data->pdata->fuel_alert_line), NULL);
	free_irq(gpio_to_irq(battery->pdata->charger.connect_line), NULL);

	alarm_cancel(&battery->alarm);
	power_supply_unregister(&battery->psy_ac);
	power_supply_unregister(&battery->psy_usb);
	power_supply_unregister(&battery->psy_battery);

	destroy_workqueue(battery->sec_TA_workqueue);
	cancel_delayed_work(&battery->fuelgauge_work);
	cancel_delayed_work(&battery->fuelgauge_recovery_work);
	cancel_delayed_work(&battery->fullcharging_work);
	cancel_delayed_work(&battery->full_comp_work);

	wake_lock_destroy(&battery->vbus_wake_lock);
	wake_lock_destroy(&battery->work_wake_lock);
	wake_lock_destroy(&battery->cable_wake_lock);
	wake_lock_destroy(&battery->fullcharge_wake_lock);
	mutex_destroy(&battery->work_lock);

	kfree(battery);

	return 0;
}

static const struct dev_pm_ops sec_battery_pm_ops = {
	.prepare = sec_bat_suspend,
	.complete = sec_bat_resume,
};

static struct platform_driver sec_bat_driver = {
	.driver = {
		.name = "sec-battery",
		.owner  = THIS_MODULE,
		.pm = &sec_battery_pm_ops,
	},
	.probe		= sec_bat_probe,
	.remove		= __devexit_p(sec_bat_remove),
};

static int __init sec_bat_init(void)
{
	return platform_driver_register(&sec_bat_driver);
}

static void __exit sec_bat_exit(void)
{
	platform_driver_unregister(&sec_bat_driver);
}

late_initcall(sec_bat_init);
module_exit(sec_bat_exit);

MODULE_DESCRIPTION("battery driver");
MODULE_LICENSE("GPL");

