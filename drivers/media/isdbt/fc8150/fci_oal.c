/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_oal.c

 Description : OS adaptation layer
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/delay.h>

#include "fc8150_regs.h"
#include "fci_types.h"

void PRINTF(HANDLE hDevice, char *fmt, ...)
{
	va_list ap;
	char str[256];

	va_start(ap, fmt);
	vsprintf(str, fmt, ap);

	printk(KERN_DEBUG"%s", str);

	va_end(ap);
}

void msWait(int ms)
{
	msleep(ms);
}

void OAL_CREATE_SEMAPHORE(void)
{

}

void OAL_DELETE_SEMAPHORE(void)
{

}

void OAL_OBTAIN_SEMAPHORE(void)
{

}

void OAL_RELEASE_SEMAPHORE(void)
{

}
