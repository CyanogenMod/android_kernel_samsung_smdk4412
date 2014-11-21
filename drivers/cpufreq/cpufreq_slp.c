/*
 *  drivers/cpufreq/cpufreq_pegasusq.c
 *
 *  Copyright (C)  2011 Samsung Electronics co. ltd
 *    ByungChang Cha <bc.cha@samsung.com>
 *
 *  Based on ondemand governor
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/reboot.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#define EARLYSUSPEND_HOTPLUGLOCK 1


#if defined(CONFIG_SLP_CHECK_CPU_LOAD)
#define CONFIG_SLP_ADAPTIVE_HOTPLUG 1
#endif

/*
 * runqueue average
 */

#define CPU_NUM	NR_CPUS

#define RQ_AVG_TIMER_RATE	10

struct runqueue_data {
	unsigned int nr_run_avg;
	unsigned int update_rate;
	int64_t last_time;
	int64_t total_time;
	struct delayed_work work;
	struct workqueue_struct *nr_run_wq;
	spinlock_t lock;
};

static struct runqueue_data *rq_data;
static void rq_work_fn(struct work_struct *work);

static void start_rq_work(void)
{
	rq_data->nr_run_avg = 0;
	rq_data->last_time = 0;
	rq_data->total_time = 0;
	if (rq_data->nr_run_wq == NULL)
		rq_data->nr_run_wq =
			create_singlethread_workqueue("nr_run_avg");

	queue_delayed_work(rq_data->nr_run_wq, &rq_data->work,
			   msecs_to_jiffies(rq_data->update_rate));
	return;
}

static void stop_rq_work(void)
{
	if (rq_data->nr_run_wq)
		cancel_delayed_work(&rq_data->work);
	return;
}

static int __init init_rq_avg(void)
{
	rq_data = kzalloc(sizeof(struct runqueue_data), GFP_KERNEL);
	if (rq_data == NULL) {
		pr_err("%s cannot allocate memory\n", __func__);
		return -ENOMEM;
	}
	spin_lock_init(&rq_data->lock);
	rq_data->update_rate = RQ_AVG_TIMER_RATE;
	INIT_DELAYED_WORK_DEFERRABLE(&rq_data->work, rq_work_fn);

	return 0;
}

static void rq_work_fn(struct work_struct *work)
{
	int64_t time_diff = 0;
	int64_t nr_run = 0;
	unsigned long flags = 0;
	int64_t cur_time = ktime_to_ns(ktime_get());

	spin_lock_irqsave(&rq_data->lock, flags);

	if (rq_data->last_time == 0)
		rq_data->last_time = cur_time;
	if (rq_data->nr_run_avg == 0)
		rq_data->total_time = 0;

	nr_run = nr_running() * 100;
	time_diff = cur_time - rq_data->last_time;
	do_div(time_diff, 1000 * 1000);

	if (time_diff != 0 && rq_data->total_time != 0) {
		nr_run = (nr_run * time_diff) +
			(rq_data->nr_run_avg * rq_data->total_time);
		do_div(nr_run, rq_data->total_time + time_diff);
	}
	rq_data->nr_run_avg = nr_run;
	rq_data->total_time += time_diff;
	rq_data->last_time = cur_time;

	if (rq_data->update_rate != 0)
		queue_delayed_work(rq_data->nr_run_wq, &rq_data->work,
				   msecs_to_jiffies(rq_data->update_rate));

	spin_unlock_irqrestore(&rq_data->lock, flags);
}

static unsigned int get_nr_run_avg(void)
{
	unsigned int nr_run_avg;
	unsigned long flags = 0;

	spin_lock_irqsave(&rq_data->lock, flags);
	nr_run_avg = rq_data->nr_run_avg;
	rq_data->nr_run_avg = 0;
	spin_unlock_irqrestore(&rq_data->lock, flags);

	return nr_run_avg;
}


/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_SAMPLING_DOWN_FACTOR		(2)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(5)
#define DEF_FREQUENCY_UP_THRESHOLD		(85)
#define DEF_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(11)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)
#define DEF_SAMPLING_RATE			(50000)
#define MIN_SAMPLING_RATE			(10000)
#define MAX_HOTPLUG_RATE			(40u)

#define DEF_MAX_CPU_LOCK			(0)
#define DEF_MIN_CPU_LOCK			(0)
#define DEF_CPU_UP_FREQ				(500000)
#define DEF_CPU_DOWN_FREQ			(200000)
#define DEF_UP_NR_CPUS				(1)
#define DEF_CPU_UP_RATE				(10)
#define DEF_CPU_DOWN_RATE			(20)
#define DEF_FREQ_STEP				(40)
#define DEF_START_DELAY				(0)

#define UP_THRESHOLD_AT_MIN_FREQ		(40)
#define FREQ_FOR_RESPONSIVENESS			(500000)

#define HOTPLUG_DOWN_INDEX			(0)
#define HOTPLUG_UP_INDEX			(1)

#ifdef CONFIG_MACH_MIDAS
static int hotplug_rq[4][2] = {
	{0, 100}, {100, 200}, {200, 300}, {300, 0}
};

static int hotplug_freq[4][2] = {
	{0, 500000},
	{200000, 500000},
	{200000, 500000},
	{200000, 0}
};
#else
static int hotplug_rq[4][2] = {
	{0, 100}, {100, 200}, {200, 300}, {300, 0}
};

static int hotplug_freq[4][2] = {
	{0, 500000},
	{200000, 500000},
	{200000, 500000},
	{200000, 0}
};
#endif

static unsigned int min_sampling_rate;

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_PEGASUSQ
static
#endif
struct cpufreq_governor cpufreq_gov_slp = {
	.name                   = "slp",
	.governor               = cpufreq_governor_dbs,
	.owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;

	unsigned int cpufreq_old;
	unsigned int cpufreq_new;

	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	struct delayed_work params_work;
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
	struct work_struct up_work;
	struct work_struct down_work;
	struct work_struct mfl_hotplug_work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int rate_mult;
	int cpu;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	struct mutex params_mutex;
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static struct workqueue_struct *dvfs_workqueue;

static unsigned int dbs_enable;	/* number of CPUs using this policy */

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
/* SLP Gov. dynamic Params is new feature which provides better performance
   and more efficient power consumption at application launching time.
   There are three param to support this feature.
 */
#define DYNAMIC_PARAMS_MAX				 3

#define DYNAMIC_PARAMS_MIN_INTERVAL		 200000
#define DYNAMIC_PARAMS_MIN_FREQ			 200000
#ifdef CONFIG_EXYNOS4X12_1400MHZ_SUPPORT
#define DYNAMIC_PARAMS_MAX_FREQ			 1400000
#else
#error    SLP Governor only supports EXYNOS4412 now
#endif
#define DYNAMIC_PARAMS_MIN_CPU			 1
#ifdef CONFIG_CPU_EXYNOS4412
#define DYNAMIC_PARAMS_MAX_CPU			 4
#else
#error    SLP Governor only supports EXYNOS4412 now
#endif

static struct workqueue_struct *dynamic_params_workqueue;
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int down_differential;
	unsigned int ignore_nice;
	unsigned int sampling_down_factor;
	unsigned int io_is_busy;
	/* pegasusq tuners */
	unsigned int freq_step;
	unsigned int cpu_up_rate;
	unsigned int cpu_down_rate;
	unsigned int cpu_up_freq;
	unsigned int cpu_down_freq;
	unsigned int up_nr_cpus;
	unsigned int max_cpu_lock;
	unsigned int min_cpu_lock;
	atomic_t hotplug_lock;
	unsigned int dvfs_debug;
	unsigned int max_freq;
	unsigned int min_freq;
#ifdef CONFIG_HAS_EARLYSUSPEND
	int early_suspend;
#endif
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	unsigned int dynamic_params;

	unsigned int params_min_interval;
	unsigned int params_min_freq;
	unsigned int params_min_cpu;

	unsigned int params_stat_min_freq;
	unsigned int params_stat_min_cpu;
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
	.ignore_nice = 0,
	.freq_step = DEF_FREQ_STEP,
	.cpu_up_rate = DEF_CPU_UP_RATE,
	.cpu_down_rate = DEF_CPU_DOWN_RATE,
	.cpu_up_freq = DEF_CPU_UP_FREQ,
	.cpu_down_freq = DEF_CPU_DOWN_FREQ,
	.up_nr_cpus = DEF_UP_NR_CPUS,
	.max_cpu_lock = DEF_MAX_CPU_LOCK,
	.min_cpu_lock = DEF_MIN_CPU_LOCK,
	.hotplug_lock = ATOMIC_INIT(0),
	.dvfs_debug = 0,
#ifdef CONFIG_HAS_EARLYSUSPEND
	.early_suspend = -1,
#endif
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	.dynamic_params = 0,

	.params_min_interval = 0,
	.params_min_freq = 0,
	.params_min_cpu = 0,

	.params_stat_min_freq = 0,
	.params_stat_min_cpu = 0,
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
};


