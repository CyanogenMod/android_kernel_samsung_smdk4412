/*
 * midas-sound.c - Sound Management of MIDAS Project
 *
 *  Copyright (C) 2012 Samsung Electrnoics
 *  JS Park <aitdark.park@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c-gpio.h>
#include <mach/irqs.h>
#include <mach/pmu.h>
#include <plat/iic.h>

#include <plat/gpio-cfg.h>
#ifdef CONFIG_ARCH_EXYNOS5
#include <mach/gpio-p10.h>
#else
#include <mach/gpio-midas.h>
#endif

#ifdef CONFIG_SND_SOC_WM8994
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#endif

#ifdef CONFIG_FM34_WE395
#include <linux/i2c/fm34_we395.h>
#endif

#if defined(CONFIG_FM_SI4709_MODULE) || defined(CONFIG_FM_SI4705) || \
	defined(CONFIG_FM_SI4705_MODULE)
#include <linux/i2c/si47xx_common.h>
#endif

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
#include <linux/exynos_audio.h>
#endif
#ifdef CONFIG_USE_ADC_DET
#include <linux/sec_jack.h>
#endif

#ifdef CONFIG_AUDIENCE_ES305
#include <linux/i2c/es305.h>
#endif

static bool midas_snd_mclk_enabled;

#if defined(CONFIG_FM_SI4705)
struct si47xx_info {
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	int gpio_sw;
#endif
	int gpio_int;
	int gpio_rst;
} si47xx_data;

#endif

#ifdef CONFIG_ARCH_EXYNOS5
#define I2C_NUM_2MIC	4
#define I2C_NUM_CODEC	7
#define SET_PLATDATA_2MIC(i2c_pd)	s3c_i2c4_set_platdata(i2c_pd)
#else /* for CONFIG_ARCH_EXYNOS4 */
#define I2C_NUM_2MIC	6
#define I2C_NUM_CODEC	4
#define SET_PLATDATA_2MIC(i2c_pd)	s3c_i2c6_set_platdata(i2c_pd)
#endif

#ifdef CONFIG_USE_ADC_DET
static struct jack_zone midas_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 0,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 1200, unstable zone, default to 3pole if it stays
		 * in this range for 100ms (10ms delays, 10 samples)
		 */
#if defined(CONFIG_MACH_GC2PD)
		.adc_high = 1150,
#else
		.adc_high = 1200,
#endif
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 1200 < adc <= 2600, unstable zone, default to 4pole if it
		 * stays in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 2600,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* 2600 < adc <= 3800, 4 pole zone, default to 4pole if it
		 * stays in this range for 50ms (10ms delays, 5 samples)
		 */
		.adc_high = 3800,
		.delay_ms = 10,
		.check_count = 5,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 3800, unstable zone, default to 3pole if it stays
		 * in this range for two seconds (10ms delays, 200 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 200,
		.jack_type = SEC_HEADSET_3POLE,
	},
};
#endif

static DEFINE_SPINLOCK(midas_snd_spinlock);

void midas_snd_set_mclk(bool on, bool forced)
{
	static int use_cnt;

	spin_lock(&midas_snd_spinlock);

	midas_snd_mclk_enabled = on;

	if (midas_snd_mclk_enabled) {
		if (use_cnt++ == 0 || forced) {
			printk(KERN_INFO "Sound: enabled mclk\n");
#ifdef CONFIG_ARCH_EXYNOS5
			exynos5_pmu_xclkout_set(midas_snd_mclk_enabled,
							XCLKOUT_XXTI);
#else /* for CONFIG_ARCH_EXYNOS4 */
			exynos4_pmu_xclkout_set(midas_snd_mclk_enabled,
							XCLKOUT_XUSBXTI);
#endif
			mdelay(10);
		}
	} else {
		if ((--use_cnt <= 0) || forced) {
			printk(KERN_INFO "Sound: disabled mclk\n");
#ifdef CONFIG_ARCH_EXYNOS5
			exynos5_pmu_xclkout_set(midas_snd_mclk_enabled,
							XCLKOUT_XXTI);
#else /* for CONFIG_ARCH_EXYNOS4 */
			exynos4_pmu_xclkout_set(midas_snd_mclk_enabled,
							XCLKOUT_XUSBXTI);
#endif
			use_cnt = 0;
		}
	}

	spin_unlock(&midas_snd_spinlock);

	printk(KERN_INFO "Sound: state: %d, use_cnt: %d\n",
					midas_snd_mclk_enabled, use_cnt);
}

bool midas_snd_get_mclk(void)
{
	return midas_snd_mclk_enabled;
}

#ifdef CONFIG_SND_SOC_WM8994
/* vbatt_devices */
static struct regulator_consumer_supply vbatt_supplies[] = {
	REGULATOR_SUPPLY("LDO1VDD", NULL),
	REGULATOR_SUPPLY("SPKVDD1", NULL),
	REGULATOR_SUPPLY("SPKVDD2", NULL),
};

static struct regulator_init_data vbatt_initdata = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(vbatt_supplies),
	.consumer_supplies = vbatt_supplies,
};

static struct fixed_voltage_config vbatt_config = {
	.init_data = &vbatt_initdata,
	.microvolts = 5000000,
	.supply_name = "VBATT",
	.gpio = -EINVAL,
};

struct platform_device vbatt_device = {
	.name = "reg-fixed-voltage",
	.id = -1,
	.dev = {
		.platform_data = &vbatt_config,
	},
};

/* wm1811 ldo1 */
static struct regulator_consumer_supply wm1811_ldo1_supplies[] = {
	REGULATOR_SUPPLY("AVDD1", NULL),
};

static struct regulator_init_data wm1811_ldo1_initdata = {
	.constraints = {
		.name = "WM1811 LDO1",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo1_supplies),
	.consumer_supplies = wm1811_ldo1_supplies,
};

/* wm1811 ldo2 */
static struct regulator_consumer_supply wm1811_ldo2_supplies[] = {
	REGULATOR_SUPPLY("DCVDD", NULL),
};

static struct regulator_init_data wm1811_ldo2_initdata = {
	.constraints = {
		.name = "WM1811 LDO2",
		.always_on = true, /* Actually status changed by LDO1 */
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo2_supplies),
	.consumer_supplies = wm1811_ldo2_supplies,
};

static struct wm8994_drc_cfg drc_value[] = {
#if defined(CONFIG_MACH_GC1)
	{
		.name = "AIF1DAC DRC -3 dB",
		.regs[0] = 0x009C,
		.regs[1] = 0x0845,
		.regs[2] = 0x0000,
		.regs[3] = 0x0004,
		.regs[4] = 0x0000,
	},
#else
	{
		.name = "voice call DRC",
		.regs[0] = 0x009B,
		.regs[1] = 0x0844,
		.regs[2] = 0x00E8,
		.regs[3] = 0x0210,
		.regs[4] = 0x0000,
	},
#endif

#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
	{
		.name = "voice call DRC",
		.regs[0] = 0x008c,
		.regs[1] = 0x0253,
		.regs[2] = 0x0028,
		.regs[3] = 0x028c,
		.regs[4] = 0x0000,
	},
#endif
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_KONA)
{
		.name = "cam rec DRC",
		.regs[0] = 0x019B,
		.regs[1] = 0x0844,
		.regs[2] = 0x0408,
		.regs[3] = 0x0108,
		.regs[4] = 0x0120,
	},
#elif defined(CONFIG_MACH_TAB3)
{
		.name = "cam rec DRC",
		.regs[0] = 0x01BA,
		.regs[1] = 0x0844,
		.regs[2] = 0x04E8,
		.regs[3] = 0x0210,
		.regs[4] = 0x012D,
	},
#endif
};

static struct wm8994_pdata wm1811_pdata = {
	.gpio_defaults = {
		[0] = WM8994_GP_FN_IRQ,	  /* GPIO1 IRQ output, CMOS mode */
		[7] = WM8994_GPN_DIR | WM8994_GP_FN_PIN_SPECIFIC, /* DACDAT3 */
		[8] = WM8994_CONFIGURE_GPIO |
		      WM8994_GP_FN_PIN_SPECIFIC, /* ADCDAT3 */
		[9] = WM8994_CONFIGURE_GPIO |\
		      WM8994_GP_FN_PIN_SPECIFIC, /* LRCLK3 */
		[10] = WM8994_CONFIGURE_GPIO |\
		       WM8994_GP_FN_PIN_SPECIFIC, /* BCLK3 */
	},

