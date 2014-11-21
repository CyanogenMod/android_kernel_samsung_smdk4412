/*
 *  max77693_charger.c
 *  Samsung max77693 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/battery/charger/max77693_charger_bat.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#define DEBUG

#define ENABLE 1
#define DISABLE 0


#define RECOVERY_DELAY		3000
#define RECOVERY_CNT		5
#define REDUCE_CURRENT_STEP	100
#define MINIMUM_INPUT_CURRENT	300

static struct dentry    *max77693_dentry;

struct max77693_charger_data {
	struct max77693_dev	*max77693;

	struct power_supply	psy_chg;

	struct workqueue_struct *wqueue;
	struct work_struct	chgin_work;
	struct delayed_work	isr_work;
	struct delayed_work	recovery_work;	/*  softreg recovery work */
	struct delayed_work	wpc_work;	/*  wpc detect work */
	struct delayed_work	chgin_init_work;	/*  chgin init work */

	/* mutex */
	struct mutex irq_lock;
	struct mutex ops_lock;

	/* wakelock */
	struct wake_lock recovery_wake_lock;

	unsigned int	is_charging;
	unsigned int	charging_type;
	unsigned int	battery_state;
	unsigned int	battery_present;
	unsigned int	cable_type;
	unsigned int	charging_current_max;
	unsigned int	charging_current;
	unsigned int	vbus_state;
	int		status;
	int		siop_level;

	int		irq_bypass;
	int		irq_therm;
	int		irq_battery;
	int		irq_chg;
	int		irq_chgin;

	/* software regulation */
	bool		soft_reg_state;
	int		soft_reg_current;

	/* unsufficient power */
	bool		reg_loop_deted;

#if defined(CONFIG_WIRELESS_CHARGING)
	/* wireless charge, w(wpc), v(vbus) */
	int		wc_w_gpio;
	int		wc_w_irq;
	int		wc_w_state;
	int		wc_v_gpio;
	int		wc_v_irq;
	int		wc_v_state;
	bool		wc_pwr_det;
#endif
	int		soft_reg_recovery_cnt;

	sec_battery_platform_data_t	*pdata;
};

static enum power_supply_property sec_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_POWER_NOW,
};

static void max77693_charger_initialize(struct max77693_charger_data *charger);
static int max77693_get_vbus_state(struct max77693_charger_data *charger);
static int max77693_get_charger_state(struct max77693_charger_data *charger);
static void max77693_dump_reg(struct max77693_charger_data *charger)
{
	u8 reg_data;
	u32 reg_addr;
	pr_info("%s\n", __func__);

	for (reg_addr = 0xB0; reg_addr <= 0xC5; reg_addr++) {
		max77693_read_reg(charger->max77693->i2c, reg_addr, &reg_data);
		pr_info("max77693: c: 0x%02x(0x%02x)\n", reg_addr, reg_data);
	}
}

static bool max77693_charger_unlock(struct max77693_charger_data *chg_data)
{
	struct i2c_client *i2c = chg_data->max77693->i2c;
	u8 reg_data;
	u8 chgprot;
	int retry_cnt = 0;
	bool need_init = false;

	do {
		max77693_read_reg(i2c, MAX77693_CHG_REG_CHG_CNFG_06, &reg_data);
		chgprot = ((reg_data & 0x0C) >> 2);
		if (chgprot != 0x03) {
			pr_err("%s: unlock err, chgprot(0x%x), retry(%d)\n",
					__func__, chgprot, retry_cnt);
			max77693_write_reg(i2c, MAX77693_CHG_REG_CHG_CNFG_06,
				(0x03 << 2));
			need_init = true;
			msleep(20);
		} else {
			pr_debug("%s: unlock success, chgprot(0x%x)\n",
				__func__, chgprot);
			break;
		}
	} while ((chgprot != 0x03) && (++retry_cnt < 10));

	return need_init;
}

static void check_charger_unlock_state(struct max77693_charger_data *chg_data)
{
	bool need_reg_init;
	pr_debug("%s\n", __func__);

	need_reg_init = max77693_charger_unlock(chg_data);
	if (need_reg_init) {
		pr_err("%s: charger locked state, reg init\n", __func__);
		max77693_charger_initialize(chg_data);
	}
}

static int max77693_get_battery_present(struct max77693_charger_data *charger)
{
	u8 reg_data;

	if (max77693_read_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_OK, &reg_data) < 0) {
		/* Eventhough there is an error,
		   don't do power-off */
		return 1;
	}

	pr_debug("%s: CHG_INT_OK(0x%02x)\n", __func__, reg_data);

	reg_data = ((reg_data & MAX77693_DETBAT) >> MAX77693_DETBAT_SHIFT);

	return reg_data;
}

static void max77693_set_charger_state(struct max77693_charger_data *charger,
		int enable)
{
	u8 reg_data;

	max77693_read_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, &reg_data);

	if (enable)
		reg_data |= MAX77693_MODE_CHGR;
	else
		reg_data &= ~MAX77693_MODE_CHGR;

	pr_info("%s: CHG_CNFG_00(0x%02x)\n", __func__, reg_data);
	max77693_write_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, reg_data);
}

