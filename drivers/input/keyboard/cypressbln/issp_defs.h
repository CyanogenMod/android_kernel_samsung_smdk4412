// filename: ISSP_Defs.h
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

--------------------------------------------------------------------------*/
#ifndef INC_ISSP_DEFS
#define INC_ISSP_DEFS

#include "issp_directives.h"

// Block-Verify Uses 64-Bytes of RAM
//      #define TARGET_DATABUFF_LEN    64
#define TARGET_DATABUFF_LEN    128	// **** CY8C20x66 Device ****

// The number of Flash blocks in each part is defined here. This is used in
// main programming loop when programming and verifying the blocks.

#ifdef CY8CTMx30x		// **** CY8C20x66 Device ****
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             256
#define SECURITY_BYTES_PER_BANK      64
#endif

#ifdef CY8C20x66		// **** CY8C20x66 Device ****
#ifdef CY8C20246		// **** CY8C20x66 Device ****
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             128
#define SECURITY_BYTES_PER_BANK      64
#elif defined(CY8C20236)
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             64
#define SECURITY_BYTES_PER_BANK      64
#else
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             256
#define SECURITY_BYTES_PER_BANK      64
#endif
#endif
#ifdef CY8C21x23
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK              64
#define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C21x34
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             128
#define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C24x23A
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK              64
#define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C24x94
#define NUM_BANKS                     2
#define BLOCKS_PER_BANK             128
#define SECURITY_BYTES_PER_BANK      32
#endif
#ifdef CY8C27x43
#define NUM_BANKS                     1
#define BLOCKS_PER_BANK             256
#define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8C29x66
#define NUM_BANKS                     4
#define BLOCKS_PER_BANK             128
#define SECURITY_BYTES_PER_BANK      32
#endif
#endif				//(INC_ISSP_DEFS)
#endif				//(PROJECT_REV_)
//end of file ISSP_Defs.h
