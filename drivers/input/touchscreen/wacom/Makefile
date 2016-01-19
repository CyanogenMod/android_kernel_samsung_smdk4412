
obj-$(CONFIG_INPUT_WACOM)	+= wacom_i2c.o wacom_i2c_firm.o wacom_i2c_func.o

ifeq ($(CONFIG_EPEN_WACOM_G9PL),y)
obj-$(CONFIG_INPUT_WACOM)	+= w9002_flash.o
else
obj-$(CONFIG_INPUT_WACOM)	+= wacom_i2c_flash.o
endif
