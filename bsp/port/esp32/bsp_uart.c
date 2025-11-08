
// Include necessary headers
#include <string.h>

// bsp includes
#include "bsp_config.h"
#include "bsp_uart.h"

// cmsis includes
#include "cmsis_os2.h"

// esp idf includes
#include "driver/uart.h"

#ifdef ESP_BOARD_LOGGING // ESP-IDF logging
    #include "esp_log.h"

    #define __FILENAME__        (strrchr("/" __FILE__, '/') + 1)
    #define UART_LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
    #define UART_LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
    #define UART_LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
    #define UART_LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)

#else // BSP logging: just map directly
    #include "cli_logger.h"
    #define UART_LOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
    #define UART_LOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
    #define UART_LOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
    #define UART_LOGD(fmt, ...) BSP_LOGD(fmt, ##__VA_ARGS__)
#endif

#define UART_EVENT_QUEUE_SIZE      20
#define UART_EVENT_TASK_STACK_SIZE 2048

// Structure to hold async operation context
typedef struct
{
    uint8_t* data;
    size_t* length;
    bspUartCallback_t callback;
    void* userContext;
    bool active;
} uartAsyncContext_t;

// Structure for UART runtime data
typedef struct
{
    osMessageQueueId_t eventQueue;
    osThreadId_t eventTask;
    uartAsyncContext_t rxContext;
    uartAsyncContext_t txContext;
} uartRuntimeData_t;

// Static storage for runtime data (one per UART port)
static uartRuntimeData_t s_uartRuntime[UART_NUM_MAX] = { 0 };

/*
 **************************************************************
 *      Local Functions
 **************************************************************
 */

// UART event task - processes UART events and invokes callbacks
static void bsp_uart_event_task(void* pvParameters)
{
    int uart_num = (int)(intptr_t)pvParameters;
    uart_event_t event;
    uartRuntimeData_t* runtime = &s_uartRuntime[uart_num];

    UART_LOGI("BSP UART%d event task started", uart_num);

    for(;;)
    {
        // Wait for UART event using CMSIS-RTOS2 API
        if(osMessageQueueGet(runtime->eventQueue, &event, NULL, osWaitForever) == osOK)
        {
            switch(event.type)
            {
            case UART_DATA:
            {
                // RX data available
                if(runtime->rxContext.active)
                {
                    size_t available = 0;
                    uart_get_buffered_data_len(uart_num, &available);

                    size_t to_read = (available < *runtime->rxContext.length) ?
                                     available :
                                     *runtime->rxContext.length;

                    int bytes_read =
                    uart_read_bytes(uart_num, runtime->rxContext.data, to_read, 0);

                    if(bytes_read > 0)
                    {
                        *runtime->rxContext.length = (size_t)bytes_read;

                        // Call callback
                        if(runtime->rxContext.callback != NULL)
                        {
                            runtime->rxContext.callback(ERR_STS_OK,
                                                        runtime->rxContext.userContext);
                        }

                        runtime->rxContext.active = false;
                    }
                }
                break;
            }

            case UART_FIFO_OVF:
            {
                UART_LOGW("UART%d FIFO overflow", uart_num);
                uart_flush_input(uart_num);
                osMessageQueueReset(runtime->eventQueue);
                break;
            }

            case UART_BUFFER_FULL:
            {
                UART_LOGW("UART%d ring buffer full", uart_num);
                uart_flush_input(uart_num);
                osMessageQueueReset(runtime->eventQueue);
                break;
            }

            case UART_BREAK:
            {
                UART_LOGD("UART%d RX break", uart_num);
                break;
            }

            case UART_PARITY_ERR:
            {
                UART_LOGE("UART%d parity error", uart_num);
                if(runtime->rxContext.active && runtime->rxContext.callback != NULL)
                {
                    runtime->rxContext.callback(ERR_STS_FAIL, runtime->rxContext.userContext);
                    runtime->rxContext.active = false;
                }
                break;
            }

            case UART_FRAME_ERR:
            {
                UART_LOGE("UART%d frame error", uart_num);
                if(runtime->rxContext.active && runtime->rxContext.callback != NULL)
                {
                    runtime->rxContext.callback(ERR_STS_FAIL, runtime->rxContext.userContext);
                    runtime->rxContext.active = false;
                }
                break;
            }

            case UART_EVENT_MAX:
            {
                // TX done - handled by checking if TX context is active
                if(runtime->txContext.active)
                {
                    if(runtime->txContext.callback != NULL)
                    {
                        runtime->txContext.callback(ERR_STS_OK,
                                                    runtime->txContext.userContext);
                    }
                    runtime->txContext.active = false;
                }
                break;
            }

            default:
                UART_LOGW("UART%d unknown event type: %d", uart_num, event.type);
                break;
            }
        }
    }
}

// Helper function to initialize async context on-demand
static errStatus_t uart_init_async_context(int uart_num)
{
    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INV_PARAM;
    }

    // Already initialized
    if(s_uartRuntime[uart_num].eventQueue != NULL)
    {
        return ERR_STS_OK;
    }

    UART_LOGI("Initializing async context for UART%d", uart_num);

    // First, delete the driver (installed without event queue)
    uart_driver_delete(uart_num);

    // Create CMSIS message queue for UART events
    s_uartRuntime[uart_num].eventQueue =
    osMessageQueueNew(UART_EVENT_QUEUE_SIZE, sizeof(uart_event_t), NULL);
    if(s_uartRuntime[uart_num].eventQueue == NULL)
    {
        UART_LOGE("Failed to create UART event queue");
        return ERR_STS_FAIL;
    }

    // Reinstall UART driver with event queue
    QueueHandle_t native_queue = (QueueHandle_t)s_uartRuntime[uart_num].eventQueue;
    esp_err_t err =
    uart_driver_install(uart_num, 256, 256, UART_EVENT_QUEUE_SIZE, &native_queue, 0);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_driver_install with queue failed: %d", err);
        osMessageQueueDelete(s_uartRuntime[uart_num].eventQueue);
        s_uartRuntime[uart_num].eventQueue = NULL;
        return ERR_STS_FAIL;
    }

    // Create CMSIS thread for UART event handling
    osThreadAttr_t task_attr = { .name       = "bsp_uart_event_task",
                                 .stack_size = UART_EVENT_TASK_STACK_SIZE,
                                 .priority   = osPriorityNormal };

    s_uartRuntime[uart_num].eventTask =
    osThreadNew(bsp_uart_event_task, (void*)(intptr_t)uart_num, &task_attr);
    if(s_uartRuntime[uart_num].eventTask == NULL)
    {
        UART_LOGE("Failed to create UART event task");
        uart_driver_delete(uart_num);
        osMessageQueueDelete(s_uartRuntime[uart_num].eventQueue);
        s_uartRuntime[uart_num].eventQueue = NULL;
        return ERR_STS_FAIL;
    }

    UART_LOGI("UART%d async context initialized", uart_num);
    return ERR_STS_OK;
}

/*
 **************************************************************
 *      Global Functions
 **************************************************************
 */

