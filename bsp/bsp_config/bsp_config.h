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
#include "bsp_cfg_map.h"
#include "bsp_core.h"

#ifdef __cplusplus
extern "C"
{
#endif


    /**********************************************************************
     * EXTERNAL VARIABLES (Must be fined by user or in bsp_cfg_map.c)
     ***********************************************************************/
    extern os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX];
    extern os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX];

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

#ifndef bspCONFIG_LOG_LINE_MAX_LEN
    #define bspCONFIG_LOG_LINE_MAX_LEN (256U)
#endif

#ifndef bspCONFIG_LOG_MUTEX_TIMEOUT_MS
    #define bspCONFIG_LOG_MUTEX_TIMEOUT_MS (100U)
#endif

#ifndef bspCONFIG_LOG_ISR_MESSAGE_MAX_LEN
    #define bspCONFIG_LOG_ISR_MESSAGE_MAX_LEN (96U)
#endif

#ifndef bspCONFIG_LOG_ISR_QUEUE_LENGTH
    #define bspCONFIG_LOG_ISR_QUEUE_LENGTH (32U)
#endif

    /*----------------------------------------------------------------------------
     * CLI Configuration
     *---------------------------------------------------------------------------*/

#ifndef bspCONFIG_CLI_PROMPT
    #define bspCONFIG_CLI_PROMPT ("> ")
#endif

#ifndef bspCONFIG_CLI_PRINTF_BUF_SIZE
    #define bspCONFIG_CLI_PRINTF_BUF_SIZE (256U)
#endif

#ifndef bspCONFIG_CLI_MAX_ARGS
    #define bspCONFIG_CLI_MAX_ARGS (16U)
#endif

#ifndef bspCONFIG_CLI_EMBEDDED_BUFFER_WORDS
    #define bspCONFIG_CLI_EMBEDDED_BUFFER_WORDS (512U)
#endif

#ifndef bspCONFIG_CLI_RX_BUF_SIZE
    #define bspCONFIG_CLI_RX_BUF_SIZE (64U)
#endif

#ifndef bspCONFIG_CLI_TX_BUF_SIZE
    #define bspCONFIG_CLI_TX_BUF_SIZE (256U)
#endif

#ifndef bspCONFIG_CLI_CMD_BUF_SIZE
    #define bspCONFIG_CLI_CMD_BUF_SIZE (256U)
#endif

#ifndef bspCONFIG_CLI_RING_SIZE
    #define bspCONFIG_CLI_RING_SIZE (256U)
#endif

#ifndef bspCONFIG_CLI_MAX_BINDINGS
    #define bspCONFIG_CLI_MAX_BINDINGS (16U)
#endif

#ifndef bspCONFIG_CLI_MUTEX_TIMEOUT_MS
    #define bspCONFIG_CLI_MUTEX_TIMEOUT_MS (100U)
#endif

#ifndef bspCONFIG_CLI_FW_NAME
    #define bspCONFIG_CLI_FW_NAME ("Firmware")
#endif

#ifndef bspCONFIG_CLI_FW_VERSION
    #define bspCONFIG_CLI_FW_VERSION ("1.0.0")
#endif

/* Warning: Autocomplete is enabled by default which increases memory usage */
#ifndef bspCONFIG_CLI_AUTOCOMPLETE
    #define bspCONFIG_CLI_AUTOCOMPLETE (0U)
#endif

    /* =========================================================================
     * UART BSP Configuration
     * -------------------------------------------------------------------------
     * These constants configure the UART driver task and ring-buffer sizes.
     * All sizes are in bytes; all priorities are FreeRTOS-scale values.
     * ========================================================================= */

#ifndef bspCONFIG_UART_ENABLE_ASYNC
    #define bspCONFIG_UART_ENABLE_ASYNC (1U)
#endif

#ifndef bspCONFIG_UART_RXTX_BUFFER_SIZE
    #define bspCONFIG_UART_RXTX_BUFFER_SIZE (256U)
#endif

/* Internal event queue length for async UART operations (ESP only). */
#ifndef bspCONFIG_UART_ASYNC_ESP_DRIVER_EVENT_Q_LEN
    #define bspCONFIG_UART_ASYNC_ESP_DRIVER_EVENT_Q_LEN (10U)
#endif

    /* =========================================================================
     * Wi-Fi BSP Configuration
     * -------------------------------------------------------------------------
     * Maximum field lengths are in bytes and include the NUL terminator where
     * the field is a C string (SSID, password, IP address).
     * MAC address length is raw bytes (no NUL terminator).
     * ========================================================================= */

#ifndef bspCONFIG_WIFI_SSID_MAX_LEN
    #define bspCONFIG_WIFI_SSID_MAX_LEN (32U)
#endif

#ifndef bspCONFIG_WIFI_PASSWORD_MAX_LEN
    #define bspCONFIG_WIFI_PASSWORD_MAX_LEN (64U)
#endif

#ifndef bspCONFIG_WIFI_IP_ADDR_MAX_LEN
    #define bspCONFIG_WIFI_IP_ADDR_MAX_LEN (46U)
#endif

#ifndef bspCONFIG_WIFI_MAC_ADDR_MAX_LEN
    #define bspCONFIG_WIFI_MAC_ADDR_MAX_LEN (6U)
#endif

#ifndef bspCONFIG_WIFI_MAX_SAVED_NETWORKS
    #define bspCONFIG_WIFI_MAX_SAVED_NETWORKS (5U)
#endif


#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */