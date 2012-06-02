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

	ret = wacom_i2c_master_send(wac_i2c->client, &flashq, len,
				    WACOM_I2C_BOOT);
	if (ret >= 0) {

		i = 0;
		do {
			msleep(20);
			ret = wacom_i2c_master_recv(wac_i2c->client, &flashq,
						    len, WACOM_I2C_BOOT);
			i++;

			if (i > RETRY)
				return -1;
		} while (flashq == 0xff);
	} else {
		msleep(20);
		len = 8;
		ret = i2c_master_send(wac_i2c->client, buf, len);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN]: Sending flash command failed\n");
			return -1;
		}
		printk(KERN_DEBUG "[E-PEN]: flash send?:%d\n", ret);
		msleep(270);
	}

	return 0;
}

int wacom_i2c_flash_query(struct wacom_i2c *wac_i2c, u8 query, u8 recvdQuery)
{
	int ret, len, i;
	u8 flashq;

	flashq = query;
	len = 1;

	ret = wacom_i2c_master_send(wac_i2c->client, &flashq, len,
				    WACOM_I2C_BOOT);
	if (ret < 0) {
		printk(KERN_ERR "[E-PEN]: query unsent:%d\n", ret);
		return -1;
	}

	/*sleep */
	msleep(20);
	i = 0;
	do {
		msleep(20);
		flashq = query;
		ret = wacom_i2c_master_recv(wac_i2c->client, &flashq, len,
					    WACOM_I2C_BOOT);
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
	if (wacom_i2c_flash_query(wac_i2c, FLASH_END, FLASH_END) == -1)
		return ERR_FAILED_EXIT;
	printk(KERN_DEBUG "[E-PEN]: Digitizer activated\n");
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
		ret = wacom_i2c_master_send(wac_i2c->client, buf, len,
					    WACOM_I2C_BOOT);
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
			ret = wacom_i2c_master_recv(wac_i2c->client, &flashq,
						    len, WACOM_I2C_BOOT);
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
		} while (flashq == 0xff);
		printk(KERN_DEBUG "[E-PEN]: Erasing at %d, ", i);
		/*sleep */
		msleep(20);
	}
	printk(KERN_DEBUG "Erasing done\n");
	return ret;
}

int wacom_i2c_flash_write(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			  u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j;
	char sum;
	u8 buf[WRITE_BUFF], bank;

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

		ret = wacom_i2c_master_send(wac_i2c->client, buf, len,
					    WACOM_I2C_BOOT);
		if (ret < 0) {
			printk(KERN_ERR "[E-PEN]: Write process aborted\n");
			return ERR_FAILED_ENTER;
		}

		msleep(20);
		len = 1;
		j = 0;
		do {
			ret = wacom_i2c_master_recv(wac_i2c->client, buf, len,
						    WACOM_I2C_BOOT);
			j++;

			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				printk(KERN_ERR "[E-PEN]: Error:%x\n", buf[0]);
				return -1;
			}
			msleep(20);
		} while (buf[0] == 0xff);

		msleep(20);
		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		ret = wacom_i2c_master_send(wac_i2c->client, buf, len,
					    WACOM_I2C_BOOT);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN]: Firmware flash aborted while writing\n");
			return ERR_NOT_FLASH;
		}

		msleep(100);
		len = 1;
		j = 0;

		do {
			msleep(20);
			ret = wacom_i2c_master_recv(wac_i2c->client, buf, len,
						    WACOM_I2C_BOOT);
			j++;

			if (j > RETRY || buf[0] == 0x82 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				printk(KERN_ERR "[E-PEN]: Error:%x\n", buf[0]);
				return -1;
			}
		} while (buf[0] == 0xff);
		printk(KERN_DEBUG "[E-PEN]: Written on:0x%lx", ulAddr);
		msleep(20);
	}
	printk(KERN_DEBUG "\nWriting done\n");

	return 0;
}

