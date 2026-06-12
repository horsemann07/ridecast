
#include "app_config.h"
#include "bsp_os_map.h"
#include "bsp_core.h"

// clang-format off
/*
 * ------------------------------------------------------------------------
 */
os_thread_map_t g_appThreadCfg[APP_THREAD_OWNER_MAX] = {

    /* --------------------------------------------------------
     *        APPLICATION THREAD CONFIGURATION
     * -------------------------------------------------------- */
    [APP_THREAD_OWNER_STARTUP] = {
        .attr = {
            .name       = "Startup",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    },
    [APP_THREAD_OWNER_UART_APP_TX] = {
        .attr = {
            .name       = "UartAppTx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    },

    [APP_THREAD_OWNER_UART_APP_RX] = {
        .attr = {
            .name       = "UartAppRx",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    },

    [APP_THREAD_OWNER_WIFI_APP] = {
        .attr = {
            .name       = "WifiApp",
            .stack_size = THREAD_STACK_3K,
            .priority   = osPriorityNormal
        }
    }
};

/*
 * ------------------------------------------------------------------------
 */

os_queue_map_t g_appQueueCfg[APP_Q_OWNER_MAX] = {
    [APP_Q_OWNER_UART_APP_RX] = {
        .msg_count      = 10U,
        .msg_size_bytes = 0U, /* Run time analysis */
        .attr = {
            .name = "uartAppRxQ"
        }
    },

    [APP_Q_OWNER_UART_APP_TX] = {
        .msg_count      = 10U,
        .msg_size_bytes = 0U, /* Run time analysis */
        .attr = {
            .name = "uartAppTxQ"
        }
    },

    [APP_Q_OWNER_WIFI_APP] = {
        .msg_count      = 10U,
        .msg_size_bytes = 0U, /* Run time analysis */
        .attr = {
            .name = "wifiAppQ"
        }
    }

};


/*
 * ------------------------------------------------------------------------
 */

os_mutex_map_t g_appMutexCfg[APP_MUTEX_OWNER_MAX] = {
    [APP_MUTEX_OWNER_UART_APP_TX] = {
        .attr = {
            .name      = "uartAppTxMtx",
            .attr_bits = osMutexPrioInherit
        }
    },

    [APP_MUTEX_OWNER_UART_APP_RX] = {
        .attr = {
            .name      = "uartAppRxMtx",
            .attr_bits = osMutexPrioInherit
        }
    }
};

/*
 * ------------------------------------------------------------------------
 */

os_semaphore_map_t g_appSemaphoreCfg[APP_SEMAPHORE_OWNER_MAX] = {
};

/*
 * ------------------------------------------------------------------------
 */

os_event_flags_map_t g_appEventFlagsCfg[APP_EVENT_FLAGS_OWNER_MAX] = {
};

const bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX] = {
    [APP_UART_OWNER_HOST_COMM] = {
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
    },
};
/**********************************************************************
 * FILE END
 ***********************************************************************/