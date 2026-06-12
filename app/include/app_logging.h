/**
 * @file app_logging.h
 * @author raghav.jha
 * @brief
 * @version
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 *
 */


#ifndef APP_LOGGING_H
#define APP_LOGGING_H

#include "app_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(appConfig_LOG_BACKEND_BSP) && defined(appConfig_LOG_BACKEND_ESP)
    #error "Only one logging backend may be selected"
#endif

#if !defined(appConfig_LOG_BACKEND_BSP) && !defined(appConfig_LOG_BACKEND_ESP)
    #define appConfig_LOG_BACKEND_BSP (1)
#endif


#if defined(appConfig_LOG_BACKEND_BSP) && (appConfig_LOG_BACKEND_BSP == 1)

    #include "bsp_log.h"

    #define ALOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
    #define ALOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
    #define ALOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
    #define ALOGD(fmt, ...) BSP_LOGD(fmt, ##__VA_ARGS__)

#elif defined(appConfig_LOG_BACKEND_ESP) && (appConfig_LOG_BACKEND_ESP == 1)

    #include "esp_log.h"

    #ifndef __FILENAME__
        #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
    #endif

    #ifndef APP_LOG_TAG
        #define APP_LOG_TAG __FILENAME__
    #endif

    #define ALOGE(fmt, ...) ESP_LOGE(APP_LOG_TAG, fmt, ##__VA_ARGS__)
    #define ALOGW(fmt, ...) ESP_LOGW(APP_LOG_TAG, fmt, ##__VA_ARGS__)
    #define ALOGI(fmt, ...) ESP_LOGI(APP_LOG_TAG, fmt, ##__VA_ARGS__)
    #define ALOGD(fmt, ...) ESP_LOGD(APP_LOG_TAG, fmt, ##__VA_ARGS__)

#else

    #define ALOGE(fmt, ...) \
        do                  \
        {                   \
        } while(0)

    #define ALOGW(fmt, ...) \
        do                  \
        {                   \
        } while(0)

    #define ALOGI(fmt, ...) \
        do                  \
        {                   \
        } while(0)

    #define ALOGD(fmt, ...) \
        do                  \
        {                   \
        } while(0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_LOGGING_H */