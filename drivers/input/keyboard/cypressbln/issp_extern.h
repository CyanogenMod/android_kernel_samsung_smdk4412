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
*/

#ifndef INC_ISSP_EXTERN
#define INC_ISSP_EXTERN

#include "issp_directives.h"

extern signed char fXRESInitializeTargetForISSP(void);
extern signed char fPowerCycleInitializeTargetForISSP(void);
extern signed char fEraseTarget(void);
extern unsigned int iLoadTarget(void);
extern void ReStartTarget(void);
extern signed char fVerifySiliconID(void);
extern signed char fAccTargetBankChecksum(unsigned int *);
extern void SetBankNumber(unsigned char);
extern signed char fProgramTargetBlock(unsigned char, unsigned char);
extern signed char fVerifyTargetBlock(unsigned char, unsigned char);
extern signed char fVerifySetup(unsigned char, unsigned char);
extern signed char fReadByteLoop(void);	/*PTJ: read bytes after VERIFY-SETUP*/
extern signed char fSecureTargetFlash(void);

extern signed char fReadStatus(void);	/*PTJ: READ-STATUS*/
extern signed char fReadCalRegisters(void);
extern signed char fReadWriteSetup(void);	/*PTJ: READ-WRITE-SETUP*/
extern signed char fReadSecurity(void);	/*PTJ: READ-SECURITY*/

extern signed char fSyncDisable(void);	/*PTJ: SYNC-DISABLE rev 307*/
extern signed char fSyncEnable(void);	/*PTJ: SYNC-ENABLE rev 307*/

extern void InitTargetTestData(void);
extern void LoadArrayWithSecurityData(unsigned char, unsigned char,
				      unsigned char);

extern void LoadProgramData(unsigned char, unsigned char);
extern signed char fLoadSecurityData(unsigned char);
extern void Delay(unsigned char);
extern unsigned char fSDATACheck(void);
extern void SCLKHigh(void);
extern void SCLKLow(void);
#ifndef RESET_MODE		/*only needed when power cycle mode*/
extern void SetSCLKHiZ(void);
#endif
extern void SetSCLKStrong(void);
extern void SetSDATAHigh(void);
extern void SetSDATALow(void);
extern void SetSDATAHiZ(void);
extern void SetSDATAStrong(void);
extern void AssertXRES(void);
extern void DeassertXRES(void);
extern void SetXRESStrong(void);
extern void ApplyTargetVDD(void);
extern void RemoveTargetVDD(void);
extern void SetTargetVDDStrong(void);

extern unsigned char fIsError;

#ifdef USE_TP
extern void InitTP(void);
extern void SetTPHigh(void);
extern void SetTPLow(void);
extern void ToggleTP(void);
#endif

extern int ISSP_main(void);

#endif				/*(INC_ISSP_EXTERN)*/
#endif				/*(PROJECT_REV_)*/
