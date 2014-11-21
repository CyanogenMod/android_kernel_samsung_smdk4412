/* linux/drivers/video/samsung/smartdimming.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Samsung Smart Dimming for OCTA
 *
 * Minwoo Kim, <minwoo7945.kim@samsung.com>
 *
*/


#include "smart_dimming_ld9042.h"
#include "ld9042_volt_tbl.h"

/*#define MTP_REVERSE */
#define V255_MTP_OFFSET	4
#define V255_GAMMA_OFFSET	5
#define VALUE_DIM_1000	1000


const u8 v1_offset_table[17] = {
	75, 69, 63, 57,
	51, 46, 41, 36,
	31, 27, 23, 19,
	15, 12, 9, 6,
	3,
};

const u8 v19_offset_table[23] = {
	101, 94, 87, 80,
	74, 68, 62, 56,
	51, 46, 41, 36,
	32, 28, 24, 20,
	17, 14, 11, 8,
	6, 4, 2,
};


const u8 range_table_count[IV_TABLE_MAX] = {
	1, 18, 24, 44, 84, 84, 1
};


const u32 table_radio[IV_TABLE_MAX] = {
	0, 404, 303, 745, 390, 390, 0
};


const u32 dv_value[IV_MAX] = {
	0, 19, 43, 87, 171, 255
};


const char color_name[3] = {'R', 'G', 'B'};


const u8 *offset_table[IV_TABLE_MAX] = {
	NULL,
	v1_offset_table,
	v19_offset_table,
	NULL,
	NULL,
	NULL,
	NULL
};

const unsigned char gamma_300cd_a2[] = {
	0x0C, 0xA9, 0xAF, 0xA9, 0xBC, 0x00, 0xAA,
	0x0C, 0xAB, 0xAE, 0xA6, 0xBB, 0x00, 0xC8,
	0x0C, 0xB5, 0xB1, 0xA7, 0xBC, 0x00, 0xCC,
};

const unsigned char *gamma_300cd_list[GAMMA_300CD_MAX] = {
	gamma_300cd_a2,
};

const unsigned char gamma_id_list[GAMMA_300CD_MAX] = {
	0xa2
};

u32 calc_v1_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

	ret = volt_table_v1[gamma] >> 10;

	return ret;
}


u32 calc_v19_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :320 */
	int ret = 0;
	u32 v1, v43;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v43 = adjust_volt[rgb_index][AD_IV43];
	ratio = volt_table_cv_65_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v43)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v43_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :320 */
	int ret = 0;
	u32 v1, v87;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v87 = adjust_volt[rgb_index][AD_IV87];
	ratio = volt_table_cv_65_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v87)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v87_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :320 */
	int ret = 0;
	u32 v1, v171;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v171 = adjust_volt[rgb_index][AD_IV171];
	ratio = volt_table_cv_65_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v171)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v171_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :320 */
	int ret = 0;
	u32 v1, v255;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v255 = adjust_volt[rgb_index][AD_IV255];
	ratio = volt_table_cv_65_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v255)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v255_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

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

	u8 calc_seq[6] = {IV_1, IV_171, IV_87, IV_43, IV_19};
	u8 ad_seq[6] = {AD_IV1, AD_IV171, AD_IV87, AD_IV43, AD_IV19};

	memset(adjust_mtp, 0, sizeof(adjust_mtp));

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = ((c + 1) * V255_MTP_OFFSET) + (c * 2);
		/* printk("1 offset : %d\n", offset); */

		if (mtp[offset] & 0x01)
			t1 = mtp[offset + 1] * -1;
		else
			t1 = mtp[offset + 1];
		/* printk("2 t1 : %d\n", t1); */

		offset = ((c+1) * V255_GAMMA_OFFSET) + (c * 2);

		t2 = smart->default_gamma[offset] << 8 | smart->default_gamma[offset+1];
		/* printk("3 t2 : %d\n", t2); */

		t2 += t1;
		/* printk("4 t2+t1 : %d\n", t2); */

		smart->mtp[c][IV_255] = t1;
		adjust_mtp[c][IV_255] = t2;
		smart->adjust_volt[c][AD_IV255] = calc_volt[IV_255](t2, c, smart->adjust_volt);

		/* for V0 All RGB Voltage Value is Reference Voltage */
		smart->adjust_volt[c][AD_IV0] = 4320;
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
			if (calc_seq[i] == IV_1)
				t1 = 0;
			else {
				offset = (c * 6) + (calc_seq[i]-1);
				/* printk("21 offset : %d\n", offset); */
				if (mtp[offset] & 0x80)
					t1 = (mtp[offset] & 0x7f) * (-1);
				else
					t1 = (mtp[offset] & 0x7f);
			}
			/* printk("22 t1 : %d\n", t1); */

			offset = (c * 7) + (calc_seq[i]);
			/* printk("23 offset : %d\n", offset); */

			t2 = smart->default_gamma[offset];
			/* printk("23 t2 : %d\n", t2); */

			t2 += t1;
			/* printk("24 t2+t1 : %d\n", t2); */

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
					ratio = (range_table_count[i]-(range_index+1)) * smart->t_info[i].rv;

				v1 = smart->adjust_volt[c][i+1] << 15;
				v2 = (smart->adjust_volt[c][i] - smart->adjust_volt[c][i+1])*ratio;
				smart->ve[j].v[c] = ((v1+v2) >> 15);
			}
			range_index++;
		}
		table_index = j;
	}

