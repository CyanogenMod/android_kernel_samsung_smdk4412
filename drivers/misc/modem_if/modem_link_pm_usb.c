/*
 * Copyright (C) 2012 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include "modem_link_pm_usb.h"

int during_hub_resume;

static inline void start_hub_work(struct link_pm_data *pm_data, int delay)
{
	if (pm_data->hub_work_running == false) {
		pm_data->hub_work_running = true;
		wake_lock(&pm_data->hub_lock);
		mif_debug("link_pm_hub_work is started\n");
		during_hub_resume = 1;
	}

	schedule_delayed_work(&pm_data->link_pm_hub, msecs_to_jiffies(delay));
}

static inline void end_hub_work(struct link_pm_data *pm_data)
{
	wake_unlock(&pm_data->hub_lock);
	pm_data->hub_work_running = false;
	mif_debug("link_pm_hub_work is done\n");
}

bool link_pm_is_connected(struct usb_link_device *usb_ld)
{
	if (has_hub(usb_ld)) {
		struct link_pm_data *pm_data = usb_ld->link_pm_data;
		if (pm_data->hub_init_lock)
			return false;

		if (pm_data->hub_status == HUB_STATE_OFF) {
			if (pm_data->hub_work_running == false)
				start_hub_work(pm_data, 0);
			return false;
		}
	}

	if (!usb_ld->if_usb_connected) {
		mif_err("mif: if not connected\n");
		return false;
	}

	return true;
}

void link_pm_preactive(struct link_pm_data *pm_data)
{
	if (pm_data->root_hub) {
		mif_info("pre-active\n");
		pm_data->hub_on_retry_cnt = 0;
		complete(&pm_data->hub_active);
		pm_runtime_put_sync(pm_data->root_hub);
	}

	pm_data->hub_status = HUB_STATE_ACTIVE;
}

static void link_pm_hub_work(struct work_struct *work)
{
	int err, cnt;
	struct link_pm_data *pm_data =
		container_of(work, struct link_pm_data, link_pm_hub.work);

	if (pm_data->hub_status == HUB_STATE_ACTIVE) {
		end_hub_work(pm_data);
		during_hub_resume = 0;
		return;
	}

	if (!pm_data->port_enable) {
		mif_err("mif: hub power func not assinged\n");
		end_hub_work(pm_data);
		return;
	}

	/* If kernel if suspend, wait the ehci resume */
	if (pm_data->dpm_suspending) {
		mif_info("dpm_suspending\n");
		start_hub_work(pm_data, 500);
		return;
	}

	switch (pm_data->hub_status) {
	case HUB_STATE_OFF:
		pm_data->hub_status = HUB_STATE_RESUMMING;
		mif_trace("hub off->on\n");

		/* skip 1st time before first probe */
		if (pm_data->root_hub)
			pm_runtime_get_sync(pm_data->root_hub);

		for (cnt=0;cnt<5;cnt++) {
			err = pm_data->port_enable(2, 1);
			if (err >= 0) {
				mif_err("hub on success\n");
				break;
			}
			mif_err("hub on fail %d th\n", cnt);
			msleep(100);
		}
		if (err < 0) {
			mif_err("hub on fail err=%d\n", err);
			err = pm_data->port_enable(2, 0);
			if (err < 0)
				mif_err("hub off fail err=%d\n", err);
			pm_data->hub_status = HUB_STATE_OFF;
			if (pm_data->root_hub)
				pm_runtime_put_sync(pm_data->root_hub);
			end_hub_work(pm_data);
		} else {
			/* resume root hub */
			start_hub_work(pm_data, 100);
		}
		break;
	case HUB_STATE_RESUMMING:
		if (pm_data->hub_on_retry_cnt++ > 50) {
			pm_data->hub_on_retry_cnt = 0;
			pm_data->hub_status = HUB_STATE_OFF;
			if (pm_data->root_hub)
				pm_runtime_put_sync(pm_data->root_hub);

			mif_err("USB Hub resume fail !!!\n");
			end_hub_work(pm_data);
		} else {
			mif_info("hub resumming: %d\n",
					pm_data->hub_on_retry_cnt);
			start_hub_work(pm_data, 200);
		}
		break;
	}
exit:
	return;
}

static int link_pm_hub_standby(void *args)
{
	struct link_pm_data *pm_data = args;
	struct usb_link_device *usb_ld = pm_data->usb_ld;
	int err = 0;

	if (!pm_data->port_enable) {
		mif_err("port power func not assinged\n");
		return -ENODEV;
	}

	err = pm_data->port_enable(2, 0);
	if (err < 0)
		mif_err("hub off fail err=%d\n", err);

	pm_data->hub_status = HUB_STATE_OFF;

	/* this function is atomic.
	 * make force disconnect in workqueue..
	 */
	return err;
}

bool link_pm_set_active(struct usb_link_device *usb_ld)
{
	int ret;
	struct link_pm_data *pm_data = usb_ld->link_pm_data;

	if (has_hub(usb_ld)) {
		if (pm_data->hub_status != HUB_STATE_ACTIVE) {
			INIT_COMPLETION(pm_data->hub_active);
			SET_SLAVE_WAKEUP(usb_ld->pdata, 1);
			ret = wait_for_completion_timeout(&pm_data->hub_active,
				msecs_to_jiffies(2000));
			if (!ret) { /*timeout*/
				mif_err("hub on timeout - retry\n");
				SET_SLAVE_WAKEUP(usb_ld->pdata, 0);
				queue_delayed_work(usb_ld->ld.tx_wq,
						&usb_ld->ld.tx_delayed_work, 0);
				return false;
			}
		}
	} else {
		/* TODO do something */
	}
	return true;
}

static long link_pm_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
{
	int value, err = 0;
	struct link_pm_data *pm_data = file->private_data;
	struct usb_link_device *usb_ld = pm_data->usb_ld;

	mif_info("cmd: 0x%08x\n", cmd);

	switch (cmd) {
	case IOCTL_LINK_CONTROL_ACTIVE:
		if (copy_from_user(&value, (const void __user *)arg,
							sizeof(int)))
			return -EFAULT;
		gpio_set_value(pm_data->gpio_link_active, value);
		mif_info("> H-ACT %d\n", value);
		break;
	case IOCTL_LINK_GET_HOSTWAKE:
		return !gpio_get_value(pm_data->gpio_link_hostwake);
	case IOCTL_LINK_CONNECTED:
		return usb_ld->if_usb_connected;
	case IOCTL_LINK_PORT_ON:
		/* ignore cp host wakeup irq, set the hub_init_lock when AP try
		 CP off and release hub_init_lock when CP boot done */
		pm_data->hub_init_lock = 0;
		if (pm_data->root_hub)
			pm_runtime_get_sync(pm_data->root_hub);
		if (pm_data->port_enable) {
			err = pm_data->port_enable(2, 1);
			if (err < 0) {
				mif_err("hub on fail err=%d\n", err);
				goto exit;
			}
			pm_data->hub_status = HUB_STATE_RESUMMING;
		}
		break;
	case IOCTL_LINK_PORT_OFF:
		err = link_pm_hub_standby(pm_data);
		if (err < 0) {
			mif_err("usb3503 standby fail\n");
			goto exit;
		}
		pm_data->hub_init_lock = 1;
		pm_data->hub_handshake_done = 0;
		break;
	default:
		break;
	}
exit:
	return err;
}

static ssize_t show_autosuspend(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct link_pm_data *pm_data = container_of(miscdev,
			struct link_pm_data, miscdev);
	struct usb_link_device *usb_ld = pm_data->usb_ld;

	p += sprintf(buf, "%s\n", pm_data->autosuspend ? "on" : "off");

	return p - buf;
}

static ssize_t store_autosuspend(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct link_pm_data *pm_data = container_of(miscdev,
			struct link_pm_data, miscdev);
	struct usb_link_device *usb_ld = pm_data->usb_ld;
	struct task_struct *task = get_current();
	char taskname[TASK_COMM_LEN];

	mif_info("autosuspend: %s: %s(%d)'\n",
			buf, get_task_comm(taskname, task), task->pid);

	if (!strncmp(buf, "on", 2)) {
		pm_data->autosuspend = true;
		if (usb_ld->usbdev)
			pm_runtime_allow(&usb_ld->usbdev->dev);
	} else if (!strncmp(buf, "off", 3)) {
		pm_data->autosuspend = false;
		if (usb_ld->usbdev)
			pm_runtime_forbid(&usb_ld->usbdev->dev);
	}

	return count;
}

