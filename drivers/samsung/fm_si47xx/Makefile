ifeq	($(CONFIG_FM_SI4705),y)
obj-$(CONFIG_FM_SI4705)		+= si47xx-drv.o
si47xx-drv-y := Si47xx_i2c_drv.o Si47xx_dev.o
else
obj-$(CONFIG_FM_SI4705)		+= Si4709_driver.o
Si4709_driver-y := Si47xx_i2c_drv.o Si47xx_dev.o
endif