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

static void qc_init_boot_map(struct dpram_link_device *dpld)
{
	struct qc_dpram_boot_map *bt_map = &dpld->qc_bt_map;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;

	bt_map->buff = dpld->dev[0]->txq.buff;
	bt_map->frame_size = (u16 *)(dpld->dp_base + dpctl->boot_size_offset);
	bt_map->tag = (u16 *)(dpld->dp_base + dpctl->boot_tag_offset);
	bt_map->count = (u16 *)(dpld->dp_base + dpctl->boot_count_offset);

	tasklet_init(&dpld->dl_tsk, qc_dload_task, (unsigned long)dpld);
}

static void qc_dload_map(struct dpram_link_device *dpld, u8 is_upload)
{
	struct qc_dpram_boot_map *bt_map = &dpld->qc_bt_map;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;
	unsigned int upload_offset = 0;

	if (is_upload == 1)	{
		upload_offset = 0x1000;
		bt_map->buff = dpld->dev[0]->rxq.buff;
	}	else {
		upload_offset = 0;
		bt_map->buff = dpld->dev[0]->txq.buff;
	}

	bt_map->frame_size = (u16 *)(dpld->dp_base +
			dpctl->boot_size_offset + upload_offset);
	bt_map->tag = (u16 *)(dpld->dp_base +
			dpctl->boot_tag_offset + upload_offset);
	bt_map->count = (u16 *)(dpld->dp_base +
			dpctl->boot_count_offset + upload_offset);

}

