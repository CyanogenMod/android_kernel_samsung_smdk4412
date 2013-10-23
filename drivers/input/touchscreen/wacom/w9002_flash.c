/*
 *  w9002_flash.c - Wacom Digitizer Controller Flash Driver
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
#include "w9002_flash.h"

static int wacom_i2c_flash_chksum(struct wacom_i2c *wac_i2c,
				  unsigned char *flash_data,
				  unsigned long *max_address)
{
	unsigned long i;
	unsigned long chksum = 0;

	for (i = 0x0000; i <= *max_address; i++)
		chksum += flash_data[i];

	chksum &= 0xFFFF;

	return (int)chksum;
}

static int wacom_flash_cmd(struct wacom_i2c *wac_i2c)
{
	int rv, len, i;
	u8 buf[10];
	bool i2c_mode = WACOM_I2C_MODE_BOOT;

#if defined(CONFIG_MACH_KONA)
		buf[0] = 0x0d;
		buf[1] = FLASH_START0;
		buf[2] = FLASH_START1;
		buf[3] = FLASH_START2;
		buf[4] = FLASH_START3;
		buf[5] = FLASH_START4;
		buf[6] = FLASH_START5;
		buf[7] = 0x0d;

		len = 8;
		rv = wacom_i2c_send(wac_i2c, buf, len, i2c_mode);
#else

	for (i = 0; i < 2; ++i) {
		len = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 0x32;
		buf[len++] = CMD_SET_FEATURE;

		rv = wacom_i2c_send(wac_i2c, buf, len, i2c_mode);
		if (rv < 0) {
			printk(KERN_DEBUG "epen:fail change to normal:%d\n",
			       rv);

			i2c_mode = WACOM_I2C_MODE_NORMAL;
			continue;
		}

		len = 0;
		buf[len++] = 5;
		buf[len++] = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 2;
		buf[len++] = 2;

		rv = wacom_i2c_send(wac_i2c, buf, len, i2c_mode);
		if (rv < 0) {
			printk(KERN_DEBUG "epen:fail change to normal:%d\n",
			       rv);
			i2c_mode = WACOM_I2C_MODE_NORMAL;
			continue;
		}
	}
#endif
	if (rv < 0) {
		printk(KERN_ERR
			"Sending flash command failed\n");
		return -1;
	}

	printk(KERN_DEBUG "epen:flash cmd sent:%d\n", rv);
	msleep(500);

	return 0;
}

static bool flash_query(struct wacom_i2c *wac_i2c)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	printk(KERN_DEBUG "epen: %s\n", __func__);
	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_QUERY;
	command[6] = ECH = 7;

	rv = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
		return false;
	}

	if ((response[3] != QUERY_CMD) || (response[4] != ECH)) {
		printk(KERN_DEBUG "epen: res3:%d res4:%d\n", response[3],
		       response[4]);
		return false;
	}
	if (response[5] != QUERY_RSP) {
		printk(KERN_DEBUG "epen: res5:%d\n", response[5]);
		return false;
	}

	return true;
}

static bool flash_blver(struct wacom_i2c *wac_i2c, int *blver)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_BLVER;
	command[6] = ECH = 7;

	rv = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
		return false;
	}

	if ((response[3] != BOOT_CMD) || (response[4] != ECH))
		return false;

	*blver = (int)response[5];

	return true;
}

static bool flash_mputype(struct wacom_i2c *wac_i2c, int *pMpuType)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_MPU;
	command[6] = ECH = 7;

	rv = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	usleep_range(1000, 1000);

	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
		return false;
	}

	if ((response[3] != MPU_CMD) || (response[4] != ECH))
		return false;

	*pMpuType = (int)response[5];

	return true;
}

static bool flash_security_unlock(struct wacom_i2c *wac_i2c, int *status)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_SECURITY_UNLOCK;
	command[6] = ECH = 7;

	rv = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return 0;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	usleep_range(1000, 1000);

	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
		return false;
	}

	if ((response[3] != SEC_CMD) || (response[4] != ECH))
		return false;

	*status = (int)response[5];

	return true;
}

static bool flash_end(struct wacom_i2c *wac_i2c)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_EXIT;
	command[6] = ECH = 7;

	rv = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	return true;
}

static int GetBLVersion(struct wacom_i2c *wac_i2c, int *pBLVer)
{
	int rv;
	int retry = 0;

	wacom_flash_cmd(wac_i2c);
	do {
		msleep(100);
		rv = flash_query(wac_i2c);
		retry++;
	} while (rv < 0 && retry < 10);

	if (rv < 0)
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;

	rv = flash_blver(wac_i2c, pBLVer);
	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;
}

static int GetMpuType(struct wacom_i2c *wac_i2c, int *pMpuType)
{
	int rv;

	if (!flash_query(wac_i2c)) {
		if (!wacom_flash_cmd(wac_i2c)) {
			return EXIT_FAIL_ENTER_FLASH_MODE;
		} else {
			msleep(100);
			if (!flash_query(wac_i2c))
				return EXIT_FAIL_FLASH_QUERY;
		}
	}

	rv = flash_mputype(wac_i2c, pMpuType);
	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL_GET_MPU_TYPE;
}

static int SetSecurityUnlock(struct wacom_i2c *wac_i2c, int *pStatus)
{
	int rv;

	if (!flash_query(wac_i2c)) {
		if (!wacom_flash_cmd(wac_i2c)) {
			return EXIT_FAIL_ENTER_FLASH_MODE;
		} else {
			msleep(100);
			if (!flash_query(wac_i2c))
				return EXIT_FAIL_FLASH_QUERY;
		}
	}

	rv = flash_security_unlock(wac_i2c, pStatus);
	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL;
}

static bool flash_erase(struct wacom_i2c *wac_i2c, bool bAllUserArea,
			int *eraseBlock, int num)
{
	int rv, ECH;
	unsigned char sum;
	unsigned char buf[72];
	unsigned char cmd_chksum;
	u16 len;
	int i, j;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	for (i = 0; i < num; i++) {
		/*msleep(500);*/
