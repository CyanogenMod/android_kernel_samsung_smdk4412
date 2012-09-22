/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_adj values will get killed. Specify the
 * minimum oom_adj values in /sys/module/lowmemorykiller/parameters/adj and the
 * number of free pages in /sys/module/lowmemorykiller/parameters/minfree. Both
 * files take a comma separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill processes
 * with a oom_adj value of 8 or higher when the free memory drops below 4096 pages
 * and kill processes with a oom_adj value of 0 or higher when the free memory
 * drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#ifdef CONFIG_ZRAM_FOR_ANDROID
#include <linux/swap.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mm_inline.h>
#endif /* CONFIG_ZRAM_FOR_ANDROID */
#define ENHANCED_LMK_ROUTINE

#ifdef ENHANCED_LMK_ROUTINE
#define LOWMEM_DEATHPENDING_DEPTH 3
#endif

static uint32_t lowmem_debug_level = 2;
static int lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};
static int lowmem_adj_size = 4;
static size_t lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};
static int lowmem_minfree_size = 4;
#ifdef CONFIG_ZRAM_FOR_ANDROID
static struct class *lmk_class;
static struct device *lmk_dev;
static int lmk_kill_pid = 0;
static int lmk_kill_ok = 0;

extern atomic_t optimize_comp_on;

extern int isolate_lru_page_compcache(struct page *page);
extern void putback_lru_page(struct page *page);
extern unsigned int zone_id_shrink_pagelist(struct zone *zone_id,struct list_head *page_list);

#define lru_to_page(_head) (list_entry((_head)->prev, struct page, lru))

#define SWAP_PROCESS_DEBUG_LOG 0
/* free RAM 8M(2048 pages) */
#define CHECK_FREE_MEMORY 2048
/* free swap (10240 pages) */
#define CHECK_FREE_SWAPSPACE  10240

static unsigned int check_free_memory = 0;

enum pageout_io {
	PAGEOUT_IO_ASYNC,
	PAGEOUT_IO_SYNC,
};


#endif /* CONFIG_ZRAM_FOR_ANDROID */

#ifdef ENHANCED_LMK_ROUTINE
static struct task_struct *lowmem_deathpending[LOWMEM_DEATHPENDING_DEPTH] = {NULL,};
#else
static struct task_struct *lowmem_deathpending;
#endif
static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			printk(x);			\
	} while (0)

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data);

static struct notifier_block task_nb = {
	.notifier_call	= task_notify_func,
};

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data)
{
	struct task_struct *task = data;

#ifdef ENHANCED_LMK_ROUTINE
	int i = 0;
	for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++)
		if (task == lowmem_deathpending[i]) {
			lowmem_deathpending[i] = NULL;
		break;
	}
#else
	if (task == lowmem_deathpending)
		lowmem_deathpending = NULL;
#endif
	return NOTIFY_OK;
}

static int lowmem_shrink(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *p;
#ifdef ENHANCED_LMK_ROUTINE
	struct task_struct *selected[LOWMEM_DEATHPENDING_DEPTH] = {NULL,};
#else
	struct task_struct *selected = NULL;
#endif
	int rem = 0;
	int tasksize;
	int i;
	int min_adj = OOM_ADJUST_MAX + 1;
#ifdef ENHANCED_LMK_ROUTINE
	int selected_tasksize[LOWMEM_DEATHPENDING_DEPTH] = {0,};
	int selected_oom_adj[LOWMEM_DEATHPENDING_DEPTH] = {OOM_ADJUST_MAX,};
	int all_selected_oom = 0;
	int max_selected_oom_idx = 0;
#else
	int selected_tasksize = 0;
	int selected_oom_adj;
#endif
	int array_size = ARRAY_SIZE(lowmem_adj);
#ifndef CONFIG_DMA_CMA
	int other_free = global_page_state(NR_FREE_PAGES);
#else
	int other_free = global_page_state(NR_FREE_PAGES) -
					global_page_state(NR_FREE_CMA_PAGES);
#endif
	int other_file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);

	/*
	 * If we already have a death outstanding, then
	 * bail out right away; indicating to vmscan
	 * that we have nothing further to offer on
	 * this pass.
	 *
	 */
