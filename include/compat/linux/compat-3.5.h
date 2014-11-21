#ifndef LINUX_3_5_COMPAT_H
#define LINUX_3_5_COMPAT_H

#include <linux/version.h>
#include <linux/fs.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0))

extern int simple_open(struct inode *inode, struct file *file);

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)) */

#endif /* LINUX_3_5_COMPAT_H */
