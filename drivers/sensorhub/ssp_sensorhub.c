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

#include "ssp.h"

/* sensorhub ioctl command */
#define SENSORHUB_IOCTL_MAGIC	'S'
#define IOCTL_READ_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)


static ssize_t ssp_sensorhub_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_data *data = container_of(file->private_data,
				struct ssp_data, sensorhub_device);
	int ret = 0;
	int i;
	u8 instruction = buf[0];

	if (count <= 0) {
		pr_err("%s: library command length err(%d)", __func__, count);
		return -EINVAL;
	}

	for (i = 0; i < count; i++)
		pr_info("%s[%d] = %d", __func__, i, buf[i]);

	if (buf[0] == MSG2SSP_INST_LIBRARY_REMOVE)
		instruction = REMOVE_LIBRARY;
	else if (buf[0] == MSG2SSP_INST_LIBRARY_ADD)
		instruction = ADD_LIBRARY;

	ret = send_instruction(data, instruction,
		(u8)buf[1], (u8 *)(buf+2), count-2);
	if (ret < 0)
		pr_err("%s: send library command err(%d)", __func__, ret);

	return ret;
}

static long ssp_sensorhub_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct ssp_data *data = container_of(file->private_data,
				     struct ssp_data, sensorhub_device);
	struct sensorhub_event *event;
	int ret = 0;
	int i;

	switch (cmd) {
	case IOCTL_READ_CONTEXT_DATA:
		/* for receive_msg */
		if (!data->large_library_length &&
			data->large_library_data == NULL) {
			if (list_empty(&data->events_head.list)) {
				pr_err("%s: list empty!", __func__);
				complete(&data->transfer_done);
				goto exit;
			}

			event =	list_first_entry(&data->events_head.list,
				struct sensorhub_event, list);
			if (IS_ERR(event)) {
				pr_err("%s: no sensor event entry", __func__);
				complete(&data->transfer_done);
				goto exit;
			}

			ret = copy_to_user(argp,
				event->library_data, event->library_length);
			if (ret < 0) {
				pr_err("%s: send library datar err(%d)",
					__func__, ret);
				complete(&data->transfer_done);
				goto exit;
			}

			for (i = 0; i < event->library_length; i++) {
				pr_info("%s[%d] = %d",
					__func__, i, event->library_data[i]);
			}

			list_del(&event->list);
			complete(&data->transfer_done);

		/* for receive_large_msg */
		} else {
			pr_info("%s: receive_large_msg ioctl", __func__);
			ret = copy_to_user(argp, data->large_library_data,
				data->large_library_length);
			if (ret < 0) {
				pr_err("%s: send large library data err(%d)",
					__func__, ret);
				goto exit;
			}

			kfree(data->large_library_data);
			data->large_library_length = 0;
		}
		break;

	default:
		pr_err("%s: icotl cmd err(%d)", __func__, cmd);
		ret = -EINVAL;
	}

exit:
	return ret;
}

static const struct file_operations ssp_sensorhub_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.write = ssp_sensorhub_write,
	.unlocked_ioctl = ssp_sensorhub_ioctl,
};

void ssp_report_sensorhub_notice(struct ssp_data *data, char notice)
{
	input_report_rel(data->sensorhub_input_dev, REL_RY, notice);
	input_sync(data->sensorhub_input_dev);

	if (notice == MSG2SSP_AP_STATUS_WAKEUP)
		pr_info("%s: wake up", __func__);
	else if (notice == MSG2SSP_AP_STATUS_SLEEP)
		pr_info("%s: sleep", __func__);
	else if (notice == MSG2SSP_AP_STATUS_RESET)
		pr_info("%s: reset", __func__);
}

static void ssp_report_sensorhub_length(struct ssp_data *data,
				int library_length)
{
	input_report_rel(data->sensorhub_input_dev, REL_RX, library_length);
	input_sync(data->sensorhub_input_dev);
	pr_info("%s = %d", __func__, library_length);
}

static int ssp_queue_sensorhub_events(struct ssp_data *data,
				char *dataframe, int start, int end)
{
	int length = end - start;
	int i = 0;

	if (length <= 0) {
		pr_err("%s: library length err(%d)", __func__, length);
		return -EINVAL;
	}

	/* allocate memory for new event */
	if (data->events[data->event_number].library_data != NULL)
		kfree(data->events[data->event_number].library_data);

	data->events[data->event_number].library_data
		= kzalloc(length * sizeof(char), GFP_KERNEL);
	if (data->events[data->event_number].library_data == NULL) {
		pr_err("%s: allocate memory for library data err", __func__);
		return -ENOMEM;
	}

	/* copy sensorhub event into queue */
	while (start < end) {
		data->events[data->event_number].library_data[i++]
			= dataframe[start++];
		pr_info("%s[%d] = %d", __func__, i-1,
			data->events[data->event_number].library_data[i-1]);
	}
	data->events[data->event_number].library_length = length;

	/* add new sensorhug event at the end of queue */
	list_add_tail(&data->events[data->event_number].list,
		&data->events_head.list);

	/* do not exceed max queue number */
	if (data->event_number++ >= LIBRARY_MAX_NUM - 1)
		data->event_number = 0;

	return length;
}

