/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_uk_types.h
 * Defines the types and constants used in the user-kernel interface
 */

#ifndef __MALI_UK_TYPES_H__
#define __MALI_UK_TYPES_H__

/*
 * NOTE: Because this file can be included from user-side and kernel-side,
 * it is up to the includee to ensure certain typedefs (e.g. u32) are already
 * defined when #including this.
 */
#include "regs/mali_200_regs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup uddapi Unified Device Driver (UDD) APIs
 *
 * @{
 */

/**
 * @addtogroup u_k_api UDD User/Kernel Interface (U/K) APIs
 *
 * @{
 */

/** @defgroup _mali_uk_core U/K Core
 * @{ */

/** Definition of subsystem numbers, to assist in creating a unique identifier
 * for each U/K call.
 *
 * @see _mali_uk_functions */
typedef enum
{
    _MALI_UK_CORE_SUBSYSTEM,      /**< Core Group of U/K calls */
    _MALI_UK_MEMORY_SUBSYSTEM,    /**< Memory Group of U/K calls */
    _MALI_UK_PP_SUBSYSTEM,        /**< Fragment Processor Group of U/K calls */
    _MALI_UK_GP_SUBSYSTEM,        /**< Vertex Processor Group of U/K calls */
	_MALI_UK_PROFILING_SUBSYSTEM, /**< Profiling Group of U/K calls */
    _MALI_UK_PMM_SUBSYSTEM,       /**< Power Management Module Group of U/K calls */
	_MALI_UK_VSYNC_SUBSYSTEM,     /**< VSYNC Group of U/K calls */
} _mali_uk_subsystem_t;

/** Within a function group each function has its unique sequence number
 * to assist in creating a unique identifier for each U/K call.
 *
 * An ordered pair of numbers selected from
 * ( \ref _mali_uk_subsystem_t,\ref  _mali_uk_functions) will uniquely identify the
 * U/K call across all groups of functions, and all functions. */
typedef enum
{
	/** Core functions */

    _MALI_UK_OPEN                    = 0, /**< _mali_ukk_open() */
    _MALI_UK_CLOSE,                       /**< _mali_ukk_close() */
    _MALI_UK_GET_SYSTEM_INFO_SIZE,        /**< _mali_ukk_get_system_info_size() */
    _MALI_UK_GET_SYSTEM_INFO,             /**< _mali_ukk_get_system_info() */
    _MALI_UK_WAIT_FOR_NOTIFICATION,       /**< _mali_ukk_wait_for_notification() */
    _MALI_UK_GET_API_VERSION,             /**< _mali_ukk_get_api_version() */
    _MALI_UK_POST_NOTIFICATION,           /**< _mali_ukk_post_notification() */

	/** Memory functions */

    _MALI_UK_INIT_MEM                = 0, /**< _mali_ukk_init_mem() */
    _MALI_UK_TERM_MEM,                    /**< _mali_ukk_term_mem() */
    _MALI_UK_GET_BIG_BLOCK,               /**< _mali_ukk_get_big_block() */
    _MALI_UK_FREE_BIG_BLOCK,              /**< _mali_ukk_free_big_block() */
    _MALI_UK_MAP_MEM,                     /**< _mali_ukk_mem_mmap() */
    _MALI_UK_UNMAP_MEM,                   /**< _mali_ukk_mem_munmap() */
    _MALI_UK_QUERY_MMU_PAGE_TABLE_DUMP_SIZE, /**< _mali_ukk_mem_get_mmu_page_table_dump_size() */
    _MALI_UK_DUMP_MMU_PAGE_TABLE,         /**< _mali_ukk_mem_dump_mmu_page_table() */
    _MALI_UK_ATTACH_UMP_MEM,             /**< _mali_ukk_attach_ump_mem() */
    _MALI_UK_RELEASE_UMP_MEM,           /**< _mali_ukk_release_ump_mem() */
    _MALI_UK_MAP_EXT_MEM,                 /**< _mali_uku_map_external_mem() */
    _MALI_UK_UNMAP_EXT_MEM,               /**< _mali_uku_unmap_external_mem() */
    _MALI_UK_VA_TO_MALI_PA,               /**< _mali_uku_va_to_mali_pa() */

    /** Common functions for each core */

    _MALI_UK_START_JOB           = 0,     /**< Start a Fragment/Vertex Processor Job on a core */
	_MALI_UK_ABORT_JOB,                   /**< Abort a job */
    _MALI_UK_GET_NUMBER_OF_CORES,         /**< Get the number of Fragment/Vertex Processor cores */
    _MALI_UK_GET_CORE_VERSION,            /**< Get the Fragment/Vertex Processor version compatible with all cores */

    /** Fragment Processor Functions  */

    _MALI_UK_PP_START_JOB            = _MALI_UK_START_JOB,            /**< _mali_ukk_pp_start_job() */
    _MALI_UK_PP_ABORT_JOB            = _MALI_UK_ABORT_JOB,            /**< _mali_ukk_pp_abort_job() */
    _MALI_UK_GET_PP_NUMBER_OF_CORES  = _MALI_UK_GET_NUMBER_OF_CORES,  /**< _mali_ukk_get_pp_number_of_cores() */
    _MALI_UK_GET_PP_CORE_VERSION     = _MALI_UK_GET_CORE_VERSION,     /**< _mali_ukk_get_pp_core_version() */

    /** Vertex Processor Functions  */

    _MALI_UK_GP_START_JOB            = _MALI_UK_START_JOB,            /**< _mali_ukk_gp_start_job() */
    _MALI_UK_GP_ABORT_JOB            = _MALI_UK_ABORT_JOB,            /**< _mali_ukk_gp_abort_job() */
    _MALI_UK_GET_GP_NUMBER_OF_CORES  = _MALI_UK_GET_NUMBER_OF_CORES,  /**< _mali_ukk_get_gp_number_of_cores() */
    _MALI_UK_GET_GP_CORE_VERSION     = _MALI_UK_GET_CORE_VERSION,     /**< _mali_ukk_get_gp_core_version() */
    _MALI_UK_GP_SUSPEND_RESPONSE,                                     /**< _mali_ukk_gp_suspend_response() */

	/** Profiling functions */

	_MALI_UK_PROFILING_START         = 0, /**< __mali_uku_profiling_start() */
	_MALI_UK_PROFILING_ADD_EVENT,         /**< __mali_uku_profiling_add_event() */
	_MALI_UK_PROFILING_STOP,              /**< __mali_uku_profiling_stop() */
	_MALI_UK_PROFILING_GET_EVENT,         /**< __mali_uku_profiling_get_event() */
	_MALI_UK_PROFILING_CLEAR,             /**< __mali_uku_profiling_clear() */
	_MALI_UK_PROFILING_GET_CONFIG,        /**< __mali_uku_profiling_get_config() */
	_MALI_UK_TRANSFER_SW_COUNTERS,

#if USING_MALI_PMM
    /** Power Management Module Functions */
    _MALI_UK_PMM_EVENT_MESSAGE = 0,       /**< Raise an event message */
#endif

	/** VSYNC reporting fuctions */
	_MALI_UK_VSYNC_EVENT_REPORT      = 0, /**< _mali_ukk_vsync_event_report() */

} _mali_uk_functions;

/** @brief Get the size necessary for system info
 *
 * @see _mali_ukk_get_system_info_size()
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                       /**< [out] size of buffer necessary to hold system information data, in bytes */
} _mali_uk_get_system_info_size_s;


