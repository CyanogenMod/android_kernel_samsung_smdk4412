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
#include "modem_link_device_dpram.h"
#include "modem_utils.h"

#if defined(CONFIG_LTE_MODEM_CMC221)
/*
** For host (flashless) booting via DPRAM
*/
#define CMC22x_AP_BOOT_DOWN_DONE	0x54329876
#define CMC22x_CP_REQ_MAIN_BIN		0xA5A5A5A5
#define CMC22x_CP_REQ_NV_DATA		0x5A5A5A5A
#define CMC22x_CP_DUMP_MAGIC		0xDEADDEAD

#define CMC22x_HOST_DOWN_START		0x1234
#define CMC22x_HOST_DOWN_END		0x4321
#define CMC22x_REG_NV_DOWN_END		0xABCD
#define CMC22x_CAL_NV_DOWN_END		0xDCBA

#define CMC22x_1ST_BUFF_READY		0xAAAA
#define CMC22x_2ND_BUFF_READY		0xBBBB
#define CMC22x_1ST_BUFF_FULL		0x1111
#define CMC22x_2ND_BUFF_FULL		0x2222

#define CMC22x_CP_RECV_NV_END		0x8888
#define CMC22x_CP_CAL_OK		0x4F4B
#define CMC22x_CP_CAL_BAD		0x4552
#define CMC22x_CP_DUMP_END		0xFADE

#define CMC22x_DUMP_BUFF_SIZE		8192	/* 8 KB */
#endif

#if defined(CONFIG_CDMA_MODEM_CBP72)
static void cbp72_init_boot_map(struct dpram_link_device *dpld)
{
	struct dpram_boot_map *bt_map = &dpld->bt_map;

	bt_map->magic = (u32 *)dpld->base;
	bt_map->buff = (u8 *)(dpld->base + DP_BOOT_BUFF_OFFSET);
	bt_map->size = dpld->size - 4;
}

static void cbp72_init_dl_map(struct dpram_link_device *dpld)
{
	dpld->dl_map.magic = (u32 *)dpld->base;
	dpld->dl_map.buff = (u8 *)(dpld->base + DP_DLOAD_BUFF_OFFSET);
}

static int _cbp72_edpram_wait_resp(struct dpram_link_device *dpld, u32 resp)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	int int2cp;

	ret = wait_for_completion_interruptible_timeout(
			&dpld->udl_cmd_complete, UDL_TIMEOUT);
	if (!ret) {
		mif_info("%s: ERR! No UDL_CMD_RESP!!!\n", ld->name);
		return -ENXIO;
	}

	int2cp = dpld->recv_intr(dpld);
	mif_debug("%s: int2cp = 0x%x\n", ld->name, int2cp);
	if (resp == int2cp || int2cp == 0xA700)
		return int2cp;
	else
		return -EINVAL;
}

static int _cbp72_edpram_download_bin(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	struct link_device *ld = &dpld->ld;
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = dpld->bt_map.buff;
	int err = 0;

	if (bf->len > dpld->bt_map.size) {
		mif_info("%s: ERR! Out of DPRAM boundary\n", ld->name);
		err = -EINVAL;
		goto exit;
	}

	if (bf->len)
		memcpy(buff, bf->data, bf->len);

	init_completion(&dpld->udl_cmd_complete);

	if (bf->req)
		dpld->send_intr(dpld, (u16)bf->req);

	if (bf->resp) {
		err = _cbp72_edpram_wait_resp(dpld, bf->resp);
		if (err < 0) {
			mif_info("%s: ERR! wait_response fail (%d)\n",
				ld->name, err);
			goto exit;
		} else if (err == bf->resp) {
			err = skb->len;
		}
	}

exit:
	dev_kfree_skb_any(skb);
	return err;
}

static int cbp72_download_binary(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	if (dpld->type == EXT_DPRAM)
		return _cbp72_edpram_download_bin(dpld, skb);
	else
		return -ENODEV;
}

