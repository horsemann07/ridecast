
/**
 * ============================================================================
 * @file    rc_os_map.c
 * @brief   RideCast RTOS and UART resource configuration maps.
 *
 * @details
 * Defines application-owned thread, queue, mutex, semaphore,
 * event-flag and UART configurations.
 *
 * BSP resources are configured separately in bsp_cfg_map.c.
 *
 * Application RTOS owner IDs begin at their corresponding
 * BSP_*_OWNER_MAX values.
 *
 * Object creation is performed by the application startup code.
 * This file provides configuration and runtime handle storage.
 *
 * @warning
 *   Object creation must skip BSP-reserved indices.
 *   Thread functions and queue message types must match their
 *   actual application implementations.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Aligned application maps and BSP UART handle configuration.
 * 2026-05-23  Initial development.
 * ============================================================================
 */

#include "app_config.h"
#include "app_tasklist.h"
#include "rc_os_map.h"

#include "bsp_os_map.h"
#include "bsp_defs.h"

#include "uart_app.h"
#include "wifi_app.h"
#include "conn_manager.h"

// clang-format off
/* =========================================================================
 * THREAD CONFIGURATION
 * ========================================================================= */

/* =========================================================================
 * THREAD CONFIGURATION
 * ========================================================================= */

os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX] =
{
    [APP_THREAD_OWNER_UART_APP_TX] =
    {
        .init   = NULL,
        .thread = vAppUartTxTask,
        .deinit = NULL,
        .arg    = NULL,

        .attr =
        {
            .name       = "UartAppTx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },

        .handle = NULL
    },

    [APP_THREAD_OWNER_UART_APP_RX] =
    {
        .init   = NULL,
        .thread = vAppUartRxTask,
        .deinit = NULL,
        .arg    = NULL,

        .attr =
        {
            .name       = "UartAppRx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },

        .handle = NULL
    },

    [APP_THREAD_OWNER_CONN_MANAGER] =
    {
        .init   = conn_manager_init,
        .thread = vConnManagerTask,
        .deinit = conn_manager_deinit,
        .arg    = NULL,

        .attr =
        {
            .name       = "ConnManager",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },

        .handle = NULL
    },

    [APP_THREAD_OWNER_RC_WIFI] =
    {
        .init   = NULL,
        .thread = vRcWifiTask,
        .deinit = NULL,
        .arg    = NULL,

        .attr =
        {
            .name       = "RcWifi",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },

        .handle = NULL
    },

    // [APP_THREAD_OWNER_RC_BLE] =
    // {
    //     .init   = NULL,
    //     .thread = vRcBleTask,
    //     .deinit = NULL,
    //     .arg    = NULL,

    //     .attr =
    //     {
    //         .name       = "RcBle",
    //         .stack_size = THREAD_STACK_3K,
    //         .priority   = osPriorityNormal
    //     },

    //     .handle = NULL
    // },

    // [APP_THREAD_OWNER_RC_BT_CLASSIC] =
    // {
    //     .init   = NULL,
    //     .thread = vRcBtClassicTask,
    //     .deinit = NULL,
    //     .arg    = NULL,

    //     .attr =
    //     {
    //         .name       = "RcBtClassic",
    //         .stack_size = THREAD_STACK_3K,
    //         .priority   = osPriorityNormal
    //     },

    //     .handle = NULL
    // }
};

/* =========================================================================
 * QUEUE CONFIGURATION
 * ========================================================================= */

os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX] =
{
    [APP_Q_OWNER_RC_UART_RX] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(uart_app_msg_t),
        .attr =
        {
            .name = "uartAppRxQ"
        },
        .handle = NULL
    },

    [APP_Q_OWNER_RC_UART_TX] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(uart_app_msg_t),
        .attr =
        {
            .name = "uartAppTxQ"
        },
        .handle = NULL
    },

    
    [APP_Q_OWNER_CONN_MANAGER] =
    {
        .msg_count      = configCONN_MANAGER_QUEUE_DEPTH,
        .msg_size_bytes = sizeof(conn_manager_msg_t),
        .attr =
        {
            .name = "connManagerQ"
        },
        .handle = NULL
    },


    [APP_Q_OWNER_RC_WIFI] =
    {
        .msg_count      = configWIFI_APP_QUEUE_LEN,
        .msg_size_bytes = sizeof(wifi_app_msg_t),
        .attr =
        {
            .name = "rcWifiQ"
        },
        .handle = NULL
    }

    // [APP_Q_OWNER_RC_BLE] =
    // {
    //     .msg_count      = 10U,
    //     .msg_size_bytes = sizeof(rc_ble_msg_t),
    //     .attr =
    //     {
    //         .name = "rcBleQ"
    //     },
    //     .handle = NULL
    // },

    // [APP_Q_OWNER_RC_BT_CLASSIC] =
    // {
    //     .msg_count      = 10U,
    //     .msg_size_bytes = sizeof(rc_bt_classic_msg_t),
    //     .attr =
    //     {
    //         .name = "rcBtClassicQ"
    //     },
    //     .handle = NULL
    // }
};


/* =========================================================================
 * MUTEX CONFIGURATION
 * ========================================================================= */

os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX] =
{
    [APP_MUTEX_OWNER_UART_APP] =
    {
        .attr =
        {
            .name      = "uartAppMutex",
            .attr_bits = osMutexPrioInherit
        },
        .handle = NULL
    },

    [APP_MUTEX_OWNER_CONN_MANAGER] =
    {
        .attr =
        {
            .name      = "rcConnectionMtx",
            .attr_bits = osMutexPrioInherit
        },
        .handle = NULL
    }
};


/* =========================================================================
 * SEMAPHORE CONFIGURATION
 * =========================================================================
 *
 * No application-owned semaphore is currently configured.
 * Entries below BSP_SEMAPHORE_OWNER_MAX are unused in this array.
 * ========================================================================= */

os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX] =
{
    {0}
};


/* =========================================================================
 * EVENT FLAGS CONFIGURATION
 * =========================================================================
 *
 * No application-owned event flag is currently configured.
 * Entries below BSP_EVENT_FLAGS_OWNER_MAX are unused in this array.
 * ========================================================================= */

os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX] =
{
    {0}
};

/* =========================================================================
 * FILE END
 * ========================================================================= */
