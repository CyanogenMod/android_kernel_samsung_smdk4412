/* linux/drivers/video/samsung/smartdimming.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Samsung Smart Dimming for OCTA
 *
 * Minwoo Kim, <minwoo7945.kim@samsung.com>
 *
*/

#include "smart_dimming_ea8061.h"
#include "ea8061_volt_tbl.h"

#define VALUE_DIM_1000	1000
#define VT_255_div_1000	605000
#define VREG_OUT_1000		6100
#define VREG_OUT_1000_1024	6246400
#define VT_255_calc_param	(VT_255_div_1000 / VREG_OUT_1000)

static const u8 range_table_count[IV_TABLE_MAX] = {
	3, 8, 12, 12, 16, 36, 64, 52, 52, 1
};

static const u32 table_radio[IV_TABLE_MAX] = {
	16384, 4138, 2763, 2763, 2080, 918, 516, 636, 636, 0
};

static const u32 dv_value[IV_MAX] = {
	0, 3, 11, 23, 35, 51, 87, 151, 203, 255
};

static const char color_name[3] = {'R', 'G', 'B'};

static const u8 *offset_table[IV_TABLE_MAX] = {
	NULL, /*V3*/
	NULL, /*V11*/
	NULL, /*V23*/
	NULL, /*V23*/
	NULL, /*V35*/
	NULL, /*V51*/
	NULL, /*V87*/
	NULL, /*V151*/
	NULL, /*V203*/
	NULL  /*V255*/
};

static const unsigned char gamma_300cd_00[] = {
	0x00, 0xE8, 0x00, 0xF7, 0x01, 0x03,
	0xDB, 0xDB, 0xDC, 0xD9, 0xD8, 0xDA, 0xCB, 0xC8, 0xCB,
	0xD4, 0xD3, 0xD7, 0xE6, 0xE6, 0xEA, 0xE2, 0xE4, 0xE5,
	0xCE, 0xC3, 0xCF, 0xB9, 0x9D, 0xDE, 0x01, 0x01, 0x00
};

static const unsigned char gamma_300cd_02[] = {
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x03, 0x02
};

static const unsigned char *gamma_300cd_list[GAMMA_300CD_MAX] = {
	gamma_300cd_00,
	gamma_300cd_00,
	gamma_300cd_02,
	gamma_300cd_02,
	gamma_300cd_02,
	gamma_300cd_02,
	gamma_300cd_02,
	gamma_300cd_02,
	gamma_300cd_02,
};

static const unsigned char gamma_id_list[GAMMA_300CD_MAX] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x25, 0x26,
};

static s16 s9_to_s16(s16 v)
{
	return (s16)(v << 7) >> 7;
}

static u32 calc_vt_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

	ret = volt_table_vt[gamma] >> 16;

	return ret;
}

static u32 calc_v3_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :320 */
	int ret = 0;
	u32 v0 = VREG_OUT_1000, v11;
	u32 ratio = 0;

	/*vt = adjust_volt[rgb_index][AD_IVT];*/
	v11 = adjust_volt[rgb_index][AD_IV11];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (v0 << 16) - ((v0-v11)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v11_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :320*/
	int ret = 0;
	u32 vt, v23;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v23 = adjust_volt[rgb_index][AD_IV23];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v23)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v23_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :319 */
	int ret = 0;
	u32 vt, v35;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v35 = adjust_volt[rgb_index][AD_IV35];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v35)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v35_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :319 */
	int ret = 0;
	u32 vt, v51;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v51 = adjust_volt[rgb_index][AD_IV51];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v51)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v51_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :319 */
	int ret = 0;
	u32 vt, v87;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v87 = adjust_volt[rgb_index][AD_IV87];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v87)*ratio);
	ret = ret >> 16;

	return ret;

}

static u32 calc_v87_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :319 */
	int ret = 0;
	u32 vt, v151;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v151 = adjust_volt[rgb_index][AD_IV151];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v151)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v151_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :319 */
	int ret = 0;
	u32 vt, v203;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v203 = adjust_volt[rgb_index][AD_IV203];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v203)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v203_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 64, DV :319 */
	int ret = 0;
	u32 vt, v255;
	u32 ratio = 0;

	vt = adjust_volt[rgb_index][AD_IVT];
	v255 = adjust_volt[rgb_index][AD_IV255];
	ratio = volt_table_cv_64_dv_320[gamma];

	ret = (vt << 16) - ((vt-v255)*ratio);
	ret = ret >> 16;

	return ret;
}

