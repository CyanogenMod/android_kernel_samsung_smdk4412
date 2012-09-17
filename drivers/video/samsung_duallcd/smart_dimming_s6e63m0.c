/* linux/drivers/video/samsung/smartdimming.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Samsung Smart Dimming for OCTA
 *
 * Minwoo Kim, <minwoo7945.kim@samsung.com>
 *
*/


#include "smart_dimming_s6e63m0.h"
#include "s6e63m0_volt_tbl.h"

#define MTP_REVERSE		1
#define VALUE_DIM_1000	1000

#define VREG_OUT_1000		4301


const u8 v1_offset_table[18] = {
	75, 69, 63, 57,
	51, 46, 41, 36,
	31, 27, 23, 19,
	15, 12, 9, 6,
	3, 0,
};


const u8 v19_offset_table[24] = {
	101, 94, 87, 80,
	74, 68, 62, 56,
	51, 46, 41, 36,
	32, 28, 24, 20,
	17, 14, 11, 8,
	6, 4, 2, 0,
};


const u8 range_table_count[IV_TABLE_MAX] = {
	1, 18, 24, 44, 84, 84, 1,
};


const u32 table_radio[IV_TABLE_MAX] = {
	0, 405, 303, 745, 390, 390, 0,
};


const u32 dv_value[IV_MAX] = {
	0, 19, 43, 87, 171, 255,
};


const char color_name[3] = {'R', 'G', 'B'};


const u8 *offset_table[IV_TABLE_MAX] = {
	NULL,
	v1_offset_table,
	v19_offset_table,
	NULL,
	NULL,
	NULL,
	NULL,
};
static const unsigned char gamma_300cd[] = {
	0x18, 0x08, 0x24, 0x6B, 0x76, 0x57,
	0xBD, 0xC3, 0xB5, 0xB4, 0xBB, 0xAC,
	0xC5, 0xC9, 0xC0, 0x00, 0xB7, 0x00,
	0xAB, 0x00, 0xCF,
};

static s16 s9_to_s16(s16 v)
{
	return (s16)(v << 7) >> 7;
}


u32 calc_v1_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

	ret = volt_table_v1[gamma] >> 10;

	return ret;
}


u32 calc_v19_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 19, DV :320 */
	int ret = 0;
	u32 v1, v43;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v43 = adjust_volt[rgb_index][AD_IV43];

	if(gamma >= V19_VOLTAGE_COUNT) {
		printk("[SMART:ERROR] : %s Exceed Max Gamma Value (%d)", __func__, gamma);
		gamma = V19_VOLTAGE_COUNT - 1;
	}
	ratio = volt_table_cv_19_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v43)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v43_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 43, DV :320 */
	int ret = 0;
	u32 v1, v87;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v87 = adjust_volt[rgb_index][AD_IV87];
	if(gamma >= V43_VOLTAGE_COUNT) {
		printk("[SMART:ERROR] : %s Exceed Max Gamma Value (%d)",__func__,gamma);
		gamma = V43_VOLTAGE_COUNT - 1;
	}
	ratio = volt_table_cv_43_dv_320[gamma];

	ret = (v1 << 10) - ((v1 - v87) * ratio);
	ret = ret >> 10;

	return ret;
}

u32 calc_v87_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 87, DV :320 */
	int ret = 0;
	u32 v1, v171;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v171 = adjust_volt[rgb_index][AD_IV255];
	if(gamma >= V87_VOLTAGE_COUNT) {
		printk("[SMART:ERROR] : %s Exceed Max Gamma Value (%d)", __func__, gamma);
		gamma = V87_VOLTAGE_COUNT - 1;
	}

	ratio = volt_table_cv_87_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v171)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v171_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 171, DV :320 */
	int ret = 0;
	u32 v1, v255;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v255 = adjust_volt[rgb_index][AD_IV255];
	if(gamma >= V171_VOLTAGE_COUNT) {
		printk("[SMART:ERROR] : %s Exceed Max Gamma Value (%d)", __func__, gamma);
		gamma = V171_VOLTAGE_COUNT - 1;
	}
	ratio = volt_table_cv_171_dv_320[gamma];

	ret = (v1 << 10) - ((v1 - v255) * ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v255_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

	if(gamma >= V255_VOLTAGE_COUNT) {
		printk("[SMART:ERROR] : %s Exceed Max Gamma Value (%d)", __func__, gamma);
		gamma = V255_VOLTAGE_COUNT - 1;
	}
	ret = volt_table_v255[gamma] >> 10;

	return ret;
}


