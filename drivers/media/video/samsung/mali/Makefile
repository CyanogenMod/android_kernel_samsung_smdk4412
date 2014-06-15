#
# Copyright (C) 2010-2012 ARM Limited. All rights reserved.
# 
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
# 
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

OSKOS :=linux
FILES_PREFIX=
MALI_INCLUDE_PREFIX := drivers/media/video/samsung/mali/
KBUILDROOT =

ifeq ($(CONFIG_MALI_DED_ONLY),y)
USING_OS_MEMORY=0
USING_MMU=0
USING_DED=1
endif

ifeq ($(CONFIG_MALI_DED_MMU),y)
USING_OS_MEMORY=0
USING_MMU=1
USING_DED=1
endif

ifeq ($(CONFIG_MALI_OSMEM_ONLY),y)
USING_MMU=1
USING_DED=0
USING_OS_MEMORY=1
endif

ifeq ($(CONFIG_MALI_DED_OSMEM),y)
USING_MMU=1
USING_DED=1
USING_OS_MEMORY=1
endif

ifeq ($(CONFIG_PM),y)
	USING_PMM = 1
ifeq ($(CONFIG_PM_RUNTIME),y)
	KERNEL_RUNTIME_PM_ENABLED = 1
endif
endif

ifeq ($(CONFIG_VIDEO_MALI400MP_DVFS),y)
USING_GPU_UTILIZATION=1
USING_MALI_DVFS_ENABLED=1
endif

ifeq ($(CONFIG_VIDEO_UMP_DEBUG),y)
BUILD=debug
endif

ifeq ($(CONFIG_VIDEO_MALI400MP_STREAMLINE_PROFILING),y)
USING_PROFILING=1
USING_TRACEPOINTS=1
endif

# set up defaults if not defined by the user
USE_UMPV2 ?= 0
PANIC_ON_WATCHDOG_TIMEOUT ?= 1 
USING_MALI400 ?= 1
USING_MMU ?= 1
USING_DED ?= 0
USING_UMP ?= 0
ONLY_ZBT ?= 0
USING_ZBT ?= 0
USING_OS_MEMORY ?= 1
USING_PMM ?= 0
USING_MALI_RUN_TIME_PM ?= 0
USING_MALI_PMM_TESTSUITE ?= 0
USING_MALI_PMU ?= 0
USING_GPU_UTILIZATION ?= 0
OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB ?= 16
USING_PROFILING ?= 0
USING_INTERNAL_PROFILING ?= 0
USING_TRACEPOINTS ?= 0
USING_MALI_MAJOR_PREDEFINE = 1
USING_MALI_DVFS_ENABLED ?= 1
USING_MALI_PMM_EARLYSUSPEND ?= 0
#USING_KERNEL_WITH_DMA_ALLOC_PHYS_PAGE ?= 0
#CONFIG_MALI_MEM_SIZE ?= 512
DISABLE_PP0 ?= 0
DISABLE_PP1 ?= 0
DISABLE_PP2 ?= 0
DISABLE_PP3 ?= 0
PROFILING_SKIP_PP_JOBS ?= 0
PROFILING_SKIP_PP_AND_GP_JOBS ?= 0
PROFILING_PRINT_L2_HITRATE_ON_GP_FINISH ?= 0
TIMESTAMP ?= default
BUILD ?= release 
TARGET_PLATFORM ?= default
KERNEL_RUNTIME_PM_ENABLED ?= 0
MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP ?= 0
MALI_PP_SCHEDULER_KEEP_SUB_JOB_STARTS_ALIGNED ?= 0
MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS ?= 0

# Get path to driver source from Linux build system
ifeq ($(USING_PROFILING),1)
ifeq ($(USING_INTERNAL_PROFILING),0)
ifndef CONFIG_TRACEPOINTS
# Should default to gator profiling, but we dont have the kernel feature required, so disable profiling
override USING_PROFILING = 0
$(warning "CONFIG_TRACEPOINTS required for USING_PROFILING")
endif
endif
endif

ifeq ($(USING_PROFILING),0)
# make sure user hasnt selected incompatible flags
override USING_INTERNAL_PROFILING = 0
endif

USING_MALI_SLP_GLOBAL_LOCK ?= 0

#config validtion check
ifeq ($(USING_OS_MEMORY),1)
        USING_MMU = 1
endif

# Check if a Mali Core sub module should be enabled, true or false returned
#submodule_enabled = $(shell gcc $(DEFINES) -E $(FILES_PREFIX)/arch/config.h | grep type | grep -c $(2))

# Inside the kernel build system

# This conditional makefile exports the global definition ARM_INTERNAL_BUILD. Customer releases will not include arm_internal.mak
-include ../../../arm_internal.mak

# Set up our defines, which will be passed to gcc
DEFINES += -DONLY_ZBT=$(ONLY_ZBT)
DEFINES += -DUSING_ZBT=$(USING_ZBT)
DEFINES += -DUSING_MMU=$(USING_MMU)
DEFINES += -DUSING_OS_MEMORY=$(USING_OS_MEMORY)
DEFINES += -DUSING_DED=$(USING_DED)
DEFINES += -DUSING_UMP=$(USING_UMP)
DEFINES += -D_MALI_OSK_SPECIFIC_INDIRECT_MMAP
DEFINES += -DUSING_MALI_PMU=$(USING_MALI_PMU)
DEFINES += -DMALI_PMM_RUNTIME_JOB_CONTROL_ON=$(KERNEL_RUNTIME_PM_ENABLED)
DEFINES += -DUSING_MALI_PMM=$(USING_PMM)
DEFINES += -DMALI_GPU_UTILIZATION=$(USING_GPU_UTILIZATION)
DEFINES += -DCONFIG_MALI_MEM_SIZE=$(CONFIG_MALI_MEM_SIZE)
DEFINES += -D_MALI_OSK_SPECIFIC_INDIRECT_MMAP
DEFINES += -DMALI_INTERNAL_TIMELINE_PROFILING_ENABLED=$(USING_INTERNAL_PROFILING)
DEFINES += -DMALI_MAJOR_PREDEFINE=$(USING_MALI_MAJOR_PREDEFINE)
DEFINES += -DMALI_DVFS_ENABLED=$(USING_MALI_DVFS_ENABLED)
DEFINES += -DUSING_MALI_PMM_EARLYSUSPEND=$(USING_MALI_PMM_EARLYSUSPEND)
DEFINES += -DMALI_STATE_TRACKING=0
DEFINES += -DMALI_OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB=$(OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB)
DEFINES += -DMALI_TRACEPOINTS_ENABLED=$(USING_TRACEPOINTS)
DEFINES += -DDISABLE_PP0=$(DISABLE_PP0)
DEFINES += -DDISABLE_PP1=$(DISABLE_PP1)
DEFINES += -DDISABLE_PP2=$(DISABLE_PP2)
DEFINES += -DDISABLE_PP3=$(DISABLE_PP3)
DEFINES += -DPROFILING_SKIP_PP_JOBS=$(PROFILING_SKIP_PP_JOBS)
DEFINES += -DPROFILING_SKIP_PP_AND_GP_JOBS=$(PROFILING_SKIP_PP_AND_GP_JOBS)
DEFINES += -DPROFILING_PRINT_L2_HITRATE_ON_GP_FINISH=$(PROFILING_PRINT_L2_HITRATE_ON_GP_FINISH)
DEFINES += -DMALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP=$(MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP)
DEFINES += -DMALI_PP_SCHEDULER_KEEP_SUB_JOB_STARTS_ALIGNED=$(MALI_PP_SCHEDULER_KEEP_SUB_JOB_STARTS_ALIGNED)
DEFINES += -DMALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS=$(MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS)
DEFINES += -DMALI_TIMELINE_PROFILING_ENABLED=$(USING_PROFILING)
DEFINES += -DMALI_POWER_MGMT_TEST_SUITE=$(USING_MALI_PMM_TESTSUITE)
DEFINES += -DMALI_OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB=$(OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB)

ifeq ($(BUILD),debug)
DEFINES += -DDEBUG
endif

# Linux has its own mmap cleanup handlers (see mali_kernel_mem_mmu.o)
DEFINES += -DMALI_UKK_HAS_IMPLICIT_MMAP_CLEANUP

# UMP
ifeq ($(CONFIG_VIDEO_UMP),y)
	DEFINES += -DMALI_USE_UNIFIED_MEMORY_PROVIDER=1
	EXTRA_CFLAGS += -I$(MALI_INCLUDE_PREFIX)../ump/include
else
	DEFINES += -DMALI_USE_UNIFIED_MEMORY_PROVIDER=0
endif

# Target build file
obj-$(CONFIG_VIDEO_UMP)	+= mali.o

# Use our defines when compiling
# MALI
INCLUDES = \
		-I$(MALI_INCLUDE_PREFIX)\
		-I$(MALI_INCLUDE_PREFIX)include \
		-I$(MALI_INCLUDE_PREFIX)common \
		-I$(MALI_INCLUDE_PREFIX)linux \
		-I$(MALI_INCLUDE_PREFIX)platform\
		-I$(MALI_INCLUDE_PREFIX)regs

EXTRA_CFLAGS += $(INCLUDES)\
		$(DEFINES)

EXTRA_CFLAGS += -I$(MALI_INCLUDE_PREFIX)linux/license/gpl
EXTRA_CFLAGS += -I$(MALI_INCLUDE_PREFIX)common/pmm

# Source files which always are included in a build
ifeq ($(CONFIG_VIDEO_UMP),y)
OSKFILES=\
        $(FILES_PREFIX)$(OSKOS)/mali_osk_irq.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_wait_queue.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_low_level_mem.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_mali.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_notification.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_time.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_timers.o
else
OSKFILES=\
        $(FILES_PREFIX)$(OSKOS)/mali_osk_atomics.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_irq.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_locks.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_wait_queue.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_low_level_mem.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_math.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_memory.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_misc.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_mali.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_notification.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_time.o \
        $(FILES_PREFIX)$(OSKOS)/mali_osk_timers.o
