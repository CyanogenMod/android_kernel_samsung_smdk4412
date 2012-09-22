/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
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
#include <linux/miscdevice.h>
#include <linux/wakelock.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <asm/byteorder.h>

#ifndef __WIMAX_CMC732_H
#define __WIMAX_CMC732_H

#ifdef __KERNEL__

#define WIMAX_POWER_SUCCESS             0
#define WIMAX_ALREADY_POWER_ON          -1
#define WIMAX_POWER_FAIL		-2
#define WIMAX_ALREADY_POWER_OFF         -3

/* wimax mode */
enum {
	SDIO_MODE = 0,
	WTM_MODE,
	MAC_IMEI_WRITE_MODE,
	USIM_RELAY_MODE,
	DM_MODE,
	USB_MODE,
	AUTH_MODE
};

/* wimax power state */
enum {
	CMC_POWER_OFF = 0,
	CMC_POWER_ON,
	CMC_POWERING_OFF,
	CMC_POWERING_ON
};

/* wimax state */
enum {
	WIMAX_STATE_NOT_READY,
	WIMAX_STATE_READY,
	WIMAX_STATE_VIRTUAL_IDLE,
	WIMAX_STATE_NORMAL,
	WIMAX_STATE_IDLE,
	WIMAX_STATE_RESET_REQUESTED,
	WIMAX_STATE_RESET_ACKED,
	WIMAX_STATE_AWAKE_REQUESTED,
};

struct wimax_cfg {
	struct wake_lock	wimax_driver_lock;	/* resume wake lock */
	struct mutex power_mutex; /*serialize power on/off*/
	struct mutex suspend_mutex;
	struct work_struct		shutdown;
	struct wimax732_platform_data *pdata;
	struct notifier_block pm_notifier;
	u8		power_state;
	/* wimax mode (SDIO, USB, etc..) */
	u8              wimax_mode;
};

struct wimax732_platform_data {
	int (*power) (int);
	void (*detect) (int);
	void (*set_mode) (void);
	void (*signal_ap_active) (int);
	int (*get_sleep_mode) (void);
	int (*is_modem_awake) (void);
	void (*wakeup_assert) (int);
	struct wimax_cfg *g_cfg;
	struct miscdevice swmxctl_dev;
	int wimax_int;
	void *adapter_data;
	void (*restore_uart_path) (void);
	int uart_sel;
	int uart_sel1;
};

void s3c_bat_use_wimax(int onoff);

#endif

#endif
