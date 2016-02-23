/*
 * drivers/cpufreq/cpufreq_lulzactiveq.c
 *
 * Copyright (C) 2010 Google, Inc.
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
 * Author: Mike Chan (mike@android.com)
 * Edited: Tegrak (luciferanna@gmail.com)
 *
 * New Version: Roberto / Gokhanmoral
 *
 * Driver values in /sys/devices/system/cpu/cpufreq/lulzactiveq
 *
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/earlysuspend.h>
#include <asm/cputime.h>
#include <linux/suspend.h>
#include <linux/slab.h>

//a hack to make comparisons easier while having different structs in pegasusq and lulzactiveq
#define hotplug_history hotplug_lulzq_history
#define dvfs_workqueue dvfs_lulzq_workqueue

#define LULZACTIVE_VERSION	(2)
#define LULZACTIVE_AUTHOR	"tegrak"

// if you changed some codes for optimization, just write your name here.
#define LULZACTIVE_TUNER "gokhanmoral-robertobsc"

static atomic_t active_count = ATOMIC_INIT(0);

#ifdef MODULE
#include <linux/kallsyms.h>
static int (*gm_cpu_up)(unsigned int cpu);
static unsigned long (*gm_nr_running)(void);
static int (*gm_sched_setscheduler_nocheck)(struct task_struct *, int,
                              const struct sched_param *);
static void (*gm___put_task_struct)(struct task_struct *t);
static int (*gm_wake_up_process)(struct task_struct *tsk);
#define cpu_up (*gm_cpu_up)
#define nr_running (*gm_nr_running)
#define put_task_struct (*gm___put_task_struct)
#define wake_up_process (*gm_wake_up_process)
#define sched_setscheduler_nocheck (*gm_sched_setscheduler_nocheck)
#endif

struct cpufreq_lulzactive_cpuinfo {
	struct timer_list cpu_timer;
	int timer_idlecancel;
	u64 time_in_idle;
	u64 idle_exit_time;
	u64 timer_run_time;
	cputime64_t idle_prev_cpu_nice;
	int idling;
	u64 freq_change_time;
	u64 freq_change_up_time;
	u64 freq_change_down_time;
	u64 freq_change_time_in_idle;
	cputime64_t freq_change_prev_cpu_nice;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;
	struct cpufreq_frequency_table lulzfreq_table[32];
	unsigned int lulzfreq_table_size;
	unsigned int target_freq;
	int governor_enabled;
	int cpu;
	struct delayed_work work;
	struct work_struct up_work;
	struct work_struct down_work;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};

static DEFINE_PER_CPU(struct cpufreq_lulzactive_cpuinfo, cpuinfo);

/* Workqueues handle frequency scaling */
static struct task_struct *up_task;
static struct workqueue_struct *down_wq;
static struct work_struct freq_scale_down_work;
static cpumask_t up_cpumask;
static spinlock_t up_cpumask_lock;
static cpumask_t down_cpumask;
static spinlock_t down_cpumask_lock;
static struct mutex set_speed_lock;

/* Hi speed to bump to from lo speed when load burst (default max) */
static u64 hispeed_freq;

/*
 * The minimum amount of time to spend at a frequency before we can step up.
 */
#define DEFAULT_UP_SAMPLE_TIME 20 * USEC_PER_MSEC
static unsigned long up_sample_time;

/*
 * The minimum amount of time to spend at a frequency before we can step down.
 */
#define DEFAULT_DOWN_SAMPLE_TIME 40 * USEC_PER_MSEC
static unsigned long down_sample_time;

/*
 * CPU freq will be increased if measured load > inc_cpu_load;
 */
#define DEFAULT_INC_CPU_LOAD 85
static unsigned long inc_cpu_load;

/*
 * CPU freq will be decreased if measured load < dec_cpu_load;
 * not implemented yet.
 */
#define DEFAULT_DEC_CPU_LOAD 50
static unsigned long dec_cpu_load;

/*
 * Increasing frequency table index
 * zero disables and causes to always jump straight to max frequency.
 */
#define DEFAULT_PUMP_UP_STEP 2
static unsigned long pump_up_step;

/*
 * The sample rate of the timer used to increase frequency
 */
#define DEFAULT_TIMER_RATE 20 * USEC_PER_MSEC
static unsigned long timer_rate;

/*
 * Decreasing frequency table index
 * zero disables and will calculate frequency according to load heuristic.
 */
#define DEFAULT_PUMP_DOWN_STEP 1
static unsigned long pump_down_step;

/*
 * Use minimum frequency while suspended.
 */
static unsigned int early_suspended;

#define SCREEN_OFF_LOWEST_STEP 		(0xffffffff)
#define DEFAULT_SCREEN_OFF_MIN_STEP	(SCREEN_OFF_LOWEST_STEP)
static unsigned long screen_off_min_step;

#ifndef CONFIG_CPU_EXYNOS4210
#define RQ_AVG_TIMER_RATE	10
#else
#define RQ_AVG_TIMER_RATE	20
#endif

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


#define DEF_SAMPLING_RATE			(50000)
#define MIN_SAMPLING_RATE			(10000)
#define MAX_HOTPLUG_RATE			(40u)

#define DEF_MAX_CPU_LOCK			(0)
#define DEF_MIN_CPU_LOCK			(0)
#define DEF_UP_NR_CPUS				(1)
#define DEF_CPU_UP_RATE				(10)
#define DEF_CPU_DOWN_RATE			(20)
#define DEF_START_DELAY				(0)

#define HOTPLUG_DOWN_INDEX			(0)
#define HOTPLUG_UP_INDEX			(1)

#ifdef CONFIG_MACH_MIDAS
static int hotplug_rq[4][2] = {
	{0, 200}, {200, 300}, {300, 400}, {400, 0}
};

static int hotplug_freq[4][2] = {
	{0, 500000},
	{200000, 500000},
	{400000, 800000},
	{500000, 0}
};
#else
static int hotplug_rq[4][2] = {
	{0, 200}, {200, 200}, {200, 300}, {300, 0}
};

static int hotplug_freq[4][2] = {
	{0, 800000},
	{500000, 800000},
	{500000, 800000},
	{500000, 0}
};
#endif

