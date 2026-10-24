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
#include "bsp_defs.h"

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

    /* -------------------------------------------------------------------------- */
    /* BSP Logging Configuration */
    /* -------------------------------------------------------------------------- */

#ifndef APP_OVERRIDE_LOG_CONFIGURATION

    /*
     * Maximum logging severity compiled into the firmware.
     *
     * Valid values:
     * 0 = BSP_LOG_NONE
     * 1 = BSP_LOG_WARN
     * 2 = BSP_LOG_ERROR
     * 3 = BSP_LOG_INFO
     * 4 = BSP_LOG_DEBUG
     *
     * Runtime logging levels are controlled independently.
     */
    #define BSP_COMPILED_LOG_LEVEL            (4)

    /* Maximum formatted log line length in bytes. */
    #define bspCONFIG_LOG_LINE_MAX_LEN        (256U)

    /* Maximum logger mutex acquisition timeout in milliseconds. */
    #define bspCONFIG_LOG_MUTEX_TIMEOUT_MS    (100U)

    /* Maximum ISR log message length in bytes. */
    #define bspCONFIG_LOG_ISR_MESSAGE_MAX_LEN (96U)

    /* Maximum number of pending ISR log messages. */
    #define bspCONFIG_LOG_ISR_QUEUE_LENGTH    (32U)

    /* Enable ANSI color formatting in log messages. */
    #define bspCONFIG_LOG_COLOR_SUPPORT       (1)

#endif /* APP_OVERRIDE_LOG_CONFIGURATION */

    /* -------------------------------------------------------------------------- */
    /* BSP CLI Configuration */
    /* -------------------------------------------------------------------------- */

#ifndef APP_OVERRIDE_CLI_CONFIGURATION

    /* Default command-line prompt string. */
    #define bspCONFIG_CLI_PROMPT                ("> ")

    /* Maximum CLI formatted output buffer size in bytes. */
    #define bspCONFIG_CLI_PRINTF_BUF_SIZE       (256U)

    /* Maximum number of command arguments. */
    #define bspCONFIG_CLI_MAX_ARGS              (16U)

    /* Number of machine words reserved for the embedded CLI buffer. */
    #define bspCONFIG_CLI_EMBEDDED_BUFFER_WORDS (512U)

    /* CLI receive buffer capacity in bytes. */
    #define bspCONFIG_CLI_RX_BUF_SIZE           (64U)

    /* CLI transmit buffer capacity in bytes. */
    #define bspCONFIG_CLI_TX_BUF_SIZE           (256U)

    /* Maximum command input buffer size in bytes. */
    #define bspCONFIG_CLI_CMD_BUF_SIZE          (256U)

    /* CLI ring buffer capacity in bytes. */
    #define bspCONFIG_CLI_RING_SIZE             (256U)

    /* Maximum number of registered CLI bindings. */
    #define bspCONFIG_CLI_MAX_BINDINGS          (16U)

    /* CLI mutex acquisition timeout in milliseconds. */
    #define bspCONFIG_CLI_MUTEX_TIMEOUT_MS      (100U)

    /* Default firmware identification string. */
    #define bspCONFIG_CLI_FW_NAME               ("Firmware")

    /* Default firmware version string. */
    #define bspCONFIG_CLI_FW_VERSION            ("1.0.0")

    /* Enable or disable CLI autocomplete support (0U: disabled, 1U: enabled). */
    #define bspCONFIG_CLI_AUTOCOMPLETE          (0U)

#endif /* APP_OVERRIDE_CLI_CONFIGURATION */

    /* -------------------------------------------------------------------------- */
    /* BSP UART Configuration */
    /* -------------------------------------------------------------------------- */

#ifndef APP_OVERRIDE_UART_CONFIGURATION

    /* Enable asynchronous UART operations (0U: disabled, 1U: enabled). */
    #define bspCONFIG_UART_ENABLE_ASYNC          (1U)

    /* Native UART transmit and receive buffer capacity in bytes. */
    #define bspCONFIG_UART_RXTX_BUFFER_SIZE      (1024U)

    /* Maximum number of pending asynchronous UART events. */
    #define bspCONFIG_UART_ASYNC_EVENT_QUEUE_LEN (16U)

    /* UART worker task stack allocation in bytes. */
    #define bspCONFIG_UART_TASK_STACK_BYTES      (4096U)

    /* UART worker scheduling priority. */
    #define bspCONFIG_UART_TASK_PRIORITY         (8U)

    /* UART event polling interval in milliseconds. */
    #define bspCONFIG_UART_POLL_MS               (10U)

    /* UART worker shutdown timeout in milliseconds. */
    #define bspCONFIG_UART_SHUTDOWN_MS           (1000U)

#endif /* APP_OVERRIDE_UART_CONFIGURATION */

    /* -------------------------------------------------------------------------- */
    /* BSP Wi-Fi Configuration */
    /* -------------------------------------------------------------------------- */

#ifndef APP_OVERRIDE_WIFI_CONFIGURATION

    /* Maximum SSID field size in bytes. */
    #define bspCONFIG_WIFI_SSID_MAX_LEN       (32U)

    /* Maximum Wi-Fi password field size in bytes. */
    #define bspCONFIG_WIFI_PASSWORD_MAX_LEN   (64U)

    /* Maximum IP address string size including the NUL terminator. */
    #define bspCONFIG_WIFI_IP_ADDR_MAX_LEN    (40U)

    /* MAC address size in raw bytes. */
    #define bspCONFIG_WIFI_MAC_ADDR_MAX_LEN   (6U)

    /* Maximum number of saved Wi-Fi network configurations. */
    #define bspCONFIG_WIFI_MAX_SAVED_NETWORKS (5U)

    /* Maximum number of pending Wi-Fi events. */
    #define configWIFI_EVENT_QUEUE_DEPTH      (10U)

    /* Wi-Fi operation timeout in milliseconds. */
    #define configWIFI_WAIT_TIMEOUT_MS        (5000U)

    /* NVS namespace used to store Wi-Fi configuration. */
    #define configWIFI_NVS_NAMESPACE          ("bsp_wifi")

#endif /* APP_OVERRIDE_WIFI_CONFIGURATION */

    /* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */