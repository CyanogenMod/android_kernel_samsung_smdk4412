/*
 * Modem control driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Suchang Woo <suchang.woo@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#ifndef __MODEM_CONTROL_H__
#define __MODEM_CONTROL_H__

#define MC_SUCCESS 0
#define MC_HOST_HIGH 1
#define MC_HOST_TIMEOUT 2

struct modemctl;
struct modemctl_ops {
	void (*modem_on)(struct modemctl *);
	void (*modem_off)(struct modemctl *);
	void (*modem_reset)(struct modemctl *);
	void (*modem_boot)(struct modemctl *);
	void (*modem_suspend)(struct modemctl *);
	void (*modem_resume)(struct modemctl *);
	void (*modem_cfg_gpio)(void);
};

struct modemctl_platform_data {
	const char *name;
	unsigned gpio_phone_on;
	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;
	unsigned gpio_usim_boot;
	unsigned gpio_flm_sel;
	unsigned gpio_cp_req_reset;	/*HSIC*/
	unsigned gpio_ipc_slave_wakeup;
	unsigned gpio_ipc_host_wakeup;
	unsigned gpio_suspend_request;
	unsigned gpio_active_state;
	unsigned gpio_cp_dump_int;
	int wakeup;
	struct modemctl_ops ops;
};

struct modemctl {
	int irq[3];

	unsigned gpio_phone_on;
	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;
	unsigned gpio_usim_boot;
	unsigned gpio_flm_sel;

	unsigned gpio_cp_req_reset;
	unsigned gpio_ipc_slave_wakeup;
	unsigned gpio_ipc_host_wakeup;
	unsigned gpio_suspend_request;
	unsigned gpio_active_state;
	unsigned gpio_cp_dump_int;
	struct modemctl_ops *ops;
	struct regulator *vcc;

	struct device *dev;
	const struct attribute_group *group;

	struct delayed_work work;
	struct work_struct resume_work;
	struct work_struct cpdump_work;
	int wakeup_flag; /*flag for CP boot GPIO sync flag*/
	int cpcrash_flag;
	int boot_done;
	struct completion *l2_done;
#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock reset_lock;
#endif
	int debug_cnt;
};

extern struct platform_device modemctl;

extern int usbsvn_request_suspend(void);
extern int usbsvn_request_resume(void);

int mc_is_modem_on(void);
int mc_is_modem_active(void);
int mc_is_suspend_request(void);
int mc_is_host_wakeup(void);
int mc_prepare_resume(int);
int mc_reconnect_gpio(void);
int mc_control_pda_active(int val);
int mc_control_active_state(int val);
int mc_control_slave_wakeup(int val);
void crash_event(int type);

#endif /* __MODEM_CONTROL_H__ */
