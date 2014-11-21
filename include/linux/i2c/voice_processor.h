/* include/linux/i2c/voice_processor.h - voice processor driver
 *
 * Copyright (C) 2012 Samsung Corporation.
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

#ifndef __VOICE_PROCESSOR_H__
#define __VOICE_PROCESSOR_H__

enum voice_processing_mode {
	VOICE_NS_BYPASS_MODE = 0,
	VOICE_NS_HANDSET_MODE,
	VOICE_NS_LOUD_MODE,
	VOICE_NS_FTM_LOOPBACK_MODE,
	NUM_OF_VOICE_NS_MODE,
};

#endif
