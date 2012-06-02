/*
 *  linux/include/linux/cpufreq_pegasusq.h
 *
 *  Copyright (C) 2001 Samsung Electronics co. ltd
 *    ByungChang Cha <bc.cha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_CPUFREQ_PEGASUSQ_H
#define _LINUX_CPUFREQ_PEGASUSQ_H

/* return -EINVAL when
 * 1. num_core is invalid value
 * 2. already locked with smaller num_core value
 */
int cpufreq_pegasusq_cpu_lock(int num_core);
int cpufreq_pegasusq_cpu_unlock(int num_core);

#endif
