#
# Makefile for the s3c framebuffer driver
#
ifeq ($(CONFIG_FB_S5P_EXTDSP),y)
obj-y				+= s3cfb_extdsp.o
s3cfb_extdsp-y			:= s3cfb_extdsp_main.o s3cfb_extdsp_ops.o
endif

ifeq ($(CONFIG_FB_S5P_EXTDSP_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