errStatus_t bspUartInit(bspUartHandle_t* ptHandle)
{
    if(ptHandle == NULL)
    {
        UART_LOGE("UART handle is NULL");
        return ERR_STS_INV_PARAM;
    }

    // Map to ESP-IDF UART port number
    int uart_num = ptHandle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INV_PARAM;
    }

    // Map parity
    uart_parity_t parity = UART_PARITY_DISABLE;
    if(ptHandle->parity == eBspUartParityOdd)
    {
        parity = UART_PARITY_ODD;
    }
    else if(ptHandle->parity == eBspUartParityEven)
    {
        parity = UART_PARITY_EVEN;
    }

    // Map stop bits
    uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
    if(ptHandle->stopBits == eBspUartStopBitsTwo)
    {
        stop_bits = UART_STOP_BITS_2;
    }
    // Map word length
    uart_word_length_t word_len = UART_DATA_8_BITS;
    if(ptHandle->wordLength == 5)
    {
        word_len = UART_DATA_5_BITS;
    }
    else if(ptHandle->wordLength == 6)
    {
        word_len = UART_DATA_6_BITS;
    }
    else if(ptHandle->wordLength == 7)
    {
        word_len = UART_DATA_7_BITS;
    }
    else if(ptHandle->wordLength == 8)
    {
        word_len = UART_DATA_8_BITS;
    }

    // Map flow control
    uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    if(ptHandle->hwFlowControlEn)
    {
        flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    }

    uart_config_t uart_config = {
        .baud_rate           = ptHandle->baudrate,
        .data_bits           = word_len,
        .parity              = parity,
        .stop_bits           = stop_bits,
        .flow_ctrl           = flow_ctrl,
        .rx_flow_ctrl_thresh = ptHandle->rxThresold,
        .source_clk          = UART_SCLK_APB,
    };

    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_param_config failed: %d", err);
        return ERR_STS_FAIL;
    }

    // Set UART pins
    err = uart_set_pin(uart_num, ptHandle->uartTxPin, ptHandle->uartRxPin,
                       ptHandle->uartRtsPin, ptHandle->uartCtsPin);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_set_pin failed: %d", err);
        return ERR_STS_FAIL;
    }

    // Install UART driver WITHOUT event queue for sync-only operation
    int fifo_size = ptHandle->fifoSize ? ptHandle->fifoSize : 256;
    err = uart_driver_install(uart_num, fifo_size, fifo_size, 0, NULL, 0);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_driver_install failed: %d", err);
        return ERR_STS_FAIL;
    }

    // Initialize async contexts (will be set up on-demand)
    memset(&s_uartRuntime[uart_num], 0, sizeof(uartRuntimeData_t));

    UART_LOGI("UART%d initialized: %d baud, %d bits, parity %d, stop %d, flow "
              "%d",
              uart_num, ptHandle->baudrate, ptHandle->wordLength,
              ptHandle->parity, ptHandle->stopBits, ptHandle->hwFlowControlEn);
    return ERR_STS_OK;
}

errStatus_t bspUartDeInit(bspUartHandle_t* ptHandle)
{
    if(ptHandle == NULL)
    {
        UART_LOGE("UART handle is NULL");
        return ERR_STS_INV_PARAM;
    }

    int uart_num = ptHandle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INV_PARAM;
    }

    // Terminate event task
    if(s_uartRuntime[uart_num].eventTask != NULL)
    {
        osThreadTerminate(s_uartRuntime[uart_num].eventTask);
        s_uartRuntime[uart_num].eventTask = NULL;
    }

    // Uninstall UART driver
    esp_err_t err = uart_driver_delete(uart_num);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_driver_delete failed: %d", err);
        return ERR_STS_FAIL;
    }

    // Delete message queue
    if(s_uartRuntime[uart_num].eventQueue != NULL)
    {
        osMessageQueueDelete(s_uartRuntime[uart_num].eventQueue);
        s_uartRuntime[uart_num].eventQueue = NULL;
    }

    UART_LOGI("UART%d de-initialized", uart_num);
    return ERR_STS_OK;
}

errStatus_t bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms)
{
    if(handle == NULL || data == NULL || length == 0)
    {
        UART_LOGE("Invalid parameters for bspUartSendSync");
        return ERR_STS_INV_PARAM;
    }

    int uart_num = handle->portNum;

    // Send data
    int bytes_sent = uart_write_bytes(uart_num, (const char*)data, length);
    if(bytes_sent < 0)
    {
        UART_LOGE("uart_write_bytes failed");
        return ERR_STS_FAIL;
    }
    else if((size_t)bytes_sent != length)
    {
        UART_LOGW("uart_write_bytes sent %d of %d bytes", bytes_sent, length);
    }

    // Wait for transmission to complete
    esp_err_t err = uart_wait_tx_done(uart_num, timeout_ms / portTICK_PERIOD_MS);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_wait_tx_done failed: %d", err);
        return ERR_STS_FAIL;
    }

    return ERR_STS_OK;
}

