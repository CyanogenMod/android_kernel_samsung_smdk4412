#undef TRACE_SYSTEM
#define TRACE_SYSTEM vmscan

#if !defined(_TRACE_VMSCAN_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_VMSCAN_H

#include <linux/types.h>
#include <linux/tracepoint.h>
#include <linux/mm.h>
#include <linux/memcontrol.h>
#include "gfpflags.h"

#define RECLAIM_WB_ANON		0x0001u
#define RECLAIM_WB_FILE		0x0002u
#define RECLAIM_WB_MIXED	0x0010u
#define RECLAIM_WB_SYNC		0x0004u
#define RECLAIM_WB_ASYNC	0x0008u

#define show_reclaim_flags(flags)				\
	(flags) ? __print_flags(flags, "|",			\
		{RECLAIM_WB_ANON,	"RECLAIM_WB_ANON"},	\
		{RECLAIM_WB_FILE,	"RECLAIM_WB_FILE"},	\
		{RECLAIM_WB_MIXED,	"RECLAIM_WB_MIXED"},	\
		{RECLAIM_WB_SYNC,	"RECLAIM_WB_SYNC"},	\
		{RECLAIM_WB_ASYNC,	"RECLAIM_WB_ASYNC"}	\
		) : "RECLAIM_WB_NONE"

#define trace_reclaim_flags(page, sync) ( \
	(page_is_file_cache(page) ? RECLAIM_WB_FILE : RECLAIM_WB_ANON) | \
	(sync & RECLAIM_MODE_SYNC ? RECLAIM_WB_SYNC : RECLAIM_WB_ASYNC)   \
	)

#define trace_shrink_flags(file, sync) ( \
	(sync & RECLAIM_MODE_SYNC ? RECLAIM_WB_MIXED : \
			(file ? RECLAIM_WB_FILE : RECLAIM_WB_ANON)) |  \
	(sync & RECLAIM_MODE_SYNC ? RECLAIM_WB_SYNC : RECLAIM_WB_ASYNC) \
	)

TRACE_EVENT(mm_vmscan_kswapd_sleep,

	TP_PROTO(int nid),

	TP_ARGS(nid),

	TP_STRUCT__entry(
		__field(	int,	nid	)
	),

	TP_fast_assign(
		__entry->nid	= nid;
	),

	TP_printk("nid=%d", __entry->nid)
);

TRACE_EVENT(mm_vmscan_kswapd_wake,

	TP_PROTO(int nid, int order),

	TP_ARGS(nid, order),

	TP_STRUCT__entry(
		__field(	int,	nid	)
		__field(	int,	order	)
	),

	TP_fast_assign(
		__entry->nid	= nid;
		__entry->order	= order;
	),

	TP_printk("nid=%d order=%d", __entry->nid, __entry->order)
);

TRACE_EVENT(mm_vmscan_wakeup_kswapd,

	TP_PROTO(int nid, int zid, int order),

	TP_ARGS(nid, zid, order),

	TP_STRUCT__entry(
		__field(	int,		nid	)
		__field(	int,		zid	)
		__field(	int,		order	)
	),

	TP_fast_assign(
		__entry->nid		= nid;
		__entry->zid		= zid;
		__entry->order		= order;
	),

	TP_printk("nid=%d zid=%d order=%d",
		__entry->nid,
		__entry->zid,
		__entry->order)
);

DECLARE_EVENT_CLASS(mm_vmscan_direct_reclaim_begin_template,

	TP_PROTO(int order, int may_writepage, gfp_t gfp_flags),

	TP_ARGS(order, may_writepage, gfp_flags),

	TP_STRUCT__entry(
		__field(	int,	order		)
		__field(	int,	may_writepage	)
		__field(	gfp_t,	gfp_flags	)
	),

	TP_fast_assign(
		__entry->order		= order;
		__entry->may_writepage	= may_writepage;
		__entry->gfp_flags	= gfp_flags;
	),

	TP_printk("order=%d may_writepage=%d gfp_flags=%s",
		__entry->order,
		__entry->may_writepage,
		show_gfp_flags(__entry->gfp_flags))
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_begin_template, mm_vmscan_direct_reclaim_begin,

	TP_PROTO(int order, int may_writepage, gfp_t gfp_flags),

	TP_ARGS(order, may_writepage, gfp_flags)
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_begin_template, mm_vmscan_memcg_reclaim_begin,

	TP_PROTO(int order, int may_writepage, gfp_t gfp_flags),

	TP_ARGS(order, may_writepage, gfp_flags)
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_begin_template, mm_vmscan_memcg_softlimit_reclaim_begin,

	TP_PROTO(int order, int may_writepage, gfp_t gfp_flags),

	TP_ARGS(order, may_writepage, gfp_flags)
);

DECLARE_EVENT_CLASS(mm_vmscan_direct_reclaim_end_template,

	TP_PROTO(unsigned long nr_reclaimed),

	TP_ARGS(nr_reclaimed),

	TP_STRUCT__entry(
		__field(	unsigned long,	nr_reclaimed	)
	),

	TP_fast_assign(
		__entry->nr_reclaimed	= nr_reclaimed;
	),

	TP_printk("nr_reclaimed=%lu", __entry->nr_reclaimed)
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_end_template, mm_vmscan_direct_reclaim_end,

	TP_PROTO(unsigned long nr_reclaimed),

	TP_ARGS(nr_reclaimed)
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_end_template, mm_vmscan_memcg_reclaim_end,

	TP_PROTO(unsigned long nr_reclaimed),

	TP_ARGS(nr_reclaimed)
);

DEFINE_EVENT(mm_vmscan_direct_reclaim_end_template, mm_vmscan_memcg_softlimit_reclaim_end,

	TP_PROTO(unsigned long nr_reclaimed),

	TP_ARGS(nr_reclaimed)
);

TRACE_EVENT(mm_shrink_slab_start,
	TP_PROTO(struct shrinker *shr, struct shrink_control *sc,
		long nr_objects_to_shrink, unsigned long pgs_scanned,
		unsigned long lru_pgs, unsigned long cache_items,
		unsigned long long delta, unsigned long total_scan),

	TP_ARGS(shr, sc, nr_objects_to_shrink, pgs_scanned, lru_pgs,
		cache_items, delta, total_scan),

	TP_STRUCT__entry(
		__field(struct shrinker *, shr)
		__field(void *, shrink)
		__field(long, nr_objects_to_shrink)
		__field(gfp_t, gfp_flags)
		__field(unsigned long, pgs_scanned)
		__field(unsigned long, lru_pgs)
		__field(unsigned long, cache_items)
		__field(unsigned long long, delta)
		__field(unsigned long, total_scan)
	),

	TP_fast_assign(
		__entry->shr = shr;
		__entry->shrink = shr->shrink;
		__entry->nr_objects_to_shrink = nr_objects_to_shrink;
		__entry->gfp_flags = sc->gfp_mask;
		__entry->pgs_scanned = pgs_scanned;
		__entry->lru_pgs = lru_pgs;
		__entry->cache_items = cache_items;
		__entry->delta = delta;
		__entry->total_scan = total_scan;
	),

	TP_printk("%pF %p: objects to shrink %ld gfp_flags %s pgs_scanned %ld lru_pgs %ld cache items %ld delta %lld total_scan %ld",
		__entry->shrink,
		__entry->shr,
		__entry->nr_objects_to_shrink,
		show_gfp_flags(__entry->gfp_flags),
		__entry->pgs_scanned,
		__entry->lru_pgs,
		__entry->cache_items,
		__entry->delta,
		__entry->total_scan)
);

TRACE_EVENT(mm_shrink_slab_end,
	TP_PROTO(struct shrinker *shr, int shrinker_retval,
		long unused_scan_cnt, long new_scan_cnt),

	TP_ARGS(shr, shrinker_retval, unused_scan_cnt, new_scan_cnt),

	TP_STRUCT__entry(
		__field(struct shrinker *, shr)
		__field(void *, shrink)
		__field(long, unused_scan)
		__field(long, new_scan)
		__field(int, retval)
		__field(long, total_scan)
	),

	TP_fast_assign(
		__entry->shr = shr;
		__entry->shrink = shr->shrink;
		__entry->unused_scan = unused_scan_cnt;
		__entry->new_scan = new_scan_cnt;
		__entry->retval = shrinker_retval;
		__entry->total_scan = new_scan_cnt - unused_scan_cnt;
	),

	TP_printk("%pF %p: unused scan count %ld new scan count %ld total_scan %ld last shrinker return val %d",
		__entry->shrink,
		__entry->shr,
		__entry->unused_scan,
		__entry->new_scan,
		__entry->total_scan,
		__entry->retval)
);

