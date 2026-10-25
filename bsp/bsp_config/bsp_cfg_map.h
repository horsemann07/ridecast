
/**
 * ============================================================================
 * @file    bsp_cfg_map.h
 * @brief   BSP-owned RTOS object identifiers.
 *
 * @details
 * Defines the owner IDs used to identify BSP threads, queues, mutexes,
 * semaphores and event flags.
 *
 * These IDs are used to index the BSP object configuration arrays
 * defined in bsp_cfg_map.c.
 *
 * Application integration:
 *   Application owner IDs may begin at the corresponding BSP_*_OWNER_MAX
 *   value to avoid overlapping BSP and application identifiers.
 *
 * @note
 *   Only BSP-owned objects are declared here.
 *   Application-owned objects are declared in the application configuration.
 *
 * @warning
 *   BSP_*_OWNER_MAX values are array boundaries, not usable object IDs.
 *   Reserved or unconfigured entries must not be created as RTOS objects.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Clarified BSP ownership and platform-dependent thread IDs.
 * ============================================================================
 */

#ifndef BSP_CFG_MAP_H
#define BSP_CFG_MAP_H

/* =========================================================================
 * BSP THREAD OWNERS
 * ========================================================================= */

typedef enum
{
    /*
     * Reserve index zero to preserve the existing ESP32 UART thread ID.
     * This entry must not be used to create a thread.
     */

    BSP_THREAD_OWNER_MAX = 0U /**< Reserved for BSP thread owner IDs. */

} app_bsp_thread_owner_t;


/* =========================================================================
 * BSP QUEUE OWNERS
 * ========================================================================= */

typedef enum
{
    BSP_QUEUE_OWNER_LOG_TASK = 0, /**< Task-context logger queue. */
    BSP_QUEUE_OWNER_LOG_ISR,      /**< ISR-context logger queue. */
    BSP_QUEUE_WIFI_EVENT,         /**< Wi-Fi event queue. */

    BSP_QUEUE_OWNER_MAX

} bsp_queue_owner_t;


/* =========================================================================
 * BSP MUTEX OWNERS
 * ========================================================================= */

typedef enum
{
    BSP_MUTEX_OWNER_LOG = 0,    /**< Logger mutex. */
    BSP_MUTEX_OWNER_CLI,        /**< CLI mutex. */
    BSP_MUTEX_OWNER_WIFI,       /**< Wi-Fi mutex. */
    BSP_MUTEX_OWNER_UART_ASYNC, /**< Asynchronous UART mutex. */

    BSP_MUTEX_OWNER_MAX

} bsp_mutex_owner_t;


/* =========================================================================
 * BSP SEMAPHORE OWNERS
 * ========================================================================= */

typedef enum
{
    BSP_SEMAPHORE_OWNER_LOG = 0, /**< Logger semaphore. */
    BSP_SEMAPHORE_OWNER_WIFI,    /**< Wi-Fi semaphore. */

    BSP_SEMAPHORE_OWNER_MAX

} bsp_semaphore_owner_t;


/* =========================================================================
 * BSP EVENT FLAG OWNERS
 * ========================================================================= */

typedef enum
{
    BSP_EVENT_FLAGS_OWNER_LOG = 0, /**< Logger event flags. */
    BSP_EVENT_FLAGS_OWNER_WIFI,    /**< Wi-Fi event flags. */

    BSP_EVENT_FLAGS_OWNER_MAX

} bsp_event_flags_owner_t;

#endif /* BSP_CFG_MAP_H */