retry:
		len = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 0x37;
		buf[len++] = CMD_SET_FEATURE;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: failing 1:%d\n", i);
			return false;
		}

		command[0] = 5;
		command[1] = 0;
		command[2] = 7;
		command[3] = 0;
		command[4] = BOOT_CMD_REPORT_ID;
		command[5] = BOOT_ERASE_FLASH;
		command[6] = ECH = i;
		command[7] = *eraseBlock;
		eraseBlock++;

		sum = 0;
		for (j = 0; j < 8; j++)
			sum += command[j];
		cmd_chksum = ~sum + 1;
		command[8] = cmd_chksum;

		rv = wacom_i2c_send(wac_i2c, command, 9, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: failing 2:%d\n", i);
			return false;
		}

		switch (i) {
		case 0:
			msleep(3000);
			break;

		case 1:
			msleep(3000);
			break;

		case 2:
			msleep(5000);
			break;

		case 3:
			msleep(500);
			break;

		default:
			msleep(5000);
			break;
		}

		len = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 0x38;
		buf[len++] = CMD_GET_FEATURE;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: failing 3:%d\n", i);
			return false;
		}

		len = 0;
		buf[len++] = 5;
		buf[len++] = 0;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: failing 4:%d\n", i);
			return false;
		}

		rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
				    WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: failing 5:%d\n", i);
			return false;
		}

		if ((response[3] != ERS_CMD) || (response[4] != ECH)) {
			printk(KERN_DEBUG "epen: failing 6:%d\n", i);
			return false;
		}

		if (response[5] == 0x80) {
			printk(KERN_DEBUG "epen: retry\n");
			goto retry;
		}
		if (response[5] != ACK) {
			printk(KERN_DEBUG "epen: failing 7:%d res5:%d\n", i,
			       response[5]);
			return false;
		}
	}
	return true;
}

