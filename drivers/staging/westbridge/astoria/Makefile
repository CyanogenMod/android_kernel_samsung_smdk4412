#
# Makefile for the kernel westbridge device drivers.
#

ifneq ($(CONFIG_WESTBRIDGE_DEBUG),y)
	EXTRA_CFLAGS		+= -WESTBRIDGE_NDEBUG
endif

obj-$(CONFIG_WESTBRIDGE)		+= device/
ifeq ($(CONFIG_MACH_U1_NA_SPR),y)
obj-$(CONFIG_WESTBRIDGE)		+= switch/
else ifeq ($(CONFIG_MACH_U1_NA_USCC),y)
obj-$(CONFIG_WESTBRIDGE)		+= switch/
else
obj-$(CONFIG_WESTBRIDGE)		+= block/
obj-$(CONFIG_WESTBRIDGE)		+= gadget/
endif

