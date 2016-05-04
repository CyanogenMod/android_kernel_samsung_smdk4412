// filename: ISSP_Vectors.h
#include "issp_revision.h"
#ifdef PROJECT_REV_304
/* Copyright 2006-2007, Cypress Semiconductor Corporation.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONRTACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND,EXPRESS OR IMPLIED,
 WITH REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 Cypress reserves the right to make changes without further notice to the
 materials described herein. Cypress does not assume any liability arising
 out of the application or use of any product or circuit described herein.
 Cypress does not authorize its products for use as critical components in
 life-support systems where a malfunction or failure may reasonably be
 expected to result in significant injury to the user. The inclusion of
 Cypress� product in a life-support systems application implies that the
 manufacturer assumes all risk of such use and in doing so indemnifies
 Cypress against all charges.

 Use may be limited by and subject to the applicable Cypress software
 license agreement.

-----------------------------------------------------------------------------*/
#ifndef INC_ISSP_VECTORS
#define INC_ISSP_VECTORS

#include "issp_directives.h"

#define HEX_DEFINE
// ------------------------- PSoC CY8C20x66 Devices ---------------------------

#ifdef CY8C20236		/// 2009.03.26. kimc
unsigned char target_id_v[] = { 0x00, 0xb3, 0x52, 0x21 };	//ID for CY8C20236
#endif
#ifdef CY8C20246		/// 2009.03.26. kimc
unsigned char target_id_v[] = { 0x00, 0xAA, 0x52, 0x21 };	//ID for CY8C20246
#endif
#ifdef CY8C20266
unsigned char target_id_v[] = { 0x00, 0x96, 0x52, 0x21 };	//ID for CY8C20266
#endif
#ifdef CY8C20366
unsigned char target_id_v[] = { 0x00, 0x97, 0x52, 0x21 };	//ID for CY8C20366
#endif
#ifdef CY8C20466
unsigned char target_id_v[] = { 0x00, 0x98, 0x52, 0x21 };	//ID for CY8C20466
#endif
#ifdef CY8C20566
unsigned char target_id_v[] = { 0x00, 0x99, 0x52, 0x21 };	//ID for CY8C20566
#endif
#ifdef CY8C20666
unsigned char target_id_v[] = { 0x00, 0x9c, 0x52, 0x21 };	//ID for CY8C20666
#endif
#ifdef CY8C20066
unsigned char target_id_v[] = { 0x00, 0x9a, 0x52, 0x21 };	//ID for CY8C20066
#endif
#ifdef CY8C200661
unsigned char target_id_v[] = { 0x00, 0x9b, 0x52, 0x21 };	//ID for CY8C200661
#endif

#ifdef CY8C20x66
unsigned char target_status00_v = 0x00;	//PTJ: Status = 00 means Success, the SROM function did what it was supposed to
unsigned char target_status01_v = 0x01;	//PTJ: Status = 01 means that function is not allowed because of block level protection, for test with verify_setup (VERIFY-SETUP)
unsigned char target_status03_v = 0x03;	//PTJ: Status = 03 is fatal error, SROM halted
unsigned char target_status04_v = 0x04;	//PTJ: Status = 04 means that ___ for test with ___ (PROGRAM-AND-VERIFY)
unsigned char target_status06_v = 0x06;	//PTJ: Status = 06 means that Calibrate1 failed, for test with id_setup_1 (ID-SETUP-1)
#endif

