/************************************************************
 * /drivers/media/isdbt/isdbt.c
 *
 *        ISDBT DRIVER
 *
 *
 *       Copyright (c) 2011 Samsung Electronics
 *
 *                       http://www.samsung.com
 *
 ***********************************************************/
#ifndef	__ISDBT_H__
#define __ISDBT_H__

/*	#define	DEBUG_MSG_SPI	*/

#ifdef DEBUG_MSG_SPI
#define	SUBJECT "ISDBT-SPI-DRIVER"
#define	I_DEV_DBG(format, ...) \
		printk("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__)

#else
#define	I_DEV_DBG(format, ...)
#endif

#endif
