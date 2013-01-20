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

static const struct fast_data {
	char library_data[3];
} fast_data_table[] = {
	{ { 1, 1, 7 } },
};

static ssize_t ssp_sensorhub_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_sensorhub_data *hub_data
		= container_of(file->private_data,
			struct ssp_sensorhub_data, sensorhub_device);
	int ret = 0;
	int i;
	u8 instruction = buf[0];

	if (count == 0) {
		pr_err("%s: library command length err(%d)", __func__, count);
		return -EINVAL;
	}

	for (i = 0; i < count; i++)
		pr_info("%s[%d] = 0x%x", __func__, i, buf[i]);

	if (buf[0] == MSG2SSP_INST_LIBRARY_REMOVE)
		instruction = REMOVE_LIBRARY;
	else if (buf[0] == MSG2SSP_INST_LIBRARY_ADD)
		instruction = ADD_LIBRARY;

	if (hub_data->ssp_data->bSspShutdown) {
		pr_err("%s: stop sending command(no ssp_data)", __func__);
		return -ENOMEM;
	}

	ret = send_instruction(hub_data->ssp_data, instruction,
		(u8)buf[1], (u8 *)(buf+2), count-2);
	if (ret <= 0)
		pr_err("%s: send library command err(%d)", __func__, ret);

	/* i2c transfer fail */
	if (ret == ERROR)
		return -EIO;
	/* no ack from MCU */
	else if (ret == FAIL)
		return -EAGAIN;
	/* success */
	else
		return count;
}

static long ssp_sensorhub_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct ssp_sensorhub_data *hub_data
		= container_of(file->private_data,
			struct ssp_sensorhub_data, sensorhub_device);
	int ret = 0;
	int i = 0;

	switch (cmd) {
	case IOCTL_READ_CONTEXT_DATA:
		/* for receive_msg */
		if (!hub_data->large_library_length &&
			hub_data->large_library_data == NULL) {

			ret = copy_to_user(argp,
				hub_data->first_event->library_data,
				hub_data->first_event->length);
			if (ret < 0) {
				pr_err("%s: send library data err(%d)",
					__func__, ret);
				complete(&hub_data->transfer_done);
				goto exit;
			}

			for (i = 0; i < hub_data->first_event->length; i++) {
				pr_info("%s[%d] = 0x%x", __func__, i,
					hub_data->first_event->library_data[i]);
			}

			hub_data->transfer_try = 0;
			complete(&hub_data->transfer_done);

		/* for receive_large_msg */
		} else {
			pr_info("%s: receive_large_msg ioctl", __func__);
			ret = copy_to_user(argp, hub_data->large_library_data,
				hub_data->large_library_length);
			if (ret < 0) {
				pr_err("%s: send large library data err(%d)",
					__func__, ret);
				goto exit;
			}

			kfree(hub_data->large_library_data);
			hub_data->large_library_length = 0;
		}
		break;

	default:
		pr_err("%s: icotl cmd err(%d)", __func__, cmd);
		ret = -EINVAL;
	}

exit:
	return ret;
}

static struct file_operations ssp_sensorhub_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.write = ssp_sensorhub_write,
	.unlocked_ioctl = ssp_sensorhub_ioctl,
};

void ssp_report_sensorhub_notice(struct ssp_data *ssp_data, char notice)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	input_report_rel(hub_data->sensorhub_input_dev, REL_RY, notice);
	input_sync(hub_data->sensorhub_input_dev);

	if (notice == MSG2SSP_AP_STATUS_WAKEUP)
		pr_info("%s: wake up", __func__);
	else if (notice == MSG2SSP_AP_STATUS_SLEEP)
		pr_info("%s: sleep", __func__);
	else if (notice == MSG2SSP_AP_STATUS_RESET)
		pr_info("%s: reset", __func__);
	else
		pr_err("%s: invalid notice", __func__);
}

static void ssp_report_sensorhub_length(struct ssp_sensorhub_data *hub_data,
				int length)
{
	input_report_rel(hub_data->sensorhub_input_dev, REL_RX, length);
	input_sync(hub_data->sensorhub_input_dev);
	pr_info("%s = %d", __func__, length);
}

static int ssp_sensorhub_is_fast_data(char *data, int start)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(fast_data_table); i++) {
		for (j = 0; j < sizeof(fast_data_table[0]); j++) {
			if (data[start + j] !=
				fast_data_table[i].library_data[j])
				break;
		}
		if (j == sizeof(fast_data_table[0]))
			return i;
	}

	return -EINVAL;
}

