/* linux/arch/arm/mach-exynos/mach-smdk4212.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#ifdef CONFIG_KEYBOARD_GPIO
#include <linux/gpio_keys.h>
#endif
#ifdef CONFIG_INPUT_SEC_KEYS
#include <linux/sec_keys.h>
#endif
#include <linux/gpio_event.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/input.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_LEDS_AAT1290A
#include <linux/leds-aat1290a.h>
#endif
#include <asm/io.h>
#ifdef CONFIG_LEDS_IR
#include <linux/leds-ir.h>
#endif
#ifdef CONFIG_LEDS_IR_PWM
#include <linux/leds-ir-pwm.h>
#endif
#ifdef CONFIG_MFD_MAX77693
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/leds-max77693.h>
#endif

#ifdef CONFIG_DC_MOTOR
#include <linux/dc_motor.h>
#endif

#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
#include <linux/battery/max17047_fuelgauge.h>
#endif
#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>
#endif
#if defined(CONFIG_CHARGER_MAX8922_U1)
#include <linux/power/max8922_charger_u1.h>
#endif
#ifdef CONFIG_STMPE811_ADC
#include <linux/stmpe811-adc.h>
#endif
#include <linux/v4l2-mediabus.h>
#include <linux/memblock.h>
#include <linux/delay.h>
#include <linux/bootmem.h>

#if defined(CONFIG_SF2_NFC_TAG)
#include <linux/nfc/nfc_tag.h>
#endif
#ifdef CONFIG_DMA_CMA
#include <linux/dma-contiguous.h>
#endif

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/setup.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/keypad.h>
#include <plat/devs.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#include <plat/s3c64xx-spi.h>
#include <plat/tvout.h>
#include <plat/csis.h>
#include <plat/media.h>
#include <plat/adc.h>
#include <media/exynos_fimc_is.h>
#include <mach/exynos-ion.h>
#include <mach/regs-gpio.h>

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
#include <mach/tdmb_pdata.h>
#elif defined(CONFIG_ISDBT)
#include <media/isdbt_pdata.h>
#endif

#include <mach/map.h>
#include <mach/spi-clocks.h>

#include <mach/dev.h>
#include <mach/ppmu.h>

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#ifdef CONFIG_EXYNOS_C2C
#include <mach/c2c.h>
#endif
#ifdef CONFIG_SEC_MODEM
#include <linux/platform_data/modem.h>
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif

#include <plat/fb-s5p.h>

#ifdef CONFIG_FB_S5P_EXTDSP
struct s3cfb_extdsp_lcd {
	int	width;
	int	height;
	int	bpp;
};
#endif
#include <mach/dev-sysmmu.h>

#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#include <plat/fimg2d.h>
#include <plat/s5p-sysmmu.h>

#include <mach/sec_debug.h>

#include <mach/gpio-midas.h>
#if defined(CONFIG_MACH_GC1)
#include <mach/gc1-power.h>
#elif defined(CONFIG_MACH_T0)
#include <mach/t0-power.h>
#elif defined(CONFIG_MACH_GD2)
#include <mach/gd2-power.h>
#elif defined(CONFIG_MACH_ZEST)
#include <mach/zest-power.h>
#elif defined(CONFIG_MACH_WATCH)
#include <mach/watch-power.h>
#elif defined(CONFIG_MACH_GC2PD)
#include <mach/gc2pd-power.h>
#else
#include <mach/midas-power.h>
#endif
#ifdef CONFIG_SEC_THERMISTOR
#include <mach/sec_thermistor.h>
#endif
#include <mach/midas-thermistor.h>
#include <mach/midas-tsp.h>
#include <mach/regs-clock.h>

#include <mach/board-lcd.h>
#include <mach/midas-sound.h>

#ifdef CONFIG_INPUT_WACOM
#include <mach/midas-wacom.h>
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
#include <linux/i2c/touchkey_i2c.h>
#endif
#ifdef CONFIG_KEYBOARD_TC370L_TOUCHKEY
#include <linux/i2c/tc370-touchkey.h>
#endif

#if defined(CONFIG_MACH_GC1)
#include <mach/gc1-jack.h>
#endif

#ifdef CONFIG_KEYBOARD_ADP5587
#include <linux/i2c/adp5587.h>
#endif

#include <mach/board-bluetooth-bcm.h>
#include "board-mobile.h"

#include "board-exynos4212.h"

bool is_cable_attached;

#ifdef CONFIG_ICE4_FPGA
extern void barcode_fpga_firmware_update(void);
#endif

#if defined(CONFIG_MACH_WATCH)
void __init exynos_watch_charger_init(void);
#endif

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4212_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDK4212_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK4212_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

#define SMDK4212_UFCON_GPS	(S3C2410_UFCON_FIFOMODE |	\
				S5PV210_UFCON_TXTRIG8 |	\
				S5PV210_UFCON_RXTRIG32)

static struct s3c2410_uartcfg smdk4212_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
		.wake_peer	= bcm_bt_lpm_exit_lpm_locked,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_GPS,
		.set_runstate	= set_gps_uart_op,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
	},
};

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI)

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x00,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "s5c73m3_spi",
		.platform_data = NULL,
		.max_speed_hz = 50000000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#if defined(CONFIG_VIDEO_DRIME4_SPI)

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x00,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "drime4_spi",
		.platform_data = NULL,
		.max_speed_hz = 12000000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#if defined(CONFIG_VIDEO_M9MO_SPI)

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x00,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "m9mo_spi",
		.platform_data = NULL,
		.max_speed_hz = 10000000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_1,
		.controller_data = &spi1_csi[0],
	}
};
#endif


#if defined(CONFIG_LINK_DEVICE_SPI)  \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) \
	|| defined(CONFIG_ISDBT) || defined(CONFIG_LINK_DEVICE_PLD)
static struct s3c64xx_spi_csinfo spi2_csi[] = {
	[0] = {
		.line = EXYNOS4_GPC1(2),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi2_board_info[] __initdata = {
#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	{
		.modalias = "tdmbspi",
		.platform_data = NULL,
		.max_speed_hz = 5000000,
		.bus_num = 2,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi2_csi[0],
	},
#elif defined(CONFIG_ISDBT)
	{
		.modalias = "fc8150_spi",
		.platform_data = NULL,
		.max_speed_hz = 5000000,
		.bus_num = 2,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi2_csi[0],
	},
#else
	{
		.modalias = "modem_if_spi",
		.platform_data = NULL,
		.bus_num = 2,
		.chip_select = 0,
		.max_speed_hz = 12*1000*1000,
		.mode = SPI_MODE_1,
		.controller_data = &spi2_csi[0],
	}
#endif
};
#endif
#endif

static struct i2c_board_info i2c_devs8_emul[];

#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
static void touchkey_init_hw(void)
{
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1)
#if defined(CONFIG_MACH_M0_CHNOPEN) || defined(CONFIG_MACH_M0_HKTW) || \
	defined(CONFIG_TARGET_LOCALE_KOR)
	/* do nothing */
#elif defined(CONFIG_MACH_C1)
#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT)
	if (system_rev < 8)
		return;
#elif defined(CONFIG_MACH_C1_KOR_LGT)
	if (system_rev < 5)
		return;
#else
	if (system_rev < 7)
		return;
#endif
#else
	if (system_rev < 11)
		return; /* rev 1.0 */
#endif
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) || \
	defined(CONFIG_MACH_M0) || \
	defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_SUPERIOR_KOR_SKT) || \
	defined(CONFIG_MACH_GD2) || \
	defined(CONFIG_MACH_ZEST) || \
	defined(CONFIG_MACH_WATCH)

#if defined(CONFIG_MACH_M3_JPN_DCM)
	if (system_rev < 3)
		gpio_request(GPIO_3_TOUCH_EN_R1, "gpio_3_touch_en");
	else
		gpio_request(GPIO_3_TOUCH_EN, "gpio_3_touch_en");
#else
	gpio_request(GPIO_3_TOUCH_EN, "gpio_3_touch_en");
#endif
#if defined(CONFIG_MACH_C1_KOR_LGT)
	gpio_request(GPIO_3_TOUCH_LDO_EN, "gpio_3_touch_ldo_en");
#endif
#endif

	gpio_request(GPIO_3_TOUCH_INT, "3_TOUCH_INT");
	s3c_gpio_setpull(GPIO_3_TOUCH_INT, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(GPIO_3_TOUCH_INT);
	gpio_direction_input(GPIO_3_TOUCH_INT);

	i2c_devs8_emul[0].irq = gpio_to_irq(GPIO_3_TOUCH_INT);
	irq_set_irq_type(gpio_to_irq(GPIO_3_TOUCH_INT), IRQF_TRIGGER_FALLING);
	s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));

	s3c_gpio_setpull(GPIO_3_TOUCH_SCL, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_3_TOUCH_SDA, S3C_GPIO_PULL_DOWN);
}

static int touchkey_suspend(void)
{
	struct regulator *regulator;
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	struct regulator *regulator_ldo17;
#endif

	regulator = regulator_get(NULL, TK_REGULATOR_NAME);
	if (IS_ERR(regulator))
		return 0;
	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	regulator_ldo17 = regulator_get(NULL, TK_VDD_REGULATOR);
	if (IS_ERR(regulator_ldo17))
		return 0;
	if (regulator_is_enabled(regulator_ldo17))
		regulator_force_disable(regulator_ldo17);
#endif

#if defined(CONFIG_MACH_C1_KOR_LGT)
	gpio_request(GPIO_3_TOUCH_LDO_EN, "gpio_3_touch_ldo_en");
	gpio_direction_output(GPIO_3_TOUCH_LDO_EN, 0);
#endif

	s3c_gpio_setpull(GPIO_3_TOUCH_SCL, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_3_TOUCH_SDA, S3C_GPIO_PULL_DOWN);

	regulator_put(regulator);
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	regulator_put(regulator_ldo17);
#endif

	return 1;
}

static int touchkey_resume(void)
{
	struct regulator *regulator;
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	struct regulator *regulator_ldo17;
#endif

#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	regulator_ldo17 = regulator_get(NULL, TK_VDD_REGULATOR);
	if (IS_ERR(regulator_ldo17))
		return 0;
	regulator_enable(regulator_ldo17);
#endif
	regulator = regulator_get(NULL, TK_REGULATOR_NAME);
	if (IS_ERR(regulator))
		return 0;
	regulator_enable(regulator);
	#if defined(CONFIG_MACH_C1_KOR_LGT)
	gpio_request(GPIO_3_TOUCH_LDO_EN, "gpio_3_touch_ldo_en");
	gpio_direction_output(GPIO_3_TOUCH_LDO_EN, 1);
	#endif
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	regulator_put(regulator_ldo17);
#endif
	regulator_put(regulator);

	s3c_gpio_setpull(GPIO_3_TOUCH_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_3_TOUCH_SDA, S3C_GPIO_PULL_NONE);

	return 1;
}

static int touchkey_power_on(bool on)
{
	int ret;

	if (on) {
		gpio_direction_output(GPIO_3_TOUCH_INT, 1);

		ret = touchkey_resume();

		irq_set_irq_type(gpio_to_irq(GPIO_3_TOUCH_INT),
			IRQF_TRIGGER_FALLING);
		s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_3_TOUCH_INT, S3C_GPIO_PULL_NONE);
	} else {
		gpio_direction_input(GPIO_3_TOUCH_INT);
		ret = touchkey_suspend();
	}

	return ret;
}

static int touchkey_led_power_on(bool on)
{
#if defined(LED_LDO_WITH_EN_PIN)
#if defined(CONFIG_MACH_M3_JPN_DCM)
	if (system_rev < 3) {
		if (on)
			gpio_direction_output(GPIO_3_TOUCH_EN_R1, 1);
		else
			gpio_direction_output(GPIO_3_TOUCH_EN_R1, 0);
	} else {
		if (on)
			gpio_direction_output(GPIO_3_TOUCH_EN, 1);
		else
			gpio_direction_output(GPIO_3_TOUCH_EN, 0);
	}
#else
	if (on)
		gpio_direction_output(GPIO_3_TOUCH_EN, 1);
	else
		gpio_direction_output(GPIO_3_TOUCH_EN, 0);
#endif
#else
	struct regulator *regulator;

	if (on) {
		regulator = regulator_get(NULL, "touch_led");
		if (IS_ERR(regulator))
			return 0;
		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "touch_led");
		if (IS_ERR(regulator))
			return 0;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}
#endif
	return 1;
}

static struct touchkey_platform_data touchkey_pdata = {
	.gpio_sda = GPIO_3_TOUCH_SDA,
	.gpio_scl = GPIO_3_TOUCH_SCL,
	.gpio_int = GPIO_3_TOUCH_INT,
	.init_platform_hw = touchkey_init_hw,
	.suspend = touchkey_suspend,
	.resume = touchkey_resume,
	.power_on = touchkey_power_on,
	.led_power_on = touchkey_led_power_on,
};
#endif /*CONFIG_KEYBOARD_CYPRESS_TOUCH*/

#ifdef CONFIG_KEYBOARD_TC370L_TOUCHKEY
int tc370_keycodes[] = {KEY_MENU, KEY_BACK};

static struct regulator *tc370_vdd_regulator = NULL;
static struct regulator *tc370_vled_regulator = NULL;

static int tc370_setup_power(struct device *dev, bool setup)
{
	return 0;
}

static void tc370_power(bool on)
{
	if (on)
		s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));
	else
		gpio_direction_input(GPIO_3_TOUCH_INT);

	gpio_direction_output(GPIO_3_TOUCH_EN, on ? 1 : 0);

	printk(KERN_INFO "%s: %s\n", __func__, (on) ? "on" : "off");
}

static void tc370_led_power(bool on)
{
	int ret = 0;

#if 0
if (!tc370_vled_regulator) {
		printk(KERN_ERR "%s: No regulator.\n", __func__);
		return;
	}

	if (on)
		ret = regulator_enable(tc370_vled_regulator);
	else
		ret = regulator_disable(tc370_vled_regulator);
#endif

	printk(KERN_INFO "%s: %s (%d)\n", __func__, (on) ? "on" : "off", ret);
}

static void tc370_pin_configure(bool to_gpios)
{
	if (to_gpios) {
		gpio_direction_output(GPIO_3_TOUCH_SCL, 1);
		gpio_direction_output(GPIO_3_TOUCH_SDA, 1);
	} else {
		s3c_gpio_setpull(GPIO_3_TOUCH_SCL, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpull(GPIO_3_TOUCH_SDA, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));
	}

#if 0
	/* the below routine is commented out.
	 * because the 'golden' use s/w i2c for tc370 touchkey.
	 */
	if (to_gpios) {
		/*
		nmk_gpio_set_mode(TOUCHKEY_SCL_GOLDEN_BRINGUP,
					NMK_GPIO_ALT_GPIO);
		*/
		gpio_direction_output(TOUCHKEY_SCL_GOLDEN_BRINGUP, 1);
		/*
		nmk_gpio_set_mode(TOUCHKEY_SDA_GOLDEN_BRINGUP,
					NMK_GPIO_ALT_GPIO);
		*/
		gpio_direction_output(TOUCHKEY_SDA_GOLDEN_BRINGUP, 1);


	} else {

		gpio_direction_output(TOUCHKEY_SCL_GOLDEN_BRINGUP, 1);
		/*
		nmk_gpio_set_mode(TOUCHKEY_SCL_GOLDEN_BRINGUP, NMK_GPIO_ALT_C);
		*/
		gpio_direction_output(TOUCHKEY_SDA_GOLDEN_BRINGUP, 1);
		/*
		nmk_gpio_set_mode(TOUCHKEY_SDA_GOLDEN_BRINGUP, NMK_GPIO_ALT_C);
		*/
	}
#endif

}

static void tc370_int_set_pull(bool to_up)
{
	#if 0
int ret;
	int pull = (to_up) ? NMK_GPIO_PULL_UP : NMK_GPIO_PULL_DOWN;

	ret = nmk_gpio_set_pull(TOUCHKEY_INT_GOLDEN_BRINGUP, pull);
	if (ret < 0)
		printk(KERN_ERR "%s: fail to set pull-%s on interrupt pin\n",
		       __func__,
		       (pull == NMK_GPIO_PULL_UP) ? "up" : "down");
#endif
}

struct tc370_platform_data touchkey_pdata = {
	.gpio_scl = GPIO_3_TOUCH_SCL,
	.gpio_sda = GPIO_3_TOUCH_SDA,
	.gpio_int = GPIO_3_TOUCH_INT,
	.gpio_en = GPIO_3_TOUCH_EN,
	/*.udelay = 2,*/
	.num_key = ARRAY_SIZE(tc370_keycodes),
	.keycodes = tc370_keycodes,
	.suspend_type = TC370_SUSPEND_WITH_POWER_OFF,
	.setup_power = tc370_setup_power,
	.power = tc370_power,
	.led_power = tc370_led_power,
	.pin_configure = tc370_pin_configure,
	.int_set_pull = tc370_int_set_pull,
};

static int tc370_init(void)
{
	int ret;
	unsigned pin;

	touchkey_pdata.enable = 1;

	pin = GPIO_3_TOUCH_EN;
	ret = gpio_request(GPIO_3_TOUCH_EN, "gpio_3_touch_en");
	if (ret < 0) {
		printk(KERN_ERR "%s: fail to request_gpio of %d (%d).\n",
			__func__, pin, ret);
		return ret;
	}

	pin = GPIO_3_TOUCH_INT;
	ret = gpio_request(pin, "3_TOUCH_INT");
	if (ret < 0) {
		printk(KERN_ERR "%s: fail to request_gpio of %d (%d).\n",
			__func__, pin, ret);
		return ret;
	}
	gpio_direction_input(pin);

	i2c_devs8_emul[0].irq = gpio_to_irq(GPIO_3_TOUCH_INT);
	s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));

	s3c_gpio_setpull(GPIO_3_TOUCH_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_3_TOUCH_SDA, S3C_GPIO_PULL_NONE);

	return 0;
}
#endif /*CONFIG_KEYBOARD_TC370L_TOUCHKEY*/

#ifdef CONFIG_KEYBOARD_ADP5587
static struct i2c_board_info i2c_devs25_emul[];

static const unsigned short qwerty_keymap[ADP5587_KEYMAPSIZE] = {
	[4] = KEY_SEARCH,
	[6] = KEY_F17, /* message */
	[7] = KEY_EMAIL,
	[8] = KEY_QUESTION,
	[10] = KEY_RIGHT,
	[11] = KEY_DOWN,
	[12] = KEY_LEFT,
	[13] = KEY_DOT,
	[14] = KEY_SPACE,
	[15] = KEY_SPACE,
	[16] = KEY_COMMA,
	[17] = KEY_RIGHTSHIFT,
	[18] = KEY_F18, /* voice search */
	[19] = KEY_LEFTALT,
	[40] = KEY_ENTER,
	[41] = KEY_UP,
	[42] = KEY_M,
	[43] = KEY_N,
	[44] = KEY_B,
	[45] = KEY_V,
	[46] = KEY_C,
	[47] = KEY_X,
	[48] = KEY_Z,
	[49] = KEY_LEFTSHIFT,
	[50] = KEY_BACKSPACE,
	[51] = KEY_L,
	[52] = KEY_K,
	[53] = KEY_J,
	[54] = KEY_H,
	[55] = KEY_G,
	[56] = KEY_F,
	[57] = KEY_D,
	[58] = KEY_S,
	[59] = KEY_A,
	[60] = KEY_P,
	[61] = KEY_O,
	[62] = KEY_I,
	[63] = KEY_U,
	[64] = KEY_Y,
	[65] = KEY_T,
	[66] = KEY_R,
	[67] = KEY_E,
	[68] = KEY_W,
	[69] = KEY_Q,
	[70] = KEY_0,
	[71] = KEY_9,
	[72] = KEY_8,
	[73] = KEY_7,
	[74] = KEY_6,
	[75] = KEY_5,
	[76] = KEY_4,
	[77] = KEY_3,
	[78] = KEY_2,
	[79] = KEY_1,
};

static const unsigned short qwerty_keymap_rev01[ADP5587_KEYMAPSIZE] = {
	[6] = KEY_F23,
	[7] = KEY_SYM,
	[8] = KEY_QUESTION,
	[10] = KEY_RIGHT,
	[11] = KEY_DOWN,
	[12] = KEY_LEFT,
	[13] = KEY_DOT,
	[14] = KEY_SPACE,
	[15] = KEY_SPACE,
	[16] = KEY_F24,
	[17] = KEY_CENTER,
	[18] = KEY_COMMA,
	[19] = KEY_LEFTALT,
	[40] = KEY_ENTER,
	[41] = KEY_UP,
	[42] = KEY_M,
	[43] = KEY_N,
	[44] = KEY_B,
	[45] = KEY_V,
	[46] = KEY_C,
	[47] = KEY_X,
	[48] = KEY_Z,
	[49] = KEY_LEFTSHIFT,
	[50] = KEY_BACKSPACE,
	[51] = KEY_L,
	[52] = KEY_K,
	[53] = KEY_J,
	[54] = KEY_H,
	[55] = KEY_G,
	[56] = KEY_F,
	[57] = KEY_D,
	[58] = KEY_S,
	[59] = KEY_A,
	[60] = KEY_P,
	[61] = KEY_O,
	[62] = KEY_I,
	[63] = KEY_U,
	[64] = KEY_Y,
	[65] = KEY_T,
	[66] = KEY_R,
	[67] = KEY_E,
	[68] = KEY_W,
	[69] = KEY_Q,
	[70] = KEY_0,
	[71] = KEY_9,
	[72] = KEY_8,
	[73] = KEY_7,
	[74] = KEY_6,
	[75] = KEY_5,
	[76] = KEY_4,
	[77] = KEY_3,
	[78] = KEY_2,
	[79] = KEY_1,
};

static struct adp5587_kpad_platform_data adp5587_pdata = {
	.rows		= 8,
	.cols		= 10,
	.keymap		= qwerty_keymap,
	.keymapsize	= ARRAY_SIZE(qwerty_keymap),
	.repeat		= 0,
	.hall_gpio	= GPIO_HALL_SW,
	.led_gpio	= GPIO_QTKEYLED_EN,
};

static void qwertykey_init(void)
{
	if (system_rev >= 1) {
		adp5587_pdata.keymap = qwerty_keymap_rev01;
		adp5587_pdata.keymapsize = ARRAY_SIZE(qwerty_keymap_rev01);
	}

	if (gpio_request(GPIO_QT_GPIO_INT, "QT_GPIO_INT"))
		pr_err("failed to request gpio(GPIO_QT_GPIO_INT)\n");
	s3c_gpio_setpull(GPIO_QT_GPIO_INT, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(GPIO_QT_GPIO_INT);
	gpio_direction_input(GPIO_QT_GPIO_INT);

	i2c_devs25_emul[0].irq = gpio_to_irq(GPIO_QT_GPIO_INT);

	if (gpio_request(GPIO_HALL_SW, "HALL_SW"))
		pr_err("failed to request gpio(GPIO_HALL_SW)\n");
	s3c_gpio_setpull(GPIO_HALL_SW, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(GPIO_HALL_SW);
	gpio_direction_input(GPIO_HALL_SW);

	if (gpio_request(GPIO_QTKEYLED_EN, "QTKEYLED_EN"))
		pr_err("failed to request gpio(GPIO_QTKEYLED_EN)\n");
	gpio_direction_output(GPIO_QTKEYLED_EN, 0);
}

#endif

#ifdef CONFIG_LEDS_GPIO
struct gpio_led gpio_leds[5] = {
	{
		.name		= "led_r",
		.gpio		= GPIO_LED1_EN,
		.default_state	= LEDS_GPIO_DEFSTATE_ON,
	},
	{
		.name		= "led_g",
		.gpio		= GPIO_LED2_EN,
                .default_state  = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name		= "led_b",
		.gpio		= GPIO_LED3_EN,
		.default_state  = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name		= "debug_led_white",
		.gpio		= GPIO_SW_LED1_EN,
		.default_state  = LEDS_GPIO_DEFSTATE_ON,
	},
	{
		.name		= "debug_led_blue",
		.gpio		= GPIO_SW_LED2_EN,
		.default_state  = LEDS_GPIO_DEFSTATE_ON,
	},
};

struct gpio_led_platform_data leds_gpio_pdata = {
	.num_leds	= 5,
	.leds		= gpio_leds,
};
struct platform_device leds_gpio_device = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &leds_gpio_pdata,
	},
};
#endif

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
static void tdmb_set_config_poweron(void)
{
	s3c_gpio_cfgpin(GPIO_TDMB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);
#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_BAFFIN)
	s3c_gpio_cfgpin(GPIO_TDMB_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_RST_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_RST_N, GPIO_LEVEL_LOW);
#endif
	s3c_gpio_cfgpin(GPIO_TDMB_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TDMB_INT, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TDMB_SPI_CLK, S3C_GPIO_SFN(5));
	s3c_gpio_cfgpin(GPIO_TDMB_SPI_MISO, S3C_GPIO_SFN(5));
	s3c_gpio_cfgpin(GPIO_TDMB_SPI_MOSI, S3C_GPIO_SFN(5));
	s3c_gpio_setpull(GPIO_TDMB_SPI_CLK, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_TDMB_SPI_MISO, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_TDMB_SPI_MOSI, S3C_GPIO_PULL_DOWN);
}
static void tdmb_set_config_poweroff(void)
{
	s3c_gpio_cfgpin(GPIO_TDMB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);

#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_BAFFIN)
	s3c_gpio_cfgpin(GPIO_TDMB_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_RST_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_RST_N, GPIO_LEVEL_LOW);
#endif

	s3c_gpio_cfgpin(GPIO_TDMB_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_INT, GPIO_LEVEL_LOW);
}

static void tdmb_gpio_on(void)
{
	printk(KERN_DEBUG "tdmb_gpio_on\n");

	tdmb_set_config_poweron();

	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);
	usleep_range(1000, 1000);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_HIGH);

#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_BAFFIN)
	usleep_range(1000, 1000);
	gpio_set_value(GPIO_TDMB_RST_N, GPIO_LEVEL_LOW);
	usleep_range(2000, 2000);
	gpio_set_value(GPIO_TDMB_RST_N, GPIO_LEVEL_HIGH);
	usleep_range(1000, 1000);
#endif

}

static void tdmb_gpio_off(void)
{
	printk(KERN_DEBUG "tdmb_gpio_off\n");

	tdmb_set_config_poweroff();
#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_BAFFIN)
	gpio_set_value(GPIO_TDMB_RST_N, GPIO_LEVEL_LOW);
	usleep_range(1000, 1000);
#endif

	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);
}

#if defined(CONFIG_TDMB_ANT_DET)
static void tdmb_ant_enable(bool en)
{
	unsigned int tdmb_ant_det_gpio;

	printk(KERN_DEBUG "tdmb_ant_enable : %d\n", en);

	if (system_rev >= 6)
		tdmb_ant_det_gpio = GPIO_TDMB_ANT_DET_REV08;
	else
		tdmb_ant_det_gpio = GPIO_TDMB_ANT_DET;

	if (en) {
		s3c_gpio_cfgpin(tdmb_ant_det_gpio, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(tdmb_ant_det_gpio, S3C_GPIO_PULL_NONE);
	} else {
		s3c_gpio_cfgpin(tdmb_ant_det_gpio, S3C_GPIO_INPUT);
		s3c_gpio_setpull(tdmb_ant_det_gpio, S3C_GPIO_PULL_NONE);
	}
}
#endif

static struct tdmb_platform_data tdmb_pdata = {
	.gpio_on = tdmb_gpio_on,
	.gpio_off = tdmb_gpio_off,
#if defined(CONFIG_TDMB_ANT_DET)
	.tdmb_ant_det_en = tdmb_ant_enable,
#endif
};

static struct platform_device tdmb_device = {
	.name			= "tdmb",
	.id				= -1,
	.dev			= {
		.platform_data = &tdmb_pdata,
	},
};

#if defined(CONFIG_MACH_BAFFIN_KOR_LGT) || defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
#define TDMB_VDD_REGULATOR "tdmb_1.8v"
#endif
static int __init tdmb_dev_init(void)
{
#if defined(CONFIG_TDMB_ANT_DET)
	unsigned int tdmb_ant_det_gpio;
	unsigned int tdmb_ant_det_irq;
	if (system_rev >= 6) {
		tdmb_ant_det_gpio = GPIO_TDMB_ANT_DET_REV08;
		tdmb_ant_det_irq = GPIO_TDMB_IRQ_ANT_DET_REV08;
	} else {
		s5p_register_gpio_interrupt(GPIO_TDMB_ANT_DET);
		tdmb_ant_det_gpio = GPIO_TDMB_ANT_DET;
		tdmb_ant_det_irq = GPIO_TDMB_IRQ_ANT_DET;
	}
	s3c_gpio_cfgpin(tdmb_ant_det_gpio, S3C_GPIO_INPUT);
	s3c_gpio_setpull(tdmb_ant_det_gpio, S3C_GPIO_PULL_NONE);
	tdmb_pdata.gpio_ant_det = tdmb_ant_det_gpio;
	tdmb_pdata.irq_ant_det = tdmb_ant_det_irq;
#endif
	tdmb_set_config_poweroff();

	s5p_register_gpio_interrupt(GPIO_TDMB_INT);
	tdmb_pdata.irq = GPIO_TDMB_IRQ;
	platform_device_register(&tdmb_device);

#if defined(CONFIG_MACH_BAFFIN_KOR_LGT) || defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	if (system_rev >= 2) { /* 0010 */
#else /* baffin */
	if (system_rev >= 4) { /* 0100 */
#endif
		struct regulator *regulator_ldo13;
		printk(KERN_INFO "[TDMB_PW] PMIC LDO13 Enable\n");
		regulator_ldo13 = regulator_get(NULL, TDMB_VDD_REGULATOR);
		if (IS_ERR(regulator_ldo13))
			return 0;
		regulator_enable(regulator_ldo13);
	}
#endif
	return 0;
}
#elif defined(CONFIG_ISDBT)
static void isdbt_set_config_poweron(void)
{
	s3c_gpio_cfgpin(GPIO_ISDBT_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ISDBT_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_ISDBT_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ISDBT_RST_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_ISDBT_INT, S3C_GPIO_SFN(GPIO_ISDBT_INT_AF));
	s3c_gpio_setpull(GPIO_ISDBT_INT, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_ISDBT_SPI_CLK, S3C_GPIO_SFN(5));
	s3c_gpio_cfgpin(GPIO_ISDBT_SPI_MISO, S3C_GPIO_SFN(5));
	s3c_gpio_cfgpin(GPIO_ISDBT_SPI_MOSI, S3C_GPIO_SFN(5));
	s3c_gpio_setpull(GPIO_ISDBT_SPI_CLK, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_ISDBT_SPI_MISO, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_ISDBT_SPI_MOSI, S3C_GPIO_PULL_DOWN);

	printk(KERN_DEBUG "isdbt_set_config_poweron\n");

}
static void isdbt_set_config_poweroff(void)
{
	s3c_gpio_cfgpin(GPIO_ISDBT_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ISDBT_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_ISDBT_RST_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ISDBT_RST_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_ISDBT_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ISDBT_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_ISDBT_INT, GPIO_LEVEL_LOW);


	printk(KERN_DEBUG "isdbt_set_config_poweroff\n");
}

static void isdbt_gpio_on(void)
{
	printk(KERN_DEBUG "isdbt_gpio_on\n");

	isdbt_set_config_poweron();

	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);
	usleep_range(1000, 1000);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_HIGH);

	usleep_range(1000, 1000);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);
	usleep_range(2000, 2000);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_HIGH);
	usleep_range(1000, 1000);

}

static void isdbt_gpio_off(void)
{
	printk(KERN_DEBUG "isdbt_gpio_off\n");

	isdbt_set_config_poweroff();

	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);
	usleep_range(1000, 1000);

	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);
}

static struct isdbt_platform_data isdbt_pdata = {
	.gpio_on = isdbt_gpio_on,
	.gpio_off = isdbt_gpio_off,
};

static struct platform_device isdbt_device = {
	.name			= "isdbt",
	.id				= -1,
	.dev			= {
		.platform_data = &isdbt_pdata,
	},
};

static int __init isdbt_dev_init(void)
{
#if defined(CONFIG_MACH_T0_JPN_LTE_DCM) && defined(CONFIG_ISDBT_ANT_DET)
	unsigned int isdbt_ant_det_gpio;
	unsigned int isdbt_ant_det_irq;

	if (system_rev > 11) {
		isdbt_ant_det_gpio = GPIO_ISDBT_ANT_DET_REV08;
		isdbt_ant_det_irq = GPIO_ISDBT_IRQ_ANT_DET_REV08;
	} else {
		s5p_register_gpio_interrupt(GPIO_ISDBT_ANT_DET);
		isdbt_ant_det_gpio = GPIO_ISDBT_ANT_DET;
		isdbt_ant_det_irq = GPIO_ISDBT_IRQ_ANT_DET;
	}

	s3c_gpio_cfgpin(isdbt_ant_det_gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(isdbt_ant_det_gpio, S3C_GPIO_PULL_NONE);
	isdbt_pdata.gpio_ant_det = isdbt_ant_det_gpio;
	isdbt_pdata.irq_ant_det = isdbt_ant_det_irq;
#endif
	isdbt_set_config_poweroff();
	s5p_register_gpio_interrupt(GPIO_ISDBT_INT);
	isdbt_pdata.irq = GPIO_ISDBT_IRQ;
	platform_device_register(&isdbt_device);

	printk(KERN_DEBUG "isdbt_dev_init\n");

	return 0;
}
#endif

#ifdef CONFIG_LEDS_AAT1290A
static int aat1290a_initGpio(void)
{
	int err;

	err = gpio_request(GPIO_CAM_SW_EN, "CAM_SW_EN");
	if (err) {
		printk(KERN_ERR "failed to request CAM_SW_EN\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_CAM_SW_EN, 1);

	return 0;
}

static void aat1290a_switch(int enable)
{
	gpio_set_value(GPIO_CAM_SW_EN, enable);
}

static int aat1290a_setGpio(void)
{
	int err;

	err = gpio_request(GPIO_TORCH_EN, "TORCH_EN");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_EN\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_TORCH_EN, 0);
	err = gpio_request(GPIO_TORCH_SET, "TORCH_SET");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_SET\n");
		gpio_free(GPIO_TORCH_EN);
		return -EPERM;
	}
	gpio_direction_output(GPIO_TORCH_SET, 0);

	return 0;
}

static int aat1290a_freeGpio(void)
{
	gpio_free(GPIO_TORCH_EN);
	gpio_free(GPIO_TORCH_SET);

	return 0;
}

static void aat1290a_torch_en(int onoff)
{
	gpio_set_value(GPIO_TORCH_EN, onoff);
}

static void aat1290a_torch_set(int onoff)
{
	gpio_set_value(GPIO_TORCH_SET, onoff);
}

static struct aat1290a_led_platform_data aat1290a_led_data = {
	.brightness = TORCH_BRIGHTNESS_50,
	.status	= STATUS_UNAVAILABLE,
	.switch_sel = aat1290a_switch,
	.initGpio = aat1290a_initGpio,
	.setGpio = aat1290a_setGpio,
	.freeGpio = aat1290a_freeGpio,
	.torch_en = aat1290a_torch_en,
	.torch_set = aat1290a_torch_set,
};

static struct platform_device s3c_device_aat1290a_led = {
	.name	= "aat1290a-led",
	.id	= -1,
	.dev	= {
		.platform_data	= &aat1290a_led_data,
	},
};
#endif

#ifdef CONFIG_LEDS_IR
static int ir_leds_initGpio(void)
{
	int err;

	err = gpio_request(GPIO_IR_LED_ON, "IR_LED_ON");
	if (err) {
		printk(KERN_ERR "failed to request IR_LED_ON\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_IR_LED_ON, 0);

	return 0;
}

static void ir_leds_setGpio(int onoff)
{
	int err = 0;
	//printk(KERN_ERR "ir_led_setGpio = %d\n",onoff);
	err = gpio_direction_output(GPIO_IR_LED_ON, onoff);
	if (err ) {
		printk(KERN_ERR "failed to set gpio.\n");
	}
}

static int ir_leds_freeGpio(void)
{
	gpio_free(GPIO_IR_LED_ON);
	return 0;
}

static struct ir_led_platform_data ir_led_data = {
	.Period = 30,
	.onTime = 0,
	.status	= STATUS_UNAVAILABLE,
	.initGpio = ir_leds_initGpio,
	.setGpio = ir_leds_setGpio,
	.freeGpio = ir_leds_freeGpio,
};

static struct platform_device s3c_device_ir_led = {
	.name	= "ir-led",
	.id	= -1,
	.dev	= {
		.platform_data	= &ir_led_data,
	},
};
#endif
#ifdef CONFIG_LEDS_IR_PWM
static int ir_led_power(bool en)
{
	// add
	return 0;
}

static struct ir_led_pwm_platform_data ir_led_pwm_data = {
	.power_en = ir_led_power,
	.max_timeout = 317460320*2,
	.pwm_id = 1,
	.pwm_duty = 95238096,		// 6ms
	.pwm_period = 317460320,	// 20ms
};

static struct platform_device s3c_device_ir_led_pwm = {
	.name	= "ir-led-pwm",
	.id	= -1,
	.dev = {
		.platform_data = &ir_led_pwm_data,
	}
};
#endif

static DEFINE_MUTEX(notify_lock);

#define DEFINE_MMC_CARD_NOTIFIER(num) \
static void (*hsmmc##num##_notify_func)(struct platform_device *, int state); \
static int ext_cd_init_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func); \
	hsmmc##num##_notify_func = notify_func; \
	mutex_unlock(&notify_lock); \
	return 0; \
} \
static int ext_cd_cleanup_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func != notify_func); \
	hsmmc##num##_notify_func = NULL; \
	mutex_unlock(&notify_lock); \
	return 0; \
}

