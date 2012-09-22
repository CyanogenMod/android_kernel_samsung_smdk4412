/*
 * Modem control driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Suchang Woo <suchang.woo@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <linux/phone_svn/modemctl.h>

#define HOST_WUP_LEVEL 0

#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
static int sprd_boot_complete;
#endif

enum {
	HOST_WAKEUP_LOW = 1,
	HOST_WAKEUP_WAIT_RESET,
} HOST_WAKEUP_STATE;

enum {
	SVNET_ERROR_RESET,
	SVNET_ERROR_CRASH,
} SVNET_ERROR_TYPE;

/* FIXME: Don't use this except pm */
static struct modemctl *global_mc;

int mc_is_modem_on(void)
{
	if (!global_mc)
		return 0;

	return gpio_get_value(global_mc->gpio_phone_on);
}
EXPORT_SYMBOL_GPL(mc_is_modem_on);

int mc_is_modem_active(void)
{
	if (!global_mc)
		return 0;

	return gpio_get_value(global_mc->gpio_phone_active);
}

int mc_control_active_state(int val)
{
	if (!global_mc)
		return -EFAULT;

#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#endif

	if (mc_is_modem_on()) {
		gpio_set_value(global_mc->gpio_active_state, val ? 1 : 0);
		dev_dbg(global_mc->dev, "ACTIVE_STATE:%d\n", val ? 1 : 0);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mc_control_active_state);

int mc_control_pda_active(int val)
{
	if (!global_mc)
		return -EFAULT;

	if (mc_is_modem_on()) {
		gpio_set_value(global_mc->gpio_pda_active , val ? 1 : 0);
		dev_dbg(global_mc->dev, "PDA_ACTIVE:%d\n", val ? 1 : 0);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mc_control_pda_active);

int mc_control_slave_wakeup(int val)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	if (!global_mc)
		return -EFAULT;

	gpio_set_value(global_mc->gpio_ipc_slave_wakeup, val ? 1 : 0);
	dev_dbg(global_mc->dev, ">SLAV_WUP:%d,%d\n", val ? 1 : 0,
		gpio_get_value(global_mc->gpio_ipc_slave_wakeup));
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(mc_control_slave_wakeup);

int mc_is_host_wakeup(void)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	if (!global_mc)
		return 0;

	return (gpio_get_value(global_mc->gpio_ipc_host_wakeup)
		== HOST_WUP_LEVEL) ? 1 : 0;
#endif
}
EXPORT_SYMBOL_GPL(mc_is_host_wakeup);

int mc_is_suspend_request(void)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	if (!global_mc)
		return 0;

	printk(KERN_DEBUG "%s:suspend requset val=%d\n", __func__,
		gpio_get_value(global_mc->gpio_suspend_request));
	return gpio_get_value(global_mc->gpio_suspend_request);
#endif
}

int mc_prepare_resume(int ms_time)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	int val;
	struct completion done;
	struct modemctl *mc = global_mc;

	if (!mc)
		return -EFAULT;

	val = gpio_get_value(mc->gpio_ipc_slave_wakeup);
	if (val) {
		gpio_set_value(mc->gpio_ipc_slave_wakeup, 0);
		dev_dbg(mc->dev, "svn SLAV_WUP:reset\n");
	}
	val = gpio_get_value(mc->gpio_ipc_host_wakeup);
	if (val == HOST_WUP_LEVEL) {
		dev_dbg(mc->dev, "svn HOST_WUP:high!\n");
		return MC_HOST_HIGH;
	}

	init_completion(&done);
	mc->l2_done = &done;
	gpio_set_value(mc->gpio_ipc_slave_wakeup, 1);
	dev_dbg(mc->dev, "svn >SLAV_WUP:1,%d\n",
		gpio_get_value(mc->gpio_ipc_slave_wakeup));

	if (!wait_for_completion_timeout(&done, ms_time)) {
		val = gpio_get_value(mc->gpio_ipc_host_wakeup);
		if (val == HOST_WUP_LEVEL) {
			dev_err(mc->dev, "maybe complete late.. %d\n", ms_time);
			mc->l2_done = NULL;
			return MC_SUCCESS;
		}
		dev_err(mc->dev, "Modem wakeup timeout %d\n", ms_time);
		gpio_set_value(mc->gpio_ipc_slave_wakeup, 0);
		dev_dbg(mc->dev, "svn >SLAV_WUP:0,%d\n",
			gpio_get_value(mc->gpio_ipc_slave_wakeup));
		mc->l2_done = NULL;
		return MC_HOST_TIMEOUT;
	}
	return MC_SUCCESS;
