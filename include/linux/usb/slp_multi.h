/*
 * Copyright (C) 2011 Samsung Electronics
 * Yongsul Oh <yongsul96.oh@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef __LINUX_USB_SLP_MULTI_H
#define __LINUX_USB_SLP_MULTI_H

enum slp_multi_config_id {
	USB_CONFIGURATION_1 = 1,
	USB_CONFIGURATION_2 = 2,
	USB_CONFIGURATION_DUAL = 0xFF,
};

struct slp_multi_func_data {
	const char *name;
	enum slp_multi_config_id usb_config_id;
};

struct slp_multi_platform_data {
	/* for mass_storage nluns */
	unsigned nluns;

	struct slp_multi_func_data *funcs;
	unsigned nfuncs;
};

#endif /* __LINUX_USB_SLP_MULTI_H */
