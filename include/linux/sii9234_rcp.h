/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * Author: Dharam Kumar <dharam.kr@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __SII9234_RCP_H__
#define __SII9234_RCP_H__

#define MAX_KEY_CODE		0x7F

struct rcp_key {
	char name[32];
	unsigned int valid:1;
	u8 code;
};

/*
 * List of All RCP key codes. By default,all keys are
 * supported(all keys are send to platform).Userspace(Platform) need to decide
 * how to act/process each received key.
 */

const struct rcp_key code[MAX_KEY_CODE+1] = {

	{"SELECT",		1,	0x00},
	{"UP",			1,	0x01},
	{"DOWN",		1,	0x02},
	{"LEFT",		1,	0x03},
	{"RIGHT",		1,	0x04},
	{"RIGHT-UP",		1,	0x05},
	{"RIGHT-DOWN",		1,	0x06},
	{"LEFT-UP",		1,	0x07},
	{"LEFT-DOWN",		1,	0x08},
	{"ROOT-MENU",		1,	0x09},
	{"SETUP-MENU",		1,	0x0A},
	{"CONTENTS-MENU",	1,	0x0B},
	{"FAVORITE-MENU",	1,	0x0C},
	{"EXIT",		1,	0x0D},
	/* Reserved Block 0x0E-0x1F */
	{"RSVD",		1,	0x0E},
	{"RSVD",		1,	0x0F},
	{"RSVD",		1,	0x10},
	{"RSVD",		1,	0x11},
	{"RSVD",		1,	0x12},
	{"RSVD",		1,	0x13},
	{"RSVD",		1,	0x14},
	{"RSVD",		1,	0x15},
	{"RSVD",		1,	0x16},
	{"RSVD",		1,	0x17},
	{"RSVD",		1,	0x18},
	{"RSVD",		1,	0x19},
	{"RSVD",		1,	0x1A},
	{"RSVD",		1,	0x1B},
	{"RSVD",		1,	0x1C},
	{"RSVD",		1,	0x1D},
	{"RSVD",		1,	0x1E},
	{"RSVD",		1,	0x1F},
	/*Numeric Keys */
	{"NUM-0",		1,	0x20},
	{"NUM-1",		1,	0x21},
	{"NUM-2",		1,	0x22},
	{"NUM-3",		1,	0x23},
	{"NUM-4",		1,	0x24},
	{"NUM-5",		1,	0x25},
	{"NUM-6",		1,	0x26},
	{"NUM-7",		1,	0x27},
	{"NUM-8",		1,	0x28},
	{"CLEAR",		1,	0x2C},
	/* 0x2D-0x2F Reserved */
	{"RSVD",		1,	0x2D},
	{"RSVD",		1,	0x2E},
	{"RSVD",		1,	0x2F},

	{"CHANNEL-UP",		1,	0x30},
	{"CHANNEL-DOWN",	1,	0x31},
	{"PREVIOUS-CHANNEL",	1,	0x32},
	{"SOUND-SELECT",	1,	0x33},
	{"INPUT-SELECT",	1,	0x34},
	{"SHOW-INFORMATION",	1,	0x35},
	{"HELP",		1,	0x36},
	{"PAGE-UP",		1,	0x37},
	{"PAGE-DOWN",		1,	0x38},
	/* 0x39-0x40 Reserved */
	{"RSVD",		1,	0x39},
	{"RSVD",		1,	0x3A},
	{"RSVD",		1,	0x3B},
	{"RSVD",		1,	0x3C},
	{"RSVD",		1,	0x3D},
	{"RSVD",		1,	0x3E},

	{"VOLUME-UP",		1,	0x41},
	{"VOLUME-DOWN",		1,	0x42},
	{"MUTE",		1,	0x43},
	{"PLAY",		1,	0x44},
	{"STOP",		1,	0x45},
	{"PAUSE",		1,	0x46},
	{"RECORD",		1,	0x47},
	{"REWIND",		1,	0x48},
	{"FAST-FORWARD",	1,	0x49},
	{"EJECT",		1,	0x4A},
	{"FORWARD",		1,	0x4B},
	{"BACKWARD",		1,	0x4C},
	/*0x4D-0x4F Reserved */
	{"RSVD",		1,	0x4D},
	{"RSVD",		1,	0x4E},
	{"RSVD",		1,	0x4F},

	{"ANGLE",		1,	0x50},
	{"SUB-PICTURE",		1,	0x51},
	/* 0x52-0x5F Reserved */
	{"RSVD",		1,	0x52},
	{"RSVD",		1,	0x53},
	{"RSVD",		1,	0x54},
	{"RSVD",		0,	0x55},
	{"RSVD",		1,	0x56},
	{"RSVD",		1,	0x57},
	{"RSVD",		1,	0x58},
	{"RSVD",		1,	0x59},
	{"RSVD",		1,	0x5A},
	{"RSVD",		1,	0x5B},
	{"RSVD",		1,	0x5C},
	{"RSVD",		1,	0x5D},

	{"PLAY_FUNC",		1,	0x60},
	{"PAUSE_PLAY_FUNC",	1,	0x61},
	{"RECORD_FUNC",		1,	0x62},
	{"PAUSE_RECORD_FUNC",	1,	0x63},
	{"STOP_FUNC",		1,	0x64},
	{"MUTE_FUNC",		1,	0x65},
	{"RESTORE_VOLUME_FUNC",	1,	0x66},
	{"TUNE_FUNC",		1,	0x67},
	{"SELECT_MEDIA_FUNC",	1,	0x68},
	/* 0x69-0x70 Reserved */
	{"RSVD",		1,	0x69},
	{"RSVD",		1,	0x6A},
	{"RSVD",		1,	0x6B},
	{"RSVD",		1,	0x6C},
	{"RSVD",		1,	0x6D},
	{"RSVD",		1,	0x6E},
	{"RSVD",		1,	0x6F},
	{"RSVD",		1,	0x70},

	{"F1",			1,	0x71},
	{"F2",			1,	0x72},
	{"F3",			1,	0x73},
	{"F4",			1,	0x74},
	{"F5",			1,	0x75},
	/* 0x76-0x7D Reserved */
	{"RSVD",		1,	0x76},
	{"RSVD",		1,	0x77},
	{"RSVD",		1,	0x78},
	{"RSVD",		1,	0x79},
	{"RSVD",		1,	0x7A},
	{"RSVD",		1,	0x7B},
	{"RSVD",		1,	0x7C},
	{"RSVD",		1,	0x7D},

	{"VENDOR_SPECIFIC",	1,	0x7E},
	{"RSVD",		1,	0x7F}
};

#endif /* __SII9234_RCP_H__ */
