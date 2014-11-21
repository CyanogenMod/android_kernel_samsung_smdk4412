/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_types.h

 Description :
*******************************************************************************/

#ifndef __FCI_TYPES_H__
#define __FCI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HANDLE
#define	HANDLE void *
#endif

#define BBM_HPI		0
#define BBM_SPI		1
#define BBM_USB		2
#define BBM_I2C		3
#define BBM_PPI		4
#define BBM_SPIB	5

#define s8          signed char
#define s16         signed short int
#define s32         signed int
#define u8          unsigned char
#define u16         unsigned short
#define u32         unsigned int
#define TRUE        1
#define FALSE       0

#ifndef NULL
#define NULL        0
#endif

#define BBM_OK      0
#define BBM_NOK     1

#define BBM_E_FAIL              0x00000001
#define BBM_E_HOSTIF_SELECT     0x00000002
#define BBM_E_HOSTIF_INIT       0x00000003
#define BBM_E_BB_WRITE          0x00000100
#define BBM_E_BB_READ           0x00000101
#define BBM_E_TN_WRITE          0x00000200
#define BBM_E_TN_READ           0x00000201
#define BBM_E_TN_CTRL_SELECT    0x00000202
#define BBM_E_TN_CTRL_INIT      0x00000203
#define BBM_E_TN_SELECT         0x00000204
#define BBM_E_TN_INIT           0x00000205
#define BBM_E_TN_RSSI           0x00000206
#define BBM_E_TN_SET_FREQ       0x00000207

#ifdef __cplusplus
}
#endif

#endif
