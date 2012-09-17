obj-$(CONFIG_AUTHENTEC_VPNCLIENT_INTERCEPTOR) := vpnclient.o

vpnclient-y := \
	kernel_alloc.o \
	kernel_encode.o \
	linux_hook_magic.o \
	linux_iface.o \
	linux_ip_glue.o \
	linux_ipm.o \
	linux_kernel_alloc.o \
	linux_main.o \
	linux_usermode.o \
	linux_virtual_adapter.o \
	linux_mutex.o \
	linux_packet.o \
	linux_procfs.o \
	linux_route.o \
	sshinetbits.o \
	sshinetencode.o \
	sshinetprint.o \
    usermodeforwarder.o

override EXTRA_CFLAGS += \
    -DKERNEL \
    -D_KERNEL \
    -DWITH_IPV6
