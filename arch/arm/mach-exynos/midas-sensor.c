#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/sensors_axes_s16.h>
#ifdef CONFIG_SENSORS_AK8975C
#include <linux/sensor/ak8975.h>
#endif
#ifdef CONFIG_SENSORS_AK8963C
#include <linux/sensor/ak8963.h>
#endif
#ifdef CONFIG_INPUT_MPU6050
#include <linux/mpu6050_input.h>
#endif
#ifdef CONFIG_SENSORS_AK8963
#include <linux/akm8963.h>
#endif
#ifdef CONFIG_SENSORS_YAS532
#include <linux/sensor/yas.h>
#endif
#ifdef CONFIG_SENSORS_K3DH
#include <linux/sensor/k3dh.h>
#endif
#include <linux/sensor/gp2a.h>
#ifdef CONFIG_SENSORS_LSM330DLC
#include <linux/sensor/lsm330dlc_accel.h>
#include <linux/sensor/lsm330dlc_gyro.h>
#endif
#ifdef CONFIG_SENSORS_K330
#include <linux/sensor/k330_accel.h>
#include <linux/sensor/k330_gyro.h>
#endif
#ifdef CONFIG_SENSORS_TMD27723
#include <linux/sensor/taos.h>
#endif
#include <linux/sensor/lps331ap.h>
#include <linux/sensor/cm36651.h>
#include <linux/sensor/cm36653.h>
#include <linux/sensor/cm3663.h>
#include <linux/sensor/bh1721.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include "midas.h"

#ifdef CONFIG_SENSORS_SSP_STM
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <plat/s3c64xx-spi.h>
#include <mach/spi-clocks.h>
#include <plat/devs.h>
#endif

#ifdef CONFIG_SENSORS_SSP
#include <linux/ssp_platformdata.h>
#endif

#ifdef CONFIG_SENSORS_ASP01
#include <linux/sensor/asp01.h>
#endif

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
#endif

#if defined(CONFIG_SENSORS_LSM330DLC) ||\
	defined(CONFIG_SENSORS_K3DH)
static u8 stm_get_position(void);

static struct accel_platform_data accel_pdata = {
	.accel_get_position = stm_get_position,
	.axis_adjust = true,
};
#endif

#ifdef CONFIG_SENSORS_LSM330DLC
static struct gyro_platform_data gyro_pdata = {
	.gyro_get_position = stm_get_position,
	.axis_adjust = true,
};
#endif

#ifdef CONFIG_INPUT_MPU6050
static struct mpu6050_input_platform_data mpu6050_pdata = {
	.orientation = {-1, 0, 0,
				0, 1, 0,
				0, 0, -1},
	.acc_cal_path = "/efs/calibration_data",
	.gyro_cal_path = "/efs/gyro_cal_data",
};
#endif

#ifdef CONFIG_SENSORS_SSP
static int wakeup_mcu(void);
static int check_mcu_busy(void);
static int check_mcu_ready(void);
static int set_mcu_reset(int on);
static int check_ap_rev(void);
#ifdef CONFIG_SENSORS_SSP_STM
static void ssp_get_positions(int *acc, int *mag);
#endif

static struct ssp_platform_data ssp_pdata = {
	.wakeup_mcu = wakeup_mcu,
	.check_mcu_busy = check_mcu_busy,
	.check_mcu_ready = check_mcu_ready,
	.set_mcu_reset = set_mcu_reset,
	.check_ap_rev = check_ap_rev,
#ifdef CONFIG_SENSORS_SSP_STM
	.get_positions = ssp_get_positions,
#endif
};
#endif

static struct i2c_board_info i2c_devs1[] __initdata = {
#if defined(CONFIG_SENSORS_LSM330DLC)
	{
		I2C_BOARD_INFO("lsm330dlc_accel", (0x32 >> 1)),
		.platform_data = &accel_pdata,
	},
	{
		I2C_BOARD_INFO("lsm330dlc_gyro", (0xD6 >> 1)),
		.platform_data = &gyro_pdata,
	},
#elif defined(CONFIG_SENSORS_K330)
	{
		I2C_BOARD_INFO("k330_accel", (0x3A >> 1)),
		.platform_data = &accel_pdata,
	},
	{
		I2C_BOARD_INFO("k330_gyro", (0xD6 >> 1)),
		.platform_data = &gyro_pdata,
	},
#elif defined(CONFIG_SENSORS_K3DH)
	{
		I2C_BOARD_INFO("k3dh", 0x19),
		.platform_data	= &accel_pdata,
	},
#elif defined(CONFIG_SENSORS_SSP_ATMEL)
	{
		I2C_BOARD_INFO("ssp", 0x18),
		.platform_data = &ssp_pdata,
		.irq = GPIO_MCU_AP_INT,
	},
#elif defined(CONFIG_INPUT_MPU6050)
	{
		I2C_BOARD_INFO("mpu6050_input", 0x68),
		.platform_data = &mpu6050_pdata,
	},
#endif
};

