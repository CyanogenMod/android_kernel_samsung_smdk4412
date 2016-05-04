// filename: ISSP_Directives.h
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

----------------------------------------------------------------------------*/

// --------------------- Compiler Directives ----------------------------------
#ifndef INC_ISSP_DIRECTIVES
#define INC_ISSP_DIRECTIVES

// This directive will enable a Genral Purpose test-point on P1.7
// It can be toggled as needed to measure timing, execution, etc...
// A "Test Point" sets a GPIO pin of the host processor high or low. This GPIO
// pin can be observed with an oscilloscope to verify the timing of key
// programming steps. TPs have been added in main() that set Port 0, pin 1
// high during bulk erase, during each block write and during security write.
// The timing of these programming steps should be verified as correct as part
// of the validation process of the final program.

//JBA
//#define USE_TP

// ****************************************************************************
// ************* USER ATTENTION REQUIRED: TARGET SUPPLY VOLTAGE ***************
// ****************************************************************************
// This directive causes the proper Initialization vector #3 to be sent
// to the Target, based on what the Target Vdd programming voltage will
// be. Either 5V (if #define enabled) or 3.3V (if #define disabled).

//JBA
//#define TARGET_VOLTAGE_IS_5V

// ****************************************************************************
// **************** USER ATTENTION REQUIRED: PROGRAMMING MODE *****************
// ****************************************************************************
// This directive selects whether code that uses reset programming mode or code
// that uses power cycle programming is use. Reset programming mode uses the
// external reset pin (XRES) to enter programming mode. Power cycle programming
// mode uses the power-on reset to enter programming mode.
// Applying signals to various pins on the target device must be done in a
// deliberate order when using power cycle mode. Otherwise, high signals to GPIO
// pins on the target will power the PSoC through the protection diodes.

//JBA
// choose the RESET MODE or POWER MODE
//      #define RESET_MODE

// ****************************************************************************
// ****************** USER ATTENTION REQUIRED: TARGET PSOC ********************
// ****************************************************************************
// The directives below enable support for various PSoC devices. The root part
// number to be programmed should be un-commented so that its value becomes
// defined.  All other devices should be commented out.
// Select one device to be supported below:

/*************** CY8CTMA30x, CY8CTMG30x, CY8CTST30x series by KIMC, 2009.08.11 ***********************************/
//#define CY8CTST300_36       // CY8CTST300_36LQXI           // 2009.08.11, not tested.
//#define CY8CTST300_48       // CY8CTST300_48LTXI           // 2009.08.11, not tested.
//#define CY8CTST300_49       // CY8CTST300_49FNXI           // 2009.08.11, not tested.
//#define CY8CTMA300_36       // CY8CTMA300_36LQXI    // 2009.08.11, Test OK.
//#define CY8CTMA300_48       // CY8CTMA300_48LTXI           // 2009.08.11, not tested.
//#define CY8CTMA300_49       // CY8CTMA300_49FNXI           // 2009.08.11, not tested.
//#define CY8CTMG300_36       // CY8CTMG300_36LQXI           // 2009.08.11, not tested.
//#define CY8CTMG300_48       // CY8CTMG300_48LTXI           // 2009.08.11, not tested.
//#define CY8CTMG300_49       // CY8CTMG300_49FNXI           // 2009.08.11, not tested.
//#define CY8CTMG300B_36       // CY8CTMG300B_36LQXI         // 2009.08.11, not tested.
//#define CY8CTMA300B_36       // CY8CTMA300B_36LQXI         // 2009.08.11, not tested.
//#define CY8CTST300B_36       // CY8CTST300B_36LQXI         // 2009.08.11, not tested.
//#define CY8CTMA301_36       // CY8CTMA301_36LQXI           // 2009.08.11, not tested.
//#define CY8CTMA301_48       // CY8CTMA301_48LTXI           // 2009.08.11, not tested.
//#define CY8CTMA301D_36       // CY8CTMA301D_36LQXI         // 2009.08.11, not tested.
//#define CY8CTMA301D_48       // CY8CTMA301D_48LTXI         // 2009.08.11, not tested.
//#define CY8CTMA300D_36       // CY8CTMA300D_36LQXI         // 2009.08.11, not tested.
//#define CY8CTMA300D_48       // CY8CTMA300D_48LTXI         // 2009.08.11, not tested.
//#define CY8CTMA300D_49       // CY8CTMA300D_49FNXIT        // 2009.08.11, not tested.
/****************************************************************************************************/

