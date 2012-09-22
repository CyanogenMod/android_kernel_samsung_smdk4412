/*****************************************************************************

 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_bb.c

 Description : API of 1-SEG baseband module

*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"

static u8 filter_coef_6M[7][16] = {
		{ 0x00, 0x00, 0x00, 0x07, 0x0f, 0x01, 0x03, 0x04
		, 0x01, 0x19, 0x14, 0x18, 0x0a, 0x26, 0x40, 0x4a},
		/*xtal_in : 24,16,32        (adc clk = 4.0000)*/
		{ 0x00, 0x07, 0x07, 0x00, 0x01, 0x03, 0x03, 0x01
		, 0x1c, 0x17, 0x16, 0x1e, 0x10, 0x27, 0x3b, 0x43},
		/*xtal_in : 26              (adc clk = 4.3333)*/
		{ 0x00, 0x07, 0x07, 0x00, 0x01, 0x03, 0x03, 0x00
		, 0x1b, 0x16, 0x16, 0x1f, 0x11, 0x27, 0x3a, 0x41},
		/*xtal_in : 27,18           (adc clk = 4.5000)*/
		{ 0x00, 0x00, 0x07, 0x07, 0x00, 0x02, 0x03, 0x03
		, 0x1f, 0x18, 0x14, 0x1a, 0x0c, 0x26, 0x3e, 0x47},
		/*xtal_in : 16.384, 24.576  (adc clk = 4.0960)*/
		{ 0x00, 0x07, 0x07, 0x00, 0x02, 0x03, 0x02, 0x0f
		, 0x1a, 0x16, 0x18, 0x02, 0x13, 0x27, 0x38, 0x3e},
		/*xtal_in : 19.2, 38.4      (adc clk = 4.8000)*/
		{ 0x00, 0x07, 0x07, 0x00, 0x01, 0x03, 0x03, 0x00
		, 0x1b, 0x16, 0x16, 0x1f, 0x11, 0x27, 0x3a, 0x41},
		/*xtal_in : 27.12           (adc clk = 4.5200)*/
		{ 0x00, 0x07, 0x07, 0x00, 0x02, 0x03, 0x03, 0x00
		, 0x1a, 0x16, 0x17, 0x01, 0x12, 0x27, 0x39, 0x3f}
		/*xtal_in : 37.4            (adc clk = 4.6750)*/
};

#if 0
static u8 filter_coef_7M[7][16] = {
		{ 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x00, 0x04, 0x05
			, 0x1f, 0x15, 0x12, 0x01, 0x22, 0x45, 0x55},
		/*xtal_in : 24,16,32        (adc clk = 4.0000)*/
		{ 0x00, 0x00, 0x00, 0x07, 0x0e, 0x0f, 0x02, 0x05, 0x04
		, 0x1c, 0x14, 0x14, 0x05, 0x24, 0x43, 0x50},
		/*xtal_in : 26              (adc clk = 4.3333)*/
		{ 0x00, 0x00, 0x00, 0x07, 0x0f, 0x00, 0x03, 0x05, 0x02
		, 0x1b, 0x14, 0x16, 0x07, 0x25, 0x42, 0x4d},
		/*xtal_in : 27,18           (adc clk = 4.5000)*/
		{ 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x01, 0x04, 0x05
		, 0x1e, 0x15, 0x13, 0x02, 0x23, 0x44, 0x53},
		/*xtal_in : 16.384, 24.576  (adc clk = 4.0960)*/
		{ 0x00, 0x00, 0x07, 0x07, 0x00, 0x02, 0x04, 0x03, 0x1f
		, 0x18, 0x14, 0x1a, 0x0c, 0x26, 0x3e, 0x47},
		/*xtal_in : 19.2, 38.4      (adc clk = 4.8000)*/
		{ 0x00, 0x00, 0x00, 0x07, 0x0f, 0x00, 0x03, 0x05, 0x02
		, 0x1b, 0x14, 0x16, 0x07, 0x25, 0x41, 0x4d},
		/*xtal_in : 27.12           (adc clk = 4.5200)*/
		{ 0x00, 0x00, 0x00, 0x07, 0x0f, 0x01, 0x03, 0x04, 0x00
		, 0x19, 0x14, 0x18, 0x0a, 0x26, 0x3f, 0x4a}
		/*xtal_in : 37.4            (adc clk = 4.6750)*/
};

static u8 filter_coef_8M[7][16] = {
		{ 0x00, 0x00, 0x00, 0x01, 0x01, 0x0f, 0x0d, 0x0f, 0x05
			, 0x06, 0x1d, 0x10, 0xf6, 0x1a, 0x4b, 0x62},
			/*xtal_in : 24,16,32        (adc clk = 4.0000)*/
		{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x0e, 0x02, 0x06
		, 0x04, 0x19, 0x10, 0xfb, 0x1e, 0x48, 0x5b},
		/*xtal_in : 26              (adc clk = 4.3333)*/
		{ 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x00, 0x03, 0x05
		, 0x00, 0x16, 0x11, 0xff, 0x21, 0x46, 0x56},
		/*xtal_in : 27,18           (adc clk = 4.5000)*/
		{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x0f, 0x0d, 0x00, 0x05
		, 0x06, 0x1c, 0x10, 0xf7, 0x1b, 0x4a, 0x60},
		/*xtal_in : 16.384, 24.576  (adc clk = 4.0960)*/
		{ 0x00, 0x00, 0x00, 0x07, 0x0e, 0x0f, 0x01, 0x05, 0x04
		, 0x1d, 0x14, 0x13, 0x04, 0x23, 0x44, 0x51},
		/*xtal_in : 19.2, 38.4      (adc clk = 4.8000)*/
		{ 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x00, 0x04, 0x05
		, 0x00, 0x16, 0x12, 0x00, 0x21, 0x46, 0x56},
		/*xtal_in : 27.12           (adc clk = 4.5200)*/
		{ 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x01, 0x04, 0x05
		, 0x1e, 0x15, 0x13, 0x02, 0x23, 0x44, 0x53}
		/*xtal_in : 37.4            (adc clk = 4.6750)*/
};
#endif

static void fc8150_clock_mode(HANDLE hDevice)
{
	int i;

#if (BBM_XTAL_FREQ == 16000)
	bbm_write(hDevice, 0x0016, 0);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1f800000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x10);
	bbm_write(hDevice, 0x2008, 0x41);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[0][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1b000000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xbd);
	bbm_write(hDevice, 0x2008, 0xa1);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[0][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3ae1);

	bbm_long_write(hDevice, 0x103c, 0x17a00000);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x6b);
	bbm_write(hDevice, 0x2008, 0x01);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[0][i]);

#endif

#elif (BBM_XTAL_FREQ == 16384)
	bbm_write(hDevice, 0x0016, 0);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2f80);

	bbm_long_write(hDevice, 0x103c, 0x20418937);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0xf7);
	bbm_write(hDevice, 0x2008, 0xdf);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[3][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2f80);

	bbm_long_write(hDevice, 0x103c, 0x1ba5e354);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xa1);
	bbm_write(hDevice, 0x2008, 0x2f);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[3][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3980);

	bbm_long_write(hDevice, 0x103c, 0x183126e9);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x4a);
	bbm_write(hDevice, 0x2008, 0x7f);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[3][i]);
#endif

#elif (BBM_XTAL_FREQ == 18000)
	bbm_write(hDevice, 0x0016, 0);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2b3c);

	bbm_long_write(hDevice, 0x103c, 0x23700000);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x9c);
	bbm_write(hDevice, 0x2008, 0xac);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[2][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2b3c);

	bbm_long_write(hDevice, 0x103c, 0x1e600000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x36);
	bbm_write(hDevice, 0x2008, 0xc8);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[2][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3456);

	bbm_long_write(hDevice, 0x103c, 0x1a940000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xd0);
	bbm_write(hDevice, 0x2008, 0xe5);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[2][i]);

#endif

#elif (BBM_XTAL_FREQ == 19200)
	bbm_write(hDevice, 0x0016, 0);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2889);

	bbm_long_write(hDevice, 0x103c, 0x25cccccd);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x62);
	bbm_write(hDevice, 0x2008, 0xe1);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[4][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2889);

	bbm_long_write(hDevice, 0x103c, 0x20666666);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0xf3);
	bbm_write(hDevice, 0x2008, 0x5c);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[4][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3111);

	bbm_long_write(hDevice, 0x103c, 0x1c59999a);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x83);
	bbm_write(hDevice, 0x2008, 0xd6);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[4][i]);

#endif

#elif (BBM_XTAL_FREQ == 24000)
	bbm_write(hDevice, 0x0016, 2);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1f800000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x10);
	bbm_write(hDevice, 0x2008, 0x41);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[0][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1b000000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xbd);
	bbm_write(hDevice, 0x2008, 0xa1);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[0][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3ae1);

	bbm_long_write(hDevice, 0x103c, 0x17a00000);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x6b);
	bbm_write(hDevice, 0x2008, 0x01);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[0][i]);

#endif

#elif (BBM_XTAL_FREQ == 26000)
	bbm_write(hDevice, 0x0016, 2);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2ce6);

	bbm_long_write(hDevice, 0x103c, 0x221fffd4);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0xc0);
	bbm_write(hDevice, 0x2008, 0x3c);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[1][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2ce6);

	bbm_long_write(hDevice, 0x103c, 0x1d3fffda);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x60);
	bbm_write(hDevice, 0x2008, 0x46);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[1][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x365a);

	bbm_long_write(hDevice, 0x103c, 0x1997ffdf);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x00);
	bbm_write(hDevice, 0x2008, 0x50);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[1][i]);

#endif

#elif (BBM_XTAL_FREQ == 27000)
	bbm_write(hDevice, 0x0016, 2);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2b3c);

	bbm_long_write(hDevice, 0x103c, 0x23700000);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x9c);
	bbm_write(hDevice, 0x2008, 0xac);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[2][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2b3c);

	bbm_long_write(hDevice, 0x103c, 0x1e600000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x36);
	bbm_write(hDevice, 0x2008, 0xc8);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[2][i]);
#else
	bbm_word_write(hDevice, 0x1032, 0x3456);

	bbm_long_write(hDevice, 0x103c, 0x1a940000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xd0);
	bbm_write(hDevice, 0x2008, 0xe5);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[2][i]);
#endif

#elif (BBM_XTAL_FREQ == 27120)
	bbm_write(hDevice, 0x0016, 2);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2b0b);

	bbm_long_write(hDevice, 0x103c, 0x239851ec);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x98);
	bbm_write(hDevice, 0x2008, 0x94);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[5][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2b0b);

	bbm_long_write(hDevice, 0x103c, 0x1e828f5c);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x32);
	bbm_write(hDevice, 0x2008, 0x02);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[5][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x341b);

	bbm_long_write(hDevice, 0x103c, 0x1ab23d71);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xcb);
	bbm_write(hDevice, 0x2008, 0x70);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[5][i]);

#endif

#elif (BBM_XTAL_FREQ == 24576)
	bbm_write(hDevice, 0x0016, 2);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2f80);

	bbm_long_write(hDevice, 0x103c, 0x20418937);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0xf7);
	bbm_write(hDevice, 0x2008, 0xdf);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[3][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2f80);

	bbm_long_write(hDevice, 0x103c, 0x1ba5e354);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xa1);
	bbm_write(hDevice, 0x2008, 0x2f);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[3][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3980);

	bbm_long_write(hDevice, 0x103c, 0x183126e9);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x4a);
	bbm_write(hDevice, 0x2008, 0x7f);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[3][i]);

#endif

#elif (BBM_XTAL_FREQ == 32000)
	/* Default Clock */
	bbm_write(hDevice, 0x0016, 1);

#if (BBM_BAND_WIDTH == 6)
	/* Default Band-width */
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1f800000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x10);
	bbm_write(hDevice, 0x2008, 0x41);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[0][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x30a4);

	bbm_long_write(hDevice, 0x103c, 0x1b000000);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xbd);
	bbm_write(hDevice, 0x2008, 0xa1);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[0][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3ae1);

	bbm_long_write(hDevice, 0x103c, 0x17a00000);

	bbm_write(hDevice, 0x200a, 0x05);
	bbm_write(hDevice, 0x2009, 0x6b);
	bbm_write(hDevice, 0x2008, 0x01);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[0][i]);

