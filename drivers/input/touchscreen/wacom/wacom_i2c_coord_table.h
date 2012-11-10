/*
 *  wacom_i2c_coord_table.h - Wacom G5 Digitizer Controller (I2C bus)
 *
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*Tables*/
#if defined(CONFIG_MACH_Q1_BD)
#include "table-q1.h"
#elif defined(CONFIG_MACH_T0)

/*Locale*/
#if defined(CONFIG_MACH_T0_EUR_OPEN) \
	|| defined(CONFIG_MACH_T0_USA_ATT) \
	|| defined(CONFIG_MACH_T0_USA_TMO)
#include "table-t03g.h"
#elif defined(CONFIG_TARGET_LOCALE_KOR)
#include "table-t0ltekors.h"
#elif defined(CONFIG_MACH_T0_JPN_LTE_DCM)
#include "table-t0ltedcm.h"
#else
#include "table-t0lte.h"
#endif

#endif


/* Origin Shift */
#if defined(CONFIG_MACH_Q1_BD)
short origin_offset[] = {600, 620};
short origin_offset_48[] = {720, 760};
#elif defined(CONFIG_MACH_T0)
short origin_offset[] = {676, 724};
#endif

/* Distance Offset Table */
short *tableX[MAX_HAND][MAX_ROTATION] = \
	{{TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44, TblX_PRight_44},
	{TblX_PRight_44, TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44} };

short *tableY[MAX_HAND][MAX_ROTATION] = \
	{{TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44, TblY_PRight_44},
	{TblY_PRight_44, TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44} };
