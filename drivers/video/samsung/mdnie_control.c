/*
 * mdnie_control.c - mDNIe register sequence intercept and control
 *
 * @Author	: Andrei F. <https://github.com/AndreiLux>
 * @Date	: February 2013
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/earlysuspend.h>

#include "mdnie.h"
#include "s3cfb.h"
#include "s3cfb_mdnie.h"

#define REFRESH_DELAY		HZ / 2
struct delayed_work mdnie_refresh_work;

bool reg_hook = 0;
bool scenario_hook = 0;
struct mdnie_info *mdnie = NULL;

extern struct mdnie_info *g_mdnie;
extern void set_mdnie_value(struct mdnie_info *mdnie, u8 force);

enum mdnie_registers {
	EFFECT_MASTER	= 0x08,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	FA_MASTER	= 0x30,	/*FA cs1 | de8 dnr4 hdr2 fa1*/
	FA_DNR_WEIGHT	= 0x39,	/*FA dnrWeight*/

	DNR_1		= 0x80,	/*DNR dirTh*/
	DNR_2		= 0x81,	/*DNR dirnumTh decon7Th*/
	DNR_3		= 0x82,	/*DNR decon5Th maskTh*/
	DNR_4		= 0x83,	/*DNR blTh*/

	DE_EGTH		= 0x90,	/*DE egth*/
	DE_PE		= 0x92, /*DE pe*/
	DE_PF		= 0x93,	/*DE pf*/
	DE_PB		= 0x94,	/*DE pb*/
	DE_NE		= 0x95,	/*DE ne*/
	DE_NF		= 0x96,	/*DE nf*/
	DE_NB		= 0x97,	/*DE nb*/
	DE_MAX_RATIO	= 0x98,	/*DE max ratio*/
	DE_MIN_RATIO	= 0x99,	/*DE min ratio*/

	CS_HG_RY	= 0xb0,	/*CS hg ry*/
	CS_HG_GC	= 0xb1,	/*CS hg gc*/
	CS_HG_BM	= 0xb2,	/*CS hg bm*/
	CS_WEIGHT_GRTH	= 0xb3,	/*CS weight grayTH*/

	SCR_RR_CR	= 0xe1,	/*SCR RrCr*/
	SCR_RG_CG	= 0xe2,	/*SCR RgCg*/
	SCR_RB_CB	= 0xe3,	/*SCR RbCb*/

	SCR_GR_MR	= 0xe4,	/*SCR GrMr*/
	SCR_GG_MG	= 0xe5,	/*SCR GgMg*/
	SCR_GB_MB	= 0xe6,	/*SCR GbMb*/

	SCR_BR_YR	= 0xe7,	/*SCR BrYr*/
	SCR_BG_YG	= 0xe8,	/*SCR BgYg*/
	SCR_BB_YB	= 0xe9,	/*SCR BbYb*/

	SCR_KR_WR	= 0xea,	/*SCR KrWr*/
	SCR_KG_WG	= 0xeb,	/*SCR KgWg*/
	SCR_KB_WB	= 0xec,	/*SCR KbWb*/

	MCM_TEMPERATURE = 0x01, /*MCM 0x64 10000K 0x28 4000K */
	MCM_9		= 0x09, /*MCM 5cb 1cr W*/
	MCM_B		= 0x0b, /*MCM 4cr 5cr W*/

	UC_Y		= 0xd0,
	UC_CS		= 0xd1,

	CC_CHSEL_STR	= 0x1f,	/*CC chsel strength*/
	CC_0		= 0x20,	/*CC lut r   0*/
	CC_1		= 0x21,	/*CC lut r  16 144*/
	CC_2		= 0x22,	/*CC lut r  32 160*/
	CC_3		= 0x23,	/*CC lut r  48 176*/
	CC_4		= 0x24,	/*CC lut r  64 192*/
	CC_5		= 0x25,	/*CC lut r  80 208*/
	CC_6		= 0x26,	/*CC lut r  96 224*/
	CC_7		= 0x27,	/*CC lut r 112 240*/
	CC_8		= 0x28	/*CC lut r 128 255*/
};

