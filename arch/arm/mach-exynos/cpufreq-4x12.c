/* linux/arch/arm/mach-exynos/cpufreq-4x12.c
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS4X12 - CPU frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/cpufreq.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/pmu.h>
#include <mach/cpufreq.h>
#include <mach/asv.h>
#include <mach/sec_debug.h>

#include <plat/clock.h>
#include <plat/cpu.h>

#define CPUFREQ_LEVEL_END	(L14 + 1)

#undef PRINT_DIV_VAL

#undef ENABLE_CLKOUT

static int max_support_idx;
static int min_support_idx = (CPUFREQ_LEVEL_END - 1);
static struct clk *cpu_clk;
static struct clk *moutcore;
static struct clk *mout_mpll;
static struct clk *mout_apll;
static bool need_dynamic_ema = false;

struct cpufreq_clkdiv {
	unsigned int	index;
	unsigned int	clkdiv;
	unsigned int	clkdiv1;
};

static unsigned int exynos4x12_volt_table[CPUFREQ_LEVEL_END];

static struct cpufreq_frequency_table exynos4x12_freq_table[] = {
	{L0, 1600*1000},
	{L1, 1500*1000},
	{L2, 1400*1000},
	{L3, 1300*1000},
	{L4, 1200*1000},
	{L5, 1100*1000},
	{L6, 1000*1000},
	{L7, 900*1000},
	{L8, 800*1000},
	{L9, 700*1000},
	{L10, 600*1000},
	{L11, 500*1000},
	{L12, 400*1000},
	{L13, 300*1000},
	{L14, 200*1000},
	{0, CPUFREQ_TABLE_END},
};

static struct cpufreq_clkdiv exynos4x12_clkdiv_table[CPUFREQ_LEVEL_END];

static unsigned int clkdiv_cpu0_4212[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL, DIVCORE2 }
	 */
	/* ARM L0: 1600Mhz */
	{ 0, 6, 7, 0, 6, 1, 2, 0 },

	/* ARM L1: 1500Mhz */
	{ 0, 6, 7, 0, 6, 1, 2, 0 },

	/* ARM L2: 1400Mhz */
	{ 0, 6, 7, 0, 6, 1, 2, 0 },

	/* ARM L3: 1300Mhz */
	{ 0, 6, 7, 0, 5, 1, 2, 0 },

	/* ARM L4: 1200Mhz */
	{ 0, 5, 7, 0, 5, 1, 2, 0 },

	/* ARM L5: 1100MHz */
	{ 0, 5, 7, 0, 4, 1, 2, 0 },

	/* ARM L6: 1000MHz */
	{ 0, 4, 7, 0, 4, 1, 1, 0 },

	/* ARM L7: 900MHz */
	{ 0, 4, 7, 0, 3, 1, 1, 0 },

	/* ARM L8: 800MHz */
	{ 0, 3, 7, 0, 3, 1, 1, 0 },

	/* ARM L9: 700MHz */
	{ 0, 3, 7, 0, 3, 1, 1, 0 },

	/* ARM L10: 600MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L11: 500MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L12: 400MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L13: 300MHz */
	{ 0, 2, 5, 0, 2, 1, 1, 0 },

	/* ARM L14: 200MHz */
	{ 0, 1, 3, 0, 1, 1, 1, 0 },
};

static unsigned int clkdiv_cpu0_4412[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL, DIVCORE2 }
	 */
	/* ARM L0: 1600Mhz */
	{ 0, 3, 7, 0, 6, 1, 7, 0 },

	/* ARM L1: 1500Mhz */
	{ 0, 3, 7, 0, 6, 1, 7, 0 },

	/* ARM L2: 1400Mhz */
	{ 0, 3, 7, 0, 6, 1, 6, 0 },

	/* ARM L3: 1300Mhz */
	{ 0, 3, 7, 0, 5, 1, 6, 0 },

	/* ARM L4: 1200Mhz */
	{ 0, 3, 7, 0, 5, 1, 5, 0 },

	/* ARM L5: 1100MHz */
	{ 0, 3, 6, 0, 4, 1, 5, 0 },

	/* ARM L6: 1000MHz */
	{ 0, 2, 5, 0, 4, 1, 4, 0 },

	/* ARM L7: 900MHz */
	{ 0, 2, 5, 0, 3, 1, 4, 0 },

	/* ARM L8: 800MHz */
	{ 0, 2, 5, 0, 3, 1, 3, 0 },

	/* ARM L9: 700MHz */
	{ 0, 2, 4, 0, 3, 1, 3, 0 },

	/* ARM L10: 600MHz */
	{ 0, 2, 4, 0, 3, 1, 2, 0 },

	/* ARM L11: 500MHz */
	{ 0, 2, 4, 0, 3, 1, 2, 0 },

	/* ARM L12: 400MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L13: 300MHz */
	{ 0, 2, 4, 0, 2, 1, 1, 0 },

	/* ARM L14: 200MHz */
	{ 0, 1, 3, 0, 1, 1, 1, 0 },
};

