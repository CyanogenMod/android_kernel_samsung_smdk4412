/* drivers/video/samsung/aid_s6e8aa0a.h
 *
 * Copyright (C) 2012 Samsung Electronics Co, Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __AID_S6E8AA0_H__
#define __AID_S6E8AA0_H__

#include "smart_dimming.h"

#define aid_300nit_260nit_F8_1st	0x3D
#define aid_300nit_260nit_F8_18th	0x04
#define aid_250nit_190nit_F8_1st	0x3D
#define aid_250nit_190nit_F8_18th	0x04
#define aid_188nit_182nit_F8_1st	0x7D
#define aid_188nit_F8_18th		0x0D
#define aid_186nit_F8_18th		0x1A
#define aid_184nit_F8_18th		0x27
#define aid_182nit_F8_18th		0x34
#define aid_180nit_110nit_F8_1st	0x7D
#define aid_180nit_110nit_F8_18th	0x42
#define aid_100nit_20nit_F8_1st		0x7D
#define aid_100nit_F8_18th		0x12
#define aid_90nit_F8_18th		0x22
#define aid_80nit_F8_18th		0x32
#define aid_70nit_F8_18th		0x41
#define aid_60nit_F8_18th		0x50
#define aid_50nit_F8_18th		0x5F
#define aid_40nit_F8_18th		0x6D
#define aid_30nit_F8_18th		0x7A
#define aid_20nit_F8_18th		0x88
#define AOR40_BASE_188			199
#define AOR40_BASE_186			211
#define AOR40_BASE_184			229
#define AOR40_BASE_182			247
#define AOR40_BASE_180			270
#define AOR40_BASE_170			256
#define AOR40_BASE_160			242
#define AOR40_BASE_150			228
#define AOR40_BASE_140			213
#define AOR40_BASE_130			199
#define AOR40_BASE_120			185
#define AOR40_BASE_110			170
#define base_20to100			110

static const struct rgb_offset_info aid_rgb_fix_table[] = {
	{GAMMA_180CD, IV_15, CI_RED, 1},  {GAMMA_180CD, IV_15, CI_GREEN, -1},
	{GAMMA_180CD, IV_15, CI_BLUE, 4},
	{GAMMA_170CD, IV_15, CI_RED, 1},  {GAMMA_170CD, IV_15, CI_GREEN, -1},
	{GAMMA_170CD, IV_15, CI_BLUE, 4},
	{GAMMA_160CD, IV_15, CI_RED, 1},  {GAMMA_160CD, IV_15, CI_GREEN, -1},
	{GAMMA_160CD, IV_15, CI_BLUE, 4},
	{GAMMA_150CD, IV_15, CI_RED, 1},  {GAMMA_150CD, IV_15, CI_GREEN, -1},
	{GAMMA_150CD, IV_15, CI_BLUE, 4},
	{GAMMA_140CD, IV_15, CI_RED, 1},  {GAMMA_140CD, IV_15, CI_GREEN, -1},
	{GAMMA_140CD, IV_15, CI_BLUE, 4},
	{GAMMA_130CD, IV_15, CI_RED, 1},  {GAMMA_130CD, IV_15, CI_GREEN, -1},
	{GAMMA_130CD, IV_15, CI_BLUE, 4},
	{GAMMA_120CD, IV_15, CI_RED, 1},  {GAMMA_120CD, IV_15, CI_GREEN, -1},
	{GAMMA_120CD, IV_15, CI_BLUE, 4},
	{GAMMA_110CD, IV_15, CI_RED, 1},  {GAMMA_110CD, IV_15, CI_GREEN, -1},
	{GAMMA_110CD, IV_15, CI_BLUE, 4},
	{GAMMA_100CD, IV_15, CI_RED, -2}, {GAMMA_100CD, IV_15, CI_GREEN, -4},
	{GAMMA_90CD, IV_15, CI_RED, -4},  {GAMMA_90CD, IV_15, CI_GREEN, -7},
	{GAMMA_80CD, IV_15, CI_RED, -8}, {GAMMA_80CD, IV_15, CI_GREEN, -12},
	{GAMMA_70CD, IV_15, CI_RED, -12}, {GAMMA_70CD, IV_15, CI_GREEN, -21},
	{GAMMA_60CD, IV_15, CI_RED, -18}, {GAMMA_60CD, IV_15, CI_GREEN, -46},
	{GAMMA_50CD, IV_15, CI_RED, -21}, {GAMMA_50CD, IV_15, CI_GREEN, -46},
	{GAMMA_50CD, IV_15, CI_BLUE, 5},
	{GAMMA_40CD, IV_15, CI_RED, -16}, {GAMMA_40CD, IV_15, CI_GREEN, -46},
	{GAMMA_40CD, IV_15, CI_BLUE, 15},
	{GAMMA_30CD, IV_15, CI_RED, -12}, {GAMMA_30CD, IV_15, CI_GREEN, -46},
	{GAMMA_30CD, IV_15, CI_BLUE, 26},
	{GAMMA_20CD, IV_15, CI_RED, -6}, {GAMMA_20CD, IV_15, CI_GREEN, -46},
	{GAMMA_20CD, IV_15, CI_BLUE, 44},
	{GAMMA_20CD, IV_35, CI_RED, -5}, {GAMMA_20CD, IV_35, CI_GREEN, -16},
};

static unsigned char aid_command_20[] = {
	aid_20nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_30[] = {
	aid_30nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_40[] = {
	aid_40nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_50[] = {
	aid_50nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_60[] = {
	aid_60nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_70[] = {
	aid_70nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_80[] = {
	aid_80nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_90[] = {
	aid_90nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_100[] = {
	aid_100nit_F8_18th,
	aid_100nit_20nit_F8_1st,
};

static unsigned char aid_command_110[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_120[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_130[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_140[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_150[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_160[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_170[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_180[] = {
	aid_180nit_110nit_F8_18th,
	aid_180nit_110nit_F8_1st,
};

static unsigned char aid_command_182[] = {
	aid_182nit_F8_18th,
	aid_188nit_182nit_F8_1st,
};

static unsigned char aid_command_184[] = {
	aid_184nit_F8_18th,
	aid_188nit_182nit_F8_1st,
};

static unsigned char aid_command_186[] = {
	aid_186nit_F8_18th,
	aid_188nit_182nit_F8_1st,
};

static unsigned char aid_command_188[] = {
	aid_188nit_F8_18th,
	aid_188nit_182nit_F8_1st,
};

static unsigned char aid_command_190[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_200[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_210[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_220[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_230[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_240[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_250[] = {
	aid_250nit_190nit_F8_18th,
	aid_250nit_190nit_F8_1st,
};

static unsigned char aid_command_300[] = {
	aid_300nit_260nit_F8_18th,
	aid_300nit_260nit_F8_1st,
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

#endif /* __AID_S6E8AA0_H__ */
