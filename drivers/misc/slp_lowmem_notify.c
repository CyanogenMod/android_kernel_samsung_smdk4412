/*
 *
 * Copyright (C) 2008-2009 Palm, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * [NOTICE]
 * modified by Baik - Samsung
 * this module must examine about license.
 * If SAMSUNG have a problem with the license, we must re-program module for
 * low memory notification.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/hugetlb.h>

#include <linux/sched.h>

#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/shmem_fs.h>

#include <asm/atomic.h>

#include <linux/slp_lowmem_notify.h>

#define MB(pages) ((K(pages))/1024)
#define K(pages) ((pages) << (PAGE_SHIFT - 10))

struct memnotify_file_info {
	int last_threshold;
	struct file *file;
	unsigned int nr_events;
};

static DECLARE_WAIT_QUEUE_HEAD(memnotify_wait);
static atomic_t nr_watcher_task = ATOMIC_INIT(0);

#define NR_MEMNOTIFY_LEVEL 4

enum {
	THRESHOLD_NORMAL,
	THRESHOLD_LOW,
	THRESHOLD_CRITICAL,
	THRESHOLD_DEADLY,
};

static const char *_threshold_string[NR_MEMNOTIFY_LEVEL] = {
	"normal  ",
	"low     ",
	"critical",
	"deadly  ",
};

static const char *threshold_string(int threshold)
{
	if (threshold > ARRAY_SIZE(_threshold_string))
		return "";

	return _threshold_string[threshold];
}

static unsigned long memnotify_messages[NR_MEMNOTIFY_LEVEL] = {
	MEMNOTIFY_NORMAL,	/* The happy state */
	MEMNOTIFY_LOW,		/* Userspace drops uneeded memory */
	MEMNOTIFY_CRITICAL,	/* Userspace OOM Killer */
	MEMNOTIFY_DEADLY,	/* Critical point, kill victims by kernel */
};

static atomic_t memnotify_last_threshold = ATOMIC_INIT(THRESHOLD_NORMAL);

static size_t memnotify_enter_thresholds[NR_MEMNOTIFY_LEVEL] = {
	INT_MAX,
	25,
	20,
	19,
};

static size_t memnotify_leave_thresholds[NR_MEMNOTIFY_LEVEL] = {
	INT_MAX,
	26,
	26,
	26,
};

static inline unsigned long memnotify_get_reclaimable(void)
{
	return global_page_state(NR_ACTIVE_FILE) +
	    global_page_state(NR_INACTIVE_FILE) +
	    global_page_state(NR_SLAB_RECLAIMABLE);
}

static inline unsigned long memnotify_get_used(void)
{
	return totalram_pages - memnotify_get_reclaimable() -
	    global_page_state(NR_FREE_PAGES);
}

static inline unsigned long memnotify_get_total(void)
{
	return totalram_pages;
}

#define OOMADJ_OF(p)	(p->signal->oom_adj)

static int memnotify_victim_task(int force_kill, gfp_t gfp_mask)
{
	struct task_struct *p;
	struct task_struct *selected = NULL;
	int tasksize;
	int selected_tasksize = 0;
	int total_selected_tasksize = 0;
	int ret_pid = -1;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (!p->mm)
			continue;
		if (OOMADJ_OF(p) == OOM_DISABLE)	/* OOM_DISABLE skip */
			continue;
		if ((total_selected_tasksize * PAGE_SIZE) >
		    memnotify_leave_thresholds[THRESHOLD_LOW]
		    * 1024 * 1024) {	/* MB to byte */
			read_unlock(&tasklist_lock);
			return -1;
		}
		if (test_tsk_thread_flag(p, TIF_MEMDIE)) {
			total_selected_tasksize += get_mm_rss(p->mm);
			continue;
		}

		tasksize = get_mm_rss(p->mm);
		if (tasksize > 0 &&
		    !(p->state & TASK_UNINTERRUPTIBLE) &&
		    (selected == NULL || OOMADJ_OF(p) > OOMADJ_OF(selected)
		     || (OOMADJ_OF(p) == OOMADJ_OF(selected) &&
			 tasksize > selected_tasksize))) {
			selected = p;
			selected_tasksize = tasksize;
		}
	}
	read_unlock(&tasklist_lock);

	if (selected != NULL && force_kill) {
		pr_crit("[SELECTED] send sigkill to %d (%s),"
			" adj %d, size %lu Byte(s)\n",
			selected->pid, selected->comm,
			OOMADJ_OF(selected), selected_tasksize * PAGE_SIZE);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		force_sig(SIGKILL, selected);
		ret_pid = 0;
		/*
		 * Give "p" a good chance of killing itself before we
		 * retry to allocate memory unless "p" is current
		 */
		if (!test_thread_flag(TIF_MEMDIE) && (gfp_mask & __GFP_WAIT))
			schedule_timeout_uninterruptible(1);
	} else if (selected == NULL) {
		pr_crit("No Victim !!!\n");
	} else {		/* (selected != NULL && force_kill == 0) */
		pr_crit("Victim task is pid %d (%s),"
			" adj %d, size %lu Byte(s)\n",
			selected->pid,
			selected->comm,
			OOMADJ_OF(selected), selected_tasksize * PAGE_SIZE);
		ret_pid = selected->pid;
	}

	return ret_pid;
}

