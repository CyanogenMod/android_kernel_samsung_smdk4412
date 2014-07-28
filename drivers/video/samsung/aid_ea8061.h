#ifndef __AID_EA8061_H__
#define __AID_EA8061_H__

#include "smart_dimming_ea8061.h"

#define aid_300nit_260nit_B3_1st	0x00
#define aid_300nit_260nit_B3_2nd	0x0A
#define aid_250nit_190nit_B3_1st	0x00
#define aid_250nit_190nit_B3_2nd	0x0A
#define aid_188nit_186nit_B3_1st	0x00
#define aid_184nit_182nit_B3_1st	0x01
#define aid_188nit_B3_2nd		0x68
#define aid_186nit_B3_2nd		0xCF
#define aid_184nit_B3_2nd		0x37
#define aid_182nit_B3_2nd		0x9F
#define aid_180nit_110nit_B3_1st	0x02
#define aid_180nit_110nit_B3_2nd	0x06
#define aid_102nit_B3_1st		0x00
#define aid_108nit_104nit_B3_1st	0x01
#define aid_108nit_B3_2nd		0xC4
#define aid_106nit_B3_2nd		0x82
#define aid_104nit_B3_2nd		0x40
#define aid_102nit_B3_2nd		0xFE
#define aid_100nit_B3_1st	0x00
#define aid_90nit_80nit_B3_1st	0x01
#define aid_70nit_60nit_B3_1st	0x02
#define aid_50nit_30nit_B3_1st	0x03
#define aid_20nit_B3_1st	0x04
#define aid_100nit_B3_2nd		0xBC
#define aid_90nit_B3_2nd		0x36
#define aid_80nit_B3_2nd		0xAB
#define aid_70nit_B3_2nd		0x25
#define aid_60nit_B3_2nd		0x95
#define aid_50nit_B3_2nd		0x0A
#define aid_40nit_B3_2nd		0x74
#define aid_30nit_B3_2nd		0xDF
#define aid_20nit_B3_2nd		0x45
#define AOR40_BASE_188		202
#define AOR40_BASE_186		215
#define AOR40_BASE_184		230
#define AOR40_BASE_182		250
#define AOR40_BASE_180		275
#define AOR40_BASE_170		260
#define AOR40_BASE_160		246
#define AOR40_BASE_150		230
#define AOR40_BASE_140		217
#define AOR40_BASE_130		202
#define AOR40_BASE_120		184
#define AOR40_BASE_110		169
#define AOR40_BASE_108		154
#define AOR40_BASE_106		141
#define AOR40_BASE_104		130
#define AOR40_BASE_102		120
#define base_20to100			110

static const struct rgb_offset_info aid_rgb_fix_table_SM2[] = {
	{GAMMA_184CD, IV_11, CI_BLUE, 1},
	{GAMMA_182CD, IV_11, CI_GREEN, -1}, {GAMMA_182CD, IV_11, CI_BLUE, 2},
	{GAMMA_180CD, IV_11, CI_RED, -1}, {GAMMA_180CD, IV_11, CI_GREEN, -2}, {GAMMA_180CD, IV_11, CI_BLUE, 3},
	{GAMMA_170CD, IV_11, CI_RED, -1}, {GAMMA_170CD, IV_11, CI_GREEN, -2}, {GAMMA_170CD, IV_11, CI_BLUE, 3},
	{GAMMA_160CD, IV_11, CI_RED, -1}, {GAMMA_160CD, IV_11, CI_GREEN, -2}, {GAMMA_160CD, IV_11, CI_BLUE, 3},
	{GAMMA_150CD, IV_11, CI_RED, -1}, {GAMMA_150CD, IV_11, CI_GREEN, -2}, {GAMMA_150CD, IV_11, CI_BLUE, 3},
	{GAMMA_140CD, IV_11, CI_RED, -1}, {GAMMA_140CD, IV_11, CI_GREEN, -2}, {GAMMA_140CD, IV_11, CI_BLUE, 3},
	{GAMMA_130CD, IV_11, CI_RED, -1}, {GAMMA_130CD, IV_11, CI_GREEN, -2}, {GAMMA_130CD, IV_11, CI_BLUE, 3},
	{GAMMA_120CD, IV_11, CI_RED, -1}, {GAMMA_120CD, IV_11, CI_GREEN, -2}, {GAMMA_120CD, IV_11, CI_BLUE, 3},
	{GAMMA_110CD, IV_11, CI_RED, -1}, {GAMMA_110CD, IV_11, CI_GREEN, -2}, {GAMMA_110CD, IV_11, CI_BLUE, 3},
	{GAMMA_108CD, IV_11, CI_RED, -1}, {GAMMA_108CD, IV_11, CI_GREEN, -2}, {GAMMA_108CD, IV_11, CI_BLUE, 3},
	{GAMMA_106CD, IV_11, CI_RED, -1}, {GAMMA_106CD, IV_11, CI_GREEN, -1}, {GAMMA_106CD, IV_11, CI_BLUE, 3},
	{GAMMA_104CD, IV_11, CI_RED, -2}, {GAMMA_104CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 4},
	{GAMMA_102CD, IV_11, CI_RED, -2}, {GAMMA_102CD, IV_11, CI_BLUE, 4},
	{GAMMA_100CD, IV_11, CI_RED, -2}, {GAMMA_100CD, IV_11, CI_BLUE, 5},
	{GAMMA_90CD, IV_11, CI_RED, -5},  {GAMMA_90CD, IV_11, CI_BLUE, 6},
	{GAMMA_80CD, IV_11, CI_RED, -6},  {GAMMA_80CD, IV_11, CI_BLUE, 8},
	{GAMMA_70CD, IV_11, CI_RED, -7},	{GAMMA_70CD, IV_11, CI_BLUE, 11},
	{GAMMA_60CD, IV_11, CI_RED, -10}, {GAMMA_60CD, IV_11, CI_BLUE, 14},
	{GAMMA_50CD, IV_11, CI_RED, -12}, {GAMMA_50CD, IV_11, CI_BLUE, 19},
	{GAMMA_40CD, IV_11, CI_RED, -18}, {GAMMA_40CD, IV_11, CI_BLUE, 24},
	{GAMMA_30CD, IV_11, CI_RED, -18}, {GAMMA_30CD, IV_11, CI_BLUE, 31},
	{GAMMA_20CD, IV_11, CI_RED, -18}, {GAMMA_20CD, IV_11, CI_BLUE, 39},
	{GAMMA_90CD, IV_23, CI_GREEN, -3},
	{GAMMA_80CD, IV_23, CI_RED, -1}, {GAMMA_80CD, IV_23, CI_GREEN, -4},
	{GAMMA_70CD, IV_23, CI_RED, -3}, {GAMMA_70CD, IV_23, CI_GREEN, -6},
	{GAMMA_60CD, IV_23, CI_RED, -4}, {GAMMA_60CD, IV_23, CI_GREEN, -9},
	{GAMMA_50CD, IV_23, CI_RED, -7}, {GAMMA_50CD, IV_23, CI_GREEN, -9},
	{GAMMA_40CD, IV_23, CI_RED, -12}, {GAMMA_40CD, IV_23, CI_GREEN, -16},
	{GAMMA_30CD, IV_23, CI_RED, -17}, {GAMMA_30CD, IV_23, CI_GREEN, -16}, {GAMMA_30CD, IV_23, CI_BLUE, 2},
	{GAMMA_20CD, IV_23, CI_RED, -22}, {GAMMA_20CD, IV_23, CI_GREEN, -16}, {GAMMA_20CD, IV_23, CI_BLUE, 9},
	{GAMMA_30CD, IV_35, CI_RED, -3}, {GAMMA_30CD, IV_35, CI_GREEN, -14},
	{GAMMA_20CD, IV_35, CI_RED, -11}, {GAMMA_20CD, IV_35, CI_GREEN, -30},
};

