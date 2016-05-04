#ifndef __U1_CYPRESS_GPIO_H__
#define __U1_CYPRESS_GPIO_H__

extern unsigned int system_rev;

#if defined (CONFIG_MACH_U1_REV00PRE) || defined (CONFIG_MACH_U1_REV01PRE) \
	|| defined (CONFIG_MACH_U1_REV00)
#define _3_GPIO_TOUCH_EN	S5PV310_GPE3(3)
#define _3_GPIO_TOUCH_INT	S5PV310_GPE3(7)
#define _3_GPIO_TOUCH_INT_AF	S3C_GPIO_SFN(0xf)
#define _3_TOUCH_SDA_28V	S5PV310_GPE4(0)
#define _3_TOUCH_SCL_28V	S5PV310_GPE4(1)

#define IRQ_TOUCH_INT		gpio_to_irq(_3_GPIO_TOUCH_INT)

#else

#define _3_GPIO_TOUCH_EN	-1
#define _3_GPIO_TOUCH_INT	GPIO_3_TOUCH_INT
#define _3_GPIO_TOUCH_INT_AF	S3C_GPIO_SFN(0xf)
#define _3_TOUCH_SDA_28V	GPIO_3_TOUCH_SDA
#define _3_TOUCH_SCL_28V	GPIO_3_TOUCH_SCL

#define IRQ_TOUCH_INT		gpio_to_irq(_3_GPIO_TOUCH_INT)
#endif


#endif
