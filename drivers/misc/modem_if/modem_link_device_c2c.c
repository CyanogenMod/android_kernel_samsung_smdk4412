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
#ifdef CONFIG_FB
#include <linux/fb.h>
#endif
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <linux/notifier.h>

#include "modem.h"
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_c2c.h"

static void trigger_forced_cp_crash(struct shmem_link_device *shmd);

#ifdef DEBUG_MODEM_IF
static void save_mem_dump(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	char *path = shmd->dump_path;
	struct file *fp;
	struct utc_time t;

	get_utc_time(&t);
	snprintf(path, MIF_MAX_PATH_LEN, "%s/%s_%d%02d%02d_%02d%02d%02d.dump",
		MIF_LOG_DIR, ld->name, t.year, t.mon, t.day, t.hour, t.min,
		t.sec);

	fp = mif_open_file(path);
	if (!fp) {
		mif_err("%s: ERR! %s open fail\n", ld->name, path);
		return;
	}
	mif_err("%s: %s opened\n", ld->name, path);

	mif_save_file(fp, shmd->base, shmd->size);

	mif_close_file(fp);
}

/**
 * mem_dump_work
 * @ws: pointer to an instance of work_struct structure
 *
 * Performs actual file operation for saving a DPRAM dump.
 */
static void mem_dump_work(struct work_struct *ws)
{
	struct shmem_link_device *shmd;

	shmd = container_of(ws, struct shmem_link_device, dump_dwork.work);
	if (!shmd) {
		mif_err("ERR! no shmd\n");
		return;
	}

	save_mem_dump(shmd);
}
#endif

static void print_pm_status(struct shmem_link_device *shmd, int level)
{
#ifdef DEBUG_MODEM_IF
	struct utc_time t;
	u32 magic;
	int ap_wakeup;
	int ap_status;
	int cp_wakeup;
	int cp_status;

	if (level < 0)
		return;

	get_utc_time(&t);
	magic = get_magic(shmd);
	ap_wakeup = gpio_get_value(shmd->gpio_ap_wakeup);
	ap_status = gpio_get_value(shmd->gpio_ap_status);
	cp_wakeup = gpio_get_value(shmd->gpio_cp_wakeup);
	cp_status = gpio_get_value(shmd->gpio_cp_status);

	/*
	** PM {ap_wakeup:cp_wakeup:cp_status:ap_status:magic:state} CALLER
	*/
	if (level > 0) {
		pr_err(LOG_TAG "[%02d:%02d:%02d.%03d] C2C PM {%d:%d:%d:%d:%X} "
			"%pf\n", t.hour, t.min, t.sec, t.msec, ap_wakeup,
			cp_wakeup, cp_status, ap_status, magic, CALLER);
	} else {
		pr_info(LOG_TAG "[%02d:%02d:%02d.%03d] C2C PM {%d:%d:%d:%d:%X} "
			"%pf\n", t.hour, t.min, t.sec, t.msec, ap_wakeup,
			cp_wakeup, cp_status, ap_status, magic, CALLER);
	}
#endif
}

static inline void s5p_change_irq_type(int irq, int value)
{
	irq_set_irq_type(irq, value ? IRQ_TYPE_LEVEL_LOW : IRQ_TYPE_LEVEL_HIGH);
	/**
	 * Exynos BSP has a problem when using level-triggering interrupt.
	 * If the irq type is changed in an interrupt handler, the handler will
	 * be called again.
	 * Below is a temporary solution until SYS.LSI resolves this problem.
	 */
	__raw_writel(eint_irq_to_bit(irq), S5P_EINT_PEND(EINT_REG_NR(irq)));
}

/**
 * recv_int2ap
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Returns the value of the CP-to-AP interrupt register.
 */
static inline u16 recv_int2ap(struct shmem_link_device *shmd)
{
	if (shmd->type == C2C_SHMEM)
		return (u16)c2c_read_interrupt();

	if (shmd->mbx2ap)
		return *shmd->mbx2ap;

	return 0;
}

/**
 * send_int2cp
 * @shmd: pointer to an instance of shmem_link_device structure
 * @mask: value to be written to the AP-to-CP interrupt register
 */
static inline void send_int2cp(struct shmem_link_device *shmd, u16 mask)
{
	struct link_device *ld = &shmd->ld;

	if (ld->mode != LINK_MODE_IPC)
		mif_info("%s: <by %pf> mask = 0x%04X\n", ld->name, CALLER, mask);

	if (shmd->type == C2C_SHMEM)
		c2c_send_interrupt(mask);

	if (shmd->mbx2cp)
		*shmd->mbx2cp = mask;
}

/**
 * get_shmem_status
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dir: direction of communication (TX or RX)
 * @mst: pointer to an instance of mem_status structure
 *
 * Takes a snapshot of the current status of a SHMEM.
 */
static void get_shmem_status(struct shmem_link_device *shmd,
			enum circ_dir_type dir, struct mem_status *mst)
{
#ifdef DEBUG_MODEM_IF
	getnstimeofday(&mst->ts);
#endif

	mst->dir = dir;
	mst->magic = get_magic(shmd);
	mst->access = get_access(shmd);
	mst->head[IPC_FMT][TX] = get_txq_head(shmd, IPC_FMT);
	mst->tail[IPC_FMT][TX] = get_txq_tail(shmd, IPC_FMT);
	mst->head[IPC_FMT][RX] = get_rxq_head(shmd, IPC_FMT);
	mst->tail[IPC_FMT][RX] = get_rxq_tail(shmd, IPC_FMT);
	mst->head[IPC_RAW][TX] = get_txq_head(shmd, IPC_RAW);
	mst->tail[IPC_RAW][TX] = get_txq_tail(shmd, IPC_RAW);
	mst->head[IPC_RAW][RX] = get_rxq_head(shmd, IPC_RAW);
	mst->tail[IPC_RAW][RX] = get_rxq_tail(shmd, IPC_RAW);
}

static inline int check_link_status(struct shmem_link_device *shmd)
{
	u32 magic = get_magic(shmd);
	int cnt;

	if (gpio_get_value(shmd->gpio_cp_status) != 0 && magic == SHM_IPC_MAGIC)
		return 0;

	cnt = 0;
	while (gpio_get_value(shmd->gpio_cp_status) == 0) {
		if (gpio_get_value(shmd->gpio_ap_status) == 0) {
			print_pm_status(shmd, 1);
			gpio_set_value(shmd->gpio_ap_status, 1);
		}

		cnt++;
		if (cnt >= 100) {
			mif_err("ERR! cp_status != 1 (cnt %d)\n", cnt);
			return -EACCES;
		}

		if (in_interrupt())
			udelay(100);
		else
			usleep_range(100, 200);
	}

	cnt = 0;
	while (1) {
		magic = get_magic(shmd);
		if (magic == SHM_IPC_MAGIC)
			break;

		cnt++;
		if (cnt >= 100) {
			mif_err("ERR! magic 0x%X != SHM_IPC_MAGIC (cnt %d)\n",
				magic, cnt);
			return -EACCES;
		}

		if (in_interrupt())
			udelay(100);
		else
			usleep_range(100, 200);
	}

	return 0;
}

static void release_cp_wakeup(struct work_struct *ws)
{
	struct shmem_link_device *shmd;
	struct link_device *ld;
	int i;
	unsigned long flags;

	shmd = container_of(ws, struct shmem_link_device, cp_sleep_dwork.work);

	spin_lock_irqsave(&shmd->pm_lock, flags);
	i = atomic_read(&shmd->ref_cnt);
	spin_unlock_irqrestore(&shmd->pm_lock, flags);
	if (i > 0)
		goto reschedule;

	/*
	 * If there is any IPC message remained in a TXQ, AP must prevent CP
	 * from going to sleep.
	 */
	ld = &shmd->ld;
	for (i = 0; i < ld->max_ipc_dev; i++) {
		if (ld->skb_txq[i]->qlen > 0)
			goto reschedule;
	}

	if (gpio_get_value(shmd->gpio_ap_wakeup))
		goto reschedule;

	gpio_set_value(shmd->gpio_cp_wakeup, 0);
	print_pm_status(shmd, 1);
	return;

reschedule:
	if (!work_pending(&shmd->cp_sleep_dwork.work)) {
		queue_delayed_work(system_nrt_wq, &shmd->cp_sleep_dwork,
				   msecs_to_jiffies(CP_WAKEUP_HOLD_TIME));
	}
}

static void release_ap_status(struct work_struct *ws)
{
	struct shmem_link_device *shmd;
	struct link_device *ld;
	int i;
	unsigned long flags;

	shmd = container_of(ws, struct shmem_link_device, link_off_dwork.work);

	spin_lock_irqsave(&shmd->pm_lock, flags);
	i = atomic_read(&shmd->ref_cnt);
	spin_unlock_irqrestore(&shmd->pm_lock, flags);
	if (i > 0)
		goto reschedule;

	if (gpio_get_value(shmd->gpio_cp_status))
		goto reschedule;

	gpio_set_value(shmd->gpio_ap_status, 0);
	print_pm_status(shmd, 1);

	if (wake_lock_active(&shmd->cp_wlock))
		wake_unlock(&shmd->cp_wlock);

	return;

reschedule:
	if (!work_pending(&shmd->link_off_dwork.work)) {
		queue_delayed_work(system_nrt_wq, &shmd->link_off_dwork,
				   msecs_to_jiffies(100));
	}
}

/**
 * forbid_cp_sleep
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Wakes up a CP if it can sleep and increases the "ref_cnt" counter in the
 * shmem_link_device instance.
 *
 * CAUTION!!! permit_cp_sleep() MUST be invoked after forbid_cp_sleep() success
 * to decrease the "ref_cnt" counter.
 */
static int forbid_cp_sleep(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	int err = 0;
	unsigned long flags;

	spin_lock_irqsave(&shmd->pm_lock, flags);
	atomic_inc(&shmd->ref_cnt);
	if (gpio_get_value(shmd->gpio_ap_status) == 0) {
		gpio_set_value(shmd->gpio_ap_status, 1);
		print_pm_status(shmd, 1);
	}
	spin_unlock_irqrestore(&shmd->pm_lock, flags);

	if (work_pending(&shmd->cp_sleep_dwork.work))
		cancel_delayed_work(&shmd->cp_sleep_dwork);

	if (work_pending(&shmd->link_off_dwork.work))
		cancel_delayed_work(&shmd->link_off_dwork);

	if (gpio_get_value(shmd->gpio_cp_wakeup) == 0) {
		gpio_set_value(shmd->gpio_cp_wakeup, 1);
		print_pm_status(shmd, 1);
	}

	if (check_link_status(shmd) < 0) {
		print_pm_status(shmd, 1);
		mif_err("%s: ERR! check_link_status fail\n", ld->name);
		err = -EACCES;
		goto exit;
	}

exit:
	return err;
}

/**
 * permit_cp_sleep
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Decreases the "ref_cnt" counter in the shmem_link_device instance if it can
 * sleep and allows a CP to sleep only if the value of "ref_cnt" counter is
 * less than or equal to 0.
 *
 * MUST be invoked after forbid_cp_sleep() success to decrease the "ref_cnt"
 * counter.
 */
static void permit_cp_sleep(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	unsigned long flags;

	spin_lock_irqsave(&shmd->pm_lock, flags);

	if (atomic_dec_return(&shmd->ref_cnt) > 0) {
		spin_unlock_irqrestore(&shmd->pm_lock, flags);
		return;
	}

	atomic_set(&shmd->ref_cnt, 0);
	spin_unlock_irqrestore(&shmd->pm_lock, flags);

	/* Hold gpio_cp_wakeup for CP_WAKEUP_HOLD_TIME until CP finishes RX */
	if (!work_pending(&shmd->cp_sleep_dwork.work)) {
		queue_delayed_work(system_nrt_wq, &shmd->cp_sleep_dwork,
				   msecs_to_jiffies(CP_WAKEUP_HOLD_TIME));
	}
}

/**
 * ap_wakeup_handler: interrupt handler for a wakeup interrupt
 * @irq: IRQ number
 * @data: pointer to a data
 *
 * 1) Reads the interrupt value
 * 2) Performs interrupt handling
 */
static irqreturn_t ap_wakeup_handler(int irq, void *data)
{
	struct shmem_link_device *shmd = (struct shmem_link_device *)data;
	struct link_device *ld = (struct link_device *)&shmd->ld;
	int ap_wakeup = gpio_get_value(shmd->gpio_ap_wakeup);
	int ap_status = gpio_get_value(shmd->gpio_ap_status);

	s5p_change_irq_type(irq, ap_wakeup);

	if (ld->mode != LINK_MODE_IPC)
		goto exit;

	if (work_pending(&shmd->cp_sleep_dwork.work))
		__cancel_delayed_work(&shmd->cp_sleep_dwork);

	print_pm_status(shmd, 1);

	if (ap_wakeup) {
		if (work_pending(&shmd->link_off_dwork.work))
			__cancel_delayed_work(&shmd->link_off_dwork);

		if (!wake_lock_active(&shmd->ap_wlock))
			wake_lock(&shmd->ap_wlock);

		if (!c2c_suspended() && !ap_status)
			gpio_set_value(shmd->gpio_ap_status, 1);
	} else {
		if (wake_lock_active(&shmd->ap_wlock))
			wake_unlock(&shmd->ap_wlock);

		queue_delayed_work(system_nrt_wq, &shmd->cp_sleep_dwork,
				   msecs_to_jiffies(CP_WAKEUP_HOLD_TIME));
	}

exit:
	return IRQ_HANDLED;
}

static irqreturn_t cp_status_handler(int irq, void *data)
{
	struct shmem_link_device *shmd = (struct shmem_link_device *)data;
	struct link_device *ld = (struct link_device *)&shmd->ld;
	int cp_status = gpio_get_value(shmd->gpio_cp_status);
	unsigned long flags;

	spin_lock_irqsave(&shmd->pm_lock, flags);

	s5p_change_irq_type(irq, cp_status);

	if (ld->mode != LINK_MODE_IPC)
		goto exit;

	if (work_pending(&shmd->link_off_dwork.work))
		__cancel_delayed_work(&shmd->link_off_dwork);

	print_pm_status(shmd, 1);

	if (cp_status) {
		if (!wake_lock_active(&shmd->cp_wlock))
			wake_lock(&shmd->cp_wlock);
	} else {
		if (atomic_read(&shmd->ref_cnt) > 0) {
			queue_delayed_work(system_nrt_wq, &shmd->link_off_dwork,
					   msecs_to_jiffies(10));
		} else {
			gpio_set_value(shmd->gpio_ap_status, 0);
			if (wake_lock_active(&shmd->cp_wlock))
				wake_unlock(&shmd->cp_wlock);
		}
	}

exit:
	spin_unlock_irqrestore(&shmd->pm_lock, flags);
	return IRQ_HANDLED;
}

#if 1
/* Functions for IPC/BOOT/DUMP RX */
#endif

/**
 * handle_cp_crash
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Actual handler for the CRASH_EXIT command from a CP.
 */
static void handle_cp_crash(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	struct io_device *iod;
	int i;

	if (shmd->forced_cp_crash)
		shmd->forced_cp_crash = false;

	/* Stop network interfaces */
	mif_netif_stop(ld);

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	for (i = 0; i < MAX_SIPC5_DEV; i++)
		skb_queue_purge(ld->skb_txq[i]);

	/* Change the modem state to STATE_CRASH_EXIT for the FMT IO device */
	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);

	/* time margin for taking state changes by rild */
	mdelay(100);

	/* Change the modem state to STATE_CRASH_EXIT for the BOOT IO device */
	iod = link_get_iod_with_format(ld, IPC_BOOT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_EXIT);
}

/**
 * handle_no_cp_crash_ack
 * @arg: pointer to an instance of shmem_link_device structure
 *
 * Invokes handle_cp_crash() to enter the CRASH_EXIT state if there was no
 * CRASH_ACK from a CP in FORCE_CRASH_ACK_TIMEOUT.
 */
static void handle_no_cp_crash_ack(unsigned long arg)
{
	struct shmem_link_device *shmd = (struct shmem_link_device *)arg;
	struct link_device *ld = &shmd->ld;

	mif_err("%s: ERR! No CRASH_EXIT ACK from CP\n", ld->name);

	handle_cp_crash(shmd);
}

/**
 * trigger_forced_cp_crash
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Triggers an enforced CP crash.
 */
static void trigger_forced_cp_crash(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	struct utc_time t;

	if (ld->mode == LINK_MODE_ULOAD) {
		mif_err("%s: <by %pf> ALREADY in progress\n", ld->name, CALLER);
		return;
	}
	ld->mode = LINK_MODE_ULOAD;
	shmd->forced_cp_crash = true;

	get_utc_time(&t);
	mif_err("%s: [%02d:%02d:%02d.%03d] <by %pf>\n",
		ld->name, t.hour, t.min, t.sec, t.msec, CALLER);

	if (!wake_lock_active(&shmd->wlock))
		wake_lock(&shmd->wlock);

#ifdef DEBUG_MODEM_IF
	if (in_interrupt())
		queue_delayed_work(system_nrt_wq, &shmd->dump_dwork, 0);
	else
		save_mem_dump(shmd);
#endif

	/* Send CRASH_EXIT command to a CP */
	send_int2cp(shmd, INT_CMD(INT_CMD_CRASH_EXIT));
	get_shmem_status(shmd, TX, msq_get_free_slot(&shmd->stat_list));

	/* If there is no CRASH_ACK from a CP in FORCE_CRASH_ACK_TIMEOUT,
	   handle_no_cp_crash_ack() will be executed. */
	mif_add_timer(&shmd->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			handle_no_cp_crash_ack, (unsigned long)shmd);

	return;
}

/**
 * cmd_crash_reset_handler
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Handles the CRASH_RESET command from a CP.
 */
