/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_isr.c

 Description : fc8150 interrupt service routine

*******************************************************************************/
#include "fci_types.h"
#include "fci_hal.h"
#include "fci_oal.h"
#include "fc8150_regs.h"

int (*pTSCallback)(u32 userdata, u8 *data, int length) = NULL;
int (*pACCallback)(u32 userdata, u8 *data, int length) = NULL;

u32 gTSUserData;
u32 gACUserData;

/* static u8 acBuffer[AC_BUF_THR]; */
static u8 tsBuffer[TS_BUF_SIZE];

static void fc8150_data(HANDLE hDevice, u8 bufIntStatus)
{
	if (bufIntStatus & 0x01) { /* TS */
		bbm_data(hDevice, BBM_TS_DATA, &tsBuffer[0], TS_BUF_SIZE/2);

		if (pTSCallback)
			(*pTSCallback)(gTSUserData
			, &tsBuffer[0], TS_BUF_SIZE/2);
	}
}

void fc8150_isr(HANDLE hDevice)
{
	u8 IntStatus = 0;
	u8 bufIntStatus = 0;

	bbm_read(hDevice, BBM_INT_STATUS, &IntStatus);
	bbm_write(hDevice, BBM_INT_STATUS, IntStatus);

	bbm_read(hDevice, BBM_BUF_STATUS, &bufIntStatus);
	if (bufIntStatus) {
		bbm_write(hDevice, BBM_BUF_STATUS, bufIntStatus);
		fc8150_data(hDevice, bufIntStatus);
	}
}
