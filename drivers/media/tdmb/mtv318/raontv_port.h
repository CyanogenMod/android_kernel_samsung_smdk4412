/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv_port.h
 *
 * Description :  RAONTECH TV configuration header file.
 *
 * Copyright (C) (2011, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RAONTV_PORT_H__
#define __RAONTV_PORT_H__

/*==============================================================================
 * Includes the user header files if neccessry.
 *============================================================================*/
#if defined(__KERNEL__) /* Linux kernel */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/mutex.h>

#elif defined(WINCE)
#include <windows.h>
#include <drvmsg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*##############################################################################
 #
 # COMMON configurations
 #
 #############################################################################*/
/*==============================================================================
 * The slave address for I2C and SPI, the base address for EBI2.
 *============================================================================*/
#define RAONTV_CHIP_ADDR	0x86

/*==============================================================================
 * Modifies the basic data types if neccessry.
 *============================================================================*/
#define BOOL		int
#define S8		s8
#define U8		u8
#define S16		s16
#define U16		u16
#define S32		s32
#define U32		u32

#define INT		int
#define UINT		unsigned int
#define LONG		long
#define ULONG		unsigned long

#define INLINE		inline

/*==============================================================================
 * Selects the TV mode(s) to target product.
 *============================================================================*/
/*#define RTV_ISDBT_ENABLE*/
#define RTV_TDMB_ENABLE
/*#define RTV_FM_ENABLE*/
/*#define RTV_DAB_ENABLE*/

/*==============================================================================
 * Defines the package type of chip to target product.
 *============================================================================*/
#define RAONTV_CHIP_PKG_WLCSP	/* MTV220/318*/
/*#define RAONTV_CHIP_PKG_QFN		// MTV818*/
/* #define RAONTV_CHIP_PKG_LGA	 MTV250/350 */
/*==============================================================================
 * Defines the external source freqenecy in KHz.
 * Ex> #define RTV_SRC_CLK_FREQ_KHz	36000 // 36MHz
 *==============================================================================
 * MTV250 : #define RTV_SRC_CLK_FREQ_KHz  32000  //must be defined
 * MTV350 : #define RTV_SRC_CLK_FREQ_KHz  24576  //must be defined
 *============================================================================*/
#define RTV_SRC_CLK_FREQ_KHz			24576

/*==============================================================================
 * Define the power type.
 *============================================================================*/
/*#define RTV_PWR_EXTERNAL*/
#define RTV_PWR_LDO
/* #define RTV_PWR_DCDC */

/*==============================================================================
 * Defines the I/O voltage.
 *============================================================================*/
#define RTV_IO_1_8V
/*#define RTV_IO_2_5V*/
/*#define RTV_IO_3_3V*/

#if defined(RTV_IO_2_5V) || defined(RTV_IO_3_3V)
#error "If VDDIO pin is connected with IO voltage,"
	" RTV_IO_1_8V must be defined,please check the HW pin connection"
#error "If VDDIO pin is connected with GND,"
	" IO voltage must be defined same as AP IO voltage"
#endif

/*==============================================================================
 * Defines the Host interface.
 *============================================================================*/
/*#define RTV_IF_MPEG2_SERIAL_TSIF*/ /* I2C + TSIF Master Mode. */
/*#define RTV_IF_MPEG2_PARALLEL_TSIF*/ /* I2C + TSIF Master Mode.
					Support only 1seg &TDMB Application!*/
/*#define RTV_IF_QUALCOMM_TSIF*/ /* I2C + TSIF Master Mode */
#define RTV_IF_SPI /* AP: SPI Master Mode */
/*#define RTV_IF_SPI_SLAVE*/ /* AP: SPI Slave Mode */
/*#define RTV_IF_EBI2*/ /* External Bus Interface Slave Mode */

/*==============================================================================
 * Defines the clear mode of interrupts for EBI/SPI in Individual mode .
 *============================================================================*/
#define RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE /* for Nested ISR and TSIF. */
/* #define RTV_MSC_INTR_MEM_ACC_CLR_MODE */ /* for NOT nested ISR.*/

/*==============================================================================
 * Defines the delay macro in milliseconds.
 *============================================================================*/
#if defined(__KERNEL__)/* Linux kernel */
#define RTV_DELAY_MS(ms)	mdelay(ms)

#elif defined(WINCE)
#define RTV_DELAY_MS(ms)	Sleep(ms)

#else
extern void mtv818_delay_ms(int ms);
#define RTV_DELAY_MS(ms)	mtv818_delay_ms(ms) /* TODO */
#endif

/*==============================================================================
 * Defines the debug message macro.
 *============================================================================*/
#if 1
#define RTV_DBGMSG0(fmt)					printk(fmt)
#define RTV_DBGMSG1(fmt, arg1)				printk(fmt, arg1)
#define RTV_DBGMSG2(fmt, arg1, arg2)		printk(fmt, arg1, arg2)
#define RTV_DBGMSG3(fmt, arg1, arg2, arg3)	printk(fmt, arg1, arg2, arg3)
#else
/* To eliminates the debug messages. */
#define RTV_DBGMSG0(fmt)					((void)0)
#define RTV_DBGMSG1(fmt, arg1)				((void)0)
#define RTV_DBGMSG2(fmt, arg1, arg2)		((void)0)
#define RTV_DBGMSG3(fmt, arg1, arg2, arg3)	((void)0)
#endif
/*#### End of Common ###########*/

/*##############################################################################
 #
 # ISDB-T specific configurations
 #
 #############################################################################*/
/*==============================================================================
 * Defines the NOTCH FILTER setting Enable.
 * In order to reject GSM/CDMA blocker, NOTCH FILTER must be defined.
 * This feature used for module company in the JAPAN.
 *============================================================================*/
/*
//#if defined(RTV_ISDBT_ENABLE)  //Do not use
//	#ifdef RAONTV_CHIP_PKG_WLCSP
//	#define RTV_NOTCH_FILTER_ENABLE
//	#endif
//#endif
*/
/*##############################################################################
 #
 # T-DMB/DAB specific configurations
 #
 #############################################################################*/
#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
#define RTV_FIC_POLLING_MODE

/* Defines the maximum number of Sub Channel to be open simultaneously. */
#define RTV_MAX_NUM_SUB_CHANNEL_USED		1

/* Defines CIF or Individual Mode for T-DMB/DAB BY RAONTECH. */
/*#define RTV_CIF_MODE_ENABLED*/
/*#define RTV_CIF_HEADER_INSERTED*/
/* Select the copying method of CIF decoded data(FIC and MSC)
	which copy_to_user() or memcpy() to fast operation for LINUX Kernel. */
/*#define RTV_CIF_LINUX_USER_SPACE_COPY_USED*/
#endif /* #if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)  */

/*##############################################################################
 #
 # FM specific configurations
 #
 #############################################################################*/
#define RTV_FM_CH_MIN_FREQ_KHz		76000
#define RTV_FM_CH_MAX_FREQ_KHz		108000
#define RTV_FM_CH_STEP_FREQ_KHz		100 /* in KHz */
/*##############################################################################
 #
 # Host Interface specific configurations
 #
 #############################################################################*/
#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_QUALCOMM_TSIF) \
	|| defined(RTV_IF_MPEG2_PARALLEL_TSIF)
#define RTV_IF_TSIF
#endif

#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_QUALCOMM_TSIF)
	#define RTV_IF_SERIAL_TSIF
#endif


#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
/*==============================================================================
 * Defines the TSIF interface for MPEG2 or QUALCOMM TSIF.
 *============================================================================*/
/*#define RTV_TSIF_FORMAT_1*/
/*#define RTV_TSIF_FORMAT_2*/
#define RTV_TSIF_FORMAT_3
/*#define RTV_TSIF_FORMAT_4*/
/*#define RTV_TSIF_FORMAT_5*/

/*#define RTV_TSIF_CLK_SPEED_DIV_2 // Host Clk/2*/
#define RTV_TSIF_CLK_SPEED_DIV_4 /* Host Clk/4 */
/*#define RTV_TSIF_CLK_SPEED_DIV_6 // Host Clk/6*/
/*#define RTV_TSIF_CLK_SPEED_DIV_8 // Host Clk/8*/

/*==========================================================================
 * Defines the register I/O macros.
 *========================================================================*/
unsigned char mtv818_i2c_read(U8 reg);
void mtv818_i2c_read_burst(U8 reg, U8 *buf, int size);
void mtv818_i2c_write(U8 reg, U8 val);
#define	RTV_REG_GET(reg) \
	mtv818_i2c_read((U8)(reg))
#define	RTV_REG_BURST_GET(reg, buf, size) \
	mtv818_i2c_read_burst((U8)(reg), buf, size)
#define	RTV_REG_SET(reg, val) \
	mtv818_i2c_write((U8)(reg), (U8)(val))
#define	RTV_REG_MASK_SET(reg, mask, val)\
do { \
	U8 tmp; \
	tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val)); \
	RTV_REG_SET(reg, tmp); \
} while (0)

