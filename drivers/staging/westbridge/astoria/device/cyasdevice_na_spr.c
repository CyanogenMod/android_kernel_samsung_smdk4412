/*
## cyandevice.c - Linux Antioch device driver file
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor
## Boston, MA  02110-1301, USA.
## ===========================
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
/* moved for staging location
 * update/patch submission
#include <linux/westbridge/cyastoria.h>
#include <linux/westbridge/cyashal.h>
#include <linux/westbridge/cyasregs.h>
*/

#include "../include/linux/westbridge/cyastoria.h"
#include "../include/linux/westbridge/cyashal.h"
#include "../include/linux/westbridge/cyasregs.h"
#include "../arch/arm/plat-c110/include/mach/westbridge/westbridge-c110-pnand-hal/cyasmemmap.h"

#ifdef WESTBRIDGE_ASTORIA
#include "firmware/cyastfw_sd_mmc_rel_silicon.h"
#include "firmware/cyastfw_mtp_sd_mmc_rel_silicon.h"
#endif

/* API exports include file */
#include "cyandevice_export.h"
#include "cyasdiagnostics.h"

#define __USE_ISR_FOR_SD_DETECT__
#ifdef __USE_CYAS_AUTO_SUSPEND__
#define CYASDEVICE_THREAD_EXIT		(1 << 0)
#define CYASDEVICE_THREAD_SUSPENDED	(1 << 1)
#define CYASDEVICE_THREAD_TIMER		(1 << 2)

#define CYASDEVICE_THREAD_ENABLE	(1 << 7)

#define CYASDEVICE_STANDBY_TIMEOUT 200
#endif

typedef struct {
	struct delayed_work work;
	int	f_reload;
	int	f_status;
	int		f_isrunning;
} cy_work_t;

typedef struct cyasdevice {
		/* Handle to the Antioch device */
		cy_as_device_handle			dev_handle;
		/* Handle to the HAL */
		cy_as_hal_device_tag			hal_tag;
		/* spinlock_t	  common_lock; */
		struct semaphore common_sema;
		unsigned long flags;

		struct workqueue_struct		*cy_wq;
		cy_work_t		*cy_work;
		struct semaphore wq_sema;
		uint8_t	f_platform_driver;

#ifdef __USE_CYAS_AUTO_SUSPEND__
		struct completion	thread_sleep_complete;
		wait_queue_head_t	thread_sleep_wq;
		struct semaphore	thread_sleep_sem;
		unsigned long		thread_sleep_flags;
		unsigned long		thread_sleep_enable;
		int					thread_sleep_count;
#endif
} cyasdevice;

#ifdef __USE_CYAS_AUTO_SUSPEND__
int cyasdevice_wakeup_thread(int flag);
int cyasdevice_enable_thread(void);
int cyasdevice_disable_thread(void);
#endif

/* global ptr to astoria device */
static cyasdevice *cy_as_device_controller;
int cy_as_device_init_done;
const char *dev_handle_name = "cy_astoria_dev_handle";

static int firmware_number;
static int f_isStandby;

extern int  cyasblkdev_start_sdcard(void);
extern int  cyasblkdev_stop_sdcard(void);

extern int  cyasblkdev_check_sdcard(void);
extern int  cyasblkdev_get_serial(void);
extern int  cyasblkdev_get_CID(char *ptrCID);
extern int  cyasblkdev_blk_init(int fmajor, int fsearch);
extern void  cyasblkdev_blk_exit(void);
int cyasdevice_reload_firmware(int mtp_mode);
void cy_as_acquire_common_lock(void);
void cy_as_release_common_lock(void);
int cyasdevice_enter_standby(void);
int cyasdevice_leave_standby(void);

#ifdef CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL
extern void cy_as_hal_config_c_s_mux(void);
#endif

extern int	cy_as_hal_detect_SD(void);

#ifdef __USE_ISR_FOR_SD_DETECT__
extern int	cy_as_hal_enable_irq(void);
extern int	cy_as_hal_disable_irq(void);
extern int cy_as_hal_configure_sd_isr(void *dev_p, irq_handler_t isr_function);
extern int cy_as_hal_free_sd_isr(void);
extern int cy_as_hal_enable_power(cy_as_hal_device_tag tag, int flag);
#endif


extern int cy_as_hal_enable_NANDCLK(int flag);

int g_SD_flag;
static void cy_wq_function(struct work_struct *work)
{
	int	status;
	int	is_inserted;
	int ret;
	int retry_count;

	down(&cy_as_device_controller->thread_sleep_sem);

	down(&cy_as_device_controller->wq_sema);
	cy_as_device_controller->cy_work->f_isrunning = 1;
	up(&cy_as_device_controller->wq_sema);

	status = cy_as_device_controller->cy_work->f_status;
	g_SD_flag = is_inserted = cy_as_hal_detect_SD();

	if (cy_as_device_controller->cy_work->f_reload == 1) {
		cy_as_device_controller->cy_work->f_reload = 0;
		cy_as_hal_enable_power(cy_as_device_controller->hal_tag, 1);
	}

	#ifdef __USE_CYAS_AUTO_SUSPEND__
	cyasdevice_disable_thread();
	#endif

	if (!is_inserted) {	/* removed SD card. */

		cy_as_hal_print_message(KERN_ERR
			"[cyasdevice.c] %s: SD card is removed.\n", __func__) ;
		if (status) {
			cy_as_hal_print_message(KERN_ERR
			"[cyasdevice.c] %s: cyasblkdev_blk_exit\n", __func__) ;
			down(&cy_as_device_controller->wq_sema);
			cyasdevice_leave_standby();
			cy_as_device_controller->cy_work->f_status = 0x0;
			up(&cy_as_device_controller->wq_sema);

			cyasblkdev_blk_exit();
			retry_count = 0;
			while (!cy_as_device_init_done) {
				if (++retry_count == 5) {
					cyasdevice_reload_firmware(0);
					break;
				}
				msleep(10);
			}

		}
		down(&cy_as_device_controller->wq_sema);
		cyasdevice_enter_standby();
		up(&cy_as_device_controller->wq_sema);
		msleep(10);
	} else {		/* insterted SD card.  */

		cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s:\
					leave standby\n", __func__) ;
		down(&cy_as_device_controller->wq_sema);
		cyasdevice_leave_standby();
		up(&cy_as_device_controller->wq_sema);

		retry_count = 0;
		while (!cy_as_device_init_done) {
			if (++retry_count == 5) {
				cyasdevice_reload_firmware(0);
				break;
			}
			msleep(10);
		}

		cy_as_hal_print_message(KERN_ERR
		"++++++++++++  [cyasdevice.c] %s: SD card is inserted.\n",
		__func__) ;
		if (cy_as_device_controller->cy_work->f_reload == 2) {
			cy_as_device_controller->cy_work->f_reload = 0;

			ret = cyasblkdev_check_sdcard();
			if (ret == 1) {
				cy_as_hal_print_message(
				"[cyasdevice.c] %s: cyasblkdev_blk_exit\n",
				__func__) ;
				cyasblkdev_blk_exit();
				status =
				cy_as_device_controller->cy_work->f_status = 0x0;
			} else if (ret < 0) {
				cyasblkdev_blk_exit();
				cy_as_hal_print_message(
				"[cyasdevice.c] %s: reload firmware\n", __func__) ;
				status = cy_as_device_controller->cy_work->f_status = 0x0;
			} else {
				#ifdef __USE_CYAS_AUTO_SUSPEND__
				cyasdevice_enable_thread();
				#endif
			}
		}
		if (status == 0) {
			cy_as_hal_print_message(KERN_ERR
			"[cyasdevice.c] %s: cyasblkdev_blk_init\n", __func__) ;
			cy_as_device_controller->cy_work->f_status = 0x1;

			retry_count = 2;

			while (retry_count--) {
				ret = cyasblkdev_blk_init(0, 0);

				if (ret) {
					cyasblkdev_blk_exit();
					msleep(10);
					cyasdevice_reload_firmware(-1);
					msleep(10);
				} else
					break;
			}

			if (ret) {
				down(&cy_as_device_controller->wq_sema);
				cyasdevice_enter_standby();
				up(&cy_as_device_controller->wq_sema);
				msleep(10);

				cy_as_device_controller->cy_work->f_isrunning = 0;
				up(&cy_as_device_controller->thread_sleep_sem);
				return;
			}
		}

		#ifdef __USE_CYAS_AUTO_SUSPEND__
		/* cyasdevice_enable_thread(); */
		#endif
	}
	cy_as_device_controller->cy_work->f_isrunning = 0;

	up(&cy_as_device_controller->thread_sleep_sem);

	return;
}
#ifdef __USE_ISR_FOR_SD_DETECT__
static irqreturn_t cyasdevice_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int cur_SD_flag = cy_as_hal_detect_SD();

	if (g_SD_flag != cur_SD_flag) {
		g_SD_flag = cur_SD_flag;
		queue_delayed_work(cy_as_device_controller->cy_wq,
			&cy_as_device_controller->cy_work->work, 0);
	}
	return IRQ_HANDLED ;
}
#endif
/* below structures and functions primarily
 * implemented for firmware loading */
#ifdef __CYAS_SYSFS_FOR_DIAGNOSTICS__

extern int cy_as_diagnostics(cy_as_diag_cmd_type mode, char *result);

char cyas_diagnostics[1024];

static ssize_t show_cyas_diagnostics(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", cyas_diagnostics);
}

static ssize_t store_cyas_diagnostics(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int	ret;
	int	mode = 0;
	int i = 0, len;

	sscanf(buf, "%s", cyas_diagnostics);

	len = strnlen(buf, PAGE_SIZE);

	while (i < len) {
		if (buf[i] < 0x30 || buf[i] > 0x39)
			break;

		mode += (buf[i]&0x0F);

		i++;

		if (i == len)
			break;
		else if (buf[i] == 0xa)
			break;
		else
			mode *= 10;
	}

	cy_as_hal_print_message("[%s] mode=%d\n", __func__, mode);

	switch (mode) {
	case 1:
		ret = cy_as_diagnostics(CY_AS_DIAG_GET_VERSION, cyas_diagnostics);
		break;
	case 2:
		ret = cy_as_diagnostics(CY_AS_DIAG_DISABLE_MSM_SDIO, cyas_diagnostics);
		break;
	case 3:
		ret = cy_as_diagnostics(CY_AS_DIAG_ENABLE_MSM_SDIO, cyas_diagnostics);
		break;
	case 4:
		ret = cy_as_diagnostics(CY_AS_DIAG_LEAVE_STANDBY, cyas_diagnostics);
		break;
	case 5:
		ret = cy_as_diagnostics(CY_AS_DIAG_ENTER_STANDBY, cyas_diagnostics);
		break;
	case 6:
		ret = cy_as_diagnostics(CY_AS_DIAG_CREATE_BLKDEV, cyas_diagnostics);
		break;
	case 7:
		ret = cy_as_diagnostics(CY_AS_DIAG_DESTROY_BLKDEV, cyas_diagnostics);
		break;
	case 11:
		ret = cy_as_diagnostics(CY_AS_DIAG_SD_MOUNT, cyas_diagnostics);
		break;
	case 12:
		ret = cy_as_diagnostics(CY_AS_DIAG_SD_READ, cyas_diagnostics);
		break;
	case 13:
		ret = cy_as_diagnostics(CY_AS_DIAG_SD_WRITE, cyas_diagnostics);
		break;
	case 14:
		ret = cy_as_diagnostics(CY_AS_DIAG_SD_UNMOUNT, cyas_diagnostics);
		break;
	case 21:
		ret = cy_as_diagnostics(CY_AS_DIAG_CONNECT_UMS, cyas_diagnostics);
		break;
	case 22:
		ret = cy_as_diagnostics(CY_AS_DIAG_DISCONNECT_UMS, cyas_diagnostics);
		break;
	case 31:
		ret = cy_as_diagnostics(CY_AS_DIAG_CONNECT_MTP, cyas_diagnostics);
		break;
	case 32:
		ret = cy_as_diagnostics(CY_AS_DIAG_DISCONNECT_MTP, cyas_diagnostics);
		break;
	case 40:
		ret = cy_as_diagnostics(CY_AS_DIAG_TEST_RESET_LOW, cyas_diagnostics);
		break;
	case 41:
		ret = cy_as_diagnostics(CY_AS_DIAG_TEST_RESET_HIGH, cyas_diagnostics);
		break;
	default:
		ret = 1;
		break;
	}

	if (ret)
		return 0;

	return len;
}

static DEVICE_ATTR(cyas_diagnostics,
		S_IRUGO | S_IWUSR,
		show_cyas_diagnostics,
		store_cyas_diagnostics);

static int serial;
static ssize_t show_cyas_serial(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	serial = cyasblkdev_get_serial();
	return snprintf(buf, PAGE_SIZE, "0x%08x\n", serial);
}

static ssize_t store_cyas_serial(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	return strlen(buf);
}

static DEVICE_ATTR(serial, S_IRUGO | S_IWUSR, show_cyas_serial, store_cyas_serial);

static int wbcid[4];
static ssize_t show_cyas_wbcid(struct device *dev, struct device_attribute *attr, char *buf)
{
	int size;
	size = cyasblkdev_get_CID((char *)wbcid);
	return snprintf(buf, PAGE_SIZE, "%08x%08x%08x%08x\n", wbcid[0], wbcid[1], wbcid[2], wbcid[3]);
}

static ssize_t store_cyas_wbcid(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	return strlen(buf);
}

static DEVICE_ATTR(wbcid, S_IRUGO | S_IWUSR, show_cyas_wbcid, store_cyas_wbcid);

#endif
static struct platform_device *westbridge_pd;

static int __devinit wb_probe(struct platform_device *devptr)
{
	cy_as_hal_print_message("%s called\n", __func__);
	return 0;
}

static int __devexit wb_remove(struct platform_device *devptr)
{
	cy_as_hal_print_message("%s called\n", __func__);
	return 0;
}

static int wb_suspend(struct platform_device *devptr, pm_message_t pm)
{
	disable_irq(WB_SDCD_IRQ_INT);

	down(&cy_as_device_controller->wq_sema);
	if (cy_as_device_controller->cy_work->f_isrunning) {
		up(&cy_as_device_controller->wq_sema);
		enable_irq(WB_SDCD_IRQ_INT);
		return -1;
	}
	up(&cy_as_device_controller->wq_sema);

	/* cy_as_hal_print_message(KERN_ERR"%s called\n", __func__); */
	/* down(&cy_as_device_controller->wq_sema); */
	cyasdevice_disable_thread();
	cancel_delayed_work(&cy_as_device_controller->cy_work->work);

	cyasdevice_enter_standby();
	/* disable clock */
	/* cy_as_hal_sleep(10); */
	msleep(10);
	disable_irq(WB_CYAS_IRQ_INT);
	gpio_set_value(WB_CLK_EN, 0);
	cy_as_hal_enable_NANDCLK(0);
	cy_as_hal_enable_power(cy_as_device_controller->hal_tag, 0);
	/* up(&cy_as_device_controller->wq_sema); */
	return 0;
}

static int wb_resume(struct platform_device *devptr)
{
	int retval;
	int sd_status;
	/* cy_as_hal_print_message(KERN_ERR"%s called\n", __func__); */
	/* cyasdevice_leave_standby(); */
	/* down(&cy_as_device_controller->wq_sema); */
	cy_as_hal_enable_power(cy_as_device_controller->hal_tag, 1);
	cy_as_hal_enable_NANDCLK(1);
	gpio_set_value(WB_CLK_EN, 1);
	msleep(10);
	/* cy_as_hal_sleep(10); */
	/* up(&cy_as_device_controller->wq_sema); */
	enable_irq(WB_CYAS_IRQ_INT);
	msleep(5);
	sd_status = cy_as_hal_detect_SD();
	if (sd_status) {
		down(&cy_as_device_controller->wq_sema);
		cy_as_device_controller->cy_work->f_isrunning = 1;
		up(&cy_as_device_controller->wq_sema);
		retval = cyasdevice_wakeup_thread(1);

		if (retval ==  0) {
			msleep(10);
			cyasblkdev_stop_sdcard();
			retval = cyasblkdev_start_sdcard();
			/* retval = cy_as_misc_storage_changed(
					cy_as_device_controller->dev_handle, 0, 0); */
			if (retval != CY_AS_ERROR_SUCCESS) {
				#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
					"%s : fail in cy_as_misc_storage_changed (%d)\n",
					__func__, retval);
				#endif
			}
		}
		down(&cy_as_device_controller->wq_sema);
		cy_as_device_controller->cy_work->f_isrunning = 0;
		up(&cy_as_device_controller->wq_sema);
	}
	if (sd_status || cy_as_device_controller->cy_work->f_status) {
		cy_as_device_controller->cy_work->f_reload = 2;
		queue_delayed_work(cy_as_device_controller->cy_wq,
			&cy_as_device_controller->cy_work->work, 0);
	}
	enable_irq(WB_SDCD_IRQ_INT);
	return 0;
}

static struct platform_driver west_bridge_driver = {
	.probe = wb_probe,
	.remove = __devexit_p(wb_remove),
	.suspend = wb_suspend,
	.resume = wb_resume,
	.driver = {
		   .name = "west_bridge_dev"},
};


static void cyasdevice_deinit(cyasdevice *cy_as_dev)
{
	cy_as_hal_print_message("<1>_cy_as_device deinitialize called\n");
	if (!cy_as_dev) {
		cy_as_hal_print_message("<1>_cy_as_device_deinit:  "
			"device handle %x is invalid\n", (uint32_t)cy_as_dev);
		return;
	}

	/* stop west_brige */
	if (cy_as_dev->dev_handle) {
		cy_as_hal_print_message("<1>_cy_as_device: "
			"cy_as_misc_destroy_device called\n");
		if (cy_as_misc_destroy_device(cy_as_dev->dev_handle) !=
			CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(
				"<1>_cy_as_device: destroying failed\n");
		}
	}

	if (cy_as_dev->hal_tag) {

 #ifdef CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL
		if (stop_o_m_a_p_kernel(dev_handle_name,
			cy_as_dev->hal_tag) != 0)
			cy_as_hal_print_message("<1>_cy_as_device: stopping "
				"OMAP kernel HAL failed\n");

 #endif
#ifdef CONFIG_MACH_C110_WESTBRIDGE_AST_PNAND_HAL
		if (cy_as_hal_c110_pnand_stop(dev_handle_name,	cy_as_dev->hal_tag) != 0)
			cy_as_hal_print_message("<1>_cy_as_device: stopping C110 CRAM HAL failed\n");
#endif

	}
	cy_as_hal_print_message("<1>_cy_as_device:HAL layer stopped\n");

	if (cy_as_dev->f_platform_driver) {
		cy_as_hal_print_message("<1>_cy_as_device: remove cyas_diagnostics\n") ;
		#ifdef __CYAS_SYSFS_FOR_DIAGNOSTICS__
		device_remove_file(&westbridge_pd->dev, &dev_attr_cyas_diagnostics);
		device_remove_file(&westbridge_pd->dev, &dev_attr_serial);
		device_remove_file(&westbridge_pd->dev, &dev_attr_wbcid);
		#endif

		cy_as_hal_print_message("<1>_cy_as_device: unregister west_bridge_driver\n") ;
		platform_driver_unregister(&west_bridge_driver);
	}

	kfree(cy_as_dev);
	cy_as_device_controller = NULL;
	cy_as_hal_print_message("<1>_cy_as_device: deinitialized\n");
}

/*called from src/cyasmisc.c:MyMiscCallback() as a func
 * pointer  [dev_p->misc_event_cb] which was previously
 * registered by CyAsLLRegisterRequestCallback(...,
 * MyMiscCallback); called from CyAsMiscConfigureDevice()
 * which is in turn called from cyasdevice_initialize() in
 * this src
 */
static void cy_misc_callback(cy_as_device_handle h,
	cy_as_misc_event_type evtype, void *evdata)
{
	(void)h;
	(void)evdata;

	switch (evtype) {
	case cy_as_event_misc_initialized:
	cy_as_hal_print_message("<1>_cy_as_device: "
		"initialization done callback triggered\n");
	cy_as_device_init_done = 1;
	break;

	case cy_as_event_misc_awake:
	cy_as_hal_print_message("<1>_cy_as_device: "
		"cy_as_event_misc_awake event callback triggered\n");
	cy_as_device_init_done = 1;
	break;
	default:
	break;
	}
}

void cy_as_acquire_common_lock()
{
	/* if (cy_as_device_controller) */
	/*while(!cy_as_device_controller);*/
	/* printk("%s called by %s\n", __func__, caller); */
	down(&cy_as_device_controller->common_sema);
}
EXPORT_SYMBOL(cy_as_acquire_common_lock);

void cy_as_release_common_lock(void)
{
	/* if (cy_as_device_controller) */
	/*while(!cy_as_device_controller);*/
	/* printk("%s called by %s\n", __func__, caller); */
	up(&cy_as_device_controller->common_sema);
}
EXPORT_SYMBOL(cy_as_release_common_lock);

/* reset astoria and reinit all regs */
 #define PNAND_REG_CFG_INIT_VAL 0x0000
void  hal_reset(cy_as_hal_device_tag tag)
{
	cy_as_hal_print_message("<1> send soft hard rst: "
		"MEM_RST_CTRL_REG_HARD...\n");
	cy_as_hal_write_register(tag, CY_AS_MEM_RST_CTRL_REG,
		CY_AS_MEM_RST_CTRL_REG_HARD);
	mdelay(60);

	cy_as_hal_print_message("<1> after RST: si_rev_REG:%x, "
		"PNANDCFG_reg:%x\n",
		 cy_as_hal_read_register(tag, CY_AS_MEM_CM_WB_CFG_ID),
		 cy_as_hal_read_register(tag, CY_AS_MEM_PNAND_CFG)
	);

	/* set it to LBD */
	cy_as_hal_write_register(tag, CY_AS_MEM_PNAND_CFG,
		PNAND_REG_CFG_INIT_VAL);
}
EXPORT_SYMBOL(hal_reset);




/* west bridge device driver main init */
static int cyasdevice_initialize(void)
{
	cyasdevice *cy_as_dev = 0;
	int		 ret	= 0;
	int		 retval = 0;
	cy_as_device_config config;
	cy_as_hal_sleep_channel channel;
	cy_as_get_firmware_version_data ver_data = {0};
	const char *str = "";
	int spin_lim;
	const struct firmware *fw_entry;

	cy_as_device_init_done = 0;

	cy_as_misc_set_log_level(8);

	cy_as_hal_print_message("<1>_cy_as_device initialize called\n");

	if (cy_as_device_controller != 0) {
		cy_as_hal_print_message("<1>_cy_as_device: the device "
			"has already been initilaized. ignoring\n");
		return -EBUSY;
	}

	/* cy_as_dev = CyAsHalAlloc (sizeof(cyasdevice), SLAB_KERNEL); */
	cy_as_dev = cy_as_hal_alloc(sizeof(cyasdevice));
	if (cy_as_dev == NULL) {
		cy_as_hal_print_message("<1>_cy_as_device: "
			"memory allocation failed\n");
		return -ENOMEM;
	}
	memset(cy_as_dev, 0, sizeof(cyasdevice));


	/* Init the HAL & CyAsDeviceHandle */

 #ifdef CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL
 /* start OMAP HAL init instsnce */

	if (!start_o_m_a_p_kernel(dev_handle_name,
		&(cy_as_dev->hal_tag), cy_false)) {

		cy_as_hal_print_message(
			"<1>_cy_as_device: start OMAP34xx HAL failed\n");
		goto done;
	}
 #endif

#ifdef CONFIG_MACH_C110_WESTBRIDGE_AST_PNAND_HAL
 /* start C110 HAL init instsnce */
	if (!cy_as_hal_c110_pnand_start(dev_handle_name,
		&(cy_as_dev->hal_tag), cy_false)) {
		cy_as_hal_print_message("<1>_cy_as_device: start C110 HAL failed\n") ;
		goto done;
	}
#endif
	/* Now create the device */
	if (cy_as_misc_create_device(&(cy_as_dev->dev_handle),
		cy_as_dev->hal_tag) != CY_AS_ERROR_SUCCESS) {

		cy_as_hal_print_message(
			"<1>_cy_as_device: create device failed\n");
		goto done;
	}

	memset(&config, 0, sizeof(config));
	config.dmaintr = cy_true;

	ret = cy_as_misc_configure_device(cy_as_dev->dev_handle, &config);
	if (ret != CY_AS_ERROR_SUCCESS) {

		cy_as_hal_print_message(
			"<1>_cy_as_device: configure device "
			"failed. reason code: %d\n", ret);
		goto done;
	}

	ret = cy_as_misc_register_callback(cy_as_dev->dev_handle,
		cy_misc_callback);
	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: "
			"cy_as_misc_register_callback failed. "
			"reason code: %d\n", ret);
		goto done;
	}

	ret = platform_driver_register(&west_bridge_driver);
	if (unlikely(ret < 0))
		return ret;
	westbridge_pd = platform_device_register_simple(
		"west_bridge_dev", -1, NULL, 0);

	if (IS_ERR(westbridge_pd)) {
		cy_as_hal_print_message("[%s] error in register the platform\
			driver for west bridge\n", __FUNCTION__);
		platform_driver_unregister(&west_bridge_driver);
		return PTR_ERR(westbridge_pd);
	} else {
		#ifdef __CYAS_SYSFS_FOR_DIAGNOSTICS__
		ret = device_create_file(&westbridge_pd->dev, &dev_attr_cyas_diagnostics);
		if (ret)
			cy_as_hal_print_message("[%s] error in device_create_file\
					for device_create_file 1\n", __FUNCTION__);
		ret = device_create_file(&westbridge_pd->dev, &dev_attr_serial);
		if (ret)
			cy_as_hal_print_message("[%s] error in device_create_file\
					for device_create_file 2\n", __FUNCTION__);
		ret = device_create_file(&westbridge_pd->dev, &dev_attr_wbcid);
		if (ret)
			cy_as_hal_print_message("[%s] error in device_create_file\
					for device_create_file 2\n", __FUNCTION__);
		#endif
	}
	cy_as_dev->f_platform_driver = 1;
#if 0
	/* Load the firmware */
	ret = request_firmware(&fw_entry,
		"west bridge fw", &westbridge_pd->dev);
	if (ret) {
		cy_as_hal_print_message("cy_as_device: "
			"request_firmware failed return val = %d\n", ret);
	} else {
		cy_as_hal_print_message("cy_as_device: "
			"got the firmware %d size=0x%x\n", ret, fw_entry->size);

		ret = cy_as_misc_download_firmware(
			cy_as_dev->dev_handle,
			fw_entry->data,
			fw_entry->size ,
			0, 0);
	}
	release_firmware(fw_entry);
#endif

#ifdef WESTBRIDGE_ASTORIA
	if (firmware_number == 0) {
		cy_as_hal_print_message("<1>_cy_as_device: firmware_number=0, download firmware cyastfw_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyastfw_sd_mmc_rel_silicon_array.fw_image,
			CYASTFW_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	} else {
		cy_as_hal_print_message("<1>_cy_as_device: firmware_number=1, download firmware cyastfw_mtp_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle, cyastfw_mtp_sd_mmc_rel_silicon_array.fw_image, CYASTFW_MTP_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	}
#endif

	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: cannot download firmware. reason code: %d\n", ret);
		goto done;
	}

	/* spin until the device init is completed */
	/* 50 -MAX wait time for the FW load & init
	 * to complete is 5sec*/
	spin_lim = 500;

	cy_as_hal_create_sleep_channel(&channel);
	while (!cy_as_device_init_done) {

		cy_as_hal_sleep_on(&channel, 10) ;

		if (spin_lim-- <= 0) {
			cy_as_hal_print_message(
			"<1>\n_e_r_r_o_r!: "
			"wait for FW init has timed out !!!");
			break;
		}
	}
	cy_as_hal_destroy_sleep_channel(&channel);

	if (spin_lim > 0)
		cy_as_hal_print_message(
			"cy_as_device: astoria firmware is loaded\n");

	ret = cy_as_misc_get_firmware_version(cy_as_dev->dev_handle,
		&ver_data, 0, 0);
	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: cannot get firmware "
			"version. reason code: %d\n", ret);
		goto done;
	}

	if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x06))
		str = "nand and SD/MMC.";
	else if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x08))
		str = "nand and CEATA.";
	else if (ver_data.media_type & 0x01)
		str = "nand.";
	else if (ver_data.media_type & 0x08)
		str = "CEATA.";
	else
		str = "SD/MMC.";

	cy_as_hal_print_message("<1> cy_as_device:_firmware version: %s "
		"major=%d minor=%d build=%d,\n_media types supported:%s\n",
		((ver_data.is_debug_mode) ? "debug" : "release"),
		ver_data.major, ver_data.minor, ver_data.build, str);

/*	printk("%s: lock address is 0x%x\n", __func__, (unsigned int)&cy_as_dev->common_lock); */
/*	spin_lock_init(&cy_as_dev->common_lock); */
/*	printk("%s: lock address is 0x%x\n", __func__, (unsigned int)&cy_as_dev->common_lock); */

	printk("%s: lock address is 0x%x\n", __func__, (unsigned int)&cy_as_dev->common_sema);
	sema_init(&cy_as_dev->common_sema, 1);
	sema_init(&cy_as_dev->wq_sema, 1);
	printk("%s: lock address is 0x%x\n", __func__, (unsigned int)&cy_as_dev->common_sema);
	/* done now */
	cy_as_device_controller = cy_as_dev;



	return 0;

done:
	if (cy_as_dev)
		cyasdevice_deinit(cy_as_dev);

	return -EINVAL;
}

cy_as_device_handle cyasdevice_getdevhandle(void)
{
	if (cy_as_device_controller) {
		#ifdef CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL
			cy_as_hal_config_c_s_mux();
		#endif

		return cy_as_device_controller->dev_handle;
	}
	return NULL;
}
EXPORT_SYMBOL(cyasdevice_getdevhandle);

cy_as_hal_device_tag cyasdevice_gethaltag(void)
{
	if (cy_as_device_controller)
		return (cy_as_hal_device_tag)
			cy_as_device_controller->hal_tag;

	return NULL;
}
EXPORT_SYMBOL(cyasdevice_gethaltag);

static int	g_first_error_enter;
static int	g_first_error_leave;

int cyasdevice_enter_standby(void)
{
	int retval;
	if (f_isStandby == cy_false) {
		/* cy_as_acquire_common_lock(); */
		/* cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s mode\n", __func__) ; */
		retval = cy_as_misc_enter_standby(cy_as_device_controller->dev_handle, cy_false, 0, 0);
		if (retval != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s: error  - %d\n", __func__, retval) ;
			if (!g_first_error_enter)
				g_first_error_enter = retval;
		}
		/* msleep(5); */

		f_isStandby = cy_true;
		/* cy_as_release_common_lock(); */
		cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s  end mode\n", __func__) ;

		if (g_first_error_enter || g_first_error_leave)
			cy_as_hal_print_message(KERN_ERR
			"[%s] 1st error %d - %d\n", __func__,
			g_first_error_enter, g_first_error_leave) ;
	}
	return 0;
}
EXPORT_SYMBOL(cyasdevice_enter_standby) ;

int cyasdevice_leave_standby(void)
{
	int retval = 0;
	uint16_t v1, v2 ;
	uint32_t count = 20 ;

	if (f_isStandby == cy_true) {
		cy_as_device_init_done = 0;

		/* cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s mode \n", __func__) ; */
		retval = cy_as_misc_leave_standby(cy_as_device_controller->dev_handle, cy_as_bus_1);
		if (retval != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s: error  - %d\n", __func__, retval) ;
			if (!g_first_error_leave)
				g_first_error_leave = retval;
		}

		while (cy_as_device_init_done == 0 && count--)
			msleep(1);

		if (cy_as_device_init_done == 0) {
			gpio_set_value(WB_WAKEUP, 0);
			msleep(1);
			gpio_set_value(WB_WAKEUP, 1);
			msleep(10);
		}
		/* cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s  count=%d\n", __func__,count) ; */

		count = 0x08;
		while (count) {
			v1 = cy_as_hal_read_register(cy_as_device_controller->hal_tag, CY_AS_MEM_RST_CTRL_REG);
			if ((v1 & CY_AS_MEM_RST_RSTCMPT) == 0)
				cy_as_hal_print_message(
				KERN_ERR"Initialization Message Received,but reset complete bit still not set\n");
			else {
				v2 = cy_as_hal_read_register(cy_as_device_controller->hal_tag, CY_AS_MEM_P0_VM_SET);
				if (v2 & CY_AS_MEM_P0_VM_SET_CFGMODE)
					cy_as_hal_print_message(
					KERN_ERR"Initialization Message Received,but config bit still not set\n");
				else
					break;
			}
			count--;
			msleep(1);
		}
		cy_as_hal_print_message(KERN_ERR"[cyasdevice.c] %s end mode  count=%d\n", __func__, count) ;

		f_isStandby = cy_false;

		if (cy_as_device_init_done == 0) {
			if (!g_first_error_leave)
				g_first_error_leave = 0x7F;
			retval = 0x7F;
		} else
			retval = 0;

		if (g_first_error_enter || g_first_error_leave)
			cy_as_hal_print_message(KERN_ERR
			"[%s] 1st error %d - %d\n", __func__,
			g_first_error_enter, g_first_error_leave) ;
	}
	return retval;
}
EXPORT_SYMBOL(cyasdevice_leave_standby) ;

/**********************************************/
/*			SD Card Detect Routine by Samsung */
/**********************************************/
extern int  cyasblkdev_blk_init(int fmajor, int fsearch);
extern void  cyasblkdev_blk_exit(void);

void cyasdevice_spoof_removal(void)
{
	cyasblkdev_blk_exit();
	cy_as_device_controller->cy_work->f_status = 0;
}
EXPORT_SYMBOL(cyasdevice_spoof_removal);

void cyasdevice_spoof_insertion(void)
{
	uint ret = 0;
	ret = cyasblkdev_blk_init(0, 0);
	if (ret != CY_AS_ERROR_SUCCESS)
		printk(KERN_ERR "%s: unable to initialize block device\n", __func__);
	cy_as_device_controller->cy_work->f_status = 1;
}
EXPORT_SYMBOL(cyasdevice_spoof_insertion);


int cyasdevice_reload_firmware(int mtp_mode)
{
	cyasdevice *cy_as_dev = cy_as_device_controller ;
	int		 ret	= 0 ;
	cy_as_device_config config ;
	cy_as_hal_sleep_channel channel ;
	/* cy_as_get_firmware_version_data ver_data = {0}; */
	/* const char *str = "" ; */
	int spin_lim;

	cy_as_hal_print_message("<r>_cy_as_device deinitialize called\n") ;

	/* cyasdevice_leave_standby(); */

#if 0/* def CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL */
	/* ret = cy_as_misc_reset(cy_as_dev->dev_handle, cy_as_reset_hard, cy_false, 0, 0); */
	/* if (ret != CY_AS_ERROR_SUCCESS) { */
	/*	cy_as_hal_print_message("<1>_cy_as_device: cy_as_misc_reset failed. reason code: %d\n", ret) ; */
	/* } */
	if (cy_as_dev) {
		/* stop west_brige */
		if (cy_as_dev->dev_handle) {
			cy_as_hal_print_message("<r>_cy_as_device: "
			    "cy_as_misc_destroy_device called\n") ;
		    if (cy_as_misc_destroy_device(cy_as_dev->dev_handle) !=
			    CY_AS_ERROR_SUCCESS)
			    cy_as_hal_print_message(
				    "<1>_cy_as_device: destroying failed\n");
	    }

		if (cy_as_dev->hal_tag) {
			if (cy_as_hal_c110_pnand_stop(dev_handle_name, cy_as_dev->hal_tag) != 0)
				cy_as_hal_print_message("<r>_cy_as_device: stopping C110 PNAND HAL failed\n");
		}

		cy_as_hal_print_message("<r>_cy_as_device:HAL layer stopped\n") ;
	}

	/* Init the HAL & CyAsDeviceHandle */
	/* start OMAP HAL init instsnce */
		if (!cy_as_hal_c110_pnand_start(dev_handle_name, &(cy_as_dev->hal_tag), cy_false)) {
			cy_as_hal_print_message("<1>_cy_as_device: start C110 HAL failed\n") ;
			goto reload_done;
		}

	/* Now create the device */
	if (cy_as_misc_create_device(&(cy_as_dev->dev_handle),
		cy_as_dev->hal_tag) != CY_AS_ERROR_SUCCESS) {

		cy_as_hal_print_message(
			"<1>_cy_as_device: create device failed\n") ;
		goto reload_done ;
	}
#else
	ret = cy_as_misc_reset(cy_as_dev->dev_handle,
		cy_as_reset_hard, cy_true, 0, 0);
	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message(
		"<1>_cy_as_device: cy_as_misc_reset failed. reason code: %d\n", ret) ;
	}

	gpio_set_value(WB_RESET, 0);
	mdelay(10);
	gpio_set_value(WB_RESET, 1);
	mdelay(50);

#endif
	if (mtp_mode == 1)
		firmware_number = 1;
	else if (mtp_mode == 0)
		firmware_number = 0;

	f_isStandby = cy_false;

	cy_as_device_init_done = 0;

	cy_as_misc_set_log_level(8);

	cy_as_hal_print_message("<r>_cy_as_device initialize called\n") ;


	memset(&config, 0, sizeof(config));
	config.dmaintr = cy_true;

	ret = cy_as_misc_configure_device(cy_as_dev->dev_handle, &config) ;
	if (ret != CY_AS_ERROR_SUCCESS) {

		cy_as_hal_print_message(
			"<1>_cy_as_device: configure device "
			"failed. reason code: %d\n", ret) ;
		goto reload_done;
	}

	ret = cy_as_misc_register_callback(cy_as_dev->dev_handle,
		cy_misc_callback) ;
	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: "
			"cy_as_misc_register_callback failed. "
			"reason code: %d\n", ret) ;
		goto reload_done;
	}

#if 0
	/* Load the firmware */
	ret = request_firmware(&fw_entry,
		"west bridge fw", &westbridge_pd->dev);
	if (ret) {
		cy_as_hal_print_message("cy_as_device: "
			"request_firmware failed return val = %d\n", ret);
	} else {
		cy_as_hal_print_message("cy_as_device: "
			"got the firmware %d size=0x%x\n", ret, fw_entry->size);

		ret = cy_as_misc_download_firmware(
			cy_as_dev->dev_handle,
			fw_entry->data,
			fw_entry->size ,
			0, 0) ;
	}
	release_firmware(fw_entry);
#endif
#ifdef WESTBRIDGE_ANTIOCH_CSP
	if (firmware_number == 0) {
		cy_as_hal_print_message("<r>reload_fw: firmware_number=0,\
			download firmware cyanfw_sd_mmc_rel_csp_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyanfw_sd_mmc_rel_csp_silicon_array.fw_image,
			CYANFW_SD_MMC_REL_CSP_SILICON_SIZE , 0, 0) ;
	} else {
		cy_as_hal_print_message("<r>reload_fw: firmware_number=0,\
			download firmware cyanfw_mtp_sd_mmc_rel_csp_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyanfw_mtp_sd_mmc_rel_csp_silicon_array.fw_image,
			CYANFW_MTP_SD_MMC_REL_CSP_SILICON_SIZE , 0, 0) ;
	}
#endif
#ifdef WESTBRIDGE_ANTIOCH
	if (firmware_number == 0) {
		cy_as_hal_print_message("<r>reload_fw: firmware_number=0,\
			download firmware cyanfw_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyanfw_sd_mmc_rel_silicon_array.fw_image,
			CYANFW_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	} else {
		cy_as_hal_print_message("<r>reload_fw: firmware_number=0,\
			download firmware cyanfw_mtp_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyanfw_mtp_sd_mmc_rel_silicon_array.fw_image,
			CYANFW_MTP_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	}
#endif
#ifdef WESTBRIDGE_ASTORIA
	if (firmware_number == 0) {
		cy_as_hal_print_message(
			"<r>reload_fw: firmware_number=0,\
			download firmware cyastfw_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyastfw_sd_mmc_rel_silicon_array.fw_image,
			CYASTFW_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	} else {
		cy_as_hal_print_message(
		"<r>reload_fw: firmware_number=0,\
		download firmware cyastfw_mtp_sd_mmc_rel_silicon_array image\n") ;
		ret = cy_as_misc_download_firmware(cy_as_dev->dev_handle,
			cyastfw_mtp_sd_mmc_rel_silicon_array.fw_image,
			CYASTFW_MTP_SD_MMC_REL_SILICON_SIZE , 0, 0) ;
	}
#endif

	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: cannot download "\
			"firmware. reason code: %d\n", ret) ;
		goto reload_done;
	}

	/* spin until the device init is completed */
	/* 50 -MAX wait time for the FW load & init
	 * to complete is 5sec*/
	spin_lim = 500;

	cy_as_hal_create_sleep_channel(&channel) ;
	while (!cy_as_device_init_done) {

		cy_as_hal_sleep_on(&channel, 10) ;

		if (spin_lim-- <= 0) {
			cy_as_hal_print_message(
			"%s: ERROR!: wait for FW init "
				"has timed out !!!", __func__);
			break;
		}
	}
	cy_as_hal_destroy_sleep_channel(&channel) ;

	if (spin_lim > 0)
		cy_as_hal_print_message(
			"cy_as_device: astoria firmware is loaded\n") ;
#if 0
	ret = cy_as_misc_get_firmware_version(cy_as_dev->dev_handle,
		&ver_data, 0, 0) ;
	if (ret != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message("<1>_cy_as_device: cannot get firmware "
			"version. reason code: %d\n", ret) ;
		goto reload_done;
	}

	if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x06))
		str = "nand and SD/MMC." ;
	else if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x08))
		str = "nand and CEATA." ;
	else if (ver_data.media_type & 0x01)
		str = "nand." ;
	else if (ver_data.media_type & 0x08)
		str = "CEATA." ;
	else
		str = "SD/MMC." ;

	cy_as_hal_print_message("<1> cy_as_device:_firmware version: %s "
		"major=%d minor=%d build=%d,\n_media types supported:%s\n",
		((ver_data.is_debug_mode) ? "debug" : "release"),
		ver_data.major, ver_data.minor, ver_data.build, str) ;
#endif
	return 0 ;

reload_done:
	return -EINVAL ;
}
EXPORT_SYMBOL(cyasdevice_reload_firmware) ;

#ifdef __USE_CYAS_AUTO_SUSPEND__
int cyasdevice_wakeup_thread(int flag)
{
	int retval;

	/* down(&cy_as_device_controller->thread_sleep_sem); */
	down(&cy_as_device_controller->wq_sema);
	cy_as_device_controller->thread_sleep_count++;
	cy_as_device_controller->thread_sleep_enable = 0;
	/* up(&cy_as_device_controller->thread_sleep_sem); */
	retval = cyasdevice_leave_standby();
	/* if (!flag) */
	/*	wake_up(&cy_as_device_controller->thread_sleep_wq); */

	up(&cy_as_device_controller->wq_sema);

	if (retval) {
		cyasdevice_reload_firmware(-1);

		if (flag) {
			retval = cyasblkdev_start_sdcard();
			if (retval)
				cy_as_hal_print_message(KERN_ERR
				"%s: cyasblkdev_start_sdcard error %d\n",
				__func__, retval);
		}
	}

	return retval;
}
EXPORT_SYMBOL(cyasdevice_wakeup_thread) ;

int cyasdevice_enable_thread(void)
{
	/* down(&cy_as_device_controller->thread_sleep_sem); */
	down(&cy_as_device_controller->wq_sema);
	cy_as_device_controller->thread_sleep_enable = CYASDEVICE_THREAD_ENABLE;
	cy_as_device_controller->thread_sleep_count++;
	/* up(&cy_as_device_controller->thread_sleep_sem); */
	wake_up(&cy_as_device_controller->thread_sleep_wq);
	up(&cy_as_device_controller->wq_sema);
	return 0;
}
EXPORT_SYMBOL(cyasdevice_enable_thread) ;

int cyasdevice_disable_thread(void)
{
	/* down(&cy_as_device_controller->thread_sleep_sem); */
	down(&cy_as_device_controller->wq_sema);
	cy_as_device_controller->thread_sleep_count++;
	cy_as_device_controller->thread_sleep_enable = 0;
	/* up(&cy_as_device_controller->thread_sleep_sem); */
	/* wake_up(&cy_as_device_controller->thread_sleep_wq); */
	up(&cy_as_device_controller->wq_sema);

	return 0;
}
EXPORT_SYMBOL(cyasdevice_disable_thread) ;

