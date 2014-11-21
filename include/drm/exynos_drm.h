/* exynos_drm.h
 *
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

#ifndef _EXYNOS_DRM_H_
#define _EXYNOS_DRM_H_

#include "drm.h"

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_exynos_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * A structure for getting buffer offset.
 *
 * @handle: a pointer to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @offset: relatived offset value of the memory region allocated.
 *	- this value should be set by user.
 */
struct drm_exynos_gem_map_off {
	unsigned int handle;
	unsigned int pad;
	uint64_t offset;
};

/**
 * A structure for mapping buffer.
 *
 * @handle: a handle to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @size: memory size to be mapped.
 * @mapped: having user virtual address mmaped.
 *	- this variable would be filled by exynos gem module
 *	of kernel side with user virtual address which is allocated
 *	by do_mmap().
 */
struct drm_exynos_gem_mmap {
	unsigned int handle;
	unsigned int pad;
	uint64_t size;
	uint64_t mapped;
};

/**
 * User-requested user space importing structure
 *
 * @userptr: user space address allocated by malloc.
 * @size: size to the buffer allocated by malloc.
 * @flags: indicate user-desired cache attribute to map the allocated buffer
 *	to kernel space.
 * @handle: a returned handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_exynos_gem_userptr {
	uint64_t userptr;
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * A structure to gem information.
 *
 * @handle: a handle to gem object created.
 * @flags: flag value including memory type and cache attribute and
 *	this value would be set by driver.
 * @size: size to memory region allocated by gem and this size would
 *	be set by driver.
 */
struct drm_exynos_gem_info {
	unsigned int handle;
	unsigned int flags;
	uint64_t size;
};

/**
 * A structure to userptr limited information.
 *
 * @userptr_limit: maximum size to userptr buffer.
 *	the buffer could be allocated by unprivileged user using malloc()
 *	and the size of the buffer would be limited as userptr_limit value.
 * @pad: just padding to be 64-bit aligned.
 */
struct drm_exynos_user_limit {
	unsigned int userptr_limit;
	unsigned int pad;
};

/**
 * A structure for user connection request of virtual display.
 *
 * @connection: indicate whether doing connetion or not by user.
 * @extensions: if this value is 1 then the vidi driver would need additional
 *	128bytes edid data.
 * @edid: the edid data pointer from user side.
 */
struct drm_exynos_vidi_connection {
	unsigned int connection;
	unsigned int extensions;
	uint64_t *edid;
};

/**
 * A structure for ump.
 *
 * @gem_handle: a pointer to gem object created.
 * @secure_id: ump secure id and this value would be filled
 *		by kernel side.
 */
struct drm_exynos_gem_ump {
	unsigned int gem_handle;
	unsigned int secure_id;
};


/* temporary codes for legacy fimc and mfc drivers. */

/**
 * A structure for getting physical address corresponding to a gem handle.
 */
struct drm_exynos_gem_get_phy {
	unsigned int gem_handle;
	unsigned int pad;
	uint64_t size;
	uint64_t phy_addr;
};

/**
 * A structure for importing physical memory to a gem.
 */
struct drm_exynos_gem_phy_imp {
	uint64_t phy_addr;
	uint64_t size;
	unsigned int gem_handle;
	unsigned int pad;
};

/* indicate cache units. */
enum e_drm_exynos_gem_cache_sel {
	EXYNOS_DRM_L1_CACHE		= 1 << 0,
	EXYNOS_DRM_L2_CACHE		= 1 << 1,
	EXYNOS_DRM_ALL_CORES		= 1 << 2,
	EXYNOS_DRM_ALL_CACHES		= EXYNOS_DRM_L1_CACHE |
						EXYNOS_DRM_L2_CACHE,
	EXYNOS_DRM_ALL_CACHES_CORES	= EXYNOS_DRM_L1_CACHE |
						EXYNOS_DRM_L2_CACHE |
						EXYNOS_DRM_ALL_CORES,
	EXYNOS_DRM_CACHE_SEL_MASK	= EXYNOS_DRM_ALL_CACHES_CORES
};

/* indicate cache operation types. */
enum e_drm_exynos_gem_cache_op {
	EXYNOS_DRM_CACHE_INV_ALL	= 1 << 3,
	EXYNOS_DRM_CACHE_INV_RANGE	= 1 << 4,
	EXYNOS_DRM_CACHE_CLN_ALL	= 1 << 5,
	EXYNOS_DRM_CACHE_CLN_RANGE	= 1 << 6,
	EXYNOS_DRM_CACHE_FSH_ALL	= EXYNOS_DRM_CACHE_INV_ALL |
						EXYNOS_DRM_CACHE_CLN_ALL,
	EXYNOS_DRM_CACHE_FSH_RANGE	= EXYNOS_DRM_CACHE_INV_RANGE |
						EXYNOS_DRM_CACHE_CLN_RANGE,
	EXYNOS_DRM_CACHE_OP_MASK	= EXYNOS_DRM_CACHE_FSH_ALL |
						EXYNOS_DRM_CACHE_FSH_RANGE
};

/* memory type definitions. */
enum e_drm_exynos_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	EXYNOS_BO_CONTIG	= 0 << 0,
	/* Physically Non-Continuous memory. */
	EXYNOS_BO_NONCONTIG	= 1 << 0,
	/* non-cachable mapping and used as default. */
	EXYNOS_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	EXYNOS_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	EXYNOS_BO_WC		= 1 << 2,
	/* user space memory allocated by malloc. */
	EXYNOS_BO_USERPTR	= 1 << 3,
	EXYNOS_BO_MASK		= EXYNOS_BO_NONCONTIG | EXYNOS_BO_CACHABLE |
					EXYNOS_BO_WC | EXYNOS_BO_USERPTR
};

/**
 * A structure for cache operation.
 *
 * @usr_addr: user space address.
 *	P.S. it SHOULD BE user space.
 * @size: buffer size for cache operation.
 * @flags: select cache unit and cache operation.
 * @gem_handle: a handle to a gem object.
 *	this gem handle is needed for cache range operation to L2 cache.
 */
struct drm_exynos_gem_cache_op {
	uint64_t usr_addr;
	unsigned int size;
	unsigned int flags;
	unsigned int gem_handle;
};

struct drm_exynos_g2d_get_ver {
	__u32	major;
	__u32	minor;
};

struct drm_exynos_g2d_cmd {
	__u32	offset;
	__u32	data;
};

enum drm_exynos_g2d_event_type {
	G2D_EVENT_NOT,
	G2D_EVENT_NONSTOP,
	G2D_EVENT_STOP,		/* not yet */
};

struct drm_exynos_g2d_set_cmdlist {
	struct drm_exynos_g2d_cmd		*cmd;
	struct drm_exynos_g2d_cmd		*cmd_gem;
	__u32					cmd_nr;
	__u32					cmd_gem_nr;

	/* for g2d event */
	__u64					user_data;
	__u32					event_type;
	__u32					reserved;
};

struct drm_exynos_g2d_exec {
	__u32					async;
	__u32					reserved;
};

/* definition of operations types */
enum drm_exynos_ops_id {
	EXYNOS_DRM_OPS_SRC,
	EXYNOS_DRM_OPS_DST,
	EXYNOS_DRM_OPS_MAX,
};

/* definition of size */
struct drm_exynos_sz {
	__u32	hsize;
	__u32	vsize;
};

/* definition of position */
struct drm_exynos_pos {
	__u32	x;
	__u32	y;
	__u32	w;
	__u32	h;
};

/* definition of flip */
enum drm_exynos_flip {
	EXYNOS_DRM_FLIP_NONE = (0 << 0),
	EXYNOS_DRM_FLIP_VERTICAL = (1 << 0),
	EXYNOS_DRM_FLIP_HORIZONTAL = (1 << 1),
};

/* definition of rotation degree */
enum drm_exynos_degree {
	EXYNOS_DRM_DEGREE_0,
	EXYNOS_DRM_DEGREE_90,
	EXYNOS_DRM_DEGREE_180,
	EXYNOS_DRM_DEGREE_270,
};

/* definition of planar */
enum drm_exynos_planer {
	EXYNOS_DRM_PLANAR_Y,
	EXYNOS_DRM_PLANAR_CB,
	EXYNOS_DRM_PLANAR_CR,
	EXYNOS_DRM_PLANAR_MAX,
};

/**
 * A structure for ipp config.
 *
 * @ops_id: property of operation directions.
 * @flip: property of mirror, flip.
 * @degree: property of rotation degree.
 * @fmt: property of image format.
 * @sz: property of image size.
 * @pos: property of image position(src-cropped,dst-scaler).
 */
struct drm_exynos_ipp_config {
	enum drm_exynos_ops_id ops_id;
	enum drm_exynos_flip	flip;
	enum drm_exynos_degree	degree;
	__u32	fmt;
	struct drm_exynos_sz	sz;
	struct drm_exynos_pos	pos;
};

/* command of ipp operations */
enum drm_exynos_ipp_cmd {
	IPP_CMD_NONE,
	IPP_CMD_M2M,
	IPP_CMD_WB,
	IPP_CMD_OUTPUT,
	IPP_CMD_MAX,
};

/**
 * A structure for ipp property.
 *
 * @config: source, destination config.
 * @cmd: command type.
 * @ipp_id: id of ipp driver.
 * @prop_id: id of property.
 */
struct drm_exynos_ipp_property {
	struct drm_exynos_ipp_config config[EXYNOS_DRM_OPS_MAX];
	enum drm_exynos_ipp_cmd	cmd;
	__u32	ipp_id;
	__u32	prop_id;
	__u32	reserved;
};

/* definition of buffer control */
enum drm_exynos_ipp_buf_ctrl {
	IPP_BUF_CTRL_QUEUE,
	IPP_BUF_CTRL_DEQUEUE,
};

/**
 * A structure for ipp buffer operations.
 *
 * @ops_id: operation directions.
 * @ctrl: buffer control.
 * @prop_id: id of property.
 * @buf_id: id of buffer.
 * @handle: Y, Cb, Cr each planar handle.
 * @user_data: user data.
 */
struct drm_exynos_ipp_buf {
	enum drm_exynos_ops_id	ops_id;
	enum drm_exynos_ipp_buf_ctrl	buf_ctrl;
	__u32	prop_id;
	__u32	buf_id;
	__u32	handle[EXYNOS_DRM_PLANAR_MAX];
	__u32	reserved;
	__u64	user_data;
};

/**
 * A structure for ipp start/stop operations.
 *
 * @prop_id: id of property.
 * @use: use ipp device.
 */
struct drm_exynos_ipp_ctrl {
	__u32	prop_id;
	__u32	use;
};

#define DRM_EXYNOS_GEM_CREATE		0x00
#define DRM_EXYNOS_GEM_MAP_OFFSET	0x01
#define DRM_EXYNOS_GEM_MMAP		0x02
#define DRM_EXYNOS_GEM_USERPTR		0x03
#define DRM_EXYNOS_GEM_GET		0x04
#define DRM_EXYNOS_USER_LIMIT		0x05
#define DRM_EXYNOS_VIDI_CONNECTION	0x07

/* temporary ioctl command. */
#define DRM_EXYNOS_GEM_EXPORT_UMP	0x10
#define DRM_EXYNOS_GEM_CACHE_OP		0x12

#define DRM_EXYNOS_GEM_GET_PHY		0x13
#define DRM_EXYNOS_GEM_PHY_IMP		0x14

/* G2D */
#define DRM_EXYNOS_G2D_GET_VER		0x20
#define DRM_EXYNOS_G2D_SET_CMDLIST	0x21
#define DRM_EXYNOS_G2D_EXEC		0x22

/* IPP - Image Post Processing */
#define DRM_EXYNOS_IPP_GET_PROPERTY	0x30
#define DRM_EXYNOS_IPP_SET_PROPERTY	0x31
#define DRM_EXYNOS_IPP_BUF			0x32
#define DRM_EXYNOS_IPP_CTRL			0x33

#define DRM_IOCTL_EXYNOS_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CREATE, struct drm_exynos_gem_create)

#define DRM_IOCTL_EXYNOS_GEM_MAP_OFFSET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MAP_OFFSET, struct drm_exynos_gem_map_off)

#define DRM_IOCTL_EXYNOS_GEM_MMAP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MMAP, struct drm_exynos_gem_mmap)

#define DRM_IOCTL_EXYNOS_GEM_USERPTR	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_USERPTR, struct drm_exynos_gem_userptr)

#define DRM_IOCTL_EXYNOS_GEM_GET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_GET,	struct drm_exynos_gem_info)

#define DRM_IOCTL_EXYNOS_USER_LIMIT	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_USER_LIMIT,	struct drm_exynos_user_limit)

#define DRM_IOCTL_EXYNOS_GEM_EXPORT_UMP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_EXPORT_UMP, struct drm_exynos_gem_ump)

#define DRM_IOCTL_EXYNOS_GEM_CACHE_OP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CACHE_OP, struct drm_exynos_gem_cache_op)

/* temporary ioctl command. */
#define DRM_IOCTL_EXYNOS_GEM_GET_PHY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_GET_PHY, struct drm_exynos_gem_get_phy)
#define DRM_IOCTL_EXYNOS_GEM_PHY_IMP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_PHY_IMP, struct drm_exynos_gem_phy_imp)

#define DRM_IOCTL_EXYNOS_VIDI_CONNECTION	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_VIDI_CONNECTION, struct drm_exynos_vidi_connection)