static struct task_rss_t {
	pid_t pid;
	char comm[TASK_COMM_LEN];
	long rss;
} task_rss[256];		/* array for tasks and rss info */
static int nr_tasks;		/* number of tasks to be stored */
static int check_peak;		/* flag to store the rss or not */
static long old_real_free;	/* last minimum of free memory size */

static void save_task_rss(void)
{
	struct task_struct *p;
	int i;

	nr_tasks = 0;
	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (!p->mm)
			continue;
		task_rss[nr_tasks].pid = p->pid;
		strncpy(task_rss[nr_tasks].comm, p->comm,
			sizeof(task_rss[nr_tasks].comm));
		task_rss[nr_tasks].rss = 0;

		/* If except pages in swap, use get_mm_rss() */
		for (i = 0; i < NR_MM_COUNTERS; i++)
			atomic_long_add(task_rss[nr_tasks].rss,
					&p->mm->rss_stat.count[i]);

		if ((++nr_tasks) > sizeof(task_rss))
			break;
	}
	read_unlock(&tasklist_lock);
}

static void memnotify_wakeup(int threshold)
{
	atomic_set(&memnotify_last_threshold, threshold);
	wake_up_interruptible_all(&memnotify_wait);
}

void memnotify_threshold(gfp_t gfp_mask)
{
	long reclaimable, real_free;
	int threshold;
	int last_threshold;
	int i;

	real_free = MB(global_page_state(NR_FREE_PAGES));
	reclaimable = MB(memnotify_get_reclaimable());
	threshold = THRESHOLD_NORMAL;
	last_threshold = atomic_read(&memnotify_last_threshold);

	/* we save the tasks and rss info when free memory size is minimum,
	 * which means total used memory is highest at that moment. */
	if (check_peak && (old_real_free > real_free)) {
		old_real_free = real_free;
		save_task_rss();
	}

	/* Obtain enter threshold level */
	for (i = (NR_MEMNOTIFY_LEVEL - 1); i >= 0; i--) {
		if (real_free < memnotify_enter_thresholds[i] &&
		    reclaimable < memnotify_enter_thresholds[i]) {
			threshold = i;
			break;
		}
	}
	/* return quickly when normal case */
	if (likely(threshold == THRESHOLD_NORMAL &&
		   last_threshold == THRESHOLD_NORMAL))
		return;

	/* Need to leave a threshold by a certain margin. */
	if (threshold < last_threshold) {
		int leave_threshold =
		    memnotify_leave_thresholds[last_threshold];

		if (real_free < leave_threshold &&
		    reclaimable < leave_threshold) {
			threshold = last_threshold;
		}
	}

	if (unlikely(threshold == THRESHOLD_DEADLY)) {
		pr_crit("<< System memory is VERY LOW - %ld MB, %d(%s)>>\n",
			(real_free + reclaimable), current->pid, current->comm);
		memnotify_victim_task(true, gfp_mask);
		return;
	} else if (last_threshold == threshold)
		return;

	/* Rate limited notification of threshold changes. */
	memnotify_wakeup(threshold);

}
EXPORT_SYMBOL(memnotify_threshold);

