#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/types.h>

#define	SECURE_LOG	0
#define	DEBUG_LOG	1

#if defined(CONFIG_MACH_M0) // for M0 1 GB RAM
#define	DEBUG_LOG_START		(0x46102000)
#define	SECURE_LOG_START	(0x46202000)
#else			   // for M3 and T0 2GB RAM
#define	DEBUG_LOG_START		(0x46202000) 
#define	SECURE_LOG_START	(0x46302000)
#endif

#define	DEBUG_LOG_SIZE	(1<<20)
#define	DEBUG_LOG_MAGIC	(0xaabbccdd)
#define	DEBUG_LOG_ENTRY_SIZE	128
typedef struct debug_log_entry_s
{
    uint32_t	timestamp;          /* timestamp at which log entry was made*/
    uint32_t	logger_id;          /* id is 1 for tima, 2 for lkmauth app  */
#define	DEBUG_LOG_MSG_SIZE	(DEBUG_LOG_ENTRY_SIZE - sizeof(uint32_t) - sizeof(uint32_t))
    char	log_msg[DEBUG_LOG_MSG_SIZE];      /* buffer for the entry                 */
} __attribute__ ((packed)) debug_log_entry_t;

typedef struct debug_log_header_s
{
    uint32_t	magic;              /* magic number                         */
    uint32_t	log_start_addr;     /* address at which log starts          */
    uint32_t	log_write_addr;     /* address at which next entry is written*/
    uint32_t	num_log_entries;    /* number of log entries                */
    char	padding[DEBUG_LOG_ENTRY_SIZE - 4 * sizeof(uint32_t)];
} __attribute__ ((packed)) debug_log_header_t;

#define DRIVER_DESC   "A kernel module to read tima debug log"

unsigned long *tima_debug_log_addr = 0;
unsigned long *tima_secure_log_addr = 0;

/**
 *      tima_proc_show - Handler for writing TIMA Log into sequential Buffer
 */
static int tima_proc_debug_show(struct seq_file *m, void *v)
{
	/* Pass on the whole debug buffer to the user-space. User-space
	 * will interpret it as it chooses to.
	 */
	seq_write(m, (const char *)tima_debug_log_addr, DEBUG_LOG_SIZE);

	return 0;
}

static int tima_proc_secure_show(struct seq_file *m, void *v)
{
	/* Pass on the whole debug buffer to the user-space. User-space
	 * will interpret it as it chooses to.
	 */
	seq_write(m, (const char *)tima_secure_log_addr, DEBUG_LOG_SIZE);

	return 0;
}

/**
 *      tima_proc_open - Handler for opening sequential file interface for proc file system 
 */
static int tima_proc_open(struct inode *inode, struct file *filp)
{
	if (!strcmp(filp->f_path.dentry->d_iname, "tima_secure_log"))
		return single_open(filp, tima_proc_secure_show, NULL);
	else
		return single_open(filp, tima_proc_debug_show, NULL);
}


static const struct file_operations tima_proc_fops = {
	.open		= tima_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/**
 *      tima_debug_log_read_init -  Initialization function for TIMA
 *
 *      It creates and initializes tima proc entry with initialized read handler 
 */
static int __init tima_debug_log_read_init(void)
{
        if (proc_create("tima_debug_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_debug_log_read_init: Error creating proc entry\n");
		goto error_return;
	}

        if (proc_create("tima_secure_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_secure_log_read_init: Error creating proc entry\n");
		goto remove_debug_entry;
	}
        printk(KERN_INFO"tima_debug_log_read_init: Registering /proc/tima_debug_log Interface \n");

	tima_debug_log_addr = (unsigned long *)phys_to_virt(DEBUG_LOG_START);
	tima_secure_log_addr = (unsigned long *)phys_to_virt(SECURE_LOG_START);
        return 0;

remove_debug_entry:
        remove_proc_entry("tima_debug_log", NULL);
error_return:
	return -1;
}


/**
 *      tima_debug_log_read_exit -  Cleanup Code for TIMA
 *
 *      It removes /proc/tima proc entry and does the required cleanup operations 
 */
static void __exit tima_debug_log_read_exit(void)
{
        remove_proc_entry("tima_debug_log", NULL);
        remove_proc_entry("tima_secure_log", NULL);
        printk(KERN_INFO"Deregistering /proc/tima_debug_log Interface\n");
}


module_init(tima_debug_log_read_init);
module_exit(tima_debug_log_read_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
