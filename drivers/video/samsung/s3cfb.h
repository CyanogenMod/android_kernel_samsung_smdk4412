/* linux/drivers/video/samsung/s3cfb.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Header file for Samsung Display Driver (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_H
#define _S3CFB_H __FILE__

#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/fb.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif
#include <plat/fb-s5p.h>
#endif
#include <linux/kthread.h>
#include <mach/cpufreq.h>

#define S3CFB_NAME		"s3cfb"
#define S3CFB_AVALUE(r, g, b)	(((r & 0xf) << 8) | \
				((g & 0xf) << 4) | \
				((b & 0xf) << 0))
#define S3CFB_CHROMA(r, g, b)	(((r & 0xff) << 16) | \
				((g & 0xff) << 8) | \
				((b & 0xff) << 0))

#if defined(CONFIG_FB_S5P_DUAL_LCD)
#define FIMD_MAX		2
#else
#define FIMD_MAX		1
#endif
#define MAX_BUFFER_NUM	3

#define POWER_ON		1
#define POWER_OFF		0

#define VSYNC_TIMEOUT_MSEC 50

#if defined(CONFIG_MACH_PX) || defined(CONFIG_MACH_Q1_BD)
#define FEATURE_BUSFREQ_LOCK /* Now, this feature only avaliable in 4210 */
#endif

/* S3C_FB_MAX_WIN
 * Set to the maximum number of windows that any of the supported hardware
 * can use. Since the platform data uses this for an array size, having it
 * set to the maximum of any version of the hardware can do is safe.
 */
#define S3C_FB_MAX_WIN	(5)

#define FENCE_SUPPORT		1
#define FENCE_NOT_SUPPORT	0

enum s3cfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
	DATA_PATH_IPC = 2,
};

enum s3cfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s3cfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

enum s3cfb_output_t {
	OUTPUT_RGB,
	OUTPUT_ITU,
	OUTPUT_I80LDI0,
	OUTPUT_I80LDI1,
	OUTPUT_WB_RGB,
	OUTPUT_WB_I80LDI0,
	OUTPUT_WB_I80LDI1,
};

enum s3cfb_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

enum s3cfb_mem_owner_t {
	DMA_MEM_NONE	= 0,
	DMA_MEM_FIMD	= 1,
	DMA_MEM_OTHER	= 2,
};

struct s3cfb_alpha {
	enum		s3cfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

struct s3cfb_chroma {
	int		enabled;
	int		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum		s3cfb_chroma_dir_t dir;
};

struct s3cfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
#if defined(CONFIG_FB_S5P_MIPI_DSIM) || defined(CONFIG_S5P_MIPI_DSI2)
	int	cmd_allow_len;
	int	stable_vfp;
	void	(*cfg_gpio)(struct platform_device *dev);
	int	(*backlight_on)(struct platform_device *dev);
	int	(*reset_lcd)(struct platform_device *dev);
#endif
};

struct s3cfb_lcd_polarity {
	int rise_vclk;
	int inv_hsync;
	int inv_vsync;
	int inv_vden;
};

#ifdef CONFIG_FB_S5P_MIPI_DSIM
/* for CPU Interface */
struct s3cfb_cpu_timing {
	unsigned int	cs_setup;
	unsigned int	wr_setup;
	unsigned int	wr_act;
	unsigned int	wr_hold;
};
#endif

struct s3cfb_lcd {
#ifdef CONFIG_FB_S5P_MIPI_DSIM
	char	*name;
#endif
	int	width;
	int	height;
	int	p_width;
	int	p_height;
	int	bpp;
	int	freq;
	int	freq_limit;
	int	vclk;
	struct	s3cfb_lcd_timing timing;
	struct	s3cfb_lcd_polarity polarity;
#ifdef CONFIG_FB_S5P_MIPI_DSIM
	struct	s3cfb_cpu_timing cpu_timing;
#endif
	void	(*init_ldi)(void);
	void	(*deinit_ldi)(void);
};

struct s3cfb_fimd_desc {
	int			state;
	int			dual;
	struct s3cfb_global	*fbdev[FIMD_MAX];
};

/**
 * struct s3cfb_vsync - vsync information+
 * @wait:              a queue for processes waiting for vsync
 * @timestamp:         the time of the last vsync interrupt
 * @active:            whether userspace is requesting vsync uevents
 * @irq_refcount:      reference count for the underlying irq
 * @irq_lock:          mutex protecting the irq refcount and register
 * @thread:            uevent-generating thread
 */
