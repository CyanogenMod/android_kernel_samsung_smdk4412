#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SENSORS_SSP_ATMEL)		+= ssp_dev.o ssp_i2c.o ssp_data.o ssp_sysfs.o\
						ssp_input.o ssp_firmware.o ssp_debug.o

obj-$(CONFIG_SENSORS_SYSFS)		+= sensors_core.o

obj-$(CONFIG_SENSORS_SSP_LSM330)	+= factory/accel_lsm330.o factory/gyro_lsm330.o

obj-$(CONFIG_SENSORS_SSP_CM36651)	+= factory/light_cm36651.o factory/prox_cm36651.o

obj-$(CONFIG_SENSORS_SSP_AK8963C)	+= ssp_ak8963c.o factory/magnetic_ak8963c.o

obj-$(CONFIG_SENSORS_SSP_BMP182)	+= factory/pressure_bmp182.o

obj-$(CONFIG_SENSORS_SSP_AT32UC3L0128)	+= factory/mcu_at32uc3l0128.o

obj-$(CONFIG_SENSORS_SSP_SENSORHUB)	+= ssp_sensorhub.o
