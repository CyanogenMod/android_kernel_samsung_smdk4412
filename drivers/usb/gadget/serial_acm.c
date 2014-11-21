/*
 * serial_acm.c -- USB modem serial driver
 *
 * Copyright 2008 (C) Samsung Electronics
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/io.h>

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/usb/cdc.h>

static int acm_notify(void *dev, u16 state);


static wait_queue_head_t modem_wait_q;

static unsigned int read_state;
static unsigned int control_line_state;

static void *acm_data;

static int modem_open(struct inode *inode, struct file *file)
{
	read_state = 0;

	return 0;
}

static int modem_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t modem_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret = 0;

	if (file->f_flags & O_NONBLOCK)
		return -EAGAIN;

	ret = wait_event_interruptible(modem_wait_q, read_state);
	if (ret)
		return ret;

	if (copy_to_user(buf, &control_line_state, sizeof(u32)))
		return -EFAULT;

	read_state = 0;

	return sizeof(u32);
}

static unsigned int modem_poll(struct file *file, poll_table *wait)
{
	int ret;
	poll_wait(file, &modem_wait_q, wait);

	ret = (read_state ? (POLLIN | POLLRDNORM) : 0);

	return ret;
}

void notify_control_line_state(u32 value)
{
	control_line_state = value;

	read_state = 1;

	wake_up_interruptible(&modem_wait_q);
}
EXPORT_SYMBOL(notify_control_line_state);


#define GS_CDC_NOTIFY_SERIAL_STATE	_IOW('S', 1, int)
#define GS_IOC_NOTIFY_DTR_TEST		_IOW('S', 3, int)

static long
modem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	printk(KERN_INFO "modem_ioctl: cmd=0x%x, arg=%lu\n", cmd, arg);

	/* handle ioctls */
	switch (cmd) {
	case GS_CDC_NOTIFY_SERIAL_STATE:
		ret = acm_notify(acm_data, __constant_cpu_to_le16(arg));
		return ret;

	case GS_IOC_NOTIFY_DTR_TEST:
		{
			printk(KERN_ALERT"DUN : DTR %d\n", (int)arg);
			notify_control_line_state((int)arg);
			break;
		}

	default:
		printk(KERN_INFO "modem_ioctl: Unknown ioctl cmd(0x%x).\n",
				cmd);
		return -ENOIOCTLCMD;
	}
	return 0;
}


static const struct file_operations modem_fops = {
	.owner		= THIS_MODULE,
	.open		= modem_open,
	.release	= modem_close,
	.read		= modem_read,
	.poll		= modem_poll,
	.llseek		= no_llseek,
	.unlocked_ioctl	= modem_ioctl,
};

static struct miscdevice modem_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= "dun",
	.fops	= &modem_fops,
};

int modem_register(void *data)
{
	if (data == NULL) {
		printk(KERN_INFO "DUN register failed. data is null.\n");
		return -1;
	}

	acm_data = data;

	init_waitqueue_head(&modem_wait_q);

	printk(KERN_INFO "DUN is registerd\n");

	return 0;
}
EXPORT_SYMBOL(modem_register);

static int modem_misc_register(void)
{
	int ret;
	ret = misc_register(&modem_device);
	if (ret) {
		printk(KERN_ERR "DUN register is failed, ret = %d\n", ret);
		return ret;
	}
	return ret;
}

void modem_unregister(void)
{
	acm_data = NULL;

	printk(KERN_INFO "DUN is unregisterd\n");
}
EXPORT_SYMBOL(modem_unregister);
