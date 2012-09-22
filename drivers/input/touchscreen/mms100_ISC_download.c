/*--------------------------------------------------------
*
*
*      Melfas MMS100 Series Download base v1.7 2011.09.23
*
*/
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/fcntl.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/syscalls.h>

#include "mms100_ISP_download.h"

/*============================================================
*
*      Include MELFAS Binary code File ( ex> MELFAS_FIRM_bin.c)
*
*      Warning!!!!
*              Please, don't add binary.c file into project
*              Just #include here !!
*
*============================================================
*/
extern const UINT16 MELFAS_binary_nLength;
extern const UINT8 MELFAS_binary[];
extern const UINT16 MELFAS_binary_nLength_4_65;
extern const UINT8 MELFAS_binary_4_65[];

extern const UINT16 MELFAS_binary_nLength_2;
extern const UINT8 MELFAS_binary_2[];

UINT8 ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
UINT8 ucInitial_Download = FALSE;
#define MELFAS_FW1 "/sdcard/midas_firmware.bin"

extern void mcsdl_delay(UINT32 nCount);

#if defined(CONFIG_MACH_M0_CHNOPEN) ||					\
	defined(CONFIG_MACH_M0_CMCC) || defined(CONFIG_MACH_M0_CTC)
extern unsigned int lcdtype;
#endif
/*
*---------------------------------
*      Downloading functions
*---------------------------------
*/
static int mms100_ISC_download(const UINT8 * pBianry, const UINT16 unLength,
			       const UINT8 nMode,
			       struct melfas_tsi_platform_data *ts_data);

static void mms100_ISC_set_ready(struct melfas_tsi_platform_data *ts_data);
static void mms100_ISC_reboot_mcs(struct melfas_tsi_platform_data *ts_data);

static UINT8 mcsdl_read_ack(void);
static void mcsdl_ISC_read_32bits(UINT8 * pData);
static void mcsdl_ISC_write_bits(UINT32 wordData, int nBits);
static UINT8 mms100_ISC_read_data(UINT8 addr);

static void mms100_ISC_enter_download_mode(void);
static void mms100_ISC_firmware_update_mode_enter(void);
static UINT8 mms100_ISC_firmware_update(UINT8 * _pBinary_reordered,
					UINT16 _unDownload_size,
					UINT8 flash_start, UINT8 flash_end);
static UINT8 mms100_ISC_read_firmware_status(void);
static int mms100_ISC_Slave_download(UINT8 nMode);
static void mms100_ISC_slave_download_start(UINT8 nMode);
static UINT8 mms100_ISC_slave_crc_ok(void);
static void mms100_ISC_leave_firmware_update_mode(void);
static void mcsdl_i2c_start(void);
static void mcsdl_i2c_stop(void);
static UINT8 mcsdl_read_byte(void);
/*
*---------------------------------
*      For debugging display
*---------------------------------
*/
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_ISC_print_result(int nRet);
#endif
/*
*----------------------------------
* Download enable command
*----------------------------------
*/
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD

void melfas_send_download_enable_command(void)
{
	/* TO DO : Fill this up */

}

#endif

int mms100_download(struct melfas_tsi_platform_data *ts_data)
{
	int ret = 0;
	int i = 0;

#if MELFAS_ISP_DOWNLOAD
	ret = mms100_ISP_download_binary_data(MELFAS_ISP_DOWNLOAD, ts_data);
	if (ret)
		printk("<MELFAS> SET Download ISP Fail\n");
#else
	ret = mms100_ISC_download_binary_data(ts_data);
	if (ret) {
		printk("<MELFAS> SET Download ISC Fail\n");
		for (i = 0; i < 3; i++) {
			if (ret) {
				ret = mms100_ISP_download_binary_data(MELFAS_ISP_DOWNLOAD, ts_data);	/*ISP mode download ( CORE + PRIVATE ) */
				printk("<MELFAS> SET Download ISC Fail\n");
			}
			if (!ret) {
				ucInitial_Download = TRUE;
				ret = mms100_ISC_download_binary_data(ts_data);	/*retry ISC mode download */
			}
			if (ret)
				printk
				    ("<MELFAS> SET Download ISC & ISP Fail\n");
			else
				break;
		}
	}
#endif
	return ret;
}

int mms100_download_file(struct melfas_tsi_platform_data *ts_data)
{
	int ret = 0;
	int i = 0;

#if MELFAS_ISP_DOWNLOAD
	ret = mms100_ISP_download_binary_file(ts_data);
	if (ret)
		printk("<MELFAS> SET Download ISP Fail\n");
#else
	ret = mms100_ISC_download_binary_file(ts_data);
	if (ret) {
		printk("<MELFAS> SET Download ISC Fail\n");
		for (i = 0; i < 3; i++) {
			if (ret) {
				ret = mms100_ISP_download_binary_file(ts_data);	/*ISP mode download ( CORE + PRIVATE ) */
				printk("<MELFAS> SET Download ISC Fail\n");
			}
			if (!ret) {
				ucInitial_Download = TRUE;
				ret = mms100_ISC_download_binary_file(ts_data);	/*retry ISC mode download */
			}
			if (ret)
				printk
				    ("<MELFAS> SET Download ISC & ISP Fail\n");
			else
				break;
		}
	}
#endif
	return ret;
}