static bool is_flash_marking(struct wacom_i2c *wac_i2c,
			     size_t data_size, bool *bMarking, int iMpuID)
{
	const int MAX_CMD_SIZE = (12 + FLASH_BLOCK_SIZE + 2);
	int rv, ECH;
	unsigned char flash_data[FLASH_BLOCK_SIZE];
	unsigned char buf[300];
	unsigned char sum;
	int len;
	unsigned int i, j;
	unsigned char response[RSP_SIZE];
	unsigned char command[MAX_CMD_SIZE];

	*bMarking = false;

	printk(KERN_DEBUG "epen: started\n");
	for (i = 0; i < FLASH_BLOCK_SIZE; i++)
		flash_data[i] = 0xFF;

	flash_data[56] = 0x00;

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 76;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_VERIFY_FLASH;
	command[6] = ECH = 1;
	command[7] = 0xC0;
	command[8] = 0x1F;
	command[9] = 0x01;
	command[10] = 0x00;
	command[11] = 8;

	sum = 0;
	for (j = 0; j < 12; j++)
		sum += command[j];

	command[MAX_CMD_SIZE - 2] = ~sum + 1;

	sum = 0;
	printk(KERN_DEBUG "epen: start writing command\n");
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
		command[i] = flash_data[i - 12];
		sum += flash_data[i - 12];
	}
	command[MAX_CMD_SIZE - 1] = ~sum + 1;

	printk(KERN_DEBUG "epen: sending command\n");
	rv = wacom_i2c_send(wac_i2c, command, MAX_CMD_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	rv = wacom_i2c_recv(wac_i2c, response, RSP_SIZE, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
		return false;
	}

	printk(KERN_DEBUG "epen: checking response\n");
	if ((response[3] != MARK_CMD) ||
	    (response[4] != ECH) || (response[5] != ACK)) {
		printk(KERN_DEBUG "epen: fails res3:%d res4:%d res5:%d\n",
		       response[3], response[4], response[5]);
		return false;
	}

	*bMarking = true;
	return true;
}

static bool flash_write_block(struct wacom_i2c *wac_i2c, char *flash_data,
			      unsigned long ulAddress, u8 *pcommand_id)
{
	const int MAX_COM_SIZE = (12 + FLASH_BLOCK_SIZE + 2);
	int len, ECH;
	unsigned char buf[300];
	int rv;
	unsigned char sum;
	unsigned char command[MAX_COM_SIZE];
	unsigned char response[RSP_SIZE];
	unsigned int i;

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0)
		return false;

	command[0] = 5;
	command[1] = 0;
	command[2] = 76;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_WRITE_FLASH;
	command[6] = ECH = ++(*pcommand_id);
	command[7] = ulAddress & 0x000000ff;
	command[8] = (ulAddress & 0x0000ff00) >> 8;
	command[9] = (ulAddress & 0x00ff0000) >> 16;
	command[10] = (ulAddress & 0xff000000) >> 24;
	command[11] = 8;
	sum = 0;
	for (i = 0; i < 12; i++)
		sum += command[i];
	command[MAX_COM_SIZE - 2] = ~sum + 1;

	sum = 0;
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
		command[i] = flash_data[ulAddress + (i - 12)];
		sum += flash_data[ulAddress + (i - 12)];
	}
	command[MAX_COM_SIZE - 1] = ~sum + 1;

	rv = wacom_i2c_send(wac_i2c, command, BOOT_CMD_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	if ((response[3] != WRITE_CMD) ||
	    (response[4] != ECH) || response[5] != ACK)
		return false;

	return true;

}

static bool flash_write(struct wacom_i2c *wac_i2c,
			unsigned char *flash_data, size_t data_size,
			unsigned long start_address, unsigned long *max_address,
			int mpuType)
{
	unsigned long ulAddress;
	int i;
	bool rv;
	unsigned long pageNo = 0;
	u8 command_id = 0;

	printk(KERN_DEBUG "epen: flash_write start\n");

	for (ulAddress = start_address; ulAddress < *max_address;
	     ulAddress += FLASH_BLOCK_SIZE) {
		unsigned int j;
		bool bWrite = false;

		/* Wacom 2012/10/04: skip if all each data locating on
			from ulAddr to ulAddr+Block_SIZE_W are 0xff */
		for (i = 0; i < FLASH_BLOCK_SIZE; i++) {
			if (flash_data[ulAddress + i] != 0xFF)
				break;
		}
		if (i == (FLASH_BLOCK_SIZE)) {
			/*printk(KERN_DEBUG"epen:BLOCK PASSED\n"); */
			continue;
		}
		/* Wacom 2012/10/04 */

		for (j = 0; j < FLASH_BLOCK_SIZE; j++) {
			if (flash_data[ulAddress + j] == 0xFF)
				continue;
			else {
				bWrite = true;
				break;
			}
		}

		if (!bWrite) {
			pageNo++;
			continue;
		}

		rv = flash_write_block(wac_i2c, flash_data, ulAddress,
				       &command_id);
		if (!rv)
			return false;

		pageNo++;
	}

	return true;
}

static bool flash_verify(struct wacom_i2c *wac_i2c,
			 unsigned char *flash_data, size_t data_size,
			 unsigned long start_address,
			 unsigned long *max_address, int mpuType)
{
	int ECH;
	unsigned long ulAddress;
	int rv;
	unsigned long pageNo = 0;
	u8 command_id = 0;
	printk(KERN_DEBUG "epen: verify starts\n");
	for (ulAddress = start_address; ulAddress < *max_address;
	     ulAddress += FLASH_BLOCK_SIZE) {
		const int MAX_CMD_SIZE = 12 + FLASH_BLOCK_SIZE + 2;
		unsigned char buf[300];
		unsigned char sum;
		int len;
		unsigned int i, j;
		unsigned char command[MAX_CMD_SIZE];
		unsigned char response[RSP_SIZE];

		len = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 0x37;
		buf[len++] = CMD_SET_FEATURE;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
			return false;
		}

		command[0] = 5;
		command[1] = 0;
		command[2] = 76;
		command[3] = 0;
		command[4] = BOOT_CMD_REPORT_ID;
		command[5] = BOOT_VERIFY_FLASH;
		command[6] = ECH = ++command_id;
		command[7] = ulAddress & 0x000000ff;
		command[8] = (ulAddress & 0x0000ff00) >> 8;
		command[9] = (ulAddress & 0x00ff0000) >> 16;
		command[10] = (ulAddress & 0xff000000) >> 24;
		command[11] = 8;

		sum = 0;
		for (j = 0; j < 12; j++)
			sum += command[j];
		command[MAX_CMD_SIZE - 2] = ~sum + 1;

		sum = 0;
		for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
			command[i] = flash_data[ulAddress + (i - 12)];
			sum += flash_data[ulAddress + (i - 12)];
		}
		command[MAX_CMD_SIZE - 1] = ~sum + 1;

		rv = wacom_i2c_send(wac_i2c, command, BOOT_CMD_SIZE,
				    WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
			return false;
		}

		if (ulAddress <= 0x0ffff)
			ndelay(250000);
		else if (ulAddress >= 0x10000 && ulAddress <= 0x20000)
			ndelay(350000);
		else
			usleep_range(10000, 10000);

		len = 0;
		buf[len++] = 4;
		buf[len++] = 0;
		buf[len++] = 0x38;
		buf[len++] = CMD_GET_FEATURE;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
			return false;
		}

		len = 0;
		buf[len++] = 5;
		buf[len++] = 0;

		rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
			return false;
		}

		rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
				    WACOM_I2C_MODE_BOOT);
		if (rv < 0) {
			printk(KERN_DEBUG "epen: 5 rv:%d\n", rv);
			return false;
		}

		if ((response[3] != VERIFY_CMD) ||
		    (response[4] != ECH) || (response[5] != ACK)) {
			printk(KERN_DEBUG "epen: res3:%d res4:%d res5:%d\n",
			       response[3], response[4], response[5]);
			return false;
		}
		pageNo++;
	}

	return true;
}

static bool flash_marking(struct wacom_i2c *wac_i2c,
			  size_t data_size, bool bMarking, int iMpuID)
{
	const int MAX_CMD_SIZE = 12 + FLASH_BLOCK_SIZE + 2;
	int rv, ECH;
	unsigned char flash_data[FLASH_BLOCK_SIZE];
	unsigned char buf[300];
	unsigned char response[RSP_SIZE];
	unsigned char sum;
	int len;
	unsigned int i, j;
	unsigned char command[MAX_CMD_SIZE];

	for (i = 0; i < FLASH_BLOCK_SIZE; i++)
		flash_data[i] = 0xFF;

	if (bMarking)
		flash_data[56] = 0x00;

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 1 rv:%d\n", rv);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 76;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_WRITE_FLASH;
	command[6] = ECH = 1;
	command[7] = 0xC0;
	command[8] = 0x1F;
	command[9] = 0x01;
	command[10] = 0x00;
	command[11] = 8;

	sum = 0;
	for (j = 0; j < 12; j++)
		sum += command[j];
	command[MAX_CMD_SIZE - 2] = ~sum + 1;

	sum = 0;
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
		command[i] = flash_data[i - 12];
		sum += flash_data[i - 12];
	}
	command[MAX_CMD_SIZE - 1] = ~sum + 1;

	rv = wacom_i2c_send(wac_i2c, command, BOOT_CMD_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 2 rv:%d\n", rv);
		return false;
	}

	usleep_range(10000, 10000);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 3 rv:%d\n", rv);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (rv < 0) {
		printk(KERN_DEBUG "epen: 4 rv:%d\n", rv);
		return false;
	}

	printk(KERN_DEBUG "epen: confirming marking\n");
	rv = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (rv < 0)
		return false;

	if ((response[3] != 1) || (response[4] != ECH)\
		|| (response[5] != ACK)) {
		printk(KERN_DEBUG "epen: failing res3:%d res4:%d res5:%d\n",
		       response[3], response[4], response[5]);
		return false;
	}

	return true;
}

int wacom_i2c_flash(struct wacom_i2c *wac_i2c)
{
	unsigned long max_address = 0;
	unsigned long start_address = 0x4000;
	int eraseBlock[50], eraseBlockNum;
	bool bRet;
	int iChecksum;
	int iBLVer, iMpuType, iStatus;
	bool bMarking;
	int iRet;
	unsigned long ulMaxRange;

	if (Binary == NULL) {
		printk(KERN_ERR"[E-PEN] Data is NULL. Exit.\n");
		return -1;
	}

#ifdef WACOM_HAVE_FWE_PIN
	if (wac_i2c->have_fwe_pin) {
		wac_i2c->wac_pdata->compulsory_flash_mode(true);
		/*Reset */
		wac_i2c->wac_pdata->reset_platform_hw();
		msleep(200);
		printk(KERN_DEBUG "epen: Set FWE\n");
	}
#endif
	wake_lock(&wac_i2c->wakelock);

	printk(KERN_DEBUG "epen:start getting the boot loader version\n");
	/*Obtain boot loader version */
	iRet = GetBLVersion(wac_i2c, &iBLVer);
	if (iRet != EXIT_OK) {
		printk(KERN_DEBUG "epen:failed to get Boot Loader version\n");
		goto fw_update_error;
	}

	printk(KERN_DEBUG "epen: start getting the MPU version\n");
	/*Obtain MPU type: this can be manually done in user space */
	iRet = GetMpuType(wac_i2c, &iMpuType);
	if (iRet != EXIT_OK) {
		printk(KERN_DEBUG "epen: failed to get MPU type\n");
		goto fw_update_error;
	}

	/*Set start and end address and block numbers */
	eraseBlockNum = 0;
	start_address = 0x4000;
	max_address = 0x12FFF;
	eraseBlock[eraseBlockNum++] = 2;
	eraseBlock[eraseBlockNum++] = 1;
	eraseBlock[eraseBlockNum++] = 0;
	eraseBlock[eraseBlockNum++] = 3;

	printk(KERN_DEBUG "epen: obtaining the checksum\n");
	/*Calculate checksum */
	iChecksum = wacom_i2c_flash_chksum(wac_i2c, Binary, &max_address);
	printk(KERN_DEBUG "epen: Checksum is :%d\n", iChecksum);

	bRet = true;

	printk(KERN_DEBUG "epen: setting the security unlock\n");
	/*Unlock security */
	iRet = SetSecurityUnlock(wac_i2c, &iStatus);
	if (iRet != EXIT_OK) {
		printk(KERN_DEBUG "epen: failed to set security unlock\n");
		goto fw_update_error;
	}

	/*Set adress range */
	ulMaxRange = max_address;
	ulMaxRange -= start_address;
	ulMaxRange >>= 6;
	if (max_address > (ulMaxRange << 6))
		ulMaxRange++;

	printk(KERN_DEBUG "epen: connecting to Wacom Digitizer\n");
	printk(KERN_DEBUG "epen: erasing the current firmware\n");
	/*Erase the old program */
	bRet = flash_erase(wac_i2c, true, eraseBlock, eraseBlockNum);
	if (!bRet) {
		printk(KERN_DEBUG "epen: failed to erase the user program\n");
		iRet = EXIT_FAIL_ERASE;
		goto fw_update_error;
	}
	printk(KERN_DEBUG "epen: erasing done\n");

	max_address = 0x11FC0;

	printk(KERN_DEBUG "epen: writing new firmware\n");
	/*Write the new program */
	bRet =
	    flash_write(wac_i2c, Binary, DATA_SIZE, start_address, &max_address,
			iMpuType);
	if (!bRet) {
		printk(KERN_DEBUG "epen: failed to write firmware\n");
		iRet = EXIT_FAIL_WRITE_FIRMWARE;
		goto fw_update_error;
	}

	printk(KERN_DEBUG "epen: start marking\n");
	/*Set mark in writing process */
	bRet = flash_marking(wac_i2c, DATA_SIZE, true, iMpuType);
	if (!bRet) {
		printk(KERN_DEBUG "epen: failed to mark firmware\n");
		iRet = EXIT_FAIL_WRITE_FIRMWARE;
		goto fw_update_error;
	}

	/*Set the address for verify */
	start_address = 0x4000;
	max_address = 0x11FBF;

	printk(KERN_DEBUG "epen: start the verification\n");
	/*Verify the written program */
	bRet =
	    flash_verify(wac_i2c, Binary, DATA_SIZE, start_address,
			 &max_address, iMpuType);
	if (!bRet) {
		printk(KERN_DEBUG "epen: failed to verify the firmware\n");
		iRet = EXIT_FAIL_VERIFY_FIRMWARE;
		goto fw_update_error;
	}

	printk(KERN_DEBUG "epen: checking the mark\n");
	/*Set mark */
	bRet = is_flash_marking(wac_i2c, DATA_SIZE, &bMarking, iMpuType);
	if (!bRet) {
		printk(KERN_DEBUG "epen: marking firmwrae failed\n");
		iRet = EXIT_FAIL_WRITING_MARK_NOT_SET;
		goto fw_update_error;
	}

	/*Enable */
	printk(KERN_DEBUG "epen: closing the boot mode\n");
	bRet = flash_end(wac_i2c);
	if (!bRet) {
		printk(KERN_DEBUG "epen: closing boot mode failed\n");
		iRet = EXIT_FAIL_WRITING_MARK_NOT_SET;
		goto fw_update_error;
	}
	iRet = EXIT_OK;
	printk(KERN_DEBUG "epen: write and verify completed\n");

fw_update_error:
	wake_unlock(&wac_i2c->wakelock);

#ifdef WACOM_HAVE_FWE_PIN
	if (wac_i2c->have_fwe_pin) {
		wac_i2c->wac_pdata->compulsory_flash_mode(false);
		/*Reset */
		wac_i2c->wac_pdata->reset_platform_hw();
		msleep(200);
	}
#endif
	return iRet;
}
