/*
 * SAMSUNG S5P USB HOST EHCI Controller
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Jingoo Han <jg1.han@samsung.com>
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <plat/cpu.h>
#include <plat/ehci.h>
#include <plat/usb-phy.h>

#include <mach/regs-pmu.h>
#include <mach/regs-usb-host.h>
#include <mach/board_rev.h>

#ifdef CONFIG_MDM_HSIC_PM
#include <linux/mdm_hsic_pm.h>
static const char hsic_pm_dev[] = "mdm_hsic_pm0";
#endif

#if defined(CONFIG_EHCI_IRQ_DISTRIBUTION)
#include <linux/cpu.h>
#endif
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
#include <mach/sec_modem.h>
#endif

struct s5p_ehci_hcd {
	struct device *dev;
	struct usb_hcd *hcd;
	struct clk *clk;
	int power_on;
};

#ifdef CONFIG_USB_EXYNOS_SWITCH
int s5p_ehci_port_power_off(struct platform_device *pdev)
{
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	(void) ehci_hub_control(hcd,
			ClearPortFeature,
			USB_PORT_FEAT_POWER,
			1, NULL, 0);
	/* Flush those writes */
	ehci_readl(ehci, &ehci->regs->command);
	return 0;
}
EXPORT_SYMBOL_GPL(s5p_ehci_port_power_off);

int s5p_ehci_port_power_on(struct platform_device *pdev)
{
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	(void) ehci_hub_control(hcd,
			SetPortFeature,
			USB_PORT_FEAT_POWER,
			1, NULL, 0);
	/* Flush those writes */
	ehci_readl(ehci, &ehci->regs->command);
	return 0;
}
EXPORT_SYMBOL_GPL(s5p_ehci_port_power_on);
#endif

static int s5p_ehci_configurate(struct usb_hcd *hcd)
{
	int delay_count = 0;

	/* This is for waiting phy before ehci configuration */
	do {
		if (readl(hcd->regs))
			break;
		udelay(1);
		++delay_count;
	} while (delay_count < 200);
	if (delay_count)
		dev_info(hcd->self.controller, "phy delay count = %d\n",
			delay_count);

	/* DMA burst Enable, set utmi suspend_on_n */
#ifdef CONFIG_USB_OHCI_S5P
	writel(readl(INSNREG00(hcd->regs)) | ENA_DMA_INCR,
#else
	writel(readl(INSNREG00(hcd->regs)) | ENA_DMA_INCR,
#endif
		INSNREG00(hcd->regs));
	return 0;
}

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB) ||\
	defined(CONFIG_CDMA_MODEM_MDM6600) || defined(CONFIG_MDM_HSIC_PM)
#ifdef CONFIG_MACH_P8LTE
#define CP_PORT		 1  /* HSIC0 in S5PC210 */
#else
#define CP_PORT      2  /* HSIC0 in S5PC210 */
#endif
#define RETRY_CNT_LIMIT 30  /* Max 300ms wait for cp resume*/

int s5p_ehci_port_control(struct platform_device *pdev, int port, int enable)
{
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	(void) ehci_hub_control(hcd,
			enable ? SetPortFeature : ClearPortFeature,
			USB_PORT_FEAT_POWER,
			port, NULL, 0);
	/* Flush those writes */
	ehci_readl(ehci, &ehci->regs->command);
	return 0;
}
#endif

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB) \
		|| defined(CONFIG_MDM_HSIC_PM)
static void s5p_wait_for_cp_resume(struct platform_device *pdev,
	struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	u32 __iomem	*portsc ;
	u32 val32, retry_cnt = 0;

#if !defined(CONFIG_MDM_HSIC_PM)
	/* when use usb3503 hub, need not wait cp resume */
	if (modem_using_hub())
		return;
#endif
	portsc = &ehci->regs->port_status[CP_PORT-1];

#if !defined(CONFIG_MDM_HSIC_PM)
	if (pdata && pdata->noti_host_states)
		pdata->noti_host_states(pdev, S5P_HOST_ON);
#endif
	do {
		msleep(10);
		val32 = ehci_readl(ehci, portsc);
	} while (++retry_cnt < RETRY_CNT_LIMIT && !(val32 & PORT_CONNECT));

	if (retry_cnt >= RETRY_CNT_LIMIT)
		dev_info(&pdev->dev, "%s: retry_cnt = %d, portsc = 0x%x\n",
				__func__, retry_cnt, val32);

#if defined(CONFIG_UMTS_MODEM_XMM6262)
	if (pdata->get_cp_active_state && !pdata->get_cp_active_state()) {
		s5p_ehci_port_control(pdev, CP_PORT, 0);
		pr_err("mif: force port%d off by cp reset\n", CP_PORT);
	}
#endif
}
#endif

static void s5p_ehci_phy_init(struct platform_device *pdev)
{
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	u32 delay_count = 0;

	if (pdata->phy_init) {
		pdata->phy_init(pdev, S5P_USB_PHY_HOST);

		while (!readl(hcd->regs) && delay_count < 200) {
			delay_count++;
			udelay(1);
		}
		if (delay_count)
			dev_info(&pdev->dev, "waiting time = %d\n",
				delay_count);
		s5p_ehci_configurate(hcd);
		dev_dbg(&pdev->dev, "%s : 0x%x\n", __func__,
				readl(INSNREG00(hcd->regs)));
	}

}

#ifdef CONFIG_PM
static int s5p_ehci_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc = 0;

#ifdef CONFIG_MDM_HSIC_PM
	/*
	 * check suspend returns 1 if it is possible to suspend
	 * otherwise, it returns 0 impossible or returns some error
	 */
	rc = check_udev_suspend_allowed(hsic_pm_dev);
	if (rc > 0) {
		set_host_stat(hsic_pm_dev, POWER_OFF);
		if (wait_dev_pwr_stat(hsic_pm_dev, POWER_OFF) < 0) {
			set_host_stat(hsic_pm_dev, POWER_ON);
			pm_runtime_resume(&pdev->dev);
			return -EBUSY;
		}
	} else if (rc == -ENODEV) {
		/* no hsic pm driver loaded, proceed suspend */
		pr_debug("%s: suspend without hsic pm\n", __func__);
	} else {
		pm_runtime_resume(&pdev->dev);
		return -EBUSY;
	}
	rc = 0;
#endif

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */

	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED && hcd->state != HC_STATE_HALT) {
		spin_unlock_irqrestore(&ehci->lock, flags);
		return -EINVAL;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	spin_unlock_irqrestore(&ehci->lock, flags);

	if (pdata && pdata->phy_exit)
		pdata->phy_exit(pdev, S5P_USB_PHY_HOST);
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
	if (pdata && pdata->noti_host_states)
		pdata->noti_host_states(pdev, S5P_HOST_OFF);
#endif

	clk_disable(s5p_ehci->clk);

	return rc;
}

static int s5p_ehci_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	clk_enable(s5p_ehci->clk);
	pm_runtime_resume(&pdev->dev);

	s5p_ehci_phy_init(pdev);

	/* if EHCI was off, hcd was removed */
	if (!s5p_ehci->power_on) {
		dev_info(dev, "Nothing to do for the device (power off)\n");
		return 0;
	}

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;
#ifdef CONFIG_MDM_HSIC_PM
	set_host_stat(hsic_pm_dev, POWER_ON);
	wait_dev_pwr_stat(hsic_pm_dev, POWER_ON);
#endif
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB) \
		|| defined(CONFIG_MDM_HSIC_PM)
	s5p_wait_for_cp_resume(pdev, hcd);
#endif
	return 0;
}

#else
#define s5p_ehci_suspend	NULL
#define s5p_ehci_resume		NULL
#endif

#ifdef CONFIG_USB_SUSPEND
static int s5p_ehci_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;

	if (pdata && pdata->phy_suspend)
		pdata->phy_suspend(pdev, S5P_USB_PHY_HOST);
#ifdef CONFIG_MDM_HSIC_PM
	request_active_lock_release(hsic_pm_dev);
#endif
	return 0;
}

static int s5p_ehci_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int rc = 0;

	if (dev->power.is_suspended)
		return 0;

#ifdef CONFIG_MDM_HSIC_PM
	request_active_lock_set(hsic_pm_dev);
