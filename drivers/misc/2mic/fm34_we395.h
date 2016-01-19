/* drivers/misc/2mic/fm34_we395.h
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

 #ifndef __FEM34_WE395_H__
#define __FM34_WE395_H__

#if defined(CONFIG_MACH_C1_KOR_LGT)
#include "./fm34_cmd/fm34_we395_c1_lgt.h"
#elif defined(CONFIG_MACH_BAFFIN_KOR_LGT)
#include "./fm34_cmd/fm34_we395_baffinlte_lgt.h"
#else
#include "./fm34_cmd/fm34_we395_default.h"
#endif
#endif
