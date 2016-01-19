/* linux/arch/arm/mach-exynos/asv-4x12.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS4X12 - ASV(Adaptive Supply Voltage) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/asv.h>
#include <mach/map.h>
#include <plat/cpu.h>

/* ASV function for Fused Chip */
#define IDS_ARM_OFFSET		24
#define IDS_ARM_MASK		0xFF
#define HPM_OFFSET		12
#define HPM_MASK		0x1F

#define FUSED_SG_OFFSET		3
#define ORIG_SG_OFFSET		17
#define ORIG_SG_MASK		0xF
#define MOD_SG_OFFSET		21
#define MOD_SG_MASK		0x7

#define LOCKING_OFFSET		7
#define LOCKING_MASK		0x1F

#define EMA_OFFSET		6
#define EMA_MASK		0x1

#define DEFAULT_ASV_GROUP	1

#define CHIP_ID_REG		(S5P_VA_CHIPID + 0x4)

unsigned int exynos_armclk_max;

struct asv_judge_table exynos4x12_limit[] = {
	/* HPM, IDS */
	{  0,   0},		/* Reserved Group */
	{  0,   0},		/* Reserved Group */
	{ 14,   9},
	{ 16,  14},
	{ 18,  17},
	{ 20,  20},
	{ 21,  24},
	{ 22,  30},
	{ 23,  34},
	{ 24,  39},
	{100, 100},
	{999, 999},		/* Reserved Group */
};

struct asv_judge_table exynos4x12_limit_rev2[] = {
	/* HPM, IDS */
	{  0,   3},		/* Reserved Group */
	{ 15,   8},		/* ASV1 Group */
	{ 16,  11},
	{ 18,  14},
	{ 19,  18},
	{ 20,  22},
	{ 21,  26},
	{ 22,  29},
	{ 23,  36},
	{ 24,  40},
	{ 25,  45},
	{ 26,  50},
	{999, 999},		/* ASV11 Group */
};

struct asv_judge_table exynos4212_limit[] = {
	/* HPM, IDS */
	{ 15,	6}, 	/* Reserved Group */
	{ 15,	9},
	{ 17,  12},
	{ 20,  14},
	{ 22,  18},
	{ 24,  22},
	{ 26,  31},
	{ 27,  35},
	{ 28,  39},
	{100,  44},
	{999,  49}, 	/* Reserved Group */
	{999, 999}, 	/* Reserved Group */
};

static int exynos4x12_get_hpm(struct samsung_asv *asv_info)
{
	asv_info->hpm_result = (asv_info->pkg_id >> HPM_OFFSET) & HPM_MASK;

	return 0;
}

static int exynos4x12_get_ids(struct samsung_asv *asv_info)
{
	asv_info->ids_result = (asv_info->pkg_id >> IDS_ARM_OFFSET) & IDS_ARM_MASK;

	return 0;
}

static void exynos4x12_pre_set_abb(void)
{
	if (soc_is_exynos4212()) {
			switch (exynos_result_of_asv) {
			case 0:
			case 1:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_080V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_100V);
				break;
			case 2:
			case 3:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_140V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_100V);
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
	 		case 11:
			case 12:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_140V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_130V);
				break;
	 		default:
 				exynos4x12_set_abb(ABB_MODE_130V);
 			break;
			}
	} else if (soc_is_exynos4412()) {
		if (samsung_rev() >= EXYNOS4412_REV_2_0) {
			switch (exynos_result_of_asv) {
			case 0:
			case 1:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_075V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_100V);
				break;
			case 2:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_100V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_140V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_100V);
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_140V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_100V);
				break;
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_INT, ABB_MODE_130V);
				exynos4x12_set_abb_member(ABB_MIF, ABB_MODE_140V);
				exynos4x12_set_abb_member(ABB_G3D, ABB_MODE_130V);
				break;
			default:
				exynos4x12_set_abb(ABB_MODE_130V);
				break;
			}
		} else {
			switch (exynos_result_of_asv) {
			case 0:
			case 1:
			case 2:
			case 3:
				exynos4x12_set_abb(ABB_MODE_100V);
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				exynos4x12_set_abb(ABB_MODE_130V);
				break;
			default:
				exynos4x12_set_abb(ABB_MODE_130V);
				break;
			}
			}
	} else {
		pr_err("%s: Can't find SoC type \n", __func__);
	}
}