/*
 * CPU hotplug lock interface
 */

static atomic_t g_hotplug_count = ATOMIC_INIT(0);
static atomic_t g_hotplug_lock = ATOMIC_INIT(0);

static void apply_hotplug_lock(void)
{
	int online, possible, lock, flag;
	struct work_struct *work;
	struct cpu_dbs_info_s *dbs_info;

	/* do turn_on/off cpus */
	dbs_info = &per_cpu(od_cpu_dbs_info, 0); /* from CPU0 */
	online = num_online_cpus();
	possible = num_possible_cpus();
	lock = atomic_read(&g_hotplug_lock);
	flag = lock - online;

	if (flag == 0)
		return;

	work = flag > 0 ? &dbs_info->up_work : &dbs_info->down_work;

	pr_debug("%s online %d possible %d lock %d flag %d %d\n",
		 __func__, online, possible, lock, flag, (int)abs(flag));

	queue_work_on(dbs_info->cpu, dvfs_workqueue, work);
}

static int cpufreq_pegasusq_cpu_lock(int num_core)
{
	int prev_lock;

	if (num_core < 1 || num_core > num_possible_cpus())
		return -EINVAL;

	prev_lock = atomic_read(&g_hotplug_lock);

	if (prev_lock != 0 && prev_lock < num_core)
		return -EINVAL;
	else if (prev_lock == num_core)
		atomic_inc(&g_hotplug_count);

	atomic_set(&g_hotplug_lock, num_core);
	atomic_set(&g_hotplug_count, 1);
	apply_hotplug_lock();

	return 0;
}

static int cpufreq_pegasusq_cpu_unlock(int num_core)
{
	int prev_lock = atomic_read(&g_hotplug_lock);

	if (prev_lock < num_core)
		return 0;
	else if (prev_lock == num_core)
		atomic_dec(&g_hotplug_count);

	if (atomic_read(&g_hotplug_count) == 0)
		atomic_set(&g_hotplug_lock, 0);

	return 0;
}


/*
 * History of CPU usage
 */
struct cpu_usage {
	unsigned int freq;
	unsigned int load[CPU_NUM];
	unsigned int rq_avg;
};

struct cpu_usage_history {
	struct cpu_usage usage[MAX_HOTPLUG_RATE];
	unsigned int num_hist;
};

static struct cpu_usage_history *hotplug_history;

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
						  cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
				  kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu,
					      cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);
show_one(down_differential, down_differential);
show_one(freq_step, freq_step);
show_one(cpu_up_rate, cpu_up_rate);
show_one(cpu_down_rate, cpu_down_rate);
show_one(cpu_up_freq, cpu_up_freq);
show_one(cpu_down_freq, cpu_down_freq);
show_one(up_nr_cpus, up_nr_cpus);
show_one(max_cpu_lock, max_cpu_lock);
show_one(min_cpu_lock, min_cpu_lock);
show_one(dvfs_debug, dvfs_debug);
static ssize_t show_hotplug_lock(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&g_hotplug_lock));
}

#define show_hotplug_param(file_name, num_core, up_down)		\
static ssize_t show_##file_name##_##num_core##_##up_down		\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", file_name[num_core - 1][up_down]);	\
}

#define store_hotplug_param(file_name, num_core, up_down)		\
static ssize_t store_##file_name##_##num_core##_##up_down		\
(struct kobject *kobj, struct attribute *attr,				\
	const char *buf, size_t count)					\
{									\
	unsigned int input;						\
	int ret;							\
	ret = sscanf(buf, "%u", &input);				\
	if (ret != 1)							\
		return -EINVAL;						\
	file_name[num_core - 1][up_down] = input;			\
	return count;							\
}

show_hotplug_param(hotplug_freq, 1, 1);
show_hotplug_param(hotplug_freq, 2, 0);
show_hotplug_param(hotplug_freq, 2, 1);
show_hotplug_param(hotplug_freq, 3, 0);
show_hotplug_param(hotplug_freq, 3, 1);
show_hotplug_param(hotplug_freq, 4, 0);

show_hotplug_param(hotplug_rq, 1, 1);
show_hotplug_param(hotplug_rq, 2, 0);
show_hotplug_param(hotplug_rq, 2, 1);
show_hotplug_param(hotplug_rq, 3, 0);
show_hotplug_param(hotplug_rq, 3, 1);
show_hotplug_param(hotplug_rq, 4, 0);

store_hotplug_param(hotplug_freq, 1, 1);
store_hotplug_param(hotplug_freq, 2, 0);
store_hotplug_param(hotplug_freq, 2, 1);
store_hotplug_param(hotplug_freq, 3, 0);
store_hotplug_param(hotplug_freq, 3, 1);
store_hotplug_param(hotplug_freq, 4, 0);

store_hotplug_param(hotplug_rq, 1, 1);
store_hotplug_param(hotplug_rq, 2, 0);
store_hotplug_param(hotplug_rq, 2, 1);
store_hotplug_param(hotplug_rq, 3, 0);
store_hotplug_param(hotplug_rq, 3, 1);
store_hotplug_param(hotplug_rq, 4, 0);

define_one_global_rw(hotplug_freq_1_1);
define_one_global_rw(hotplug_freq_2_0);
define_one_global_rw(hotplug_freq_2_1);
define_one_global_rw(hotplug_freq_3_0);
define_one_global_rw(hotplug_freq_3_1);
define_one_global_rw(hotplug_freq_4_0);

define_one_global_rw(hotplug_rq_1_1);
define_one_global_rw(hotplug_rq_2_0);
define_one_global_rw(hotplug_rq_2_1);
define_one_global_rw(hotplug_rq_3_0);
define_one_global_rw(hotplug_rq_3_1);
define_one_global_rw(hotplug_rq_4_0);

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
	    input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle =
			get_cpu_idle_time(j, &dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
	}
	return count;
}

static ssize_t store_down_differential(struct kobject *a, struct attribute *b,
				       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.down_differential = min(input, 100u);
	return count;
}

static ssize_t store_freq_step(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.freq_step = min(input, 100u);
	return count;
}

static ssize_t store_cpu_up_rate(struct kobject *a, struct attribute *b,
				 const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_up_rate = min(input, MAX_HOTPLUG_RATE);
	return count;
}

static ssize_t store_cpu_down_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_down_rate = min(input, MAX_HOTPLUG_RATE);
	return count;
}

static ssize_t store_cpu_up_freq(struct kobject *a, struct attribute *b,
				 const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_up_freq = min(input, dbs_tuners_ins.max_freq);
	return count;
}

static ssize_t store_cpu_down_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.cpu_down_freq = max(input, dbs_tuners_ins.min_freq);
	return count;
}

static ssize_t store_up_nr_cpus(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.up_nr_cpus = min(input, num_possible_cpus());
	return count;
}

static ssize_t store_max_cpu_lock(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.max_cpu_lock = min(input, num_possible_cpus());
	return count;
}

static ssize_t store_min_cpu_lock(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.min_cpu_lock = min(input, num_possible_cpus());
	return count;
}

static ssize_t store_hotplug_lock(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int prev_lock;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	input = min(input, num_possible_cpus());
	prev_lock = atomic_read(&dbs_tuners_ins.hotplug_lock);

	if (prev_lock)
		cpufreq_pegasusq_cpu_unlock(prev_lock);

	if (input == 0) {
		atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
		return count;
	}

	ret = cpufreq_pegasusq_cpu_lock(input);
	if (ret) {
		printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
			atomic_read(&g_hotplug_lock), input);
		return ret;
	}

	atomic_set(&dbs_tuners_ins.hotplug_lock, input);

	return count;
}

static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.dvfs_debug = input > 0;
	return count;
}

/******************************************************************************/

static int get_index(int cnt, int ring_size, int diff)
{
	int ret = 0, modified_diff;

	if ((diff > ring_size) || (diff * (-1) > ring_size))
		modified_diff = diff % ring_size;
	else
		modified_diff = diff;

	ret = (ring_size + cnt + modified_diff) % ring_size;

	return ret;
}


#ifdef CONFIG_SLP_CHECK_CPU_LOAD

#define CPU_TASK_HISTORY_NUM 30000
struct cpu_task_history_tag {
	unsigned long long time;
	struct task_struct *task;
	unsigned int pid;
};
static struct cpu_task_history_tag
	cpu_task_history[CPU_NUM][CPU_TASK_HISTORY_NUM];
static struct cpu_task_history_tag
	cpu_task_history_view[CPU_NUM][CPU_TASK_HISTORY_NUM];

struct list_head process_headlist;

static unsigned int  cpu_task_history_cnt[CPU_NUM];
static unsigned int  cpu_task_history_show_start_cnt;
static unsigned int  cpu_task_history_show_end_cnt;
static  int  cpu_task_history_show_select_cpu;

static unsigned long long  total_time, section_start_time, section_end_time;