int wacom_i2c_flash_verify(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			   u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j;
	char sum;
	u8 buf[WRITE_BUFF], bank;

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

		ret = wacom_i2c_master_send(wac_i2c->client, buf, len,
					    WACOM_I2C_BOOT);
		if (ret < 0) {
			printk(KERN_ERR "[E-PEN]: Write process aborted\n");
			return ERR_FAILED_ENTER;
		}

		len = 1;

		do {
			msleep(20);
			ret = wacom_i2c_master_recv(wac_i2c->client, buf, len,
						    WACOM_I2C_BOOT);
			j++;
			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				printk(KERN_ERR "[E-PEN]: Error:%x\n", buf[0]);
				return -1;
			}
		} while (buf[0] == 0xff);

		msleep(20);
		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		ret = wacom_i2c_master_send(wac_i2c->client, buf, len,
					    WACOM_I2C_BOOT);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN]: Firmware flash aborted while writing\n");
			return ERR_NOT_FLASH;
		}

		msleep(20);
		len = 1;
		j = 0;
		do {
			msleep(20);
			ret = wacom_i2c_master_recv(wac_i2c->client, buf, len,
						    WACOM_I2C_BOOT);
			j++;

			if (j > RETRY || buf[0] == 0x81 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				printk(KERN_ERR "[E-PEN]: Error:%x", buf[0]);
				return -1;
			}
		} while (buf[0] == 0xff);
		printk(KERN_DEBUG "[E-PEN]: Verified:0x%lx", ulAddr);
		msleep(20);
	}
	printk("\n[E-PEN]: Verifying done\n");

	return 0;
}

int wacom_i2c_flash(struct wacom_i2c *wac_i2c)
{
	int ret, blver, mcu;
	unsigned long i, maxAddr;
	ret = blver = mcu = 0;
	i = maxAddr = 0;

	wake_lock(&wac_i2c->wakelock);

	ret = wacom_i2c_flash_cmd(wac_i2c);
	msleep(20);
	ret = wacom_i2c_flash_enter(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: flashEnter:%d\n", ret);

	/*sleep */
	msleep(20);

	blver = wacom_i2c_flash_BLVer(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: blver:%d\n", blver);

	/*sleep */
	msleep(20);

	mcu = wacom_i2c_flash_mcuId(wac_i2c);
	printk(KERN_DEBUG "[E-PEN]: mcu:%x\n", mcu);
	if (Mpu_type != mcu) {
		wacom_i2c_flash_end(wac_i2c);
		return -1;
	}

	/*sleep */
	msleep(20);

	switch (mcu) {
	case MPUVER_W8501:
		printk(KERN_DEBUG "[E-PEN]: flashing for w8501 started\n");
		maxAddr = MAX_ADDR_W8501;
		ret = wacom_i2c_flash_erase(wac_i2c, FLASH_ERASE,
					    MAX_BLOCK_W8501, END_BLOCK);
		if (ret < 0)
			return -1;
		printk(KERN_DEBUG "[E-PEN]: erased:%d\n", ret);

		msleep(20);

		ret = wacom_i2c_flash_write(wac_i2c, START_ADDR,
					    NUM_BLOCK_2WRITE, maxAddr);
		if (ret < 0)
			return -1;

		msleep(20);

		ret = wacom_i2c_flash_verify(wac_i2c, START_ADDR,
					     NUM_BLOCK_2WRITE, maxAddr);
		if (ret < 0)
			return -1;

		break;

	case MPUVER_514:
		printk(KERN_DEBUG "[E-PEN]: Flashing for 514 started\n");
		maxAddr = MAX_ADDR_514;
		ret = wacom_i2c_flash_erase(wac_i2c, FLASH_ERASE,
					    MAX_BLOCK_514, END_BLOCK);
		if (ret < 0)
			return -1;
		printk(KERN_DEBUG "[E-PEN]: erased:%d\n", ret);

		msleep(20);

		ret = wacom_i2c_flash_write(wac_i2c, START_ADDR,
					    NUM_BLOCK_2WRITE, maxAddr);
		if (ret < 0)
			return -1;

		msleep(20);

		ret = wacom_i2c_flash_verify(wac_i2c, START_ADDR,
					     NUM_BLOCK_2WRITE, maxAddr);
		if (ret < 0)
			return -1;

		break;

	default:
		printk(KERN_DEBUG "[E-PEN]: default called\n");
		break;
	}
	msleep(20);

	ret = wacom_i2c_flash_end(wac_i2c);
	wake_unlock(&wac_i2c->wakelock);
	printk(KERN_DEBUG "[E-PEN]: Firmware successfully updated:%x\n", ret);

	return 0;
}
