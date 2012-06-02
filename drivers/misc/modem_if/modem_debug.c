/* linux/drivers/misc/modem_if/modem_debug.c
 *
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/if_arp.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_variation.h"
#include "modem_utils.h"

static const char *hex = "0123456789abcdef";

static inline void mif_irq2str(struct mif_event_buff *evtb, char *buff)
{
	int i;
	char tb[32];

	if (evtb->link_type == LINKDEV_DPRAM) {
		struct dpram_irq_buff *irqb = &evtb->dpram_irqb;

		sprintf(tb, "{0x%04X %d 0x%04X}",
			irqb->magic, irqb->access, irqb->int2ap);
		strcat(buff, tb);

		for (i = 0; i < IPC_RFS; i++) {
			snprintf(tb, 32, " {%d: %u %u %u %u}", i,
				irqb->qsp[i].txq.in, irqb->qsp[i].txq.out,
				irqb->qsp[i].rxq.in, irqb->qsp[i].rxq.out);
			strcat(buff, tb);
		}
	} else {
		sprintf(tb, "link unspeicified");
		strcat(buff, tb);
	}
}

static inline void mif_dump2hex(const char *data, size_t len, char *buff)
{
	char *src = (char *)data;
	int i;
	char tb[4];

	tb[3] = 0;
	for (i = 0; i < len; i++) {
		tb[0] = hex[(*src >> 4) & 0xf];
		tb[1] = hex[*src & 0xf];
		tb[2] = ' ';
		strcat(buff, tb);
		src++;
	}
}

static inline void mif_fin_str(char *buff)
{
	char tb[4];
	sprintf(tb, "\n");
	strcat(buff, tb);
}

static void mif_log2str(struct mif_event_buff *evtb, char *buff)
{
	struct timeval *tv;
	struct tm date;

	tv = &evtb->tv;

	time_to_tm((tv->tv_sec - sys_tz.tz_minuteswest * 60), 0, &date);
	sprintf(evtb->time, "%02d:%02d:%02d.%03ld",
		date.tm_hour, date.tm_min, date.tm_sec, (tv->tv_usec / 1000));

	if (evtb->evt == MIF_IRQ_EVT) {
		sprintf(buff, "%s IRQ <%s> ", evtb->time, evtb->ld);
		mif_irq2str(evtb, buff);
		mif_fin_str(buff);
	} else {
		size_t len = evtb->len < (MAX_MIF_LOG_LEN >> 3) ?
				evtb->len : (MAX_MIF_LOG_LEN >> 3);
		sprintf(buff, "%s [%d] <%s:%s> ",
			evtb->time, evtb->evt, evtb->iod, evtb->ld);
		mif_dump2hex(evtb->data, len, buff);
		mif_fin_str(buff);
	}
}

static void mif_print_logs(struct modem_ctl *mc)
{
	struct sk_buff *skb;
	struct mif_event_buff *evtb;
	u8 *buff;

	buff = kmalloc(2048, GFP_ATOMIC);
	if (!buff)
		return;

	while (1) {
		skb = skb_dequeue(&mc->evtq);
		if (!skb)
			break;

		evtb = (struct mif_event_buff *)skb->data;
		memset(buff, 0, 2048);

		mif_log2str(evtb, buff);
		pr_info("mif: %s", buff);

		dev_kfree_skb_any(skb);
	}

	kfree(buff);
}

static void mif_save_logs(struct modem_ctl *mc)
{
	struct file *fp = mc->log_fp;
	struct sk_buff *skb;
	struct mif_event_buff *evtb;
	int qlen = mc->evtq.qlen;
	int i;
	int ret;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	for (i = 0; i < qlen; i++) {
		skb = skb_dequeue(&mc->evtq);

#if 0
		if (evtb->evt < mc->log_level) {
			ret = fp->f_op->write(fp, skb->data,
					MAX_MIF_EVT_BUFF_SIZE, &fp->f_pos);
			if (ret < 0) {
				mif_log2str((struct mif_event_buff *)skb->data,
						mc->buff);
				printk(KERN_ERR "%s", mc->buff);
			}
		}
#else
		evtb = (struct mif_event_buff *)skb->data;
		if (evtb->evt < mc->log_level) {
			mif_log2str(evtb, mc->buff);
			ret = fp->f_op->write(fp, mc->buff, strlen(mc->buff),
						&fp->f_pos);
			if (ret < 0)
				printk(KERN_ERR "%s", mc->buff);
		}
#endif

		dev_kfree_skb_any(skb);
	}

	set_fs(old_fs);
}

static void mif_evt_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, evt_work);
	struct file *fp = mc->log_fp;
	loff_t size;
	mm_segment_t old_fs;

	/* use_mif_log */

	if (!mc->log_level || mc->fs_failed) {
		mif_print_logs(mc);
		return;
	}

	/* use_mif_log && log_level && !fs_failed */

	if (!mc->fs_ready) {
		if (mc->evtq.qlen > 1000)
			mif_print_logs(mc);
		return;
	}

	/* use_mif_log && log_level && !fs_failed && fs_ready */

	if (fp) {
		mif_save_logs(mc);

		old_fs = get_fs();
		set_fs(get_ds());
		size = fp->f_pos;
		set_fs(old_fs);
		if (size > MAX_MIF_LOG_FILE_SIZE) {
			mif_err("%s size %lld > %d\n", mc->log_path, size,
				MAX_MIF_LOG_FILE_SIZE);
			mif_close_log_file(mc);
			mif_open_log_file(mc);
		}
	}
}

void mif_irq_log(struct modem_ctl *mc, struct sk_buff *skb)
{
	if (!mc || !mc->use_mif_log)
		return;
	skb_queue_tail(&mc->evtq, skb);
}

void mif_ipc_log(struct modem_ctl *mc, enum mif_event_id evt,
		struct io_device *iod, struct link_device *ld,
		u8 *data, unsigned size)
{
	struct sk_buff *skb;
	struct mif_event_buff *evtb;
	unsigned len;

	if (!mc || !mc->use_mif_log)
		return;

	skb = alloc_skb(MAX_MIF_EVT_BUFF_SIZE, GFP_ATOMIC);
	if (!skb)
		return;

	evtb = (struct mif_event_buff *)skb_put(skb, MAX_MIF_EVT_BUFF_SIZE);
	memset(evtb, 0, MAX_MIF_EVT_BUFF_SIZE);

	do_gettimeofday(&evtb->tv);
	evtb->evt = evt;

	strncpy(evtb->mc, mc->name, MAX_MIF_NAME_LEN);

	if (iod)
		strncpy(evtb->iod, iod->name, MAX_MIF_NAME_LEN);

	if (ld) {
		strncpy(evtb->ld, ld->name, MAX_MIF_NAME_LEN);
		evtb->link_type = ld->link_type;
	}

	len = min_t(unsigned, MAX_MIF_LOG_LEN, size);
	memcpy(evtb->data, data, len);

	evtb->rcvd = size;
	evtb->len = len;

	skb_queue_tail(&mc->evtq, skb);
}

void mif_flush_logs(struct modem_ctl *mc)
{
	if (!mc || !mc->use_mif_log)
		return;

	if (atomic_read(&mc->log_open))
		queue_work(mc->evt_wq, &mc->evt_work);
}

