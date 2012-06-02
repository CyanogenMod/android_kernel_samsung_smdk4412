#define FEATURE_SAMSUNG_SPI

/**
 * Samsung Virtual Network driver using IpcSpi device
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#define DEBUG

/*
#define FORMAT_TX_DUMP
#define RAW_TX_DUMP
#define RFS_TX_DUMP
#define FORMAT_RX_DUMP
#define RAW_RX_DUMP
#define RFS_RX_DUMP
#define RAW_TX_RX_LENGTH_DUMP
#define RFS_TX_RX_LENGTH_DUMP
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/phone_svn/ipc_spi.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include <linux/vmalloc.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <mach/regs-pmu.h>
#include <linux/kthread.h>
#include <linux/err.h>

#include <linux/workqueue.h>
#ifdef FEATURE_SAMSUNG_SPI
#include "spi_main.h"
#endif

#define DRVNAME "onedram"

#define ONEDRAM_REG_OFFSET	0xFFF800
#define ONEDRAM_REG_SIZE		0x800

#define MB_VALID					0x0080
#define MB_COMMAND				0x0040

#define MB_CMD(x)			(MB_VALID | MB_COMMAND | x)
#define MB_DATA(x)			(MB_VALID | x)

#define MBD_SEND_FMT			0x0002
#define MBD_SEND_RAW			0x0001
#define MBD_SEND_RFS				0x0100
#define MBD_REQ_ACK_FMT		0x0020
#define MBD_REQ_ACK_RAW		0x0010
#define MBD_REQ_ACK_RFS		0x0400
#define MBD_RES_ACK_FMT		0x0008
#define MBD_RES_ACK_RAW		0x0004
#define MBD_RES_ACK_RFS			0x0200


#define FMT_OUT 0x0FE000
#define FMT_IN		0x10E000
#define FMT_SZ		0x10000   /* 65536 bytes */

#define RAW_OUT 0x11E000
#define RAW_IN		0x21E000
#define RAW_SZ		0x100000 /* 1 MB */

#define RFS_OUT 0x31E000
#define RFS_IN		0x41E000
#define RFS_SZ		0x100000 /* 1 MB */

static u32 ipc_spi_get_send_vbuff_command(void);
static void ipc_spi_set_send_vbuff_command_clear(void);
static inline void ipc_spi_get_pointer_of_vbuff_format_tx(u32 *head, u32 *tail);
static inline void ipc_spi_get_pointer_of_vbuff_format_rx(u32 *head, u32 *tail);
static inline void ipc_spi_get_pointer_of_vbuff_raw_tx(u32 *head, u32 *tail);
static inline void ipc_spi_get_pointer_of_vbuff_raw_rx(u32 *head, u32 *tail);
static inline void ipc_spi_get_pointer_of_vbuff_rfs_tx(u32 *head, u32 *tail);
static inline void ipc_spi_get_pointer_of_vbuff_rfs_rx(u32 *head, u32 *tail);
static inline void ipc_spi_update_tail_of_vbuff_format_tx(u32 u_tail);
static inline void ipc_spi_update_head_of_vbuff_format_rx(u32 u_head);
static inline void ipc_spi_update_tail_of_vbuff_raw_tx(u32 u_tail);
static inline void ipc_spi_update_head_of_vbuff_raw_rx(u32 u_head);
static inline void ipc_spi_update_tail_of_vbuff_rfs_tx(u32 u_tail);
static inline void ipc_spi_update_head_of_vbuff_rfs_rx(u32 u_head);
/*
#define LOOPBACK_TEST
*/
#ifdef LOOPBACK_TEST
int loop_back_test = 1;
#define LOOP_SIZE	2048
#else
int loop_back_test;
#endif

enum {
	MBC_NONE = 0,
	MBC_INIT_START,
	MBC_INIT_END,
	MBC_REQ_ACTIVE,
	MBC_RES_ACTIVE,
	MBC_TIME_SYNC,
	MBC_POWER_OFF,
	MBC_RESET,
	MBC_PHONE_START,
	MBC_ERR_DISPLAY,
	MBC_POWER_SAVE,
	MBC_NV_REBUILD,
	MBC_EMER_DOWN,
	MBC_REQ_SEM,
	MBC_RES_SEM,
	MBC_MAX
};
#define MBC_MASK					0xFF

#define MAX_BUF_SIZE				2044
#define DEF_BUF_SIZE				MAX_BUF_SIZE

static struct spi_device *p_ipc_spi;

#ifdef LOOPBACK_TEST
struct spi_protocol_header {
	unsigned long current_data_size:12;
	unsigned long more:1;
	unsigned long rx_error:1;
	unsigned long packet_id:2;
	unsigned long reserved:2;
	unsigned long next_data_size:10;
	unsigned long RI:1;
	unsigned long DCD:1;
	unsigned long RTSCTS:1;
	unsigned long DSRDTR:1;
};
#else
struct spi_protocol_header {
	unsigned long current_data_size:31;
	unsigned long more:1;
};
#endif

struct onedram_reg_mapped {
	u32 sem;
	u32 reserved1[7];
	u32 mailbox_AB;	/* CP write, AP read */
	u32 reserved2[7];
	u32 mailbox_BA;	/*AP write, CP read */
	u32 reserved3[23];
	u32 check_AB;		/* can't read */
	u32 reserved4[7];
	u32 check_BA;		/* 0: CP read, 1: CP don't read */
};

struct ipc_spi_handler {
	struct list_head list;
	void *data;
	void (*handler)(u32, void *);
};

struct ipc_spi_handler_head {
	struct list_head list;
	u32 len;
	spinlock_t lock;
};
static struct ipc_spi_handler_head h_list;

static struct resource ipc_spi_resource = {
	.name = DRVNAME,
	.start = 0,
	.end = -1,
	.flags = IORESOURCE_MEM,
};

struct ipc_spi {
	struct class *class;
	struct device *dev;
	struct cdev cdev;
	dev_t devid;

	wait_queue_head_t waitq;
	struct fasync_struct *async_queue;
	u32 mailbox;

	unsigned long base;
	unsigned long size;
	void __iomem *mmio;

	int irq;

	struct completion comp;
	atomic_t ref_sem;
	unsigned long flags;

	const struct attribute_group *group;

	struct onedram_reg_mapped *reg;
};
struct ipc_spi *ipc_spi;

struct workqueue_struct *ipc_spi_wq;
struct ipc_spi_send_modem_bin_workq_data {
	struct ipc_spi *od;

	struct work_struct send_modem_w;
};
struct ipc_spi_send_modem_bin_workq_data *ipc_spi_send_modem_work_data;

static DEFINE_SPINLOCK(ipc_spi_lock);

static unsigned long hw_tmp; /* for hardware */
static inline int _read_sem(struct ipc_spi *od);
static inline void _write_sem(struct ipc_spi *od, int v);
static int ipc_spi_thread_restart(void);
static inline void ipc_spi_get_pointer_of_vbuff_format_tx(u32 *head, u32 *tail);

struct completion ril_init;
static unsigned gpio_mrdy;
static unsigned gpio_srdy;
struct semaphore srdy_sem;
struct semaphore transfer_event_sem;
int send_modem_spi = 1;
int is_cp_reset;
int boot_done;

#ifndef FEATURE_SAMSUNG_SPI
static
#endif
int transfer_thread_waiting;

static void __iomem *p_virtual_buff;

static unsigned long recv_cnt;
static unsigned long send_cnt;
static ssize_t show_debug(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	struct ipc_spi *od = dev_get_drvdata(d);

	if (!od)
		return 0;

	p += sprintf(p, "Semaphore: %d (%d)\n", _read_sem(od), (char)hw_tmp);
	p += sprintf(p, "Mailbox: %x\n", od->reg->mailbox_AB);
	p += sprintf(p, "Reference count: %d\n", atomic_read(&od->ref_sem));
	p += sprintf(p, "Mailbox send: %lu\n", send_cnt);
	p += sprintf(p, "Mailbox recv: %lu\n", recv_cnt);

	return p - buf;
}


#ifdef FEATURE_SAMSUNG_SPI
void *ipc_spi_get_queue_buff(void)
{
	return (void *)p_virtual_buff;
}
#endif


static DEVICE_ATTR(debug, S_IRUGO, show_debug, NULL);

static struct attribute *ipc_spi_attributes[] = {
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group ipc_spi_group = {
	.attrs = ipc_spi_attributes,
};

static inline void _write_sem(struct ipc_spi *od, int v)
{
	od->reg->sem = 1;

	hw_tmp = od->reg->sem;	/* for hardware */
}

static inline int _read_sem(struct ipc_spi *od)
{
	od->reg->sem = 1;

	return od->reg->sem;
}

static inline int _send_cmd(struct ipc_spi *od, u32 cmd)
{
	u32 tmp_cmd;

	if (!od) {
		printk(KERN_ERR "[%s]ipcspi: Dev is NULL, but try to access\n",
			__func__);
		return -EFAULT;
	}

	if (!od->reg) {
		dev_err(od->dev, "(%d) Failed to send cmd, not initialized\n",
			__LINE__);

		return -EFAULT;
	}

	if (cmd & MB_VALID) {
		if (cmd & MB_COMMAND) {
			tmp_cmd = (cmd & MBC_MASK) & ~(MB_CMD(0));

			switch (tmp_cmd) {
			case MBC_INIT_END:	/* 0x0002 */
				dev_info(od->dev, "(%d) send ril init cmd : %x\n",
					__LINE__, cmd);

				send_cnt++;
				od->reg->mailbox_BA = cmd;

#ifdef FEATURE_SAMSUNG_SPI
				spi_main_send_signal(SPI_MAIN_MSG_IPC_SEND);
#else
				if (transfer_thread_waiting) {
					transfer_thread_waiting = 0;
					up(&transfer_event_sem);
				}
#endif

				complete_all(&ril_init);

				break;

			default:
				break;
			}
		} else {
			dev_dbg(od->dev, "(%d) =>send data (0x%x)\n",
				__LINE__, cmd);

#ifdef FEATURE_SAMSUNG_SPI
			spi_main_send_signal(SPI_MAIN_MSG_IPC_SEND);
#else
			if (transfer_thread_waiting) {
				transfer_thread_waiting = 0;
				up(&transfer_event_sem);
			}
#endif
		}
	} else {
		if (cmd == 0x45674567) {
			dev_dbg(od->dev, "(%d) modem image loaded.\n",
				__LINE__);
			dev_dbg(od->dev, "(%d) start to send modem binary to cp.\n",
				__LINE__);

			ipc_spi_send_modem_work_data->od = od;
			if (!queue_work(ipc_spi_wq,
				&ipc_spi_send_modem_work_data->send_modem_w))
				dev_err(od->dev, "(%d) already exist w-q\n", __LINE__);

		} else {
			dev_err(od->dev, "(%d) send invalid cmd : 0x%x\n",
				__LINE__, cmd);
		}
	}

	return 0;
}

static inline int _recv_cmd(struct ipc_spi *od, u32 *cmd)
{
	if (!cmd)
		return -EINVAL;

	if (!od) {
		printk(KERN_ERR "[%s]ipcspi: Dev is NULL, but try to access\n",
			__func__);
		return -EFAULT;
	}

	if (!od->reg) {
		dev_err(od->dev, "(%d) Failed to read cmd, not initialized\n",
			__LINE__);

		return -EFAULT;
	}

	recv_cnt++;
	*cmd = od->reg->mailbox_AB;
	return 0;
}

static inline int _get_auth(struct ipc_spi *od, u32 cmd)
{
	unsigned long timeleft;
	int retry = 0;

	/* send cmd every 20m seconds */
	while (1) {
		_send_cmd(od, cmd);

		timeleft = wait_for_completion_timeout(&od->comp, HZ/50);
#if 0
		if (timeleft)
			break;
#endif
		if (_read_sem(od))
			break;

		retry++;
		if (retry > 50) { /* time out after 1 seconds */
			dev_err(od->dev, "(%d) get authority time out\n", __LINE__);

			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int get_auth(struct ipc_spi *od, u32 cmd)
{
	int r;

	if (!od) {
		printk(KERN_ERR "[%s]ipcspi: Dev is NULL, but try to access\n", __func__);
		return -EFAULT;
	}

	if (!od->reg) {
		dev_err(od->dev, "(%d) Failed to get authority\n", __LINE__);

		return -EFAULT;
	}

	if (_read_sem(od))
		return 0;

	if (cmd)
		r = _get_auth(od, cmd);
	else
		r = -EACCES;

	return r;
}

static int put_auth(struct ipc_spi *od, int release)
{
	if (!od) {
		printk(KERN_ERR "[%s]onedram: Dev is NULL, but try to access\n", __func__);
		return -EFAULT;
	}

	if (!od->reg) {
		dev_err(od->dev, "(%d) Failed to put authority\n", __LINE__);

		return -EFAULT;
	}

	return 0;
}

static int rel_sem(struct ipc_spi *od)
{
	if (!od) {
		printk(KERN_ERR "[%s]onedram: Dev is NULL, but try to access\n", __func__);
		return -EFAULT;
	}

	if (!od->reg) {
		dev_err(od->dev, "(%d) Failed to put authority\n", __LINE__);

		return -EFAULT;
	}

	return 0;
}

int onedram_read_mailbox(u32 *mb)
{
	return _recv_cmd(ipc_spi, mb);
}
EXPORT_SYMBOL(onedram_read_mailbox);

int onedram_write_mailbox(u32 mb)
{
	return _send_cmd(ipc_spi, mb);
}
EXPORT_SYMBOL(onedram_write_mailbox);

void onedram_init_mailbox(void)
{
	int r = 0;
	/* flush mailbox before registering onedram irq */
	r = ipc_spi->reg->mailbox_AB;

	/* Set nINT_ONEDRAM_CP to low */
	ipc_spi->reg->mailbox_BA = 0x0;
}
EXPORT_SYMBOL(onedram_init_mailbox);

int onedram_get_auth(u32 cmd)
{
	return get_auth(ipc_spi, cmd);
}
EXPORT_SYMBOL(onedram_get_auth);

int onedram_put_auth(int release)
{
	return put_auth(ipc_spi, release);
}
EXPORT_SYMBOL(onedram_put_auth);

int onedram_rel_sem(void)
{
	return rel_sem(ipc_spi);
}
EXPORT_SYMBOL(onedram_rel_sem);

int onedram_read_sem(void)
{
	return _read_sem(ipc_spi);
}
EXPORT_SYMBOL(onedram_read_sem);

void onedram_get_vbase(void **vbase)
{
	*vbase = (void *)ipc_spi->mmio;
}
EXPORT_SYMBOL(onedram_get_vbase);


static int ipc_spi_irq_log_flag;
static irqreturn_t ipc_spi_irq_handler(int irq, void *data)	/* SRDY Rising EDGE ISR */
{
	struct ipc_spi *od = (struct ipc_spi *)data;
	int srdy_pin = 0;

	srdy_pin = gpio_get_value(gpio_srdy);

	if (!boot_done)
		return IRQ_HANDLED;

	/*
	if (ipc_spi_irq_log_flag)
		dev_dbg(od->dev, "(%d) isr SRDY : %d\n", __LINE__, srdy_pin);
	*/

	if (!srdy_pin) {
		printk(KERN_ERR "SRDY LOW.\n");
		return IRQ_HANDLED;
	}

	up(&srdy_sem);

	if (transfer_thread_waiting) {
		transfer_thread_waiting = 0;

		if (ipc_spi_irq_log_flag)
			dev_dbg(od->dev, "(%d) signal transfer event.\n", __LINE__);

		up(&transfer_event_sem);
	}

	return IRQ_HANDLED;
}

static int ipc_spi_vm_pagefault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct ipc_spi *od = vma->vm_private_data;

	vmf->page = vmalloc_to_page(od->mmio + (vmf->pgoff << PAGE_SHIFT));
	get_page(vmf->page);

	return 0;
}

static struct vm_operations_struct ipc_spi_vm_ops = {
	.fault = ipc_spi_vm_pagefault,
};

static int ipc_spi_open(struct inode *inode, struct file *filp)
{
	struct cdev *cdev = inode->i_cdev;
	struct ipc_spi *od = container_of(cdev, struct ipc_spi, cdev);

	filp->private_data = od;
	return 0;
}

static int ipc_spi_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static unsigned int ipc_spi_poll(struct file *filp, poll_table *wait)
{
	struct ipc_spi *od;
	u32 data;

	od = filp->private_data;

	poll_wait(filp, &od->waitq, wait);

	spin_lock_irq(&ipc_spi_lock);
	data = od->mailbox;
	spin_unlock_irq(&ipc_spi_lock);

	if (data)
		return POLLIN | POLLRDNORM;

	return 0;
}

static ssize_t ipc_spi_read(struct file *filp, char __user *buf,
		size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	u32 data;
	ssize_t retval;
	struct ipc_spi *od;

	od = filp->private_data;

	if (count < sizeof(u32))
		return -EINVAL;

	add_wait_queue(&od->waitq, &wait);

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);

		spin_lock_irq(&ipc_spi_lock);
		data = od->mailbox;
		od->mailbox = 0;
		spin_unlock_irq(&ipc_spi_lock);

		if (data)
			break;
		else if (filp->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		} else if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	}

	retval = put_user(data, (u32 __user *)buf);
	if (!retval)
		retval = sizeof(u32);
out:
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&od->waitq, &wait);

	return retval;
}

static ssize_t ipc_spi_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *ppos)
{
	struct ipc_spi *od;

	od = filp->private_data;

	if (count) {
		u32 data;

		if (get_user(data, (u32 __user *)buf))
			return -EFAULT;

		_send_cmd(od, data);
	}

	return count;
}

static int ipc_spi_fasync(int fd, struct file *filp, int on)
{
	struct ipc_spi *od;

	od = filp->private_data;

	return fasync_helper(fd, filp, on, &od->async_queue);
}

static int ipc_spi_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ipc_spi *od;
	unsigned long size;
	unsigned long offset;

	od = filp->private_data;
	if (!od || !vma)
		return -EFAULT;

	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	dev_dbg(od->dev, "(%d) Req region: 0x%08lx 0x%08lx\n", __LINE__, offset, size);

	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &ipc_spi_vm_ops;
	vma->vm_private_data = od;

	dev_dbg(od->dev, "(%d) ipc_spi_mmap Done.\n", __LINE__);

	return 0;
}

int ipc_spi_sema_init(void)
{
	printk(KERN_ERR "[SPI] Srdy sema init\n");
	sema_init(&srdy_sem, 0);
	return 0;
}

static long ipc_spi_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{

	int r = 0;

	printk(KERN_ERR "[%s]\n", __func__);

	switch (cmd) {
	case ONEDRAM_CP_CRASH:
		panic("CP Crash");
		break;
	case ONEDRAM_REL_SEM:
		r = ipc_spi_thread_restart();
		break;
	case ONEDRAM_SEMA_INIT:
		r = ipc_spi_sema_init();
		break;
	default:
		r = -ENOIOCTLCMD;
		break;
	}

	return r;
}

static const struct file_operations ipc_spi_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = ipc_spi_read,
	.write = ipc_spi_write,
	.poll = ipc_spi_poll,
	.fasync = ipc_spi_fasync,
	.open = ipc_spi_open,
	.release = ipc_spi_release,
	.mmap = ipc_spi_mmap,
	.unlocked_ioctl = ipc_spi_ioctl,
};

static int _register_chrdev(struct ipc_spi *od)
{
	int r;
	dev_t devid;

	od->class = class_create(THIS_MODULE, DRVNAME);
	if (IS_ERR(od->class)) {
		r = PTR_ERR(od->class);
		od->class = NULL;
		return r;
	}

	r = alloc_chrdev_region(&devid, 0, 1, DRVNAME);
	if (r)
		return r;

	cdev_init(&od->cdev, &ipc_spi_fops);

	r = cdev_add(&od->cdev, devid, 1);
	if (r) {
		unregister_chrdev_region(devid, 1);
		return r;
	}
	od->devid = devid;

	od->dev = device_create(od->class, NULL, od->devid, od, DRVNAME);
	if (IS_ERR(od->dev)) {
		r = PTR_ERR(od->dev);
		od->dev = NULL;
		return r;
	}
	dev_set_drvdata(od->dev, od);

	return 0;
}

static inline int _request_mem(struct ipc_spi *od, struct platform_device *pdev)
{
	od->mmio = vmalloc(od->size);
	if (!od->mmio) {
		dev_err(&pdev->dev, "(%d) Failed to vmalloc size : %lu\n", __LINE__, od->size);

		return -EBUSY;
	} else {
		dev_dbg(&pdev->dev, "(%d) vmalloc Done. mmio : 0x%08x\n", __LINE__, (u32)od->mmio);
	}

	memset((void *)od->mmio, 0, od->size);

	od->reg = (struct onedram_reg_mapped *)(
			(char *)od->mmio + ONEDRAM_REG_OFFSET);
	dev_dbg(&pdev->dev, "(%d) Onedram semaphore: %d\n", __LINE__, _read_sem(od));

	od->reg->sem = 1;
	dev_dbg(&pdev->dev, "(%d) force set sem to 1 : %d\n", __LINE__, od->reg->sem);

	od->reg->mailbox_AB = 0x000000C8;
	od->reg->mailbox_BA = 0;
	dev_dbg(&pdev->dev, "(%d) force set mailbox to 0 : AB=0x%x, BA=0x%x\n", __LINE__, od->reg->mailbox_AB, od->reg->mailbox_BA);

	ipc_spi_resource.start = (resource_size_t)od->mmio;
	ipc_spi_resource.end = (resource_size_t)od->mmio + od->size - 1;

	p_virtual_buff = od->mmio;

	return 0;
}

static void _release(struct ipc_spi *od)
{
	if (!od)
		return;

	if (od->irq)
		free_irq(od->irq, od);

	if (od->group)
		sysfs_remove_group(&od->dev->kobj, od->group);

	if (od->dev)
		device_destroy(od->class, od->devid);

	if (od->devid) {
		cdev_del(&od->cdev);
		unregister_chrdev_region(od->devid, 1);
	}

	if (od->mmio) {
		od->reg = NULL;

		vfree((void *)od->mmio);

		ipc_spi_resource.start = 0;
		ipc_spi_resource.end = -1;
	}

	if (od->class)
		class_destroy(od->class);

	kfree(od);
}

struct resource *onedram_request_region(resource_size_t start,
		resource_size_t n, const char *name)
{
	struct resource *res;

	start += ipc_spi_resource.start;
	res = __request_region(&ipc_spi_resource, start, n, name, 0);
	if (!res)
		return NULL;

	return res;
}
EXPORT_SYMBOL(onedram_request_region);

void onedram_release_region(resource_size_t start, resource_size_t n)
{
	start += ipc_spi_resource.start;
	__release_region(&ipc_spi_resource, start, n);
}
EXPORT_SYMBOL(onedram_release_region);

int onedram_register_handler(void (*handler)(u32, void *), void *data)
{
	unsigned long flags;
	struct ipc_spi_handler *hd;

	if (!handler)
		return -EINVAL;

	hd = kzalloc(sizeof(struct ipc_spi_handler), GFP_KERNEL);
	if (!hd)
		return -ENOMEM;

	hd->data = data;
	hd->handler = handler;

	spin_lock_irqsave(&h_list.lock, flags);
	list_add_tail(&hd->list, &h_list.list);
	h_list.len++;
	spin_unlock_irqrestore(&h_list.lock, flags);

	return 0;
}
EXPORT_SYMBOL(onedram_register_handler);

int onedram_unregister_handler(void (*handler)(u32, void *))
{
	unsigned long flags;
	struct list_head *l, *tmp;

	if (!handler)
		return -EINVAL;

	spin_lock_irqsave(&h_list.lock, flags);
	list_for_each_safe(l, tmp, &h_list.list) {
		struct ipc_spi_handler *hd = list_entry(l, struct ipc_spi_handler, list);

		if (hd->handler == handler) {
			list_del(&hd->list);
			h_list.len--;
			kfree(hd);
		}
	}
	spin_unlock_irqrestore(&h_list.lock, flags);

	return 0;
}
EXPORT_SYMBOL(onedram_unregister_handler);

static void _unregister_all_handlers(void)
{
	unsigned long flags;
	struct list_head *l, *tmp;

	spin_lock_irqsave(&h_list.lock, flags);
	list_for_each_safe(l, tmp, &h_list.list) {
		struct ipc_spi_handler *hd = list_entry(l, struct ipc_spi_handler, list);

		list_del(&hd->list);
		h_list.len--;
		kfree(hd);
	}
	spin_unlock_irqrestore(&h_list.lock, flags);
}

static void _init_data(struct ipc_spi *od)
{
	init_completion(&od->comp);
	atomic_set(&od->ref_sem, 0);
	INIT_LIST_HEAD(&h_list.list);
	spin_lock_init(&h_list.lock);
	h_list.len = 0;
	init_waitqueue_head(&od->waitq);

	init_completion(&ril_init);
	sema_init(&srdy_sem, 0);
	sema_init(&transfer_event_sem, 0);
}

static u32 ipc_spi_get_send_vbuff_command(void)
{
	u32 cmd = 0;

	memcpy((void *)&cmd, (void *)(p_virtual_buff + ONEDRAM_REG_OFFSET + 64), sizeof(cmd));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get mailbox_BA cmd : 0x%x\n", __LINE__, cmd); */

	return cmd;
}

static void ipc_spi_set_send_vbuff_command_clear(void)
{
	u32 cmd = 0;

	memcpy((void *)(p_virtual_buff + ONEDRAM_REG_OFFSET + 64), (void *)&cmd, sizeof(cmd));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) clear mailbox_BA cmd : 0x%x\n", __LINE__, cmd); */
}