#ifdef ENHANCED_LMK_ROUTINE
	for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++) {
		if (lowmem_deathpending[i] &&
			time_before_eq(jiffies, lowmem_deathpending_timeout))
			return 0;
	}
#else
	if (lowmem_deathpending &&
	    time_before_eq(jiffies, lowmem_deathpending_timeout))
		return 0;
#endif

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
		if (other_free < lowmem_minfree[i] &&
		    other_file < lowmem_minfree[i]) {
			min_adj = lowmem_adj[i];
			break;
		}
	}
	if (sc->nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %lu, %x, ofree %d %d, ma %d\n",
			     sc->nr_to_scan, sc->gfp_mask, other_free, other_file,
			     min_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (sc->nr_to_scan <= 0 || min_adj == OOM_ADJUST_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %lu, %x, return %d\n",
			     sc->nr_to_scan, sc->gfp_mask, rem);
		return rem;
	}

#ifdef ENHANCED_LMK_ROUTINE
	for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++)
		selected_oom_adj[i] = min_adj;
#else
	selected_oom_adj = min_adj;
#endif

	read_lock(&tasklist_lock);
	for_each_process(p) {
		struct mm_struct *mm;
		struct signal_struct *sig;
		int oom_adj;
#ifdef ENHANCED_LMK_ROUTINE
		int is_exist_oom_task = 0;
#endif
		task_lock(p);
		mm = p->mm;
		sig = p->signal;
		if (!mm || !sig) {
			task_unlock(p);
			continue;
		}
		oom_adj = sig->oom_adj;
		if (oom_adj < min_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;

#ifdef ENHANCED_LMK_ROUTINE
		if (all_selected_oom < LOWMEM_DEATHPENDING_DEPTH) {
			for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++) {
				if (!selected[i]) {
					is_exist_oom_task = 1;
					max_selected_oom_idx = i;
					break;
				}
			}
		} else if (selected_oom_adj[max_selected_oom_idx] < oom_adj ||
			(selected_oom_adj[max_selected_oom_idx] == oom_adj &&
			selected_tasksize[max_selected_oom_idx] < tasksize)) {
			is_exist_oom_task = 1;
		}

		if (is_exist_oom_task) {
			selected[max_selected_oom_idx] = p;
			selected_tasksize[max_selected_oom_idx] = tasksize;
			selected_oom_adj[max_selected_oom_idx] = oom_adj;

			if (all_selected_oom < LOWMEM_DEATHPENDING_DEPTH)
				all_selected_oom++;

			if (all_selected_oom == LOWMEM_DEATHPENDING_DEPTH) {
				for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++) {
					if (selected_oom_adj[i] < selected_oom_adj[max_selected_oom_idx])
						max_selected_oom_idx = i;
					else if (selected_oom_adj[i] == selected_oom_adj[max_selected_oom_idx] &&
						selected_tasksize[i] < selected_tasksize[max_selected_oom_idx])
						max_selected_oom_idx = i;
				}
			}

			lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
				p->pid, p->comm, oom_adj, tasksize);
		}
#else
		if (selected) {
			if (oom_adj < selected_oom_adj)
				continue;
			if (oom_adj == selected_oom_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_adj = oom_adj;
		lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
			     p->pid, p->comm, oom_adj, tasksize);
#endif
	}
#ifdef ENHANCED_LMK_ROUTINE
	for (i = 0; i < LOWMEM_DEATHPENDING_DEPTH; i++) {
		if (selected[i]) {
			lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d\n",
				selected[i]->pid, selected[i]->comm,
				selected_oom_adj[i], selected_tasksize[i]);
			lowmem_deathpending[i] = selected[i];
			lowmem_deathpending_timeout = jiffies + HZ;
			force_sig(SIGKILL, selected[i]);
			rem -= selected_tasksize[i];
		}
	}
#else
	if (selected) {
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d\n",
			     selected->pid, selected->comm,
			     selected_oom_adj, selected_tasksize);
		lowmem_deathpending = selected;
		lowmem_deathpending_timeout = jiffies + HZ;
		force_sig(SIGKILL, selected);
		rem -= selected_tasksize;
	}
#endif
	lowmem_print(4, "lowmem_shrink %lu, %x, return %d\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);
	read_unlock(&tasklist_lock);
	return rem;
}

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};

#ifdef CONFIG_ZRAM_FOR_ANDROID
/*
 * zone_id_shrink_pagelist() clear page flags,
 * update the memory zone status, and swap pagelist
 */

static unsigned int shrink_pages(struct mm_struct *mm,
				 struct list_head *zone0_page_list,
				 struct list_head *zone1_page_list,
				 unsigned int num_to_scan)
{
	unsigned long addr;
	unsigned int isolate_pages_countter = 0;

	struct vm_area_struct *vma = mm->mmap;
	while (vma != NULL) {

		for (addr = vma->vm_start; addr < vma->vm_end;
		     addr += PAGE_SIZE) {
			struct page *page;
			/*get the page address from virtual memory address */
			page = follow_page(vma, addr, FOLL_GET);

			if (page && !IS_ERR(page)) {

				put_page(page);
				/* only moveable, anonymous and not dirty pages can be swapped  */
				if ((!PageUnevictable(page))
				    && (!PageDirty(page)) && ((PageAnon(page)))
				    && (0 == page_is_file_cache(page))) {
					switch (page_zone_id(page)) {
					case 0:
						if (!isolate_lru_page_compcache(page)) {
							/* isolate page from LRU and add to temp list  */
							/*create new page list, it will be used in shrink_page_list */
							list_add_tail(&page->lru, zone0_page_list);
							isolate_pages_countter++;
						}
						break;
					case 1:
						if (!isolate_lru_page_compcache(page)) {
							/* isolate page from LRU and add to temp list  */
							/*create new page list, it will be used in shrink_page_list */
							list_add_tail(&page->lru, zone1_page_list);
							isolate_pages_countter++;
						}
						break;
					default:
						break;
					}
				}
			}

			if (isolate_pages_countter >= num_to_scan) {
				return isolate_pages_countter;
			}
		}

		vma = vma->vm_next;
	}

	return isolate_pages_countter;
}

/*
 * swap_application_pages() will search the
 * pages which can be swapped, then call
 * zone_id_shrink_pagelist to update zone
 * status
 */
static unsigned int swap_pages(struct list_head *zone0_page_list,
			       struct list_head *zone1_page_list)
{
	struct zone *zone_id_0 = &NODE_DATA(0)->node_zones[0];
	struct zone *zone_id_1 = &NODE_DATA(0)->node_zones[1];
	unsigned int pages_counter = 0;

	/*if the page list is not empty, call zone_id_shrink_pagelist to update zone status */
	if ((zone_id_0) && (!list_empty(zone0_page_list))) {
		pages_counter +=
		    zone_id_shrink_pagelist(zone_id_0, zone0_page_list);
	}
	if ((zone_id_1) && (!list_empty(zone1_page_list))) {
		pages_counter +=
		    zone_id_shrink_pagelist(zone_id_1, zone1_page_list);
	}
	return pages_counter;
}

static ssize_t lmk_state_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d\n", lmk_kill_pid, lmk_kill_ok);
}

/*
 * lmk_state_store() will called by framework,
 * the framework will send the pid of process that need to be swapped
 */
