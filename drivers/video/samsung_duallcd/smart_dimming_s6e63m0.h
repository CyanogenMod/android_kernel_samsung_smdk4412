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
//#define GAMMA_300CD_MAX		16

#define V1_VOLTAGE_COUNT        141
#define V19_VOLTAGE_COUNT       256
#define V43_VOLTAGE_COUNT       256
#define V87_VOLTAGE_COUNT       256
#define V171_VOLTAGE_COUNT      256
#define V255_VOLTAGE_COUNT      381


enum {
	CI_RED		= 0,
	CI_GREEN	= 1,
	CI_BLUE		= 2,
	CI_MAX		= 3,
};


enum {
	IV_1		= 0,
	IV_19		= 1,
	IV_43		= 2,
	IV_87		= 3,
	IV_171		= 4,
	IV_255		= 5,
	IV_MAX		= 6,
	IV_TABLE_MAX	= 7,
};


enum {
	AD_IV0		= 0,
	AD_IV1		= 1,
	AD_IV19		= 2,
	AD_IV43		= 3,
	AD_IV87		= 4,
	AD_IV171	= 5,
	AD_IV255	= 6,
	AD_IVMAX	= 7,
};


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
u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[]);
#endif
