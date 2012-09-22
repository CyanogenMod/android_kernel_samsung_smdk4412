/* linux/drivers/video/samsung/smartdimming.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Samsung Smart Dimming for OCTA
 *
 * Minwoo Kim, <minwoo7945.kim@samsung.com>
 *
*/


#include "smart_dimming.h"
#include "s6e8aa0_volt_tbl.h"

#define MTP_REVERSE		1
#define VALUE_DIM_1000	1000


const u8 v1_offset_table[14] = {
	47, 42, 37, 32,
	27, 23, 19, 15,
	12,  9,  6,  4,
	2,  0
};


const u8 v15_offset_table[20] = {
	66, 62, 58, 54,
	50, 46, 43, 38,
	34, 30, 27, 24,
	21, 18, 15, 12,
	9,  6,  3, 0,
};


const u8 range_table_count[IV_TABLE_MAX] = {
	1, 14, 20, 24, 28, 84, 84, 1
};


const u32 table_radio[IV_TABLE_MAX] = {
	0, 630, 468, 1365, 1170, 390, 390, 0
};


const u32 dv_value[IV_MAX] = {
	0, 15, 35, 59, 87, 171, 255
};


const char color_name[3] = {'R', 'G', 'B'};


const u8 *offset_table[IV_TABLE_MAX] = {
	NULL,
	v1_offset_table,
	v15_offset_table,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

const unsigned char gamma_300cd_23[] = {
	0x0f, 0x0f, 0x0f, 0xee, 0xb4, 0xee,
	0xcb, 0xc2, 0xc4, 0xda, 0xd7, 0xd5,
	0xae, 0xaf, 0xa7, 0xc0, 0xc1, 0xbb,
	0x00, 0x9f, 0x00, 0x95, 0x00, 0xd4,
};

const unsigned char gamma_300cd_33[] = {
	0x0f, 0x00, 0x0f, 0xda, 0xc0, 0xe4,
	0xc8, 0xc8, 0xc6, 0xd3, 0xd6, 0xd0,
	0xab, 0xb2, 0xa6, 0xbf, 0xc2, 0xb9,
	0x00, 0x93, 0x00, 0x86, 0x00, 0xd1,
};

const unsigned char gamma_300cd_43[] = {
	0x1f, 0x1f, 0x1f, 0xe3, 0x69, 0xe0,
	0xcc, 0xc1, 0xce, 0xd4, 0xd2, 0xd2,
	0xae, 0xae, 0xa6, 0xbf, 0xc0, 0xb6,
	0x00, 0xa3, 0x00, 0x90, 0x00, 0xd7,
};

const unsigned char gamma_300cd_53[] = {
	0x1f, 0x1f, 0x1f, 0xe7, 0x7f, 0xe3,
	0xcc, 0xc1, 0xd0, 0xd5, 0xd3, 0xd3,
	0xae, 0xaf, 0xa8, 0xbe, 0xc0, 0xb7,
	0x00, 0xa8, 0x00, 0x90, 0x00, 0xd3,
};

const unsigned char gamma_300cd_63[] = {
	0x60, 0x10, 0x60, 0xb5, 0xd3, 0xbd,
	0xb1, 0xd2, 0xb0, 0xc0, 0xdc, 0xc0,
	0x94, 0xba, 0x91, 0xac, 0xc5, 0xa9,
	0x00, 0xc2, 0x00, 0xb7, 0x00, 0xed,
};

const unsigned char gamma_300cd_73[] = {
	0x1f, 0x1f, 0x1f, 0xed, 0xe6, 0xe7,
	0xd1, 0xd3, 0xd4, 0xda, 0xd8, 0xd7,
	0xb1, 0xaf, 0xab, 0xbd, 0xbb, 0xb8,
	0x00, 0xd6, 0x00, 0xda, 0x00, 0xfa,
};

const unsigned char gamma_300cd_83[] = {
	0x69, 0x5A, 0x6C, 0xA1, 0xB7, 0x9D,
	0xAB, 0xB6, 0xAF, 0xB8, 0xC1, 0xB9,
	0x8E, 0x96, 0x8B, 0xA6, 0xAC, 0xA4,
	0x00, 0xD2, 0x00, 0xD3, 0x00, 0xF1
};

/* SM2 , DALI PANEL - Midas Universal board */
const unsigned char gamma_300cd_4e[] = {
	0x58, 0x1F, 0x63, 0xAC, 0xB4, 0x99,
	0xAD, 0xBA, 0xA3, 0xC0, 0xCB, 0xBB,
	0x93, 0x9F, 0x8B, 0xAD, 0xB4, 0xA7,
	0x00, 0xBE, 0x00, 0xAB, 0x00, 0xE7,
};

/* SM2  C1/M0 4.8" */
const unsigned char gamma_300cd_20[] = {
	0x43, 0x14, 0x45, 0xAD, 0xBE, 0xA9,
	0xB0, 0xC3, 0xAF, 0xC1, 0xCD, 0xC0,
	0x95, 0xA2, 0x91, 0xAC, 0xB5, 0xAA,
	0x00, 0xB0, 0x00, 0xA0, 0x00, 0xCC,
};

/* M3, DALI PANEL - Midas Universal board */
const unsigned char gamma_300cd_2a[] = {
	0x4A, 0x01, 0x4D, 0xBC, 0xF1, 0xC6,
	0xB1, 0xD6, 0xB3, 0xC2, 0xDD, 0xC2,
	0x96, 0xBD, 0x96, 0xAF, 0xC9, 0xAE,
	0x00, 0xA5, 0x00, 0x91, 0x00, 0xC8,
};

const unsigned char gamma_300cd_29[] = {
	0x4A, 0x01, 0x4D, 0xBC, 0xF1, 0xC6,
	0xB1, 0xD6, 0xB3, 0xC2, 0xDD, 0xC2,
	0x96, 0xBD, 0x96, 0xAF, 0xC9, 0xAE,
	0x00, 0xA5, 0x00, 0x91, 0x00, 0xC8,
};

/* SM2, C1/M0 DALI Panel */
const unsigned char gamma_300cd_8e[] = {
	0x71, 0x31, 0x7B, 0xA4, 0xB6, 0x95,
	0xA9, 0xBC, 0xA2, 0xBB, 0xC9, 0xB6,
	0x91, 0xA3, 0x8B, 0xAD, 0xB6, 0xA9,
	0x00, 0xD6, 0x00, 0xBE, 0x00, 0xFC,
};

/* SM2, C1/M0 Cellox Panel */
const unsigned char gamma_300cd_ae[] = {
	0x5F, 0x2E, 0x67, 0xAA, 0xC6, 0xAC,
	0xB0, 0xC8, 0xBB, 0xBE, 0xCB, 0xBD,
	0x97, 0xA5, 0x91, 0xAF, 0xB8, 0xAB,
	0x00, 0xC2, 0x00, 0xBA, 0x00, 0xE2,
};

/* M0 A-Type Panel */
const unsigned char gamma_300cd_d2[] = {
	0x41, 0x0A, 0x47, 0xAB, 0xBE, 0xA8,
	0xAF, 0xC5, 0xB7, 0xC3, 0xCC, 0xC3,
	0x9A, 0xA3, 0x96, 0xB1, 0xB7, 0xAF,
	0x00, 0xBD, 0x00, 0xAC, 0x00, 0xDE,
};

/* SM2, C1/M0 4.8" - D*/
const unsigned char gamma_300cd_60[] = {
	0x3F, 0x12, 0x41, 0xB4, 0xCB, 0xB0,
	0xB2, 0xC4, 0xB2, 0xC4, 0xCF, 0xC2,
	0x9A, 0xA7, 0x97, 0xB2, 0xBA, 0xB0,
	0x00, 0xA0, 0x00, 0x98, 0x00, 0xB7,
};

const unsigned char *gamma_300cd_list[GAMMA_300CD_MAX] = {
	gamma_300cd_23,
	gamma_300cd_33,
	gamma_300cd_43,
	gamma_300cd_53,
	gamma_300cd_63,
	gamma_300cd_73,
	gamma_300cd_83,
	gamma_300cd_20,
	gamma_300cd_2a,
	gamma_300cd_29,
	gamma_300cd_4e,
	gamma_300cd_8e,
	gamma_300cd_ae,
	gamma_300cd_d2,
	gamma_300cd_60,
};

const unsigned char gamma_id_list[GAMMA_300CD_MAX] = {
	0x23, 0x33, 0x43, 0x53, 0x63, 0x73, 0x83, 0x20,
	0x2a, 0x29, 0x4e, 0x8e, 0xae, 0xd2, 0x60
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


u32 calc_v15_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 20, DV :320 */
	int ret = 0;
	u32 v1, v35;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v35 = adjust_volt[rgb_index][AD_IV35];
	ratio = volt_table_cv_20_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v35)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v35_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
{
	/* for CV : 65, DV :320 */
	int ret = 0;
	u32 v1, v59;
	u32 ratio = 0;

	v1 = adjust_volt[rgb_index][AD_IV1];
	v59 = adjust_volt[rgb_index][AD_IV59];
	ratio = volt_table_cv_65_dv_320[gamma];

	ret = (v1 << 10) - ((v1-v59)*ratio);
	ret = ret >> 10;

	return ret;
}


u32 calc_v50_volt(s16 gamma, int rgb_index, u32 adjust_volt[CI_MAX][AD_IVMAX])
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
	u8 index;
	u8 table_index = 0;

	u32 v1, v2;
	u32 ratio;

	u32(*calc_volt[IV_MAX])(s16 gamma, int rgb_index,
			u32 adjust_volt[CI_MAX][AD_IVMAX]) = {
		calc_v1_volt,
		calc_v15_volt,
		calc_v35_volt,
		calc_v50_volt,
		calc_v87_volt,
		calc_v171_volt,
		calc_v255_volt,
	};

	u8 calc_seq[6] = {IV_1, IV_171, IV_87, IV_59, IV_35, IV_15};
	u8 ad_seq[6] = {AD_IV1, AD_IV171, AD_IV87, AD_IV59, AD_IV35, AD_IV15};

	memset(adjust_mtp, 0, sizeof(adjust_mtp));

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
#if defined(MTP_REVERSE)
		offset1 = IV_255*(c+1)+(c*2);
		t1 = s9_to_s16(mtp[offset1]<<8|mtp[offset1+1]);
#else
		t1 = s9_to_s16(mtp[offset]<<8|mtp[offset+1]);
#endif
		t2 = s9_to_s16(smart->default_gamma[offset]<<8|
			smart->default_gamma[offset+1]) + t1;
		smart->mtp[c][IV_255] = t1;
		adjust_mtp[c][IV_255] = t2;
		smart->adjust_volt[c][AD_IV255] =
			calc_volt[IV_255](t2, c, smart->adjust_volt);

		/* for V0 All RGB Voltage Value is Reference Voltage */
		smart->adjust_volt[c][AD_IV0] = 4600;
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++) {
#if defined(MTP_REVERSE)
			t1 = (s8)mtp[(calc_seq[i])+(c*8)];
#else
			t1 = (s8)mtp[CI_MAX*calc_seq[i]+c];
#endif
			t2 = smart->default_gamma[CI_MAX*calc_seq[i]+c] + t1;

			smart->mtp[c][calc_seq[i]] = t1;
			adjust_mtp[c][calc_seq[i]] = t2;
			smart->adjust_volt[c][ad_seq[i]] =
				calc_volt[calc_seq[i]](t2, c,
						smart->adjust_volt);
		}
	}

	for (i = 0; i < AD_IVMAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			smart->ve[table_index].v[c] = smart->adjust_volt[c][i];

		index = 0;
		for (j = table_index + 1;
			j < table_index + range_table_count[i]; j++) {
			for (c = CI_RED; c < CI_MAX; c++) {
				if (smart->t_info[i].offset != NULL)
					ratio = smart->t_info[i].offset[index]
						* smart->t_info[i].rv;
				else
					ratio = (range_table_count[i] -
						(index + 1)) *
						smart->t_info[i].rv;

				v1 = smart->adjust_volt[c][i+1] << 15;
				v2 = (smart->adjust_volt[c][i] -
					smart->adjust_volt[c][i+1]) * ratio;
				smart->ve[j].v[c] = ((v1+v2) >> 15);
			}
			index++;
		}
		table_index = j;
	}

