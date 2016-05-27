# Makefile of modem_if

EXTRA_CFLAGS += -Idrivers/misc/modem_if

obj-y += sipc5_modem.o sipc5_io_device.o sipc5_common.o
obj-y += sipc4_modem.o sipc4_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o

obj-$(CONFIG_UMTS_MODEM_XMM6260) += modem_modemctl_device_xmm6260.o
obj-$(CONFIG_UMTS_MODEM_XMM6262) += modem_modemctl_device_xmm6262.o
obj-$(CONFIG_CDMA_MODEM_CBP71) += modem_modemctl_device_cbp71.o
obj-$(CONFIG_CDMA_MODEM_CBP72) += modem_modemctl_device_cbp72.o
obj-$(CONFIG_CDMA_MODEM_CBP82) += modem_modemctl_device_cbp82.o
obj-$(CONFIG_LTE_MODEM_CMC221) += modem_modemctl_device_cmc221.o
obj-$(CONFIG_UMTS_MODEM_SS222) += modem_modemctl_device_ss222.o
obj-$(CONFIG_CDMA_MODEM_MDM6600) += modem_modemctl_device_mdm6600.o
obj-$(CONFIG_GSM_MODEM_ESC6270) += modem_modemctl_device_esc6270.o
obj-$(CONFIG_CDMA_MODEM_QSC6085) += modem_modemctl_device_qsc6085.o
obj-$(CONFIG_TDSCDMA_MODEM_SPRD8803) += modem_modemctl_device_sprd8803.o

obj-$(CONFIG_LINK_DEVICE_MIPI) += modem_link_device_mipi.o
obj-$(CONFIG_LINK_DEVICE_USB) += modem_link_device_usb.o modem_link_pm_usb.o
obj-$(CONFIG_LINK_DEVICE_HSIC) += modem_link_device_hsic.o
obj-$(CONFIG_LINK_DEVICE_DPRAM) += modem_link_device_dpram.o modem_link_device_dpram_ext_op.o
obj-$(CONFIG_LINK_DEVICE_PLD) += modem_link_device_pld.o modem_link_device_pld_ext_op.o
obj-$(CONFIG_LINK_DEVICE_C2C) += modem_link_device_c2c.o
obj-$(CONFIG_LINK_DEVICE_SPI) += modem_link_device_spi.o

obj-$(CONFIG_BOOT_DEVICE_SPI) += modem_boot_device_spi.o

obj-$(CONFIG_SIM_SLOT_SWITCH)  += modem_sim_slot_switch.o

# Check whether or not memory-type interface
ifeq ($(CONFIG_LINK_DEVICE_DPRAM),y)
LINK_DEVICE_MEMORY_INTERFACE=y
endif

ifeq ($(CONFIG_LINK_DEVICE_PLD),y)
LINK_DEVICE_MEMORY_INTERFACE=y
endif

ifeq ($(CONFIG_LINK_DEVICE_C2C),y)
LINK_DEVICE_MEMORY_INTERFACE=y
endif

ifeq ($(CONFIG_LINK_DEVICE_SHMEM),y)
LINK_DEVICE_MEMORY_INTERFACE=y
endif

ifdef LINK_DEVICE_MEMORY_INTERFACE
obj-y += modem_link_device_memory.o
endif