#ifdef CONFIG_S3C_DEV_HSMMC3
	DEFINE_MMC_CARD_NOTIFIER(3)
#endif

/*
 * call this when you need sd stack to recognize insertion or removal of card
 * that can't be told by SDHCI regs
 */
void mmc_force_presence_change(struct platform_device *pdev)
{
	void (*notify_func)(struct platform_device *, int state) = NULL;
	mutex_lock(&notify_lock);
#ifdef CONFIG_S3C_DEV_HSMMC3
	if (pdev == &s3c_device_hsmmc3)
		notify_func = hsmmc3_notify_func;
#endif

	if (notify_func)
		notify_func(pdev, 1);
	else
		pr_warn("%s: called for device with no notifier\n", __func__);
	mutex_unlock(&notify_lock);
}
EXPORT_SYMBOL_GPL(mmc_force_presence_change);

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdk4212_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdk4212_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk4212_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= EXYNOS4_GPX3(4),
	.ext_cd_gpio_invert	= true,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.vmmc_name		= "vtf_2.8v"
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdk4212_hsmmc3_pdata __initdata = {
/* new code for brm4334 */
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,

	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags = S3C_SDHCI_PM_IGNORE_SUSPEND_RESUME,
	.ext_cd_init = ext_cd_init_hsmmc3,
	.ext_cd_cleanup = ext_cd_cleanup_hsmmc3,
};
#endif

#ifdef CONFIG_EXYNOS4_DEV_MSHC
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.fifo_depth		= 0x80,
#if defined(CONFIG_EXYNOS4_MSHC_8BIT) && \
	defined(CONFIG_EXYNOS4_MSHC_DDR)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
				  MMC_CAP_UHS_DDR50 | MMC_CAP_CMD23,
#ifdef CONFIG_MMC_MSHCI_ENABLE_CACHE
	.host_caps2		= MMC_CAP2_ADAPT_PACKED | MMC_CAP2_PACKED_CMD |
				  MMC_CAP2_CACHE_CTRL | MMC_CAP2_POWEROFF_NOTIFY,
#else
	.host_caps2		= MMC_CAP2_ADAPT_PACKED | MMC_CAP2_PACKED_CMD |
				  MMC_CAP2_POWEROFF_NOTIFY,
#endif
#elif defined(CONFIG_EXYNOS4_MSHC_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
#elif defined(CONFIG_EXYNOS4_MSHC_DDR)
	.host_caps		= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50 |
				  MMC_CAP_CMD23,
#endif
	.int_power_gpio		= GPIO_eMMC_EN,
};
#endif

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk4212_ehci_pdata;

static void __init smdk4212_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4212_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata smdk4212_ohci_pdata;

static void __init smdk4212_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk4212_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
#endif

/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata smdk4212_usbgadget_pdata;

#include <linux/usb/android_composite.h>
static void __init smdk4212_usbgadget_init(void)
{
	struct s5p_usbgadget_platdata *pdata = &smdk4212_usbgadget_pdata;
	struct android_usb_platform_data *android_pdata =
		s3c_device_android_usb.dev.platform_data;
	if (android_pdata) {
		unsigned int cdfs = 0;
#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC) || \
	defined(CONFIG_MACH_M0_DUOSCTC)
		unsigned int newluns = 1;
		cdfs = 1;   /* China CTC required CDFS */
#elif defined(CONFIG_MACH_T0_USA_VZW)
		unsigned int newluns = 0;
		cdfs = 1;   /* VZW required CDFS */
#else
		unsigned int newluns = 2;
#endif
		printk(KERN_DEBUG "usb: %s: default luns=%d, new luns=%d\n",
				__func__, android_pdata->nluns, newluns);
		android_pdata->nluns = newluns;
		android_pdata->cdfs_support = cdfs;
	} else {
		printk(KERN_DEBUG "usb: %s android_pdata is not available\n",
				__func__);
	}

	s5p_usbgadget_set_platdata(pdata);

#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT) || \
	defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN) || \
	defined(CONFIG_MACH_GC1_KOR_SKT) || defined(CONFIG_MACH_GC1_KOR_KT) || \
	defined(CONFIG_MACH_GC1_KOR_LGT) || defined(CONFIG_MACH_GC1_USA_VZW) || \
	defined(CONFIG_MACH_GC2PD)


	pdata = s3c_device_usbgadget.dev.platform_data;
	if (pdata) {
		/* Squelch Threshold Tune [13:11] (111 : -20%) */
		pdata->phy_tune_mask |= (0x7 << 11);
		pdata->phy_tune |= (0x7 << 11);
		printk(KERN_DEBUG "usb: %s tune_mask=0x%x, tune=0x%x\n",
			__func__, pdata->phy_tune_mask, pdata->phy_tune);
	}
#elif defined(CONFIG_MACH_ZEST)
	pdata = s3c_device_usbgadget.dev.platform_data;
	if (pdata) {
		/* Squelch Threshold Tune [13:11] (100 : -5%) */
		pdata->phy_tune_mask |= (0x7 << 11);
		pdata->phy_tune |= (0x4 << 11);
		printk(KERN_DEBUG "usb: %s tune_mask=0x%x, tune=0x%x\n",
			__func__, pdata->phy_tune_mask, pdata->phy_tune);
	}
#endif

}
#endif

#ifdef CONFIG_DC_MOTOR
void __init dc_motor_init(void)
{
	pr_info("dc_motor_init() is called!");
}

void dc_motor_power(bool on)
{
	struct regulator *regulator;
	regulator = regulator_get(NULL, "vdd_mot_2.7v");

	pr_info("dc_motor_power %s\n", on ? "ON" : "OFF");

	if (on) {
		regulator_enable(regulator);
	} else {
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
		else
			regulator_force_disable(regulator);
	}
	regulator_put(regulator);
}

static struct dc_motor_platform_data dc_motor_pdata = {
	.power = dc_motor_power,
	.max_timeout = 10000,
};

static struct platform_device dc_motor_device = {
	.name = "sec-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &dc_motor_pdata,
	},
};
#endif


#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
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

#if !defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
#ifdef CONFIG_MFD_MAX77693
#ifdef CONFIG_VIBETONZ
static struct max77693_haptic_platform_data max77693_haptic_pdata = {
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
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
	.duty = 35500,
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

#ifdef CONFIG_LEDS_MAX77693
static struct max77693_led_platform_data max77693_led_pdata = {
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

extern struct max77693_muic_data max77693_muic;
extern struct max77693_regulator_data max77693_regulators;

#if defined(CONFIG_SAMSUNG_MUIC)
static bool is_muic_default_uart_path_cp(void)
{
#if defined(CONFIG_MACH_M0_CTC)
	return false;
#else
#ifdef CONFIG_MACH_M0
	if (system_rev == 5)
		return true;
#endif
#ifdef CONFIG_MACH_C1
	if (system_rev == 4)
		return true;
#endif
	return false;
#endif
}
#endif /* CONFIG_SAMSUNG_MUIC */

struct max77693_platform_data exynos4_max77693_info = {
	.irq_base	= IRQ_BOARD_IFIC_START,
	.irq_gpio	= GPIO_IF_PMIC_IRQ,
	.wakeup		= 1,
	.muic = &max77693_muic,
#if defined(CONFIG_SAMSUNG_MUIC)
	.is_default_uart_path_cp =  is_muic_default_uart_path_cp,
#endif /* CONFIG_SAMSUNG_MUIC */
	.regulators = &max77693_regulators,
	.num_regulators = MAX77693_REG_MAX,
#ifdef CONFIG_VIBETONZ
	.haptic_data = &max77693_haptic_pdata,
#endif
#ifdef CONFIG_LEDS_MAX77693
	.led_data = &max77693_led_pdata,
#endif
#ifdef CONFIG_BATTERY_MAX77693_CHARGER
	.charger_data = &max77693_charger_pdata,
#endif
};
#endif /* CONFIG_MFD_MAX77693 */
#endif /* !CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_CHARGER_MAX8922_U1)
static int max8922_cfg_gpio(void)
{
	printk(KERN_INFO "[Battery] %s called.\n", __func__);

	s3c_gpio_cfgpin(GPIO_CHG_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CHG_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CHG_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_CHG_ING_N, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CHG_ING_N, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);

	return 0;
}

static struct max8922_platform_data max8922_pdata = {
	.cfg_gpio = max8922_cfg_gpio,
	.gpio_chg_en = GPIO_CHG_EN,
	.gpio_chg_ing = GPIO_CHG_ING_N,
	.gpio_ta_nconnected = GPIO_TA_nCONNECTED,
};

static struct platform_device max8922_device_charger = {
	.name = "max8922-charger",
	.id = -1,
	.dev.platform_data = &max8922_pdata,
};
#endif /* CONFIG_CHARGER_MAX8922_U1 */

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
};

#if !defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
#ifdef CONFIG_MFD_MAX77693
#if defined(CONFIG_MUIC_I2C_USE_S3C_DEV_I2C4)
static struct i2c_board_info i2c_devs4_max77693[] __initdata = {
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data	= &exynos4_max77693_info,
	}
};
#endif /* CONFIG_MUIC_I2C_USE_S3C_DEV_I2C4 */
#endif
#endif /* !CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
static void s3c_i2c5_cfg_gpio_gc1(void)
{
	/* DDC_HDMI_SDA */
	s3c_gpio_cfgpin(EXYNOS4_GPB(2), S3C_GPIO_SFN(0x0));
	s3c_gpio_setpull(EXYNOS4_GPB(2), S3C_GPIO_PULL_NONE);

	/* _SCL */
	s3c_gpio_cfgpin(EXYNOS4_GPB(3), S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(EXYNOS4_GPB(3), S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgall_range(EXYNOS4_GPB(2), 2,
		S3C_GPIO_SFN(3), S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(EXYNOS4_GPB(2), S5P_GPIO_DRVSTR_LV4);
	s5p_gpio_set_drvstr(EXYNOS4_GPB(3), S5P_GPIO_DRVSTR_LV4);

	s3c_gpio_cfgpin(GPIO_HDMI_EN, S3C_GPIO_OUTPUT);	/* HDMI_EN */
	s3c_gpio_setpull(GPIO_HDMI_EN, S3C_GPIO_PULL_NONE);
}

static struct i2c_gpio_platform_data gpio_i2c_data5 = {
	.sda_pin = EXYNOS4_GPB(2),
	.scl_pin = EXYNOS4_GPB(3),
	.udelay = 25,
	.timeout = 0,
};

struct platform_device s3c_device_i2c5 = {
	.name = "i2c-gpio",
	.id = 5,
	.dev.platform_data = &gpio_i2c_data5,
};
#endif
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GD2) || defined(CONFIG_MACH_GC2PD)
static struct i2c_board_info i2c_devs5[] __initdata = {
	/* HDMI */
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
};

static void hdmi_ext_ic_control_gc1(bool ic_on)
{
	if (ic_on)
		gpio_set_value(GPIO_HDMI_EN, GPIO_LEVEL_HIGH);
	else
		gpio_set_value(GPIO_HDMI_EN, GPIO_LEVEL_LOW);
}
#endif
#ifdef CONFIG_MACH_GD2
static void s3c_i2c5_cfg_gpio_gd2(struct platform_device *dev)
{
	s3c_gpio_cfgall_range(EXYNOS4_GPB(2), 2,
		S3C_GPIO_SFN(3), S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(EXYNOS4_GPB(2), S5P_GPIO_DRVSTR_LV4);
	s5p_gpio_set_drvstr(EXYNOS4_GPB(3), S5P_GPIO_DRVSTR_LV4);
	s3c_gpio_cfgpin(GPIO_HDMI_EN, S3C_GPIO_OUTPUT);	/* HDMI_EN */
	s3c_gpio_setpull(GPIO_HDMI_EN, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_HDMI_CT_CP_HPD, S3C_GPIO_OUTPUT);	/* HDMI_CT_CP_HPD */
	s3c_gpio_setpull(GPIO_HDMI_CT_CP_HPD, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_HDMI_CT_CP_HPD, GPIO_LEVEL_HIGH);
}
#endif

#ifdef CONFIG_S3C_DEV_I2C5
#if !defined(CONFIG_MACH_T0_EUR_OPEN) && !defined(CONFIG_MACH_T0_CHN_OPEN) \
	&& !defined(CONFIG_MACH_GD2) && !defined(CONFIG_MACH_IPCAM) && !defined(CONFIG_MACH_GC2PD)
static struct i2c_board_info i2c_devs5[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8997
	{
		I2C_BOARD_INFO("max8997", (0xcc >> 1)),
		.platform_data = &exynos4_max8997_info,
	},
#endif

#if defined(CONFIG_REGULATOR_MAX77686)
	/* max77686 on i2c5 other than M1 board */
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	},
#endif
};
#endif
struct s3c2410_platform_i2c default_i2c5_data __initdata = {
	.bus_num        = 5,
	.flags          = 0,
	.slave_addr     = 0x10,
	.frequency      = 100*1000,
	.sda_delay      = 100,
#ifdef CONFIG_MACH_GD2
	.cfg_gpio	= s3c_i2c5_cfg_gpio_gd2,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_I2C6
static struct i2c_board_info i2c_devs6[] __initdata = {
};
#if defined(CONFIG_MACH_T0_EUR_OPEN) || defined(CONFIG_MACH_T0_CHN_OPEN) || \
	defined(CONFIG_MACH_GD2)
static void i2c6_mhl_ddc_cfg_gpio(struct platform_device *dev)
{
	s3c_gpio_cfgall_range(EXYNOS4_GPC1(3), 2,
		S3C_GPIO_SFN(4), S3C_GPIO_PULL_NONE);
}
struct s3c2410_platform_i2c default_i2c6_data __initdata = {
	.bus_num        = 6,
	.flags          = 0,
	.slave_addr     = 0x10,
	.frequency      = 100*1000,
	.sda_delay      = 100,
	.cfg_gpio	= i2c6_mhl_ddc_cfg_gpio,
};
#endif /* CONFIG_MACH_T0_EUR_OPEN */
#endif /* CONFIG_S3C_DEV_I2C6 */

#if defined(CONFIG_MACH_GC1)
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	},
};
static struct i2c_board_info i2c_devs7_s5m[] __initdata = {
	{
		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
		.platform_data = &exynos4_s5m8767_info,
	},
};
#else
static struct i2c_board_info i2c_devs7[] __initdata = {
#if defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_BAFFIN) || \
	defined(CONFIG_MACH_GD2) || defined(CONFIG_MACH_ZEST) || \
	defined(CONFIG_MACH_IPCAM)  || defined(CONFIG_MACH_GC2PD) || \
	defined(CONFIG_MACH_WATCH)
#if defined(CONFIG_REGULATOR_MAX77686) /* max77686 on i2c7 with M1 board */
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	},
#endif

#if defined(CONFIG_REGULATOR_S5M8767)
	{
		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
		.platform_data = &exynos4_s5m8767_info,
		.irq	= IRQ_EINT(7),
	},
#endif
#endif
};
#endif /* CONFIG_MACH_GC1 */

/* Bluetooth */
#ifdef CONFIG_BT_BCM4330
static struct platform_device bcm4330_bluetooth_device = {
	.name = "bcm4330_bluetooth",
	.id = -1,
};
#endif

#ifdef CONFIG_BT_BCM4334
static struct platform_device bcm4334_bluetooth_device = {
	.name = "bcm4334_bluetooth",
	.id = -1,
};
#endif
#ifdef CONFIG_BT_BCM4335
static struct platform_device bcm4335_bluetooth_device = {
	.name = "bcm4335_bluetooth",
	.id = -1,
};

#endif

#if !defined(CONFIG_MACH_GD2)
static struct i2c_gpio_platform_data gpio_i2c_data8 = {
	.sda_pin = GPIO_3_TOUCH_SDA,
	.scl_pin = GPIO_3_TOUCH_SCL,
};

struct platform_device s3c_device_i2c8 = {
	.name = "i2c-gpio",
	.id = 8,
	.dev.platform_data = &gpio_i2c_data8,
};

/* I2C8 */
static struct i2c_board_info i2c_devs8_emul[] = {
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
	{
		I2C_BOARD_INFO("sec_touchkey", 0x20),
		.platform_data = &touchkey_pdata,
	},
#endif
#ifdef CONFIG_KEYBOARD_TC370L_TOUCHKEY
	{
		I2C_BOARD_INFO(TC370_DEVICE, 0x20),
		.platform_data = &touchkey_pdata,
	},
#endif
};
#endif

/* I2C9 */
static struct i2c_board_info i2c_devs9_emul[] __initdata = {
};

/* I2C10 */
static struct i2c_board_info i2c_devs10_emul[] __initdata = {
};

/* I2C11 */
static struct i2c_board_info i2c_devs11_emul[] __initdata = {
};

/* I2C12 */
#if defined(CONFIG_PN65N_NFC) || defined(CONFIG_PN547_NFC)\
	|| defined(CONFIG_BCM2079X_NFC)
#if defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_ZEST) || defined(CONFIG_MACH_GC2PD)\
	|| defined(CONFIG_MACH_WATCH)
static struct i2c_board_info i2c_devs12_emul[] __initdata = {
};
#endif
#endif

#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
static struct i2c_gpio_platform_data gpio_i2c_data14 = {
	.sda_pin = GPIO_FUEL_SDA,
	.scl_pin = GPIO_FUEL_SCL,
};

struct platform_device s3c_device_i2c14 = {
	.name = "i2c-gpio",
	.id = 14,
	.dev.platform_data = &gpio_i2c_data14,
};

static struct max17047_platform_data max17047_pdata = {
	.irq_gpio = GPIO_FUEL_ALERT,
};

/* I2C14 */
static struct i2c_board_info i2c_devs14_emul[] __initdata = {
	{
		I2C_BOARD_INFO("max17047-fuelgauge", 0x36),
		.platform_data = &max17047_pdata,
	},
};
#endif

#ifdef CONFIG_SAMSUNG_MHL
/* I2C15 */
static struct i2c_gpio_platform_data gpio_i2c_data15 = {
	.sda_pin = GPIO_MHL_SDA_1_8V,
	.scl_pin = GPIO_MHL_SCL_1_8V,
	.udelay = 3,
	.timeout = 0,
};

struct platform_device s3c_device_i2c15 = {
	.name = "i2c-gpio",
	.id = 15,
	.dev = {
		.platform_data = &gpio_i2c_data15,
	}
};

static struct i2c_board_info i2c_devs15_emul[] __initdata = {
};

/* I2C16 */
#if !defined(CONFIG_MACH_T0) && !defined(CONFIG_MACH_M3) \
	&& !defined(CONFIG_MACH_GD2)
static struct i2c_gpio_platform_data gpio_i2c_data16 = {
	.sda_pin = GPIO_MHL_DSDA_2_8V,
	.scl_pin = GPIO_MHL_DSCL_2_8V,
};

struct platform_device s3c_device_i2c16 = {
	.name = "i2c-gpio",
	.id = 16,
	.dev.platform_data = &gpio_i2c_data16,
};
#endif /* !defined(CONFIG_MACH_T0) */

static struct i2c_board_info i2c_devs16_emul[] __initdata = {
};
#endif

#if !defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
#if defined(CONFIG_MFD_MAX77693)
static struct i2c_gpio_platform_data gpio_i2c_data17 = {
	.sda_pin = GPIO_IF_PMIC_SDA,
	.scl_pin = GPIO_IF_PMIC_SCL,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &gpio_i2c_data17,
};

/* I2C17 */
static struct i2c_board_info i2c_devs17_emul[] __initdata = {
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data	= &exynos4_max77693_info,
	}
};
#endif /* CONFIG_MFD_MAX77693 */
#endif /* !CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#ifdef CONFIG_MACH_GD2
static struct i2c_gpio_platform_data gpio_i2c_data18 = {
	.sda_pin = GPIO_CAM_SDA,
	.scl_pin = GPIO_CAM_SCL,
};

struct platform_device s3c_device_i2c18 = {
	.name = "i2c-gpio",
	.id = 18,
	.dev.platform_data = &gpio_i2c_data18,
};

/* I2C18 */
static struct i2c_board_info i2c_devs18_emul[] __initdata = {
};
#elif defined(CONFIG_MACH_GC2PD) && defined(CONFIG_SENSORS_K330)
static struct i2c_gpio_platform_data gpio_i2c_data18 = {
	.sda_pin	= GPIO_GSENSE_SDA_18V,
	.scl_pin	= GPIO_GSENSE_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c18 = {
	.name	= "i2c-gpio",
	.id	= 18,
	.dev.platform_data	= &gpio_i2c_data18,
};
#endif

#if 0
#if defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_M0)
static struct i2c_gpio_platform_data i2c18_platdata = {
	.sda_pin		= GPIO_8M_CAM_SDA_18V,
	.scl_pin		= GPIO_8M_CAM_SCL_18V,
	.udelay			= 2, /* 250 kHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c18 = {
	.name	= "i2c-gpio",
	.id	= 18,
	.dev.platform_data	= &i2c18_platdata,
};

/* I2C18 */
/* No explicit i2c client array here. The channel number 18 is passed
   to camera driver from midas-camera.c instead. */
#endif
#endif

/* I2C21 */
#if defined(CONFIG_LEDS_AN30259A) || defined(CONFIG_LEDS_LP5521)
static struct i2c_gpio_platform_data gpio_i2c_data21 = {
	.scl_pin = GPIO_S_LED_I2C_SCL,
	.sda_pin = GPIO_S_LED_I2C_SDA,
};

struct platform_device s3c_device_i2c21 = {
	.name = "i2c-gpio",
	.id = 21,
	.dev.platform_data = &gpio_i2c_data21,
};
#endif

/* I2C21 */
#ifdef CONFIG_LEDS_AN30259A
static struct i2c_board_info i2c_devs21_emul[] __initdata = {
	{
		I2C_BOARD_INFO("an30259a", 0x30),
	},
};
#endif

/* I2C22 */
#if defined(CONFIG_BARCODE_EMUL_ICE4)
static struct i2c_gpio_platform_data gpio_i2c_data22 = {
	.scl_pin = GPIO_BARCODE_SCL_1_8V,
	.sda_pin = GPIO_BARCODE_SDA_1_8V,
};

struct platform_device s3c_device_i2c22 = {
	.name = "i2c-gpio",
	.id = 22,
	.dev.platform_data = &gpio_i2c_data22,
};

static struct i2c_board_info i2c_devs22_emul[] __initdata = {
	{
		I2C_BOARD_INFO("ice4", (0x6c)),
	},
};
#elif defined(CONFIG_ICE4_FPGA)
static void irda_gpio_init(void)
{
	printk(KERN_ERR "%s called!\n", __func__);
	gpio_request(GPIO_IR_LED_EN, "ir_led_en");
	gpio_direction_output(GPIO_IR_LED_EN, 1);
#ifdef CONFIG_MACH_GC2PD
	gpio_request(GPIO_IR_LED_LDO_EN, "ir_led_ldo_en");
	gpio_direction_output(GPIO_IR_LED_LDO_EN, 1);
#endif
	gpio_request_one(GPIO_FPGA_CDONE, GPIOF_IN, "FPGA_CDONE");
	gpio_request_one(GPIO_FPGA_CRESET, GPIOF_OUT_INIT_HIGH, "FPGA_CRESET");
}
static void irda_fpga_device_init(void)
{
	int ret;
	printk(KERN_ERR "%s called!\n", __func__);
	ret = gpio_request(GPIO_IRDA_IRQ, "irda_irq");
	if (ret) {
		printk(KERN_ERR "%s: gpio_request fail[%d], ret = %d\n",
			__func__, GPIO_IRDA_IRQ, ret);
		return;
	}
	s3c_gpio_cfgpin(GPIO_IRDA_IRQ, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_IRDA_IRQ, S3C_GPIO_PULL_UP);
	gpio_direction_input(GPIO_IRDA_IRQ);
	printk(KERN_ERR "%s complete\n", __func__);
	return;

}
static struct i2c_gpio_platform_data gpio_i2c_data22 = {
	.sda_pin = GPIO_IRDA_SDA,
	.scl_pin = GPIO_IRDA_SCL,
	.udelay = 2,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0,
};

struct platform_device s3c_device_i2c22 = {
	.name = "i2c-gpio",
	.id = 22,
	.dev.platform_data = &gpio_i2c_data22,
};

static struct i2c_board_info i2c_devs22_emul[] __initdata = {
	{
		I2C_BOARD_INFO("ice4", (0x50)),
	},
};

void __init exynos4_fpga_init(void)
{
	printk(KERN_ERR "[%s] initialization start!\n", __func__);

	irda_gpio_init();

	barcode_fpga_firmware_update();

	irda_fpga_device_init();
}

#endif

#ifdef CONFIG_KEYBOARD_ADP5587
static struct i2c_gpio_platform_data gpio_i2c_data25 = {
	.sda_pin = GPIO_QT_GPIO_SDA,
	.scl_pin = GPIO_QT_GPIO_SCL,
	.udelay  = 0,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0
};

struct platform_device s3c_device_i2c25 = {
	.name = "i2c-gpio",
	.id = 25,
	.dev.platform_data = &gpio_i2c_data25,
};

/* I2C25 */
static struct i2c_board_info i2c_devs25_emul[] = {
	{
		I2C_BOARD_INFO("sec_keypad", 0x34),
		.platform_data = &adp5587_pdata,
	},
};
#endif

#if defined(CONFIG_FELICA)

#define  FELICA_GPIO_RFS_NAME     "FeliCa-RFS"
#define  FELICA_GPIO_PON_NAME     "FeliCa-PON"
#define  FELICA_GPIO_INT_NAME     "FeliCa-INT"
#define  FELICA_GPIO_I2C_SDA_NAME "FeliCa-SDA"
#define  FELICA_GPIO_I2C_SCL_NAME "FeliCa-SCL"

static struct  i2c_gpio_platform_data  i2c30_gpio_platdata = {
	.sda_pin = FELICA_GPIO_I2C_SDA,
	.scl_pin = FELICA_GPIO_I2C_SCL,
	.udelay  = 0,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0
};

static struct  platform_device  s3c_device_i2c30 = {
	.name  = "i2c-gpio",
	.id   = 30,                               /* adepter number */
	.dev.platform_data = &i2c30_gpio_platdata,
};

static struct i2c_board_info i2c_devs30[] __initdata = {
	{
		I2C_BOARD_INFO("felica_i2c", (0x56 >> 1)),
	},
};

#endif /* CONFIG_FELICA */

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
};

static int __init setup_ram_console_mem(char *str)
{
	unsigned size = memparse(str, &str);

	if (size && (*str == '@')) {
		unsigned long long base = simple_strtoul(++str, &str, 0);
		if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
			pr_err("%s: failed reserving size %d "
			       "at base 0x%llx\n", __func__, size, base);
			return -1;
		}

		ram_console_resource[0].start = base;
		ram_console_resource[0].end = base + size - 1;
		pr_err("%s: %x at %llx\n", __func__, size, base);
	}
	return 0;
}

__setup("ram_console=", setup_ram_console_mem);
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
static struct samsung_battery_platform_data samsung_battery_pdata = {
	.charger_name	= "max77693-charger",
	.fuelgauge_name	= "max17047-fuelgauge",
#if defined(CONFIG_CHARGER_MAX8922_U1)
	.sub_charger_name   = "max8922-charger",
#endif
#if defined(CONFIG_MACH_GC1)
	.voltage_max = 4200000,
#else
	.voltage_max = 4350000,
#endif
	.voltage_min = 3400000,

#if defined(CONFIG_MACH_GC1)
#if defined(CONFIG_MACH_GC1_USA_ATT)
	.in_curr_limit = 700,
	.chg_curr_ta = 700,
	.chg_curr_dock = 700,
#elif defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_GC1_USA_VZW)
	.in_curr_limit = 1000,
	.chg_curr_ta = 1200,
	.chg_curr_dock = 1000,
#else
	.in_curr_limit = 1000,
	.chg_curr_ta = 1000,
	.chg_curr_dock = 1000,
#endif
	.chg_curr_siop_lv1 = 475,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 475,
#elif defined(CONFIG_MACH_T0)
	.in_curr_limit = 1800,
	.chg_curr_ta = 1700,
	.chg_curr_dock = 1700,
	.chg_curr_siop_lv1 = 1000,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 1,	/* zero make charger off */
#elif defined(CONFIG_MACH_GD2)
	.in_curr_limit = 1900,
	.chg_curr_ta = 2000,
	.chg_curr_dock = 2000,
	.chg_curr_siop_lv1 = 1000,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 1,	/* zero make charger off */
#elif defined(CONFIG_MACH_BAFFIN_KOR_SKT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_KT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	.in_curr_limit = 1000,
	.chg_curr_ta = 1500,
	.chg_curr_dock = 1000,
	.chg_curr_siop_lv1 = 475,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 475,
#elif defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	.in_curr_limit = 1000,
	.chg_curr_ta = 1500,
	.chg_curr_dock = 1000,
	.chg_curr_siop_lv1 = 475,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 475,
#elif defined(CONFIG_MACH_ZEST)
	.in_curr_limit = 1000,
	.chg_curr_ta = 900,
	.chg_curr_dock = 900,
	.chg_curr_siop_lv1 = 475,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 475,
#else
	.in_curr_limit = 1000,
	.chg_curr_ta = 1000,
	.chg_curr_dock = 1000,
	.chg_curr_siop_lv1 = 475,
	.chg_curr_siop_lv2 = 475,
	.chg_curr_siop_lv3 = 475,
#endif

	.chg_curr_usb = 475,
	.chg_curr_cdp = 1000,
#if defined(CONFIG_MACH_T0_USA_VZW)
	.chg_curr_wpc = 650,
#else
	.chg_curr_wpc = 475,
#endif
	.chg_curr_etc = 475,

	.chng_interval = 30,
	.chng_susp_interval = 60,
	.norm_interval = 120,
	.norm_susp_interval = 7200,
	.emer_lv1_interval = 30,
	.emer_lv2_interval = 10,

#if defined(CONFIG_MACH_GC1)
	.recharge_voltage = 4150000,
#else
	/* it will be cacaluated in probe */
	.recharge_voltage = 4300000,
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC) || \
	defined(CONFIG_MACH_T0_USA_VZW) || defined(CONFIG_MACH_T0_USA_SPR) || \
	defined(CONFIG_MACH_T0_USA_USCC) || defined(CONFIG_MACH_T0_CHN_CTC) || \
	defined(CONFIG_MACH_GC1_USA_VZW)
#if defined(CONFIG_MACH_GC1)
	/* GC1-KOR, GC1-VZW - 1650mAh Battery : ABS Timer Spec(6hr / 2hr) */
	.abstimer_charge_duration = 6 * 60 * 60,
	.abstimer_charge_duration_wpc = 8 * 60 * 60,
	.abstimer_recharge_duration = 2 * 60 * 60,
#else
	.abstimer_charge_duration = 8 * 60 * 60,
	.abstimer_charge_duration_wpc = 8 * 60 * 60,
	.abstimer_recharge_duration = 2 * 60 * 60,
#endif
#else
	.abstimer_charge_duration = 6 * 60 * 60,
	.abstimer_charge_duration_wpc = 8 * 60 * 60,
	.abstimer_recharge_duration = 1.5 * 60 * 60,
#endif

	.cb_det_src = CABLE_DET_CHARGER,
#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT)
	.overheat_stop_temp = 640,
	.overheat_recovery_temp = 429,
	.freeze_stop_temp = -70,
	.freeze_recovery_temp = 8,
#elif defined(CONFIG_MACH_C1_KOR_LGT)
	.overheat_stop_temp = 630,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -70,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_M0_KOR_SKT) || defined(CONFIG_MACH_M0_KOR_KT)
	.overheat_stop_temp = 710,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -40,
	.freeze_recovery_temp = 30,
#elif defined(CONFIG_MACH_T0_KOR_SKT) || defined(CONFIG_MACH_T0_KOR_KT) || \
	defined(CONFIG_MACH_T0_KOR_LGT)
	.overheat_stop_temp = 660,
	.overheat_recovery_temp = 425,
	.freeze_stop_temp = -45,
	.freeze_recovery_temp = 3,
#elif defined(CONFIG_MACH_BAFFIN_KOR_SKT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_KT)
	.overheat_stop_temp = 620,
	.overheat_recovery_temp = 445,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 13,
#elif defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	.overheat_stop_temp = 620,
	.overheat_recovery_temp = 445,
	.freeze_stop_temp = -48,
	.freeze_recovery_temp = 15,
#elif defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	.overheat_stop_temp = 640,
	.overheat_recovery_temp = 420,
	.freeze_stop_temp = -49,
	.freeze_recovery_temp = -11,
#elif defined(CONFIG_MACH_GC1)
	.overheat_stop_temp = 600,
	.overheat_recovery_temp = 412,
	.freeze_stop_temp = -30,
	.freeze_recovery_temp = 3,
#else
	.overheat_stop_temp = 600,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#endif /* KOR model */
#elif defined(CONFIG_TARGET_LOCALE_USA)
#if defined(CONFIG_MACH_C1_USA_ATT)
	.overheat_stop_temp = 450,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_T0_USA_ATT)
	.overheat_stop_temp = 475,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_T0_USA_VZW)
	.overheat_stop_temp = 515,
	.overheat_recovery_temp = 440,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_T0_USA_TMO)
	.overheat_stop_temp = 475,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_T0_USA_SPR)
	.overheat_stop_temp = 470,
	.overheat_recovery_temp = 420,
	.freeze_stop_temp = -80,
	.freeze_recovery_temp = -10,
#elif defined(CONFIG_MACH_T0_USA_USCC)
	.overheat_stop_temp = 630,
	.overheat_recovery_temp = 420,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 30,
#elif defined(CONFIG_MACH_GC1_USA_VZW)
	.overheat_stop_temp = 470,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -49,
	.freeze_recovery_temp = 15,
#elif defined(CONFIG_MACH_M3_USA_TMO)
	.overheat_stop_temp = 460,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_ZEST)
	.overheat_stop_temp = 475,
	.overheat_recovery_temp = 415,
	.freeze_stop_temp = -35,
	.freeze_recovery_temp = 5,
#else
	/* USA default */
	.overheat_stop_temp = 450,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#endif
#elif defined(CONFIG_MACH_T0_CHN_CTC)
	.overheat_stop_temp = 642,
	.overheat_recovery_temp = 450,
	.freeze_stop_temp = -40,
	.freeze_recovery_temp = 20,
#elif defined(CONFIG_MACH_M0_CTC)
#if defined(CONFIG_MACH_M0_DUOSCTC)
	.overheat_stop_temp = 660,
	.overheat_recovery_temp = 440,
	.freeze_stop_temp = -40,
	.freeze_recovery_temp = 20,
#else
	.overheat_stop_temp = 640,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 30,
#endif
#elif defined(CONFIG_MACH_GC1)
	.overheat_stop_temp = 600,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#else
	/* M0 EUR */
	.overheat_stop_temp = 600,
	.overheat_recovery_temp = 400,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,
#endif

	/* CTIA spec */
#if defined(CONFIG_TARGET_LOCALE_USA) && !defined(CONFIG_MACH_T0_USA_USCC)
	.ctia_spec  = true,
#else
	.ctia_spec  = false,
#endif

	/* CTIA temperature spec */
	.event_time = 10 * 60,
#if defined(CONFIG_MACH_C1_USA_ATT)
	.event_overheat_stop_temp = 600,
	.event_overheat_recovery_temp = 400,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 480,
	.lpm_overheat_recovery_temp = 450,
	.lpm_freeze_stop_temp = -50,
	.lpm_freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_T0_USA_VZW)
	.event_overheat_stop_temp = 600,
	.event_overheat_recovery_temp = 409,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 480,
	.lpm_overheat_recovery_temp = 450,
	.lpm_freeze_stop_temp = -50,
	.lpm_freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_GC1_USA_VZW)
	.event_overheat_stop_temp = 610,
	.event_overheat_recovery_temp = 400,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 480,
	.lpm_overheat_recovery_temp = 450,
	.lpm_freeze_stop_temp = -50,
	.lpm_freeze_recovery_temp = 0,
#elif defined(CONFIG_MACH_M3_USA_TMO)
	.event_overheat_stop_temp = 600,
	.event_overheat_recovery_temp = 400,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 460,
	.lpm_overheat_recovery_temp = 430,
	.lpm_freeze_stop_temp = -40,
	.lpm_freeze_recovery_temp = 10,
#elif defined(CONFIG_MACH_ZEST)
	.event_overheat_stop_temp = 635,
	.event_overheat_recovery_temp = 415,
	.event_freeze_stop_temp = 5,
	.event_freeze_recovery_temp = -15,
	.lpm_overheat_stop_temp = 475,
	.lpm_overheat_recovery_temp = 415,
	.lpm_freeze_stop_temp = -35,
	.lpm_freeze_recovery_temp = 5,
#else
	/* USA default */
	.event_overheat_stop_temp = 600,
	.event_overheat_recovery_temp = 400,
	.event_freeze_stop_temp = -50,
	.event_freeze_recovery_temp = 0,
	.lpm_overheat_stop_temp = 480,
	.lpm_overheat_recovery_temp = 450,
	.lpm_freeze_stop_temp = -50,
	.lpm_freeze_recovery_temp = 0,
#endif

	.temper_src = TEMPER_AP_ADC,
	.temper_ch = 2, /* if src == TEMPER_AP_ADC */
#ifdef CONFIG_S3C_ADC
	/*
	 * s3c adc driver does not convert raw adc data.
	 * so, register convert function.
	 */
	.covert_adc = convert_adc,
#endif

#if (defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_M3_USA_TMO)) && \
	defined(CONFIG_TARGET_LOCALE_USA)
	.vf_det_src = VF_DET_GPIO,	/* check H/W rev in battery probe */