#if 0
	for (i = IV_1; i < IV_MAX; i++) {
		printk(KERN_INFO "V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : 0x%08x(%04d)",
			color_name[c], smart->mtp[c][i], smart->mtp[c][i]);
		printk("\n");
	}

	for (i = IV_1; i < IV_MAX; i++) {
		printk(KERN_INFO "V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : 0x%08x(%04d)", color_name[c],
				adjust_mtp[c][i], adjust_mtp[c][i]);
		printk("\n");
	}

	for (i = AD_IV0; i < AD_IVMAX; i++) {
		printk(KERN_INFO "V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : %04dV",
				color_name[c], smart->adjust_volt[c][i]);
		printk("\n");
	}

	for (i = 0; i < 256; i++) {
		printk(KERN_INFO "Gray Level : %03d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("  %c : %04dV",
				color_name[c], smart->ve[i].v[c]);
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
		smart->t_info[i].offset = offset_table[i];
		smart->t_info[i].rv = table_radio[i];
		offset += range_table_count[i];
	}
	smart->flooktbl = flookup_table;
	smart->g300_gra_tbl = gamma_300_gra_table;
	smart->g22_tbl = gamma_22_table;

	for (i = 0; i < GAMMA_300CD_MAX; i++) {
		if (smart->panelid[1] == gamma_id_list[i])
			break;
	}

	if (i >= GAMMA_300CD_MAX) {
		printk(KERN_ERR "Can't found default gamma table\n");
		smart->default_gamma = gamma_300cd_list[GAMMA_300CD_MAX-1];
	} else
		smart->default_gamma = gamma_300cd_list[i];

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

	lookup_index = (gamma/VALUE_DIM_1000)+1;
	if (lookup_index > MAX_GRADATION) {
		printk(KERN_ERR "ERROR Wrong input value  LOOKUP INDEX : %d\n",
				lookup_index);
		return 0;
	}

	/* printk("lookup index : %d\n",lookup_index); */

	if (smart->flooktbl[lookup_index].count) {
		if (smart->flooktbl[lookup_index-1].count) {
			table_index = smart->flooktbl[lookup_index-1].entry;
			table_count = smart->flooktbl[lookup_index].count +
				smart->flooktbl[lookup_index-1].count;
		} else {
			table_index = smart->flooktbl[lookup_index].entry;
			table_count = smart->flooktbl[lookup_index].count;
		}
	} else {
		offset += 1;
		while (!(smart->flooktbl[lookup_index + offset].count ||
				smart->flooktbl[lookup_index - offset].count))
			offset++;

		if (smart->flooktbl[lookup_index-offset].count)
			table_index =
				smart->flooktbl[lookup_index - offset].entry;
		else
			table_index =
				smart->flooktbl[lookup_index + offset].entry;
		table_count = smart->flooktbl[lookup_index + offset].count +
			smart->flooktbl[lookup_index - offset].count;
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
	ret = (595 * 1000) - (130 * v1);
	ret = ret/1000;

	return ret;
}


