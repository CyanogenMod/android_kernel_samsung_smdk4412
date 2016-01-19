/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 */
#include <mach/gpio.h>

#include "ssp.h"

#define SSP_FIRMWARE_REVISION_STM	13061300 /*MCU L5, 6500*/

/* Bootload mode cmd */
#define BL_FW_NAME			"ssp_b.fw"
#define BL_UMS_FW_NAME			"ssp_b.bin"
#define BL_CRASHED_FW_NAME		"ssp_crashed.fw"

#define BL_UMS_FW_PATH			255

#define APP_SLAVE_ADDR			0x18
#define BOOTLOADER_SLAVE_ADDR		0x26

/* Bootloader mode status */
#define BL_WAITING_BOOTLOAD_CMD		0xc0	/* valid 7 6 bit only */
#define BL_WAITING_FRAME_DATA		0x80	/* valid 7 6 bit only */
#define BL_FRAME_CRC_CHECK		0x02
#define BL_FRAME_CRC_FAIL		0x03
#define BL_FRAME_CRC_PASS		0x04
#define BL_APP_CRC_FAIL			0x40	/* valid 7 8 bit only */
#define BL_BOOT_STATUS_MASK		0x3f

/* Command to unlock bootloader */
#define BL_UNLOCK_CMD_MSB		0xaa
#define BL_UNLOCK_CMD_LSB		0xdc

/* STM */
#define SSP_STM_DEBUG	0

#define SEND_ADDR_LEN	5
#define BL_SPI_SOF 0x5A
#define BL_ACK  0x79
#define BL_NACK 0x1F
#define DEF_ACK_CMD_RETRIES 3000
#define STM_MAX_XFER_SIZE 256
#define STM_MAX_BUFFER_SIZE 260
#define STM_APP_ADDR	0x08000000

#define WMEM_COMMAND           0x31  /* Write Memory command          */
#define GO_COMMAND             0x21  /* GO command                    */
#define EXT_ER_COMMAND         0x44  /* Erase Memory command          */

#define XOR_WMEM_COMMAND       0xCE  /* Write Memory command          */
#define XOR_GO_COMMAND         0xDE  /* GO command                    */
#define XOR_EXT_ER_COMMAND     0xBB  /* Erase Memory command          */

#define EXT_ER_DATA_LEN	3

struct stm32fwu_spi_cmd {
	u8 cmd;
	u8 xor_cmd;
	u8 ack_pad; /* Send this when waiting for an ACK */
	u8 reserved;
	int status; /* ACK or NACK (or error) */
	int timeout; /* This is number of retries */
	int ack_loops;
};

unsigned int get_module_rev(struct ssp_data *data)
{
	return SSP_FIRMWARE_REVISION_STM;
}

void stm32_set_ss(unsigned int gpio, int enable)
{
	mdelay(1);
	gpio_set_value(gpio, enable);
	mdelay(1);
}

static int stm32fwu_spi_wait_for_ack(struct spi_device *spi,
	struct stm32fwu_spi_cmd *cmd, u8 dummy_bytes)
{
	struct spi_message m;
	char tx_buf = 0x0;
	char rx_buf = 0x0;
	struct spi_transfer	t = {
		.tx_buf		= &tx_buf,
		.rx_buf		= &rx_buf,
		.len		= 1,
		.bits_per_word = 8,
	};
	int i = 0;
	int ret;

#if SSP_STM_DEBUG
	pr_info("[SSP] %s: dummy byte = 0x%02hhx\n",
		__func__, dummy_bytes);
#endif
	while (i < cmd->timeout) {
		tx_buf = dummy_bytes;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);

		ret = spi_sync(spi, &m);
		if ((rx_buf == BL_ACK) || (rx_buf == BL_NACK)) {
			cmd->ack_loops = i;
			usleep_range(1000, 1100);
			return (int)rx_buf;
		}
		usleep_range(1000, 1100);
		i++;
	}
#if SSP_STM_DEBUG
	dev_err(&spi->dev, "%s: Timeout after %d loops\n",
		__func__, cmd->timeout);
#endif
	return -EIO;
}

static int stm32fwu_spi_send_cmd(struct spi_device *spi,
				 struct stm32fwu_spi_cmd *cmd)
{
	u8 tx_buf[3] = {0,};
	u8 rx_buf[3] = {0,};
	u8 dummy_byte = 0;
	struct spi_message m;
	struct spi_transfer	t = {
		.tx_buf		= tx_buf,
		.rx_buf		= rx_buf,
		.len		= 3,
		.bits_per_word = 8,
	};
	int ret;

	spi_message_init(&m);

	tx_buf[0] = BL_SPI_SOF;
	tx_buf[1] = cmd->cmd;
	tx_buf[2] = cmd->xor_cmd;

	spi_message_add_tail(&t, &m);

#if SSP_STM_DEBUG
	pr_info("[SSP] tx buffer for cmd is: 0x%02hx 0x%02hx 0x%02hx\n",
	       tx_buf[0], tx_buf[1], tx_buf[2]);
#endif
	ret = spi_sync(spi, &m);

	if ((rx_buf[0] == BL_ACK) || (rx_buf[0] == BL_NACK)) {
#if SSP_STM_DEBUG
		pr_info("Found Cmd Ack/NAK (0x%02hhX on first read\n",
			rx_buf[0]);
#endif
		cmd->ack_loops = 0;
		return (int)rx_buf[0];
	}

	usleep_range(5000, 5500);
	dummy_byte = cmd->ack_pad;

	/* check for ack/nack and loop until found */
	ret = stm32fwu_spi_wait_for_ack(spi, cmd, dummy_byte);
	cmd->status = ret;

	if (ret != BL_ACK) {
		pr_err("[SSP] %s: Got NAK or Error %d\n", __func__, ret);
		return ret;
	}
#if SSP_STM_DEBUG
	pr_info("[SSP] Got 0x%02hX for ack after sending cmd 0x%02hX\n",
		ret, cmd->cmd);
#endif

	return ret;
}

static int stm32fwu_spi_write(struct spi_device *spi,
	const u8 *buffer, ssize_t len)
{
	int ret;
	u8 rx_buf[STM_MAX_BUFFER_SIZE] = {0,};
	struct spi_message m;
	struct spi_transfer	t = {
		.tx_buf		= buffer,
		.rx_buf		= rx_buf,
		.len		= len,
		.bits_per_word = 8,
	};

	if (len > STM_MAX_BUFFER_SIZE) {
		pr_err("[SSP] Can't send more than 256 bytes at a time\n");
		return -EINVAL;
	}
#if SSP_STM_DEBUG
	pr_info("[SSP] Writing %d bytes from buffer @ %p\n", len, buffer);
#endif
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(spi, &m);
#if SSP_STM_DEBUG
	pr_info("[SSP] %s: spi_sync() returned %d\n", __func__, ret);
#endif

	if (ret < 0) {
		pr_err("[SSP] Error in %d spi_read()\n", ret);
		return ret;
	}

	return len;
}

static int send_addr(struct spi_device *spi, u32 fw_addr, int send_short)
{
	int res;
	int i = send_short;
	int len = SEND_ADDR_LEN - send_short;
	u8 header[SEND_ADDR_LEN];
	struct stm32fwu_spi_cmd dummy_cmd;
	dummy_cmd.timeout = 1000;


	header[0] = (u8)((fw_addr >> 24) & 0xFF);
	header[1] = (u8)((fw_addr >> 16) & 0xFF);
	header[2] = (u8)((fw_addr >> 8) & 0xFF);
	header[3] = (u8)(fw_addr & 0xFF);
	header[4] = header[0] ^ header[1] ^ header[2] ^ header[3];
	if ((i < 0) || (i > 1)) {
		pr_err("[SSP] Send short flag must be 0 or 1.\n");
		return -EINVAL;
	    }

	res = stm32fwu_spi_write(spi, &header[i], len);

	if (res <  len) {
		pr_err("[SSP] Error in sending address. Res  %d\n", res);
		return ((res > 0) ? -EIO : res);
	}
#if SSP_STM_DEBUG
	pr_info("[SSP] send_addr(): wrote addr OK\n");
#endif
	res = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, 0x79);
	if (res != BL_ACK) {
		pr_err("[SSP] send_addr(): rcv_ack returned 0x%x\n",
			res);
		return res;
	}
	return 0;
}

static int fw_write_stm(struct spi_device *spi, u32 fw_addr,
	int len, const u8 *buffer)
{
	int res;
	struct stm32fwu_spi_cmd cmd;
	struct stm32fwu_spi_cmd dummy_cmd;
	int i;
	u8 xor = 0;
	u8 send_buff[STM_MAX_BUFFER_SIZE] = {0,};

	cmd.cmd = WMEM_COMMAND;
	cmd.xor_cmd = XOR_WMEM_COMMAND;
	cmd.timeout = 1000;
	cmd.ack_pad = (u8)((fw_addr >> 24) & 0xFF);
	dummy_cmd.timeout = 1000;

#if SSP_STM_DEBUG
	pr_info("[SSP] sending WMEM_COMMAND\n");
#endif

	if (len > STM_MAX_XFER_SIZE) {
		pr_err("[SSP] Can't send more than 256 bytes per "
			"transaction\n");
		return -EINVAL;
	}

	send_buff[0] = len - 1;
	memcpy(&send_buff[1], buffer, len);
	for (i = 0; i < (len + 1); i++)
		xor ^= send_buff[i];

	send_buff[len + 1] = xor;

	res = stm32fwu_spi_send_cmd(spi, &cmd);
	if (res != BL_ACK) {
		pr_err("[SSP] Error %d sending read_mem cmd\n", res);
		return res;
	}

	if (cmd.ack_loops > 0)
		res = send_addr(spi, fw_addr, 1);
	else
		res = send_addr(spi, fw_addr, 0);

	if (res != 0) {
		pr_err("[SSP] Error %d sending write_mem Address\n", res);
		return res;
	}

	res = stm32fwu_spi_write(spi, send_buff, len + 2);
	if (res < len) {
		pr_err("[SSP] Error writing to flash. res = %d\n", res);
		return ((res > 0) ? -EIO : res);
	}

	for (i = 0; i < 5; i++) {
		res = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, 0x79);
		if (res == BL_ACK)
			return len;

		if (res == BL_NACK) {
			pr_err("[SSP] Got NAK waiting for WRITE_MEM "
				"to complete\n");
			return res;
		}
	}
	pr_err("[SSP] timeout waiting for ACK for WRITE_MEM command\n");

	return -ETIME;
}

static int load_ums_fw_bootmode(struct spi_device *spi, const char *pFn)
{
	const u8 *buff = NULL;
	int remaining;
	char fw_path[BL_UMS_FW_PATH+1];
	unsigned int uPos = 0;
	unsigned int fw_addr = STM_APP_ADDR;
	int iRet = SUCCESS;
	int block = STM_MAX_XFER_SIZE;
	int count = 0;
	int err_count = 0;
	int retry_count = 0;
	unsigned int uFSize = 0, uNRead = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs = get_fs();

	pr_info("[SSP] ssp_load_ums_fw start!!!\n");
	set_fs(get_ds());

	snprintf(fw_path, BL_UMS_FW_PATH, "/sdcard/ssp/%s", pFn);

	fp = filp_open(fw_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		iRet = ERROR;
		pr_err("file %s open error:%d\n", fw_path, (s32)fp);
		goto err_open;
	}

	uFSize = (unsigned int)fp->f_path.dentry->d_inode->i_size;
	pr_info("ssp_load_ums firmware size: %u\n", uFSize);

	buff = kzalloc((size_t)uFSize, GFP_KERNEL);
	if (!buff) {
		iRet = ERROR;
		pr_err("fail to alloc buffer for fw\n");
		goto err_alloc;
	}

	uNRead = (unsigned int)vfs_read(fp, (char __user *)buff,
			(unsigned int)uFSize, &fp->f_pos);
	if (uNRead != uFSize) {
		iRet = ERROR;
		pr_err("fail to read file %s (nread = %u)\n", fw_path, uNRead);
		goto err_fw_size;
	}

	remaining = (int)uFSize;
	while (remaining > 0) {
		if (block > remaining)
			block = remaining;

		while (retry_count < 3) {
			iRet = fw_write_stm(spi, fw_addr, block, buff + uPos);
			if (iRet < block) {
				pr_err("[SSP] Error writing to addr 0x%08X\n",
					fw_addr);
				if (iRet < 0) {
					pr_err("[SSP] Erro was %d\n", iRet);
				} else {
					pr_err("[SSP] Incomplete write of %d "
						"bytes\n", iRet);
					iRet = -EIO;
				}
				retry_count++;
				err_count++;
			} else {
				retry_count = 0;
				break;
			}
		}
		if (iRet < 0)
			pr_err("[SSP] Writing MEM failed: %d, retry cont: %d\n",
				iRet, err_count);

		remaining -= block;
		uPos += block;
		fw_addr += block;
		if (count++ == 20) {
			pr_info("[SSP] Updated %u bytes / %u bytes\n", uPos,
				uFSize);
			count = 0;
		}
	}

	pr_info("[SSP] Firmware download is success.(%d bytes, retry %d)\n",
		uPos, err_count);

out:
err_fw_size:
	kfree(buff);
err_alloc:
	filp_close(fp, NULL);
err_open:
	set_fs(old_fs);
	return iRet;
}

static int fw_erase_stm(struct spi_device *spi)
{
	struct stm32fwu_spi_cmd cmd;
	struct stm32fwu_spi_cmd dummy_cmd;
	int ret;
	char buff[EXT_ER_DATA_LEN] = {0xff, 0xff, 0x00};

	cmd.cmd = EXT_ER_COMMAND;
	cmd.xor_cmd = XOR_EXT_ER_COMMAND;
	cmd.timeout = 1000;
	cmd.ack_pad = 0xFF;
	dummy_cmd.timeout = DEF_ACK_CMD_RETRIES;

	ret = stm32fwu_spi_send_cmd(spi, &cmd);

	if (ret < 0 || ret != BL_ACK) {
		pr_err("[SSP] fw_erase failed\n");
		return -EIO;
	}
	if (cmd.ack_loops == 0)
		ret = stm32fwu_spi_write(spi, buff, EXT_ER_DATA_LEN);
	else
		ret = stm32fwu_spi_write(spi, buff, EXT_ER_DATA_LEN-1);

#if SSP_STM_DEBUG
	pr_info("[SSP] %s: stm32fwu_spi_write(sync byte) returned %d\n",
		__func__, ret);
#endif

	ret = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, 0x79);

#if SSP_STM_DEBUG
	pr_info("[SSP] %s: stm32fwu_spi_wait_for_ack returned %d (0x%x)\n",
		__func__, ret, ret);
#endif

	return ret;

}
static int load_kernel_fw_bootmode(struct spi_device *spi, const char *pFn)
{
	const struct firmware *fw = NULL;
	int remaining;
	unsigned int uPos = 0;
	unsigned int fw_addr = STM_APP_ADDR;
	int iRet = SUCCESS;
	int block = STM_MAX_XFER_SIZE;
	int count = 0;
	int err_count = 0;
	int retry_count = 0;

	pr_info("[SSP] ssp_load_fw start!!!\n");

	iRet = request_firmware(&fw, pFn, &spi->dev);
	if (iRet) {
		pr_err("[SSP] Unable to open firmware %s\n", pFn);
		return iRet;
	}

	remaining = fw->size;
	while (remaining > 0) {
		if (block > remaining)
			block = remaining;

		while (retry_count < 3) {
			iRet = fw_write_stm(spi, fw_addr, block,
				fw->data + uPos);
			if (iRet < block) {
				pr_err("[SSP] Error writing to addr 0x%08X\n",
					fw_addr);
				if (iRet < 0) {
					pr_err("[SSP] Erro was %d\n", iRet);
				} else {
					pr_err("[SSP] Incomplete write of %d "
						"bytes\n", iRet);
					iRet = -EIO;
				}
				retry_count++;
				err_count++;
			} else {
				retry_count = 0;
				break;
			}
		}
		if (iRet < 0) {
			pr_err("[SSP] Writing MEM failed: %d, retry cont: %d\n",
				iRet, err_count);
		}
		remaining -= block;
		uPos += block;
		fw_addr += block;
		if (count++ == 20) {
			pr_info("[SSP] Updated %u bytes / %u bytes\n", uPos,
				fw->size);
			count = 0;
		}
	}

	pr_info("[SSP] Firmware download is success.(%d bytes, retry %d)\n",
		uPos, err_count);

out:
	release_firmware(fw);
	return iRet;
}

static void change_to_bootmode(struct ssp_data *data)
{
	int iCnt;
	int ret;
	char syncb = BL_SPI_SOF;
	struct stm32fwu_spi_cmd dummy_cmd;

	dummy_cmd.timeout = DEF_ACK_CMD_RETRIES;

	data->set_mcu_reset(0);
	mdelay(4);
	data->set_mcu_reset(1);
	usleep_range(45000, 47000);

	for (iCnt = 0; iCnt < 9; iCnt++) {
		data->set_mcu_reset(0);
		usleep_range(4000, 4200);
		data->set_mcu_reset(1);
		usleep_range(15000, 15500);
	}

	ret = stm32fwu_spi_write(data->spi, &syncb, 1);
#if SSP_STM_DEBUG
	pr_info("[SSP] stm32fwu_spi_write(sync byte) returned %d\n", ret);
#endif
	ret = stm32fwu_spi_wait_for_ack(data->spi, &dummy_cmd, 0x79);
#if SSP_STM_DEBUG
	pr_info("[SSP] stm32fwu_spi_wait_for_ack returned %d (0x%x)\n",
		ret, ret);
#endif
}

void toggle_mcu_reset(struct ssp_data *data)
{
	data->set_mcu_reset(0);
	usleep_range(1000, 1200);
	data->set_mcu_reset(1);
	msleep(50);
}

static int update_mcu_bin(struct ssp_data *data, int iBinType)
{
	int iRet = SUCCESS;
	struct stm32fwu_spi_cmd cmd;

	cmd.cmd = GO_COMMAND;
	cmd.xor_cmd = XOR_GO_COMMAND;
	cmd.timeout = 1000;
	cmd.ack_pad = (u8)((STM_APP_ADDR >> 24) & 0xFF);

	pr_info("[SSP] ssp_change_to_bootmode\n");

	change_to_bootmode(data);

	iRet = fw_erase_stm(data->spi);
	if (iRet < 0) {
		pr_err("[SSP]: %s - fw_erase_stm %d\n",
			__func__, iRet);
		return iRet;
	}

	/* After erase put bootmode again: debug */
	change_to_bootmode(data);

	switch (iBinType) {
	case KERNEL_BINARY:
		iRet = load_kernel_fw_bootmode(data->spi,
			BL_FW_NAME);
		break;

	case KERNEL_CRASHED_BINARY:
		iRet = load_kernel_fw_bootmode(data->spi,
			BL_CRASHED_FW_NAME);
		break;
	case UMS_BINARY:
		iRet = load_ums_fw_bootmode(data->spi,
			BL_UMS_FW_NAME);
		break;

	default:
		pr_err("[SSP] binary type error!!\n");
	}

	/* STM : GO USER ADDR */
	stm32fwu_spi_send_cmd(data->spi, &cmd);
	if (cmd.ack_loops > 0)
		send_addr(data->spi, STM_APP_ADDR, 1);
	else
		send_addr(data->spi, STM_APP_ADDR, 0);

	msleep(SSP_SW_RESET_TIME);

	return iRet;
}

int forced_to_download_binary(struct ssp_data *data, int iBinType)
{
	int iRet = 0;
	int retry = 3;

	ssp_dbg("[SSP]: %s - mcu binany update!\n", __func__);

	ssp_enable(data, false);

	data->fw_dl_state = FW_DL_STATE_DOWNLOADING;
	pr_info("[SSP] %s, DL state = %d\n", __func__,
	data->fw_dl_state);
	do {
		pr_info("[SSP] %d try\n", 3 - retry);
		iRet = update_mcu_bin(data, iBinType);
	} while (retry -- > 0 && iRet < 0);

	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - update_mcu_bin failed!\n", __func__);
		goto out;
	}

	data->fw_dl_state = FW_DL_STATE_SYNC;
	pr_info("[SSP] %s, DL state = %d\n", __func__, data->fw_dl_state);
	iRet = initialize_mcu(data);
	if (iRet < 0) {
		iRet = ERROR;
		ssp_dbg("[SSP]: %s - initialize_mcu failed!\n", __func__);
		goto out;
	}

	ssp_enable(data, true);
	sync_sensor_state(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif

	data->fw_dl_state = FW_DL_STATE_DONE;
	pr_info("[SSP] %s, DL state = %d\n", __func__, data->fw_dl_state);

	iRet = SUCCESS;
out:
	return iRet;
}

static unsigned int check_firmware_rev(struct ssp_data *data)
{
	char chTxData = MSG2SSP_AP_FIRMWARE_REV;
	char chRxBuf[3] = {0,};
	unsigned int uRev = SSP_INVALID_REVISION;
	int iRet;

	iRet = ssp_read_data(data, &chTxData, 1, chRxBuf, 3, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - spi fail %d\n", __func__, iRet);
	} else
		uRev = ((unsigned int)chRxBuf[0] << 16)
			| ((unsigned int)chRxBuf[1] << 8) | chRxBuf[2];
	return uRev;
}

int check_fwbl(struct ssp_data *data)
{
	unsigned int fw_revision;

	fw_revision = SSP_FIRMWARE_REVISION_STM;
	data->uCurFirmRev = check_firmware_rev(data);

	if (data->uCurFirmRev == SSP_INVALID_REVISION) {
		pr_err("[SSP] SSP_INVALID_REVISION\n");
		return FW_DL_STATE_NEED_TO_SCHEDULE;

	} else {
		pr_info("[SSP] MCU Firm Rev : Old = %8u, New = %8u\n",
			data->uCurFirmRev, fw_revision);

		if (data->uCurFirmRev != fw_revision)
			return FW_DL_STATE_NEED_TO_SCHEDULE;
	}

	return FW_DL_STATE_NONE;
}
