#include "wacom_i2c_flash_g9.h"
/*-------------------------------------------------------*/
/*********************************************************/
/*---------NEW FUNCTIONS---------------------------------*/
/*********************************************************/
/*-------------------------------------------------------*/

#define DATA_SIZE (65536 * 4)
#define WACOM_I2C_FLASH 0x56
#define WACOM_I2C_FLASH2 0x9

int wacom_i2c_flash_cmd_g9(struct wacom_i2c *wac_i2c)
{
	int ret, len;
	u8 buf[8];

	printk(KERN_DEBUG "[PEN] %s\n", __func__);

	buf[0] = 0x0d;
	buf[1] = FLASH_START0;
	buf[2] = FLASH_START1;
	buf[3] = FLASH_START2;
	buf[4] = FLASH_START3;
	buf[5] = FLASH_START4;
	buf[6] = FLASH_START5;
	buf[7] = 0x0d;

	len = 8;
	ret = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH);
	if (ret < 0) {
		printk(KERN_ERR
			"[PEN] Sending flash command failed 2\n");
		return -1;
	}
	msleep(270);

	return 0;
}

bool flash_cmd(struct wacom_i2c *wac_i2c)
{
	int rv, len;
	u8 buf[10];

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x32;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_master_send(wac_i2c->client,
			buf, len, WACOM_I2C_FLASH);
	if (rv < 0)
		return false;

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 2;
	buf[len++] = 2;
	rv = wacom_i2c_master_send(wac_i2c->client,
			buf, len, WACOM_I2C_FLASH);
	if (rv < 0)
		return false;

	return true;
}

