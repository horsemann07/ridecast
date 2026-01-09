/**
 * @file app_config.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-30
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app_bsp_config.h"

const bspUartHandle_t g_bspUartHandles[] = {
    /* console configuration */
    {
    .baudrate        = BSP_UART_BAUD_115200,
    .uartRxPin       = 16,
    .uartTxPin       = 17,
    .uartRtsPin      = PIN_UNUSED,
    .uartCtsPin      = PIN_UNUSED,
    .fifoSize        = 1024,
    .portNum         = 2,
    .wordLength      = eBspUartWordLength8,
    .parity          = eBspUartParityNone,
    .stopBits        = eBspUartStopBitsOne,
    .hwFlowControlEn = false,
    .mode            = eBspUartModeTxRx,
    .oversampling    = 16,
    .invertTx        = DISABLE,
    .invertRx        = DISABLE,
    .dmaEnable       = DISABLE,
    .rxThreshold     = eBspUartWordLength8,
    .uartOwner       = BSP_UART_OWNER_CONSOLE,
    },

    /* send gnss data configuration */
    {
    .baudrate        = BSP_UART_BAUD_115200,
    .uartRxPin       = 16,
    .uartTxPin       = 17,
    .uartRtsPin      = PIN_UNUSED,
    .uartCtsPin      = PIN_UNUSED,
    .fifoSize        = 128,
    .portNum         = 0,
    .wordLength      = eBspUartWordLength8,
    .parity          = eBspUartParityNone,
    .stopBits        = eBspUartStopBitsOne,
    .hwFlowControlEn = false,
    .mode            = eBspUartModeTxRx,
    .oversampling    = 16,
    .invertTx        = DISABLE,
    .invertRx        = DISABLE,
    .dmaEnable       = DISABLE,
    .rxThreshold     = eBspUartWordLength8,
    .uartOwner       = BSP_UART_OWNER_GNSS,
    },


};
