/*
 * linux/arch/arm/mach-exynos/midas-tsp.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>

#include <linux/err.h>
#include <linux/gpio.h>
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224_U1)
#include <linux/delay.h>
#include <linux/i2c/mxt224_u1.h>
#elif defined(CONFIG_TOUCHSCREEN_MELFAS_GC)
#include <linux/platform_data/mms_ts_gc.h>
#elif defined(CONFIG_TOUCHSCREEN_ATMEL_MXT540E)
#include <linux/delay.h>
#include <linux/i2c/mxt540e.h>
#elif defined(CONFIG_TOUCHSCREEN_MELFAS_NOTE)
#include <linux/platform_data/mms152_ts.h>
#elif defined(CONFIG_TOUCHSCREEN_ATMEL_MXT540S)
#include <linux/i2c/mxt540s.h>
#elif defined(CONFIG_TOUCHSCREEN_CYTTSP4)
#include <linux/platform_data/cypress_cyttsp4.h>
#include <linux/delay.h>
#include <linux/input.h>

#else
#include <linux/platform_data/mms_ts.h>
#endif
#include <linux/regulator/consumer.h>
#include <plat/gpio-cfg.h>

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
#include <linux/cpufreq.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224_U1)
/* mxt224 TSP */
extern bool is_cable_attached;

static struct charging_status_callbacks {
	void (*tsp_set_charging_cable) (int type);
} charging_cbs;

void tsp_register_callback(void *function)
{
	charging_cbs.tsp_set_charging_cable = function;
}

void tsp_read_ta_status(void *ta_status)
{
	*(bool *) ta_status = is_cable_attached;
}

static void mxt224_power_on(void)
{
	struct regulator *regulator;

	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator))
		return;

	regulator_enable(regulator);
	printk(KERN_INFO "[TSP] melfas power on\n");

	regulator_put(regulator);

	mdelay(70);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	mdelay(40);

	printk(KERN_INFO "mxt224_power_on is finished\n");
}

EXPORT_SYMBOL(mxt224_power_on);

static void mxt224_power_off(void)
{
	struct regulator *regulator;

	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_DOWN);

	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator))
		return;

	regulator_disable(regulator);

	regulator_put(regulator);
	printk(KERN_INFO "mxt224_power_off is finished\n");
}

EXPORT_SYMBOL(mxt224_power_off);

/*
  Configuration for MXT224
*/
#define MXT224_THRESHOLD_BATT		40
#define MXT224_THRESHOLD_BATT_INIT		55
#define MXT224_THRESHOLD_CHRG		70
#define MXT224_NOISE_THRESHOLD_BATT		30
#define MXT224_NOISE_THRESHOLD_CHRG		40
#define MXT224_MOVFILTER_BATT		11
#define MXT224_MOVFILTER_CHRG		46
#define MXT224_ATCHCALST		9
#define MXT224_ATCHCALTHR		30

static u8 t7_config[] = { GEN_POWERCONFIG_T7,
	48,	/* IDLEACQINT */
	255,	/* ACTVACQINT */
	25	/* ACTV2IDLETO: 25 * 200ms = 5s */
};

static u8 t8_config[] = { GEN_ACQUISITIONCONFIG_T8,
	10, 0, 5, 1, 0, 0, MXT224_ATCHCALST, MXT224_ATCHCALTHR
};				/*byte 3: 0 */

static u8 t9_config[] = { TOUCH_MULTITOUCHSCREEN_T9,
	131, 0, 0, 19, 11, 0, 32, MXT224_THRESHOLD_BATT, 2, 1,
	0,
	15,			/* MOVHYSTI */
	1, MXT224_MOVFILTER_BATT, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 0, 0, 0, 0, 143, 55, 143, 90, 18
};

static u8 t18_config[] = { SPT_COMCONFIG_T18,
	0, 1
};

static u8 t20_config[] = { PROCI_GRIPFACESUPPRESSION_T20,
	7, 0, 0, 0, 0, 0, 0, 30, 20, 4, 15, 10
};

static u8 t22_config[] = { PROCG_NOISESUPPRESSION_T22,
	143, 0, 0, 0, 0, 0, 0, 3, MXT224_NOISE_THRESHOLD_BATT, 0, 0, 29, 34, 39,
	49, 58, 3
};

static u8 t28_config[] = { SPT_CTECONFIG_T28,
	0, 0, 3, 16, 19, 60
};
static u8 end_config[] = { RESERVED_T255 };

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

/*
  Configuration for MXT224-E
*/
#define MXT224E_THRESHOLD_BATT		50
#define MXT224E_THRESHOLD_CHRG		40
#define MXT224E_CALCFG_BATT		0x42
#define MXT224E_CALCFG_CHRG		0x52
#define MXT224E_ATCHFRCCALTHR_NORMAL		40
#define MXT224E_ATCHFRCCALRATIO_NORMAL		55
#define MXT224E_GHRGTIME_BATT		27
#define MXT224E_GHRGTIME_CHRG		22
#define MXT224E_ATCHCALST		4
#define MXT224E_ATCHCALTHR		35
#define MXT224E_BLEN_BATT		32
#define MXT224E_BLEN_CHRG		16
#define MXT224E_MOVFILTER_BATT		46
#define MXT224E_MOVFILTER_CHRG		46
#define MXT224E_ACTVSYNCSPERX_NORMAL		32
#define MXT224E_NEXTTCHDI_NORMAL		0

#if defined(CONFIG_TARGET_LOCALE_NAATT)
static u8 t7_config_e[] = { GEN_POWERCONFIG_T7,
	48, 255, 25
};

static u8 t8_config_e[] = { GEN_ACQUISITIONCONFIG_T8,
	27, 0, 5, 1, 0, 0, 8, 8, 8, 180
};

/* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	139, 0, 0, 19, 11, 0, 32, 50, 2, 1,
	10, 3, 1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
	18, 15, 50, 50, 2
};

static u8 t15_config_e[] = { TOUCH_KEYARRAY_T15,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t18_config_e[] = { SPT_COMCONFIG_T18,
	0, 0
};

static u8 t23_config_e[] = { TOUCH_PROXIMITY_T23,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t25_config_e[] = { SPT_SELFTEST_T25,
	0, 0, 188, 52, 124, 21, 188, 52, 124, 21, 0, 0, 0, 0
};

static u8 t40_config_e[] = { PROCI_GRIPSUPPRESSION_T40,
	0, 0, 0, 0, 0
};

static u8 t42_config_e[] = { PROCI_TOUCHSUPPRESSION_T42,
	0, 32, 120, 100, 0, 0, 0, 0
};

static u8 t46_config_e[] = { SPT_CTECONFIG_T46,
	0, 3, 16, 35, 0, 0, 1, 0
};

static u8 t47_config_e[] = { PROCI_STYLUS_T47,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*MXT224E_0V5_CONFIG */
static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 0, 0, 100, 4, 64, 10, 0, 20, 5, 0, 38, 0, 5,
	0, 0, 0, 0, 0, 0, 32, 50, 2, 3, 1, 11, 10, 5, 40, 10, 10,
	10, 10, 143, 40, 143, 80, 18, 15, 2
};

static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	1, 4, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 0, 0, 100, 4, 64, 10, 0, 20, 5, 0, 38, 0, 20,
	0, 0, 0, 0, 0, 0, 16, 70, 2, 5, 2, 46, 10, 5, 40, 10, 0,
	10, 10, 143, 40, 143, 80, 18, 15, 2
};

#elif defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00)
static u8 t7_config_e[] = { GEN_POWERCONFIG_T7,
	48, 255, 15
};

static u8 t8_config_e[] = { GEN_ACQUISITIONCONFIG_T8,
	27, 0, 5, 1, 0, 0, 4, 35, 40, 55
};

static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	131, 0, 0, 19, 11, 0, 32, 50, 2, 7,
	10, 3, 1, 46, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
	18, 15, 50, 50, 2
};

static u8 t15_config_e[] = { TOUCH_KEYARRAY_T15,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t18_config_e[] = { SPT_COMCONFIG_T18,
	0, 0
};

static u8 t23_config_e[] = { TOUCH_PROXIMITY_T23,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t25_config_e[] = { SPT_SELFTEST_T25,
	0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t40_config_e[] = { PROCI_GRIPSUPPRESSION_T40,
	0, 0, 0, 0, 0
};

static u8 t42_config_e[] = { PROCI_TOUCHSUPPRESSION_T42,
	0, 32, 120, 100, 0, 0, 0, 0
};

static u8 t46_config_e[] = { SPT_CTECONFIG_T46,
	0, 3, 16, 48, 0, 0, 1, 0, 0
};

static u8 t47_config_e[] = { PROCI_STYLUS_T47,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 4, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 5, 0, 0,
	0, 0, 0, 0, 32, 50, 2, 3, 1, 46,
	10, 5, 40, 10, 10, 10, 10, 143, 40, 143,
	80, 18, 15, 2
};

static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	1, 4, 80, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 70, 2, 5, 2, 46,
	10, 5, 40, 10, 0, 10, 10, 143, 40, 143,
	80, 18, 15, 2
};
#else

static u8 t7_config_e[] = { GEN_POWERCONFIG_T7,
	48,			/* IDLEACQINT */
	255,			/* ACTVACQINT */
	25			/* ACTV2IDLETO: 25 * 200ms = 5s */
};

static u8 t8_config_e[] = { GEN_ACQUISITIONCONFIG_T8,
	MXT224E_GHRGTIME_BATT, 0, 5, 1, 0, 0,
	MXT224E_ATCHCALST, MXT224E_ATCHCALTHR,
	MXT224E_ATCHFRCCALTHR_NORMAL,
	MXT224E_ATCHFRCCALRATIO_NORMAL
};

/* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
#ifdef CONFIG_TARGET_LOCALE_NA
#ifdef CONFIG_MACH_U1_NA_USCC_REV05
static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	139, 0, 0, 19, 11, 0, MXT224E_BLEN_BATT, MXT224E_THRESHOLD_BATT, 2, 1,
	10,
	10,			/* MOVHYSTI */
	1, MXT224E_MOVFILTER_BATT, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
	18, 15, 50, 50, 0
};

#else
static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	139, 0, 0, 19, 11, 0, MXT224E_BLEN_BATT, MXT224E_THRESHOLD_BATT, 2, 1,
	10,
	10,			/* MOVHYSTI */
	1, MXT224E_MOVFILTER_BATT, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
	18, 15, 50, 50, 2
};
#endif
#else
static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	139, 0, 0, 19, 11, 0, MXT224E_BLEN_BATT, MXT224E_THRESHOLD_BATT, 2, 1,
	10,
	15,			/* MOVHYSTI */
	1, MXT224E_MOVFILTER_BATT, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
	223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
	18, 15, 50, 50, MXT224E_NEXTTCHDI_NORMAL
};
#endif

