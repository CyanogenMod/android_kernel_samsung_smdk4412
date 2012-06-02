/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : OCI.c
 *  [Description] : The file implement the external
 *		and internal functions of OCI
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2009/02/10
 *  [Revision History]
 *	  (1) 2008/06/12   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Created this file and Implement functions of OCI
 *
 ****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/


static bool ch_enable[16];

/* OCI Internal Functions */

static void oci_set_global_interrupt(bool set)
{
	gahbcfg_t ahbcfg;

	otg_dbg(OTG_DBG_OCI, "oci_set_global_interrupt\n");

	ahbcfg.d32 = 0;
	ahbcfg.b.glblintrmsk = 1;

	if (set)
		update_reg_32(GAHBCFG, ahbcfg.d32);
	else
		clear_reg_32(GAHBCFG, ahbcfg.d32);
}

static int oci_channel_alloc(u8 *ch_num)
{
	hcchar_t hcchar = {.d32 = 0};
	u8 ch;

	for (ch = 0 ; ch < 16 ; ch++) {
		if (ch_enable[ch] == true) {

			hcchar.d32 = read_reg_32(HCCHAR(ch));

			if (hcchar.b.chdis == 0) {
				*ch_num = ch;
				ch_enable[ch] = false;
				return USB_ERR_SUCCESS;
			}
		}
	}

	return USB_ERR_FAIL;
}



static void oci_flush_tx_fifo(u32 num)
{
	grstctl_t greset = {.d32 = 0};
	u32 count = 0;

	otg_dbg(OTG_DBG_OCI, "oci_flush_tx_fifo\n");

	greset.b.txfflsh = 1;
	greset.b.txfnum = num;
	write_reg_32(GRSTCTL, greset.d32);

	/* wait for flush to end */
	while (greset.b.txfflsh == 1) {
		greset.d32 = read_reg_32(GRSTCTL);
		if (++count > MAX_COUNT)
			break;
	};

	/* Wait for 3 PHY Clocks*/
	udelay(30);
}

static void oci_flush_rx_fifo(void)
{
	grstctl_t greset = {.d32 = 0};
	u32 count = 0;

	otg_dbg(OTG_DBG_OCI, "oci_flush_rx_fifo\n");

	greset.b.rxfflsh = 1;
	write_reg_32(GRSTCTL, greset.d32);

	do {
		greset.d32 = read_reg_32(GRSTCTL);
		if (++count > MAX_COUNT)
			break;
	} while (greset.b.rxfflsh == 1);

	/* Wait for 3 PHY Clocks*/
	udelay(30);
}

static void oci_config_flush_fifo(u32 mode)
{
	ghwcfg2_t hwcfg2 = {.d32 = 0};

	otg_dbg(OTG_DBG_OCI, "oci_config_flush_fifo\n");

	hwcfg2.d32 = read_reg_32(GHWCFG2);

	/* Configure data FIFO sizes */
	if (hwcfg2.b.dynamic_fifo) {
		/* Rx FIFO */
		write_reg_32(GRXFSIZ, 0x0000010D);

		/* Non-periodic Tx FIFO */
		write_reg_32(GNPTXFSIZ, 0x0080010D);

		if (mode == OTG_HOST_MODE) {
			/* For Periodic transactions, */
			/* program HPTXFSIZ */
		}
	}

	/* Flush the FIFOs */
	oci_flush_tx_fifo(0);

	oci_flush_rx_fifo();
}

static int oci_core_reset(void)
{
	grstctl_t greset = {.d32 = 0};
	u32 count = 0;

	otg_dbg(OTG_DBG_OCI, "oci_core_reset\n");

	/* Wait for AHB master IDLE state. */
	do {
		greset.d32 = read_reg_32(GRSTCTL);
		mdelay(50);

		if (++count > 100) {
			otg_dbg(OTG_DBG_OCI, "AHB status is not IDLE\n");
			return USB_ERR_FAIL;
		}

	} while (greset.b.ahbidle != 1);

	/* Core Soft Reset */
	greset.b.csftrst = 1;
	write_reg_32(GRSTCTL, greset.d32);

	/* Wait for 3 PHY Clocks*/
	mdelay(50);
	return USB_ERR_SUCCESS;
}