struct s3cfb_vsync {
	wait_queue_head_t       wait;
	ktime_t                 timestamp;
	bool                    active;
	int                     irq_refcount;
	struct mutex            irq_lock;
	struct task_struct      *thread;
};

#ifdef CONFIG_FB_S5P_SYSMMU
struct sysmmu_flag {
	bool			enabled;
	unsigned long		default_fb_addr;
	unsigned long		pgd;
};
#endif

struct s3cfb_global {
	void __iomem		*regs;
	void __iomem		*regs_org;
	void __iomem		*ielcd_regs;	/* to force upload Dump */
	void __iomem		*dsim_regs;	/* to forcd upload Dump */
	struct mutex		lock;
	struct mutex		output_lock;
	spinlock_t		slock;
	struct device		*dev;
#ifdef CONFIG_BUSFREQ_OPP
	struct device           *bus_dev;
#endif
	spinlock_t		vsync_slock;
	struct clk		*clock;
	int			irq;
	wait_queue_head_t	wq;
	unsigned int		wq_count;
	struct fb_info		**fb;
	struct s3cfb_vsync	vsync_info;

	atomic_t		enabled_win;
	enum s3cfb_output_t	output;
	enum s3cfb_rgb_mode_t	rgb_mode;
	struct s3cfb_lcd	*lcd;
	int			system_state;

	/* New added */
	struct list_head	update_regs_list;
	struct mutex		update_regs_list_lock;
	struct kthread_worker	update_regs_worker;
	struct task_struct	*update_regs_thread;
	struct kthread_work	update_regs_work;

	struct sw_sync_timeline *timeline;
	int			timeline_max;
	unsigned int		support_fence;
#ifdef CONFIG_HAS_WAKELOCK
	struct early_suspend	early_suspend;
	struct wake_lock	idle_lock;
#endif
#ifdef FEATURE_BUSFREQ_LOCK
	atomic_t		busfreq_lock_cnt;	/* Bus frequency Lock count */
	int			busfreq_flag;		/* context bus frequency flag*/
#endif
#ifdef CONFIG_FB_S5P_SYSMMU
	struct sysmmu_flag	sysmmu;
#endif
#if defined(CONFIG_FB_S5P_GD2EVF)
	bool			suspend;
#endif
        struct fb_fix_screeninfo initial_fix;
        struct fb_var_screeninfo initial_var;
};

struct s3cfb_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	enum			s3cfb_data_path_t path;
	enum			s3cfb_mem_owner_t owner;
	int			local_channel;
	int			dma_burst;
	unsigned int		pseudo_pal[16];
	struct			s3cfb_alpha alpha;
	struct			s3cfb_chroma chroma;
	int			power_state;
};

struct s3cfb_user_window {
	int x;
	int y;
};

struct s3cfb_user_plane_alpha {
	int		channel;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s3cfb_user_chroma {
	int		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

enum s3c_fb_pixel_format {
	S3C_FB_PIXEL_FORMAT_RGBA_8888 = 0,
	S3C_FB_PIXEL_FORMAT_RGB_888 = 1,
	S3C_FB_PIXEL_FORMAT_BGRA_8888 = 2,
	S3C_FB_PIXEL_FORMAT_RGB_565 = 3,
	S3C_FB_PIXEL_FORMAT_RGBX_8888 = 4,
	S3C_FB_PIXEL_FORMAT_RGBA_5551 = 5,
	S3C_FB_PIXEL_FORMAT_RGBA_4444 = 6,
	S3C_FB_PIXEL_FORMAT_MAX = 7,
};

enum s3c_fb_blending {
	S3C_FB_BLENDING_NONE = 0,
	S3C_FB_BLENDING_PREMULT = 1,
	S3C_FB_BLENDING_COVERAGE = 2,
	S3C_FB_BLENDING_MAX = 3,
};

struct s3c_fb_win_config {
	enum {
		S3C_FB_WIN_STATE_DISABLED = 0,
		S3C_FB_WIN_STATE_COLOR,
		S3C_FB_WIN_STATE_BUFFER,
	} state;

	union {
		__u32 color;
		struct {
			int	fd;
			__u32	phys_addr;
			__u32	virt_addr;
			__u32	offset;
			__u32	stride;
			enum s3c_fb_pixel_format format;
			enum s3c_fb_blending blending;
			int	fence_fd;
		};
	};

	int	x;
	int	y;
	__u32	w;
	__u32	h;
};

struct s3c_fb_win_config_data {
	int	fence;
	struct s3c_fb_win_config config[S3C_FB_MAX_WIN];
};

struct s3c_reg_data {
	struct list_head	list;
	u32			shadowcon;
	u32			wincon[S3C_FB_MAX_WIN];
	u32			win_rgborder[S3C_FB_MAX_WIN];
	u32			winmap[S3C_FB_MAX_WIN];
	u32			vidosd_a[S3C_FB_MAX_WIN];
	u32			vidosd_b[S3C_FB_MAX_WIN];
	u32			vidosd_c[S3C_FB_MAX_WIN];
	u32			vidosd_d[S3C_FB_MAX_WIN];
	u32			vidw_alpha0[S3C_FB_MAX_WIN];
	u32			vidw_alpha1[S3C_FB_MAX_WIN];
	u32			blendeq[S3C_FB_MAX_WIN - 1];
	u32			vidw_buf_start[S3C_FB_MAX_WIN];
	u32			vidw_buf_end[S3C_FB_MAX_WIN];
	u32			vidw_buf_size[S3C_FB_MAX_WIN];
};

#define BLENDING_NONE			0x0100
#define BLENDING_PREMULT		0x0105
#define BLENDING_COVERAGE		0x0405

/* IOCTL commands */
#define S3CFB_WIN_POSITION		_IOW('F', 203, \
						struct s3cfb_user_window)
#define S3CFB_WIN_SET_PLANE_ALPHA	_IOW('F', 204, \
						struct s3cfb_user_plane_alpha)
