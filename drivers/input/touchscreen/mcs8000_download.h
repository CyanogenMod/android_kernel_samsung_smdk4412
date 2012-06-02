//--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download base v1.0 2010.04.05
//
//
//--------------------------------------------------------


#ifndef __MELFAS_FIRMWARE_DOWNLOAD_H__
#define __MELFAS_FIRMWARE_DOWNLOAD_H__
#include <linux/melfas_ts.h>


//=====================================================================
//
//   MELFAS Firmware download pharameters
//
//=====================================================================

#define MELFAS_TRANSFER_LENGTH					(32/8)		// Fixed value
#define MELFAS_FIRMWARE_MAX_SIZE				(32*1024)

#define MELFAS_2CHIP_DOWNLOAD_ENABLE                0       // 0 : 1Chip Download, 1: 2Chip Download

//----------------------------------------------------
//   ISP Mode
//----------------------------------------------------
#define ISP_MODE_ERASE_FLASH					0x01
#define ISP_MODE_SERIAL_WRITE					0x02
#define ISP_MODE_SERIAL_READ					0x03
#define ISP_MODE_NEXT_CHIP_BYPASS				0x04


//----------------------------------------------------
//   Return values of download function
//----------------------------------------------------
#define MCSDL_RET_SUCCESS						0x00
#define MCSDL_RET_ERASE_FLASH_VERIFY_FAILED		0x01
#define MCSDL_RET_PROGRAM_VERIFY_FAILED			0x02

#define MCSDL_RET_PROGRAM_SIZE_IS_WRONG			0x10
#define MCSDL_RET_VERIFY_SIZE_IS_WRONG			0x11
#define MCSDL_RET_WRONG_BINARY					0x12

#define MCSDL_RET_READING_HEXFILE_FAILED		0x21
#define MCSDL_RET_FILE_ACCESS_FAILED			0x22
#define MCSDL_RET_MELLOC_FAILED					0x23

#define MCSDL_RET_WRONG_MODULE_REVISION			0x30


//----------------------------------------------------
//	When you can't control VDD nor CE.
//	Set this value 1
//	Then Melfas Chip can prepare chip reset.
//----------------------------------------------------

#define MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD	0							// If 'enable download command' is needed ( Pinmap dependent option ).

//============================================================
//
//	Port setting. ( Melfas preset this value. )
//
//============================================================

// If want to set Enable : Set to 1

#define MCSDL_USE_CE_CONTROL						0
#define MCSDL_USE_VDD_CONTROL						1
#define MCSDL_USE_RESETB_CONTROL                    1

void mcsdl_vdd_on(void);
void mcsdl_vdd_off(void);

//#define GPIO_TOUCH_INT	19
//#define GPIO_I2C0_SCL   30
//#define GPIO_I2C0_SDA   29


/* Touch Screen Interface Specification Multi Touch (V0.5) */

/* REGISTERS */
#define MCSTS_STATUS_REG        0x00 //Status
#define MCSTS_MODE_CONTROL_REG  0x01 //Mode Control
#define MCSTS_RESOL_HIGH_REG    0x02 //Resolution(High Byte)
#define MCSTS_RESOL_X_LOW_REG   0x08 //Resolution(X Low Byte)
#define MCSTS_RESOL_Y_LOW_REG   0x0a //Resolution(Y Low Byte)
#define MCSTS_INPUT_INFO_REG    0x10 //Input Information
#define MCSTS_POINT_HIGH_REG    0x11 //Point(High Byte)
#define MCSTS_POINT_X_LOW_REG   0x12 //Point(X Low Byte)
#define MCSTS_POINT_Y_LOW_REG   0x13 //Point(Y Low Byte)
#define MCSTS_STRENGTH_REG      0x14 //Strength
#define MCSTS_MODULE_VER_REG    0x30 //H/W Module Revision
#define MCSTS_FIRMWARE_VER_REG  0x31 //F/W Version



//============================================================
//
//	Porting factors for Baseband
//
//============================================================


#include "mcs8000_download_porting.h"


//----------------------------------------------------
//	Functions
//----------------------------------------------------

int mcsdl_download_binary_data(struct melfas_tsi_platform_data *ts_data); // with binary type .c   file.
int mcsdl_download_binary_file(struct melfas_tsi_platform_data *ts_data); // with binary type .bin file.

#if MELFAS_ENABLE_DELAY_TEST					// For initial porting test.
void mcsdl_delay_test(INT32 nCount);
#endif


#endif		//#ifndef __MELFAS_FIRMWARE_DOWNLOAD_H__
