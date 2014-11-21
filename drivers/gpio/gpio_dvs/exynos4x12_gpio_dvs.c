/*
 * Samsung Mobile VE Group.
 *
 * drivers/gpio/gpio_dvs/exynos4x12_gpio_dvs.c - Read GPIO info. from SMDK4412
 *
 * Copyright (C) 2013, Samsung Electronics.
 *
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
/* This header is essential at this file */
#include <plat/gpio-cfg-helpers.h>

#include <mach/gpio-midas.h>

/********************* Fixed Code Area !***************************/
#include <linux/secgpio_dvs.h>
#include <linux/platform_device.h>
/****************************************************************/

extern int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config);

static u32 exynos4_gpio_num[] = {
	EXYNOS4_GPA0(0),
	EXYNOS4_GPA0(1),
	EXYNOS4_GPA0(2),
	EXYNOS4_GPA0(3),
	EXYNOS4_GPA0(4),
	EXYNOS4_GPA0(5),
	EXYNOS4_GPA0(6),
	EXYNOS4_GPA0(7),
	EXYNOS4_GPA1(0),
	EXYNOS4_GPA1(1),
	EXYNOS4_GPA1(2),
	EXYNOS4_GPA1(3),
	EXYNOS4_GPA1(4),
	EXYNOS4_GPA1(5),
	EXYNOS4_GPB(0),
	EXYNOS4_GPB(1),
	EXYNOS4_GPB(2),
	EXYNOS4_GPB(3),
	EXYNOS4_GPB(4),
	EXYNOS4_GPB(5),
	EXYNOS4_GPB(6),
	EXYNOS4_GPB(7),
	EXYNOS4_GPC0(0),
	EXYNOS4_GPC0(1),
	EXYNOS4_GPC0(2),
	EXYNOS4_GPC0(3),
	EXYNOS4_GPC0(4),
	EXYNOS4_GPC1(0),
	EXYNOS4_GPC1(1),
	EXYNOS4_GPC1(2),
	EXYNOS4_GPC1(3),
	EXYNOS4_GPC1(4),
	EXYNOS4_GPD0(0),
	EXYNOS4_GPD0(1),
	EXYNOS4_GPD0(2),
	EXYNOS4_GPD0(3),
	EXYNOS4_GPD1(0),
	EXYNOS4_GPD1(1),
	EXYNOS4_GPD1(2),
	EXYNOS4_GPD1(3),
	EXYNOS4_GPF0(0),
	EXYNOS4_GPF0(1),
	EXYNOS4_GPF0(2),
	EXYNOS4_GPF0(3),
	EXYNOS4_GPF0(4),
	EXYNOS4_GPF0(5),
	EXYNOS4_GPF0(6),
	EXYNOS4_GPF0(7),
	EXYNOS4_GPF1(0),
	EXYNOS4_GPF1(1),
	EXYNOS4_GPF1(2),
	EXYNOS4_GPF1(3),
	EXYNOS4_GPF1(4),
	EXYNOS4_GPF1(5),
	EXYNOS4_GPF1(6),
	EXYNOS4_GPF1(7),
	EXYNOS4_GPF2(0),
	EXYNOS4_GPF2(1),
	EXYNOS4_GPF2(2),
	EXYNOS4_GPF2(3),
	EXYNOS4_GPF2(4),
	EXYNOS4_GPF2(5),
	EXYNOS4_GPF2(6),
	EXYNOS4_GPF2(7),
	EXYNOS4_GPF3(0),
	EXYNOS4_GPF3(1),
	EXYNOS4_GPF3(2),
	EXYNOS4_GPF3(3),
	EXYNOS4_GPF3(4),
	EXYNOS4_GPF3(5),
	EXYNOS4212_GPJ0(0),
	EXYNOS4212_GPJ0(1),
	EXYNOS4212_GPJ0(2),
	EXYNOS4212_GPJ0(3),
	EXYNOS4212_GPJ0(4),
	EXYNOS4212_GPJ0(5),
	EXYNOS4212_GPJ0(6),
	EXYNOS4212_GPJ0(7),
	EXYNOS4212_GPJ1(0),
	EXYNOS4212_GPJ1(1),
	EXYNOS4212_GPJ1(2),
	EXYNOS4212_GPJ1(3),
	EXYNOS4212_GPJ1(4),
	EXYNOS4_GPK0(0),
	EXYNOS4_GPK0(1),
	EXYNOS4_GPK0(2),
	EXYNOS4_GPK0(3),
	EXYNOS4_GPK0(4),
	EXYNOS4_GPK0(5),
	EXYNOS4_GPK0(6),
	EXYNOS4_GPK1(0),
	EXYNOS4_GPK1(1),
	EXYNOS4_GPK1(2),
	EXYNOS4_GPK1(3),
	EXYNOS4_GPK1(4),
	EXYNOS4_GPK1(5),
	EXYNOS4_GPK1(6),
	EXYNOS4_GPK2(0),
	EXYNOS4_GPK2(1),
	EXYNOS4_GPK2(2),
	EXYNOS4_GPK2(3),
	EXYNOS4_GPK2(4),
	EXYNOS4_GPK2(5),
	EXYNOS4_GPK2(6),
	EXYNOS4_GPK3(0),
	EXYNOS4_GPK3(1),
	EXYNOS4_GPK3(2),
	EXYNOS4_GPK3(3),
	EXYNOS4_GPK3(4),
	EXYNOS4_GPK3(5),
	EXYNOS4_GPK3(6),
	EXYNOS4_GPL0(0),
	EXYNOS4_GPL0(1),
	EXYNOS4_GPL0(2),
	EXYNOS4_GPL0(3),
	EXYNOS4_GPL0(4),
	EXYNOS4_GPL0(6),
	EXYNOS4_GPL1(0),
	EXYNOS4_GPL1(1),
	EXYNOS4_GPL2(0),
	EXYNOS4_GPL2(1),
	EXYNOS4_GPL2(2),
	EXYNOS4_GPL2(3),
	EXYNOS4_GPL2(4),
	EXYNOS4_GPL2(5),
	EXYNOS4_GPL2(6),
	EXYNOS4_GPL2(7),
	EXYNOS4212_GPM0(0),
	EXYNOS4212_GPM0(1),
	EXYNOS4212_GPM0(2),
	EXYNOS4212_GPM0(3),
	EXYNOS4212_GPM0(4),
	EXYNOS4212_GPM0(5),
	EXYNOS4212_GPM0(6),
	EXYNOS4212_GPM0(7),
	EXYNOS4212_GPM1(0),
	EXYNOS4212_GPM1(1),
	EXYNOS4212_GPM1(2),
	EXYNOS4212_GPM1(3),
	EXYNOS4212_GPM1(4),
	EXYNOS4212_GPM1(5),
	EXYNOS4212_GPM1(6),
	EXYNOS4212_GPM2(0),
	EXYNOS4212_GPM2(1),
	EXYNOS4212_GPM2(2),
	EXYNOS4212_GPM2(3),
	EXYNOS4212_GPM2(4),
	EXYNOS4212_GPM3(0),
	EXYNOS4212_GPM3(1),
	EXYNOS4212_GPM3(2),
	EXYNOS4212_GPM3(3),
	EXYNOS4212_GPM3(4),
	EXYNOS4212_GPM3(5),
	EXYNOS4212_GPM3(6),
	EXYNOS4212_GPM3(7),
	EXYNOS4212_GPM4(0),
	EXYNOS4212_GPM4(1),
	EXYNOS4212_GPM4(2),
	EXYNOS4212_GPM4(3),
	EXYNOS4212_GPM4(4),
	EXYNOS4212_GPM4(5),
	EXYNOS4212_GPM4(6),
	EXYNOS4212_GPM4(7),
	EXYNOS4212_GPV0(0),
	EXYNOS4212_GPV0(1),
	EXYNOS4212_GPV0(2),
	EXYNOS4212_GPV0(3),
	EXYNOS4212_GPV0(4),
	EXYNOS4212_GPV0(5),
	EXYNOS4212_GPV0(6),
	EXYNOS4212_GPV0(7),
	EXYNOS4212_GPV1(0),
	EXYNOS4212_GPV1(1),
	EXYNOS4212_GPV1(2),
	EXYNOS4212_GPV1(3),
	EXYNOS4212_GPV1(4),
	EXYNOS4212_GPV1(5),
	EXYNOS4212_GPV1(6),
	EXYNOS4212_GPV1(7),
	EXYNOS4212_GPV2(0),
	EXYNOS4212_GPV2(1),
	EXYNOS4212_GPV2(2),
	EXYNOS4212_GPV2(3),
	EXYNOS4212_GPV2(4),
	EXYNOS4212_GPV2(5),
	EXYNOS4212_GPV2(6),
	EXYNOS4212_GPV2(7),
	EXYNOS4212_GPV3(0),
	EXYNOS4212_GPV3(1),
	EXYNOS4212_GPV3(2),
	EXYNOS4212_GPV3(3),
	EXYNOS4212_GPV3(4),
	EXYNOS4212_GPV3(5),
	EXYNOS4212_GPV3(6),
	EXYNOS4212_GPV3(7),
	EXYNOS4212_GPV4(0),
	EXYNOS4212_GPV4(1),
	EXYNOS4_GPX0(0),
	EXYNOS4_GPX0(1),
	EXYNOS4_GPX0(2),
	EXYNOS4_GPX0(3),
	EXYNOS4_GPX0(4),
	EXYNOS4_GPX0(5),
	EXYNOS4_GPX0(6),
	EXYNOS4_GPX0(7),
	EXYNOS4_GPX1(0),
	EXYNOS4_GPX1(1),
	EXYNOS4_GPX1(2),
	EXYNOS4_GPX1(3),
	EXYNOS4_GPX1(4),
	EXYNOS4_GPX1(5),
	EXYNOS4_GPX1(6),
	EXYNOS4_GPX1(7),
	EXYNOS4_GPX2(0),
	EXYNOS4_GPX2(1),
	EXYNOS4_GPX2(2),
	EXYNOS4_GPX2(3),
	EXYNOS4_GPX2(4),
	EXYNOS4_GPX2(5),
	EXYNOS4_GPX2(6),
	EXYNOS4_GPX2(7),
	EXYNOS4_GPX3(0),
	EXYNOS4_GPX3(1),
	EXYNOS4_GPX3(2),
	EXYNOS4_GPX3(3),
	EXYNOS4_GPX3(4),
	EXYNOS4_GPX3(5),
	EXYNOS4_GPX3(6),
	EXYNOS4_GPX3(7),
	EXYNOS4_GPY0(0),
	EXYNOS4_GPY0(1),
	EXYNOS4_GPY0(2),
	EXYNOS4_GPY0(3),
	EXYNOS4_GPY0(4),
	EXYNOS4_GPY0(5),
	EXYNOS4_GPY1(0),
	EXYNOS4_GPY1(1),
	EXYNOS4_GPY1(2),
	EXYNOS4_GPY1(3),
	EXYNOS4_GPY2(0),
	EXYNOS4_GPY2(1),
	EXYNOS4_GPY2(2),
	EXYNOS4_GPY2(3),
	EXYNOS4_GPY2(4),
	EXYNOS4_GPY2(5),
	EXYNOS4_GPY3(0),
	EXYNOS4_GPY3(1),
	EXYNOS4_GPY3(2),
	EXYNOS4_GPY3(3),
	EXYNOS4_GPY3(4),
	EXYNOS4_GPY3(5),
	EXYNOS4_GPY3(6),
	EXYNOS4_GPY3(7),
	EXYNOS4_GPY4(0),
	EXYNOS4_GPY4(1),
	EXYNOS4_GPY4(2),
	EXYNOS4_GPY4(3),
	EXYNOS4_GPY4(4),
	EXYNOS4_GPY4(5),
	EXYNOS4_GPY4(6),
	EXYNOS4_GPY4(7),
	EXYNOS4_GPY5(0),
	EXYNOS4_GPY5(1),
	EXYNOS4_GPY5(2),
	EXYNOS4_GPY5(3),
	EXYNOS4_GPY5(4),
	EXYNOS4_GPY5(5),
	EXYNOS4_GPY5(6),
	EXYNOS4_GPY5(7),
	EXYNOS4_GPY6(0),
	EXYNOS4_GPY6(1),
	EXYNOS4_GPY6(2),
	EXYNOS4_GPY6(3),
	EXYNOS4_GPY6(4),
	EXYNOS4_GPY6(5),
	EXYNOS4_GPY6(6),
	EXYNOS4_GPY6(7),
	EXYNOS4_GPZ(0),
	EXYNOS4_GPZ(1),
	EXYNOS4_GPZ(2),
	EXYNOS4_GPZ(3),
	EXYNOS4_GPZ(4),
	EXYNOS4_GPZ(5),
	EXYNOS4_GPZ(6)
};