/*
*============================================================
*
*      Main Download furnction
*
*   1. Run mms100_ISC_download(*pBianry,unLength, nMode)
*       nMode : 0 (Core download)
*       nMode : 1 (Private Custom download)
*       nMode : 2 (Public Custom download)
*
*============================================================
*/
int mms100_ISC_download_binary_data(struct melfas_tsi_platform_data *ts_data)
{

	int i, nRet;
	INT8 dl_enable_bit = 0x00;
	INT8 version_info = 0;

#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
	melfas_send_download_enable_command();
	mcsdl_delay(MCSDL_DELAY_100US);
#endif

	MELFAS_DISABLE_BASEBAND_ISR();	/* Disable Baseband touch interrupt ISR. */
	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();	/* Disable Baseband watchdog timer */

	/*
	 *---------------------------------
	 * set download enable mode
	 *---------------------------------
	 */

	if (ucInitial_Download)
		ucSlave_addr = ISC_DEFAULT_SLAVE_ADDR;
	if (MELFAS_CORE_FIRWMARE_UPDATE_ENABLE || ucInitial_Download) {
		version_info =
		    mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_CORE);
		printk("<MELFAS> CORE_VERSION : 0x%2X\n", version_info);
/*              if(version_info< MELFAS_DOWNLAOD_CORE_VERSION || version_info==0xFF)*/
		dl_enable_bit |= 0x01;
	}
	if (MELFAS_PRIVATE_CONFIGURATION_UPDATE_ENABLE) {
		version_info =
		    mms100_ISC_read_data
		    (MELFAS_FIRMWARE_VER_REG_PRIVATE_CUSTOM);
		printk("<MELFAS> PRIVATE_CUSTOM_VERSION : 0x%2X\n",
		       version_info);
		if (version_info < MELFAS_DOWNLAOD_PRIVATE_VERSION
		    || version_info == 0xFF)
			dl_enable_bit |= 0x02;
	}
	if (MELFAS_PUBLIC_CONFIGURATION_UPDATE_ENABLE) {
		version_info =
		    mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PUBLIC_CUSTOM);
		printk("<MELFAS> PUBLIC_CUSTOM_VERSION : 0x%2X\n",
		       version_info);
		if (version_info < MELFAS_DOWNLAOD_PUBLIC_VERSION
		    || version_info == 0xFF)
			dl_enable_bit |= 0x04;
	}
	/*
	 *------------------------
	 * Run Download
	 *------------------------
	 */
	for (i = 0; i < 3; i++) {
		if (dl_enable_bit & (1 << i)) {
			if (i < 2) {	/* 0: core, 1: private custom */

#if defined(CONFIG_MACH_M0_CHNOPEN) || defined(CONFIG_MACH_M0_CTC)
			if (lcdtype == 0x20) {
				nRet = mms100_ISC_download((const UINT8*)
				MELFAS_binary_4_65,
				(const UINT16)MELFAS_binary_nLength_4_65,
				(const INT8)i, ts_data);

				printk(KERN_DEBUG "<MELFAS> lcdtype is 4.8 inch. ISC firmware update with 4.65 firmware file\n");
			} else {
				nRet = mms100_ISC_download((const UINT8*)
				MELFAS_binary,
				(const UINT16)MELFAS_binary_nLength,
				(const INT8)i, ts_data);

				printk(KERN_DEBUG "<MELFAS> lcdtype is 4.65 inch. ISC firmware update with 4.8 firmware file\n");
			}
			ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
			ucInitial_Download = FALSE;
#elif defined(CONFIG_MACH_M0_CMCC)
			if (lcdtype == 0x20) {
				nRet = mms100_ISC_download((const UINT8*)
				MELFAS_binary,
				(const UINT16)MELFAS_binary_nLength,
				(const INT8)i, ts_data);

				printk(KERN_DEBUG "<MELFAS> lcdtype is 4.8 inch. ISC firmware update with 4.8 firmware file\n");
			} else {
				nRet = mms100_ISC_download((const UINT8*)
				MELFAS_binary_4_65,
				(const UINT16)MELFAS_binary_nLength_4_65,
				(const INT8)i, ts_data);

				printk(KERN_DEBUG "<MELFAS> lcdtype is 4.65 inch. ISC firmware update with 4.65 firmware file\n");
			}
			ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
			ucInitial_Download = FALSE;
#else

#if defined(CONFIG_MACH_M0)
			if (system_rev == 2 || system_rev >= 5)
#else
			if (system_rev == 2 || system_rev >= 4)
#endif
				nRet = mms100_ISC_download((const UINT8 *)
							   MELFAS_binary,
							   (const UINT16)
							   MELFAS_binary_nLength, (const INT8)i, ts_data);
			else
				nRet = mms100_ISC_download((const UINT8 *)
							   MELFAS_binary_4_65,
							   (const UINT16)
							   MELFAS_binary_nLength_4_65, (const INT8)i, ts_data);
				ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
				ucInitial_Download = FALSE;
#endif
			}
			/*    else      * 2: public custom
			   nRet = mms100_ISC_download( (const UINT8*) MELFAS_binary_2, (const UINT16)MELFAS_binary_nLength_2, (const INT8)i, ts_data); */
			if (nRet)
				goto fw_error;
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
			nRet = mms100_ISC_Slave_download((const INT8)i, ts_data);	/* Slave Binary data download */
			if (nRet)
				goto fw_error;
#endif
		}
	}

	MELFAS_ROLLBACK_BASEBAND_ISR();	/* Roll-back Baseband touch interrupt ISR. */
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();	/* Roll-back Baseband watchdog timer */
	return 0;

 fw_error:
/*      mcsdl_erase_flash(0);
	mcsdl_erase_flash(1);*/
	return nRet;

}

int mms100_ISC_download_binary_file(struct melfas_tsi_platform_data *ts_data)
{
	int i;
	INT8 dl_enable_bit = 0x00;
	INT8 version_info = 0;

	UINT8 *pBinary[2] = { NULL, NULL };
	UINT16 nBinary_length[2] = { 0, 0 };
	UINT8 IdxNum = MELFAS_2CHIP_DOWNLOAD_ENABLE;
	/*
	 *==================================================
	 *
	 *      1. Read '.bin file'
	 *   2. *pBinary[0]       : Binary data(Core + Private Custom)
	 *       *pBinary[1]       : Binary data(Public)
	 *         nBinary_length[0] : Firmware size(Core + Private Custom)
	 *         nBinary_length[1] : Firmware size(Public)
	 *   3. Run mms100_ISC_download(*pBianry,unLength, nMode)
	 *       nMode : 0 (Core download)
	 *       nMode : 1 (Private Custom download)
	 *       nMode : 2 (Public Custom download)
	 *
	 *==================================================
	 */
#if 1

	int nRet = 0;
/*		int retry_cnt = 0;*/
	long fw1_size = 0;
	unsigned char *fw_data1;
	struct file *filp;
	loff_t pos;
	int ret = 0;
	mm_segment_t oldfs;
/*		spinlock_t	lock;*/

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MELFAS_FW1, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("[TSP]file open error:%d\n", (s32) filp);
		return -1;
	}

	fw1_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("[TSP]Size of the file : %ld(bytes)\n", fw1_size);

	fw_data1 = kmalloc(fw1_size, GFP_KERNEL);
	memset(fw_data1, 0, fw1_size);

	pos = 0;
	memset(fw_data1, 0, fw1_size);
	ret = vfs_read(filp, (char __user *)fw_data1, fw1_size, &pos);

	if (ret != fw1_size) {
		pr_err("[TSP]Failed to read file %s (ret = %d)\n", MELFAS_FW1,
		       ret);
		kfree(fw_data1);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(oldfs);

#endif

#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
	melfas_send_download_enable_command();
	mcsdl_delay(MCSDL_DELAY_100US);
#endif

	MELFAS_DISABLE_BASEBAND_ISR();	/* Disable Baseband touch interrupt ISR. */
	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();	/* Disable Baseband watchdog timer */

	for (i = 0; i <= IdxNum; i++) {
		if (pBinary[i] != NULL && nBinary_length[i] > 0
		    && nBinary_length[i] < MELFAS_FIRMWARE_MAX_SIZE) {
		} else {
			nRet = MCSDL_RET_WRONG_BINARY;
		}
	}
	/*
	 *---------------------------------
	 * set download enable mode
	 *---------------------------------
	 */
	if (MELFAS_CORE_FIRWMARE_UPDATE_ENABLE) {
		version_info =
		    mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_CORE);
		printk("<MELFAS> CORE_VERSION : 0x%2X\n", version_info);
/*		if (version_info < 0x01 || version_info == 0xFF)*/
		dl_enable_bit |= 0x01;
	}
	if (MELFAS_PRIVATE_CONFIGURATION_UPDATE_ENABLE) {
		version_info =
		    mms100_ISC_read_data
		    (MELFAS_FIRMWARE_VER_REG_PRIVATE_CUSTOM);
		printk("<MELFAS> PRIVATE_CUSTOM_VERSION : 0x%2X\n",
		       version_info);
		if (version_info < 0x01 || version_info == 0xFF)
			dl_enable_bit |= 0x02;
	}
	if (MELFAS_PUBLIC_CONFIGURATION_UPDATE_ENABLE) {
		version_info =
		    mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PUBLIC_CUSTOM);
		printk("<MELFAS> PUBLIC_CUSTOM_VERSION : 0x%2X\n",
		       version_info);
		if (version_info < 0x01 || version_info == 0xFF)
			dl_enable_bit |= 0x04;
	}
	/*
	 *------------------------
	 * Run Download
	 *------------------------
	 */

	for (i = 0; i < 3; i++) {
		if (dl_enable_bit & (1 << i)) {
			if (i < 2)	/* 0: core, 1: private custom */
				nRet = mms100_ISC_download((const UINT8 *)
							   fw_data1,
							   (const UINT16)
							   fw1_size,
							   (const INT8)i,
							   ts_data);
			/*              else    * 2: public custom
			   nRet = mms100_ISC_download((const UINT8 *)
			   pBinary[1],
			   (const UINT16)
			   nBinary_length[1],
			   (const INT8)i,
			   ts_data); */
			if (nRet)
				goto fw_error;
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
			nRet = mms100_ISC_Slave_download((const INT8)i);	/* Slave Binary data download */
			if (nRet)
				goto fw_error;
#endif
		}
	}
	MELFAS_ROLLBACK_BASEBAND_ISR();	/* Roll-back Baseband touch interrupt ISR. */
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();	/* Roll-back Baseband watchdog timer */
	return 0;
	kfree(fw_data1);

 fw_error:
/*  mcsdl_erase_flash(0);
	csdl_erase_flash(1);
	*/

	return nRet;

}

/*
*------------------------------------------------------------------
*
*      Download function
*
*------------------------------------------------------------------
*/
static int mms100_ISC_download(const UINT8 * pBianry, const UINT16 unLength,
			       const UINT8 nMode,
			       struct melfas_tsi_platform_data *ts_data)
{
	int nRet;
	/*int i = 0; */
	UINT8 fw_status = 0;

	UINT8 private_flash_start = ISC_PRIVATE_CONFIG_FLASH_START;
	UINT8 public_flash_start = ISC_PUBLIC_CONFIG_FLASH_START;
	/*UINT8 core_version; */
	UINT8 flash_start[3] = { 0, 0, 0 };
	UINT8 flash_end[3] = { 31, 31, 31 };

	/*
	 *---------------------------------
	 * Check Binary Size
	 *---------------------------------
	 */
	if (unLength > MELFAS_FIRMWARE_MAX_SIZE) {

		nRet = MCSDL_RET_PROGRAM_SIZE_IS_WRONG;
		goto MCSDL_DOWNLOAD_FINISH;
	}
	/*
	 *---------------------------------
	 * Make it ready
	 *---------------------------------
	 */
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	printk("<MELFAS> Ready\n");
#endif

	mms100_ISC_set_ready(ts_data);

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	if (nMode == 0)
		printk("<MELFAS> Core_firmware_download_via_ISC start!!!\n");
	else if (nMode == 1)
		printk
		    ("<MELFAS> Private_Custom_firmware_download_via_ISC start!!!\n");
	else
		printk
		    ("<MELFAS> Public_Custom_firmware_download_via_ISC start!!!\n");

#endif
	/*
	 *--------------------------------------------------------------
	 * INITIALIZE
	 *--------------------------------------------------------------
	 */
	printk("<MELFAS> ISC_DOWNLOAD_MODE_ENTER\n\n");
	mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);

#if ISC_READ_DOWNLOAD_POSITION
	printk("<MELFAS> Read download position.\n\n");
	if (1 << nMode != ISC_CORE_FIRMWARE_DL_MODE) {
		private_flash_start =
		    mms100_ISC_read_data(ISC_PRIVATE_CONFIGURATION_START_ADDR);
		public_flash_start =
		    mms100_ISC_read_data(ISC_PUBLIC_CONFIGURATION_START_ADDR);
	}
#endif

	flash_start[0] = 0;
	flash_end[0] = flash_end[2] = 31;
	flash_start[1] = private_flash_start;
	flash_start[2] = flash_end[1] = public_flash_start;
	printk
	    ("<MELFAS> Private Configration start at %2dKB, Public Configration start at %2dKB\n",
	     private_flash_start, public_flash_start);

	mcsdl_delay(MCSDL_DELAY_60MS);
	/*
	 *--------------------------------------------------------------
	 * FIRMWARE UPDATE MODE ENTER
	 *--------------------------------------------------------------
	 */
	printk("<MELFAS> FIRMWARE_UPDATE_MODE_ENTER\n\n");
	mms100_ISC_firmware_update_mode_enter();
	mcsdl_delay(MCSDL_DELAY_60MS);
#if 1
	fw_status = mms100_ISC_read_firmware_status();
	if (fw_status == 0x01)
		printk("<MELFAS> Firmware update mode enter success!!!\n");
	else {
		printk("<MELFAS> Error detected!! firmware status is 0x%02x.\n",
		       fw_status);
		nRet = MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED;
		goto MCSDL_DOWNLOAD_FINISH;
	}
	mcsdl_delay(MCSDL_DELAY_60MS);
