/********************* Fixed Code Area !***************************/
#include <linux/secgpio_dvs.h>
#include <linux/platform_device.h>
/****************************************************************/

/********************* Fixed Code Area !***************************/
#define GET_RESULT_GPIO(a, b, c)	\
	(((a)<<10 & 0xFC00) |((b)<<4 & 0x03F0) | ((c) & 0x000F))

#define GET_GPIO_IO(value)	\
	(unsigned char)((0xFC00 & (value)) >> 10)
#define GET_GPIO_PUPD(value)	\
	(unsigned char)((0x03F0 & (value)) >> 4)
#define GET_GPIO_LH(value)	\
	(unsigned char)(0x000F & (value))
/****************************************************************/

/****************************************************************/
/* Define value in accordance with
	the specification of each BB vendor. */
#define AP_GPIO_COUNT NEED_DEFINE
/****************************************************************/

/****************************************************************/
/* Pre-defined variables. (DO NOT CHANGE THIS!!) */
static uint16_t checkgpiomap_result[GDVS_PHONE_STATUS_MAX][AP_GPIO_COUNT];
static struct gpiomap_result_t gpiomap_result = {
	.init = checkgpiomap_result[PHONE_INIT],
	.sleep = checkgpiomap_result[PHONE_SLEEP]
};

#ifdef SECGPIO_SLEEP_DEBUGGING
static struct sleepdebug_gpiotable sleepdebug_table;
#endif
/****************************************************************/


/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void check_gpio_status(unsigned char phonestate)
{
	return;
}
/****************************************************************/

#ifdef SECGPIO_SLEEP_DEBUGGING
/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
void setgpio_for_sleepdebug(int gpionum, uint16_t  io_pupd_lh)
{

}
/****************************************************************/

/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void undo_sleepgpio(void)
{
	int i;
	int gpio_num;

	pr_info("[GPIO_DVS][%s] ++\n", __func__);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;
		/*
		 * << Caution >>
		 * If it's necessary,
		 * change the following function to another appropriate one
		 * or delete it
		 */
		setgpio_for_sleepdebug(gpio_num, gpiomap_result.sleep[gpio_num]);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
/****************************************************************/
#endif

/********************* Fixed Code Area !***************************/
#ifdef SECGPIO_SLEEP_DEBUGGING
static void set_sleepgpio(void)
{
	int i;
	int gpio_num;
	uint16_t set_data;

	pr_info("[GPIO_DVS][%s] ++, cnt=%d\n",
		__func__, sleepdebug_table.gpio_count);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		pr_info("[GPIO_DVS][%d] gpio_num(%d), io(%d), pupd(%d), lh(%d)\n",
			i, sleepdebug_table.gpioinfo[i].gpio_num,
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;

		// to prevent a human error caused by "don't care" value
		if( sleepdebug_table.gpioinfo[i].io == 1)		/* IN */
			sleepdebug_table.gpioinfo[i].lh =
				GET_GPIO_LH(gpiomap_result.sleep[gpio_num]);
		else if( sleepdebug_table.gpioinfo[i].io == 2)		/* OUT */
			sleepdebug_table.gpioinfo[i].pupd =
				GET_GPIO_PUPD(gpiomap_result.sleep[gpio_num]);

		set_data = GET_RESULT_GPIO(
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		setgpio_for_sleepdebug(gpio_num, set_data);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
#endif

static struct gpio_dvs_t gpio_dvs = {
	.result = &gpiomap_result,
	.count = AP_GPIO_COUNT,
	.check_init = false,
	.check_sleep = false,
	.check_gpio_status = check_gpio_status,
#ifdef SECGPIO_SLEEP_DEBUGGING
	.sdebugtable = &sleepdebug_table,
	.set_sleepgpio = set_sleepgpio,
	.undo_sleepgpio = undo_sleepgpio,
#endif
};

static struct platform_device secgpio_dvs_device = {
	.name	= "secgpio_dvs",
	.id		= -1,
	.dev.platform_data = &gpio_dvs,
};

static struct platform_device *secgpio_dvs_devices[] __initdata = {
	&secgpio_dvs_device,
};

static int __init secgpio_dvs_device_init(void)
{
	return platform_add_devices(
		secgpio_dvs_devices, ARRAY_SIZE(secgpio_dvs_devices));
}
arch_initcall(secgpio_dvs_device_init);
/****************************************************************/


