/*
 * Copyright (C) 2011 Google, Inc.
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
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/platform_data/modem_na.h>
#include <linux/io.h>
#include "modem_prj.h"
#include "modem_link_device_dpram.h"

/* interrupt masks.*/
#define INT_MASK_VALID			0x0080
#define INT_MASK_CMD			0x0040
#define INT_MASK_REQ_ACK_F		0x0020
#define INT_MASK_REQ_ACK_R		0x0010
#define INT_MASK_RES_ACK_F		0x0008
#define INT_MASK_RES_ACK_R		0x0004
#define INT_MASK_SEND_F			0x0002
#define INT_MASK_SEND_R			0x0001
#define INT_VALID(x)			((x) & INT_MASK_VALID)
#define INT_CMD_VALID(x)		((x) & INT_MASK_CMD)
#define INT_NON_CMD(x)			(INT_MASK_VALID | (x))
#define INT_CMD(x)			(INT_MASK_VALID | INT_MASK_CMD | (x))

#define INT_CMD_MASK(x)			((x) & 0xF)
#define INT_CMD_INIT_START		0x1
#define INT_CMD_INIT_END		0x2
#define INT_CMD_REQ_ACTIVE		0x3
#define INT_CMD_RES_ACTIVE		0x4
#define INT_CMD_REQ_TIME_SYNC		0x5
#define INT_CMD_PHONE_START		0x8
#define INT_CMD_ERR_DISPLAY		0x9
#define INT_CMD_PHONE_DEEP_SLEEP	0xA
#define INT_CMD_NV_REBUILDING		0xB
#define INT_CMD_EMER_DOWN		0xC
#define INT_CMD_PIF_INIT_DONE		0xD
#define INT_CMD_SILENT_NV_REBUILDING	0xE
#define INT_CMD_NORMAL_POWER_OFF	0xF

/* special interrupt cmd indicating modem boot failure. */
#define INT_POWERSAFE_FAIL              0xDEAD

#define GOTA_CMD_VALID(x)		(((x) & 0xA000) == 0xA000)
#define GOTA_RESULT_FAIL		0x2
#define GOTA_RESULT_SUCCESS		0x1
#define GOTA_CMD_MASK(x)		(((x) >> 8) & 0xF)
#define GOTA_CMD_RECEIVE_READY		0x1
#define GOTA_CMD_DOWNLOAD_START_REQ	0x2
#define GOTA_CMD_DOWNLOAD_START_RESP	0x3
#define GOTA_CMD_IMAGE_SEND_REQ		0x4
#define GOTA_CMD_IMAGE_SEND_RESP	0x5
#define GOTA_CMD_SEND_DONE_REQ		0x6
#define GOTA_CMD_SEND_DONE_RESP		0x7
#define GOTA_CMD_STATUS_UPDATE		0x8
#define GOTA_CMD_UPDATE_DONE		0x9
#define GOTA_CMD_EFS_CLEAR_RESP		0xB
#define GOTA_CMD_ALARM_BOOT_OK		0xC
#define GOTA_CMD_ALARM_BOOT_FAIL	0xD

#define CMD_DL_START_REQ		0x9200
#define CMD_IMG_SEND_REQ		0x9400
#define CMD_DL_SEND_DONE_REQ		0x9600
#define CMD_UL_RECEIVE_RESP		0x9601
#define CMD_UL_RECEIVE_DONE_RESP	0x9801

#define START_INDEX			0x7F
#define END_INDEX			0x7E

#define DP_MAGIC_CODE			0xAA
#define DP_MAGIC_DMDL			0x4445444C
#define DP_MAGIC_UMDL			0x4445444D
#define DP_DPRAM_SIZE			0x4000
#define DP_DEFAULT_WRITE_LEN		8168
#define DP_DEFAULT_DUMP_LEN		16366
#ifdef CONFIG_INTERNAL_MODEM_IF
#define DP_DUMP_HEADER_SIZE		8
#else
#define DP_DUMP_HEADER_SIZE             7
#endif
#define GOTA_TIMEOUT			(50 * HZ)
#define GOTA_SEND_TIMEOUT		(200 * HZ)
#define DUMP_TIMEOUT			(30 * HZ)
#define DUMP_START_TIMEOUT		(100 * HZ)
#define IDPRAM_PHY_START    0x13A00000
#define IDPRAM_SIZE 0x4000



static int
dpram_download(struct dpram_link_device *dpld, const char *buf, int len);
static int
dpram_upload(struct dpram_link_device *dpld, struct dpram_firmware *uploaddata);
static inline void
dpram_writeh(u16 value,  void __iomem *p_dest);
static void
dpram_clear(struct dpram_link_device *dpld);
static struct io_device *
dpram_find_iod(struct dpram_link_device *dpld, int id);
static void
dpram_write_command(struct dpram_link_device *dpld, u16 cmd);
static inline int
dpram_readh(void __iomem *p_dest);

#ifdef CONFIG_INTERNAL_MODEM_IF

#define INT_MASK_CMD_PDA_SLEEP	0x0C
#define INT_MASK_CMD_DPRAM_DOWN 0x0C
#define INT_MASK_CMD_PDA_WAKEUP 0x0A
#define INT_MASK_CMD_CP_WAKEUP_START 0x0A
#define INT_MASK_CMD_DPRAM_DOWN_NACK	0x07

#include <plat/gpio-cfg.h>
#include <linux/suspend.h>

struct idpram_link_pm_data *pm;

void idpram_magickey_init(struct idpram_link_pm_data *pm_data)
{
	u16 acc_code = 0x01;

	dpram_writeh(DP_MAGIC_CODE, &pm_data->dpld->dpram->magic);
	dpram_writeh(acc_code, &pm_data->dpld->dpram->enable);
}

int idpram_get_write_lock(struct idpram_link_pm_data *pm_data)
{
	return atomic_read(&pm_data->write_lock);
}