/** @defgroup _mali_uk_getsysteminfo U/K Get System Info
 * @{ */

/**
 * Type definition for the core version number.
 * Used when returning the version number read from a core
 *
 * Its format is that of the 32-bit Version register for a particular core.
 * Refer to the "Mali200 and MaliGP2 3D Graphics Processor Technical Reference
 * Manual", ARM DDI 0415C, for more information.
 */
typedef u32 _mali_core_version;

/**
 * Enum values for the different modes the driver can be put in.
 * Normal is the default mode. The driver then uses a job queue and takes job objects from the clients.
 * Job completion is reported using the _mali_ukk_wait_for_notification call.
 * The driver blocks this io command until a job has completed or failed or a timeout occurs.
 *
 * The 'raw' mode is reserved for future expansion.
 */
typedef enum _mali_driver_mode
{
	_MALI_DRIVER_MODE_RAW = 1,    /**< Reserved for future expansion */
	_MALI_DRIVER_MODE_NORMAL = 2  /**< Normal mode of operation */
} _mali_driver_mode;

/** @brief List of possible cores
 *
 * add new entries to the end of this enum */
typedef enum _mali_core_type
{
	_MALI_GP2 = 2,                /**< MaliGP2 Programmable Vertex Processor */
	_MALI_200 = 5,                /**< Mali200 Programmable Fragment Processor */
	_MALI_400_GP = 6,             /**< Mali400 Programmable Vertex Processor */
	_MALI_400_PP = 7,             /**< Mali400 Programmable Fragment Processor */
	/* insert new core here, do NOT alter the existing values */
} _mali_core_type;

/** @brief Information about each Mali Core
 *
 * Information is stored in a linked list, which is stored entirely in the
 * buffer pointed to by the system_info member of the
 * _mali_uk_get_system_info_s arguments provided to _mali_ukk_get_system_info()
 *
 * Both Fragment Processor (PP) and Vertex Processor (GP) cores are represented
 * by this struct.
 *
 * The type is reported by the type field, _mali_core_info::_mali_core_type.
 *
 * Each core is given a unique Sequence number identifying it, the core_nr
 * member.
 *
 * Flags are taken directly from the resource's flags, and are currently unused.
 *
 * Multiple mali_core_info structs are linked in a single linked list using the next field
 */
typedef struct _mali_core_info
{
	_mali_core_type type;            /**< Type of core */
	_mali_core_version version;      /**< Core Version, as reported by the Core's Version Register */
	u32 reg_address;                 /**< Address of Registers */
	u32 core_nr;                     /**< Sequence number */
	u32 flags;                       /**< Flags. Currently Unused. */
	struct _mali_core_info * next;   /**< Next core in Linked List */
} _mali_core_info;

/** @brief Capabilities of Memory Banks
 *
 * These may be used to restrict memory banks for certain uses. They may be
 * used when access is not possible (e.g. Bus does not support access to it)
 * or when access is possible but not desired (e.g. Access is slow).
 *
 * In the case of 'possible but not desired', there is no way of specifying
 * the flags as an optimization hint, so that the memory could be used as a
 * last resort.
 *
 * @see _mali_mem_info
 */
typedef enum _mali_bus_usage
{

	_MALI_PP_READABLE   = (1<<0),  /** Readable by the Fragment Processor */
	_MALI_PP_WRITEABLE  = (1<<1),  /** Writeable by the Fragment Processor */
	_MALI_GP_READABLE   = (1<<2),  /** Readable by the Vertex Processor */
	_MALI_GP_WRITEABLE  = (1<<3),  /** Writeable by the Vertex Processor */
	_MALI_CPU_READABLE  = (1<<4),  /** Readable by the CPU */
	_MALI_CPU_WRITEABLE = (1<<5),  /** Writeable by the CPU */
	_MALI_MMU_READABLE  = _MALI_PP_READABLE | _MALI_GP_READABLE,   /** Readable by the MMU (including all cores behind it) */
	_MALI_MMU_WRITEABLE = _MALI_PP_WRITEABLE | _MALI_GP_WRITEABLE, /** Writeable by the MMU (including all cores behind it) */
} _mali_bus_usage;

/** @brief Information about the Mali Memory system
 *
 * Information is stored in a linked list, which is stored entirely in the
 * buffer pointed to by the system_info member of the
 * _mali_uk_get_system_info_s arguments provided to _mali_ukk_get_system_info()
 *
 * Each element of the linked list describes a single Mali Memory bank.
 * Each allocation can only come from one bank, and will not cross multiple
 * banks.
 *
 * Each bank is uniquely identified by its identifier member. On Mali-nonMMU
 * systems, to allocate from this bank, the value of identifier must be passed
 * as the type_id member of the  _mali_uk_get_big_block_s arguments to
 * _mali_ukk_get_big_block.
 *
 * On Mali-MMU systems, there is only one bank, which describes the maximum
 * possible address range that could be allocated (which may be much less than
 * the available physical memory)
 *
 * The flags member describes the capabilities of the memory. It is an error
 * to attempt to build a job for a particular core (PP or GP) when the memory
 * regions used do not have the capabilities for supporting that core. This
 * would result in a job abort from the Device Driver.
 *
 * For example, it is correct to build a PP job where read-only data structures
 * are taken from a memory with _MALI_PP_READABLE set and
 * _MALI_PP_WRITEABLE clear, and a framebuffer with  _MALI_PP_WRITEABLE set and
 * _MALI_PP_READABLE clear. However, it would be incorrect to use a framebuffer
 * where _MALI_PP_WRITEABLE is clear.
 */
typedef struct _mali_mem_info
{
	u32 size;                     /**< Size of the memory bank in bytes */
	_mali_bus_usage flags;        /**< Capabilitiy flags of the memory */
	u32 maximum_order_supported;  /**< log2 supported size */
	u32 identifier;               /**< Unique identifier, to be used in allocate calls */
	struct _mali_mem_info * next; /**< Next List Link */
} _mali_mem_info;