u8 calc_voltage_table(struct str_smart_dim *smart, const u8 *mtp)
{
	int c, i, j;
#if defined(MTP_REVERSE)
	int offset1 = 0;
#endif
	int offset = 0;
	s16 t1, t2;
	s16 adjust_mtp[CI_MAX][IV_MAX];
	/* u32 adjust_volt[CI_MAX][AD_IVMAX] = {0, }; */
	u8 range_index;
	u8 table_index = 0;

	u32 v1, v2;
	u32 ratio;

	u32(*calc_volt[IV_MAX])(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_v1_volt,
		calc_v19_volt,
		calc_v43_volt,
		calc_v87_volt,
		calc_v171_volt,
		calc_v255_volt,
	};

	u8 calc_seq[5] = {IV_1, IV_171, IV_87, IV_43, IV_19};
	u8 ad_seq[5] = {AD_IV1, AD_IV171, AD_IV87, AD_IV43, AD_IV19};

	/*
	s16 (*mtp_data)[IV_MAX];
	u32 (*adjust_volt)[AD_IVMAX];
	struct str_voltage_entry *ve;

	mtp_data = smart->mtp;
	adjust_volt = smart->adjust_volt;
	ve = smart->ve;
	*/

	memset(adjust_mtp, 0, sizeof(adjust_mtp));

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255 * CI_MAX + (c * 2);
#if defined(MTP_REVERSE)
		offset1 = c * 7 + 5;
		t1 = s9_to_s16(mtp[offset1] << 8|mtp[offset1 + 1]);
#else
		t1 = s9_to_s16(mtp[offset] << 8|mtp[offset + 1]);
#endif
		t2 = s9_to_s16(smart->default_gamma[offset] << 8|
			smart->default_gamma[offset + 1]) + t1;
		smart->mtp[c][IV_255] = t1;
		adjust_mtp[c][IV_255] = t2;
		smart->adjust_volt[c][AD_IV255] = calc_volt[IV_255](t2, c, smart->adjust_volt);

		/* for V0 All RGB Voltage Value is Reference Voltage */
		smart->adjust_volt[c][AD_IV0] = VREG_OUT_1000;
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
#if defined(MTP_REVERSE)
			t1 = (s8)mtp[(calc_seq[i]) + (c * 7)];
#else
			t1 = (s8)mtp[CI_MAX * calc_seq[i] + c];
#endif
			t2 = smart->default_gamma[CI_MAX * calc_seq[i] + c] + t1;

			smart->mtp[c][calc_seq[i]] = t1;
			adjust_mtp[c][calc_seq[i]] = t2;
			smart->adjust_volt[c][ad_seq[i]] = calc_volt[calc_seq[i]](t2, c, smart->adjust_volt);
		}
	}

	for (i = 0; i < AD_IVMAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			smart->ve[table_index].v[c] = smart->adjust_volt[c][i];

		range_index = 0;
		for (j = table_index + 1; j < table_index + range_table_count[i]; j++) {
			for (c = CI_RED; c < CI_MAX; c++) {
				if (smart->t_info[i].offset_table != NULL)
					ratio = smart->t_info[i].offset_table[range_index] * smart->t_info[i].rv;
				else
					ratio = (range_table_count[i]-(range_index + 1)) * smart->t_info[i].rv;

				v1 = smart->adjust_volt[c][i + 1] << 15;
				v2 = (smart->adjust_volt[c][i] - smart->adjust_volt[c][i + 1]) * ratio;
				smart->ve[j].v[c] = ((v1 + v2) >> 15);
			}
			range_index++;
		}
		table_index = j;
	}

