/* cyanblkdev_block.c - West Bridge Linux Block Driver source file
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

/*
 * Linux block driver implementation for Cypress West Bridge.
 * Based on the mmc block driver implementation by Andrew Christian
 * for the linux 2.6.26 kernel.
 * mmc_block.c, 5/28/2002
 */

/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *		  28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>
/*#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)*/
#include <linux/slab.h>
/*#endif*/
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>

#include <asm/system.h>
/*#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)*/
#include <linux/uaccess.h>
/*#else
#include  < asm/uaccess.h>
#endif*/

#include <linux/scatterlist.h>
#include <linux/time.h>
#include <linux/signal.h>
#include <linux/delay.h>

#include "cyasblkdev_queue.h"

#ifndef __USE_SYNC_FUNCTION__
/* #define __CYAS_USE_WORK_QUEUE */
#endif

#define CYASBLKDEV_SHIFT	0	/* Only a single partition. */
#define CYASBLKDEV_MAX_REQ_LEN	(256)
#define CYASBLKDEV_NUM_MINORS	(256 >> CYASBLKDEV_SHIFT)
#define CY_AS_TEST_NUM_BLOCKS   (64)
#define CYASBLKDEV_MINOR_0 1
#define CYASBLKDEV_MINOR_1 2
#define CYASBLKDEV_MINOR_2 3

static int major;

/* parameters passed from the user space */
static int vfat_search;
static int gl_vfat_offset[2][2] = { {-1, -1}, {-1, -1} };

static int private_partition_bus = -1;

static int private_partition_size = -1;

extern int cyasdevice_reload_firmware(int mtp_mode);
#ifdef __USE_CYAS_AUTO_SUSPEND__
extern int cyasdevice_wakeup_thread(int flag);
extern int cyasdevice_enable_thread(void);
extern int cyasdevice_disable_thread(void);
#endif
extern int cyasdevice_save_error(int error);

int cyasblkdev_start_sdcard(void);
int cyasblkdev_stop_sdcard(void);

/*
 * There is one cyasblkdev_blk_data per slot.
 */
struct cyasblkdev_blk_data {
	spinlock_t lock;
	int media_count[2];
	const struct block_device_operations *blkops;
	unsigned int usage;
	unsigned int suspended;

	/* handle to the west bridge device this handle, typdefed as *void  */
	cy_as_device_handle dev_handle;

	/* our custom structure, in addition to request queue,
	 * adds lock & semaphore items*/
	struct cyasblkdev_queue queue;

	/* 16 entries is enough given max request size
	 * 16 * 4K (64 K per request)*/
	struct scatterlist sg[16];

	/* non-zero enables printk of executed reqests */
	unsigned int dbgprn_flags;

	/*gen_disk for private, system disk */
	struct gendisk *system_disk;
	cy_as_media_type system_disk_type;
	cy_bool system_disk_read_only;
	cy_bool system_disk_bus_num;

	/* sector size for the medium */
	unsigned int system_disk_blk_size;
	unsigned int system_disk_first_sector;
	unsigned int system_disk_unit_no;
	unsigned int system_disk_disk_cap;

	/*gen_disk for bus 0 */
	struct gendisk *user_disk_0;
	cy_as_media_type user_disk_0_type;
	cy_bool user_disk_0_read_only;
	cy_bool user_disk_0_bus_num;

	/* sector size for the medium */
	unsigned int user_disk_0_blk_size;
	unsigned int user_disk_0_first_sector;
	unsigned int user_disk_0_unit_no;
	unsigned int user_disk_0_disk_cap;

	/*gen_disk for bus 1 */
	struct gendisk *user_disk_1;
	cy_as_media_type user_disk_1_type;
	cy_bool user_disk_1_read_only;
	cy_bool user_disk_1_bus_num;

	/* sector size for the medium */
	unsigned int user_disk_1_blk_size;
	unsigned int user_disk_1_first_sector;
	unsigned int user_disk_1_unit_no;
	unsigned int user_disk_1_disk_cap;

	unsigned char user_disk_0_serial_num[4];
	unsigned char user_disk_1_serial_num[4];
	unsigned char system_disk_serial_num[4];

	unsigned char user_disk_0_CID[16];
};


#ifdef __CYAS_USE_WORK_QUEUE
typedef struct {
	struct work_struct work;
} cy_work_t;

static struct workqueue_struct *cyas_blk_wq;
static cy_work_t *cyas_blk_work;
#endif

/* pointer to west bridge block data device superstructure */
static struct cyasblkdev_blk_data g_blk_dev;
static struct cyasblkdev_blk_data *gl_bd;

static int g_is_block_dev_ready;

/* static DECLARE_MUTEX (open_lock); */
static DEFINE_MUTEX(open_lock);

/* local forwardd declarationss  */
static cy_as_device_handle *cyas_dev_handle;
static void cyasblkdev_blk_deinit(struct cyasblkdev_blk_data *bd);

/*change debug print options */
#define DBGPRN_RD_RQ	   (1 < 0)
#define DBGPRN_WR_RQ		(1 < 1)
#define DBGPRN_RQ_END	  (1 < 2)

int blkdev_ctl_dbgprn(int prn_flags)
{
	int cur_options = gl_bd->dbgprn_flags;

	DBGPRN_FUNC_NAME;

	/* set new debug print options */
	gl_bd->dbgprn_flags = prn_flags;

	/* return previous */
	return cur_options;
}

EXPORT_SYMBOL(blkdev_ctl_dbgprn);

static struct cyasblkdev_blk_data *cyasblkdev_blk_get(struct gendisk *disk)
{
	struct cyasblkdev_blk_data *bd;

	DBGPRN_FUNC_NAME;

	/* down (&open_lock); */
	mutex_lock(&open_lock);
	bd = disk->private_data;

	if (bd && (bd->usage == 0))
		bd = NULL;

	if (bd) {
		bd->usage++;
#ifndef NBDEBUG
		cy_as_hal_print_message("cyasblkdev_blk_get: usage = %d\n",
					bd->usage);
#endif
	}
	/* up (&open_lock); */
	mutex_unlock(&open_lock);
	return bd;
}

static void cyasblkdev_blk_put(struct cyasblkdev_blk_data *bd)
{
	int ret;
	DBGPRN_FUNC_NAME;

	/* down (&open_lock); */
	mutex_lock(&open_lock);
	if (bd && (bd == gl_bd)) {
		bd->usage--;
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    (" cyasblkdev_blk_put , bd->usage= %d\n", bd->usage);
#endif
	} else {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    ("cyasblkdev: blk_put(bd) on bd = NULL!\n");
#endif
		/* up (&open_lock); */
		mutex_unlock(&open_lock);
		return;
	}

	if (bd->usage == 0) {
		if (bd->queue.queue != NULL)
			blk_cleanup_queue(bd->queue.queue);

		if (bd->user_disk_0 != NULL)
			put_disk(bd->user_disk_0);
		if (bd->user_disk_1 != NULL)
			put_disk(bd->user_disk_1);
		if (bd->system_disk != NULL)
			put_disk(bd->system_disk);
#ifdef __USE_CYAS_AUTO_SUSPEND__
		cyasdevice_wakeup_thread(1);
#endif

		ret = cy_as_storage_release(bd->dev_handle, 0, 0, 0, 0);
		if (CY_AS_ERROR_SUCCESS != ret) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("cyasblkdev: cannot release bus 0 - %d\n",
			     ret);
#endif
		}

		ret = cy_as_storage_release(bd->dev_handle, 1, 0, 0, 0);
		if (CY_AS_ERROR_SUCCESS != ret) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("cyasblkdev: cannot release bus 1 - %d\n",
			     ret);
#endif
		}

		ret = cy_as_storage_stop(bd->dev_handle, 0, 0);
		if (CY_AS_ERROR_SUCCESS != ret) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("cyasblkdev: cannot stop storage stack - %d\n",
			     ret);
#endif
		}
#ifdef __CY_ASTORIA_SCM_KERNEL_HAL__
		/* If the SCM Kernel HAL is being used, disable the use
		 * of scatter/gather lists at the end of block driver usage.
		 */
		cy_as_hal_disable_scatter_list(cyasdevice_gethaltag());
#endif

		/*ptr to global struct cyasblkdev_blk_data */
		/* kfree (bd); */
		gl_bd = NULL;
	}
#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message("cyasblkdev (blk_put): usage = %d\n",
				bd->usage);
#endif
	/* up (&open_lock); */
	mutex_unlock(&open_lock);
}

static int cyasblkdev_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct cyasblkdev_blk_data *bd = cyasblkdev_blk_get(bdev->bd_disk);
	int ret = -ENXIO;
	int bus_num = 1;
	unsigned char tempbuf[32];
	unsigned char *serial_num = tempbuf;

	DBGPRN_FUNC_NAME;

	if (bd) {
		ret = 0;

		if (bdev->bd_disk == bd->user_disk_0) {
			if ((mode & FMODE_WRITE)
			    && bd->user_disk_0_read_only) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("device marked as readonly "
				     "and write requested\n");
#endif

				cyasblkdev_blk_put(bd);
				ret = -EROFS;
			}
			bus_num = bd->user_disk_0_bus_num;
			serial_num = bd->user_disk_0_serial_num;
		} else if (bdev->bd_disk == bd->user_disk_1) {
			if ((mode & FMODE_WRITE)
			    && bd->user_disk_1_read_only) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("device marked as readonly "
				     "and write requested\n");
#endif

				cyasblkdev_blk_put(bd);
				ret = -EROFS;
			}
			bus_num = bd->user_disk_1_bus_num;
			serial_num = bd->user_disk_1_serial_num;
		} else if (bdev->bd_disk == bd->system_disk) {
			if ((mode & FMODE_WRITE)
			    && bd->system_disk_read_only) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("device marked as readonly "
				     "and write requested\n");
#endif

				cyasblkdev_blk_put(bd);
				ret = -EROFS;
			}
			bus_num = bd->system_disk_bus_num;
			serial_num = bd->system_disk_serial_num;
		}
#if 0
		if (bd->usage == 2) {
#ifdef __USE_CYAS_AUTO_SUSPEND__
			cyasdevice_wakeup_thread(1);
#endif
			reg_data.buf_p = tempbuf;
			reg_data.length = 16;
			retval =
			    cy_as_storage_sd_register_read(bd->dev_handle,
							   bus_num, 0,
							   cy_as_sd_reg_CID,
							   &reg_data, 0,
							   0);
			if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
							"cyasblkdev_blk_open : fail in read CID register (%d)\n",
							ret);
#endif
				cyasblkdev_blk_put(bd);
				ret = -ENXIO;
			} else {
				cy_as_hal_print_message(KERN_ERR
							"cyasblkdev_blk_open : serial num = 0x%x 0x%x 0x%x 0x%x\n",
							tempbuf[9],
							tempbuf[10],
							tempbuf[11],
							tempbuf[12]);
				if (memcmp(serial_num, &tempbuf[9], 4)) {
					retval =
					    cy_as_misc_storage_changed(bd->
								       dev_handle,
								       0,
								       0);
					if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message
						    (KERN_ERR
						     "cyasblkdev_blk_open : fail in cy_as_misc_storage_changed (%d) \n",
						     ret);
#endif
					}
					memcpy(serial_num, &tempbuf[9], 4);
					check_disk_change(bdev);
				}
			}
		}
#endif
	}

	return ret;
}

static int cyasblkdev_blk_release(struct gendisk *disk, fmode_t mode)
{
	struct cyasblkdev_blk_data *bd = disk->private_data;

	DBGPRN_FUNC_NAME;

	cyasblkdev_blk_put(bd);
	return 0;
}

static int cyasblkdev_blk_ioctl(struct block_device *bdev,
				fmode_t mode,
				unsigned int cmd, unsigned long arg)
{
	DBGPRN_FUNC_NAME;

	if (cmd == HDIO_GETGEO) {
		/*for now  we only process geometry IOCTL */
		struct hd_geometry geo;

		memset(&geo, 0, sizeof(struct hd_geometry));

		geo.cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = get_start_sect(bdev);

		/* copy to user space */
		return copy_to_user((void __user *) arg, &geo, sizeof(geo))
		    ? -EFAULT : 0;
	}

	return -ENOTTY;
}

/* Media_changed block_device opp
 * this one is called by kernel to confirm if the media really changed
 * as we indicated by issuing check_disk_change() call */
int cyasblkdev_media_changed(struct gendisk *gd)
{
	struct cyasblkdev_blk_data *bd;

#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message(KERN_ERR
				"cyasblkdev_media_changed() is called\n");
#endif

	if (gd)
		bd = gd->private_data;
	else {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    ("cyasblkdev_media_changed() is called, "
		     "but gd is null\n");
#endif
	}

	/* return media change state "1" yes, 0 no */
	return 0;
}

/*  this one called by kernel to give us a chence
 * to prep the new media before it starts to rescaning
 * of the newlly inserted SD media */
int cyasblkdev_revalidate_disk(struct gendisk *gd)
{
	/*int (*revalidate_disk) (struct gendisk *); */

#ifndef WESTBRIDGE_NDEBUG
	if (gd)
		cy_as_hal_print_message
		    ("cyasblkdev_revalidate_disk() is called, "
		     "(gl_bd->usage:%d)\n", gl_bd->usage);
#endif

	/* 0 means ok, kern can go ahead with partition rescan */
	return 0;
}


/*standard block device driver interface */
static struct block_device_operations cyasblkdev_bdops = {
	.open = cyasblkdev_blk_open,
	.release = cyasblkdev_blk_release,
	.ioctl = cyasblkdev_blk_ioctl,
	/* .getgeo              = cyasblkdev_blk_getgeo, */
	/* added to support media removal( real and simulated) media */
	.media_changed = cyasblkdev_media_changed,
	/* added to support media removal( real and simulated) media */
	.revalidate_disk = cyasblkdev_revalidate_disk,
	.owner = THIS_MODULE,
};

#if 0
/* west bridge block device prep request function */
static int cyasblkdev_blk_prep_rq(struct cyasblkdev_queue *bq,
				  struct request *req)
{
	struct cyasblkdev_blk_data *bd = bq->data;
	int stat = BLKPREP_OK;

	DBGPRN_FUNC_NAME;

	/* If we have no device, we haven't finished initialising. */
	if (!bd || !bd->dev_handle) {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"cyasblkdev %s: killing request - no device/host\n",
					req->rq_disk->disk_name);
#endif
		stat = BLKPREP_KILL;
	}

	if (bd->suspended) {
		blk_plug_device(bd->queue.queue);
		stat = BLKPREP_DEFER;
	}

	/* Check for excessive requests. */
#if defined(__FOR_KERNEL_2_6_35__) || defined(__FOR_KERNEL_2_6_32__)
	if (blk_rq_pos(req) + blk_rq_sectors(req) >
	    get_capacity(req->rq_disk)) {
		cy_as_hal_print_message(KERN_ERR
					"cyasblkdev: bad request address\n");
		stat = BLKPREP_KILL;
	}
#else
	if (req->sector + req->nr_sectors > get_capacity(req->rq_disk)) {
		cy_as_hal_print_message
		    ("cyasblkdev: bad request address\n");
		stat = BLKPREP_KILL;
	}
#endif
	return stat;
}
#endif
#ifdef __CYAS_USE_WORK_QUEUE
static void cyasblkdev_workqueue(struct work_struct *work)
{
	struct cyasblkdev_blk_data *bd = gl_bd;
#ifdef __DEBUG_BLK_LOW_LEVEL__2
	cy_as_hal_print_message(KERN_ERR "%s : execute next_queue\n",
				__func__);
#endif
	spin_lock_irq(&bd->lock);

	/*elevate next request, if there is one */
	if (1) {		/* if  (!blk_queue_plugged (bd->queue.queue)) { */
		/* queue is not plugged */
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
		bd->queue.req = blk_fetch_request(bd->queue.queue);
#else
		bd->queue.req = elv_next_request(bd->queue.queue);
#endif
#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR "%s blkdev_callback: "
					"blk_fetch_request():%p\n",
					__func__, bd->queue.req);
#endif
	} else
		bd->queue.req = NULL;

	spin_unlock_irq(&bd->lock);
	if (bd->queue.req) {
#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR
					"%s blkdev_callback: about to "
					"call issue_fn:%p\n", __func__,
					bd->queue.req);
#endif

		bd->queue.issue_fn(&bd->queue, bd->queue.req);
	}
}
#endif

#ifndef __USE_SYNC_FUNCTION__
/*west bridge storage async api on_completed callback */
static void cyasblkdev_issuecallback(
					    /* Handle to the device completing the storage operation */
					    cy_as_device_handle handle,
					    /* The media type completing the operation */
					    cy_as_media_type type,
					    /* The device completing the operation */
					    uint32_t device,
					    /* The unit completing the operation */
					    uint32_t unit,
					    /* The block number of the completed operation */
					    uint32_t block_number,
					    /* The type of operation */
					    cy_as_oper_type op,
					    /* The error status */
					    cy_as_return_status_t status)
{
	struct cyasblkdev_blk_data *bd = gl_bd;
	uint32_t req_nr_sectors = 0;
	int retry_cnt = 0;
	/* DBGPRN_FUNC_NAME; */
	if (status != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s: async r/w: op:%d failed with error %d at address %d\n",
					__func__, op, status,
					block_number);
#endif
	}
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
	req_nr_sectors = blk_rq_sectors(bd->queue.req);
#else
	req_nr_sectors = bd->queue.req->nr_sectors;
#endif

#ifdef __DEBUG_BLK_LOW_LEVEL__2
	cy_as_hal_print_message(KERN_ERR
				"%s calling blk_end_request from issue_callback "
				"req=0x%x, status=0x%x, nr_sectors=0x%x\n",
				__func__, (unsigned int) bd->queue.req,
				status, (unsigned int) req_nr_sectors);
#endif
	/* if (rq_data_dir (gl_bd->queue.req) != READ) { */
	/* if (op == cy_as_op_write) { */
	cy_as_release_common_lock();
	/* } */
	/* note: blk_end_request w/o __ prefix should
	 * not require spinlocks on the queue*/
	if (status != CY_AS_ERROR_SUCCESS) {
		cy_as_hal_print_message(KERN_ERR
					"%s: calling blk_end_request, with -EIO value",
					__func__);
		while (blk_end_request
		       (bd->queue.req, -EIO, req_nr_sectors * 512));
	} else {
		while (blk_end_request
		       (bd->queue.req, status, req_nr_sectors * 512)) {
			retry_cnt++;
		};
	}
#ifdef __DEBUG_BLK_LOW_LEVEL__2
	cy_as_hal_print_message(KERN_ERR
				"%s blkdev_callback: ended rq on %d sectors, "
				"with err:%d, n:%d times\n", __func__,
				(int) req_nr_sectors, status, retry_cnt);
#endif

#ifdef __CYAS_USE_WORK_QUEUE
	queue_work(cyas_blk_wq, &cyas_blk_work->work);
#else
	spin_lock_irq(&bd->lock);

	/*elevate next request, if there is one */
	if (1) {		/* if  (!blk_queue_plugged (bd->queue.queue)) { */
		/* queue is not plugged */
#if defined(__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
		bd->queue.req = blk_fetch_request(bd->queue.queue);
#else
		bd->queue.req = elv_next_request(bd->queue.queue);
#endif
#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR "%s blkdev_callback: "
					"blk_fetch_request():%p\n",
					__func__, bd->queue.req);
#endif
	} else
		bd->queue.req = NULL;

	spin_unlock_irq(&bd->lock);

	if (bd->queue.req) {

#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR
					"%s blkdev_callback: about to "
					"call issue_fn:%p\n", __func__,
					bd->queue.req);
#endif

		bd->queue.issue_fn(&bd->queue, bd->queue.req);
	}
#endif				/* #ifdef __CYAS_USE_WORK_QUEUE */
}
#endif

#ifdef __USE_SYNC_FUNCTION__
static int f_cyasblock_debug;
void cyasblkdev_set_debug_flag(int flag)
{
	f_cyasblock_debug = flag;
}

EXPORT_SYMBOL(cyasblkdev_set_debug_flag);
/* issue astoria blkdev request (issue_fn) */
static int cyasblkdev_blk_issue_rq(struct cyasblkdev_queue *bq,
				   struct request *req)
{
	struct cyasblkdev_blk_data *bd = bq->data;
	int index = 0;
	int ret = CY_AS_ERROR_SUCCESS;
	uint32_t req_sector = 0;
	uint32_t req_nr_sectors = 0;
	int bus_num = 1;
	int lcl_unit_no = 0;
	int retry_value = 1;

	DBGPRN_FUNC_NAME;

#ifdef __USE_CYAS_AUTO_SUSPEND__
	cyasdevice_wakeup_thread(1);
#endif
	/*
	 * will construct a scatterlist for the given request;
	 * the return value is the number of actually used
	 * entries in the resulting list. Then, this scatterlist
	 * can be used for the actual DMA prep operation.
	 */
	do {
		spin_lock_irq(&bd->lock);
		index = blk_rq_map_sg(bq->queue, req, bd->sg);
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
		req_sector = blk_rq_pos(req);
		req_nr_sectors = blk_rq_sectors(req);
#else
		req_sector = req->sector;
		req_nr_sectors = req->nr_sectors;
#endif

		if (req->rq_disk == bd->user_disk_0) {
			bus_num = bd->user_disk_0_bus_num;
			req_sector += bd->user_disk_0_first_sector;
			lcl_unit_no = bd->user_disk_0_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
			cy_as_hal_print_message(KERN_ERR
						"%s: request made to disk 0 for sector=%d, num_sectors=%d, unit_no=%d\n",
						__func__, req_sector,
						(int) req_nr_sectors,
						lcl_unit_no);
#endif
		} else if (req->rq_disk == bd->user_disk_1) {
			bus_num = bd->user_disk_1_bus_num;
			req_sector += bd->user_disk_1_first_sector;
			lcl_unit_no = bd->user_disk_1_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
			cy_as_hal_print_message(KERN_ERR
						"%s: request made to disk 1 for sector=%d, num_sectors=%d, unit_no=%d\n",
						__func__, req_sector,
						(int) req_nr_sectors,
						lcl_unit_no);
#endif
		} else if (req->rq_disk == bd->system_disk) {
			bus_num = bd->system_disk_bus_num;
			req_sector += bd->system_disk_first_sector;
			lcl_unit_no = bd->system_disk_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
			cy_as_hal_print_message(KERN_ERR
						"%s: request made to system disk for sector=%d, num_sectors=%d, unit_no=%d\n",
						__func__, req_sector,
						(int) req_nr_sectors,
						lcl_unit_no);
#endif
		}
#ifndef WESTBRIDGE_NDEBUG
		else {
			cy_as_hal_print_message
			    ("%s: invalid disk used for request\n",
			     __func__);
		}
#endif

		spin_unlock_irq(&bd->lock);

		/*printk("%s: pre-check handle 0x%x\n", __func__, bd->dev_handle); */
		if (!(bd->dev_handle))
			bd->dev_handle = cyasdevice_getdevhandle();

		if (req_nr_sectors == 0) {
			cy_as_hal_print_message(KERN_ERR
						"%s: Invalid Params req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x\n\n",
						__func__, req_sector,
						req_nr_sectors,
						(uint32_t) bd->sg);
			while (blk_end_request
			       (req, -EIO, req_nr_sectors * 512));
			/* bq->req = NULL ; */
			/* ret = CY_AS_ERROR_INVALID_BLOCK; */
			goto issue_rq_next;
		}
		/*
		   if( req_sector > (uint32_t)get_capacity(req->rq_disk))
		   {
		   cy_as_hal_print_message(KERN_ERR"%s: req_sector = %d  of  cap = %d\n", __func__, req_sector, (int)get_capacity(req->rq_disk));
		   req_sector = (uint32_t)get_capacity(req->rq_disk);
		   while (blk_end_request(req,  -EIO,   req_nr_sectors*512));

		   goto issue_rq_next;
		   }
		 */
		if ((bd->sg == NULL) || (sg_virt(bd->sg) == NULL)) {
			/* cy_as_hal_print_message (KERN_ERR"%s: Invalid valid address =0x%x, sg=0x%x \n\n",
			   __func__, (uint32_t)bd->sg, sg_virt (bd->sg)); */
			while (blk_end_request
			       (req, -EIO, req_nr_sectors * 512));
			/* bq->req = NULL ; */
			/* ret = CY_AS_ERROR_INVALID_BLOCK; */
			goto issue_rq_next;
		}

		if (rq_data_dir(req) == READ) {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
			cy_as_hal_print_message(KERN_ERR
						"%s: calling read() req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x, index=%d\n\n",
						__func__, req_sector,
						req_nr_sectors,
						(uint32_t) bd->sg, index);
#endif
			if (ret == CY_AS_ERROR_SUCCESS) {
				retry_value = 10;
				do {
#ifdef __USE_CYAS_AUTO_SUSPEND__
					cyasdevice_wakeup_thread(1);
#endif
					/* cy_as_acquire_common_lock (); */
					ret =
					    cy_as_storage_read(bd->
							       dev_handle,
							       bus_num, 0,
							       lcl_unit_no,
							       req_sector,
							       bd->sg,
							       req_nr_sectors);
					/* cy_as_release_common_lock (); */
					if ((ret ==
					     CY_AS_ERROR_ASYNC_PENDING)
					    || (ret ==
						CY_AS_ERROR_MEDIA_ACCESS_FAILURE)) {
						msleep(1);
#ifndef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message
						    (KERN_ERR
						     "%s: read again caused by %d, left count=%d , req_sector=%d\n",
						     __func__, ret,
						     retry_value,
						     req_sector);
#endif
					} else if (ret ==
						   CY_AS_ERROR_ENDPOINT_DISABLED) {
						cyasdevice_reload_firmware
						    (-1);
						cyasblkdev_start_sdcard();
					} else
						break;
				} while (retry_value--);
			} else {
				cy_as_hal_print_message(KERN_ERR
							"%s: cy_as_storage_read returned %d\n",
							__func__, ret);
			}

		} else {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
			cy_as_hal_print_message(KERN_ERR
						"%s: calling write() req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x\n",
						__func__, req_sector,
						req_nr_sectors,
						(uint32_t) bd->sg);
#endif
			if (ret == CY_AS_ERROR_SUCCESS) {
				retry_value = 10;
				do {
#ifdef __USE_CYAS_AUTO_SUSPEND__
					cyasdevice_wakeup_thread(1);
#endif
					/* cy_as_acquire_common_lock (); */
					ret =
					    cy_as_storage_write(bd->
								dev_handle,
								bus_num, 0,
								lcl_unit_no,
								req_sector,
								bd->sg,
								req_nr_sectors);
					/* cy_as_release_common_lock (); */
					if ((ret ==
					     CY_AS_ERROR_ASYNC_PENDING)
					    || (ret ==
						CY_AS_ERROR_MEDIA_ACCESS_FAILURE)) {
#ifndef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message
						    (KERN_ERR
						     "%s: write again caused by %d, left count=%d, req_sector=%d \n",
						     __func__, ret,
						     retry_value,
						     req_sector);
#endif
						msleep(1);
					} else if (ret ==
						   CY_AS_ERROR_ENDPOINT_DISABLED)
					{
						cyasdevice_reload_firmware
						    (-1);
						cyasblkdev_start_sdcard();
					} else
						break;
				} while (retry_value--);


			} else {
				cy_as_hal_print_message(KERN_ERR
							"%s: cy_as_storage_write returned %d sector %d\n",
							__func__, ret,
							req_sector);
			}
		}
		if (ret == CY_AS_ERROR_INVALID_HANDLE) {
			uint32_t temp_handle;
			temp_handle = (uint32_t) cyasdevice_getdevhandle();
			/*cy_as_hal_print_message (KERN_ERR"%s: invalid handle 0x%x, temp handle=0x%x\n", __func__, bd->dev_handle, temp_handle); */
			/*debug only */
			/* panic (" (david's panic) west bridge: invalid handle panic"); */
			while (blk_end_request
			       (req, -EIO, req_nr_sectors * 512));
		} else if (ret != CY_AS_ERROR_SUCCESS) {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
			/*      cy_as_hal_print_message (KERN_ERR"%s:ending i/o request on reg:%x, %d\n", __func__,  (uint32_t)req, ret); */
#endif

			while (blk_end_request
			       (req, -EIO, req_nr_sectors * 512));

			if ((ret == CY_AS_ERROR_NO_FIRMWARE)
			    || (ret == CY_AS_ERROR_TIMEOUT)) {
				/* cyasdevice_save_error (ret); */
				cyasdevice_reload_firmware(-1);
				ret = cyasblkdev_start_sdcard();
				if (ret)
					cy_as_hal_print_message(KERN_ERR
								"%s: cyasblkdev_start_sdcard error %d\n",
								__func__,
								ret);

				if (ret == CY_AS_ERROR_TIMEOUT)
					ret = CY_AS_ERROR_NO_SUCH_DEVICE;
			}
		} else {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
			cy_as_hal_print_message(KERN_ERR
						"%s: CY_AS_SUCCESS\n\n",
						__func__);
#endif
			while (blk_end_request
			       (req, ret, req_nr_sectors * 512));
		}
	      issue_rq_next:
		spin_lock_irq(&bd->lock);

		/*elevate next request, if there is one */
		/* if  (!blk_queue_plugged (bd->queue.queue)) { */
		if (1) {
			/* queue is not plugged */
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			bq->req = req = blk_fetch_request(bq->queue);
#else
			bq->req = req = elv_next_request(bq->queue);
#endif
#ifdef __DEBUG_BLK_LOW_LEVEL__
			cy_as_hal_print_message(KERN_ERR
						"%s blkdev_callback: blk_fetch_request():%p\n",
						__func__, bq->req);
#endif
		} else
			bq->req = req = NULL;

		if (req)
			retry_value = 1;
		else
			retry_value = 0;
		spin_unlock_irq(&bd->lock);

	} while (retry_value);

#ifdef __USE_CYAS_AUTO_SUSPEND__
	cyasdevice_enable_thread();
#endif

	/* if (ret == CY_AS_ERROR_TIMEOUT)
	   {
	   cyasdevice_reload_firmware (-1);
	   } */
	return ret;
}
#else

/* issue astoria blkdev request (issue_fn) */
static int cyasblkdev_blk_issue_rq(struct cyasblkdev_queue *bq,
				   struct request *req)
{
	struct cyasblkdev_blk_data *bd = bq->data;
	int index = 0;
	int ret = CY_AS_ERROR_SUCCESS;
	uint32_t req_sector = 0;
	uint32_t req_nr_sectors = 0;
	int bus_num = 0;
	int lcl_unit_no = 0;
	int retry_value = 0;

	DBGPRN_FUNC_NAME;

	/*
	 * will construct a scatterlist for the given request;
	 * the return value is the number of actually used
	 * entries in the resulting list. Then, this scatterlist
	 * can be used for the actual DMA prep operation.
	 */
	spin_lock_irq(&bd->lock);
	index = blk_rq_map_sg(bq->queue, req, bd->sg);

#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
	req_sector = blk_rq_pos(req);
	req_nr_sectors = blk_rq_sectors(req);
#else
	req_sector = req->sector;
	req_nr_sectors = req->nr_sectors;
#endif

	if (req->rq_disk == bd->user_disk_0) {
		bus_num = bd->user_disk_0_bus_num;
		req_sector += bd->user_disk_0_first_sector;
		lcl_unit_no = bd->user_disk_0_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR
					"%s: request made to disk 0 "
					"for sector=%d, num_sectors=%d, unit_no=%d\n",
					__func__, req_sector,
					(int) req_nr_sectors, lcl_unit_no);
#endif
	} else if (req->rq_disk == bd->user_disk_1) {
		bus_num = bd->user_disk_1_bus_num;
		req_sector += bd->user_disk_1_first_sector;
		lcl_unit_no = bd->user_disk_1_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR
					"%s: request made to disk 1 for "
					"sector=%d, num_sectors=%d, unit_no=%d\n",
					__func__, req_sector,
					(int) req_nr_sectors, lcl_unit_no);
#endif
	} else if (req->rq_disk == bd->system_disk) {
		bus_num = bd->system_disk_bus_num;
		req_sector += bd->system_disk_first_sector;
		lcl_unit_no = bd->system_disk_unit_no;

#ifdef __DEBUG_BLK_LOW_LEVEL__
		cy_as_hal_print_message(KERN_ERR
					"%s: request made to system disk "
					"for sector=%d, num_sectors=%d, unit_no=%d\n",
					__func__, req_sector,
					(int) req_nr_sectors, lcl_unit_no);
#endif
	}
#ifndef WESTBRIDGE_NDEBUG
	else {
		cy_as_hal_print_message(KERN_ERR
					"%s: invalid disk used for request\n",
					__func__);
	}
#endif

	spin_unlock_irq(&bd->lock);

	if (req_nr_sectors == 0) {
		cy_as_hal_print_message(KERN_ERR
					"%s: Invalid Params req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x\n\n",
					__func__, req_sector,
					req_nr_sectors, (uint32_t) bd->sg);
		while (blk_end_request(req, -EIO, req_nr_sectors * 512));
		bq->req = NULL;
		return CY_AS_ERROR_INVALID_BLOCK;
	}

	if (req_sector > (uint32_t) get_capacity(req->rq_disk)) {
		cy_as_hal_print_message(KERN_ERR
					"%s: req_sector = %d  of  cap = %d\n",
					__func__, req_sector,
					(int) get_capacity(req->rq_disk));
		req_sector = (uint32_t) get_capacity(req->rq_disk);
	}

	if (lcl_unit_no > 3) {
		cy_as_hal_print_message(KERN_ERR
					"%s: req_nr_sectors = %d\n",
					__func__, req_nr_sectors);
		while (blk_end_request
		       (req, (ret == CY_AS_ERROR_SUCCESS),
			req_nr_sectors * 512));
		bq->req = NULL;
		return CY_AS_ERROR_INVALID_BLOCK;
	}

	if ((bd->sg == NULL) || (sg_virt(bd->sg) == NULL)) {
		cy_as_hal_print_message(KERN_ERR
					"%s: Invalid valid address =0x%x, sg=0x%x \n\n",
					__func__, (uint32_t) bd->sg,
					(uint32_t) sg_virt(bd->sg));
		while (blk_end_request(req, -EIO, req_nr_sectors * 512));
		bq->req = NULL;
		return CY_AS_ERROR_INVALID_BLOCK;
	}


	cy_as_acquire_common_lock();
	if (rq_data_dir(req) == READ) {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
		cy_as_hal_print_message(KERN_ERR "%s: calling readasync() "
					"req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x index=%d\n\n",
					__func__, req_sector,
					req_nr_sectors, (uint32_t) bd->sg,
					index);
#endif

#if 1				/* def  __CYAS_USE_WORK_QUEUE */
		retry_value = 1000;
		do {
#ifdef __USE_CYAS_AUTO_SUSPEND__
			cyasdevice_wakeup_thread(1);
#endif
			/* cy_as_acquire_common_lock (); */
			ret =
			    cy_as_storage_read_async(bd->dev_handle,
						     bus_num, 0,
						     lcl_unit_no,
						     req_sector, bd->sg,
						     req_nr_sectors,
						     (cy_as_storage_callback)
						     cyasblkdev_issuecallback);
			/* cy_as_release_common_lock (); */
			if (ret == CY_AS_ERROR_ASYNC_PENDING) {
#if 1				/* ndef WESTBRIDGE_NDEBUG */
				cy_as_hal_print_message(KERN_ERR
							"%s: readasync again caused by %d, left count=%d , req_sector=%d\n",
							__func__, ret,
							retry_value,
							req_sector);
#endif
				/* cy_as_release_common_lock (); */
				msleep(5);
			} else
				break;
		} while (retry_value--);
#else

		/* cy_as_acquire_common_lock (); */
		ret = cy_as_storage_read_async(bd->dev_handle, bus_num, 0,
					       lcl_unit_no, req_sector,
					       bd->sg, req_nr_sectors,
					       (cy_as_storage_callback)
					       cyasblkdev_issuecallback);
#endif
		if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			cy_as_hal_print_message(KERN_ERR
						"%s:readasync() error %d at "
						"address %ld, unit no %d\n",
						__func__, ret,
						(long) blk_rq_pos(req),
						lcl_unit_no);
#else
			cy_as_hal_print_message
			    ("%s:readasync () error %d at address %ld, unit no %d\n",
			     __func__, ret, req->sector, lcl_unit_no);
#endif
			cy_as_hal_print_message(KERN_ERR
						"%s:ending i/o request "
						"on reg:%x\n", __func__,
						(uint32_t) req);
#endif
			/* cy_as_release_common_lock (); */
			while (blk_end_request(req,
					       (ret ==
						CY_AS_ERROR_SUCCESS),
					       req_nr_sectors * 512));

			retry_value = 1;
		} else {
			return ret;
		}
	} else {
#ifdef __DEBUG_BLK_LOW_LEVEL__2
		cy_as_hal_print_message(KERN_ERR
					"%s: calling writeasync() "
					"req_sector=0x%x, req_nr_sectors=0x%x, bd->sg:%x\n\n",
					__func__, req_sector,
					req_nr_sectors, (uint32_t) bd->sg);
#endif

#if 1				/* def  __CYAS_USE_WORK_QUEUE */
		retry_value = 1000;
		do {
#ifdef __USE_CYAS_AUTO_SUSPEND__
			cyasdevice_wakeup_thread(1);
#endif
			/* cy_as_acquire_common_lock (); */
			ret =
			    cy_as_storage_write_async(bd->dev_handle,
						      bus_num, 0,
						      lcl_unit_no,
						      req_sector, bd->sg,
						      req_nr_sectors,
						      (cy_as_storage_callback)
						      cyasblkdev_issuecallback);
			if (ret == CY_AS_ERROR_ASYNC_PENDING) {
#if 0				/* ndef WESTBRIDGE_NDEBUG */
				cy_as_hal_print_message
				    ("%s: writeasync again caused by %d, left count=%d , req_sector=%d\n",
				     __func__, ret, retry_value,
				     req_sector);
#endif
				/* cy_as_release_common_lock (); */
				msleep(5);
			} else
				break;
		} while (retry_value--);
#else
		cy_as_acquire_common_lock();
		ret = cy_as_storage_write_async(bd->dev_handle, bus_num, 0,
						lcl_unit_no, req_sector,
						bd->sg, req_nr_sectors,
						(cy_as_storage_callback)
						cyasblkdev_issuecallback);
#endif

		if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			cy_as_hal_print_message(KERN_ERR
						"%s: write failed with "
						"error %d at address %ld, unit no %d\n",
						__func__, ret,
						(long) blk_rq_pos(req),
						lcl_unit_no);
#else
			cy_as_hal_print_message
			    ("%s: write failed with error %d at address %ld, unit no %d\n",
			     __FUNCTION__, ret, req->sector, lcl_unit_no);
#endif
#endif

			cy_as_release_common_lock();

			/*end IO op on this request(does both
			 * end_that_request_... _first & _last) */
			while (blk_end_request(req,
					       (ret ==
						CY_AS_ERROR_SUCCESS),
					       req_nr_sectors * 512));
			retry_value = 1;
		} else {
			return ret;
		}
	}

	cy_as_release_common_lock();
	while (retry_value) {
		spin_lock_irq(&bd->lock);

		/*elevate next request, if there is one */
		if (1) {	/* if  (!blk_queue_plugged (bd->queue.queue)) { */
			/* queue is not plugged */
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			bq->req = req = blk_fetch_request(bq->queue);
#else
			bq->req = req = elv_next_request(bq->queue);
#endif
#ifdef __DEBUG_BLK_LOW_LEVEL__
			cy_as_hal_print_message(KERN_ERR
						"%s blkdev_callback: blk_fetch_request():%p\n",
						__func__, bq->req);
#endif
		} else
			bq->req = req = NULL;

		if (req) {
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			req_nr_sectors = blk_rq_sectors(req);
#else
			req_nr_sectors = req->nr_sectors;	/* SECT_NUM_TRANSLATE (req->nr_sectors); */
#endif
		} else
			retry_value = 0;

		spin_unlock_irq(&bd->lock);

		if (retry_value) {
			while (blk_end_request
			       (req, -EIO, req_nr_sectors * 512));
		}
	}

	return ret;
}
#endif				/*               #ifdef __USE_SYNC_FUNCTION__ */

static unsigned long
    dev_use[CYASBLKDEV_NUM_MINORS / (8 * sizeof(unsigned long))];


/* storage event callback (note: called in astoria isr context) */
static void cyasblkdev_storage_callback(cy_as_device_handle dev_h,
					cy_as_bus_number_t bus,
					uint32_t device,
					cy_as_storage_event evtype,
					void *evdata)
{
#ifdef __DEBUG_BLK_LOW_LEVEL__
	cy_as_hal_print_message(KERN_ERR
				"%s: bus:%d, device:%d, evtype:%d, "
				"evdata:%p\n ", __func__, bus, device,
				evtype, evdata);
#endif

	switch (evtype) {
	case cy_as_storage_processor:
		break;

	case cy_as_storage_removed:
		break;

	case cy_as_storage_inserted:
		break;

	default:
		break;
	}
}

#define SECTORS_TO_SCAN 819200
#define SECTORS_TO_READ 32

