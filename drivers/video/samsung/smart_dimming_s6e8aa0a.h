/* linux/drivers/video/samsung/smartdimming.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Samsung Smart Dimming for OCTA
 *
 * Minwoo Kim, <minwoo7945.kim@samsung.com>
 *
*/


#ifndef __SMART_DIMMING_H__
#define __SMART_DIMMING_H__


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>

#define MAX_GRADATION		300
#define PANEL_ID_MAX		3
#define GAMMA_300CD_MAX		18


enum {
	CI_RED,
	CI_GREEN,
	CI_BLUE,
	CI_MAX,
};


enum {
	IV_1,
	IV_15,
	IV_35,
	IV_59,
	IV_87,
	IV_171,
	IV_255,
	IV_MAX,
	IV_TABLE_MAX,
};


enum {
	AD_IV0,
	AD_IV1,
	AD_IV15,
	AD_IV35,
	AD_IV59,
	AD_IV87,
	AD_IV171,
	AD_IV255,
	AD_IVMAX,
};

#ifdef CONFIG_AID_DIMMING
enum {
	G_21,
	G_215,
	G_22,
	G_MAX,
};
#endif

struct str_voltage_entry {
	u32 v[CI_MAX];
};


struct str_table_info {
	/* et : start gray value */
	u8 st;
	/* end gray value, st + count */
	u8 et;
	u8 count;
	const u8 *offset_table;
	/* rv : ratio value */
	u32 rv;
};


struct str_flookup_table {
	u16 entry;
	u16 count;
};


struct str_smart_dim {
	u8 panelid[PANEL_ID_MAX];
	s16 mtp[CI_MAX][IV_MAX];
	struct str_voltage_entry ve[256];
	const u8 *default_gamma;
	struct str_table_info t_info[IV_TABLE_MAX];
	const struct str_flookup_table *flooktbl;
	const u32 *g22_tbl;
#ifdef CONFIG_AID_DIMMING
	const u32 *gamma_table[G_MAX];
#endif
	const u32 *g300_gra_tbl;
	u32 adjust_volt[CI_MAX][AD_IVMAX];
};

struct rgb_offset_info {
	unsigned int	candela_idx;
	unsigned int	gray;
	unsigned int	rgb;
	int		offset;
};


int init_table_info(struct str_smart_dim *smart);
u8 calc_voltage_table(struct str_smart_dim *smart, const u8 *mtp);
#ifdef CONFIG_AID_DIMMING
u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve);
u32 calc_gamma_table_215_190(struct str_smart_dim *smart, u32 gv, u8 result[]);
#else
u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[]);
#endif

#endif
