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

#include "ssp.h"

static int ssp_sensorhub_print_data(const char *func_name,
					const char *data, int length)
{
	char buf[6];
	char *log_str;
	int log_size = strlen(func_name) + 2 + sizeof(buf) * length + 1;
	int i;

	log_str = kzalloc(log_size, GFP_ATOMIC);
	if (unlikely(!log_str)) {
		sensorhub_err("allocate memory for data log err");
		return -ENOMEM;
	}

	for (i = 0; i < length; i++) {
		if (i == 0) {
			strlcat(log_str, func_name, log_size);
			strlcat(log_str, ": ", log_size);
		} else {
			strlcat(log_str, ", ", log_size);
		}
		snprintf(buf, sizeof(buf), "%d", (signed char)data[i]);
		strlcat(log_str, buf, log_size);
	}

	pr_info("%s", log_str);
	kfree(log_str);
	return log_size;
}

static ssize_t ssp_sensorhub_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_sensorhub_data *hub_data
		= container_of(file->private_data,
			struct ssp_sensorhub_data, sensorhub_device);
	unsigned char instruction;
	int ret = 0;

	if (unlikely(count < 2)) {
		sensorhub_err("library data length err(%d)", count);
		return -EINVAL;
	}

	ssp_sensorhub_print_data(__func__, buf, count);

	if (unlikely(hub_data->ssp_data->bSspShutdown)) {
		sensorhub_err("stop sending library data(shutdown)");
		return -EBUSY;
	}

	if (buf[0] == MSG2SSP_INST_LIBRARY_REMOVE)
		instruction = REMOVE_LIBRARY;
	else if (buf[0] == MSG2SSP_INST_LIBRARY_ADD)
		instruction = ADD_LIBRARY;
	else
		instruction = buf[0];

	ret = send_instruction(hub_data->ssp_data, instruction,
		(unsigned char)buf[1], (unsigned char *)(buf+2), count-2);
	if (unlikely(ret <= 0)) {
		sensorhub_err("send library data err(%d)", ret);
		/* i2c transfer fail */
		if (ret == ERROR)
			return -EIO;
		/* i2c transfer done but no ack from MCU */
		else if (ret == FAIL)
			return -EAGAIN;
	}

	return count;
}

static ssize_t ssp_sensorhub_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_sensorhub_data *hub_data
		= container_of(file->private_data,
			struct ssp_sensorhub_data, sensorhub_device);
	int retries = MAX_DATA_COPY_TRY;
	int length = 0;
	int ret = 0;

	spin_lock_bh(&hub_data->sensorhub_lock);
	if (unlikely(list_empty(&hub_data->events_head.list))) {
		sensorhub_info("no library data");
		goto exit;
	}

	/* first in first out */
	hub_data->first_event
		= list_first_entry(&hub_data->events_head.list,
				struct sensorhub_event, list);
	length = hub_data->first_event->library_length;

	/* remove first event from the list */
	list_del(&hub_data->first_event->list);

	while (retries--) {
		ret = copy_to_user(buf,
			hub_data->first_event->library_data,
			hub_data->first_event->library_length);
		if (likely(!ret))
			break;
	}
	if (unlikely(ret)) {
		sensorhub_err("read library data err(%d)", ret);
		goto exit;
	}

	ssp_sensorhub_print_data(__func__,
		hub_data->first_event->library_data,
		hub_data->first_event->library_length);

	complete(&hub_data->sensorhub_completion);

exit:
	spin_unlock_bh(&hub_data->sensorhub_lock);
	return ret ? -ret : length;
}

static long ssp_sensorhub_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct ssp_sensorhub_data *hub_data
		= container_of(file->private_data,
			struct ssp_sensorhub_data, sensorhub_device);
	void __user *argp = (void __user *)arg;
	int retries = MAX_DATA_COPY_TRY;
	int length = hub_data->large_library_length;
	int ret = 0;

	switch (cmd) {
	case IOCTL_READ_LARGE_CONTEXT_DATA:
		if (unlikely(!hub_data->large_library_data
			|| !hub_data->large_library_length)) {
			sensorhub_info("no large library data");
			return 0;
		}

		while (retries--) {
			ret = copy_to_user(argp,
				hub_data->large_library_data,
				hub_data->large_library_length);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			sensorhub_err("read large library data err(%d)", ret);
			return -ret;
		}

		ssp_sensorhub_print_data(__func__,
			hub_data->large_library_data,
			hub_data->large_library_length);

		kfree(hub_data->large_library_data);
		hub_data->large_library_length = 0;
		break;

	default:
		sensorhub_err("ioctl cmd err(%d)", cmd);
		return -EINVAL;
	}

	return length;
}

static struct file_operations ssp_sensorhub_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.write = ssp_sensorhub_write,
	.read = ssp_sensorhub_read,
	.unlocked_ioctl = ssp_sensorhub_ioctl,
};

void ssp_sensorhub_report_notice(struct ssp_data *ssp_data, char notice)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	input_report_rel(hub_data->sensorhub_input_dev, NOTICE, notice);
	input_sync(hub_data->sensorhub_input_dev);

	if (notice == MSG2SSP_AP_STATUS_WAKEUP)
		sensorhub_info("wake up");
	else if (notice == MSG2SSP_AP_STATUS_SLEEP)
		sensorhub_info("sleep");
	else if (notice == MSG2SSP_AP_STATUS_RESET)
		sensorhub_info("reset");
	else
		sensorhub_err("invalid notice(0x%x)", notice);
}

static void ssp_sensorhub_report_library(struct ssp_sensorhub_data *hub_data)
{
	input_report_rel(hub_data->sensorhub_input_dev, DATA, DATA);
	input_sync(hub_data->sensorhub_input_dev);
	wake_lock_timeout(&hub_data->sensorhub_wake_lock, WAKE_LOCK_TIMEOUT);
}

static void ssp_sensorhub_report_large_library(
			struct ssp_sensorhub_data *hub_data)
{
	input_report_rel(hub_data->sensorhub_input_dev, LARGE_DATA, LARGE_DATA);
	input_sync(hub_data->sensorhub_input_dev);
	wake_lock_timeout(&hub_data->sensorhub_wake_lock, WAKE_LOCK_TIMEOUT);
}

static int ssp_sensorhub_list(struct ssp_sensorhub_data *hub_data,
				char *dataframe, int start, int end)
{
	struct list_head *list;
	int length = end - start;
	int events = 0;

	if (unlikely(length <= 0)) {
		sensorhub_err("library length err(%d)", length);
		return -EINVAL;
	}

	ssp_sensorhub_print_data(__func__, dataframe+start, length);

	spin_lock_bh(&hub_data->sensorhub_lock);
	/* how many events in the list? */
	list_for_each(list, &hub_data->events_head.list)
		events++;

	/* overwrite new event if list is full */
	if (unlikely(events >= LIST_SIZE)) {
		struct sensorhub_event *oldest_event
			= list_first_entry(&hub_data->events_head.list,
					struct sensorhub_event, list);
		list_del(&oldest_event->list);
		sensorhub_info("overwrite event");
	}

	/* allocate memory for new event */
	kfree(hub_data->events[hub_data->event_number].library_data);
	hub_data->events[hub_data->event_number].library_data
		= kzalloc(length * sizeof(char), GFP_ATOMIC);
	if (unlikely(!hub_data->events[hub_data->event_number].library_data)) {
		sensorhub_err("allocate memory for library err");
		spin_unlock_bh(&hub_data->sensorhub_lock);
		return -ENOMEM;
	}

	/* copy new event into memory */
	memcpy(hub_data->events[hub_data->event_number].library_data,
		dataframe+start, length);
	hub_data->events[hub_data->event_number].library_length = length;

	/* add new event into the end of list */
	list_add_tail(&hub_data->events[hub_data->event_number].list,
			&hub_data->events_head.list);
	spin_unlock_bh(&hub_data->sensorhub_lock);

	/* not to overflow max list capacity */
	if (hub_data->event_number++ >= LIST_SIZE - 1)
		hub_data->event_number = 0;

	return events + (events >= LIST_SIZE ? 0 : 1);
}

static int ssp_sensorhub_thread(void *arg)
{
	struct ssp_sensorhub_data *hub_data = (struct ssp_sensorhub_data *)arg;
	int ret = 0;

	while (likely(!kthread_should_stop())) {
		/* run thread if list is not empty */
		wait_event_interruptible(hub_data->sensorhub_wq,
				kthread_should_stop() ||
				!list_empty(&hub_data->events_head.list));

		/* exit thread if kthread should stop */
		if (unlikely(kthread_should_stop())) {
			sensorhub_info("kthread_stop()");
			break;
		}

		/* report sensorhub event to user */
		ssp_sensorhub_report_library(hub_data);

		/* wait until transfer finished */
		ret = wait_for_completion_timeout(
			&hub_data->sensorhub_completion, COMPLETION_TIMEOUT);
		if (unlikely(!ret))
			sensorhub_err("wait timed out");
		else if (unlikely(ret < 0))
			sensorhub_err("wait for completion err(%d)", ret);
	}

	return 0;
}

