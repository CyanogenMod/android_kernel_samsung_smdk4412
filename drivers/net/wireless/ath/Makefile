NOSTDINC_FLAGS := -I$(srctree)/include/compat/ \
        -include $(srctree)/include/compat/linux/compat-2.6.h \
        $(CFLAGS)

obj-$(CONFIG_ATH6KL)		+= ath6kl/
obj-$(CONFIG_ATH_COMMON)	+= ath.o

ath-objs :=	main.o \
		regd.o \
		hw.o \
		key.o
ath-$(CONFIG_ATH_DEBUG) += debug.o
