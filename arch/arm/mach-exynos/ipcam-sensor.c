#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/sensor/sensors_core.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include "midas.h"
#include <linux/sensor/shtc1.h>
#include <linux/sensor/bh1730.h>


static struct shtc1_platform_data shtc1_pdata = {
	.blocking_io = false,
	.high_precision = false,
};


static struct i2c_board_info i2c_devs0[] __initdata = {
#if defined(CONFIG_SENSORS_SEC_SHT21)
	{
		I2C_BOARD_INFO("sht21", 0x40),
	},
#endif
#if defined(CONFIG_SENSORS_SEC_SHTC1)
	{
		I2C_BOARD_INFO("shtc1", 0x70),
		.platform_data = &shtc1_pdata,
	},
#endif
};

#if defined(CONFIG_SENSORS_BH1730)
static int light_power_on(bool onoff)
{
	struct regulator *light_vcc;

	pr_info("%s, is called\n", __func__);

	light_vcc = regulator_get(NULL ,"light_3.0v");

	if (IS_ERR(light_vcc)) {
		pr_err("%s: cannot get light_vcc\n", __func__);
		goto done;
	}

	if (onoff) {
		regulator_enable(light_vcc);
	} else {
		regulator_disable(light_vcc);
	}

	regulator_put(light_vcc);
done:
	pr_info("%s, onoff = %d\n", __func__, onoff);
	return 0;
}
#endif

#if defined(CONFIG_SENSORS_BH1730)
static struct bh1730fvc_platform_data bh1730fvc_pdata = {
	.light_power_on = light_power_on,
};
#endif

static struct i2c_board_info i2c_devs1[] __initdata = {
#if defined(CONFIG_SENSORS_BH1730)
	{
		I2C_BOARD_INFO("bh1730fvc", 0x29),
		.platform_data = &bh1730fvc_pdata,
	},
#endif
};

static int __init midas_sensor_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

#if defined(CONFIG_SENSORS_SEC_SHTC1) || defined(CONFIG_SENSORS_SEC_SHT21)
	ret = i2c_add_devices(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	if (ret < 0) {
		pr_err("%s, i2c0 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

#if defined(CONFIG_SENSORS_BH1730)
	ret = i2c_add_devices(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	if (ret < 0) {
		pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

	return ret;
}
module_init(midas_sensor_init);
