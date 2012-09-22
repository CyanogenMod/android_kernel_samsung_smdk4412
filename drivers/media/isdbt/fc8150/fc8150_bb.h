/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_bb.h

 Description : API of 1-SEG baseband module

*******************************************************************************/


#ifndef __FC8150_BB__
#define __FC8150_BB__

#include "fci_types.h"

extern int fc8150_reset(HANDLE hDevice);
extern int fc8150_probe(HANDLE hDevice);
extern int fc8150_init(HANDLE hDevice);
extern int fc8150_deinit(HANDLE hDevice);
extern int fc8150_scan_status(HANDLE hDevice);

#endif