static unsigned int clkdiv_cpu1_4212[CPUFREQ_LEVEL_END][2] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */
	/* ARM L0: 1600MHz */
	{ 6, 0 },

	/* ARM L1: 1500MHz */
	{ 6, 0 },

	/* ARM L2: 1400MHz */
	{ 6, 0 },

	/* ARM L3: 1300MHz */
	{ 5, 0 },

	/* ARM L4: 1200MHz */
	{ 5, 0 },

	/* ARM L5: 1100MHz */
	{ 4, 0 },

	/* ARM L6: 1000MHz */
	{ 4, 0 },

	/* ARM L7: 900MHz */
	{ 3, 0 },

	/* ARM L8: 800MHz */
	{ 3, 0 },

	/* ARM L9: 700MHz */
	{ 3, 0 },

	/* ARM L10: 600MHz */
	{ 3, 0 },

	/* ARM L11: 500MHz */
	{ 3, 0 },

	/* ARM L12: 400MHz */
	{ 3, 0 },

	/* ARM L13: 300MHz */
	{ 3, 0 },

	/* ARM L14: 200MHz */
	{ 3, 0 },
};

static unsigned int clkdiv_cpu1_4412[CPUFREQ_LEVEL_END][3] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM, DIVCORES }
	 */
	/* ARM L0: 1600MHz */
	{ 6, 0, 7 },

	/* ARM L1: 1500MHz */
	{ 6, 0, 7 },

	/* ARM L2: 1400MHz */
	{ 6, 0, 6 },

	/* ARM L3: 1300MHz */
	{ 5, 0, 6 },

	/* ARM L4: 1200MHz */
	{ 5, 0, 5 },

	/* ARM L5: 1100MHz */
	{ 4, 0, 5 },

	/* ARM L6: 1000MHz */
	{ 4, 0, 4 },

	/* ARM L7: 900MHz */
	{ 3, 0, 4 },

	/* ARM L8: 800MHz */
	{ 3, 0, 3 },

	/* ARM L9: 700MHz */
	{ 3, 0, 3 },

	/* ARM L10: 600MHz */
	{ 3, 0, 2 },

	/* ARM L11: 500MHz */
	{ 3, 0, 2 },

	/* ARM L12: 400MHz */
	{ 3, 0, 1 },

	/* ARM L13: 300MHz */
	{ 3, 0, 1 },

	/* ARM L14: 200MHz */
	{ 3, 0, 0 },
};

