

#include "app_config.h"


// clang-format off


/* =========================================================================
 * UART CONFIGURATION
 * =========================================================================
 *
 * Host UART communicates with the vehicle cluster.
 *
 * GPIO assignments below are the original ESP32 assignments.
 * Validate them against the actual ESP32 module and board schematic.
 *
 * The handle must remain writable and at a stable address.
 * ========================================================================= */
bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX] =
{
    [APP_UART_OWNER_HOST_COMM] =
    {
        .config =
        {
            .port         = eBspUartPort2,
            .owner        = (bspUartOwner_t)APP_UART_OWNER_HOST_COMM,

            .baudrate     = BSP_UART_BAUD_115200,
            .wordLength   = eBspUartWordLength8,
            .parity       = eBspUartParityNone,
            .stopBits     = eBspUartStopBitsOne,
            .mode         = eBspUartModeTxRx,

            .uartRxPin    = 16U,
            .uartTxPin    = 17U,
            .uartRtsPin   = BSP_UART_PIN_NO_CHANGE,
            .uartCtsPin   = BSP_UART_PIN_NO_CHANGE,

            .fifoSize     = configHOST_UART_RXTX_BUF_SIZE,
            .rxThreshold  = 8U,

            .flowControl  = eBspUartFlowControlNone,
            .dmaMode      = eBspUartDmaDisabled,

            .oversampling = 16U,
            .invertTx     = false,
            .invertRx     = false
        }
    }
};