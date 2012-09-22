# Makefile for the kernel westbridge hal
#
ifneq ($(CONFIG_WESTBRIDGE_DEBUG),y)
	EXTRA_CFLAGS		+= -DWESTBRIDGE_NDEBUG
endif

obj-$(CONFIG_WESTBRIDGE_SWITCH_DRIVER)  += cyasswitch.o