/********************* Fixed Code Area !***************************/
#define GET_RESULT_GPIO(a, b, c)	\
	(((a)<<10 & 0xFC00) |((b)<<4 & 0x03F0) | ((c) & 0x000F))

#define GET_GPIO_IO(value)	\
	(unsigned char)((0xFC00 & (value)) >> 10)
#define GET_GPIO_PUPD(value)	\
	(unsigned char)((0x03F0 & (value)) >> 4)
#define GET_GPIO_LH(value)	\
	(unsigned char)(0x000F & (value))
/****************************************************************/

/****************************************************************/
/* Define value in accordance with
	the specification of each BB vendor. */
#define AP_GPIO_COUNT (sizeof(exynos4_gpio_num)/sizeof(u32))
/****************************************************************/

/****************************************************************/
/* Pre-defined variables. (DO NOT CHANGE THIS!!) */
static uint16_t checkgpiomap_result[GDVS_PHONE_STATUS_MAX][AP_GPIO_COUNT];
static struct gpiomap_result_t gpiomap_result = {
	.init = checkgpiomap_result[PHONE_INIT],
	.sleep = checkgpiomap_result[PHONE_SLEEP]
};

#ifdef SECGPIO_SLEEP_DEBUGGING
static struct sleepdebug_gpiotable sleepdebug_table;
#endif
/****************************************************************/

