/*--------------------------------------------------------
//
//
//	Melfas MCS8000 Series Download base v1.0 2010.04.05
//
//------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/preempt.h>
#include <linux/io.h>

#include "mms152_download.h"

#include <linux/fs.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#define MELFAS_FW1 "/sdcard/Master.bin"

#include <plat/regs-watchdog.h>
#include <mach/map.h>
#define TPS 3200
#define TSP_FW_WATCHDOG	0

/*============================================================
//
//	Include MELFAS Binary code File ( ex> MELFAS_FIRM_bin.c)
//
//	Warning!!!!
//		Please, don't add binary.c file into project
//		Just #include here !!
//
//==========================================================*/


#define ISP_FW_MAX_TRY	3

#include "GFS_03x14.c"
#include "G2M_12x09.c"
#include "GFD_26x07.c"


UINT8 ucVerifyBuffer[MELFAS_TRANSFER_LENGTH];

/*---------------------------------
//	Downloading functions
//--------------------------------*/
static int mcsdl_download(const UINT8 *pData,
					const UINT16 nLength, INT8 IdxNum);
static void mcsdl_set_ready(void);
static void mcsdl_reboot_mcs(void);
static int mcsdl_erase_flash(INT8 IdxNum);
static int mcsdl_program_flash(UINT8 *pDataOriginal, UINT16 unLength,
		INT8 IdxNum);
static void mcsdl_program_flash_part(UINT8 *pData);
static int mcsdl_verify_flash(UINT8 *pData, UINT16 nLength, INT8 IdxNum);
static void mcsdl_read_flash(UINT8 *pBuffer);
static int mcsdl_read_flash_from(UINT8 *pBuffer, UINT16 unStart_addr,
		UINT16 unLength, INT8 IdxNum);
static void mcsdl_select_isp_mode(UINT8 ucMode);
static void mcsdl_unselect_isp_mode(void);
static void mcsdl_read_32bits(UINT8 *pData);
static void mcsdl_write_bits(UINT32 wordData, int nBits);
static void mcsdl_scl_toggle_twice(void);

static int mcsdl_download_verify(const UINT8 *pData,
					const UINT16 nLength, INT8 IdxNum);
static int mcsdl_verify_flash_all(UINT8 *pData, UINT16 nLength, INT8 IdxNum);

/*---------------------------------
//	For debugging display
//-------------------------------*/
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_print_result(int nRet);
#endif

#if MCSDL_USE_VDD_CONTROL
void mcsdl_vdd_on(void)
{
#ifdef CONFIG_MACH_P2LTE_REV00
	struct regulator *regulator;
	regulator = regulator_get(NULL, "hdp_2.8v");

	regulator_enable(regulator);
	regulator_put(regulator);
#endif
}
void mcsdl_vdd_off(void)
{
#ifdef CONFIG_MACH_P2LTE_REV00
	struct regulator *regulator;
	regulator = regulator_get(NULL, "hdp_2.8v");

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);

	regulator_put(regulator);
#endif
}
#endif

/*----------------------------------
// Download enable command
//--------------------------------*/
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD

void melfas_send_download_enable_command(void)
{

}

#endif

/*============================================================
//
//	Main Download furnction
//
//   1. Run mcsdl_download( pBinary[IdxNum], nBinary_length[IdxNum], IdxNum);
//       IdxNum : 0 (Master Chip Download)
//       IdxNum : 1 (2Chip Download)
//
//
//==========================================================*/

int mcsdl_download_binary_data(int touch_id)
{
	int nRet = 0, i;
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
	melfas_send_download_enable_command();
	mcsdl_delay(MCSDL_DELAY_100US);
#endif

	/*------------------------
	// Run Download
	//----------------------*/

	pr_info("%s touch_id = [%d]", __func__, touch_id);

	for (i = 0 ; i < ISP_FW_MAX_TRY; i++) {
		if (touch_id == 0) {
			pr_info("[TSP] GFS F/W ISP update");
			nRet = mcsdl_download((const UINT8 *) MELFAS_binary_1,
				(const UINT16) MELFAS_binary_nLength_1, 0);
		} else if (touch_id == 1) {
			pr_info("[TSP] G2M F/W ISP update");
			nRet = mcsdl_download((const UINT8 *) MELFAS_binary_2,
				(const UINT16) MELFAS_binary_nLength_2, 0);
		} else if (touch_id == 2) {
			pr_info("[TSP] GFD F/W ISP update");
			nRet = mcsdl_download((const UINT8 *) MELFAS_binary_3,
				(const UINT16) MELFAS_binary_nLength_3, 0);
		} else if (touch_id == 3) {
			pr_info("[TSP] G2W F/W ISP update");
		}
		if (nRet == 0)
			break;
	}

	if (i != 0)
		pr_info("[TSP] ISP D/W try count : %d", i);

	return nRet;
}

int mcsdl_download_binary_data_verify(int touch_id)
{
	int nRet = 0;

	if (touch_id == 0)
		nRet = mcsdl_download_verify((const UINT8 *) MELFAS_binary_1,
				(const UINT16) MELFAS_binary_nLength_1, 0);
	else if (touch_id == 1)
		nRet = mcsdl_download_verify((const UINT8 *) MELFAS_binary_2,
				(const UINT16) MELFAS_binary_nLength_2, 0);
	else if (touch_id == 2)
		nRet = mcsdl_download_verify((const UINT8 *) MELFAS_binary_3,
				(const UINT16) MELFAS_binary_nLength_3, 0);
	else if (touch_id == 3)
		pr_info("[TSP] mcsdl_download_binary_data_verify G2W");

	return nRet;
}

int mcsdl_download_binary_file(void)
{
	int nRet = 0;
	int retry_cnt = 0;
	long fw1_size = 0;
	unsigned char *fw_data1;
	struct file *filp;
	loff_t	pos;
	int	ret = 0;
	mm_segment_t oldfs;
	spinlock_t	lock;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MELFAS_FW1, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("file open error:%d", (s32)filp);
		return -1;
	}

	fw1_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("Size of the file : %ld(bytes)", fw1_size);

	fw_data1 = kmalloc(fw1_size, GFP_KERNEL);
	memset(fw_data1, 0, fw1_size);

	pos = 0;
	memset(fw_data1, 0, fw1_size);
	ret = vfs_read(filp, (char __user *)fw_data1, fw1_size, &pos);

	if (ret != fw1_size) {
		pr_err("Failed to read file %s (ret = %d)", MELFAS_FW1, ret);
		kfree(fw_data1);
		filp_close(filp, NULL);
		return -1;
	}

	filp_close(filp, NULL);

	set_fs(oldfs);
	spin_lock_init(&lock);
	spin_lock(&lock);

	for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
		pr_info("[TSP] ADB - Firmware update! try : %d", retry_cnt+1);
		nRet = mcsdl_download((const UINT8 *) fw_data1,
						(const UINT16)fw1_size, 0);
		if (nRet)
			continue;
		break;
	}

	kfree(fw_data1);
	spin_unlock(&lock);
	return nRet;
}

static int mcsdl_download_verify(const UINT8 *pBianry, const UINT16 unLength,
						INT8 IdxNum)
{
	int nRet;

	mcsdl_set_ready();
	msleep(200);

	pr_info("[TSP] mcsdl_download_verify ");
	preempt_disable();
	nRet = mcsdl_verify_flash_all((UINT8 *) pBianry,
				(UINT16) unLength, IdxNum);
	preempt_enable();

	mcsdl_reboot_mcs();

	return nRet;
}

/*------------------------------------------------------------------
//
//	Download function
//
//----------------------------------------------------------------*/
static int mcsdl_download(const UINT8 *pBianry, const UINT16 unLength,
		INT8 IdxNum)
{
	int nRet;

	/*---------------------------------
	// Check Binary Size
	//-------------------------------*/
	if (unLength >= MELFAS_FIRMWARE_MAX_SIZE) {
		nRet = MCSDL_RET_PROGRAM_SIZE_IS_WRONG;
		goto MCSDL_DOWNLOAD_FINISH;
	}

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info(" - Starting download...");
#endif

	/*---------------------------------
	// Make it ready
	//-------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP]  Ready");
#endif

	mcsdl_set_ready();

	/*---------------------------------
	// Erase Flash
	//-------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info(" > Erase");
#endif

	preempt_disable();
	nRet = mcsdl_erase_flash(IdxNum);
	preempt_enable();

	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;

	/*---------------------------------
	// Program Flash
	//-------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info(" > Program   ");
#endif

#if TSP_FW_WATCHDOG
	writel(20 * TPS, S3C2410_WTCNT);
	pr_info("[TSP] Watchdog kicking Before Flash");
#endif

	preempt_disable();
	nRet = mcsdl_program_flash((UINT8 *) pBianry,
				(UINT16) unLength, IdxNum);
	preempt_enable();

	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;
	/*---------------------------------
	// Verify flash
	//-------------------------------*/

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info(" > Verify    ");
#endif

#if TSP_FW_WATCHDOG
	writel(20 * TPS, S3C2410_WTCNT);
	pr_info("[TSP] Watchdog kicking Before Verify");
#endif

	preempt_disable();
	nRet = mcsdl_verify_flash((UINT8 *) pBianry,
				(UINT16) unLength, IdxNum);
	preempt_enable();

	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;

	nRet = MCSDL_RET_SUCCESS;

MCSDL_DOWNLOAD_FINISH:

#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_print_result(nRet);
#endif

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info(" > Rebooting");
	pr_info(" - Fin.\n");
#endif

#if TSP_FW_WATCHDOG
	writel(20 * TPS, S3C2410_WTCNT);
	pr_info("[TSP] Watchdog kicking After F/W update");
#endif

	mcsdl_reboot_mcs();

	return nRet;
}

/*------------------------------------------------------------------
//
//	Sub functions
//
//----------------------------------------------------------------*/

static int mcsdl_erase_flash(INT8 IdxNum)
{
	int i;
	UINT8 readBuffer[32];

	/*----------------------------------------
	//	Do erase
	//--------------------------------------*/
	if (IdxNum > 0)
		mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);

	mcsdl_select_isp_mode(ISP_MODE_ERASE_FLASH);
	mcsdl_unselect_isp_mode();

	/*----------------------------------------
	//	Check 'erased well'
	//--------------------------------------*/
	/* start ADD DELAY */
	mcsdl_read_flash_from(readBuffer, 0x0000, 16, IdxNum);
	mcsdl_read_flash_from(&readBuffer[16], 0x7FF0, 16, IdxNum);
	/* end ADD DELAY */

	/* Compare with '0xFF'  */
	for (i = 0; i < 32; i++) {
		if (readBuffer[i] != 0xFF)
			return MCSDL_RET_ERASE_FLASH_VERIFY_FAILED;
	}

	return MCSDL_RET_SUCCESS;
}

static int mcsdl_program_flash(UINT8 *pDataOriginal, UINT16 unLength,
					INT8 IdxNum)
{
	int i;

	UINT8 *pData;
	UINT8 ucLength;

	UINT16 addr;
	UINT32 header;

	addr = 0;
	pData = pDataOriginal;

	ucLength = MELFAS_TRANSFER_LENGTH;

	while ((addr * 4) < (int) unLength) {

		if ((unLength - (addr * 4)) < MELFAS_TRANSFER_LENGTH)
			ucLength = (UINT8)(unLength - (addr * 4));

		/*--------------------------------------
		//	Select ISP Mode
		//------------------------------------*/

		/* start ADD DELAY */
		mcsdl_delay(MCSDL_DELAY_40US);
		/* end ADD DELAY */
		if (IdxNum > 0)
			mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
		mcsdl_select_isp_mode(ISP_MODE_SERIAL_WRITE);

		/*---------------------------------------------
		//	Header
		//	Address[13ibts] <<1
		//-------------------------------------------*/
		header = ((addr & 0x1FFF) << 1) | 0x0;
		header = header << 14;

		/* Write 18bits */
		mcsdl_write_bits(header, 18);
		/* start ADD DELAY */
		/* mcsdl_delay(MCSDL_DELAY_5MS); */
		/* end ADD DELAY */

		/*---------------------------------
		//	Writing
		//------------------------------- */
		/* addr += (UINT16)ucLength; */
		addr += 1;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		pr_info(KENR_INFO "#");
#endif

		mcsdl_program_flash_part(pData);

		pData += ucLength;

		/*---------------------------------------------
		//	Tail
		//------------------------------------------- */
		MCSDL_GPIO_SDA_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_40US);

		for (i = 0; i < 6; i++) {

			if (i == 2)
				mcsdl_delay(MCSDL_DELAY_20US);
			else if (i == 3)
				mcsdl_delay(MCSDL_DELAY_40US);

			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_10US);
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_10US);
		}

		mcsdl_unselect_isp_mode();
		/* start ADD DELAY */
		mcsdl_delay(MCSDL_DELAY_300US);
		/* end ADD DELAY */
	}
	return MCSDL_RET_SUCCESS;
}

static void mcsdl_program_flash_part(UINT8 *pData)
{
	int i;
	UINT32 data;

	/*---------------------------------
	//	Body
	//-------------------------------*/

	data = (UINT32) pData[0] << 0;
	data |= (UINT32) pData[1] << 8;
	data |= (UINT32) pData[2] << 16;
	data |= (UINT32) pData[3] << 24;
	mcsdl_write_bits(data, 32);

}

static int mcsdl_verify_flash_all(UINT8 *pDataOriginal, UINT16 unLength,
					INT8 IdxNum)
{
	int i, j;
	int nRet = MCSDL_RET_SUCCESS;

	UINT8 *pData;
	UINT8 ucLength;

	UINT16 addr;
	UINT32 wordData;

	addr = 0;
	pData = (UINT8 *) pDataOriginal;

	ucLength = MELFAS_TRANSFER_LENGTH;

	while ((addr * 4) < (int) unLength) {
		if ((unLength - (addr * 4)) < MELFAS_TRANSFER_LENGTH)
			ucLength = (UINT8)(unLength - (addr * 4));

		mcsdl_delay(MCSDL_DELAY_40US);

		if (IdxNum > 0)
			mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
		mcsdl_select_isp_mode(ISP_MODE_SERIAL_READ);

		wordData = ((addr & 0x1FFF) << 1) | 0x0;
		wordData <<= 14;

		mcsdl_write_bits(wordData, 18);

		addr += 1;

		mcsdl_read_flash(ucVerifyBuffer);

		MCSDL_GPIO_SDA_SET_HIGH();

		for (i = 0; i < 6; i++) {
			if (i == 2)
				mcsdl_delay(MCSDL_DELAY_3US);
			else if (i == 3)
				mcsdl_delay(MCSDL_DELAY_40US);

			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_10US);
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_10US);
		}

		for (j = 0; j < (int) ucLength; j++) {
			if (ucVerifyBuffer[j] != pData[j]) {
				pr_info("[TSP][Err] Addr [0x%04X] Ori[0x%02X]-Read[0x%02X]",
					addr, pData[j], ucVerifyBuffer[j]);
				nRet |= MCSDL_RET_PROGRAM_VERIFY_FAILED;
			}
		}

		pData += ucLength;
		mcsdl_unselect_isp_mode();
	}

	nRet |= MCSDL_RET_SUCCESS;

MCSDL_VERIFY_FLASH_FINISH:

	mcsdl_unselect_isp_mode();

	return nRet;
}

static int mcsdl_verify_flash(UINT8 *pDataOriginal, UINT16 unLength,
					INT8 IdxNum)
{
	int i, j;
	int nRet;

	UINT8 *pData;
	UINT8 ucLength;

	UINT16 addr;
	UINT32 wordData;

	addr = 0;
	pData = (UINT8 *) pDataOriginal;

	ucLength = MELFAS_TRANSFER_LENGTH;

	while ((addr * 4) < (int) unLength) {
		if ((unLength - (addr * 4)) < MELFAS_TRANSFER_LENGTH)
			ucLength = (UINT8)(unLength - (addr * 4));

		mcsdl_delay(MCSDL_DELAY_40US);

		/*--------------------------------------
		//	Select ISP Mode
		//------------------------------------*/
		if (IdxNum > 0)
			mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
		mcsdl_select_isp_mode(ISP_MODE_SERIAL_READ);

		/*---------------------------------------------
		//	Header
		//	Address[13ibts] <<1
		//-------------------------------------------*/

		wordData = ((addr & 0x1FFF) << 1) | 0x0;
		wordData <<= 14;

		mcsdl_write_bits(wordData, 18);

		addr += 1;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		printk(KERN_INFO "#");
#endif

		/*--------------------
		// Read flash
		//------------------*/
		mcsdl_read_flash(ucVerifyBuffer);

		MCSDL_GPIO_SDA_SET_HIGH();

		for (i = 0; i < 6; i++) {
			if (i == 2)
				mcsdl_delay(MCSDL_DELAY_3US);
			else if (i == 3)
				mcsdl_delay(MCSDL_DELAY_40US);

			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_10US);
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_10US);
		}
		/*--------------------
		// Comparing
		//------------------*/

		for (j = 0; j < (int) ucLength; j++) {

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
			pr_info("[TSP] %02X", ucVerifyBuffer[j]);
#endif

			if (ucVerifyBuffer[j] != pData[j]) {

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
				pr_info("[TSP][Err] Addr [0x%04X] Ori[0x%02X]-Read[0x%02X]",
					addr, pData[j], ucVerifyBuffer[j]);
#endif
				nRet = MCSDL_RET_PROGRAM_VERIFY_FAILED;
				goto MCSDL_VERIFY_FLASH_FINISH;
			}
		}

		pData += ucLength;

		mcsdl_unselect_isp_mode();
	}

	nRet = MCSDL_RET_SUCCESS;

MCSDL_VERIFY_FLASH_FINISH:

	mcsdl_unselect_isp_mode();

	return nRet;
}

static void mcsdl_read_flash(UINT8 *pBuffer)
{
	int i;

	MCSDL_GPIO_SDA_SET_LOW();

	mcsdl_delay(MCSDL_DELAY_40US);

	for (i = 0; i < 5; i++) {
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_10US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_10US);
	}

	mcsdl_read_32bits(pBuffer);
}

static int mcsdl_read_flash_from(UINT8 *pBuffer, UINT16 unStart_addr,
		UINT16 unLength, INT8 IdxNum)
{
	int i;
	int j;

	UINT8 ucLength;

	UINT16 addr;
	UINT32 wordData;

	if (unLength >= MELFAS_FIRMWARE_MAX_SIZE)
		return MCSDL_RET_PROGRAM_SIZE_IS_WRONG;

	addr = 0;
	ucLength = MELFAS_TRANSFER_LENGTH;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] %04X : ", unStart_addr);
#endif

	for (i = 0; i < (int) unLength; i += (int) ucLength) {

		addr = (UINT16) i;
		if (IdxNum > 0)
			mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);

		mcsdl_select_isp_mode(ISP_MODE_SERIAL_READ);
		wordData = (((unStart_addr + addr) & 0x1FFF) << 1) | 0x0;
		wordData <<= 14;

		mcsdl_write_bits(wordData, 18);

		if ((unLength - addr) < MELFAS_TRANSFER_LENGTH)
			ucLength = (UINT8)(unLength - addr);

		/*--------------------
		// Read flash
		//------------------*/
		mcsdl_read_flash(&pBuffer[addr]);

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		for (j = 0; j < (int) ucLength; j++)
			pr_info("[TSP] %02X ", pBuffer[j]);
#endif
		mcsdl_unselect_isp_mode();
	}

	return MCSDL_RET_SUCCESS;

}

static void mcsdl_set_ready(void)
{
	/*--------------------------------------------
	// Tkey module reset
	//------------------------------------------*/

	MCSDL_VDD_SET_LOW();
/*
	//MCSDL_CE_SET_LOW();
	//MCSDL_CE_SET_OUTPUT();

	//MCSDL_SET_GPIO_I2C();
*/
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_OUTPUT();

	MCSDL_GPIO_SCL_SET_LOW();
	MCSDL_GPIO_SCL_SET_OUTPUT();

	MCSDL_RESETB_SET_LOW();
	MCSDL_RESETB_SET_OUTPUT();

	mcsdl_delay(MCSDL_DELAY_25MS);

	MCSDL_VDD_SET_HIGH();
/*	//MCSDL_CE_SET_HIGH();*/

	MCSDL_GPIO_SDA_SET_HIGH();

	mcsdl_delay(MCSDL_DELAY_25MS);

}

static void mcsdl_reboot_mcs(void)
{
	/*--------------------------------------------
	// Tkey module reset
	//------------------------------------------*/

	MCSDL_VDD_SET_LOW();
/*
	//MCSDL_CE_SET_LOW();
	//MCSDL_CE_SET_OUTPUT();
*/
	MCSDL_GPIO_SDA_SET_HIGH();
	MCSDL_GPIO_SDA_SET_OUTPUT();

	MCSDL_GPIO_SCL_SET_HIGH();
	MCSDL_GPIO_SCL_SET_OUTPUT();
/*
	//MCSDL_SET_HW_I2C();
*/
	MCSDL_RESETB_SET_LOW();
	MCSDL_RESETB_SET_OUTPUT();

	mcsdl_delay(MCSDL_DELAY_25MS);

	MCSDL_RESETB_SET_HIGH();
	MCSDL_RESETB_SET_INPUT();
	MCSDL_VDD_SET_HIGH();
/*
	//MCSDL_CE_SET_HIGH();
*/
	mcsdl_delay(MCSDL_DELAY_30MS);

}

/*--------------------------------------------
//
//   Write ISP Mode entering signal
//
//------------------------------------------*/

static void mcsdl_select_isp_mode(UINT8 ucMode)
{
	int i;

	UINT8 enteringCodeMassErase[16] = {
		0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1 };
	UINT8 enteringCodeSerialWrite[16] = {
		0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
	UINT8 enteringCodeSerialRead[16] = {
		0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 };
	UINT8 enteringCodeNextChipBypass[16] = {
		1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1 };

	/*------------------------------------
	// Entering ISP mode : Part 1
	//----------------------------------*/

	for (i = 0; i < 16; i++) {
		if (ucMode == ISP_MODE_ERASE_FLASH) {
			if (enteringCodeMassErase[i] == 1)
				MCSDL_RESETB_SET_HIGH();
			else
				MCSDL_RESETB_SET_LOW();
		} else if (ucMode == ISP_MODE_SERIAL_WRITE) {
			if (enteringCodeSerialWrite[i] == 1)
				MCSDL_RESETB_SET_HIGH();
			else
				MCSDL_RESETB_SET_LOW();
		} else if (ucMode == ISP_MODE_SERIAL_READ) {
			if (enteringCodeSerialRead[i] == 1)
				MCSDL_RESETB_SET_HIGH();
			else
				MCSDL_RESETB_SET_LOW();
		} else if (ucMode == ISP_MODE_NEXT_CHIP_BYPASS) {
			if (enteringCodeNextChipBypass[i] == 1)
				MCSDL_RESETB_SET_HIGH();
			else
				MCSDL_RESETB_SET_LOW();
		}

		/*start add delay for INT*/
		mcsdl_delay(MCSDL_DELAY_7US);
		/*end delay for INT*/

		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_3US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_3US);
	}

	MCSDL_RESETB_SET_HIGH();

	/*---------------------------------------------------
	// Entering ISP mode : Part 2	- Only Mass Erase
	//-------------------------------------------------*/

	if (ucMode == ISP_MODE_ERASE_FLASH) {
		mcsdl_delay(MCSDL_DELAY_7US);
		for (i = 0; i < 4; i++) {

			if (i == 2)
				mcsdl_delay(MCSDL_DELAY_25MS);
			else if (i == 3)
				mcsdl_delay(MCSDL_DELAY_150US);

			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_3US);
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_7US);
		}
	}
}