static int lowmemnotify_open(struct inode *inode, struct file *file)
{
	struct memnotify_file_info *info;
	int err = 0;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		err = -ENOMEM;
		goto out;
	}

	info->file = file;
	info->nr_events = 0;
	file->private_data = info;
	atomic_inc(&nr_watcher_task);
 out:
	return err;
}

static int lowmemnotify_release(struct inode *inode, struct file *file)
{
	struct memnotify_file_info *info = file->private_data;

	kfree(info);
	atomic_dec(&nr_watcher_task);
	return 0;
}

static unsigned int lowmemnotify_poll(struct file *file, poll_table * wait)
{
	unsigned int retval = 0;
	struct memnotify_file_info *info = file->private_data;
	int threshold;

	poll_wait(file, &memnotify_wait, wait);

	threshold = atomic_read(&memnotify_last_threshold);

	if (info->last_threshold != threshold) {
		info->last_threshold = threshold;
		retval = POLLIN;
		info->nr_events++;

		pr_info("%s (%d%%, Used %ldMB, Free %ldMB, Reclaimable"
			" %ldMB, %s)\n",
			__func__,
			(int)(MB(memnotify_get_used()) * 100
			      / MB(memnotify_get_total())),
			MB(memnotify_get_used()),
			MB(global_page_state(NR_FREE_PAGES)),
			MB(memnotify_get_reclaimable()),
			threshold_string(threshold));

	} else if (info->nr_events > 0)
		retval = POLLIN;

	return retval;
}

static ssize_t lowmemnotify_read(struct file *file,
				 char __user *buf, size_t count, loff_t *ppos)
{
	int threshold;
	unsigned long data;
	ssize_t ret = 0;
	struct memnotify_file_info *info = file->private_data;

	if (count < sizeof(unsigned long))
		return -EINVAL;

	threshold = atomic_read(&memnotify_last_threshold);
	data = memnotify_messages[threshold];

	ret = put_user(data, (unsigned long __user *)buf);
	if (0 == ret) {
		ret = sizeof(unsigned long);
		info->nr_events = 0;
	}
	return ret;
}

static ssize_t meminfo_show(struct class *class, struct class_attribute *attr,
			    char *buf)
{
	unsigned long used;
	unsigned long total_mem;
	int used_ratio;

	int last_threshold;

	int len = 0;
	int i;

	used = memnotify_get_used();
	total_mem = memnotify_get_total();

	last_threshold = atomic_read(&memnotify_last_threshold);

	used_ratio = used * 100 / total_mem;

	len += snprintf(buf + len, PAGE_SIZE,
			"Total RAM size: \t%15ld MB( %6ld kB)\n",
			MB(totalram_pages), K(totalram_pages));

	len += snprintf(buf + len, PAGE_SIZE,
			"Used (Mem+Swap): \t%15ld MB( %6ld kB)\n", MB(used),
			K(used));

	len +=
	    snprintf(buf + len, PAGE_SIZE,
		     "Used (Mem):  \t\t%15ld MB( %6ld kB)\n", MB(used),
		     K(used));

	len +=
	    snprintf(buf + len, PAGE_SIZE,
		     "Used (Swap): \t\t%15ld MB( %6ld kB)\n",
		     MB(total_swap_pages - nr_swap_pages),
		     K(total_swap_pages - nr_swap_pages));

	len += snprintf(buf + len, PAGE_SIZE,
			"Used Ratio: \t\t%15d  %%\n", used_ratio);

	len +=
	    snprintf(buf + len, PAGE_SIZE, "Mem Free:\t\t%15ld MB( %6ld kB)\n",
		     MB((long)(global_page_state(NR_FREE_PAGES))),
		     K((long)(global_page_state(NR_FREE_PAGES))));

	len += snprintf(buf + len, PAGE_SIZE,
			"Available (Free+Reclaimable):%10ld MB( %6ld kB)\n",
			MB((long)(total_mem - used)),
			K((long)(total_mem - used)));

	len += snprintf(buf + len, PAGE_SIZE,
			"Reserve Page:\t\t%15ld MB( %6ld kB)\n",
			MB((long)(totalreserve_pages)),
			K((long)(totalreserve_pages)));

	len += snprintf(buf + len, PAGE_SIZE,
			"Last Threshold:    \t%s\n",
			threshold_string(last_threshold));

	len += snprintf(buf + len, PAGE_SIZE,
			"Enter Thresholds: available\t    used\n");
	for (i = 0; i < NR_MEMNOTIFY_LEVEL; i++) {
		unsigned long limit = MB(total_mem) -
		    memnotify_enter_thresholds[i];
		long left = limit - MB(used);
		len += snprintf(buf + len, PAGE_SIZE,
				"%s:\t%4d MB, %15ld MB, Rem: %10ld MB\n",
				threshold_string(i),
				memnotify_enter_thresholds[i], limit, left);

	}
	len += snprintf(buf + len, PAGE_SIZE,
			"Leave Thresholds: available\t    used\n");
	for (i = 0; i < NR_MEMNOTIFY_LEVEL; i++) {
		unsigned long limit = MB(total_mem) -
		    memnotify_leave_thresholds[i];
		long left = limit - MB(used);
		len += snprintf(buf + len, PAGE_SIZE,
				"%s:\t%4d MB, %15ld MB, Rem: %10ld MB\n",
				threshold_string(i),
				memnotify_leave_thresholds[i], limit, left);
	}

	return len;
}

#define DEFAULT_MARGIN	5	/* MB */
static size_t leave_margin = DEFAULT_MARGIN;

static ssize_t leave_margin_store(struct class *class, struct class_attribute *attr,
			   const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;
	long val;
	ret = strict_strtol(buf, 10, &val);
	if (ret == 0) {
		ret = count;
		leave_margin = val;
		val += memnotify_enter_thresholds[THRESHOLD_LOW];
		memnotify_leave_thresholds[THRESHOLD_LOW] = val;
		memnotify_leave_thresholds[THRESHOLD_CRITICAL] = val;
		memnotify_leave_thresholds[THRESHOLD_DEADLY] = val;
	}
	return ret;
}

static ssize_t threshold_lv1_store(struct class *class, struct class_attribute *attr,
			    const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;
	long val;
	ret = strict_strtol(buf, 10, &val);
	if (ret == 0) {
		ret = count;
		memnotify_enter_thresholds[THRESHOLD_LOW] = val;
		/* set leave thresholds with the margin */
		val += leave_margin;
		memnotify_leave_thresholds[THRESHOLD_LOW] = val;
		memnotify_leave_thresholds[THRESHOLD_CRITICAL] = val;
		memnotify_leave_thresholds[THRESHOLD_DEADLY] = val;
	}
	return ret;
}

static ssize_t threshold_lv2_store(struct class *class, struct class_attribute *attr,
			    const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;
	long val;
	ret = strict_strtol(buf, 10, &val);
	if (ret == 0) {
		ret = count;
		memnotify_enter_thresholds[THRESHOLD_CRITICAL] = val;
	}
	return ret;
}

static ssize_t victim_task_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	int pid;
	int len = 0;
	int *intbuf;

	pid = memnotify_victim_task(false, 0);

	intbuf = (int *)buf;
	*intbuf = pid;
	len = sizeof(int);

	return len;
}

static ssize_t peak_rss_show(struct class *class,
			     struct class_attribute *attr, char *buf)
{
	int len = 0;
	int i;

	len += snprintf(buf + len, PAGE_SIZE, "  PID\tRSS(KB)\t NAME\n");
	len += snprintf(buf + len, PAGE_SIZE,
			"=================================\n");
	for (i = 0; i < nr_tasks; i++) {
		len += snprintf(buf + len, PAGE_SIZE, "%5d\t%6ld\t %s\n",
				task_rss[i].pid, K(task_rss[i].rss),
				task_rss[i].comm);
	}

	return (ssize_t) len;
}

static ssize_t peak_rss_store(struct class *class, struct class_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;
	long val;
	ret = strict_strtol(buf, 10, &val);
	if (ret == 0) {
		ret = count;
		if (val) {
			check_peak = 1;
			save_task_rss();
		} else {
			check_peak = 0;
			/* set initial free memory with total memory size */
			old_real_free = memnotify_get_total();
		}
	}
	return ret;
}

static CLASS_ATTR(meminfo, S_IRUGO, meminfo_show, NULL);
static CLASS_ATTR(threshold_lv1, S_IWUSR, NULL, threshold_lv1_store);
static CLASS_ATTR(threshold_lv2, S_IWUSR, NULL, threshold_lv2_store);
static CLASS_ATTR(leave_margin, S_IWUSR, NULL, leave_margin_store);
static CLASS_ATTR(victim_task, S_IRUGO | S_IWUSR, victim_task_show, NULL);
static CLASS_ATTR(check_peak_rss, S_IRUGO | S_IWUSR, peak_rss_show,
		  peak_rss_store);

