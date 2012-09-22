/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "drm.h"
#include "drm_crtc_helper.h"

#include <drm/exynos_drm.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_crtc.h"
#include "exynos_drm_encoder.h"
#include "exynos_drm_fbdev.h"
#include "exynos_drm_fb.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_g2d.h"
#include "exynos_drm_ipp.h"
#include "exynos_drm_plane.h"
#include "exynos_drm_vidi.h"
#include "exynos_drm_dmabuf.h"
#include "exynos_drm_iommu.h"

#define DRIVER_NAME	"exynos"
#define DRIVER_DESC	"Samsung SoC DRM"
#define DRIVER_DATE	"20110530"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

#define VBLANK_OFF_DELAY	50000

struct exynos_drm_gem_info_data {
	struct drm_file *filp;
	struct seq_file *m;
};

static int exynos_drm_gem_one_info(int id, void *ptr, void *data)
{
	struct drm_gem_object *obj = ptr;
	struct exynos_drm_gem_info_data *gem_info_data = data;
	struct drm_exynos_file_private *file_priv =
					gem_info_data->filp->driver_priv;
	struct exynos_drm_gem_obj *exynos_gem = to_exynos_gem_obj(obj);
	struct exynos_drm_gem_buf *buf = exynos_gem->buffer;

	seq_printf(gem_info_data->m, "%3d \t%3d \t%3d \t%2d \t\t%2d \t0x%08lx"\
				" \t0x%x \t0x%08lx \t%2d \t\t%2d \t\t%2d\n",
				gem_info_data->filp->pid,
				file_priv->tgid,
				id,
				atomic_read(&obj->refcount.refcount),
				atomic_read(&obj->handle_count),
				exynos_gem->size,
				exynos_gem->flags,
				buf->page_size,
				buf->pfnmap,
				obj->export_dma_buf ? 1 : 0,
				obj->import_attach ? 1 : 0);

	return 0;
}

static int exynos_drm_gem_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *drm_dev = node->minor->dev;
	struct exynos_drm_gem_info_data gem_info_data;

	gem_info_data.m = m;

	seq_printf(gem_info_data.m, "pid \ttgid \thandle \trefcount \thcount "\
				"\tsize \t\tflags \tpage_size \tpfnmap \t"\
				"exyport_to_fd \timport_from_fd\n");

	list_for_each_entry(gem_info_data.filp, &drm_dev->filelist, lhead)
		idr_for_each(&gem_info_data.filp->object_idr,
				exynos_drm_gem_one_info, &gem_info_data);

	return 0;
}

static struct drm_info_list exynos_drm_debugfs_list[] = {
	{"gem_info", exynos_drm_gem_info, DRIVER_GEM},
};
#define EXYNOS_DRM_DEBUGFS_ENTRIES ARRAY_SIZE(exynos_drm_debugfs_list)

static int exynos_drm_load(struct drm_device *dev, unsigned long flags)
{
	struct exynos_drm_private *private;
	struct drm_minor *minor;
	int ret;
	int nr;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	private = kzalloc(sizeof(struct exynos_drm_private), GFP_KERNEL);
	if (!private) {
		DRM_ERROR("failed to allocate private\n");
		return -ENOMEM;
	}

	/* maximum size of userptr is limited to 16MB as default. */
	private->userptr_limit = SZ_16M;

	/* setup device address space for iommu. */
	private->vmm = exynos_drm_iommu_setup(0x80000000, 0x40000000);
	if (IS_ERR(private->vmm)) {
		DRM_ERROR("failed to setup iommu.\n");
		kfree(private);
		return PTR_ERR(private->vmm);
	}

	INIT_LIST_HEAD(&private->pageflip_event_list);
	dev->dev_private = (void *)private;

	drm_mode_config_init(dev);

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(dev);

	exynos_drm_mode_config_init(dev);

	/*
	 * EXYNOS4 is enough to have two CRTCs and each crtc would be used
	 * without dependency of hardware.
	 */
	for (nr = 0; nr < MAX_CRTC; nr++) {
		ret = exynos_drm_crtc_create(dev, nr);
		if (ret)
			goto err_crtc;
	}

	for (nr = 0; nr < MAX_PLANE; nr++) {
		struct drm_plane *plane;
		unsigned int possible_crtcs = (1 << MAX_CRTC) - 1;

		plane = exynos_plane_init(dev, possible_crtcs, false);
		if (!plane)
			goto err_crtc;
	}

	ret = drm_vblank_init(dev, MAX_CRTC);
	if (ret)
		goto err_crtc;

	/*
	 * probe sub drivers such as display controller and hdmi driver,
	 * that were registered at probe() of platform driver
	 * to the sub driver and create encoder and connector for them.
	 */
	ret = exynos_drm_device_register(dev);
	if (ret)
		goto err_vblank;

	/* setup possible_clones. */
	exynos_drm_encoder_setup(dev);

	/*
	 * create and configure fb helper and also exynos specific
	 * fbdev object.
	 */
	ret = exynos_drm_fbdev_init(dev);
	if (ret) {
		DRM_ERROR("failed to initialize drm fbdev\n");
		goto err_drm_device;
	}

	drm_vblank_offdelay = VBLANK_OFF_DELAY;

	minor = dev->primary;
	ret = drm_debugfs_create_files(exynos_drm_debugfs_list,
						EXYNOS_DRM_DEBUGFS_ENTRIES,
						minor->debugfs_root, minor);
	if (ret)
		DRM_DEBUG_DRIVER("failed to create exynos-drm debugfs.\n");

	return 0;

err_drm_device:
	exynos_drm_device_unregister(dev);
err_vblank:
	drm_vblank_cleanup(dev);
err_crtc:
	drm_mode_config_cleanup(dev);
	kfree(private);

	return ret;
}

