# arch/arm/mach-exynos/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

ifneq ($(CONFIG_CMA),y)
obj-y				:= reserve_mem-exynos4.o
endif
obj-m				:=
obj-n				:=
obj-				:=

# Core support for EXYNOS system

obj-y				+= init.o irq-combiner.o dma.o irq-eint.o ppmu.o
obj-$(CONFIG_ARM_TRUSTZONE)	+= irq-sgi.o
obj-$(CONFIG_ARCH_EXYNOS4)	+= exynos4-smc.o cpu-exynos4.o clock-exynos4.o pmu-exynos4.o ppc.o

CFLAGS_exynos4-smc.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)
obj-$(CONFIG_ARCH_EXYNOS5)	+= cpu-exynos5.o clock-exynos5.o pmu-exynos5.o

obj-y				+= clock-domain.o
obj-$(CONFIG_CPU_EXYNOS4210)	+= clock-exynos4210.o
obj-$(CONFIG_CPU_EXYNOS4212)	+= clock-exynos4212.o
obj-$(CONFIG_EXYNOS4_PM)	+= pm-exynos4.o sleep-exynos4.o
AFLAGS_sleep-exynos4.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

obj-$(CONFIG_EXYNOS5_PM)	+= pm-exynos5.o sleep-exynos5.o
obj-$(CONFIG_CPU_FREQ)		+= cpufreq.o
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= asv.o
ifeq ($(CONFIG_CPU_EXYNOS4210),y)
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= asv-4210.o
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= cpufreq-4210.o
else
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= asv-4x12.o
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= cpufreq-4x12.o
endif

obj-$(CONFIG_EXYNOS5_CPUFREQ)	+= asv.o asv-5250.o
obj-$(CONFIG_EXYNOS5_CPUFREQ)	+= cpufreq-5250.o
obj-$(CONFIG_EXYNOS4_CPUIDLE)	+= cpuidle-exynos4.o idle-exynos4.o
AFLAGS_idle-exynos4.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

obj-$(CONFIG_EXYNOS5_CPUIDLE)	+= cpuidle-exynos5.o idle-exynos5.o

obj-$(CONFIG_BUSFREQ)		+= busfreq.o
obj-$(CONFIG_BUSFREQ_OPP)	+= dev.o
ifeq ($(CONFIG_BUSFREQ_OPP),y)
obj-$(CONFIG_ARCH_EXYNOS4)	+= busfreq_opp_exynos4.o busfreq_opp_4x12.o
obj-$(CONFIG_ARCH_EXYNOS5)	+= busfreq_opp_exynos5.o busfreq_opp_5250.o
endif
obj-$(CONFIG_DISPFREQ_OPP)	+= dispfreq_opp_exynos4.o
obj-$(CONFIG_BUSFREQ_LOCK_WRAPPER)	+= dev-slp.o

obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o

obj-$(CONFIG_EXYNOS_MCT)	+= mct.o

obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o

obj-$(CONFIG_STAND_ALONE_POLICY)	+= stand-hotplug.o
obj-$(CONFIG_LEGACY_HOTPLUG_POLICY)	+= pm-hotplug.o
obj-$(CONFIG_WITH_DVFS_POLICY)		+= dvfs-hotplug.o
obj-$(CONFIG_DVFS_NR_RUNNING_POLICY)	+= dynamic-dvfs-nr_running-hotplug.o
obj-$(CONFIG_NR_RUNNING_POLICY)		+= dynamic-nr_running-hotplug.o

# machine support

obj-$(CONFIG_MACH_SMDKC210)		+= mach-smdkv310.o
obj-$(CONFIG_MACH_SMDKV310)		+= mach-smdkv310.o
obj-$(CONFIG_MACH_ARMLEX4210)		+= mach-armlex4210.o
obj-$(CONFIG_MACH_UNIVERSAL_C210)	+= mach-universal_c210.o
obj-$(CONFIG_MACH_TRATS)		+= board-trats.o sec-common.o sec-switch_max8997.o \
					   u1-wlan.o u1_regulator_consumer.o sec-reboot.o
obj-$(CONFIG_MACH_U1)			+= mach-u1.o sec-common.o sec-switch_max8997.o
obj-$(CONFIG_WIMAX_CMC)			+= u1-wimax.o
ifneq ($(CONFIG_MACH_U1_NA_SPR),y)
ifneq ($(CONFIG_MACH_U1_NA_USCC),y)
obj-$(CONFIG_MACH_U1_BD)		+= u1-gpio.o
else
obj-$(CONFIG_MACH_U1_BD)		+= u1-gpio.o
endif
else
obj-$(CONFIG_MACH_U1_BD)		+= u1-gpio.o
endif
obj-$(CONFIG_MACH_Q1_BD)		+= q1-gpio.o
obj-$(CONFIG_MACH_U1)			+= u1-wlan.o
obj-$(CONFIG_MACH_PX)			+= mach-px.o sec-common.o px-switch.o
obj-$(CONFIG_MACH_P2)			+= p2-gpio.o
obj-$(CONFIG_MACH_P4)			+= p4-gpio.o
obj-$(CONFIG_MACH_P8)			+= p8-gpio.o
obj-$(CONFIG_MACH_P8LTE)		+= p8lte-gpio.o
obj-$(CONFIG_MACH_P10)			+= board-p10-wlan.o
obj-$(CONFIG_PANEL_U1)			+= u1-panel.o u1-panel_a2.o u1-panel_m2.o
obj-$(CONFIG_PANEL_U1_NA_SPR)           += u1-na-spr-panel.o
obj-$(CONFIG_MACH_U1)			+= u1_regulator_consumer.o
obj-$(CONFIG_MACH_PX)			+= u1_regulator_consumer.o
obj-$(CONFIG_MACH_NURI)			+= mach-nuri.o
obj-$(CONFIG_MACH_SMDK4X12)		+= mach-smdk4x12.o
ifeq ($(CONFIG_MACH_P4NOTE),y)
obj-$(CONFIG_MACH_MIDAS)		+= mach-p4notepq.o px-switch.o p4note-jack.o
else
ifeq ($(CONFIG_MACH_TAB3),y)
ifeq ($(CONFIG_TAB3_00_BD),y)
obj-$(CONFIG_MACH_MIDAS)		+= mach-tab3.o px-switch.o tab3-jack.o board-tab3-battery.o \
								   midas-thermistor.o
else
obj-$(CONFIG_MACH_MIDAS)		+= mach-tab3.o tab3-jack.o board-slim8-battery.o midas-thermistor.o
endif
else
ifeq ($(CONFIG_MACH_KONA),y)
ifeq ($(CONFIG_KONA_00_BD),y)
obj-$(CONFIG_MACH_MIDAS)               += mach-kona.o px-switch.o board-kona-battery.o
else
obj-$(CONFIG_MACH_MIDAS)               += mach-kona.o
endif
else
ifeq ($(CONFIG_MACH_IRON),y)
obj-$(CONFIG_MACH_MIDAS)		+= board-grande.o grande-jack.o
else
ifeq ($(CONFIG_MACH_GRANDE),y)
obj-$(CONFIG_MACH_MIDAS)		+= board-grande.o grande-jack.o
else
ifeq ($(CONFIG_MACH_SP7160LTE),y)
obj-$(CONFIG_MACH_MIDAS)		+= mach-sp7160lte.o sp7160lte-switch.o sp7160lte-jack.o
else
obj-$(CONFIG_MACH_MIDAS)		+= mach-midas.o
endif
endif
endif
endif
endif
endif

obj-$(CONFIG_CHARGER_MANAGER)		+= charger-slp.o
obj-$(CONFIG_MACH_SMDK5210)		+= mach-smdk5210.o
obj-$(CONFIG_MACH_SMDK5250)		+= mach-smdk5250.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-mmc.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-display.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-power.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-audio.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-usb.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-input.o
obj-$(CONFIG_MACH_SMDK5250)		+= board-smdk5250-spi.o

obj-$(CONFIG_MACH_P4NOTE)		+= p4-input.o
obj-$(CONFIG_MACH_TAB3)			+= tab3-input.o
obj-$(CONFIG_MACH_KONA)			+= kona-input.o
obj-$(CONFIG_MACH_SP7160LTE)		+= sp7160lte-input.o

ifeq ($(CONFIG_MACH_WATCH),y)
obj-$(CONFIG_BATTERY_SAMSUNG_P1X)	+= board-watch-battery.o
endif

ifeq ($(CONFIG_MACH_GRANDE),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   board-grande-tsp.o board-midas-wlan.o \
					   board-grande-camera.o board-grande-thermistor.o \
					   board-grande-lcd.o
else
ifeq ($(CONFIG_MACH_IRON),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   board-grande-tsp.o board-midas-wlan.o \
					   board-grande-camera.o board-grande-thermistor.o \
					   board-grande-lcd.o
else
ifeq ($(CONFIG_MACH_M3),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o board-lcd-mipi.o
else
ifeq ($(CONFIG_MACH_T0),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)		+= board-lcd-mipi.o
else
ifeq ($(CONFIG_MACH_P4NOTE),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o\
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o
obj-$(CONFIG_FB_S5P)			+= board-lcd-rgb.o
else
ifeq ($(CONFIG_MACH_TAB3),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o\
					   board-midas-wlan.o \
					   midas-camera.o \
					   midas-mhl.o
obj-$(CONFIG_FB_S5P_S6C1372)		+= board-lcd-rgb.o
obj-$(CONFIG_FB_S5P_S6D7AA0)		+= board-lcd-mipi.o
else
ifeq ($(CONFIG_MACH_KONA),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o\
					   board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o \
					   midas-wacom.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)		+= board-lcd-mipi.o
else
ifeq ($(CONFIG_MACH_GD2),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)		+= board-lcd-mipi.o
obj-$(CONFIG_FB_S5P_GD2EVF)		+= board-lcd-evf.o
else
ifeq ($(CONFIG_MACH_SP7160LTE),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o\
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o
obj-$(CONFIG_FB_S5P)			+= board-lcd-rgb.o
else
ifeq ($(CONFIG_MACH_IPCAM),y)
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   board-midas-wlan.o \
					   midas-tsp.o \
					   midas-camera.o \
					   midas-thermistor.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)		+= board-lcd-mipi.o
else
obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o \
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o

obj-$(CONFIG_FB_S5P_MIPI_DSIM)		+= board-lcd-mipi.o
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

obj-$(CONFIG_IRON_BD)			+= board-iron-gpio.o board-grande-power.o
obj-$(CONFIG_GRANDE_BD)			+= board-grande-gpio.o board-grande-power.o
obj-$(CONFIG_P4NOTE_00_BD)		+= p4note-gpio.o p4note-power.o
obj-$(CONFIG_TAB3_00_BD)		+= tab3-gpio.o tab3-power.o
obj-$(CONFIG_TAB3_01_BD)		+= tab3-gpio.o tab3-power.o
obj-$(CONFIG_KONA_00_BD)		+= kona-gpio.o kona-power.o
obj-$(CONFIG_KONA_01_BD)		+= kona-01-gpio.o kona-01-power.o
ifeq ($(CONFIG_MACH_SF2),y)
obj-$(CONFIG_GC1_00_BD)			+= gc1-gpio.o sf2-power.o gc1-jack.o
else
obj-$(CONFIG_GC1_00_BD)			+= gc1-gpio.o gc1-power.o gc1-jack.o
endif
obj-$(CONFIG_GC2PD_00_BD)		+= gc2pd-gpio.o gc2pd-power.o
obj-$(CONFIG_MACH_T0)			+= t0-gpio.o t0-power.o
obj-$(CONFIG_MACH_BAFFIN)			+= baffin-gpio.o baffin-power.o
obj-$(CONFIG_HD_00_BD)			+= hd-gpio.o hd-power.o
obj-$(CONFIG_MACH_GD2)			+= gd2-power.o
obj-$(CONFIG_GD2_01_BD)			+= gd2-gpio_01.o
obj-$(CONFIG_ZEST_00_BD)		+= zest-gpio.o zest-power.o
obj-$(CONFIG_WATCH_00_BD)		+= watch-gpio.o watch-power.o
obj-$(CONFIG_WATCH_01_BD)		+= watch-gpio.o watch-power.o
obj-$(CONFIG_WATCH_02_BD)		+= watch-gpio.o watch-power.o
obj-$(CONFIG_SP7160LTE_00_BD)		+= sp7160lte-gpio.o sp7160lte-power.o
obj-$(CONFIG_MACH_IPCAM)		+= ipcam-gpio.o ipcam-power.o
ifeq ($(CONFIG_MACH_M3),y)
obj-$(CONFIG_MIDAS_COMMON_BD)		+= m3-gpio.o midas-power.o
else
obj-$(CONFIG_MIDAS_COMMON_BD)		+= midas-gpio.o midas-power.o
endif
obj-$(CONFIG_EXTCON)			+= midas-extcon.o

obj-$(CONFIG_NAPLES_COMMON)		+= naples-gpio.o sec-common.o \
					   naples-power.o naples-tsp.o board-midas-wlan.o \
					   naples-camera.o midas-thermistor.o \
					   midas-mhl.o

obj-$(CONFIG_NFC_DEVICES)			+= midas-nfc.o

ifeq ($(CONFIG_MACH_GRANDE),y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o board-grande-sensor.o
else
ifeq ($(CONFIG_MACH_IRON),y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o board-grande-sensor.o
else
ifeq ($(CONFIG_MACH_KONA_SENSOR), y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o
obj-$(CONFIG_MACH_KONA_SENSOR)		+= kona-sensor.o
else
ifeq ($(CONFIG_MACH_GD2_SENSOR),y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o
obj-$(CONFIG_MACH_GD2_SENSOR)		+= gd2-sensor.o
else
ifeq ($(CONFIG_MACH_GC2PD),y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o gc2pd-sensor.o
else
ifeq ($(CONFIG_MACH_IPCAM),y)
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o ipcam-sensor.o
else
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o midas-sensor.o
endif
endif
endif
endif
endif
endif

obj-$(CONFIG_LEDS_LP5521)		+= midas-leds.o

obj-$(CONFIG_MACH_U1)			+= sec-reboot.o
obj-$(CONFIG_MACH_PX)			+= sec-reboot.o

obj-$(CONFIG_MACH_P10)			+= mach-p10.o p10-gpio.o sec-common.o p10-input.o \
					   p10-switch.o p10-battery.o p10-mhl.o

# device support

obj-$(CONFIG_ARCH_EXYNOS)		+= dev-audio.o
obj-$(CONFIG_EXYNOS5_DEV_AHCI)		+= dev-ahci-exynos5.o
obj-$(CONFIG_EXYNOS4_DEV_AHCI)		+= dev-ahci.o
obj-$(CONFIG_EXYNOS_DEV_PD)		+= dev-pd.o
ifeq ($(CONFIG_EXYNOS_DEV_PD),y)
obj-$(CONFIG_ARCH_EXYNOS4)		+= dev-pd-exynos4.o
obj-$(CONFIG_ARCH_EXYNOS5)		+= dev-pd-exynos5.o
endif
obj-$(CONFIG_S5P_SYSTEM_MMU)		+= dev-sysmmu-exynos4.o
obj-$(CONFIG_EXYNOS_DEV_SYSMMU)		+= dev-sysmmu.o
obj-$(CONFIG_EXYNOS4_DEV_DWMCI)		+= dev-dwmci.o
obj-$(CONFIG_EXYNOS_DEV_C2C)		+= dev-c2c.o
obj-$(CONFIG_EXYNOS4_DEV_FIMC_IS)	+= dev-fimc-is.o
obj-$(CONFIG_EXYNOS4_DEV_FIMC_LITE)	+= dev-fimc-lite.o
obj-$(CONFIG_EXYNOS5_DEV_GSC)		+= dev-gsc.o

obj-$(CONFIG_EXYNOS4_SETUP_FIMC)	+= setup-fimc.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC0)	+= setup-fimc0.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC1)	+= setup-fimc1.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC2)	+= setup-fimc2.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC3)	+= setup-fimc3.o
obj-$(CONFIG_EXYNOS4_SETUP_CSIS)	+= setup-csis.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMD0)	+= setup-fimd0.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMD)	+= setup-fimd.o
obj-$(CONFIG_EXYNOS4_SETUP_DP)		+= setup-dp.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC_IS)	+= setup-fimc-is.o
obj-$(CONFIG_EXYNOS5_SETUP_GSC)		+= setup-gsc.o
obj-$(CONFIG_EXYNOS4_SETUP_HDMI)	+= setup-hdmi.o
obj-y					+= setup-i2c0.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C1)	+= setup-i2c1.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C2)	+= setup-i2c2.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C3)	+= setup-i2c3.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C4)	+= setup-i2c4.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C5)	+= setup-i2c5.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C6)	+= setup-i2c6.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C7)	+= setup-i2c7.o
obj-$(CONFIG_EXYNOS4_SETUP_KEYPAD)	+= setup-keypad.o
obj-$(CONFIG_EXYNOS4_SETUP_MFC)		+= setup-mfc.o
obj-$(CONFIG_EXYNOS4_SETUP_SDHCI)	+= setup-sdhci.o
obj-$(CONFIG_EXYNOS4_SETUP_SDHCI_GPIO)	+= setup-sdhci-gpio.o
obj-$(CONFIG_EXYNOS4_SETUP_MSHCI)	+= setup-mshci.o
obj-$(CONFIG_EXYNOS4_SETUP_MSHCI_GPIO)	+= setup-mshci-gpio.o