static int exynos4x12_asv_store_result(struct samsung_asv *asv_info)
{
	unsigned int i;

	if (soc_is_exynos4412()) {
		if (samsung_rev() >= EXYNOS4412_REV_2_0) {
			for (i = 0; i < ARRAY_SIZE(exynos4x12_limit_rev2); i++) {
				if ((asv_info->ids_result <= exynos4x12_limit_rev2[i].ids_limit) ||
				    (asv_info->hpm_result <= exynos4x12_limit_rev2[i].hpm_limit)) {
						exynos_result_of_asv = i;
					break;
				}
			}
		} else {
			for (i = 0; i < ARRAY_SIZE(exynos4x12_limit); i++) {
				if ((asv_info->ids_result <= exynos4x12_limit[i].ids_limit) ||
				    (asv_info->hpm_result <= exynos4x12_limit[i].hpm_limit)) {
						exynos_result_of_asv = i;
					break;
				}
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(exynos4212_limit); i++) {
			if ((asv_info->ids_result <= exynos4212_limit[i].ids_limit) ||
			    (asv_info->hpm_result <= exynos4212_limit[i].hpm_limit)) {
				exynos_result_of_asv = i;
				break;
			}
		}

	}

	/*
	 * If ASV result value is lower than default value
	 * Fix with default value.
	 */
	if (samsung_rev() < EXYNOS4412_REV_2_0) {
		if (exynos_result_of_asv < DEFAULT_ASV_GROUP)
			exynos_result_of_asv = DEFAULT_ASV_GROUP;
	}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	pr_info("EXYNOS4X12(NO SG): IDS : %d HPM : %d RESULT : %d\n",
		asv_info->ids_result, asv_info->hpm_result, exynos_result_of_asv);
#endif

	exynos4x12_pre_set_abb();

	return 0;
}

int exynos4x12_asv_init(struct samsung_asv *asv_info)
{
	unsigned int tmp;
	unsigned int exynos_orig_sp;
	unsigned int exynos_mod_sp;
	int exynos_cal_asv;

	exynos_result_of_asv = 0;
	exynos_special_flag = 0;
	exynos_dynamic_ema = false;

	pr_info("EXYNOS4X12: Adaptive Support Voltage init\n");

	tmp = __raw_readl(CHIP_ID_REG);

	/* Store PKG_ID */
	asv_info->pkg_id = tmp;

#ifdef CONFIG_EXYNOS4X12_1000MHZ_SUPPORT
	exynos_armclk_max = 1000000;
#else
	/* If maximum armclock is fused, set its value */
	if (samsung_rev() < EXYNOS4412_REV_2_0) {
		switch (tmp & MOD_SG_MASK) {
		case 0:
		case 3:
			exynos_armclk_max = 1400000;
			break;
		case 2:
			exynos_armclk_max = 1000000;
			break;
		default:
			exynos_armclk_max = 1400000;
			break;
		}
	}
#endif

	if ((tmp >> EMA_OFFSET) & EMA_MASK)
		exynos_dynamic_ema = true;
	else
		exynos_dynamic_ema = false;

	/* If Speed group is fused, get speed group from */
	if ((tmp >> FUSED_SG_OFFSET) & 0x1) {
		exynos_orig_sp = (tmp >> ORIG_SG_OFFSET) & ORIG_SG_MASK;
		exynos_mod_sp = (tmp >> MOD_SG_OFFSET) & MOD_SG_MASK;

		exynos_cal_asv = exynos_orig_sp - exynos_mod_sp;

		if (soc_is_exynos4212()) {
			if (exynos_cal_asv < 0)
				exynos_result_of_asv = DEFAULT_ASV_GROUP;
			else
				exynos_result_of_asv = exynos_cal_asv;
		} else {
			/*
			 * If There is no origin speed group,
			 * store 1 asv group into exynos_result_of_asv.
			 */
			if (!exynos_orig_sp) {
				pr_info("EXYNOS4X12: No Origin speed Group\n");
				exynos_result_of_asv = DEFAULT_ASV_GROUP;
			} else {
				if (exynos_cal_asv < DEFAULT_ASV_GROUP)
					exynos_result_of_asv = DEFAULT_ASV_GROUP;
				else
					exynos_result_of_asv = exynos_cal_asv;
			}
		}
		
		pr_info("EXYNOS4X12(SG):  ORIG : %d MOD : %d RESULT : %d\n",
			exynos_orig_sp, exynos_mod_sp, exynos_result_of_asv);

		/*
		 * If fused speed group is 1 and ids value is lower than 3,
		 * voltage value should be set to asv 0 group.
		 */
		if (samsung_rev() >= EXYNOS4412_REV_2_0) {
			if (exynos_result_of_asv == 1) {
				exynos4x12_get_ids(asv_info);
				if ((asv_info->ids_result <= exynos4x12_limit_rev2[0].ids_limit))
					exynos_result_of_asv = 0;
			}
		}

		/* set Special flag into exynos_special_flag */
		exynos_special_flag = (tmp >> LOCKING_OFFSET) & LOCKING_MASK;

		exynos4x12_pre_set_abb();

		return -EEXIST;
	}

	/* set Special flag into exynos_special_flag */
	exynos_special_flag = (tmp >> LOCKING_OFFSET) & LOCKING_MASK;

	asv_info->get_ids = exynos4x12_get_ids;
	asv_info->get_hpm = exynos4x12_get_hpm;
	asv_info->store_result = exynos4x12_asv_store_result;

	return 0;
}
