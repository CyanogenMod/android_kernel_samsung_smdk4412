#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SENSORS_SSP_STM)		+= ssp_dev.o ssp_comm.o ssp_data.o ssp_sysfs.o\
						ssp_input.o ssp_firmware.o ssp_debug.o ssp_spi.o
obj-$(CONFIG_SENSORS_SYSFS)		+= sensors_core.o

obj-$(CONFIG_SENSORS_SSP_MPU6500)	+= factory/accel_mpu6500.o factory/gyro_mpu6500.o

obj-$(CONFIG_SENSORS_SSP_CM3320)	+= factory/light_cm3320.o
obj-$(CONFIG_SENSORS_SSP_MAX88920)	+= factory/prox_max88920.o factory/gesture_max88920.o

obj-$(CONFIG_SENSORS_SSP_YAS532)	+= factory/magnetic_yas532.o

obj-$(CONFIG_SENSORS_SSP_BMP182)	+= factory/pressure_bmp182.o
obj-$(CONFIG_SENSORS_SSP_SHTC1)		+= factory/temphumidity_shtc1.o

obj-$(CONFIG_SENSORS_SSP_ATUC128L5HAR)	+= factory/mcu_atuc128l5har.o
obj-$(CONFIG_SENSORS_SSP_STM32F401)	+= factory/mcu_stm32f401.o

obj-$(CONFIG_SENSORS_SSP_SENSORHUB)	+= ssp_sensorhub.o