/*************** CY8CTMA30x, CY8CTMG30x, CY8CTST30x series by KIMC, 2009.08.11 ***********************************/
// ------------------------- PSoC CY8CTMA30x, CY8CTMG30x, CY8CTST30x Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8CTST300_36		// CY8CTST300_36LQXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x06, 0x71, 0x70, 0x11 };
#endif
#ifdef CY8CTST300_48		// CY8CTST300_48LTXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x06, 0x72, 0x70, 0x11 };
#endif
#ifdef CY8CTST300_49		// CY8CTST300_49FNXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x06, 0x73, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300_36		// CY8CTMA300_36LQXI    // 2009.08.11, Test OK.
unsigned char target_id_v[] = { 0x05, 0x71, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300_48		// CY8CTMA300_48LTXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x72, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300_49		// CY8CTMA300_49FNXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x73, 0x70, 0x11 };
#endif
#ifdef CY8CTMG300_36		// CY8CTMG300_36LQXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x07, 0x71, 0x70, 0x11 };
#endif
#ifdef CY8CTMG300_48		// CY8CTMG300_48LTXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x07, 0x72, 0x70, 0x11 };
#endif
#ifdef CY8CTMG300_49		// CY8CTMG300_49FNXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x07, 0x73, 0x70, 0x11 };
#endif
#ifdef CY8CTMG300B_36		// CY8CTMG300B_36LQXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x07, 0x75, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300B_36		// CY8CTMA300B_36LQXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x74, 0x70, 0x11 };
#endif
#ifdef CY8CTST300B_36		// CY8CTST300B_36LQXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x06, 0x74, 0x70, 0x11 };
#endif
#ifdef CY8CTMA301_36		// CY8CTMA301_36LQXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x75, 0x70, 0x11 };
#endif
#ifdef CY8CTMA301_48		// CY8CTMA301_48LTXI           // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x76, 0x70, 0x11 };
#endif
#ifdef CY8CTMA301D_36		// CY8CTMA301D_36LQXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x77, 0x70, 0x11 };
#endif
#ifdef CY8CTMA301D_48		// CY8CTMA301D_48LTXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x78, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300D_36		// CY8CTMA300D_36LQXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x79, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300D_48		// CY8CTMA300D_48LTXI         // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x80, 0x70, 0x11 };
#endif
#ifdef CY8CTMA300D_49		// CY8CTMA300D_49FNXIT        // 2009.08.11, not tested.
unsigned char target_id_v[] = { 0x05, 0x81, 0x70, 0x11 };
#endif
/********************************************************************************************************/

/*************** CY8CTMG/TST series modified by KJHW, 2009.08.14 ***********************************/
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8CTMG110
unsigned char target_id_v[] = { 0x07, 0x38 };	//ID for CY8CTMG110

unsigned char target_status00_v = 0x00;	//PTJ: Status = 00 means Success, the SROM function did what it was supposed to
unsigned char target_status01_v = 0x01;	//PTJ: Status = 01 means that function is not allowed because of block level protection, for test with verify_setup (VERIFY-SETUP)
unsigned char target_status03_v = 0x03;	//PTJ: Status = 03 is fatal error, SROM halted
unsigned char target_status04_v = 0x04;	//PTJ: Status = 04 means that ___ for test with ___ (PROGRAM-AND-VERIFY)
unsigned char target_status06_v = 0x06;	//PTJ: Status = 06 means that Calibrate1 failed, for test with id_setup_1 (ID-SETUP-1)
#endif

#ifdef CY8CTST200_24PIN
unsigned char target_id_v[] = { 0x06, 0x6D, 0x52, 0x21 };	//ID for CY8CTST200
#endif
#ifdef CY8CTST200_32PIN
unsigned char target_id_v[] = { 0x06, 0x6E, 0x52, 0x21 };	//ID for CY8CTST200
#endif
#ifdef CY8CTMG200_24PIN
unsigned char target_id_v[] = { 0x07, 0x6D, 0x52, 0x21 };	//ID for CY8CTMG200
#endif
#ifdef CY8CTMG200_32PIN
unsigned char target_id_v[] = { 0x07, 0x6E, 0x52, 0x21 };	//ID for CY8CTMG200
#endif

/********************************************************************************************************/

// ------------------------- PSoC CY8C21x23 Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C21123
unsigned char target_id_v[] = { 0x00, 0x17 };	//ID for CY8C21123
#endif
#ifdef CY8C21223
unsigned char target_id_v[] = { 0x00, 0x18 };	//ID for CY8C21223
#endif
#ifdef CY8C21323
unsigned char target_id_v[] = { 0x00, 0x19 };	//ID for CY8C2132
#endif
#ifdef CY8C21002
unsigned char target_id_v[] = { 0x00, 0x3F };	//ID for CY8C21x23 ICE pod
#endif

// ------------------------- PSoC CY8C21x34 Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C21234
unsigned char target_id_v[] = { 0x00, 0x36 };	//ID for CY8C21234
#endif
#ifdef CY8C21334
unsigned char target_id_v[] = { 0x00, 0x37 };	//ID for CY8C21334
#endif
#ifdef CY8C21434
unsigned char target_id_v[] = { 0x00, 0x38 };	//ID for CY8C21434
#endif
#ifdef CY8C21534
unsigned char target_id_v[] = { 0x00, 0x40 };	//ID for CY8C21534
#endif
#ifdef CY8C21634
unsigned char target_id_v[] = { 0x00, 0x49 };	//ID for CY8C21634
#endif
#ifdef CY8C21001
unsigned char target_id_v[] = { 0x00, 0x39 };	//ID for CY8C21x34 ICE pod
#endif

// ------------------------- PSoC CY8C24x23A Devices --------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C24123A
unsigned char target_id_v[] = { 0x00, 0x32 };	//ID for CY8C24123A
#endif
#ifdef CY8C24223A
unsigned char target_id_v[] = { 0x00, 0x33 };	//ID for CY8C24223A
#endif
#ifdef CY8C24423A
unsigned char target_id_v[] = { 0x00, 0x34 };	//ID for CY8C24423A
#endif
#ifdef CY8C24000A
unsigned char target_id_v[] = { 0x00, 0x35 };	//ID for CY8C24x23A ICE pod
#endif

// ------------------------- PSoC CY8C24x94 Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C24794
unsigned char target_id_v[] = { 0x00, 0x1D };	//ID for CY8C24794
#endif
#ifdef CY8C24894
unsigned char target_id_v[] = { 0x00, 0x1F };	//ID for CY8C24894
#endif
#ifdef CY8C24994
unsigned char target_id_v[] = { 0x00, 0x59 };	//ID for CY8C24994
#endif
#ifdef CY8C24094
unsigned char target_id_v[] = { 0x00, 0x1B };	//ID for CY8C24094
#endif

// ------------------------- PSoC CY8C27x43 Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C27143
unsigned char target_id_v[] = { 0x00, 0x09 };	//ID for CY8C27143
#endif
#ifdef CY8C27243
unsigned char target_id_v[] = { 0x00, 0x0A };	//ID for CY8C27243
#endif
#ifdef CY8C27443
unsigned char target_id_v[] = { 0x00, 0x0B };	//ID for CY8C27443
#endif
#ifdef CY8C27543
unsigned char target_id_v[] = { 0x00, 0x0C };	//ID for CY8C27543
#endif
#ifdef CY8C27643
unsigned char target_id_v[] = { 0x00, 0x0D };	//ID for CY8C27643
#endif
#ifdef CY8C27002
unsigned char target_id_v[] = { 0x00, 0x0E };	//ID for CY8C27x43 ICE pod
#endif

// ------------------------- PSoC CY8C29x66 Devices ---------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8C29466
unsigned char target_id_v[] = { 0x00, 0x2A };	//ID for CY8C29466
#endif
#ifdef CY8C29566
unsigned char target_id_v[] = { 0x00, 0x2B };	//ID for CY8C29566
#endif
#ifdef CY8C29666
unsigned char target_id_v[] = { 0x00, 0x2C };	//ID for CY8C29666
#endif
#ifdef CY8C29866
unsigned char target_id_v[] = { 0x00, 0x2D };	//ID for CY8C29866
#endif
#ifdef CY8C29002
unsigned char target_id_v[] = { 0x00, 0x2E };	//ID for CY8C29002
#endif

// --------- CY8C20x66 Vectors ------------------------------------------------
// ----------------------------------------------------------------------------
#ifdef TSYNC
const unsigned int num_bits_tsync_enable = 110;
const unsigned char tsync_enable[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09,
	0xF7, 0x00, 0x1F, 0xDE, 0xE0, 0x1C
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b11011110, 0 b11100000,
	    0 b00011100
#endif
};

const unsigned int num_bits_tsync_disable = 110;
const unsigned char tsync_disable[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE2, 0x1F, 0x71, 0x00, 0x7D, 0xFC, 0x01,
	0xF7, 0x00, 0x1F, 0xDE, 0xE0, 0x1C
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01110001, 0 b00000000,
	    0 b01111101, 0 b11111100, 0 b00000001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b11011110, 0 b11100000,
	    0 b00011100
#endif
};
#endif

#ifdef CY8CTMx30x
#ifdef ID_SETUP_1
const unsigned int num_bits_id_setup_1 = 616;	//KIMC, 2009.08.11, PTJ: id_setup_1 with TSYNC enabled for MW and disabled for IOW
const unsigned char id_setup_1[] = {
	0 b11001010, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00001101, 0 b11101110, 0 b00100001, 0 b11110111, 0 b11110000,
	    0 b00100111, 0 b11011100, 0 b01000000,
	0 b10011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11100111, 0 b11000001,
	0 b11010111, 0 b10011111, 0 b00100000, 0 b01111110, 0 b00111111,
	    0 b10011101, 0 b01111000, 0 b11110110,
	0 b00100001, 0 b11110111, 0 b10111000, 0 b10000111, 0 b11011111,
	    0 b11000000, 0 b00011111, 0 b01110001,
	0 b00000000, 0 b01111101, 0 b11000000, 0 b00000111, 0 b11110111,
	    0 b10111000, 0 b00000111, 0 b11011110,
	0 b10000000, 0 b01111111, 0 b01111010, 0 b10000000, 0 b01111101,
	    0 b11101100, 0 b00000001, 0 b11110111,
	0 b10000000, 0 b01001111, 0 b11011111, 0 b00000000, 0 b00011111,
	    0 b01111100, 0 b10100000, 0 b01111101,
	0 b11110100, 0 b01100001, 0 b11110111, 0 b11111000, 0 b10010111
};
#endif
#else
#ifdef ID_SETUP_1
const unsigned int num_bits_id_setup_1 = 594;	//PTJ: id_setup_1 with TSYNC enabled for MW and disabled for IOW
const unsigned char id_setup_1[] = {
#ifdef HEX_DEFINE
	0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0D, 0xEE, 0x21, 0xF7, 0xF0, 0x27, 0xDC, 0x40,
	0x9F, 0x70, 0x01, 0xFD, 0xEE, 0x01, /*0x21, */ 0xE7, 0xC1,
	0xD7, 0x9F, 0x20, 0x7E, 0x7D, 0x88, 0x7D, 0xEE,
	0x21, 0xF7, 0xF0, 0x07, 0xDC, 0x40, 0x1F, 0x70,
	0x01, 0xFD, 0xEE, 0x01, 0xF7, 0xA0, 0x1F, 0xDE,
	0xA0, 0x1F, 0x7B, 0x00, 0x7D, 0xE0, 0x13, 0xF7,
	0xC0, 0x07, 0xDF, 0x28, 0x1F, 0x7D, 0x18, 0x7D,
	0xFE, 0x25, 0xC0
#else
	0 b11001010, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00001101, 0 b11101110, 0 b00100001, 0 b11110111, 0 b11110000,
	    0 b00100111, 0 b11011100, 0 b01000000,
	0 b10011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00100001, 0 b11100111, 0 b11000001,
	0 b11010111, 0 b10011111, 0 b00100000, 0 b01111110, 0 b01111101,
	    0 b10001000, 0 b01111101, 0 b11101110,
	0 b00100001, 0 b11110111, 0 b11110000, 0 b00000111, 0 b11011100,
	    0 b01000000, 0 b00011111, 0 b01110000,
	0 b00000001, 0 b11111101, 0 b11101110, 0 b00000001, 0 b11110111,
	    0 b10100000, 0 b00011111, 0 b11011110,
	0 b10100000, 0 b00011111, 0 b01111011, 0 b00000000, 0 b01111101,
	    0 b11100000, 0 b00010011, 0 b11110111,
	0 b11000000, 0 b00000111, 0 b11011111, 0 b00101000, 0 b00011111,
	    0 b01111101, 0 b00011000, 0 b01111101,
	0 b11111110, 0 b00100101, 0 b11000000
#endif
};
#endif
#endif

#ifdef SET_BLOCK_NUM
const unsigned int num_bits_set_block_num = 11;	//PTJ:
const unsigned char set_block_num[] = {
#ifdef HEX_DEFINE
	0x9f, 0x40, 0x1c
#else
	0 b11011110, 0 b11100000, 0 b00011110, 0 b01111101, 0 b00000000,
	    0 b01110000
#endif
};

const unsigned int num_bits_set_block_num_end = 3;	//PTJ: this selects the first three bits of set_block_num_end
const unsigned char set_block_num_end = 0xE0;
#endif

#ifdef READ_WRITE_SETUP
const unsigned int num_bits_read_write_setup = 66;	//PTJ:
const unsigned char read_write_setup[] = {
#ifdef HEX_DEFINE
	0xde, 0xf0, 0x1f, 0x78, 0x00, 0x7d, 0xa0, 0x03,
	0xc0
#else
	0 b11011110, 0 b11110000, 0 b00011111, 0 b01111000, 0 b00000000,
	    0 b01111101, 0 b10100000, 0 b00000011,
	0 b11000000
#endif
};
#endif

#ifdef VERIFY_SETUP
const unsigned int num_bits_my_verify_setup = 440;
const unsigned char verify_setup[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0xfc, 0x81,
	0xf9, 0xf7, 0x01, 0xf7, 0xf0, 0x07, 0xdc, 0x40,
	0x1f, 0x70, 0x01, 0xfd, 0xee, 0x01, 0xf6, 0xa8,
	0x0f, 0xde, 0x80, 0x7f, 0x7a, 0x80, 0x7d, 0xec,
	0x01, 0xf7, 0x80, 0x0f, 0xdf, 0x00, 0x1f, 0x7c,
	0xa0, 0xfd, 0xf4, 0x61, 0xf7, 0xf8, 0x97
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11110000,
	    0 b00000111, 0 b11011100, 0 b01000000,
	0 b00011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11110110, 0 b10101000,
	0 b00001111, 0 b11011110, 0 b10000000, 0 b01111111, 0 b01111010,
	    0 b10000000, 0 b01111101, 0 b11101100,
	0 b00000001, 0 b11110111, 0 b10000000, 0 b00001111, 0 b11011111,
	    0 b00000000, 0 b00011111, 0 b01111100,
	0 b10100000, 0 b01111101, 0 b11110100, 0 b01100001, 0 b11110111,
	    0 b11111000, 0 b10010111
#endif
};
#endif

#ifdef ERASE
const unsigned int num_bits_erase = 396;	//PTJ: erase with TSYNC Enable and Disable
const unsigned char erase[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0x7c, 0x85,
	0xfd, 0xfc, 0x01, 0xf7, 0x10, 0x07, 0xdc, 0x00,
	0x7f, 0x7b, 0x80, 0x7d, 0xe0, 0x0b, 0xf7, 0xa0,
	0x1f, 0xd7, 0xa0, 0x1f, 0x7b, 0x04, 0x7d, 0xf0,
	0x01, 0xf7, 0xc9, 0x87, 0xdf, 0x48, 0x1f, 0x7f,
	0x89, 0x70
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000101,
	0 b11111101, 0 b11111100, 0 b00000001, 0 b11110111, 0 b00010000,
	    0 b00000111, 0 b11011100, 0 b00000000,
	0 b01111111, 0 b01111011, 0 b10000000, 0 b01111101, 0 b11100000,
	    0 b00001011, 0 b11110111, 0 b10100000,
	0 b00011111, 0 b11011110, 0 b10100000, 0 b00011111, 0 b01111011,
	    0 b00000100, 0 b01111101, 0 b11110000,
	0 b00000001, 0 b11110111, 0 b11001001, 0 b10000111, 0 b11011111,
	    0 b01001000, 0 b00011111, 0 b01111111,
	0 b10001001, 0 b01110000
#endif
};
#endif

#ifdef SECURE
const unsigned int num_bits_secure = 440;	//PTJ: secure with TSYNC Enable and Disable
const unsigned char secure[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0x7c, 0x81,
	0xf9, 0xf7, 0x01, 0xf7, 0xf0, 0x07, 0xdc, 0x40,
	0x1f, 0x70, 0x01, 0xfd, 0xee, 0x01, 0xf6, 0xa0,
	0x0f, 0xde, 0x80, 0x7f, 0x7a, 0x80, 0x7d, 0xec,
	0x01, 0xf7, 0x80, 0x27, 0xdf, 0x00, 0x1f, 0x7c,
	0xa0, 0x7d, 0xf4, 0x61, 0xf7, 0xf8, 0x97
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11110000,
	    0 b00000111, 0 b11011100, 0 b01000000,
	0 b00011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11110110, 0 b10100000,
	0 b00001111, 0 b11011110, 0 b10000000, 0 b01111111, 0 b01111010,
	    0 b10000000, 0 b01111101, 0 b11101100,
	0 b00000001, 0 b11110111, 0 b10000000, 0 b00100111, 0 b11011111,
	    0 b00000000, 0 b00011111, 0 b01111100,
	0 b10100000, 0 b01111101, 0 b11110100, 0 b01100001, 0 b11110111,
	    0 b11111000, 0 b10010111
#endif
};
#endif

#ifdef READ_SECURITY
const unsigned int num_bits_ReadSecuritySetup = 88;	//PTJ: READ-SECURITY-SETUP
const unsigned char ReadSecuritySetup[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x60, 0x88, 0x7d, 0x84, 0x21,
	0xf7, 0xb8, 0x07
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01100000, 0 b10001000,
	    0 b01111101, 0 b10000100, 0 b00100001,
	0 b11110111, 0 b10111000, 0 b00000111
#endif
};

const unsigned int num_bits_read_security_pt1 = 78;	//PTJ: This sends the beginning of the Read Supervisory command
const unsigned char read_security_pt1[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x72, 0x87, 0x7d, 0xca, 0x01,
	0xf7, 0x28
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01110010, 0 b10000111,
	    0 b01111101, 0 b11001010, 0 b00000001,
	0 b11110111, 0 b00101000
#endif
};

const unsigned int num_bits_read_security_pt1_end = 25;	//PTJ: this finishes the Address Low command and sends the Address High command
const unsigned char read_security_pt1_end[] = {
#ifdef HEX_DEFINE
	0xfb, 0x94, 0x03, 0x80
#else
	0 b11111011, 0 b10010100, 0 b00000011, 0 b10000000
#endif
};

const unsigned int num_bits_read_security_pt2 = 198;	//PTJ: load the test queue with the op code for MOV 1,E5h register into Accumulator A
const unsigned char read_security_pt2[] = {
#ifdef HEX_DEFINE
	0xde, 0xe0, 0x1f, 0x7a, 0x01, 0xfd, 0xea, 0x01,
	0xf7, 0xb0, 0x07, 0xdf, 0x0b, 0xbf, 0x7c, 0xf2,
	0xfd, 0xf4, 0x61, 0xf7, 0xb8, 0x87, 0xdf, 0xe2,
	0x5c
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111010, 0 b00000001,
	    0 b11111101, 0 b11101010, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b11011111, 0 b00001011,
	    0 b10111111, 0 b01111100, 0 b11110010,
	0 b11111101, 0 b11110100, 0 b01100001, 0 b11110111, 0 b10111000,
	    0 b10000111, 0 b11011111, 0 b11100010,
	0 b01011100
#endif
};

const unsigned int num_bits_read_security_pt3 = 122;	//PTJ:
const unsigned char read_security_pt3[] = {
#ifdef HEX_DEFINE
	0xde, 0xe0, 0x1f, 0x7a, 0x01, 0xfd, 0xea, 0x01,
	0xf7, 0xb0, 0x07, 0xdf, 0x0a, 0x7f, 0x7c, 0xc0
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111010, 0 b00000001,
	    0 b11111101, 0 b11101010, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b11011111, 0 b00001010,
	    0 b01111111, 0 b01111100, 0 b11000000
#endif
};

const unsigned int num_bits_read_security_pt3_end = 47;	//PTJ:
const unsigned char read_security_pt3_end[] = {
#ifdef HEX_DEFINE
	0xfb, 0xe8, 0xc3, 0xef, 0xf1, 0x2e
#else
	0 b11111011, 0 b11101000, 0 b11000011, 0 b11101111, 0 b11110001,
	    0 b00101110
#endif
};

#endif

// --------- CY8C20x66 Checksum Setup Vector ----------------------------------
// ----------------------------------------------------------------------------
#ifdef CHECKSUM_SETUP
const unsigned int num_bits_checksum_setup = 418;	//PTJ: Checksum with TSYNC Enable and Disable
const unsigned char checksum_setup[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0x7c, 0x81,
	0xf9, 0xf4, 0x01, 0xf7, 0xf0, 0x07, 0xdc, 0x40,
	0x1f, 0x70, 0x01, 0xfd, 0xee, 0x01, 0xf7, 0xa0,
	0x1f, 0xde, 0xa0, 0x1f, 0x7b, 0x00, 0x7d, 0xe0,
	0x0f, 0xf7, 0xc0, 0x07, 0xdf, 0x28, 0x1f, 0x7d,
	0x18, 0x7d, 0xfe, 0x25, 0xc0
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111001, 0 b11110100, 0 b00000001, 0 b11110111, 0 b11110000,
	    0 b00000111, 0 b11011100, 0 b01000000,
	0 b00011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11110111, 0 b10100000,
	0 b00011111, 0 b11011110, 0 b10100000, 0 b00011111, 0 b01111011,
	    0 b00000000, 0 b01111101, 0 b11100000,
	0 b00001111, 0 b11110111, 0 b11000000, 0 b00000111, 0 b11011111,
	    0 b00101000, 0 b00011111, 0 b01111101,
	0 b00011000, 0 b01111101, 0 b11111110, 0 b00100101, 0 b11000000
#endif
};
#endif

// --------- CY8C21x23, CY8C21x34 & CY8C27x43 Checksum Setup Vectors ----------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CHECKSUM_SETUP_21_27
const unsigned int num_bits_checksum = 286;
const unsigned char checksum_v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7B, 0x00, 0x79, 0xF0, 0x75,
	0xE7, 0xC8, 0x1F, 0xDE, 0xA0, 0x1F, 0x7A, 0x01,
	0xF9, 0xF7, 0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48,
	0x1E, 0x7D, 0x00, 0x7D, 0xE0, 0x0F, 0xF7, 0xC0,
	0x07, 0xDF, 0xE2, 0x5C
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111011, 0 b00000000,
	    0 b01111001, 0 b11110000, 0 b01110101,
	0 b11100111, 0 b11001000, 0 b00011111, 0 b11011110, 0 b10100000,
	    0 b00011111, 0 b01111010, 0 b00000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11001001,
	    0 b10000111, 0 b11011111, 0 b01001000,
	0 b00011110, 0 b01111101, 0 b00000000, 0 b01111101, 0 b11100000,
	    0 b00001111, 0 b11110111, 0 b11000000,
	0 b00000111, 0 b11011111, 0 b11100010, 0 b01011100
#endif
};
#endif