static int cpufreq_governor_lulzactive(struct cpufreq_policy *policy,
		unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_LULZACTIVE
static
#endif
struct cpufreq_governor cpufreq_gov_lulzactive = {
    .name = "lulzactiveq",
	.governor = cpufreq_governor_lulzactive,
	.max_transition_latency = 10000000,
	.owner = THIS_MODULE,
};

struct workqueue_struct *dvfs_workqueue;
// putted tunners aggrouped into this structure, to be more clear.
static struct dbs_tuners {
    unsigned int  hotplug_sampling_rate;

    /* hotplug tuners - from lulzactiveq */
	unsigned int cpu_up_rate;
	unsigned int cpu_down_rate;
	unsigned int up_nr_cpus;
	unsigned int max_cpu_lock;
	unsigned int min_cpu_lock;
	atomic_t hotplug_lock;
	unsigned int dvfs_debug;
	unsigned int ignore_nice;

} dbs_tuners_ins = {
	.hotplug_sampling_rate=DEF_SAMPLING_RATE,

	.cpu_up_rate = DEF_CPU_UP_RATE,
	.cpu_down_rate = DEF_CPU_DOWN_RATE,
	.up_nr_cpus = DEF_UP_NR_CPUS,
	.max_cpu_lock = DEF_MAX_CPU_LOCK,
	.min_cpu_lock = DEF_MIN_CPU_LOCK,
	.hotplug_lock = ATOMIC_INIT(0),
	.dvfs_debug = 0,
	.ignore_nice = 0,
#ifdef CONFIG_HAS_EARLYSUSPEND
#endif
};

static unsigned int get_lulzfreq_table_size(struct cpufreq_lulzactive_cpuinfo *pcpu) {
	unsigned int size = 0, i;
	for (i = 0; (pcpu->freq_table[i].frequency != CPUFREQ_TABLE_END); i++) {
		unsigned int freq = pcpu->freq_table[i].frequency;
		if (freq == CPUFREQ_ENTRY_INVALID) continue;
		pcpu->lulzfreq_table[size].index = i; //in case we need it later -gm
		pcpu->lulzfreq_table[size].frequency = freq;
		size++;
	}
	pcpu->lulzfreq_table[size].index = 0;
	pcpu->lulzfreq_table[size].frequency = CPUFREQ_TABLE_END;
	return size;
}

static inline void fix_screen_off_min_step(struct cpufreq_lulzactive_cpuinfo *pcpu) {
	if (pcpu->lulzfreq_table_size <= 0) {
		screen_off_min_step = 0;
		return;
	}

	if (DEFAULT_SCREEN_OFF_MIN_STEP == screen_off_min_step)
		for(screen_off_min_step=0;
		pcpu->lulzfreq_table[screen_off_min_step].frequency != 500000;
		screen_off_min_step++);

	if (screen_off_min_step >= pcpu->lulzfreq_table_size)
		for(screen_off_min_step=0;
		pcpu->lulzfreq_table[screen_off_min_step].frequency != 500000;
		screen_off_min_step++);
}

static inline unsigned int adjust_screen_off_freq(
	struct cpufreq_lulzactive_cpuinfo *pcpu, unsigned int freq) {

	if (early_suspended && freq > pcpu->lulzfreq_table[screen_off_min_step].frequency) {
		freq = pcpu->lulzfreq_table[screen_off_min_step].frequency;
		pcpu->target_freq = pcpu->policy->cur;

		if (freq > pcpu->policy->max)
			freq = pcpu->policy->max;
		if (freq < pcpu->policy->min)
			freq = pcpu->policy->min;
	}

	return freq;
}

static void cpufreq_lulzactive_timer(unsigned long data)
{
	unsigned int delta_idle;
	unsigned int delta_time;
	int cpu_load;
	int load_since_change;
	u64 time_in_idle;
	u64 idle_exit_time;
	struct cpufreq_lulzactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, data);
	u64 now_idle;
	unsigned int new_freq;
	unsigned int index;
	cputime64_t cur_nice;
	unsigned long cur_nice_jiffies;
	unsigned long flags;
	int ret;

	smp_rmb();

	if (!pcpu->governor_enabled)
		goto exit;

    // do not let inc_cpu_load be less than dec_cpu_load.
    if (dec_cpu_load > inc_cpu_load) {
        dec_cpu_load = dec_cpu_load - (dec_cpu_load - inc_cpu_load) - 1;
    }

	/*
	 * Once pcpu->timer_run_time is updated to >= pcpu->idle_exit_time,
	 * this lets idle exit know the current idle time sample has
	 * been processed, and idle exit can generate a new sample and
	 * re-arm the timer.  This prevents a concurrent idle
	 * exit on that CPU from writing a new set of info at the same time
	 * the timer function runs (the timer function can't use that info
	 * until more time passes).
	 */
	time_in_idle = pcpu->time_in_idle;
	idle_exit_time = pcpu->idle_exit_time;
	now_idle = get_cpu_idle_time_us(data, &pcpu->timer_run_time);
	smp_wmb();

	/* If we raced with cancelling a timer, skip. */
	if (!idle_exit_time)
		goto exit;

	delta_idle = (unsigned int) cputime64_sub(now_idle, time_in_idle);
	delta_time = (unsigned int) cputime64_sub(pcpu->timer_run_time,
						  idle_exit_time);

	/*
	 * If timer ran less than 1ms after short-term sample started, retry.
	 */
	if (delta_time < 1000)
		goto rearm;

	if (dbs_tuners_ins.ignore_nice) {

		cur_nice = cputime64_sub(kstat_cpu(data).cpustat.nice,
					 pcpu->idle_prev_cpu_nice);
		/*
		 * Assumption: nice time between sampling periods will
		 * be less than 2^32 jiffies for 32 bit sys
		 */
		cur_nice_jiffies = (unsigned long)
			cputime64_to_jiffies64(cur_nice);

		delta_idle += jiffies_to_usecs(cur_nice_jiffies);

		if (dbs_tuners_ins.dvfs_debug) {
			printk(KERN_ERR "[LULZ TIMER] NICE TIME IN IDLE: %u\n",
					jiffies_to_usecs(cur_nice_jiffies));
		}

	}


	if (delta_idle > delta_time)
		cpu_load = 0;
	else
		cpu_load = 100 * (delta_time - delta_idle) / delta_time;

	delta_idle = (unsigned int) cputime64_sub(now_idle,
						pcpu->freq_change_time_in_idle);
	delta_time = (unsigned int) cputime64_sub(pcpu->timer_run_time,
						  pcpu->freq_change_time);

	if (dbs_tuners_ins.ignore_nice) {

		cur_nice = cputime64_sub(kstat_cpu(data).cpustat.nice,
					 pcpu->freq_change_prev_cpu_nice);

		/*
		 * Assumption: nice time between sampling periods will
		 * be less than 2^32 jiffies for 32 bit sys
		 */
		cur_nice_jiffies = (unsigned long)
			cputime64_to_jiffies64(cur_nice);

		delta_idle += jiffies_to_usecs(cur_nice_jiffies);

                if (dbs_tuners_ins.dvfs_debug) {
                        printk(KERN_ERR "[LULZ TIMER] NICE TIME IN RUN: %u\n",
                                        jiffies_to_usecs(cur_nice_jiffies));
                }
	}

	if ((delta_time == 0) || (delta_idle > delta_time))
		load_since_change = 0;
	else
		load_since_change =
			100 * (delta_time - delta_idle) / delta_time;

	/*
	 * Choose greater of short-term load (since last idle timer
	 * started or timer function re-armed itself) or long-term load
	 * (since last frequency change).
	 */
	if (load_since_change > cpu_load)
		cpu_load = load_since_change;

	/*
	 * START lulzactive algorithm section
	 */
	if (cpu_load >= inc_cpu_load) {
		if (pump_up_step) {
			if (pcpu->policy->cur < pcpu->policy->max) {
				ret = cpufreq_frequency_table_target(
					pcpu->policy, pcpu->lulzfreq_table,
					pcpu->policy->cur, CPUFREQ_RELATION_H,
					&index);
				if (ret < 0) {
					goto rearm;
				}

				// apply pump_up_step by tegrak
				index -= pump_up_step;
				if (index < 0)
					index = 0;

				new_freq = pcpu->lulzfreq_table[index].frequency;
			}
			else
				new_freq = pcpu->policy->max;
		}
		else {
			if (pcpu->policy->cur == pcpu->policy->min)
				new_freq = hispeed_freq;
			else
				new_freq = pcpu->policy->max * cpu_load / 100;
		}

		if (dbs_tuners_ins.dvfs_debug) {
			if (pcpu->policy->cur < pcpu->policy->max) {
				printk(KERN_ERR "[PUMP UP] %s, CPU %d, %d>=%lu, from %d to %d\n",
					__func__, pcpu->cpu, cpu_load, inc_cpu_load, pcpu->policy->cur, new_freq);
			}
		}
	}
	else if (cpu_load <= dec_cpu_load){
		if (pump_down_step) {
			ret = cpufreq_frequency_table_target(
				pcpu->policy, pcpu->lulzfreq_table,
				pcpu->policy->cur, CPUFREQ_RELATION_H,
				&index);
			if (ret < 0) {
				goto rearm;
			}

			// apply pump_down_step by tegrak
			index += pump_down_step;
			if (index >= pcpu->lulzfreq_table_size) {
				index = pcpu->lulzfreq_table_size - 1;
			}

			new_freq = (pcpu->policy->cur > pcpu->policy->min) ?
				(pcpu->lulzfreq_table[index].frequency) :
				(pcpu->policy->min);
		}
		else {
			new_freq = pcpu->policy->cur * cpu_load / 100;
		}

		if (dbs_tuners_ins.dvfs_debug) {
			if (pcpu->policy->cur > pcpu->policy->min) {
				printk(KERN_ERR "[PUMP DOWN] %s, CPU %d, %d<=%lu, from %d to %d\n",
						__func__, pcpu->cpu, cpu_load, dec_cpu_load, pcpu->policy->cur, new_freq);
			}
		}
	}
	else
	{
		new_freq = pcpu->policy->cur; //pcpu->lulzfreq_table[index].frequency;

		if (dbs_tuners_ins.dvfs_debug) {
			printk (KERN_ERR "[PUMP MAINTAIN] load = %d, %d\n", cpu_load, new_freq);
		}
	}



	if (cpufreq_frequency_table_target(pcpu->policy, pcpu->lulzfreq_table,
					   new_freq, CPUFREQ_RELATION_H,
					   &index)) {
		pr_warn_once("timer %d: cpufreq_frequency_table_target error\n",
			     (int) data);
		goto rearm;
	}
	new_freq = pcpu->lulzfreq_table[index].frequency;

	// adjust freq when screen off
	new_freq = adjust_screen_off_freq(pcpu, new_freq);

	if (pcpu->target_freq == new_freq)
		goto rearm_if_notmax;

	/*
	 * Do not scale down unless we have been at this frequency for the
	 * minimum sample time.
	 */
	if (new_freq < pcpu->target_freq) {
		if (cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_down_time)
		    < down_sample_time) {
			if (dbs_tuners_ins.dvfs_debug) {
				printk (KERN_ERR "[PUMP REARM DOWN]: CPU %d, (%llu - %llu) < %lu\n",
				pcpu->cpu, pcpu->timer_run_time, pcpu->freq_change_down_time, down_sample_time);
			}
			goto rearm;
		}
	}
	else {
		if (cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_up_time) <
		    up_sample_time) {
			if (dbs_tuners_ins.dvfs_debug)  {
				printk (KERN_ERR "[PUMP REARM UP]: CPU %d, (%llu - %llu) < %lu\n",
						pcpu->cpu, pcpu->timer_run_time, pcpu->freq_change_up_time, up_sample_time);
			}
			/* don't reset timer */
			goto rearm;
		}
	}

	if (new_freq < pcpu->target_freq) {
			if (dbs_tuners_ins.dvfs_debug) {
	            printk (KERN_ERR "[PUMP DOWN NOW] CPU %d, after %u (run: %llu - last down: %llu), last freq change: %lu\n",
			            pcpu->cpu, (unsigned int) cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_down_time),
					    pcpu->timer_run_time, pcpu->freq_change_down_time, (unsigned long) cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_time));
	        }
		pcpu->target_freq = new_freq;
		spin_lock_irqsave(&down_cpumask_lock, flags);
		cpumask_set_cpu(data, &down_cpumask);
		spin_unlock_irqrestore(&down_cpumask_lock, flags);
		queue_work(down_wq, &freq_scale_down_work);
	} else {
		if (dbs_tuners_ins.dvfs_debug) {
			printk (KERN_ERR "[PUMP UP NOW] CPU %d, after %u (run: %llu - last up: %llu), last freq change: %lu\n",
					pcpu->cpu, (unsigned int) cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_up_time),
					pcpu->timer_run_time, pcpu->freq_change_up_time, (unsigned long) cputime64_sub(pcpu->timer_run_time, pcpu->freq_change_time));
		}
		pcpu->target_freq = new_freq;
		spin_lock_irqsave(&up_cpumask_lock, flags);
		cpumask_set_cpu(data, &up_cpumask);
		spin_unlock_irqrestore(&up_cpumask_lock, flags);
		wake_up_process(up_task);
	}

rearm_if_notmax:
	/*
	 * Already set max speed and don't see a need to change that,
	 * wait until next idle to re-evaluate, don't need timer.
	 */
	if (pcpu->target_freq == pcpu->policy->max)
		goto exit;

rearm:
	if (!timer_pending(&pcpu->cpu_timer)) {
		/*
		 * If already at min: if that CPU is idle, don't set timer.
		 * Else cancel the timer if that CPU goes idle.  We don't
		 * need to re-evaluate speed until the next idle exit.
		 */
		if (pcpu->target_freq == pcpu->policy->min) {
			smp_rmb();

			if (pcpu->idling)
				goto exit;

			pcpu->timer_idlecancel = 1;
		}

		pcpu->time_in_idle = get_cpu_idle_time_us(
			data, &pcpu->idle_exit_time);
		if (dbs_tuners_ins.ignore_nice)
			pcpu->idle_prev_cpu_nice = kstat_cpu(data).cpustat.nice;
		mod_timer(&pcpu->cpu_timer,
			  jiffies + usecs_to_jiffies(timer_rate));
	}

exit:
	return;
}

static void cpufreq_lulzactive_idle_start(void)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, smp_processor_id());
	int pending;

	if (!pcpu->governor_enabled)
		return;

	pcpu->idling = 1;
	smp_wmb();
	pending = timer_pending(&pcpu->cpu_timer);

	if (pcpu->target_freq != pcpu->policy->min) {
#ifdef CONFIG_SMP
		/*
		 * Entering idle while not at lowest speed.  On some
		 * platforms this can hold the other CPU(s) at that speed
		 * even though the CPU is idle. Set a timer to re-evaluate
		 * speed so this idle CPU doesn't hold the other CPUs above
		 * min indefinitely.  This should probably be a quirk of
		 * the CPUFreq driver.
		 */
		if (!pending) {
			pcpu->time_in_idle = get_cpu_idle_time_us(
				smp_processor_id(), &pcpu->idle_exit_time);
			pcpu->timer_idlecancel = 0;
			if (dbs_tuners_ins.ignore_nice)
				pcpu->idle_prev_cpu_nice = kstat_cpu(smp_processor_id()).cpustat.nice;
			mod_timer(&pcpu->cpu_timer,
				  jiffies + usecs_to_jiffies(timer_rate));
		}
#endif
	} else {
		/*
		 * If at min speed and entering idle after load has
		 * already been evaluated, and a timer has been set just in
		 * case the CPU suddenly goes busy, cancel that timer.  The
		 * CPU didn't go busy; we'll recheck things upon idle exit.
		 */
		if (pending && pcpu->timer_idlecancel) {
			del_timer(&pcpu->cpu_timer);
			/*
			 * Ensure last timer run time is after current idle
			 * sample start time, so next idle exit will always
			 * start a new idle sampling period.
			 */
			pcpu->idle_exit_time = 0;
			pcpu->timer_idlecancel = 0;
		}
	}

}