static int ssp_queue_sensorhub_events(struct ssp_sensorhub_data *hub_data,
				char *dataframe, int start, int end)
{
	struct sensorhub_event *event;
	int length = end - start;
	int event_number = hub_data->event_number;
	int events = 0;
	int ret = 0;
	int i = 0;

	if (length <= 0) {
		pr_err("%s: library length err(%d)", __func__, length);
		return -EINVAL;
	}

	/* how many events in the list? */
	spin_lock_bh(&hub_data->sensorhub_lock);
	list_for_each_entry(event, &hub_data->events_head.list, list)
		events++;
	spin_unlock_bh(&hub_data->sensorhub_lock);

	/* drop event if queue is full */
	if (events >= LIBRARY_MAX_NUM) {
		pr_info("%s: queue is full", __func__);
		hub_data->transfer_ready++;
		ret = ssp_sensorhub_is_fast_data(dataframe, start);
		if (ret >= 0)
			event_number = LIBRARY_MAX_NUM + ret;
		else
			return -ENOMEM;
	}

	/* allocate memory for new event */
	if (hub_data->events[event_number].library_data != NULL)
		kfree(hub_data->events[event_number].library_data);

	hub_data->events[event_number].library_data
		= kzalloc(length * sizeof(char), GFP_KERNEL);
	if (hub_data->events[event_number].library_data == NULL) {
		pr_err("%s: allocate memory for library data err", __func__);
		return -ENOMEM;
	}

	/* copy sensorhub event into queue */
	while (start < end) {
		hub_data->events[event_number].library_data[i++]
			= dataframe[start++];
		pr_info("%s[%d] = 0x%x", __func__, i-1,
			hub_data->events[event_number].library_data[i-1]);
	}
	hub_data->events[event_number].length = length;

	if (events <= LIBRARY_MAX_NUM) {
		/* add new event at the end of queue */
		spin_lock_bh(&hub_data->sensorhub_lock);
		list_add_tail(&hub_data->events[event_number].list,
			&hub_data->events_head.list);
		if (events++ < LIBRARY_MAX_NUM)
			hub_data->transfer_ready = 0;
		spin_unlock_bh(&hub_data->sensorhub_lock);

		/* do not exceed max queue number */
		if (hub_data->event_number++ >= LIBRARY_MAX_NUM - 1)
			hub_data->event_number = 0;
	} else {
		spin_lock_bh(&hub_data->sensorhub_lock);
		list_replace(hub_data->events_head.list.prev,
				&hub_data->events[event_number].list);
		spin_unlock_bh(&hub_data->sensorhub_lock);
	}

	pr_info("%s: total %d events", __func__, events);
	return events;
}

static int ssp_receive_large_msg(struct ssp_sensorhub_data *hub_data,
				u8 sub_cmd)
{
	char send_data[2] = { 0, };
	char receive_data[2] = { 0, };
	char *large_msg_data; /* Nth large msg data */
	int length = 0; /* length of Nth large msg */
	int data_locater = 0; /* large_library_data current position */
	int total_msg_number; /* total number of large msg */
	int msg_number; /* current number of large msg */
	int ret = 0;

	/* receive the first msg length */
	send_data[0] = MSG2SSP_STT;
	send_data[1] = sub_cmd;

	/* receive_data(msg length) is two byte because msg is large */
	ret = ssp_i2c_read(hub_data->ssp_data, send_data, 2,
			receive_data, 2, 0);
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
	ret = ssp_i2c_read(hub_data->ssp_data, send_data, 1,
			large_msg_data, length, 0);
	if (ret < 0) {
		pr_err("%s: receive 1st large msg err(%d)", __func__, ret);
		kfree(large_msg_data);
		return ret;
	}

	/* empty the previous large library data */
	if (hub_data->large_library_length != 0)
		kfree(hub_data->large_library_data);

	/* large_msg_data[0] of the first msg: total number of large msg
	 * large_msg_data[1-2] of the first msg: total msg length
	 * large_msg_data[3-N] of the first msg: the first msg data itself */
	total_msg_number = large_msg_data[0];
	hub_data->large_library_length
			= (int)((unsigned int)large_msg_data[1] << 8)
				+ (unsigned int)large_msg_data[2];
	hub_data->large_library_data
		= kzalloc((hub_data->large_library_length * sizeof(char)),
			GFP_KERNEL);

	/* copy the fist msg data into large_library_data */
	memcpy(hub_data->large_library_data, &large_msg_data[3],
		(length - 3) * sizeof(char));
	kfree(large_msg_data);

	data_locater = length - 3;

	/* 2nd, 3rd,...Nth msg */
	for (msg_number = 0; msg_number < total_msg_number; msg_number++) {
		/* receive Nth msg length */
		send_data[0] = MSG2SSP_STT;
		send_data[1] = 0x81 + msg_number;

		/* receive_data(msg length) is two byte because msg is large */
		ret = ssp_i2c_read(hub_data->ssp_data, send_data, 2,
				receive_data, 2, 0);
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
		ret = ssp_i2c_read(hub_data->ssp_data, send_data, 1,
				large_msg_data, length, 0);
		if (ret < 0) {
			pr_err("%s: recieve %dth large msg err(%d)",
			       __func__, msg_number + 2, ret);
			kfree(large_msg_data);
			return ret;
		}

		/* copy(append) Nth msg data into large_library_data */
		memcpy(&hub_data->large_library_data[data_locater],
			large_msg_data,	length * sizeof(char));
		data_locater += length;
		kfree(large_msg_data);
	}

	return hub_data->large_library_length;
}

static int ssp_senosrhub_thread_func(void *arg)
{
	struct ssp_sensorhub_data *hub_data = (struct ssp_sensorhub_data *)arg;
	struct sensorhub_event *event;
	int events = 0;
	int ret = 0;

	while (!kthread_should_stop()) {
		/* run if only event queue is not empty */
		wait_event_interruptible(hub_data->sensorhub_waitqueue,
				kthread_should_stop() ||
				!list_empty(&hub_data->events_head.list));

		/* exit thread if kthread should stop */
		if (unlikely(kthread_should_stop())) {
			pr_info("%s: kthread_stop()", __func__);
			break;
		}

		/* exit thread
		 * if user does not get data with consecutive trials
		 */
		if (unlikely(hub_data->transfer_try++ >= LIBRARY_MAX_TRY)) {
			pr_err("%s: user does not get data", __func__);
			break;
		}

		/* report sensorhub event to user */
		if (hub_data->transfer_ready == 0) {
			/* first in first out */
			hub_data->first_event
				= list_first_entry(&hub_data->events_head.list,
						struct sensorhub_event, list);
			if (IS_ERR(hub_data->first_event)) {
				pr_err("%s: first event err(%ld)", __func__,
					PTR_ERR(hub_data->first_event));
				continue;
			}

			/* report sensorhub event to user */
			ssp_report_sensorhub_length(hub_data,
					hub_data->first_event->length);
			wake_lock_timeout(&hub_data->sensorhub_wake_lock, 5*HZ);
			hub_data->transfer_ready++;
		}

		/* wait until user gets data */
		ret = wait_for_completion_timeout(&hub_data->transfer_done,
			3*HZ);
		if (ret == 0) {
			pr_err("%s: wait timed out", __func__);
			hub_data->transfer_ready = 0;
		} else if (ret < 0) {
			pr_err("%s: wait_for_completion_timeout err(%d)",
				__func__, ret);
		}

		/* remove first event only if transfer succeed */
		if (hub_data->transfer_try == 0) {
			/* remove first event */
			spin_lock_bh(&hub_data->sensorhub_lock);
			if (!list_empty(&hub_data->events_head.list))
				list_del(&hub_data->first_event->list);
			hub_data->transfer_ready = 0;

			/* how many events in the list? */
			events = 0;
			list_for_each_entry(event,
				&hub_data->events_head.list, list)
				events++;
			spin_unlock_bh(&hub_data->sensorhub_lock);

			pr_info("%s: %d events remain", __func__, events);
			continue;
		}

		/* throw away extra events */
		if (hub_data->transfer_ready > EVENT_WAIT_COUNT)
			hub_data->transfer_ready = 0;

		usleep_range(10000, 10000);
	}

	pr_info("%s: exit", __func__);
	return ret;
}

int ssp_handle_sensorhub_data(struct ssp_data *ssp_data, char *dataframe,
				int start, int end)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	/* add new sensorhub event into queue */
	int ret = ssp_queue_sensorhub_events(hub_data, dataframe, start, end);
	wake_up(&hub_data->sensorhub_waitqueue);

	return ret;
}

int ssp_handle_sensorhub_large_data(struct ssp_data *ssp_data, u8 sub_cmd)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	/* receive large size of library data */
	int ret = ssp_receive_large_msg(hub_data, sub_cmd);
	if (ret >= 0) {
		ssp_report_sensorhub_length(hub_data,
			hub_data->large_library_length);
		wake_lock_timeout(&hub_data->sensorhub_wake_lock, 3*HZ);
	} else {
		pr_err("%s: ssp_receive_large_msg err(%d)", __func__, ret);
	}

	return ret;
}

int ssp_initialize_sensorhub(struct ssp_data *ssp_data)
{
	struct ssp_sensorhub_data *hub_data;
	int ret;

	hub_data = kzalloc(sizeof(*hub_data), GFP_KERNEL);
	if (!hub_data) {
		pr_err("%s: failed to allocate memory for sensorhub data",
			__func__);
		return -ENOMEM;
	}
	hub_data->ssp_data = ssp_data;
	ssp_data->hub_data = hub_data;

	/* allocate sensorhub input devices */
	hub_data->sensorhub_input_dev = input_allocate_device();
	if (!hub_data->sensorhub_input_dev) {
		pr_err("%s: allocate sensorhub input devices err", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_sensorhub;
	}

	wake_lock_init(&hub_data->sensorhub_wake_lock, WAKE_LOCK_SUSPEND,
			"sensorhub_wake_lock");
	INIT_LIST_HEAD(&hub_data->events_head.list);
	init_waitqueue_head(&hub_data->sensorhub_waitqueue);
	init_completion(&hub_data->transfer_done);
	spin_lock_init(&hub_data->sensorhub_lock);

	ret = input_register_device(hub_data->sensorhub_input_dev);
	if (ret < 0) {
		pr_err("%s: could not register sensorhub input device(%d)",
			__func__, ret);
		input_free_device(hub_data->sensorhub_input_dev);
		goto err_input_register_device_sensorhub;
	}

	hub_data->sensorhub_input_dev->name = "ssp_context";
	input_set_drvdata(hub_data->sensorhub_input_dev, hub_data);
	input_set_capability(hub_data->sensorhub_input_dev, EV_REL, REL_RX);
	input_set_capability(hub_data->sensorhub_input_dev, EV_REL, REL_RY);

	/* create sensorhub device node */
	hub_data->sensorhub_device.minor = MISC_DYNAMIC_MINOR;
	hub_data->sensorhub_device.name = "ssp_sensorhub";
	hub_data->sensorhub_device.fops = &ssp_sensorhub_fops;

	ret = misc_register(&hub_data->sensorhub_device);
	if (ret < 0) {
		pr_err("%s: misc_register() failed", __func__);
		goto err_misc_register;
	}

	hub_data->sensorhub_task = kthread_run(ssp_senosrhub_thread_func,
					(void *)hub_data, "ssp_sensorhub_task");
	if (IS_ERR(hub_data->sensorhub_task)) {
		ret = PTR_ERR(hub_data->sensorhub_task);
		goto err_kthread_create;
	}

	return 0;

err_kthread_create:
	misc_deregister(&hub_data->sensorhub_device);
err_misc_register:
	input_unregister_device(hub_data->sensorhub_input_dev);
err_input_register_device_sensorhub:
	complete_all(&hub_data->transfer_done);
	wake_lock_destroy(&hub_data->sensorhub_wake_lock);
err_input_allocate_device_sensorhub:
	kfree(hub_data);
	return ret;
}

void ssp_remove_sensorhub(struct ssp_data *ssp_data)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	ssp_sensorhub_fops.write = NULL;
	ssp_sensorhub_fops.unlocked_ioctl = NULL;
	misc_deregister(&hub_data->sensorhub_device);
	input_unregister_device(hub_data->sensorhub_input_dev);
	wake_lock_destroy(&hub_data->sensorhub_wake_lock);
	complete_all(&hub_data->transfer_done);
	if (hub_data->sensorhub_task)
		kthread_stop(hub_data->sensorhub_task);
	kfree(hub_data);
}

MODULE_DESCRIPTION("Samsung Sensor Platform(SSP) sensorhub driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