// -------------- CY8C24x23 & CY8C24x23A Checksum Setup Vectors ---------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CHECKSUM_SETUP_24_24A
const unsigned int num_bits_checksum = 286;
const unsigned char checksum_v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7B, 0x00, 0x79, 0xF0, 0x75,
	0xE7, 0xC8, 0x1F, 0xDE, 0xA0, 0x1F, 0x7A, 0x01,
	0xF9, 0xF7, 0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48,
	0x1E, 0x7D, 0x20, 0x7D, 0xE0, 0x0F, 0xF7, 0xC0,
	0x07, 0xDF, 0xE2, 0x5C
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111011, 0 b00000000,
	    0 b01111001, 0 b11110000, 0 b01110101,
	0 b11100111, 0 b11001000, 0 b00011111, 0 b11011110, 0 b10100000,
	    0 b00011111, 0 b01111010, 0 b00000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11001001,
	    0 b10000111, 0 b11011111, 0 b01001000,
	0 b00011110, 0 b01111101, 0 b00100000, 0 b01111101, 0 b11100000,
	    0 b00001111, 0 b11110111, 0 b11000000,
	0 b00000111, 0 b11011111, 0 b11100010, 0 b01011100
#endif
};
#endif

// -------------- CY8C24x94 & CY8C29x66 Checksum Setup Vectors ----------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CHECKSUM_SETUP_24_29
const unsigned int num_bits_checksum = 286;
const unsigned char checksum_v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7B, 0x00, 0x79, 0xF0, 0x75,
	0xE7, 0xC8, 0x1F, 0xDE, 0xA0, 0x1F, 0x7A, 0x01,
	0xF9, 0xF6, 0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48,
	0x1E, 0x7D, 0x40, 0x7D, 0xE0, 0x0F, 0xF7, 0xC0,
	0x07, 0xDF, 0xE2, 0x5C
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111011, 0 b00000000,
	    0 b01111001, 0 b11110000, 0 b01110101,
	0 b11100111, 0 b11001000, 0 b00011111, 0 b11011110, 0 b10100000,
	    0 b00011111, 0 b01111010, 0 b00000001,
	0 b11111001, 0 b11110110, 0 b00000001, 0 b11110111, 0 b11001001,
	    0 b10000111, 0 b11011111, 0 b01001000,
	0 b00011110, 0 b01111101, 0 b00100000, 0 b01111101, 0 b11100000,
	    0 b00001111, 0 b11110111, 0 b11000000,
	0 b00000111, 0 b11011111, 0 b11100010, 0 b01011100
#endif
};
#endif

// ---- CY8C20x66 Program Block Vector ----------------------------------------
//
// ----------------------------------------------------------------------------
#ifdef PROGRAM_AND_VERIFY
const unsigned int num_bits_program_and_verify = 440;	//KIMC, PTJ: length of program_block[], not including zero padding at end
const unsigned char program_and_verify[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0x7c, 0x81,
	0xf9, 0xf7, 0x01, 0xf7, 0xf0, 0x07, 0xdc, 0x40,
	0x1f, 0x70, 0x01, 0xfd, 0xee, 0x01, 0xf6, 0xa0,
	0x0f, 0xde, 0x80, 0x7f, 0x7a, 0x80, 0x7d, 0xec,
	0x01, 0xf7, 0x80, 0x57, 0xdf, 0x00, 0x1f, 0x7c,
	0xa0, 0x7d, 0xf4, 0x61, 0xf7, 0xf8, 0x97
#else
	0 b00011011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b00011110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b00011111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11110000,
	    0 b00000111, 0 b11011100, 0 b01000000,
	0 b00000011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11110110, 0 b10100000,
	0 b00000001111, 0 b11011110, 0 b10000000, 0 b01111111, 0 b01111010,
	    0 b10000000, 0 b01111101, 0 b11101100,
	0 b00000000001, 0 b11110111, 0 b10000000, 0 b01010111, 0 b11011111,
	    0 b00000000, 0 b00011111, 0 b01111100,
	0 b00010100000, 0 b01111101, 0 b11110100, 0 b01100001, 0 b11110111,
	    0 b11111000, 0 b10010111
#endif
};
#endif

