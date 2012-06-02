/*
*
* drivers/media/tdmb/tdmb.c
*
* tdmb driver
*
* Copyright (C) (2011, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <linux/types.h>
#include <linux/fcntl.h>

/* for delay(sleep) */
#include <linux/delay.h>

/* for mutex */
#include <linux/mutex.h>

/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>

#include <linux/io.h>
#include <mach/gpio.h>

#include "tdmb.h"
#define TDMB_PRE_MALLOC 1

static struct class *tdmb_class;

/* ring buffer */
char *ts_ring;
unsigned int *tdmb_ts_head;
unsigned int *tdmb_ts_tail;
char *tdmb_ts_buffer;
unsigned int tdmb_ts_size;

unsigned int *cmd_head;
unsigned int *cmd_tail;
static char *cmd_buffer;
static unsigned int cmd_size;

static unsigned long tdmb_last_ch;

static struct tdmb_platform_data gpio_cfg;
static struct tdmb_drv_func *tdmbdrv_func;

static bool tdmb_pwr_on;
static bool tdmb_power_on(void)
{
	bool ret;

	if (tdmb_create_databuffer(tdmbdrv_func->get_int_size()) == false) {
		DPRINTK("%s : tdmb_create_databuffer fail\n", __func__);
		ret = false;
	} else if (tdmb_create_workqueue() == true) {
		DPRINTK("%s : tdmb_create_workqueue ok\n", __func__);
		ret = tdmbdrv_func->power_on();
	} else {
		ret = false;
	}
	tdmb_pwr_on = ret;
	DPRINTK("%s : ret(%d)\n", __func__, ret);
	return ret;
}
static bool tdmb_power_off(void)
{
	DPRINTK("%s : tdmb_pwr_on(%d)\n", __func__, tdmb_pwr_on);

	if (tdmb_pwr_on) {
		tdmbdrv_func->power_off();
		tdmb_destroy_workqueue();
		tdmb_destroy_databuffer();
		tdmb_pwr_on = false;
	}
	tdmb_last_ch = 0;

	return true;
}

static int tdmb_open(struct inode *inode, struct file *filp)
{
	DPRINTK("tdmb_open!\n");
	return 0;
}

static ssize_t
tdmb_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	DPRINTK("tdmb_read\n");

	return 0;
}

static int tdmb_release(struct inode *inode, struct file *filp)
{
	DPRINTK("tdmb_release\n");

	tdmb_power_off();

#if TDMB_PRE_MALLOC
	tdmb_ts_size = 0;
	cmd_size = 0;
#else
	if (ts_ring != 0) {
		kfree(ts_ring);
		ts_ring = 0;
		tdmb_ts_size = 0;
		cmd_size = 0;
	}
#endif

	return 0;
}

#if TDMB_PRE_MALLOC
static void tdmb_make_ring_buffer(void)
{
	size_t size = TDMB_RING_BUFFER_MAPPING_SIZE;

	/* size should aligned in PAGE_SIZE */
	if (size % PAGE_SIZE) /* klaatu hard coding */
		size = size + size % PAGE_SIZE;

	ts_ring = kmalloc(size, GFP_KERNEL);
	DPRINTK("RING Buff Create OK\n");
}

#endif

static int tdmb_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size;
	unsigned long pfn;

	DPRINTK("%s\n", __func__);

	vma->vm_flags |= VM_RESERVED;
	size = vma->vm_end - vma->vm_start;
	DPRINTK("size given : %x\n", size);

#if TDMB_PRE_MALLOC
	size = TDMB_RING_BUFFER_MAPPING_SIZE;
	if (!ts_ring) {
		DPRINTK("RING Buff ReAlloc(%d)!!\n", size);
#endif
		/* size should aligned in PAGE_SIZE */
		if (size % PAGE_SIZE) /* klaatu hard coding */
			size = size + size % PAGE_SIZE;

		ts_ring = kmalloc(size, GFP_KERNEL);
#if TDMB_PRE_MALLOC
	}
#endif

	pfn = virt_to_phys(ts_ring) >> PAGE_SHIFT;

	DPRINTK("vm_start:%lx,ts_ring:%p,size:%x,prot:%lx,pfn:%lx\n",
			vma->vm_start, ts_ring, size, vma->vm_page_prot, pfn);

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
		return -EAGAIN;

	DPRINTK("succeeded\n");

	tdmb_ts_head = (unsigned int *)ts_ring;
	tdmb_ts_tail = (unsigned int *)(ts_ring + 4);
	tdmb_ts_buffer = ts_ring + 8;

	*tdmb_ts_head = 0;
	*tdmb_ts_tail = 0;

	tdmb_ts_size = size-8; /* klaatu hard coding */
	tdmb_ts_size
	= ((tdmb_ts_size / DMB_TS_SIZE) * DMB_TS_SIZE) - (30 * DMB_TS_SIZE);

	DPRINTK("head : %x, tail : %x, buffer : %x, size : %x\n",
			(unsigned int)tdmb_ts_head, (unsigned int)tdmb_ts_tail,
			(unsigned int)tdmb_ts_buffer, tdmb_ts_size);

	cmd_buffer = tdmb_ts_buffer + tdmb_ts_size + 8;
	cmd_head = (unsigned int *)(cmd_buffer - 8);
	cmd_tail = (unsigned int *)(cmd_buffer - 4);

	*cmd_head = 0;
	*cmd_tail = 0;

	cmd_size = 30 * DMB_TS_SIZE - 8; /* klaatu hard coding */

	DPRINTK("cmd head : %x, tail : %x, buffer : %x, size : %x\n",
			(unsigned int)cmd_head, (unsigned int)cmd_tail,
			(unsigned int)cmd_buffer, cmd_size);

	return 0;
}


static int _tdmb_cmd_update(
	unsigned char *cmd_header,
	unsigned char cmd_header_size,
	unsigned char *data,
	unsigned short data_size)
{
	unsigned int size;
	unsigned int head;
	unsigned int tail;
	unsigned int dist;
	unsigned int temp_size;
	unsigned int data_size_tmp;

	if (data_size > cmd_size) {
		DPRINTK(" Error - cmd size too large\n");
		return false;
	}

	head = *cmd_head;
	tail = *cmd_tail;
	size = cmd_size;
	data_size_tmp = data_size + cmd_header_size;

	if (head >= tail)
		dist = head-tail;
	else
		dist = size + head-tail;

	if (size - dist <= data_size_tmp) {
		DPRINTK("too small space is left in Cmd Ring Buffer!!\n");
		return false;
	}

	DPRINTK("%x head %d tail %d\n", (unsigned int)cmd_buffer, head, tail);

	if (head+data_size_tmp <= size) {
		memcpy((cmd_buffer + head),
			(char *)cmd_header, cmd_header_size);
		memcpy((cmd_buffer + head + cmd_header_size),
			(char *)data, data_size);
		head += data_size_tmp;
		if (head == size)
			head = 0;
	} else {
		temp_size = size - head;
		if (temp_size < cmd_header_size) {
			memcpy((cmd_buffer+head),
				(char *)cmd_header, temp_size);
			memcpy((cmd_buffer),
				(char *)cmd_header+temp_size,
				(cmd_header_size - temp_size));
			head = cmd_header_size - temp_size;
		} else {
			memcpy((cmd_buffer+head),
				(char *)cmd_header, cmd_header_size);
			head += cmd_header_size;
			if (head == size)
				head = 0;
		}

		temp_size = size - head;
		if (temp_size < data_size) {
			memcpy((cmd_buffer+head),
				(char *)data, temp_size);
			memcpy((cmd_buffer),
				(char *)data+temp_size,
				(data_size - temp_size));
			head = data_size - temp_size;
		} else {
			memcpy((cmd_buffer+head),
				(char *)data, data_size);
			head += data_size;
			if (head == size)
				head = 0;
		}
	}

	*cmd_head = head;

	return true;
}

unsigned char tdmb_make_result(
	unsigned char cmd,
	unsigned short data_len,
	unsigned char  *data)
{
	unsigned char cmd_header[4] = {0,};

	cmd_header[0] = TDMB_CMD_START_FLAG;
	cmd_header[1] = cmd;
	cmd_header[2] = (data_len>>8)&0xff;
	cmd_header[3] = data_len&0xff;

	_tdmb_cmd_update(cmd_header, 4 , data,  data_len);

	return true;
}

unsigned long tdmb_get_chinfo(void)
{
	return tdmb_last_ch;
}

void tdmb_pull_data(struct work_struct *work)
{
	tdmbdrv_func->pull_data();
}

bool tdmb_control_irq(bool set)
{
	bool ret = true;
	int irq_ret;
	if (set) {
		irq_set_irq_type(gpio_cfg.irq, IRQ_TYPE_EDGE_FALLING);
		irq_ret = request_irq(gpio_cfg.irq
						, tdmb_irq_handler
						, IRQF_DISABLED
						, TDMB_DEV_NAME
						, NULL);
		if (irq_ret < 0) {
			DPRINTK("request_irq failed !! \r\n");
			ret = false;
		}
	} else {
		free_irq(gpio_cfg.irq, NULL);
	}

	return ret;
}