/*************** CY8CTMG/TST series modified by KJHW, 2009.08.14 *********************************************/
//#define CY8CTMG110
//#define CY8CTST200_24PIN
//#define CY8CTST200_32PIN
//#define CY8CTMG200_24PIN
//#define CY8CTMG200_32PIN
/***************************************************************************************************/

#define CY8C20236
// **** CY8C20x66 devices ****
//#define CY8C20246       /// 2009.03.26. kimc
//#define CY8C20266
//#define CY8C20366
//#define CY8C20466
//#define CY8C20566
//#define CY8C20666
//#define CY8C20066
//#define CY8C200661

// **** CY8C21x23 devices ****
//#define CY8C21123
//#define CY8C21223
//#define CY8C21323
//#define CY8C21002

// **** CY8C21x34 devices ****
//#define CY8C21234
//#define CY8C21334
//#define CY8C21434
//#define CY8C21534
//#define CY8C21634
//#define CY8C21001

// **** CY8C24x23A devices ****
//#define CY8C24123A
//#define CY8C24223A
//#define CY8C24423A
//#define CY8C24000A

// **** CY8C24x94 devices ****
//#define CY8C24794
//#define CY8C24894
//#define CY8C24994
//#define CY8C24094

// **** CY8C27x34 devices ****
//#define CY8C27143
//#define CY8C27243
//#define CY8C27443
//#define CY8C27543
//#define CY8C27643
//#define CY8C27002

// **** CY8C29x66 devices ****
//#define CY8C29466
//#define CY8C29566
//#define CY8C29666
//#define CY8C29866
//#define CY8C29002

//-----------------------------------------------------------------------------
// This section sets the Family that has been selected. These are used to
// simplify other conditional compilation blocks.
//-----------------------------------------------------------------------------

/*************** CY8CTMA30x, CY8CTMG30x, CY8CTST30x series by KIMC, 2009.08.11 ***********************************/
#ifdef CY8CTST300_36		// CY8CTST300_36LQXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTST300_48		// CY8CTST300_48LTXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTST300_49		// CY8CTST300_49FNXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300_36		// CY8CTMA300_36LQXI         // 2009.08.11, test OK
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300_48		// CY8CTMA300_48LTXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300_49		// CY8CTMA300_49FNXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMG300_36		// CY8CTMG300_36LQXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMG300_48		// CY8CTMG300_48LTXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMG300_49		// CY8CTMG300_49FNXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMG300B_36		// CY8CTMG300B_36LQXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300B_36		// CY8CTMA300B_36LQXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTST300B_36		// CY8CTST300B_36LQXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA301_36		// CY8CTMA301_36LQXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA301_48		// CY8CTMA301_48LTXI           // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA301D_36		// CY8CTMA301D_36LQXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA301D_48		// CY8CTMA301D_48LTXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300D_36		// CY8CTMA300D_36LQXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300D_48		// CY8CTMA300D_48LTXI         // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
#ifdef CY8CTMA300D_49		// CY8CTMA300D_49FNXIT        // 2009.08.11, not tested.
#define CY8CTMx30x
#define CY8C20x66
#endif
/**************************************************/

/*************** CY8CTMG/TST series modified by KJHW, 2009.08.14 *********************************************/

#ifdef CY8CTMG110
#define CY8C21x34
#endif
#ifdef CY8CTST200_24PIN
#define CY8C20x66
#endif
#ifdef CY8CTST200_32PIN
#define CY8C20x66
#endif
#ifdef CY8CTMG200_24PIN
#define CY8C20x66
#endif
#ifdef CY8CTMG200_32PIN
#define CY8C20x66
#endif

/***************************************************************************************************/
#ifdef CY8C20236
#define CY8C20x66
#endif
#ifdef CY8C20246		/// 2009.03.26. kimc
#define CY8C20x66
#endif
#ifdef CY8C20266
#define CY8C20x66
#endif
#ifdef CY8C20366
#define CY8C20x66
#endif
#ifdef CY8C20466
#define CY8C20x66
#endif
#ifdef CY8C20566
#define CY8C20x66
#endif
#ifdef CY8C20666
#define CY8C20x66
#endif
#ifdef CY8C20066
#define CY8C20x66
#endif
#ifdef CY8C200661
#define CY8C20x66
#endif