static inline u32 s3c_gpio_do_getLH(struct s3c_gpio_chip *chip,
					  unsigned int off)
{
	void __iomem *reg = chip->base + 0x04;
	int shift = off;
	u32 pdata = __raw_readl(reg);

	pdata >>= shift;
	pdata &= 0x1;
	return pdata;
}


static unsigned s3c_gpio_getLH(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	unsigned ret = 0;
	int offset;

	if (chip) {
		offset = pin - chip->chip.base;

		s3c_gpio_lock(chip, flags);
		ret = s3c_gpio_do_getLH(chip, offset);
		s3c_gpio_unlock(chip, flags);
	}

	return ret;
}

static unsigned s3c_gpio_getcfg_dvs(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	unsigned ret = 0;
	int offset;

	if (chip) {
		chip->config->get_config = s3c_gpio_getcfg_s3c64xx_4bit;
		offset = pin - chip->chip.base;

		s3c_gpio_lock(chip, flags);
		ret = s3c_gpio_do_getcfg(chip, offset);
		s3c_gpio_unlock(chip, flags);
	}

	return ret;
}

static s3c_gpio_pull_t s3c_gpio_get_slp_cfgpin(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= EXYNOS4_GPX0(0)) && (pin <= EXYNOS4_GPX3(7)))
		return -EINVAL;

	reg = chip->base + 0x10;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con >>= shift;
	con &= 0x3;

	local_irq_restore(flags);

	return (__force s3c_gpio_pull_t)con;
}

