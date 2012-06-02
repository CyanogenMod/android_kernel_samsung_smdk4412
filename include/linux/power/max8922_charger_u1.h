/*
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX8922_CHARGER_H_
#define __MAX8922_CHARGER_H_	__FILE__

struct max8922_platform_data {
	int	(*topoff_cb)(void);
	int	(*cfg_gpio)(void);
	int	gpio_chg_en;
	int	gpio_chg_ing;
	int	gpio_ta_nconnected;
};

#endif /* __MAX8922_CHARGER_H_ */
