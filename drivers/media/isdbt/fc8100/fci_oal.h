/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fci_oal.h

 Description : OS Adatation Layer header

 History :
 ----------------------------------------------------------------------
 2009/09/13	jason		initial
*******************************************************************************/

#ifndef __FCI_OAL_H__
#define __FCI_OAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void PRINTF(HANDLE hDevice, char *fmt, ...);
extern void msWait(int ms);

#ifdef __cplusplus
}
#endif

#endif
