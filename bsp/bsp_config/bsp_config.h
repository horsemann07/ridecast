/*
 * bsp_config.h
 * ------------
 * Board Support Package (BSP) compile-time configuration constants.
 */


#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H


#include <stdint.h>
#include <stddef.h>
#include "bsp_os_map.h"
#include "bsp_core.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* =========================================================================
 * BSP Logging Configuration
 * -------------------------------------------------------------------------
 * BSP_COMPILED_LOG_LEVEL defines the maximum log severity compiled into
 * the image.
 *
 * Runtime behavior:
 * - Use bsp_log_set_level(...) to change the active log level at runtime.
 * - Use BSP_LOG_NONE at runtime to disable logging completely.
 *
 * Valid values:
 *   0 = BSP_LOG_NONE
 *   1 = BSP_LOG_WARN
 *   2 = BSP_LOG_ERROR
 *   3 = BSP_LOG_INFO
 *   4 = BSP_LOG_DEBUG
 * ========================================================================= */
#ifndef BSP_COMPILED_LOG_LEVEL
    #define BSP_COMPILED_LOG_LEVEL 4
#endif

/* =========================================================================
 * UART BSP Configuration
 * -------------------------------------------------------------------------
 * These constants configure the UART driver task and ring-buffer sizes.
 * All sizes are in bytes; all priorities are FreeRTOS-scale values.
 * ========================================================================= */
#define bspCONFIG_UART_ENABLE_ASYNC     (1U)
#define bspCONFIG_UART_RXTX_BUFFER_SIZE (256U)
/* Internal event queue length for async UART operations only for ESP */
#define bspCONFIG_UART_ASYNC_ESP_DRIVER_EVENT_Q_LEN ((uint32_t)10U)


/* =========================================================================
 * Wi-Fi BSP Configuration
 * -------------------------------------------------------------------------
 * Maximum field lengths are in bytes and include the NUL terminator where
 * the field is a C string (SSID, password, IP address).
 * MAC address length is raw bytes (no NUL terminator).
 * ========================================================================= */
#define bspCONFIG_WIFI_SSID_MAX_LEN       (32U)
#define bspCONFIG_WIFI_PASSWORD_MAX_LEN   (64U)
#define bspCONFIG_WIFI_IP_ADDR_MAX_LEN    (46U)
#define bspCONFIG_WIFI_MAC_ADDR_MAX_LEN   (6U)
#define bspCONFIG_WIFI_MAX_SAVED_NETWORKS (5U)


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
         *        BSP THREAD CONFIGURATION
         * -------------------------------------------------------- */
        BSP_THREAD_OWNER_LOGGER     = 0, /*< Logger thread. */
        BSP_THREAD_OWNER_UART_ASYNC = 1, /**< UART asynchronous thread. */
        BSP_THREAD_OWNER_MAX             /**< Maximum number of threads. */
    } app_bsp_thread_owner_t;


    /**********************************************************************
     * ALL QUEUES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add queue entries to bsp_queue_owner_t enum.
     * - Initialize corresponding os_queue_map_t entry in g_appBspQueueObj  array.
     * - Queue handles are filled at runtime after queue creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_WIFI_EVENT_QUEUE = 0, /*< WIFI event queue length. */
        BSP_QUEUE_OWNER_MAX       /*< Maximum number of queues. */
    } bsp_queue_owner_t;


    /**********************************************************************
     * ALL MUTEXES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add mutex entries to bsp_mutex_owner_t enum.
     * - Initialize corresponding os_mutex_map_t entry in g_bspMutexCfg  array.
     * - Mutex handles are filled at runtime after mutex creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_MUTEX_OWNER_LOG        = 0, /*< Log message mutex. */
        BSP_MUTEX_OWNER_WIFI       = 1, /*< WiFi module mutex. */
        BSP_MUTEX_OWNER_UART_ASYNC = 2, /*< UART asynchronous mutex. */
        BSP_MUTEX_OWNER_MAX             /*< Maximum number of mutexes. */
    } bsp_mutex_owner_t;


    /**********************************************************************
     * ALL SEMAPHORES (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add semaphore entries to bsp_semaphore_owner_t enum.
     * - Initialize corresponding os_semaphore_map_t entry in  g_appBspSemaphoreObj           array.
     * - Semaphore handles are filled at runtime after semaphore creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_SEMAPHORE_OWNER_LOG  = 0, /*< Log message semaphore. */
        BSP_SEMAPHORE_OWNER_WIFI = 1, /*< WiFi module semaphore. */
        BSP_SEMAPHORE_OWNER_MAX       /*< Maximum number of semaphores. */
    } bsp_semaphore_owner_t;


    /**********************************************************************
     * ALL EVENT FLAGS (BSP + APP) MUST BE DEFINED IN THIS FILE
     * - Add event flag entries to bsp_event_flags_owner_t enum.
     * - Initialize corresponding os_event_flags_map_t entry in
     * g_appBspEventFlagsObj array.
     * - Event flag handles are filled at runtime after event flag creation.
     ***********************************************************************/
    typedef enum
    {
        BSP_EVENT_FLAGS_OWNER_LOG  = 0, /*< Log message event flags. */
        BSP_EVENT_FLAGS_OWNER_WIFI = 1, /*< WiFi module event flags. */
        BSP_EVENT_FLAGS_OWNER_MAX       /*< Maximum number of event flags. */
    } bsp_event_flags_owner_t;


    /**********************************************************************
     * EXTERNAL VARIABLES
     ***********************************************************************/
    extern os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX];
    extern os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX];


#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */