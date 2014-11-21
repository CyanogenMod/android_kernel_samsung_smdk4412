 /*
  * Copyright (C) 2008 Samsung Electronics, Inc.
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

#ifndef __S5PC210_MC1N2_H
#define __S5PC210_MC1N2_H

enum mic_type {
	MAIN_MIC = 0x1,
	SUB_MIC  = 0x2,
};

struct mc1n2_platform_data {
	void (*set_main_mic_bias)(bool on);
	void (*set_sub_mic_bias)(bool on);
	int (*set_adc_power_constraints)(int disabled);
};

#endif