static int cbp72_dump_start(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	u8 *dest = dpld->ul_map.buff;
	int ret;

	ld->mode = LINK_MODE_ULOAD;

	ret = del_timer(&dpld->crash_timer);
	wake_lock(&dpld->wlock);

	iowrite32(DP_MAGIC_UMDL, dpld->ul_map.magic);

	iowrite8((u8)START_FLAG, dest + 0);
	iowrite8((u8)0x1, dest + 1);
	iowrite8((u8)0x1, dest + 2);
	iowrite8((u8)0x0, dest + 3);
	iowrite8((u8)END_FLAG, dest + 4);

	init_completion(&dpld->crash_start_complete);

	return 0;
}

static int _cbp72_edpram_upload(struct dpram_link_device *dpld,
		struct dpram_dump_arg *dump, unsigned char __user *target)
{
	struct link_device *ld = &dpld->ld;
	struct ul_header header;
	u8 *dest = NULL;
	u8 *buff = NULL;
	u16 plen = 0;
	int err = 0;
	int ret = 0;
	int buff_size = 0;

	mif_debug("\n");

	init_completion(&dpld->udl_cmd_complete);

	mif_debug("%s: req %x, resp %x", ld->name, dump->req, dump->resp);

	if (dump->req)
		dpld->send_intr(dpld, (u16)dump->req);

	if (dump->resp) {
		err = _cbp72_edpram_wait_resp(dpld, dump->resp);
		if (err < 0) {
			mif_info("%s: ERR! wait_response fail (%d)\n",
				ld->name, err);
			goto exit;
		}
	}

	if (dump->cmd)
		return err;

	dest = (u8 *)dpld->ul_map.buff;

	header.bop = *(u8 *)(dest);
	header.total_frame = *(u16 *)(dest + 1);
	header.curr_frame = *(u16 *)(dest + 3);
	header.len = *(u16 *)(dest + 5);

	mif_debug("%s: total frame:%d, current frame:%d, data len:%d\n",
		ld->name, header.total_frame, header.curr_frame, header.len);

	plen = min_t(u16, header.len, DP_DEFAULT_DUMP_LEN);

	buff = vmalloc(DP_DEFAULT_DUMP_LEN);
	if (!buff) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy(buff, dest + sizeof(struct ul_header), plen);
	ret = copy_to_user(dump->buff, buff, plen);
	if (ret < 0) {
		mif_info("%s: ERR! dump copy_to_user fail\n", ld->name);
		err = -EIO;
		goto exit;
	}
	buff_size = plen;

	ret = copy_to_user(target + 4, &buff_size, sizeof(int));
	if (ret < 0) {
		mif_info("%s: ERR! size copy_to_user fail\n", ld->name);
		err = -EIO;
		goto exit;
	}

	vfree(buff);
	return err;

exit:
	if (buff)
		vfree(buff);
	iowrite32(0, dpld->ul_map.magic);
	wake_unlock(&dpld->wlock);
	return err;
}

static int cbp72_dump_update(struct dpram_link_device *dpld, void *arg)
{
	struct link_device *ld = &dpld->ld;
	struct dpram_dump_arg dump;
	int ret;

	ret = copy_from_user(&dump, (void __user *)arg, sizeof(dump));
	if (ret  < 0) {
		mif_info("%s: ERR! copy_from_user fail\n", ld->name);
		return ret;
	}

	return _cbp72_edpram_upload(dpld, &dump, (unsigned char __user *)arg);
}

static int cbp72_set_dl_magic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	ld->mode = LINK_MODE_DLOAD;

	iowrite32(DP_MAGIC_DMDL, dpld->dl_map.magic);

	return 0;
}

static int cbp72_ioctl(struct dpram_link_device *dpld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct link_device *ld = &dpld->ld;
	int err = 0;

	switch (cmd) {
	case IOCTL_MODEM_DL_START:
		err = cbp72_set_dl_magic(ld, iod);
		if (err < 0)
			mif_err("%s: ERR! set_dl_magic fail\n", ld->name);
		break;

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		err = -EINVAL;
		break;
	}

	return err;
}
#endif

#if defined(CONFIG_LTE_MODEM_CMC221)
/* For CMC221 SFR for IDPRAM */
#define CMC_INT2CP_REG		0x10	/* Interrupt to CP            */
#define CMC_INT2AP_REG		0x50
#define CMC_CLR_INT_REG		0x28	/* Clear Interrupt to AP      */
#define CMC_RESET_REG		0x3C
#define CMC_PUT_REG		0x40	/* AP->CP reg for hostbooting */
#define CMC_GET_REG		0x50	/* CP->AP reg for hostbooting */

static void cmc221_init_boot_map(struct dpram_link_device *dpld)
{
	struct dpram_boot_map *bt_map = &dpld->bt_map;

	bt_map->buff = dpld->base;
	bt_map->size = dpld->size;
	bt_map->req = (u32 *)(dpld->base + DP_BOOT_REQ_OFFSET);
	bt_map->resp = (u32 *)(dpld->base + DP_BOOT_RESP_OFFSET);
}

static void cmc221_init_dl_map(struct dpram_link_device *dpld)
{
	dpld->dl_map.magic = (u32 *)dpld->base;
	dpld->dl_map.buff = (u8 *)dpld->base;
}

static void cmc221_init_ul_map(struct dpram_link_device *dpld)
{
	dpld->ul_map.magic = (u32 *)dpld->base;
	dpld->ul_map.buff = (u8 *)dpld->base;
}

static void cmc221_init_ipc_map(struct dpram_link_device *dpld)
{
	struct dpram_ipc_16k_map *dpram_map;
	struct dpram_ipc_device *dev;
	u8 __iomem *sfr_base = dpld->sfr_base;

	dpram_map = (struct dpram_ipc_16k_map *)dpld->base;

	/* Magic code and access enable fields */
	dpld->ipc_map.magic = (u16 __iomem *)&dpram_map->magic;
	dpld->ipc_map.access = (u16 __iomem *)&dpram_map->access;

	/* FMT */
	dev = &dpld->ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)&dpram_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *)&dpram_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&dpram_map->fmt_tx_buff[0];
	dev->txq.size = DP_16K_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&dpram_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *)&dpram_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&dpram_map->fmt_rx_buff[0];
	dev->rxq.size = DP_16K_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send    = INT_MASK_SEND_F;

	/* RAW */
	dev = &dpld->ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)&dpram_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *)&dpram_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&dpram_map->raw_tx_buff[0];
	dev->txq.size = DP_16K_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&dpram_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *)&dpram_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&dpram_map->raw_rx_buff[0];
	dev->rxq.size = DP_16K_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send    = INT_MASK_SEND_R;

	/* SFR */
	dpld->sfr.int2cp = (u16 __iomem *)(sfr_base + CMC_INT2CP_REG);
	dpld->sfr.int2ap = (u16 __iomem *)(sfr_base + CMC_INT2AP_REG);
	dpld->sfr.clr_int2ap = (u16 __iomem *)(sfr_base + CMC_CLR_INT_REG);
	dpld->sfr.reset = (u16 __iomem *)(sfr_base + CMC_RESET_REG);
	dpld->sfr.msg2cp = (u16 __iomem *)(sfr_base + CMC_PUT_REG);
	dpld->sfr.msg2ap = (u16 __iomem *)(sfr_base + CMC_GET_REG);

	/* Interrupt ports */
	dpld->ipc_map.mbx_cp2ap = dpld->sfr.int2ap;
	dpld->ipc_map.mbx_ap2cp = dpld->sfr.int2cp;
}

static inline void cmc221_idpram_reset(struct dpram_link_device *dpld)
{
	iowrite16(1, dpld->sfr.reset);
}

static inline u16 cmc221_idpram_recv_msg(struct dpram_link_device *dpld)
{
	return ioread16(dpld->sfr.msg2ap);
}

static inline void cmc221_idpram_send_msg(struct dpram_link_device *dpld,
					u16 msg)
{
	iowrite16(msg, dpld->sfr.msg2cp);
}

static int _cmc221_idpram_wait_resp(struct dpram_link_device *dpld, u32 resp)
{
	struct link_device *ld = &dpld->ld;
	int count = 50000;
	u32 rcvd = 0;

	if (resp == CMC22x_CP_REQ_NV_DATA) {
		while (1) {
			rcvd = ioread32(dpld->bt_map.resp);
			if (rcvd == resp)
				break;

			rcvd = cmc221_idpram_recv_msg(dpld);
			if (rcvd == 0x9999) {
				mif_info("%s: Invalid resp 0x%04X\n",
					ld->name, rcvd);
				panic("CP Crash ... BAD CRC in CP");
			}

			if (count-- < 0) {
				mif_info("%s: Invalid resp 0x%08X\n",
					ld->name, rcvd);
				return -EAGAIN;
			}

			udelay(100);
		}
	} else {
		while (1) {
			rcvd = cmc221_idpram_recv_msg(dpld);

			if (rcvd == resp)
				break;

			if (resp == CMC22x_CP_RECV_NV_END &&
			    rcvd == CMC22x_CP_CAL_BAD) {
				mif_info("%s: CMC22x_CP_CAL_BAD\n", ld->name);
				break;
			}

			if (count-- < 0) {
				mif_info("%s: Invalid resp 0x%04X\n",
					ld->name, rcvd);
				return -EAGAIN;
			}

			udelay(100);
		}
	}

	return rcvd;
}

static int _cmc221_idpram_send_boot(struct dpram_link_device *dpld, void *arg)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *bt_buff = dpld->bt_map.buff;
	struct dpram_boot_img cp_img;
	u8 *img_buff = NULL;
	int err = 0;
	int cnt = 0;

	ld->mode = LINK_MODE_BOOT;

	dpld->dpctl->setup_speed(DPRAM_SPEED_LOW);

	/* Test memory... After testing, memory is cleared. */
	if (mif_test_dpram(ld->name, bt_buff, dpld->bt_map.size) < 0) {
		mif_info("%s: ERR! mif_test_dpram fail!\n", ld->name);
		ld->mode = LINK_MODE_OFFLINE;
		return -EIO;
	}

	memset(&cp_img, 0, sizeof(struct dpram_boot_img));

	/* Get information about the boot image */
	err = copy_from_user(&cp_img, arg, sizeof(cp_img));
	mif_info("%s: CP image addr = 0x%08X, size = %d\n",
		ld->name, (int)cp_img.addr, cp_img.size);

	/* Alloc a buffer for the boot image */
	img_buff = kzalloc(dpld->bt_map.size, GFP_KERNEL);
	if (!img_buff) {
		mif_info("%s: ERR! kzalloc fail\n", ld->name);
		ld->mode = LINK_MODE_OFFLINE;
		return -ENOMEM;
	}

	/* Copy boot image from the user space to the image buffer */
	err = copy_from_user(img_buff, cp_img.addr, cp_img.size);

	/* Copy boot image to DPRAM and verify it */
	memcpy(bt_buff, img_buff, cp_img.size);
	if (memcmp16_to_io(bt_buff, img_buff, cp_img.size)) {
		mif_info("%s: ERR! Boot may be broken!!!\n", ld->name);
		goto err;
	}

	cmc221_idpram_reset(dpld);
	usleep_range(1000, 2000);

	if (cp_img.mode == HOST_BOOT_MODE_NORMAL) {
		mif_info("%s: HOST_BOOT_MODE_NORMAL\n", ld->name);
		mif_info("%s: Send req 0x%08X\n", ld->name, cp_img.req);
		iowrite32(cp_img.req, dpld->bt_map.req);

		/* Wait for cp_img.resp for up to 2 seconds */
		mif_info("%s: Wait resp 0x%08X\n", ld->name, cp_img.resp);
		while (ioread32(dpld->bt_map.resp) != cp_img.resp) {
			cnt++;
			usleep_range(1000, 2000);

			if (cnt > 1000) {
				mif_info("%s: ERR! Invalid resp 0x%08X\n",
					ld->name, ioread32(dpld->bt_map.resp));
				goto err;
			}
		}
	} else {
		mif_info("%s: HOST_BOOT_MODE_DUMP\n", ld->name);
	}

	kfree(img_buff);

	mif_info("%s: Send BOOT done\n", ld->name);

	dpld->dpctl->setup_speed(DPRAM_SPEED_HIGH);

	return 0;

err:
	ld->mode = LINK_MODE_OFFLINE;
	kfree(img_buff);

	mif_info("%s: ERR! Boot send fail!!!\n", ld->name);
	return -EIO;
}

static int cmc221_download_boot(struct dpram_link_device *dpld, void *arg)
{
	if (dpld->type == CP_IDPRAM)
		return _cmc221_idpram_send_boot(dpld, arg);
	else
		return -ENODEV;
}

static int _cmc221_idpram_download_bin(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	int err = 0;
	int ret = 0;
	struct link_device *ld = &dpld->ld;
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = (dpld->bt_map.buff + bf->offset);

	if ((bf->offset + bf->len) > dpld->bt_map.size) {
		mif_info("%s: ERR! Out of DPRAM boundary\n", ld->name);
		err = -EINVAL;
		goto exit;
	}

	if (bf->len)
		memcpy(buff, bf->data, bf->len);

	if (bf->req)
		cmc221_idpram_send_msg(dpld, (u16)bf->req);

	if (bf->resp) {
		err = _cmc221_idpram_wait_resp(dpld, bf->resp);
		if (err < 0)
			mif_info("%s: ERR! wait_response fail (err %d)\n",
				ld->name, err);
	}

	if (bf->req == CMC22x_CAL_NV_DOWN_END)
		mif_info("%s: CMC22x_CAL_NV_DOWN_END\n", ld->name);

exit:
	if (err < 0)
		ret = err;
	else
		ret = skb->len;

	dev_kfree_skb_any(skb);

	return ret;
}

static int cmc221_download_binary(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	if (dpld->type == CP_IDPRAM)
		return _cmc221_idpram_download_bin(dpld, skb);
	else
		return -ENODEV;
}

static int cmc221_dump_start(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	int ret;

	ld->mode = LINK_MODE_ULOAD;

	ret = del_timer(&dpld->crash_timer);
	wake_lock(&dpld->wlock);

	dpld->crash_rcvd = 0;
	iowrite32(CMC22x_CP_DUMP_MAGIC, dpld->ul_map.magic);

	init_completion(&dpld->crash_start_complete);

	return 0;
}

static void _cmc221_idpram_wait_dump(unsigned long arg)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)arg;
	u16 msg;

	msg = cmc221_idpram_recv_msg(dpld);
	if (msg == CMC22x_CP_DUMP_END) {
		complete_all(&dpld->crash_recv_done);
		return;
	}

	if (((dpld->crash_rcvd & 0x1) == 0) && (msg == CMC22x_1ST_BUFF_FULL)) {
		complete_all(&dpld->crash_recv_done);
		return;
	}

	if (((dpld->crash_rcvd & 0x1) == 1) && (msg == CMC22x_2ND_BUFF_FULL)) {
		complete_all(&dpld->crash_recv_done);
		return;
	}

	mif_add_timer(&dpld->crash_timer, DUMP_WAIT_TIMEOUT,
		_cmc221_idpram_wait_dump, (unsigned long)dpld);
}

static int _cmc221_idpram_upload(struct dpram_link_device *dpld,
		struct dpram_dump_arg *dumparg)
{
	struct link_device *ld = &dpld->ld;
	int ret;
	u8 __iomem *src;
	int buff_size = CMC22x_DUMP_BUFF_SIZE;

	if ((dpld->crash_rcvd & 0x1) == 0)
		cmc221_idpram_send_msg(dpld, CMC22x_1ST_BUFF_READY);
	else
		cmc221_idpram_send_msg(dpld, CMC22x_2ND_BUFF_READY);

	init_completion(&dpld->crash_recv_done);

	mif_add_timer(&dpld->crash_timer, DUMP_WAIT_TIMEOUT,
		_cmc221_idpram_wait_dump, (unsigned long)dpld);

	ret = wait_for_completion_interruptible_timeout(
			&dpld->crash_recv_done, DUMP_TIMEOUT);
	if (!ret) {
		mif_info("%s: ERR! CP didn't send dump data!!!\n", ld->name);
		goto err_out;
	}

	if (cmc221_idpram_recv_msg(dpld) == CMC22x_CP_DUMP_END) {
		mif_info("%s: CMC22x_CP_DUMP_END\n", ld->name);
		return 0;
	}

	if ((dpld->crash_rcvd & 0x1) == 0)
		src = dpld->ul_map.buff;
	else
		src = dpld->ul_map.buff + CMC22x_DUMP_BUFF_SIZE;

	memcpy(dpld->buff, src, buff_size);

	ret = copy_to_user(dumparg->buff, dpld->buff, buff_size);
	if (ret < 0) {
		mif_info("%s: ERR! copy_to_user fail\n", ld->name);
		goto err_out;
	}

	dpld->crash_rcvd++;
	return buff_size;

err_out:
	return -EIO;
}

static int cmc221_dump_update(struct dpram_link_device *dpld, void *arg)
{
	struct link_device *ld = &dpld->ld;
	struct dpram_dump_arg dump;
	int ret;

	ret = copy_from_user(&dump, (void __user *)arg, sizeof(dump));
	if (ret  < 0) {
		mif_info("%s: ERR! copy_from_user fail\n", ld->name);
		return ret;
	}

	return _cmc221_idpram_upload(dpld, &dump);
}

static int cmc221_ioctl(struct dpram_link_device *dpld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct link_device *ld = &dpld->ld;
	int err = 0;

	switch (cmd) {
	case IOCTL_DPRAM_SEND_BOOT:
		err = cmc221_download_boot(dpld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_boot fail\n", ld->name);
		break;

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		err = -EINVAL;
		break;
	}

	return err;
}

static void cmc221_idpram_clr_int2ap(struct dpram_link_device *dpld)
{
	iowrite16(0xFFFF, dpld->sfr.clr_int2ap);
}

static int cmc221_idpram_wakeup(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct modem_data *mdm_data = ld->mdm_data;
	int cnt = 0;

	gpio_set_value(mdm_data->gpio_dpram_wakeup, 1);

	while (!gpio_get_value(mdm_data->gpio_dpram_status)) {
		if (cnt++ > 10) {
			if (in_irq())
				mif_err("ERR! gpio_dpram_status == 0 in IRQ\n");
			else
				mif_err("ERR! gpio_dpram_status == 0\n");
			return -EACCES;
		}

		mif_info("gpio_dpram_status == 0 (cnt %d)\n", cnt);
		if (in_interrupt())
			udelay(1000);
		else
			usleep_range(1000, 2000);
	}

	return 0;
}

static void cmc221_idpram_sleep(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	gpio_set_value(ld->mdm_data->gpio_dpram_wakeup, 0);
}
#endif

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
	struct qc_dpram_boot_map *qbt_map = &dpld->qc_bt_map;
	struct modemlink_dpram_control *dpctl = dpld->dpctl;

	qbt_map->buff = dpld->base;
	qbt_map->frame_size = (u16 *)(dpld->base + dpctl->boot_size_offset);
	qbt_map->tag = (u16 *)(dpld->base + dpctl->boot_tag_offset);
	qbt_map->count = (u16 *)(dpld->base + dpctl->boot_count_offset);

	tasklet_init(&dpld->dl_tsk, qc_dload_task, (unsigned long)dpld);
}

static int qc_prepare_download(struct dpram_link_device *dpld)
{
	int retval = 0;
	int count = 0;

	while (1) {
		if (dpld->udl_check.copy_start) {
			dpld->udl_check.copy_start = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > 300) {
			mif_err("ERR! count %d\n", count);
			return -1;
		}
	}

	return retval;
}

static void _qc_do_download(struct dpram_link_device *dpld,
			struct dpram_udl_param *param)
{
	struct qc_dpram_boot_map *qbt_map = &dpld->qc_bt_map;

	if (param->size <= dpld->dpctl->max_boot_frame_size) {
		memcpy(qbt_map->buff, param->addr, param->size);
		iowrite16(param->size, qbt_map->frame_size);
		iowrite16(param->tag, qbt_map->tag);
		iowrite16(param->count, qbt_map->count);
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

		usleep_range(10000, 11000);

		count++;
		if (count > cnt_limit) {
			dpld->udl_check.total_size = 0;
			dpld->udl_check.rest_size = 0;
			mif_err("ERR! count %d\n", count);
			retval = -1;
			break;
		}
	}

	vfree(img);

	return retval;
}

static int qc_download_binary(struct dpram_link_device *dpld, void *arg)
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
		if (dpld->udl_check.total_size)
			tasklet_schedule(&dpld->dl_tsk);
		break;

	case 0xABCD:
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

		usleep_range(10000, 11000);

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

		usleep_range(10000, 11000);

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
	struct qc_dpram_boot_map *qbt_map = &dpld->qc_bt_map;
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

		usleep_range(1000, 2000);

		count++;
		if (count > 200) {
			mif_err("<%s:%d>\n", __func__, __LINE__);
			return -1;
		}
	}

	param->size = ioread16(qbt_map->frame_size);
	memcpy(param->addr, qbt_map->buff, param->size);
	param->tag = ioread16(qbt_map->tag);
	param->count = ioread16(qbt_map->count);

	dpld->send_intr(dpld, 0xDB12);

	return retval;
}

static int qc_uload_step1(struct dpram_link_device *dpld)
{
	int retval = 0;
	int count = 0;
	u16 intval = 0;
	u16 mask = 0;

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

		usleep_range(1000, 2000);

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
		err = qc_download_binary(dpld, (void *)arg);
		if (err < 0)
			mif_info("%s: ERR! download_binary fail\n", ld->name);
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

static irqreturn_t qc_dpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = (struct link_device *)&dpld->ld;
	u16 int2ap = 0;

	if (unlikely(ld->mode == LINK_MODE_OFFLINE))
		return IRQ_HANDLED;

	int2ap = dpld->recv_intr(dpld);

	if (int2ap == INT_POWERSAFE_FAIL) {
		mif_info("%s: int2ap == INT_POWERSAFE_FAIL\n", ld->name);
		goto exit;
	}

	if (int2ap == 0x1234 || int2ap == 0xDBAB || int2ap == 0xABCD) {
		qc_dload_cmd_handler(dpld, int2ap);
		goto exit;
	} else if (int2ap == 0x4321 || int2ap == 0x5432) {
		mif_err("%s: ERR! CP err cmd (err %d)\n", __func__, int2ap);
		goto exit;
	}

	if (likely(INT_VALID(int2ap)))
		dpld->ipc_rx_handler(dpld, int2ap);
	else
		mif_info("%s: ERR! invalid intr 0x%04X\n", ld->name, int2ap);

exit:
	return IRQ_HANDLED;
}
#endif

static struct dpram_ext_op ext_op_set[] = {
#ifdef CONFIG_CDMA_MODEM_CBP72
	[VIA_CBP72] = {
		.exist = 1,
		.init_boot_map = cbp72_init_boot_map,
		.init_dl_map = cbp72_init_dl_map,
		.download_binary = cbp72_download_binary,
		.dump_start = cbp72_dump_start,
		.dump_update = cbp72_dump_update,
		.ioctl = cbp72_ioctl,
	},
#endif
#ifdef CONFIG_LTE_MODEM_CMC221
	[SEC_CMC221] = {
		.exist = 1,
		.init_boot_map = cmc221_init_boot_map,
		.init_dl_map = cmc221_init_dl_map,
		.init_ul_map = cmc221_init_ul_map,
		.init_ipc_map = cmc221_init_ipc_map,
		.download_binary = cmc221_download_binary,
		.dump_start = cmc221_dump_start,
		.dump_update = cmc221_dump_update,
		.ioctl = cmc221_ioctl,
		.clear_intr = cmc221_idpram_clr_int2ap,
		.wakeup = cmc221_idpram_wakeup,
		.sleep = cmc221_idpram_sleep,
	},
#endif
#if defined(CONFIG_CDMA_MODEM_MDM6600)
	[QC_MDM6600] = {
		.exist = 1,
		.init_boot_map = qc_init_boot_map,
		.cp_start_handler = qc_start_handler,
		.crash_log = qc_crash_log,
		.ioctl = qc_ioctl,
		.irq_handler = qc_dpram_irq_handler,
	},
#endif
#if defined(CONFIG_GSM_MODEM_ESC6270)
	[QC_ESC6270] = {
		.exist = 1,
		.init_boot_map = qc_init_boot_map,
		.cp_start_handler = qc_start_handler,
		.crash_log = qc_crash_log,
		.ioctl = qc_ioctl,
		.irq_handler = qc_dpram_irq_handler,
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

