/*-------------------------------------------------------
//
//	Melfas MMS100 Series Download base v1.0 2010.04.05
//
//------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include "mms152_download.h"
#include "mms152_download_porting.h"

/*============================================================
//
//	Include MELFAS Binary code File ( ex> MELFAS_FIRM_bin.c)
//
//	Warning!!!!
//		Please, don't add binary.c file into project
//		Just #include here !!
//
//==========================================================*/

/*---------------------------------
//	Downloading functions
//-------------------------------*/

static int mms100_ISC_download(const UINT8 *pBianry,
		const UINT16 unLength, const UINT8 nMode);
static void mms100_ISC_set_ready(void);
static void mms100_ISC_reboot_mcs(void);
static UINT8 mcsdl_read_ack(void);
static void mcsdl_ISC_read_32bits(UINT8 *pData);
static void mcsdl_ISC_write_bits(UINT32 wordData, int nBits);
static UINT8 mms100_ISC_read_data(UINT8 addr);
static void mms100_ISC_enter_download_mode(void);
static void mms100_ISC_firmware_update_mode_enter(void);
static UINT8 mms100_ISC_firmware_update(UINT8 *_pBinary_reordered,
		UINT16 _unDownload_size, UINT8 flash_start, UINT8 flash_end);
static UINT8 mms100_ISC_read_firmware_status(void);
static int mms100_ISC_Slave_download(UINT8 nMode);
static void mms100_ISC_slave_download_start(UINT8 nMode);
static UINT8 mms100_ISC_slave_crc_ok(void);
static void mms100_ISC_leave_firmware_update_mode(void);
static void mcsdl_i2c_start(void);
static void mcsdl_i2c_stop(void);
static UINT8 mcsdl_read_byte(void);

/*---------------------------------
//	For debugging display
//-------------------------------*/
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_ISC_print_result(int nRet);
#endif

/*----------------------------------
// Download enable command
//--------------------------------*/
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
void melfas_send_download_enable_command(void)
{
	/* TO DO : Fill this up */
}
#endif

/*============================================================
//
//	Main Download furnction
//
//	1. Run mcsdl_download( pBinary[IdxNum], nBinary_length[IdxNum], IdxNum);
//	IdxNum : 0 (Master Chip Download)
//	IdxNum : 1 (2Chip Download)
//
//
//==========================================================*/

int mms100_ISC_download_binary_data(int touch_id, INT8 dl_enable_bit)
{

	int	nRet;
	int	i;

#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
	melfas_send_download_enable_command();
	mcsdl_delay(MCSDL_DELAY_100US);
#endif

/*------------------------
// Run Download
//----------------------*/
	for (i = 0; i < 3; i++) {
		/* 0: core, 1: private custom,  2: public custom */
		if (dl_enable_bit & (1<<i)) {
			if (touch_id == 0) {
				pr_info("[TSP] GFS F/W ISC update : ");
				nRet = mms100_ISC_download(
					(const UINT8 *) MELFAS_binary_1,
					(const UINT16)MELFAS_binary_nLength_1,
					(const INT8)i);
			} else if (touch_id == 1) {
				pr_info("[TSP] G2M F/W ISC update : ");
				nRet = mms100_ISC_download(
					(const UINT8 *) MELFAS_binary_2,
					(const UINT16)MELFAS_binary_nLength_2,
					(const INT8)i);
			} else if (touch_id == 2) {
				pr_info("[TSP] GFD F/W ISC update : ");
				nRet = mms100_ISC_download(
					(const UINT8 *) MELFAS_binary_3,
					(const UINT16)MELFAS_binary_nLength_3,
					(const INT8)i);
			} else if (touch_id == 3)
				pr_info("[TSP] G2W F/W ISC update skip");
			else {
				pr_info("[TSP] Invalid touch_id error!");
				nRet = -1;
			}
			if (nRet)
				goto fw_error;
		}
	}
	return 0;

fw_error:
	return nRet;

}

/*------------------------------------------------------------------
//
//	Download function
//
//----------------------------------------------------------------*/

