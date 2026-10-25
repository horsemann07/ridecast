
/* ============================================================================
 * File        : cmsis_os2.c
 * Description : CMSIS-RTOS2 adaptation for ESP-IDF 5.5 FreeRTOS (ESP32)
 * Author      : Jha Raghav
 * Date        : 2026-10-23
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * This code is adapted from the user-supplied CMSIS-RTOS2 FreeRTOS wrapper.
 * WARNING: Not a certified MISRA C:2012 implementation. Review deviations,
 * hardware-test and run a qualified static analyzer before production release.
 * WARNING: Do not invoke CMSIS blocking APIs while holding a FreeRTOS spinlock.
 * WARNING: Only use CMSIS task notifications on their reserved notification slot.
 *
 * Change Log (most recent first):
 * 2026-10-23  Jha Raghav  ESP-IDF 5.5 compatibility and safety corrections
 * 2026-10-23  Jha Raghav  Original application port baseline
 * ============================================================================ */

/* -------------------------------------------------------------------------- */
/* Standard Library Headers                                                   */
/* -------------------------------------------------------------------------- */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdatomic.h> /* C11 atomics: documented MISRA language deviation */

/* -------------------------------------------------------------------------- */
/* CMSIS and ESP-IDF Headers                                                  */
/* -------------------------------------------------------------------------- */

#include "sdkconfig.h"

#include "cmsis_os2.h"
#include "cmsis_esp_storage.h"

#include "esp_idf_version.h"
#include "esp_timer.h"

/* -------------------------------------------------------------------------- */
/* FreeRTOS Headers                                                           */
/* -------------------------------------------------------------------------- */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/portmacro.h"
#include "freertos/idf_additions.h"

/* -------------------------------------------------------------------------- */
/* Target Platform Validation                                                 */
/* -------------------------------------------------------------------------- */

/*
 * This compatibility implementation currently supports
 * the original ESP32 target using ESP-IDF 5.5.
 */
#if !defined(CONFIG_IDF_TARGET_ESP32)
    #error "cmsis_esp port currently supports original ESP32 with ESP-IDF 5.5"
#endif

/* -------------------------------------------------------------------------- */
/* ESP-IDF Version Validation                                                 */
/* -------------------------------------------------------------------------- */

/* Reject ESP-IDF versions outside the validated release series. */
#if (ESP_IDF_VERSION_MAJOR != 5) || (ESP_IDF_VERSION_MINOR != 5)
    #error "cmsis_esp port is scoped to ESP-IDF 5.5; revalidate before upgrading"
#endif

/* -------------------------------------------------------------------------- */
/* FreeRTOS Trace Configuration                                               */
/* -------------------------------------------------------------------------- */

/* Default trace support to disabled when not configured. */
#ifndef configUSE_TRACE_FACILITY
    #define configUSE_TRACE_FACILITY 0
#endif

/* -------------------------------------------------------------------------- */
/* CMSIS-RTOS2 Feature Configuration                                          */
/* -------------------------------------------------------------------------- */

/*
 * These configuration options control the CMSIS-RTOS2
 * features exposed by this ESP-IDF compatibility layer.
 *
 * Existing definitions from the FreeRTOS configuration
 * take precedence over the defaults provided below.
 *
 * Ensure the values remain compatible with the selected
 * ESP-IDF sdkconfig and FreeRTOS configuration.
 */

/* -------------------------------------------------------------------------- */
/* Mutex Support                                                              */
/* -------------------------------------------------------------------------- */

/* Enable CMSIS-RTOS2 mutex management by default. */
#ifndef configUSE_OS2_MUTEX
    #define configUSE_OS2_MUTEX 1
#endif

/* -------------------------------------------------------------------------- */
/* Software Timer Support                                                     */
/* -------------------------------------------------------------------------- */

/* Enable CMSIS-RTOS2 software timer management by default. */
#ifndef configUSE_OS2_TIMER
    #define configUSE_OS2_TIMER 1
#endif

/* -------------------------------------------------------------------------- */
/* Thread Flags Support                                                       */
/* -------------------------------------------------------------------------- */

/* Enable CMSIS-RTOS2 thread notification flags by default. */
#ifndef configUSE_OS2_THREAD_FLAGS
    #define configUSE_OS2_THREAD_FLAGS 1
#endif

/* -------------------------------------------------------------------------- */
/* Thread Suspend and Resume Support                                          */
/* -------------------------------------------------------------------------- */

/* Enable thread suspension and resumption by default. */
#ifndef configUSE_OS2_THREAD_SUSPEND_RESUME
    #define configUSE_OS2_THREAD_SUSPEND_RESUME 1
#endif

/* -------------------------------------------------------------------------- */
/* Thread Enumeration Support                                                 */
/* -------------------------------------------------------------------------- */

/*
 * Disable thread enumeration by default.
 * Enumeration requires FreeRTOS trace facility support
 * and additional review for ESP-IDF SMP operation.
 */
#ifndef configUSE_OS2_THREAD_ENUMERATE
    #define configUSE_OS2_THREAD_ENUMERATE 0
#endif

/* -------------------------------------------------------------------------- */
/* Event Flags ISR Support                                                    */
/* -------------------------------------------------------------------------- */

/*
 * Disable CMSIS event flag modification from interrupt
 * context by default.
 *
 * ISR event group operations depend on the FreeRTOS
 * timer daemon command mechanism.
 */
#ifndef configUSE_OS2_EVENTFLAGS_FROM_ISR
    #define configUSE_OS2_EVENTFLAGS_FROM_ISR 0
#endif

/* -------------------------------------------------------------------------- */
/* CPU Affinity Support                                                       */
/* -------------------------------------------------------------------------- */

/* Disable optional CMSIS CPU affinity support by default. */
#ifndef configUSE_OS2_CPU_AFFINITY
    #define configUSE_OS2_CPU_AFFINITY 0
#endif

/* -------------------------------------------------------------------------- */
/* Interrupt Context Detection                                                */
/* -------------------------------------------------------------------------- */

/*
 * Detect native ESP-IDF interrupt context.
 *
 * An application critical section must not be interpreted
 * as interrupt context.
 *
 * ISR-safe APIs must only be invoked at interrupt priority
 * levels supported by the underlying FreeRTOS port.
 */
static inline uint32_t IRQ_Context(void)
{
    /* Return a normalized interrupt context indication. */
    return (xPortInIsrContext() != pdFALSE) ? 1U : 0U;
}

/* -------------------------------------------------------------------------- */
/* Internal Constants                                                         */
/* -------------------------------------------------------------------------- */

/* Maximum number of supported CMSIS thread notification flag bits. */
#define MAX_BITS_TASK_NOTIFY  31U

/* Maximum number of supported FreeRTOS event group flag bits. */
#define MAX_BITS_EVENT_GROUPS 24U

/* -------------------------------------------------------------------------- */
/* Invalid Thread Flag Bit Mask                                               */
/* -------------------------------------------------------------------------- */

/* Reserve the highest bit for CMSIS thread flag error reporting. */
#define THREAD_FLAGS_INVALID_BITS \
    (~((UINT32_C(1) << MAX_BITS_TASK_NOTIFY) - UINT32_C(1)))

/* -------------------------------------------------------------------------- */
/* Invalid Event Flag Bit Mask                                                */
/* -------------------------------------------------------------------------- */

/* Reserve the upper eight bits for CMSIS event flag error reporting. */
#define EVENT_FLAGS_INVALID_BITS \
    (~((UINT32_C(1) << MAX_BITS_EVENT_GROUPS) - UINT32_C(1)))

/* -------------------------------------------------------------------------- */
/* Kernel Version Configuration                                               */
/* -------------------------------------------------------------------------- */

/*
 * Encode the native FreeRTOS kernel version using
 * the CMSIS version representation.
 *
 * Format: major.minor.revision (mmnnnrrrr decimal encoding).
 */
#define KERNEL_VERSION                                         \
    (                                                          \
    ((uint32_t)tskKERNEL_VERSION_MAJOR * UINT32_C(10000000)) + \
    ((uint32_t)tskKERNEL_VERSION_MINOR * UINT32_C(10000)) +    \
    ((uint32_t)tskKERNEL_VERSION_BUILD * UINT32_C(1)))

/* -------------------------------------------------------------------------- */
/* Kernel Identification                                                      */
/* -------------------------------------------------------------------------- */

/* Construct the kernel identification string. */
#define KERNEL_ID ("FreeRTOS " tskKERNEL_VERSION_NUMBER)

/* -------------------------------------------------------------------------- */
/* Internal Type Definitions                                                  */
/* -------------------------------------------------------------------------- */

/* Timer callback metadata used by the CMSIS timer adapter. */
typedef cmsis_esp_timer_context_t TimerCallback_t;

/* Message queue metadata used by the CMSIS queue adapter. */
typedef cmsis_esp_queue_meta_t MessageQueueMeta_t;

/* Static message queue storage and metadata. */
typedef cmsis_esp_static_queue_t StaticMessageQueueMeta_t;

/* Mutex metadata used by the CMSIS mutex adapter. */
typedef cmsis_esp_mutex_meta_t MutexMeta_t;

/* Static mutex storage and metadata. */
typedef cmsis_esp_static_mutex_t StaticMutexMeta_t;

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Timer Deletion Synchronization                                             */
/* -------------------------------------------------------------------------- */

/*
 * Timer deletion and acknowledgment are ordered through the
 * FreeRTOS timer daemon command queue.
 *
 * Release/acquire synchronization prevents callback acknowledgment
 * state from being observed before the corresponding update.
 */
typedef struct
{
    atomic_uint done;
} TimerDeleteAck_t;
/*---------------------------------------------------------------------------*/

/* -------------------------------------------------------------------------- */
/* CMSIS-RTOS2 Kernel Management                                              */
/* -------------------------------------------------------------------------- */


osStatus_t osKernelInitialize(void)
{
    /* Reject kernel initialization from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /*
     * ESP-IDF initializes and starts FreeRTOS before app_main().
     * Treat an already-running scheduler as initialized.
     */
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        return osOK;
    }

    /* Application-managed scheduler initialization is unsupported. */
    return osError;
}

/* -------------------------------------------------------------------------- */

osStatus_t osKernelGetInfo(osVersion_t* version, char* id_buf, uint32_t id_size)
{
    uint32_t copy_size;

    /* Populate the kernel version structure when requested. */
    if(version != NULL)
    {
        version->api    = KERNEL_VERSION;
        version->kernel = KERNEL_VERSION;
    }

    /* Copy the kernel identification string when a buffer is provided. */
    if((id_buf != NULL) && (id_size != 0U))
    {
        /* Limit the copy capacity to the kernel ID string size. */
        copy_size = id_size;

        if(copy_size > (uint32_t)sizeof(KERNEL_ID))
        {
            copy_size = (uint32_t)sizeof(KERNEL_ID);
        }

        /* Copy identification characters when space is available. */
        if(copy_size > 1U)
        {
            (void)memcpy(id_buf, KERNEL_ID, (size_t)(copy_size - 1U));
        }

        /* Ensure the identification string is NUL-terminated. */
        id_buf[copy_size - 1U] = '\0';
    }

    return osOK;
}

/* -------------------------------------------------------------------------- */

osKernelState_t osKernelGetState(void)
{
    osKernelState_t state = osKernelInactive;

    /* Check whether the native FreeRTOS scheduler is running. */
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        state = osKernelRunning;
    }

    return state;
}

/* -------------------------------------------------------------------------- */

osStatus_t osKernelStart(void)
{
    /* Reject scheduler startup from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* ESP-IDF owns the scheduler startup sequence. */
    return osError;
}

/* -------------------------------------------------------------------------- */

int32_t osKernelLock(void)
{
    /* Global CMSIS scheduler locking is unsupported in ESP-IDF SMP. */
    return (int32_t)osError;
}

/* -------------------------------------------------------------------------- */

int32_t osKernelUnlock(void)
{
    /* Global CMSIS scheduler unlocking is unsupported. */
    return (int32_t)osError;
}

/* -------------------------------------------------------------------------- */

int32_t osKernelRestoreLock(int32_t lock)
{
    /* Mark the unsupported lock state parameter as unused. */
    (void)lock;

    /* Global CMSIS scheduler lock restoration is unsupported. */
    return (int32_t)osError;
}

/* -------------------------------------------------------------------------- */

uint32_t osKernelGetTickCount(void)
{
    TickType_t ticks;

    /* Select the appropriate tick API for the execution context. */
    if(IRQ_Context() != 0U)
    {
        /* Retrieve the tick count from interrupt context. */
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        /* Retrieve the tick count from task context. */
        ticks = xTaskGetTickCount();
    }

    /* Convert the native tick representation to CMSIS uint32_t. */
    return (uint32_t)ticks;
}

/* -------------------------------------------------------------------------- */

uint32_t osKernelGetTickFreq(void)
{
    /* Return the configured FreeRTOS tick frequency. */
    return (uint32_t)configTICK_RATE_HZ;
}

/* -------------------------------------------------------------------------- */
/* Kernel System Timer Count                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the current RTOS system timer count.
 *
 * @details
 * Uses the ESP-IDF monotonic timer to obtain the
 * elapsed time in microseconds.
 *
 * The native timer returns a 64-bit signed value.
 * The result is converted to the 32-bit CMSIS
 * system timer count representation.
 *
 * @return
 * Lower 32 bits of the ESP-IDF microsecond timer count.
 *
 * @note
 * The returned value wraps every 2^32 microseconds,
 * approximately 71.6 minutes.
 *
 * @note
 * This timer is independent of the FreeRTOS tick count.
 *
 * @par MISRA C Considerations
 * - Explicit integer conversion is used.
 * - No application state is modified.
 * - Timer wraparound behavior is documented.
 */
uint32_t osKernelGetSysTimerCount(void)
{
    int64_t timer_count;

    /* Retrieve the monotonic ESP-IDF timer count in microseconds. */
    timer_count = esp_timer_get_time();

    /* Return the lower 32 bits of the microsecond timer count. */
    return (uint32_t)((uint64_t)timer_count);
}

/* -------------------------------------------------------------------------- */
/* Kernel System Timer Frequency                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the RTOS system timer frequency.
 *
 * @details
 * Returns the frequency corresponding to the
 * ESP-IDF microsecond timer.
 *
 * Since esp_timer_get_time() reports microseconds,
 * the timer frequency is 1 MHz.
 *
 * @return
 * System timer frequency in hertz.
 *
 * @par MISRA C Considerations
 * - Uses an explicitly unsigned integer constant.
 * - No application state is accessed or modified.
 */
uint32_t osKernelGetSysTimerFreq(void)
{
    /* ESP-IDF monotonic timer resolution is one microsecond. */
    return 1000000U;
}

/* -------------------------------------------------------------------------- */
/* CMSIS-to-FreeRTOS Priority Conversion                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a CMSIS-RTOS2 priority to a native FreeRTOS priority.
 *
 * @details
 * Maps the supported CMSIS-RTOS2 priority range
 * into the available FreeRTOS application priority range.
 *
 * CMSIS priority range:
 * - osPriorityLow
 * - osPriorityRealtime7
 *
 * Native FreeRTOS priority range:
 * - 1U
 * - configMAX_PRIORITIES - 1U
 *
 * Native priority zero is intentionally excluded.
 *
 * The conversion uses linear integer scaling.
 * Multiple CMSIS priority levels may map to the
 * same native FreeRTOS priority.
 *
 * @param[in] priority
 * CMSIS-RTOS2 priority to convert.
 *
 * @param[out] native_priority
 * Pointer to the converted native FreeRTOS priority.
 *
 * @retval true
 * Priority conversion completed successfully.
 *
 * @retval false
 * Invalid output pointer, unsupported CMSIS priority,
 * or insufficient native priority levels.
 *
 * @note
 * Priority mapping is monotonic but not necessarily unique.
 *
 * @par MISRA C Considerations
 * - Output pointer is validated before use.
 * - CMSIS priority boundaries are checked.
 * - Division by zero is prevented by configuration checks.
 * - Integer conversions are explicit.
 * - Output is written only after validation.
 */
static bool cmsis_priority_to_native(osPriority_t priority, UBaseType_t* native_priority)
{
    const int32_t priority_low   = (int32_t)osPriorityLow;
    const int32_t priority_high  = (int32_t)osPriorityRealtime7;
    const int32_t priority_value = (int32_t)priority;

    uint32_t priority_offset;
    uint32_t cmsis_range;
    uint32_t native_range;
    uint32_t mapped_priority;

    /* Validate the output pointer and CMSIS priority range. */
    if((native_priority == NULL) || (priority_value < priority_low) ||
       (priority_value > priority_high) || (configMAX_PRIORITIES < 3))
    {
        return false;
    }

    /* Calculate the normalized CMSIS priority offset. */
    priority_offset = (uint32_t)(priority_value - priority_low);

    /* Calculate the supported CMSIS priority range. */
    cmsis_range = (uint32_t)(priority_high - priority_low);

    /* Reserve native priority zero for FreeRTOS idle usage. */
    native_range = (uint32_t)(configMAX_PRIORITIES - 2U);

    /* Scale the CMSIS priority into the native FreeRTOS range. */
    mapped_priority = 1U + ((priority_offset * native_range) / cmsis_range);

    /* Store the converted native priority. */
    *native_priority = (UBaseType_t)mapped_priority;

    return true;
}

/* -------------------------------------------------------------------------- */
/* FreeRTOS-to-CMSIS Priority Conversion                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a native FreeRTOS priority to a CMSIS-RTOS2 priority.
 *
 * @details
 * Maps a native FreeRTOS task priority into
 * the supported CMSIS-RTOS2 priority range.
 *
 * Native FreeRTOS priority zero and priorities
 * greater than or equal to configMAX_PRIORITIES
 * are rejected.
 *
 * The conversion uses linear integer scaling.
 *
 * @param[in] native_priority
 * Native FreeRTOS priority to convert.
 *
 * @return
 * Corresponding CMSIS-RTOS2 priority.
 *
 * @retval osPriorityError
 * Native priority is outside the supported range.
 *
 * @note
 * Due to priority range compression, converting
 * from CMSIS to native and back may not reproduce
 * the original CMSIS priority exactly.
 *
 * @par MISRA C Considerations
 * - Native priority boundaries are explicitly checked.
 * - Division by zero is prevented by configuration checks.
 * - Intermediate calculations use explicitly sized types.
 * - Final enumeration conversion is explicit.
 */