static int idpram_write_lock(struct idpram_link_pm_data *pm_data, int lock)
{
	int lock_value = 0;

	mif_info("MIF: idpram write_lock(%d)\n", lock);

	switch (lock) {
	case 0:		/* unlock */
		if (atomic_read(&pm_data->write_lock))
			lock_value = atomic_dec_return(&pm_data->write_lock);
		if (lock_value)
			mif_err("MIF: ipdram write unlock but lock value=%d\n",
			lock_value);
		break;
	case 1:		/* lock */
		if (!atomic_read(&pm_data->write_lock))
			lock_value = atomic_inc_return(&pm_data->write_lock);
		if (lock_value != 1)
			mif_err("MIF: ipdram write lock but lock value=%d\n",
				lock_value);
		break;
	}
	return 0;
}

static int idpram_resume_init(struct idpram_link_pm_data *pm_data)
{

	pm_data->pm_states = IDPRAM_PM_RESUME_START;
	pm_data->last_pm_mailbox = 0;

	dpram_clear(pm_data->dpld);
	idpram_magickey_init(pm_data);

	/* Initialize the dpram controller */
	pm_data->mdata->sfr_init();

	/*re-initialize internal dpram gpios */
	s3c_gpio_cfgpin(pm_data->mdata->gpio_mbx_intr, S3C_GPIO_SFN(0x2));

	idpram_write_lock(pm_data, 0);
	return 0;
}


void idpram_timeout_handler(struct idpram_link_pm_data *pm_data)
{
	struct io_device *iod = dpram_find_iod(pm_data->dpld, FMT_IDX);

	mif_info("MIF: <%s>", __func__);

	if (!gpio_get_value(pm_data->mdata->gpio_phone_active)) {
		mif_err("MIF: <%s:%s> (Crash silent Reset)\n",
			__func__, pm_data->dpld->ld.name);

		if (iod && iod->modem_state_changed)
			iod->modem_state_changed(iod, STATE_CRASH_EXIT);
	}
}

static int idpram_resume_check(struct idpram_link_pm_data *pm_data)
{
	/* check last pm mailbox */
	mif_info("MIF: idpram %s, last_pm_mailbox=%x\n", __func__,
		pm_data->last_pm_mailbox);

	if (pm_data->last_pm_mailbox == INT_CMD(INT_MASK_CMD_PDA_WAKEUP)) {
		pm_data->last_pm_mailbox = 0;
		return 0;
	}

	dpram_write_command(pm_data->dpld, INT_CMD(INT_MASK_CMD_PDA_WAKEUP));
	mif_info("MIF: idpram sent PDA_WAKEUP Mailbox(0x%x)\n",
		INT_CMD(INT_MASK_CMD_PDA_WAKEUP));

	return -1;
}

static void idpram_resume_retry(struct work_struct *work)
{
	struct idpram_link_pm_data *pm_data =
		container_of(work, struct idpram_link_pm_data, \
		resume_work.work);

	mif_debug("MIF: %s\n", __func__);

	if (!idpram_resume_check(pm_data)) {
		mif_info("MIF: idpram resume ok\n");
		idpram_write_lock(pm_data, 0);
		wake_lock_timeout(&pm_data->hold_wlock, msecs_to_jiffies(20));
		return;
	}
	if (pm_data->resume_retry--) {
		schedule_delayed_work(&pm_data->resume_work, \
			msecs_to_jiffies(200));
		wake_lock_timeout(&pm_data->hold_wlock, msecs_to_jiffies(260));
	} else {
		mif_info("MIF: idpram resume T-I-M-E-O-UT\n");
		idpram_timeout_handler(pm_data);
		/* hold wakelock until uevnet sent to rild */
		wake_lock_timeout(&pm_data->hold_wlock, HZ*7);
		idpram_write_lock(pm_data, 0);
	}
}


static irqreturn_t link_ap_wakeup_handler(int irq, void *data)
{
	struct idpram_link_pm_data *pm_data = data;

	mif_info("MIF: <%s> 5 seconds.\n", __func__);
	wake_lock_timeout(&pm_data->host_wakeup_wlock, 5*HZ);

	return IRQ_HANDLED;
}

static int idpram_pm_suspend(struct device *dev)
{
	struct idpram_link_pm_data *pm_data = pm;

	pm_data->pm_states = IDPRAM_PM_SUSPEND_START;
	gpio_set_value(pm_data->mdata->gpio_pda_active, 0);

	mif_debug("MIF: <%s>\n", __func__);

	return 0;
}
static int idpram_pm_resume(struct device *dev)
{
	struct idpram_link_pm_data *pm_data = pm;

	idpram_resume_init(pm_data);
	gpio_set_value(pm_data->mdata->gpio_pda_active, 1);
	mif_debug("MIF: <%s>\n", __func__);
	return 0;
}

static int __devinit idpram_pm_probe(struct platform_device *pdev)
{
	return 0;
}
static void idpram_pm_shutdown(struct platform_device *pdev)
{
}

static const struct dev_pm_ops idpram_pm_ops = {
	.suspend    = idpram_pm_suspend,
	.resume     = idpram_pm_resume,
};

static struct platform_driver idpram_pm_driver = {
	.probe = idpram_pm_probe,
	.shutdown = idpram_pm_shutdown,
	.driver = {
		.name = "idparam_pm",
		.pm   = &idpram_pm_ops,
	},
};

static void idpram_powerup_start(struct idpram_link_pm_data *pm_data)
{
	pm_data->last_pm_mailbox = INT_CMD(INT_MASK_CMD_PDA_WAKEUP);
	pm_data->pm_states = IDPRAM_PM_ACTIVE;
	mif_debug("MIF: <%s>\n", __func__);
}

static void idpram_power_down_nack(struct idpram_link_pm_data *pm_data)
{
	pm_data->last_pm_mailbox = INT_CMD(INT_MASK_CMD_DPRAM_DOWN_NACK);
	complete(&pm_data->idpram_down);
	mif_debug("MIF: <%s>\n", __func__);
}

static void idpram_power_down(struct idpram_link_pm_data *pm_data)
{
	pm_data->last_pm_mailbox = INT_CMD(INT_MASK_CMD_DPRAM_DOWN);
	complete(&pm_data->idpram_down);
	mif_debug("MIF: <%s>\n", __func__);
}