static u8 t15_config_e[] = { TOUCH_KEYARRAY_T15,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t18_config_e[] = { SPT_COMCONFIG_T18,
	0, 0
};

static u8 t23_config_e[] = { TOUCH_PROXIMITY_T23,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t25_config_e[] = { SPT_SELFTEST_T25,
	0, 0, 0, 0, 0, 0, 0, 0
};

#ifdef CONFIG_MACH_U1_NA_USCC_REV05
static u8 t38_config_e[] = { SPT_USERDATA_T38,
	0, 1, 13, 19, 44, 0, 0, 0
};
#else
static u8 t38_config_e[] = { SPT_USERDATA_T38,
	0, 1, 14, 23, 44, 0, 0, 0
};
#endif

static u8 t40_config_e[] = { PROCI_GRIPSUPPRESSION_T40,
	0, 0, 0, 0, 0
};

static u8 t42_config_e[] = { PROCI_TOUCHSUPPRESSION_T42,
	0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t46_config_e[] = { SPT_CTECONFIG_T46,
	0, 3, 16, MXT224E_ACTVSYNCSPERX_NORMAL, 0, 0, 1, 0, 0
};

static u8 t47_config_e[] = { PROCI_STYLUS_T47,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*MXT224E_0V5_CONFIG */
#ifdef CONFIG_TARGET_LOCALE_NA
#ifdef CONFIG_MACH_U1_NA_USCC_REV05
static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, 0x52, 0, 0, 0, 0, 0, 10, 15,
	0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
	10, 0, 10, 5, 0, 19, 0, 20, 0, 0,
	0, 0, 0, 0, 0, 40, 2,	/*blen=0,threshold=50 */
	10,			/* MOVHYSTI */
	1, 47,
	10, 5, 40, 240, 245, 10, 10, 148, 50, 143,
	80, 18, 10, 0
};

static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, 0x40, 0, 0, 0, 0, 0, 10, 15,
	0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 5, 0, 0,	/*byte 27 original value 20 */
	0, 0, 0, 0, 32, MXT224E_THRESHOLD, 2,
	10,
	1, 46,
	MXT224_MAX_MT_FINGERS, 5, 40, 10, 0, 10, 10, 143, 40, 143,
	80, 18, 15, 0
};
#else
static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	1, 4, 0x50, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 0, 40, 2,	/*blen=0,threshold=50 */
	10,			/* MOVHYSTI */
	1, 15,
	10, 5, 40, 240, 245, 10, 10, 148, 50, 143,
	80, 18, 10, 2
};

static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	1, 4, 0x40, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 5, 0, 0,	/*byte 27 original value 20 */
	0, 0, 0, 0, 32, 50, 2,
	10,
	1, 46,
	MXT224_MAX_MT_FINGERS, 5, 40, 10, 0, 10, 10, 143, 40, 143,
	80, 18, 15, 2
};
#endif /*CONFIG_MACH_U1_NA_USCC_REV05 */
#else
static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, MXT224E_CALCFG_CHRG, 0, 0, 0, 0, 0, 10, 15,
	0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
	10, 0, 9, 5, 0, 15, 0, 20, 0, 0,
	0, 0, 0, 0, 0, MXT224E_THRESHOLD_CHRG, 2,
	15,			/* MOVHYSTI */
	1, 47,
	MXT224_MAX_MT_FINGERS, 5, 40, 235, 235, 10, 10, 160, 50, 143,
	80, 18, 10, 0
};

static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, MXT224E_CALCFG_BATT, 0, 0, 0, 0, 0, 10, 15,
	0, 0, 0, 6, 6, 0, 0, 48, 4, 48,
	10, 0, 10, 5, 0, 20, 0, 5, 0, 0,	/*byte 27 original value 20 */
	0, 0, 0, 0, 32, MXT224E_THRESHOLD_BATT, 2,
	15,
	1, 46,
	MXT224_MAX_MT_FINGERS, 5, 40, 10, 10, 10, 10, 143, 40, 143,
	80, 18, 15, 0
};
#endif /*CONFIG_TARGET_LOCALE_NA */
#endif /*CONFIG_TARGET_LOCALE_NAATT */

static u8 end_config_e[] = { RESERVED_T255 };

static const u8 *mxt224e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t23_config_e,
	t25_config_e,
	t38_config_e,
	t40_config_e,
	t42_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	end_config_e,
};

static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TSP_INT,
	.config = mxt224_config,
	.config_e = mxt224e_config,
	.t48_config_batt_e = t48_config_e,
	.t48_config_chrg_e = t48_config_chrg_e,
	.min_x = 0,
	.max_x = 479,
	.min_y = 0,
	.max_y = 799,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.atchcalst = MXT224_ATCHCALST,
	.atchcalsthr = MXT224_ATCHCALTHR,
	.tchthr_batt = MXT224_THRESHOLD_BATT,
	.tchthr_batt_init = MXT224_THRESHOLD_BATT_INIT,
	.tchthr_charging = MXT224_THRESHOLD_CHRG,
	.noisethr_batt = MXT224_NOISE_THRESHOLD_BATT,
	.noisethr_charging = MXT224_NOISE_THRESHOLD_CHRG,
	.movfilter_batt = MXT224_MOVFILTER_BATT,
	.movfilter_charging = MXT224_MOVFILTER_CHRG,
	.atchcalst_e = MXT224E_ATCHCALST,
	.atchcalsthr_e = MXT224E_ATCHCALTHR,
	.tchthr_batt_e = MXT224E_THRESHOLD_BATT,
	.tchthr_charging_e = MXT224E_THRESHOLD_CHRG,
	.calcfg_batt_e = MXT224E_CALCFG_BATT,
	.calcfg_charging_e = MXT224E_CALCFG_CHRG,
	.atchfrccalthr_e = MXT224E_ATCHFRCCALTHR_NORMAL,
	.atchfrccalratio_e = MXT224E_ATCHFRCCALRATIO_NORMAL,
	.chrgtime_batt_e = MXT224E_GHRGTIME_BATT,
	.chrgtime_charging_e = MXT224E_GHRGTIME_CHRG,
	.blen_batt_e = MXT224E_BLEN_BATT,
	.blen_charging_e = MXT224E_BLEN_CHRG,
	.movfilter_batt_e = MXT224E_MOVFILTER_BATT,
	.movfilter_charging_e = MXT224E_MOVFILTER_CHRG,
	.actvsyncsperx_e = MXT224E_ACTVSYNCSPERX_NORMAL,
	.nexttchdi_e = MXT224E_NEXTTCHDI_NORMAL,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
	.register_cb = tsp_register_callback,
	.read_ta_status = tsp_read_ta_status,
};

void mxt224_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void mxt224_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
}

/* I2C3 */
static struct i2c_board_info i2c_devs3[] __initdata = {
	{
	 I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4a),
	 .platform_data = &mxt224_data},
};

