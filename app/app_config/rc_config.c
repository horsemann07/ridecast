#include "rc_config.h"
#include "bsp_os_map.h"
#include "bsp_core.h"

#include "rc_wifi.h"
#include "rc_ble.h"
#include "rc_bt_classic.h"

// clang-format off


/* =========================================================================
 * THREAD CONFIGURATION
 * ========================================================================= */

os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX] =
{
    [APP_THREAD_OWNER_STARTUP] =
    {
        .func = vAppStartupTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "Startup",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },

    [APP_THREAD_OWNER_UART_APP_TX] =
    {
        .func = vAppUartTxTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "UartAppTx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },

    [APP_THREAD_OWNER_UART_APP_RX] =
    {
        .func = vAppUartRxTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "UartAppRx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },

    [APP_THREAD_OWNER_RC_WIFI] =
    {
        .func = vRcWifiTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "RcWifi",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },

    [APP_THREAD_OWNER_RC_BLE] =
    {
        .func = vRcBleTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "RcBle",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        },
        .handle = NULL
    },

    [APP_THREAD_OWNER_RC_BT_CLASSIC] =
    {
        .func = vRcBtClassicTask,
        .arg  = NULL,
        .attr =
        {
            .name       = "RcBtClassic",
            .stack_size = THREAD_STACK_3K,
            .priority = osPriorityNormal
        },
        .handle = NULL
    }
};


/* =========================================================================
 * QUEUE CONFIGURATION
 * ========================================================================= */

os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX] =
{
    [APP_Q_OWNER_RC_UART_RX] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(rc_uart_rx_msg_t),
        .attr =
        {
            .name = "uartAppRxQ"
        }
    },

    [APP_Q_OWNER_RC_UART_TX] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(rc_uart_tx_msg_t),
        .attr =
        {
            .name = "uartAppTxQ"
        }
    },

    [APP_Q_OWNER_RC_WIFI] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(rc_wifi_msg_t),
        .attr =
        {
            .name = "rcWifiQ"
        }
    },

    [APP_Q_OWNER_RC_BLE] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(rc_ble_msg_t),
        .attr =
        {
            .name = "rcBleQ"
        }
    },

    [APP_Q_OWNER_RC_BT_CLASSIC] =
    {
        .msg_count      = 10U,
        .msg_size_bytes = sizeof(rc_bt_classic_msg_t),
        .attr =
        {
            .name = "rcBtClassicQ"
        }
    }
};


/* =========================================================================
 * MUTEX CONFIGURATION
 * ========================================================================= */

os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX] =
{
    [APP_MUTEX_OWNER_UART_APP_TX] =
    {
        .attr =
        {
            .name      = "uartAppTxMtx",
            .attr_bits = osMutexPrioInherit
        }
    },

    [APP_MUTEX_OWNER_UART_APP_RX] =
    {
        .attr =
        {
            .name      = "uartAppRxMtx",
            .attr_bits = osMutexPrioInherit
        }
    },

    [APP_MUTEX_OWNER_RC_CONNECTION] =
    {
        .attr =
        {
            .name      = "rcConnectionMtx",
            .attr_bits = osMutexPrioInherit
        }
    }
};


/* =========================================================================
 * SEMAPHORE CONFIGURATION
 * ========================================================================= */

os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX] =
{
};


/* =========================================================================
 * EVENT FLAGS CONFIGURATION
 * ========================================================================= */

os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX] =
{
};


/* =========================================================================
 * UART CONFIGURATION
 * ========================================================================= */

const bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX] =
{
    [APP_UART_OWNER_HOST_COMM] =
    {
        .baudrate        = BSP_UART_BAUD_115200,
        .uartRxPin       = 16,
        .uartTxPin       = 17,
        .uartRtsPin      = GPIO_PIN_UNUSED,
        .uartCtsPin      = GPIO_PIN_UNUSED,
        .fifoSize        = appCfg_HOST_UART_RXTX_BUF_SIZE,
        .portNum         = eBspUartPort2,
        .wordLength      = eBspUartWordLength8,
        .parity          = eBspUartParityNone,
        .stopBits        = eBspUartStopBitsOne,
        .hwFlowControlEn = false,
        .mode            = eBspUartModeTxRx,
        .oversampling    = 16,
        .invertTx        = BSP_DISABLE,
        .invertRx        = BSP_DISABLE,
        .dmaEnable       = BSP_DISABLE,
        .rxThreshold     = eBspUartWordLength8,
        .uartOwner       = APP_UART_OWNER_HOST_COMM,
    }
};


/**********************************************************************
 * FILE END
 **********************************************************************/