uint32_t cyasblkdev_get_vfat_offset(int bus_num, int unit_no)
{
	/*
	 * for sd media, vfat partition boot record is not always
	 * located at sector it greatly depends on the system and
	 * software that was used to format the sd however, linux
	 * fs layer always expects it at sector 0, this function
	 * finds the offset and then uses it in all media r/w
	 * operations
	 */
	int sect_no, stat;
	uint8_t *sect_buf;
	bool br_found = false;

	DBGPRN_FUNC_NAME;

	sect_buf = kmalloc(1024, GFP_KERNEL);

	/* since HAL layer always uses sg lists instead of the
	 * buffer (for hw dmas) we need to initialize the sg list
	 * for local buffer*/
	sg_init_one(gl_bd->sg, sect_buf, 512);

	/*
	 * Check MPR partition table 1st, then try to scan through
	 * 1st 384 sectors until BR signature(intel JMP istruction
	 * code and ,0x55AA) is found
	 */
#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message
	    ("%s scanning media for vfat partition...\n", __func__);
#endif
	stat = cy_as_storage_read(
					 /* Handle to the device of interest */
					 gl_bd->dev_handle,
					 /* The bus to access */
					 bus_num,
					 /* The device to access */
					 0,
					 /* The unit to access */
					 unit_no,
					 /* absolute sector number */
					 0,
					 /* sg structure */
					 gl_bd->sg,
					 /* The number of blocks to be read */
					 1);
	if ((sect_buf[510] == 0x55) && (sect_buf[511] == 0xaa)) {
		int start_LBA;
		int size_LBA = 0;
		char type;

		start_LBA = 0;

		start_LBA += (sect_buf[446 + 11] & 0xFF);
		start_LBA = start_LBA << 8;
		start_LBA += (sect_buf[446 + 10] & 0xFF);
		start_LBA = start_LBA << 8;
		start_LBA += (sect_buf[446 + 9] & 0xFF);
		start_LBA = start_LBA << 8;
		start_LBA += (sect_buf[446 + 8] & 0xFF);

		size_LBA += (sect_buf[446 + 15] & 0xFF);
		size_LBA = size_LBA << 8;
		size_LBA += (sect_buf[446 + 14] & 0xFF);
		size_LBA = size_LBA << 8;
		size_LBA += (sect_buf[446 + 13] & 0xFF);
		size_LBA = size_LBA << 8;
		size_LBA += (sect_buf[446 + 12] & 0xFF);

		type = sect_buf[446 + 4];
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    ("%s find LBA start at 0x%x, type=[0x%x]\n", __func__,
		     start_LBA, type);
		cy_as_hal_print_message
		    ("%s : LBA size=0x%x, disk cap=[0x%x]\n", __func__,
		     size_LBA, gl_bd->user_disk_0_disk_cap);
#endif
		if (type && (start_LBA < SECTORS_TO_SCAN)
		    && (size_LBA <= gl_bd->user_disk_0_disk_cap)) {
			stat = cy_as_storage_read(
							 /* Handle to the device of interest */
							 gl_bd->dev_handle,
							 /* The bus to access */
							 bus_num,
							 /* The device to access */
							 0,
							 /* The unit to access */
							 unit_no,
							 /* absolute sector number */
							 start_LBA,
							 /* sg structure */
							 gl_bd->sg,
							 /* The number of blocks to be read */
							 1);

			if ((sect_buf[510] == 0x55)
			    && (sect_buf[511] == 0xaa)) {
				if (sect_buf[0] == 0xEB) {
					br_found = true;
					sect_no = start_LBA;
#ifndef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message
					    ("%s vfat partition found "
					     "at LBA start:%d\n", __func__,
					     sect_no);
#endif
				}

			}

		}
	}

	if (!br_found) {
		for (sect_no = 0; sect_no < SECTORS_TO_SCAN; sect_no++) {
#if 0				/* ndef WESTBRIDGE_NDEBUG */
			cy_as_hal_print_message(KERN_ERR
						"%s before cyasstorageread "
						"gl_bd->sg addr=0x%x\n",
						__func__,
						(unsigned int) gl_bd->sg);
#endif

			stat = cy_as_storage_read(
							 /* Handle to the device of interest */
							 gl_bd->dev_handle,
							 /* The bus to access */
							 bus_num,
							 /* The device to access */
							 0,
							 /* The unit to access */
							 unit_no,
							 /* absolute sector number */
							 sect_no,
							 /* sg structure */
							 gl_bd->sg,
							 /* The number of blocks to be read */
							 1);

			/* try only sectors with boot signature */
			if ((sect_buf[510] == 0x55)
			    && (sect_buf[511] == 0xaa)) {
				/* vfat boot record may also be located at
				 * sector 0, check it first  */
				if (sect_buf[0] == 0xEB) {
#ifndef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message
					    ("%s vfat partition found "
					     "at sector:%d\n", __func__,
					     sect_no);
#endif

					br_found = true;
					break;
				}
			}

			if (stat != 0) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
							"%s sector scan error: %d\n",
							__func__, stat);
#endif
				break;
			}
		}
	}

	kfree(sect_buf);

	if (br_found) {
		gl_vfat_offset[bus_num][unit_no] = sect_no;
		return sect_no;
	} else {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    ("%s sector scan error %d, bus % d unit %d\n",
		     __func__, stat, bus_num, unit_no);
#endif
		gl_vfat_offset[bus_num][unit_no] = 0;
		return 0;
	}
}

uint32_t cyasblkdev_export_vfat_offset(int bus_num, int unit_no)
{
	if (gl_vfat_offset[bus_num][unit_no] == -1)
		gl_vfat_offset[bus_num][unit_no] =
		    cyasblkdev_get_vfat_offset(bus_num, unit_no);

	return gl_vfat_offset[bus_num][unit_no];

}

EXPORT_SYMBOL(cyasblkdev_export_vfat_offset);

cy_as_storage_query_device_data dev_data = { 0 };

static int cyasblkdev_add_disks(int bus_num,
				struct cyasblkdev_blk_data *bd,
				int total_media_count, int devidx)
{
	int ret = 0;
	uint64_t disk_cap;
	int lcl_unit_no;
	cy_as_storage_query_unit_data unit_data = { 0 };
	cy_as_storage_sd_reg_read_data reg_data;
	unsigned char tempbuf[32];

#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message(KERN_ERR "%s:query device: "
				"type:%d, removable:%d, writable:%d, "
				"blksize %d, units:%d, locked:%d, "
				"erase_sz:%d\n",
				__func__,
				dev_data.desc_p.type,
				dev_data.desc_p.removable,
				dev_data.desc_p.writeable,
				dev_data.desc_p.block_size,
				dev_data.desc_p.number_units,
				dev_data.desc_p.locked,
				dev_data.desc_p.erase_unit_size);
#endif

	/*  make sure that device is not locked  */
	if (dev_data.desc_p.locked) {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message("%s: device is locked\n",
					__func__);
#endif
		ret =
		    cy_as_storage_release(bd->dev_handle, bus_num, 0, 0,
					  0);
		if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s cannot release"
						" storage\n", __func__);
#endif
			goto out;
		}
		goto out;
	}

	unit_data.device = 0;
	unit_data.unit = 0;
	unit_data.bus = bus_num;
	ret = cy_as_storage_query_unit(bd->dev_handle, &unit_data, 0, 0);
	if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR "%s: cannot query "
					"%d device unit - reason code %d\n",
					__func__, bus_num, ret);
#endif
		goto out;
	}

	if (private_partition_bus == bus_num) {
		if (private_partition_size > 0) {
			ret =
			    cy_as_storage_create_p_partition(bd->
							     dev_handle,
							     bus_num, 0,
							     private_partition_size,
							     0, 0);
			if ((ret != CY_AS_ERROR_SUCCESS)
			    && (ret != CY_AS_ERROR_ALREADY_PARTITIONED)) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
							"%s: cy_as_storage_"
							"create_p_partition after size > 0 check "
							"failed with error code %d\n",
							__func__, ret);
#endif

				disk_cap = (uint64_t)
				    (unit_data.desc_p.unit_size);
				lcl_unit_no = 0;

			} else if (ret == CY_AS_ERROR_ALREADY_PARTITIONED) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("%s: cy_as_storage_create_p_partition "
				     "indicates memory already partitioned\n",
				     __func__);
#endif

				/*check to see that partition
				 * matches size */
				if (unit_data.desc_p.unit_size !=
				    private_partition_size) {
					ret =
					    cy_as_storage_remove_p_partition
					    (bd->dev_handle, bus_num, 0, 0,
					     0);
					if (ret == CY_AS_ERROR_SUCCESS) {
						ret =
						    cy_as_storage_create_p_partition
						    (bd->dev_handle,
						     bus_num, 0,
						     private_partition_size,
						     0, 0);
						if (ret ==
						    CY_AS_ERROR_SUCCESS) {
							unit_data.bus =
							    bus_num;
							unit_data.device =
							    0;
							unit_data.unit = 1;
						} else {
#ifndef WESTBRIDGE_NDEBUG
							cy_as_hal_print_message
							    ("%s: cy_as_storage_create_p_partition "
							     "after removal unexpectedly failed "
							     "with error %d\n",
							     __func__,
							     ret);
#endif

							/* need to requery bus
							 * seeing as delete
							 * successful and create
							 * failed we have changed
							 * the disk properties */
							unit_data.bus =
							    bus_num;
							unit_data.device =
							    0;
							unit_data.unit = 0;
						}

						ret =
						    cy_as_storage_query_unit
						    (bd->dev_handle,
						     &unit_data, 0, 0);
						if (ret !=
						    CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
							cy_as_hal_print_message
							    ("%s: cannot query %d "
							     "device unit - reason code %d\n",
							     __func__,
							     bus_num, ret);
#endif
							goto out;
						} else {
							disk_cap =
							    (uint64_t)
							    (unit_data.
							     desc_p.
							     unit_size);
							lcl_unit_no =
							    unit_data.unit;
						}
					} else {
#ifndef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message
						    ("%s: cy_as_storage_remove_p_partition "
						     "failed with error %d\n",
						     __func__, ret);
#endif

						unit_data.bus = bus_num;
						unit_data.device = 0;
						unit_data.unit = 1;

						ret =
						    cy_as_storage_query_unit
						    (bd->dev_handle,
						     &unit_data, 0, 0);
						if (ret !=
						    CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
							cy_as_hal_print_message
							    ("%s: cannot query %d "
							     "device unit - reason "
							     "code %d\n",
							     __func__,
							     bus_num, ret);
#endif
							goto out;
						}

						disk_cap = (uint64_t)
						    (unit_data.desc_p.
						     unit_size);
						lcl_unit_no =
						    unit_data.unit;
					}
				} else {
#ifndef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message(KERN_ERR
								"%s: partition "
								"exists and sizes equal\n",
								__func__);
#endif

					/*partition already existed,
					 * need to query second unit*/
					unit_data.bus = bus_num;
					unit_data.device = 0;
					unit_data.unit = 1;

					ret =
					    cy_as_storage_query_unit(bd->
								     dev_handle,
								     &unit_data,
								     0, 0);
					if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
						cy_as_hal_print_message
						    ("%s: cannot query %d "
						     "device unit "
						     "- reason code %d\n",
						     __func__, bus_num,
						     ret);
