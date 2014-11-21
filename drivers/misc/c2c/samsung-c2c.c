/*
 * Samsung C2C driver
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Kisang Lee <kisang80.lee@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/cma.h>
#include <linux/sysfs.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/gpio.h>
#ifdef CONFIG_C2C_IPC_ENABLE
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#endif
#include <asm/mach-types.h>

#include <plat/cpu.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-c2c.h>
#include <mach/regs-pmu.h>
#include <mach/pmu.h>
#include <mach/gpio.h>
#include <mach/dma.h>

#include <mach/c2c.h>
#include "samsung-c2c.h"

static int c2c_set_sharedmem(enum c2c_shrdmem_size size, dma_addr_t addr);
static void c2c_set_interrupt(u32 genio_num, enum c2c_interrupt set_int);

void c2c_reset_ops(void)
{
	u32 set_clk = 0;
	int i;

	c2c_con.setup_gpio(c2c_con.rx_width, c2c_con.tx_width);

	if (c2c_con.opp_mode == C2C_OPP100)
		set_clk = c2c_con.clk_opp100;
	else if (c2c_con.opp_mode == C2C_OPP50)
		set_clk = c2c_con.clk_opp50;
	else if (c2c_con.opp_mode == C2C_OPP25)
		set_clk = c2c_con.clk_opp25;
	else
		set_clk = c2c_con.clk_opp50;

	clk_set_rate(c2c_con.c2c_sclk, (set_clk + 1)  * MHZ);
	clk_set_rate(c2c_con.c2c_aclk, ((set_clk / 2) + 1) * MHZ);

	/* First phase - C2C block reset */
	c2c_set_reset(C2C_CLEAR);
	c2c_set_reset(C2C_SET);
	c2c_set_rtrst(C2C_SET);

	/* Second phase - Clear clock gating */
	c2c_set_clock_gating(C2C_CLEAR);

	/*
	** Third phase - Restore C2C settings
	*/

	/* Restore TX/RX BUS width */
	c2c_set_tx_buswidth(c2c_con.tx_width);
	c2c_set_rx_buswidth(c2c_con.rx_width);
	c2c_writel(((c2c_con.rx_width << 4) | (c2c_con.tx_width)),
			EXYNOS_C2C_PORTCONFIG);

	/* Restore C2C SHDMEM base address & size */
	c2c_set_shdmem_size(c2c_con.shdmem_c2c_size);
	c2c_set_base_addr(c2c_con.shdmem_addr >> 22);
	c2c_set_memdone(C2C_SET);

	pr_debug("[C2C] %s: addr 0x%08X, size %lu\n",
		__func__, c2c_con.shdmem_addr, c2c_con.shdmem_byte_size);

	/* Restore C2C clocks */
	c2c_writel(set_clk, EXYNOS_C2C_FCLK_FREQ);
	c2c_writel(set_clk, EXYNOS_C2C_RX_MAX_FREQ);
	c2c_set_func_clk(0);

	/* Restore retention register */
	c2c_writel(c2c_con.retention_reg, EXYNOS_C2C_IRQ_EN_SET1);

	/* Restore C2C_GENO resister*/
	for (i = 0; i < 32; i++)
		c2c_set_interrupt(i, C2C_INT_HIGH);

	pr_debug("[C2C] %s: sclk %ld Hz\n",
		__func__, clk_get_rate(c2c_con.c2c_sclk));
	pr_debug("[C2C] %s: aclk %ld Hz\n",
		__func__, clk_get_rate(c2c_con.c2c_aclk));

	pr_debug("[C2C] %s: REG.PORT_CONFIG 0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_PORTCONFIG));
	pr_debug("[C2C] %s: REG.FCLK_FREQ   0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_FCLK_FREQ));
	pr_debug("[C2C] %s: REG.RX_MAX_FREQ 0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_RX_MAX_FREQ));
	pr_debug("[C2C] %s: REG.IRQ_EN_SET1 0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_IRQ_EN_SET1));

	pr_debug("[C2C] %s: C2C_GENO_INT 0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_GENO_INT));
	pr_debug("[C2C] %s: C2C_GENO_LEVEL 0x%08X\n",
		__func__, c2c_readl(EXYNOS_C2C_GENO_LEVEL));

	/* Last phase - Set clock gating */
	c2c_set_clock_gating(C2C_SET);
}

static void print_pm_status(int level)
{
	int ap_wakeup;
	int cp_wakeup;
	int cp_status;
	int ap_status;
	struct timespec ts;
	struct tm tm;

	if (level < 0)
		return;

	getnstimeofday(&ts);
	ap_wakeup = gpio_get_value(c2c_con.gpio_ap_wakeup);
	cp_wakeup = gpio_get_value(c2c_con.gpio_cp_wakeup);
	cp_status = gpio_get_value(c2c_con.gpio_cp_status);
	ap_status = gpio_get_value(c2c_con.gpio_ap_status);

	time_to_tm((ts.tv_sec - (sys_tz.tz_minuteswest * 60)), 0, &tm);

	/*
	** PM {ap_wakeup:cp_wakeup:cp_status:ap_status} CALLER
	*/
	if (level > 0) {
		pr_err("mif: [%02d:%02d:%02d.%03ld] C2C PM {%d:%d:%d:%d} "
			"%pf\n", tm.tm_hour, tm.tm_min, tm.tm_sec,
			(ts.tv_nsec > 0) ? (ts.tv_nsec / 1000000) : 0,
			ap_wakeup, cp_wakeup, cp_status, ap_status,
			__builtin_return_address(0));
	} else {
		pr_info("mif: [%02d:%02d:%02d.%03ld] C2C PM {%d:%d:%d:%d} "
			"%pf\n", tm.tm_hour, tm.tm_min, tm.tm_sec,
			(ts.tv_nsec > 0) ? (ts.tv_nsec / 1000000) : 0,
			ap_wakeup, cp_wakeup, cp_status, ap_status,
			__builtin_return_address(0));
	}
}

static ssize_t c2c_ctrl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret;

	c2c_set_clock_gating(C2C_CLEAR);

	ret = sprintf(buf, "C2C State");
	ret += sprintf(&buf[ret], "SysReg : 0x%x\n",
			readl(c2c_con.c2c_sysreg));
	ret += sprintf(&buf[ret], "Port Config : 0x%x\n",
			c2c_readl(EXYNOS_C2C_PORTCONFIG));
	ret += sprintf(&buf[ret], "FCLK_FREQ : %d\n",
			c2c_readl(EXYNOS_C2C_FCLK_FREQ));
	ret += sprintf(&buf[ret], "RX_MAX_FREQ : %d\n",
			c2c_readl(EXYNOS_C2C_RX_MAX_FREQ));
	ret += sprintf(&buf[ret], "IRQ_EN_SET1 : 0x%x\n",
			c2c_readl(EXYNOS_C2C_IRQ_EN_SET1));
	ret += sprintf(&buf[ret], "IRQ_RAW_STAT1 : 0x%x\n",
			c2c_readl(EXYNOS_C2C_IRQ_RAW_STAT1));
	ret += sprintf(&buf[ret], "Get C2C sclk rate : %ld\n",
			clk_get_rate(c2c_con.c2c_sclk));
	ret += sprintf(&buf[ret], "Get C2C aclk rate : %ld\n",
			clk_get_rate(c2c_con.c2c_aclk));

	ret += sprintf(&buf[ret], "EXYNOS_C2C_GENO_INT : 0x%x\n",
			c2c_readl(EXYNOS_C2C_GENO_INT));
	ret += sprintf(&buf[ret], "EXYNOS_C2C_GENO_LEVEL : 0x%x\n",
			c2c_readl(EXYNOS_C2C_GENO_LEVEL));
	ret += sprintf(&buf[ret], "STANDBY_IN (0:linked 1: Dislinked) : %d\n",
			c2c_readl(EXYNOS_C2C_STANDBY_IN));

	c2c_set_clock_gating(C2C_SET);

	return ret;
}

static ssize_t c2c_ctrl_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ops_num, opp_val, req_clk;

	sscanf(buf, "%2d", &ops_num);

	switch (ops_num) {
	case 1:
		c2c_writel(((c2c_con.rx_width << 4) | (c2c_con.tx_width)),
			EXYNOS_C2C_PORTCONFIG);
		c2c_set_sharedmem(c2c_con.shdmem_c2c_size, c2c_con.shdmem_addr);
		c2c_set_memdone(C2C_SET);
		break;

	case 2:
	case 3:
	case 4:
		opp_val = ops_num - 1;
		req_clk = 0;
		pr_err("[C2C] Set current OPP mode (%d)\n", opp_val);

		if (opp_val == C2C_OPP100)
			req_clk = c2c_con.clk_opp100;
		else if (opp_val == C2C_OPP50)
			req_clk = c2c_con.clk_opp50;
		else if (opp_val == C2C_OPP25)
			req_clk = c2c_con.clk_opp25;

		if (opp_val == 0 || req_clk == 1) {
			pr_err("[C2C] ERR! undefined OPP mode\n");
		} else {
			c2c_set_clock_gating(C2C_CLEAR);
			if (c2c_con.opp_mode < opp_val) {
				/* increase case */
				clk_set_rate(c2c_con.c2c_sclk,
					(req_clk + 1) * MHZ);
				c2c_writel(req_clk, EXYNOS_C2C_FCLK_FREQ);
				c2c_set_func_clk(req_clk);
				c2c_writel(req_clk, EXYNOS_C2C_RX_MAX_FREQ);
			} else if (c2c_con.opp_mode > opp_val) {
				/* decrease case */
				c2c_writel(req_clk, EXYNOS_C2C_RX_MAX_FREQ);
				clk_set_rate(c2c_con.c2c_sclk,
					(req_clk + 1) * MHZ);
				c2c_writel(req_clk, EXYNOS_C2C_FCLK_FREQ);
				c2c_set_func_clk(req_clk);
			} else{
				pr_err("[C2C] same OPP mode\n");
			}
			c2c_con.opp_mode = opp_val;
			c2c_set_clock_gating(C2C_SET);
		}

		pr_err("[C2C] sclk rate %ld\n", clk_get_rate(c2c_con.c2c_sclk));
		pr_err("[C2C] aclk rate %ld\n", clk_get_rate(c2c_con.c2c_aclk));
		break;

	default:
		pr_err("[C2C] Wrong C2C operation number\n");
		pr_err("[C2C] ---C2C Operation Number---\n");
		pr_err("[C2C] 1. C2C Reset\n");
		pr_err("[C2C] 2. Set OPP25\n");
		pr_err("[C2C] 3. Set OPP50\n");
		pr_err("[C2C] 4. Set OPP100\n");
	}

	return count;
}

static DEVICE_ATTR(c2c_ctrl, 0644, c2c_ctrl_show, c2c_ctrl_store);

int c2c_open(struct inode *inode, struct file *filp)
{
	pr_err("[C2C] %s\n", __func__);
	return 0;
}

int c2c_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int err;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long pfn = __phys_to_pfn(c2c_con.shdmem_addr + offset);

	if ((offset + size) > c2c_con.shdmem_byte_size) {
		pr_err("[C2C] ERR! (offset + size) > %lu MB\n",
			c2c_con.shdmem_byte_size >> 20);
		return -EINVAL;
	}

	/* I/O memory must be noncacheable. */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= (VM_IO | VM_RESERVED);

	err = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if (err) {
		pr_err("[C2C] ERR! remap_pfn_range fail\n");
		return -EAGAIN;
	}

	pr_err("[C2C] %s: vm_start 0x%08X\n", __func__, (u32)vma->vm_start);
	pr_err("[C2C] %s: vm_end   0x%08X\n", __func__, (u32)vma->vm_end);
	pr_err("[C2C] %s: vm_flags 0x%08X\n", __func__, (u32)vma->vm_flags);
	return 0;
}

static long c2c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	c2c_reset_ops();
	return 0;
}

static int c2c_release(struct inode *inode, struct file *filp)
{
	pr_err("[C2C] %s\n", __func__);
	return 0;
}

static const struct file_operations c2c_fops = {
	.owner = THIS_MODULE,
	.open = c2c_open,
	.release = c2c_release,
	.mmap = c2c_mmap,
	.unlocked_ioctl = c2c_ioctl,
};

static struct miscdevice char_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= C2C_DEV_NAME,
	.fops	= &c2c_fops
};

static int c2c_set_sharedmem(enum c2c_shrdmem_size size, dma_addr_t addr)
{
	dma_addr_t base = addr;
	size_t alloc = 1 << (22 + size);

	if (addr == 0) {
		pr_err("[C2C] %s: call cma_alloc() ...\n", __func__);
		base = cma_alloc(c2c_con.c2c_dev, "c2c_shdmem", alloc, 0);
		if (IS_ERR_VALUE(base)) {
			pr_err("[C2C] %s: cma_alloc fail\n", __func__);
			cma_free(base);
			return -ENOMEM;
		}
		c2c_con.shdmem_addr = base;
	}
	c2c_con.shdmem_c2c_size = size;
	c2c_con.shdmem_byte_size = alloc;

	/* Set DRAM Base Addr & Size */
	c2c_set_shdmem_size(size);
	c2c_set_base_addr(base >> 22);

	pr_info("[C2C] %s: base 0x%08X, size %d\n", __func__, (int)base, alloc);

	return 0;
}

unsigned long c2c_get_phys_base(void)
{
	return c2c_con.shdmem_addr;
}
EXPORT_SYMBOL(c2c_get_phys_base);

unsigned long c2c_get_phys_size(void)
{
	return c2c_con.shdmem_byte_size;
}
EXPORT_SYMBOL(c2c_get_phys_size);

unsigned long c2c_get_sh_rgn_offset(void)
{
	return C2C_SH_RGN_OFFSET;
}
EXPORT_SYMBOL(c2c_get_sh_rgn_offset);

unsigned long c2c_get_sh_rgn_size(void)
{
	return C2C_SH_RGN_SIZE;
}
EXPORT_SYMBOL(c2c_get_sh_rgn_size);

static void c2c_set_interrupt(u32 genio_num, enum c2c_interrupt set_int)
{
	u32 cur_int_reg, cur_lev_reg;

	cur_int_reg = c2c_readl(EXYNOS_C2C_GENO_INT);
	cur_lev_reg = c2c_readl(EXYNOS_C2C_GENO_LEVEL);

	switch (set_int) {
	case C2C_INT_TOGGLE:
		cur_int_reg &= ~(0x1 << genio_num);
		c2c_writel(cur_int_reg, EXYNOS_C2C_GENO_INT);
		break;
	case C2C_INT_HIGH:
		cur_int_reg |= (0x1 << genio_num);
		cur_lev_reg |= (0x1 << genio_num);
		c2c_writel(cur_int_reg, EXYNOS_C2C_GENO_INT);
		c2c_writel(cur_lev_reg, EXYNOS_C2C_GENO_LEVEL);
		break;
	case C2C_INT_LOW:
		cur_int_reg |= (0x1 << genio_num);
		cur_lev_reg &= ~(0x1 << genio_num);
		c2c_writel(cur_int_reg, EXYNOS_C2C_GENO_INT);
		c2c_writel(cur_lev_reg, EXYNOS_C2C_GENO_LEVEL);
		break;
	}
}

static irqreturn_t c2c_sscm0_irq(int irq, void *data)
{
	/* TODO : This function will be used other type boards */
	return IRQ_HANDLED;
}

static irqreturn_t c2c_sscm1_irq(int irq, void *data)
{
	u32 en_irq = c2c_readl(EXYNOS_C2C_IRQ_EN_STAT1);
	u32 raw_irq = c2c_readl(EXYNOS_C2C_IRQ_RAW_STAT1);

	/* Clear interrupt here ASAP */
	c2c_writel(raw_irq, EXYNOS_C2C_IRQ_EN_STAT1);

	pr_debug("[C2C] %s: en_irq 0x%08X, raw_irq 0x%08X\n",
		__func__, en_irq, raw_irq);

#ifdef CONFIG_C2C_IPC_ENABLE
	if ((en_irq >> C2C_GENIO_MBOX_INT) & 1) {
		if (c2c_con.hd.handler)
			c2c_con.hd.handler(c2c_con.hd.data, raw_irq);
	}
#endif

	return IRQ_HANDLED;
}

static void set_c2c_device(struct platform_device *pdev)
{
	struct exynos_c2c_platdata *pdata = pdev->dev.platform_data;
	u32 set_clk;
	int i;

	exynos_c2c_request_pwr_mode = exynos4_c2c_request_pwr_mode;

	spin_lock_init(&c2c_con.pm_lock);

	c2c_con.c2c_sysreg = pdata->c2c_sysreg;
	c2c_con.rx_width = pdata->rx_width;
	c2c_con.tx_width = pdata->tx_width;
	c2c_con.clk_opp100 = pdata->clk_opp100;
	c2c_con.clk_opp50 = pdata->clk_opp50;
	c2c_con.clk_opp25 = pdata->clk_opp25;
	c2c_con.opp_mode = pdata->default_opp_mode;
	c2c_con.shdmem_addr = pdata->shdmem_addr;
	c2c_con.shdmem_c2c_size = pdata->shdmem_size;
#ifdef CONFIG_C2C_IPC_ENABLE
	c2c_con.shd_pages = NULL;
	c2c_con.hd.data = NULL;
	c2c_con.hd.handler = NULL;
#endif
	c2c_con.c2c_clk = clk_get(&pdev->dev, "c2c");
	c2c_con.c2c_sclk = clk_get(&pdev->dev, "sclk_c2c");
	c2c_con.c2c_aclk = clk_get(&pdev->dev, "aclk_c2c");

	clk_enable(c2c_con.c2c_clk);

	writel(C2C_SYSREG_DEFAULT, c2c_con.c2c_sysreg);

	/* Set clock to default mode */
	if (c2c_con.opp_mode == C2C_OPP100)
		set_clk = c2c_con.clk_opp100;
	else if (c2c_con.opp_mode == C2C_OPP50)
		set_clk = c2c_con.clk_opp50;
	else if (c2c_con.opp_mode == C2C_OPP25)
		set_clk = c2c_con.clk_opp25;
	else {
		pr_err("[C2C] Default OPP mode is not selected.\n");
		c2c_con.opp_mode = C2C_OPP50;
		set_clk = c2c_con.clk_opp50;
	}

	clk_set_rate(c2c_con.c2c_sclk, (set_clk + 1)  * MHZ);
	clk_set_rate(c2c_con.c2c_aclk, ((set_clk / 2) + 1) * MHZ);

	pr_err("[C2C] Get C2C sclk rate : %ld\n",
				clk_get_rate(c2c_con.c2c_sclk));
	pr_err("[C2C] Get C2C aclk rate : %ld\n",
				clk_get_rate(c2c_con.c2c_aclk));

	if (pdata->setup_gpio) {
		c2c_con.setup_gpio = pdata->setup_gpio;
		pdata->setup_gpio(pdata->rx_width, pdata->tx_width);
	}

	c2c_set_sharedmem(pdata->shdmem_size, pdata->shdmem_addr);

	/* Set SYSREG to memdone */
	c2c_set_memdone(C2C_SET);
	c2c_set_clock_gating(C2C_CLEAR);

	/* Set C2C clock register */
	c2c_writel(set_clk, EXYNOS_C2C_FCLK_FREQ);
	c2c_writel(set_clk, EXYNOS_C2C_RX_MAX_FREQ);
	c2c_set_func_clk(0);

	/* Set C2C buswidth */
	c2c_writel(((pdata->rx_width << 4) | (pdata->tx_width)),
					EXYNOS_C2C_PORTCONFIG);
	c2c_set_tx_buswidth(pdata->tx_width);
	c2c_set_rx_buswidth(pdata->rx_width);

	/* Enable all of GENI/O Interrupt */
	c2c_writel((0x1 << C2C_GENIO_MBOX_INT), EXYNOS_C2C_IRQ_EN_SET1);
	c2c_con.retention_reg |= (0x1 << C2C_GENIO_MBOX_INT);

	c2c_writel((0x1 << C2C_GENIO_MBOX_EXT_INT), EXYNOS_C2C_IRQ_EN_SET1);
	c2c_con.retention_reg |= (0x1 << C2C_GENIO_MBOX_EXT_INT);

	if (exynos_c2c_request_pwr_mode != NULL)
		exynos_c2c_request_pwr_mode(MAX_LATENCY);

	for (i = 0; i < 32; i++)
		c2c_set_interrupt(i, C2C_INT_HIGH);

	pr_err("[C2C] REG.PORT_CONFIG: 0x%08X\n",
		c2c_readl(EXYNOS_C2C_PORTCONFIG));
	pr_err("[C2C] REG.FCLK_FREQ  : 0x%08X\n",
		c2c_readl(EXYNOS_C2C_FCLK_FREQ));
	pr_err("[C2C] REG.RX_MAX_FREQ: 0x%08X\n",
		c2c_readl(EXYNOS_C2C_RX_MAX_FREQ));
	pr_err("[C2C] REG.IRQ_EN_SET1: 0x%08X\n",
		c2c_readl(EXYNOS_C2C_IRQ_EN_SET1));

	c2c_set_clock_gating(C2C_SET);
}

#ifdef CONFIG_C2C_IPC_ENABLE
void __iomem *c2c_request_cp_region(unsigned int cp_addr,
		unsigned int size)
{
	dma_addr_t phy_cpmem;

	phy_cpmem = cma_alloc(c2c_con.c2c_dev, "c2c_shdmem", size, 0);
	if (IS_ERR_VALUE(phy_cpmem)) {
		pr_err("[C2C] ERR! cma_alloc fail\n");
		return NULL;
	}

	return phys_to_virt(phy_cpmem);
}
EXPORT_SYMBOL(c2c_request_cp_region);

void c2c_release_cp_region(void *rgn)
{
	dma_addr_t phy_cpmem;

	phy_cpmem = virt_to_phys(rgn);

	cma_free(phy_cpmem);
}
EXPORT_SYMBOL(c2c_release_cp_region);

void __iomem *c2c_request_sh_region(unsigned int sh_addr, unsigned int size)
{
	int i;
	struct page **pages;
	unsigned int num_pages = (size >> PAGE_SHIFT);
	void *pv;

	pages = kmalloc(num_pages * sizeof(*pages), GFP_KERNEL);
	for (i = 0; i < num_pages; i++) {
		pages[i] = phys_to_page(sh_addr);
		sh_addr += PAGE_SIZE;
	}

	c2c_con.shd_pages = (void *)pages;

	pv = vmap(pages, num_pages, VM_MAP, pgprot_noncached(PAGE_KERNEL));

	return (void __iomem *)pv;
}
EXPORT_SYMBOL(c2c_request_sh_region);

void c2c_release_sh_region(void *rgn)
{
	vunmap(rgn);
	kfree(c2c_con.shd_pages);
	c2c_con.shd_pages = NULL;
}
EXPORT_SYMBOL(c2c_release_sh_region);

int c2c_register_handler(void (*handler)(void *, u32), void *data)
{
	if (!handler)
		return -EINVAL;

	c2c_con.hd.data = data;
	c2c_con.hd.handler = handler;
	return 0;
}
EXPORT_SYMBOL(c2c_register_handler);

int c2c_unregister_handler(void (*handler)(void *, u32))
{
	if (!handler || (c2c_con.hd.handler != handler))
		return -EINVAL;

	c2c_con.hd.data = NULL;
	c2c_con.hd.handler = NULL;
	return 0;
}
EXPORT_SYMBOL(c2c_unregister_handler);

void c2c_send_interrupt(u32 cmd)
{
	pr_debug("[C2C] %s: int_val 0x%08X\n", __func__, cmd);

	if (c2c_read_link()) {
		pr_err("[C2C] %s: c2c_read_link fail\n", __func__);
		return;
	}

	c2c_writel(cmd,	EXYNOS_C2C_GENI_CONTROL);
	c2c_writel(0x0,	EXYNOS_C2C_GENI_CONTROL);
}
EXPORT_SYMBOL(c2c_send_interrupt);

void c2c_reset_interrupt(void)
{
	c2c_writel(c2c_readl(EXYNOS_C2C_IRQ_RAW_STAT1),
		EXYNOS_C2C_IRQ_EN_STAT1);
}
EXPORT_SYMBOL(c2c_reset_interrupt);

u32 c2c_read_interrupt(void)
{
	u32 intr;

	if (c2c_read_link())
		return 0;

	/* Read interrupt value */
	intr = c2c_readl(EXYNOS_C2C_IRQ_RAW_STAT1);

	/* Reset interrupt port */
	c2c_writel(intr, EXYNOS_C2C_IRQ_EN_STAT1);

	return intr;
}
EXPORT_SYMBOL(c2c_read_interrupt);

u32 c2c_read_link(void)
{
	u32 linkStat;
	c2c_set_clock_gating(C2C_CLEAR);
	linkStat = c2c_readl(EXYNOS_C2C_STANDBY_IN);
	c2c_set_clock_gating(C2C_SET);
	return linkStat;
}
EXPORT_SYMBOL(c2c_read_link);

void c2c_reload(void)
{
	c2c_reset_ops();
}
EXPORT_SYMBOL(c2c_reload);
#endif /*CONFIG_C2C_IPC_ENABLE*/

static int __devinit samsung_c2c_probe(struct platform_device *pdev)
{
	struct exynos_c2c_platdata *pdata = pdev->dev.platform_data;
	struct resource *res0;
	struct resource *res1;
	int sscm_irq0, sscm_irq1;
	int err = 0;
	pr_err("[C2C] %s: +++\n", __func__);

	c2c_con.c2c_dev = &pdev->dev;

	/* resource for AP's SSCM region */
	res0 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res0) {
		dev_err(&pdev->dev, "no memory resource defined(AP's SSCM)\n");
		return -ENOENT;
	}

	res0 = request_mem_region(res0->start, resource_size(res0), pdev->name);
	if (!res0) {
		dev_err(&pdev->dev, "failded to request memory resource(AP)\n");
		return -ENOENT;
	}

	pdata->ap_sscm_addr = ioremap(res0->start, resource_size(res0));
	if (!pdata->ap_sscm_addr) {
		dev_err(&pdev->dev, "failded to request memory resource(AP)\n");
		goto release_ap_sscm;
	}
	c2c_con.ap_sscm_addr = pdata->ap_sscm_addr;

	/* resource for CP's SSCM region */
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res1) {
		dev_err(&pdev->dev, "no memory resource defined(AP's SSCM)\n");
		goto unmap_ap_sscm;
	}
	res1 = request_mem_region(res1->start, resource_size(res1), pdev->name);
	if (!res1) {
		dev_err(&pdev->dev, "failded to request memory resource(AP)\n");
		goto unmap_ap_sscm;
	}
	pdata->cp_sscm_addr = ioremap(res1->start, resource_size(res1));
	if (!pdata->cp_sscm_addr) {
		dev_err(&pdev->dev, "failded to request memory resource(CP)\n");
		goto release_cp_sscm;
	}
	c2c_con.cp_sscm_addr = pdata->cp_sscm_addr;

	/* Request IRQ */
	sscm_irq0 = platform_get_irq(pdev, 0);
	if (sscm_irq0 < 0) {
		dev_err(&pdev->dev, "no irq specified\n");
		goto unmap_cp_sscm;
	}
	err = request_irq(sscm_irq0, c2c_sscm0_irq, 0, pdev->name, pdev);
	if (err) {
		dev_err(&pdev->dev, "Can't request SSCM0 IRQ\n");
		goto unmap_cp_sscm;
	}
	/* SSCM0 irq will be only used for master(CP) device */
	disable_irq(sscm_irq0);

	sscm_irq1 = platform_get_irq(pdev, 1);
	if (sscm_irq1 < 0) {
		dev_err(&pdev->dev, "no irq specified\n");
		goto release_sscm_irq0;
	}

	err = request_irq(sscm_irq1, c2c_sscm1_irq, IRQF_NO_SUSPEND,
			pdev->name, pdev);
	if (err) {
		dev_err(&pdev->dev, "Can't request SSCM1 IRQ\n");
		goto release_sscm_irq0;
	}

	err = misc_register(&char_dev);
	if (err) {
		dev_err(&pdev->dev, "Can't register chrdev!\n");
		goto release_sscm_irq0;
	}

	set_c2c_device(pdev);

	/* Create sysfs file for C2C debug */
	err = device_create_file(&pdev->dev, &dev_attr_c2c_ctrl);
	if (err) {
		dev_err(&pdev->dev, "Failed to create sysfs for C2C\n");
		goto release_sscm_irq1;
	}

	pr_err("[C2C] %s: ---\n", __func__);
	return 0;

release_sscm_irq1:
	free_irq(sscm_irq1, pdev);

release_sscm_irq0:
	free_irq(sscm_irq0, pdev);

unmap_cp_sscm:
	iounmap(pdata->cp_sscm_addr);

release_cp_sscm:
	release_mem_region(res1->start, resource_size(res1));

unmap_ap_sscm:
	iounmap(pdata->ap_sscm_addr);

release_ap_sscm:
	release_mem_region(res0->start, resource_size(res0));

	return err;
}

static int __devexit samsung_c2c_remove(struct platform_device *pdev)
{
	/* TODO */
	return 0;
}

#ifdef CONFIG_PM
static int samsung_c2c_suspend(struct platform_device *dev, pm_message_t pm)
{
	int ap_wakeup = gpio_get_value(c2c_con.gpio_ap_wakeup);
	int cp_status = gpio_get_value(c2c_con.gpio_cp_status);

	print_pm_status(1);

	if (cp_status || ap_wakeup) {
		print_pm_status(1);
		return -EAGAIN;
	}

	return 0;
}

static int samsung_c2c_resume(struct platform_device *dev)
{
	print_pm_status(1);

	if (gpio_get_value(c2c_con.gpio_ap_wakeup))
		gpio_set_value(c2c_con.gpio_ap_status, 1);

	return 0;
}

void c2c_assign_gpio_ap_wakeup(unsigned int gpio)
{
	c2c_con.gpio_ap_wakeup = gpio;
}
EXPORT_SYMBOL(c2c_assign_gpio_ap_wakeup);

void c2c_assign_gpio_ap_status(unsigned int gpio)
{
	c2c_con.gpio_ap_status = gpio;
}
EXPORT_SYMBOL(c2c_assign_gpio_ap_status);

void c2c_assign_gpio_cp_wakeup(unsigned int gpio)
{
	c2c_con.gpio_cp_wakeup = gpio;
}
EXPORT_SYMBOL(c2c_assign_gpio_cp_wakeup);

void c2c_assign_gpio_cp_status(unsigned int gpio)
{
	c2c_con.gpio_cp_status = gpio;
}
EXPORT_SYMBOL(c2c_assign_gpio_cp_status);

bool c2c_connected(void)
{
	if (gpio_get_value(c2c_con.gpio_ap_wakeup))
		return true;

	if (gpio_get_value(c2c_con.gpio_cp_wakeup))
		return true;

	if (gpio_get_value(c2c_con.gpio_cp_status))
		return true;

	if (gpio_get_value(c2c_con.gpio_ap_status))
		return true;

	return false;
}
EXPORT_SYMBOL(c2c_connected);

int c2c_suspend(void)
{
	int ret = 0;
	unsigned long flags;

	/* Acquire a spin lock for C2C PM */
	spin_lock_irqsave(&c2c_con.pm_lock, flags);

	if (c2c_connected()) {
		print_pm_status(1);
		ret = -EBUSY;
		goto exit;
	}

	c2c_set_clock_gating(C2C_CLEAR);
	c2c_con.suspended = true;

exit:
	/* Release the spin lock */
	spin_unlock_irqrestore(&c2c_con.pm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(c2c_suspend);

int c2c_resume(void)
{
	unsigned long flags;

	/* Acquire a spin lock for C2C PM */
	spin_lock_irqsave(&c2c_con.pm_lock, flags);

	c2c_reset_ops();
	c2c_con.suspended = false;

	/* Release the spin lock */
	spin_unlock_irqrestore(&c2c_con.pm_lock, flags);

	return 0;
}
EXPORT_SYMBOL(c2c_resume);

bool c2c_suspended(void)
{
	int ret;
	unsigned long flags;

	/* Acquire a spin lock for C2C PM */
	spin_lock_irqsave(&c2c_con.pm_lock, flags);

	ret = c2c_con.suspended;

	/* Release the spin lock */
	spin_unlock_irqrestore(&c2c_con.pm_lock, flags);

	return ret;
}
EXPORT_SYMBOL(c2c_suspended);
#else
#define samsung_c2c_suspend NULL
#define samsung_c2c_resume NULL
#endif

static struct platform_driver samsung_c2c_driver = {
	.probe = samsung_c2c_probe,
	.remove = __devexit_p(samsung_c2c_remove),
	.suspend = samsung_c2c_suspend,
	.resume = samsung_c2c_resume,
	.driver = {
		.name = "samsung-c2c",
		.owner = THIS_MODULE,
	},
};

static int __init samsung_c2c_init(void)
{
	return platform_driver_register(&samsung_c2c_driver);
}
module_init(samsung_c2c_init);

static void __exit samsung_c2c_exit(void)
{
	platform_driver_unregister(&samsung_c2c_driver);
}
module_exit(samsung_c2c_exit);

MODULE_DESCRIPTION("Samsung C2C driver");
MODULE_AUTHOR("Kisang Lee <kisang80.lee@samsung.com>");
MODULE_LICENSE("GPL");

