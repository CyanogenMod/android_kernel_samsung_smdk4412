/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SSP_SENSORHUB__
#define __SSP_SENSORHUB__

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "ssp.h"

#define SUBCMD_GPIOWAKEUP	0X02
#define SUBCMD_POWEREUP		0X04
#define LIBRARY_MAX_NUM		5
#define LIBRARY_MAX_TRY		32
#define EVENT_WAIT_COUNT	3

/* sensorhub ioctl command */
#define SENSORHUB_IOCTL_MAGIC	'S'
#define IOCTL_READ_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)


struct sensorhub_event {
	char *library_data;
	int length;
	struct list_head list;
};

struct ssp_sensorhub_data {
	struct ssp_data *ssp_data;
	struct input_dev *sensorhub_input_dev;
	struct miscdevice sensorhub_device;
	struct wake_lock sensorhub_wake_lock;
	struct completion transfer_done;
	struct task_struct *sensorhub_task;
	struct sensorhub_event events_head;
	struct sensorhub_event events[LIBRARY_MAX_NUM + 1];
	struct sensorhub_event *first_event;
	int event_number;
	int transfer_try;
	int transfer_ready;
	int large_library_length;
	char *large_library_data;
	wait_queue_head_t sensorhub_waitqueue;
	spinlock_t sensorhub_lock;
};

void ssp_report_sensorhub_notice(struct ssp_data *data, char notice);
int ssp_handle_sensorhub_data(struct ssp_data *data, char *dataframe,
				int start, int end);
int ssp_handle_sensorhub_large_data(struct ssp_data *data, u8 sub_cmd);
int ssp_initialize_sensorhub(struct ssp_data *data);
void ssp_remove_sensorhub(struct ssp_data *data);

#endif
