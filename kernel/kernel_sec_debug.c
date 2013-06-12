/* kernel_sec_debug.c
 *
 * Exception handling in kernel by SEC
 *
 * Copyright (c) 2010 Samsung Electronics
 *                http://www.samsung.com/
 *
 */

#ifdef CONFIG_SEC_DEBUG

#include <linux/kernel_sec_common.h>
#include <asm/cacheflush.h>           /* cacheflush*/
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/file.h>
#include <mach/hardware.h>

#ifndef CONFIG_S5PV310_RAFFAELLO
#define CONFIG_S5PV310_RAFFAELLO
#endif

/*
 *  Variable
 */

const char *gkernel_sec_build_info_date_time[] = {
	__DATE__,
	__TIME__
};

#define DEBUG_LEVEL_FILE_NAME	"/mnt/.lfs/debug_level.inf"
#define DEBUG_LEVEL_RD	0
#define DEBUG_LEVEL_WR	1
static int debuglevel;

/* klaatu*/
/*sched_log_t gExcpTaskLog[SCHED_LOG_MAX];*/
unsigned int gExcpTaskLogIdx = 0;

typedef enum {
	__SERIAL_SPEED,
	__LOAD_RAMDISK,
	__BOOT_DELAY,
	__LCD_LEVEL,
	__SWITCH_SEL,
	__PHONE_DEBUG_ON,
	__LCD_DIM_LEVEL,
	__MELODY_MODE,
	__REBOOT_MODE,
	__NATION_SEL,
	__SET_DEFAULT_PARAM,
	__PARAM_INT_11,
	__PARAM_INT_12,
	__PARAM_INT_13,
	__PARAM_INT_14,
	__VERSION,
	__CMDLINE,
	__PARAM_STR_2,
	__PARAM_STR_3,
	__PARAM_STR_4
} param_idx;

char gkernel_sec_build_info[100];
unsigned int HWREV = 1;
unsigned char  kernel_sec_cause_str[KERNEL_SEC_DEBUG_CAUSE_STR_LEN];

/*
 *  Function
 */

void __iomem *kernel_sec_viraddr_wdt_reset_reg;
__used t_kernel_sec_arm_core_regsiters kernel_sec_core_reg_dump;
__used t_kernel_sec_mmu_info           kernel_sec_mmu_reg_dump;
__used kernel_sec_upload_cause_type     gkernel_sec_upload_cause;

#if defined(CONFIG_S5PV310_RAFFAELLO)  /* dpram*/
static volatile void __iomem *idpram_base;      /* Base of internal DPRAM*/

static volatile void __iomem *onedram_base = NULL;     /* Base of OneDRAM*/
static volatile unsigned int *onedram_sem;
static volatile unsigned int *onedram_mailboxAB;		/*received mail*/
static volatile unsigned int *onedram_mailboxBA;		/*send mail*/
#else
volatile void __iomem *dpram_base = 0;
volatile unsigned int *onedram_sem;
volatile unsigned int *onedram_mailboxAB;		/*received mail*/
volatile unsigned int *onedram_mailboxBA;		/*send mail*/
#endif
unsigned int received_cp_ack = 0;

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);

#if defined(CONFIG_S5PV310_RAFFAELLO)  /* dpram*/

/*
 * assigned 16K internal dpram buf for debugging
 */

#define DPRAM_BUF_SIZE 0x4000
struct _idpram_buf {
	unsigned int  dpram_start_key1;
	unsigned int  dpram_start_key2;
	unsigned char dpram_buf[DPRAM_BUF_SIZE];
	unsigned int  dpram_end_key;
} g_cdma_dpram_buf = {
	.dpram_start_key1 = 'R',
	.dpram_start_key2 = 'A',
	.dpram_buf[0] = 'N',
	.dpram_buf[1] = 'O',
	.dpram_buf[2] = 'N',
	.dpram_buf[3] = 'E',
	.dpram_end_key = 'D'
};

void kernel_sec_cdma_dpram_dump(void)
{
	printk(KERN_EMERG "Backup CDMA dpram to RAM refore upload\n");
	memcpy(g_cdma_dpram_buf.dpram_buf, (void *)idpram_base, DPRAM_BUF_SIZE);
	printk(KERN_EMERG "buf address (0x%x), dpram (0x%x)\n", \
	(unsigned int)g_cdma_dpram_buf.dpram_buf, (unsigned int) idpram_base);
}
EXPORT_SYMBOL(kernel_sec_cdma_dpram_dump);
#endif

void kernel_sec_set_cp_upload(void)
{
	unsigned int send_mail, wait_count;

#if defined(CONFIG_S5PV310_RAFFAELLO)  /* dpram*/
	static volatile u16 *cp_dpram_mbx_BA;      /*send mail box*/
	static volatile u16 *cp_dpram_mbx_AB;      /*receive mail box*/
	u16 cp_irq_mask;

	*((unsigned short *)idpram_base) = 0x554C;

	cp_dpram_mbx_BA = (volatile u16 *)(idpram_base + 0x3FFC);
	cp_dpram_mbx_AB = (volatile u16 *)(idpram_base + 0x3FFE);
	cp_irq_mask = 0xCF; /*0x80|0x40|0x0F;*/

#ifdef CDMA_IPC_C210_IDPRAM
	iowrite16(cp_irq_mask, (void *)cp_dpram_mbx_BA);
#else
	*cp_dpram_mbx_BA = cp_irq_mask;
#endif
	printk(KERN_EMERG"[kernel_sec_dump_set_cp_upload]" \
		"set cp upload mode, MailboxBA 0x%8x\n", cp_irq_mask);
	wait_count = 0;
	while (1) {
		cp_irq_mask = ioread16((void *)cp_dpram_mbx_AB);
		if (cp_irq_mask == 0xCF) {
			printk(KERN_EMERG"  - Done. cp_irq_mask: 0x%04X\n", \
				cp_irq_mask);
			break;
		}
		mdelay(10);
		if (++wait_count > 500) {
			printk(KERN_EMERG"- Fail to set CP uploadmode." \
				"cp_irq_mask: 0x%04X\n", cp_irq_mask);
			break;
			}
	}
	printk(KERN_EMERG"modem_wait_count : %d\n", wait_count);

#else
	send_mail = KERNEL_SEC_DUMP_AP_DEAD_INDICATOR;

	*onedram_sem = 0x0;
	*onedram_mailboxBA = send_mail;
	printk(KERN_EMERG"[kernel_sec_dump_set_cp_upload]" \
		"set cp upload mode, MailboxBA 0x%8x\n", send_mail);
	wait_count = 0;
	received_cp_ack = 0;
	while (1) {
		if (received_cp_ack == 1) {
			printk(KERN_EMERG"  - Done.\n");
			break;
		}
		mdelay(10);
		if (++wait_count > 500) {
			printk(KERN_EMERG"  - Fail to set CP uploadmode.\n");
			break;
		}
	}
	printk(KERN_EMERG"modem_wait_count : %d\n", wait_count);
#endif

#if defined(CONFIG_S5PV310_RAFFAELLO)  /* dpram*/
	/*
	 *  QSC6085 marking the QSC upload mode
	 */
	*((unsigned int *)idpram_base) = 0xdeaddead;
	printk(KERN_EMERG"QSC upload magic key write\n");
	kernel_sec_cdma_dpram_dump();
#endif
}
EXPORT_SYMBOL(kernel_sec_set_cp_upload);


void kernel_sec_set_cp_ack(void)   /*is set by dpram - dpram_irq_handler*/
{
	received_cp_ack = 1;
}
EXPORT_SYMBOL(kernel_sec_set_cp_ack);



void kernel_sec_set_upload_magic_number(void)
{
	int *magic_virt_addr = (int *) LOKE_BOOT_USB_DWNLD_V_ADDR;

	if ((KERNEL_SEC_DEBUG_LEVEL_MID == kernel_sec_get_debug_level()) ||
		(KERNEL_SEC_DEBUG_LEVEL_HIGH == kernel_sec_get_debug_level())) {
		*magic_virt_addr = LOKE_BOOT_USB_DWNLDMAGIC_NO; /* SET*/
		printk(KERN_EMERG"KERNEL:magic_number=0x%x" \
			"SET_UPLOAD_MAGIC_NUMBER\n", *magic_virt_addr);
	} else {
		*magic_virt_addr = 0;
		printk(KERN_EMERG"KERNEL:" \
			"magic_number=0x%x DEBUG LEVEL low!!\n", \
			*magic_virt_addr);
	}
}
EXPORT_SYMBOL(kernel_sec_set_upload_magic_number);


void kernel_sec_get_debug_level_from_boot(void)
{
	unsigned int temp;
	temp = __raw_readl(S5P_INFORM6);
	temp &= KERNEL_SEC_DEBUG_LEVEL_MASK;
	temp = temp >> KERNEL_SEC_DEBUG_LEVEL_BIT;

	if (temp == 0x0)  /*low*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;
	else if (temp == 0x1)  /*mid*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	else if (temp == 0x2)  /*high*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_HIGH;
	else {
		printk(KERN_EMERG"KERNEL:kernel_sec_get_debug_level_from_boot" \
			"(reg value is incorrect.)\n");
		/*debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	}

	printk(KERN_EMERG"KERNEL:" \
		"kernel_sec_get_debug_level_from_boot=0x%x\n", debuglevel);
}


void kernel_sec_clear_upload_magic_number(void)
{
	int *magic_virt_addr = (int *) LOKE_BOOT_USB_DWNLD_V_ADDR;

	*magic_virt_addr = 0;  /* CLEAR*/
	printk(KERN_EMERG"KERNEL:magic_number=%x " \
		"CLEAR_UPLOAD_MAGIC_NUMBER\n", *magic_virt_addr);
}
EXPORT_SYMBOL(kernel_sec_clear_upload_magic_number);

void kernel_sec_map_wdog_reg(void)
{
	/* Virtual Mapping of Watchdog register */
	kernel_sec_viraddr_wdt_reset_reg = ioremap_nocache(S3C_PA_WDT, 0x400);

	if (kernel_sec_viraddr_wdt_reset_reg == NULL) {
		printk(KERN_EMERG"Failed to ioremap()" \
			"region in forced upload keystring\n");
	}
}
EXPORT_SYMBOL(kernel_sec_map_wdog_reg);

void kernel_sec_set_upload_cause(kernel_sec_upload_cause_type uploadType)
{
	unsigned int temp;
	gkernel_sec_upload_cause = uploadType;


	temp = __raw_readl(S5P_INFORM6);
	/*KERNEL_SEC_UPLOAD_CAUSE_MASK    0x000000FF*/
	temp |= uploadType;
	__raw_writel(temp , S5P_INFORM6);
	printk(KERN_EMERG"(kernel_sec_set_upload_cause)" \
		": upload_cause set %x\n", uploadType);
}
EXPORT_SYMBOL(kernel_sec_set_upload_cause);

void kernel_sec_set_cause_strptr(unsigned char *str_ptr, int size)
{
	unsigned int temp;

	memset((void *)kernel_sec_cause_str, 0, sizeof(kernel_sec_cause_str));
	memcpy(kernel_sec_cause_str, str_ptr, size);

	temp = virt_to_phys(kernel_sec_cause_str);
	/*loke read this ptr, display_aries_upload_image*/
	__raw_writel(temp, LOKE_BOOT_USB_DWNLD_V_ADDR+4);
}
EXPORT_SYMBOL(kernel_sec_set_cause_strptr);


void kernel_sec_set_autotest(void)
{
	unsigned int temp;

	temp = __raw_readl(S5P_INFORM6);
	temp |= 1<<KERNEL_SEC_UPLOAD_AUTOTEST_BIT;
	__raw_writel(temp , S5P_INFORM6);
}
EXPORT_SYMBOL(kernel_sec_set_autotest);

void kernel_sec_set_build_info(void)
{
	char *p = gkernel_sec_build_info;
	sprintf(p, "ARIES_BUILD_INFO: HWREV: %x", HWREV);
	strcat(p, " Date:");
	strcat(p, gkernel_sec_build_info_date_time[0]);
	strcat(p, " Time:");
	strcat(p, gkernel_sec_build_info_date_time[1]);
}
EXPORT_SYMBOL(kernel_sec_set_build_info);

void kernel_sec_init(void)
{
	/*set the dpram mailbox virtual address*/
#if defined(CONFIG_S5PV310_RAFFAELLO)  /*dpram*/

	idpram_base = (volatile void *) \
	ioremap_nocache(IDPRAM_PHYSICAL_ADDR, 0x4000);
	if (idpram_base == NULL)
		printk(KERN_EMERG "failed ioremap g_idpram_region\n");
#endif
	kernel_sec_get_debug_level_from_boot();
	kernel_sec_set_upload_magic_number();
	kernel_sec_set_upload_cause(UPLOAD_CAUSE_INIT);
	kernel_sec_map_wdog_reg();
}
EXPORT_SYMBOL(kernel_sec_init);

/* core reg dump function*/
void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters *regs)
{
	asm(
		/* we will be in SVC mode when we enter this function.
			Collect SVC registers along with cmn registers.*/
		"str r0, [%0,#0]\n\t"		/*R0*/
		"str r1, [%0,#4]\n\t"		/*R1*/
		"str r2, [%0,#8]\n\t"		/*R2*/
		"str r3, [%0,#12]\n\t"		/*R3*/
		"str r4, [%0,#16]\n\t"		/*R4*/
		"str r5, [%0,#20]\n\t"		/*R5*/
		"str r6, [%0,#24]\n\t"		/*R6*/
		"str r7, [%0,#28]\n\t"		/*R7*/
		"str r8, [%0,#32]\n\t"		/*R8*/
		"str r9, [%0,#36]\n\t"		/*R9*/
		"str r10, [%0,#40]\n\t"	/*R10*/
		"str r11, [%0,#44]\n\t"	/*R11*/
		"str r12, [%0,#48]\n\t"	/*R12*/

		/* SVC */
		"str r13, [%0,#52]\n\t"	/*R13_SVC*/
		"str r14, [%0,#56]\n\t"	/*R14_SVC*/
		"mrs r1, spsr\n\t"			/*SPSR_SVC*/
		"str r1, [%0,#60]\n\t"

		/* PC and CPSR */
		"sub r1, r15, #0x4\n\t"	/*PC*/
		"str r1, [%0,#64]\n\t"
		"mrs r1, cpsr\n\t"			/*CPSR*/
		"str r1, [%0,#68]\n\t"

		/* SYS/USR */
		"mrs r1, cpsr\n\t"			/*switch to SYS mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1f\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [%0,#72]\n\t"	/*R13_USR*/
		"str r14, [%0,#76]\n\t"	/*R13_USR*/

		/*FIQ*/
		"mrs r1, cpsr\n\t"			/*switch to FIQ mode*/
		"and r1,r1,#0xFFFFFFE0\n\t"
		"orr r1,r1,#0x11\n\t"
		"msr cpsr,r1\n\t"
		"str r8, [%0,#80]\n\t"		/*R8_FIQ*/
		"str r9, [%0,#84]\n\t"		/*R9_FIQ*/
		"str r10, [%0,#88]\n\t"	/*R10_FIQ*/
		"str r11, [%0,#92]\n\t"	/*R11_FIQ*/
		"str r12, [%0,#96]\n\t"	/*R12_FIQ*/
		"str r13, [%0,#100]\n\t"	/*R13_FIQ*/
		"str r14, [%0,#104]\n\t"	/*R14_FIQ*/
		"mrs r1, spsr\n\t"			/*SPSR_FIQ*/
		"str r1, [%0,#108]\n\t"

		/*IRQ*/
		"mrs r1, cpsr\n\t"			/*switch to IRQ mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x12\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [%0,#112]\n\t"	/*R13_IRQ*/
		"str r14, [%0,#116]\n\t"	/*R14_IRQ*/
		"mrs r1, spsr\n\t"			/* SPSR_IRQ*/
		"str r1, [%0,#120]\n\t"

		/*MON*/
		"mrs r1, cpsr\n\t"	/*switch to monitor mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x16\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#124]\n\t"	/*R13_MON*/
		"str r14, [%0,#128]\n\t"	/*R14_MON*/
		"mrs r1, spsr\n\t"			/*SPSR_MON*/
		"str r1, [%0,#132]\n\t"

		/*ABT*/
		"mrs r1, cpsr\n\t"	/* switch to Abort mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x17\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#136]\n\t"	/*R13_ABT*/
		"str r14, [%0,#140]\n\t"	/* R14_ABT*/
		"mrs r1, spsr\n\t"			/* SPSR_ABT*/
		"str r1, [%0,#144]\n\t"

		/*UND*/
		"mrs r1, cpsr\n\t"	/* switch to undef mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1B\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [%0,#148]\n\t"	/* R13_UND*/
		"str r14, [%0,#152]\n\t"	/*R14_UND*/
		"mrs r1, spsr\n\t"			/*SPSR_UND*/
		"str r1, [%0,#156]\n\t"

		/* restore to SVC mode */
		"mrs r1, cpsr\n\t"	/* switch to undef mode*/
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x13\n\t"
		"msr cpsr,r1\n\t"
		:				/* output */
		: "r"(regs)	/* input */
		: "%r1"			/* clobbered register */
		);
		return;
}
EXPORT_SYMBOL(kernel_sec_get_core_reg_dump);

int kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info)
{
	asm("mrc    p15, 0, r1, c1, c0, 0\n\t"	/*SCTLR*/
		"str r1, [%0]\n\t"
		"mrc    p15, 0, r1, c2, c0, 0\n\t"	/*TTBR0*/
		"str r1, [%0,#4]\n\t"
		"mrc    p15, 0, r1, c2, c0,1\n\t"	/*TTBR1*/
		"str r1, [%0,#8]\n\t"
		"mrc    p15, 0, r1, c2, c0,2\n\t"	/*TTBCR*/
		"str r1, [%0,#12]\n\t"
		"mrc    p15, 0, r1, c3, c0,0\n\t"	/*DACR*/
		"str r1, [%0,#16]\n\t"
		"mrc    p15, 0, r1, c5, c0,0\n\t"	/*DFSR*/
		"str r1, [%0,#20]\n\t"
		"mrc    p15, 0, r1, c6, c0,0\n\t"	/*DFAR*/
		"str r1, [%0,#24]\n\t"
		"mrc    p15, 0, r1, c5, c0,1\n\t"	/*IFSR*/
		"str r1, [%0,#28]\n\t"
		"mrc    p15, 0, r1, c6, c0,2\n\t"	/*IFAR*/
		"str r1, [%0,#32]\n\t"
		/*Dont populate DAFSR and RAFSR*/
		"mrc    p15, 0, r1, c10, c2,0\n\t"	/*PMRRR*/
		"str r1, [%0,#44]\n\t"
		"mrc    p15, 0, r1, c10, c2,1\n\t"	/*NMRRR*/
		"str r1, [%0,#48]\n\t"
		"mrc    p15, 0, r1, c13, c0,0\n\t"	/*FCSEPID*/
		"str r1, [%0,#52]\n\t"
		"mrc    p15, 0, r1, c13, c0,1\n\t"	/*CONTEXT*/
		"str r1, [%0,#56]\n\t"
		"mrc    p15, 0, r1, c13, c0,2\n\t"	/*URWTPID*/
		"str r1, [%0,#60]\n\t"
		"mrc    p15, 0, r1, c13, c0,3\n\t"	/*UROTPID*/
		"str r1, [%0,#64]\n\t"
		"mrc    p15, 0, r1, c13, c0,4\n\t"	/*POTPIDR*/
		"str r1, [%0,#68]\n\t"
		:					/* output */
		: "r"(mmu_info)    /* input */
		: "%r1", "memory"         /* clobbered register */
		);
	return 0;
}
EXPORT_SYMBOL(kernel_sec_get_mmu_reg_dump);

