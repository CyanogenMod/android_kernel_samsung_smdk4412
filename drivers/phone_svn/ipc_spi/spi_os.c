/**************************************************************
	spi_os.c

	adapt os api and spi api

	This is MATER side.
***************************************************************/

/**************************************************************
	Preprocessor by common
***************************************************************/

#include "spi_main.h"
#include "spi_os.h"


/**************************************************************
	Preprocessor by platform
	(Android)
***************************************************************/

#include <linux/vmalloc.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/time.h>

/**********************************************************
Prototype		void * spi_os_malloc ( unsigned int length )

Type		function

Description	allocate memory

Param input	length	: length of size

Return value	0		: fail
			(other)	: address of memory allocated
***********************************************************/
void *spi_os_malloc(unsigned int length)
{
	if (length <= 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_malloc fail : len 0\n"));
		return 0;
	}

	return kmalloc(length, GFP_ATOMIC);
}


/*====================================
Prototype		void * spi_os_vmalloc ( unsigned int length )

Type		function

Description	allocate memory with vmalloc

Param input	length	: length of size

Return value	0		: fail
			(other)	: address of memory allocated
====================================*/
void *spi_os_vmalloc(unsigned int length)
{
	if (length <= 0) {
		SPI_OS_ERROR(("spi_os_malloc fail : length is 0\n"));
		return 0;
	}
	return vmalloc(length);
}


/**********************************************************
Prototype		int spi_os_free ( void * addr )

Type		function

Description	free memory

Param input	addr	: address of memory to be released

Return value	0	: fail
			1	: success
***********************************************************/
int spi_os_free(void *addr)
{
	if (addr == 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_free fail : addr is 0\n"));
		return 0;
	}

	kfree(addr);
	return 1;
}


/**********************************************************
Prototype		int spi_os_vfree ( void * addr )

Type		function

Description	free memory with vfree

Param input	addr	: address of memory to be released

Return value	0	: fail
			1	: success
***********************************************************/
int spi_os_vfree(void *addr)
{
	if (addr == 0) {
		SPI_OS_ERROR(("spi_os_free fail : addr is 0\n"));
		return 0;
	}

#ifdef SPI_FEATURE_OMAP4430
	vfree(addr);
#elif defined SPI_FEATURE_SC8800G
	SCI_FREE(addr);
#endif

	return 1;
}


/**********************************************************
Prototype	int spi_os_memcpy ( void * dest, void * src, unsigned int length )

Type		function

Description	copy memory

Param input	dest	: address of memory to be save
			src	: address of memory to copy
			length	: length of memory to copy

Return value	0	: fail
			1	: success
***********************************************************/
int spi_os_memcpy(void *dest, void *src, unsigned int length)
{
	if (dest == 0 || src == 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_memcpy fail\n"));
		return 0;
	}

	memcpy(dest, src, length);

	return 1;
}


/**********************************************************
Prototype	void * spi_os_memset ( void * addr, int value, unsigned int length )

Type		function

Description	set memory as parameter

Param input	addr	: address of memory to be set
			value	: value to set
			length	: length of memory to be set

Return value	(none)
***********************************************************/
void *spi_os_memset(void *addr, int value, unsigned int length)
{
	if (addr == 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_memcpy fail\n"));
		return 0;
	}

	return memset(addr, value, length);
}


/**********************************************************
Prototype		int spi_os_create_mutex( char * name, unsigned int priority_inherit )

Type		function

Description	create mutex

Param input	name	: name of mutex
			priority_inherit	: mutex inheritance

Return value	id of mutex created
***********************************************************/
int spi_os_create_mutex(char *name, unsigned int priority_inherit)
{
	if (name == 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_memcpy fail\n"));
		return 0;
	}

	return 0;
}


/**********************************************************
Prototype		int spi_os_delete_mutex ( int pmutex )

Type		function

Description	delete mutex

Param input	pmutex	: id of mutex

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_delete_mutex(int pmutex)
{
	return 0;
}


/**********************************************************
Prototype		int  spi_os_acquire_mutex ( int pmutex, unsigned int wait  )

Type		function

Description	acquire mutex

Param input	pmutex	: id of mutex to acquire
			wait		: mutex waiting time

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_acquire_mutex(int pmutex, unsigned int wait)
{
	return 0;
}


/**********************************************************
Prototype		int spi_os_release_mutex (int pmutex )

Type		function

Description	release mutex

Param input	pmutex	: id of mutex to release

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_release_mutex(int pmutex)
{
	return 0;
}


/**********************************************************
Prototype		int spi_os_create_sem ( char * name, unsigned int init_count)

Type		function

Description	create semaphore

Param input	name	: name of semaphore
			init_count : semaphore count

Return value	id of semaphore created
***********************************************************/
int spi_os_create_sem(char *name, unsigned int init_count)
{
	if (name == 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_memcpy fail\n"));
		return 0;
	}

	return 0;
}


