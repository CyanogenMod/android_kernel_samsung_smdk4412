# Makefile of modem_if

obj-y += modem.o modem_io_device.o modem_net_flowcontrol_device.o \
	dpram_recovery.o

obj-$(CONFIG_CDMA_MODEM_QSC6085) += modem_modemctl_device_qsc6085.o

obj-$(CONFIG_LINK_DEVICE_DPRAM) += modem_link_device_dpram.o
