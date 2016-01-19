/*
 * secgpio_dvs.h -- Samsung GPIO debugging and verification system
 */

#ifndef __SECGPIO_DVS_H
#define __SECGPIO_DVS_H

#include <linux/types.h>

/* #define SECGPIO_SLEEP_DEBUGGING */

enum gdvs_phone_status {
	PHONE_INIT = 0,
	PHONE_SLEEP,
	GDVS_PHONE_STATUS_MAX
};

struct gpiomap_result_t {
	uint16_t *init;
	uint16_t *sleep;
};

#ifdef SECGPIO_SLEEP_DEBUGGING
enum sleepdata_phase {
	SLDATA_START = 0,
	SLDATA_DATA,
	SLDATA_END
};

struct sleepdebug_gpioinfo {
	int gpio_num;
	unsigned char io;
	unsigned char pupd;
	unsigned char lh;
};

struct sleepdebug_gpiotable {
	bool active;
	int gpio_count;
	struct sleepdebug_gpioinfo *gpioinfo;
};
#endif

struct gpio_dvs_t {
	struct gpiomap_result_t *result;
	unsigned int count;
	bool check_init;
	bool check_sleep;
	void (*check_gpio_status)(unsigned char);
#ifdef SECGPIO_SLEEP_DEBUGGING
	struct sleepdebug_gpiotable *sdebugtable;
	void (*set_sleepgpio)(void);
	void (*undo_sleepgpio)(void);
#endif
};

void gpio_dvs_check_initgpio(void);
void gpio_dvs_check_sleepgpio(void);
#ifdef SECGPIO_SLEEP_DEBUGGING
void gpio_dvs_set_sleepgpio(void);
void gpio_dvs_undo_sleepgpio(void);
#endif

#endif /* __SECGPIO_DVS_H */