#if defined(CONFIG_SENSORS_SSP_STM)
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line		= GPIO_SHUB_SPI_SSN,
		.set_level	= gpio_set_value,
		.fb_delay	= 0x0, // 0x2,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias		= "ssp-spi",
		.max_speed_hz		= 500 * 1000, // 10*1000*1000,
		.bus_num		= 1,
		.chip_select		= 0,
		.mode			= SPI_MODE_1,
		.irq			= GPIO_MCU_AP_INT,
		.controller_data	= &spi1_csi[0],
		.platform_data		= &ssp_pdata,
	}
};
#endif

#ifdef CONFIG_SENSORS_SSP
static int initialize_ssp_gpio(void)
{
	int err;

	err = gpio_request(GPIO_AP_MCU_INT, "AP_MCU_INT_PIN");
	if (err)
		printk(KERN_ERR "failed to request AP_MCU_INT for SSP\n");

	s3c_gpio_cfgpin(GPIO_AP_MCU_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_AP_MCU_INT, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_AP_MCU_INT, 1);

	err = gpio_request(GPIO_MCU_AP_INT_2, "MCU_AP_INT_PIN2");
	if (err)
		printk(KERN_ERR "failed to request AP_MCU_INT for SSP\n");

	s3c_gpio_cfgpin(GPIO_MCU_AP_INT_2, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_MCU_AP_INT_2, S3C_GPIO_PULL_NONE);

	err = gpio_request(GPIO_MCU_NRST, "AP_MCU_RESET");
	if (err)
		printk(KERN_ERR "failed to request AP_MCU_RESET for SSP\n");

	s3c_gpio_cfgpin(GPIO_MCU_NRST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MCU_NRST, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_MCU_NRST, 1);

	return 0;
}

static int wakeup_mcu(void)
{
	gpio_set_value(GPIO_AP_MCU_INT, 0);
	udelay(1);
	gpio_set_value(GPIO_AP_MCU_INT, 1);

	return 0;
}

static int set_mcu_reset(int on)
{
	if (on == 0)
		gpio_set_value(GPIO_MCU_NRST, 0);
	else
		gpio_set_value(GPIO_MCU_NRST, 1);

	return 0;
}

static int check_mcu_busy(void)
{
	return gpio_get_value(GPIO_MCU_AP_INT);
}

static int check_mcu_ready(void)
{
	return gpio_get_value(GPIO_MCU_AP_INT_2);
}

static int check_ap_rev(void)
{
	return system_rev;
}

static void ssp_get_positions(int *acc, int *mag)
{
	if (system_rev < 2)
		*acc = MPU6500_TOP_RIGHT_UPPER;
	else
		*acc = MPU6500_BOTTOM_LEFT_LOWER;

	*mag = 0;

	pr_info("%s, system rev : 0x%x, position acc : %d, mag = %d\n",
		__func__, system_rev ,*acc, *mag);
}
#endif

#if defined(CONFIG_SENSORS_K330)
static u8 stm_get_position(void)
{
	u8 position;

#if defined(CONFIG_MACH_ZEST)
	position = TYPE2_BOTTOM_UPPER_LEFT;
#else
	position = TYPE2_TOP_UPPER_RIGHT;
#endif
	return position;
}
#endif

#if defined(CONFIG_SENSORS_LSM330DLC) || \
	defined(CONFIG_SENSORS_K3DH)
static u8 stm_get_position(void)
{
	int position = 0;

#if defined(CONFIG_MACH_M3) /* C2_SPR, M3 */
#if defined(CONFIG_MACH_M3_USA_TMO)
	position = 4;
#else
	position = 2; /* top/lower-right */
#endif
#elif defined(CONFIG_MACH_M0_CMCC)
	if (system_rev == 2)
		position = 0; /* top/upper-left */
	else
		position = 2; /* top/lower-right */
#elif defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT)
	if (system_rev >= 6)
		position = 6; /* bottom/lower-right */
	else
		position = 3; /* top/lower-left */
#elif defined(CONFIG_MACH_C1_KOR_LGT)
	if (system_rev >= 6)
		position = 2; /* top/lower-right */
	else if (system_rev == 5)
		position = 4; /* bottom/upper-left */
	else
		position = 3; /* top/lower-left */
#elif defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE)
	position = 4; /* bottom/upper-left */
#elif defined(CONFIG_MACH_M0)
	if (system_rev == 3 || system_rev == 0)
		position = 6; /* bottom/lower-right */
	else if (system_rev == 1 || system_rev == 2\
		|| system_rev == 4 || system_rev == 5)
		position = 0; /* top/upper-left */
	else
		position = 2; /* top/lower-right */
#elif defined(CONFIG_MACH_C1)
	if (system_rev == 3 || system_rev == 0)
		position = 7; /* bottom/lower-left */
	else if (system_rev == 2)
		position = 3; /* top/lower-left */
	else
		position = 2; /* top/lower-right */
#elif defined(CONFIG_MACH_GC1)
	if (system_rev >= 6)
		position = 6; /* bottom/lower-right */
	else if (system_rev == 2 || system_rev == 3)
		position = 1; /* top/upper-right */
	else
		position = 0; /* top/upper-left */
#elif defined(CONFIG_MACH_BAFFIN_KOR_SKT) || defined(CONFIG_MACH_BAFFIN_KOR_KT)
	if (system_rev >= 2)
		position = 3; /* top/lower-left */
	else
		position = 2; /* top/lower-right */
#elif defined(CONFIG_MACH_BAFFIN_KOR_LGT)
		position = 3; /* top/lower-left */
#elif defined(CONFIG_MACH_TAB3)
#if defined(CONFIG_TARGET_TAB3_3G10) ||\
	defined(CONFIG_TARGET_TAB3_WIFI10) ||\
	defined(CONFIG_TARGET_TAB3_LTE10) ||\
	defined(CONFIG_TARGET_TAB3_3G8) ||\
	defined(CONFIG_TARGET_TAB3_WIFI8) ||\
	defined(CONFIG_TARGET_TAB3_LTE8)
	position = 5; /* bottom/ upper-right */
#else
	if (system_rev > 3)
		position = 7; /* bottom/ lower-left */
	else
		position = 6; /* bottom/ lower-right */
#endif
#else /* Common */
	position = 2; /* top/lower-right */
#endif
	return position;
}
#endif

