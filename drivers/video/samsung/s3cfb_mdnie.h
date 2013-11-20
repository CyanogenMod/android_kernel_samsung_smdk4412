
/* linux/drivers/video/samsung/s3cfb_mdnie.h
 *
 * Header file for Samsung (MDNIE) driver
 *
 * Copyright (c) 2009 Samsung Electronics
 *	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3CFB_MDNIE_H__
#define __S3CFB_MDNIE_H__

#define S3C_MDNIE_PHY_BASE		0x11CA0000
#define S3C_MDNIE_MAP_SIZE		0x00001000

/* Register Address */
#if defined(CONFIG_CPU_EXYNOS4210)
#define MDNIE_REG_BANK_SEL		0x0000
#define MDNIE_REG_WIDTH		0x0022
#define MDNIE_REG_HEIGHT		0x0023
#define MDNIE_REG_MASK		0x0028

#define MDNIE_REG_PWM_CONTROL	0x00B4
#define MDNIE_REG_POWER_LUT0		0x0076
#define MDNIE_REG_POWER_LUT2		0x0077
#define MDNIE_REG_POWER_LUT4		0x0078
#define MDNIE_REG_POWER_LUT6		0x0079
#define MDNIE_REG_POWER_LUT8		0x007A

#elif defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#define MDNIE_REG_BANK_SEL		0x0000
#define MDNIE_REG_WIDTH		0x0003
#define MDNIE_REG_HEIGHT		0x0004
#define MDNIE_REG_MASK		0x00FF

#define S3C_MDNIE_rR1			0x0001

#define MDNIE_REG_PWM_CONTROL	0x00B6
#define MDNIE_REG_POWER_LUT0		0x0079
#define MDNIE_REG_POWER_LUT2		0x007A
#define MDNIE_REG_POWER_LUT4		0x007B
#define MDNIE_REG_POWER_LUT6		0x007C
#define MDNIE_REG_POWER_LUT8		0x007D
#endif

#if defined(CONFIG_CPU_EXYNOS4210)
#define MDNIE_REG_RED_R		0x00C8
#define MDNIE_REG_RED_G		0x00C9
#define MDNIE_REG_RED_B		0x00CA
#define MDNIE_REG_BLUE_R		0x00CB
#define MDNIE_REG_BLUE_G		0x00CC
#define MDNIE_REG_BLUE_B		0x00CD
#define MDNIE_REG_GREEN_R		0x00CE
#define MDNIE_REG_GREEN_G		0x00CF
#define MDNIE_REG_GREEN_B		0x00D0
#define MDNIE_REG_WHITE_R		0x00D1
#define MDNIE_REG_WHITE_G		0x00D2
#define MDNIE_REG_WHITE_B		0x00D3
#elif defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#define MDNIE_REG_RED_R		0x00E1		/*SCR RrCr*/
#define MDNIE_REG_RED_G		0x00E2		/*SCR RgCg*/
#define MDNIE_REG_RED_B		0x00E3		/*SCR RbCb*/
#define MDNIE_REG_BLUE_R		0x00E4		/*SCR GrMr*/
#define MDNIE_REG_BLUE_G		0x00E5		/*SCR GgMg*/
#define MDNIE_REG_BLUE_B		0x00E6		/*SCR GbMb*/
#define MDNIE_REG_GREEN_R		0x00E7		/*SCR BrYr*/
#define MDNIE_REG_GREEN_G		0x00E8		/*SCR BgYg*/
#define MDNIE_REG_GREEN_B		0x00E9		/*SCR BbYb*/
#define MDNIE_REG_WHITE_R		0x00EA		/*SCR KrWr*/
#define MDNIE_REG_WHITE_G		0x00EB		/*SCR KgWg*/
#define MDNIE_REG_WHITE_B		0x00EC		/*SCR KbWb*/
#endif

/* Register Value */
#if defined(CONFIG_CPU_EXYNOS4210)
#define MDNIE_PWM_BANK		0x0000
#elif defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#define MDNIE_PWM_BANK		0x0001	/* CMC624's PWM CTL is in BANK1 */
#endif

#define S3C_MDNIE_INPUT_DATA_ENABLE	(1 << 10)

#define	S3C_MDNIE_SIZE_MASK		0x7FF
#define S3C_MDNIE_HSIZE(n)		(n & S3C_MDNIE_SIZE_MASK)
#define S3C_MDNIE_VSIZE(n)		(n & S3C_MDNIE_SIZE_MASK)


#define TRUE				1
#define FALSE				0

void mdnie_setup(void);
int mdnie_display_on(void);
int mdnie_display_off(void);

int mdnie_write(unsigned int addr, unsigned int val);
int mdnie_mask(void);
int mdnie_unmask(void);

#endif