static const struct file_operations memnotify_fops = {
	.open = lowmemnotify_open,
	.release = lowmemnotify_release,
	.read = lowmemnotify_read,
	.poll = lowmemnotify_poll,
};

static struct device *memnotify_device;
static struct class *memnotify_class;
static int memnotify_major = -1;

static int __init lowmemnotify_init(void)
{
	int err;

	pr_info("Low Memory Notify loaded\n");

	memnotify_enter_thresholds[0] = MB(totalram_pages);
	memnotify_leave_thresholds[0] = MB(totalram_pages);

	memnotify_major = register_chrdev(0, MEMNOTIFY_DEVICE, &memnotify_fops);
	if (memnotify_major < 0) {
		pr_err("Unable to get major number for memnotify dev\n");
		err = -EBUSY;
		goto error_create_chr_dev;
	}

	memnotify_class = class_create(THIS_MODULE, MEMNOTIFY_DEVICE);
	if (IS_ERR(memnotify_class)) {
		err = PTR_ERR(memnotify_class);
		goto error_class_create;
	}

	memnotify_device =
	    device_create(memnotify_class, NULL, MKDEV(memnotify_major, 0),
			  NULL, MEMNOTIFY_DEVICE);

	if (IS_ERR(memnotify_device)) {
		err = PTR_ERR(memnotify_device);
		goto error_create_class_dev;
	}

	err = class_create_file(memnotify_class, &class_attr_meminfo);
	if (err) {
		pr_err("%s: couldn't create meminfo.\n", __func__);
		goto error_create_meminfo_class_file;
	}

	err = class_create_file(memnotify_class, &class_attr_threshold_lv1);
	if (err) {
		pr_err("%s: couldn't create threshold level 1.\n", __func__);
		goto error_create_threshold_lv1_class_file;
	}

	err = class_create_file(memnotify_class, &class_attr_threshold_lv2);
	if (err) {
		pr_err("%s: couldn't create threshold level 2.\n", __func__);
		goto error_create_threshold_lv2_class_file;
	}

	err = class_create_file(memnotify_class, &class_attr_leave_margin);
	if (err) {
		pr_err("%s: couldn't create leave margin.\n", __func__);
		goto error_create_leave_margin_class_file;
	}

	err = class_create_file(memnotify_class, &class_attr_victim_task);
	if (err) {
		pr_err("%s: couldn't create victim sysfs file.\n", __func__);
		goto error_create_victim_task_class_file;
	}

	err = class_create_file(memnotify_class, &class_attr_check_peak_rss);
	if (err) {
		pr_err("%s: couldn't create peak rss sysfs file.\n", __func__);
		goto error_create_peak_rss_class_file;
	}

	/* set initial free memory with total memory size */
	old_real_free = memnotify_get_total();

	return 0;

 error_create_peak_rss_class_file:
	class_remove_file(memnotify_class, &class_attr_victim_task);
 error_create_victim_task_class_file:
	class_remove_file(memnotify_class, &class_attr_leave_margin);
 error_create_leave_margin_class_file:
	class_remove_file(memnotify_class, &class_attr_threshold_lv2);
 error_create_threshold_lv2_class_file:
	class_remove_file(memnotify_class, &class_attr_threshold_lv1);
 error_create_threshold_lv1_class_file:
	class_remove_file(memnotify_class, &class_attr_meminfo);
 error_create_meminfo_class_file:
	device_del(memnotify_device);
 error_create_class_dev:
	class_destroy(memnotify_class);
 error_class_create:
	unregister_chrdev(memnotify_major, MEMNOTIFY_DEVICE);
 error_create_chr_dev:
	return err;
}

static void __exit lowmemnotify_exit(void)
{
	if (memnotify_device)
		device_del(memnotify_device);
	if (memnotify_class)
		class_destroy(memnotify_class);
	if (memnotify_major >= 0)
		unregister_chrdev(memnotify_major, MEMNOTIFY_DEVICE);
}

module_init(lowmemnotify_init);
module_exit(lowmemnotify_exit);

MODULE_LICENSE("GPL");