#if defined(CONFIG_SENSORS_LSM330DLC) || \
	defined(CONFIG_SENSORS_K3DH) ||\
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
#if defined(CONFIG_TARGET_TAB3_3G8) ||\
	defined(CONFIG_TARGET_TAB3_WIFI8) ||\
	defined(CONFIG_TARGET_TAB3_LTE8)
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_DOWN);
#else
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_ACC_INT, 2);
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_ACC_INT, S5P_GPIO_DRVSTR_LV1);
	i2c_devs1[0].irq = gpio_to_irq(GPIO_ACC_INT);
#endif

	return ret;
}
#endif

#if defined(CONFIG_SENSORS_LSM330DLC) ||\
	defined(CONFIG_SENSORS_K330)
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
#else
	i2c_devs1[1].irq = -1; /* polling */
#endif

	/* Gyro sensor data enable pin initialization */
	s3c_gpio_cfgpin(GPIO_GYRO_DE, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_GYRO_DE, 0);
	s3c_gpio_setpull(GPIO_GYRO_DE, S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(GPIO_GYRO_DE, S5P_GPIO_DRVSTR_LV1);

	return ret;
}
#endif

#ifdef CONFIG_INPUT_MPU6050
static int mpu_gpio_init(void)
{
	int ret = gpio_request(GPIO_GYRO_INT, "mpu_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio mpu_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	/* Accelerometer sensor interrupt pin initialization */
	s5p_register_gpio_interrupt(GPIO_GYRO_INT);
	s3c_gpio_cfgpin(GPIO_GYRO_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_GYRO_INT, 2);
	s3c_gpio_setpull(GPIO_GYRO_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_GYRO_INT, S5P_GPIO_DRVSTR_LV1);
	i2c_devs1[0].irq = gpio_to_irq(GPIO_GYRO_INT);

	return ret;
}
#endif

#ifdef CONFIG_SENSORS_ASP01
static struct asp01_platform_data asp01_pdata = {
	.t_out = GPIO_RF_TOUCH,
};

static struct i2c_board_info i2c_devs6[] __initdata = {
	{
		I2C_BOARD_INFO("asp01", (0x48 >> 1)),
		.platform_data = &asp01_pdata,
	},
};

static int grip_gpio_init(void)
{
	int ret = gpio_request(GPIO_RF_TOUCH, "asp01_grip_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio asp01_grip_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	/* Grip sensor interrupt pin initialization */
	s3c_gpio_cfgpin(GPIO_RF_TOUCH, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_RF_TOUCH, 2);
	s3c_gpio_setpull(GPIO_RF_TOUCH, S3C_GPIO_PULL_UP);
	s5p_gpio_set_drvstr(GPIO_RF_TOUCH, S5P_GPIO_DRVSTR_LV1);

	i2c_devs6[0].irq = gpio_to_irq(GPIO_RF_TOUCH); /* interrupt */
	asp01_pdata.irq = i2c_devs6[0].irq;

	return ret;
}

static void grip_init_code_set(void)
{
#if defined(CONFIG_MACH_TAB3)
#if defined(CONFIG_TARGET_TAB3_3G10) ||\
		defined(CONFIG_TARGET_TAB3_LTE10)
	if (system_rev) {
		asp01_pdata.cr_divsr = 10;
		asp01_pdata.cr_divnd = 12;
		asp01_pdata.cs_divsr = 10;
		asp01_pdata.cs_divnd = 12;
	}
	asp01_pdata.init_code[SET_UNLOCK] = 0x5a;
	asp01_pdata.init_code[SET_RST_ERR] = 0x33;
	asp01_pdata.init_code[SET_PROX_PER] = 0x40;
	asp01_pdata.init_code[SET_PAR_PER] = 0x40;
	asp01_pdata.init_code[SET_TOUCH_PER] = 0x3c;
	asp01_pdata.init_code[SET_HI_CAL_PER] = 0x30;
	asp01_pdata.init_code[SET_BSMFM_SET] = 0x31;
	asp01_pdata.init_code[SET_ERR_MFM_CYC] = 0x33;
	asp01_pdata.init_code[SET_TOUCH_MFM_CYC] = 0x34;
	asp01_pdata.init_code[SET_SYS_FUNC] = 0x10;
	asp01_pdata.init_code[SET_HI_CAL_SPD] = 0x21;
	asp01_pdata.init_code[SET_CAL_SPD] = 0x04;
	asp01_pdata.init_code[SET_BFT_MOT] = 0x40;
	asp01_pdata.init_code[SET_TOU_RF_EXT] = 0x00;
	asp01_pdata.init_code[SET_OFF_TIME] = 0x30;
	asp01_pdata.init_code[SET_SENSE_TIME] = 0x28;
	asp01_pdata.init_code[SET_DUTY_TIME] = 0x50;
	asp01_pdata.init_code[SET_HW_CON1] = 0x78;
	asp01_pdata.init_code[SET_HW_CON2] = 0x27;
	asp01_pdata.init_code[SET_HW_CON3] = 0x20;
	asp01_pdata.init_code[SET_HW_CON4] = 0x27;
	asp01_pdata.init_code[SET_HW_CON5] = 0x83;
	asp01_pdata.init_code[SET_HW_CON6] = 0x3f;
	asp01_pdata.init_code[SET_HW_CON7] = 0x48;
	asp01_pdata.init_code[SET_HW_CON8] = 0x20;
	asp01_pdata.init_code[SET_HW_CON9] = 0x04;
	asp01_pdata.init_code[SET_HW_CON10] = 0x27;
	asp01_pdata.init_code[SET_HW_CON11] = 0x00;
#elif defined(CONFIG_TARGET_TAB3_3G8)
	asp01_pdata.cr_divsr = 10;
	asp01_pdata.cr_divnd = 12;
	asp01_pdata.cs_divsr = 10;
	asp01_pdata.cs_divnd = 12;
	asp01_pdata.init_code[SET_UNLOCK] = 0x5a;
	asp01_pdata.init_code[SET_RST_ERR] = 0x33;
	asp01_pdata.init_code[SET_PROX_PER] = 0x30;
	asp01_pdata.init_code[SET_PAR_PER] = 0x30;
	asp01_pdata.init_code[SET_TOUCH_PER] = 0x3c;
	asp01_pdata.init_code[SET_HI_CAL_PER] = 0x18;
	asp01_pdata.init_code[SET_BSMFM_SET] = 0x30;
	asp01_pdata.init_code[SET_ERR_MFM_CYC] = 0x33;
	asp01_pdata.init_code[SET_TOUCH_MFM_CYC] = 0x25;
	asp01_pdata.init_code[SET_SYS_FUNC] = 0x10;
	asp01_pdata.init_code[SET_HI_CAL_SPD] = 0x18;
	asp01_pdata.init_code[SET_CAL_SPD] = 0x03;
	asp01_pdata.init_code[SET_BFT_MOT] = 0x40;
	asp01_pdata.init_code[SET_TOU_RF_EXT] = 0x00;
	asp01_pdata.init_code[SET_OFF_TIME] = 0x30;
	asp01_pdata.init_code[SET_SENSE_TIME] = 0x48;
	asp01_pdata.init_code[SET_DUTY_TIME] = 0x50;
	asp01_pdata.init_code[SET_HW_CON1] = 0x78;
	asp01_pdata.init_code[SET_HW_CON2] = 0x27;
	asp01_pdata.init_code[SET_HW_CON3] = 0xD0;
	asp01_pdata.init_code[SET_HW_CON4] = 0x27;
	asp01_pdata.init_code[SET_HW_CON5] = 0x83;
	asp01_pdata.init_code[SET_HW_CON6] = 0x3f;
	asp01_pdata.init_code[SET_HW_CON7] = 0x48;
	asp01_pdata.init_code[SET_HW_CON8] = 0x20;
	asp01_pdata.init_code[SET_HW_CON9] = 0x04;
	asp01_pdata.init_code[SET_HW_CON10] = 0x27;
	asp01_pdata.init_code[SET_HW_CON11] = 0x00;
#elif defined(CONFIG_TARGET_TAB3_LTE8)
	asp01_pdata.cr_divsr = 10;
	asp01_pdata.cr_divnd = 12;
	asp01_pdata.cs_divsr = 10;
	asp01_pdata.cs_divnd = 12;
	asp01_pdata.init_code[SET_UNLOCK] = 0x5a;
	asp01_pdata.init_code[SET_RST_ERR] = 0x33;
	asp01_pdata.init_code[SET_PROX_PER] = 0x30;
	asp01_pdata.init_code[SET_PAR_PER] = 0x24;
	asp01_pdata.init_code[SET_TOUCH_PER] = 0x3c;
	asp01_pdata.init_code[SET_HI_CAL_PER] = 0x18;
	asp01_pdata.init_code[SET_BSMFM_SET] = 0x30;
	asp01_pdata.init_code[SET_ERR_MFM_CYC] = 0x33;
	asp01_pdata.init_code[SET_TOUCH_MFM_CYC] = 0x37;
	asp01_pdata.init_code[SET_SYS_FUNC] = 0x10;
	asp01_pdata.init_code[SET_HI_CAL_SPD] = 0x1A;
	asp01_pdata.init_code[SET_CAL_SPD] = 0x03;
	asp01_pdata.init_code[SET_BFT_MOT] = 0x40;
	asp01_pdata.init_code[SET_TOU_RF_EXT] = 0x00;
	asp01_pdata.init_code[SET_OFF_TIME] = 0x30;
	asp01_pdata.init_code[SET_SENSE_TIME] = 0x48;
	asp01_pdata.init_code[SET_DUTY_TIME] = 0x50;
	asp01_pdata.init_code[SET_HW_CON1] = 0x78;
	asp01_pdata.init_code[SET_HW_CON2] = 0x27;
	asp01_pdata.init_code[SET_HW_CON3] = 0xC8;
	asp01_pdata.init_code[SET_HW_CON4] = 0x27;
	asp01_pdata.init_code[SET_HW_CON5] = 0x83;
	asp01_pdata.init_code[SET_HW_CON6] = 0x3f;
	asp01_pdata.init_code[SET_HW_CON7] = 0x48;
	asp01_pdata.init_code[SET_HW_CON8] = 0x20;
	asp01_pdata.init_code[SET_HW_CON9] = 0x04;
	asp01_pdata.init_code[SET_HW_CON10] = 0x27;
	asp01_pdata.init_code[SET_HW_CON11] = 0x00;
#else /* tab3 7 3g*/
	if (system_rev) {
		asp01_pdata.cr_divsr = 10;
		asp01_pdata.cr_divnd = 12;
		asp01_pdata.cs_divsr = 10;
		asp01_pdata.cs_divnd = 12;

		asp01_pdata.init_code[SET_UNLOCK] = 0x5a;
		asp01_pdata.init_code[SET_RST_ERR] = 0x33;
		asp01_pdata.init_code[SET_PROX_PER] = 0x30;
		asp01_pdata.init_code[SET_PAR_PER] = 0x30;
		asp01_pdata.init_code[SET_TOUCH_PER] = 0x3c;
		asp01_pdata.init_code[SET_HI_CAL_PER] = 0x30;
		asp01_pdata.init_code[SET_BSMFM_SET] = 0x31;
		asp01_pdata.init_code[SET_ERR_MFM_CYC] = 0x33;
		asp01_pdata.init_code[SET_TOUCH_MFM_CYC] = 0x34;
		asp01_pdata.init_code[SET_SYS_FUNC] = 0x10;
		asp01_pdata.init_code[SET_HI_CAL_SPD] = 0x21;
		asp01_pdata.init_code[SET_CAL_SPD] = 0x04;
		asp01_pdata.init_code[SET_BFT_MOT] = 0x40;
		asp01_pdata.init_code[SET_TOU_RF_EXT] = 0x00;
		asp01_pdata.init_code[SET_OFF_TIME] = 0x30;
		asp01_pdata.init_code[SET_SENSE_TIME] = 0x28;
		asp01_pdata.init_code[SET_DUTY_TIME] = 0x50;
		asp01_pdata.init_code[SET_HW_CON1] = 0x78;
		asp01_pdata.init_code[SET_HW_CON2] = 0x27;
		asp01_pdata.init_code[SET_HW_CON3] = 0x20;
		asp01_pdata.init_code[SET_HW_CON4] = 0x27;
		asp01_pdata.init_code[SET_HW_CON5] = 0x83;
		asp01_pdata.init_code[SET_HW_CON6] = 0x3f;
		asp01_pdata.init_code[SET_HW_CON7] = 0x48;
		asp01_pdata.init_code[SET_HW_CON8] = 0x20;
		asp01_pdata.init_code[SET_HW_CON9] = 0x04;
		asp01_pdata.init_code[SET_HW_CON10] = 0x27;
		asp01_pdata.init_code[SET_HW_CON11] = 0x00;
	} else {
		asp01_pdata.init_code[SET_UNLOCK] = 0x5a;
		asp01_pdata.init_code[SET_RST_ERR] = 0x33;
		asp01_pdata.init_code[SET_PROX_PER] = 0x30;
		asp01_pdata.init_code[SET_PAR_PER] = 0x30;
		asp01_pdata.init_code[SET_TOUCH_PER] = 0x3c;
		asp01_pdata.init_code[SET_HI_CAL_PER] = 0x50;
		asp01_pdata.init_code[SET_BSMFM_SET] = 0x31;
		asp01_pdata.init_code[SET_ERR_MFM_CYC] = 0x33;
		asp01_pdata.init_code[SET_TOUCH_MFM_CYC] = 0x34;
		asp01_pdata.init_code[SET_SYS_FUNC] = 0x10;
		asp01_pdata.init_code[SET_HI_CAL_SPD] = 0x1a;
		asp01_pdata.init_code[SET_CAL_SPD] = 0x02;
		asp01_pdata.init_code[SET_BFT_MOT] = 0x40;
		asp01_pdata.init_code[SET_TOU_RF_EXT] = 0x22;
		asp01_pdata.init_code[SET_OFF_TIME] = 0x30;
		asp01_pdata.init_code[SET_SENSE_TIME] = 0x48;
		asp01_pdata.init_code[SET_DUTY_TIME] = 0x50;
		asp01_pdata.init_code[SET_HW_CON1] = 0x78;
		asp01_pdata.init_code[SET_HW_CON2] = 0x27;
		asp01_pdata.init_code[SET_HW_CON3] = 0x20;
		asp01_pdata.init_code[SET_HW_CON4] = 0x27;
		asp01_pdata.init_code[SET_HW_CON5] = 0x83;
		asp01_pdata.init_code[SET_HW_CON6] = 0x3f;
		asp01_pdata.init_code[SET_HW_CON7] = 0x48;
		asp01_pdata.init_code[SET_HW_CON8] = 0x20;
		asp01_pdata.init_code[SET_HW_CON9] = 0x04;
		asp01_pdata.init_code[SET_HW_CON10] = 0x37;
		asp01_pdata.init_code[SET_HW_CON11] = 0x11;
	}
#endif
#endif
}

#endif

#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651) || \
	defined(CONFIG_SENSORS_CM3663) || \
	defined(CONFIG_SENSORS_TMD27723) || defined(CONFIG_SENSORS_CM36653)
static int proximity_leda_on(bool onoff)
{
#if defined(CONFIG_MACH_TAB3)
	struct regulator *gp2a_vled;

	gp2a_vled = regulator_get(NULL, "leda_2.8v");

	if (IS_ERR(gp2a_vled)) {
		pr_err("gp2a: cannot get leda_2.8v\n");
		goto done;
	}
	if (onoff)
		regulator_enable(gp2a_vled);
	else
		regulator_disable(gp2a_vled);

	regulator_put(gp2a_vled);
done:
#else
	gpio_set_value(GPIO_PS_ALS_EN, onoff);
#endif
	pr_info("%s, onoff = %d\n", __func__, onoff);
	return 0;
}

static int optical_gpio_init(void)
{
	int ret = 0;
#if !defined(CONFIG_MACH_TAB3)
	ret = gpio_request(GPIO_PS_ALS_EN, "optical_power_supply_on");

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
#endif
	return ret;
}
#endif

#if defined(CONFIG_SENSORS_TMD27723)
static struct taos_platform_data taos_pdata = {
	.power  = proximity_leda_on,
	.als_int = GPIO_PS_ALS_INT,
	.prox_thresh_hi = 650,
	.prox_thresh_low = 460,
	.prox_th_hi_cal = 460,
	.prox_th_low_cal = 350,
	.als_time = 0xED,
	.intr_filter = 0x33,
	.prox_pulsecnt = 0x18,
	.prox_gain = 0x28,
	.coef_atime = 50,
	.ga = 58,
	.coef_a = 1000,
	.coef_b = 1760,
	.coef_c = 570,
	.coef_d = 963,
};
#endif

#if defined(CONFIG_SENSORS_CM36651)
/* Depends window, threshold is needed to be set */
static u8 cm36651_get_threshold(void)
{
	u8 threshold = 15;

	/* Add model config and threshold here. */
#if defined(CONFIG_MACH_M0_DUOSCTC)
	threshold = 13;
#elif defined(CONFIG_MACH_M0)
	threshold = 15;
#elif defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT) ||\
	defined(CONFIG_MACH_C1_KOR_LGT)
	if (system_rev >= 6)
		threshold = 15;
#elif defined(CONFIG_MACH_M3)
	threshold = 14;
#elif defined(CONFIG_MACH_C1)
	if (system_rev >= 7)
		threshold = 15;
#endif

	return threshold;
}

static struct cm36651_platform_data cm36651_pdata = {
	.cm36651_led_on = proximity_leda_on,
	.cm36651_get_threshold = cm36651_get_threshold,
	.irq = GPIO_PS_ALS_INT,
};
#endif

#if defined(CONFIG_SENSORS_CM36653)
/* Depends window, threshold is needed to be set */
static u16 cm36653_get_threshold(void)
{
	u8 threshold_L = 6;
	u8 threshold_H = 8;

	return (threshold_H << 8) | threshold_L;
}

static struct cm36653_platform_data cm36653_pdata = {
	.cm36653_led_on = proximity_leda_on,
	.cm36653_get_threshold = cm36653_get_threshold,
	.irq = GPIO_PS_ALS_INT,
};
#endif

#if defined(CONFIG_SENSORS_CM3663)
static struct cm3663_platform_data cm3663_pdata = {
	.proximity_power = proximity_leda_on,
};
#endif

#if defined(CONFIG_SENSORS_GP2A)
static unsigned long gp2a_get_threshold(u8 *thesh_diff)
{
	u8 threshold = 0x09;

	if (thesh_diff)
		*thesh_diff = 1;

#if defined(CONFIG_MACH_BAFFIN_KOR_SKT) || defined(CONFIG_MACH_BAFFIN_KOR_KT)
	if (system_rev > 4)
		threshold = 0x06;
	else
		threshold = 0x07;
	if (thesh_diff)
		*thesh_diff = 2;
#elif defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	if (system_rev > 5)
		threshold = 0x06;
	else
		threshold = 0x07;
	if (thesh_diff)
		*thesh_diff = 2;
#elif defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	threshold = 0x07;
	if (thesh_diff)
		*thesh_diff = 2;
#elif defined(CONFIG_MACH_M3_USA_TMO)
	threshold = 0x07;
	if (thesh_diff)
		*thesh_diff = 2;
#endif

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

static struct i2c_board_info i2c_devs9_emul[] __initdata = {
#if defined(CONFIG_SENSORS_GP2A)
	{
		I2C_BOARD_INFO("gp2a", (0x72 >> 1)),
	},
#endif
#if defined(CONFIG_SENSORS_CM36651)
	{
		I2C_BOARD_INFO("cm36651", (0x30 >> 1)),
		.platform_data = &cm36651_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_CM3663)
	{
		I2C_BOARD_INFO("cm3663", (0x20)),
		.irq = GPIO_PS_ALS_INT,
		.platform_data = &cm3663_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_BH1721)
	{
		I2C_BOARD_INFO("bh1721fvc", 0x23),
	},
#endif
#if defined(CONFIG_SENSORS_BH1730)
	{
		I2C_BOARD_INFO("bh1730fvc", 0x29),
	},
#endif
#if defined(CONFIG_SENSORS_AL3201)
	{
		I2C_BOARD_INFO("AL3201", 0x1c),
	},
#endif
#if defined(CONFIG_SENSORS_TMD27723)
	{
		I2C_BOARD_INFO("taos", 0x39),
		.platform_data = &taos_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_CM36653)
	{
		I2C_BOARD_INFO("cm36653", 0x60),
		.platform_data = &cm36653_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_CM3323)
	{
		I2C_BOARD_INFO("cm3323", 0x10),
	},
#endif
};

#ifdef CONFIG_SENSORS_AK8975C
static struct akm8975_platform_data akm8975_pdata = {
	.gpio_data_ready_int = GPIO_MSENSOR_INT,
};

static struct i2c_board_info i2c_devs10_emul[] __initdata = {
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
	},
};

static int ak8975c_gpio_init(void)
{
	int ret = gpio_request(GPIO_MSENSOR_INT, "gpio_akm_int");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio akm_int.(%d)\n",
			__func__, ret);
		return ret;
	}

	s5p_register_gpio_interrupt(GPIO_MSENSOR_INT);
	s3c_gpio_setpull(GPIO_MSENSOR_INT, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(GPIO_MSENSOR_INT, S3C_GPIO_SFN(0xF));
	i2c_devs10_emul[0].irq = gpio_to_irq(GPIO_MSENSOR_INT);
	return ret;
}
#endif

#ifdef CONFIG_SENSORS_AK8963C
static u8 ak8963_get_position(void)
{
	u8 position;

#if defined(CONFIG_MACH_ZEST)
	position = TYPE3_BOTTOM_UPPER_RIGHT;
#elif defined(CONFIG_MACH_SF2)
	if(system_rev <= 14)
		position = TYPE3_TOP_LOWER_LEFT;
	else
		position = TYPE3_BOTTOM_UPPER_RIGHT;
#elif defined(CONFIG_MACH_GC1)
	position = TYPE3_TOP_LOWER_LEFT;
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

#ifdef CONFIG_SENSORS_AK8963
static char ak_get_layout(void)
{
	char layout = 0;
#ifdef CONFIG_MACH_BAFFIN
	if (system_rev >= 1)
		layout = 3;
	else
		layout = 4;
#endif
	return layout;
}

static struct akm8963_platform_data akm8963_pdata = {
	.layout = ak_get_layout,
	.outbit = 1,
	.gpio_RST = GPIO_MSENSE_RST_N,
};

static struct i2c_board_info i2c_devs10_emul[] __initdata = {
	{
		I2C_BOARD_INFO("akm8963", 0x0C),
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

#ifdef CONFIG_SENSORS_YAS532
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

static void yas532_set_layout(void)
{
#if defined(CONFIG_TARGET_TAB3_3G10) ||\
	defined(CONFIG_TARGET_TAB3_WIFI10) ||\
	defined(CONFIG_TARGET_TAB3_LTE10)
	magnetic_pdata.orientation = YAS532_POSITION_4;
#elif defined(CONFIG_TARGET_TAB3_3G8) ||\
	defined(CONFIG_TARGET_TAB3_WIFI8) ||\
	defined(CONFIG_TARGET_TAB3_LTE8)
	magnetic_pdata.orientation = YAS532_POSITION_7;
#else /* TAB3 7 inch */
	magnetic_pdata.orientation = YAS532_POSITION_4;
#endif
}
#endif

#ifdef CONFIG_SENSORS_LPS331
static int lps331_gpio_init(void)
{
	int ret = gpio_request(GPIO_BARO_INT, "lps331_irq");

	pr_info("%s\n", __func__);

	if (ret) {
		pr_err("%s, Failed to request gpio lps331_irq(%d)\n",
			__func__, ret);
		return ret;
	}

	s3c_gpio_cfgpin(GPIO_BARO_INT, S3C_GPIO_INPUT);
	gpio_set_value(GPIO_BARO_INT, 2);
	s3c_gpio_setpull(GPIO_BARO_INT, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(GPIO_BARO_INT, S5P_GPIO_DRVSTR_LV1);
	return ret;
}

static struct lps331ap_platform_data lps331ap_pdata = {
	.irq =	GPIO_BARO_INT,
};

static struct i2c_board_info i2c_devs11_emul[] __initdata = {
	{
		I2C_BOARD_INFO("lps331ap", 0x5D),
		.platform_data = &lps331ap_pdata,
	},
};
#endif

static int __init midas_sensor_init(void)
{
	int ret = 0;
#if defined(CONFIG_SENSORS_SSP_STM)
	unsigned int gpio;
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &exynos_device_spi1.dev;
#endif

	/* Gyro & Accelerometer Sensor */
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
#elif defined(CONFIG_SENSORS_K3DH)
	ret = accel_gpio_init();
	if (ret < 0) {
		pr_err("%s, accel_gpio_init fail(err=%d)\n", __func__, ret);
		return ret;
	}
#elif defined(CONFIG_INPUT_MPU6050)
	ret = mpu_gpio_init();
	if (ret < 0) {
		pr_err("%s, mpu_gpio_init fail(err=%d)\n", __func__, ret);
		return ret;
	}
#elif defined(CONFIG_SENSORS_SSP)
	initialize_ssp_gpio();
#endif

#if defined(CONFIG_SENSORS_SSP_STM)

	sclk = clk_get(spi1_dev, "dout_spi1");
	if (IS_ERR(sclk))
		dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi1_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi1_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent\n");

	clk_set_rate(sclk, 500 * 1000);
	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(GPIO_SHUB_SPI_SSN, "SPI_CS1")) {
		gpio_direction_output(GPIO_SHUB_SPI_SSN, 1);
		s3c_gpio_cfgpin(GPIO_SHUB_SPI_SSN, S3C_GPIO_SFN(1));
		s3c_gpio_setpull(GPIO_SHUB_SPI_SSN, S3C_GPIO_PULL_UP);
		exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				    ARRAY_SIZE(spi1_csi));
	}

	for (gpio = GPIO_SHUB_SPI_SCK; gpio <= GPIO_SHUB_SPI_MOSI; gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	ret = spi_register_board_info(spi1_board_info,
			ARRAY_SIZE(spi1_board_info));
	if (ret < 0) {
		pr_err("[SSP]: %s, failed to register spi(err = %d)\n",
			__func__, ret);
		return ret;
	}

	ret = platform_device_register(&exynos_device_spi1);
	if (ret < 0) {
		pr_err("[SSP]: %s, failed to register platform_device"
			" (err = %d)\n", __func__, ret);
		return ret;
	}

#else
	ret = i2c_add_devices(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	if (ret < 0) {
		pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

	/* Grip Sensor */
#ifdef CONFIG_SENSORS_ASP01
	grip_gpio_init();
	grip_init_code_set();
	ret = i2c_add_devices(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
	if (ret < 0) {
		pr_err("%s, i2c6 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif
	/* Optical Sensor */
#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651) || \
	defined(CONFIG_SENSORS_CM3663) || defined(CONFIG_SENSORS_TMD27723) || \
	defined(CONFIG_SENSORS_CM36653)
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
#elif defined(CONFIG_SENSORS_BH1721) || defined(CONFIG_SENSORS_AL3201) || \
	defined(CONFIG_SENSORS_BH1730) || defined(CONFIG_SENSORS_CM3323)
	ret = i2c_add_devices(9, i2c_devs9_emul, ARRAY_SIZE(i2c_devs9_emul));
	if (ret < 0) {
		pr_err("%s, i2c9 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif

#if defined(CONFIG_SENSORS_GP2A)
	ret = platform_device_register(&opt_gp2a);
	if (ret < 0) {
		pr_err("%s, failed to register opt_gp2a(err=%d)\n",
			__func__, ret);
		return ret;
	}
#endif

	/* Magnetic Sensor */
#ifdef CONFIG_SENSORS_AK8975C
	ret = ak8975c_gpio_init();
	if (ret < 0) {
		pr_err("%s, ak8975c_gpio_init fail(err=%d)\n", __func__, ret);
		return ret;
	}
	ret = i2c_add_devices(10, i2c_devs10_emul, ARRAY_SIZE(i2c_devs10_emul));
	if (ret < 0) {
		pr_err("%s, i2c10 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif
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
#ifdef CONFIG_SENSORS_AK8963
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
#ifdef CONFIG_SENSORS_YAS532
	yas532_set_layout();
	ret = i2c_add_devices(10, i2c_devs10_emul,
		ARRAY_SIZE(i2c_devs10_emul));
	if (ret < 0) {
		pr_err("%s, i2c10 adding i2c fail(err=%d)\n",
			__func__, ret);
		return ret;
	}
#endif
	/* Pressure Sensor */
#ifdef CONFIG_SENSORS_LPS331
	ret = lps331_gpio_init();
	if (ret < 0) {
		pr_err("%s, lps331_gpio_init fail(err=%d)\n", __func__, ret);
		return ret;
	}
	ret = i2c_add_devices(11, i2c_devs11_emul, ARRAY_SIZE(i2c_devs11_emul));
	if (ret < 0) {
		pr_err("%s, i2c1 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#endif
	return ret;
}
module_init(midas_sensor_init);
