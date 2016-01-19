/* linux/arch/arm/mach-exynos/include/mach/memory.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS4 - Memory definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H __FILE__

#define PLAT_PHYS_OFFSET		UL(0x40000000)
#define CONSISTENT_DMA_SIZE		(SZ_8M + SZ_8M + SZ_4M)

#if defined(CONFIG_MACH_SMDKV310) || defined(CONFIG_MACH_SMDK5250)
#define NR_BANKS			16
#endif

/* Maximum of 256MiB in one bank */
#define MAX_PHYSMEM_BITS	32
#define SECTION_SIZE_BITS	28

/* Required by ION to allocate scatterlist(sglist) with nents > 256 */
#define ARCH_HAS_SG_CHAIN

#ifdef CONFIG_EXYNOS_MARK_PAGE_HOLES
#define PAGE_HOLE_EVEN_ODD	1 /* if even, 0 */
#define RAMCH_UNBALANCE_START_OFF (SZ_1G) /* + SZ_512M */
#define is_pfn_hole(pfn)						      \
	((pfn >= (PHYS_PFN_OFFSET + (RAMCH_UNBALANCE_START_OFF / PAGE_SIZE))) \
	&& ((pfn & 1) == PAGE_HOLE_EVEN_ODD))
#else
#define is_pfn_hole(pfn) 0
#endif

#endif /* __ASM_ARCH_MEMORY_H */
