#ifndef __NT35560_H__
#define __NT35560_H__

#define SLEEPMSEC	0x1000
#define ENDDEF		0x2000
#define	DEFMASK	0xFF00
#define COMMAND_ONLY	0xFE
#define DATA_ONLY	0xFF


const unsigned short SEQ_SET_PIXEL_FORMAT[] = {
	0x3A, 0x77,

	ENDDEF, 0x0000
};

const unsigned short SEQ_RGBCTRL[] = {
	0x3B, 0x07,
	DATA_ONLY, 0x0A,
	DATA_ONLY, 0x0E,
	DATA_ONLY, 0x0A,
	DATA_ONLY, 0x0A,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SET_HORIZONTAL_ADDRESS[] = {
	0x2A, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0xDF,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SET_VERTICAL_ADDRESS[] = {
	0x2B, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	DATA_ONLY, 0x1F,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SET_ADDRESS_MODE[] = {
	0x36, 0xD4,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SLPOUT[] = {
	SLEEPMSEC, 25,

	0x11, COMMAND_ONLY,

	SLEEPMSEC, 200,

	ENDDEF, 0x00
};

const unsigned short SEQ_WRDISBV[] = {
	0x51, 0x6C,

	ENDDEF, 0x0000
};

const unsigned short SEQ_WRCTRLD_1[] = {
	0x55, 0x01,

	ENDDEF, 0x0000
};

const unsigned short SEQ_WRCABCMB[] = {
	0x5E, 0x00,

	ENDDEF, 0x0000
};

const unsigned short SEQ_WRCTRLD_2[] = {
	0x53, 0x2C,

	ENDDEF, 0x0000
};

const unsigned short SEQ_DISPLAY_ON[] = {
	0x29, COMMAND_ONLY,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SET_DISPLAY_OFF[] = {
	0x28, COMMAND_ONLY,
	SLEEPMSEC, 25,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SLPIN[] = {
	0x10, COMMAND_ONLY,

	ENDDEF, 0x0000
};

#endif	/* __NT35560_H__ */