#endif
	/*
	 *--------------------------------------------------------------
	 * FIRMWARE UPDATE
	 *--------------------------------------------------------------
	 */
	printk("<MELFAS> FIRMWARE UPDATE\n\n");
	nRet =
	    mms100_ISC_firmware_update((UINT8 *) pBianry, (UINT16) unLength,
				       flash_start[nMode], flash_end[nMode]);
	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;
	/*
	 *--------------------------------------------------------------
	 * LEAVE FIRMWARE UPDATE MODE
	 *--------------------------------------------------------------
	 */
	printk("<MELFAS> LEAVE FIRMWARE UPDATE MODE\n\n");

	mms100_ISC_leave_firmware_update_mode();
	mcsdl_delay(MCSDL_DELAY_60MS);
	/*
	   fw_status = mms100_ISC_read_firmware_status();
	   if(fw_status == 0xFF || fw_status == 0x00 )
	   printk("<MELFAS> Living firmware update mode success!!!\n");
	   else
	   {
	   printk("<MELFAS> Error detected!! firmware status is 0x%02x.\n", fw_status);
	   nRet = MCSDL_LEAVE_FIRMWARE_UPDATE_MODE_FAILED;
	   goto MCSDL_DOWNLOAD_FINISH;
	   }
	 */
	nRet = MCSDL_RET_SUCCESS;

 MCSDL_DOWNLOAD_FINISH:

#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result(nRet);	/*Show result */
#endif

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	printk("<MELMAS> Rebooting\n");
	printk("<MELMAS>  - Fin.\n\n");
#endif

	mms100_ISC_reboot_mcs(ts_data);

	return nRet;
}

static int mms100_ISC_Slave_download(UINT8 nMode)
{
	int nRet;
	/*int core_version = 0; */
	/*
	 *---------------------------------
	 * Make it ready
	 *---------------------------------
	 */
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	printk("<MELFAS> Ready\n");
#endif

/*     mms100_ISC_set_ready();*/

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	if (nMode == 0)
		printk
		    ("<MELFAS> Core_firmware_slave_download_via_ISC start!!!\n");
	else if (nMode == 1)
		printk
		    ("<MELFAS> Private_Custom_firmware_slave_download_via_ISC start!!!\n");
	else
		printk
		    ("<MELFAS> Public_Custom_firmware_slave_download_via_ISC start!!!\n");
#endif
	/*
	 *--------------------------------------------------------------
	 * INITIALIZE
	 *--------------------------------------------------------------
	 */
	printk("<MELFAS> ISC_DOWNLOAD_MODE_ENTER\n\n");
	mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);
	/*
	 *--------------------------------------------------------------
	 * Slave download start
	 *--------------------------------------------------------------
	 */
	mms100_ISC_slave_download_start(nMode + 1);
	nRet = mms100_ISC_slave_crc_ok();
	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;

	nRet = MCSDL_RET_SUCCESS;

 MCSDL_DOWNLOAD_FINISH:

#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result(nRet);	/*Show result */
#endif

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	printk("<MELMAS> Rebooting\n");
	printk("<MELMAS>  - Fin.\n\n");
#endif

/*     mms100_ISC_reboot_mcs();*/

	return nRet;
}

/*
*------------------------------------------------------------------
*
*      Sub functions
*
*------------------------------------------------------------------
*/
static UINT8 mms100_ISC_read_data(UINT8 addr)
{
	UINT32 wordData = 0x00000000;
	UINT8 write_buffer[4];
	UINT8 flash_start;

	mcsdl_i2c_start();
	write_buffer[0] = ucSlave_addr << 1;
	write_buffer[1] = addr;	/*command */
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16);

	mcsdl_ISC_write_bits(wordData, 16);
	mcsdl_delay(MCSDL_DELAY_60MS);

	mcsdl_i2c_start();
	/*1byte read */
	wordData = (ucSlave_addr << 1 | 0x01) << 24;
	mcsdl_ISC_write_bits(wordData, 8);
	flash_start = mcsdl_read_byte();
	wordData = (0x01) << 31;
	mcsdl_ISC_write_bits(wordData, 1);	/*nack */
	mcsdl_i2c_stop();
	return flash_start;

}

static void mms100_ISC_enter_download_mode(void)
{
	UINT32 wordData = 0x00000000;
	UINT8 write_buffer[4];

	mcsdl_i2c_start();
	write_buffer[0] = ucSlave_addr << 1;	/* slave addr */
	write_buffer[1] = ISC_DOWNLOAD_MODE_ENTER;	/* command */
	write_buffer[2] = 0x01;	/* sub_command */
	wordData =
	    (write_buffer[0] << 24) | (write_buffer[1] << 16) | (write_buffer[2]
								 << 8);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
}

static void mms100_ISC_firmware_update_mode_enter(void)
{
	UINT32 wordData = 0x00000000;
	mcsdl_i2c_start();
	wordData =
	    (ucSlave_addr << 1) << 24 | (0xAE << 16) | (0x55 << 8) | (0x00);
	mcsdl_ISC_write_bits(wordData, 32);
	wordData = 0x00000000;
	mcsdl_ISC_write_bits(wordData, 32);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
}