DECLARE_EVENT_CLASS(mm_vmscan_lru_isolate_template,

	TP_PROTO(int order,
		unsigned long nr_requested,
		unsigned long nr_scanned,
		unsigned long nr_taken,
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		unsigned long nr_lumpy_taken,
		unsigned long nr_lumpy_dirty,
		unsigned long nr_lumpy_failed,
		isolate_mode_t isolate_mode),
#endif

#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, nr_lumpy_taken, nr_lumpy_dirty, nr_lumpy_failed, isolate_mode),
#else
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, isolate_mode),
#endif

	TP_STRUCT__entry(
		__field(int, order)
		__field(unsigned long, nr_requested)
		__field(unsigned long, nr_scanned)
		__field(unsigned long, nr_taken)
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		__field(unsigned long, nr_lumpy_taken)
		__field(unsigned long, nr_lumpy_dirty)
		__field(unsigned long, nr_lumpy_failed)
		__field(isolate_mode_t, isolate_mode)
#endif
	),

	TP_fast_assign(
		__entry->order = order;
		__entry->nr_requested = nr_requested;
		__entry->nr_scanned = nr_scanned;
		__entry->nr_taken = nr_taken;
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		__entry->nr_lumpy_taken = nr_lumpy_taken;
		__entry->nr_lumpy_dirty = nr_lumpy_dirty;
		__entry->nr_lumpy_failed = nr_lumpy_failed;
#endif
		__entry->isolate_mode = isolate_mode;
	),

#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
	TP_printk("isolate_mode=%d order=%d nr_requested=%lu nr_scanned=%lu nr_taken=%lu contig_taken=%lu contig_dirty=%lu contig_failed=%lu",
#else
	TP_printk("isolate_mode=%d order=%d nr_requested=%lu nr_scanned=%lu nr_taken=%lu",
#endif
		__entry->isolate_mode,
		__entry->order,
		__entry->nr_requested,
		__entry->nr_scanned,
		__entry->nr_taken
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		, __entry->nr_lumpy_taken,
		__entry->nr_lumpy_dirty,
		__entry->nr_lumpy_failed
#endif
		)
);

DEFINE_EVENT(mm_vmscan_lru_isolate_template, mm_vmscan_lru_isolate,

	TP_PROTO(int order,
		unsigned long nr_requested,
		unsigned long nr_scanned,
		unsigned long nr_taken,
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		unsigned long nr_lumpy_taken,
		unsigned long nr_lumpy_dirty,
		unsigned long nr_lumpy_failed,
		isolate_mode_t isolate_mode),
#endif

#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, nr_lumpy_taken, nr_lumpy_dirty, nr_lumpy_failed, isolate_mode)
#else
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, isolate_mode)
#endif

);

DEFINE_EVENT(mm_vmscan_lru_isolate_template, mm_vmscan_memcg_isolate,

	TP_PROTO(int order,
		unsigned long nr_requested,
		unsigned long nr_scanned,
		unsigned long nr_taken,
#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
		unsigned long nr_lumpy_taken,
		unsigned long nr_lumpy_dirty,
		unsigned long nr_lumpy_failed,
		isolate_mode_t isolate_mode),
#endif

#ifndef CONFIG_DISABLE_LUMPY_RECLAIM
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, nr_lumpy_taken, nr_lumpy_dirty, nr_lumpy_failed, isolate_mode)
#else
	TP_ARGS(order, nr_requested, nr_scanned, nr_taken, isolate_mode)
#endif

);

TRACE_EVENT(mm_vmscan_writepage,

	TP_PROTO(struct page *page,
		int reclaim_flags),

	TP_ARGS(page, reclaim_flags),

	TP_STRUCT__entry(
		__field(struct page *, page)
		__field(int, reclaim_flags)
	),

	TP_fast_assign(
		__entry->page = page;
		__entry->reclaim_flags = reclaim_flags;
	),

	TP_printk("page=%p pfn=%lu flags=%s",
		__entry->page,
		page_to_pfn(__entry->page),
		show_reclaim_flags(__entry->reclaim_flags))
);

