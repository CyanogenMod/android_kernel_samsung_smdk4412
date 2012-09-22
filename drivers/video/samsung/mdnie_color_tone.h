#ifndef __MDNIE_COLOR_TONE_H__
#define __MDNIE_COLOR_TONE_H__

#include "mdnie.h"

static const unsigned short tune_color_tone_1[] = {
	0x0000, 0x0000,   /*BANK 0*/
	0x0008, 0x0020,   /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,   /*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xaf00,   /*SCR RrCr*/
	0x00e2, 0x00b7,   /*SCR RgCg*/
	0x00e3, 0x00bc,   /*SCR RbCb*/
	0x00e4, 0x00af,   /*SCR GrMr*/
	0x00e5, 0xb700,   /*SCR GgMg*/
	0x00e6, 0x00bc,   /*SCR GbMb*/
	0x00e7, 0x00af,   /*SCR BrYr*/
	0x00e8, 0x00b7,   /*SCR BgYg*/
	0x00e9, 0xbc00,   /*SCR BbYb*/
	0x00ea, 0x00af,   /*SCR KrWr*/
	0x00eb, 0x00b7,   /*SCR KgWg*/
	0x00ec, 0x00bc,   /*SCR KbWb*/
	0x00ff, 0x0000,   /*Mask Release*/
	END_SEQ, 0x0000,

};

static const unsigned short tune_color_tone_2[] = {
	0x0000, 0x0000,   /*BANK 0*/
	0x0008, 0x0020,   /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,   /*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xa000,   /*SCR RrCr*/
	0x00e2, 0x00a8,   /*SCR RgCg*/
	0x00e3, 0x00b2,   /*SCR RbCb*/
	0x00e4, 0x00a0,   /*SCR GrMr*/
	0x00e5, 0xa800,   /*SCR GgMg*/
	0x00e6, 0x00b2,   /*SCR GbMb*/
	0x00e7, 0x00a0,   /*SCR BrYr*/
	0x00e8, 0x00a8,   /*SCR BgYg*/
	0x00e9, 0xb200,   /*SCR BbYb*/
	0x00ea, 0x00a0,   /*SCR KrWr*/
	0x00eb, 0x00a8,   /*SCR KgWg*/
	0x00ec, 0x00b2,   /*SCR KbWb*/
	0x00ff, 0x0000,   /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_color_tone_3[] = {
	0x0000, 0x0000,   /*BANK 0*/
	0x0008, 0x0020,   /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,   /*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0x9100,   /*SCR RrCr*/
	0x00e2, 0x0099,   /*SCR RgCg*/
	0x00e3, 0x00a3,   /*SCR RbCb*/
	0x00e4, 0x0091,   /*SCR GrMr*/
	0x00e5, 0x9900,   /*SCR GgMg*/
	0x00e6, 0x00a3,   /*SCR GbMb*/
	0x00e7, 0x0091,   /*SCR BrYr*/
	0x00e8, 0x0099,   /*SCR BgYg*/
	0x00e9, 0xa300,   /*SCR BbYb*/
	0x00ea, 0x0091,   /*SCR KrWr*/
	0x00eb, 0x0099,   /*SCR KgWg*/
	0x00ec, 0x00a3,   /*SCR KbWb*/
	0x00ff, 0x0000,   /*Mask Release*/
	END_SEQ, 0x0000,
};

#if !defined(CONFIG_FB_MDNIE_PWM)
static const unsigned short tune_negative[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*SCR*/
	0x00e1, 0x00ff,	/*SCR RrCr*/
	0x00e2, 0xff00,	/*SCR RgCg*/
	0x00e3, 0xff00,	/*SCR RbCb*/
	0x00e4, 0xff00,	/*SCR GrMr*/
	0x00e5, 0x00ff,	/*SCR GgMg*/
	0x00e6, 0xff00,	/*SCR GbMb*/
	0x00e7, 0xff00,	/*SCR BrYr*/
	0x00e8, 0xff00,	/*SCR BgYg*/
	0x00e9, 0x00ff,	/*SCR BbYb*/
	0x00ea, 0xff00,	/*SCR KrWr*/
	0x00eb, 0xff00,	/*SCR KgWg*/
	0x00ec, 0xff00,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};
#else
static const unsigned short tune_negative[] = {
	0x0000, 0x0000, /*BANK 0*/
	0x0008, 0x0020, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000, /*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0x00ff, /*SCR RrCr*/
	0x00e2, 0xff00, /*SCR RgCg*/
	0x00e3, 0xff00, /*SCR RbCb*/
	0x00e4, 0xff00, /*SCR GrMr*/
	0x00e5, 0x00ff, /*SCR GgMg*/
	0x00e6, 0xff00, /*SCR GbMb*/
	0x00e7, 0xff00, /*SCR BrYr*/
	0x00e8, 0xff00, /*SCR BgYg*/
	0x00e9, 0x00ff, /*SCR BbYb*/
	0x00ea, 0xff00, /*SCR KrWr*/
	0x00eb, 0xff00, /*SCR KgWg*/
	0x00ec, 0xff00, /*SCR KbWb*/
	0x00ff, 0x0000, /*Mask Release*/
	END_SEQ, 0x0000,
};
#endif

static const unsigned short tune_negative_cabc[] = {
	0x0000, 0x0000, /*BANK 0*/
	0x0008, 0x0220, /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000, /*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0x00ff, /*SCR RrCr*/
	0x00e2, 0xff00, /*SCR RgCg*/
	0x00e3, 0xff00, /*SCR RbCb*/
	0x00e4, 0xff00, /*SCR GrMr*/
	0x00e5, 0x00ff, /*SCR GgMg*/
	0x00e6, 0xff00, /*SCR GbMb*/
	0x00e7, 0xff00, /*SCR BrYr*/
	0x00e8, 0xff00, /*SCR BgYg*/
	0x00e9, 0x00ff, /*SCR BbYb*/
	0x00ea, 0xff00, /*SCR KrWrv*/
	0x00eb, 0xff00, /*SCR KgWg*/
	0x00ec, 0xff00, /*SCR KbWb*/
	0x0000, 0x0001, /*BANK 1*/
	0x0075, 0x0000, /*CABC dgain*/
	0x0076, 0x0000,
	0x0077, 0x0000,
	0x0078, 0x0000,
	0x007f, 0x0002, /*dynamic lcd*/
	0x00ff, 0x0000, /*Mask Release*/
	END_SEQ, 0x0000,
};

struct mdnie_tunning_info negative_table[CABC_MAX] = {
#if defined(CONFIG_FB_MDNIE_PWM)
	{"NEGATIVE",		tune_negative},
	{"NEGATIVE_CABC",	tune_negative_cabc},
#else
	{"NEGATIVE",		tune_negative},
#endif
};

struct mdnie_tunning_info color_tone_table[COLOR_TONE_MAX - COLOR_TONE_1] = {
	{"COLOR_TONE_1",	tune_color_tone_1},
	{"COLOR_TONE_2",	tune_color_tone_2},
	{"COLOR_TONE_3",	tune_color_tone_3},
};

#endif /* __MDNIE_COLOR_TONE_H__ */
