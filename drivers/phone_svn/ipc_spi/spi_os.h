#ifndef __SPI_OS_H__
#define __SPI_OS_H__

#include "spi_main.h"

#include <linux/kernel.h>
#include <linux/workqueue.h>


#define SPI_OS_ASSERT(x) printk x
#define SPI_OS_ERROR(x) printk x

#ifdef SPI_FEATURE_NOLOG
#define SPI_OS_TRACE_MID(x)
#define SPI_OS_TRACE(x)
#else
#define SPI_OS_TRACE(x) printk x
#ifdef SPI_FEATURE_DEBUG
#define SPI_OS_TRACE_MID(x) printk x
#else
#define SPI_OS_TRACE_MID(x)
#endif /* SPI_FEATURE_DEBUG */
#endif /* SPI_FEATURE_NOLOG */

struct spi_work {
	struct work_struct work;
	int signal_code;
};

struct spi_os_msg {
	unsigned int signal_code;
	unsigned int data_length;
	void *data;
};

typedef void (*SPI_OS_TIMER_T)(unsigned long);
/* Inherit option */
#define SPI_OS_MUTEX_NO_INHERIT 0
#define SPI_OS_MUTEX_INHERIT 1
/* Wait option. */
#define SPI_OS_MUTEX_NO_WAIT                 /* TBD */
#define SPI_OS_MUTEX_WAIT_FOREVER       /* TBD */
#define SPI_OS_TIMER_CALLBACK(X)  void X(unsigned long param)

/* Memory */
extern void *spi_os_malloc(unsigned int length);
extern void *spi_os_vmalloc(unsigned int length);
extern int      spi_os_free(void *addr);
extern int		spi_os_vfree(void *addr);
extern int      spi_os_memcpy(void *dest, void *src, unsigned int length);
extern void *spi_os_memset(void *addr, int value, unsigned int length);

/* Mutex */
extern int spi_os_create_mutex(char *name,
		unsigned int priority_inherit);
extern int spi_os_delete_mutex(int sem);
extern int spi_os_acquire_mutex(int sem, unsigned int wait);
extern int spi_os_release_mutex(int sem);


/* Semaphore */
extern int spi_os_create_sem(char *name, unsigned int init_count);
extern int spi_os_delete_sem(int sem);
extern int spi_os_acquire_sem(int sem, unsigned int wait);
extern int spi_os_release_sem(int sem);

/* Timer */
extern void spi_os_sleep(unsigned long msec);
extern void spi_os_loop_delay(unsigned long cnt);
extern int spi_os_create_timer(void *timer, char *name,
		SPI_OS_TIMER_T callback, int param, unsigned long duration);
extern int spi_os_start_timer(void *timer, SPI_OS_TIMER_T callback,
		int param, unsigned long duration);
extern int spi_os_stop_timer(void *timer);
extern int spi_os_delete_timer(void *timer);

/* Log */
extern unsigned long spi_os_get_tick(void);
extern void spi_os_get_tick_by_log(char *name);
extern void spi_os_trace_dump(char *name, void *data, int length);
extern void spi_os_trace_dump_low(char *name, void *data, int length);
#endif