static inline void ipc_spi_get_pointer_of_vbuff_format_tx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 16), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 20), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get FMT tx_head : %d, tx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_get_pointer_of_vbuff_format_rx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 16 + 8), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 20 + 8), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get FMT rx_head : %d, rx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_get_pointer_of_vbuff_raw_tx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 32), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 36), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RAW tx_head : %d, tx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_get_pointer_of_vbuff_raw_rx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 32 + 8), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 36 + 8), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RAW rx_head : %d, rx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_get_pointer_of_vbuff_rfs_tx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 48), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 52), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RFS tx_head : %d, tx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_get_pointer_of_vbuff_rfs_rx(u32 *head, u32 *tail)
{
	memcpy((void *)head, (void *)(p_virtual_buff + 48 + 8), sizeof(*head));
	memcpy((void *)tail, (void *)(p_virtual_buff + 52 + 8), sizeof(*tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RFS rx_head : %d, rx_tail : %d\n", __LINE__, *head, *tail); */
}

static inline void ipc_spi_update_tail_of_vbuff_format_tx(u32 u_tail)
{
	memcpy((void *)(p_virtual_buff + 20), (void *)&u_tail, sizeof(u_tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update FMT tx_tail : %d\n", __LINE__, u_tail); */
}

static inline void ipc_spi_update_head_of_vbuff_format_rx(u32 u_head)
{
	memcpy((void *)(p_virtual_buff + 20 + 4), (void *)&u_head, sizeof(u_head));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update FMT rx_head : %d\n", __LINE__, u_head); */
}

static inline void ipc_spi_update_tail_of_vbuff_raw_tx(u32 u_tail)
{
	memcpy((void *)(p_virtual_buff + 36), (void *)&u_tail, sizeof(u_tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update RAW tx_tail : %d\n", __LINE__, u_tail); */
}

static inline void ipc_spi_update_head_of_vbuff_raw_rx(u32 u_head)
{
	memcpy((void *)(p_virtual_buff + 36 + 4), (void *)&u_head, sizeof(u_head));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update RAW rx_head : %d\n", __LINE__, u_head); */
}

static inline void ipc_spi_update_tail_of_vbuff_rfs_tx(u32 u_tail)
{
	memcpy((void *)(p_virtual_buff + 52), (void *)&u_tail, sizeof(u_tail));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update RFS tx_tail : %d\n", __LINE__, u_tail); */
}

static inline void ipc_spi_update_head_of_vbuff_rfs_rx(u32 u_head)
{
	memcpy((void *)(p_virtual_buff + 52 + 4), (void *)&u_head, sizeof(u_head));
	/* dev_dbg(&p_ipc_spi->dev, "(%d) update RFS rx_head : %d\n", __LINE__, u_head); */
}

#ifndef FEATURE_SAMSUNG_SPI
static
#endif
u32 ipc_spi_get_length_vbuff_format_tx(u32 *out_head, u32 *out_tail)
{
	u32 len = 0;

	ipc_spi_get_pointer_of_vbuff_format_tx(out_head, out_tail);

	if (*out_head >= *out_tail)
		len = *out_head - *out_tail;
	else
		len = FMT_SZ - *out_tail + *out_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get FMT tx_len : %d\n", __LINE__, len); */

	return len;
}

static u32 ipc_spi_get_length_vbuff_raw_tx(u32 *out_head, u32 *out_tail)
{
	u32 len = 0;

	ipc_spi_get_pointer_of_vbuff_raw_tx(out_head, out_tail);

	if (*out_head >= *out_tail)
		len = *out_head - *out_tail;
	else
		len = RAW_SZ - *out_tail + *out_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RAW tx_len : %d\n", __LINE__, len); */

	return len;
}

static u32 ipc_spi_get_length_vbuff_rfs_tx(u32 *out_head, u32 *out_tail)
{
	u32 len = 0;

	ipc_spi_get_pointer_of_vbuff_rfs_tx(out_head, out_tail);

	if (*out_head >= *out_tail)
		len = *out_head - *out_tail;
	else
		len = RFS_SZ - *out_tail + *out_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RFS tx_len : %d\n", __LINE__, len); */

	return len;
}

static u32 ipc_spi_get_space_vbuff_format_rx(u32 *in_head, u32 *in_tail)
{
	u32 space = 0;

	ipc_spi_get_pointer_of_vbuff_format_rx(in_head, in_tail);

	/* check the memory space remained */
	if (*in_tail <= *in_head)
		space = FMT_SZ - *in_head + *in_tail;
	else
		space = *in_tail - *in_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get FMT rx space : %d\n", __LINE__, space); */

	return space;
}

static u32 ipc_spi_get_space_vbuff_raw_rx(u32 *in_head, u32 *in_tail)
{
	u32 space = 0;

	ipc_spi_get_pointer_of_vbuff_raw_rx(in_head, in_tail);

	/* check the memory space remained */
	if (*in_tail <= *in_head)
		space = RAW_SZ - *in_head + *in_tail;
	else
		space = *in_tail - *in_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RAW rx space : %d\n", __LINE__, space); */

	return space;
}

static u32 ipc_spi_get_space_vbuff_rfs_rx(u32 *in_head, u32 *in_tail)
{
	u32 space = 0;

	ipc_spi_get_pointer_of_vbuff_rfs_rx(in_head, in_tail);

	/* check the memory space remained */
	if (*in_tail <= *in_head)
		space = RFS_SZ - *in_head + *in_tail;
	else
		space = *in_tail - *in_head;

	/* dev_dbg(&p_ipc_spi->dev, "(%d) get RFS rx space : %d\n", __LINE__, space); */

	return space;
}

static int ipc_spi_check_send_data(void)
{
	int retval = 0;
	u32 out_head = 0;
	u32 out_tail = 0;

#ifdef LOOPBACK_TEST
	if (loop_back_test)
		retval = 1;
#endif

	if (ipc_spi_get_send_vbuff_command())
		retval = 1;

	ipc_spi_get_pointer_of_vbuff_format_tx(&out_head, &out_tail);
	if (out_head != out_tail)
		retval = 1;

	ipc_spi_get_pointer_of_vbuff_raw_tx(&out_head, &out_tail);
	if (out_head != out_tail)
		retval = 1;

	ipc_spi_get_pointer_of_vbuff_rfs_tx(&out_head, &out_tail);
	if (out_head != out_tail)
		retval = 1;

	return retval;
}

static void ipc_spi_copy_from_vbuff_format_tx(void *p_des, u32 offset_vbuff, u32 copy_len)
{
	if ((offset_vbuff + copy_len) <= FMT_SZ) {
		memcpy((void *)p_des, (void *)(p_virtual_buff + FMT_OUT + offset_vbuff), copy_len);
	} else {
		memcpy((void *)p_des, (void *)(p_virtual_buff + FMT_OUT + offset_vbuff), FMT_SZ - offset_vbuff);
		memcpy((void *)(p_des + (FMT_SZ - offset_vbuff)), (void *)(p_virtual_buff + FMT_OUT), copy_len - (FMT_SZ - offset_vbuff));
	}
}

static void ipc_spi_copy_from_vbuff_raw_tx(void *p_des, u32 offset_vbuff, u32 copy_len)
{
	if ((offset_vbuff + copy_len) <= RAW_SZ) {
		memcpy((void *)p_des, (void *)(p_virtual_buff + RAW_OUT + offset_vbuff), copy_len);
	} else {
		memcpy((void *)p_des, (void *)(p_virtual_buff + RAW_OUT + offset_vbuff), RAW_SZ - offset_vbuff);
		memcpy((void *)(p_des + (RAW_SZ - offset_vbuff)), (void *)(p_virtual_buff + RAW_OUT), copy_len - (RAW_SZ - offset_vbuff));
	}
}

static void ipc_spi_copy_from_vbuff_rfs_tx(void *p_des, u32 offset_vbuff, u32 copy_len)
{
	if ((offset_vbuff + copy_len) <= RFS_SZ) {
		memcpy((void *)p_des, (void *)(p_virtual_buff + RFS_OUT + offset_vbuff), copy_len);
	} else {
		memcpy((void *)p_des, (void *)(p_virtual_buff + RFS_OUT + offset_vbuff), RFS_SZ - offset_vbuff);
		memcpy((void *)(p_des + (RFS_SZ - offset_vbuff)), (void *)(p_virtual_buff + RFS_OUT), copy_len - (RFS_SZ - offset_vbuff));
	}
}

#ifdef LOOPBACK_TEST
static void ipc_spi_prepare_tx_data(u8 *tx_b)
{
	u32 len = 0, tx_b_remail_len = DEF_BUF_SIZE, read_size = 0;
	struct spi_protocol_header *tx_header = (struct spi_protocol_header *)tx_b;
	u32 cmd = 0;
	u8 cmd_8 = 0;
	u16 mux = 0;
	u16 p_tx_b = sizeof(struct spi_protocol_header);
	u32 p_send_data_h = 0, p_send_data_t = 0;
	u16 pkt_fmt_len = 0;
	u32 pkt_len = 0;
	u8 bof = 0, eof = 0;
	int i;

	memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

	cmd = ipc_spi_get_send_vbuff_command(); /* check mailbox command data */
	if (cmd) {
		cmd_8 = cmd & 0xFF;
		dev_dbg(&p_ipc_spi->dev, "(%d) =>exist CMD cmd_8 : %x\n", __LINE__, cmd_8);

		mux = 0x0004;
		memcpy((void *)(tx_b + sizeof(struct spi_protocol_header)), (void *)&mux, sizeof(mux));

		memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux)), (void *)&cmd_8, sizeof(cmd_8));
		ipc_spi_set_send_vbuff_command_clear();

		tx_header->current_data_size = sizeof(mux) + sizeof(cmd_8);
		tx_header->next_data_size = DEF_BUF_SIZE >> 2;

		if (ipc_spi_check_send_data())
			tx_header->more = 1;
		else
			tx_header->more = 0;

	} else { /* check format, raw, rfs data */
		len = ipc_spi_get_length_vbuff_format_tx(&p_send_data_h, &p_send_data_t); /* len : vbuff_format_tx length */
		if (len) {
			dev_dbg(&p_ipc_spi->dev, "(%d) =>prepare FMT data\n", __LINE__);

			mux = 0x0001;
			read_size = 0;

			while (len > read_size) { /* till len is zero */
				/* bof : 0x7F check */
				ipc_spi_copy_from_vbuff_format_tx((void *)&bof, p_send_data_t, sizeof(bof));

				if (bof != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) FMT bof error, remove invalid data. bof : %x\n", __LINE__, bof);

					ipc_spi_update_tail_of_vbuff_format_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				}

				/* packet length check */
				ipc_spi_copy_from_vbuff_format_tx((void *)&pkt_fmt_len, p_send_data_t + sizeof(bof), sizeof(pkt_fmt_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) =>FMT packet len : %d\n", __LINE__, pkt_fmt_len);

				if ((pkt_fmt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) FMT wrong packet len, remove invalid data. packet len : %x\n", __LINE__, pkt_fmt_len);

					ipc_spi_update_tail_of_vbuff_format_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				} else if ((pkt_fmt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > tx_b_remail_len) {
					dev_dbg(&p_ipc_spi->dev, "(%d) =>FMT tx more set\n", __LINE__);

					tx_header->more = 1;

					break;
				}

				/* make spi tx packet (1 packet : vbuff -> tx_b) */
				memcpy((void *)(tx_b + p_tx_b), (void *)&mux, sizeof(mux));
				p_tx_b += sizeof(mux);
				tx_b_remail_len -= sizeof(mux);

				ipc_spi_copy_from_vbuff_format_tx((void *)(tx_b + p_tx_b), p_send_data_t, pkt_fmt_len + sizeof(bof) + sizeof(eof));

#ifdef FORMAT_TX_DUMP
				printk(KERN_ERR "[IPC_SPI => FMT TX :");
				for (i = 0; i < (pkt_fmt_len + sizeof(bof) + sizeof(eof)); i++)
					printk(KERN_ERR " %02x", *((u8 *)(tx_b + p_tx_b + i)));

				printk(KERN_ERR "]\n");
#endif

				p_tx_b += pkt_fmt_len + sizeof(bof) + sizeof(eof);
				p_send_data_t += pkt_fmt_len + sizeof(bof) + sizeof(eof);
				tx_b_remail_len -= pkt_fmt_len + sizeof(bof) + sizeof(eof);

				p_send_data_t %= FMT_SZ;
				ipc_spi_update_tail_of_vbuff_format_tx(p_send_data_t); /* in-pointer update */

				read_size += pkt_fmt_len + sizeof(bof) + sizeof(eof);
				if (len < read_size)
					dev_err(&p_ipc_spi->dev, "(%d) FMT tx read error len : %d, read_size : %d\n", __LINE__, len, read_size);

				dev_dbg(&p_ipc_spi->dev, "(%d) =>FMT len : %d\n", __LINE__, len);
			}

			tx_header->current_data_size = DEF_BUF_SIZE - tx_b_remail_len;
			tx_header->next_data_size = DEF_BUF_SIZE >> 2;
		}

		len = ipc_spi_get_length_vbuff_raw_tx(&p_send_data_h, &p_send_data_t);
		if (len) {
			dev_dbg(&p_ipc_spi->dev, "(%d) =>prepare RAW data\n", __LINE__);

			mux = 0x0002;
			read_size = 0;

			while (len > read_size) { /* till len is zero */
				/* bof : 0x7F check */
				ipc_spi_copy_from_vbuff_raw_tx((void *)&bof, p_send_data_t, sizeof(bof));

				if (bof != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) RAW bof error, remove invalid data. bof : %x\n", __LINE__, bof);

					ipc_spi_update_tail_of_vbuff_raw_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				}

				/* packet length check */
				ipc_spi_copy_from_vbuff_raw_tx((void *)&pkt_len, p_send_data_t + sizeof(bof), sizeof(pkt_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) =>RAW packet len : %d\n", __LINE__, pkt_len);

				if ((pkt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) RAW wrong packet len, remove invalid data. packet len : %x\n", __LINE__, pkt_len);

					ipc_spi_update_tail_of_vbuff_raw_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				} else if ((pkt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > tx_b_remail_len) {
					dev_dbg(&p_ipc_spi->dev, "(%d) =>RAW tx more set\n", __LINE__);

					tx_header->more = 1;

					break;
				}

				/* make spi tx packet (1 packet : vbuff -> tx_b) */
				memcpy((void *)(tx_b + p_tx_b), (void *)&mux, sizeof(mux));
				p_tx_b += sizeof(mux);
				tx_b_remail_len -= sizeof(mux);

				ipc_spi_copy_from_vbuff_raw_tx((void *)(tx_b + p_tx_b), p_send_data_t, pkt_len + sizeof(bof) + sizeof(eof));

#ifdef RAW_TX_DUMP
				printk(KERN_ERR "[IPC_SPI => RAW TX :");
				for (i = 0 ; i < (pkt_len + sizeof(bof) + sizeof(eof)) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(tx_b + p_tx_b + i)));

				printk(KERN_ERR "]\n");
#endif

				p_tx_b += pkt_len + sizeof(bof) + sizeof(eof);
				p_send_data_t += pkt_len + sizeof(bof) + sizeof(eof);
				tx_b_remail_len -= pkt_len + sizeof(bof) + sizeof(eof);

				p_send_data_t %= RAW_SZ;
				ipc_spi_update_tail_of_vbuff_raw_tx(p_send_data_t); /* in-pointer update */

				read_size += pkt_len + sizeof(bof) + sizeof(eof);
				if (len < read_size)
					dev_err(&p_ipc_spi->dev, "(%d) RAW tx read error len : %d, read_size : %d\n", __LINE__, len, read_size);

				dev_dbg(&p_ipc_spi->dev, "(%d) =>RAW len : %d\n", __LINE__, len);
			}

			tx_header->current_data_size = DEF_BUF_SIZE - tx_b_remail_len;
			tx_header->next_data_size = DEF_BUF_SIZE >> 2;
		}

		len = ipc_spi_get_length_vbuff_rfs_tx(&p_send_data_h, &p_send_data_t);
		if (len) {
			dev_dbg(&p_ipc_spi->dev, "(%d) =>prepare RFS data\n", __LINE__);

			mux = 0x0003;
			read_size = 0;

			while (len > read_size) { /* till len is zero */
				/* bof : 0x7F check */
				ipc_spi_copy_from_vbuff_rfs_tx((void *)&bof, p_send_data_t, sizeof(bof));

				if (bof != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) RFS bof error, remove invalid data. bof : %x\n", __LINE__, bof);

					ipc_spi_update_tail_of_vbuff_rfs_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				}

				/* packet length check */
				ipc_spi_copy_from_vbuff_rfs_tx((void *)&pkt_len, p_send_data_t + sizeof(bof), sizeof(pkt_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) =>RFS packet len : %d\n", __LINE__, pkt_len);

				if ((pkt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) RFS wrong packet len, remove invalid data. packet len : %x\n", __LINE__, pkt_len);

					ipc_spi_update_tail_of_vbuff_rfs_tx(p_send_data_h); /* remove invalid data */
					memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

					return;
				} else if ((pkt_len + sizeof(bof) + sizeof(eof) + sizeof(mux)) > tx_b_remail_len) {
					dev_dbg(&p_ipc_spi->dev, "(%d) =>RFS tx more set\n", __LINE__);

					tx_header->more = 1;

					break;
				}

				/* make spi tx packet (1 packet : vbuff -> tx_b) */
				memcpy((void *)(tx_b + p_tx_b), (void *)&mux, sizeof(mux));
				p_tx_b += sizeof(mux);
				tx_b_remail_len -= sizeof(mux);

				ipc_spi_copy_from_vbuff_rfs_tx((void *)(tx_b + p_tx_b), p_send_data_t, pkt_len + sizeof(bof) + sizeof(eof));

#ifdef RFS_TX_DUMP
				printk(KERN_ERR "[IPC_SPI => RFS TX :");
				for (i = 0 ; i < (pkt_len + sizeof(bof) + sizeof(eof)) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(tx_b + p_tx_b + i)));

				printk(KERN_ERR "]\n");
#endif

				p_tx_b += pkt_len + sizeof(bof) + sizeof(eof);
				p_send_data_t += pkt_len + sizeof(bof) + sizeof(eof);
				tx_b_remail_len -= pkt_len + sizeof(bof) + sizeof(eof);

				p_send_data_t %= RFS_SZ;
				ipc_spi_update_tail_of_vbuff_rfs_tx(p_send_data_t); /* in-pointer update */

				read_size += pkt_len + sizeof(bof) + sizeof(eof);
				if (len < read_size)
					dev_err(&p_ipc_spi->dev, "(%d) RFS tx read error len : %d, read_size : %d\n", __LINE__, len, read_size);

				dev_dbg(&p_ipc_spi->dev, "(%d) =>RFS len : %d\n", __LINE__, len);

#ifdef RFS_TX_RX_LENGTH_DUMP
				printk(KERN_ERR "[IPC_SPI => RFS TX : %d]\n", pkt_len);
#endif
			}

			tx_header->current_data_size = DEF_BUF_SIZE - tx_b_remail_len;
			tx_header->next_data_size = DEF_BUF_SIZE >> 2;
		}
	}

#ifndef LOOPBACK_TEST
	dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] TX : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		tx_b[0], tx_b[1], tx_b[2], tx_b[3], tx_b[4], tx_b[5], tx_b[6], tx_b[7], tx_b[8], tx_b[9], tx_b[10], tx_b[11], tx_b[12], tx_b[13], tx_b[14], tx_b[15], tx_b[16], tx_b[17], tx_b[18], tx_b[19],
		tx_b[20], tx_b[21], tx_b[22], tx_b[23], tx_b[24], tx_b[25], tx_b[26], tx_b[27], tx_b[28], tx_b[29], tx_b[30], tx_b[31], tx_b[32], tx_b[33], tx_b[34], tx_b[35], tx_b[36], tx_b[37], tx_b[38], tx_b[39]);
#endif
}

static void ipc_spi_prepare_loopback_tx_data(u8 *tx_b, u8 *rx_b, u8 *test_data)
{
	struct spi_protocol_header *tx_header = (struct spi_protocol_header *)tx_b;
	u16 mux = 0;
	u8 bof = 0x7F, eof = 0x7E;
	u16 pkt_len = LOOP_SIZE - 8;
	u16 ipc_len = pkt_len - 3;
	u32 ipc_header = 0x010B0A00;

	int i;

	memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

	mux = 0x0001;

	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header)), (void *)&mux, sizeof(mux));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux)), (void *)&bof, sizeof(bof));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof)), (void *)&pkt_len, sizeof(pkt_len));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len) + 1), (void *)&ipc_len, sizeof(ipc_len));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len) + 4), (void *)&ipc_header, sizeof(ipc_header));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len) + 4 + 4), (void *)test_data, LOOP_SIZE-18);
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + pkt_len), (void *)&eof, sizeof(eof));

	tx_header->current_data_size = sizeof(mux) + sizeof(bof) + pkt_len + sizeof(eof);
	tx_header->next_data_size = DEF_BUF_SIZE >> 2;

	printk(KERN_ERR "[SPI DUMP] LOOP TX :]");
	for (i = 0 ; i < 30 ; i++)
		printk(KERN_ERR "%02x ", *((u8 *)(tx_b + i)));
	printk(KERN_ERR "| ");
	for (i = LOOP_SIZE - 30 ; i < LOOP_SIZE ; i++)
		printk(KERN_ERR "%02x ", *((u8 *)(tx_b + i)));
	printk(KERN_ERR "]\n");

}
#endif

#if 0
static void ipc_spi_prepare_loopback_tx_data(u8 *tx_b, u8 *rx_b)
{
	struct spi_protocol_header *tx_header = (struct spi_protocol_header *)tx_b;
	u16 mux = 0;
	u8 bof = 0x7F, eof = 0x7E;
	u32 pkt_len = 16;
	u8 test_data[10] = "0123456789";
	u8 ch_id = 31, control_id = 0;

	int i;

	memset((void *)tx_b, 0, DEF_BUF_SIZE + 4);

	mux = 0x0002;

	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header)), (void *)&mux, sizeof(mux));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux)), (void *)&bof, sizeof(bof));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof)), (void *)&pkt_len, sizeof(pkt_len));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len)),
		(void *)&ch_id, sizeof(ch_id));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len) + sizeof(ch_id)),
		(void *)&control_id, sizeof(control_id));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + sizeof(pkt_len)
		+ sizeof(ch_id) + sizeof(control_id)), (void *)&test_data, sizeof(test_data));
	memcpy((void *)(tx_b + sizeof(struct spi_protocol_header) + sizeof(mux) + sizeof(bof) + pkt_len), (void *)&eof, sizeof(eof));

	tx_header->current_data_size = sizeof(mux) + sizeof(bof) + pkt_len + sizeof(eof);
	tx_header->next_data_size = DEF_BUF_SIZE >> 2;

	printk(KERN_ERR "[SPI DUMP] LOOP TX : ");
	for (i = 0 ; i < 30 ; i++)
		printk(KERN_ERR "%02x ", tx_b[i]);
	printk(KERN_ERR "\n");
}
#endif

static void ipc_spi_set_MRDY_pin(int val)
{
	/* dev_dbg(&p_ipc_spi->dev, "(%d) set MRDY %d\n", __LINE__, val); */

	gpio_set_value(gpio_mrdy, val);
}

#ifndef FEATURE_SAMSUNG_SPI
static
#endif
int ipc_spi_tx_rx_sync(u8 *tx_d, u8 *rx_d, unsigned len)
{
	struct spi_transfer t;
	struct spi_message msg;

	memset(&t, 0, sizeof t);

	t.len = len;

	t.tx_buf = tx_d;
	t.rx_buf = rx_d;

	t.cs_change = 0;

	t.bits_per_word = 32;
	t.speed_hz = 12000000;

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);

	return spi_sync(p_ipc_spi, &msg);
}

static void ipc_spi_swap_data_htn(u8 *data, int len)
{
	int i;

	for (i = 0 ; i < len ; i += 4)
		*(u32 *)(data + i) = htonl(*(u32 *)(data + i));
}

static void ipc_spi_swap_data_nth(u8 *data, int len)
{
	int i;

	for (i = 0 ; i < len ; i += 4)
		*(u32 *)(data + i) = ntohl(*(u32 *)(data + i));
}

static int ipc_spi_copy_to_vbuff_format_rx(void *data, u16 len)
{
	u32 head = 0, tail = 0, new_head = 0, space = 0;
	int copy_retry_count = 0;

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to FMT vbuff, len : %d\n", __LINE__, len);

COPY_TO_VBUFF_FMT_RETRY:

	space = ipc_spi_get_space_vbuff_format_rx(&head, &tail);
	if (space < len) {
		dev_err(&p_ipc_spi->dev, "(%d) FMT vbuff is full, space : %d, len : %d\n", __LINE__, space, len);

		copy_retry_count++;
		if (copy_retry_count > 20) {
			dev_err(&p_ipc_spi->dev, "(%d) FMT vbuff is full. copy fail.\n", __LINE__);

			return -ENOMEM;
		}

		msleep(20);
		goto COPY_TO_VBUFF_FMT_RETRY;
	}

	if (head + len <= FMT_SZ) {
		memcpy((void *)(p_virtual_buff + FMT_IN + head), data, len);
	} else {
		memcpy((void *)(p_virtual_buff + FMT_IN + head), data, FMT_SZ - head);
		memcpy((void *)(p_virtual_buff + FMT_IN), (void *)(data + (FMT_SZ - head)), len - (FMT_SZ - head));
	}

	new_head = (head + len) % FMT_SZ;
	ipc_spi_update_head_of_vbuff_format_rx(new_head);

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to FMT vbuff done.\n", __LINE__);

	return 0;
}