#if 1
	printk(KERN_INFO "++++++++++++++++++++++++++++++ MTP VALUE ++++++++++++++++++++++++++++++\n");
	for (i = IV_1; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : 0x%08x(%04d)", color_name[c], smart->mtp[c][i], smart->mtp[c][i]);
		printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ ADJUST VALUE ++++++++++++++++++++++++++++++\n");
	for (i = IV_1; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : 0x%08x(%04d)", color_name[c],
				adjust_mtp[c][i], adjust_mtp[c][i]);
		printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ ADJUST VOLTAGE ++++++++++++++++++++++++++++++\n");
	for (i = AD_IV0; i < AD_IVMAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : %04dV", color_name[c], smart->adjust_volt[c][i]);
		printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++++++++++  VOLTAGE TABLE ++++++++++++++++++++++++++++++++++++++\n");
	for (i = 0; i < 256; i++) {
		printk("Gray Level : %03d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : %04dV", color_name[c], smart->ve[i].v[c]);
		printk("\n");
	}
#endif
	return 0;
}


int init_table_info(struct str_smart_dim *smart)
{
	int i;
	int offset = 0;

	for (i = 0; i < IV_TABLE_MAX; i++) {
		smart->t_info[i].count = (u8)range_table_count[i];
		smart->t_info[i].offset_table = offset_table[i];
		smart->t_info[i].rv = table_radio[i];
		offset += range_table_count[i];
	}
	smart->flooktbl = flookup_table;
	smart->g300_gra_tbl = gamma_300_gra_table;
	smart->g22_tbl = gamma_22_table;

	smart->default_gamma = gamma_300cd;

#if 1
	printk(" jbass.choi : %s\n", __func__);
	for (i = 0; i <  (IV_TABLE_MAX * 3); i++)
		printk(KERN_INFO "%d : %x", i, smart->default_gamma[i]);
	printk("\n");
#endif

	return 0;
}


u32 lookup_vtbl_idx(struct str_smart_dim *smart, u32 gamma)
{
	u32 lookup_index;
	u16 table_count, table_index;
	u32 gap, i;
	u32 minimum = smart->g300_gra_tbl[255];
	u32 candidate = 0;
	u32 offset = 0;

	/* printk("Input Gamma Value : %d\n", gamma); */

	lookup_index = (gamma >> 10) + 1;
	if (lookup_index > MAX_GRADATION) {
		printk(KERN_ERR "ERROR Wrong input value  LOOKUP INDEX : %d\n", lookup_index);
		lookup_index = MAX_GRADATION - 1;
		/*return 0;*/
	}

	/* printk("lookup index : %d\n",lookup_index); */

	if (smart->flooktbl[lookup_index].count) {
		if (smart->flooktbl[lookup_index - 1].count) {
			table_index = smart->flooktbl[lookup_index - 1].entry;
			table_count = smart->flooktbl[lookup_index].count + smart->flooktbl[lookup_index - 1].count;
		} else {
			table_index = smart->flooktbl[lookup_index].entry;
			table_count = smart->flooktbl[lookup_index].count;
		}
	} else {
		offset += 1;
		while (!(smart->flooktbl[lookup_index + offset].count || smart->flooktbl[lookup_index - offset].count))
			offset++;

		if (smart->flooktbl[lookup_index-offset].count)
			table_index = smart->flooktbl[lookup_index - offset].entry;
		else
			table_index = smart->flooktbl[lookup_index + offset].entry;
		table_count = smart->flooktbl[lookup_index + offset].count + smart->flooktbl[lookup_index - offset].count;
	}


	for (i = 0; i < table_count; i++) {
		if (gamma > smart->g300_gra_tbl[table_index])
			gap = gamma - smart->g300_gra_tbl[table_index];
		else
			gap = smart->g300_gra_tbl[table_index] - gamma;

		if (gap == 0) {
			candidate = table_index;
			break;
		}

		if (gap < minimum) {
			minimum = gap;
			candidate = table_index;
		}
		table_index++;
	}
#if 0
	printk(KERN_INFO "cal : found index  : %d\n", candidate);
	printk(KERN_INFO "gamma : %d, found index : %d found gamma : %d\n",
		gamma, candidate, smart->g300_gra_tbl[candidate]);
#endif
	return candidate;
}


u32 calc_v1_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 ret;
	u32 v1;

	v1 = dv[ci][IV_1];
	ret = (595 << 10) - (143 * v1);
	ret = ret >> 10;

	return ret;
}


u32 calc_v19_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v19, v43;
	u32 ret;

	v1 = dv[ci][IV_1];
	v19 = dv[ci][IV_19];
	v43 = dv[ci][IV_43];

#if 0
	t1 = (v1 - v19) * 1000;
	t2 = v1 - v43;

	ret = 320 * (t1 / t2)-(20 * 1000);

	ret = ret / 1000;
