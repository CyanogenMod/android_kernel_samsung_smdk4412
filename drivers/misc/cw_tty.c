/* linux/arch/arm/mach-xxxx/board-c1ctc-modems.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
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

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/reboot.h>
#include <linux/keyboard.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/time.h>
#include <linux/slab.h>
#include <linux/unistd.h>

#define ID_CW_TTY_CWPPP			36
#define ID_CW_TTY_CWXAN			41

#define CW_TTY_MAJOR_NUM		235

/* Maximum PDP data length */
#define MAX_PDP_DATA_LEN		1500

/* Maximum PDP packet length including header and start/stop bytes */
#define MAX_PDP_PACKET_LEN		(MAX_PDP_DATA_LEN + 4 + 2)

#define MAX_CW_TTY_BUFFER_LEN		(8192*2)

struct cw_tty_info {
	/* PDP context ID */
	u8			id;

	/* Tx packet buffer */
	u8			tx_buf[MAX_CW_TTY_BUFFER_LEN];

	/* App device interface */
	union {
		/* Virtual serial interface */
		struct {
			struct tty_driver	tty_driver[2];
			int			refcount;
			struct tty_struct	*tty_table[1];
			struct ktermios		*termios[1];
			struct ktermios		*termios_locked[1];
			char			tty_name[64];
			struct tty_struct	*tty;
			struct semaphore	write_lock;
		} vs_u;
	} dev_u;
#define vs_dev		dev_u.vs_u
};

struct cw_tty_info *cw_tty[2];

struct cw_tty_info *cw_tty_get_dev_by_id(int id)
{
	struct cw_tty_info *dev;

	if (id == ID_CW_TTY_CWPPP)
		dev = cw_tty[0];
	else if (id == ID_CW_TTY_CWXAN)
		dev = cw_tty[1];
	else
		dev = NULL;

	return dev;
}

struct cw_tty_info *cw_tty_get_dev_by_name(const char *name)
{
	struct cw_tty_info *dev;

	if (strcmp("ttyCWPPP", name) == 0)
		dev = cw_tty[0];
	else if (strcmp("ttyCWXAN", name) == 0)
		dev = cw_tty[1];
	else
		dev = NULL;

	return dev;
}

/* Added by Venkatesh GR SISO Received PPP Data is sent to ppp */
static int XanCwReceivedWiFiData(int nDestChannelID,
				u8 *pi8_RecvdBuf, int nRecvdBuflen)
{
	int ret = 0;
	struct cw_tty_info *dev = NULL;

	if (pi8_RecvdBuf == NULL) {
		pr_err("[%s] input buffer is NULL\n", __func__);
		return 0;
	}

	dev = cw_tty_get_dev_by_id(nDestChannelID);
	if (dev == NULL) {
		pr_err("[%s] cw_tty_get_dev_by_id is NULL\n", __func__);
		return 0;
	}

	if (dev->vs_dev.tty != NULL && dev->vs_dev.refcount) {
		ret = tty_insert_flip_string(dev->vs_dev.tty,
					(u8 *)pi8_RecvdBuf, nRecvdBuflen);
		if (ret < nRecvdBuflen)
			pr_err("[%s] tty_insert_flip_string fail %d\n",
								__func__, ret);

		tty_flip_buffer_push(dev->vs_dev.tty);
	} else
		pr_err("[%s] refcount: %d\n", __func__,
						dev->vs_dev.refcount);

	return ret;
}

/* Added by Venkatesh GR SISO Received PPP Data from pppd to Xanadu */
static int XanCwSendToXanadu(int nSrcChannelID,
		int nDestChannelID, u8 *pi8_RecvdBuf, int nRecvdBuflen)
{
	int ret = 0;
	struct cw_tty_info *dev = NULL;
	int nXanChannelID = nDestChannelID;

	if (pi8_RecvdBuf == NULL) {
		pr_err("[%s] input buffer is NULL\n", __func__);
		return 0;
	}

	dev = cw_tty_get_dev_by_id(nXanChannelID);
	if (dev == NULL) {
		pr_err("[%s] cw_tty_get_dev_by_id is NULL\n", __func__);
		return 0;
	}

	if (dev->vs_dev.tty != NULL && dev->vs_dev.refcount) {
		ret = tty_insert_flip_string(dev->vs_dev.tty,
					(u8 *)pi8_RecvdBuf, nRecvdBuflen);
		if (ret < nRecvdBuflen)
			pr_err("[%s] tty_insert_flip_string returned %d\n",
								__func__, ret);

		tty_flip_buffer_push(dev->vs_dev.tty);
	} else
		pr_err("[%s] refcount: %d\n", __func__,
						dev->vs_dev.refcount);

	return ret;
}


static int cw_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct cw_tty_info *dev;

	pr_info("[%s] name: %s, current: %s\n", __func__,
					tty->driver->name, current->comm);

	dev = cw_tty_get_dev_by_name(tty->driver->name);
	if (dev == NULL) {
		pr_err("[%s] cw_tty_get_dev_by_name is NULL\n", __func__);
		return -ENODEV;
	}

	tty->driver_data = (void *)dev;
	tty->low_latency = 1;
	dev->vs_dev.tty = tty;
	dev->vs_dev.refcount++;

	pr_info("[%s] name:%s, refcount: %d\n", __func__,
				tty->driver->name, dev->vs_dev.refcount);

	return 0;
}

