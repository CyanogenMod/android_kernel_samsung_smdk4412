/*
 * arch/arm/mach-exynos/board-exynos4212-muic.c
 *
 * c source file supporting MUIC board specific register
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>
#include <mach/irqs.h>

#include <linux/muic/muic.h>

#include "board-exynos4212.h"

extern struct muic_platform_data exynos4_muic_pdata;

static int muic_init_gpio_cb(int switch_sel)
{
	struct muic_platform_data *pdata = &exynos4_muic_pdata;
	const char *usb_mode;
	const char *uart_mode;
	int ret = 0;

	pr_info("%s\n", __func__);

#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER)
	mutex_init(&pdata->muic_plat_mutex);
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER */

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

#if defined(CONFIG_MUIC_USE_11PIN_MHL_TO_USB)
	if (pdata->set_gpio_usb_mhl_sel)
		pdata->set_gpio_usb_mhl_sel(MUIC_PATH_DP_DM);
#endif /* CONFIG_MUIC_USE_11PIN_MHL_TO_USB */

	return ret;
}

static int muic_set_gpio_usb_sel(int usb_sel)
{
	return 0;
}

static int muic_set_gpio_uart_sel(int uart_sel)
{
	const char *mode;
	int uart_sel_gpio = exynos4_muic_pdata.gpio_uart_sel;
	int uart_sel_val;
	int ret;

	ret = gpio_request(uart_sel_gpio, "GPIO_UART_SEL");
	if (ret) {
		pr_err("failed to gpio_request GPIO_UART_SEL\n");
		return ret;
	}

	uart_sel_val = gpio_get_value(uart_sel_gpio);

	pr_info("%s: uart_sel(%d), GPIO_UART_SEL(%d)=%c ->", __func__, uart_sel,
			uart_sel_gpio, (uart_sel_val == 0 ? 'L' : 'H'));

	switch (uart_sel) {
	case MUIC_PATH_UART_AP:
		mode = "AP_UART";
		if (gpio_is_valid(uart_sel_gpio))
			gpio_set_value(uart_sel_gpio, GPIO_LEVEL_HIGH);
		break;
	case MUIC_PATH_UART_CP:
		mode = "CP_UART";
		if (gpio_is_valid(uart_sel_gpio))
			gpio_set_value(uart_sel_gpio, GPIO_LEVEL_LOW);
		break;
	default:
		mode = "Error";
		break;
	}

	uart_sel_val = gpio_get_value(uart_sel_gpio);

	gpio_free(uart_sel_gpio);

	pr_info(" %s, GPIO_UART_SEL(%d)=%c\n", mode, uart_sel_gpio,
			(uart_sel_val == 0 ? 'L' : 'H'));

	return 0;
}

#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER)
static bool muic_is_vbus_high(void)
{
	const char *mode;
	int vbus_irq_gpio = exynos4_muic_pdata.vbus_irq_gpio;
	int vbus_val;
	bool ret;

	ret = gpio_request(vbus_irq_gpio, "GPIO_V_BUS_INT");
	if (ret) {
		pr_err("failed to gpio_request GPIO_V_BUS_INT\n");
		return ret;
	}

	vbus_val = gpio_get_value(vbus_irq_gpio);
	gpio_free(vbus_irq_gpio);

	switch (vbus_val) {
	case GPIO_LEVEL_LOW:
		mode = "V BUS INT Low";
		ret = false;
		break;
	case GPIO_LEVEL_HIGH:
		mode = "V BUS INT High";
		ret = true;
		break;
	default:
		mode = "Error";
		ret = false;
		break;
	}

	pr_info("%s: GPIO_V_BUS_INT(%d)=%c, %s", __func__, vbus_irq_gpio,
			(vbus_val == GPIO_LEVEL_LOW ? 'L' : 'H'), mode);

	return ret;
}
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER */

#if defined(CONFIG_MUIC_USE_11PIN_MHL_TO_USB)
static int muic_set_gpio_usb_mhl_sel(int usb_mhl_sel)
{
	const char *mode;
	int usb_mhl_sel_gpio = exynos4_muic_pdata.gpio_usb_mhl_sel;
	int usb_mhl_sel_val;
	int ret;

	ret = gpio_request(usb_mhl_sel_gpio, "GPIO_USB_MHL_SEL");
	if (ret) {
		pr_err("failed to gpio_request GPIO_USB_MHL_SEL\n");
		return ret;
	}

	usb_mhl_sel_val = gpio_get_value(usb_mhl_sel_gpio);

	pr_info("%s: usb_mhl_sel(%d), GPIO_USB_MHL_SEL(%d)=%c ->", __func__,
			usb_mhl_sel, usb_mhl_sel_gpio,
			(usb_mhl_sel_val == 0 ? 'L' : 'H'));

	switch (usb_mhl_sel) {
	case MUIC_PATH_DP_DM:
		mode = "11Pin Path to D+/D-";
		if (gpio_is_valid(usb_mhl_sel_gpio))
			gpio_set_value(usb_mhl_sel_gpio, GPIO_LEVEL_HIGH);
		break;
	case MUIC_PATH_MHL:
		mode = "11Pin Path to MHL+/MHL-";
		if (gpio_is_valid(usb_mhl_sel_gpio))
			gpio_set_value(usb_mhl_sel_gpio, GPIO_LEVEL_LOW);
		break;
	default:
		mode = "usb_mhl_sel Error";
		break;
	}

	usb_mhl_sel_val = gpio_get_value(usb_mhl_sel_gpio);

	gpio_free(usb_mhl_sel_gpio);

	pr_info(" %s, GPIO_USB_MHL_SEL(%d)=%c\n", mode, usb_mhl_sel_gpio,
			(usb_mhl_sel_val == 0 ? 'L' : 'H'));

	return 0;
}
#endif /* CONFIG_MUIC_USE_11PIN_MHL_TO_USB */

#if defined(CONFIG_MUIC_FSA9485)
struct muic_platform_data exynos4_muic_pdata = {
	.irq_gpio		= GPIO_MUIC_IRQ,

#if defined(GPIO_USB_SEL)
	.gpio_usb_sel		= GPIO_USB_SEL,
	.set_gpio_usb_sel	= muic_set_gpio_usb_sel,
#endif /* GPIO_USB_SEL */

#if defined(GPIO_UART_SEL)
	.gpio_uart_sel		= GPIO_UART_SEL,
	.set_gpio_uart_sel	= muic_set_gpio_uart_sel,
#endif /* GPIO_UART_SEL */

#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER)
	.vbus_irq_gpio		= GPIO_V_BUS_INT,
	.is_vbus_high		= muic_is_vbus_high,
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER */

#if defined(CONFIG_MUIC_USE_11PIN_MHL_TO_USB)
	.gpio_usb_mhl_sel	= GPIO_USB_SEL1,
	.set_gpio_usb_mhl_sel	= muic_set_gpio_usb_mhl_sel,
#endif /* CONFIG_MUIC_USE_11PIN_MHL_TO_USB */

	.init_gpio_cb		= muic_init_gpio_cb,
};
#endif /* CONFIG_MUIC_FSA9485 */

#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
/* I2C17 */
static struct i2c_board_info i2c_devs17_emul[] __initdata = {
#if defined(CONFIG_MUIC_FSA9485)
	{
		I2C_BOARD_INFO(MUIC_DEV_NAME, (0x4A >> 1)),
		.platform_data = &exynos4_muic_pdata,
	}
#endif /* CONFIG_MUIC_FSA9485 */
};

static struct i2c_gpio_platform_data gpio_i2c_data17 = {
	.sda_pin = GPIO_MUIC_SDA,
	.scl_pin = GPIO_MUIC_SCL,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &gpio_i2c_data17,
};
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */

static struct platform_device *exynos4_muic_device[] __initdata = {
#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
	&s3c_device_i2c17,
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */
};

void __init exynos4_exynos4212_muic_init(void)
{
#if defined(CONFIG_MUIC_I2C_USE_I2C17_EMUL)
	i2c_register_board_info(17, i2c_devs17_emul,
				ARRAY_SIZE(i2c_devs17_emul));
#endif /* CONFIG_MUIC_I2C_USE_I2C17_EMUL */

	platform_add_devices(exynos4_muic_device,
			ARRAY_SIZE(exynos4_muic_device));
}