static u32 calc_v255_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret = 0;

	ret = volt_table_v255[gamma] >> 16;

	return ret;
}

u8 calc_voltage_table_ea8061(struct str_smart_dim *smart, const u8 *mtp)
{
	int c, i, j;
#if defined(MTP_REVERSE)
	int offset1 = 0;
#endif
	int offset = 0;
	s16 t1, t2;
	s16 adjust_mtp[CI_MAX][IV_MAX];
	u8 range_index;
	u8 table_index = 0;
	u32 v1, v2;
	u32 ratio;
	u32(*calc_volt[IV_MAX])(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_vt_volt,
		calc_v3_volt,
		calc_v11_volt,
		calc_v23_volt,
		calc_v35_volt,
		calc_v51_volt,
		calc_v87_volt,
		calc_v151_volt,
		calc_v203_volt,
		calc_v255_volt,
	};
	u8 calc_seq[9] = { IV_VT,  IV_203, IV_151, IV_87, IV_51, IV_35, IV_23, IV_11, IV_3};
	u8 ad_seq[9] = {AD_IVT, AD_IV203, AD_IV151, AD_IV87, AD_IV51, AD_IV35, AD_IV23, AD_IV11, AD_IV3};

	memset(adjust_mtp, 0, sizeof(adjust_mtp));

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = c*2;
		t1 = s9_to_s16(mtp[offset]<<8|mtp[offset+1]);
		t2 = (smart->default_gamma[offset]<<8|smart->default_gamma[offset+1]) + t1;
		smart->mtp[c][IV_255] = t1;
		adjust_mtp[c][IV_255] = t2;
		smart->adjust_volt[c][AD_IV255] = calc_volt[IV_255](t2, c, smart->adjust_volt);
		/* for V0 All RGB Voltage Value is Reference Voltage */
		smart->adjust_volt[c][AD_IVT] = VREG_OUT_1000;
	}

	for (i = IV_VT; i < IV_255; i++) {
		for (c = CI_RED; c < CI_MAX; c++) {
			if (i < IV_3) {
				t1 = 0;
				t2 = smart->default_gamma[CI_MAX*(10-calc_seq[i])+c] + t1;
				smart->mtp[c][calc_seq[i]] = t1;
				adjust_mtp[c][calc_seq[i]] = t2;
				smart->adjust_volt[c][ad_seq[i]] = calc_volt[calc_seq[i]](t2, c, smart->adjust_volt);
			} else {
				t1 = (s8)mtp[CI_MAX*(10-calc_seq[i])+c];
				t2 = smart->default_gamma[CI_MAX*(10-calc_seq[i])+c] + t1;
				smart->mtp[c][calc_seq[i]] = t1;
				adjust_mtp[c][calc_seq[i]] = t2;
				smart->adjust_volt[c][ad_seq[i]] = calc_volt[calc_seq[i]](t2, c, smart->adjust_volt);
			}
		}
	}

	for (i = AD_IVT; i < AD_IVMAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++) {
			if (i == 0)
				smart->ve[table_index].v[c] = VREG_OUT_1000;
			else
				smart->ve[table_index].v[c] = smart->adjust_volt[c][i];
		}
		range_index = 0;

		 for (j = table_index+1; j < table_index+range_table_count[i]; j++) {
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

	for (i = 1; i < 3; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			smart->ve[i].v[c] = smart->ve[3].v[c]+((smart->ve[0].v[c]-smart->ve[3].v[c])*(3-i)/3);
	}

#if 0
	printk(KERN_INFO "++++++++++++++++++++++++++++++ MTP VALUE ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_MAX; i++) {
			printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
				printk("  %c : 0x%08x(%04d)", color_name[c], smart->mtp[c][i], smart->mtp[c][i]);
			printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ ADJUST VALUE ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_MAX; i++) {
			printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
				printk("  %c : 0x%08x(%04d)", color_name[c],
				adjust_mtp[c][i], adjust_mtp[c][i]);
			printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ ADJUST VOLTAGE ++++++++++++++++++++++++++++++\n");
	for (i = AD_IVT; i < AD_IVMAX; i++) {
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


int init_table_info_ea8061(struct str_smart_dim *smart)
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
	smart->gamma_table[G_21] = GAMMA_CONTROL_TABLE[G_21];
	smart->gamma_table[G_213] = GAMMA_CONTROL_TABLE[G_213];
	smart->gamma_table[G_215] = GAMMA_CONTROL_TABLE[G_215];
	smart->gamma_table[G_218] = GAMMA_CONTROL_TABLE[G_218];
	smart->gamma_table[G_22] = GAMMA_CONTROL_TABLE[G_22];
	smart->gamma_table[G_221] = GAMMA_CONTROL_TABLE[G_221];
	smart->gamma_table[G_222] = GAMMA_CONTROL_TABLE[G_222];
	smart->gamma_table[G_223] = GAMMA_CONTROL_TABLE[G_223];
	smart->gamma_table[G_224] = GAMMA_CONTROL_TABLE[G_224];
	smart->gamma_table[G_225] = GAMMA_CONTROL_TABLE[G_225];

	for (i = 0; i < GAMMA_300CD_MAX; i++) {
		if (smart->panelid[2] == gamma_id_list[i])
			break;
	}

	if (i >= GAMMA_300CD_MAX) {
		printk(KERN_ERR "[SMART DIMMING-WARNING] %s Can't found default gamma table\n", __func__);
		smart->default_gamma = gamma_300cd_list[GAMMA_300CD_MAX-1];
	} else
		smart->default_gamma = gamma_300cd_list[i];

	return 0;
}

static u32 lookup_vtbl_idx(struct str_smart_dim *smart, u32 gamma)
{
	u32 lookup_index;
	u16 table_count, table_index;
	u32 gap, i;
	u32 minimum = smart->g300_gra_tbl[255];
	u32 candidate = 0;
	u32 offset = 0;

	lookup_index = (gamma/VALUE_DIM_1000)+1;
	if (lookup_index > MAX_GRADATION) {
		/*printk(KERN_ERR "ERROR Wrong input value  LOOKUP INDEX : %d\n", lookup_index);*/
		return 0;
	}

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

static u32 calc_vt_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	return 0;
}

static u32 calc_v3_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 v0 = VREG_OUT_1000, v3, v11;
	u32 ret;

	/*v0 = dv[ci][IV_0];*/
	v3 = dv[ci][IV_3];
	v11 = dv[ci][IV_11];

	t1 = (v0 - v3) << 10;
	t2 = (v0 - v11) ? (v0 - v11) : (v0) ? v0 : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v11_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v11, v23;
	u32 ret;

	vt = adjust_volt[ci][AD_IVT];
	v11 = dv[ci][IV_11];
	v23 = dv[ci][IV_23];
	t1 = (vt - v11) << 10;
	t2 = (vt - v23) ? (vt - v23) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v23_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v23, v35;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt = adjust_volt[ci][AD_IVT];
	v23 = dv[ci][IV_23];
	v35 = dv[ci][IV_35];

	t1 = (vt - v23) << 10;
	t2 = (vt - v35) ? (vt - v35) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v35_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v35, v51;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt = adjust_volt[ci][AD_IVT];
	v35 = dv[ci][IV_35];
	v51 = dv[ci][IV_51];

	t1 = (vt - v35) << 10;
	t2 = (vt - v51) ? (vt - v51) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v51_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v51, v87;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt = adjust_volt[ci][AD_IVT];
	v51 = dv[ci][IV_51];
	v87 = dv[ci][IV_87];

	t1 = (vt - v51) << 10;
	t2 = (vt - v87) ? (vt - v87) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}


static u32 calc_v87_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v87, v151;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt = adjust_volt[ci][AD_IVT];
	v87 = dv[ci][IV_87];
	v151 = dv[ci][IV_151];

	t1 = (vt - v87) << 10;
	t2 = (vt - v151) ? (vt - v151) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v151_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v151, v203;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt = adjust_volt[ci][AD_IVT];
	v151 = dv[ci][IV_151];
	v203 = dv[ci][IV_203];

	t1 = (vt - v151) << 10;
	t2 = (vt - v203) ? (vt - v203) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v203_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 t1, t2;
	u32 vt, v151, v255;
	u32 ret;

	/*vt = dv[ci][IV_VT];*/
	vt =  adjust_volt[ci][IV_VT];
	v151 = dv[ci][IV_203];
	v255 = dv[ci][IV_255];

	t1 = (vt - v151) << 10;
	t2 = (vt - v255) ? (vt - v255) : (vt) ? vt : 1;
	ret = (320 * (t1/t2)) - (64 << 10);
	ret >>= 10;

	return ret;
}

