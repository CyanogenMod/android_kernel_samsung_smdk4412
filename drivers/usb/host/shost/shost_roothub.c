/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : RootHub.c
 *  [Description] : The file implement the external
 *		and internal functions of RootHub
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2009/02/10
 *  [Revision History]
 *	  (1) 2008/06/13   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Created this file and implements functions of RootHub
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


/**
 * void bus_suspend(void)
 *
 * @brief Make suspend status when this platform support PM Mode
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */
static void bus_suspend(void)
{
	hprt_t	hprt;
	pcgcctl_t	pcgcctl;

	otg_dbg(OTG_DBG_ROOTHUB, " bus_suspend\n");

	hprt.d32 = 0;
	pcgcctl.d32 = 0;

	hprt.b.prtsusp = 1;
	update_reg_32(HPRT, hprt.d32);

	pcgcctl.b.pwrclmp = 1;
	update_reg_32(PCGCCTL, pcgcctl.d32);
	udelay(1);

	pcgcctl.b.rstpdwnmodule = 1;
	update_reg_32(PCGCCTL, pcgcctl.d32);
	udelay(1);

	pcgcctl.b.stoppclk = 1;
	update_reg_32(PCGCCTL, pcgcctl.d32);
	udelay(1);
}

/**
 * int bus_resume(struct sec_otghost *otghost)
 *
 * @brief Make resume status when this platform support PM Mode
 *
 * @param None
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 *
 * @remark
 *
 */
static int bus_resume(struct sec_otghost *otghost)
{
	/*
	hprt_t	hprt;
	pcgcctl_t	pcgcctl;
	hprt.d32 = 0;
	pcgcctl.d32 = 0;

	pcgcctl.b.stoppclk = 1;
	clear_reg_32(PCGCCTL,pcgcctl.d32);
	udelay(1);

	pcgcctl.b.pwrclmp = 1;
	clear_reg_32(PCGCCTL,pcgcctl.d32);
	udelay(1);

	pcgcctl.b.rstpdwnmodule = 1;
	clear_reg_32(PCGCCTL,pcgcctl.d32);
	udelay(1);

	hprt.b.prtres = 1;
	update_reg_32(HPRT, hprt.d32);
	mdelay(20);

	clear_reg_32(HPRT, hprt.d32);
	*/
	otg_dbg(OTG_DBG_ROOTHUB, "bus_resume()......\n");
	otg_dbg(OTG_DBG_ROOTHUB, "wait for 50 ms...\n");

	mdelay(50);

	if (oci_init(otghost) == USB_ERR_SUCCESS) {
		if (oci_start() == USB_ERR_SUCCESS) {
			otg_dbg(OTG_DBG_ROOTHUB, "OTG Init Success\n");
			return USB_ERR_SUCCESS;
		}
	}

	return USB_ERR_FAIL;
}

static void setPortPower(bool on)
{
#ifdef CONFIG_USB_S3C_OTG_HOST_DTGDRVVBUS
	hprt_t hprt = {.d32 = 0};

	if (on) {
		hprt.d32 = read_reg_32(HPRT);

		if (!hprt.b.prtpwr) {
			/*hprt.d32 = 0;*/
			hprt.b.prtpwr = 1;
			write_reg_32(HPRT, hprt.d32);
		}
	} else {
		hprt.b.prtpwr = 1;
		clear_reg_32(HPRT, hprt.d32);
	}
#else
	otg_dbg(true, "turn %s Vbus\n", on ? "on" : "off");
#endif
}

/**
 * int get_otg_port_status(const u8 port, char* status)
 *
 * @brief Get port change bitmap information
 *
 * @param [IN] port : port number
 *	   [OUT] status : buffer to store bitmap information
 *
 * @returnUSB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 *
 * @remark
 *
 */
static inline int get_otg_port_status(
		struct usb_hcd *hcd, const u8 port, char *status)
{

	struct sec_otghost *otghost = hcd_to_sec_otghost(hcd);
	/* return root_hub_feature(port, GetPortStatus, NULL, status); */
#if 0
 /* for debug */
	hprt_t hprt;

	hprt.d32 = read_reg_32(HPRT);