void __slp_store_task_history(unsigned int cpu, struct task_struct *task)
{
	unsigned int cnt;

	if (++cpu_task_history_cnt[cpu] >= CPU_TASK_HISTORY_NUM)
		cpu_task_history_cnt[cpu] = 0;
	cnt = cpu_task_history_cnt[cpu];

	cpu_task_history[cpu][cnt].time = cpu_clock(UINT_MAX);
	cpu_task_history[cpu][cnt].task = task;
	cpu_task_history[cpu][cnt].pid = task->pid;
}

struct cpu_process_runtime_tag {
	struct list_head list;
	unsigned long long runtime;
	struct task_struct *task;
	unsigned int pid;
	unsigned int cnt;
	unsigned int usage;
};

static unsigned long long calc_delta_time(unsigned int cpu, unsigned int index)
{
	unsigned long long run_start_time, run_end_time;

	if (index == 0) {
		run_start_time
		    = cpu_task_history_view[cpu][CPU_TASK_HISTORY_NUM-1].time;
	} else
		run_start_time = cpu_task_history_view[cpu][index-1].time;

	if (run_start_time < section_start_time)
		run_start_time = section_start_time;

	run_end_time = cpu_task_history_view[cpu][index].time;

	if (run_end_time < section_start_time)
		return 0;

	if (run_end_time > section_end_time)
		run_end_time = section_end_time;

	return  run_end_time - run_start_time;
}


static void add_process_to_list(unsigned int cpu, unsigned int index)
{
	struct cpu_process_runtime_tag *new_process;

	new_process
		= kmalloc(sizeof(struct cpu_process_runtime_tag), GFP_KERNEL);
	new_process->runtime = calc_delta_time(cpu, index);
	new_process->cnt = 1;
	new_process->task = cpu_task_history_view[cpu][index].task;
	new_process->pid = cpu_task_history_view[cpu][index].pid;

	if (new_process->runtime != 0) {
		INIT_LIST_HEAD(&new_process->list);
		list_add_tail(&new_process->list, &process_headlist);
	} else
		kfree(new_process);

	return;
}

static void del_process_list(void)
{
	struct cpu_process_runtime_tag *curr;
	struct list_head *p;

	list_for_each_prev(p, &process_headlist) {
		curr = list_entry(p, struct cpu_process_runtime_tag, list);
		kfree(curr);
	}
	process_headlist.prev = NULL;
	process_headlist.next = NULL;

}


#define CPU_LOAD_HISTORY_NUM 1000

struct cpu_load_freq_history_tag {
	char time[16];
	unsigned long long time_stamp;
	unsigned int cpufreq;
	unsigned int cpu_load[CPU_NUM];
	unsigned int touch_event;
	unsigned int nr_onlinecpu;
	unsigned int nr_run_avg;
	unsigned int task_history_cnt[CPU_NUM];
};
static struct cpu_load_freq_history_tag
	cpu_load_freq_history[CPU_LOAD_HISTORY_NUM];
static struct cpu_load_freq_history_tag
	cpu_load_freq_history_view[CPU_LOAD_HISTORY_NUM];
static unsigned int  cpu_load_freq_history_cnt;
static unsigned int  cpu_load_freq_history_view_cnt;

static  int  cpu_load_freq_history_show_cnt = 85;

struct cpu_process_runtime_tag temp_process_runtime;

static int comp_list_runtime(struct list_head *list1, struct list_head *list2)
{
	struct cpu_process_runtime_tag *list1_struct, *list2_struct;

	int ret = 0;
	 list1_struct = list_entry(list1, struct cpu_process_runtime_tag, list);
	 list2_struct = list_entry(list2, struct cpu_process_runtime_tag, list);

	if (list1_struct->runtime > list2_struct->runtime)
		ret = 1;
	else if (list1_struct->runtime < list2_struct->runtime)
		ret = -1;
	else
		ret  = 0;

	return ret;
}

static void swap_process_list(struct list_head *list1, struct list_head *list2)
{
	struct list_head *list1_prev, *list1_next , *list2_prev, *list2_next;

	list1_prev = list1->prev;
	list1_next = list1->next;

	list2_prev = list2->prev;
	list2_next = list2->next;

	list1->prev = list2;
	list1->next = list2_next;

	list2->prev = list1_prev;
	list2->next = list1;

	list1_prev->next = list2;
	list2_next->prev = list1;

}

static unsigned int view_list(char *buf, unsigned int ret)
{
	struct list_head *p;
	struct cpu_process_runtime_tag *curr;
	char task_name[80] = {0,};
	char *p_name = NULL;

	unsigned int cnt = 0, list_num = 0;
	unsigned long long  t, total_time_for_clc;

	list_for_each(p, &process_headlist) {
		curr = list_entry(p, struct cpu_process_runtime_tag, list);
		list_num++;
	}

	for (cnt = 0; cnt < list_num; cnt++) {
		list_for_each(p, &process_headlist) {
		curr = list_entry(p, struct cpu_process_runtime_tag, list);
			if (p->next != &process_headlist) {
				if (comp_list_runtime(p, p->next) == -1)
					swap_process_list(p, p->next);
			}
		}
	}

	total_time_for_clc = total_time;
	do_div(total_time_for_clc, 100);

	cnt = 1;
	list_for_each(p, &process_headlist) {
		curr = list_entry(p, struct cpu_process_runtime_tag, list);
		t = curr->runtime * 100 + 5;
		do_div(t, total_time_for_clc);
		curr->usage = t;

		if ((curr != NULL) && (curr->task != NULL)
			&& (curr->task->pid == curr->pid)) {
			p_name = curr->task->comm;

		} else {
			snprintf(task_name, sizeof(task_name)
						, "NOT found task");
			p_name = task_name;
		}

		if (ret < PAGE_SIZE - 1) {
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret,
				"[%2d] %16s(%4d) %4d[sched] %11lld[ns]"\
				" %3d.%02d[%%]\n"
				, cnt++, p_name, curr->pid
				, curr->cnt, curr->runtime
				, curr->usage/100, curr->usage%100);
		} else
			break;
	}

	return ret;
}


static struct cpu_process_runtime_tag *search_exist_pid(unsigned int pid)
{
	struct list_head *p;
	struct cpu_process_runtime_tag *curr;

	list_for_each(p, &process_headlist) {
		curr = list_entry(p, struct cpu_process_runtime_tag, list);
		if (curr->pid == pid)
			return curr;
	}
	return NULL;
}

static void clc_process_run_time(unsigned int cpu
			, unsigned int start_cnt, unsigned int end_cnt)
{
	unsigned  int cnt = 0,  start_array_num;
	unsigned int end_array_num, end_array_num_plus1;
	unsigned int i, loop_cnt;
	struct cpu_process_runtime_tag *process_runtime_data;
	unsigned long long t1, t2;

	start_array_num
	    = (cpu_load_freq_history_view[start_cnt].task_history_cnt[cpu] + 1)
		% CPU_TASK_HISTORY_NUM;

	section_start_time
		= cpu_load_freq_history_view[start_cnt].time_stamp;
	section_end_time
		= cpu_load_freq_history_view[end_cnt].time_stamp;

	end_array_num
	= cpu_load_freq_history_view[end_cnt].task_history_cnt[cpu];
	end_array_num_plus1
	= (cpu_load_freq_history_view[end_cnt].task_history_cnt[cpu] + 1)
			% CPU_TASK_HISTORY_NUM;


	t1 = cpu_task_history_view[cpu][end_array_num].time;
	t2 = cpu_task_history_view[cpu][end_array_num_plus1].time;

	if (t2 < t1)
		end_array_num_plus1 = end_array_num;

	total_time = section_end_time - section_start_time;

	if (process_headlist.next != NULL)
		del_process_list();

	INIT_LIST_HEAD(&process_headlist);

	if (end_array_num_plus1 >= start_array_num)
		loop_cnt = end_array_num_plus1-start_array_num + 1;
	else
		loop_cnt = end_array_num_plus1
				+ CPU_TASK_HISTORY_NUM - start_array_num + 1;

	for (i = start_array_num, cnt = 0; cnt < loop_cnt; cnt++, i++) {
		if (i >= CPU_TASK_HISTORY_NUM)
			i = 0;
		process_runtime_data
			= search_exist_pid(cpu_task_history_view[cpu][i].pid);
		if (process_runtime_data == NULL)
			add_process_to_list(cpu, i);
		else {
			process_runtime_data->runtime
				+= calc_delta_time(cpu, i);
			process_runtime_data->cnt++;
		}
	}

}

static void process_sched_time_view(unsigned int cpu,
			unsigned int start_cnt, unsigned int end_cnt)
{
	unsigned  int i = 0, start_array_num;
	unsigned int end_array_num, start_array_num_for_time;

	start_array_num_for_time
	= cpu_load_freq_history_view[start_cnt].task_history_cnt[cpu];
	start_array_num
	= (cpu_load_freq_history_view[start_cnt].task_history_cnt[cpu]+1)
			% CPU_TASK_HISTORY_NUM;
	end_array_num
		= cpu_load_freq_history_view[end_cnt].task_history_cnt[cpu];

	total_time = section_end_time - section_start_time;

	if (end_cnt == start_cnt+1) {
		pr_emerg("[%d] TOTAL SECTION TIME = %lld[ns]\n\n"
			, end_cnt, total_time);
	} else {
		pr_emerg("[%d~%d] TOTAL SECTION TIME = %lld[ns]\n\n"
					, start_cnt+1, end_cnt, total_time);
	}

	for (i = start_array_num_for_time; i <= end_array_num+2; i++) {
		pr_emerg("[%d] %lld %16s %4d\n", i
			, cpu_task_history_view[cpu][i].time
			, cpu_task_history_view[cpu][i].task->comm
			, cpu_task_history_view[cpu][i].pid);
	}

}


