
/**
 * ============================================================================
 * @file    app_config.h
 * @brief   RideCast application-level configuration.
 *
 * @details
 * Defines application-specific compile-time settings used by RideCast.
 *
 * The BSP configuration is maintained separately in bsp_config.h.
 * Application RTOS object owners and maps are defined in rc_os_map.h.
 * Application task declarations are maintained in app_tasklist.h.
 *
 * All settings may be overridden through build-system definitions.
 *
 * @note
 *   The application must not hardcode a hardware platform here.
 *   Platform selection is controlled by the build system.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Separated application settings from OS resource mappings.
 * ============================================================================
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H


/* =========================================================================
 * APPLICATION LOGGING CONFIGURATION
 * ========================================================================= */

/* Logger backend identifiers. */
#ifndef LOGGING_DISABLED
    #define LOGGING_DISABLED (0U)
#endif

#ifndef ESP_LOGGER_ENABLED
    #define ESP_LOGGER_ENABLED (1U)
#endif

#ifndef BSP_LOGGER_ENABLED
    #define BSP_LOGGER_ENABLED (2U)
#endif

/*
 * Default logging backend.
 *
 * ESP32 development: ESP-IDF logger
 * Other platforms:   BSP logger
 *
 * Build configurations may override this selection.
 */
#ifndef configDEFAULT_LOGGER_ENABLED

    #if defined(CMAKE_ESPx_PLATFORM)
        #define configDEFAULT_LOGGER_ENABLED ESP_LOGGER_ENABLED
    #else
        #define configDEFAULT_LOGGER_ENABLED BSP_LOGGER_ENABLED
    #endif

#endif


/* =========================================================================
 * UART APPLICATION CONFIGURATION
 * ========================================================================= */


/* =========================================================================
 * UART APPLICATION CONFIGURATION
 * ========================================================================= */
#ifndef configHOST_UART_RXTX_BUF_SIZE
    #define configHOST_UART_RXTX_BUF_SIZE (256U)
#endif

/* Raw UART RX callback buffer capacity. */
#ifndef configUART_APP_RX_BUF_LEN
    #define configUART_APP_RX_BUF_LEN (configHOST_UART_RXTX_BUF_SIZE)
#endif

/* Encoded UART TX frame capacity. */
#ifndef configUART_APP_TX_BUF_LEN
    #define configUART_APP_TX_BUF_LEN (configHOST_UART_RXTX_BUF_SIZE)
#endif

/* Periodic UART communication heartbeat interval, in milliseconds. */
#ifndef configUART_APP_HEARTBEAT_MS
    #define configUART_APP_HEARTBEAT_MS (10000U)
#endif

/* UART RX polling timeout, in milliseconds. */
#ifndef configUART_APP_RX_TIMEOUT_MS
    #define configUART_APP_RX_TIMEOUT_MS (100U)
#endif

/* Number of raw UART chunks accepted by the application RX queue. */
#ifndef configUART_APP_RX_Q_LEN
    #define configUART_APP_RX_Q_LEN (16U)
#endif

/* Number of complete protocol frames accepted by the TX queue. */
#ifndef configUART_APP_TX_Q_LEN
    #define configUART_APP_TX_Q_LEN (8U)
#endif

/* UART TX worker wait timeout, in milliseconds. */
#ifndef configUART_APP_TX_WAIT_MS
    #define configUART_APP_TX_WAIT_MS (1000U)
#endif


/* =========================================================================
 * CONNECTION MANAGER CONFIGURATION
 * ========================================================================= */

#ifndef configCONN_MANAGER_DEFAULT_MAX_RETRY
    #define configCONN_MANAGER_DEFAULT_MAX_RETRY (5U)
#endif

#ifndef configCONN_MANAGER_DEFAULT_RETRY_INTERVAL_SEC
    #define configCONN_MANAGER_DEFAULT_RETRY_INTERVAL_SEC (1U)
#endif

#ifndef configCONN_MANAGER_WORKER_POLL_MS
    #define configCONN_MANAGER_WORKER_POLL_MS (100U)
#endif

#ifndef configCONN_MANAGER_QUEUE_DEPTH
    #define configCONN_MANAGER_QUEUE_DEPTH (16U)
#endif


/* =========================================================================
 * WI-FI APPLICATION CONFIGURATION
 * ========================================================================= */

/* =========================================================================
 * WI-FI APPLICATION CONFIGURATION
 * ========================================================================= */

/* Maximum Wi-Fi SSID field size in bytes. */
#ifndef configWIFI_APP_SSID_MAX_LEN
    #define configWIFI_APP_SSID_MAX_LEN (32U)
#endif

/* Maximum Wi-Fi password field size in bytes. */
#ifndef configWIFI_APP_PASS_MAX_LEN
    #define configWIFI_APP_PASS_MAX_LEN (64U)
#endif

/* Development authentication token buffer capacity. */
#ifndef configWIFI_APP_TOKEN_MAX_LEN
    #define configWIFI_APP_TOKEN_MAX_LEN (96U)
#endif

#ifndef configWIFI_RXTX_BUF_SIZE
    #define configWIFI_RXTX_BUF_SIZE (2048U)
#endif

/* Maximum reconnection attempts. */
#ifndef configWIFI_APP_MAX_RETRY
    #define configWIFI_APP_MAX_RETRY (5U)
#endif

/* Initial retry backoff in milliseconds. */
#ifndef configWIFI_APP_RETRY_BASE_MS
    #define configWIFI_APP_RETRY_BASE_MS (1000U)
#endif

/* Maximum retry backoff in milliseconds. */
#ifndef configWIFI_APP_RETRY_MAX_MS
    #define configWIFI_APP_RETRY_MAX_MS (15000U)
#endif

/* TCP authentication timeout in milliseconds. */
#ifndef configWIFI_APP_AUTH_TIMEOUT_MS
    #define configWIFI_APP_AUTH_TIMEOUT_MS (5000U)
#endif

/* Wi-Fi worker queue depth. */
#ifndef configWIFI_APP_QUEUE_LEN
    #define configWIFI_APP_QUEUE_LEN (8U)
#endif

/* Wi-Fi worker processing period in milliseconds. */
#ifndef configWIFI_APP_LOOP_MS
    #define configWIFI_APP_LOOP_MS (50U)
#endif

#ifndef configWIFI_TCP_PORT
    #define configWIFI_TCP_PORT (5000U)
#endif

/* =========================================================================
 * BLE APPLICATION CONFIGURATION
 * ========================================================================= */

#ifndef configBLE_RXTX_BUF_SIZE
    #define configBLE_RXTX_BUF_SIZE (256U)
#endif


/* =========================================================================
 * BLUETOOTH CLASSIC APPLICATION CONFIGURATION
 * ========================================================================= */

#ifndef configBT_CLASSIC_RXTX_BUF_SIZE
    #define configBT_CLASSIC_RXTX_BUF_SIZE (256U)
#endif

/* =========================================================================
 * BSP CONFIGURATION
 * ========================================================================= */

#include "bsp_config.h"


/* =========================================================================
 * APPLICATION RTOS OWNER MAP
 * ========================================================================= */

#include "rc_os_map.h"


/* =========================================================================
 * APPLICATION PERIPHERAL OWNER MAP
 * ========================================================================= */

#include "rc_peripheral_map.h"


/* =========================================================================
 * APPLICATION TASK DECLARATIONS
 * ========================================================================= */

#include "app_tasklist.h"


#endif /* APP_CONFIG_H */
