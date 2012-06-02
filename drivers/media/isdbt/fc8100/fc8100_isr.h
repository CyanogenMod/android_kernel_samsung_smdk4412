/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_isr.c

 Description : API of dmb baseband module

 History :
 ----------------------------------------------------------------------
 2009/12/11	ckw		initial
*******************************************************************************/

#ifndef __FC8000_ISR__
#define __FC8000_ISR__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

extern u32 gUserData;
extern int (*pCallback)(u32 userdata, u8 *data, int length);

extern void fc8100_isr(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