static void cpufreq_lulzactive_idle_end(void)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, smp_processor_id());

	pcpu->idling = 0;
	smp_wmb();

	/*
	 * Arm the timer for 1-2 ticks later if not already, and if the timer
	 * function has already processed the previous load sampling
	 * interval.  (If the timer is not pending but has not processed
	 * the previous interval, it is probably racing with us on another
	 * CPU.  Let it compute load based on the previous sample and then
	 * re-arm the timer for another interval when it's done, rather
	 * than updating the interval start time to be "now", which doesn't
	 * give the timer function enough time to make a decision on this
	 * run.)
	 */
	if (timer_pending(&pcpu->cpu_timer) == 0 &&
	    pcpu->timer_run_time >= pcpu->idle_exit_time &&
	    pcpu->governor_enabled) {
		pcpu->time_in_idle =
			get_cpu_idle_time_us(smp_processor_id(),
					     &pcpu->idle_exit_time);
		pcpu->timer_idlecancel = 0;
		if (dbs_tuners_ins.ignore_nice)
			pcpu->idle_prev_cpu_nice = kstat_cpu(smp_processor_id()).cpustat.nice;
		mod_timer(&pcpu->cpu_timer,
			  jiffies + usecs_to_jiffies(timer_rate));
	}

}

static int cpufreq_lulzactive_up_task(void *data)
{
	unsigned int cpu;
	cpumask_t tmp_mask;
	unsigned long flags;
	struct cpufreq_lulzactive_cpuinfo *pcpu;


	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		spin_lock_irqsave(&up_cpumask_lock, flags);

		if (cpumask_empty(&up_cpumask)) {
			spin_unlock_irqrestore(&up_cpumask_lock, flags);
			schedule();

			if (kthread_should_stop())
				break;

			spin_lock_irqsave(&up_cpumask_lock, flags);
		}

		set_current_state(TASK_RUNNING);
		tmp_mask = up_cpumask;
		cpumask_clear(&up_cpumask);
		spin_unlock_irqrestore(&up_cpumask_lock, flags);

		for_each_cpu(cpu, &tmp_mask) {
			unsigned int j;
			unsigned int max_freq = 0;

			pcpu = &per_cpu(cpuinfo, cpu);
			smp_rmb();

			if (!pcpu->governor_enabled)
				continue;

			mutex_lock(&set_speed_lock);

			for_each_cpu(j, pcpu->policy->cpus) {
				struct cpufreq_lulzactive_cpuinfo *pjcpu =
					&per_cpu(cpuinfo, j);

				if (pjcpu->target_freq > max_freq)
					max_freq = pjcpu->target_freq;
			}

			if (max_freq != pcpu->policy->cur)
				__cpufreq_driver_target(pcpu->policy,
							max_freq,
							CPUFREQ_RELATION_H);
			mutex_unlock(&set_speed_lock);

			pcpu->freq_change_time_in_idle =
				get_cpu_idle_time_us(cpu,
						     &pcpu->freq_change_time);
			pcpu->freq_change_prev_cpu_nice = kstat_cpu(cpu).cpustat.nice;


			/*
			 *  The pcpu->freq_change_time is shared by scaling up and down,
			 *  in a way that disrespect theit both sampling.
			 *  If cpu rates are 20 scaled down and 40 and scale up;
			 *  If cpu do scale down at 20 it will renew not only the scale down sampling
			 *  but also the scale up; so scale up will not up at 40, only at 60.
			 */
			pcpu->freq_change_up_time = pcpu->freq_change_time;
		}
	}

	return 0;
}

static void cpufreq_lulzactive_freq_down(struct work_struct *work)
{
	unsigned int cpu;
	cpumask_t tmp_mask;
	unsigned long flags;
	struct cpufreq_lulzactive_cpuinfo *pcpu;

	spin_lock_irqsave(&down_cpumask_lock, flags);
	tmp_mask = down_cpumask;
	cpumask_clear(&down_cpumask);
	spin_unlock_irqrestore(&down_cpumask_lock, flags);

	for_each_cpu(cpu, &tmp_mask) {
		unsigned int j;
		unsigned int max_freq = 0;

		pcpu = &per_cpu(cpuinfo, cpu);
		smp_rmb();

		if (!pcpu->governor_enabled)
			continue;

		mutex_lock(&set_speed_lock);

		for_each_cpu(j, pcpu->policy->cpus) {
			struct cpufreq_lulzactive_cpuinfo *pjcpu =
				&per_cpu(cpuinfo, j);

			if (pjcpu->target_freq > max_freq)
				max_freq = pjcpu->target_freq;
		}

		if (max_freq != pcpu->policy->cur)
			__cpufreq_driver_target(pcpu->policy, max_freq,
						CPUFREQ_RELATION_H);

		mutex_unlock(&set_speed_lock);
		pcpu->freq_change_time_in_idle =
			get_cpu_idle_time_us(cpu,
					     &pcpu->freq_change_time);
		pcpu->freq_change_prev_cpu_nice = kstat_cpu(cpu).cpustat.nice;
		/*
		 *  The pcpu->freq_change_time is shared by scaling up and down,
		 *  in a way that disrespect theit both sampling.
		 *  If cpu rates are 20 scaled down and 40 and scale up;
		 *  If cpu do scale down at 20 it will renew not only the scale down sampling
		 *  but also the scale up; so scale up will not up at 40, only at 60.
		 */
		pcpu->freq_change_down_time = pcpu->freq_change_time;
	}
}

static ssize_t show_hispeed_freq(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", hispeed_freq);
}

static ssize_t store_hispeed_freq(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	u64 val;

	ret = strict_strtoull(buf, 0, &val);
	if (ret < 0)
		return ret;
	hispeed_freq = val;
	return count;
}

static struct global_attr hispeed_freq_attr = __ATTR(hispeed_freq, 0644,
		show_hispeed_freq, store_hispeed_freq);