#define S3CFB_WIN_SET_CHROMA		_IOW('F', 205, \
						struct s3cfb_user_chroma)
#define S3CFB_SET_VSYNC_INT		_IOW('F', 206, u32)
#define S3CFB_GET_VSYNC_INT_STATUS	_IOR('F', 207, u32)
#define S3CFB_GET_ION_USER_HANDLE       _IOWR('F', 208, struct s3c_fb_user_ion_client)
#define S3CFB_WIN_CONFIG                _IOW('F', 209, struct s3c_fb_win_config_data)
#define S3CFB_GET_LCD_WIDTH		_IOR('F', 302, int)
#define S3CFB_GET_LCD_HEIGHT		_IOR('F', 303, int)
#define S3CFB_SET_WRITEBACK		_IOW('F', 304, u32)
#define S3CFB_SET_WIN_ON		_IOW('F', 305, u32)
#define S3CFB_SET_WIN_OFF		_IOW('F', 306, u32)
#define S3CFB_SET_WIN_PATH		_IOW('F', 307, \
						enum s3cfb_data_path_t)
#define S3CFB_SET_WIN_ADDR		_IOW('F', 308, unsigned long)
#define S3CFB_SET_WIN_MEM		_IOW('F', 309, \
						enum s3cfb_mem_owner_t)
#define S3CFB_GET_FB_PHY_ADDR           _IOR('F', 310, unsigned int)
#define S3CFB_GET_CUR_WIN_BUF_ADDR	_IOR('F', 311, unsigned int)
#if defined(CONFIG_CPU_EXYNOS4210)
#define S3CFB_SET_WIN_MEM_START		_IOW('F', 312, u32)
#endif
#define S3CFB_SET_ALPHA_MODE		_IOW('F', 313, unsigned int)
#define S3CFB_SET_INITIAL_CONFIG        _IO('F', 314)
#define S3CFB_SUPPORT_FENCE	        _IOW('F', 315, unsigned int)

extern struct fb_ops			s3cfb_ops;
extern struct s3cfb_global	*get_fimd_global(int id);

/* S3CFB */
extern struct s3c_platform_fb *to_fb_plat(struct device *dev);
extern int s3cfb_draw_logo(struct fb_info *fb);
extern int s3cfb_enable_window(struct s3cfb_global *fbdev, int id);
extern int s3cfb_disable_window(struct s3cfb_global *fbdev, int id);
extern int s3cfb_update_power_state(struct s3cfb_global *fbdev, int id,
				int state);
extern int s3cfb_init_global(struct s3cfb_global *fbdev);
extern int s3cfb_map_video_memory(struct s3cfb_global *fbdev,
				struct fb_info *fb);
extern int s3cfb_unmap_video_memory(struct s3cfb_global *fbdev,
				struct fb_info *fb);
extern int s3cfb_map_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb, int fimd_id);
extern int s3cfb_unmap_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb);
extern int s3cfb_set_bitfield(struct fb_var_screeninfo *var);
extern int s3cfb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s3cfb_window *win);
extern int s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb);
extern int s3cfb_check_var_window(struct s3cfb_global *fbdev,
			struct fb_var_screeninfo *var, struct fb_info *fb);