#elif (defined(CONFIG_MACH_GC1) && \
	defined(CONFIG_TARGET_LOCALE_USA)) || \
	defined(CONFIG_MACH_ZEST)
	.vf_det_src = VF_DET_ADC_GPIO,	/* check H/W rev in battery probe */
#else
	.vf_det_src = VF_DET_CHARGER,
#endif
	.vf_det_ch = 0,	/* if src == VF_DET_ADC */
#if defined(CONFIG_MACH_GC1)
	.vf_det_th_l = 310,
	.vf_det_th_h = 490,
#elif defined(CONFIG_MACH_ZEST)
	.vf_det_th_l = 500,
	.vf_det_th_h = 800,
#else
	.vf_det_th_l = 100,
	.vf_det_th_h = 1500,
#endif
#if ((defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_GC1) || \
	defined(CONFIG_MACH_M3_USA_TMO)) && \
	defined(CONFIG_TARGET_LOCALE_USA)) || \
	defined(CONFIG_MACH_ZEST)
	.batt_present_gpio = GPIO_BATT_PRESENT_N_INT,
#endif

	.suspend_chging = true,

	.led_indicator = false,

	.battery_standever = false,
};

static struct platform_device samsung_device_battery = {
	.name	= "samsung-battery",
	.id	= -1,
	.dev.platform_data = &samsung_battery_pdata,
};
#endif

#ifdef CONFIG_SEC_SUBTHERMISTOR
#if defined(CONFIG_MACH_IPCAM)
static struct sec_therm_adc_table subtemper_table_ap[] = {
	{ 159,   800 },
	{ 192,   750 },
	{ 231,   700 },
	{ 267,   650 },
	{ 321,   600 },
	{ 387,   550 },
	{ 454,   500 },
	{ 532,   450 },
	{ 625,   400 },
	{ 715,   350 },
	{ 823,   300 },
	{ 934,   250 },
	{ 1033,  200 },
	{ 1146,  150 },
	{ 1259,  100 },
	{ 1381,  50 },
	{ 1489,  0 },
	{ 1582,  -50 },
	{ 1654,  -100 },
	{ 1726,  -150 },
};

static struct sec_therm_platform_data sec_subtherm_pdata = {
	.adc_channel	= 3,
	.adc_arr_size	= ARRAY_SIZE(subtemper_table_ap),
	.adc_table	= subtemper_table_ap,
	.polling_interval = 20 * 1000, /* msecs */
	.get_siop_level = NULL,
};
#else
static struct sec_therm_adc_table subtemper_table_ap[] = {
	{ 345,   600 },
	{ 390,   520 },
	{ 410,   500 },
	{ 698,   400 },
	{ 898,   300 },
	{ 1132,  200 },
	{ 1363,  100 },
	{ 1574,    0 },
	{ 1732, -100 },
	{ 1860, -200 },
};

static struct sec_therm_platform_data sec_subtherm_pdata = {
	.adc_channel	= 3,
	.adc_arr_size	= ARRAY_SIZE(subtemper_table_ap),
	.adc_table	= subtemper_table_ap,
	.polling_interval = 20 * 1000, /* msecs */
	.get_siop_level = NULL,
};
#endif

struct platform_device sec_device_subthermistor = {
	.name = "sec-subthermistor",
	.id = -1,
	.dev.platform_data = &sec_subtherm_pdata,
};
#endif

#ifdef CONFIG_KEYBOARD_GPIO
#define GPIO_KEYS(_code, _gpio, _active_low, _iswake, _hook)	\
	{							\
		.code = _code,					\
		.gpio = _gpio,					\
		.active_low = _active_low,			\
		.type = EV_KEY,					\
		.wakeup = _iswake,				\
		.debounce_interval = 10,			\
		.isr_hook = _hook,				\
		.value = 1					\
	}

struct gpio_keys_button midas_buttons[] = {
#if defined(CONFIG_MACH_GC1)
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_CAMERA_FOCUS, GPIO_S1_KEY,
			1, 1, sec_debug_check_crash_key),
	/*KEY_CAMERA_SHUTTER*/
	GPIO_KEYS(0x220, GPIO_S2_KEY,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_CAMERA_ZOOMIN, GPIO_TELE_KEY,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_CAMERA_ZOOMOUT, GPIO_WIDE_KEY,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(0x221, GPIO_FAST_TELE_KEY,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(0x222, GPIO_FAST_WIDE_KEY,
			1, 1, sec_debug_check_crash_key),
#if 0
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_MENU, GPIO_MENU_KEY,
			1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_BACK, GPIO_BACK_KEY,
			1, 1, sec_debug_check_crash_key),
#endif
#elif defined(CONFIG_MACH_ZEST)
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
#elif defined(CONFIG_MACH_WATCH)
	/* NOTICE: if nPOWER's key code is changed,
	 *         sec_debug.c should be also modifed.
	 */
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key), /* use only power key on real board */
#if defined(CONFIG_WATCH_00_BD) || defined(CONFIG_WATCH_01_BD)
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
#endif
#elif defined(CONFIG_MACH_IPCAM)
	GPIO_KEYS(KEY_POWER, GPIO_RESET_KEY,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_BLUETOOTH, GPIO_EASY_SETUP,
		  1, 1, sec_debug_check_crash_key),
#else
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
#endif
};

#if defined(CONFIG_MACH_M0_CTC) \
	|| defined(CONFIG_MACH_C1) \
	|| defined(CONFIG_MACH_M0)
struct gpio_keys_button m0_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
};
#endif

#if defined(CONFIG_MACH_M0) || \
	defined(CONFIG_MACH_C1_USA_ATT)
struct gpio_keys_button m0_rev11_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
};
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) && \
	(defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1))
struct gpio_keys_button c1_rev04_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
};
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) && \
	defined(CONFIG_MACH_BAFFIN)
struct gpio_keys_button baffin_kor_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
};
struct gpio_keys_button baffin_kor_rev06_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  0, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  0, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
};
#endif

#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_M3)
#if defined(CONFIG_TARGET_LOCALE_KOR)
struct gpio_keys_button t0_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY,
		  1, 1, sec_debug_check_crash_key),
};
#else
struct gpio_keys_button t0_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY,
		  1, 1, sec_debug_check_crash_key),
};
#endif
#endif

#if defined(CONFIG_MACH_M3_USA_TMO)
struct gpio_keys_button m3_uas_tmo_00_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY,
		  1, 1, sec_debug_check_crash_key),
};
#endif

struct gpio_keys_platform_data midas_gpiokeys_platform_data = {
	midas_buttons,
	ARRAY_SIZE(midas_buttons),
#ifdef CONFIG_MACH_GC1
	.gpio_strobe_insert = STR_PU_DET_18V,
#endif
};

static struct platform_device midas_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &midas_gpiokeys_platform_data,
	},
};
#endif	/* CONFIG_KEYBOARD_GPIO */

#if defined(CONFIG_INPUT_SEC_KEYS)
static int gd2_sec_key_init(u32 gpio, const char *name)
{
	int ret = 0;

	ret = gpio_request(gpio, name);
	if (ret)
		printk(KERN_ERR "[sec_keys] failed to request %s\n", name);
	else {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_INPUT);
#if defined(CONFIG_MACH_GC2PD)
		if (((system_rev == 0x1) || (system_rev == 0x2) ||
			(system_rev == 0x3)) &&
			((gpio == GPIO_S1_KEY) || (gpio == GPIO_S2_KEY) ||
			(gpio == GPIO_HOME_KEY)))
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		else
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
#elif defined(CONFIG_GD2_01_BD)
		if ((gpio == GPIO_CAMERA_IFUNC) || (gpio == GPIO_CAMERA_FOCUS_L) ||
			(gpio == GPIO_CAMERA_FOCUS_R))
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);
		else
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
#else
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
#endif
	}
	return ret;
}

#if defined(CONFIG_MACH_GC2PD)
static void check_vdd(void)
{
	u32 gpio = GPIO_IR_LED_LDO_EN;
	if (!gpio_get_value(gpio)) {
		printk(KERN_ERR "[sec_keys] set high ir led\n");
		gpio_set_value(gpio, true);
	}

	gpio = GPIO_ZRPR_EN;
	if (!gpio_get_value(gpio)) {
		printk(KERN_ERR "[sec_keys] set high ir led\n");
		gpio_set_value(gpio, true);
	}
}
#endif

#define SEC_KEYS(_name, _gpio, _code, _wake, _act_type, _type, _deb)	\
{									\
	.name = _name,					\
	.wakeup = _wake,					\
	.act_type = _act_type,					\
	.gpio = _gpio,					\
	.type = _type,					\
	.code = _code,					\
	.debounce = _deb,					\
	.gpio_init = gd2_sec_key_init,		\
}

