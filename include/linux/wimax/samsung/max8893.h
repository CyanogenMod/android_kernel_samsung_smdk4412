/*
*
*	MAX8893 PMIC driver for WiMAX with CMC732.
*	This is not a regulator driver.
*/

#ifndef MAX8893_H
#define MAX8993_H __FILE__
/*
*
*
*		Default/Reset values of MAX8893C registers
*
*
*
*/
/*
#define DEF_VAL_MAX8893_REG_ONOFF 0x01
#define DEF_VAL_MAX8893_REG_DISCHARGE	0xff
#define DEF_VAL_MAX8893_REG_LSTIME	0x08
#define DEF_VAL_MAX8893_REG_DVSRAMP	0x09
#define DEF_VAL_MAX8893_REG_BUCK	0x02
#define DEF_VAL_MAX8893_REG_LDO1	0x02
#define DEF_VAL_MAX8893_REG_LDO2	0x0e
#define DEF_VAL_MAX8893_REG_LDO3	0x11
#define DEF_VAL_MAX8893_REG_LDO4	0x19
#define DEF_VAL_MAX8893_REG_LDO5	0x16
*/
/*
*
*		Register address of MAX8893 A/B/C
*		BUCK is marked as LDO "-1"
*/

#define BUCK (-1)
#define LDO1 1
#define LDO2 2
#define LDO3 3
#define LDO4 4
#define LDO5 5
#define DISABLE_USB 6
#define MAX8893_REG_ONOFF 0x00
#define MAX8893_REG_DISCHARGE	0x01
#define MAX8893_REG_LSTIME	0x02
#define MAX8893_REG_DVSRAMP	0x03
#define MAX8893_REG_LDO(x) ((x+1) ? (4+x) : 4)
#define ON 1
#define OFF 0

/*
*	The maximum and minimum voltage an LDO can provide
*	Buck, x = -1
*/

#define MAX_VOLTAGE(x) ((x+1) ? 3300 : 2400)
#define MIN_VOLTAGE(x) ((0x04&x) ? 800 : ((0x01&x) ? 1600 : 1200))
/*
*
*
*ENABLE_LDO(x) generates a mask which needs
*to be ORed with the contents of onoff reg
*
*DISABLE_LDO(x) generates a mask which needs
*to be ANDed with contents of the off reg
*
*For BUCK, x=-1
*/
#define ENABLE_LDO(x) (0x80>>(x+1))
#define DISABLE_LDO(x) (~(0x80>>(x+1)))

int wimax_pmic_set_voltage(void);
#endif
