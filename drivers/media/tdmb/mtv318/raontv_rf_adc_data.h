/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv_rf_adc_data.h
 *
 * Description : RAONTECH TV RF ADC data header file.
 *
 * Copyright (C) (2011, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#if (RTV_SRC_CLK_FREQ_KHz == 13000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/*Based 13MHz, 8MHz*/
	{0x0D, 0x01, 0x1F, 0x27, 0x07, 0x80, 0xB9},
	/*Based 13MHz, 8.192MHz*/ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/*Based 13MHz, 9MHz*/
	{0x0D, 0x01, 0x1F, 0x27, 0x07, 0xB0, 0xB9},
	/*Based 13MHz, 9.6MHz*/ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x99}, {0x39, 0x9C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x70}, {0x39, 0x5C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x70}, {0x39, 0x5C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x70}, {0x39, 0x6C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 16000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 16MHz, 8MHz External Clock4 */
	{0x04, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 16MHz, 8.192MHz*/ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 16MHz, 9MHz External Clock5 */
	{0x04, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 16MHz, 9.6MHz External Clock6 */
	{0x05, 0x01, 0x1F, 0x27, 0x07, 0x90, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x7D}, {0x39, 0x7C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x5B}, {0x39, 0x4C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x5B}, {0x39, 0x4C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x5B}, {0x39, 0x5C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 16384)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 16.384MHz, 8MHz External Clock8 */
	{0x10, 0x01, 0x1F, 0x27, 0x07, 0x77, 0xB9},
	/* Based 16.384MHz, 8.192MHz External Clock7 */
	{0x04, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 16.384MHz, 9MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 16.384MHz, 9.6MHz External Clock9 */
	{0x08, 0x01, 0x1F, 0x27, 0x06, 0xE1, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x7A}, {0x39, 0x7C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz
};
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x59}, {0x39, 0x4C}
};

static const enum E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_TDMB[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/* 175280: 7A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 177008: 7B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 178736: 7C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 181280: 8A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 183008: 8B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 184736: 8C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 187280: 9A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 189008: 9B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 190736: 9C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 193280: 10A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 195008: 10B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 196736: 10C */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 199280: 11A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 201008: 11B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 202736: 11C */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 205280: 12A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 207008: 12B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 208736: 12C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 211280: 13A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 213008: 13B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 214736: 13C */
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x59}, {0x39, 0x4C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_B3[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*5A : 174928*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5B : 176640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*5C : 178352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5D : 180064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6A : 181936*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*6B : 183648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6C : 185360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6D : 187072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7A : 188928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7B : 190640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7C : 192352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7D : 194064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8A : 195936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8B : 197648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8C : 199360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8D : 201072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9A : 202928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9B : 204640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9C : 206352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*9D : 208064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10A: 209936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10N: 210096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10B: 211648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10C: 213360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10D: 215072*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*11A: 216928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11N: 217008*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11B: 218640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11C: 220352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11D: 222064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12A: 223936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12N: 224096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12B: 225648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12C: 227360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12D: 229072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13A: 230784*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13B: 232496*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13C: 234208*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13D: 235776*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13E: 237488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13F: 239200*/
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_L[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*LA: 1452960*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LB: 1454672*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LC: 1456384*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LD: 1458096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LE: 1459808*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LF: 1461520*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LG: 1463232*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LH: 1464944*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LI: 1466656*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LJ: 1468368*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LK: 1470080*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LL: 1471792*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LM: 1473504*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LN: 1475216*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LO: 1476928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LP: 1478640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LQ: 1480352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LR: 1482064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LS: 1483776*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LT: 1485488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LU: 1487200*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LV: 1488912*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LW: 1490624*/

};

#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x59}, {0x39, 0x4C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 18000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 18MHz, 8MHz External Clock10 */
	{0x06, 0x01, 0x13, 0x25, 0x06, 0x80, 0xB4},
	/* Based 18MHz, 8.192MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 18MHz, 9MHz External Clock11*/
	{0x06, 0x01, 0x13, 0x25, 0x06, 0x90, 0xB4},
	/* Based 18MHz, 9.6MHz External Clock12*/
	{0x05, 0x01, 0x13, 0x25, 0x06, 0x80, 0xB4}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x6F}, {0x39, 0x6C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x51}, {0x39, 0x4C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x51}, {0x39, 0x4C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x51}, {0x39, 0x4C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 19200)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 19.2MHz, 8MHz */
	{0x04, 0x01, 0x0B, 0x23, 0x06, 0x50, 0xB0},
	/* Based 19.2MHz, 8.192MHz */
	{0x19, 0x01, 0x1F, 0x3A, 0x0A, 0x00, 0xA2},
	/* Based 19.2MHz, 9MHz */
	{0x04, 0x01, 0x0B, 0x23, 0x06, 0x5A, 0xB0},
	/* Based 19.2MHz, 9.6MHz */
	{0x04, 0x01, 0x0B, 0x23, 0x06, 0x60, 0xB0}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x68}, {0x39, 0x6C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x4C}, {0x39, 0x3C}
};

static const enum E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_TDMB[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/* 175280: 7A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 177008: 7B */,
	RTV_ADC_CLK_FREQ_9_MHz/* 178736: 7C */,
	RTV_ADC_CLK_FREQ_9_MHz/* 181280: 8A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 183008: 8B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 184736: 8C */,
	RTV_ADC_CLK_FREQ_9_MHz/* 187280: 9A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 189008: 9B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 190736: 9C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 193280: 10A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 195008: 10B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 196736: 10C */,
	RTV_ADC_CLK_FREQ_9_MHz/* 199280: 11A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 201008: 11B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 202736: 11C */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 205280: 12A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 207008: 12B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 208736: 12C */,
	RTV_ADC_CLK_FREQ_9_MHz/* 211280: 13A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 213008: 13B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 214736: 13C */
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x4C}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_B3[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*5A : 174928*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5B : 176640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*5C : 178352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5D : 180064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6A : 181936*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*6B : 183648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6C : 185360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6D : 187072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7A : 188928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7B : 190640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7C : 192352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7D : 194064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8A : 195936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8B : 197648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8C : 199360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8D : 201072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9A : 202928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9B : 204640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9C : 206352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*9D : 208064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10A: 209936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10N: 210096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10B: 211648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10C: 213360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10D: 215072*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*11A: 216928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11N: 217008*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11B: 218640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11C: 220352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11D: 222064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12A: 223936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12N: 224096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12B: 225648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12C: 227360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12D: 229072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13A: 230784*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13B: 232496*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13C: 234208*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13D: 235776*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13E: 237488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13F: 239200*/
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_L[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*LA: 1452960*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LB: 1454672*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LC: 1456384*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LD: 1458096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LE: 1459808*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LF: 1461520*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LG: 1463232*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LH: 1464944*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LI: 1466656*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LJ: 1468368*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LK: 1470080*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LL: 1471792*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LM: 1473504*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LN: 1475216*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LO: 1476928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LP: 1478640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LQ: 1480352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LR: 1482064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LS: 1483776*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LT: 1485488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LU: 1487200*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LV: 1488912*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LW: 1490624*/
};

#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x4C}, {0x39, 0x4C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 24000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 24MHz, 8MHz External Clock17 */
	{0x06, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 24MHz, 8.192MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 24MHz, 9MHz External Clock18 */
	{0x06, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 24MHz, 9.6MHz External Clock19 */
	{0x05, 0x01, 0x0B, 0x23, 0x06, 0x60, 0xB0}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x53}, {0x39, 0x5C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x3D}, {0x39, 0x3C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x3D}, {0x39, 0x3C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x3D}, {0x39, 0x3C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 24576)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 24.576MHz, 8MHz External Clock21 */
	{0x08, 0x01, 0x13, 0x25, 0x06, 0x7D, 0xB4},
	/* Based 24.576MHz, 8.192MHz External Clock20 */
	{0x06, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 24.576MHz, 9MHz *//* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 24.576MHz, 9.6MHz External Clock22 */
	{0x0C, 0x01, 0x1F, 0x27, 0x06, 0xE1, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x51}, {0x39, 0x4C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_8_192_MHz
};
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x3B}, {0x39, 0x2C}
};

static const enum E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_TDMB[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/* 175280: 7A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 177008: 7B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 178736: 7C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 181280: 8A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 183008: 8B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 184736: 8C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 187280: 9A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 189008: 9B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 190736: 9C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 193280: 10A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 195008: 10B */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 196736: 10C */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 199280: 11A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 201008: 11B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 202736: 11C */,
	RTV_ADC_CLK_FREQ_9_6_MHz/* 205280: 12A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 207008: 12B */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 208736: 12C */,
	RTV_ADC_CLK_FREQ_8_192_MHz/* 211280: 13A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 213008: 13B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 214736: 13C */
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x3B}, {0x39, 0x2C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_B3[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*5A : 174928*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5B : 176640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*5C : 178352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*5D : 180064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6A : 181936*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*6B : 183648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6C : 185360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*6D : 187072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7A : 188928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7B : 190640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7C : 192352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*7D : 194064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8A : 195936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8B : 197648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*8C : 199360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8D : 201072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9A : 202928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9B : 204640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*9C : 206352*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*9D : 208064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10A: 209936*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10N: 210096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10B: 211648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10C: 213360*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*10D: 215072*/,
	RTV_ADC_CLK_FREQ_9_6_MHz/*11A: 216928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11N: 217008*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11B: 218640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*11C: 220352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11D: 222064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12A: 223936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12N: 224096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12B: 225648*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*12C: 227360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12D: 229072*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13A: 230784*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13B: 232496*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13C: 234208*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13D: 235776*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13E: 237488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*13F: 239200*/
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_L[] = {
	RTV_ADC_CLK_FREQ_8_192_MHz/*LA: 1452960*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LB: 1454672*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LC: 1456384*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LD: 1458096*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LE: 1459808*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LF: 1461520*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LG: 1463232*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LH: 1464944*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LI: 1466656*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LJ: 1468368*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LK: 1470080*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LL: 1471792*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LM: 1473504*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LN: 1475216*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LO: 1476928*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LP: 1478640*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LQ: 1480352*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LR: 1482064*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LS: 1483776*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LT: 1485488*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LU: 1487200*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LV: 1488912*/,
	RTV_ADC_CLK_FREQ_8_192_MHz/*LW: 1490624*/
};

#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x3B}, {0x39, 0x6C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 26000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 26MHz, 8MHz External Clock23*/
	{0x0D, 0x01, 0x1F, 0x27, 0x06, 0xC0, 0xB8},
	/* Based 26MHz, 8.192MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 26MHz, 9MHz External Clock24 */
	{0x0D, 0x01, 0x1F, 0x27, 0x06, 0xD8, 0xB8},
	/* Based 26MHz, 9.6MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x4C}, {0x39, 0x4C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x38}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x38}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x38}, {0x39, 0x3C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 27000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 27MHz, 8MHz External Clock25 */
	{0x09, 0x01, 0x13, 0x25, 0x06, 0x80, 0xB4},
	/* Based 27MHz, 8.192MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 27MHz, 9MHz External Clock26 */
	{0x06, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 27MHz, 9.6MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x4A}, {0x39, 0x4C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x36}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x36}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x36}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 32000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 32MHz, 8MHz External Clock27 */
	{0x08, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 32MHz, 8.192MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 32MHz, 9MHz External Clock28 */
	{0x08, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 32MHz, 9.6MHz External Clock29 */
	{0x0A, 0x01, 0x1F, 0x27, 0x07, 0x90, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x3E}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x2D}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x2D}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x2D}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 32768)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 32.768MHz, 8MHz External Clock31 */
	{0x20, 0x01, 0x1F, 0x27, 0x07, 0x77, 0xB9},
	/* Based 32.768MHz, 8.192MHz External Clock30 */
	{0x08, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 32.768MHz, 9MHz */ /* Unsupport Clock */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 32.768MHz, 9.6MHz External Clock32 */
	{0x10, 0x01, 0x1F, 0x27, 0x06, 0xE1, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x3D}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x2C}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x2C}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x2C}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 36000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 36MHz, 8MHz External Clock33 */
	{0x09, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 36MHz, 8.192MHz */ /* Unsupport */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 36MHz, 9MHz External Clock34 */
	{0x09, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 36MHz, 9.6MHz External Clock35 */
	{0x0A, 0x01, 0x13, 0x25, 0x06, 0x80, 0xB4}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x37}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x28}, {0x39, 0x2C}
};


