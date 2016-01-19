/*
 * include/linux/sec_keys.h
 *
 * Copyright (c) 2013 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef _SEC_KEYS_H_
#define _SEC_KEYS_H_

#define SEC_KEY_PATH	"sec_key"
#define SIZE_ROTATE_KEY	4

enum SEC_KEYS_ACT_TYPE {
	KEY_ACT_TYPE_HIGH = 0,
	KEY_ACT_TYPE_LOW,
	KEY_ACT_TYPE_ONE_CH,
	KEY_ACT_TYPE_JOG_KEY,
	KEY_ACT_TYPE_ZOOM_RING,
	KEY_ACT_TYPE_MAX,
};

struct sec_keys_info {
	struct input_dev *input;
	struct list_head list;
	struct delayed_work dwork;
	int (*gpio_init)(u32, const char *);
	const char *name;
	bool wakeup;
	bool key_state;
	bool open;
	u8 act_type;
	u32 gpio;
	u32 type;
	u32 code;
	u64 debounce;
};

struct sec_keys_platform_data {
	struct sec_keys_info *key_info;
	void (*gpio_check)(void);
	bool ev_rep;
	int nkeys;
	void *data;
	bool lpcharge;
};

#endif /* _SEC_KEYS_H_ */

