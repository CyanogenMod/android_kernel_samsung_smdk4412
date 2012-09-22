/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Authors:
 *	Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include "drmP.h"
#include "drm_backlight.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/cma.h>
#include <plat/map-base.h>

#include <drm/exynos_drm.h>
#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_iommu.h"
#include "exynos_drm_ipp.h"

/*
 * IPP is stand for Image Post Processing and
 * supports image scaler/rotator and input/output DMA operations.
 * using FIMC, GSC, Rotator, so on.
 * IPP is integration device driver of same attribute h/w
 */

#define get_ipp_context(dev)	platform_get_drvdata(to_platform_device(dev))

/*
 * A structure of event.
 *
 * @base: base of event.
 * @event: ipp event.
 */
struct drm_exynos_ipp_send_event {
	struct drm_pending_event	base;
	struct drm_exynos_ipp_event	event;
};

/*
 * A structure of command node.
 *
 * @list: list head to command queue information.
 * @mem_list: list head to source,destination memory queue information.
 * @property: property information.
 */
struct drm_exynos_ipp_cmd_node {
	struct list_head	list;
	struct list_head	mem_list[EXYNOS_DRM_OPS_MAX];
	struct drm_exynos_ipp_property	property;
};

/*
 * A structure of memory node.
 *
 * @list: list head to memory queue information.
 * @ops_id: id of operations.
 * @prop_id: id of property.
 * @buf_id: id of buffer.
 * @buf_info: gem objects and dma address, size.
 */
struct drm_exynos_ipp_mem_node {
	struct list_head	list;
	enum drm_exynos_ops_id	ops_id;
	u32	prop_id;
	u32	buf_id;
	struct drm_exynos_ipp_buf_info	buf_info;
};

/*
 * A structure of ipp context.
 *
 * @subdrv: prepare initialization using subdrv.
 * @lock: locking of operations.
 * @ipp_idr: ipp driver idr.
 * @sched_event: schdule event list
 * @sched_cmd: schdule command list
 */
struct ipp_context {
	struct exynos_drm_subdrv	subdrv;
	struct mutex	lock;
	struct idr	ipp_idr;
	struct work_struct	sched_event;
	struct work_struct	sched_cmd;
};

static LIST_HEAD(exynos_drm_ippdrv_list);
static BLOCKING_NOTIFIER_HEAD(exynos_drm_ippnb_list);

int exynos_drm_ippdrv_register(struct exynos_drm_ippdrv *ippdrv)
{
	DRM_DEBUG_KMS("%s\n", __func__);

	if (!ippdrv)
		return -EINVAL;

	list_add_tail(&ippdrv->list, &exynos_drm_ippdrv_list);

	return 0;
}
EXPORT_SYMBOL_GPL(exynos_drm_ippdrv_register);

int exynos_drm_ippdrv_unregister(struct exynos_drm_ippdrv *ippdrv)
{
	DRM_DEBUG_KMS("%s\n", __func__);

	if (!ippdrv)
		return -EINVAL;

	list_del(&ippdrv->list);

	return 0;
}
EXPORT_SYMBOL_GPL(exynos_drm_ippdrv_unregister);

int exynos_drm_ipp_get_property(struct drm_device *drm_dev, void *data,
					struct drm_file *file)
{
	struct exynos_drm_ippdrv *ippdrv;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* get ipp driver entry */
	list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {
		/* check idle state and dedicated state */
		if (ippdrv->state == IPP_STATE_START &&
		    ippdrv->dedicated)
			continue;

		/* ToDo: get property */
		DRM_DEBUG_KMS("%s:ipp_id[%d]\n", __func__, ippdrv->ipp_id);

		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(exynos_drm_ipp_get_property);

static int ipp_create_id(struct idr *id_idr, void *obj, u32 *idp)
{
	int ret = -EINVAL;

	/* ToDo: need spin_lock ? */

again:
	/* ensure there is space available to allocate a handle */
	if (idr_pre_get(id_idr, GFP_KERNEL) == 0)
		return -ENOMEM;

	ret = idr_get_new_above(id_idr, obj, 1, (int *)idp);
	if (ret == -EAGAIN)
		goto again;

	return ret;
}

static void *ipp_find_id(struct idr *id_idr, u32 id)
{
	void *obj;

	/* ToDo: need spin_lock ? */

	/* find object using handle */
	obj = idr_find(id_idr, id);
	if (obj == NULL)
		return NULL;

	return obj;
}

static struct exynos_drm_ippdrv
	*ipp_find_driver(struct ipp_context *ctx,
	struct drm_exynos_ipp_property *property)
{
	struct exynos_drm_ippdrv *ippdrv;
	u32 ipp_id = property->ipp_id;

	DRM_DEBUG_KMS("%s:ipp_id[%d]\n", __func__, ipp_id);

	if (ipp_id) {
		/* find ipp driver */
		ippdrv = ipp_find_id(&ctx->ipp_idr, ippdrv->ipp_id);
		if (!ippdrv) {
			DRM_ERROR("not found ipp%d driver.\n", ipp_id);
			return NULL;
		}

		/* check idle state and dedicated state */
		if (ippdrv->state == IPP_STATE_START &&
		    ippdrv->dedicated) {
			DRM_ERROR("used choose device.\n");
			return NULL;
		}

		/* check property */
		if (ippdrv->check_property &&
		    ippdrv->check_property(ippdrv->dev, property)) {
			DRM_ERROR("not support property.\n");
			return NULL;
		}

		return ippdrv;
	} else {
		/* get ipp driver entry */
		list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {
			/* check idle state and dedicated state */
			if (ippdrv->state == IPP_STATE_IDLE &&
			    ippdrv->dedicated)
				continue;

			/* check property */
			if (ippdrv->check_property &&
			    ippdrv->check_property(ippdrv->dev, property)) {
				DRM_DEBUG_KMS("not support property.\n");
				continue;
			}

			return ippdrv;
		}

		DRM_ERROR("not support ipp driver operations.\n");
	}

	return NULL;
}

int exynos_drm_ipp_set_property(struct drm_device *drm_dev, void *data,
					struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_ipp_private *priv = file_priv->ipp_priv;
	struct device *dev = priv->dev;
	struct ipp_context *ctx = get_ipp_context(dev);
	struct exynos_drm_ippdrv *ippdrv;
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_property *property = data;
	int ret, i;

	DRM_DEBUG_KMS("%s\n", __func__);

	if (!ctx) {
		DRM_ERROR("invalid context.\n");
		return -EINVAL;
	}

	if (!property) {
		DRM_ERROR("invalid property parameter.\n");
		return -EINVAL;
	}

	/* find ipp driver using ipp id */
	ippdrv = ipp_find_driver(ctx, property);
	if (!ippdrv) {
		DRM_ERROR("failed to get ipp driver.\n");
		return -EINVAL;
	}

	/* allocate command node */
	c_node = kzalloc(sizeof(*c_node), GFP_KERNEL);
	if (!c_node) {
		DRM_ERROR("failed to allocate map node.\n");
		return -ENOMEM;
	}

	/* create property id */
	ret = ipp_create_id(&ippdrv->prop_idr, c_node, &property->prop_id);
	if (ret) {
		DRM_ERROR("failed to create id.\n");
		goto err_clear;
	}

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, property->prop_id);

	/* stored property information and ippdrv in private data */
	c_node->property = *property;
	for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++)
		INIT_LIST_HEAD(&c_node->mem_list[i]);