static ssize_t lmk_state_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	sscanf(buf, "%d,%d", &lmk_kill_pid, &lmk_kill_ok);

	/* if the screen on, the optimized compcache will stop */
	if (atomic_read(&optimize_comp_on) != 1)
		return size;

	if (lmk_kill_ok == 1) {
		struct task_struct *p;
		struct task_struct *selected = NULL;
		struct sysinfo ramzswap_info = { 0 };
		struct mm_struct *mm_scan = NULL;

		/*
		 * check the free RAM and swap area,
		 * stop the optimized compcache in cpu idle case;
		 * leave some swap area for using in low memory case
		 */
		si_swapinfo(&ramzswap_info);
		si_meminfo(&ramzswap_info);

		if ((ramzswap_info.freeswap < CHECK_FREE_SWAPSPACE) ||
		    (ramzswap_info.freeram < check_free_memory)) {
#if SWAP_PROCESS_DEBUG_LOG > 0
			printk(KERN_INFO "idletime compcache is ignored : free RAM %lu, free swap %lu\n",
			ramzswap_info.freeram, ramzswap_info.freeswap);
#endif
			lmk_kill_ok = 0;
			return size;
		}

		read_lock(&tasklist_lock);
		for_each_process(p) {
			if ((p->pid == lmk_kill_pid) &&
			    (__task_cred(p)->uid > 10000)) {
				task_lock(p);
				selected = p;
				if (!selected->mm || !selected->signal) {
					task_unlock(p);
					selected = NULL;
					break;
				}
				mm_scan = selected->mm;
				if (mm_scan) {
					if (selected->flags & PF_KTHREAD)
						mm_scan = NULL;
					else
						atomic_inc(&mm_scan->mm_users);
				}
				task_unlock(selected);

#if SWAP_PROCESS_DEBUG_LOG > 0
				printk(KERN_INFO "idle time compcache: swap process pid %d, name %s, oom %d, task size %ld\n",
					p->pid, p->comm,
					p->signal->oom_adj,
					get_mm_rss(p->mm));
#endif
				break;
			}
		}
		read_unlock(&tasklist_lock);

		if (mm_scan) {
			LIST_HEAD(zone0_page_list);
			LIST_HEAD(zone1_page_list);
			int pages_tofree = 0, pages_freed = 0;

			down_read(&mm_scan->mmap_sem);
			pages_tofree =
			shrink_pages(mm_scan, &zone0_page_list,
					&zone1_page_list, 0x7FFFFFFF);
			up_read(&mm_scan->mmap_sem);
			mmput(mm_scan);
			pages_freed =
			    swap_pages(&zone0_page_list,
				       &zone1_page_list);
			lmk_kill_ok = 0;

		}
	}

	return size;
}

static DEVICE_ATTR(lmk_state, 0664, lmk_state_show, lmk_state_store);

#endif /* CONFIG_ZRAM_FOR_ANDROID */

static int __init lowmem_init(void)
{
#ifdef CONFIG_ZRAM_FOR_ANDROID
	struct zone *zone;
	unsigned int high_wmark = 0;
#endif
	task_free_register(&task_nb);
	register_shrinker(&lowmem_shrinker);

#ifdef CONFIG_ZRAM_FOR_ANDROID
	for_each_zone(zone) {
		if (high_wmark < zone->watermark[WMARK_HIGH])
			high_wmark = zone->watermark[WMARK_HIGH];
	}
	check_free_memory = (high_wmark != 0) ? high_wmark : CHECK_FREE_MEMORY;

	lmk_class = class_create(THIS_MODULE, "lmk");
	if (IS_ERR(lmk_class)) {
		printk(KERN_ERR "Failed to create class(lmk)\n");
		return 0;
	}
	lmk_dev = device_create(lmk_class, NULL, 0, NULL, "lowmemorykiller");
	if (IS_ERR(lmk_dev)) {
		printk(KERN_ERR
		       "Failed to create device(lowmemorykiller)!= %ld\n",
		       IS_ERR(lmk_dev));
		return 0;
	}
	if (device_create_file(lmk_dev, &dev_attr_lmk_state) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_lmk_state.attr.name);
#endif /* CONFIG_ZRAM_FOR_ANDROID */

	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
	task_free_unregister(&task_nb);
}

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

