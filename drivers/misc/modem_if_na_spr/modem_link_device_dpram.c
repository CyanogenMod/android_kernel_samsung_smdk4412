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
#include <linux/platform_data/modem_na_spr.h>
#include <linux/io.h>
#include "modem_prj.h"
#include "modem_link_device_dpram.h"
#include <linux/ratelimit.h>

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

#define DP_DUMP_HEADER_SIZE		8

#define GOTA_TIMEOUT			(50 * HZ)
#define GOTA_SEND_TIMEOUT		(200 * HZ)
#define DUMP_TIMEOUT			(30 * HZ)
#define DUMP_START_TIMEOUT		(100 * HZ)
#define PDA_SLEEP_CMD_TIMEOUT		(50 * HZ)

#define IDPRAM_PHY_START    0x13A00000
#define IDPRAM_SIZE 0x4000

/* collecting modem ramdump when it crashes */
/* mbx commands */
#define CMD_CP_RAMDUMP_START_REQ        0x9200
#define CMD_CP_RAMDUMP_SEND_REQ         0x9400
#define CMD_CP_RAMDUMP_SEND_DONE_REQ    0x9600

#define CMD_CP_RAMDUMP_START_RESP       0x0300
#define CMD_CP_RAMDUMP_SEND_RESP        0x0500
#define CMD_CP_RAMDUMP_SEND_DONE_RESP   0x0700

/* magic codes */
#define QSC_UPLOAD_MODE                 (0x444D554C)
#define QSC_UPLOAD_MODE_COMPLETE        (0xABCDEF90)

#define RAMDUMP_CMD_TIMEOUT     (5*HZ)
#define LOCAL_RAMDUMP_BUFF_SIZE (IDPRAM_SIZE - 4 - 12 - 4) /* (16KB -20B) */
#define MODEM_RAM_SIZE          (32 * 1024 * 1024) /* 32MB */

#define FMT_WAKE_TIME   (HZ/2)
#define RFS_WAKE_TIME   (HZ*3)
#define RAW_WAKE_TIME   (HZ*6)

struct ramdump_cmd_hdr {
	u32 addr;
	u32 size;
	u32 copyto_offset;
};

static int dpram_start_ramdump(struct link_device *ld, struct io_device *iod);
static int dpram_read_ramdump(struct link_device *ld, struct io_device *iod);
static int dpram_stop_ramdump(struct link_device *ld, struct io_device *iod);
/* collecting modem ramdump when it crashes */

static int
dpram_download(struct dpram_link_device *dpld, const char *buf, int len);
static int
dpram_upload(struct dpram_link_device *dpld,
	struct dpram_firmware *uploaddata);
static inline void
dpram_writeh(u16 value,  void __iomem *p_dest);
static inline void
dpram_writeh_enable(u16 value,  void __iomem *p_dest);
static void
dpram_clear(struct dpram_link_device *dpld);
static struct io_device *
dpram_find_iod(struct dpram_link_device *dpld, int id);
static struct io_device *
dpram_find_iod_by_format(struct dpram_link_device *dpld, int format);
static void
dpram_write_command(struct dpram_link_device *dpld, u16 cmd);
static inline int
dpram_readh(void __iomem *p_dest);

#define INT_MASK_CMD_PDA_SLEEP           0x000D
#define INT_MASK_CMD_DPRAM_DOWN          0x000B
#define INT_MASK_CMD_PDA_WAKEUP          0x000C
#define INT_MASK_CMD_CP_WAKEUP_START     0x000E

#include <plat/gpio-cfg.h>
#include <linux/suspend.h>

#define DPRAM_RESUME_CHECK_RETRY_CNT 5
struct idpram_link_pm_data *pm;

/* to write ramdump into a file */
#include <linux/fs.h>

#undef ENABLE_FORCED_CP_CRASH
#undef CP_CRASHES_ON_PDA_SLEEP_CMD

static struct file *fp;
struct file *mif_open_file(const char *path)
{
	struct file *fp;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	fp = filp_open(path, O_RDWR|O_CREAT|O_APPEND, 0666);

	set_fs(old_fs);

	if (IS_ERR(fp))
		return NULL;

	return fp;
}

void mif_save_file(struct file *fp, const char *buff, size_t size)
{
	int ret;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	ret = fp->f_op->write(fp, buff, size, &fp->f_pos);

	if (ret < 0)
		mif_err("ERR! write fail\n");

	set_fs(old_fs);
}

void mif_close_file(struct file *fp)
{
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	filp_close(fp, NULL);

	set_fs(old_fs);
}
/* to write ramdump into a file */

void dpram_write_magic_code(struct dpram_link_device *dpld,
	u32 magic_code)
{
	u16 acc_code = 0x01;

	pr_info("MIF: <%s>", __func__);

	dpram_writeh(magic_code, &dpld->dpram->magic);
	dpram_writeh_enable(acc_code, &dpld->dpram->enable);
}

static void kernel_sec_dump_cp_handle(void)
{
	pr_info("MIF: %s\n", __func__);

	panic("CP Crashed");
}

void dpram_force_cp_crash(struct link_device *ld)
{
	int ret;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	pr_info("MIF: <%s+>", __func__);

	/*
	1. set the magic number to 0x554C
	2. send mbx cmd 0x000F
	3. wait for completon
	4. CP responds with cmd = 0xCF
	5. call panic("CP Crashed");
	*/
	dpram_write_magic_code(dpld, 0x554C);

	dpram_write_command(dpld, INT_CMD(0x000F));

	init_completion(&dpld->cp_crash_done);
	ret = wait_for_completion_interruptible_timeout(
		&dpld->cp_crash_done, 20 * HZ);
	if (!ret) {
		pr_err("MIF: CP didn't ack MBX_CMD_PHONE_RESET\n");
		dpram_write_magic_code(dpld, DP_MAGIC_CODE);
	}

	pr_info("MIF: <%s->", __func__);
}

void idpram_magickey_init(struct idpram_link_pm_data *pm_data)
{
	u16 acc_code = 0x01;

	pr_info("MIF: <%s>\n", __func__);

	dpram_writeh(DP_MAGIC_CODE, &pm_data->dpld->dpram->magic);
	dpram_writeh_enable(acc_code, &pm_data->dpld->dpram->enable);
}

int idpram_get_write_lock(struct idpram_link_pm_data *pm_data)
{
	return atomic_read(&pm_data->write_lock);
}