extern void s3cfb_set_win_params(struct s3cfb_global *fbdev, int id);
extern int s3cfb_set_par_window(struct s3cfb_global *fbdev, struct fb_info *fb);
extern int s3cfb_set_par(struct fb_info *fb);
extern int s3cfb_init_fbinfo(struct s3cfb_global *fbdev, int id);
extern int s3cfb_alloc_framebuffer(struct s3cfb_global *fbdev, int fimd_id);
extern int s3cfb_open(struct fb_info *fb, int user);
extern int s3cfb_release_window(struct fb_info *fb);
extern int s3cfb_release(struct fb_info *fb, int user);
extern int s3cfb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *fb);
extern int s3cfb_blank(int blank_mode, struct fb_info *fb);
extern inline unsigned int __chan_to_field(unsigned int chan,
					struct fb_bitfield bf);
extern int s3cfb_setcolreg(unsigned int regno, unsigned int red,
			unsigned int green, unsigned int blue,
			unsigned int transp, struct fb_info *fb);
extern int s3cfb_cursor(struct fb_info *fb, struct fb_cursor *cursor);
extern int s3cfb_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg);
extern int s3cfb_enable_localpath(struct s3cfb_global *fbdev, int id);
extern int s3cfb_disable_localpath(struct s3cfb_global *fbdev, int id);

/* FIMD */
extern int s3cfb_clear_interrupt(struct s3cfb_global *ctrl);
extern int s3cfb_display_on(struct s3cfb_global *ctrl);
extern int s3cfb_set_output(struct s3cfb_global *ctrl);
extern int s3cfb_set_display_mode(struct s3cfb_global *ctrl);
extern int s3cfb_display_on(struct s3cfb_global *ctrl);
extern int s3cfb_display_off(struct s3cfb_global *ctrl);
extern int s3cfb_set_clock(struct s3cfb_global *ctrl);
extern int s3cfb_set_polarity(struct s3cfb_global *ctrl);
extern int s3cfb_set_polarity_only(struct s3cfb_global *ctrl);
extern int s3cfb_set_timing(struct s3cfb_global *ctrl);
extern int s3cfb_set_lcd_size(struct s3cfb_global *ctrl);
extern int s3cfb_set_global_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_set_vsync_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_get_vsync_interrupt(struct s3cfb_global *ctrl);
extern int s3cfb_set_fifo_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_window_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_window_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_win_map_on(struct s3cfb_global *ctrl, int id, int color);
extern int s3cfb_win_map_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_control(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_alpha_mode(struct s3cfb_global *ctrl, int id, unsigned int mode);
extern int s3cfb_set_alpha_value_width(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_alpha_blending(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_oneshot(struct s3cfb_global *ctrl, int value);
extern int s3cfb_set_alpha_value(struct s3cfb_global *ctrl, int value);
extern int s3cfb_set_window_position(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_address(struct s3cfb_global *ctrl, int id);
extern int s3cfb_get_win_cur_buf_addr(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_chroma_key(struct s3cfb_global *ctrl, int id);
extern int s3cfb_channel_localpath_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_channel_localpath_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_vsync_int(struct fb_info *info, bool active);
extern int s3cfb_check_vsync_status(struct s3cfb_global *ctrl);
extern int s3cfb_set_dualrgb(struct s3cfb_global *ctrl, int mode);
extern int s3cfb_set_window_protect(struct s3cfb_global *ctrl, int id, bool protect);
extern void s3c_fb_update_regs(struct s3cfb_global *fbdev, struct s3c_reg_data *regs);
#ifdef CONFIG_FB_S5P_SYSMMU
extern void s3cfb_clean_outer_pagetable(unsigned long vaddr, size_t size);
#endif
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
extern int s3cfb_wait_for_vsync(struct s3cfb_global *fbdev, u32 timeout);
#endif

/* LCD */
extern void s3cfb_set_lcd_info(struct s3cfb_global *ctrl);

#ifdef CONFIG_FB_S5P_MIPI_DSIM
extern int s3cfb_vsync_status_check(void);
extern void s5p_dsim_early_suspend(void);
extern void s5p_dsim_late_resume(void);
extern void set_dsim_hs_clk_toggle_count(u8 count);
extern void set_dsim_lcd_enabled(u8 enable);
#endif


#ifdef CONFIG_FB_S5P_AMS369FG06
extern void ams369fg06_ldi_init(void);
extern void ams369fg06_ldi_enable(void);
extern void ams369fg06_ldi_disable(void);
extern void ams369fg06_gpio_cfg(void);
#elif defined(CONFIG_FB_S5P_LMS501KF03)
extern void lms501kf03_ldi_disable(void);
#endif

#endif /* _S3CFB_H */

