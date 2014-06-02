/*
 *  drivers/input/touchscreen/synaptics_sysfs.c
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/synaptics_s7301.h>
#include "synaptics_sysfs.h"

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
struct device *synaptics_with_gpio_led_device;
#endif

const char *sec_sysfs_cmd_list[] = {
	"fw_update",
	"get_fw_ver_bin",
	"get_fw_ver_ic",
	"get_config_ver",
	"get_threshold",
	"module_off_master",
	"module_on_master",
	"get_chip_vendor",
	"get_chip_name",
	"get_x_num",
	"get_y_num",
	"run_rawcap_read",
	"run_rx_to_rx_read",
	"run_tx_to_tx_read",
	"run_tx_to_gnd_read",
	"get_rawcap",
	"get_rx_to_rx",
	"get_tx_to_tx",
	"get_tx_to_gnd"
};

static int synaptics_ts_load_fw(struct synaptics_drv_data *data)
{
	struct file *fp;
	mm_segment_t old_fs;
	u16 fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

#if defined(CONFIG_MACH_KONA)
	fp = filp_open(SYNAPTICS_FW2, O_RDONLY, S_IRUSR);
#else
	fp = filp_open(SYNAPTICS_FW, O_RDONLY, S_IRUSR);
#endif
	if (IS_ERR(fp)) {
		printk(KERN_ERR "[TSP] failed to open %s.\n", SYNAPTICS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;
	if (0 < fw_size) {
		u8 *fw_data;
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
			fw_size, &fp->f_pos);
#if defined(CONFIG_MACH_KONA)
        printk(KERN_DEBUG "[TSP] start, file path %s, size %u Bytes\n",
		       SYNAPTICS_FW2, fw_size);
#else
		printk(KERN_DEBUG "[TSP] start, file path %s, size %u Bytes\n",
		       SYNAPTICS_FW, fw_size);
#endif

		if (nread != fw_size) {
			printk(KERN_ERR
			       "[TSP] failed to read firmware file, nread %u Bytes\n",
			       nread);
			error = -EIO;
		} else
			synaptics_fw_updater(data, fw_data);

		kfree(fw_data);
	}

	filp_close(fp, current->files);
 open_err:
	set_fs(old_fs);
	return error;
}

static int set_report_type(struct synaptics_drv_data *data, u8 command)
{
	return synaptics_ts_write_data(data,
			data->f54.data_base_addr,
			command);
}

static int set_report_index(struct synaptics_drv_data *data, u16 index)
{
	u8 buf[2];
	buf[0] = index & 0xff;
	buf[1] = (index & 0xff00) >> 8;
	return synaptics_ts_write_block(data,
			data->f54.data_base_addr + 1,
			buf, 2);
}

static void set_report_mode(struct synaptics_drv_data *data,
	u8 command, u8 result)
{
	u8 buf, cnt = 0;

	synaptics_ts_write_data(data,
		data->f54.command_base_addr,
		command);

	/* Wait until the command is completed */
	do {
		msleep(20);
		synaptics_ts_read_data(data,
			data->f54.command_base_addr,
			&buf);
		if (cnt++ > 150) {
			printk(KERN_WARNING
				"[TSP] Fail - cmd : %u, result : %u\n",
				command, result);
			break;
		}

	} while (buf != result);
}

static void soft_reset(struct synaptics_drv_data *data)
{
	u8 buf;
	synaptics_ts_write_data(data, 0xff, 0x00);
	synaptics_ts_write_data(data,
		data->f01.command_base_addr,
		0x01);

	msleep(160);

	/* Read Interrupt status register to Interrupt line goes to high */
	synaptics_ts_read_data(data,
		data->f01.data_base_addr + 1,
		&buf);
}

static void check_all_raw_cap(struct synaptics_drv_data *data)
{
	int i, j, k=0;
	u16 temp = 0;
	u16 length;

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	u8 escape_rx_line;

	if (data->pdata->support_extend_button)
		escape_rx_line = data->pdata->extend_button_map->nbuttons;
	else
		escape_rx_line = data->pdata->button_map->nbuttons;

	length = data->x_line * (data->y_line + escape_rx_line) * 2;

	if (NULL == data->references)
		data->references = kzalloc(length, GFP_KERNEL);

	data->refer_min = 0xffff;
	data->refer_max = 0x0;

	/* set the index */
	set_report_index(data, 0x0000);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	/* read all report data */
	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		data->references, length);

	for (i = 0; i < data->x_line; i++) {
		for (j = 0; j < data->y_line + escape_rx_line; j++) {
			temp = (u16)(data->references[k] |
				(data->references[k+1] << 8));

			if (k != 0 && j !=0) {
				if (j >= data->y_line) {
					if (data->debug) {
						printk(KERN_DEBUG
							"[TSP][skip] raw cap[%d] : %u\n",
							k, temp);
					}
					k += 2;
					continue;
				}
			}
			if (data->debug) {
				if (data->debug) {
					printk(KERN_DEBUG
						"[TSP] raw cap[%d] : %u\n",
						k, temp);
				}
			}
			if (temp < data->refer_min)
				data->refer_min = temp;
			if (temp > data->refer_max)
				data->refer_max = temp;
			k += 2;
		}
	}
#else
	length = data->x_line * data->y_line * 2;

	if (NULL == data->references)
		data->references = kzalloc(length, GFP_KERNEL);

	data->refer_min = 0xffff;
	data->refer_max = 0x0;

	/* set the index */
	set_report_index(data, 0x0000);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	/* read all report data */
	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		data->references, length);

	for (i = 0; i < length; i += 2) {
		temp = (u16)(data->references[i] |
			(data->references[i+1] << 8));
		if (data->debug) {
			if ((temp <= FULL_RAW_CAP_LOWER_LIMIT)
				|| (temp >= FULL_RAW_CAP_UPPER_LIMIT)) {
				printk(KERN_DEBUG
					"[TSP] raw cap[%d] : %u\n",
					i, temp);
			}
		}
		if (temp < data->refer_min)
			data->refer_min = temp;
		if (temp > data->refer_max)
			data->refer_max = temp;
	}
#endif
	printk(KERN_DEBUG "[TSP] min : %u, max : %u\n",
		data->refer_min, data->refer_max);
}

static void check_tx_to_tx(struct synaptics_drv_data *data)
{
	int i = 0;
	u8 length = (data->x_line / 8) + 1;

	if (NULL == data->tx_to_tx)
		data->tx_to_tx = kzalloc(data->x_line, GFP_KERNEL);

	/* set the index */
	set_report_index(data, 0x0000);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		data->tx_to_tx, length);

	/*
	* Byte-0 houses Tx responses Tx7:Tx0
	* Byte-1 houses Tx responses Tx15:Tx8
	* Byte-2 houses Tx responses Tx23:Tx16
	* Byte-3 houses Tx responses Tx31:Tx24
	*/
	for (i = 0; i < data->x_line; i++) {
		if (data->tx_to_tx[i / 8] & (0x1 << i % 8)) {
			data->tx_to_tx[i] = 0x1;
			printk(KERN_WARNING
				"[TSP] %s %d short\n",
				__func__, i);
		} else
			data->tx_to_tx[i] = 0x0;
	}
}

static void check_tx_to_gnd(struct synaptics_drv_data *data)
{
	int i = 0;
	u8 length = (data->x_line / 8) + 1;

	if (NULL == data->tx_to_gnd)
		data->tx_to_gnd = kzalloc(data->x_line, GFP_KERNEL);

	/* set the index */
	set_report_index(data, 0x0000);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		data->tx_to_gnd, length);

	/*
	* Byte-0 houses Tx responses Tx7:Tx0
	* Byte-1 houses Tx responses Tx15:Tx8
	* Byte-2 houses Tx responses Tx23:Tx16
	* Byte-3 houses Tx responses Tx31:Tx24
	*/
	for (i = 0; i < data->x_line; i++) {
		if (data->tx_to_gnd[i / 8] & (0x1 << i % 8)) {
			data->tx_to_gnd[i] = 0x1;
			printk(KERN_WARNING
				"[TSP] %s %d short\n",
				__func__, i);
		} else
			data->tx_to_gnd[i] = 0x0;
	}
}

