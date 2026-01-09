

/*
===============================================================================
BSP UART ASYNC ARCHITECTURE (ESP32 + ESP-IDF + CMSIS-RTOS2)
===============================================================================

1. One UART driver instance per port (ESP-IDF).
2. One FreeRTOS queue per UART port:
   - Required by ESP-IDF uart_driver_install().
   - Used only inside BSP (not exposed).

3. One shared CMSIS-RTOS2 thread:
   - Waits on a CMSIS EventFlag.
   - Processes UART events for ALL ports.
   - No polling, no busy loops.

4. RX behavior:
   - RX is always enabled once async is enabled.
   - When data arrives, ESP-IDF pushes an event into the UART queue.
   - EventFlag wakes the shared task.
   - Task drains the queue and invokes the registered callback (if any).

5. TX behavior:
   - ESP-IDF has no TX-complete interrupt.
   - TX async is "fire-and-notify":
       - uart_write_bytes()
       - optional callback invoked immediately.

6. Callback model:
   - One callback per UART port.
   - Same callback used for RX and TX.
   - If no callback is registered, BSP does nothing.

7. RTOS usage:
   - CMSIS-RTOS2 for threads, delays, event flags.
   - FreeRTOS queue ONLY where ESP-IDF requires it.

This design is:
✔ Event-driven (no polling)
✔ ESP-IDF compliant
✔ CMSIS-friendly
✔ Scalable
✔ BSP-grade

UART interrupt (hardware)
   ↓
ESP-IDF ISR
   ↓
uart_event_t pushed to FreeRTOS queue   ← automatic
   ↓
FreeRTOS notifier task (BSP code)       ← YOU add this
   ↓
osEventFlagsSet(UART_EVT_WAKE)           ← YOU call this
   ↓
CMSIS UART event thread wakes
   ↓
CMSIS thread drains queues
   ↓
User callback invoked

===============================================================================
*/


// Include necessary headers
#include <string.h>

/* BSP */
#include "bsp_uart.h"
#include "bsp_config.h"

/* CMSIS */
#include "cmsis_os2.h"

/* ESP-IDF */
#include "driver/uart.h"

#ifdef ESP_BOARD_LOGGING
    #include "esp_log.h"
    #define __FILENAME__        (strrchr("/" __FILE__, '/') + 1)
    #define UART_LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
    #define UART_LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
    #define UART_LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#else
    #include "cli_logger.h"
    #define UART_LOGI(fmt, ...) BSP_LOGI(fmt, ##__VA_ARGS__)
    #define UART_LOGW(fmt, ...) BSP_LOGW(fmt, ##__VA_ARGS__)
    #define UART_LOGE(fmt, ...) BSP_LOGE(fmt, ##__VA_ARGS__)
#endif

#define UART_EVENT_QUEUE_SIZE      20
#define UART_EVENT_TASK_STACK_SIZE 2048

typedef struct
{
    QueueHandle_t evtQ;        /* ESP-IDF UART event queue */
    bspUartCallback_t cbFn;    /* registered callback */
    void* cbCtx;               /* user callback context */
    bspUartASyncCtx_t rxTxCtx; /* RX/TX shared context */
} uartRt_t;

static uartRt_t s_uartRt[UART_NUM_MAX];

#define UART_EVT_WAKE (1U << 0)

/* One QueueSet for all UART event queues (FreeRTOS only) */
static QueueSetHandle_t s_uartQueueSet;

/* CMSIS threads */
static osThreadId_t s_uartNotifyTask;
static osThreadId_t s_uartEvtThread;

/* CMSIS sync primitive */
static osEventFlagsId_t s_uartEvtFlags;

static const osThreadAttr_t uartEvtAttr = { .name       = "uart_evt",
                                            .priority   = osPriorityNormal,
                                            .stack_size = 2048 };

/*
 **************************************************************
 *      Local Functions
 **************************************************************
 */

static void uartNotifyTask(void* arg)
{
    QueueSetMemberHandle_t activeMember;
    uart_event_t ev;

    (void)arg;

    UART_LOGI("UART QueueSet notifier started");

    for(;;)
    {
        /* Block until ANY UART queue has data */
        activeMember = xQueueSelectFromSet(s_uartQueueSet, portMAX_DELAY);

        if(activeMember == NULL)
        {
            continue;
        }

        /* Receive exactly one event from the active queue */
        if(xQueueReceive(activeMember, &ev, 0) == pdTRUE)
        {
            /* Put event back so CMSIS task can process it */
            xQueueSendToFront(activeMember, &ev, 0);

            /* Wake CMSIS UART event thread */
            osEventFlagsSet(s_uartEvtFlags, UART_EVT_WAKE);
        }
    }
}


static void uartEvtThread(void* arg)
{
    uart_event_t ev;
    (void)arg;

    UART_LOGI("UART async event thread started");

    for(;;)
    {
        /* Sleep until ANY UART signals activity */
        osEventFlagsWait(s_uartEvtFlags, UART_EVT_WAKE, osFlagsWaitAny, osWaitForever);

        osEventFlagsClear(s_uartEvtFlags, UART_EVT_WAKE);

        for(uint8_t port = 0; port < UART_NUM_MAX; port++)
        {
            uartRt_t* rt = &s_uartRt[port];

            if(rt->evtQ == NULL)
                continue;

            while(xQueueReceive(rt->evtQ, &ev, 0) == pdTRUE)
            {
                if((ev.type == UART_DATA) && (rt->cbFn != NULL))
                {
                    int rd = uart_read_bytes(port, rt->rxTxCtx.data,
                                             BSP_UART_RXTX_BUFFER_SIZE, 0);

                    if(rd > 0)
                    {
                        rt->rxTxCtx.data_len  = (uint16_t)rd;
                        rt->rxTxCtx.uart_port = port;

                        rt->cbFn(ERR_STS_OK, &rt->rxTxCtx, rt->cbCtx);
                    }
                }
                else if((ev.type == UART_FIFO_OVF) || (ev.type == UART_BUFFER_FULL))
                {
                    uart_flush_input(port);
                    xQueueReset(rt->evtQ);
                }
            }
        }
    }
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
        return ERR_STS_INVALID_PARAM;
    }

    // Map to ESP-IDF UART port number
    int uart_num = ptHandle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INVALID_PARAM;
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
        .rx_flow_ctrl_thresh = ptHandle->rxThreshold,
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

    int fifo_size = ptHandle->fifoSize ? ptHandle->fifoSize : BSP_UART_RXTX_BUFFER_SIZE;

    memset(&s_uartRt[uart_num], 0, sizeof(uartRt_t));

#if BSP_UART_ASYNC_EN
    /* Create QueueSet once */
    if(s_uartQueueSet == NULL)
    {
        /* Size = sum of all queue lengths */
        s_uartQueueSet = xQueueCreateSet(UART_NUM_MAX * UART_EVENT_QUEUE_SIZE);
        if(s_uartRt[uart_num].evtQ == NULL)
        {
            UART_LOGE("Failed to create UART event queue");
            return ERR_STS_FAIL;
        }
    }

    /* Install UART driver with this queue */
    uart_driver_install(uart_num, BSP_UART_RXTX_BUFFER_SIZE, BSP_UART_RXTX_BUFFER_SIZE,
                        UART_EVENT_QUEUE_SIZE, &s_uartRt[uart_num].evtQ, 0);


    if(s_uartEvtFlags == NULL)
    {
        s_uartEvtFlags = osEventFlagsNew(NULL);
    }

    if(s_uartNotifyTask == NULL)
    {
        static const osThreadAttr_t uartNotifyAttr = { .name = "uart_notify",
                                                       .priority = osPriorityAboveNormal,
                                                       .stack_size = 1024 };

        if(s_uartNotifyTask == NULL)
        {
            s_uartNotifyTask = osThreadNew(uartNotifyTask, NULL, &uartNotifyAttr);
        }
    }
#else
    uart_driver_install(uart_num, fifo_size, fifo_size, 0, NULL, 0);
#endif

    UART_LOGI("UART%d initialized: %d baud, %d bits, parity %d, stop %d, flow "
              "%d",
              uart_num, ptHandle->baudrate, ptHandle->wordLength,
              ptHandle->parity, ptHandle->stopBits, ptHandle->hwFlowControlEn);
    return ERR_STS_OK;
}