	/* make dedicated state without m2m */
	if (property->cmd != IPP_CMD_M2M)
		ippdrv->dedicated = true;
	priv->ippdrv = ippdrv;

	list_add_tail(&c_node->list, &ippdrv->cmd_list);

	return 0;

err_clear:
	kfree(c_node);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_drm_ipp_set_property);

static struct drm_exynos_ipp_cmd_node
	*ipp_find_cmd_node(struct exynos_drm_ippdrv *ippdrv, u32 prop_id)
{
	struct drm_exynos_ipp_cmd_node *c_node;

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, prop_id);

	/* ToDo: same with find_cmd_node and find_id */

	/* find ipp driver */
	c_node = ipp_find_id(&ippdrv->prop_idr, prop_id);
	if (!c_node) {
		DRM_ERROR("not found property%d.\n", prop_id);
		return NULL;
	}

	return c_node;
}

static struct drm_exynos_ipp_mem_node
	*ipp_find_mem_node(struct drm_exynos_ipp_cmd_node *c_node,
	struct drm_exynos_ipp_buf *buf)
{
	struct drm_exynos_ipp_mem_node *m_node;
	int count = 0;

	DRM_DEBUG_KMS("%s:buf_id[%d]\n", __func__, buf->buf_id);

	/* find memory node entry */
	list_for_each_entry(m_node, &c_node->mem_list[buf->ops_id], list) {
		DRM_DEBUG_KMS("%s:count[%d]c_node[0x%x]\n",
			__func__, count++, (int)c_node);

		/* compare buffer id */
		if (m_node->buf_id == buf->buf_id)
			return m_node;
	}

	return NULL;
}

static struct drm_exynos_ipp_property
	*ipp_find_property(struct exynos_drm_ippdrv *ippdrv, u32 prop_id)
{
	struct drm_exynos_ipp_property *property;
	struct drm_exynos_ipp_cmd_node *c_node;
	int count = 0;

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, prop_id);

	/* find command node entry */
	list_for_each_entry(c_node, &ippdrv->cmd_list, list) {
		DRM_DEBUG_KMS("%s:count[%d]c_node[0x%x]\n",
			__func__, count++, (int)c_node);

		property = &c_node->property;
		/* compare property id */
		if (property->prop_id == prop_id)
			return property;
	}

	return NULL;
}

static int ipp_set_property(struct exynos_drm_ippdrv *ippdrv,
	struct drm_exynos_ipp_property *property)
{
	struct exynos_drm_ipp_ops *ops = NULL;
	int ret, i, swap = 0;

	if (!property) {
		DRM_ERROR("invalid property parameter.\n");
		return -EINVAL;
	}

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, property->prop_id);

	/* reset h/w block */
	if (ippdrv->reset &&
		ippdrv->reset(ippdrv->dev)) {
		DRM_ERROR("failed to reset.\n");
		return -EINVAL;
	}

	/* set source,destination operations */
	for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
		/* ToDo: integrate property and config */
		struct drm_exynos_ipp_config *config =
			&property->config[i];

		ops = ippdrv->ops[i];
		if (!ops || !config) {
			DRM_ERROR("not support ops and config.\n");
			return -EINVAL;
		}

		/* set format */
		if (ops->set_fmt) {
			ret = ops->set_fmt(ippdrv->dev, config->fmt);
			if (ret) {
				DRM_ERROR("not support format.\n");
				return ret;
			}
		}

		/* set transform for rotation, flip */
		if (ops->set_transf) {
			swap = ops->set_transf(ippdrv->dev, config->degree,
				config->flip);
			if (swap < 0) {
				DRM_ERROR("not support tranf.\n");
				return -EINVAL;
			}
		}

		/* set size */
		if (ops->set_size) {
			ret = ops->set_size(ippdrv->dev, swap, &config->pos,
				&config->sz);
			if (ret) {
				DRM_ERROR("not support size.\n");
				return ret;
			}
		}
	}

	return 0;
}

static int ipp_set_mem_node(struct exynos_drm_ippdrv *ippdrv,
	struct drm_exynos_ipp_mem_node *node)
{
	struct exynos_drm_ipp_ops *ops = NULL;
	int ret;