static UINT8 mms100_ISC_firmware_update(UINT8 * _pBinary_reordered,
					UINT16 _unDownload_size,
					UINT8 flash_start, UINT8 flash_end)
{

	int i = 0, j = 0, n, m;

	UINT8 fw_status;

	UINT32 wordData = 0x00000000;

	UINT16 nOffset = 0;
	UINT16 cLength = 8;
	UINT16 CRC_check_buf, CRC_send_buf, IN_data;
	UINT16 XOR_bit_1, XOR_bit_2, XOR_bit_3;

	UINT8 write_buffer[64];

	nOffset = 0;
	cLength = 8;		/*256 */

	printk("<MELFAS> flash start : %2d, flash end : %2d\n", flash_start,
	       flash_end);

	while (flash_start + nOffset < flash_end) {

		CRC_check_buf = 0xFFFF;
		mcsdl_i2c_start();
		write_buffer[0] = ucSlave_addr << 1;
		write_buffer[1] = 0XAE;	/* command */
		write_buffer[2] = 0XF1;	/* sub_command */
		write_buffer[3] = flash_start + nOffset;

		wordData =
		    (write_buffer[0] << 24) | (write_buffer[1] << 16) |
		    (write_buffer[2] << 8) | write_buffer[3];
		mcsdl_ISC_write_bits(wordData, 32);
		mcsdl_delay(MCSDL_DELAY_100MS);
		mcsdl_delay(MCSDL_DELAY_100MS);

		for (m = 7; m >= 0; m--) {
			IN_data = (write_buffer[3] >> m) & 0x01;
			XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
			XOR_bit_2 = XOR_bit_1 ^ (CRC_check_buf >> 11 & 0x01);
			XOR_bit_3 = XOR_bit_1 ^ (CRC_check_buf >> 4 & 0x01);
			CRC_send_buf =
			    (XOR_bit_1 << 4) | (CRC_check_buf >> 12 & 0x0F);
			CRC_send_buf =
			    (CRC_send_buf << 7) | (XOR_bit_2 << 6) |
			    (CRC_check_buf >> 5 & 0x3F);
			CRC_send_buf =
			    (CRC_send_buf << 4) | (XOR_bit_3 << 3) |
			    (CRC_check_buf >> 1 & 0x0007);
			CRC_check_buf = CRC_send_buf;
		}

		for (j = 0; j < 32; j++) {
			for (i = 0; i < cLength; i++) {
				write_buffer[i * 4 + 3] =
				    _pBinary_reordered[(flash_start +
							nOffset) * 1024 +
						       j * 32 + i * 4 + 0];
				write_buffer[i * 4 + 2] =
				    _pBinary_reordered[(flash_start +
							nOffset) * 1024 +
						       j * 32 + i * 4 + 1];
				write_buffer[i * 4 + 1] =
				    _pBinary_reordered[(flash_start +
							nOffset) * 1024 +
						       j * 32 + i * 4 + 2];
				write_buffer[i * 4 + 0] =
				    _pBinary_reordered[(flash_start +
							nOffset) * 1024 +
						       j * 32 + i * 4 + 3];
				for (n = 0; n < 4; n++) {
					for (m = 7; m >= 0; m--) {
						IN_data =
						    (write_buffer[i * 4 + n] >>
						     m) & 0x0001;
						XOR_bit_1 =
						    (CRC_check_buf & 0x0001) ^
						    IN_data;
						XOR_bit_2 =
						    XOR_bit_1 ^ (CRC_check_buf
								 >> 11 & 0x01);
						XOR_bit_3 =
						    XOR_bit_1 ^ (CRC_check_buf
								 >> 4 & 0x01);
						CRC_send_buf =
						    (XOR_bit_1 << 4) |
						    (CRC_check_buf >> 12 &
						     0x0F);
						CRC_send_buf =
						    (CRC_send_buf << 7) |
						    (XOR_bit_2 << 6) |
						    (CRC_check_buf >> 5 & 0x3F);
						CRC_send_buf =
						    (CRC_send_buf << 4) |
						    (XOR_bit_3 << 3) |
						    (CRC_check_buf >> 1 &
						     0x0007);
						CRC_check_buf = CRC_send_buf;
					}
				}
			}

			for (i = 0; i < cLength; i++) {
				wordData =
				    (write_buffer[i * 4 + 0] << 24) |
				    (write_buffer[i * 4 + 1] << 16) |
				    (write_buffer[i * 4 + 2] << 8) |
				    write_buffer[i * 4 + 3];
				mcsdl_ISC_write_bits(wordData, 32);
				mcsdl_delay(MCSDL_DELAY_150US);
			}
		}

		write_buffer[1] = CRC_check_buf & 0xFF;
		write_buffer[0] = CRC_check_buf >> 8 & 0xFF;

		wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16);
		mcsdl_ISC_write_bits(wordData, 16);
		mcsdl_delay(MCSDL_DELAY_100US);
		mcsdl_i2c_stop();

		fw_status = mms100_ISC_read_firmware_status();
		if (fw_status == 0x03) {
			printk("<MELFAS> Firmware update success!!!\n");
		} else {
			printk
			    ("<MELFAS> Error detected!! firmware status is 0x%02x.\n",
			     fw_status);
			return MCSDL_FIRMWARE_UPDATE_FAILED;
		}
		nOffset += 1;
		printk("<MELFAS> %d KB Downloaded...\n", nOffset);
	}

	return MCSDL_RET_SUCCESS;

}