static u32 calc_v255_reg(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	u32 ret;
	u32 v255;
	v255 = dv[ci][IV_255];

	ret = (559 * 1000) - (VT_255_calc_param * v255);
	ret = ret / 1000;

	return ret;
}

u32 calc_gamma_table_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp)
{
	u32 i, c, t1;
	u32 temp;
	u32 lidx;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_vt_reg,
		calc_v3_reg,
		calc_v11_reg,
		calc_v23_reg,
		calc_v35_reg,
		calc_v51_reg,
		calc_v87_reg,
		calc_v151_reg,
		calc_v203_reg,
		calc_v255_reg,
	};

memset(gamma, 0, sizeof(gamma));

#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][IV_VT] = smart->ve[AD_IVT].v[c]; /* not use V1 calculate value*/
#endif

	for (i = IV_3 ; i < IV_MAX; i++) {
		temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv) / 1000;
		lidx = lookup_vtbl_idx(smart, temp);
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx].v[c];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = c*2;
		t1 = s9_to_s16(mtp[offset]<<8|mtp[offset+1]);
		smart->mtp[c][IV_255] = t1;
	}

	for (i = 1; i < 10; i++) {
		for (c = CI_RED; c < CI_MAX; c++) {
			t1 = (s8)mtp[CI_MAX*(i + 1)+c];
			smart->mtp[c][IV_255 - i] = t1;
		}
	}

	/* for IV_1 does not calculate value */
	/* Do not use  gamma value (IV_1) */