errStatus_t bspUartDeInit(bspUartHandle_t* ptHandle)
{
    uint8_t uart_num;
    esp_err_t err;

    if(ptHandle == NULL)
    {
        UART_LOGE("UART handle is NULL");
        return ERR_STS_INVALID_PARAM;
    }

    uart_num = ptHandle->portNum;

    if(uart_num >= UART_NUM_MAX)
    {
        UART_LOGE("Invalid UART port number: %d", uart_num);
        return ERR_STS_INVALID_PARAM;
    }

    UART_LOGI("De-initializing UART%d", uart_num);

    /* -------------------------------------------------
     * 1. Uninstall ESP-IDF UART driver
     * ------------------------------------------------- */
    err = uart_driver_delete(uart_num);
    if(err != ESP_OK)
    {
        UART_LOGE("uart_driver_delete failed: %d", err);
        return ERR_STS_FAIL;
    }

    /* -------------------------------------------------
     * 2. Remove UART queue from QueueSet
     * ------------------------------------------------- */
    if((s_uartQueueSet != NULL) && (s_uartRt[uart_num].evtQ != NULL))
    {
        xQueueRemoveFromSet(s_uartRt[uart_num].evtQ, s_uartQueueSet);
    }

    /* -------------------------------------------------
     * 3. Delete UART event queue (FreeRTOS)
     * ------------------------------------------------- */
    if(s_uartRt[uart_num].evtQ != NULL)
    {
        vQueueDelete(s_uartRt[uart_num].evtQ);
        s_uartRt[uart_num].evtQ = NULL;
    }

    /* -------------------------------------------------
     * 4. Clear callback and runtime context
     * ------------------------------------------------- */
    s_uartRt[uart_num].cbFn  = NULL;
    s_uartRt[uart_num].cbCtx = NULL;
    memset(&s_uartRt[uart_num].rxTxCtx, 0, sizeof(bspUartASyncCtx_t));

    UART_LOGI("UART%d de-initialized successfully", uart_num);
    return ERR_STS_OK;
}

errStatus_t bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms)
{
    if(handle == NULL || data == NULL || length == 0)
    {
        UART_LOGE("Invalid parameters for bspUartSendSync");
        return ERR_STS_INVALID_PARAM;
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
        return ERR_STS_INVALID_PARAM;
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

errStatus_t bspUartRegisterCb(uint8_t port, bspUartCallback_t cb, void* userCtx)
{
    if(port >= UART_NUM_MAX)
    {
        return ERR_STS_INVALID_PARAM;
    }

    /* Create per-UART event queue */
    s_uartRt[port].evtQ = xQueueCreate(UART_EVENT_QUEUE_SIZE, sizeof(uart_event_t));
    if(s_uartRt[port].evtQ == NULL)
    {
        UART_LOGE("Failed to create UART event queue");
        return ERR_STS_FAIL;
    }

    /* Add UART queue to QueueSet */
    if(s_uartQueueSet != NULL && xQueueAddToSet(s_uartRt[port].evtQ, s_uartQueueSet) != pdPASS)
    {
        UART_LOGE("Failed to add UART event queue to QueueSet");
        osMessageQueueDelete(s_uartRt[port].evtQ);
        s_uartRt[port].evtQ = NULL;
        return ERR_STS_FAIL;
    }

    /* Install UART driver with this queue */
    uart_driver_install(port, BSP_UART_RXTX_BUFFER_SIZE, BSP_UART_RXTX_BUFFER_SIZE,
                        UART_EVENT_QUEUE_SIZE, &s_uartRt[port].evtQ, 0);

    s_uartRt[port].cbFn  = cb;
    s_uartRt[port].cbCtx = userCtx;
    return ERR_STS_OK;
}


errStatus_t bspUartSendAsync(bspUartHandle_t* handle,
                             const uint8_t* data,
                             size_t length,
                             bspUartCallback_t callback,
                             void* userContext)
{
    // ESP-IDF has no TX-done interrupt
    UART_LOGE("bspUartSendAsync not supported on ESP32");
    return ERR_STS_FUNC_NOT_SUPPORTED;
}