static osPriority_t cmsis_priority_from_native(UBaseType_t native_priority)
{
    uint32_t native_offset;
    uint32_t native_range;
    uint32_t cmsis_range;
    uint32_t mapped_offset;

    int32_t mapped_priority;

    /* Reject unsupported native FreeRTOS priorities. */
    if((native_priority == 0U) || (native_priority >= (UBaseType_t)configMAX_PRIORITIES) ||
       (configMAX_PRIORITIES < 3))
    {
        return osPriorityError;
    }

    /* Normalize the native priority to a zero-based offset. */
    native_offset = (uint32_t)(native_priority - 1U);

    /* Calculate the available native priority range. */
    native_range = (uint32_t)(configMAX_PRIORITIES - 2U);

    /* Calculate the supported CMSIS priority range. */
    cmsis_range = (uint32_t)((int32_t)osPriorityRealtime7 - (int32_t)osPriorityLow);

    /* Scale the native priority into the CMSIS priority range. */
    mapped_offset = (native_offset * cmsis_range) / native_range;

    /* Add the CMSIS minimum priority offset. */
    mapped_priority = (int32_t)osPriorityLow + (int32_t)mapped_offset;

    /* Return the corresponding CMSIS priority enumeration. */
    return (osPriority_t)mapped_priority;
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* CMSIS-RTOS2 Thread Management                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Create a new RTOS thread.
 *
 * @details
 * Creates a FreeRTOS task using the CMSIS-RTOS2 thread
 * configuration supplied by the application.
 *
 * The function supports:
 * - Static and dynamic task allocation.
 * - Application-defined thread names.
 * - CMSIS-to-FreeRTOS priority conversion.
 * - Application-provided stack and control block memory.
 * - Core affinity configuration.
 *
 * The implementation rejects:
 * - Invalid thread entry functions.
 * - Thread creation from interrupt context.
 * - Unsupported TrustZone module identifiers.
 * - Unsupported thread attribute flags.
 * - Joinable thread attributes.
 * - Invalid stack sizes and static allocation buffers.
 * - Unsupported core affinity masks.
 *
 * @param[in] func
 * Pointer to the thread entry function.
 *
 * @param[in] argument
 * Optional argument passed to the thread entry function.
 *
 * @param[in] attr
 * Optional pointer to the thread attribute structure.
 * NULL selects the implementation defaults.
 *
 * @return
 * Identifier of the newly created thread,
 * or NULL if thread creation fails.
 *
 * @note
 * Joinable threads are not supported by this implementation.
 *
 * @note
 * ESP-IDF FreeRTOS expects stack sizes in bytes for
 * both dynamic and static task creation APIs.
 *
 * @note
 * Static thread allocation requires application-provided
 * task control block and stack memory.
 *
 * @warning
 * Application-provided static task memory must remain
 * valid throughout the lifetime of the created thread.
 *
 * @par MISRA C Considerations
 * - Function pointers are validated before use.
 * - Interrupt context is checked before task creation.
 * - Thread attributes are validated before processing.
 * - Stack size and alignment requirements are checked.
 * - Priority conversion is validated.
 * - Allocation paths are selected through compile-time
 *   FreeRTOS configuration options.
 * - Explicit pointer conversions are used where required
 *   by the FreeRTOS interface.
 */
osThreadId_t osThreadNew(osThreadFunc_t func, void* argument, const osThreadAttr_t* attr)
{
    TaskHandle_t handle = NULL;

    const char* name = "cmsis";

    uint32_t stack_bytes = (uint32_t)configMINIMAL_STACK_SIZE;

    osPriority_t wanted = osPriorityNormal;

    UBaseType_t native_priority = 0U;

    bool static_alloc = false;

    BaseType_t core = tskNO_AFFINITY;

    /* Validate the thread entry function and execution context. */
    if((func == NULL) || (IRQ_Context() != 0U))
    {
        return NULL;
    }

    /* Process optional application-provided thread attributes. */
    if(attr != NULL)
    {
        /* Override the default thread name when provided. */
        if(attr->name != NULL)
        {
            name = attr->name;
        }

        /* Reject unsupported TrustZone module configuration. */
        if(attr->tz_module != 0U)
        {
            return NULL;
        }

        /* Override the default thread priority when requested. */
        if(attr->priority != osPriorityNone)
        {
            wanted = attr->priority;
        }

        /*
         * Reject unsupported thread attribute bits.
         * The joinable attribute is checked separately.
         */
        if((attr->attr_bits & (uint32_t)(~osThreadJoinable)) != 0U)
        {
            return NULL;
        }

        /* Joinable thread functionality is not implemented. */
        if((attr->attr_bits & osThreadJoinable) != 0U)
        {
            return NULL;
        }

        /* Override the default stack capacity when provided. */
        if(attr->stack_size != 0U)
        {
            stack_bytes = attr->stack_size;
        }

        /*
         * Validate stack size alignment with the
         * native FreeRTOS stack element size.
         */
        if((attr->stack_size != 0U) && ((attr->stack_size % sizeof(StackType_t)) != 0U))
        {
            return NULL;
        }

        /*
         * Determine whether application-provided
         * static task allocation is requested.
         */
        if((attr->cb_mem != NULL) || (attr->stack_mem != NULL) || (attr->cb_size != 0U))
        {
            /*
             * Validate the supplied static control block,
             * stack buffer, stack capacity, and alignment.
             */
            if((attr->cb_mem == NULL) || (attr->cb_size < sizeof(StaticTask_t)) ||
               (attr->stack_mem == NULL) || (attr->stack_size == 0U) ||
               (((uintptr_t)attr->stack_mem % sizeof(StackType_t)) != 0U))
            {
                return NULL;
            }

            /* Select the static task allocation path. */
            static_alloc = true;
        }

        /* Apply thread core affinity when explicitly configured. */
        if(attr->affinity_mask != 0U)
        {
            if(attr->affinity_mask == 1U)
            {
                /* Pin the thread to core zero. */
                core = 0;
            }
#if (configNUMBER_OF_CORES > 1)
            else if(attr->affinity_mask == 2U)
            {
                /* Pin the thread to core one. */
                core = 1;
            }
            else if(attr->affinity_mask == 3U)
            {
                /* Allow execution on either available core. */
                core = tskNO_AFFINITY;
            }
#endif
            else
            {
                /* Reject unsupported core affinity masks. */
                return NULL;
            }
        }
    }

    /*
     * Validate the minimum supported stack capacity
     * and convert the CMSIS priority to FreeRTOS priority.
     */
    if((stack_bytes < 512U) || (!cmsis_priority_to_native(wanted, &native_priority)))
    {
        return NULL;
    }

    /* Select the appropriate FreeRTOS task allocation method. */
    if(static_alloc)
    {
#if (configSUPPORT_STATIC_ALLOCATION == 1)

        /*
         * Create a statically allocated FreeRTOS task.
         *
         * ESP-IDF expects stack_bytes in bytes.
         * Task control block and stack memory are
         * provided by the application.
         */
        handle = xTaskCreateStaticPinnedToCore((TaskFunction_t)func, name,
                                               stack_bytes, argument, native_priority,
                                               (StackType_t*)attr->stack_mem,
                                               (StaticTask_t*)attr->cb_mem, core);

#endif
    }
    else
    {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

        /*
         * Create a dynamically allocated FreeRTOS task.
         *
         * ESP-IDF expects stack_bytes in bytes.
         */
        if(xTaskCreatePinnedToCore((TaskFunction_t)func, name, stack_bytes, argument,
                                   native_priority, &handle, core) != pdPASS)
        {
            /* Ensure an invalid handle is returned on failure. */
            handle = NULL;
        }

#endif
    }

    /* Return the created task handle as a CMSIS thread identifier. */
    return (osThreadId_t)handle;
}

/* -------------------------------------------------------------------------- */
/* Thread Name                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the name of an RTOS thread.
 *
 * @details
 * Returns the name associated with the supplied
 * FreeRTOS task handle.
 *
 * The function uses the native FreeRTOS
 * pcTaskGetName() API.
 *
 * @param[in] thread_id
 * Identifier of the target thread.
 *
 * @return
 * Pointer to the thread name as a NUL-terminated string,
 * or NULL if the supplied thread identifier is NULL.
 *
 * @note
 * The returned name is owned by the FreeRTOS task
 * and must not be modified by the application.
 *
 * @warning
 * Although this wrapper permits calls from interrupt
 * context, the caller must verify that the native
 * FreeRTOS implementation supports the operation
 * in the intended execution context.
 *
 * @par MISRA C Considerations
 * - Thread identifier is validated before use.
 * - Explicit task handle conversion is used.
 * - No application memory is allocated or modified.
 */
const char* osThreadGetName(osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;

    const char* name = NULL;

    /* Retrieve the name only when a valid task handle is supplied. */
    if(hTask != NULL)
    {
        /*
         * Preserve the existing implementation behavior:
         * retrieve the task name using the native API.
         */
        name = pcTaskGetName(hTask);
    }

    return name;
}

/* -------------------------------------------------------------------------- */
/* Current Thread Identifier                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the identifier of the current thread.
 *
 * @details
 * Retrieves the current FreeRTOS task handle
 * and converts it to the CMSIS-RTOS2 thread
 * identifier representation.
 *
 * @return
 * Identifier of the currently executing thread.
 *
 * @note
 * This implementation uses the native
 * xTaskGetCurrentTaskHandle() API.
 *
 * @par MISRA C Considerations
 * - Uses an explicit handle conversion.
 * - No shared application state is modified.
 * - No dynamic memory allocation is performed.
 */
osThreadId_t osThreadGetId(void)
{
    TaskHandle_t handle;

    /* Retrieve the currently executing FreeRTOS task handle. */
    handle = xTaskGetCurrentTaskHandle();

    /* Convert the native task handle into a CMSIS thread identifier. */
    return (osThreadId_t)handle;
}

/* -------------------------------------------------------------------------- */
/* Thread State                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the current state of an RTOS thread.
 *
 * @details
 * Queries the native FreeRTOS task state and
 * converts it into the corresponding CMSIS-RTOS2
 * thread state.
 *
 * Native FreeRTOS states are mapped as follows:
 *
 * - eRunning   : osThreadRunning
 * - eReady     : osThreadReady
 * - eBlocked   : osThreadBlocked
 * - eSuspended : osThreadBlocked
 * - eDeleted   : osThreadError
 * - eInvalid   : osThreadError
 *
 * @param[in] thread_id
 * Identifier of the thread whose state is requested.
 *
 * @retval osThreadRunning
 * Thread is currently executing.
 *
 * @retval osThreadReady
 * Thread is ready for execution.
 *
 * @retval osThreadBlocked
 * Thread is blocked or suspended.
 *
 * @retval osThreadError
 * Invalid thread identifier, interrupt context,
 * or unsupported native thread state.
 *
 * @par MISRA C Considerations
 * - Thread identifier and execution context are validated.
 * - All supported native states are handled explicitly.
 * - Every switch case uses a scoped block.
 * - The switch statement includes a default branch.
 * - No application state is modified.
 */
osThreadState_t osThreadGetState(osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;

    osThreadState_t state = osThreadError;

    /* Reject unsupported execution contexts and invalid handles. */
    if((IRQ_Context() != 0U) || (hTask == NULL))
    {
        return osThreadError;
    }

    /* Convert the native FreeRTOS task state into CMSIS state. */
    switch(eTaskGetState(hTask))
    {
        case eRunning:
        {
            /* Thread is currently executing. */
            state = osThreadRunning;
            break;
        }

        case eReady:
        {
            /* Thread is ready to execute. */
            state = osThreadReady;
            break;
        }

        case eBlocked:
        case eSuspended:
        {
            /* Treat blocked and suspended tasks as CMSIS blocked. */
            state = osThreadBlocked;
            break;
        }

        case eDeleted:
        case eInvalid:
        default:
        {
            /* Report unsupported or invalid task states. */
            state = osThreadError;
            break;
        }
    }

    return state;
}

/* -------------------------------------------------------------------------- */
/* Thread Stack Space                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the available stack space of a thread.
 *
 * @details
 * Retrieves the minimum remaining stack capacity
 * recorded by the FreeRTOS stack high-water-mark
 * mechanism.
 *
 * In the targeted ESP-IDF implementation,
 * the returned stack high-water mark is expressed
 * in bytes.
 *
 * @param[in] thread_id
 * Identifier of the target thread.
 *
 * @return
 * Recorded minimum available stack space in bytes,
 * or zero for an invalid thread identifier or
 * unsupported execution context.
 *
 * @note
 * This value represents the stack high-water mark,
 * not necessarily the current unused stack capacity.
 *
 * @warning
 * Stack high-water-mark functionality depends on the
 * native FreeRTOS configuration and target port.
 *
 * @par MISRA C Considerations
 * - Thread identifier and context are validated.
 * - Native return value is explicitly converted.
 * - No application state is modified.
 */
uint32_t osThreadGetStackSpace(osThreadId_t thread_id)
{
    UBaseType_t stack_space;

    /* Reject interrupt context and invalid thread handles. */
    if((IRQ_Context() != 0U) || (thread_id == NULL))
    {
        return 0U;
    }

    /* Retrieve the recorded native stack high-water mark. */
    stack_space = uxTaskGetStackHighWaterMark((TaskHandle_t)thread_id);

    /* Return the available stack space in bytes. */
    return (uint32_t)stack_space;
}

/* -------------------------------------------------------------------------- */
/* Thread Priority Configuration                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Change the priority of an RTOS thread.
 *
 * @details
 * Validates the requested CMSIS-RTOS2 thread priority
 * and converts it into a native FreeRTOS priority.
 *
 * The native FreeRTOS task priority is updated
 * using vTaskPrioritySet().
 *
 * @param[in] thread_id
 * Identifier of the thread to modify.
 *
 * @param[in] priority
 * Requested CMSIS-RTOS2 priority.
 *
 * @retval osOK
 * Thread priority successfully updated.
 *
 * @retval osErrorISR
 * Function called from interrupt context.
 *
 * @retval osErrorParameter
 * Invalid thread identifier or unsupported priority.
 *
 * @note
 * Multiple CMSIS priority levels may map to the
 * same native FreeRTOS priority.
 *
 * @par MISRA C Considerations
 * - Interrupt context is explicitly validated.
 * - Thread identifier is checked before use.
 * - Requested priority is validated before assignment.
 * - Explicit task handle conversion is used.
 */
osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
{
    UBaseType_t native_priority = 0U;

    /* Reject priority changes from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the thread identifier and convert its priority. */
    if((thread_id == NULL) || (!cmsis_priority_to_native(priority, &native_priority)))
    {
        return osErrorParameter;
    }

    /* Apply the converted native FreeRTOS task priority. */
    vTaskPrioritySet((TaskHandle_t)thread_id, native_priority);

    return osOK;
}

/* -------------------------------------------------------------------------- */
/* Thread Priority Retrieval                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the current priority of an RTOS thread.
 *
 * @details
 * Retrieves the native FreeRTOS task priority
 * and converts it into the corresponding
 * CMSIS-RTOS2 thread priority.
 *
 * @param[in] thread_id
 * Identifier of the target thread.
 *
 * @return
 * Converted CMSIS-RTOS2 thread priority.
 *
 * @retval osPriorityError
 * Invalid thread identifier, interrupt context,
 * or unsupported native priority.
 *
 * @note
 * Priority conversion may be lossy because the
 * CMSIS priority range is larger than the available
 * native FreeRTOS priority range.
 *
 * @par MISRA C Considerations
 * - Thread identifier and execution context are validated.
 * - Native priority is retrieved through the FreeRTOS API.
 * - Priority conversion is performed by a dedicated helper.
 * - No application state is modified.
 */
osPriority_t osThreadGetPriority(osThreadId_t thread_id)
{
    UBaseType_t native_priority;

    /* Reject interrupt context and invalid thread handles. */
    if((IRQ_Context() != 0U) || (thread_id == NULL))
    {
        return osPriorityError;
    }

    /* Retrieve the native FreeRTOS task priority. */
    native_priority = uxTaskPriorityGet((TaskHandle_t)thread_id);

    /* Convert the native priority to its CMSIS representation. */
    return cmsis_priority_from_native(native_priority);
}

/* -------------------------------------------------------------------------- */
/* Thread Yield                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Yield execution to another ready thread.
 *
 * @details
 * Requests the FreeRTOS scheduler to yield execution
 * from the calling thread.
 *
 * This allows the scheduler to select another
 * eligible ready task according to FreeRTOS
 * scheduling rules.
 *
 * @retval osOK
 * Scheduler yield operation requested successfully.
 *
 * @retval osErrorISR
 * Function called from interrupt context.
 *
 * @note
 * Yielding does not guarantee that a different
 * thread will execute immediately.
 *
 * @par MISRA C Considerations
 * - Interrupt context is checked before scheduling.
 * - Native scheduler functionality is invoked only
 *   from task context.
 * - All execution paths return a defined status.
 */
osStatus_t osThreadYield(void)
{
    osStatus_t status;

    /* Reject scheduler yielding from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else
    {
        /* Request FreeRTOS to yield execution. */
        taskYIELD();

        status = osOK;
    }

    return status;
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Thread Suspend                                                             */
/* -------------------------------------------------------------------------- */

#if (configUSE_OS2_THREAD_SUSPEND_RESUME == 1)

osStatus_t osThreadSuspend(osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t status;

    /* Validate execution context and thread identifier. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if(hTask == NULL)
    {
        status = osErrorParameter;
    }
    else
    {
        /* Suspend the specified FreeRTOS task. */
        vTaskSuspend(hTask);
        status = osOK;
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Thread Resume                                                              */
/* -------------------------------------------------------------------------- */

osStatus_t osThreadResume(osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;

    /* Reject calls from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the thread identifier. */
    if(hTask == NULL)
    {
        return osErrorParameter;
    }

    /*
     * Only explicitly suspended tasks may be resumed.
     * Do not use xTaskAbortDelay() to unblock sleeping tasks.
     */
    if(eTaskGetState(hTask) != eSuspended)
    {
        return osErrorResource;
    }

    /* Resume the suspended FreeRTOS task. */
    vTaskResume(hTask);

    return osOK;
}

#endif /* (configUSE_OS2_THREAD_SUSPEND_RESUME == 1) */

/* -------------------------------------------------------------------------- */
/* Thread Exit                                                                */
/* -------------------------------------------------------------------------- */

__NO_RETURN void osThreadExit(void)
{
#ifndef USE_FreeRTOS_HEAP_1

    /* Delete the currently executing FreeRTOS task. */
    vTaskDelete(NULL);

#endif

    /* Prevent returning from a thread termination function. */
    for(;;)
    {
        /* Intentionally empty. */
    }
}

/* -------------------------------------------------------------------------- */
/* Thread Termination                                                         */
/* -------------------------------------------------------------------------- */

osStatus_t osThreadTerminate(osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t status;

#ifndef USE_FreeRTOS_HEAP_1

    eTaskState task_state;

    /* Reject thread termination from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if(hTask == NULL)
    {
        /* Reject an invalid thread identifier. */
        status = osErrorParameter;
    }
    else
    {
        /* Retrieve the current native FreeRTOS task state. */
        task_state = eTaskGetState(hTask);

        if(task_state != eDeleted)
        {
            /* Terminate the specified FreeRTOS task. */
            vTaskDelete(hTask);
            status = osOK;
        }
        else
        {
            /* Reject termination of an already deleted task. */
            status = osErrorResource;
        }
    }

#else

    /* Task deletion is unsupported with the selected heap configuration. */
    (void)hTask;
    status = osError;

#endif

    return status;
}

/* -------------------------------------------------------------------------- */
/* Thread Count                                                               */
/* -------------------------------------------------------------------------- */

uint32_t osThreadGetCount(void)
{
    uint32_t count = 0U;

    /* Retrieve the task count only from task context. */
    if(IRQ_Context() == 0U)
    {
        count = (uint32_t)uxTaskGetNumberOfTasks();
    }

    return count;
}

/* -------------------------------------------------------------------------- */
/* Thread Enumeration                                                         */
/* -------------------------------------------------------------------------- */

#if ((configUSE_OS2_THREAD_ENUMERATE == 1) && (configUSE_TRACE_FACILITY == 1))

uint32_t osThreadEnumerate(osThreadId_t* thread_array, uint32_t array_items)
{
    uint32_t i;
    uint32_t count     = 0U;
    TaskStatus_t* task = NULL;

    /* Validate execution context and destination array. */
    if((IRQ_Context() != 0U) || (thread_array == NULL) || (array_items == 0U))
    {
        return 0U;
    }

    /*
     * Suspend scheduling on the calling core while
     * retrieving the FreeRTOS task snapshot.
     */
    vTaskSuspendAll();

    /* Determine the number of active FreeRTOS tasks. */
    count = (uint32_t)uxTaskGetNumberOfTasks();

    /* Allocate temporary storage for task status information. */
    task = (TaskStatus_t*)pvPortMalloc((size_t)count * sizeof(TaskStatus_t));

    if(task != NULL)
    {
        /* Retrieve the current FreeRTOS task information. */
        count = (uint32_t)uxTaskGetSystemState(task, (UBaseType_t)count, NULL);

        /* Copy task handles into the caller-provided array. */
        for(i = 0U; (i < count) && (i < array_items); ++i)
        {
            thread_array[i] = (osThreadId_t)task[i].xHandle;
        }

        /* Return the number of thread identifiers copied. */
        count = i;
    }
    else
    {
        /* Report an empty result if temporary allocation fails. */
        count = 0U;
    }

    /* Resume scheduling on the calling core. */
    (void)xTaskResumeAll();

    /* Release temporary task status storage. */
    if(task != NULL)
    {
        vPortFree(task);
    }

    return count;
}

#endif /* Thread enumeration configuration */

/* -------------------------------------------------------------------------- */
/* Thread Affinity Configuration                                              */
/* -------------------------------------------------------------------------- */

osStatus_t osThreadSetAffinityMask(osThreadId_t thread_id, uint32_t affinity_mask)
{
    /*
     * Changing thread affinity after creation is unsupported
     * by this CMSIS-to-ESP-IDF compatibility implementation.
     */
    (void)affinity_mask;

    if(thread_id == NULL)
    {
        return osErrorParameter;
    }

    return osErrorResource;
}

/* -------------------------------------------------------------------------- */
/* Thread Affinity Retrieval                                                  */
/* -------------------------------------------------------------------------- */

uint32_t osThreadGetAffinityMask(osThreadId_t thread_id)
{
    BaseType_t core;
    uint32_t affinity_mask = 0U;

    /* Reject invalid identifiers and interrupt context. */
    if((thread_id == NULL) || (IRQ_Context() != 0U))
    {
        return 0U;
    }

    /* Retrieve the native FreeRTOS task core affinity. */
    core = xTaskGetCoreID((TaskHandle_t)thread_id);

    if(core == tskNO_AFFINITY)
    {
        /* Return a mask containing all configured CPU cores. */
        affinity_mask = ((uint32_t)1U << (uint32_t)configNUMBER_OF_CORES) - 1U;
    }
    else if((core < 0) || (core >= configNUMBER_OF_CORES))
    {
        /* Reject an unsupported native core identifier. */
        affinity_mask = 0U;
    }
    else
    {
        /* Convert the native core identifier to a bitmask. */
        affinity_mask = (uint32_t)1U << (uint32_t)core;
    }

    return affinity_mask;
}

/* -------------------------------------------------------------------------- */
/* Thread Flags Management                                                    */
/* -------------------------------------------------------------------------- */

// #if (configUSE_OS2_THREAD_FLAGS == 1)

/* -------------------------------------------------------------------------- */
/* Thread Flags Set                                                           */
/* -------------------------------------------------------------------------- */

uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)
{
    TaskHandle_t hTask   = (TaskHandle_t)thread_id;
    uint32_t prior_flags = 0U;
    BaseType_t result;
    BaseType_t yield = pdFALSE;

    /* Validate the thread identifier and requested flag bits. */
    if((hTask == NULL) || (flags == 0U) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U))
    {
        return osFlagsErrorParameter;
    }

    /* Select the appropriate task notification API. */
    if(IRQ_Context() != 0U)
    {
        /* Set thread flags from interrupt context. */
        result = xTaskNotifyAndQueryFromISR(hTask, flags, eSetBits, &prior_flags, &yield);

        /* Request a context switch when required. */
        portYIELD_FROM_ISR(yield);
    }
    else
    {
        /* Set thread flags from normal task context. */
        result = xTaskNotifyAndQuery(hTask, flags, eSetBits, &prior_flags);
    }

    /* Return the resulting flag value or an operation error. */
    if(result == pdPASS)
    {
        return (prior_flags | flags);
    }

    return osFlagsErrorResource;
}