void kernel_sec_save_final_context(void)
{
	if (kernel_sec_get_mmu_reg_dump(&kernel_sec_mmu_reg_dump) < 0)
		printk(KERN_EMERG"(kernel_sec_save_final_context) kernel_sec_get_mmu_reg_dump faile.\n");
	kernel_sec_get_core_reg_dump(&kernel_sec_core_reg_dump);

	printk(KERN_EMERG "(kernel_sec_save_final_context) Final context was saved before the system reset.\n");
}
EXPORT_SYMBOL(kernel_sec_save_final_context);


/*
 *  bSilentReset
 *    TRUE  : Silent reset - clear the magic code.
 *    FALSE : Reset to upload mode - not clear the magic code.
 *
 *  TODO : DebugLevel consideration should be added.
 */
/*extern void Ap_Cp_Switch_Config(u16 ap_cp_mode);*/
void kernel_sec_hw_reset(bool bSilentReset)
{
/*Ap_Cp_Switch_Config(0);*/

	if (bSilentReset || (KERNEL_SEC_DEBUG_LEVEL_LOW == \
			kernel_sec_get_debug_level()))	{
		kernel_sec_clear_upload_magic_number();
		printk(KERN_EMERG "(kernel_sec_hw_reset)" \
			"Upload Magic Code is cleared for silet reset.\n");
	}

	printk(KERN_EMERG "(kernel_sec_hw_reset) %s\n", gkernel_sec_build_info);

	printk(KERN_EMERG "(kernel_sec_hw_reset) The forced reset was called." \
		"The system will be reset !!\n");

	/* flush cache back to ram */
	flush_cache_all();

	__raw_writel(0x8000, kernel_sec_viraddr_wdt_reset_reg + 0x4);
	__raw_writel(0x1, kernel_sec_viraddr_wdt_reset_reg + 0x4);
	__raw_writel(0x8, kernel_sec_viraddr_wdt_reset_reg + 0x8);
	__raw_writel(0x8021, kernel_sec_viraddr_wdt_reset_reg);

    /* Never happened because the reset will occur before this. */
	while (1);
}
EXPORT_SYMBOL(kernel_sec_hw_reset);


bool kernel_set_debug_level(int level)
{
	/*if (sec_set_param_value)
	{
		if( (level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			( level == KERNEL_SEC_DEBUG_LEVEL_MID ) )
		{
			sec_set_param_value(__PHONE_DEBUG_ON, (void*)&level);
			printk(KERN_NOTICE "(kernel_set_debug_level)
			The debug value is %x !!\n", level);
			return 1;
		}
		else
		{
		printk(KERN_NOTICE "(kernel_set_debug_level)
		The debug value is invalid (%x) !!\n", level);
			return 0;
		}
	}
	else
	{*/
		printk(KERN_NOTICE "(kernel_set_debug_level)" \
		" sec_set_param_value is not intialized !!\n");
		return 0;
	/*}*/
}
EXPORT_SYMBOL(kernel_set_debug_level);