static unsigned short master_sequence[92] = { 
	0x0000, 0x0000,  0x0008, 0x0000,  0x0030, 0x0000,  0x0090, 0x0080,
	0x0092, 0x0030,  0x0093, 0x0060,  0x0094, 0x0060,  0x0095, 0x0030,
	0x0096, 0x0060,  0x0097, 0x0060,  0x0098, 0x1000,  0x0099, 0x0100,
	0x00b0, 0x1010,  0x00b1, 0x1010,  0x00b2, 0x1010,  0x00b3, 0x1804,

	0x00e1, 0xff00,  0x00e2, 0x00ff,  0x00e3, 0x00ff,  0x00e4, 0x00ff,
	0x00e5, 0xff00,  0x00e6, 0x00ff,  0x00e7, 0x00ff,  0x00e8, 0x00ff,
	0x00e9, 0xff00,  0x00ea, 0x00ff,  0x00eb, 0x00ff,  0x00ec, 0x00ff,
	0x0000, 0x0001,  0x0001, 0x0041,  0x0009, 0xa08b,  0x000b, 0x7a7a,

	0x00d0, 0x01c0,  0x00d1, 0x01ff,  0x001f, 0x0080,  0x0020, 0x0000,
	0x0021, 0x1090,  0x0022, 0x20a0,  0x0023, 0x30b0,  0x0024, 0x40c0,
	0x0025, 0x50d0,  0x0026, 0x60e0,  0x0027, 0x70f0,  0x0028, 0x80ff,
	0x00ff, 0x0000,  0xffff, 0x0000,
};

static ssize_t show_mdnie_property(struct device *dev,
				    struct device_attribute *attr, char *buf);

static ssize_t store_mdnie_property(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count);

#define _effect(name_, reg_, mask_, shift_, regval_)\
{ 									\
	.attribute = {							\
			.attr = {					\
				  .name = name_,			\
				  .mode = S_IRUGO | S_IWUSR | S_IWGRP,	\
				},					\
			.show = show_mdnie_property,			\
			.store = store_mdnie_property,			\
		     },							\
	.reg 	= reg_ ,						\
	.mask 	= mask_ ,						\
	.shift 	= shift_ ,						\
	.delta 	= 1 ,							\
	.value 	= 0 ,							\
	.regval = regval_						\
}

//TODO Fix S6E8AA0 once we have calibration data
#if defined(CONFIG_FB_S5P_S6EVR02)
#define _model(a, b) b
#else 
#if defined(CONFIG_FB_S5P_S6E8AA0)
#define _model(a, b) b
#endif
#endif

struct mdnie_effect {
	const struct device_attribute	attribute;
	u8				reg;
	u16				mask;
	u8				shift;
	bool				delta;
	int				value;
	u16				regval;
};

struct mdnie_effect mdnie_controls[] = {
	/* Master switches */
	_effect("s_dithering"		, EFFECT_MASTER	, (1 << 11), 11	, _model( 0	, 0	)),
	_effect("s_UC"			, EFFECT_MASTER	, (1 << 10), 10	, _model( 0	, 0	)),
	_effect("s_ABC"			, EFFECT_MASTER	, (1 << 9), 9	, _model( 0	, 0	)),
	_effect("s_CP"			, EFFECT_MASTER	, (1 << 8), 8	, _model( 0	, 0	)),

	_effect("s_gamma_curve"		, EFFECT_MASTER	, (1 << 7), 7	, _model( 0	, 0	)),
	_effect("s_MCM"			, EFFECT_MASTER	, (1 << 6), 6	, _model( 0	, 0	)),
	_effect("s_channel_filters"	, EFFECT_MASTER	, (1 << 5), 5	, _model( 0	, 1	)),
	_effect("s_SCC"			, EFFECT_MASTER	, (1 << 4), 4	, _model( 0	, 0	)),

	_effect("s_chroma_saturation"	, EFFECT_MASTER	, (1 << 3), 3	, _model( 0	, 1	)),
	_effect("s_edge_enhancement"	, EFFECT_MASTER	, (1 << 2), 2	, _model( 0	, 0	)),
	_effect("s_digital_noise_reduction", EFFECT_MASTER, (1 << 1), 1	, _model( 0	, 0	)),
	_effect("s_high_dynamic_range"	, EFFECT_MASTER	, (1 << 0), 0	, _model( 0	, 0	)),

	/* Factory setting overrides */
	_effect("s_factory_chroma_saturation", FA_MASTER , (1 << 4), 4	, _model( 0	, 0	)),
	_effect("s_factory_edge_enhancement", FA_MASTER	, (1 << 3), 3	, _model( 0	, 0	)),
	_effect("s_factory_digital_noise_reduction", FA_MASTER , (1 << 2), 2, _model( 0	, 0	)),
	_effect("s_factory_high_dynamic_range", FA_MASTER, (1 << 1), 1	, _model( 0	, 0	)),
	_effect("s_FA_fa"		, FA_MASTER 	, (1 << 0), 0	, _model( 0	, 0	)),