static struct sec_keys_info sec_key_info[] = {
#if defined(CONFIG_MACH_GD2)
	SEC_KEYS("nPOWER", GPIO_nPOWER, KEY_POWER,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER1", GPIO_S1_KEY, KEY_CAMERA_FOCUS,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER2", GPIO_S2_KEY, KEY_CAMERA_SHUTTER,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("JOG_R", GPIO_JOG_R, KEY_CAMERA_JOG_R,
		false, KEY_ACT_TYPE_JOG_KEY, EV_KEY, 0),
	SEC_KEYS("JOG_L", GPIO_JOG_L, KEY_CAMERA_JOG_L,
		false, KEY_ACT_TYPE_JOG_KEY, EV_KEY, 0),
	SEC_KEYS("JOG_OK", GPIO_JOG_KEY, KEY_SELECT,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("RECORD", GPIO_RECORD, KEY_RECORD,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
#if defined(CONFIG_GD2_01_BD)
	SEC_KEYS("STR_UP", GPIO_STR_UP, KEY_CAMERA_STROBE,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("IFUNC", GPIO_CAMERA_IFUNC, KEY_CAMERA_IFUNC,
		false, KEY_ACT_TYPE_ONE_CH, EV_KEY, 15),
	SEC_KEYS("FOCUS_LEFT", GPIO_CAMERA_FOCUS_L, KEY_CAMERA_FOCUS_L,
		false, KEY_ACT_TYPE_ONE_CH, EV_KEY, 0),
	SEC_KEYS("FOCUS_RIGHT", GPIO_CAMERA_FOCUS_R, KEY_CAMERA_FOCUS_R,
		false, KEY_ACT_TYPE_ONE_CH, EV_KEY, 0),
#endif	/* CONFIG_GD2_01_BD */
#elif defined(CONFIG_MACH_GC2PD)
	SEC_KEYS("nPOWER", GPIO_nPOWER, KEY_POWER,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("Home", GPIO_HOME_KEY, KEY_HOMEPAGE,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER1", GPIO_S1_KEY, KEY_CAMERA_FOCUS,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER2", GPIO_S2_KEY, KEY_CAMERA,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("VOL_UP", GPIO_VOL_UP, KEY_VOLUMEUP,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("VOL_DOWN", GPIO_VOL_DOWN, KEY_VOLUMEDOWN,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("ZOOM_IN", GPIO_WIDE_KEY, KEY_ZOOM_RING_IN,
		false, KEY_ACT_TYPE_ZOOM_RING, EV_KEY, 40),
	SEC_KEYS("ZOOM_OUT", GPIO_TELE_KEY, KEY_ZOOM_RING_OUT,
		false, KEY_ACT_TYPE_ZOOM_RING, EV_KEY, 40),
#endif	/* CONFIG_MACH_GC2PD */
};

#if defined(CONFIG_MACH_GC2PD)
static struct sec_keys_info sec_key_info01[] = {
	SEC_KEYS("nPOWER", GPIO_nPOWER, KEY_POWER,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("Home", GPIO_HOME_KEY, KEY_HOMEPAGE,
		true, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER1", GPIO_S1_KEY, KEY_CAMERA_FOCUS,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("SHUTTER2", GPIO_S2_KEY, KEY_CAMERA,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("VOL_UP", GPIO_VOL_UP, KEY_VOLUMEUP,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("VOL_DOWN", GPIO_VOL_DOWN, KEY_VOLUMEDOWN,
		false, KEY_ACT_TYPE_LOW, EV_KEY, 10),
	SEC_KEYS("ZOOM_IN", GPIO_WIDE_KEY_0, KEY_ZOOM_RING_IN,
		false, KEY_ACT_TYPE_ZOOM_RING, EV_KEY, 40),
	SEC_KEYS("ZOOM_OUT", GPIO_TELE_KEY_0, KEY_ZOOM_RING_OUT,
		false, KEY_ACT_TYPE_ZOOM_RING, EV_KEY, 40),
	SEC_KEYS("HALL", GPIO_HALL_INT_N, SW_FLIP,
		true, KEY_ACT_TYPE_HIGH, EV_SW, 10),
};
#endif

static struct sec_keys_platform_data gd2_keys_platform_data = {
	.key_info = sec_key_info,
	.nkeys = ARRAY_SIZE(sec_key_info),
	.ev_rep = false,
};

static struct platform_device gd2_keys = {
	.name	= "sec_keys",
	.dev	= {
		.platform_data = &gd2_keys_platform_data,
	},
};

#if defined(CONFIG_MACH_GC2PD)
extern unsigned int lpcharge;
static void sec_key_init(void)
{
	int ret = 0;
	u32 gpio = GPIO_IR_LED_LDO_EN;
	ret = gpio_request(gpio, "IR_LED");
	if (ret)
		printk(KERN_ERR "[sec_keys] failed to request IR_LED\n");
	else {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		gpio_set_value(gpio, true);
	}

	gpio = GPIO_ZRPR_EN;
	ret = gpio_request(gpio, "ZRPR");
	if (ret)
		printk(KERN_ERR "[sec_keys] failed to request ZRPR\n");
	else {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		gpio_set_value(gpio, true);
	}

	if (system_rev >= 1) {
		s5p_register_gpio_interrupt(GPIO_WIDE_KEY_0);
		s5p_register_gpio_interrupt(GPIO_TELE_KEY_0);
		s5p_register_gpio_interrupt(GPIO_HALL_INT_N);
		gd2_keys_platform_data.key_info = sec_key_info01;
		gd2_keys_platform_data.nkeys = ARRAY_SIZE(sec_key_info01);
	}
	gd2_keys_platform_data.gpio_check = check_vdd;
	gd2_keys_platform_data.lpcharge = lpcharge;
};
#endif
#if defined(CONFIG_MACH_GD2)
extern unsigned int lpcharge;
static void sec_key_init(void)
{
	gd2_keys_platform_data.lpcharge = lpcharge;
};
#endif

#endif	/* CONFIG_INPUT_SEC_KEYS */

#if defined(CONFIG_SEC_DEBUG)
struct input_debug_key_state kstate[] = {
#if defined(CONFIG_MACH_GD2)
	SET_DEBUG_KEY(KEY_CAMERA_FOCUS, true),
	SET_DEBUG_KEY(KEY_CAMERA_SHUTTER, true),
	SET_DEBUG_KEY(KEY_POWER, false),
	SET_DEBUG_KEY(KEY_CAMERA_JOG_L, false),
	SET_DEBUG_KEY(KEY_CAMERA_JOG_R, false),
	SET_DEBUG_KEY(KEY_SELECT, false),
	SET_DEBUG_KEY(KEY_RECORD, false),
#if defined(CONFIG_GD2_01_BD)
	SET_DEBUG_KEY(KEY_CAMERA_STROBE, false),
#endif	/* CONFIG_GD2_01_BD */
#elif defined(CONFIG_MACH_GC2PD)
	SET_DEBUG_KEY(KEY_CAMERA_FOCUS, false),
	SET_DEBUG_KEY(KEY_CAMERA, false),
	SET_DEBUG_KEY(KEY_POWER, false),
	SET_DEBUG_KEY(KEY_HOMEPAGE, false),
	SET_DEBUG_KEY(KEY_ZOOM_RING_IN, false),
	SET_DEBUG_KEY(KEY_ZOOM_RING_OUT, false),
	SET_DEBUG_KEY(KEY_VOLUMEUP, false),
	SET_DEBUG_KEY(KEY_VOLUMEDOWN, true),
#endif	/* CONFIG_MACH_GC2PD */
#if defined(CONFIG_FAST_BOOT)
	SET_DEBUG_KEY(KEY_FAKE_PWR, false),
#endif	/* CONFIG_FAST_BOOT */
#if defined(CONFIG_MACH_SF2)
	SET_DEBUG_KEY(KEY_POWER, false),
	SET_DEBUG_KEY(KEY_VOLUMEUP, false),
	SET_DEBUG_KEY(KEY_VOLUMEDOWN, false),
#endif
};

static struct input_debug_pdata input_debug_pdata = {
	.nkeys = ARRAY_SIZE(kstate),
	.key_state = kstate,
};

static struct platform_device input_debug = {
	.name	= SEC_DEBUG_NAME,
	.dev	= {
		.platform_data = &input_debug_pdata,
	},
};
#endif	/* CONFIG_SEC_DEBUG */
#if defined(CONFIG_SF2_NFC_TAG)
static struct nfc_tag_pdata nfc_tag_pdata ={
	.irq_nfc = GPIO_NFC_IRQ,
};
static struct platform_device nfc_tag_devdata={
	.name = "NFC_TAG",
	.dev  = {
              .platform_data =&nfc_tag_pdata,
	},
};
#endif
#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 0x41,
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 199 * 1000000,	/* 160 Mhz */
};
#endif

/* BUSFREQ to control memory/bus */
static struct device_domain busfreq;

static struct platform_device exynos4_busfreq = {
	.id = -1,
	.name = "exynos-busfreq",
};

static struct i2c_gpio_platform_data i2c9_platdata = {
#if defined(CONFIG_SENSORS_CM3663)
	.sda_pin	= GPIO_PS_ALS_SDA_18V,
	.scl_pin	= GPIO_PS_ALS_SCL_18V,
#elif defined(CONFIG_SENSORS_BH1721)
	.sda_pin	= GPIO_PS_ALS_SDA_28V,
	.scl_pin	= GPIO_PS_ALS_SCL_28V,
#elif defined(CONFIG_SENSORS_CM36651)
	.sda_pin	= GPIO_RGB_SDA_1_8V,
	.scl_pin	= GPIO_RGB_SCL_1_8V,
#elif defined(CONFIG_SENSORS_GP2A)
	.sda_pin	= GPIO_PS_ALS_SDA_28V,
	.scl_pin	= GPIO_PS_ALS_SCL_28V,
#elif defined(CONFIG_SENSORS_TMD27723)
	.sda_pin	= GPIO_PS_ALS_SDA_28V,
	.scl_pin	= GPIO_PS_ALS_SCL_28V,
#endif
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c9 = {
	.name	= "i2c-gpio",
	.id	= 9,
	.dev.platform_data	= &i2c9_platdata,
};

#ifdef CONFIG_SENSORS_AK8975C
static struct i2c_gpio_platform_data i2c10_platdata = {
	.sda_pin	= GPIO_MSENSOR_SDA_18V,
	.scl_pin	= GPIO_MSENSOR_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name	= "i2c-gpio",
	.id	= 10,
	.dev.platform_data	= &i2c10_platdata,
};
#endif

#ifdef CONFIG_SENSORS_AK8963C
static struct i2c_gpio_platform_data i2c10_platdata = {
	.sda_pin	= GPIO_MSENSOR_SDA_18V,
	.scl_pin	= GPIO_MSENSOR_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name	= "i2c-gpio",
	.id	= 10,
	.dev.platform_data	= &i2c10_platdata,
};
#endif

#ifdef CONFIG_SENSORS_AK8963
static struct i2c_gpio_platform_data i2c10_platdata = {
	.sda_pin	= GPIO_MSENSOR_SDA_18V,
	.scl_pin	= GPIO_MSENSOR_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name	= "i2c-gpio",
	.id	= 10,
	.dev.platform_data	= &i2c10_platdata,
};
#endif

#ifdef CONFIG_SENSORS_LPS331
static struct i2c_gpio_platform_data i2c11_platdata = {
	.sda_pin	= GPIO_BSENSE_SDA_18V,
	.scl_pin	= GPIO_BENSE_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c11 = {
	.name			= "i2c-gpio",
	.id	= 11,
	.dev.platform_data	= &i2c11_platdata,
};
#endif

#if defined(CONFIG_PN65N_NFC) || defined(CONFIG_PN547_NFC)\
	|| defined(CONFIG_BCM2079X_NFC)
#if defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_ZEST) || defined(CONFIG_MACH_GC2PD) \
	|| defined(CONFIG_MACH_WATCH)
static struct i2c_gpio_platform_data i2c12_platdata = {
	.sda_pin		= GPIO_NFC_SDA_18V,
	.scl_pin		= GPIO_NFC_SCL_18V,
	.udelay			= 2, /* 250 kHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c12 = {
	.name	= "i2c-gpio",
	.id		= 12,
	.dev.platform_data	= &i2c12_platdata,
};
#endif
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
static void otg_accessory_power(int enable)
{
	u8 on = (u8)!!enable;
	int err;
#ifdef CONFIG_MFD_MAX77693
	/* max77693 otg power control */
	otg_control(enable);
#endif
#if !defined(CONFIG_MACH_M3_USA_TMO) && !defined(CONFIG_MACH_ZEST)
	err = gpio_request(GPIO_OTG_EN, "USB_OTG_EN");
	if (err)
		printk(KERN_ERR "failed to request USB_OTG_EN\n");
	gpio_direction_output(GPIO_OTG_EN, on);
	gpio_free(GPIO_OTG_EN);
#endif
	pr_info("%s: otg accessory power = %d\n", __func__, on);
}

static void otg_accessory_powered_booster(int enable)
{
	u8 on = (u8)!!enable;
#ifdef CONFIG_MFD_MAX77693
	/* max77693 powered otg power control */
	powered_otg_control(enable);
#endif
	pr_info("%s: otg accessory power = %d\n", __func__, on);
}

static struct host_notifier_platform_data host_notifier_pdata = {
	.ndev.name	= "usb_otg",
	.booster	= otg_accessory_power,
	.powered_booster = otg_accessory_powered_booster,
	.thread_enable	= 0,
};

struct platform_device host_notifier_device = {
	.name = "host_notifier",
	.dev.platform_data = &host_notifier_pdata,
};
#endif

#ifdef CONFIG_SEC_WATCHDOG_RESET
static struct platform_device watchdog_reset_device = {
	.name = "watchdog-reset",
	.id = -1,
};
#endif

#ifdef CONFIG_CORESIGHT_ETM

#define CORESIGHT_PHYS_BASE		0x10880000
#define CORESIGHT_ETB_PHYS_BASE		(CORESIGHT_PHYS_BASE + 0x1000)
#define CORESIGHT_TPIU_PHYS_BASE	(CORESIGHT_PHYS_BASE + 0x3000)
#define CORESIGHT_FUNNEL_PHYS_BASE	(CORESIGHT_PHYS_BASE + 0x4000)
#define CORESIGHT_ETM_PHYS_BASE		(CORESIGHT_PHYS_BASE + 0x1C000)

static struct resource coresight_etb_resources[] = {
	{
		.start = CORESIGHT_ETB_PHYS_BASE,
		.end   = CORESIGHT_ETB_PHYS_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device coresight_etb_device = {
	.name          = "coresight_etb",
	.id            = -1,
	.num_resources = ARRAY_SIZE(coresight_etb_resources),
	.resource      = coresight_etb_resources,
};

static struct resource coresight_tpiu_resources[] = {
	{
		.start = CORESIGHT_TPIU_PHYS_BASE,
		.end   = CORESIGHT_TPIU_PHYS_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device coresight_tpiu_device = {
	.name          = "coresight_tpiu",
	.id            = -1,
	.num_resources = ARRAY_SIZE(coresight_tpiu_resources),
	.resource      = coresight_tpiu_resources,
};

static struct resource coresight_funnel_resources[] = {
	{
		.start = CORESIGHT_FUNNEL_PHYS_BASE,
		.end   = CORESIGHT_FUNNEL_PHYS_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device coresight_funnel_device = {
	.name          = "coresight_funnel",
	.id            = -1,
	.num_resources = ARRAY_SIZE(coresight_funnel_resources),
	.resource      = coresight_funnel_resources,
};

static struct resource coresight_etm_resources[] = {
	{
		.start = CORESIGHT_ETM_PHYS_BASE,
		.end   = CORESIGHT_ETM_PHYS_BASE + (SZ_4K * 4) - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device coresight_etm_device = {
	.name          = "coresight_etm",
	.id            = -1,
	.num_resources = ARRAY_SIZE(coresight_etm_resources),
	.resource      = coresight_etm_resources,
};

#endif

static struct platform_device *midas_devices[] __initdata = {
#ifdef CONFIG_SEC_WATCHDOG_RESET
	&watchdog_reset_device,
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
	/* Samsung Power Domain */
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_GPS],
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_pd[PD_ISP],
#endif
	&exynos4_device_pd[PD_GPS_ALIVE],
	/* legacy fimd */
#ifdef CONFIG_FB_S5P
	&s3c_device_fb,
#ifdef CONFIG_FB_S5P_LMS501KF03
	&s3c_device_spi_gpio,
#endif
#endif

#ifdef CONFIG_FB_S5P_MDNIE
	&mdnie_device,
#endif
#ifdef CONFIG_LCD_FREQ_SWITCH
	&lcdfreq_device,
#endif
#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif

#ifdef CONFIG_SND_SOC_WM8994
	&vbatt_device,
#endif

	&s3c_device_wdt,
	&s3c_device_rtc,

	&s3c_device_i2c0,
#if !defined(CONFIG_MACH_GC2PD)
	&s3c_device_i2c1,
#endif
#ifdef CONFIG_S3C_DEV_I2C2
	&s3c_device_i2c2,
#endif
	&s3c_device_i2c3,
#ifdef CONFIG_S3C_DEV_I2C4
	&s3c_device_i2c4,
#endif

#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
	&s3c_device_i2c5,
#endif

#if defined(CONFIG_AUDIENCE_ES305) || \
	defined(CONFIG_MACH_T0_EUR_OPEN) || \
	defined(CONFIG_MACH_T0_CHN_OPEN) || \
	defined(CONFIG_MACH_GD2)
	&s3c_device_i2c6,
#elif defined(CONFIG_MACH_ZEST)
	&s3c_device_i2c6,
#endif
	&s3c_device_i2c7,
#if !defined(CONFIG_MACH_GD2)
	&s3c_device_i2c8,
#endif
	&s3c_device_i2c9,
#ifdef CONFIG_SENSORS_AK8975C
	&s3c_device_i2c10,
#endif
#ifdef CONFIG_SENSORS_AK8963C
	&s3c_device_i2c10,
#endif
#ifdef CONFIG_SENSORS_AK8963
	&s3c_device_i2c10,
#endif

#ifdef CONFIG_SENSORS_LPS331
	&s3c_device_i2c11,
#endif
	/* &s3c_device_i2c12, */
#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
	&s3c_device_i2c14,	/* max17047-fuelgauge */
#endif

#ifdef CONFIG_SAMSUNG_MHL
	&s3c_device_i2c15,
#if !defined(CONFIG_MACH_T0) && \
	!defined(CONFIG_MACH_M3) && \
	!defined(CONFIG_MACH_GD2)
	&s3c_device_i2c16,
#endif
#endif /* CONFIG_SAMSUNG_MHL */

#if !defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
#if defined(CONFIG_MFD_MAX77693)
	&s3c_device_i2c17,
#endif /* CONFIG_MFD_MAX77693 */
#endif /* !CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */
#if defined(CONFIG_MACH_GD2) ||\
	(defined(CONFIG_MACH_GC2PD) && defined(CONFIG_SENSORS_K330))
	&s3c_device_i2c18,
#endif

#ifdef CONFIG_DC_MOTOR
	&dc_motor_device,
#endif

#if defined(CONFIG_LEDS_AN30259A) || defined(CONFIG_LEDS_LP5521)
	&s3c_device_i2c21,
#endif
#if defined(CONFIG_BARCODE_EMUL_ICE4)  || defined(CONFIG_ICE4_FPGA)
	&s3c_device_i2c22,
#endif
#ifdef CONFIG_KEYBOARD_ADP5587
	&s3c_device_i2c25,
#endif
#if defined(CONFIG_FELICA)
	&s3c_device_i2c30,
#endif

#if defined CONFIG_USB_EHCI_S5P
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
#else
	&s5p_device_ehci,
#endif
#endif

#if defined CONFIG_USB_OHCI_S5P
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
#else
	&s5p_device_ohci,
#endif
#endif

#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#if defined(CONFIG_USB_ANDROID) || defined(CONFIG_USB_G_ANDROID)
	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
#endif
#ifdef CONFIG_EXYNOS4_DEV_MSHC
	&s3c_device_mshci,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

#ifdef CONFIG_SND_SAMSUNG_AC97
	&exynos_device_ac97,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos_device_pcm0,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
	&exynos_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos_device_srp,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_fimc_is,
#endif
#ifdef CONFIG_FB_S5P_LD9040
	&ld9040_spi_gpio,
#endif
#ifdef CONFIG_FB_S5P_GD2EVF
	&gd2evf_spi_gpio,
#endif
#ifdef CONFIG_VIDEO_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif
#ifdef CONFIG_FB_S5P_EXTDSP
	&s3c_device_extdsp,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_fimc3,
/* CONFIG_VIDEO_SAMSUNG_S5P_FIMC is the feature for mainline */
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc3,
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
	&s3c_device_csis0,
	&s3c_device_csis1,
#endif
#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	&s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
	&SYSMMU_PLATDEV(g2d_acp),
	&SYSMMU_PLATDEV(fimc0),
	&SYSMMU_PLATDEV(fimc1),
	&SYSMMU_PLATDEV(fimc2),
	&SYSMMU_PLATDEV(fimc3),
	&SYSMMU_PLATDEV(jpeg),
#ifdef CONFIG_FB_S5P_SYSMMU
	&SYSMMU_PLATDEV(fimd0),
#endif
	&SYSMMU_PLATDEV(mfc_l),
	&SYSMMU_PLATDEV(mfc_r),
	&SYSMMU_PLATDEV(tv),
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&SYSMMU_PLATDEV(is_isp),
	&SYSMMU_PLATDEV(is_drc),
	&SYSMMU_PLATDEV(is_fd),
	&SYSMMU_PLATDEV(is_cpu),
#endif
#endif
#ifdef CONFIG_ION_EXYNOS
	&exynos_device_ion,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
	&s5p_device_jpeg,
#endif
	&samsung_asoc_dma,
#ifndef CONFIG_SND_SOC_SAMSUNG_USE_DMA_WRAPPER
	&samsung_asoc_idma,
#endif
#if defined(CONFIG_CHARGER_MAX8922_U1)
	&max8922_device_charger,
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI) || defined(CONFIG_VIDEO_DRIME4_SPI) || defined(CONFIG_VIDEO_M9MO_SPI)
	&exynos_device_spi1,
#endif
#if defined(CONFIG_LINK_DEVICE_SPI)
	&exynos_device_spi2,
#elif defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	&exynos_device_spi2,
#elif defined(CONFIG_ISDBT)
	&exynos_device_spi2,
#elif defined(CONFIG_LINK_DEVICE_PLD)
	&exynos_device_spi2,
#endif
#endif
#ifdef CONFIG_BT_BCM4330
	&bcm4330_bluetooth_device,
#endif
#ifdef CONFIG_BT_BCM4334
	&bcm4334_bluetooth_device,
#endif
#ifdef CONFIG_BT_BCM4335
	&bcm4335_bluetooth_device,
#endif
#ifdef CONFIG_S5P_DEV_ACE
	&s5p_device_ace,
#endif
	&exynos4_busfreq,
#ifdef CONFIG_USB_HOST_NOTIFY
	&host_notifier_device,
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#endif
#ifdef CONFIG_LEDS_GPIO
	&leds_gpio_device,
#endif
#ifdef CONFIG_CORESIGHT_ETM
	&coresight_etb_device,
	&coresight_tpiu_device,
	&coresight_funnel_device,
	&coresight_etm_device,
#endif
};

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
/* below temperature base on the celcius degree */
struct s5p_platform_tmu midas_tmu_data __initdata = {
	.ts = {
		.stop_1st_throttle  = 78,
		.start_1st_throttle = 80,
		.stop_2nd_throttle  = 87,
		.start_2nd_throttle = 103,
		.start_tripping	    = 110, /* temp to do tripping */
		.start_emergency    = 120, /* To protect chip,forcely kernel panic */
		.stop_mem_throttle  = 80,
		.start_mem_throttle = 85,
		.stop_tc  = 13,
		.start_tc = 10,
	},
	.cpufreq = {
		.limit_1st_throttle  = 800000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
	},
	.temp_compensate = {
		.arm_volt = 925000, /* vdd_arm in uV for temperature compensation */
		.bus_volt = 900000, /* vdd_bus in uV for temperature compensation */
		.g3d_volt = 900000, /* vdd_g3d in uV for temperature compensation */
	},
};
#endif

#if defined CONFIG_USB_OHCI_S5P
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
static int __init s5p_ohci_device_initcall(void)
{
	return platform_device_register(&s5p_device_ohci);
}
late_initcall(s5p_ohci_device_initcall);
#endif
#endif

#if defined CONFIG_USB_EHCI_S5P
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
static int __init s5p_ehci_device_initcall(void)
{
	return platform_device_register(&s5p_device_ehci);
}
late_initcall(s5p_ehci_device_initcall);
#endif
#endif

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GD2)
#ifdef CONFIG_HDMI_CONTROLLED_BY_EXT_IC
	.ext_ic_control = hdmi_ext_ic_control_gc1,
#endif
#endif

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};

#if (defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD))\
		&& defined(CONFIG_HDMI_TX_STRENGTH)
static u8 gc1_hdmi_tx_val[5] = {0x00, 0x1f, 0x00, 0x00, 0x00};
static struct s5p_tx_tuning gc1_tx_tuning = {
	/* tx_ch: bit4 - Pre-emp
	 *	  bit3 - Amp all
	 *	  bit2 - fine amp ch0
	 *	  bit1 - fine amp ch1
	 *	  bit0 - fine amp ch2
	 */
	.tx_ch = 0x08,
	.tx_val = &gc1_hdmi_tx_val[0],
};
static struct s5p_platform_tvout hdmi_tvout_data __initdata = {
	.tx_tune = &gc1_tx_tuning,
};
#endif
#endif

#if defined(CONFIG_CMA)
static unsigned long fbmem_start;
static unsigned long fbmem_size;
static int __init early_fbmem(char *p)
{
	char *endp;

	if (!p)
		return -EINVAL;

	fbmem_size = memparse(p, &endp);
	if (*endp == '@')
		fbmem_start = memparse(endp + 1, &endp);

	return endp > p ? 0 : -EINVAL;
}
early_param("fbmem", early_fbmem);

static void __init exynos4_reserve_mem(void)
{
	static struct cma_region regions[] = {
#ifdef CONFIG_EXYNOS_C2C
		{
			.name = "c2c_shdmem",
			.size = C2C_SHAREDMEM_SIZE,
			{
				.alignment = C2C_SHAREDMEM_SIZE,
			},
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		{
			.name = "fimc_is",
			.size = CONFIG_VIDEO_EXYNOS_MEMSIZE_FIMC_IS * SZ_1K,
			{
				.alignment = 1 << 26,
			},
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
		{
			.name = "fimd",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
			{
				.alignment = 1 << 20,
			},
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			{
				.alignment = 1 << 20,
			},
#if defined(CONFIG_MACH_GD2)
			.start = 0x5B400000,
#else
			.start = 0
#endif
		},
#endif
#if !defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0)
		{
			.name = "mfc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0,
		},
#endif
#if !defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE)
		{
			.name	= "ion",
			.size	= CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
		{
			.name = "mfc",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
		{
			.name		= "b2",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "b1",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "fw",
			.size		= 1 << 20,
			{ .alignment	= 128 << 10 },
		},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		{
			.name = "srp",
			.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
		{
			.name = "fimg2d",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
#ifndef CONFIG_USE_FIMC_CMA
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
			.start = 0x5e800000,
#elif defined(CONFIG_MACH_GD2)
			.start = 0x5d700000,
#elif defined(CONFIG_MACH_WATCH)
#else
			.start = 0x65c00000,
#endif
		},
#endif
#endif

#if (CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 > 0)
#ifndef CONFIG_USE_FIMC_CMA
		{
			.name = "fimc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
#if defined(CONFIG_MACH_GD2)
			.start = 0x60500000,
#endif
		},
#endif
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
		{
			.name = "mfc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
			{
				.alignment = 1 << 26,
			},
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
			.start = 0x5e000000,
#elif defined(CONFIG_MACH_GD2)
			.start = 0x5B000000,
#else
			.start = 0x64000000,
#endif
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_NORMAL
		{
			.name = "mfc-normal",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_NORMAL * SZ_1K,
#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
			.start = 0x5e000000,
#elif defined(CONFIG_MACH_GD2)
			.start = 0x5B000000,
#elif defined(CONFIG_MACH_ZEST)
			.start = 0x60800000,
#elif defined(CONFIG_MACH_WATCH)
#else
			.start = 0x64000000,
#endif
		},
#endif
		{
			.size = 0
		},
	};

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	static struct cma_region regions_secure[] = {
#if !defined(CONFIG_DMA_CMA)
#ifdef CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE
		{
			.name	= "ion",
			.size	= CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
#if defined(CONFIG_MACH_GD2)
			.start	= 0x53200000,
#endif
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE
		{
			.name = "mfc-secure",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE * SZ_1K,
#if defined(CONFIG_MACH_GD2)
			.start = 0x50100000,
#endif
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
		},
#else /*defined(CONFIG_DMA_CMA)*/
#if defined(CONFIG_USE_MFC_CMA)
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_ZEST) || defined(CONFIG_MACH_WATCH)
#ifdef CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE
		{
			.name = "ion",
			.size = CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
#if defined(CONFIG_MACH_ZEST)
			.start = 0x5B200000,
#else
			.start = 0x5F200000,
#endif
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE
		{
			.name = "mfc-secure",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE * SZ_1K,
#if defined(CONFIG_MACH_ZEST)
			.start = 0x58100000,
#else
			.start = 0x5C100000,
#endif
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
#if defined(CONFIG_MACH_ZEST)
			.start = 0x58000000,
#else
			.start = 0x5C000000,
#endif
		},
#elif defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
#ifdef CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE
		{
			.name = "ion",
			.size = CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
			.start = 0x53300000,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE
		{
			.name = "mfc-secure",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE * SZ_1K,
			.start = 0x50200000,
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
			.start = 0x50000000,
		},
#endif
#else
#ifdef CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE
		{
			.name   = "ion",
			.size   = CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE
		{
			.name = "mfc-secure",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC_SECURE * SZ_1K,
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
		},
#endif
#endif
		{
			.size = 0
		},
	};
#else /* !CONFIG_EXYNOS_CONTENT_PATH_PROTECTION */
	struct cma_region *regions_secure = NULL;
#endif

	static const char map[] __initconst =
#ifdef CONFIG_EXYNOS_C2C
		"samsung-c2c=c2c_shdmem;"
#endif
		"s3cfb.0=fimd;exynos4-fb.0=fimd;samsung-pd.1=fimd;"
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
		"exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_ION_EXYNOS
		"ion-exynos=ion;"
#endif
#ifdef CONFIG_VIDEO_MFC5X
		"s3c-mfc/A=mfc0,mfc-secure;"
		"s3c-mfc/B=mfc1,mfc-normal;"
		"s3c-mfc/AB=mfc;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		"s5p-mfc/f=fw;"
		"s5p-mfc/a=b1;"
		"s5p-mfc/b=b2;"
#endif
		"samsung-rp=srp;"
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		"exynos4-fimc-is=fimc_is;"
#endif
		"s5p-fimg2d=fimg2d;"
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
		"s5p-smem/sectbl=sectbl;"
#endif
		"s5p-smem/mfc=mfc-secure;"
		"s5p-smem/fimc=ion;"
		"s5p-smem/mfc-shm=mfc-normal;"
		"s5p-smem/fimd=fimd;"
		"s5p-smem/fimc0=fimc0";

	int i;

	s5p_cma_region_reserve(regions, regions_secure, 0, map);

	pr_err("[CMA] %s: regions\n", __func__);
	for (i = 0; i < ARRAY_SIZE(regions); i++) {
		if (regions[i].size == 0)
			break;
		pr_err("[CMA] %s: regions[%d] 0x%08X + 0x%07X (%s)\n",
			__func__, i, regions[i].start, regions[i].size,
			regions[i].name);
	}

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	pr_err("[CMA] %s: regions_secure\n", __func__);
	for (i = 0; i < ARRAY_SIZE(regions_secure); i++) {
		if (regions_secure[i].size == 0)
			break;
		pr_err("[CMA] %s: regions_secure[%d] 0x%08X + 0x%07X (%s)\n",
			__func__, i, regions_secure[i].start,
			regions_secure[i].size, regions_secure[i].name);
	}
#endif

	if (!fbmem_start || !fbmem_size)
		return;

	for (i = 0; i < ARRAY_SIZE(regions); i++) {
		if (regions[i].name && !strcmp(regions[i].name, "fimd")) {
			memcpy(phys_to_virt(regions[i].start), phys_to_virt(fbmem_start), fbmem_size * SZ_1K);
			printk(KERN_INFO "Bootloader sent 'fbmem' : %08X\n", (u32)fbmem_start);
			break;
		}
	}
}
#else
static inline void exynos4_reserve_mem(void)
{
}
#endif

#ifdef CONFIG_BACKLIGHT_PWM
/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4212_bl_gpio_info = {
	.no = EXYNOS4_GPD0(1),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4212_bl_data = {
	.pwm_id = 1,
#ifdef CONFIG_FB_S5P_LMS501KF03
	.pwm_period_ns = 1000,
#endif
};
#endif

static void __init midas_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk4212_uartcfgs, ARRAY_SIZE(smdk4212_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
	exynos4_reserve_mem();
#endif

	/* as soon as INFORM6 is visible, sec_debug is ready to run */
	sec_debug_init();
}

static void __init exynos_sysmmu_init(void)
{
	ASSIGN_SYSMMU_POWERDOMAIN(fimc0, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc1, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc2, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc3, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(jpeg, &exynos4_device_pd[PD_CAM].dev);

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
#endif
	ASSIGN_SYSMMU_POWERDOMAIN(tv, &exynos4_device_pd[PD_TV].dev);
#ifdef CONFIG_VIDEO_FIMG2D
	sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#ifdef CONFIG_VIDEO_MFC5X
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
#ifdef CONFIG_VIDEO_FIMC
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s3c_device_fimc0.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s3c_device_fimc1.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s3c_device_fimc2.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s3c_device_fimc3.dev);
#endif
#ifdef CONFIG_VIDEO_TVOUT
	sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_tvout.dev);
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
	sysmmu_set_owner(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
#ifdef CONFIG_FB_S5P_SYSMMU
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimd0).dev, &s3c_device_fb.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	ASSIGN_SYSMMU_POWERDOMAIN(is_isp, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_drc, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_fd, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_cpu, &exynos4_device_pd[PD_ISP].dev);

	sysmmu_set_owner(&SYSMMU_PLATDEV(is_isp).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_drc).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_fd).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_cpu).dev,
		&exynos4_device_fimc_is.dev);
#endif
}

#ifdef CONFIG_FB_S5P_EXTDSP
struct platform_device s3c_device_extdsp = {
	.name		= "s3cfb_extdsp",
	.id		= 0,
};

static struct s3cfb_extdsp_lcd dummy_buffer = {
	.width = 1920,
	.height = 1080,
	.bpp = 16,
};

static struct s3c_platform_fb default_extdsp_data __initdata = {
	.hw_ver		= 0x70,
	.nr_wins	= 1,
	.default_win	= 0,
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
	.lcd		= &dummy_buffer
};

void __init s3cfb_extdsp_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_extdsp_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;
		s3c_device_extdsp.dev.platform_data = npd;
	}
}
#endif

static inline int need_i2c5(void)
{
#if defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M0)
	return system_rev != 3;
#else
	return 1;
#endif
}

#if defined(CONFIG_FELICA)
static void felica_setup(void)
{
#if defined(CONFIG_MACH_M3_JPN_DCM)
	if (system_rev < 3) {
		/* I2C SDA GPY2[4] */
		gpio_request(FELICA_GPIO_I2C_SDA_R1, FELICA_GPIO_I2C_SDA_NAME);
		s3c_gpio_setpull(FELICA_GPIO_I2C_SDA_R1, S3C_GPIO_PULL_DOWN);
		gpio_free(FELICA_GPIO_I2C_SDA_R1);

		/* I2C SCL GPY2[5] */
		gpio_request(FELICA_GPIO_I2C_SCL_R1, FELICA_GPIO_I2C_SCL_NAME);
		s3c_gpio_setpull(FELICA_GPIO_I2C_SCL_R1, S3C_GPIO_PULL_DOWN);
		gpio_free(FELICA_GPIO_I2C_SCL_R1);
	} else {
		/* I2C SDA GPY2[4] */
		gpio_request(FELICA_GPIO_I2C_SDA, FELICA_GPIO_I2C_SDA_NAME);
		s3c_gpio_setpull(FELICA_GPIO_I2C_SDA, S3C_GPIO_PULL_DOWN);
		gpio_free(FELICA_GPIO_I2C_SDA);

		/* I2C SCL GPY2[5] */
		gpio_request(FELICA_GPIO_I2C_SCL, FELICA_GPIO_I2C_SCL_NAME);
		s3c_gpio_setpull(FELICA_GPIO_I2C_SCL, S3C_GPIO_PULL_DOWN);
		gpio_free(FELICA_GPIO_I2C_SCL);
	}
#elif defined(CONFIG_MACH_T0_JPN_LTE_DCM)
	/* I2C SDA GPY2[4] */
	gpio_request(FELICA_GPIO_I2C_SDA, FELICA_GPIO_I2C_SDA_NAME);
	s3c_gpio_setpull(FELICA_GPIO_I2C_SDA, S3C_GPIO_PULL_DOWN);
	gpio_free(FELICA_GPIO_I2C_SDA);

	/* I2C SCL GPY2[5] */
	gpio_request(FELICA_GPIO_I2C_SCL, FELICA_GPIO_I2C_SCL_NAME);
	s3c_gpio_setpull(FELICA_GPIO_I2C_SCL, S3C_GPIO_PULL_DOWN);
	gpio_free(FELICA_GPIO_I2C_SCL);
#endif
	/* PON GPL2[7] */
	gpio_request(FELICA_GPIO_PON, FELICA_GPIO_PON_NAME);
	s3c_gpio_setpull(FELICA_GPIO_PON, S3C_GPIO_PULL_DOWN);
	s3c_gpio_cfgpin(FELICA_GPIO_PON, S3C_GPIO_SFN(1)); /* OUTPUT */
	gpio_free(FELICA_GPIO_PON);

	/* RFS GPL2[6] */
	gpio_request(FELICA_GPIO_RFS, FELICA_GPIO_RFS_NAME);
	s3c_gpio_setpull(FELICA_GPIO_RFS, S3C_GPIO_PULL_DOWN);
	gpio_direction_input(FELICA_GPIO_RFS);
	gpio_free(FELICA_GPIO_RFS);

	/* INT GPX1[7] = WAKEUP_INT1[7] */
	gpio_request(FELICA_GPIO_INT, FELICA_GPIO_INT_NAME);
	s3c_gpio_setpull(FELICA_GPIO_INT, S3C_GPIO_PULL_DOWN);
	s5p_register_gpio_interrupt(FELICA_GPIO_INT);
	gpio_direction_input(FELICA_GPIO_INT);
	irq_set_irq_type(gpio_to_irq(FELICA_GPIO_INT), IRQF_TRIGGER_FALLING);
	s3c_gpio_cfgpin(FELICA_GPIO_INT, S3C_GPIO_SFN(0xF)); /* EINT */
	gpio_free(FELICA_GPIO_INT);
}
#endif
#ifdef CONFIG_S3C_DEV_I2C3
static void touch_i2c3_cfg_gpio(struct platform_device *dev)
{
	s3c_gpio_cfgall_range(GPIO_TSP_SDA_18V, 2,
		S3C_GPIO_SFN(3), S3C_GPIO_PULL_NONE);
};

static struct s3c2410_platform_i2c touch_i2c3_platdata __initdata = {
	.bus_num	= 3,
	.flags		= 0,
	.slave_addr = 0x10,
	.frequency	= 400*1000,
	.sda_delay	= 100,
	.cfg_gpio	= touch_i2c3_cfg_gpio,
};
#endif

static void __init midas_machine_init(void)
{
#ifdef CONFIG_BUSFREQ_OPP
	struct clk *ppmu_clk = NULL;
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI) || defined(CONFIG_VIDEO_DRIME4_SPI) || defined(CONFIG_VIDEO_M9MO_SPI)
	unsigned int gpio;
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &exynos_device_spi1.dev;
#endif
#if defined(CONFIG_LINK_DEVICE_SPI) \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) \
	|| defined(CONFIG_ISDBT) || defined(CONFIG_LINK_DEVICE_PLD)
	struct device *spi2_dev = &exynos_device_spi2.dev;
#endif
#endif

	/*
	  * prevent 4x12 ISP power off problem
	  * ISP_SYS Register has to be 0 before ISP block power off.
	  */
	__raw_writel(0x0, S5P_CMU_RESET_ISP_SYS);

#if defined(CONFIG_MACH_M3_JPN_DCM)
	if (system_rev < 3) {
		i2c10_platdata.sda_pin = GPIO_MSENSOR_SDA_18V_R1;
		i2c10_platdata.scl_pin = GPIO_MSENSOR_SCL_18V_R1;
#ifdef CONFIG_BATTERY_WPC_CHARGER
		max77693_charger_pdata.vbus_irq_gpio = GPIO_V_BUS_INT_R1;
#endif
		i2c30_gpio_platdata.sda_pin = FELICA_GPIO_I2C_SDA_R1;
		i2c30_gpio_platdata.scl_pin = FELICA_GPIO_I2C_SCL_R1;
	}
#endif

#if defined(CONFIG_MACH_GD2)
	if (system_rev >= 0x03) {
		gpio_i2c_data14.sda_pin = GPIO_FUEL_SDA_02;
		gpio_i2c_data14.scl_pin = GPIO_FUEL_SCL_02;
		pr_info("%s: fuel_scl and fuel_sda changed for rev(0x%x)\n",
			__func__, system_rev);
	}
#endif

	/* initialise the gpios */
	midas_config_gpio_table();
	exynos4_sleep_gpio_table_set = midas_config_sleep_gpio_table;

	midas_power_init();

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

#if !defined(CONFIG_MACH_GC2PD)
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif

#if defined(CONFIG_S3C_DEV_I2C2) \
	&& !defined(CONFIG_MACH_T0_EUR_OPEN) \
	&& !defined(CONFIG_MACH_T0_CHN_CU_DUOS) \
	&& !defined(CONFIG_MACH_T0_CHN_OPEN_DUOS) \
	&& !defined(CONFIG_MACH_T0_CHN_CMCC) \
	&& !defined(CONFIG_MACH_T0_CHN_OPEN) \
	&& !defined(CONFIG_MACH_GD2)
	s3c_i2c2_set_platdata(NULL);
#endif
#ifdef CONFIG_S3C_DEV_I2C3
	s3c_i2c3_set_platdata(&touch_i2c3_platdata);
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
	midas_tsp_init();
#ifndef CONFIG_TOUCHSCREEN_MELFAS_GC
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
	midas_tsp_set_lcdtype(lcdtype);
#endif
#endif
#endif

#ifdef CONFIG_LEDS_AAT1290A
	platform_device_register(&s3c_device_aat1290a_led);
#endif
#ifdef CONFIG_LEDS_IR
	platform_device_register(&s3c_device_ir_led);
#endif
#ifdef CONFIG_LEDS_IR_PWM
	platform_device_register(&s3c_device_ir_led_pwm);
#endif
#ifdef CONFIG_S3C_DEV_I2C4
	s3c_i2c4_set_platdata(NULL);
#if defined(CONFIG_MACH_M0)
	if (system_rev == 3) {
		i2c_register_board_info(4, i2c_devs4_max77693,
			ARRAY_SIZE(i2c_devs4_max77693));
	}
#endif
#endif /* CONFIG_S3C_DEV_I2C4 */
	midas_sound_init();

#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
	i2c_register_board_info(5, i2c_devs5,
			ARRAY_SIZE(i2c_devs5));
#endif
#ifdef CONFIG_S3C_DEV_I2C5
#if defined(CONFIG_MACH_T0_EUR_OPEN) || \
	defined(CONFIG_MACH_T0_CHN_OPEN) || \
	defined(CONFIG_MACH_IPCAM)
	s3c_i2c5_set_platdata(NULL);
#else
	if (need_i2c5()) {
		s3c_i2c5_set_platdata(&default_i2c5_data);
		i2c_register_board_info(5, i2c_devs5,
			ARRAY_SIZE(i2c_devs5));
	}
#endif
#endif

#ifdef CONFIG_DC_MOTOR
	dc_motor_init();
#endif

#if defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)
	s3c_i2c5_cfg_gpio_gc1();
#endif

#if defined(CONFIG_INPUT_WACOM)
	midas_wacom_init();
#endif

#ifdef CONFIG_S3C_DEV_I2C6
#if defined(CONFIG_MACH_T0_EUR_OPEN) || \
	defined(CONFIG_MACH_T0_CHN_OPEN) || \
	defined(CONFIG_MACH_GD2)
	s3c_i2c6_set_platdata(&default_i2c6_data);
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#elif defined(CONFIG_MACH_ZEST)
	s3c_i2c6_set_platdata(NULL);
#endif
#endif

#if defined(CONFIG_MACH_GC1)
	s3c_i2c7_set_platdata(NULL);
	if (system_rev < 1) {
		i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));
	} else {
		i2c_register_board_info(7, i2c_devs7_s5m,
						ARRAY_SIZE(i2c_devs7_s5m));
	}
#else
	s3c_i2c7_set_platdata(NULL);
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));
#endif
#if !defined(CONFIG_MACH_GD2)
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
	touchkey_init_hw();
#endif
#ifdef CONFIG_KEYBOARD_TC370L_TOUCHKEY
	tc370_init();
#endif
	i2c_register_board_info(8, i2c_devs8_emul, ARRAY_SIZE(i2c_devs8_emul));
#if !defined(CONFIG_MACH_GC1) && !defined(CONFIG_LEDS_AAT1290A)
	gpio_request(GPIO_3_TOUCH_INT, "3_TOUCH_INT");
	s5p_register_gpio_interrupt(GPIO_3_TOUCH_INT);
#endif
#endif
	i2c_register_board_info(9, i2c_devs9_emul, ARRAY_SIZE(i2c_devs9_emul));

	i2c_register_board_info(10, i2c_devs10_emul,
				ARRAY_SIZE(i2c_devs10_emul));

	i2c_register_board_info(11, i2c_devs11_emul,
				ARRAY_SIZE(i2c_devs11_emul));

#if defined(CONFIG_PN65N_NFC) || defined(CONFIG_PN547_NFC)\
|| defined(CONFIG_BCM2079X_NFC)
#if defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_ZEST) || defined(CONFIG_MACH_GC2PD) \
	|| defined(CONFIG_MACH_WATCH)
	i2c_register_board_info(12, i2c_devs12_emul,
				ARRAY_SIZE(i2c_devs12_emul));
#endif
#endif

#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
	/* max17047 fuel gauge */
	i2c_register_board_info(14, i2c_devs14_emul,
				ARRAY_SIZE(i2c_devs14_emul));
#endif

#if defined(CONFIG_CHARGER_TSU8111)
	exynos_watch_charger_init();
#endif

#ifdef CONFIG_SAMSUNG_MHL
	printk(KERN_INFO "%s() register sii9234 driver\n", __func__);

	i2c_register_board_info(15, i2c_devs15_emul,
				ARRAY_SIZE(i2c_devs15_emul));
	i2c_register_board_info(16, i2c_devs16_emul,
				ARRAY_SIZE(i2c_devs16_emul));
#endif

#if !defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
#if defined(CONFIG_MFD_MAX77693)
#if defined(CONFIG_SAMSUNG_MUIC)
#if defined(CONFIG_MACH_T0) && defined(CONFIG_TARGET_LOCALE_KOR)
	if (system_rev >= 9)
		max77693_haptic_pdata.motor_en = motor_en;
#endif
#if defined(CONFIG_MACH_T0_JPN_LTE_DCM)
	if (system_rev >= 12)
		max77693_haptic_pdata.motor_en = motor_en;
#endif
#if defined(CONFIG_MACH_BAFFIN_KOR_SKT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_KT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	if (system_rev >= 2)
		max77693_haptic_pdata.motor_en = motor_en;
#endif
#endif /* CONFIG_SAMSUNG_MUIC */

	i2c_register_board_info(17, i2c_devs17_emul,
				ARRAY_SIZE(i2c_devs17_emul));
#endif /* CONFIG_MFD_MAX77693 */
#endif /* !CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#ifdef CONFIG_MACH_GD2
	i2c_register_board_info(18, i2c_devs18_emul,
				ARRAY_SIZE(i2c_devs18_emul));
#endif

#if defined(CONFIG_LEDS_AN30259A) || defined(CONFIG_LEDS_LP5521)
	i2c_register_board_info(21, i2c_devs21_emul,
				ARRAY_SIZE(i2c_devs21_emul));
#endif
#if defined(CONFIG_BARCODE_EMUL_ICE4) || defined(CONFIG_ICE4_FPGA)
	i2c_register_board_info(22, i2c_devs22_emul,
				ARRAY_SIZE(i2c_devs22_emul));
#endif

#ifdef CONFIG_KEYBOARD_ADP5587
	qwertykey_init();
	i2c_register_board_info(25, i2c_devs25_emul,
				ARRAY_SIZE(i2c_devs25_emul));
#endif

#if defined(CONFIG_FELICA)
	i2c_register_board_info(30, i2c_devs30, ARRAY_SIZE(i2c_devs30));
#endif

#if defined(GPIO_OLED_DET)
	if (unlikely(gpio_request(GPIO_OLED_DET, "OLED_DET")))
		pr_err("Request GPIO_OLED_DET is failed\n");
	else {
		s5p_register_gpio_interrupt(GPIO_OLED_DET);
		gpio_free(GPIO_OLED_DET);
	}
#endif

#if defined(GPIO_VGH_DET)
	if (unlikely(gpio_request(GPIO_VGH_DET, "VGH_DET")))
		pr_err("Request GPIO_VGH_DET is failed\n");
	else {
		s5p_register_gpio_interrupt(GPIO_VGH_DET);
		gpio_free(GPIO_VGH_DET);
	}
#endif

#if defined(GPIO_ERR_FG)
	if (unlikely(gpio_request(GPIO_ERR_FG, "OLED_DET")))
		pr_err("Request GPIO_ERR_FG is failed\n");
	else {
		s5p_register_gpio_interrupt(GPIO_ERR_FG);
		gpio_free(GPIO_ERR_FG);
	}
#endif

#ifdef CONFIG_FB_S5P
#ifdef CONFIG_FB_S5P_LMS501KF03
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&lms501kf03_data);
#endif
#if defined(CONFIG_FB_S5P_GD2EVF)
	gd2evf_fb_init();
#endif
#if defined(CONFIG_FB_S5P_MIPI_DSIM)
	mipi_fb_init();
#elif defined(CONFIG_FB_S5P_LD9040)
	ld9040_fb_init();
#elif defined(CONFIG_BACKLIGHT_PWM)
	samsung_bl_set(&smdk4212_bl_gpio_info, &smdk4212_bl_data);
#endif
	s3cfb_set_platdata(&fb_platform_data);
#if defined(CONFIG_FB_S5P_GD2EVF)
	mipi_fb_platdata_addr = s3c_device_fb.dev.platform_data;
#endif

#ifdef CONFIG_EXYNOS_DEV_PD
	s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_USB_EHCI_S5P
	smdk4212_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
	smdk4212_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
	smdk4212_usbgadget_init();
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	exynos4_fimc_is_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
	exynos4_device_fimc_is.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif
#endif
#ifdef CONFIG_EXYNOS4_DEV_MSHC
	s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdk4212_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdk4212_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdk4212_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s3c_sdhci3_set_platdata(&smdk4212_hsmmc3_pdata);
#endif

	midas_camera_init();

#ifdef CONFIG_FB_S5P_EXTDSP
	s3cfb_extdsp_set_platdata(&default_extdsp_data);
#endif

#if defined(CONFIG_FELICA)
	felica_setup();
#endif /* CONFIG_FELICA */

#if defined(CONFIG_VIDEO_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#if (defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD)) \
		&& defined(CONFIG_HDMI_TX_STRENGTH)
	s5p_hdmi_tvout_set_platdata(&hdmi_tvout_data);
#endif
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
	exynos4_device_pd[PD_TV].dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_ION_EXYNOS
	exynos_ion_set_platdata();
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
	exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 200 * MHZ);
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc");
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

	brcm_wlan_init();

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&midas_tmu_data);
#endif

	exynos_sysmmu_init();

	platform_add_devices(midas_devices, ARRAY_SIZE(midas_devices));

#if defined(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)
	exynos4_exynos4212_adc_init();
#endif /* CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
	exynos4_exynos4212_mfd_init();
#endif /* CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_MUIC_ADD_PLATFORM_DEVICE)
	exynos4_exynos4212_muic_init();
#endif /* CONFIG_MUIC_ADD_PLATFORM_DEVICE */

#ifdef CONFIG_S3C_ADC
#if defined(CONFIG_MACH_GC1) || \
	defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_BAFFIN) || \
	defined(CONFIG_MACH_GD2) || \
	defined(CONFIG_MACH_GC2PD) || \
	defined(CONFIG_MACH_IPCAM) || \
	defined(CONFIG_MACH_WATCH)
	platform_device_register(&s3c_device_adc);
#else
	if (system_rev != 3)
		platform_device_register(&s3c_device_adc);
#endif

#if defined(CONFIG_MACH_GC1)
	gc1_jack_init();
#endif

#endif
#if defined(CONFIG_BATTERY_SAMSUNG)
	platform_device_register(&samsung_device_battery);
#endif
#ifdef CONFIG_SEC_THERMISTOR
	platform_device_register(&sec_device_thermistor);
#endif
#ifdef CONFIG_SEC_SUBTHERMISTOR
	platform_device_register(&sec_device_subthermistor);
#endif

#ifdef CONFIG_KEYBOARD_GPIO
#if defined(CONFIG_MACH_M0_CTC)
	midas_gpiokeys_platform_data.buttons = m0_buttons;
	midas_gpiokeys_platform_data.nbuttons = ARRAY_SIZE(m0_buttons);
#elif defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M0)
	if (system_rev != 3 && system_rev >= 1) {
		midas_gpiokeys_platform_data.buttons = m0_buttons;
		midas_gpiokeys_platform_data.nbuttons = ARRAY_SIZE(m0_buttons);
	}
#endif
	/* Above logic is too complex. Let's override whatever the
	   result is... */

#if defined(CONFIG_MACH_M0)
#if defined(CONFIG_MACH_M0_CHNOPEN) || defined(CONFIG_MACH_M0_HKTW)
	{
#else
	if (system_rev >= 11) {
#endif
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_NONE);
		midas_gpiokeys_platform_data.buttons = m0_rev11_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(m0_rev11_buttons);
	}
#elif defined(CONFIG_TARGET_LOCALE_KOR) && \
	(defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1))
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_NONE);
		midas_gpiokeys_platform_data.buttons = c1_rev04_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(c1_rev04_buttons);

#elif defined(CONFIG_MACH_C1_USA_ATT)
	if (system_rev >= 7) {
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
		midas_gpiokeys_platform_data.buttons = m0_rev11_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(m0_rev11_buttons);
	}
#elif defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_M3)
	midas_gpiokeys_platform_data.buttons = t0_buttons;
	midas_gpiokeys_platform_data.nbuttons = ARRAY_SIZE(t0_buttons);
#if defined(CONFIG_MACH_M3_USA_TMO)
	if (system_rev == 0) {
		midas_gpiokeys_platform_data.buttons = m3_uas_tmo_00_buttons;
		midas_gpiokeys_platform_data.nbuttons =
				ARRAY_SIZE(m3_uas_tmo_00_buttons);
	} else {
		midas_gpiokeys_platform_data.buttons = t0_buttons;
		midas_gpiokeys_platform_data.nbuttons = ARRAY_SIZE(t0_buttons);
	}
#endif
#elif defined(CONFIG_MACH_BAFFIN_KOR_SKT) || \
	defined(CONFIG_MACH_BAFFIN_KOR_KT)
	if (system_rev >= 0x4) {
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
		midas_gpiokeys_platform_data.buttons = baffin_kor_rev06_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(baffin_kor_rev06_buttons);
	} else {
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
		midas_gpiokeys_platform_data.buttons = baffin_kor_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(baffin_kor_buttons);
	}
#elif defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	if (system_rev >= 0x5) {
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
		midas_gpiokeys_platform_data.buttons = baffin_kor_rev06_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(baffin_kor_rev06_buttons);
	} else {
		s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
		midas_gpiokeys_platform_data.buttons = baffin_kor_buttons;
		midas_gpiokeys_platform_data.nbuttons =
			ARRAY_SIZE(baffin_kor_buttons);
	}
#elif defined(CONFIG_MACH_SUPERIOR_KOR_SKT)
	s3c_gpio_setpull(GPIO_OK_KEY_ANDROID, S3C_GPIO_PULL_UP);
	midas_gpiokeys_platform_data.buttons = baffin_kor_buttons;
	midas_gpiokeys_platform_data.nbuttons =
		ARRAY_SIZE(baffin_kor_buttons);
#endif

#ifdef CONFIG_MACH_GC1
	/*for emul type*/
	if (system_rev < 2) {
		printk(KERN_DEBUG"[KEYS] rev %x. switch wide/tele gpio\n",
			system_rev);
		gpio_direction_output(GPIO_TOP_PCB_PWREN, 1);
		midas_buttons[3].gpio = GPIO_WIDE_KEY;
		midas_buttons[4].gpio = GPIO_TELE_KEY;
		midas_buttons[5].code = KEY_RECORD;
		midas_buttons[5].gpio = GPIO_RECORD_KEY;
		midas_buttons[6].code = KEY_PLAY;
		midas_buttons[6].gpio = GPIO_PLAY_KEY;
	}
	/*strobe open/close*/
	gpio_request(STR_PU_DET_18V, "STR_PU_DET_18V");
	s3c_gpio_cfgpin(STR_PU_DET_18V, S3C_GPIO_SFN(0xf));
	s5p_register_gpio_interrupt(STR_PU_DET_18V);
	gpio_direction_input(STR_PU_DET_18V);
#endif
#if defined(CONFIG_MACH_IPCAM)
	s3c_gpio_setpull(GPIO_RESET_KEY, S3C_GPIO_PULL_UP);
	s5p_register_gpio_interrupt(GPIO_RESET_KEY);
#endif
	platform_device_register(&midas_keypad);
#endif	/* CONFIG_KEYBOARD_GPIO */

#if defined(CONFIG_INPUT_SEC_KEYS)
#if defined(CONFIG_MACH_GD2)
	s5p_register_gpio_interrupt(GPIO_STR_UP);
	s5p_register_gpio_interrupt(GPIO_CAMERA_IFUNC);
#endif	/* CONFIG_MACH_GD2 */
#if defined(CONFIG_MACH_GC2PD) || defined(CONFIG_MACH_GD2)
	sec_key_init();
#endif
	platform_device_register(&gd2_keys);
#endif

#if defined(CONFIG_SEC_DEBUG)
	platform_device_register(&input_debug);
#endif

#if defined(CONFIG_SF2_NFC_TAG)
      platform_device_register(&nfc_tag_devdata);
#endif

#if defined(CONFIG_S3C_DEV_I2C5)
	if (need_i2c5())
		platform_device_register(&s3c_device_i2c5);
#endif

#if defined(CONFIG_PN65N_NFC) || defined(CONFIG_PN547_NFC)\
	|| defined(CONFIG_BCM2079X_NFC)
#if defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_ZEST) || defined(CONFIG_MACH_GC2PD) \
	|| defined(CONFIG_MACH_WATCH)
	platform_device_register(&s3c_device_i2c12);
#endif
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI) || defined(CONFIG_VIDEO_DRIME4_SPI) || defined(CONFIG_VIDEO_M9MO_SPI)
	sclk = clk_get(spi1_dev, "dout_spi1");
	if (IS_ERR(sclk))
		dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi1_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi1_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
		       prnt->name, sclk->name);

	clk_set_rate(sclk, 50 * 1000 * 1000); /*25MHz*/

	clk_put(sclk);
	clk_put(prnt);

#if defined(CONFIG_MACH_GC2PD) && defined(CONFIG_VIDEO_M9MO_SPI)
	exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				ARRAY_SIZE(spi1_csi));
#else
	if (!gpio_request(EXYNOS4_GPB(5), "SPI_CS1")) {
		gpio_direction_output(EXYNOS4_GPB(5), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(5), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				    ARRAY_SIZE(spi1_csi));
	}
#endif

	for (gpio = EXYNOS4_GPB(4); gpio < EXYNOS4_GPB(8); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
#endif

#if defined(CONFIG_LINK_DEVICE_SPI) \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) \
	|| defined(CONFIG_ISDBT) || defined(CONFIG_LINK_DEVICE_PLD)
	sclk = NULL;
	prnt = NULL;

	sclk = clk_get(spi2_dev, "dout_spi2");
	if (IS_ERR(sclk))
		dev_err(spi2_dev, "failed to get sclk for SPI-2\n");
	prnt = clk_get(spi2_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi2_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
		       prnt->name, sclk->name);

	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPC1(2), "SPI_CS2")) {
		gpio_direction_output(EXYNOS4_GPC1(2), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPC1(2), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPC1(2), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(2, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi2_csi));
	}
	for (gpio = EXYNOS4_GPC1(1); gpio < EXYNOS4_GPC1(5); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
#endif

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	tdmb_dev_init();
#elif defined(CONFIG_ISDBT)
	isdbt_dev_init();
#endif
#endif

#ifdef CONFIG_BUSFREQ_OPP
	dev_add(&busfreq, &exynos4_busfreq.dev);

	/* PPMUs using for cpufreq get clk from clk_list */
	ppmu_clk = clk_get(NULL, "ppmudmc0");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc0\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmudmc1");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc1\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmucpu");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_cpu\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif


	/* 400 kHz for initialization of MMC Card  */
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS3) & 0xfffffff0)
		     | 0x9, EXYNOS4_CLKDIV_FSYS3);