#endif

#elif (BBM_XTAL_FREQ == 37400)
	bbm_write(hDevice, 0x0016, 1);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x299e);

	bbm_long_write(hDevice, 0x103c, 0x24d0cccd);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x7a);
	bbm_write(hDevice, 0x2008, 0x0f);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[6][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x299e);

	bbm_long_write(hDevice, 0x103c, 0x1f8e6666);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x0e);
	bbm_write(hDevice, 0x2008, 0x66);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[6][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3261);

	bbm_long_write(hDevice, 0x103c, 0x1b9c999a);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0xa2);
	bbm_write(hDevice, 0x2008, 0xbe);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[6][i]);

#endif

#elif (BBM_XTAL_FREQ == 38400)
	bbm_write(hDevice, 0x0016, 1);

#if (BBM_BAND_WIDTH == 6)
	bbm_word_write(hDevice, 0x1032, 0x2889);

	bbm_long_write(hDevice, 0x103c, 0x25cccccd);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0x62);
	bbm_write(hDevice, 0x2008, 0xe1);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_6M[4][i]);

#elif (BBM_BAND_WIDTH == 7)
	bbm_word_write(hDevice, 0x1032, 0x2889);

	bbm_long_write(hDevice, 0x103c, 0x20666666);

	bbm_write(hDevice, 0x200a, 0x03);
	bbm_write(hDevice, 0x2009, 0xf3);
	bbm_write(hDevice, 0x2008, 0x5c);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_7M[4][i]);

#else
	bbm_word_write(hDevice, 0x1032, 0x3111);

	bbm_long_write(hDevice, 0x103c, 0x1c59999a);

	bbm_write(hDevice, 0x200a, 0x04);
	bbm_write(hDevice, 0x2009, 0x83);
	bbm_write(hDevice, 0x2008, 0xd6);

	for (i = 0; i < 16; i++)
		bbm_write(hDevice, 0x1040+i, filter_coef_8M[4][i]);

#endif
#endif
}

int fc8150_reset(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_SW_RESET, 0x7f);
	bbm_write(hDevice, BBM_SW_RESET, 0xff);

	return BBM_OK;
}

int fc8150_probe(HANDLE hDevice)
{
	u16 ver;
	bbm_word_read(hDevice, BBM_CHIP_ID_L, &ver);

	return (ver == 0x8150) ? BBM_OK : BBM_NOK;
}

int fc8150_init(HANDLE hDevice)
{
	bbm_write(hDevice, 0x00b0, 0x03);
	bbm_write(hDevice, 0x00b1, 0x00);
	bbm_write(hDevice, 0x00b2, 0x14);

	fc8150_reset(hDevice);
	fc8150_clock_mode(hDevice);

	bbm_write(hDevice, 0x1000, 0x27);
	bbm_write(hDevice, 0x1004, 0x4d);
	bbm_write(hDevice, 0x1069, 0x09);
	bbm_write(hDevice, 0x1075, 0x00);

	bbm_word_write(hDevice, 0x00b4, 0x0000);
	/*bbm_write(hDevice, 0x00b6, 0x03);*/ /* Default 0x03, 0x00, 0x07*/
	bbm_write(hDevice, 0x00b9, 0x00);
	bbm_write(hDevice, 0x00ba, 0x01);

	bbm_write(hDevice, 0x2004, 0x41);
	bbm_write(hDevice, 0x2106, 0x1f);

	bbm_write(hDevice, 0x5010, 0x00);
	bbm_write(hDevice, BBM_RS_FAIL_TX, 0x00); /* RS FAILURE TX: 0x02*/

	bbm_word_write(hDevice, BBM_BUF_TS_START, TS_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_TS_END, TS_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_TS_THR, TS_BUF_THR);

	/*bbm_word_write(hDevice, BBM_BUF_AC_START, AC_BUF_START);*/
	/*bbm_word_write(hDevice, BBM_BUF_AC_END, AC_BUF_END);*/
	/*bbm_word_write(hDevice, BBM_BUF_AC_THR, AC_BUF_THR);*/

	/*bbm_write(hDevice, BBM_INT_POLAR_SEL, 0x00);*/
	/*bbm_write(hDevice, BBM_INT_AUTO_CLEAR, 0x01);*/
	/*bbm_write(hDevice, BBM_STATUS_AUTO_CLEAR_EN, 0x00);*/

	bbm_write(hDevice, BBM_BUF_ENABLE, 0x01);
	bbm_write(hDevice, BBM_BUF_INT, 0x01);

	bbm_write(hDevice, BBM_INT_MASK, 0x07);
	bbm_write(hDevice, BBM_INT_STS_EN, 0x01);

	return BBM_OK;
}

int fc8150_deinit(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_SW_RESET, 0x00);

	return BBM_OK;
}

int fc8150_scan_status(HANDLE hDevice)
{
	u32   ifagc_timeout       = 7;
	u32   ofdm_timeout        = 16;
	u32   ffs_lock_timeout    = 10;
	u32   dagc_timeout        = 100; /* always done*/
	u32   cfs_timeout         = 12;
	u32   tmcc_timeout        = 105;
	u32   ts_err_free_timeout = 0;
	int   rssi;
	u8    data, data1;
	u8    lay0_mod, lay0_cr;
	int   i;

	for (i = 0; i < ifagc_timeout; i++) {
		bbm_read(hDevice, 0x3025, &data);

		if (data & 0x01)
			break;

		msWait(10);
	}

	if (i == ifagc_timeout)
		return BBM_NOK;

	tuner_get_rssi(hDevice, &rssi);

	if (rssi < -107)
		return BBM_NOK;

	for (; i < ofdm_timeout; i++) {
		bbm_read(hDevice, 0x3025, &data);

		if (data & 0x08)
			break;

		msWait(10);
	}

	if (i == ofdm_timeout)
		return BBM_NOK;

	if (0 == (data & 0x04))
		return BBM_NOK;

	for (; i < ffs_lock_timeout; i++) {
		bbm_read(hDevice, 0x3026, &data);

		if (data & 0x10)
			break;

		msWait(10);
	}

	if (i == ffs_lock_timeout)
		return BBM_NOK;

	/* DAGC Lock*/
	for (i = 0; i < dagc_timeout; i++) {
		bbm_read(hDevice, 0x3026, &data);

		if (data & 0x01)
			break;

		msWait(10);
	}

	if (i == dagc_timeout)
		return BBM_NOK;

	for (i = 0; i < cfs_timeout; i++) {
		bbm_read(hDevice, 0x3025, &data);

		if (data & 0x40)
			break;

		msWait(10);
	}

	if (i == cfs_timeout)
		return BBM_NOK;

	bbm_read(hDevice, 0x2023, &data1);
	if (data1 & 1)
		return BBM_NOK;

	for (i = 0; i <		tmcc_timeout; i++) {
		bbm_read(hDevice, 0x3026, &data);
		if (data & 0x02)
			break;

		msWait(10);
	}

	if (i == tmcc_timeout)
		return BBM_NOK;

	bbm_read(hDevice, 0x4113, &data);
	bbm_read(hDevice, 0x4114, &data1);

	if (((data >> 3) & 0x1) == 0)
		return BBM_NOK;

	lay0_mod = ((data & 0x10) >> 2) |
			   ((data & 0x20) >> 4) |
			   ((data & 0x40) >> 6);

	lay0_cr = ((data & 0x80) >> 5) |
				((data1 & 0x01) << 1) |
				((data1 & 0x02) >> 1);

	if (!((lay0_mod == 1) || (lay0_mod == 2)))
		return BBM_NOK;

	if (((0x70 & data) == 0x40) && ((0x1c & data1) == 0x18))
		ts_err_free_timeout = 400;
	else
		ts_err_free_timeout = 650;

	for (i = 0; i < ts_err_free_timeout; i++) {
		bbm_read(hDevice, 0x5053, &data);
		if (data & 0x01)
			break;

		msWait(10);
	}

	if (i == ts_err_free_timeout)
		return BBM_NOK;

	return BBM_OK;
}