	_effect("FA_dnr_weight"		, FA_DNR_WEIGHT	, (0xff)  , 0	, _model( 0	, 0	)),
	
	/* Digital noise reduction */
	_effect("dnr_dirTh"		, DNR_1		, (0xffff), 0	, 4095	),
	_effect("dnr_dirnumTh"		, DNR_2		, (0xff00), 8	, 25	),
	_effect("dnr_decon7Th"		, DNR_2		, (0x00ff), 0	, 255	),
	_effect("dnr_decon5Th"		, DNR_3		, (0xff00), 8	, 255	),
	_effect("dnr_maskTh"		, DNR_3		, (0x00ff), 0	, 22	),
	_effect("dnr_blTh"		, DNR_4		, (0x00ff), 0	, 0	),

	/* Ditigal edge enhancement */
	_effect("de_egth"		, DE_EGTH	, (0x00ff), 0	, 128	),

	_effect("de_positive_e"		, DE_PE		, (0x00ff), 0	, 48	),
	_effect("de_positive_f"		, DE_PF		, (0x00ff), 0	, 96	),
	_effect("de_positive_b"		, DE_PB		, (0x00ff), 0	, 96	),

	_effect("de_negative_e"		, DE_NE		, (0x00ff), 0	, 48	),
	_effect("de_negative_f"		, DE_NF		, (0x00ff), 0	, 96	),
	_effect("de_negative_b"		, DE_NB		, (0x00ff), 0	, 96	),

	_effect("de_min_ratio"		, DE_MIN_RATIO	, (0xffff), 0	, 4096	),
	_effect("de_max_ratio"		, DE_MAX_RATIO	, (0xffff), 0	, 256	),

	/* Chroma saturation */
	_effect("cs_weight"		, CS_WEIGHT_GRTH, (0xff00), 8	, 24	),
	_effect("cs_gray_threshold"	, CS_WEIGHT_GRTH, (0x00ff), 0	, 4	),

	_effect("cs_red"		, CS_HG_RY	, (0xff00), 8	, _model( 16	, 5	)),
	_effect("cs_green"		, CS_HG_GC	, (0xff00), 8	, _model( 16	, 6	)),
	_effect("cs_blue"		, CS_HG_BM	, (0xff00), 8	, _model( 16	, 8	)),

	_effect("cs_yellow"		, CS_HG_RY	, (0x00ff), 0	, _model( 16	, 18	)),
	_effect("cs_cyan"		, CS_HG_GC	, (0x00ff), 0	, _model( 16	, 22	)),
	_effect("cs_magenta"		, CS_HG_BM	, (0x00ff), 0	, _model( 16	, 6	)),
	
	/* Colour channel pass-through filters */
	_effect("scr_red_red"		, SCR_RR_CR	, (0xff00), 8	, _model( 255	, 239	)),
	_effect("scr_red_green"		, SCR_RG_CG	, (0xff00), 8	, _model( 0	, 18	)),
	_effect("scr_red_blue"		, SCR_RB_CB	, (0xff00), 8	, _model( 0	, 0	)),

	_effect("scr_cyan_red"		, SCR_RR_CR	, (0x00ff), 0	, _model( 0	, 159	)),
	_effect("scr_cyan_green"	, SCR_RG_CG	, (0x00ff), 0	, _model( 255	, 246	)),
	_effect("scr_cyan_blue"		, SCR_RB_CB	, (0x00ff), 0	, _model( 255	, 255	)),
	
	_effect("scr_green_red"		, SCR_GR_MR	, (0xff00), 8	, _model( 0	, 95	)),
	_effect("scr_green_green"	, SCR_GG_MG	, (0xff00), 8	, _model( 255	, 223	)),
	_effect("scr_green_blue"	, SCR_GB_MB	, (0xff00), 8	, _model( 0	, 15	)),

	_effect("scr_magenta_red"	, SCR_GR_MR	, (0x00ff), 0	, _model( 255	, 255	)),
	_effect("scr_magenta_green"	, SCR_GG_MG	, (0x00ff), 0	, _model( 0	, 28	)),
	_effect("scr_magenta_blue"	, SCR_GB_MB	, (0x00ff), 0	, _model( 255	, 255	)),
	
	_effect("scr_blue_red"		, SCR_BR_YR	, (0xff00), 8	, _model( 0	, 18	)),
	_effect("scr_blue_green"	, SCR_BG_YG	, (0xff00), 8	, _model( 0	, 0	)),
	_effect("scr_blue_blue"		, SCR_BB_YB	, (0xff00), 8	, _model( 255	, 246	)),

