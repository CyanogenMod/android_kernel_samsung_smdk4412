/* drivers/char/s3c_mem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S3C MEM driver for /dev/mem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/errno.h>	/* error codes */
#include <asm/div64.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/mman.h>
#include <linux/dma-mapping.h>

#include <linux/unistd.h>
#include <linux/version.h>
#include <mach/map.h>
#include <mach/hardware.h>

#include "s3c_mem.h"
#ifdef CONFIG_S3C_DMA_MEM
#include "s3c_dma_mem.h"
#endif

#ifdef CONFIG_S3C_MEM_CMA_ALLOC
#include <linux/cma.h>
#include <linux/platform_device.h>
#endif

static int flag;

static unsigned int physical_address;

#ifdef USE_DMA_ALLOC
static unsigned int virtual_address;
#endif

#ifdef CONFIG_S3C_MEM_CMA_ALLOC

#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
struct device *s3c_mem_cma_dev;
static struct device *s3c_mem_get_dev(void)
{
	return s3c_mem_cma_dev;
}

#else
struct s3c_slot_info *s3c_slot_info;
int s3c_cma_max_block_num;
int s3c_cma_block_size;


static void s3c_mem_log(struct s3c_dev_info *prv_data, bool mem_info)
{
	int i = 0;
	for (i = 0; i < prv_data->dev_max_slot_num; i++)
		printk(KERN_INFO
		       "s_slot_info[%d].s_start_addr=0x%x s_mapped=%d\n", i,
		       prv_data->s_slot_info[i].s_start_addr,
		       prv_data->s_slot_info[i].s_mapped);
	if (mem_info)
		printk(KERN_INFO
		       "s_cur_mem_info->paddr=0x%x s_mem_info->vaddr=0x%x s_mem_info->size=%d\n",
		       prv_data->s_cur_mem_info.paddr,
		       prv_data->s_cur_mem_info.vaddr,
		       prv_data->s_cur_mem_info.mapped_size);
}

static unsigned long s3c_mapping_slot(struct s3c_dev_info *prv_data)
{
	int i, j, k, v_start_slot = 0;
	unsigned long lv_ret = 0;

	for (i = 0; i < prv_data->dev_max_slot_num; i++) {
		if (prv_data->s_slot_info[i].s_mapped == false) {
			if (i + prv_data->s_cur_mem_info.req_memblock >
			    prv_data->dev_max_slot_num) {
				printk(KERN_ERR "ERROR : not enough memory\n");
				return lv_ret;
			}
			v_start_slot = i;
			for (j = i;
			     j < i + prv_data->s_cur_mem_info.req_memblock;
			     j++) {
				if (prv_data->s_slot_info[j].s_mapped == true)
					break;
			}
			if (j == i + prv_data->s_cur_mem_info.req_memblock) {
				lv_ret =
				    __phys_to_pfn(prv_data->s_slot_info
						  [v_start_slot].s_start_addr);
				physical_address = (unsigned int)
				    prv_data->s_slot_info[v_start_slot].
				    s_start_addr;
				for (k = v_start_slot; k < j; k++) {
					prv_data->s_slot_info[k].s_mapped =
					    true;
					printk(KERN_INFO
					       "prv_data->s_slot_info[%d].s_mapped=1\n",
					       k);
				}
				break;
			}
		} else
			continue;
	}
	if (i == prv_data->dev_max_slot_num)
		printk(KERN_ERR "ERROR :can not find the suitable slot\n");

	return lv_ret;
}

static int s3c_unmapping_slot(struct s3c_dev_info *prv_data)
{
	int i, j, lv_ret = 0;
	for (i = 0; i < prv_data->dev_max_slot_num; i++) {
		if (prv_data->s_slot_info[i].s_start_addr ==
		    prv_data->s_cur_mem_info.paddr) {
			for (j = i;
			     j < i + prv_data->s_cur_mem_info.req_memblock;
			     j++) {
				prv_data->s_slot_info[j].s_mapped = false;
				printk(KERN_INFO
				       "s_slot_info[%d].s_mapped = 0\n", j);
			}
		}
	}
	return lv_ret;
}
#endif

int s3c_mem_open(struct inode *inode, struct file *filp)
{
	struct s3c_dev_info *prv_data;
	mutex_lock(&mem_open_lock);

	prv_data = kzalloc(sizeof(struct s3c_dev_info), GFP_KERNEL);
	if (!prv_data) {
		pr_err("%s: not enough memory\n", __func__);
		return -ENOMEM;
	}
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
	prv_data->s3c_mem_cma_dev = s3c_mem_get_dev();
	prv_data->s_cur_mem_info.phy_addr = 0;
	prv_data->s_cur_mem_info.vir_addr = 0;
	prv_data->s_cur_mem_info.size = 0;
#else
	prv_data->s_slot_info = s3c_slot_info;
	prv_data->dev_slot_size = s3c_cma_block_size;
	prv_data->dev_max_slot_num = s3c_cma_max_block_num;
	prv_data->s_cur_mem_info.paddr = 0;
	prv_data->s_cur_mem_info.vaddr = 0;
	prv_data->s_cur_mem_info.mapped_size = 0;
	prv_data->s_cur_mem_info.req_memblock = 0;
#endif
	filp->private_data = prv_data;

	mutex_unlock(&mem_open_lock);

	return 0;
}

int s3c_mem_release(struct inode *inode, struct file *filp)
{
	struct mm_struct *mm = current->mm;
	struct s3c_dev_info *prv_data =
	    (struct s3c_dev_info *)filp->private_data;

	mutex_lock(&mem_release_lock);
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
	if (prv_data->s_cur_mem_info.vir_addr) {
		if (do_munmap
		    (mm, prv_data->s_cur_mem_info.vir_addr,
		     prv_data->s_cur_mem_info.size) < 0) {
			printk(KERN_ERR "do_munmap() failed !!\n");
			mutex_unlock(&mem_free_lock);
			return -EINVAL;
		}
		if (prv_data->s_cur_mem_info.phy_addr)
			cma_free(prv_data->s_cur_mem_info.phy_addr);
		prv_data->s_cur_mem_info.vir_addr = 0;
		prv_data->s_cur_mem_info.size = 0;
		prv_data->s_cur_mem_info.phy_addr = 0;
		DEBUG("do_munmap() succeed !!\n");
	}
#else

	printk(KERN_INFO
	       "prv_data->s_cur_mem_info->paddr=0x%x vaddr=0x%x size=%d\n",
	       prv_data->s_cur_mem_info.paddr, prv_data->s_cur_mem_info.vaddr,
	       prv_data->s_cur_mem_info.mapped_size);

	if (prv_data->s_cur_mem_info.vaddr) {
		s3c_unmapping_slot(prv_data);
		if (do_munmap
		    (mm, prv_data->s_cur_mem_info.vaddr,
		     prv_data->s_cur_mem_info.mapped_size) < 0) {
			printk(KERN_ERR "do_munmap() failed !!\n");
			mutex_unlock(&mem_release_lock);
			return -EINVAL;
		}
	}
#endif
	kfree(filp->private_data);
	filp->private_data = NULL;
	mutex_unlock(&mem_release_lock);

	return 0;
}
#endif

long s3c_mem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
#ifdef USE_DMA_ALLOC
	unsigned long virt_addr;
#else
	unsigned long *virt_addr;
#endif

	struct mm_struct *mm = current->mm;
	struct s3c_mem_alloc param;
	struct vm_area_struct *vma;
	unsigned long start, this_pfn;
#ifdef CONFIG_S3C_DMA_MEM
	struct s3c_mem_dma_param dma_param;
#endif

	switch (cmd) {
	case S3C_MEM_ALLOC:
		mutex_lock(&mem_alloc_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_alloc_lock);
			return -EFAULT;
		}
		flag = MEM_ALLOC;
		param.vir_addr = do_mmap(file, 0, param.size,
				(PROT_READ|PROT_WRITE), MAP_SHARED, 0);
		DEBUG("param.vir_addr = %08x, %d\n",
						param.vir_addr, __LINE__);
		if (param.vir_addr == -EINVAL) {
			printk(KERN_INFO "S3C_MEM_ALLOC FAILED\n");
			flag = 0;
			mutex_unlock(&mem_alloc_lock);
			return -EFAULT;
		}
		param.phy_addr = physical_address;
#ifdef USE_DMA_ALLOC
		param.kvir_addr = virtual_address;
#endif

		DEBUG("KERNEL MALLOC : param.phy_addr = 0x%X \t "
		      "size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			flag = 0;
			mutex_unlock(&mem_alloc_lock);
			return -EFAULT;
		}
		flag = 0;
		mutex_unlock(&mem_alloc_lock);

		break;
#ifdef CONFIG_S3C_MEM_CMA_ALLOC
	case S3C_MEM_CMA_ALLOC:
		{
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
			struct cma_info mem_info;
			int err;
#endif
			struct s3c_dev_info *prv_data =
			    (struct s3c_dev_info *)file->private_data;

			mutex_lock(&mem_alloc_lock);
			if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
					   sizeof(struct s3c_mem_alloc))) {
				mutex_unlock(&mem_alloc_lock);
				return -EFAULT;
			}
			flag = MEM_CMA_ALLOC;

#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
			err = cma_info(&mem_info, prv_data->s3c_mem_cma_dev, 0);
			printk(KERN_DEBUG "%s : [cma_info] start_addr : 0x%x, end_addr	: 0x%x, "
				"total_size : 0x%x, free_size : 0x%x req_size : 0x%x\n",
				__func__, mem_info.lower_bound,
				mem_info.upper_bound, mem_info.total_size,
				mem_info.free_size, param.size);

			if (err || (mem_info.free_size < param.size)) {
				printk(KERN_ERR "%s: get cma info failed\n",
					__func__);
				mutex_unlock(&mem_alloc_lock);
				return -ENOMEM;
			}
			param.phy_addr = (dma_addr_t) cma_alloc
			    (prv_data->s3c_mem_cma_dev, "dma",
			     (size_t) param.size, 0);

			printk(KERN_INFO "param.phy_addr = 0x%x\n",
			       param.phy_addr);
			if (!param.phy_addr) {
				printk(KERN_ERR "%s: cma_alloc failed\n",
					__func__);

				mutex_unlock(&mem_alloc_lock);
				return -ENOMEM;
			}
			prv_data->s_cur_mem_info.size = param.size;
			prv_data->s_cur_mem_info.phy_addr = param.phy_addr;
#endif

			param.vir_addr =
			    do_mmap(file, 0, param.size,
				    (PROT_READ | PROT_WRITE), MAP_SHARED, 0);
			DEBUG("param.vir_addr = %08x, %d\n", param.vir_addr,
			      __LINE__);

			if (param.vir_addr == -EINVAL) {
				printk(KERN_ERR "S3C_MEM_ALLOC FAILED\n");
				flag = 0;
				mutex_unlock(&mem_alloc_lock);
				return -EFAULT;
			}
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
			prv_data->s_cur_mem_info.vir_addr = param.vir_addr;
#else
			param.phy_addr = physical_address;
			printk(KERN_INFO "physical_address=0x%x\n",
			       physical_address);
			prv_data->s_cur_mem_info.paddr = param.phy_addr;
			prv_data->s_cur_mem_info.vaddr = param.vir_addr;
			prv_data->s_cur_mem_info.mapped_size =
			    PAGE_ALIGN(param.size);
#endif
			if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
					 sizeof(struct s3c_mem_alloc))) {
				flag = 0;
				mutex_unlock(&mem_alloc_lock);
				return -EFAULT;
			}
			flag = 0;

			mutex_unlock(&mem_alloc_lock);
		}
		break;

