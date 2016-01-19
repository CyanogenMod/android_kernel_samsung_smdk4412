/*
 * max77693_charger.h
 * Samsung max77693 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MAX77693_CHARGER_BAT_H
#define __MAX77693_CHARGER_BAT_H __FILE__

#define MAX77693_CHARGER_I2C_ADDR	(0xCC >> 1)
#define MAX77693_FUELGAUGE_I2C_ADDR	(0x6C >> 1)

/* MAX77693_CHG_REG_CHG_INT */
#define MAX77693_BYP_I                  (1 << 0)
#define MAX77693_THM_I                  (1 << 2)
#define MAX77693_BAT_I                  (1 << 3)
#define MAX77693_CHG_I                  (1 << 4)
#define MAX77693_CHGIN_I                (1 << 6)

/* MAX77693_CHG_REG_CHG_INT_MASK */
#define MAX77693_BYP_IM                 (1 << 0)
#define MAX77693_THM_IM                 (1 << 2)
#define MAX77693_BAT_IM                 (1 << 3)
#define MAX77693_CHG_IM                 (1 << 4)
#define MAX77693_CHGIN_IM               (1 << 6)

/* MAX77693_CHG_REG_CHG_INT_OK */
#define MAX77693_BYP_OK                 0x01
#define MAX77693_BYP_OK_SHIFT           0
#define MAX77693_BATP_OK                0x04
#define MAX77693_BATP_OK_SHIFT          2
#define MAX77693_THM_OK                 0x04
#define MAX77693_THM_OK_SHIFT           2
#define MAX77693_BAT_OK                 0x08
#define MAX77693_BAT_OK_SHIFT           3
#define MAX77693_CHG_OK                 0x10
#define MAX77693_CHG_OK_SHIFT           4
#define MAX77693_WCIN_OK                0x20
#define MAX77693_WCIN_OK_SHIFT          5
#define MAX77693_CHGIN_OK               0x40
#define MAX77693_CHGIN_OK_SHIFT         6
#define MAX77693_DETBAT                 0x80
#define MAX77693_DETBAT_SHIFT           7

/* MAX77693_CHG_REG_CHG_DTLS_00 */
#define MAX77693_BATP_DTLS              0x01
#define MAX77693_BATP_DTLS_SHIFT        0
#define MAX77693_WCIN_DTLS              0x18
#define MAX77693_WCIN_DTLS_SHIFT        3
#define MAX77693_THM_DTLS               0x07
#define MAX77693_THM_DTLS_SHIFT         0
#define MAX77693_CHGIN_DTLS             0x60
#define MAX77693_CHGIN_DTLS_SHIFT       5

/* MAX77693_CHG_REG_CHG_DTLS_01 */
#define MAX77693_CHG_DTLS               0x0F
#define MAX77693_CHG_DTLS_SHIFT         0
#define MAX77693_BAT_DTLS               0x70
#define MAX77693_BAT_DTLS_SHIFT         4

/* MAX77693_CHG_REG_CHG_DTLS_02 */
#define MAX77693_BYP_DTLS               0x0F
#define MAX77693_BYP_DTLS_SHIFT         0
#define MAX77693_BYP_DTLS0      	0x1
#define MAX77693_BYP_DTLS1      	0x2
#define MAX77693_BYP_DTLS2      	0x4
#define MAX77693_BYP_DTLS3      	0x8

/* MAX77693_CHG_REG_CHG_CNFG_00 */
#define MAX77693_MODE_DEFAULT   	0x04
#define MAX77693_MODE_CHGR      	0x01
#define MAX77693_MODE_OTG       	0x02
#define MAX77693_MODE_BUCK      	0x04

/* MAX77693_CHG_REG_CHG_CNFG_02 */
#define MAX77693_CHG_CC         	0x3F
#define MAX77693_OTG_ILIM         	0x80

/* MAX77693_CHG_REG_CHG_CNFG_03 */
#define MAX77693_CHG_TO_ITH             0x07

/* MAX77693_CHG_REG_CHG_CNFG_04 */
#define MAX77693_CHG_MINVSYS_MASK       0xE0
#define MAX77693_CHG_MINVSYS_SHIFT      5
#define MAX77693_CHG_PRM_MASK           0x1F
#define MAX77693_CHG_PRM_SHIFT          0

/* MAX77693_CHG_REG_CHG_CNFG_09 */
#define MAX77693_CHG_CHGIN_LIM  	0x7F

/* MAX77693_CHG_REG_CHG_CNFG_12 */
#define MAX77693_CHG_WCINSEL            0x40

/* MAX77693_MUIC_REG_CDETCTRL1 */
#define MAX77693_CHGTYPMAN              0x02
#define MAX77693_CHGTYPMAN_SHIFT        1

/* MAX77693_MUIC_REG_STATUS2 */
#define MAX77693_VBVOLT                 0x40
#define MAX77693_VBVOLT_SHIFT           6
#define MAX77693_CHGDETRUN              0x08
#define MAX77693_CHGDETRUN_SHIFT        3
#define MAX77693_CHGTYPE                0x07
#define MAX77693_CHGTYPE_SHIFT          0

/* irq */
#define IRQ_DEBOUNCE_TIME       20      /* msec */

/* charger type detection */
#define DET_ERR_RETRY   5
#define DET_ERR_DELAY   200

/* soft charging */
#define SOFT_CHG_START_CURR     100     /* mA */
#define SOFT_CHG_START_DUR      100     /* ms */
#define SOFT_CHG_CURR_STEP      100     /* mA */
#define SOFT_CHG_STEP_DUR       20      /* ms */

#define DEFAULT_AC_CURRENT	1600	/* mA */
#define DEFAULT_USB_CURRENT	500	/* mA */

/*
enum {
        POWER_SUPPLY_VBUS_UNKNOWN = 0,
        POWER_SUPPLY_VBUS_UVLO,
        POWER_SUPPLY_VBUS_WEAK,
        POWER_SUPPLY_VBUS_OVLO,
        POWER_SUPPLY_VBUS_GOOD,
};
*/

struct sec_chg_info {
        bool dummy;
};

extern sec_battery_platform_data_t sec_battery_pdata;
#endif
