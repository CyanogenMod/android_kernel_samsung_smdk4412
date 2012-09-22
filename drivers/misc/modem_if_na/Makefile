# Makefile of modem_if

obj-y += modem.o modem_io_device.o modem_net_flowcontrol_device.o

obj-$(CONFIG_CDMA_MODEM_CBP71) += modem_modemctl_device_cbp71.o
obj-$(CONFIG_LTE_MODEM_CMC220) += modem_modemctl_device_cmc220.o lte_modem_bootloader.o
obj-$(CONFIG_LINK_DEVICE_DPRAM) += modem_link_device_dpram.o
obj-$(CONFIG_LINK_DEVICE_USB) += modem_link_device_usb.o modem_link_pm_usb.o
