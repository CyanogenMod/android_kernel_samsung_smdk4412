/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fci_types.h

 Description :

 History :
 ----------------------------------------------------------------------
 2009/08/31	jason		initial
*******************************************************************************/

#ifndef __FCI_TYPES_H__
#define __FCI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HANDLE
typedef int		HANDLE;
#endif

#define s8		char
#define s16		short int
#define s32		int

#define u8		unsigned char
#define u16		unsigned short
#define u32		unsigned int

#define TRUE		1
#define FALSE		0

#ifndef NULL
#define NULL		0
#endif

#define BBM_OK		0
#define BBM_NOK		1

#ifdef __cplusplus
}
#endif

#endif
