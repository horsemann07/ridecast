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

#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */