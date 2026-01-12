

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

typedef enum {
    UART_RX_IDLE = 0,
    UART_RX_ACTIVE,
} uartRxState_t;

typedef struct
{
    uartRxState_t state;

    uint8_t* rxBuf;
    size_t   rxExpected;
    size_t   rxReceived;

    bspUartCallback_t cbFn;
    void* cbCtx;

    QueueHandle_t evtQ;
} uartAsyncRt_t;


static uartAsyncRt_t s_uartRt[UART_NUM_MAX];

/* CMSIS threads */
static osThreadId_t s_uartEvtThread;

static const osThreadAttr_t uartEvtAttr = { .name       = "uart_evt",
                                            .priority   = osPriorityNormal,
                                            .stack_size = 2048 };
/* ========================================================= */
/* UART EVENT TASK                                           */
/* ========================================================= */
static void uartEventTask(void* arg)
{
    uart_event_t ev;
    (void)arg;

    for (;;)
    {
        for (uint8_t port = 0; port < UART_NUM_MAX; port++)
        {
            uartAsyncRt_t* rt = &s_uartRt[port];
            if (rt->evtQ == NULL)
                continue;

            if (xQueueReceive(rt->evtQ, &ev, BSP_UART_POLLING_DELAY_MS) != pdTRUE)
                continue;

            /* RX data event */
            if (ev.type == UART_DATA && rt->state == UART_RX_ACTIVE)
            {
                int rd = uart_read_bytes(
                    port,
                    rt->rxBuf + rt->rxReceived,
                    rt->rxExpected - rt->rxReceived,
                    0);

                if (rd > 0)
                {
                    rt->rxReceived += rd;

                    /* Completion condition */
                    if (rt->rxReceived >= rt->rxExpected)
                    {
                        rt->state    = UART_RX_IDLE;
                        rt->rxStatus = ERR_STS_OK;

                        memcpy(ctx.data, rt->rxBuf, rt->rxReceived);
                        rt->cbFn(ERR_STS_OK, &ctx, rt->cbCtx);
                    }
                }
            }
            /* RX error */
            else if (ev.type == UART_FIFO_OVF ||
                     ev.type == UART_BUFFER_FULL)
            {
                uart_flush_input(port);

                rt->state    = UART_RX_IDLE;
                rt->rxStatus = ERR_STS_FAIL;

                if (rt->cbFn)
                    rt->cbFn(ERR_STS_FAIL, NULL, rt->cbCtx);
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

    memset(&s_uartRt[uart_num], 0, sizeof(uartAsyncRt_t));

    uartAsyncRt_t* rt = &s_uartRt[ptHandle->portNum];
    memset(rt, 0, sizeof(*rt));

    uart_driver_install(
        h->portNum,
        BSP_UART_RXTX_BUFFER_SIZE,
        BSP_UART_RXTX_BUFFER_SIZE,
        UART_EVT_QUEUE_LEN,
        &rt->evtQ,
        0);

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
     * 3. Delete UART event queue (FreeRTOS)
     * ------------------------------------------------- */
    if(s_uartRt[uart_num].evtQ != NULL)
    {
        osMessageQueueDelete(s_uartRt[uart_num].evtQ);
        s_uartRt[uart_num].evtQ = NULL;
    }

    /* -------------------------------------------------
     * 4. Clear callback and runtime context
     * ------------------------------------------------- */
    s_uartRt[uart_num].cbFn  = NULL;
    s_uartRt[uart_num].cbCtx = NULL;
    memset(&s_uartRt[uart_num], 0, sizeof(uartAsyncRt_t));

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
/* -------------------------------------------------- */
/* CALLBACK REGISTRATION                              */
/* -------------------------------------------------- */
errStatus_t bspUartSetCallback(bspUartHandle_t* handle,
                               bspUartCallback_t callback,
                               void* userContext)
{
    if (!handle || !callback)
        return ERR_STS_INVALID_PARAM;

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];
    rt->cbFn    = callback;
    rt->cbCtx = userContext;
    return ERR_STS_OK;
}

/* -------------------------------------------------- */
/* ASYNC READ        */
/* -------------------------------------------------- */
errStatus_t bspUartReadAsync(bspUartHandle_t* handle,
                             uint8_t* buffer,
                             size_t length)
{
    if (!handle || !buffer || length == 0)
        return ERR_STS_INVALID_PARAM;

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];

    if (rt->state != UART_RX_IDLE)
        return ERR_STS_BUSY;

    if (rt->cbFn == NULL)
        return ERR_STS_FAIL; /* callback not registered */

    rt->rxBuf      = buffer;
    rt->rxExpected = length;
    rt->rxReceived = 0;
    rt->state      = UART_RX_ACTIVE;

    return ERR_STS_OK;
}

errStatus_t bspUartWriteAsync(bspUartHandle_t* handle,
                              const uint8_t* buffer,
                              size_t length)
{
    if (!handle || !buffer || length == 0)
        return ERR_STS_INVALID_PARAM;

    int uart_num = handle->portNum;

    int written = uart_write_bytes(uart_num,
                                   (const char*)buffer,
                                   length);
    if (written < 0)
        return ERR_STS_FAIL;

    return ERR_STS_OK;
}



errStatus_t bspUartIoctl(bspUartHandle_t* handle,
                          bspUartIoctlRequest_t req,
                          void* arg)
{
    if (!handle)
        return ERR_STS_INVALID_PARAM;

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];

    switch (req)
    {
        case eBspUartGetRxCount:
            if (!arg)
                return ERR_STS_INVALID_PARAM;
            *(size_t*)arg = rt->rxReceived;
            return ERR_STS_OK;

        case eBspUartIsRxBusy:
            if (!arg)
                return ERR_STS_INVALID_PARAM;
            *(bool*)arg = (rt->state == UART_RX_ACTIVE);
            return ERR_STS_OK;

        case eBspUartCancelRx:
            if (rt->state == UART_RX_ACTIVE)
            {
                rt->state    = UART_RX_IDLE;
                rt->rxStatus = ERR_STS_FAIL;

                if (rt->cbFn)
                    rt->cbFn(ERR_STS_FAIL, NULL, rt->cbCtx);
            }
            return ERR_STS_OK;

        default:
            return ERR_STS_INVALID_PARAM;
    }
}