endif #($(CONFIG_VIDEO_UMP),y)

ifeq ($(CONFIG_CPU_EXYNOS4210),y)
	MALI_PLATFORM_DIR = platform/orion-m400
else
	MALI_PLATFORM_DIR = platform/pegasus-m400
endif #($(CONFIG_CPU_EXYNOS4210),y)

MALI_PLATFORM_FILE=$(MALI_PLATFORM_DIR)/mali_platform.o
UKKFILES=\
        $(FILES_PREFIX)$(OSKOS)/mali_ukk_mem.o \
        $(FILES_PREFIX)$(OSKOS)/mali_ukk_gp.o \
        $(FILES_PREFIX)$(OSKOS)/mali_ukk_pp.o \
        $(FILES_PREFIX)$(OSKOS)/mali_ukk_core.o

ifeq ($(USING_PROFILING),1)
UKKFILES += \
	$(FILES_PREFIX)$(OSKOS)/mali_ukk_profiling.o
endif

mali-y := \
	common/mali_kernel_core.o \
	linux/mali_kernel_linux.o \
	common/mali_kernel_descriptor_mapping.o \
	common/mali_session.o \
	common/mali_device_pause_resume.o \
	common/mali_kernel_vsync.o \
	linux/mali_ukk_vsync.o \
	linux/mali_kernel_sysfs.o \
	common/mali_mmu.o \
	common/mali_mmu_page_directory.o \
	common/mali_memory.o \
	common/mali_kernel_memory_engine.o \
	common/mali_block_allocator.o \
	common/mali_kernel_mem_os.o \
	common/mali_mem_validation.o \
	common/mali_hw_core.o \
	common/mali_gp.o \
	common/mali_pp.o \
	common/mali_pp_job.o \
	common/mali_gp_job.o \
	common/mali_scheduler.o \
	common/mali_gp_scheduler.o \
	common/mali_pp_scheduler.o \
	common/mali_cluster.o \
	common/mali_group.o \
	common/mali_dlbu.o \
	common/mali_pm.o \
	common/mali_pmu.o \
	common/mali_user_settings_db.o \
	$(OSKOS)/mali_osk_pm.o \
	linux/mali_kernel_pm.o \
	linux/mali_pmu_power_up_down.o \
	$(MALI_PLATFORM_FILE) \
	$(OSKFILES) \
	$(UKKFILES)
#	__malidrv_build_info.c

# Selecting files to compile by parsing the config file

ifeq ($(USING_INTERNAL_PROFILING),1)
PROFILING_BACKEND_SOURCES = \
	linux/mali_osk_profiling_internal.o \
	timestamp-$(TIMESTAMP)/mali_timestamp.o
EXTRA_CFLAGS += -I$(MALI_INCLUDE_PREFIX)timestamp-$(TIMESTAMP)
else
ifeq ($(USING_PROFILING),1)
PROFILING_BACKEND_SOURCES = \
	linux/mali_osk_profiling_gator.o
endif
endif

# Add the profiling sources
mali-y += $(PROFILING_BACKEND_SOURCES)

# Mali-400 PP in use
ifeq ($(USING_MALI_PMM_TESTSUITE),1)
EXTRA_CFLAGS += -I$(MALI_INCLUDE_PREFIX)platform/mali_pmu_testing
endif

ifeq ($(USING_GPU_UTILIZATION),1)
EXTRA_CFLAGS += -DCONFIG_MALI400_GPU_UTILIZATION=1

mali-y += \
	common/mali_kernel_utilization.o
endif

ifeq ($(USING_MALI_DVFS_ENABLED),1)
mali-y += $(MALI_PLATFORM_DIR)/mali_platform_dvfs.o
endif #($(USING_MALI_DVFS_ENABLED),1)

EXTRA_CFLAGS += -DUSING_MALI400

# Mali Level2 cache in use
EXTRA_CFLAGS += -DUSING_MALI400_L2_CACHE
mali-y += common/mali_l2_cache.o

# Mali SLP Global lock feature
ifeq ($(USING_MALI_SLP_GLOBAL_LOCK),1)
mali-y += \
	linux/mali_slp_global_lock.o
endif


ifeq ($(PANIC_ON_WATCHDOG_TIMEOUT),1)
	EXTRA_CFLAGS += -DUSING_KERNEL_PANIC
endif

# Get subversion revision number, fall back to 0000 if no svn info is available
SVN_REV:=$(shell ((svnversion | grep -E "^[0-9]+" && echo -n 'Revision: ' && svnversion) || git svn info | sed -e 's/$$$$/M/' | grep '^Revision: ' || echo ${MALI_RELEASE_NAME}) 2>/dev/null | sed -e 's/^Revision: //')

EXTRA_CFLAGS += -DSVN_REV=$(SVN_REV)
EXTRA_CFLAGS += -DSVN_REV_STRING=\"$(SVN_REV)\"

