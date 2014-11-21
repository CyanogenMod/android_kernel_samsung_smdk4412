#
# Compass Slaves MPUxxxx
#

obj-$(CONFIG_MPU_SENSORS_MPU6050_LSM303DLX_M) += inv_mpu_lsm303dlx_m.o
inv_mpu_lsm303dlx_m-objs +=	lsm303dlx_m.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_MMC314X) += inv_mpu_mmc314x.o
inv_mpu_mmc314x-objs +=	mmc314x.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_YAS529) += inv_mpu_yas529.o
inv_mpu_yas529-objs +=	yas529-kernel.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_YAS530_411) += inv_mpu_yas530.o
inv_mpu_yas530-objs +=	yas530.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_HSCDTD002B) += inv_mpu_hscdtd002b.o
inv_mpu_hscdtd002b-objs +=	hscdtd002b.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_HSCDTD004A) += inv_mpu_hscdtd004a.o
inv_mpu_hscdtd004a-objs +=	hscdtd004a.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_AK8975) += inv_mpu_ak8975.o
inv_mpu_ak8975-objs +=	ak8975.o

obj-$(CONFIG_MPU_SENSORS_MPU6050_AK8972) += inv_mpu_ak8972.o
inv_mpu_ak8972-objs +=	ak8972.o

EXTRA_CFLAGS += -Idrivers/misc/inv_mpu
EXTRA_CFLAGS += -D__C99_DESIGNATED_INITIALIZER