static int mms100_ISC_download(const UINT8 *pBianry,
					const UINT16 unLength,
					const UINT8 nMode)
{
	int nRet;
	int i = 0;
	UINT8 fw_status = 0;

	UINT8 private_flash_start = ISC_PRIVATE_CONFIG_FLASH_START;
	UINT8 public_flash_start = ISC_PUBLIC_CONFIG_FLASH_START;
	UINT8 core_version;
	UINT8 flash_start[3] = {0, 0, 0};
	UINT8 flash_end[3] =  {31, 31, 31};

	/*---------------------------------
	// Check Binary Size
	//-------------------------------*/
	if (unLength >= MELFAS_FIRMWARE_MAX_SIZE) {
		nRet = MCSDL_RET_PROGRAM_SIZE_IS_WRONG;
		goto MCSDL_DOWNLOAD_FINISH;
	}

	/*---------------------------------
	// Make it ready
	//-------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Ready");
#endif

	mms100_ISC_set_ready();

	if (nMode == 0)
		pr_info("[TSP] Core F/W D/L ISC start!!!");
	else if (nMode == 1)
		pr_info("[TSP] Private F/W D/L ISC start!!!");
	else
		pr_info("[TSP] Public F/W D/L ISC start!!!");

/*--------------------------------------------------------------
// INITIALIZE
//------------------------------------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] ISC_DOWNLOAD_MODE_ENTER");
#endif
	mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);

#if ISC_READ_DOWNLOAD_POSITION
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Read download position.");
#endif
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
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Private Conf start : %2dKB, Public Conf start : %2dKB",
			private_flash_start , public_flash_start);
#endif

	mcsdl_delay(MCSDL_DELAY_60MS);

/*--------------------------------------------------------------
// FIRMWARE UPDATE MODE ENTER
//------------------------------------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] FIRMWARE_UPDATE_MODE_ENTER\n");
#endif
	mms100_ISC_firmware_update_mode_enter();
	mcsdl_delay(MCSDL_DELAY_60MS);
#if 1
	 fw_status = mms100_ISC_read_firmware_status();
	if (fw_status == 0x01) {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		pr_info("[TSP] Firmware update mode enter success!!!");
#endif
	} else {
		pr_info("[TSP] Error detected!! firmware status is 0x%02x.",
				fw_status);
		nRet = MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED;
		goto MCSDL_DOWNLOAD_FINISH;
	}

	mcsdl_delay(MCSDL_DELAY_60MS);
#endif

 /*--------------------------------------------------------------
// FIRMWARE UPDATE
//------------------------------------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] FIRMWARE UPDATE\n");
#endif
	nRet = mms100_ISC_firmware_update((UINT8 *)pBianry, (UINT16)unLength,
					flash_start[nMode], flash_end[nMode]);
	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;

	/*--------------------------------------------------------------
	// LEAVE FIRMWARE UPDATE MODE
	//------------------------------------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] LEAVE FIRMWARE UPDATE MODE");
#endif
	mms100_ISC_leave_firmware_update_mode();
	mcsdl_delay(MCSDL_DELAY_60MS);
/*
	fw_status = mms100_ISC_read_firmware_status();

	if(fw_status == 0xFF || fw_status == 0x00 )
	{
		pr_info("[TSP] Living firmware update mode success!!!");
	}
	else
	{
		pr_info("[TSP] Error detected!! firmware status is 0x%02x.",
			fw_status);
		nRet = MCSDL_LEAVE_FIRMWARE_UPDATE_MODE_FAILED;
		goto MCSDL_DOWNLOAD_FINISH;
	}
*/
	nRet = MCSDL_RET_SUCCESS;

MCSDL_DOWNLOAD_FINISH:

	#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result(nRet);
	#endif

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Rebooting");
	pr_info("[TSP]  - Fin.");
	#endif

	mms100_ISC_reboot_mcs();

	return nRet;
}

static int mms100_ISC_Slave_download(UINT8 nMode)
{
	int nRet;
	int core_version = 0;
	/*---------------------------------
	// Make it ready
	//-------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Ready");
#endif

	mms100_ISC_set_ready();

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	if (nMode == 0)
		pr_info("[TSP] Slave Core F/W D/L ISC start!!!");
	else if (nMode == 1)
		pr_info("[TSP] Slave Private F/W D/L ISC start!!!");
	else
		pr_info("[TSP] Slave Public F/W D/L ISC start!!!");
	#endif

/*--------------------------------------------------------------
// INITIALIZE
//------------------------------------------------------------*/
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] ISC_DOWNLOAD_MODE_ENTER\n");
#endif
	mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);

