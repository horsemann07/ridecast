#ifndef DEMO_NAL_TCP_TLS_H
#define DEMO_NAL_TCP_TLS_H


#include "nal_network.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
/* ============================================================
 * Logging abstraction layer
 * ============================================================ */
#ifdef ESP_BOARD_LOGGING // ESP-IDF logging
    #include "esp_log.h"
    #define __FILENAME__        (strrchr("/" __FILE__, '/') + 1)
    #define DEMO_LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
    #define DEMO_LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
    #define DEMO_LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
    #define DEMO_LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)
#else // CLI/BSP fallback logging
    #include "cli_logger.h"
    #define DEMO_LOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
    #define DEMO_LOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
    #define DEMO_LOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
    #define DEMO_LOGD(fmt, ...) BSP_LOGD(fmt, ##__VA_ARGS__)
#endif

#endif // DEMO_NAL_TCP_TLS_H