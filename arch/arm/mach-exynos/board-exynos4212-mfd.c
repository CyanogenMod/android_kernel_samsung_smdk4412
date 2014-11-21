/* linux/arch/arm/mach-exynos/board-exynos4212-mfd.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>
#include <mach/irqs.h>

#include "board-exynos4212.h"

#ifdef CONFIG_MFD_MAX14577
#include <linux/mfd/max14577.h>
#include <linux/mfd/max14577-private.h>

#if defined(CONFIG_USE_MUIC)
#include <linux/muic/muic.h>
#endif /* CONFIG_USE_MUIC */
#endif /* CONFIG_MFD_MAX14577 */

#ifdef CONFIG_MFD_MAX77693
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>

#ifdef CONFIG_LEDS_MAX77693
#include <linux/leds-max77693.h>
#endif /* CONFIG_LEDS_MAX77693 */
#endif /* CONFIG_MFD_MAX77693 */

#ifdef CONFIG_MFD_MAX14577
#if defined(CONFIG_MUIC_MAX14577)
extern struct muic_platform_data max14577_muic_pdata;

static int muic_init_gpio_cb(int switch_sel)
{
	struct muic_platform_data *pdata = &max14577_muic_pdata;
	const char *usb_mode;
	const char *uart_mode;
	int ret = 0;

	pr_info("%s\n", __func__);

	if (switch_sel & SWITCH_SEL_USB_MASK) {
		pdata->usb_path = MUIC_PATH_USB_AP;
		usb_mode = "PDA";
	} else {
		pdata->usb_path = MUIC_PATH_USB_CP;
		usb_mode = "MODEM";
	}

	if (pdata->set_gpio_usb_sel)
		ret = pdata->set_gpio_usb_sel(pdata->uart_path);

	if (switch_sel & SWITCH_SEL_UART_MASK) {
		pdata->uart_path = MUIC_PATH_UART_AP;
		uart_mode = "AP";
	} else {
		pdata->uart_path = MUIC_PATH_UART_CP;
		uart_mode = "CP";
	}

	if (pdata->set_gpio_uart_sel)
		ret = pdata->set_gpio_uart_sel(pdata->uart_path);

	pr_info("%s: usb_path(%s), uart_path(%s)\n", __func__, usb_mode,
			uart_mode);

	return ret;
}

struct muic_platform_data max14577_muic_pdata = {
	.init_gpio_cb	= muic_init_gpio_cb,
};
#endif /* CONFIG_MUIC_MAX14577 */

extern struct max14577_regulator_data max14577_regulators;

extern struct max14577_platform_data exynos4_max14577_info;

static int max14577_set_gpio_pogo_cb(int new_dev)
{
	struct max14577_platform_data *pdata = &exynos4_max14577_info;
	int gpio_val = GPIO_LEVEL_LOW;
	int ret = 0;

	pr_info("%s new_dev(%d)\n", __func__, new_dev);

	switch (new_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		gpio_val = GPIO_LEVEL_HIGH;
		break;
	default:
		gpio_val = GPIO_LEVEL_LOW;
		break;
	}

	if (pdata->set_gpio_pogo_vbatt_en)
		ret = pdata->set_gpio_pogo_vbatt_en(gpio_val);

	/* wait 500ms for safely switching VBATT <-> VBUS voltage input */
	msleep(500);

	if (pdata->set_gpio_pogo_vbus_en)
		ret = pdata->set_gpio_pogo_vbus_en(gpio_val);

	return ret;
}

static int max14577_set_gpio_pogo_vbatt_en(int gpio_val)
{
	const char *mode;
	int pogo_vbatt_en_gpio = exynos4_max14577_info.gpio_pogo_vbatt_en;
	int pogo_vbatt_en_val;
	int ret;

	ret = gpio_request(pogo_vbatt_en_gpio, "GPIO_POGO_VBATT_EN");
	if (ret) {
		pr_err("failed to gpio_request GPIO_POGO_VBATT_EN\n");
		return ret;
	}

	pogo_vbatt_en_val = gpio_get_value(pogo_vbatt_en_gpio);

	pr_info("%s: pogo_vbatt_en(%d), GPIO_POGO_VBATT_EN(%d)=%c ->",
			__func__, gpio_val, pogo_vbatt_en_gpio,
			(pogo_vbatt_en_val == GPIO_LEVEL_LOW ? 'L' : 'H'));

	if (gpio_val == GPIO_LEVEL_LOW) {
		mode = "POGO_VBATT_EN DISABLE";
	} else if (gpio_val == GPIO_LEVEL_HIGH) {
		mode = "POGO_VBATT_EN ENABLE";
	} else {
		mode = "Error";
		goto out;
	}

	if (gpio_is_valid(pogo_vbatt_en_gpio))
		gpio_set_value(pogo_vbatt_en_gpio, gpio_val);

out:
	pogo_vbatt_en_val = gpio_get_value(pogo_vbatt_en_gpio);

	gpio_free(pogo_vbatt_en_gpio);

	pr_info(" %s, GPIO_POGO_VBATT_EN(%d)=%c\n", mode, pogo_vbatt_en_gpio,
			(pogo_vbatt_en_val == GPIO_LEVEL_LOW ? 'L' : 'H'));

	return 0;
}

