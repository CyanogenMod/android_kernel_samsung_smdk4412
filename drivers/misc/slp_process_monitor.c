
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/thread_notify.h>

#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>

#define PMON_DEVICE "pmon"

struct _process_mon_data {
	int major;
	int initialized;
	struct class *cls;
	struct device *dev;

	struct list_head mp_list;	/* monitor process list */
	struct list_head dp_list;	/* dead process list  */
	struct work_struct pmon_work;

	int watcher_pid;
};

enum mp_entry_type {
	MP_VIP,
	MP_PERMANENT,
	MP_NONE
};

struct mp_entry {
	struct list_head list;
	enum mp_entry_type type;
	pid_t pid;
};

struct dp_entry {
	struct list_head list;
	pid_t pid;
};

static struct _process_mon_data pmon_data = {
	.initialized = 0,
	.watcher_pid = 0
};

static DEFINE_SPINLOCK(mp_list_lock);
static DEFINE_SPINLOCK(dp_list_lock);
static DECLARE_WAIT_QUEUE_HEAD(pmon_wait);
static atomic_t nr_watcher_task = ATOMIC_INIT(0);

static int pmon_open(struct inode *inode, struct file *file)
{
	int nr_read_task;

	/* only 1 process can do "read open" */
	if ((file->f_flags & O_ACCMODE) != O_WRONLY) {
		nr_read_task = atomic_read(&nr_watcher_task);
		if (nr_read_task > 0)
			return -EACCES;
		else {
			atomic_inc(&nr_watcher_task);
			pmon_data.watcher_pid = get_current()->pid;
			pr_info("add process monitor task = %d\n",
				pmon_data.watcher_pid);
		}
	}

	return 0;
}

static int pmon_release(struct inode *inode, struct file *file)
{
	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		atomic_dec(&nr_watcher_task);

	return 0;
}

static ssize_t pmon_read(struct file *file, char __user *buf, size_t count,
			 loff_t *ppos)
{
	struct dp_entry *dp;
	ssize_t ret = 0;

	spin_lock(&dp_list_lock);

	if (!list_empty(&pmon_data.dp_list)) {
		dp = list_first_entry(&pmon_data.dp_list, struct dp_entry,
				      list);
		if (copy_to_user(buf, &(dp->pid), sizeof(pid_t))) {
			spin_unlock(&dp_list_lock);
			return -EFAULT;
		}
		ret = sizeof(pid_t);
		list_del(&dp->list);
		kfree(dp);
	}

	spin_unlock(&dp_list_lock);

	return ret;
}

static unsigned int pmon_poll(struct file *file, poll_table *wait)
{
	unsigned int retval = 0;
	poll_wait(file, &pmon_wait, wait);
	spin_lock(&dp_list_lock);
	if (!list_empty(&pmon_data.dp_list))
		retval = POLLIN;
	spin_unlock(&dp_list_lock);

	return retval;
}

static int mp_store(const char *buf, enum mp_entry_type type, size_t count)
{
	struct mp_entry *new_mp;
	int *pid;
	int ret = 0;

	pid = (int *)buf;
	pr_debug("monitor process - %d : %d\n", *pid, type);

	spin_lock(&mp_list_lock);
	list_for_each_entry(new_mp, &pmon_data.mp_list, list) {
		if (new_mp->pid == *pid) {
			pr_info("Already exist ! pid: %d\n", *pid);
			ret = -1;
			break;
		}
	}
	spin_unlock(&mp_list_lock);
	if (ret == -1)
		return count;

	new_mp = NULL;
	new_mp = kmalloc(sizeof(struct mp_entry), GFP_KERNEL);
	if (!new_mp)
		return -ENOMEM;
	new_mp->pid = *pid;
	new_mp->type = type;

	spin_lock(&mp_list_lock);
	list_add_tail(&new_mp->list, &pmon_data.mp_list);
	spin_unlock(&mp_list_lock);

	return count;
}

static ssize_t mp_remove(struct class *class, struct class_attribute *attr,
			 const char *buf, size_t count)
{
	struct mp_entry *rm_mp, *next;
	int pid, ret = -1;
	int val;

	if (buf == NULL)
		return -1;
	if (!kstrtoint(buf, 0, &val))
			pid = val;

	spin_lock(&mp_list_lock);
	list_for_each_entry_safe(rm_mp, next, &pmon_data.mp_list, list) {
		if (rm_mp->pid == pid) {
			list_del(&rm_mp->list);
			kfree(rm_mp);
			pr_debug("remove the monitoring process - %d\n", pid);
			ret = 0;
			break;
		}
	}
	spin_unlock(&mp_list_lock);

	if (ret == -1)
		pr_info("No precess to be removed - %d\n", pid);

	return count;
}

static ssize_t mp_vip_store(struct class *class, struct class_attribute *attr,
			    const char *buf, size_t count)
{
	return mp_store(buf, MP_VIP, count);
}

static ssize_t mp_pnp_store(struct class *class, struct class_attribute *attr,
			    const char *buf, size_t count)
{
	return mp_store(buf, MP_PERMANENT, count);
}

static void pmon_work_thread(struct work_struct *work)
{
	if (pmon_data.watcher_pid)
		wake_up_interruptible_all(&pmon_wait);
	else
		kobject_uevent(&pmon_data.dev->kobj, KOBJ_CHANGE);
}

