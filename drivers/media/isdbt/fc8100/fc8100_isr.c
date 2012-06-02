/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_isr.c

 Description : fc8100 interrupt service routine

 History :
 ----------------------------------------------------------------------
 2009/12/11	ckw		initial
*******************************************************************************/
#include "fci_types.h"
#include "fci_hal.h"

u32 gUserData;
int (*pCallback)(u32 userdata, u8 *data, int length) = NULL;

u8 dmbBuffer[188*4];

void fc8100_isr(HANDLE hDevice)
{

	bbm_data(hDevice, 0, &dmbBuffer[0], 188*2);

	if (pCallback)
		(*pCallback)(gUserData, &dmbBuffer[0], 188);
}