static struct device_attribute attr_autosuspend =
		__ATTR(autosuspend, S_IRUGO | S_IWUSR,
		show_autosuspend, store_autosuspend);

static int link_pm_open(struct inode *inode, struct file *file)
{
	struct link_pm_data *pm_data =
		(struct link_pm_data *)file->private_data;
	file->private_data = (void *)pm_data;
	return 0;
}

static int link_pm_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations link_pm_fops = {
	.owner = THIS_MODULE,
	.open = link_pm_open,
	.release = link_pm_release,
	.unlocked_ioctl = link_pm_ioctl,
};

static int link_pm_notifier_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	struct link_pm_data *pm_data =
			container_of(this, struct link_pm_data,	pm_notifier);
	struct usb_link_device *usb_ld = pm_data->usb_ld;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		pm_data->dpm_suspending = true;
		if (has_hub(usb_ld))
			link_pm_hub_standby(pm_data);
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
		pm_data->dpm_suspending = false;
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

int link_pm_init(struct usb_link_device *usb_ld, void *data)
{
	int err;
	int irq;
	struct platform_device *pdev = (struct platform_device *)data;
	struct modem_data *pdata =
			(struct modem_data *)pdev->dev.platform_data;
	struct modemlink_pm_data *pm_pdata = pdata->link_pm_data;
	struct link_pm_data *pm_data =
			kzalloc(sizeof(struct link_pm_data), GFP_KERNEL);
	if (!pm_data) {
		mif_err("link_pm_data is NULL\n");
		return -ENOMEM;
	}
	/* get link pm data from modemcontrol's platform data */
	pm_data->gpio_link_active = pm_pdata->gpio_link_active;
	pm_data->gpio_link_hostwake = pm_pdata->gpio_link_hostwake;
	pm_data->gpio_link_slavewake = pm_pdata->gpio_link_slavewake;
	pm_data->link_reconnect = pm_pdata->link_reconnect;
	pm_data->port_enable = pm_pdata->port_enable;
	pm_data->freq_lock = pm_pdata->freq_lock;
	pm_data->freq_unlock = pm_pdata->freq_unlock;
	pm_data->autosuspend_delay_ms = pm_pdata->autosuspend_delay_ms;
	pm_data->autosuspend = true;

	pm_data->usb_ld = usb_ld;
	usb_ld->link_pm_data = pm_data;

	pm_data->miscdev.minor = MISC_DYNAMIC_MINOR;
	pm_data->miscdev.name = "link_pm";
	pm_data->miscdev.fops = &link_pm_fops;

	during_hub_resume = 0;

	err = misc_register(&pm_data->miscdev);
	if (err < 0) {
		mif_err("fail to register pm device(%d)\n", err);
		goto err_misc_register;
	}

	err = device_create_file(pm_data->miscdev.this_device,
			&attr_autosuspend);
	if (err) {
		mif_err("fail to create file: autosuspend: %d\n", err);
		goto err_create_file;
	}

	pm_data->hub_init_lock = 1;
	irq = gpio_to_irq(usb_ld->pdata->gpio_host_wakeup);
	err = request_threaded_irq(irq, NULL, usb_resume_irq,
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "modem_usb_wake", usb_ld);
	if (err) {
		mif_err("Failed to allocate an interrupt(%d)\n", irq);
		goto err_request_irq;
	}
	enable_irq_wake(irq);

	pm_data->has_usbhub = pm_pdata->has_usbhub;

	if (has_hub(usb_ld)) {
		init_completion(&pm_data->hub_active);
		pm_data->hub_status = HUB_STATE_OFF;
		pm_data->hub_handshake_done = 0;
		pm_data->root_hub = NULL;

		pm_pdata->hub_standby = link_pm_hub_standby;
		pm_pdata->hub_pm_data = pm_data;

		wake_lock_init(&pm_data->hub_lock, WAKE_LOCK_SUSPEND,
				"modem_hub_enum_lock");
		INIT_DELAYED_WORK(&pm_data->link_pm_hub, link_pm_hub_work);
		pm_data->hub_work_running = false;
	}

	pm_data->pm_notifier.notifier_call = link_pm_notifier_event;
	register_pm_notifier(&pm_data->pm_notifier);

	return 0;

err_request_irq:
err_create_file:
	misc_deregister(&pm_data->miscdev);
err_misc_register:
	kfree(pm_data);
	return err;
}
