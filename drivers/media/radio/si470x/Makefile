#
# Makefile for radios with Silicon Labs Si470x FM Radio Receivers
#

radio-usb-si470x-objs	:= radio-si470x-usb.o radio-si470x-common.o
radio-i2c-si470x-objs	:= radio-si470x-i2c.o radio-si470x-common.o
radio-i2c-si4705-objs	:= radio-si4705-i2c.o radio-si4705-common.o

obj-$(CONFIG_USB_SI470X) += radio-usb-si470x.o
obj-$(CONFIG_I2C_SI470X) += radio-i2c-si470x.o
obj-$(CONFIG_I2C_SI4705) += radio-i2c-si4705.o