static int ipc_spi_copy_to_vbuff_raw_rx(void *data, u32 len)
{
	u32 head = 0, tail = 0, new_head = 0, space = 0;
	int copy_retry_count = 0;

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to RAW vbuff, len : %d\n", __LINE__, len);

COPY_TO_VBUFF_RAW_RETRY:

	space = ipc_spi_get_space_vbuff_raw_rx(&head, &tail);
	if (space < len) {
		dev_err(&p_ipc_spi->dev, "(%d) RAW vbuff is full, space : %d, len : %d\n", __LINE__, space, len);

		copy_retry_count++;
		if (copy_retry_count > 20) {
			dev_err(&p_ipc_spi->dev, "(%d) RAW vbuff is full. copy fail.\n", __LINE__);

			return -ENOMEM;
		}

		msleep(20);
		goto COPY_TO_VBUFF_RAW_RETRY;
	}

	if (head + len <= RAW_SZ) {
		memcpy((void *)(p_virtual_buff + RAW_IN + head), data, len);
	} else {
		memcpy((void *)(p_virtual_buff + RAW_IN + head), data, RAW_SZ - head);
		memcpy((void *)(p_virtual_buff + RAW_IN), (void *)(data + (RAW_SZ - head)), len - (RAW_SZ - head));
	}

	new_head = (head + len) % RAW_SZ;
	ipc_spi_update_head_of_vbuff_raw_rx(new_head);

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to RAW vbuff done.\n", __LINE__);

	return 0;
}

static int ipc_spi_copy_to_vbuff_rfs_rx(void *data, u32 len)
{
	u32 head = 0, tail = 0, new_head = 0, space = 0;
	int copy_retry_count = 0;

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to RFS vbuff, len : %d\n", __LINE__, len);

COPY_TO_VBUFF_RFS_RETRY:

	space = ipc_spi_get_space_vbuff_rfs_rx(&head, &tail);
	if (space < len) {
		dev_err(&p_ipc_spi->dev, "(%d) RFS vbuff is full, space : %d, len : %d\n", __LINE__, space, len);

		copy_retry_count++;
		if (copy_retry_count > 20) {
			dev_err(&p_ipc_spi->dev, "(%d) RFS vbuff is full. copy fail.\n", __LINE__);

			return -ENOMEM;
		}

		msleep(20);
		goto COPY_TO_VBUFF_RFS_RETRY;
	}

	if (head + len <= RFS_SZ) {
		memcpy((void *)(p_virtual_buff + RFS_IN + head), data, len);
	} else {
		memcpy((void *)(p_virtual_buff + RFS_IN + head), data, RFS_SZ - head);
		memcpy((void *)(p_virtual_buff + RFS_IN), (void *)(data + (RFS_SZ - head)), len - (RFS_SZ - head));
	}

	new_head = (head + len) % RFS_SZ;
	ipc_spi_update_head_of_vbuff_rfs_rx(new_head);

	dev_dbg(&p_ipc_spi->dev, "(%d) <=copy data to RFS vbuff done.\n", __LINE__);

	return 0;
}

#ifndef FEATURE_SAMSUNG_SPI
static
#endif
void ipc_spi_make_data_interrupt(u32 cmd,  struct ipc_spi *od)
{
	struct list_head *l;
	unsigned long flags;
	u32 mailbox;

	mailbox = cmd;
	/* dev_dbg(&p_ipc_spi->dev, "(%d) <=make data int : 0x%08x\n", __LINE__, mailbox); */

	if (h_list.len) {
		spin_lock_irqsave(&h_list.lock, flags);
		list_for_each(l, &h_list.list) {
			struct ipc_spi_handler *h = list_entry(l, struct ipc_spi_handler, list);

			if (h->handler)
				h->handler(mailbox, h->data);
		}
		spin_unlock_irqrestore(&h_list.lock, flags);

		spin_lock(&ipc_spi_lock);
		od->mailbox = mailbox;
		spin_unlock(&ipc_spi_lock);
	} else {
		od->mailbox = mailbox;
	}

/*	dev_dbg(&p_ipc_spi->dev, "(%d) <=send data int cmd event\n", __LINE__); */

	wake_up_interruptible(&od->waitq);
	kill_fasync(&od->async_queue, SIGIO, POLL_IN);
}

static int rx_prev_data_saved;
static u16 rx_prev_data_mux;
static u32 rx_prev_data_size;
static u32 rx_prev_data_remain;

static void ipc_spi_rx_process(u8 *rx_b,  u8 *rx_save_b, struct ipc_spi *od)
{
	int retval = 0;
	struct spi_protocol_header *rx_header = (struct spi_protocol_header *)rx_b;
	u32 total_size = 0, read_size = 0;
	u16 packet_fmt_len = 0;
	u32 packet_len = 0;
	u8 bof = 0, eof = 0;
	u16 mux = 0;
	u16 p_read = 4;
	u32 int_cmd = 0;
	int i;

/*	dev_dbg(&p_ipc_spi->dev, "(%d) rx process, more : %d, CTS : %d, current size : %d, next size : %d\n", __LINE__, rx_header->more, rx_header->RTSCTS, rx_header->current_data_size, rx_header->next_data_size); */

	total_size = rx_header->current_data_size;

	while (total_size > read_size) {
		int_cmd = 0;

		if (!rx_prev_data_saved) {
			/* check bof : 0x7F */
			memcpy((void *)&bof, (void *)(rx_b + p_read + sizeof(mux)), sizeof(bof));
/*			dev_dbg(&p_ipc_spi->dev, "(%d) rx process, bof : %x\n", __LINE__, bof); */
			if (bof != 0x7F) {
				dev_err(&p_ipc_spi->dev, "(%d) rx process, bof error : %x\n", __LINE__, bof);
/*
				printk(KERN_ERR "[IPC_SPI <= RX :");
				for (i = 0 ; i < DEF_BUF_SIZE + 4 ; i++) {
					printk(KERN_ERR " %02x", *((u8 *)(rx_b + i)));
				}
				printk(KERN_ERR "]\n");
*/
				break;
			}
		}

		if (!rx_prev_data_saved) {
			/* read mux */
			memcpy((void *)&mux, (void *)(rx_b + p_read), sizeof(mux));
/*			dev_dbg(&p_ipc_spi->dev, "(%d) rx process, mux : 0x%04x\n", __LINE__, mux); */
			if (mux > 0x4 || mux < 0x0) {
				dev_err(&p_ipc_spi->dev, "(%d) rx process, mux error : %x\n", __LINE__, mux);
				break;
			}
		} else {
			mux = rx_prev_data_mux;

			dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data, prev_mux : %d\n", __LINE__, mux);
		}

		switch (mux) {
		case 0x0001:
/*			dev_dbg(&p_ipc_spi->dev, "(%d) rx process, got FMT : %x\n", __LINE__, mux);  */

			if (rx_prev_data_saved) {
				if (rx_prev_data_remain > DEF_BUF_SIZE) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data, FMT saved : %d, remain : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_mux = 0x0001;

					memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), DEF_BUF_SIZE);
					rx_prev_data_size += DEF_BUF_SIZE;
					rx_prev_data_remain -= DEF_BUF_SIZE;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data save, FMT size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), rx_prev_data_remain);
				p_read += rx_prev_data_remain;
				read_size += rx_prev_data_remain;

				if (*rx_save_b != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, FMT bof error : %x\n", __LINE__, *rx_save_b);

					rx_prev_data_saved = 0;

					return;
				}

				if (*(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1) != 0x7E) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, FMT eof error : %x\n", __LINE__, *(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1));

					rx_prev_data_saved = 0;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_format_rx((void *)rx_save_b, rx_prev_data_size + rx_prev_data_remain);
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx prev data, cp -> ap FMT memory FULL!!!.\n", __LINE__);
					return;
				} else {
					dev_dbg(&p_ipc_spi->dev, "(%d) rx prev data, <= one packet FMT read Done. size : %d, total_size : %d, read_size : %d, p_read : %d\n", __LINE__,
						rx_prev_data_size + rx_prev_data_remain, total_size, read_size, p_read);
				}

#ifdef FORMAT_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= FMT RX :");
				for (i = 0 ; i < (rx_prev_data_size + rx_prev_data_remain) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_save_b + i)));
				printk(KERN_ERR "]\n");
#endif

				rx_prev_data_saved = 0;
			} else {
				/*  read packet len */
				memcpy((void *)&packet_fmt_len, (void *)(rx_b + p_read + sizeof(mux) + sizeof(bof)), sizeof(packet_fmt_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) <=rx process, FMT pkt len : %d\n", __LINE__, packet_fmt_len);
/*					if (packet_fmt_len > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, FMT pkt len error : %d\n", __LINE__, packet_fmt_len);
					return;
				} */

				if ((packet_fmt_len + sizeof(mux) + sizeof(bof) + sizeof(eof)) > (total_size - read_size)) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data, FMT packet_fmt_len : %d, remain : %d\n", __LINE__, packet_fmt_len, total_size - read_size);

					rx_prev_data_mux = 0x0001;

					memcpy((void *)rx_save_b, (void *)(rx_b + p_read + sizeof(mux)), total_size - read_size - sizeof(mux));
					rx_prev_data_size = total_size - read_size - sizeof(mux);
					rx_prev_data_remain = packet_fmt_len + sizeof(bof) + sizeof(eof) - rx_prev_data_size;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data save, FMT size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_format_rx((void *)(rx_b + p_read + sizeof(mux)), packet_fmt_len + sizeof(bof) + sizeof(eof));
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, cp -> ap FMT memory FULL!!!.\n", __LINE__);
					return;
				} else {
/*						dev_dbg(&p_ipc_spi->dev, "(%d) rx process, one packet FMT read Done.\n", __LINE__); */
				}

#ifdef FORMAT_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= FMT RX :");
				for (i = 0 ; i < (packet_fmt_len + sizeof(bof) + sizeof(eof)) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_b + p_read + sizeof(mux) + i)));
				printk(KERN_ERR "]\n");
#endif

				p_read += packet_fmt_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
				read_size += packet_fmt_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
/*					dev_dbg(&p_ipc_spi->dev, "(%d) rx process, FMT read size : %d\n", __LINE__, read_size); */
			}

			/* make data interrupt cmd */
			int_cmd = MB_DATA(MBD_SEND_FMT);
			ipc_spi_make_data_interrupt(int_cmd, od);
			break;

		case 0x0002:
/*				dev_dbg(&p_ipc_spi->dev, "(%d) rx process, got RAW : %x\n", __LINE__, mux); */

			if (rx_prev_data_saved) {
				if (rx_prev_data_remain > DEF_BUF_SIZE) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data, RAW saved : %d, remain : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_mux = 0x0002;

					memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), DEF_BUF_SIZE);
					rx_prev_data_size += DEF_BUF_SIZE;
					rx_prev_data_remain -= DEF_BUF_SIZE;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data save, RAW size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), rx_prev_data_remain);
				p_read += rx_prev_data_remain;
				read_size += rx_prev_data_remain;

				if (*rx_save_b != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, RAW bof error : %x\n", __LINE__, *rx_save_b);

					rx_prev_data_saved = 0;

					return;
				}

				if (*(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1) != 0x7E) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, RAW eof error : %x\n", __LINE__, *(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1));

					rx_prev_data_saved = 0;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_raw_rx((void *)rx_save_b, rx_prev_data_size + rx_prev_data_remain);
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx prev data, cp -> ap RAW memory FULL!!!.\n", __LINE__);
					return;
				} else {
					dev_dbg(&p_ipc_spi->dev, "(%d) rx prev data, <= one packet RAW read Done. size : %d, total_size : %d, read_size : %d, p_read : %d\n", __LINE__,
						rx_prev_data_size + rx_prev_data_remain, total_size, read_size, p_read);
				}

#ifdef RAW_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= RAW RX :");
				for (i = 0 ; i < (rx_prev_data_size + rx_prev_data_remain) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_save_b + i)));
				printk(KERN_ERR "]\n");
#endif

				rx_prev_data_saved = 0;
			} else {
				/* read packet len */
				memcpy((void *)&packet_len, (void *)(rx_b + p_read + sizeof(mux) + sizeof(bof)), sizeof(packet_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) <=rx process, RAW pkt len : %d\n", __LINE__, packet_len);
/*					if (packet_len > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, RAW pkt len error : %d\n", __LINE__, packet_len);
					return;
				} */

				if ((packet_len + sizeof(mux) + sizeof(bof) + sizeof(eof)) > (total_size - read_size)) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data, RAW packet_len : %d, remain : %d\n", __LINE__, packet_len, total_size - read_size);

					rx_prev_data_mux = 0x0002;

					memcpy((void *)rx_save_b, (void *)(rx_b + p_read + sizeof(mux)), total_size - read_size - sizeof(mux));
					rx_prev_data_size = total_size - read_size - sizeof(mux);
					rx_prev_data_remain = packet_len + sizeof(bof) + sizeof(eof) - rx_prev_data_size;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data save, RAW size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_raw_rx((void *)(rx_b + p_read + sizeof(mux)), packet_len + sizeof(bof) + sizeof(eof));
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, cp -> ap RAW memory FULL!!!.\n", __LINE__);
					return;
				} else {
/*						dev_dbg(&p_ipc_spi->dev, "(%d) rx process, one packet RAW read Done.\n", __LINE__); */
				}

#ifdef RAW_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= RAW RX :");
				for (i = 0 ; i < (packet_len + sizeof(bof) + sizeof(eof)) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_b + p_read + sizeof(mux) + i)));
				printk(KERN_ERR "]\n");
#endif

				p_read += packet_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
				read_size += packet_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
/*					dev_dbg(&p_ipc_spi->dev, "(%d) rx process, RAW read size : %d\n", __LINE__, read_size); */
			}

			/* make data interrupt cmd */
			int_cmd = MB_DATA(MBD_SEND_RAW);
			ipc_spi_make_data_interrupt(int_cmd, od);
			break;

		case 0x0003:
/*				dev_dbg(&p_ipc_spi->dev, "(%d) rx process, got RFS : %x\n", __LINE__, mux); */

			if (rx_prev_data_saved) {
				if (rx_prev_data_remain > DEF_BUF_SIZE) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data, RFS saved : %d, remain : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_mux = 0x0003;

					memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), DEF_BUF_SIZE);
					rx_prev_data_size += DEF_BUF_SIZE;
					rx_prev_data_remain -= DEF_BUF_SIZE;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx 2nd prev data save, RFS size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				memcpy((void *)(rx_save_b + rx_prev_data_size), (void *)(rx_b + 4), rx_prev_data_remain);
				p_read += rx_prev_data_remain;
				read_size += rx_prev_data_remain;

				if (*rx_save_b != 0x7F) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, RFS bof error : %x\n", __LINE__, *rx_save_b);

					rx_prev_data_saved = 0;

					return;
				}

				if (*(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1) != 0x7E) {
					dev_err(&p_ipc_spi->dev, "(%d) <=rx prev, RFS eof error : %x\n", __LINE__, *(u8 *)(rx_save_b + rx_prev_data_size + rx_prev_data_remain - 1));

					rx_prev_data_saved = 0;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_rfs_rx((void *)rx_save_b, rx_prev_data_size + rx_prev_data_remain);
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx prev data, cp -> ap RFS memory FULL!!!.\n", __LINE__);
					return;
				} else {
					dev_dbg(&p_ipc_spi->dev, "(%d) rx prev data, <= one packet RFS read Done. size : %d, total_size : %d, read_size : %d, p_read : %d\n", __LINE__,
						rx_prev_data_size + rx_prev_data_remain, total_size, read_size, p_read);
				}

#ifdef RFS_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= RFS RX :");
				for (i = 0 ; i < (rx_prev_data_size + rx_prev_data_remain) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_save_b + i)));
				printk(KERN_ERR "]\n");
#endif

#ifdef RFS_TX_RX_LENGTH_DUMP
				printk(KERN_ERR "[IPC_SPI <= RFS RX : %d]\n", rx_prev_data_size + rx_prev_data_remain);
#endif

				rx_prev_data_saved = 0;
			} else {
				/* read packet len */
				memcpy((void *)&packet_len, (void *)(rx_b + p_read + sizeof(mux) + sizeof(bof)), sizeof(packet_len));
				dev_dbg(&p_ipc_spi->dev, "(%d) <=rx process, RFS pkt len : %d\n", __LINE__, packet_len);
/*					if (packet_len > DEF_BUF_SIZE) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, RFS pkt len error : %d\n", __LINE__, packet_len);
					return;
				} */

				if ((packet_len + sizeof(mux) + sizeof(bof) + sizeof(eof)) > (total_size - read_size)) {
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data, RFS packet_len : %d, remain : %d\n", __LINE__, packet_len, total_size - read_size);

					rx_prev_data_mux = 0x0003;

					memcpy((void *)rx_save_b, (void *)(rx_b + p_read + sizeof(mux)), total_size - read_size - sizeof(mux));
					rx_prev_data_size = total_size - read_size - sizeof(mux);
					rx_prev_data_remain = packet_len + sizeof(bof) + sizeof(eof) - rx_prev_data_size;
					dev_dbg(&p_ipc_spi->dev, "(%d) <=rx prev data save, RFS size : %d, remain size : %d\n", __LINE__, rx_prev_data_size, rx_prev_data_remain);

					rx_prev_data_saved = 1;

					return;
				}

				/* copy to v_buff */
				retval = ipc_spi_copy_to_vbuff_rfs_rx((void *)(rx_b + p_read + sizeof(mux)), packet_len + sizeof(bof) + sizeof(eof));
				if (retval < 0) {
					dev_err(&p_ipc_spi->dev, "(%d) rx process, cp -> ap RFS memory FULL!!!.\n", __LINE__);
					return;
				} else {
/*						dev_dbg(&p_ipc_spi->dev, "(%d) rx process, one packet RFS read Done.\n", __LINE__); */
				}

#ifdef RFS_RX_DUMP
				printk(KERN_ERR "[IPC_SPI <= RFS RX :");
				for (i = 0 ; i < (packet_len + sizeof(bof) + sizeof(eof)) ; i++)
					printk(KERN_ERR " %02x", *((u8 *)(rx_b + p_read + sizeof(mux) + i)));
				printk(KERN_ERR "]\n");
#endif

				p_read += packet_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
				read_size += packet_len + sizeof(bof) + sizeof(eof) + sizeof(mux);
/*				dev_dbg(&p_ipc_spi->dev, "(%d) rx process, RFS read size : %d\n", __LINE__, read_size); */

#ifdef RFS_TX_RX_LENGTH_DUMP
				printk(KERN_ERR "[IPC_SPI <= RFS RX : %d]\n", packet_len);
#endif

			}

			/* make data interrupt cmd */
			int_cmd = MB_DATA(MBD_SEND_RFS);
			ipc_spi_make_data_interrupt(int_cmd, od);
			break;

		case 0x0004:
			dev_dbg(&p_ipc_spi->dev, "(%d) <=rx process, got command : %x\n", __LINE__, mux);
			break;

		default:
			break;
		}

		if (total_size < read_size)
			dev_err(&p_ipc_spi->dev, "(%d) rx process, read error.\n", __LINE__);

/*		dev_dbg(&p_ipc_spi->dev, "(%d) rx process, LOOP ==== total : %d, read : %d, p_read : %d\n", __LINE__, total_size, read_size, p_read); */
	}
}

static int ipc_spi_thread(void *data)
{
	struct ipc_spi *od = (struct ipc_spi *)data;

	int retval = 0;
#ifdef LOOPBACK_TEST
	u8 *tx_buf = NULL;
	struct spi_protocol_header *tx_header = NULL;
	u8 *rx_buf = NULL;
	struct spi_protocol_header *rx_header = NULL;
	int skip_SRDY_chk = 0;
	int clear_tx_buf = 0;
	u8 *rx_save_buf = NULL;
	u8 *test_data = NULL;
	int tx_loopback_data = 1;
	u8 rx_loopback_data;
	int i, rx_loopback_count = 0;
#endif

#if defined(CONFIG_MACH_Q1_BD)
	u32 val = __raw_readl(S5P_INFORM2);

	if (val == 1) {
		printk(KERN_ERR "[LPM mode] ipc_spi_thread exit!\n");
		goto exit;
	}
#else
	if (lpcharge == 1) {
		printk(KERN_ERR "[LPM mode] ipc_spi_thread exit!\n");
		goto exit;
	}
#endif

	daemonize("ipc_spi_thread");

	printk(KERN_ERR "[%s] ipc_spi_thread start.\n", __func__);
	boot_done = 1;

	wait_for_completion(&ril_init);

	printk(KERN_ERR "[%s] ril_init completed.\n", __func__);

	if (!p_ipc_spi) {
		printk(KERN_ERR "[%s] p_ipc_spi is NULL.\n", __func__);

		retval = -ENODEV;
		goto exit;
	}

	if (!p_virtual_buff) {
		dev_err(&p_ipc_spi->dev, "[%s] p_virtual_buff is NULL.\n", __func__);

		retval = -ENODEV;
		goto exit;
	}

#ifdef LOOPBACK_TEST
	tx_buf = kmalloc(DEF_BUF_SIZE + 4, GFP_ATOMIC);
	if (!tx_buf) {
		dev_err(&p_ipc_spi->dev, "[%s] tx_buf kmalloc fail.", __func__);

		retval = -ENOMEM;
		goto exit;
	}
	tx_header = (struct spi_protocol_header *)tx_buf;

	rx_buf = kmalloc(DEF_BUF_SIZE + 4, GFP_ATOMIC);
	if (!rx_buf) {
		dev_err(&p_ipc_spi->dev, "[%s] rx_buf  kmalloc fail.", __func__);

		retval = -ENOMEM;
		goto exit;
	}
	rx_header = (struct spi_protocol_header *)rx_buf;

	rx_save_buf = kmalloc(DEF_BUF_SIZE * 3, GFP_ATOMIC);
	if (!rx_save_buf) {
		dev_err(&p_ipc_spi->dev, "[%s] rx_save_buf  kmalloc fail.", __func__);

		retval = -ENOMEM;
		goto exit;
	}
	memset((void *)rx_save_buf, 0, DEF_BUF_SIZE * 3);

	test_data = kmalloc(DEF_BUF_SIZE + 4, GFP_ATOMIC);
	if (!test_data) {
		dev_err(&p_ipc_spi->dev, "[%s] test_data  kmalloc fail.", __func__);

		retval = -ENOMEM;
		goto exit;
	}
	memset((void *)test_data, 0, DEF_BUF_SIZE + 4);
#endif

	dev_dbg(&p_ipc_spi->dev, "(%d) wait 2 sec... srdy : %d\n", __LINE__, gpio_get_value(gpio_srdy));
	msleep(2000);

	while (gpio_get_value(gpio_srdy))
		;
	dev_dbg(&p_ipc_spi->dev, "(%d) cp booting... Done.\n", __LINE__);

	dev_dbg(&p_ipc_spi->dev, "(%d) wait 1 sec...\n", __LINE__);
	msleep(1000);

	printk(KERN_ERR "[IPC_SPI] Start IPC Communication. MRDY : %d, SRDY : %d\n", gpio_get_value(gpio_mrdy), gpio_get_value(gpio_srdy));
	sema_init(&srdy_sem, 0);

	ipc_spi_irq_log_flag = 1;

#ifndef LOOPBACK_TEST
	spi_main(1,  od);
#else
	while (1) {

		if (!ipc_spi_check_send_data()) { /* no send data */
			if (down_trylock(&srdy_sem)) { /* no srdy sem */
				dev_dbg(&p_ipc_spi->dev, "(%d) no data and no sem, wait tx-srdy event.\n", __LINE__);

				transfer_thread_waiting = 1;

				skip_SRDY_chk = 0;

				down(&transfer_event_sem); /* wait event(tx or srdy) */
				dev_dbg(&p_ipc_spi->dev, "(%d) got tx-srdy event.\n", __LINE__);
			} else {
				dev_dbg(&p_ipc_spi->dev, "(%d) srdy_sem already exist\n", __LINE__);

				skip_SRDY_chk = 1;
				clear_tx_buf = 1;
			}
		} else {
			dev_dbg(&p_ipc_spi->dev, "(%d) send data exist\n", __LINE__);
		}

		/* HERE : Got tx data event or Got SRDY isr event */
		if (gpio_get_value(gpio_mrdy)) {
			dev_dbg(&p_ipc_spi->dev, "(%d) MRDY HIGH!!!\n", __LINE__);

			ipc_spi_set_MRDY_pin(0);
		}
		ipc_spi_set_MRDY_pin(1);

		do {

			if (clear_tx_buf) {
				memset((void *)tx_buf, 0, DEF_BUF_SIZE + 4);

				clear_tx_buf = 0;
			} else {
				ipc_spi_prepare_tx_data(tx_buf);
			}
#ifdef LOOPBACK_TEST
			if (loop_back_test) {
				if (tx_header->current_data_size == 0x3) {
					/*C2 send*/
				} else {
					memset((void *)test_data, tx_loopback_data, LOOP_SIZE-18);
					ipc_spi_prepare_loopback_tx_data(tx_buf, rx_buf , test_data);
					if (tx_loopback_data == 0x0A) {
						loop_back_test = 1;
						tx_loopback_data = 1;
					} else
						tx_loopback_data++;
				}
			}
			skip_SRDY_chk = 1;	/* AP Only Loopback */
#endif

			if (!skip_SRDY_chk) {
RETRY_WAIT_SEM:

				dev_dbg(&p_ipc_spi->dev, "(%d) wait SRDY : %d\n", __LINE__, gpio_get_value(gpio_srdy));

				if (down_timeout(&srdy_sem, 2 * HZ)) {
					dev_err(&p_ipc_spi->dev, "(%d) SRDY TimeOUT!!! MRDY : %d, SRDY : %d\n", __LINE__, gpio_get_value(gpio_mrdy), gpio_get_value(gpio_srdy));

					ipc_spi_set_MRDY_pin(0);
					mdelay(10);
					ipc_spi_set_MRDY_pin(1);

					dev_err(&p_ipc_spi->dev, "(%d) SRDY TimeOUT Reset!!! MRDY : %d, SRDY : %d\n", __LINE__, gpio_get_value(gpio_mrdy), gpio_get_value(gpio_srdy));

					if (!gpio_get_value(gpio_srdy))
						goto RETRY_WAIT_SEM;
				}
				dev_dbg(&p_ipc_spi->dev, "(%d) got SRDY : %d\n", __LINE__, gpio_get_value(gpio_srdy));
			} else {
				dev_dbg(&p_ipc_spi->dev, "(%d) skip wait SRDY.\n", __LINE__);

				skip_SRDY_chk = 0;
			}

			ipc_spi_swap_data_htn(tx_buf, DEF_BUF_SIZE + 4);    /* host to network */

			retval = ipc_spi_tx_rx_sync(tx_buf, rx_buf, DEF_BUF_SIZE + 4);
			if (retval != 0)
				dev_err(&p_ipc_spi->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
			else
/*				dev_dbg(&p_ipc_spi->dev, "(%d) transmit Done.\n", __LINE__); */

			ipc_spi_swap_data_nth(rx_buf, DEF_BUF_SIZE + 4);    /* network to host */
			*(u32 *)tx_header = ntohl(*(u32 *)tx_header);

#ifdef LOOPBACK_TEST
			printk(KERN_ERR "[SPI DUMP] LOOP RX :]");
			for (i = 0 ; i < 30 ; i++)
				printk(KERN_ERR "%02x ", *((u8 *)(rx_buf + i)));

			printk(KERN_ERR "| ");
			for (i = LOOP_SIZE - 30 ; i < LOOP_SIZE ; i++)
				printk(KERN_ERR "%02x ", *((u8 *)(rx_buf + i)));

			printk(KERN_ERR "]\n");
#else
			dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] RX : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
				rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7], rx_buf[8], rx_buf[9], rx_buf[10], rx_buf[11], rx_buf[12], rx_buf[13], rx_buf[14], rx_buf[15], rx_buf[16], rx_buf[17], rx_buf[18], rx_buf[19],
				rx_buf[20], rx_buf[21], rx_buf[22], rx_buf[23], rx_buf[24], rx_buf[25], rx_buf[26], rx_buf[27], rx_buf[28], rx_buf[29], rx_buf[30], rx_buf[31], rx_buf[32], rx_buf[33], rx_buf[34], rx_buf[35], rx_buf[36], rx_buf[37], rx_buf[38], rx_buf[39]);
#endif

#ifdef LOOPBACK_TEST
			/* Loopback test, IPC header[0A 0C 03] */
			if (rx_buf[14] == 0x0A && rx_buf[15] == 0x0B && rx_buf[16] == 0x01) {
				rx_loopback_data = rx_buf[17];

				for (i = 0; i < LOOP_SIZE - 18; i++) {
					if (rx_buf[17+i] != rx_loopback_data) {
						printk(KERN_ERR "[Loopback] %dth %02x != %02x Invaild data!\n", i, rx_loopback_data, rx_buf[17+i]);
						goto exit;
					}
				}
				if (i == (LOOP_SIZE-18))
					printk(KERN_ERR "[Loopback] %02x is OK!\n", rx_loopback_data);

				rx_loopback_count++;
				if ((rx_loopback_count % 10) == 0 && rx_loopback_data == 0x0A) {
					tx_loopback_data = 1;
					loop_back_test = 1;
					printk(KERN_ERR "[Loopback] rx_loopback_count = %d\n", rx_loopback_count);
				}
				rx_header->RTSCTS = 0;
				rx_header->more = 0;
			}
#else
			if (*(u32 *)rx_header != 0x00000000 && *(u32 *)rx_header != 0xFFFFFFFF) { /* valid spi header */
/*				dev_dbg(&p_ipc_spi->dev, "(%d) got valid rx data.\n", __LINE__); */

				/* RX process */
				ipc_spi_rx_process(rx_buf, rx_save_buf, od);

				if (rx_header->RTSCTS) { /* modem is not available. */
					dev_err(&p_ipc_spi->dev, "(%d) rx CTS set.\n", __LINE__);

					clear_tx_buf = 1;
				}

			} else {
/*				dev_dbg(&p_ipc_spi->dev, "(%d) got invalid rx data.\n", __LINE__); */

				rx_header->RTSCTS = 0;
				rx_header->more = 0;
			}
#endif
			dev_dbg(&p_ipc_spi->dev, "(%d) check more, CTS : %d, rx more : %d, tx more : %d\n", __LINE__, rx_header->RTSCTS, rx_header->more, tx_header->more);
			msleep(2000);
		} while (rx_header->RTSCTS || rx_header->more || tx_header->more);

		ipc_spi_set_MRDY_pin(0);

/*		dev_dbg(&p_ipc_spi->dev, "(%d) LOOP Done ============================\n", __LINE__); */
	}
#endif

exit:
	printk(KERN_ERR "(%d) thread stop.\n", __LINE__);

	return retval;
}

/* Send SPRD main image through SPI */
#if defined(CONFIG_CHN_CMCC_SPI_SPRD)
#define CP_VER_2
/* #define SPRD_TRANSLATE_PACKET */
#define SPRD_BLOCK_SIZE	32768

#ifdef CP_VER_2
enum image_type {
	MODEM_MAIN,
	MODEM_DSP,
	MODEM_NV,
	MODEM_EFS,
	MODEM_RUN,
};
#else
enum image_type {
	MODEM_KERNEL,
	MODEM_USER,
	MODEM_DSP,
	MODEM_NV,
	MODEM_RUN,
};
#endif

struct image_buf {
	unsigned int length;
	unsigned int offset;
	unsigned int address;
	unsigned char *buf;
};

struct sprd_image_buf {
	u8 *tx_b;
	u8 *rx_b;
	u8 *encoded_tx_b;
	u8 *decoded_rx_b;

	int tx_size;
	int rx_size;
	int encoded_tx_size;
	int decoded_rx_size;
};

/* CRC */
#define CRC_16_POLYNOMIAL		0x1021
#define CRC_16_L_POLYNOMIAL	0x8408
#define CRC_16_L_SEED	0xFFFF
#define CRC_TAB_SIZE	256	/* 2^CRC_TAB_BITS	   */
#define CRC_16_L_OK	0x0
#define HDLC_FLAG	0x7E
#define HDLC_ESCAPE	0x7D
#define HDLC_ESCAPE_MASK	0x20
#define CRC_CHECK_SIZE	0x02

#define M_32_SWAP(a) {					\
		u32 _tmp;				\
		_tmp = a;					\
		((u8 *)&a)[0] = ((u8 *)&_tmp)[3]; \
		((u8 *)&a)[1] = ((u8 *)&_tmp)[2]; \
		((u8 *)&a)[2] = ((u8 *)&_tmp)[1]; \
		((u8 *)&a)[3] = ((u8 *)&_tmp)[0]; \
		}

#define M_16_SWAP(a) {					\
		u16 _tmp;					\
		_tmp = (u16)a;			\
		((u8 *)&a)[0] = ((u8 *)&_tmp)[1];	\
		((u8 *)&a)[1] = ((u8 *)&_tmp)[0];	\
		}

unsigned int sprd_crc_calc(char *buf_ptr, unsigned int len)
{
	unsigned int i;
	unsigned short crc = 0;

	while (len-- != 0) {
		for (i = 0x80; i != 0 ; i = i>>1) {
			if ((crc & 0x8000) != 0) {
				crc = crc << 1 ;
				crc = crc ^ 0x1021;
			} else {
				crc = crc << 1 ;
			}

			if ((*buf_ptr & i) != 0)
				crc = crc ^ 0x1021;
		}
		buf_ptr++;
	}

	return crc;

}

