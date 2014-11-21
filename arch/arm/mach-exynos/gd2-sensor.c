#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/sensors_axes_s16.h>
#if defined(CONFIG_SENSORS_AK8963C)
#include <linux/sensor/ak8963.h>
#endif
#if defined(CONFIG_SENSORS_LSM330DLC)
#include <linux/sensor/lsm330dlc_accel.h>
#include <linux/sensor/lsm330dlc_gyro.h>
#endif
#if defined(CONFIG_SENSORS_K330)
#include <linux/sensor/k330_accel.h>
#include <linux/sensor/k330_gyro.h>
#endif
#if defined(CONFIG_SENSORS_CM36651)
#include <linux/sensor/cm36651.h>
#endif
#if defined(CONFIG_SENSORS_CM36653)
#include <linux/sensor/cm36653.h>
#endif
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/gpio-midas.h>
#include "midas.h"


#if defined(CONFIG_SENSORS_LSM330DLC) ||\
	defined(CONFIG_SENSORS_K330)
static u8 stm_get_position(void)
{
	u8 position;

	/* Add model config and position here. */
	position = TYPE2_BOTTOM_LOWER_LEFT;

	return position;
}

#if defined(CONFIG_SENSORS_K330)
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
#endif

#if defined(CONFIG_SENSORS_LSM330DLC)
static struct accel_platform_data accel_pdata = {
	.accel_get_position = stm_get_position,
	.axis_adjust = true,
};

static struct gyro_platform_data gyro_pdata = {
	.gyro_get_position = stm_get_position,
	.axis_adjust = true,
};
#endif
#endif

#if defined(CONFIG_SENSORS_AK8963C)
static u8 ak8963_get_position(void)
{
	u8 position;

	/* Add model config and position here. */
	position = TYPE3_BOTTOM_LOWER_RIGHT;

	return position;
}

static struct akm8963_platform_data akm8963_pdata = {
	.gpio_data_ready_int = GPIO_MSENSOR_INT,
	.gpio_RST = GPIO_MSENSE_RST_N,
	.mag_get_position = ak8963_get_position,
	.select_func = select_func_s16,
};
#endif


#if defined(CONFIG_SENSORS_LSM330DLC) || \
	defined(CONFIG_SENSORS_K330)
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
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);

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
#if 0
	s5p_register_gpio_interrupt(GPIO_GYRO_INT);
	s3c_gpio_cfgpin(GPIO_GYRO_INT, S3C_GPIO_SFN(0xF));
#else
	s3c_gpio_cfgpin(GPIO_GYRO_INT, S3C_GPIO_INPUT);
#endif
	gpio_set_value(GPIO_GYRO_INT, 2);
	s3c_gpio_setpull(GPIO_GYRO_INT, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_INT, S5P_GPIO_DRVSTR_LV1);
#if 0
	i2c_devs1[1].irq = gpio_to_irq(GPIO_GYRO_INT); /* interrupt */
#endif

	/* Gyro sensor data enable pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_DE, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_GYRO_DE, 0);
	s3c_gpio_setpull(GPIO_GYRO_DE, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_DE, S5P_GPIO_DRVSTR_LV1);

	return ret;
}
#endif

#if defined(CONFIG_SENSORS_AK8963C)
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
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_MSENSOR_INT, S5P_GPIO_DRVSTR_LV1);

	ret = gpio_request(GPIO_MSENSE_RST_N, "gpio_akm_rst");
	if (ret) {
		pr_err("%s, Failed to request gpio akm_rst.(%d)\n",
			__func__, ret);
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_MSENSE_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MSENSE_RST_N, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(GPIO_MSENSE_RST_N, S5P_GPIO_DRVSTR_LV1);
	gpio_direction_output(GPIO_MSENSE_RST_N, 1);

	return ret;
}
#endif

#if defined(CONFIG_SENSORS_CM36651) || defined(CONFIG_SENSORS_CM36653)
static int proximity_leda_on(bool onoff)
{
	gpio_set_value(GPIO_PS_ALS_EN, onoff);
	pr_info("%s, onoff = %d\n", __func__, onoff);
	return 0;
}

static int optical_gpio_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = gpio_request(GPIO_PS_ALS_EN, "optical_power_supply_on");
	if (ret) {
		pr_err("%s, Failed to request gpio optical power supply(%d)\n",
			__func__, ret);
		return ret;
	}

	/* configuring for gp2a gpio for LEDA power */
	s3c_gpio_cfgpin(GPIO_PS_ALS_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ALS_EN, 0);
	s3c_gpio_setpull(GPIO_PS_ALS_EN, S3C_GPIO_PULL_DOWN);

	ret = gpio_request(GPIO_PS_ALS_INT, "CM36653_PS_INT");
	if (ret) {
		pr_err("%s, Failed to request gpio CM36653_PS_INT(%d)\n",
			__func__, ret);
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_PS_ALS_INT, S3C_GPIO_INPUT);
#ifdef CONFIG_MACH_GD2
	if (system_rev <= 3)/* No external Pull Up */
		s3c_gpio_setpull(GPIO_PS_ALS_INT, S3C_GPIO_PULL_UP);
	else
#endif
	s3c_gpio_setpull(GPIO_PS_ALS_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_PS_ALS_INT, S5P_GPIO_DRVSTR_LV1);
	gpio_free(GPIO_PS_ALS_INT);

	return ret;
}
#endif

#if defined(CONFIG_SENSORS_CM36651)
/* Depends window, threshold is needed to be set */
static u8 cm36651_get_threshold(void)
{
	u8 threshold;

	/* Add model config and threshold here. */
	if (system_rev >= 4)
		threshold = 13;
	else
		threshold = 35;

	return threshold;
}

static struct cm36651_platform_data cm36651_pdata = {
	.cm36651_led_on = proximity_leda_on,
	.cm36651_get_threshold = cm36651_get_threshold,
	.irq = GPIO_PS_ALS_INT,
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("cm36651", (0x30 >> 1)),
		.platform_data = &cm36651_pdata,
	},
};
#endif

#if defined(CONFIG_SENSORS_CM36653)
/* Depends window, threshold is needed to be set */
static u16 cm36653_get_threshold(void)
{
	u16 threshold = 0x140a;

	return threshold;
}

static u16 cm36653_get_cal_threshold(void)
{
	u16 threshold = 0x0f09;

	return threshold;
}

static struct cm36653_platform_data cm36653_pdata = {
	.cm36653_led_on = proximity_leda_on,
	.cm36653_get_threshold = cm36653_get_threshold,
	.cm36653_get_cal_threshold = cm36653_get_cal_threshold,
	.irq = GPIO_PS_ALS_INT,
};
#endif
static struct i2c_board_info i2c_devs0_01_bd[] __initdata = {
#if defined(CONFIG_SENSORS_K330)
	/* The slave address depends on
	  * whether SDO pins are connected to voltage supply or ground */
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
#endif
#if defined(CONFIG_SENSORS_AK8963C)
	{
		I2C_BOARD_INFO("ak8963", 0x0C),
		.platform_data = &akm8963_pdata,
		.irq = IRQ_EINT(17),
	},
#endif
};

static struct i2c_board_info i2c_devs1_03_bd[] __initdata = {
#if defined(CONFIG_SENSORS_K330)
	/* The slave address depends on
	  * whether SDO pins are connected to voltage supply or ground */
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
#endif
#if defined(CONFIG_SENSORS_AK8963C)
	{
		I2C_BOARD_INFO("ak8963", 0x0C),
		.platform_data = &akm8963_pdata,
		.irq = IRQ_EINT(17),
	},
#endif
#if defined(CONFIG_SENSORS_CM36651)
	{
		I2C_BOARD_INFO("cm36651", (0x30 >> 1)),
		.platform_data = &cm36651_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_CM36653)
	{
		I2C_BOARD_INFO("cm36653", 0x60),
		.platform_data = &cm36653_pdata,
	},
#endif
};

static struct i2c_board_info i2c_devs1_04_bd[] __initdata = {
#if defined(CONFIG_SENSORS_K330)
	/* The slave address depends on
	  * whether SDO pins are connected to voltage supply or ground */
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
#endif
#if defined(CONFIG_SENSORS_AK8963C)
	{
		I2C_BOARD_INFO("ak8963", 0x0C),
		.platform_data = &akm8963_pdata,
		.irq = IRQ_EINT(17),
	},
#endif
};

static struct i2c_board_info i2c_devs0_04_bd[] __initdata = {
	#if defined(CONFIG_SENSORS_CM36653)
	{
		I2C_BOARD_INFO("cm36653", 0x60),
		.platform_data = &cm36653_pdata,
	},
	#endif
};

static int __init midas_sensor_init(void)
{
	int ret = 0;

	/* Gyro & Accelerometer Sensor GPIO init */
#if defined(CONFIG_SENSORS_LSM330DLC) || defined(CONFIG_SENSORS_K330)
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
#endif

	/* Magnetic Sensor GPIO init */
#ifdef CONFIG_SENSORS_AK8963C
	ret = ak8963c_gpio_init();
	if (ret < 0) {
		pr_err("%s, ak8963c_gpio_init fail(err=%d)\n",
						 __func__, ret);
		return ret;
	}
#endif

	/* Optical Sensor */
#if defined(CONFIG_SENSORS_CM36651) || defined(CONFIG_SENSORS_CM36653)
	ret = optical_gpio_init();
	if (ret) {
		pr_err("%s, optical_gpio_init(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

	/* Add i2c Devices */
	if (system_rev >= 8) {
		ret = i2c_add_devices(0, i2c_devs0_04_bd, ARRAY_SIZE(i2c_devs0_04_bd));
		if (ret < 0) {
			pr_err("%s, i2c0 adding i2c fail(err=%d)\n", __func__, ret);
			return ret;
		}
		ret = i2c_add_devices(1, i2c_devs1_04_bd, ARRAY_SIZE(i2c_devs1_04_bd));
		if (ret < 0) {
			pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
			return ret;
		}
	} else if (system_rev >= 4) { /* I2C channels have been changed since Rev0.3 */
		ret = i2c_add_devices(1, i2c_devs1_03_bd, ARRAY_SIZE(i2c_devs1_03_bd));
		if (ret < 0) {
			pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
			return ret;
		}
	} else {
		ret = i2c_add_devices(0, i2c_devs0_01_bd, ARRAY_SIZE(i2c_devs0_01_bd));
		if (ret < 0) {
			pr_err("%s, i2c0 adding i2c fail(err=%d)\n", __func__, ret);
			return ret;
		}

		ret = i2c_add_devices(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
		if (ret < 0) {
			pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
			return ret;
		}
	}

	return ret;
}
module_init(midas_sensor_init);
