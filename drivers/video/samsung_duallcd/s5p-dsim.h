/* linux/drivers/video/samsung/s5p-dsim.h
 *
 * Header file for Samsung MIPI-DSIM driver.
 *
 * Copyright (c) 2009 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
*/

#ifndef _S5P_DSIM_H
#define _S5P_DSIM_H

#include <linux/device.h>

enum dsim_read_id {
	Ack = 0x02,
	EoTp = 0x08,
	GenShort1B = 0x11,
	GenShort2B = 0x12,
	GenLong = 0x1a,
	DcsLong = 0x1c,
	DcsShort1B = 0x21,
	DcsShort2B = 0x22,
};

struct mipi_lcd_driver {
	s8	name[64];

	s32	(*init)(struct device *dev);
	void	(*display_on)(struct device *dev);
	s32	(*probe)(struct device *dev);
	s32	(*remove)(struct device *dev);
	void	(*shutdown)(struct device *dev);
	s32	(*suspend)(struct device *dev, pm_message_t mesg);
	s32	(*resume)(struct device *dev);
};

struct dsim_ops {
	u8	(*cmd_write)(void *ptr, u32 data0, u32 data1, u32 data2);
	int	(*cmd_read)(void *ptr, u8 addr, u16 count, u8 *buf);
	void	(*suspend)(void);
	void	(*resume)(void);
};

/* Indicates the state of the device */
struct dsim_global {
	struct device *dev;
	struct device panel;
	struct clk *clock;
	struct s5p_platform_dsim *pd;
	struct dsim_config *dsim_info;
	struct dsim_lcd_config *dsim_lcd_info;
	/* lcd panel data. */
	struct s3cfb_lcd *lcd_panel_info;
	/* platform and machine specific data for lcd panel driver. */
	struct mipi_ddi_platform_data *mipi_ddi_pd;
	/* lcd panel driver based on MIPI-DSI. */
	struct mipi_lcd_driver *mipi_drv;

	unsigned int irq;
	unsigned int te_irq;
	unsigned int reg_base;
#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	unsigned int panel_select;
#endif /*CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD*/
	unsigned char state;
	unsigned int data_lane;
	enum dsim_byte_clk_src e_clk_src;
	unsigned long hs_clk;
	unsigned long byte_clk;
	unsigned long escape_clk;
	unsigned char freq_band;
	char header_fifo_index[DSIM_HEADER_FIFO_SZ];

	struct delayed_work	dsim_work;
	struct delayed_work	check_hs_toggle_work;
	unsigned int		dsim_toggle_per_frame_count;

	struct dsim_ops		*ops;
};

int s5p_dsim_register_lcd_driver(struct mipi_lcd_driver *lcd_drv);

#endif /* _S5P_DSIM_LOWLEVEL_H */
