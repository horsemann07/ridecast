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
#include <stdio.h>

#include "app_bsp_config.h"

#include "cmsis_os2.h"
#include "bsp_uart.h"
#include "bsp_err_sts.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #ifdef ESP_BOARD_LOGGING // ESP-IDF logging
    #include "esp_log.h"
#ifndef __FILENAME__
#define __FILENAME__   (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)

// #else // BSP logging: just map directly
//     #include "bsp_log.h"
//     #define LOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
//     #define LOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
//     #define LOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
//     #define LOGD(fmt, ...) BSP_LOGD(fmt, ##__VA_ARGS__)
// #endif


/* UART configuration */
#define UART_TASK_STACK_SIZE 1024
#define UART_TASK_PRIORITY   osPriorityNormal

#define DEMO_UART_PORT       2
#define DEMO_UART_TX_PIN     17
#define DEMO_UART_RX_PIN     16

static osThreadId_t uartTaskHandle;

/* UART task */
static void UartLoopbackTask(void* argument)
{
    (void)argument;

    LOGI("UART task started\n");

    bsp_err_sts_t ret;

    uint8_t txData[] = "CMSIS UART LOOPBACK";
    uint8_t rxData[64];
    size_t rxLen = 0;

    bspUartHandle_t uart = { .portNum    = DEMO_UART_PORT,
                             .baudrate   = BSP_UART_BAUD_115200,
                             .wordLength = eBspUartWordLength8,
                             .parity     = eBspUartParityNone,
                             .stopBits   = eBspUartStopBitsOne,
                             .mode       = eBspUartModeTxRx,

                             .uartTxPin  = DEMO_UART_TX_PIN,
                             .uartRxPin  = DEMO_UART_RX_PIN,
                             .uartRtsPin = 0,
                             .uartCtsPin = 0,

                             .fifoSize     = 128,
                             .oversampling = 16,
                             .rxThreshold  = 1,

                             .invertTx        = 0,
                             .invertRx        = 0,
                             .dmaEnable       = 0,
                             .hwFlowControlEn = false };

    /* Initialize UART */
    ret = bspUartInit(&uart);
    if(ret != BSP_ERR_STS_OK)
    {
        LOGI("UART init failed: %s\n", bsp_err_sts_to_str(ret));
        // osThreadExit();
        vTaskDelete(NULL);
    }


    for(;;)
    {
        memset(rxData, 0, sizeof(rxData));
        rxLen = 0;

        /* Transmit */
        ret = bspUartSendSync(&uart, txData, strlen((char*)txData), 1000);

        if(ret != BSP_ERR_STS_OK)
        {
            LOGE("TX failed: %s\n", bsp_err_sts_to_str(ret));
        }
        else
        {
            /* Receive */
            ret = bspUartReceiveSync(&uart, rxData, sizeof(rxData), &rxLen, 1000);

            if(ret == BSP_ERR_STS_OK && rxLen == strlen((char*)txData) &&
               memcmp(txData, rxData, rxLen) == 0)
            {
                LOGI("Loopback OK: %s\n", rxData);
            }
            else
            {
                LOGE("Loopback FAIL (len=%d)\n", (int)rxLen);
            }
        }

        /* Run every 2 seconds */
        // osDelay(2000);
        /* Yield CPU cleanly */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void AppUartSyncDemoStart(void)
{
    const osThreadAttr_t attr = { .name       = "uart_loopback",
                                  .priority   = UART_TASK_PRIORITY,
                                  .stack_size = UART_TASK_STACK_SIZE };

    /* Create UART task */
    uartTaskHandle = osThreadNew(UartLoopbackTask,
                                 NULL,
                                 &attr);


    // xTaskCreate(UartLoopbackTask, "uart_loopback", UART_TASK_STACK_SIZE, NULL,
    //             UART_TASK_PRIORITY, NULL);
}
