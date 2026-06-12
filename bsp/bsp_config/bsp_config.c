
#include "bsp_config.h"
#include "bsp_os_map.h"
#include "bsp_core.h"

// clang-format off
/*
 * ------------------------------------------------------------------------
 */
os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX] = {
    [BSP_THREAD_OWNER_LOGGER] = {
        .attr = {
            .name       = "Logger",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    },

    [BSP_THREAD_OWNER_UART_ASYNC] = {
        .attr = {
            .name       = "UARTAsync",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    },
};

/*
 * ------------------------------------------------------------------------
 */

os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX] = {
        
    [BSP_WIFI_EVENT_QUEUE] = {
            .msg_count      = 10U,
            .msg_size_bytes = 0U, /* Run time analysis */
            .attr = {
                .name = "bspWifiEventQ"
            }
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
        }
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
        }
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
        }
    },

    [BSP_SEMAPHORE_OWNER_WIFI] = {
        .max_count     = 10U,
        .initial_count = 0U,
        .attr = {
            .name = "bspWifiSem"
        }
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