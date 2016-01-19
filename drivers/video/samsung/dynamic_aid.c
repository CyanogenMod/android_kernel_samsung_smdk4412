/* linux/drivers/video/backlight/dynamic_aid.c
 *
 * Samsung Electronics Dynamic AID for AMOLED.
 *
 * Copyright (c) 2013 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>

#include "dynamic_aid.h"

#ifdef DYNAMIC_AID_DEBUG
#define aid_dbg(format, arg...)	printk(format, ##arg)
#else
#define aid_dbg(format, arg...)
#endif

struct rgb64_t {
	s64 rgb[3];
};

struct dynamic_aid_info {
	struct dynamic_aid_param_t param;
	int			*iv_tbl;
	int			iv_max;
	int			iv_top;
	int			*ibr_tbl;
	int			ibr_max;
	int			ibr_top;
	int			 *mtp;
	int			vreg;

	struct rgb64_t *point_voltages;
	struct rgb64_t *output_voltages;
	int			*l_value;
	int			*l_lookup_table;
	int			*m_gray;
	struct rgb64_t *m_voltage;
};

#define MUL_100(x)		(x*100)
#define MUL_1000(x)		(x*1000)
#define MUL_10000(x)		(x*10000)
#define DIV_10(x)		((x+5)/10)
#define DIV_100(x)		((x+50)/100)

static int calc_point_voltages(struct dynamic_aid_info d_aid)
{
	int ret, iv, c;
	struct rgb64_t *vt, *vp;
	int *vd, *mtp;
	int numerator, denominator;
	s64 v0;
	s64 t1, t2;
	struct rgb64_t *point_voltages;

	point_voltages = d_aid.point_voltages;
	v0 = d_aid.vreg;
	ret = 0;

	/* iv == 0; */
	{
		vd = (int *)&d_aid.param.gamma_default[0];
		mtp = &d_aid.mtp[0];

		numerator = d_aid.param.formular[0].numerator;
		denominator = d_aid.param.formular[0].denominator;

		for (c = 0; c < CI_MAX; c++) {
			t1 = v0;
			t2 = v0*d_aid.param.vt_voltage_value[vd[c] + mtp[c]];
			point_voltages[0].rgb[c] = t1 - div_s64(t2, denominator);
		}
	}

	iv = d_aid.iv_max - 1;

	/* iv == (IV_MAX-1) ~ 1; */
	for (iv; iv > 0; iv--) {
		vt = &point_voltages[0];
		vp = &point_voltages[iv+1];
		vd = (int *)&d_aid.param.gamma_default[iv*CI_MAX];
		mtp = &d_aid.mtp[iv*CI_MAX];

		numerator = d_aid.param.formular[iv].numerator;
		denominator = d_aid.param.formular[iv].denominator;

		for (c = 0; c < CI_MAX; c++) {
			if (iv == 1) {
				t1 = v0;
				t2 = v0 - vp->rgb[c];
			} else if (iv == d_aid.iv_max - 1) {
				t1 = v0;
				t2 = v0;
			} else {
				t1 = vt->rgb[c];
				t2 = vt->rgb[c] - vp->rgb[c];
			}
			t2 *= vd[c] + mtp[c] + numerator;
			point_voltages[iv].rgb[c] = (t1 - div_s64(t2, denominator));
		}
	}

#ifdef DYNAMIC_AID_DEBUG
	for (iv = 0; iv < d_aid.iv_max; iv++) {
		aid_dbg("Point Voltage[%d] = ", iv);
		for (c = 0; c < CI_MAX; c++)
			aid_dbg("%lld ", point_voltages[iv].rgb[c]);
		aid_dbg("\n");
	}
#endif

	return ret;
}

static int calc_output_voltages(struct dynamic_aid_info d_aid)
{
	int iv, i, c;
	int v_idx, v_cnt;
	struct rgb_t v, v_diff;
	struct rgb64_t *output_voltages;
	struct rgb64_t *point_voltages;

	output_voltages = d_aid.output_voltages;
	point_voltages = d_aid.point_voltages;
	iv = d_aid.iv_max - 1;

	/* iv == (IV_MAX-1) ~ 0; */
	for (iv; iv > 0; iv--) {
		v_cnt = d_aid.iv_tbl[iv] - d_aid.iv_tbl[iv-1];
		v_idx = d_aid.iv_tbl[iv];

		for (c = 0; c < CI_MAX; c++) {
			v.rgb[c] = point_voltages[iv].rgb[c];
			v_diff.rgb[c] = point_voltages[iv-1].rgb[c] - point_voltages[iv].rgb[c];
		}

		if (iv == 1)
			for (c = 0; c < CI_MAX; c++)
				v_diff.rgb[c] = d_aid.vreg - point_voltages[iv].rgb[c];

		for (i = 0; i < v_cnt; i++, v_idx--) {
			for (c = 0; c < CI_MAX; c++)
				output_voltages[v_idx].rgb[c] = v.rgb[c] + v_diff.rgb[c]*i/v_cnt;
		}
	}

	for (c = 0; c < CI_MAX; c++)
		output_voltages[0].rgb[c] = d_aid.vreg;

#ifdef DYNAMIC_AID_DEBUG
	for (iv = 0; iv <= d_aid.iv_top; iv++) {
		aid_dbg("Output Voltage[%d] = ", iv);
		for (c = 0; c < CI_MAX; c++)
			aid_dbg("%lld ", output_voltages[iv].rgb[c]);
		aid_dbg("\n");
	}
#endif

	return 0;
}

static int calc_voltage_table(struct dynamic_aid_info d_aid)
{
	int ret;

	ret = calc_point_voltages(d_aid);
	if (ret)
		return ret;

	ret = calc_output_voltages(d_aid);
	if (ret)
		return ret;

	return 0;
}

static int init_l_lookup_table(struct dynamic_aid_info d_aid)
{
	int iv;
	int *gamma_curve;
	int *l_lookup_table;

	gamma_curve = (int *)d_aid.param.gc_lut;
	l_lookup_table = d_aid.l_lookup_table;

	for (iv = 0; iv <= d_aid.iv_top; iv++)
		l_lookup_table[iv] = DIV_100(d_aid.ibr_top*gamma_curve[iv]);

#ifdef DYNAMIC_AID_DEBUG
	for (iv = 0; iv <= d_aid.iv_top; iv++)
		aid_dbg("L lookup table[%d] = %d\n", iv, l_lookup_table[iv]);
#endif
	return 0;
}

static int min_diff_gray(int in, int *table, int table_cnt)
{
	int ret, i, min, temp;

	ret = min = table[table_cnt] + 1;

	for (i = 0; i <= table_cnt; i++) {
		temp = table[i] - in;
		if (temp < 0)
			temp = -temp;

		if (temp <= min) {
			min = temp;
			ret = i;
		}

		if ((in >= table[i-1]) && (in <= table[i]))
			break;
	}
	return ret;
}

static int calc_l_values(struct dynamic_aid_info d_aid, int ibr)
{
	int iv;
	int *gamma_curve;
	int *br_base;
	int *l_value;

	gamma_curve = (int *)d_aid.param.gc_tbls[ibr];
	br_base = (int *)d_aid.param.br_base;
	l_value = d_aid.l_value;
	iv = d_aid.iv_max - 1;

	/* iv == (IV_MAX - 1) ~ 0; */
	for (iv; iv >= 0; iv--) {
		if (iv == d_aid.iv_max - 1)
			l_value[iv] = MUL_10000(br_base[ibr]);
		else
			l_value[iv] = DIV_100(br_base[ibr]*gamma_curve[d_aid.iv_tbl[iv]]);
	}

#ifdef DYNAMIC_AID_DEBUG
	aid_dbg("L value (%d) = ", d_aid.ibr_tbl[ibr]);
	for (iv = 0; iv < d_aid.iv_max; iv++)
		aid_dbg("%d ", l_value[iv]);
	aid_dbg("\n");
#endif
	return 0;
}

static int calc_m_gray_values(struct dynamic_aid_info d_aid, int ibr)
{
	int iv;
	int (*offset_gra)[d_aid.iv_max];
	int *l_lookup_table;
	int *l_value;
	int *m_gray;

	offset_gra = (int(*)[])d_aid.param.offset_gra;
	l_lookup_table = d_aid.l_lookup_table;
	l_value = d_aid.l_value;
	m_gray = d_aid.m_gray;
	iv = d_aid.iv_max - 1;

	/* iv == (IV_MAX - 1) ~ 0; */
	for (iv; iv >= 0; iv--) {
		m_gray[iv] = min_diff_gray(l_value[iv], l_lookup_table, d_aid.iv_top);
		if (offset_gra)
			m_gray[iv] += offset_gra[ibr][iv];
	}

#ifdef DYNAMIC_AID_DEBUG
	aid_dbg("M-Gray value[%d] = ", d_aid.ibr_tbl[ibr]);
	for (iv = 0; iv < d_aid.iv_max; iv++)
		aid_dbg("%d ", d_aid.m_gray[iv]);
	aid_dbg("\n");
#endif

	return 0;
}

static int calc_m_rgb_voltages(struct dynamic_aid_info d_aid, int ibr)
{
	int iv, c;
	struct rgb64_t *output_voltages;
	struct rgb64_t *point_voltages;
	int *m_gray;
	struct rgb64_t *m_voltage;

	output_voltages = d_aid.output_voltages;
	point_voltages = d_aid.point_voltages;
	m_gray = d_aid.m_gray;
	m_voltage = d_aid.m_voltage;
	iv = d_aid.iv_max - 1;

	/* iv == (IV_MAX - 1) ~ 1; */
	for (iv; iv > 0; iv--) {
		for (c = 0; c < CI_MAX; c++)
			m_voltage[iv].rgb[c] = output_voltages[m_gray[iv]].rgb[c];
	}

	/* iv == 0; */
	for (c = 0; c < CI_MAX; c++)
		m_voltage[iv].rgb[c] = point_voltages[0].rgb[c];

#ifdef DYNAMIC_AID_DEBUG
	aid_dbg("M-RGB voltage (%d) =\n", d_aid.ibr_tbl[ibr]);
	for (iv = 0; iv < d_aid.iv_max; iv++) {
		aid_dbg("[%d] = ", iv);
		for (c = 0; c < CI_MAX; c++)
			aid_dbg("%lld ", d_aid.m_voltage[iv].rgb[c]);
		aid_dbg("\n");
	}
#endif

	return 0;
}

static int calc_gamma(struct dynamic_aid_info d_aid, int ibr, int *result)
{
	int ret, iv, c;
	int numerator, denominator;
	s64 t1, t2;
	int *vd, *mtp, *res;
	struct rgb_t (*offset_color)[d_aid.iv_max];
	struct rgb64_t *m_voltage;

	offset_color = (struct rgb_t(*)[])d_aid.param.offset_color;
	m_voltage = d_aid.m_voltage;
	iv = d_aid.iv_max - 1;
	ret = 0;

	/* iv == (IV_MAX - 1) ~ 1; */
	for (iv; iv > 0; iv--) {
		mtp = &d_aid.mtp[iv*CI_MAX];
		res = &result[iv*CI_MAX];
		numerator = d_aid.param.formular[iv].numerator;
		denominator = d_aid.param.formular[iv].denominator;
		for (c = 0; c < CI_MAX; c++) {
			if (iv == 0) {
				t1 = 0;
				t2 = 1;
			} else if (iv == 1) {
				t1 = d_aid.vreg - m_voltage[iv].rgb[c];
				t2 = d_aid.vreg - m_voltage[iv+1].rgb[c];
			} else if (iv == d_aid.iv_max - 1) {
				t1 = d_aid.vreg - m_voltage[iv].rgb[c];
				t2 = d_aid.vreg;
			} else {
				t1 = m_voltage[0].rgb[c] - m_voltage[iv].rgb[c];
				t2 = m_voltage[0].rgb[c] - m_voltage[iv+1].rgb[c];
			}
			res[c] = div64_s64((t1 + 1) * denominator, t2) - numerator;
			res[c] -= mtp[c];

			if (offset_color)
				res[c] += offset_color[ibr][iv].rgb[c];
		}
	}

	/* iv == 0; */
	vd = (int *)&d_aid.param.gamma_default[0];
	mtp = &d_aid.mtp[0];
	res = &result[0];
	for (c = 0; c < CI_MAX; c++)
		res[c] = vd[c];

#ifdef DYNAMIC_AID_DEBUG
	aid_dbg("Gamma (%d) =\n", d_aid.ibr_tbl[ibr]);
	for (iv = 0; iv < d_aid.iv_max; iv++) {
		aid_dbg("[%d] = ", iv);
		for (c = 0; c < CI_MAX; c++)
			aid_dbg("%X ", result[iv*CI_MAX+c]);
		aid_dbg("\n");
	}
#endif

	return ret;
}

