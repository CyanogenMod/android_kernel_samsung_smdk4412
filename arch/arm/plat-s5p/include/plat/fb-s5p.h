/* linux/arch/arm/plat-s5p/include/plat/fb-s5p.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_FB_S5P_H
#define __ASM_PLAT_FB_S5P_H __FILE__

#define FB_SWAP_WORD	(1 << 24)
#define FB_SWAP_HWORD	(1 << 16)
#define FB_SWAP_BYTE	(1 << 8)
#define FB_SWAP_BIT	(1 << 0)

struct platform_device;
struct clk;

#ifdef CONFIG_FB_S5P_MIPI_DSIM
/* enumerates display mode. */
enum {
	SINGLE_LCD_MODE = 1,
	DUAL_LCD_MODE = 2,
};

/* enumerates interface mode. */
enum {
	FIMD_RGB_INTERFACE = 1,
	FIMD_CPU_INTERFACE = 2,
};
#endif

struct s3c_platform_fb {
	int		hw_ver;
	char		clk_name[16];
	int		nr_wins;
	int		nr_buffers[5];
	int		default_win;
	int		swap;
	void		*lcd;
	void		(*set_display_path)(void);
	void		(*cfg_gpio)(struct platform_device *dev);
	int		(*backlight_on)(struct platform_device *dev);
	int		(*backlight_off)(struct platform_device *dev);
	int		(*lcd_on)(struct platform_device *dev);
	int		(*lcd_off)(struct platform_device *dev);
	int		(*clk_on)(struct platform_device *pdev, struct clk **s3cfb_clk);
	int		(*clk_off)(struct platform_device *pdev, struct clk **clk);
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
	char	*name;
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

extern void s3cfb_set_platdata(struct s3c_platform_fb *fimd);

/* defined by architecture to configure gpio */
extern void s3cfb_set_display_path(void);
extern void s3cfb_cfg_gpio(struct platform_device *pdev);
extern int s3cfb_backlight_on(struct platform_device *pdev);
extern int s3cfb_backlight_off(struct platform_device *pdev);
extern int s3cfb_lcd_on(struct platform_device *pdev);
extern int s3cfb_lcd_off(struct platform_device *pdev);
extern int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk);
extern int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk);
extern void s3cfb_get_clk_name(char *clk_name);
#endif /* __ASM_PLAT_FB_S5P_H */