static int check_pernament_process(int pid, char *tsk_name)
{
	struct dp_entry *new_dp;
	struct mp_entry *mp, *next;
	pid_t found_pid = 0;
	enum mp_entry_type mtype = MP_NONE;

	if (!pmon_data.initialized)
		return -1;

	spin_lock(&mp_list_lock);
	list_for_each_entry_safe(mp, next, &pmon_data.mp_list, list) {
		if (mp->pid == pid) {
			found_pid = mp->pid;
			mtype = mp->type;
			list_del(&mp->list);
			kfree(mp);
			break;
		}
	}
	spin_unlock(&mp_list_lock);

	if (pmon_data.watcher_pid == pid || mtype == MP_VIP) {
		if (pmon_data.watcher_pid == pid)
			pr_info("<<< process monitor process dead: %d (%s)\n",
				pid, tsk_name);
		else
			pr_info("<<< VIP process dead: %d (%s)>>>\n", pid,
				tsk_name);
		pmon_data.watcher_pid = 0;
		schedule_work(&pmon_data.pmon_work);
		return 0;
	}

	if (found_pid) {
		new_dp = kmalloc(sizeof(struct dp_entry), GFP_ATOMIC);
		if (!new_dp) {
			pr_err("Not enough memory\n");
			return -1;	/* TODO - must do retry */
		}
		new_dp->pid = found_pid;
		spin_lock(&dp_list_lock);
		list_add_tail(&new_dp->list, &pmon_data.dp_list);
		spin_unlock(&dp_list_lock);

		schedule_work(&pmon_data.pmon_work);
	}

	return 0;
}

static int process_mon_do(struct notifier_block *self, unsigned long cmd,
			  void *t)
{
	struct thread_info *thread;

	switch (cmd) {
	case THREAD_NOTIFY_FLUSH:
	case THREAD_NOTIFY_SWITCH:
		break;
	case THREAD_NOTIFY_EXIT:
		thread = (struct thread_info *)t;
		check_pernament_process(thread->task->pid, thread->task->comm);
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block process_mon_nb = {
	.notifier_call = process_mon_do,
};

static ssize_t mp_show(char *buf, enum mp_entry_type type)
{
	int len = 0;
	struct mp_entry *mp;

	if (!pmon_data.initialized)
		return -1;

	spin_lock(&mp_list_lock);
	list_for_each_entry(mp, &pmon_data.mp_list, list) {
		if (mp->type == type)
			len += snprintf(buf + len, PAGE_SIZE,
					"%d ", mp->pid);
	}
	spin_unlock(&mp_list_lock);

	len += snprintf(buf + len, PAGE_SIZE, "\n");

	return len;
}

static ssize_t mp_vip_show(struct class *class, struct class_attribute *attr,
			   char *buf)
{
	return mp_show(buf, MP_VIP);
}

static ssize_t mp_pnp_show(struct class *class, struct class_attribute *attr,
			   char *buf)
{
	return mp_show(buf, MP_PERMANENT);
}

static CLASS_ATTR(rm_pmon, S_IWUGO, NULL, mp_remove);
static CLASS_ATTR(mp_vip, 0644, mp_vip_show, mp_vip_store);
static CLASS_ATTR(mp_pnp, 0644, mp_pnp_show, mp_pnp_store);

static const struct file_operations pmon_fops = {
	.open = pmon_open,
	.release = pmon_release,
	.read = pmon_read,
	.poll = pmon_poll,
};

static int __init process_mon_init(void)
{
	int err = 0;

	pmon_data.major = register_chrdev(0, PMON_DEVICE, &pmon_fops);
	if (pmon_data.major < 0) {
		pr_err("Unable to get major number for pmon dev\n");
		err = -EBUSY;
		goto error_create_chr_dev;
	}

	pmon_data.cls = class_create(THIS_MODULE, PMON_DEVICE);
	if (IS_ERR(pmon_data.cls)) {
		pr_err("class create err\n");
		err = PTR_ERR(pmon_data.cls);
		goto error_class_create;
	}

	pmon_data.dev = device_create(pmon_data.cls, NULL,
				      MKDEV(pmon_data.major, 0), NULL,
				      PMON_DEVICE);

	if (IS_ERR(pmon_data.dev)) {
		pr_err("device create err\n");
		err = PTR_ERR(pmon_data.dev);
		goto error_create_class_dev;
	}

	err = class_create_file(pmon_data.cls, &class_attr_rm_pmon);
	if (err) {
		pr_err("%s: couldn't create meminfo.\n", __func__);
		goto error_create_class_file_pmoninfo;
	}

	err = class_create_file(pmon_data.cls, &class_attr_mp_vip);
	if (err) {
		pr_err("%s: couldn't create meminfo.\n", __func__);
		goto error_create_class_file_mp_vip;
	}
	err = class_create_file(pmon_data.cls, &class_attr_mp_pnp);
	if (err) {
		pr_err("%s: couldn't create meminfo.\n", __func__);
		goto error_create_class_file_mp_pnp;
	}

	INIT_LIST_HEAD(&pmon_data.mp_list);
	INIT_LIST_HEAD(&pmon_data.dp_list);

	INIT_WORK(&pmon_data.pmon_work, pmon_work_thread);
	thread_register_notifier(&process_mon_nb);
	pmon_data.initialized = 1;

	return 0;

error_create_class_file_mp_pnp:
	class_remove_file(pmon_data.cls, &class_attr_mp_vip);
error_create_class_file_mp_vip:
	class_remove_file(pmon_data.cls, &class_attr_rm_pmon);
error_create_class_file_pmoninfo:
	device_del(pmon_data.dev);
error_create_class_dev:
	class_destroy(pmon_data.cls);
error_class_create:
	unregister_chrdev(pmon_data.major, PMON_DEVICE);
error_create_chr_dev:
	return err;
}

static void __exit process_mon_exit(void)
{
	thread_unregister_notifier(&process_mon_nb);
}

module_init(process_mon_init);
module_exit(process_mon_exit);

MODULE_AUTHOR("baik");
MODULE_DESCRIPTION("SLP Process Monitoring driver");
MODULE_LICENSE("GPL");
