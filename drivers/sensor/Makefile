#
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SENSORS_CORE)		+= sensors_core.o
# accelerometer_sensor
obj-$(CONFIG_SENSORS_K3DH)		+= k3dh.o
obj-$(CONFIG_SENSORS_BMA254)	+= bma254_driver.o
# gyro_sensor
obj-$(CONFIG_SENSORS_K3G)		+= k3g.o
# combo chipset
obj-$(CONFIG_SENSORS_LSM330DLC)	+= lsm330dlc_accel.o lsm330dlc_gyro.o
obj-$(CONFIG_SENSORS_K330)	+= k330_accel.o k330_gyro.o
# magnetic_sensor
obj-$(CONFIG_SENSORS_AK8975C)	+= ak8975.o
ifeq ($(CONFIG_MACH_SF2),y)
obj-$(CONFIG_SENSORS_AK8963C)	+= ak8963_sf2.o
else
obj-$(CONFIG_SENSORS_AK8963C)	+= ak8963.o
endif
obj-$(CONFIG_SENSORS_YAS532)	+= yas_mag_kernel_driver.o
obj-$(CONFIG_SENSORS_YAS_ORI)	+= yas_ori_kernel_driver.o
# optical_sensor
obj-$(CONFIG_SENSORS_CM3663)	+= cm3663.o
obj-$(CONFIG_SENSORS_TMD27723)	+= taos.o
obj-$(CONFIG_SENSORS_GP2A)		+= gp2a_proximity.o gp2a_light.o
obj-$(CONFIG_SENSORS_GP2A_ANALOG)	+= gp2a_analog.o
obj-$(CONFIG_SENSORS_CM36651)		+= cm36651.o
obj-$(CONFIG_SENSORS_CM36653)		+= cm36653.o
obj-$(CONFIG_SENSORS_CM3323)		+= cm3323.o
obj-$(CONFIG_SENSORS_BH1721)		+= bh1721.o
obj-$(CONFIG_SENSORS_BH1730)		+= bh1730.o
obj-$(CONFIG_SENSORS_AL3201)		+= al3201.o
# barometer_sensor
obj-$(CONFIG_SENSORS_BMP180)		+= bmp180.o
obj-$(CONFIG_SENSORS_LPS331)		+= lps331ap.o
# VE_GROUP
obj-$(CONFIG_SENSORS_PAS2M110)		+= pas2m110.o
# grip sensor
obj-$(CONFIG_SENSORS_ASP01)		+= asp01.o
# temp-humidity sensor
obj-$(CONFIG_SENSORS_SEC_SHTC1)	+= shtc1.o