static s3c_gpio_pull_t s3c_gpio_slp_getpull_updown(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= EXYNOS4_GPX0(0)) && (pin <= EXYNOS4_GPX3(7)))
		return -EINVAL;

	reg = chip->base + 0x14;

	offset = pin - chip->chip.base;
	shift = offset * 2;


	local_irq_save(flags);

	con = __raw_readl(reg);
	con >>= shift;
	con &= 0x3;

	local_irq_restore(flags);

	return (__force s3c_gpio_pull_t)con;
}


/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void check_gpio_status(unsigned char phonestate)
{
	u32 i, gpio;
	u32 cfg, val, pud;

	u8 temp_io = 0, temp_pupd = 0, temp_lh = 0;

	pr_info("[GPIO_DVS][%s] state : %s\n", __func__,
		(phonestate == PHONE_INIT) ? "init" : "sleep");

	for (i = 0; i < AP_GPIO_COUNT; i++) {
		gpio = exynos4_gpio_num[i];

		if (phonestate == PHONE_INIT ||
			((gpio >= EXYNOS4_GPX0(0)) &&
				(gpio <= EXYNOS4_GPX3(7)))) {

			/************************************/
			/* In/Out Check */
			cfg = s3c_gpio_getcfg_dvs(gpio);
			if (cfg == S3C_GPIO_INPUT)
				temp_io = 0x01;	/* GPIO_IN */
			else if (cfg == S3C_GPIO_OUTPUT)
				temp_io = 0x02;	/* GPIO_OUT */
			else if (cfg == S3C_GPIO_SFN(0xF))
				temp_io = 0x03;	/* INT */
			else
				temp_io = 0x00;		/* FUNC */
			/************************************/
			/* State H/L Check */
			if (temp_io == 0x00) /* FUNC */
				//val = 0xE;		/* unknown(Don't care) */
				temp_lh = 0xE;	/* unknown(Don't care) */
			else {
				val = s3c_gpio_getLH(gpio);
				if (val == S3C_GPIO_SETPIN_ZERO)
					temp_lh = 0x0;
				else if (val == S3C_GPIO_SETPIN_ONE)
					temp_lh = 0x1;
				else
					temp_lh = 0xF;	/* Error */
			}
			/************************************/
			pud = s3c_gpio_getpull(gpio);
		} else {		/* if(phonestate == PHONE_SLEEP) { */
			/************************************/
			/* In/Out & State H/L Check */
			cfg = s3c_gpio_get_slp_cfgpin(gpio);
			if (cfg == S3C_GPIO_SLP_INPUT) {
				temp_io = 0x01;	/* GPIO_IN */
				temp_lh = 0xE;	/* unknown(Don't care) */
			} else if (cfg == S3C_GPIO_SLP_OUT0) {
				temp_io = 0x02;	/* GPIO_OUT */
				temp_lh = 0x0;
			} else if (cfg == S3C_GPIO_SLP_OUT1) {
				temp_io = 0x02;	/* GPIO_OUT */
				temp_lh = 0x1;
			} else if (cfg == S3C_GPIO_SLP_PREV) {
				temp_io = 0x04;	/* PREV */
				temp_lh = 0xE;	/* unknown(Don't care) */
			} else {
				temp_io = 0x3F;	/* not alloc.(Error) */
				temp_lh = 0xF;	/* Error */
			}
			/************************************/

			pud = s3c_gpio_slp_getpull_updown(gpio);
		}

		if (pud == S3C_GPIO_PULL_NONE)
			temp_pupd = 0x00;
		else if (pud == S3C_GPIO_PULL_DOWN)
			temp_pupd = 0x01;
		else if (pud == S3C_GPIO_PULL_UP)
			temp_pupd = 0x02;
		else {
			temp_pupd = 0x3F;	// Error
			pr_err("[GPIO_DVS] i : %d, gpio : %d, pud : %d\n",
					i, gpio, pud);
		}

		checkgpiomap_result[phonestate][i] =
			GET_RESULT_GPIO(temp_io, temp_pupd, temp_lh);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);

	return;
}
/****************************************************************/

