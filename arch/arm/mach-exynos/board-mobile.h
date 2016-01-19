#ifndef __SAMSUNG_MOBILE_BOARD_COMMON_H
#define __SAMSUNG_MOBILE_BOARD_COMMON_H

/*
 * Please add the common mobile external declarations
 */
extern void midas_camera_init(void);

/* charger-manager */
extern struct charger_global_desc midas_charger_g_desc;
extern struct platform_device midas_charger_manager;
extern void cm_change_fullbatt_uV(void);

#if 0
/* MAX77693 */
extern struct max77693_muic_data max77693_muic;
extern struct max77693_regulator_data max77693_regulators;
#endif

/* i2c-gpio(sw) pin configuration */
#define GPIO_I2C_PIN_SETUP(_bus)	{	\
	s3c_gpio_cfgpin(gpio_i2c_##_bus.sda_pin, S3C_GPIO_INPUT);	\
	s3c_gpio_setpull(gpio_i2c_##_bus.sda_pin, S3C_GPIO_PULL_NONE);	\
	s3c_gpio_cfgpin(gpio_i2c_##_bus.scl_pin, S3C_GPIO_INPUT);	\
	s3c_gpio_setpull(gpio_i2c_##_bus.scl_pin, S3C_GPIO_PULL_NONE);	\
}

#ifdef CONFIG_SLP
extern int __init midas_nfc_init(int i2c_bus);

/* NTC thermistor */
extern struct platform_device midas_ncp15wb473_thermistor;
extern int __init adc_ntc_init(int port);
#endif

/* wifi */
extern int brcm_wlan_init(void);

/* gps */
extern void set_gps_uart_op(int onoff);

#ifdef CONFIG_MACH_KONA
extern void exynos_kona_charger_init(void);
#endif

#if defined(CONFIG_MACH_TAB3)
#if defined(CONFIG_TAB3_00_BD)
extern void exynos_tab3_charger_init(void);
#else
extern void exynos_slim8_charger_init(void);
#endif
#endif
#endif