static void cmd_crash_reset_handler(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	struct io_device *iod = NULL;
	int i;
	struct utc_time t;

	ld->mode = LINK_MODE_ULOAD;

	if (!wake_lock_active(&shmd->wlock))
		wake_lock(&shmd->wlock);

	get_utc_time(&t);
	mif_err("%s: ERR! [%02d:%02d:%02d.%03d] Recv 0xC7 (CRASH_RESET)\n",
		ld->name, t.hour, t.min, t.sec, t.msec);
#ifdef DEBUG_MODEM_IF
	queue_delayed_work(system_nrt_wq, &shmd->dump_dwork, 0);
#endif

	/* Stop network interfaces */
	mif_netif_stop(ld);

	/* Purge the skb_txq in every IPC device (IPC_FMT, IPC_RAW, etc.) */
	for (i = 0; i < MAX_SIPC5_DEV; i++)
		skb_queue_purge(ld->skb_txq[i]);

	/* Change the modem state to STATE_CRASH_RESET for the FMT IO device */
	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_RESET);

	/* time margin for taking state changes by rild */
	mdelay(100);

	/* Change the modem state to STATE_CRASH_RESET for the BOOT IO device */
	iod = link_get_iod_with_format(ld, IPC_BOOT);
	if (iod)
		iod->modem_state_changed(iod, STATE_CRASH_RESET);
}

/**
 * cmd_crash_exit_handler
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Handles the CRASH_EXIT command from a CP.
 */
static void cmd_crash_exit_handler(struct shmem_link_device *shmd)
{
	struct link_device *ld = &shmd->ld;
	struct utc_time t;

	ld->mode = LINK_MODE_ULOAD;

	del_timer(&shmd->crash_ack_timer);

	if (!wake_lock_active(&shmd->wlock))
		wake_lock(&shmd->wlock);

	get_utc_time(&t);
	mif_err("%s: ERR! [%02d:%02d:%02d.%03d] Recv 0xC9 (CRASH_EXIT)\n",
		ld->name, t.hour, t.min, t.sec, t.msec);
#ifdef DEBUG_MODEM_IF
	queue_delayed_work(system_nrt_wq, &shmd->dump_dwork, 0);
#endif

	handle_cp_crash(shmd);
}

/**
 * cmd_phone_start_handler
 * @shmd: pointer to an instance of shmem_link_device structure
 *
 * Handles the PHONE_START command from a CP.
 */
static void cmd_phone_start_handler(struct shmem_link_device *shmd)
{
	int err;
	struct link_device *ld = &shmd->ld;
	struct io_device *iod;
	int ap_wakeup = gpio_get_value(shmd->gpio_ap_wakeup);
	int cp_status = gpio_get_value(shmd->gpio_cp_status);

	mif_err("%s: Recv 0xC8 (CP_START)\n", ld->name);

	iod = link_get_iod_with_format(ld, IPC_FMT);
	if (!iod) {
		mif_err("%s: ERR! no iod\n", ld->name);
		return;
	}

	err = init_shmem_ipc(shmd);
	if (err)
		return;

	if (wake_lock_active(&shmd->wlock))
		wake_unlock(&shmd->wlock);

	s5p_change_irq_type(shmd->irq_ap_wakeup, ap_wakeup);
	if (ap_wakeup && !wake_lock_active(&shmd->ap_wlock))
		wake_lock(&shmd->ap_wlock);

	s5p_change_irq_type(shmd->irq_cp_status, cp_status);
	if (cp_status && !wake_lock_active(&shmd->ap_wlock))
		wake_lock(&shmd->cp_wlock);

	ld->mode = LINK_MODE_IPC;
	iod->modem_state_changed(iod, STATE_ONLINE);
}

/**
 * cmd_handler: processes a SHMEM command from a CP
 * @shmd: pointer to an instance of shmem_link_device structure
 * @cmd: SHMEM command from a CP
 */
static void cmd_handler(struct shmem_link_device *shmd, u16 cmd)
{
	struct link_device *ld = &shmd->ld;

	switch (INT_CMD_MASK(cmd)) {
	case INT_CMD_CRASH_RESET:
		cmd_crash_reset_handler(shmd);
		break;

	case INT_CMD_CRASH_EXIT:
		cmd_crash_exit_handler(shmd);
		break;

	case INT_CMD_PHONE_START:
		cmd_phone_start_handler(shmd);
		complete_all(&ld->init_cmpl);
		break;

	default:
		mif_err("%s: unknown command 0x%04X\n", ld->name, cmd);
		break;
	}
}

/**
 * ipc_rx_work
 * @ws: pointer to an instance of work_struct structure
 *
 * Invokes the recv method in the io_device instance to perform receiving IPC
 * messages from each skb.
 */
static void ipc_rx_work(struct work_struct *ws)
{
	struct shmem_link_device *shmd;
	struct link_device *ld;
	struct io_device *iod;
	struct sk_buff *skb;
	int i;

	shmd = container_of(ws, struct shmem_link_device, ipc_rx_dwork.work);
	ld = &shmd->ld;

	for (i = 0; i < MAX_SIPC5_DEV; i++) {
		iod = shmd->iod[i];
		while (1) {
			skb = skb_dequeue(ld->skb_rxq[i]);
			if (!skb)
				break;
			iod->recv_skb(iod, ld, skb);
		}
	}
}

/**
 * rx_ipc_frames
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : ILLEGAL status
 *   ret > 0  : valid data
 *
 * Must be invoked only when there is data in the corresponding RXQ.
 *
 * Requires a recv_skb method in the io_device instance, so this function must
 * be used for only SIPC5.
 */
static int rx_ipc_frames(struct shmem_link_device *shmd, int dev,
			struct circ_status *circ)
{
	struct link_device *ld = &shmd->ld;
	struct sk_buff_head *rxq = ld->skb_rxq[dev];
	struct sk_buff *skb;
	int ret;
	/**
	 * variables for the status of the circular queue
	 */
	u8 *src;
	u8 hdr[SIPC5_MIN_HEADER_SIZE];
	/**
	 * variables for RX processing
	 */
	int qsize;	/* size of the queue			*/
	int rcvd;	/* size of data in the RXQ or error	*/
	int rest;	/* size of the rest data		*/
	int out;	/* index to the start of current frame	*/
	u8 *dst;	/* pointer to the destination buffer	*/
	int tot;	/* total length including padding data	*/

	src = circ->buff;
	qsize = circ->qsize;
	out = circ->out;
	rcvd = circ->size;

	rest = rcvd;
	tot = 0;
	while (rest > 0) {
		/* Copy the header in the frame to the header buffer */
		circ_read(hdr, src, qsize, out, SIPC5_MIN_HEADER_SIZE);

		/* Check the config field in the header */
		if (unlikely(!sipc5_start_valid(hdr))) {
			mif_err("%s: ERR! %s INVALID config 0x%02X "
				"(rcvd %d, rest %d)\n", ld->name,
				get_dev_name(dev), hdr[0], rcvd, rest);
			ret = -EBADMSG;
			goto exit;
		}

		/* Verify the total length of the frame (data + padding) */
		tot = sipc5_get_total_len(hdr);
		if (unlikely(tot > rest)) {
			mif_err("%s: ERR! %s tot %d > rest %d (rcvd %d)\n",
				ld->name, get_dev_name(dev), tot, rest, rcvd);
			ret = -EBADMSG;
			goto exit;
		}

		/* Allocate an skb */
		skb = dev_alloc_skb(tot);
		if (!skb) {
			mif_err("%s: ERR! %s dev_alloc_skb fail\n",
				ld->name, get_dev_name(dev));
			ret = -ENOMEM;
			goto exit;
		}

		/* Set the attribute of the skb as "single frame" */
		skbpriv(skb)->single_frame = true;

		/* Read the frame from the RXQ */
		dst = skb_put(skb, tot);
		circ_read(dst, src, qsize, out, tot);

		/* Store the skb to the corresponding skb_rxq */
		skb_queue_tail(rxq, skb);

		/* Calculate new out value */
		rest -= tot;
		out += tot;
		if (unlikely(out >= qsize))
			out -= qsize;
	}

	/* Update tail (out) pointer to empty out the RXQ */
	set_rxq_tail(shmd, dev, circ->in);

	return rcvd;

exit:
#ifdef DEBUG_MODEM_IF
	mif_err("%s: ERR! rcvd:%d tot:%d rest:%d\n", ld->name, rcvd, tot, rest);
	pr_ipc(1, "c2c: ERR! CP2MIF", (src + out), (rest > 20) ? 20 : rest);
#endif

	return ret;
}

/**
 * msg_handler: receives IPC messages from every RXQ
 * @shmd: pointer to an instance of shmem_link_device structure
 * @mst: pointer to an instance of mem_status structure
 *
 * 1) Receives all IPC message frames currently in every IPC RXQ.
 * 2) Sends RES_ACK responses if there are REQ_ACK requests from a CP.
 * 3) Completes all threads waiting for the corresponding RES_ACK from a CP if
 *    there is any RES_ACK response.
 */
static void msg_handler(struct shmem_link_device *shmd, struct mem_status *mst)
{
	struct link_device *ld = &shmd->ld;
	struct circ_status circ;
	int i = 0;
	int ret = 0;

	if (!ipc_active(shmd))
		return;

	/* Read data from every RXQ */
	for (i = 0; i < MAX_SIPC5_DEV; i++) {
		/* Invoke an RX function only when there is data in the RXQ */
		if (likely(mst->head[i][RX] != mst->tail[i][RX])) {
			ret = get_rxq_rcvd(shmd, i, mst, &circ);
			if (unlikely(ret < 0)) {
				mif_err("%s: ERR! get_rxq_rcvd fail (err %d)\n",
					ld->name, ret);
#ifdef DEBUG_MODEM_IF
				trigger_forced_cp_crash(shmd);
#endif
				return;
			}

			ret = rx_ipc_frames(shmd, i, &circ);
			if (ret < 0) {
#ifdef DEBUG_MODEM_IF
				trigger_forced_cp_crash(shmd);
#endif
				reset_rxq_circ(shmd, i);
			}
		}
	}

	/* Schedule soft IRQ for RX */
	queue_delayed_work(system_nrt_wq, &shmd->ipc_rx_dwork, 0);
}

static void msg_rx_task(unsigned long data)
{
	struct shmem_link_device *shmd = (struct shmem_link_device *)data;
	struct link_device *ld = &shmd->ld;
	struct mem_status mst;
	u16 mask = 0;

	get_shmem_status(shmd, RX, &mst);

	if ((mst.head[IPC_FMT][RX] != mst.tail[IPC_FMT][RX])
	    || (mst.head[IPC_RAW][RX] != mst.tail[IPC_RAW][RX])) {
#if 0
		print_mem_status(ld, &mst);
#endif
		msg_handler(shmd, &mst);
	}

	/*
	** Check and process REQ_ACK (at this time, in == out)
	*/
	if (unlikely(shmd->dev[IPC_FMT]->req_ack_rcvd)) {
		mask |= get_mask_res_ack(shmd, IPC_FMT);
		shmd->dev[IPC_FMT]->req_ack_rcvd = 0;
	}

	if (unlikely(shmd->dev[IPC_RAW]->req_ack_rcvd)) {
		mask |= get_mask_res_ack(shmd, IPC_RAW);
		shmd->dev[IPC_RAW]->req_ack_rcvd = 0;
	}

	if (unlikely(mask)) {
#ifdef DEBUG_MODEM_IF
		mif_info("%s: send RES_ACK 0x%04X\n", ld->name, mask);
#endif
		send_int2cp(shmd, INT_NON_CMD(mask));
	}
}

/**
 * ipc_handler: processes a SHMEM command or receives IPC messages
 * @shmd: pointer to an instance of shmem_link_device structure
 * @mst: pointer to an instance of mem_status structure
 *
 * Invokes cmd_handler for a SHMEM command or msg_handler for IPC messages.
 */
static void ipc_handler(struct shmem_link_device *shmd, struct mem_status *mst)
{
#ifdef DEBUG_MODEM_IF
	struct link_device *ld = &shmd->ld;
#endif
	u16 intr = mst->int2ap;

	if (unlikely(INT_CMD_VALID(intr))) {
		cmd_handler(shmd, intr);
		return;
	}

	/*
	** Check REQ_ACK from CP -> REQ_ACK will be processed in the RX tasklet
	*/
	if (unlikely(intr & get_mask_req_ack(shmd, IPC_FMT)))
		shmd->dev[IPC_FMT]->req_ack_rcvd = 1;

	if (unlikely(intr & get_mask_req_ack(shmd, IPC_RAW)))
		shmd->dev[IPC_RAW]->req_ack_rcvd = 1;

	/*
	** Check and process RES_ACK from CP
	*/
	if (unlikely(intr & get_mask_res_ack(shmd, IPC_FMT))) {
#ifdef DEBUG_MODEM_IF
		mif_info("%s: recv FMT RES_ACK\n", ld->name);
#endif
		complete(&shmd->req_ack_cmpl[IPC_FMT]);
	}

	if (unlikely(intr & get_mask_res_ack(shmd, IPC_RAW))) {
#ifdef DEBUG_MODEM_IF
		mif_info("%s: recv RAW RES_ACK\n", ld->name);
#endif
		complete(&shmd->req_ack_cmpl[IPC_RAW]);
	}

	/*
	** Schedule RX tasklet
	*/
	tasklet_hi_schedule(&shmd->rx_tsk);
}

/**
 * rx_udl_frames
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @mst: pointer to an instance of mem_status structure
 *
 * Returns
 *   ret < 0  : error
 *   ret == 0 : ILLEGAL status
 *   ret > 0  : valid data
 *
 * Must be invoked only when there is data in the corresponding RXQ.
 *
 * Requires a recv_skb method in the io_device instance, so this function must
 * be used for only SIPC5.
 */
static int rx_udl_frames(struct shmem_link_device *shmd, int dev,
			struct circ_status *circ)
{
	struct link_device *ld = &shmd->ld;
	struct io_device *iod;
	struct sk_buff *skb;
	int ret;
	/**
	 * variables for the status of the circular queue
	 */
	u8 *src;
	u8 hdr[SIPC5_MIN_HEADER_SIZE];
	/**
	 * variables for RX processing
	 */
	int qsize;	/* size of the queue			*/
	int rcvd;	/* size of data in the RXQ or error	*/
	int rest;	/* size of the rest data		*/
	int out;	/* index to the start of current frame	*/
	u8 *dst;	/* pointer to the destination buffer	*/
	int tot;	/* total length including padding data	*/

	src = circ->buff;
	qsize = circ->qsize;
	out = circ->out;
	rcvd = circ->size;

	rest = rcvd;
	tot = 0;
	while (rest > 0) {
		/* Copy the header in the frame to the header buffer */
		circ_read(hdr, src, qsize, out, SIPC5_MIN_HEADER_SIZE);

		/* Check the config field in the header */
		if (unlikely(!sipc5_start_valid(hdr))) {
			mif_err("%s: ERR! %s INVALID config 0x%02X "
				"(rest %d, rcvd %d)\n", ld->name,
				get_dev_name(dev), hdr[0], rest, rcvd);
			pr_ipc(1, "UDL", (src + out), (rest > 20) ? 20 : rest);
			ret = -EBADMSG;
			goto exit;
		}

		/* Verify the total length of the frame (data + padding) */
		tot = sipc5_get_total_len(hdr);
		if (unlikely(tot > rest)) {
			mif_err("%s: ERR! %s tot %d > rest %d (rcvd %d)\n",
				ld->name, get_dev_name(dev), tot, rest, rcvd);
			ret = -ENODATA;
			goto exit;
		}

		/* Allocate an skb */
		skb = alloc_skb(tot + NET_SKB_PAD, GFP_KERNEL);
		if (!skb) {
			mif_err("%s: ERR! %s alloc_skb fail\n",
				ld->name, get_dev_name(dev));
			ret = -ENOMEM;
			goto exit;
		}
		skb_reserve(skb, NET_SKB_PAD);

		/* Set the attribute of the skb as "single frame" */
		skbpriv(skb)->single_frame = true;

		/* Read the frame from the RXQ */
		dst = skb_put(skb, tot);
		circ_read(dst, src, qsize, out, tot);

		/* Pass the skb to an iod */
		iod = link_get_iod_with_channel(ld, sipc5_get_ch_id(skb->data));
		if (!iod) {
			mif_err("%s: ERR! no IPC_BOOT iod\n", ld->name);
			break;
		}

#ifdef DEBUG_MODEM_IF
		if (!std_udl_with_payload(std_udl_get_cmd(skb->data))) {
			if (ld->mode == LINK_MODE_DLOAD) {
				pr_ipc(0, "[CP->AP] DL CMD", skb->data,
					(skb->len > 20 ? 20 : skb->len));
			} else {
				pr_ipc(0, "[CP->AP] UL CMD", skb->data,
					(skb->len > 20 ? 20 : skb->len));
			}
		}
#endif

		iod->recv_skb(iod, ld, skb);

		/* Calculate new out value */
		rest -= tot;
		out += tot;
		if (unlikely(out >= qsize))
			out -= qsize;
	}

	/* Update tail (out) pointer to empty out the RXQ */
	set_rxq_tail(shmd, dev, circ->in);

	return rcvd;

exit:
	return ret;
}

/**
 * udl_rx_work
 * @ws: pointer to an instance of the work_struct structure
 *
 * Invokes the recv method in the io_device instance to perform receiving IPC
 * messages from each skb.
 */
static void udl_rx_work(struct work_struct *ws)
{
	struct shmem_link_device *shmd;
	struct link_device *ld;
	struct sk_buff_head *rxq;
	struct mem_status mst;
	struct circ_status circ;
	int dev = IPC_RAW;

	shmd = container_of(ws, struct shmem_link_device, udl_rx_dwork.work);
	ld = &shmd->ld;
	rxq = ld->skb_rxq[dev];

	while (1) {
		get_shmem_status(shmd, RX, &mst);
		if (mst.head[dev][RX] == mst.tail[dev][RX])
			break;

		/* Invoke an RX function only when there is data in the RXQ */
		if (get_rxq_rcvd(shmd, dev, &mst, &circ) < 0) {
			mif_err("%s: ERR! get_rxq_rcvd fail\n", ld->name);
#ifdef DEBUG_MODEM_IF
			trigger_forced_cp_crash(shmd);
#endif
			break;
		}

		if (rx_udl_frames(shmd, dev, &circ) < 0) {
			skb_queue_purge(rxq);
			break;
		}
	}
}

