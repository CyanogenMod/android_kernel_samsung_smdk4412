/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_dev.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Driver interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_DEV_H
#define __MFC_DEV_H __FILE__

#include <linux/mutex.h>
#include <linux/firmware.h>

#include "mfc_inst.h"

#define MFC_DEV_NAME	"s3c-mfc"
#define MFC_NAME_LEN	16

struct mfc_reg {
	resource_size_t	rsrc_start;
	resource_size_t	rsrc_len;
	void __iomem	*base;
};

struct mfc_pm {
	char		pd_name[MFC_NAME_LEN];
	char		clk_name[MFC_NAME_LEN];
	struct clk	*clock;
	atomic_t	power;
#ifdef CONFIG_PM_RUNTIME
	struct device	*device;
#endif
};

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
struct mfc_vcm {
	struct vcm	*sysmmu_vcm;
	unsigned long	*sysmmu_pgd;
};
#endif

struct mfc_mem {
	unsigned long	base;	/* phys. or virt. addr for MFC	*/
	size_t		size;	/* total size			*/
	unsigned char	*addr;	/* kernel virtual address space */
#if (defined(SYSMMU_MFC_ON) && !defined(CONFIG_VIDEO_MFC_VCM_UMP) && !defined(CONFIG_S5P_VMEM))
	void		*vmalloc_addr;	/* not aligned vmalloc alloc. addr */
#endif
#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	struct vcm_res	*vcm_s;
#endif
};

struct mfc_fw {
	const struct firmware	*info;
	int			requesting;
	int			state;
	int			ver;
#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
	struct vcm_mmu_res	*vcm_s;
	struct vcm_res		*vcm_k;
#elif defined(CONFIG_S5P_VMEM)
	int			vmem_cookie;
#endif
};

struct mfc_dev {
	char			name[MFC_NAME_LEN];
	struct mfc_reg		reg;
	int			irq;
	struct mfc_pm		pm;

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	struct mfc_vcm		vcm_info;
#endif
	int			mem_ports;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	struct mfc_mem		mem_infos[MFC_MAX_MEM_CHUNK_NUM];
	struct mfc_mem		drm_info;
#else
	struct mfc_mem		mem_infos[MFC_MAX_MEM_PORT_NUM];
#endif

	atomic_t		inst_cnt;
	struct mfc_inst_ctx	*inst_ctx[MFC_MAX_INSTANCE_NUM];

	struct mutex		lock;
	wait_queue_head_t	wait_sys;
	int			irq_sys;
	/* FIXME: remove or use 2 codec channel */
	wait_queue_head_t	wait_codec[2];
	int			irq_codec[2];

	struct mfc_fw		fw;

#if defined(CONFIG_DMA_CMA) && defined(CONFIG_USE_MFC_CMA)
	/* NEW CMA */
	void            *cma_vaddr;
	dma_addr_t      cma_dma_addr;
#endif

	struct s5p_vcm_mmu	*_vcm_mmu;

	struct device		*device;
#if defined(CONFIG_BUSFREQ_OPP) || defined(CONFIG_BUSFREQ_LOCK_WRAPPER)
	struct device           *bus_dev;
#endif
#if defined(CONFIG_BUSFREQ)
	atomic_t		busfreq_lock_cnt; /* Bus frequency Lock count */
#endif
#if defined(CONFIG_MACH_GC1) && defined(CONFIG_EXYNOS4_CPUFREQ)
	atomic_t		cpufreq_lock_cnt; /* CPU frequency Lock count */
	int				cpufreq_level; /* CPU frequency leve */
#endif
#if defined(CONFIG_CPU_EXYNOS4210) && defined(CONFIG_EXYNOS4_CPUFREQ)
	atomic_t		cpufreq_lock_cnt; /* CPU frequency Lock count */
	int				cpufreq_level; /* CPU frequency leve */
#endif
#ifdef CONFIG_BUSFREQ_OPP
	atomic_t  dmcthreshold_lock_cnt; /* dmc max threshold Lock count */
#endif
#if SUPPORT_SLICE_ENCODING
	int			slice_encoding_flag;
	wait_queue_head_t	wait_slice;
	int			slice_sys;
	int			wait_slice_timeout;
	int			frame_working_flag;
	wait_queue_head_t	wait_frame;
	int			frame_sys;
	int			wait_frame_timeout;
#endif
};

#endif /* __MFC_DEV_H */
