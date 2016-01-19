/*
 * CORERIVER TOUCHCORE 370L touchkey driver
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Taeyoon Yoon <tyoony.yoon@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_TC370_H
#define __LINUX_TC370_H

#define TC370_DEVICE	"tc370"

enum {
	TC370_SUSPEND_WITH_POWER_OFF = 0,
	TC370_SUSPEND_WITH_SLEEP_CMD,
};

struct tc370_platform_data {
	u8	enable;
	u32	gpio_scl;
	u32	gpio_sda;
	u32	gpio_int;
	u32	gpio_en;
	int	udelay;
	int	num_key;
	int	*keycodes;
	u8	suspend_type;
	u8	exit_flag;
	int	(*setup_power)(struct device *, bool);
	void	(*power)(bool);
	void	(*led_power)(bool);
	void	(*pin_configure)(bool to_gpios);
	void	(*int_set_pull)(bool);
};

#define SEC_FAC_TK
#if defined(SEC_FAC_TK)
extern struct class *sec_class;
#endif

#endif /* __LINUX_TC370_H */