static int exynos_drm_unload(struct drm_device *dev)
{
	struct exynos_drm_private *private;

	private = dev->dev_private;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	/* release vmm object and device address space for iommu. */
	exynos_drm_iommu_cleanup(private->vmm);

	exynos_drm_fbdev_fini(dev);
	exynos_drm_device_unregister(dev);
	drm_vblank_cleanup(dev);
	drm_kms_helper_poll_fini(dev);
	drm_mode_config_cleanup(dev);
	kfree(dev->dev_private);

	dev->dev_private = NULL;

	drm_debugfs_remove_files(exynos_drm_debugfs_list,
				EXYNOS_DRM_DEBUGFS_ENTRIES, dev->primary);

	return 0;
}

static int exynos_drm_open(struct drm_device *dev, struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	file_priv = kzalloc(sizeof(*file_priv), GFP_KERNEL);
	if (!file_priv)
		return -ENOMEM;

	file_priv->tgid = task_tgid_nr(current);

	drm_prime_init_file_private(&file->prime);
	file->driver_priv = file_priv;

	return exynos_drm_subdrv_open(dev, file);
}

static void exynos_drm_preclose(struct drm_device *dev,
					struct drm_file *file)
{
	struct exynos_drm_private *private = dev->dev_private;
	struct drm_pending_vblank_event *e, *t;
	unsigned long flags;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	/* release events of current file */
	spin_lock_irqsave(&dev->event_lock, flags);
	list_for_each_entry_safe(e, t, &private->pageflip_event_list,
			base.link) {
		if (e->base.file_priv == file) {
			list_del(&e->base.link);
			e->base.destroy(&e->base);
		}
	}
	drm_prime_destroy_file_private(&file->prime);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	exynos_drm_subdrv_close(dev, file);
}

static void exynos_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	if (!file->driver_priv)
		return;

	kfree(file->driver_priv);
	file->driver_priv = NULL;
}

static void exynos_drm_lastclose(struct drm_device *dev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	exynos_drm_fbdev_restore_mode(dev);
}

static struct vm_operations_struct exynos_drm_gem_vm_ops = {
	.fault = exynos_drm_gem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static struct drm_ioctl_desc exynos_ioctls[] = {
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_CREATE, exynos_drm_gem_create_ioctl,
			DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_MAP_OFFSET,
			exynos_drm_gem_map_offset_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_MMAP,
			exynos_drm_gem_mmap_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_USERPTR,
			exynos_drm_gem_userptr_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_GET,
			exynos_drm_gem_get_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_USER_LIMIT,
			exynos_drm_gem_user_limit_ioctl, DRM_MASTER |
			DRM_ROOT_ONLY),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_EXPORT_UMP,
			exynos_drm_gem_export_ump_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_CACHE_OP,
			exynos_drm_gem_cache_op_ioctl, DRM_UNLOCKED),
	/* temporary ioctl commands. */
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_GET_PHY,
			exynos_drm_gem_get_phy_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_PHY_IMP,
			exynos_drm_gem_phy_imp_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_VIDI_CONNECTION,
			vidi_connection_ioctl, DRM_UNLOCKED | DRM_AUTH),

	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_GET_VER,
			exynos_g2d_get_ver_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_SET_CMDLIST,
			exynos_g2d_set_cmdlist_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_EXEC,
			exynos_g2d_exec_ioctl, DRM_UNLOCKED | DRM_AUTH),

	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_GET_PROPERTY,
			exynos_drm_ipp_get_property, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_SET_PROPERTY,
			exynos_drm_ipp_set_property, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_BUF,
			exynos_drm_ipp_buf, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_CTRL,
			exynos_drm_ipp_ctrl, DRM_UNLOCKED | DRM_AUTH),
};

static const struct file_operations exynos_drm_driver_fops = {
	.owner		= THIS_MODULE,
	.open		= drm_open,
	.mmap		= exynos_drm_gem_mmap,
	.poll		= drm_poll,
	.read		= drm_read,
	.unlocked_ioctl	= drm_ioctl,
	.release	= drm_release,
};