void cpu_load_touch_event(unsigned int event)
{
	unsigned int cnt = 0;

	cnt = cpu_load_freq_history_cnt;
	if (event == 0)
		cpu_load_freq_history[cnt].touch_event = 100;
	else if (event == 1)
		cpu_load_freq_history[cnt].touch_event = 1;

}

static void store_cpu_load(unsigned int cpufreq, unsigned int cpu_load[]
						, unsigned int nr_run_avg)
{
	unsigned int j = 0, cnt = 0;
	unsigned long long t;
	unsigned long  nanosec_rem;

	if (++cpu_load_freq_history_cnt >= CPU_LOAD_HISTORY_NUM)
		cpu_load_freq_history_cnt = 0;

	cnt = cpu_load_freq_history_cnt;
	cpu_load_freq_history[cnt].cpufreq = cpufreq;

	t = cpu_clock(UINT_MAX);
	cpu_load_freq_history[cnt].time_stamp = t;
	nanosec_rem = do_div(t, 1000000000);
	sprintf(cpu_load_freq_history[cnt].time, "%2lu.%02lu"
				, (unsigned long) t, nanosec_rem / 10000000);

	for (j = 0; j < CPU_NUM; j++)
		cpu_load_freq_history[cnt].cpu_load[j] = cpu_load[j];

	cpu_load_freq_history[cnt].touch_event = 0;
	cpu_load_freq_history[cnt].nr_onlinecpu = num_online_cpus();
	cpu_load_freq_history[cnt].nr_run_avg = nr_run_avg;

	for (j = 0; j < CPU_NUM; j++) {
		cpu_load_freq_history[cnt].task_history_cnt[j]
			= cpu_task_history_cnt[j];
	}

}


static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return -1;
	while (str[count] && str[count] >= '0' && str[count] <= '9') {
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}

static ssize_t store_cpu_load_freq(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	int show_num = 0;

	show_num = atoi(buf);
	cpu_load_freq_history_show_cnt = show_num;

	return count;
}


unsigned int show_cpu_load_freq_sub(int cnt, int show_cnt, char *buf, int ret)
{
	int j, delta = 0;

	if ((cnt - show_cnt) < 0) {
		delta = cnt - show_cnt;
		cnt = CPU_LOAD_HISTORY_NUM + delta;
	} else
		cnt -= show_cnt;

	if ((cnt+1 >= CPU_LOAD_HISTORY_NUM)
			|| (cpu_load_freq_history_view[cnt+1].time == 0))
		cnt = 0;
	else
		cnt++;

	ret +=  snprintf(buf + ret, PAGE_SIZE - ret
		, "======================================="
		"========================================\n");
	ret +=  snprintf(buf + ret, PAGE_SIZE - ret
		, "    TIME    CPU_FREQ   [INDEX]\tCPU0 \tCPU1" \
			  " \tCPU2\tCPU3\tONLINE\tNR_RUN\n");\

	for (j = 0; j < show_cnt; j++) {

		if (cnt > CPU_LOAD_HISTORY_NUM-1)
			cnt = 0;

		if (ret < PAGE_SIZE - 1) {
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret
			, "%8s\t%d.%d\t[%3d]\t%3d\t%3d\t%3d\t%3d\t%3d "
				"\t%2d.%02d\n"
			, cpu_load_freq_history_view[cnt].time
			, cpu_load_freq_history_view[cnt].cpufreq/1000000
			, (cpu_load_freq_history_view[cnt].cpufreq/100000) % 10
			, cnt
			, cpu_load_freq_history_view[cnt].cpu_load[0]
			, cpu_load_freq_history_view[cnt].cpu_load[1]
			, cpu_load_freq_history_view[cnt].cpu_load[2]
			, cpu_load_freq_history_view[cnt].cpu_load[3]
			, cpu_load_freq_history_view[cnt].nr_onlinecpu
			, cpu_load_freq_history_view[cnt].nr_run_avg/100
			, cpu_load_freq_history_view[cnt].nr_run_avg%100);
		} else
			break;
		++cnt;
	}
	return ret;

}

static ssize_t show_cpu_load_freq(struct kobject *kobj,
					struct attribute *attr, char *buf)
{
	int ret = 0;
	int cnt = 0, show_cnt;
	int buffer_size = 0;

	cpu_load_freq_history_view_cnt = cpu_load_freq_history_cnt;
	cnt = cpu_load_freq_history_view_cnt - 1;
	memcpy(cpu_load_freq_history_view, cpu_load_freq_history
					, sizeof(cpu_load_freq_history_view));

	memcpy(cpu_task_history_view, cpu_task_history
					, sizeof(cpu_task_history_view));

	show_cnt = cpu_load_freq_history_show_cnt;

	ret = show_cpu_load_freq_sub(cnt, show_cnt, buf,  ret);

	buffer_size = strlen(buf);

	return buffer_size;
}


static void set_cpu_load_freq_history_array_range(const char *buf)
{
	int show_array_num = 0, select_cpu;
	char cpy_buf[80] = {0,};
	char *p1, *p2;

	p1 = strstr(buf, "-");
	p2 = strstr(buf, "c");
	if (p2 == NULL)
		p2 = strstr(buf, "C");

	if (p2 != NULL) {
		select_cpu = atoi(p2+1);
		if (select_cpu >= 0 && select_cpu < 4)
			cpu_task_history_show_select_cpu = select_cpu;
		else
			cpu_task_history_show_select_cpu = 0;
	} else
		cpu_task_history_show_select_cpu = 0;

	if (p1 != NULL) {
		strncpy(cpy_buf, buf, sizeof(cpy_buf) - 1);
		*p1 = '\0';
		cpu_task_history_show_start_cnt = atoi(cpy_buf) - 1;
		cpu_task_history_show_end_cnt = atoi(p1+1);

	} else {
		show_array_num = atoi(buf);
		cpu_task_history_show_start_cnt = show_array_num - 1;
		cpu_task_history_show_end_cnt = show_array_num;
	}

}


static ssize_t store_check_running(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{

	set_cpu_load_freq_history_array_range(buf);

	return count;
}

static ssize_t show_check_running(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	unsigned int i, ret = 0;
	int buf_size = 0;

	unsigned int start_cnt = cpu_task_history_show_start_cnt;
	unsigned int end_cnt = cpu_task_history_show_end_cnt;
	unsigned long  msec_rem;
	unsigned long long t;

	for (i = 0; i < CPU_NUM ; i++) {
		if (cpu_task_history_show_select_cpu != -1)
			if (i != cpu_task_history_show_select_cpu)
				continue;
		clc_process_run_time(i, start_cnt, end_cnt);

		t = total_time;
		msec_rem = do_div(t, 1000000);

		if (end_cnt == start_cnt+1) {
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret,
				"[%d] TOTAL SECTION TIME = %ld.%ld[ms]\n\n"
				, end_cnt	, (unsigned long)t, msec_rem);
		} else {
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret,
				"[%d~%d] TOTAL SECTION TIME = %ld.%ld[ms]\n\n"
				, start_cnt+1, end_cnt, (unsigned long)t
				, msec_rem);
		}

		if ((end_cnt) == (start_cnt + 1)) {
			ret = show_cpu_load_freq_sub((int)end_cnt+2
								, 5, buf, ret);
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret, "\n\n");
		}

		ret +=  snprintf(buf + ret, PAGE_SIZE - ret,
			"#############################"
			" CPU %d #############################\n", i);

		if (cpu_task_history_show_select_cpu == -1)
			ret = view_list(buf, ret);
		else if (i == cpu_task_history_show_select_cpu)
			ret = view_list(buf, ret);

		if (ret < PAGE_SIZE - 1)
			ret +=  snprintf(buf + ret, PAGE_SIZE - ret, "\n\n");
	}

	buf_size = strlen(buf);

	return buf_size;
}

static ssize_t store_check_running_detail(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	return count;
}

static ssize_t show_check_running_detail(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	unsigned int i;
	unsigned int start_cnt = cpu_task_history_show_start_cnt;
	unsigned int end_cnt = cpu_task_history_show_end_cnt;

	for (i = 0; i < CPU_NUM; i++) {
		pr_emerg("#############################"
			   " CPU %d #############################\n", i);
		process_sched_time_view(i, start_cnt, end_cnt);
	}
	return 0;
}
#endif

enum {
	ANDROID,
	SLP,
};

static unsigned int cpu_hotplug_policy;
static ssize_t store_hotplug_policy(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{

	if (strncmp(buf, "android", 7) == 0) {
		if (cpu_hotplug_policy != ANDROID) {
			cpu_hotplug_policy = ANDROID;
			start_rq_work();
		}

	} else if (strncmp(buf, "slp", 3) == 0) {
		if (cpu_hotplug_policy != SLP) {
			cpu_hotplug_policy = SLP;
			stop_rq_work();
		}
	}

	return count;
}

static ssize_t show_hotplug_policy(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	unsigned int ret = 0;

	if (cpu_hotplug_policy == ANDROID)
		ret +=  snprintf(buf + ret, PAGE_SIZE - ret, "android\n");
	else if (cpu_hotplug_policy == SLP)
		ret +=  snprintf(buf + ret, PAGE_SIZE - ret, "slp\n");
	else
		ret +=  snprintf(buf + ret, PAGE_SIZE - ret
				, "[ERROR] unknown policy\n");

	return ret;
}


/******************************************************************************/

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS

int  block_cpu_off(int online)
{
	int ret = 0;

	/* If Dynamic Params is enabled, We should pass down work
	   when online cpu is bigger than params_min_cpu value */
	if ((dbs_tuners_ins.dynamic_params != 0) &&
		(dbs_tuners_ins.params_min_cpu != 0)) {

		if (online > dbs_tuners_ins.params_min_cpu) {
			pr_debug("%s online is bigger than min_cpu "
				"online %d min_cpu %d\n", __func__,
				online, dbs_tuners_ins.params_min_cpu);
		} else {
			pr_debug("%s online is smaller or equal with min_cpu "
				"online %d min_cpu %d\n", __func__,
				online, dbs_tuners_ins.params_min_cpu);
			dbs_tuners_ins.params_stat_min_cpu++;
			ret = 1;
		}
	}

	return ret;

}

static ssize_t store_gov_dynamic_params_enable(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	int value;
	struct cpu_dbs_info_s *dbs_info;
	dbs_info = &per_cpu(od_cpu_dbs_info, 0);

	sscanf(buf, "%d", &value);
	mutex_lock(&dbs_info->params_mutex);
	if (value > 0) {
		dbs_tuners_ins.dynamic_params = 1;

		dbs_tuners_ins.params_stat_min_freq = 0;
		dbs_tuners_ins.params_stat_min_cpu = 0;
	} else
		dbs_tuners_ins.dynamic_params = 0;
	mutex_unlock(&dbs_info->params_mutex);
	return count;
}

static ssize_t show_gov_dynamic_params_enable(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n",
		dbs_tuners_ins.dynamic_params ? "enabled" :	"disabled");
}

static int extract_gov_dynamic_params(struct cpu_dbs_info_s *dbs_info,
			const char *buf)
{
	int i, j, ret = 0;
	char *tmp = (char *)buf;
	char *params_buf[3] = {"i", "f", "c"};

	mutex_lock(&dbs_info->params_mutex);

	if (dbs_tuners_ins.dynamic_params == 0) {
		pr_err("dynamic params disabled now\n");
		mutex_unlock(&dbs_info->params_mutex);
		return -1;
	}

	/* Clear dynamic parameters */
	dbs_tuners_ins.params_min_interval = 0;
	dbs_tuners_ins.params_min_freq = 0;
	dbs_tuners_ins.params_min_cpu = 0;

	/* extract dynamic parameters from sysfs buffer */
	for (i = 0; i < DYNAMIC_PARAMS_MAX; i++) {
		tmp = strstr(buf, params_buf[i]);
		if (tmp == NULL)
			continue;

		j = atoi(&tmp[2]);

		switch (tmp[0]) {
		case 'i':
			/* Minimum interval should be bigger than 200 mSec */
			if (j >= DYNAMIC_PARAMS_MIN_INTERVAL)
				dbs_tuners_ins.params_min_interval = j;
			 else
				pr_err("Minimum interval is too short %d\n", j);
			break;
		case 'f':
			/* Minimum frequency should be between 1.4G and 200M */
			if ((j >= DYNAMIC_PARAMS_MIN_FREQ)
				&& (j <= DYNAMIC_PARAMS_MAX_FREQ))
				dbs_tuners_ins.params_min_freq = j;
			else
				pr_err("Minimum frequency is too low"
							" or high %d\n", j);
			break;
		case 'c':
			/* Minimum running CPU should be between 1 and 4 */
			if ((j >= DYNAMIC_PARAMS_MIN_CPU)
				&& (j <= DYNAMIC_PARAMS_MAX_CPU))
				dbs_tuners_ins.params_min_cpu = j;
			 else
				pr_err("Minimum run cpu is too small"
							" or a lot %d\n", j);
			break;
		default:
			pr_err("default\n");
			break;
		}
	}

	if ((dbs_tuners_ins.params_min_interval == 0) ||
		(dbs_tuners_ins.params_min_freq == 0) ||
		(dbs_tuners_ins.params_min_cpu == 0)) {
		pr_err("%s error : dynamic parameters should be setting with "
			"some value i=%d, f=%d, c=%d\n"
			, __func__, dbs_tuners_ins.params_min_interval
			, dbs_tuners_ins.params_min_freq
			, dbs_tuners_ins.params_min_cpu);
		dbs_tuners_ins.params_min_interval = 0;
		dbs_tuners_ins.params_min_freq = 0;
		dbs_tuners_ins.params_min_cpu = 0;
		ret = -1;
	}

	mutex_unlock(&dbs_info->params_mutex);

	return ret;
}

static ssize_t store_gov_dynamic_params(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	int ret, delay;
	struct cpu_dbs_info_s *dbs_info;
	dbs_info = &per_cpu(od_cpu_dbs_info, 0);

	ret = extract_gov_dynamic_params(dbs_info, buf);
	if (ret < 0) {
		pr_err("%s error : return error value from extract_gov_dynamic_params\n"
			, __func__);
		return count;
	}

	delay = usecs_to_jiffies(dbs_tuners_ins.params_min_interval);
	pr_err("delay  %d\n", delay);
	queue_delayed_work_on(0, dynamic_params_workqueue
					, &dbs_info->params_work, delay);
	return count;
}

static ssize_t show_gov_dynamic_params(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	unsigned int ret = 0;

	ret +=  snprintf(buf, PAGE_SIZE
			, "%16s\t%10d\n%16s\t%10d\n\%16s\t%10d\n"
			, "Interval [us]   ", dbs_tuners_ins.params_min_interval
			, "min-Freq [KHz]  ", dbs_tuners_ins.params_min_freq
			, "min-CPU  [Units]", dbs_tuners_ins.params_min_cpu);

	return ret;
}

static ssize_t store_gov_dynamic_params_stat(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	return count;
}

static ssize_t show_gov_dynamic_params_stat(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	unsigned int ret = 0;
		dbs_tuners_ins.params_stat_min_freq = 0;
		dbs_tuners_ins.params_stat_min_cpu = 0;

	ret +=  snprintf(buf, PAGE_SIZE
			, "%16s\t%10d\n%16s\t%10d\n"
			, "min freq. hit  "
			, dbs_tuners_ins.params_stat_min_freq
			, "min cpu   hit  "
			, dbs_tuners_ins.params_stat_min_cpu);

	return ret;
}



#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(down_differential);
define_one_global_rw(freq_step);
define_one_global_rw(cpu_up_rate);
define_one_global_rw(cpu_down_rate);
define_one_global_rw(cpu_up_freq);
define_one_global_rw(cpu_down_freq);
define_one_global_rw(up_nr_cpus);
define_one_global_rw(max_cpu_lock);
define_one_global_rw(min_cpu_lock);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);
#ifdef CONFIG_SLP_CHECK_CPU_LOAD
define_one_global_rw(cpu_load_freq);
define_one_global_rw(check_running);
define_one_global_rw(check_running_detail);
#endif
define_one_global_rw(hotplug_policy);
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
define_one_global_rw(gov_dynamic_params_enable);
define_one_global_rw(gov_dynamic_params);
define_one_global_rw(gov_dynamic_params_stat);
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&io_is_busy.attr,
	&down_differential.attr,
	&freq_step.attr,
	&cpu_up_rate.attr,
	&cpu_down_rate.attr,
	&cpu_up_freq.attr,
	&cpu_down_freq.attr,
	&up_nr_cpus.attr,
	/* priority: hotplug_lock > max_cpu_lock > min_cpu_lock
	   Exception: hotplug_lock on early_suspend uses min_cpu_lock */
	&max_cpu_lock.attr,
	&min_cpu_lock.attr,
	&hotplug_lock.attr,
	&dvfs_debug.attr,
#ifdef CONFIG_SLP_CHECK_CPU_LOAD
	&cpu_load_freq.attr,
	&check_running.attr,
	&check_running_detail.attr,
