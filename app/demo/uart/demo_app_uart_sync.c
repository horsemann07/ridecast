/**
 * @file app_uart.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <string.h>


#include "bsp_uart.h"
#include "cmsis_os2.h"
#include "bsp_log.h"
#include "app_bsp_config.h"


#ifdef ESP_BOARD_LOGGING // ESP-IDF logging
    #include "esp_log.h"

    #define __FILENAME__   (strrchr("/" __FILE__, '/') + 1)
    #define LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
    #define LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)

#else // BSP logging: just map directly
    #include "bsp_log.h"
    #define LOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
    #define LOGD(fmt, ...) BSP_LOGD(fmt, ##__VA_ARGS__)
#endif


static void UartSyncTask(void* argument)
{
    (void)argument;

    const char txMsg[] = "UART Sync Test\r\n";
    uint8_t rxBuf[32];

    LOGI("Starting UART synchronous demo");

    if(bspUartInit((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS]) != ERR_STS_OK)
    {
        LOGE("UART init failed");
        osThreadExit();
    }

    while(1)
    {
        /* Send data synchronously */
        if(bspUartSendSync((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS], (const uint8_t*)txMsg,
                           strlen(txMsg), 1000U) == ERR_STS_OK)
        {
            LOGI("TX complete");
        }
        else
        {
            LOGW("TX timeout/error");
        }

        /* Receive data synchronously */
        if(bspUartReceiveSync((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS], rxBuf, 32, 2000U) == ERR_STS_OK)
        {
            LOGI("RX complete");
            LOGD("RX data: %.*s", 32, rxBuf);
        }
        else
        {
            LOGW("RX timeout");
        }

        osDelay(1000U);
    }
}

void AppUartSyncDemoStart(void)
{
    const osThreadAttr_t attr = { .name       = "UartSyncTask",
                                  .priority   = osPriorityNormal,
                                  .stack_size = 4096U };

    (void)osThreadNew(UartSyncTask, NULL, &attr);
}