static int idpram_write_lock(struct idpram_link_pm_data *pm_data, int lock)
{
	int lock_value = 0;

	pr_info("MIF: <%s> lock = %d\n", __func__, lock);

	switch (lock) {
	case 0:		/* unlock */
		if (atomic_read(&pm_data->write_lock))
			lock_value = atomic_dec_return(&pm_data->write_lock);
		if (lock_value)
			pr_err("MIF: ipdram write unlock but lock value=%d\n",
			lock_value);
		break;
	case 1:		/* lock */
		if (!atomic_read(&pm_data->write_lock))
			lock_value = atomic_inc_return(&pm_data->write_lock);
		if (lock_value != 1)
			pr_err("MIF: ipdram write lock but lock value=%d\n",
				lock_value);
		break;
	}
	return 0;
}

static int idpram_resume_init(struct idpram_link_pm_data *pm_data)
{
	pr_info("MIF: <%s>\n", __func__);

	pm_data->pm_states = IDPRAM_PM_RESUME_START;
	pm_data->last_pm_mailbox = 0;

	dpram_clear(pm_data->dpld);
	idpram_magickey_init(pm_data);

	/* Initialize the dpram controller */
	pm_data->mdata->sfr_init();

	/* re-initialize internal dpram gpios */
	s3c_gpio_cfgpin(pm_data->mdata->gpio_mbx_intr, S3C_GPIO_SFN(0x2));

	/* write_lock will be released when dpram resume noti comes */
	/*
	idpram_write_lock(pm_data, 0);
	*/

	return 0;
}

void idpram_timeout_handler(struct idpram_link_pm_data *pm_data)
{
	struct io_device *iod = dpram_find_iod(pm_data->dpld, FMT_IDX);

	pr_info("MIF: <%s>\n", __func__);

	if (!gpio_get_value(pm_data->mdata->gpio_phone_active)) {
		pr_err("MIF: <%s:%s> (Crash silent Reset)\n",
			__func__, pm_data->dpld->ld.name);

		if (iod && iod->modem_state_changed)
			iod->modem_state_changed(iod, STATE_CRASH_EXIT);
	}
}

static int idpram_resume_check(struct idpram_link_pm_data *pm_data)
{
	/* check last pm mailbox */
	pr_info("MIF: <%s> last_pm_mailbox = 0x%X\n", __func__,
		pm_data->last_pm_mailbox);

	if (pm_data->last_pm_mailbox == INT_CMD(INT_MASK_CMD_PDA_WAKEUP)) {
		pm_data->last_pm_mailbox = 0;
		return 0;
	}

	dpram_write_command(pm_data->dpld, INT_CMD(INT_MASK_CMD_PDA_WAKEUP));
	pr_info("MIF: idpram sent PDA_WAKEUP cmd = 0x%X\n",
		INT_CMD(INT_MASK_CMD_PDA_WAKEUP));

	return -1;
}

static void idpram_resume_retry(struct work_struct *work)
{
	struct idpram_link_pm_data *pm_data =
		container_of(work, struct idpram_link_pm_data, \
		resume_work.work);

	pr_info("MIF: <%s+>\n", __func__);

	if (!idpram_resume_check(pm_data)) {
		pr_info("MIF: idpram resumed\n");
		idpram_write_lock(pm_data, 0);
		wake_unlock(&pm_data->hold_wlock);
		schedule_delayed_work(&pm_data->dpld->ld.tx_delayed_work,
					msecs_to_jiffies(10));
		pr_info("MIF: <%s->\n", __func__);
		return;
	}
	if (pm_data->resume_retry_cnt--) {
		pr_info("MIF: idpram not resumed yet\n");
		schedule_delayed_work(&pm_data->resume_work, \
			msecs_to_jiffies(200));
	} else {
		pr_info("MIF: idpram resume T-I-M-E-O-UT\n");
		idpram_timeout_handler(pm_data);
		wake_unlock(&pm_data->hold_wlock);
		/* hold wakelock until uevnet sent to rild */
		wake_lock_timeout(&pm_data->hold_wlock, HZ*7);
		idpram_write_lock(pm_data, 0);

		kernel_sec_dump_cp_handle();
	}

	pr_info("MIF: <%s->\n", __func__);
}

static irqreturn_t cp_dump_irq_handler(int irq, void *data)
{
	/*
	struct idpram_link_pm_data *pm_data = data;
	int val = gpio_get_value(pm_data->mdata->gpio_cp_dump_int);

	pr_info(KERN_DEBUG "MIF: <%s> val = %d\n", __func__, val);

	if (val)
		irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
	else
		irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	*/

	return IRQ_HANDLED;
}

static irqreturn_t ap_wakeup_irq_handler(int irq, void *data)
{
	struct idpram_link_pm_data *pm_data = data;

	pr_info("MIF: <%s> hold lock for 5 secs\n", __func__);

	wake_lock_timeout(&pm_data->host_wakeup_wlock, 5*HZ);

	return IRQ_HANDLED;
}

/*
static int idpram_pm_suspend(struct device *dev)
{
	struct idpram_link_pm_data *pm_data = pm;

	pr_info("MIF: <%s>\n", __func__);

	pm_data->pm_states = IDPRAM_PM_SUSPEND_START;
	gpio_set_value(pm_data->mdata->gpio_pda_active, 0);

	return 0;
}

static int idpram_pm_resume(struct device *dev)
{
	struct idpram_link_pm_data *pm_data = pm;

	pr_info("MIF: <%s>\n", __func__);

	idpram_resume_init(pm_data);
	gpio_set_value(pm_data->mdata->gpio_pda_active, 1);

	return 0;
}

static int __devinit idpram_pm_probe(struct platform_device *pdev)
{
	pr_info("MIF: <%s>\n", __func__);

	return 0;
}

static void idpram_pm_shutdown(struct platform_device *pdev)
{
	pr_info("MIF: <%s>\n", __func__);
}

static const struct dev_pm_ops idpram_pm_ops = {
	.suspend    = idpram_pm_suspend,
	.resume     = idpram_pm_resume,
};

static struct platform_driver idpram_pm_driver = {
	.probe = idpram_pm_probe,
	.shutdown = idpram_pm_shutdown,
	.driver = {
		.name = "idpram_pm",
		.pm   = &idpram_pm_ops,
	},
};
*/

static void idpram_powerup_start(struct idpram_link_pm_data *pm_data)
{
	pr_info("MIF: <%s>\n", __func__);

	pm_data->last_pm_mailbox = INT_CMD(INT_MASK_CMD_PDA_WAKEUP);
	pm_data->pm_states = IDPRAM_PM_ACTIVE;
}

static void idpram_power_down(struct idpram_link_pm_data *pm_data)
{
	pr_info("MIF: <%s>\n", __func__);

	pm_data->last_pm_mailbox = INT_CMD(INT_MASK_CMD_DPRAM_DOWN);
	complete(&pm_data->idpram_down);
}

static int idpram_post_resume(struct idpram_link_pm_data *pm_data)
{
	int gpio_val = 0;

	pr_info("MIF: <%s+> pm_states = %d\n", __func__, pm_data->pm_states);

	switch (pm_data->pm_states) {
	/* schedule_work */
	case IDPRAM_PM_DPRAM_POWER_DOWN:
		gpio_set_value(pm_data->mdata->gpio_pda_active, 0);

		msleep(50);

		idpram_resume_init(pm_data);

		msleep(50);

		gpio_set_value(pm_data->mdata->gpio_pda_active, 1);

		msleep(20);

		gpio_val = gpio_get_value(pm_data->mdata->gpio_pda_active);

		if (gpio_val == 0) {
			pr_err("MIF: <%s> PDA_ACTIVE is still low. "
				"setting it again.\n", __func__);
			gpio_set_value(pm_data->mdata->gpio_pda_active, 1);
		}

		pm_data->resume_retry_cnt = DPRAM_RESUME_CHECK_RETRY_CNT;
		wake_lock(&pm_data->hold_wlock);
		schedule_delayed_work(&pm_data->resume_work, \
			msecs_to_jiffies(20));

		break;

	case IDPRAM_PM_RESUME_START:
		break;

	case IDPRAM_PM_SUSPEND_PREPARE:
		break;
	}

	pr_info("MIF: <%s->\n", __func__);

	return 0;
}

static int idpram_pre_suspend(struct idpram_link_pm_data *pm_data)
{
	int timeout_ret = 0;
	int suspend_retry = 3;
	u16 intr_out = INT_CMD(INT_MASK_CMD_PDA_SLEEP);
	struct io_device *iod = dpram_find_iod_by_format(pm_data->dpld,
		IPC_RAMDUMP);

	pr_info("MIF: <%s+>\n", __func__);

#if defined(ENABLE_FORCED_CP_CRASH)
	/*
	1. write magic number=0x554C
	2. send phone reset cmd=0x000F
	3. collect the modem ramdump
	4. modem reset
	5. modem on
	*/
	dpram_force_cp_crash(&pm_data->dpld->ld);

	fp = mif_open_file("/sdcard/ramdump1.data");
	if (!fp)
		pr_err("MIF: <%s> file pointer is NULL\n", __func__);

	dpram_start_ramdump(&pm_data->dpld->ld, iod);

	for (; iod->ramdump_size;)
		dpram_read_ramdump(&pm_data->dpld->ld, iod);

	dpram_stop_ramdump(&pm_data->dpld->ld, iod);

	mif_close_file(fp);

	iod->mc->ops.modem_reset(iod->mc);
	iod->mc->ops.modem_on(iod->mc);

	pr_info("MIF: <%s->\n", __func__);
	return 0;
#endif

	pm_data->pm_states = IDPRAM_PM_SUSPEND_PREPARE;
	pm_data->last_pm_mailbox = 0;
	idpram_write_lock(pm_data, 1);

	gpio_set_value(pm_data->mdata->gpio_mbx_intr, 1);

	/* prevent PDA_ACTIVE status is low */
	gpio_set_value(pm_data->mdata->gpio_pda_active, 1);

	do {
		init_completion(&pm_data->idpram_down);
		dpram_write_command(pm_data->dpld, intr_out);
		pr_info("MIF: sending cmd = 0x%X\n", intr_out);
		timeout_ret =
		wait_for_completion_timeout(&pm_data->idpram_down,
			PDA_SLEEP_CMD_TIMEOUT);

		if (!timeout_ret)
			pr_err("MIF: timeout!. retry cnt = %d\n",
				suspend_retry);
	} while (!timeout_ret && --suspend_retry);

	if (!timeout_ret && !suspend_retry) {
		pr_err("MIF: no response for PDA_SLEEP cmd\n");
		kernel_sec_dump_cp_handle();
	}

	pr_info("MIF: last responce from cp = 0x%X\n",
		pm_data->last_pm_mailbox);

	switch (pm_data->last_pm_mailbox) {
	case INT_CMD(INT_MASK_CMD_DPRAM_DOWN):
		pr_info("MIF: INT_MASK_CMD_DPRAM_DOWN\n");
		break;
	default:
		pr_err("MIF: idpram down or not ready!! intr = 0x%X\n",
			dpram_readh(&pm_data->dpld->dpram->mbx_cp2ap));
		wake_lock_timeout(&pm_data->hold_wlock,
			msecs_to_jiffies(500));
		idpram_write_lock(pm_data, 0);

		/*
		1. Flash a modem which crashes when AP sends
		   cmd=PDA_SLEEP
		2. collect the modem ramdump
		3. modem reset
		4. modem on
		*/
#if defined(CP_CRASHES_ON_PDA_SLEEP_CMD)
		fp = mif_open_file("/sdcard/ramdump2.data");
		if (!fp)
			pr_err("MIF: <%s> fp is NULL\n", __func__);

		dpram_start_ramdump(&pm_data->dpld->ld, iod);

		for (; iod->ramdump_size;)
			dpram_read_ramdump(&pm_data->dpld->ld, iod);

		dpram_stop_ramdump(&pm_data->dpld->ld, iod);

		mif_close_file(fp);

		iod->mc->ops.modem_reset(iod->mc);
		iod->mc->ops.modem_on(iod->mc);
#endif

		pr_info("MIF: <%s->\n", __func__);
		return 0;
	}

	/*
	* Because, if dpram was powered down, cp dpram random intr was
	* ocurred. so, fixed by muxing cp dpram intr pin to GPIO output
	* high,..
	*/
	gpio_set_value(pm_data->mdata->gpio_mbx_intr, 1);
	s3c_gpio_cfgpin(pm_data->mdata->gpio_mbx_intr,
		S3C_GPIO_OUTPUT);

	pm_data->pm_states = IDPRAM_PM_DPRAM_POWER_DOWN;

	pr_info("MIF: <%s->\n", __func__);
	return 0;
}

static int idpram_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	int err;

	pr_info("MIF: <%s+> event = %lu\n", __func__, event);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		err = idpram_pre_suspend(pm);
		if (err)
			pr_err("MIF: pre-suspend err\n");
		break;

	case PM_POST_SUSPEND:
		err = idpram_post_resume(pm);
		if (err)
			pr_err("MIF: pre-suspend err\n");
		break;
	}

	pr_info("MIF: <%s->\n", __func__);

	return NOTIFY_DONE;
}

static struct notifier_block idpram_link_pm_notifier = {
	.notifier_call = idpram_notifier_event,
};

static int idpram_init_magic_num(struct dpram_link_device *dpld)
{
	const u16 magic_code = 0x4D4E;
	u16 acc_code = 0;
	u16 ret_value = 0;

	pr_info("MIF: <%s>\n", __func__);

	/*write DPRAM disable code */
	dpram_writeh(acc_code, &dpld->dpram->enable);

	/* write DPRAM magic code :  Normal boot Magic - 0x4D4E*/
	dpram_writeh(magic_code, &dpld->dpram->magic);

	/*write enable code */
	acc_code = 0x0001;
	dpram_writeh_enable(acc_code, &dpld->dpram->enable);

	ret_value = dpram_readh(&dpld->dpram->magic);

	pr_info("MIF: <%s> Read_MagicNum : 0x%04X\n", __func__, ret_value);

	return 0;
}

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

	pr_info("MIF: <%s>\n", __func__);

	pm = kzalloc(sizeof(struct idpram_link_pm_data), GFP_KERNEL);
	if (!pm) {
		pr_err("MIF: <%s> link_pm_data is NULL\n", __func__);
		return -ENOMEM;
	}

	pm->mdata = pdata;
	pm->dpld = idpram_ld;
	pm->resume_retry_cnt = DPRAM_RESUME_CHECK_RETRY_CNT;
	idpram_ld->link_pm_data = pm;

	if (pdata->modem_type == QC_QSC6085)
		idpram_ld->init_magic_num = idpram_init_magic_num;

	/* for pm notifier */
	register_pm_notifier(&idpram_link_pm_notifier);

	init_completion(&pm->idpram_down);
	wake_lock_init(&pm->host_wakeup_wlock,
		WAKE_LOCK_SUSPEND, "dpram_host_wakeup");
	wake_lock_init(&pm->hold_wlock,
		WAKE_LOCK_SUSPEND, "dpram_power_down");
	/* Currently, these two wake locks and read_lock are unused.
	wake_lock_init(&pm->wakeup_wlock, WAKE_LOCK_SUSPEND, "dpram_wakeup");
	wake_lock_init(&pm->rd_wlock, WAKE_LOCK_SUSPEND, "dpram_pwrdn");
	atomic_set(&pm->read_lock, 0);
	*/
	atomic_set(&pm->write_lock, 0);
	INIT_DELAYED_WORK(&pm->resume_work, idpram_resume_retry);

	/*
	r = platform_driver_register(&idpram_pm_driver);
	if (r) {
		pr_err("MIF: platform_driver_register failed\n");
		goto err_platform_driver_register;
	}
	*/

	irq = gpio_to_irq(pdata->gpio_ap_wakeup);
	r = request_irq(irq, ap_wakeup_irq_handler,
		IRQF_TRIGGER_RISING, "idpram_host_wakeup", (void *)pm);

	pr_info("MIF: <%s> ap_wakeup IRQ = %d, %d\n", __func__,
		irq, pm->mdata->gpio_ap_wakeup);

	if (r) {
		pr_err("MIF: <%s> fail to request host_wake_irq(%d)\n",
			__func__, r);
		goto err_request_irq;
	}

	r = enable_irq_wake(irq);
	if (r) {
		pr_err("MIF: <%s> fail to enable_irq_wake host_wake_irq(%d)\n",
			__func__, r);
		goto err_set_wake_irq;
	}

	/* cp dump int interrupt for LPA mode */
	irq = gpio_to_irq(pdata->gpio_cp_dump_int);
	r = request_irq(irq, cp_dump_irq_handler, IRQF_TRIGGER_RISING,
		"CP_DUMP_INT_irq", (void *) pm);
	if (r) {
		pr_err("MIF: <%s> request_irq(cp_dump_irq) failed. ret = %d\n",
			__func__, r);
		goto err_request_irq;
	}

	r = enable_irq_wake(irq);
	if (r) {
		pr_err("MIF: <%s> enable_irq_wake(cp_dump_irq) failed"
			" ret =%d\n", __func__, r);
		goto err_set_wake_irq;
	}

	pr_info("MIF: <%s->\n", __func__);
	return 0;

err_set_wake_irq:
	free_irq(irq, (void *)pm);
err_request_irq:
	/*
	platform_driver_unregister(&idpram_pm_driver);
	*/
err_platform_driver_register:
	kfree(pm);
	return r;
}

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

static inline void dpram_writeh_enable(u16 value,  void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;

	if (0) {
		pr_info("MIF: <%s> setting debug level\n", __func__);
		value = 0x0101;
	}

	iowrite16(value, dest);
}

static inline void dpram_writeb(u8 value,  void __iomem *p_dest)
{
	unsigned long dest = (unsigned long)p_dest;
	iowrite8(value, dest);
}


static void dpram_write_command(struct dpram_link_device *dpld, u16 cmd)
{
	int retry_cnt = 5;

	/* if modem is yet to receive the previous cmd */
	while (!gpio_get_value(dpld->link_pm_data->mdata->gpio_mbx_intr) && \
		retry_cnt--) {
		udelay(1000);
		pr_debug("MIF: <%s> GPIO_DPRAM_INT_CP_N is low. retrying\n",
			__func__);
	}

	if (retry_cnt == -1)
		pr_err("MIF: <%s> GPIO_DPRAM_INT_CP_N is still low "
			"but ignoring\n", __func__);


	/* If PDA is in transition to LPA */
	retry_cnt = 5;
	while (!gpio_get_value(dpld->link_pm_data->mdata->gpio_pda_active) && \
		retry_cnt--) {
		udelay(1000);
		pr_debug("MIF: <%s> GPIO_PDA_ACTIVE is low. retrying\n",
			__func__);
	}

	if (retry_cnt == -1)
		pr_err("MIF: <%s> GPIO_PDA_ACTIVE is still low "
			"but ignoring\n", __func__);

	pr_debug("MIF: <%s> cmd = 0x%X\n", __func__, cmd);
	dpram_writeh(cmd, &dpld->dpram->mbx_ap2cp);
}

static void dpram_clear_interrupt(struct dpram_link_device *dpld)
{
	pr_info("MIF: <%s>\n", __func__);
	dpram_writeh(0, &dpld->dpram->mbx_cp2ap);
}

static void dpram_drop_data(struct dpram_device *device, u16 head)
{
	pr_info("MIF: <%s>\n", __func__);
	dpram_writeh(head, &device->in->tail);
}

static void dpram_zero_circ(struct dpram_circ *circ)
{
	dpram_writeh(0, &circ->head);
	dpram_writeh(0, &circ->tail);
}

static void dpram_clear(struct dpram_link_device *dpld)
{
	pr_info("MIF: <%s>\n", __func__);
	dpram_zero_circ(&dpld->dpram->fmt_out);
	dpram_zero_circ(&dpld->dpram->raw_out);
	dpram_zero_circ(&dpld->dpram->fmt_in);
	dpram_zero_circ(&dpld->dpram->raw_in);
}

static bool dpram_circ_valid(int size, u16 head, u16 tail)
{
	if (head >= size) {
		pr_err("MIF: head(%d) >= size(%d)\n", head, size);
		return false;
	}
	if (tail >= size) {
		pr_err("MIF: tail(%d) >= size(%d)\n", tail, size);
		return false;
	}
	return true;
}

static int dpram_init_and_report(struct dpram_link_device *dpld)
{
	const u16 init_end = INT_CMD(INT_CMD_INIT_END);
	u16 magic;
	u16 enable;

	pr_info("MIF: <%s>\n", __func__);

	dpram_writeh(0, &dpld->dpram->enable);
	dpram_clear(dpld);
	dpram_writeh(DP_MAGIC_CODE, &dpld->dpram->magic);
	dpram_writeh_enable(1, &dpld->dpram->enable);

	/* Send init end code to modem */
	dpram_write_command(dpld, init_end);

	magic = dpram_readh(&dpld->dpram->magic);
	if (magic != DP_MAGIC_CODE) {
		pr_err("MIF:: <%s> Failed to check magic\n", __func__);
		return -1;
	}

	enable = dpram_readh(&dpld->dpram->enable);
	pr_info("MIF: <%s> enable = 0x%X\n", __func__, enable);
	if (!enable) {
		pr_err("MIF: <%s> DPRAM enable failed\n", __func__);
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

static struct io_device *
dpram_find_iod_by_format(struct dpram_link_device *dpld, int format)
{
	struct io_device *iod;

	list_for_each_entry(iod, &dpld->list_of_io_devices, list)
		if (format == iod->format)
			return iod;

	return NULL;
}

static void cmd_req_active_handler(struct dpram_link_device *dpld)
{
	pr_info("MIF: %s pda_active = %d\n", __func__,
		gpio_get_value(dpld->link_pm_data->mdata->gpio_pda_active));

	dpram_write_command(dpld, INT_CMD(INT_CMD_RES_ACTIVE));
}

static void cmd_error_display_handler(struct dpram_link_device *dpld)
{
	struct io_device *iod = dpram_find_iod(dpld, FMT_IDX);

	pr_info("MIF: <%s>\n", __func__);
	pr_info("MIF: Received 0xC9 from modem (CP Crash)\n");
	pr_info("MIF: <%s>\n", dpld->dpram->fmt_in_buff);

	if (iod && iod->modem_state_changed)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	/* enabling upload mode for CP crash */
	/*kernel_sec_dump_cp_handle();*/
}

static void cmd_phone_start_handler(struct dpram_link_device *dpld)
{
	struct io_device *iod = NULL;
	struct modem_data *mdata = dpld->link_pm_data->mdata;

	pr_info("MIF: <%s>\n", __func__);

	if (mdata->modem_type == QC_QSC6085)
		dpram_write_command(dpld, INT_CMD(INT_CMD_INIT_START));

	iod = dpram_find_iod(dpld, FMT_IDX);

	pr_info("MIF: Received 0xC8 from modem (Boot OK)\n");

	complete_all(&dpld->dpram_init_cmd);
	dpram_init_and_report(dpld);

	if (mdata->modem_type == QC_QSC6085)
		if (iod->mc->phone_state != STATE_ONLINE)
			iod->modem_state_changed(iod, STATE_ONLINE);
}

static void command_handler(struct dpram_link_device *dpld, u16 cmd)
{
	pr_info("MIF: <%s> cmd = 0x%X\n", __func__, INT_CMD_MASK(cmd));

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_REQ_ACTIVE:
		cmd_req_active_handler(dpld);
		break;

	case INT_CMD_ERR_DISPLAY:
		/* If modem crashes while PDA_SLEEP is in progres */
		complete(&dpld->link_pm_data->idpram_down);

		cmd_error_display_handler(dpld);
		break;

	case INT_CMD_PHONE_START:
		cmd_phone_start_handler(dpld);
		break;

	case INT_MASK_CMD_DPRAM_DOWN:
		idpram_power_down(dpld->link_pm_data);
		break;

	case INT_MASK_CMD_CP_WAKEUP_START:
		idpram_powerup_start(dpld->link_pm_data);
		break;

	case INT_CMD_NORMAL_POWER_OFF:
		complete(&dpld->cp_crash_done);
		cmd_error_display_handler(dpld);
		break;

	default:
		pr_err("MIF: <%s> Unknown command = 0x%X\n", __func__, cmd);
	}
}

static int dpram_process_modem_update(struct dpram_link_device *dpld,
					struct dpram_firmware *pfw)
{
	int ret = 0;
	char *buff = vmalloc(pfw->size);

	pr_info("MIF: <%s>\n", __func__);
	pr_info("[GOTA] modem size =[%d]\n", pfw->size);

	if (!buff)
		return -ENOMEM;

	ret = copy_from_user(buff, pfw->firmware, pfw->size);
	if (ret < 0) {
		pr_err("MIF: <%s> Copy from user failedi. Ln(%d)\n",
			__func__, __LINE__);
		goto out;
	}

	ret = dpram_download(dpld, buff, pfw->size);
	if (ret < 0)
		pr_err("firmware write failed\n");

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

	pr_info("MIF: <%s>\n", __func__);

	ret = copy_from_user(&fw, (void __user *)_arg, sizeof(fw));
	if (ret  < 0) {
		pr_err("copy from user failed!");
		return ret;
	}

	return dpram_process_modem_update(dpld, &fw);
}

static int dpram_dump_update(struct link_device *ld, struct io_device *iod,
							unsigned long _arg)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_firmware *fw = (struct dpram_firmware *)_arg ;

	pr_info("MIF: <%s>\n", __func__);

	return dpram_upload(dpld, fw);
}

static int dpram_start_ramdump(struct link_device *ld, struct io_device *iod)
{
	int ret;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	pr_info("MIF: <%s+>\n", __func__);

	dpram_writew(QSC_UPLOAD_MODE, &dpld->dpram->magic);

	/*
	when cp makes phone_active low, RIL has to trigger ramdump before cp
	makes phone_active high hence removing the below check
	*/
	/*
	while (!gpio_get_value(GPIO_QSC_PHONE_ACTIVE)) {
		msleep(100);
		pr_err("retying for PHONE_ACTIVE");
	}
	*/

	/* reset modem so that it goes to upload mode */
	/* ap does not need to reset cp during CRASH_EXIT case */
	if (gpio_get_value(iod->mc->gpio_phone_active)) {
		gpio_set_value(iod->mc->gpio_cp_reset, 0);
		msleep(100);
		gpio_set_value(iod->mc->gpio_cp_reset, 1);
	}

	dpram_write_command(dpld, CMD_CP_RAMDUMP_START_REQ);
	init_completion(&dpld->ramdump_cmd_done);
	ret = wait_for_completion_interruptible_timeout(
		&dpld->ramdump_cmd_done, RAMDUMP_CMD_TIMEOUT);
	if (!ret) {
		iod->ramdump_size = 0;
		pr_err("MIF: CP didn't respond to RAMDUMP_START_REQ\n");
		return ret;
	}

	iod->ramdump_addr = 0x00000000;
	iod->ramdump_size = MODEM_RAM_SIZE;
	iod->mc->ramdump_active = true;

	pr_info("MIF: <%s->\n", __func__);

	return ret;
}

static int dpram_read_ramdump(struct link_device *ld, struct io_device *iod)
{
	int ret;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct ramdump_cmd_hdr header;
	static char local_ramdump_buff[LOCAL_RAMDUMP_BUFF_SIZE];

	pr_debug("MIF: <%s+>\n", __func__);

	memset(&header, 0, sizeof(header));
	header.addr = iod->ramdump_addr;
	header.size = min(iod->ramdump_size, (u32) LOCAL_RAMDUMP_BUFF_SIZE);
	header.copyto_offset = 0x38000010;

	memcpy_toio((void *)((u8 *)dpld->dpram + 4), (void *)&header,
		sizeof(header));

	dpram_write_command(dpld, CMD_CP_RAMDUMP_SEND_REQ);
	init_completion(&dpld->ramdump_cmd_done);
	ret = wait_for_completion_interruptible_timeout(
		&dpld->ramdump_cmd_done, RAMDUMP_CMD_TIMEOUT);
	if (!ret) {
		iod->ramdump_size = 0;
		pr_err("MIF: CP didn't respond to RAMDUMP_SEND_REQ\n");
		return 0;
	}

	memcpy_fromio((void *) local_ramdump_buff,
		(void *) ((u8 *)dpld->dpram + 4 + 12), header.size);

#if defined(ENABLE_FORCED_CP_CRASH) || \
	defined(CP_CRASHES_ON_PDA_SLEEP_CMD)
	mif_save_file(fp, local_ramdump_buff, header.size);
#else
	iod->recv(iod, local_ramdump_buff, header.size);
#endif

	iod->ramdump_addr += header.size;
	iod->ramdump_size -= header.size;

	pr_debug("MIF: <%s-> remaining=%u\n", __func__, iod->ramdump_size);

	return header.size;
}

static int dpram_stop_ramdump(struct link_device *ld, struct io_device *iod)
{
	int ret;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	pr_info("MIF: <%s+>\n", __func__);

	dpram_write_command(dpld, CMD_CP_RAMDUMP_SEND_DONE_REQ);

	init_completion(&dpld->ramdump_cmd_done);
	ret = wait_for_completion_interruptible_timeout(
		&dpld->ramdump_cmd_done, RAMDUMP_CMD_TIMEOUT);
	if (!ret)
		pr_err("MIF: CP didn't respond to SEND_DONE_REQ\n");

	iod->mc->ramdump_active = false;

	iod = dpram_find_iod(dpld, FMT_IDX);
	iod->modem_state_changed(iod, STATE_BOOTING);

	pr_info("MIF: <%s->\n", __func__);

	return ret;
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
	pr_debug("=====> %s,  head: %d, tail: %d\n", __func__, head, tail);

	if (head == tail) {
		pr_err("MIF: <%s> head == tail\n", __func__);
		goto err_dpram_read;
	}

	if (!dpram_circ_valid(device->in_buff_size, head, tail)) {
		pr_err("MIF: <%s> invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		goto err_dpram_read;
	}

	iod = dpram_find_iod(dpld, dev_idx);
	if (!iod) {
		pr_err("MIF: iod == NULL\n");
		goto err_dpram_read;
	}

	/* Get data size in DPRAM*/
	size = (head > tail) ? (head - tail) :
		(device->in_buff_size - tail + head);

	/* ----- (tail) 7f 00 00 7e (head) ----- */
	if (head > tail) {
		buff = device->in_buff_addr + tail;
		if (iod->recv(iod, buff, size) < 0) {
			pr_err("MIF: <%s> recv error, dropping data\n",
								__func__);
			dpram_drop_data(device, head);
			goto err_dpram_read;
		}
	} else { /* 00 7e (head) ----------- (tail) 7f 00 */
		/* 1. tail -> buffer end.*/
		tmp_size = device->in_buff_size - tail;
		buff = device->in_buff_addr + tail;
		if (iod->recv(iod, buff, tmp_size) < 0) {
			pr_err("MIF: <%s> recv error, dropping data\n",
								__func__);
			dpram_drop_data(device, head);
			goto err_dpram_read;
		}

		/* 2. buffer start -> head.*/
		if (size > tmp_size) {
			buff = (char *)device->in_buff_addr;
			if (iod->recv(iod, buff, (size - tmp_size)) < 0) {
				pr_err("MIF: <%s> recv error, dropping data\n",
								__func__);
				dpram_drop_data(device, head);
				goto err_dpram_read;
			}
		}
	}

	/* new tail */
	tail = (u16)((tail + size) % device->in_buff_size);
	dpram_writeh(tail, &device->in->tail);

	/*
	printk_ratelimited(KERN_DEBUG "MIF: <%s>  len = %d\n", __func__, size);
	*/

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

	pr_debug("MIF: <%s+> non_cmd = 0x%04X\n", __func__, non_cmd);

	magic = dpram_readh(&dpld->dpram->magic);
	access = dpram_readh(&dpld->dpram->enable);

	if (!access || magic != DP_MAGIC_CODE) {
		pr_err("fmr receive error!!!!  access = 0x%X, magic =0x%X",
				access, magic);
		return;
	}

	/* Check formatted data region */
	device = &dpld->dev_map[FMT_IDX];
	head = dpram_readh(&device->in->head);
	tail = dpram_readh(&device->in->tail);

	if (!dpram_circ_valid(device->in_buff_size, head, tail)) {
		pr_err("MIF: <%s> invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		return;
	}

	if (head != tail) {
		if (non_cmd & INT_MASK_REQ_ACK_F)
			atomic_inc(&dpld->fmt_txq_req_ack_rcvd);

		ret = dpram_read(dpld, device, FMT_IDX);
		if (ret < 0)
			pr_err("MIFL <%s> dpram_read failed\n", __func__);

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
		pr_err("MIF: <%s> invalid circular buffer\n", __func__);
		dpram_zero_circ(device->in);
		return;
	}

	if (head != tail) {
		if (non_cmd & INT_MASK_REQ_ACK_R)
			atomic_inc(&dpld->raw_txq_req_ack_rcvd);

		ret = dpram_read(dpld, device, RAW_IDX);
		if (ret < 0)
			pr_err("MIF: <%s> dpram_read failed\n", __func__);

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

	pr_debug("MIF: <%s->\n", __func__);
}

static void gota_cmd_handler(struct dpram_link_device *dpld, u16 cmd)
{
	if (cmd & GOTA_RESULT_FAIL) {
		pr_err("[GOTA] Command failed: %04x\n", cmd);
		return;
	}
	complete(&dpld->ramdump_cmd_done);
	return;

	switch (GOTA_CMD_MASK(cmd)) {
	case CMD_CP_RAMDUMP_START_RESP:
	case CMD_CP_RAMDUMP_SEND_RESP:
	case CMD_CP_RAMDUMP_SEND_DONE_RESP:
		pr_info("[GOTA] MODEM RAM DUMP\n");
		complete(&dpld->ramdump_cmd_done);
		break;

	case GOTA_CMD_RECEIVE_READY:
		pr_info("[GOTA] Send CP-->AP RECEIVE_READY\n");
		dpram_write_command(dpld, CMD_DL_START_REQ);
		break;

	case GOTA_CMD_DOWNLOAD_START_RESP:
		pr_info("[GOTA] Send CP-->AP DOWNLOAD_START_RESP\n");
		complete_all(&dpld->gota_download_start_complete);
		break;

	case GOTA_CMD_SEND_DONE_RESP:
		pr_info("[GOTA] Send CP-->AP SEND_DONE_RESP\n");
		complete_all(&dpld->gota_send_done);
		break;

	case GOTA_CMD_UPDATE_DONE:
		pr_info("[GOTA] Send CP-->AP UPDATE_DONE\n");
		complete_all(&dpld->gota_update_done);
		break;

	case GOTA_CMD_IMAGE_SEND_RESP:
		pr_info("MIF: Send CP-->AP IMAGE_SEND_RESP\n");
		complete_all(&dpld->dump_receive_done);
		break;

	default:
		pr_err("[GOTA] Unknown command.. 0x%X\n", cmd);
	}
}

static irqreturn_t dpram_irq_handler(int irq, void *p_ld)
{
	u16 cp2ap;

	struct link_device *ld = (struct link_device *)p_ld;
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	cp2ap = dpram_readh(&dpld->dpram->mbx_cp2ap);

	pr_debug("MIF: <%s> rcvd CP2AP cmd = 0x%X\n", __func__, cp2ap);

	if (cp2ap == INT_POWERSAFE_FAIL) {
		pr_err("MIF: Received INT_POWERSAFE_FAIL\n");
		goto exit_irq;
	}

	if (GOTA_CMD_VALID(cp2ap))
		gota_cmd_handler(dpld, cp2ap);
	else if (INT_CMD_VALID(cp2ap))
		command_handler(dpld, cp2ap);
	else if (INT_VALID(cp2ap))
		non_command_handler(dpld, cp2ap);
	else
		pr_err("MIF: <%s> Invalid cmd = %04x\n", __func__, cp2ap);

exit_irq:
	dpld->clear_interrupt();

	return IRQ_HANDLED;
}

static int dpram_attach_io_dev(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	pr_info("MIF: <%s+>\n", __func__);

	iod->link = ld;

	/* list up io devices */
	list_add(&iod->list, &dpld->list_of_io_devices);

	switch (iod->format) {
	case IPC_FMT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = FMT_WAKE_TIME;
		break;

	case IPC_RFS:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RFS_WAKE_TIME;
		break;

	case IPC_MULTI_RAW:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;

	case IPC_BOOT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = 3 * HZ;

	default:
		break;
	}

	pr_info("MIF: <%s->\n", __func__);
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

	/* Internal DPRAM, check dpram ready?*/
	if (idpram_get_write_lock(dpld->link_pm_data)) {
		printk_ratelimited(KERN_ERR "MIF: <%s> resume in progres. "
			"retry later\n", __func__);
		return -EAGAIN;
	}

	/* If PDA is in transition to LPA */
	if (!gpio_get_value(dpld->link_pm_data->mdata->gpio_pda_active)) {
		pr_err("MIF: <%s> PDA_ACTIVE is low. retry later", __func__);
		return -EBUSY;
	}

	head = dpram_readh(&device->out->head);
	tail = dpram_readh(&device->out->tail);

	if (!dpram_circ_valid(device->out_buff_size, head, tail)) {
		pr_err("MIF: <%s> invalid circular buffer\n", __func__);
		dpram_zero_circ(device->out);
		return -EINVAL;
	}

	free_space = (head < tail) ? tail - head - 1 :
			device->out_buff_size + tail - head - 1;
	if (len > free_space) {
		pr_info("MIF: <%s> No space in Q."
			" pkt_len[%d] free_space[%d] head[%u] tail[%u]"
			" out_buff_size[%d]\n", __func__,
			len, free_space, head, tail, device->out_buff_size);
		return -EINVAL;
	}

	pr_debug("MIF: WRITE: len[%d] free_space[%d] head[%u] tail[%u]"
			" out_buff_size[%d]\n",
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

	/*
	printk_ratelimited(KERN_DEBUG "MIF: <%s> len = %d\n", __func__, len);
	*/

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

	/* don't write while cp's ramdump is in progress */
	struct io_device *iod = dpram_find_iod(dpld, FMT_IDX);
	if (iod->mc->ramdump_active) {
		pr_err("MIF: <%s> ramdump is in progress!\n", __func__);
		return;
	}

	pr_debug("MIF: <%s>\n", __func__);

	device = &dpld->dev_map[FMT_IDX];
	while ((skb = skb_dequeue(&ld->sk_fmt_tx_q))) {
		ret = dpram_write(dpld, device, skb->data, skb->len);
		if (ret < 0) {
			skb_queue_head(&ld->sk_fmt_tx_q, skb);
			if (ret != -EAGAIN)
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
			if (ret != -EAGAIN)
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

	pr_debug("MIF <%s+> fmt = %d, len = %d\n", __func__, iod->format, len);

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
	pr_debug("MIF <%s->\n", __func__);
	return len;
}

static void dpram_table_init(struct dpram_link_device *dpld)
{
	struct dpram_device *dev;
	struct dpram_map __iomem *dpram = dpld->dpram;

	pr_info("MIF: <%s>\n", __func__);

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

	pr_info("MIF: <%s>\n", __func__);

	dpram_writew(DP_MAGIC_DMDL, &dpld->dpram->magic);
	return 0;
}

static int dpram_set_ulmagic(struct link_device *ld, struct io_device *iod)
{
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	struct dpram_map *dpram = (void *)dpld->dpram;
	u8 *dest;

	pr_info("MIF: <%s>\n", __func__);

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

	pr_info("MIF: <%s>\n", __func__);

	header.start_index = START_INDEX;
	header.nframes = nframes;

	if (dpld->board_ota_reset != NULL)
		dpld->board_ota_reset();

	while (len > 0) {
		plen = min(len, DP_DEFAULT_WRITE_LEN);
		dest = (u8 *)&dpram->fmt_out;

		pr_info("[GOTA] Start write frame %d/%d\n", curframe, nframes);

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
			init_completion(&dpld->gota_download_start_complete);
			dpram_write_command(dpld, 0);

			ret = wait_for_completion_interruptible_timeout(
				&dpld->gota_download_start_complete,
				GOTA_TIMEOUT);
			if (!ret) {
				pr_err("[GOTA] CP didn't send DL_START\n");
				return -ENXIO;
			}
		}

		dpram_write_command(dpld, CMD_IMG_SEND_REQ);
		ret = wait_for_completion_interruptible_timeout(
				&dpld->gota_send_done, GOTA_SEND_TIMEOUT);
		if (!ret) {
			pr_err("[GOTA] CP didn't send SEND_DONE_RESP\n");
			return -ENXIO;
		}

		curframe++;
	}

	dpram_write_command(dpld, CMD_DL_SEND_DONE_REQ);
	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_update_done, GOTA_TIMEOUT);
	if (!ret) {
		pr_err("[GOTA] CP didn't send UPDATE_DONE_NOTIFICATION\n");
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

	pr_info("MIF: <%s>\n", __func__);

	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_download_start_complete,
			DUMP_START_TIMEOUT);
	if (!ret) {
		pr_err("[GOTA] CP didn't send DOWNLOAD_START\n");
		goto err_out;
	}

	wake_lock(&dpld->dpram_wake_lock);

	memset(buff, 0, DP_DEFAULT_DUMP_LEN);

	dpram_write_command(dpld, CMD_IMG_SEND_REQ);
	pr_info("MIF: write CMD_IMG_SEND_REQ(0x9400)\n");

	while (1) {
		init_completion(&dpld->dump_receive_done);
		ret = wait_for_completion_interruptible_timeout(
				&dpld->dump_receive_done, DUMP_TIMEOUT);
		if (!ret) {
			pr_err("MIF: CP didn't send DATA_SEND_DONE_RESP\n");
			goto err_out;
		}

		dest = (u8 *)(&dpram->fmt_out);

		header.bop = *(u16 *)(dest);
		header.total_frame = *(u16 *)(dest + 2);
		header.curr_frame = *(u16 *)(dest + 4);
		header.len = *(u16 *)(dest + 6);

		pr_err("total frame:%d, current frame:%d, data len:%d\n",
			header.total_frame, header.curr_frame,
			header.len);

		dest += DP_DUMP_HEADER_SIZE;
		plen = min(header.len, (u16)DP_DEFAULT_DUMP_LEN);

		memcpy(buff, dest, plen);
		dest += plen;

		ret = copy_to_user(uploaddata->firmware + tlen,	buff,  plen);
		if (ret < 0) {
			pr_err("MIF: Copy to user failed\n");
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

	pr_info("1st dump region data size=%d\n", uploaddata->size);
	pr_info("2st dump region data size=%d\n", uploaddata->is_delta);

	init_completion(&dpld->gota_send_done);
	ret = wait_for_completion_interruptible_timeout(
			&dpld->gota_send_done, DUMP_TIMEOUT);
	if (!ret) {
		pr_err("[GOTA] CP didn't send SEND_DONE_RESP\n");
		goto err_out;
	}

	dpram_write_command(dpld, CMD_UL_RECEIVE_DONE_RESP);
	pr_info("MIF: write CMD_UL_RECEIVE_DONE_RESP(0x9801)\n");

	dpram_writew(0, &dpld->dpram->magic); /*clear magic code */

	wake_unlock(&dpld->dpram_wake_lock);

	vfree(buff);
	return 0;

err_out:
	vfree(buff);
	dpram_writew(0, &dpld->dpram->magic);
	pr_err("CDMA dump error out\n");
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
	struct modem_data *pdata =
		(struct modem_data *)pdev->dev.platform_data;

	pr_info("MIF: <%s>\n", __func__);

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
	init_completion(&dpld->cp_crash_done);
	init_completion(&dpld->ramdump_cmd_done);

	ld->name = "dpram";
	ld->attach = dpram_attach_io_dev;
	ld->send = dpram_send;
	ld->gota_start = dpram_set_dlmagic;
	ld->modem_update = dpram_modem_update;
	ld->dump_start = dpram_set_ulmagic;
	ld->dump_update = dpram_dump_update;
	ld->start_ramdump = dpram_start_ramdump;
	ld->read_ramdump = dpram_read_ramdump;
	ld->stop_ramdump = dpram_stop_ramdump;

	dpld->clear_interrupt = pdata->clear_intr;
	if (pdata->ota_reset != NULL)
		dpld->board_ota_reset = pdata->ota_reset;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("MIF: Failed to get mem region\n");
		goto err;
	}
	dpld->dpram = ioremap(res->start, resource_size(res));
	pr_info("MIF: dpram start address:0x%08x\n", res->start);
	dpld->irq = platform_get_irq_byname(pdev, "dpram_irq");
	if (!dpld->irq) {
		pr_err("MIF: <%s> Failed to get IRQ\n", __func__);
		goto err;
	}

	dpram_table_init(dpld);

	atomic_set(&dpld->raw_txq_req_ack_rcvd, 0);
	atomic_set(&dpld->fmt_txq_req_ack_rcvd, 0);

	dpram_writeh(0, &dpld->dpram->magic);
	ret = idpram_link_pm_init(dpld, pdev);

	if (ret)
		pr_err("MIF: idpram_link_pm_init fail.(%d)\n", ret);

	flag = IRQF_DISABLED;
	printk(KERN_ERR "MIF: dpram irq : %d\n", dpld->irq);
	dpld->clear_interrupt();
	ret = request_irq(dpld->irq, dpram_irq_handler, flag,
							"dpram irq", ld);
	if (ret) {
		pr_err("MIF: DPRAM interrupt handler failed\n");
		goto err;
	}


	return ld;

err:
	iounmap(dpld->dpram);
	kfree(dpld);
	return NULL;
}