#ifndef CONFIG_MACH_NEWTON_BD
void midas_tsp_set_platdata(struct mxt224_platform_data *pdata)
{
	if (!pdata)
		pdata = &mxt224_data;

	i2c_devs3[0].platform_data = pdata;
}
#endif

void __init midas_tsp_init(void)
{
#ifndef CONFIG_MACH_NEWTON_BD
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);
#endif
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#elif defined(CONFIG_TOUCHSCREEN_MELFAS_GC)

static bool enabled;
int melfas_power(int on)
{
	struct regulator *regulator_pwr;
	struct regulator *regulator_vdd;
	int ret = 0;

	if (enabled == on) {
		pr_err("melfas-ts : %s same state!", __func__);
		return 0;
	}

	regulator_pwr = regulator_get(NULL, "touch");
	regulator_vdd = regulator_get(NULL, "touch_1.8v");

	if (IS_ERR(regulator_pwr)) {
		pr_err("melfas-ts : %s regulator_pwr error!", __func__);
		return PTR_ERR(regulator_pwr);
	}
	if (IS_ERR(regulator_vdd)) {
		pr_err("melfas-ts : %s regulator_vdd error!", __func__);
		return PTR_ERR(regulator_vdd);
	}

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_pwr);
	} else {
		if (regulator_is_enabled(regulator_pwr))
			regulator_disable(regulator_pwr);
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
	}

	if (regulator_is_enabled(regulator_pwr) == !!on &&
		regulator_is_enabled(regulator_vdd) == !!on) {
		pr_info("melfas-ts : %s %s", __func__, !!on ? "ON" : "OFF");
		enabled = on;
	} else {
		pr_err("melfas-ts : regulator_is_enabled value error!");
		ret = -1;
	}

	regulator_put(regulator_vdd);
	regulator_put(regulator_pwr);

	return ret;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("melfas-ts : %s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		/*s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT); */
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_info("melfas-ts : melfas_register_callback");
}

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 720,
	.max_y = 1280,
	.invert_x = 0,
	.invert_y = 0,
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.tsp_vendor = "MELFAS",
	.tsp_ic	= "MMS144",
	.tsp_tx = 26,	/* TX_NUM (Reg Addr : 0xEF) */
	.tsp_rx = 14,	/* RX_NUM (Reg Addr : 0xEE) */