#endif
						goto out;
					} else {
						disk_cap = (uint64_t)
						    (unit_data.desc_p.
						     unit_size);
						lcl_unit_no =
						    unit_data.unit;
					}
				}
			} else {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("%s: cy_as_storage_create_p_partition "
				     "created successfully\n", __func__);
#endif

				disk_cap = (uint64_t)
				    (unit_data.desc_p.unit_size -
				     private_partition_size);

				lcl_unit_no = 1;
			}
		}
#ifndef WESTBRIDGE_NDEBUG
		else {
			cy_as_hal_print_message
			    ("%s: invalid partition_size%d\n", __func__,
			     private_partition_size);

			disk_cap = (uint64_t)
			    (unit_data.desc_p.unit_size);
			lcl_unit_no = 0;
		}
#endif
	} else {
		disk_cap = (uint64_t)
		    (unit_data.desc_p.unit_size);
		lcl_unit_no = 0;
	}

	if ((bus_num == 0) || (total_media_count == 1)) {
		sprintf(bd->user_disk_0->disk_name,
			"cyasblkdevblk%d", devidx);

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message("%s: disk unit_sz:%lu blk_sz:%d, "
					"start_blk:%lu, capacity:%llu\n",
					__func__, (unsigned long)
					unit_data.desc_p.unit_size,
					unit_data.desc_p.block_size,
					(unsigned long)
					unit_data.desc_p.start_block,
					(uint64_t) disk_cap);
#endif

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s: setting gendisk disk "
					"capacity to %d\n", __func__,
					(int) disk_cap);
#endif

		/* initializing bd->queue */
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR "%s: init bd->queue\n",
					__func__);
#endif
		/* this will create a
		 * queue kernel thread */
		cyasblkdev_init_queue(&bd->queue, &bd->lock);

		/* bd->queue.prep_fn = cyasblkdev_blk_prep_rq; */
		bd->queue.issue_fn = cyasblkdev_blk_issue_rq;
		bd->queue.data = bd;

		/*blk_size should always
		 * be a multiple of 512,
		 * set to the max to ensure
		 * that all accesses aligned
		 * to the greatest multiple,
		 * can adjust request to
		 * smaller block sizes
		 * dynamically*/

		bd->user_disk_0_read_only = !dev_data.desc_p.writeable;
		bd->user_disk_0_blk_size = dev_data.desc_p.block_size;
		bd->user_disk_0_type = dev_data.desc_p.type;
		bd->user_disk_0_bus_num = bus_num;
		bd->user_disk_0->major = major;
		bd->user_disk_0->first_minor = devidx << CYASBLKDEV_SHIFT;
		bd->user_disk_0->minors = 8;
		bd->user_disk_0->fops = &cyasblkdev_bdops;
		bd->user_disk_0->private_data = bd;
		bd->user_disk_0->queue = bd->queue.queue;
		bd->dbgprn_flags = DBGPRN_RD_RQ;
		bd->user_disk_0_unit_no = lcl_unit_no;

#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
		blk_queue_logical_block_size(bd->queue.queue,
					     bd->user_disk_0_blk_size);
#else
		blk_queue_hardsect_size(bd->queue.queue,
					bd->user_disk_0_blk_size);
#endif

		bd->user_disk_0_disk_cap = disk_cap;

		set_capacity(bd->user_disk_0, disk_cap);

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message
		    ("%s: returned from set_capacity %d\n", __func__,
		     (int) disk_cap);
#endif

		/* need to start search from
		 * public partition beginning */
#if 1
		if (vfat_search) {
			bd->user_disk_0_first_sector =
			    cyasblkdev_get_vfat_offset(bd->
						       user_disk_0_bus_num,
						       bd->
						       user_disk_0_unit_no);
		} else {
			bd->user_disk_0_first_sector = 0;
		}
#else
		cyasblkdev_get_vfat_offset(bd->user_disk_0_bus_num,
					   bd->user_disk_0_unit_no);
		bd->user_disk_0_first_sector = 0;

#endif
		reg_data.buf_p = tempbuf;
		reg_data.length = 16;
		ret =
		    cy_as_storage_sd_register_read(bd->dev_handle,
						   bd->user_disk_0_bus_num,
						   0, cy_as_sd_reg_CID,
						   &reg_data, 0, 0);
		if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"cyasblkdev_blk_open : fail in read CID register on user_disk_0\n");
#endif
			goto out;
		} else {
			memcpy(bd->user_disk_0_serial_num, &tempbuf[9], 4);
			memcpy(bd->user_disk_0_CID, &tempbuf[0], 16);
		}

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message("%s: set user_disk_0_first "
					"sector to %d\n", __func__,
					bd->user_disk_0_first_sector);
		cy_as_hal_print_message("%s: add_disk: disk->major=0x%x\n",
					__func__, bd->user_disk_0->major);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->first_minor=0x%x\n",
					__func__,
					bd->user_disk_0->first_minor);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->minors=0x%x\n", __func__,
					bd->user_disk_0->minors);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->disk_name=%s\n", __func__,
					bd->user_disk_0->disk_name);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->part_tbl=0x%x\n", __func__,
					(unsigned int)
					bd->user_disk_0->part_tbl);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->queue=0x%x\n", __func__,
					(unsigned int)
					bd->user_disk_0->queue);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->flags=0x%x\n",
					__func__, (unsigned int)
					bd->user_disk_0->flags);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->driverfs_dev=0x%x\n",
					__func__, (unsigned int)
					bd->user_disk_0->driverfs_dev);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->slave_dir=0x%x\n",
					__func__, (unsigned int)
					bd->user_disk_0->slave_dir);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->random=0x%x\n",
					__func__, (unsigned int)
					bd->user_disk_0->random);
		cy_as_hal_print_message("%s: add_disk: "
					"disk->node_id=0x%x\n",
					__func__, (unsigned int)
					bd->user_disk_0->node_id);
#endif

		add_disk(bd->user_disk_0);

	} else if ((bus_num == 1) && (total_media_count == 2)) {
		bd->user_disk_1_read_only = !dev_data.desc_p.writeable;
		bd->user_disk_1_blk_size = dev_data.desc_p.block_size;
		bd->user_disk_1_type = dev_data.desc_p.type;
		bd->user_disk_1_bus_num = bus_num;
		bd->user_disk_1->major = major;
		bd->user_disk_1->first_minor =
		    (devidx + 1) << CYASBLKDEV_SHIFT;
		bd->user_disk_1->minors = 8;
		bd->user_disk_1->fops = &cyasblkdev_bdops;
		bd->user_disk_1->private_data = bd;
		bd->user_disk_1->queue = bd->queue.queue;
		bd->dbgprn_flags = DBGPRN_RD_RQ;
		bd->user_disk_1_unit_no = lcl_unit_no;

		sprintf(bd->user_disk_1->disk_name,
			"cyasblkdevblk%d", (devidx + 1));

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message("%s: disk unit_sz:%lu "
					"blk_sz:%d, "
					"start_blk:%lu, "
					"capacity:%llu\n",
					__func__, (unsigned long)
					unit_data.desc_p.unit_size,
					unit_data.desc_p.block_size,
					(unsigned long)
					unit_data.desc_p.start_block,
					(uint64_t) disk_cap);
#endif

		/*blk_size should always be a
		 * multiple of 512, set to the max
		 * to ensure that all accesses
		 * aligned to the greatest multiple,
		 * can adjust request to smaller
		 * block sizes dynamically*/
		if (bd->user_disk_0_blk_size > bd->user_disk_1_blk_size) {
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			blk_queue_logical_block_size(bd->queue.queue,
						     bd->
						     user_disk_0_blk_size);
#else
			blk_queue_hardsect_size(bd->queue.queue,
						bd->user_disk_0_blk_size);
#endif
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("%s: set hard sect_sz:%d\n", __func__,
			     bd->user_disk_0_blk_size);
#endif
		} else {
#if defined (__FOR_KERNEL_2_6_35__) || defined (__FOR_KERNEL_2_6_32__)
			blk_queue_logical_block_size(bd->queue.queue,
						     bd->
						     user_disk_1_blk_size);
#else
			blk_queue_hardsect_size(bd->queue.queue,
						bd->user_disk_1_blk_size);
#endif
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("%s: set hard sect_sz:%d\n", __func__,
			     bd->user_disk_1_blk_size);
#endif
		}

		bd->user_disk_1_disk_cap = disk_cap;
		set_capacity(bd->user_disk_1, disk_cap);
#if 1
		if (vfat_search) {
			bd->user_disk_1_first_sector =
			    cyasblkdev_get_vfat_offset(bd->
						       user_disk_1_bus_num,
						       bd->
						       user_disk_1_unit_no);
		} else {
			bd->user_disk_1_first_sector = 0;
		}
#else
		cyasblkdev_get_vfat_offset(bd->user_disk_1_bus_num,
					   bd->user_disk_1_unit_no);
		bd->user_disk_1_first_sector = 0;
#endif
		reg_data.buf_p = tempbuf;
		reg_data.length = 16;
		ret =
		    cy_as_storage_sd_register_read(bd->dev_handle,
						   bd->user_disk_1_bus_num,
						   0, cy_as_sd_reg_CID,
						   &reg_data, 0, 0);
		if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"cyasblkdev_blk_open : fail in read CID register on user_disk_1 \n");
#endif
		} else {
			memcpy(bd->user_disk_1_serial_num, &tempbuf[9], 4);
			memcpy(bd->user_disk_0_CID, &tempbuf[0], 16);
		}

		add_disk(bd->user_disk_1);
	}

	if (lcl_unit_no > 0) {
		if (bd->system_disk == NULL) {
			bd->system_disk =
			    alloc_disk(CYASBLKDEV_MINOR_2
				       << CYASBLKDEV_SHIFT);
			if (bd->system_disk == NULL) {
				/* kfree (bd); */
				bd = ERR_PTR(-ENOMEM);
				return CY_AS_ERROR_OUT_OF_MEMORY;
			}
			disk_cap = (uint64_t)
			    (private_partition_size);

			/* set properties of
			 * system disk */
			bd->system_disk_read_only =
			    !dev_data.desc_p.writeable;
			bd->system_disk_blk_size =
			    dev_data.desc_p.block_size;
			bd->system_disk_bus_num = bus_num;
			bd->system_disk->major = major;
			bd->system_disk->first_minor =
			    (devidx + 2) << CYASBLKDEV_SHIFT;
			bd->system_disk->minors = 8;
			bd->system_disk->fops = &cyasblkdev_bdops;
			bd->system_disk->private_data = bd;
			bd->system_disk->queue = bd->queue.queue;
			/* don't search for vfat
			 * with system disk */
			bd->system_disk_first_sector = 0;
			sprintf(bd->system_disk->disk_name,
				"cyasblkdevblk%d", (devidx + 2));

			bd->system_disk_disk_cap = disk_cap;
			set_capacity(bd->system_disk, disk_cap);

			reg_data.buf_p = tempbuf;
			reg_data.length = 16;
			ret =
			    cy_as_storage_sd_register_read(bd->dev_handle,
							   bd->
							   system_disk_bus_num,
							   0,
							   cy_as_sd_reg_CID,
							   &reg_data, 0,
							   0);
			if (ret != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
							"cyasblkdev_blk_open : fail in read CID register on system_disk\n");
#endif
			} else {
				memcpy(bd->system_disk_serial_num,
				       &tempbuf[9], 4);
				memcpy(bd->user_disk_0_CID, &tempbuf[0],
				       16);
			}

			add_disk(bd->system_disk);
		}
#ifndef WESTBRIDGE_NDEBUG
		else {
			cy_as_hal_print_message
			    ("%s: system disk already allocated %d\n",
			     __func__, bus_num);
		}
#endif
	}
      out:
	return ret;
}

static struct cyasblkdev_blk_data *cyasblkdev_blk_alloc(void)
{
	struct cyasblkdev_blk_data *bd;
	int ret = 0;
	cy_as_return_status_t stat = -1;
	int bus_num = 0;
	int total_media_count = 0;
	int devidx = 0;
	int check_media_count;
	DBGPRN_FUNC_NAME;

	total_media_count = 0;
	devidx = find_first_zero_bit(dev_use, CYASBLKDEV_NUM_MINORS);
	if (devidx >= CYASBLKDEV_NUM_MINORS)
		return ERR_PTR(-ENOSPC);

	__set_bit(devidx, dev_use);
	__set_bit(devidx + 1, dev_use);

	/* bd = kzalloc (sizeof (struct cyasblkdev_blk_data), GFP_KERNEL); */
	bd = &g_blk_dev;
	if (bd) {
		gl_bd = bd;
		memset(bd, 0, sizeof(struct cyasblkdev_blk_data));

		spin_lock_init(&bd->lock);
		bd->usage = 1;
		/* setup the block_dev_ops pointer */
		bd->blkops = &cyasblkdev_bdops;

		/* Get the device handle */
		bd->dev_handle = cyasdevice_getdevhandle();
		if (0 == bd->dev_handle) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message("%s: get device failed\n",
						__func__);
#endif
			ret = ENODEV;
			goto out;
		}
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s west bridge device handle:%x\n",
					__func__,
					(uint32_t) bd->dev_handle);
#endif

		/* start the storage api and get a handle to the
		 * device we are interested in. */

		/* Error code to use if the conditions are not satisfied. */
		ret = ENOMEDIUM;

		stat =
		    cy_as_misc_release_resource(bd->dev_handle,
						cy_as_bus_0);
		if ((stat != CY_AS_ERROR_SUCCESS)
		    && (stat != CY_AS_ERROR_RESOURCE_NOT_OWNED)) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s: cannot release "
						"resource bus 0 - reason code %d\n",
						__func__, stat);
#endif
		}

		stat =
		    cy_as_misc_release_resource(bd->dev_handle,
						cy_as_bus_1);
		if ((stat != CY_AS_ERROR_SUCCESS)
		    && (stat != CY_AS_ERROR_RESOURCE_NOT_OWNED)) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s: cannot release "
						"resource bus 0 - reason code %d\n",
						__func__, stat);
#endif
		}

		/* skkm */
#if 1
		stat =
		    cy_as_storage_device_control(bd->dev_handle, 1, 0,
						 cy_true, cy_false,
						 cy_as_storage_detect_GPIO,
						 0, 0);
		if (stat != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message
			    ("<1>CyAsBlkDev: Cannot control StorageDevice - Reason code %d\n",
			     stat);
			goto out;
		}
#endif
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s: call storage start APIs\n",
					__func__);
#endif
		/* start storage stack */
		stat = cy_as_storage_start(bd->dev_handle, 0, 0x101);
		if (stat != CY_AS_ERROR_SUCCESS
		    && stat != CY_AS_ERROR_ALREADY_RUNNING) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s: cannot start storage "
						"stack - reason code %d\n",
						__func__, stat);
#endif
			goto out;
		}
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s: storage started:%d ok\n",
					__func__, stat);
#endif

		stat = cy_as_storage_register_callback(bd->dev_handle,
						       cyasblkdev_storage_callback);
		if (stat != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s: cannot register callback "
						"- reason code %d\n",
						__func__, stat);
#endif
			goto out;
		}

		for (bus_num = 0; bus_num < 2; bus_num++) {
			stat = cy_as_storage_query_bus(bd->dev_handle,
						       bus_num,
						       &bd->
						       media_count
						       [bus_num], 0, 0);
			if (stat == CY_AS_ERROR_SUCCESS) {
				total_media_count = total_media_count +
				    bd->media_count[bus_num];
			} else {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message(KERN_ERR
							"%s: cannot query %d, "
							"reason code: %d\n",
							__func__, bus_num,
							stat);
#endif
				goto out;
			}
		}

		if (total_media_count == 0) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message
			    ("%s: no storage media was found\n", __func__);
#endif
			goto out;
		} else if (total_media_count >= 1) {
			if (bd->user_disk_0 == NULL) {

				bd->user_disk_0 =
				    alloc_disk(CYASBLKDEV_MINOR_0
					       << CYASBLKDEV_SHIFT);
				if (bd->user_disk_0 == NULL) {
					/* kfree (bd); */
					bd = ERR_PTR(-ENOMEM);
					return bd;
				}
			}
#ifndef WESTBRIDGE_NDEBUG
			else {
				cy_as_hal_print_message(KERN_ERR
							"%s: no available "
							"gen_disk for disk 0, "
							"physically inconsistent\n",
							__func__);
			}
#endif
		}

		if (total_media_count == 2) {
			if (bd->user_disk_1 == NULL) {
				bd->user_disk_1 =
				    alloc_disk(CYASBLKDEV_MINOR_1
					       << CYASBLKDEV_SHIFT);
				if (bd->user_disk_1 == NULL) {
					/* kfree (bd); */
					bd = ERR_PTR(-ENOMEM);
					return bd;
				}
			}
#ifndef WESTBRIDGE_NDEBUG
			else {
				cy_as_hal_print_message(KERN_ERR
							"%s: no available "
							"gen_disk for media, "
							"physically inconsistent\n",
							__func__);
			}
#endif
		}
#ifndef WESTBRIDGE_NDEBUG
		else if (total_media_count > 2) {
			cy_as_hal_print_message(KERN_ERR
						"%s: count corrupted = 0x%d\n",
						__func__,
						total_media_count);
		}
#endif

#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s: %d device(s) found\n",
					__func__, total_media_count);
#endif

		check_media_count = 0;

		for (bus_num = 0; bus_num <= 1; bus_num++) {
			/*claim storage for cpu */
#if 1				/* skkm */
			stat = cy_as_storage_claim(bd->dev_handle,
						   bus_num, 0, 0, 0);
			if (stat != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(KERN_ERR
							"%s: cannot claim "
							"%d bus - reason code %d\n",
							__func__, bus_num,
							stat);
				/* goto out; */
			}
#endif

			dev_data.bus = bus_num;
			dev_data.device = 0;

			stat = cy_as_storage_query_device(bd->dev_handle,
							  &dev_data, 0, 0);
			if (stat == CY_AS_ERROR_SUCCESS) {
#if 0				/* skkm */
				stat =
				    cy_as_storage_change_sd_frequency(bd->
								      dev_handle,
								      bus_num,
								      0x11,
								      0x18,
								      0,
								      0);
				if (stat != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message
					    ("%s: Cannot control cy_as_storage_change_sd_frequency - reason [%d]\n",
					     __func__, stat);
#endif
				}
#endif
				stat =
				    cyasblkdev_add_disks(bus_num, bd,
							 total_media_count,
							 devidx);
				if (stat == CY_AS_ERROR_SUCCESS)
					check_media_count++;
			} else if (stat == CY_AS_ERROR_NO_SUCH_DEVICE) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("%s: no device on bus %d\n", __func__,
				     bus_num);
#endif
			} else {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("%s: cannot query %d device "
				     "- reason code %d\n", __func__,
				     bus_num, stat);
#endif
				goto err_put_disk;
			}
		}		/* end for (bus_num = 0; bus_num <= 1; bus_num++) */

		if (total_media_count != check_media_count)
			goto err_put_disk;

		/* sg_init_table (bd->sg, 16); */
		return bd;
	}
      err_put_disk:
	if (bd->user_disk_0 != NULL) {
		put_disk(bd->user_disk_0);
		bd->user_disk_0 = NULL;
	}
	if (bd->user_disk_1 != NULL) {
		put_disk(bd->user_disk_1);
		bd->user_disk_1 = NULL;
	}
	if (bd->system_disk != NULL) {
		put_disk(bd->system_disk);
		bd->system_disk = NULL;
	}

      out:
	__clear_bit(devidx, dev_use);
	__clear_bit(devidx + 1, dev_use);

#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message("%s: bd failed to initialize\n", __func__);
#endif

	/* kfree (bd); */
	/* bd = ERR_PTR (-ret); */
	return ERR_PTR(-ret);
}


/*init west bridge block device */
static int cyasblkdev_blk_initialize(void)
{
	struct cyasblkdev_blk_data *bd;
	int res;

	DBGPRN_FUNC_NAME;

	res = register_blkdev(major, "cyasblkdev");

	if (res < 0) {
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_WARNING
					"%s unable to get major %d for cyasblkdev media: %d\n",
					__func__, major, res);
#endif
		return res;
	}

	if (major == 0)
		major = res;

#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message
	    ("%s cyasblkdev registered with major number: %d\n", __func__,
	     major);
#endif

	bd = cyasblkdev_blk_alloc();
	if (IS_ERR(bd))
		return PTR_ERR(bd);

	return 0;
}

/* start block device */
int cyasblkdev_blk_init(int fmajor, int fsearch)
{
	int res = -ENOMEM;

	DBGPRN_FUNC_NAME;

	if (g_is_block_dev_ready)
		return 0;

	g_is_block_dev_ready = 1;

	major = fmajor;
	vfat_search = fsearch;

#ifdef __CYAS_USE_WORK_QUEUE
	cyas_blk_wq = create_workqueue("cyas_blk_wq");
	cyas_blk_work =
	    (cy_work_t *) kmalloc(sizeof(cy_work_t), GFP_KERNEL);
	if (cyas_blk_work) {
		INIT_WORK(&cyas_blk_work->work, cyasblkdev_workqueue);
	}
#endif
	/* get the cyasdev handle for future use */
	cyas_dev_handle = cyasdevice_getdevhandle();

	if (cyasblkdev_blk_initialize() == 0)
		return 0;
#ifndef WESTBRIDGE_NDEBUG
	cy_as_hal_print_message(KERN_ERR "cyasblkdev init error:%d\n",
				res);
#endif
	return res;
}