// ---- CY8C21xxx, CY8C24x23A, CY8C24x94 & CY8C29x66 Program Block Vectors ----
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef PROGRAM_BLOCK_21_24_29
const unsigned int num_bits_program_block = 308;
const unsigned char program_block[] = {
#ifdef HEX_DEFINE
	0x9F, 0x8A, 0x9E, 0x7F, 0x2B, 0x7D, 0xEE, 0x01,
	0xF7, 0xB0, 0x07, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
	0xFD, 0xEA, 0x01, 0xF7, 0xA0, 0x1F, 0x9F, 0x70,
	0x1F, 0x7C, 0x98, 0x7D, 0xF4, 0x81, 0xF7, 0x80,
	0x17, 0xDF, 0x00, 0x1F, 0x7F, 0x89, 0x70
#else
	0 b10011111, 0 b10001010, 0 b10011110, 0 b01111111, 0 b00101011,
	    0 b01111101, 0 b11101110, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111101, 0 b11101010, 0 b00000001, 0 b11110111, 0 b10100000,
	    0 b00011111, 0 b10011111, 0 b01110000,
	0 b00011111, 0 b01111100, 0 b10011000, 0 b01111101, 0 b11110100,
	    0 b10000001, 0 b11110111, 0 b10000000,
	0 b00010111, 0 b11011111, 0 b00000000, 0 b00011111, 0 b01111111,
	    0 b10001001, 0 b01110000
#endif
};
#endif

// --------------------- CY8C27x43 Program Block Vectors-----------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef PROGRAM_BLOCK_27
const unsigned int num_bits_program_block = 308;

const unsigned char program_block[] = {
#ifdef HEX_DEFINE
	0x9F, 0x82, 0xBE, 0x7F, 0x2B, 0x7D, 0xEE, 0x01,
	0xF7, 0xB0, 0x07, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
	0xFD, 0xEA, 0x01, 0xF7, 0xA0, 0x1F, 0x9F, 0x70,
	0x1F, 0x7C, 0x98, 0x7D, 0xF4, 0x81, 0xF7, 0x80,
	0x17, 0xDF, 0x00, 0x1F, 0x7F, 0x89, 0x70
#else
	0 b10011111, 0 b10000010, 0 b10111110, 0 b01111111, 0 b00101011,
	    0 b01111101, 0 b11101110, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111101, 0 b11101010, 0 b00000001, 0 b11110111, 0 b10100000,
	    0 b00011111, 0 b10011111, 0 b01110000,
	0 b00011111, 0 b01111100, 0 b10011000, 0 b01111101, 0 b11110100,
	    0 b10000001, 0 b11110111, 0 b10000000,
	0 b00010111, 0 b11011111, 0 b00000000, 0 b00011111, 0 b01111111,
	    0 b10001001, 0 b01110000
#endif
};
#endif

