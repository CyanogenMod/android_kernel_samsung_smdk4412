#
# Makefile for the s3c framebuffer driver
#
ifeq ($(CONFIG_FB_S5P),y)
obj-y				+= s3cfb.o
s3cfb-y				:= s3cfb_main.o s3cfb_ops.o lcdfreq.o
obj-$(CONFIG_ARCH_S5PV210)	+= s3cfb_fimd6x.o
obj-$(CONFIG_ARCH_EXYNOS4)	+= s3cfb_fimd6x.o

obj-$(CONFIG_FB_S5P_S6C1372)	+= s3cfb_s6c1372.o
obj-$(CONFIG_FB_S5P_S6F1202A)	+= s3cfb_s6c1372.o
obj-$(CONFIG_FB_S5P_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S5P_HT101HD1)	+= s3cfb_ht101hd1.o
obj-$(CONFIG_FB_S5P_WA101S)	+= s3cfb_wa101s.o
obj-$(CONFIG_FB_S5P_AMS369FG06)	+= s3cfb_ams369fg06.o
obj-$(CONFIG_FB_S5P_LD9040)	+= ld9040.o smart_dimming_ld9042.o
obj-$(CONFIG_FB_S5P_NT35560)	+= nt35560.o
obj-$(CONFIG_FB_S5P_MDNIE)      += s3cfb_mdnie.o s3cfb_ielcd.o mdnie.o mdnie_tunning.o
obj-$(CONFIG_FB_S5P_LMS501KF03)	+= s3cfb_lms501kf03.o
obj-$(CONFIG_FB_S5P_LMS501XX)	+= s3cfb_lms501xx.o
obj-$(CONFIG_FB_S5P_DUMMY_MIPI_LCD)	+= s3cfb_dummymipilcd.o
obj-$(CONFIG_FB_S5P_S6E8AA0)	+= s3cfb_s6e8aa0.o smart_dimming.o
obj-$(CONFIG_FB_S5P_EA8061)	+= s3cfb_ea8061.o smart_dimming_ea8061.o
obj-$(CONFIG_FB_S5P_S6EVR02)	+= s3cfb_s6evr02.o smart_dimming_s6evr02.o s3cfb_ea8061.o smart_dimming_ea8061.o
obj-$(CONFIG_FB_S5P_S6E8AB0)	+= s3cfb_s6e8ab0.o smart_dimming_s6e8ab0.o
obj-$(CONFIG_FB_S5P_S6E39A0)	+= s3cfb_s6e39a0.o smart_dimming.o
obj-$(CONFIG_FB_S5P_S6E63M0)	+= s3cfb_s6e63m0.o smart_dimming.o
obj-$(CONFIG_FB_S5P_S6D6AA1)	+= s6d6aa1.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)	+= s5p_dsim_lowlevel.o
obj-$(CONFIG_FB_S5P_MIPI_DSIM)	+= s5p-dsim.o
obj-$(CONFIG_TARGET_LOCALE_KOR)	+= boot_progressbar.o
endif

ifeq ($(CONFIG_FB_S5P_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

# version 2 rules
ifeq ($(CONFIG_FB_S5P_V2),y)
obj-y					+= s3cfb2.o
obj-$(CONFIG_PLAT_S3C64XX)		+= s3cfb2_fimd4x.o
obj-$(CONFIG_PLAT_S5P64XX)		+= s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5P6450)              += s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5PC100)		+= s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5PC110)		+= s3cfb2_fimd6x.o
obj-$(CONFIG_FB_S5P_V2_LTE480WV)	+= s3cfb2_lte480wv.o
obj-$(CONFIG_FB_S5P_V2_TL2796)		+= s3cfb2_tl2796.o
endif

ifeq ($(CONFIG_FB_S5P_V2_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