	DRM_DEBUG_KMS("%s:node[0x%x]\n", __func__, (int)node);

	if (!node) {
		DRM_ERROR("invalid queue node.\n");
		ret = -EFAULT;
		return ret;
	}

	DRM_DEBUG_KMS("%s:ops_id[%d]\n", __func__, node->ops_id);

	/* get operations callback */
	ops = ippdrv->ops[node->ops_id];
	if (!ops) {
		DRM_DEBUG_KMS("not support ops.\n");
		ret = -EIO;
		return ret;
	}

	/* set address and enable irq */
	if (ops->set_addr) {
		ret = ops->set_addr(ippdrv->dev, &node->buf_info,
			node->buf_id, IPP_BUF_CTRL_QUEUE);
		if (ret) {
			if (ret != -ENOMEM)
				DRM_ERROR("failed to set addr.\n");
			return ret;
		}
	}

	return 0;
}

static int ipp_free_mem_node(struct drm_device *drm_dev,
	struct exynos_drm_ippdrv *ippdrv,
	struct drm_exynos_ipp_mem_node *node)
{
	int ret, i;

	DRM_DEBUG_KMS("%s:node[0x%x]\n", __func__, (int)node);

	if (!node) {
		DRM_ERROR("invalid queue node.\n");
		ret = -EFAULT;
		return ret;
	}

	DRM_DEBUG_KMS("%s:ops_id[%d]\n", __func__, node->ops_id);

	/* put gem buffer */
	for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++) {
		void *gem_obj = node->buf_info.gem_objs[i];

		if (gem_obj)
			exynos_drm_gem_put_dma_addr(drm_dev, gem_obj);
	}

	/* delete list in queue */
	list_del(&node->list);
	kfree(node);

	return 0;
}

/* ToDo: Merge with stop property */
static void ipp_free_cmd_list(struct drm_device *drm_dev,
	struct exynos_drm_ippdrv *ippdrv)
{
	struct drm_exynos_ipp_cmd_node *c_node, *tc_node;
	struct drm_exynos_ipp_mem_node *m_node, *tm_node;
	struct list_head *head;
	int ret, i, count = 0;

	/* get command node entry */
	list_for_each_entry_safe(c_node, tc_node,
		&ippdrv->cmd_list, list) {
		DRM_DEBUG_KMS("%s:count[%d]c_node[0x%x]\n",
			__func__, count++, (int)c_node);

		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			/* source/destination memory list */
			head = &c_node->mem_list[i];

			/* get memory node entry */
			list_for_each_entry_safe(m_node, tm_node, head, list) {
				/* free memory node to ippdrv */
				ret = ipp_free_mem_node(drm_dev, ippdrv,
					m_node);
				if (ret)
					DRM_ERROR("failed to free m node.\n");
			}
		}

		/* delete list */
		list_del(&c_node->list);
		kfree(c_node);
	}

	return;
}

static int ipp_start_property(struct drm_device *drm_dev,
	struct exynos_drm_ippdrv *ippdrv, u32 prop_id)
{
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_mem_node *m_node, tm_node;
	struct drm_exynos_ipp_property *property;
	struct list_head *head;
	int ret, i;

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, prop_id);

	/* find command node */
	c_node = ipp_find_cmd_node(ippdrv, prop_id);
	if (!c_node) {
		DRM_ERROR("invalid command node list.\n");
		return -EINVAL;
	}

	/* get property */
	property = &c_node->property;
	if (property->prop_id != prop_id) {
		DRM_ERROR("invalid property id.\n");
		return -EINVAL;
	}

	/* set current property in ippdrv */
	ippdrv->property = property;
	ret = ipp_set_property(ippdrv, property);
	if (ret) {
		DRM_ERROR("failed to set property.\n");
		ippdrv->property = NULL;
		return ret;
	}

	/* check command type */
	switch (property->cmd) {
	case IPP_CMD_M2M:
		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			/* source/destination memory list */
			head = &c_node->mem_list[i];

			if (list_empty(head)) {
				DRM_DEBUG_KMS("bypass empty list.\n");
				return 0;
			}

			/* get first entry */
			m_node = list_first_entry(head,
				struct drm_exynos_ipp_mem_node, list);
			if (!m_node) {
				DRM_DEBUG_KMS("failed to get node.\n");
				ret = -EFAULT;
				return ret;
			}

			DRM_DEBUG_KMS("%s:m_node[0x%x]\n",
				__func__, (int)m_node);

			/* must be set 0 src buffer id in m2m */
			if (i == EXYNOS_DRM_OPS_SRC) {
				tm_node = *m_node;
				tm_node.buf_id = 0;
				m_node = &tm_node;
			}

			/* set memory node to ippdrv */
			ret = ipp_set_mem_node(ippdrv, m_node);
			if (ret) {
				DRM_ERROR("failed to set m node.\n");
				return ret;
			}
		}
		break;
	case IPP_CMD_WB:
		/* destination memory list */
		head = &c_node->mem_list[EXYNOS_DRM_OPS_DST];

		/* get list entry */
		list_for_each_entry(m_node, head, list) {
			/* set memory node to ippdrv */
			ret = ipp_set_mem_node(ippdrv, m_node);
			if (ret) {
				DRM_ERROR("failed to set m node.\n");
				return ret;
			}
		}
		break;
	case IPP_CMD_OUTPUT:
		/* source memory list */
		head = &c_node->mem_list[EXYNOS_DRM_OPS_SRC];

		/* get list entry */
		list_for_each_entry(m_node, head, list) {
			/* set memory node to ippdrv */
			ret = ipp_set_mem_node(ippdrv, m_node);
			if (ret) {
				DRM_ERROR("failed to set m node.\n");
				return ret;
			}
		}
		break;
	default:
		DRM_ERROR("invalid operations.\n");
		ret = -EINVAL;
		return ret;
	}

	/* start operations */
	if (ippdrv->start) {
		ret = ippdrv->start(ippdrv->dev, property->cmd);
		if (ret) {
			DRM_ERROR("failed to start ops.\n");
			return ret;
		}
	}

	return 0;
}

