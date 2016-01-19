/*
 * serial_acm_slp.c -- USB modem serial driver
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
#include <linux/mutex.h>

#include <linux/uaccess.h>
#include <linux/io.h>

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/usb/cdc.h>
#include <linux/slab.h>

/*
 * Changes
 *
 * Yongsul Oh <yongsul96.oh> 2011.05.13
 * Added mutex lock/unlock mechanism  & usb connection state check routine
 *  to improve state-control-mechanism & stability
 */

static int acm_notify(void *dev, u16 state);

static struct task_struct *acm_dbg_task;
static atomic_t link_state;

struct serial_acm_dev {
	unsigned int read_state;
	unsigned int control_line_state;
	unsigned char busy;

	void *acm_data;
	wait_queue_head_t modem_wait_q;

	spinlock_t link_lock;
};

static struct serial_acm_dev *s_acm_dev;

static int modem_open(struct inode *inode, struct file *file)
{
	if (s_acm_dev->busy) {
		printk(KERN_DEBUG "ACM_dun is already opened by %s(%d),  tried %s(%d)\n",
			acm_dbg_task ? acm_dbg_task->comm  : "NULL",
			acm_dbg_task ? acm_dbg_task->pid : 0,
			current->comm, current->pid);
		return -EBUSY;
	}

	if (!atomic_read(&link_state)) {
		printk(KERN_DEBUG "Cannot open ACM_dun(not connected)\n");
		return -ENODEV;
	}

	s_acm_dev->busy = 1;
	s_acm_dev->read_state = 0;
	acm_dbg_task = current;

	printk(KERN_DEBUG "Opened ACM_dun by processor: %s(%d), and real_parent: %s(%d)\n",
		 current->comm , current->pid,
		 current->real_parent->comm , current->real_parent->pid);

	return 0;
}

static int modem_close(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "Closed ACM_dun from processor: %s(%d), but opend by: %s(%d)\n",
			current->comm , current->pid,
			acm_dbg_task ? acm_dbg_task->comm  : "NULL",
			acm_dbg_task ? acm_dbg_task->pid : 0);

	acm_dbg_task = NULL;
	s_acm_dev->busy = 0;

	return 0;
}

static ssize_t modem_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret = 0;

	if (file->f_flags & O_NONBLOCK)
		return -EPERM;

	ret = wait_event_interruptible(s_acm_dev->modem_wait_q,
			(s_acm_dev->read_state || (!atomic_read(&link_state))));
	if (ret)
		return ret;

	if (!atomic_read(&link_state)) {
		printk(KERN_DEBUG "Cannot read ACM_dun(not connected)\n");
		return -ENODEV;
	}

	if (copy_to_user(buf, &s_acm_dev->control_line_state, sizeof(u32)))
		return -EFAULT;

	s_acm_dev->read_state = 0;

	return sizeof(u32);
}

static unsigned int modem_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &s_acm_dev->modem_wait_q, wait);
	return (s_acm_dev->read_state ? (POLLIN | POLLRDNORM) : 0) |
			 (!atomic_read(&link_state) ? (POLLERR | POLLHUP) : 0);
}

void notify_control_line_state(u32 value)
{
	if (s_acm_dev == NULL) {
		printk(KERN_ERR "ACM_dun notify control failed. s_acm_dev is null.\n");
		return ;
	}

	s_acm_dev->control_line_state = value;

	s_acm_dev->read_state = 1;

	wake_up_interruptible(&s_acm_dev->modem_wait_q);
}
EXPORT_SYMBOL(notify_control_line_state);

#define GS_CDC_NOTIFY_SERIAL_STATE	_IOW('S', 1, int)
#define GS_IOC_NOTIFY_DTR_TEST		_IOW('S', 3, int)

static long modem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_DEBUG "ACM_dun modem_ioctl: cmd=0x%x, arg=%lu\n", cmd, arg);

	if (!atomic_read(&link_state)) {
		printk(KERN_DEBUG "Cannot ioctl ACM_dun(not connected)\n");
		return -ENODEV;
	}

	/* handle ioctls */
	switch (cmd) {
	case GS_CDC_NOTIFY_SERIAL_STATE:
		acm_notify(s_acm_dev->acm_data, __constant_cpu_to_le16(arg));
		break;

	case GS_IOC_NOTIFY_DTR_TEST:
		printk(KERN_ALERT"DUN : DTR %d\n", (int)arg);
		notify_control_line_state((int)arg);
		break;

	default:
		printk(KERN_ERR "ACM_dun: Unknown ioctl cmd(0x%x).\n", cmd);
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
	.unlocked_ioctl		= modem_ioctl,
};

static struct miscdevice modem_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "dun",
	.fops	= &modem_fops,
};

int modem_register(void *data)
{
	if ((data == NULL) || (s_acm_dev == NULL)) {
		printk(KERN_ERR "ACM_dun connect failed. data or s_acm_dev is null.\n");
		return -1;
	}

	spin_lock(&s_acm_dev->link_lock);

	s_acm_dev->acm_data = data;
	atomic_set(&link_state, 1);

	spin_unlock(&s_acm_dev->link_lock);

	printk(KERN_INFO "ACM_dun is connected\n");

	return 0;
}
EXPORT_SYMBOL(modem_register);

void modem_unregister(void)
{
	if (s_acm_dev == NULL) {
		printk(KERN_ERR "ACM_dun disconnect failed. s_acm_dev is null.\n");
		return ;
	}

	spin_lock(&s_acm_dev->link_lock);

	atomic_set(&link_state, 0);
	s_acm_dev->acm_data = NULL;
	wake_up(&s_acm_dev->modem_wait_q);

	spin_unlock(&s_acm_dev->link_lock);

	printk(KERN_INFO "ACM_dun is disconnected\n");
}
EXPORT_SYMBOL(modem_unregister);

static int modem_misc_register(void)
{
	struct serial_acm_dev *dev;
	int ret = 0;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	ret = misc_register(&modem_device);
	if (ret) {
		printk(KERN_ERR "[serial_acm] misc_register failed\n");
		kfree(dev);
		goto err_reg;
	}

	spin_lock_init(&dev->link_lock);
	init_waitqueue_head(&dev->modem_wait_q);
	atomic_set(&link_state, 0);

	acm_dbg_task = NULL;

	s_acm_dev = dev;
err_reg:
	return ret;
}

static void modem_misc_unregister(void)
{
	struct serial_acm_dev *dev = s_acm_dev;

	misc_deregister(&modem_device);

	atomic_set(&link_state, 0);
	dev->acm_data = NULL;

	wake_up(&s_acm_dev->modem_wait_q);
	kfree(s_acm_dev);
}
