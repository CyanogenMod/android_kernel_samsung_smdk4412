#include <linux/gpio.h>
#include <linux/i2c.h>
#ifdef CONFIG_PN65N_NFC
#include <linux/nfc/pn65n.h>
#endif
#ifdef CONFIG_PN547_NFC
#include <linux/nfc/pn547.h>
#endif
#if defined(CONFIG_BCM2079X_NFC)
#include <linux/nfc/bcm2079x.h>
#endif

#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include "midas.h"

#if defined(CONFIG_PN65N_NFC)
#if defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M0_CTC)
#define I2C_BUSNUM_PN65N	(system_rev == 3 ? 0 : 5)
#elif defined(CONFIG_MACH_M3)
#define I2C_BUSNUM_PN65N	12
#elif defined(CONFIG_MACH_M0)
#define I2C_BUSNUM_PN65N	(system_rev == 3 ? 12 : 5)
#elif defined(CONFIG_MACH_T0)
#define I2C_BUSNUM_PN65N	(system_rev == 0 ? 5 : 12)
#elif defined(CONFIG_MACH_BAFFIN)
#define I2C_BUSNUM_PN65N	5
#else
#define I2C_BUSNUM_PN65N	12
#endif
#endif

#if defined(CONFIG_PN547_NFC)
#if defined(CONFIG_MACH_ZEST)
#define I2C_BUSNUM_PN547	(system_rev >= 1 ? 12 : 5)
#else
#define I2C_BUSNUM_PN547	5
#endif
#endif

/* GPIO_LEVEL_NONE = 2, GPIO_LEVEL_LOW = 0 */
#if defined(CONFIG_BCM2079X_NFC)
static unsigned int nfc_gpio_table[][4] = {
	{GPIO_NFC_IRQ, S3C_GPIO_INPUT, 2, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_EN, S3C_GPIO_OUTPUT, 0, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_FIRMWARE, S3C_GPIO_OUTPUT, 1, S3C_GPIO_PULL_NONE},
};
#else
static unsigned int nfc_gpio_table[][4] = {
	{GPIO_NFC_IRQ, S3C_GPIO_INPUT, 2, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_EN, S3C_GPIO_OUTPUT, 0, S3C_GPIO_PULL_NONE},
	{GPIO_NFC_FIRMWARE, S3C_GPIO_OUTPUT, 0, S3C_GPIO_PULL_NONE},
};
#endif

static inline void nfc_setup_gpio(void)
{
	int err = 0;
	int array_size = ARRAY_SIZE(nfc_gpio_table);
	u32 i, gpio;

#if defined(CONFIG_MACH_GC2PD)
	/* for EMUL board NFC_LDO_EN supporting */
	pr_info("%s, set NFC_LDO_EN pin to high", __func__);
	s3c_gpio_cfgpin(EXYNOS4_GPY4(5), S3C_GPIO_OUTPUT);
	gpio_set_value(EXYNOS4_GPY4(5), 1);
	s3c_gpio_setpull(EXYNOS4_GPY4(5), S3C_GPIO_PULL_UP);
#endif

	for (i = 0; i < array_size; i++) {
		gpio = nfc_gpio_table[i][0];

		err = s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(nfc_gpio_table[i][1]));
		if (err < 0)
			pr_err("%s, s3c_gpio_cfgpin gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		err = s3c_gpio_setpull(gpio, nfc_gpio_table[i][3]);
		if (err < 0)
			pr_err("%s, s3c_gpio_setpull gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		if (nfc_gpio_table[i][2] != 2)
			gpio_set_value(gpio, nfc_gpio_table[i][2]);
	}
}

#if defined(CONFIG_PN65N_NFC)
static struct pn65n_i2c_platform_data pn65n_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.ven_gpio = GPIO_NFC_EN,
	.firm_gpio = GPIO_NFC_FIRMWARE,
};

static struct i2c_board_info i2c_dev_pn65n __initdata = {
	I2C_BOARD_INFO("pn65n", 0x2b),
	.irq = IRQ_EINT(15),
	.platform_data = &pn65n_pdata,
};
#elif defined(CONFIG_PN547_NFC)
static struct pn547_i2c_platform_data pn547_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.ven_gpio = GPIO_NFC_EN,
	.firm_gpio = GPIO_NFC_FIRMWARE,
};

static struct i2c_board_info i2c_dev_pn547 __initdata = {
	I2C_BOARD_INFO("pn547", 0x2b),
	.irq = IRQ_EINT(15),
	.platform_data = &pn547_pdata,
};
#elif defined(CONFIG_BCM2079X_NFC)
static struct bcm2079x_platform_data bcm2079x_i2c_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.en_gpio = GPIO_NFC_EN,
	.wake_gpio = GPIO_NFC_FIRMWARE,
};

static struct i2c_board_info i2c_dev_bcm2079x __initdata = {
	I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
	.irq = IRQ_EINT(20),
	.platform_data = &bcm2079x_i2c_pdata,
};
#endif

static int __init midas_nfc_init(void)
{
	int ret;

	nfc_setup_gpio();

#if defined(CONFIG_PN65N_NFC)
	ret = i2c_add_devices(I2C_BUSNUM_PN65N, &i2c_dev_pn65n, 1);
	if (ret < 0) {
		pr_err("%s, i2c%d adding i2c fail(err=%d)\n",
			__func__, I2C_BUSNUM_PN65N, ret);
		return ret;
	}
#elif defined(CONFIG_PN547_NFC)
	ret = i2c_add_devices(I2C_BUSNUM_PN547, &i2c_dev_pn547, 1);
	if (ret < 0) {
		pr_err("%s, i2c%d adding i2c fail(err=%d)\n",
			__func__, I2C_BUSNUM_PN547, ret);
		return ret;
	}
#elif defined(CONFIG_BCM2079X_NFC)
	ret = i2c_add_devices(12, &i2c_dev_bcm2079x, 1);
	if (ret < 0) {
		pr_err("%s, i2c12 adding i2c fail(err=%d)\n", __func__, ret);
		return ret;
	}
#else
	ret = 0;
#endif

	return ret;
}
module_init(midas_nfc_init);
