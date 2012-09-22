/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNOppU General Public License, version 2, as published by
 *  the Free Software Foundation (the "GPL").
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php,
 *  or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 *
 ************************************************************************************/
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/hid.h>
#include <linux/sched.h>

MODULE_AUTHOR("Wenbin Yu <wenbinyu@broadcom.com>");
MODULE_DESCRIPTION("User level driver support for BT lock");
MODULE_SUPPORTED_DEVICE("btlock");
MODULE_LICENSE("GPL");

#define BTLOCK_NAME      "btlock"
#define BTLOCK_MINOR     224

#define PR(msg, ...) printk("#### %s " msg, current->comm, ##__VA_ARGS__)

struct btlock {
	int lock;
	int cookie;
};

static struct semaphore lock;
static int owner_cookie;

static int btlock_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int btlock_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t btlock_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	struct btlock lock_para;
	ssize_t ret = 0;
	char cookie_msg[5] = {0};
	char owner_msg[5] = {0};

	if (count < sizeof(struct btlock))
		return -EINVAL;

	if (copy_from_user(&lock_para, buffer, sizeof(struct btlock))) {
		return -EFAULT;
	}
	memcpy(cookie_msg, &lock_para.cookie, sizeof(lock_para.cookie));
	if (lock_para.lock == 0) {
		if (owner_cookie == lock_para.cookie) {
			owner_cookie = 0;
			up(&lock);
			PR("lock released, cookie: %s\n", cookie_msg);
		} else
			memcpy(owner_msg, &owner_cookie, sizeof(owner_cookie));
PR("release, cookie_mismatch:%s, owner:%s\n", cookie_msg,
				owner_cookie == 0 ? "NULL" : owner_msg);
	} else if (lock_para.lock == 1) {
		#if 1
		if (down_killable(&lock)) {
			PR("killed\n");
			return -ERESTARTSYS;
		}
		#else
		down(&lock);
		#endif
		owner_cookie = lock_para.cookie;
		PR("lock acquired, %s\n", cookie_msg);
	} else if (lock_para.lock == 2) {
		if (down_trylock(&lock) != 0) {
			PR("try failed\n");
			return -ERESTARTSYS;
		} else {
			PR("try success\n");
			owner_cookie = lock_para.cookie;
		}
	}

	return ret;
}

static long btlock_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations btlock_fops = {
	.owner   = THIS_MODULE,
	.open    = btlock_open,
	.release = btlock_release,
	.write   = btlock_write,
	.unlocked_ioctl = btlock_unlocked_ioctl,
};

static struct miscdevice btlock_misc = {
	.name  = BTLOCK_NAME,
	.minor = BTLOCK_MINOR,
	.fops  = &btlock_fops,
};


static int __init btlock_init(void)
{
	int ret;

	PR("init\n");

	ret = misc_register(&btlock_misc);
	if (ret != 0) {
		PR("Error: failed to register Misc driver,  ret = %d\n", ret);
		return ret;
	}

	sema_init(&lock, 1);
	return ret;
}

static void __exit btlock_exit(void)
{
	PR("btlock_exit:\n");

	misc_deregister(&btlock_misc);
}

module_init(btlock_init);
module_exit(btlock_exit);
