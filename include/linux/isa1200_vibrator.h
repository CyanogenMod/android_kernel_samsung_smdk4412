/* arch/arm/mach-tegra/sec_vibrator.c
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef _LINUX_SEC_VIBRATOR_H
#define _LINUX_SEC_VIBRATOR_H

#define HAPTIC_CONTROL_REG0		0x30
#define HAPTIC_CONTROL_REG1		0x31
#define HAPTIC_CONTROL_REG2		0x32
#define HAPTIC_PLL_REG				0x33
#define HAPTIC_CONTROL_REG4		0x34
#define HAPTIC_PWM_DUTY_REG		0x35
#define HAPTIC_PWM_PERIOD_REG	0x36
#define HAPTIC_AMPLITUDE_REG		0x37

/* HAPTIC_CONTROL_REG0 */
#define CTL0_DIVIDER128				0
#define CTL0_DIVIDER256				1
#define CTL0_DIVIDER512				2
#define CTL0_DIVIDER1024			3
#define CTL0_13MHZ					1 << 2
#define CTL0_PWM_INPUT			1 << 3
#define CTL0_PWM_GEN				2 << 3
#define CTL0_WAVE_GEN				3 << 3
#define CTL0_HIGH_DRIVE			1 << 5
#define CTL0_OVER_DR_EN			1 << 6
#define CTL0_NORMAL_OP			1 << 7

/* HAPTIC_CONTROL_REG1 */
#define CTL1_HAPTICOFF_16U			0
#define CTL1_HAPTICOFF_32U			1
#define CTL1_HAPTICOFF_64U			2
#define CTL1_HAPTICOFF_100U		3
#define CTL1_HAPTICON_1U			1 << 2
#define CTL1_SMART_EN				1 << 3
#define CTL1_PLL_EN					1 << 4
#define CTL1_ERM_TYPE				1 << 5
#define CTL1_DEFAULT				1 << 6
#define CTL1_EXT_CLOCK				1 << 7

/* HAPTIC_CONTROL_REG2 */
#define CTL2_EFFECT_EN				1
#define CTL2_START_EFF_EN			1 << 2
#define CTL2_SOFT_RESET_EN			1 << 7

struct isa1200_vibrator_platform_data {
	struct clk *(*get_clk) (void);
	int (*gpio_en) (bool) ;
	int pwm_id;
	int max_timeout;
	u8 ctrl0;
	u8 ctrl1;
	u8 ctrl2;
	u8 ctrl4;
	u8 pll;
	u8 duty;
	u8 period;
	u16 pwm_duty;
	u16 pwm_period;
};

#if defined(CONFIG_VIBETONZ)
extern int vibtonz_i2c_write(u8 addr, int length, u8 *data);
extern void vibtonz_clk_enable(bool en);
extern void vibtonz_chip_enable(bool en);
extern void vibtonz_clk_config(int duty);
#endif

#endif
