/*
 * YMU831 ASoC codec driver
 *
 * Copyright (c) 2012 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef YMU831_H
#define YMU831_H

#include <sound/pcm.h>
#include <sound/soc.h>
#include "mcdriver.h"

#define MC_ASOC_NAME			"ymu831"
/*
 * dai: set_clkdiv
 */
/* div_id */
#define MC_ASOC_BCLK_MULT		5

/* div for MC_ASOC_BCLK_MULT */
#define MC_ASOC_LRCK_X64		(0)
#define MC_ASOC_LRCK_X48		(1)
#define MC_ASOC_LRCK_X32		(2)
#define MC_ASOC_LRCK_X512		(3)
#define MC_ASOC_LRCK_X256		(4)
#define MC_ASOC_LRCK_X192		(5)
#define MC_ASOC_LRCK_X128		(6)
#define MC_ASOC_LRCK_X96		(7)
#define MC_ASOC_LRCK_X24		(8)
#define MC_ASOC_LRCK_X16		(9)

/*
 * hwdep: ioctl
 */
#define MC_ASOC_MAGIC			'N'
#define MC_ASOC_IOCTL_SET_CTRL		(1)
#define MC_ASOC_IOCTL_READ_REG		(2)
#define MC_ASOC_IOCTL_WRITE_REG		(3)
#define MC_ASOC_IOCTL_NOTIFY_HOLD	(4)
#define MC_ASOC_IOCTL_GET_DSP_DATA	(5)
#define MC_ASOC_IOCTL_SET_DSP_DATA	(6)

struct ymc_ctrl_args {
	void		*param;
	unsigned long	size;
	unsigned long	option;
};

struct ymc_dspdata_args {
	unsigned char	*buf;
	unsigned long	bufsize;
	unsigned long	size;
};

#define YMC_IOCTL_SET_CTRL \
	_IOW(MC_ASOC_MAGIC, MC_ASOC_IOCTL_SET_CTRL, struct ymc_ctrl_args)

#define YMC_IOCTL_READ_REG \
	_IOWR(MC_ASOC_MAGIC, MC_ASOC_IOCTL_READ_REG, struct MCDRV_REG_INFO)

#define YMC_IOCTL_WRITE_REG \
	_IOWR(MC_ASOC_MAGIC, MC_ASOC_IOCTL_WRITE_REG, struct MCDRV_REG_INFO)

#define YMC_IOCTL_NOTIFY_HOLD \
	_IOWR(MC_ASOC_MAGIC, MC_ASOC_IOCTL_NOTIFY_HOLD, unsigned long)
#define YMC_IOCTL_GET_DSP_DATA \
	_IOWR(MC_ASOC_MAGIC, MC_ASOC_IOCTL_GET_DSP_DATA, \
		struct ymc_dspdata_args)
#define YMC_IOCTL_SET_DSP_DATA \
	_IOWR(MC_ASOC_MAGIC, MC_ASOC_IOCTL_SET_DSP_DATA, \
		struct ymc_dspdata_args)

#define YMC_DSP_OUTPUT_BASE		(0x00000000)
#define YMC_DSP_OUTPUT_SP		(0x00000001)
#define YMC_DSP_OUTPUT_RC		(0x00000002)
#define YMC_DSP_OUTPUT_HP		(0x00000003)
#define YMC_DSP_OUTPUT_LO1		(0x00000004)
#define YMC_DSP_OUTPUT_LO2		(0x00000005)
#define YMC_DSP_OUTPUT_BT		(0x00000006)

#define YMC_DSP_INPUT_BASE		(0x00000010)
#define YMC_DSP_INPUT_MAINMIC		(0x00000020)
#define YMC_DSP_INPUT_SUBMIC		(0x00000030)
#define YMC_DSP_INPUT_2MIC		(0x00000040)
#define YMC_DSP_INPUT_HEADSET		(0x00000050)
#define YMC_DSP_INPUT_BT		(0x00000060)
#define YMC_DSP_INPUT_LINEIN1		(0x00000070)
#define YMC_DSP_INPUT_LINEIN2		(0x00000080)

#define YMC_DSP_VOICECALL_BASE_1MIC	(0x00000100)
#define YMC_DSP_VOICECALL_BASE_2MIC	(0x00000200)
#define YMC_DSP_VOICECALL_SP_1MIC	(0x00000300)
#define YMC_DSP_VOICECALL_SP_2MIC	(0x00000400)
#define YMC_DSP_VOICECALL_RC_1MIC	(0x00000500)
#define YMC_DSP_VOICECALL_RC_2MIC	(0x00000600)
#define YMC_DSP_VOICECALL_HP_1MIC	(0x00000700)
#define YMC_DSP_VOICECALL_HP_2MIC	(0x00000800)
#define YMC_DSP_VOICECALL_LO1_1MIC	(0x00000900)
#define YMC_DSP_VOICECALL_LO1_2MIC	(0x00000A00)
#define YMC_DSP_VOICECALL_LO2_1MIC	(0x00000B00)
#define YMC_DSP_VOICECALL_LO2_2MIC	(0x00000C00)
#define YMC_DSP_VOICECALL_HEADSET	(0x00000D00)
#define YMC_DSP_VOICECALL_BT		(0x00000E00)
#define YMC_DSP_VOICECALL_BASE_COMMON	(0x00000F00)

#define YMC_NOTITY_HOLD_OFF		(0)
#define YMC_NOTITY_HOLD_ON		(1)

#endif