/* queue worker thread */
static int cyasdevice_sleep_thread(void *d)
{
	/* cyasdevice *cy_as_dev = (cyasdevice *)cy_as_device_controller; */
	u32 qth_pid;
	DECLARE_WAITQUEUE(wait, current);
	/* DBGPRN_FUNC_NAME; */
	long	ret = 0;
	int	prev_counter = 0;
	int check_retry = 0;
	/*
	 * set iothread to ensure that we aren't put to sleep by
	 * the process freezing.  we handle suspension ourselves.
	 */
	daemonize("cyasdevice_sleep_thread");

	/* signal to queue_init() so it could contnue */
	complete(&cy_as_device_controller->thread_sleep_complete);

	down(&cy_as_device_controller->wq_sema);
	add_wait_queue(&cy_as_device_controller->thread_sleep_wq, &wait);

	cy_as_device_controller->thread_sleep_flags = CYASDEVICE_THREAD_SUSPENDED;
	cy_as_device_controller->thread_sleep_count = 0;

	qth_pid = current->pid;

	#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message(KERN_ERR
		"%s:%x started, cyasdevice:%p\n", __func__,
		qth_pid, cy_as_device_controller) ;
	#endif

	do {
		/* the thread wants to be woken up by signals as well */
		if (cy_as_device_controller->thread_sleep_flags & CYASDEVICE_THREAD_EXIT) {
			#ifdef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR"%s:got SLEEP_T_EXIT flag\n", __func__);
			#endif
			break;
		}

		#if 0
		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}
		#endif

		/* set_current_state(TASK_INTERRUPTIBLE); */
		/* yields to the next rdytorun proc,
		 * then goes back to sleep*/
		set_current_state(TASK_INTERRUPTIBLE);
		up(&cy_as_device_controller->wq_sema);
		if (cy_as_device_controller->thread_sleep_flags & CYASDEVICE_THREAD_TIMER)
			ret = schedule_timeout(msecs_to_jiffies(CYASDEVICE_STANDBY_TIMEOUT));
		else
			schedule();

		set_current_state(TASK_RUNNING);
		/* down(&cy_as_dev->thread_sleep_sem); */
		down(&cy_as_device_controller->wq_sema);
		if (cy_as_device_controller->thread_sleep_enable) {
			if (cy_as_device_controller->thread_sleep_flags & CYASDEVICE_THREAD_TIMER) {
				if (!ret) {  /*timeout */

					if (prev_counter == cy_as_device_controller->thread_sleep_count)
						check_retry++;
					else
						check_retry = 0;

					prev_counter = cy_as_device_controller->thread_sleep_count;
					if (check_retry > 3) {
						cy_as_device_controller->thread_sleep_flags = CYASDEVICE_THREAD_SUSPENDED;
						#ifdef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message(KERN_ERR
							"%s: chage mode to CYASDEVICE_THREAD_SUSPENDED\n",
							__func__);
						#endif
					}
				} else {  /*  wake_up */

					if (cy_as_device_controller->cy_work->f_status == 0) {
					#ifdef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message(KERN_ERR
						"%s: chage mode to CYASDEVICE_THREAD_SUSPENDED\n",
						__func__);
					#endif
						cy_as_device_controller->thread_sleep_flags =
							CYASDEVICE_THREAD_SUSPENDED;
					}
					check_retry = 0;
				}
			} else {
				if (cy_as_device_controller->cy_work->f_status) {
					#ifdef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message(KERN_ERR
						"%s: chage mode to CYASDEVICE_THREAD_TIMER\n",
						__func__);
					#endif
					cy_as_device_controller->thread_sleep_flags = CYASDEVICE_THREAD_TIMER;
				}
				check_retry = 0;
			}
		} else {
			check_retry = 0;
			#ifdef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
				"%s: CYASDEVICE_THREAD diabled\n", __func__);
			#endif
			cy_as_device_controller->thread_sleep_flags = CYASDEVICE_THREAD_SUSPENDED;
		}

		if (check_retry > 3) {
			check_retry = 0;
			/* to do for enter standby mode. */
			/* down(&cy_as_dev->wq_sema); */
			cyasdevice_enter_standby();
			msleep(20);
			/* up(&cy_as_dev->wq_sema); */
		}
	} while (1);

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&cy_as_device_controller->thread_sleep_wq, &wait);
	complete_and_exit(&cy_as_device_controller->thread_sleep_complete, 0);

	#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message(KERN_ERR"%s: is finished\n", __func__) ;
	#endif

	return 0;
}
#endif
/*init Westbridge device driver **/
static int __init cyasdevice_init(void)
{
	int retry_count = 3;
	int ret_val = 0;
	f_isStandby = cy_false;
/* Retry if Westbridge device is failed to initialize */
	do {
		ret_val = cyasdevice_initialize();
		if (ret_val == 0)
			break;
		msleep(100);
	} while (--retry_count);

	if (ret_val != 0)
		return -ENODEV;

#ifdef __USE_CYAS_AUTO_SUSPEND__
	{
		int ret;

		init_completion(
			&cy_as_device_controller->thread_sleep_complete);
		init_waitqueue_head(&cy_as_device_controller->thread_sleep_wq);
		/* init_MUTEX(&cy_as_device_controller->thread_sleep_sem); */
		sema_init(&cy_as_device_controller->thread_sleep_sem, 1);

		ret = kernel_thread(cyasdevice_sleep_thread,
			cy_as_device_controller, CLONE_KERNEL);
		if (ret >= 0) {
			/* wait until the thread is spawned */
			wait_for_completion(
				&cy_as_device_controller->thread_sleep_complete);

			/* reinitialize the completion */
			init_completion(
			&cy_as_device_controller->thread_sleep_complete);
			ret = 0;
		}
	}
#endif

	cy_as_device_controller->cy_wq = create_workqueue("wb_queue");
	if (cy_as_device_controller->cy_wq) {
		cy_as_device_controller->cy_work = (cy_work_t *)kmalloc(sizeof(cy_work_t),
						GFP_KERNEL);
		if (cy_as_device_controller->cy_work) {
			INIT_DELAYED_WORK(
				&cy_as_device_controller->cy_work->work,
				cy_wq_function);
			cy_as_device_controller->cy_work->f_status = 0x0;
			cy_as_device_controller->cy_work->f_reload = 1;
			cy_as_device_controller->cy_work->f_isrunning = 0;

#ifdef __USE_ISR_FOR_SD_DETECT__
			cy_as_hal_configure_sd_isr(
				(void *)cy_as_device_controller->hal_tag,
				(irq_handler_t) cyasdevice_int_handler);
#endif
			 if (cy_as_hal_detect_SD()) {
				cy_as_hal_print_message(
					"<1>%s: SD card scan:\n", __func__) ;
		/* queue_delayed_work(cy_as_device_controller->cy_wq,
		&cy_as_device_controller->cy_work->work, 10 * HZ); */
			} else {
				queue_delayed_work(
				cy_as_device_controller->cy_wq,
				&cy_as_device_controller->cy_work->work, HZ);
			}

		}
	}

	return 0;
}


static void __exit cyasdevice_cleanup(void)
{
#ifdef __USE_CYAS_AUTO_SUSPEND__
	cy_as_device_controller->thread_sleep_flags = CYASDEVICE_THREAD_EXIT;
	wake_up(&cy_as_device_controller->thread_sleep_wq);
	wait_for_completion(&cy_as_device_controller->thread_sleep_complete);
#endif
	kfree(cy_as_device_controller->cy_work);
	destroy_workqueue(cy_as_device_controller->cy_wq);

#ifdef __USE_ISR_FOR_SD_DETECT__
	cy_as_hal_free_sd_isr();
#endif
	cyasdevice_deinit(cy_as_device_controller);
}


MODULE_DESCRIPTION("west bridge device driver");
MODULE_AUTHOR("cypress semiconductor");
MODULE_LICENSE("GPL");

module_init(cyasdevice_init);
module_exit(cyasdevice_cleanup);

/*[]*/
