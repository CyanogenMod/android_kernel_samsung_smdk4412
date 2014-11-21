/*
 * leds-regulator.h - platform data structure for MAX8997 LEDs.
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_LEDS_MAX8997_H
#define __LINUX_LEDS_MAX8997_H

struct led_max8997_platform_data {
	char *name;                     /* LED name as expected by LED class */
	unsigned int brightness; /* initial brightness value */
};

#endif /* __LINUX_LEDS_MAX8997_H */