/**
 * udl_handler: receives BOOT/DUMP IPC messages from every RXQ
 * @shmd: pointer to an instance of shmem_link_device structure
 * @mst: pointer to an instance of mem_status structure
 *
 * 1) Receives all IPC message frames currently in every IPC RXQ.
 * 2) Sends RES_ACK responses if there are REQ_ACK requests from a CP.
 * 3) Completes all threads waiting for the corresponding RES_ACK from a CP if
 *    there is any RES_ACK response.
 */
static void udl_handler(struct shmem_link_device *shmd, struct mem_status *mst)
{
	u16 intr = mst->int2ap;

	/* Schedule soft IRQ for RX */
	queue_delayed_work(system_nrt_wq, &shmd->udl_rx_dwork, 0);

	/* Check and process RES_ACK */
	if (intr & INT_MASK_RES_ACK_SET) {
		if (intr & get_mask_res_ack(shmd, IPC_RAW)) {
#ifdef DEBUG_MODEM_IF
			struct link_device *ld = &shmd->ld;
			mif_info("%s: recv RAW RES_ACK\n", ld->name);
			print_circ_status(ld, IPC_RAW, mst);
#endif
			complete(&shmd->req_ack_cmpl[IPC_RAW]);
		}
	}
}

/**
 * c2c_irq_handler: interrupt handler for a C2C interrupt
 * @data: pointer to a data
 *
 * 1) Reads the interrupt value
 * 2) Performs interrupt handling
 *
 * Flow for normal interrupt handling:
 *   c2c_irq_handler -> udl_handler
 *   c2c_irq_handler -> ipc_handler -> cmd_handler -> cmd_xxx_handler
 *   c2c_irq_handler -> ipc_handler -> msg_handler -> rx_ipc_frames ->  ...
 */
static void c2c_irq_handler(void *data, u32 intr)
{
	struct shmem_link_device *shmd = (struct shmem_link_device *)data;
	struct link_device *ld = (struct link_device *)&shmd->ld;
	struct mem_status *mst = msq_get_free_slot(&shmd->stat_list);

	get_shmem_status(shmd, RX, mst);

	if (unlikely(ld->mode == LINK_MODE_OFFLINE)) {
		mif_info("%s: ld->mode == LINK_MODE_OFFLINE\n", ld->name);
		return;
	}

	if (unlikely(!INT_VALID(intr))) {
		mif_info("%s: ERR! invalid intr 0x%X\n", ld->name, intr);
		return;
	}

	mst->int2ap = intr;

	if (ld->mode == LINK_MODE_DLOAD || ld->mode == LINK_MODE_ULOAD)
		udl_handler(shmd, mst);
	else
		ipc_handler(shmd, mst);
}

#if 1
/* Functions for IPC/BOOT/DUMP TX */
#endif

/**
 * write_ipc_to_txq
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @circ: pointer to an instance of circ_status structure
 * @skb: pointer to an instance of sk_buff structure
 *
 * Must be invoked only when there is enough space in the TXQ.
 */
static void write_ipc_to_txq(struct shmem_link_device *shmd, int dev,
			struct circ_status *circ, struct sk_buff *skb)
{
	u32 qsize = circ->qsize;
	u32 in = circ->in;
	u8 *buff = circ->buff;
	u8 *src = skb->data;
	u32 len = skb->len;
#ifdef DEBUG_MODEM_IF
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = shmd->ld.mc;
	char tag[MIF_MAX_STR_LEN];

	snprintf(tag, MIF_MAX_STR_LEN, "LNK: %s->%s", iod->name, mc->name);

	if (dev == IPC_FMT)
		pr_ipc(1, tag, src, (len > 20 ? 20 : len));
#if 0
	if (dev == IPC_RAW)
		pr_ipc(0, tag, src, (len > 20 ? 20 : len));
#endif
#endif

	/* Write data to the TXQ */
	circ_write(buff, src, qsize, in, len);

	/* Update new head (in) pointer */
	set_txq_head(shmd, dev, circ_new_pointer(qsize, in, len));
}

/**
 * xmit_ipc_msg
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * Tries to transmit IPC messages in the skb_txq of @dev as many as possible.
 *
 * Returns total length of IPC messages transmitted or an error code.
 */
static int xmit_ipc_msg(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	unsigned long flags;
	struct circ_status circ;
	int space;
	int copied = 0;

	/* Acquire the spin lock for a TXQ */
	spin_lock_irqsave(&shmd->tx_lock[dev], flags);

	while (1) {
		/* Get the size of free space in the TXQ */
		space = get_txq_space(shmd, dev, &circ);
		if (unlikely(space < 0)) {
#ifdef DEBUG_MODEM_IF
			/* Trigger a enforced CP crash */
			trigger_forced_cp_crash(shmd);
#endif
			/* Empty out the TXQ */
			reset_txq_circ(shmd, dev);
			copied = -EIO;
			break;
		}

		skb = skb_dequeue(txq);
		if (unlikely(!skb))
			break;

		/* Check the free space size comparing with skb->len */
		if (unlikely(space < skb->len)) {
#ifdef DEBUG_MODEM_IF
			struct mem_status mst;
#endif
			/* Set res_required flag for the "dev" */
			atomic_set(&shmd->res_required[dev], 1);

			/* Take the skb back to the skb_txq */
			skb_queue_head(txq, skb);

			mif_err("%s: <by %pf> NOSPC in %s_TXQ"
				"{qsize:%u in:%u out:%u} free:%u < len:%u\n",
				ld->name, CALLER, get_dev_name(dev),
				circ.qsize, circ.in, circ.out, space, skb->len);
#ifdef DEBUG_MODEM_IF
			get_shmem_status(shmd, TX, &mst);
			print_circ_status(ld, dev, &mst);
#endif
			copied = -ENOSPC;
			break;
		}

#ifdef DEBUG_MODEM_IF
		if (!ipc_active(shmd)) {
			if (get_magic(shmd) == SHM_PM_MAGIC) {
				mif_err("%s: Going to SLEEP\n", ld->name);
				copied = -EBUSY;
			} else {
				mif_err("%s: IPC is NOT active\n", ld->name);
				copied = -EIO;
			}
			break;
		}
#endif

		/* TX only when there is enough space in the TXQ */
		write_ipc_to_txq(shmd, dev, &circ, skb);
		copied += skb->len;
		dev_kfree_skb_any(skb);
	}

	/* Release the spin lock */
	spin_unlock_irqrestore(&shmd->tx_lock[dev], flags);

	return copied;
}

/**
 * wait_for_res_ack
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * 1) Sends an REQ_ACK interrupt for @dev to CP.
 * 2) Waits for the corresponding RES_ACK for @dev from CP.
 *
 * Returns the return value from wait_for_completion_interruptible_timeout().
 */
static int wait_for_res_ack(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	struct completion *cmpl = &shmd->req_ack_cmpl[dev];
	unsigned long timeout = msecs_to_jiffies(RES_ACK_WAIT_TIMEOUT);
	int ret;
	u16 mask;

#ifdef DEBUG_MODEM_IF
	mif_info("%s: send %s REQ_ACK\n", ld->name, get_dev_name(dev));
#endif

	mask = get_mask_req_ack(shmd, dev);
	send_int2cp(shmd, INT_NON_CMD(mask));

	/* ret < 0 if interrupted, ret == 0 on timeout */
	ret = wait_for_completion_interruptible_timeout(cmpl, timeout);
	if (ret < 0) {
		mif_err("%s: %s: wait_for_completion interrupted! (ret %d)\n",
			ld->name, get_dev_name(dev), ret);
		goto exit;
	}

	if (ret == 0) {
		struct mem_status mst;
		get_shmem_status(shmd, TX, &mst);

		mif_err("%s: wait_for_completion TIMEOUT! (no %s_RES_ACK)\n",
			ld->name, get_dev_name(dev));

		/*
		** The TXQ must be checked whether or not it is empty, because
		** an interrupt mask can be overwritten by the next interrupt.
		*/
		if (mst.head[dev][TX] == mst.tail[dev][TX]) {
			ret = get_txq_buff_size(shmd, dev);
#ifdef DEBUG_MODEM_IF
			mif_err("%s: %s_TXQ has been emptied\n",
				ld->name, get_dev_name(dev));
			print_circ_status(ld, dev, &mst);
#endif
		}
	}

exit:
	return ret;
}

/**
 * process_res_ack
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 *
 * 1) Tries to transmit IPC messages in the skb_txq with xmit_ipc_msg().
 * 2) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 3) Restarts SHMEM flow control if xmit_ipc_msg() returns -ENOSPC.
 *
 * Returns the return value from xmit_ipc_msg().
 */
static int process_res_ack(struct shmem_link_device *shmd, int dev)
{
	int ret;
	u16 mask;

	ret = xmit_ipc_msg(shmd, dev);
	if (ret > 0) {
		mask = get_mask_send(shmd, dev);
		send_int2cp(shmd, INT_NON_CMD(mask));
		get_shmem_status(shmd, TX, msq_get_free_slot(&shmd->stat_list));
	}

	if (ret >= 0)
		atomic_set(&shmd->res_required[dev], 0);

	return ret;
}

/**
 * fmt_tx_work: performs TX for FMT IPC device under SHMEM flow control
 * @ws: pointer to an instance of the work_struct structure
 *
 * 1) Starts waiting for RES_ACK of FMT IPC device.
 * 2) Returns immediately if the wait is interrupted.
 * 3) Restarts SHMEM flow control if there is a timeout from the wait.
 * 4) Otherwise, it performs processing RES_ACK for FMT IPC device.
 */
static void fmt_tx_work(struct work_struct *ws)
{
	struct link_device *ld;
	struct shmem_link_device *shmd;
	int ret;

	ld = container_of(ws, struct link_device, fmt_tx_dwork.work);
	shmd = to_shmem_link_device(ld);

	ret = wait_for_res_ack(shmd, IPC_FMT);
	/* ret < 0 if interrupted */
	if (ret < 0)
		return;

	/* ret == 0 on timeout */
	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_FMT], 0);
		return;
	}

	ret = process_res_ack(shmd, IPC_FMT);
	if (ret >= 0) {
		permit_cp_sleep(shmd);
		return;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC || ret == -EBUSY) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_FMT],
				   msecs_to_jiffies(1));
	}
}

/**
 * raw_tx_work: performs TX for RAW IPC device under SHMEM flow control.
 * @ws: pointer to an instance of the work_struct structure
 *
 * 1) Starts waiting for RES_ACK of RAW IPC device.
 * 2) Returns immediately if the wait is interrupted.
 * 3) Restarts SHMEM flow control if there is a timeout from the wait.
 * 4) Otherwise, it performs processing RES_ACK for RAW IPC device.
 */
static void raw_tx_work(struct work_struct *ws)
{
	struct link_device *ld;
	struct shmem_link_device *shmd;
	int ret;

	ld = container_of(ws, struct link_device, raw_tx_dwork.work);
	shmd = to_shmem_link_device(ld);

	ret = wait_for_res_ack(shmd, IPC_RAW);
	/* ret < 0 if interrupted */
	if (ret < 0)
		return;

	/* ret == 0 on timeout */
	if (ret == 0) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RAW], 0);
		return;
	}

	ret = process_res_ack(shmd, IPC_RAW);
	if (ret >= 0) {
		permit_cp_sleep(shmd);
		mif_netif_wake(ld);
		return;
	}

	/* At this point, ret < 0 */
	if (ret == -ENOSPC || ret == -EBUSY) {
		queue_delayed_work(ld->tx_wq, ld->tx_dwork[IPC_RAW],
				   msecs_to_jiffies(1));
	}
}

/**
 * c2c_send_ipc
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @skb: pointer to an skb that will be transmitted
 *
 * 1) Tries to transmit IPC messages in the skb_txq with xmit_ipc_msg().
 * 2) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 3) Starts SHMEM flow control if xmit_ipc_msg() returns -ENOSPC.
 */
static int c2c_send_ipc(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	int ret;
	u16 mask;

	if (atomic_read(&shmd->res_required[dev]) > 0) {
		mif_info("%s: %s_TXQ is full\n", ld->name, get_dev_name(dev));
		return 0;
	}

	ret = xmit_ipc_msg(shmd, dev);
	if (likely(ret > 0)) {
		mask = get_mask_send(shmd, dev);
		send_int2cp(shmd, INT_NON_CMD(mask));
		get_shmem_status(shmd, TX, msq_get_free_slot(&shmd->stat_list));
		goto exit;
	}

	/* If there was no TX, just exit */
	if (ret == 0)
		goto exit;

	/* At this point, ret < 0 */
	if (ret == -ENOSPC || ret == -EBUSY) {
		/* Prohibit CP from sleeping until the TXQ buffer is empty */
		if (forbid_cp_sleep(shmd) < 0) {
			trigger_forced_cp_crash(shmd);
			goto exit;
		}

		/*----------------------------------------------------*/
		/* shmd->res_required[dev] was set in xmit_ipc_msg(). */
		/*----------------------------------------------------*/

		if (dev == IPC_RAW)
			mif_netif_stop(ld);

		queue_delayed_work(ld->tx_wq, ld->tx_dwork[dev],
				   msecs_to_jiffies(1));
	}

exit:
	return ret;
}

/**
 * c2c_try_send_ipc
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @iod: pointer to an instance of the io_device structure
 * @skb: pointer to an skb that will be transmitted
 *
 * 1) Enqueues an skb to the skb_txq for @dev in the link device instance.
 * 2) Tries to transmit IPC messages with c2c_send_ipc().
 */
static void c2c_try_send_ipc(struct shmem_link_device *shmd, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct link_device *ld = &shmd->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	int ret;

	if (forbid_cp_sleep(shmd) < 0) {
		trigger_forced_cp_crash(shmd);
		goto exit;
	}

	if (unlikely(txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err("%s: %s txq->qlen %d >= %d\n", ld->name,
			get_dev_name(dev), txq->qlen, MAX_SKB_TXQ_DEPTH);
		dev_kfree_skb_any(skb);
		goto exit;
	}

	skb_queue_tail(txq, skb);

	ret = c2c_send_ipc(shmd, dev);
	if (ret < 0) {
		mif_err("%s->%s: ERR! c2c_send_ipc fail (err %d)\n",
			iod->name, ld->name, ret);
	}

exit:
	permit_cp_sleep(shmd);
}

static int c2c_send_udl_cmd(struct shmem_link_device *shmd, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct link_device *ld = &shmd->ld;
	u8 *buff;
	u8 *src;
	u32 qsize;
	u32 in;
	int space;
	int tx_bytes;
	struct circ_status circ;

	if (iod->format == IPC_BOOT) {
		pr_ipc(0, "[AP->CP] DL CMD", skb->data,
			(skb->len > 20 ? 20 : skb->len));
	} else {
		pr_ipc(0, "[AP->CP] UL CMD", skb->data,
			(skb->len > 20 ? 20 : skb->len));
	}

	/* Get the size of free space in the TXQ */
	space = get_txq_space(shmd, dev, &circ);
	if (space < 0) {
		reset_txq_circ(shmd, dev);
		tx_bytes = -EIO;
		goto exit;
	}

	/* Get the size of data to be sent */
	tx_bytes = skb->len;

	/* Check the size of free space */
	if (space < tx_bytes) {
		mif_err("%s: NOSPC in %s_TXQ {qsize:%u in:%u out:%u}, "
			"free:%u < tx_bytes:%u\n", ld->name, get_dev_name(dev),
			circ.qsize, circ.in, circ.out, space, tx_bytes);
		tx_bytes = -ENOSPC;
		goto exit;
	}

	/* Write data to the TXQ */
	buff = circ.buff;
	src = skb->data;
	qsize = circ.qsize;
	in = circ.in;
	circ_write(buff, src, qsize, in, tx_bytes);

	/* Update new head (in) pointer */
	set_txq_head(shmd, dev, circ_new_pointer(qsize, circ.in, tx_bytes));

exit:
	dev_kfree_skb_any(skb);
	return tx_bytes;
}

static int c2c_send_udl_data(struct shmem_link_device *shmd, int dev)
{
	struct link_device *ld = &shmd->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct sk_buff *skb;
	u8 *src;
	int tx_bytes;
	int copied;
	u8 *buff;
	u32 qsize;
	u32 in;
	u32 out;
	int space;
	struct circ_status circ;

	/* Get the size of free space in the TXQ */
	space = get_txq_space(shmd, dev, &circ);
	if (space < 0) {
#ifdef DEBUG_MODEM_IF
		/* Trigger a enforced CP crash */
		trigger_forced_cp_crash(shmd);
#endif
		/* Empty out the TXQ */
		reset_txq_circ(shmd, dev);
		return -EFAULT;
	}

	buff = circ.buff;
	qsize = circ.qsize;
	in = circ.in;
	out = circ.out;
	space = circ.size;

	copied = 0;
	while (1) {
		skb = skb_dequeue(txq);
		if (!skb)
			break;

		/* Get the size of data to be sent */
		src = skb->data;
		tx_bytes = skb->len;

		/* Check the free space size comparing with skb->len */
		if (space < tx_bytes) {
			/* Set res_required flag for the "dev" */
			atomic_set(&shmd->res_required[dev], 1);

			/* Take the skb back to the skb_txq */
			skb_queue_head(txq, skb);

			mif_info("NOSPC in RAW_TXQ {qsize:%u in:%u out:%u}, "
				"space:%u < tx_bytes:%u\n",
				qsize, in, out, space, tx_bytes);
			break;
		}

		/*
		** TX only when there is enough space in the TXQ
		*/
		circ_write(buff, src, qsize, in, tx_bytes);

		copied += tx_bytes;
		in = circ_new_pointer(qsize, in, tx_bytes);
		space -= tx_bytes;

		dev_kfree_skb_any(skb);
	}

	/* Update new head (in) pointer */
	if (copied > 0) {
		in = circ_new_pointer(qsize, circ.in, copied);
		set_txq_head(shmd, dev, in);
	}

	return copied;
}

/**
 * c2c_send_udl
 * @shmd: pointer to an instance of shmem_link_device structure
 * @dev: IPC device (IPC_FMT, IPC_RAW, etc.)
 * @iod: pointer to an instance of the io_device structure
 * @skb: pointer to an skb that will be transmitted
 *
 * 1) Enqueues an skb to the skb_txq for @dev in the link device instance.
 * 2) Tries to transmit IPC messages in the skb_txq by invoking xmit_ipc_msg()
 *    function.
 * 3) Sends an interrupt to CP if there is no error from xmit_ipc_msg().
 * 4) Starts SHMEM flow control if xmit_ipc_msg() returns -ENOSPC.
 */
static void c2c_send_udl(struct shmem_link_device *shmd, int dev,
			struct io_device *iod, struct sk_buff *skb)
{
	struct link_device *ld = &shmd->ld;
	struct sk_buff_head *txq = ld->skb_txq[dev];
	struct completion *cmpl = &shmd->req_ack_cmpl[dev];
	struct std_dload_info *dl_info = &shmd->dl_info;
	struct mem_status mst;
	u32 timeout = msecs_to_jiffies(RES_ACK_WAIT_TIMEOUT);
	u32 udl_cmd;
	int ret;
	u16 mask = get_mask_send(shmd, dev);

	udl_cmd = std_udl_get_cmd(skb->data);
	if (iod->format == IPC_RAMDUMP || !std_udl_with_payload(udl_cmd)) {
		ret = c2c_send_udl_cmd(shmd, dev, iod, skb);
		if (ret > 0)
			send_int2cp(shmd, INT_NON_CMD(mask));
		else
			mif_err("ERR! c2c_send_udl_cmd fail (err %d)\n", ret);
		goto exit;
	}

	skb_queue_tail(txq, skb);
	if (txq->qlen < dl_info->num_frames)
		goto exit;

	mask |= get_mask_req_ack(shmd, dev);
	while (1) {
		ret = c2c_send_udl_data(shmd, dev);
		if (ret < 0) {
			mif_err("ERR! c2c_send_udl_data fail (err %d)\n", ret);
			skb_queue_purge(txq);
			break;
		}

		if (skb_queue_empty(txq)) {
			send_int2cp(shmd, INT_NON_CMD(mask));
			break;
		}

		send_int2cp(shmd, INT_NON_CMD(mask));

		do {
			ret = wait_for_completion_timeout(cmpl, timeout);
			get_shmem_status(shmd, TX, &mst);
		} while (mst.head[dev][TX] != mst.tail[dev][TX]);
	}

exit:
	return;
}

/**
 * c2c_send
 * @ld: pointer to an instance of the link_device structure
 * @iod: pointer to an instance of the io_device structure
 * @skb: pointer to an skb that will be transmitted
 *
 * Returns the length of data transmitted or an error code.
 *
 * Normal call flow for an IPC message:
 *   c2c_try_send_ipc -> c2c_send_ipc -> xmit_ipc_msg -> write_ipc_to_txq
 *
 * Call flow on congestion in a IPC TXQ:
 *   c2c_try_send_ipc -> c2c_send_ipc -> xmit_ipc_msg ,,, queue_delayed_work
 *   => xxx_tx_work -> wait_for_res_ack
 *   => msg_handler
 *   => process_res_ack -> xmit_ipc_msg (,,, queue_delayed_work ...)
 */
static int c2c_send(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	int dev = iod->format;
	int len = skb->len;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
		if (likely(ld->mode == LINK_MODE_IPC)) {
			c2c_try_send_ipc(shmd, dev, iod, skb);
		} else {
			mif_err("%s->%s: ERR! ld->mode != LINK_MODE_IPC\n",
				iod->name, ld->name);
			dev_kfree_skb_any(skb);
		}
		return len;

	case IPC_BOOT:
	case IPC_RAMDUMP:
		c2c_send_udl(shmd, IPC_RAW, iod, skb);
		return len;

	default:
		mif_err("%s: ERR! no TXQ for %s\n", ld->name, iod->name);
		dev_kfree_skb_any(skb);
		return -ENODEV;
	}
}

#if 1
/* Functions for BOOT/DUMP and INIT */
#endif

static int c2c_dload_start(struct link_device *ld, struct io_device *iod)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	u32 magic;

	ld->mode = LINK_MODE_DLOAD;

	clear_shmem_map(shmd);

	set_magic(shmd, SHM_BOOT_MAGIC);
	magic = get_magic(shmd);
	if (magic != SHM_BOOT_MAGIC) {
		mif_err("%s: ERR! magic 0x%08X != SHM_BOOT_MAGIC 0x%08X\n",
			ld->name, magic, SHM_BOOT_MAGIC);
		return -EFAULT;
	}

	return 0;
}

/**
 * c2c_set_dload_info
 * @ld: pointer to an instance of link_device structure
 * @iod: pointer to an instance of io_device structure
 * @arg: pointer to an instance of std_dload_info structure in "user" memory
 *
 */
static int c2c_set_dload_info(struct link_device *ld, struct io_device *iod,
			unsigned long arg)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	struct std_dload_info *dl_info = &shmd->dl_info;
	int ret;

	ret = copy_from_user(dl_info, (void __user *)arg,
			sizeof(struct std_dload_info));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	return 0;
}

static int c2c_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	mif_err("+++\n");
	trigger_forced_cp_crash(shmd);
	mif_err("---\n");
	return 0;
}

static int c2c_dump_start(struct link_device *ld, struct io_device *iod)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);

	ld->mode = LINK_MODE_ULOAD;

	clear_shmem_map(shmd);

	mif_err("%s: magic = 0x%08X\n", ld->name, SHM_DUMP_MAGIC);
	set_magic(shmd, SHM_DUMP_MAGIC);

	return 0;
}

static void c2c_remap_4mb_ipc_region(struct shmem_link_device *shmd)
{
	struct shmem_4mb_phys_map *map;
	struct shmem_ipc_device *dev;

	map = (struct shmem_4mb_phys_map *)shmd->base;

	/* Magic code and access enable fields */
	shmd->ipc_map.magic = (u32 __iomem *)&map->magic;
	shmd->ipc_map.access = (u32 __iomem *)&map->access;

	/* FMT */
	dev = &shmd->ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u32 __iomem *)&map->fmt_tx_head;
	dev->txq.tail = (u32 __iomem *)&map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&map->fmt_tx_buff[0];
	dev->txq.size = SHM_4M_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u32 __iomem *)&map->fmt_rx_head;
	dev->rxq.tail = (u32 __iomem *)&map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&map->fmt_rx_buff[0];
	dev->rxq.size = SHM_4M_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send    = INT_MASK_SEND_F;

	/* RAW */
	dev = &shmd->ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u32 __iomem *)&map->raw_tx_head;
	dev->txq.tail = (u32 __iomem *)&map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&map->raw_tx_buff[0];
	dev->txq.size = SHM_4M_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u32 __iomem *)&map->raw_rx_head;
	dev->rxq.tail = (u32 __iomem *)&map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&map->raw_rx_buff[0];
	dev->rxq.size = SHM_4M_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send    = INT_MASK_SEND_R;

	/* interrupt ports */
	shmd->ipc_map.mbx2ap = NULL;
	shmd->ipc_map.mbx2cp = NULL;
}

static int c2c_init_ipc_map(struct shmem_link_device *shmd)
{
	if (shmd->size >= SHMEM_SIZE_4MB)
		c2c_remap_4mb_ipc_region(shmd);
	else
		return -EINVAL;

	memset(shmd->base, 0, shmd->size);

	shmd->magic = shmd->ipc_map.magic;
	shmd->access = shmd->ipc_map.access;

	shmd->dev[IPC_FMT] = &shmd->ipc_map.dev[IPC_FMT];
	shmd->dev[IPC_RAW] = &shmd->ipc_map.dev[IPC_RAW];

	shmd->mbx2ap = shmd->ipc_map.mbx2ap;
	shmd->mbx2cp = shmd->ipc_map.mbx2cp;

	return 0;
}
static int c2c_init_communication(struct link_device *ld, struct io_device *iod)
{
	struct shmem_link_device *shmd = to_shmem_link_device(ld);
	struct io_device *check_iod;

	if (iod->format == IPC_BOOT)
		return 0;

	/* send 0xC2 */
	switch(iod->format) {
	case IPC_FMT:
		check_iod = link_get_iod_with_format(ld, IPC_RFS);
		if (check_iod ? atomic_read(&check_iod->opened) : true) {
			mif_err("%s: Send 0xC2 (INIT_END)\n", ld->name);
			send_int2cp(shmd, INT_CMD(INT_CMD_INIT_END));
		} else
			mif_err("%s defined but not opened\n", check_iod->name);
		break;
	case IPC_RFS:
		check_iod = link_get_iod_with_format(ld, IPC_FMT);
		if (check_iod && atomic_read(&check_iod->opened)) {
			mif_err("%s: Send 0xC2 (INIT_END)\n", ld->name);
			send_int2cp(shmd, INT_CMD(INT_CMD_INIT_END));
		} else
			mif_err("not opened\n");
		break;
	default:
		break;
	}
	return 0;
}

#if 0
static void c2c_link_terminate(struct link_device *ld, struct io_device *iod)
{
	if (iod->format == IPC_FMT && ld->mode == LINK_MODE_IPC) {
		if (!atomic_read(&iod->opened)) {
			ld->mode = LINK_MODE_OFFLINE;
			mif_err("%s: %s: link mode changed: IPC -> OFFLINE\n",
				iod->name, ld->name);
		}
	}

	return;
}
#endif

struct link_device *c2c_create_link_device(struct platform_device *pdev)
{
	struct modem_data *modem;
	struct shmem_link_device *shmd;
	struct link_device *ld;
	int err;
	int i;
	u32 phys_base;
	u32 offset;
	u32 size;
	int irq;
	unsigned long irq_flags;
	char name[MIF_MAX_NAME_LEN];

	/*
	** Get the modem (platform) data
	*/
	modem = (struct modem_data *)pdev->dev.platform_data;
	if (!modem) {
		mif_err("ERR! modem == NULL\n");
		return NULL;
	}
	mif_err("%s: %s\n", modem->link_name, modem->name);

	if (modem->ipc_version < SIPC_VER_50) {
		mif_err("%s: %s: ERR! IPC version %d < SIPC_VER_50\n",
			modem->link_name, modem->name, modem->ipc_version);
		return NULL;
	}

	/*
	** Alloc an instance of shmem_link_device structure
	*/
	shmd = kzalloc(sizeof(struct shmem_link_device), GFP_KERNEL);
	if (!shmd) {
		mif_err("%s: ERR! shmd kzalloc fail\n", modem->link_name);
		goto error;
	}
	ld = &shmd->ld;

	/*
	** Retrieve modem data and SHMEM control data from the modem data
	*/
	ld->mdm_data = modem;
	ld->name = modem->link_name;
	ld->aligned = 1;
	ld->ipc_version = modem->ipc_version;
	ld->max_ipc_dev = MAX_SIPC5_DEV;

	/*
	** Set attributes as a link device
	*/
	ld->init_comm = c2c_init_communication;
#if 0
	ld->terminate_comm = c2c_link_terminate;
#endif
	ld->send = c2c_send;
	ld->dload_start = c2c_dload_start;
	ld->firm_update = c2c_set_dload_info;
	ld->force_dump = c2c_force_dump;
	ld->dump_start = c2c_dump_start;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;

	skb_queue_head_init(&ld->sk_fmt_rx_q);
	skb_queue_head_init(&ld->sk_raw_rx_q);
	ld->skb_rxq[IPC_FMT] = &ld->sk_fmt_rx_q;
	ld->skb_rxq[IPC_RAW] = &ld->sk_raw_rx_q;

	init_completion(&ld->init_cmpl);

	/*
	** Retrieve SHMEM resource
	*/
	if (modem->link_types & LINKTYPE(LINKDEV_C2C)) {
		shmd->type = C2C_SHMEM;
		mif_debug("%s: shmd->type = C2C_SHMEM\n", ld->name);
	} else if (modem->link_types & LINKTYPE(LINKDEV_SHMEM)) {
		shmd->type = REAL_SHMEM;
		mif_debug("%s: shmd->type = REAL_SHMEM\n", ld->name);
	} else {
		mif_err("%s: ERR! invalid type\n", ld->name);
		goto error;
	}

	phys_base = c2c_get_phys_base();
	offset = c2c_get_sh_rgn_offset();
	size = c2c_get_sh_rgn_size();
	mif_debug("%s: phys_base:0x%08X offset:0x%08X size:%d\n",
		ld->name, phys_base, offset, size);

	shmd->start = phys_base + offset;
	shmd->size = size;
	shmd->base = c2c_request_sh_region(shmd->start, shmd->size);
	if (!shmd->base) {
		mif_err("%s: ERR! c2c_request_sh_region fail\n", ld->name);
		goto error;
	}

	mif_debug("%s: phys_addr:0x%08X virt_addr:0x%08X size:%d\n",
		ld->name, shmd->start, (int)shmd->base, shmd->size);

	/*
	** Initialize SHMEM maps (physical map -> logical map)
	*/
	err = c2c_init_ipc_map(shmd);
	if (err < 0) {
		mif_err("%s: ERR! c2c_init_ipc_map fail (err %d)\n",
			ld->name, err);
		goto error;
	}

	/*
	** Initialize locks, completions, and bottom halves
	*/
	sprintf(shmd->wlock_name, "%s_wlock", ld->name);
	wake_lock_init(&shmd->wlock, WAKE_LOCK_SUSPEND, shmd->wlock_name);

	sprintf(shmd->ap_wlock_name, "%s_ap_wlock", ld->name);
	wake_lock_init(&shmd->ap_wlock, WAKE_LOCK_SUSPEND, shmd->ap_wlock_name);

	sprintf(shmd->cp_wlock_name, "%s_cp_wlock", ld->name);
	wake_lock_init(&shmd->cp_wlock, WAKE_LOCK_SUSPEND, shmd->cp_wlock_name);

	init_completion(&shmd->udl_cmpl);
	for (i = 0; i < MAX_SIPC5_DEV; i++)
		init_completion(&shmd->req_ack_cmpl[i]);

	tasklet_init(&shmd->rx_tsk, msg_rx_task, (unsigned long)shmd);
	INIT_DELAYED_WORK(&shmd->ipc_rx_dwork, ipc_rx_work);
	INIT_DELAYED_WORK(&shmd->udl_rx_dwork, udl_rx_work);

	for (i = 0; i < MAX_SIPC5_DEV; i++) {
		spin_lock_init(&shmd->tx_lock[i]);
		atomic_set(&shmd->res_required[i], 0);
	}

	ld->tx_wq = create_singlethread_workqueue("shmem_tx_wq");
	if (!ld->tx_wq) {
		mif_err("%s: ERR! fail to create tx_wq\n", ld->name);
		goto error;
	}
	INIT_DELAYED_WORK(&ld->fmt_tx_dwork, fmt_tx_work);
	INIT_DELAYED_WORK(&ld->raw_tx_dwork, raw_tx_work);
	ld->tx_dwork[IPC_FMT] = &ld->fmt_tx_dwork;
	ld->tx_dwork[IPC_RAW] = &ld->raw_tx_dwork;

	spin_lock_init(&shmd->stat_list.lock);
	spin_lock_init(&shmd->trace_list.lock);
#ifdef DEBUG_MODEM_IF
	INIT_DELAYED_WORK(&shmd->dump_dwork, mem_dump_work);
#endif

	INIT_DELAYED_WORK(&shmd->cp_sleep_dwork, release_cp_wakeup);
	INIT_DELAYED_WORK(&shmd->link_off_dwork, release_ap_status);
	spin_lock_init(&shmd->pm_lock);

	/*
	** Retrieve SHMEM IRQ GPIO#, IRQ#, and IRQ flags
	*/
	shmd->gpio_pda_active = modem->gpio_pda_active;
	mif_err("PDA_ACTIVE gpio# = %d (value %d)\n",
		shmd->gpio_pda_active, gpio_get_value(shmd->gpio_pda_active));

	shmd->gpio_ap_status = modem->gpio_ap_status;
	shmd->gpio_ap_wakeup = modem->gpio_ap_wakeup;
	shmd->irq_ap_wakeup = modem->irq_ap_wakeup;
	if (!shmd->irq_ap_wakeup) {
		mif_err("ERR! no irq_ap_wakeup\n");
		goto error;
	}
	mif_debug("CP2AP_WAKEUP IRQ# = %d\n", shmd->irq_ap_wakeup);

	shmd->gpio_cp_wakeup = modem->gpio_cp_wakeup;
	shmd->gpio_cp_status = modem->gpio_cp_status;
	shmd->irq_cp_status = modem->irq_cp_status;
	if (!shmd->irq_cp_status) {
		mif_err("ERR! no irq_cp_status\n");
		goto error;
	}
	mif_debug("CP2AP_STATUS IRQ# = %d\n", shmd->irq_cp_status);

	c2c_assign_gpio_ap_wakeup(shmd->gpio_ap_wakeup);
	c2c_assign_gpio_ap_status(shmd->gpio_ap_status);
	c2c_assign_gpio_cp_wakeup(shmd->gpio_cp_wakeup);
	c2c_assign_gpio_cp_status(shmd->gpio_cp_status);

	gpio_set_value(shmd->gpio_pda_active, 1);
	gpio_set_value(shmd->gpio_ap_status, 1);

	/*
	** Register interrupt handlers
	*/
	err = c2c_register_handler(c2c_irq_handler, shmd);
	if (err) {
		mif_err("%s: ERR! c2c_register_handler fail (err %d)\n",
			ld->name, err);
		goto error;
	}

	snprintf(name, MIF_MAX_NAME_LEN, "%s_ap_wakeup", ld->name);
	irq = shmd->irq_ap_wakeup;
	irq_flags = (IRQF_NO_THREAD | IRQF_NO_SUSPEND | IRQF_TRIGGER_HIGH);
	err = mif_register_isr(irq, ap_wakeup_handler, irq_flags, name, shmd);
	if (err)
		goto error;

	snprintf(name, MIF_MAX_NAME_LEN, "%s_cp_status", ld->name);
	irq = shmd->irq_cp_status;
	irq_flags = (IRQF_NO_THREAD | IRQF_NO_SUSPEND | IRQF_TRIGGER_HIGH);
	err = mif_register_isr(irq, cp_status_handler, irq_flags, name, shmd);
	if (err)
		goto error;

	return ld;

error:
	mif_err("xxx\n");
	kfree(shmd);
	return NULL;
}