/* -------------------------------------------------------------------------- */

// #endif /* (configUSE_OS2_THREAD_FLAGS == 1) */

/* -------------------------------------------------------------------------- */
/* Thread Flags Clear                                                         */
/* -------------------------------------------------------------------------- */

uint32_t osThreadFlagsClear(uint32_t flags)
{
    /* Reject calls from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osFlagsErrorISR;
    }

    /* Reject reserved or unsupported thread flag bits. */
    if((flags & THREAD_FLAGS_INVALID_BITS) != 0U)
    {
        return osFlagsErrorParameter;
    }

    /*
     * Atomically clear the specified thread flag bits.
     * Return the flag value before the clear operation.
     */
    return (uint32_t)ulTaskNotifyValueClear(NULL, flags);
}

/* -------------------------------------------------------------------------- */
/* Thread Flags Get                                                           */
/* -------------------------------------------------------------------------- */

uint32_t osThreadFlagsGet(void)
{
    /* Reject calls from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osFlagsErrorISR;
    }

    /*
     * Retrieve the current thread notification value
     * without clearing any flag bits.
     */
    return (uint32_t)ulTaskNotifyValueClear(NULL, 0U);
}

/* -------------------------------------------------------------------------- */
/* Thread Flags Wait                                                          */
/* -------------------------------------------------------------------------- */

uint32_t osThreadFlagsWait(uint32_t flags, uint32_t options, uint32_t timeout)
{
    TickType_t remaining = (TickType_t)timeout;
    TimeOut_t time_state;

    uint32_t current  = 0U;
    uint32_t observed = 0U;

    BaseType_t result;

    /* Reject calls from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osFlagsErrorISR;
    }

    /* Validate requested flags and supported wait options. */
    if((flags == 0U) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U) ||
       ((options & ~(osFlagsWaitAll | osFlagsNoClear)) != 0U))
    {
        return osFlagsErrorParameter;
    }

    /* Capture the initial FreeRTOS timeout state. */
    vTaskSetTimeOutState(&time_state);

    /* Continue until the requested flags are available or timeout occurs. */
    for(;;)
    {
        /*
         * Read the current sticky flag bits without clearing them.
         * Accumulate observed flags across notification events.
         */
        current = (uint32_t)ulTaskNotifyValueClear(NULL, 0U);
        observed |= current;

        /* Check whether the requested wait condition is satisfied. */
        if((((options & osFlagsWaitAll) != 0U) && ((observed & flags) == flags)) ||
           (((options & osFlagsWaitAll) == 0U) && ((observed & flags) != 0U)))
        {
            /* Clear the matched flags unless osFlagsNoClear is selected. */
            if((options & osFlagsNoClear) == 0U)
            {
                (void)ulTaskNotifyValueClear(NULL, observed & flags);
            }

            return observed;
        }

        /* Handle zero timeout or an expired wait period. */
        if(remaining == 0U)
        {
            if(timeout == 0U)
            {
                return osFlagsErrorResource;
            }

            return osFlagsErrorTimeout;
        }

        /* Wait for a task notification without clearing flag bits. */
        result = xTaskNotifyWait(0U, 0U, &current, remaining);

        if(result == pdPASS)
        {
            /* Preserve the flags received from the notification. */
            observed |= current;
        }
        else
        {
            /* No notification was received within the wait period. */
            return osFlagsErrorTimeout;
        }

        /* Update the remaining timeout for a finite wait. */
        if((timeout != osWaitForever) &&
           (xTaskCheckForTimeOut(&time_state, &remaining) != pdFALSE))
        {
            remaining = 0U;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Generic Wait Functions                                                     */
/* -------------------------------------------------------------------------- */
/* RTOS Delay */
/* -------------------------------------------------------------------------- */

osStatus_t osDelay(uint32_t ticks)
{
    /* Reject delay requests from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* A zero-tick delay is invalid for this CMSIS API. */
    if(ticks == 0U)
    {
        return osErrorParameter;
    }

    /* Suspend the current task for the requested number of ticks. */
    vTaskDelay((TickType_t)ticks);

    return osOK;
}

/* -------------------------------------------------------------------------- */
/* RTOS Delay Until                                                           */
/* -------------------------------------------------------------------------- */

osStatus_t osDelayUntil(uint32_t ticks)
{
    TickType_t current_ticks;
    TickType_t delay;

    osStatus_t status;

    /* Reject absolute delay requests from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else
    {
        status = osOK;

        /* Retrieve the current FreeRTOS kernel tick count. */
        current_ticks = xTaskGetTickCount();

        /* Calculate the remaining ticks until the target time. */
        delay = (TickType_t)ticks - current_ticks;

        /*
         * Accept only a future target within half
         * the native tick counter range.
         *
         * This supports wraparound-aware tick comparisons.
         */
        if((delay != 0U) && ((delay >> ((8U * sizeof(TickType_t)) - 1U)) == 0U))
        {
            /* Delay until the specified absolute tick value. */
            if(xTaskDelayUntil(&current_ticks, delay) == pdFALSE)
            {
                /* The requested delay did not block the task. */
                status = osError;
            }
        }
        else
        {
            /* Reject an expired or invalid absolute tick target. */
            status = osErrorParameter;
        }
    }

    return status;
}


/* -------------------------------------------------------------------------- */
/* Timer Deletion Acknowledgment Callback                                     */
/* -------------------------------------------------------------------------- */

static void cmsis_timer_delete_ack(void* arg, uint32_t unused)
{
    TimerDeleteAck_t* ack = (TimerDeleteAck_t*)arg;

    /* Mark the unused callback parameter explicitly. */
    (void)unused;

    /* Publish the timer deletion acknowledgment. */
    atomic_store_explicit(&ack->done, 1U, memory_order_release);
}

/* -------------------------------------------------------------------------- */
/* Timer Management Functions                                                 */
/* -------------------------------------------------------------------------- */

#if (configUSE_OS2_TIMER == 1)

/* -------------------------------------------------------------------------- */
/* Internal Timer Callback                                                    */
/* -------------------------------------------------------------------------- */

static void TimerCallback(TimerHandle_t hTimer)
{
    TimerCallback_t* callback;

    /* Retrieve the callback context from the FreeRTOS timer ID. */
    callback = (TimerCallback_t*)pvTimerGetTimerID(hTimer);

    /* Invoke the application callback when configured. */
    if((callback != NULL) && (callback->func != NULL))
    {
        callback->func(callback->arg);
    }
}

/* -------------------------------------------------------------------------- */
/* Timer Creation                                                             */
/* -------------------------------------------------------------------------- */

osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void* argument, const osTimerAttr_t* attr)
{
    TimerCallback_t* callback = NULL;
    TimerHandle_t timer       = NULL;

    const char* name = "cmsis-timer";

    BaseType_t reload;
    bool using_static = false;

    /* Reject unsupported timer attribute flags. */
    if((attr != NULL) && (attr->attr_bits != 0U))
    {
        return NULL;
    }

    /* Validate execution context, callback, and timer type. */
    if((IRQ_Context() != 0U) || (func == NULL) ||
       ((type != osTimerOnce) && (type != osTimerPeriodic)))
    {
        return NULL;
    }

    /* Process optional application-provided timer attributes. */
    if(attr != NULL)
    {
        /* Override the default timer name when provided. */
        if(attr->name != NULL)
        {
            name = attr->name;
        }

        /* Check whether static timer allocation is requested. */
        if((attr->cb_mem != NULL) || (attr->cb_size != 0U))
        {
            /*
             * The application-provided memory must accommodate
             * both the FreeRTOS timer control block and the
             * CMSIS callback context.
             */
            if((attr->cb_mem == NULL) || (attr->cb_size < sizeof(cmsis_esp_static_timer_t)) ||
               (((uintptr_t)attr->cb_mem % sizeof(void*)) != 0U))
            {
                return NULL;
            }

            /* Select static timer allocation. */
            using_static = true;
        }
    }

    /* Prepare the timer callback context. */
    if(using_static)
    {
    #if (configSUPPORT_STATIC_ALLOCATION == 1)

        /*
         * Obtain callback storage from the application-provided
         * static timer memory.
         *
         * The caller must retain this memory until timer
         * deletion has completed.
         */
        callback = &((cmsis_esp_static_timer_t*)attr->cb_mem)->callback;

        callback->dynamic_alloc = 0U;

    #endif
    }
    else
    {
    #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

        /* Allocate the callback context dynamically. */
        callback = (TimerCallback_t*)pvPortMalloc(sizeof(*callback));

        if(callback != NULL)
        {
            callback->dynamic_alloc = 1U;
        }

    #endif
    }

    /* Reject timer creation if callback storage is unavailable. */
    if(callback == NULL)
    {
        return NULL;
    }

    /* Initialize the application callback context. */
    callback->func = func;
    callback->arg  = argument;

    /* Configure timer auto-reload behavior. */
    reload = (type == osTimerPeriodic) ? pdTRUE : pdFALSE;

    /* Create the FreeRTOS software timer. */
    if(using_static)
    {
    #if (configSUPPORT_STATIC_ALLOCATION == 1)

        /* Create a statically allocated software timer. */
        timer = xTimerCreateStatic(name, 1U, reload, callback, TimerCallback,
                                   &((cmsis_esp_static_timer_t*)attr->cb_mem)->cb);

    #endif
    }
    else
    {
    #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

        /* Create a dynamically allocated software timer. */
        timer = xTimerCreate(name, 1U, reload, callback, TimerCallback);

    #endif
    }

    /* Release dynamically allocated callback storage on failure. */
    if((timer == NULL) && (callback->dynamic_alloc != 0U))
    {
        vPortFree(callback);
    }

    /* Return the native timer handle as a CMSIS timer identifier. */
    return (osTimerId_t)timer;
}

/* -------------------------------------------------------------------------- */
/* Timer Name                                                                 */
/* -------------------------------------------------------------------------- */

const char* osTimerGetName(osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    const char* name     = NULL;

    /* Retrieve the timer name only for a valid timer handle. */
    if(hTimer != NULL)
    {
        /*
         * Preserve the original implementation behavior.
         * The native FreeRTOS API is used in either context.
         */
        name = pcTimerGetName(hTimer);
    }

    return name;
}

/* -------------------------------------------------------------------------- */
/* Timer Start                                                                */
/* -------------------------------------------------------------------------- */

osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t status;

    /* Reject timer operations from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if((hTimer == NULL) || (ticks == 0U))
    {
        /* Validate the timer identifier and period. */
        status = osErrorParameter;
    }
    else
    {
        /*
         * Set the timer period and start or restart it.
         * Do not block while submitting the timer command.
         */
        if(xTimerChangePeriod(hTimer, (TickType_t)ticks, 0U) == pdPASS)
        {
            status = osOK;
        }
        else
        {
            /* Timer command submission failed. */
            status = osErrorResource;
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Timer Stop                                                                 */
/* -------------------------------------------------------------------------- */

osStatus_t osTimerStop(osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t status;

    /* Reject timer operations from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if(hTimer == NULL)
    {
        /* Reject an invalid timer identifier. */
        status = osErrorParameter;
    }
    else
    {
        /* Verify that the timer is currently active. */
        if(xTimerIsTimerActive(hTimer) == pdFALSE)
        {
            status = osErrorResource;
        }
        else
        {
            /* Submit a nonblocking timer stop command. */
            if(xTimerStop(hTimer, 0U) == pdPASS)
            {
                status = osOK;
            }
            else
            {
                /* Timer stop command submission failed. */
                status = osError;
            }
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Timer Running State                                                        */
/* -------------------------------------------------------------------------- */

uint32_t osTimerIsRunning(osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    uint32_t running     = 0U;

    /* Query timer state only from a valid task context. */
    if((IRQ_Context() == 0U) && (hTimer != NULL))
    {
        running = (uint32_t)xTimerIsTimerActive(hTimer);
    }

    return running;
}

/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* Timer Deletion                                                             */
/* -------------------------------------------------------------------------- */

osStatus_t osTimerDelete(osTimerId_t timer_id)
{
    TimerHandle_t timer = (TimerHandle_t)timer_id;
    TimerCallback_t* callback;
    TimerDeleteAck_t acknowledgment;

    /* Reject timer deletion from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the timer identifier. */
    if(timer == NULL)
    {
        return osErrorParameter;
    }

    /*
     * Blocking timer deletion must not execute from the
     * FreeRTOS timer daemon or a software timer callback.
     */
    if(xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle())
    {
        return osErrorResource;
    }

    /* Retrieve the callback context associated with the timer. */
    callback = (TimerCallback_t*)pvTimerGetTimerID(timer);

    if(callback == NULL)
    {
        return osErrorResource;
    }

    /* Initialize the deletion acknowledgment state. */
    atomic_init(&acknowledgment.done, 0U);

    /*
     * Queue the timer deletion request.
     * The timer daemon processes this command before
     * the acknowledgment command submitted afterward.
     */
    if(xTimerDelete(timer, portMAX_DELAY) != pdPASS)
    {
        return osErrorResource;
    }

    /* Queue an acknowledgment after the deletion command. */
    if(xTimerPendFunctionCall(cmsis_timer_delete_ack, &acknowledgment, 0U, portMAX_DELAY) != pdPASS)
    {
        /*
         * Deletion was queued but completion was not confirmed.
         * Preserve the callback metadata rather than freeing it.
         */
        return osErrorResource;
    }

    /* Wait until the timer daemon acknowledges deletion. */
    while(atomic_load_explicit(&acknowledgment.done, memory_order_acquire) == 0U)
    {
        vTaskDelay(1U);
    }

    /* Release only dynamically allocated callback metadata. */
    if(callback->dynamic_alloc != 0U)
    {
        vPortFree(callback);
    }

    return osOK;
}

#endif /* (configUSE_OS2_TIMER == 1) */

/* -------------------------------------------------------------------------- */
/* Event Flags Management Functions                                           */
/* -------------------------------------------------------------------------- */
/* Event Flags Creation                                                       */
/* -------------------------------------------------------------------------- */

osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t* attr)
{
    EventGroupHandle_t hEventGroup = NULL;
    int32_t mem;

    /* Reject unsupported CMSIS safety-class and MPU attributes. */
    if((attr != NULL) && (attr->attr_bits != 0U))
    {
        return NULL;
    }

    /* Event flag creation is supported only from task context. */
    if(IRQ_Context() == 0U)
    {
        mem = -1;

        /* Determine the requested event group allocation method. */
        if(attr != NULL)
        {
            if((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticEventGroup_t)))
            {
                /* Application-provided control block selects static allocation. */
                mem = 1;
            }
            else
            {
                if((attr->cb_mem == NULL) && (attr->cb_size == 0U))
                {
                    /* No control block provided: select dynamic allocation. */
                    mem = 0;
                }
            }
        }
        else
        {
            /* Default to dynamic allocation when attributes are absent. */
            mem = 0;
        }

        /* Create the native FreeRTOS event group. */
        if(mem == 1)
        {
#if (configSUPPORT_STATIC_ALLOCATION == 1)

            /* Create the event group using application-owned memory. */
            hEventGroup = xEventGroupCreateStatic((StaticEventGroup_t*)attr->cb_mem);

#endif
        }
        else if(mem == 0)
        {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

            /* Create the event group using FreeRTOS dynamic allocation. */
            hEventGroup = xEventGroupCreate();

#endif
        }
        else
        {
            /* Invalid allocation attributes: return a NULL handle. */
        }
    }

    /* Return the event group handle as a CMSIS event flags identifier. */
    return (osEventFlagsId_t)hEventGroup;
}

/* -------------------------------------------------------------------------- */
/* Event Flags Set                                                            */
/* -------------------------------------------------------------------------- */

uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
{
    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;
    uint32_t result_flags;
    BaseType_t yield;

    /* Validate the event group identifier and requested flag bits. */
    if((hEventGroup == NULL) || (flags == 0U) || ((flags & EVENT_FLAGS_INVALID_BITS) != 0U))
    {
        result_flags = (uint32_t)osErrorParameter;
    }
    else if(IRQ_Context() != 0U)
    {
#if (configUSE_OS2_EVENTFLAGS_FROM_ISR == 0)

        (void)yield;

        /*
         * Setting event flags from an ISR requires support
         * for the FreeRTOS timer daemon command mechanism.
         */
        result_flags = (uint32_t)osErrorResource;

#else

        yield = pdFALSE;

        /* Queue the event flag set operation from interrupt context. */
        if(xEventGroupSetBitsFromISR(hEventGroup, (EventBits_t)flags, &yield) == pdFAIL)
        {
            result_flags = (uint32_t)osErrorResource;
        }
        else
        {
            /*
             * Read currently set bits and include the bits
             * requested by this operation.
             */
            result_flags = (uint32_t)xEventGroupGetBitsFromISR(hEventGroup);

            result_flags |= flags;

            /* Request a context switch if the timer daemon was unblocked. */
            portYIELD_FROM_ISR(yield);
        }

#endif
    }
    else
    {
        /* Set the specified event flags from task context. */
        result_flags = (uint32_t)xEventGroupSetBits(hEventGroup, (EventBits_t)flags);
    }

    return result_flags;
}

/* -------------------------------------------------------------------------- */
/* Event Flags Clear                                                          */
/* -------------------------------------------------------------------------- */

uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)
{
    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;
    uint32_t result_flags;

    /* Validate the event group identifier and requested flag bits. */
    if((hEventGroup == NULL) || (flags == 0U) || ((flags & EVENT_FLAGS_INVALID_BITS) != 0U))
    {
        result_flags = (uint32_t)osErrorParameter;
    }
    else if(IRQ_Context() != 0U)
    {
#if (configUSE_OS2_EVENTFLAGS_FROM_ISR == 0)

        /* Event flag clearing from interrupt context is disabled. */
        result_flags = (uint32_t)osErrorResource;

#else

        /* Retrieve the event flags before requesting the clear operation. */
        result_flags = (uint32_t)xEventGroupGetBitsFromISR(hEventGroup);

        /* Queue the event flag clear operation from interrupt context. */
        if(xEventGroupClearBitsFromISR(hEventGroup, (EventBits_t)flags) == pdFAIL)
        {
            result_flags = (uint32_t)osErrorResource;
        }
        else
        {
            /*
             * The clear operation is deferred through the timer
             * daemon command queue.
             *
             * Request a yield to help the daemon process the
             * pending operation.
             */
            portYIELD_FROM_ISR(pdTRUE);
        }

#endif
    }
    else
    {
        /* Clear the specified bits and return their previous state. */
        result_flags = (uint32_t)xEventGroupClearBits(hEventGroup, (EventBits_t)flags);
    }

    return result_flags;
}

/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* Event Flags Get                                                            */
/* -------------------------------------------------------------------------- */

uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)
{
    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;
    uint32_t result_flags          = 0U;

    /* Retrieve event flags only when the identifier is valid. */
    if(hEventGroup != NULL)
    {
        if(IRQ_Context() != 0U)
        {
            /* Retrieve event flags from interrupt context. */
            result_flags = (uint32_t)xEventGroupGetBitsFromISR(hEventGroup);
        }
        else
        {
            /* Retrieve event flags from task context. */
            result_flags = (uint32_t)xEventGroupGetBits(hEventGroup);
        }
    }

    return result_flags;
}

/* -------------------------------------------------------------------------- */
/* Event Flags Wait                                                           */
/* -------------------------------------------------------------------------- */

uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout)
{
    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;

    BaseType_t wait_all;
    BaseType_t exit_clear;

    uint32_t result_flags;

    /* Validate supported event flag wait options. */
    if((options & ~(osFlagsWaitAll | osFlagsNoClear)) != 0U)
    {
        return osFlagsErrorParameter;
    }

    /* Validate the event group identifier and requested flags. */
    if((hEventGroup == NULL) || (flags == 0U) || ((flags & EVENT_FLAGS_INVALID_BITS) != 0U))
    {
        result_flags = (uint32_t)osErrorParameter;
    }
    else if(IRQ_Context() != 0U)
    {
        /* Event flag waiting is unsupported in interrupt context. */
        if(timeout == 0U)
        {
            /* Nonblocking ISR wait is unsupported. */
            result_flags = (uint32_t)osErrorISR;
        }
        else
        {
            /* Blocking ISR wait is invalid. */
            result_flags = (uint32_t)osFlagsErrorParameter;
        }
    }
    else
    {
        /* Determine whether all requested flags must be set. */
        if((options & osFlagsWaitAll) != 0U)
        {
            wait_all = pdTRUE;
        }
        else
        {
            wait_all = pdFALSE;
        }

        /* Determine whether matched flags are cleared on exit. */
        if((options & osFlagsNoClear) != 0U)
        {
            exit_clear = pdFALSE;
        }
        else
        {
            exit_clear = pdTRUE;
        }

        /* Wait for the requested FreeRTOS event group bits. */
        result_flags =
        (uint32_t)xEventGroupWaitBits(hEventGroup, (EventBits_t)flags,
                                      exit_clear, wait_all, (TickType_t)timeout);

        /* Validate the event flag wait condition. */
        if((options & osFlagsWaitAll) != 0U)
        {
            /* All requested flags must be present. */
            if((flags & result_flags) != flags)
            {
                if(timeout > 0U)
                {
                    result_flags = (uint32_t)osErrorTimeout;
                }
                else
                {
                    result_flags = (uint32_t)osErrorResource;
                }
            }
        }
        else
        {
            /* At least one requested flag must be present. */
            if((flags & result_flags) == 0U)
            {
                if(timeout > 0U)
                {
                    result_flags = (uint32_t)osErrorTimeout;
                }
                else
                {
                    result_flags = (uint32_t)osErrorResource;
                }
            }
        }
    }

    /* Return the flags observed before optional clearing. */
    return result_flags;
}

/* -------------------------------------------------------------------------- */
/* Event Flags Delete                                                         */
/* -------------------------------------------------------------------------- */

osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)
{
    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;
    osStatus_t status;

#ifndef USE_FreeRTOS_HEAP_1

    /* Reject event group deletion from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if(hEventGroup == NULL)
    {
        /* Reject an invalid event group identifier. */
        status = osErrorParameter;
    }
    else
    {
        /* Delete the native FreeRTOS event group. */
        vEventGroupDelete(hEventGroup);
        status = osOK;
    }

#else

    /* Event group deletion is unsupported in this configuration. */
    (void)hEventGroup;
    status = osError;

#endif

    return status;
}

/* -------------------------------------------------------------------------- */
/* Mutex Management Functions                                                 */
/* -------------------------------------------------------------------------- */

#if (configUSE_OS2_MUTEX == 1)

/* -------------------------------------------------------------------------- */
/* Mutex Creation                                                             */
/* -------------------------------------------------------------------------- */

osMutexId_t osMutexNew(const osMutexAttr_t* attr)
{
    MutexMeta_t* meta        = NULL;
    SemaphoreHandle_t handle = NULL;

    bool recursive    = false;
    bool static_alloc = false;

    const char* name = NULL;

    /* Mutex creation is not permitted from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return NULL;
    }

    /* Process optional application-provided mutex attributes. */
    if(attr != NULL)
    {
        /*
         * Only recursive and priority inheritance attributes
         * are supported by this compatibility implementation.
         *
         * FreeRTOS mutexes provide priority inheritance by default.
         */
        if((attr->attr_bits & ~(osMutexRecursive | osMutexPrioInherit)) != 0U)
        {
            return NULL;
        }

        /* Determine whether a recursive mutex is requested. */
        recursive = ((attr->attr_bits & osMutexRecursive) != 0U);

        /* Preserve the application-provided mutex name. */
        name = attr->name;

        /* Check whether static mutex allocation is requested. */
        if((attr->cb_mem != NULL) || (attr->cb_size != 0U))
        {
            /*
             * Validate the application-provided memory capacity
             * and alignment requirements.
             */
            if((attr->cb_mem == NULL) || (attr->cb_size < sizeof(StaticMutexMeta_t)) ||
               (((uintptr_t)attr->cb_mem % sizeof(void*)) != 0U))
            {
                return NULL;
            }

            /* Select the static mutex allocation method. */
            static_alloc = true;
        }
    }

    /* Create the mutex using the selected allocation method. */
    if(static_alloc)
    {
    #if (configSUPPORT_STATIC_ALLOCATION == 1)

        StaticMutexMeta_t* static_meta = (StaticMutexMeta_t*)attr->cb_mem;

        /* Use application-owned metadata storage. */
        meta = &static_meta->meta;

        #if (configUSE_RECURSIVE_MUTEXES == 1)

        if(recursive)
        {
            /* Create a statically allocated recursive mutex. */
            handle = xSemaphoreCreateRecursiveMutexStatic(&static_meta->cb);
        }
        else

        #endif
        if(!recursive)
        {
            /* Create a statically allocated standard mutex. */
            handle = xSemaphoreCreateMutexStatic(&static_meta->cb);
        }

    #endif
    }
    else
    {
    #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

        /* Allocate metadata for the new mutex. */
        meta = (MutexMeta_t*)pvPortMalloc(sizeof(*meta));

        if(meta != NULL)
        {
        #if (configUSE_RECURSIVE_MUTEXES == 1)

            if(recursive)
            {
                /* Create a dynamically allocated recursive mutex. */
                handle = xSemaphoreCreateRecursiveMutex();
            }
            else

        #endif
            if(!recursive)
            {
                /* Create a dynamically allocated standard mutex. */
                handle = xSemaphoreCreateMutex();
            }
        }

    #endif
    }

    /* Release dynamically allocated metadata if creation failed. */
    if(handle == NULL)
    {
        if((meta != NULL) && (!static_alloc))
        {
            vPortFree(meta);
        }

        return NULL;
    }

    /* Initialize the mutex metadata. */
    meta->handle        = handle;
    meta->recursive     = recursive ? 1U : 0U;
    meta->dynamic_alloc = static_alloc ? 0U : 1U;
    meta->name          = name;

    #if (configQUEUE_REGISTRY_SIZE > 0)

    /* Register the mutex name when the queue registry is enabled. */
    if(name != NULL)
    {
        vQueueAddToRegistry(handle, name);
    }

    #endif

    /* Return the mutex metadata as a CMSIS mutex identifier. */
    return (osMutexId_t)meta;
}

