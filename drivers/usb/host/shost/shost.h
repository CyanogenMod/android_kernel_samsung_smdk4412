#ifndef _SHOST_H
#define _SHOST_H


#include <linux/usb.h>
#include <linux/errno.h>
#include <linux/wakelock.h>
#include <plat/s5p-otghost.h>


#include "shost_debug.h"
#include "shost_list.h"

#include "shost_const.h"
#include "shost_errorcode.h"
#include "shost_regs.h"
#include "shost_struct.h"

#include "shost_kal.h"
#include "shost_mem.h"
#include "shost_oci.h"

#include "shost_scheduler.h"
#include "shost_transfer.h"

/* #ifdef CONFIG_MACH_C1 GB */
#if 0
	#include <plat/regs-otg.h>
	#define OTG_CLOCK			S5P_CLKGATE_IP_FSYS
	#define OTG_PHY_CONTROL		S5P_USBOTG_PHY_CONTROL
	#define OTG_PHYPWR			S3C_USBOTG_PHYPWR
	#define OTG_PHYCLK			S3C_USBOTG_PHYCLK
	#define OTG_RSTCON			S3C_USBOTG_RSTCON
	#define S3C_VA_HSOTG		S3C_VA_OTG
#else /* ICS */
	#include <mach/regs-usb-phy.h>
	#define OTG_CLOCK			EXYNOS4_CLKGATE_IP_FSYS
	#define OTG_PHY_CONTROL		S5P_USBOTG_PHY_CONTROL
	#define OTG_PHYPWR			EXYNOS4_PHYPWR
	#define OTG_PHYCLK			EXYNOS4_PHYCLK
	#define OTG_RSTCON			EXYNOS4_RSTCON
	#define IRQ_OTG				IRQ_USB_HSOTG
	#define S3C_VA_OTG			S3C_VA_HSOTG
#endif

/* transferchecker-common.c
 * called by isr
 */
void do_transfer_checker(struct sec_otghost *otghost);
void otg_print_registers(void);

#ifdef CONFIG_USB_HOST_NOTIFY
#undef CONFIG_USB_HOST_NOTIFY
#endif

#endif