int ssp_sensorhub_handle_data(struct ssp_data *ssp_data, char *dataframe,
				int start, int end)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	/* add new sensorhub event into list */
	int ret = ssp_sensorhub_list(hub_data, dataframe, start, end);
	wake_up(&hub_data->sensorhub_wq);

	return ret;
}

static int ssp_sensorhub_receive_large_data(struct ssp_sensorhub_data *hub_data,
						unsigned char sub_cmd)
{
	static int pos; /* large_library_data current position */
	char send_data[2] = { 0, }; /* send data */
	char receive_data[5] = { 0, }; /* receive data */
	char *msg_data; /* Nth msg data */
	int total_length = 0; /* total length */
	int msg_length = 0; /* Nth msg length */
	int total_msg_number; /* total msg number */
	int msg_number; /* current msg number */
	int ret = 0;

	sensorhub_info("sub_cmd = 0x%x", sub_cmd);

	send_data[0] = MSG2SSP_STT;
	send_data[1] = sub_cmd;

	/* receive_data[0-1] : total length
	 * receive_data[2] >> 4 : total msg number
	 * receive_data[2] & 0x0F : current msg number */
	ret = ssp_i2c_read(hub_data->ssp_data, send_data, sizeof(send_data),
			receive_data, sizeof(receive_data), DEFAULT_RETRIES);
	if (unlikely(ret < 0)) {
		sensorhub_err("MSG2SSP_STT i2c err(%d)", ret);
		return ret;
	}

	/* get total length */
	total_length = ((unsigned int)receive_data[0] << 8)
			+ (unsigned int)receive_data[1];
	sensorhub_info("total length = %d", total_length);

	total_msg_number = (int)(receive_data[2] >> 4);
	msg_number = (int)(receive_data[2] & 0x0F);

	/* if this is the first msg */
	if (msg_number <= 1) {
		/* empty previous large_library_data */
		if (hub_data->large_library_length != 0)
			kfree(hub_data->large_library_data);

		/* allocate new memory for large_library_data */
		hub_data->large_library_data
			= kzalloc((total_length	* sizeof(char)), GFP_KERNEL);
		if (unlikely(!hub_data->large_library_data)) {
			sensorhub_err("allocate memory for large library err");
			return -ENOMEM;
		}
		hub_data->large_library_length = total_length;
	}

	/* get the Nth msg length */
	msg_length = ((unsigned int)receive_data[3] << 8)
			+ (unsigned int)receive_data[4];
	sensorhub_info("%dth msg length = %d", msg_number, msg_length);

	/* receive the Nth msg data */
	send_data[0] = MSG2SSP_SRM;
	msg_data = kzalloc((msg_length  * sizeof(char)), GFP_KERNEL);
	if (unlikely(!msg_data)) {
		sensorhub_err("allocate memory for msg data err");
		return -ENOMEM;
	}

	ret = ssp_i2c_read(hub_data->ssp_data, send_data, 1,
			msg_data, msg_length, 0);
	if (unlikely(ret < 0)) {
		sensorhub_err("receive %dth msg err(%d)", msg_number, ret);
		kfree(msg_data);
		return ret;
	}

	/* copy the Nth msg data into large_library_data */
	memcpy(&hub_data->large_library_data[pos],
		&msg_data[0], msg_length * sizeof(char));
	kfree(msg_data);
	pos += msg_length;

	if (msg_number < total_msg_number) {
		/* still receiving msg data */
		sensorhub_info("current msg length = %d(%d/%d)",
			msg_length, msg_number, total_msg_number);
	} else {
		/* finish receiving msg data */
		sensorhub_info("total msg length = %d(%d/%d)",
			pos, msg_number, total_msg_number);
		pos = 0;
	}

	return msg_number;
}