TRACE_EVENT(mm_vmscan_lru_shrink_inactive,

	TP_PROTO(int nid, int zid,
			unsigned long nr_scanned, unsigned long nr_reclaimed,
			int priority, int reclaim_flags),

	TP_ARGS(nid, zid, nr_scanned, nr_reclaimed, priority, reclaim_flags),

	TP_STRUCT__entry(
		__field(int, nid)
		__field(int, zid)
		__field(unsigned long, nr_scanned)
		__field(unsigned long, nr_reclaimed)
		__field(int, priority)
		__field(int, reclaim_flags)
	),

	TP_fast_assign(
		__entry->nid = nid;
		__entry->zid = zid;
		__entry->nr_scanned = nr_scanned;
		__entry->nr_reclaimed = nr_reclaimed;
		__entry->priority = priority;
		__entry->reclaim_flags = reclaim_flags;
	),

	TP_printk("nid=%d zid=%d nr_scanned=%ld nr_reclaimed=%ld priority=%d flags=%s",
		__entry->nid, __entry->zid,
		__entry->nr_scanned, __entry->nr_reclaimed,
		__entry->priority,
		show_reclaim_flags(__entry->reclaim_flags))
);

TRACE_EVENT(replace_swap_token,
	TP_PROTO(struct mm_struct *old_mm,
		 struct mm_struct *new_mm),

	TP_ARGS(old_mm, new_mm),

	TP_STRUCT__entry(
		__field(struct mm_struct*,	old_mm)
		__field(unsigned int,		old_prio)
		__field(struct mm_struct*,	new_mm)
		__field(unsigned int,		new_prio)
	),

	TP_fast_assign(
		__entry->old_mm   = old_mm;
		__entry->old_prio = old_mm ? old_mm->token_priority : 0;
		__entry->new_mm   = new_mm;
		__entry->new_prio = new_mm->token_priority;
	),

	TP_printk("old_token_mm=%p old_prio=%u new_token_mm=%p new_prio=%u",
		  __entry->old_mm, __entry->old_prio,
		  __entry->new_mm, __entry->new_prio)
);

DECLARE_EVENT_CLASS(put_swap_token_template,
	TP_PROTO(struct mm_struct *swap_token_mm),

	TP_ARGS(swap_token_mm),

	TP_STRUCT__entry(
		__field(struct mm_struct*, swap_token_mm)
	),

	TP_fast_assign(
		__entry->swap_token_mm = swap_token_mm;
	),

	TP_printk("token_mm=%p", __entry->swap_token_mm)
);

DEFINE_EVENT(put_swap_token_template, put_swap_token,
	TP_PROTO(struct mm_struct *swap_token_mm),
	TP_ARGS(swap_token_mm)
);

DEFINE_EVENT_CONDITION(put_swap_token_template, disable_swap_token,
	TP_PROTO(struct mm_struct *swap_token_mm),
	TP_ARGS(swap_token_mm),
	TP_CONDITION(swap_token_mm != NULL)
);

TRACE_EVENT_CONDITION(update_swap_token_priority,
	TP_PROTO(struct mm_struct *mm,
		 unsigned int old_prio,
		 struct mm_struct *swap_token_mm),

	TP_ARGS(mm, old_prio, swap_token_mm),

	TP_CONDITION(mm->token_priority != old_prio),

	TP_STRUCT__entry(
		__field(struct mm_struct*, mm)
		__field(unsigned int, old_prio)
		__field(unsigned int, new_prio)
		__field(struct mm_struct*, swap_token_mm)
		__field(unsigned int, swap_token_prio)
	),

	TP_fast_assign(
		__entry->mm		= mm;
		__entry->old_prio	= old_prio;
		__entry->new_prio	= mm->token_priority;
		__entry->swap_token_mm	= swap_token_mm;
		__entry->swap_token_prio = swap_token_mm ? swap_token_mm->token_priority : 0;
	),

	TP_printk("mm=%p old_prio=%u new_prio=%u swap_token_mm=%p token_prio=%u",
		  __entry->mm, __entry->old_prio, __entry->new_prio,
		  __entry->swap_token_mm, __entry->swap_token_prio)
);

#endif /* _TRACE_VMSCAN_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