/*--------------------------------------------------------------
// Slave download start
//------------------------------------------------------------*/
	mms100_ISC_slave_download_start(nMode+1);
	nRet = mms100_ISC_slave_crc_ok();
	if (nRet != MCSDL_RET_SUCCESS)
		goto MCSDL_DOWNLOAD_FINISH;

	nRet = MCSDL_RET_SUCCESS;

MCSDL_DOWNLOAD_FINISH:

	#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result(nRet);
	#endif

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] Rebooting");
	pr_info("[TSP]  - Fin.");
	#endif

	mms100_ISC_reboot_mcs();

	return nRet;
}


/*------------------------------------------------------------------
//
//	Sub functions
//
//----------------------------------------------------------------*/
static UINT8 mms100_ISC_read_data(UINT8 addr)
{
	UINT32 wordData = 0x00000000;
	UINT8  write_buffer[4];
	UINT8 flash_start;

	mcsdl_i2c_start();
	write_buffer[0] = ISC_MODE_SLAVE_ADDRESS << 1;
	write_buffer[1] = addr;
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16);

	 mcsdl_ISC_write_bits(wordData, 16);
	 mcsdl_delay(MCSDL_DELAY_60MS);

	 mcsdl_i2c_start();
	 wordData = (ISC_MODE_SLAVE_ADDRESS << 1 | 0x01) << 24;
	 mcsdl_ISC_write_bits(wordData, 8);
	 flash_start = mcsdl_read_byte();
	 wordData = (0x01) << 31;
	 mcsdl_ISC_write_bits(wordData, 1);
	 mcsdl_i2c_stop();
	 return flash_start;

}

static void mms100_ISC_enter_download_mode(void)
{
	UINT32 wordData = 0x00000000;
	UINT8  write_buffer[4];

	mcsdl_i2c_start();
	write_buffer[0] = ISC_MODE_SLAVE_ADDRESS << 1;
	write_buffer[1] = ISC_DOWNLOAD_MODE_ENTER;
	write_buffer[2] = 0x01;
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16) |
			(write_buffer[2] << 8);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
}

static void mms100_ISC_firmware_update_mode_enter(void)
{
	UINT32 wordData = 0x00000000;
	mcsdl_i2c_start();
	wordData = (ISC_MODE_SLAVE_ADDRESS << 1) << 24 | (0xAE << 16) |
			(0x55 << 8) | (0x00);
	mcsdl_ISC_write_bits(wordData, 32);
	wordData = 0x00000000;
	mcsdl_ISC_write_bits(wordData, 32);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
}

static UINT8 mms100_ISC_firmware_update(UINT8 *_pBinary_reordered,
						UINT16 _unDownload_size,
						UINT8 flash_start,
						UINT8 flash_end)
{

	int i = 0, j = 0, n, m;

	UINT8 fw_status;
	UINT32 wordData = 0x00000000;
	UINT16 nOffset = 0;
	UINT16 cLength = 8;
	UINT16 CRC_check_buf, CRC_send_buf, IN_data;
	UINT16 XOR_bit_1, XOR_bit_2, XOR_bit_3;

	UINT8  write_buffer[64];

	nOffset = 0;
	cLength = 8;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	pr_info("[TSP] flash start : %2d, flash end : %2d",
			flash_start, flash_end);
#endif
	while (flash_start + nOffset < flash_end) {

		CRC_check_buf = 0xFFFF;
		mcsdl_i2c_start();
		write_buffer[0] = ISC_MODE_SLAVE_ADDRESS << 1;
		write_buffer[1] = 0XAE;
		write_buffer[2] = 0XF1;
		write_buffer[3] = flash_start + nOffset;

		wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16) |
				(write_buffer[2] << 8) | write_buffer[3];
		mcsdl_ISC_write_bits(wordData, 32);
		mcsdl_delay(MCSDL_DELAY_100MS);
		mcsdl_delay(MCSDL_DELAY_100MS);

		#if MELFAS_CRC_CHECK_ENABLE
			for (m = 7; m >= 0; m--) {
				IN_data = (write_buffer[3] >> m) & 0x01;
				XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
				XOR_bit_2 = XOR_bit_1^
						(CRC_check_buf >> 11 & 0x01);
				XOR_bit_3 = XOR_bit_1^
						(CRC_check_buf >> 4 & 0x01);
				CRC_send_buf = (XOR_bit_1 << 4) |
						(CRC_check_buf >> 12 & 0x0F);
				CRC_send_buf = (CRC_send_buf << 7) |
						(XOR_bit_2 << 6) |
						(CRC_check_buf >> 5 & 0x3F);
				CRC_send_buf = (CRC_send_buf << 4) |
						(XOR_bit_3 << 3) |
						(CRC_check_buf >> 1 & 0x0007);
				CRC_check_buf = CRC_send_buf;
			}
/*			pr_info("[TSP] CRC_check_buf 0x%02x, 0x%02x",
			(UINT8)(CRC_check_buf >> 8 & 0xFF),
			(UINT8)(CRC_check_buf & 0xFF));*/
#endif
/*			if(nOffset < _unDownload_size/1024 +1)
				{
*/
			for (j = 0; j < 32; j++) {
				for (i = 0; i < cLength; i++) {
					write_buffer[i*4+3] = _pBinary_reordered[(flash_start+nOffset)*1024+j*32+i*4+0];
					write_buffer[i*4+2] = _pBinary_reordered[(flash_start+nOffset)*1024+j*32+i*4+1];
					write_buffer[i*4+1] = _pBinary_reordered[(flash_start+nOffset)*1024+j*32+i*4+2];
					write_buffer[i*4+0] = _pBinary_reordered[(flash_start+nOffset)*1024+j*32+i*4+3];
/*pr_info("[TSP] write buffer : 0x%02x,0x%02x,0x%02x,0x%02x",
write_buffer[i*4+0],write_buffer[i*4+1],write_buffer[i*4+2],write_buffer[i*4+3]);*/
#if MELFAS_CRC_CHECK_ENABLE
					for (n = 0; n < 4; n++) {
						for (m = 7; m >= 0; m--) {
							IN_data = (write_buffer[i*4+n] >> m) & 0x0001;
							XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
							XOR_bit_2 = XOR_bit_1^(CRC_check_buf >> 11 & 0x01);
							XOR_bit_3 = XOR_bit_1^(CRC_check_buf >> 4 & 0x01);
							CRC_send_buf = (XOR_bit_1 << 4) | (CRC_check_buf >> 12 & 0x0F);
							CRC_send_buf = (CRC_send_buf << 7) | (XOR_bit_2 << 6) | (CRC_check_buf >> 5 & 0x3F);
							CRC_send_buf = (CRC_send_buf << 4) | (XOR_bit_3 << 3) | (CRC_check_buf >> 1 & 0x0007);
							CRC_check_buf = CRC_send_buf;
						}
					}
/*pr_info("[TSP] CRC_check_buf 0x%02x, 0x%02x",
(UINT8)(CRC_check_buf >> 8 & 0xFF), (UINT8)(CRC_check_buf & 0xFF));*/
#endif
				}
				for (i = 0; i < cLength; i++) {
					wordData = (write_buffer[i*4+0] << 24) | (write_buffer[i*4+1] << 16) | (write_buffer[i*4+2] << 8) | write_buffer[i*4+3];
					mcsdl_ISC_write_bits(wordData, 32);
					mcsdl_delay(MCSDL_DELAY_100US);
					mcsdl_delay(MCSDL_DELAY_100US);
				}
			}
/*					}*/

#if MELFAS_CRC_CHECK_ENABLE
			write_buffer[1] =  CRC_check_buf & 0xFF;
			write_buffer[0] = CRC_check_buf >> 8 & 0xFF;
			wordData = (write_buffer[0] << 24) |
					(write_buffer[1] << 16);
			mcsdl_ISC_write_bits(wordData, 16);
/*pr_info("[TSP] CRC_data = 0x%02x 0x%02x",write_buffer[0],write_buffer[1]);*/
			mcsdl_delay(MCSDL_DELAY_100US);
#endif
			mcsdl_i2c_stop();

#if MELFAS_CRC_CHECK_ENABLE
			fw_status = mms100_ISC_read_firmware_status();
			if (fw_status == 0x03) {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
				pr_info("[TSP] Firmware update success!!!");
#endif
			} else {
				pr_info("[TSP] Error ! F/W status : 0x%02x.",
						fw_status);
				return MCSDL_FIRMWARE_UPDATE_FAILED;
			}
#endif
			nOffset += 1;
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
			pr_info("[TSP] %d KB Downloaded...", nOffset);
#endif
	}
	return MCSDL_RET_SUCCESS;

}