#endif
}

int mc_reconnect_gpio(void)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	struct modemctl *mc = global_mc;

	if (!mc)
		return -EFAULT;

	dev_err(mc->dev, "TRY Reconnection...\n");

	gpio_set_value(mc->gpio_active_state, 0);
	msleep(20);
	gpio_set_value(mc->gpio_ipc_slave_wakeup, 1);
	msleep(20);
	gpio_set_value(mc->gpio_ipc_slave_wakeup, 0);
	msleep(20);
	gpio_set_value(mc->gpio_active_state, 1);

	return 0;
#endif
}

int mc_is_cp_dump_int(void)
{
	if (!global_mc)
		return 0;

	return gpio_get_value(global_mc->gpio_cp_dump_int);
}

#ifdef CONFIG_SEC_DEBUG
/*
 * HSIC CP uploas scenario -
 * 1. CP send Crash message
 * 2. Rild save the ram data to file via HSIC
 * 3. Rild call the kernel_upload() for AP ram dump
 */
static void kernel_upload(struct modemctl *mc)
{
	/*TODO: check the DEBUG LEVEL*/
	if (mc->cpcrash_flag)
		panic("CP Crash");
	else
		panic("HSIC Disconnected");
}
#else
static void kernel_upload(struct modemctl *mc) {}
#endif

static int modem_on(struct modemctl *mc)
{
	dev_dbg(mc->dev, "%s\n", __func__);
	if (!mc->ops || !mc->ops->modem_on)
		return -ENXIO;

#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	sprd_boot_complete = 0;
#endif

	mc->ops->modem_on(mc);

	return 0;
}

static int modem_off(struct modemctl *mc)
{
	dev_dbg(mc->dev, "%s\n", __func__);
	if (!mc->ops || !mc->ops->modem_off)
		return -ENXIO;

	mc->ops->modem_off(mc);

	return 0;
}

static int modem_reset(struct modemctl *mc)
{
	dev_dbg(mc->dev, "%s\n", __func__);
	if (!mc->ops || !mc->ops->modem_reset)
		return -ENXIO;

	mc->ops->modem_reset(mc);

	return 0;
}

static int modem_boot(struct modemctl *mc)
{
	dev_dbg(mc->dev, "%s\n", __func__);
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	sprd_boot_complete = 1;
#else
	if (!mc->ops || !mc->ops->modem_boot)
		return -ENXIO;

	mc->ops->modem_boot(mc);
#endif
	return 0;
}

static int modem_get_active(struct modemctl *mc)
{
	dev_dbg(mc->dev, "%s\n", __func__);
	if (!mc->gpio_phone_active || !mc->gpio_cp_reset)
		return -ENXIO;

	dev_dbg(mc->dev, "cp %d phone %d\n",
			gpio_get_value(mc->gpio_cp_reset),
			gpio_get_value(mc->gpio_phone_active));

	if (gpio_get_value(mc->gpio_phone_active))
		return 1;
	else
		return 0;

	if (gpio_get_value(mc->gpio_cp_reset))
		return !!gpio_get_value(mc->gpio_phone_active);

	return 0;
}

static ssize_t show_control(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	struct modemctl *mc = dev_get_drvdata(d);
	struct modemctl_ops *ops = mc->ops;

	if (ops) {
		if (ops->modem_on)
			p += sprintf(p, "on ");
		if (ops->modem_off)
			p += sprintf(p, "off ");
		if (ops->modem_reset)
			p += sprintf(p, "reset ");
		if (ops->modem_boot)
			p += sprintf(p, "boot ");
	} else {
		p += sprintf(p, "(No ops)");
	}

	p += sprintf(p, "\n");
	return p - buf;
}