int kernel_get_debug_level()
{
	int debug_level = -1;

/*	if (sec_get_param_value)
	{
		sec_get_param_value(__PHONE_DEBUG_ON, &debug_level);
	}
*/
	if ((debug_level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			(debug_level == KERNEL_SEC_DEBUG_LEVEL_MID)) {
		printk(KERN_NOTICE "(kernel_get_debug_level) kernel"  \
			"debug level is %x !!\n", debug_level);
		return debug_level;
	}
	printk(KERN_NOTICE "(kernel_get_debug_level) kernel"  \
		"debug level is invalid (%x) !!\n", debug_level);
	return debug_level;
}
EXPORT_SYMBOL(kernel_get_debug_level);

int kernel_sec_lfs_debug_level_op(int dir, int flags)
{
	struct file *filp;
	mm_segment_t fs;

	int ret;

	filp = filp_open(DEBUG_LEVEL_FILE_NAME, flags, 0);

	if (IS_ERR(filp)) {
		pr_err("%s: filp_open failed. (%ld)\n", __func__,
				PTR_ERR(filp));

		return -1;
	}

	fs = get_fs();
	set_fs(get_ds());

	if (dir == DEBUG_LEVEL_RD)
		ret = filp->f_op->read(filp, (char __user *)&debuglevel,
				sizeof(int), &filp->f_pos);
	else
		ret = filp->f_op->write(filp, (char __user *)&debuglevel,
				sizeof(int), &filp->f_pos);

	set_fs(fs);
	filp_close(filp, NULL);

	return ret;
}

bool kernel_sec_set_debug_level(int level)
{
	int ret;

		if ((level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
				(level == KERNEL_SEC_DEBUG_LEVEL_MID) ||
				(level == KERNEL_SEC_DEBUG_LEVEL_HIGH)) {
			debuglevel = level;
			/* write to param.lfs */
			ret = kernel_sec_lfs_debug_level_op(DEBUG_LEVEL_WR, \
			O_RDWR|O_SYNC);

			if (ret == sizeof(debuglevel))
				pr_info("%s: debuglevel.inf" \
				 "write successfully.\n", \
				__func__);
				/* write to regiter (magic code) */
				kernel_sec_set_upload_magic_number();

				printk(KERN_NOTICE  \
					"(kernel_sec_set_debug_level)" \
					"The debug value is 0x%x !!\n", level);
				return 1;
		} else {
			printk(KERN_NOTICE "(kernel_sec_set_debug_level)" \
			"The debug value is" \
			"invalid(0x%x)!! Set default level(LOW)\n", level);
			debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;
			return 0;
		}
	}
EXPORT_SYMBOL(kernel_sec_set_debug_level);



int kernel_sec_get_debug_level_from_param()
{
	int ret;

	/* read from param.lfs*/
	ret = kernel_sec_lfs_debug_level_op(DEBUG_LEVEL_RD, O_RDONLY);

	if (ret == sizeof(debuglevel))
		pr_info("%s: debuglevel.inf read successfully.\n", __func__);
	if ((debuglevel == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			(debuglevel == KERNEL_SEC_DEBUG_LEVEL_MID) ||
			(debuglevel == KERNEL_SEC_DEBUG_LEVEL_HIGH)) {
		/* return debug level */
		printk(KERN_NOTICE "(kernel_sec_get_debug_level_from_param)" \
				"kernel debug level is 0x%x !!\n", debuglevel);
		return debuglevel;
	} else {
		/*In case of invalid debug level, default (debug level low)*/
		printk(KERN_NOTICE "(kernel_sec_get_debug_level_from_param)" \
		"The debug value is invalid(0x%x)!!" \
		"Set default level(LOW)\n", debuglevel);
		/*debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	}
	return debuglevel;
}
EXPORT_SYMBOL(kernel_sec_get_debug_level_from_param);

int kernel_sec_get_debug_level()
{
	return debuglevel;
}
EXPORT_SYMBOL(kernel_sec_get_debug_level);

int kernel_sec_check_debug_level_high(void)
{
	if (KERNEL_SEC_DEBUG_LEVEL_HIGH == kernel_sec_get_debug_level())
		return 1;
	return 0;
}
EXPORT_SYMBOL(kernel_sec_check_debug_level_high);

#endif /* CONFIG_KERNEL_DEBUG_SEC*/
