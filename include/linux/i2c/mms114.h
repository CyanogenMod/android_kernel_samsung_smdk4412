/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MMS114_H
#define __LINUX_MMS114_H

struct mms114_platform_data {
	unsigned int x_size;
	unsigned int y_size;
	unsigned int contact_threshold;
	unsigned int moving_threshold;
	bool x_invert;
	bool y_invert;

	void (*cfg_pin)(bool);
};

#endif	/* __LINUX_MMS114_H */