static ssize_t store_control(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct modemctl *mc = dev_get_drvdata(d);

	if (!strncmp(buf, "on", 2)) {
		modem_on(mc);
		return count;
	}

	if (!strncmp(buf, "off", 3)) {
		modem_off(mc);
		return count;
	}

	if (!strncmp(buf, "reset", 5)) {
		modem_reset(mc);
		return count;
	}

	if (!strncmp(buf, "boot", 4)) {
		modem_boot(mc);
		return count;
	}

	if (!strncmp(buf, "daolpu", 6)) {
		kernel_upload(mc);
		return count;
	}

	return count;
}

static ssize_t show_status(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	struct modemctl *mc = dev_get_drvdata(d);

	p += sprintf(p, "%d\n", modem_get_active(mc));

	return p - buf;
}

static ssize_t show_wakeup(struct device *d,
		struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else
	struct modemctl *mc = dev_get_drvdata(d);
	int count = 0;

	if (!mc->gpio_ipc_host_wakeup)
		return -ENXIO;

	count += sprintf(buf + count, "%d\n",
			mc->wakeup_flag);

	return count;
#endif
}

static ssize_t store_wakeup(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return 0;
#else

	struct modemctl *mc = dev_get_drvdata(d);

	if (!strncmp(buf, "reset", 5)) {
		mc->wakeup_flag = HOST_WAKEUP_WAIT_RESET;
		dev_dbg(mc->dev, "%s: wakup_flag %d\n",
			__func__, mc->wakeup_flag);
		return count;
	}
	return 0;
#endif
}

static ssize_t show_debug(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	int i;
	struct modemctl *mc = dev_get_drvdata(d);

	for (i = 0; i < ARRAY_SIZE(mc->irq); i++) {
		if (mc->irq[i])
			p += sprintf(p, "Irq %d: %d\n", i, mc->irq[i]);
	}

	p += sprintf(p, "GPIO ----\n");

	if (mc->gpio_phone_on)
		p += sprintf(p, "\t%3d %d : phone on\n", mc->gpio_phone_on,
				gpio_get_value(mc->gpio_phone_on));
	if (mc->gpio_phone_active)
		p += sprintf(p, "\t%3d %d : phone active\n",
				mc->gpio_phone_active,
				gpio_get_value(mc->gpio_phone_active));
	if (mc->gpio_pda_active)
		p += sprintf(p, "\t%3d %d : pda active\n", mc->gpio_pda_active,
				gpio_get_value(mc->gpio_pda_active));
	if (mc->gpio_cp_reset)
		p += sprintf(p, "\t%3d %d : CP reset\n", mc->gpio_cp_reset,
				gpio_get_value(mc->gpio_cp_reset));
	if (mc->gpio_usim_boot)
		p += sprintf(p, "\t%3d %d : USIM boot\n", mc->gpio_usim_boot,
				gpio_get_value(mc->gpio_usim_boot));
	if (mc->gpio_flm_sel)
		p += sprintf(p, "\t%3d %d : FLM sel\n", mc->gpio_flm_sel,
				gpio_get_value(mc->gpio_flm_sel));

	p += sprintf(p, "Support types ---\n");

	return p - buf;
}
static DEVICE_ATTR(control, 0664, show_control, store_control);
static DEVICE_ATTR(status, S_IRUGO, show_status, NULL);
static DEVICE_ATTR(wakeup, 0664, show_wakeup, store_wakeup);
static DEVICE_ATTR(debug, S_IRUGO, show_debug, NULL);

static struct attribute *modemctl_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_status.attr,
	&dev_attr_wakeup.attr,
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group modemctl_group = {
	.attrs = modemctl_attributes,
};