// inc_cpu_load
static ssize_t show_inc_cpu_load(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", inc_cpu_load);
}

static ssize_t store_inc_cpu_load(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	if(strict_strtoul(buf, 0, &inc_cpu_load)==-EINVAL) return -EINVAL;

	if (inc_cpu_load > 100) {
		inc_cpu_load = 100;
	}
	else if (inc_cpu_load < 10) {
		inc_cpu_load = 10;
	}
	return count;
}

static struct global_attr inc_cpu_load_attr = __ATTR(inc_cpu_load, 0666,
		show_inc_cpu_load, store_inc_cpu_load);

// dec_cpu_load
static ssize_t show_dec_cpu_load(struct kobject *kobj,
                     struct attribute *attr, char *buf)
{
    return sprintf(buf, "%lu\n", dec_cpu_load);
}

static ssize_t store_dec_cpu_load(struct kobject *kobj,
            struct attribute *attr, const char *buf, size_t count)
{
    if(strict_strtoul(buf, 0, &dec_cpu_load)==-EINVAL) return -EINVAL;

    if (dec_cpu_load > 90) {
        dec_cpu_load = 90;
    }
    else if (dec_cpu_load <= 0) {
        dec_cpu_load = 10;
    }

    return count;
}

static struct global_attr dec_cpu_load_attr = __ATTR(dec_cpu_load, 0666,
		show_dec_cpu_load, store_dec_cpu_load);

// down_sample_time
static ssize_t show_down_sample_time(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", down_sample_time);
}

static ssize_t store_down_sample_time(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	if(strict_strtoul(buf, 0, &down_sample_time)==-EINVAL) return -EINVAL;
	return count;
}

static struct global_attr down_sample_time_attr = __ATTR(down_sample_time, 0666,
		show_down_sample_time, store_down_sample_time);

// up_sample_time
static ssize_t show_up_sample_time(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", up_sample_time);
}

static ssize_t store_up_sample_time(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	if(strict_strtoul(buf, 0, &up_sample_time)==-EINVAL) return -EINVAL;
	return count;
}

static struct global_attr up_sample_time_attr = __ATTR(up_sample_time, 0666,
		show_up_sample_time, store_up_sample_time);

// debug_mode
static ssize_t show_debug_mode(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "0\n");
}

static ssize_t store_debug_mode(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    dbs_tuners_ins.dvfs_debug = (input > 0);

	return count;
}

static struct global_attr debug_mode_attr = __ATTR(debug_mode, 0666,
		show_debug_mode, store_debug_mode);

// pump_up_step
static ssize_t show_pump_up_step(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", pump_up_step);
}

static ssize_t store_pump_up_step(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	if(strict_strtoul(buf, 0, &pump_up_step)==-EINVAL) return -EINVAL;
	return count;
}

static struct global_attr pump_up_step_attr = __ATTR(pump_up_step, 0666,
		show_pump_up_step, store_pump_up_step);

// pump_down_step
static ssize_t show_pump_down_step(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", pump_down_step);
}

static ssize_t store_pump_down_step(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu;

	if(strict_strtoul(buf, 0, &pump_down_step)==-EINVAL) return -EINVAL;

	pcpu = &per_cpu(cpuinfo, 0);
	// fix out of bound
	if (pcpu->lulzfreq_table_size <= pump_down_step) {
		pump_down_step = pcpu->lulzfreq_table_size - 1;
	}
	return count;
}

static struct global_attr pump_down_step_attr = __ATTR(pump_down_step, 0666,
		show_pump_down_step, store_pump_down_step);

// screen_off_min_step
static ssize_t show_screen_off_min_step(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu;

	pcpu = &per_cpu(cpuinfo, 0);
	fix_screen_off_min_step(pcpu);

	return sprintf(buf, "%lu\n", screen_off_min_step);
}

static ssize_t store_screen_off_min_step(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu;

	if(strict_strtoul(buf, 0, &screen_off_min_step)==-EINVAL) return -EINVAL;

	pcpu = &per_cpu(cpuinfo, 0);
	fix_screen_off_min_step(pcpu);

	return count;
}

static struct global_attr screen_off_min_step_attr = __ATTR(screen_off_min_step, 0666,
		show_screen_off_min_step, store_screen_off_min_step);

// author
static ssize_t show_author(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", LULZACTIVE_AUTHOR);
}

static struct global_attr author_attr = __ATTR(author, 0444,
		show_author, NULL);

// tuner
static ssize_t show_tuner(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", LULZACTIVE_TUNER);
}

static struct global_attr tuner_attr = __ATTR(tuner, 0444,
		show_tuner, NULL);

// version
static ssize_t show_version(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LULZACTIVE_VERSION);
}

static struct global_attr version_attr = __ATTR(version, 0444,
		show_version, NULL);

// freq_table
static ssize_t show_freq_table(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	struct cpufreq_lulzactive_cpuinfo *pcpu;
	char temp[64];
	int i;

	pcpu = &per_cpu(cpuinfo, 0);

	for (i = 0; i < pcpu->lulzfreq_table_size; i++) {
		sprintf(temp, "%u\n", pcpu->lulzfreq_table[i].frequency);
		strcat(buf, temp);
	}

	return strlen(buf);
}

static struct global_attr freq_table_attr = __ATTR(freq_table, 0444,
		show_freq_table, NULL);


/*
 * CPU hotplug lock interface
 */

static atomic_t g_hotplug_count = ATOMIC_INIT(0);
static atomic_t g_hotplug_lock = ATOMIC_INIT(0);

static void apply_hotplug_lock(void)
{
	int online, possible, lock, flag;
	struct work_struct *work;
    struct cpufreq_lulzactive_cpuinfo *dbs_info;

	/* do turn_on/off cpus */
    dbs_info = &per_cpu(cpuinfo, 0); /* from CPU0 */
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

int cpufreq_lulzactiveq_cpu_lock(int num_core)
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

int cpufreq_lulzactiveq_cpu_unlock(int num_core)
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

void cpufreq_lulzactiveq_min_cpu_lock(unsigned int num_core)
{
	int online, flag;
	struct cpufreq_lulzactive_cpuinfo *dbs_info;

	dbs_tuners_ins.min_cpu_lock = min(num_core, num_possible_cpus());

	dbs_info = &per_cpu(cpuinfo, 0); /* from CPU0 */
	online = num_online_cpus();
	flag = (int)num_core - online;
	if (flag <= 0)
		return;
	queue_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->up_work);
}

void cpufreq_lulzactiveq_min_cpu_unlock(void)
{
	int online, lock, flag;
	struct cpufreq_lulzactive_cpuinfo *dbs_info;

	dbs_tuners_ins.min_cpu_lock = 0;

	dbs_info = &per_cpu(cpuinfo, 0); /* from CPU0 */
	online = num_online_cpus();
	lock = atomic_read(&g_hotplug_lock);
	if (lock == 0)
		return;
	flag = lock - online;
	if (flag >= 0)
		return;
	queue_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->down_work);
}

/*
 * History of CPU usage
 */
struct cpu_usage {
	unsigned int freq;
	unsigned int load[NR_CPUS];
	unsigned int rq_avg;
};