static int max14577_set_gpio_pogo_vbus_en(int gpio_val)
{
	const char *mode;
	int pogo_vbus_en_gpio = exynos4_max14577_info.gpio_pogo_vbus_en;
	int pogo_vbus_en_val;
	int ret;

	ret = gpio_request(pogo_vbus_en_gpio, "GPIO_POGO_VBUS_EN");
	if (ret) {
		pr_err("failed to gpio_request GPIO_POGO_VBUS_EN\n");
		return ret;
	}

	pogo_vbus_en_val = gpio_get_value(pogo_vbus_en_gpio);

	pr_info("%s: pogo_vbus_en(%d), GPIO_POGO_VBUS_EN(%d)=%c ->",
			__func__, gpio_val, pogo_vbus_en_gpio,
			(pogo_vbus_en_val == GPIO_LEVEL_LOW ? 'L' : 'H'));

	if (gpio_val == GPIO_LEVEL_LOW) {
		mode = "POGO_VBUS_EN DISABLE";
	} else if (gpio_val == GPIO_LEVEL_HIGH) {
		mode = "POGO_VBUS_EN ENABLE";
	} else {
		mode = "Error";
		goto out;
	}

	if (gpio_is_valid(pogo_vbus_en_gpio))
		gpio_set_value(pogo_vbus_en_gpio, gpio_val);

out:
	pogo_vbus_en_val = gpio_get_value(pogo_vbus_en_gpio);

	gpio_free(pogo_vbus_en_gpio);

	pr_info(" %s, GPIO_POGO_VBUS_EN(%d)=%c\n", mode, pogo_vbus_en_gpio,
			(pogo_vbus_en_val == GPIO_LEVEL_LOW ? 'L' : 'H'));

	return 0;
}

struct max14577_platform_data exynos4_max14577_info = {
	.irq_base		= IRQ_BOARD_IFIC_START,
	.irq_gpio		= GPIO_IF_PMIC_IRQ,
	.wakeup			= true,

#if defined(GPIO_POGO_VBATT_EN)
	.gpio_pogo_vbatt_en	= GPIO_POGO_VBATT_EN,
	.set_gpio_pogo_vbatt_en	= max14577_set_gpio_pogo_vbatt_en,
#endif /* GPIO_POGO_VBATT_EN */

#if defined(GPIO_POGO_VBUS_EN)
	.gpio_pogo_vbus_en	= GPIO_POGO_VBUS_EN,
	.set_gpio_pogo_vbus_en	= max14577_set_gpio_pogo_vbus_en,
#endif /* GPIO_POGO_VBUS_EN */

#if defined(CONFIG_MUIC_MAX14577)
	.muic_pdata		= &max14577_muic_pdata,
#endif /* CONFIG_MUIC_MAX14577 */

	.num_regulators		= MAX14577_REG_MAX,
	.regulators		= &max14577_regulators,
};
#endif /* CONFIG_MFD_MAX14577 */

#ifdef CONFIG_MFD_MAX77693
extern struct max77693_muic_data max77693_muic;
extern struct max77693_regulator_data max77693_regulators;

#ifdef CONFIG_LEDS_MAX77693
struct max77693_led_platform_data max77693_led_pdata = {
	.num_leds = 4,

	.leds[0].name = "leds-sec1",
	.leds[0].id = MAX77693_FLASH_LED_1,
	.leds[0].timer = MAX77693_FLASH_TIME_500MS,
	.leds[0].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = 0x1F,

	.leds[1].name = "leds-sec2",
	.leds[1].id = MAX77693_FLASH_LED_2,
	.leds[1].timer = MAX77693_FLASH_TIME_500MS,
	.leds[1].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[1].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = 0x1F,

	.leds[2].name = "torch-sec1",
	.leds[2].id = MAX77693_TORCH_LED_1,
	.leds[2].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[2].brightness = 0x03,

	.leds[3].name = "torch-sec2",
	.leds[3].id = MAX77693_TORCH_LED_2,
	.leds[3].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[3].brightness = 0x04,
};
#endif

#if defined(CONFIG_MACH_GC1)
static void motor_init_hw(void)
{
	if (gpio_request(EXYNOS4_GPD0(0), "VIBTONE_PWM") < 0)
		printk(KERN_ERR "[VIB] gpio requst is failed\n");
	else {
		gpio_direction_output(EXYNOS4_GPD0(0), 0);
		printk(KERN_DEBUG "[VIB] gpio request is succeed\n");
	}
}

static void motor_en(bool enable)
{
	gpio_direction_output(EXYNOS4_GPD0(0), enable);
	printk(KERN_DEBUG "[VIB] motor_enabled GPIO GPD0(0) : %d\n",
	       gpio_get_value(EXYNOS4_GPD0(0)));
}
#endif
#ifdef CONFIG_MACH_BAFFIN
static void motor_en(bool enable)
{
	gpio_direction_output(EXYNOS4_GPY2(2), enable);
	printk(KERN_DEBUG "[VIB] motor_enabled GPIO GPY2(2) : %d\n",
	       gpio_get_value(EXYNOS4_GPY2(2)));
}
#endif
#if defined(CONFIG_MACH_T0) && defined(CONFIG_TARGET_LOCALE_KOR) || \
	defined(CONFIG_MACH_T0_JPN_LTE_DCM)
static void motor_en(bool enable)
{
	gpio_direction_output(EXYNOS4_GPC0(3), enable);
	printk(KERN_DEBUG "[VIB] motor_enabled GPIO GPC0(3) : %d\n",
	       gpio_get_value(EXYNOS4_GPC0(3)));
}
#endif
#ifdef CONFIG_VIBETONZ
static struct max77693_haptic_platform_data max77693_haptic_pdata = {
#if defined(CONFIG_MACH_GC1)
	.reg2 = MOTOR_ERM,
	.pwm_id = 1,
	.init_hw = motor_init_hw,
	.motor_en = motor_en,
#else
	.reg2 = MOTOR_LRA | EXT_PWM | DIVIDER_128,
	.pwm_id = 0,
	.init_hw = NULL,
	.motor_en = NULL,
#endif
	.max_timeout = 10000,
#if defined(CONFIG_MACH_GC2PD)
	.duty = 37900,
#else
	.duty = 35500,
#endif
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	.period = 38295,
#elif defined(CONFIG_MACH_ZEST)
	.period = 38054,
#else
	.period = 37904,
#endif
	.regulator_name = "vmotor",
};
#endif
#ifdef CONFIG_BATTERY_MAX77693_CHARGER
static struct max77693_charger_platform_data max77693_charger_pdata = {
#ifdef CONFIG_BATTERY_WPC_CHARGER
	.wpc_irq_gpio = GPIO_WPC_INT,
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_GD2)
	.vbus_irq_gpio = GPIO_V_BUS_INT,
#endif
#if defined(CONFIG_MACH_T0) ||  \
	defined(CONFIG_MACH_GD2)
	.wc_pwr_det = true,
#else
	.wc_pwr_det = false,
#endif
#endif
};
#endif

struct max77693_platform_data exynos4_max77693_info = {
	.irq_base	= IRQ_BOARD_IFIC_START,
	.irq_gpio	= GPIO_IF_PMIC_IRQ,
	.wakeup		= 1,
	.muic = &max77693_muic,
	.regulators = &max77693_regulators,
	.num_regulators = MAX77693_REG_MAX,
#if defined(CONFIG_CHARGER_MAX77693_BAT)
	.charger_data	= &sec_battery_pdata,
#elif defined(CONFIG_BATTERY_MAX77693_CHARGER)
	.charger_data = &max77693_charger_pdata,
#endif
#ifdef CONFIG_VIBETONZ
	.haptic_data = &max77693_haptic_pdata,
#endif
#ifdef CONFIG_LEDS_MAX77693
	.led_data = &max77693_led_pdata,
#endif
};
#endif /* CONFIG_MFD_MAX77693 */

#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
/* I2C17 */
static struct i2c_board_info i2c_devs17_emul[] __initdata = {
#ifdef CONFIG_MFD_MAX14577
	{
		I2C_BOARD_INFO(MFD_DEV_NAME, MAX14577_I2C_ADDR),
		.platform_data	= &exynos4_max14577_info,
	}
#endif

#ifdef CONFIG_MFD_MAX77693
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data	= &exynos4_max77693_info,
	}
#endif
};

static struct i2c_gpio_platform_data gpio_i2c_data17 = {
	.sda_pin = GPIO_IF_PMIC_SDA,
	.scl_pin = GPIO_IF_PMIC_SCL,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &gpio_i2c_data17,
};
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */

static struct platform_device *exynos4_mfd_device[] __initdata = {
#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
	&s3c_device_i2c17,
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */
};

void __init exynos4_exynos4212_mfd_init(void)
{
	pr_info("%s\n", __func__);

#if defined(CONFIG_WATCH_02_BD)
	if (system_rev == 0x02)
		exynos4_max14577_info.set_gpio_pogo_cb = max14577_set_gpio_pogo_cb;
#endif /* CONFIG_WATCH_02_BD */

#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
	i2c_register_board_info(17, i2c_devs17_emul,
				ARRAY_SIZE(i2c_devs17_emul));
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */

	platform_add_devices(exynos4_mfd_device, ARRAY_SIZE(exynos4_mfd_device));
}
