/* linux/drivers/modem/modem.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_variation.h"
#include "modem_utils.h"

#define FMT_WAKE_TIME   (HZ/2)
#define RFS_WAKE_TIME   (HZ*3)
#define RAW_WAKE_TIME   (HZ*6)

static struct modem_ctl *create_modemctl_device(struct platform_device *pdev)
{
	int ret = 0;
	struct modem_data *pdata;
	struct modem_ctl *modemctl;
	struct device *dev = &pdev->dev;

	/* create modem control device */
	modemctl = kzalloc(sizeof(struct modem_ctl), GFP_KERNEL);
	if (!modemctl)
		return NULL;

	modemctl->dev = dev;
	modemctl->phone_state = STATE_OFFLINE;

	pdata = pdev->dev.platform_data;
	modemctl->mdm_data = pdata;
	modemctl->name = pdata->name;

	/* initialize link device list */
	INIT_LIST_HEAD(&modemctl->commons.link_dev_list);

	/* initialize tree of io devices */
	modemctl->commons.iodevs_tree_chan = RB_ROOT;
	modemctl->commons.iodevs_tree_fmt = RB_ROOT;

	/* init modemctl device for getting modemctl operations */
	ret = call_modem_init_func(modemctl, pdata);
	if (ret) {
		kfree(modemctl);
		return NULL;
	}

	modemctl->use_mif_log = pdata->use_mif_log;
	if (pdata->use_mif_log) {
		mif_err("<%s> IPC logger can be used.\n",
			pdata->name);
	}

	ret = mif_init_log(modemctl);
	if (ret < 0) {
		kfree(modemctl);
		return NULL;
	}

	mif_info("%s is created!!!\n", pdata->name);
	return modemctl;
}

static struct io_device *create_io_device(struct modem_io_t *io_t,
		struct modem_ctl *modemctl, struct modem_data *pdata)
{
	int ret = 0;
	struct io_device *iod = NULL;

	iod = kzalloc(sizeof(struct io_device), GFP_KERNEL);
	if (!iod) {
		mif_err("iod == NULL\n");
		return NULL;
	}

	rb_init_node(&iod->node_chan);
	rb_init_node(&iod->node_fmt);

	iod->name = io_t->name;
	iod->id = io_t->id;
	iod->format = io_t->format;
	iod->io_typ = io_t->io_type;
	iod->link_types = io_t->links;
	iod->net_typ = pdata->modem_net;
	iod->use_handover = pdata->use_handover;
	iod->ipc_version = pdata->ipc_version;
	atomic_set(&iod->opened, 0);

	/* link between io device and modem control */
	iod->mc = modemctl;
	if (iod->format == IPC_FMT)
		modemctl->iod = iod;
	if (iod->format == IPC_BOOT) {
		modemctl->bootd = iod;
		mif_info("Bood device = %s\n", iod->name);
	}

	/* register misc device or net device */
	ret = sipc4_init_io_device(iod);
	if (ret) {
		kfree(iod);
		mif_err("sipc4_init_io_device fail (%d)\n", ret);
		return NULL;
	}

	mif_debug("%s is created!!!\n", iod->name);
	return iod;
}

static int attach_devices(struct modem_ctl *mc, struct io_device *iod,
		enum modem_link tx_link)
{
	struct link_device *ld;

	/* add iod to rb_tree */
	if (iod->format != IPC_RAW)
		insert_iod_with_format(&mc->commons, iod->format, iod);

	if (sipc4_is_not_reserved_channel(iod->id))
		insert_iod_with_channel(&mc->commons, iod->id, iod);

	/* find link type for this io device */
	list_for_each_entry(ld, &mc->commons.link_dev_list, list) {
		if (IS_CONNECTED(iod, ld)) {
			/* The count 1 bits of iod->link_types is count
			 * of link devices of this iod.
			 * If use one link device,
			 * or, 2+ link devices and this link is tx_link,
			 * set iod's link device with ld
			 */
			if ((countbits(iod->link_types) <= 1) ||
					(tx_link == ld->link_type)) {
				mif_debug("set %s->%s\n", iod->name, ld->name);

				set_current_link(iod, ld);

				if (iod->ipc_version == SIPC_VER_42) {
					if (iod->format == IPC_FMT) {
						int ch = iod->id & 0x03;
						ld->fmt_iods[ch] = iod;
					}
				}
			}
		}
	}

	/* if use rx dynamic switch, set tx_link at modem_io_t of
	 * board-*-modems.c
	 */
	if (!get_current_link(iod)) {
		mif_err("%s->link == NULL\n", iod->name);
		BUG();
	}

	switch (iod->format) {
	case IPC_FMT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = FMT_WAKE_TIME;
		break;

	case IPC_RFS:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RFS_WAKE_TIME;
		break;

	case IPC_MULTI_RAW:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;
	case IPC_BOOT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = 3 * HZ;
	default:
		break;
	}

	return 0;
}

static int __devinit modem_probe(struct platform_device *pdev)
{
	int i;
	struct modem_data *pdata = pdev->dev.platform_data;
	struct modem_ctl *modemctl;
	struct io_device *iod[pdata->num_iodevs];
	struct link_device *ld;

	mif_err("%s\n", pdev->name);
	memset(iod, 0, sizeof(iod));

	modemctl = create_modemctl_device(pdev);
	if (!modemctl) {
		mif_err("modemctl == NULL\n");
		goto err_free_modemctl;
	}

	/* create link device */
	/* support multi-link device */
	for (i = 0; i < LINKDEV_MAX ; i++) {
		/* find matching link type */
		if (pdata->link_types & LINKTYPE(i)) {
			ld = call_link_init_func(pdev, i);
			if (!ld)
				goto err_free_modemctl;

			mif_err("link created: %s\n", ld->name);
			ld->link_type = i;
			ld->mc = modemctl;
			list_add(&ld->list, &modemctl->commons.link_dev_list);
		}
	}

	/* create io deivces and connect to modemctl device */
	for (i = 0; i < pdata->num_iodevs; i++) {
		iod[i] = create_io_device(&pdata->iodevs[i], modemctl, pdata);
		if (!iod[i]) {
			mif_err("iod[%d] == NULL\n", i);
			goto err_free_modemctl;
		}

		attach_devices(modemctl, iod[i],
				pdata->iodevs[i].tx_link);
	}

	platform_set_drvdata(pdev, modemctl);

	mif_info("Complete!!!\n");
	return 0;

err_free_modemctl:
	for (i = 0; i < pdata->num_iodevs; i++)
		if (iod[i] != NULL)
			kfree(iod[i]);

	if (modemctl != NULL)
		kfree(modemctl);

	return -ENOMEM;
}

static void modem_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_ctl *mc = dev_get_drvdata(dev);
	mc->ops.modem_off(mc);
	mc->phone_state = STATE_OFFLINE;
}

static int modem_suspend(struct device *pdev)
{
#ifndef CONFIG_LINK_DEVICE_HSIC
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 0);
#endif

	return 0;
}

static int modem_resume(struct device *pdev)
{
#ifndef CONFIG_LINK_DEVICE_HSIC
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 1);
#endif

	return 0;
}

static const struct dev_pm_ops modem_pm_ops = {
	.suspend    = modem_suspend,
	.resume     = modem_resume,
};

static struct platform_driver modem_driver = {
	.probe = modem_probe,
	.shutdown = modem_shutdown,
	.driver = {
		.name = "modem_if",
		.pm   = &modem_pm_ops,
	},
};

static int __init modem_init(void)
{
	return platform_driver_register(&modem_driver);
}

module_init(modem_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem Interface Driver");