static unsigned int exynos4x12_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1600MHz */
	((200<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L1: 1500MHz */
	((250<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L2: 1400MHz */
	((175<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L3: 1300MHz */
	((325<<16)|(6<<8)|(0x0)),

	/* APLL FOUT L4: 1200MHz */
	((200<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L5: 1100MHz */
	((275<<16)|(6<<8)|(0x0)),

	/* APLL FOUT L6: 1000MHz */
	((125<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L7: 900MHz */
	((150<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L8: 800MHz */
	((100<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L9: 700MHz */
	((175<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L10: 600MHz */
	((200<<16)|(4<<8)|(0x1)),

	/* APLL FOUT L11: 500MHz */
	((125<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L12 400MHz */
	((100<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L13: 300MHz */
	((200<<16)|(4<<8)|(0x2)),

	/* APLL FOUT L14: 200MHz */
	((100<<16)|(3<<8)|(0x2)),

};

/*
 * ASV group voltage table
 */

#define NO_ABB_LIMIT	L8

static const unsigned int asv_voltage_4212[CPUFREQ_LEVEL_END][13] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11    ASV12 */
	{ 1300000, 1287500, 1287500, 1275000, 1287500, 1275000,	1262500, 1237500, 1225000, 1212500, 1200000, 1187500, 1175000 }, /* L0 */
	{ 1300000, 1287500, 1287500, 1275000, 1287500, 1275000,	1262500, 1237500, 1225000, 1212500, 1200000, 1187500, 1175000 }, /* L1 */
	{ 1262500, 1250000, 1237500, 1225000, 1237500, 1225000,	1212500, 1187500, 1175000, 1162500, 1150000, 1137500, 1125000 }, /* L2 */
	{ 1212500, 1200000, 1187500, 1175000, 1187500, 1175000,	1162500, 1137500, 1125000, 1112500, 1100000, 1087500, 1075000 }, /* L3 */
	{ 1162500, 1150000, 1137500, 1125000, 1137500, 1125000,	1112500, 1087500, 1075000, 1062500, 1050000, 1037500, 1025000 }, /* L4 */
	{ 1125000, 1112500, 1100000, 1087500, 1100000, 1087500,	1075000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500 }, /* L5 */
	{ 1087500, 1075000, 1062500, 1050000, 1062500, 1050000,	1037500, 1012500, 1000000,  987500,  975000,  962500,  950000 }, /* L6 */
	{ 1050000, 1037500, 1025000, 1012500, 1025000, 1012500,	1000000,  975000,  962500,  950000,  937500,  925000,  912500 }, /* L7 */
	{ 1012500, 1000000,  987500,  975000,  987500,  975000,	 962500,  937500,  925000,  912500,  912500,  912500,  900000 }, /* L8 */
	{  975000,  962500,  950000,  937500,  950000,  937500,	 925000,  912500,  912500,  900000,  900000,  900000,  887500 }, /* L9 */
	{  950000,  937500,  925000,  912500,  925000,  912500,	 900000,  900000,  900000,  887500,  887500,  887500,  875000 }, /* L10 */
	{  950000,  937500,  925000,  912500,  925000,  912500,	 900000,  887500,  887500,  875000,  875000,  875000,  862500 }, /* L11 */
	{  937500,  925000,  912500,  900000,  900000,  900000,	 887500,  887500,  887500,  875000,  875000,  875000,  862500 }, /* L12 */
	{  937500,  925000,  912500,  900000,  900000,  900000,	 887500,  887500,  887500,  875000,  875000,  862500,  850000 }, /* L13 */
	{  937500,  925000,  912500,  900000,  900000,  900000,	 887500,  887500,  887500,  875000,  875000,  862500,  850000 }, /* L14 */
};

static const unsigned int asv_voltage_s[CPUFREQ_LEVEL_END] = {
	1300000, 1300000, 1300000, 1250000, 1200000, 1175000, 1100000,
	1050000, 1025000, 1000000, 1000000, 1000000, 950000, 950000
};

#if defined(CONFIG_MACH_GC1)
/* ASV table for 12.5mV step */
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][12] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L0 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L1 - Not used */
	{ 1325000, 1312500, 1300000, 1287500, 1300000, 1287500,	1275000, 1250000, 1250000, 1237500, 1225000, 1212500 },
	{ 1300000, 1275000, 1237500, 1237500, 1250000, 1250000,	1237500, 1212500, 1200000, 1200000, 1187500, 1175000 },
	{ 1225000, 1212500, 1200000, 1187500, 1200000, 1187500,	1175000, 1150000, 1137500, 1125000, 1125000, 1112500 },
	{ 1175000, 1162500, 1150000, 1137500, 1150000, 1137500,	1125000, 1100000, 1100000, 1075000, 1075000, 1062500 },
	{ 1125000, 1112500, 1100000, 1087500, 1100000, 1087500,	1075000, 1050000, 1037500, 1025000, 1025000, 1012500 },
	{ 1100000, 1087500, 1075000, 1075000, 1075000, 1062500,	1050000, 1037500, 1025000, 1012500, 1012500, 1000000 }, /* 900 */
	{ 1062500, 1050000, 1025000, 1025000, 1025000, 1012500,	1000000,  987500,  987500,  987500,  987500,  975000 }, /* 800 */
	{ 1012500, 1000000,  975000,  975000,  975000,  975000,	 962500,  962500,  950000,  950000,  950000,  937500 },
	{ 1000000,  987500,  962500,  962500,  962500,  962500,	 950000,  950000,  937500,  937500,  937500,  925000 },
	{  987500,  975000,  950000,  937500,  950000,  937500,	 937500,  937500,  912500,  912500,  912500,  900000 },
	{  975000,  962500,  950000,  925000,  950000,  925000,	 925000,  925000,  900000,  900000,  900000,  887500 },
	{  950000,  937500,  925000,  900000,  925000,  900000,	 900000,  900000,  900000,  887500,  875000,  862500 },
	{  925000,  912500,  900000,  900000,  900000,  900000,	 900000,  900000,  887500,  875000,  875000,  862500 },
};
#else
/* ASV table for 12.5mV step */
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][12] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L0 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L1 - Not used */
	{ 1325000, 1312500, 1300000, 1287500, 1300000, 1287500,	1275000, 1250000, 1250000, 1237500, 1225000, 1212500 },
	{ 1300000, 1275000, 1237500, 1237500, 1250000, 1250000,	1237500, 1212500, 1200000, 1200000, 1187500, 1175000 },
	{ 1225000, 1212500, 1200000, 1187500, 1200000, 1187500,	1175000, 1150000, 1137500, 1125000, 1125000, 1112500 },
	{ 1175000, 1162500, 1150000, 1137500, 1150000, 1137500,	1125000, 1100000, 1100000, 1075000, 1075000, 1062500 },
	{ 1125000, 1112500, 1100000, 1087500, 1100000, 1087500,	1075000, 1050000, 1037500, 1025000, 1025000, 1012500 },
	{ 1075000, 1062500, 1050000, 1050000, 1050000, 1037500,	1025000, 1012500, 1000000,  987500,  987500,  975000 },
	{ 1037500, 1025000, 1000000, 1000000, 1000000,  987500,	 975000,  962500,  962500,  962500,  962500,  950000 },
	{ 1012500, 1000000,  975000,  975000,  975000,  975000,	 962500,  962500,  950000,  950000,  950000,  937500 },
	{ 1000000,  987500,  962500,  962500,  962500,  962500,	 950000,  950000,  937500,  937500,  937500,  925000 },
	{  987500,  975000,  950000,  937500,  950000,  937500,	 937500,  937500,  912500,  912500,  912500,  900000 },
	{  975000,  962500,  950000,  925000,  950000,  925000,	 925000,  925000,  900000,  900000,  900000,  887500 },
	{  950000,  937500,  925000,  900000,  925000,  900000,	 900000,  900000,  900000,  887500,  875000,  862500 },
	{  925000,  912500,  900000,  900000,  900000,  900000,	 900000,  900000,  887500,  875000,  875000,  862500 },
};
#endif
/* 20120927 DVFS table for pega prime */
static const unsigned int asv_voltage_step_12_5_rev2[CPUFREQ_LEVEL_END][13] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11    ASV12 */
	{ 1312500, 1312500, 1312500, 1312500, 1300000, 1287500, 1275000, 1262500, 1250000, 1237500, 1212500, 1200000, 1187500 },	/* L0 */
	{ 1312500, 1262500, 1262500, 1262500, 1250000, 1237500,	1225000, 1212500, 1200000, 1187500, 1162500, 1150000, 1137500 },	/* L1 */
	{ 1275000, 1225000, 1225000, 1225000, 1212500, 1200000, 1187500, 1175000, 1162500, 1150000, 1125000, 1112500, 1100000 },	/* L2 */
	{ 1225000, 1175000, 1175000, 1175000, 1162500, 1150000, 1137500, 1125000, 1112500, 1100000, 1075000, 1062500, 1050000 },	/* L3 */
	{ 1187500, 1137500, 1137500, 1137500, 1125000, 1112500, 1100000, 1087500, 1075000, 1062500, 1037500, 1025000, 1012500 },	/* L4 */
	{ 1150000, 1100000, 1100000, 1100000, 1087500, 1075000, 1062500, 1050000, 1037500, 1025000, 1000000,  987500,  975000 },	/* L5 */
	{ 1125000, 1075000, 1075000, 1075000, 1062500, 1050000, 1037500, 1025000, 1012500, 1000000,  975000,  962500,  950000 },	/* L6 */
	{ 1100000, 1050000, 1050000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  975000,  950000,  937500,  925000 },	/* L7 */
	{ 1062500, 1012500, 1012500, 1012500, 1000000,  987500,  975000,  962500,  950000,  937500,  912500,  900000,  887500 },	/* L8 */
	{ 1037500,  987500,  987500,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  887500,  887500,  887500 },	/* L9 */
	{ 1012500,  962500,  962500,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000,  875000,  875000 },	/* L10 */
	{ 1000000,  950000,  950000,  950000,  937500,  925000,  912500,  900000,  887500,  887500,  875000,  875000,  875000 },	/* L11 */
	{  987500,  937500,  937500,  937500,  925000,  912500,  900000,  887500,  887500,  887500,  875000,  875000,  875000 },	/* L12 */
	{  975000,  925000,  925000,  925000,  912500,  900000,  887500,  887500,  887500,  887500,  875000,  875000,  875000 },	/* L13 */
	{  962500,  912500,  912500,  912500,  900000,  887500,  887500,  887500,  887500,  887500,  875000,  875000,  875000 },	/* L14 */
};

static const unsigned int asv_voltage_step_1ghz[CPUFREQ_LEVEL_END][12] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L0 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L1 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L2 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L3 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L4 - Not used */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L5 - Not used */
	{ 1200000, 1200000, 1200000, 1200000, 1125000, 1125000,	1125000, 1075000, 1075000, 1075000, 1075000, 1037500 },
	{ 1150000, 1150000, 1150000, 1150000, 1075000, 1075000,	1075000, 1037500, 1037500, 1037500, 1037500, 1000000 },
	{ 1100000, 1100000, 1100000, 1100000, 1025000, 1025000,	1025000,  987500,  987500,  987500,  987500,  975000 },
	{ 1050000, 1050000, 1050000, 1050000, 1000000, 1000000,	1000000,  987500,  987500,  987500,  987500,  962500 },
	{ 1025000, 1025000, 1025000, 1025000,  987500,  987500,	 987500,  975000,  975000,  975000,  975000,  950000 },
	{ 1000000, 1000000, 1000000, 1000000,  975000,  975000,	 975000,  962500,  962500,  962500,  962500,  925000 },
	{ 1000000, 1000000, 1000000, 1000000,  975000,  975000,	 975000,  950000,  950000,  950000,  950000,  912500 },
	{  975000,  975000,  975000,  975000,  950000,  950000,	 950000,  925000,  925000,  925000,  925000,  887500 },
	{  975000,  975000,  975000,  975000,  937500,  937500,	 937500,  925000,  925000,  925000,  925000,  887500 },
};

static void set_clkdiv(unsigned int div_index)
{
	unsigned int tmp;
	unsigned int stat_cpu1;

	/* Change Divider - CPU0 */

	tmp = exynos4x12_clkdiv_table[div_index].clkdiv;

	__raw_writel(tmp, EXYNOS4_CLKDIV_CPU);

	do {
		tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU);
	} while (tmp & 0x11111111);

#ifdef PRINT_DIV_VAL
	tmp = __raw_readl(EXYNOS4_CLKDIV_CPU);
	pr_info("DIV_CPU0[0x%x]\n", tmp);

#endif

	/* Change Divider - CPU1 */
	tmp = exynos4x12_clkdiv_table[div_index].clkdiv1;

	__raw_writel(tmp, EXYNOS4_CLKDIV_CPU1);
	if (soc_is_exynos4212())
		stat_cpu1 = 0x11;
	else
		stat_cpu1 = 0x111;

	do {
		tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU1);
	} while (tmp & stat_cpu1);
#ifdef PRINT_DIV_VAL
	tmp = __raw_readl(EXYNOS4_CLKDIV_CPU1);
	pr_info("DIV_CPU1[0x%x]\n", tmp);
#endif
}

static void set_apll(unsigned int new_index,
			     unsigned int old_index)
{
	unsigned int tmp, pdiv;

	/* 1. MUX_CORE_SEL = MPLL,
	 * ARMCLK uses MPLL for lock time */
	if (clk_set_parent(moutcore, mout_mpll))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				mout_mpll->name, moutcore->name);

	do {
		tmp = (__raw_readl(EXYNOS4_CLKMUX_STATCPU)
			>> EXYNOS4_CLKSRC_CPU_MUXCORE_SHIFT);
		tmp &= 0x7;
	} while (tmp != 0x2);

	/* 2. Set APLL Lock time */
	pdiv = ((exynos4x12_apll_pms_table[new_index] >> 8) & 0x3f);

	__raw_writel((pdiv * 250), EXYNOS4_APLL_LOCK);

	/* 3. Change PLL PMS values */
	tmp = __raw_readl(EXYNOS4_APLL_CON0);
	tmp &= ~((0x3ff << 16) | (0x3f << 8) | (0x7 << 0));
	tmp |= exynos4x12_apll_pms_table[new_index];
	__raw_writel(tmp, EXYNOS4_APLL_CON0);

	/* 4. wait_lock_time */
	do {
		tmp = __raw_readl(EXYNOS4_APLL_CON0);
	} while (!(tmp & (0x1 << EXYNOS4_APLLCON0_LOCKED_SHIFT)));

	/* 5. MUX_CORE_SEL = APLL */
	if (clk_set_parent(moutcore, mout_apll))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				mout_apll->name, moutcore->name);

	do {
		tmp = __raw_readl(EXYNOS4_CLKMUX_STATCPU);
		tmp &= EXYNOS4_CLKMUX_STATCPU_MUXCORE_MASK;
	} while (tmp != (0x1 << EXYNOS4_CLKSRC_CPU_MUXCORE_SHIFT));

}

bool exynos4x12_pms_change(unsigned int old_index, unsigned int new_index)
{
	unsigned int old_pm = (exynos4x12_apll_pms_table[old_index] >> 8);
	unsigned int new_pm = (exynos4x12_apll_pms_table[new_index] >> 8);

	return (old_pm == new_pm) ? 0 : 1;
}

static void exynos4x12_set_frequency(unsigned int old_index,
				  unsigned int new_index)
{
	unsigned int tmp;

	sec_debug_aux_log(SEC_DEBUG_AUXLOG_CPU_BUS_CLOCK_CHANGE,
			"%s: old_index=%d, new_index=%d(%ps)",
			__func__, old_index, new_index,
			__builtin_return_address(0));

	if (old_index > new_index) {
		if (exynos4x12_volt_table[new_index] >= 950000 &&
				need_dynamic_ema)
				__raw_writel(0x101, EXYNOS4_EMA_CONF);

		if ((soc_is_exynos4212())
			&& (exynos_result_of_asv > 3)
			&& (old_index > L7) && (new_index <= L7)) {
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
		} else if ((samsung_rev() >= EXYNOS4412_REV_2_0)
			&& (exynos_result_of_asv > 2)
			&& (old_index > L8) && (new_index <= L8)) {
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
		}

		if (!exynos4x12_pms_change(old_index, new_index)) {
			/* 1. Change the system clock divider values */
			set_clkdiv(new_index);
			/* 2. Change just s value in apll m,p,s value */
			tmp = __raw_readl(EXYNOS4_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4x12_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, EXYNOS4_APLL_CON0);

		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the system clock divider values */
			set_clkdiv(new_index);
			/* 2. Change the apll m,p,s value */
			set_apll(new_index, old_index);
		}
	} else if (old_index < new_index) {
		if (!exynos4x12_pms_change(old_index, new_index)) {
			/* 1. Change just s value in apll m,p,s value */
			tmp = __raw_readl(EXYNOS4_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4x12_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, EXYNOS4_APLL_CON0);
			/* 2. Change the system clock divider values */
			set_clkdiv(new_index);
		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the apll m,p,s value */
			set_apll(new_index, old_index);
			/* 2. Change the system clock divider values */
			set_clkdiv(new_index);
		}
		if ((soc_is_exynos4212())
			&& (exynos_result_of_asv > 3)
			&& (old_index <= L7) && (new_index > L7)) {
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
		} else if ((samsung_rev() >= EXYNOS4412_REV_2_0)
			&& (exynos_result_of_asv > 2)
			&& (old_index <= L8) && (new_index > L8)) {
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
		}
		if (exynos4x12_volt_table[new_index] < 950000 &&
				need_dynamic_ema)
			__raw_writel(0x404, EXYNOS4_EMA_CONF);
	}

	/* ABB value is changed in below case */
	if (soc_is_exynos4412() && (exynos_result_of_asv > 3)
		&& (samsung_rev() < EXYNOS4412_REV_2_0)) {
		if (new_index == L14)
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
		else
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
	}
}

/* Get maximum cpufreq index of chip */
static unsigned int get_max_cpufreq_idx(void)
{
	int index = -EINVAL;

#if defined(CONFIG_EXYNOS4X12_800MHZ_SUPPORT)
	index = L8;
#elif defined(CONFIG_EXYNOS4X12_400MHZ_SUPPORT)
	index = L12;
#else
	if (soc_is_exynos4212()) {
		index = L1;
	} else if (soc_is_exynos4412()) {
		/* exynos4x12 prime supports 1.6GHz */
		if (samsung_rev() >= EXYNOS4412_REV_2_0)
			index = L0;
		else {
		/* exynos4x12 supports only 1.4GHz and 1.1GHz */
			if (exynos_armclk_max != 1400000)
				index = L6;
			else
				index = L2;
		}
	}
#endif

	return index;
}

static void __init set_volt_table(void)
{
	unsigned int i, tmp;

	max_support_idx = get_max_cpufreq_idx();

	for (i = 0; i < max_support_idx; i++)
		exynos4x12_freq_table[i].frequency = CPUFREQ_ENTRY_INVALID;

	pr_info("DVFS : VDD_ARM Voltage table set with %d Group\n", exynos_result_of_asv);

	if (exynos_result_of_asv == 0xff) {
		for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
			exynos4x12_volt_table[i] = asv_voltage_s[i];
	} else {
		if (soc_is_exynos4212()) {
			for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
				exynos4x12_volt_table[i] =
					asv_voltage_4212[i][exynos_result_of_asv];
		} else if (soc_is_exynos4412()) {
			if (samsung_rev() >= EXYNOS4412_REV_2_0) {
				for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
					exynos4x12_volt_table[i] =
						asv_voltage_step_12_5_rev2[i][exynos_result_of_asv];
			} else {
				if (exynos_armclk_max == 1000000) {
					for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
						exynos4x12_volt_table[i] =
							asv_voltage_step_1ghz[i][exynos_result_of_asv];
				} else {
					for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
						exynos4x12_volt_table[i] =
							asv_voltage_step_12_5[i][exynos_result_of_asv];
				}
			}
		} else {
			pr_err("%s: Can't find SoC type \n", __func__);
		}
	}

	if (soc_is_exynos4212() ||
		(soc_is_exynos4412() && (samsung_rev() >= EXYNOS4412_REV_2_0))) {
		tmp = (is_special_flag() >> ARM_LOCK_FLAG) & 0x3;

		if (tmp) {
			pr_info("%s : special flag[%d]\n", __func__, tmp);
			switch (tmp) {
			case 1:
				if (soc_is_exynos4212())
					i = L9; 	/* 700MHz fixed volt */
				else
					i = L11;	/* 500MHz fixed volt */
				break;
			case 2:
				if (soc_is_exynos4212())
					i = L8; 	/* 800MHz fixed volt */
				else
					i = L9; 	/* 700MHz fixed volt */
				break;
			case 3:
				if (soc_is_exynos4212())
					i = L7; 	/* 900MHz fixed volt */
				else
					i = L8; 	/* 800MHz fixed volt */
				break;
			default:
				break;
			}

			pr_info("ARM voltage locking at L%d\n", i);

			for (tmp = (i + 1) ; tmp < CPUFREQ_LEVEL_END ; tmp++) {
				exynos4x12_volt_table[tmp] =
					exynos4x12_volt_table[i];
				pr_info("CPUFREQ: L%d : %d\n", tmp, exynos4x12_volt_table[tmp]);
			}
		}

		if (exynos_dynamic_ema) {
			need_dynamic_ema = true;
			pr_info("%s: Dynamic EMA is enabled\n", __func__);
		}
	}
}

/*
 * The values of the table is not correct.
 * Copied from C210 as prototype assuming that unmapping 512KiB
 * requires 128 DMA operations.
 */
#ifdef CONFIG_SLP
static struct dvfs_qos_info exynos4x12_dma_lat_qos[] = {
	{ 118,	200000, L14 },
	{ 40,	500000, L11 },
	{ 24,	800000, L8 },
	{ 16,	1000000, L6 },
	{},
};
#endif

int exynos4x12_cpufreq_init(struct exynos_dvfs_info *info)
{
	int i;
	unsigned int tmp;
	unsigned long rate;

	set_volt_table();

	cpu_clk = clk_get(NULL, "armclk");
	if (IS_ERR(cpu_clk))
		return PTR_ERR(cpu_clk);

	moutcore = clk_get(NULL, "moutcore");
	if (IS_ERR(moutcore))
		goto err_moutcore;

	mout_mpll = clk_get(NULL, "mout_mpll");
	if (IS_ERR(mout_mpll))
		goto err_mout_mpll;

	rate = clk_get_rate(mout_mpll) / 1000;

	mout_apll = clk_get(NULL, "mout_apll");
	if (IS_ERR(mout_apll))
		goto err_mout_apll;

	for (i = L0; i <  CPUFREQ_LEVEL_END; i++) {

		exynos4x12_clkdiv_table[i].index = i;

		tmp = __raw_readl(EXYNOS4_CLKDIV_CPU);

		tmp &= ~(EXYNOS4_CLKDIV_CPU0_CORE_MASK |
			EXYNOS4_CLKDIV_CPU0_COREM0_MASK |
			EXYNOS4_CLKDIV_CPU0_COREM1_MASK |
			EXYNOS4_CLKDIV_CPU0_PERIPH_MASK |
			EXYNOS4_CLKDIV_CPU0_ATB_MASK |
			EXYNOS4_CLKDIV_CPU0_PCLKDBG_MASK |
			EXYNOS4_CLKDIV_CPU0_APLL_MASK |
			EXYNOS4_CLKDIV_CPU0_CORE2_MASK);

		if (soc_is_exynos4212()) {
			tmp |= ((clkdiv_cpu0_4212[i][0] << EXYNOS4_CLKDIV_CPU0_CORE_SHIFT) |
				(clkdiv_cpu0_4212[i][1] << EXYNOS4_CLKDIV_CPU0_COREM0_SHIFT) |
				(clkdiv_cpu0_4212[i][2] << EXYNOS4_CLKDIV_CPU0_COREM1_SHIFT) |
				(clkdiv_cpu0_4212[i][3] << EXYNOS4_CLKDIV_CPU0_PERIPH_SHIFT) |
				(clkdiv_cpu0_4212[i][4] << EXYNOS4_CLKDIV_CPU0_ATB_SHIFT) |
				(clkdiv_cpu0_4212[i][5] << EXYNOS4_CLKDIV_CPU0_PCLKDBG_SHIFT) |
				(clkdiv_cpu0_4212[i][6] << EXYNOS4_CLKDIV_CPU0_APLL_SHIFT) |
				(clkdiv_cpu0_4212[i][7] << EXYNOS4_CLKDIV_CPU0_CORE2_SHIFT));
		} else {
			tmp |= ((clkdiv_cpu0_4412[i][0] << EXYNOS4_CLKDIV_CPU0_CORE_SHIFT) |
				(clkdiv_cpu0_4412[i][1] << EXYNOS4_CLKDIV_CPU0_COREM0_SHIFT) |
				(clkdiv_cpu0_4412[i][2] << EXYNOS4_CLKDIV_CPU0_COREM1_SHIFT) |
				(clkdiv_cpu0_4412[i][3] << EXYNOS4_CLKDIV_CPU0_PERIPH_SHIFT) |
				(clkdiv_cpu0_4412[i][4] << EXYNOS4_CLKDIV_CPU0_ATB_SHIFT) |
				(clkdiv_cpu0_4412[i][5] << EXYNOS4_CLKDIV_CPU0_PCLKDBG_SHIFT) |
				(clkdiv_cpu0_4412[i][6] << EXYNOS4_CLKDIV_CPU0_APLL_SHIFT) |
				(clkdiv_cpu0_4412[i][7] << EXYNOS4_CLKDIV_CPU0_CORE2_SHIFT));
		}

		exynos4x12_clkdiv_table[i].clkdiv = tmp;

		tmp = __raw_readl(EXYNOS4_CLKDIV_CPU1);

		if (soc_is_exynos4212()) {
			tmp &= ~(EXYNOS4_CLKDIV_CPU1_COPY_MASK |
				EXYNOS4_CLKDIV_CPU1_HPM_MASK);
			tmp |= ((clkdiv_cpu1_4212[i][0] << EXYNOS4_CLKDIV_CPU1_COPY_SHIFT) |
				(clkdiv_cpu1_4212[i][1] << EXYNOS4_CLKDIV_CPU1_HPM_SHIFT));
		} else {
			tmp &= ~(EXYNOS4_CLKDIV_CPU1_COPY_MASK |
				EXYNOS4_CLKDIV_CPU1_HPM_MASK |
				EXYNOS4_CLKDIV_CPU1_CORES_MASK);
			tmp |= ((clkdiv_cpu1_4412[i][0] << EXYNOS4_CLKDIV_CPU1_COPY_SHIFT) |
				(clkdiv_cpu1_4412[i][1] << EXYNOS4_CLKDIV_CPU1_HPM_SHIFT) |
				(clkdiv_cpu1_4412[i][2] << EXYNOS4_CLKDIV_CPU1_CORES_SHIFT));
		}
		exynos4x12_clkdiv_table[i].clkdiv1 = tmp;
	}

	info->mpll_freq_khz = rate;
#ifdef CONFIG_SLP
	/* S-Boot at 20120406 uses L8 at bootup */
	info->pm_lock_idx = L8;

	/*
	 * However, the bootup frequency might get changed anytime.
	 * Thus, we'd like to get the value at bootup time.
	 */
	rate = clk_get_rate(cpu_clk) / 1000;
	for (i = 0; exynos4x12_freq_table[i].frequency != CPUFREQ_TABLE_END;
	     i++) {
		if (exynos4x12_freq_table[i].frequency == rate) {
			info->pm_lock_idx = exynos4x12_freq_table[i].index;
			break;
		}
	}
	pr_info("Bootup CPU Frequency = [%d] %dMHz\n", info->pm_lock_idx,
		rate / 1000);
#else

#if defined(CONFIG_EXYNOS4X12_800MHZ_SUPPORT)
	info->pm_lock_idx = L8;
#elif defined(CONFIG_EXYNOS4X12_400MHZ_SUPPORT)
	info->pm_lock_idx = L12;
#else
	info->pm_lock_idx = L6;
#endif
#endif
	/*
	 * ARM clock source will be changed APLL to MPLL temporary
	 * in exynos4x12_set_frequency.
	 * To support MPLL, vdd_arm is supplied to voltage at frequency
	 * higher than MPLL.
	 * So, pll_safe_idx set to value based on MPLL clock.(800MHz or 880MHz)
	 */
	if (samsung_rev() >= EXYNOS4412_REV_2_0)
		info->pll_safe_idx = L7;
	else
		info->pll_safe_idx = L8;

	info->max_support_idx = max_support_idx;
	info->min_support_idx = min_support_idx;
	info->cpu_clk = cpu_clk;
	info->volt_table = exynos4x12_volt_table;
	info->freq_table = exynos4x12_freq_table;
	info->set_freq = exynos4x12_set_frequency;
	info->need_apll_change = exynos4x12_pms_change;
#ifdef CONFIG_SLP
	info->cpu_dma_latency = exynos4x12_dma_lat_qos;
#endif

#ifdef ENABLE_CLKOUT
	tmp = __raw_readl(EXYNOS4_CLKOUT_CMU_CPU);
	tmp &= ~0xffff;
	tmp |= 0x1904;
	__raw_writel(tmp, EXYNOS4_CLKOUT_CMU_CPU);

	exynos4_pmu_xclkout_set(1, XCLKOUT_CMU_CPU);
#endif

	return 0;

err_mout_apll:
	if (!IS_ERR(mout_mpll))
		clk_put(mout_mpll);
err_mout_mpll:
	if (!IS_ERR(moutcore))
		clk_put(moutcore);
err_moutcore:
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);

	pr_debug("%s: failed initialization\n", __func__);
	return -EINVAL;
}
EXPORT_SYMBOL(exynos4x12_cpufreq_init);