static const enum E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_TDMB[] = {
	RTV_ADC_CLK_FREQ_8_MHz/* 175280: 7A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 177008: 7B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 178736: 7C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 181280: 8A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 183008: 8B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 184736: 8C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 187280: 9A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 189008: 9B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 190736: 9C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 193280: 10A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 195008: 10B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 196736: 10C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 199280: 11A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 201008: 11B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 202736: 11C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 205280: 12A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 207008: 12B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 208736: 12C */,
	RTV_ADC_CLK_FREQ_8_MHz/* 211280: 13A */,
	RTV_ADC_CLK_FREQ_8_MHz/* 213008: 13B */,
	RTV_ADC_CLK_FREQ_8_MHz/* 214736: 13C */
};

#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x28}, {0x39, 0x2C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_B3[] = {
	RTV_ADC_CLK_FREQ_8_MHz/*5A : 174928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*5B : 176640*/,
	RTV_ADC_CLK_FREQ_8_MHz/*5C : 178352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*5D : 180064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*6A : 181936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*6B : 183648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*6C : 185360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*6D : 187072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7A : 188928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7B : 190640*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7C : 192352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*7D : 194064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8A : 195936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8B : 197648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8C : 199360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*8D : 201072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9A : 202928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9B : 204640*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9C : 206352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*9D : 208064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10A: 209936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10N: 210096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10B: 211648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10C: 213360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*10D: 215072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11A: 216928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11N: 217008*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11B: 218640*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11C: 220352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*11D: 222064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12A: 223936*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12N: 224096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12B: 225648*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12C: 227360*/,
	RTV_ADC_CLK_FREQ_8_MHz/*12D: 229072*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13A: 230784*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13B: 232496*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13C: 234208*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13D: 235776*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13E: 237488*/,
	RTV_ADC_CLK_FREQ_8_MHz/*13F: 239200*/
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_DAB_L[] = {

	RTV_ADC_CLK_FREQ_8_MHz/*LA: 1452960*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LB: 1454672*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LC: 1456384*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LD: 1458096*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LE: 1459808*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LF: 1461520*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LG: 1463232*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LH: 1464944*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LI: 1466656*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LJ: 1468368*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LK: 1470080*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LL: 1471792*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LM: 1473504*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LN: 1475216*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LO: 1476928*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LP: 1478640*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LQ: 1480352*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LR: 1482064*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LS: 1483776*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LT: 1485488*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LU: 1487200*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LV: 1488912*/,
	RTV_ADC_CLK_FREQ_8_MHz/*LW: 1490624*/
};

