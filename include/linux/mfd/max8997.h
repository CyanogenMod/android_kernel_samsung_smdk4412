/*
 * max8997.h - Voltage regulator driver for the Maxim 8997
 *
 *  Copyright (C) 2009-2010 Samsung Electrnoics
 *
 *  based on max8998.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_MFD_MAX8997_H
#define __LINUX_MFD_MAX8997_H

#include <linux/regulator/machine.h>
#if defined(CONFIG_MACH_Q1_BD)  || defined(CONFIG_MACH_U1_NA_USCC)
#define MAX8997_SUPPORT_TORCH
#endif /* CONFIG_MACH_Q1_BD */

/* MAX 8997 regulator ids */
enum {
	MAX8997_LDO1 = 1,
	MAX8997_LDO2,
	MAX8997_LDO3,
	MAX8997_LDO4,
	MAX8997_LDO5,
	MAX8997_LDO6,
	MAX8997_LDO7,
	MAX8997_LDO8,
	MAX8997_LDO9,
	MAX8997_LDO10,
	MAX8997_LDO11,
	MAX8997_LDO12,
	MAX8997_LDO13,
	MAX8997_LDO14,
	MAX8997_LDO15,
	MAX8997_LDO16,
	MAX8997_LDO17,
	MAX8997_LDO18,
	MAX8997_LDO21,
	MAX8997_BUCK1,
	MAX8997_BUCK2,
	MAX8997_BUCK3,
	MAX8997_BUCK4,
	MAX8997_BUCK5,
	MAX8997_BUCK6,
	MAX8997_BUCK7,
	MAX8997_EN32KHZ_AP,
	MAX8997_EN32KHZ_CP,
	MAX8997_ENVICHG,
	MAX8997_ESAFEOUT1,
	MAX8997_ESAFEOUT2,
	MAX8997_FLASH_CUR,
	MAX8997_MOVIE_CUR,
#ifdef MAX8997_SUPPORT_TORCH
	MAX8997_FLASH_TORCH,
#endif /* MAX8997_SUPPORT_TORCH */

	MAX8997_REG_MAX,
};

/**
 * max8997_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct max8997_regulator_data {
	int				id;
	struct regulator_init_data	*initdata;
	int	(*is_valid_regulator)(int, struct regulator_init_data*);
};

struct max8997_power_data {
	int		(*set_charger)(int);
	int		(*topoff_cb) (void);
	unsigned	batt_detect:1;
	unsigned	topoff_threshold:2;
	unsigned	fast_charge:3;	/* charge current */
};

#ifdef CONFIG_VIBETONZ
#define MAX8997_MOTOR_REG_CONFIG2		0x2
#define MOTOR_LRA		(1<<7)
#define MOTOR_EN		(1<<6)
#define EXT_PWM		(0<<5)
#define DIVIDER_128		(1<<1)
#define DIVIDER_256		(1<<0 | 1<<1)

struct max8997_motor_data {
	u16 max_timeout;
	u16 duty;
	u16 period;
	u16 reg2;
	void (*init_hw)(void);
	void (*motor_en)(bool);
	unsigned int pwm_id;
};
#endif

enum {
	MAX8997_MUIC_DETACHED = 0,
	MAX8997_MUIC_ATTACHED
};

enum {
	AP_USB_MODE = 0,
	CP_USB_MODE,
	AUDIO_MODE,
};

enum {
	UART_PATH_CP = 0,
	UART_PATH_AP,
};

enum {
	USB_SEL_IF = 0,
	USB_SEL_CP,
};

enum muic_acc_type {
	MUIC_ACC_TYPE_NONE = 0,
	MUIC_ACC_TYPE_OTG,
	MUIC_ACC_TYPE_MHL,
	MUIC_ACC_TYPE_STATION,
	MUIC_ACC_TYPE_JIG_USB_OFF,
	MUIC_ACC_TYPE_JIG_USB_ON,
	MUIC_ACC_TYPE_DESKDOCK,
	MUIC_ACC_TYPE_JIG_UART_OFF,
	MUIC_ACC_TYPE_JIG_UART_ON,
	MUIC_ACC_TYPE_CARDOCK,
	MUIC_ACC_TYPE_TA,
	MUIC_ACC_TYPE_USB,
	MUIC_ACC_TYPE_UNKNOWN
};

enum muic_chg_type {
	MUIC_CHG_TYPE_NONE = 0,
	MUIC_CHG_TYPE_USB,
	MUIC_CHG_TYPE_TA,
	MUIC_CHG_TYPE_MHL_VB,
	MUIC_CHG_TYPE_UNKNOWN
};

enum cable_type {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_OTG,
	CABLE_TYPE_TA,
	CABLE_TYPE_DESKDOCK,
	CABLE_TYPE_CARDOCK,
	CABLE_TYPE_STATION,
	CABLE_TYPE_JIG_UART_OFF,
	CABLE_TYPE_JIG_UART_OFF_VB,	/* VBUS enabled */
	CABLE_TYPE_JIG_UART_ON,
	CABLE_TYPE_JIG_USB_OFF,
	CABLE_TYPE_JIG_USB_ON,
	CABLE_TYPE_MHL,
	CABLE_TYPE_MHL_VB,
	CABLE_TYPE_UNKNOWN
};

struct max8997_muic_data {
	void		(*usb_cb) (u8 attached);
	void		(*uart_cb) (u8 attached);
	int		(*charger_cb) (int cable_type);
	void		(*deskdock_cb) (bool attached);
	void		(*cardock_cb) (bool attached);
	void		(*mhl_cb) (int attached);
	void		(*init_cb) (void);
	int		(*set_safeout) (int path);
	bool		(*is_mhl_attached) (void);
	int		(*cfg_uart_gpio) (void);
	void		(*jig_uart_cb) (int path);
	int		(*host_notify_cb) (int enable);
	int		gpio_usb_sel;
	int		gpio_uart_sel;
	int		usb_path;
	int		uart_path;
};

struct max8997_buck1_dvs_funcs {
	int (*set_buck1_dvs_table)(struct max8997_buck1_dvs_funcs *ptr,
				unsigned int *voltage_table, int arr_size);
	int (*get_buck1_dvs_table)(struct max8997_buck1_dvs_funcs *ptr,
				unsigned int *voltage_table);
};

#define BUCK1_TABLE_SIZE	7

/**
 * struct max8997_board - packages regulator init data
 * @regulators: array of defined regulators
 * @num_regulators: number of regultors used
 * @irq_base: base IRQ number for max8997, required for IRQs
 * @ono: power onoff IRQ number for max8997
 * @wakeup: configure the irq as a wake-up source
 * @buck1_gpiodvs: enable/disable GPIO DVS for BUCK1
 * @buck1_voltages: BUCK1 supported voltage list for GPIO DVS(uV)
 *		it must have descending order.
 * @buck1_max_vol: maximun voltage for BUCK1 (B1_TV_1)
 * @buck2_max_vol: maximun voltage for BUCK2 (B2_TV_1)
 * @buck5_max_vol: maximun voltage for BUCK5 (B5_TV_1)
 * @buck_set1: BUCK gpio pin 1 to set output voltage
 * @buck_set2: BUCK gpio pin 2 to set output voltage
 * @buck_set3: BUCK gpio pin 3 to set output voltage
 * @buck_ramp_en: enable BUCKx RAMP
 * @buck_ramp_delay: ramp delay(usec) BUCK RAMP register(0x15)
 * @flash_cntl_val: value of MAX8997_REG_FLASH_CNTL register
 * @mr_debounce_time: manual reset debounce time (sec), (default 7sec)
 */
struct max8997_platform_data {
	struct max8997_regulator_data	*regulators;
	int				num_regulators;
	int				irq_base;
	int				ono;
	int				wakeup;
	bool				buck1_gpiodvs;
	unsigned int			buck1_max_vol;
	unsigned int			buck2_max_vol;
	unsigned int			buck5_max_vol;
	unsigned int			buck1_voltages[BUCK1_TABLE_SIZE];
	int				buck_set1;
	int				buck_set2;
	int				buck_set3;
	bool				buck_ramp_en;
	int				buck_ramp_delay;
	int				flash_cntl_val;
	int				mr_debounce_time;
	struct max8997_power_data	*power;
	struct max8997_muic_data	*muic;
#ifdef CONFIG_VIBETONZ
	struct max8997_motor_data *motor;
#endif
	void (*register_buck1_dvs_funcs)(struct max8997_buck1_dvs_funcs *ptr);
};

#endif /*  __LINUX_MFD_MAX8997_H */