/*	.fw_version = 0x02, */
	.config_fw_version = "GC100_Me_0829",
	.register_cb = melfas_register_callback,
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	pr_info("melfas-ts : GC TSP init() is called : [%d]", system_rev);

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("melfas-ts : failed to request gpio(TSP_INT)");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	pr_info("melfas-ts : %s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#elif defined(CONFIG_TOUCHSCREEN_ATMEL_MXT540E)
static struct charging_status_callbacks {
	void (*tsp_set_charging_cable) (int type);
} charging_cbs;

void tsp_register_callback(void *function)
{
	charging_cbs.tsp_set_charging_cable = function;
}

void tsp_read_ta_status(void *ta_status)
{
	*(bool *) ta_status = is_cable_attached;
}

void tsp_charger_infom(bool en)
{
	if (charging_cbs.tsp_set_charging_cable)
		charging_cbs.tsp_set_charging_cable(en);
}

void __init midas_tsp_set_lcdtype(int lcd_type)
{
}

static void mxt540e_power_on(void)
{
	struct regulator *regulator_avdd;
	struct regulator *regulator_vdd;

	regulator_avdd = regulator_get(NULL, "touch");
	regulator_vdd = regulator_get(NULL, "touch_1.8v");

	if (IS_ERR(regulator_avdd)) {
		pr_err("[TSP] %s regulator_avdd error!", __func__);
		return;
	}
	if (IS_ERR(regulator_vdd)) {
		pr_err("[TSP] %s regulator_vdd error!", __func__);
		return;
	}

	s3c_gpio_cfgpin(GPIO_TSP_LDO_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_EN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_TSP_LDO_EN, GPIO_LEVEL_HIGH);

	regulator_enable(regulator_vdd);
	regulator_enable(regulator_avdd);
	if (regulator_is_enabled(regulator_avdd) &&
		regulator_is_enabled(regulator_vdd)) {
		pr_info("[TSP] %s", __func__);
	} else {
		pr_err("[TSP] %s fail", __func__);
	}
	regulator_put(regulator_vdd);
	regulator_put(regulator_avdd);

	msleep(MXT540E_HW_RESET_TIME);

	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

static void mxt540e_power_off(void)
{
	struct regulator *regulator_avdd;
	struct regulator *regulator_vdd;

	regulator_avdd = regulator_get(NULL, "touch");
	regulator_vdd = regulator_get(NULL, "touch_1.8v");

	if (IS_ERR(regulator_avdd)) {
		pr_err("[TSP] %s regulator_avdd error!", __func__);
		return;
	}
	if (IS_ERR(regulator_vdd)) {
		pr_err("[TSP] %s regulator_vdd error!", __func__);
		return;
	}

	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_TSP_LDO_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_EN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_TSP_LDO_EN, GPIO_LEVEL_LOW);

	if (regulator_is_enabled(regulator_avdd))
		regulator_disable(regulator_avdd);
	if (regulator_is_enabled(regulator_vdd))
		regulator_disable(regulator_vdd);
	if (!regulator_is_enabled(regulator_avdd) &&
		!regulator_is_enabled(regulator_vdd)) {
		pr_info("[TSP] %s", __func__);
	} else {
		pr_err("[TSP] %s fail", __func__);
	}
	regulator_put(regulator_vdd);
	regulator_put(regulator_avdd);
}

/*
  Configuration for MXT540E
*/
#define MXT540E_MAX_MT_FINGERS		10
#define MXT540E_CHRGTIME_BATT		48
#define MXT540E_CHRGTIME_CHRG		48
#define MXT540E_THRESHOLD_BATT		50
#define MXT540E_THRESHOLD_CHRG		40
#define MXT540E_ACTVSYNCSPERX_BATT		34
#define MXT540E_ACTVSYNCSPERX_CHRG		34
#define MXT540E_CALCFG_BATT		98
#define MXT540E_CALCFG_CHRG		114
#define MXT540E_ATCHFRCCALTHR_WAKEUP		8
#define MXT540E_ATCHFRCCALRATIO_WAKEUP		180
#define MXT540E_ATCHFRCCALTHR_NORMAL		40
#define MXT540E_ATCHFRCCALRATIO_NORMAL		55

static u8 t7_config_e[] = { GEN_POWERCONFIG_T7,
	48, 255, 50
};

static u8 t8_config_e[] = { GEN_ACQUISITIONCONFIG_T8,
	MXT540E_CHRGTIME_BATT, 0, 5, 1, 0, 0, 4, 20,
	MXT540E_ATCHFRCCALTHR_WAKEUP, MXT540E_ATCHFRCCALRATIO_WAKEUP
};

static u8 t9_config_e[] = { TOUCH_MULTITOUCHSCREEN_T9,
	131, 0, 0, 16, 26, 0, 192, MXT540E_THRESHOLD_BATT, 2, 6,
	10, 10, 10, 80, MXT540E_MAX_MT_FINGERS, 20, 40, 20, 31, 3,
	255, 4, 3, 3, 2, 2, 136, 60, 136, 40,
	18, 15, 0, 0, 0
};

static u8 t15_config_e[] = { TOUCH_KEYARRAY_T15,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t18_config_e[] = { SPT_COMCONFIG_T18,
	0, 0
};

static u8 t19_config_e[] = { SPT_GPIOPWM_T19,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t24_config_e[] = { PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t25_config_e[] = { SPT_SELFTEST_T25,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t27_config_e[] = { PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	0, 0, 0, 0, 0, 0, 0
};

static u8 t40_config_e[] = { PROCI_GRIPSUPPRESSION_T40,
	0, 0, 0, 0, 0
};

static u8 t42_config_e[] = { PROCI_TOUCHSUPPRESSION_T42,
	0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t43_config_e[] = { SPT_DIGITIZER_T43,
	0, 0, 0, 0, 0, 0, 0
};

static u8 t46_config_e[] = { SPT_CTECONFIG_T46,
	0, 0, 16, MXT540E_ACTVSYNCSPERX_BATT, 0, 0, 1, 0
};

static u8 t47_config_e[] = { PROCI_STYLUS_T47,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t48_config_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, MXT540E_CALCFG_BATT, 0, 0, 0, 0, 0, 1, 2,
	0, 0, 0, 6, 6, 0, 0, 28, 4, 64,
	10, 0, 20, 6, 0, 30, 0, 0, 0, 0,
	0, 0, 0, 0, 192, MXT540E_THRESHOLD_BATT, 2, 10, 10, 47,
	MXT540E_MAX_MT_FINGERS, 5, 20, 253, 0, 7, 7, 160, 55, 136,
	0, 18, 5, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static u8 t48_config_chrg_e[] = { PROCG_NOISESUPPRESSION_T48,
	3, 132, MXT540E_CALCFG_CHRG, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 36, 4, 64,
	10, 0, 10, 6, 0, 20, 0, 0, 0, 0,
	0, 0, 0, 0, 112, MXT540E_THRESHOLD_CHRG, 2, 10, 5, 65,
	MXT540E_MAX_MT_FINGERS, 5, 20, 253, 0, 7, 7, 160, 55, 136,
	0, 18, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static u8 t52_config_e[] = { TOUCH_PROXKEY_T52,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t55_config_e[] = {ADAPTIVE_T55,
	0, 0, 0, 0, 0, 0
};

static u8 t57_config_e[] = {SPT_GENERICDATA_T57,
	243, 25, 1
};

static u8 t61_config_e[] = {SPT_TIMER_T61,
	0, 0, 0, 0, 0
};

static u8 end_config_e[] = { RESERVED_T255 };

static const u8 *mxt540e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t19_config_e,
	t24_config_e,
	t25_config_e,
	t27_config_e,
	t40_config_e,
	t42_config_e,
	t43_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	t52_config_e,
	t55_config_e,
	t57_config_e,
	t61_config_e,
	end_config_e,
};

struct mxt540e_platform_data mxt540e_data = {
	.max_finger_touches = MXT540E_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TSP_INT,
	.config_e = mxt540e_config,
	.min_x = 0,
	.max_x = 799,
	.min_y = 0,
	.max_y = 1279,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.chrgtime_batt = MXT540E_CHRGTIME_BATT,
	.chrgtime_charging = MXT540E_CHRGTIME_CHRG,
	.tchthr_batt = MXT540E_THRESHOLD_BATT,
	.tchthr_charging = MXT540E_THRESHOLD_CHRG,
	.actvsyncsperx_batt = MXT540E_ACTVSYNCSPERX_BATT,
	.actvsyncsperx_charging = MXT540E_ACTVSYNCSPERX_CHRG,
	.calcfg_batt_e = MXT540E_CALCFG_BATT,
	.calcfg_charging_e = MXT540E_CALCFG_CHRG,
	.atchfrccalthr_e = MXT540E_ATCHFRCCALTHR_NORMAL,
	.atchfrccalratio_e = MXT540E_ATCHFRCCALRATIO_NORMAL,
	.t48_config_batt_e = t48_config_e,
	.t48_config_chrg_e = t48_config_chrg_e,
	.power_on = mxt540e_power_on,
	.power_off = mxt540e_power_off,
	.power_on_with_oleddet = mxt540e_power_on,
	.power_off_with_oleddet = mxt540e_power_off,
	.register_cb = tsp_register_callback,
	.read_ta_status = tsp_read_ta_status,
};

/* I2C3 */
static struct i2c_board_info i2c_devs3[] __initdata = {
	{
	 I2C_BOARD_INFO(MXT540E_DEV_NAME, 0x4c),
	 .platform_data = &mxt540e_data},
};

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	pr_info("[TSP] T0 TSP init() is called");

	gpio = GPIO_TSP_LDO_EN;
	gpio_request(gpio, "TSP_LDO_EN");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("[TSP] failed to request gpio(TSP_INT)");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	pr_info("[TSP] T0 %s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#elif defined(CONFIG_TOUCHSCREEN_ATMEL_MXT540S)
#define MXT_FIRMWARE_540S	"tsp_atmel/mXT540S.fw"

struct mxt_callbacks *charger_callbacks;

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void ts_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] ts_register_callback\n");
}

int get_lcd_type;
void __init midas_tsp_set_lcdtype(int lcd_type)
{
	get_lcd_type = lcd_type;
}

static int ts_power_on(void)
{
	struct regulator *regulator;

	/* enable I2C pullup */
	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_on : tsp_vdd regulator_get failed\n");
		return -EIO;
	}
	regulator_enable(regulator);
	regulator_put(regulator);

	/* enable DVDD */
	s3c_gpio_cfgpin(GPIO_TSP_LDO_28V_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_28V_EN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_TSP_LDO_28V_EN, GPIO_LEVEL_HIGH);

	/* enable AVDD */
	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_on : tsp_avdd regulator_get failed\n");
		return -EIO;
	}
	regulator_enable(regulator);
	regulator_put(regulator);

	/* touch interrupt pin */
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

	printk(KERN_ERR "mxt_power_on is finished\n");

	return 0;
}

static int ts_power_off(void)
{
	struct regulator *regulator;

	/* disable AVDD */
	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_off : tsp_avdd regulator_get failed\n");
		return -EIO;
	}

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);
	regulator_put(regulator);

	/* disable DVDD */
	s3c_gpio_cfgpin(GPIO_TSP_LDO_28V_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_28V_EN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_TSP_LDO_28V_EN, GPIO_LEVEL_LOW);

	/* disable I2C pullup */
	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_off : tsp_vdd regulator_get failed\n");
		return -EIO;
	}

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);
	regulator_put(regulator);

	/* touch interrupt pin */
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

	printk(KERN_ERR "mxt_power_off is finished\n");

	return 0;
}

static int ts_power_reset(void)
{
	return 0;
}

static void ts_gpio_init(void)
{
	gpio_request(GPIO_TSP_LDO_28V_EN, "TSP_LDO_28V_EN");
	gpio_direction_output(GPIO_TSP_LDO_28V_EN, GPIO_LEVEL_LOW);
	gpio_export(GPIO_TSP_LDO_28V_EN, 0);

	/* touch interrupt */
	gpio_request(GPIO_TSP_INT, "TSP_INT");
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(GPIO_TSP_INT);
}

static struct mxt_platform_data mxt_data = {
	.max_finger_touches = 10,
	.gpio_read_done = GPIO_TSP_INT,
	.min_x = 0,
	.max_x = 4095,
	.min_y = 0,
	.max_y = 4095,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 255,
	.power_on = ts_power_on,
	.power_off = ts_power_off,
	.power_reset = ts_power_reset,
	.boot_address = 0x24,
	.firmware_name = MXT_FIRMWARE_540S,
	.register_cb = ts_register_callback,
};

static struct i2c_board_info i2c_devs3[] __initdata = {
	{
		I2C_BOARD_INFO(MXT_DEV_NAME, 0x4a),
		.platform_data = &mxt_data,
	}
};

void __init midas_tsp_init(void)
{
	ts_gpio_init();

	i2c_devs3[0].irq = gpio_to_irq(GPIO_TSP_INT);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

	printk(KERN_ERR "%s touch : %d\n",
		 __func__, i2c_devs3[0].irq);
}

#elif defined(CONFIG_TOUCHSCREEN_MELFAS_NOTE)
/* MELFAS TSP(T0) */
static bool enabled;
int TSP_VDD_18V(int on)
{
	struct regulator *regulator;

	if (enabled == on)
		return 0;

	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on) {
		regulator_enable(regulator);
		/*printk(KERN_INFO "[TSP] melfas power on\n"); */
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	enabled = on;
	regulator_put(regulator);

	return 0;
}

int melfas_power(bool on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_avdd;
	int ret;
	if (enabled == on)
		return 0;

	regulator_vdd = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator_vdd))
			return PTR_ERR(regulator_vdd);

	regulator_avdd = regulator_get(NULL, "touch");
	if (IS_ERR(regulator_avdd))
		return PTR_ERR(regulator_avdd);

	printk(KERN_DEBUG "[TSP] %s %s\n", __func__, on ? "on" : "off");

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_avdd);
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		if (regulator_is_enabled(regulator_avdd))
			regulator_disable(regulator_avdd);
	}

	enabled = on;
	regulator_put(regulator_vdd);
	regulator_put(regulator_avdd);

	return 0;
}

