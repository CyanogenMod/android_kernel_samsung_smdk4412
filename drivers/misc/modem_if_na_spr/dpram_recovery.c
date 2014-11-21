#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif    /* CONFIG_PROC_FS */
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/netdevice.h>
#include "dpram_recovery.h"
#define _DEBUG_

#include <mach/gpio.h>

#ifdef _DEBUG_
#define MSGCRIT "\x1b[1;31m"
#define MSGERR "\x1b[1;33m"
#define MSGWARN "\x1b[1;35m"
#define MSGINFO "\x1b[1;32m"
#define MSGDBG "\x1b[1;37m"

#define MSGEND "\x1b[0m\n"
#else
#define MSGCRIT
#define MSGERR
#define MSGWARN
#define MSGINFO
#define MSGDBG
#define MSGEND
#endif

#define DRIVER_NAME "dpram_recovery"
#define MAJOR_NUM 245

#ifndef min
#define min(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#define MAKEWORD(a, b)      ((u16)(((u8)(a)) | ((u16)((u8)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((u16)(a)) | ((u32)((u16)(b))) << 16))
#define LOWORD(l)           ((u16)(l))
#define HIWORD(l)           ((u16)(((u32)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((u8)(w))
#define HIBYTE(w)           ((u8)(((u16)(w) >> 8) & 0xFF))

#ifdef IRQ_DPRAM_AP_INT_N
#undef IRQ_DPRAM_AP_INT_N
#endif
#define IRQ_DPRAM_AP_INT_N  IRQ_MODEM_IF /*IDPRAM's special interrupt in AP*/

#define S5PV310_MDM_IF_SEL 2

static u16	SeqIdx;
static u16  g_TotFrame;
static u16  g_CurFrame = 1;
static u16 error_code = 0x0;


struct dpram_dev {
	int memsize;
	int dpram_vbase;
	struct cdev cdev;
};

static struct dpram_dev *dpram;
static struct dpram_firmware fw;

#define CRC_TAB_SIZE	256             /* 2^CRC_TAB_BITS      */
#define CRC_16_L_SEED	0xFFFF

const u16 CRC_Table[CRC_TAB_SIZE] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static int dpram_recovery_ioremap(struct dpram_dev *dev);
static int dpram_recovery_modem_pwr_status(void);
static void dpram_recovery_modem_pwroff(void);
static int dpram_recovery_modem_pwron(void);
static u32 dpram_recovery_ReadMagicCode(void);
static u32 dpram_recovery_ReadMagicCode_fota(void);
static void dpram_recovery_WriteMagicCode(u32 dwCode);
static void dpram_recovery_WriteMagicCode_fota(u32 dwCode);
static int dpram_recovery_dpram_Init(void);
static int dpram_recovery_download_Init(void);
static u16 dpram_recovery_ReadCommand(void);
static void dpram_recovery_WriteCommand(u16 nCmd);
static u32 dpram_recovery_check_command(u16 intr, u16 mask);
static int dpram_recovery_wait_response(u16 cmd_mask);
static int dpram_recovery_WaitReadyPhase(void);
static int dpram_recovery_write_modem_firmware(struct dpram_dev *dev,
				char __user *firmware, int size);
static int dpram_recovery_write_modem_full_firmware(struct dpram_dev *dev,
				char __user *firmware, int size);
static u32 dpram_recovery_WriteImage(u8 * const pBuf, u32 dwWriteLen);
static u16 dpram_recovery_CalcTotFrame(u32 nDividend, u16 nDivisor);
static void dpram_recovery_WriteDoneRequest(void);
static int dpram_recovery_SendDonePhase(void);
static int dpram_recovery_UpdateRequestPhase(void);
static u32 dpram_recovery_DownloadControl(u8 *pBufIn, u32 Len);
static u32 dpram_recovery_DownloadControl_fullfirmware(u8 *pBufIn, u32 Len);
static u32 dpram_recovery_StatusNotiPhase(u32 *pPercent, u8 *UpDoneNotiFlag);
static u32 dpram_recovery_StatusNotiPhase_download(u32 *pPercent,
						   u8 *UpDoneNotiFlag);
static int dpram_recovery_check_status(struct dpram_dev *dev, int __user *pct);
static int dpram_recovery_check_status_download(struct dpram_dev *dev,
						int __user *pct);
static int register_interrupt_handler(void);

struct Up_Noti {
	u16 Bop;
	u16 progress_status;
	u16 error_code;
	u16 Eop;
};

struct Up_Notification {
	u16 Bop;
	u16 Region;
	u16 Percent;
	u16 Eop;
};

static int
inline __writel_once(struct dpram_dev *dev, int addr, int data)
{
	*(int *)(dev->dpram_vbase+addr) = data;

	return 0;
}

static int
dpram_recovery_ioremap(struct dpram_dev *dev)
{
	int i;

	/*Clock Settings for Modem IF ====>FixMe */
	u32 val;
	void __iomem *regs = ioremap(0x10030000, 0x10000);

	printk(MSGDBG "%s" MSGEND, __func__);
	/*Enabling Clock for ModemIF in Reg CLK_GATE_IP_PERIL:0x1003C950 */
	val = readl(regs + 0xC950);
	val |= 0x10000000;
	writel(val, regs + 0xC950);
	iounmap(regs);

	dev->dpram_vbase = (int)ioremap_nocache(DPRAM_BASE_ADDR, DPRAM_SIZE);

	if (dev->dpram_vbase == 0) {
		printk(KERN_ERR "failed ioremap\n");
		return -ENOENT;
	}

	printk(KERN_DEBUG "dpram vbase=0x%8x\n", dev->dpram_vbase);

	dev->memsize = DPRAM_SIZE;

	printk(MSGDBG "dpram_vbase = 0x%08x" MSGEND, dev->dpram_vbase);
	printk(MSGDBG "memsize = 0x%08x" MSGEND, dev->memsize);

	for (i = 0; i < DPRAM_SIZE; i = i+4) {
		if (__writel_once(dev, i, 0xffffffff)) {
			printk(KERN_DEBUG "Exit during dpram"
				" initialization.\n");
			return 0;
		}
	}

	return 0;
}

static int
dpram_recovery_modem_pwr_status(void)
{
	printk(KERN_ERR "%s Modem Active: %d\n", __func__,
				 gpio_get_value(GPIO_QSC_PHONE_ACTIVE));

	return gpio_get_value(GPIO_QSC_PHONE_ACTIVE);
}

static void
dpram_recovery_modem_pwroff(void)
{
	int phone_wait_cnt = 0;

	printk(KERN_ERR "%s\n", __func__);

	/* phone power off */
	gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);

	/*  confirm phone off */
	while (1) {
		if (gpio_get_value(GPIO_QSC_PHONE_ACTIVE)) {
			gpio_set_value(GPIO_QSC_PHONE_RST, 0);
			if (phone_wait_cnt > 1) {
				printk(KERN_ALERT "%s: PHONE OFF "
							"Failed\n", __func__);
				break;
			}
			phone_wait_cnt++;
			mdelay(1000);
		} else {
			printk(KERN_ALERT "%s: PHONE OFF Success\n", __func__);
			break;
		}
	}

}

static int
dpram_recovery_modem_pwron(void)
{
	printk(KERN_ERR "%s\n", __func__);

	gpio_direction_output(GPIO_QSC_PHONE_RST, GPIO_LEVEL_HIGH);
	gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);
	msleep(100);

	gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_HIGH);
	msleep(400);
	msleep(400);
	msleep(200);
	gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_LOW);


	return 1;
}

static u32
dpram_recovery_ReadMagicCode(void)
{
	u32 dwCode;
	u32 *dwMagicCodeAddress;

	dwMagicCodeAddress = (u32 *)(dpram->dpram_vbase);
	memcpy((void *)&dwCode, (void *)(dwMagicCodeAddress), 4);

	return dwCode;
}

static void
dpram_recovery_WriteMagicCode(u32 dwCode)
{
	u32 *dwMagicCodeAddress;
	dwMagicCodeAddress = (u32 *)(dpram->dpram_vbase);

	SeqIdx = 1;
	memcpy((void *)dwMagicCodeAddress, (void *)&dwCode, 4);
}

static u32
dpram_recovery_ReadMagicCode_fota(void)
{
	u32 dwCode;
	u32 *dwMagicCodeAddress;

	dwMagicCodeAddress = (u32 *)(dpram->dpram_vbase);
	memcpy((void *)&dwCode, (void *)(dwMagicCodeAddress + 1), 4);

	return dwCode;
}

static void
dpram_recovery_WriteMagicCode_fota(u32 dwCode)
{
	u32 *dwMagicCodeAddress;
	dwMagicCodeAddress = (u32 *)(dpram->dpram_vbase);

	SeqIdx = 1;
	memcpy((void *)(dwMagicCodeAddress + 1), (void *)&dwCode, 4);
}


static int
dpram_recovery_dpram_Init(void)
{
	const u32 dwMagicCodeToWrite = MAGIC_FODN;
	const u32 dwMagicCodeToWrite_fota = MAGIC_FOTA;
	u32 dwMagicCode = 0;
	u32 dwMagicCode_fota = 0;
	int iRetry = 0, nCnt = 0;

	printk(KERN_DEBUG "+DPRAM_Init\n");

	/* Write the magic code */
	for (iRetry = 1; iRetry > 0 ; iRetry--) {
		dwMagicCode = dpram_recovery_ReadMagicCode();

		if (dwMagicCode == dwMagicCodeToWrite) {
			printk(KERN_DEBUG "dwMagicCode == dwMagicCodeToWrite!!\n");
			break;
		}
		dpram_recovery_WriteMagicCode(dwMagicCodeToWrite);

		dwMagicCode = dpram_recovery_ReadMagicCode();
		printk(KERN_ERR "\n #### magic code: %x\n", dwMagicCode);
	}

	for (iRetry = 1; iRetry > 0 ; iRetry--) {

		dwMagicCode_fota = dpram_recovery_ReadMagicCode_fota();

		if (dwMagicCode_fota == dwMagicCodeToWrite_fota)
			break;
		dpram_recovery_WriteMagicCode_fota(dwMagicCodeToWrite_fota);

		dwMagicCode_fota = dpram_recovery_ReadMagicCode_fota();
		printk(KERN_ERR "\n ### magic code for FOTA: %x\n",
			dwMagicCode_fota);
	}

	dpram_recovery_modem_pwron();

	/* Check phone on status */
	while (!dpram_recovery_modem_pwr_status()) {
		msleep(1000);
		nCnt++;
		if (nCnt >= 20) {
			printk(KERN_DEBUG "no phone active!!\n");
			return FALSE;
		}
	}
	/* added one second delay after phone on */
	mdelay(1000);
	printk(KERN_DEBUG "Phone Active!!!\n");
	printk(KERN_DEBUG "-DPRAM_Init\n");
	return TRUE;
}

/*  download */
static int
dpram_recovery_download_Init(void)
{
	const u32 dwMagicCodeToWrite = MAGIC_DMDL;
	u32 dwMagicCode = 0;
	int iRetry = 0, nCnt = 0;

	printk(KERN_DEBUG "+DPRAM_Init\n");

	/* Write the magic code */
	for (iRetry = 1; iRetry > 0 ; iRetry--) {
		dwMagicCode = dpram_recovery_ReadMagicCode();

		if (dwMagicCode == dwMagicCodeToWrite) {
			printk(KERN_DEBUG "dwMagicCode == dwMagicCodeToWrite!!\n");
			break;
		}
		dpram_recovery_WriteMagicCode(dwMagicCodeToWrite);

		dwMagicCode = dpram_recovery_ReadMagicCode();
		printk(KERN_ERR "\n #### magic code: %x\n", dwMagicCode);
	}

	dpram_recovery_modem_pwron();

	/* Check phone on status */
	while (!dpram_recovery_modem_pwr_status()) {
		msleep(1000);
		nCnt++;
		if (nCnt >= 20) {
			printk(KERN_DEBUG "no phone active!!\n");
			return FALSE;
		}
	}
	mdelay(1000);
	printk(KERN_DEBUG "Phone Active!!!\n");
	printk(KERN_DEBUG "-DPRAM_Init\n");
	return TRUE;
}

/* download */
static u16
dpram_recovery_ReadCommand(void)
{
	u16 nCmd = 0;

	memcpy((void *)&nCmd, (void *)(dpram->dpram_vbase +
				DPRAM_PHONE2PDA_INTERRUPT_ADDRESS), 2);

	return nCmd;
}