static int qc_prepare_download(struct dpram_link_device *dpld)
{
	int retval = 0;
	int count = 0;

	qc_dload_map(dpld, 0);

	while (1) {
		if (dpld->udl_check.copy_start) {
			dpld->udl_check.copy_start = 0;
			break;
		}

		msleep(20);

		count++;
		if (count > 1000) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return retval;
}

static void _qc_do_download(struct dpram_link_device *dpld,
			struct dpram_udl_param *param)
{
	struct qc_dpram_boot_map *bt_map = &dpld->qc_bt_map;

	if (param->size <= dpld->dpctl->max_boot_frame_size) {
		iowrite16(PLD_ADDR_MASK(&bt_map->buff[0]),
					dpld->address_buffer);
		memcpy(dpld->dp_base, param->addr, param->size);

		iowrite16(PLD_ADDR_MASK(&bt_map->frame_size[0]),
					dpld->address_buffer);
		iowrite16(param->size, dpld->dp_base);

		iowrite16(PLD_ADDR_MASK(&bt_map->tag[0]),
					dpld->address_buffer);
		iowrite16(param->tag, dpld->dp_base);

		iowrite16(PLD_ADDR_MASK(&bt_map->count[0]),
					dpld->address_buffer);
		iowrite16(param->count, dpld->dp_base);

		dpld->send_intr(dpld, 0xDB12);
	} else {
		mif_info("param->size %d\n", param->size);
	}
}

static int _qc_download(struct dpram_link_device *dpld, void *arg,
			enum qc_dload_tag tag)
{
	int retval = 0;
	int count = 0;
	int cnt_limit;
	unsigned char *img;
	struct dpram_udl_param param;

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

	dpld->udl_check.total_size = param.size;
	dpld->udl_check.rest_size = param.size;
	dpld->udl_check.send_size = 0;
	dpld->udl_check.copy_complete = 0;

	dpld->udl_param.addr = img;
	dpld->udl_param.size = dpld->dpctl->max_boot_frame_size;
	if (tag == QC_DLOAD_TAG_NV)
		dpld->udl_param.count = 1;
	else
		dpld->udl_param.count = param.count;
	dpld->udl_param.tag = tag;

	if (dpld->udl_check.rest_size < dpld->dpctl->max_boot_frame_size)
		dpld->udl_param.size = dpld->udl_check.rest_size;

	/* Download image (binary or NV) */
	_qc_do_download(dpld, &dpld->udl_param);

	/* Wait for completion
	*/
	if (tag == QC_DLOAD_TAG_NV)
		cnt_limit = 200;
	else
		cnt_limit = 1000;

	while (1) {
		if (dpld->udl_check.copy_complete) {
			dpld->udl_check.copy_complete = 0;
			retval = 0;
			break;
		}

		msleep(20);

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

static int qc_download_bin(struct dpram_link_device *dpld, void *arg)
{
	return _qc_download(dpld, arg, QC_DLOAD_TAG_BIN);
}

static int qc_download_nv(struct dpram_link_device *dpld, void *arg)
{
	return _qc_download(dpld, arg, QC_DLOAD_TAG_NV);
}

static void qc_dload_task(unsigned long data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;

	dpld->udl_check.send_size += dpld->udl_param.size;
	dpld->udl_check.rest_size -= dpld->udl_param.size;

	dpld->udl_param.addr += dpld->udl_param.size;

	if (dpld->udl_check.send_size >= dpld->udl_check.total_size) {
		dpld->udl_check.copy_complete = 1;
		dpld->udl_param.tag = 0;
		return;
	}

	if (dpld->udl_check.rest_size < dpld->dpctl->max_boot_frame_size)
		dpld->udl_param.size = dpld->udl_check.rest_size;

	dpld->udl_param.count += 1;

	_qc_do_download(dpld, &dpld->udl_param);
}

static void qc_dload_cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	switch (cmd) {
	case 0x1234:
		dpld->udl_check.copy_start = 1;
		break;

	case 0xDBAB:
		tasklet_schedule(&dpld->dl_tsk);
		break;

	case 0xABCD:
		mif_info("[%s] booting Start\n", dpld->ld.name);
		dpld->udl_check.boot_complete = 1;
		break;

	default:
		mif_err("ERR! unknown command 0x%04X\n", cmd);
	}
}

static int qc_boot_start(struct dpram_link_device *dpld)
{
	u16 mask = 0;
	int count = 0;

	/* Send interrupt -> '0x4567' */
	mask = 0x4567;
	dpld->send_intr(dpld, mask);

	while (1) {
		if (dpld->udl_check.boot_complete) {
			dpld->udl_check.boot_complete = 0;
			break;
		}

		msleep(20);

		count++;
		if (count > 200) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return 0;
}

static int qc_boot_post_process(struct dpram_link_device *dpld)
{
	int count = 0;

	while (1) {
		if (dpld->boot_start_complete) {
			dpld->boot_start_complete = 0;
			break;
		}

		msleep(20);

		count++;
		if (count > 200) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return 0;
}

static void qc_start_handler(struct dpram_link_device *dpld)
{
	/*
	 * INT_MASK_VALID | INT_MASK_CMD | INT_MASK_CP_AIRPLANE_BOOT |
	 * INT_MASK_CP_AP_ANDROID | INT_MASK_CMD_INIT_END
	 */
	u16 mask = (0x0080 | 0x0040 | 0x1000 | 0x0100 | 0x0002);

	dpld->boot_start_complete = 1;

	/* Send INIT_END code to CP */
	mif_info("send 0x%04X (INIT_END)\n", mask);

	dpld->send_intr(dpld, mask);
}

static void qc_crash_log(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	static unsigned char buf[151];
	u8 __iomem *data = NULL;

	data = dpld->get_rx_buff(dpld, IPC_FMT);
	memcpy(buf, data, (sizeof(buf) - 1));

	mif_info("PHONE ERR MSG\t| %s Crash\n", ld->mdm_data->name);
	mif_info("PHONE ERR MSG\t| %s\n", buf);
}

static int _qc_data_upload(struct dpram_link_device *dpld,
			struct dpram_udl_param *param)
{
	struct qc_dpram_boot_map *bt_map = &dpld->qc_bt_map;
	int retval = 0;
	u16 intval = 0;
	int count = 0;

	while (1) {
		if (!gpio_get_value(dpld->gpio_dpram_int)) {
			intval = dpld->recv_intr(dpld);
			if (intval == 0xDBAB) {
				break;
			} else {
				mif_err("intr 0x%08x\n", intval);
				return -1;
			}
		}

		msleep(20);

		count++;
		if (count > 200) {
			mif_err("<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	iowrite16(PLD_ADDR_MASK(&bt_map->frame_size[0]),
				dpld->address_buffer);
	param->size = ioread16(dpld->dp_base);

	iowrite16(PLD_ADDR_MASK(&bt_map->tag[0]),
				dpld->address_buffer);
	param->tag = ioread16(dpld->dp_base);

	iowrite16(PLD_ADDR_MASK(&bt_map->count[0]),
				dpld->address_buffer);
	param->count = ioread16(dpld->dp_base);

	iowrite16(PLD_ADDR_MASK(&bt_map->buff[0]),
				dpld->address_buffer);
	memcpy(param->addr, dpld->dp_base, param->size);

	dpld->send_intr(dpld, 0xDB12);

	return retval;
}

static int qc_uload_step1(struct dpram_link_device *dpld)
{
	int retval = 0;
	int count = 0;
	u16 intval = 0;
	u16 mask = 0;

	qc_dload_map(dpld, 1);

	mif_info("+---------------------------------------------+\n");
	mif_info("|            UPLOAD PHONE SDRAM               |\n");
	mif_info("+---------------------------------------------+\n");

	while (1) {
		if (!gpio_get_value(dpld->gpio_dpram_int)) {
			intval = dpld->recv_intr(dpld);
			mif_info("intr 0x%04x\n", intval);
			if (intval == 0x1234) {
				break;
			} else {
				mif_info("ERR! invalid intr\n");
				return -1;
			}
		}

		msleep(20);

		count++;
		if (count > 200) {
			intval = dpld->recv_intr(dpld);
			mif_info("count %d, intr 0x%04x\n", count, intval);
			if (intval == 0x1234)
				break;
			return -1;
		}
	}

	mask = 0xDEAD;
	dpld->send_intr(dpld, mask);

	return retval;
}

static int qc_uload_step2(struct dpram_link_device *dpld, void *arg)
{
	int retval = 0;
	struct dpram_udl_param param;

	retval = copy_from_user((void *)&param, (void *)arg, sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_from_user fail (err %d)\n", retval);
		return -1;
	}

	retval = _qc_data_upload(dpld, &param);
	if (retval < 0) {
		mif_err("ERR! _qc_data_upload fail (err %d)\n", retval);
		return -1;
	}

	if (!(param.count % 500))
		mif_info("param->count = %d\n", param.count);

	if (param.tag == 4) {
		enable_irq(dpld->irq);
		mif_info("param->tag = %d\n", param.tag);
	}

	retval = copy_to_user((unsigned long *)arg, &param, sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_to_user fail (err %d)\n", retval);
		return -1;
	}

	return retval;
}

static int qc_ioctl(struct dpram_link_device *dpld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct link_device *ld = &dpld->ld;
	int err = 0;

	switch (cmd) {
	case IOCTL_DPRAM_PHONE_POWON:
		err = qc_prepare_download(dpld);
		if (err < 0)
			mif_info("%s: ERR! prepare_download fail\n", ld->name);
		break;

	case IOCTL_DPRAM_PHONEIMG_LOAD:
		err = qc_download_bin(dpld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_bin fail\n", ld->name);
		break;

	case IOCTL_DPRAM_NVDATA_LOAD:
		err = qc_download_nv(dpld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_nv fail\n", ld->name);
		break;

	case IOCTL_DPRAM_PHONE_BOOTSTART:
		err = qc_boot_start(dpld);
		if (err < 0) {
			mif_info("%s: ERR! boot_start fail\n", ld->name);
			break;
		}

		err = qc_boot_post_process(dpld);
		if (err < 0)
			mif_info("%s: ERR! boot_post_process fail\n", ld->name);

		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP1:
		disable_irq_nosync(dpld->irq);
		err = qc_uload_step1(dpld);
		if (err < 0) {
			enable_irq(dpld->irq);
			mif_info("%s: ERR! upload_step1 fail\n", ld->name);
		}
		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP2:
		err = qc_uload_step2(dpld, (void *)arg);
		if (err < 0) {
			enable_irq(dpld->irq);
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

static struct dpram_ext_op ext_op_set[] = {
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

struct dpram_ext_op *dpram_get_ext_op(enum modem_t modem)
{
	if (ext_op_set[modem].exist)
		return &ext_op_set[modem];
	else
		return NULL;
}
