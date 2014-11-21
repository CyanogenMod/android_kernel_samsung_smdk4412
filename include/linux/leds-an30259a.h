/*
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

#ifndef _LEDS_AN30259A_H
#define _LEDS_AN30259A_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LED_LIGHT_OFF	0
#define LED_LIGHT_ON	1
#define LED_LIGHT_PULSE	2
#define LED_LIGHT_SLOPE	3

/*
 * This struct gets passed to the ioctl call.
 * If only one of struct gets passed to the ioctl then it is assumed to define
 * the behavior for all 3 color components: R, G and B.
 * If 3 structs are passed, then each one is assumed to describe a single color:
 * R first, then G, then B.
 *
 * Requesting a color value of 0 is equivalent to requesting LED_LIGHT_OFF
 *
 * If only describing a single color (ie passing a single struct), then
 * start_delay will get ignored
 *
 * Other parameters may get ignored depending on the requested state:
 * LIGHT_ON only requires color
 * LIGHT_PULSE requires color, time_on and time_off
 *
 * Total time for time_slope_up_1 + time_slope_up_2 + time_on as well as for
 * time_slope_down_1 + time_slope_down_2 + time_off will be rounded up to the
 * nearest .5 seconds.
 *
 * Each of the time_slope_* values will get rounded up to the nearest multiple
 * of 4ms up to 7680ms
 */

struct an30259_led_conf {
	const char      *name;
	int          brightness;
	int          max_brightness;
	int          flags;
};

struct an30259a_pr_control {
	/* LED color in RGB format */
	__u32			color;
	/* see defines above */
	__u32			state;
	/* initial delay in ms */
	__u16			start_delay;
	/* time to reach mid_brightness_up from off in ms */
	__u16			time_slope_up_1;
	/* time to reach color from mid_brightness_up in ms */
	__u16			time_slope_up_2;
	/* time at max brightness in ms */
	__u16			time_on;
	/* time to reach mid_brightness_down from max brightness in ms */
	__u16			time_slope_down_1;
	/* time to reach off from mid_brightness_down in ms */
	__u16			time_slope_down_2;
	/* time off in ms */
	__u16			time_off;
	/* mid point brightness in 1/128 increments of color */
	__u8			mid_brightness;
} __packed;

#define AN30259A_PR_SET_LED	_IOW('S', 42, struct an30259a_pr_control)
#define AN30259A_PR_SET_LEDS	_IOW('S', 43, struct an30259a_pr_control[3])
#define AN30259A_PR_SET_IMAX	_IOW('S', 44, __u8)
#endif						/* _LEDS_AN30259A_H */