struct cpu_usage_history {
	struct cpu_usage usage[MAX_HOTPLUG_RATE];
	unsigned int num_hist;
};

struct cpu_usage_history *hotplug_history;
// defines file parameters names.


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}

show_one(hotplug_sampling_rate,hotplug_sampling_rate);
show_one(cpu_up_rate, cpu_up_rate);
show_one(cpu_down_rate, cpu_down_rate);
#ifndef CONFIG_CPU_EXYNOS4210
show_one(up_nr_cpus, up_nr_cpus);
#endif
show_one(max_cpu_lock, max_cpu_lock);
show_one(min_cpu_lock, min_cpu_lock);
show_one(dvfs_debug, dvfs_debug);
show_one(ignore_nice_load, ignore_nice);
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
#ifndef CONFIG_CPU_EXYNOS4210
show_hotplug_param(hotplug_freq, 2, 1);
show_hotplug_param(hotplug_freq, 3, 0);
show_hotplug_param(hotplug_freq, 3, 1);
show_hotplug_param(hotplug_freq, 4, 0);
#endif

show_hotplug_param(hotplug_rq, 1, 1);
show_hotplug_param(hotplug_rq, 2, 0);
#ifndef CONFIG_CPU_EXYNOS4210
show_hotplug_param(hotplug_rq, 2, 1);
show_hotplug_param(hotplug_rq, 3, 0);
show_hotplug_param(hotplug_rq, 3, 1);
show_hotplug_param(hotplug_rq, 4, 0);
#endif

store_hotplug_param(hotplug_freq, 1, 1);
store_hotplug_param(hotplug_freq, 2, 0);
#ifndef CONFIG_CPU_EXYNOS4210
store_hotplug_param(hotplug_freq, 2, 1);
store_hotplug_param(hotplug_freq, 3, 0);
store_hotplug_param(hotplug_freq, 3, 1);
store_hotplug_param(hotplug_freq, 4, 0);
#endif

store_hotplug_param(hotplug_rq, 1, 1);
store_hotplug_param(hotplug_rq, 2, 0);
#ifndef CONFIG_CPU_EXYNOS4210
store_hotplug_param(hotplug_rq, 2, 1);
store_hotplug_param(hotplug_rq, 3, 0);
store_hotplug_param(hotplug_rq, 3, 1);
store_hotplug_param(hotplug_rq, 4, 0);
#endif

define_one_global_rw(hotplug_freq_1_1);
define_one_global_rw(hotplug_freq_2_0);
#ifndef CONFIG_CPU_EXYNOS4210
define_one_global_rw(hotplug_freq_2_1);
define_one_global_rw(hotplug_freq_3_0);
define_one_global_rw(hotplug_freq_3_1);
define_one_global_rw(hotplug_freq_4_0);
#endif

define_one_global_rw(hotplug_rq_1_1);
define_one_global_rw(hotplug_rq_2_0);
#ifndef CONFIG_CPU_EXYNOS4210
define_one_global_rw(hotplug_rq_2_1);
define_one_global_rw(hotplug_rq_3_0);
define_one_global_rw(hotplug_rq_3_1);
define_one_global_rw(hotplug_rq_4_0);
#endif


static ssize_t store_hotplug_sampling_rate(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

    dbs_tuners_ins.hotplug_sampling_rate = input; //, MIN_SAMPLING_RATE);
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


#ifndef CONFIG_CPU_EXYNOS4210
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
#endif

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
	if (input == 0)
		cpufreq_lulzactiveq_min_cpu_unlock();
	else
		cpufreq_lulzactiveq_min_cpu_lock(input);
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
		cpufreq_lulzactiveq_cpu_unlock(prev_lock);

	if (input == 0) {
		atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
		return count;
	}

	ret = cpufreq_lulzactiveq_cpu_lock(input);
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
static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	return count;
}


define_one_global_rw(hotplug_sampling_rate);
#ifndef CONFIG_CPU_EXYNOS4210
define_one_global_rw(up_nr_cpus);
#endif
define_one_global_rw(max_cpu_lock);
define_one_global_rw(min_cpu_lock);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);
define_one_global_rw(cpu_up_rate);
define_one_global_rw(cpu_down_rate);
define_one_global_rw(ignore_nice_load);


static struct attribute *lulzactive_attributes[] = {
	&hispeed_freq_attr.attr,
	&inc_cpu_load_attr.attr,
	&dec_cpu_load_attr.attr,
	&up_sample_time_attr.attr,
	&down_sample_time_attr.attr,
	&pump_up_step_attr.attr,
	&pump_down_step_attr.attr,
	&screen_off_min_step_attr.attr,
	&debug_mode_attr.attr,
	&ignore_nice_load.attr,

    /*hotplug attributes*/

    &hotplug_sampling_rate.attr,
	&cpu_up_rate.attr,
	&cpu_down_rate.attr,
#ifndef CONFIG_CPU_EXYNOS4210
	&up_nr_cpus.attr,
#endif
	/* priority: hotplug_lock > max_cpu_lock > min_cpu_lock
	   Exception: hotplug_lock on early_suspend uses min_cpu_lock */
	&max_cpu_lock.attr,
	&min_cpu_lock.attr,
	&hotplug_lock.attr,
	&dvfs_debug.attr,
	&hotplug_freq_1_1.attr,
	&hotplug_freq_2_0.attr,
#ifndef CONFIG_CPU_EXYNOS4210
	&hotplug_freq_2_1.attr,
	&hotplug_freq_3_0.attr,
	&hotplug_freq_3_1.attr,
	&hotplug_freq_4_0.attr,
#endif
	&hotplug_rq_1_1.attr,
	&hotplug_rq_2_0.attr,
#ifndef CONFIG_CPU_EXYNOS4210
	&hotplug_rq_2_1.attr,
	&hotplug_rq_3_0.attr,
	&hotplug_rq_3_1.attr,
	&hotplug_rq_4_0.attr,
#endif

	&author_attr.attr,
	&tuner_attr.attr,
	&version_attr.attr,
	&freq_table_attr.attr,
	NULL,
};

void start_lulzactiveq(void);
void stop_lulzactiveq(void);

static struct attribute_group lulzactive_attr_group = {
	.attrs = lulzactive_attributes,
    .name = "lulzactiveq",
};