	_effect("scr_yellow_red"	, SCR_BR_YR	, (0x00ff), 0	, _model( 255	, 255	)),
	_effect("scr_yellow_green"	, SCR_BG_YG	, (0x00ff), 0	, _model( 255	, 239	)),
	_effect("scr_yellow_blue"	, SCR_BB_YB	, (0x00ff), 0	, _model( 0	, 90	)),

	_effect("scr_black_red"		, SCR_KR_WR	, (0xff00), 8	, _model( 0	, 0	)),
	_effect("scr_black_green"	, SCR_KG_WG	, (0xff00), 8	, _model( 0	, 0	)),
	_effect("scr_black_blue"	, SCR_KB_WB	, (0xff00), 8	, _model( 0	, 0	)),

	_effect("scr_white_red"		, SCR_KR_WR	, (0x00ff), 0	, _model( 255	, 255	)),
	_effect("scr_white_green"	, SCR_KG_WG	, (0x00ff), 0	, _model( 255	, 236	)),
	_effect("scr_white_blue"	, SCR_KB_WB	, (0x00ff), 0	, _model( 255	, 243	)),

	/* MCM */
	_effect("mcm_temperature"	, MCM_TEMPERATURE, (0x00ff), 0	, 100	),

	_effect("mcm_9_left"		, MCM_9		, (0xff00), 8	, 160	),
	_effect("mcm_9_right"		, MCM_9		, (0x00ff), 0	, 139	),

	_effect("mcm_B_left"		, MCM_9		, (0xff00), 8	, 122	),
	_effect("mcm_B_right"		, MCM_9		, (0x00ff), 0	, 122	),

	/* UC */
	_effect("UC_y"			, UC_Y		, (0xffff), 0	, 448	),
	_effect("UC_chroma_saturation"	, UC_CS		, (0xffff), 0	, 511	),

	/* Greyscale gamma curve */
	_effect("cc_channel_selection"	, CC_CHSEL_STR	, (0xff00), 8	, 0	),
	_effect("cc_channel_strength"	, CC_CHSEL_STR	, (0x00ff), 0	, 128	),
	
	_effect("cc_0"			, CC_0		, (0x00ff), 0	, 0	),
	_effect("cc_16"			, CC_1		, (0xff00), 8	, 16	),
	_effect("cc_32"			, CC_2		, (0xff00), 8	, 32	),
	_effect("cc_48"			, CC_3		, (0xff00), 8	, 48	),
	_effect("cc_64"			, CC_4		, (0xff00), 8	, 64	),
	_effect("cc_80"			, CC_5		, (0xff00), 8	, 80	),
	_effect("cc_96"			, CC_6		, (0xff00), 8	, 96	),
	_effect("cc_112"		, CC_7		, (0xff00), 8	, 112	),
	_effect("cc_128"		, CC_8		, (0xff00), 8	, 128	),
	_effect("cc_144"		, CC_1		, (0x00ff), 0	, 144	),
	_effect("cc_160"		, CC_2		, (0x00ff), 0	, 160	),
	_effect("cc_176"		, CC_3		, (0x00ff), 0	, 176	),
	_effect("cc_192"		, CC_4		, (0x00ff), 0	, 192	),
	_effect("cc_208"		, CC_5		, (0x00ff), 0	, 208	),
	_effect("cc_224"		, CC_6		, (0x00ff), 0	, 224	),
	_effect("cc_240"		, CC_7		, (0x00ff), 0	, 240	),
	_effect("cc_255"		, CC_8		, (0x00ff), 0	, 255	),
};

static int is_switch(unsigned int reg)
{
	switch(reg) {
		case EFFECT_MASTER:
		case FA_MASTER:
			return true;
		default:
			return false;
	}
}

static ssize_t show_mdnie_property(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mdnie_effect *effect = (struct mdnie_effect*)(attr);


	if(is_switch(effect->reg))
		return sprintf(buf, "%d", effect->value);
	
	return sprintf(buf, "%s %d", effect->delta ? "delta" : "override", effect->value);
};