#if defined(CONFIG_MACH_T0) || defined(CONFIG_MACH_M3_JPN_DCM) || \
	defined(CONFIG_MACH_GD2)
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS2) & 0xfff0fff0)
		     | 0x90009, EXYNOS4_CLKDIV_FSYS2);
#else
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS2) & 0xfff0fff0)
		     | 0x80008, EXYNOS4_CLKDIV_FSYS2);
#endif
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS1) & 0xfff0fff0)
		     | 0x80008, EXYNOS4_CLKDIV_FSYS1);
#if defined(CONFIG_MACH_SUPERIOR_KOR_SKT) || defined(CONFIG_MACH_BAFFIN)
	__raw_writel((__raw_readl(S5P_EINT_FLTCON(7)) & 0xffffff00)
		     | 0xff, S5P_EINT_FLTCON(7));
#endif
#if defined(CONFIG_ICE4_FPGA)
	exynos4_fpga_init();
#endif
}

#ifdef CONFIG_EXYNOS_MARK_PAGE_HOLES
static bool __initdata marked_hole_in_ram;
static void __init exynos_page_hole_area_reserve(void)
{
       unsigned long end;
       unsigned long start = PHYS_OFFSET + RAMCH_UNBALANCE_START_OFF;

       end = bank_pfn_end(&meminfo.bank[meminfo.nr_banks - 1]) << PAGE_SHIFT;
       if (memblock_reserve(start, end - start)) {
	       pr_err("%s: Failed to reserve %#lx ~ %#lx", __func__,
			       start, end);
       } else {
	       marked_hole_in_ram = true;
	       pr_err("%s: Reserved %#lx ~ %#lx for marking page holes",
			       __func__, start, end);
       }
}