#endif
	case S3C_MEM_CACHEABLE_ALLOC:
		mutex_lock(&mem_cacheable_alloc_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_cacheable_alloc_lock);
			return -EFAULT;
		}
		flag = MEM_ALLOC_CACHEABLE;
		param.vir_addr = do_mmap(file, 0, param.size,
				(PROT_READ|PROT_WRITE), MAP_SHARED, 0);
		DEBUG("param.vir_addr = %08x, %d\n",
				param.vir_addr, __LINE__);
		if (param.vir_addr == -EINVAL) {
			printk(KERN_INFO "S3C_MEM_ALLOC FAILED\n");
			flag = 0;
			mutex_unlock(&mem_cacheable_alloc_lock);
			return -EFAULT;
		}
		param.phy_addr = physical_address;
		DEBUG("KERNEL MALLOC : param.phy_addr = 0x%X"
		      " \t size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			flag = 0;
			mutex_unlock(&mem_cacheable_alloc_lock);
			return -EFAULT;
		}
		flag = 0;
		mutex_unlock(&mem_cacheable_alloc_lock);

		break;

	case S3C_MEM_SHARE_ALLOC:
		mutex_lock(&mem_share_alloc_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_share_alloc_lock);
			return -EFAULT;
		}
		flag = MEM_ALLOC_SHARE;
		physical_address = param.phy_addr;
		DEBUG("param.phy_addr = %08x, %d\n",
		      physical_address, __LINE__);
		param.vir_addr = do_mmap(file, 0, param.size,
				(PROT_READ|PROT_WRITE), MAP_SHARED, 0);
		DEBUG("param.vir_addr = %08x, %d\n",
				param.vir_addr, __LINE__);
		if (param.vir_addr == -EINVAL) {
			printk(KERN_INFO "S3C_MEM_SHARE_ALLOC FAILED\n");
			flag = 0;
			mutex_unlock(&mem_share_alloc_lock);
			return -EFAULT;
		}
		DEBUG("MALLOC_SHARE : param.phy_addr = 0x%X \t "
		      "size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			flag = 0;
			mutex_unlock(&mem_share_alloc_lock);
			return -EFAULT;
		}
		flag = 0;
		mutex_unlock(&mem_share_alloc_lock);

		break;

	case S3C_MEM_CACHEABLE_SHARE_ALLOC:
		mutex_lock(&mem_cacheable_share_alloc_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_cacheable_share_alloc_lock);
			return -EFAULT;
		}
		flag = MEM_ALLOC_CACHEABLE_SHARE;
		physical_address = param.phy_addr;
		DEBUG("param.phy_addr = %08x, %d\n",
		      physical_address, __LINE__);
		param.vir_addr = do_mmap(file, 0, param.size,
				(PROT_READ|PROT_WRITE), MAP_SHARED, 0);
		DEBUG("param.vir_addr = %08x, %d\n",
				param.vir_addr, __LINE__);
		if (param.vir_addr == -EINVAL) {
			printk(KERN_INFO "S3C_MEM_SHARE_ALLOC FAILED\n");
			flag = 0;
			mutex_unlock(&mem_cacheable_share_alloc_lock);
			return -EFAULT;
		}
		DEBUG("MALLOC_SHARE : param.phy_addr = 0x%X \t "
		      "size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			flag = 0;
			mutex_unlock(&mem_cacheable_share_alloc_lock);
			return -EFAULT;
		}
		flag = 0;
		mutex_unlock(&mem_cacheable_share_alloc_lock);

		break;

	case S3C_MEM_FREE:
		mutex_lock(&mem_free_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_free_lock);
			return -EFAULT;
		}

		DEBUG("KERNEL FREE : param.phy_addr = 0x%X \t "
		      "size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (do_munmap(mm, param.vir_addr, param.size) < 0) {
			printk(KERN_INFO "do_munmap() failed !!\n");
			mutex_unlock(&mem_free_lock);
			return -EINVAL;
		}

#ifdef USE_DMA_ALLOC
		virt_addr = param.kvir_addr;
		dma_free_writecombine(NULL, param.size,
				(unsigned int *) virt_addr, param.phy_addr);
#else
		virt_addr = (unsigned long *)phys_to_virt(param.phy_addr);
		kfree(virt_addr);
#endif
		param.size = 0;
		DEBUG("do_munmap() succeed !!\n");

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_free_lock);
			return -EFAULT;
		}

		mutex_unlock(&mem_free_lock);

		break;
#ifdef CONFIG_S3C_MEM_CMA_ALLOC
	case S3C_MEM_CMA_FREE:
		{
			struct s3c_dev_info *prv_data =
			    (struct s3c_dev_info *)file->private_data;

			mutex_lock(&mem_free_lock);
			if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
					   sizeof(struct s3c_mem_alloc))) {
				mutex_unlock(&mem_free_lock);
				return -EFAULT;
			}

			DEBUG("KERNEL FREE : param.phy_addr = 0x%X \t "
			      "size = %d \t param.vir_addr = 0x%X, %d\n",
			      param.phy_addr, param.size, param.vir_addr,
			      __LINE__);

			printk
			    ("FREE : pa = 0x%x size = %d va = 0x%x\n",
			     param.phy_addr, param.size, param.vir_addr);
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
			if (param.vir_addr) {
				if (do_munmap(mm, param.vir_addr, param.size) <
				    0) {
					printk(KERN_ERR
					       "do_munmap() failed !!\n");
					mutex_unlock(&mem_free_lock);
					return -EINVAL;
				}
				if (prv_data->s_cur_mem_info.phy_addr) {
					cma_free(prv_data->
						 s_cur_mem_info.phy_addr);
				}

				param.size = 0;
				prv_data->s_cur_mem_info.vir_addr = 0;
				prv_data->s_cur_mem_info.size = 0;
				prv_data->s_cur_mem_info.phy_addr = 0;
				DEBUG("do_munmap() succeed !!\n");
			}
#else
			if (param.vir_addr) {
				s3c_unmapping_slot(prv_data);

				if (do_munmap(mm, param.vir_addr, param.size) <
				    0) {
					printk(KERN_ERR
					       "do_munmap() failed !!\n");
					mutex_unlock(&mem_free_lock);
					return -EINVAL;
				}
				param.size = 0;
				prv_data->s_cur_mem_info.paddr = 0;
				prv_data->s_cur_mem_info.vaddr = 0;
				prv_data->s_cur_mem_info.mapped_size = 0;
				prv_data->s_cur_mem_info.req_memblock = 0;
				DEBUG("do_munmap() succeed !!\n");
			}
#endif
			if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
					 sizeof(struct s3c_mem_alloc))) {
				mutex_unlock(&mem_free_lock);
				return -EFAULT;
			}

			mutex_unlock(&mem_free_lock);
		}
		break;
#endif
	case S3C_MEM_SHARE_FREE:
		mutex_lock(&mem_share_free_lock);
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_share_free_lock);
			return -EFAULT; }

		DEBUG("MEM_SHARE_FREE : param.phy_addr = 0x%X \t "
		      "size = %d \t param.vir_addr = 0x%X, %d\n",
				param.phy_addr, param.size, param.vir_addr,
				__LINE__);

		if (do_munmap(mm, param.vir_addr, param.size) < 0) {
			printk(KERN_INFO "do_munmap() failed - MEM_SHARE_FREE!!\n");
			mutex_unlock(&mem_share_free_lock);
			return -EINVAL;
		}

		param.vir_addr = 0;
		DEBUG("do_munmap() succeed !! - MEM_SHARE_FREE\n");

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			mutex_unlock(&mem_share_free_lock);
			return -EFAULT;
		}

		mutex_unlock(&mem_share_free_lock);

		break;

#ifdef CONFIG_S3C_DMA_MEM
	case S3C_MEM_DMA_COPY:
		if (copy_from_user(&dma_param, (struct s3c_mem_dma_param *)arg,
				   sizeof(struct s3c_mem_dma_param))) {
			return -EFAULT;
		}
		if (s3c_dma_mem_start(current->mm, &dma_param,
				S3C_DMA_MEM2MEM)) {
			return -EINVAL;
		}
		if (copy_to_user((struct s3c_mem_dma_param *)arg, &dma_param,
				 sizeof(struct s3c_mem_dma_param))) {
			return -EFAULT;
		}
		break;
#endif

	case S3C_MEM_GET_PADDR:
		if (copy_from_user(&param, (struct s3c_mem_alloc *)arg,
				   sizeof(struct s3c_mem_alloc))) {
			return -EFAULT;
		}
		start = param.vir_addr;
		down_read(&mm->mmap_sem);
		vma = find_vma(mm, start);

		if (vma == NULL) {
			up_read(&mm->mmap_sem);
			return -EINVAL;
		}

		if (follow_pfn(vma, start, &this_pfn)) {
			up_read(&mm->mmap_sem);
			return -EINVAL;
		}

		param.phy_addr = this_pfn << PAGE_SHIFT;
		up_read(&mm->mmap_sem);

		if (copy_to_user((struct s3c_mem_alloc *)arg, &param,
				 sizeof(struct s3c_mem_alloc))) {
			return -EFAULT;
		}
		break;

	default:
		DEBUG("s3c_mem_ioctl() : default !!\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(s3c_mem_ioctl);

int s3c_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long pageFrameNo = 0, size, phys_addr;

#ifdef USE_DMA_ALLOC
	unsigned long virt_addr;
#else
	unsigned long *virt_addr;
#endif

	size = vma->vm_end - vma->vm_start;

	switch (flag) {
	case MEM_ALLOC:
	case MEM_ALLOC_CACHEABLE:

#ifdef USE_DMA_ALLOC
		virt_addr = (unsigned long)dma_alloc_writecombine(NULL, size,
				(unsigned int *) &phys_addr,
								  GFP_KERNEL);
#else
		virt_addr = kmalloc(size, GFP_DMA | GFP_ATOMIC);
#endif
		if (!virt_addr) {
			printk(KERN_INFO "kmalloc() failed !\n");
			return -EINVAL;
		}
		DEBUG("MMAP_KMALLOC : virt addr = 0x%08x, size = %d, %d\n",
		      virt_addr, size, __LINE__);

#ifndef USE_DMA_ALLOC
		dmac_map_area(virt_addr, size / sizeof(unsigned long), 2);
		phys_addr = virt_to_phys((unsigned long *)virt_addr);
#endif
		physical_address = (unsigned int)phys_addr;

#ifdef USE_DMA_ALLOC
		virtual_address = virt_addr;
#endif
		pageFrameNo = __phys_to_pfn(phys_addr);
		break;
#ifdef CONFIG_S3C_MEM_CMA_ALLOC
	case MEM_CMA_ALLOC:
		{
			struct s3c_dev_info *prv_data =
			    (struct s3c_dev_info *)filp->private_data;
			vma->vm_page_prot =
			    pgprot_writecombine(vma->vm_page_prot);
#ifdef CONFIG_VIDEO_SAMSUNG_USE_DMA_MEM
			pageFrameNo =
			    __phys_to_pfn(prv_data->s_cur_mem_info.phy_addr);
#else
			prv_data->s_cur_mem_info.req_memblock =
			    PAGE_ALIGN(size) / prv_data->dev_slot_size;

			if (PAGE_ALIGN(size) % prv_data->dev_slot_size)
				prv_data->s_cur_mem_info.req_memblock++;

			printk(KERN_INFO "required slot=%d size=%lu\n",
			       prv_data->s_cur_mem_info.req_memblock, size);


			pageFrameNo = s3c_mapping_slot(prv_data);
#endif
			if (!pageFrameNo) {
				printk(KERN_ERR "mapping failed !\n");
				return -EINVAL;
			}

		}
		break;
#endif
	case MEM_ALLOC_SHARE:
	case MEM_ALLOC_CACHEABLE_SHARE:
		DEBUG("MMAP_KMALLOC_SHARE : phys addr = 0x%08x, %d\n",
		      physical_address, __LINE__);

/* page frame number of the address for the physical_address to be shared. */
		pageFrameNo = __phys_to_pfn(physical_address);
		DEBUG("MMAP_KMALLOC_SHARE : vma->end = 0x%08x, "
		      "vma->start = 0x%08x, size = %d, %d\n",
		      vma->vm_end, vma->vm_start, size, __LINE__);
		break;

	default:
		break;
	}

	if ((flag == MEM_ALLOC) || (flag == MEM_ALLOC_SHARE))
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	vma->vm_flags |= VM_RESERVED;

	if (remap_pfn_range(vma, vma->vm_start, pageFrameNo,
			    size, vma->vm_page_prot)) {
		printk(KERN_INFO "s3c_mem_mmap() : remap_pfn_range() failed !\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(s3c_mem_mmap);