static void cw_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct cw_tty_info *dev;

	dev = cw_tty_get_dev_by_name(tty->driver->name);
	if (dev == NULL) {
		pr_err("[%s] cw_tty_get_dev_by_name is NULL\n", __func__);
		return;
	}

	dev->vs_dev.refcount--;
	pr_info("[%s] name: %s, refcount: %d\n", __func__,
				tty->driver->name, dev->vs_dev.refcount);

	return;
}

static int cw_tty_write(struct tty_struct *tty,
			const unsigned char *buf, int count)
{
	int ret = 0;
	struct cw_tty_info *dev = (struct cw_tty_info *)tty->driver_data;
	int nDestChannelID = 0;

	/* Added by Venkatesh GR, SISO For CDMA+WiFi requirements -- Start*/
	if (strcmp(dev->vs_dev.tty_name, "ttyCWPPP") == 0) {
		nDestChannelID = ID_CW_TTY_CWXAN;

		/* Send the Buffer Received From PPPD to CWTunnel */
		ret = XanCwSendToXanadu(dev->id, nDestChannelID,
							(u8 *)buf, count);

	} else if (strcmp(dev->vs_dev.tty_name, "ttyCWXAN") == 0) {
		nDestChannelID = ID_CW_TTY_CWPPP;

		/* Send the Buffer Received From CWTunnel to PPPD */
		ret = XanCwReceivedWiFiData(nDestChannelID, (u8 *)buf, count);
	}

	return ret;
}

static int cw_tty_write_room(struct tty_struct *tty)
{
	return MAX_CW_TTY_BUFFER_LEN;
}

static int cw_tty_chars_in_buffer(struct tty_struct *tty)
{
	return 0;
}

static int cw_tty_ioctl(struct tty_struct *tty,
		    unsigned int cmd, unsigned long arg)
{
	pr_info("[%s] name: %s cmd: %x\n", __func__,
					tty->driver->name, cmd);

	return -ENOIOCTLCMD;
}

static const struct tty_operations cw_tty_ops = {
	.open = cw_tty_open,
	.close = cw_tty_close,
	.write = cw_tty_write,
	.write_room = cw_tty_write_room,
	.chars_in_buffer = cw_tty_chars_in_buffer,
	.ioctl = cw_tty_ioctl,
};

static int __init cw_tty_init(void)
{
	struct tty_driver	*tty_driver;
	struct cw_tty_info  *dev;

	dev = kmalloc(sizeof(struct cw_tty_info)
			+ MAX_PDP_PACKET_LEN, GFP_KERNEL);
	if (dev == NULL) {
		pr_err("[%s] out of memory\n", __func__);
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct cw_tty_info));

	dev->id = ID_CW_TTY_CWPPP;
	strcpy(dev->vs_dev.tty_name, "ttyCWPPP");
	tty_driver = &dev->vs_dev.tty_driver[0];

	kref_init(&tty_driver->kref);

	tty_driver->magic = TTY_DRIVER_MAGIC;
	tty_driver->driver_name	= "ttyCW";
	tty_driver->name = "ttyCWPPP";
	tty_driver->major = CW_TTY_MAJOR_NUM;
	tty_driver->minor_start = dev->id;
	tty_driver->num		= 1;
	tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	tty_driver->subtype = SERIAL_TYPE_NORMAL;
	tty_driver->flags = TTY_DRIVER_REAL_RAW;
	tty_driver->ttys	= dev->vs_dev.tty_table;
	tty_driver->termios	= dev->vs_dev.termios;
	tty_driver->termios_locked	= dev->vs_dev.termios_locked;
	tty_set_operations(tty_driver, &cw_tty_ops);

	if (tty_register_driver(tty_driver)) {
		put_tty_driver(tty_driver);
		pr_err("[%s] Couldn't register ttyCWPPP driver\n", __func__);
		return -ENOMEM;
	}
	sema_init(&dev->vs_dev.write_lock, 1);
	cw_tty[0] = dev;

	dev = kmalloc(sizeof(struct cw_tty_info)
			+ MAX_PDP_PACKET_LEN, GFP_KERNEL);
	if (dev == NULL) {
		pr_err("[%s] out of memory\n", __func__);
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct cw_tty_info));

	dev->id = ID_CW_TTY_CWXAN;
	strcpy(dev->vs_dev.tty_name, "ttyCWXAN");
	tty_driver = &dev->vs_dev.tty_driver[1];

	kref_init(&tty_driver->kref);

	tty_driver->magic = TTY_DRIVER_MAGIC;
	tty_driver->driver_name	= "ttyCW";
	tty_driver->name = "ttyCWXAN";
	tty_driver->major = CW_TTY_MAJOR_NUM;
	tty_driver->minor_start = dev->id;
	tty_driver->num		= 1;
	tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	tty_driver->subtype = SERIAL_TYPE_NORMAL;
	tty_driver->flags = TTY_DRIVER_REAL_RAW;
	tty_driver->ttys	= dev->vs_dev.tty_table;
	tty_driver->termios	= dev->vs_dev.termios;
	tty_driver->termios_locked	= dev->vs_dev.termios_locked;
	tty_set_operations(tty_driver, &cw_tty_ops);

	if (tty_register_driver(tty_driver)) {
		put_tty_driver(tty_driver);
		pr_err("[%s] Couldn't register ttyCWXAN driver\n", __func__);
		return -ENOMEM;
	}
	sema_init(&dev->vs_dev.write_lock, 1);
	cw_tty[1] = dev;

	return 0;
}

late_initcall(cw_tty_init);