static struct drm_driver exynos_drm_driver = {
	.driver_features	= DRIVER_HAVE_IRQ | DRIVER_MODESET |
					DRIVER_GEM | DRIVER_PRIME,
	.load			= exynos_drm_load,
	.unload			= exynos_drm_unload,
	.open			= exynos_drm_open,
	.preclose		= exynos_drm_preclose,
	.lastclose		= exynos_drm_lastclose,
	.postclose		= exynos_drm_postclose,
	.get_vblank_counter	= drm_vblank_count,
	.enable_vblank		= exynos_drm_crtc_enable_vblank,
	.disable_vblank		= exynos_drm_crtc_disable_vblank,
	.gem_init_object	= exynos_drm_gem_init_object,
	.gem_free_object	= exynos_drm_gem_free_object,
	.gem_vm_ops		= &exynos_drm_gem_vm_ops,
	.gem_close_object	= &exynos_drm_gem_close_object,
	.dumb_create		= exynos_drm_gem_dumb_create,
	.dumb_map_offset	= exynos_drm_gem_dumb_map_offset,
	.dumb_destroy		= exynos_drm_gem_dumb_destroy,
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export	= exynos_dmabuf_prime_export,
	.gem_prime_import	= exynos_dmabuf_prime_import,
	.ioctls			= exynos_ioctls,
	.fops			= &exynos_drm_driver_fops,
	.name	= DRIVER_NAME,
	.desc	= DRIVER_DESC,
	.date	= DRIVER_DATE,
	.major	= DRIVER_MAJOR,
	.minor	= DRIVER_MINOR,
};

static int exynos_drm_platform_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	exynos_drm_driver.num_ioctls = DRM_ARRAY_SIZE(exynos_ioctls);

	return drm_platform_init(&exynos_drm_driver, pdev);
}

static int exynos_drm_platform_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	drm_platform_exit(&exynos_drm_driver, pdev);

	return 0;
}

static struct platform_driver exynos_drm_platform_driver = {
	.probe		= exynos_drm_platform_probe,
	.remove		= __devexit_p(exynos_drm_platform_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "exynos-drm",
	},
};

static int __init exynos_drm_init(void)
{
	int ret;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

#ifdef CONFIG_DRM_EXYNOS_FIMD
	ret = platform_driver_register(&fimd_driver);
	if (ret < 0)
		goto out_fimd;
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
	ret = platform_driver_register(&hdmi_driver);
	if (ret < 0)
		goto out_hdmi;
	ret = platform_driver_register(&mixer_driver);
	if (ret < 0)
		goto out_mixer;
	ret = platform_driver_register(&exynos_drm_common_hdmi_driver);
	if (ret < 0)
		goto out_common_hdmi;
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	ret = platform_driver_register(&vidi_driver);
	if (ret < 0)
		goto out_vidi;
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	ret = platform_driver_register(&g2d_driver);
	if (ret < 0)
		goto out_g2d;
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	ret = platform_driver_register(&rotator_driver);
	if (ret < 0)
		goto out_rotator;
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	ret = platform_driver_register(&fimc_driver);
	if (ret < 0)
		goto out_fimc;
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	ret = platform_driver_register(&gsc_driver);
	if (ret < 0)
		goto out_gsc;
#endif

#ifdef CONFIG_DRM_EXYNOS_IPP
	ret = platform_driver_register(&ipp_driver);
	if (ret < 0)
		goto out_ipp;
#endif

	ret = platform_driver_register(&exynos_drm_platform_driver);
	if (ret < 0)
		goto out;

	return 0;

out:
#ifdef CONFIG_DRM_EXYNOS_IPP
	platform_driver_unregister(&ipp_driver);
out_ipp:
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	platform_driver_unregister(&gsc_driver);
out_gsc:
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	platform_driver_unregister(&fimc_driver);
out_fimc:
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	platform_driver_unregister(&rotator_driver);
out_rotator:
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	platform_driver_unregister(&g2d_driver);
out_g2d:
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	platform_driver_unregister(&vidi_driver);
out_vidi:
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
	platform_driver_unregister(&exynos_drm_common_hdmi_driver);
out_common_hdmi:
	platform_driver_unregister(&mixer_driver);
out_mixer:
	platform_driver_unregister(&hdmi_driver);
out_hdmi:
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMD
	platform_driver_unregister(&fimd_driver);
out_fimd:
#endif
	return ret;
}

static void __exit exynos_drm_exit(void)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	platform_driver_unregister(&exynos_drm_platform_driver);

#ifdef CONFIG_DRM_EXYNOS_IPP
	platform_driver_unregister(&ipp_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	platform_driver_unregister(&gsc_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	platform_driver_unregister(&fimc_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	platform_driver_unregister(&rotator_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	platform_driver_unregister(&g2d_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	platform_driver_unregister(&vidi_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
	platform_driver_unregister(&exynos_drm_common_hdmi_driver);
	platform_driver_unregister(&mixer_driver);
	platform_driver_unregister(&hdmi_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMD
	platform_driver_unregister(&fimd_driver);
#endif
}

module_init(exynos_drm_init);
module_exit(exynos_drm_exit);

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_AUTHOR("Seung-Woo Kim <sw0312.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM Driver");
MODULE_LICENSE("GPL");