/** @brief Info about the whole Mali system.
 *
 * This Contains a linked list of the cores and memory banks available. Each
 * list pointer will remain inside the system_info buffer supplied in the
 * _mali_uk_get_system_info_s arguments to a _mali_ukk_get_system_info call.
 *
 * The has_mmu member must be inspected to ensure the correct group of
 * Memory function calls is obtained - that is, those for either Mali-MMU
 * or Mali-nonMMU. @see _mali_uk_memory
 */
typedef struct _mali_system_info
{
	_mali_core_info * core_info;  /**< List of _mali_core_info structures */
	_mali_mem_info * mem_info;    /**< List of _mali_mem_info structures */
	u32 has_mmu;                  /**< Non-zero if Mali-MMU present. Zero otherwise. */
	_mali_driver_mode drivermode; /**< Reserved. Must always be _MALI_DRIVER_MODE_NORMAL */
} _mali_system_info;

/** @brief Arguments to _mali_ukk_get_system_info()
 *
 * A buffer of the size returned by _mali_ukk_get_system_info_size() must be
 * allocated, and the pointer to this buffer must be written into the
 * system_info member. The buffer must be suitably aligned for storage of
 * the _mali_system_info structure - for example, one returned by
 * _mali_osk_malloc(), which will be suitably aligned for any structure.
 *
 * The ukk_private member must be set to zero by the user-side. Under an OS
 * implementation, the U/K interface must write in the user-side base address
 * into the ukk_private member, so that the common code in
 * _mali_ukk_get_system_info() can determine how to adjust the pointers such
 * that they are sensible from user space. Leaving ukk_private as NULL implies
 * that no pointer adjustment is necessary - which will be the case on a
 * bare-metal/RTOS system.
 *
 * @see _mali_system_info
 */
typedef struct
{
    void *ctx;                              /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                               /**< [in] size of buffer provided to store system information data */
	_mali_system_info * system_info;        /**< [in,out] pointer to buffer to store system information data. No initialisation of buffer required on input. */
	u32 ukk_private;                        /**< [in] Kernel-side private word inserted by certain U/K interface implementations. Caller must set to Zero. */
} _mali_uk_get_system_info_s;
/** @} */ /* end group _mali_uk_getsysteminfo */

/** @} */ /* end group _mali_uk_core */


/** @defgroup _mali_uk_gp U/K Vertex Processor
 * @{ */

/** @defgroup _mali_uk_gp_suspend_response_s Vertex Processor Suspend Response
 * @{ */

/** @brief Arguments for _mali_ukk_gp_suspend_response()
 *
 * When _mali_wait_for_notification() receives notification that a
 * Vertex Processor job was suspended, you need to send a response to indicate
 * what needs to happen with this job. You can either abort or resume the job.
 *
 * - set @c code to indicate response code. This is either @c _MALIGP_JOB_ABORT or
 * @c _MALIGP_JOB_RESUME_WITH_NEW_HEAP to indicate you will provide a new heap
 * for the job that will resolve the out of memory condition for the job.
 * - copy the @c cookie value from the @c _mali_uk_gp_job_suspended_s notification;
 * this is an identifier for the suspended job
 * - set @c arguments[0] and @c arguments[1] to zero if you abort the job. If
 * you resume it, @c argument[0] should specify the Mali start address for the new
 * heap and @c argument[1] the Mali end address of the heap.
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 *
 */
typedef enum _maligp_job_suspended_response_code
{
	_MALIGP_JOB_ABORT,                  /**< Abort the Vertex Processor job */
	_MALIGP_JOB_RESUME_WITH_NEW_HEAP    /**< Resume the Vertex Processor job with a new heap */
} _maligp_job_suspended_response_code;

typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 cookie;                     /**< [in] cookie from the _mali_uk_gp_job_suspended_s notification */
	_maligp_job_suspended_response_code code; /**< [in] abort or resume response code, see \ref _maligp_job_suspended_response_code */
	u32 arguments[2];               /**< [in] 0 when aborting a job. When resuming a job, the Mali start and end address for a new heap to resume the job with */
} _mali_uk_gp_suspend_response_s;

/** @} */ /* end group _mali_uk_gp_suspend_response_s */

/** @defgroup _mali_uk_gpstartjob_s Vertex Processor Start Job
 * @{ */

/** @brief Status indicating the result of starting a Vertex or Fragment processor job */
typedef enum
{
    _MALI_UK_START_JOB_STARTED,                         /**< Job started */
    _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED,    /**< Job started and bumped a lower priority job that was pending execution */
    _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE           /**< Job could not be started at this time. Try starting the job again */
} _mali_uk_start_job_status;

/** @brief Status indicating the result of starting a Vertex or Fragment processor job */
typedef enum
{
	MALI_UK_START_JOB_FLAG_DEFAULT = 0,          /**< Default behaviour; Flush L2 caches before start, no following jobs */
	MALI_UK_START_JOB_FLAG_NO_FLUSH = 1,         /**< No need to flush L2 caches before start */
	MALI_UK_START_JOB_FLAG_MORE_JOBS_FOLLOW = 2, /**< More related jobs follows, try to schedule them as soon as possible after this job */
} _mali_uk_start_job_flags;

/** @brief Status indicating the result of the execution of a Vertex or Fragment processor job  */

typedef enum
{
	_MALI_UK_JOB_STATUS_END_SUCCESS         = 1<<(16+0),
	_MALI_UK_JOB_STATUS_END_OOM             = 1<<(16+1),
	_MALI_UK_JOB_STATUS_END_ABORT           = 1<<(16+2),
	_MALI_UK_JOB_STATUS_END_TIMEOUT_SW      = 1<<(16+3),
	_MALI_UK_JOB_STATUS_END_HANG            = 1<<(16+4),
	_MALI_UK_JOB_STATUS_END_SEG_FAULT       = 1<<(16+5),
	_MALI_UK_JOB_STATUS_END_ILLEGAL_JOB     = 1<<(16+6),
	_MALI_UK_JOB_STATUS_END_UNKNOWN_ERR     = 1<<(16+7),
	_MALI_UK_JOB_STATUS_END_SHUTDOWN        = 1<<(16+8),
	_MALI_UK_JOB_STATUS_END_SYSTEM_UNUSABLE = 1<<(16+9)
} _mali_uk_job_status;

#define MALIGP2_NUM_REGS_FRAME (6)