unsigned short sprd_crc_calc_fdl(unsigned short *src, int len)
{
	unsigned int sum = 0;
	unsigned short SourceValue, DestValue;
	unsigned short lowSourceValue, hiSourceValue;

	/* Get sum value of the source.*/
	while (len > 1) {
		SourceValue = *src++;
		DestValue	= 0;
		lowSourceValue = (SourceValue & 0xFF00) >> 8;
		hiSourceValue = (SourceValue & 0x00FF) << 8;
		DestValue = lowSourceValue | hiSourceValue;

		sum += DestValue;
		len -= 2;
	}

	if (len == 1)
		sum += *((unsigned char *) src);

	sum = (sum >> 16) + (sum & 0x0FFFF);
	sum += (sum >> 16);

	return ~sum;
}

int encode_msg(struct sprd_image_buf *img, int bcrc)
{
	u16	crc;				/* CRC value*/
	u8	*src_ptr;		   /* source buffer pointer*/
	int	dest_len;		   /* output buffer length*/
	u8	*dest_ptr;		   /* dest buffer pointer*/
	u8	high_crc, low_crc;
	register int	curr;

	/* CRC Check. */
	src_ptr  = img->tx_b;

	/*	CRC Check. */
	if (bcrc)
		crc = sprd_crc_calc(src_ptr, img->tx_size);
	else
		crc  = sprd_crc_calc_fdl((unsigned short *)src_ptr, img->tx_size);

	high_crc = (crc>>8) & 0xFF;
	low_crc  = crc & 0xFF;

	/* Get the total size to be allocated.*/
	dest_len = 0;

	for (curr = 0; curr < img->tx_size; curr++) {
		switch (*(src_ptr+curr)) {
		case HDLC_FLAG:
		case HDLC_ESCAPE:
			dest_len += 2;
			break;
		default:
			dest_len++;
			break;
		}
	}

	switch (low_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		dest_len += 2;
		break;
	default:
		dest_len++;
	}

	switch (high_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		dest_len += 2;
		break;
	default:
		dest_len++;
	}

	dest_ptr = kmalloc(dest_len + 2, GFP_ATOMIC);
	/* Memory Allocate fail.*/
	if (dest_ptr == NULL)
		return 0;

	*dest_ptr = HDLC_FLAG;
	dest_len  = 1;

	/* do escape*/
	for (curr = 0; curr < img->tx_size; curr++) {
		switch (*(src_ptr+curr)) {
		case HDLC_FLAG:
		case HDLC_ESCAPE:
			*(dest_ptr + dest_len++) = HDLC_ESCAPE;
			*(dest_ptr + dest_len++) = *(src_ptr + curr) ^ HDLC_ESCAPE_MASK;
			break;
		default:
			*(dest_ptr + dest_len++) = *(src_ptr + curr);
			break;
		}
	}

	switch (high_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		*(dest_ptr + dest_len++) = HDLC_ESCAPE;
		*(dest_ptr + dest_len++) = high_crc ^ HDLC_ESCAPE_MASK;
		break;
	default:
		*(dest_ptr + dest_len++) = high_crc;
	}

	switch (low_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		*(dest_ptr + dest_len++) = HDLC_ESCAPE;
		*(dest_ptr + dest_len++) = low_crc ^ HDLC_ESCAPE_MASK;
		break;
	default:
		*(dest_ptr + dest_len++) = low_crc;
	}


	*(dest_ptr + dest_len++) = HDLC_FLAG;

	memcpy(img->encoded_tx_b, dest_ptr, dest_len);
	img->encoded_tx_size = dest_len;
/*
	LOGD("[In encode_msg dest_ptr]");
	for (i=0;i<dest_len;i++)
		LOGD("0x%X", *(dest_ptr+i));

	LOGD("[In encode_msg output_buf]");
	for (i=0;i<img->encoded_tx_size;i++)
		LOGD("0x%X", *(img->encoded_tx_b+i));
*/
	kfree(dest_ptr);
	return 1;
}

int decode_msg(struct sprd_image_buf *img, int bcrc)
{
	u16	crc;	/* CRC value*/
	u8	*src_ptr;	/* source buffer pointer*/
	int	dest_len;	/* output buffer length*/
	u8	*dest_ptr;	/* dest buffer pointer*/
	register int		curr;

	/* Check if exist End Flag.*/
	src_ptr = img->rx_b;

	dest_len = 0;

	if (img->rx_size < 4)
		return -1;

	/* Get the total size to be allocated for decoded message.*/
	for (curr = 1; curr < img->rx_size - 1; curr++) {
		switch (*(src_ptr + curr)) {
		case HDLC_ESCAPE:
			curr++;
			dest_len++;
			break;
		default:
			dest_len++;
			break;
		}
	}

	/* Allocate meomory for decoded message*/
	dest_ptr = NULL;
	dest_ptr = kmalloc(dest_len, GFP_ATOMIC);
	/* Memory Free fail.*/
	if (dest_ptr == NULL)
		return -2;

	memset(dest_ptr, 0, dest_len);

	curr = 0;
	dest_len = 0;
	/* Do de-escape.*/
	for (curr = 1; curr < img->rx_size - 1; curr++) {
		switch (*(src_ptr + curr)) {
		case HDLC_ESCAPE:
			curr++;
			*(dest_ptr + dest_len) = *(src_ptr + curr) ^ HDLC_ESCAPE_MASK;
			break;
		default:
			*(dest_ptr + dest_len) = *(src_ptr + curr);
			break;
		}

		dest_len = dest_len + 1;
	}

	/*	CRC Check. */
	if (bcrc)
		crc = sprd_crc_calc(dest_ptr, dest_len);
	else
		crc  = sprd_crc_calc_fdl((unsigned short *)dest_ptr, dest_len);

	if (crc != CRC_16_L_OK) {
		dev_err(&p_ipc_spi->dev, "CRC error : 0x%X", crc);
		kfree(dest_ptr);
		return -3;
	}

	memcpy(img->decoded_rx_b, dest_ptr, dest_len - CRC_CHECK_SIZE);
	img->decoded_rx_size = dest_len - CRC_CHECK_SIZE ;
/*
	LOGD("[In decode_msg dest_ptr]");
	for (i=0;i<dest_len;i++)
		LOGD("0x%X", *(dest_ptr+i));

	LOGD("[In decode_msg output_buf]");
	for (i=0;i<*img->decoded_rx_size;i++)
		LOGD("0x%X", *(img->decoded_rx_b+i));
*/
	kfree(dest_ptr);
	return 1;
}

static int ipc_spi_send_modem_bin_execute_cmd(struct ipc_spi *od, u8 *spi_ptr, u32 spi_size, u16 spi_type, u16 spi_crc, struct sprd_image_buf *sprd_img)
{
	int retval;
	u16 send_packet_size;
	u8 *send_packet_data;
	u16 d1_crc;
	u16 d2_crc = spi_crc;
	u16 type = spi_type;
#ifdef SPRD_TRANSLATE_PACKET
	int i, cnt_7E = 0;
	int bcrc = 0;
#endif

	/* D1 */
	send_packet_size = spi_size;	/* u32 -> u16 */
#ifdef SPRD_TRANSLATE_PACKET
	sprd_img->tx_size = 6;
	memcpy(sprd_img->tx_b, &send_packet_size, sizeof(send_packet_size));
	*(sprd_img->tx_b+2) = 0x00; *(sprd_img->tx_b+3) = 0x00;	/* reserved 4 bytes */
	*(sprd_img->tx_b+4) = 0x00; *(sprd_img->tx_b+5) = 0x00;

	d1_crc = sprd_crc_calc_fdl((unsigned short *)sprd_img->tx_b, sprd_img->tx_size);
	M_16_SWAP(d1_crc);
	memcpy((sprd_img->tx_b+6), &d1_crc, sizeof(d1_crc));
	sprd_img->tx_size += 2;
#else
	sprd_img->tx_size = 6;
	M_16_SWAP(d2_crc);
	memcpy(sprd_img->tx_b, &send_packet_size, sizeof(send_packet_size));
	memcpy((sprd_img->tx_b+2), &type, sizeof(type));
	memcpy((sprd_img->tx_b+4), &d2_crc, sizeof(d2_crc));

	d1_crc = sprd_crc_calc_fdl((unsigned short *)sprd_img->tx_b, sprd_img->tx_size);
	M_16_SWAP(d1_crc);
	memcpy((sprd_img->tx_b+6), &d1_crc, sizeof(d1_crc));
	sprd_img->tx_size += 2;
#endif
/*
	dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] TX_D1(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x]\n", sprd_img->tx_size,
		sprd_img->tx_b[0], sprd_img->tx_b[1], sprd_img->tx_b[2], sprd_img->tx_b[3], sprd_img->tx_b[4],
		sprd_img->tx_b[5], sprd_img->tx_b[6], sprd_img->tx_b[7]);
*/

	if (down_timeout(&srdy_sem, 2 * HZ)) {
		dev_err(&p_ipc_spi->dev, "(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n", __LINE__, gpio_get_value(gpio_srdy), srdy_sem.count);
		return -1;
	}

	retval = ipc_spi_tx_rx_sync(sprd_img->tx_b, sprd_img->rx_b, sprd_img->tx_size);
	if (retval != 0) {
		dev_err(&p_ipc_spi->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
		return -1;
	} else {
/*		dev_dbg(&p_ipc_spi->dev, "(%d) transmit Done.\n", __LINE__); */
	}
/*
	dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] TX_D2(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		send_packet_size, spi_ptr[0], spi_ptr[1], spi_ptr[2], spi_ptr[3], spi_ptr[4],
		spi_ptr[5], spi_ptr[6], spi_ptr[7], spi_ptr[8], spi_ptr[9],
		spi_ptr[10], spi_ptr[11], spi_ptr[12], spi_ptr[13], spi_ptr[14],
		spi_ptr[15], spi_ptr[16], spi_ptr[17], spi_ptr[18], spi_ptr[19],
		spi_ptr[send_packet_size - 10], spi_ptr[send_packet_size - 9], spi_ptr[send_packet_size - 8], spi_ptr[send_packet_size - 7], spi_ptr[send_packet_size - 6],
		spi_ptr[send_packet_size - 5], spi_ptr[send_packet_size - 4], spi_ptr[send_packet_size - 3], spi_ptr[send_packet_size - 2], spi_ptr[send_packet_size - 1]);
*/
	if ((type == 0x0003) || (type == 0x0004)) {
		printk(KERN_ERR "D2 Skip!!\n");
		goto ACK;
	}

	/* D2 */
	send_packet_data = spi_ptr;

	if (down_timeout(&srdy_sem, 2 * HZ)) {
		dev_err(&p_ipc_spi->dev, "(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n", __LINE__, gpio_get_value(gpio_srdy), srdy_sem.count);
		return -1;
	}

	retval = ipc_spi_tx_rx_sync(send_packet_data, sprd_img->rx_b, send_packet_size);
	if (retval != 0) {
		dev_err(&p_ipc_spi->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
		return -1;
	} else {
/*		dev_dbg(&p_ipc_spi->dev, "(%d) transmit Done.\n", __LINE__); */
	}

ACK:

	if (is_cp_reset) {
		while (!gpio_get_value(gpio_srdy))
			;
	} else {
	if (down_timeout(&srdy_sem, 2 * HZ)) {
		dev_err(&p_ipc_spi->dev, "(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n", __LINE__,  gpio_get_value(gpio_srdy), srdy_sem.count);
		dev_err(&p_ipc_spi->dev, "[SPI DUMP] TX_D1(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x]\n", sprd_img->tx_size,
			sprd_img->tx_b[0], sprd_img->tx_b[1], sprd_img->tx_b[2], sprd_img->tx_b[3], sprd_img->tx_b[4],
			sprd_img->tx_b[5], sprd_img->tx_b[6], sprd_img->tx_b[7]);
		dev_err(&p_ipc_spi->dev, "[SPI DUMP] TX_D2(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
			send_packet_size, spi_ptr[0], spi_ptr[1], spi_ptr[2], spi_ptr[3], spi_ptr[4],
			spi_ptr[5], spi_ptr[6], spi_ptr[7]);

		/* WA (CP Reset) jongmoon.suh */
		if (gpio_get_value(gpio_srdy))
			;
		else
			return -1;

	}
	}

	memset(sprd_img->tx_b, 0, SPRD_BLOCK_SIZE+10);
	retval = ipc_spi_tx_rx_sync(sprd_img->tx_b, sprd_img->rx_b, 8);
	if (retval != 0) {
		dev_err(&p_ipc_spi->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
		return -1;
	} else {
/*		dev_dbg(&p_ipc_spi->dev, "(%d) transmit Done.\n", __LINE__); */
	}
#ifdef SPRD_TRANSLATE_PACKET
	for (i = 0; i < 8; i++) {
		if (*(sprd_img->rx_b+i) == 0x7E)
			cnt_7E++;
		if (cnt_7E == 2)
			break;
	}
	sprd_img->rx_size = i + 1;
/*
	dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] RX(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n", sprd_img->rx_size,
		sprd_img->rx_b[0], sprd_img->rx_b[1], sprd_img->rx_b[2], sprd_img->rx_b[3], sprd_img->rx_b[4], sprd_img->rx_b[5], sprd_img->rx_b[6], sprd_img->rx_b[7],
		sprd_img->rx_b[8], sprd_img->rx_b[9], sprd_img->rx_b[10], sprd_img->rx_b[11], sprd_img->rx_b[12], sprd_img->rx_b[13], sprd_img->rx_b[14], sprd_img->rx_b[15]);
*/
	if (sprd_img->rx_size != 8) {
		dev_err(&p_ipc_spi->dev, "ACK size error!, %d\n", sprd_img->rx_size);
		dev_err(&p_ipc_spi->dev, "[SPI DUMP] RX(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n", sprd_img->rx_size,
		sprd_img->rx_b[0], sprd_img->rx_b[1], sprd_img->rx_b[2], sprd_img->rx_b[3], sprd_img->rx_b[4], sprd_img->rx_b[5], sprd_img->rx_b[6], sprd_img->rx_b[7],
		sprd_img->rx_b[8], sprd_img->rx_b[9], sprd_img->rx_b[10], sprd_img->rx_b[11], sprd_img->rx_b[12], sprd_img->rx_b[13], sprd_img->rx_b[14], sprd_img->rx_b[15]);

		return -1;
	}
	retval = decode_msg(sprd_img, bcrc);
	if (retval != 1) {
		dev_err(&p_ipc_spi->dev, "decode_msg(ACK) error!\n");
		return -1;
	}
#else
	memcpy(sprd_img->decoded_rx_b, sprd_img->rx_b, 4);
#endif
	if ((*(sprd_img->decoded_rx_b+0) == 0x00) && (*(sprd_img->decoded_rx_b+1) == 0x80) &&
		(*(sprd_img->decoded_rx_b+2) == 0x00) && (*(sprd_img->decoded_rx_b+3) == 0x00)) {
		/* dev_dbg(&p_ipc_spi->dev, "[SPRD] CP sent ACK"); */
	} else {
		dev_err(&p_ipc_spi->dev, "Transfer ACK error!srdy_sem = %d\n", srdy_sem.count);
		dev_err(&p_ipc_spi->dev, "[SPI DUMP] RX : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		sprd_img->rx_b[0], sprd_img->rx_b[1], sprd_img->rx_b[2], sprd_img->rx_b[3], sprd_img->rx_b[4], sprd_img->rx_b[5], sprd_img->rx_b[6], sprd_img->rx_b[7],
		sprd_img->rx_b[8], sprd_img->rx_b[9], sprd_img->rx_b[10], sprd_img->rx_b[11], sprd_img->rx_b[12], sprd_img->rx_b[13], sprd_img->rx_b[14], sprd_img->rx_b[15]);

		return -1;
	}

	return retval;

}

static int ipc_spi_send_modem_bin_xmit_img(struct ipc_spi *od, enum image_type type, struct image_buf *img)
{
	int retval = 0;
	struct sprd_image_buf sprd_img;
	unsigned int data_size;
	unsigned int send_size = 0;
	unsigned int rest_size = 0;
	unsigned int spi_size = 0;
	unsigned int address;
	unsigned int fdl1_size;
	/* No Translate */
	u16 crc = 0;
	u16 spi_type = 0;

	unsigned char *spi_ptr;
	unsigned char *ptr;
	int i, j;
#ifdef SPRD_TRANSLATE_PACKET
	int bcrc = 0;
#endif
	u16 sprd_packet_size = SPRD_BLOCK_SIZE;

	sprd_img.tx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.tx_b) {
		dev_err(&p_ipc_spi->dev, "(%d) tx_b kmalloc fail.", __LINE__);
		return -1;
	}
	memset(sprd_img.tx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.rx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.rx_b) {
		dev_err(&p_ipc_spi->dev, "(%d) rx_b kmalloc fail.", __LINE__);
		return -1;
	}
	memset(sprd_img.rx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.encoded_tx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.encoded_tx_b) {
		dev_err(&p_ipc_spi->dev, "(%d) encoded_tx_b kmalloc fail.", __LINE__);
		return -1;
	}
	memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.decoded_rx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.decoded_rx_b) {
		dev_err(&p_ipc_spi->dev, "(%d) encoded_rx_b kmalloc fail.", __LINE__);
		return -1;
	}
	memset(sprd_img.decoded_rx_b, 0, SPRD_BLOCK_SIZE*2);

	dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img type : %d.\n", __LINE__, type);
	memcpy(&fdl1_size, (void *)(p_virtual_buff + 4), 4);

	switch (type) {
#ifdef CP_VER_2
	case MODEM_MAIN:
		memcpy(&img->address, (void *)(p_virtual_buff + 8), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 12), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + 0x30 + fdl1_size);
		img->offset = img->length + fdl1_size + 0x30;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save MAIN to img.\n", __LINE__);

		break;

	case MODEM_DSP:
		memcpy(&img->address, (void *)(p_virtual_buff + 16), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 20), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save DSP to img.\n", __LINE__);

		break;

	case MODEM_NV:
		memcpy(&img->address, (void *)(p_virtual_buff + 24), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 28), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save NV to img.\n", __LINE__);

		break;

	case MODEM_EFS:
		memcpy(&img->address, (void *)(p_virtual_buff + 32), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 36), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save EFS to img.\n", __LINE__);

		break;
#else
	case MODEM_KERNEL:
		memcpy(&img->address, (void *)(p_virtual_buff + 8), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 12), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + 0x30 + fdl1_size);
		img->offset = img->length + fdl1_size + 0x30;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save KERNEL to img.\n", __LINE__);

		break;

	case MODEM_USER:
		memcpy(&img->address, (void *)(p_virtual_buff + 16), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 20), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save USER to img.\n", __LINE__);

		break;

	case MODEM_DSP:
		memcpy(&img->address, (void *)(p_virtual_buff + 24), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 28), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save DSP to img.\n", __LINE__);

		break;

	case MODEM_NV:
		memcpy(&img->address, (void *)(p_virtual_buff + 32), 4);
		memcpy(&img->length , (void *)(p_virtual_buff + 36), 4);
		img->buf  = (unsigned char *)(p_virtual_buff + img->offset);
		img->offset += img->length;
		dev_dbg(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save NV to img.\n", __LINE__);

		break;
#endif
	case MODEM_RUN:
#ifdef SPRD_TRANSLATE_PACKET
		sprd_img.tx_size = 4;
		*(sprd_img.tx_b+0) = 0x00; *(sprd_img.tx_b+1) = 0x04;
		*(sprd_img.tx_b+2) = 0x00; *(sprd_img.tx_b+3) = 0x00;
		retval = encode_msg(&sprd_img, bcrc);
		if (retval != 1) {
			dev_err(&p_ipc_spi->dev, "encode_msg(Transfer Start) error!");
			return -1;
		}
#else
		memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE*2);
		sprd_img.encoded_tx_size = 0;
		spi_type = 0x0004;
		crc = 0;
#endif
		spi_ptr = sprd_img.encoded_tx_b;
		spi_size = sprd_img.encoded_tx_size;

		retval = ipc_spi_send_modem_bin_execute_cmd(od, spi_ptr, spi_size, spi_type, crc, &sprd_img);
		if (retval < 0) {
			dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
			return -1;
		}
		return retval;

	default:
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img wrong : %d.", __LINE__, type);
		return -1;
	}

	dev_dbg(&p_ipc_spi->dev, "(%d) Start send img. size : %d\n", __LINE__, img->length);

	ptr = img->buf;
	data_size = sprd_packet_size;
	rest_size = img->length;
	address = img->address;

	M_32_SWAP(img->address);
	M_32_SWAP(img->length);

	/* Send Transfer Start */
