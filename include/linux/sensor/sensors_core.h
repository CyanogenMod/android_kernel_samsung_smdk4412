/*
 * Driver model for sensor
 *
 * Copyright (C) 2008 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __LINUX_SENSORS_CORE_H_INCLUDED
#define __LINUX_SENSORS_CORE_H_INCLUDED

extern struct device *sensors_classdev_register(char *sensors_name);
extern void sensors_classdev_unregister(struct device *dev);

struct accel_platform_data {
	int (*accel_get_position) (void);
	 /* Change axis or not for user-level
	 * If it is true, driver reports adjusted axis-raw-data
	 * to user-space based on accel_get_position() value,
	 * or if it is false, driver reports original axis-raw-data */
	bool axis_adjust;
};

struct gyro_platform_data {
	int (*gyro_get_position) (void);
	 /* Change axis or not for user-level
	 * If it is true, driver reports adjusted axis-raw-data
	 * to user-space based on gyro_get_position() value,
	 * or if it is false, driver reports original axis-raw-data */
	bool axis_adjust;
};
#endif	/* __LINUX_SENSORS_CORE_H_INCLUDED */