static void cpu_up_work(struct work_struct *work)
{
	int cpu;
	int online = num_online_cpus();
	int nr_up = dbs_tuners_ins.up_nr_cpus;
	int min_cpu_lock = dbs_tuners_ins.min_cpu_lock;
	int hotplug_lock = atomic_read(&g_hotplug_lock);

	if (hotplug_lock && min_cpu_lock)
		nr_up = max(hotplug_lock, min_cpu_lock) - online;
	else if (hotplug_lock)
		nr_up = hotplug_lock - online;
	else if (min_cpu_lock)
		nr_up = max(nr_up, min_cpu_lock - online);

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
	int avg_freq = 0, avg_rq = 0;
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

	if (num_hist % up_rate)
		return 0;
	if(num_hist == 0) num_hist = MAX_HOTPLUG_RATE;

	for (i = num_hist - 1; i >= num_hist - up_rate; --i) {
		usage = &hotplug_history->usage[i];

		freq = usage->freq;
		rq_avg =  usage->rq_avg;

		min_freq = min(min_freq, freq);
		min_rq_avg = min(min_rq_avg, rq_avg);
		avg_rq += rq_avg;
		avg_freq += freq;

		if (dbs_tuners_ins.dvfs_debug)
			debug_hotplug_check(1, rq_avg, freq, usage);
	}
	avg_rq /= up_rate;
	avg_freq /= up_rate;

	if (avg_freq >= up_freq && avg_rq > up_rq) {
		printk(KERN_ERR "[HOTPLUG IN] %s %d>=%d && %d>%d\n",
			__func__, min_freq, up_freq, min_rq_avg, up_rq);
//		hotplug_history->num_hist = 0;
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
	int avg_freq = 0, avg_rq = 0;
	int online;
	int hotplug_lock = atomic_read(&g_hotplug_lock);

	if (hotplug_lock > 0)
		return 0;

	online = num_online_cpus();
	down_freq = hotplug_freq[online - 1][HOTPLUG_DOWN_INDEX];
	down_rq = hotplug_rq[online - 1][HOTPLUG_DOWN_INDEX];

	if (online == 1)
		return 0;

	if (dbs_tuners_ins.max_cpu_lock != 0
		&& online > dbs_tuners_ins.max_cpu_lock)
		return 1;

	if (dbs_tuners_ins.min_cpu_lock != 0
		&& online <= dbs_tuners_ins.min_cpu_lock)
		return 0;

	if (num_hist % down_rate)
		return 0;
	if(num_hist == 0) num_hist = MAX_HOTPLUG_RATE; //make it circular -gm

	for (i = num_hist - 1; i >= num_hist - down_rate; --i) {
		usage = &hotplug_history->usage[i];

		freq = usage->freq;
		rq_avg =  usage->rq_avg;

		max_freq = max(max_freq, freq);
		max_rq_avg = max(max_rq_avg, rq_avg);
		avg_rq += rq_avg;
		avg_freq += freq;

		if (dbs_tuners_ins.dvfs_debug)
			debug_hotplug_check(0, rq_avg, freq, usage);
	}
	avg_rq /= down_rate;
	avg_freq /= down_rate;

	if (avg_freq <= down_freq && avg_rq <= down_rq) {
		printk(KERN_ERR "[HOTPLUG OUT] %s %d<=%d && %d<%d\n",
			__func__, max_freq, down_freq, max_rq_avg, down_rq);
//		hotplug_history->num_hist = 0;
		return 1;
	}

	return 0;
}

static void dbs_check_cpu(struct cpufreq_lulzactive_cpuinfo *this_dbs_info)
{
	struct cpufreq_policy *policy;
	int num_hist = hotplug_history->num_hist;
	int max_hotplug_rate = MAX_HOTPLUG_RATE;

	policy = this_dbs_info->policy;

	hotplug_history->usage[num_hist].freq = policy->cur;
	hotplug_history->usage[num_hist].rq_avg = get_nr_run_avg();
	++hotplug_history->num_hist;

	/* Check for CPU hotplug */
	if (check_up()) {
		queue_work_on(this_dbs_info->cpu, dvfs_workqueue,
			      &this_dbs_info->up_work);
	} else if (check_down()) {
		queue_work_on(this_dbs_info->cpu, dvfs_workqueue,
			      &this_dbs_info->down_work);
	}
	if (hotplug_history->num_hist  == max_hotplug_rate)
		hotplug_history->num_hist = 0;
}

static void do_dbs_timer(struct work_struct *work)
{
    struct cpufreq_lulzactive_cpuinfo *dbs_info =
            container_of(work, struct cpufreq_lulzactive_cpuinfo, work.work);
	unsigned int cpu = dbs_info->cpu;
	int delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);
	/* We want all CPUs to do sampling nearly on
	 * same jiffy
	 */
    delay = usecs_to_jiffies(dbs_tuners_ins.hotplug_sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}
static inline void hotplug_timer_init(struct cpufreq_lulzactive_cpuinfo *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(DEF_START_DELAY * 1000 * 1000
            + dbs_tuners_ins.hotplug_sampling_rate);
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	INIT_WORK(&dbs_info->up_work, cpu_up_work);
	INIT_WORK(&dbs_info->down_work, cpu_down_work);

	queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue,
			      &dbs_info->work, delay + 2 * HZ);
}

static int cpufreq_governor_lulzactive(struct cpufreq_policy *policy,
		unsigned int event)
{
	int rc;
	unsigned int j;
	struct cpufreq_lulzactive_cpuinfo *pcpu;
	struct cpufreq_frequency_table *freq_table;

	switch (event) {
	case CPUFREQ_GOV_START:
		if (!cpu_online(policy->cpu))
			return -EINVAL;

        /* init works and timer of each cpu */

        hotplug_history->num_hist = 0;
        start_rq_work();
		freq_table =
			cpufreq_frequency_get_table(policy->cpu);

		for_each_cpu(j, policy->cpus) {
			pcpu = &per_cpu(cpuinfo, j);
			pcpu->policy = policy;
			pcpu->target_freq = policy->cur;
			pcpu->freq_table = freq_table;
			pcpu->freq_change_time_in_idle =
				get_cpu_idle_time_us(j,
						     &pcpu->freq_change_time);
			pcpu->freq_change_up_time = pcpu->freq_change_down_time = pcpu->freq_change_time;
			pcpu->governor_enabled = 1;
			smp_wmb();
			pcpu->lulzfreq_table_size = get_lulzfreq_table_size(pcpu);

			// fix invalid screen_off_min_step
			fix_screen_off_min_step(pcpu);
			if (dbs_tuners_ins.ignore_nice) {
				pcpu->freq_change_prev_cpu_nice =
					kstat_cpu(j).cpustat.nice;
			}
		}

		if (!hispeed_freq)
			hispeed_freq = policy->max;

		/*  starting hotplug */
		pcpu = &per_cpu(cpuinfo, policy->cpu);
		pcpu->cpu = policy->cpu;
		mutex_init(&pcpu->timer_mutex);
		hotplug_timer_init (pcpu);
		/*
		 * Do not register the idle hook and create sysfs
		 * entries if we have already done so.
		 */
		if (atomic_inc_return(&active_count) > 1)
			return 0;
        start_lulzactiveq();

		rc = sysfs_create_group(cpufreq_global_kobject,
				&lulzactive_attr_group);
		if (rc)
			return rc;

		break;

	case CPUFREQ_GOV_STOP:
		/* finish works from each cpu */
		pcpu = &per_cpu(cpuinfo, policy->cpu);
		cancel_delayed_work_sync(&pcpu->work);
		cancel_work_sync(&pcpu->up_work);
		cancel_work_sync(&pcpu->down_work);
		mutex_destroy(&pcpu->timer_mutex);
		for_each_cpu(j, policy->cpus) {
			pcpu = &per_cpu(cpuinfo, j);
			pcpu->governor_enabled = 0;
			smp_wmb();
			del_timer_sync(&pcpu->cpu_timer);

			/*
			 * Reset idle exit time since we may cancel the timer
			 * before it can run after the last idle exit time,
			 * to avoid tripping the check in idle exit for a timer
			 * that is trying to run.
			 */
			pcpu->idle_exit_time = 0;
		}

		flush_work(&freq_scale_down_work);
        stop_rq_work();
		if (atomic_dec_return(&active_count) > 0)
			return 0;

		sysfs_remove_group(cpufreq_global_kobject,
				&lulzactive_attr_group);
        stop_lulzactiveq();
		break;

	case CPUFREQ_GOV_LIMITS:
		if (policy->max < policy->cur)
			__cpufreq_driver_target(policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > policy->cur)
			__cpufreq_driver_target(policy,
					policy->min, CPUFREQ_RELATION_L);
		break;
	}
	return 0;
}