	.irq_base = IRQ_BOARD_CODEC_START,

	/* The enable is shared but assign it to LDO1 for software */
	.ldo = {
		{
			.enable = GPIO_WM8994_LDO,
			.init_data = &wm1811_ldo1_initdata,
		},
		{
			.init_data = &wm1811_ldo2_initdata,
		},
	},
	/* Apply DRC Value */
	.drc_cfgs = drc_value,
	.num_drc_cfgs = ARRAY_SIZE(drc_value),

	/* Support external capacitors*/
	.jd_ext_cap = 1,

	/* Regulated mode at highest output voltage */
#ifdef CONFIG_TARGET_LOCALE_KOR
	.micbias = {0x22, 0x22},
#elif defined(CONFIG_MACH_C1_USA_ATT)
	.micbias = {0x2f, 0x29},
#elif defined(CONFIG_MACH_GC1) | defined(CONFIG_MACH_GC2PD)
	.micbias = {0x2f, 0x2b},
#elif defined(CONFIG_MACH_KONA)
	.micbias = {0x2f, 0x2f},
#elif defined(CONFIG_MACH_TAB3)
	.micbias = {0x25, 0x2f},
#else
	.micbias = {0x2f, 0x27},
#endif

	.micd_lvl_sel = 0xFF,

	.ldo_ena_always_driven = true,
	.ldo_ena_delay = 30000,
#ifdef CONFIG_TARGET_LOCALE_KOR
	.lineout2_diff = 0,
#endif
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_BAFFIN) || \
	defined(CONFIG_MACH_SP7160LTE)
	.lineout1fb = 0,
#else
	.lineout1fb = 1,
#endif
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1_KOR_SKT) || \
	defined(CONFIG_MACH_C1_KOR_KT) || defined(CONFIG_MACH_C1_KOR_LGT) || \
	defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_GC1) || \
	defined(CONFIG_MACH_C1_USA_ATT) || defined(CONFIG_MACH_T0) || \
	defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_BAFFIN) || \
	defined(CONFIG_MACH_TAB3) || defined(CONFIG_MACH_KONA)
	.lineout2fb = 0,
#else
	.lineout2fb = 1,
#endif
};

static struct i2c_board_info i2c_wm1811[] __initdata = {
	{
		I2C_BOARD_INFO("wm1811", (0x34 >> 1)),	/* Audio CODEC */
		.platform_data = &wm1811_pdata,
		.irq = IRQ_EINT(30),
	},
};

#endif

#ifdef CONFIG_FM34_WE395
static struct fm34_platform_data fm34_we395_pdata = {
	.gpio_pwdn = GPIO_FM34_PWDN,
	.gpio_rst = GPIO_FM34_RESET,
	.gpio_bp = GPIO_FM34_BYPASS,
	.set_mclk = midas_snd_set_mclk,
};
#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
static struct fm34_platform_data fm34_we395_pdata_rev05 = {
	.gpio_pwdn = GPIO_FM34_PWDN,
	.gpio_rst = GPIO_FM34_RESET_05,
	.gpio_bp = GPIO_FM34_BYPASS_05,
	.set_mclk = midas_snd_set_mclk,
};
#endif

static struct i2c_board_info i2c_2mic[] __initdata = {
	{
		I2C_BOARD_INFO("fm34_we395", (0xC0 >> 1)), /* 2MIC */
		.platform_data = &fm34_we395_pdata,
	},
};

#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
static struct i2c_gpio_platform_data gpio_i2c_fm34 = {
	.sda_pin = GPIO_FM34_SDA,
	.scl_pin = GPIO_FM34_SCL,
};

struct platform_device s3c_device_fm34 = {
	.name = "i2c-gpio",
	.id = I2C_NUM_2MIC,
	.dev.platform_data = &gpio_i2c_fm34,
};
#endif
#endif

#if defined(CONFIG_FM_SI4709_MODULE) || defined(CONFIG_FM_SI4705) || \
	defined(CONFIG_FM_SI4705_MODULE)
