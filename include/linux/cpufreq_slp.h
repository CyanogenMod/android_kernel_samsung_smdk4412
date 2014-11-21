
#ifndef _LINUX_CPUFREQ_SLP_H
#define _LINUX_CPUFREQ_SLP_H

void cpu_load_touch_event(unsigned int event);

#if defined(CONFIG_SLP_CHECK_CPU_LOAD)
void __slp_store_task_history(unsigned int cpu, struct task_struct *task);

static inline void slp_store_task_history(unsigned int cpu
					, struct task_struct *task)
{
	__slp_store_task_history(cpu, task);
}
#else
static inline void slp_store_task_history(unsigned int cpu
					, struct task_struct *task)
{

}
#endif


#endif