bool
flash_query(struct wacom_i2c *wac_i2c)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x37;				/* Command-LSB, ReportType:Feature(11) ReportID:7 */
	buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

	printk(KERN_DEBUG
		"[PEN] %s started buf[3]:%d len:%d\n",
		__func__, buf[3], len);
	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;						/* Data Register-LSB */
	command[1] = 0;						/* Data-Register-MSB */
	command[2] = 5;							/* Length Field-LSB */
	command[3] = 0;							/* Length Field-MSB */
	command[4] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[5] = BOOT_QUERY;					/* Report:Boot Query command */
	command[6] = ECH = 7;							/* Report:echo */

	rv = wacom_i2c_master_send(wac_i2c->client, command, 7, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x38;				/* Command-LSB, ReportType:Feature(11) ReportID:8 */
	buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;					/* Data Register-LSB */
	buf[len++] = 0;					/* Data Register-MSB */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_recv(wac_i2c->client, response,
		BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ( (response[3] != QUERY_CMD) ||
	     (response[4] != ECH) ) {
		printk(KERN_ERR "[PEN] %s res3:%d res4:%d\n", __func__, response[3], response[4]);
		return false;
	}
	if (response[5] != QUERY_RSP) {
		printk(KERN_ERR "[PEN] %s res5:%d\n", __func__, response[5]);
		return false;
	}

	return true;
}

bool
flash_blver(struct wacom_i2c *wac_i2c, int *blver)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x37;				/* Command-LSB, ReportType:Feature(11) ReportID:7 */
	buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;						/* Data Register-LSB */
	command[1] = 0;						/* Data-Register-MSB */
	command[2] = 5;							/* Length Field-LSB */
	command[3] = 0;							/* Length Field-MSB */
	command[4] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[5] = BOOT_BLVER;					/* Report:Boot Version command */
	command[6] = ECH = 7;							/* Report:echo */

	rv = wacom_i2c_master_send(wac_i2c->client, command, 7, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x38;				/* Command-LSB, ReportType:Feature(11) ReportID:8 */
	buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;					/* Data Register-LSB */
	buf[len++] = 0;					/* Data Register-MSB */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_recv(wac_i2c->client, response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ((response[3] != BOOT_CMD) ||
		(response[4] != ECH))
		return false;

	*blver = (int)response[5];

	return true;
}

bool
flash_mputype(struct wacom_i2c *wac_i2c, int* pMpuType)
{
	int rv, ECH;
	u8 buf[4];
	u16 len;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x37;				/* Command-LSB, ReportType:Feature(11) ReportID:7 */
	buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;						/* Data Register-LSB */
	command[1] = 0;						/* Data-Register-MSB */
	command[2] = 5;							/* Length Field-LSB */
	command[3] = 0;							/* Length Field-MSB */
	command[4] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[5] = BOOT_MPU;					/* Report:Boot Query command */
	command[6] = ECH = 7;							/* Report:echo */

	rv = wacom_i2c_master_send(wac_i2c->client, command, 7, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x38;				/* Command-LSB, ReportType:Feature(11) ReportID:8 */
	buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;					/* Data Register-LSB */
	buf[len++] = 0;					/* Data Register-MSB */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_recv(wac_i2c->client, response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ((response[3] != MPU_CMD) ||
		(response[4] != ECH))
		return false;

	*pMpuType = (int)response[5];

	return true;
}

bool
flash_security_unlock(struct wacom_i2c *wac_i2c, int *status)
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

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_SECURITY_UNLOCK;
	command[6] = ECH = 7;

	rv = wacom_i2c_master_send(wac_i2c->client, command, 7, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_recv(wac_i2c->client, response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ((response[3] != SEC_CMD) ||
		(response[4] != ECH))
		return false;

	*status = (int)response[5];

	return true;
}

bool
flash_end(struct wacom_i2c *wac_i2c)
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

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_EXIT;
	command[6] = ECH = 7;

	rv = wacom_i2c_master_send(wac_i2c->client, command, 7, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	return true;
}


bool
flash_devcieType(struct wacom_i2c *wac_i2c)
{
	int rv;
	u8 buf[4];
	u16 len;

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x32;				/* Command-LSB, ReportType:Feature(11) ReportID:2 */
	buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;					/* Data Register-LSB */
	buf[len++] = 0;					/* Data Register-MSB */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_recv(wac_i2c->client, buf, 4, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	return true;
}

int
GetFWVersion(struct wacom_i2c *wac_i2c, PFW_VERSION pVer)
{
	int rv;
	unsigned char buf[12];
	int len;
	int iRet = EXIT_FAIL_GET_FIRMWARE_VERSION;

	buf[0] = pen_QUERY;
	len = 1;
	printk(KERN_DEBUG "[PEN] %s started\n", __func__);
	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0)
		goto end;

	msleep(1);

	len = 10;

	rv = wacom_i2c_master_recv(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0)
		goto end;

	if (buf[0] != 0x0F)
		goto end;

	pVer->UpVer = buf[7];
	pVer->LoVer = buf[8];
	iRet = EXIT_OK;

 end:
	return iRet;
}

int
GetBLVersion(struct wacom_i2c *wac_i2c, int* pBLVer)
{
	int rv;
	wacom_i2c_flash_cmd_g9(wac_i2c);
	if (!flash_query(wac_i2c)) {
		if (!wacom_i2c_flash_cmd_g9(wac_i2c)) {
			return EXIT_FAIL_ENTER_FLASH_MODE;
		}
		else {
			msleep(100);
			if (!flash_query(wac_i2c)){
				return EXIT_FAIL_FLASH_QUERY;
			}
		}
	}

	rv = flash_blver(wac_i2c, pBLVer);
 	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;
}

int
GetMpuType(struct wacom_i2c *wac_i2c, int* pMpuType)
{
	int rv;

	if (!flash_query(wac_i2c))
	{
		if (!wacom_i2c_flash_cmd_g9(wac_i2c))
		{
			return EXIT_FAIL_ENTER_FLASH_MODE;
		}
		else{
			msleep(100);
			if (!flash_query(wac_i2c))
			{
				return EXIT_FAIL_FLASH_QUERY;
			}
		}
	}

	rv = flash_mputype(wac_i2c, pMpuType);
	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL_GET_MPU_TYPE;
}

int
SetSecurityUnlock(struct wacom_i2c *wac_i2c, int* pStatus)
{
	int rv;

	if (!flash_query(wac_i2c))
	{
		if (!wacom_i2c_flash_cmd_g9(wac_i2c))
		{
			return EXIT_FAIL_ENTER_FLASH_MODE;
		}
		else{
			msleep(100);
			if (!flash_query(wac_i2c))
			{
				return EXIT_FAIL_FLASH_QUERY;
			}
		}
	}

	rv = flash_security_unlock(wac_i2c, pStatus);
	if (rv)
		return EXIT_OK;
	else
		return EXIT_FAIL;
}

bool
flash_erase(struct wacom_i2c *wac_i2c, bool bAllUserArea, int *eraseBlock, int num)
{
	int rv, ECH;
	unsigned char sum;
	unsigned char buf[72];
	unsigned char cmd_chksum;
	u16 len;
	int i,j;
	unsigned char command[CMD_SIZE];
	//unsigned char command_data[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	for (i=0; i<num; i++)
	{
		msleep(500);
retry:
		len = 0;
		buf[len++] = 4;					/* Command Register-LSB */
		buf[len++] = 0;					/* Command Register-MSB */
		buf[len++] = 0x37;				/* Command-LSB, ReportType:Feature(11) ReportID:7 */
		buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

		printk(KERN_DEBUG "[PEN] %s sending SET_FEATURE:%d\n", __func__, i);

		rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
		if (rv < 0) {
			printk(KERN_ERR "[PEN] %s(%d) %d\n",
				__func__, rv, __LINE__);
			return false;
		}

		printk(KERN_DEBUG "[PEN] %s setting a command:%d\n", __func__, i);

		command[0] = 5;						/* Data Register-LSB */
		command[1] = 0;						/* Data-Register-MSB */
		command[2] = 7;						/* Length Field-LSB */
		command[3] = 0;						/* Length Field-MSB */
		command[4] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
		command[5] = BOOT_ERASE_FLASH;			/* Report:erase command */
		command[6] = ECH = i;						/* Report:echo */
		command[7] = *eraseBlock;				/* Report:erased block No. */
		eraseBlock++;

		sum = 0;
		for (j=0; j<8; j++)
			sum += command[j];
		cmd_chksum = ~sum+1;					/* Report:check sum */
		command[8] = cmd_chksum;

		rv = wacom_i2c_master_send(wac_i2c->client, command, 9, WACOM_I2C_FLASH2);
		if (rv < 0) {
			printk(KERN_ERR "[PEN] %s(%d) %d\n",
				__func__, rv, __LINE__);
			return false;
		}


		msleep(5000);

		len = 0;
		buf[len++] = 4;					/* Command Register-LSB */
		buf[len++] = 0;					/* Command Register-MSB */
		buf[len++] = 0x38;				/* Command-LSB, ReportType:Feature(11) ReportID:8 */
		buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

		printk(KERN_DEBUG "[PEN] %s sending GET_FEATURE :%d\n", __func__, i);
		rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
		if (rv < 0) {
			printk(KERN_ERR "[PEN] %s(%d) %d\n",
				__func__, rv, __LINE__);
			return false;
		}


		len = 0;
		buf[len++] = 5;					/* Data Register-LSB */
		buf[len++] = 0;					/* Data Register-MSB */

		rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
		if (rv < 0) {
			printk(KERN_ERR "[PEN] %s(%d) %d\n",
				__func__, rv, __LINE__);
			return false;
		}


		rv = wacom_i2c_master_recv(wac_i2c->client, response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
		if (rv < 0) {
			printk(KERN_ERR "[PEN] %s(%d) %d\n",
				__func__, rv, __LINE__);
			return false;
		}


		if ((response[3] != ERS_CMD) ||
		    (response[4] != ECH)) {
			printk(KERN_ERR "[PEN] %s failing 5:%d\n", __func__, i);
			return false;
		}

		if (response[5] == 0x80) {
			printk(KERN_ERR "[PEN] %s retry\n", __func__);
			goto retry;
		}
		if (response[5] != ACK) {
			printk(KERN_ERR "[PEN] %s failing 6:%d res5:%d\n", __func__, i, response[5]);
			return false;
		}
		printk(KERN_DEBUG "[PEN] %s %d\n", __func__, i);
	}
	return true;
}

bool
is_flash_marking(struct wacom_i2c *wac_i2c,
	  size_t data_size, bool* bMarking, int iMpuID)
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

	printk(KERN_DEBUG "[PEN] %s started\n", __func__);
	for (i=0; i<FLASH_BLOCK_SIZE; i++) {
		flash_data[i]=0xFF;
	}

	flash_data[56]=0x00;

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
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

	command[MAX_CMD_SIZE - 2] = ~sum+1;

	sum = 0;
	printk(KERN_DEBUG "[PEN] %s start writing command\n", __func__);
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++){
		command[i] = flash_data[i-12];
		sum += flash_data[i-12];
	}
	command[MAX_CMD_SIZE - 1] = ~sum+1;

	printk(KERN_DEBUG "[PEN] %s sending command\n", __func__);
	rv = wacom_i2c_master_send(wac_i2c->client, command, MAX_CMD_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}


	msleep(10);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	printk(KERN_DEBUG "[PEN] %s sending GET_FEATURE 1\n", __func__);
	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}


	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	printk(KERN_DEBUG "[PEN] %s sending GET_FEATURE 2\n", __func__);
	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}


	printk(KERN_DEBUG "[PEN] %s receiving GET_FEATURE\n", __func__);
	rv = wacom_i2c_master_recv(wac_i2c->client, response, RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}


	printk(KERN_DEBUG "[PEN] %s checking response\n", __func__);
	if ((response[3] != MARK_CMD) ||
	    (response[4] != ECH) ||
	    (response[5] != ACK) ) {
		printk(KERN_ERR "[PEN] %s fails res3:%d res4:%d res5:%d\n", __func__, response[3], response[4], response[5]);
		return false;
	}

	*bMarking = true;
	return true;
}

bool
flash_write_block(struct wacom_i2c *wac_i2c, char *flash_data, unsigned long ulAddress, u8 *pcommand_id)
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
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x37;				/* Command-LSB, ReportType:Feature(11) ReportID:7 */
	buf[len++] = CMD_SET_FEATURE;	/* Command-MSB, SET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	command[0] = 5;						/* Data Register-LSB */
	command[1] = 0;						/* Data-Register-MSB */
	command[2] = 76;						/* Length Field-LSB */
	command[3] = 0;						/* Length Field-MSB */
	command[4] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[5] = BOOT_WRITE_FLASH;			/* Report:program  command */
	command[6] = ECH = ++(*pcommand_id);		/* Report:echo */
	command[7] = ulAddress&0x000000ff;
	command[8] = (ulAddress&0x0000ff00) >> 8;
	command[9] = (ulAddress&0x00ff0000) >> 16;
	command[10] = (ulAddress&0xff000000) >> 24;			/* Report:address(4bytes) */
	command[11] = 8;						/* Report:size(8*8=64) */
	sum = 0;
	for (i=0; i<12; i++)
		sum += command[i];
	command[MAX_COM_SIZE - 2] = ~sum+1;					/* Report:command checksum */

	sum = 0;
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++){
		command[i] = flash_data[ulAddress+(i-12)];
		sum += flash_data[ulAddress+(i-12)];
	}
	command[MAX_COM_SIZE - 1] = ~sum+1;				/* Report:data checksum */

	rv = wacom_i2c_master_send(wac_i2c->client, command, BOOT_CMD_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;					/* Command Register-LSB */
	buf[len++] = 0;					/* Command Register-MSB */
	buf[len++] = 0x38;				/* Command-LSB, ReportType:Feature(11) ReportID:8 */
	buf[len++] = CMD_GET_FEATURE;	/* Command-MSB, GET_REPORT */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;					/* Data Register-LSB */
	buf[len++] = 0;					/* Data Register-MSB */

	rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	rv = wacom_i2c_master_send(wac_i2c->client, response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ((response[3] != WRITE_CMD) ||
		(response[4] != ECH) ||
		response[5] != ACK)
		return false;

	return true;

}

bool
flash_write(struct wacom_i2c *wac_i2c,
	  unsigned char *flash_data, size_t data_size, unsigned long start_address, unsigned long *max_address, int mpuType)
{
	unsigned long ulAddress;
	int rv;
	unsigned long pageNo=0;
	u8 command_id = 0;

	printk(KERN_DEBUG "[PEN] %s flash_write start\n", __func__);

	for (ulAddress = start_address; ulAddress < *max_address; ulAddress += FLASH_BLOCK_SIZE)
		{
			unsigned int j;
			bool bWrite = false;

			for (j = 0; j < FLASH_BLOCK_SIZE; j++)
				{
					if (flash_data[ulAddress+j] == 0xFF)
						continue;
					else
						{
							bWrite = true;
							break;
						}
				}

			if (!bWrite)
				{
					pageNo++;
					continue;
				}

			rv = flash_write_block(wac_i2c, flash_data, ulAddress, &command_id);
			if(rv < 0)
				return false;

			pageNo++;
		}

	return true;
}

bool
flash_verify(struct wacom_i2c *wac_i2c,
	  unsigned char *flash_data, size_t data_size, unsigned long start_address, unsigned long *max_address, int mpuType)
{
	int ECH;
	unsigned long ulAddress;
	bool rv;
	unsigned long pageNo = 0;
	u8 command_id = 0;
	printk(KERN_DEBUG "[PEN] %s verify starts\n", __func__);
	for (ulAddress = start_address; ulAddress < *max_address; ulAddress += FLASH_BLOCK_SIZE)
		{
			const int MAX_CMD_SIZE = 12 + FLASH_BLOCK_SIZE + 2;
			unsigned char buf[300];
			unsigned char sum;
			int len;
			unsigned int i, j;
			unsigned char command[MAX_CMD_SIZE];
			unsigned char response[RSP_SIZE];

			//	printk(KERN_DEBUG "[PEN] %s verify starts\n", __func__);
			len = 0;
			buf[len++] = 4;
			buf[len++] = 0;
			buf[len++] = 0x37;
			buf[len++] = CMD_SET_FEATURE;

			rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
			if (rv < 0)
				return false;

			command[0] = 5;
			command[1] = 0;
			command[2] = 76;
			command[3] = 0;
			command[4] = BOOT_CMD_REPORT_ID;
			command[5] = BOOT_VERIFY_FLASH;
			command[6] = ECH = ++command_id;;
			command[7] = ulAddress&0x000000ff;
			command[8] = (ulAddress&0x0000ff00) >> 8;
			command[9] = (ulAddress&0x00ff0000) >> 16;
			command[10] = (ulAddress&0xff000000) >> 24;
			command[11] = 8;

			sum = 0;
			for (j=0; j<12; j++)
				sum += command[j];
			command[MAX_CMD_SIZE - 2] = ~sum+1;

			sum = 0;
			for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++){
				command[i] = flash_data[ulAddress+(i-12)];
				sum += flash_data[ulAddress+(i-12)];
			}
			command[MAX_CMD_SIZE - 1] = ~sum+1;

			rv = wacom_i2c_master_send(wac_i2c->client, command, BOOT_CMD_SIZE, WACOM_I2C_FLASH2);
			if (rv < 0)
				return false;

			msleep(10);

			len = 0;
			buf[len++] = 4;
			buf[len++] = 0;
			buf[len++] = 0x38;
			buf[len++] = CMD_GET_FEATURE;

			rv = wacom_i2c_master_send(wac_i2c->client, buf, len, WACOM_I2C_FLASH2);
			if (rv < 0)
				return false;

			len = 0;
			buf[len++] = 5;
			buf[len++] = 0;

			rv = wacom_i2c_master_send(wac_i2c->client, buf, len,
				WACOM_I2C_FLASH2);
			if (rv < 0)
				return false;

			rv = wacom_i2c_master_recv(wac_i2c->client, response,
				BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
			if (rv < 0)
				return false;

			if ((response[3] != VERIFY_CMD) ||
			    (response[4] != ECH) ||
			    (response[5] != ACK)) {
				printk(KERN_ERR
					"[PEN] %s res3:%d res4:%d res5:%d\n",
					__func__, response[3], response[4], response[5]);
				return false;
			}
			pageNo++;
		}

	return true;
}

bool
flash_marking(struct wacom_i2c *wac_i2c,
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
	/* Command Register-LSB */
	buf[len++] = 4;
	/* Command Register-MSB */
	buf[len++] = 0;
	/* Command-LSB, ReportType:Feature(11) ReportID:7 */
	buf[len++] = 0x37;
	/* Command-MSB, SET_REPORT */
	buf[len++] = CMD_SET_FEATURE;

	rv = wacom_i2c_master_send(wac_i2c->client,
		buf, len, WACOM_I2C_FLASH2);
	if (rv < 0)
		return false;

	/* Data Register-LSB */
	command[0] = 5;
	/* Data-Register-MSB */
	command[1] = 0;
	/* Length Field-LSB */
	command[2] = 76;
	/* Length Field-MSB */
	command[3] = 0;
	/* Report:ReportID */
	command[4] = BOOT_CMD_REPORT_ID;
	/* Report:program  command */
	command[5] = BOOT_WRITE_FLASH;
	/* Report:echo */
	command[6] = ECH = 1;
	command[7] = 0xC0;
	command[8] = 0x1F;
	command[9] = 0x01;
	/* Report:address(4bytes) */
	command[10] = 0x00;
	/* Report:size(8*8=64) */
	command[11] = 8;

	sum = 0;
	for (j = 0; j < 12; j++)
		sum += command[j];
	command[MAX_CMD_SIZE - 2] = ~sum + 1;

	sum = 0;
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
		command[i] = flash_data[i-12];
		sum += flash_data[i-12];
	}

	/* Report:data checksum */
	command[MAX_CMD_SIZE - 1] = ~sum + 1;


	rv = wacom_i2c_master_send(wac_i2c->client,
		command, BOOT_CMD_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	msleep(10);

	len = 0;
	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	rv = wacom_i2c_master_send(wac_i2c->client,
		buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	rv = wacom_i2c_master_send(wac_i2c->client,
		buf, len, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	printk(KERN_DEBUG "[PEN] %s confirming marking\n", __func__);
	rv = wacom_i2c_master_recv(wac_i2c->client,
		response, BOOT_RSP_SIZE, WACOM_I2C_FLASH2);
	if (rv < 0) {
		printk(KERN_ERR "[PEN] %s(%d) %d\n",
			__func__, rv, __LINE__);
		return false;
	}

	if ((response[3] != 1) ||
	    (response[4] != ECH) ||
	    (response[5] != ACK)) {
		printk(KERN_ERR
			"[PEN] %s failing res3:%d res4:%d res5:%d\n",
			__func__, response[3], response[4], response[5]);
		return false;
	}

	return true;
}

int
FlashWrite(struct wacom_i2c *wac_i2c, char* filename)
{
	unsigned long	max_address = 0;
	unsigned long	start_address = 0x4000;
	int eraseBlock[32], eraseBlockNum;
	bool bRet;
	unsigned long ulMaxRange;
	int iChecksum;
	int iBLVer, iMpuType, iStatus;
	int iRet;
	bool bBootFlash = false;

	iRet = GetBLVersion(wac_i2c, &iBLVer);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s Failed to get Boot Loader version\n",
			__func__);
		return iRet;
	}

	iRet = GetMpuType(wac_i2c, &iMpuType);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR "[PEN] %s Failed to get MPU type\n", __func__);
		return iRet;
	}

	printk(KERN_DEBUG "[PEN] %s start reading hex file\n", __func__);

	eraseBlockNum = 0;
	start_address = 0x4000;
	max_address = 0x12FFF;
	eraseBlock[eraseBlockNum++] = 2;
	eraseBlock[eraseBlockNum++] = 1;
	eraseBlock[eraseBlockNum++] = 0;
	eraseBlock[eraseBlockNum++] = 3;

	if (bBootFlash)
		eraseBlock[eraseBlockNum++] = 4;

	iChecksum =
		wacom_i2c_flash_chksum(wac_i2c,
			Binary, &max_address);
	printk(KERN_DEBUG "[PEN] %s iChecksum:%d\n",
		__func__, iChecksum);

	bRet = true;

	iRet = SetSecurityUnlock(wac_i2c, &iStatus);
	if (iRet != EXIT_OK)
		return iRet;

	ulMaxRange = max_address;
	ulMaxRange -= start_address;
	ulMaxRange >>= 6;
	if (max_address > (ulMaxRange << 6))
		ulMaxRange++;

	printk(KERN_DEBUG "[PEN] %s connecting to wacom digitizer\n", __func__);

	if (!bBootFlash) {
		printk(KERN_DEBUG
			"[PEN] %s erasing the user program\n",
			__func__);

		bRet = flash_erase(wac_i2c, true, eraseBlock, eraseBlockNum);
		if (!bRet) {
			printk(KERN_ERR
				"[PEN] %s failed to erase the user program\n",
				__func__);
			return EXIT_FAIL_ERASE;
		}
	}

	printk(KERN_DEBUG "[PEN] %s writing new user program\n", __func__);

	bRet = flash_write(wac_i2c, Binary, DATA_SIZE,
		start_address, &max_address, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s failed to write the user program\n",
			__func__);
		return EXIT_FAIL_WRITE_FIRMWARE;
	}

	bRet = flash_marking(wac_i2c, DATA_SIZE, true, iMpuType);
	if (!bRet) {
		printk(KERN_ERR "[PEN] %s failed to set mark\n", __func__);
		return EXIT_FAIL_WRITE_FIRMWARE;
	}

	printk(KERN_DEBUG "[PEN] %s writing completed\n", __func__);
	return EXIT_OK;
}