obj-$(CONFIG_S3C64XX_DEV_SPI)		+= dev-spi.o
obj-$(CONFIG_BOOT_DEVICE_SPI)		+= dev-spi-boot.o

obj-$(CONFIG_EXYNOS4_SETUP_USB_PHY)	+= setup-usb-phy.o
obj-$(CONFIG_EXYNOS4_SETUP_FB_S5P)	+= setup-fb-s5p.o
obj-$(CONFIG_EXYNOS4_SETUP_TVOUT)	+= setup-tvout.o
obj-$(CONFIG_EXYNOS4_SETUP_JPEG)	+= setup-jpeg.o
obj-$(CONFIG_EXYNOS4_SETUP_THERMAL)	+= tmu.o
obj-$(CONFIG_EXYNOS_SETUP_THERMAL)	+= tmu_exynos.o
obj-$(CONFIG_EXYNOS4_SETUP_MIPI_DSI)	+= setup-dsim.o
obj-$(CONFIG_EXYNOS4_SETUP_MIPI_DSIM)	+= setup-mipidsim.o
obj-$(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) += secmem-allocdev.o
obj-$(CONFIG_SEC_MODEM_SP7160LTE)	+= board-sp7160lte-modems.o
obj-$(CONFIG_SEC_MODEM_M0_C2C)		+= board-midas-modems.o
obj-$(CONFIG_SEC_MODEM_M0)		+= board-m0-modems.o
obj-$(CONFIG_SEC_MODEM_M0_TD)           += board-m0-td-modems.o
obj-$(CONFIG_SEC_MODEM_Q1_TD)           += board-q1-td-modems.o
obj-$(CONFIG_SEC_MODEM_T0_TD_DUAL)	+= board-m0-modems.o board-m0-td-modems.o
obj-$(CONFIG_SEC_MODEM_M0_CTC)		+= board-m0ctc-modems.o
obj-$(CONFIG_SEC_MODEM_T0_CU_DUOS)	+= board-m0-modems.o board-t0cu-duos-modems.o
obj-$(CONFIG_SEC_MODEM_T0_OPEN_DUOS)	+= board-m0-modems.o board-t0cu-duos-modems.o
obj-$(CONFIG_SEC_MODEM_IRON)		+= board-iron-modems.o
obj-$(CONFIG_SEC_MODEM_GAIA)		+= board-gaia-modems.o
obj-$(CONFIG_SEC_MODEM_C1)		+= board-c1-modems.o
obj-$(CONFIG_SEC_MODEM_C1_LGT)		+= board-c1lgt-modems.o
obj-$(CONFIG_SEC_MODEM_U1)		+= board-u1-modems.o
obj-$(CONFIG_SEC_MODEM_U1_SPR)		+= board-u1-spr-modems.o
obj-$(CONFIG_SEC_MODEM_U1_LGT)		+= board-u1-lgt-modems.o
obj-$(CONFIG_SEC_MODEM_P8LTE)		+= board-p8ltevzw-modems.o
obj-$(CONFIG_SEC_MODEM_GODIVA2)		+= board-godiva2-modems.o
obj-$(CONFIG_SEC_MODEM_ZEST)		+= board-zest-modems.o
obj-$(CONFIG_SEC_MODEM_GC2_ATT)		+= board-gc2att-modems.o
obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o sec_getlog.o sec_gaf.o
obj-$(CONFIG_SEC_WATCHDOG_RESET)	+= sec_watchdog.o
obj-$(CONFIG_SEC_LOG)			+= sec_log.o
obj-$(CONFIG_EXYNOS5_DEV_BTS)		+= bts.o

obj-$(CONFIG_WAKEUP_ASSIST)		+= wakeup_assist.o
obj-$(CONFIG_BT_CSR8811)		+= board-bluetooth-csr8811.o
obj-$(CONFIG_ION_EXYNOS)		+= dev-ion.o

obj-$(CONFIG_BT_BCM4330)		+= board-bluetooth-bcm4330.o
obj-$(CONFIG_BT_BCM4334)		+= board-bluetooth-bcm4334.o
obj-$(CONFIG_BT_BCM43241)		+= board-bluetooth-bcm43241.o
obj-$(CONFIG_BT_BCM4335)		+= board-bluetooth-bcm4335.o
obj-$(CONFIG_SEC_THERMISTOR)		+= sec_thermistor.o
obj-$(CONFIG_SEC_SUBTHERMISTOR)		+= sec_subthermistor.o
obj-$(CONFIG_EXYNOS_SYSREG_PM)		+= sysreg.o

obj-$(CONFIG_EXYNOS4_DEV_TMU)           += dev-slp-tmu.o
obj-$(CONFIG_QC_MODEM_MDM9X15)	+= mdm2.o mdm_common.o mdm_device.o
obj-$(CONFIG_MSM_SUBSYSTEM_RESTART)	+= subsystem_restart.o
obj-$(CONFIG_MDM_HSIC_PM)		+= mdm_hsic_pm.o
ifeq ($(CONFIG_MACH_T0),y)
obj-$(CONFIG_INPUT_WACOM)		+= midas-wacom.o
endif

