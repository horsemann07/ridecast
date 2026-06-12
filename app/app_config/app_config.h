/*
 * =========================================================================
 * app_config.h
 *
 * Description:
 * This header file defines the configuration for the Board Support Package
 * (BSP) used in the application. It includes necessary includes, type
 * definitions, and function prototypes related to BSP configuration.
 *
 * Change History:
 * Format:
 * YYYY-MM-DD - Developer - Description
 * =========================================================================
 *
 * 2026-05-23 - J.Raghav - Initial development
 *
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "bsp_config.h"
#include "bsp_uart.h"


/* =========================================================================
 * Global Configurations for BSP and Application
 * ========================================================================= */
/*
 * Board selection should come from build system or board-specific config.
 * Example:
 *   -DappConfig_BOARD_ESP32=1
 */
#define appConfig_BOARD_ESP32 (1)

/* --------------------------------
** Logging Configuration
** -------------------------------- */
#if defined(appConfig_BOARD_ESP32)
    #define appConfig_LOG_BACKEND_ESP (1)
#else
    #define appConfig_LOG_BACKEND_BSP (1)
#endif

#define appCfg_HOST_UART_RXTX_BUF_SIZE (256U)
#define appCfg_WIFI_RXTX_BUF_SIZE      (256U)
#define appCfg_WIFI_TCP_PORT           (5555U)


    /**********************************************************************
     *
     * ALL THREADS (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add thread entries to app_bsp_thread_owner_t enum.
     * - Initialize corresponding os_thread_map_t entry in g_appThreadCfg  array.
     * - Thread handles are filled at runtime after thread creation.
     ***********************************************************************/
    typedef enum
    {
        /* --------------------------------------------------------
         *        APPLICATION THREAD CONFIGURATION
         * -------------------------------------------------------- */
        APP_THREAD_OWNER_STARTUP = 0, /**< Startup thread - runs once after scheduler starts. */
        APP_THREAD_OWNER_UART_APP_TX = 1, /**< UART App TX thread. */
        APP_THREAD_OWNER_UART_APP_RX = 2, /**< UART App RX thread. */

        APP_THREAD_OWNER_WIFI_APP = 3,    /**< WiFi App thread. */

        APP_THREAD_OWNER_MAX              /**< Maximum number of threads. */
    } app_thread_owner_t;

    /**********************************************************************
     * ALL QUEUES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add queue entries to app_bsp_queue_owner_t enum.
     * - Initialize corresponding os_queue_map_t entry in g_appBspQueueObj  array.
     * - Queue handles are filled at runtime after queue creation.
     ***********************************************************************/
    typedef enum
    {
        APP_Q_OWNER_UART_APP_RX = 0, /*< UART App RX queue. */
        APP_Q_OWNER_UART_APP_TX = 1, /*< UART App TX queue. */
        APP_Q_OWNER_WIFI_APP    = 2, /*< WiFi App internal command queue. */
        APP_Q_OWNER_MAX              /*< Maximum number of queues. */
    } app_queue_owner_t;


    /**********************************************************************
     * ALL MUTEXES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add mutex entries to app_bsp_mutex_owner_t enum.
     * - Initialize corresponding os_mutex_map_t entry in g_appMutexCfg  array.
     * - Mutex handles are filled at runtime after mutex creation.
     ***********************************************************************/
    typedef enum
    {
        APP_MUTEX_OWNER_UART_APP_TX = 0, /*< UART App TX mutex. */
        APP_MUTEX_OWNER_UART_APP_RX = 1, /*< UART App RX mutex. */

        APP_MUTEX_OWNER_MAX              /*< Maximum number of mutexes. */
    } app_mutex_owner_t;


    /**********************************************************************
     * ALL SEMAPHORES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add semaphore entries to app_bsp_semaphore_owner_t enum.
     * - Initialize corresponding os_semaphore_map_t entry in  g_appBspSemaphoreObj            array.
     * - Semaphore handles are filled at runtime after semaphore creation.
     ***********************************************************************/
    typedef enum
    {
        APP_SEMAPHORE_OWNER_LOG  = 0, /*< Log message semaphore. */
        APP_SEMAPHORE_OWNER_WIFI = 1, /*< WiFi module semaphore. */
        APP_SEMAPHORE_OWNER_MAX       /*< Maximum number of semaphores. */
    } app_semaphore_owner_t;


    /**********************************************************************
     * ALL EVENT FLAGS (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add event flag entries to app_bsp_event_flags_owner_t enum.
     * - Initialize corresponding os_event_flags_map_t entry in
     * g_appBspEventFlagsObj array.
     * - Event flag handles are filled at runtime after event flag creation.
     ***********************************************************************/
    typedef enum
    {
        APP_EVENT_FLAGS_OWNER_LOG  = 0, /*< Log message event flags. */
        APP_EVENT_FLAGS_OWNER_WIFI = 1, /*< WiFi module event flags. */
        APP_EVENT_FLAGS_OWNER_MAX       /*< Maximum number of event flags. */
    } app_event_flags_owner_t;


    typedef enum
    {
        APP_UART_OWNER_HOST_COMM = 0, /*< UART instance used for Host communication. */
        APP_UART_OWNER_MAX            /*< Maximum number of UART instances. */
    } app_uart_owner_t;

    /**********************************************************************
     * OS MAP EXTERNAL VARIABLES
     ***********************************************************************/
    extern os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX];
    extern os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX];

    /**********************************************************************
     * PERIPHERAL MAP EXTERNAL VARIABLES
     ***********************************************************************/
    extern const bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX];

#ifdef __cplusplus
}
#endif

#endif // APP_CONFIG_H