static void __init exynos_page_hole_init_reserve(void)
{
	unsigned long start = PHYS_OFFSET + RAMCH_UNBALANCE_START_OFF;
	unsigned long end = bank_pfn_end(&meminfo.bank[meminfo.nr_banks - 1])
		<< PAGE_SHIFT;

	if (!marked_hole_in_ram)
		return;

	memblock_free(start, end - start);

	/*
	 * The holes in the lowmem is marked here.
	 * Those in the highmem is marked in free_area() in arch/arm/mm/init.c
	 */
	start += PAGE_HOLE_EVEN_ODD * PAGE_SIZE;
	while (start < __pa(high_memory)) {
		pr_err("%s: reserve physical hole at %#lx\n", __func__, start);
		if (reserve_bootmem(start, PAGE_SIZE, BOOTMEM_EXCLUSIVE))
			pr_err("%s: Failed to reserve physical hole at %#lx\n",
					__func__, start);
		start += PAGE_SIZE * 2;
	}
}
#else
#define exynos_page_hole_area_reserve() do { } while (0)
#define exynos_page_hole_init_reserve() do { } while (0)
#endif

static void __init exynos4_reserve(void)
{
	int ret = 0;
	exynos_page_hole_area_reserve();

#ifdef CONFIG_DMA_CMA
#ifdef CONFIG_USE_FIMC_CMA
	ret = dma_declare_contiguous(&s3c_device_fimc1.dev,
		CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K, 0x65800000, 0);
	if (ret != 0)
		panic("alloc failed for FIMC1\n");
	else {
		static struct cma_region fimc_reg = {
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
			.start = 0x65800000,
			.reserved = 1,
		};

		if (cma_early_region_register(&fimc_reg))
			pr_err("S5P/CMA: Failed to register '%s'\n",
						fimc_reg.name);
	}
#endif

#if defined(CONFIG_USE_MFC_CMA)
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_WATCH)
	ret = dma_declare_contiguous(&s5p_device_mfc.dev,
			0x02800000, 0x5C800000, 0);
#elif defined(CONFIG_MACH_ZEST)
	ret = dma_declare_contiguous(&s5p_device_mfc.dev,
			0x02800000, 0x58800000, 0);
#elif defined(CONFIG_MACH_GC1)|| defined(CONFIG_MACH_GC2PD)
	ret = dma_declare_contiguous(&s5p_device_mfc.dev,
			0x02800000, 0x50800000, 0);
#endif
#endif
	if (ret != 0)
		printk(KERN_ERR "%s Fail\n", __func__);
#endif
}

static void __init exynos_init_reserve(void)
{
	sec_debug_magic_init();
	exynos_page_hole_init_reserve();
}

MACHINE_START(SMDK4412, "SMDK4x12")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= midas_map_io,
	.init_machine	= midas_machine_init,
	.timer		= &exynos4_timer,
	.reserve	= &exynos4_reserve,
	.init_early	= &exynos_init_reserve,
MACHINE_END

MACHINE_START(SMDK4212, "SMDK4x12")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= midas_map_io,
	.init_machine	= midas_machine_init,
	.timer		= &exynos4_timer,
	.reserve	= &exynos4_reserve,
	.init_early	= &exynos_init_reserve,
MACHINE_END
