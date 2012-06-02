/*
 *  smb347_charger.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  SangYoung Son <hello.son@samsung.com>
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

#ifndef __SMB347_CHARGER_H_
#define __SMB347_CHARGER_H_

struct smb_charger_callbacks {
	void (*set_charging_state) (int, int);
	int (*get_charging_state) (void);
	void (*set_charging_current) (int);
	int (*get_charging_current) (void);
	int (*get_charger_is_full) (void);
};

struct smb_charger_data {
	void (*register_callbacks)(struct smb_charger_callbacks *);
	void (*unregister_callbacks)(void);
	int enable;
	int stat;
	int ta_nconnected;
};

#endif
