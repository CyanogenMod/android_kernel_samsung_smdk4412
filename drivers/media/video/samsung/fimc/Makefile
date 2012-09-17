obj-$(CONFIG_VIDEO_FIMC)	+= fimc_v4l2.o fimc_output.o fimc_overlay.o fimc_regs.o

obj-$(CONFIG_CPU_EXYNOS4412)	+= fimc_capture.o fimc_dev.o
obj-$(CONFIG_CPU_EXYNOS4212)	+= fimc_capture.o fimc_dev.o

obj-$(CONFIG_CPU_EXYNOS4210)	+= fimc_capture_u1.o fimc_dev_u1.o
obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_CPU_S5PV210)	+= ipc.o

ifeq ($(CONFIG_CPU_S5PV210),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

ifeq ($(CONFIG_ARCH_EXYNOS4),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