static void max77693_set_buck(struct max77693_charger_data *charger,
		int enable)
{
	u8 reg_data;

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_00, &reg_data);

	if (enable)
		reg_data |= MAX77693_MODE_BUCK;
	else
		reg_data &= ~MAX77693_MODE_BUCK;

	pr_debug("%s: CHG_CNFG_00(0x%02x)\n", __func__, reg_data);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_00, reg_data);
}

static void max77693_set_input_current(struct max77693_charger_data *charger,
		int cur)
{
	int set_current_reg, now_current_reg;
	int vbus_state, curr_step, delay;
	u8 set_reg, reg_data;
	int chg_state;

	mutex_lock(&charger->ops_lock);
	disable_irq(charger->irq_chgin);
	if (charger->cable_type == POWER_SUPPLY_TYPE_WIRELESS)
		set_reg = MAX77693_CHG_REG_CHG_CNFG_10;
	else
		set_reg = MAX77693_CHG_REG_CHG_CNFG_09;

	if (cur <= 0) {
		/* disable only buck because power onoff test issue */
		max77693_write_reg(charger->max77693->i2c,
			set_reg, 0x19);
//		max77693_set_buck(charger, DISABLE);
		goto exit;
	} else
		max77693_set_buck(charger, ENABLE);

	set_current_reg = cur / 20;
	if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY)
		goto set_input_current;

	max77693_read_reg(charger->max77693->i2c,
		set_reg, &reg_data);
	if (reg_data == set_current_reg) {
		/* check uvlo  */
		while((set_current_reg > (MINIMUM_INPUT_CURRENT / 20)) && (set_current_reg < 255)) {
			vbus_state = max77693_get_vbus_state(charger);
			if (((vbus_state == 0x00) || (vbus_state == 0x01)) &&
				(charger->cable_type != POWER_SUPPLY_TYPE_WIRELESS)) {
				/* UVLO */
				set_current_reg -= 5;
				if (set_current_reg < (MINIMUM_INPUT_CURRENT / 20))
					set_current_reg = (MINIMUM_INPUT_CURRENT / 20);
				max77693_write_reg(charger->max77693->i2c,
						set_reg, set_current_reg);
				pr_info("%s: set_current_reg(0x%02x)\n", __func__, set_current_reg);
				chg_state = max77693_get_charger_state(charger);
				if ((chg_state != POWER_SUPPLY_STATUS_CHARGING) &&
						(chg_state != POWER_SUPPLY_STATUS_FULL))
					break;
				msleep(50);
			} else
				break;
		}
		goto exit;
	}

	if (reg_data == 0) {
		now_current_reg = SOFT_CHG_START_CURR / 20;
		max77693_write_reg(charger->max77693->i2c,
			set_reg, now_current_reg);
		msleep(SOFT_CHG_START_DUR);
	} else
		now_current_reg = reg_data;

	if (cur <= 1000) {
		curr_step = 1;
		delay = 50;
	} else {
		curr_step = SOFT_CHG_CURR_STEP / 20;
		delay = SOFT_CHG_STEP_DUR;
	}
	now_current_reg += (curr_step);

	while (now_current_reg < set_current_reg &&
			charger->cable_type != POWER_SUPPLY_TYPE_BATTERY)
	{
		now_current_reg = min(now_current_reg, set_current_reg);
		max77693_write_reg(charger->max77693->i2c,
			set_reg, now_current_reg);
		msleep(delay);

		vbus_state = max77693_get_vbus_state(charger);
		if (((vbus_state == 0x00) || (vbus_state == 0x01)) &&
				(charger->cable_type != POWER_SUPPLY_TYPE_WIRELESS)) {
			/* UVLO */
			if (now_current_reg > (curr_step * 3))
				now_current_reg -= (curr_step * 3);
			/* current limit 300mA */
			if (now_current_reg < (MINIMUM_INPUT_CURRENT / 20))
				now_current_reg = (MINIMUM_INPUT_CURRENT / 20);
			curr_step /= 2;
			max77693_write_reg(charger->max77693->i2c,
					set_reg, now_current_reg);
			pr_info("%s: now_current_reg(0x%02x)\n", __func__, now_current_reg);
			chg_state = max77693_get_charger_state(charger);
			if ((chg_state != POWER_SUPPLY_STATUS_CHARGING) &&
					(chg_state != POWER_SUPPLY_STATUS_FULL))
				goto exit;
			if (curr_step < 2)
				goto exit;
			msleep(50);
		} else
			now_current_reg += (curr_step);
	}

set_input_current:
	pr_info("%s: reg_data(0x%02x), input(%d)\n",
		__func__, set_current_reg, cur);
	max77693_write_reg(charger->max77693->i2c,
		set_reg, set_current_reg);
exit:
	enable_irq(charger->irq_chgin);
	mutex_unlock(&charger->ops_lock);
}

static int max77693_get_input_current(struct max77693_charger_data *charger)
{
	u8 reg_data;
	int get_current = 0;

	if (charger->cable_type == POWER_SUPPLY_TYPE_WIRELESS) {
		max77693_read_reg(charger->max77693->i2c,
				MAX77693_CHG_REG_CHG_CNFG_10, &reg_data);
		pr_info("%s: CHG_CNFG_10(0x%02x)\n", __func__, reg_data);
	} else {
		max77693_read_reg(charger->max77693->i2c,
				MAX77693_CHG_REG_CHG_CNFG_09, &reg_data);
		pr_info("%s: CHG_CNFG_09(0x%02x)\n", __func__, reg_data);
	}
	get_current = reg_data * 20;

	pr_debug("%s: get input current: %dmA\n", __func__, get_current);
	return get_current;
}

static void max77693_set_topoff_current(struct max77693_charger_data *charger,
		int cur, int timeout)
{
	u8 reg_data;

	if (cur >= 350)
		reg_data = 0x07;
	else if (cur >= 300)
		reg_data = 0x06;
	else if (cur >= 250)
		reg_data = 0x05;
	else if (cur >= 200)
		reg_data = 0x04;
	else if (cur >= 175)
		reg_data = 0x03;
	else if (cur >= 150)
		reg_data = 0x02;
	else if (cur >= 125)
		reg_data = 0x01;
	else
		reg_data = 0x00;

	/* the unit of timeout is second*/
	timeout = timeout / 60;
	reg_data |= ((timeout / 10) << 3);
	pr_info("%s: reg_data(0x%02x), topoff(%d), time(%d)\n", __func__, reg_data, cur, timeout);

	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_03, reg_data);
}

static void max77693_set_charge_current(struct max77693_charger_data *charger,
		int cur)
{
	u8 reg_data = 0;

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_02, &reg_data);
	reg_data &= ~MAX77693_CHG_CC;

	if (!cur) {
		/* No charger */
		max77693_write_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_02, reg_data);
	} else {
		reg_data |= ((cur * 3 / 100) << 0);
		max77693_write_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_02, reg_data);
	}
	pr_info("%s: charge current %d mA, reg_data(0x%02x)\n",
				__func__, cur, reg_data);
}

/* in soft regulation, current recovery operation */
static void max77693_recovery_work(struct work_struct *work)
{
	struct max77693_charger_data *chg_data = container_of(work,
						struct max77693_charger_data,
						recovery_work.work);
	u8 dtls_00, chgin_dtls;
	u8 dtls_01, chg_dtls;
	u8 dtls_02, byp_dtls;
	pr_info("%s: start\n", __func__);

	wake_unlock(&chg_data->recovery_wake_lock);
	if ((!chg_data->is_charging) || mutex_is_locked(&chg_data->ops_lock) ||
			(chg_data->cable_type != POWER_SUPPLY_TYPE_MAINS))
		return;
	max77693_read_reg(chg_data->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_00, &dtls_00);
	max77693_read_reg(chg_data->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_01, &dtls_01);
	max77693_read_reg(chg_data->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_02, &dtls_02);

	chgin_dtls = ((dtls_00 & MAX77693_CHGIN_DTLS) >>
				MAX77693_CHGIN_DTLS_SHIFT);
	chg_dtls = ((dtls_01 & MAX77693_CHG_DTLS) >>
				MAX77693_CHG_DTLS_SHIFT);
	byp_dtls = ((dtls_02 & MAX77693_BYP_DTLS) >>
				MAX77693_BYP_DTLS_SHIFT);

	if ((chg_data->soft_reg_recovery_cnt < RECOVERY_CNT) && (
		(chgin_dtls == 0x3) && (chg_dtls != 0x8) && (byp_dtls == 0x0))) {
		pr_info("%s: try to recovery, cnt(%d)\n", __func__,
				(chg_data->soft_reg_recovery_cnt + 1));
		max77693_set_input_current(chg_data,
				chg_data->charging_current_max);

	} else {
		pr_info("%s: fail to recovery, cnt(%d)\n", __func__,
				(chg_data->soft_reg_recovery_cnt + 1));

		pr_info("%s:  CHGIN(0x%x), CHG(0x%x), BYP(0x%x)\n",
				__func__, chgin_dtls, chg_dtls, byp_dtls);

		/* schedule softreg recovery wq */
		if (chg_data->soft_reg_recovery_cnt < RECOVERY_CNT) {
			wake_lock(&chg_data->recovery_wake_lock);
			queue_delayed_work(chg_data->wqueue, &chg_data->recovery_work,
				msecs_to_jiffies(RECOVERY_DELAY));
		} else {
			pr_info("%s: recovery cnt(%d) is over\n",
				__func__, RECOVERY_CNT);
		}
	}

	pr_info("%s: end\n", __func__);
	/* add recovery try count */
	chg_data->soft_reg_recovery_cnt++;
}

