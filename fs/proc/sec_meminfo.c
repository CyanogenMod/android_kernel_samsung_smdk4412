#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <asm/atomic.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "internal.h"
#include <asm/sections.h>
#include <linux/sec_meminfo.h>

enum alloc_type {
	MALI_TYPE = 0,
	UMP_TYPE,
	PGD_TYPE,
	MAX_TYPE
};

struct alloc_item {
	int total_cnt;
	int low_cnt;
};

static struct alloc_item alloc_cnt[MAX_TYPE];

static inline int check_lowmem_page(struct page *page)
{
	if (page_zonenum(page) == ZONE_HIGHMEM)
		return 0;
	return 1;
}

void sec_meminfo_set_alloc_cnt(int alloc_type, int is_alloc, struct page *page)
{
	if (is_alloc) {
		alloc_cnt[alloc_type].total_cnt++;
		if (page != NULL && check_lowmem_page(page))
			alloc_cnt[alloc_type].low_cnt++;
	} else {
		alloc_cnt[alloc_type].total_cnt--;
		if (page != NULL && check_lowmem_page(page))
			alloc_cnt[alloc_type].low_cnt--;
	}
}
EXPORT_SYMBOL(sec_meminfo_set_alloc_cnt);

int sec_meminfo_get_alloc_total_cnt(int alloc_type)
{
	return alloc_cnt[alloc_type].total_cnt;
}

int sec_meminfo_get_alloc_low_cnt(int alloc_type)
{
	return alloc_cnt[alloc_type].low_cnt;
}

static int sec_meminfo_proc_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	struct vmalloc_info vmi;
	long cached;
	unsigned long ktotal, htotal;
	struct zone *r_zone = NULL;
	struct zone *h_zone = NULL;
	struct zone *zone = NULL;

/*
 * display in kilobytes.
 */
#define K(x) ((x) << (PAGE_SHIFT - 10))
	get_vmalloc_info(&vmi);
	si_meminfo(&i);
	si_swapinfo(&i);

	cached = global_page_state(NR_FILE_PAGES) -
			total_swapcache_pages - i.bufferram;

	if (cached < 0)
		cached = 0;

	for_each_populated_zone(zone) {
		r_zone = zone;
		if (strcmp(r_zone->name, "Normal") == 0)
			break;
	}

	for_each_populated_zone(zone) {
		h_zone = zone;
		if (strcmp(h_zone->name, "High") == 0)
			break;
	}

	ktotal = K(global_page_state(NR_SLAB_RECLAIMABLE) \
		+global_page_state(NR_SLAB_UNRECLAIMABLE)) \
		+(global_page_state(NR_KERNEL_STACK) * THREAD_SIZE / 1024) \
		+K(global_page_state(NR_PAGETABLE)) \
		+(zone_page_state(r_zone, NR_FILE_PAGES)*4) \
		+(zone_page_state(r_zone, NR_ANON_PAGES)*4) \
		+(vmi.low_page_cnt*4) \
		+(sec_meminfo_get_alloc_total_cnt(PGD_TYPE)*16) \
		+DIV_ROUND_UP(((__init_end) - (__init_begin)), SZ_1K) \
		+DIV_ROUND_UP(((_etext) - (_text)), SZ_1K) \
		+DIV_ROUND_UP(((_edata) - (_sdata)), SZ_1K) \
		+DIV_ROUND_UP(((__bss_stop) - (__bss_start)), SZ_1K);

#ifdef CONFIG_VIDEO_MALI400MP
	ktotal += (sec_meminfo_get_alloc_low_cnt(MALI_TYPE)*4);
#endif
#ifdef CONFIG_VIDEO_UMP
	ktotal += (sec_meminfo_get_alloc_low_cnt(UMP_TYPE)*4);
#endif

	htotal = (zone_page_state(h_zone, NR_FILE_PAGES)*4) \
		+(zone_page_state(h_zone, NR_ANON_PAGES)*4) \
		+((vmi.total_page_cnt-vmi.low_page_cnt)*4);

#ifdef CONFIG_VIDEO_MALI400MP
	htotal += ((sec_meminfo_get_alloc_total_cnt(MALI_TYPE) - sec_meminfo_get_alloc_low_cnt(MALI_TYPE))*4);
#endif
#ifdef CONFIG_VIDEO_UMP
	htotal += ((sec_meminfo_get_alloc_total_cnt(UMP_TYPE) - sec_meminfo_get_alloc_low_cnt(UMP_TYPE))*4);
