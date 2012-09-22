/*
 * CMC623 Setting table for SLP7 Machine.
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 *		Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mdnie.h"

static const unsigned short dynamic_ui[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x008c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0040,	/*DE pe*/
	0x0093, 0x0040,	/*DE pf*/
	0x0094, 0x0040,	/*DE pb*/
	0x0095, 0x0040,	/*DE ne*/
	0x0096, 0x0040,	/*DE nf*/
	0x0097, 0x0040,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0080,	/*CC chsel strength*/
	0x0020, 0x0000,	/*CC lut r   0*/
	0x0021, 0x0d93,	/*CC lut r  16 144*/
	0x0022, 0x1aa5,	/*CC lut r  32 160*/
	0x0023, 0x29b7,	/*CC lut r  48 176*/
	0x0024, 0x39c8,	/*CC lut r  64 192*/
	0x0025, 0x4bd8,	/*CC lut r  80 208*/
	0x0026, 0x5de6,	/*CC lut r  96 224*/
	0x0027, 0x6ff4,	/*CC lut r 112 240*/
	0x0028, 0x81ff,	/*CC lut r 128 255*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short dynamic_gallery[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x008c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0080,	/*DE pe*/
	0x0093, 0x0080,	/*DE pf*/
	0x0094, 0x0080,	/*DE pb*/
	0x0095, 0x0080,	/*DE ne*/
	0x0096, 0x0080,	/*DE nf*/
	0x0097, 0x0080,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1404,	/*CS weight grayTH*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0080,	/*CC chsel strength*/
	0x0020, 0x0000,	/*CC lut r   0*/
	0x0021, 0x0d93,	/*CC lut r  16 144*/
	0x0022, 0x1aa5,	/*CC lut r  32 160*/
	0x0023, 0x29b7,	/*CC lut r  48 176*/
	0x0024, 0x39c8,	/*CC lut r  64 192*/
	0x0025, 0x4bd8,	/*CC lut r  80 208*/
	0x0026, 0x5de6,	/*CC lut r  96 224*/
	0x0027, 0x6ff4,	/*CC lut r 112 240*/
	0x0028, 0x81ff,	/*CC lut r 128 255*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short dynamic_video[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x008c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0080,	/*DE pe*/
	0x0093, 0x0080,	/*DE pf*/
	0x0094, 0x0080,	/*DE pb*/
	0x0095, 0x0080,	/*DE ne*/
	0x0096, 0x0080,	/*DE nf*/
	0x0097, 0x0080,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1404,	/*CS weight grayTH*/
	0x00e1, 0xff00,	/*SCR RrCr*/
	0x00e2, 0x00ff,	/*SCR RgCg*/
	0x00e3, 0x00ff,	/*SCR RbCb*/
	0x00e4, 0x00ff,	/*SCR GrMr*/
	0x00e5, 0xff00,	/*SCR GgMg*/
	0x00e6, 0x00ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00ff,	/*SCR BgYg*/
	0x00e9, 0xff00,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00ff,	/*SCR KgWg*/
	0x00ec, 0x00ff,	/*SCR KbWb*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0080,	/*CC chsel strength*/
	0x0020, 0x0000,	/*CC lut r   0*/
	0x0021, 0x0d93,	/*CC lut r  16 144*/
	0x0022, 0x1aa5,	/*CC lut r  32 160*/
	0x0023, 0x29b7,	/*CC lut r  48 176*/
	0x0024, 0x39c8,	/*CC lut r  64 192*/
	0x0025, 0x4bd8,	/*CC lut r  80 208*/
	0x0026, 0x5de6,	/*CC lut r  96 224*/
	0x0027, 0x6ff4,	/*CC lut r 112 240*/
	0x0028, 0x81ff,	/*CC lut r 128 255*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short dynamic_vtcall[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x008e,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0005,	/*FA cs1 | de8 dnr4 hdr2 fa1*/
	0x0039, 0x0080,	/*FA dnrWeight*/
	0x0080, 0x0fff,	/*DNR dirTh*/
	0x0081, 0x19ff,	/*DNR dirnumTh decon7Th*/
	0x0082, 0xff16,	/*DNR decon5Th maskTh*/
	0x0083, 0x0000,	/*DNR blTh*/
	0x0092, 0x00e0,	/*DE pe*/
	0x0093, 0x00e0,	/*DE pf*/
	0x0094, 0x00e0,	/*DE pb*/
	0x0095, 0x00e0,	/*DE ne*/
	0x0096, 0x00e0,	/*DE nf*/
	0x0097, 0x00e0,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0010,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1a04,	/*CS weight grayTH*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0080,	/*CC chsel strength*/
	0x0020, 0x0000,	/*CC lut r  0*/
	0x0021, 0x0d93,	/*CC lut r  16 144*/
	0x0022, 0x1aa5,	/*CC lut r  32 160*/
	0x0023, 0x29b7,	/*CC lut r  48 176*/
	0x0024, 0x39c8,	/*CC lut r  64 192*/
	0x0025, 0x4bd8,	/*CC lut r  80 208*/
	0x0026, 0x5de6,	/*CC lut r  96 224*/
	0x0027, 0x6ff4,	/*CC lut r 112 240*/
	0x0028, 0x81ff,	/*CC lut r 128 255*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short movie_ui[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00f8,	/*SCR KgWg*/
	0x00ec, 0x00f1,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short movie_gallery[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00f8,	/*SCR KgWg*/
	0x00ec, 0x00f1,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short movie_video[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0000,	/*DE pe*/
	0x0093, 0x0000,	/*DE pf*/
	0x0094, 0x0000,	/*DE pb*/
	0x0095, 0x0000,	/*DE ne*/
	0x0096, 0x0000,	/*DE nf*/
	0x0097, 0x0000,	/*DE nb*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1004,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00f8,	/*SCR KgWg*/
	0x00ec, 0x00f1,	/*SCR KbWb*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0000,	/*CC chsel strength*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short movie_vtcall[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x002e,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0005,	/*FA cs1 | de8 dnr4 hdr2 fa1*/
	0x0039, 0x0080,	/*FA dnrWeight*/
	0x0080, 0x0fff,	/*DNR dirTh*/
	0x0081, 0x19ff,	/*DNR dirnumTh decon7Th*/
	0x0082, 0xff16,	/*DNR decon5Th maskTh*/
	0x0083, 0x0000,	/*DNR blTh*/
	0x0092, 0x0040,	/*DE pe*/
	0x0093, 0x0040,	/*DE pf*/
	0x0094, 0x0040,	/*DE pb*/
	0x0095, 0x0040,	/*DE ne*/
	0x0096, 0x0040,	/*DE nf*/
	0x0097, 0x0040,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0010,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1204,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00f8,	/*SCR KgWg*/
	0x00ec, 0x00f1,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short natural_ui[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x002c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0020,	/*DE pe*/
	0x0093, 0x0020,	/*DE pf*/
	0x0094, 0x0020,	/*DE pb*/
	0x0095, 0x0020,	/*DE ne*/
	0x0096, 0x0020,	/*DE nf*/
	0x0097, 0x0020,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00fa,	/*SCR KgWg*/
	0x00ec, 0x00f8,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short natural_gallery[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x002c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00fa,	/*SCR KgWg*/
	0x00ec, 0x00f8,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short natural_video[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x002c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00fa,	/*SCR KgWg*/
	0x00ec, 0x00f8,	/*SCR KbWb*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0000,	/*CC chsel strength*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short natural_vtcall[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x002e,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0005,	/*FA cs1 | de8 dnr4 hdr2 fa1*/
	0x0039, 0x0080,	/*FA dnrWeight*/
	0x0080, 0x0fff,	/*DNR dirTh*/
	0x0081, 0x19ff,	/*DNR dirnumTh decon7Th*/
	0x0082, 0xff16,	/*DNR decon5Th maskTh*/
	0x0083, 0x0000,	/*DNR blTh*/
	0x0092, 0x00c0,	/*DE pe*/
	0x0093, 0x00c0,	/*DE pf*/
	0x0094, 0x00c0,	/*DE pb*/
	0x0095, 0x00c0,	/*DE ne*/
	0x0096, 0x00c0,	/*DE nf*/
	0x0097, 0x00c0,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0010,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x00e1, 0xd6ac,	/*SCR RrCr*/
	0x00e2, 0x32ff,	/*SCR RgCg*/
	0x00e3, 0x2ef0,	/*SCR RbCb*/
	0x00e4, 0xa5fa,	/*SCR GrMr*/
	0x00e5, 0xff4d,	/*SCR GgMg*/
	0x00e6, 0x59ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00fb,	/*SCR BgYg*/
	0x00e9, 0xff61,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00fa,	/*SCR KgWg*/
	0x00ec, 0x00f8,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short standard_ui[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x000c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0020,	/*DE pe*/
	0x0093, 0x0020,	/*DE pf*/
	0x0094, 0x0020,	/*DE pb*/
	0x0095, 0x0020,	/*DE ne*/
	0x0096, 0x0020,	/*DE nf*/
	0x0097, 0x0020,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1604,	/*CS weight grayTH*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short standard_gallery[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x000c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1204,	/*CS weight grayTH*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short standard_video[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x000c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1204,	/*CS weight grayTH*/
	0x00e1, 0xff00,	/*SCR RrCr*/
	0x00e2, 0x00ff,	/*SCR RgCg*/
	0x00e3, 0x00ff,	/*SCR RbCb*/
	0x00e4, 0x00ff,	/*SCR GrMr*/
	0x00e5, 0xff00,	/*SCR GgMg*/
	0x00e6, 0x00ff,	/*SCR GbMb*/
	0x00e7, 0x00ff,	/*SCR BrYr*/
	0x00e8, 0x00ff,	/*SCR BgYg*/
	0x00e9, 0xff00,	/*SCR BbYb*/
	0x00ea, 0x00ff,	/*SCR KrWr*/
	0x00eb, 0x00ff,	/*SCR KgWg*/
	0x00ec, 0x00ff,	/*SCR KbWb*/
	0x0000, 0x0001,	/*BANK 1*/
	0x001f, 0x0000,	/*CC chsel strength*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short standard_vtcall[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x000e,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0005,	/*FA cs1 | de8 dnr4 hdr2 fa1*/
	0x0039, 0x0080,	/*FA dnrWeight*/
	0x0080, 0x0fff,	/*DNR dirTh*/
	0x0081, 0x19ff,	/*DNR dirnumTh decon7Th*/
	0x0082, 0xff16,	/*DNR decon5Th maskTh*/
	0x0083, 0x0000,	/*DNR blTh*/
	0x0092, 0x00c0,	/*DE pe*/
	0x0093, 0x00c0,	/*DE pf*/
	0x0094, 0x00c0,	/*DE pb*/
	0x0095, 0x00c0,	/*DE ne*/
	0x0096, 0x00c0,	/*DE nf*/
	0x0097, 0x00c0,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0010,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1804,	/*CS weight grayTH*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short camera[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x000c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg ry*/
	0x00b1, 0x1010,	/*CS hg gc*/
	0x00b2, 0x1010,	/*CS hg bm*/
	0x00b3, 0x1204,	/*CS weight grayTH*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short camera_outdoor[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x040c,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x0092, 0x0060,	/*DE pe*/
	0x0093, 0x0060,	/*DE pf*/
	0x0094, 0x0060,	/*DE pb*/
	0x0095, 0x0060,	/*DE ne*/
	0x0096, 0x0060,	/*DE nf*/
	0x0097, 0x0060,	/*DE nb*/
	0x0098, 0x1000,	/*DE max ratio*/
	0x0099, 0x0100,	/*DE min ratio*/
	0x00b0, 0x1010,	/*CS hg RY*/
	0x00b1, 0x1010,	/*CS hg GC*/
	0x00b2, 0x1010,	/*CS hg BM*/
	0x00b3, 0x1204,	/*CS weight grayTH*/
	0x0000, 0x0001,	/*BANK 1*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short browser_tone1[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xaf00,	/*SCR RrCr*/
	0x00e2, 0x00b7,	/*SCR RgCg*/
	0x00e3, 0x00bc,	/*SCR RbCb*/
	0x00e4, 0x00af,	/*SCR GrMr*/
	0x00e5, 0xb700,	/*SCR GgMg*/
	0x00e6, 0x00bc,	/*SCR GbMb*/
	0x00e7, 0x00af,	/*SCR BrYr*/
	0x00e8, 0x00b7,	/*SCR BgYg*/
	0x00e9, 0xbc00,	/*SCR BbYb*/
	0x00ea, 0x00af,	/*SCR KrWr*/
	0x00eb, 0x00b7,	/*SCR KgWg*/
	0x00ec, 0x00bc,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short browser_tone2[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0xa000,	/*SCR RrCr*/
	0x00e2, 0x00a8,	/*SCR RgCg*/
	0x00e3, 0x00b2,	/*SCR RbCb*/
	0x00e4, 0x00a0,	/*SCR GrMr*/
	0x00e5, 0xa800,	/*SCR GgMg*/
	0x00e6, 0x00b2,	/*SCR GbMb*/
	0x00e7, 0x00a0,	/*SCR BrYr*/
	0x00e8, 0x00a8,	/*SCR BgYg*/
	0x00e9, 0xb200,	/*SCR BbYb*/
	0x00ea, 0x00a0,	/*SCR KrWr*/
	0x00eb, 0x00a8,	/*SCR KgWg*/
	0x00ec, 0x00b2,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short browser_tone3[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0020,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 de8 hdr2 fa1*/
	0x00e1, 0x9100,	/*SCR RrCr*/
	0x00e2, 0x0099,	/*SCR RgCg*/
	0x00e3, 0x00a3,	/*SCR RbCb*/
	0x00e4, 0x0091,	/*SCR GrMr*/
	0x00e5, 0x9900,	/*SCR GgMg*/
	0x00e6, 0x00a3,	/*SCR GbMb*/
	0x00e7, 0x0091,	/*SCR BrYr*/
	0x00e8, 0x0099,	/*SCR BgYg*/
	0x00e9, 0xa300,	/*SCR BbYb*/
	0x00ea, 0x0091,	/*SCR KrWr*/
	0x00eb, 0x0099,	/*SCR KgWg*/
	0x00ec, 0x00a3,	/*SCR KbWb*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short bypass[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x0000,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0030, 0x0000,	/*FA cs1 | de8 hdr2 fa1*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short negative[] = {
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
};

static const unsigned short outdoor[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x04ac,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short cold[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x00ec,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08b,	/*MCM 5cb 1cr W*/
	0x000b, 0x7a7a,	/*MCM 4cr 5cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short cold_outdoor[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x04ec,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0064,	/*MCM 10000K*/
	0x0009, 0xa08b,	/*MCM 5cb 1cr W*/
	0x000b, 0x7a7a,	/*MCM 4cr 5cr W*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short warm[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x00ec,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7878,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08b,	/*MCM 5cb 1cr W*/
	0x00ff, 0x0000,	/*Mask Release*/
};

static const unsigned short warm_outdoor[] = {
	0x0000, 0x0000,	/*BANK 0*/
	0x0008, 0x04ec,	/*Dither8 UC4 ABC2 CP1 |
					CC8 MCM4 SCR2 SCC1 | CS8 DE4 DNR2 HDR1*/
	0x0000, 0x0001,	/*BANK 1*/
	0x0001, 0x0028,	/*MCM 4000K*/
	0x0007, 0x7878,	/*MCM 1cb 2cb W*/
	0x0009, 0xa08b,	/*MCM 5cb 1cr W*/
	0x00d0, 0x01c0,	/*UC y*/
	0x00d1, 0x01ff,	/*UC cs*/
	0x00ff, 0x0000,	/*Mask Release*/
};

const struct mdnie_tables mdnie_main_tables[] = {
	{ "dynamic_ui", dynamic_ui, ARRAY_SIZE(dynamic_ui) },
	{ "dynamic_gallery", dynamic_gallery, ARRAY_SIZE(dynamic_gallery) },
	{ "dynamic_video", dynamic_video, ARRAY_SIZE(dynamic_video) },
	{ "dynamic_vtcall", dynamic_vtcall, ARRAY_SIZE(dynamic_vtcall) },
	{ "standard_ui", standard_ui, ARRAY_SIZE(standard_ui) },
	{ "standard_gallery", standard_gallery, ARRAY_SIZE(standard_gallery) },
	{ "standard_video", standard_video, ARRAY_SIZE(standard_video) },
	{ "standard_vtcall", standard_vtcall, ARRAY_SIZE(standard_vtcall) },
	{ "natural_ui", natural_ui, ARRAY_SIZE(natural_ui) },
	{ "natural_gallery",	 natural_gallery, ARRAY_SIZE(natural_gallery) },
	{ "natural_video", natural_video, ARRAY_SIZE(natural_video) },
	{ "natural_vtcall", natural_vtcall, ARRAY_SIZE(natural_vtcall) },
	{ "movie_ui", movie_ui, ARRAY_SIZE(movie_ui) },
	{ "movie_gallery", movie_gallery, ARRAY_SIZE(movie_gallery) },
	{ "movie_video", movie_video, ARRAY_SIZE(movie_video) },
	{ "movie_vtcall", movie_vtcall, ARRAY_SIZE(movie_vtcall) },
	{ "camera", camera, ARRAY_SIZE(camera) },
	{ "camera_outdoor", camera_outdoor, ARRAY_SIZE(camera_outdoor) },
	{ "browser_tone1",	 browser_tone1, ARRAY_SIZE(browser_tone1) },
	{ "browser_tone2",	 browser_tone2, ARRAY_SIZE(browser_tone2) },
	{ "browser_tone3",	 browser_tone3, ARRAY_SIZE(browser_tone3) },
	{ "negative", negative, ARRAY_SIZE(negative) },
	{ "bypass", bypass, ARRAY_SIZE(bypass) },
	{ "outdoor", outdoor, ARRAY_SIZE(outdoor) },
	{ "warm", warm, ARRAY_SIZE(warm) },
	{ "warm_outdoor", warm_outdoor, ARRAY_SIZE(warm_outdoor) },
	{ "cold", cold, ARRAY_SIZE(cold) },
	{ "cold_outdoor", cold_outdoor, ARRAY_SIZE(cold_outdoor) },
};

