#
# Makefile for the kernel westbridge block driver
#

ifneq ($(CONFIG_WESTBRIDGE_DEBUG),y)
	EXTRA_CFLAGS		+= -DWESTBRIDGE_NDEBUG
endif

obj-$(CONFIG_WESTBRIDGE_BLOCK_DRIVER)  += cyasblkdev.o
cyasblkdev-y		:= cyasblkdev_block_na_spr.o cyasblkdev_queue_na_spr.o

