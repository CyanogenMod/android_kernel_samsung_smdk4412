/*
 * midas-power.c - Power Management of MIDAS Project
 *
 *  Copyright (C) 2011 Samsung Electrnoics
 *  Chiwoong Byun <woong.byun@samsung.com>
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
 *
 *=======================================
 * Kona power history
 *=======================================
 * 2013. 2. 02 : jaecheol kim (jc22.kim@samsung.com)
 *  - kona add if_pmic(max77693) from revision 0.8(gpio=06)
        so divide power files based on kona-power.c
 */

#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-midas.h>
#include <mach/irqs.h>

#include <linux/mfd/max77686.h>
#include <linux/mfd/max77693.h>


#ifdef CONFIG_MFD_MAX77693
static struct regulator_consumer_supply safeout1_supply[] = {
	REGULATOR_SUPPLY("safeout1", NULL),
};

static struct regulator_consumer_supply safeout2_supply[] = {
	REGULATOR_SUPPLY("safeout2", NULL),
};

static struct regulator_consumer_supply charger_supply[] = {
	REGULATOR_SUPPLY("vinchg1", "charger-manager.0"),
	REGULATOR_SUPPLY("vinchg1", NULL),
};

static struct regulator_init_data safeout1_init_data = {
	.constraints	= {
		.name		= "safeout1 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
		.boot_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(safeout1_supply),
	.consumer_supplies	= safeout1_supply,
};

static struct regulator_init_data safeout2_init_data = {
	.constraints	= {
		.name		= "safeout2 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
		.boot_on	= 0,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(safeout2_supply),
	.consumer_supplies	= safeout2_supply,
};

static struct regulator_init_data charger_init_data = {
	.constraints	= {
		.name		= "CHARGER",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_CURRENT,
		.boot_on	= 1,
		.min_uA		= 60000,
		.max_uA		= 2580000,
	},
	.num_consumer_supplies	= ARRAY_SIZE(charger_supply),
	.consumer_supplies	= charger_supply,
};

struct max77693_regulator_data max77693_regulators[] = {
	{MAX77693_ESAFEOUT1, &safeout1_init_data,},
	{MAX77693_ESAFEOUT2, &safeout2_init_data,},
	{MAX77693_CHARGER, &charger_init_data,},
};
#endif

#if defined(CONFIG_REGULATOR_MAX77686)
/* max77686 */

#ifdef CONFIG_SND_SOC_WM8994
static struct regulator_consumer_supply ldo3_supply[] = {
	REGULATOR_SUPPLY("AVDD2", NULL),
	REGULATOR_SUPPLY("CPVDD", NULL),
	REGULATOR_SUPPLY("DBVDD1", NULL),
	REGULATOR_SUPPLY("DBVDD2", NULL),
	REGULATOR_SUPPLY("DBVDD3", NULL),
};
#else
static struct regulator_consumer_supply ldo3_supply[] = {};
#endif

static struct regulator_consumer_supply ldo5_supply[] = {
	REGULATOR_SUPPLY("vcc_1.8v", NULL),
	REGULATOR_SUPPLY("touchkey", NULL), /*touchkey*/
};

static struct regulator_consumer_supply ldo8_supply[] = {
	REGULATOR_SUPPLY("vmipi_1.0v", NULL),
	REGULATOR_SUPPLY("VDD10", "s5p-mipi-dsim.0"),
	REGULATOR_SUPPLY("vdd", "exynos4-hdmi"),
	REGULATOR_SUPPLY("vdd_pll", "exynos4-hdmi"),
};

static struct regulator_consumer_supply ldo9_supply[] = {
	REGULATOR_SUPPLY("vled_ic_1.9v", NULL),
};

static struct regulator_consumer_supply ldo10_supply[] = {
	REGULATOR_SUPPLY("vmipi_1.8v", NULL),
	REGULATOR_SUPPLY("VDD18", "s5p-mipi-dsim.0"),
	REGULATOR_SUPPLY("vdd_osc", "exynos4-hdmi"),
};

static struct regulator_consumer_supply ldo11_supply[] = {
	REGULATOR_SUPPLY("vabb1_1.9v", NULL),
};

static struct regulator_consumer_supply ldo12_supply[] = {
	REGULATOR_SUPPLY("votg_3.0v", NULL),
};

static struct regulator_consumer_supply ldo14_supply[] = {
	REGULATOR_SUPPLY("vabb2_1.9v", NULL),
};

static struct regulator_consumer_supply ldo18_supply[] = {
	REGULATOR_SUPPLY("cam_io_1.8v", NULL),
};

static struct regulator_consumer_supply ldo19_supply[] = {
	REGULATOR_SUPPLY("vt_core_1.8v", NULL),
};

static struct regulator_consumer_supply ldo21_supply[] = {
	REGULATOR_SUPPLY("vtf_2.8v", NULL),
};

static struct regulator_consumer_supply ldo23_supply[] = {
	REGULATOR_SUPPLY("vmotor", NULL),
};

static struct regulator_consumer_supply ldo24_supply[] = {
	REGULATOR_SUPPLY("cam_a2.8v", NULL),
};

static struct regulator_consumer_supply ldo25_supply[] = {
	REGULATOR_SUPPLY("tsp_3.3v", NULL),
};

static struct regulator_consumer_supply ldo26_supply[] = {
	REGULATOR_SUPPLY("3mp_af_2.8v", NULL),
};

static struct regulator_consumer_supply max77686_buck1[] = {
	REGULATOR_SUPPLY("vdd_mif", NULL),
	REGULATOR_SUPPLY("vdd_mif", "exynos4412-busfreq"),
};

static struct regulator_consumer_supply max77686_buck2 =
	REGULATOR_SUPPLY("vdd_arm", NULL);

static struct regulator_consumer_supply max77686_buck3[] = {
	REGULATOR_SUPPLY("vdd_int", NULL),
	REGULATOR_SUPPLY("vdd_int", "exynoss4412-busfreq"),
};

static struct regulator_consumer_supply max77686_buck4[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
	REGULATOR_SUPPLY("vdd_g3d", "mali_dev.0"),
};

static struct regulator_consumer_supply max77686_buck9 =
	REGULATOR_SUPPLY("3mp_core_1.2v", NULL);

static struct regulator_consumer_supply max77686_enp32khz[] = {
	REGULATOR_SUPPLY("lpo_in", "bcm47511"),
	REGULATOR_SUPPLY("lpo", "bcm4334_bluetooth"),
};

#define REGULATOR_INIT(_ldo, _name, _min_uV, _max_uV, _always_on, _ops_mask, \
		       _disabled)					\
	static struct regulator_init_data _ldo##_init_data = {		\
		.constraints = {					\
			.name = _name,					\
			.min_uV = _min_uV,				\
			.max_uV = _max_uV,				\
			.always_on	= _always_on,			\
			.boot_on	= _always_on,			\
			.apply_uV	= 1,				\
			.valid_ops_mask = _ops_mask,			\
			.state_mem = {					\
				.disabled	= _disabled,		\
				.enabled	= !(_disabled),		\
			}						\
		},							\
		.num_consumer_supplies = ARRAY_SIZE(_ldo##_supply),	\
		.consumer_supplies = &_ldo##_supply[0],			\
	};

REGULATOR_INIT(ldo3, "VCC_1.8V_AP", 1800000, 1800000, 1, 0, 0);
REGULATOR_INIT(ldo5, "VCC_1.8V_IO", 1800000, 1800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo8, "VMIPI_1.0V", 1000000, 1000000, 1,
	       REGULATOR_CHANGE_STATUS, 0);
#if defined(CONFIG_IR_REMOCON_MC96)
REGULATOR_INIT(ldo9, "VLED_IC_1.9V", 1950000, 1950000, 1,
	       REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo9, "VLED_IC_1.9V", 1950000, 1950000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
#endif
REGULATOR_INIT(ldo10, "VMIPI_1.8V", 1800000, 1800000, 1,
	       REGULATOR_CHANGE_STATUS, 0);
REGULATOR_INIT(ldo11, "VABB1_1.9V", 1950000, 1950000, 1,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo12, "VUOTG_3.0V", 3000000, 3000000, 1,
	       REGULATOR_CHANGE_STATUS, 0);
REGULATOR_INIT(ldo14, "VABB2_1.9V", 1950000, 1950000, 1,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo18, "CAM_IO_1.8V", 1800000, 1800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo19, "VT_CORE_1.8V", 1800000, 1800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo21, "VTF_2.8V", 2800000, 2800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo23, "VCC_MOTOR_3.0V", 3000000, 3000000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo24, "CAM_A2.8V", 2800000, 2800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo25, "TSP_3.3V", 3300000, 3300000, 0,
	       REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo26, "3MP_AF_2.8V", 2800000, 2800000, 0,
	       REGULATOR_CHANGE_STATUS, 1);

static struct regulator_init_data max77686_buck1_data = {
	.constraints = {
		.name = "vdd_mif range",
		.min_uV = 850000,
		.max_uV = 1200000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = ARRAY_SIZE(max77686_buck1),
	.consumer_supplies = max77686_buck1,
};

static struct regulator_init_data max77686_buck2_data = {
	.constraints = {
		.name = "vdd_arm range",
		.min_uV = 850000,
		.max_uV = 1500000,
		.apply_uV = 1,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &max77686_buck2,
};

static struct regulator_init_data max77686_buck3_data = {
	.constraints = {
		.name = "vdd_int range",
		.min_uV = 850000,
		.max_uV = 1300000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = ARRAY_SIZE(max77686_buck3),
	.consumer_supplies = max77686_buck3,
};

static struct regulator_init_data max77686_buck4_data = {
	.constraints = {
		.name = "vdd_g3d range",
		.min_uV = 850000,
		.max_uV = 1200000,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
		REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(max77686_buck4),
	.consumer_supplies = max77686_buck4,
};

static struct regulator_init_data max77686_buck9_data = {
	.constraints = {
		.name = "3MP_CORE_1.2V",
		.min_uV = 1200000,
		.max_uV = 1400000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
		REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.disabled = 1,
		},
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &max77686_buck9,
};

static struct regulator_init_data max77686_enp32khz_data = {
	.constraints = {
		.name = "32KHZ_PMIC",
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.enabled	= 1,
			.disabled	= 0,
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(max77686_enp32khz),
	.consumer_supplies = max77686_enp32khz,
};

static struct max77686_regulator_data max77686_regulators[] = {
	{MAX77686_BUCK1, &max77686_buck1_data,},
	{MAX77686_BUCK2, &max77686_buck2_data,},
	{MAX77686_BUCK3, &max77686_buck3_data,},
	{MAX77686_BUCK4, &max77686_buck4_data,},
	{MAX77686_BUCK9, &max77686_buck9_data,},
	{MAX77686_LDO3, &ldo3_init_data,},
	{MAX77686_LDO5, &ldo5_init_data,},
	{MAX77686_LDO8, &ldo8_init_data,},
	{MAX77686_LDO9, &ldo9_init_data,},
	{MAX77686_LDO10, &ldo10_init_data,},
	{MAX77686_LDO11, &ldo11_init_data,},
	{MAX77686_LDO12, &ldo12_init_data,},
	{MAX77686_LDO14, &ldo14_init_data,},
	{MAX77686_LDO18, &ldo18_init_data,},
	{MAX77686_LDO19, &ldo19_init_data,},
	{MAX77686_LDO21, &ldo21_init_data,},
	{MAX77686_LDO23, &ldo23_init_data,},
	{MAX77686_LDO24, &ldo24_init_data,},
	{MAX77686_LDO25, &ldo25_init_data,},
	{MAX77686_LDO26, &ldo26_init_data,},
	{MAX77686_P32KH, &max77686_enp32khz_data,},
};

struct max77686_opmode_data max77686_opmode_data[MAX77686_REG_MAX] = {
	[MAX77686_LDO3] = {MAX77686_LDO3, MAX77686_OPMODE_NORMAL},
	[MAX77686_LDO8] = {MAX77686_LDO8, MAX77686_OPMODE_STANDBY},
	[MAX77686_LDO10] = {MAX77686_LDO10, MAX77686_OPMODE_STANDBY},
	[MAX77686_LDO11] = {MAX77686_LDO11, MAX77686_OPMODE_STANDBY},
	[MAX77686_LDO12] = {MAX77686_LDO12, MAX77686_OPMODE_STANDBY},
	[MAX77686_LDO14] = {MAX77686_LDO14, MAX77686_OPMODE_STANDBY},
	[MAX77686_BUCK1] = {MAX77686_BUCK1, MAX77686_OPMODE_STANDBY},
	[MAX77686_BUCK2] = {MAX77686_BUCK2, MAX77686_OPMODE_STANDBY},
	[MAX77686_BUCK3] = {MAX77686_BUCK3, MAX77686_OPMODE_STANDBY},
	[MAX77686_BUCK4] = {MAX77686_BUCK4, MAX77686_OPMODE_STANDBY},
};

struct max77686_platform_data exynos4_max77686_info = {
	.num_regulators = ARRAY_SIZE(max77686_regulators),
	.regulators = max77686_regulators,
	.irq_gpio	= GPIO_PMIC_IRQ,
	.irq_base	= IRQ_BOARD_PMIC_START,
	.wakeup		= 1,

	.opmode_data = max77686_opmode_data,
	.ramp_rate = MAX77686_RAMP_RATE_27MV,

	.buck234_gpio_dvs = {
		/* Use DVS2 register of each bucks to supply stable power
		 * after sudden reset */
		{GPIO_PMIC_DVS1, 1},
		{GPIO_PMIC_DVS2, 0},
		{GPIO_PMIC_DVS3, 0},
	},
	.buck234_gpio_selb = {
		GPIO_BUCK2_SEL,
		GPIO_BUCK3_SEL,
		GPIO_BUCK4_SEL,
	},
	.buck2_voltage[0] = 1100000,	/* 1.1V */
	.buck2_voltage[1] = 1100000,	/* 1.1V */
	.buck2_voltage[2] = 1100000,	/* 1.1V */
	.buck2_voltage[3] = 1100000,	/* 1.1V */
	.buck2_voltage[4] = 1100000,	/* 1.1V */
	.buck2_voltage[5] = 1100000,	/* 1.1V */
	.buck2_voltage[6] = 1100000,	/* 1.1V */
	.buck2_voltage[7] = 1100000,	/* 1.1V */

	.buck3_voltage[0] = 1100000,	/* 1.1V */
	.buck3_voltage[1] = 1000000,	/* 1.0V */
	.buck3_voltage[2] = 1100000,	/* 1.1V */
	.buck3_voltage[3] = 1100000,	/* 1.1V */
	.buck3_voltage[4] = 1100000,	/* 1.1V */
	.buck3_voltage[5] = 1100000,	/* 1.1V */
	.buck3_voltage[6] = 1100000,	/* 1.1V */
	.buck3_voltage[7] = 1100000,	/* 1.1V */

	.buck4_voltage[0] = 1100000,	/* 1.1V */
	.buck4_voltage[1] = 1000000,	/* 1.0V */
	.buck4_voltage[2] = 1100000,	/* 1.1V */
	.buck4_voltage[3] = 1100000,	/* 1.1V */
	.buck4_voltage[4] = 1100000,	/* 1.1V */
	.buck4_voltage[5] = 1100000,	/* 1.1V */
	.buck4_voltage[6] = 1100000,	/* 1.1V */
	.buck4_voltage[7] = 1100000,	/* 1.1V */
};

void midas_power_init(void)
{
	/* do nothing */
	printk(KERN_INFO "%s\n", __func__);
}
#endif /* CONFIG_REGULATOR_MAX77686 */

void midas_power_set_muic_pdata(void *pdata, int gpio)
{
	gpio_request(gpio, "AP_PMIC_IRQ");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
}

void midas_power_gpio_init(void)
{
	/* do nothing */
}
