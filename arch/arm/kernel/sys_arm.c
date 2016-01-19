/*
 *  linux/arch/arm/kernel/sys_arm.c
 *
 *  Copyright (C) People who wrote linux/arch/i386/kernel/sys_i386.c
 *  Copyright (C) 1995, 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Linux/arm
 *  platform.
 */
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

/* Fork a new task - this creates a new program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_fork(struct pt_regs *regs)
{
#ifdef CONFIG_MMU
	return do_fork(SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
#else
	/* can not support in nommu mode */
	return(-EINVAL);
#endif
}

/* Clone a task - this clones the calling program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_clone(unsigned long clone_flags, unsigned long newsp,
			 int __user *parent_tidptr, int tls_val,
			 int __user *child_tidptr, struct pt_regs *regs)
{
	if (!newsp)
		newsp = regs->ARM_sp;

	return do_fork(clone_flags, newsp, regs, 0, parent_tidptr, child_tidptr);
}

asmlinkage int sys_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
}

/* Samsung Rooting Restriction Feature */
#if defined CONFIG_SEC_RESTRICT_FORK
#if defined CONFIG_SEC_RESTRICT_ROOTING_LOG
#define PRINT_LOG(...)	printk(KERN_ERR __VA_ARGS__)
#else
#define PRINT_LOG(...)
#endif	// End of CONFIG_SEC_RESTRICT_ROOTING_LOG

#define CHECK_ROOT_UID(x) (x->cred->uid == 0 || x->cred->gid == 0 || \
			x->cred->euid == 0 || x->cred->egid == 0 || \
			x->cred->suid == 0 || x->cred->sgid == 0)

/*  sec_check_execpath
    return value : give task's exec path is matched or not
*/
int sec_check_execpath(struct mm_struct *mm, char *denypath)
{
	struct file *exe_file;
	char *path, *pathbuf = NULL;
	unsigned int path_length = 0, denypath_length = 0;
	int ret = 0;

	if(mm == NULL)
		return 0;
	
	if(!(exe_file = get_mm_exe_file(mm)))
	{
		PRINT_LOG("Cannot get exe from task->mm.\n");
		goto out_nofile;
	}

	if(!(pathbuf = kmalloc(PATH_MAX, GFP_TEMPORARY)))
	{
		PRINT_LOG("failed to kmalloc for pathbuf\n");
		goto out;
	}

	path = d_path(&exe_file->f_path, pathbuf, PATH_MAX);

	if (IS_ERR(path)) {
		PRINT_LOG("Error get path..\n");
		goto out;
	}

	path_length = strlen(path);
	denypath_length = strlen(denypath);

	if(!strncmp(path, denypath, (path_length < denypath_length) ?
				path_length : denypath_length)) {
		ret = 1;
	}
out :
	fput(exe_file);
out_nofile:
	if(pathbuf)
		kfree(pathbuf);

	return ret;
}
EXPORT_SYMBOL(sec_check_execpath);

static int sec_restrict_fork(void)
{
	struct cred *shellcred;
	int ret = 0;
	struct task_struct *parent_tsk;
	struct mm_struct *parent_mm = NULL;
	const struct cred *parent_cred;

	read_lock(&tasklist_lock);
	parent_tsk = current->parent;
	if (!parent_tsk) {
		read_unlock(&tasklist_lock);
		return 0;
	}
	get_task_struct(parent_tsk);
	/* holding on to the task struct is enough so just release
	 * the tasklist lock here */
	read_unlock(&tasklist_lock);

	/* 1. Allowed case - init process. */
	if(current->pid == 1 || parent_tsk->pid == 1)
		goto out;

	/* get current->parent's mm struct to access it's mm
	 * and to keep it alive */
	parent_mm = get_task_mm(parent_tsk);
	
	/* 1.1 Skip for kernel tasks */
	if(current->mm == NULL || parent_mm == NULL)
		goto out;

	/* 2. Restrict case - parent process is /sbin/adbd. */
	if( sec_check_execpath(parent_mm, "/sbin/adbd") ) {

		shellcred = prepare_creds();
		if(!shellcred) {
			ret = 1;
			goto out;
		}

		shellcred->uid = 2000;
		shellcred->gid = 2000;
		shellcred->euid = 2000;
		shellcred->egid = 2000;

		commit_creds(shellcred);
		ret = 0;
		goto out;
	}

	/* 3. Restrict case - execute file in /data directory.
	*/
	if( sec_check_execpath(current->mm, "/data/") ) {
		ret = 1;
		goto out;
	}

	/* 4. Restrict case - parent's privilege is not root. */
	parent_cred = get_task_cred(parent_tsk);
	if (!parent_cred)
		goto out;
	if(!CHECK_ROOT_UID(parent_tsk))
		ret = 1;
	put_cred(parent_cred);

out:
	if (parent_mm)
		mmput(parent_mm);
	put_task_struct(parent_tsk);

	return ret;
}
#endif	/* End of CONFIG_SEC_RESTRICT_FORK */

/* sys_execve() executes a new program.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_execve(const char __user *filenamei,
			  const char __user *const __user *argv,
			  const char __user *const __user *envp, struct pt_regs *regs)
{
	int error;
	char * filename;

	filename = getname(filenamei);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		goto out;

#if defined CONFIG_SEC_RESTRICT_FORK
	if(CHECK_ROOT_UID(current))
		if(sec_restrict_fork())
		{
			PRINT_LOG("Restricted making process. PID = %d(%s) "
							"PPID = %d(%s)\n",
				current->pid, current->comm,
				current->parent->pid, current->parent->comm);
			return -EACCES;
		}
#endif	// End of CONFIG_SEC_RESTRICT_FORK

	error = do_execve(filename, argv, envp, regs);
	putname(filename);
out:
	return error;
}

int kernel_execve(const char *filename,
		  const char *const argv[],
		  const char *const envp[])
{
	struct pt_regs regs;
	int ret;

	memset(&regs, 0, sizeof(struct pt_regs));
	ret = do_execve(filename,
			(const char __user *const __user *)argv,
			(const char __user *const __user *)envp, &regs);
	if (ret < 0)
		goto out;

	/*
	 * Save argc to the register structure for userspace.
	 */
	regs.ARM_r0 = ret;

	/*
	 * We were successful.  We won't be returning to our caller, but
	 * instead to user space by manipulating the kernel stack.
	 */
	asm(	"add	r0, %0, %1\n\t"
		"mov	r1, %2\n\t"
		"mov	r2, %3\n\t"
		"bl	memmove\n\t"	/* copy regs to top of stack */
		"mov	r8, #0\n\t"	/* not a syscall */
		"mov	r9, %0\n\t"	/* thread structure */
		"mov	sp, r0\n\t"	/* reposition stack pointer */
		"b	ret_to_user"
		:
		: "r" (current_thread_info()),
		  "Ir" (THREAD_START_SP - sizeof(regs)),
		  "r" (&regs),
		  "Ir" (sizeof(regs))
		: "r0", "r1", "r2", "r3", "r8", "r9", "ip", "lr", "memory");

 out:
	return ret;
}
EXPORT_SYMBOL(kernel_execve);

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}
