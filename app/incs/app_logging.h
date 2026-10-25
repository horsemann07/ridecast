
/**
 * @file    app_logging.h
 * @author  raghav.jha
 * @brief   RideCast application logging interface and backend selection.
 * @version 1.2
 * @date    2026-10-25
 *
 * @copyright Copyright (c) 2026
 * Navyantra Tech Private Limited
 *
 * @details
 * Provides a common logging interface for all RideCast application modules.
 *
 * Supported logging backends:
 *   - ESP-IDF logger
 *   - BSP logger
 *   - Logging disabled
 *
 * The logging backend is selected at compile time using
 * configDEFAULT_LOGGER_ENABLED from app_config.h.
 *
 * Application modules should use:
 *   ALOGE(...) - Error
 *   ALOGW(...) - Warning
 *   ALOGI(...) - Information
 *   ALOGD(...) - Debug
 *
 * Usage:
 *   #include "app_logging.h"
 *
 *   ALOGI("RideCast initialized");
 *   ALOGE("UART initialization failed");
 *
 * Each source file may define APP_LOG_TAG before including this header
 * to override the default tag derived from its source filename.
 *
 * @note
 *   This file provides logging macros only.
 *   Logger initialization and secure-session handling are performed
 *   by the selected logging backend.
 *
 * @warning
 *   Native ESP-IDF logging may transmit plaintext messages.
 *   Market release builds requiring secure logging must select
 *   the BSP secure logger and configure ESP-IDF console logging
 *   separately.
 *
 *   Selecting the BSP logger does not itself establish an encrypted
 *   session or guarantee that other platform logs are suppressed.
 *
 * ============================================================================
 * Change Log:
 *
 * Date          Author          Description
 * ------------  --------------  ----------------------------------------------
 * 2026-10-25    Jha Raghav      Unified application log macros and validation
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

#ifndef configDEFAULT_LOGGER_ENABLED
    #error "configDEFAULT_LOGGER_ENABLED is not defined"
#endif

#ifndef ESP_LOGGER_ENABLED
    #error "ESP_LOGGER_ENABLED is not defined"
#endif

#ifndef BSP_LOGGER_ENABLED
    #error "BSP_LOGGER_ENABLED is not defined"
#endif

#ifndef LOGGING_DISABLED
    #error "LOGGING_DISABLED is not defined"
#endif

#if (ESP_LOGGER_ENABLED == BSP_LOGGER_ENABLED) || \
    (ESP_LOGGER_ENABLED == LOGGING_DISABLED) || \
    (BSP_LOGGER_ENABLED == LOGGING_DISABLED)
    #error "Logging backend identifiers must be unique"
#endif

#if (configDEFAULT_LOGGER_ENABLED != ESP_LOGGER_ENABLED) && \
    (configDEFAULT_LOGGER_ENABLED != BSP_LOGGER_ENABLED) && \
    (configDEFAULT_LOGGER_ENABLED != LOGGING_DISABLED)
    #error "Invalid configDEFAULT_LOGGER_ENABLED selection"
#endif


/* ============================================================================
 * 3. ESP-IDF LOGGER
 * ============================================================================
 */

#if (configDEFAULT_LOGGER_ENABLED == ESP_LOGGER_ENABLED)

    #if !defined(CMAKE_ESPx_PLATFORM)
        #error "ESP-IDF logger selected for a non-ESP platform"
    #endif

    #include <string.h>
    #include "esp_log.h"

    /* Default tag is the source filename. */
    #ifndef APP_LOG_TAG
        #define APP_LOG_TAG (strrchr("/" __FILE__, '/') + 1)
    #endif

    #define ALOGE(...) ESP_LOGE(APP_LOG_TAG, __VA_ARGS__)
    #define ALOGW(...) ESP_LOGW(APP_LOG_TAG, __VA_ARGS__)
    #define ALOGI(...) ESP_LOGI(APP_LOG_TAG, __VA_ARGS__)
    #define ALOGD(...) ESP_LOGD(APP_LOG_TAG, __VA_ARGS__)


/* ============================================================================
 * 4. BSP LOGGER
 * ============================================================================
 */

#elif (configDEFAULT_LOGGER_ENABLED == BSP_LOGGER_ENABLED)

    #include "bsp_log.h"

    #define ALOGE(...) LOGE(__VA_ARGS__)
    #define ALOGW(...) LOGW(__VA_ARGS__)
    #define ALOGI(...) LOGI(__VA_ARGS__)
    #define ALOGD(...) LOGD(__VA_ARGS__)


/* ============================================================================
 * 5. LOGGING DISABLED
 * ============================================================================
 */

#else

    #define ALOGE(...) ((void)0)
    #define ALOGW(...) ((void)0)
    #define ALOGI(...) ((void)0)
    #define ALOGD(...) ((void)0)

#endif

#endif /* APP_LOGGING_H */