u32 calc_v15_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v15, v35;
	u32 ret;

	v1 = dv[ci][IV_1];
	v15 = dv[ci][IV_15];
	v35 = dv[ci][IV_35];

#if 0
	t1 = (v1 - v15) * 1000;
	t2 = v1 - v35;

	ret = 320*(t1/t2)-(20*1000);

	ret = ret/1000;
#else
	t1 = (v1 - v15) << 10;
	t2 = (v1 - v35) ? (v1 - v35) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (20 << 10);
	ret >>= 10;

#endif
	return ret;
}


u32 calc_v35_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v35, v57;
	u32 ret;

	v1 = dv[ci][IV_1];
	v35 = dv[ci][IV_35];
	v57 = dv[ci][IV_59];

#if 0
	t1 = (v1 - v35) * 1000;
	t2 = v1 - v57;
	ret = 320*(t1/t2) - (65 * 1000);

	ret = ret/1000;
#else
	t1 = (v1 - v35) << 10;
	t2 = (v1 - v57) ? (v1 - v57) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);

	ret >>= 10;
#endif

	return ret;
}


u32 calc_v50_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 t1, t2;
	u32 v1, v57, v87;
	u32 ret;

	v1 = dv[ci][IV_1];
	v57 = dv[ci][IV_59];
	v87 = dv[ci][IV_87];