#ifdef SPRD_TRANSLATE_PACKET
	sprd_img.tx_size = 12;
	*(sprd_img.tx_b+0) = 0x00; *(sprd_img.tx_b+1) = 0x01;
	*(sprd_img.tx_b+2) = 0x00; *(sprd_img.tx_b+3) = 0x08;
	memcpy((sprd_img.tx_b+4), &img->address, sizeof(img->address));
	memcpy((sprd_img.tx_b+8), &img->length, sizeof(img->length));

	retval = encode_msg(&sprd_img, bcrc);
	if (retval != 1) {
		dev_err(&p_ipc_spi->dev, "encode_msg(Transfer Start) error!");
		return -1;
	}
#else
	sprd_img.tx_size = 8;
	memcpy((sprd_img.tx_b+0), &img->address, sizeof(img->address));
	memcpy((sprd_img.tx_b+4), &img->length, sizeof(img->length));

	spi_type = 0x0001;
	crc = sprd_crc_calc_fdl((unsigned short *)sprd_img.tx_b, sprd_img.tx_size);
	memcpy(sprd_img.encoded_tx_b, sprd_img.tx_b, sprd_img.tx_size);
	sprd_img.encoded_tx_size = sprd_img.tx_size;
#endif

	spi_ptr = sprd_img.encoded_tx_b;
	spi_size = sprd_img.encoded_tx_size;

	dev_dbg(&p_ipc_spi->dev, "(%d) [Transfer Start, Type = %d, Packet = %d]\n", __LINE__, type, sprd_packet_size);
	retval = ipc_spi_send_modem_bin_execute_cmd(od, spi_ptr, spi_size, spi_type, crc, &sprd_img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		return -1;
	}
	M_32_SWAP(img->length);

	/* Send Data */
	for (i = 0; send_size < img->length; i++) {
		if (rest_size < sprd_packet_size)
			data_size = rest_size;
#ifdef SPRD_TRANSLATE_PACKET
		sprd_img.tx_size = sprd_packet_size + 4;
		*(sprd_img.tx_b+0) = 0x00; *(sprd_img.tx_b+1) = 0x02;	/* type */
		M_16_SWAP(sprd_packet_size);
		memcpy((sprd_img.tx_b + 2), &sprd_packet_size, sizeof(sprd_packet_size));
		M_16_SWAP(sprd_packet_size);

		for (j = 0; j < data_size; j++)
			*(sprd_img.tx_b+4+j) = *(ptr + j);

		retval = encode_msg(&sprd_img, bcrc);
		if (retval != 1) {
			dev_err(&p_ipc_spi->dev, "encode_msg(TransferData_%d) error!", i);
			return -1;
		}
#else
		sprd_img.encoded_tx_size = sprd_packet_size;
		for (j = 0; j < data_size; j++)
			*(sprd_img.encoded_tx_b+j) = *(ptr + j);

		spi_type = 0x0002;
		crc = sprd_crc_calc_fdl((unsigned short *)sprd_img.encoded_tx_b, sprd_img.encoded_tx_size);
#endif
		spi_ptr = sprd_img.encoded_tx_b;
		spi_size = sprd_img.encoded_tx_size;

		retval = ipc_spi_send_modem_bin_execute_cmd(od, spi_ptr, spi_size, spi_type, crc, &sprd_img);
		if (retval < 0) {
			dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d, %d", __LINE__, retval, i);
			return -1;
		}

		send_size += data_size;
		rest_size -= data_size;
		ptr += data_size;

		if (!(i % 100))
			dev_dbg(&p_ipc_spi->dev, "(%d) [%d] 0x%x size done, rest size: 0x%x\n", __LINE__, i, send_size, rest_size);
	}

	/* Send Transfer End */
#ifdef SPRD_TRANSLATE_PACKET
	sprd_img.tx_size = 4;
	*(sprd_img.tx_b+0) = 0x00; *(sprd_img.tx_b+1) = 0x03;
	*(sprd_img.tx_b+2) = 0x00; *(sprd_img.tx_b+3) = 0x00;
	retval = encode_msg(&sprd_img, bcrc);
	if (retval != 1) {
		dev_err(&p_ipc_spi->dev, "encode_msg(TransferEnd) error!");
		return -1;
	}
#else
	memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE * 2);
	sprd_img.encoded_tx_size = 0;

	spi_type = 0x0003;
	crc = 0;
#endif
	spi_ptr = sprd_img.encoded_tx_b;
	spi_size = sprd_img.encoded_tx_size;

	dev_dbg(&p_ipc_spi->dev, "(%d) [Transfer END]\n", __LINE__);
	retval = ipc_spi_send_modem_bin_execute_cmd(od, spi_ptr, spi_size, spi_type, crc, &sprd_img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		return -1;
	}

	kfree(sprd_img.tx_b);
	kfree(sprd_img.rx_b);
	kfree(sprd_img.encoded_tx_b);
	kfree(sprd_img.decoded_rx_b);

	return retval;

}

static void ipc_spi_send_modem_bin(struct work_struct *send_modem_w)
{
	int retval = 0;
	u32 int_cmd = 0xABCDABCD;
	u32 int_cmd_fail = 0xDCBADCBA;
	struct image_buf img;
	unsigned long tick1, tick2;

	struct ipc_spi_send_modem_bin_workq_data *smw
		= container_of(send_modem_w, struct ipc_spi_send_modem_bin_workq_data, send_modem_w);
	struct ipc_spi *od = smw->od;

	tick1 = jiffies_to_msecs(jiffies);

	dev_dbg(&p_ipc_spi->dev, "[SPI DUMP] p_virtual_buff : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		*(u8 *)(p_virtual_buff), *(u8 *)(p_virtual_buff + 1), *(u8 *)(p_virtual_buff + 2), *(u8 *)(p_virtual_buff + 3), *(u8 *)(p_virtual_buff + 4),
		 *(u8 *)(p_virtual_buff + 5),  *(u8 *)(p_virtual_buff + 6),  *(u8 *)(p_virtual_buff + 7),  *(u8 *)(p_virtual_buff + 8),  *(u8 *)(p_virtual_buff + 9),
		*(u8 *)(p_virtual_buff + 10),  *(u8 *)(p_virtual_buff + 11),  *(u8 *)(p_virtual_buff + 12),  *(u8 *)(p_virtual_buff + 13),	*(u8 *)(p_virtual_buff + 14),
		*(u8 *)(p_virtual_buff + 15),  *(u8 *)(p_virtual_buff + 16),  *(u8 *)(p_virtual_buff + 17),  *(u8 *)(p_virtual_buff + 18),	*(u8 *)(p_virtual_buff + 19));
#ifdef CP_VER_2
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_MAIN, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
#else
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_KERNEL, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}

	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_USER, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
#endif
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_DSP, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}

	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_NV, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
#ifdef CP_VER_2
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_EFS, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
#endif
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_RUN, &img);
	if (retval < 0) {
		dev_err(&p_ipc_spi->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}

	send_modem_spi = 0;
	is_cp_reset = 0;
	tick2 = jiffies_to_msecs(jiffies);
	dev_dbg(&p_ipc_spi->dev, "Downloading takes %lu msec\n", (tick2-tick1));

	/* make data interrupt cmd */
	ipc_spi_make_data_interrupt(int_cmd, od);

	return;

err:
	/* make data interrupt cmd */
	ipc_spi_make_data_interrupt(int_cmd_fail, od);

}

#else /*EUR */
struct ipc_spi_send_modem_bin_header {
	u16 sot;
	u16 type;
	u16 length;
};

struct ipc_spi_send_modem_bin_footer {
	u16 crc;
	u16 eot;
};


static u16 ipc_spi_send_modem_bin_make_crc(u8 *buf)
{
	u16 crc = 0;
	int i;
	struct ipc_spi_send_modem_bin_header *header = (struct ipc_spi_send_modem_bin_header *)(buf + 4);

	crc += header->type;
	crc += header->length;

	buf += 4;
	buf += sizeof(struct ipc_spi_send_modem_bin_header);
	for (i = 0 ; i < header->length ; i++)
		crc += *buf++;

	return crc;
}

