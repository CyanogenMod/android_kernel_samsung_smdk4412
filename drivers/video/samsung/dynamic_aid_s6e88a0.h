#ifndef __DYNAMIC_AID_XXXX_H
#define __DYNAMIC_AID_XXXX_H __FILE__

#include "dynamic_aid.h"
#include "dynamic_aid_gamma_curve.h"

enum {
	IV_VT,
	IV_3,
	IV_11,
	IV_23,
	IV_35,
	IV_51,
	IV_87,
	IV_151,
	IV_203,
	IV_255,
	IV_MAX
};

enum {
	IBRIGHTNESS_10NT,
	IBRIGHTNESS_11NT,
	IBRIGHTNESS_12NT,
	IBRIGHTNESS_13NT,
	IBRIGHTNESS_14NT,
	IBRIGHTNESS_15NT,
	IBRIGHTNESS_16NT,
	IBRIGHTNESS_17NT,
	IBRIGHTNESS_19NT,
	IBRIGHTNESS_20NT,
	IBRIGHTNESS_21NT,
	IBRIGHTNESS_22NT,
	IBRIGHTNESS_24NT,
	IBRIGHTNESS_25NT,
	IBRIGHTNESS_27NT,
	IBRIGHTNESS_29NT,
	IBRIGHTNESS_30NT,
	IBRIGHTNESS_32NT,
	IBRIGHTNESS_34NT,
	IBRIGHTNESS_37NT,
	IBRIGHTNESS_39NT,
	IBRIGHTNESS_41NT,
	IBRIGHTNESS_44NT,
	IBRIGHTNESS_47NT,
	IBRIGHTNESS_50NT,
	IBRIGHTNESS_53NT,
	IBRIGHTNESS_56NT,
	IBRIGHTNESS_60NT,
	IBRIGHTNESS_64NT,
	IBRIGHTNESS_68NT,
	IBRIGHTNESS_72NT,
	IBRIGHTNESS_77NT,
	IBRIGHTNESS_82NT,
	IBRIGHTNESS_87NT,
	IBRIGHTNESS_93NT,
	IBRIGHTNESS_98NT,
	IBRIGHTNESS_105NT,
	IBRIGHTNESS_111NT,
	IBRIGHTNESS_119NT,
	IBRIGHTNESS_126NT,
	IBRIGHTNESS_134NT,
	IBRIGHTNESS_143NT,
	IBRIGHTNESS_152NT,
	IBRIGHTNESS_162NT,
	IBRIGHTNESS_172NT,
	IBRIGHTNESS_183NT,
	IBRIGHTNESS_195NT,
	IBRIGHTNESS_207NT,
	IBRIGHTNESS_220NT,
	IBRIGHTNESS_234NT,
	IBRIGHTNESS_249NT,
	IBRIGHTNESS_265NT,
	IBRIGHTNESS_282NT,
	IBRIGHTNESS_300NT,
	IBRIGHTNESS_400NT,
	IBRIGHTNESS_MAX
};

#define VREG_OUT_X1000		6100	/* VREG_OUT x 1000 */

static const int index_voltage_table[IBRIGHTNESS_MAX] = {
	0,		/* IV_VT */
	3,		/* IV_3 */
	11,		/* IV_11 */
	23,		/* IV_23 */
	35,		/* IV_35 */
	51,		/* IV_51 */
	87,		/* IV_87 */
	151,		/* IV_151 */
	203,		/* IV_203 */
	255		/* IV_255 */
};

