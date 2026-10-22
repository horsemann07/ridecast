
#include "bsp_config.h"
#include "bsp_core.h"

#include "bsp_log.h"
#include "bsp_wifi.h"

#if defined(CMAKE_ESP_PLATFORM) && (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)
extern void bspUartEventThread(void* arg);
#endif /* CMAKE_ESP_PLATFORM */


       // clang-format off
/*
 * ------------------------------------------------------------------------
 */
os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX] = {
#if defined(CMAKE_ESP_PLATFORM) && (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)
    [BSP_THREAD_OWNER_UART_ASYNC] = {

        .func = bspUartEventThread,

        .func = NULL,

        .arg  = NULL,
        .attr = {
            .name       = "UARTAsync",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },
#endif /* CMAKE_ESP_PLATFORM */
};

/*
 * ------------------------------------------------------------------------
 */

os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX] = {
        
    [BSP_QUEUE_OWNER_LOG] = {
        .msg_count      = bspCONFIG_LOG_ISR_QUEUE_LENGTH,
        .msg_size_bytes = sizeof(bspLogIsrRecord_t),
        .attr = {
            .name = "bspLogQ"
        },
        .handle = NULL
    },
    [BSP_QUEUE_WIFI_EVENT] = {
            .msg_count      = 10U,
            .msg_size_bytes = sizeof(bspWifiContext_t),
            .attr = {
                .name = "bspWifiEventQ"
            },
            .handle = NULL
        }
};


/*
 * ------------------------------------------------------------------------
 */

os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX] = {

    [BSP_MUTEX_OWNER_LOG] = {
        .attr = {
            .name      = "bspLogMtx",
            .attr_bits = osMutexPrioInherit
        },
        .handle = NULL
    },

    [BSP_MUTEX_OWNER_WIFI] = {
        .attr = {
            .name      = "bspWifiMtx",
            .attr_bits = osMutexPrioInherit
        }
    },

    [BSP_MUTEX_OWNER_UART_ASYNC] = {
        .attr = {
            .name      = "bspUartAsyncMtx",
            .attr_bits = osMutexPrioInherit
        },
        .handle = NULL
    }
};

/*
 * ------------------------------------------------------------------------
 */

os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX] = {

    [BSP_SEMAPHORE_OWNER_LOG] = {
        .max_count     = 10U,
        .initial_count = 0U,
        .attr = {
            .name = "bspLogSem"
        },
        .handle = NULL
    },

    [BSP_SEMAPHORE_OWNER_WIFI] = {
        .max_count     = 10U,
        .initial_count = 0U,
        .attr = {
            .name = "bspWifiSem"
        },
        .handle = NULL
    }
};

/*
 * ------------------------------------------------------------------------
 */

os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX] = {

    [BSP_EVENT_FLAGS_OWNER_LOG] = {
        .flags = 0U,
        .attr = {
            .name = "bspLogEventFlags"
        }
    },

    [BSP_EVENT_FLAGS_OWNER_WIFI] = {
        .flags = 0U,
        .attr = {
            .name = "bspWifiEventFlags"
        }
    }
};


/**********************************************************************
 * FILE END
 ***********************************************************************/