static void
dpram_recovery_clear_modem_command(void)
{
	u16 clear;
	clear = 0x0000;

	printk(KERN_DEBUG "dpram_recovery_clear_modem_command\n");

	memcpy((void *)(dpram->dpram_vbase +
		DPRAM_PHONE2PDA_INTERRUPT_ADDRESS), (void *)&clear, 2);
}

static void
dpram_recovery_WriteCommand(u16 nCmd)
{
	printk(KERN_DEBUG "\n [DPRAM_WriteCommand] Start : 0x%04x\n", nCmd);

	dpram_recovery_clear_modem_command();

	memcpy((void *)(dpram->dpram_vbase +
			DPRAM_PDA2PHONE_INTERRUPT_ADDRESS), (void *)&nCmd, 2);
}

static u32
dpram_recovery_check_command(u16 intr, u16 mask)
{
	/* There's no Interrupt from Phone. we have to poll again.*/
	if (intr == 0)
		return CMD_RETRY;

	/* Some command arrived.*/
	else {
		if ((intr & MASK_CMD_VALID) != MASK_CMD_VALID)
			return CMD_RETRY;

		else if ((intr & MASK_PDA_CMD) == MASK_PDA_CMD)
			return CMD_RETRY;

		else if ((intr & MASK_PHONE_CMD) == MASK_PHONE_CMD) {
			if ((intr & mask) == mask)
				return CMD_TRUE;

			else
				return CMD_FALSE;

		} else
			return CMD_RETRY;

	}
}

static int
dpram_recovery_wait_response(u16 cmd_mask)
{
	u16 intr;
	int nRetry = 0;
	int ret = 0;

	while (1) {
		if (nRetry > 100)
			return FALSE;

		intr = dpram_recovery_ReadCommand();
		printk(KERN_DEBUG "intr = 0x%x\n", intr);

		ret = dpram_recovery_check_command(intr, cmd_mask);
		if (ret == CMD_TRUE) {
			printk(KERN_DEBUG "READY ok\n");
			return TRUE;
		} else if (ret == CMD_FALSE) {
			printk(KERN_DEBUG "READY failed.\n");
			return FALSE;
		}

		nRetry++;
		if (fw.image_type == DPRAM_MODEM_DELTA_IMAGE)
			msleep(1000);
		else
			msleep(25);

	}

	return TRUE;
}

static int
dpram_recovery_WaitReadyPhase(void)
{
	int retval = TRUE;

	/* Send Delta Image Receive Ready Request */
	printk(KERN_ERR "Writing command for modem\n");
	dpram_recovery_WriteCommand(CMD_FOTA_IMG_RECEIVE_READY_REQ);

	/* Wait Delta Image Receive Ready Response */
	printk(KERN_ERR "Waiting for the response from Modem\n");
	retval = dpram_recovery_wait_response(
				MASK_CMD_FOTA_IMG_RECEIVE_READY_RESP);

	return retval;
}

static int
dpram_recovery_WaitReadyPhase_download(void)
{
	int retval = TRUE;

	/* Send Delta Image Receive Ready Request */
	printk(KERN_ERR "Writing command for modem\n");
	dpram_recovery_WriteCommand(CMD_FOTA_IMG_SEND_REQ);

	/* Wait Delta Image Receive Ready Response */
	printk(KERN_ERR "Waiting for the response from Modem\n");
	retval = dpram_recovery_wait_response(
		MASK_CMD_FOTA_SEND_DONE_RESP | MASK_CMD_RESULT_SUCCESS);

	return retval;
}

static int
dpram_recovery_write_modem_firmware(
		struct dpram_dev *dev, char __user *firmware, int size)
{
	int ret = FALSE;

	/* Start Wait Ready Phase */
	if (dpram_recovery_WaitReadyPhase() == FALSE) {
		printk(KERN_ERR "error[-2] in return %s\n", __func__);
		ret = -2;
		goto Exit;
	}

	/* Downlaod start Req ~ Update Status Noti */
	ret = dpram_recovery_DownloadControl((u8 *)firmware, size);
	if (ret < 0) {
		printk(KERN_DEBUG "[DPRAM_Download]Failed in DwnldCtrll\n.");
		goto Exit;
	}
	printk(KERN_DEBUG "[DPRAM_Download]FileSize : %d\n", size);

	if (!dpram_recovery_UpdateRequestPhase()) {
		printk(KERN_DEBUG "  Wait FOTA Update Start Response Failed!!\n");
		ret = -7; /* -7 means that FOTA Update Start Response failed. */
		goto Exit;
	}

	ret = TRUE;

Exit:
	return ret;
}

static int
dpram_recovery_write_modem_full_firmware(struct dpram_dev *dev,
					 char __user *firmware, int size)
{
	int ret = FALSE;

	/* Start Wait Ready Phase */
	if (dpram_recovery_wait_response(\
		MASK_CMD_FOTA_IMG_RECEIVE_READY_RESP) == FALSE) {
		printk(KERN_ERR "error[-2] in return %s\n", __func__);
		ret = -2;
		goto Exit;
	}

	/* Clear Response */
	dpram_recovery_clear_modem_command();

	/* Downlaod start Req ~ Update Status Noti */
	ret = dpram_recovery_DownloadControl_fullfirmware(
		(u8 *)firmware, size);
	if (ret < 0) {
		printk(KERN_DEBUG "[DPRAM_Download]Failed in "
			"DownloadControl\n.");
		goto Exit;
	}

	printk(KERN_DEBUG "[DPRAM_Download]FileSize : %d\n", size);
	printk(KERN_DEBUG "Bootloader Image Download Completed!\n");

	ret = TRUE;

Exit:
	return ret;
}

static u16
dpram_recovery_MakeCRC(u16 length, u16 *data_ptr_WORD)
{
	u16  data_crc = CRC_16_L_SEED;
	u16  const *table = CRC_Table;
	u8 *data_ptr = (u8 *)data_ptr_WORD;
	int i;

	for (i = 0; i < length; i++) {
		data_crc = (((data_crc) >> 8) ^ table[((data_crc) ^
					(u16)(data_ptr[i])) & 0x00ff]);
	 /*printk(KERN_DEBUG "%010d:0x%04x\r\n", i, data_crc ); */
	}
	return data_crc;
}