// ----------------------------- General PSoC Vectors--------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
const unsigned int num_bits_init1 = 396;
const unsigned char init1_v[] = {
#ifdef HEX_DEFINE
	0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0D, 0xEE, 0x01, 0xF7, 0xB0, 0x07, 0x9F, 0x07,
	0x5E, 0x7C, 0x81, 0xFD, 0xEA, 0x01, 0xF7, 0xA0,
	0x1F, 0x9F, 0x70, 0x1F, 0x7C, 0x98, 0x7D, 0xF4,
	0x81, 0xF7, 0x80, 0x4F, 0xDF, 0x00, 0x1F, 0x7F,
	0x89, 0x70
#else
	0 b11001010, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000, 0 b00000000,
	    0 b00000000, 0 b00000000, 0 b00000000,
	0 b00001101, 0 b11101110, 0 b00000001, 0 b11110111, 0 b10110000,
	    0 b00000111, 0 b10011111, 0 b00000111,
	0 b01011110, 0 b01111100, 0 b10000001, 0 b11111101, 0 b11101010,
	    0 b00000001, 0 b11110111, 0 b10100000,
	0 b00011111, 0 b10011111, 0 b01110000, 0 b00011111, 0 b01111100,
	    0 b10011000, 0 b01111101, 0 b11110100,
	0 b10000001, 0 b11110111, 0 b10000000, 0 b01001111, 0 b11011111,
	    0 b00000000, 0 b00011111, 0 b01111111,
	0 b10001001, 0 b01110000
#endif
};

const unsigned int num_bits_init2 = 286;
const unsigned char init2_v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7B, 0x00, 0x79, 0xF0, 0x75,
	0xE7, 0xC8, 0x1F, 0xDE, 0xA0, 0x1F, 0x7A, 0x01,
	0xF9, 0xF7, 0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48,
	0x1E, 0x7D, 0x00, 0xFD, 0xE0, 0x0D, 0xF7, 0xC0,
	0x07, 0xDF, 0xE2, 0x5C
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111011, 0 b00000000,
	    0 b01111001, 0 b11110000, 0 b01110101,
	0 b11100111, 0 b11001000, 0 b00011111, 0 b11011110, 0 b10100000,
	    0 b00011111, 0 b01111010, 0 b00000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11001001,
	    0 b10000111, 0 b11011111, 0 b01001000,
	0 b00011110, 0 b01111101, 0 b00000000, 0 b11111101, 0 b11100000,
	    0 b00001101, 0 b11110111, 0 b11000000,
	0 b00000111, 0 b11011111, 0 b11100010, 0 b01011100
#endif
};

const unsigned int num_bits_init3_5v = 836;
const unsigned char init3_5v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7A, 0x01, 0xFD, 0xEA, 0x01,
	0xF7, 0xB0, 0x47, 0xDF, 0x0A, 0x3F, 0x7C, 0xFE,
	0x7D, 0xF4, 0x61, 0xF7, 0xF8, 0x97, 0x00, 0x00,
	0x03, 0x7B, 0x80, 0x7D, 0xE8, 0x07, 0xF7, 0xA8,
	0x07, 0xDE, 0xC1, 0x1F, 0x7C, 0x30, 0x7D, 0xF3,
	0xD5, 0xF7, 0xD1, 0x87, 0xDE, 0xE2, 0x1F, 0x7F,
	0x89, 0x70, 0x00, 0x00, 0x37, 0xB8, 0x07, 0xDE,
	0x80, 0x7F, 0x7A, 0x80, 0x7D, 0xEC, 0x11, 0xF7,
	0xC2, 0x8F, 0xDF, 0x3F, 0xBF, 0x7D, 0x18, 0x7D,
	0xFE, 0x25, 0xC0, 0x00, 0x00, 0xDE, 0xE0, 0x1F,
	0x7A, 0x01, 0xFD, 0xEA, 0x01, 0xF7, 0xB0, 0x47,
	0xDF, 0x0C, 0x1F, 0x7C, 0xF4, 0x7D, 0xF4, 0x61,
	0xF7, 0xB8, 0x87, 0xDF, 0xE2, 0x5C, 0x00, 0x00,
	0x00
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111010, 0 b00000001,
	    0 b11111101, 0 b11101010, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b01000111, 0 b11011111, 0 b00001010,
	    0 b00111111, 0 b01111100, 0 b11111110,
	0 b01111101, 0 b11110100, 0 b01100001, 0 b11110111, 0 b11111000,
	    0 b10010111, 0 b00000000, 0 b00000000,
	0 b00000011, 0 b01111011, 0 b10000000, 0 b01111101, 0 b11101000,
	    0 b00000111, 0 b11110111, 0 b10101000,
	0 b00000111, 0 b11011110, 0 b11000001, 0 b00011111, 0 b01111100,
	    0 b00110000, 0 b01111101, 0 b11110011,
	0 b11010101, 0 b11110111, 0 b11010001, 0 b10000111, 0 b11011110,
	    0 b11100010, 0 b00011111, 0 b01111111,
	0 b10001001, 0 b01110000, 0 b00000000, 0 b00000000, 0 b00110111,
	    0 b10111000, 0 b00000111, 0 b11011110,
	0 b10000000, 0 b01111111, 0 b01111010, 0 b10000000, 0 b01111101,
	    0 b11101100, 0 b00010001, 0 b11110111,
	0 b11000010, 0 b10001111, 0 b11011111, 0 b00111111, 0 b10111111,
	    0 b01111101, 0 b00011000, 0 b01111101,
	0 b11111110, 0 b00100101, 0 b11000000, 0 b00000000, 0 b00000000,
	    0 b11011110, 0 b11100000, 0 b00011111,
	0 b01111010, 0 b00000001, 0 b11111101, 0 b11101010, 0 b00000001,
	    0 b11110111, 0 b10110000, 0 b01000111,
	0 b11011111, 0 b00001100, 0 b00011111, 0 b01111100, 0 b11110100,
	    0 b01111101, 0 b11110100, 0 b01100001,
	0 b11110111, 0 b10111000, 0 b10000111, 0 b11011111, 0 b11100010,
	    0 b01011100, 0 b00000000, 0 b00000000,
	0 b00000000
#endif
};

