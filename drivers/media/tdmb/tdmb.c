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

#if defined(CONFIG_TDMB_ANT_DET)
#include <linux/input.h>
#endif

#ifdef CONFIG_MACH_C1
#include <linux/wakelock.h>
static struct wake_lock tdmb_wlock;
#endif
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
	if (tdmb_create_databuffer(tdmbdrv_func->get_int_size()) == false) {
		DPRINTK("tdmb_create_databuffer fail\n");
		goto create_databuffer_fail;
	}
	if (tdmb_create_workqueue() == false) {
		DPRINTK("tdmb_create_workqueue fail\n");
		goto create_workqueue_fail;
	}
	if (tdmbdrv_func->power_on() == false) {
		DPRINTK("power_on fail\n");
		goto power_on_fail;
	}

	DPRINTK("power_on success\n");
#ifdef CONFIG_MACH_C1
	wake_lock(&tdmb_wlock);
#endif
	tdmb_pwr_on = true;
	return true;

power_on_fail:
	tdmb_destroy_workqueue();
create_workqueue_fail:
	tdmb_destroy_databuffer();
create_databuffer_fail:
	tdmb_pwr_on = false;

	return false;
}
static bool tdmb_power_off(void)
{
	DPRINTK("%s : tdmb_pwr_on(%d)\n", __func__, tdmb_pwr_on);

	if (tdmb_pwr_on) {
		tdmbdrv_func->power_off();
		tdmb_destroy_workqueue();
		tdmb_destroy_databuffer();
#ifdef CONFIG_MACH_C1
		wake_unlock(&tdmb_wlock);
#endif
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

#if defined(CONFIG_TDMB_ANT_DET)

static struct input_dev *tdmb_ant_input;
static int tdmb_check_ant;
static int ant_prev_status;

#define TDMB_ANT_CHECK_DURATION 500000
#define TDMB_ANT_CHECK_COUNT 2
static bool tdmb_ant_det_check_value(void)
{
	int loop = 0;
	int cur_val = 0, prev_val = 0;
	bool ret = false;

	tdmb_check_ant = 1;

	prev_val = ant_prev_status ? 0 : 1;
	for (loop = 0; loop < TDMB_ANT_CHECK_COUNT; loop++) {
		usleep_range(TDMB_ANT_CHECK_DURATION, TDMB_ANT_CHECK_DURATION);
		cur_val = gpio_get_value_cansleep(gpio_cfg.gpio_ant_det);
		if (prev_val != cur_val || ant_prev_status == cur_val)
			break;
		prev_val = cur_val;
	}

	if (loop == TDMB_ANT_CHECK_COUNT) {
		if (ant_prev_status == 0 && cur_val == 1)
			ret = true;

		ant_prev_status = cur_val;
	}

	tdmb_check_ant = 0;

	DPRINTK("%s cnt(%d) cur(%d) ret(%d)\n", __func__, loop, cur_val, ret);

	return ret;
}

static int tdmb_ant_det_ignore_irq(void)
{
	DPRINTK("%s tdmb_check_ant=%d\n", __func__, tdmb_check_ant);
	return tdmb_check_ant;
}

static void tdmb_ant_det_work_func(struct work_struct *work)
{
	int val = 0;

	if (!tdmb_ant_input) {
		DPRINTK("%s: input device is not registered\n", __func__);
		return;
	}

	if (tdmb_ant_det_check_value()) {
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_UP, 1);
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_UP, 0);
		input_sync(tdmb_ant_input);
		DPRINTK("%s: sys_rev:%d\n", __func__, system_rev);
	}
}

static struct workqueue_struct *tdmb_ant_det_wq;
static DECLARE_WORK(tdmb_ant_det_work, tdmb_ant_det_work_func);
static bool tdmb_ant_det_reg_input(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;

	DPRINTK("%s\n", __func__);

	input = input_allocate_device();
	if (!input) {
		DPRINTK("Can't allocate input device\n");
		err = -ENOMEM;
	}
	set_bit(EV_KEY, input->evbit);
	set_bit(KEY_DMB_ANT_DET_UP & KEY_MAX, input->keybit);
	set_bit(KEY_DMB_ANT_DET_DOWN & KEY_MAX, input->keybit);
	input->name = "sec_dmb_key";
	input->phys = "sec_dmb_key/input0";
	input->dev.parent = &pdev->dev;

	err = input_register_device(input);
	if (err) {
		DPRINTK("Can't register dmb_ant_det key: %d\n", err);
		goto free_input_dev;
	}
	tdmb_ant_input = input;
	ant_prev_status = gpio_get_value_cansleep(gpio_cfg.gpio_ant_det);

	return true;

free_input_dev:
	input_free_device(input);
	return false;
}

static void tdmb_ant_det_unreg_input(void)
{
	DPRINTK("%s\n", __func__);
	if (tdmb_ant_input) {
		input_unregister_device(tdmb_ant_input);
		tdmb_ant_input = NULL;
	}
}
static bool tdmb_ant_det_create_wq(void)
{
	DPRINTK("%s\n", __func__);
	tdmb_ant_det_wq = create_singlethread_workqueue("tdmb_ant_det_wq");
	if (tdmb_ant_det_wq)
		return true;
	else
		return false;
}

static bool tdmb_ant_det_destroy_wq(void)
{
	DPRINTK("%s\n", __func__);
	if (tdmb_ant_det_wq) {
		flush_workqueue(tdmb_ant_det_wq);
		destroy_workqueue(tdmb_ant_det_wq);
		tdmb_ant_det_wq = NULL;
	}
	return true;
}

static irqreturn_t tdmb_ant_det_irq_handler(int irq, void *dev_id)
{
	int ret = 0;

	if (tdmb_ant_det_ignore_irq())
		return IRQ_HANDLED;

	if (tdmb_ant_det_wq) {
		ret = queue_work(tdmb_ant_det_wq, &tdmb_ant_det_work);
		if (ret == 0)
			DPRINTK("%s queue_work fail\n", __func__);
	}

	return IRQ_HANDLED;
}

static bool tdmb_ant_det_irq_set(bool set)
{
	bool ret = true;
	int irq_ret;
	DPRINTK("%s\n", __func__);

	if (set) {
		if (system_rev >= 6)
			irq_set_irq_type(gpio_cfg.irq_ant_det
					, IRQ_TYPE_EDGE_BOTH);
		else
			irq_set_irq_type(gpio_cfg.irq_ant_det
					, IRQ_TYPE_EDGE_RISING);

		irq_ret = request_irq(gpio_cfg.irq_ant_det
						, tdmb_ant_det_irq_handler
						, IRQF_DISABLED
						, "tdmb_ant_det"
						, NULL);
		if (irq_ret < 0) {
			DPRINTK("%s %d\r\n", __func__, irq_ret);
			ret = false;
		}
		enable_irq_wake(gpio_cfg.irq_ant_det);
	} else {
		disable_irq_wake(gpio_cfg.irq_ant_det);
		free_irq(gpio_cfg.irq_ant_det, NULL);
	}

	return ret;
}
#endif

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
#ifdef CONFIG_MACH_C1
	wake_lock_init(&tdmb_wlock, WAKE_LOCK_SUSPEND, "tdmb_wlock");
#endif

#if defined(CONFIG_TDMB_ANT_DET)
	if (!tdmb_ant_det_reg_input(pdev))
		goto err_reg_input;
	if (!tdmb_ant_det_create_wq())
		goto free_reg_input;
	if (!tdmb_ant_det_irq_set(true))
		goto free_ant_det_wq;

	return 0;

free_ant_det_wq:
	tdmb_ant_det_destroy_wq();
free_reg_input:
	tdmb_ant_det_unreg_input();
err_reg_input:
	return -EFAULT;
#else
	return 0;
#endif

}

static int tdmb_remove(struct platform_device *pdev)
{
	DPRINTK("tdmb_remove!\n");
#if defined(CONFIG_TDMB_ANT_DET)
	tdmb_ant_det_unreg_input();
	tdmb_ant_det_destroy_wq();
	tdmb_ant_det_irq_set(false);
#endif
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
#ifdef CONFIG_MACH_C1
	wake_lock_destroy(&tdmb_wlock);
#endif
}

module_init(tdmb_init);
module_exit(tdmb_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("TDMB Driver");
MODULE_LICENSE("GPL v2");
