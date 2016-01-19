#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/sensors_axes_s16.h>
#ifdef CONFIG_SENSORS_AK8963C
#include <linux/sensor/ak8963.h>
#endif
#ifdef CONFIG_SENSORS_GP2A
#include <linux/sensor/gp2a.h>
#endif
#ifdef CONFIG_SENSORS_K330
#include <linux/sensor/k330_accel.h>
#include <linux/sensor/k330_gyro.h>
#endif
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include "midas.h"


#if defined(CONFIG_SENSORS_K330)
static u8 stm_get_position(void);
static bool gyro_en;

static struct accel_platform_data accel_pdata = {
	.accel_get_position = stm_get_position,
	.select_func = select_func_s16,
	.gyro_en = &gyro_en,
};

static struct gyro_platform_data gyro_pdata = {
	.gyro_get_position = stm_get_position,
	.select_func = select_func_s16,
	.gyro_en = &gyro_en,
};

static struct i2c_board_info i2c_devs18_emul[] __initdata = {
	{
		I2C_BOARD_INFO("k330_accel", (0x3C >> 1)),
		.platform_data = &accel_pdata,
		.irq = IRQ_EINT(0),
	},
	{
		I2C_BOARD_INFO("k330_gyro", (0xD4 >> 1)),
		.platform_data = &gyro_pdata,
		.irq = -1,	/* polling */
	},
};
#endif

#if defined(CONFIG_SENSORS_K330)
static u8 stm_get_position(void)
{
	u8 position;

#if defined(CONFIG_MACH_GC2PD)
	if (system_rev < 1)
		position = TYPE2_BOTTOM_LOWER_RIGHT;
	else
		position = TYPE2_BOTTOM_LOWER_LEFT;
#else
	position = TYPE2_TOP_UPPER_RIGHT;
#endif
	return position;
}
#endif


