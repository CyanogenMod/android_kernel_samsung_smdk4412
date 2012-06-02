#
# Kernel makefile for motions sensors
#
#

# MPU
obj-$(CONFIG_MPU_SENSORS_MPU3050)	+= mpu3050.o
mpu3050-objs += mpuirq.o \
	slaveirq.o \
	mpu-dev.o \
	mpu-i2c.o \
	mlsl-kernel.o \
	mlos-kernel.o \
	mpu-accel.o \
	$(MLLITE_DIR)mldl_cfg.o

#
# Accel options
#
ifdef CONFIG_MPU_SENSORS_ADXL346
mpu3050-objs += $(MLLITE_DIR)accel/adxl346.o
endif

ifdef CONFIG_MPU_SENSORS_BMA150
mpu3050-objs += $(MLLITE_DIR)accel/bma150.o
endif

ifdef CONFIG_MPU_SENSORS_BMA222
mpu3050-objs += $(MLLITE_DIR)accel/bma222.o
endif

ifdef CONFIG_MPU_SENSORS_KXSD9
mpu3050-objs += $(MLLITE_DIR)accel/kxsd9.o
endif

ifdef CONFIG_MACH_BOSE_ATT
	mpu3050-objs += $(MLLITE_DIR)accel/kxud9.o
	mpu3050-objs += $(MLLITE_DIR)accel/kxtf9.o
else
ifdef CONFIG_MPU_SENSORS_KXUD9
mpu3050-objs += $(MLLITE_DIR)accel/kxud9.o
endif

ifdef CONFIG_MPU_SENSORS_KXTF9
mpu3050-objs += $(MLLITE_DIR)accel/kxtf9.o
endif
endif

ifdef CONFIG_MPU_SENSORS_LIS331DLH
mpu3050-objs += $(MLLITE_DIR)accel/lis331.o
endif

ifdef CONFIG_MPU_SENSORS_LIS3DH
mpu3050-objs += $(MLLITE_DIR)accel/lis3dh.o
endif

ifdef CONFIG_MPU_SENSORS_LSM303DLHA
mpu3050-objs += $(MLLITE_DIR)accel/lsm303a.o
endif

ifdef CONFIG_MPU_SENSORS_MMA8450
mpu3050-objs += $(MLLITE_DIR)accel/mma8450.o
endif

ifdef CONFIG_MPU_SENSORS_MMA845X
mpu3050-objs += $(MLLITE_DIR)accel/mma845x.o
endif

ifdef CONFIG_MPU_SENSORS_KXTF9_LIS3DH
mpu3050-objs += $(MLLITE_DIR)accel/kxtf9.o
mpu3050-objs += $(MLLITE_DIR)accel/lis3dh.o
endif

#
# Compass options
#
ifdef CONFIG_MPU_SENSORS_AK8975
mpu3050-objs += $(MLLITE_DIR)compass/mpuak8975.o
endif

ifdef CONFIG_MPU_SENSORS_AMI30X
mpu3050-objs += $(MLLITE_DIR)compass/ami30x.o
endif

ifdef CONFIG_MPU_SENSORS_HMC5883
mpu3050-objs += $(MLLITE_DIR)compass/hmc5883.o
endif

ifdef CONFIG_MPU_SENSORS_LSM303DLHM
mpu3050-objs += $(MLLITE_DIR)compass/lsm303m.o
endif

ifdef CONFIG_MPU_SENSORS_MMC314X
mpu3050-objs += $(MLLITE_DIR)compass/mmc314x.o
endif

ifdef CONFIG_MPU_SENSORS_MMC328X
mpu3050-objs += $(MLLITE_DIR)compass/mmc328x.o
endif

ifdef CONFIG_MPU_SENSORS_YAS529
mpu3050-objs += $(MLLITE_DIR)compass/yas529-kernel.o
endif

ifdef CONFIG_MPU_SENSORS_HSCDTD002B
mpu3050-objs += $(MLLITE_DIR)compass/hscdtd002b.o
endif

ifdef CONFIG_MPU_SENSORS_HSCDTD004A
mpu3050-objs += $(MLLITE_DIR)compass/hscdtd004a.o
endif

#
# Pressure options
#
ifdef CONFIG_MPU_SENSORS_BMA085
mpu3050-objs += $(MLLITE_DIR)pressure/bma085.o
endif

ccflags-y += -I$(M)/$(MLLITE_DIR) \
                -I$(M)/../../include \
		-Idrivers/misc/mpu3050 \
                -Iinclude/linux

obj-$(CONFIG_MPU_SENSORS_MPU6000)+= mpu6000.o
mpu6000-objs += mpuirq.o \
	slaveirq.o \
	mpu-dev.o \
	mpu-i2c.o \
	mlsl-kernel.o \
	mlos-kernel.o \
	$(MLLITE_DIR)mldl_cfg.o \
	$(MLLITE_DIR)accel/mantis.o

ifdef CONFIG_MPU_SENSORS_MPU6000
ccflags-y += -DM_HW
endif

obj-$(CONFIG_MPU_SENSORS_CORE)			+= sensors_core.o
obj-$(CONFIG_MPU_SENSORS_TIMERIRQ)+= timerirq.o

ifdef CONFIG_MPU_SENSORS_DEBUG
ccflags-y += -DDEBUG
endif

