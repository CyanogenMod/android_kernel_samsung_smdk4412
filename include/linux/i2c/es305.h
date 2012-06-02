/* include/linux/i2c/es305.h - audience ES305 voice processor driver
 *
 * Copyright (C) 2012 Samsung Electronics
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */


#ifndef __ES305_PDATA_H__
#define __ES305_PDATA_H__

#define MODULE_NAME "[Audience] :"
#define DEBUG	0
#define RETRY_CNT	5

enum es305_cmd {
	ES305_SW_RESET,
	ES305_SYNC,
	ES305_BOOT,
	ES305_SLEEP,
	ES305_BYPASS_DATA,
};

struct es305_platform_data {
	void (*set_mclk) (bool, bool);
	int gpio_wakeup;
	int gpio_reset;
};

int es305_load_firmware(void);
int es305_hw_reset(void);
int es305_set_cmd(enum es305_cmd);
#endif