static const struct rgb_offset_info aid_rgb_fix_table_M4[] = {
	{GAMMA_180CD, IV_11, CI_GREEN, 1},
	{GAMMA_170CD, IV_11, CI_GREEN, 1},
	{GAMMA_160CD, IV_11, CI_GREEN, 1},
	{GAMMA_150CD, IV_11, CI_GREEN, 1},
	{GAMMA_140CD, IV_11, CI_GREEN, 1},
	{GAMMA_130CD, IV_11, CI_GREEN, 1},
	{GAMMA_120CD, IV_11, CI_GREEN, 1},
	{GAMMA_110CD, IV_11, CI_GREEN, 1},
	{GAMMA_180CD, IV_23, CI_GREEN, 1},
	{GAMMA_170CD, IV_23, CI_GREEN, 1},
	{GAMMA_160CD, IV_23, CI_GREEN, 1},
	{GAMMA_150CD, IV_23, CI_GREEN, 1},
	{GAMMA_140CD, IV_23, CI_GREEN, 1},
	{GAMMA_130CD, IV_23, CI_GREEN, 1},
	{GAMMA_120CD, IV_23, CI_GREEN, 1},
	{GAMMA_110CD, IV_23, CI_GREEN, 1},
	{GAMMA_180CD, IV_35, CI_RED, -10}, {GAMMA_180CD, IV_35, CI_GREEN, 2},
	{GAMMA_170CD, IV_35, CI_RED, -11}, {GAMMA_170CD, IV_35, CI_GREEN, 2},
	{GAMMA_160CD, IV_35, CI_RED, -12}, {GAMMA_160CD, IV_35, CI_GREEN, 2},
	{GAMMA_150CD, IV_35, CI_RED, -13}, {GAMMA_150CD, IV_35, CI_GREEN, 2},
	{GAMMA_140CD, IV_35, CI_RED, -14}, {GAMMA_140CD, IV_35, CI_GREEN, 2},
	{GAMMA_130CD, IV_35, CI_RED, -15}, {GAMMA_130CD, IV_35, CI_GREEN, 2},
	{GAMMA_120CD, IV_35, CI_RED, -16}, {GAMMA_120CD, IV_35, CI_GREEN, 2},
	{GAMMA_110CD, IV_35, CI_RED, -17}, {GAMMA_110CD, IV_35, CI_GREEN, 2},
	{GAMMA_90CD, IV_11, CI_GREEN, 12}, {GAMMA_90CD, IV_11, CI_BLUE, -7},
	{GAMMA_80CD, IV_11, CI_GREEN, 20}, {GAMMA_80CD, IV_11, CI_BLUE, 2},
	{GAMMA_70CD, IV_11, CI_RED, -17}, {GAMMA_70CD, IV_11, CI_GREEN, 20}, {GAMMA_70CD, IV_11, CI_BLUE, 5},
	{GAMMA_60CD, IV_11, CI_RED, -17}, {GAMMA_60CD, IV_11, CI_GREEN, 29}, {GAMMA_60CD, IV_11, CI_BLUE, 16},
	{GAMMA_50CD, IV_11, CI_RED, -17}, {GAMMA_50CD, IV_11, CI_GREEN, 39}, {GAMMA_50CD, IV_11, CI_BLUE, 27},
	{GAMMA_40CD, IV_11, CI_RED, -17}, {GAMMA_40CD, IV_11, CI_GREEN, 49}, {GAMMA_40CD, IV_11, CI_BLUE, 37},
	{GAMMA_30CD, IV_11, CI_RED, -52}, {GAMMA_30CD, IV_11, CI_GREEN, 60}, {GAMMA_30CD, IV_11, CI_BLUE, 52},
	{GAMMA_20CD, IV_11, CI_RED, -56}, {GAMMA_20CD, IV_11, CI_GREEN, 76}, {GAMMA_20CD, IV_11, CI_BLUE, 72},
	{GAMMA_70CD, IV_23, CI_RED, -6}, {GAMMA_70CD, IV_23, CI_GREEN, 3},
	{GAMMA_60CD, IV_23, CI_RED, -13}, {GAMMA_60CD, IV_23, CI_GREEN, 4},
	{GAMMA_50CD, IV_23, CI_RED, -20}, {GAMMA_50CD, IV_23, CI_GREEN, 5},
	{GAMMA_40CD, IV_23, CI_RED, -30}, {GAMMA_40CD, IV_23, CI_GREEN, 6},
	{GAMMA_30CD, IV_23, CI_RED, -35}, {GAMMA_30CD, IV_23, CI_GREEN, -2},
	{GAMMA_20CD, IV_23, CI_RED, -46}, {GAMMA_20CD, IV_23, CI_GREEN, -6}, {GAMMA_20CD, IV_23, CI_BLUE, 11},
	{GAMMA_30CD, IV_35, CI_RED, -23}, {GAMMA_30CD, IV_35, CI_GREEN, 11},
	{GAMMA_20CD, IV_35, CI_RED, -39}, {GAMMA_20CD, IV_35, CI_GREEN, 26},
};