#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x28}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 38400)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 38.4MHz, 8MHz External Clock36 */
	{0x08, 0x01, 0x0B, 0x23, 0x06, 0x50, 0xB0},
	/* Based 38.4MHz, 8.192MHz External Clock37 */
	{0x19, 0x01, 0x1F, 0x27, 0x06, 0x00, 0xB9},
	/* Based 38.4MHz, 9MHz External Clock38 */
	{0x08, 0x01, 0x0B, 0x23, 0x06, 0x5A, 0xB0},
	/* Based 38.4MHz, 9.6MHz External Clock39 */
	{0x0A, 0x01, 0x0F, 0x27, 0x07, 0x78, 0xB8}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x34}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x26}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x26}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x26}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 40000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 40MHz, 8MHz External Clock40 */
	{0x0A, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 40MHz, 8.192MHz */ /* Unsupport */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 40MHz, 9MHz External Clock41 */
	{0x0A, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 40MHz, 9.6MHz External Clock42 */
	{0x19, 0x01, 0x1F, 0x27, 0x06, 0x20, 0xB9}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x32}, {0x39, 0x3C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x24}, {0x39, 0x2C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x24}, {0x39, 0x2C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x24}, {0x39, 0x2C}
};
#endif

#elif (RTV_SRC_CLK_FREQ_KHz == 48000)
static const U8 g_abAdcClkSynTbl[MAX_NUM_RTV_ADC_CLK_FREQ_TYPE][7] = {
	/* Based 48MHz, 8MHz External Clock43 */
	{0x0C, 0x01, 0x0F, 0x27, 0x07, 0x60, 0xB8},
	/* Based 48MHz, 8.192MHz */ /* Unsupport */
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* Based 48MHz, 9MHz External Clock44 */
	{0x0C, 0x01, 0x0F, 0x27, 0x07, 0x6C, 0xB8},
	/* Based 48MHz, 9.6MHz External Clock45 */
	{0x0A, 0x01, 0x0B, 0x23, 0x06, 0x60, 0xB0}
};

#ifdef RTV_ISDBT_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_ISDBT[] = {
	{0x37, 0x29}, {0x39, 0x2C}
};

static const E_RTV_ADC_CLK_FREQ_TYPE g_aeAdcClkTypeTbl_ISDBT[] = {
	RTV_ADC_CLK_FREQ_8_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	RTV_ADC_CLK_FREQ_9_MHz
};
#endif

#ifdef RTV_TDMB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_TDMB[] = {
	{0x37, 0x1E}, {0x39, 0x1C}
};
#endif

#ifdef RTV_DAB_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_DAB[] = {
	{0x37, 0x1E}, {0x39, 0x1C}
};
#endif

#ifdef RTV_FM_ENABLE
static const struct RTV_REG_INIT_INFO g_atAutoLnaInitData_FM[] = {
	{0x37, 0x1E}, {0x39, 0x2C}
};
#endif

#else
#error "Unsupport external clock freqency!"
#endif

