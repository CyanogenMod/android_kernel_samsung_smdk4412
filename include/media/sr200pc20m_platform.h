/*
 * Driver for SR200PC20M (2M camera) from SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define DEFAULT_PREVIEW_WIDTH		640
#define DEFAULT_PREVIEW_HEIGHT		480
#define DEFAULT_CAPTURE_WIDTH		1600
#define DEFAULT_CAPTURE_HEIGHT		1200

/* Define debug level */
#define CAMDBG_LEVEL_ERR		(1 << 0)
#define CAMDBG_LEVEL_WARN		(1 << 1)
#define CAMDBG_LEVEL_INFO		(1 << 2)
#define CAMDBG_LEVEL_DEBUG		(1 << 3)
#define CAMDBG_LEVEL_TRACE		(1 << 4)
#define CAMDBG_LEVEL_DEFAULT	\
	(CAMDBG_LEVEL_ERR | CAMDBG_LEVEL_WARN | CAMDBG_LEVEL_INFO)

struct sr200pc20m_platform_data {
	u32 default_width;
	u32 default_height;
	u32 pixelformat;
	u32 freq;	/* MCLK in KHz */

	/* This SoC supports Parallel & CSI-2 */
	u32 is_mipi;		/* set to 1 if mipi */
	s32 streamoff_delay;	/* ms, type is signed */

	u8 dbg_level;
};