/**********************************************************
Prototype		int spi_os_delete_sem ( int sem )

Type		function

Description	delete semaphore

Param input	sem	: id of semaphore to delete

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_delete_sem(int sem)
{
	return 0;
}


/**********************************************************
Prototype		int spi_os_acquire_sem (int sem, unsigned int wait )

Type		function

Description	acquire semaphore

Param input	sem	: id of semaphore to acquire
			wait	: mutex waiting time

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_acquire_sem(int sem, unsigned int wait)
{
	return 0;
}


/**********************************************************
Prototype		int spi_os_release_sem (int sem )

Type		function

Description	release semaphore

Param input	sem	: id of semaphore to release

Return value	0 : success
			1 : fail
***********************************************************/
int spi_os_release_sem(int sem)
{
	return 0;
}


/**********************************************************
Prototype		void spi_os_sleep ( unsigned long msec )

Type		function

Description	sleep os

Param input	msec	: time to sleep

Return value	(none)
***********************************************************/
void spi_os_sleep(unsigned long msec)
{
	if (msec <= 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_sleep fail\n"));
		return;
	}

	msleep(msec);
}


/**********************************************************
Prototype		void spi_os_loop_delay ( unsigned long cnt )

Type		function

Description	delay task with loop

Param input	cnt	: delay count

Return value	(none)
***********************************************************/
void spi_os_loop_delay(unsigned long cnt)
{
	unsigned int timeout;
	timeout = 0;
	while (++timeout < cnt)
		;
}


/**********************************************************
Prototype		void * spi_os_create_timer

Type		function

Description	create timer

Param input	name : timer name
			callback : timer callback function
			param : timer param
			duration : timer expire duration

Return value	timer ptr
***********************************************************/
int spi_os_create_timer(void *timer, char *name,
		SPI_OS_TIMER_T callback, int param, unsigned long duration)
{
	struct timer_list *tm = timer;


	if (name == 0 || callback == 0 || param <= 0 || duration <= 0) {
		SPI_OS_ERROR(("[SPI] ERROR : spi_os_create_timer fail\n"));
		return 0;
	}

	init_timer(tm);

	tm->expires = jiffies + ((duration * HZ) / 1000);
	tm->data = (unsigned long) param;
	tm->function = callback;

	return 1;
}


/**********************************************************
Prototype		int spi_os_start_timer

Type		function

Description	start timer

Param input	timer : timer ptr
			callback : timer callback function
			param : timer param
			duration : timer expire duration

Return value	1 : success
			0 : fail
***********************************************************/
int spi_os_start_timer(void *timer, SPI_OS_TIMER_T callback,
		int param, unsigned long duration)
{
	add_timer((struct timer_list *) timer);
	return 1;
}


/**********************************************************
Prototype		int spi_os_stop_timer (void * timer)

Type		function

Description	stop timer

Param input	timer : timer ptr

Return value	1 : success
			0 : fail
***********************************************************/
int spi_os_stop_timer(void *timer)
{
	int value = 0;

	value = del_timer((struct timer_list *) timer);

	return value;
}


/**********************************************************
Prototype		int spi_os_delete_timer (void * timer)

Type		function

Description	delete timer

Param input	timer : timer ptr

Return value	1 : success
			0 : fail
***********************************************************/
int spi_os_delete_timer(void *timer)
{
	return 1;
}


/**********************************************************
Prototype		unsigned long spi_os_get_tick (void)

Type		function

Description	get system tick

Param input	(none)

Return value	system tick

***********************************************************/
unsigned long spi_os_get_tick(void)
{
	unsigned long tick = 0;

	tick = jiffies_to_msecs(jiffies);
	return tick;
}


/**********************************************************
Prototype		void spi_os_get_tick_by_log (char * name)

Type		function

Description	print tick time to log

Param input	name : print name

Return value	(none)
***********************************************************/
void spi_os_get_tick_by_log(char *name)
{
	SPI_OS_TRACE(("[SPI] %s tick %lu ms\n", name, spi_os_get_tick()));

}