static void reduce_input_current(struct max77693_charger_data *charger, int cur)
{
	u8 set_reg;
	u8 set_value;

	if ((!charger->is_charging) || mutex_is_locked(&charger->ops_lock) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_WIRELESS))
		return;
	set_reg = MAX77693_CHG_REG_CHG_CNFG_09;
	if (!max77693_read_reg(charger->max77693->i2c,
				set_reg, &set_value)) {
		if ((set_value <= (MINIMUM_INPUT_CURRENT / 20)) ||
		    (set_value <= (cur / 20)))
			return;
		set_value -= (cur / 20);
		set_value = (set_value < (MINIMUM_INPUT_CURRENT / 20)) ?
			(MINIMUM_INPUT_CURRENT / 20) : set_value;
		max77693_write_reg(charger->max77693->i2c,
				set_reg, set_value);
		pr_info("%s: set current: reg:(0x%x), val:(0x%x)\n",
				__func__, set_reg, set_value);
	}
	if(charger->cable_type == POWER_SUPPLY_TYPE_MAINS) {
		/* schedule softreg recovery wq */
		cancel_delayed_work_sync(&charger->recovery_work);
		wake_lock(&charger->recovery_wake_lock);
		queue_delayed_work(charger->wqueue, &charger->recovery_work,
				msecs_to_jiffies(RECOVERY_DELAY));
	}
}

static int max77693_get_vbus_state(struct max77693_charger_data *charger)
{
	u8 reg_data;

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_DTLS_00, &reg_data);
	if (charger->cable_type == POWER_SUPPLY_TYPE_WIRELESS)
		reg_data = ((reg_data & MAX77693_WCIN_DTLS) >>
			MAX77693_WCIN_DTLS_SHIFT);
	else
		reg_data = ((reg_data & MAX77693_CHGIN_DTLS) >>
			MAX77693_CHGIN_DTLS_SHIFT);

	switch (reg_data) {
	case 0x00:
		pr_info("%s: VBUS is invalid. CHGIN < CHGIN_UVLO\n",
			__func__);
		break;
	case 0x01:
		pr_info("%s: VBUS is invalid. CHGIN < MBAT+CHGIN2SYS" \
			"and CHGIN > CHGIN_UVLO\n", __func__);
		break;
	case 0x02:
		pr_info("%s: VBUS is invalid. CHGIN > CHGIN_OVLO",
			__func__);
		break;
	case 0x03:
		pr_info("%s: VBUS is valid. CHGIN < CHGIN_OVLO", __func__);
		break;
	default:
		break;
	}

	return reg_data;
}

static int max77693_get_charger_state(struct max77693_charger_data *charger)
{
	int state;
	u8 reg_data;

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_DTLS_01, &reg_data);
	reg_data = ((reg_data & MAX77693_CHG_DTLS) >> MAX77693_CHG_DTLS_SHIFT);
	pr_info("%s: CHG_DTLS : 0x%2x\n", __func__, reg_data);

	switch (reg_data) {
	case 0x0:
	case 0x1:
	case 0x2:
		state = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case 0x3:
	case 0x4:
		state = POWER_SUPPLY_STATUS_FULL;
		break;
	case 0x5:
	case 0x6:
	case 0x7:
		state = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case 0x8:
	case 0xA:
	case 0xB:
		state = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		state = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	return state;
}

static int max77693_get_health_state(struct max77693_charger_data *charger)
{
	int state;
	int vbus_state;
	int chg_state;
	u8 reg_data;

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_DTLS_01, &reg_data);
	reg_data = ((reg_data & MAX77693_BAT_DTLS) >> MAX77693_BAT_DTLS_SHIFT);

	switch (reg_data) {
	case 0x00:
		pr_info("%s: No battery and the charger is suspended\n",
			__func__);
		state = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	case 0x01:
		pr_info("%s: battery unspec failure\n",
			__func__);
		state = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;
	case 0x02:
		pr_info("%s: battery dead\n", __func__);
		state = POWER_SUPPLY_HEALTH_DEAD;
		break;
	case 0x03:
		pr_info("%s: battery good\n", __func__);
		state = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x04:
		pr_info("%s: battery is okay" \
			"but its voltage is low\n", __func__);
		state = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x05:
		pr_info("%s: battery ovp\n", __func__);
		state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		break;
	default:
		pr_info("%s: battery unknown : 0x%d\n", __func__, reg_data);
		state = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	}

	if (state == POWER_SUPPLY_HEALTH_GOOD) {
		/* VBUS OVP state return battery OVP state */
		vbus_state = max77693_get_vbus_state(charger);
		/* read CHG_DTLS and detecting battery terminal error */
		chg_state = max77693_get_charger_state(charger);
		/*  OVP is higher priority */
		if (vbus_state == 0x02) { /*  CHGIN_OVLO */
			pr_info("%s: vbus ovp\n", __func__);
			state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (reg_data == 0x04 &&
				chg_state == POWER_SUPPLY_STATUS_FULL) {
			pr_info("%s: battery terminal error\n", __func__);
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		}
	}

	return state;
}

static int sec_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max77693_charger_data *charger =
		container_of(psy, struct max77693_charger_data, psy_chg);
	u8 reg_data;
	int input_current = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = POWER_SUPPLY_TYPE_BATTERY;
		if (max77693_read_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_OK, &reg_data) == 0) {
			if (reg_data & MAX77693_CHGIN_OK)
				val->intval = POWER_SUPPLY_TYPE_MAINS;
			else if (reg_data & MAX77693_WCIN_OK) {
				val->intval = POWER_SUPPLY_TYPE_WIRELESS;
#if defined(CONFIG_WIRELESS_CHARGING)
				charger->wc_w_state = 1;
#endif
			}
		}
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = max77693_get_charger_state(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = max77693_get_health_state(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = charger->charging_current_max;
		input_current = max77693_get_input_current(charger);
		pr_info("%s: max77693 Inow(%dmA)\n",
			__func__, input_current);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = charger->charging_current;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = max77693_get_input_current(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (!charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = max77693_get_battery_present(charger);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct max77693_charger_data *charger =
		container_of(psy, struct max77693_charger_data, psy_chg);
	union power_supply_propval value;
	int set_charging_current, set_charging_current_max;
	const int usb_charging_current = charger->pdata->charging_current[
		POWER_SUPPLY_TYPE_USB].fast_charging_current;
	const int wpc_charging_current = charger->pdata->charging_current[
		POWER_SUPPLY_TYPE_WIRELESS].input_current_limit;

	/* check and unlock */
	check_charger_unlock_state(charger);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		psy_do_property("battery", get,
				POWER_SUPPLY_PROP_HEALTH, value);
		if (val->intval == POWER_SUPPLY_TYPE_BATTERY) {
			charger->is_charging = false;
			charger->soft_reg_recovery_cnt = 0;
			set_charging_current = 0;
			set_charging_current_max = 0;
		} else {
			charger->is_charging = true;
			/* decrease the charging current according to siop level */
			set_charging_current =
				charger->charging_current * charger->siop_level / 100;
			if (set_charging_current > 0 &&
					set_charging_current < usb_charging_current)
				set_charging_current = usb_charging_current;
			if (val->intval == POWER_SUPPLY_TYPE_WIRELESS)
				set_charging_current_max = wpc_charging_current;
			else
				set_charging_current_max =
					charger->charging_current_max;
		}

		max77693_set_charger_state(charger, charger->is_charging);
		/* if battery full, only disable charging  */
		if ((charger->status == POWER_SUPPLY_STATUS_CHARGING) ||
				(charger->status == POWER_SUPPLY_STATUS_DISCHARGING) ||
				(value.intval == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)) {

			/* current setting */
			max77693_set_charge_current(charger,
				set_charging_current);
			/* if battery is removed, disable input current and reenable input current
			  *  to enable buck always */
			if (value.intval == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
				max77693_set_input_current(charger, 0);
			else
				max77693_set_input_current(charger,
						set_charging_current_max);
			max77693_set_topoff_current(charger,
				charger->pdata->charging_current[
				val->intval].full_check_current_1st,
				charger->pdata->charging_current[
				val->intval].full_check_current_2nd);
		}
		break;
	/* val->intval : input charging current */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->charging_current_max = val->intval;
		break;
	/*  val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->siop_level = val->intval;
		if (charger->is_charging) {
			/* decrease the charging current according to siop level */
			int current_now =
				charger->charging_current * val->intval / 100;
			if (current_now > 0 &&
					current_now < usb_charging_current)
				current_now = usb_charging_current;
			max77693_set_charge_current(charger, current_now);
		}
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		max77693_set_charge_current(charger,
				val->intval);
		max77693_set_input_current(charger,
				val->intval);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void max77693_charger_initialize(struct max77693_charger_data *charger)
{
	u8 reg_data;
	pr_debug("%s\n", __func__);

	/* unlock charger setting protect */
	reg_data = (0x03 << 2);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_06, reg_data);

	/*
	 * fast charge timer disable
	 * restart threshold disable
	 * pre-qual charge enable(default)
	 */
	reg_data = (0x0 << 0) | (0x03 << 4);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_01, reg_data);

	/*
	 * charge current 466mA(default)
	 * otg current limit 900mA
	 */
	max77693_read_reg(charger->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_02, &reg_data);
	reg_data |= (1 << 7);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_02, reg_data);

	/*
	 * top off current 100mA
	 * top off timer 40min
	 */
	reg_data = (0x04 << 3);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_03, reg_data);

	/*
	 * cv voltage 4.2V or 4.35V
	 * MINVSYS 3.6V(default)
	 */
	reg_data = (0xDD << 0);
	/*
	pr_info("%s: battery cv voltage %s, (sysrev %d)\n", __func__,
		(((reg_data & MAX77693_CHG_PRM_MASK) == \
		(0x1D << MAX77693_CHG_PRM_SHIFT)) ? "4.35V" : "4.2V"),
		system_rev);
	*/
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_04, reg_data);

	max77693_dump_reg(charger);
}

static void sec_chg_isr_work(struct work_struct *work)
{
	struct max77693_charger_data *charger =
		container_of(work, struct max77693_charger_data, isr_work.work);

	union power_supply_propval val;

	if (charger->pdata->full_check_type ==
			SEC_BATTERY_FULLCHARGED_CHGINT) {

		val.intval = max77693_get_charger_state(charger);

		switch (val.intval) {
		case POWER_SUPPLY_STATUS_DISCHARGING:
			pr_err("%s: Interrupted but Discharging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_NOT_CHARGING:
			pr_err("%s: Interrupted but NOT Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_FULL:
			pr_info("%s: Interrupted by Full\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_STATUS, val);
			break;

		case POWER_SUPPLY_STATUS_CHARGING:
			pr_err("%s: Interrupted but Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_UNKNOWN:
		default:
			pr_err("%s: Invalid Charger Status\n", __func__);
			break;
		}
	}

	if (charger->pdata->ovp_uvlo_check_type ==
			SEC_BATTERY_OVP_UVLO_CHGINT) {

		val.intval = max77693_get_health_state(charger);

		switch (val.intval) {
		case POWER_SUPPLY_HEALTH_OVERHEAT:
		case POWER_SUPPLY_HEALTH_COLD:
			pr_err("%s: Interrupted but Hot/Cold\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_DEAD:
			pr_err("%s: Interrupted but Dead\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
			pr_info("%s: Interrupted by OVP/UVLO\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_HEALTH, val);
			break;

		case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
			pr_err("%s: Interrupted but Unspec\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_GOOD:
			pr_err("%s: Interrupted but Good\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_UNKNOWN:
		default:
			pr_err("%s: Invalid Charger Health\n", __func__);
			break;
		}
	}
}

static int max77693_debugfs_show(struct seq_file *s, void *data)
{
	struct max77693_charger_data *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "MAX77693 CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = 0xB0; reg <= 0xC5; reg++) {
		max77693_read_reg(charger->max77693->i2c, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");

	return 0;
}

static int max77693_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, max77693_debugfs_show, inode->i_private);
}

static const struct file_operations max77693_debugfs_fops = {
	.open           = max77693_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static irqreturn_t sec_chg_irq_thread(int irq, void *irq_data)
{
	struct max77693_charger_data *charger = irq_data;

	pr_info("%s: Charger interrupt occured\n", __func__);

	if ((charger->pdata->full_check_type ==
				SEC_BATTERY_FULLCHARGED_CHGINT) ||
			(charger->pdata->ovp_uvlo_check_type ==
			 SEC_BATTERY_OVP_UVLO_CHGINT))
		schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

#if defined(CONFIG_WIRELESS_CHARGING)
static irqreturn_t wpc_charger_irq(int irq, void *data)
{
	struct max77693_charger_data *chg_data = data;
	int wc_w_state;
	union power_supply_propval value;
	pr_info("%s: irq(%d)\n", __func__, irq);

	/* check and unlock */
	check_charger_unlock_state(chg_data);

	wc_w_state = !gpio_get_value(chg_data->wc_w_gpio);
	if ((chg_data->wc_w_state == 0) && (wc_w_state == 1)) {
		value.intval = POWER_SUPPLY_TYPE_WIRELESS
					<<ONLINE_TYPE_MAIN_SHIFT;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		pr_info("%s: wpc activated, set V_INT as PN\n",
				__func__);
	} else if ((chg_data->wc_w_state == 1) && (wc_w_state == 0)) {
		value.intval =
			POWER_SUPPLY_TYPE_BATTERY<<ONLINE_TYPE_MAIN_SHIFT;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		pr_info("%s: wpc deactivated, set V_INT as PD\n",
				__func__);
	}
	pr_info("%s: w(%d to %d)\n", __func__,
			chg_data->wc_w_state, wc_w_state);

	chg_data->wc_w_state = wc_w_state;

	return IRQ_HANDLED;
}
#endif

static irqreturn_t max77693_bypass_irq(int irq, void *data)
{
	struct max77693_charger_data *chg_data = data;
	u8 dtls_02;
	u8 byp_dtls;
	u8 chg_cnfg_00;
	u8 vbus_state;

	pr_info("%s: irq(%d)\n", __func__, irq);

	/* check and unlock */
	check_charger_unlock_state(chg_data);

	max77693_read_reg(chg_data->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_02,
				&dtls_02);

	byp_dtls = ((dtls_02 & MAX77693_BYP_DTLS) >>
				MAX77693_BYP_DTLS_SHIFT);
	pr_info("%s: BYP_DTLS(0x%02x)\n", __func__, byp_dtls);
	vbus_state = max77693_get_vbus_state(chg_data);

	if (byp_dtls & 0x1) {
		pr_info("%s: bypass overcurrent limit\n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
		/* max77693_muic_host_notify_cb(0); */
#endif
		/* disable the register values just related to OTG and
		   keep the values about the charging */
		max77693_read_reg(chg_data->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		max77693_write_reg(chg_data->max77693->i2c,
					MAX77693_CHG_REG_CHG_CNFG_00,
					chg_cnfg_00);
	}
	if ((byp_dtls & 0x8) && (vbus_state < 0x03))
		reduce_input_current(chg_data, REDUCE_CURRENT_STEP);

	return IRQ_HANDLED;
}

static void max77693_chgin_isr_work(struct work_struct *work)
{
	struct max77693_charger_data *charger = container_of(work,
			struct max77693_charger_data, chgin_work);
	u8 reg_data;
	u8 chgin_dtls, chg_dtls;
	u8 prev_chgin_dtls = 0xff;
	int battery_health;
	union power_supply_propval value;
	int stable_count = 0;


	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_INT_MASK, &reg_data);
	reg_data |= (1 << 6);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_INT_MASK, reg_data);

	while (1) {
		psy_do_property("battery", get,
				POWER_SUPPLY_PROP_HEALTH, value);
		battery_health = value.intval;

		max77693_read_reg(charger->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_00,
				&chgin_dtls);
		chgin_dtls = ((chgin_dtls & MAX77693_CHGIN_DTLS) >>
				MAX77693_CHGIN_DTLS_SHIFT);
		max77693_read_reg(charger->max77693->i2c,
				MAX77693_CHG_REG_CHG_DTLS_01, &chg_dtls);
		chg_dtls = ((chg_dtls & MAX77693_CHG_DTLS) >>
				MAX77693_CHG_DTLS_SHIFT);
		if (prev_chgin_dtls == chgin_dtls)
			stable_count++;
		else
			stable_count = 0;
		if (stable_count > 10) {
			pr_info("%s: irq(%d), chgin(0x%x), prev 0x%x\n",
					__func__, charger->irq_chgin,
					chgin_dtls, prev_chgin_dtls);
			if (charger->is_charging) {
				if ((chgin_dtls == 0x02) && \
					(battery_health == POWER_SUPPLY_HEALTH_GOOD)) {
					pr_info("%s: charger is over voltage\n",
							__func__);
					value.intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
					psy_do_property("battery", set,
						POWER_SUPPLY_PROP_HEALTH, value);
				} else if ((battery_health == \
						POWER_SUPPLY_HEALTH_OVERVOLTAGE) &&
						(chgin_dtls != 0x02)) {
					pr_info("%s: charger is good\n", __func__);
					value.intval = POWER_SUPPLY_HEALTH_GOOD;
					psy_do_property("battery", set,
						POWER_SUPPLY_PROP_HEALTH, value);
				}
			}
			break;
		}

		if (charger->is_charging) {
			/* reduce only at CC MODE */
			if (((chgin_dtls == 0x0) || (chgin_dtls == 0x01)) &&
					(chg_dtls == 0x01) && (stable_count > 2))
				reduce_input_current(charger, REDUCE_CURRENT_STEP);
		}
		prev_chgin_dtls = chgin_dtls;
		msleep(100);
	}

	max77693_read_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_INT_MASK, &reg_data);
	reg_data &= ~(1 << 6);
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_INT_MASK, reg_data);
		
}

static irqreturn_t max77693_chgin_irq(int irq, void *data)
{
	struct max77693_charger_data *charger = data;
	queue_work(charger->wqueue, &charger->chgin_work);

	return IRQ_HANDLED;
}

/* register chgin isr after sec_battery_probe */
static void max77693_chgin_init_work(struct work_struct *work)
{
	struct max77693_charger_data *charger = container_of(work,
						struct max77693_charger_data,
						chgin_init_work.work);
	int ret;

	pr_info("%s \n", __func__);
	ret = request_threaded_irq(charger->irq_chgin, NULL,
			max77693_chgin_irq, 0, "chgin-irq", charger);
	if (ret < 0) {
		pr_err("%s: fail to request chgin IRQ: %d: %d\n",
				__func__, charger->irq_chgin, ret);
	}
}

static __devinit int max77693_charger_probe(struct platform_device *pdev)
{
	struct max77693_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct max77693_charger_data *charger;
	int ret = 0;
	u8 reg_data;

	pr_info("%s: MAX77693 Charger driver probe\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	charger->max77693 = iodev;
	charger->pdata = pdata->charger_data;
	charger->siop_level = 100;

	platform_set_drvdata(pdev, charger);

	charger->psy_chg.name           = "sec-charger";
	charger->psy_chg.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property   = sec_chg_get_property;
	charger->psy_chg.set_property   = sec_chg_set_property;
	charger->psy_chg.properties     = sec_charger_props;
	charger->psy_chg.num_properties = ARRAY_SIZE(sec_charger_props);

	mutex_init(&charger->ops_lock);

	if (charger->pdata->chg_gpio_init) {
		if (!charger->pdata->chg_gpio_init()) {
			pr_err("%s: Failed to Initialize GPIO\n", __func__);
			goto err_free;
		}
	}

	max77693_charger_initialize(charger);

	charger->wqueue =
	    create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!charger->wqueue) {
		pr_err("%s: Fail to Create Workqueue\n", __func__);
		goto err_free;
	}
	INIT_WORK(&charger->chgin_work, max77693_chgin_isr_work);
	INIT_DELAYED_WORK(&charger->chgin_init_work, max77693_chgin_init_work);
	wake_lock_init(&charger->recovery_wake_lock, WAKE_LOCK_SUSPEND,
					       "charger-recovery");
	INIT_DELAYED_WORK(&charger->recovery_work, max77693_recovery_work);

	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_power_supply_register;
	}

	if (charger->pdata->chg_irq) {
		INIT_DELAYED_WORK_DEFERRABLE(
				&charger->isr_work, sec_chg_isr_work);
		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, sec_chg_irq_thread,
				charger->pdata->chg_irq_attr,
				"charger-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Reqeust IRQ\n", __func__);
			goto err_irq;
		}
	}

#if defined(CONFIG_WIRELESS_CHARGING)
	charger->wc_w_gpio = pdata->wc_irq_gpio;
	if (charger->wc_w_gpio) {
		charger->wc_w_irq = gpio_to_irq(charger->wc_w_gpio);
		ret = gpio_request(charger->wc_w_gpio, "wpc_charger-irq");
		if (ret < 0) {
			pr_err("%s: failed requesting gpio %d\n", __func__,
				charger->wc_w_gpio);
			goto err_wc_irq;
		}
		ret = request_threaded_irq(charger->wc_w_irq,
				NULL, wpc_charger_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT,
				"wpc-int", charger);
		if (ret) {
			pr_err("%s: Failed to Reqeust IRQ\n", __func__);
			goto err_wc_irq;
		}
		enable_irq_wake(charger->wc_w_irq);
		charger->wc_w_state = !gpio_get_value(charger->wc_w_gpio);
	}
#endif

	charger->irq_chgin = pdata->irq_base + MAX77693_CHG_IRQ_CHGIN_I;
	/* enable chgin irq after sec_battery_probe */
	queue_delayed_work(charger->wqueue, &charger->chgin_init_work,
			msecs_to_jiffies(3000));

	charger->irq_bypass = pdata->irq_base + MAX77693_CHG_IRQ_BYP_I;
	ret = request_threaded_irq(charger->irq_bypass, NULL,
			max77693_bypass_irq, 0, "bypass-irq", charger);
	if (ret < 0)
		pr_err("%s: fail to request bypass IRQ: %d: %d\n",
				__func__, charger->irq_bypass, ret);

	max77693_dentry = debugfs_create_file("max77693-regs",
			S_IRUSR, NULL, charger, &max77693_debugfs_fops);
	return 0;
err_wc_irq:
	free_irq(charger->pdata->chg_irq, NULL);
err_irq:
	power_supply_unregister(&charger->psy_chg);
err_power_supply_register:
	destroy_workqueue(charger->wqueue);
err_free:
	kfree(charger);

	return ret;

}

static int __devexit max77693_charger_remove(struct platform_device *pdev)
{
	struct max77693_charger_data *charger =
				platform_get_drvdata(pdev);

	if (!IS_ERR_OR_NULL(max77693_dentry))
		debugfs_remove(max77693_dentry);

	destroy_workqueue(charger->wqueue);
#if defined(CONFIG_WIRELESS_CHARGING)
	if (charger->wc_w_irq)
		free_irq(charger->wc_w_irq, NULL);
#endif
	if (charger->pdata->chg_irq)
		free_irq(charger->pdata->chg_irq, NULL);

	power_supply_unregister(&charger->psy_chg);
	kfree(charger);

	return 0;
}

#if defined CONFIG_PM
static int max77693_charger_suspend(struct device *dev)
{
	return 0;
}

static int max77693_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define max77693_charger_suspend NULL
#define max77693_charger_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(max77693_charger_pm_ops, max77693_charger_suspend,
		max77693_charger_resume);

void max77693_charger_shutdown(struct device *dev)
{
	struct max77693_charger_data *charger =
				dev_get_drvdata(dev);
	u8 reg_data;

	if (!charger->max77693->i2c) {
		pr_err("%s: no max77693 i2c client\n", __func__);
		return;
	}
	reg_data = 0x04;
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_00, reg_data);
	reg_data = 0x19;
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_09, reg_data);
	reg_data = 0x19;
	max77693_write_reg(charger->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_10, reg_data);
	pr_info("func:%s \n", __func__);
}

static struct platform_driver max77693_charger_driver = {
	.driver = {
		.name = "max77693-charger",
		.owner = THIS_MODULE,
		.pm = &max77693_charger_pm_ops,
		.shutdown = max77693_charger_shutdown,
	},
	.probe = max77693_charger_probe,
	.remove = __devexit_p(max77693_charger_remove),
};

static int __init max77693_charger_init(void)
{
	pr_info("func:%s\n", __func__);
	return platform_driver_register(&max77693_charger_driver);
}
module_init(max77693_charger_init);

static void __exit max77693_charger_exit(void)
{
	platform_driver_register(&max77693_charger_driver);
}

module_exit(max77693_charger_exit);

MODULE_DESCRIPTION("max77693 charger driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
