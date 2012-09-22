/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_oal.h

 Description : OS adatation layer
*******************************************************************************/

#ifndef __FCI_OAL_H__
#define __FCI_OAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void PRINTF(HANDLE hDevice, char *fmt, ...);
extern void msWait(int ms);

extern void OAL_CREATE_SEMAPHORE(void);
extern void OAL_DELETE_SEMAPHORE(void);
extern void OAL_OBTAIN_SEMAPHORE(void);
extern void OAL_RELEASE_SEMAPHORE(void);

#ifdef __cplusplus
}
#endif

#endif