#elif defined(RTV_IF_SPI)
/*=========================================================================
 * Defines the register I/O macros.
 *========================================================================*/
unsigned char mtv_spi_read(unsigned char reg);
void mtv_spi_read_burst(unsigned char reg, unsigned char *buf, int size);
void mtv_spi_write(unsigned char reg, unsigned char val);

#define	RTV_REG_GET(reg) \
	(U8)mtv_spi_read((U8)(reg))
#define	RTV_REG_BURST_GET(reg, buf, size) \
	mtv_spi_read_burst((U8)(reg), buf, size)
#define	RTV_REG_SET(reg, val) \
	mtv_spi_write((U8)(reg), (U8)(val))
#define	RTV_REG_MASK_SET(reg, mask, val) \
do { \
	U8 tmp; \
	tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val)); \
	RTV_REG_SET(reg, tmp); \
} while (0)



#else
#error "Must define the interface definition !"
#endif

#if defined(RTV_IF_SPI) \
	|| ((defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)) \
	&& !defined(RTV_FIC_POLLING_MODE))
#if defined(__KERNEL__)
extern struct mutex raontv_guard;
#define RTV_GUARD_INIT		mutex_init(&raontv_guard)
#define RTV_GUARD_LOCK		mutex_lock(&raontv_guard)
#define RTV_GUARD_FREE		mutex_unlock(&raontv_guard)
#define RTV_GUARD_DEINIT	((void)0)

