# Makefile of modem_v2

EXTRA_CFLAGS += -Idrivers/misc/modem_v2

obj-y += sipc5_modem.o sipc5_io_device.o
obj-y += sipc4_modem.o sipc4_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o

obj-$(CONFIG_UMTS_MODEM_XMM6260) += modem_modemctl_device_xmm6260.o
obj-$(CONFIG_UMTS_MODEM_XMM6262) += modem_modemctl_device_xmm6262.o
obj-$(CONFIG_UMTS_MODEM_XMM6360) += modem_modemctl_device_xmm6262.o
obj-$(CONFIG_GSM_MODEM_ESC6270) += modem_modemctl_device_esc6270.o
obj-$(CONFIG_CDMA_MODEM_MDM6600) += modem_modemctl_device_mdm6600.o

obj-$(CONFIG_LINK_DEVICE_PLD) += modem_link_device_pld.o modem_link_device_pld_ext_op.o
obj-$(CONFIG_LINK_DEVICE_HSIC) += modem_link_device_hsic_ncm.o link_usb_cdc_ncm.o
