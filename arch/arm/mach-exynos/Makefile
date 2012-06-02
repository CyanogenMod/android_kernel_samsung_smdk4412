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
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= asv.o asv-4210.o asv-4x12.o
obj-$(CONFIG_EXYNOS4_CPUFREQ)	+= cpufreq-4210.o cpufreq-4x12.o
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
ifeq ($(CONFIG_MACH_U1CAMERA_BD),y)
obj-$(CONFIG_MACH_U1)			+= mach-u1cam.o
else
obj-$(CONFIG_MACH_U1)			+= mach-u1.o
endif
obj-$(CONFIG_MACH_U1)			+= sec-common.o sec-switch_max8997.o
obj-$(CONFIG_MACH_U1_BD)		+= u1-gpio.o board-gps-gsd4t.o
obj-$(CONFIG_MACH_Q1_BD)		+= q1-gpio.o board-gps-bcm475x.o
obj-$(CONFIG_MACH_U1CAMERA_BD)		+= u1camera-gpio.o board-gps-bcm475x.o
obj-$(CONFIG_MACH_U1)			+= u1-wlan.o
obj-$(CONFIG_MACH_PX)			+= mach-px.o sec-common.o board-gps-bcm475x.o px-switch.o
obj-$(CONFIG_MACH_P2)			+= p2-gpio.o
obj-$(CONFIG_MACH_P4)			+= p4-gpio.o
obj-$(CONFIG_MACH_P8)			+= p8-gpio.o
obj-$(CONFIG_MACH_P10)			+= board-p10-wlan.o board-gps-bcm475x.o
obj-$(CONFIG_PANEL_U1)			+= u1-panel.o u1-panel_a2.o u1-panel_m2.o
obj-$(CONFIG_PANEL_S2PLUS)		+= s2plus-panel.o
obj-$(CONFIG_MACH_U1)			+= u1_regulator_consumer.o
obj-$(CONFIG_MACH_PX)			+= u1_regulator_consumer.o
obj-$(CONFIG_MACH_NURI)			+= mach-nuri.o
obj-$(CONFIG_MACH_SMDK4X12)		+= mach-smdk4x12.o
ifeq ($(CONFIG_MACH_P4NOTE),y)
obj-$(CONFIG_MACH_MIDAS)		+= mach-p4notepq.o px-switch.o p4note-jack.o
else
obj-$(CONFIG_MACH_MIDAS)		+= mach-midas.o
endif
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

obj-$(CONFIG_MIDAS_COMMON)		+= sec-common.o board-gps-bcm475x.o \
					   midas-tsp.o board-midas-wlan.o \
					   midas-camera.o midas-thermistor.o \
					   midas-mhl.o midas-lcd.o midas-sound.o

obj-$(CONFIG_P4NOTE_00_BD)		+= p4note-gpio.o p4note-power.o
obj-$(CONFIG_GC1_00_BD)			+= gc1-gpio.o gc1-power.o
obj-$(CONFIG_T0_00_BD)			+= midas-gpio.o midas-power.o
obj-$(CONFIG_MIDAS_COMMON_BD)	+= midas-gpio.o midas-power.o
obj-$(CONFIG_EXTCON)			+= midas-extcon.o

obj-$(CONFIG_NAPLES_COMMON)		+= naples-gpio.o sec-common.o midas-gps.o \
					   naples-power.o naples-tsp.o board-midas-wlan.o \
					   naples-camera.o midas-thermistor.o \
					   midas-mhl.o

obj-$(CONFIG_PN65N_NFC)			+= midas-nfc.o
obj-$(CONFIG_MACH_MIDAS)		+= midas-sensor.o
obj-$(CONFIG_LEDS_LP5521)		+= midas-leds.o

obj-$(CONFIG_MACH_U1)			+= sec-reboot.o
obj-$(CONFIG_MACH_PX)			+= sec-reboot.o
obj-$(CONFIG_MACH_MIDAS)		+= sec-reboot.o

obj-$(CONFIG_MACH_P10)			+= mach-p10.o p10-gpio.o sec-common.o p10-input.o \
					   p10-switch.o p10-battery.o midas-sound.o p10-mhl.o

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

obj-$(CONFIG_EXYNOS4_SETUP_USB_PHY)	+= setup-usb-phy.o
obj-$(CONFIG_EXYNOS4_SETUP_FB_S5P)	+= setup-fb-s5p.o
obj-$(CONFIG_EXYNOS4_SETUP_TVOUT)	+= setup-tvout.o
obj-$(CONFIG_EXYNOS4_SETUP_JPEG)	+= setup-jpeg.o
obj-$(CONFIG_EXYNOS4_SETUP_THERMAL)	+= tmu.o
obj-$(CONFIG_EXYNOS_SETUP_THERMAL)	+= tmu_exynos.o
obj-$(CONFIG_EXYNOS4_SETUP_MIPI_DSI)	+= setup-dsim.o
obj-$(CONFIG_EXYNOS4_SETUP_MIPI_DSIM)	+= setup-mipidsim.o
obj-$(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) += secmem-allocdev.o
obj-$(CONFIG_EXYNOS_C2C)		+= setup-c2c.o
obj-$(CONFIG_SEC_MODEM_M0_C2C)		+= board-midas-modems.o
ifeq ($(CONFIG_MACH_P4NOTE),y)
obj-$(CONFIG_SEC_MODEM_M0)		+= board-p4notepq-modems.o
else
obj-$(CONFIG_SEC_MODEM_M0)		+= board-m0-modems.o
endif
obj-$(CONFIG_SEC_MODEM_M0_TD)           += board-m0-td-modems.o
obj-$(CONFIG_SEC_MODEM_M0_CTC)		+= board-m0ctc-modems.o
#obj-$(CONFIG_SEC_MODEM_M1)		+= board-m1-modems.o
obj-$(CONFIG_SEC_MODEM_GAIA)		+= board-gaia-modems.o
obj-$(CONFIG_SEC_MODEM_M1)		+= board-c1-modems.o
obj-$(CONFIG_SEC_MODEM_C1)		+= board-c1-modems.o
obj-$(CONFIG_SEC_MODEM_C1_VZW)		+= board-c1vzw-modems.o
obj-$(CONFIG_SEC_MODEM_C1_LGT)		+= board-c1lgt-modems.o
obj-$(CONFIG_SEC_MODEM_C1_CTC)		+= board-c1ctc-modems.o
obj-$(CONFIG_SEC_MODEM_U1)		+= board-u1-modems.o
obj-$(CONFIG_SEC_MODEM_JENGA)		+= board-jenga-modems.o
obj-$(CONFIG_SEC_MODEM_S2PLUS)		+= board-s2plus-modems.o
obj-$(CONFIG_SEC_MODEM_U1_LGT)		+= board-u1-lgt-modems.o
obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o sec_getlog.o sec_gaf.o
obj-$(CONFIG_SEC_WATCHDOG_RESET)	+= sec_watchdog.o
obj-$(CONFIG_SEC_LOG)			+= sec_log.o
obj-$(CONFIG_EXYNOS5_DEV_BTS)		+= bts.o

obj-$(CONFIG_WAKEUP_ASSIST)		+= wakeup_assist.o
obj-$(CONFIG_BT_CSR8811)		+= board-bluetooth-csr8811.o
obj-$(CONFIG_ION_EXYNOS)		+= dev-ion.o
obj-$(CONFIG_MFD_MAX77693)		+= sec-switch.o
obj-$(CONFIG_BT_BCM4330)		+= board-bluetooth-bcm4330.o
obj-$(CONFIG_BT_BCM4334)		+= board-bluetooth-bcm4334.o
obj-$(CONFIG_BT_BCM43241)		+= board-bluetooth-bcm43241.o
obj-$(CONFIG_GPS_BCM47511)		+= bcm47511.o
obj-$(CONFIG_SEC_THERMISTOR)		+= sec_thermistor.o
obj-$(CONFIG_EXYNOS_SYSREG_PM)		+= sysreg.o

obj-$(CONFIG_QC_MODEM_MDM9X15)	+= mdm2.o mdm_common.o mdm_device.o
obj-$(CONFIG_MSM_SUBSYSTEM_RESTART)	+= subsystem_notif.o subsystem_restart.o
