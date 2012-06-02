/*
 * Copyright (c) 2011 Qualcomm Atheros
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/module.h>
#include <linux/circ_buf.h>
#include "core.h"
#include "debug.h"
#include "dbglog.h"

#define DEVICE_NAME "ath6kl-dbglog"
#define DBGLOG_BUFSIZ 8192
static int major_num;
static int num_readers;
static struct circ_buf dbglog_buf;
static spinlock_t dbglog_lock;
static wait_queue_head_t dbglog_wait;
static struct class *dbglog_class;
static struct device *dbglog_dev;


void ath6kl_dbglog_add(const u8 *buf, size_t len)
{
	size_t i;
	int added = 0;
	int cspace;

	if (major_num < 0 || dbglog_buf.buf == NULL)
		return;

	spin_lock_bh(&dbglog_lock);
	i = 0;
	while (i < len) {
		if (CIRC_SPACE(dbglog_buf.head, dbglog_buf.tail,
			       DBGLOG_BUFSIZ) == 0) {
			spin_unlock_bh(&dbglog_lock);
			if (num_readers)
				ath6kl_dbg(ATH6KL_DBG_WMI,
					   "dbglog buffer overflow");
			return;
		}
		cspace = CIRC_SPACE_TO_END(dbglog_buf.head, dbglog_buf.tail,
					   DBGLOG_BUFSIZ);
		if ((size_t) cspace > len - i)
			cspace = len - i;
		memcpy(&dbglog_buf.buf[dbglog_buf.head], &buf[i], cspace);
		dbglog_buf.head = (dbglog_buf.head + cspace) &
			(DBGLOG_BUFSIZ - 1);
		i += cspace;
		added++;
	}
	spin_unlock_bh(&dbglog_lock);
	if (added)
		wake_up(&dbglog_wait);
}

static int ath6kl_dbglog_open(struct inode *inode, struct file *filp)
{
	if (major_num < 0 || dbglog_buf.buf == NULL)
		return -EOPNOTSUPP;

	if (num_readers)
		return -EBUSY;

	num_readers++;
	try_module_get(THIS_MODULE);

	return 0;
}

static int ath6kl_dbglog_release(struct inode *inode, struct file *filp)
{
	num_readers--;
	module_put(THIS_MODULE);
	return 0;
}

static bool ath6kl_dbglog_empty(void)
{
	return CIRC_CNT(dbglog_buf.head, dbglog_buf.tail, DBGLOG_BUFSIZ) == 0;
}

static ssize_t ath6kl_dbglog_read(struct file *filp, char *buf, size_t len,
				  loff_t *offset)
{
	char *pos = buf;

	wait_event_interruptible(dbglog_wait, !ath6kl_dbglog_empty());
	spin_lock_bh(&dbglog_lock);
	while (len &&
	       CIRC_CNT(dbglog_buf.head, dbglog_buf.tail, DBGLOG_BUFSIZ) > 0) {
		int ccnt = CIRC_CNT_TO_END(dbglog_buf.head, dbglog_buf.tail,
					   DBGLOG_BUFSIZ);
		spin_unlock_bh(&dbglog_lock);
		if ((size_t) ccnt > len)
			ccnt = len;
		if (copy_to_user(pos, &dbglog_buf.buf[dbglog_buf.tail], ccnt))
			return -EFAULT;
		pos += ccnt;
		spin_lock_bh(&dbglog_lock);
		dbglog_buf.tail = (dbglog_buf.tail + ccnt) &
			(DBGLOG_BUFSIZ - 1);
		len -= ccnt;
	}
	spin_unlock_bh(&dbglog_lock);

	return pos - buf;
}

static ssize_t ath6kl_dbglog_write(struct file *filp, const char *buf,
				   size_t len, loff_t *off)
{
	return -EOPNOTSUPP;
}

static struct file_operations fops = {
	.open = ath6kl_dbglog_open,
	.release = ath6kl_dbglog_release,
	.read = ath6kl_dbglog_read,
	.write = ath6kl_dbglog_write,
};

int ath6kl_dbglog_init(void)
{
	major_num = -1;
	dbglog_buf.buf = kmalloc(DBGLOG_BUFSIZ, GFP_KERNEL);
	if (dbglog_buf.buf == NULL)
		return -ENOMEM;

	dbglog_buf.head = dbglog_buf.tail = 0;

	major_num = register_chrdev(0, DEVICE_NAME, &fops);
	if (major_num < 0) {
		ath6kl_err("Failed to register chrdev for dbglog: %d\n",
			   major_num);
		return major_num;
	}

	dbglog_class = class_create(THIS_MODULE, DEVICE_NAME);
	dbglog_dev = device_create(dbglog_class, NULL, MKDEV(major_num, 0),
				   NULL, DEVICE_NAME);

	spin_lock_init(&dbglog_lock);
	init_waitqueue_head(&dbglog_wait);

	return 0;
}

void ath6kl_dbglog_deinit(void)
{
	device_destroy(dbglog_class, MKDEV(major_num, 0));
	class_unregister(dbglog_class);
	class_destroy(dbglog_class);
	unregister_chrdev(major_num, DEVICE_NAME);
	kfree(dbglog_buf.buf);
}