static int s3c_gpio_slp_setpull_updown_dvs
	(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= EXYNOS4_GPX0(0)) && (pin <= EXYNOS4_GPX3(7)))
		return -EINVAL;

	if (config > S3C_GPIO_PULL_UP)
		return -EINVAL;

	reg = chip->base + 0x14;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;
	__raw_writel(con, reg);

	local_irq_restore(flags);

	return 0;
}

void set_gpio_pupd(int gpionum, unsigned long temp_gpio_reg)
{
	int gpio;
	gpio = exynos4_gpio_num[gpionum];
	if ((gpio >= EXYNOS4_GPX0(0)) && (gpio <= EXYNOS4_GPX3(7)))
		s3c_gpio_setpull(gpio, temp_gpio_reg);
	else
		s3c_gpio_slp_setpull_updown_dvs(gpio, temp_gpio_reg);
}

#ifdef SECGPIO_SLEEP_DEBUGGING
/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
void setgpio_for_sleepdebug(int gpionum, uint16_t  io_pupd_lh)
{
	unsigned char temp_io, temp_pupd, temp_lh;
	int temp_data = io_pupd_lh;
	int gpio;

	pr_info("[GPIO_DVS][%s] gpionum=%d, io_pupd_lh=0x%x\n",
		__func__, gpionum, io_pupd_lh);

	temp_io = GET_GPIO_IO(io_pupd_lh);
	temp_pupd = GET_GPIO_PUPD(io_pupd_lh);
	temp_lh = GET_GPIO_LH(io_pupd_lh);

	pr_info("[GPIO_DVS][%s] io=%d, pupd=%d, lh=%d\n",
		__func__, temp_io, temp_pupd, temp_lh);

	gpio = exynos4_gpio_num[gpionum];

	/* in case of 'INPUT', set PD/PU */
	if (temp_io == 0x01) {
		/* 0x0:NP, 0x1:PD, 0x2:PU */
		if (temp_pupd == 0x0)
			temp_data = S3C_GPIO_PULL_NONE;
		else if (temp_pupd == 0x1)
			temp_data = S3C_GPIO_PULL_DOWN;
		else if (temp_pupd == 0x2)
			temp_data = S3C_GPIO_PULL_UP;

		set_gpio_pupd(gpionum, temp_data);
	}
	/* in case of 'OUTPUT', set L/H */
	else if (temp_io == 0x02) {
		pr_info("[GPIO_DVS][%s] %d gpio set %d\n",
			__func__, gpionum, temp_lh);
		if ((gpio >= EXYNOS4_GPX0(0)) && (gpio <= EXYNOS4_GPX3(7)))
			gpio_set_value(gpio, temp_lh);
		else {
			if (temp_lh == 0x0)
				s3c_gpio_slp_cfgpin(gpio, S3C_GPIO_SLP_OUT0);
			else
				s3c_gpio_slp_cfgpin(gpio, S3C_GPIO_SLP_OUT1);
		}
	}


}
/****************************************************************/

