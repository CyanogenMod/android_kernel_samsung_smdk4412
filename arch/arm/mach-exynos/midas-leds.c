#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/leds-lp5521.h>

#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include "midas.h"

#ifdef CONFIG_LEDS_LP5521
static struct lp5521_led_config lp5521_led_config[] = {
	{
		.name = "red",
		.chan_nr = 0,
		.led_current = 50,
		.max_current = 130,
	}, {
		.name = "green",
		.chan_nr = 1,
		.led_current = 30,
		.max_current = 130,
	}, {
		.name = "blue",
		.chan_nr = 2,
		.led_current = 30,
		.max_current = 130,
	},
};
#define LP5521_CONFIGS (LP5521_PWM_HF | LP5521_PWRSAVE_EN | \
LP5521_CP_MODE_AUTO | LP5521_R_TO_BATT | \
LP5521_CLK_INT)

static struct lp5521_platform_data lp5521_pdata = {
	.led_config = lp5521_led_config,
	.num_channels = ARRAY_SIZE(lp5521_led_config),
	.clock_mode = LP5521_CLOCK_INT,
	.update_config = LP5521_CONFIGS,
};
static struct i2c_board_info i2c_devs21_emul[] __initdata = {
	{
		I2C_BOARD_INFO("lp5521", 0x32),
		.platform_data = &lp5521_pdata,
	},
};

int __init plat_leds_init(void)
{
	i2c_add_devices(21, i2c_devs21_emul, ARRAY_SIZE(i2c_devs21_emul));
	return 0;
};
module_init(plat_leds_init);
#endif