static int calc_gamma_table(struct dynamic_aid_info d_aid, int **gamma)
{
	int ibr;
#ifdef DYNAMIC_AID_DEBUG
	int iv, c;
#endif

	init_l_lookup_table(d_aid);

	/* ibr == 0 ~ (IBRIGHTNESS_MAX - 1); */
	for (ibr = 0; ibr < d_aid.ibr_max; ibr++) {
		calc_l_values(d_aid, ibr);
		calc_m_gray_values(d_aid, ibr);
		calc_m_rgb_voltages(d_aid, ibr);
		calc_gamma(d_aid, ibr, gamma[ibr]);
	}

#ifdef DYNAMIC_AID_DEBUG
	for (ibr = 0; ibr < d_aid.ibr_max; ibr++) {
		aid_dbg("Gamma [%03d] = ", d_aid.ibr_tbl[ibr]);
		for (iv = 1; iv < d_aid.iv_max; iv++) {
			for (c = 0; c < CI_MAX; c++)
				aid_dbg("%04d ", gamma[ibr][iv*CI_MAX+c]);
		}
		for (c = 0; c < CI_MAX; c++)
			aid_dbg("%d ", gamma[ibr][c]);
		aid_dbg("\n");
	}
#endif

	return 0;
}

int dynamic_aid(struct dynamic_aid_param_t param, int **gamma)
{
	struct dynamic_aid_info d_aid;
	int ret;

	d_aid.param = param;
	d_aid.iv_tbl = (int *)param.iv_tbl;
	d_aid.iv_max = param.iv_max;
	d_aid.iv_top = param.iv_tbl[param.iv_max-1]; /* number of top voltage index:  255 */
	d_aid.mtp = param.mtp;
	d_aid.vreg = MUL_100(param.vreg);

	d_aid.ibr_tbl = (int *)param.ibr_tbl;
	d_aid.ibr_max = param.ibr_max;
	d_aid.ibr_top = param.ibr_tbl[param.ibr_max-1]; /* number of top brightness : 300nt */

	d_aid.point_voltages = kzalloc(sizeof(struct rgb64_t)*d_aid.iv_max, GFP_KERNEL);
	if (!d_aid.point_voltages) {
		printk(KERN_ERR "failed to allocate point_voltages\n");
		ret = -ENOMEM;
		goto error1;
	}
	d_aid.output_voltages = kzalloc(sizeof(struct rgb64_t)*(d_aid.iv_top+1), GFP_KERNEL);
	if (!d_aid.output_voltages) {
		printk(KERN_ERR "failed to allocate output_voltages\n");
		ret = -ENOMEM;
		goto error2;
	}
	d_aid.l_value = kzalloc(sizeof(int)*d_aid.iv_max, GFP_KERNEL);
	if (!d_aid.l_value) {
		printk(KERN_ERR "failed to allocate l_value\n");
		ret = -ENOMEM;
		goto error3;
	}
	d_aid.l_lookup_table = kzalloc(sizeof(int)*(d_aid.iv_top+1), GFP_KERNEL);
	if (!d_aid.l_lookup_table) {
		printk(KERN_ERR "failed to allocate l_lookup_table\n");
		ret = -ENOMEM;
		goto error4;
	}
	d_aid.m_gray = kzalloc(sizeof(int)*d_aid.iv_max, GFP_KERNEL);
	if (!d_aid.m_gray) {
		printk(KERN_ERR "failed to allocate m_gray\n");
		ret = -ENOMEM;
		goto error5;
	}
	d_aid.m_voltage = kzalloc(sizeof(struct rgb64_t)*d_aid.iv_max, GFP_KERNEL);
	if (!d_aid.m_voltage) {
		printk(KERN_ERR "failed to allocate m_voltage\n");
		ret = -ENOMEM;
		goto error6;
	}

	ret = calc_voltage_table(d_aid);
	if (ret)
		goto error7;

	ret = calc_gamma_table(d_aid, gamma);
	if (ret)
		goto error7;

	printk(KERN_INFO "Dynamic Aid Finished !\n");

error7:
	kfree(d_aid.m_voltage);
error6:
	kfree(d_aid.m_gray);
error5:
	kfree(d_aid.l_lookup_table);
error4:
	kfree(d_aid.l_value);
error3:
	kfree(d_aid.output_voltages);
error2:
	kfree(d_aid.point_voltages);
error1:
	return ret;

}
