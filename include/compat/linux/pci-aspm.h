#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25))
#include_next <linux/pci-aspm.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25)) */