void tdmb_control_gpio(bool poweron)
{
	if (poweron)
		gpio_cfg.gpio_on();
	else
		gpio_cfg.gpio_off();
}

static long tdmb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long fig_freq = 0;
	struct ensemble_info_type *ensemble_info;
	struct tdmb_dm dm_buff;

	DPRINTK("call tdmb_ioctl : 0x%x\n", cmd);

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC) {
		DPRINTK("tdmb_ioctl : _IOC_TYPE error\n");
		return -EINVAL;
	}
	if (_IOC_NR(cmd) >= IOCTL_MAXNR) {
		DPRINTK("tdmb_ioctl : _IOC_NR(cmd) 0x%x\n", _IOC_NR(cmd));
		return -EINVAL;
	}

	switch (cmd) {
	case IOCTL_TDMB_GET_DATA_BUFFSIZE:
		DPRINTK("IOCTL_TDMB_GET_DATA_BUFFSIZE %d\n", tdmb_ts_size);
		ret = copy_to_user((unsigned int *)arg,
				&tdmb_ts_size, sizeof(unsigned int));
		break;

	case IOCTL_TDMB_GET_CMD_BUFFSIZE:
		DPRINTK("IOCTL_TDMB_GET_CMD_BUFFSIZE %d\n", cmd_size);
		ret = copy_to_user((unsigned int *)arg,
				&cmd_size, sizeof(unsigned int));
		break;

	case IOCTL_TDMB_POWER_ON:
		DPRINTK("IOCTL_TDMB_POWER_ON\n");
		ret = tdmb_power_on();
		break;

	case IOCTL_TDMB_POWER_OFF:
		DPRINTK("IOCTL_TDMB_POWER_OFF\n");
		ret = tdmb_power_off();
		break;

	case IOCTL_TDMB_SCAN_FREQ_ASYNC:
		DPRINTK("IOCTL_TDMB_SCAN_FREQ_ASYNC\n");

		fig_freq = arg;

		ensemble_info = vmalloc(sizeof(struct ensemble_info_type));
		memset((char *)ensemble_info, 0x00\
			, sizeof(struct ensemble_info_type));

		ret = tdmbdrv_func->scan_ch(ensemble_info, fig_freq);
		if (ret == true)
			tdmb_make_result(DMB_FIC_RESULT_DONE,
				sizeof(struct ensemble_info_type),
				(unsigned char *)ensemble_info);
		else
			tdmb_make_result(DMB_FIC_RESULT_FAIL,
				sizeof(unsigned long),
				(unsigned char *)&fig_freq);

		vfree(ensemble_info);
		tdmb_last_ch = 0;
		break;

	case IOCTL_TDMB_SCAN_FREQ_SYNC:
		fig_freq = ((struct ensemble_info_type *)arg)->ensem_freq;
		DPRINTK("IOCTL_TDMB_SCAN_FREQ_SYNC %ld\n", fig_freq);

		ensemble_info = vmalloc(sizeof(struct ensemble_info_type));
		memset((char *)ensemble_info, 0x00\
			, sizeof(struct ensemble_info_type));

		ret = tdmbdrv_func->scan_ch(ensemble_info, fig_freq);
		if (ret == true) {
			if (copy_to_user((struct ensemble_info_type *)arg,
					ensemble_info,
					sizeof(struct ensemble_info_type))
				)
				DPRINTK("cmd(%x) : copy_to_user failed\n", cmd);
		}

		vfree(ensemble_info);
		tdmb_last_ch = 0;
		break;

	case IOCTL_TDMB_SCANSTOP:
		DPRINTK("IOCTL_TDMB_SCANSTOP\n");
		ret = false;
		break;

	case IOCTL_TDMB_ASSIGN_CH:
		DPRINTK("IOCTL_TDMB_ASSIGN_CH %ld\n", arg);
		tdmb_init_data();
		ret = tdmbdrv_func->set_ch(arg, (arg % 1000), false);
		if (ret == true)
			tdmb_last_ch = arg;
		else
			tdmb_last_ch = 0;
		break;

	case IOCTL_TDMB_ASSIGN_CH_TEST:
		DPRINTK("IOCTL_TDMB_ASSIGN_CH_TEST %ld\n", arg);
		tdmb_init_data();
		ret = tdmbdrv_func->set_ch(arg, (arg % 1000), true);
		if (ret == true)
			tdmb_last_ch = arg;
		else
			tdmb_last_ch = 0;
		break;

	case IOCTL_TDMB_GET_DM:
		tdmbdrv_func->get_dm(&dm_buff);
		if (copy_to_user((struct tdmb_dm *)arg\
			, &dm_buff, sizeof(struct tdmb_dm)))
			DPRINTK("IOCTL_TDMB_GET_DM : copy_to_user failed\n");
		ret = true;
		DPRINTK("rssi %d, ber %d, ANT %d\n",
			dm_buff.rssi, dm_buff.ber, dm_buff.antenna);
		break;
	}

	return ret;
}

