/* linux/drivers/video/s5p_mipi_dsi_lowlevel.c
 *
 * Samsung SoC MIPI-DSI lowlevel driver.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * InKi Dae, <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/io.h>

#include <mach/map.h>

#include <plat/mipi_dsim2.h>
#include <plat/regs-dsim2.h>

void s5p_mipi_dsi_func_reset(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_SWRST);

	reg |= DSIM_FUNCRST;

	writel(reg, dsim->reg_base + S5P_DSIM_SWRST);
}

void s5p_mipi_dsi_sw_reset(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_SWRST);

	reg |= DSIM_SWRST;

	writel(reg, dsim->reg_base + S5P_DSIM_SWRST);
}

void s5p_mipi_dsi_sw_reset_release(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_INTSRC);

	reg |= INTSRC_SW_RST_RELEASE;

	writel(reg, dsim->reg_base + S5P_DSIM_INTSRC);
}

int s5p_mipi_dsi_get_sw_reset_release(struct mipi_dsim_device *dsim)
{
	return (readl(dsim->reg_base + S5P_DSIM_INTSRC)) &
			INTSRC_SW_RST_RELEASE;
}

unsigned int s5p_mipi_dsi_read_interrupt_mask(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_INTMSK);

	return reg;
}

void s5p_mipi_dsi_set_interrupt_mask(struct mipi_dsim_device *dsim,
		unsigned int mode, unsigned int mask)
{
	unsigned int reg = 0;

	if (mask)
		reg |= mode;
	else
		reg &= ~mode;

	writel(reg, dsim->reg_base + S5P_DSIM_INTMSK);
}

void s5p_mipi_dsi_init_fifo_pointer(struct mipi_dsim_device *dsim,
		unsigned int cfg)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_FIFOCTRL);

	writel(reg & ~(cfg), dsim->reg_base + S5P_DSIM_FIFOCTRL);
	mdelay(10);
	reg |= cfg;

	writel(reg, dsim->reg_base + S5P_DSIM_FIFOCTRL);
}

/*
 * this function set PLL P, M and S value in D-PHY
 */
void s5p_mipi_dsi_set_phy_tunning(struct mipi_dsim_device *dsim,
		unsigned int value)
{
	writel(DSIM_AFC_CTL(value), dsim->reg_base + S5P_DSIM_PHYACCHR);
}

void s5p_mipi_dsi_set_main_stand_by(struct mipi_dsim_device *dsim,
		unsigned int enable)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_MDRESOL);

	reg &= ~DSIM_MAIN_STAND_BY;

	if (enable)
		reg |= DSIM_MAIN_STAND_BY;

	writel(reg, dsim->reg_base + S5P_DSIM_MDRESOL);
}

void s5p_mipi_dsi_set_main_disp_resol(struct mipi_dsim_device *dsim,
	unsigned int width_resol, unsigned int height_resol)
{
	unsigned int reg;

	/* standby should be set after configuration so set to not ready*/
	reg = (readl(dsim->reg_base + S5P_DSIM_MDRESOL)) &
		~(DSIM_MAIN_STAND_BY);
	writel(reg, dsim->reg_base + S5P_DSIM_MDRESOL);

	reg &= ~((0x7ff << 16) | (0x7ff << 0));
	reg |= DSIM_MAIN_VRESOL(height_resol) | DSIM_MAIN_HRESOL(width_resol);

	reg |= DSIM_MAIN_STAND_BY;
	writel(reg, dsim->reg_base + S5P_DSIM_MDRESOL);
}

void s5p_mipi_dsi_set_main_disp_vporch(struct mipi_dsim_device *dsim,
	unsigned int cmd_allow, unsigned int vfront, unsigned int vback)
{
	unsigned int reg;

	reg = (readl(dsim->reg_base + S5P_DSIM_MVPORCH)) &
		~((DSIM_CMD_ALLOW_MASK) | (DSIM_STABLE_VFP_MASK) |
		(DSIM_MAIN_VBP_MASK));

	reg |= ((cmd_allow & 0xf) << DSIM_CMD_ALLOW_SHIFT) |
		((vfront & 0x7ff) << DSIM_STABLE_VFP_SHIFT) |
		((vback & 0x7ff) << DSIM_MAIN_VBP_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_MVPORCH);
}

