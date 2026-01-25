#ifndef NAL_LOG_H
#define NAL_LOG_H

#include "nal_config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ============================================================
 * Log level configuration
 * ============================================================ */

/*
 * Log levels:
 * 0 = OFF
 * 1 = ERROR
 * 2 = WARN
 * 3 = INFO
 * 4 = DEBUG
 */
#ifndef NAL_LOG_LEVEL
    #define NAL_LOG_LEVEL 3
#endif

/* ============================================================
 * Platform log mapping
 * ============================================================ */

#if defined(ESP_PLATFORM)

    /* ---------- ESP-IDF ---------- */
    #include "esp_log.h"

    #ifndef _FILENAME__
        #define _FILENAME__ (__builtin_strrchr("/" __FILE__, '/') + 1)
    #endif

    #if (NAL_LOG_LEVEL >= 1)
        #define NAL_LOGE(fmt, ...) ESP_LOGE(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGE(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 2)
        #define NAL_LOGW(fmt, ...) ESP_LOGW(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGW(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 3)
        #define NAL_LOGI(fmt, ...) ESP_LOGI(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGI(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 4)
        #define NAL_LOGD(fmt, ...) ESP_LOGD(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGD(fmt, ...)
    #endif

#elif defined(NAL_USE_BSP_LOG)

    /* ---------- BSP / Custom ---------- */
    #include "bsp_log.h"

    #if (NAL_LOG_LEVEL >= 1)
        #define NAL_LOGE(fmt, ...) BSP_LOGE(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGE(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 2)
        #define NAL_LOGW(fmt, ...) BSP_LOGW(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGW(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 3)
        #define NAL_LOGI(fmt, ...) BSP_LOGI(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGI(fmt, ...)
    #endif

    #if (NAL_LOG_LEVEL >= 4)
        #define NAL_LOGD(fmt, ...) BSP_LOGD(_FILENAME__, fmt, ##__VA_ARGS__)
    #else
        #define NAL_LOGD(fmt, ...)
    #endif

#else
    #error "No log backend selected. Please define NAL_USE_BSP_LOG or ESP_PLATFORM."
#endif /* platform selection */

#endif /* NAL_LOG_H */