/** @brief Arguments for _mali_ukk_gp_start_job()
 *
 * To start a Vertex Processor job
 * - associate the request with a reference to a @c mali_gp_job_info by setting
 * user_job_ptr to the address of the @c mali_gp_job_info of the job.
 * - set @c priority to the priority of the @c mali_gp_job_info
 * - specify a timeout for the job by setting @c watchdog_msecs to the number of
 * milliseconds the job is allowed to run. Specifying a value of 0 selects the
 * default timeout in use by the device driver.
 * - copy the frame registers from the @c mali_gp_job_info into @c frame_registers.
 * - set the @c perf_counter_flag, @c perf_counter_src0 and @c perf_counter_src1 to zero
 * for a non-instrumented build. For an instrumented build you can use up
 * to two performance counters. Set the corresponding bit in @c perf_counter_flag
 * to enable them. @c perf_counter_src0 and @c perf_counter_src1 specify
 * the source of what needs to get counted (e.g. number of vertex loader
 * cache hits). For source id values, see ARM DDI0415A, Table 3-60.
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 *
 * When @c _mali_ukk_gp_start_job() returns @c _MALI_OSK_ERR_OK, status contains the
 * result of the request (see \ref _mali_uk_start_job_status). If the job could
 * not get started (@c _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE) it should be
 * tried again. If the job had a higher priority than the one currently pending
 * execution (@c _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED), it will bump
 * the lower priority job and returns the address of the @c mali_gp_job_info
 * for that job in @c returned_user_job_ptr. That job should get requeued.
 *
 * After the job has started, @c _mali_wait_for_notification() will be notified
 * that the job finished or got suspended. It may get suspended due to
 * resource shortage. If it finished (see _mali_ukk_wait_for_notification())
 * the notification will contain a @c _mali_uk_gp_job_finished_s result. If
 * it got suspended the notification will contain a @c _mali_uk_gp_job_suspended_s
 * result.
 *
 * The @c _mali_uk_gp_job_finished_s contains the job status (see \ref _mali_uk_job_status),
 * the number of milliseconds the job took to render, and values of core registers
 * when the job finished (irq status, performance counters, renderer list
 * address). A job has finished succesfully when its status is
 * @c _MALI_UK_JOB_STATUS_FINISHED. If the hardware detected a timeout while rendering
 * the job, or software detected the job is taking more than watchdog_msecs to
 * complete, the status will indicate @c _MALI_UK_JOB_STATUS_HANG.
 * If the hardware detected a bus error while accessing memory associated with the
 * job, status will indicate @c _MALI_UK_JOB_STATUS_SEG_FAULT.
 * status will indicate @c _MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to
 * stop the job but the job didn't start on the hardware yet, e.g. when the
 * driver shutdown.
 *
 * In case the job got suspended, @c _mali_uk_gp_job_suspended_s contains
 * the @c user_job_ptr identifier used to start the job with, the @c reason
 * why the job stalled (see \ref _maligp_job_suspended_reason) and a @c cookie
 * to identify the core on which the job stalled.  This @c cookie will be needed
 * when responding to this nofication by means of _mali_ukk_gp_suspend_response().
 * (see _mali_ukk_gp_suspend_response()). The response is either to abort or
 * resume the job. If the job got suspended due to an out of memory condition
 * you may be able to resolve this by providing more memory and resuming the job.
 *
 */
typedef struct
{
    void *ctx;                          /**< [in,out] user-kernel context (trashed on output) */
    u32 user_job_ptr;                   /**< [in] identifier for the job in user space, a @c mali_gp_job_info* */
    u32 priority;                       /**< [in] job priority. A lower number means higher priority */
    u32 watchdog_msecs;                 /**< [in] maximum allowed runtime in milliseconds. The job gets killed if it runs longer than this. A value of 0 selects the default used by the device driver. */
    u32 frame_registers[MALIGP2_NUM_REGS_FRAME]; /**< [in] core specific registers associated with this job */
    u32 perf_counter_flag;              /**< [in] bitmask indicating which performance counters to enable, see \ref _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE and related macro definitions */
    u32 perf_counter_src0;              /**< [in] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;              /**< [in] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 returned_user_job_ptr;          /**< [out] identifier for the returned job in user space, a @c mali_gp_job_info* */
    _mali_uk_start_job_status status;   /**< [out] indicates job start status (success, previous job returned, requeue) */
	u32 abort_id;                       /**< [in] abort id of this job, used to identify this job for later abort requests */
	u32 perf_counter_l2_src0;           /**< [in] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;           /**< [in] source id for Mali-400 MP L2 cache performance counter 1 */
	u32 frame_builder_id;				/**< [in] id of the originating frame builder */
	u32 flush_id;						/**< [in] flush id within the originating frame builder */
} _mali_uk_gp_start_job_s;

#define _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE (1<<0) /**< Enable performance counter SRC0 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE (1<<1) /**< Enable performance counter SRC1 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_SRC0_ENABLE (1<<2) /**< Enable performance counter L2_SRC0 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_SRC1_ENABLE (1<<3) /**< Enable performance counter L2_SRC1 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_RESET       (1<<4) /**< Enable performance counter L2_RESET for a job */

/** @} */ /* end group _mali_uk_gpstartjob_s */

typedef struct
{
    u32 user_job_ptr;               /**< [out] identifier for the job in user space */
    _mali_uk_job_status status;     /**< [out] status of finished job */
    u32 irq_status;                 /**< [out] value of the GP interrupt rawstat register (see ARM DDI0415A) */
    u32 status_reg_on_stop;         /**< [out] value of the GP control register */
    u32 vscl_stop_addr;             /**< [out] value of the GP VLSCL start register */
    u32 plbcl_stop_addr;            /**< [out] value of the GP PLBCL start register */
    u32 heap_current_addr;          /**< [out] value of the GP PLB PL heap start address register */
    u32 perf_counter_src0;          /**< [out] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;          /**< [out] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter0;              /**< [out] value of perfomance counter 0 (see ARM DDI0415A) */
    u32 perf_counter1;              /**< [out] value of perfomance counter 1 (see ARM DDI0415A) */
    u32 render_time;                /**< [out] number of microseconds it took for the job to render */
	u32 perf_counter_l2_src0;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_gp_job_finished_s;

typedef enum _maligp_job_suspended_reason
{
	_MALIGP_JOB_SUSPENDED_OUT_OF_MEMORY  /**< Polygon list builder unit (PLBU) has run out of memory */
} _maligp_job_suspended_reason;

typedef struct
{
	u32 user_job_ptr;                    /**< [out] identifier for the job in user space */
	_maligp_job_suspended_reason reason; /**< [out] reason why the job stalled */
	u32 cookie;                          /**< [out] identifier for the core in kernel space on which the job stalled */
} _mali_uk_gp_job_suspended_s;

/** @} */ /* end group _mali_uk_gp */


/** @defgroup _mali_uk_pp U/K Fragment Processor
 * @{ */

/** @defgroup _mali_uk_ppstartjob_s Fragment Processor Start Job
 * @{ */

/** @brief Arguments for _mali_ukk_pp_start_job()
 *
 * To start a Fragment Processor job
 * - associate the request with a reference to a mali_pp_job by setting
 * @c user_job_ptr to the address of the @c mali_pp_job of the job.
 * - set @c priority to the priority of the mali_pp_job
 * - specify a timeout for the job by setting @c watchdog_msecs to the number of
 * milliseconds the job is allowed to run. Specifying a value of 0 selects the
 * default timeout in use by the device driver.
 * - copy the frame registers from the @c mali_pp_job into @c frame_registers.
 * For MALI200 you also need to copy the write back 0,1 and 2 registers.
 * - set the @c perf_counter_flag, @c perf_counter_src0 and @c perf_counter_src1 to zero
 * for a non-instrumented build. For an instrumented build you can use up
 * to two performance counters. Set the corresponding bit in @c perf_counter_flag
 * to enable them. @c perf_counter_src0 and @c perf_counter_src1 specify
 * the source of what needs to get counted (e.g. number of vertex loader
 * cache hits). For source id values, see ARM DDI0415A, Table 3-60.
 * - pass in the user-kernel context in @c ctx that was returned from _mali_ukk_open()
 *
 * When _mali_ukk_pp_start_job() returns @c _MALI_OSK_ERR_OK, @c status contains the
 * result of the request (see \ref _mali_uk_start_job_status). If the job could
 * not get started (@c _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE) it should be
 * tried again. If the job had a higher priority than the one currently pending
 * execution (@c _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED), it will bump
 * the lower priority job and returns the address of the @c mali_pp_job
 * for that job in @c returned_user_job_ptr. That job should get requeued.
 *
 * After the job has started, _mali_wait_for_notification() will be notified
 * when the job finished. The notification will contain a
 * @c _mali_uk_pp_job_finished_s result. It contains the @c user_job_ptr
 * identifier used to start the job with, the job @c status (see \ref _mali_uk_job_status),
 * the number of milliseconds the job took to render, and values of core registers
 * when the job finished (irq status, performance counters, renderer list
 * address). A job has finished succesfully when its status is
 * @c _MALI_UK_JOB_STATUS_FINISHED. If the hardware detected a timeout while rendering
 * the job, or software detected the job is taking more than @c watchdog_msecs to
 * complete, the status will indicate @c _MALI_UK_JOB_STATUS_HANG.
 * If the hardware detected a bus error while accessing memory associated with the
 * job, status will indicate @c _MALI_UK_JOB_STATUS_SEG_FAULT.
 * status will indicate @c _MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to
 * stop the job but the job didn't start on the hardware yet, e.g. when the
 * driver shutdown.
 *
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 user_job_ptr;               /**< [in] identifier for the job in user space */
    u32 priority;                   /**< [in] job priority. A lower number means higher priority */
    u32 watchdog_msecs;             /**< [in] maximum allowed runtime in milliseconds. The job gets killed if it runs longer than this. A value of 0 selects the default used by the device driver. */
    u32 frame_registers[MALI200_NUM_REGS_FRAME]; /**< [in] core specific registers associated with this job, see ARM DDI0415A */
    u32 wb0_registers[MALI200_NUM_REGS_WBx];
    u32 wb1_registers[MALI200_NUM_REGS_WBx];
    u32 wb2_registers[MALI200_NUM_REGS_WBx];
    u32 perf_counter_flag;              /**< [in] bitmask indicating which performance counters to enable, see \ref _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE and related macro definitions */
    u32 perf_counter_src0;              /**< [in] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;              /**< [in] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 returned_user_job_ptr;          /**< [out] identifier for the returned job in user space */
    _mali_uk_start_job_status status;   /**< [out] indicates job start status (success, previous job returned, requeue) */
	u32 abort_id;                       /**< [in] abort id of this job, used to identify this job for later abort requests */
	u32 perf_counter_l2_src0;           /**< [in] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;           /**< [in] source id for Mali-400 MP L2 cache performance counter 1 */
	u32 frame_builder_id;				/**< [in] id of the originating frame builder */
	u32 flush_id;						/**< [in] flush id within the originating frame builder */
	_mali_uk_start_job_flags flags;     /**< [in] Flags for job, see _mali_uk_start_job_flags for more information */
} _mali_uk_pp_start_job_s;
/** @} */ /* end group _mali_uk_ppstartjob_s */