#endif
	&hotplug_policy.attr,
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	&gov_dynamic_params_enable.attr,
	&gov_dynamic_params.attr,
	&gov_dynamic_params_stat.attr,
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
	&hotplug_freq_1_1.attr,
	&hotplug_freq_2_0.attr,
	&hotplug_freq_2_1.attr,
	&hotplug_freq_3_0.attr,
	&hotplug_freq_3_1.attr,
	&hotplug_freq_4_0.attr,
	&hotplug_rq_1_1.attr,
	&hotplug_rq_2_0.attr,
	&hotplug_rq_2_1.attr,
	&hotplug_rq_3_0.attr,
	&hotplug_rq_3_1.attr,
	&hotplug_rq_4_0.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "slp",
};

/************************** sysfs end ************************/

int get_cpu_load(unsigned int cpu, unsigned int nr_check)
{
	int cur_cnt, i, sum = 0, ret;

	cur_cnt = cpu_load_freq_history_cnt;

	for (i = 0; i < nr_check; i++) {
		sum += cpu_load_freq_history[cur_cnt].cpu_load[cpu];
		cur_cnt = get_index(cur_cnt, CPU_LOAD_HISTORY_NUM, -1);
	}

	if (nr_check != 0)
		ret = sum / nr_check;
	else
		ret = -1;

	return ret;
}

static void cpu_up_work(struct work_struct *work)
{
	int cpu;
	int online = num_online_cpus();
	int nr_up = dbs_tuners_ins.up_nr_cpus;
	int hotplug_lock = atomic_read(&g_hotplug_lock);
	if (hotplug_lock)
		nr_up = hotplug_lock - online;

	if (online == 1) {
		printk(KERN_ERR "CPU_UP 3\n");
		cpu_up(num_possible_cpus() - 1);
		nr_up -= 1;
	}

	for_each_cpu_not(cpu, cpu_online_mask) {
		if (nr_up-- == 0)
			break;
		if (cpu == 0)
			continue;
		printk(KERN_ERR "CPU_UP %d\n", cpu);
		cpu_up(cpu);
	}
}

static void cpu_down_work(struct work_struct *work)
{
	int cpu;
	int online = num_online_cpus();
	int nr_down = 1;
	int hotplug_lock = atomic_read(&g_hotplug_lock);

	if (hotplug_lock)
		nr_down = online - hotplug_lock;

	for_each_online_cpu(cpu) {
		if (cpu == 0)
			continue;
		printk(KERN_ERR "CPU_DOWN %d\n", cpu);
		cpu_down(cpu);
		if (--nr_down == 0)
			break;
	}
}

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
#ifndef CONFIG_ARCH_EXYNOS4
	if (p->cur == p->max)
		return;
#endif

	__cpufreq_driver_target(p, freq, CPUFREQ_RELATION_L);
}

/*
 * print hotplug debugging info.
 * which 1 : UP, 0 : DOWN
 */
static void debug_hotplug_check(int which, int rq_avg, int freq,
			 struct cpu_usage *usage)
{
	int cpu;
	printk(KERN_ERR "CHECK %s rq %d.%02d freq %d [", which ? "up" : "down",
	       rq_avg / 100, rq_avg % 100, freq);
	for_each_online_cpu(cpu) {
		printk(KERN_ERR "(%d, %d), ", cpu, usage->load[cpu]);
	}
	printk(KERN_ERR "]\n");
}

static int check_up(void)
{
	int num_hist = hotplug_history->num_hist;
	struct cpu_usage *usage;
	int freq, rq_avg;
	int i;
	int up_rate = dbs_tuners_ins.cpu_up_rate;
	int up_freq, up_rq;
	int min_freq = INT_MAX;
	int min_rq_avg = INT_MAX;
	int online;
	int hotplug_lock = atomic_read(&g_hotplug_lock);

	if (hotplug_lock > 0)
		return 0;

	online = num_online_cpus();
	up_freq = hotplug_freq[online - 1][HOTPLUG_UP_INDEX];
	up_rq = hotplug_rq[online - 1][HOTPLUG_UP_INDEX];

	if (online == num_possible_cpus())
		return 0;

	if (dbs_tuners_ins.max_cpu_lock != 0
		&& online >= dbs_tuners_ins.max_cpu_lock)
		return 0;

	if (dbs_tuners_ins.min_cpu_lock != 0
		&& online < dbs_tuners_ins.min_cpu_lock)
		return 1;

	if (num_hist == 0 || num_hist % up_rate)
		return 0;

	for (i = num_hist - 1; i >= num_hist - up_rate; --i) {
		usage = &hotplug_history->usage[i];

		freq = usage->freq;
		rq_avg =  usage->rq_avg;

		min_freq = min(min_freq, freq);
		min_rq_avg = min(min_rq_avg, rq_avg);

		if (dbs_tuners_ins.dvfs_debug)
			debug_hotplug_check(1, rq_avg, freq, usage);
	}

	if (min_freq >= up_freq && min_rq_avg > up_rq) {
		printk(KERN_ERR "[HOTPLUG IN] %s %d>=%d && %d>%d\n",
			__func__, min_freq, up_freq, min_rq_avg, up_rq);
		hotplug_history->num_hist = 0;
		return 1;
	}
	return 0;
}

static int check_down(void)
{
	int num_hist = hotplug_history->num_hist;
	struct cpu_usage *usage;
	int freq, rq_avg;
	int i;
	int down_rate = dbs_tuners_ins.cpu_down_rate;
	int down_freq, down_rq;
	int max_freq = 0;
	int max_rq_avg = 0;
	int online;
	int hotplug_lock = atomic_read(&g_hotplug_lock);

	if (hotplug_lock > 0)
		return 0;

	online = num_online_cpus();
	down_freq = hotplug_freq[online - 1][HOTPLUG_DOWN_INDEX];
	down_rq = hotplug_rq[online - 1][HOTPLUG_DOWN_INDEX];

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	/* If Dynamic Params is enabled, We should pass down work
	   when online cpu is bigger than params_min_cpu value */
	if ((dbs_tuners_ins.dynamic_params != 0) &&
		(dbs_tuners_ins.params_min_cpu != 0)) {

		if (online > dbs_tuners_ins.params_min_cpu) {
			pr_debug("%s online is bigger than min_cpu "
					"online %d min_cpu %d\n", __func__,
					online, dbs_tuners_ins.params_min_cpu);
		} else {
			pr_debug("%s online is smaller or equal with min_cpu "
					"online %d min_cpu %d\n", __func__,
					online, dbs_tuners_ins.params_min_cpu);
			dbs_tuners_ins.params_stat_min_cpu++;
			return 0;
		}
	}
#endif
	if (online == 1)
		return 0;

	if (dbs_tuners_ins.max_cpu_lock != 0
		&& online > dbs_tuners_ins.max_cpu_lock)
		return 1;

	if (dbs_tuners_ins.min_cpu_lock != 0
		&& online <= dbs_tuners_ins.min_cpu_lock)
		return 0;

	if (num_hist == 0 || num_hist % down_rate)
		return 0;

	for (i = num_hist - 1; i >= num_hist - down_rate; --i) {
		usage = &hotplug_history->usage[i];

		freq = usage->freq;
		rq_avg =  usage->rq_avg;

		max_freq = max(max_freq, freq);
		max_rq_avg = max(max_rq_avg, rq_avg);

		if (dbs_tuners_ins.dvfs_debug)
			debug_hotplug_check(0, rq_avg, freq, usage);
	}

	if (max_freq <= down_freq && max_rq_avg <= down_rq) {
		printk(KERN_ERR "[HOTPLUG OUT] %s %d<=%d && %d<%d\n",
			__func__, max_freq, down_freq, max_rq_avg, down_rq);
		hotplug_history->num_hist = 0;
		return 1;
	}

	return 0;
}


/******************** HOT PLUG ******************************************/

#define MAX_CPU_FREQ 1400000
#define NUM_CPUS 4



enum {
	DOWN,
	UP,
};
enum {
	NORMAL,
	NOT_OFF,
};

enum {
	UP_THRESHOLD,
	DOWN_THRESHOLD,
};

enum {
	NEED_TO_TRUN_ON_CPU,
	NEED_TO_TRUN_OFF_CPU,
	LEAVE_CPU,
};

#define CPU_THRESHOLD_RATIO (0.8)

static unsigned int cpu_threshold_level[][2] = {
	{MAX_CPU_FREQ * CPU_THRESHOLD_RATIO,  (-1) },
	{MAX_CPU_FREQ * CPU_THRESHOLD_RATIO, 200000},
	{MAX_CPU_FREQ * CPU_THRESHOLD_RATIO, 300000},
	{ (-1)      , 400000 },

};

static unsigned int num_online_cpu;
static unsigned int num_running_task;
static unsigned int min_load_freq;
static unsigned int sum_load_freq;
static unsigned int max_free_load_freq;
static unsigned int avg_load_freq;

static void cpu_updown(unsigned int cpu_num, unsigned int up_down)
{
	if (up_down == UP) {
		printk(KERN_ERR "cpu_up %d ", cpu_num);
		cpu_up(cpu_num);
	} else if (up_down == DOWN) {
		printk(KERN_ERR "cpu_down %d ", cpu_num);
		cpu_down(cpu_num);
	}
}

static void cpu_mfl_hotplug_work(struct work_struct *work)
{
	int i;
	int need_condition = LEAVE_CPU;

	unsigned int turn_on_cpu = 0, turn_off_cpu = 0;
	unsigned int cpu_on_freq, cpu_off_freq;
	static unsigned int cpu_on_freq_ctn_cnt, cpu_off_freq_ctn_cnt;

	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, mfl_hotplug_work);

	unsigned int freq_new = dbs_info->cpufreq_new;

	int hotplug_lock = atomic_read(&g_hotplug_lock);
	if (hotplug_lock > 0)
		return;

	num_online_cpu = num_online_cpus();
	num_running_task = nr_running();
	max_free_load_freq = (MAX_CPU_FREQ * 100)  - min_load_freq;
	avg_load_freq = sum_load_freq / num_online_cpu;
	cpu_on_freq = cpu_threshold_level[num_online_cpu-1][UP_THRESHOLD];
	cpu_off_freq =  cpu_threshold_level[num_online_cpu-1][DOWN_THRESHOLD];

	if (freq_new >= cpu_on_freq)
		cpu_on_freq_ctn_cnt++;
	 else
		cpu_on_freq_ctn_cnt = 0;

	if (freq_new <= cpu_off_freq)
		cpu_off_freq_ctn_cnt++;
	else
		cpu_off_freq_ctn_cnt = 0;

	cpu_load_freq_history[cpu_load_freq_history_cnt].nr_run_avg
						= num_running_task * 100;

	/* CHECK CPU ON */
	switch (num_online_cpu) {
	case 1:
		if ((num_running_task >= 2) && (cpu_on_freq_ctn_cnt >= 2))
			need_condition = NEED_TO_TRUN_ON_CPU;
		break;
	case 2:
		if ((num_running_task >= 3) && (cpu_on_freq_ctn_cnt >= 3)) {
			if (max_free_load_freq < (MAX_CPU_FREQ * 50))
				need_condition = NEED_TO_TRUN_ON_CPU;
		}
		break;
	case 3:
		if ((num_running_task >= 4) && (cpu_on_freq_ctn_cnt >= 4))  {
			if (max_free_load_freq < (MAX_CPU_FREQ * 50))
				need_condition = NEED_TO_TRUN_ON_CPU;
		}
		break;
	}

	/* CHECK CPU OFF */
	switch (num_online_cpu) {
	case 2:
		if ((cpu_off_freq_ctn_cnt >= 3)
			&& (avg_load_freq < (MAX_CPU_FREQ * 20)))
				need_condition = NEED_TO_TRUN_OFF_CPU;
		break;
	case 3:
		if ((cpu_off_freq_ctn_cnt >= 2)
			&& (avg_load_freq < (MAX_CPU_FREQ * 30)))
			need_condition = NEED_TO_TRUN_OFF_CPU;

		break;
	case 4:
		if ((cpu_off_freq_ctn_cnt >= 2)
			&& (avg_load_freq < (MAX_CPU_FREQ * 40)))
			need_condition = NEED_TO_TRUN_OFF_CPU;
		break;
	}


	if ((need_condition == NEED_TO_TRUN_ON_CPU) \
		|| (need_condition == NEED_TO_TRUN_OFF_CPU)) {
		cpu_on_freq_ctn_cnt = 0;

		if (need_condition == NEED_TO_TRUN_ON_CPU) {
			for (i = NUM_CPUS-1; i > 0; --i) {
				if (cpu_online(i) == 0) {
					turn_on_cpu = i;
					break;
				}
			}
			cpu_updown(turn_on_cpu, UP);
		} else if (need_condition == NEED_TO_TRUN_OFF_CPU) {
			for (i  = 1; i <=  NUM_CPUS - 1; ++i) {
				#if defined(CONFIG_SLP_ADAPTIVE_HOTPLUG)
					if ((num_online_cpu == 4)
							&& (i == 1)) {
						int cpu1_load, cpu2_load;
						cpu1_load = get_cpu_load(1, 5);
						cpu2_load = get_cpu_load(2, 5);
						if (cpu1_load > cpu2_load)
							i = 2;
					}
				#endif
				if (cpu_online(i) == 1) {
					turn_off_cpu = i;
					break;
				}
			}
		#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
			if (block_cpu_off(num_online_cpu) == 0)
		#endif
				cpu_updown(turn_off_cpu, DOWN);
		}
	}

}


static void check_mfl_hotplug_work(struct cpu_dbs_info_s *this_dbs_info
			, unsigned int old_freq, unsigned int new_freq)
{
	this_dbs_info->cpufreq_old = old_freq;
	this_dbs_info->cpufreq_new = new_freq;

	queue_work_on(this_dbs_info->cpu, dvfs_workqueue,
				      &this_dbs_info->mfl_hotplug_work);

}
/****************************************************************/

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int max_load_freq;

	struct cpufreq_policy *policy;
	unsigned int j;
	int num_hist = hotplug_history->num_hist;
	int max_hotplug_rate = max(dbs_tuners_ins.cpu_up_rate,
				   dbs_tuners_ins.cpu_down_rate);
	int up_threshold = dbs_tuners_ins.up_threshold;
	unsigned int nr_run_avg = 0;
	unsigned int old_freq, new_freq;


#ifdef CONFIG_SLP_CHECK_CPU_LOAD
	unsigned int cpu_load[CPU_NUM];
#endif
	policy = this_dbs_info->cur_policy;

	hotplug_history->usage[num_hist].freq = policy->cur;

	if (cpu_hotplug_policy == ANDROID)
		nr_run_avg = get_nr_run_avg();

	hotplug_history->usage[num_hist].rq_avg = nr_run_avg;
	++hotplug_history->num_hist;

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

#ifdef CONFIG_SLP_CHECK_CPU_LOAD
	for (j = 0; j < CPU_NUM; j++)
		cpu_load[j] = 0;
#endif

	min_load_freq =  (unsigned int)(-1);
	sum_load_freq = 0;

	old_freq = policy->cur;
	new_freq = policy->cur;

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		cputime64_t prev_wall_time, prev_idle_time, prev_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load, load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
		prev_wall_time = j_dbs_info->prev_cpu_wall;
		prev_idle_time = j_dbs_info->prev_cpu_idle;
		prev_iowait_time = j_dbs_info->prev_cpu_iowait;

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
							 prev_wall_time);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
							 prev_idle_time);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int) cputime64_sub(cur_iowait_time,
							   prev_iowait_time);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
						 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
				cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

#ifdef CONFIG_SLP_CHECK_CPU_LOAD
		cpu_load[j] = load;
#endif
		hotplug_history->usage[num_hist].load[j] = load;

		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;

		if (cpu_hotplug_policy == SLP) {
			if (cpu_active(j) == 1) {
				if (load_freq < min_load_freq)
					min_load_freq = load_freq;
			}
			sum_load_freq += load_freq;
		}
	}

#ifdef CONFIG_SLP_CHECK_CPU_LOAD
	store_cpu_load(policy->cur, cpu_load, nr_run_avg);
