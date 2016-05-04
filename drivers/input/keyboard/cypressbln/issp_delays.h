// filename: ISSP_Delays.h
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
#ifndef INC_ISSP_DELAYS
#define INC_ISSP_DELAYS

// The Delay() routine, in ISSP_Driver_Routines.c, has a delay of n+3 usec,
// where n is the value passed to the routine.  This is true for the m8c micro-
// processor in the PSoC when it is running at a CPU clock of 24MHz.
//
// PROCESSOR_SPECIFIC
// If another processor is used, or if the m8c is running at a slower clock
// speed, then the delay parameters will be different. This file makes changing
// the delays simpiler when porting the program to other processors.

// DELAY_M is the slope of the Delay = Mx + B equation
#define DELAY_M    1
// DELAY_B is the offset of the delay in Delay = Mx + B.
#define DELAY_B    3

///////////////////////////////////////////////////////////////////////////////
// CAUTION:
// For the above parameters the minimum delay value is 3 (this would result in
// 0 being passed for a minimum delay. A value less than 3 would actually
// create a negative number, causing a very long delay
///////////////////////////////////////////////////////////////////////////////

// TRANSITION_TIMEOUT is a loop counter for a 100msec timeout when waiting for
// a high-to-low transition. This is used in the polling loop of
// fDetectHiLoTransition(). Each pass through the loop takes approximately 15
// usec. 100 msec is about 6740 loops. 13480
#define TRANSITION_TIMEOUT     0x100000	//6740

// XRES_DELAY is the time duration for which XRES is asserted. This defines
// a 63 usec delay.
// The minimum Xres time (from the device datasheet) is 10 usec.
//mhsong #define XRES_CLK_DELAY    ((63 - DELAY_B) / DELAY_M)
#define XRES_CLK_DELAY    63

// POWER_CYCLE_DELAY is the time required when power is cycled to the target
// device to create a power reset after programming has been completed. The
// actual time of this delay will vary from system to system depending on the
// bypass capacitor size.  A delay of 150 usec is used here.
//mhsong #define POWER_CYCLE_DELAY ((150 - DELAY_B) / DELAY_M)
#define POWER_CYCLE_DELAY 150

// DELAY_100us delays 100 usec. This is used in fXRESInitializeTargetForISSP to
// time the wait for Vdd to become stable after a power up.  A loop runs 10 of
// these for a total delay of 1 msec.
//mhsong #define DELAY100us        ((100 - DELAY_B) / DELAY_M)
#define DELAY100us        100

#endif				//(INC_ISSP_DELAYS)
#endif				//(PROJECT_REV_)
//end of file ISSP_Delays.h