/* -------------------------------------------------------------------------- */
/* Mutex Acquire                                                              */
/* -------------------------------------------------------------------------- */

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    const MutexMeta_t* meta = (const MutexMeta_t*)mutex_id;
    BaseType_t result;

    /* Reject mutex acquisition from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the mutex identifier and native handle. */
    if((meta == NULL) || (meta->handle == NULL))
    {
        return osErrorParameter;
    }

    /* Select the appropriate native mutex acquisition API. */
    if(meta->recursive != 0U)
    {
    #if (configUSE_RECURSIVE_MUTEXES == 1)

        /* Acquire the recursive FreeRTOS mutex. */
        result = xSemaphoreTakeRecursive(meta->handle, (TickType_t)timeout);

    #else

        /* Recursive mutex support is disabled. */
        return osErrorResource;

    #endif
    }
    else
    {
        /* Acquire the standard FreeRTOS mutex. */
        result = xSemaphoreTake(meta->handle, (TickType_t)timeout);
    }

    /* Report successful mutex acquisition. */
    if(result == pdPASS)
    {
        return osOK;
    }

    /* Distinguish immediate failure from a timed-out wait. */
    if(timeout == 0U)
    {
        return osErrorResource;
    }

    return osErrorTimeout;
}

/* -------------------------------------------------------------------------- */
/* Mutex Release                                                              */
/* -------------------------------------------------------------------------- */

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    const MutexMeta_t* meta = (const MutexMeta_t*)mutex_id;
    BaseType_t result;

    /* Reject mutex release from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the mutex identifier and native handle. */
    if((meta == NULL) || (meta->handle == NULL))
    {
        return osErrorParameter;
    }

    /* Select the appropriate native mutex release API. */
    if(meta->recursive != 0U)
    {
    #if (configUSE_RECURSIVE_MUTEXES == 1)

        /* Release the recursive FreeRTOS mutex. */
        result = xSemaphoreGiveRecursive(meta->handle);

    #else

        /* Recursive mutex support is disabled. */
        return osErrorResource;

    #endif
    }
    else
    {
        /* Release the standard FreeRTOS mutex. */
        result = xSemaphoreGive(meta->handle);
    }

    /* Return the result of the native mutex release operation. */
    if(result == pdPASS)
    {
        return osOK;
    }

    return osErrorResource;
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Mutex Owner Retrieval                                                      */
/* -------------------------------------------------------------------------- */

osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
    const MutexMeta_t* meta = (const MutexMeta_t*)mutex_id;

    /* Validate the execution context and mutex metadata. */
    if((IRQ_Context() != 0U) || (meta == NULL) || (meta->handle == NULL))
    {
        return NULL;
    }

    /* Retrieve the thread currently holding the mutex. */
    return (osThreadId_t)xSemaphoreGetMutexHolder(meta->handle);
}

/* -------------------------------------------------------------------------- */
/* Mutex Deletion                                                             */
/* -------------------------------------------------------------------------- */

osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
    MutexMeta_t* meta = (MutexMeta_t*)mutex_id;

    /* Reject mutex deletion from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the mutex metadata and native handle. */
    if((meta == NULL) || (meta->handle == NULL))
    {
        return osErrorParameter;
    }

    /*
     * The caller must ensure that no task currently owns
     * the mutex or is waiting to acquire it.
     */

    #if (configQUEUE_REGISTRY_SIZE > 0)

    /* Remove the mutex from the FreeRTOS queue registry. */
    vQueueUnregisterQueue(meta->handle);

    #endif

    /* Delete the native FreeRTOS mutex. */
    vSemaphoreDelete(meta->handle);

    /* Invalidate the stored native mutex handle. */
    meta->handle = NULL;

    /* Release dynamically allocated mutex metadata. */
    if(meta->dynamic_alloc != 0U)
    {
        vPortFree(meta);
    }

    return osOK;
}

#endif /* (configUSE_OS2_MUTEX == 1) */

/* -------------------------------------------------------------------------- */
/* Mutex Name Retrieval                                                       */
/* -------------------------------------------------------------------------- */

const char* osMutexGetName(osMutexId_t mutex_id)
{
    const MutexMeta_t* meta = (const MutexMeta_t*)mutex_id;
    const char* name        = NULL;

    /* Retrieve the name from the CMSIS mutex metadata. */
    if(meta != NULL)
    {
        name = meta->name;
    }

    return name;
}

/* -------------------------------------------------------------------------- */
/* Semaphore Management Functions                                             */
/* -------------------------------------------------------------------------- */
/* Semaphore Creation                                                         */
/* -------------------------------------------------------------------------- */

osSemaphoreId_t
osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t* attr)
{
    SemaphoreHandle_t hSemaphore = NULL;
    int32_t mem                  = -1;

    /* Reject unsupported CMSIS safety-class and MPU attributes. */
    if((attr != NULL) && (attr->attr_bits != 0U))
    {
        return NULL;
    }

    /* Validate execution context and semaphore count parameters. */
    if((IRQ_Context() != 0U) || (max_count == 0U) || (initial_count > max_count))
    {
        return NULL;
    }

    /* Determine the semaphore allocation method. */
    if(attr != NULL)
    {
        if((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticSemaphore_t)))
        {
            /* Select application-provided static control block memory. */
            mem = 1;
        }
        else if((attr->cb_mem == NULL) && (attr->cb_size == 0U))
        {
            /* Select FreeRTOS dynamic allocation. */
            mem = 0;
        }
        else
        {
            /* Invalid static allocation attributes. */
        }
    }
    else
    {
        /* Default to dynamic allocation when attributes are absent. */
        mem = 0;
    }

    /* Continue only when a valid allocation method was selected. */
    if(mem != -1)
    {
        if(max_count == 1U)
        {
            /* Create a binary semaphore. */
            if(mem == 1)
            {
#if (configSUPPORT_STATIC_ALLOCATION == 1)

                hSemaphore =
                xSemaphoreCreateBinaryStatic((StaticSemaphore_t*)attr->cb_mem);

#endif
            }
            else
            {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

                hSemaphore = xSemaphoreCreateBinary();

#endif
            }

            /* Set the initial binary semaphore token count. */
            if((hSemaphore != NULL) && (initial_count != 0U))
            {
                if(xSemaphoreGive(hSemaphore) != pdPASS)
                {
                    /* Release the semaphore if initialization fails. */
                    vSemaphoreDelete(hSemaphore);
                    hSemaphore = NULL;
                }
            }
        }
        else
        {
            /* Create a counting semaphore. */
            if(mem == 1)
            {
#if (configSUPPORT_STATIC_ALLOCATION == 1)

                hSemaphore =
                xSemaphoreCreateCountingStatic((UBaseType_t)max_count,
                                               (UBaseType_t)initial_count,
                                               (StaticSemaphore_t*)attr->cb_mem);

#endif
            }
            else
            {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

                hSemaphore = xSemaphoreCreateCounting((UBaseType_t)max_count,
                                                      (UBaseType_t)initial_count);

#endif
            }
        }

#if (configQUEUE_REGISTRY_SIZE > 0)

        /* Register the semaphore name when configured. */
        if((hSemaphore != NULL) && (attr != NULL) && (attr->name != NULL))
        {
            vQueueAddToRegistry(hSemaphore, attr->name);
        }

#endif
    }

    /* Return the native semaphore as a CMSIS identifier. */
    return (osSemaphoreId_t)hSemaphore;
}

/* -------------------------------------------------------------------------- */
/* Semaphore Acquisition                                                      */
/* -------------------------------------------------------------------------- */

osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;

    osStatus_t status = osOK;
    BaseType_t yield;

    /* Validate the semaphore identifier. */
    if(hSemaphore == NULL)
    {
        status = osErrorParameter;
    }
    else if(IRQ_Context() != 0U)
    {
        /* Blocking semaphore acquisition is not permitted from ISR. */
        if(timeout != 0U)
        {
            status = osErrorParameter;
        }
        else
        {
            yield = pdFALSE;

            /* Attempt to acquire a semaphore token from ISR. */
            if(xSemaphoreTakeFromISR(hSemaphore, &yield) != pdPASS)
            {
                status = osErrorResource;
            }
            else
            {
                /* Request a context switch when required. */
                portYIELD_FROM_ISR(yield);
            }
        }
    }
    else
    {
        /* Acquire the semaphore using the specified timeout. */
        if(xSemaphoreTake(hSemaphore, (TickType_t)timeout) != pdPASS)
        {
            if(timeout != 0U)
            {
                status = osErrorTimeout;
            }
            else
            {
                status = osErrorResource;
            }
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Semaphore Release                                                          */
/* -------------------------------------------------------------------------- */

osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;

    osStatus_t status = osOK;
    BaseType_t yield;

    /* Validate the semaphore identifier. */
    if(hSemaphore == NULL)
    {
        status = osErrorParameter;
    }
    else if(IRQ_Context() != 0U)
    {
        yield = pdFALSE;

        /* Release a semaphore token from interrupt context. */
        if(xSemaphoreGiveFromISR(hSemaphore, &yield) != pdTRUE)
        {
            status = osErrorResource;
        }
        else
        {
            /* Request a context switch when required. */
            portYIELD_FROM_ISR(yield);
        }
    }
    else
    {
        /* Release the semaphore token from task context. */
        if(xSemaphoreGive(hSemaphore) != pdPASS)
        {
            status = osErrorResource;
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Semaphore Token Count                                                      */
/* -------------------------------------------------------------------------- */

uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;

    uint32_t count = 0U;

    /* Retrieve the token count only for a valid semaphore. */
    if(hSemaphore != NULL)
    {
        if(IRQ_Context() != 0U)
        {
            /* Retrieve the semaphore count from interrupt context. */
            count = (uint32_t)uxSemaphoreGetCountFromISR(hSemaphore);
        }
        else
        {
            /* Retrieve the semaphore count from task context. */
            count = (uint32_t)uxSemaphoreGetCount(hSemaphore);
        }
    }

    return count;
}

/* -------------------------------------------------------------------------- */
/* Semaphore Deletion                                                         */
/* -------------------------------------------------------------------------- */

osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;

    osStatus_t status;

#ifndef USE_FreeRTOS_HEAP_1

    /* Reject semaphore deletion from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        status = osErrorISR;
    }
    else if(hSemaphore == NULL)
    {
        /* Reject an invalid semaphore identifier. */
        status = osErrorParameter;
    }
    else
    {
    #if (configQUEUE_REGISTRY_SIZE > 0)

        /* Remove the semaphore from the FreeRTOS queue registry. */
        vQueueUnregisterQueue(hSemaphore);

    #endif

        /* Delete the native FreeRTOS semaphore. */
        vSemaphoreDelete(hSemaphore);

        status = osOK;
    }

#else

    /* Semaphore deletion is unsupported in this configuration. */
    (void)hSemaphore;
    status = osError;

#endif

    return status;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Management Functions                                         */
/* -------------------------------------------------------------------------- */
/* Message Queue Creation                                                     */
/* -------------------------------------------------------------------------- */

osMessageQueueId_t
osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t* attr)
{
    MessageQueueMeta_t* meta = NULL;
    QueueHandle_t handle     = NULL;

    bool static_alloc = false;
    const char* name  = NULL;

    /* Reject unsupported CMSIS safety-class and MPU attributes. */
    if((attr != NULL) && (attr->attr_bits != 0U))
    {
        return NULL;
    }

    /* Validate execution context, dimensions, and total size. */
    if((IRQ_Context() != 0U) || (msg_count == 0U) || (msg_size == 0U) ||
       (msg_count > (UINT32_MAX / msg_size)))
    {
        return NULL;
    }

    /* Process optional message queue attributes. */
    if(attr != NULL)
    {
        name = attr->name;

        /* Determine whether static queue allocation is requested. */
        if((attr->cb_mem != NULL) || (attr->cb_size != 0U) ||
           (attr->mq_mem != NULL) || (attr->mq_size != 0U))
        {
            /*
             * Validate the static control block, message storage,
             * total capacity, and control block alignment.
             */
            if((attr->cb_mem == NULL) || (attr->cb_size < sizeof(StaticMessageQueueMeta_t)) ||
               (attr->mq_mem == NULL) || (attr->mq_size < (msg_count * msg_size)) ||
               (((uintptr_t)attr->cb_mem % sizeof(void*)) != 0U))
            {
                return NULL;
            }

            static_alloc = true;
        }
    }

    /* Create the queue using the selected allocation method. */
    if(static_alloc)
    {
#if (configSUPPORT_STATIC_ALLOCATION == 1)

        StaticMessageQueueMeta_t* static_meta = (StaticMessageQueueMeta_t*)attr->cb_mem;

        /* Use application-owned queue metadata storage. */
        meta = &static_meta->meta;

        /* Create the statically allocated FreeRTOS queue. */
        handle = xQueueCreateStatic((UBaseType_t)msg_count, (UBaseType_t)msg_size,
                                    (uint8_t*)attr->mq_mem, &static_meta->cb);

#endif
    }
    else
    {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)

        /* Allocate CMSIS message queue metadata. */
        meta = (MessageQueueMeta_t*)pvPortMalloc(sizeof(*meta));

        if(meta != NULL)
        {
            /* Create a dynamically allocated FreeRTOS queue. */
            handle = xQueueCreate((UBaseType_t)msg_count, (UBaseType_t)msg_size);
        }

#endif
    }

    /* Release dynamically allocated metadata if creation failed. */
    if(handle == NULL)
    {
        if((meta != NULL) && (!static_alloc))
        {
            vPortFree(meta);
        }

        return NULL;
    }

    /* Initialize the CMSIS message queue metadata. */
    meta->handle       = handle;
    meta->msg_count    = msg_count;
    meta->msg_size     = msg_size;
    meta->name         = name;
    meta->meta_dynamic = static_alloc ? 0U : 1U;

#if (configQUEUE_REGISTRY_SIZE > 0)

    /* Register the queue name when registry support is enabled. */
    if(name != NULL)
    {
        vQueueAddToRegistry(handle, name);
    }

#endif

    /* Return the metadata as the CMSIS message queue identifier. */
    return (osMessageQueueId_t)meta;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Put                                                          */
/* -------------------------------------------------------------------------- */

osStatus_t
osMessageQueuePut(osMessageQueueId_t mq_id, const void* msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;

    QueueHandle_t hQueue = (mq != NULL) ? mq->handle : NULL;

    osStatus_t status = osOK;
    BaseType_t yield;

    /*
     * FreeRTOS queues use FIFO ordering.
     * Nonzero CMSIS message priorities are unsupported.
     */
    if(msg_prio != 0U)
    {
        return osErrorParameter;
    }

    /* Select the message queue API based on execution context. */
    if(IRQ_Context() != 0U)
    {
        /* Validate the queue, message pointer, and ISR timeout. */
        if((hQueue == NULL) || (msg_ptr == NULL) || (timeout != 0U))
        {
            status = osErrorParameter;
        }
        else
        {
            yield = pdFALSE;

            /* Submit the message from interrupt context. */
            if(xQueueSendToBackFromISR(hQueue, msg_ptr, &yield) != pdTRUE)
            {
                status = osErrorResource;
            }
            else
            {
                /* Request a context switch when required. */
                portYIELD_FROM_ISR(yield);
            }
        }
    }
    else
    {
        /* Validate the queue and message pointer. */
        if((hQueue == NULL) || (msg_ptr == NULL))
        {
            status = osErrorParameter;
        }
        else
        {
            /* Submit the message using the requested timeout. */
            if(xQueueSendToBack(hQueue, msg_ptr, (TickType_t)timeout) != pdPASS)
            {
                if(timeout != 0U)
                {
                    status = osErrorTimeout;
                }
                else
                {
                    status = osErrorResource;
                }
            }
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Get                                                          */
/* -------------------------------------------------------------------------- */

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void* msg_ptr, uint8_t* msg_prio, uint32_t timeout)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;

    QueueHandle_t hQueue = (mq != NULL) ? mq->handle : NULL;

    osStatus_t status = osOK;
    BaseType_t yield;

    /* Native FreeRTOS queues do not provide message priorities. */
    if(msg_prio != NULL)
    {
        *msg_prio = 0U;
    }

    /* Select the queue receive API based on execution context. */
    if(IRQ_Context() != 0U)
    {
        /* Validate the queue, destination, and ISR timeout. */
        if((hQueue == NULL) || (msg_ptr == NULL) || (timeout != 0U))
        {
            status = osErrorParameter;
        }
        else
        {
            yield = pdFALSE;

            /* Retrieve a queued message from interrupt context. */
            if(xQueueReceiveFromISR(hQueue, msg_ptr, &yield) != pdPASS)
            {
                status = osErrorResource;
            }
            else
            {
                /* Request a context switch when required. */
                portYIELD_FROM_ISR(yield);
            }
        }
    }
    else
    {
        /* Validate the queue and destination buffer. */
        if((hQueue == NULL) || (msg_ptr == NULL))
        {
            status = osErrorParameter;
        }
        else
        {
            /* Receive the next queued message. */
            if(xQueueReceive(hQueue, msg_ptr, (TickType_t)timeout) != pdPASS)
            {
                if(timeout != 0U)
                {
                    status = osErrorTimeout;
                }
                else
                {
                    status = osErrorResource;
                }
            }
        }
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Capacity                                                     */
/* -------------------------------------------------------------------------- */

uint32_t osMessageQueueGetCapacity(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;
    uint32_t capacity      = 0U;

    /* Retrieve the configured maximum number of queue messages. */
    if(mq != NULL)
    {
        capacity = mq->msg_count;
    }

    return capacity;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Message Size                                                 */
/* -------------------------------------------------------------------------- */

uint32_t osMessageQueueGetMsgSize(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;
    uint32_t msg_size      = 0U;

    /* Retrieve the configured message size in bytes. */
    if(mq != NULL)
    {
        msg_size = mq->msg_size;
    }

    return msg_size;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Message Count                                                */
/* -------------------------------------------------------------------------- */

uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;

    QueueHandle_t hQueue = (mq != NULL) ? mq->handle : NULL;

    UBaseType_t count = 0U;

    /* Retrieve the queued message count for a valid queue. */
    if(hQueue != NULL)
    {
        if(IRQ_Context() != 0U)
        {
            /* Retrieve the message count from interrupt context. */
            count = uxQueueMessagesWaitingFromISR(hQueue);
        }
        else
        {
            /* Retrieve the message count from task context. */
            count = uxQueueMessagesWaiting(hQueue);
        }
    }

    return (uint32_t)count;
}

/* -------------------------------------------------------------------------- */
/* Message Queue Available Space                                              */
/* -------------------------------------------------------------------------- */

uint32_t osMessageQueueGetSpace(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;
    QueueHandle_t hQueue;
    UBaseType_t count;
    uint32_t available = 0U;

    /* Validate the CMSIS message queue metadata. */
    if(mq == NULL)
    {
        return 0U;
    }

    /* Retrieve the native FreeRTOS queue handle. */
    hQueue = mq->handle;

    /* Select the message count API based on execution context. */
    if(IRQ_Context() != 0U)
    {
        count = uxQueueMessagesWaitingFromISR(hQueue);
    }
    else
    {
        count = uxQueueMessagesWaiting(hQueue);
    }

    /* Calculate the remaining message slots without underflow. */
    if(mq->msg_count >= (uint32_t)count)
    {
        available = mq->msg_count - (uint32_t)count;
    }

    return available;
}


/* -------------------------------------------------------------------------- */
/* Message Queue Reset                                                        */
/* -------------------------------------------------------------------------- */

osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;

    /* Queue reset is not permitted from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the queue metadata and native queue handle. */
    if((mq == NULL) || (mq->handle == NULL))
    {
        return osErrorParameter;
    }

    /* Discard all pending messages from the queue. */
    if(xQueueReset(mq->handle) != pdPASS)
    {
        return osErrorResource;
    }

    return osOK;
}


/* -------------------------------------------------------------------------- */
/* Message Queue Deletion                                                     */
/* -------------------------------------------------------------------------- */

osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
{
    MessageQueueMeta_t* mq = (MessageQueueMeta_t*)mq_id;

    /* Queue deletion is not permitted from interrupt context. */
    if(IRQ_Context() != 0U)
    {
        return osErrorISR;
    }

    /* Validate the queue metadata and native queue handle. */
    if((mq == NULL) || (mq->handle == NULL))
    {
        return osErrorParameter;
    }

    /*
     * The caller must ensure that no task is accessing
     * the queue or waiting for a queue operation.
     */

#if (configQUEUE_REGISTRY_SIZE > 0)

    /* Remove the queue from the FreeRTOS queue registry. */
    vQueueUnregisterQueue(mq->handle);

#endif

    /* Delete the native FreeRTOS queue. */
    vQueueDelete(mq->handle);

    /* Invalidate the stored native queue handle. */
    mq->handle = NULL;

    /* Release dynamically allocated queue metadata. */
    if(mq->meta_dynamic != 0U)
    {
        vPortFree(mq);
    }

    return osOK;
}

/* -------------------------------------------------------------------------- */