int is_melfas_vdd_on(void)
{
	int ret;
	/* 3.3V */
	static struct regulator *regulator;

	if (!regulator) {
		regulator = regulator_get(NULL, "touch");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		/*s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT); */
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

void melfas_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void melfas_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

int get_lcd_type;
void __init midas_tsp_set_lcdtype(int lcd_type)
{
	get_lcd_type = lcd_type;
}

int melfas_get_lcdtype(void)
{
	return get_lcd_type;
}
struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}

#ifdef CONFIG_LCD_FREQ_SWITCH
struct tsp_lcd_callbacks *lcd_callbacks;
struct tsp_lcd_callbacks {
	void (*inform_lcd)(struct tsp_lcd_callbacks *, bool);
};

void tsp_lcd_infom(bool en)
{
	if (lcd_callbacks && lcd_callbacks->inform_lcd)
		lcd_callbacks->inform_lcd(lcd_callbacks, en);
}

static void melfas_register_lcd_callback(void *cb)
{
	lcd_callbacks = cb;
	pr_debug("[TSP] melfas_register_lcd_callback\n");
}
#endif

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 720,
	.max_y = 1280,
#if 0
	.invert_x = 720,
	.invert_y = 1280,
#else
	.invert_x = 0,
	.invert_y = 0,
#endif
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.is_vdd_on = is_melfas_vdd_on,
	.config_fw_version = "N7100_Me_0910",
/*	.set_touch_i2c		= melfas_set_touch_i2c, */
/*	.set_touch_i2c_to_gpio	= melfas_set_touch_i2c_to_gpio, */
	.lcd_type = melfas_get_lcdtype,
	.register_cb = melfas_register_callback,
#ifdef CONFIG_LCD_FREQ_SWITCH
	.register_lcd_cb = melfas_register_lcd_callback,
#endif
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#elif defined(CONFIG_TOUCHSCREEN_CYTTSP4)

#define CY_I2C_NAME     "cyttsp4-i2c"

#define CY_USE_TMA400_SP2
/* use the following define if the device is a TMA400 family part
 */
#define CY_USE_TMA400
#define CY_USE_BUTTON_TEST_PANEL

#ifdef CY_USE_TMA400
#define CY_I2C_TCH_ADR	0x24
#define CY_I2C_LDR_ADR	0x24
#ifdef CY_USE_BUTTON_TEST_PANEL
#define CY_MAXX 480
#define CY_MAXY 800
#else
#define CY_MAXX 880
#define CY_MAXY 1280
#endif
#define CY_MINX 0
#define CY_MINY 0
#endif /* --CY_USE_TMA400 */

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MIN_P 0
#define CY_ABS_MIN_W 0
#ifdef CY_USE_TMA400
#define CY_ABS_MIN_T 0
#endif /* --CY_USE_TMA400 */

#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MAX_P 255
#define CY_ABS_MAX_W 255
#ifdef CY_USE_TMA400
#define CY_ABS_MAX_T 15
#endif /* --CY_USE_TMA400 */
#define CY_IGNORE_VALUE 0xFFFF



#include "cyttsp4_params.h"

static struct touch_settings cyttsp4_sett_param_regs = {
	.data = (uint8_t *)&cyttsp4_param_regs[0],
	.size = ARRAY_SIZE(cyttsp4_param_regs),
	.tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
	.data = (uint8_t *)&cyttsp4_param_size[0],
	.size = ARRAY_SIZE(cyttsp4_param_size),
	.tag = 0,
};


/* Design Data Table */
static u8 cyttsp4_ddata[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24 /* test padding
	, 25, 26, 27, 28, 29, 30, 31 */
};

static struct touch_settings cyttsp4_sett_ddata = {
	.data = (uint8_t *)&cyttsp4_ddata[0],
	.size = ARRAY_SIZE(cyttsp4_ddata),
	.tag = 0,
};

/* Manufacturing Data Table */
static u8 cyttsp4_mdata[] = {
	65, 64, /* test truncation */63, 62, 61, 60, 59, 58, 57, 56, 55,
	54, 53, 52, 51, 50, 49, 48,
	47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
	31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static struct touch_settings cyttsp4_sett_mdata = {
	.data = (uint8_t *)&cyttsp4_mdata[0],
	.size = ARRAY_SIZE(cyttsp4_mdata),
	.tag = 0,
};

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

/* use this define to include auto boot image
 */
#define CY_USE_INCLUDE_FBL
#ifdef CY_USE_INCLUDE_FBL
#include "cyttsp4_img.h"
static struct touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
};
#else
static struct touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
#ifdef CY_USE_TMA400
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
#endif /* --CY_USE_TMA400 */
#ifndef CY_USE_TMA400
	ABS_MT_TOUCH_MAJOR, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
#endif /* --CY_USE_TMA400 */
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -128, 127, 0, 0,
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static bool enabled;
int TSP_VDD_18V(int on)
{
	struct regulator *regulator;

	if (enabled == on)
		return 0;

	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on) {
		regulator_enable(regulator);
		/*printk(KERN_INFO "[TSP] melfas power on\n"); */
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	enabled = on;
	regulator_put(regulator);

	return 0;
}

int melfas_power(int on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_avdd;
	int ret;
	if (enabled == on)
		return 0;

	regulator_vdd = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator_vdd))
			return PTR_ERR(regulator_vdd);

	regulator_avdd = regulator_get(NULL, "touch");
	if (IS_ERR(regulator_avdd))
		return PTR_ERR(regulator_avdd);

	printk(KERN_DEBUG "[TSP] %s %s\n", __func__, on ? "on" : "off");

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_avdd);
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		if (regulator_is_enabled(regulator_avdd))
			regulator_disable(regulator_avdd);
	}

	enabled = on;
	regulator_put(regulator_vdd);
	regulator_put(regulator_avdd);

	return 0;
}