static UINT8 mms100_ISC_read_firmware_status(void)
{
	UINT32 wordData = 0x00000000;
	UINT8 fw_status;
	mcsdl_i2c_start();
	/* WRITE 0xAF */
	wordData = (ucSlave_addr << 1) << 24 | (0xAF << 16);
	mcsdl_ISC_write_bits(wordData, 16);
	mcsdl_i2c_stop();
	mcsdl_delay(MCSDL_DELAY_25MS);

	mcsdl_i2c_start();
	/* 1byte read */
	wordData = (ucSlave_addr << 1 | 0x01) << 24;
	mcsdl_ISC_write_bits(wordData, 8);
	fw_status = mcsdl_read_byte();
	wordData = (0x01) << 31;
	mcsdl_ISC_write_bits(wordData, 1);	/*Nack */
	mcsdl_i2c_stop();
	return fw_status;
}

static void mms100_ISC_slave_download_start(UINT8 nMode)
{
	UINT32 wordData = 0x00000000;
	UINT8 write_buffer[4];
	mcsdl_i2c_start();
	/* WRITE 0xAF */
	write_buffer[0] = ucSlave_addr << 1;
	write_buffer[1] = ISC_DOWNLOAD_MODE;	/* command */
	write_buffer[2] = nMode;	/* 0x01: core, 0x02: private custom, 0x03: public custsom */
	wordData =
	    (write_buffer[0] << 24) | (write_buffer[1] << 16) | (write_buffer[2]
								 << 8);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
	mcsdl_delay(MCSDL_DELAY_100MS);
}

static UINT8 mms100_ISC_slave_crc_ok(void)
{
	UINT32 wordData = 0x00000000;
	UINT8 CRC_status = 0;
	UINT8 write_buffer[4];
	UINT8 check_count = 0;

	mcsdl_i2c_start();
	write_buffer[0] = ucSlave_addr << 1;
	write_buffer[1] = ISC_READ_SLAVE_CRC_OK;	/* command */
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16);

	mcsdl_ISC_write_bits(wordData, 16);

	while (CRC_status != 0x01 && check_count < 200) {	/*check_count 200 : 20sec */
		mcsdl_i2c_start();
		/* 1byte read */
		wordData = (ucSlave_addr << 1 | 0x01) << 24;
		mcsdl_ISC_write_bits(wordData, 8);
		CRC_status = mcsdl_read_byte();
		wordData = (0x01) << 31;
		mcsdl_ISC_write_bits(wordData, 1);	/*Nack */
		if (check_count % 10 == 0)
			printk("<MELFAS> %d sec...\n", check_count / 10);
		mcsdl_i2c_stop();

		if (CRC_status == 1)
			return MCSDL_RET_SUCCESS;
		else if (CRC_status == 2)
			return MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED;
		mcsdl_delay(MCSDL_DELAY_100MS);
		check_count++;
	}
	return MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER;
}

static void mms100_ISC_leave_firmware_update_mode(void)
{
	UINT32 wordData = 0x00000000;
	mcsdl_i2c_start();
	wordData =
	    (ucSlave_addr << 1) << 24 | (0xAE << 16) | (0x0F << 8) | (0xF0);
	mcsdl_ISC_write_bits(wordData, 32);
	mcsdl_i2c_stop();
}

static void mcsdl_i2c_start(void)
{
	MCSDL_GPIO_SDA_SET_HIGH();
	MCSDL_GPIO_SDA_SET_OUTPUT(0);
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SCL_SET_HIGH();
	MCSDL_GPIO_SCL_SET_OUTPUT(1);
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SDA_SET_LOW();
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SCL_SET_LOW();
}

static void mcsdl_i2c_stop(void)
{
	MCSDL_GPIO_SCL_SET_LOW();
	MCSDL_GPIO_SCL_SET_OUTPUT(0);
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_OUTPUT(0);
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SDA_SET_HIGH();

}

static void mms100_ISC_set_ready(struct melfas_tsi_platform_data *ts_data)
{
	/*
	 *--------------------------------------------
	 * Tkey module reset
	 *--------------------------------------------
	 */
	ts_data->power(0);	/*MCSDL_VDD_SET_LOW(); // power */
	/*MCSDL_CE_SET_LOW();
	   MCSDL_CE_SET_OUTPUT(); */

	MCSDL_SET_GPIO_I2C();

	MCSDL_GPIO_SDA_SET_HIGH();
	MCSDL_GPIO_SDA_SET_OUTPUT(1);

	MCSDL_GPIO_SCL_SET_HIGH();
	MCSDL_GPIO_SCL_SET_OUTPUT(1);

	MCSDL_RESETB_SET_INPUT();

	/*MCSDL_CE_SET_HIGH;
	   MCSDL_CE_SET_OUTPUT(); */

	mcsdl_delay(MCSDL_DELAY_60MS);	/* Delay for Stable VDD */

	ts_data->power(1);	/*MCSDL_VDD_SET_HIGH(); */

	mcsdl_delay(MCSDL_DELAY_500MS);	/* Delay for Stable VDD */
}

