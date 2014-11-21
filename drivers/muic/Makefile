#
# Makefile for muic devices
#

obj-$(CONFIG_MUIC_FSA9485)		+= fsa9485-muic.o
obj-$(CONFIG_MUIC_MAX14577)		+= max14577-muic.o
obj-$(CONFIG_MUIC_MAX8997)		+= max8997-muic.o

ifeq ($(CONFIG_SAMSUNG_MUIC),y)
obj-$(CONFIG_MFD_MAX77693)		+= max77693-muic.o
else ifeq ($(CONFIG_DI_MUIC),y)
obj-$(CONFIG_MFD_MAX77693)		+= max77693-muic_di.o
endif

