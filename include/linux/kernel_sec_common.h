#ifndef _KERNEL_SEC_COMMON_H_
#define _KERNEL_SEC_COMMON_H_

#include <asm/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <linux/sched.h>

/* MAGIC_CODE in LOKE
 you have to use this vitrual address with consideration */
/*#define LOKE_BOOT_USB_DWNLD_V_ADDR  0xC1000000*/
#define LOKE_BOOT_USB_DWNLD_V_ADDR  0xD1000000
#define LOKE_BOOT_USB_DWNLDMAGIC_NO 0x66262564

/* AP -> CP : AP Crash Ind */
#define KERNEL_SEC_DUMP_AP_DEAD_INDICATOR 0xABCD00C9
/* CP -> AP : CP ready for uplaod mode */
#define KERNEL_SEC_DUMP_AP_DEAD_ACK 0xCACAEDED

/*it's longer than DPRAM_ERR_MSG_LEN, in dpram.h */
#define KERNEL_SEC_DEBUG_CAUSE_STR_LEN 65
#define KERNEL_SEC_DEBUG_LEVEL_LOW	(0x574F4C44)
#define KERNEL_SEC_DEBUG_LEVEL_MID	(0x44494D44)
#define KERNEL_SEC_DEBUG_LEVEL_HIGH	(0x47494844)

/* INFORMATION REGISTER */
/*
#define S5P_INFORM0	S5P_CLKREG(0xF000)
#define S5P_INFORM1	S5P_CLKREG(0xF004)
#define S5P_INFORM2	S5P_CLKREG(0xF008)
#define S5P_INFORM3	S5P_CLKREG(0xF00C)
#define S5P_INFORM4	S5P_CLKREG(0xF010)
#define S5P_INFORM5	S5P_CLKREG(0xF014)
#define S5P_INFORM6	S5P_CLKREG(0xF018) /*Magic code for upload cause.*/
#define S5P_INFORM7	S5P_CLKREG(0xF01C)

/* WDOG register */
/*#define (S3C_PA_WDT) 0xE2700000 */

#if 1 /* dpram Physical address for the internal DPRAM in S5PC210 */
#define IDPRAM_PHYSICAL_ADDR	S5P_PA_MODEMIF
#endif
/* klaatu - schedule log */
/* #define SCHED_LOG_MAX 1000 */

typedef struct {
	void *dummy;
	void *fn;
} irq_log_t;

typedef union {
	char task[TASK_COMM_LEN];
	irq_log_t irq;
} task_log_t;

typedef struct {
	unsigned long long time;
	task_log_t log;
} sched_log_t;

/* extern sched_log_t gExcpTaskLog[SCHED_LOG_MAX]; */
extern unsigned int gExcpTaskLogIdx;

typedef struct tag_mmu_info {
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
} t_kernel_sec_mmu_info;

/*ARM CORE regs mapping structure*/
typedef struct {
	/* COMMON */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;

	/* SVC */
	unsigned int r13_svc;
	unsigned int r14_svc;
	unsigned int spsr_svc;

	/* PC & CPSR */
	unsigned int pc;
	unsigned int cpsr;

	/* USR/SYS */
	unsigned int r13_usr;
	unsigned int r14_usr;

	/* FIQ */
	unsigned int r8_fiq;
	unsigned int r9_fiq;
	unsigned int r10_fiq;
	unsigned int r11_fiq;
	unsigned int r12_fiq;
	unsigned int r13_fiq;
	unsigned int r14_fiq;
	unsigned int spsr_fiq;

	/* IRQ */
	unsigned int r13_irq;
	unsigned int r14_irq;
	unsigned int spsr_irq;

	/* MON */
	unsigned int r13_mon;
	unsigned int r14_mon;
	unsigned int spsr_mon;

	/* ABT */
	unsigned int r13_abt;
	unsigned int r14_abt;
	unsigned int spsr_abt;

	/* UNDEF */
	unsigned int r13_und;
	unsigned int r14_und;
	unsigned int spsr_und;

} t_kernel_sec_arm_core_regsiters;

typedef enum {
	UPLOAD_CAUSE_INIT           = 0x00000000,
	UPLOAD_CAUSE_KERNEL_PANIC   = 0x000000C8,
	UPLOAD_CAUSE_FORCED_UPLOAD  = 0x00000022,
	UPLOAD_CAUSE_CP_ERROR_FATAL = 0x000000CC,
	UPLOAD_CAUSE_USER_FAULT		= 0x0000002F,
#if 1  /* dpram */
	UPLOAD_CAUSE_CDMA_ERROR     = 0x000000F1,
	UPLOAD_CAUSE_CDMA_TIMEOUT   = 0x000000F2,
	UPLOAD_CAUSE_CDMA_RESET     = 0x000000F3,
#endif
	BLK_UART_MSG_FOR_FACTRST_2ND_ACK = 0x00000088,
} kernel_sec_upload_cause_type;

#define KERNEL_SEC_UPLOAD_CAUSE_MASK     0x000000FF
#define KERNEL_SEC_UPLOAD_AUTOTEST_BIT   31
#define KERNEL_SEC_UPLOAD_AUTOTEST_MASK  (1<<KERNEL_SEC_UPLOAD_AUTOTEST_BIT)

#define KERNEL_SEC_DEBUG_LEVEL_BIT   29
#define KERNEL_SEC_DEBUG_LEVEL_MASK  (3<<KERNEL_SEC_DEBUG_LEVEL_BIT)

extern void __iomem *kernel_sec_viraddr_wdt_reset_reg;
extern void kernel_sec_map_wdog_reg(void);

extern void kernel_sec_set_cp_upload(void);
extern void kernel_sec_set_cp_ack(void);
extern void kernel_sec_set_upload_magic_number(void);
extern void kernel_sec_set_upload_cause \
	(kernel_sec_upload_cause_type uploadType);
extern void kernel_sec_set_cause_strptr(unsigned char *str_ptr, int size);
extern void kernel_sec_set_autotest(void);
extern void kernel_sec_clear_upload_magic_number(void);
extern void kernel_sec_set_build_info(void);

extern void kernel_sec_hw_reset(bool bSilentReset);
extern void kernel_sec_init(void);

extern void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters *regs);
extern int  kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info);
extern void kernel_sec_save_final_context(void);

extern bool kernel_set_debug_level(int level);
extern int kernel_get_debug_level(void);
extern int kernel_get_debug_state(void);

extern bool kernel_sec_set_debug_level(int level);
extern int kernel_sec_get_debug_level_from_param(void);
extern int kernel_sec_get_debug_level(void);

#if 1  /* dpram */
extern void kernel_sec_cdma_dpram_dump(void);
#endif

#define KERNEL_SEC_LEN_BUILD_TIME 16
#define KERNEL_SEC_LEN_BUILD_DATE 16

#endif /* _KERNEL_SEC_COMMON_H_ */