#endif

	seq_printf(m,
		"LowMemTotal:	%8lu kB\n"
		"LowMemFree:	%8lu kB\n"
		"LowMemUsed:	%8lu kB\n"
		"-KnownTotal:	%8lu kB\n"
		"-Etc:		%8ld kB\n"
		"Slab:		%8lu kB\n"
		"-SReclaimable:	%8lu kB\n"
		"-SUnreclaim:	%8lu kB\n"
		"KernelStack:	%8lu kB\n"
		"PageTables:	%8lu kB\n"
		"File Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"--Mapped:	%8lu kB\n"
		"--Shmem:	%8lu kB\n"
		"Anon Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"Vmalloc:	%8lu kB\n"
		"PGD:		%8d kB\n"
		".init:		%8d kB\n"
		".text:		%8d kB\n"
		".data:		%8d kB\n"
		".bss:		%8d kB\n"
	#ifdef CONFIG_VIDEO_MALI400MP
		"Mali:		%8d kB\n"
	#endif
	#ifdef CONFIG_VIDEO_UMP
		"UMP:		%8d kB\n"
	#endif
		"\n"
		"HighMemTotal:	%8lu kB\n"
		"HighMemFree:	%8lu kB\n"
		"HighMemUsed:	%8lu kB\n"
		"-KnownTotal:	%8lu kB\n"
		"-Etc:		%8ld kB\n"
		"File Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"--Mapped:	%8lu kB\n"
		"--Shmem:	%8lu kB\n"
		"Anon Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"Vmalloc:	%8lu kB\n"
	#ifdef CONFIG_VIDEO_MALI400MP
		"Mali:		%8d kB\n"
	#endif
	#ifdef CONFIG_VIDEO_UMP
		"UMP:		%8d kB\n\n"
	#endif
		,
		K(i.totalram-i.totalhigh),
		K(i.freeram-i.freehigh),
		K((i.totalram-i.totalhigh)-(i.freeram-i.freehigh)),
		ktotal,
		K((i.totalram-i.totalhigh)-(i.freeram-i.freehigh))-ktotal,
		K(global_page_state(NR_SLAB_RECLAIMABLE) +
				global_page_state(NR_SLAB_UNRECLAIMABLE)),
		K(global_page_state(NR_SLAB_RECLAIMABLE)),
		K(global_page_state(NR_SLAB_UNRECLAIMABLE)),
		global_page_state(NR_KERNEL_STACK) * THREAD_SIZE / 1024,
		K(global_page_state(NR_PAGETABLE)),
		zone_page_state(r_zone, NR_FILE_PAGES)*4,
		zone_page_state(r_zone, NR_ACTIVE_FILE)*4,
		zone_page_state(r_zone, NR_INACTIVE_FILE)*4,
		zone_page_state(r_zone, NR_FILE_MAPPED)*4,
		zone_page_state(r_zone, NR_SHMEM)*4,
		zone_page_state(r_zone, NR_ANON_PAGES)*4,
		zone_page_state(r_zone, NR_ACTIVE_ANON)*4,
		zone_page_state(r_zone, NR_INACTIVE_ANON)*4,
		(vmi.low_page_cnt*4),
		(sec_meminfo_get_alloc_total_cnt(PGD_TYPE)*16),
		DIV_ROUND_UP(((__init_end) - (__init_begin)), SZ_1K),
		DIV_ROUND_UP(((_etext) - (_text)), SZ_1K),
		DIV_ROUND_UP(((_edata) - (_sdata)), SZ_1K),
		DIV_ROUND_UP(((__bss_stop) - (__bss_start)), SZ_1K),
	#ifdef CONFIG_VIDEO_MALI400MP
		(sec_meminfo_get_alloc_low_cnt(MALI_TYPE)*4),
	#endif
	#ifdef CONFIG_VIDEO_UMP
		(sec_meminfo_get_alloc_low_cnt(UMP_TYPE)*4),
	#endif
		K(i.totalhigh),
		K(i.freehigh),
		K((i.totalhigh)-(i.freehigh)),
		htotal,
		K((i.totalhigh)-(i.freehigh))-htotal,
		zone_page_state(h_zone, NR_FILE_PAGES)*4,
		zone_page_state(h_zone, NR_ACTIVE_FILE)*4,
		zone_page_state(h_zone, NR_INACTIVE_FILE)*4,
		zone_page_state(h_zone, NR_FILE_MAPPED)*4,
		zone_page_state(h_zone, NR_SHMEM)*4,
		zone_page_state(h_zone, NR_ANON_PAGES)*4,
		zone_page_state(h_zone, NR_ACTIVE_ANON)*4,
		zone_page_state(h_zone, NR_INACTIVE_ANON)*4,
		(vmi.total_page_cnt-vmi.low_page_cnt)*4
	#ifdef CONFIG_VIDEO_MALI400MP
		, ((sec_meminfo_get_alloc_total_cnt(MALI_TYPE) - sec_meminfo_get_alloc_low_cnt(MALI_TYPE))*4),
	#endif
	#ifdef CONFIG_VIDEO_UMP
		((sec_meminfo_get_alloc_total_cnt(UMP_TYPE) - sec_meminfo_get_alloc_low_cnt(UMP_TYPE))*4)
	#endif
		);
	return 0;
#undef K
}

void sec_meminfo_print(void)
{
	struct sysinfo i;
	struct vmalloc_info vmi;
	long cached;
	unsigned long ktotal, htotal;
	struct zone *r_zone = NULL;
	struct zone *h_zone = NULL;
	struct zone *zone = NULL;

/*
 * display in kilobytes.
 */
#define K(x) ((x) << (PAGE_SHIFT - 10))
	get_vmalloc_info(&vmi);
	si_meminfo(&i);
	si_swapinfo(&i);

	cached = global_page_state(NR_FILE_PAGES) -
			total_swapcache_pages - i.bufferram;

	if (cached < 0)
		cached = 0;

	for_each_populated_zone(zone) {
		r_zone = zone;
		if (strcmp(r_zone->name, "Normal") == 0)
			break;
	}

	for_each_populated_zone(zone) {
		h_zone = zone;
		if (strcmp(h_zone->name, "High") == 0)
			break;
	}

	ktotal = K(global_page_state(NR_SLAB_RECLAIMABLE) \
		+global_page_state(NR_SLAB_UNRECLAIMABLE)) \
		+(global_page_state(NR_KERNEL_STACK) * THREAD_SIZE / 1024) \
		+K(global_page_state(NR_PAGETABLE)) \
		+(zone_page_state(r_zone, NR_FILE_PAGES)*4) \
		+(zone_page_state(r_zone, NR_ANON_PAGES)*4) \
		+(vmi.low_page_cnt*4) \
		+(sec_meminfo_get_alloc_total_cnt(PGD_TYPE)*16) \
		+DIV_ROUND_UP(((__init_end) - (__init_begin)), SZ_1K) \
		+DIV_ROUND_UP(((_etext) - (_text)), SZ_1K) \
		+DIV_ROUND_UP(((_edata) - (_sdata)), SZ_1K) \
		+DIV_ROUND_UP(((__bss_stop) - (__bss_start)), SZ_1K);

#ifdef CONFIG_VIDEO_MALI400MP
	ktotal += (sec_meminfo_get_alloc_low_cnt(MALI_TYPE)*4);
#endif
#ifdef CONFIG_VIDEO_UMP
	ktotal += (sec_meminfo_get_alloc_low_cnt(UMP_TYPE)*4);
#endif

	htotal = (zone_page_state(h_zone, NR_FILE_PAGES)*4) \
		+(zone_page_state(h_zone, NR_ANON_PAGES)*4) \
		+((vmi.total_page_cnt-vmi.low_page_cnt)*4);

#ifdef CONFIG_VIDEO_MALI400MP
	htotal += ((sec_meminfo_get_alloc_total_cnt(MALI_TYPE) - sec_meminfo_get_alloc_low_cnt(MALI_TYPE))*4);
#endif
#ifdef CONFIG_VIDEO_UMP
	htotal += ((sec_meminfo_get_alloc_total_cnt(UMP_TYPE) - sec_meminfo_get_alloc_low_cnt(UMP_TYPE))*4);
#endif

	printk(
		"\nSEC memory information\n"
		"LowMemTotal:	%8lu kB\n"
		"LowMemFree:	%8lu kB\n"
		"LowMemUsed:	%8lu kB\n"
		"-KnownTotal:	%8lu kB\n"
		"-Etc:		%8ld kB\n"
		"Slab:		%8lu kB\n"
		"-SReclaimable:	%8lu kB\n"
		"-SUnreclaim:	%8lu kB\n"
		"KernelStack:	%8lu kB\n"
		"PageTables:	%8lu kB\n"
		"File Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"--Mapped:	%8lu kB\n"
		"--Shmem:	%8lu kB\n"
		"Anon Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"Vmalloc:	%8lu kB\n"
		"PGD:		%8d kB\n"
		".init:		%8d kB\n"
		".text:		%8d kB\n"
		".data:		%8d kB\n"
		".bss:		%8d kB\n"
	#ifdef CONFIG_VIDEO_MALI400MP
		"Mali:		%8d kB\n"
	#endif
	#ifdef CONFIG_VIDEO_UMP
		"UMP:		%8d kB\n"
	#endif
		"\n"
		"HighMemTotal:	%8lu kB\n"
		"HighMemFree:	%8lu kB\n"
		"HighMemUsed:	%8lu kB\n"
		"-KnownTotal:	%8lu kB\n"
		"-Etc:		%8ld kB\n"
		"File Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"--Mapped:	%8lu kB\n"
		"--Shmem:	%8lu kB\n"
		"Anon Page:	%8lu kB\n"
		"-Active:	%8lu kB\n"
		"-Inactive:	%8lu kB\n"
		"Vmalloc:	%8lu kB\n"
	#ifdef CONFIG_VIDEO_MALI400MP
		"Mali:		%8d kB\n"
	#endif
	#ifdef CONFIG_VIDEO_UMP
		"UMP:		%8d kB\n\n"
	#endif
		,
		K(i.totalram-i.totalhigh),
		K(i.freeram-i.freehigh),
		K((i.totalram-i.totalhigh)-(i.freeram-i.freehigh)),
		ktotal,
		K((i.totalram-i.totalhigh)-(i.freeram-i.freehigh))-ktotal,
		K(global_page_state(NR_SLAB_RECLAIMABLE) +
				global_page_state(NR_SLAB_UNRECLAIMABLE)),
		K(global_page_state(NR_SLAB_RECLAIMABLE)),
		K(global_page_state(NR_SLAB_UNRECLAIMABLE)),
		global_page_state(NR_KERNEL_STACK) * THREAD_SIZE / 1024,
		K(global_page_state(NR_PAGETABLE)),
		zone_page_state(r_zone, NR_FILE_PAGES)*4,
		zone_page_state(r_zone, NR_ACTIVE_FILE)*4,
		zone_page_state(r_zone, NR_INACTIVE_FILE)*4,
		zone_page_state(r_zone, NR_FILE_MAPPED)*4,
		zone_page_state(r_zone, NR_SHMEM)*4,
		zone_page_state(r_zone, NR_ANON_PAGES)*4,
		zone_page_state(r_zone, NR_ACTIVE_ANON)*4,
		zone_page_state(r_zone, NR_INACTIVE_ANON)*4,
		(vmi.low_page_cnt*4),
		(sec_meminfo_get_alloc_total_cnt(PGD_TYPE)*16),
		DIV_ROUND_UP(((__init_end) - (__init_begin)), SZ_1K),
		DIV_ROUND_UP(((_etext) - (_text)), SZ_1K),
		DIV_ROUND_UP(((_edata) - (_sdata)), SZ_1K),
		DIV_ROUND_UP(((__bss_stop) - (__bss_start)), SZ_1K),
	#ifdef CONFIG_VIDEO_MALI400MP
		(sec_meminfo_get_alloc_low_cnt(MALI_TYPE)*4),
	#endif
	#ifdef CONFIG_VIDEO_UMP
		(sec_meminfo_get_alloc_low_cnt(UMP_TYPE)*4),
	#endif
		K(i.totalhigh),
		K(i.freehigh),
		K((i.totalhigh)-(i.freehigh)),
		htotal,
		K((i.totalhigh)-(i.freehigh))-htotal,
		zone_page_state(h_zone, NR_FILE_PAGES)*4,
		zone_page_state(h_zone, NR_ACTIVE_FILE)*4,
		zone_page_state(h_zone, NR_INACTIVE_FILE)*4,
		zone_page_state(h_zone, NR_FILE_MAPPED)*4,
		zone_page_state(h_zone, NR_SHMEM)*4,
		zone_page_state(h_zone, NR_ANON_PAGES)*4,
		zone_page_state(h_zone, NR_ACTIVE_ANON)*4,
		zone_page_state(h_zone, NR_INACTIVE_ANON)*4,
		(vmi.total_page_cnt-vmi.low_page_cnt)*4
	#ifdef CONFIG_VIDEO_MALI400MP
		, ((sec_meminfo_get_alloc_total_cnt(MALI_TYPE) - sec_meminfo_get_alloc_low_cnt(MALI_TYPE))*4),
	#endif
	#ifdef CONFIG_VIDEO_UMP
		((sec_meminfo_get_alloc_total_cnt(UMP_TYPE) - sec_meminfo_get_alloc_low_cnt(UMP_TYPE))*4)
	#endif
		);
#undef K
}

static int sec_meminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_meminfo_proc_show, NULL);
}

static const struct file_operations sec_meminfo_proc_fops = {
	.open		= sec_meminfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init sec_proc_meminfo_init(void)
{
	proc_create("sec_meminfo", 0, NULL, &sec_meminfo_proc_fops);
	return 0;
}
module_init(sec_proc_meminfo_init);
