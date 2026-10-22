/*
 * =========================================================================
 * rc_config.h
 *
 * Description:
 * RideCast configuration and OS/peripheral owner definitions.
 *
 * Change History:
 * Format:
 * YYYY-MM-DD - Developer - Description
 *
 * 2026-05-23 - J.Raghav - Initial development
 * =========================================================================
 */

#ifndef RC_CONFIG_H
#define RC_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "bsp_config.h"
#include "bsp_uart.h"


/* =========================================================================
 * GLOBAL CONFIGURATIONS
 * ========================================================================= */

/*
 * Board selection should come from build system or board-specific config.
 * Example:
 *   -DappConfig_BOARD_ESP32=1
 */
#define appConfig_BOARD_ESP32 (1U)


/* --------------------------------
 * Logging Configuration
 * -------------------------------- */
#if defined(appConfig_BOARD_ESP32)
    #define appConfig_LOG_BACKEND_ESP (1U)
#else
    #define appConfig_LOG_BACKEND_BSP (1U)
#endif


    /* --------------------------------
     * RideCast Configuration
     * -------------------------------- */

#define appCfg_HOST_UART_RXTX_BUF_SIZE  (256U)
#define appCfg_WIFI_RXTX_BUF_SIZE       (256U)
#define appCfg_WIFI_TCP_PORT            (5555U)
#define appCfg_BLE_RXTX_BUF_SIZE        (256U)
#define appCfg_BT_CLASSIC_RXTX_BUF_SIZE (256U)


    /* =========================================================================
     * THREAD OWNER CONFIGURATION
     * =========================================================================
     *
     * ALL THREADS (BSP + APP) MUST BE DEFINED IN THIS FILE.
     *
     * - Add thread entry to this enum.
     * - Initialize corresponding entry in g_appThreadCfg.
     * - Thread handles are filled at runtime after thread creation.
     *
     * ========================================================================= */

    typedef enum
    {
        /* --------------------------------------------------------
         * Application threads
         * -------------------------------------------------------- */

        APP_THREAD_OWNER_STARTUP       = BSP_THREAD_OWNER_MAX,
        APP_THREAD_OWNER_UART_APP_TX   = BSP_THREAD_OWNER_MAX + 1,
        APP_THREAD_OWNER_UART_APP_RX   = BSP_THREAD_OWNER_MAX + 2,
        APP_THREAD_OWNER_RC_WIFI       = BSP_THREAD_OWNER_MAX + 3,
        APP_THREAD_OWNER_RC_BLE        = BSP_THREAD_OWNER_MAX + 4,
        APP_THREAD_OWNER_RC_BT_CLASSIC = BSP_THREAD_OWNER_MAX + 5,
        APP_THREAD_OWNER_MAX

    } app_thread_owner_t;


    /* =========================================================================
     * QUEUE OWNER CONFIGURATION
     * =========================================================================
     *
     * ALL QUEUES (BSP + APP) MUST BE DEFINED IN THIS FILE.
     *
     * ========================================================================= */

    typedef enum
    {
        APP_Q_OWNER_RC_UART_RX    = BSP_QUEUE_OWNER_MAX,
        APP_Q_OWNER_RC_UART_TX    = BSP_QUEUE_OWNER_MAX + 1,
        APP_Q_OWNER_RC_WIFI       = BSP_QUEUE_OWNER_MAX + 2,
        APP_Q_OWNER_RC_BLE        = BSP_QUEUE_OWNER_MAX + 3,
        APP_Q_OWNER_RC_BT_CLASSIC = BSP_QUEUE_OWNER_MAX + 4,
        APP_Q_OWNER_MAX

    } app_queue_owner_t;


    /* =========================================================================
     * MUTEX OWNER CONFIGURATION
     * =========================================================================
     *
     * ALL MUTEXES (BSP + APP) MUST BE DEFINED IN THIS FILE.
     *
     * ========================================================================= */

    typedef enum
    {
        APP_MUTEX_OWNER_UART_APP_TX   = BSP_MUTEX_OWNER_MAX,
        APP_MUTEX_OWNER_UART_APP_RX   = BSP_MUTEX_OWNER_MAX + 1,
        APP_MUTEX_OWNER_RC_CONNECTION = BSP_MUTEX_OWNER_MAX + 2,
        APP_MUTEX_OWNER_MAX
    } app_mutex_owner_t;


    /* =========================================================================
     * SEMAPHORE OWNER CONFIGURATION
     * =========================================================================
     *
     * No RideCast semaphore is required initially.
     * Add one only when the implementation requires it.
     *
     * ========================================================================= */

    typedef enum
    {
        APP_SEMAPHORE_OWNER_MAX = BSP_SEMAPHORE_OWNER_MAX
    } app_semaphore_owner_t;


    /* =========================================================================
     * EVENT FLAGS OWNER CONFIGURATION
     * =========================================================================
     *
     * No RideCast event flag is required initially.
     * Add one only when the implementation requires it.
     *
     * ========================================================================= */

    typedef enum
    {
        APP_EVENT_FLAGS_OWNER_MAX = BSP_EVENT_FLAGS_OWNER_MAX
    } app_event_flags_owner_t;


    /* =========================================================================
     * UART OWNER CONFIGURATION
     * ========================================================================= */

    typedef enum
    {
        APP_UART_OWNER_CONSOLE   = 0,
        APP_UART_OWNER_HOST_COMM = 1,
        APP_UART_OWNER_MAX
    } app_uart_owner_t;


    /* =========================================================================
     * OS MAP EXTERNAL VARIABLES
     * ========================================================================= */

    extern os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX];
    extern os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX];


    /* =========================================================================
     * PERIPHERAL MAP EXTERNAL VARIABLES
     * ========================================================================= */
    extern const bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX];


#ifdef __cplusplus
}
#endif

#endif /* RC_CONFIG_H */