#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		gamma[c][IV_VT] = smart->default_gamma[c];
#endif

	for (i = IV_3; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma[c][i] = (s16)calc_reg[i](c, dv, smart->adjust_volt) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
		result[offset+1] = gamma[c][IV_255] & 0xff;
		result[offset] = (u8)((gamma[c][IV_255] >> 8) & 0xff);
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_VT; i < IV_255; i++)
			result[(CI_MAX*i)+c] = gamma[c][i];
	}

#if 0

printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND VOLTAGE ++++++++++++++++++++++++++++++\n");
for (i = IV_VT; i < IV_255; i++)  {
		printk("V Level : %d - ", i);
			for (c = CI_RED; c < CI_MAX; c++)
				printk("%c : %04dV", color_name[c], dv[c][i]);
			printk("\n");
}

printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND REG ++++++++++++++++++++++++++++++\n");
for (i = IV_VT; i < IV_255; i++)  {
		printk("V Level : %d - ", i);
			for (c = CI_RED; c < CI_MAX; c++)
				printk("%c : %3d, 0x%2x", color_name[c], gamma[c][i], gamma[c][i]);
			printk("\n");
}
#endif
return 0;
}

u32 calc_gamma_table_190_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp)
{
	u32 i, c, t1;
	u32 temp;
	u32 lidx_215_190;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma_215_190[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_vt_reg,
		calc_v3_reg,
		calc_v11_reg,
		calc_v23_reg,
		calc_v35_reg,
		calc_v51_reg,
		calc_v87_reg,
		calc_v151_reg,
		calc_v203_reg,
		calc_v255_reg,
	};
	memset(gamma_215_190, 0, sizeof(gamma_215_190));

#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][IV_1] = smart->ve[AD_IV1].v[c];
#endif

	for (i = IV_3; i < IV_MAX; i++) {
		if (i ==  IV_151) {
			temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv)/1000;
			lidx_215_190 = lookup_vtbl_idx(smart, temp)-1;
		} else if ((i ==  IV_203) || (i ==  IV_255)) {
			temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv)/1000;
			lidx_215_190 = lookup_vtbl_idx(smart, temp)-2;
		} else {
			temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv)/1000;
			lidx_215_190 = lookup_vtbl_idx(smart, temp);
		}

		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx_215_190].v[c];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = c*2;
		t1 = s9_to_s16(mtp[offset]<<8|mtp[offset+1]);
		smart->mtp[c][IV_255] = t1;
	}

	for (i = 1; i < 10; i++) {
		for (c = CI_RED; c < CI_MAX; c++) {
			t1 = (s8)mtp[CI_MAX*(i + 1)+c];
			smart->mtp[c][IV_255 - i] = t1;
		}
	}

	/* for IV1 does not calculate value */
	/* just use default gamma value (IV1) */