static int idpram_post_resume(struct idpram_link_pm_data *pm_data)
{
	int gpio_val = 0;

	mif_info("MIF: idpram %s\n", __func__);

	switch (pm_data->pm_states) {
	/* schedule_work */
	case IDPRAM_PM_DPRAM_POWER_DOWN:
		gpio_set_value(pm_data->mdata->gpio_pda_active, 0);
		mif_info("MIF: idpram PDA_ACTIVE LOW\n");

		msleep(50);

		idpram_resume_init(pm_data);

		msleep(50);

		gpio_set_value(pm_data->mdata->gpio_pda_active, 1);

		msleep(20);

		gpio_val = gpio_get_value(pm_data->mdata->gpio_pda_active);
		mif_info("MIF: idpram PDA_ACTIVE (%d)\n", gpio_val);

		if (gpio_val == 0) {
			gpio_set_value(pm_data->mdata->gpio_pda_active, 1);
			mif_info("MIF: idpram PDA_ACTIVE set again.\n");
		}
		break;

	case IDPRAM_PM_RESUME_START:
		break;

	case IDPRAM_PM_SUSPEND_PREPARE:
		break;
	}
	return 0;
}


static int idpram_pre_suspend(struct idpram_link_pm_data *pm_data)
{
	int timeout_ret = 0;
	int suspend_retry = 2;
	u16 intr_out = INT_CMD(INT_MASK_CMD_PDA_SLEEP);

	pm_data->pm_states = IDPRAM_PM_SUSPEND_PREPARE;
	pm_data->last_pm_mailbox = 0;
	idpram_write_lock(pm_data, 1);

	gpio_set_value(pm_data->mdata->gpio_mbx_intr, 1);

	/* prevent PDA_ACTIVE ststus is low */
	gpio_set_value(pm_data->mdata->gpio_pda_active, 1);

	if (!atomic_read(&pm_data->read_lock)) {
		do {
			init_completion(&pm_data->idpram_down);
			dpram_write_command(pm_data->dpld, intr_out);
			mif_err("MIF: idpram sent PDA_SLEEP Mailbox(0x%X)\n",
				intr_out);
			timeout_ret =
			wait_for_completion_timeout(&pm_data->idpram_down,
				(HZ/5));
			mif_err("MIF: suspend_enter cnt = %d\n",
				suspend_retry);
		} while (!timeout_ret && suspend_retry--);

		switch (pm_data->last_pm_mailbox) {
		case INT_CMD(INT_MASK_CMD_DPRAM_DOWN):
			break;

		/* if nack or other interrup, hold wakelock for DPM resume*/
		case INT_CMD(INT_MASK_CMD_DPRAM_DOWN_NACK):
			mif_err("MIF: idpram dpram down get NACK\n");

		default:
			mif_err("MIF: CP dpram Down not ready! intr=0x%X\n",
				dpram_readh(&pm_data->dpld->dpram->mbx_cp2ap));
			wake_lock_timeout(&pm_data->hold_wlock,
				msecs_to_jiffies(500));
			idpram_write_lock(pm_data, 0);
			return 0;
		}
		/*
		* Because, if dpram was powered down, cp dpram random intr was
		* ocurred. so, fixed by muxing cp dpram intr pin to GPIO output
		* high,..
		*/
		gpio_set_value(pm_data->mdata->gpio_mbx_intr, 1);
		s3c_gpio_cfgpin(pm_data->mdata->gpio_mbx_intr, S3C_GPIO_OUTPUT);
		pm_data->pm_states = IDPRAM_PM_DPRAM_POWER_DOWN;

		return 0;
	} else {
		mif_err("MIF: idpram hold read_lock\n");
		return -EBUSY;
	}
}

static int idpram_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	int err;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		mif_debug("MIF: PM_SUSPEND_PREPARE+\n");
		err = idpram_pre_suspend(pm);
		if (err)
			mif_err("MIF: pre-suspend err\n");
		break;

	case PM_POST_SUSPEND:
		mif_debug("MIF: PM_POST_SUSPEND+\n");
		err = idpram_post_resume(pm);
		if (err)
			mif_err("MIF: pre-suspend err\n");
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block idpram_link_pm_notifier = {
	.notifier_call = idpram_notifier_event,
};

static int idpram_link_pm_init
(
struct dpram_link_device *idpram_ld,
struct platform_device *pdev
)
{
	int r = 0;
	unsigned irq = 0;
	struct modem_data *pdata =
			(struct modem_data *)pdev->dev.platform_data;

	mif_info("MIF: <%s>\n", __func__);

	pm = kzalloc(sizeof(struct idpram_link_pm_data), GFP_KERNEL);
	if (!pm) {
		mif_err("MIF: %s: link_pm_data is NULL\n", __func__);
		return -ENOMEM;
	}

	pm->mdata = pdata;
	pm->dpld = idpram_ld;
	idpram_ld->link_pm_data = pm;

	/*for pm notifire*/
	register_pm_notifier(&idpram_link_pm_notifier);


	init_completion(&pm->idpram_down);
	wake_lock_init(&pm->host_wakeup_wlock,
		WAKE_LOCK_SUSPEND,
		"HOST_WAKEUP_WLOCK");
	wake_lock_init(&pm->rd_wlock, WAKE_LOCK_SUSPEND, "dpram_pwrdn");
	wake_lock_init(&pm->hold_wlock, WAKE_LOCK_SUSPEND, "dpram_hold");
	wake_lock_init(&pm->wakeup_wlock, WAKE_LOCK_SUSPEND, "dpram_wakeup");
	atomic_set(&pm->read_lock, 0);
	atomic_set(&pm->write_lock, 0);
	INIT_DELAYED_WORK(&pm->resume_work, idpram_resume_retry);

	r = platform_driver_register(&idpram_pm_driver);
	if (r) {
		mif_err("MIF: wakelocks_init: platform_driver_register failed\n");
		goto err_platform_driver_register;
	}

	irq = gpio_to_irq(pdata->gpio_ap_wakeup);
	r = request_irq(irq, link_ap_wakeup_handler,
		IRQF_TRIGGER_RISING,
		"idpram_host_wakeup", (void *)pm);

	mif_info("MIF: <%s> DPRAM IRQ# = %d, %d\n", __func__,
		irq,
		pm->mdata->gpio_ap_wakeup);

	if (r) {
		mif_err("MIF: %s:fail to request irq(%d) host_wake_irq\n",
			__func__, r);
		goto err_request_irq;
	}

	r = enable_irq_wake(irq);
	if (r) {
		mif_err("MIF: %s: failed to enable_irq_wake:%d host_wake_irq\n",
			__func__, r);
		goto err_set_wake_irq;
	}

	mif_info("MIF: <%s> END\n", __func__);
	return 0;

err_set_wake_irq:
	free_irq(irq, (void *)pm);
err_request_irq:
	platform_driver_unregister(&idpram_pm_driver);
err_platform_driver_register:
	kfree(pm);
	return r;
}
#endif /*CONFIG_INTERNAL_MODEM_IF*/



static inline int dpram_readh(void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;
	return ioread16(dest);
}

static inline void dpram_writew(u32 value,  void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;
	iowrite32(value, dest);
}

static inline void dpram_writeh(u16 value,  void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;
	iowrite16(value, dest);
}

static inline void dpram_writeb(u8 value,  void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;
	iowrite8(value, dest);
}


static void dpram_write_command(struct dpram_link_device *dpld, u16 cmd)
{
	dpram_writeh(cmd, &dpld->dpram->mbx_ap2cp);
}

static void dpram_clear_interrupt(struct dpram_link_device *dpld)
{
	dpram_writeh(0, &dpld->dpram->mbx_cp2ap);
}

static void dpram_drop_data(struct dpram_device *device, u16 head)
{
	dpram_writeh(head, &device->in->tail);
}

static void dpram_zero_circ(struct dpram_circ *circ)
{
	dpram_writeh(0, &circ->head);
	dpram_writeh(0, &circ->tail);
}

static void dpram_clear(struct dpram_link_device *dpld)
{
	dpram_zero_circ(&dpld->dpram->fmt_out);
	dpram_zero_circ(&dpld->dpram->raw_out);
	dpram_zero_circ(&dpld->dpram->fmt_in);
	dpram_zero_circ(&dpld->dpram->raw_in);
}

static bool dpram_circ_valid(int size, u16 head, u16 tail)
{
	if (head >= size) {
		mif_err("MIF: head(%d) >= size(%d)\n", head, size);
		return false;
	}
	if (tail >= size) {
		mif_err("MIF: tail(%d) >= size(%d)\n", tail, size);
		return false;
	}
	return true;
}

static int dpram_init_and_report(struct dpram_link_device *dpld)
{
	const u16 init_end = INT_CMD(INT_CMD_INIT_END);
	u16 magic;
	u16 enable;

	dpram_writeh(0, &dpld->dpram->enable);
	dpram_clear(dpld);
	dpram_writeh(DP_MAGIC_CODE, &dpld->dpram->magic);
	dpram_writeh(1, &dpld->dpram->enable);

	/* Send init end code to modem */
	dpram_write_command(dpld, init_end);

	magic = dpram_readh(&dpld->dpram->magic);
	if (magic != DP_MAGIC_CODE) {
		mif_err("MIF:: %s: Failed to check magic\n", __func__);
		return -1;
	}

	enable = dpram_readh(&dpld->dpram->enable);
	if (!enable) {
		mif_err("MIF:: %s: DPRAM enable failed\n", __func__);
		return -1;
	}

	return 0;
}

static struct io_device *dpram_find_iod(struct dpram_link_device *dpld, int id)
{
	struct io_device *iod;

	list_for_each_entry(iod, &dpld->list_of_io_devices, list) {
		if ((id == FMT_IDX && iod->format == IPC_FMT) ||
				(id == RAW_IDX && iod->format == IPC_MULTI_RAW))
			return iod;
	}

	return NULL;
}

static void cmd_req_active_handler(struct dpram_link_device *dpld)
{
	dpram_write_command(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
}

static void cmd_error_display_handler(struct dpram_link_device *dpld)
{
	struct io_device *iod = dpram_find_iod(dpld, FMT_IDX);

	mif_info("MIF: Received 0xc9 from modem (CP Crash)\n");
	mif_info("MIF: %s\n", dpld->dpram->fmt_in_buff);

	if (iod && iod->modem_state_changed)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);
}

static void cmd_phone_start_handler(struct dpram_link_device *dpld)
{
	mif_debug("MIF: Received 0xc8 from modem (Boot OK)\n");
	complete_all(&dpld->dpram_init_cmd);
	dpram_init_and_report(dpld);
}

static void cmd_nv_rebuild_handler(struct dpram_link_device *dpld)
{
		struct io_device *iod = dpram_find_iod(dpld, FMT_IDX);

		mif_info("MIF: Received nv rebuilding from modem\n");
		mif_info("MIF: %s\n", dpld->dpram->fmt_in_buff);

		if (iod && iod->modem_state_changed)
			iod->modem_state_changed(iod, STATE_NV_REBUILDING);
}


static void command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	mif_debug("MIF: %s: %x\n", __func__, cmd);

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		cmd_req_active_handler(dpld);
		break;

	case INT_CMD_ERR_DISPLAY:
		cmd_error_display_handler(dpld);
		break;

	case INT_CMD_PHONE_START:
		cmd_phone_start_handler(dpld);
		break;

	case INT_CMD_NV_REBUILDING:
		mif_err("[MODEM_IF] NV_REBUILDING\n");
		cmd_nv_rebuild_handler(dpld);
		break;

	case INT_CMD_PIF_INIT_DONE:
		complete_all(&dpld->modem_pif_init_done);
		break;

	case INT_CMD_SILENT_NV_REBUILDING:
		mif_err("[MODEM_IF] SILENT_NV_REBUILDING\n");
		break;

#ifdef CONFIG_INTERNAL_MODEM_IF
	case INT_MASK_CMD_DPRAM_DOWN:
		idpram_power_down(dpld->link_pm_data);
		break;

	case INT_MASK_CMD_DPRAM_DOWN_NACK:
		idpram_power_down_nack(dpld->link_pm_data);
		break;

	case INT_MASK_CMD_CP_WAKEUP_START:
		idpram_powerup_start(dpld->link_pm_data);
		break;
#else
	case INT_CMD_NORMAL_POWER_OFF:
		/*ToDo:*/
		/*kernel_sec_set_cp_ack()*/;
		break;

	case INT_CMD_REQ_TIME_SYNC:
	case INT_CMD_PHONE_DEEP_SLEEP:
	case INT_CMD_EMER_DOWN:
		break;