/**********************************************************
Prototype		void spi_os_trace_dump (char * name, void * data, int length)

Description	print buffer value by hex code
			if buffer size too big, it change to....
			and print 64 byte of front and 64 byte of tail

Param input	name	: print name
			data		: buffer for print
			length	: print length

Return value	(none)
***********************************************************/
void spi_os_trace_dump(char *name, void *data, int length)
{
#ifdef SPI_FEATURE_DEBUG
	#define SPI_OS_TRACE_DUMP_PER_LINE	16
	#define SPI_OS_TRACE_MAX_LINE			8
	#define SPI_OS_TRACE_HALF_LINE
		(SPI_OS_TRACE_MAX_LINE / 2)
	#define SPI_OS_TRACE_MAX_DUMP_SIZE
		(SPI_OS_TRACE_DUMP_PER_LINE*SPI_OS_TRACE_MAX_LINE)

	int i = 0, lines = 0, halflinemode = 0;

	char buf[SPI_OS_TRACE_DUMP_PER_LINE * 3 + 1];
	char *pdata = NULL;

	char ch = 0;

	SPI_OS_TRACE_MID(("[SPI] spi_os_trace_dump (%s length[%d])\n",
		name, length));

	spi_os_memset(buf, 0x00, sizeof(buf));

	if (length > SPI_OS_TRACE_MAX_DUMP_SIZE)
		halflinemode = 1;

	pdata = data;
	for (i = 0 ; i < length ; i++) {
		if ((i != 0) && ((i % SPI_OS_TRACE_DUMP_PER_LINE) == 0)) {
			buf[SPI_OS_TRACE_DUMP_PER_LINE*3] = 0;
			SPI_OS_TRACE_MID(("%s\n", buf));
			spi_os_memset(buf, 0x00, sizeof(buf));
			lines++;
			if (SPI_OS_TRACE_HALF_LINE == lines
				&& halflinemode == 1) {
				SPI_OS_TRACE_MID((" ......\n"));
				pdata += (length - SPI_OS_TRACE_MAX_DUMP_SIZE);
				i += (length - SPI_OS_TRACE_MAX_DUMP_SIZE);
			}
		}

		ch = (*pdata&0xF0)>>4;
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3] =
			((ch > 9) ? (ch-10 + 'A') : (ch +  '0'));
		ch = (*pdata&0x0F);
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3+1] =
			((ch > 9) ? (ch-10 + 'A') : (ch +  '0'));
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3+2] = 0x20;
		pdata++;
	}

	if (buf[0] != '\0')
		SPI_OS_TRACE_MID(("%s\n", buf));

	#undef SPI_OS_TRACE_DUMP_PER_LINE
	#undef SPI_OS_TRACE_MAX_LINE
	#undef SPI_OS_TRACE_HALF_LINE
	#undef SPI_OS_TRACE_MAX_DUMP_SIZE
#endif
}


/**********************************************************
Prototype		void spi_os_trace_dump (char * name, void * data, int length)

Description	print buffer value by hex code as SPI_OS_TRACE

			this function print only 16 byte

Param input	name	: print name
			data		: buffer for print
			length	: print length

Return value	(none)
***********************************************************/

void spi_os_trace_dump_low(char *name, void *data, int length)
{
	#define SPI_OS_TRACE_DUMP_PER_LINE 16

	int i = 0;
	char buf[SPI_OS_TRACE_DUMP_PER_LINE * 3 + 1] = {0,};
	char *pdata = NULL;
	char ch = 0;

	SPI_OS_ERROR(("[SPI] spi_os_trace_dump_low (%s length[%d])\n",
		name, length));

	spi_os_memset(buf, 0x00, sizeof(buf));

	if (length > SPI_OS_TRACE_DUMP_PER_LINE)
		length = SPI_OS_TRACE_DUMP_PER_LINE;

	pdata = data;
	for (i = 0 ; i < length ; i++) {
		ch = (*pdata&0xF0)>>4;
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3] =
			((ch > 9) ? (ch-10 + 'A') : (ch +  '0'));
		ch = (*pdata&0x0F);
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3+1] =
			((ch > 9) ? (ch-10 + 'A') : (ch +  '0'));
		buf[(i%SPI_OS_TRACE_DUMP_PER_LINE)*3+2] = 0x20;
		pdata++;
	}

	if (buf[0] != '\0')
		SPI_OS_ERROR(("%s\n", buf));

	#undef SPI_OS_TRACE_DUMP_PER_LINE
}
