/**
 * @file app_logging.h
 * @author raghav.jha
 * @brief Application logging backend selection.
 * @version 1.1
 * @date 2026-10-24
 *
 * @copyright Copyright (c) 2026
 * Navyantra Tech Private Limited
 *
 * @details
 * Provides a common application logging interface.
 *
 * Supported backends:
 * - ESP-IDF logger
 * - BSP asynchronous logger
 * - Logging disabled
 *
 * Only the selected backend is compiled.
 *
 * @warning
 * Native ESP-IDF logging may transmit plaintext messages.
 * Market release builds requiring encrypted logging must
 * select the secure BSP logger and configure ESP-IDF
 * console logging separately.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-24    Jha Raghav      Corrected backend selection and variadic macros
 * 2026-05-30    Jha Raghav      Initial version
 * ============================================================================
 */

#ifndef APP_LOGGING_H
#define APP_LOGGING_H

/* ============================================================================
 * 1. CONFIGURATION
 * ============================================================================
 */

#include "app_config.h"

/* ============================================================================
 * 2. CONFIGURATION VALIDATION
 * ============================================================================
 */

#if !defined(configDEFAULT_LOGGER_ENABLED)
    #error "configDEFAULT_LOGGER_ENABLED is not defined"
#endif

#if !defined(ESP_LOGGER_ENABLED)
    #error "ESP_LOGGER_ENABLED is not defined"
#endif

#if !defined(BSP_LOGGER_ENABLED)
    #error "BSP_LOGGER_ENABLED is not defined"
#endif

#if (ESP_LOGGER_ENABLED == BSP_LOGGER_ENABLED)
    #error "ESP and BSP logger backend values must be different"
#endif

/* ============================================================================
 * 3. ESP-IDF LOGGER
 * ============================================================================
 */

#if (configDEFAULT_LOGGER_ENABLED == ESP_LOGGER_ENABLED)

    #include "esp_log.h"

    #ifndef APP_LOG_TAG
        #define APP_LOG_TAG (strrchr("/" __FILE__, '/') + 1)
    #endif

    #define WIFI_LOGI(...) ESP_LOGI(APP_LOG_TAG, __VA_ARGS__)
    #define WIFI_LOGW(...) ESP_LOGW(APP_LOG_TAG, __VA_ARGS__)
    #define WIFI_LOGE(...) ESP_LOGE(APP_LOG_TAG, __VA_ARGS__)
    #define WIFI_LOGD(...) ESP_LOGD(APP_LOG_TAG, __VA_ARGS__)

    /* ============================================================================
     * 4. BSP LOGGER
     * ============================================================================
     */

#elif (configDEFAULT_LOGGER_ENABLED == BSP_LOGGER_ENABLED)

    #include "bsp_log.h"

    #define WIFI_LOGI(...) LOGI(__VA_ARGS__)
    #define WIFI_LOGW(...) LOGW(__VA_ARGS__)
    #define WIFI_LOGE(...) LOGE(__VA_ARGS__)
    #define WIFI_LOGD(...) LOGD(__VA_ARGS__)

    /* ============================================================================
     * 5. LOGGING DISABLED
     * ============================================================================
     */

#else

    #define WIFI_LOGI(...) ((void)0)
    #define WIFI_LOGW(...) ((void)0)
    #define WIFI_LOGE(...) ((void)0)
    #define WIFI_LOGD(...) ((void)0)

#endif

#endif /* APP_LOGGING_H */