static void mcsdl_unselect_isp_mode(void)
{
	int i;
/*
	// MCSDL_GPIO_SDA_SET_HIGH();
	// MCSDL_GPIO_SDA_SET_OUTPUT();
*/
	MCSDL_RESETB_SET_LOW();

	mcsdl_delay(MCSDL_DELAY_3US);

	for (i = 0; i < 10; i++) {
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_3US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_3US);
	}
}

static void mcsdl_read_32bits(UINT8 *pData)
{
	int i, j;

	MCSDL_GPIO_SDA_SET_INPUT();

	for (i = 3; i >= 0; i--) {
		pData[i] = 0;
		for (j = 0; j < 8; j++) {
			pData[i] <<= 1;
			MCSDL_GPIO_SCL_SET_HIGH();
			mcsdl_delay(MCSDL_DELAY_3US);
			MCSDL_GPIO_SCL_SET_LOW();
			mcsdl_delay(MCSDL_DELAY_3US);

			if (MCSDL_GPIO_SDA_IS_HIGH())
				pData[i] |= 0x01;
		}
	}
}

static void mcsdl_write_bits(UINT32 wordData, int nBits)
{
	int i;

	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_OUTPUT();

	for (i = 0; i < nBits; i++) {

		if (wordData & 0x80000000)
			MCSDL_GPIO_SDA_SET_HIGH();
		else
			MCSDL_GPIO_SDA_SET_LOW();

		mcsdl_delay(MCSDL_DELAY_7US);

		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_3US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_3US);

		wordData <<= 1;
	}
}

static void mcsdl_scl_toggle_twice(void)
{

	MCSDL_GPIO_SDA_SET_HIGH();
	MCSDL_GPIO_SDA_SET_OUTPUT();

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_20US);
	MCSDL_GPIO_SCL_SET_LOW();
	mcsdl_delay(MCSDL_DELAY_20US);

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_20US);
	MCSDL_GPIO_SCL_SET_LOW();
	mcsdl_delay(MCSDL_DELAY_20US);
}

/*============================================================
//
//	Delay Function
//
//==========================================================*/
void mcsdl_delay(UINT32 nCount)
{
	switch (nCount) {

	case MCSDL_DELAY_1US:
		udelay(1);
		break;
	case MCSDL_DELAY_2US:
		udelay(2);
		break;
	case MCSDL_DELAY_3US:
		udelay(3);
		break;
	case MCSDL_DELAY_5US:
		udelay(5);
		break;
	case MCSDL_DELAY_7US:
		udelay(7);
		break;
	case MCSDL_DELAY_10US:
		udelay(10);
		break;
	case MCSDL_DELAY_15US:
		udelay(15);
		break;
	case MCSDL_DELAY_20US:
		udelay(20);
		break;
	case MCSDL_DELAY_100US:
		udelay(100);
		break;
	case MCSDL_DELAY_150US:
		udelay(150);
		break;
	case MCSDL_DELAY_500US:
		udelay(500);
		break;
	case MCSDL_DELAY_800US:
		udelay(800);
		break;
	case MCSDL_DELAY_1MS:
		mdelay(1);
		break;
	case MCSDL_DELAY_5MS:
		mdelay(5);
		break;
	case MCSDL_DELAY_10MS:
		mdelay(10);
		break;
	case MCSDL_DELAY_25MS:
		mdelay(25);
		break;
	case MCSDL_DELAY_30MS:
		mdelay(30);
		break;
	case MCSDL_DELAY_40MS:
		mdelay(40);
		break;
	case MCSDL_DELAY_45MS:
		mdelay(45);
		break;
	case MCSDL_DELAY_100MS:
		mdelay(100);
		break;
	case MCSDL_DELAY_300US:
		udelay(300);
		break;
	case MCSDL_DELAY_60MS:
		mdelay(60);
		break;
	case MCSDL_DELAY_40US:
		udelay(40);
		break;
	case MCSDL_DELAY_50MS:
		mdelay(50);
		break;
	case MCSDL_DELAY_70US:
		udelay(70);
		break;
	case MCSDL_DELAY_500MS:
		mdelay(500);
		break;

	default:
		break;
	}
}

