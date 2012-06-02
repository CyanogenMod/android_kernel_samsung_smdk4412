/*
 * tcpal_debug.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include "tcpal_os.h"
#include "tcpal_debug.h"

module_param(tcbd_debug_class, int, 0644);

u32 tcbd_debug_class =
	DEBUG_API_COMMON |
	/*DEBUG_DRV_PERI | */
	/*DEBUG_DRV_IO |   */
	/*DEBUG_DRV_COMP | */
	/*DEBUG_DRV_RF |   */
	/*DEBUG_TCPAL_OS |   */
	/*DEBUG_TCPAL_CSPI | */
	/*DEBUG_TCPAL_I2C |  */
	/*DEBUG_TCHAL |      */
	/*DEBUG_STREAM_READ | */
	/*DEBUG_STREAM_PARSER |*/
	/*DEBUG_PARSING_PROC | */
	/*DEBUG_INTRRUPT | */
	DEBUG_INFO |
	DEBUG_ERROR;
