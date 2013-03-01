#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/mmc/card.h>
#include <asm/uaccess.h>

#include "ram.h"

/*
 * Warning: This code is probably thread-unsafe, buggy as hell, and may
 * fry your device and your house. Use it with your own responsibility!
 */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "mmcram"

static int g_major;
static struct mmc_card *g_card = NULL;
static u8 *g_page;

DEFINE_SEMAPHORE(g_mutex);

#define MOVI_PAGE_SIZE (512)

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

int init_mmc_ram(struct mmc_card *card)
{
	g_card = card;
        g_major = register_chrdev(0, DEVICE_NAME, &fops);

	if (g_major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", g_major);
	  return g_major;
	}

	return SUCCESS;
}

static int device_open(struct inode *inode, struct file *file)
{
	if ((g_page = (u8 *)kmalloc(512, GFP_KERNEL)) == NULL) {
		printk(KERN_ALERT "kmalloc(512) failed\n");
		return -ENOMEM;
	}

	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
	kfree(g_page);
	return SUCCESS;
}

static ssize_t device_read(struct file *filp,
			   char __user *buf,
			   size_t count,
			   loff_t *ppos)
{
	size_t read = 0;
	down(&g_mutex);

	/* Memory map */
	if (*ppos < 0x20000) {
		count = min(count, (u32)(0x20000 - *ppos));
	} else if (0x20000 <= *ppos && *ppos < 0x40000) {
		// Memory hole here -- just return zeroes
		count = min(count, (u32)(0x40000 - *ppos));
		*ppos += count;
		if (clear_user(buf, count))
			read = -EFAULT;
		else
			read = count;
		goto out;
	} else if (0x40000 <= *ppos && *ppos < 0x80000) {
		count = min(count, (u32)(0x80000 - *ppos));
	} else {
		goto out;
	}

	while (count) {
		u32 addr = rounddown(*ppos, MOVI_PAGE_SIZE);
		u32 off = *ppos - addr;
		u32 to_read = min(count, MOVI_PAGE_SIZE - off);

		mmc_movi_read_ram_page(g_card, g_page, addr);
		if (copy_to_user(&buf[read], &g_page[off], to_read)) {
			read = -EFAULT;
			goto out;
		}

		count -= to_read;
		*ppos += to_read;
		read += to_read;
	}

out:
	up(&g_mutex);
	return read;
}

static ssize_t device_write(struct file *filp,
                            const char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
	return -EINVAL;
}