static ssize_t store_mdnie_property(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct mdnie_effect *effect = (struct mdnie_effect*)(attr);
	int val, ret;
	
	if(sscanf(buf, "%d", &val) != 1) {
		char *s = kzalloc(10 * sizeof(char), GFP_KERNEL);
		if(sscanf(buf, "%10c", s) != 1) {
			ret = -EINVAL;
		} else {
			printk("inputted: '%s'\n", s);

			if(strncmp(s, "override", 8)) {
				effect->delta = 0;
				ret = count;
			}
			
			if(strncmp(s, "delta", 5)) {
				effect->delta = 1;
				ret = count;
			}

			ret = -EINVAL;
		}

		kfree(s);
		if(ret != -EINVAL)
			goto refresh;
		return ret;
	}

	if(is_switch(effect->reg)) {
		effect->value = val;
	} else {
		if(val > (effect->mask >> effect->shift))
			val = (effect->mask >> effect->shift);

		if(val < -(effect->mask >> effect->shift))
			val = -(effect->mask >> effect->shift);

		effect->value = val;
	}

refresh:
	cancel_delayed_work_sync(&mdnie_refresh_work);
	schedule_delayed_work_on(0, &mdnie_refresh_work, REFRESH_DELAY);

	return count;
};

static ssize_t show_reg_hook(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", reg_hook);
};

static ssize_t store_reg_hook(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int val;
	
	if(sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	reg_hook = !!val;

	cancel_delayed_work_sync(&mdnie_refresh_work);
	schedule_delayed_work_on(0, &mdnie_refresh_work, REFRESH_DELAY);

	return count;
};
static ssize_t show_sequence_hook(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", scenario_hook);
};

static ssize_t store_sequence_hook(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int val;
	
	if(sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	scenario_hook = !!val;

	cancel_delayed_work_sync(&mdnie_refresh_work);
	schedule_delayed_work_on(0, &mdnie_refresh_work, REFRESH_DELAY);

	return count;
};

unsigned short mdnie_reg_hook(unsigned short reg, unsigned short value)
{
	struct mdnie_effect *effect = (struct mdnie_effect*)&mdnie_controls;
	int i;
	int tmp, original;
	unsigned short regval;

	original = value;

	if(!scenario_hook && !reg_hook)
		return value;

	for(i = 0; i < ARRAY_SIZE(mdnie_controls); i++) {
	    if(effect->reg == reg) {
		if(scenario_hook) {
			tmp = regval = effect->regval;
		} else {
			tmp = regval = (value & effect->mask) >> effect->shift;
		}

		if(reg_hook) {
			if(is_switch(reg)) {
				if(reg == EFFECT_MASTER && effect->shift < 3) {
					if((1 << mdnie->scenario) & effect->value) {
						tmp = effect->value ? !regval : regval;
					}
				} else {
					tmp = effect->value ? !regval : regval;
				}
			} else {
				if(effect->delta) {
					tmp += effect->value;
				} else {
					tmp = effect->value;
				}
			}

			if(tmp > (effect->mask >> effect->shift))
				tmp = (effect->mask >> effect->shift);

			if(tmp < 0)
				tmp = 0;

			regval = (unsigned short)tmp;
		}

		value &= ~effect->mask;
		value |= regval << effect->shift;

/*
		printk("mdnie: hook on: 0x%X val: 0x%X -> 0x%X effect:%4d\n",
			reg, original, value, tmp);
*/
	    }
	    ++effect;
	}
	
	return value;
}

unsigned short *mdnie_sequence_hook(struct mdnie_info *pmdnie, unsigned short *seq)
{
	if(mdnie == NULL)
		mdnie = pmdnie;

	if(!scenario_hook)
		return seq;

	return (unsigned short *)&master_sequence;
}

static void do_mdnie_refresh(struct work_struct *work)
{
	set_mdnie_value(g_mdnie, 1);
}

const struct device_attribute master_switch_attr[] = {
	{ 
		.attr = { .name = "hook_intercept",
			  .mode = S_IRUGO | S_IWUSR | S_IWGRP, },
		.show = show_reg_hook,
		.store = store_reg_hook,
	},{
		.attr = { .name = "sequence_intercept",
			  .mode = S_IRUGO | S_IWUSR | S_IWGRP, },
		.show = show_sequence_hook,
		.store = store_sequence_hook,
	},
};

void init_intercept_control(struct kobject *kobj)
{
	int i, ret;
	struct kobject *subdir;

	subdir = kobject_create_and_add("hook_control", kobj);

	for(i = 0; i < ARRAY_SIZE(mdnie_controls); i++) {
		ret = sysfs_create_file(subdir, &mdnie_controls[i].attribute.attr);
	}

	ret = sysfs_create_file(kobj, &master_switch_attr[0].attr);
	ret = sysfs_create_file(kobj, &master_switch_attr[1].attr);

	INIT_DELAYED_WORK(&mdnie_refresh_work, do_mdnie_refresh);
}
