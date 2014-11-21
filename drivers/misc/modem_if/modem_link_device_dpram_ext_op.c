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
#include <linux/suspend.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_dpram.h"

#if defined(CONFIG_CDMA_MODEM_CBP72) || defined(CONFIG_CDMA_MODEM_CBP82)
static void cbp_init_boot_map(struct dpram_link_device *dpld)
{
	struct memif_boot_map *bt_map = &dpld->bt_map;

	bt_map->magic = (u32 *)dpld->base;
	bt_map->buff = (u8 *)(dpld->base + DP_BOOT_BUFF_OFFSET);
	bt_map->space = dpld->size - 4;
}

static void cbp_init_dl_map(struct dpram_link_device *dpld)
{
	dpld->dl_map.magic = (u32 *)dpld->base;
	dpld->dl_map.buff = (u8 *)(dpld->base + DP_DLOAD_BUFF_OFFSET);
}

static int cbp_udl_wait_resp(struct dpram_link_device *dpld, u32 resp)
{
	int ret;
	int int2ap;

	mif_debug("wait for 0x%04X\n", resp);
	ret = wait_for_completion_timeout(&dpld->udl_cmpl, UDL_TIMEOUT);
	if (!ret) {
		mif_info("ERR! No UDL_CMD_RESP!!!\n");
		return -EIO;
	}

	int2ap = dpld->recv_intr(dpld);
	if (resp == int2ap || int2ap == CMD_UL_RECV_DONE_REQ) {
		mif_debug("int2ap = 0x%04X\n", int2ap);
		return int2ap;
	} else {
		mif_err("ERR! int2ap 0x%04X != resp 0x%04X\n", int2ap, resp);
		return -EINVAL;
	}
}

static int cbp_xmit_binary(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = dpld->bt_map.buff;
	int err = 0;

	if (bf->len > dpld->bt_map.space) {
		mif_info("ERR! Out of DPRAM boundary\n");
		err = -ERANGE;
		goto exit;
	}

	if (bf->len)
		memcpy16_to_io(buff, bf->data, bf->len);

#ifdef CONFIG_CDMA_MODEM_CBP72
	init_completion(&dpld->udl_cmpl);
#endif

	if (bf->req) {
		dpld->send_intr(dpld, (u16)bf->req);
		mif_debug("send intr 0x%04X\n", (u16)bf->req);
	}

	if (bf->resp) {
		err = cbp_udl_wait_resp(dpld, bf->resp);
		if (err < 0) {
			mif_err("ERR! cbp_udl_wait_resp fail (err %d)\n", err);
			goto exit;
		} else if (err == bf->resp) {
			err = skb->len;
		}
	}

exit:
	dev_kfree_skb_any(skb);
	return err;
}

static int cbp_dump_start(struct dpram_link_device *dpld)
{
	u8 *dest = dpld->ul_map.buff;
	int ret;

	dpld->ld.mode = LINK_MODE_ULOAD;

	ret = del_timer(&dpld->crash_timer);
	wake_lock(&dpld->wlock);

	iowrite32(DP_MAGIC_UMDL, dpld->ul_map.magic);

	iowrite8((u8)START_FLAG, dest + 0);
	iowrite8((u8)0x1, dest + 1);
	iowrite8((u8)0x1, dest + 2);
	iowrite8((u8)0x0, dest + 3);
	iowrite8((u8)END_FLAG, dest + 4);

	init_completion(&dpld->crash_cmpl);

	return 0;
}

static int cbp_dump_update(struct dpram_link_device *dpld, unsigned long arg)
{
	struct dpram_dump_arg dump;
	struct dpram_udl_header header;
	unsigned char __user *target = (unsigned char __user *)arg;
	int err = 0;
	int resp = 0;
	u8 *dest = NULL;
	u8 *buff = NULL;
	u8 *header_buff = NULL;
	int buff_size = 0;
	u16 plen = 0;

	err = copy_from_user(&dump, (void __user *)arg, sizeof(dump));
	if (err < 0) {
		mif_err("ERR! ARG copy_from_user fail (err %d)\n", err);
		goto exit;
	}
	mif_debug("req %x, resp %x", dump.req, dump.resp);

	if (dump.req)
		dpld->send_intr(dpld, (u16)dump.req);

	if (dump.resp) {
		resp = err = cbp_udl_wait_resp(dpld, dump.resp);
		if (err < 0) {
			mif_info("ERR! wait_response fail (err %d)\n", err);
			goto exit;
		}
	}

	if (dump.cmd)
		goto exit;

	dest = (u8 *)dpld->ul_map.buff;

	header_buff = vmalloc(sizeof(struct dpram_udl_header));
	if (!header_buff) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy16_from_io(header_buff, dest, sizeof(struct dpram_udl_header));

	header.bop = *(u8 *)(header_buff);
	header.num_frames = *(u16 *)(header_buff + 1);
	header.curr_frame = *(u16 *)(header_buff + 3);
	header.len = *(u16 *)(header_buff + 5);
#ifdef CONFIG_CDMA_MODEM_CBP82
	header.pad = *(u8 *)(header_buff + 7);
#endif

	mif_debug("total frames:%d, current frame:%d, data len:%d\n",
		header.num_frames, header.curr_frame, header.len);

	plen = min_t(u16, header.len, DP_DEFAULT_DUMP_LEN);

	buff = vmalloc(DP_DEFAULT_DUMP_LEN);
	if (!buff) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy16_from_io(buff, dest + sizeof(struct dpram_udl_header), plen);
	err = copy_to_user(dump.buff, buff, plen);
	if (err) {
		mif_info("ERR! DUMP copy_to_user fail\n");
		err = -EIO;
		goto exit;
	}

	buff_size = plen;
	err = copy_to_user(target + 4, &buff_size, sizeof(int));
	if (err) {
		mif_info("ERR! SIZE copy_to_user fail\n");
		err = -EIO;
		goto exit;
	}

	/* Return response value */
	if (err == 0)
		err = resp;

	vfree(buff);
	return err;

exit:
	if (buff)
		vfree(buff);
	iowrite32(0, dpld->ul_map.magic);
	wake_unlock(&dpld->wlock);
	return err;
}
#endif

#if defined(CONFIG_LTE_MODEM_CMC221)
/*
** For CMC221 SFR for IDPRAM
*/
#define CMC_INT2CP_REG		0x10	/* Interrupt to CP            */
#define CMC_INT2AP_REG		0x50
#define CMC_CLR_INT_REG		0x28	/* Clear Interrupt to AP      */
#define CMC_RESET_REG		0x3C
#define CMC_PUT_REG		0x40	/* AP->CP reg for hostbooting */
#define CMC_GET_REG		0x50	/* CP->AP reg for hostbooting */

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

#define CMC22x_DUMP_BUFF_SIZE		8192

/* CMC221 IDPRAM SFR */
struct cmc221_idpram_sfr {
	u16 __iomem *int2cp;
	u16 __iomem *int2ap;
	u16 __iomem *clr_int2ap;
	u16 __iomem *reset;
	u16 __iomem *msg2cp;
	u16 __iomem *msg2ap;
};

struct cmc221_boot_img {
	char *addr;
	int size;
	enum cp_boot_mode mode;
	unsigned req;
	unsigned resp;
};

static struct cmc221_idpram_sfr cmc_sfr;

static void cmc221_init_boot_map(struct dpram_link_device *dpld)
{
	struct memif_boot_map *bt_map = &dpld->bt_map;

	bt_map->buff = dpld->base;
	bt_map->space = dpld->size;
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
	cmc_sfr.int2cp = (u16 __iomem *)(sfr_base + CMC_INT2CP_REG);
	cmc_sfr.int2ap = (u16 __iomem *)(sfr_base + CMC_INT2AP_REG);
	cmc_sfr.clr_int2ap = (u16 __iomem *)(sfr_base + CMC_CLR_INT_REG);
	cmc_sfr.reset = (u16 __iomem *)(sfr_base + CMC_RESET_REG);
	cmc_sfr.msg2cp = (u16 __iomem *)(sfr_base + CMC_PUT_REG);
	cmc_sfr.msg2ap = (u16 __iomem *)(sfr_base + CMC_GET_REG);

	/* Interrupt ports */
	dpld->ipc_map.mbx_cp2ap = cmc_sfr.int2ap;
	dpld->ipc_map.mbx_ap2cp = cmc_sfr.int2cp;
}

static inline void cmc221_idpram_reset(struct dpram_link_device *dpld)
{
	iowrite16(1, cmc_sfr.reset);
}

static inline u16 cmc221_idpram_recv_msg(struct dpram_link_device *dpld)
{
	return ioread16(cmc_sfr.msg2ap);
}

static inline void cmc221_idpram_send_msg(struct dpram_link_device *dpld,
					u16 msg)
{
	iowrite16(msg, cmc_sfr.msg2cp);
}

static int cmc221_idpram_wait_resp(struct dpram_link_device *dpld, u32 resp)
{
	int count = 50000;
	u32 rcvd = 0;

	if (resp == CMC22x_CP_REQ_NV_DATA) {
		while (1) {
			rcvd = ioread32(dpld->bt_map.resp);
			if (rcvd == resp)
				break;

			rcvd = cmc221_idpram_recv_msg(dpld);
			if (rcvd == 0x9999) {
				mif_info("invalid resp 0x%04X\n", rcvd);
				panic("CP Crash ... BAD CRC in CP");
			}

			if (count-- < 0) {
				mif_info("invalid resp 0x%08X\n", rcvd);
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
				mif_info("invalid resp CMC22x_CP_CAL_BAD\n");
				break;
			}

			if (count-- < 0) {
				mif_info("invalid resp 0x%04X\n", rcvd);
				return -EAGAIN;
			}

			udelay(100);
		}
	}

	return rcvd;
}

static int cmc221_xmit_boot(struct dpram_link_device *dpld, unsigned long arg)
{
	struct link_device *ld = &dpld->ld;
	u8 __iomem *bt_buff = dpld->bt_map.buff;
	struct cmc221_boot_img cp_img;
	u8 *img_buff = NULL;
	int err = 0;
	int cnt = 0;
	mif_info("+++\n");

	ld->mode = LINK_MODE_BOOT;

	dpld->dpram->setup_speed(DPRAM_SPEED_LOW);

	/* Test memory... After testing, memory is cleared. */
	if (mif_test_dpram(ld->name, bt_buff, dpld->bt_map.space) < 0) {
		mif_info("ERR! mif_test_dpram fail!\n");
		ld->mode = LINK_MODE_OFFLINE;
		return -EIO;
	}

	memset(&cp_img, 0, sizeof(struct cmc221_boot_img));

	/* Get information about the boot image */
	err = copy_from_user(&cp_img, (void __user *)arg, sizeof(cp_img));
	mif_info("CP image addr = 0x%08X, size = %d\n",
		(int)cp_img.addr, cp_img.size);

	/* Alloc a buffer for the boot image */
	img_buff = kzalloc(dpld->bt_map.space, GFP_KERNEL);
	if (!img_buff) {
		mif_info("ERR! kzalloc fail\n");
		ld->mode = LINK_MODE_OFFLINE;
		return -ENOMEM;
	}

	/* Copy boot image from the user space to the image buffer */
	err = copy_from_user(img_buff, (void __user *)cp_img.addr, cp_img.size);

	/* Copy boot image to DPRAM and verify it */
	memcpy(bt_buff, img_buff, cp_img.size);
	if (memcmp16_to_io(bt_buff, img_buff, cp_img.size)) {
		mif_info("ERR! Boot may be broken!!!\n");
		goto err;
	}

	cmc221_idpram_reset(dpld);
	usleep_range(1000, 2000);

	if (cp_img.mode == CP_BOOT_MODE_NORMAL) {
		mif_info("CP_BOOT_MODE_NORMAL\n");
		mif_info("send req 0x%08X\n", cp_img.req);
		iowrite32(cp_img.req, dpld->bt_map.req);

		/* Wait for cp_img.resp for up to 2 seconds */
		mif_info("wait resp 0x%08X\n", cp_img.resp);
		while (ioread32(dpld->bt_map.resp) != cp_img.resp) {
			cnt++;
			usleep_range(1000, 2000);

			if (cnt > 1000) {
				mif_info("ERR! invalid resp 0x%08X\n",
					ioread32(dpld->bt_map.resp));
				goto err;
			}
		}
	} else {
		mif_info("CP_BOOT_MODE_DUMP\n");
	}

	kfree(img_buff);

	mif_info("send BOOT done\n");

	dpld->dpram->setup_speed(DPRAM_SPEED_HIGH);

	mif_info("---\n");
	return 0;

err:
	ld->mode = LINK_MODE_OFFLINE;
	kfree(img_buff);

	mif_err("FAIL!!!\n");
	mif_info("---\n");
	return -EIO;
}

static int cmc221_idpram_download_bin(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	int err = 0;
	int ret = 0;
	struct dpram_boot_frame *bf = (struct dpram_boot_frame *)skb->data;
	u8 __iomem *buff = (dpld->bt_map.buff + bf->offset);

	if ((bf->offset + bf->len) > dpld->bt_map.space) {
		mif_info("ERR! out of DPRAM boundary\n");
		err = -EINVAL;
		goto exit;
	}

	if (bf->len)
		memcpy(buff, bf->data, bf->len);

	if (bf->req)
		cmc221_idpram_send_msg(dpld, (u16)bf->req);

	if (bf->resp) {
		err = cmc221_idpram_wait_resp(dpld, bf->resp);
		if (err < 0)
			mif_info("ERR! wait_resp fail (err %d)\n", err);
	}

	if (bf->req == CMC22x_CAL_NV_DOWN_END)
		mif_info("request CMC22x_CAL_NV_DOWN_END\n");

exit:
	if (err < 0)
		ret = err;
	else
		ret = skb->len;

	dev_kfree_skb_any(skb);

	return ret;
}

static int cmc221_xmit_binary(struct dpram_link_device *dpld,
			struct sk_buff *skb)
{
	if (dpld->type == CP_IDPRAM)
		return cmc221_idpram_download_bin(dpld, skb);
	else
		return -ENODEV;
}

static int cmc221_dump_start(struct dpram_link_device *dpld)
{
	dpld->ld.mode = LINK_MODE_ULOAD;

	del_timer(&dpld->crash_timer);
	wake_lock(&dpld->wlock);

	dpld->crash_rcvd = 0;
	iowrite32(CMC22x_CP_DUMP_MAGIC, dpld->ul_map.magic);

	init_completion(&dpld->crash_cmpl);

	return 0;
}

static void cmc221_idpram_wait_dump(unsigned long arg)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)arg;
	u16 msg;

	msg = cmc221_idpram_recv_msg(dpld);
	if (msg == CMC22x_CP_DUMP_END) {
		complete_all(&dpld->crash_cmpl);
		return;
	}

	if (((dpld->crash_rcvd & 0x1) == 0) && (msg == CMC22x_1ST_BUFF_FULL)) {
		complete_all(&dpld->crash_cmpl);
		return;
	}

	if (((dpld->crash_rcvd & 0x1) == 1) && (msg == CMC22x_2ND_BUFF_FULL)) {
		complete_all(&dpld->crash_cmpl);
		return;
	}

	mif_add_timer(&dpld->crash_timer, DUMP_WAIT_TIMEOUT,
		cmc221_idpram_wait_dump, (unsigned long)dpld);
}

static int cmc221_idpram_upload(struct dpram_link_device *dpld,
			struct dpram_dump_arg *dumparg)
{
	int ret;
	u8 __iomem *src;
	int buff_size = CMC22x_DUMP_BUFF_SIZE;

	if ((dpld->crash_rcvd & 0x1) == 0)
		cmc221_idpram_send_msg(dpld, CMC22x_1ST_BUFF_READY);
	else
		cmc221_idpram_send_msg(dpld, CMC22x_2ND_BUFF_READY);

	init_completion(&dpld->crash_cmpl);

	mif_add_timer(&dpld->crash_timer, DUMP_WAIT_TIMEOUT,
		cmc221_idpram_wait_dump, (unsigned long)dpld);

	ret = wait_for_completion_timeout(&dpld->crash_cmpl, DUMP_TIMEOUT);
	if (!ret) {
		mif_info("ERR! no dump from CP!!!\n");
		goto err_out;
	}

	if (cmc221_idpram_recv_msg(dpld) == CMC22x_CP_DUMP_END) {
		mif_info("recv CMC22x_CP_DUMP_END\n");
		return 0;
	}

	if ((dpld->crash_rcvd & 0x1) == 0)
		src = dpld->ul_map.buff;
	else
		src = dpld->ul_map.buff + CMC22x_DUMP_BUFF_SIZE;

	memcpy(dpld->buff, src, buff_size);

	ret = copy_to_user(dumparg->buff, dpld->buff, buff_size);
	if (ret < 0) {
		mif_info("ERR! copy_to_user fail\n");
		goto err_out;
	}

	dpld->crash_rcvd++;
	return buff_size;

err_out:
	return -EIO;
}

static int cmc221_dump_update(struct dpram_link_device *dpld, unsigned long arg)
{
	struct dpram_dump_arg dump;
	int ret;

	ret = copy_from_user(&dump, (void __user *)arg, sizeof(dump));
	if (ret  < 0) {
		mif_info("ERR! copy_from_user fail\n");
		return ret;
	}

	return cmc221_idpram_upload(dpld, &dump);
}

static void cmc221_idpram_clr_int2ap(struct dpram_link_device *dpld)
{
	iowrite16(0xFFFF, cmc_sfr.clr_int2ap);
}

static int cmc221_idpram_wakeup(struct dpram_link_device *dpld)
{
	int cnt = 0;

	gpio_set_value(dpld->gpio_cp_wakeup, 1);

	while (!gpio_get_value(dpld->gpio_cp_status)) {
		if (cnt++ > 10) {
			if (in_irq())
				mif_err("ERR! gpio_cp_status == 0 in IRQ\n");
			else
				mif_err("ERR! gpio_cp_status == 0\n");
			return -EACCES;
		}

		mif_info("gpio_cp_status == 0 (cnt %d)\n", cnt);
		if (in_interrupt())
			udelay(1000);
		else
			usleep_range(1000, 2000);
	}

	return 0;
}

static void cmc221_idpram_sleep(struct dpram_link_device *dpld)
{
	gpio_set_value(dpld->gpio_cp_wakeup, 0);
}
#endif

#if defined(CONFIG_CDMA_MODEM_MDM6600) || defined(CONFIG_GSM_MODEM_ESC6270)
enum qc_dload_tag {
	QC_DLOAD_TAG_NONE = 0,
	QC_DLOAD_TAG_BIN,
	QC_DLOAD_TAG_NV,
	QC_DLOAD_TAG_MAX
};

struct qc_dpram_boot_map {
	u8 __iomem *buff;
	u16 __iomem *frame_size;
	u16 __iomem *tag;
	u16 __iomem *count;
};

struct qc_dpram_udl_param {
	unsigned char *addr;
	unsigned int size;
	unsigned int count;
	unsigned int tag;
};

struct qc_dpram_udl_check {
	unsigned int total_size;
	unsigned int rest_size;
	unsigned int send_size;
	unsigned int copy_start;
	unsigned int copy_complete;
	unsigned int boot_complete;
};

static struct qc_dpram_boot_map qc_bt_map;
static struct qc_dpram_udl_param qc_udl_param;
static struct qc_dpram_udl_check qc_udl_check;

static void qc_dload_task(unsigned long data);

static void qc_init_boot_map(struct dpram_link_device *dpld)
{
	struct qc_dpram_boot_map *qbt_map = &qc_bt_map;
	struct modemlink_dpram_data *dpram = dpld->dpram;

	qbt_map->buff = dpld->base;
	qbt_map->frame_size = (u16 *)(dpld->base + dpram->boot_size_offset);
	qbt_map->tag = (u16 *)(dpld->base + dpram->boot_tag_offset);
	qbt_map->count = (u16 *)(dpld->base + dpram->boot_count_offset);

	tasklet_init(&dpld->dl_tsk, qc_dload_task, (unsigned long)dpld);
}

static int qc_prepare_download(struct dpram_link_device *dpld)
{
	int retval = 0;
	int count = 0;

	while (1) {
		if (qc_udl_check.copy_start) {
			qc_udl_check.copy_start = 0;
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

static void qc_do_download(struct dpram_link_device *dpld,
			struct qc_dpram_udl_param *param)
{
	struct qc_dpram_boot_map *qbt_map = &qc_bt_map;

	if (param->size <= dpld->dpram->max_boot_frame_size) {
		memcpy(qbt_map->buff, param->addr, param->size);
		iowrite16(param->size, qbt_map->frame_size);
		iowrite16(param->tag, qbt_map->tag);
		iowrite16(param->count, qbt_map->count);
		dpld->send_intr(dpld, 0xDB12);
	} else {
		mif_info("param->size %d\n", param->size);
	}
}

static int qc_download(struct dpram_link_device *dpld, void *arg,
			enum qc_dload_tag tag)
{
	int retval = 0;
	int count = 0;
	int cnt_limit;
	unsigned char *img;
	struct qc_dpram_udl_param param;

	retval = copy_from_user((void *)&param, (void __user *)arg,
				sizeof(param));
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

	qc_udl_check.total_size = param.size;
	qc_udl_check.rest_size = param.size;
	qc_udl_check.send_size = 0;
	qc_udl_check.copy_complete = 0;

	qc_udl_param.addr = img;
	qc_udl_param.size = dpld->dpram->max_boot_frame_size;
	if (tag == QC_DLOAD_TAG_NV)
		qc_udl_param.count = 1;
	else
		qc_udl_param.count = param.count;
	qc_udl_param.tag = tag;

	if (qc_udl_check.rest_size < dpld->dpram->max_boot_frame_size)
		qc_udl_param.size = qc_udl_check.rest_size;

	/* Download image (binary or NV) */
	qc_do_download(dpld, &qc_udl_param);

	/* Wait for completion
	*/
	if (tag == QC_DLOAD_TAG_NV)
		cnt_limit = 200;
	else
		cnt_limit = 1000;

	while (1) {
		if (qc_udl_check.copy_complete) {
			qc_udl_check.copy_complete = 0;
			retval = 0;
			break;
		}

		usleep_range(10000, 11000);

		count++;
		if (count > cnt_limit) {
			qc_udl_check.total_size = 0;
			qc_udl_check.rest_size = 0;
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
	return qc_download(dpld, arg, QC_DLOAD_TAG_BIN);
}

static int qc_download_nv(struct dpram_link_device *dpld, void *arg)
{
	return qc_download(dpld, arg, QC_DLOAD_TAG_NV);
}

static void qc_dload_task(unsigned long data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;

	qc_udl_check.send_size += qc_udl_param.size;
	qc_udl_check.rest_size -= qc_udl_param.size;

	qc_udl_param.addr += qc_udl_param.size;

	if (qc_udl_check.send_size >= qc_udl_check.total_size) {
		qc_udl_check.copy_complete = 1;
		qc_udl_param.tag = 0;
		return;
	}

	if (qc_udl_check.rest_size < dpld->dpram->max_boot_frame_size)
		qc_udl_param.size = qc_udl_check.rest_size;

	qc_udl_param.count += 1;

	qc_do_download(dpld, &qc_udl_param);
}

static void qc_dload_cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	switch (cmd) {
	case 0x1234:
		qc_udl_check.copy_start = 1;
		break;

	case 0xDBAB:
		if (qc_udl_check.total_size)
			tasklet_schedule(&dpld->dl_tsk);
		break;

	case 0xABCD:
		qc_udl_check.boot_complete = 1;
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
		if (qc_udl_check.boot_complete) {
			qc_udl_check.boot_complete = 0;
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
	static unsigned char buf[151];
	u8 __iomem *data = NULL;

	data = dpld->get_rxq_buff(dpld, IPC_FMT);
	memcpy(buf, data, (sizeof(buf) - 1));

	mif_info("PHONE ERR MSG\t| %s Crash\n", dpld->ld.mc->name);
	mif_info("PHONE ERR MSG\t| %s\n", buf);
}

static int qc_data_upload(struct dpram_link_device *dpld,
			struct qc_dpram_udl_param *param)
{
	struct qc_dpram_boot_map *qbt_map = &qc_bt_map;
	int retval = 0;
	u16 intval = 0;
	int count = 0;

	while (1) {
		if (!gpio_get_value(dpld->gpio_int2ap)) {
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
		if (!gpio_get_value(dpld->gpio_int2ap)) {
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
	struct qc_dpram_udl_param param;

	retval = copy_from_user((void *)&param, (void __user *)arg,
				sizeof(param));
	if (retval < 0) {
		mif_err("ERR! copy_from_user fail (err %d)\n", retval);
		return -1;
	}

	retval = qc_data_upload(dpld, &param);
	if (retval < 0) {
		mif_err("ERR! qc_data_upload fail (err %d)\n", retval);
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
	int err = 0;

	switch (cmd) {
	case IOCTL_DPRAM_PHONE_POWON:
		err = qc_prepare_download(dpld);
		if (err < 0)
			mif_info("ERR! prepare_download fail\n");
		break;

	case IOCTL_DPRAM_PHONEIMG_LOAD:
		err = qc_download_binary(dpld, (void *)arg);
		if (err < 0)
			mif_info("ERR! download_binary fail\n");
		break;

	case IOCTL_DPRAM_NVDATA_LOAD:
		err = qc_download_nv(dpld, (void *)arg);
		if (err < 0)
			mif_info("ERR! download_nv fail\n");
		break;

	case IOCTL_DPRAM_PHONE_BOOTSTART:
		err = qc_boot_start(dpld);
		if (err < 0) {
			mif_info("ERR! boot_start fail\n");
			break;
		}

		err = qc_boot_post_process(dpld);
		if (err < 0)
			mif_info("ERR! boot_post_process fail\n");

		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP1:
		disable_irq_nosync(dpld->irq);
		err = qc_uload_step1(dpld);
		if (err < 0) {
			enable_irq(dpld->irq);
			mif_info("ERR! upload_step1 fail\n");
		}
		break;

	case IOCTL_DPRAM_PHONE_UPLOAD_STEP2:
		err = qc_uload_step2(dpld, (void *)arg);
		if (err < 0) {
			enable_irq(dpld->irq);
			mif_info("ERR! upload_step2 fail\n");
		}
		break;

	default:
		mif_err("ERR! invalid cmd 0x%08X\n", cmd);
		err = -EINVAL;
		break;
	}

	return err;
}

static irqreturn_t qc_dpram_irq_handler(int irq, void *data)
{
	struct dpram_link_device *dpld = (struct dpram_link_device *)data;
	struct link_device *ld = &dpld->ld;
	struct mem_status stat;
	u16 int2ap = 0;

	if (ld->mode == LINK_MODE_OFFLINE) {
		int2ap = dpld->recv_intr(dpld);
		return IRQ_HANDLED;
	}

	dpld->get_dpram_status(dpld, RX, &stat);
	int2ap = stat.int2ap;

	if (int2ap == INT_POWERSAFE_FAIL) {
		mif_info("int2ap == INT_POWERSAFE_FAIL\n");
		goto exit;
	}

	if (int2ap == 0x1234 || int2ap == 0xDBAB || int2ap == 0xABCD) {
		qc_dload_cmd_handler(dpld, int2ap);
		goto exit;
	} else if (int2ap == 0x4321 || int2ap == 0x5432) {
		mif_err("ERR! CP error command (0x%04X)\n", int2ap);
		goto exit;
	}

	if (likely(INT_VALID(int2ap)))
		dpld->ipc_rx_handler(dpld, &stat);
	else
		mif_info("ERR! invalid intr 0x%04X\n", int2ap);

exit:
	return IRQ_HANDLED;
}
#endif

#if defined(CONFIG_CDMA_MODEM_QSC6085)
#define CMD_CP_RAMDUMP_START_REQ	0x9200
#define CMD_CP_RAMDUMP_SEND_REQ		0x9400
#define CMD_CP_RAMDUMP_SEND_DONE_REQ	0x9600

#define CMD_CP_RAMDUMP_START_RESP	0x0300
#define CMD_CP_RAMDUMP_SEND_RESP	0x0500
#define CMD_CP_RAMDUMP_SEND_DONE_RESP	0x0700

#define QSC_UPLOAD_MODE			(0x444D554C)
#define QSC_UPLOAD_MODE_COMPLETE	(0xABCDEF90)

#define RAMDUMP_CMD_TIMEOUT		(5 * HZ)
#define QSC6085_RAM_SIZE		(32 * 1024 * 1024) /* 32MB */

struct qsc6085_dump_command {
	u32 addr;
	u32 size;
	u32 copyto_offset;
};

struct qsc6085_dump_status {
	u32 dump_size;
	u32 addr;
	u32 rcvd;
	u32 rest;
};

static struct qsc6085_dump_status qsc_dump_stat;

static void qsc6085_dump_work(struct work_struct *work);

static void qsc6085_init_dl_map(struct dpram_link_device *dpld)
{
	dpld->dl_map.magic = (u32 *)dpld->base;
	dpld->dl_map.buff = (u8 *)(dpld->base + DP_DLOAD_BUFF_OFFSET);
}

static void qsc6085_init_ul_map(struct dpram_link_device *dpld)
{
	int magic_size = DP_ULOAD_MAGIC_SIZE;
	int cmd_size = sizeof(struct qsc6085_dump_command);
	int mbx_size = DP_MBX_SET_SIZE;

	dpld->ul_map.magic = (u32 *)dpld->base;
	dpld->ul_map.cmd = dpld->base + magic_size;
	dpld->ul_map.cmd_size = cmd_size;
	dpld->ul_map.buff = dpld->base + magic_size + cmd_size;
	dpld->ul_map.space = dpld->size - (magic_size + cmd_size + mbx_size);
}

static void qsc6085_req_active_handler(struct dpram_link_device *dpld)
{
	struct modem_ctl *mc = dpld->ld.mc;
	mif_info("pda_active = %d\n", gpio_get_value(mc->gpio_pda_active));
	dpld->send_intr(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
}

static void qsc6085_error_display_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;

	mif_err("recv 0xC9 (CRASH_EXIT)\n");
	mif_err("CP Crash: %s\n", dpld->get_rxq_buff(dpld, IPC_FMT));

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);
}

static void qsc6085_start_handler(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct io_device *iod;

	mif_info("recv 0xC8 (CP_START)\n");

	mif_info("send 0xC1 (INIT_START)\n");
	dpld->send_intr(dpld, INT_CMD(INT_CMD_INIT_START));

	dpld->reset_dpram_ipc(dpld);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (!iod) {
		mif_err("ERR! no iod\n");
		return;
	}
	iod->modem_state_changed(iod, STATE_ONLINE);

	mif_info("send 0xC2 (INIT_END)\n");
	dpld->send_intr(dpld, INT_CMD(INT_CMD_INIT_END));
}

static void qsc6085_command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		qsc6085_req_active_handler(dpld);
		break;

	case INT_CMD_ERR_DISPLAY:
#ifdef CONFIG_LINK_DEVICE_S5P_IDPRAM
		/* If modem crashes while PDA_SLEEP is in progres */
		dpld->pm_op->halt_suspend(dpld);
#endif
		qsc6085_error_display_handler(dpld);
		break;

	case INT_CMD_PHONE_START:
		qsc6085_start_handler(dpld);
		complete_all(&ld->init_cmpl);
		break;

#ifdef CONFIG_LINK_DEVICE_S5P_IDPRAM
	case INT_CMD_IDPRAM_SUSPEND_ACK:
		dpld->pm_op->power_down(dpld);
		break;

	case INT_CMD_IDPRAM_WAKEUP_START:
		dpld->pm_op->power_up(dpld);
		break;
#endif

	case INT_CMD_NORMAL_POWER_OFF:
		complete(&dpld->crash_cmpl);
		qsc6085_error_display_handler(dpld);
		break;

	default:
		mif_err("unknown command 0x%04X\n", cmd);
		break;
	}
}

static int qsc6085_download_firmware(struct dpram_link_device *dpld,
			struct modem_firmware *fw)
{
	int ret = 0;
	char __user *src = fw->binary;
	int rest = fw->size;
	char __iomem *dst = NULL;
	unsigned long timeout;
	u16 curr_frame = 0;
	u16 len = 0;
	struct dpram_udl_header header;

	header.bop = START_FLAG;
	header.num_frames = DIV_ROUND_UP(len, DP_DEFAULT_WRITE_LEN);
	mif_err("FW %d bytes = %d frames\n", fw->size, header.num_frames);

	while (rest > 0) {
		curr_frame++;
		len = min(rest, DP_DEFAULT_WRITE_LEN);

		header.curr_frame = curr_frame;
		header.len = len;
		mif_info(">>> frame# %u, len %u\n", curr_frame, len);

		dst = dpld->dl_map.buff;
		memcpy(dst, &header, sizeof(header));

		dst += sizeof(header);
		ret = copy_from_user(dst, (void __user *)src, len);
		if (ret < 0) {
			mif_err("copy_from_user fail\n");
			return -EIO;
		}

		dst += len;
		src += len;
		rest -= len;

		iowrite8(END_FLAG, (dst+3));

		if (curr_frame == 1) {
			dpld->send_intr(dpld, 0);
			timeout = UDL_TIMEOUT;
		} else {
			dpld->send_intr(dpld, CMD_DL_SEND_REQ);
			timeout = UDL_SEND_TIMEOUT;
		}

		ret = wait_for_completion_timeout(&dpld->udl_cmpl, timeout);
		if (!ret) {
			mif_err("ERR! no response from CP\n");
			return -EIO;
		}
	}

	mif_err("send CMD_DL_DONE_REQ to CP\n");
	dpld->send_intr(dpld, CMD_DL_DONE_REQ);

	ret = wait_for_completion_timeout(&dpld->udl_cmpl, UDL_TIMEOUT);
	if (!ret) {
		mif_err("ERR! no response from CP\n");
		return -EIO;
	}

	return 0;
}

static int qsc6085_dload_firmware(struct dpram_link_device *dpld,
			unsigned long arg)
{
	int ret;
	struct modem_firmware fw;
	mif_err("+++\n");

	ret = copy_from_user(&fw, (void __user *)arg, sizeof(fw));
	if (ret < 0) {
		mif_err("ERR! copy_from_user fail!\n");
		return ret;
	}

	ret = qsc6085_download_firmware(dpld, &fw);

	mif_err("---\n");
	return ret;
}

static int qsc6085_dump_start(struct dpram_link_device *dpld)
{
	int ret;
	struct link_device *ld = &dpld->ld;
	struct modem_ctl *mc = ld->mc;
	struct qsc6085_dump_status *dump_stat = &qsc_dump_stat;
	mif_err("+++\n");

	init_completion(&dpld->crash_cmpl);
	INIT_DELAYED_WORK(&dpld->crash_dwork, qsc6085_dump_work);

	iowrite32(QSC_UPLOAD_MODE, &dpld->ul_map.magic);

	/* reset modem so that it goes to upload mode */
	/* ap does not need to reset cp during CRASH_EXIT case */
	if (gpio_get_value(mc->gpio_phone_active))
		mc->ops.modem_reset(mc);

	dpld->send_intr(dpld, CMD_CP_RAMDUMP_START_REQ);
	ret = wait_for_completion_timeout(&dpld->crash_cmpl,
					RAMDUMP_CMD_TIMEOUT);
	if (!ret) {
		mif_err("ERR! no response to CP_RAMDUMP_START_REQ\n");
		dump_stat->dump_size = 0;
	} else {
		dump_stat->dump_size = QSC6085_RAM_SIZE;
		dump_stat->addr = 0;
		dump_stat->rcvd = 0;
		dump_stat->rest = dump_stat->dump_size;
	}

	queue_delayed_work(system_nrt_wq, &dpld->crash_dwork, 0);

	mif_err("---\n");
	return 0;
}

static int qsc6085_dump_update(struct dpram_link_device *dpld,
			unsigned long arg)
{
	int ret;
	struct link_device *ld = &dpld->ld;
	struct io_device *iod = link_get_iod_with_format(ld, IPC_RAMDUMP);
	struct memif_uload_map *ul_map = &dpld->ul_map;
	struct qsc6085_dump_status *dump_stat = &qsc_dump_stat;
	struct qsc6085_dump_command dump_cmd;

	while (iod->sk_rx_q.qlen > 0)
		usleep_range(1000, 1100);

	memset(&dump_cmd, 0, sizeof(dump_cmd));
	dump_cmd.addr = dump_stat->addr;
	dump_cmd.size = min(dump_stat->rest, ul_map->space);
	dump_cmd.copyto_offset = 0x38000010;

	memcpy_toio(ul_map->cmd, &dump_cmd, ul_map->cmd_size);

	dpld->send_intr(dpld, CMD_CP_RAMDUMP_SEND_REQ);
	ret = wait_for_completion_timeout(&dpld->crash_cmpl,
					RAMDUMP_CMD_TIMEOUT);
	if (!ret) {
		dump_stat->dump_size = 0;
		mif_err("ERR! no response to CP_RAMDUMP_SEND_REQ\n");
		ret = -EIO;
		goto exit;
	}

	memcpy_fromio(dpld->buff, ul_map->buff, dump_cmd.size);

	ret = iod->recv(iod, ld, dpld->buff, dump_cmd.size);
	if (ret < 0)
		goto exit;

	dump_stat->addr += dump_cmd.size;
	dump_stat->rcvd += dump_cmd.size;
	dump_stat->rest -= dump_cmd.size;
	mif_info("rest = %u bytes\n", dump_stat->rest);

	ret = dump_cmd.size;

exit:
	return ret;
}

static void qsc6085_dump_work(struct work_struct *work)
{
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct qsc6085_dump_status *dump_stat = &qsc_dump_stat;
	int ret;

	dpld = container_of(work, struct dpram_link_device, crash_dwork.work);
	ld = &dpld->ld;

	ret = qsc6085_dump_update(dpld, 0);
	if (ret > 0 && dump_stat->rest > 0)
		queue_delayed_work(system_nrt_wq, &dpld->crash_dwork, 0);
}

static int qsc6085_dump_finish(struct dpram_link_device *dpld,
			unsigned long arg)
{
	int ret;
	struct completion *cmpl = &dpld->crash_cmpl;
	mif_err("+++\n");

	init_completion(cmpl);

	dpld->send_intr(dpld, CMD_CP_RAMDUMP_SEND_DONE_REQ);

	ret = wait_for_completion_timeout(cmpl, RAMDUMP_CMD_TIMEOUT);
	if (!ret) {
		mif_err("ERR! no response to CP_RAMDUMP_SEND_DONE_REQ\n");
		ret = -EIO;
	}

	mif_err("---\n");
	return ret;
}
#endif

static struct dpram_ext_op ext_op_set[MAX_MODEM_TYPE] = {
#ifdef CONFIG_CDMA_MODEM_CBP72
	[VIA_CBP72] = {
		.exist = 1,
		.init_boot_map = cbp_init_boot_map,
		.init_dl_map = cbp_init_dl_map,
		.xmit_binary = cbp_xmit_binary,
		.dump_start = cbp_dump_start,
		.dump_update = cbp_dump_update,
	},
#endif
#ifdef CONFIG_CDMA_MODEM_CBP82
	[VIA_CBP82] = {
		.exist = 1,
		.init_boot_map = cbp_init_boot_map,
		.init_dl_map = cbp_init_dl_map,
		.xmit_binary = cbp_xmit_binary,
		.dump_start = cbp_dump_start,
		.dump_update = cbp_dump_update,
	},
#endif
#ifdef CONFIG_LTE_MODEM_CMC221
	[SEC_CMC221] = {
		.exist = 1,
		.init_boot_map = cmc221_init_boot_map,
		.init_dl_map = cmc221_init_dl_map,
		.init_ul_map = cmc221_init_ul_map,
		.init_ipc_map = cmc221_init_ipc_map,
		.xmit_boot = cmc221_xmit_boot,
		.xmit_binary = cmc221_xmit_binary,
		.dump_start = cmc221_dump_start,
		.dump_update = cmc221_dump_update,
		.clear_int2ap = cmc221_idpram_clr_int2ap,
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
#if defined(CONFIG_CDMA_MODEM_QSC6085)
	[QC_QSC6085] = {
		.exist = 1,
		.init_dl_map = qsc6085_init_dl_map,
		.init_ul_map = qsc6085_init_ul_map,
		.cmd_handler = qsc6085_command_handler,
		.firm_update = qsc6085_dload_firmware,
		.dump_start = qsc6085_dump_start,
		.dump_update = qsc6085_dump_update,
		.dump_finish = qsc6085_dump_finish,
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

#ifdef CONFIG_LINK_DEVICE_S5P_IDPRAM
#define GPIO_IDPRAM_SFN		S3C_GPIO_SFN(2)

#define MAX_CHECK_RETRY_CNT	5
#define MAX_RESUME_TRY_CNT	5

static bool s5p_idpram_is_pm_locked(struct dpram_link_device *dpld)
{
	struct modem_ctl *mc = dpld->ld.mc;
	struct idpram_pm_data *pm_data = &dpld->pm_data;

	/* If PM is in SUSPEND */
	if (atomic_read(&pm_data->pm_lock) > 0) {
		mif_info("in SUSPEND\n");
		return true;
	}

	/* If AP is in or into LPA */
	if (!gpio_get_value(mc->gpio_pda_active)) {
		mif_info("in LPA\n");
		return true;
	}

	return false;
}

static void s5p_idpram_set_pm_lock(struct dpram_link_device *dpld, int lock)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;

	/* 0 = unlock, 1 = lock */
	switch (lock) {
	case 0:
		if (atomic_read(&pm_data->pm_lock))
			atomic_set(&pm_data->pm_lock, lock);
		break;

	case 1:
		if (!atomic_read(&pm_data->pm_lock))
			atomic_set(&pm_data->pm_lock, lock);
		break;

	default:
		break;
	}
}

static void s5p_idpram_try_resume(struct work_struct *work)
{
	struct idpram_pm_data *pm_data;
	struct dpram_link_device *dpld;
	struct link_device *ld;
	unsigned long delay;
	u16 cmd;
	mif_info("+++\n");

	pm_data = container_of(work, struct idpram_pm_data, resume_dwork.work);
	dpld = container_of(pm_data, struct dpram_link_device, pm_data);
	ld = &dpld->ld;

	if (pm_data->last_msg == INT_CMD(INT_CMD_IDPRAM_RESUME_REQ)) {
		pm_data->last_msg = 0;

		s5p_idpram_set_pm_lock(dpld, 0);
		wake_unlock(&pm_data->hold_wlock);

		delay = msecs_to_jiffies(10);
		schedule_delayed_work(&pm_data->tx_dwork, delay);

		mif_info("%s resumed\n", ld->name);
		goto exit;
	}

	if (pm_data->resume_try_cnt++ < MAX_RESUME_TRY_CNT) {
		mif_info("%s not resumed yet\n", ld->name);

		cmd = INT_CMD(INT_CMD_IDPRAM_RESUME_REQ);
		mif_info("send IDPRAM_RESUME_REQ (0x%X)\n", cmd);
		dpld->send_intr(dpld, cmd);

		delay = msecs_to_jiffies(200);
		schedule_delayed_work(&pm_data->resume_dwork, delay);
	} else {
		struct io_device *iod;
		mif_err("ERR! %s resume T-I-M-E-O-U-T\n", ld->name);

		iod = link_get_iod_with_format(ld, IPC_FMT);
		if (iod)
			iod->modem_state_changed(iod, STATE_CRASH_EXIT);

		wake_unlock(&pm_data->hold_wlock);

		/* hold wakelock until uevnet sent to rild */
		wake_lock_timeout(&pm_data->hold_wlock, HZ*7);
		s5p_idpram_set_pm_lock(dpld, 0);
	}

exit:
	mif_info("---\n");
}

static irqreturn_t s5p_cp_dump_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t s5p_ap_wakeup_irq_handler(int irq, void *data)
{
	struct idpram_pm_data *pm_data = data;
	wake_lock_timeout(&pm_data->ap_wlock, HZ*5);
	return IRQ_HANDLED;
}

static void s5p_idpram_power_down(struct dpram_link_device *dpld)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	mif_info("+++\n");

	pm_data->last_msg = INT_CMD(INT_CMD_IDPRAM_SUSPEND_ACK);
	complete(&pm_data->down_cmpl);

	mif_info("---\n");
}

static void s5p_idpram_power_up(struct dpram_link_device *dpld)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	mif_info("+++\n");

	pm_data->last_msg = INT_CMD(INT_CMD_IDPRAM_RESUME_REQ);
	pm_data->pm_state = IDPRAM_PM_ACTIVE;

	mif_info("---\n");
}

static void s5p_idpram_halt_suspend(struct dpram_link_device *dpld)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	mif_info("+++\n");

	complete(&pm_data->down_cmpl);

	mif_info("---\n");
}

static int s5p_idpram_prepare_suspend(struct dpram_link_device *dpld)
{
	struct link_device *ld = &dpld->ld;
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	struct modem_ctl *mc = dpld->ld.mc;
	struct completion *cmpl;
	unsigned long timeout;
	unsigned long rest;
	int cnt = 0;
	u16 cmd = INT_CMD(INT_CMD_IDPRAM_SUSPEND_REQ);
	mif_info("+++\n");

	pm_data->pm_state = IDPRAM_PM_SUSPEND_PREPARE;
	pm_data->last_msg = 0;
	s5p_idpram_set_pm_lock(dpld, 1);

	/*
	* Because, if dpram was powered down, cp dpram random intr was
	* ocurred. so, fixed by muxing cp dpram intr pin to GPIO output
	* high,..
	*/
	gpio_set_value(dpld->gpio_int2cp, 1);
	s3c_gpio_cfgpin(dpld->gpio_int2cp, S3C_GPIO_OUTPUT);

	/* prevent PDA_ACTIVE status is low */
	gpio_set_value(mc->gpio_pda_active, 1);

	cmpl = &pm_data->down_cmpl;
	timeout = IDPRAM_SUSPEND_REQ_TIMEOUT;
	cnt = 0;
	do {
		init_completion(cmpl);

		mif_info("send IDPRAM_SUSPEND_REQ (0x%X)\n", cmd);
		dpld->send_intr(dpld, cmd);

		rest = wait_for_completion_timeout(cmpl, timeout);
		if (rest == 0) {
			cnt++;
			mif_err("timeout!!! (count = %d)\n", cnt);
			if (cnt >= 3) {
				mif_err("ERR! no response from CP\n");
				break;
			}
		}
	} while (rest == 0);

	switch (pm_data->last_msg) {
	case INT_CMD(INT_CMD_IDPRAM_SUSPEND_ACK):
		mif_info("recv IDPRAM_SUSPEND_ACK (0x%X)\n", pm_data->last_msg);
		pm_data->pm_state = IDPRAM_PM_DPRAM_POWER_DOWN;
		break;

	default:
		mif_err("ERR! %s down or not ready!!! (intr 0x%04X)\n",
			ld->name, dpld->recv_intr(dpld));
		timeout = msecs_to_jiffies(500);
		wake_lock_timeout(&pm_data->hold_wlock, timeout);
		s5p_idpram_set_pm_lock(dpld, 0);
		break;
	}

	mif_info("---\n");
	return 0;
}

static int s5p_idpram_resume_init(struct dpram_link_device *dpld)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	mif_info("+++\n");

	pm_data->pm_state = IDPRAM_PM_RESUME_START;
	pm_data->last_msg = 0;

	dpld->reset_dpram_ipc(dpld);

	/* re-initialize internal dpram gpios */
	s3c_gpio_cfgpin(dpld->gpio_int2cp, GPIO_IDPRAM_SFN);

	mif_info("---\n");
	return 0;
}

static int s5p_idpram_start_resume(struct dpram_link_device *dpld)
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	struct modem_ctl *mc = dpld->ld.mc;
	unsigned long delay;
	mif_info("+++ (pm_state = %d)\n", pm_data->pm_state);

	switch (pm_data->pm_state) {
	/* schedule_work */
	case IDPRAM_PM_DPRAM_POWER_DOWN:
		gpio_set_value(mc->gpio_pda_active, 0);
		msleep(50);

		s5p_idpram_resume_init(dpld);
		msleep(50);

		gpio_set_value(mc->gpio_pda_active, 1);
		msleep(20);

		pm_data->resume_try_cnt = 0;
		wake_lock(&pm_data->hold_wlock);

		delay = msecs_to_jiffies(20);
		schedule_delayed_work(&pm_data->resume_dwork, delay);
		break;

	case IDPRAM_PM_RESUME_START:
	case IDPRAM_PM_SUSPEND_PREPARE:
	default:
		break;
	}

	mif_info("---\n");
	return 0;
}

static int s5p_idpram_notify_pm_event(struct notifier_block *this,
			unsigned long event, void *v)
{
	struct idpram_pm_data *pm_data;
	struct dpram_link_device *dpld;
	int err;
	mif_info("+++ (event 0x%08X)\n", (int)event);

	pm_data = container_of(this, struct idpram_pm_data, pm_noti);
	dpld = container_of(pm_data, struct dpram_link_device, pm_data);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		err = s5p_idpram_prepare_suspend(dpld);
		break;

	case PM_POST_SUSPEND:
		err = s5p_idpram_start_resume(dpld);
		break;

	default:
		break;
	}

	mif_info("---\n");
	return NOTIFY_DONE;
}

static int s5p_idpram_pm_init(struct dpram_link_device *dpld,
	struct modem_data *modem, void (*pm_tx_func)(struct work_struct *work))
{
	struct idpram_pm_data *pm_data = &dpld->pm_data;
	int err;
	unsigned gpio;
	unsigned irq;
	mif_info("+++\n");

	atomic_set(&pm_data->pm_lock, 0);

	init_completion(&pm_data->down_cmpl);

	wake_lock_init(&pm_data->ap_wlock, WAKE_LOCK_SUSPEND, "ap_wakeup");
	wake_lock_init(&pm_data->hold_wlock, WAKE_LOCK_SUSPEND, "dpram_hold");

	INIT_DELAYED_WORK(&pm_data->tx_dwork, pm_tx_func);
	INIT_DELAYED_WORK(&pm_data->resume_dwork, s5p_idpram_try_resume);

	pm_data->resume_try_cnt = 0;

	/* register PM notifier */
	pm_data->pm_noti.notifier_call = s5p_idpram_notify_pm_event;
	register_pm_notifier(&pm_data->pm_noti);

	/*
	** Register gpio_ap_wakeup interrupt handler
	*/
	gpio = modem->gpio_ap_wakeup;
	irq = gpio_to_irq(gpio);
	mif_info("gpio_ap_wakeup: GPIO# %d, IRQ# %d\n", gpio, irq);

	err = request_irq(irq, s5p_ap_wakeup_irq_handler, IRQF_TRIGGER_RISING,
			"idpram_ap_wakeup", (void *)pm_data);
	if (err) {
		mif_err("ERR! request_irq(#%d) fail (err %d)\n", irq, err);
		goto exit;
	}

	err = enable_irq_wake(irq);
	if (err) {
		mif_err("ERR! enable_irq_wake(#%d) fail (err %d)\n", irq, err);
		free_irq(irq, (void *)pm_data);
		goto exit;
	}

	/*
	** Register gpio_cp_dump_int interrupt handler for LPA mode
	*/
	gpio = modem->gpio_cp_dump_int;
	irq = gpio_to_irq(gpio);
	mif_info("gpio_cp_dump_int: GPIO# %d, IRQ# %d\n", gpio, irq);

	err = request_irq(irq, s5p_cp_dump_irq_handler, IRQF_TRIGGER_RISING,
			"idpram_cp_dump", (void *)pm_data);
	if (err) {
		mif_err("ERR! request_irq(#%d) fail (err %d)\n", irq, err);
		free_irq(gpio_to_irq(modem->gpio_ap_wakeup), (void *)pm_data);
		goto exit;
	}

	err = enable_irq_wake(irq);
	if (err) {
		mif_err("ERR! enable_irq_wake(#%d) fail (err %d)\n", irq, err);
		free_irq(gpio_to_irq(modem->gpio_cp_dump_int), (void *)pm_data);
		free_irq(gpio_to_irq(modem->gpio_ap_wakeup), (void *)pm_data);
		goto exit;
	}

exit:
	mif_err("---\n");
	return err;
}

static bool s5p_idpram_int2cp_possible(struct dpram_link_device *dpld)
{
	struct modem_ctl *mc = dpld->ld.mc;
	int i;
	int level;

	for (i = 1; i <= MAX_CHECK_RETRY_CNT; i++) {
		level = gpio_get_value(dpld->gpio_int2cp);
		if (level)
			break;

		/* CP has not yet received previous command. */
		mif_info("gpio_ipc_int2cp == 0 (count %d)\n", i);

		usleep_range(1000, 1100);
	}

	for (i = 1; i <= MAX_CHECK_RETRY_CNT; i++) {
		level = gpio_get_value(mc->gpio_pda_active);
		if (level)
			break;

		/* AP is in transition to LPA mode. */
		mif_info("gpio_pda_active == 0 (count %d)\n", i);

		usleep_range(1000, 1100);
	}

	return true;
}
#endif

static struct idpram_pm_op idpram_pm_op_set[MAX_AP_TYPE] = {
#ifdef CONFIG_LINK_DEVICE_S5P_IDPRAM
	[S5P] = {
		.pm_init = s5p_idpram_pm_init,
		.power_down = s5p_idpram_power_down,
		.power_up = s5p_idpram_power_up,
		.halt_suspend = s5p_idpram_halt_suspend,
		.locked = s5p_idpram_is_pm_locked,
		.int2cp_possible = s5p_idpram_int2cp_possible,
	},
#endif
};

struct idpram_pm_op *idpram_get_pm_op(enum ap_type ap)
{
	if (idpram_pm_op_set[ap].exist)
		return &idpram_pm_op_set[ap];
	else
		return NULL;
}

