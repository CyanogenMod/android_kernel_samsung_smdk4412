#ifndef __MDNIE_TABLE_H__
#define __MDNIE_TABLE_H__

#include "mdnie.h"


static const unsigned char power_lut[2][9] = {
	{0x42, 0x47, 0x3E, 0x52, 0x42, 0x3F, 0x3A, 0x37, 0x3F},
	{0x38, 0x3d, 0x34, 0x48, 0x38, 0x35, 0x30, 0x2d, 0x35},
};

static const unsigned short tune_std_ui_cabcOff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0000,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 | de8 hdr2 fa1*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_camera[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0030,	/*DE pf*/
	0x0094,  0x0030,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0030,	/*DE nf*/
	0x0097,  0x0030,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_camera_outdoor[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x04ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0030,	/*DE pf*/
	0x0094,  0x0030,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0030,	/*DE nf*/
	0x0097,  0x0030,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00d0,  0x01a0,	/*UC y*/
	0x00d1,  0x01ff,	/*UC cs*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_ui_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00a8,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0894,	/*CC lut r	16 144*/
	0x0022,  0x18a6,	/*CC lut r	32 160*/
	0x0023,  0x28b8,	/*CC lut r	48 176*/
	0x0024,  0x3ac9,	/*CC lut r	64 192*/
	0x0025,  0x4cd9,	/*CC lut r	80 208*/
	0x0026,  0x5ee7,	/*CC lut r	96 224*/
	0x0027,  0x70f4,	/*CC lut r 112 240*/
	0x0028,  0x82ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_video_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0080,	/*DE pf*/
	0x0094,  0x0080,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0080,	/*DE nf*/
	0x0097,  0x0080,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0894,	/*CC lut r	16 144*/
	0x0022,  0x18a6,	/*CC lut r	32 160*/
	0x0023,  0x28b8,	/*CC lut r	48 176*/
	0x0024,  0x3ac9,	/*CC lut r	64 192*/
	0x0025,  0x4cd9,	/*CC lut r	80 208*/
	0x0026,  0x5ee7,	/*CC lut r	96 224*/
	0x0027,  0x70f4,	/*CC lut r 112 240*/
	0x0028,  0x82ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_gallery_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0050,	/*DE pf*/
	0x0094,  0x0050,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0050,	/*DE nf*/
	0x0097,  0x0050,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0894,	/*CC lut r	16 144*/
	0x0022,  0x18a6,	/*CC lut r	32 160*/
	0x0023,  0x28b8,	/*CC lut r	48 176*/
	0x0024,  0x3ac9,	/*CC lut r	64 192*/
	0x0025,  0x4cd9,	/*CC lut r	80 208*/
	0x0026,  0x5ee7,	/*CC lut r	96 224*/
	0x0027,  0x70f4,	/*CC lut r 112 240*/
	0x0028,  0x82ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_vt_cabcOff[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0088,	 /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x0a88,  /*CC lut r	16 144*/
	0x0022, 0x1499,  /*CC lut r	32 160*/
	0x0023, 0x1eaa,  /*CC lut r	48 176*/
	0x0024, 0x30bb,  /*CC lut r	64 192*/
	0x0025, 0x42cc,  /*CC lut r	80 208*/
	0x0026, 0x54dd,  /*CC lut r	96 224*/
	0x0027, 0x66ee,  /*CC lut r 112 240*/
	0x0028, 0x77ff,  /*CC lut r 128 255*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_ui_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00a8,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_video_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0060,	/*DE pf*/
	0x0094,  0x0060,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0060,	/*DE nf*/
	0x0097,  0x0060,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_gallery_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0030,	/*DE pf*/
	0x0094,  0x0030,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0030,	/*DE nf*/
	0x0097,  0x0030,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_vt_cabcOff[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0088,	 /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x0a88,  /*CC lut r	16 144*/
	0x0022, 0x1499,  /*CC lut r	32 160*/
	0x0023, 0x1eaa,  /*CC lut r	48 176*/
	0x0024, 0x30bb,  /*CC lut r	64 192*/
	0x0025, 0x42cc,  /*CC lut r	80 208*/
	0x0026, 0x54dd,  /*CC lut r	96 224*/
	0x0027, 0x66ee,  /*CC lut r 112 240*/
	0x0028, 0x77ff,  /*CC lut r 128 255*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_ui_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_video_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092,  0x0000,	/*DE pe*/
	0x0093,  0x0000,	/*DE pf*/
	0x0094,  0x0000,	/*DE pb*/
	0x0095,  0x0000,	/*DE ne*/
	0x0096,  0x0000,	/*DE nf*/
	0x0097,  0x0000,	/*DE nb*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1004,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_gallery_cabcOff[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x00a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0890,	/*CC lut r	16 144*/
	0x0022,  0x18a0,	/*CC lut r	32 160*/
	0x0023,  0x30b0,	/*CC lut r	48 176*/
	0x0024,  0x40c0,	/*CC lut r	64 192*/
	0x0025,  0x50d0,	/*CC lut r	80 208*/
	0x0026,  0x60e0,	/*CC lut r	96 224*/
	0x0027,  0x70f0,	/*CC lut r 112 240*/
	0x0028,  0x80ff,	/*CC lut r 128 255*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_vt_cabcOff[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0088,	 /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x0a88,  /*CC lut r	16 144*/
	0x0022, 0x1499,  /*CC lut r	32 160*/
	0x0023, 0x1eaa,  /*CC lut r	48 176*/
	0x0024, 0x30bb,  /*CC lut r	64 192*/
	0x0025, 0x42cc,  /*CC lut r	80 208*/
	0x0026, 0x54dd,  /*CC lut r	96 224*/
	0x0027, 0x66ee,  /*CC lut r 112 240*/
	0x0028, 0x77ff,  /*CC lut r 128 255*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_ui_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02a8,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c98,	/*CC lut r	16 144*/
	0x0022,  0x1caa,	/*CC lut r	32 160*/
	0x0023,  0x2cbc,	/*CC lut r	48 176*/
	0x0024,  0x3ecd,	/*CC lut r	64 192*/
	0x0025,  0x50dc,	/*CC lut r	80 208*/
	0x0026,  0x62e9,	/*CC lut r	96 224*/
	0x0027,  0x74f5,	/*CC lut r 112 240*/
	0x0028,  0x86ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_video_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0080,	/*DE pf*/
	0x0094,  0x0080,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0080,	/*DE nf*/
	0x0097,  0x0080,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c98,	/*CC lut r	16 144*/
	0x0022,  0x1caa,	/*CC lut r	32 160*/
	0x0023,  0x2cbc,	/*CC lut r	48 176*/
	0x0024,  0x3ecd,	/*CC lut r	64 192*/
	0x0025,  0x50dc,	/*CC lut r	80 208*/
	0x0026,  0x62e9,	/*CC lut r	96 224*/
	0x0027,  0x74f5,	/*CC lut r 112 240*/
	0x0028,  0x86ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_gallery_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x028c,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0050,	/*DE pf*/
	0x0094,  0x0050,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0050,	/*DE nf*/
	0x0097,  0x0050,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1a04,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c98,	/*CC lut r	16 144*/
	0x0022,  0x1caa,	/*CC lut r	32 160*/
	0x0023,  0x2cbc,	/*CC lut r	48 176*/
	0x0024,  0x3ecd,	/*CC lut r	64 192*/
	0x0025,  0x50dc,	/*CC lut r	80 208*/
	0x0026,  0x62e9,	/*CC lut r	96 224*/
	0x0027,  0x74f5,	/*CC lut r 112 240*/
	0x0028,  0x86ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_vt_cabcOn[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0288,  /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x0090, 0x0080,  /*DE egth*/
	0x0092, 0x0030,  /*DE pe*/
	0x0093, 0x0080,  /*DE pf*/
	0x0094, 0x0080,  /*DE pb*/
	0x0095, 0x0030,  /*DE ne*/
	0x0096, 0x0080,  /*DE nf*/
	0x0097, 0x0080,  /*DE nb*/
	0x0098, 0x1000,  /*DE max ratio*/
	0x0099, 0x0100,  /*DE min ratio*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x1090,  /*CC lut r	16 144*/
	0x0022, 0x20a0,  /*CC lut r	32 160*/
	0x0023, 0x30b0,  /*CC lut r	48 176*/
	0x0024, 0x40c0,  /*CC lut r	64 192*/
	0x0025, 0x50d0,  /*CC lut r	80 208*/
	0x0026, 0x60e0,  /*CC lut r	96 224*/
	0x0027, 0x70f0,  /*CC lut r 112 240*/
	0x0028, 0x80ff,  /*CC lut r 128 255*/
	0x0075, 0x0000,  /*CABC dgain*/
	0x0076, 0x0000,
	0x0077, 0x0000,
	0x0078, 0x0000,
	0x007f, 0x0002,  /*dynamic lcd*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_ui_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02a8,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_video_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0060,	/*DE pf*/
	0x0094,  0x0060,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0060,	/*DE nf*/
	0x0097,  0x0060,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_gallery_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0090,  0x0080,	/*DE egth*/
	0x0092,  0x0030,	/*DE pe*/
	0x0093,  0x0030,	/*DE pf*/
	0x0094,  0x0030,	/*DE pb*/
	0x0095,  0x0030,	/*DE ne*/
	0x0096,  0x0030,	/*DE nf*/
	0x0097,  0x0030,	/*DE nb*/
	0x0098,  0x1000,	/*DE max ratio*/
	0x0099,  0x0100,	/*DE min ratio*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1804,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00ff,	/*SCR KgWg*/
	0x00ec,  0x00f8,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_vt_cabcOn[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0288,  /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x0090, 0x0080,  /*DE egth*/
	0x0092, 0x0030,  /*DE pe*/
	0x0093, 0x0080,  /*DE pf*/
	0x0094, 0x0080,  /*DE pb*/
	0x0095, 0x0030,  /*DE ne*/
	0x0096, 0x0080,  /*DE nf*/
	0x0097, 0x0080,  /*DE nb*/
	0x0098, 0x1000,  /*DE max ratio*/
	0x0099, 0x0100,  /*DE min ratio*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x1090,  /*CC lut r	16 144*/
	0x0022, 0x20a0,  /*CC lut r	32 160*/
	0x0023, 0x30b0,  /*CC lut r	48 176*/
	0x0024, 0x40c0,  /*CC lut r	64 192*/
	0x0025, 0x50d0,  /*CC lut r	80 208*/
	0x0026, 0x60e0,  /*CC lut r	96 224*/
	0x0027, 0x70f0,  /*CC lut r 112 240*/
	0x0028, 0x80ff,  /*CC lut r 128 255*/
	0x0075, 0x0000,  /*CABC dgain*/
	0x0076, 0x0000,
	0x0077, 0x0000,
	0x0078, 0x0000,
	0x007f, 0x0002,  /*dynamic lcd*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_ui_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_video_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092,  0x0000,	/*DE pe*/
	0x0093,  0x0000,	/*DE pf*/
	0x0094,  0x0000,	/*DE pb*/
	0x0095,  0x0000,	/*DE ne*/
	0x0096,  0x0000,	/*DE nf*/
	0x0097,  0x0000,	/*DE nb*/
	0x00b0,  0x1010,	/*CS hg ry*/
	0x00b1,  0x1010,	/*CS hg gc*/
	0x00b2,  0x1010,	/*CS hg bm*/
	0x00b3,  0x1004,	/*CS weight grayTH*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_gallery_cabcOn[] = {
	0x0000,  0x0000,	/*BANK 0*/
	0x0008,  0x02a0,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030,  0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1,  0xff00,	/*SCR RrCr*/
	0x00e2,  0x00ff,	/*SCR RgCg*/
	0x00e3,  0x00ff,	/*SCR RbCb*/
	0x00e4,  0x00ff,	/*SCR GrMr*/
	0x00e5,  0xff00,	/*SCR GgMg*/
	0x00e6,  0x00ff,	/*SCR GbMb*/
	0x00e7,  0x00ff,	/*SCR BrYr*/
	0x00e8,  0x00ff,	/*SCR BgYg*/
	0x00e9,  0xff00,	/*SCR BbYb*/
	0x00ea,  0x00ff,	/*SCR KrWr*/
	0x00eb,  0x00fb,	/*SCR KgWg*/
	0x00ec,  0x00ec,	/*SCR KbWb*/
	0x0000,  0x0001,	/*BANK 1*/
	0x001f,  0x0080,	/*CC chsel strength*/
	0x0020,  0x0000,	/*CC lut r	 0*/
	0x0021,  0x0c96,	/*CC lut r	16 144*/
	0x0022,  0x1ca7,	/*CC lut r	32 160*/
	0x0023,  0x34b8,	/*CC lut r	48 176*/
	0x0024,  0x44c9,	/*CC lut r	64 192*/
	0x0025,  0x54d9,	/*CC lut r	80 208*/
	0x0026,  0x64e7,	/*CC lut r	96 224*/
	0x0027,  0x74f4,	/*CC lut r 112 240*/
	0x0028,  0x85ff,	/*CC lut r 128 255*/
	0x0075,  0x0000,	/*CABC dgain*/
	0x0076,  0x0000,
	0x0077,  0x0000,
	0x0078,  0x0000,
	0x007f,  0x0002,	/*dynamic lcd*/
	0x00ff,  0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_vt_cabcOn[] = {
	0x0000, 0x0000,  /*BANK 0*/
	0x0008, 0x0288,  /*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,  /*FA cs1 de8 hdr2 fa1*/
	0x0090, 0x0080,  /*DE egth*/
	0x0092, 0x0030,  /*DE pe*/
	0x0093, 0x0080,  /*DE pf*/
	0x0094, 0x0080,  /*DE pb*/
	0x0095, 0x0030,  /*DE ne*/
	0x0096, 0x0080,  /*DE nf*/
	0x0097, 0x0080,  /*DE nb*/
	0x0098, 0x1000,  /*DE max ratio*/
	0x0099, 0x0100,  /*DE min ratio*/
	0x00b0, 0x1010,  /*CS hg ry*/
	0x00b1, 0x1010,  /*CS hg gc*/
	0x00b2, 0x1010,  /*CS hg bm*/
	0x00b3, 0x1c04,  /*CS weight grayTH*/
	0x0000, 0x0001,  /*BANK 1*/
	0x001f, 0x0080,  /*CC chsel strength*/
	0x0020, 0x0000,  /*CC lut r	 0*/
	0x0021, 0x1090,  /*CC lut r	16 144*/
	0x0022, 0x20a0,  /*CC lut r	32 160*/
	0x0023, 0x30b0,  /*CC lut r	48 176*/
	0x0024, 0x40c0,  /*CC lut r	64 192*/
	0x0025, 0x50d0,  /*CC lut r	80 208*/
	0x0026, 0x60e0,  /*CC lut r	96 224*/
	0x0027, 0x70f0,  /*CC lut r 112 240*/
	0x0028, 0x80ff,  /*CC lut r 128 255*/
	0x0075, 0x0000,  /*CABC dgain*/
	0x0076, 0x0000,
	0x0077, 0x0000,
	0x0078, 0x0000,
	0x007f, 0x0002,  /*dynamic lcd*/
	0x00ff, 0x0000,  /*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_warm_cabcoff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x00ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7575,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold_cabcoff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x00ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x000b, 0x7979,	/*MCM 4cr 5cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_outdoor_cabcoff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x04ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x00d0, 0x01a0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_warm_outdoor_cabcoff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x04ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7575,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold_outdoor_cabcoff[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x00ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x000b, 0x7979,	/*MCM 4cr 5cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};


static const unsigned short tune_warm_cabcon[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x02ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7575,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold_cabcon[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x02ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x000b, 0x7979,	/*MCM 4cr 5cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_outdoor_cabcon[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x06ac,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x00d0, 0x01a0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_warm_outdoor_cabcon[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x06ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7575,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold_outdoor_cabcon[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x06ec,	/*Dither8 UC4 ABC2 CP1 | CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08e,	/*MCM 5cb 1cr W*/
	0x000b, 0x7979,	/*MCM 4cr 5cr W*/
	0x00d0, 0x01a0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
	END_SEQ, 0x0000,
};


struct mdnie_tunning_info etc_table[CABC_MAX][OUTDOOR_MAX][TONE_MAX] = {
	{
		{
			{"NORMAL",		NULL},
			{"WARM",		tune_warm_cabcoff},
			{"COLD",		tune_cold_cabcoff},
		},
		{
			{"NORMAL_OUTDOOR",	tune_outdoor_cabcoff},
			{"WARM_OUTDOOR",	tune_warm_outdoor_cabcoff},
			{"COLD_OUTDOOR",	tune_cold_outdoor_cabcoff},
		},
	},
	{
		{
			{"NORMAL_CABC",		NULL},
			{"WARM_CABC",		tune_warm_cabcon},
			{"COLD_CABC",		tune_cold_cabcon},
		},
		{
			{"NORMAL_OUTDOOR_CABC",	tune_outdoor_cabcon},
			{"WARM_OUTDOOR_CABC",	tune_warm_outdoor_cabcon},
			{"COLD_OUTDOOR_CABC",	tune_cold_outdoor_cabcon},
		},
	},
};


struct mdnie_tunning_info_cabc tunning_table[CABC_MAX][MODE_MAX][SCENARIO_MAX] = {
{
	{
		{"DYNAMIC_UI",		tune_dynamic_ui_cabcOff, 0},
		{"DYNAMIC_VIDEO_NOR",	tune_dynamic_video_cabcOff, LUT_VIDEO},
		{"DYNAMIC_VIDEO_WARM",	tune_dynamic_video_cabcOff, LUT_VIDEO},
		{"DYNAMIC_VIDEO_COLD",	tune_dynamic_video_cabcOff, LUT_VIDEO},
		{"CAMERA",		tune_camera, 0},
		{"DYNAMIC_UI",		tune_dynamic_ui_cabcOff, 0},
		{"DYNAMIC_GALLERY",	tune_dynamic_gallery_cabcOff, 0},
		{"DYNAMIC_VT",		tune_dynamic_vt_cabcOff, 0},
	}, {
		{"STANDARD_UI",		tune_standard_ui_cabcOff, 0},
		{"STANDARD_VIDEO_NOR",	tune_standard_video_cabcOff, LUT_VIDEO},
		{"STANDARD_VIDEO_WARM",	tune_standard_video_cabcOff, LUT_VIDEO},
		{"STANDARD_VIDEO_COLD",	tune_standard_video_cabcOff, LUT_VIDEO},
		{"CAMERA",		tune_camera, 0},
		{"STANDARD_UI",		tune_standard_ui_cabcOff, 0},
		{"STANDARD_GALLERY",	tune_standard_gallery_cabcOff, 0},
		{"STANDARD_VT",		tune_standard_vt_cabcOff, 0},
	}, {
		{"MOVIE_UI",		tune_movie_ui_cabcOff, 0},
		{"MOVIE_VIDEO_NOR",	tune_movie_video_cabcOff, LUT_VIDEO},
		{"MOVIE_VIDEO_WARM",	tune_movie_video_cabcOff, LUT_VIDEO},
		{"MOVIE_VIDEO_COLD",	tune_movie_video_cabcOff, LUT_VIDEO},
		{"CAMERA",		tune_camera, 0},
		{"MOVIE_UI",		tune_movie_ui_cabcOff, 0},
		{"MOVIE_GALLERY",	tune_movie_gallery_cabcOff, 0},
		{"MOVIE_VT",		tune_movie_vt_cabcOff, 0},
	},
},
{
	{
		{"DYNAMIC_UI_CABC",		tune_dynamic_ui_cabcOn, 0},
		{"DYNAMIC_VIDEO_NOR_CABC",	tune_dynamic_video_cabcOn, LUT_VIDEO},
		{"DYNAMIC_VIDEO_WARM_CABC",	tune_dynamic_video_cabcOn, LUT_VIDEO},
		{"DYNAMIC_VIDEO_COLD",		tune_dynamic_video_cabcOn, LUT_VIDEO},
		{"CAMERA",			tune_camera, 0},
		{"DYNAMIC_UI_CABC",		tune_dynamic_ui_cabcOn, 0},
		{"DYNAMIC_GALLERY_CABC",	tune_dynamic_gallery_cabcOn, 0},
		{"DYNAMIC_VT_CABC",		tune_dynamic_vt_cabcOn, 0},
	}, {
		{"STANDARD_UI_CABC",		tune_standard_ui_cabcOn, 0},
		{"STANDARD_VIDEO_NOR_CABC",	tune_standard_video_cabcOn, LUT_VIDEO},
		{"STANDARD_VIDEO_WARM_CABC",	tune_standard_video_cabcOn, LUT_VIDEO},
		{"STANDARD_VIDEO_COLD_CABC",	tune_standard_video_cabcOn, LUT_VIDEO},
		{"CAMERA",			tune_camera, 0},
		{"STANDARD_UI_CABC",		tune_standard_ui_cabcOn, 0},
		{"STANDARD_GALLERY_CABC",	tune_standard_gallery_cabcOn, 0},
		{"STANDARD_VT_CABC",		tune_standard_vt_cabcOn, 0},
	}, {
		{"MOVIE_UI_CABC",		tune_movie_ui_cabcOn, 0},
		{"MOVIE_VIDEO_NOR_CABC",	tune_movie_video_cabcOn, LUT_VIDEO},
		{"MOVIE_VIDEO_WARM_CABC",	tune_movie_video_cabcOn, LUT_VIDEO},
		{"MOVIE_VIDEO_COLD_CABC",	tune_movie_video_cabcOn, LUT_VIDEO},
		{"CAMERA",			tune_camera, 0},
		{"MOVIE_UI_CABC",		tune_movie_ui_cabcOn, 0},
		{"MOVIE_GALLERY_CABC",		tune_movie_gallery_cabcOn, 0},
		{"MOVIE_VT_CABC",		tune_movie_vt_cabcOn, 0},
	},
},
};

#endif /* __MDNIE_TABLE_H__ */