static const int index_brightness_table[IBRIGHTNESS_MAX] = {
	10,	/* IBRIGHTNESS_10NT */
	11,	/* IBRIGHTNESS_11NT */
	12,	/* IBRIGHTNESS_12NT */
	13,	/* IBRIGHTNESS_13NT */
	14,	/* IBRIGHTNESS_14NT */
	15,	/* IBRIGHTNESS_15NT */
	16,	/* IBRIGHTNESS_16NT */
	17,	/* IBRIGHTNESS_17NT */
	19,	/* IBRIGHTNESS_19NT */
	20,	/* IBRIGHTNESS_20NT */
	21,	/* IBRIGHTNESS_21NT */
	22,	/* IBRIGHTNESS_22NT */
	24,	/* IBRIGHTNESS_24NT */
	25,	/* IBRIGHTNESS_25NT */
	27,	/* IBRIGHTNESS_27NT */
	29,	/* IBRIGHTNESS_29NT */
	30,	/* IBRIGHTNESS_30NT */
	32,	/* IBRIGHTNESS_32NT */
	34,	/* IBRIGHTNESS_34NT */
	37,	/* IBRIGHTNESS_37NT */
	39,	/* IBRIGHTNESS_39NT */
	41,	/* IBRIGHTNESS_41NT */
	44,	/* IBRIGHTNESS_44NT */
	47,	/* IBRIGHTNESS_47NT */
	50,	/* IBRIGHTNESS_50NT */
	53,	/* IBRIGHTNESS_53NT */
	56,	/* IBRIGHTNESS_56NT */
	60,	/* IBRIGHTNESS_60NT */
	64,	/* IBRIGHTNESS_64NT */
	68,	/* IBRIGHTNESS_68NT */
	72,	/* IBRIGHTNESS_72NT */
	77,	/* IBRIGHTNESS_77NT */
	82,	/* IBRIGHTNESS_82NT */
	87,	/* IBRIGHTNESS_87NT */
	93,	/* IBRIGHTNESS_93NT */
	98,	/* IBRIGHTNESS_98NT */
	105,	/* IBRIGHTNESS_105NT */
	111,	/* IBRIGHTNESS_111NT */
	119,	/* IBRIGHTNESS_119NT */
	126,	/* IBRIGHTNESS_126NT */
	134,	/* IBRIGHTNESS_134NT */
	143,	/* IBRIGHTNESS_143NT */
	152,	/* IBRIGHTNESS_152NT */
	162,	/* IBRIGHTNESS_162NT */
	172,	/* IBRIGHTNESS_172NT */
	183,	/* IBRIGHTNESS_183NT */
	195,	/* IBRIGHTNESS_195NT */
	207,	/* IBRIGHTNESS_207NT */
	220,	/* IBRIGHTNESS_220NT */
	234,	/* IBRIGHTNESS_234NT */
	249,	/* IBRIGHTNESS_249NT */
	265,	/* IBRIGHTNESS_265NT */
	282,	/* IBRIGHTNESS_282NT */
	300,	/* IBRIGHTNESS_300NT */
	300	/* IBRIGHTNESS_400NT */
};

static const int gamma_default_0[IV_MAX*CI_MAX] = {
	0x00, 0x00, 0x00,	/* IV_VT */
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x100, 0x100, 0x100	/* IV_255 */
};

static const int *gamma_default = gamma_default_0;

static const struct formular_t gamma_formula[IV_MAX] = {
	{0, 860},	/* IV_VT */
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{72, 860}	/* IV_255 */
};

static const int vt_voltage_value[] = {
	0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 138, 148, 158, 168, 178, 186
};

static const int brightness_base_table[IBRIGHTNESS_MAX] = {
	110,	/* IBRIGHTNESS_10NT */
	110,	/* IBRIGHTNESS_11NT */
	110,	/* IBRIGHTNESS_12NT */
	110,	/* IBRIGHTNESS_13NT */
	110,	/* IBRIGHTNESS_14NT */
	110,	/* IBRIGHTNESS_15NT */
	110,	/* IBRIGHTNESS_16NT */
	110,	/* IBRIGHTNESS_17NT */
	110,	/* IBRIGHTNESS_19NT */
	110,	/* IBRIGHTNESS_20NT */
	110,	/* IBRIGHTNESS_21NT */
	110,	/* IBRIGHTNESS_22NT */
	110,	/* IBRIGHTNESS_24NT */
	110,	/* IBRIGHTNESS_25NT */
	110,	/* IBRIGHTNESS_27NT */
	110,	/* IBRIGHTNESS_29NT */
	110,	/* IBRIGHTNESS_30NT */
	110,	/* IBRIGHTNESS_32NT */
	110,	/* IBRIGHTNESS_34NT */
	110,	/* IBRIGHTNESS_37NT */
	110,	/* IBRIGHTNESS_39NT */
	110,	/* IBRIGHTNESS_41NT */
	110,	/* IBRIGHTNESS_44NT */
	110,	/* IBRIGHTNESS_47NT */
	110,	/* IBRIGHTNESS_50NT */
	110,	/* IBRIGHTNESS_53NT */
	110,	/* IBRIGHTNESS_56NT */
	110,	/* IBRIGHTNESS_60NT */
	110,	/* IBRIGHTNESS_64NT */
	110,	/* IBRIGHTNESS_68NT */
	110,	/* IBRIGHTNESS_72NT */
	110,	/* IBRIGHTNESS_77NT */
	110,	/* IBRIGHTNESS_82NT */
	110,	/* IBRIGHTNESS_87NT */
	110,	/* IBRIGHTNESS_93NT */
	110,	/* IBRIGHTNESS_98NT */
	110,	/* IBRIGHTNESS_105NT */
	179,	/* IBRIGHTNESS_111NT */
	191,	/* IBRIGHTNESS_119NT */
	201,	/* IBRIGHTNESS_126NT */
	213,	/* IBRIGHTNESS_134NT */
	226,	/* IBRIGHTNESS_143NT */
	239,	/* IBRIGHTNESS_152NT */
	254,	/* IBRIGHTNESS_162NT */
	269,	/* IBRIGHTNESS_172NT */
	183,	/* IBRIGHTNESS_183NT */
	195,	/* IBRIGHTNESS_195NT */
	207,	/* IBRIGHTNESS_207NT */
	220,	/* IBRIGHTNESS_220NT */
	234,	/* IBRIGHTNESS_234NT */
	249,	/* IBRIGHTNESS_249NT */
	265,	/* IBRIGHTNESS_265NT */
	282,	/* IBRIGHTNESS_282NT */
	300,	/* IBRIGHTNESS_300NT */
	300	/* IBRIGHTNESS_400NT */
};

static const int *gamma_curve_tables[IBRIGHTNESS_MAX] = {
	gamma_curve_1p95_table,	/* IBRIGHTNESS_10NT */
	gamma_curve_1p95_table,	/* IBRIGHTNESS_11NT */
	gamma_curve_1p95_table,	/* IBRIGHTNESS_12NT */
	gamma_curve_1p95_table,	/* IBRIGHTNESS_13NT */
	gamma_curve_1p95_table,	/* IBRIGHTNESS_14NT */
	gamma_curve_1p95_table,	/* IBRIGHTNESS_15NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_16NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_17NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_19NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_20NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_21NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_22NT */
	gamma_curve_2p00_table,	/* IBRIGHTNESS_24NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_25NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_27NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_29NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_30NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_32NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_34NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_37NT */
	gamma_curve_2p05_table,	/* IBRIGHTNESS_39NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_41NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_44NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_47NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_50NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_53NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_56NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_60NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_64NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_68NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_72NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_77NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_82NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_87NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_93NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_98NT */
	gamma_curve_2p10_table,	/* IBRIGHTNESS_105NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_111NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_119NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_126NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_134NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_143NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_152NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_162NT */
	gamma_curve_2p12_table,	/* IBRIGHTNESS_172NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_183NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_195NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_207NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_220NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_234NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_249NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_265NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_282NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_300NT */
	gamma_curve_2p20_table	/* IBRIGHTNESS_400NT */
};

static const int *gamma_curve_lut = gamma_curve_2p20_table;

