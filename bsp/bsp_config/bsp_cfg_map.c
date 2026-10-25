
/**
 * ============================================================================
 * @file    bsp_cfg_map.c
 * @brief   BSP RTOS object configuration maps.
 *
 * @details
 * Provides the BSP-owned CMSIS-RTOS2 object configurations for threads,
 * message queues, mutexes, semaphores and event flags.
 *
 * Object creation is performed by the system initialization code.
 * This module supplies the attributes and stores the runtime handles.
 *
 * Configuration:
 *   - Thread and object owner IDs are defined in bsp_cfg_map.h.
 *   - Configurable defaults are defined in bsp_config.h.
 *   - Applications may override supported config* macros.
 *
 * @note
 *   A thread or synchronization object must only be created when its
 *   corresponding map entry is configured for the target platform.
 *
 * @warning
 *   Do not create an RTOS object from a reserved or unconfigured entry.
 *   Runtime handles must be checked after object creation.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Updated configuration names and corrected BSP object maps.
 * ============================================================================
 */

#include "bsp_config.h"
#include "bsp_defs.h"

#include "bsp_log.h"
#include "bsp_wifi.h"


// clang-format off
/* =========================================================================
 * BSP THREAD CONFIGURATION
 * ========================================================================= */

os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX];

/* =========================================================================
 * BSP QUEUE CONFIGURATION
 * ========================================================================= */

os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX] =
{
    [BSP_QUEUE_OWNER_LOG_TASK] =
    {
        .msg_count      = configLOG_TASK_QUEUE_LENGTH,
        .msg_size_bytes = sizeof(log_text_record_t),
        .attr =
        {
            .name = "bspLogTaskQ"
        },
        .handle = NULL
    },

    [BSP_QUEUE_OWNER_LOG_ISR] =
    {
        .msg_count      = configLOG_ISR_QUEUE_LENGTH,
        .msg_size_bytes = sizeof(log_isr_record_t),
        .attr =
        {
            .name = "bspLogIsrQ"
        },
        .handle = NULL
    },

    [BSP_QUEUE_WIFI_EVENT] =
    {
        .msg_count      = configWIFI_EVENT_QUEUE_DEPTH,
        .msg_size_bytes = sizeof(bspWifiContext_t),
        .attr =
        {
            .name = "bspWifiEventQ"
        },
        .handle = NULL
    }
};

/* =========================================================================
 * BSP MUTEX CONFIGURATION
 * ========================================================================= */

os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX] = {
    [BSP_MUTEX_OWNER_LOG] = { .attr = { .name = "bspLogMtx", .attr_bits = osMutexPrioInherit },
                              .handle = NULL },

    [BSP_MUTEX_OWNER_CLI] = { .attr = { .name = "bspCliMtx", .attr_bits = osMutexPrioInherit },
                              .handle = NULL },

    [BSP_MUTEX_OWNER_WIFI] = { .attr = { .name = "bspWifiMtx", .attr_bits = osMutexPrioInherit },
                               .handle = NULL },

#if defined(CMAKE_ESPx_PLATFORM) && (configUART_ENABLE_ASYNC == 1U)

    [BSP_MUTEX_OWNER_UART_ASYNC] = { .attr = { .name = "bspUartAsyncMtx", .attr_bits = osMutexPrioInherit },
                                     .handle = NULL }

#endif
};


/* =========================================================================
 * BSP SEMAPHORE CONFIGURATION
 * ========================================================================= */

os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX] = {
    [BSP_SEMAPHORE_OWNER_LOG] = { .max_count     = 10U,
                                  .initial_count = 0U,
                                  .attr          = { .name = "bspLogSem" },
                                  .handle        = NULL },

    [BSP_SEMAPHORE_OWNER_WIFI] = { .max_count     = 10U,
                                   .initial_count = 0U,
                                   .attr          = { .name = "bspWifiSem" },
                                   .handle        = NULL }
};


/* =========================================================================
 * BSP EVENT FLAGS CONFIGURATION
 * ========================================================================= */

os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX] = {
    [BSP_EVENT_FLAGS_OWNER_LOG] = { .flags  = 0U,
                                    .attr   = { .name = "bspLogEventFlags" },
                                    .handle = NULL },

    [BSP_EVENT_FLAGS_OWNER_WIFI] = { .flags = 0U, .attr = { .name = "bspWifiEventFlags" }, .handle = NULL }
};