#endif
	/* platform device isn't suspended */
	if (pdata && pdata->phy_resume)
		rc = pdata->phy_resume(pdev, S5P_USB_PHY_HOST);

	if (rc) {
		s5p_ehci_configurate(hcd);

		/* emptying the schedule aborts any urbs */
		spin_lock_irq(&ehci->lock);
		if (ehci->reclaim)
			end_unlink_async(ehci);
		ehci_work(ehci);
		spin_unlock_irq(&ehci->lock);

		usb_root_hub_lost_power(hcd->self.root_hub);

		ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
		ehci_writel(ehci, INTR_MASK, &ehci->regs->intr_enable);
		(void)ehci_readl(ehci, &ehci->regs->intr_enable);

		/* here we "know" root ports should always stay powered */
		ehci_port_power(ehci, 1);

		hcd->state = HC_STATE_SUSPENDED;
#ifdef CONFIG_MDM_HSIC_PM
		set_host_stat(hsic_pm_dev, POWER_ON);
		wait_dev_pwr_stat(hsic_pm_dev, POWER_ON);
#endif
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB) \
		|| defined(CONFIG_MDM_HSIC_PM)
		s5p_wait_for_cp_resume(pdev, hcd);
#endif
	}

	return 0;
}
#else
#define s5p_ehci_runtime_suspend	NULL
#define s5p_ehci_runtime_resume		NULL
#endif

static const struct hc_driver s5p_ehci_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "S5P EHCI Host Controller",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	.get_frame_number	= ehci_get_frame,

	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,

	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static ssize_t show_ehci_power(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);

	return sprintf(buf, "EHCI Power %s\n", (s5p_ehci->power_on) ? "on" : "off");
}

static ssize_t store_ehci_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;
	int power_on;
	int irq;
	int retval;

	if (sscanf(buf, "%d", &power_on) != 1)
		return -EINVAL;

	device_lock(dev);

	if (!power_on && s5p_ehci->power_on) {
		printk(KERN_DEBUG "%s: EHCI turns off\n", __func__);
		pm_runtime_forbid(dev);
		s5p_ehci->power_on = 0;
		usb_remove_hcd(hcd);

		if (pdata && pdata->phy_exit)
			pdata->phy_exit(pdev, S5P_USB_PHY_HOST);

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
		/*HSIC IPC control the ACTIVE_STATE*/
		if (pdata && pdata->noti_host_states)
			pdata->noti_host_states(pdev, S5P_HOST_OFF);
#endif
	} else if (power_on) {
		printk(KERN_DEBUG "%s: EHCI turns on\n", __func__);
		if (s5p_ehci->power_on) {
			pm_runtime_forbid(dev);
			usb_remove_hcd(hcd);
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
			/*HSIC IPC control the ACTIVE_STATE*/
			if (pdata && pdata->noti_host_states)
				pdata->noti_host_states(pdev, S5P_HOST_OFF);
#endif
		} else
			s5p_ehci_phy_init(pdev);

		irq = platform_get_irq(pdev, 0);
		retval = usb_add_hcd(hcd, irq,
				IRQF_DISABLED | IRQF_SHARED);
		if (retval < 0) {
			dev_err(dev, "Power On Fail\n");
			goto exit;
		}

		s5p_ehci->power_on = 1;
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
		/* Sometimes XMM6262 send remote wakeup when hub enter suspend
		 * So, set the hub waiting 500ms autosuspend delay*/
		if (hcd->self.root_hub)
			pm_runtime_set_autosuspend_delay(
				&hcd->self.root_hub->dev,
				msecs_to_jiffies(500));

		/*HSIC IPC control the ACTIVE_STATE*/
		if (pdata && pdata->noti_host_states)
			pdata->noti_host_states(pdev, S5P_HOST_ON);
#endif
		pm_runtime_allow(dev);
	}
exit:
	device_unlock(dev);
	return count;
}
static DEVICE_ATTR(ehci_power, 0664, show_ehci_power, store_ehci_power);

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
static ssize_t store_port_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;

	int power_on, port;
	int err;

	err = sscanf(buf, "%d %d", &power_on, &port);
	if (err < 2 || port < 0 || port > 3 || power_on < 0 || power_on > 1) {
		pr_err("port power fail: port_power 1 2(port 2 enable 1)\n");
		return count;
	}

	pr_debug("%s: Port:%d power: %d\n", __func__, port, power_on);
	device_lock(dev);
	s5p_ehci_port_control(pdev, port, power_on);

	/*HSIC IPC control the ACTIVE_STATE*/
	if (pdata && pdata->noti_host_states && port == CP_PORT)
		pdata->noti_host_states(pdev, power_on ? S5P_HOST_ON :
			S5P_HOST_OFF);
	device_unlock(dev);
	return count;
}
static DEVICE_ATTR(port_power, 0664, NULL, store_port_power);
#endif

static inline int create_ehci_sys_file(struct ehci_hcd *ehci)
{
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
	BUG_ON(device_create_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_port_power));
#endif
	return device_create_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_power);
}

static inline void remove_ehci_sys_file(struct ehci_hcd *ehci)
{
	device_remove_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_ehci_power);
#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
	device_remove_file(ehci_to_hcd(ehci)->self.controller,
			&dev_attr_port_power);
#endif
}

#if defined(CONFIG_EHCI_IRQ_DISTRIBUTION)
static int s5p_ehci_irq_no = 0;
static int s5p_ehci_irq_cpu = 0;

/* total cpu core numbers to irq cpu (cpu0 is default)
 * 1 (single): cpu0
 * 2 (dual)  : cpu1
 * 3         : cpu1
 * 4 (quad)  : cpu3
 */
static int s5p_ehci_cpus[] = {0, 1, 1, 3};

static int __cpuinit s5p_ehci_cpu_notify(struct notifier_block *self,
				unsigned long action, void *hcpu)
{
	int cpu = (unsigned long)hcpu;

	if (!s5p_ehci_irq_no || cpu != s5p_ehci_irq_cpu)
		goto exit;

	switch (action) {
	case CPU_ONLINE:
	case CPU_DOWN_FAILED:
	case CPU_ONLINE_FROZEN:
		irq_set_affinity(s5p_ehci_irq_no, cpumask_of(s5p_ehci_irq_cpu));
		pr_debug("%s: set ehci irq to cpu%d\n", __func__, cpu);
		break;
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
		irq_set_affinity(s5p_ehci_irq_no, cpumask_of(0));
		pr_debug("%s: set ehci irq to cpu%d\n", __func__, 0);
		break;
	default:
		break;
	}
exit:
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata s5p_ehci_cpu_notifier = {
	.notifier_call = s5p_ehci_cpu_notify,
};
#endif

static int __devinit s5p_ehci_probe(struct platform_device *pdev)
{
	struct s5p_ehci_platdata *pdata;
	struct s5p_ehci_hcd *s5p_ehci;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct resource *res;
	int irq;
	int err;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data defined\n");
		return -EINVAL;
	}

	s5p_ehci = kzalloc(sizeof(struct s5p_ehci_hcd), GFP_KERNEL);
	if (!s5p_ehci)
		return -ENOMEM;
	s5p_ehci->dev = &pdev->dev;

	hcd = usb_create_hcd(&s5p_ehci_hc_driver, &pdev->dev,
					dev_name(&pdev->dev));
	if (!hcd) {
		dev_err(&pdev->dev, "Unable to create HCD\n");
		err = -ENOMEM;
		goto fail_hcd;
	}

	s5p_ehci->hcd = hcd;
	s5p_ehci->clk = clk_get(&pdev->dev, "usbhost");

	if (IS_ERR(s5p_ehci->clk)) {
		dev_err(&pdev->dev, "Failed to get usbhost clock\n");
		err = PTR_ERR(s5p_ehci->clk);
		goto fail_clk;
	}

	err = clk_enable(s5p_ehci->clk);
	if (err)
		goto fail_clken;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get I/O memory\n");
		err = -ENXIO;
		goto fail_io;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = ioremap(res->start, resource_size(res));
	if (!hcd->regs) {
		dev_err(&pdev->dev, "Failed to remap I/O memory\n");
		err = -ENOMEM;
		goto fail_io;
	}

	irq = platform_get_irq(pdev, 0);
	if (!irq) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		err = -ENODEV;
		goto fail;
	}

	platform_set_drvdata(pdev, s5p_ehci);

	s5p_ehci_phy_init(pdev);

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs +
		HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));

	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	err = usb_add_hcd(hcd, irq, IRQF_DISABLED | IRQF_SHARED);
	if (err) {
		dev_err(&pdev->dev, "Failed to add USB HCD\n");
		goto fail;
	}

	create_ehci_sys_file(ehci);
	s5p_ehci->power_on = 1;

#ifdef CONFIG_USB_SUSPEND
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif
#ifdef CONFIG_MDM_HSIC_PM
	/* halt controller before driving suspend on ths bus */
	ehci->susp_sof_bug = 1;

	set_host_stat(hsic_pm_dev, POWER_ON);
	pm_runtime_allow(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&hcd->self.root_hub->dev, 0);

	pm_runtime_forbid(&pdev->dev);
	enable_periodic(ehci);
#endif

#ifdef CONFIG_EHCI_IRQ_DISTRIBUTION
	if (num_possible_cpus() > 1) {
		s5p_ehci_irq_no = irq;
		s5p_ehci_irq_cpu = s5p_ehci_cpus[num_possible_cpus() - 1];
		irq_set_affinity(s5p_ehci_irq_no, cpumask_of(s5p_ehci_irq_cpu));
		register_cpu_notifier(&s5p_ehci_cpu_notifier);
	}
#endif

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
	/* for cp enumeration */
	pm_runtime_forbid(&pdev->dev);
	/*HSIC IPC control the ACTIVE_STATE*/
	if (pdata && pdata->noti_host_states)
		pdata->noti_host_states(pdev, S5P_HOST_ON);
#endif

	return 0;

fail:
	iounmap(hcd->regs);
fail_io:
	clk_disable(s5p_ehci->clk);
fail_clken:
	clk_put(s5p_ehci->clk);
fail_clk:
	usb_put_hcd(hcd);
fail_hcd:
	kfree(s5p_ehci);
	return err;
}

static int __devexit s5p_ehci_remove(struct platform_device *pdev)
{
	struct s5p_ehci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;

/* pm_runtime_disable called twice during pdev unregistering
 * it causes disable_depth mismatching, so rpm for this device
 * cannot works from disable_depth count
 * replace it to runtime forbid.
 */
#ifdef CONFIG_USB_SUSPEND
#ifdef CONFIG_MDM_HSIC_PM
	pm_runtime_forbid(&pdev->dev);
#else
	pm_runtime_disable(&pdev->dev);
#endif
#endif
	s5p_ehci->power_on = 0;
	remove_ehci_sys_file(hcd_to_ehci(hcd));
	usb_remove_hcd(hcd);

#ifdef CONFIG_EHCI_IRQ_DISTRIBUTION
	if (num_possible_cpus() > 1) {
		s5p_ehci_irq_no = 0;
		s5p_ehci_irq_cpu = 0;
		unregister_cpu_notifier(&s5p_ehci_cpu_notifier);
	}
#endif

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
	/*HSIC IPC control the ACTIVE_STATE*/
	if (pdata && pdata->noti_host_states)
		pdata->noti_host_states(pdev, S5P_HOST_OFF);
#endif
	if (pdata && pdata->phy_exit)
		pdata->phy_exit(pdev, S5P_USB_PHY_HOST);

	iounmap(hcd->regs);

	clk_disable(s5p_ehci->clk);
	clk_put(s5p_ehci->clk);

	usb_put_hcd(hcd);
	kfree(s5p_ehci);

	return 0;
}

static void s5p_ehci_shutdown(struct platform_device *pdev)
{
	struct s5p_ehci_hcd *s5p_ehci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ehci->hcd;

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}

static const struct dev_pm_ops s5p_ehci_pm_ops = {
	.suspend		= s5p_ehci_suspend,
	.resume			= s5p_ehci_resume,
#ifdef CONFIG_HIBERNATION
	.freeze			= s5p_ehci_suspend,
	.thaw			= s5p_ehci_resume,
	.restore		= s5p_ehci_resume,
#endif
	.runtime_suspend	= s5p_ehci_runtime_suspend,
	.runtime_resume		= s5p_ehci_runtime_resume,
};

static struct platform_driver s5p_ehci_driver = {
	.probe		= s5p_ehci_probe,
	.remove		= __devexit_p(s5p_ehci_remove),
	.shutdown	= s5p_ehci_shutdown,
	.driver = {
		.name	= "s5p-ehci",
		.owner	= THIS_MODULE,
		.pm = &s5p_ehci_pm_ops,
	}
};

MODULE_ALIAS("platform:s5p-ehci");