const unsigned int num_bits_init3_3v = 836;
const unsigned char init3_3v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7A, 0x01, 0xFD, 0xEA, 0x01,
	0xF7, 0xB0, 0x47, 0xDF, 0x0A, 0x3F, 0x7C, 0xFC,
	0x7D, 0xF4, 0x61, 0xF7, 0xF8, 0x97, 0x00, 0x00,
	0x03, 0x7B, 0x80, 0x7D, 0xE8, 0x07, 0xF7, 0xA8,
	0x07, 0xDE, 0xC1, 0x1F, 0x7C, 0x30, 0x7D, 0xF3,
	0xD5, 0xF7, 0xD1, 0x87, 0xDE, 0xE2, 0x1F, 0x7F,
	0x89, 0x70, 0x00, 0x00, 0x37, 0xB8, 0x07, 0xDE,
	0x80, 0x7F, 0x7A, 0x80, 0x7D, 0xEC, 0x11, 0xF7,
	0xC2, 0x8F, 0xDF, 0x3F, 0x3F, 0x7D, 0x18, 0x7D,
	0xFE, 0x25, 0xC0, 0x00, 0x00, 0xDE, 0xE0, 0x1F,
	0x7A, 0x01, 0xFD, 0xEA, 0x01, 0xF7, 0xB0, 0x47,
	0xDF, 0x0C, 0x1F, 0x7C, 0xF4, 0x7D, 0xF4, 0x61,
	0xF7, 0xB8, 0x87, 0xDF, 0xE2, 0x5C, 0x00, 0x00,
	0x00
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111010, 0 b00000001,
	    0 b11111101, 0 b11101010, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b01000111, 0 b11011111, 0 b00001010,
	    0 b00111111, 0 b01111100, 0 b11111100,
	0 b01111101, 0 b11110100, 0 b01100001, 0 b11110111, 0 b11111000,
	    0 b10010111, 0 b00000000, 0 b00000000,
	0 b00000011, 0 b01111011, 0 b10000000, 0 b01111101, 0 b11101000,
	    0 b00000111, 0 b11110111, 0 b10101000,
	0 b00000111, 0 b11011110, 0 b11000001, 0 b00011111, 0 b01111100,
	    0 b00110000, 0 b01111101, 0 b11110011,
	0 b11010101, 0 b11110111, 0 b11010001, 0 b10000111, 0 b11011110,
	    0 b11100010, 0 b00011111, 0 b01111111,
	0 b10001001, 0 b01110000, 0 b00000000, 0 b00000000, 0 b00110111,
	    0 b10111000, 0 b00000111, 0 b11011110,
	0 b10000000, 0 b01111111, 0 b01111010, 0 b10000000, 0 b01111101,
	    0 b11101100, 0 b00010001, 0 b11110111,
	0 b11000010, 0 b10001111, 0 b11011111, 0 b00111111, 0 b00111111,
	    0 b01111101, 0 b00011000, 0 b01111101,
	0 b11111110, 0 b00100101, 0 b11000000, 0 b00000000, 0 b00000000,
	    0 b11011110, 0 b11100000, 0 b00011111,
	0 b01111010, 0 b00000001, 0 b11111101, 0 b11101010, 0 b00000001,
	    0 b11110111, 0 b10110000, 0 b01000111,
	0 b11011111, 0 b00001100, 0 b00011111, 0 b01111100, 0 b11110100,
	    0 b01111101, 0 b11110100, 0 b01100001,
	0 b11110111, 0 b10111000, 0 b10000111, 0 b11011111, 0 b11100010,
	    0 b01011100, 0 b00000000, 0 b00000000,
	0 b00000000
#endif
};

#if 0				//
const unsigned int num_bits_id_setup = 330;
const unsigned char id_setup_v[] = {
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01110000, 0 b00000001,
	    0 b01111101, 0 b11101110, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111101, 0 b11101010, 0 b00000001, 0 b11110111, 0 b10100000,
	    0 b00011111, 0 b10011111, 0 b01110000,
	0 b00011111, 0 b01111100, 0 b10011000, 0 b01111101, 0 b11110100,
	    0 b10000001, 0 b11100111, 0 b11010000,
	0 b00000111, 0 b11011110, 0 b00000000, 0 b11011111, 0 b01111100,
	    0 b00000000, 0 b01111101, 0 b11111110,
	0 b00100101, 0 b11000000
};
#endif
#ifdef ID_SETUP_2
const unsigned int num_bits_id_setup_2 = 418;	//PTJ: id_setup_2 with TSYNC Disable (TSYNC enabled before with SendVector(tsync_enable....)
const unsigned char id_setup_2[] = {
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1f, 0x7f, 0x02, 0x7d, 0xc4, 0x09,
	0xf7, 0x00, 0x1f, 0x9f, 0x07, 0x5e, 0x7c, 0x81,
	0xf9, 0xf4, 0x01, 0xf7, 0xf0, 0x07, 0xdc, 0x40,
	0x1f, 0x70, 0x01, 0xfd, 0xee, 0x01, 0xf7, 0xa0,
	0x1f, 0xde, 0xa0, 0x1f, 0x7b, 0x00, 0x7d, 0xe0,
	0x0d, 0xf7, 0xc0, 0x07, 0xdf, 0x28, 0x1f, 0x7d,
	0x18, 0x7d, 0xfe, 0x25, 0xc0
#else
	0 b11011110, 0 b11100010, 0 b00011111, 0 b01111111, 0 b00000010,
	    0 b01111101, 0 b11000100, 0 b00001001,
	0 b11110111, 0 b00000000, 0 b00011111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111001, 0 b11110100, 0 b00000001, 0 b11110111, 0 b11110000,
	    0 b00000111, 0 b11011100, 0 b01000000,
	0 b00011111, 0 b01110000, 0 b00000001, 0 b11111101, 0 b11101110,
	    0 b00000001, 0 b11110111, 0 b10100000,
	0 b00011111, 0 b11011110, 0 b10100000, 0 b00011111, 0 b01111011,
	    0 b00000000, 0 b01111101, 0 b11100000,
	0 b00001101, 0 b11110111, 0 b11000000, 0 b00000111, 0 b11011111,
	    0 b00101000, 0 b00011111, 0 b01111101,
	0 b00011000, 0 b01111101, 0 b11111110, 0 b00100101, 0 b11000000
#endif
};
#endif

const unsigned int num_bits_erase_all = 308;
const unsigned char erase_all_v[] = {
#ifdef HEX_DEFINE
	0x9F, 0x82, 0xBE, 0x7F, 0x2B, 0x7D, 0xEE, 0x01,
	0xF7, 0xB0, 0x07, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
	0xFD, 0xEA, 0x01, 0xF7, 0xA0, 0x1F, 0x9F, 0x70,
	0x1F, 0x7C, 0x98, 0x7D, 0xF4, 0x81, 0xF7, 0x80,
	0x2F, 0xDF, 0x00, 0x1F, 0x7F, 0x89, 0x70
#else
	0 b10011111, 0 b10000010, 0 b10111110, 0 b01111111, 0 b00101011,
	    0 b01111101, 0 b11101110, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111101, 0 b11101010, 0 b00000001, 0 b11110111, 0 b10100000,
	    0 b00011111, 0 b10011111, 0 b01110000,
	0 b00011111, 0 b01111100, 0 b10011000, 0 b01111101, 0 b11110100,
	    0 b10000001, 0 b11110111, 0 b10000000,
	0 b00101111, 0 b11011111, 0 b00000000, 0 b00011111, 0 b01111111,
	    0 b10001001, 0 b01110000
#endif
};

