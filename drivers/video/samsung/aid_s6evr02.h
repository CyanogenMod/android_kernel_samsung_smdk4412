#ifndef __AID_S6EVR02_H__
#define __AID_S6EVR02_H__

#include "smart_dimming_s6evr02.h"

#define aid_300nit		0xFF
#define aid_190nit_250nit		0xFF
#define aid_188nit			0xEA
#define aid_186nit			0xD6
#define aid_184nit			0xC2
#define aid_182nit			0xAD
#define aid_110nit_180nit		0x99
#define aid_108nit			0xF6
#define aid_106nit			0xEF
#define aid_104nit			0xE8
#define aid_102nit			0xE1
#define aid_100nit			0xDA
#define aid_90nit				0xC2
#define aid_80nit				0xAB
#define aid_70nit				0x93
#define aid_60nit				0x7D
#define aid_50nit				0x66
#define aid_40nit				0x51
#define aid_30nit				0x3C
#define aid_20nit				0x28
#define AOR40_BASE_188		202
#define AOR40_BASE_186		215
#define AOR40_BASE_184		230
#define AOR40_BASE_182		250
#define AOR40_BASE_180		275
#define AOR40_BASE_170		260
#define AOR40_BASE_160		246
#define AOR40_BASE_150		231
#define AOR40_BASE_140		217
#define AOR40_BASE_130		202
#define AOR40_BASE_120		188
#define AOR40_BASE_110		169
#define AOR40_BASE_108		110
#define AOR40_BASE_106		110
#define AOR40_BASE_104		110
#define AOR40_BASE_102		110
#define base_20to100			110

static const struct rgb_offset_info aid_rgb_fix_table[] = {
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
	{GAMMA_108CD, IV_11, CI_RED, -1}, {GAMMA_108CD, IV_11, CI_GREEN, -2}, {GAMMA_104CD, IV_11, CI_BLUE, 3},
	{GAMMA_106CD, IV_11, CI_RED, -1}, {GAMMA_106CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 3},
	{GAMMA_104CD, IV_11, CI_RED, -2}, {GAMMA_104CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 4},
	{GAMMA_102CD, IV_11, CI_RED, -2}, {GAMMA_102CD, IV_11, CI_BLUE, 4},
	{GAMMA_100CD, IV_11, CI_RED, -2}, {GAMMA_100CD, IV_11, CI_BLUE, 5},
	{GAMMA_90CD, IV_11, CI_RED, -5}, {GAMMA_90CD, IV_11, CI_BLUE, 6},
	{GAMMA_80CD, IV_11, CI_RED, -6}, {GAMMA_80CD, IV_11, CI_BLUE, 8},
	{GAMMA_70CD, IV_11, CI_RED, -7}, {GAMMA_70CD, IV_11, CI_BLUE, 11},
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

static unsigned char aid_command_20[] = {
	aid_20nit
};

static unsigned char aid_command_30[] = {
	aid_30nit
};

static unsigned char aid_command_40[] = {
	aid_40nit
};

static unsigned char aid_command_50[] = {
	aid_50nit
};

static unsigned char aid_command_60[] = {
	aid_60nit
};

static unsigned char aid_command_70[] = {
	aid_70nit
};

static unsigned char aid_command_80[] = {
	aid_80nit
};

static unsigned char aid_command_90[] = {
	aid_90nit
};

static unsigned char aid_command_100[] = {
	aid_100nit
};

static unsigned char aid_command_102[] = {
	aid_102nit
};

static unsigned char aid_command_104[] = {
	aid_104nit
};

static unsigned char aid_command_106[] = {
	aid_106nit
};

static unsigned char aid_command_108[] = {
	aid_108nit
};

static unsigned char aid_command_110[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_120[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_130[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_140[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_150[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_160[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_170[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_180[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_182[] = {
	aid_182nit
};

static unsigned char aid_command_184[] = {
	aid_184nit
};

static unsigned char aid_command_186[] = {
	aid_186nit
};

static unsigned char aid_command_188[] = {
	aid_188nit
};

static unsigned char aid_command_190[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_200[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_210[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_220[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_230[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_240[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_250[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_300[] = {
	aid_300nit
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