static int ipp_stop_property(struct drm_device *drm_dev,
	struct exynos_drm_ippdrv *ippdrv, u32 prop_id)
{
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_mem_node *m_node, *tm_node;
	struct drm_exynos_ipp_property *property;
	enum drm_exynos_ipp_cmd	cmd;
	struct list_head *head;
	int ret, i;

	DRM_DEBUG_KMS("%s:prop_id[%d]\n", __func__, prop_id);

	/* find command node */
	c_node = ipp_find_cmd_node(ippdrv, prop_id);
	if (!c_node) {
		DRM_ERROR("invalid command node list.\n");
		return -EINVAL;
	}

	/* get property */
	property = &c_node->property;
	if (property->prop_id != prop_id) {
		DRM_ERROR("invalid property id.\n");
		return -EINVAL;
	}

	/* copy current command for memory list */
	cmd = property->cmd;

	/* stop operations */
	if (ippdrv->stop)
		ippdrv->stop(ippdrv->dev, property->cmd);

	/* check command type */
	switch (property->cmd) {
	case IPP_CMD_M2M:
		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			/* source/destination memory list */
			head = &c_node->mem_list[i];

			/* get list entry */
			list_for_each_entry_safe(m_node, tm_node,
				head, list) {
				/* free memory node to ippdrv */
				ret = ipp_free_mem_node(drm_dev, ippdrv,
					m_node);
				if (ret) {
					DRM_ERROR("failed to free m node.\n");
					return ret;
				}
			}
		}
		break;
	case IPP_CMD_WB:
		/* destination memory list */
		head = &c_node->mem_list[EXYNOS_DRM_OPS_DST];

		/* get list entry */
		list_for_each_entry_safe(m_node, tm_node, head, list) {
			/* free memory node to ippdrv */
			ret = ipp_free_mem_node(drm_dev, ippdrv, m_node);
			if (ret) {
				DRM_ERROR("failed to free m node.\n");
				return ret;
			}
		}
		break;
	case IPP_CMD_OUTPUT:
		/* source memory list */
		head = &c_node->mem_list[EXYNOS_DRM_OPS_SRC];

		/* get list entry */
		list_for_each_entry_safe(m_node, tm_node, head, list) {
			/* free memory node to ippdrv */
			ret = ipp_free_mem_node(drm_dev, ippdrv, m_node);
			if (ret) {
				DRM_ERROR("failed to free m node.\n");
				return ret;
			}
		}
		break;
	default:
		DRM_ERROR("invalid operations.\n");
		ret = -EINVAL;
		return ret;
	}

	/* delete list */
	list_del(&c_node->list);
	kfree(c_node);

	return 0;
}

static void ipp_free_event(struct drm_pending_event *event)
{
	kfree(event);
}

static int ipp_make_event(struct drm_device *drm_dev, struct drm_file *file,
	struct exynos_drm_ippdrv *ippdrv, struct drm_exynos_ipp_buf *buf)
{
	struct drm_exynos_ipp_send_event *e;
	unsigned long flags;

	DRM_DEBUG_KMS("%s:ops_id[%d]buf_id[%d]\n", __func__,
		buf->ops_id, buf->buf_id);

	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e) {
		DRM_ERROR("failed to allocate event.\n");

		spin_lock_irqsave(&drm_dev->event_lock, flags);
		file->event_space += sizeof(e->event);
		spin_unlock_irqrestore(&drm_dev->event_lock, flags);
		return -ENOMEM;
	}

	DRM_DEBUG_KMS("%s:e[0x%x]\n", __func__, (int)e);

	/* make event */
	e->event.base.type = DRM_EXYNOS_IPP_EVENT;
	e->event.base.length = sizeof(e->event);
	e->event.user_data = buf->user_data;
	e->event.buf_id[EXYNOS_DRM_OPS_DST] = buf->buf_id;
	e->base.event = &e->event.base;
	e->base.file_priv = file;
	e->base.destroy = ipp_free_event;

	list_add_tail(&e->base.link, &ippdrv->event_list);

	return 0;
}

static struct drm_exynos_ipp_mem_node
	*ipp_make_mem_node(struct drm_device *drm_dev,
	struct drm_file *file,
	struct exynos_drm_ippdrv *ippdrv,
	struct drm_exynos_ipp_buf *buf)
{
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_mem_node *m_node;
	struct drm_exynos_ipp_buf_info buf_info;
	void *addr;
	unsigned long size;
	int i;

	m_node = kzalloc(sizeof(*m_node), GFP_KERNEL);
	if (!m_node) {
		DRM_ERROR("failed to allocate queue node.\n");
		return NULL;
	}

	/* clear base address for error handling */
	memset(&buf_info, 0x0, sizeof(buf_info));

	/* find command node */
	c_node = ipp_find_cmd_node(ippdrv, buf->prop_id);
	if (!c_node) {
		DRM_ERROR("failed to get command node.\n");
		goto err_clear;
	}

	/* operations, buffer id */
	m_node->ops_id = buf->ops_id;
	m_node->prop_id = buf->prop_id;
	m_node->buf_id = buf->buf_id;

	DRM_DEBUG_KMS("%s:m_node[0x%x]ops_id[%d]\n", __func__,
		(int)m_node, buf->ops_id);
	DRM_DEBUG_KMS("%s:prop_id[%d]buf_id[%d]\n", __func__,
		buf->prop_id, m_node->buf_id);

