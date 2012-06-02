/*
* linux/sensor/lps331ap.h
*
* STMicroelectronics LPS331AP Pressure / Temperature Sensor module driver
*
* Copyright (C) 2010 STMicroelectronics- MSH - Motion Mems BU - Application Team
* Matteo Dameno (matteo.dameno@st.com)
* Carmine Iascone (carmine.iascone@st.com)
*
* Both authors are willing to be considered the contact and update points for
* the driver.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/
/******************************************************************************
 Revision 1.0.0 2011/Feb/14:
	first release
	moved to input/misc
 Revision 1.0.1 2011/Apr/04:
	xxx
 Revision 1.0.2 2011/Sep/15:
	corrects ord bug, forces BDU enable
 Revision 1.0.3 2011/Sep/15:
	introduces compansation params reading and sysfs file to get them
 Revision 1.0.4 2011/Dec/12:
	sets maximum allowable resolution modes dynamically with ODR;
 Revision 1.0.5 2012/Feb/29:
	introduces more compansation params and extends sysfs file content
	format to get them; reduces minimum polling period define;
 Revision 1.0.6 2012/Mar/30:
	introduces one more compansation param and extends sysfs file content
	format to get it.
******************************************************************************/

#ifndef	__LPS331AP_H__
#define	__LPS331AP_H__

#define LPS331AP_PRS_MIN_POLL_PERIOD_MS	40

#define	SAD0L				0x00
#define	SAD0H				0x01
#define	LPS331AP_PRS_I2C_SADROOT	0x2E
#define	LPS331AP_PRS_I2C_SAD_L		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0L)
#define	LPS331AP_PRS_I2C_SAD_H		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0H)
#define	LPS331AP_PRS_DEV_NAME		"lps331ap"

/* Barometer and Termometer output data rate ODR */
#define	LPS331AP_PRS_ODR_ONESH	0x00	/* one shot both		*/
#define	LPS331AP_PRS_ODR_1_1	0x10	/*  1  Hz baro,  1  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_7_7	0x50	/*  7  Hz baro,  7  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_12_12	0x60	/* 12.5Hz baro, 12.5Hz term ODR	*/
#define	LPS331AP_PRS_ODR_25_25	0x70	/* 25  Hz baro, 25  Hz term ODR	*/

/*	Pressure section defines		*/
/*	Pressure Sensor Operating Mode		*/
#define	LPS331AP_PRS_ENABLE		0x01
#define	LPS331AP_PRS_DISABLE		0x00

/*	Output conversion factors		*/
#define	SENSITIVITY_T		480	/* =	480 LSB/degrC	*/
#define	SENSITIVITY_P		4096	/* =	LSB/mbar	*/
#define	SENSITIVITY_P_SHIFT	12	/* =	4096 LSB/mbar	*/
#define	TEMPERATURE_OFFSET	42.5f	/* =	42.5 degrC	*/

#ifdef __KERNEL__
struct lps331ap_platform_data {
	int irq;
};
#endif /* __KERNEL__ */

#endif  /* __LPS331AP_H__ */
