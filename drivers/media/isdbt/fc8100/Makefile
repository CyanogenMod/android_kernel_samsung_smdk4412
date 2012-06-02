#fci driver

obj-$(CONFIG_ISDBT_FC8100) := isdbt.o
isdbt-objs := fc8100.o bbm.o fc8100_bb.o fci_tun.o fci_hal.o fci_i2c.o fc8100_isr.o fc8100_tun.o fc8100_i2c.o fci_oal.o

#KDIR := /home/android/workspace/S5PC100/kernel
#KDIR := /usr/src/linux-headers-2.6.31-14-generic
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
