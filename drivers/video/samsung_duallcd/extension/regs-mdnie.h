/* linux/arch/arm/plat-s5p/include/plat/regs-mdnie.h
 *
 * Header file for Samsung SoC mDNIe device.
 *
 * Copyright (c) 2010 Samsung Electronics
 *
 * Author : Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_MDNIE_H
#define _REGS_MDNIE_H

#define MDNIE_R0		0x0000
#define MDNIE_R1		0x0004
#define MDNIE_R2		0x0008
#define MDNIE_R3		0x000C
#define MDNIE_R4		0x0010
#define MDNIE_R5		0x0014
#define MDNIE_R28		0x00A0

/* R1 */
#define MDNIE_R1_MCM_BYPASS_MODE	(1 << 2)
#define MDNIE_R1_ALG_DNR_HDTR_MASK	(0x3 << 0)
#define MDNIE_R1_ALG_DNR_HDTR(x)	(((x) & 0x3) << 0)
#define MDNIE_R1_ROI_PCA_OVE		(1 << 10)
#define MDNIE_R1_ROI_HDTR		(1 << 9)
#define MDNIE_R1_ROI_DNR		(1 << 8)
#define MDNIE_R1_ROI_OUTSIDE		(1 << 7)
#define MDNIE_R1_ABC_SEL_MASK		(0x3 << 4)
#define MDNIE_R1_ABC_SEL(x)		(((x) & 0x3) << 4)

/* R2 */
#define MDNIE_R2_H_START_MASK		(0x7ff << 0)
#define MDNIE_R2_H_START(x)		(((x) & 0x7ff) << 0)

/* R3 */
#define MDNIE_R3_WIDTH_MASK		(0x7ff << 0)

/* R4 */
#define MDNIE_R4_HEIGHT_MASK		(0x7ff << 0)

/* R5 */
#define MDNIE_R5_V_END_MASK		(0x7ff << 0)
#define MDNIE_R5_V_END(x)		(((x) & 0x7ff) << 0)

/* R6 */
#define MDNIE_R6_DITHER_ENABLE		(1 << 4)

/* R34 */
#define MDNIE_R34_WIDTH_MASK		(0x7ff << 0)
#define MDNIE_R34_WIDTH(x)		(((x) & 0x7ff) << 0)

/* R35 */
#define MDNIE_R35_HEIGHT_MASK		(0x7ff << 0)
#define MDNIE_R35_HEIGHT(x)		(((x) & 0x7ff) << 0)

/* R44 */
#define MDNIE_R44_DNR_BYPASS_MODE	(1 << 14)

/* R58 */
#define MDNIE_R58_HDTR_BYPASS_MODE	(0x1f << 0)

/* R73 */
#define MDNIE_R73_SN_LVL_MASK		(0x3 << 10)
#define MDNIE_R73_SN_LVL(x)		(((x) & 0x3) << 10)
#define MDNIE_R73_SY_LVL_MASK		(0x3 << 8)
#define MDNIE_R73_SY_LVL(x)		(((x) & 0x3) << 8)
#define MDNIE_R73_GR_LVL_MASK		(0x3 << 6)
#define MDNIE_R73_GR_LVL(x)		(((x) & 0x3) << 6)
#define MDNIE_R73_RD_LVL_MASK		(0x3 << 4)
#define MDNIE_R73_RD_LVL(x)		(((x) & 0x3) << 4)
#define MDNIE_R73_YE_LVL_MASK		(0x3 << 2)
#define MDNIE_R73_YE_LVL(x)		(((x) & 0x3) << 2)
#define MDNIE_R73_PU_LVL_MASK		(0x3 << 0)
#define MDNIE_R73_PU_LVL(x)		(((x) & 0x3) << 0)

/* R82 */
#define MDNIE_R82_SN_CC_OFF		(1 << 5)
#define MDNIE_R82_SY_CC_OFF		(1 << 4)
#define MDNIE_R82_GR_CC_OFF		(1 << 3)
#define MDNIE_R82_RD_CC_OFF		(1 << 2)
#define MDNIE_R82_YE_CC_OFF		(1 << 1)
#define MDNIE_R82_PU_CC_OFF		(1 << 0)

