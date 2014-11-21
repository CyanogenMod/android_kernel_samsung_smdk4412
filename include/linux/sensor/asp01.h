/*
 *  AD Semiconductor grip sensor driver
 *
 *  Copyright (C) 2012 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef __ASP01_GRIP_HEADER__
#define __ASP01_GRIP_HEADER__

/* order of init code */
enum ASP01_INIT_CODE {
	SET_UNLOCK = 0,
	SET_RST_ERR,
	SET_PROX_PER,
	SET_PAR_PER,
	SET_TOUCH_PER,
	SET_HI_CAL_PER,
	SET_BSMFM_SET,
	SET_ERR_MFM_CYC,
	SET_TOUCH_MFM_CYC,
	SET_HI_CAL_SPD,
	SET_CAL_SPD,
	SET_BFT_MOT,
	SET_TOU_RF_EXT,
	SET_SYS_FUNC,
	SET_OFF_TIME,
	SET_SENSE_TIME,
	SET_DUTY_TIME,
	SET_HW_CON1,
	SET_HW_CON2,
	SET_HW_CON3,
	SET_HW_CON4,
	SET_HW_CON5,
	SET_HW_CON6,
	SET_HW_CON7,
	SET_HW_CON8,
	SET_HW_CON9,
	SET_HW_CON10,
	SET_HW_CON11,
	SET_REG_NUM,
};

struct asp01_platform_data {
	int t_out; /* touch int pin output */
	int cr_divsr;
	int cr_divnd;
	int cs_divsr;
	int cs_divnd;
	int irq;
	u8 init_code[SET_REG_NUM];
};

#endif