void crash_event(int type)
{
	char *envs[2] = { NULL, NULL };

	if (!global_mc)
		return;
#ifdef CONFIG_SEC_DEBUG
	envs[0] = (type == SVNET_ERROR_CRASH) ? "MAILBOX=cp_exit"
		: "MAILBOX=cp_reset";
#else
	envs[0] = "MAILBOX=cp_reset";
#endif
	kobject_uevent_env(&global_mc->dev->kobj, KOBJ_OFFLINE, envs);
}

/*
 * mc_work - PHONE_ACTIVE irq
 *  After
 *	CP Crash : PHONE_ACTIVE(L) + CP_DUMP_INT(H) (0xC9)
 *	CP Reset : PHONE_ACTIVE(L) + CP_DUMP_INT(L) (0xC7)
 *  Before
 *	CP Crash : PHONE_ACTIVE(L) + SUSPEND_REQUEST(H) (0xC9)
 *	CP Reset : PHONE_ACTIVE(L) + SUSPEND_REQUEST(L) (0xC7)
 */
static void mc_work(struct work_struct *work_arg)
{
	struct modemctl *mc = container_of(work_arg, struct modemctl,
		work.work);
	int error;
	int cpdump_int;
	char *envs[2] = { NULL, NULL };

#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	if (!sprd_boot_complete)
		return;
#endif

	error = modem_get_active(mc);
	if (error < 0) {
		dev_err(mc->dev, "Not initialized\n");
		return;
	}

	cpdump_int = gpio_get_value(mc->gpio_cp_dump_int);
	dev_dbg(mc->dev, "PHONE ACTIVE: %d CP_DUMP_INT: %d\n",
		error, cpdump_int);

	if ((!error) && cpdump_int) {
		dev_err(mc->dev, "CP abnormal dead\n");
		dev_err(mc->dev, "(%d) send uevent to RIL [cpdump_int:%d]\n",
			__LINE__, cpdump_int);
		envs[0] = "MAILBOX=cp_reset";
		kobject_uevent_env(&mc->dev->kobj, KOBJ_OFFLINE, envs);
		sprd_boot_complete = 0;
		return;
	}
	envs[0] = cpdump_int ? "MAILBOX=cp_exit" : "MAILBOX=cp_reset";

	if (error && gpio_get_value(global_mc->gpio_phone_on)) {
		mc->cpcrash_flag = 0;
		mc->boot_done = 1;
		kobject_uevent(&mc->dev->kobj, KOBJ_ONLINE);
		wake_unlock(&mc->reset_lock);
	} else if (mc->boot_done) {
		if (modem_get_active(mc)) {
			wake_unlock(&mc->reset_lock);
			return;
		}
		if (mc->cpcrash_flag > 3) {
			dev_dbg(mc->dev, "send uevent to RIL [cpdump_int:%d]\n",
				cpdump_int);
			kobject_uevent_env(&mc->dev->kobj, KOBJ_OFFLINE, envs);
		} else {
			mc->cpcrash_flag++;
			schedule_delayed_work(&mc->work, msecs_to_jiffies(50));
		}
	}
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	if (cpdump_int) {
		dev_dbg(mc->dev, "send uevent to RIL [cpdump_int:%d]\n",
			cpdump_int);
		kobject_uevent_env(&mc->dev->kobj, KOBJ_OFFLINE, envs);
	}
#endif
}

static irqreturn_t modemctl_resume_thread(int irq, void *dev_id)
{
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	return IRQ_HANDLED;
#else
	struct modemctl *mc = (struct modemctl *)dev_id;
	int val = gpio_get_value(mc->gpio_ipc_host_wakeup);
	int err;

	dev_dbg(mc->dev, "svn <HOST_WUP:%d\n", val);
	if (val != HOST_WUP_LEVEL) {
		if (mc->l2_done) {
			complete(mc->l2_done);
			mc->l2_done = NULL;
		}
		gpio_set_value(mc->gpio_ipc_slave_wakeup, 0);
		dev_dbg(mc->dev, "svn >SLAV_WUP:0,%d\n",
			gpio_get_value(mc->gpio_ipc_slave_wakeup));
		mc->debug_cnt = 0;
		return IRQ_HANDLED;
	}

	if (val == HOST_WUP_LEVEL)
		mc->debug_cnt++;

	if (mc->debug_cnt > 30) {
		dev_err(mc->dev, "Abnormal Host wakeup -- over 30times");
		disable_irq(irq);
		mc->debug_cnt = 0;
		crash_event(SVNET_ERROR_RESET);
		/*crash_event(SVNET_ERROR_CRASH);*/
		/*panic("HSIC Disconnected");*/
		msleep(1000);
		enable_irq(irq);
	}

	if (!val
		&& mc->wakeup_flag == HOST_WAKEUP_WAIT_RESET) {
		mc->wakeup_flag = HOST_WAKEUP_LOW;
		dev_dbg(mc->dev, "%s: wakeup flag (%d)\n",
			__func__, mc->wakeup_flag);
	}
	return IRQ_HANDLED;
#endif
}

static irqreturn_t modemctl_irq_handler(int irq, void *dev_id)
{
	struct modemctl *mc = (struct modemctl *)dev_id;

	wake_lock_timeout(&mc->reset_lock, HZ*30);
	if (!work_pending(&mc->work.work))
		schedule_delayed_work(&mc->work, msecs_to_jiffies(100));
		/*schedule_work(&mc->work);*/

	return IRQ_HANDLED;
}

static void mc_cpdump_worker(struct work_struct *work)
{
	struct modemctl *mc = container_of(work, struct modemctl, cpdump_work);
	int val = gpio_get_value(mc->gpio_cp_dump_int);

	dev_dbg(mc->dev, "CP_DUMP_INT:%d\n", val);
}

static irqreturn_t modemctl_cpdump_irq(int irq, void *dev_id)
{
	struct modemctl *mc = (struct modemctl *)dev_id;
	int val = gpio_get_value(mc->gpio_cp_dump_int);

	dev_dbg(mc->dev, "CP_DUMP_INT:%d\n", val);

	if (!work_pending(&mc->cpdump_work))
		schedule_work(&mc->cpdump_work);

	return IRQ_HANDLED;
}

static void _free_all(struct modemctl *mc)
{
	int i;

	if (mc) {
		if (mc->ops)
			mc->ops = NULL;

		if (mc->group)
			sysfs_remove_group(&mc->dev->kobj, mc->group);

		for (i = 0; i < ARRAY_SIZE(mc->irq); i++) {
			if (mc->irq[i])
				free_irq(mc->irq[i], mc);
		}

		kfree(mc);
	}
}

static int __devinit modemctl_probe(struct platform_device *pdev)
{
	struct modemctl_platform_data *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	struct modemctl *mc;
	int irq;
	int error;

	printk(KERN_ERR "[%s]\n ", __func__);

	if (!pdata) {
		dev_err(dev, "No platform data\n");
		return -EINVAL;
	}

	mc = kzalloc(sizeof(struct modemctl), GFP_KERNEL);
	if (!mc) {
		dev_err(dev, "Failed to allocate device\n");
		return -ENOMEM;
	}

	mc->gpio_phone_on = pdata->gpio_phone_on;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_cp_req_reset = pdata->gpio_cp_req_reset;
#if !defined(CONFIG_CHN_CMCC_SPI_SPRD)
	mc->gpio_ipc_slave_wakeup = pdata->gpio_ipc_slave_wakeup;
	mc->gpio_ipc_host_wakeup = pdata->gpio_ipc_host_wakeup;
#endif
	mc->gpio_suspend_request = pdata->gpio_suspend_request;
	mc->gpio_active_state = pdata->gpio_active_state;
	mc->gpio_usim_boot = pdata->gpio_usim_boot;
	mc->gpio_flm_sel = pdata->gpio_flm_sel;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->ops = &pdata->ops;
	mc->dev = dev;
	dev_set_drvdata(mc->dev, mc);

	error = sysfs_create_group(&mc->dev->kobj, &modemctl_group);
	if (error) {
		dev_err(dev, "Failed to create sysfs files\n");
		goto fail;
	}
	mc->group = &modemctl_group;

	INIT_DELAYED_WORK(&mc->work, mc_work);
	INIT_WORK(&mc->cpdump_work, mc_cpdump_worker);
	wake_lock_init(&mc->reset_lock, WAKE_LOCK_SUSPEND, "modemctl");

	mc->ops->modem_cfg_gpio();

	irq = gpio_to_irq(pdata->gpio_phone_active);
	error = request_irq(irq, modemctl_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"phone_active", mc);
	if (error) {
		dev_err(dev, "Failed to allocate an interrupt(%d)\n", irq);
		goto fail;
	}
	mc->irq[0] = irq;
	enable_irq_wake(irq);
#if !defined(CONFIG_CHN_CMCC_SPI_SPRD)
	irq = gpio_to_irq(pdata->gpio_ipc_host_wakeup);

	error = request_threaded_irq(irq, NULL, modemctl_resume_thread,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"IPC_HOST_WAKEUP", mc);

	if (error) {
		dev_err(dev, "Failed to allocate an interrupt(%d)\n", irq);
		goto fail;
	}
	mc->irq[1] = irq;
	enable_irq_wake(irq);
#endif
	irq = gpio_to_irq(pdata->gpio_cp_dump_int);
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
	error = request_irq(irq, modemctl_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"CP_DUMP_INT", mc);
#else
	error = request_irq(irq, modemctl_cpdump_irq,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"CP_DUMP_INT", mc);
#endif
	if (error) {
		dev_err(dev, "Failed to allocate an interrupt(%d)\n", irq);
		goto fail;
	}
	mc->irq[2] = irq;
	enable_irq_wake(irq);
	mc->debug_cnt = 0;

	device_init_wakeup(&pdev->dev, pdata->wakeup);
	platform_set_drvdata(pdev, mc);
	global_mc = mc;
	printk("[%s Done]\n ", __func__);
	return 0;

fail:
	_free_all(mc);
	return error;
}

static int __devexit modemctl_remove(struct platform_device *pdev)
{
	struct modemctl *mc = platform_get_drvdata(pdev);

	flush_work(&mc->work.work);
	flush_work(&mc->cpdump_work);
	platform_set_drvdata(pdev, NULL);
	wake_lock_destroy(&mc->reset_lock);
	_free_all(mc);
	return 0;
}

#ifdef CONFIG_PM
static int modemctl_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct modemctl *mc = platform_get_drvdata(pdev);

	if (mc->ops && mc->ops->modem_suspend)
		mc->ops->modem_suspend(mc);

	if (device_may_wakeup(dev) && mc_is_modem_on())
		enable_irq_wake(mc->irq[1]);

	return 0;
}

static int modemctl_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct modemctl *mc = platform_get_drvdata(pdev);

	if (device_may_wakeup(dev) && mc_is_modem_on())
		disable_irq_wake(mc->irq[1]);

	if (mc->ops && mc->ops->modem_resume)
		mc->ops->modem_resume(mc);

	return 0;
}

static const struct dev_pm_ops modemctl_pm_ops = {
	.suspend	= modemctl_suspend,
	.resume		= modemctl_resume,
};
#endif

static struct platform_driver modemctl_driver = {
	.probe		= modemctl_probe,
	.remove		= __devexit_p(modemctl_remove),
	.driver		= {
		.name	= "modemctl",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &modemctl_pm_ops,
#endif
	},
};

static int __init modemctl_init(void)
{
	int retval;
	retval = platform_device_register(&modemctl);
	if (retval < 0)
		return retval;
	return platform_driver_register(&modemctl_driver);
}

static void __exit modemctl_exit(void)
{
	platform_driver_unregister(&modemctl_driver);
}

module_init(modemctl_init);
module_exit(modemctl_exit);

MODULE_DESCRIPTION("Modem control driver");
MODULE_AUTHOR("Suchang Woo <suchang.woo@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:modemctl");