static const unsigned char aor_cmd[IBRIGHTNESS_MAX][2] = {
	{0x03, 0x77},	/* IBRIGHTNESS_10NT */
	{0x03, 0x73},	/* IBRIGHTNESS_11NT */
	{0x03, 0x69},	/* IBRIGHTNESS_12NT */
	{0x03, 0x65},	/* IBRIGHTNESS_13NT */
	{0x03, 0x61},	/* IBRIGHTNESS_14NT */
	{0x03, 0x55},	/* IBRIGHTNESS_15NT */
	{0x03, 0x50},	/* IBRIGHTNESS_16NT */
	{0x03, 0x45},	/* IBRIGHTNESS_17NT */
	{0x03, 0x35},	/* IBRIGHTNESS_19NT */
	{0x03, 0x27},	/* IBRIGHTNESS_20NT */
	{0x03, 0x23},	/* IBRIGHTNESS_21NT */
	{0x03, 0x17},	/* IBRIGHTNESS_22NT */
	{0x03, 0x11},	/* IBRIGHTNESS_24NT */
	{0x03, 0x04},	/* IBRIGHTNESS_25NT */
	{0x02, 0xF4},	/* IBRIGHTNESS_27NT */
	{0x02, 0xE3},	/* IBRIGHTNESS_29NT */
	{0x02, 0xD7},	/* IBRIGHTNESS_30NT */
	{0x02, 0xC6},	/* IBRIGHTNESS_32NT */
	{0x02, 0xB7},	/* IBRIGHTNESS_34NT */
	{0x02, 0xA1},	/* IBRIGHTNESS_37NT */
	{0x02, 0x91},	/* IBRIGHTNESS_39NT */
	{0x02, 0x78},	/* IBRIGHTNESS_41NT */
	{0x02, 0x62},	/* IBRIGHTNESS_44NT */
	{0x02, 0x45},	/* IBRIGHTNESS_47NT */
	{0x02, 0x30},	/* IBRIGHTNESS_50NT */
	{0x02, 0x13},	/* IBRIGHTNESS_53NT */
	{0x01, 0xF5},	/* IBRIGHTNESS_56NT */
	{0x01, 0xD3},	/* IBRIGHTNESS_60NT */
	{0x01, 0xB1},	/* IBRIGHTNESS_64NT */
	{0x01, 0x87},	/* IBRIGHTNESS_68NT */
	{0x01, 0x63},	/* IBRIGHTNESS_72NT */
	{0x01, 0x35},	/* IBRIGHTNESS_77NT */
	{0x01, 0x05},	/* IBRIGHTNESS_82NT */
	{0x00, 0xD5},	/* IBRIGHTNESS_87NT */
	{0x00, 0xA1},	/* IBRIGHTNESS_93NT */
	{0x00, 0x6F},	/* IBRIGHTNESS_98NT */
	{0x00, 0x31},	/* IBRIGHTNESS_105NT */
	{0x01, 0x86},	/* IBRIGHTNESS_111NT */
	{0x01, 0x86},	/* IBRIGHTNESS_119NT */
	{0x01, 0x86},	/* IBRIGHTNESS_126NT */
	{0x01, 0x86},	/* IBRIGHTNESS_134NT */
	{0x01, 0x86},	/* IBRIGHTNESS_143NT */
	{0x01, 0x86},	/* IBRIGHTNESS_152NT */
	{0x01, 0x86},	/* IBRIGHTNESS_162NT */
	{0x01, 0x86},	/* IBRIGHTNESS_172NT */
	{0x00, 0x08},	/* IBRIGHTNESS_183NT */
	{0x00, 0x08},	/* IBRIGHTNESS_195NT */
	{0x00, 0x08},	/* IBRIGHTNESS_207NT */
	{0x00, 0x08},	/* IBRIGHTNESS_220NT */
	{0x00, 0x08},	/* IBRIGHTNESS_234NT */
	{0x00, 0x08},	/* IBRIGHTNESS_249NT */
	{0x00, 0x08},	/* IBRIGHTNESS_265NT */
	{0x00, 0x08},	/* IBRIGHTNESS_282NT */
	{0x00, 0x08},	/* IBRIGHTNESS_300NT */
	{0x00, 0x08}	/* IBRIGHTNESS_400NT */
};

