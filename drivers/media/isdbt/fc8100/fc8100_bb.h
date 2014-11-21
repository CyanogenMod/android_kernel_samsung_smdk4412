/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_bb.h

 Description : baseband header file

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/

#ifndef __FC8100_BB_H__
#define __FC8100_BB_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8100_reset(HANDLE hDevice);
extern int fc8100_probe(HANDLE hDevice);
extern int fc8100_init(HANDLE hDevice);
extern int fc8100_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
