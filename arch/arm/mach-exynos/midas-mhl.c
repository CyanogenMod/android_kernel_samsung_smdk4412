#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sii9234.h>

#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include "midas.h"

#ifdef CONFIG_SAMSUNG_MHL
static void sii9234_cfg_gpio(void)
{
	printk(KERN_INFO "%s()\n", __func__);

	/* AP_MHL_SDA */
	s3c_gpio_cfgpin(GPIO_MHL_SDA_1_8V, S3C_GPIO_SFN(0x0));
	s3c_gpio_setpull(GPIO_MHL_SDA_1_8V, S3C_GPIO_PULL_NONE);

	/* AP_MHL_SCL */
	s3c_gpio_cfgpin(GPIO_MHL_SCL_1_8V, S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(GPIO_MHL_SCL_1_8V, S3C_GPIO_PULL_NONE);

	/* GPH1(6) XEINT 14 */
	s3c_gpio_cfgpin(GPIO_MHL_WAKE_UP, S3C_GPIO_INPUT);
	irq_set_irq_type(MHL_WAKEUP_IRQ, IRQ_TYPE_EDGE_RISING);
	s3c_gpio_setpull(GPIO_MHL_WAKE_UP, S3C_GPIO_PULL_DOWN);

	gpio_request(GPIO_MHL_INT, "MHL_INT");
	s5p_register_gpio_interrupt(GPIO_MHL_INT);
	s3c_gpio_setpull(GPIO_MHL_INT, S3C_GPIO_PULL_DOWN);
	irq_set_irq_type(MHL_INT_IRQ, IRQ_TYPE_EDGE_RISING);
	s3c_gpio_cfgpin(GPIO_MHL_INT, GPIO_MHL_INT_AF);

	s3c_gpio_cfgpin(GPIO_HDMI_EN, S3C_GPIO_OUTPUT);	/* HDMI_EN */
	gpio_set_value(GPIO_HDMI_EN, GPIO_LEVEL_LOW);
	s3c_gpio_setpull(GPIO_HDMI_EN, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_MHL_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MHL_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);

#if !defined(CONFIG_MACH_C1_KOR_LGT) && !defined(CONFIG_SAMSUNG_MHL_9290)
#if !defined(CONFIG_MACH_P4NOTE)
	s3c_gpio_cfgpin(GPIO_MHL_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);
#endif
#endif
}

static void sii9234_power_onoff(bool on)
{
	printk(KERN_INFO "%s(%d)\n", __func__, on);

	if (on) {
		/* To avoid floating state of the HPD pin *
		 * in the absence of external pull-up     */
		s3c_gpio_setpull(GPIO_HDMI_HPD, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_HDMI_EN, GPIO_LEVEL_HIGH);

		s3c_gpio_setpull(GPIO_MHL_SCL_1_8V, S3C_GPIO_PULL_DOWN);
		s3c_gpio_setpull(GPIO_MHL_SCL_1_8V, S3C_GPIO_PULL_NONE);

		/* sii9234_unmaks_interrupt(); // - need to add */
		/* VCC_SUB_2.0V is always on */
	} else {
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
		usleep_range(10000, 20000);
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_HIGH);

		/* To avoid floating state of the HPD pin *
		 * in the absence of external pull-up     */
		s3c_gpio_setpull(GPIO_HDMI_HPD, S3C_GPIO_PULL_DOWN);
		gpio_set_value(GPIO_HDMI_EN, GPIO_LEVEL_LOW);

		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
	}
}

static void sii9234_reset(void)
{
	printk(KERN_INFO "%s()\n", __func__);

	s3c_gpio_cfgpin(GPIO_MHL_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MHL_RST, S3C_GPIO_PULL_NONE);


	gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
	usleep_range(10000, 20000);
	gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_HIGH);
}

#ifndef CONFIG_SAMSUNG_USE_11PIN_CONNECTOR
#ifndef CONFIG_MACH_P4NOTE
static void mhl_usb_switch_control(bool on)
{
	printk(KERN_INFO "%s() [MHL] USB path change : %s\n",
	       __func__, on ? "MHL" : "USB");
	if (on == 1) {
		if (gpio_get_value(GPIO_MHL_SEL))
			printk(KERN_INFO "[MHL] GPIO_MHL_SEL : already 1\n");
		else
			gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_HIGH);
	} else {
		if (!gpio_get_value(GPIO_MHL_SEL))
			printk(KERN_INFO "[MHL] GPIO_MHL_SEL : already 0\n");
		else
			gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);
	}
}
#endif
#endif

static struct sii9234_platform_data sii9234_pdata = {
	.init = sii9234_cfg_gpio,
#if defined(CONFIG_SAMSUNG_USE_11PIN_CONNECTOR) || \
		defined(CONFIG_MACH_P4NOTE)
	.mhl_sel = NULL,
#else
	.mhl_sel = mhl_usb_switch_control,
#endif
	.hw_onoff = sii9234_power_onoff,
	.hw_reset = sii9234_reset,
	.enable_vbus = NULL,
#if defined(__MHL_NEW_CBUS_MSC_CMD__)
	.vbus_present = NULL,
#else
	.vbus_present = NULL,
#endif

#ifdef CONFIG_EXTCON
	.extcon_name = "max77693-muic",
#endif
};

static struct i2c_board_info __initdata i2c_devs_sii9234[] = {
	{
		I2C_BOARD_INFO("sii9234_mhl_tx", 0x72>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_tpi", 0x7A>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_hdmi_rx", 0x92>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_cbus", 0xC8>>1),
		.platform_data = &sii9234_pdata,
	},
};

static struct i2c_board_info i2c_dev_hdmi_ddc __initdata = {
	I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
};

static int __init midas_mhl_init(void)
{
	int ret;
#define I2C_BUS_ID_MHL	15
	ret = i2c_add_devices(I2C_BUS_ID_MHL, i2c_devs_sii9234,
			ARRAY_SIZE(i2c_devs_sii9234));

	if (ret < 0) {
		printk(KERN_ERR "[MHL] adding i2c fail - nodevice\n");
		return -ENODEV;
	}
#if defined(CONFIG_MACH_S2PLUS) || defined(CONFIG_MACH_P4NOTE)
	sii9234_pdata.ddc_i2c_num = 5;
#else
	sii9234_pdata.ddc_i2c_num = (system_rev == 3 ? 16 : 5);
#endif

#ifdef CONFIG_MACH_SLP_PQ_LTE
	sii9234_pdata.ddc_i2c_num = 16;
#endif
	ret = i2c_add_devices(sii9234_pdata.ddc_i2c_num, &i2c_dev_hdmi_ddc, 1);
	if (ret < 0) {
		printk(KERN_ERR "[MHL] adding ddc fail - nodevice\n");
		return -ENODEV;
	}

	return 0;
}
module_init(midas_mhl_init);
#endif