#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		gamma_215_190[c][IV_VT] = smart->default_gamma[c];
#endif

	for (i = IV_3; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma_215_190[c][i] = (s16)calc_reg[i](c, dv, smart->adjust_volt) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
		result[offset+1] = gamma_215_190[c][IV_255] & 0xff;
		result[offset] = (u8)((gamma_215_190[c][IV_255] >> 8) & 0xff);
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_VT; i < IV_255; i++)
			result[(CI_MAX*i)+c] = gamma_215_190[c][i];
	}

#if 0
	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND VOLTAGE ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_255; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %04dV", color_name[c], dv[c][i]);
		printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND REG ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_255; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
				printk("2.15Gamma %c : %3d, 0x%2x", color_name[c], gamma_215_190[c][i], gamma_215_190[c][i]);
		printk("\n");
	}
#endif
	return 0;
}

u32 calc_gamma_table_20_100_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp)
{
	u32 i, c, t1;
	u32 temp;
	u32 lidx_110;
	u32 dv[CI_MAX][IV_MAX];
	s16 gamma_110[CI_MAX][IV_MAX];
	u16 offset;
	u32(*calc_reg[IV_MAX])(int ci, u32 dv[CI_MAX][IV_MAX], u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_vt_reg,
		calc_v3_reg,
		calc_v11_reg,
		calc_v23_reg,
		calc_v35_reg,
		calc_v51_reg,
		calc_v87_reg,
		calc_v151_reg,
		calc_v203_reg,
		calc_v255_reg,
	};
	memset(gamma_110, 0, sizeof(gamma_110));

#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][IV_1] = smart->ve[AD_IV1].v[c];
#endif

	for (i = IV_3; i < IV_MAX; i++) {
		if (i ==  IV_11) {
			temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv)/1000;
			lidx_110 = lookup_vtbl_idx(smart, temp)-1;
		} else {
			temp = (smart->gamma_table[gamma_curve][dv_value[i]] * gv)/1000;
			lidx_110 = lookup_vtbl_idx(smart, temp);
		}

		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx_110].v[c];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = c*2;
		t1 = s9_to_s16(mtp[offset]<<8|mtp[offset+1]);
		smart->mtp[c][IV_255] = t1;
	}

	for (i = 1; i < 10; i++) {
		for (c = CI_RED; c < CI_MAX; c++) {
			t1 = (s8)mtp[CI_MAX*(i + 1)+c];
			smart->mtp[c][IV_255 - i] = t1;
		}
	}

	/* for IV1 does not calculate value */
	/* just use default gamma value (IV1) */
#if 0
	for (c = CI_RED; c < CI_MAX; c++)
		gamma_210_110[c][IV_VT] = smart->default_gamma[c];
#endif

	for (i = IV_3; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma_110[c][i] = (s16)calc_reg[i](c, dv, smart->adjust_volt) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
		result[offset+1] = gamma_110[c][IV_255] & 0xff;
		result[offset] = (u8)((gamma_110[c][IV_255] >> 8) & 0xff);
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_VT; i < IV_255; i++)
			result[(CI_MAX*i)+c] = gamma_110[c][i];
	}

#if 0
	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND VOLTAGE ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %04dV", color_name[c], dv[c][i]);
		printk("\n");
	}

	printk(KERN_INFO "\n\n++++++++++++++++++++++++++++++ FOUND REG ++++++++++++++++++++++++++++++\n");
	for (i = IV_VT; i < IV_MAX; i++) {
		printk("V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("2.15Gamma %c : %3d, 0x%2x", color_name[c], gamma_110[c][i], gamma_210_110[c][i]);
		printk("\n");
	}
#endif
	return 0;
}
