/*
 * Driver model for sensor
 *
 * Copyright (C) 2008 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_SENSORS_AXES_S16_H_INCLUDED
#define __LINUX_SENSORS_AXES_S16_H_INCLUDED

#include "sensors_axes.h"

static void axes_pxpypz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pynxpz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nxnypz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nypxpz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pypxnz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pxnynz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nxpynz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nynxnz_s16(s16 *x, s16 *y, s16 *z);

static void axes_pxpynz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pynxnz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nxnynz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nypxnz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nxpypz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pypxpz_s16(s16 *x, s16 *y, s16 *z);
static void axes_pxnypz_s16(s16 *x, s16 *y, s16 *z);
static void axes_nynxpz_s16(s16 *x, s16 *y, s16 *z);

axes_func_s16 select_func_s16(u8 position)
{
	switch (position) {
	case AXES_PXPYPZ:
		pr_info("%s, AXES_PXPYPZ\n", __func__);
		return (axes_func_s16)axes_pxpypz_s16;
	case AXES_PYNXPZ:
		pr_info("%s, AXES_PYNXPZ\n", __func__);
		return (axes_func_s16)axes_pynxpz_s16;
	case AXES_NXNYPZ:
		pr_info("%s, AXES_NXNYPZ\n", __func__);
		return (axes_func_s16)axes_nxnypz_s16;
	case AXES_NYPXPZ:
		pr_info("%s, AXES_NYPXPZ\n", __func__);
		return (axes_func_s16)axes_nypxpz_s16;
	case AXES_NXPYNZ:
		pr_info("%s, AXES_NXPYNZ\n", __func__);
		return (axes_func_s16)axes_nxpynz_s16;
	case AXES_PYPXNZ:
		pr_info("%s, AXES_PYPXNZ\n", __func__);
		return (axes_func_s16)axes_pypxnz_s16;
	case AXES_PXNYNZ:
		pr_info("%s, AXES_PXNYNZ\n", __func__);
		return (axes_func_s16)axes_pxnynz_s16;
	case AXES_NYNXNZ:
		pr_info("%s, AXES_NYNXNZ\n", __func__);
		return (axes_func_s16)axes_nynxnz_s16;

	case AXES_PXPYNZ:
		pr_info("%s, AXES_PXPYNZ\n", __func__);
		return (axes_func_s16)axes_pxpynz_s16;
	case AXES_PYNXNZ:
		pr_info("%s, AXES_PYNXNZ\n", __func__);
		return (axes_func_s16)axes_pynxnz_s16;
	case AXES_NXNYNZ:
		pr_info("%s, AXES_NXNYNZ\n", __func__);
		return (axes_func_s16)axes_nxnynz_s16;
	case AXES_NYPXNZ:
		pr_info("%s, AXES_NYPXNZ\n", __func__);
		return (axes_func_s16)axes_nypxnz_s16;
	case AXES_NXPYPZ:
		pr_info("%s, AXES_NXPYPZ\n", __func__);
		return (axes_func_s16)axes_nxpypz_s16;
	case AXES_PYPXPZ:
		pr_info("%s, AXES_PYPXPZ\n", __func__);
		return (axes_func_s16)axes_pypxpz_s16;
	case AXES_PXNYPZ:
		pr_info("%s, AXES_PXNYPZ\n", __func__);
		return (axes_func_s16)axes_pxnypz_s16;
	case AXES_NYNXPZ:
		pr_info("%s, AXES_NYNXPZ\n", __func__);
		return (axes_func_s16)axes_nynxpz_s16;
	default:
		pr_info("%s, AXES_PXPYPZ\n", __func__);
		return (axes_func_s16)axes_pxpypz_s16;
	}
}

static void axes_pxpypz_s16(s16 *x, s16 *y, s16 *z)
{
}

static void axes_pynxpz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = origin_y;
	*y = -origin_x;
}

static void axes_nxnypz_s16(s16 *x, s16 *y, s16 *z)
{
	*x = -*x;
	*y = -*y;
}

static void axes_nypxpz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = -origin_y;
	*y = origin_x;
}

static void axes_nxpynz_s16(s16 *x, s16 *y, s16 *z)
{
	*x = -*x;
	*z = -*z;
}

static void axes_pypxnz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = origin_y;
	*y = origin_x;
	*z = -*z;
}

static void axes_pxnynz_s16(s16 *x, s16 *y, s16 *z)
{
	*y = -*y;
	*z = -*z;
}

static void axes_nynxnz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = -origin_y;
	*y = -origin_x;
	*z = -*z;
}

static void axes_pxpynz_s16(s16 *x, s16 *y, s16 *z)
{
	*z = -*z;
}

static void axes_pynxnz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = origin_y;
	*y = -origin_x;
	*z = -*z;
}

static void axes_nxnynz_s16(s16 *x, s16 *y, s16 *z)
{
	*x = -*x;
	*y = -*y;
	*z = -*z;
}

static void axes_nypxnz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = -origin_y;
	*y = origin_x;
	*z = -*z;
}

static void axes_nxpypz_s16(s16 *x, s16 *y, s16 *z)
{
	*x = -*x;
}

static void axes_pypxpz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = origin_y;
	*y = origin_x;
}

static void axes_pxnypz_s16(s16 *x, s16 *y, s16 *z)
{
	*y = -*y;
}

static void axes_nynxpz_s16(s16 *x, s16 *y, s16 *z)
{
	s16 origin_x = *x, origin_y = *y;

	*x = -origin_y;
	*y = -origin_x;
}
#endif	/* __LINUX_SENSORS_AXES_S16_H_INCLUDED */