	for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++) {
		unsigned int gem_obj;

		DRM_DEBUG_KMS("%s:i[%d]handle[0x%x]\n", __func__,
			i, buf->handle[i]);

		/* get dma address by handle */
		if (buf->handle[i] != 0) {
			addr = exynos_drm_gem_get_dma_addr(drm_dev,
					buf->handle[i], file, &gem_obj);
			if (!addr) {
				DRM_ERROR("failed to get addr.\n");
				goto err_clear;
			}

			size = exynos_drm_gem_get_size(drm_dev,
						buf->handle[i], file);
			if (!size) {
				DRM_ERROR("failed to get size.\n");
				goto err_clear;
			}

			buf_info.gem_objs[i] = (void *)gem_obj;
			buf_info.base[i] = *(dma_addr_t *) addr;
			buf_info.size[i] = (uint64_t) size;
		}
	}

	m_node->buf_info = buf_info;
	list_add_tail(&m_node->list, &c_node->mem_list[buf->ops_id]);

	return m_node;

err_clear:
	kfree(m_node);

	return NULL;
}

int exynos_drm_ipp_buf(struct drm_device *drm_dev, void *data,
					struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_ipp_private *priv = file_priv->ipp_priv;
	struct exynos_drm_ippdrv *ippdrv = priv->ippdrv;
	struct drm_exynos_ipp_buf *buf = data;
	struct exynos_drm_ipp_ops *ops = NULL;
	struct drm_exynos_ipp_send_event *e, *te;
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_mem_node *m_node = NULL, *tm_node;
	struct drm_exynos_ipp_property *property;
	struct drm_exynos_ipp_buf_info buf_info;
	struct list_head *head;
	int ret, i;

	DRM_DEBUG_KMS("%s\n", __func__);

	if (!buf) {
		DRM_ERROR("invalid buf parameter.\n");
		return -EINVAL;
	}

	if (!ippdrv) {
		DRM_ERROR("failed to get ipp driver.\n");
		return -EINVAL;
	}

	if (buf->ops_id >= EXYNOS_DRM_OPS_MAX) {
		DRM_ERROR("invalid ops parameter.\n");
		return -EINVAL;
	}

	ops = ippdrv->ops[buf->ops_id];
	if (!ops) {
		DRM_ERROR("failed to get ops.\n");
		return -EINVAL;
	}

	DRM_DEBUG_KMS("%s:ops_id[%s]buf_id[%d]buf_ctrl[%d]\n",
		__func__, buf->ops_id ? "dst" : "src",
		buf->buf_id, buf->buf_ctrl);

	/* clear base address for error handling */
	memset(&buf_info, 0x0, sizeof(buf_info));

	/* find command node */
	c_node = ipp_find_cmd_node(ippdrv, buf->prop_id);
	if (!c_node) {
		DRM_ERROR("failed to get command node.\n");
		ret = -EINVAL;
		goto err_clear;
	}

	/* get property */
	property = &c_node->property;
	if (!property) {
		DRM_ERROR("invalid property parameter.\n");
		goto err_clear;
	}

	/* buffer control */
	switch (buf->buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		/* make memory node */
		m_node = ipp_make_mem_node(drm_dev, file, ippdrv, buf);
		if (!m_node) {
			DRM_ERROR("failed to make queue node.\n");
			ret = -EINVAL;
			goto err_clear;
		}

		buf_info = m_node->buf_info;

		if (pm_runtime_suspended(ippdrv->dev))
			break;

		/* set address */
		if (property->cmd != IPP_CMD_M2M && ops->set_addr) {
			ret = ops->set_addr(ippdrv->dev, &buf_info, buf->buf_id,
				buf->buf_ctrl);
			if (ret) {
				DRM_ERROR("failed to set addr.\n");
				goto err_clear;
			}
		}
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		/* free node */
		list_for_each_entry_safe(m_node, tm_node,
		    &c_node->mem_list[buf->ops_id], list) {
			if (m_node->buf_id == buf->buf_id &&
				m_node->ops_id == buf->ops_id) {
				/* free memory node to ippdrv */
				ret = ipp_free_mem_node(drm_dev, ippdrv,
					m_node);
				if (ret) {
					DRM_ERROR("failed to free m node.\n");
					goto err_clear;
				}
			}
		}

		if (pm_runtime_suspended(ippdrv->dev)) {
			DRM_ERROR("suspended:invalid operations.\n");
			ret = -EINVAL;
			goto err_clear;
		}

		/* clear address */
		if (ops->set_addr) {
			ret = ops->set_addr(ippdrv->dev, &buf_info, buf->buf_id,
				buf->buf_ctrl);
			if (ret) {
				DRM_ERROR("failed to set addr.\n");
				goto err_clear;
			}
		}
		break;
	default:
		DRM_ERROR("invalid buffer control.\n");
		return -EINVAL;
	}

	/* destination buffer need event control */
	if (buf->ops_id == EXYNOS_DRM_OPS_DST) {
		switch (buf->buf_ctrl) {
		case IPP_BUF_CTRL_QUEUE:
			/* make event */
			ret = ipp_make_event(drm_dev, file, ippdrv, buf);
			if (ret) {
				DRM_ERROR("failed to make event.\n");
				goto err_clear;
			}
			break;
		case IPP_BUF_CTRL_DEQUEUE:
			/* free event */
			list_for_each_entry_safe(e, te,
				&ippdrv->event_list, base.link) {
				if (e->event.buf_id[EXYNOS_DRM_OPS_DST] ==
				    buf->buf_id) {
					/* delete list */
					list_del(&e->base.link);
					kfree(e);
				}
			}
			break;
		default:
			/* no action */
			break;
		}
	}

	/*
	 * If set source, destination buffer and enable pm
	 * m2m operations need start operations in queue
	 */
	if (property->cmd == IPP_CMD_M2M) {
		/* start operations was not set */
		if (pm_runtime_suspended(ippdrv->dev)) {
			DRM_DEBUG_KMS("suspended state.\n");
			return 0;
		}

		/* check source/destination buffer status */
		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			/* source/destination memory list */
			head = &c_node->mem_list[i];

			/* check list empty */
			if (list_empty(head)) {
				DRM_DEBUG_KMS("list empty.\n");
				return 0;
			}
		}

		/* check property id and buffer property id */
		if (property->prop_id != buf->prop_id) {
			DRM_ERROR("invalid property id.\n");
			goto err_clear;
		}

		/* start property */
		ret = ipp_start_property(drm_dev, ippdrv, property->prop_id);
		if (ret) {
			DRM_ERROR("failed to start property.\n");
			goto err_clear;
		}
	}

	return 0;

err_clear:
	DRM_ERROR("%s:failed to set buf.\n", __func__);

	/* delete list */
	list_for_each_entry_safe(m_node, tm_node,
		&c_node->mem_list[buf->ops_id], list) {
		if (m_node->buf_id == buf->buf_id &&
			m_node->ops_id == buf->ops_id) {
			list_del(&m_node->list);
			kfree(m_node);
		}
	}

	/* put gem buffer */
	for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++) {
		void *gem_obj = buf_info.gem_objs[i];

		if (gem_obj)
			exynos_drm_gem_put_dma_addr(drm_dev,
				gem_obj);
	}

	/* free address */
	switch (buf->buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
	case IPP_BUF_CTRL_DEQUEUE:
		if (pm_runtime_suspended(ippdrv->dev)) {
			DRM_ERROR("suspended:invalid error operations.\n");
			return -EINVAL;
		}

		/* clear base address for error handling */
		memset(&buf_info, 0x0, sizeof(buf_info));

		/* don't need check error case */
		if (ops->set_addr)
			ops->set_addr(ippdrv->dev, &buf_info,
				buf->buf_id, IPP_BUF_CTRL_DEQUEUE);
		break;
	default:
		/* no action */
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_drm_ipp_buf);

int exynos_drm_ipp_ctrl(struct drm_device *drm_dev, void *data,
					struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_ipp_private *priv = file_priv->ipp_priv;
	struct exynos_drm_ippdrv *ippdrv = priv->ippdrv;
	struct drm_exynos_ipp_ctrl *ctrl = data;
	int ret;

	DRM_DEBUG_KMS("%s\n", __func__);

	if (!ctrl) {
		DRM_ERROR("invalid control parameter.\n");
		return -EINVAL;
	}

	if (!ippdrv) {
		DRM_ERROR("failed to get ipp driver.\n");
		return -EINVAL;
	}

	DRM_DEBUG_KMS("%s:use[%d]\n", __func__, ctrl->use);

	/* ToDo: expand ctrl operation */

	/*
	 * start/stop operations,
	 * set use to 1, you can use start operations
	 * other case is stop opertions
	 */
	if (ctrl->use) {
		if (pm_runtime_suspended(ippdrv->dev))
			pm_runtime_get_sync(ippdrv->dev);

		ret = ipp_start_property(drm_dev, ippdrv, ctrl->prop_id);
		if (ret) {
			DRM_ERROR("failed to start property.\n");
			goto err_clear;
		}

		ippdrv->state = IPP_STATE_START;
	} else {
		ippdrv->state = IPP_STATE_STOP;
		ippdrv->dedicated = false;
		ippdrv->property = NULL;

		ret = ipp_stop_property(drm_dev, ippdrv, ctrl->prop_id);
		if (ret) {
			DRM_ERROR("failed to stop property.\n");
			goto err_clear;
		}

		if (!pm_runtime_suspended(ippdrv->dev))
			pm_runtime_put_sync(ippdrv->dev);
	}

	return 0;

err_clear:
	/*
	 * ToDo: register clear if needed
	 * If failed choose device using property. then
	 * revert register clearing if needed
	 */

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_drm_ipp_ctrl);

int exynos_drm_ippnb_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(
			&exynos_drm_ippnb_list, nb);
}
EXPORT_SYMBOL_GPL(exynos_drm_ippnb_register);

int exynos_drm_ippnb_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(
			&exynos_drm_ippnb_list, nb);
}
EXPORT_SYMBOL_GPL(exynos_drm_ippnb_unregister);

int exynos_drm_ippnb_send_event(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(
			&exynos_drm_ippnb_list, val, v);
}
EXPORT_SYMBOL_GPL(exynos_drm_ippnb_send_event);