#else
	t1 = (v1 - v19) << 10;
	t2 = (v1 - v43) ? (v1 - v43) : (v1) ? v1 : 1;
	ret = (320 * (t1 / t2)) - (65 << 10);
	ret >>= 10;

#endif
	return ret;
}


u32 calc_v43_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v43, v87;
	u32 ret;

	v1 = dv[ci][IV_1];
	v43 = dv[ci][IV_43];
	v87 = dv[ci][IV_87];

#if 0
	t1 = (v1 - v43) * 1000;
	t2 = v1 - v57;
	ret = 320 * (t1/t2) - (65 * 1000);

	ret = ret / 1000;
#else
	t1 = (v1 - v43) << 10;
	t2 = (v1 - v87) ? (v1 - v87) : (v1) ? v1 : 1;
	ret = (320 * (t1 / t2)) - (65 << 10);

	ret >>= 10;
#endif

	return ret;
}

u32 calc_v87_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v87, v171;
	u32 ret;

	v1 = dv[ci][IV_1];
	v87 = dv[ci][IV_87];
	v171 = dv[ci][IV_171];

#if 0
	t1 = (v1 - v87) * 1000;
	t2 = v1 - v171;
	ret = 320 * (t1 / t2) - (65 * 1000);
	ret = ret / 1000;
#else
	t1 = (v1 - v87) << 10;
	t2 = (v1 - v171) ? (v1 - v171) : (v1) ? v1 : 1;
	ret = (320 * (t1 / t2)) - (65 << 10);
	ret >>= 10;
#endif

	return ret;
}


u32 calc_v171_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v171, v255;
	u32 ret;

	v1 = dv[ci][IV_1];
	v171 = dv[ci][IV_171];
	v255 = dv[ci][IV_255];

#if 0
	t1 = (v1 - v171) * 1000;
	t2 = v1 - v255;
	ret = 320 * (t1/t2) - (65 * 1000);
	ret = ret / 1000;
#else
	t1 = (v1 - v171) << 10;
	t2 = (v1 - v255) ? (v1 - v255) : (v1) ? v1 : 1;
	ret = (320 * (t1 / t2)) - (65 << 10);
	ret >>= 10;
#endif

	return ret;
}

u32 calc_v255_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 ret;
	u32 v255;

	v255 = dv[ci][IV_255];

	ret = (480 << 10) - (143 * v255);
	ret = ret >> 10;

	return ret;
}

u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[])
{
	u32 i, c;
	u32 temp;
	u32 lidx;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX]) = {
		calc_v1_reg,
		calc_v19_reg,
		calc_v43_reg,
		calc_v87_reg,
		calc_v171_reg,
		calc_v255_reg,
	};
	/*
	s16 (*mtp_data)[IV_MAX];
	u32 (*adjust_volt)[AD_IVMAX];
	struct str_voltage_entry *ve;
	mtp_data= smart->mtp;
	adjust_volt = smart->adjust_volt;
	ve = smart->ve;
	*/

	memset(gamma, 0, sizeof(gamma));

	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][0] = smart->adjust_volt[c][AD_IV1];


	for (i = IV_19; i < IV_MAX; i++) {
		temp = (smart->g22_tbl[dv_value[i]]) * gv;
		lidx = lookup_vtbl_idx(smart, temp);
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx].v[c];
	}

	/* for IV1 does not calculate value */
	/* just use default gamma value (IV1) */
	for (c = CI_RED; c < CI_MAX; c++)
		gamma[c][IV_1] = smart->default_gamma[c];

	for (i = IV_19; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma[c][i] = (s16)calc_reg[i](c, dv) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255 * CI_MAX + (c * 2);
		result[offset + 1] = gamma[c][IV_255];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++)
			result[(CI_MAX * i) + c] = gamma[c][i];
	}

#if 1
	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND VOLTAGE ++++++++++++++++++++++++++++++\n");
	for (i = IV_1; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %04dV", color_name[c], dv[c][i]);
		printk("\n");
	}


	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND REG ++++++++++++++++++++++++++++++\n");
	for (i = IV_1; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %3d, 0x%2x", color_name[c], gamma[c][i], gamma[c][i]);
		printk("\n");
	}
#endif
	return 0;
}