static void cyasblkdev_blk_deinit(struct cyasblkdev_blk_data *bd)
{
	DBGPRN_FUNC_NAME;

	if (bd) {
		int devidx;

		if (bd->user_disk_0 != NULL) {
			cy_as_hal_print_message(KERN_ERR
						"[%s] del_gendisk user_disk_0\n",
						__func__);
			del_gendisk(bd->user_disk_0);
			devidx = bd->user_disk_0->first_minor
			    >> CYASBLKDEV_SHIFT;
			__clear_bit(devidx, dev_use);
		}

		if (bd->user_disk_1 != NULL) {
			cy_as_hal_print_message(KERN_ERR
						"[%s] del_gendisk user_disk_1\n",
						__func__);
			del_gendisk(bd->user_disk_1);
			devidx = bd->user_disk_1->first_minor
			    >> CYASBLKDEV_SHIFT;
			__clear_bit(devidx, dev_use);
		}

		if (bd->system_disk != NULL) {
			cy_as_hal_print_message(KERN_ERR
						"[%s] del_gendisk system_disk\n",
						__func__);
			del_gendisk(bd->system_disk);
			devidx = bd->system_disk->first_minor
			    >> CYASBLKDEV_SHIFT;
			__clear_bit(devidx, dev_use);
		}

		if (bd->queue.queue != NULL)
			cyasblkdev_cleanup_queue(&bd->queue);

		cyasblkdev_blk_put(bd);
/*
		while(1)
		{
			if( bd->usage == 1 )
				break;
			cy_as_hal_sleep(10);
		}
*/
	}
}

/* block device exit */
void cyasblkdev_blk_exit(void)
{
	int loot_count = 1000;
	DBGPRN_FUNC_NAME;

	/*cy_as_hal_print_message (KERN_ERR"[%s] dev = %d gl_bd = 0x%x\n", __func__, g_is_block_dev_ready, gl_bd) ; */

	if (g_is_block_dev_ready) {
		if (gl_bd) {
			cyasblkdev_blk_deinit(gl_bd);
			unregister_blkdev(major, "cyasblkdev");

			while (--loot_count) {
				if (gl_bd == NULL)
					break;
				msleep(10);
			}

			if (gl_bd)
				cyasblkdev_blk_put(gl_bd);
		}
		gl_vfat_offset[0][0] = -1;
		gl_vfat_offset[0][1] = -1;
		gl_vfat_offset[1][0] = -1;
		gl_vfat_offset[1][1] = -1;

#ifdef __CYAS_USE_WORK_QUEUE
		kfree(cyas_blk_work);
		cyas_blk_work = NULL;
		destroy_workqueue(cyas_blk_wq);
#endif
		g_is_block_dev_ready = 0;
	}
}

EXPORT_SYMBOL(cyasblkdev_blk_init);
EXPORT_SYMBOL(cyasblkdev_blk_exit);

int cyasblkdev_start_sdcard(void)
{
	struct cyasblkdev_blk_data *bd;
	int bus_num;
	int retval;
	cy_as_storage_query_device_data dev_data;
	uint32_t count = 0;

	DBGPRN_FUNC_NAME;

	if (g_is_block_dev_ready && gl_bd) {
		bd = gl_bd;

		if (bd->user_disk_0) {
			bus_num = bd->user_disk_0_bus_num;
		} else if (bd->user_disk_1) {
			bus_num = bd->user_disk_1_bus_num;
		} else if (bd->system_disk) {
			bus_num = bd->system_disk_bus_num;
		} else
			return -1;

		cyas_dev_handle = bd->dev_handle =
		    cyasdevice_getdevhandle();

		retval =
		    cy_as_storage_device_control(bd->dev_handle, 1, 0,
						 cy_true, cy_false,
						 cy_as_storage_detect_GPIO,
						 0, 0);
		if (retval != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message
			    ("<1>CyAsBlkDev: Cannot control StorageDevice - Reason code %d\n",
			     retval);
		}

		retval = cy_as_storage_start(bd->dev_handle, 0, 0x101);
		if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s : fail in cy_as_storage_stop (%d)\n",
						__func__, retval);
#endif
			return retval;
		}
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s : cy_as_storage_start is called\n",
					__func__);
#endif

		retval =
		    cy_as_storage_query_media(bd->dev_handle,
					      cy_as_media_sd_flash, &count,
					      0, 0);
		if (retval != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR
						"ERROR: Cannot query SD device count - Reason code %d\n",
						retval);
			return retval;
		}
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s : cy_as_storage_query_media is called - media count = %d\n",
					__func__, count);
#endif

		if (count) {
			dev_data.bus = bus_num;
			dev_data.device = 0;

			retval =
			    cy_as_storage_query_device(bd->dev_handle,
						       &dev_data, 0, 0);
			if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
				cy_as_hal_print_message
				    ("%s: cannot query %d device - reason code %d\n",
				     __func__, bus_num, retval);
#endif
				return retval;
			}
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s : cy_as_storage_query_device is called - found \n",
						__func__);
#endif
		} else {
			return -1;
		}
	}
	return 0;
}

EXPORT_SYMBOL(cyasblkdev_start_sdcard);

int cyasblkdev_stop_sdcard(void)
{
	struct cyasblkdev_blk_data *bd;
	int retval;

	DBGPRN_FUNC_NAME;

	if (g_is_block_dev_ready && gl_bd) {
		bd = gl_bd;

		retval = cy_as_storage_stop(bd->dev_handle, 0, 0);
		if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s : fail in cy_as_storage_stop (%d)\n",
						__func__, retval);
#endif
			return -1;
		}
#ifndef WESTBRIDGE_NDEBUG
		cy_as_hal_print_message(KERN_ERR
					"%s : cy_as_storage_stop is called - found \n",
					__func__);
#endif

	}
	return 0;
}

EXPORT_SYMBOL(cyasblkdev_stop_sdcard);


/* block device exit */
int cyasblkdev_check_sdcard(void)
{
	struct cyasblkdev_blk_data *bd;
	cy_as_storage_sd_reg_read_data reg_data;
	int bus_num;
	unsigned char tempbuf[32];
	unsigned char *serial_num;
	int retval;

	DBGPRN_FUNC_NAME;

	if (g_is_block_dev_ready && gl_bd) {
		bd = gl_bd;

		if (bd->user_disk_0) {
			bus_num = bd->user_disk_0_bus_num;
			serial_num = bd->user_disk_0_serial_num;
		} else if (bd->user_disk_1) {
			bus_num = bd->user_disk_1_bus_num;
			serial_num = bd->user_disk_1_serial_num;
		} else if (bd->system_disk) {
			bus_num = bd->system_disk_bus_num;
			serial_num = bd->system_disk_serial_num;
		} else
			return 0;

#ifdef __USE_CYAS_AUTO_SUSPEND__
		cyasdevice_wakeup_thread(1);
#endif
		/*
		   retval = cy_as_misc_storage_changed(bd->dev_handle, 0, 0);
		   if(retval != CY_AS_ERROR_SUCCESS)
		   {
		   #ifndef WESTBRIDGE_NDEBUG
		   cy_as_hal_print_message(KERN_ERR "%s : fail in cy_as_misc_storage_changed (%d) \n", __func__, retval);
		   #endif
		   return -1;
		   }
		 */
		reg_data.buf_p = tempbuf;
		reg_data.length = 16;
		retval =
		    cy_as_storage_sd_register_read(bd->dev_handle, bus_num,
						   0, cy_as_sd_reg_CID,
						   &reg_data, 0, 0);
		if (retval != CY_AS_ERROR_SUCCESS) {
#ifndef WESTBRIDGE_NDEBUG
			cy_as_hal_print_message(KERN_ERR
						"%s : fail in read CID register (%d)\n",
						__func__, retval);
#endif
			return -1;
		} else {
			cy_as_hal_print_message(KERN_ERR
						"%s : serial num = 0x%x 0x%x 0x%x 0x%x\n",
						__func__, tempbuf[9],
						tempbuf[10], tempbuf[11],
						tempbuf[12]);
			if (memcmp(serial_num, &tempbuf[9], 4)) {
				return 1;
			}
		}

	} else
		return 1;
	return 0;
}

EXPORT_SYMBOL(cyasblkdev_check_sdcard);

int cyasblkdev_copy_char_to_int(char *dest, char *src)
{
	dest[0] = src[3];
	dest[1] = src[2];
	dest[2] = src[1];
	dest[3] = src[0];
	return 4;
}

int cyasblkdev_get_serial(void)
{
	struct cyasblkdev_blk_data *bd;
	int serial_num = 0;

	if (g_is_block_dev_ready && gl_bd) {
		bd = gl_bd;

		if (bd->user_disk_0) {
			cyasblkdev_copy_char_to_int((char *) &serial_num,
						    bd->
						    user_disk_0_serial_num);
		} else if (bd->user_disk_1) {
			cyasblkdev_copy_char_to_int((char *) &serial_num,
						    bd->
						    user_disk_1_serial_num);
		} else if (bd->system_disk) {
			cyasblkdev_copy_char_to_int((char *) &serial_num,
						    bd->
						    system_disk_serial_num);
		}
	}
	return serial_num;
}

EXPORT_SYMBOL(cyasblkdev_get_serial);

int cyasblkdev_get_CID(char *ptrCID)
{
	struct cyasblkdev_blk_data *bd;
	int i;
	if (g_is_block_dev_ready && gl_bd) {
		bd = gl_bd;

		if (bd->user_disk_0) {
			for (i = 0; i < 16; i += 4)
				cyasblkdev_copy_char_to_int(&ptrCID[i],
							    &bd->
							    user_disk_0_CID
							    [i]);
			return 16;
		} else if (bd->user_disk_1) {
			for (i = 0; i < 16; i += 4)
				cyasblkdev_copy_char_to_int(&ptrCID[i],
							    &bd->
							    user_disk_0_CID
							    [i]);
			return 16;
		} else if (bd->system_disk) {
			for (i = 0; i < 16; i += 4)
				cyasblkdev_copy_char_to_int(&ptrCID[i],
							    &bd->
							    user_disk_0_CID
							    [i]);
			return 16;
		}
	}

	memset(ptrCID, 0, 16);
	return 0;
}

EXPORT_SYMBOL(cyasblkdev_get_CID);

/*[]*/
