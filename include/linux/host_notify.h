/*
 *  Host notify class driver
 *
 * Copyright (C) 2011 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
*/

#ifndef __LINUX_HOST_NOTIFY_H__
#define __LINUX_HOST_NOTIFY_H__

enum host_uevent_state {
	NOTIFY_HOST_NONE,
	NOTIFY_HOST_ADD,
	NOTIFY_HOST_REMOVE,
	NOTIFY_HOST_OVERCURRENT,
	NOTIFY_HOST_LOWBATT,
	NOTIFY_HOST_UNKNOWN,
};

enum otg_mode {
	NOTIFY_NONE_MODE,
	NOTIFY_HOST_MODE,
	NOTIFY_PERIPHERAL_MODE,
	NOTIFY_TEST_MODE,
};

enum booster_power {
	NOTIFY_POWER_OFF,
	NOTIFY_POWER_ON,
};

enum set_command {
	NOTIFY_SET_OFF,
	NOTIFY_SET_ON,
};

struct host_notify_dev {
	const char	*name;
	struct device	*dev;
	int		index;
	int		state;
	int		mode;
	int		booster;
	void		(*set_mode)(int);
	void		(*set_booster)(int);
};

struct host_notifier_platform_data {
	struct	host_notify_dev ndev;
	int		gpio;
	void	(*booster)(int);
	void (*powered_booster)(int);
	int		(*usbhostd_start)(void);
	int		(*usbhostd_stop)(void);
	int		thread_enable;
	int		irq_enable;
};

extern void host_state_notify(struct host_notify_dev *ndev, int state);
extern int host_notify_dev_register(struct host_notify_dev *ndev);
extern void host_notify_dev_unregister(struct host_notify_dev *ndev);

#endif /* __LINUX_HOST_NOTIFY_H__ */