/*============================================================
//
//	Debugging print functions.
//
//==========================================================*/

#ifdef MELFAS_ENABLE_DBG_PRINT
static void mcsdl_print_result(int nRet)
{
	if (nRet == MCSDL_RET_SUCCESS)
		pr_info(" > MELFAS Firmware downloading SUCCESS.");
	else {

		pr_info(" > MELFAS Firmware downloading FAILED  :  ");

		switch (nRet) {

		case MCSDL_RET_SUCCESS:
			pr_info("MCSDL_RET_SUCCESS");
			break;
		case MCSDL_RET_ERASE_FLASH_VERIFY_FAILED:
			pr_info("MCSDL_RET_ERASE_FLASH_VERIFY_FAILED");
			break;
		case MCSDL_RET_PROGRAM_VERIFY_FAILED:
			pr_info("MCSDL_RET_PROGRAM_VERIFY_FAILED");
			break;

		case MCSDL_RET_PROGRAM_SIZE_IS_WRONG:
			pr_info("MCSDL_RET_PROGRAM_SIZE_IS_WRONG");
			break;
		case MCSDL_RET_VERIFY_SIZE_IS_WRONG:
			pr_info("MCSDL_RET_VERIFY_SIZE_IS_WRONG");
			break;
		case MCSDL_RET_WRONG_BINARY:
			pr_info("MCSDL_RET_WRONG_BINARY");
			break;

		case MCSDL_RET_READING_HEXFILE_FAILED:
			pr_info("MCSDL_RET_READING_HEXFILE_FAILED");
			break;
		case MCSDL_RET_FILE_ACCESS_FAILED:
			pr_info("MCSDL_RET_FILE_ACCESS_FAILED");
			break;
		case MCSDL_RET_MELLOC_FAILED:
			pr_info("MCSDL_RET_MELLOC_FAILED");
			break;

		case MCSDL_RET_WRONG_MODULE_REVISION:
			pr_info("MCSDL_RET_WRONG_MODULE_REVISION");
			break;

		default:
			pr_info("UNKNOWN ERROR. [0x%02X].", nRet);
			break;
		}
	}

}

#endif

#if MELFAS_ENABLE_DELAY_TEST

/*============================================================
//
//	For initial testing of delay and gpio control
//
//	You can confirm GPIO control and delay time by calling this function.
//
//==========================================================*/

void mcsdl_delay_test(INT32 nCount)
{
	INT16 i;

	MELFAS_DISABLE_BASEBAND_ISR();
	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();

	/*--------------------------------
	//	Repeating 'nCount' times
	//------------------------------*/

	MCSDL_SET_GPIO_I2C();
	MCSDL_GPIO_SCL_SET_OUTPUT();
	MCSDL_GPIO_SDA_SET_OUTPUT();
	MCSDL_RESETB_SET_OUTPUT();

	MCSDL_GPIO_SCL_SET_HIGH();

	for (i = 0; i < nCount; i++) {

#if 1

		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_20US);
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_100US);
#elif 0

		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_500US);
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_1MS);
#else

		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_25MS);
		TKEY_INTR_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_45MS);
		TKEY_INTR_SET_HIGH();
#endif
	}

	MCSDL_GPIO_SCL_SET_HIGH();

	MELFAS_ROLLBACK_BASEBAND_ISR();
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();
}

#endif