static void check_rx_to_rx(struct synaptics_drv_data *data)
{
	int i = 0, j = 0, k = 0;
	u8 *buff;
	u16 length = data->y_line * data->y_line * 2;
	u16 temp = 0;

	buff = kzalloc(length, GFP_KERNEL);

	/* disable the CBC setting */
	synaptics_ts_write_data(data,
		data->f54.control_base_addr + 8,
		0x00);

	/* noCDM4 */
	synaptics_ts_write_data(data,
		data->f54.control_base_addr + 0xa6,
		0x01);

	set_report_mode(data, 0x04, 0x00);
	set_report_mode(data, 0x02, 0x00);

	/* set the index */
	set_report_index(data, 0x0000);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	/* read 1st rx_to_rx data */
	length = data->x_line * data->y_line * 2;

	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		buff, length);

	for (i = 0, k = 0; i < data->x_line; i++) {
		for (j = 0; j < data->y_line; j++, k += 2) {
			temp = buff[k] | (buff[k+1] << 8);
			data->rx_to_rx[i][j] = temp;
		}
	}

	/* read 2nd rx_to_rx data */
	length = data->y_line *
		(data->y_line - data->x_line) * 2;

	set_report_type(data,
		REPORT_TYPE_RX_TO_RX2);

	/* set the index */
	set_report_index(data, 0x0000);

	set_report_mode(data, 0x01, 0x00);

	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		buff, length);

	for (k = 0; i < data->y_line; i++) {
		for (j = 0; j < data->y_line; j++, k += 2) {
			temp = buff[k] | (buff[k+1] << 8);
			data->rx_to_rx[i][j] = temp;
		}
	}

	if (data->debug) {
		for (i = 0; i < data->y_line; i++) {
			printk("[TSP] line %d :", i);
			for (j = 0; j < data->y_line; j++)
				printk(" %d", (s16)(data->rx_to_rx[j][i]));
			printk("\n");
		}
	}

	/* to the calibration */
	set_report_mode(data, 0x02, 0x00);

	kfree(buff);
}

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
static void check_delta_cap(struct synaptics_drv_data *data)
{
	int i, k=0;
	u16 temp = 0;
	u16 length;
	u8 escape_rx_line;
    u8 *btn_data;
	int start_button_data;

	if (data->pdata->support_extend_button)
		escape_rx_line = data->pdata->extend_button_map->nbuttons;
	else
		escape_rx_line = data->pdata->button_map->nbuttons;

	length = escape_rx_line * 2;

    btn_data = kzalloc(length, GFP_KERNEL);

	data->refer_min = 0xffff;
	data->refer_max = 0x0;

	start_button_data = ((data->x_line * (data->y_line + escape_rx_line)) + data->y_line) * 2;

	/* set the index */
	set_report_index(data, start_button_data);

	/* Set the GetReport bit to run the AutoScan */
	set_report_mode(data, 0x01, 0x00);

	/* read all report data */
	synaptics_ts_read_block(data,
		data->f54.data_base_addr + 3,
		btn_data, length);

	for (i = 0; i < escape_rx_line; i++) {
		temp = (u16)(btn_data[k] | (btn_data[k+1] << 8));
		printk(KERN_DEBUG "[TSP] index[btn:%d] data[0x%x]\n", i, temp);

		if (temp > BUTTON_THRESHOLD_LIMIT)
			data->pdata->button_pressure[i] = BUTTON_THRESHOLD_MIN;
		else
			data->pdata->button_pressure[i] = temp;
		k = k + 2;
	}

    kfree(btn_data);
}
#endif

static void check_diagnostics_mode(struct synaptics_drv_data *data)
{
	/* Set report mode */
	set_report_type(data, data->cmd_report_type);

	switch (data->cmd_report_type) {
	case REPORT_TYPE_RAW_CAP:
		check_all_raw_cap(data);
		break;

	case REPORT_TYPE_TX_TO_TX:
		check_tx_to_tx(data);
		break;
	case REPORT_TYPE_TX_TO_GND:
		check_tx_to_gnd(data);
		break;

	case REPORT_TYPE_RX_TO_RX:
		/* check the result */
		check_rx_to_rx(data);
		break;

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	case REPORT_TYPE_DELTA_CAP:
		check_delta_cap(data);
		break;
#endif
		
	default:
		break;
	}

	/* Reset */
	soft_reset(data);
}

static u16 synaptics_get_threshold(struct synaptics_drv_data *data)
{
	u8 tmp = 0;
	synaptics_ts_read_data(data,
		data->f11.control_base_addr + 10,
		&tmp);
	printk(KERN_DEBUG "[TSP] threshold : %u\n", tmp);
	return (u16)tmp;
}

