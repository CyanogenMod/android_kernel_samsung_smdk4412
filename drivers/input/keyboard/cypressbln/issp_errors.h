// filename: ISSP_Errors.h
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
#ifndef INC_ISSP_ERRORS
#define INC_ISSP_ERRORS

// The following are defines for error messages from the ISSP program.
#define PASS           0
// PASS is used to indicate that a function completed successfully.
#define ERROR         -1
// ERROR is a generic failure used within lower level functions before the
// error is reported.  This should not be seen as an error that is reported
// from main.
#define INIT_ERROR     1
// INIT_ERROR means a step in chip initialization failed.
#define SiID_ERROR     2
// SiID_ERROR means that the Silicon ID check failed. This happens if the
// target part does not match the device type that the ISSP program is
// configured for.
#define ERASE_ERROR    3
// ERASE_ERROR means that the bulk erase step failed. 
#define BLOCK_ERROR    4
// BLOCK_ERROR means that a step in programming a Flash block or the verify
// of the block failed.
#define VERIFY_ERROR   5
// VERIFY_ERROR means that the checksum verification failed.
#define SECURITY_ERROR 6
// SECURITY_ERROR means that the write of the security information failed.
#define STATUS_ERROR 7

#endif				//(INC_ISSP_ERRORS)
#endif				//(PROJECT_REV_)
//end of file ISSP_Errors.h
