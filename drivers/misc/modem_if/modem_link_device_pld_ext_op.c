/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_pld.h"
#include "modem_utils.h"

#if defined(CONFIG_CDMA_MODEM_MDM6600) || defined(CONFIG_GSM_MODEM_ESC6270)
enum qc_dload_tag {
	QC_DLOAD_TAG_NONE = 0,
	QC_DLOAD_TAG_BIN,
	QC_DLOAD_TAG_NV,
	QC_DLOAD_TAG_MAX
};

static void qc_dload_task(unsigned long data);

static void qc_init_boot_map(struct pld_link_device *pld)
{
	struct qc_dpram_boot_map *qbt_map = &pld->qc_bt_map;
	struct modemlink_dpram_data *dpram = pld->dpram;

	qbt_map->buff = pld->dev[0]->txq.buff;
	qbt_map->frame_size = (u16 *)(pld->base + dpram->boot_size_offset);
	qbt_map->tag = (u16 *)(pld->base + dpram->boot_tag_offset);
	qbt_map->count = (u16 *)(pld->base + dpram->boot_count_offset);

	tasklet_init(&pld->dl_tsk, qc_dload_task, (unsigned long)pld);
}

static void qc_dload_map(struct pld_link_device *pld, u8 is_upload)
{
	struct qc_dpram_boot_map *qbt_map = &pld->qc_bt_map;
	struct modemlink_dpram_data *dpram = pld->dpram;
	unsigned int upload_offset = 0;

	if (is_upload == 1)	{
		upload_offset = 0x1000;
		qbt_map->buff = pld->dev[0]->rxq.buff;
	}	else {
		upload_offset = 0;
		qbt_map->buff = pld->dev[0]->txq.buff;
	}

	qbt_map->frame_size = (u16 *)(pld->base +
			dpram->boot_size_offset + upload_offset);
	qbt_map->tag = (u16 *)(pld->base +
			dpram->boot_tag_offset + upload_offset);
	qbt_map->count = (u16 *)(pld->base +
			dpram->boot_count_offset + upload_offset);
}

static int qc_prepare_download(struct pld_link_device *pld)
{
	int retval = 0;
	int count = 0;

	qc_dload_map(pld, 0);

	while (1) {
		if (pld->qc_udl_check.copy_start) {
			pld->qc_udl_check.copy_start = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > 1000) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return retval;
}

static void _qc_do_download(struct pld_link_device *pld,
			struct qc_dpram_udl_param *param)
{
	struct qc_dpram_boot_map *qbt_map = &pld->qc_bt_map;

	if (param->size <= pld->dpram->max_boot_frame_size) {
		iowrite16(PLD_ADDR_MASK(&qbt_map->buff[0]),
					pld->address_buffer);
		memcpy(pld->base, param->addr, param->size);

		iowrite16(PLD_ADDR_MASK(&qbt_map->frame_size[0]),
					pld->address_buffer);
		iowrite16(param->size, pld->base);

		iowrite16(PLD_ADDR_MASK(&qbt_map->tag[0]),
					pld->address_buffer);
		iowrite16(param->tag, pld->base);

		iowrite16(PLD_ADDR_MASK(&qbt_map->count[0]),
					pld->address_buffer);
		iowrite16(param->count, pld->base);

		pld->send_intr(pld, 0xDB12);
	} else {
		mif_info("param->size %d\n", param->size);
	}
}

static int _qc_download(struct pld_link_device *pld, void *arg,
			enum qc_dload_tag tag)
{
	int retval = 0;
	int count = 0;
	int cnt_limit;
	unsigned char *img;
	struct qc_dpram_udl_param param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_from_user fail\n");
		return -1;
	}

	img = vmalloc(param.size);
	if (!img) {
		mif_err("ERR! vmalloc fail\n");
		return -1;
	}
	memset(img, 0, param.size);
	memcpy(img, param.addr, param.size);

	pld->qc_udl_check.total_size = param.size;
	pld->qc_udl_check.rest_size = param.size;
	pld->qc_udl_check.send_size = 0;
	pld->qc_udl_check.copy_complete = 0;

	pld->qc_udl_param.addr = img;
	pld->qc_udl_param.size = pld->dpram->max_boot_frame_size;
	if (tag == QC_DLOAD_TAG_NV)
		pld->qc_udl_param.count = 1;
	else
		pld->qc_udl_param.count = param.count;
	pld->qc_udl_param.tag = tag;

	if (pld->qc_udl_check.rest_size < pld->dpram->max_boot_frame_size)
		pld->qc_udl_param.size = pld->qc_udl_check.rest_size;

	/* Download image (binary or NV) */
	_qc_do_download(pld, &pld->qc_udl_param);

	/* Wait for completion
	*/
	if (tag == QC_DLOAD_TAG_NV)
		cnt_limit = 200;
	else
		cnt_limit = 1000;

	while (1) {
		if (pld->qc_udl_check.copy_complete) {
			pld->qc_udl_check.copy_complete = 0;
			retval = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > cnt_limit) {
			mif_err("ERR! count %d\n", count);
			retval = -1;
			break;
		}
	}

	vfree(img);

	return retval;
}

static int qc_download_bin(struct pld_link_device *pld, void *arg)
{
	return _qc_download(pld, arg, QC_DLOAD_TAG_BIN);
}

static int qc_download_nv(struct pld_link_device *pld, void *arg)
{
	return _qc_download(pld, arg, QC_DLOAD_TAG_NV);
}

static void qc_dload_task(unsigned long data)
{
	struct pld_link_device *pld = (struct pld_link_device *)data;

	pld->qc_udl_check.send_size += pld->qc_udl_param.size;
	pld->qc_udl_check.rest_size -= pld->qc_udl_param.size;

	pld->qc_udl_param.addr += pld->qc_udl_param.size;

	if (pld->qc_udl_check.send_size >= pld->qc_udl_check.total_size) {
		pld->qc_udl_check.copy_complete = 1;
		pld->qc_udl_param.tag = 0;
		return;
	}

	if (pld->qc_udl_check.rest_size < pld->dpram->max_boot_frame_size)
		pld->qc_udl_param.size = pld->qc_udl_check.rest_size;

	pld->qc_udl_param.count += 1;

	_qc_do_download(pld, &pld->qc_udl_param);
}

static void qc_dload_cmd_handler(struct pld_link_device *pld, u16 cmd)
{
	switch (cmd) {
	case 0x1234:
		pld->qc_udl_check.copy_start = 1;
		break;

	case 0xDBAB:
		tasklet_schedule(&pld->dl_tsk);
		break;

	case 0xABCD:
		mif_info("[%s] booting Start\n", pld->ld.name);
		pld->qc_udl_check.boot_complete = 1;
		break;

	default:
		mif_err("ERR! unknown command 0x%04X\n", cmd);
	}
}

static int qc_boot_start(struct pld_link_device *pld)
{
	u16 mask = 0;
	int count = 0;

	/* Send interrupt -> '0x4567' */
	mask = 0x4567;
	pld->send_intr(pld, mask);

	while (1) {
		if (pld->qc_udl_check.boot_complete) {
			pld->qc_udl_check.boot_complete = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > 200) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return 0;
}

static int qc_boot_post_process(struct pld_link_device *pld)
{
	int count = 0;

	while (1) {
		if (pld->boot_start_complete) {
			pld->boot_start_complete = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > 200) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return 0;
}

static void qc_start_handler(struct pld_link_device *pld)
{
	/*
	 * INT_MASK_VALID | INT_MASK_CMD | INT_MASK_CP_AIRPLANE_BOOT |
	 * INT_MASK_CP_AP_ANDROID | INT_MASK_CMD_INIT_END
	 */
	u16 mask = (0x0080 | 0x0040 | 0x1000 | 0x0100 | 0x0002);

	pld->boot_start_complete = 1;

	/* Send INIT_END code to CP */
	mif_info("send 0x%04X (INIT_END)\n", mask);

	pld->send_intr(pld, mask);
}

static void qc_crash_log(struct pld_link_device *pld)
{
	struct link_device *ld = &pld->ld;
	static unsigned char buf[151];
	u8 __iomem *data = NULL;

	data = pld->get_rx_buff(pld, IPC_FMT);
	memcpy(buf, data, (sizeof(buf) - 1));

	mif_info("PHONE ERR MSG\t| %s Crash\n", ld->mdm_data->name);
	mif_info("PHONE ERR MSG\t| %s\n", buf);
}

static int _qc_data_upload(struct pld_link_device *pld,
			struct qc_dpram_udl_param *param)
{
	struct qc_dpram_boot_map *qbt_map = &pld->qc_bt_map;
	int retval = 0;
	u16 intval = 0;
	int count = 0;

	while (1) {
		if (!gpio_get_value(pld->gpio_ipc_int2ap)) {
			intval = pld->recv_intr(pld);
			if (intval == 0xDBAB) {
				break;
			} else {
				mif_err("intr 0x%08x\n", intval);
				return -1;
			}
		}

		usleep_range(1000, 2000);

		count++;
		if (count > 200) {
			mif_err("<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	iowrite16(PLD_ADDR_MASK(&qbt_map->frame_size[0]),
				pld->address_buffer);
	param->size = ioread16(pld->base);

	iowrite16(PLD_ADDR_MASK(&qbt_map->tag[0]),
				pld->address_buffer);
	param->tag = ioread16(pld->base);

	iowrite16(PLD_ADDR_MASK(&qbt_map->count[0]),
				pld->address_buffer);
	param->count = ioread16(pld->base);

	iowrite16(PLD_ADDR_MASK(&qbt_map->buff[0]),
				pld->address_buffer);
	memcpy(param->addr, pld->base, param->size);

	pld->send_intr(pld, 0xDB12);

	return retval;
}

static int qc_uload_step1(struct pld_link_device *pld)
{
	int retval = 0;
	int count = 0;
	u16 intval = 0;
	u16 mask = 0;

	qc_dload_map(pld, 1);

	mif_info("+---------------------------------------------+\n");
	mif_info("|            UPLOAD PHONE SDRAM               |\n");
	mif_info("+---------------------------------------------+\n");

	while (1) {
		if (!gpio_get_value(pld->gpio_ipc_int2ap)) {
			intval = pld->recv_intr(pld);
			mif_info("intr 0x%04x\n", intval);
			if (intval == 0x1234) {
				break;
			} else {
				mif_info("ERR! invalid intr\n");
				return -1;
			}
		}

		usleep_range(1000, 2000);

		count++;
		if (count > 200) {
			intval = pld->recv_intr(pld);
			mif_info("count %d, intr 0x%04x\n", count, intval);
			if (intval == 0x1234)
				break;
			return -1;
		}
	}

	mask = 0xDEAD;
	pld->send_intr(pld, mask);

	return retval;
}

static int qc_uload_step2(struct pld_link_device *pld, void *arg)
{
	int retval = 0;
	struct qc_dpram_udl_param param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_from_user fail (err %d)\n", retval);
		return -1;
	}

	retval = _qc_data_upload(pld, &param);
	if (retval < 0) {
		mif_err("ERR! _qc_data_upload fail (err %d)\n", retval);
		return -1;
	}

	if (!(param.count % 500))
		mif_info("param->count = %d\n", param.count);

	if (param.tag == 4) {
		enable_irq(pld->irq);
		mif_info("param->tag = %d\n", param.tag);
	}

	retval = copy_to_user((unsigned long *)arg, &param, sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_to_user fail (err %d)\n", retval);
		return -1;
	}

	return retval;
}

static int qc_ioctl(struct pld_link_device *pld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct link_device *ld = &pld->ld;
	int err = 0;

	switch (cmd) {
	case IOCTL_DPRAM_PHONE_POWON:
		err = qc_prepare_download(pld);
		if (err < 0)
			mif_info("%s: ERR! prepare_download fail\n", ld->name);
		break;

	case IOCTL_DPRAM_PHONEIMG_LOAD:
		err = qc_download_bin(pld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_bin fail\n", ld->name);
		break;

	case IOCTL_DPRAM_NVDATA_LOAD:
		err = qc_download_nv(pld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_nv fail\n", ld->name);
		break;

	case IOCTL_DPRAM_PHONE_BOOTSTART:
		err = qc_boot_start(pld);
		if (err < 0) {
			mif_info("%s: ERR! boot_start fail\n", ld->name);
			break;
		}

		err = qc_boot_post_process(pld);
		if (err < 0)
			mif_info("%s: ERR! boot_post_process fail\n", ld->name);

		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP1:
		disable_irq_nosync(pld->irq);
		err = qc_uload_step1(pld);
		if (err < 0) {
			enable_irq(pld->irq);
			mif_info("%s: ERR! upload_step1 fail\n", ld->name);
		}
		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP2:
		err = qc_uload_step2(pld, (void *)arg);
		if (err < 0) {
			enable_irq(pld->irq);
			mif_info("%s: ERR! upload_step2 fail\n", ld->name);
		}
		break;

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		err = -EINVAL;
		break;
	}

	return err;
}
#endif

static struct pld_ext_op ext_op_set[] = {
#if defined(CONFIG_CDMA_MODEM_MDM6600)
	[QC_MDM6600] = {
		.exist = 1,
		.init_boot_map = qc_init_boot_map,
		.dload_cmd_handler = qc_dload_cmd_handler,
		.cp_start_handler = qc_start_handler,
		.crash_log = qc_crash_log,
		.ioctl = qc_ioctl,
	},
#endif
#if defined(CONFIG_GSM_MODEM_ESC6270)
	[QC_ESC6270] = {
		.exist = 1,
		.init_boot_map = qc_init_boot_map,
		.dload_cmd_handler = qc_dload_cmd_handler,
		.cp_start_handler = qc_start_handler,
		.crash_log = qc_crash_log,
		.ioctl = qc_ioctl,
	},
#endif
};

struct pld_ext_op *pld_get_ext_op(enum modem_t modem)
{
	if (ext_op_set[modem].exist)
		return &ext_op_set[modem];
	else
		return NULL;
}