	otg_dbg(OTG_DBG_ROOTHUB,
		"HPRT:spd=%d,pwr=%d,lnsts=%d,rst=%d,susp=%d,res=%d,"
		"ovrcurract=%d,ena=%d,connsts=%d\n",
		    hprt.b.prtspd,
		    hprt.b.prtpwr,
		    hprt.b.prtlnsts,
		    hprt.b.prtrst,
		    hprt.b.prtsusp,
		    hprt.b.prtres,
		    hprt.b.prtovrcurract,
		    hprt.b.prtena,
		    hprt.b.prtconnsts);

#endif
	status[port] = 0;
	status[port] |= (otghost->port_flag.b.port_connect_status_change ||
			otghost->port_flag.b.port_reset_change ||
			otghost->port_flag.b.port_enable_change ||
			otghost->port_flag.b.port_suspend_change ||
			otghost->port_flag.b.port_over_current_change) << 1;

#if 0
 /* for debug */
	otg_dbg(OTG_DBG_ROOTHUB,
		"connect:%d,reset:%d,enable:%d,suspend:%d,over_current:%d\n",
		    otghost->port_flag.b.port_connect_status_change,
		    otghost->port_flag.b.port_reset_change,
		    otghost->port_flag.b.port_enable_change,
		    otghost->port_flag.b.port_suspend_change,
		    otghost->port_flag.b.port_over_current_change);
#endif

	if (status[port]) {
		otg_dbg(OTG_DBG_ROOTHUB,
			" Root port status changed\n");
		otg_dbg(OTG_DBG_ROOTHUB,
			"  port_connect_status_change: %d\n",
			    otghost->port_flag.b.port_connect_status_change);
		otg_dbg(OTG_DBG_ROOTHUB,
			"  port_reset_change: %d\n",
			    otghost->port_flag.b.port_reset_change);
		otg_dbg(OTG_DBG_ROOTHUB,
			"  port_enable_change: %d\n",
			    otghost->port_flag.b.port_enable_change);
		otg_dbg(OTG_DBG_ROOTHUB,
			"  port_suspend_change: %d\n",
			    otghost->port_flag.b.port_suspend_change);
		otg_dbg(OTG_DBG_ROOTHUB,
			"  port_over_current_change: %d\n",
			    otghost->port_flag.b.port_over_current_change);
	}

	return (status[port] != 0);
}

/**
 * int reset_and_enable_port(struct usb_hcd *hcd, const u8 port)
 *
 * @brief Reset port and make enable status the specific port
 *
 * @param [IN] port : port number
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 *
 * @remark
 *
 */
static int reset_and_enable_port(struct usb_hcd *hcd, const u8 port)
{
	hprt_t hprt;
	u32 count = 0;
	u32 max_error_count = 10000;
	struct sec_otghost *otghost = hcd_to_sec_otghost(hcd);

	hprt.d32 = read_reg_32(HPRT);

	otg_dbg(OTG_DBG_ROOTHUB,
		" reset_and_enable_port\n");

	if (hprt.b.prtconnsts == 0) {
		otg_dbg(OTG_DBG_ROOTHUB,
				"No Attached Device, HPRT = 0x%x\n", hprt.d32);

		otghost->port_flag.b.port_connect_status_change = 1;
		otghost->port_flag.b.port_connect_status = 0;

		return USB_ERR_FAIL;
	}

	if (!hprt.b.prtena) {
		hprt.b.prtrst = 1; /* drive reset */
		write_reg_32(HPRT, hprt.d32);

		mdelay(60);
		hprt.b.prtrst = 0;
		write_reg_32(HPRT, hprt.d32);

		do {
			hprt.d32 = read_reg_32(HPRT);

			if (count > max_error_count) {
				otg_dbg(OTG_DBG_ROOTHUB,
				"Port Reset Fail : HPRT : 0x%x\n", hprt.d32);
				return USB_ERR_FAIL;
			}
			count++;

		} while (!hprt.b.prtena);

	}
	return USB_ERR_SUCCESS;
}

/**
 * int root_hub_feature(
 *		struct usb_hcd *hcd,
 *		const u8 port,
 *		const u16 type_req,
 *		const u16 feature,
 *		void* buf)
 *
 * @brief Get port change bitmap information
 *
 * @param [IN] port : port number
 *	   [IN] type_req : request type of hub feature as usb 2.0 spec
 *	   [IN] feature : hub feature as usb 2.0 spec
 *	   [OUT] status : buffer to store results
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If call fail \n
 *
 * @remark
 *
 */
static inline int root_hub_feature(
		struct usb_hcd *hcd,
		const u8 port,
		const u16 type_req,
		const u16 feature,
		void *buf)
{
	int retval = USB_ERR_SUCCESS;
	struct hub_descriptor *desc = NULL;
	u32 port_status = 0;
	hprt_t hprt = {.d32 = 0};
	struct sec_otghost *otghost = hcd_to_sec_otghost(hcd);

	otg_dbg(OTG_DBG_ROOTHUB, " root_hub_feature\n");

	switch (type_req) {
	case ClearHubFeature:
		otg_dbg(OTG_DBG_ROOTHUB, "case ClearHubFeature\n");
		switch (feature) {
		case C_HUB_LOCAL_POWER:
			otg_dbg(OTG_DBG_ROOTHUB,
				"case ClearHubFeature -C_HUB_LOCAL_POWER\n");
			break;
		case C_HUB_OVER_CURRENT:
			otg_dbg(OTG_DBG_ROOTHUB,
				"case ClearHubFeature -C_HUB_OVER_CURRENT\n");
			/* Nothing required here */
			break;
		default:
			retval = USB_ERR_FAIL;
		}
		break;

	case ClearPortFeature:
		otg_dbg(OTG_DBG_ROOTHUB, "case ClearPortFeature\n");
		switch (feature) {
		case USB_PORT_FEAT_ENABLE:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_ENABLE\n");
			hprt.b.prtena = 1;
			update_reg_32(HPRT, hprt.d32);
			break;

		case USB_PORT_FEAT_SUSPEND:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_SUSPEND\n");
			bus_resume(otghost);
			break;

		case USB_PORT_FEAT_POWER:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_POWER\n");
			setPortPower(false);
			break;

		case USB_PORT_FEAT_INDICATOR:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_INDICATOR\n");
			/* Port inidicator not supported */
			break;

		case USB_PORT_FEAT_C_CONNECTION:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_C_CONNECTION\n");
			/* Clears drivers internal connect status change flag */
			otghost->port_flag.b.port_connect_status_change = 0;
			break;

		case USB_PORT_FEAT_C_RESET:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_C_RESET\n");
			/* Clears the driver's internal Port Reset Change flag*/
			otghost->port_flag.b.port_reset_change = 0;
			break;

		case USB_PORT_FEAT_C_ENABLE:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_C_ENABLE\n");
			/* Clears Port Enable/Disable Change flag */
			otghost->port_flag.b.port_enable_change = 0;
			break;

		case USB_PORT_FEAT_C_SUSPEND:
			otg_dbg(OTG_DBG_ROOTHUB,
			"case ClearPortFeature -USB_PORT_FEAT_C_SUSPEND\n");
			/* Clears the driver's internal Port Suspend
			 * Change flag, which is set when resume signaling on
			 * the host port is complete */
			otghost->port_flag.b.port_suspend_change = 0;
			break;

		case USB_PORT_FEAT_C_OVER_CURRENT:
			otg_dbg(OTG_DBG_ROOTHUB,
				"case ClearPortFeature - USB_PORT_FEAT_C_OVER_CURRENT\n");
			otghost->port_flag.b.port_over_current_change = 0;
			break;

		default:
			retval = USB_ERR_FAIL;
			otg_dbg(OTG_DBG_ROOTHUB,
				"case ClearPortFeature - FAIL\n");
		}
		break;

	case GetHubDescriptor:
		otg_dbg(OTG_DBG_ROOTHUB, "case GetHubDescriptor\n");
		desc = (struct hub_descriptor *)buf;
		desc->desc_length = 9;
		desc->desc_type = 0x29;
		desc->port_number = 1;
		desc->hub_characteristics = 0x08;
		desc->power_on_to_power_good = 1;
		desc->hub_control_current = 0;
		desc->DeviceRemovable[0] = 0;
		desc->DeviceRemovable[1] = 0xff;
		break;

	case GetHubStatus:
		otg_dbg(OTG_DBG_ROOTHUB, "case GetHubStatus\n");
		otg_mem_set(buf, 0, 4);
		break;

	case GetPortStatus:
		otg_dbg(OTG_DBG_ROOTHUB, "case GetPortStatus\n");

		if (otghost->port_flag.b.port_connect_status_change) {
			port_status |= (1 << USB_PORT_FEAT_C_CONNECTION);
			otg_dbg(OTG_DBG_ROOTHUB,
			"case GetPortStatus - USB_PORT_FEAT_C_CONNECTION\n");
		}

		if (otghost->port_flag.b.port_enable_change) {
			port_status |= (1 << USB_PORT_FEAT_C_ENABLE);
			otg_dbg(OTG_DBG_ROOTHUB,
			"case GetPortStatus - USB_PORT_FEAT_C_ENABLE\n");
		}

		if (otghost->port_flag.b.port_suspend_change) {
			port_status |= (1 << USB_PORT_FEAT_C_SUSPEND);
			otg_dbg(OTG_DBG_ROOTHUB,
			"case GetPortStatus - USB_PORT_FEAT_C_SUSPEND\n");
		}

		if (otghost->port_flag.b.port_reset_change) {
			port_status |= (1 << USB_PORT_FEAT_C_RESET);
			otg_dbg(OTG_DBG_ROOTHUB,
			"case GetPortStatus - USB_PORT_FEAT_C_RESET\n");
		}

		if (otghost->port_flag.b.port_over_current_change) {
			port_status |= (1 << USB_PORT_FEAT_C_OVER_CURRENT);
			otg_dbg(OTG_DBG_ROOTHUB,
			"case GetPortStatus - USB_PORT_FEAT_C_OVER_CURRENT\n");
		}

		if (!otghost->port_flag.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return 0's for the remainder of the port status
			 * since the port register can't be read if the core
			 * is in device mode.
			 */
			otg_dbg(OTG_DBG_ROOTHUB,
					"case GetPortStatus - disconnected\n");

			*((__le32 *)buf) = cpu_to_le32(port_status);
			/*break;*/
		}

		hprt.d32 = read_reg_32(HPRT);

		if (hprt.b.prtconnsts)
			port_status |= (1 << USB_PORT_FEAT_CONNECTION);

		if (hprt.b.prtena)
			port_status |= (1 << USB_PORT_FEAT_ENABLE);

		if (hprt.b.prtsusp)
			port_status |= (1 << USB_PORT_FEAT_SUSPEND);

		if (hprt.b.prtovrcurract)
			port_status |= (1 << USB_PORT_FEAT_OVER_CURRENT);

		if (hprt.b.prtrst)
			port_status |= (1 << USB_PORT_FEAT_RESET);

		if (hprt.b.prtpwr)
			port_status |= (1 << USB_PORT_FEAT_POWER);

		if (hprt.b.prtspd == 0) {
			port_status |=  USB_PORT_STAT_HIGH_SPEED;
		} else {
			if (hprt.b.prtspd == 2)
				port_status |= USB_PORT_STAT_LOW_SPEED;
		}

		if (hprt.b.prttstctl)
			port_status |= (1 << USB_PORT_FEAT_TEST);

		*((__le32 *)buf) = cpu_to_le32(port_status);

		otg_dbg(OTG_DBG_ROOTHUB, "port_status=0x%x.\n", port_status);
		break;

	case SetHubFeature:
		otg_dbg(OTG_DBG_ROOTHUB, "case SetHubFeature\n");
		/* No HUB features supported */
		break;

	case SetPortFeature:
		otg_dbg(OTG_DBG_ROOTHUB, "case SetPortFeature\n");
		if (!otghost->port_flag.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return without doing anything since the port
			 * register can't be written if the core is in device
			 * mode.
			 */
			otg_dbg(OTG_DBG_ROOTHUB,
				"case SetPortFeature - disconnected\n");

			/*break;*/
		}

		switch (feature) {
		case USB_PORT_FEAT_SUSPEND:
			otg_dbg(true,
			"case SetPortFeature -USB_PORT_FEAT_SUSPEND\n");
			bus_suspend();
			break;

		case USB_PORT_FEAT_POWER:
			otg_dbg(true,
			"case SetPortFeature -USB_PORT_FEAT_POWER\n");
			setPortPower(true);
			break;

		case USB_PORT_FEAT_RESET:
			otg_dbg(true,
			"case SetPortFeature -USB_PORT_FEAT_RESET\n");
			retval = reset_and_enable_port(hcd, port);
			/*
			if(retval == USB_ERR_SUCCESS)
				wake_lock(&otghost->wake_lock);
			*/
			break;

		case USB_PORT_FEAT_INDICATOR:
			otg_dbg(true,
			"case SetPortFeature -USB_PORT_FEAT_INDICATOR\n");
			break;

		default:
			otg_dbg(true, "case SetPortFeature -USB_ERR_FAIL\n");
			retval = USB_ERR_FAIL;
			break;
		}
		break;

	default:
		retval = USB_ERR_FAIL;
		otg_err(true, "root_hub_feature() Function Error\n");
		break;
	}

	if (retval != USB_ERR_SUCCESS)
		retval = USB_ERR_FAIL;

	return retval;
}