static void synaptics_fw_phone(struct synaptics_drv_data *data,
	u8 *buf)
{
	strncpy(buf, data->firm_version,
		sizeof(data->firm_version));
	printk(KERN_DEBUG "[TSP] firm phone : %s\n",
		data->firm_version);
}

static void synaptics_fw_panel(struct synaptics_drv_data *data,
	u8 *buf)
{

	synaptics_ts_read_block(data,
		data->f34.control_base_addr,
		buf, 4);

	printk(KERN_DEBUG "[TSP] firm panel : %s\n", buf);
}

static void synaptics_fw_config(struct synaptics_drv_data *data,
	u8 *buf)
{

	strncpy(buf, data->firm_config,
		sizeof(data->firm_config));
	printk(KERN_DEBUG "[TSP] config : %s\n",
		data->firm_config);
}

static int sec_fw_cmd(struct synaptics_drv_data *data,
	u32 type)
{
	int ret = 0;
	switch (type) {
	case CMD_FW_CMD_BUILT_IN:
		ret = synaptics_fw_updater(data, NULL);
		break;

	case CMD_FW_CMD_UMS:
		ret = synaptics_ts_load_fw(data);
		break;

	default:
		break;
	}

	return ret;
}

static u16 get_value(struct synaptics_drv_data *data,
	u32 pos_x, u32 pos_y)
{
	u16 tmp = 0;
	u8 escape_rx_line;

	switch (data->cmd_report_type) {
	case REPORT_TYPE_RAW_CAP:
	{
		u16 position;
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
		if (data->pdata->support_extend_button)
			escape_rx_line =
				data->pdata->extend_button_map->nbuttons;
		else
			escape_rx_line =
				data->pdata->button_map->nbuttons;

		position = (u16)((data->y_line +
			escape_rx_line) * pos_x) + pos_y;
#else
		position = (u16)(data->y_line * pos_x) + pos_y;
#endif
		position *= 2;
		tmp = (u16)(data->references[position] |
			(data->references[position+1] << 8));
		break;
	}

	case REPORT_TYPE_TX_TO_TX:
		tmp = data->tx_to_tx[pos_x];
		break;
	case REPORT_TYPE_TX_TO_GND:
		tmp = data->tx_to_gnd[pos_x];
		break;

	case REPORT_TYPE_RX_TO_RX:
		tmp = data->rx_to_rx[pos_x][pos_y];
		break;

	default:
		break;
	}

	return tmp;
}

static int sec_sysfs_check_cmd(u8 *buf,
	u32 *param)
{
	int cmd = 0;
	u8 cnt = 0, cnt2 = 0, start = 0;
	u8 end = strlen(buf);
	do {
		if (!strncmp(sec_sysfs_cmd_list[cmd],
			buf, strlen(sec_sysfs_cmd_list[cmd])))
			break;
		cmd++;
	} while (cmd < CMD_LIST_MAX);

	if (CMD_LIST_MAX == cmd)
		return cmd;

	printk(KERN_DEBUG
		"[TSP] mode : %s\n",
		sec_sysfs_cmd_list[cmd]);

	cnt = start = strlen(sec_sysfs_cmd_list[cmd]) + 1;

	while (cnt < end) {
		if ((buf[cnt] == ',') || cnt == end - 1) {
			u8 *tmp;
			int len = 0;
			len = cnt - start;
			if (cnt == end - 1)
				len++;
			tmp = kzalloc(len, GFP_KERNEL);
			memcpy(tmp, &buf[start], len);
			printk(KERN_DEBUG
				"[TSP] param[%u] : %s\n",
				cnt2, tmp);
			if (kstrtouint(tmp, 10, &param[cnt2]))
				cmd = CMD_LIST_MAX;
			else
				cnt2++;
			kfree(tmp);
			start = cnt + 1;
		}
		cnt++;
	}

	return cmd;
}

static void sec_sysfs_numstr(s16 data, u8 *str)
{
	sprintf(str, "%d", data);
}

static void sec_sysfs_cmd(struct synaptics_drv_data *data,
	const char *str)
{
	int cmd = 0, cnt = 0;
	int buf_size = 0, i = 0;
	int ret = 0;
	int irq = gpio_to_irq(data->gpio);
	u8 *buf, *buf2, *buf3;
	u8 *tmp_str[7];
	u16 temp = 0;
	u32 param[2] = {0,};

	buf = kzalloc(strlen(str), GFP_KERNEL);
	buf2 = kzalloc(8, GFP_KERNEL);
	buf3 = kzalloc(8, GFP_KERNEL);
	memset(data->cmd_result, 0x0, sizeof(data->cmd_result));

	sscanf(str, "%s", buf);
	cmd = sec_sysfs_check_cmd(buf, param);

	tmp_str[cnt++] = buf;
	tmp_str[cnt++] = ":";

	printk(KERN_DEBUG
		"[TSP] %s : %u, %u\n",
		__func__,
		param[0], param[1]);

	if (CMD_STATUS_WAITING == data->cmd_status)
		data->cmd_status = CMD_STATUS_RUNNING;
	else
		data->cmd_status = CMD_STATUS_WAITING;

	disable_irq(irq);

	switch (cmd) {
	case CMD_LIST_FW_UPDATE:
		ret = sec_fw_cmd(data, param[0]);
		if (ret)
			tmp_str[cnt++] = "FAIL";
		else
			tmp_str[cnt++] = "PASS";
		break;

	case CMD_LIST_FW_VER_BIN:
		synaptics_fw_phone(data, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_FW_VER_IC:
		synaptics_fw_panel(data, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_CONFIG_VER:
		synaptics_fw_config(data, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_GET_THRESHOLD:
		temp = synaptics_get_threshold(data);
		sec_sysfs_numstr(temp, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_POWER_OFF:
		ret = data->pdata->set_power(false);
		if (ret)
			tmp_str[cnt++] = "FAIL";
		else
			tmp_str[cnt++] = "PASS";
		break;

	case CMD_LIST_POWER_ON:
		ret = data->pdata->set_power(true);
		if (ret)
			tmp_str[cnt++] = "FAIL";
		else
			tmp_str[cnt++] = "PASS";
		break;

	case CMD_LIST_VENDOR:
		tmp_str[cnt++] = "Synaptics";
		break;

	case CMD_LIST_IC_NAME:
		tmp_str[cnt++] = "S7301";
		break;

	case CMD_LIST_X_SIZE:
		sec_sysfs_numstr(data->x_line, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_Y_SIZE:
		sec_sysfs_numstr(data->y_line, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_READ_REF:
		synaptics_ts_write_data(data,
			0xf0, 0x01);
		data->cmd_report_type = REPORT_TYPE_RAW_CAP;
		check_diagnostics_mode(data);
		synaptics_ts_write_data(data,
			0xf0, 0x00);
		sec_sysfs_numstr(data->refer_min, buf2);
		tmp_str[cnt++] = buf2;
		tmp_str[cnt++] = ",";
		sec_sysfs_numstr(data->refer_max, buf3);
		tmp_str[cnt++] = buf3;
		break;

	case CMD_LIST_READ_RX:
		data->cmd_report_type = REPORT_TYPE_RX_TO_RX;
		check_diagnostics_mode(data);
		break;

	case CMD_LIST_READ_TX:
		data->cmd_report_type = REPORT_TYPE_TX_TO_TX;
		check_diagnostics_mode(data);
		break;

	case CMD_LIST_READ_TXG:
		data->cmd_report_type = REPORT_TYPE_TX_TO_GND;
		check_diagnostics_mode(data);
		break;

	case CMD_LIST_GET_REF:
		temp = get_value(data, param[0], param[1]);
		sec_sysfs_numstr(temp, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_GET_RX:
		temp = get_value(data, param[0], param[1]);
		sec_sysfs_numstr(temp, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_GET_TX:
		temp = get_value(data, param[0], param[1]);
		sec_sysfs_numstr(temp, buf2);
		tmp_str[cnt++] = buf2;
		break;

	case CMD_LIST_GET_TXG:
		temp = get_value(data, param[0], param[1]);
		sec_sysfs_numstr(temp, buf2);
		tmp_str[cnt++] = buf2;
		break;

	default:
		printk(KERN_DEBUG
			"[TSP] unkown mode : %s\n", buf);
		break;
	}

	enable_irq(irq);

	for (i = 0; i < cnt; i++) {
		if (buf_size < MAX_CMD_SIZE) {
			memcpy(&data->cmd_result[buf_size],
				tmp_str[i], strlen(tmp_str[i]));
			buf_size += strlen(tmp_str[i]);
		} else
			break;
	}

	if (cmd == CMD_LIST_MAX)
		data->cmd_status = CMD_STATUS_FAIL;
	else
		data->cmd_status = CMD_STATUS_OK;

	kfree(buf);
	kfree(buf2);
	kfree(buf3);
}

static void sec_sysfs_cmd_status(struct synaptics_drv_data *data)
{
	u8 buf[8] = {0, };
	switch (data->cmd_status) {
	case CMD_STATUS_OK:
		strcpy(buf, "OK");
		break;

	case CMD_STATUS_FAIL:
		strcpy(buf, "FAIL");
		break;

	case CMD_STATUS_WAITING:
		strcpy(buf, "WAITING");
		break;

	case CMD_STATUS_RUNNING:
		strcpy(buf, "RUNNING");
		break;

	default:
		break;
	}
	data->cmd_temp = buf;
	printk(KERN_DEBUG
		"[TSP] unkown mode : %s\n", buf);
}

static ssize_t sec_sysfs_show_cmd_list(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int i = 0, cnt = 0;
	for (i = 0; i < CMD_LIST_MAX; i++)
		cnt += sprintf(buf + cnt,
			"%s\n", sec_sysfs_cmd_list[i]);
	return cnt;
}

void synaptics_ts_drawing_mode(struct synaptics_drv_data *data)
{
	u8 val = 0;
	u16 addr = 0;

	addr = data->f11.control_base_addr;
	synaptics_ts_read_data(data, addr, &val);

	if (!data->drawing_mode) {
		val |= ABS_POS_BIT;
		printk(KERN_DEBUG
			"[TSP] set normal mode\n");
	} else {
		val &= ~(ABS_POS_BIT);
		printk(KERN_DEBUG
			"[TSP] set drawing mode\n");
	}
	/* set ads pos filter */
	synaptics_ts_write_data(data, addr, val);
}

static void set_abs_pos_filter(struct synaptics_drv_data *data,
	const char *str)
{
	u32 buf = 0;
	sscanf(str, "%u", &buf);
	data->drawing_mode = !buf;
	if (data->ready)
		synaptics_ts_drawing_mode(data);
}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void sec_sysfs_set_debug(struct synaptics_drv_data *data,
	const char *str)
{
	u32 buf = 0;
	sscanf(str, "%u", &buf);
	data->debug = !!buf;
}

static void set_config(struct synaptics_drv_data *data,
	const char *str)
{
	u32 buf1, buf2 = 0;
	sscanf(str, "%u %u", &buf1, &buf2);
	synaptics_ts_write_data(data, buf1, buf2);
}
#endif

#define SET_SHOW_FN(name, fn, format, ...)	\
static ssize_t sec_sysfs_show_##name(struct device *dev,	\
				     struct device_attribute *attr,	\
				     char *buf)		\
{	\
	struct synaptics_drv_data *data = dev_get_drvdata(dev);	\
	fn;	\
	return sprintf(buf, format "\n", ## __VA_ARGS__);	\
}

#define SET_STORE_FN(name, fn)		\
static ssize_t sec_sysfs_store_##name(struct device *dev,	\
				     struct device_attribute *attr,	\
				     const char *buf, size_t size)	\
{	\
	struct synaptics_drv_data *data = dev_get_drvdata(dev);	\
	fn(data, buf);	\
	return size;	\
}

SET_SHOW_FN(cmd_status,
	sec_sysfs_cmd_status(data),
	"%s", data->cmd_temp);
SET_SHOW_FN(cmd_result,
	printk(KERN_DEBUG "[TSP] cmd result - %s\n",
		data->cmd_result),
	"%s", data->cmd_result);

SET_STORE_FN(cmd, sec_sysfs_cmd);
SET_STORE_FN(set_jitter, set_abs_pos_filter);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
SET_STORE_FN(set_debug, sec_sysfs_set_debug);
SET_STORE_FN(set_config, set_config);
#endif

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP,
	NULL, sec_sysfs_store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO,
	sec_sysfs_show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO,
	sec_sysfs_show_cmd_result, NULL);
static DEVICE_ATTR(cmd_list, S_IRUGO,
	sec_sysfs_show_cmd_list, NULL);
static DEVICE_ATTR(set_jitter, S_IWUSR | S_IWGRP,
	NULL, sec_sysfs_store_set_jitter);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static DEVICE_ATTR(set_debug, S_IWUSR,
	NULL, sec_sysfs_store_set_debug);
static DEVICE_ATTR(set_config, S_IWUSR,
	NULL, sec_sysfs_store_set_config);
#endif

static struct attribute *sec_sysfs_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_cmd_list.attr,
	&dev_attr_set_jitter.attr,
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	&dev_attr_set_debug.attr,
	&dev_attr_set_config.attr,
#endif
	NULL,
};

static struct attribute_group sec_sysfs_attr_group = {
	.attrs = sec_sysfs_attributes,
};

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
static ssize_t sec_touchkey_sensitivity_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	printk(KERN_INFO "[TSP] do noting!\n");
	return size;
}

static ssize_t sec_touchkey_back_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON4]);
	else
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON2]);
}

