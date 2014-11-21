/* linux/arch/arm/plat-s5p/include/plat/fimc_is.h
 *
 * Copyright (C) 2011 Samsung Electronics, Co. Ltd
 *
 * Exynos 4 series FIMC-IS slave device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef EXYNOS_FIMC_IS_H_
#define EXYNOS_FIMC_IS_H_ __FILE__

#include <linux/videodev2.h>

#define FIMC_IS_MAX_CAMIF_CLIENTS	2
#define FIMC_IS_MAX_SENSOR_NAME_LEN	16
#define EXYNOS4_FIMC_IS_MAX_DIV_CLOCKS		2
#define EXYNOS4_FIMC_IS_MAX_CONTROL_CLOCKS	16

#define UART_ISP_SEL		0
#define UART_ISP_RATIO		1

#define to_fimc_is_plat(d)	(to_platform_device(d)->dev.platform_data)

#if defined(CONFIG_ARCH_EXYNOS5)
enum exynos5_csi_id {
	CSI_ID_A = 0,
	CSI_ID_B
};

enum exynos5_flite_id {
	FLITE_ID_A = 0,
	FLITE_ID_B
};

enum exynos5_sensor_position {
	SENSOR_POSITION_REAR = 0,
	SENSOR_POSITION_FRONT
};
enum exynos5_sensor_id {
	SENSOR_NAME_S5K3H2	= 1,
	SENSOR_NAME_S5K6A3	= 2,
	SENSOR_NAME_S5K4E5	= 3,
	SENSOR_NAME_S5K3H7	= 4,
	SENSOR_NAME_S5K6B2	= 5,
	SENSOR_NAME_CUSTOM	= 100,
	SENSOR_NAME_END
};

enum exynos5_sensor_channel {
	SENSOR_CONTROL_I2C0	= 0,
	SENSOR_CONTROL_I2C1	= 1
};
#endif

struct platform_device;

#if defined(CONFIG_ARCH_EXYNOS5)
/**
 * struct exynos5_fimc_is_sensor_info  - image sensor information required for host
 *			      interace configuration.
*/
struct exynos5_fimc_is_sensor_info {
	char sensor_name[FIMC_IS_MAX_SENSOR_NAME_LEN];
	enum exynos5_sensor_position sensor_position;
	enum exynos5_sensor_id sensor_id;
	enum exynos5_csi_id csi_id;
	enum exynos5_flite_id flite_id;
	enum exynos5_sensor_channel i2c_channel;

	int max_width;
	int max_height;
	int max_frame_rate;


	int mipi_lanes;     /* MIPI data lanes */
	int mipi_settle;    /* MIPI settle */
	int mipi_align;     /* MIPI data align: 24/32 */
};
#endif
/**
 * struct exynos4_platform_fimc_is - camera host interface platform data
 *
 * @isp_info: properties of camera sensor required for host interface setup
*/
struct exynos4_platform_fimc_is {
	int	hw_ver;
	struct clk	*div_clock[EXYNOS4_FIMC_IS_MAX_DIV_CLOCKS];
	struct clk	*control_clock[EXYNOS4_FIMC_IS_MAX_CONTROL_CLOCKS];
	void	(*cfg_gpio)(struct platform_device *pdev);
	int	(*clk_get)(struct platform_device *pdev);
	int	(*clk_put)(struct platform_device *pdev);
	int	(*clk_cfg)(struct platform_device *pdev);
	int	(*clk_on)(struct platform_device *pdev);
	int	(*clk_off)(struct platform_device *pdev);
	int	(*filter_on)(void);
	int	(*filter_off)(void);
};

#if defined(CONFIG_ARCH_EXYNOS5)
struct exynos5_platform_fimc_is {
	int	hw_ver;
	struct exynos5_fimc_is_sensor_info
		*sensor_info[FIMC_IS_MAX_CAMIF_CLIENTS];
	void	(*cfg_gpio)(struct platform_device *pdev);
	int	(*clk_cfg)(struct platform_device *pdev);
	int	(*clk_on)(struct platform_device *pdev);
	int	(*clk_off)(struct platform_device *pdev);
};
#endif

extern struct exynos4_platform_fimc_is exynos4_fimc_is_default_data;
extern void exynos4_fimc_is_set_platdata(struct exynos4_platform_fimc_is *pd);
#if defined(CONFIG_ARCH_EXYNOS5)
extern void exynos5_fimc_is_set_platdata(struct exynos5_platform_fimc_is *pd);
#endif
/* defined by architecture to configure gpio */
extern void exynos_fimc_is_cfg_gpio(struct platform_device *pdev);

/* platform specific clock functions */
extern int exynos_fimc_is_cfg_clk(struct platform_device *pdev);
extern int exynos_fimc_is_clk_on(struct platform_device *pdev);
extern int exynos_fimc_is_clk_off(struct platform_device *pdev);
extern int exynos_fimc_is_clk_get(struct platform_device *pdev);
extern int exynos_fimc_is_clk_put(struct platform_device *pdev);
#ifdef CONFIG_MACH_IPCAM
extern int exynos_fimc_is_filter_on(void);
extern int exynos_fimc_is_filter_off(void);
#endif

#if defined(CONFIG_ARCH_EXYNOS5)
/* defined by architecture to configure gpio */
extern void exynos5_fimc_is_cfg_gpio(struct platform_device *pdev);

/* platform specific clock functions */
extern int exynos5_fimc_is_cfg_clk(struct platform_device *pdev);
extern int exynos5_fimc_is_clk_on(struct platform_device *pdev);
extern int exynos5_fimc_is_clk_off(struct platform_device *pdev);
#endif
#endif /* EXYNOS_FIMC_IS_H_ */