#if 0
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


int init_table_info_22(struct str_smart_dim *smart)
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

	for (i = 0; i < GAMMA_300CD_MAX; i++) {
		if (smart->panelid[0] == gamma_id_list[i])
			break;
	}

	if (i >= GAMMA_300CD_MAX) {
		printk(KERN_ERR "[SMART DIMMING-WARNING] %s Can't found default gamma table\n", __func__);
		smart->default_gamma = gamma_300cd_list[GAMMA_300CD_MAX-1];
	} else
		smart->default_gamma = gamma_300cd_list[i];

#if 0
	for (i = 0; i < 24; i++)
		printk(KERN_INFO "Index : %d : %x\n", i, smart->default_gamma[i]);
#endif

	return 0;
}

int init_table_info_19(struct str_smart_dim *smart)
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
	smart->g19_tbl = gamma_19_table;

	for (i = 0; i < GAMMA_300CD_MAX; i++) {
		if (smart->panelid[0] == gamma_id_list[i])
			break;
	}

	if (i >= GAMMA_300CD_MAX) {
		printk(KERN_ERR "[SMART DIMMING-WARNING] %s Can't found default gamma table\n", __func__);
		smart->default_gamma = gamma_300cd_list[GAMMA_300CD_MAX-1];
	} else
		smart->default_gamma = gamma_300cd_list[i];

#if 0
	for (i = 0; i < 24; i++)
		printk(KERN_INFO "Index : %d : %x\n", i, smart->default_gamma[i]);
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

	lookup_index = (gamma/1000) + 1;
	if (lookup_index > MAX_GRADATION) {
		printk(KERN_ERR "ERROR Wrong input value  LOOKUP INDEX : %d\n", lookup_index);
		lookup_index = MAX_GRADATION - 1;
	}

	/* printk("lookup index : %d\n",lookup_index); */

	if (smart->flooktbl[lookup_index].count) {
		if (smart->flooktbl[lookup_index-1].count) {
			table_index = smart->flooktbl[lookup_index-1].entry;
			table_count = smart->flooktbl[lookup_index].count + smart->flooktbl[lookup_index-1].count;
		} else {
			table_index = smart->flooktbl[lookup_index].entry;
			table_count = smart->flooktbl[lookup_index].count;
		}
	} else {
		offset += 1;
		while (!(smart->flooktbl[lookup_index+offset].count || smart->flooktbl[lookup_index-offset].count))
			offset++;

		if (smart->flooktbl[lookup_index-offset].count)
			table_index = smart->flooktbl[lookup_index-offset].entry;
		else
			table_index = smart->flooktbl[lookup_index+offset].entry;
		table_count = smart->flooktbl[lookup_index+offset].count + smart->flooktbl[lookup_index-offset].count;
	}


	for (i = 0; i < table_count; i++) {
		if (gamma > smart->g300_gra_tbl[table_index])
			gap = gamma - smart->g300_gra_tbl[table_index];
		else
			gap = smart->g300_gra_tbl[table_index] - gamma;

		/* printk("gap : %d\n", gap); */

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

	return candidate;
}


u32 calc_v1_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 ret;
	u32 v1;

	v1 = dv[ci][IV_1];
	ret = (595 * 1000) - (139 * v1);
	ret = ret/1000;

	/* printk("%s v1 value : %d, ret : %d\n", __func__, v1, ret); */

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

	t1 = (v1 - v19) << 10;
	t2 = (v1 - v43) ? (v1 - v43) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
	ret >>= 10;

	/* printk("%s v1 : %d,v19 : %d, v43 : %d ret : %d\n", __func__, v1, v19, v43, ret); */

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

	t1 = (v1 - v43) << 10;
	t2 = (v1 - v87) ? (v1 - v87) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
	ret >>= 10;

	/* printk("%s v43 : %d, ret : %d\n", __func__, v43, ret); */

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

	t1 = (v1 - v87) << 10;
	t2 = (v1 - v171) ? (v1 - v171) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
	ret >>= 10;

	/* printk("%s v87 : %d, ret : %d\n", __func__, v87, ret); */

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

	t1 = (v1 - v171) << 10;
	t2 = (v1 - v255) ? (v1 - v255) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
	ret >>= 10;

	/* printk("%s v171 : %d, ret : %d\n", __func__, v171, ret); */

	return ret;
}


u32 calc_v255_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 ret;
	u32 v255;

	v255 = dv[ci][IV_255];

	ret = (480 * 1000) - (139 * v255);
	ret = ret / 1000;

	/* printk("%s v255 : %d, ret : %d\n",__func__, v255, ret); */

	return ret;
}


