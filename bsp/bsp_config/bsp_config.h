
/**
 * ============================================================================
 * @file    bsp_config.h
 * @brief   Compile-time configuration for the reusable BSP.
 *
 * @details
 * Provides default configuration for BSP logging, CLI, UART and Wi-Fi.
 *
 * Configuration naming convention:
 *   configMODULE_SETTING
 *
 * Examples:
 *   configLOG_COLOR_SUPPORT
 *   configCLI_MAX_COMMANDS
 *   configUART_ENABLE_ASYNC
 *   configWIFI_EVENT_QUEUE_DEPTH
 *
 * Standalone BSP:
 *   All undefined configuration macros use the defaults in this file.
 *
 * Application integration:
 *   Define BSP_USE_APP_CONFIG_OVERRIDES in the build and provide
 *   app_bsp_overrides.h on the compiler include path.
 *
 *   The application overrides only the required settings. All remaining
 *   settings retain their BSP defaults.
 *
 * @warning
 *   The override header must contain preprocessor definitions only.
 *   It must not include bsp_config.h or application headers.
 *
 *   BSP and application modules must use identical configuration values
 *   during compilation.
 *
 * @note
 *   BSP RTOS owner identifiers are defined in bsp_cfg_map.h.
 *   BSP RTOS object configuration arrays are defined in bsp_cfg_map.c.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Standardized BSP configuration on config* macro naming.
 * ============================================================================
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#if defined(BSP_USE_APP_CONFIG_OVERRIDES)
    #include "app_bsp_overrides.h"
#endif

#include "bsp_os_map.h"
#include "bsp_cfg_map.h"
#include "bsp_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * BSP RTOS OBJECT MAPS
     * ========================================================================= */

    extern os_thread_map_t g_bspThreadCfg[BSP_THREAD_OWNER_MAX];
    extern os_queue_map_t g_bspQueueCfg[BSP_QUEUE_OWNER_MAX];
    extern os_mutex_map_t g_bspMutexCfg[BSP_MUTEX_OWNER_MAX];
    extern os_semaphore_map_t g_bspSemaphoreCfg[BSP_SEMAPHORE_OWNER_MAX];
    extern os_event_flags_map_t g_bspEventFlagsCfg[BSP_EVENT_FLAGS_OWNER_MAX];


/* =========================================================================
 * LOGGER CONFIGURATION
 * ========================================================================= */
/*
 * Compiled logging level:
 * 0 = NONE
 * 1 = WARN
 * 2 = ERROR
 * 3 = INFO
 * 4 = DEBUG
 */
#ifndef configLOG_COMPILED_LEVEL
    #define configLOG_COMPILED_LEVEL (4U)
#endif

#ifndef configLOG_LINE_MAX_LEN
    #define configLOG_LINE_MAX_LEN (256U)
#endif

#ifndef configLOG_MUTEX_TIMEOUT_MS
    #define configLOG_MUTEX_TIMEOUT_MS (100U)
#endif

#ifndef configLOG_TASK_MESSAGE_MAX
    #define configLOG_TASK_MESSAGE_MAX (160U)
#endif

#ifndef configLOG_ISR_MESSAGE_MAX
    #define configLOG_ISR_MESSAGE_MAX (96U)
#endif

#ifndef configLOG_OUTPUT_MAX
    #define configLOG_OUTPUT_MAX (320U)
#endif

#ifndef configLOG_TASK_QUEUE_LENGTH
    #define configLOG_TASK_QUEUE_LENGTH (32U)
#endif


#ifndef configLOG_ISR_QUEUE_LENGTH
    #define configLOG_ISR_QUEUE_LENGTH (32U)
#endif

#ifndef configLOG_COLOR_SUPPORT
    #define configLOG_COLOR_SUPPORT (1U)
#endif

    /* =========================================================================
     * LOGGER CONFIGURATION VALIDATION
     * ========================================================================= */

#if (configLOG_COMPILED_LEVEL > 4U)
    #error "configLOG_COMPILED_LEVEL must be 0..4"
#endif

#if (configLOG_COLOR_SUPPORT != 0U) && (configLOG_COLOR_SUPPORT != 1U)
    #error "configLOG_COLOR_SUPPORT must be 0 or 1"
#endif

#if (configLOG_TASK_MESSAGE_MAX < 8U) || (configLOG_TASK_MESSAGE_MAX > UINT16_MAX)
    #error "Invalid configLOG_TASK_MESSAGE_MAX"
#endif

#if (configLOG_ISR_MESSAGE_MAX < 2U) || (configLOG_ISR_MESSAGE_MAX > UINT16_MAX)
    #error "Invalid configLOG_ISR_MESSAGE_MAX"
#endif

#if (configLOG_OUTPUT_MAX < 48U) || (configLOG_OUTPUT_MAX > UINT16_MAX)
    #error "Invalid configLOG_OUTPUT_MAX"
#endif

#if (configLOG_LINE_MAX_LEN < 1U) || (configLOG_LINE_MAX_LEN > UINT16_MAX)
    #error "Invalid configLOG_LINE_MAX_LEN"
#endif

#if (configLOG_MUTEX_TIMEOUT_MS < 1U)
    #error "configLOG_MUTEX_TIMEOUT_MS must be nonzero"
#endif

#if (configLOG_ISR_QUEUE_LENGTH < 1U)
    #error "configLOG_ISR_QUEUE_LENGTH must be nonzero"
#endif


    /* =========================================================================
     * CLI CONFIGURATION
     * ========================================================================= */

#ifndef configCLI_PROMPT
    #define configCLI_PROMPT ("> ")
#endif

#ifndef configCLI_PRINTF_BUF_SIZE
    #define configCLI_PRINTF_BUF_SIZE (256U)
#endif

#ifndef configCLI_MAX_ARGS
    #define configCLI_MAX_ARGS (16U)
#endif

#ifndef configCLI_EMBEDDED_BUFFER_WORDS
    #define configCLI_EMBEDDED_BUFFER_WORDS (512U)
#endif

#ifndef configCLI_RX_BUF_SIZE
    #define configCLI_RX_BUF_SIZE (64U)
#endif

#ifndef configCLI_TX_BUF_SIZE
    #define configCLI_TX_BUF_SIZE (256U)
#endif

#ifndef configCLI_CMD_BUF_SIZE
    #define configCLI_CMD_BUF_SIZE (256U)
#endif

#ifndef configCLI_RING_SIZE
    #define configCLI_RING_SIZE (256U)
#endif

#ifndef configCLI_MAX_BINDINGS
    #define configCLI_MAX_BINDINGS (16U)
#endif

#ifndef configCLI_MUTEX_TIMEOUT_MS
    #define configCLI_MUTEX_TIMEOUT_MS (100U)
#endif

#ifndef configCLI_FW_NAME
    #define configCLI_FW_NAME ("Firmware")
#endif

#ifndef configCLI_FW_VERSION
    #define configCLI_FW_VERSION ("1.0.0")
#endif

#ifndef configCLI_AUTOCOMPLETE
    #define configCLI_AUTOCOMPLETE (0U)
#endif

#ifndef configCLI_MAX_COMMANDS
    #define configCLI_MAX_COMMANDS (24U)
#endif

#ifndef configCLI_MAX_OUTPUT_CHUNKS
    #define configCLI_MAX_OUTPUT_CHUNKS (256U)
#endif


    /* =========================================================================
     * CLI CONFIGURATION VALIDATION
     * ========================================================================= */

#if (configCLI_MAX_COMMANDS < 1U)
    #error "configCLI_MAX_COMMANDS must be at least 1"
#endif

#if (configCLI_MAX_OUTPUT_CHUNKS < 1U)
    #error "configCLI_MAX_OUTPUT_CHUNKS must be at least 1"
#endif

#if (configCLI_MAX_ARGS < 1U) || (configCLI_MAX_BINDINGS < 1U)
    #error "CLI argument and binding limits must be nonzero"
#endif

#if (configCLI_AUTOCOMPLETE != 0U) && (configCLI_AUTOCOMPLETE != 1U)
    #error "configCLI_AUTOCOMPLETE must be 0 or 1"
#endif


    /* =========================================================================
     * UART CONFIGURATION
     * ========================================================================= */