int mif_init_log(struct modem_ctl *mc)
{
	char wq_name[32];
	char wq_suffix[32];

	mc->log_level = 0;

	atomic_set(&mc->log_open, 0);

	memset(wq_name, 0, sizeof(wq_name));
	memset(wq_suffix, 0, sizeof(wq_suffix));
	strncpy(wq_name, mc->name, sizeof(wq_name));
	snprintf(wq_suffix, sizeof(wq_suffix), "%s", "_evt_wq");
	strncat(wq_name, wq_suffix, sizeof(wq_suffix));
	mc->evt_wq = create_singlethread_workqueue(wq_name);
	if (!mc->evt_wq) {
		printk(KERN_ERR "<%s:%s> fail to create %s\n",
			__func__, mc->name, wq_name);
		return -EFAULT;
	}
	printk(KERN_ERR "<%s:%s> %s created\n",
		__func__, mc->name, wq_name);

	INIT_WORK(&mc->evt_work, mif_evt_work);

	skb_queue_head_init(&mc->evtq);

	mc->buff = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!mc->buff) {
		printk(KERN_ERR "<%s> kzalloc fail\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

void mif_set_log_level(struct modem_ctl *mc)
{
	struct file *fp;
	int ret;
	mm_segment_t old_fs;

	mc->log_level = 0;

	old_fs = get_fs();
	set_fs(get_ds());
	fp = filp_open(MIF_LOG_LV_FILE, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk(KERN_ERR "<%s:%s> %s open fail\n",
			__func__, mc->name, MIF_LOG_LV_FILE);
	} else {
		char tb;

		ret = fp->f_op->read(fp, &tb, 1, &fp->f_pos);
		if (ret > 0) {
			mc->log_level = tb & 0xF;
		} else {
			printk(KERN_ERR "<%s:%s> read fail (err %d)\n",
				__func__, mc->name, ret);
		}
	}
	set_fs(old_fs);

	if (mc->use_mif_log && !mc->log_level)
		atomic_set(&mc->log_open, 1);

	printk(KERN_ERR "<%s:%s> log level = %d\n",
		__func__, mc->name, mc->log_level);
}

int mif_open_log_file(struct modem_ctl *mc)
{
	struct timeval now;
	struct tm date;
	mm_segment_t old_fs;

	if (!mc || !mc->use_mif_log)
		return -EINVAL;

	if (!mc->log_level) {
		printk(KERN_ERR "<%s:%s> IPC logger is disabled.\n",
			__func__, mc->name);
		return -EINVAL;
	}

	if (!mc->fs_ready) {
		printk(KERN_ERR "<%s:%s> File system is not ready.\n",
			__func__, mc->name);
		return -EINVAL;
	}

	if (mc->fs_failed) {
		printk(KERN_ERR "<%s:%s> Log file cannot be created.\n",
			__func__, mc->name);
		return -EINVAL;
	}

	do_gettimeofday(&now);
	time_to_tm((now.tv_sec - sys_tz.tz_minuteswest * 60), 0, &date);

	snprintf(mc->log_path, MAX_MIF_LOG_PATH_LEN,
		"%s/%s_mif_log.%ld%02d%02d.%02d%02d%02d.txt",
		MIF_LOG_DIR, mc->name,
		(1900 + date.tm_year), (1 + date.tm_mon), date.tm_mday,
		date.tm_hour, date.tm_min, date.tm_sec);

	old_fs = get_fs();
	set_fs(get_ds());
	mc->log_fp = filp_open(mc->log_path, O_RDWR|O_CREAT, 0666);
	set_fs(old_fs);
	if (IS_ERR(mc->log_fp)) {
		printk(KERN_ERR "<%s:%s> %s open fail\n",
			__func__, mc->name, mc->log_path);
		mc->log_fp = NULL;
		mc->fs_failed = true;
		return -ENOENT;
	}

	atomic_set(&mc->log_open, 1);

	mif_err("open %s\n", mc->log_path);

	return 0;
}

void mif_close_log_file(struct modem_ctl *mc)
{
	mm_segment_t old_fs;

	if (!mc || !mc->use_mif_log || !mc->log_level || mc->fs_failed ||
	    !mc->fs_ready || !mc->log_fp)
		return;

	atomic_set(&mc->log_open, 0);

	flush_work_sync(&mc->evt_work);

	mif_err("close %s\n", mc->log_path);

	mif_save_logs(mc);

	old_fs = get_fs();
	set_fs(get_ds());
	filp_close(mc->log_fp, NULL);
	set_fs(old_fs);

	mc->log_fp = NULL;
}