static void fmradio_power(int on)
{
	int err;
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	gpio_set_value(si47xx_data.gpio_sw, GPIO_LEVEL_HIGH);
#endif
	if (on) {
		err = gpio_request(GPIO_FM_INT, "GPC1");
		if (err) {
			pr_err(KERN_ERR "GPIO_FM_INT GPIO set error!\n");
			return;
		}
		gpio_direction_output(GPIO_FM_INT, 1);
		gpio_set_value(si47xx_data.gpio_rst, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_FM_INT, GPIO_LEVEL_LOW);
		usleep_range(5, 10);
		gpio_set_value(si47xx_data.gpio_rst, GPIO_LEVEL_HIGH);
		usleep_range(10, 15);
		gpio_set_value(GPIO_FM_INT, GPIO_LEVEL_HIGH);

		s3c_gpio_cfgpin(GPIO_FM_INT, S3C_GPIO_SFN(0xF));
		gpio_free(GPIO_FM_INT);
	} else {
		gpio_set_value(si47xx_data.gpio_rst, GPIO_LEVEL_LOW);
	}
}

static struct si47xx_platform_data si47xx_pdata = {
#if defined(CONFIG_MACH_M0_CTC)
	.rx_vol = {0x0, 0x15, 0x18, 0x1B, 0x1E, 0x21, 0x24, 0x27,
		0x2A, 0x2D, 0x30, 0x33, 0x36, 0x39, 0x3C, 0x3F},
#else
	.rx_vol = {0x0, 0x13, 0x16, 0x19, 0x1C, 0x1F, 0x22, 0x25,
		0x28, 0x2B, 0x2E, 0x31, 0x34, 0x37, 0x3A, 0x3D},
#endif
	.power = fmradio_power,
};

static struct i2c_gpio_platform_data gpio_i2c_data19 = {
	.sda_pin = GPIO_FM_SDA,
	.scl_pin = GPIO_FM_SCL,
};

struct platform_device s3c_device_i2c19 = {
	.name = "i2c-gpio",
	.id = 19,
	.dev.platform_data = &gpio_i2c_data19,
};

/* I2C19 */
static struct i2c_board_info i2c_devs19_emul[] __initdata = {
#if defined(CONFIG_FM_SI4705) || defined(CONFIG_FM_SI4705_MODULE)
	{
		I2C_BOARD_INFO("Si47xx", (0x22 >> 1)),
		.platform_data = &si47xx_pdata,
#if defined(CONFIG_MACH_M0_DUOSCTC)
		.irq = IRQ_EINT(4),
#else
		.irq = IRQ_EINT(11),
#endif
	},
#endif
#ifdef CONFIG_FM_SI4709_MODULE
	{
		I2C_BOARD_INFO("Si4709", (0x20 >> 1)),
	},
#endif

};
#endif

static void m0_gpio_init(void)
{
#ifdef CONFIG_FM_SI4705
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	si47xx_data.gpio_sw = GPIO_FM_MIC_SW;
#endif

	si47xx_data.gpio_rst = GPIO_FM_RST;
	s3c_gpio_setpull(GPIO_FM_INT, S3C_GPIO_PULL_UP);

	if (gpio_is_valid(si47xx_data.gpio_rst)) {
		if (gpio_request(si47xx_data.gpio_rst, "GPC1"))
			debug(KERN_ERR "Failed to request "
			"FM_RST!\n\n");
		gpio_direction_output(si47xx_data.gpio_rst, GPIO_LEVEL_LOW);
	}
#endif
}


#ifdef CONFIG_AUDIENCE_ES305
static struct es305_platform_data es305_pdata = {
	.gpio_wakeup = GPIO_ES305_WAKEUP,
	.gpio_reset = GPIO_ES305_RESET,
	.set_mclk = midas_snd_set_mclk,
};

static struct i2c_board_info i2c_2mic[] __initdata = {
	{
		I2C_BOARD_INFO("audience_es305", 0x3E), /* 2MIC */
		.platform_data = &es305_pdata,
	},
};
#endif

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
struct exynos_sound_platform_data midas_sound_pdata __initdata = {
#ifdef CONFIG_USE_ADC_DET
	.zones = midas_jack_zones,
	.num_zones = ARRAY_SIZE(midas_jack_zones),
#endif
};
#endif