static int ssp_receive_large_msg(struct ssp_data *data, u8 sub_cmd)
{
	char send_data[2] = { 0, };
	char receive_data[2] = { 0, };
	char *large_msg_data; /* Nth large msg data */
	int length = 0; /* length of Nth large msg */
	int data_locater = 0; /* large_library_data current position */
	int total_msg_number; /* total number of large msg */
	int msg_number; /* current number of large msg */
	int ret = 0;

	waiting_wakeup_mcu(data);

	/* receive the first msg length */
	send_data[0] = MSG2SSP_STT;
	send_data[1] = sub_cmd;

	/* receive_data(msg length) is two byte because msg is large */
	ret = ssp_i2c_read(data, send_data, 2, receive_data, 2);
	if (ret < 0) {
		pr_err("%s: MSG2SSP_STT i2c err(%d)", __func__, ret);
		return ret;
	}

	/* get the first msg length */
	length = ((unsigned int)receive_data[0] << 8)
		+ (unsigned int)receive_data[1];
	if (length < 3) {
		/* do not print err message with power-up */
		if (sub_cmd != SUBCMD_POWEREUP)
			pr_err("%s: 1st large msg data not ready(length=%d)",
				__func__, length);
		return -EINVAL;
	}

	/* receive the first msg data */
	send_data[0] = MSG2SSP_SRM;
	large_msg_data = kzalloc((length  * sizeof(char)), GFP_KERNEL);
	ret = ssp_i2c_read(data, send_data, 1,
				large_msg_data, length);
	if (ret < 0) {
		pr_err("%s: receive 1st large msg err(%d)", __func__, ret);
		kfree(large_msg_data);
		return ret;
	}

	/* empty the previous large library data */
	if (data->large_library_length != 0)
		kfree(data->large_library_data);

	/* large_msg_data[0] of the first msg: total number of large msg
	 * large_msg_data[1-2] of the first msg: total msg length
	 * large_msg_data[3-N] of the first msg: the first msg data itself */
	total_msg_number = large_msg_data[0];
	data->large_library_length = (int)((unsigned int)large_msg_data[1] << 8)
				+ (unsigned int)large_msg_data[2];
	data->large_library_data
		= kzalloc((data->large_library_length * sizeof(char)),
			GFP_KERNEL);

	/* copy the fist msg data into large_library_data */
	memcpy(data->large_library_data, &large_msg_data[3],
		(length - 3) * sizeof(char));
	kfree(large_msg_data);

	data_locater = length - 3;

	/* 2nd, 3rd,...Nth msg */
	for (msg_number = 0; msg_number < total_msg_number; msg_number++) {
		/* receive Nth msg length */
		send_data[0] = MSG2SSP_STT;
		send_data[1] = 0x81 + msg_number;

		/* receive_data(msg length) is two byte because msg is large */
		ret = ssp_i2c_read(data, send_data, 2, receive_data, 2);
		if (ret < 0) {
			pr_err("%s: MSG2SSP_STT i2c err(%d)",
					__func__, ret);
			return ret;
		}

		/* get the Nth msg length */
		length = ((unsigned int)receive_data[0] << 8)
			+ (unsigned int)receive_data[1];
		if (length <= 0) {
			pr_err("%s: %dth large msg data not ready(length=%d)",
					__func__, msg_number + 2, length);
			return -EINVAL;
		}

		large_msg_data = kzalloc((length  * sizeof(char)),
					GFP_KERNEL);

		/* receive Nth msg data */
		send_data[0] = MSG2SSP_SRM;
		ret = ssp_i2c_read(data, send_data, 1,
					large_msg_data, length);
		if (ret < 0) {
			pr_err("%s: recieve %dth large msg err(%d)",
			       __func__, msg_number + 2, ret);
			kfree(large_msg_data);
			return ret;
		}

		/* copy(append) Nth msg data into large_library_data */
		memcpy(&data->large_library_data[data_locater],
			large_msg_data,	length * sizeof(char));
		data_locater += length;
		kfree(large_msg_data);
	}

	return data->large_library_length;
}