static unsigned char aid_command_20[] = {
	aid_20nit_B3_1st,
	aid_20nit_B3_2nd,
};

static unsigned char aid_command_30[] = {
	aid_50nit_30nit_B3_1st,
	aid_30nit_B3_2nd,
};

static unsigned char aid_command_40[] = {
	aid_50nit_30nit_B3_1st,
	aid_40nit_B3_2nd,
};

static unsigned char aid_command_50[] = {
	aid_50nit_30nit_B3_1st,
	aid_50nit_B3_2nd,
};

static unsigned char aid_command_60[] = {
	aid_70nit_60nit_B3_1st,
	aid_60nit_B3_2nd,
};

static unsigned char aid_command_70[] = {
	aid_70nit_60nit_B3_1st,
	aid_70nit_B3_2nd,
};

static unsigned char aid_command_80[] = {
	aid_90nit_80nit_B3_1st,
	aid_80nit_B3_2nd,
};

static unsigned char aid_command_90[] = {
	aid_90nit_80nit_B3_1st,
	aid_90nit_B3_2nd,
};

static unsigned char aid_command_100[] = {
	aid_100nit_B3_1st,
	aid_100nit_B3_2nd,
};

static unsigned char aid_command_102[] = {
	aid_102nit_B3_1st,
	aid_102nit_B3_2nd,
};

static unsigned char aid_command_104[] = {
	aid_108nit_104nit_B3_1st,
	aid_104nit_B3_2nd,
};

static unsigned char aid_command_106[] = {
	aid_108nit_104nit_B3_1st,
	aid_106nit_B3_2nd,
};

static unsigned char aid_command_108[] = {
	aid_108nit_104nit_B3_1st,
	aid_108nit_B3_2nd,
};

static unsigned char aid_command_110[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_120[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_130[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_140[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_150[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_160[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_170[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_180[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_182[] = {
	aid_184nit_182nit_B3_1st,
	aid_182nit_B3_2nd,
};

static unsigned char aid_command_184[] = {
	aid_184nit_182nit_B3_1st,
	aid_184nit_B3_2nd,
};

static unsigned char aid_command_186[] = {
	aid_188nit_186nit_B3_1st,
	aid_186nit_B3_2nd,
};

static unsigned char aid_command_188[] = {
	aid_188nit_186nit_B3_1st,
	aid_188nit_B3_2nd,
};

static unsigned char aid_command_190[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_200[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_210[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_220[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_230[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_240[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_250[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_300[] = {
	aid_300nit_260nit_B3_1st,
	aid_300nit_260nit_B3_2nd,

};

static unsigned char *aid_command_table[GAMMA_MAX] = {
	aid_command_20,
	aid_command_30,
	aid_command_40,
	aid_command_50,
	aid_command_60,
	aid_command_70,
	aid_command_80,
	aid_command_90,
	aid_command_100,
	aid_command_102,
	aid_command_104,
	aid_command_106,
	aid_command_108,
	aid_command_110,
	aid_command_120,
	aid_command_130,
	aid_command_140,
	aid_command_150,
	aid_command_160,
	aid_command_170,
	aid_command_180,
	aid_command_182,
	aid_command_184,
	aid_command_186,
	aid_command_188,
	aid_command_190,
	aid_command_200,
	aid_command_210,
	aid_command_220,
	aid_command_230,
	aid_command_240,
	aid_command_250,
	aid_command_300
};

#endif