static const struct file_operations tdmb_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = tdmb_open,
	.read           = tdmb_read,
	.unlocked_ioctl  = tdmb_ioctl,
	.mmap           = tdmb_mmap,
	.release	    = tdmb_release,
	.llseek         = no_llseek,
};

static struct tdmb_drv_func *tdmb_get_drv_func(void)
{
	struct tdmb_drv_func * (*func)(void);
#if defined(CONFIG_TDMB_T3900) && defined(CONFIG_TDMB_TCC3170)
	if (system_rev >= 11)
		func = tcc3170_drv_func;
	else
		func = t3900_drv_func;
#elif defined(CONFIG_TDMB_T3900) || defined(CONFIG_TDMB_T39F0)
	func = t3900_drv_func;
#elif defined(CONFIG_TDMB_FC8050)
	func = fc8050_drv_func;
#elif defined(CONFIG_TDMB_MTV318)
	func = mtv318_drv_func;
#elif defined(CONFIG_TDMB_TCC3170)
	func = tcc3170_drv_func;
#else
	#error what???
#endif

	return func();
}

static int tdmb_probe(struct platform_device *pdev)
{
	int ret;
	struct device *tdmb_dev;
	struct tdmb_platform_data *p = pdev->dev.platform_data;

	DPRINTK("call tdmb_probe\n");

	ret = register_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME, &tdmb_ctl_fops);
	if (ret < 0)
		DPRINTK("register_chrdev(TDMB_DEV) failed!\n");

	tdmb_class = class_create(THIS_MODULE, TDMB_DEV_NAME);
	if (IS_ERR(tdmb_class)) {
		unregister_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME);
		class_destroy(tdmb_class);
		DPRINTK("class_create failed!\n");

		return -EFAULT;
	}

	tdmb_dev = device_create(tdmb_class, NULL,
				MKDEV(TDMB_DEV_MAJOR, TDMB_DEV_MINOR),
				NULL, TDMB_DEV_NAME);
	if (IS_ERR(tdmb_dev)) {
		DPRINTK("device_create failed!\n");

		unregister_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME);
		class_destroy(tdmb_class);

		return -EFAULT;
	}

	memcpy(&gpio_cfg, p, sizeof(struct tdmb_platform_data));

	tdmb_init_bus();
	tdmbdrv_func = tdmb_get_drv_func();
	if (tdmbdrv_func->init)
		tdmbdrv_func->init();

#if TDMB_PRE_MALLOC
	tdmb_make_ring_buffer();
#endif

	return 0;
}

static int tdmb_remove(struct platform_device *pdev)
{
	return 0;
}

static int tdmb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int tdmb_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver tdmb_driver = {
	.probe	= tdmb_probe,
	.remove = tdmb_remove,
	.suspend = tdmb_suspend,
	.resume = tdmb_resume,
	.driver = {
		.owner	= THIS_MODULE,
		.name = "tdmb"
	},
};

static int __init tdmb_init(void)
{
	int ret;

#ifdef CONFIG_BATTERY_SEC
	if (is_lpcharging_state()) {
		pr_info("%s : LPM Charging Mode! return 0\n", __func__);
		return 0;
	}
#endif

	DPRINTK("<klaatu TDMB> module init\n");
	ret = platform_driver_register(&tdmb_driver);
	if (ret)
		return ret;

	return 0;
}

static void __exit tdmb_exit(void)
{
	DPRINTK("<klaatu TDMB> module exit\n");
#if TDMB_PRE_MALLOC
	if (ts_ring != 0) {
		kfree(ts_ring);
		ts_ring = 0;
	}
#endif
	unregister_chrdev(TDMB_DEV_MAJOR, "tdmb");
	device_destroy(tdmb_class, MKDEV(TDMB_DEV_MAJOR, TDMB_DEV_MINOR));
	class_destroy(tdmb_class);

	platform_driver_unregister(&tdmb_driver);

	tdmb_exit_bus();
}

module_init(tdmb_init);
module_exit(tdmb_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("TDMB Driver");
MODULE_LICENSE("GPL v2");
