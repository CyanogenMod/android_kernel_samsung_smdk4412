#ifndef LINUX_3_0_COMPAT_H
#define LINUX_3_0_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))

#include <linux/rcupdate.h>

/*
 * since commit 1c5cae815d19ffe02bdfda1260949ef2b1806171
 * "net: call dev_alloc_name from register_netdevice" dev_alloc_name is
 * called automatically. This is not implemented in older kernel
 * versions so it will result in device wrong names.
 */
static inline int register_netdevice_name(struct net_device *dev)
{
	int err;

	if (strchr(dev->name, '%')) {
		err = dev_alloc_name(dev, dev->name);
		if (err < 0)
			return err;
	}

	return register_netdevice(dev);
}

#define register_netdevice(dev) register_netdevice_name(dev)

/* BCMA core, see drivers/bcma/ */
#ifndef BCMA_CORE
/* Broadcom's specific AMBA core, see drivers/bcma/ */
struct bcma_device_id {
	__u16	manuf;
	__u16	id;
	__u8	rev;
	__u8	class;
};
#define BCMA_CORE(_manuf, _id, _rev, _class)  \
	{ .manuf = _manuf, .id = _id, .rev = _rev, .class = _class, }
#define BCMA_CORETABLE_END  \
	{ 0, },

#define BCMA_ANY_MANUF		0xFFFF
#define BCMA_ANY_ID		0xFFFF
#define BCMA_ANY_REV		0xFF
#define BCMA_ANY_CLASS		0xFF
#endif /* BCMA_CORE */

int mac_pton(const char *s, u8 *mac);

int __must_check kstrtoull_from_user(const char __user *s, size_t count, unsigned int base, unsigned long long *res);
int __must_check kstrtoll_from_user(const char __user *s, size_t count, unsigned int base, long long *res);
int __must_check kstrtoul_from_user(const char __user *s, size_t count, unsigned int base, unsigned long *res);
int __must_check kstrtol_from_user(const char __user *s, size_t count, unsigned int base, long *res);
int __must_check kstrtouint_from_user(const char __user *s, size_t count, unsigned int base, unsigned int *res);
int __must_check kstrtoint_from_user(const char __user *s, size_t count, unsigned int base, int *res);
int __must_check kstrtou16_from_user(const char __user *s, size_t count, unsigned int base, u16 *res);
int __must_check kstrtos16_from_user(const char __user *s, size_t count, unsigned int base, s16 *res);
int __must_check kstrtou8_from_user(const char __user *s, size_t count, unsigned int base, u8 *res);
int __must_check kstrtos8_from_user(const char __user *s, size_t count, unsigned int base, s8 *res);

static inline int __must_check kstrtou64_from_user(const char __user *s, size_t count, unsigned int base, u64 *res)
{
	return kstrtoull_from_user(s, count, base, res);
}

static inline int __must_check kstrtos64_from_user(const char __user *s, size_t count, unsigned int base, s64 *res)
{
	return kstrtoll_from_user(s, count, base, res);
}

static inline int __must_check kstrtou32_from_user(const char __user *s, size_t count, unsigned int base, u32 *res)
{
	return kstrtouint_from_user(s, count, base, res);
}

static inline int __must_check kstrtos32_from_user(const char __user *s, size_t count, unsigned int base, s32 *res)
{
	return kstrtoint_from_user(s, count, base, res);
}

/*
 * This adds a nested function everywhere kfree_rcu() was called. This
 * function frees the memory and is given as a function to call_rcu().
 * The rcu callback could happen every time also after the module was
 *  unloaded and this will cause problems.
 */
#define kfree_rcu(data, rcuhead)		do {			\
		void __kfree_rcu_fn(struct rcu_head *rcu_head)		\
		{							\
			void *___ptr;					\
			___ptr = container_of(rcu_head, typeof(*(data)), rcuhead);\
			kfree(___ptr);					\
		}							\
		call_rcu(&(data)->rcuhead, __kfree_rcu_fn);		\
	} while (0)

#ifdef MODULE

/*
 * The define overwriting module_exit is based on the original module_exit
 * which looks like this:
 * #define module_exit(exitfn)                                    \
 *         static inline exitcall_t __exittest(void)               \
 *         { return exitfn; }                                      \
 *         void cleanup_module(void) __attribute__((alias(#exitfn)));
 *
 * We replaced the call to the actual function exitfn() with a call to our
 * function which calls the original exitfn() and then rcu_barrier()
 *
 * As a module will not be unloaded that ofter it should not have a big
 * performance impact when rcu_barrier() is called on every module exit,
 * also when no kfree_rcu() backport is used in that module.
 */
#undef module_exit
#define module_exit(exitfn)						\
	static void __exit __exit_compat(void)				\
	{								\
		exitfn();						\
		rcu_barrier();						\
	}								\
	void cleanup_module(void) __attribute__((alias("__exit_compat")));

#endif

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)) */

#endif /* LINUX_3_0_COMPAT_H */
