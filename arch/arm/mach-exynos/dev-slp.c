/* arch/arm/mach-exynos/dev-slp.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *	MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * Wrapper functions for legacy/obsolete kernel hacks to get QoS.
 * Supported hacks:
 * - exynos4_busfreq_lock/exynso4_busfreq_lock_free from busfreq
 * - dev_lock/dev_unlock from busfreq_opp with dev.c
 * Please note that PM QoS is the standard method to address QoS issues.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/pm_qos_params.h>
#include <mach/cpufreq.h>
#include <mach/dev.h>

static struct pm_qos_request_list qos_wrapper[DVFS_LOCK_ID_END];

/* Wrappers for obsolete legacy kernel hack (busfreq_lock/lock_free) */
int exynos4_busfreq_lock(unsigned int nId, enum busfreq_level_request lvl)
{
	s32 qos_value;

	if (WARN(nId >= DVFS_LOCK_ID_END, "incorrect nId."))
		return -EINVAL;
	if (WARN(lvl >= BUS_LEVEL_END, "incorrect level."))
		return -EINVAL;

	switch (lvl) {
	case BUS_L0:
		qos_value = 400266;
		break;
	case BUS_L1:
		qos_value = 400200;
		break;
	case BUS_L2:
		qos_value = 267200;
		break;
	case BUS_L3:
		qos_value = 267160;
		break;
	case BUS_L4:
		qos_value = 160160;
		break;
	case BUS_L5:
		qos_value = 133133;
		break;
	case BUS_L6:
		qos_value = 100100;
		break;
	default:
		qos_value = 0;
	}

	if (qos_wrapper[nId].pm_qos_class == 0) {
		pm_qos_add_request(&qos_wrapper[nId],
				   PM_QOS_BUS_DMA_THROUGHPUT, qos_value);
	} else {
		pm_qos_update_request(&qos_wrapper[nId], qos_value);
	}

	return 0;
}
void exynos4_busfreq_lock_free(unsigned int nId)
{
	if (WARN(nId >= DVFS_LOCK_ID_END, "incorrect nId."))
		return;

	if (qos_wrapper[nId].pm_qos_class)
		pm_qos_update_request(&qos_wrapper[nId],
				      PM_QOS_BUS_DMA_THROUGHPUT_DEFAULT_VALUE);
}

/* Wrappers for busfreq_opp style kernel hacks */
#define BUSFREQ_DUMMY_DEV_LOCK	0xBED4E4BC;
static struct device *busfreq_dummy = (void *) BUSFREQ_DUMMY_DEV_LOCK;

static LIST_HEAD(dev_lock_list_head);
struct dev_lock_entry {
	struct pm_qos_request_list qos;
	struct device *dev;
	struct list_head node;
};
static DEFINE_MUTEX(dev_lock_lock);

int dev_add(struct device_domain *domain, struct device *device)
{
	WARN(true, "dev_add is not supported with wrappers. "
		   "Please use PM QoS at the QoS-server side.\n");
	return -EINVAL;
}

struct device *dev_get(const char *name)
{
	if (!strcmp(name, "exynos-busfreq"))
		return busfreq_dummy;

	WARN(true, "dev_get() supports exynos-busfreq only\n");

	return ERR_PTR(-EINVAL);
}

void dev_put(const char *name)
{
	return;
}

unsigned long dev_max_freq(struct device *device)
{
	if (device != busfreq_dummy)
		return 0;

	return 400000;
}

int dev_lock(struct device *dummy, struct device *dev, unsigned long freq)
{
	struct dev_lock_entry *pos;
	struct list_head *head;

	if (dummy != busfreq_dummy)
		return -EINVAL;

	head = &dev_lock_list_head;

	mutex_lock(&dev_lock_lock);

	list_for_each_entry(pos, head, node) {
		if (pos->dev == dev)
			goto found;
	}
	pos = kzalloc(sizeof(struct dev_lock_entry), GFP_KERNEL);
	pos->dev = dev;
	pm_qos_add_request(&pos->qos, PM_QOS_BUS_DMA_THROUGHPUT,
			   PM_QOS_BUS_DMA_THROUGHPUT_DEFAULT_VALUE);
	list_add_tail(&pos->node, &dev_lock_list_head);
found:
	pm_qos_update_request(&pos->qos, freq);

	mutex_unlock(&dev_lock_lock);

	return 0;
}

int dev_unlock(struct device *dummy, struct device *dev)
{
	return dev_lock(dummy, dev, 0);
}

int dev_lock_list(struct device *device, char *buf)
{
	int count = 0;
	struct dev_lock_entry *pos;
	struct list_head *head;

	head = &dev_lock_list_head;
	count = sprintf(buf, "Lock List\n");
	mutex_lock(&dev_lock_lock);
	list_for_each_entry(pos, head, node) {
		count  += sprintf(buf + count, "%s : %d\n",
				  dev_name(pos->dev),
				  pos->qos.list.prio);
	}
	mutex_unlock(&dev_lock_lock);

	return count;
}

