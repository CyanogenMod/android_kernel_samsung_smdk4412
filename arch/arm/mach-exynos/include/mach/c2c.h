/* linux/arch/arm/mach-exynos/include/mach/c2c.h
 *
 * Copyright 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Platform header file for Samsung C2C Interface driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __MACH_C2C_H__
#define __MACH_C2C_H__ __FILE__

#include <linux/io.h>
#include <mach/regs-pmu.h>
#include <mach/pmu.h>

#define C2C_SFN			S3C_GPIO_SFN(2)

#define EXYNOS5_PA_ETC8		0x10D100A0

#define C2C_SYSREG_DEFAULT	0x832AA803

#define C2C_SHAREDMEM_BASE	0x60000000
#if defined(CONFIG_C2C_IPC_ONLY)
#define C2C_SHAREDMEM_SIZE	SZ_4M
#else
#define C2C_SHAREDMEM_SIZE	SZ_64M
#endif

#if defined(CONFIG_C2C_IPC_ONLY)
#define C2C_SH_RGN_SIZE		SZ_4M
#define C2C_CP_RGN_SIZE		0
#elif defined(CONFIG_C2C_IPC_ENABLE)
#define C2C_SH_RGN_SIZE		SZ_4M
#define C2C_CP_RGN_SIZE		(C2C_SHAREDMEM_SIZE - C2C_SH_RGN_SIZE)
#else
#define C2C_SH_RGN_SIZE		0
#define C2C_CP_RGN_SIZE		C2C_SHAREDMEM_SIZE
#endif

#define C2C_SH_RGN_OFFSET	C2C_CP_RGN_SIZE

extern unsigned long c2c_get_phys_base(void);
extern unsigned long c2c_get_phys_size(void);
extern unsigned long c2c_get_sh_rgn_offset(void);
extern unsigned long c2c_get_sh_rgn_size(void);

extern void c2c_assign_gpio_ap_wakeup(unsigned int gpio);
extern void c2c_assign_gpio_ap_status(unsigned int gpio);
extern void c2c_assign_gpio_cp_wakeup(unsigned int gpio);
extern void c2c_assign_gpio_cp_status(unsigned int gpio);
extern bool c2c_connected(void);
extern int c2c_suspend(void);
extern bool c2c_suspended(void);
extern int c2c_resume(void);

#ifdef CONFIG_C2C_IPC_ENABLE
extern void __iomem *c2c_request_cp_region(unsigned int cp_addr,
		unsigned int size);
extern void __iomem *c2c_request_sh_region(unsigned int sh_addr,
		unsigned int size);
extern void c2c_release_cp_region(void *rgn);
extern void c2c_release_sh_region(void *rgn);

extern int c2c_register_handler(void (*handler)(void *, u32), void *data);
extern int c2c_unregister_handler(void (*handler)(void *, u32));
extern void c2c_send_interrupt(u32 cmd);
extern void c2c_reset_interrupt(void);
extern u32 c2c_read_interrupt(void);
extern u32 c2c_read_link(void);
extern void c2c_reload(void);

struct c2c_ipc_handler {
	void *data;
	void (*handler)(void *, u32);
};
#endif

enum c2c_opp_mode {
	C2C_OPP0 = 0,
	C2C_OPP25 = 1,
	C2C_OPP50 = 2,
	C2C_OPP100 = 3,
};

enum c2c_buswidth {
	C2C_BUSWIDTH_8 = 0,
	C2C_BUSWIDTH_10 = 1,
	C2C_BUSWIDTH_16 = 2,
};

enum c2c_shrdmem_size {
	C2C_MEMSIZE_4 = 0,
	C2C_MEMSIZE_8 = 1,
	C2C_MEMSIZE_16 = 2,
	C2C_MEMSIZE_32 = 3,
	C2C_MEMSIZE_64 = 4,
	C2C_MEMSIZE_128 = 5,
	C2C_MEMSIZE_256 = 6,
	C2C_MEMSIZE_512 = 7,
};

struct exynos_c2c_platdata {
	void (*setup_gpio)(enum c2c_buswidth rx_width,
			enum c2c_buswidth tx_width);
	void (*set_cprst)(void);
	void (*clear_cprst)(void);
	u32 (*get_c2c_state)(void);

	dma_addr_t shdmem_addr;
	enum c2c_shrdmem_size shdmem_size;

	void __iomem *ap_sscm_addr;
	void __iomem *cp_sscm_addr;

	enum c2c_buswidth rx_width;
	enum c2c_buswidth tx_width;
	u32 clk_opp100;	/* clock of OPP100 mode */
	u32 clk_opp50;	/* clock of OPP50 mode */
	u32 clk_opp25;	/* clock of OPP25 */
	enum c2c_opp_mode default_opp_mode;

	/* System Register address for C2C */
	void __iomem *c2c_sysreg;

	char *c2c_clk;
};

#endif /*__MACH_C2C_H__*/