errStatus_t bspUartReceiveSync(bspUartHandle_t* handle, uint8_t* data, size_t* length, uint32_t timeout_ms)
{
    if(handle == NULL || data == NULL || length == NULL)
    {
        UART_LOGE("Invalid parameters for bspUartReceiveSync");
        return ERR_STS_INV_PARAM;
    }

    int uart_num = handle->portNum;

    // Receive data
    int bytes_recv =
    uart_read_bytes(uart_num, data, *length, timeout_ms / portTICK_PERIOD_MS);
    if(bytes_recv < 0)
    {
        UART_LOGE("uart_read_bytes failed.");
        return ERR_STS_FAIL;
    }

    *length = (size_t)bytes_recv;
    return ERR_STS_OK;
}

errStatus_t bspUartSendAsync(bspUartHandle_t* handle,
                             const uint8_t* data,
                             size_t length,
                             bspUartCallback_t callback,
                             void* userContext)
{
    if(handle == NULL || data == NULL || length == 0)
    {
        UART_LOGE("Invalid parameters for bspUartSendAsync");
        return ERR_STS_INV_PARAM;
    }

    int uart_num = handle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INV_PARAM;
    }

    // Initialize async context on first use
    if(s_uartRuntime[uart_num].eventQueue == NULL)
    {
        errStatus_t status = uart_init_async_context(uart_num);
        if(status != ERR_STS_OK)
        {
            return status;
        }
    }

    // Check if a TX operation is already in progress
    if(s_uartRuntime[uart_num].txContext.active)
    {
        UART_LOGE("TX operation already in progress");
        return ERR_STS_BUSY;
    }

    // Store callback context
    s_uartRuntime[uart_num].txContext.callback    = callback;
    s_uartRuntime[uart_num].txContext.userContext = userContext;
    s_uartRuntime[uart_num].txContext.active      = true;

    // Send data
    int bytes_sent = uart_write_bytes(uart_num, (const char*)data, length);
    if(bytes_sent < 0 || (size_t)bytes_sent != length)
    {
        UART_LOGE("uart_write_bytes failed or incomplete");
        s_uartRuntime[uart_num].txContext.active = false;
        return ERR_STS_FAIL;
    }

    // The event task will handle the TX_DONE event and call the callback
    return ERR_STS_OK;
}

errStatus_t bspUartReceiveAsync(bspUartHandle_t* handle,
                                uint8_t* data,
                                size_t* length,
                                bspUartCallback_t callback,
                                void* userContext)
{
    if(handle == NULL || data == NULL || length == NULL)
    {
        UART_LOGE("Invalid parameters for bspUartReceiveAsync");
        return ERR_STS_INV_PARAM;
    }

    int uart_num = handle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INV_PARAM;
    }

    // Initialize async context on first use
    if(s_uartRuntime[uart_num].eventQueue == NULL)
    {
        errStatus_t status = uart_init_async_context(uart_num);
        if(status != ERR_STS_OK)
        {
            return status;
        }
    }

    // Check if an RX operation is already in progress
    if(s_uartRuntime[uart_num].rxContext.active)
    {
        UART_LOGE("RX operation already in progress");
        return ERR_STS_BUSY;
    }

    // Store async context
    s_uartRuntime[uart_num].rxContext.data        = data;
    s_uartRuntime[uart_num].rxContext.length      = length;
    s_uartRuntime[uart_num].rxContext.callback    = callback;
    s_uartRuntime[uart_num].rxContext.userContext = userContext;
    s_uartRuntime[uart_num].rxContext.active      = true;

    // Return immediately - event task will handle UART_DATA event
    return ERR_STS_OK;
}