#ifndef configUART_ENABLE_ASYNC
    #define configUART_ENABLE_ASYNC (1U)
#endif

#ifndef configUART_RXTX_BUFFER_SIZE
    #define configUART_RXTX_BUFFER_SIZE (1024U)
#endif

#ifndef configUART_ASYNC_EVENT_QUEUE_LEN
    #define configUART_ASYNC_EVENT_QUEUE_LEN (16U)
#endif

#ifndef configUART_TASK_STACK_BYTES
    #define configUART_TASK_STACK_BYTES (4096U)
#endif

#ifndef configUART_TASK_PRIORITY
    #define configUART_TASK_PRIORITY (8U)
#endif

#ifndef configUART_POLL_MS
    #define configUART_POLL_MS (10U)
#endif

#ifndef configUART_SHUTDOWN_MS
    #define configUART_SHUTDOWN_MS (1000U)
#endif


    /* =========================================================================
     * UART CONFIGURATION VALIDATION
     * ========================================================================= */

#if (configUART_ENABLE_ASYNC != 0U) && (configUART_ENABLE_ASYNC != 1U)
    #error "configUART_ENABLE_ASYNC must be 0 or 1"
#endif

#if (configUART_RXTX_BUFFER_SIZE < 256U)
    #error "UART driver buffer must be at least 256 bytes"
#endif

#if (configUART_ENABLE_ASYNC == 1U)

    #if (configUART_ASYNC_EVENT_QUEUE_LEN < 1U) || (configUART_POLL_MS < 1U)
        #error "UART async queue depth and poll interval must be nonzero"
    #endif

    #if (configUART_TASK_STACK_BYTES < 1U)
        #error "UART worker stack size must be nonzero"
    #endif

#endif


    /* =========================================================================
     * WI-FI CONFIGURATION
     * ========================================================================= */

#ifndef configWIFI_SSID_MAX_LEN
    #define configWIFI_SSID_MAX_LEN (32U)
#endif

#ifndef configWIFI_PASSWORD_MAX_LEN
    #define configWIFI_PASSWORD_MAX_LEN (64U)
#endif

#ifndef configWIFI_IP_ADDR_MAX_LEN
    #define configWIFI_IP_ADDR_MAX_LEN (40U)
#endif

#ifndef configWIFI_MAC_ADDR_MAX_LEN
    #define configWIFI_MAC_ADDR_MAX_LEN (6U)
#endif

#ifndef configWIFI_MAX_SAVED_NETWORKS
    #define configWIFI_MAX_SAVED_NETWORKS (5U)
#endif

#ifndef configWIFI_EVENT_QUEUE_DEPTH
    #define configWIFI_EVENT_QUEUE_DEPTH (10U)
#endif

#ifndef configWIFI_WAIT_TIMEOUT_MS
    #define configWIFI_WAIT_TIMEOUT_MS (5000U)
#endif

#ifndef configWIFI_NVS_NAMESPACE
    #define configWIFI_NVS_NAMESPACE ("bsp_wifi")
#endif


    /* =========================================================================
     * WI-FI CONFIGURATION VALIDATION
     * ========================================================================= */

#if (configWIFI_SSID_MAX_LEN < 32U)
    #error "configWIFI_SSID_MAX_LEN must be at least 32"
#endif

#if (configWIFI_PASSWORD_MAX_LEN < 64U)
    #error "configWIFI_PASSWORD_MAX_LEN must be at least 64"
#endif

#if (configWIFI_MAC_ADDR_MAX_LEN != 6U)
    #error "configWIFI_MAC_ADDR_MAX_LEN must equal 6"
#endif

#if (configWIFI_MAX_SAVED_NETWORKS < 1U) || (configWIFI_MAX_SAVED_NETWORKS > 32U)
    #error "configWIFI_MAX_SAVED_NETWORKS must be 1..32"
#endif

#if (configWIFI_EVENT_QUEUE_DEPTH < 1U)
    #error "configWIFI_EVENT_QUEUE_DEPTH must be at least 1"
#endif

#if (configWIFI_WAIT_TIMEOUT_MS < 1U)
    #error "configWIFI_WAIT_TIMEOUT_MS must be nonzero"
#endif


#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */
