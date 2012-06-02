#ifndef _SEC_MODEM_H_
#define _SEC_MODEM_H_

enum hsic_lpa_states {
	STATE_HSIC_LPA_ENTER,
	STATE_HSIC_LPA_WAKE,
	STATE_HSIC_LPA_PHY_INIT,
};

#if defined(CONFIG_LINK_DEVICE_HSIC) || defined(CONFIG_LINK_DEVICE_USB)
void set_host_states(struct platform_device *pdev, int type);
void set_hsic_lpa_states(int states);
int get_cp_active_state(void);
#else
#define set_hsic_lpa_states(states) do {} while (0);
#endif

#endif