/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void undo_sleepgpio(void)
{
	int i;
	int gpio_num;

	pr_info("[GPIO_DVS][%s] ++\n", __func__);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;
		/*
		 * << Caution >>
		 * If it's necessary,
		 * change the following function to another appropriate one
		 * or delete it
		 */
		//setgpio_for_sleepdebug(gpio_num, gpiomap_result.sleep[gpio_num]);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
/****************************************************************/
#endif

/********************* Fixed Code Area !***************************/
#ifdef SECGPIO_SLEEP_DEBUGGING
static void set_sleepgpio(void)
{
	int i;
	int gpio_num;
	uint16_t set_data;

	pr_info("[GPIO_DVS][%s] ++, cnt=%d\n",
		__func__, sleepdebug_table.gpio_count);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		pr_info("[GPIO_DVS][%d] gpio_num(%d), io(%d), pupd(%d), lh(%d)\n",
			i, sleepdebug_table.gpioinfo[i].gpio_num,
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;

		// to prevent a human error caused by "don't care" value
		if( sleepdebug_table.gpioinfo[i].io == 1)		/* IN */
			sleepdebug_table.gpioinfo[i].lh =
				GET_GPIO_LH(gpiomap_result.sleep[gpio_num]);
		else if( sleepdebug_table.gpioinfo[i].io == 2)		/* OUT */
			sleepdebug_table.gpioinfo[i].pupd =
				GET_GPIO_PUPD(gpiomap_result.sleep[gpio_num]);

		set_data = GET_RESULT_GPIO(
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		setgpio_for_sleepdebug(gpio_num, set_data);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
#endif

static struct gpio_dvs_t gpio_dvs = {
	.result = &gpiomap_result,
	.count = AP_GPIO_COUNT,
	.check_init = false,
	.check_sleep = false,
	.check_gpio_status = check_gpio_status,
#ifdef SECGPIO_SLEEP_DEBUGGING
	.sdebugtable = &sleepdebug_table,
	.set_sleepgpio = set_sleepgpio,
	.undo_sleepgpio = undo_sleepgpio,
#endif
};

static struct platform_device secgpio_dvs_device = {
	.name	= "secgpio_dvs",
	.id		= -1,
	.dev.platform_data = &gpio_dvs,
};

static struct platform_device *secgpio_dvs_devices[] __initdata = {
	&secgpio_dvs_device,
};

static int __init secgpio_dvs_device_init(void)
{
	return platform_add_devices(
		secgpio_dvs_devices, ARRAY_SIZE(secgpio_dvs_devices));
}
/* For compatibility of IORA Init Breakpoint, I have modified exceptively the following function. */
/* arch_initcall(secgpio_dvs_device_init); */
postcore_initcall(secgpio_dvs_device_init);
/****************************************************************/