#if 0
	t1 = (v1 - v57) * 1000;
	t2 = v1 - v87;
	ret = 320*(t1/t2) - (65 * 1000);
	ret = ret/1000;
#else
	t1 = (v1 - v57) << 10;
	t2 = (v1 - v87) ? (v1 - v87) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
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
	ret = 320*(t1/t2) - (65 * 1000);
	ret = ret/1000;
#else
	t1 = (v1 - v87) << 10;
	t2 = (v1 - v171) ? (v1 - v171) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
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
	ret = 320*(t1/t2) - (65 * 1000);
	ret = ret/1000;
#else
	t1 = (v1 - v171) << 10;
	t2 = (v1 - v255) ? (v1 - v255) : (v1) ? v1 : 1;
	ret = (320 * (t1/t2)) - (65 << 10);
	ret >>= 10;
#endif

	return ret;
}


u32 calc_v255_reg(int ci, u32 dv[CI_MAX][IV_MAX])
{
	u32 ret;
	u32 v255;

	v255 = dv[ci][IV_255];

	ret = (500 * 1000) - (130 * v255);
	ret = ret / 1000;

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
		calc_v15_reg,
		calc_v35_reg,
		calc_v50_reg,
		calc_v87_reg,
		calc_v171_reg,
		calc_v255_reg,
	};

	memset(gamma, 0, sizeof(gamma));

	for (c = CI_RED; c < CI_MAX; c++)
		dv[c][0] = smart->adjust_volt[c][AD_IV1];


	for (i = IV_15; i < IV_MAX; i++) {
		temp = smart->g22_tbl[dv_value[i]] * gv;
		lidx = lookup_vtbl_idx(smart, temp);
		for (c = CI_RED; c < CI_MAX; c++)
			dv[c][i] = smart->ve[lidx].v[c];
	}

	/* for IV1 does not calculate value */
	/* just use default gamma value (IV1) */
	for (c = CI_RED; c < CI_MAX; c++)
		gamma[c][IV_1] = smart->default_gamma[c];

	for (i = IV_15; i < IV_MAX; i++) {
		for (c = CI_RED; c < CI_MAX; c++)
			gamma[c][i] =
				(s16)calc_reg[i](c, dv) - smart->mtp[c][i];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		offset = IV_255*CI_MAX+c*2;
		result[offset+1] = gamma[c][IV_255];
	}

	for (c = CI_RED; c < CI_MAX; c++) {
		for (i = IV_1; i < IV_255; i++)
			result[(CI_MAX*i)+c] = gamma[c][i];
	}

#if 0
	for (i = IV_1; i < IV_MAX; i++) {
		printk(KERN_INFO "V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %04dV", color_name[c], dv[c][i]);
		printk("\n");
	}

	for (i = IV_1; i < IV_MAX; i++) {
		printk(KERN_INFO "V Level : %d - ", i);
		for (c = CI_RED; c < CI_MAX; c++)
			printk("%c : %3d, 0x%2x",
				color_name[c], gamma[c][i], gamma[c][i]);
		printk("\n");
	}
#endif
	return 0;
}