static void mms100_ISC_reboot_mcs(struct melfas_tsi_platform_data *ts_data)
{
	/*
	 *--------------------------------------------
	 * Tkey module reset
	 *--------------------------------------------
	 */
	mms100_ISC_set_ready(ts_data);
}

static UINT8 mcsdl_read_ack(void)
{
/*	int i;*/
	UINT8 pData = 0x00;
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_INPUT();

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_3US);
	if (MCSDL_GPIO_SDA_IS_HIGH())
		pData = 0x01;
	MCSDL_GPIO_SCL_SET_LOW();
	mcsdl_delay(MCSDL_DELAY_3US);
	return pData;
}

static void mcsdl_ISC_read_32bits(UINT8 * pData)
{
	int i, j;
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_INPUT();

	for (i = 3; i >= 0; i--) {

		pData[i] = 0;

		for (j = 0; j < 8; j++) {

			pData[i] <<= 1;

			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_3US);
			if (MCSDL_GPIO_SDA_IS_HIGH())
				pData[i] |= 0x01;
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_3US);
		}
	}

}

static UINT8 mcsdl_read_byte(void)
{
	int i;
	UINT8 pData = 0x00;
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_INPUT();

	MCSDL_GPIO_SCL_SET_INPUT();
	while (!MCSDL_GPIO_SCL_IS_HIGH()) ;
	MCSDL_GPIO_SCL_SET_HIGH();
	MCSDL_GPIO_SCL_SET_OUTPUT(1);

	for (i = 0; i < 8; i++) {
		pData <<= 1;
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_1US);
		if (MCSDL_GPIO_SDA_IS_HIGH())
			pData |= 0x01;
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_1US);
	}
	return pData;
}

static void mcsdl_ISC_write_bits(UINT32 wordData, int nBits)
{
	int i;

	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_OUTPUT(0);

	for (i = 0; i < nBits; i++) {

		if (wordData & 0x80000000) {
			MCSDL_GPIO_SDA_SET_HIGH();
		} else {
			MCSDL_GPIO_SDA_SET_LOW();
		}

		mcsdl_delay(MCSDL_DELAY_1US);

		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_1US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_1US);

		wordData <<= 1;
		if ((i % 8) == 7) {
			mcsdl_read_ack();	/*ead Ack */

			MCSDL_GPIO_SDA_SET_LOW();
			MCSDL_GPIO_SDA_SET_OUTPUT(0);
		}
	}
}

/*
*============================================================
*
*      Debugging print functions.
*
*============================================================
*/
#ifdef MELFAS_ENABLE_DBG_PRINT

static void mcsdl_ISC_print_result(int nRet)
{
	if (nRet == MCSDL_RET_SUCCESS) {

		printk("<MELFAS> Firmware downloading SUCCESS.\n");

	} else {

		printk("<MELFAS> Firmware downloading FAILED  :  ");

		switch (nRet) {

		case MCSDL_RET_SUCCESS:
			printk("<MELFAS> MCSDL_RET_SUCCESS\n");
			break;
		case MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED:
			printk
			    ("<MELFAS> MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED\n");
			break;
		case MCSDL_RET_PROGRAM_VERIFY_FAILED:
			printk("<MELFAS> MCSDL_RET_PROGRAM_VERIFY_FAILED\n");
			break;

		case MCSDL_RET_PROGRAM_SIZE_IS_WRONG:
			printk("<MELFAS> MCSDL_RET_PROGRAM_SIZE_IS_WRONG\n");
			break;
		case MCSDL_RET_VERIFY_SIZE_IS_WRONG:
			printk("<MELFAS> MCSDL_RET_VERIFY_SIZE_IS_WRONG\n");
			break;
		case MCSDL_RET_WRONG_BINARY:
			printk("<MELFAS> MCSDL_RET_WRONG_BINARY\n");
			break;

		case MCSDL_RET_READING_HEXFILE_FAILED:
			printk("<MELFAS> MCSDL_RET_READING_HEXFILE_FAILED\n");
			break;
		case MCSDL_RET_FILE_ACCESS_FAILED:
			printk("<MELFAS> MCSDL_RET_FILE_ACCESS_FAILED\n");
			break;
		case MCSDL_RET_MELLOC_FAILED:
			printk("<MELFAS> MCSDL_RET_MELLOC_FAILED\n");
			break;

		case MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED:
			printk
			    ("<MELFAS> MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED\n");
			break;
		case MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER:
			printk
			    ("<MELFAS> MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER\n");
			break;

		case MCSDL_RET_WRONG_MODULE_REVISION:
			printk("<MELFAS> MCSDL_RET_WRONG_MODULE_REVISION\n");
			break;

		default:
			printk("<MELFAS> UNKNOWN ERROR. [0x%02X].\n", nRet);
			break;
		}

		printk("\n");
	}

}

#endif
