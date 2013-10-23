#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/devs.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/gpio-rev00-kona.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include "midas.h"
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/yas.h>
#include <linux/sensor/gp2a.h>
#include <mach/kona-sensor.h>

#if defined(CONFIG_SENSORS_BMA254) || defined(CONFIG_SENSORS_K3DH)
static int accel_gpio_init(void)
{
	int ret = gpio_request(GPIO_ACC_INT, "accelerometer_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio accelerometer_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_ACC_INT, 2);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_ACC_INT, S5P_GPIO_DRVSTR_LV1);

	return ret;
}

static int acceleromter_get_position(void)
{
	int position = 0;

#if defined(CONFIG_TARGET_LOCALE_USA)
	if (system_rev >= 3)
		position = 4;
	else if (system_rev >= 1)
		position = 3;
	else
		position = 4;
#elif defined(CONFIG_MACH_KONA_EUR_LTE)
	if (system_rev >= 3)
		position = 4;
	else if (system_rev >= 1)
		position = 3;
	else
		position = 4;
#elif defined(CONFIG_MACH_KONA)
	if (system_rev >= 1)
		position = 4;
	else
		position = 4;
#else
	position = 4;
#endif
	return position;
}

static struct accel_platform_data accel_pdata = {
	.accel_get_position = acceleromter_get_position,
	.axis_adjust = true,
};

#if defined(CONFIG_SENSORS_BMA254)
static struct i2c_board_info i2c_devs1_1[] __initdata = {
	{
		I2C_BOARD_INFO("bma254", 0x18),
		.platform_data = &accel_pdata,
		.irq = IRQ_EINT(0),
	},
};
#endif
#if defined(CONFIG_SENSORS_K3DH)
static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("k3dh", 0x19),
		.platform_data = &accel_pdata,
		.irq = IRQ_EINT(0),
	},
};
#endif
#endif

#ifdef CONFIG_SENSORS_YAS532
static struct i2c_gpio_platform_data gpio_i2c_data10 = {
		.sda_pin = GPIO_MSENSOR_SDA_18V,
		.scl_pin = GPIO_MSENSOR_SCL_18V,
};

struct platform_device s3c_device_i2c10 = {
		.name = "i2c-gpio",
		.id = 10,
		.dev.platform_data = &gpio_i2c_data10,
};

static struct mag_platform_data magnetic_pdata = {
		.offset_enable = 0,
		.chg_status = CABLE_TYPE_NONE,
		.ta_offset.v = {0, 0, 0},
		.usb_offset.v = {0, 0, 0},
		.full_offset.v = {0, 0, 0},
};

static struct i2c_board_info i2c_devs10_emul[] __initdata = {
	{
		I2C_BOARD_INFO("yas532", 0x2e),
		.platform_data = &magnetic_pdata,
	},
};
#endif

#ifdef CONFIG_SENSORS_GP2A
static int proximity_leda_on(bool onoff)
{
	pr_info("%s, onoff = %d\n", __func__, onoff);

	gpio_set_value(GPIO_PS_ALS_EN, onoff);

	return 0;
}

static int optical_gpio_init(void)
{
	int ret = gpio_request(GPIO_PS_ALS_EN, "optical_power_supply_on");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio optical power supply(%d)\n",
			__func__, ret);
		return ret;
	}

	/* configuring for gp2a gpio for LEDA power */
	s3c_gpio_cfgpin(GPIO_PS_ALS_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ALS_EN, 0);
	s3c_gpio_setpull(GPIO_PS_ALS_EN, S3C_GPIO_PULL_NONE);
	return ret;
}

static unsigned long gp2a_get_threshold(u8 *thesh_diff)
{
	u8 threshold = 0x09;

	if (thesh_diff)
		*thesh_diff = 1;

	if (thesh_diff)
		pr_info("%s, threshold low = 0x%x, high = 0x%x\n",
			__func__, threshold, (threshold + *thesh_diff));
	else
		pr_info("%s, threshold = 0x%x\n", __func__, threshold);
	return threshold;
}

static struct gp2a_platform_data gp2a_pdata = {
	.gp2a_led_on	= proximity_leda_on,
	.p_out = GPIO_PS_ALS_INT,
	.gp2a_get_threshold = gp2a_get_threshold,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev = {
		.platform_data = &gp2a_pdata,
	},
};
#endif

#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_AL3201)
static struct i2c_gpio_platform_data gpio_i2c_data12 = {
		.sda_pin = GPIO_PS_ALS_SDA_28V,
		.scl_pin = GPIO_PS_ALS_SCL_28V,
};

struct platform_device s3c_device_i2c12 = {
		.name = "i2c-gpio",
		.id = 12,
		.dev.platform_data = &gpio_i2c_data12,
};

static struct i2c_board_info i2c_devs12_emul[] __initdata = {
#if defined(CONFIG_SENSORS_AL3201)
	{I2C_BOARD_INFO("AL3201", 0x1c),},
#endif
#if defined(CONFIG_SENSORS_GP2A)
	{I2C_BOARD_INFO("gp2a", 0x39),},
#endif
};
#endif

static struct platform_device *kona_sensor_devices[] __initdata = {
#if defined(CONFIG_SENSORS_BMA254) || defined(CONFIG_SENSORS_K3DH)
	&s3c_device_i2c1,
#endif
#ifdef CONFIG_SENSORS_YAS532
	&s3c_device_i2c10,
#endif
#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_AL3201)
	&s3c_device_i2c12,
#endif
};

int kona_sensor_init(void)
{
	int ret = 0;

	/* accelerometer sensor */
	pr_info("%s, is called\n", __func__);

#if defined(CONFIG_SENSORS_BMA254) || defined(CONFIG_SENSORS_K3DH)
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif
#ifdef CONFIG_SENSORS_YAS532
	/* magnetic sensor */
	i2c_register_board_info(10, i2c_devs10_emul,
				ARRAY_SIZE(i2c_devs10_emul));
#endif
#ifdef CONFIG_SENSORS_GP2A
	/* optical sensor */
	ret = optical_gpio_init();
	if (ret < 0)
		pr_err("%s, optical_gpio_init fail(err=%d)\n", __func__, ret);

	i2c_register_board_info(12, i2c_devs12_emul,
				ARRAY_SIZE(i2c_devs12_emul));

	ret = platform_device_register(&opt_gp2a);
	if (ret < 0) {
		pr_err("%s, failed to register opt_gp2a(err=%d)\n",
			__func__, ret);
		return ret;
	}
#elif defined(CONFIG_SENSORS_AL3201)
	i2c_register_board_info(12, i2c_devs12_emul,
				ARRAY_SIZE(i2c_devs12_emul));
#endif
	platform_add_devices(kona_sensor_devices,
				ARRAY_SIZE(kona_sensor_devices));

	return ret;
}