int is_melfas_vdd_on(void)
{
	int ret;
	/* 3.3V */
	static struct regulator *regulator;

	if (!regulator) {
		regulator = regulator_get(NULL, "touch");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		/*s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT); */
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

void melfas_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void melfas_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

int get_lcd_type;
void __init midas_tsp_set_lcdtype(int lcd_type)
{
	get_lcd_type = lcd_type;
}

int melfas_get_lcdtype(void)
{
	return get_lcd_type;
}
struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}

int cyttsp4_hw_reset(void)
{
	struct regulator *regulator;
	int ret = 0;

	regulator = regulator_get(NULL, "touch");

	regulator_enable(regulator);
	TSP_VDD_18V(1);
	mdelay(20);

	if (regulator_is_enabled(regulator)) {
		regulator_disable(regulator);
		TSP_VDD_18V(0);
	}
	mdelay(40);

	regulator_enable(regulator);
	TSP_VDD_18V(1);
	mdelay(20);

	regulator_put(regulator);

	return ret;
}

int cyttsp4_hw_power(int on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_avdd;
	int ret;

	if (enabled == on)
		return 0;

	regulator_vdd = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator_vdd))
		return PTR_ERR(regulator_vdd);

	regulator_avdd = regulator_get(NULL, "touch");
	if (IS_ERR(regulator_avdd))
		return PTR_ERR(regulator_avdd);

	printk(KERN_DEBUG "[TSP] %s %s\n", __func__, on ? "on" : "off");

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_avdd);
	} else {
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		if (regulator_is_enabled(regulator_avdd))
			regulator_disable(regulator_avdd);
	}

	enabled = on;
	regulator_put(regulator_vdd);
	regulator_put(regulator_avdd);

	return 0;
}