#ifdef CY8C21123
#define CY8C21x23
#endif
#ifdef CY8C21223
#define CY8C21x23
#endif
#ifdef CY8C21323
#define CY8C21x23
#endif
#ifdef CY8C21002
#define CY8C21x23
#endif
#ifdef CY8C21234
#define CY8C21x34
#endif
#ifdef CY8C21334
#define CY8C21x34
#endif
#ifdef CY8C21434
#define CY8C21x34
#endif
#ifdef CY8C21534
#define CY8C21x34
#endif
#ifdef CY8C21634
#define CY8C21x34
#endif
#ifdef CY8C21001
#define CY8C21x34
#endif
#ifdef CY8C24123A
#define CY8C24x23A
#endif
#ifdef CY8C24223A
#define CY8C24x23A
#endif
#ifdef CY8C24423A
#define CY8C24x23A
#endif
#ifdef CY8C24000A
#define CY8C24x23A
#endif
#ifdef CY8C24794
#define CY8C24x94
#endif
#ifdef CY8C24894
#define CY8C24x94
#endif
#ifdef CY8C24994
#define CY8C24x94
#endif
#ifdef CY8C24094
#define CY8C24x94
#endif
#ifdef CY8C27143
#define CY8C27x43
#endif
#ifdef CY8C27243
#define CY8C27x43
#endif
#ifdef CY8C27443
#define CY8C27x43
#endif
#ifdef CY8C27543
#define CY8C27x43
#endif
#ifdef CY8C27643
#define CY8C27x43
#endif
#ifdef CY8C27002
#define CY8C27x43
#endif
#ifdef CY8C29466
#define CY8C29x66
#endif
#ifdef CY8C29566
#define CY8C29x66
#endif
#ifdef CY8C29666
#define CY8C29x66
#endif
#ifdef CY8C29866
#define CY8C29x66
#endif
#ifdef CY8C29002
#define CY8C29x66
#endif

//-----------------------------------------------------------------------------
// The directives below are used for Krypton.
// See the Krypton programming spec 001-15870 rev *A for more details. (The
// spec uses "mnemonics" instead of "directives"
//-----------------------------------------------------------------------------
#ifdef CY8C20x66
#define TSYNC

#define ID_SETUP_1		//PTJ: ID_SETUP_1 is similar to init1_v
#define ID_SETUP_2		//PTJ: ID_SETUP_2 is similar to init2_v
#define SET_BLOCK_NUM
#define CHECKSUM_SETUP		//PTJ: CHECKSUM_SETUP_20x66 is the same as CHECKSUM-SETUP in 001-15870
#define READ_CHECKSUM
#define PROGRAM_AND_VERIFY	//PTJ: PROGRAM_BLOCK_20x66 is the same as PROGRAM-AND-VERIFY in 001-15870
#define ERASE
#define	SECURE
#define READ_SECURITY
#define READ_WRITE_SETUP
#define WRITE_BYTE
#define VERIFY_SETUP
#define READ_STATUS
#define READ_BYTE
	//READ_ID_WORD                                          //PTJ: 3rd Party Progrmmer will have to write code to handle this directive, we do it out own way in this code, see read_id_v
#endif
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// The directives below are used to define various sets of vectors that differ
// for more than one set of PSoC parts.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****
#ifdef CY8C21x23
#define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C21x34
#define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C24x23A
#define CHECKSUM_SETUP_24_24A
#endif
#ifdef CY8C24x94
#define CHECKSUM_SETUP_24_29
#endif
#ifdef CY8C27x43
#define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C29x66
#define CHECKSUM_SETUP_24_29
#endif

// **** Select a Program Block Vector ****

#ifdef CY8C21x23
#define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C21x34
#define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x23A
#define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x94
#define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C27x43
#define PROGRAM_BLOCK_27
#endif
#ifdef CY8C29x66
#define PROGRAM_BLOCK_21_24_29
#endif

//-----------------------------------------------------------------------------
// The directives below are used to control switching banks if the device is
// has multiple banks of Flash.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****
#ifdef CY8C24x94
#define MULTI_BANK
#endif
#ifdef CY8C29x66
#define MULTI_BANK
#endif

// ----------------------------------------------------------------------------
#endif				//(INC_ISSP_DIRECTIVES)
#endif				//(PROJECT_REV_)
//end of file ISSP_Directives.h