int FlashVerify(struct wacom_i2c *wac_i2c, char *filename)
{
	unsigned long max_address = 0;
	unsigned long start_address = 0x4000;
	bool bRet;
	int iChecksum;
	int iBLVer, iMpuType;
	unsigned long ulMaxRange;
	bool bMarking;
	int iRet;

	iRet = GetBLVersion(wac_i2c, &iBLVer);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s failed to get Boot Loader version\n",
			__func__);
		return iRet;
	}

	iRet = GetMpuType(wac_i2c, &iMpuType);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s failed to get MPU type\n",
			__func__);
		return iRet;
	}

	start_address = 0x4000;
	max_address = 0x11FBF;

	iChecksum = wacom_i2c_flash_chksum(wac_i2c, Binary, &max_address);
	printk(KERN_DEBUG
		"[PEN] %s check sum is: %d\n",
		__func__, iChecksum);

	ulMaxRange = max_address;
	ulMaxRange -= start_address;
	ulMaxRange >>= 6;
	if (max_address > (ulMaxRange << 6))
		ulMaxRange++;

	bRet = flash_verify(wac_i2c, Binary, DATA_SIZE,
			start_address, &max_address, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s failed to verify the firmware\n",
			__func__);
		return EXIT_FAIL_VERIFY_FIRMWARE;
	}

	bRet = is_flash_marking(wac_i2c, DATA_SIZE,
			&bMarking, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s there's no marking\n",
			__func__);
		return EXIT_FAIL_VERIFY_WRITING_MARK;
	}

	printk(KERN_DEBUG
		"[PEN] %s verifying completed\n",
		__func__);

	return EXIT_OK;
}

int wacom_i2c_flash_g9(struct wacom_i2c *wac_i2c)
{
	unsigned long max_address = 0;
	unsigned long start_address = 0x4000;
	int eraseBlock[32], eraseBlockNum;
	bool bRet;
	int iChecksum;
	int iBLVer, iMpuType, iStatus;
	bool bBootFlash = false;
	bool bMarking;
	int iRet;
	unsigned long ulMaxRange;

	printk(KERN_DEBUG "[PEN] %s\n", __func__);
	printk(KERN_DEBUG
		"[PEN] start getting the boot loader version\n");
	/*Obtain boot loader version*/
	iRet = GetBLVersion(wac_i2c, &iBLVer);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s failed to get Boot Loader version\n",
			__func__);
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;
	}

	printk(KERN_DEBUG
		"[PEN] start getting the MPU version\n");
	/*Obtain MPU type: this can be manually done in user space*/
	iRet = GetMpuType(wac_i2c, &iMpuType);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s failed to get MPU type\n",
			__func__);
		return EXIT_FAIL_GET_MPU_TYPE;
	}

	/*Set start and end address and block numbers*/
	eraseBlockNum = 0;
	start_address = 0x4000;
	max_address = 0x12FFF;
	eraseBlock[eraseBlockNum++] = 2;
	eraseBlock[eraseBlockNum++] = 1;
	eraseBlock[eraseBlockNum++] = 0;
	eraseBlock[eraseBlockNum++] = 3;

	/*If MPU is in Boot mode, do below*/
	if (bBootFlash)
		eraseBlock[eraseBlockNum++] = 4;

	printk(KERN_DEBUG
		"[PEN] obtaining the checksum\n");
	/*Calculate checksum*/
	iChecksum = wacom_i2c_flash_chksum(wac_i2c, Binary, &max_address);
	printk(KERN_DEBUG
		"[PEN] Checksum is :%d\n",
		iChecksum);

	bRet = true;

	printk(KERN_DEBUG
		"[PEN] setting the security unlock\n");
	/*Unlock security*/
	iRet = SetSecurityUnlock(wac_i2c, &iStatus);
	if (iRet != EXIT_OK) {
		printk(KERN_ERR
			"[PEN] %s failed to set security unlock\n",
			__func__);
		return iRet;
	}

	/*Set adress range*/
	ulMaxRange = max_address;
	ulMaxRange -= start_address;
	ulMaxRange >>= 6;
	if (max_address > (ulMaxRange<<6))
		ulMaxRange++;

	printk(KERN_DEBUG
		"[PEN] connecting to Wacom Digitizer\n");
	printk(KERN_DEBUG
		"[PEN] erasing the current firmware\n");
	/*Erase the old program*/
	bRet = flash_erase(wac_i2c, true, eraseBlock,  eraseBlockNum);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s failed to erase the user program\n",
			__func__);
		return EXIT_FAIL_ERASE;
	}
	printk(KERN_DEBUG
		"[PEN] erasing done\n");

	max_address = 0x11FC0;

	printk(KERN_DEBUG
		"[PEN] writing new firmware\n");
	/*Write the new program*/
	bRet = flash_write(wac_i2c, Binary, DATA_SIZE,
			start_address, &max_address, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s failed to write firmware\n",
			__func__);
		return EXIT_FAIL_WRITE_FIRMWARE;
	}

	printk(KERN_DEBUG
		"[PEN] start marking\n");

	/*Set mark in writing process*/
	bRet = flash_marking(wac_i2c, DATA_SIZE, true, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s failed to mark firmware\n",
			__func__);
		return EXIT_FAIL_WRITE_FIRMWARE;
	}

	/*Set the address for verify*/
	start_address = 0x4000;
	max_address = 0x11FBF;

	printk(KERN_DEBUG
		"[PEN] start the verification\n");
	/*Verify the written program*/
	bRet = flash_verify(wac_i2c, Binary, DATA_SIZE,
			start_address, &max_address, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] failed to verify the firmware\n");
		return EXIT_FAIL_VERIFY_FIRMWARE;
	}


	printk(KERN_DEBUG
		"[PEN] checking the mark\n");
	/*Set mark*/
	bRet = is_flash_marking(wac_i2c, DATA_SIZE,
		&bMarking, iMpuType);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s marking firmwrae failed\n",
			__func__);
		return EXIT_FAIL_WRITING_MARK_NOT_SET;
	}

	/*Enable */
	printk(KERN_DEBUG
		"[PEN] closing the boot mode\n");

	bRet = flash_end(wac_i2c);
	if (!bRet) {
		printk(KERN_ERR
			"[PEN] %s closing boot mode failed\n",
			__func__);
		return EXIT_FAIL_WRITING_MARK_NOT_SET;
	}

	printk(KERN_DEBUG
		"[PEN] write and verify completed\n");
	return EXIT_OK;
}