static struct platform_device *midas_sound_devices[] __initdata = {
#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
#ifdef CONFIG_FM34_WE395
	&s3c_device_fm34,
#endif
#endif
#if defined(CONFIG_FM_SI4709_MODULE) || defined(CONFIG_FM_SI4705) || \
	defined(CONFIG_FM_SI4705_MODULE)
	&s3c_device_i2c19,
#endif

};

static void midas_sound_i2c_set_platdata(void)
{
	struct s3c2410_platform_i2c *pd;

	pd = &default_i2c_data;
	pd->bus_num = 4;
	pd->frequency = 100 * 1000;

	s3c_i2c4_set_platdata(pd);
}

void __init midas_sound_init(void)
{
	printk(KERN_INFO "Sound: start %s\n", __func__);

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	pr_info("%s: system rev = %d\n", __func__, system_rev);
#ifdef CONFIG_USE_ADC_DET
#if defined(CONFIG_MACH_GC1_USA_ATT)
	if (system_rev >= 13)
		midas_sound_pdata.use_jackdet_type = 1;
#elif defined(CONFIG_MACH_GC1_USA_VZW)
	if (system_rev >= 17)
		midas_sound_pdata.use_jackdet_type = 1;
#elif defined(CONFIG_MACH_GC2PD) || defined(CONFIG_MACH_KONA)
		midas_sound_pdata.use_jackdet_type = 1;
#endif
#endif
#endif
	m0_gpio_init();

	platform_add_devices(midas_sound_devices,
		ARRAY_SIZE(midas_sound_devices));

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA
	pr_info("%s: set sound platform data for midas device\n", __func__);
	if (exynos_sound_set_platform_data(&midas_sound_pdata))
		pr_err("%s: failed to register sound pdata\n", __func__);
#endif

#ifdef CONFIG_ARCH_EXYNOS5
#ifndef CONFIG_MACH_P10_LTE_00_BD
	i2c_wm1811[0].irq = IRQ_EINT(29);
#endif
	midas_sound_i2c_set_platdata();
	i2c_register_board_info(I2C_NUM_CODEC, i2c_wm1811,
					ARRAY_SIZE(i2c_wm1811));
#else /* for CONFIG_ARCH_EXYNOS4 */
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || \
	defined(CONFIG_MACH_TAB3) || defined(CONFIG_MACH_IPCAM)
	i2c_wm1811[0].irq = 0;
	midas_sound_i2c_set_platdata();
	i2c_register_board_info(I2C_NUM_CODEC, i2c_wm1811,
					ARRAY_SIZE(i2c_wm1811));

#elif defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_GC2PD) || \
	defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_BAFFIN)
		midas_sound_i2c_set_platdata();
		i2c_register_board_info(I2C_NUM_CODEC, i2c_wm1811,
						ARRAY_SIZE(i2c_wm1811));
#else
	if (system_rev != 3) {
		midas_sound_i2c_set_platdata();
		i2c_register_board_info(I2C_NUM_CODEC, i2c_wm1811,
						ARRAY_SIZE(i2c_wm1811));
	}
#endif
#endif/* CONFIG_ARCH_EXYNOS5 */

#ifdef CONFIG_FM34_WE395
	midas_snd_set_mclk(true, false);
	SET_PLATDATA_2MIC(NULL);

#if defined(CONFIG_MACH_C1_KOR_LGT)
	if (system_rev > 5)
		i2c_2mic[0].platform_data = &fm34_we395_pdata_rev05;
#endif
#if defined(CONFIG_MACH_BAFFIN_KOR_LGT)
		i2c_2mic[0].platform_data = &fm34_we395_pdata_rev05;
#endif

	i2c_register_board_info(I2C_NUM_2MIC, i2c_2mic, ARRAY_SIZE(i2c_2mic));
#endif


#ifdef CONFIG_AUDIENCE_ES305
	midas_snd_set_mclk(true, false);
	SET_PLATDATA_2MIC(NULL);
	i2c_register_board_info(I2C_NUM_2MIC, i2c_2mic, ARRAY_SIZE(i2c_2mic));
#endif
#if defined(CONFIG_FM_SI4709_MODULE) || defined(CONFIG_FM_SI4705) || \
	defined(CONFIG_FM_SI4705_MODULE)
	i2c_register_board_info(19, i2c_devs19_emul,
				ARRAY_SIZE(i2c_devs19_emul));
#endif

}