u32 calc_gamma_table_22(struct str_smart_dim *smart, u32 gv, u8 result[])
{
	u32 i, c;
	u32 temp;
	u32 lidx_22;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma_22[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX]) = {
		calc_v1_reg,
		calc_v19_reg,
		calc_v43_reg,
		calc_v87_reg,
		calc_v171_reg,
		calc_v255_reg,
	};

	/* printk("%s was call gv : %d\n", __func__, gv);*/
	memset(gamma_22, 0, sizeof(gamma_22));

#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][0] = smart->adjust_volt[c][AD_IV1];
#endif

	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][IV_1] = smart->ve[AD_IV1].v[c];

	for (i = IV_19; i < IV_MAX; i++) {
		temp = (smart->g22_tbl[dv_value[i]] * gv)/1000;
		/* printk("temp : %d, g22 val : %d, gv : %d\n",temp,smart->g22_tbl[dv_value[i]], gv); */
		lidx_22 = lookup_vtbl_idx(smart, temp);
		/* printk("look index : %d\n", lidx_22); */
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx_22].v[c];
	}

	for (i = IV_1; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma_22[c][i] = (u16)calc_reg[i](c, dv) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = ((c+1) * V255_GAMMA_OFFSET) + (c * 2);
		result[offset+1] = gamma_22[c][IV_255] & 0xff;
		result[offset] = (u8)((gamma_22[c][IV_255] >> 8) & 0xff);
		/* printk("%s array index is result[%d+1] V255 print gamma : %d\n", __func__, offset, result[offset+1]); */
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
			if (i < IV_255) {
				offset = (c*7)+i;
				result[offset] = gamma_22[c][i];
				/* printk("%s array index is result[%d] V1~v171  print gamma : %d\n", __func__, offset, result[offset]); */
			}
			/*
			else if (i == IV_255) {
				offset = ((c + 1) * V255_GAMMA_OFFSET) + (c * 2);
				 result[offset]=0;
				 printk("%s array index is result[%d] V255 print gamma : %d\n", __func__, offset, result[offset]);
			}
			*/
		}
	}

#if 0
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
			printk("2.2Gamma %c : %3d, 0x%2x", color_name[c], gamma_22[c][i], gamma_22[c][i]);
		printk("\n");
	}
#endif
	return 0;
}


u32 calc_gamma_table_19(struct str_smart_dim *smart, u32 gv, u8 result[])
{
	u32 i, c;
	u32 temp;
	u32 lidx_19;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma_19[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX]) = {
		calc_v1_reg,
		calc_v19_reg,
		calc_v43_reg,
		calc_v87_reg,
		calc_v171_reg,
		calc_v255_reg,
	};

	/* printk("%s was call gv : %d\n", __func__, gv);*/
	memset(gamma_19, 0, sizeof(gamma_19));

#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][0] = smart->adjust_volt[c][AD_IV1];
#endif

	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][IV_1] = smart->ve[AD_IV1].v[c];


	for (i = IV_19; i < IV_MAX; i++) {
		temp = (smart->g19_tbl[dv_value[i]] * gv)/1000;
		/* printk("temp : %d, g19 val : %d, gv : %d\n",temp,smart->g19_tbl[dv_value[i]], gv); */
		lidx_19 = lookup_vtbl_idx(smart, temp);
		/* printk("look index : %d\n",lidx_19); */
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx_19].v[c];
	}

	/* for IV1 does not calculate value */
	/* just use default gamma value (IV1) */
#if  0
	for (c = CI_RED; c < CI_MAX; c++)
		gamma[c][IV_1] = smart->default_gamma[c];
#endif

	for (i = IV_1; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma_19[c][i] = (u16)calc_reg[i](c, dv) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = ((c+1) * V255_GAMMA_OFFSET) + (c * 2);
		result[offset+1] = gamma_19[c][IV_255] & 0xff;
		result[offset] = (u8)((gamma_19[c][IV_255] >> 8) & 0xff);
		/* printk("%s array index is result[%d+1] V255 print gamma : %d\n", __func__, offset, result[offset+1]); */
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
			if (i < IV_255) {
				offset = (c*7)+i;
				result[offset] = gamma_19[c][i];
				/* printk("%s array index is result[%d] V1~v171  print gamma : %d\n", __func__, offset, result[offset]); */
			}
			/*
			else if (i == IV_255) {
				offset = ((c + 1) * V255_GAMMA_OFFSET) + (c * 2);
				 result[offset]=0;
				 printk("%s array index is result[%d] V255 print gamma : %d\n", __func__, offset, result[offset]);
			}
			*/
		}
	}

#if 0
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
			printk("1.9Gamma %c : %3d, 0x%2x", color_name[c], gamma_19[c][i], gamma_19[c][i]);
		printk("\n");
	}
#endif
	return 0;
}