#define CY_WAKE_DFLT                99	/* causes wake strobe on INT line
					 * in sample board configuration
					 * platform data->hw_recov() function
					 */
int cyttsp4_hw_recov(int on)
{
	int retval = 0;
	int gpio;
	int ret;

	switch (on) {
	case 0:
		cyttsp4_hw_reset();
		retval = 0;
		break;
	case CY_WAKE_DFLT:
		gpio = GPIO_TSP_INT;
		ret = gpio_request(gpio, "TSP_INT");
		retval = gpio_direction_output(gpio, 0);
		if (retval < 0) {
			pr_err("%s: Fail switch IRQ pin to OUT r=%d\n",
				__func__, retval);
		} else {
			udelay(2000);
			retval = gpio_direction_input(gpio);
			if (retval < 0) {
				pr_err("%s: Fail switch IRQ pin to IN"
					" r=%d\n", __func__, retval);
			}
		}
		break;
	default:
		retval = -ENOSYS;
		break;
	}

	return retval;
}

int cyttsp4_irq_stat(void)
{
	int irq_stat = 0;
	int gpio;
	int ret;

	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	irq_stat = gpio_get_value(gpio);

	return irq_stat;
}

struct touch_platform_data cyttsp4_i2c_touch_platform_data = {
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		&cyttsp4_sett_param_regs, /* &cyttsp4_sett_param_regs, */
		&cyttsp4_sett_param_size, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.fw = &cyttsp4_firmware,
	.frmwrk = &cyttsp4_framework,
	.addr = {CY_I2C_TCH_ADR, CY_I2C_LDR_ADR},
	.flags = 0x00,
	.hw_reset = cyttsp4_hw_reset,
	.hw_power = cyttsp4_hw_power,
	.hw_recov = cyttsp4_hw_recov,
	.irq_stat = cyttsp4_irq_stat,
};

static struct i2c_board_info i2c_devs3[] = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, CY_I2C_TCH_ADR),
		.platform_data = &cyttsp4_i2c_touch_platform_data
	},
};

void __init midas_tsp_set_platdata(struct touch_platform_data *pdata)
{
	if (!pdata)
		pdata = &cyttsp4_i2c_touch_platform_data;

	i2c_devs3[0].platform_data = pdata;
}

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#else /* CONFIG_TOUCHSCREEN_ATMEL_MXT224_U1 */

/* MELFAS TSP */
static bool enabled;
int TSP_VDD_18V(bool on)
{
	struct regulator *regulator;

	if (enabled == on)
		return 0;

	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on) {
		regulator_enable(regulator);
		/*printk(KERN_INFO "[TSP] melfas power on\n"); */
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	enabled = on;
	regulator_put(regulator);

	return 0;
}

int melfas_power(bool on)
{
	struct regulator *regulator;
	int ret;
	if (enabled == on)
		return 0;

	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	printk(KERN_DEBUG "[TSP] %s %s\n", __func__, on ? "on" : "off");

	if (on) {
		/* Analog-Panel Power */
		regulator_enable(regulator);
		/* IO Logit Power */
		TSP_VDD_18V(true);
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator)) {
			regulator_disable(regulator);
			TSP_VDD_18V(false);
		}
	}

	enabled = on;
	regulator_put(regulator);

	return 0;
}

int is_melfas_vdd_on(void)
{
	int ret;
	/* 3.3V */
	static struct regulator *regulator;

	if (!regulator) {
		regulator = regulator_get(NULL, "touch");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
/*
		ret = regulator_set_voltage(regulator, 3300000, 3300000);
		if (ret) {
			pr_err("%s: unable to set ldo17 voltage to 3.3V\n",
			       __func__);
			return ret;
		} */
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		/*s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT); */
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

int get_lcd_type;
void __init midas_tsp_set_lcdtype(int lcd_type)
{
	get_lcd_type = lcd_type;
}

int melfas_get_lcdtype(void)
{
	return get_lcd_type;
}
struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 720,
	.max_y = 1280,
	.invert_x = 0,
	.invert_y = 0,
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.is_vdd_on = is_melfas_vdd_on,
	.config_fw_version = "I9300_Me_0507",
	.lcd_type = melfas_get_lcdtype,
	.register_cb = melfas_register_callback,
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}
#endif /* CONFIG_TOUCHSCREEN_ATMEL_MXT224_U1 */

/*
 * Flexrate supports reducing cpufreq ondemand polling rate
 * based on the user input events including touch events.
 * This reduces response time if the touch event triggers tasks that require
 * heavy CPU loads and does not incur unnecessary CPUFreq-up if the touch
 * event does not trigger such tasks.
 */
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
static void flexrate_work(struct work_struct *work)
{
	cpufreq_ondemand_flexrate_request(10000, 10);
}

#include <linux/pm_qos_params.h>
static struct pm_qos_request_list busfreq_qos;
static void flexrate_qos_cancel(struct work_struct *work)
{
	pm_qos_update_request(&busfreq_qos, 0);
}

static DECLARE_WORK(flex_work, flexrate_work);
static DECLARE_DELAYED_WORK(busqos_work, flexrate_qos_cancel);

void midas_tsp_request_qos(void *data)
{
	if (!work_pending(&flex_work))
		schedule_work_on(0, &flex_work);

	/* Guarantee that the bus runs at >= 266MHz */
	if (!pm_qos_request_active(&busfreq_qos))
		pm_qos_add_request(&busfreq_qos, PM_QOS_BUS_DMA_THROUGHPUT,
				   266000);
	else {
		cancel_delayed_work_sync(&busqos_work);
		pm_qos_update_request(&busfreq_qos, 266000);
	}

	/* Cancel the QoS request after 1/10 sec */
	schedule_delayed_work_on(0, &busqos_work, HZ / 5);
}
#endif