#elif defined(WINCE)
extern CRITICAL_SECTION raontv_guard;
#define RTV_GUARD_INIT      InitializeCriticalSection(&raontv_guard)
#define RTV_GUARD_LOCK      EnterCriticalSection(&raontv_guard)
#define RTV_GUARD_FREE      LeaveCriticalSection(&raontv_guard)
#define RTV_GUARD_DEINIT    DeleteCriticalSection(&raontv_guard)
#else
/* temp: TODO */
#define RTV_GUARD_INIT		((void)0)
#define RTV_GUARD_LOCK		((void)0)
#define RTV_GUARD_FREE	((void)0)
#define RTV_GUARD_DEINIT	((void)0)
#endif

#else
#define RTV_GUARD_INIT		((void)0)
#define RTV_GUARD_LOCK		((void)0)
#define RTV_GUARD_FREE	((void)0)
#define RTV_GUARD_DEINIT	((void)0)
#endif

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)\
|| defined(__KERNEL__) || defined(WINCE)
	#ifdef RTV_MSC_INTR_MEM_ACC_CLR_MODE
	#undef RTV_MSC_INTR_MEM_ACC_CLR_MODE
	#endif

	#ifndef RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
	#define RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
	#endif
#endif


/*==============================================================================
 * Check erros
 *============================================================================*/
#if !defined(RAONTV_CHIP_PKG_WLCSP) \
	&& !defined(RAONTV_CHIP_PKG_QFN) \
	&& !defined(RAONTV_CHIP_PKG_LGA)
#error "Must define the package type !"
#endif

#if !defined(RTV_PWR_EXTERNAL) \
	&& !defined(RTV_PWR_LDO) \
	&& !defined(RTV_PWR_DCDC)
#error "Must define the power type !"
#endif

#if !defined(RTV_IO_1_8V) \
	&& !defined(RTV_IO_2_5V) \
	&& !defined(RTV_IO_3_3V)
#error "Must define I/O voltage!"
#endif

#if defined(RTV_IF_MPEG2_SERIAL_TSIF) \
	|| defined(RTV_IF_SPI_SLAVE) \
	|| defined(RTV_IF_MPEG2_PARALLEL_TSIF) \
	|| defined(RTV_IF_QUALCOMM_TSIF) \
	|| defined(RTV_IF_SPI)
#if (RAONTV_CHIP_ADDR >= 0xFF)
#error "Invalid chip address"
#endif
#elif defined(RTV_IF_EBI2)
#if (RAONTV_CHIP_ADDR <= 0xFF)
#error "Invalid chip address"
#endif

#else
#error "Must define the interface definition !"
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
#ifndef RTV_MAX_NUM_SUB_CHANNEL_USED
#error "Should be define!"
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
#if (RTV_MAX_NUM_SUB_CHANNEL_USED < 0) || (RTV_MAX_NUM_SUB_CHANNEL_USED > 5)
#error "Must from 1 to 5"
#endif
#else
#if (RTV_MAX_NUM_SUB_CHANNEL_USED < 0) || (RTV_MAX_NUM_SUB_CHANNEL_USED > 4)
#error "Must from 1 to 4"
#endif
#endif
#else
#ifdef RTV_MAX_NUM_SUB_CHANNEL_USED
#undef RTV_MAX_NUM_SUB_CHANNEL_USED
#define RTV_MAX_NUM_SUB_CHANNEL_USED		1 /* To not make error. */
#else
#define RTV_MAX_NUM_SUB_CHANNEL_USED		1
#endif
#endif

#ifdef RTV_IF_MPEG2_PARALLEL_TSIF
#if defined(RTV_FM_ENABLE) \
	|| defined(RTV_DAB_ENABLE) \
	|| defined(RAONTV_CHIP_PKG_WLCSP) \
	|| defined(RAONTV_CHIP_PKG_LGA)
#error "Not support parallel TSIF!"
#endif

#if defined(RTV_TDMB_ENABLE) && (RTV_MAX_NUM_SUB_CHANNEL_USED > 1)
#error "Not support T-DMB multi sub channel mode!"
#endif

#if defined(RTV_DAB_ENABLE) && (RTV_MAX_NUM_SUB_CHANNEL_USED > 1)
#error "Not support DAB multi sub channel mode!"
#endif
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
#if !defined(RTV_MSC_INTR_MEM_ACC_CLR_MODE) \
	&& !defined(RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE)
#error " Should selects an interrupt clear mode"
#endif

#if defined(RTV_MSC_INTR_MEM_ACC_CLR_MODE) \
	&& defined(RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE)
#error " Should selects an interrupt clear mode"
#endif
#endif

#if defined(RTV_DAB_ENABLE) && defined(RTV_TDMB_ENABLE)
#error "Should select RTV_DAB_ENABLE(B3, L-BAND, B3-Korea) "
	"or RTV_TDMB_ENABLE(B3-Korea)"
#endif

#if defined(RTV_CIF_HEADER_INSERTED) && !defined(RTV_CIF_MODE_ENABLED)
#error "Should defines RTV_CIF_MODE_ENABLED"
#endif

void rtvOEM_ConfigureInterrupt(void);
void rtvOEM_PowerOn(int on);

#ifdef __cplusplus
}
#endif

#endif /* __RAONTV_PORT_H__ */