static UINT8 mms100_ISC_read_firmware_status()
{
	UINT32 wordData = 0x00000000;
	UINT8 fw_status;
	mcsdl_i2c_start();
	wordData = (ISC_MODE_SLAVE_ADDRESS << 1) << 24 | (0xAF << 16);
	mcsdl_ISC_write_bits(wordData, 16);
	mcsdl_i2c_stop();
	mcsdl_delay(MCSDL_DELAY_100MS);

	mcsdl_i2c_start();
	wordData = (ISC_MODE_SLAVE_ADDRESS << 1 | 0x01) << 24;
	mcsdl_ISC_write_bits(wordData, 8);
	fw_status = mcsdl_read_byte();
	wordData = (0x01) << 31;
	mcsdl_ISC_write_bits(wordData, 1);
	mcsdl_i2c_stop();
	return fw_status;
}

static void mms100_ISC_slave_download_start(UINT8 nMode)
{
	UINT32 wordData = 0x00000000;
	UINT8  write_buffer[4];

	mcsdl_i2c_start();
	write_buffer[0] = ISC_MODE_SLAVE_ADDRESS << 1;
	write_buffer[1] = ISC_DOWNLOAD_MODE;
	write_buffer[2] = nMode;
	/*nMode  0x01: core, 0x02: private custom, 0x03: public custsom*/
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16) |
			(write_buffer[2] << 8);
	mcsdl_ISC_write_bits(wordData, 24);
	mcsdl_i2c_stop();
	mcsdl_delay(MCSDL_DELAY_100MS);
}

static UINT8 mms100_ISC_slave_crc_ok(void)
{
	UINT32 wordData = 0x00000000;
	UINT8 CRC_status = 0;
	UINT8  write_buffer[4];
	UINT8 check_count = 0;


	mcsdl_i2c_start();
	write_buffer[0] = ISC_MODE_SLAVE_ADDRESS << 1;
	write_buffer[1] = ISC_READ_SLAVE_CRC_OK;
	wordData = (write_buffer[0] << 24) | (write_buffer[1] << 16);

	mcsdl_ISC_write_bits(wordData, 16);

	while (CRC_status != 0x01 && check_count < 200) {
		mcsdl_i2c_start();
		wordData = (ISC_MODE_SLAVE_ADDRESS << 1 | 0x01) << 24;
		mcsdl_ISC_write_bits(wordData, 8);
		CRC_status = mcsdl_read_byte();
		wordData = (0x01) << 31;
		mcsdl_ISC_write_bits(wordData, 1);
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		if (check_count % 10 == 0)
			pr_info("[TSP] %d sec...", check_count/10);
#endif
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

static void mms100_ISC_leave_firmware_update_mode()
{
	UINT32 wordData = 0x00000000;
	mcsdl_i2c_start();
	wordData = (ISC_MODE_SLAVE_ADDRESS << 1) << 24 | (0xAE << 16) |
			(0x0F << 8) | (0xF0);
	mcsdl_ISC_write_bits(wordData, 32);
	mcsdl_i2c_stop();
}

static void mcsdl_i2c_start(void)
{
	MCSDL_GPIO_SDA_SET_OUTPUT_ISC(1);
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SCL_SET_OUTPUT_ISC(1);
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SDA_SET_LOW();
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SCL_SET_LOW();
}

static void mcsdl_i2c_stop(void)
{
	MCSDL_GPIO_SCL_SET_OUTPUT_ISC(0);
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SDA_SET_OUTPUT_ISC(0);
	mcsdl_delay(MCSDL_DELAY_1US);

	MCSDL_GPIO_SCL_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_1US);
	MCSDL_GPIO_SDA_SET_HIGH();

}

static void mms100_ISC_set_ready(void)
{
/*--------------------------------------------
// Tkey module reset
//------------------------------------------*/

	MCSDL_VDD_SET_LOW(); /* power*/

/*	MCSDL_SET_GPIO_I2C();*/

	MCSDL_GPIO_SDA_SET_OUTPUT_ISC(1);
	MCSDL_GPIO_SDA_SET_HIGH();

	MCSDL_GPIO_SCL_SET_OUTPUT_ISC(1);
	MCSDL_GPIO_SCL_SET_HIGH();

	MCSDL_RESETB_SET_INPUT();

/*	MCSDL_CE_SET_HIGH;
	MCSDL_CE_SET_OUTPUT();*/
	mcsdl_delay(MCSDL_DELAY_60MS); /* Delay for Stable VDD*/

	MCSDL_VDD_SET_HIGH();

	mcsdl_delay(MCSDL_DELAY_500MS); /*// Delay for Stable VDD*/
}


static void mms100_ISC_reboot_mcs(void)
{
/*--------------------------------------------
// Tkey module reset
//------------------------------------------*/
	mms100_ISC_set_ready();
}

static UINT8 mcsdl_read_ack(void)
{
	int i;
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

static void mcsdl_ISC_read_32bits(UINT8 *pData)
{
	int i, j;
	MCSDL_GPIO_SDA_SET_LOW();
	MCSDL_GPIO_SDA_SET_INPUT();

	for (i = 3; i >= 0; i--) {
		pData[i] = 0;
		for (j = 0; j < 8 ; j++) {
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
	while (!MCSDL_GPIO_SCL_IS_HIGH())
		;
	MCSDL_GPIO_SCL_SET_OUTPUT_ISC(1);

	for (i = 0; i < 8 ; i++) {
		pData <<= 1;
		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_3US);
		if (MCSDL_GPIO_SDA_IS_HIGH())
			pData |= 0x01;
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_3US);
	}
	return pData;
}


static void mcsdl_ISC_write_bits(UINT32 wordData, int nBits)
{
	int i;

	MCSDL_GPIO_SDA_SET_OUTPUT_ISC(0);
	MCSDL_GPIO_SDA_SET_LOW();

	for (i = 0; i < nBits; i++) {
		if (wordData & 0x80000000)
			MCSDL_GPIO_SDA_SET_HIGH();
		else
			MCSDL_GPIO_SDA_SET_LOW();

		mcsdl_delay(MCSDL_DELAY_3US);

		MCSDL_GPIO_SCL_SET_HIGH();
		mcsdl_delay(MCSDL_DELAY_3US);
		MCSDL_GPIO_SCL_SET_LOW();
		mcsdl_delay(MCSDL_DELAY_3US);

		wordData <<= 1;
		if ((i%8) == 7) {
			mcsdl_read_ack();

			MCSDL_GPIO_SDA_SET_OUTPUT_ISC(0);
			MCSDL_GPIO_SDA_SET_LOW();
		}
	}
}


/*============================================================
//
//	Debugging print functions.
//
//==========================================================*/

#ifdef MELFAS_ENABLE_DBG_PRINT

static void mcsdl_ISC_print_result(int nRet)
{
	if (nRet == MCSDL_RET_SUCCESS) {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
		pr_info("[TSP] Firmware downloading SUCCESS.");
#endif
	} else {
		pr_info("[TSP] Firmware downloading FAILED  :  ");

		switch (nRet) {

		case MCSDL_RET_SUCCESS:
			pr_info("[TSP] MCSDL_RET_SUCCESS");
			break;
		case MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED:
			pr_info("[TSP] MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED");
			break;
		case MCSDL_RET_PROGRAM_VERIFY_FAILED:
			pr_info("[TSP] MCSDL_RET_PROGRAM_VERIFY_FAILED");
			break;
		case MCSDL_RET_PROGRAM_SIZE_IS_WRONG:
			pr_info("[TSP] MCSDL_RET_PROGRAM_SIZE_IS_WRONG");
			break;
		case MCSDL_RET_VERIFY_SIZE_IS_WRONG:
			pr_info("[TSP] MCSDL_RET_VERIFY_SIZE_IS_WRONG");
			break;
		case MCSDL_RET_WRONG_BINARY:
			pr_info("[TSP] MCSDL_RET_WRONG_BINARY");
			break;
		case MCSDL_RET_READING_HEXFILE_FAILED:
			pr_info("[TSP] MCSDL_RET_READING_HEXFILE_FAILED");
			break;
		case MCSDL_RET_FILE_ACCESS_FAILED:
			pr_info("[TSP] MCSDL_RET_FILE_ACCESS_FAILED");
			break;
		case MCSDL_RET_MELLOC_FAILED:
			pr_info("[TSP] MCSDL_RET_MELLOC_FAILED");
			break;
		case MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED:
			pr_info("[TSP] MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED");
			break;
		case MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER:
			pr_info("[TSP] MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER");
			break;
		case MCSDL_RET_WRONG_MODULE_REVISION:
			pr_info("[TSP] MCSDL_RET_WRONG_MODULE_REVISION");
			break;
		default:
			pr_info("[TSP] UNKNOWN ERROR. [0x%02X].", nRet);
			break;
		}
	}
}

#endif
