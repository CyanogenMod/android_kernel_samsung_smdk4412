
#ifndef __FC8150_ISR__
#define __FC8150_ISR__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

extern u32 gACUserData;
extern u32 gTSUserData;

extern int (*pACCallback)(u32 userdata, u8 *data, int length);
extern int (*pTSCallback)(u32 userdata, u8 *data, int length);

extern void fc8150_isr(HANDLE hDevice);

#ifdef __cplusplus
}
#endif
#endif
