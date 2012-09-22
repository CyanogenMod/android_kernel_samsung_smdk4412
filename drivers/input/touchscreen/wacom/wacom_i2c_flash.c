/*
 *  wacom_i2c_flash.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/wacom_i2c.h>
#include "wacom_i2c_flash.h"

#define ERR_HEX 0x056
#define RETRY_TRANSFER 5
unsigned char checksum_data[4];

int calc_checksum(unsigned char *flash_data)
{
	unsigned long i;

	if (ums_binary)
		return 0;

	for (i = 0; i < 4; i++)
		checksum_data[i] = 0;

	for (i = 0x0000; i < 0xC000; i += 4) {
		checksum_data[0] ^= flash_data[i];
		checksum_data[1] ^= flash_data[i+1];
		checksum_data[2] ^= flash_data[i+2];
		checksum_data[3] ^= flash_data[i+3];
	}

	printk(KERN_DEBUG
		"[E-PEN] %s : %02x, %02x, %02x, %02x\n",
		__func__, checksum_data[0], checksum_data[1],
		checksum_data[2], checksum_data[3]);

	for (i = 0; i < 4; i++) {
		if (checksum_data[i] != Firmware_checksum[i+1])
			return -ERR_HEX;
	}

	return 0;
}

int wacom_i2c_flash_chksum(struct wacom_i2c *wac_i2c, unsigned char *flash_data,
			   unsigned long *max_address)
{
	unsigned long i;
	unsigned long chksum = 0;

	for (i = 0x0000; i <= *max_address; i++)
		chksum += flash_data[i];

	chksum &= 0xFFFF;

	return (int)chksum;
}

int wacom_i2c_flash_cmd(struct wacom_i2c *wac_i2c)
{

	int ret, len, i;
	u8 buf[10], flashq;

	buf[0] = 0x0d;
	buf[1] = FLASH_START0;
	buf[2] = FLASH_START1;
	buf[3] = FLASH_START2;
	buf[4] = FLASH_START3;
	buf[5] = FLASH_START4;
	buf[6] = FLASH_START5;
	buf[7] = 0x0d;
	flashq = 0xE0;
	len = 1;

	ret = wacom_i2c_send(wac_i2c, &flashq, len, true);
	if (ret >= 0) {

		i = 0;
		do {
			msleep(1);
			ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
			i++;

			if (i > RETRY)
				return -1;
		} while (flashq == 0xff);
	} else {
		msleep(1);
		len = 8;
		ret = wacom_i2c_send(wac_i2c, buf, len, false);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN]: Sending flash command failed\n");
			return -1;
		}
		printk(KERN_DEBUG "[E-PEN]: flash send?:%d\n", ret);
		msleep(270);
	}

	wac_i2c->boot_mode = true;

	return 0;
}

int wacom_i2c_flash_query(struct wacom_i2c *wac_i2c, u8 query, u8 recvdQuery)
{
	int ret, len, i;
	u8 flashq;

	flashq = query;
	len = 1;

	ret = wacom_i2c_send(wac_i2c, &flashq, len, true);
	if (ret < 0) {
		printk(KERN_ERR "[E-PEN]: query unsent:%d\n", ret);
		return -1;
	}

	/*sleep*/
	msleep(10);
	i = 0;
	do {
		msleep(1);
		flashq = query;
		ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
		i++;

		if (i > RETRY)
			return -1;
		printk(KERN_DEBUG "[E-PEN]: ret:%d flashq:%x\n", ret, flashq);
	} while (recvdQuery == 0xff && flashq != recvdQuery);
	printk(KERN_DEBUG "[E-PEN]: query:%x\n", flashq);

	return flashq;
}

int wacom_i2c_flash_end(struct wacom_i2c *wac_i2c)
{
	int ret;

	/* 2012/07/04 Evaluation for 0x80 and 0xA0 added by Wacom*/
	do {
		ret = wacom_i2c_flash_query(wac_i2c, FLASH_END, FLASH_END);
		if (ret == -1)
			return ERR_FAILED_EXIT;
	} while (ret == 0xA0 || ret != 0x80);
	/* 2012/07/04 Evaluation for 0x80 and 0xA0 added by Wacom*/

	/*2012/07/05
	below added for giving firmware enough time to change to user mode*/
	msleep(1000);

	printk(KERN_DEBUG "[E-PEN] Digitizer activated\n");
	wac_i2c->boot_mode = false;
	return 0;
}

int wacom_i2c_flash_enter(struct wacom_i2c *wac_i2c)
{
	if (wacom_i2c_flash_query(wac_i2c, FLASH_QUERY, FLASH_ACK) == -1)
		return ERR_NOT_FLASH;
	return 0;
}

int wacom_i2c_flash_BLVer(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	ret = wacom_i2c_flash_query(wac_i2c, FLASH_BLVER, 0x40);
	if (ret == -1)
		return ERR_UNSENT;

	return ret;
}

int wacom_i2c_flash_mcuId(struct wacom_i2c *wac_i2c)
{
	int ret = 0;

	ret = wacom_i2c_flash_query(wac_i2c, FLASH_MPU, 0x26);
	if (ret == -1)
		return ERR_UNSENT;

	return ret;
}

int wacom_i2c_flash_erase(struct wacom_i2c *wac_i2c, u8 cmd_erase,
			  u8 cmd_block, u8 endBlock)
{
	int len, ret, i, j;
	u8 buf[3], sum, block, flashq;
	unsigned long swtich_slot_time;

	ret = 0;

	for (i = cmd_block; i >= endBlock; i--) {
		block = i;
		block |= 0x80;

		sum = cmd_erase;
		sum += block;
		sum = ~sum + 1;

		buf[0] = cmd_erase;
		buf[1] = block;
		buf[2] = sum;

		len = 3;
		ret = wacom_i2c_send(wac_i2c, buf, len, true);
		if (ret < 0) {
			printk(KERN_ERR "[E-PEN]: Erase failed\n");
			return -1;
		}

		len = 1;
		flashq = 0;
		j = 0;

		do {
			/*sleep */
			msleep(100);
			ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
			j++;

			if (j > RETRY || flashq == 0x84 || flashq == 0x88
			    || flashq == 0x8A || flashq == 0x90) {
				/*
				   0xff:No data
				   0x84:Erase failure
				   0x88:Erase time parameter error
				   0x8A:Write time parameter error
				   0x90:Checksum error
				 */
				printk(KERN_ERR "[E-PEN]: Error:%x\n", flashq);
				return -1;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (flashq == 0xff || flashq != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			printk(KERN_DEBUG "[E-PEN]: Erasing at %d, ", i);
		/*sleep */
		msleep(1);
	}
	printk(KERN_DEBUG "[E-PEN] Erasing done\n");
	return ret;
}

int wacom_i2c_flash_write(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			  u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j, k;
	char sum;
	u8 buf[WRITE_BUFF], bank;
	unsigned long swtich_slot_time;

	ret = len = i = 0;
	bank = BANK;

	for (ulAddr = startAddr; ulAddr <= maxAddr; ulAddr += BLOCK_SIZE_W) {

		sum = 0;
		buf[0] = FLASH_WRITE;
		buf[1] = (u8) (ulAddr & 0xff);
		buf[2] = (u8) ((ulAddr & 0xff00) >> 8);
		buf[3] = size;
		buf[4] = bank;

		for (i = 0; i < 5; i++)
			sum += buf[i];

		sum = ~sum + 1;
		buf[5] = sum;

		len = 6;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				printk(KERN_ERR "[E-PEN]: Write process aborted\n");
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		msleep(10);
		len = 1;
		j = 0;
		do {
			msleep(5);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				printk(KERN_ERR "[E-PEN] Error: %x , 0x%lx(%d)\n",
					buf[0], ulAddr, __LINE__);
				return -1;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		msleep(1);

		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				printk(KERN_ERR "[E-PEN]: Write process aborted\n");
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		msleep(50);
		len = 1;
		j = 0;

		do {
			msleep(10);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x82 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				printk(KERN_ERR "[E-PEN] Error: %x , 0x%lx(%d)\n",
					buf[0], ulAddr, __LINE__);
				return -1;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			printk(KERN_DEBUG "[E-PEN]: Written on:0x%lx", ulAddr);
		msleep(1);
	}
	printk(KERN_DEBUG "\nWriting done\n");

	return 0;
}

int wacom_i2c_flash_verify(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			   u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j, k;
	char sum;
	u8 buf[WRITE_BUFF], bank;
	unsigned long swtich_slot_time;

	ret = len = i = 0;
	bank = BANK;

	for (ulAddr = startAddr; ulAddr <= maxAddr; ulAddr += BLOCK_SIZE_W) {

		sum = 0;
		buf[0] = FLASH_VERIFY;
		buf[1] = (u8) (ulAddr & 0xff);
		buf[2] = (u8) ((ulAddr & 0xff00) >> 8);
		buf[3] = size;
		buf[4] = bank;

		for (i = 0; i < 5; i++)
			sum += buf[i];
		sum = ~sum + 1;
		buf[5] = sum;

		len = 6;
		j = 0;
		/*sleep */

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				printk(KERN_ERR "[E-PEN]: Write process aborted\n");
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		len = 1;

		do {
			msleep(1);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;
			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				printk(KERN_ERR "[E-PEN] Error: %x , 0x%lx(%d)\n",
					buf[0], ulAddr, __LINE__);
				return -1;
			}
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		msleep(1);
		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				printk(KERN_ERR "[E-PEN]: Write process aborted\n");
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		len = 1;
		j = 0;
		do {
			msleep(2);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x81 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				printk(KERN_ERR "[E-PEN] Error: %x , 0x%lx(%d)\n",
					buf[0], ulAddr, __LINE__);
				return -1;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			printk(KERN_DEBUG "[E-PEN]: Verified:0x%lx", ulAddr);
		msleep(1);
	}

#if defined(CONFIG_MACH_P4NOTE)
	if (calc_checksum(Binary)) {
		printk(KERN_DEBUG
			"[E-PEN] check sum not matched\n");
		return -ERR_HEX;
	}
#endif

	printk("\n[E-PEN]: Verifying done\n");

	return 0;
}

int wacom_i2c_flash(struct wacom_i2c *wac_i2c)
{
	bool fw_update_enable = false;
	int ret = 0, blver = 0, mcu = 0;
	u32 max_addr = 0, cmd_addr = 0;

	if (Binary == NULL) {
		printk(KERN_ERR"[E-PEN] Data is NULL. Exit.\n");
		return -1;
	}

#if defined(CONFIG_MACH_P4NOTE)
	if (calc_checksum(Binary)) {
		printk(KERN_DEBUG
			"[E-PEN] check sum not matched\n");
		return -ERR_HEX;
	}
#endif

	wake_lock(&wac_i2c->wakelock);

	ret = wacom_i2c_flash_cmd(wac_i2c);
	if (ret < 0)
		goto fw_update_error;
	msleep(10);

	ret = wacom_i2c_flash_enter(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: flashEnter:%d\n", ret);
	msleep(10);

	blver = wacom_i2c_flash_BLVer(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: blver:%d\n", blver);
	msleep(10);

	mcu = wacom_i2c_flash_mcuId(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: mcu:%x\n", mcu);
	if (Mpu_type != mcu) {
		printk(KERN_DEBUG "[E-PEN]: mcu:%x\n", mcu);
		ret = -ENXIO;
		goto mcu_type_error;
	}
	msleep(1);

	switch (mcu) {
	case MPUVER_W8501:
		printk(KERN_DEBUG "[E-PEN]: flashing for w8501 started\n");
		max_addr = MAX_ADDR_W8501;
		cmd_addr = MAX_BLOCK_W8501;
		fw_update_enable = true;
		break;

	case MPUVER_514:
		printk(KERN_DEBUG "[E-PEN]: Flashing for 514 started\n");
		max_addr = MAX_ADDR_514;
		cmd_addr = MAX_BLOCK_514;
		fw_update_enable = true;
		break;

	case MPUVER_505:
		printk(KERN_DEBUG "[E-PEN]: Flashing for 505 started\n");
		max_addr = MAX_ADDR_505;
		cmd_addr = MAX_BLOCK_505;
		fw_update_enable = true;
		break;

	default:
		printk(KERN_DEBUG "[E-PEN]: default called\n");
		break;
	}

	if (fw_update_enable) {
		bool valid_hex = false;
		int cnt = 0;
		/*2012/07/04: below modified by Wacom*/
		/*If wacom_i2c_flash_verify returns -ERR_HEX, */
		/*please redo whole process of flashing from  */
		/*wacom_i2c_flash_erase                       */
		do {
			ret = wacom_i2c_flash_erase(wac_i2c, FLASH_ERASE,
				    cmd_addr, END_BLOCK);
			if (ret < 0) {
				printk(KERN_ERR "[E-PEN] error - erase\n");
				continue;
			}
			msleep(20);

			ret = wacom_i2c_flash_write(wac_i2c, START_ADDR,
				    NUM_BLOCK_2WRITE, max_addr);
			if (ret < 0) {
				printk(KERN_ERR "[E-PEN] error - writing\n");
				continue;
			}
			msleep(20);

			ret = wacom_i2c_flash_verify(wac_i2c, START_ADDR,
				     NUM_BLOCK_2WRITE, max_addr);

			if (ret == -ERR_HEX)
				printk(KERN_DEBUG "[E-PEN]: firmware is not valied\n");
			else if (ret < 0) {
				printk(KERN_ERR "[E-PEN] error - verifying\n");
				continue;
			} else
				valid_hex = true;
		} while ((!valid_hex) && (cnt < 10));
		/*2012/07/04: Wacom*/

		printk(KERN_DEBUG "[E-PEN]: Firmware successfully updated\n");
	}
	msleep(1);

mcu_type_error:
	wacom_i2c_flash_end(wac_i2c);
	if (ret < 0)
		printk(KERN_ERR "[E-PEN] error - wacom_i2c_flash_end\n");

fw_update_error:
	wake_unlock(&wac_i2c->wakelock);
	return ret;
}