static int ssp_senosrhub_thread_func(void *arg)
{
	struct ssp_data *data = (struct ssp_data *)arg;
	struct sensorhub_event *event;
	int ret = 0;

	while (!kthread_should_stop() || !list_empty(&data->events_head.list)) {
		/* run if only event queue is not empty */
		wait_event_interruptible(data->sensorhub_waitqueue,
				kthread_should_stop() ||
				!list_empty(&data->events_head.list));

		/* first in first out */
		event = list_first_entry(&data->events_head.list,
			struct sensorhub_event, list);
		if (IS_ERR(event)) {
			pr_err("%s: no sensor event entry", __func__);
			continue;
		}

		/* report sensorhub event to user */
		ssp_report_sensorhub_length(data, event->library_length);
		wake_lock_timeout(&data->sensorhub_wake_lock, 5*HZ);

		/* wait until user gets data */
		ret = wait_for_completion_timeout(&data->transfer_done, 3*HZ);
		if (ret == 0) {
			pr_err("%s: wait timed out", __func__);
		} else if (ret < 0) {
			pr_err("%s: wait_for_completion_timeout err(%d)",
				__func__, ret);
		}
	}

	return ret;
}

int ssp_handle_sensorhub_data(struct ssp_data *data, char *dataframe,
				int start, int end)
{
	/* add new sensorhub event into queue */
	int ret = ssp_queue_sensorhub_events(data, dataframe, start, end);
	if (ret < 0)
		pr_err("%s: ssp_queue_sensorhub_events err(%d)", __func__, ret);
	wake_up(&data->sensorhub_waitqueue);

	return ret;
}

int ssp_handle_sensorhub_large_data(struct ssp_data *data, u8 sub_cmd)
{
	/* receive large size of library data */
	int ret = ssp_receive_large_msg(data, sub_cmd);
	if (ret >= 0) {
		ssp_report_sensorhub_length(data, data->large_library_length);
		wake_lock_timeout(&data->sensorhub_wake_lock, 3*HZ);
	} else {
		pr_err("%s: ssp_receive_large_msg err(%d)", __func__, ret);
	}

	return ret;
}

int ssp_initialize_sensorhub(struct ssp_data *data)
{
	int ret;

	/* allocate sensorhub input devices */
	data->sensorhub_input_dev = input_allocate_device();
	if (!data->sensorhub_input_dev) {
		pr_err("%s: allocate sensorhub input devices err", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_sensorhub;
	}

	wake_lock_init(&data->sensorhub_wake_lock, WAKE_LOCK_SUSPEND,
			"sensorhub_wake_lock");
	INIT_LIST_HEAD(&data->events_head.list);
	init_waitqueue_head(&data->sensorhub_waitqueue);
	init_completion(&data->transfer_done);

	ret = input_register_device(data->sensorhub_input_dev);
	if (ret < 0) {
		pr_err("%s: could not register sensorhub input device(%d)",
			__func__, ret);
		input_free_device(data->sensorhub_input_dev);
		goto err_input_register_device_sensorhub;
	}

	data->sensorhub_input_dev->name = "ssp_context";
	input_set_drvdata(data->sensorhub_input_dev, data);
	input_set_capability(data->sensorhub_input_dev, EV_REL, REL_RX);
	input_set_capability(data->sensorhub_input_dev, EV_REL, REL_RY);

	/* create sensorhub device node */
	data->sensorhub_device.minor = MISC_DYNAMIC_MINOR;
	data->sensorhub_device.name = "ssp_sensorhub";
	data->sensorhub_device.fops = &ssp_sensorhub_fops;

	ret = misc_register(&data->sensorhub_device);
	if (ret < 0) {
		pr_err("%s: misc_register() failed", __func__);
		goto err_misc_register;
	}

	data->sensorhub_task = kthread_run(ssp_senosrhub_thread_func,
					(void *)data, "ssp_sensorhub_task");
	if (IS_ERR(data->sensorhub_task)) {
		ret = PTR_ERR(data->sensorhub_task);
		goto err_kthread_create;
	}

	return 0;

err_kthread_create:
	misc_deregister(&data->sensorhub_device);
err_misc_register:
	input_unregister_device(data->sensorhub_input_dev);
err_input_register_device_sensorhub:
	complete_all(&data->transfer_done);
	wake_lock_destroy(&data->sensorhub_wake_lock);
err_input_allocate_device_sensorhub:
	return ret;
}

void ssp_remove_sensorhub(struct ssp_data *data)
{
	complete_all(&data->transfer_done);
	kthread_stop(data->sensorhub_task);
	misc_deregister(&data->sensorhub_device);
	input_unregister_device(data->sensorhub_input_dev);
	wake_lock_destroy(&data->sensorhub_wake_lock);
}

MODULE_DESCRIPTION("Samsung Sensor Platform(SSP) sensorhub driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