static const int offset_gradation[IBRIGHTNESS_MAX][IV_MAX] = {	/* V0 ~ V255 */
	{0, -3, -3, 0, 2, 3, 3, 2, 1, 0},     /* IBRIGHTNESS_10NT */
	{0, -3, -3, 0, 2, 3, 3, 2, 1, 0},     /* IBRIGHTNESS_11NT */
	{0, -2, -3, 0, 2, 3, 3, 1, 1, 0},     /* IBRIGHTNESS_12NT */
	{0, -2, -3, 0, 2, 3, 3, 1, 1, 0},     /* IBRIGHTNESS_13NT */
	{0, -2, -3, -1, 1, 2, 3, 1, 0, 0},    /* IBRIGHTNESS_14NT */
	{0, -2, -3, -1, 1, 1, 2, 1, 1, 0},    /* IBRIGHTNESS_15NT */
	{0, -2, -3, 0, 2, 3, 3, 2, 1, 0},     /* IBRIGHTNESS_16NT */
	{0, -1, -2, 0, 2, 3, 3, 1, 1, 0},     /* IBRIGHTNESS_17NT */
	{0, -1, -2, 0, 2, 3, 3, 1, 1, 0},     /* IBRIGHTNESS_19NT */
	{0, -1, -2, 0, 1, 2, 3, 1, 1, 0},     /* IBRIGHTNESS_20NT */
	{0, -1, -2, 0, 1, 2, 3, 1, 1, 0},     /* IBRIGHTNESS_21NT */
	{0, -2, -2, 0, 1, 2, 2, 2, 1, 0},     /* IBRIGHTNESS_22NT */
	{0, -2, -2, 0, 1, 2, 2, 2, 1, 0},     /* IBRIGHTNESS_24NT */
	{0, -1, -1, 2, 3, 3, 4, 2, 1, 0},     /* IBRIGHTNESS_25NT */
	{0, -1, -1, 2, 3, 3, 4, 2, 1, 0},     /* IBRIGHTNESS_27NT */
	{0, -1, -1, 2, 3, 2, 4, 2, 1, 0},     /* IBRIGHTNESS_29NT */
	{0, -1, -1, 2, 3, 2, 4, 2, 1, 0},     /* IBRIGHTNESS_30NT */
	{0, -1, -1, 2, 3, 2, 4, 2, 1, 0},     /* IBRIGHTNESS_32NT */
	{0, -1, -1, 1, 2, 2, 4, 2, 1, 0},     /* IBRIGHTNESS_34NT */
	{0, -1, -1, 1, 2, 2, 3, 1, 0, 0},     /* IBRIGHTNESS_37NT */
	{0, -1, -1, 1, 2, 1, 2, 1, 0, 0},     /* IBRIGHTNESS_39NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_41NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_44NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_47NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_50NT */
	{0, 0, 0, 1, 2, 1, 3, 2, 1, 0},       /* IBRIGHTNESS_53NT */
	{0, 0, 0, 1, 2, 1, 3, 1, 1, 0},       /* IBRIGHTNESS_56NT */
	{0, 0, 0, 1, 2, 1, 3, 1, 1, 0},       /* IBRIGHTNESS_60NT */
	{0, 0, 0, 1, 1, 1, 2, 1, 1, 0},       /* IBRIGHTNESS_64NT */
	{0, 0, 0, 1, 1, 1, 2, 1, 1, 0},       /* IBRIGHTNESS_68NT */
	{0, 0, 0, 1, 1, 1, 2, 1, 1, 0},       /* IBRIGHTNESS_72NT */
	{0, 0, 0, 1, 1, 1, 1, 1, 1, 0},       /* IBRIGHTNESS_77NT */
	{0, 0, 0, 1, 1, 1, 1, 1, 1, 0},       /* IBRIGHTNESS_82NT */
	{0, 0, 0, 0, 0, 0, 1, 1, 1, 0},       /* IBRIGHTNESS_87NT */
	{0, 0, 0, 0, 0, 0, 1, 1, 1, 0},       /* IBRIGHTNESS_93NT */
	{0, 0, 0, 0, 0, 0, 1, 1, 1, 0},       /* IBRIGHTNESS_98NT */
	{0, 0, -1, -1, -1, -1, 0, 0, 0, 0},   /* IBRIGHTNESS_105NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_111NT */
	{0, 0, 0, 1, 2, 2, 2, 2, 1, 0},       /* IBRIGHTNESS_119NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_126NT */
	{0, 0, -1, 1, 2, 2, 3, 2, 1, 0},      /* IBRIGHTNESS_134NT */
	{0, 0, -1, 1, 2, 2, 2, 2, 1, 0},      /* IBRIGHTNESS_143NT */
	{0, 0, -1, 1, 2, 2, 3, 2, 1, 0},      /* IBRIGHTNESS_152NT */
	{0, 0, -1, 1, 1, 1, 1, 2, 1, 0},      /* IBRIGHTNESS_162NT */
	{0, 0, -2, 1, 1, 1, 1, 2, 1, 0},      /* IBRIGHTNESS_172NT */
	{0, 1, 1, 2, 2, 2, 4, 2, 1, 0},       /* IBRIGHTNESS_183NT */
	{0, 1, 1, 2, 2, 2, 3, 2, 1, 0},       /* IBRIGHTNESS_195NT */
	{0, 1, 1, 2, 1, 2, 3, 1, 0, 0},       /* IBRIGHTNESS_207NT */
	{0, 0, 0, 1, 2, 2, 2, 2, 1, 0},       /* IBRIGHTNESS_220NT */
	{0, 0, 0, 1, 2, 2, 3, 3, 2, 0},       /* IBRIGHTNESS_234NT */
	{0, 0, 0, 1, 2, 2, 3, 2, 0, 0},       /* IBRIGHTNESS_249NT */
	{0, 0, 0, 0, 1, 1, 2, 2, 0, 0},       /* IBRIGHTNESS_265NT */
	{0, 0, -1, 1, 1, 1, 1, 2, 1, 0},      /* IBRIGHTNESS_282NT */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* IBRIGHTNESS_300NT */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}        /* IBRIGHTNESS_400NT */
};

