// filename: ISSP_Routines.c
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
 Cypressï¿?product in a life-support systems application implies that the
 manufacturer assumes all risk of such use and in doing so indemnifies
 Cypress against all charges.

 Use may be limited by and subject to the applicable Cypress software
 license agreement.

--------------------------------------------------------------------------*/
#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/earlysuspend.h>
#include <asm/io.h>
#include <linux/hrtimer.h>
//mhsong #include <m8c.h>        // part specific constants and macros
//mhsong #include "PSoCAPI.h"    // PSoC API definitions for all User Modules
#include "issp_defs.h"
#include "issp_vectors.h"
#include "issp_extern.h"
#include "issp_errors.h"
#include "issp_directives.h"
#include "issp_delays.h"

unsigned char bTargetDataIN;
unsigned char abTargetDataOUT[TARGET_DATABUFF_LEN];

unsigned char bTargetAddress;
unsigned char bTargetDataPtr = 0;
unsigned char bTargetID[10];
unsigned char bTargetStatus[10];	//PTJ: created to support READ-STATUS in fReadStatus()

unsigned char fIsError;

/* ((((((((((((((((((((( LOW-LEVEL ISSP SUBROUTINE SECTION ))))))))))))))))))))
   (( The subroutines in this section use functions from the C file          ))
   (( ISSP_Drive_Routines.c. The functions in that file interface to the     ))
   (( processor specific hardware. So, these functions should work as is, if ))
   (( the routines in ISSP_Drive_Routines.c are correctly converted.         ))
   (((((((((((((((((((((((((((((((((((())))))))))))))))))))))))))))))))))))))))*/

// ============================================================================
// RunClock()
// Description:
// Run Clock without sending/receiving bits. Use this when transitioning from
// write to read and read to write "num_cycles" is number of SCLK cycles, not
// number of counter cycles.
//
// SCLK cannot run faster than the specified maximum frequency of 8MHz. Some
// processors may need to have delays added after setting SCLK low and setting
// SCLK high in order to not exceed this specification. The maximum frequency
// of SCLK should be measured as part of validation of the final program
//
// ============================================================================
void RunClock(unsigned int iNumCycles)
{
	int i;

	for (i = 0; i < iNumCycles; i++) {
		SCLKLow();
		SCLKHigh();
	}
	// function exits with CLK high.
}

// ============================================================================
// bReceiveBit()
// Clocks the SCLK pin (high-low-high) and reads the status of the SDATA pin
// after the rising edge.
//
// SCLK cannot run faster than the specified maximum frequency of 8MHz. Some
// processors may need to have delays added after setting SCLK low and setting
// SCLK high in order to not exceed this specification. The maximum frequency
// of SCLK should be measured as part of validation of the final program
//
// Returns:
//     0 if SDATA was low
//     1 if SDATA was high
// ============================================================================
unsigned char bReceiveBit(void)
{
	SCLKLow();
	SCLKHigh();
	if (fSDATACheck()) {
		return (1);
	} else {
		return (0);
	}
}

// ============================================================================
// bReceiveByte()
// Calls ReceiveBit 8 times to receive one byte.
// Returns:
//     The 8-bit values recieved.
// ============================================================================
unsigned char bReceiveByte(void)
{
	unsigned char b;
	unsigned char bCurrByte = 0x00;

	for (b = 0; b < 8; b++) {
		bCurrByte = (bCurrByte << 1) + bReceiveBit();
	}
	return (bCurrByte);
}

// ============================================================================
// SendByte()
// This routine sends up to one byte of a vector, one bit at a time.
//    bCurrByte   the byte that contains the bits to be sent.
//    bSize       the number of bits to be sent. Valid values are 1 to 8.
//
// SCLK cannot run faster than the specified maximum frequency of 8MHz. Some
// processors may need to have delays added after setting SCLK low and setting
// SCLK high in order to not exceed this specification. The maximum frequency
// of SCLK should be measured as part of validation of the final program
//
// There is no returned value.
// ============================================================================
void SendByte(unsigned char bCurrByte, unsigned char bSize)
{
	unsigned char b = 0;

	for (b = 0; b < bSize; b++) {
		if (bCurrByte & 0x80) {
			// Send a '1'
			SetSDATAHigh();
			SCLKHigh();
			SCLKLow();
		} else {
			// Send a '0'
			SetSDATALow();
			SCLKHigh();
			SCLKLow();
		}
		bCurrByte = bCurrByte << 1;
	}
}

// ============================================================================
// SendVector()
// This routine sends the vector specifed. All vectors constant strings found
// in ISSP_Vectors.h.  The data line is returned to HiZ after the vector is
// sent.
//    bVect      a pointer to the vector to be sent.
//    nNumBits   the number of bits to be sent.
//    bCurrByte  scratch var to keep the byte to be sent.
//
// There is no returned value.
// ============================================================================
void SendVector(const unsigned char *bVect, unsigned int iNumBits)
{
	SetSDATAStrong();
	while (iNumBits > 0) {
		if (iNumBits >= 8) {
			SendByte(*(bVect), 8);
			iNumBits -= 8;
			bVect++;
		} else {
			SendByte(*(bVect), iNumBits);
			iNumBits = 0;
		}
	}
	SetSDATALow();		// issp_test_20100709 add
	SetSDATAHiZ();
}

// ============================================================================
// fDetectHiLoTransition()
// Waits for transition from SDATA = 1 to SDATA = 0.  Has a 100 msec timeout.
// TRANSITION_TIMEOUT is a loop counter for a 100msec timeout when waiting for
// a high-to-low transition. This is used in the polling loop of
// fDetectHiLoTransition(). The timing of the while(1) loops can be calculated
// and the number of loops is counted, using iTimer, to determine when 100
// msec has passed.
//
//// SCLK cannot run faster than the specified maximum frequency of 8MHz. Some
// processors may need to have delays added after setting SCLK low and setting
// SCLK high in order to not exceed this specification. The maximum frequency
// of SCLK should be measured as part of validation of the final program
//
// Returns:
//     0 if successful
//    -1 if timed out.
// ============================================================================
signed char fDetectHiLoTransition(void)
{
	// nTimer breaks out of the while loops if the wait in the two loops totals
	// more than 100 msec.  Making this static makes the loop run a faster.
	// This is really a processor/compiler dependency and it not needed.
	static unsigned int iTimer;

	// NOTE:
	// These loops look unconventional, but it is necessary to check SDATA_PIN
	// as shown because the transition can be missed otherwise, due to the
	// length of the SDATA Low-High-Low after certain commands.

	// Generate clocks for the target to pull SDATA High
	//dog_kick();
	iTimer = TRANSITION_TIMEOUT;
	printk(KERN_DEBUG
	       "Generate clocks for the target to pull SDATA High\n");
	while (1) {
		SCLKLow();
		if (fSDATACheck())	// exit once SDATA goes HI
			break;
		SCLKHigh();
		// If the wait is too long then timeout
		if (iTimer-- == 0) {
			return (ERROR);
		}
	}
	//dog_kick();
	// Generate Clocks and wait for Target to pull SDATA Low again
	iTimer = TRANSITION_TIMEOUT;	// reset the timeout counter
	printk(KERN_DEBUG
	       "Generate Clocks and wait for Target to pull SDATA Low again\n");
	while (1) {
		SCLKLow();	//issp_test_20100709 unblock
		if (!fSDATACheck()) {	// exit once SDATA returns LOW
			break;
		}
		SCLKHigh();	//issp_test_20100709 unblock
		// If the wait is too long then timeout
		if (iTimer-- == 0) {
			return (ERROR);
		}
	}
	printk("fDetectHiLoTransition OUT!!!!\n");
	return (PASS);
}

/* ((((((((((((((((((((( HIGH-LEVEL ISSP ROUTINE SECTION ))))))))))))))))))))))
   (( These functions are mostly made of calls to the low level routines     ))
   (( above.  This should isolate the processor-specific changes so that     ))
   (( these routines do not need to be modified.                             ))
   (((((((((((((((((((((((((((((((((((())))))))))))))))))))))))))))))))))))))))*/

#ifdef RESET_MODE
// ============================================================================
// fXRESInitializeTargetForISSP()
// Implements the intialization vectors for the device.
// Returns:
//     0 if successful
//     INIT_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fXRESInitializeTargetForISSP(void)
{
	// Configure the pins for initialization
	SetSDATAHiZ();
	SetSCLKStrong();
	SCLKLow();
	// Cycle reset and put the device in programming mode when it exits reset
	AssertXRES();
	DeassertXRES();
	// !!! NOTE:
	//  The timing spec that requires that the first Init-Vector happen within
	//  1 msec after the reset/power up. For this reason, it is not advisable
	//  to separate the above RESET_MODE or POWER_CYCLE_MODE code from the
	//  Init-Vector instructions below. Doing so could introduce excess delay
	//  and cause the target device to exit ISSP Mode.

	//PTJ: Send id_setup_1 instead of init1_v
	//PTJ: both send CA Test Key and do a Calibrate1 SROM function
	SendVector(id_setup_1, num_bits_id_setup_1);
	if (fIsError = fDetectHiLoTransition()) {
//        TX8SW_CPutString("\r\n fDetectHiLoTransition Error");
		printk("\r\n fDetectHiLoTransition Error\n");
		return (INIT_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

	// NOTE: DO NOT not wait for HiLo on SDATA after vector Init-3
	//       it does not occur (per spec).
	return (PASS);
}
#else				//else = the part is power cycle programmed

// ============================================================================
// fPowerCycleInitializeTargetForISSP()
// Implements the intialization vectors for the device.
// The first time fDetectHiLoTransition is called the Clk pin is highZ because
// the clock is not needed during acquire.
// Returns:
//     0 if successful
//     INIT_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fPowerCycleInitializeTargetForISSP(void)
{
	// unsigned char n;

	// Set all pins to highZ to avoid back powering the PSoC through the GPIO
	// protection diodes.
	SetSCLKHiZ();
	SetSDATAHiZ();

	// Turn on power to the target device before other signals
	SetTargetVDDStrong();
	ApplyTargetVDD();
	// wait 1msec for the power to stabilize
#if 0
	for (n = 0; n < 10; n++) {
		Delay(DELAY100us);
	}
#endif
	// Set SCLK to high Z so there is no clock and wait for a high to low
	// transition on SDAT. SCLK is not needed this time.
	SetSCLKHiZ();
//    printk(KERN_DEBUG "fDetectHiLoTransition\n");
	if ((fIsError = fDetectHiLoTransition())) {
		return (INIT_ERROR);
	}
	// Configure the pins for initialization
//    SetSDATAHiZ(); // issp_test_20100709 block
	SetSCLKStrong();
	SCLKLow();		//PTJ: DO NOT SET A BREAKPOINT HERE AND EXPECT SILICON ID TO PASS!

	// !!! NOTE:
	//  The timing spec that requires that the first Init-Vector happen within
	//  1 msec after the reset/power up. For this reason, it is not advisable
	//  to separate the above RESET_MODE or POWER_CYCLE_MODE code from the
	//  Init-Vector instructions below. Doing so could introduce excess delay
	//  and cause the target device to exit ISSP Mode.

	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);	//PTJ: rev308, added to match spec
//    printk("SendVector(id_setup_1)\n",0,0,0);
	SendVector(id_setup_1, num_bits_id_setup_1);
	if ((fIsError = fDetectHiLoTransition())) {
		return (INIT_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

	// NOTE: DO NOT not wait for HiLo on SDATA after vector Init-3
	//       it does not occur (per spec).
	return (PASS);
}
#endif

// ============================================================================
// fVerifySiliconID()
// Returns:
//     0 if successful
//     Si_ID_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fVerifySiliconID(void)
{
	SendVector(id_setup_2, num_bits_id_setup_2);
	printk("fVerifySiliconID: SendVector id_stup2 END\n");

	if ((fIsError = fDetectHiLoTransition())) {
		printk("fVerifySiliconID(): fDetectHiLoTransition Error\n");
		return (SiID_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);
	SendVector(tsync_enable, num_bits_tsync_enable);
	printk
	    ("fVerifySiliconID: SendVector(wait_and_poll_end) (tsync_enable) END\n");

	//Send Read ID vector and get Target ID
	SendVector(read_id_v, 11);	// Read-MSB Vector is the first 11-Bits
	RunClock(2);		// Two SCLK cycles between write & read
	bTargetID[0] = bReceiveByte();
	RunClock(1);
	SendVector(read_id_v + 2, 12);	// 1+11 bits starting from the 3rd byte

	RunClock(2);		// Read-LSB Command
	bTargetID[1] = bReceiveByte();

	RunClock(1);
	SendVector(read_id_v + 4, 1);	// 1 bit starting from the 5th byte

	//read Revision ID from Accumulator A and Accumulator X
	SendVector(read_id_v + 5, 11);	//11 bits starting from the 6th byte
	RunClock(2);
	bTargetID[2] = bReceiveByte();	//Read from Acc.X
	RunClock(1);
	SendVector(read_id_v + 7, 12);	//1+11 bits starting from the 8th byte

	RunClock(2);
	bTargetID[3] = bReceiveByte();	//Read from Acc.A

	RunClock(1);
	SendVector(read_id_v + 4, 1);	//1bit starting from the 5th byte,

	SendVector(tsync_disable, num_bits_tsync_disable);

	// Print READ-ID
	/*
	   TX8SW_CPutString("\r\n Silicon-ID : ");
	   TX8SW_PutChar(' ');
	   TX8SW_PutSHexByte(bTargetID[0]);
	   TX8SW_PutChar(' ');
	   TX8SW_PutSHexByte(bTargetID[1]);
	   TX8SW_PutChar(' ');
	   TX8SW_PutSHexByte(bTargetID[2]);
	   TX8SW_PutChar(' ');
	   TX8SW_PutSHexByte(bTargetID[3]);
	   TX8SW_PutChar(' ');
	 */
#if 0				// issp_test_20100709 block
	printk("issp_routines.c: ID0:0x%X, ID1:0x%X, ID2: 0x%X, ID2: 0x%X\n",
	       bTargetID[0], bTargetID[1], bTargetID[2], bTargetID[3]);

	if ((bTargetID[0] != target_id_v[0]) || (bTargetID[1] != target_id_v[1])
	    || (bTargetID[2] != target_id_v[2])
	    || (bTargetID[3] != target_id_v[3])) {
		return (SiID_ERROR);
	} else {
		return (PASS);
	}
#else
	return (PASS);

#endif
}

// PTJ: =======================================================================
// fReadStatus()
// Returns:
//     0 if successful
//     _____ if timed out on handshake to the device.
// ============================================================================
signed char fReadStatus(void)
{
	SendVector(tsync_enable, num_bits_tsync_enable);	//PTJ:

	//Send Read ID vector and get Target ID
	SendVector(read_id_v, 11);	// Read-MSB Vector is the first 11-Bits
	RunClock(2);		// Two SCLK cycles between write & read
	bTargetStatus[0] = bReceiveByte();
	RunClock(1);
	//SendVector(read_id_v+2, 12);    // 12 bits starting from the 3rd character

	//RunClock(2);                    // Read-LSB Command
	//bTargetStatus[1] = bReceiveByte();

	//RunClock(1);
	SendVector(read_id_v + 4, 1);	// 1 bit starting from the 5th character

	SendVector(tsync_disable, num_bits_tsync_disable);

	if (bTargetStatus[0] == target_status00_v) {
		return (PASS);	//PTJ: Status = 00 means Success, the SROM function did what it was supposed to
	}
	if (bTargetStatus[0] == target_status01_v) {
		return (STATUS_ERROR);	//PTJ: Status = 01 means that function is not allowed because of block level protection, for test with verify_setup (VERIFY-SETUP)
	}
	if (bTargetStatus[0] == target_status03_v) {
		return (STATUS_ERROR);	//PTJ: Status = 03 is fatal error, SROM halted
	}
	if (bTargetStatus[0] == target_status04_v) {
		return (STATUS_ERROR);	//PTJ: Status = 04 means there was a checksum faliure with either the smart write code checksum, or the smart write paramters checksum, for test with PROGRAM-AND-VERIFY
	}
	if (bTargetStatus[0] == target_status06_v) {
		return (STATUS_ERROR);	//PTJ: Status = 06 means that Calibrate1 failed, for test with id_setup_1 (ID-SETUP-1)
	} else {
		return (STATUS_ERROR);
	}
}

// PTJ: =======================================================================
// fReadCalRegisters()
// PTJ:  use this to read some cal registers that should be loaded by Calibrate1 in id_setup_1
// Returns:
//     0 if successful
//     _____ if timed out on handshake to the device.
// ============================================================================
signed char fReadCalRegisters(void)
{
	SendVector(tsync_enable, num_bits_tsync_enable);

	SendVector(Switch_Bank1, 22);

	SendVector(read_IMOtrim, 11);	// Read-MSB Vector is the first 11-Bits
	RunClock(2);		// Two SCLK cycles between write & read
	bTargetStatus[0] = bReceiveByte();
	RunClock(1);
	// Set SDATA to Strong Drive here because SendByte() does not
	SetSDATAStrong();
	SendByte(read_reg_end, 1);

	SendVector(read_SPCtrim, 11);	// Read-MSB Vector is the first 11-Bits
	RunClock(2);		// Two SCLK cycles between write & read
	bTargetStatus[1] = bReceiveByte();
	RunClock(1);
	// Set SDATA to Strong Drive here because SendByte() does not
	SetSDATAStrong();
	SendByte(read_reg_end, 1);

	SendVector(read_VBGfinetrim, 11);	// Read-MSB Vector is the first 11-Bits
	RunClock(2);		// Two SCLK cycles between write & read
	bTargetStatus[2] = bReceiveByte();
	RunClock(1);
	// Set SDATA to Strong Drive here because SendByte() does not
	SetSDATAStrong();
	SendByte(read_reg_end, 1);

	SendVector(Switch_Bank0, 22);

	SendVector(tsync_disable, num_bits_tsync_disable);

	if (bTargetStatus[0] == target_status00_v) {
		return (PASS);	//PTJ: Status = 00 means Success, the SROM function did what it was supposed to
	}
	return PASS;
}

// PTJ: =======================================================================
// fReadWriteSetup()
// PTJ: The READ-WRITE-SETUP vector will enable TSYNC and switches the device
//              to SRAM bank1 for PROGRAM-AND-VERIFY, SECURE and VERIFY-SETUP.
// Returns:
//     0 if successful
//     _____ if timed out on handshake to the device.
// ============================================================================
signed char fReadWriteSetup(void)
{
	SendVector(read_write_setup, num_bits_read_write_setup);
	return (PASS);		//PTJ: is there anything else that should be done?
}

// PTJ: =======================================================================
// fSyncEnable()
// PTJ: The SYNC-ENABLE vector will enable TSYNC
//
// Returns:
//     0 if successful
//     _____ if timed out on handshake to the device.
// ============================================================================
signed char fSyncEnable(void)
{
	SendVector(tsync_enable, num_bits_tsync_enable);	//PTJ: 307 for tsync enable testing
	return (PASS);		//PTJ: is there anything else that should be done?
}

// PTJ: =======================================================================
// fSyncDisable()
// PTJ: The SYNC-ENABLE vector will enable TSYNC
//
// Returns:
//     0 if successful
//     _____ if timed out on handshake to the device.
// ============================================================================
signed char fSyncDisable(void)
{
	SendVector(tsync_disable, num_bits_tsync_disable);	//PTJ: 307 for tsync enable testing
	return (PASS);
}

// ============================================================================
// fEraseTarget()
// Perform a bulk erase of the target device.
// Returns:
//     0 if successful
//     ERASE_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fEraseTarget(void)
{
	SendVector(erase, num_bits_erase);
	if ((fIsError = fDetectHiLoTransition())) {
//        TX8SW_CPutString("\r\n fDetectHiLoTransition");
		//printk("\r\n fDetectHiLoTransition\n"); // issp_test_2010 block
		return (ERASE_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);
	return (PASS);
}

extern unsigned int iBlockCounter;
// ============================================================================
// LoadTarget()
// Transfers data from array in Host to RAM buffer in the target.
// Returns the checksum of the data.
// ============================================================================
unsigned int iLoadTarget(void)
{
	unsigned char bTemp;
	unsigned int iChecksumData = 0;

	// Set SDATA to Strong Drive here because SendByte() does not
	SetSDATAStrong();

	// Transfer the temporary RAM array into the target.
	// In this section, a 128-Byte array was specified by #define, so the entire
	// 128-Bytes are written in this loop.
	bTargetAddress = 0x00;
	bTargetDataPtr = 0x00;

	while (bTargetDataPtr < TARGET_DATABUFF_LEN) {
		bTemp = abTargetDataOUT[bTargetDataPtr];
		iChecksumData += bTemp;

		SendByte(write_byte_start, 4);	//PTJ: we need to be able to write 128 bytes from address 0x80 to 0xFF
		SendByte(bTargetAddress, 7);	//PTJ: we need to be able to write 128 bytes from address 0x80 to 0xFF
		SendByte(bTemp, 8);
		SendByte(write_byte_end, 3);

		// !!!NOTE:
		// SendByte() uses MSbits, so inc by '2' to put the 0..128 address into
		// the seven MSBit locations.
		//
		// This can be confusing, but check the logic:
		//   The address is only 7-Bits long. The SendByte() subroutine will
		// send however-many bits, BUT...always reads them bits from left-to-
		// right. So in order to pass a value of 0..128 as the address using
		// SendByte(), we have to left justify the address by 1-Bit.
		//   This can be done easily by incrementing the address each time by
		// '2' rather than by '1'.

		bTargetAddress += 2;	//PTJ: inc by 2 in order to support a 128 byte address space, MSB~1 for address
		bTargetDataPtr++;
	}

	return (iChecksumData);
}

#ifdef MULTI_BANK
// ============================================================================
// SetBankNumber()
// Set the bank number in the target device.
// Returns:
//     none
// ============================================================================
void SetBankNumber(unsigned char bBankNumber)
{
	// Send the bank-select vector.
	SendVector(set_bank_number, 33);

	// Set the drive here because SendByte() does not.
	SetSDATAStrong();
	SendByte(bBankNumber, 8);
	SendVector(set_bank_number_end, 25);
}
#endif

// ============================================================================
// fProgramTargetBlock()
// Program one block with data that has been loaded into a RAM buffer in the
// target device.
// Returns:
//     0 if successful
//     BLOCK_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fProgramTargetBlock(unsigned char bBankNumber,
				unsigned char bBlockNumber)
{

	SendVector(tsync_enable, num_bits_tsync_enable);

	SendVector(set_block_num, num_bits_set_block_num);

	// Set the drive here because SendByte() does not.
	SetSDATAStrong();
	SendByte(bBlockNumber, 8);
	SendByte(set_block_num_end, 3);

	SendVector(tsync_disable, num_bits_tsync_disable);	//PTJ:

	// Send the program-block vector.
	SendVector(program_and_verify, num_bits_program_and_verify);	//PTJ: PROGRAM-AND-VERIFY
	// wait for acknowledge from target.
	if ((fIsError = fDetectHiLoTransition())) {
		return (BLOCK_ERROR);
	}
	// Send the Wait-For-Poll-End vector
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);
	return (PASS);

	//PTJ: Don't do READ-STATUS here because that will
	//PTJ: require that we return multiple error values, if error occurs
}

// ============================================================================
// fAddTargetBankChecksum()
// Reads and adds the target bank checksum to the referenced accumulator.
// Returns:
//     0 if successful
//     VERIFY_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fAccTargetBankChecksum(unsigned int *pAcc)
{
	unsigned int wCheckSumData;

	SendVector(checksum_setup, num_bits_checksum_setup);	//PTJ:CHECKSUM-SETUP, it is taking 100ms > time > 200ms to complete the checksum
	if ((fIsError = fDetectHiLoTransition())) {	//100ms is default
		return (VERIFY_ERROR);
	}

	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

	SendVector(tsync_enable, num_bits_tsync_enable);

	//Send Read Checksum vector and get Target Checksum
	SendVector(read_checksum_v, 11);	// first 11-bits is ReadCKSum-MSB
	RunClock(2);		// Two SCLKs between write & read
	bTargetDataIN = bReceiveByte();
	wCheckSumData = bTargetDataIN << 8;

	RunClock(1);		// See Fig. 6
	SendVector(read_checksum_v + 2, 12);	// 12 bits starting from 3rd character
	RunClock(2);		// Read-LSB Command
	bTargetDataIN = bReceiveByte();
	wCheckSumData |= (bTargetDataIN & 0xFF);
	RunClock(1);
	SendVector(read_checksum_v + 3, 1);	// Send the final bit of the command   //PTJ: read_checksum_v may have to change if TSYNC needs to be enabled

	SendVector(tsync_disable, num_bits_tsync_disable);

	*pAcc = wCheckSumData;

	return (PASS);
}

// ============================================================================
// ReStartTarget()
// After programming, the target PSoC must be reset to take it out of
// programming mode. This routine performs a reset.
// ============================================================================
void ReStartTarget(void)
{
#ifdef RESET_MODE
	// Assert XRES, then release, then disable XRES-Enable
	AssertXRES();
	Delay(XRES_CLK_DELAY);
	DeassertXRES();
#else
	// Set all pins to highZ to avoid back powering the PSoC through the GPIO
	// protection diodes.
	SetSCLKHiZ();
	SetSDATAHiZ();
	// Cycle power on the target to cause a reset
	RemoveTargetVDD();
	mdelay(300);
	ApplyTargetVDD();
#endif
}

// ============================================================================
// fVerifySetup()
// Verify the block just written to. This can be done byte-by-byte before the
// protection bits are set.
// Returns:
//     0 if successful
//     BLOCK_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fVerifySetup(unsigned char bBankNumber, unsigned char bBlockNumber)
{
	SendVector(tsync_enable, num_bits_tsync_enable);

	SendVector(set_block_num, num_bits_set_block_num);

	//Set the drive here because SendByte() does not
	SetSDATAStrong();
	SendByte(bBlockNumber, 8);
	SendByte(set_block_num_end, 3);	//PTJ:

	SendVector(tsync_disable, num_bits_tsync_disable);	//PTJ:

	SendVector(verify_setup, num_bits_my_verify_setup);	//PTJ:
	if ((fIsError = fDetectHiLoTransition())) {
		return (BLOCK_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

	return (PASS);
}

// ============================================================================
// fReadByteLoop()
// Reads the data back from Target SRAM and compares it to expected data in
// Host SRAM
// Returns:
//     0 if successful
//     BLOCK_ERROR if timed out on handshake to the device.
// ============================================================================

signed char fReadByteLoop(void)
{
	bTargetAddress = 0;
	bTargetDataPtr = 0;

	while (bTargetDataPtr < TARGET_DATABUFF_LEN) {
		//Send Read Byte vector and then get a byte from Target
		SendVector(read_byte_v, 4);
		// Set the drive here because SendByte() does not
		SetSDATAStrong();
		SendByte(bTargetAddress, 7);

		RunClock(2);	// Run two SCLK cycles between writing and reading
		SetSDATAHiZ();	// Set to HiZ so Target can drive SDATA
		bTargetDataIN = bReceiveByte();

		RunClock(1);
		SendVector(read_byte_v + 1, 1);	// Send the ReadByte Vector End

		// Test the Byte that was read from the Target against the original
		// value (already in the 128-Byte array "abTargetDataOUT[]"). If it
		// matches, then bump the address & pointer,loop-back and continue.
		// If it does NOT match abort the loop and return and error.
		if (bTargetDataIN != abTargetDataOUT[bTargetDataPtr]) {
#ifdef TX_ON
			UART_PutCRLF();
			UART_CPutString("bTargetDataIN : ");
			UART_PutHexByte(bTargetDataIN);
			UART_CPutString(" abTargetDataOUT : ");
			UART_PutHexByte(abTargetDataOUT[bTargetDataPtr]);
#endif
			return (BLOCK_ERROR);
		}

		bTargetDataPtr++;
		// Increment the address by 2 to accomodate 7-Bit addressing
		// (puts the 7-bit address into MSBit locations for "SendByte()").
		bTargetAddress += 2;

	}

	return (PASS);
}

// ============================================================================
// fVerifyTargetBlock()
// Verify the block just written to. This can be done byte-by-byte before the
// protection bits are set.
// Returns:
//     0 if successful
//     BLOCK_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fVerifyTargetBlock(unsigned char bBankNumber,
			       unsigned char bBlockNumber)
{
	SendVector(set_block_number, 11);

	//Set the drive here because SendByte() does not
	SetSDATAStrong();
	SendByte(bBlockNumber, 8);
	SendByte(set_block_number_end, 3);

	SendVector(verify_setup_v, num_bits_verify_setup);
	if ((fIsError = fDetectHiLoTransition())) {
		return (BLOCK_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

	bTargetAddress = 0;
	bTargetDataPtr = 0;

	while (bTargetDataPtr < TARGET_DATABUFF_LEN) {
		//Send Read Byte vector and then get a byte from Target
		SendVector(read_byte_v, 4);	//PTJ 308: this was changed from sending the first 5 bits to sending the first 4
		// Set the drive here because SendByte() does not
		SetSDATAStrong();
		SendByte(bTargetAddress, 6);

		RunClock(2);	// Run two SCLK cycles between writing and reading
		SetSDATAHiZ();	// Set to HiZ so Target can drive SDATA
		bTargetDataIN = bReceiveByte();

		RunClock(1);
		SendVector(read_byte_v + 1, 1);	// Send the ReadByte Vector End

		// Test the Byte that was read from the Target against the original
		// value (already in the 128-Byte array "abTargetDataOUT[]"). If it
		// matches, then bump the address & pointer,loop-back and continue.
		// If it does NOT match abort the loop and return an error.
		if (bTargetDataIN != abTargetDataOUT[bTargetDataPtr])
			return (BLOCK_ERROR);

		bTargetDataPtr++;
		// Increment the address by four to accomodate 6-Bit addressing
		// (puts the 6-bit address into MSBit locations for "SendByte()").
		bTargetAddress += 4;
	}
	return (PASS);
}

// ============================================================================
// fSecureTargetFlash()
// Before calling, load the array, abTargetDataOUT, with the desired security
// settings using LoadArrayWithSecurityData(StartAddress,Length,SecurityType).
// The can be called multiple times with different SecurityTypes as needed for
// particular Flash Blocks. Or set them all the same using the call below:
// LoadArrayWithSecurityData(0,SECURITY_BYTES_PER_BANK, 0);
// Returns:
//     0 if successful
//     SECURITY_ERROR if timed out on handshake to the device.
// ============================================================================
signed char fSecureTargetFlash(void)
{
	unsigned char bTemp;

	// Transfer the temporary RAM array into the target
	bTargetAddress = 0x00;
	bTargetDataPtr = 0x00;

	SetSDATAStrong();
	while (bTargetDataPtr < SECURITY_BYTES_PER_BANK) {
		bTemp = abTargetDataOUT[bTargetDataPtr];
		SendByte(write_byte_start, 4);
		SendByte(bTargetAddress, 7);
		SendByte(bTemp, 8);
		SendByte(write_byte_end, 3);

		// SendBytes() uses MSBits, so increment the address by '2' to put
		// the 0..n address into the seven MSBit locations
		bTargetAddress += 2;	//PTJ: inc by 2 in order to support a 128 byte address space
		bTargetDataPtr++;
	}

	SendVector(secure, num_bits_secure);	//PTJ:
	if ((fIsError = fDetectHiLoTransition())) {
		return (SECURITY_ERROR);
	}
	SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);
	return (PASS);
}

// ============================================================================
// PTJ: fReadSecurity()
// This reads from SM0 with Read Supervisory SPC command.
// Need to have SPC Test Mode enabled before using these commands?
// Returns:
//     0 if successful
//     __________ if timed out on handshake to the device.
// ============================================================================
signed char fReadSecurity(void)
{
	SendVector(ReadSecuritySetup, num_bits_ReadSecuritySetup);
//      SendVector(SPCTestMode_enable, num_bits_SPCTestMode_enable);

	bTargetAddress = 0x00;
	while (bTargetAddress < (SECURITY_BYTES_PER_BANK * 2)) {	//PTJ: we do SECURITY_BYTES_PER_BANK * 2 because we bTargetAddress += 2

		//PTJ: TSYNC Enable
		SendVector(tsync_enable, num_bits_tsync_enable);

		SendVector(read_security_pt1, num_bits_read_security_pt1);	//PTJ:
		// Set the drive here because SendByte() does not.
		SetSDATAStrong();
		SendByte(bTargetAddress, 7);	//PTJ: hardcode MSb of address as 0 in bit stream
		SendVector(read_security_pt1_end,
			   num_bits_read_security_pt1_end);

		//PTJ: TSYNC Disable
		SendVector(tsync_disable, num_bits_tsync_disable);

		SendVector(read_security_pt2, num_bits_read_security_pt2);

		SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

		SendVector(read_security_pt3, num_bits_read_security_pt3);

		SetSDATAStrong();
		SendByte(bTargetAddress, 7);

		SendVector(read_security_pt3_end,
			   num_bits_read_security_pt3_end);

		SendVector(wait_and_poll_end, num_bits_wait_and_poll_end);

		bTargetAddress += 2;
	}

	bTargetAddress = 0x00;
	bTargetDataPtr = 0x00;

	SendVector(tsync_enable, num_bits_tsync_enable);	//PTJ: 307, added for tsync testing
	while (bTargetAddress < (SECURITY_BYTES_PER_BANK * 2)) {	//PTJ: we do SECURITY_BYTES_PER_BANK * 2 because we bTargetAddress += 2

		//Send Read Byte vector and then get a byte from Target
		SendVector(read_byte_v, 4);
		// Set the drive here because SendByte() does not
		SetSDATAStrong();
		SendByte(bTargetAddress, 7);

		RunClock(2);	// Run two SCLK cycles between writing and reading
		SetSDATAHiZ();	// Set to HiZ so Target can drive SDATA
		bTargetDataIN = bReceiveByte();

		RunClock(1);
		SendVector(read_byte_v + 1, 1);	// Send the ReadByte Vector End

		// Test the Byte that was read from the Target against the original
		// value (already in the 128-Byte array "abTargetDataOUT[]"). If it
		// matches, then bump the address & pointer,loop-back and continue.
		// If it does NOT match abort the loop and return and error.
		if (bTargetDataIN != abTargetDataOUT[bTargetDataPtr])
//            return(BLOCK_ERROR);

			// Increment the address by two to accomodate 7-Bit addressing
			// (puts the 7-bit address into MSBit locations for "SendByte()").
			bTargetDataPtr++;
		bTargetAddress += 2;
	}

	SendVector(tsync_disable, num_bits_tsync_disable);	//PTJ: 307, added for tsync testing
	return (PASS);
}

#endif				//(PROJECT_REV_)
// end of file ISSP_Routines.c
