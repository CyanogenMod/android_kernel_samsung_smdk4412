#ifndef __AID_S6EVR01_H__
#define __AID_S6EVR01_H__

#include "smart_dimming_s6evr01.h"

#define aid_190nit_300nit		0xFF
#define aid_188nit			0xEA
#define aid_186nit			0xD6
#define aid_184nit			0xC2
#define aid_182nit			0xAD
#define aid_110nit_180nit		0x99
#define aid_108nit			0xA4
#define aid_106nit			0xB0
#define aid_104nit			0xBC
#define aid_102nit			0xC7
#define aid_100nit			0xEC
#define aid_90nit				0xD3
#define aid_80nit				0xBA
#define aid_70nit				0xA1
#define aid_60nit				0x89
#define aid_50nit				0x71
#define aid_40nit				0x5A
#define aid_30nit				0x43
#define aid_20nit				0x2E

#define AOR40_BASE_188		203
#define AOR40_BASE_186		214
#define AOR40_BASE_184		230
#define AOR40_BASE_182		251
#define AOR40_BASE_180		273
#define AOR40_BASE_170		259
#define AOR40_BASE_160		246
#define AOR40_BASE_150		232
#define AOR40_BASE_140		219
#define AOR40_BASE_130		205
#define AOR40_BASE_120		192
#define AOR40_BASE_110		178
#define AOR40_BASE_108		164
#define AOR40_BASE_106		151
#define AOR40_BASE_104		141
#define AOR40_BASE_102		131
#define base_20to100			110

static const struct rgb_offset_info aid_rgb_fix_table[] = {
	{GAMMA_180CD, IV_11, CI_RED, -3}, {GAMMA_180CD, IV_11, CI_BLUE, 7},
	{GAMMA_170CD, IV_11, CI_RED, -3}, {GAMMA_170CD, IV_11, CI_BLUE, 7},
	{GAMMA_160CD, IV_11, CI_RED, -3}, {GAMMA_160CD, IV_11, CI_BLUE, 7},
	{GAMMA_150CD, IV_11, CI_RED, -3}, {GAMMA_150CD, IV_11, CI_BLUE, 7},
	{GAMMA_140CD, IV_11, CI_RED, -3}, {GAMMA_140CD, IV_11, CI_BLUE, 7},
	{GAMMA_130CD, IV_11, CI_RED, -3}, {GAMMA_130CD, IV_11, CI_BLUE, 7},
	{GAMMA_120CD, IV_11, CI_RED, -3}, {GAMMA_120CD, IV_11, CI_BLUE, 7},
	{GAMMA_110CD, IV_11, CI_RED, -3}, {GAMMA_110CD, IV_11, CI_BLUE, 7},
	{GAMMA_180CD, IV_23, CI_RED, 1}, {GAMMA_180CD, IV_23, CI_BLUE, 1},
	{GAMMA_170CD, IV_23, CI_RED, 1}, {GAMMA_170CD, IV_23, CI_BLUE, 1},
	{GAMMA_160CD, IV_23, CI_RED, 1}, {GAMMA_160CD, IV_23, CI_BLUE, 1},
	{GAMMA_150CD, IV_23, CI_RED, 1}, {GAMMA_150CD, IV_23, CI_BLUE, 1},
	{GAMMA_140CD, IV_23, CI_RED, 1}, {GAMMA_140CD, IV_23, CI_BLUE, 1},
	{GAMMA_130CD, IV_23, CI_RED, 1}, {GAMMA_130CD, IV_23, CI_BLUE, 1},
	{GAMMA_120CD, IV_23, CI_RED, 1}, {GAMMA_120CD, IV_23, CI_BLUE, 1},
	{GAMMA_110CD, IV_23, CI_RED, 1}, {GAMMA_110CD, IV_23, CI_BLUE, 1},
	{GAMMA_108CD, IV_23, CI_RED, -2}, {GAMMA_108CD, IV_23, CI_BLUE, 5},
	{GAMMA_106CD, IV_23, CI_RED, -1}, {GAMMA_106CD, IV_23, CI_BLUE, 3},
	{GAMMA_104CD, IV_23, CI_BLUE, 1},
	{GAMMA_80CD, IV_11, CI_RED, -2},
	{GAMMA_70CD, IV_11, CI_RED, -11}, {GAMMA_70CD, IV_11, CI_BLUE, 2},
	{GAMMA_60CD, IV_11, CI_RED, -26}, {GAMMA_60CD, IV_11, CI_BLUE, 5},
	{GAMMA_50CD, IV_11, CI_RED, -37}, {GAMMA_50CD, IV_11, CI_BLUE, 7},
	{GAMMA_40CD, IV_11, CI_RED, -56}, {GAMMA_40CD, IV_11, CI_BLUE, 10},
	{GAMMA_30CD, IV_11, CI_RED, -72}, {GAMMA_30CD, IV_11, CI_BLUE, 15},
	{GAMMA_20CD, IV_11, CI_RED, -93}, {GAMMA_20CD, IV_11, CI_BLUE, 21},
	{GAMMA_60CD, IV_23, CI_RED, -1}, {GAMMA_60CD, IV_23, CI_BLUE, 1},
	{GAMMA_50CD, IV_23, CI_RED, -2}, {GAMMA_50CD, IV_23, CI_BLUE, 2},
	{GAMMA_40CD, IV_23, CI_RED, -2}, {GAMMA_40CD, IV_23, CI_BLUE, 4},
	{GAMMA_30CD, IV_23, CI_RED, -5}, {GAMMA_30CD, IV_23, CI_BLUE, 7},
	{GAMMA_20CD, IV_23, CI_RED, -11}, {GAMMA_20CD, IV_23, CI_BLUE, 12},
	{GAMMA_90CD, IV_35, CI_BLUE, 1},
	{GAMMA_80CD, IV_35, CI_BLUE, 1},
	{GAMMA_70CD, IV_35, CI_BLUE, 1},
	{GAMMA_60CD, IV_35, CI_GREEN, -1}, {GAMMA_60CD, IV_35, CI_BLUE, 2},
	{GAMMA_50CD, IV_35, CI_RED, 1}, {GAMMA_50CD, IV_35, CI_GREEN, -1}, {GAMMA_50CD, IV_35, CI_BLUE, 3},
	{GAMMA_40CD, IV_35, CI_RED, 1}, {GAMMA_40CD, IV_35, CI_GREEN, -1}, {GAMMA_40CD, IV_35, CI_BLUE, 5},
	{GAMMA_30CD, IV_35, CI_RED, 1}, {GAMMA_30CD, IV_35, CI_GREEN, -2}, {GAMMA_30CD, IV_35, CI_BLUE, 6},
	{GAMMA_20CD, IV_35, CI_RED, 1}, {GAMMA_20CD, IV_35, CI_GREEN, -4}, {GAMMA_20CD, IV_35, CI_BLUE, 8},
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
	aid_190nit_300nit
};

static unsigned char aid_command_200[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_210[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_220[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_230[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_240[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_250[] = {
	aid_190nit_300nit
};

static unsigned char aid_command_300[] = {
	aid_190nit_300nit
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
