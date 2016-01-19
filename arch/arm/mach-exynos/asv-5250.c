/* linux/arch/arm/mach-exynos/asv-4x12.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS5250 - ASV(Adaptive Supply Voltage) driver
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
#include <linux/string.h>

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

#define DEFAULT_ASV_GROUP	1

#define CHIP_ID_REG		(S5P_VA_CHIPID + 0x4)
#define LOT_ID_REG		(S5P_VA_CHIPID + 0x14)

struct asv_judge_table exynos5250_limit[] = {
	/* HPM, IDS */
	{  0,   0},		/* Reserved Group */
	{  9,   7},
	{ 10,   9},
	{ 12,  11},
	{ 14,  14},
	{ 15,  17},
	{ 16,  20},
	{ 17,  23},
	{ 18,  27},
	{ 19,  30},
	{100, 100},
	{999, 999},		/* Reserved Group */
};

static int exynos5250_get_hpm(struct samsung_asv *asv_info)
{
	asv_info->hpm_result = (asv_info->pkg_id >> HPM_OFFSET) & HPM_MASK;

	return 0;
}

static int exynos5250_get_ids(struct samsung_asv *asv_info)
{
	asv_info->ids_result = (asv_info->pkg_id >> IDS_ARM_OFFSET) & IDS_ARM_MASK;

	return 0;
}

/*
 * If lot id is "NZVPU", it is need to modify for ARM_IDS value
 */
static int exynos5250_check_lot_id(void)
{
	unsigned int lid_reg = 0;
	unsigned int rev_lid = 0;
	unsigned int i;
	unsigned int tmp;
	char lot_id[5];

	lid_reg = __raw_readl(LOT_ID_REG);

	for (i = 0; i < 32; i++) {
		tmp = (lid_reg >> i) & 0x1;
		rev_lid += tmp << (31 - i);
	}

	lot_id[0] = 'N';
	lid_reg = (rev_lid >> 11) & 0x1FFFFF;

	for (i = 4; i >= 1; i--) {
		tmp = lid_reg % 36;
		lid_reg /= 36;
		lot_id[i] = (tmp < 10) ? (tmp + '0') : ((tmp - 10) + 'A');
	}

	return strncmp(lot_id, "NZVPU", ARRAY_SIZE(lot_id));
}

static void exynos5250_pre_set_abb(void)
{
	switch (exynos_result_of_asv) {
	case 0:
	case 1:
	case 2:
		exynos4x12_set_abb(ABB_MODE_080V);
		break;
	case 3:
	case 4:
		exynos4x12_set_abb(ABB_MODE_BYPASS);
		break;
	default:
		exynos4x12_set_abb(ABB_MODE_130V);
		break;
	}
}

static int exynos5250_asv_store_result(struct samsung_asv *asv_info)
{
	unsigned int i;

	if (!exynos5250_check_lot_id())
		asv_info->ids_result -= 15;

	if (soc_is_exynos5250()) {
		for (i = 0; i < ARRAY_SIZE(exynos5250_limit); i++) {
			if ((asv_info->ids_result <= exynos5250_limit[i].ids_limit) ||
			    (asv_info->hpm_result <= exynos5250_limit[i].hpm_limit)) {
				exynos_result_of_asv = i;
				break;
			}
		}
	}

	/*
	 * If ASV result value is lower than default value
	 * Fix with default value.
	 */
	if (exynos_result_of_asv < DEFAULT_ASV_GROUP)
		exynos_result_of_asv = DEFAULT_ASV_GROUP;

	pr_info("EXYNOS5250(NO SG): IDS : %d HPM : %d RESULT : %d\n",
		asv_info->ids_result, asv_info->hpm_result, exynos_result_of_asv);

	exynos5250_pre_set_abb();

	return 0;
}

int exynos5250_asv_init(struct samsung_asv *asv_info)
{
	unsigned int tmp;
	unsigned int exynos_orig_sp;
	unsigned int exynos_mod_sp;
	int exynos_cal_asv;

	exynos_result_of_asv = 0;

	pr_info("EXYNOS5250: Adaptive Support Voltage init\n");

	tmp = __raw_readl(CHIP_ID_REG);

	/* Store PKG_ID */
	asv_info->pkg_id = tmp;

	/* If Speed group is fused, get speed group from */
	if ((tmp >> FUSED_SG_OFFSET) & 0x1) {
		exynos_orig_sp = (tmp >> ORIG_SG_OFFSET) & ORIG_SG_MASK;
		exynos_mod_sp = (tmp >> MOD_SG_OFFSET) & MOD_SG_MASK;

		exynos_cal_asv = exynos_orig_sp - exynos_mod_sp;
		/*
		 * If There is no origin speed group,
		 * store 1 asv group into exynos_result_of_asv.
		 */
		if (!exynos_orig_sp) {
			pr_info("EXYNOS5250: No Origin speed Group\n");
			exynos_result_of_asv = DEFAULT_ASV_GROUP;
		} else {
			if (exynos_cal_asv < DEFAULT_ASV_GROUP)
				exynos_result_of_asv = DEFAULT_ASV_GROUP;
			else
				exynos_result_of_asv = exynos_cal_asv;
		}

		pr_info("EXYNOS5250(SG):  ORIG : %d MOD : %d RESULT : %d\n",
			exynos_orig_sp, exynos_mod_sp, exynos_result_of_asv);

		return -EEXIST;
	}

	asv_info->get_ids = exynos5250_get_ids;
	asv_info->get_hpm = exynos5250_get_hpm;
	asv_info->store_result = exynos5250_asv_store_result;

	return 0;
}
