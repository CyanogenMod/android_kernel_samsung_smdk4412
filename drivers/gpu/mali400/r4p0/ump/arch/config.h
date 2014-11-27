/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2013 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __ARCH_CONFIG_UMP_H__
#define __ARCH_CONFIG_UMP_H__

#define ARCH_UMP_BACKEND_DEFAULT          	USING_MEMORY
#if (USING_MEMORY == 0) /* Dedicated Memory */
#define ARCH_UMP_MEMORY_ADDRESS_DEFAULT   	0x2C000000
#else
#define ARCH_UMP_MEMORY_ADDRESS_DEFAULT   	0
#endif

#define ARCH_UMP_MEMORY_SIZE_DEFAULT 		UMP_MEM_SIZE*1024*1024
#endif /* __ARCH_CONFIG_H__ */
