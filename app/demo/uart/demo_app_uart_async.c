/**
 * @file demo_app_uart_async.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-30
 *
 * @copyright Copyright (c) 2025
 *
 */


#include <string.h>

#include "cmsis_os2.h"

#include "bsp_uart.h"
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


typedef struct
{
    uint8_t* rxBuf; /**< RX buffer pointer */
    size_t bufSize; /**< RX buffer size */
} UartRxContext_t;


static void UartRxCallback(errStatus_t status, void* userContext)
{
    const UartRxContext_t* ctx = (const UartRxContext_t*)userContext;

    if((status == ERR_STS_OK) && (ctx != NULL) && (ctx->rxBuf != NULL))
    {
        /* Ensure safe string print (if text-based) */
        ctx->rxBuf[ctx->bufSize - 1U] = '\0';

        LOGI("Async RX (%u bytes): %s", (unsigned int)ctx->bufSize, (char*)ctx->rxBuf);
    }
    else
    {
        LOGW("Async RX failed (status=%d)", status);
    }
}

static void UartTxCallback(errStatus_t status, void* userContext)
{
    (void)userContext;
    LOGI("Async TX complete (status=%d)", status);
}

static void UartAsyncTask(void* argument)
{
    (void)argument;

    static uint8_t rxBuf[32];
    static const char txMsg[] = "UART Async Test\r\n";

    /* Initialize UART */
    if(bspUartInit((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS]) != ERR_STS_OK)
    {
        LOGE("UART init failed");
        osThreadExit();
    }

    static uint8_t rxBuf[64];
    static UartRxContext_t rxCtx = { .rxBuf = rxBuf, .bufSize = sizeof(rxBuf) };

    LOGI("Starting UART async RX demo");

    while(1)
    {
        /* Async TX */
        (void)bspUartSendAsync((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS],
                               (const uint8_t*)txMsg, strlen(txMsg), UartTxCallback, NULL);

        /* Async RX */
        (void)bspUartReceiveAsync((bspUartHandle_t*)&g_bspUartHandles[BSP_UART_OWNER_GNSS],
                                  rxBuf, sizeof(rxBuf), UartRxCallback, &rxCtx);

        osDelay(2000U);
    }
}

void AppUartAsyncDemoStart(void)
{
    const osThreadAttr_t attr = { .name       = "UartAsyncTask",
                                  .priority   = osPriorityBelowNormal,
                                  .stack_size = 4096U };

    (void)osThreadNew(UartAsyncTask, NULL, &attr);
}