/**
 * int oci_core_init(void)
 *
 * @brief process core initialize as s5pc110 otg spec
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
static int oci_core_init(void)
{
	gahbcfg_t ahbcfg	= {.d32 = 0};
	gusbcfg_t usbcfg	= {.d32 = 0};
	ghwcfg2_t hwcfg2	= {.d32 = 0};
	gintmsk_t gintmsk	= {.d32 = 0};

	otg_dbg(OTG_DBG_OCI, "oci_core_init\n");

	usbcfg.d32 = read_reg_32(GUSBCFG);
	otg_dbg(OTG_DBG_OCI, "before - GUSBCFG=0x%x, GOTGCTL=0x%x\n",
			usbcfg.d32, read_reg_32(GOTGCTL));

	/* PHY parameters */
	usbcfg.b.physel			= 0;
	usbcfg.b.phyif			= 1; /* 16 bit */
	usbcfg.b.ulpi_utmi_sel	= 0; /* UTMI */
	/* usbcfg.b.ddrsel		= 1; */ /* DDR */
	usbcfg.b.usbtrdtim		= 5; /* 16 bit UTMI */
	usbcfg.b.toutcal		= 7;
	usbcfg.b.forcehstmode	= 1;
	write_reg_32(GUSBCFG, usbcfg.d32);

	otg_dbg(OTG_DBG_OCI,
		"after - GUSBCFG=0x%x, GOTGCTL=0x%x\n",
		read_reg_32(GUSBCFG),
		read_reg_32(GOTGCTL));

	/* Reset after setting the PHY parameters */
	if (oci_core_reset() == USB_ERR_SUCCESS) {
		/* Program the GAHBCFG Register.*/
		hwcfg2.d32 = read_reg_32(GHWCFG2);

		switch (hwcfg2.b.architecture) {
		case HWCFG2_ARCH_SLAVE_ONLY:
			otg_dbg(OTG_DBG_OCI, "Slave Only Mode\n");
			ahbcfg.b.nptxfemplvl = 0;
			ahbcfg.b.ptxfemplvl = 0;
			break;

		case HWCFG2_ARCH_EXT_DMA:
			otg_dbg(OTG_DBG_OCI, "External DMA Mode - TBD!\n");
			break;

		case HWCFG2_ARCH_INT_DMA:
			otg_dbg(OTG_DBG_OCI, "Internal DMA Setting\n");
			ahbcfg.b.dmaenable = true;
			ahbcfg.b.hburstlen = INT_DMA_MODE_INCR4;
			break;

		default:
			otg_dbg(OTG_DBG_OCI, "ERR> hwcfg2\n ");
			break;
		}
		write_reg_32(GAHBCFG, ahbcfg.d32);

		/* Program the GUSBCFG register.*/
		switch (hwcfg2.b.op_mode) {
		case MODE_HNP_SRP_CAPABLE:
			otg_dbg(OTG_DBG_OCI,
				"GHWCFG2 OP Mode : MODE_HNP_SRP_CAPABLE\n");
			usbcfg.b.hnpcap = 1;
			usbcfg.b.srpcap = 1;

			otg_dbg(OTG_DBG_OCI,
				"OTG_DBG_OCI : use HNP and SRP\n");
			break;

		case MODE_SRP_ONLY_CAPABLE:
			otg_dbg(OTG_DBG_OCI,
				"GHWCFG2 OP Mode : MODE_SRP_ONLY_CAPABLE\n");
			usbcfg.b.srpcap = 1;
			break;

		case MODE_NO_HNP_SRP_CAPABLE:
			otg_dbg(OTG_DBG_OCI,
				"GHWCFG2 OP Mode : MODE_NO_HNP_SRP_CAPABLE\n");
			usbcfg.b.hnpcap = 0;
			break;

		case MODE_SRP_CAPABLE_DEVICE:
			otg_dbg(OTG_DBG_OCI,
				"GHWCFG2 OP Mode : MODE_SRP_CAPABLE_DEVICE\n");
			usbcfg.b.srpcap = 1;
			break;

		case MODE_NO_SRP_CAPABLE_DEVICE:
			otg_dbg(OTG_DBG_OCI,
			"GHWCFG2 OP Mode : MODE_NO_SRP_CAPABLE_DEVICE\n");
			usbcfg.b.srpcap = 0;
			break;

		case MODE_SRP_CAPABLE_HOST:
			otg_dbg(OTG_DBG_OCI,
				"GHWCFG2 OP Mode : MODE_SRP_CAPABLE_HOST\n");
			usbcfg.b.srpcap = 1;
			break;

		case MODE_NO_SRP_CAPABLE_HOST:
			otg_dbg(OTG_DBG_OCI,
			"GHWCFG2 OP Mode : MODE_NO_SRP_CAPABLE_HOST\n");
			usbcfg.b.srpcap = 0;
			break;
		default:
			otg_err(OTG_DBG_OCI, "ERR> hwcfg2\n");
			break;
		}
		write_reg_32(GUSBCFG, usbcfg.d32);

		/* Program the GINTMSK register.*/
		gintmsk.b.modemismatch	= 1;
		gintmsk.b.sofintr	= 1;
		/*gintmsk.b.otgintr	= 1; */
		gintmsk.b.conidstschng	= 1;
		/*gintmsk.b.wkupintr	= 1;*/
		gintmsk.b.disconnect	= 1;
		/*gintmsk.b.usbsuspend	= 1;*/
		/*gintmsk.b.sessreqintr	= 1;*/
		/*gintmsk.b.portintr	= 1;*/
		/*gintmsk.b.hcintr		= 1; */
		write_reg_32(GINTMSK, gintmsk.d32);

		return USB_ERR_SUCCESS;

	} else {
		otg_err(OTG_DBG_OCI, "Core Reset FAIL\n");
		return USB_ERR_FAIL;
	}
}

/**
 * int oci_host_init(void)
 *
 * @brief Process host initialize as s5pc110 spec
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */

static int oci_host_init(void)
{
	gintmsk_t gintmsk = {.d32 = 0};
	hcfg_t hcfg = {.d32 = 0};
#if 0
/*#ifdef CONFIG_USB_S3C_OTG_HOST_DTGDRVVBUS*/
	hprt_t hprt;
	hprt.d32 = read_reg_32(HPRT);
#endif

	otg_dbg(OTG_DBG_OCI, "oci_host_init\n");

	gintmsk.b.portintr = 1;
	update_reg_32(GINTMSK, gintmsk.d32);

	hcfg.b.fslspclksel = HCFG_30_60_MHZ;
	update_reg_32(HCFG, hcfg.d32);

#if 0
/*#ifdef CONFIG_USB_S3C_OTG_HOST_DTGDRVVBUS*/
	/* turn on vbus */
	if (!hprt.b.prtpwr) {
		hprt.b.prtpwr = 1;
		write_reg_32(HPRT, hprt.d32);

		otg_dbg(true, "turn on Vbus\n");
	}
#endif

	oci_config_flush_fifo(OTG_HOST_MODE);

	return USB_ERR_SUCCESS;
}

/**
 * int oci_channel_init( u8 ch_num, struct stransfer *transfer)
 *
 * @brief Process channel initialize to prepare starting transfer
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
static int oci_channel_init(u8 ch_num, struct stransfer *transfer)
{
	u32	intr_enable = 0;
	gintmsk_t	gintmsk = {.d32 = 0};
	hcchar_t		hcchar = {.d32 = 0};
	hctsiz_t		hctsiz = {.d32 = 0};

	otg_dbg(OTG_DBG_OCI, "oci_channel_init\n");

	/* Clear channel information */
	write_reg_32(HCTSIZ(ch_num), 0);
	write_reg_32(HCCHAR(ch_num), 0);
	write_reg_32(HCINTMSK(ch_num), 0);
	write_reg_32(HCINT(ch_num), INT_ALL);/*write clear*/
	write_reg_32(HCDMA(ch_num), 0);

	/* enable host channel interrupt in GINTSTS */
	gintmsk.b.hcintr = 1;
	update_reg_32(GINTMSK, gintmsk.d32);
	/* Enable the top level host channel interrupt in HAINT */
	intr_enable = (1 << ch_num);
	update_reg_32(HAINTMSK, intr_enable);
	/* unmask the down level host channel interrupt in HCINT */
	write_reg_32(HCINTMSK(ch_num), transfer->hc_reg.hc_int_msk.d32);

	/* Program the HCSIZn register with the endpoint characteristics for */
	hctsiz.b.xfersize = transfer->buf_size;
	hctsiz.b.pktcnt = transfer->packet_cnt;

	/* Program the HCCHARn register with the endpoint characteristics for */
	hcchar.b.mps = transfer->ed_desc_p->max_packet_size;
	hcchar.b.epnum = transfer->ed_desc_p->endpoint_num;
	hcchar.b.epdir = transfer->ed_desc_p->is_ep_in;
	hcchar.b.lspddev = (transfer->ed_desc_p->dev_speed == LOW_SPEED_OTG);
	hcchar.b.eptype = transfer->ed_desc_p->endpoint_type;
	hcchar.b.multicnt = transfer->ed_desc_p->mc;
	hcchar.b.devaddr = transfer->ed_desc_p->device_addr;

	if (transfer->ed_desc_p->endpoint_type == INT_TRANSFER ||
		transfer->ed_desc_p->endpoint_type == ISOCH_TRANSFER) {
		u32 uiFrameNum = 0;
		uiFrameNum = oci_get_frame_num();

		hcchar.b.oddfrm = (uiFrameNum % 2) ? 1 : 0;

		/*
		 * if transfer type is periodic transfer,
		 * must support sof interrupt
		 */

		/*
		gintmsk.b.sofintr = 1;
		update_reg_32(GINTMSK, gintmsk.d32);
		*/
	}

	if (transfer->ed_desc_p->endpoint_type == CONTROL_TRANSFER) {

		struct td  *td_p;
		td_p = (struct td *)transfer->parent_td;

		switch (td_p->standard_dev_req_info.conrol_transfer_stage) {

		case SETUP_STAGE:
			hctsiz.b.pid = transfer->ed_status_p->
				control_data_toggle.setup;
			hcchar.b.epdir = EP_OUT;
			break;

		case DATA_STAGE:
			hctsiz.b.pid =
				transfer->ed_status_p->control_data_toggle.data;
			hcchar.b.epdir = transfer->ed_desc_p->is_ep_in;
			break;

		case STATUS_STAGE:
			hctsiz.b.pid = transfer->ed_status_p->
				control_data_toggle.status;

			if (td_p->standard_dev_req_info.is_data_stage) {
				hcchar.b.epdir =
					~(transfer->ed_desc_p->is_ep_in);
			} else
				hcchar.b.epdir = EP_IN;
			break;
		default:
			break;
		}

	} else
		hctsiz.b.pid = transfer->ed_status_p->data_toggle;

	otg_dbg(OTG_DBG_OCI, "eptype %d, stage %d, dir %d\n",
			transfer->ed_desc_p->endpoint_type,
			((struct td *)transfer->parent_td)->
				standard_dev_req_info.conrol_transfer_stage,
			hcchar.b.epdir);

	hctsiz.b.dopng = transfer->ed_status_p->is_ping_enable;
	write_reg_32(HCTSIZ(ch_num), hctsiz.d32);
	transfer->ed_status_p->is_ping_enable = false;

	/* Write DMA Address */
	write_reg_32(HCDMA(ch_num), transfer->phy_addr);

	/* Wrote HCCHAR Register */
	write_reg_32(HCCHAR(ch_num), hcchar.d32);

	return USB_ERR_SUCCESS;
}

static int oci_dev_init(void)
{
	otg_dbg(OTG_DBG_OCI, "oci_dev_init - do nothing.\n");
	/* return USB_ERR_FAIL; */
	return USB_ERR_SUCCESS;
}

static int oci_init_mode(void)
{
	gintsts_t	gintsts;
	gintsts.d32 = read_reg_32(GINTSTS);

	otg_dbg(OTG_DBG_OCI,
		"GINSTS = 0x%x,GINMSK = 0x%x.\n",
		(unsigned int)gintsts.d32,
		(unsigned int)read_reg_32(GINTMSK));

	if (gintsts.b.curmode == OTG_HOST_MODE) {
		otg_dbg(OTG_DBG_OCI, "HOST Mode\n");

		if (oci_host_init() == USB_ERR_SUCCESS) {
			return USB_ERR_SUCCESS;

		} else {
			otg_dbg(OTG_DBG_OCI, "oci_host_init() Fail\n");
			return USB_ERR_FAIL;
		}

	} else { /* Device Mode */
		otg_dbg(OTG_DBG_OCI, "DEVICE Mode\n");

		if (oci_dev_init() == USB_ERR_SUCCESS) {
			return USB_ERR_SUCCESS;

		} else {
			otg_err(OTG_DBG_OCI, "oci_dev_init() Fail\n");
			return USB_ERR_FAIL;
		}
	}

	return USB_ERR_SUCCESS;
}



/**
 * int oci_start(void)
 *
 * @brief start to operate oci module by calling oci_core_init function
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
static int oci_start(void)
{
	otg_dbg(OTG_DBG_OCI, "oci_start\n");

	if (oci_core_init() == USB_ERR_SUCCESS) {
		mdelay(50);

		if (oci_init_mode() == USB_ERR_SUCCESS) {
			oci_set_global_interrupt(true);
			return USB_ERR_SUCCESS;

		} else {
			otg_dbg(OTG_DBG_OCI, "oci_init_mode() Fail\n");
			return USB_ERR_FAIL;
		}

	} else {
		otg_dbg(OTG_DBG_OCI, "oci_core_init() Fail\n");
		return USB_ERR_FAIL;
	}
}

/**
 * int oci_stop(void)
 *
 * @brief stop to opearte otg core
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
static int oci_stop(void)
{
	gintmsk_t gintmsk = {.d32 = 0};

	otg_dbg(OTG_DBG_OCI, "oci_stop\n");

	/* sometimes, port interrupt occured after removed
	 * otg host driver. so, we have to mask port interrupt. */
	write_reg_32(GINTMSK, gintmsk.d32);

	oci_set_global_interrupt(false);

	return USB_ERR_SUCCESS;
}


static int oci_sys_init(struct sec_otghost *otghost)
{
	struct sec_otghost_data *pdata = otghost->otg_data;

	if (pdata && pdata->phy_init && pdata->pdev) {
		pr_info("otg phy_init\n");
		clk_enable(pdata->clk);
		pdata->phy_init(0);

		writel(0, OTG_PHYPWR);
		writel(0x87, OTG_PHYCLK);
	} else
		pr_info("otg phy_init failed\n");

	return USB_ERR_SUCCESS;
}

/**
 * int oci_init(struct sec_otghost *otghost)
 *
 * @brief Initialize oci module.
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
static int oci_init(struct sec_otghost *otghost)
{
	otg_mem_set((void *)ch_enable, true, sizeof(bool)*16);
	otghost->ch_halt = false;

	if (oci_sys_init(otghost) == USB_ERR_SUCCESS) {
		if (oci_core_reset() == USB_ERR_SUCCESS) {

			oci_set_global_interrupt(false);
			return USB_ERR_SUCCESS;

		} else {
			otg_dbg(OTG_DBG_OCI, "oci_core_reset() Fail\n");
			return USB_ERR_FAIL;
		}
	}

	return USB_ERR_FAIL;
}

/***********************************************************************
 * API
 */

/* also called by ischeduler.c */
int oci_channel_dealloc(u8 ch_num)
{
	if (ch_num < 16 && ch_enable[ch_num] == false) {
		ch_enable[ch_num] = true;

		write_reg_32(HCTSIZ(ch_num), 0);
		write_reg_32(HCCHAR(ch_num), 0);
		write_reg_32(HCINTMSK(ch_num), 0);
		write_reg_32(HCINT(ch_num), INT_ALL);
		write_reg_32(HCDMA(ch_num), 0);
		return USB_ERR_SUCCESS;
	}

	return USB_ERR_FAIL;
}

/**
 * oci_start_transfer(struct sec_otghost *otghost, struct stransfer *transfer)
 *
 * @brief start transfer by using transfer information to receive from scheduler
 *
 * @param [IN] *transfer - information about transfer to write register by calling oci_channel_init function
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */

u8 oci_start_transfer(struct sec_otghost *otghost, struct stransfer *transfer)
{
	hcchar_t hcchar = {.d32 = 0};

	otg_dbg(OTG_DBG_OCI, "oci_start_transfer\n");

	if (transfer->alloc_chnum == CH_NONE) {

		if (oci_channel_alloc(&(transfer->alloc_chnum))
				!= USB_ERR_SUCCESS) {

			otg_dbg(OTG_DBG_OCI,
				"Fail - Channel Allocation Error\n");
			return CH_NONE;
		}
	}

	oci_channel_init(transfer->alloc_chnum, transfer);

	hcchar.b.chen = 1;
	update_reg_32(HCCHAR(transfer->alloc_chnum), hcchar.d32);

	return transfer->alloc_chnum;
}

/**
 * int oci_stop_transfer(struct sec_otghost *otghost, u8 ch_num)
 *
 * @brief stop to transfer even if transfering
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
int oci_stop_transfer(struct sec_otghost *otghost, u8 ch_num)
{
	hcchar_t	hcchar = {.d32 = 0};
	hcintmsk_t	hcintmsk = {.d32 = 0};
	int count = 0, max_error_count = 10000;

	otg_dbg(OTG_DBG_OCI,
		"step1: oci_stop_transfer ch=%d, hcchar=0x%x\n",
			ch_num, read_reg_32(HCCHAR(ch_num)));

	if (ch_num > 16)
		return USB_ERR_FAIL;

	otghost->ch_halt = true;

	hcintmsk.b.chhltd = 1;
	update_reg_32(HCINTMSK(ch_num), hcintmsk.d32);

	hcchar.b.chdis = 1;
	hcchar.b.chen = 1;
	update_reg_32(HCCHAR(ch_num), hcchar.d32);

	/* wait for  Channel Disabled Interrupt */
	do {
		hcchar.d32 = read_reg_32(HCCHAR(ch_num));

		if (count > max_error_count) {
			otg_dbg(OTG_DBG_OCI,
				"Warning!! oci_stop_transfer()"
				"ChDis is not cleared! ch=%d, hcchar=0x%x\n",
				ch_num, hcchar.d32);
			break;
		}
		count++;

	} while (hcchar.b.chdis);

	oci_channel_dealloc(ch_num);

	clear_reg_32(HAINTMSK, ch_num);
	write_reg_32(HCINT(ch_num), INT_ALL);
	clear_reg_32(HCINTMSK(ch_num), INT_ALL);

	otghost->ch_halt = false;
	otg_dbg(OTG_DBG_OCI,
		"step2 : oci_stop_transfer ch=%d, hcchar=0x%x\n",
			ch_num, read_reg_32(HCCHAR(ch_num)));

	return USB_ERR_SUCCESS;
}

/**
 * u32 oci_get_frame_num(void)
 *
 * @brief Get current frame number by reading register.
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
u32 oci_get_frame_num(void)
{
	hfnum_t hfnum;
	hfnum.d32 = read_reg_32(HFNUM);

	otg_dbg(OTG_DBG_OCI, "oci_get_frame_num=%d\n", hfnum.b.frnum);

	return hfnum.b.frnum;
}

/**
 * u16 oci_get_frame_interval(void)
 *
 * @brief Get current frame interval by reading register.
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 * @remark
 *
 */
u16 oci_get_frame_interval(void)
{
	hfir_t hfir;
	hfir.d32 = read_reg_32(HFIR);
	return hfir.b.frint;
}

void oci_set_frame_interval(u16 interval)
{
	hfir_t hfir = {.d32 = 0};
	hfir.b.frint = interval;
	write_reg_32(HFIR, hfir.d32);
}