# for MUIC driver
obj-$(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)	+= board-exynos4212-mfd.o
obj-$(CONFIG_MUIC_ADD_PLATFORM_DEVICE)		+= board-exynos4212-muic.o
obj-$(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)	+= board-exynos4212-adc.o
ifeq ($(CONFIG_MFD_MAX77693),y)
obj-$(CONFIG_SAMSUNG_MUIC)			+= sec-switch_max77693.o
obj-$(CONFIG_DI_MUIC)				+= sec-switch_max77693_di.o
else
obj-$(CONFIG_SAMSUNG_MUIC)			+= sec-switch_muic.o
endif

obj-$(CONFIG_CORESIGHT_ETM) += coresight.o coresight-etb.o coresight-tpiu.o coresight-funnel.o coresight-etm.o

# for board sound
obj-$(CONFIG_SND_SOC_SAMSUNG_MIDAS_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_LUNGO_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_T0_WM1811)	+= t0-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_T0DUOS_WM1811)	+= t0-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_M3_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_GD2_WM1811)	+= t0-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_ZEST_WM1811)	+= t0-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_TAB3_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_KONA_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_GC2PD_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_SLP_MIDAS_WM1811)	+= midas-sound.o
obj-$(CONFIG_SND_SOC_SAMSUNG_SLP_EXYNOS_WM1811)	+= midas-sound.o
ifeq ($(CONFIG_MACH_M0_DUOSCTC),y)
obj-$(CONFIG_SND_SOC_SAMSUNG_GRANDE_WM1811)	+= midas-sound.o
else
obj-$(CONFIG_SND_SOC_SAMSUNG_GRANDE_WM1811)	+= board-grande-sound.o
endif

obj-$(CONFIG_SND_SOC_SAMSUNG_WATCH_YMU831)	+= watch-sound-y.o

# for audio platform data driver
obj-$(CONFIG_EXYNOS_SOUND_PLATFORM_DATA)	+= exynos_sound_platform_data.o

ifeq ($(CONFIG_GPS_BCMxxxxx),y)
obj-$(CONFIG_GPS_BCM47511)		+= sec_gps_bcm47511.o
obj-$(CONFIG_GPS_BCM4752)		+= sec_gps_bcm4752.o
obj-$(CONFIG_GPS_BCM47521)		+= sec_gps_bcm47521.o
endif

ifeq ($(CONFIG_GPS_CSRxT),y)
obj-$(CONFIG_GPS_GSD4T)			+= sec_gps_gsd4t.o
obj-$(CONFIG_GPS_GSD5T)			+= sec_gps_gsd5t.o
endif