int ssp_sensorhub_handle_large_data(struct ssp_data *ssp_data,
					unsigned char sub_cmd)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;
	static bool err;
	static int current_msg_number = 1;
	int total_msg_number = (int)(sub_cmd >> 4);
	int msg_number = (int)(sub_cmd & 0x0F);
	int ret;

	/* skip the rest transfer if error occurs */
	if (unlikely(err)) {
		if (msg_number <= 1) {
			current_msg_number = 1;
			err = false;
		} else {
			return -EIO;
		}
	}

	/* next msg is the right one? */
	if (current_msg_number++ != msg_number) {
		sensorhub_err("next msg should be %dth but %dth",
			current_msg_number - 1, msg_number);
		sensorhub_err("skip the rest %d msg transfer",
			total_msg_number - msg_number);
		err = true;
		return -EINVAL;
	}

	/* receive large library data */
	ret = ssp_sensorhub_receive_large_data(hub_data, sub_cmd);
	if (unlikely(ret < 0)) {
		sensorhub_err("receive large msg err(%d/%d)(%d)",
			msg_number, total_msg_number, ret);
		sensorhub_err("skip the rest %d msg transfer",
			total_msg_number - msg_number);
		err = true;
		return ret;
	}

	/* finally ready to go to user */
	if (msg_number >= total_msg_number) {
		ssp_sensorhub_report_large_library(hub_data);
		current_msg_number = 1;
	}

	return ret;
}

int ssp_sensorhub_initialize(struct ssp_data *ssp_data)
{
	struct ssp_sensorhub_data *hub_data;
	int ret;

	/* allocate memory for sensorhub data */
	hub_data = kzalloc(sizeof(*hub_data), GFP_KERNEL);
	if (!hub_data) {
		sensorhub_err("allocate memory for sensorhub data err");
		ret = -ENOMEM;
		goto exit;
	}
	hub_data->ssp_data = ssp_data;
	ssp_data->hub_data = hub_data;

	/* init wakelock, list, waitqueue, completion and spinlock */
	wake_lock_init(&hub_data->sensorhub_wake_lock, WAKE_LOCK_SUSPEND,
			"ssp_sensorhub_wake_lock");
	INIT_LIST_HEAD(&hub_data->events_head.list);
	init_waitqueue_head(&hub_data->sensorhub_wq);
	init_completion(&hub_data->sensorhub_completion);
	spin_lock_init(&hub_data->sensorhub_lock);

	/* allocate sensorhub input device */
	hub_data->sensorhub_input_dev = input_allocate_device();
	if (!hub_data->sensorhub_input_dev) {
		sensorhub_err("allocate sensorhub input device err");
		ret = -ENOMEM;
		goto err_input_allocate_device_sensorhub;
	}

	/* set sensorhub input device */
	input_set_drvdata(hub_data->sensorhub_input_dev, hub_data);
	hub_data->sensorhub_input_dev->name = "ssp_context";
	input_set_capability(hub_data->sensorhub_input_dev, EV_REL, DATA);
	input_set_capability(hub_data->sensorhub_input_dev, EV_REL, LARGE_DATA);
	input_set_capability(hub_data->sensorhub_input_dev, EV_REL, NOTICE);

	/* register sensorhub input device */
	ret = input_register_device(hub_data->sensorhub_input_dev);
	if (ret < 0) {
		sensorhub_err("register sensorhub input device err(%d)", ret);
		input_free_device(hub_data->sensorhub_input_dev);
		goto err_input_register_device_sensorhub;
	}

	/* register sensorhub misc device */
	hub_data->sensorhub_device.minor = MISC_DYNAMIC_MINOR;
	hub_data->sensorhub_device.name = "ssp_sensorhub";
	hub_data->sensorhub_device.fops = &ssp_sensorhub_fops;

	ret = misc_register(&hub_data->sensorhub_device);
	if (ret < 0) {
		sensorhub_err("register sensorhub misc device err(%d)", ret);
		goto err_misc_register;
	}

	/* create and run sensorhub thread */
	hub_data->sensorhub_task = kthread_run(ssp_sensorhub_thread,
				(void *)hub_data, "ssp_sensorhub_thread");
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
err_input_allocate_device_sensorhub:
	complete_all(&hub_data->sensorhub_completion);
	wake_lock_destroy(&hub_data->sensorhub_wake_lock);
	kfree(hub_data);
exit:
	return ret;
}

void ssp_sensorhub_remove(struct ssp_data *ssp_data)
{
	struct ssp_sensorhub_data *hub_data = ssp_data->hub_data;

	ssp_sensorhub_fops.write = NULL;
	ssp_sensorhub_fops.read = NULL;
	ssp_sensorhub_fops.unlocked_ioctl = NULL;

	kthread_stop(hub_data->sensorhub_task);
	misc_deregister(&hub_data->sensorhub_device);
	input_unregister_device(hub_data->sensorhub_input_dev);
	complete_all(&hub_data->sensorhub_completion);
	wake_lock_destroy(&hub_data->sensorhub_wake_lock);
	kfree(hub_data);
}

MODULE_DESCRIPTION("Seamless Sensor Platform(SSP) sensorhub driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