#endif

	/* Check for CPU hotplug */
	if (cpu_hotplug_policy == ANDROID) {
		if (check_up()) {
			queue_work_on(this_dbs_info->cpu, dvfs_workqueue,
				      &this_dbs_info->up_work);
		} else if (check_down()) {
			queue_work_on(this_dbs_info->cpu, dvfs_workqueue,
				      &this_dbs_info->down_work);
		}
	}
	if (hotplug_history->num_hist  == max_hotplug_rate)
		hotplug_history->num_hist = 0;

	/* Check for frequency increase */
	if (policy->cur < FREQ_FOR_RESPONSIVENESS)
		up_threshold = UP_THRESHOLD_AT_MIN_FREQ;

	if (max_load_freq > up_threshold * policy->cur) {
		int inc = (policy->max * dbs_tuners_ins.freq_step) / 100;
		int target = min(policy->max, policy->cur + inc);
		/* If switching to max speed, apply sampling_down_factor */
		if (policy->cur < policy->max && target == policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;

		if (cpu_hotplug_policy == SLP) {
			new_freq = target;
			check_mfl_hotplug_work(this_dbs_info
						, old_freq , new_freq);
		}

		dbs_freq_increase(policy, target);
		return;
	}

	/* Check for frequency decrease */
#ifndef CONFIG_ARCH_EXYNOS4
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		return;
#endif

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus DOWN_DIFFERENTIAL points under
	 * the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	    policy->cur) {
		unsigned int freq_next;
		unsigned int down_thres;

		freq_next = max_load_freq /
			(dbs_tuners_ins.up_threshold -
			 dbs_tuners_ins.down_differential);

		/* No longer fully busy, reset rate_mult */
		this_dbs_info->rate_mult = 1;

		if (freq_next < policy->min)
			freq_next = policy->min;


		down_thres = UP_THRESHOLD_AT_MIN_FREQ
			- dbs_tuners_ins.down_differential;

		if (freq_next < FREQ_FOR_RESPONSIVENESS
			&& (max_load_freq / freq_next) > down_thres)
			freq_next = FREQ_FOR_RESPONSIVENESS;

		if (cpu_hotplug_policy == SLP)
			new_freq = freq_next;

		if (policy->cur == freq_next) {
			if (cpu_hotplug_policy == SLP) {
				check_mfl_hotplug_work(this_dbs_info
							, old_freq , new_freq);
			}
			return;
		}

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	/* If Dynamic Params is enabled, We should return frequency
	   changing function, when freq_next is bigger than
	   params_min_freq value */
		if ((dbs_tuners_ins.dynamic_params != 0)
		&& (dbs_tuners_ins.params_min_freq != 0)) {
			if (freq_next > dbs_tuners_ins.params_min_freq) {
				pr_debug("%s freq_next is bigger than min_freq "
				"freq_next %d min_freq %d\n", __func__,
				freq_next, dbs_tuners_ins.params_min_freq);
			} else {
				pr_debug("%s freq_next is smaller or equal with min_freq "
				"freq_next %d min_freq %d\n", __func__,
				freq_next, dbs_tuners_ins.params_min_cpu);
				dbs_tuners_ins.params_stat_min_freq++;
				return;
			}
	}
#endif
		__cpufreq_driver_target(policy, freq_next,
					CPUFREQ_RELATION_L);
	}

	if (cpu_hotplug_policy == SLP)
		check_mfl_hotplug_work(this_dbs_info, old_freq , new_freq);

}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int delay;
	int primary_delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);
	/* We want all CPUs to do sampling nearly on
	 * same jiffy
	 */
	delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				 * dbs_info->rate_mult);
	primary_delay = delay;
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	if (delay < primary_delay / 2)
		delay = primary_delay / 2;

	queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(DEF_START_DELAY * 1000 * 1000
				     + dbs_tuners_ins.sampling_rate);
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	INIT_WORK(&dbs_info->up_work, cpu_up_work);
	INIT_WORK(&dbs_info->down_work, cpu_down_work);
	INIT_WORK(&dbs_info->mfl_hotplug_work, cpu_mfl_hotplug_work);

	queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue,
			      &dbs_info->work, delay + 2 * HZ);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
	cancel_work_sync(&dbs_info->up_work);
	cancel_work_sync(&dbs_info->down_work);
}

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
static void dynamic_params_work(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, params_work.work);

	mutex_lock(&dbs_info->params_mutex);
	dbs_tuners_ins.params_min_interval = 0;
	dbs_tuners_ins.params_min_freq = 0;
	dbs_tuners_ins.params_min_cpu = 0;
	pr_debug("%s dynamic params work called\n", __func__);
	mutex_unlock(&dbs_info->params_mutex);
}

static inline void gov_dynamic_params_timer_init
				(struct cpu_dbs_info_s *dbs_info)
{
	INIT_DELAYED_WORK(&dbs_info->params_work, dynamic_params_work);
}

static inline void gov_dynamic_params_timer_exit
				(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->params_work);
}
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

static int pm_notifier_call(struct notifier_block *this,
			    unsigned long event, void *ptr)
{
	static unsigned int prev_hotplug_lock;
	switch (event) {
	case PM_SUSPEND_PREPARE:
		prev_hotplug_lock = atomic_read(&g_hotplug_lock);
		atomic_set(&g_hotplug_lock, 1);
		apply_hotplug_lock();
		pr_debug("%s enter suspend\n", __func__);
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		atomic_set(&g_hotplug_lock, prev_hotplug_lock);
		if (prev_hotplug_lock)
			apply_hotplug_lock();
		prev_hotplug_lock = 0;
		pr_debug("%s exit suspend\n", __func__);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block pm_notifier = {
	.notifier_call = pm_notifier_call,
};

static int reboot_notifier_call(struct notifier_block *this,
				unsigned long code, void *_cmd)
{
	atomic_set(&g_hotplug_lock, 1);
	return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
	.notifier_call = reboot_notifier_call,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
unsigned int prev_freq_step;
unsigned int prev_sampling_rate;
static void cpufreq_pegasusq_early_suspend(struct early_suspend *h)
{
#if EARLYSUSPEND_HOTPLUGLOCK
	dbs_tuners_ins.early_suspend =
		atomic_read(&g_hotplug_lock);
#endif
	prev_freq_step = dbs_tuners_ins.freq_step;
	prev_sampling_rate = dbs_tuners_ins.sampling_rate;
	dbs_tuners_ins.freq_step = 20;
	dbs_tuners_ins.sampling_rate *= 4;
#if EARLYSUSPEND_HOTPLUGLOCK
	atomic_set(&g_hotplug_lock,
	    (dbs_tuners_ins.min_cpu_lock) ? dbs_tuners_ins.min_cpu_lock : 1);
	apply_hotplug_lock();
	stop_rq_work();
#endif
}
static void cpufreq_pegasusq_late_resume(struct early_suspend *h)
{
#if EARLYSUSPEND_HOTPLUGLOCK
	atomic_set(&g_hotplug_lock, dbs_tuners_ins.early_suspend);
#endif
	dbs_tuners_ins.early_suspend = -1;
	dbs_tuners_ins.freq_step = prev_freq_step;
	dbs_tuners_ins.sampling_rate = prev_sampling_rate;
#if EARLYSUSPEND_HOTPLUGLOCK
	apply_hotplug_lock();
	start_rq_work();
#endif
}
#endif

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		dbs_tuners_ins.max_freq = policy->max;
		dbs_tuners_ins.min_freq = policy->min;
		hotplug_history->num_hist = 0;

		if (cpu_hotplug_policy == ANDROID)
			start_rq_work();

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
				&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
					kstat_cpu(j).cpustat.nice;
			}
		}
		this_dbs_info->cpu = cpu;
		this_dbs_info->rate_mult = 1;
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			min_sampling_rate = MIN_SAMPLING_RATE;
			dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
			dbs_tuners_ins.io_is_busy = 0;
		}
		mutex_unlock(&dbs_mutex);

		register_reboot_notifier(&reboot_notifier);

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_timer_init(this_dbs_info);

#if !EARLYSUSPEND_HOTPLUGLOCK
		register_pm_notifier(&pm_notifier);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
		register_early_suspend(&early_suspend);
#endif
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
		mutex_init(&this_dbs_info->params_mutex);
		gov_dynamic_params_timer_init(this_dbs_info);
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

		break;

	case CPUFREQ_GOV_STOP:
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
		gov_dynamic_params_timer_exit(this_dbs_info);
		mutex_destroy(&this_dbs_info->params_mutex);
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&early_suspend);
#endif
#if !EARLYSUSPEND_HOTPLUGLOCK
		unregister_pm_notifier(&pm_notifier);
#endif

		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);

		unregister_reboot_notifier(&reboot_notifier);

		dbs_enable--;
		mutex_unlock(&dbs_mutex);

		stop_rq_work();

		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);

		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
						policy->max,
						CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
						policy->min,
						CPUFREQ_RELATION_L);

		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

static int __init cpufreq_gov_dbs_init(void)
{
	int ret;

	ret = init_rq_avg();
	if (ret)
		return ret;

	hotplug_history = kzalloc(sizeof(struct cpu_usage_history), GFP_KERNEL);
	if (!hotplug_history) {
		pr_err("%s cannot create hotplug history array\n", __func__);
		ret = -ENOMEM;
		goto err_hist;
	}

	dvfs_workqueue = create_workqueue("kslp");
	if (!dvfs_workqueue) {
		pr_err("%s cannot create dvfs_workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_dvfs_queue;
	}

#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	dynamic_params_workqueue = create_workqueue("slp_dynamic_params");
	if (!dynamic_params_workqueue) {
		pr_err("%s cannot create dynamic_params_workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_dynamic_params;
	}
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */

	ret = cpufreq_register_governor(&cpufreq_gov_slp);
	if (ret)
		goto err_reg;

#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	early_suspend.suspend = cpufreq_pegasusq_early_suspend;
	early_suspend.resume = cpufreq_pegasusq_late_resume;
#endif

	return ret;

err_reg:
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	destroy_workqueue(dynamic_params_workqueue);
err_dynamic_params:
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
	destroy_workqueue(dvfs_workqueue);
err_dvfs_queue:
	kfree(hotplug_history);
err_hist:
	kfree(rq_data);
	return ret;
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_slp);
#ifdef CONFIG_SLP_GOV_DYNAMIC_PARAMS
	destroy_workqueue(dynamic_params_workqueue);
#endif /* CONFIG_SLP_GOV_DYNAMIC_PARAMS */
	destroy_workqueue(dvfs_workqueue);
	kfree(hotplug_history);
	kfree(rq_data);
}

MODULE_AUTHOR("ByungChang Cha <bc.cha@samsung.com>");
MODULE_DESCRIPTION("'cpufreq_pegasusq' - A dynamic cpufreq/cpuhotplug governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PEGASUSQ
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
