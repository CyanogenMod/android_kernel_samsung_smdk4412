#ifndef __LINUX_CM36653_H
#define __CM36653_H__

#ifdef __KERNEL__
struct cm36653_platform_data {
	int (*cm36653_led_on) (bool);
	u16 (*cm36653_get_threshold)(void);
	u16 (*cm36653_get_cal_threshold)(void);
	int irq;		/* proximity-sensor irq gpio */
};
#endif
#endif