#define DRM_IOCTL_EXYNOS_G2D_GET_VER		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_GET_VER, struct drm_exynos_g2d_get_ver)
#define DRM_IOCTL_EXYNOS_G2D_SET_CMDLIST	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_SET_CMDLIST, struct drm_exynos_g2d_set_cmdlist)
#define DRM_IOCTL_EXYNOS_G2D_EXEC		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_EXEC, struct drm_exynos_g2d_exec)

#define DRM_IOCTL_EXYNOS_IPP_GET_PROPERTY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_GET_PROPERTY, struct drm_exynos_ipp_property)
#define DRM_IOCTL_EXYNOS_IPP_SET_PROPERTY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_SET_PROPERTY, struct drm_exynos_ipp_property)
#define DRM_IOCTL_EXYNOS_IPP_BUF	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_BUF, struct drm_exynos_ipp_buf)
#define DRM_IOCTL_EXYNOS_IPP_CTRL		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_CTRL, struct drm_exynos_ipp_ctrl)

/* EXYNOS specific events */
#define DRM_EXYNOS_G2D_EVENT		0x80000000
#define DRM_EXYNOS_IPP_EVENT		0x80000001

struct drm_exynos_g2d_event {
	struct drm_event	base;
	__u64			user_data;
	__u32			tv_sec;
	__u32			tv_usec;
	__u32			cmdlist_no;
	__u32			reserved;
};

struct drm_exynos_ipp_event {
	struct drm_event	base;
	__u64			user_data;
	__u32			tv_sec;
	__u32			tv_usec;
	__u32			prop_id;
	__u32			reserved;
	__u32			buf_id[EXYNOS_DRM_OPS_MAX];
};

/**
 * A structure for lcd panel information.
 *
 * @timing: default video mode for initializing
 * @width_mm: physical size of lcd width.
 * @height_mm: physical size of lcd height.
 */
struct exynos_drm_panel_info {
	struct fb_videomode timing;
	u32 width_mm;
	u32 height_mm;
};

/**
 * Platform Specific Structure for DRM based FIMD.
 *
 * @panel: default panel info for initializing
 * @default_win: default window layer number to be used for UI.
 * @bpp: default bit per pixel.
 * @enabled: indicate whether fimd hardware was on or not at bootloader.
 */
struct exynos_drm_fimd_pdata {
	struct exynos_drm_panel_info panel;
	u32				vidcon0;
	u32				vidcon1;
	unsigned int			default_win;
	unsigned int			bpp;
	bool				enabled;
	bool				mdnie_enabled;
	unsigned int			dynamic_refresh;
	unsigned int			high_freq;
};

/**
 * Platform Specific Structure for DRM based HDMI.
 *
 * @hdmi_dev: device point to specific hdmi driver.
 * @mixer_dev: device point to specific mixer driver.
 *
 * this structure is used for common hdmi driver and each device object
 * would be used to access specific device driver(hdmi or mixer driver)
 */
struct exynos_drm_common_hdmi_pd {
	struct device *hdmi_dev;
	struct device *mixer_dev;
};

/**
 * Platform Specific Structure for DRM based HDMI core.
 *
 * @is_v13: set if hdmi version 13 is.
 * @cfg_hpd: function pointer to configure hdmi hotplug detection pin
 * @get_hpd: function pointer to get value of hdmi hotplug detection pin
 */
struct exynos_drm_hdmi_pdata {
	bool is_v13;
	void (*cfg_hpd)(bool external);
	int (*get_hpd)(void);
};

/**
 * Platform Specific Structure for DRM based FIMC.
 *
 * @inv_pclk: if set 1. invert pixel clock
 * @inv_vsync: if set 1. invert vsync signal for wb
 * @inv_href: if set 1. invert href signal
 * @inv_hsync: if set 1. invert hsync signal for wb
 */
struct exynos_drm_fimc_pol {
	unsigned int inv_pclk;
	unsigned int inv_vsync;
	unsigned int inv_href;
	unsigned int inv_hsync;
};

/* definition of chipset version */
enum exynos_drm_fimc_ver {
	FIMC_EXYNOS_4210,
	FIMC_EXYNOS_4212,
	FIMC_EXYNOS_4412,
};

/**
 * Platform Specific Structure for DRM based FIMC.
 *
 * @pol: current hardware block polarity settings.
 * @ver: current hardware block version.
 */
struct exynos_drm_fimc_pdata {
	struct exynos_drm_fimc_pol pol;
	enum exynos_drm_fimc_ver ver;
};

#endif