typedef struct
{
    u32 user_job_ptr;               /**< [out] identifier for the job in user space */
    _mali_uk_job_status status;     /**< [out] status of finished job */
    u32 irq_status;                 /**< [out] value of interrupt rawstat register (see ARM DDI0415A) */
    u32 last_tile_list_addr;        /**< [out] value of renderer list register (see ARM DDI0415A); necessary to restart a stopped job */
    u32 perf_counter_src0;          /**< [out] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;          /**< [out] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter0;              /**< [out] value of perfomance counter 0 (see ARM DDI0415A) */
    u32 perf_counter1;              /**< [out] value of perfomance counter 1 (see ARM DDI0415A) */
    u32 render_time;                /**< [out] number of microseconds it took for the job to render */
	u32 perf_counter_l2_src0;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0_raw;   /**< [out] Raw value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1_raw;   /**< [out] Raw value of the Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_pp_job_finished_s;
/** @} */ /* end group _mali_uk_pp */


/** @addtogroup _mali_uk_core U/K Core
 * @{ */

/** @defgroup _mali_uk_waitfornotification_s Wait For Notification
 * @{ */

/** @brief Notification type encodings
 *
 * Each Notification type is an ordered pair of (subsystem,id), and is unique.
 *
 * The encoding of subsystem,id into a 32-bit word is:
 * encoding = (( subsystem << _MALI_NOTIFICATION_SUBSYSTEM_SHIFT ) & _MALI_NOTIFICATION_SUBSYSTEM_MASK)
 *            | (( id <<  _MALI_NOTIFICATION_ID_SHIFT ) & _MALI_NOTIFICATION_ID_MASK)
 *
 * @see _mali_uk_wait_for_notification_s
 */
typedef enum
{
	/** core notifications */

	_MALI_NOTIFICATION_CORE_SHUTDOWN_IN_PROGRESS =  (_MALI_UK_CORE_SUBSYSTEM << 16) | 0x20,
	_MALI_NOTIFICATION_APPLICATION_QUIT =           (_MALI_UK_CORE_SUBSYSTEM << 16) | 0x40,

	/** Fragment Processor notifications */

	_MALI_NOTIFICATION_PP_FINISHED =                (_MALI_UK_PP_SUBSYSTEM << 16) | 0x10,

	/** Vertex Processor notifications */

	_MALI_NOTIFICATION_GP_FINISHED =                (_MALI_UK_GP_SUBSYSTEM << 16) | 0x10,
	_MALI_NOTIFICATION_GP_STALLED =                 (_MALI_UK_GP_SUBSYSTEM << 16) | 0x20,
} _mali_uk_notification_type;

/** to assist in splitting up 32-bit notification value in subsystem and id value */
#define _MALI_NOTIFICATION_SUBSYSTEM_MASK 0xFFFF0000
#define _MALI_NOTIFICATION_SUBSYSTEM_SHIFT 16
#define _MALI_NOTIFICATION_ID_MASK 0x0000FFFF
#define _MALI_NOTIFICATION_ID_SHIFT 0


/** @brief Arguments for _mali_ukk_wait_for_notification()
 *
 * On successful return from _mali_ukk_wait_for_notification(), the members of
 * this structure will indicate the reason for notification.
 *
 * Specifically, the source of the notification can be identified by the
 * subsystem and id fields of the mali_uk_notification_type in the code.type
 * member. The type member is encoded in a way to divide up the types into a
 * subsystem field, and a per-subsystem ID field. See
 * _mali_uk_notification_type for more information.
 *
 * Interpreting the data union member depends on the notification type:
 *
 * - type == _MALI_NOTIFICATION_CORE_SHUTDOWN_IN_PROGRESS
 *     - The kernel side is shutting down. No further
 * _mali_uk_wait_for_notification() calls should be made.
 *     - In this case, the value of the data union member is undefined.
 *     - This is used to indicate to the user space client that it should close
 * the connection to the Mali Device Driver.
 * - type == _MALI_NOTIFICATION_PP_FINISHED
 *    - The notification data is of type _mali_uk_pp_job_finished_s. It contains the user_job_ptr
 * identifier used to start the job with, the job status, the number of milliseconds the job took to render,
 * and values of core registers when the job finished (irq status, performance counters, renderer list
 * address).
 *    - A job has finished succesfully when its status member is _MALI_UK_JOB_STATUS_FINISHED.
 *    - If the hardware detected a timeout while rendering the job, or software detected the job is
 * taking more than watchdog_msecs (see _mali_ukk_pp_start_job()) to complete, the status member will
 * indicate _MALI_UK_JOB_STATUS_HANG.
 *    - If the hardware detected a bus error while accessing memory associated with the job, status will
 * indicate _MALI_UK_JOB_STATUS_SEG_FAULT.
 *    - Status will indicate MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to stop the job but the job
 * didn't start the hardware yet, e.g. when the driver closes.
 * - type == _MALI_NOTIFICATION_GP_FINISHED
 *     - The notification data is of type _mali_uk_gp_job_finished_s. The notification is similar to that of
 * type == _MALI_NOTIFICATION_PP_FINISHED, except that several other GP core register values are returned.
 * The status values have the same meaning for type == _MALI_NOTIFICATION_PP_FINISHED.
 * - type == _MALI_NOTIFICATION_GP_STALLED
 *     - The nofication data is of type _mali_uk_gp_job_suspended_s. It contains the user_job_ptr
 * identifier used to start the job with, the reason why the job stalled and a cookie to identify the core on
 * which the job stalled.
 *     - The reason member of gp_job_suspended is set to _MALIGP_JOB_SUSPENDED_OUT_OF_MEMORY
 * when the polygon list builder unit has run out of memory.
 */
typedef struct
{
    void *ctx;                       /**< [in,out] user-kernel context (trashed on output) */
	_mali_uk_notification_type type; /**< [out] Type of notification available */
    union
    {
        _mali_uk_gp_job_suspended_s gp_job_suspended;/**< [out] Notification data for _MALI_NOTIFICATION_GP_STALLED notification type */
        _mali_uk_gp_job_finished_s  gp_job_finished; /**< [out] Notification data for _MALI_NOTIFICATION_GP_FINISHED notification type */
        _mali_uk_pp_job_finished_s  pp_job_finished; /**< [out] Notification data for _MALI_NOTIFICATION_PP_FINISHED notification type */
    } data;
} _mali_uk_wait_for_notification_s;

/** @brief Arguments for _mali_ukk_post_notification()
 *
 * Posts the specified notification to the notification queue for this application.
 * This is used to send a quit message to the callback thread.
 */
typedef struct
{
    void *ctx;                       /**< [in,out] user-kernel context (trashed on output) */
	_mali_uk_notification_type type; /**< [in] Type of notification to post */
} _mali_uk_post_notification_s;
/** @} */ /* end group _mali_uk_waitfornotification_s */

/** @defgroup _mali_uk_getapiversion_s Get API Version
 * @{ */

/** helpers for Device Driver API version handling */

/** @brief Encode a version ID from a 16-bit input
 *
 * @note the input is assumed to be 16 bits. It must not exceed 16 bits. */
#define _MAKE_VERSION_ID(x) (((x) << 16UL) | (x))

/** @brief Check whether a 32-bit value is likely to be Device Driver API
 * version ID. */
#define _IS_VERSION_ID(x) (((x) & 0xFFFF) == (((x) >> 16UL) & 0xFFFF))

/** @brief Decode a 16-bit version number from a 32-bit Device Driver API version
 * ID */
#define _GET_VERSION(x) (((x) >> 16UL) & 0xFFFF)

/** @brief Determine whether two 32-bit encoded version IDs match */
#define _IS_API_MATCH(x, y) (IS_VERSION_ID((x)) && IS_VERSION_ID((y)) && (GET_VERSION((x)) == GET_VERSION((y))))

/**
 * API version define.
 * Indicates the version of the kernel API
 * The version is a 16bit integer incremented on each API change.
 * The 16bit integer is stored twice in a 32bit integer
 * For example, for version 1 the value would be 0x00010001
 */
#define _MALI_API_VERSION 10
#define _MALI_UK_API_VERSION _MAKE_VERSION_ID(_MALI_API_VERSION)

/**
 * The API version is a 16-bit integer stored in both the lower and upper 16-bits
 * of a 32-bit value. The 16-bit API version value is incremented on each API
 * change. Version 1 would be 0x00010001. Used in _mali_uk_get_api_version_s.
 */
typedef u32 _mali_uk_api_version;

/** @brief Arguments for _mali_uk_get_api_version()
 *
 * The user-side interface version must be written into the version member,
 * encoded using _MAKE_VERSION_ID(). It will be compared to the API version of
 * the kernel-side interface.
 *
 * On successful return, the version member will be the API version of the
 * kernel-side interface. _MALI_UK_API_VERSION macro defines the current version
 * of the API.
 *
 * The compatible member must be checked to see if the version of the user-side
 * interface is compatible with the kernel-side interface, since future versions
 * of the interface may be backwards compatible.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	_mali_uk_api_version version;   /**< [in,out] API version of user-side interface. */
	int compatible;                 /**< [out] @c 1 when @version is compatible, @c 0 otherwise */
} _mali_uk_get_api_version_s;
/** @} */ /* end group _mali_uk_getapiversion_s */