static ssize_t sec_touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON2]);
	else
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON1]);
}

static ssize_t sec_touchkey_threshold_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d %d %d %d %d\n",
			BUTTON5_0_THRESHOLD,
			BUTTON5_1_THRESHOLD,
			BUTTON5_2_THRESHOLD,
			BUTTON5_3_THRESHOLD,
			BUTTON5_4_THRESHOLD);
	else
		return sprintf(buf, "%d %d\n",
			BUTTON2_0_THRESHOLD,
			BUTTON2_1_THRESHOLD);
}

static ssize_t sec_touchkey_dummy_btn1_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON1]);
	else {
		printk(KERN_DEBUG "[TSP] dummy btn1 not supported\n");
		return 0;
	}
}

static ssize_t sec_touchkey_dummy_btn3_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON3]);
	else {
		printk(KERN_DEBUG "[TSP] dummy btn3 not supported\n");
		return 0;
	}
}

static ssize_t sec_touchkey_dummy_btn5_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d\n", data->pdata->button_pressure[BUTTON5]);
	else {
		printk(KERN_DEBUG "[TSP] dummy btn5 not supported\n");
		return 0;
	}
}

static ssize_t sec_touchkey_button_all_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int irq = gpio_to_irq(data->gpio);

	disable_irq(irq);
	synaptics_ts_write_data(data, 0xf0, 0x01);
	data->cmd_report_type = REPORT_TYPE_DELTA_CAP;
	check_diagnostics_mode(data);
	synaptics_ts_write_data(data, 0xf0, 0x00);
	enable_irq(irq);

	if (data->pdata->support_extend_button)
		return sprintf(buf, "%d %d %d %d %d\n",
			data->pdata->button_pressure[BUTTON1],
			data->pdata->button_pressure[BUTTON2],
			data->pdata->button_pressure[BUTTON3],
			data->pdata->button_pressure[BUTTON4],
			data->pdata->button_pressure[BUTTON5]);
	else
		return sprintf(buf, "%d %d\n",
			data->pdata->button_pressure[BUTTON1],
			data->pdata->button_pressure[BUTTON2]);
}

static ssize_t sec_touchkey_button_status_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	u8 int_status = 0;
	u8 button_status = 0;

	/* check interrupt status */
	synaptics_ts_read_data(data,
		data->f01.data_base_addr + 1,
		&int_status);

	/* check button status */
	synaptics_ts_read_data(data,
		data->f1a.data_base_addr,
		&button_status);

	return sprintf(buf, "%d %d\n",
		int_status,
		button_status);
}

static ssize_t sec_brightness_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);
	int on_off;

	if (sscanf(buf, "%d\n", &on_off) == 1) {
		//printk(KERN_DEBUG "[TSPLED] touch_led_on [%d]\n", on_off);
		data->pdata->led_control(on_off);
	} else {
		printk(KERN_DEBUG "[TSPLED] buffer read failed\n");
	}
	return size;
}

static ssize_t sec_extra_button_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int extra_event;

	if (sscanf(buf, "%d\n", &extra_event) == 1) {
		printk(KERN_DEBUG "[TSP] extra event [%d]\n", extra_event);
	} else {
		printk(KERN_DEBUG "[TSP] buffer read failed\n");
	}
	return size;
}

static ssize_t sec_keypad_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", atomic_read(&data->keypad_enable));
}

static ssize_t sec_keypad_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct synaptics_drv_data *data = dev_get_drvdata(dev);

	unsigned int val = 0;
	sscanf(buf, "%d", &val);
	val = (val == 0 ? 0 : 1);
	atomic_set(&data->keypad_enable, val);
	if (val) {
		set_bit(KEY_BACK, data->input->keybit);
		set_bit(KEY_MENU, data->input->keybit);
		set_bit(KEY_HOME, data->input->keybit);
	} else {
		clear_bit(KEY_BACK, data->input->keybit);
		clear_bit(KEY_MENU, data->input->keybit);
		clear_bit(KEY_HOME, data->input->keybit);
	}
	input_sync(data->input);

	return count;
}

static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR,
	NULL, sec_touchkey_sensitivity_store);
static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR,
	sec_touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO | S_IWUSR,
	sec_touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO | S_IWUSR,
	sec_touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn1, S_IRUGO | S_IWUSR,
	sec_touchkey_dummy_btn1_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn3, S_IRUGO | S_IWUSR,
	sec_touchkey_dummy_btn3_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn5, S_IRUGO | S_IWUSR,
	sec_touchkey_dummy_btn5_show, NULL);
static DEVICE_ATTR(touchkey_button_all, S_IRUGO | S_IWUSR,
	sec_touchkey_button_all_show, NULL);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR,
	NULL, sec_brightness_store);
static DEVICE_ATTR(extra_button_event, S_IRUGO | S_IWUSR,
	NULL, sec_extra_button_store);
static DEVICE_ATTR(touchkey_button_status, S_IRUGO | S_IWUSR,
	sec_touchkey_button_status_show, NULL);
static DEVICE_ATTR(keypad_enable, S_IRUGO|S_IWUSR,
	sec_keypad_enable_show, sec_keypad_enable_store);

static struct attribute *sec_touchkey_sysfs_attributes[] = {
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_dummy_btn1.attr,
	&dev_attr_touchkey_dummy_btn3.attr,
	&dev_attr_touchkey_dummy_btn5.attr,
	&dev_attr_touchkey_button_all.attr,
	&dev_attr_brightness.attr,
	&dev_attr_extra_button_event.attr,
	&dev_attr_touchkey_button_status.attr,
	&dev_attr_keypad_enable.attr,
	NULL,
};

static struct attribute_group sec_touchkey_sysfs_attr_group = {
	.attrs = sec_touchkey_sysfs_attributes,
};
#endif

int set_tsp_sysfs(struct synaptics_drv_data *data)
{
	int ret = 0;

	data->dev = device_create(sec_class, NULL, 0, data, "tsp");
	if (IS_ERR(data->dev)) {
		pr_err("[TSP] failed to create device for the sysfs\n");
		ret = -ENODEV;
		goto err_device_create;
	}

	ret = sysfs_create_group(&data->dev->kobj, &sec_sysfs_attr_group);
	if (ret) {
		pr_err("[TSP] failed to create sysfs group\n");
		goto err_device_create;
	}
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
	synaptics_with_gpio_led_device = device_create(sec_class,
				NULL, 0, data, "sec_touchkey");
	if (IS_ERR(synaptics_with_gpio_led_device)) {
		pr_err("[TSP] failed to create device for tsp_touchkey sysfs\n");
		ret = -ENODEV;
		goto err_device_create;
	}

	ret = sysfs_create_group(&synaptics_with_gpio_led_device->kobj,
				&sec_touchkey_sysfs_attr_group);
	if (ret) {
		pr_err("[TSP] failed to create sec_touchkey sysfs group\n");
		goto err_device_create;
	}
#endif

	return 0;

err_device_create:
	return ret;
}

void remove_tsp_sysfs(struct synaptics_drv_data *data)
{
	if (NULL != data->references)
		kfree(data->references);
	if (NULL != data->tx_to_tx)
		kfree(data->tx_to_tx);
	if (NULL != data->tx_to_gnd)
		kfree(data->tx_to_gnd);

	sysfs_remove_group(&data->dev->kobj, &sec_sysfs_attr_group);
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
	sysfs_remove_group(&synaptics_with_gpio_led_device->kobj,
			&sec_touchkey_sysfs_attr_group);
#endif
	put_device(data->dev);
	device_unregister(data->dev);
}

MODULE_AUTHOR("junki671.min@samsung.com");
MODULE_DESCRIPTION("sec sysfs for synaptics tsp");
MODULE_LICENSE("GPL");