static u32
dpram_recovery_WriteImage(u8 *const pBuf, u32 dwWriteLen)
{
	u8 *pDest;
	u8 *pDest_Data;
	u16 Len;
	u16 nCrc;


	pDest = (u8 *)(dpram->dpram_vbase +
				DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS);
	Len   = (u16)min((u32)dwWriteLen, (u32)FODN_DEFAULT_WRITE_LEN);


	/* Start Index */
	*pDest++ = LOBYTE(START_INDEX);
	*pDest++ = HIBYTE(START_INDEX);

	/*Total Frame number: */
	*pDest++ = LOBYTE(g_TotFrame);
	*pDest++ = HIBYTE(g_TotFrame);

	/*Current Frame number; */
	*pDest++ = LOBYTE(g_CurFrame);
	*pDest++ = HIBYTE(g_CurFrame);
	g_CurFrame++;

	/* Length - Does it include the length of START_INDEX?? */
	*pDest++ = LOBYTE(Len);
	*pDest++ = HIBYTE(Len);

	/* Data */
	pDest_Data = pDest;
	memcpy((void *)pDest, (void *)pBuf, Len);
	pDest += Len;

	/* Fill null if data length is odd */
	if (Len%2 != 0) {
		*pDest++ = 0xff;
		printk(KERN_DEBUG "odd  0x%08x  0x%02x\n",
				(unsigned int)(pDest-1), (u8)(*pDest-1));
	}


	if (Len < FODN_DEFAULT_WRITE_LEN) {
		memset((void *)pDest, 0x0 /*0xff*/,\
			FODN_DEFAULT_WRITE_LEN - Len);
		pDest += (FODN_DEFAULT_WRITE_LEN - Len) ;
	}
	printk(KERN_DEBUG "CRC start 0x%08x\n", (unsigned int)pDest);

	nCrc = dpram_recovery_MakeCRC(Len, (u16 *)pDest_Data);

	*pDest++ = LOBYTE(nCrc);
	*pDest++ = HIBYTE(nCrc);

	printk(KERN_DEBUG "CRC value 0x%04x\n", nCrc);

	*pDest++ = LOBYTE(END_INDEX);
	*pDest++ = HIBYTE(END_INDEX);

	/* Write Command*/
	dpram_recovery_WriteCommand(CMD_FOTA_IMG_SEND_REQ);

	return Len;
}

static u32
dpram_recovery_WriteImage_download(u8 *const pBuf, u32 dwWriteLen)
{
	u8 *pDest;
	u8 *pDest_Data;
	u16 Len;
	u16 nCrc;

	pDest = (u8 *)(dpram->dpram_vbase +
				DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS);
	Len   = (u16)min((u32)dwWriteLen, (u32)FODN_DEFAULT_WRITE_LEN);


	/* Start Index */
	*pDest++ = LOBYTE(START_INDEX);
	*pDest++ = HIBYTE(START_INDEX);

	/*Total Frame number: */
	*pDest++ = LOBYTE(g_TotFrame);
	*pDest++ = HIBYTE(g_TotFrame);

	/*Current Frame number; */
	*pDest++ = LOBYTE(g_CurFrame);
	*pDest++ = HIBYTE(g_CurFrame);
	g_CurFrame++;

	/* Length - Does it include the length of START_INDEX?? */
	*pDest++ = LOBYTE(Len);
	*pDest++ = HIBYTE(Len);

	/* Data */
	pDest_Data = pDest;
	memcpy((void *)pDest, (void *)pBuf, Len);
	pDest += Len;

	/* Fill null if data length is odd */
	if (Len%2 != 0) {
		*pDest++ = 0xff;
		printk(KERN_DEBUG "odd  0x%08x  0x%02x\n",
			(unsigned int)(pDest-1), (u8)(*pDest-1));
	}

	/*printk(KERN_DEBUG "len:%d default len:%d\n",
	Len, FODN_DEFAULT_WRITE_LEN);*/

	if (Len < FODN_DEFAULT_WRITE_LEN) {
		memset((void *)pDest, 0x0 /*0xff*/,
			FODN_DEFAULT_WRITE_LEN - Len);
		pDest += (FODN_DEFAULT_WRITE_LEN - Len) ;
	}
	printk(KERN_DEBUG "CRC start 0x%08x\n", (unsigned int)pDest);

	nCrc = dpram_recovery_MakeCRC(Len, (u16 *)pDest_Data);

	*pDest++ = LOBYTE(nCrc);
	*pDest++ = HIBYTE(nCrc);

	printk(KERN_DEBUG "CRC value 0x%04x\n", nCrc);

	pDest = (u8 *)(dpram->dpram_vbase + BSP_DPRAM_BASE_SIZE - 2*\
		DPRAM_INTERRUPT_SIZE - DPRAM_INDEX_SIZE);

	*pDest++ = LOBYTE(END_INDEX);
	*pDest++ = HIBYTE(END_INDEX);

	/* Write Command*/
	dpram_recovery_WriteCommand(CMD_FOTA_UPDATE_START_REQ);

	return Len;
}


static u16
dpram_recovery_CalcTotFrame(u32 nDividend, u16 nDivisor)
{
	u16 nCompVal1 = 0;
	u16 nCompVal2 = 0;

	printk(KERN_DEBUG "[CalcTotFrame](%d)  %d %d\n", __LINE__,
		nDividend, nDivisor);
	nCompVal1 = (u16) (nDividend / nDivisor);
	nCompVal2 = (u16) (nDividend  - (nCompVal1 * nDivisor));
	if (nCompVal2 > 0) {
		printk(KERN_DEBUG "[CalcTotFrame](%d) val2 : %d\n",
			__LINE__, nCompVal2);
		nCompVal1++;
	}
	printk(KERN_DEBUG "[CalcTotFrame](%d) result %d\n",
		__LINE__, nCompVal1);
	return nCompVal1;
}

static void
dpram_recovery_WriteDoneRequest(void)
{
	printk(KERN_DEBUG "[DPRAM_WriteDoneRequest] Start\n");

	SeqIdx = 1;

	dpram_recovery_WriteCommand(CMD_FOTA_SEND_DONE_REQ);
	printk(KERN_DEBUG "[DPRAM_WriteDoneRequest] End\n");
}

static int
dpram_recovery_SendDonePhase(void)
{
	int retval = TRUE;

	/* Send Write Done Request */
	dpram_recovery_WriteDoneRequest();

	/* Wait Write Done Response */
	retval = dpram_recovery_wait_response(MASK_CMD_FOTA_SEND_DONE_RESP);
	if (retval == FALSE) {
		printk(KERN_DEBUG " Wait Write Done Response Failed!!\n");
		return retval;
	}

	printk(KERN_DEBUG "Wait 0.5 secs.. .. ..\n");
	msleep(500);

	return retval;
}

static int
dpram_recovery_UpdateRequestPhase(void)
{
	int retval = TRUE;

	/* Send FOTA Update Start Request */
	dpram_recovery_WriteCommand(CMD_FOTA_UPDATE_START_REQ);
	/* Wait FOTA Update Start Response */
	/*retval = dpram_recovery_wait_response
	(MASK_CMD_FOTA_UPDATE_START_RESP); */

	return retval;
}

static u32
dpram_recovery_DownloadControl(u8 *pBufIn, u32 Len)
{
	u32 dwWriteLen = 0, dwWrittenLen = 0, dwTotWrittenLen = 0;
	u32 dwRet = 0;
	u16 nTotalFrame = 0, nIntrValue = 0;
	int nwRetry = 0, nrRetry = 0, retval;

	nTotalFrame = dpram_recovery_CalcTotFrame(Len, DELTA_PACKET_SIZE);
	g_TotFrame =  nTotalFrame;
	printk(KERN_DEBUG "  total frame:%d,%d\n",
		g_TotFrame, nTotalFrame);

	while (dwTotWrittenLen < Len) {
		/*Write proper size of image to DPRAM*/
		printk(KERN_DEBUG "DPRAM_WriteImage %d/%d start\n",
				g_CurFrame, g_TotFrame);
		dwWriteLen = min((u32)(Len - dwTotWrittenLen),
						 (u32)DELTA_PACKET_SIZE);
		dwWrittenLen = dpram_recovery_WriteImage(pBufIn, dwWriteLen);
		printk(KERN_DEBUG "Written data:%d\n", dwWrittenLen);

		if (dwWrittenLen > 0) {
			dwTotWrittenLen += dwWrittenLen;
			pBufIn += dwWrittenLen;
		} else {
			printk(KERN_DEBUG "Write Image Len is wierd.\n");
		if (nwRetry < 3) {
			nwRetry++;
			continue;
		} else {
			printk(KERN_DEBUG "Fail to Write Image to DPRAM.\n");
			dwRet = -3;
			goto Exit;
		}
		}

		/* Wait for the IMAGE WRITE RESPONSE */
		while (nrRetry < 10) {
			retval = dpram_recovery_wait_response
						(MASK_CMD_FOTA_IMG_SEND_RESP);
		if (retval == FALSE) {
			printk(KERN_DEBUG "Wait Image Send Response Failed\n");
			dwRet = -4;
			goto Exit;
		  } else {
			nIntrValue = dpram_recovery_ReadCommand();
			if ((nIntrValue & MASK_CMD_RESULT_FAIL) == 0) {
				printk(KERN_DEBUG "%d /%d ok\n", g_CurFrame - 1,
								g_TotFrame);
				break;
			} else {
				msleep(100);
				nrRetry++;
				dpram_recovery_WriteCommand(\
						CMD_FOTA_IMG_SEND_REQ);
					if (nrRetry >= 10) {
						dwRet = -5;
						goto Exit;
					}
				}
			}
		}
	}

	g_CurFrame = 1;

	if (!dpram_recovery_SendDonePhase()) {
		printk(KERN_DEBUG "  "
			"There's something unexpeted in SendDone Phase.\n");
		dwRet = -6; /* -6 means that SendDone Phase failed. */
		goto Exit;
	}

Exit:
	return dwRet;
}

static u32
dpram_recovery_DownloadControl_fullfirmware(u8 *pBufIn, u32 Len)
{
	u32 dwWriteLen = 0, dwWrittenLen = 0, dwTotWrittenLen = 0;
	u32 dwRet = 0;
	u16 nTotalFrame = 0, nIntrValue = 0;
	int nwRetry = 0, nrRetry = 0, retval;

	/* Send command and wait for response */
	if (dpram_recovery_WaitReadyPhase_download() == FALSE) {
		printk(KERN_ERR "error[-2] in return %s\n", __func__);
		dwRet = -2;
		goto Exit;
	}

	nTotalFrame = dpram_recovery_CalcTotFrame(Len, FODN_DEFAULT_WRITE_LEN);
	g_TotFrame =  nTotalFrame;
	printk(KERN_DEBUG "total frame:%d\n", nTotalFrame);

	while (dwTotWrittenLen < Len) {
		/*Write proper size of image to DPRAM*/
		printk(KERN_DEBUG "DPRAM_WriteImage %d/%d start\n",
				g_CurFrame, g_TotFrame);
		dwWriteLen = min((u32)(Len - dwTotWrittenLen),
			(u32)FODN_DEFAULT_WRITE_LEN);
		dwWrittenLen = dpram_recovery_WriteImage_download(pBufIn,
			dwWriteLen);
		printk(KERN_DEBUG "Written data:%d\n", dwWrittenLen);

		if (dwWrittenLen > 0) {
			dwTotWrittenLen += dwWrittenLen;
			pBufIn += dwWrittenLen;
		} else {
			printk(KERN_DEBUG "Write Image Len is wierd.\n");
			if (nwRetry < 3) {
				nwRetry++;
				printk(KERN_DEBUG "Retry to write. "
					"nRetry = %8x\n", nwRetry);
				continue;
			} else {
				printk(KERN_DEBUG "Fail to Write "
					"Image to DPRAM.\n");
				dwRet = -3;
				goto Exit;
			}
		}

		/* Wait for the IMAGE WRITE RESPONSE */
		while (nrRetry < 10) {
			retval = dpram_recovery_wait_response(
				MASK_CMD_IMAGE_SEND_RESPONSE);
			if (retval == FALSE) {
				dwRet = -4;
				goto Exit;
			} else {
				nIntrValue = dpram_recovery_ReadCommand();
				if ((nIntrValue & MASK_CMD_RESULT_FAIL) == 0) {
					break;
				} else {
					msleep(100);
					nrRetry++;
					dpram_recovery_WriteCommand(
						CMD_FOTA_UPDATE_START_REQ);

					if (nrRetry >= 10) {
						dwRet = -5;
						goto Exit;
					}
				}
			}
		}
	}

	g_CurFrame = 1;

	/* SendDone Phase */
	dpram_recovery_WriteCommand(CMD_DL_SEND_DONE_REQ);
	printk(KERN_DEBUG "[DPRAM_WriteDoneRequest] End\n");

	printk(KERN_DEBUG "Wait 0.5 secs.. .. ..\n");
	msleep(500);


Exit:
	return dwRet;
}

static u32
dpram_recovery_StatusNotiPhase(u32 *pPercent, u8 *UpDoneNotiFlag)
{
	struct Up_Noti UpStatus;
	u16 intr = 0, ret = 0;
	u32 dwRet = TRUE;

	UpStatus.progress_status = 0x00;
	UpStatus.error_code = 0x00;

	*UpDoneNotiFlag = FALSE;

	/*dpram_recovery_WriteCommand(CMD_FOTA_UPDATE_STATUS_IND); */

	/* Wait FOTA Update Status Indication */
	dwRet = dpram_recovery_wait_response(MASK_CMD_FOTA_UPDATE_STATUS_IND);
	if (dwRet == FALSE) {
		printk(KERN_DEBUG "Wait FOTA Update Status "
			"Indication Failed!!\n");
		intr = dpram_recovery_ReadCommand();
		ret = dpram_recovery_check_command(intr,
			MASK_CMD_FOTA_UPDATE_END_IND);
		if (ret == CMD_TRUE) {
			dwRet = TRUE;
			*UpDoneNotiFlag = TRUE;
			printk(KERN_DEBUG "But FOTA Update Done "
				"Indication Received!!\n");
			goto StatusRead;
		} else
			goto Exit;
	}

StatusRead:
	memcpy((void *)&UpStatus, (void *)(dpram->dpram_vbase + \
		DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS), 8);

	printk(KERN_DEBUG "0x%x  0x%x  0x%x  0x%x\n",
		UpStatus.Bop, UpStatus.progress_status,
		UpStatus.error_code, UpStatus.Eop);

	if ((UpStatus.Bop != START_INDEX) || (UpStatus.Eop != END_INDEX)) {
		printk(KERN_DEBUG "Packet format is wrong.\n");
		dwRet = 0;
	} else {
		printk(KERN_DEBUG "Get the Status info.\n");
		*pPercent = (u32)UpStatus.progress_status;
		error_code = (u16)UpStatus.error_code;
	}

	if (error_code != 0) {
		printk(KERN_DEBUG "Update error is occured, "
			"error number:0x%x.\n", error_code);
		dwRet = FALSE;
		goto Exit;
	}

Exit:
	/* Clear modem command for next IOCTL_CHK_STAT */
	dpram_recovery_clear_modem_command();

	return dwRet;
}

static u32
dpram_recovery_StatusNotiPhase_download(u32 *pPercent, u8 *UpDoneNotiFlag)
{
	struct Up_Notification upstatus;
	u16 ret = 0;
	u32 dwRet = TRUE;

	upstatus.Region = 0x0;
	upstatus.Percent = 0x0;

	*UpDoneNotiFlag = FALSE;

	dwRet = dpram_recovery_wait_response(
		MASK_CMD_UPDATE_DONE_NOTIFICATION|MASK_CMD_RESULT_SUCCESS);
	if (dwRet == FALSE) {
		dwRet = dpram_recovery_wait_response(
			MASK_CMD_STATUS_UPDATE_NOTIFICATION |
			MASK_CMD_RESULT_SUCCESS);
		if (dwRet == TRUE) {
			printk(KERN_DEBUG "Received UPDATE Status "
				"Notification Command in %s", __func__);
			goto StatusRead;
		} else {
			ret = FALSE;
			goto Exit;
		}
	} else {
		printk(KERN_DEBUG " Successfully Received UPDATE "
			"DONE Command in %s", __func__);
		*UpDoneNotiFlag = TRUE;
		ret = TRUE;
		goto StatusRead;
	}

StatusRead:
	dpram_recovery_clear_modem_command();
	memcpy((void *)&upstatus, (void *)(dpram->dpram_vbase + \
		DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS), 8);
	if ((upstatus.Bop != START_INDEX) || (upstatus.Eop != END_INDEX)) {
		printk(KERN_DEBUG "CP Update Failed by Bad Header!!\n");
		printk(KERN_DEBUG "0x%x  0x%x\n", upstatus.Bop, upstatus.Eop);
		ret = FALSE;
		goto Exit;
	} else {
		*pPercent = upstatus.Percent;
		printk(KERN_DEBUG "Region= 0x%x Percent = 0x%x\n",
			upstatus.Region, upstatus.Percent);
		ret = TRUE;
	}
Exit:
	/* Clear modem command for next IOCTL_CHK_STAT */
	dpram_recovery_clear_modem_command();
	return ret;

}

static int
dpram_recovery_check_status(struct dpram_dev *dev, int __user *pct)
{
	u32 bUpStatusPer = 0, PrevbUpStatusPer = 0;
	u8 UpDoneNotiFlag = 0;
	u32 dwRet = 1;
	int retval = 0;

	/* Process for UPDATE STATUS Noti. phase. */
	retval = dpram_recovery_StatusNotiPhase(&bUpStatusPer,
		&UpDoneNotiFlag);
	if (retval == FALSE) {
		printk(KERN_DEBUG "Wait Status Noti Phase Failed!!\n");
		dwRet = -8;
	} else {
		printk(KERN_DEBUG "Progress:%d\n", bUpStatusPer);

		if (bUpStatusPer > PrevbUpStatusPer)
			PrevbUpStatusPer = bUpStatusPer;

		*pct = bUpStatusPer;

		if ((bUpStatusPer == 100) && (UpDoneNotiFlag == TRUE)) {
			printk(KERN_DEBUG "Done!!!\n");
			dwRet = 0;
		}
	}

	return dwRet;
}

static int
dpram_recovery_check_status_download(struct dpram_dev *dev, int __user *pct)
{
	u32 bUpStatusPer = 0, PrevbUpStatusPer = 0;
	u8 UpDoneNotiFlag = 0;
	u32 dwRet = 1;
	int retval = 0;

	/* Process for UPDATE STATUS Noti. phase. */
	retval = dpram_recovery_StatusNotiPhase_download(&bUpStatusPer,
		&UpDoneNotiFlag);
	if (retval == FALSE) {
		printk(KERN_DEBUG "Wait Status Noti Phase Failed!!\n");
		dwRet = -8;
	} else {
		printk(KERN_DEBUG "Progress:%d\n", bUpStatusPer);

		if (bUpStatusPer > PrevbUpStatusPer)
			PrevbUpStatusPer = bUpStatusPer;

		*pct = bUpStatusPer;

		if ((bUpStatusPer == 100) && (UpDoneNotiFlag == TRUE)) {
			printk(KERN_DEBUG "Done!!!\n");
			dwRet = 0;
		}
	}

	return dwRet;
}

static ssize_t
dpram_recovery_read(struct file *filp, char __user *buff,
		size_t count, loff_t *fpos)
{
	return 0;
}

static ssize_t
dpram_recovery_write(struct file *filp, const char __user *buff,
		size_t size, loff_t *fpos)
{
	return 0;
}

static long
dpram_recovery_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct dpram_dev *dev;
	int ret = 0, ret_check = 0;
	u32 magic_code = 0x0;

	printk(MSGDBG "%s" MSGEND, __func__);
	printk(KERN_ERR "%s\n", __func__);

	dev = container_of(filp->f_path.dentry->d_inode->i_cdev,
		struct dpram_dev, cdev);

	switch (cmd) {
	case IOCTL_MODEM_GOTA_START:
	case IOCTL_WRITE_MAGIC_CODE:
		printk(KERN_ERR "%s IOCTL_MODEM_GOTA_START\n", __func__);
		dpram_recovery_dpram_Init();
		break;

	case IOCTL_MODEM_BOOT_ON:
		printk(KERN_ERR "%s IOCTL_MODEM_BOOT_ON\n", __func__);
		break;

	case IOCTL_MODEM_FW_UPDATE:
	case IOCTL_ST_FW_UPDATE:
		printk(KERN_ERR "%s IOCTL_ST_FW_UPDATE\n", __func__);

		if (arg == 0) {
			printk(MSGWARN "Firmware should be "
				"written prior to this call" MSGEND);
		} else {
			ret_check = copy_from_user((void *)&fw,
				(void *)arg, sizeof(fw));
				if (ret_check) {
					printk("KERN_ERR [IOCTL_ST_FW"
					"_UPDATE]copy from user failed!");
				return -EFAULT;
			}
		}

		/* Dpram Phone off */
		dpram_recovery_modem_pwroff();
		if (fw.image_type == DPRAM_MODEM_DELTA_IMAGE) {
			dpram_recovery_dpram_Init();
			ret = dpram_recovery_write_modem_firmware(dev,
				fw.firmware, fw.size);
			if (ret < 0)
				printk(KERN_ERR "firmware write failed\n");

		} else {
			/* Enter Download Mode */
			dpram_recovery_download_Init();
			ret = dpram_recovery_write_modem_full_firmware(dev,
				fw.firmware, fw.size);
			if (ret < 0)
				printk(KERN_ERR "firmware write failed\n");
		}
		break;

	case IOCTL_CHK_STAT:
		{
			struct stat_info *pst;

			printk(KERN_ERR "%s IOCTL_CHK_STAT\n", __func__);

			pst = (struct stat_info *)arg;
			if (fw.image_type == DPRAM_MODEM_DELTA_IMAGE)
				ret = dpram_recovery_check_status(dev,
					&(pst->pct));
			else
				ret = dpram_recovery_check_status_download(dev,
					&(pst->pct));
		}
		break;

	case IOCTL_MOD_PWROFF:
	case IOCTL_MODEM_BOOT_OFF:
		printk(KERN_ERR "%s IOCTL_MOD_PWROFF\n", __func__);

		/* Delay for display */
		msleep(2000);

		/* Clear Magic Code */
		dpram_recovery_WriteMagicCode(magic_code);

		dpram_recovery_modem_pwroff();
		break;

	default:
		printk(KERN_ERR "Unknown command = %d\n", cmd);
		break;
	}

	return ret;
}

static int
dpram_recovery_open(struct inode *inode, struct file *filp)
{
	struct dpram_dev *dev;

	printk(KERN_ERR "%s\n", __func__);

	dev = container_of(inode->i_cdev, struct dpram_dev, cdev);
	filp->private_data = (void *)dev;

	return 0;
}

static int
dpram_recovery_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "dpram recovery device released.\n");
	return 0;
}


static const struct file_operations dpram_recovery_fops = {
	.owner = THIS_MODULE,
	.read = dpram_recovery_read,
	.write = dpram_recovery_write,
	.unlocked_ioctl = dpram_recovery_ioctl,
	.open = dpram_recovery_open,
	.release = dpram_recovery_release,
};

static int register_dpram_rec_device(void)
{
	struct device *dpram_rec_dev_t;
	struct class *dpram_rec_class;
	int ret;

	ret = register_chrdev(MAJOR_NUM, DRIVER_NAME, &dpram_recovery_fops);
	if (ret < 0)
		goto err;

	dpram_rec_class = class_create(THIS_MODULE, "rec");
	if (IS_ERR(dpram_rec_class)) {
		ret = -EFAULT;
		goto err1;
	}

	dpram_rec_dev_t = device_create(dpram_rec_class,
		NULL, MKDEV(MAJOR_NUM, 0), NULL, DRIVER_NAME);
	if (IS_ERR(dpram_rec_dev_t)) {
		ret = -EFAULT;
		goto err2;
	}

	return 0;

err2:
	class_destroy(dpram_rec_class);

err1:
	unregister_chrdev(MAJOR_NUM, DRIVER_NAME);

err:
	return ret;
}

/*  Dpram Interrupt register */
static irqreturn_t phone_active_irq_handler(int irq, void *dev_id)
{
	printk(KERN_ERR "Received interrupt for PHONE ACTIVE\n");
	if (gpio_get_value(GPIO_QSC_PHONE_ACTIVE)) {
		printk(KERN_ALERT "%s: Phone Active high and Modem is ON\n",
			__func__);
	} else {
		printk(KERN_ALERT "%s: Phone Active Low and Modem is OFF\n",
			__func__);
	}
	return IRQ_HANDLED;
}

static int register_interrupt_handler(void)
{
	int retval = 0, phone_active_irq;
	phone_active_irq = gpio_to_irq(IRQ_QSC_PHONE_ACTIVE);

	/* phone active interrupt */
	retval = request_irq(phone_active_irq, phone_active_irq_handler,
		IRQF_DISABLED, "QSC_ACTIVE", NULL);
	if (retval)
		printk(KERN_ERR "Phone active interrupt handler failed.\n");

	return 0;
}

/*
 * DPRAM DRIVER INITIALIZE FUNCTIONs
 */
int dpram_init_hw(void)
{
	int rv;
	int phone_active_irq, dpram_wakeup_irq;

	/* 1) Initialize the interrupt pins */
	irq_set_irq_type(IRQ_DPRAM_AP_INT_N, IRQ_TYPE_LEVEL_LOW);
	rv = gpio_request(IRQ_QSC_INT, "gpx1_0");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get gpio GPX1_0\n",
			__func__);
		goto err;
	}
	dpram_wakeup_irq = gpio_to_irq(IRQ_QSC_INT);
	s3c_gpio_cfgpin(GPIO_C210_DPRAM_INT_N, S3C_GPIO_SFN(0xFF));
	s3c_gpio_setpull(GPIO_C210_DPRAM_INT_N, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(dpram_wakeup_irq, IRQ_TYPE_EDGE_RISING);

	rv = gpio_request(IRQ_QSC_PHONE_ACTIVE, "gpx1_6");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get gpio GPX1_6\n",
			__func__);
		goto err1;
	}

	phone_active_irq = gpio_to_irq(IRQ_QSC_PHONE_ACTIVE);

	s3c_gpio_setpull(GPIO_QSC_PHONE_ACTIVE, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_QSC_PHONE_RST, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(phone_active_irq, IRQ_TYPE_EDGE_FALLING);
	/* 4)gpio e3-e4 are for Modem if */

	s3c_gpio_cfgpin(S5PV310_GPE3_0_MDM_DATA_0,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_1_MDM_DATA_1,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_2_MDM_DATA_2,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_3_MDM_DATA_3,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_4_MDM_DATA_4,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_5_MDM_DATA_5,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_6_MDM_DATA_6,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE3_7_MDM_DATA_7,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	s3c_gpio_cfgpin(S5PV310_GPE4_0_MDM_DATA_8,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_1_MDM_DATA_9,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_2_MDM_DATA_10,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_3_MDM_DATA_11,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_4_MDM_DATA_12,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_5_MDM_DATA_13,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_6_MDM_DATA_14,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE4_7_MDM_DATA_15,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	s3c_gpio_cfgpin(S5PV310_GPE0_0_MDM_WEn,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_1_MDM_CSn,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_2_MDM_Rn,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_3_MDM_IRQn,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));
	s3c_gpio_cfgpin(S5PV310_GPE0_4_MDM_ADVN,
		S3C_GPIO_SFN(S5PV310_MDM_IF_SEL));

	rv = gpio_request(GPIO_PDA_ACTIVE, "GPY4_2");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get gpio GPY4_2\n",
			__func__);
		goto err2;
	}

	rv = gpio_request(GPIO_QSC_PHONE_ON, "GPC1_1");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get gpio GPC1_1\n",
			__func__);
		goto err3;
	}

	rv = gpio_request(GPIO_QSC_PHONE_RST, "GPX1_4");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get "
			"gpio GPX1_4\n", __func__);
		goto err4;
	}

	/*To increase driving strength of this pin */
	s5p_gpio_set_drvstr(GPIO_QSC_PHONE_RST, S5P_GPIO_DRVSTR_LV4);

	/*To config Not Connected pin GPY4_6 to I/P with no pullup */
	rv = gpio_request(GPIO_CP_REQ_RESET, "GPY4_6");
	if (rv < 0) {
		printk(KERN_ERR "IDPRAM: [%s] failed to get "
			"gpio GPY4_6\n", __func__);
		goto err5;
	}

	gpio_direction_input(GPIO_CP_REQ_RESET);
	s3c_gpio_setpull(GPIO_CP_REQ_RESET, S3C_GPIO_PULL_NONE);

	/* 5)PDA_ACTIVE, QSC_PHONE_ON, QSC_RESET_N */
	gpio_direction_output(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);
	gpio_direction_output(GPIO_QSC_PHONE_ON, GPIO_LEVEL_HIGH);
	s3c_gpio_setpull(GPIO_QSC_PHONE_ON, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_QSC_PHONE_RST, GPIO_LEVEL_LOW);

	return 0;

err5:	gpio_free(GPIO_QSC_PHONE_RST);
err4:	gpio_free(GPIO_QSC_PHONE_ON);
err3:	gpio_free(GPIO_PDA_ACTIVE);
err2:	gpio_free(IRQ_QSC_PHONE_ACTIVE);
err1:	gpio_free(IRQ_QSC_INT);
err:	return rv;
}

static int __devinit dpram_recovery_probe(struct platform_device *pdev)
{
	int err = 0;
	g_TotFrame = 0;

	printk(KERN_ERR "[DPRAM_RECOVERY] %s() ...\n", __func__);

	dpram = kmalloc(sizeof(struct dpram_dev), GFP_KERNEL);
	if (dpram == NULL) {
		printk(KERN_ERR "dpram recovery device allocation failed!!\n");
		err = -ENOMEM;
		goto out;
	}

	if (dpram_recovery_ioremap(dpram) < 0) {
		printk(KERN_ERR "Dpram recovery ioremap failed!\n");
		err = -EFAULT;
		goto out_err1;
	}

	err = register_dpram_rec_device();
	if (err) {
		printk(KERN_ERR "Failed to register dpram recovery device.\n");
		goto out_err2;
	}

	err = dpram_init_hw();
	if (err < 0) {
		printk(KERN_ERR "dpram_init_hw() failed\n");
		goto out_err2;
	}

	err = register_interrupt_handler();
	if (err < 0) {
		printk(KERN_ERR "register_interrupt_handler() failed!!!\n");
		goto out_err3;
	}

	printk(KERN_ERR "%s %d\n", __func__, __LINE__);

	return 0;

out_err3:
	gpio_free(IRQ_QSC_PHONE_ACTIVE);

out_err2:
	iounmap(dpram);

out_err1:
	kfree(dpram);

out:
	return err;
}


static int dpram_recovery_suspend(struct device *pdev)
{
	return 0;
}

static int dpram_recovery_resume(struct device *pdev)
{
	return 0;
}

static const struct dev_pm_ops dpram_recovery_pm_ops = {
	.suspend    = dpram_recovery_suspend,
	.resume     = dpram_recovery_resume,
};

static struct platform_driver dpram_recovery_driver = {
	.probe = dpram_recovery_probe,
	.driver = {
		.name = "dpram-recovery",
		.pm   = &dpram_recovery_pm_ops,
	},
};

static int __init dpram_recovery_init(void)
{
	return platform_driver_register(&dpram_recovery_driver);
}

module_init(dpram_recovery_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DPRAM Device Driver for recovery");