static int cpufreq_lulzactive_idle_notifier(struct notifier_block *nb,
					     unsigned long val,
					     void *data)
{
	switch (val) {
	case IDLE_START:
		cpufreq_lulzactive_idle_start();
		break;
	case IDLE_END:
		cpufreq_lulzactive_idle_end();
		break;
	}

	return 0;
}

static struct notifier_block cpufreq_lulzactive_idle_nb = {
	.notifier_call = cpufreq_lulzactive_idle_notifier,
};

static void lulzactive_early_suspend(struct early_suspend *handler) {
	early_suspended = 1;
}

static void lulzactive_late_resume(struct early_suspend *handler) {
	early_suspended = 0;
}

static struct early_suspend lulzactive_power_suspend = {
	.suspend = lulzactive_early_suspend,
	.resume = lulzactive_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};

void start_lulzactiveq(void)
{
	//it is more appropriate to start the up_task thread after starting the governor -gm
	unsigned int i, index500, index800;
	struct cpufreq_lulzactive_cpuinfo *pcpu;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

	if( pump_up_step == 0 )
	{
		pcpu = &per_cpu(cpuinfo, 0);
		cpufreq_frequency_table_target(
				pcpu->policy, pcpu->lulzfreq_table,
				500000, CPUFREQ_RELATION_H,
				&index500);
		cpufreq_frequency_table_target(
				pcpu->policy, pcpu->lulzfreq_table,
				800000, CPUFREQ_RELATION_H,
				&index800);
		for(i=index800;i<index500;i++)
		{
		  if(pcpu->lulzfreq_table[i].frequency==CPUFREQ_ENTRY_INVALID) continue;
		  pump_up_step++;
		}
	}
	if( pump_down_step == 0 )
	{
		pump_down_step = pump_up_step;
	}

	up_task = kthread_create(cpufreq_lulzactive_up_task, NULL,
                 "klulzqup");

	sched_setscheduler_nocheck(up_task, SCHED_FIFO, &param);
	get_task_struct(up_task);

	idle_notifier_register(&cpufreq_lulzactive_idle_nb);
	register_early_suspend(&lulzactive_power_suspend);
}

void stop_lulzactiveq(void)
{
	//cleanup the thread after stopping the governor -gm
	kthread_stop(up_task);
	put_task_struct(up_task);

	idle_notifier_unregister(&cpufreq_lulzactive_idle_nb);
	unregister_early_suspend(&lulzactive_power_suspend);
	pump_up_step = DEFAULT_PUMP_UP_STEP;
	pump_down_step = DEFAULT_PUMP_DOWN_STEP;
}

static int __init cpufreq_lulzactive_init(void)
{
    unsigned int i; int ret;
    struct cpufreq_lulzactive_cpuinfo *pcpu;
	up_sample_time = DEFAULT_UP_SAMPLE_TIME;
	down_sample_time = DEFAULT_DOWN_SAMPLE_TIME;
	inc_cpu_load = DEFAULT_INC_CPU_LOAD;
	dec_cpu_load = DEFAULT_DEC_CPU_LOAD;
	pump_up_step = DEFAULT_PUMP_UP_STEP;
	pump_down_step = DEFAULT_PUMP_DOWN_STEP;
	early_suspended = 0;
	screen_off_min_step = DEFAULT_SCREEN_OFF_MIN_STEP;
	timer_rate = DEFAULT_TIMER_RATE;
	ret = init_rq_avg();
	if(ret) return ret;

#ifdef MODULE
	gm_cpu_up = (int (*)(unsigned int cpu))kallsyms_lookup_name("cpu_up");
	gm_nr_running = (unsigned long (*)(void))kallsyms_lookup_name("nr_running");
	gm_sched_setscheduler_nocheck = (int (*)(struct task_struct *, int,
		const struct sched_param *))kallsyms_lookup_name("sched_setscheduler_nocheck");
	gm___put_task_struct = (void (*)(struct task_struct *))kallsyms_lookup_name("__put_task_struct");
	gm_wake_up_process = (int (*)(struct task_struct *))kallsyms_lookup_name("wake_up_process");
#endif
	hotplug_history = kzalloc(sizeof(struct cpu_usage_history), GFP_KERNEL);
	if (!hotplug_history) {
		pr_err("%s cannot create lulzactive hotplug history array\n", __func__);
		ret = -ENOMEM;
		goto err_queue;
	}

	dvfs_workqueue = create_workqueue("klulzactiveq");
	if (!dvfs_workqueue) {
		pr_err("%s cannot create workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_freeuptask;
	}

	/* Initalize per-cpu timers */
	for_each_possible_cpu(i) {
		pcpu = &per_cpu(cpuinfo, i);
		init_timer(&pcpu->cpu_timer);
		pcpu->cpu_timer.function = cpufreq_lulzactive_timer;
		pcpu->cpu_timer.data = i;
	}

	/* No rescuer thread, bind to CPU queuing the work for possibly
	   warm cache (probably doesn't matter much). */
	down_wq = alloc_workqueue("klulzactiveq_down", 0, 1);

	if (!down_wq)
		goto err_freeuptask;

	INIT_WORK(&freq_scale_down_work,
		  cpufreq_lulzactive_freq_down);

	spin_lock_init(&up_cpumask_lock);
	spin_lock_init(&down_cpumask_lock);
	mutex_init(&set_speed_lock);

	return cpufreq_register_governor(&cpufreq_gov_lulzactive);

err_freeuptask:
	kfree(hotplug_history);
	put_task_struct(up_task);
err_queue:
	kfree(rq_data);
	return (ret) ? ret : -ENOMEM;
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_LULZACTIVE
fs_initcall(cpufreq_lulzactive_init);
#else
module_init(cpufreq_lulzactive_init);
#endif

static void __exit cpufreq_lulzactive_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_lulzactive);
	kthread_stop(up_task);
	put_task_struct(up_task);
	destroy_workqueue(dvfs_workqueue);
	destroy_workqueue(down_wq);
	kfree(hotplug_history);
	kfree(rq_data);
}

module_exit(cpufreq_lulzactive_exit);

MODULE_AUTHOR("Tegrak <luciferanna@gmail.com>");
MODULE_DESCRIPTION("'lulzactiveQ' - improved lulzactive governor with hotplug logic");
MODULE_LICENSE("GPL");