const unsigned char read_id_v[] = {
#ifdef HEX_DEFINE
	0xBF, 0x00, 0xDF, 0x90, 0x00, 0xFE, 0x60, 0xFF, 0x00
#else
	0 b10111111, 0 b00000000, 0 b11011111, 0 b10010000, 0 b00000000,
	    0 b11111110, 0 b0110000, 0 b11111111, 0 b00000000
#endif
};

const unsigned char Switch_Bank1[] =	//PTJ: use this to switch between register banks
{
#ifdef HEX_DEFINE
	0xde, 0xe2, 0x1c
#else
	0 b11011110, 0 b11100010, 0 b00011100
#endif
};

const unsigned char Switch_Bank0[] =	//PTJ: use this to switch between register banks
{
#ifdef HEX_DEFINE
	0xde, 0xe0, 0x1c
#else
	0 b11011110, 0 b11100000, 0 b00011100
#endif
};

const unsigned char read_IMOtrim[] =	//PTJ: read the 1,E8h register after id__setup_1 to see if the cal data was loaded properly.
{
#ifdef HEX_DEFINE
	0xfd, 0x00, 0x10
#else
	0 b11111101, 0 b00000000, 0 b00010000
#endif
};

const unsigned char read_SPCtrim[] =	//PTJ: read the 1,E7h register after id__setup_1 to see if the cal data was loaded properly.
{
#ifdef HEX_DEFINE
	0xfc, 0xe0, 0x10
#else
	0 b11111100, 0 b11100000, 0 b00010000
#endif
};

const unsigned char read_VBGfinetrim[] =	//PTJ: read the 1,D7h register after id__setup_1 to see if the cal data was loaded properly.
{
#ifdef HEX_DEFINE
	0xfa, 0xe0, 0x08
#else
	0 b11111010, 0 b11100000, 0 b0001000
#endif
};

const unsigned char read_reg_end = 0x80;	//PTJ: this is the final '1' after a MR command

const unsigned char write_byte_start = 0x90;	//PTJ: this is set to SRAM 0x80
const unsigned char write_byte_end = 0xE0;

const unsigned char set_block_number[] = { 0x9F, 0x40, 0xE0 };

const unsigned char set_block_number_end = 0xE0;
#ifdef MULTI_BANK
const unsigned char set_bank_number[] = { 0xDE, 0xE2, 0x1F, 0x7D, 0x00 };
const unsigned char set_bank_number_end[] = { 0xFB, 0xDC, 0x03, 0x80 };
#endif

//    const unsigned char    num_bits_wait_and_poll_end = 40;                   //PTJ 308: commented out
const unsigned char num_bits_wait_and_poll_end = 30;	//PTJ 308: added to match spec
const unsigned char wait_and_poll_end[] = {
//        0x00, 0x00, 0x00, 0x00, 0x00                                                                  //PTJ 308: commented out
	0x00, 0x00, 0x00, 0x00	//PTJ 308: added to match spec
};				// forty '0's per the spec

const unsigned char read_checksum_v[] = {
#ifdef HEX_DEFINE
	0xBF, 0x20, 0xDF, 0x80, 0x80
#else
	0 b10111111, 0 b00100000, 0 b11011111, 0 b10000000, 0 b10000000
#endif
};

const unsigned char read_byte_v[] = {
#ifdef HEX_DEFINE
	0xB0, 0x80
#else
	0 b10110000, 0 b10000000
#endif
};

const unsigned int num_bits_verify_setup = 264;
const unsigned char verify_setup_v[] = {
#ifdef HEX_DEFINE
	0xDE, 0xE0, 0x1F, 0x7B, 0x00, 0x79, 0xF0, 0x75,
	0xE7, 0xC8, 0x1F, 0xDE, 0xA0, 0x1F, 0x7A, 0x01,
	0xF9, 0xF7, 0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48,
	0x1F, 0x78, 0x00, 0xFD, 0xF0, 0x01, 0xF7, 0xF8,
	0x97
#else
	0 b11011110, 0 b11100000, 0 b00011111, 0 b01111011, 0 b00000000,
	    0 b01111001, 0 b11110000, 0 b01110101,
	0 b11100111, 0 b11001000, 0 b00011111, 0 b11011110, 0 b10100000,
	    0 b00011111, 0 b01111010, 0 b00000001,
	0 b11111001, 0 b11110111, 0 b00000001, 0 b11110111, 0 b11001001,
	    0 b10000111, 0 b11011111, 0 b01001000,
	0 b00011111, 0 b01111000, 0 b00000000, 0 b11111101, 0 b11110000,
	    0 b00000001, 0 b11110111, 0 b11111000,
	0 b10010111
#endif
};

const unsigned int num_bits_security = 308;
const unsigned char security_v[] = {
#ifdef HEX_DEFINE
	0x9F, 0x8A, 0x9E, 0x7F, 0x2B, 0x7D, 0xEE, 0x01,
	0xF7, 0xB0, 0x07, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
	0xFD, 0xEA, 0x01, 0xF7, 0xA0, 0x1F, 0x9F, 0x70,
	0x1F, 0x7C, 0x98, 0x7D, 0xF4, 0x81, 0xF7, 0x80,
	0x27, 0xDF, 0x00, 0x1F, 0x7F, 0x89, 0x70
#else
	0 b10011111, 0 b10001010, 0 b10011110, 0 b01111111, 0 b00101011,
	    0 b01111101, 0 b11101110, 0 b00000001,
	0 b11110111, 0 b10110000, 0 b00000111, 0 b10011111, 0 b00000111,
	    0 b01011110, 0 b01111100, 0 b10000001,
	0 b11111101, 0 b11101010, 0 b00000001, 0 b11110111, 0 b10100000,
	    0 b00011111, 0 b10011111, 0 b01110000,
	0 b00011111, 0 b01111100, 0 b10011000, 0 b01111101, 0 b11110100,
	    0 b10000001, 0 b11110111, 0 b10000000,
	0 b00100111, 0 b11011111, 0 b00000000, 0 b00011111, 0 b01111111,
	    0 b10001001, 0 b01110000
#endif
};

#endif				//(INC_ISSP_VECTORS)
#endif				//(PROJECT_REV_)
//end of file ISSP_Vectors.h
