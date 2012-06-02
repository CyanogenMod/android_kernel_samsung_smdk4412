/*
 * MC-1N2 ASoC codec driver
 *
 * Copyright (c) 2010-2011 Yamaha Corporation
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

#ifndef MC1N2_H
#define MC1N2_H

#include "mcdriver.h"
#include <linux/mfd/mc1n2_pdata.h>

/*
 * dai: set_sysclk
 */
/* clk_id */
#define MC1N2_CLKI              0

/* default freq for MC1N2_CLKI */
#define MC1N2_DEFAULT_CLKI      19200000

/*
 * dai: set_clkdiv
 */
/* div_id */
#define MC1N2_CKSEL             0
#define MC1N2_DIVR0             1
#define MC1N2_DIVF0             2
#define MC1N2_DIVR1             3
#define MC1N2_DIVF1             4
#define MC1N2_BCLK_MULT         5

/* div for MC1N2_BCLK_MULT */
#define MC1N2_LRCK_X8           0
#define MC1N2_LRCK_X16          1
#define MC1N2_LRCK_X24          2
#define MC1N2_LRCK_X32          3
#define MC1N2_LRCK_X48          4
#define MC1N2_LRCK_X64          5
#define MC1N2_LRCK_X128         6
#define MC1N2_LRCK_X256         7
#define MC1N2_LRCK_X512         8

/*
 * hwdep: ioctl
 */
#define MC1N2_MAGIC             'N'
#define MC1N2_IOCTL_NR_GET      1
#define MC1N2_IOCTL_NR_SET      2
#define MC1N2_IOCTL_NR_BOTH     3
#define MC1N2_IOCTL_NR_NOTIFY   4

#define MC1N2_IOCTL_GET_CTRL \
	_IOR(MC1N2_MAGIC, MC1N2_IOCTL_NR_GET, struct mc1n2_ctrl_args)
#define MC1N2_IOCTL_SET_CTRL \
	_IOW(MC1N2_MAGIC, MC1N2_IOCTL_NR_SET, struct mc1n2_ctrl_args)

#define MC1N2_IOCTL_READ_REG \
	_IOWR(MC1N2_MAGIC, MC1N2_IOCTL_NR_BOTH, struct mc1n2_ctrl_args)

#define MC1N2_IOCTL_NOTIFY \
	_IOW(MC1N2_MAGIC, MC1N2_IOCTL_NR_NOTIFY, struct mc1n2_ctrl_args)

struct mc1n2_ctrl_args {
	unsigned long dCmd;
	void *pvPrm;
	unsigned long dPrm;
};

/*
 * MC1N2_IOCTL_NOTIFY dCmd definitions
 */
#define MCDRV_NOTIFY_CALL_START		0x00000000
#define MCDRV_NOTIFY_CALL_STOP		0x00000001
#define MCDRV_NOTIFY_MEDIA_PLAY_START	0x00000002
#define MCDRV_NOTIFY_MEDIA_PLAY_STOP	0x00000003
#define MCDRV_NOTIFY_FM_PLAY_START	0x00000004
#define MCDRV_NOTIFY_FM_PLAY_STOP	0x00000005
#define MCDRV_NOTIFY_BT_SCO_ENABLE	0x00000006
#define MCDRV_NOTIFY_BT_SCO_DISABLE	0x00000007
#define MCDRV_NOTIFY_VOICE_REC_START	0x00000008
#define MCDRV_NOTIFY_VOICE_REC_STOP	0x00000009
#define MCDRV_NOTIFY_HDMI_START		0x0000000A
#define MCDRV_NOTIFY_HDMI_STOP		0x0000000B
#define	MCDRV_NOTIFY_RECOVER		0x0000000C
#define MCDRV_NOTIFY_2MIC_CALL_START	0x0000000D

#define MC1N2_MODE_IDLE			(0x00)
#define MC1N2_MODE_CALL_ON		(0x1<<0)
#define MC1N2_MODE_FM_ON		(0x1<<1)

/*
 * Setup parameters
 */
struct mc1n2_setup {
	MCDRV_INIT_INFO init;
	unsigned char pcm_extend[IOPORT_NUM];
	unsigned char pcm_hiz_redge[IOPORT_NUM];
	unsigned char pcm_hperiod[IOPORT_NUM];
	unsigned char slot[IOPORT_NUM][SNDRV_PCM_STREAM_LAST+1][DIO_CHANNELS];
};

/*
 * Codec Status definitions (for backward compatibility)
 */
#define CMD_CODEC_EMERGENCY_RECOVERY 9 // Emergency recovery for Error like -EIO, -ESTRPIPE, and etc.

#endif
