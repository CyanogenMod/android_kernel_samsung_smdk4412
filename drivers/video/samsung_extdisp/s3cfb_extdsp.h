/* linux/drivers/video/samsung/s3cfb_extdsp.h
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

#ifndef _S3CFB_EXTDSP_H
#define _S3CFB_EXTDSP_H __FILE__

#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/fb.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif
#include <plat/fb-s5p.h>
#endif

#define S3CFB_EXTDSP_NAME		"s3cfb_extdsp"
#define POWER_ON		1
#define POWER_OFF		0

enum s3cfb_extdsp_output_t {
	OUTPUT_RGB,
	OUTPUT_ITU,
	OUTPUT_I80LDI0,
	OUTPUT_I80LDI1,
	OUTPUT_WB_RGB,
	OUTPUT_WB_I80LDI0,
	OUTPUT_WB_I80LDI1,
};

enum s3cfb_extdsp_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

enum s3cfb_extdsp_mem_owner_t {
	DMA_MEM_NONE	= 0,
	DMA_MEM_FIMD	= 1,
	DMA_MEM_OTHER	= 2,
};

enum s3cfb_extdsp_buf_status_t {
	BUF_FREE	= 0,
	BUF_ACTIVE	= 1,
	BUF_LOCKED	= 2,
};

struct s3cfb_extdsp_lcd_polarity {
	int rise_vclk;
	int inv_hsync;
	int inv_vsync;
	int inv_vden;
};

struct s3cfb_extdsp_lcd {
	int	width;
	int	height;
	int	bpp;
};

struct s3cfb_extdsp_extdsp_desc {
	int			state;
	struct s3cfb_extdsp_global	*fbdev[1];
};

struct s3cfb_extdsp_time_stamp {
	unsigned int		phys_addr;
	struct timeval		time_marker;
};

struct s3cfb_extdsp_buf_list {
	unsigned int		phys_addr;
	struct timeval		time_marker;
	int			buf_status;
};

struct s3cfb_extdsp_global {
	struct mutex		lock;
	struct device		*dev;
#ifdef CONFIG_BUSFREQ_OPP
	struct device           *bus_dev;
#endif
	struct fb_info		**fb;

	atomic_t		enabled_win;
	enum s3cfb_extdsp_output_t	output;
	enum s3cfb_extdsp_rgb_mode_t	rgb_mode;
	struct s3cfb_extdsp_lcd	*lcd;
	int 			system_state;
#ifdef CONFIG_HAS_WAKELOCK
	struct early_suspend	early_suspend;
	struct wake_lock	idle_lock;
#endif
	struct s3cfb_extdsp_buf_list	buf_list[CONFIG_FB_S5P_EXTDSP_NR_BUFFERS];
	unsigned int			enabled_tz;
	unsigned int			lock_cnt;
};

struct s3cfb_extdsp_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	unsigned int		pseudo_pal[16];
	int			power_state;
	int			lock_status;
	int			lock_buf_idx;
	unsigned int		lock_buf_offset;
	unsigned int		free_buf_offset;
};

struct s3cfb_extdsp_user_window {
	int x;
	int y;
};

/* IOCTL commands */
#define S3CFB_EXTDSP_WIN_POSITION		_IOW('F', 203, \
						struct s3cfb_extdsp_user_window)
#define S3CFB_EXTDSP_GET_LCD_WIDTH		_IOR('F', 302, int)
#define S3CFB_EXTDSP_GET_LCD_HEIGHT		_IOR('F', 303, int)
#define S3CFB_EXTDSP_SET_WIN_ON			_IOW('F', 305, u32)
#define S3CFB_EXTDSP_SET_WIN_OFF		_IOW('F', 306, u32)
#define S3CFB_EXTDSP_SET_WIN_ADDR		_IOW('F', 308, unsigned long)
#define S3CFB_EXTDSP_GET_FB_PHY_ADDR		_IOR('F', 310, unsigned int)

#define S3CFB_EXTDSP_LOCK_BUFFER		_IOW('F', 320, int)
#define S3CFB_EXTDSP_GET_NEXT_INDEX		_IOW('F', 321, unsigned int)
#define S3CFB_EXTDSP_GET_LOCKED_BUFFER		_IOW('F', 322, unsigned int)
#define S3CFB_EXTDSP_PUT_TIME_STAMP		_IOW('F', 323, \
						struct s3cfb_extdsp_time_stamp)
#define S3CFB_EXTDSP_GET_TIME_STAMP		_IOW('F', 324, \
						struct s3cfb_extdsp_time_stamp)
#define S3CFB_EXTDSP_GET_TZ_MODE		_IOW ('F', 325, unsigned int)
#define S3CFB_EXTDSP_SET_TZ_MODE		_IOW ('F', 326, unsigned int)
#define S3CFB_EXTDSP_GET_LOCKED_NUMBER		_IOW ('F', 327, unsigned int)
#define S3CFB_EXTDSP_LOCK_AND_GET_BUF		_IOW ('F', 328, \
						struct s3cfb_extdsp_buf_list)
#define S3CFB_EXTDSP_GET_FREE_BUFFER		_IOW('F', 329, unsigned int)

extern struct fb_ops			s3cfb_extdsp_ops;
extern inline struct s3cfb_extdsp_global	*get_extdsp_global(int id);

/* S3CFB_EXTDSP */
extern int s3cfb_extdsp_enable_window(struct s3cfb_extdsp_global *fbdev, int id);
extern int s3cfb_extdsp_disable_window(struct s3cfb_extdsp_global *fbdev, int id);
extern int s3cfb_extdsp_update_power_state(struct s3cfb_extdsp_global *fbdev, int id,
				int state);
extern int s3cfb_extdsp_init_global(struct s3cfb_extdsp_global *fbdev);
extern int s3cfb_extdsp_map_default_video_memory(struct s3cfb_extdsp_global *fbdev,
					struct fb_info *fb, int extdsp_id);
extern int s3cfb_extdsp_unmap_default_video_memory(struct s3cfb_extdsp_global *fbdev,
					struct fb_info *fb);
extern int s3cfb_extdsp_check_var(struct fb_var_screeninfo *var, struct fb_info *fb);
extern int s3cfb_extdsp_check_var_window(struct s3cfb_extdsp_global *fbdev,
			struct fb_var_screeninfo *var, struct fb_info *fb);
extern int s3cfb_extdsp_set_par_window(struct s3cfb_extdsp_global *fbdev, struct fb_info *fb);
extern int s3cfb_extdsp_set_par(struct fb_info *fb);
extern int s3cfb_extdsp_init_fbinfo(struct s3cfb_extdsp_global *fbdev, int id);
extern int s3cfb_extdsp_alloc_framebuffer(struct s3cfb_extdsp_global *fbdev, int extdsp_id);
extern int s3cfb_extdsp_open(struct fb_info *fb, int user);
extern int s3cfb_extdsp_release_window(struct fb_info *fb);
extern int s3cfb_extdsp_release(struct fb_info *fb, int user);
extern int s3cfb_extdsp_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *fb);
extern int s3cfb_extdsp_blank(int blank_mode, struct fb_info *fb);
extern inline unsigned int __chan_to_field(unsigned int chan,
					struct fb_bitfield bf);
extern int s3cfb_extdsp_setcolreg(unsigned int regno, unsigned int red,
			unsigned int green, unsigned int blue,
			unsigned int transp, struct fb_info *fb);
extern int s3cfb_extdsp_cursor(struct fb_info *fb, struct fb_cursor *cursor);
extern int s3cfb_extdsp_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg);

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
extern void s3cfb_extdsp_early_suspend(struct early_suspend *h);
extern void s3cfb_extdsp_late_resume(struct early_suspend *h);
#endif
#endif

#endif /* _S3CFB_EXTDSP_H */