void s5p_mipi_dsi_set_main_disp_hporch(struct mipi_dsim_device *dsim,
	unsigned int front, unsigned int back)
{
	unsigned int reg;

	reg = (readl(dsim->reg_base + S5P_DSIM_MHPORCH)) &
		~((DSIM_MAIN_HFP_MASK) | (DSIM_MAIN_HBP_MASK));

	reg |= (front << DSIM_MAIN_HFP_SHIFT) | (back << DSIM_MAIN_HBP_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_MHPORCH);
}

void s5p_mipi_dsi_set_main_disp_sync_area(struct mipi_dsim_device *dsim,
	unsigned int vert, unsigned int hori)
{
	unsigned int reg;

	reg = (readl(dsim->reg_base + S5P_DSIM_MSYNC)) &
		~((DSIM_MAIN_VSA_MASK) | (DSIM_MAIN_HSA_MASK));

	reg |= ((vert & 0x3ff) << DSIM_MAIN_VSA_SHIFT) |
		(hori << DSIM_MAIN_HSA_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_MSYNC);
}

void s5p_mipi_dsi_set_sub_disp_resol(struct mipi_dsim_device *dsim,
	unsigned int vert, unsigned int hori)
{
	unsigned int reg;

	reg = (readl(dsim->reg_base + S5P_DSIM_SDRESOL)) &
		~(DSIM_SUB_STANDY_MASK);

	writel(reg, dsim->reg_base + S5P_DSIM_SDRESOL);

	reg &= ~(DSIM_SUB_VRESOL_MASK) | ~(DSIM_SUB_HRESOL_MASK);
	reg |= ((vert & 0x7ff) << DSIM_SUB_VRESOL_SHIFT) |
		((hori & 0x7ff) << DSIM_SUB_HRESOL_SHIFT);
	writel(reg, dsim->reg_base + S5P_DSIM_SDRESOL);

	reg |= (1 << DSIM_SUB_STANDY_SHIFT);
	writel(reg, dsim->reg_base + S5P_DSIM_SDRESOL);
}

void s5p_mipi_dsi_init_config(struct mipi_dsim_device *dsim)
{
	struct mipi_dsim_config *dsim_config = dsim->dsim_config;

	unsigned int cfg = (readl(dsim->reg_base + S5P_DSIM_CONFIG)) &
		~((1 << 28) | (0x1f << 20) | (0x3 << 5));

#ifdef CONFIG_MACH_SLP_NAPLES
	if (dsim_config->e_interface == DSIM_COMMAND) {
		cfg = (dsim_config->auto_flush << 29) |
		(dsim_config->eot_disable << 28) |
		(dsim_config->e_no_data_lane << DSIM_NUM_OF_DATALANE_SHIFT);
	} else {
#endif
	cfg =	(dsim_config->auto_flush << 29) |
		(dsim_config->eot_disable << 28) |
		(dsim_config->auto_vertical_cnt << DSIM_AUTO_MODE_SHIFT) |
		(dsim_config->hse << DSIM_HSE_MODE_SHIFT) |
		(dsim_config->hfp << DSIM_HFP_MODE_SHIFT) |
		(dsim_config->hbp << DSIM_HBP_MODE_SHIFT) |
		(dsim_config->hsa << DSIM_HSA_MODE_SHIFT) |
		(dsim_config->e_no_data_lane << DSIM_NUM_OF_DATALANE_SHIFT);
#ifdef CONFIG_MACH_SLP_NAPLES
	}
#endif
	writel(cfg, dsim->reg_base + S5P_DSIM_CONFIG);
}

void s5p_mipi_dsi_display_config(struct mipi_dsim_device *dsim,
				struct mipi_dsim_config *dsim_config)
{
	u32 reg = (readl(dsim->reg_base + S5P_DSIM_CONFIG)) &
		~((0x3 << 26) | (1 << 25) | (0x3 << 18) | (0x7 << 12) |
		(0x3 << 16) | (0x7 << 8));

	if (dsim_config->e_interface == DSIM_VIDEO)
		reg |= (1 << 25);
	else if (dsim_config->e_interface == DSIM_COMMAND)
		reg &= ~(1 << 25);
	else {
		dev_err(dsim->dev, "unknown lcd type.\n");
		return;
	}

	/* main lcd */
	reg |= ((u8) (dsim_config->e_burst_mode) & 0x3) << 26 |
		((u8) (dsim_config->e_virtual_ch) & 0x3) << 18 |
		((u8) (dsim_config->e_pixel_format) & 0x7) << 12;

	writel(reg, dsim->reg_base + S5P_DSIM_CONFIG);
}

void s5p_mipi_dsi_enable_lane(struct mipi_dsim_device *dsim, unsigned int lane,
	unsigned int enable)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_CONFIG);

	if (enable)
		reg |= DSIM_LANE_ENx(lane);
	else
		reg &= ~DSIM_LANE_ENx(lane);

	writel(reg, dsim->reg_base + S5P_DSIM_CONFIG);
}


void s5p_mipi_dsi_set_data_lane_number(struct mipi_dsim_device *dsim,
	unsigned int count)
{
	unsigned int cfg;

	/* get the data lane number. */
	cfg = DSIM_NUM_OF_DATA_LANE(count);

	writel(cfg, dsim->reg_base + S5P_DSIM_CONFIG);
}

void s5p_mipi_dsi_enable_afc(struct mipi_dsim_device *dsim, unsigned int enable,
	unsigned int afc_code)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_PHYACCHR);

	if (enable) {
		reg |= (1 << 14);
		reg &= ~(0x7 << 5);
		reg |= (afc_code & 0x7) << 5;
	} else
		reg &= ~(1 << 14);

	writel(reg, dsim->reg_base + S5P_DSIM_PHYACCHR);
}

void s5p_mipi_dsi_enable_pll_bypass(struct mipi_dsim_device *dsim,
	unsigned int enable)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_CLKCTRL)) &
		~(DSIM_PLL_BYPASS_EXTERNAL);

	reg |= enable << DSIM_PLL_BYPASS_SHIFT;

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_set_pll_pms(struct mipi_dsim_device *dsim, unsigned int p,
	unsigned int m, unsigned int s)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_PLLCTRL);

	reg |= ((p & 0x3f) << 13) | ((m & 0x1ff) << 4) | ((s & 0x7) << 1);

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

void s5p_mipi_dsi_pll_freq_band(struct mipi_dsim_device *dsim,
		unsigned int freq_band)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_PLLCTRL)) &
		~(0x1f << DSIM_FREQ_BAND_SHIFT);

	reg |= ((freq_band & 0x1f) << DSIM_FREQ_BAND_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

void s5p_mipi_dsi_pll_freq(struct mipi_dsim_device *dsim,
		unsigned int pre_divider, unsigned int main_divider,
		unsigned int scaler)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_PLLCTRL)) &
		~(0x7ffff << 1);

	reg |= (pre_divider & 0x3f) << 13 | (main_divider & 0x1ff) << 4 |
		(scaler & 0x7) << 1;

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

void s5p_mipi_dsi_pll_stable_time(struct mipi_dsim_device *dsim,
	unsigned int lock_time)
{
	writel(lock_time, dsim->reg_base + S5P_DSIM_PLLTMR);
}

void s5p_mipi_dsi_enable_pll(struct mipi_dsim_device *dsim, unsigned int enable)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_PLLCTRL)) &
		~(0x1 << DSIM_PLL_EN_SHIFT);

	reg |= ((enable & 0x1) << DSIM_PLL_EN_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

void s5p_mipi_dsi_set_byte_clock_src(struct mipi_dsim_device *dsim,
		unsigned int src)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_CLKCTRL)) &
		~(0x3 << DSIM_BYTE_CLK_SRC_SHIFT);

	reg |= ((unsigned int) src) << DSIM_BYTE_CLK_SRC_SHIFT;

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_enable_byte_clock(struct mipi_dsim_device *dsim,
		unsigned int enable)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_CLKCTRL)) &
		~(1 << DSIM_BYTE_CLKEN_SHIFT);

	reg |= enable << DSIM_BYTE_CLKEN_SHIFT;

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_set_esc_clk_prs(struct mipi_dsim_device *dsim,
		unsigned int enable, unsigned int prs_val)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_CLKCTRL)) &
		~((1 << DSIM_ESC_CLKEN_SHIFT) | (0xffff));

	reg |= enable << DSIM_ESC_CLKEN_SHIFT;
	if (enable)
		reg |= prs_val;

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_enable_esc_clk_on_lane(struct mipi_dsim_device *dsim,
		unsigned int lane_sel, unsigned int enable)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_CLKCTRL);

	if (enable)
		reg |= DSIM_LANE_ESC_CLKEN(lane_sel);
	else

		reg &= ~DSIM_LANE_ESC_CLKEN(lane_sel);

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_force_dphy_stop_state(struct mipi_dsim_device *dsim,
	unsigned int enable)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_ESCMODE)) &
		~(0x1 << DSIM_FORCE_STOP_STATE_SHIFT);

	reg |= ((enable & 0x1) << DSIM_FORCE_STOP_STATE_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_ESCMODE);
}

unsigned int s5p_mipi_dsi_is_lane_state(struct mipi_dsim_device *dsim)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_STATUS);

	/**
	 * check clock and data lane states.
	 * if MIPI-DSI controller was enabled at bootloader then
	 * TX_READY_HS_CLK is enabled otherwise STOP_STATE_CLK.
	 * so it should be checked for two case.
	 */
	if ((reg & DSIM_STOP_STATE_DAT(0xf)) &&
			((reg & DSIM_STOP_STATE_CLK) ||
			 (reg & DSIM_TX_READY_HS_CLK)))
		return 1;
	else
		return 0;

	return 0;
}

void s5p_mipi_dsi_set_stop_state_counter(struct mipi_dsim_device *dsim,
		unsigned int cnt_val)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_ESCMODE)) &
		~(0x7ff << DSIM_STOP_STATE_CNT_SHIFT);

	reg |= ((cnt_val & 0x7ff) << DSIM_STOP_STATE_CNT_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_ESCMODE);
}

void s5p_mipi_dsi_set_bta_timeout(struct mipi_dsim_device *dsim,
		unsigned int timeout)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_TIMEOUT)) &
		~(0xff << DSIM_BTA_TOUT_SHIFT);

	reg |= (timeout << DSIM_BTA_TOUT_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_TIMEOUT);
}

void s5p_mipi_dsi_set_lpdr_timeout(struct mipi_dsim_device *dsim,
		unsigned int timeout)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_TIMEOUT)) &
		~(0xffff << DSIM_LPDR_TOUT_SHIFT);

	reg |= (timeout << DSIM_LPDR_TOUT_SHIFT);

	writel(reg, dsim->reg_base + S5P_DSIM_TIMEOUT);
}

void s5p_mipi_dsi_set_cpu_transfer_mode(struct mipi_dsim_device *dsim,
		unsigned int lp)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_ESCMODE);

	reg &= ~DSIM_CMD_LPDT_LP;

	if (lp)
		reg |= DSIM_CMD_LPDT_LP;

	writel(reg, dsim->reg_base + S5P_DSIM_ESCMODE);
}

void s5p_mipi_dsi_set_lcdc_transfer_mode(struct mipi_dsim_device *dsim,
		unsigned int lp)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_ESCMODE);

	reg &= ~DSIM_TX_LPDT_LP;

	if (lp)
		reg |= DSIM_TX_LPDT_LP;

	writel(reg, dsim->reg_base + S5P_DSIM_ESCMODE);
}

void s5p_mipi_dsi_enable_hs_clock(struct mipi_dsim_device *dsim,
		unsigned int enable)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_CLKCTRL)) &
		~(1 << DSIM_TX_REQUEST_HSCLK_SHIFT);

	reg |= enable << DSIM_TX_REQUEST_HSCLK_SHIFT;

	writel(reg, dsim->reg_base + S5P_DSIM_CLKCTRL);
}

void s5p_mipi_dsi_dp_dn_swap(struct mipi_dsim_device *dsim,
		unsigned int swap_en)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_PHYACCHR1);

	reg &= ~(0x3 << 0);
	reg |= (swap_en & 0x3) << 0;

	writel(reg, dsim->reg_base + S5P_DSIM_PHYACCHR1);
}

void s5p_mipi_dsi_hs_zero_ctrl(struct mipi_dsim_device *dsim,
		unsigned int hs_zero)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_PLLCTRL)) &
		~(0xf << 28);

	reg |= ((hs_zero & 0xf) << 28);

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

void s5p_mipi_dsi_prep_ctrl(struct mipi_dsim_device *dsim, unsigned int prep)
{
	unsigned int reg = (readl(dsim->reg_base + S5P_DSIM_PLLCTRL)) &
		~(0x7 << 20);

	reg |= ((prep & 0x7) << 20);

	writel(reg, dsim->reg_base + S5P_DSIM_PLLCTRL);
}

unsigned int s5p_mipi_dsi_read_interrupt(struct mipi_dsim_device *dsim)
{
	return readl(dsim->reg_base + S5P_DSIM_INTSRC);
}

void s5p_mipi_dsi_clear_interrupt(struct mipi_dsim_device *dsim,
					unsigned int src)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_INTSRC);

	reg |= src;

	writel(reg, dsim->reg_base + S5P_DSIM_INTSRC);
}

void s5p_mipi_dsi_set_interrupt(struct mipi_dsim_device *dsim,
					unsigned int src, unsigned int enable)
{
	unsigned int reg = 0;

	if (enable)
		reg |= src;
	else
		reg &= ~src;

	writel(reg, dsim->reg_base + S5P_DSIM_INTSRC);
}

unsigned int s5p_mipi_dsi_is_pll_stable(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	reg = readl(dsim->reg_base + S5P_DSIM_STATUS);

	return reg & (1 << 31) ? 1 : 0;
}

unsigned int s5p_mipi_dsi_get_fifo_state(struct mipi_dsim_device *dsim)
{
	unsigned int ret;

	ret = readl(dsim->reg_base + S5P_DSIM_FIFOCTRL) & ~(0x1f);

	return ret;
}

void s5p_mipi_dsi_wr_tx_header(struct mipi_dsim_device *dsim,
	unsigned int di, unsigned int data0, unsigned int data1)
{
	unsigned int reg = (data1 << 16) | (data0 << 8) | ((di & 0x3f) << 0);

	writel(reg, dsim->reg_base + S5P_DSIM_PKTHDR);
}

void s5p_mipi_dsi_rd_tx_header(struct mipi_dsim_device *dsim,
	unsigned int di, unsigned int data0)
{
	unsigned int reg = (data0 << 8) | (di << 0);

	writel(reg, dsim->reg_base + S5P_DSIM_PKTHDR);
}

unsigned int s5p_mipi_dsi_rd_rx_fifo(struct mipi_dsim_device *dsim)
{
	unsigned int reg = 0;

	reg = readl(dsim->reg_base + S5P_DSIM_RXFIFO);

	return reg;
}

unsigned int _s5p_mipi_dsi_get_frame_done_status(struct mipi_dsim_device *dsim)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_INTSRC);

	return (reg & INTSRC_FRAME_DONE) ? 1 : 0;
}

void _s5p_mipi_dsi_clear_frame_done(struct mipi_dsim_device *dsim)
{
	unsigned int reg = readl(dsim->reg_base + S5P_DSIM_INTSRC);

	writel(reg | INTSRC_FRAME_DONE, dsim->reg_base +
		S5P_DSIM_INTSRC);
}

void s5p_mipi_dsi_wr_tx_data(struct mipi_dsim_device *dsim,
		unsigned int tx_data)
{
	writel(tx_data, dsim->reg_base + S5P_DSIM_PAYLOAD);
}