/* R84 */
#define MDNIE_R84_LIGHT_P_MASK		(0xff << 8)
#define MDNIE_R84_LIGHT_P(x)		(((x) & 0xff) << 8)
#define MDNIE_R84_CHROMA_P_MASK		(0xff << 0)
#define MDNIE_R84_CHROMA_P(x)		(((x) & 0xff) << 0)

/* R91 */
#define MDNIE_R91_QUADRANT_ON		(1 << 8)
#define MDNIE_R91_COLOR_TEMP_DEST_MASK	(0xff << 0)
#define MDNIE_R91_COLOR_TEMP_DEST(x)	(((x) & 0xff) << 0)

/* R106 */
#define MDNIE_R106_QUADRANT_TMP1_MASK	(0xff << 8)
#define MDNIE_R106_QUADRANT_TMP1(x)	(((x) & 0xff) << 8)
#define MDNIE_R106_QUADRANT_TMP2_MASK	(0xff << 0)
#define MDNIE_R106_QUADRANT_TMP2(x)	(((x) & 0xff) << 0)

/* R107 */
#define MDNIE_R107_QUADRANT_TMP3_MASK	(0xff << 8)
#define MDNIE_R107_QUADRANT_TMP3(x)	(((x) & 0xff) << 8)
#define MDNIE_R107_QUADRANT_TMP4_MASK	(0xff << 0)
#define MDNIE_R107_QUADRANT_TMP4(x)	(((x) & 0xff) << 0)

/* R124 */
#define MDNIE_R124_CABC_BLU_ENABLE	(1 << 1)
#define MDNIE_R124_DISPLAY_SEL_OLED	(1 << 0)

/* R125 */
#define MDNIE_R125_ALS_FLAG_UPDATED	(1 << 0)

/* R126 */
#define MDNIE_R126_ALS_DATA_MASK	(0xffff << 0)
#define MDNIE_R126_ALS_DATA(x)		(((x) & 0xffff) << 0)

/* R127 */
#define MDNIE_R127_WIN_SIZE_MASK	(0xf << 4)
#define MDNIE_R127_WIN_SIZE(x)		(((x) & 0xf) << 4)
#define MDNIE_R127_ALS_MODE_MASK	(0x3 << 0)
#define MDNIE_R127_ALS_MODE(x)		(((x) & 0x3) << 0)

/* R130 */
#define MDNIE_R130_AMB_LVL_MASK		(0x4 << 0)
#define MDNIE_R130_AMB_LVL(x)		(((x) & 0x4) << 0)

/* R179 */
#define MDNIE_R179_UP_SL_MASK		(0xff << 8)
#define MDNIE_R179_UP_SL(x)		(((x) & 0xff) << 8)
#define MDNIE_R179_DOWN_SL_MASK		(0xff << 0)
#define MDNIE_R179_DOWN_SL(x)		(((x) & 0xff) << 0)

/* R180 */
#define MDNIE_R180_PWM_CE_PWM_COEFF	(1 << 15)
#define MDNIE_R180_POLARITY_HIGH_ACTIVE	(1 << 14)
#define MDNIE_R180_LABC_MODE_MASK	(0x3 << 12)
#define MDNIE_R180_LABC_MODE(x)		(((x) & 0x3) << 12)
#define MDNIE_R180_ALC_EN		(1 << 11)
#define MDNIE_R180_PWM_COEFF_COUNT_MASK	(0x7ff << 0)
#define MDNIE_R180_PWM_COEFF_COUNT(x)	(((x) & 0x7ff) << 0)

/* R238 */
#define MDNIE_R238_ROI_DITHER		(1 << 15)
#define MDNIE_R238_ROI_OUTSIDE		(1 << 14)
#define MDNIE_R238_H_START_MASK		(0x7ff << 0)
#define MDNIE_R238_H_START(x)		(((x) & 0x7ff) << 0)

/* R239 */
#define MDNIE_R239_H_END_MASK		(0x7ff << 0)
#define MDNIE_R239_H_END(x)		(((x) & 0x7ff) << 0)

/* R240 */
#define MDNIE_R240_V_START_MASK		(0x7ff << 0)
#define MDNIE_R240_V_START(x)		(((x) & 0x7ff) << 0)

/* R241 */
#define MDNIE_R241_V_END_MASK		(0x7ff << 0)
#define MDNIE_R241_V_END(x)		(((x) & 0x7ff) << 0)

#endif