#endif

	default:
		mif_err("Unknown command.. %x\n", cmd);
	}
}


static int dpram_process_modem_update(struct dpram_link_device *dpld,
					struct dpram_firmware *pfw)
{
	int ret = 0;
	char *buff = vmalloc(pfw->size);

	mif_debug("[GOTA] modem size =[%d]\n", pfw->size);

	if (!buff)
		return -ENOMEM;

	ret = copy_from_user(buff, pfw->firmware, pfw->size);
	if (ret < 0) {
		mif_err("[%s:%d] Copy from user failed\n", __func__, __LINE__);
		goto out;
	}

	ret = dpram_download(dpld, buff, pfw->size);
	if (ret < 0)
		mif_err("firmware write failed\n");

out:
	vfree(buff);
	return ret;
}


static int dpram_modem_update(struct link_device *ld, struct io_device *iod,
							unsigned long _arg)
{
	int ret;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_firmware fw;

	mif_debug("[GOTA] dpram_modem_update\n");

	ret = copy_from_user(&fw, (void __user *)_arg, sizeof(fw));
	if (ret  < 0) {
		mif_err("copy from user failed!");
		return ret;
	}

	return dpram_process_modem_update(dpld, &fw);
}

static int dpram_dump_update(struct link_device *ld, struct io_device *iod,
							unsigned long _arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_firmware *fw = (struct dpram_firmware *)_arg ;

	mif_debug("MIF: dpram_dump_update()\n");

	return dpram_upload(dpld, fw);
}

static int dpram_read(struct dpram_link_device *dpld,
		      struct dpram_device *device, int dev_idx)
{
	struct io_device *iod;
	int size;
	int tmp_size;
	u16 head, tail;
	char *buff;

	head = dpram_readh(&device->in->head);
	tail = dpram_readh(&device->in->tail);
	mif_debug("=====> %s,  head: %d, tail: %d\n", __func__, head, tail);

	if (head == tail) {
		mif_err("MIF: %s: head == tail\n", __func__);
		goto err_dpram_read;
	}

	if (!dpram_circ_valid(device->in_buff_size, head, tail)) {
		mif_err("MIF: %s: invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		goto err_dpram_read;
	}

	iod = dpram_find_iod(dpld, dev_idx);
	if (!iod) {
		mif_err("MIF: iod == NULL\n");
		goto err_dpram_read;
	}

	/* Get data size in DPRAM*/
	size = (head > tail) ? (head - tail) :
		(device->in_buff_size - tail + head);

	/* ----- (tail) 7f 00 00 7e (head) ----- */
	if (head > tail) {
		buff = device->in_buff_addr + tail;
		if (iod->recv(iod, buff, size) < 0) {
			mif_err("MIF: %s: recv error, dropping data\n",
								__func__);
			dpram_drop_data(device, head);
			goto err_dpram_read;
		}
	} else { /* 00 7e (head) ----------- (tail) 7f 00 */
		/* 1. tail -> buffer end.*/
		tmp_size = device->in_buff_size - tail;
		buff = device->in_buff_addr + tail;
		if (iod->recv(iod, buff, tmp_size) < 0) {
			mif_err("MIF: %s: recv error, dropping data\n",
								__func__);
			dpram_drop_data(device, head);
			goto err_dpram_read;
		}

		/* 2. buffer start -> head.*/
		if (size > tmp_size) {
			buff = (char *)device->in_buff_addr;
			if (iod->recv(iod, buff, (size - tmp_size)) < 0) {
				mif_err("MIF: %s: recv error, dropping data\n",
								__func__);
				dpram_drop_data(device, head);
				goto err_dpram_read;
			}
		}
	}

	/* new tail */
	tail = (u16)((tail + size) % device->in_buff_size);
	dpram_writeh(tail, &device->in->tail);

	return size;

err_dpram_read:
	return -EINVAL;
}

static void non_command_handler(struct dpram_link_device *dpld,
				u16 non_cmd)
{
	struct dpram_device *device = NULL;
	u16 head, tail;
	u16 magic, access;
	int ret = 0;

	mif_debug("MIF: Entering non_command_handler(0x%04X)\n", non_cmd);

	magic = dpram_readh(&dpld->dpram->magic);
	access = dpram_readh(&dpld->dpram->enable);

	if (!access || magic != DP_MAGIC_CODE) {
		mif_err("fmr recevie error!!!!  access = 0x%x, magic =0x%x",
				access, magic);
		return;
	}

	/* Check formatted data region */
	device = &dpld->dev_map[FMT_IDX];
	head = dpram_readh(&device->in->head);
	tail = dpram_readh(&device->in->tail);

	if (!dpram_circ_valid(device->in_buff_size, head, tail)) {
		mif_err("MIF: %s: invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		return;
	}

	if (head != tail) {
		if (non_cmd & INT_MASK_REQ_ACK_F)
			atomic_inc(&dpld->fmt_txq_req_ack_rcvd);

		ret = dpram_read(dpld, device, FMT_IDX);
		if (ret < 0)
			mif_err("%s, dpram_read failed\n", __func__);

		if (atomic_read(&dpld->fmt_txq_req_ack_rcvd) > 0) {
			dpram_write_command(dpld,
				INT_NON_CMD(INT_MASK_RES_ACK_F));
			atomic_set(&dpld->fmt_txq_req_ack_rcvd, 0);
		}
	} else {
		if (non_cmd & INT_MASK_REQ_ACK_F) {
			dpram_write_command(dpld,
				INT_NON_CMD(INT_MASK_RES_ACK_F));
			atomic_set(&dpld->fmt_txq_req_ack_rcvd, 0);
		}
	}

	/* Check raw data region */
	device = &dpld->dev_map[RAW_IDX];
	head = dpram_readh(&device->in->head);
	tail = dpram_readh(&device->in->tail);

	if (!dpram_circ_valid(device->in_buff_size, head, tail)) {
		mif_err("MIF: %s: invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		return;
	}

	if (head != tail) {
		if (non_cmd & INT_MASK_REQ_ACK_R)
			atomic_inc(&dpld->raw_txq_req_ack_rcvd);

		ret = dpram_read(dpld, device, RAW_IDX);
		if (ret < 0)
			mif_err("%s, dpram_read failed\n", __func__);

		if (atomic_read(&dpld->raw_txq_req_ack_rcvd) > 0) {
			dpram_write_command(dpld,
				INT_NON_CMD(INT_MASK_RES_ACK_R));
			atomic_set(&dpld->raw_txq_req_ack_rcvd, 0);
		}
	} else {
		if (non_cmd & INT_MASK_REQ_ACK_R) {
			dpram_write_command(dpld,
				INT_NON_CMD(INT_MASK_RES_ACK_R));
			atomic_set(&dpld->raw_txq_req_ack_rcvd, 0);
		}
	}
}

static void gota_cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	if (cmd & GOTA_RESULT_FAIL) {
		mif_err("[GOTA] Command failed: %04x\n", cmd);
		return;
	}

	switch (GOTA_CMD_MASK(cmd)) {
	case GOTA_CMD_RECEIVE_READY:
		mif_debug("[GOTA] Send CP-->AP RECEIVE_READY\n");
		dpram_write_command(dpld, CMD_DL_START_REQ);
		break;

	case GOTA_CMD_DOWNLOAD_START_RESP:
		mif_debug("[GOTA] Send CP-->AP DOWNLOAD_START_RESP\n");
		complete_all(&dpld->gota_download_start_complete);
		break;

	case GOTA_CMD_SEND_DONE_RESP:
		mif_debug("[GOTA] Send CP-->AP SEND_DONE_RESP\n");
		complete_all(&dpld->gota_send_done);
		break;

	case GOTA_CMD_UPDATE_DONE:
		mif_debug("[GOTA] Send CP-->AP UPDATE_DONE\n");
		complete_all(&dpld->gota_update_done);
		break;

	case GOTA_CMD_IMAGE_SEND_RESP:
		mif_debug("MIF: Send CP-->AP IMAGE_SEND_RESP\n");
		complete_all(&dpld->dump_receive_done);
		break;

	default:
		mif_err("[GOTA] Unknown command.. %x\n", cmd);
	}
}

static irqreturn_t dpram_irq_handler(int irq, void *p_ld)
{
	u16 cp2ap;

	struct link_device *ld = (struct link_device *)p_ld;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	cp2ap = dpram_readh(&dpld->dpram->mbx_cp2ap);
#if 0
	mif_err("MIF: received CP2AP = 0x%x\n", cp2ap);
#endif
	if (cp2ap == INT_POWERSAFE_FAIL) {
		mif_err("MIF: Received POWERSAFE_FAIL\n");
		goto exit_irq;
	}

	if (GOTA_CMD_VALID(cp2ap))
		gota_cmd_handler(dpld, cp2ap);
	else if (INT_CMD_VALID(cp2ap))
		command_handler(dpld, cp2ap);
	else if (INT_VALID(cp2ap))
		non_command_handler(dpld, cp2ap);
	else
		mif_err("MIF: Invalid command %04x\n", cp2ap);

exit_irq:
#ifdef CONFIG_INTERNAL_MODEM_IF
	dpld->clear_interrupt();
#endif
	return IRQ_HANDLED;
}

static int dpram_attach_io_dev(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	iod->link = ld;
	/* list up io devices */
	list_add(&iod->list, &dpld->list_of_io_devices);

	return 0;
}

static int dpram_write(struct dpram_link_device *dpld,
			struct dpram_device *device,
			const unsigned char *buf,
			int len)
{
	u16 head;
	u16 tail;
	u16 irq_mask;
	int free_space;
	int last_size;

#ifdef CONFIG_INTERNAL_MODEM_IF
	/* Internal DPRAM, check dpram ready?*/
	if (idpram_get_write_lock(dpld->link_pm_data)) {
		printk(KERN_INFO "dpram_write_net - not ready return -EAGAIN\n");
		return -EAGAIN;
	}
#endif

	head = dpram_readh(&device->out->head);
	tail = dpram_readh(&device->out->tail);

	if (!dpram_circ_valid(device->out_buff_size, head, tail)) {
		mif_err("MIF: %s: invalid circular buffer\n", __func__);
		dpram_zero_circ(device->out);
		return -EINVAL;
	}

	free_space = (head < tail) ? tail - head - 1 :
			device->out_buff_size + tail - head - 1;
	if (len > free_space) {
		mif_debug("WRITE: No space in Q\n"
			 "len[%d] free_space[%d] head[%u] tail[%u] out_buff_size =%d\n",
			 len, free_space, head, tail, device->out_buff_size);
		return -EINVAL;
	}

	mif_debug("WRITE: len[%d] free_space[%d] head[%u] tail[%u] out_buff_size =%d\n",
			len, free_space, head, tail, device->out_buff_size);

	if (head < tail) {
		/* +++++++++ head ---------- tail ++++++++++ */
		memcpy((device->out_buff_addr + head), buf, len);
	} else {
		/* ------ tail +++++++++++ head ------------ */
		last_size = device->out_buff_size - head;
		memcpy((device->out_buff_addr + head), buf,
			len > last_size ? last_size : len);
		if (len > last_size) {
			memcpy(device->out_buff_addr, (buf + last_size),
				(len - last_size));
		}
	}

	/* Update new head */
	head = (u16)((head + len) % device->out_buff_size);
	dpram_writeh(head, &device->out->head);

	irq_mask = INT_MASK_VALID;

	if (len > 0)
		irq_mask |= device->mask_send;

	dpram_write_command(dpld, irq_mask);

	return len;
}

static void dpram_write_work(struct work_struct *work)
{
	struct link_device *ld =
		container_of(work, struct link_device, tx_delayed_work.work);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_device *device;
	struct sk_buff *skb;
	bool reschedule = false;
	int ret;

	device = &dpld->dev_map[FMT_IDX];
	while ((skb = skb_dequeue(&ld->sk_fmt_tx_q))) {
		ret = dpram_write(dpld, device, skb->data, skb->len);
		if (ret < 0) {
			skb_queue_head(&ld->sk_fmt_tx_q, skb);
			reschedule = true;
			break;
		}
		dev_kfree_skb_any(skb);
	}

	device = &dpld->dev_map[RAW_IDX];
	while ((skb = skb_dequeue(&ld->sk_raw_tx_q))) {
		ret = dpram_write(dpld, device, skb->data, skb->len);
		if (ret < 0) {
			skb_queue_head(&ld->sk_raw_tx_q, skb);
			reschedule = true;
			break;
		}
		dev_kfree_skb_any(skb);
	}

	if (reschedule)
		schedule_delayed_work(&ld->tx_delayed_work,
					msecs_to_jiffies(10));
}

static int dpram_send(struct link_device *ld, struct io_device *iod,
		      struct sk_buff *skb)
{
	int len = skb->len;
	mif_debug("%s: iod->format = %d\n", __func__, iod->format);

	switch (iod->format) {
	case IPC_FMT:
		skb_queue_tail(&ld->sk_fmt_tx_q, skb);
		break;

	case IPC_RAW:
		skb_queue_tail(&ld->sk_raw_tx_q, skb);
		break;

	case IPC_BOOT:
	case IPC_RFS:
	default:
		dev_kfree_skb_any(skb);
		return 0;
	}

	schedule_delayed_work(&ld->tx_delayed_work, 0);
	return len;
}

static void dpram_table_init(struct dpram_link_device *dpld)
{
	struct dpram_device *dev;
	struct dpram_map __iomem *dpram = dpld->dpram;

	dev                 = &dpld->dev_map[FMT_IDX];
	dev->in             = &dpram->fmt_in;
	dev->in_buff_addr   = dpram->fmt_in_buff;
	dev->in_buff_size   = DP_FMT_IN_BUFF_SIZE;
	dev->out            = &dpram->fmt_out;
	dev->out_buff_addr  = dpram->fmt_out_buff;
	dev->out_buff_size  = DP_FMT_OUT_BUFF_SIZE;
	dev->mask_req_ack   = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack   = INT_MASK_RES_ACK_F;
	dev->mask_send      = INT_MASK_SEND_F;

	dev                 = &dpld->dev_map[RAW_IDX];
	dev->in             = &dpram->raw_in;
	dev->in_buff_addr   = dpram->raw_in_buff;
	dev->in_buff_size   = DP_RAW_IN_BUFF_SIZE;
	dev->out            = &dpram->raw_out;
	dev->out_buff_addr  = dpram->raw_out_buff;
	dev->out_buff_size  = DP_RAW_OUT_BUFF_SIZE;
	dev->mask_req_ack   = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack   = INT_MASK_RES_ACK_R;
	dev->mask_send      = INT_MASK_SEND_R;
}

static int dpram_set_dlmagic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	dpram_writew(DP_MAGIC_DMDL, &dpld->dpram->magic);
	return 0;
}

static int dpram_set_ulmagic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_map *dpram = (void *)dpld->dpram;
	u8 *dest;
	dest = (u8 *)(&dpram->fmt_out);

	dpram_writew(DP_MAGIC_UMDL, &dpld->dpram->magic);

	dpram_writeb((u8)START_INDEX, dest + 0);
	dpram_writeb((u8)0x1, dest + 1);
	dpram_writeb((u8)0x1, dest + 2);
	dpram_writeb((u8)0x0, dest + 3);
	dpram_writeb((u8)END_INDEX, dest + 4);

	init_completion(&dpld->gota_download_start_complete);
	dpram_write_command(dpld, CMD_DL_START_REQ);

	return 0;
}

static int
dpram_download(struct dpram_link_device *dpld, const char *buf, int len)
{
	struct dpram_map *dpram = (void *)dpld->dpram;
	struct dpram_ota_header header;
	u16 nframes;
	u16 curframe = 1;
	u16 plen;
	u8 *dest;
	int ret;

	nframes = DIV_ROUND_UP(len, DP_DEFAULT_WRITE_LEN);

	mif_debug("[GOTA] download len = %d\n", len);

	header.start_index = START_INDEX;
	header.nframes = nframes;

#ifdef CONFIG_INTERNAL_MODEM_IF
	dpld->board_ota_reset();
#endif
	while (len > 0) {
		plen = min(len, DP_DEFAULT_WRITE_LEN);
		dest = (u8 *)&dpram->fmt_out;

		mif_debug("[GOTA] Start write frame %d/%d\n", \
							curframe, nframes);

		header.curframe = curframe;
		header.len = plen;

		memcpy(dest, &header, sizeof(header));
		dest += sizeof(header);

		memcpy(dest, buf, plen);
		dest += plen;
		buf += plen;
		len -= plen;

		dpram_writeb(END_INDEX, dest+3);

		init_completion(&dpld->gota_send_done);

		if (curframe == 1) {
#ifdef CONFIG_INTERNAL_MODEM_IF
			init_completion(&dpld->gota_download_start_complete);
			dpram_write_command(dpld, 0);
#endif
			ret = wait_for_completion_interruptible_timeout(
				&dpld->gota_download_start_complete,
				GOTA_TIMEOUT);
			if (!ret) {
				mif_err("[GOTA] CP didn't send DOWNLOAD_START\n");
				return -ENXIO;
			}
		}

		dpram_write_command(dpld, CMD_IMG_SEND_REQ);
		ret = wait_for_completion_interruptible_timeout(
				&dpld->gota_send_done, GOTA_SEND_TIMEOUT);
		if (!ret) {
			mif_err("[GOTA] CP didn't send SEND_DONE_RESP\n");
			return -ENXIO;
		}

		curframe++;
	}

	dpram_write_command(dpld, CMD_DL_SEND_DONE_REQ);
	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_update_done, GOTA_TIMEOUT);
	if (!ret) {
		mif_err("[GOTA] CP didn't send UPDATE_DONE_NOTIFICATION\n");
		return -ENXIO;
	}

	return 0;
}

static int
dpram_upload(struct dpram_link_device *dpld, struct dpram_firmware *uploaddata)
{
	struct dpram_map *dpram = (void *)dpld->dpram;
	struct ul_header header;
	u8 *dest;
	u8 *buff = vmalloc(DP_DEFAULT_DUMP_LEN);
	u16 plen = 0;
	u32 tlen = 0;
	int ret;
	int region = 0;

	mif_debug("MIF: dpram_upload()\n");

	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_download_start_complete,
			DUMP_START_TIMEOUT);
	if (!ret) {
		mif_err("[GOTA] CP didn't send DOWNLOAD_START\n");
		goto err_out;
	}

	wake_lock(&dpld->dpram_wake_lock);

	memset(buff, 0, DP_DEFAULT_DUMP_LEN);

	dpram_write_command(dpld, CMD_IMG_SEND_REQ);
	mif_debug("MIF: write CMD_IMG_SEND_REQ(0x9400)\n");

	while (1) {
		init_completion(&dpld->dump_receive_done);
		ret = wait_for_completion_interruptible_timeout(
				&dpld->dump_receive_done, DUMP_TIMEOUT);
		if (!ret) {
			mif_err("MIF: CP didn't send DATA_SEND_DONE_RESP\n");
			goto err_out;
		}

		dest = (u8 *)(&dpram->fmt_out);

#ifdef CONFIG_INTERNAL_MODEM_IF
		header.bop = *(u16 *)(dest);
		header.total_frame = *(u16 *)(dest + 2);
		header.curr_frame = *(u16 *)(dest + 4);
		header.len = *(u16 *)(dest + 6);
#else
		header.bop = *(u8 *)(dest);
		header.total_frame = *(u16 *)(dest + 1);
		header.curr_frame = *(u16 *)(dest + 3);
		header.len = *(u16 *)(dest + 5);
#endif

		mif_err("total frame:%d, current frame:%d, data len:%d\n",
			header.total_frame, header.curr_frame,
			header.len);

		dest += DP_DUMP_HEADER_SIZE;
		plen = min(header.len, (u16)DP_DEFAULT_DUMP_LEN);

		memcpy(buff, dest, plen);
		dest += plen;

		ret = copy_to_user(uploaddata->firmware + tlen,	buff,  plen);
		if (ret < 0) {
			mif_err("MIF: Copy to user failed\n");
			goto err_out;
		}

		tlen += plen;

		if (header.total_frame == header.curr_frame) {
			if (region) {
				uploaddata->is_delta = tlen - uploaddata->size;
				dpram_write_command(dpld, CMD_UL_RECEIVE_RESP);
				break;
			} else {
				uploaddata->size = tlen;
				region = 1;
			}
		}
		dpram_write_command(dpld, CMD_UL_RECEIVE_RESP);
	}

	mif_debug("1st dump region data size=%d\n", uploaddata->size);
	mif_debug("2st dump region data size=%d\n", uploaddata->is_delta);

	init_completion(&dpld->gota_send_done);
	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_send_done, DUMP_TIMEOUT);
	if (!ret) {
		mif_err("[GOTA] CP didn't send SEND_DONE_RESP\n");
		goto err_out;
	}

	dpram_write_command(dpld, CMD_UL_RECEIVE_DONE_RESP);
	mif_debug("MIF: write CMD_UL_RECEIVE_DONE_RESP(0x9801)\n");

	dpram_writew(0, &dpld->dpram->magic); /*clear magic code */

	wake_unlock(&dpld->dpram_wake_lock);

	vfree(buff);
	return 0;

err_out:
	vfree(buff);
	dpram_writew(0, &dpld->dpram->magic);
	mif_err("CDMA dump error out\n");
	wake_unlock(&dpld->dpram_wake_lock);
	return -EIO;
}

struct link_device *dpram_create_link_device(struct platform_device *pdev)
{
	int ret;
	struct dpram_link_device *dpld;
	struct link_device *ld;
	struct resource *res;
	unsigned long flag = 0;
#ifdef CONFIG_INTERNAL_MODEM_IF
	struct modem_data *pdata = (struct modem_data *)pdev->dev.platform_data;
#endif
	BUILD_BUG_ON(sizeof(struct dpram_map) != DP_DPRAM_SIZE);

	dpld = kzalloc(sizeof(struct dpram_link_device), GFP_KERNEL);
	if (!dpld)
		return NULL;
		ld = &dpld->ld;

	INIT_LIST_HEAD(&dpld->list_of_io_devices);
	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	INIT_DELAYED_WORK(&ld->tx_delayed_work, dpram_write_work);

	wake_lock_init(&dpld->dpram_wake_lock, WAKE_LOCK_SUSPEND, "DPRAM");

	init_completion(&dpld->modem_pif_init_done);
	init_completion(&dpld->dpram_init_cmd);
	init_completion(&dpld->gota_send_done);
	init_completion(&dpld->gota_update_done);
	init_completion(&dpld->gota_download_start_complete);
	init_completion(&dpld->dump_receive_done);

	ld->name = "dpram";
	ld->attach = dpram_attach_io_dev;
	ld->send = dpram_send;
	ld->gota_start = dpram_set_dlmagic;
	ld->modem_update = dpram_modem_update;
	ld->dump_start = dpram_set_ulmagic;
	ld->dump_update = dpram_dump_update;

#ifdef CONFIG_INTERNAL_MODEM_IF
	dpld->clear_interrupt = pdata->clear_intr;
	dpld->board_ota_reset = pdata->ota_reset;
#endif
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		mif_err("MIF:  Failed to get mem region\n");
		goto err;
	}
	dpld->dpram = ioremap(res->start, resource_size(res));
	printk(KERN_INFO " MIF:  start address:0x%08x\n", \
	res->start);
	dpld->irq = platform_get_irq_byname(pdev, "dpram_irq");
	if (!dpld->irq) {
		mif_err("MIF:  %s: Failed to get IRQ\n", __func__);
		goto err;
	}

	dpram_table_init(dpld);

	atomic_set(&dpld->raw_txq_req_ack_rcvd, 0);
	atomic_set(&dpld->fmt_txq_req_ack_rcvd, 0);

	dpram_writeh(0, &dpld->dpram->magic);
#ifdef CONFIG_INTERNAL_MODEM_IF
	ret = idpram_link_pm_init(dpld, pdev);

	if (ret)
		mif_err("MIF: idpram_link_pm_init fail.(%d)\n", ret);
#endif
	flag = IRQF_DISABLED;
	printk(KERN_ERR "dpram irq: %d\n", dpld->irq);
	dpld->clear_interrupt();
	ret = request_irq(dpld->irq, dpram_irq_handler, flag,
							"dpram irq", ld);
	if (ret) {
		mif_err("MIF: DPRAM interrupt handler failed\n");
		goto err;
	}

	return ld;

err:
	iounmap(dpld->dpram);
	kfree(dpld);
	return NULL;
}