/** @} */ /* end group _mali_uk_core */


/** @defgroup _mali_uk_memory U/K Memory
 * @{ */

/** @brief Arguments for _mali_ukk_init_mem(). */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 mali_address_base;          /**< [out] start of MALI address space */
	u32 memory_size;                /**< [out] total MALI address space available */
} _mali_uk_init_mem_s;

/** @brief Arguments for _mali_ukk_term_mem(). */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
} _mali_uk_term_mem_s;

/** @brief Arguments for _mali_ukk_get_big_block()
 *
 * - type_id should be set to the value of the identifier member of one of the
 * _mali_mem_info structures returned through _mali_ukk_get_system_info()
 * - ukk_private must be zero when calling from user-side. On Kernel-side, the
 * OS implementation of the U/K interface can use it to communicate data to the
 * OS implementation of the OSK layer. Specifically, ukk_private will be placed
 * into the ukk_private member of the _mali_uk_mem_mmap_s structure. See
 * _mali_ukk_mem_mmap() for more details.
 * - minimum_size_requested will be updated if it is too small
 * - block_size will always be >= minimum_size_requested, because the underlying
 * allocation mechanism may only be able to divide up memory regions in certain
 * ways. To avoid wasting memory, block_size should always be taken into account
 * rather than assuming minimum_size_requested was really allocated.
 * - to free the memory, the returned cookie member must be stored, and used to
 * refer to it.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 type_id;                    /**< [in] the type id of the memory bank to allocate memory from */
	u32 minimum_size_requested;     /**< [in,out] minimum size of the allocation */
	u32 ukk_private;                /**< [in] Kernel-side private word inserted by certain U/K interface implementations. Caller must set to Zero. */
	u32 mali_address;               /**< [out] address of the allocation in mali address space */
	void *cpuptr;                   /**< [out] address of the allocation in the current process address space */
	u32 block_size;                 /**< [out] size of the block that got allocated */
	u32 flags;                      /**< [out] flags associated with the allocated block, of type _mali_bus_usage */
	u32 cookie;                     /**< [out] identifier for the allocated block in kernel space  */
} _mali_uk_get_big_block_s;

/** @brief Arguments for _mali_ukk_free_big_block()
 *
 * All that is required is that the cookie member must be set to the value of
 * the cookie member returned through _mali_ukk_get_big_block()
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 cookie;                     /**< [in] identifier for mapped memory object in kernel space  */
} _mali_uk_free_big_block_s;

/** @note Mali-MMU only */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 phys_addr;                  /**< [in] physical address */
	u32 size;                       /**< [in] size */
	u32 mali_address;               /**< [in] mali address to map the physical memory to */
	u32 rights;                     /**< [in] rights necessary for accessing memory */
	u32 flags;                      /**< [in] flags, see \ref _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE */
	u32 cookie;                     /**< [out] identifier for mapped memory object in kernel space  */
} _mali_uk_map_external_mem_s;

/** Flag for _mali_uk_map_external_mem_s and _mali_uk_attach_ump_mem_s */
#define _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE (1<<0)

/** @note Mali-MMU only */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 cookie;                     /**< [out] identifier for mapped memory object in kernel space  */
} _mali_uk_unmap_external_mem_s;

/** @note This is identical to _mali_uk_map_external_mem_s above, however phys_addr is replaced by secure_id */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 secure_id;                  /**< [in] secure id */
	u32 size;                       /**< [in] size */
	u32 mali_address;               /**< [in] mali address to map the physical memory to */
	u32 rights;                     /**< [in] rights necessary for accessing memory */
	u32 flags;                      /**< [in] flags, see \ref _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE */
	u32 cookie;                     /**< [out] identifier for mapped memory object in kernel space  */
} _mali_uk_attach_ump_mem_s;

/** @note Mali-MMU only; will be supported in future version */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 cookie;                     /**< [in] identifier for mapped memory object in kernel space  */
} _mali_uk_release_ump_mem_s;

/** @brief Arguments for _mali_ukk_va_to_mali_pa()
 *
 * if size is zero or not a multiple of the system's page size, it will be
 * rounded up to the next multiple of the page size. This will occur before
 * any other use of the size parameter.
 *
 * if va is not PAGE_SIZE aligned, it will be rounded down to the next page
 * boundary.
 *
 * The range (va) to ((u32)va)+(size-1) inclusive will be checked for physical
 * contiguity.
 *
 * The implementor will check that the entire physical range is allowed to be mapped
 * into user-space.
 *
 * Failure will occur if either of the above are not satisfied.
 *
 * Otherwise, the physical base address of the range is returned through pa,
 * va is updated to be page aligned, and size is updated to be a non-zero
 * multiple of the system's pagesize.
 */
typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	void *va;                       /**< [in,out] Virtual address of the start of the range */
	u32 pa;                         /**< [out] Physical base address of the range */
	u32 size;                       /**< [in,out] Size of the range, in bytes. */
} _mali_uk_va_to_mali_pa_s;


typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                       /**< [out] size of MMU page table information (registers + page tables) */
} _mali_uk_query_mmu_page_table_dump_size_s;

typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                       /**< [in] size of buffer to receive mmu page table information */
    void *buffer;                   /**< [in,out] buffer to receive mmu page table information */
    u32 register_writes_size;       /**< [out] size of MMU register dump */
	u32 *register_writes;           /**< [out] pointer within buffer where MMU register dump is stored */
	u32 page_table_dump_size;       /**< [out] size of MMU page table dump */
	u32 *page_table_dump;           /**< [out] pointer within buffer where MMU page table dump is stored */
} _mali_uk_dump_mmu_page_table_s;

/** @} */ /* end group _mali_uk_memory */


/** @addtogroup _mali_uk_pp U/K Fragment Processor
 * @{ */

/** @brief Arguments for _mali_ukk_get_pp_number_of_cores()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_pp_number_of_cores(), @c number_of_cores
 * will contain the number of Fragment Processor cores in the system.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 number_of_cores;            /**< [out] number of Fragment Processor cores in the system */
} _mali_uk_get_pp_number_of_cores_s;

/** @brief Arguments for _mali_ukk_get_pp_core_version()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_pp_core_version(), @c version contains
 * the version that all Fragment Processor cores are compatible with.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    _mali_core_version version;     /**< [out] version returned from core, see \ref _mali_core_version  */
} _mali_uk_get_pp_core_version_s;

typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 abort_id;                   /**< [in] ID of job(s) to abort */
} _mali_uk_pp_abort_job_s;

/** @} */ /* end group _mali_uk_pp */


/** @addtogroup _mali_uk_gp U/K Vertex Processor
 * @{ */

/** @brief Arguments for _mali_ukk_get_gp_number_of_cores()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_gp_number_of_cores(), @c number_of_cores
 * will contain the number of Vertex Processor cores in the system.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 number_of_cores;            /**< [out] number of Vertex Processor cores in the system */
} _mali_uk_get_gp_number_of_cores_s;

/** @brief Arguments for _mali_ukk_get_gp_core_version()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_gp_core_version(), @c version contains
 * the version that all Vertex Processor cores are compatible with.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    _mali_core_version version;     /**< [out] version returned from core, see \ref _mali_core_version */
} _mali_uk_get_gp_core_version_s;

typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 abort_id;                   /**< [in] ID of job(s) to abort */
} _mali_uk_gp_abort_job_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 limit;                      /**< [in,out] The desired limit for number of events to record on input, actual limit on output */
} _mali_uk_profiling_start_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 event_id;                   /**< [in] event id to register (see  enum mali_profiling_events for values) */
	u32 data[5];                    /**< [in] event specific data */
} _mali_uk_profiling_add_event_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 count;                      /**< [out] The number of events sampled */
} _mali_uk_profiling_stop_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 index;                      /**< [in] which index to get (starting at zero) */
	u64 timestamp;                  /**< [out] timestamp of event */
	u32 event_id;                   /**< [out] event id of event (see  enum mali_profiling_events for values) */
	u32 data[5];                    /**< [out] event specific data */
} _mali_uk_profiling_get_event_s;

typedef struct
{
	void *ctx;

	u32 id;
	s64 value;
} _mali_uk_sw_counters_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
} _mali_uk_profiling_clear_s;

typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 enable_events;              /**< [out] 1 if user space process should generate events, 0 if not */
} _mali_uk_profiling_get_config_s;


/** @} */ /* end group _mali_uk_gp */


/** @addtogroup _mali_uk_memory U/K Memory
 * @{ */

/** @brief Arguments to _mali_ukk_mem_mmap()
 *
 * Use of the phys_addr member depends on whether the driver is compiled for
 * Mali-MMU or nonMMU:
 * - in the nonMMU case, this is the physical address of the memory as seen by
 * the CPU (which may be a constant offset from that used by Mali)
 * - in the MMU case, this is the Mali Virtual base address of the memory to
 * allocate, and the particular physical pages used to back the memory are
 * entirely determined by _mali_ukk_mem_mmap(). The details of the physical pages
 * are not reported to user-space for security reasons.
 *
 * The cookie member must be stored for use later when freeing the memory by
 * calling _mali_ukk_mem_munmap(). In the Mali-MMU case, the cookie is secure.
 *
 * The ukk_private word must be set to zero when calling from user-space. On
 * Kernel-side, the  OS implementation of the U/K interface can use it to
 * communicate data to the OS implementation of the OSK layer. In particular,
 * _mali_ukk_get_big_block() directly calls _mali_ukk_mem_mmap directly, and
 * will communicate its own ukk_private word through the ukk_private member
 * here. The common code itself will not inspect or modify the ukk_private
 * word, and so it may be safely used for whatever purposes necessary to
 * integrate Mali Memory handling into the OS.
 *
 * The uku_private member is currently reserved for use by the user-side
 * implementation of the U/K interface. Its value must be zero.
 */
typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	void *mapping;                  /**< [out] Returns user-space virtual address for the mapping */
	u32 size;                       /**< [in] Size of the requested mapping */
	u32 phys_addr;                  /**< [in] Physical address - could be offset, depending on caller+callee convention */
	u32 cookie;                     /**< [out] Returns a cookie for use in munmap calls */
	void *uku_private;              /**< [in] User-side Private word used by U/K interface */
	void *ukk_private;              /**< [in] Kernel-side Private word used by U/K interface */
} _mali_uk_mem_mmap_s;

/** @brief Arguments to _mali_ukk_mem_munmap()
 *
 * The cookie and mapping members must be that returned from the same previous
 * call to _mali_ukk_mem_mmap(). The size member must correspond to cookie
 * and mapping - that is, it must be the value originally supplied to a call to
 * _mali_ukk_mem_mmap that returned the values of mapping and cookie.
 *
 * An error will be returned if an attempt is made to unmap only part of the
 * originally obtained range, or to unmap more than was originally obtained.
 */
typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	void *mapping;                  /**< [in] The mapping returned from mmap call */
	u32 size;                       /**< [in] The size passed to mmap call */
	u32 cookie;                     /**< [in] Cookie from mmap call */
} _mali_uk_mem_munmap_s;
/** @} */ /* end group _mali_uk_memory */

#if USING_MALI_PMM

/** @defgroup _mali_uk_pmm U/K Power Management Module
 * @{ */

/** @brief Power management event message identifiers.
 *
 * U/K events start after id 200, and can range up to 999
 * Adding new events will require updates to the PMM mali_pmm_event_id type
 */
#define _MALI_PMM_EVENT_UK_EXAMPLE 201

/** @brief Generic PMM message data type, that will be dependent on the event msg
 */
typedef u32 mali_pmm_message_data;


/** @brief Arguments to _mali_ukk_pmm_event_message()
 */
typedef struct
{
	void *ctx;                          /**< [in,out] user-kernel context (trashed on output) */
	u32 id;                             /**< [in] event id */
	mali_pmm_message_data data;         /**< [in] specific data associated with the event */
} _mali_uk_pmm_message_s;

/** @} */ /* end group _mali_uk_pmm */
#endif /* USING_MALI_PMM */

/** @defgroup _mali_uk_vsync U/K VSYNC Wait Reporting Module
 * @{ */

/** @brief VSYNC events
 *
 * These events are reported when DDK starts to wait for vsync and when the
 * vsync has occured and the DDK can continue on the next frame.
 */
typedef enum _mali_uk_vsync_event
{
	_MALI_UK_VSYNC_EVENT_BEGIN_WAIT = 0,
	_MALI_UK_VSYNC_EVENT_END_WAIT
} _mali_uk_vsync_event;

/** @brief Arguments to _mali_ukk_vsync_event()
 *
 */
typedef struct
{
	void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	_mali_uk_vsync_event event;     /**< [in] VSYNCH event type */
} _mali_uk_vsync_event_report_s;

/** @} */ /* end group _mali_uk_vsync */

/** @} */ /* end group u_k_api */

/** @} */ /* end group uddapi */

#ifdef __cplusplus
}
#endif

#endif /* __MALI_UK_TYPES_H__ */