void ipp_send_event_handler(struct exynos_drm_ippdrv *ippdrv,
	int buf_id)
{
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct drm_exynos_ipp_property *property = ippdrv->property;
	struct drm_exynos_ipp_cmd_node *c_node;
	struct drm_exynos_ipp_mem_node *m_node;
	struct drm_exynos_ipp_buf buf;
	struct drm_exynos_ipp_send_event *e;
	struct list_head *head;
	struct timeval now;
	unsigned long flags;
	u32 q_buf_id[EXYNOS_DRM_OPS_MAX] = {0, };
	int ret, i;

	DRM_DEBUG_KMS("%s:buf_id[%d]\n", __func__, buf_id);

	if (!drm_dev) {
		DRM_ERROR("failed to get drm_dev.\n");
		return;
	}

	if (list_empty(&ippdrv->event_list)) {
		DRM_ERROR("event list is empty.\n");
		return;
	}

	if (!property) {
		DRM_ERROR("failed to get property.\n");
		return;
	}

	/* find command node */
	c_node = ipp_find_cmd_node(ippdrv, property->prop_id);
	if (!c_node) {
		DRM_ERROR("invalid command node list.\n");
		return;
	}

	/* check command type */
	switch (property->cmd) {
	case IPP_CMD_M2M:
		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			/* source/destination memory list */
			head = &c_node->mem_list[i];

			if (list_empty(head)) {
				DRM_ERROR("empty list.\n");
				return;
			}

			/* get first entry */
			m_node = list_first_entry(head,
				struct drm_exynos_ipp_mem_node, list);
			if (!m_node) {
				DRM_ERROR("empty list.\n");
				return;
			}

			q_buf_id[i] = m_node->buf_id;

			/* free memory node to ippdrv */
			ret = ipp_free_mem_node(drm_dev, ippdrv, m_node);
			if (ret)
				DRM_ERROR("failed to free m node.\n");
		}
		break;
	case IPP_CMD_WB:
		/* clear buf for finding */
		memset(&buf, 0x0, sizeof(buf));
		buf.ops_id = EXYNOS_DRM_OPS_DST;
		buf.buf_id = buf_id;

		/* get memory node entry */
		m_node = ipp_find_mem_node(c_node, &buf);
		if (!m_node) {
			DRM_ERROR("empty list.\n");
			return;
		}

		q_buf_id[EXYNOS_DRM_OPS_DST] = m_node->buf_id;

		/* free memory node to ippdrv */
		ret = ipp_free_mem_node(drm_dev, ippdrv, m_node);
		if (ret)
			DRM_ERROR("failed to free m node.\n");
		break;
	case IPP_CMD_OUTPUT:
		/* source memory list */
		head = &c_node->mem_list[EXYNOS_DRM_OPS_SRC];

		/* get first entry */
		m_node = list_first_entry(head,
			struct drm_exynos_ipp_mem_node, list);
		if (!m_node) {
			DRM_ERROR("empty list.\n");
			return;
		}

		q_buf_id[EXYNOS_DRM_OPS_SRC] = m_node->buf_id;

		/* free memory node to ippdrv */
		ret = ipp_free_mem_node(drm_dev, ippdrv, m_node);
		if (ret)
			DRM_ERROR("failed to free m node.\n");
		break;
	default:
		DRM_ERROR("invalid operations.\n");
		return;
	}

	/* ToDo: Fix buffer id */
	if (q_buf_id[EXYNOS_DRM_OPS_DST] != buf_id)
		DRM_ERROR("failed to match buffer id %d, %d.\n",
			q_buf_id[EXYNOS_DRM_OPS_DST], buf_id);

	/* get first event entry */
	e = list_first_entry(&ippdrv->event_list,
		struct drm_exynos_ipp_send_event, base.link);

	do_gettimeofday(&now);
	e->event.tv_sec = now.tv_sec;
	e->event.tv_usec = now.tv_usec;
	e->event.prop_id = property->prop_id;

	/* set buffer id about source destination */
	for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
		/* ToDo: compare index. If needed */
		e->event.buf_id[i] = q_buf_id[i];
	}

	spin_lock_irqsave(&drm_dev->event_lock, flags);
	list_move_tail(&e->base.link, &e->base.file_priv->event_list);
	wake_up_interruptible(&e->base.file_priv->event_wait);
	spin_unlock_irqrestore(&drm_dev->event_lock, flags);

	/* ToDo: Need to handle the property queue */

	switch (property->cmd) {
	case IPP_CMD_M2M:
		for (i = 0; i < EXYNOS_DRM_OPS_MAX; i++) {
			head = &c_node->mem_list[i];
			if (list_empty(head))
				return;
		}

		ret = ipp_start_property(drm_dev, ippdrv, property->prop_id);
		if (ret) {
			DRM_ERROR("failed to start property.\n");
			return;
		}
		break;
	case IPP_CMD_WB:
	case IPP_CMD_OUTPUT:
	default:
		break;
	}

	DRM_DEBUG_KMS("%s:finish cmd[%d]\n", __func__, property->cmd);
}

static void ipp_sched_event(struct work_struct *sched_event)
{
	struct ipp_context *ctx = container_of(sched_event,
		struct ipp_context, sched_event);

	DRM_DEBUG_KMS("%s\n", __func__);
	/* ToDo:send event handler */
}

static void ipp_sched_cmd(struct work_struct *sched_cmd)
{
	struct ipp_context *ctx = container_of(sched_cmd,
		struct ipp_context, sched_cmd);

	DRM_DEBUG_KMS("%s\n", __func__);
	/* ToDo: schedule next work */
}

static int ipp_subdrv_probe(struct drm_device *drm_dev, struct device *dev)
{
	struct exynos_drm_ippdrv *ippdrv;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;
	int ret;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* get ipp driver entry */
	list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {
		ippdrv->drm_dev = drm_dev;

		/* ToDo: need move open ? */

		/* init prop idr */
		idr_init(&ippdrv->prop_idr);

		/* check iommu use case */
		if (ippdrv->iommu_used) {
			ret = exynos_drm_iommu_activate(drm_priv->vmm,
								ippdrv->dev);
			if (ret) {
				DRM_ERROR("failed to activate iommu\n");
				goto err_clear;
			}
		}
	}

	return 0;

err_clear:
	/* get ipp driver entry */
	list_for_each_entry_reverse(ippdrv, &exynos_drm_ippdrv_list, list)
		if ((ippdrv->iommu_used) && (drm_priv->vmm))
			exynos_drm_iommu_deactivate(drm_priv->vmm, ippdrv->dev);

	return ret;
}

static void ipp_subdrv_remove(struct drm_device *drm_dev, struct device *dev)
{
	struct exynos_drm_ippdrv *ippdrv;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* get ipp driver entry */
	list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {

		/* ToDo: need move close ? */

		/* remove,destroy property idr */
		idr_remove_all(&ippdrv->prop_idr);
		idr_destroy(&ippdrv->prop_idr);

		if (drm_priv->vmm)
			exynos_drm_iommu_deactivate(drm_priv->vmm, ippdrv->dev);

		ippdrv->drm_dev = NULL;
		exynos_drm_ippdrv_unregister(ippdrv);
	}

	/* ToDo: free notifier callback list if needed */
}

