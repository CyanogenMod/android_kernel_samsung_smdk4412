/*
 $License:
    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  $
 */

#ifndef __MPUIRQ__
#define __MPUIRQ__

#ifdef __KERNEL__
#include <linux/i2c-dev.h>
#endif

#define MPUIRQ_ENABLE_DEBUG          (1)
#define MPUIRQ_GET_INTERRUPT_CNT     (2)
#define MPUIRQ_GET_IRQ_TIME          (3)
#define MPUIRQ_GET_LED_VALUE         (4)
#define MPUIRQ_SET_TIMEOUT           (5)
#define MPUIRQ_SET_ACCEL_INFO        (6)
#define MPUIRQ_SET_FREQUENCY_DIVIDER (7)

#ifdef FEATURE_GYRO_SELFTEST_INTERRUPT
#define MPUIRQ_GET_MPUIRQ_JIFFIES       (8)
#define MPUIRQ_SET_SELFTEST_IRQ_HANDLER (9)
#define MPUIRQ_SET_MPUIRQ_HANDLER       (10)
#endif

#ifdef __KERNEL__

void mpuirq_exit(void);
int mpuirq_init(struct i2c_client *mpu_client);

#endif

#endif
