/*
 *  Copyright (C) 2013, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#define LIST_SIZE			5
#define MAX_DATA_COPY_TRY		2
#define WAKE_LOCK_TIMEOUT		(3*HZ)
#define COMPLETION_TIMEOUT		(2*HZ)
#define DATA				REL_RX
#define LARGE_DATA			REL_RY
#define NOTICE				REL_RZ

#define SENSORHUB_IOCTL_MAGIC		'S'
#define IOCTL_READ_LARGE_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)

#define sensorhub_info(str, args...) pr_info("%s: " str, __func__, ##args)
#define sensorhub_debug(str, args...) pr_debug("%s: " str, __func__, ##args)
#define sensorhub_err(str, args...) pr_err("%s: " str, __func__, ##args)


struct sensorhub_event {
	char *library_data;
	int library_length;
	struct list_head list;
};

struct ssp_sensorhub_data {
	struct ssp_data *ssp_data;
	struct input_dev *sensorhub_input_dev;
	struct miscdevice sensorhub_device;
	struct wake_lock sensorhub_wake_lock;
	struct completion sensorhub_completion;
	struct task_struct *sensorhub_task;
	struct sensorhub_event events_head;
	struct sensorhub_event events[LIST_SIZE];
	struct sensorhub_event *first_event;
	char *large_library_data;
	int large_library_length;
	int event_number;
	wait_queue_head_t sensorhub_wq;
	spinlock_t sensorhub_lock;
};

void ssp_sensorhub_report_notice(struct ssp_data *ssp_data, char notice);
int ssp_sensorhub_handle_data(struct ssp_data *ssp_data, char *dataframe,
				int start, int end);
int ssp_sensorhub_handle_large_data(struct ssp_data *ssp_data, u8 sub_cmd);
int ssp_sensorhub_initialize(struct ssp_data *ssp_data);
void ssp_sensorhub_remove(struct ssp_data *ssp_data);

#endif
