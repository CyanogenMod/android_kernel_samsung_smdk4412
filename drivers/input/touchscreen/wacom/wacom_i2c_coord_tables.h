/*
 *  wacom_i2c_coord_tables.h - Wacom G5 Digitizer Controller (I2C bus)
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

#ifndef _LINUX_WACOM_I2C_COORD_TABLES_H
#define _LINUX_WACOM_I2C_COORD_TABLES_H

/* Portrait */

/* Portrait Right */
extern short CodTblX_PRight_48[];
extern short CodTblY_PRight_48[];

/* Portrait Left */
extern short CodTblX_PLeft_48[];
extern short CodTblY_PLeft_48[];

/* Landscape 1 */

/* Landscape 90 Right is same with Portrait Left */
extern short *CodTblX_LRight_48;
extern short *CodTblY_LRight_48;

/* Landscape 90 Left */
extern short CodTblX_LLeft_48[];
extern short CodTblY_LLeft_48[];


/* Landscape 2 */

/* Landscape 270 Right */
extern short *CodTblX_LRight2_48;
extern short *CodTblY_LRight2_48;

/* Landscape 270 Left */
extern short *CodTblX_LLeft2_48;
extern short *CodTblY_LLeft2_48;



extern short CodTblX_CCW_LLeft_44[];
extern short CodTblY_CCW_LLeft_44[];

extern short CodTblX_CW_LRight_44[];
extern short CodTblY_CW_LRight_44[];

extern short CodTblX_PLeft_44[];
extern short CodTblY_PLeft_44[];

extern short CodTblX_Right_44[];
extern short CodTblY_Right_44[];


extern short *tableX[MAX_HAND][MAX_ROTATION];
extern short *tableY[MAX_HAND][MAX_ROTATION];

extern short *tableX_48[MAX_HAND][MAX_ROTATION];
extern short *tableY_48[MAX_HAND][MAX_ROTATION];

extern short tilt_offsetX[MAX_HAND][MAX_ROTATION];
extern short tilt_offsetY[MAX_HAND][MAX_ROTATION];

#if defined(CONFIG_MACH_T0)
extern short tilt_offsetX_B713[MAX_HAND][MAX_ROTATION];
extern short tilt_offsetY_B713[MAX_HAND][MAX_ROTATION];
#endif

extern short tilt_offsetX_48[MAX_HAND][MAX_ROTATION];
extern short tilt_offsetY_48[MAX_HAND][MAX_ROTATION];

extern short origin_offset[2];
extern short origin_offset_48[2];

extern char* tuning_version;
extern char* tuning_version_B713;

extern char *tuning_model;

#endif /* _LINUX_WACOM_I2C_COORD_TABLES_H */
