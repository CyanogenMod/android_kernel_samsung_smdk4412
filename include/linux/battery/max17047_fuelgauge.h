/*
 * max17047_fuelgauge.h
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * based on max17042_battery.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX17047_BATTERY_H_
#define __MAX17047_BATTERY_H_

struct max17047_platform_data {
	int irq_gpio;

	bool enable_current_sense;
	bool enable_gauging_temperature;

	const char *psy_name;
};

#endif

