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

#define MAX_GRADATION		250
#define PANEL_ID_MAX		3
#define GAMMA_300CD_MAX	4


#define V1_VOLTAGE_COUNT	150
#define V255_VOLTAGE_COUNT	390
#define V15_VOLTAGE_COUNT	255
#define V35_VOLTAGE_COUNT	255
#define V59_VOLTAGE_COUNT	255
#define V87_VOLTAGE_COUNT	255
#define V171_VOLTAGE_COUNT	255


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


int init_table_info(struct str_smart_dim *smart);
u8 calc_voltage_table(struct str_smart_dim *smart, const u8 *mtp);
u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[]);


#endif