static int ipp_subdrv_open(struct drm_device *drm_dev, struct device *dev,
							struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_ipp_private *priv;
	struct exynos_drm_ippdrv *ippdrv;
	int count = 0;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* ToDo: multi device open */

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		DRM_ERROR("failed to allocate priv.\n");
		return -ENOMEM;
	}

	priv->dev = dev;
	file_priv->ipp_priv = priv;
	INIT_LIST_HEAD(&priv->event_list);

	/* get ipp driver entry */
	list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {
		DRM_DEBUG_KMS("%s:count[%d]ippdrv[0x%x]\n", __func__,
			count++, (int)ippdrv);

		/* check idle state */
		if (ippdrv->state != IPP_STATE_IDLE)
			continue;

		INIT_LIST_HEAD(&ippdrv->event_list);
		INIT_LIST_HEAD(&ippdrv->cmd_list);
		list_splice_init(&priv->event_list, &ippdrv->event_list);
	}

	return 0;
}

static void ipp_subdrv_close(struct drm_device *drm_dev, struct device *dev,
							struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_ipp_private *priv = file_priv->ipp_priv;
	struct exynos_drm_ippdrv *ippdrv_cur = priv->ippdrv;
	struct exynos_drm_ippdrv *ippdrv;
	struct drm_exynos_ipp_send_event *e, *te;
	int count;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* ToDo: for multi device close */

	/* get ipp driver entry */
	list_for_each_entry(ippdrv, &exynos_drm_ippdrv_list, list) {
		DRM_DEBUG_KMS("%s:ippdrv_cur[0x%x]ippdrv[0x%x]\n",
			__func__, (int)ippdrv_cur, (int)ippdrv);

		DRM_DEBUG_KMS("%s:state[%d]dedicated[%d]\n", __func__,
			ippdrv->state, ippdrv->dedicated);

		/* current used ippdrv stop needed */
		if (ippdrv_cur && ippdrv_cur == ippdrv) {
			if (ippdrv->state == IPP_STATE_START) {
				if (ippdrv->stop)
					ippdrv->stop(ippdrv->dev, ippdrv->cmd);

				if (!pm_runtime_suspended(ippdrv->dev))
					pm_runtime_put_sync(ippdrv->dev);
			}

			ippdrv->state = IPP_STATE_IDLE;
			ippdrv->dedicated = false;
		}

		/* check idle state */
		if (ippdrv->state != IPP_STATE_IDLE)
			continue;

		/* free event */
		count = 0;
		list_for_each_entry_safe(e, te,
			&ippdrv->event_list, base.link) {
			DRM_DEBUG_KMS("%s:count[%d]e[0x%x]\n",
				__func__, count++, (int)e);

			/* delete list */
			list_del(&e->base.link);
			kfree(e);
		}

		/* free property list */
		ipp_free_cmd_list(drm_dev, ippdrv);
		/* ToDo: How can get current fd property ? */
	}

	kfree(file_priv->ipp_priv);

	return;
}

static int __devinit ipp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ipp_context *ctx;
	struct exynos_drm_subdrv *subdrv;
	struct exynos_drm_ippdrv *tippdrv;
	int ret = -EINVAL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* init ioctl lock */
	mutex_init(&ctx->lock);
	/* init event, cmd work thread */
	INIT_WORK(&ctx->sched_event, ipp_sched_event);
	INIT_WORK(&ctx->sched_cmd, ipp_sched_cmd);
	/* init ipp driver idr */
	idr_init(&ctx->ipp_idr);

	/* get ipp driver entry */
	list_for_each_entry(tippdrv, &exynos_drm_ippdrv_list, list) {
		/* create ipp id */
		ret = ipp_create_id(&ctx->ipp_idr, tippdrv, &tippdrv->ipp_id);
		if (ret) {
			DRM_ERROR("failed to create id.\n");
			goto err_clear;
		}

		DRM_DEBUG_KMS("%s:ipp_id[%d]\n", __func__, tippdrv->ipp_id);

		if (tippdrv->ipp_id == 0)
			DRM_ERROR("failed to get ipp_id[%d]\n",
				tippdrv->ipp_id);
	}

	/* set sub driver informations */
	subdrv = &ctx->subdrv;
	subdrv->dev = dev;
	subdrv->probe = ipp_subdrv_probe;
	subdrv->remove = ipp_subdrv_remove;
	subdrv->open = ipp_subdrv_open;
	subdrv->close = ipp_subdrv_close;

	/* set driver data */
	platform_set_drvdata(pdev, ctx);

	/* register sub driver */
	ret = exynos_drm_subdrv_register(subdrv);
	if (ret < 0) {
		DRM_ERROR("failed to register drm ipp device.\n");
		goto err_clear;
	}

	dev_info(&pdev->dev, "drm ipp registered successfully.\n");

	return 0;

err_clear:
	kfree(ctx);

	return ret;
}

static int __devexit ipp_remove(struct platform_device *pdev)
{
	struct ipp_context *ctx = platform_get_drvdata(pdev);

	DRM_DEBUG_KMS("%s\n", __func__);

	/* unregister sub driver */
	exynos_drm_subdrv_unregister(&ctx->subdrv);

	/* remove,destroy ipp idr */
	idr_remove_all(&ctx->ipp_idr);
	idr_destroy(&ctx->ipp_idr);

	kfree(ctx);

	return 0;
}


struct platform_driver ipp_driver = {
	.probe		= ipp_probe,
	.remove		= __devexit_p(ipp_remove),
	.driver		= {
		.name	= "exynos-drm-ipp",
		.owner	= THIS_MODULE,
	},
};

