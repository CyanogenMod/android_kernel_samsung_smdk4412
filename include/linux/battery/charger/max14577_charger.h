/*
 * max14577_charger.h
 * Samsung MAX14577 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __MAX14577_CHARGER_H
#define __MAX14577_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max14577.h>
#include <linux/mfd/max14577-private.h>

/* no slave address for charger (MFD) */

/* typedef for MFD */
#define sec_charger_dev_t	struct max14577_dev
#define sec_charger_pdata_t	struct max14577_platform_data

struct sec_chg_info {
	bool dummy;
};

#endif /* __MAX14577_CHARGER_H */