#define EBL_PACKET_SIZE	4096
static int ipc_spi_send_modem_bin_execute_cmd(struct ipc_spi *od, u16 type, u32 len, void *data)
{
	int retval = 0;
	u8 *tx_b = NULL;
	u8 *rx_b = NULL;
	struct spi_protocol_header *tx_spi_header = NULL;
	struct spi_protocol_header *rx_spi_header = NULL;
	struct ipc_spi_send_modem_bin_header *tx_header = NULL;
	struct ipc_spi_send_modem_bin_footer *tx_footer = NULL;
	struct ipc_spi_send_modem_bin_header *rx_header = NULL;
	struct ipc_spi_send_modem_bin_footer *rx_footer = NULL;

	tx_b = kmalloc(EBL_PACKET_SIZE, GFP_ATOMIC);
	if (!tx_b) {
		dev_err(od->dev, "(%d) tx_b kmalloc fail.", __LINE__);
		return -ENOMEM;
	}
	tx_spi_header = (struct spi_protocol_header *)tx_b;
	tx_header = (struct ipc_spi_send_modem_bin_header *)(tx_b + 4);
	memset(tx_b, 0, EBL_PACKET_SIZE);

	rx_b = kmalloc(EBL_PACKET_SIZE, GFP_ATOMIC);
	if (!rx_b) {
		dev_err(od->dev, "(%d) rx_b kmalloc fail.", __LINE__);
		return -ENOMEM;
	}
	rx_spi_header = (struct spi_protocol_header *)rx_b;
	rx_header = (struct ipc_spi_send_modem_bin_header *)(rx_b + 4);
	memset(rx_b, 0, EBL_PACKET_SIZE);

	ipc_spi_set_MRDY_pin(1);

	tx_spi_header->next_data_size = (EBL_PACKET_SIZE - 4) >> 2;
	tx_spi_header->current_data_size = sizeof(struct ipc_spi_send_modem_bin_header) + len + sizeof(struct ipc_spi_send_modem_bin_footer);

	tx_header->sot = 0x0002;
	tx_header->type = type;
	tx_header->length = len;

	memcpy((void *)(tx_b + sizeof(struct ipc_spi_send_modem_bin_header) + 4), data, len);

	tx_footer = (struct ipc_spi_send_modem_bin_footer *)(tx_b + 4 + sizeof(struct ipc_spi_send_modem_bin_header) + len);

	tx_footer->crc = ipc_spi_send_modem_bin_make_crc(tx_b);

	tx_footer->eot = 0x0003;
/*
	dev_dbg(od->dev, "[SPI DUMP] tx : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		tx_b[0], tx_b[1], tx_b[2], tx_b[3], tx_b[4],
		tx_b[5], tx_b[6], tx_b[7], tx_b[8], tx_b[9],
		tx_b[10], tx_b[11], tx_b[12], tx_b[13], tx_b[14],
		tx_b[15], tx_b[16], tx_b[17], tx_b[18], tx_b[19],
		tx_b[EBL_PACKET_SIZE - 10], tx_b[EBL_PACKET_SIZE - 9], tx_b[EBL_PACKET_SIZE - 8], tx_b[EBL_PACKET_SIZE - 7], tx_b[EBL_PACKET_SIZE - 6],
		tx_b[EBL_PACKET_SIZE - 5], tx_b[EBL_PACKET_SIZE - 4], tx_b[EBL_PACKET_SIZE - 3], tx_b[EBL_PACKET_SIZE - 2], tx_b[EBL_PACKET_SIZE - 1]);
*/
	ipc_spi_swap_data_htn(tx_b, EBL_PACKET_SIZE);

	down(&srdy_sem);

	retval = ipc_spi_tx_rx_sync(tx_b, rx_b, EBL_PACKET_SIZE);
	if (retval != 0)
		dev_err(od->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
	else
		;/*dev_dbg(od->dev, "(%d) transmit Done.\n", __LINE__); */

/*
	dev_dbg(od->dev, "[SPI DUMP] rx : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		rx_b[0], rx_b[1], rx_b[2], rx_b[3], rx_b[4],
		rx_b[5], rx_b[6], rx_b[7], rx_b[8], rx_b[9],
		rx_b[10], rx_b[11], rx_b[12], rx_b[13], rx_b[14],
		rx_b[15], rx_b[16], rx_b[17], rx_b[18], rx_b[19],
		rx_b[EBL_PACKET_SIZE - 10], rx_b[EBL_PACKET_SIZE - 9], rx_b[EBL_PACKET_SIZE - 8], rx_b[EBL_PACKET_SIZE - 7], rx_b[EBL_PACKET_SIZE - 6],
		rx_b[EBL_PACKET_SIZE - 5], rx_b[EBL_PACKET_SIZE - 4], rx_b[EBL_PACKET_SIZE - 3], rx_b[EBL_PACKET_SIZE - 2], rx_b[EBL_PACKET_SIZE - 1]);
*/

	if (type == 0x0208) /* ReqForceHwReset */
		return 0;

	memset(tx_b, 0, EBL_PACKET_SIZE);
	memset(rx_b, 0, EBL_PACKET_SIZE);

	down(&srdy_sem);

	retval = ipc_spi_tx_rx_sync(tx_b, rx_b, EBL_PACKET_SIZE);
	if (retval != 0)
		dev_err(od->dev, "(%d) spi sync error : %d\n", __LINE__, retval);
	else
		;/*dev_dbg(od->dev, "(%d) transmit Done.\n", __LINE__); */

	ipc_spi_swap_data_nth(rx_b, EBL_PACKET_SIZE);
/*
	dev_dbg(od->dev, "[SPI DUMP] rx : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		rx_b[0], rx_b[1], rx_b[2], rx_b[3], rx_b[4],
		rx_b[5], rx_b[6], rx_b[7], rx_b[8], rx_b[9],
		rx_b[10], rx_b[11], rx_b[12], rx_b[13], rx_b[14],
		rx_b[15], rx_b[16], rx_b[17], rx_b[18], rx_b[19],
		rx_b[EBL_PACKET_SIZE - 10], rx_b[EBL_PACKET_SIZE - 9], rx_b[EBL_PACKET_SIZE - 8], rx_b[EBL_PACKET_SIZE - 7], rx_b[EBL_PACKET_SIZE - 6],
		rx_b[EBL_PACKET_SIZE - 5], rx_b[EBL_PACKET_SIZE - 4], rx_b[EBL_PACKET_SIZE - 3], rx_b[EBL_PACKET_SIZE - 2], rx_b[EBL_PACKET_SIZE - 1]);
*/
	if (rx_header->type != type) {
		dev_err(od->dev, "(%d) execute cmd ack error : 0x%x(0x%x)\n", __LINE__, rx_header->type, type);

		dev_dbg(od->dev, "[SPI DUMP] rx : [%02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
			rx_b[0], rx_b[1], rx_b[2], rx_b[3], rx_b[4],
			rx_b[5], rx_b[6], rx_b[7], rx_b[8], rx_b[9],
			rx_b[10], rx_b[11], rx_b[12], rx_b[13], rx_b[14],
			rx_b[15], rx_b[16], rx_b[17], rx_b[18], rx_b[19],
			rx_b[EBL_PACKET_SIZE - 10], rx_b[EBL_PACKET_SIZE - 9], rx_b[EBL_PACKET_SIZE - 8], rx_b[EBL_PACKET_SIZE - 7], rx_b[EBL_PACKET_SIZE - 6],
			rx_b[EBL_PACKET_SIZE - 5], rx_b[EBL_PACKET_SIZE - 4], rx_b[EBL_PACKET_SIZE - 3], rx_b[EBL_PACKET_SIZE - 2], rx_b[EBL_PACKET_SIZE - 1]);

		retval = -1;
	} else {
/*		dev_dbg(od->dev, "(%d) execute cmd ack Done.\n", __LINE__); */
	}

	ipc_spi_set_MRDY_pin(0);

	return retval;
}

enum image_type {
	MODEM_PSI,
	MODEM_EBL,
	MODEM_MAIN,
	MODEM_NV,
};

struct image_buf {
	unsigned int length;
	unsigned char *buf;
};

#define PSI_OFFSET			0
#define EBL_OFFSET		0x10000
#define MAIN_OFFSET		0x28000
#define NV_OFFSET			0xA00000

#define PSI_LEN				(EBL_OFFSET - PSI_OFFSET - 1)
#define EBL_LEN			(MAIN_OFFSET - EBL_OFFSET)
#define MAIN_LEN			(NV_OFFSET - MAIN_OFFSET)
#define NV_LEN				(2 * 1024 * 1024)

#define MAIN_OFFSET_VM	0
#define NV_OFFSET_VM		0xD80000

#define SPI_SEND_BLOCK_SIZE		4080

static int ipc_spi_send_modem_bin_xmit_img(struct ipc_spi *od, enum image_type type, unsigned int *address)
{
	int retval = 0;
	struct image_buf img;
	unsigned int data_size;
	unsigned int send_size = 0;
	unsigned int rest_size = 0;
	unsigned char *ptr;
	int i;

	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img type : %d.\n", __LINE__, type);
	switch (type) {
	case MODEM_MAIN:
		img.buf = (unsigned char *)(p_virtual_buff + MAIN_OFFSET_VM);
		img.length = MAIN_LEN;
		dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save MAIN to img.\n", __LINE__);

		break;

	case MODEM_NV:
		img.buf = (unsigned char *)(p_virtual_buff + NV_OFFSET_VM);
		img.length = NV_LEN;
		dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img save NV to img.\n", __LINE__);

		break;

	default:
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img wrong : %d.", __LINE__, type);
		return -1;
	}

	/* Command : ReqFlashSetAddress(0x0802) */
	retval = ipc_spi_send_modem_bin_execute_cmd(od, 0x0802, sizeof(unsigned int), address);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		return -1;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd Done.\n", __LINE__);

	dev_dbg(od->dev, "(%d) Start send img. size : %d\n", __LINE__, img.length);
	ptr = img.buf;
	data_size = SPI_SEND_BLOCK_SIZE;
	rest_size = img.length;

	for (i = 0 ; send_size < img.length ; i++) {
		if (rest_size < SPI_SEND_BLOCK_SIZE)
			data_size = rest_size;

		/* Command : ReqFlashWriteBlock(0x0804) */
		retval = ipc_spi_send_modem_bin_execute_cmd(od, 0x0804, data_size, ptr);
		if (retval < 0) {
			dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
			return -1;
		}

		send_size += data_size;
		rest_size -= data_size;
		ptr += data_size;

		if (!(i % 100))
			dev_dbg(od->dev, "(%d) [%d] 0x%x size done, rest size: 0x%x\n", __LINE__, i, send_size, rest_size);
	}

	return retval;
}

static void ipc_spi_send_modem_bin(struct work_struct *send_modem_w)
{
	int retval = 0;
	u32 int_cmd = 0xABCDABCD;
	u32 int_cmd_fail = 0xDCBADCBA;
	struct ipc_spi_send_modem_bin_workq_data *smw
		= container_of(send_modem_w, struct ipc_spi_send_modem_bin_workq_data, send_modem_w);
	struct ipc_spi *od = smw->od;

	unsigned int modem_addr = 0x60300000;
	unsigned int nvm_static_fix_addr = 0x60e80000;
	unsigned int nvm_static_cal_addr = 0x60f00000;
	unsigned int nvm_dynamic_addr = 0x60f80000;
	unsigned int nvm_addr = 0x60C00000;
	unsigned short sec_end = 0x0000;
	unsigned int force_hw_reset = 0x00111001;

	u8 *sec_start = NULL;

	dev_dbg(od->dev, "[SPI DUMP] mb : [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n",
		*(u8 *)(p_virtual_buff), *(u8 *)(p_virtual_buff + 1), *(u8 *)(p_virtual_buff + 2), *(u8 *)(p_virtual_buff + 3), *(u8 *)(p_virtual_buff + 4),
		 *(u8 *)(p_virtual_buff + 5),  *(u8 *)(p_virtual_buff + 6),  *(u8 *)(p_virtual_buff + 7),  *(u8 *)(p_virtual_buff + 8),  *(u8 *)(p_virtual_buff + 9),
		*(u8 *)(p_virtual_buff + 10),  *(u8 *)(p_virtual_buff + 11),  *(u8 *)(p_virtual_buff + 12),  *(u8 *)(p_virtual_buff + 13),  *(u8 *)(p_virtual_buff + 14),
		*(u8 *)(p_virtual_buff + 15),  *(u8 *)(p_virtual_buff + 16),  *(u8 *)(p_virtual_buff + 17),  *(u8 *)(p_virtual_buff + 18),  *(u8 *)(p_virtual_buff + 19));

	/* Command : ReqSecStart(0x0204) */
	sec_start = kmalloc(2048, GFP_ATOMIC);
	if (!sec_start) {
		dev_err(od->dev, "(%d) sec_start kmalloc fail.", __LINE__);
		goto err;
	}
	memset(sec_start, 0, 2048);

	retval = ipc_spi_send_modem_bin_execute_cmd(od, 0x0204, 2048, (void *)sec_start);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd Done.\n", __LINE__);

	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_MAIN, &modem_addr);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img MODEM_MAIN Done.\n", __LINE__);

	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_NV, &nvm_addr);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img MODEM_NV:nvm_addr Done.\n", __LINE__);

/*
	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_NV, &nvm_static_cal_addr);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img MODEM_NV:nvm_static_cal_addr Done.\n", __LINE__);

	retval = ipc_spi_send_modem_bin_xmit_img(od, MODEM_NV, &nvm_dynamic_addr);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_xmit_img MODEM_NV:nvm_dynamic_addr Done.\n", __LINE__);
*/

	/* Command : ReqSecEnd(0x0205) */
	retval = ipc_spi_send_modem_bin_execute_cmd(od, 0x0205, sizeof(unsigned short), &sec_end);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd; ReqSecEnd Done.\n", __LINE__);

	/* Command : ReqForceHwReset(0x0208) */
	retval = ipc_spi_send_modem_bin_execute_cmd(od, 0x0208, sizeof(unsigned int), &force_hw_reset);
	if (retval < 0) {
		dev_err(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd fail : %d", __LINE__, retval);
		goto err;
	}
	dev_dbg(od->dev, "(%d) ipc_spi_send_modem_bin_execute_cmd; ReqForceHwReset Done.\n", __LINE__);

	kfree(sec_start);

	ipc_spi_set_MRDY_pin(0);

	ipc_spi_make_data_interrupt(int_cmd, od);

	return;

err:
	ipc_spi_make_data_interrupt(int_cmd_fail, od);
}
#endif

static int __devinit ipc_spi_platform_probe(struct platform_device *pdev)
{
	int r;
	int irq;
	struct ipc_spi *od = NULL;
	struct ipc_spi_platform_data *pdata;
	struct resource *res;
	struct task_struct *th;

	printk(KERN_ERR "[%s]\n", __func__);
	pdata = pdev->dev.platform_data;
	if (!pdata || !pdata->cfg_gpio) {
		dev_err(&pdev->dev, "No platform data\n");
		r = -EINVAL;
		goto err;
	}

	gpio_mrdy = pdata->gpio_ipc_mrdy;
	gpio_srdy = pdata->gpio_ipc_srdy;

	dev_dbg(&pdev->dev, "(%d) gpio_mrdy : %d, gpio_srdy : %d(%d)\n", __LINE__, gpio_mrdy, gpio_srdy, gpio_get_value(gpio_srdy));

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "(%d) failed to get irq number\n", __LINE__);

		r = -EINVAL;
		goto err;
	}
	irq = res->start;

	od = kzalloc(sizeof(struct ipc_spi), GFP_KERNEL);
	if (!od) {
		dev_err(&pdev->dev, "(%d) failed to allocate device\n", __LINE__);

		r = -ENOMEM;
		goto err;
	}
	ipc_spi = od;

	dev_dbg(&pdev->dev, "(%d) IpcSpi dev: %p\n", __LINE__, od);

	od->base = 0;
	od->size = 0x1000000; /* 16M */
	r = _request_mem(od, pdev);
	if (r)
		goto err;

	/* init mailbox state before registering irq handler */
	onedram_init_mailbox();

	_init_data(od);

	printk(KERN_ERR "cfg_gpio run %p\n", pdata);

	pdata->cfg_gpio();

#ifndef FEATURE_SAMSUNG_SPI
	irq = gpio_to_irq(gpio_srdy);
	r = request_irq(irq, ipc_spi_irq_handler, IRQF_TRIGGER_RISING, "IPC_SRDY", od);
	if (r) {
		dev_err(&pdev->dev, "(%d) Failed to allocate an interrupt: %d\n", __LINE__, irq);

		goto err;
	}
	od->irq = irq;
	enable_irq_wake(irq);
#endif

	/* Init work structure */
	ipc_spi_send_modem_work_data = kmalloc(sizeof(struct ipc_spi_send_modem_bin_workq_data), GFP_ATOMIC);
	if (!ipc_spi_send_modem_work_data) {
		dev_err(&pdev->dev, "(%d) memory alloc fail\n", __LINE__);

		r = -ENOMEM;
		goto err;
	}
	INIT_WORK(&ipc_spi_send_modem_work_data->send_modem_w, ipc_spi_send_modem_bin);

	r = _register_chrdev(od);
	if (r) {
		dev_err(&pdev->dev, "(%d) Failed to register chrdev\n", __LINE__);

		goto err;
	}

	r = sysfs_create_group(&od->dev->kobj, &ipc_spi_group);
	if (r) {
		dev_err(&pdev->dev, "(%d) Failed to create sysfs files\n", __LINE__);

		goto err;
	}
	od->group = &ipc_spi_group;

	platform_set_drvdata(pdev, od);

#ifdef FEATURE_SAMSUNG_SPI
	spi_main_init(pdata);
#endif

	th = kthread_create(ipc_spi_thread, (void *)od, "ipc_spi_thread");
	if (IS_ERR(th)) {
		dev_err(&pdev->dev, "kernel_thread() failed : %d\n", r);

		goto err;
	}
	wake_up_process(th);

	dev_info(&pdev->dev, "(%d) platform probe Done.\n", __LINE__);

	return 0;

err:
	_release(od);
	return r;
}

static int __devexit ipc_spi_platform_remove(struct platform_device *pdev)
{
	struct ipc_spi *od = platform_get_drvdata(pdev);

	/* TODO: need onedram_resource clean? */
	_unregister_all_handlers();
	platform_set_drvdata(pdev, NULL);
	ipc_spi = NULL;
	_release(od);

	/* Free work queue data */
	kfree(ipc_spi_send_modem_work_data);

	return 0;
}

#ifdef CONFIG_PM
static int ipc_spi_platform_suspend(struct platform_device *pdev, pm_message_t state)
{
/*	struct onedram *od = platform_get_drvdata(pdev); */

	return 0;
}

static int ipc_spi_platform_resume(struct platform_device *pdev)
{
/*	struct onedram *od = platform_get_drvdata(pdev); */

	return 0;
}
#else
#  define ipc_spi_platform_suspend NULL
#  define ipc_spi_platform_resume NULL
#endif

static int ipc_spi_probe(struct spi_device *spi)
{
	int retval = 0;

	printk(KERN_ERR "[%s]\n", __func__);

	p_ipc_spi = spi;
	p_ipc_spi->mode = SPI_MODE_1;
	p_ipc_spi->bits_per_word = 32;

	retval = spi_setup(p_ipc_spi);
	if (retval != 0) {
		printk(KERN_ERR "[%s] spi_setup ERROR : %d\n", __func__, retval);

		return retval;
	}

	dev_info(&p_ipc_spi->dev, "(%d) spi probe Done.\n", __LINE__);

	return retval;
}

static int ipc_spi_remove(struct spi_device *spi)
{
	return 0;
}

static struct platform_driver ipc_spi_platform_driver = {
	.probe = ipc_spi_platform_probe,
	.remove = __devexit_p(ipc_spi_platform_remove),
	.suspend = ipc_spi_platform_suspend,
	.resume = ipc_spi_platform_resume,
	.driver = {
		.name = DRVNAME,
	},
};

static struct spi_driver ipc_spi_driver = {
	.probe = ipc_spi_probe,
	.remove = __devexit_p(ipc_spi_remove),
	.driver = {
		.name = "ipc_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
};

int ipc_spi_thread_restart()
{
	int retval;

	send_modem_spi = 1;
	is_cp_reset = 1;

	printk(KERN_ERR "[IPC_SPI] ipc_spi_thread_restart\n");

	spi_set_restart();

	platform_driver_unregister(&ipc_spi_platform_driver);

	retval = platform_driver_register(&ipc_spi_platform_driver);
	if (retval < 0) {
		printk(KERN_ERR "[%s] platform_driver_register ERROR : %d\n", __func__, retval);

		goto exit;
	}

	return 0;

exit:
	return retval;
}
EXPORT_SYMBOL(ipc_spi_thread_restart);

static int __init ipc_spi_init(void)
{
	int retval = 0;

	printk(KERN_ERR "[%s]\n", __func__);

	retval = spi_register_driver(&ipc_spi_driver);
	if (retval < 0) {
		printk(KERN_ERR "[%s] spi_register_driver ERROR : %d\n", __func__, retval);

		goto exit;
	}

	retval = platform_driver_register(&ipc_spi_platform_driver);
	if (retval < 0) {
		printk(KERN_ERR "[%s] platform_driver_register ERROR : %d\n", __func__, retval);

		goto exit;
	}

	/* creat work queue thread */
	ipc_spi_wq = create_singlethread_workqueue("ipc_spi_wq");

	if (!ipc_spi_wq) {
		printk(KERN_ERR "[%s] get workqueue thread fail\n", __func__);

		retval = -ENOMEM;
		goto exit;
	}

	printk(KERN_ERR "[%s](%d) init Done - 1011260100.\n", __func__, __LINE__);

	return 0;

exit:
	return retval;
}

static void __exit ipc_spi_exit(void)
{
	printk(KERN_ERR "[%s]\n", __func__);

	spi_unregister_driver(&ipc_spi_driver);
	platform_driver_unregister(&ipc_spi_platform_driver);

	if (ipc_spi_wq)
		destroy_workqueue(ipc_spi_wq);
}

module_init(ipc_spi_init);
module_exit(ipc_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wonhee Seo <wonhee48.seo@samsung.com>");
MODULE_DESCRIPTION("IpcSpi driver");