static const int offset_color[IBRIGHTNESS_MAX][CI_MAX * IV_MAX] = {	/* V0 ~ V255 */
	{0, 0, 0, 0, 0, 0, -12, -9, 15, -3, -9, 14, -3, -2, 6, 0, -2, 3, 0, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -12, -9, 15, -3, -9, 14, -3, -2, 6, 0, -2, 3, 0, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -12, -9, 15, -4, -9, 13, -2, -2, 5, 0, -2, 3, 0, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -11, -8, 15, -4, -9, 13, -2, -2, 5, 0, -2, 3, 0, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -11, -8, 15, -4, -9, 11, 0, -2, 5, 0, -1, 3, 0, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -8, 15, -5, -8, 11, -1, -2, 5, -1, -2, 3, -1, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -7, 15, -5, -8, 11, 0, -1, 5, 0, -1, 3, -1, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -6, 15, -5, -7, 11, 0, -2, 4, 0, -1, 3, -1, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -6, -8, 10, 0, -1, 4, 0, -1, 2, -1, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -6, -8, 9, -1, -2, 3, 0, -1, 2, 0, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -6, -7, 9, -1, -2, 3, 0, -1, 2, 0, -2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -3, -6, 8, -1, -2, 3, 0, -1, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, -5, 15, -3, -5, 7, -1, -2, 3, 0, -1, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, -5, 15, -3, -5, 7, -1, -3, 2, 1, 0, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, -5, 15, -3, -5, 7, -1, -3, 2, 1, 0, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -2, -4, 6, 0, -1, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -2, -4, 6, 0, -1, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 15, -2, -4, 6, 0, -1, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -5, 15, -2, -4, 5, 0, -1, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -5, 14, -2, -4, 5, 0, -1, 2, 1, 0, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -5, 14, -1, -4, 4, -1, -1, 2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -5, 14, -1, -3, 4, -1, -1, 2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -10, -5, 13, -1, -3, 4, -1, -1, 2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 12, -1, -3, 3, -1, -1, 2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, -5, 11, -1, -3, 3, 0, -1, 2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, -5, 10, -1, -3, 3, 0, -1, 2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, -4, 9, -1, -3, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -7, -4, 8, -2, -3, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -7, -4, 7, -2, -3, 2, 0, -1, 1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -6, -3, 6, -1, -2, 1, 0, 0, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -6, -2, 5, -1, -2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -6, -2, 4, -1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, -2, 3, -1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, -2, 2, -1, -2, 0, -1, 0, 1, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, -1, 1, -1, -2, 0, -1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, -1, 1, -1, -2, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, 0, 0, -1, -1, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 5, 0, -1, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 5, 0, -2, 3, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 5, 0, -1, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 4, 0, -1, 2, -1, -1, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 4, 0, -1, 2, 0, -1, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 4, 0, 0, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 4, 0, -1, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 0, 4, 1, 0, 2, -1, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#endif /* __DYNAMIC_AID_XXXX_H */