#if defined(CONFIG_SENSORS_K330)
static int accel_gpio_init(void)
{
	int ret = gpio_request(GPIO_ACC_INT, "accelerometer_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio accelerometer_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	/* Accelerometer sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_ACC_INT, 2);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(GPIO_ACC_INT, S5P_GPIO_DRVSTR_LV1);

	return ret;
}

static int gyro_gpio_init(void)
{
	int ret = gpio_request(GPIO_GYRO_INT, "stm_gyro_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio stm_gyro_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	ret = gpio_request(GPIO_GYRO_DE, "stm_gyro_data_enable");

	if (ret) {
		pr_err("%s, Failed to request gpio stm_gyro_data_enable(%d)\n",
			__func__, ret);
		return ret;
	}

	/* Gyro sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_GYRO_INT, 2);
	s3c_gpio_setpull(GPIO_GYRO_INT, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_INT, S5P_GPIO_DRVSTR_LV1);

	/* Gyro sensor data enable pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_DE, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_GYRO_DE, 0);
	s3c_gpio_setpull(GPIO_GYRO_DE, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_DE, S5P_GPIO_DRVSTR_LV1);

	return ret;
}
#endif

#if defined(CONFIG_SENSORS_GP2A)
static int proximity_leda_on(bool onoff)
{
	gpio_set_value(GPIO_PS_ALS_EN_3, onoff);

	pr_info("%s, onoff = %d\n", __func__, onoff);
	return 0;
}

static int light_vdd_on(bool onoff)
{
	gpio_set_value(GPIO_PS_ALS_EN_285, onoff);

	pr_info("%s, onoff = %d\n", __func__, onoff);
	return 0;
}

static int optical_gpio_init(void)
{
	int ret;

	ret = gpio_request(GPIO_PS_ALS_EN_285, "ps_power_supply_on");
	if (ret) {
		pr_err("%s, Failed to request gpio ps power supply(%d)\n",
			__func__, ret);
		return ret;
	}

	ret = gpio_request(GPIO_PS_ALS_EN_3, "als_power_supply_on");
	if (ret) {
		pr_err("%s, Failed to request gpio als power supply(%d)\n",
			__func__, ret);
		return ret;
	}

	/* configuring for gp2a gpio for LEDA power */
	s3c_gpio_cfgpin(GPIO_PS_ALS_EN_285, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ALS_EN_285, 0);
	s3c_gpio_setpull(GPIO_PS_ALS_EN_285, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_PS_ALS_EN_3, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ALS_EN_3, 0);
	s3c_gpio_setpull(GPIO_PS_ALS_EN_3, S3C_GPIO_PULL_NONE);
	return ret;
}
#endif

#if defined(CONFIG_SENSORS_GP2A)
static unsigned long gp2a_get_threshold(u8 *thesh_diff)
{
	u8 threshold = 0x08;

	if (thesh_diff)
		*thesh_diff = 2;

	if (thesh_diff)
		pr_info("%s, threshold low = 0x%x, high = 0x%x\n",
			__func__, threshold, (threshold + *thesh_diff));
	else
		pr_info("%s, threshold = 0x%x\n", __func__, threshold);

	return threshold;
}

static struct gp2a_platform_data gp2a_pdata = {
	.gp2a_led_on	= proximity_leda_on,
	.gp2a_vdd_on = light_vdd_on,
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

static struct gp2a_platform_data gp2a_light_pdata = {
	.gp2a_vdd_on = light_vdd_on,
};

static struct platform_device light_gp2a = {
	.name = "light_sensor",
	.id = -1,
	.dev = {
		.platform_data = &gp2a_light_pdata,
	},
};
#endif

static struct i2c_board_info i2c_devs9_emul[] __initdata = {
#if defined(CONFIG_SENSORS_GP2A)
	{
		I2C_BOARD_INFO("gp2a", (0x72 >> 1)),
	},
#endif
};

#ifdef CONFIG_SENSORS_AK8963C
static u8 ak8963_get_position(void)
{
	u8 position;

#if defined(CONFIG_MACH_GC1)
	position = TYPE3_TOP_LOWER_LEFT;
#elif defined(CONFIG_MACH_GC2PD)
	position = TYPE3_TOP_UPPER_RIGHT;
#else
	position = TYPE3_TOP_LOWER_RIGHT;
#endif
	return position;
}

static struct akm8963_platform_data akm8963_pdata = {
	.gpio_data_ready_int = GPIO_MSENSOR_INT,
	.gpio_RST = GPIO_MSENSE_RST_N,
	.mag_get_position = ak8963_get_position,
	.select_func = select_func_s16,
};

static struct i2c_board_info i2c_devs10_emul[] __initdata = {
	{
		I2C_BOARD_INFO("ak8963", 0x0C),
		.platform_data = &akm8963_pdata,
	},
};

static int ak8963c_gpio_init(void)
{
	int ret;

	pr_info("%s\n", __func__);

	ret = gpio_request(GPIO_MSENSOR_INT, "gpio_akm_int");
	if (ret) {
		pr_err("%s, Failed to request gpio akm_int.(%d)\n",
			__func__, ret);
		return ret;
	}
	s5p_register_gpio_interrupt(GPIO_MSENSOR_INT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_SFN(0xF));
	i2c_devs10_emul[0].irq = gpio_to_irq(GPIO_MSENSOR_INT);

	ret = gpio_request(GPIO_MSENSE_RST_N, "gpio_akm_rst");
	if (ret) {
		pr_err("%s, Failed to request gpio akm_rst.(%d)\n",
			__func__, ret);
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_MSENSE_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MSENSE_RST_N, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_MSENSE_RST_N, 1);

	return ret;
}
#endif

static int __init midas_sensor_init(void)
{
	int ret;
	pr_info("%s\n", __func__);

	/* Gyro & Accelerometer Sensor */
#if defined(CONFIG_SENSORS_K330)
	ret = accel_gpio_init();
	if (ret < 0) {
		pr_err("%s, accel_gpio_init fail(err=%d)\n", __func__, ret);
		return ret;
	}
	ret = gyro_gpio_init();
	if (ret < 0) {
		pr_err("%s, gyro_gpio_init(err=%d)\n", __func__, ret);
		return ret;
	}
	ret = i2c_add_devices(18, i2c_devs18_emul, ARRAY_SIZE(i2c_devs18_emul));
	if (ret < 0) {
		pr_err("%s, i2c18 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

	/* Optical Sensor */
#if defined(CONFIG_SENSORS_GP2A)
	ret = optical_gpio_init();
	if (ret) {
		pr_err("%s, optical_gpio_init(err=%d)\n", __func__, ret);
		return ret;
	}
	ret = i2c_add_devices(9, i2c_devs9_emul, ARRAY_SIZE(i2c_devs9_emul));
	if (ret < 0) {
		pr_err("%s, i2c9 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}

	ret = platform_device_register(&opt_gp2a);
	if (ret < 0) {
		pr_err("%s, failed to register opt_gp2a(err=%d)\n",
			__func__, ret);
		return ret;
	}

	ret = platform_device_register(&light_gp2a);
	if (ret < 0) {
		pr_err("%s, failed to register light_gp2a(err=%d)\n",
			__func__, ret);
		return ret;
	}
#endif

	/* Magnetic Sensor */
#ifdef CONFIG_SENSORS_AK8963C
		ret = ak8963c_gpio_init();
		if (ret < 0) {
			pr_err("%s, ak8963c_gpio_init fail(err=%d)\n",
							 __func__, ret);
			return ret;
		}
		ret = i2c_add_devices(10, i2c_devs10_emul,
						ARRAY_SIZE(i2c_devs10_emul));
		if (ret < 0) {
			pr_err("%s, i2c10 adding i2c fail(err=%d)\n",
							__func__, ret);
			return ret;
		}
#endif

	return ret;
}
module_init(midas_sensor_init);
