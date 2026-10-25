
/**
 * ============================================================================
 * @file    rc_os_map.h
 * @brief   RideCast application configuration and resource owner definitions.
 *
 * @details
 * Defines RideCast application-owned RTOS resource identifiers,
 * peripheral owner identifiers and application configuration values.
 *
 * BSP resources are defined separately in bsp_cfg_map.h.
 * Application resource IDs begin at the corresponding BSP OWNER_MAX.
 *
 * This permits BSP and application resources to use separate configuration
 * maps while maintaining unique owner identifiers.
 *
 * Usage:
 *   - Add application owners to the appropriate enum.
 *   - Add matching configuration entries to rc_config.c.
 *   - Initialize only the application owner range.
 *   - Access the created objects through their configuration-map handles.
 *
 * @warning
 *   BSP_*_OWNER_MAX and APP_*_OWNER_MAX are array boundaries, not
 *   usable resource identifiers.
 *
 *   Application arrays use global owner IDs as their indices.
 *   Do not initialize entries reserved for BSP owners.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Aligned application owners with reusable BSP resource maps.
 * 2026-05-23  Initial development.
 * ============================================================================
 */

#ifndef RC_OS_MAP_H
#define RC_OS_MAP_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"
#include "bsp_config.h"
#include "bsp_uart.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // clang-format off
/* =========================================================================
    * APPLICATION THREAD OWNERS
    * =========================================================================
    *
    * Owner IDs start after the BSP thread owner range.
    * The corresponding entries are defined in g_appThreadCfg.
    * ========================================================================= */

typedef enum
{
    APP_THREAD_OWNER_UART_APP_TX = BSP_THREAD_OWNER_MAX,
    APP_THREAD_OWNER_UART_APP_RX,
    APP_THREAD_OWNER_CONN_MANAGER,
    APP_THREAD_OWNER_RC_WIFI,
    // APP_THREAD_OWNER_RC_BLE,
    // APP_THREAD_OWNER_RC_BT_CLASSIC,

    APP_THREAD_OWNER_MAX

} app_thread_owner_t;


/* =========================================================================
    * APPLICATION QUEUE OWNERS
    * ========================================================================= */

typedef enum
{
    APP_Q_OWNER_RC_UART_RX = BSP_QUEUE_OWNER_MAX,
    APP_Q_OWNER_RC_UART_TX,
    APP_Q_OWNER_CONN_MANAGER,
    APP_Q_OWNER_RC_WIFI,
    // APP_Q_OWNER_RC_BLE,
    // APP_Q_OWNER_RC_BT_CLASSIC,

    APP_Q_OWNER_MAX

} app_queue_owner_t;


/* =========================================================================
    * APPLICATION MUTEX OWNERS
    * ========================================================================= */

typedef enum
{
    APP_MUTEX_OWNER_UART_APP = BSP_MUTEX_OWNER_MAX,
    APP_MUTEX_OWNER_CONN_MANAGER,

    APP_MUTEX_OWNER_MAX

} app_mutex_owner_t;


/* =========================================================================
    * APPLICATION SEMAPHORE OWNERS
    * =========================================================================
    *
    * No application-owned semaphores are currently configured.
    * ========================================================================= */

typedef enum
{
    APP_SEMAPHORE_OWNER_MAX = BSP_SEMAPHORE_OWNER_MAX

} app_semaphore_owner_t;


/* =========================================================================
    * APPLICATION EVENT FLAG OWNERS
    * =========================================================================
    *
    * No application-owned event flags are currently configured.
    * ========================================================================= */

typedef enum
{
    APP_EVENT_FLAGS_OWNER_MAX = BSP_EVENT_FLAGS_OWNER_MAX

} app_event_flags_owner_t;


/* Application RTOS configuration maps. */
extern os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX];
extern os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX];
extern os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX];
extern os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX];
extern os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX];

/* =========================================================================
* APPLICATION RTOS HANDLE ACCESSORS
*
* The owner enum identifies the corresponding RTOS object.
*
* @note
*   These macros return the existing handle. They do not create
*   an RTOS object or validate whether initialization succeeded.
* ========================================================================= */

/* -------------------------------------------------------------------------- */
/* Application RTOS Handle Access Macros                                      */
/* -------------------------------------------------------------------------- */

/* Get application RTOS resource handles. */
#define APP_THREAD_HANDLE(owner)       (g_appThreadCfg[(owner)].handle)
#define APP_QUEUE_HANDLE(owner)        (g_appQueueCfg[(owner)].handle)
#define APP_MUTEX_HANDLE(owner)        (g_appMutexCfg[(owner)].handle)
#define APP_SEMAPHORE_HANDLE(owner)    (g_appSemaphoreCfg[(owner)].handle)
#define APP_EVENT_FLAGS_HANDLE(owner)  (g_appEventFlagsCfg[(owner)].handle)

/* -------------------------------------------------------------------------- */
/* Application RTOS Handle Clear Macros                                       */
/* -------------------------------------------------------------------------- */

/* Clear the stored application thread handle. */
#define APP_THREAD_HANDLE_CLEAR(owner) \
    (APP_THREAD_HANDLE(owner) = NULL)

/* Clear the stored application queue handle. */
#define APP_QUEUE_HANDLE_CLEAR(owner) \
    (APP_QUEUE_HANDLE(owner) = NULL)

/* Clear the stored application mutex handle. */
#define APP_MUTEX_HANDLE_CLEAR(owner) \
    (APP_MUTEX_HANDLE(owner) = NULL)

/* Clear the stored application semaphore handle. */
#define APP_SEMAPHORE_HANDLE_CLEAR(owner) \
    (APP_SEMAPHORE_HANDLE(owner) = NULL)

/* Clear the stored application event flags handle. */
#define APP_EVENT_FLAGS_HANDLE_CLEAR(owner) \
    (APP_EVENT_FLAGS_HANDLE(owner) = NULL)
#ifdef __cplusplus
}
#endif

#endif /* RC_OS_MAP_H */
