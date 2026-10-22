/**
 * @file bsp_uart.c
 * @brief BSP UART port layer for ESP32 (ESP-IDF + CMSIS-RTOS2).
 *
 * @par Architecture
 * - One UART driver instance per port (ESP-IDF).
 * - One CMSIS osMessageQueue per port for UART events.
 * - One shared CMSIS-RTOS2 thread processes events for ALL ports.
 * - No direct FreeRTOS API usage (only CMSIS-RTOS2 wrappers).
 *
 * @par RX Model
 * - User calls bspUartReadAsync() to arm a buffer.
 * - ESP-IDF pushes UART_DATA event into the queue.
 * - Shared event thread dequeues and reads bytes via ESP-IDF.
 * - User callback invoked with received data.
 * - User must re-arm by calling bspUartReadAsync() again in callback.
 *
 * @par TX Model
 * - bspUartWriteAsync() is fire-and-forget (ESP-IDF buffers internally).
 * - bspUartSendSync() blocks until TX FIFO is drained.
 *
 * @par Thread Safety
 * - Per-port CMSIS mutex protects runtime context.
 * - Callback is invoked with mutex released to prevent deadlock.
 *
 * @par Known Limitations
 * - ESP-IDF uart_driver_install() internally uses FreeRTOS queue.
 *   We wrap it via osMessageQueue handle cast (ESP-IDF provides the handle).
 * - Only one async RX operation per port at a time.
 *
 * @see bsp_uart.h
 */

#include <string.h>

/* BSP */
#include "bsp_uart.h"
#include "bsp_err_sts.h"

/* CMSIS-RTOS2 (only RTOS API used) */
#include "cmsis_os2.h"

/* ESP-IDF (hardware access only) */
#include "driver/uart.h"
#include "esp_log.h"

/* -------------------------------------------------- */
/* INTERNAL LOGGING (uses ESP-IDF log before BSP log  */
/* is available)                                      */
/* -------------------------------------------------- */

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define UART_LOGI(fmt, ...)       ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define UART_LOGW(fmt, ...)       ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define UART_LOGE(fmt, ...)       ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)

/* -------------------------------------------------- */
/* CONFIGURATION                                      */
/* -------------------------------------------------- */

/** @brief Maximum number of UART ports supported. */
#define BSP_UART_PORT_MAX         ((uint8_t)UART_NUM_MAX)

/** @brief Event queue depth per UART port. */
#define BSP_UART_EVT_QUEUE_DEPTH  (BSP_UART_ASYNC_EVNT_QUEUE_LEN)

/** @brief Mutex acquire timeout for runtime context access (ms). */
#define BSP_UART_MUTEX_TIMEOUT_MS (50U)

/** @brief Maximum time to wait for event in shared thread (ms). */
#define BSP_UART_EVT_POLL_MS      (10U)

/* -------------------------------------------------- */
/* INTERNAL TYPES                                     */
/* -------------------------------------------------- */

/**
 * @brief UART RX state machine states.
 */
typedef enum
{
    UART_RX_STATE_IDLE   = 0U, /**< No active RX operation. */
    UART_RX_STATE_ARMED  = 1U, /**< Buffer armed, waiting for data. */
    UART_RX_STATE_ACTIVE = 2U  /**< Data reception in progress. */
} uartRxState_t;


/**
 * @brief Per-port asynchronous runtime context.
 *
 * @note All fields are protected by the per-port mutex except
 *       evtQ which is set once during init and read-only thereafter.
 */
typedef struct
{
    osMutexId_t mutex;  /**< Per-port mutex for context protection. */
    QueueHandle_t evtQ; /**< ESP-IDF event queue (set by uart_driver_install). */
    bspUartCallback_t cbFn; /**< User callback function pointer. */
    void* cbCtx;            /**< User callback context. */
    uint8_t* rxBuf;         /**< User-provided RX buffer. */
    size_t rxBufSize;       /**< Size of user RX buffer. */
    size_t rxReceived;      /**< Bytes received in current operation. */
    uartRxState_t rxState;  /**< Current RX state. */
    bool initialized;       /**< Port initialization flag. */
} uartAsyncRt_t;


typedef struct
{
    osMutexId_t mutex;

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)
    QueueHandle_t evtQ;
#endif

    bool initialized;

    /* Runtime transfer state */
    bool rxBusy;
    bool txBusy;

    /* Async callbacks */
    bspUartRxCallback_t rxCallback;
    bspUartTxCallback_t txCallback;

    void* rxCallbackContext;
    void* txCallbackContext;

    /* Async RX */
    uint8_t* rxBuffer;
    size_t rxBufferSize;
    size_t rxReceived;

} bspUartPrivate_t;

/* -------------------------------------------------- */
/* STATIC STATE                                       */
/* -------------------------------------------------- */

/** @brief Per-port runtime contexts (zero-initialized). */
static uartAsyncRt_t s_uartRt[BSP_UART_PORT_MAX];

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)

/** @brief Shared UART event processing thread ID. */
static osThreadId_t s_uartEvtThreadId = NULL;

/** @brief Reference count of initialized async UART ports. */
static uint8_t s_asyncPortCount = 0U;


#endif /* BSP_UART_ENABLE_ASYNC */

/* -------------------------------------------------- */
/* FORWARD DECLARATIONS                               */
/* -------------------------------------------------- */

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)
void bspUartEventThread(void* arg);
static void bspUartProcessPortEvents(uint8_t port);
#endif /* BSP_UART_ENABLE_ASYNC */

static bsp_err_sts_t bspUartValidateHandle(const bspUartHandle_t* ptHandle);
static uart_parity_t bspUartMapParity(bspUartParity_t parity);
static uart_stop_bits_t bspUartMapStopBits(bspUartStopBits_t stopBits);
static uart_word_length_t bspUartMapWordLength(bspUartWordLength_t wordLen);

/* -------------------------------------------------- */
/* HELPER FUNCTIONS                                   */
/* -------------------------------------------------- */

/**
 * @brief Validate UART handle pointer and port number.
 *
 * @param[in] ptHandle  Pointer to UART handle.
 *
 * @retval PLT_ERR_STS_OK            Handle is valid.
 * @retval PLT_ERR_STS_INVALID_PARAM Handle is NULL or port out of range.
 */
static bsp_err_sts_t bspUartValidateHandle(const bspUartHandle_t* ptHandle)
{
    bsp_err_sts_t retVal = PLT_ERR_STS_OK;

    if(ptHandle == NULL)
    {
        retVal = PLT_ERR_STS_INVALID_PARAM;
    }
    else if(ptHandle->portNum >= BSP_UART_PORT_MAX)
    {
        retVal = PLT_ERR_STS_INVALID_PARAM;
    }
    else
    {
        /* Valid */
    }

    return retVal;
}

/**
 * @brief Map BSP parity enum to ESP-IDF parity enum.
 */
static uart_parity_t bspUartMapParity(bspUartParity_t parity)
{
    uart_parity_t mapped;

    switch(parity)
    {
        case eBspUartParityOdd:
            mapped = UART_PARITY_ODD;
            break;

        case eBspUartParityEven:
            mapped = UART_PARITY_EVEN;
            break;

        case eBspUartParityNone:
            /* Fall through */
        default:
            mapped = UART_PARITY_DISABLE;
            break;
    }

    return mapped;
}

/**
 * @brief Map BSP stop bits enum to ESP-IDF stop bits enum.
 */
static uart_stop_bits_t bspUartMapStopBits(bspUartStopBits_t stopBits)
{
    uart_stop_bits_t mapped;

    switch(stopBits)
    {
        case eBspUartStopBitsTwo:
            mapped = UART_STOP_BITS_2;
            break;

        case eBspUartStopBitsOne:
            /* Fall through */
        default:
            mapped = UART_STOP_BITS_1;
            break;
    }

    return mapped;
}

/**
 * @brief Map BSP word length to ESP-IDF word length enum.
 */
static uart_word_length_t bspUartMapWordLength(bspUartWordLength_t wordLen)
{
    uart_word_length_t mapped;

    switch(wordLen)
    {
        case eBspUartWordLength5:
            mapped = UART_DATA_5_BITS;
            break;

        case eBspUartWordLength6:
            mapped = UART_DATA_6_BITS;
            break;

        case eBspUartWordLength7:
            mapped = UART_DATA_7_BITS;
            break;

        case eBspUartWordLength8:
            /* Fall through */
        default:
            mapped = UART_DATA_8_BITS;
            break;
    }

    return mapped;
}

/* -------------------------------------------------- */
/* ASYNC EVENT THREAD                                 */
/* -------------------------------------------------- */

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)

/**
 * @brief Shared UART event processing thread.
 *
 * This thread iterates over all initialized UART ports, checks their
 * event queues (non-blocking), and processes any pending events.
 *
 * @par Design Rationale
 * - Single thread for all ports reduces resource usage.
 * - Non-blocking queue reads with periodic sleep avoids busy-wait.
 * - Mutex released before callback invocation prevents deadlock if
 *   user calls bspUartReadAsync() from within callback.
 *
 * @param[in] arg  Unused thread argument.
 */
void bspUartEventThread(void* arg)
{
    (void)arg;

    for(;;)
    {
        bool anyActivity = false;

        for(uint8_t port = 0U; port < BSP_UART_PORT_MAX; port++)
        {
            uartAsyncRt_t* rt = &s_uartRt[port];

            /* Skip uninitialized ports */
            if(rt->initialized == false)
            {
                continue;
            }

            if(rt->evtQ == NULL)
            {
                continue;
            }

            bspUartProcessPortEvents(port);
            anyActivity = true;
        }

        /*
         * Sleep to avoid busy-loop when no events are pending.
         * 10ms gives ~100Hz polling rate which is adequate for
         * UART event processing at typical baud rates.
         *
         * CORNER CASE: If all ports are idle (no evtQ), we still
         * sleep to avoid CPU spin. Thread will be terminated on
         * last port deinit.
         */
        if(anyActivity == false)
        {
            (void)osDelay(BSP_UART_EVT_POLL_MS);
        }
        else
        {
            /*
             * Yield to allow other equal-priority tasks to run.
             * osDelay(1) gives minimum 1 tick delay.
             */
            (void)osDelay(1U);
        }
    }
}

/**
 * @brief Process pending events for a single UART port.
 *
 * Dequeues events from ESP-IDF event queue and handles:
 * - UART_DATA: Read available bytes into user buffer, invoke callback.
 * - UART_FIFO_OVF / UART_BUFFER_FULL: Flush and report error.
 * - UART_BREAK / UART_PARITY_ERR / UART_FRAME_ERR: Report error.
 *
 * @param[in] port  UART port number.
 *
 * @par Corner Cases Handled
 * - Callback is NULL: data is read and discarded to prevent FIFO backup.
 * - RX buffer not armed: data is read into temp buffer and discarded.
 * - Multiple events queued: all are processed in one call.
 * - Mutex timeout: event is left in queue for next iteration.
 */
static void bspUartProcessPortEvents(uint8_t port)
{
    uartAsyncRt_t* rt = &s_uartRt[port];
    uart_event_t evt;
    BaseType_t qRet;

    /*
     * Process ALL pending events in queue (non-blocking).
     * xQueueReceive is used here because ESP-IDF uart_driver_install()
     * creates a FreeRTOS queue internally. We cannot avoid this ESP-IDF
     * dependency but we limit it to this single point.
     *
     * NOTE: This is the ONLY place where a non-CMSIS RTOS API is used,
     * and it is because ESP-IDF mandates it for uart_driver_install().
     */
    qRet = xQueueReceive(rt->evtQ, &evt, 0);

    while(qRet == pdTRUE)
    {
        switch(evt.type)
        {
            case UART_DATA:
            {
                /*
                 * Acquire mutex to access rx context safely.
                 * If timeout occurs, skip this event - it will cause
                 * data to remain in ESP-IDF RX ring buffer until next poll.
                 */
                if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
                {
                    break;
                }

                if((rt->rxState == UART_RX_STATE_ARMED) || (rt->rxState == UART_RX_STATE_ACTIVE))
                {
                    rt->rxState = UART_RX_STATE_ACTIVE;

                    /*
                     * Read available bytes up to user buffer size.
                     * evt.size contains number of bytes available in
                     * ESP-IDF ring buffer.
                     */
                    size_t toRead    = (size_t)evt.size;
                    size_t remaining = rt->rxBufSize - rt->rxReceived;

                    if(toRead > remaining)
                    {
                        toRead = remaining;
                    }

                    int rd =
                    uart_read_bytes((int)port, &rt->rxBuf[rt->rxReceived], toRead, 0 /* Non-blocking: data is already available */
                    );

                    if(rd > 0)
                    {
                        rt->rxReceived += (size_t)rd;
                    }

                    /*
                     * Invoke callback if we received data.
                     * Release mutex BEFORE callback to allow user to
                     * call bspUartReadAsync() from within callback
                     * without deadlock.
                     */
                    bspUartCallback_t cbFn = rt->cbFn;
                    void* cbCtx            = rt->cbCtx;
                    uint8_t* rxBuf         = rt->rxBuf;
                    size_t rxLen           = rt->rxReceived;

                    /* Reset state for next operation */
                    rt->rxState    = UART_RX_STATE_IDLE;
                    rt->rxBuf      = NULL;
                    rt->rxBufSize  = 0U;
                    rt->rxReceived = 0U;

                    (void)osMutexRelease(rt->mutex);

                    /* Invoke callback outside mutex */
                    if((cbFn != NULL) && (rxLen > 0U))
                    {
                        cbFn(PLT_ERR_STS_OK, rxBuf, (uint16_t)rxLen, cbCtx);
                    }
                }
                else
                {
                    /*
                     * CORNER CASE: No RX buffer armed.
                     * Read and discard data to prevent ESP-IDF ring buffer
                     * from filling up and causing UART_BUFFER_FULL events.
                     */
                    uint8_t discardBuf[64];
                    (void)uart_read_bytes((int)port, discardBuf, sizeof(discardBuf), 0);

                    (void)osMutexRelease(rt->mutex);
                }

                break;
            }

            case UART_FIFO_OVF:
                /* Fall through - same handling */
            case UART_BUFFER_FULL:
            {
                /*
                 * CORNER CASE: Hardware FIFO overflow or ESP-IDF ring buffer
                 * full. This happens when data arrives faster than we process.
                 * Flush input to recover and notify user of data loss.
                 */
                (void)uart_flush_input((int)port);

                if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) == osOK)
                {
                    bspUartCallback_t cbFn = rt->cbFn;
                    void* cbCtx            = rt->cbCtx;

                    rt->rxState    = UART_RX_STATE_IDLE;
                    rt->rxBuf      = NULL;
                    rt->rxBufSize  = 0U;
                    rt->rxReceived = 0U;

                    (void)osMutexRelease(rt->mutex);

                    if(cbFn != NULL)
                    {
                        cbFn(PLT_ERR_STS_BUFFER_OVERFLOW, NULL, 0U, cbCtx);
                    }
                }

                break;
            }

            /* Fall through */
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
            case UART_BREAK:
            {
                /*
                 * CORNER CASE: Line errors.
                 * Notify user but do not reset RX state - let user decide
                 * whether to re-arm or abort.
                 */
                if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) == osOK)
                {
                    bspUartCallback_t cbFn = rt->cbFn;
                    void* cbCtx            = rt->cbCtx;

                    /* reset arm state so upper layer can re-arm */
                    rt->rxState    = UART_RX_STATE_IDLE;
                    rt->rxBuf      = NULL;
                    rt->rxBufSize  = 0U;
                    rt->rxReceived = 0U;

                    (void)osMutexRelease(rt->mutex);

                    if(cbFn != NULL)
                    {
                        cbFn(PLT_ERR_STS_FAIL, NULL, 0U, cbCtx);
                    }
                }
                break;
            }

            default:
            {
                /* Unknown event type - ignore */
                break;
            }
        }

        /* Try next event in queue */
        qRet = xQueueReceive(rt->evtQ, &evt, 0);
    }
}

#endif /* BSP_UART_ENABLE_ASYNC */

/* -------------------------------------------------- */
/* PUBLIC API: INIT / DEINIT                          */
/* -------------------------------------------------- */

bsp_err_sts_t bspUartInit(bspUartHandle_t* handle)
{
    bsp_err_sts_t retVal;
    bspUartPrivate_t* pPrivate;
    uart_config_t uartCfg;
    esp_err_t espErr;

    /* --------------------------------------------------
     * Validate handle
     * -------------------------------------------------- */

    if(handle == NULL)
    {
        return PLT_ERR_INVALID_PARAM;
    }

    if(handle->config.port >= eBspUartPort8)
    {
        return PLT_ERR_INVALID_PARAM;
    }

    /* --------------------------------------------------
     * Check double initialization
     * -------------------------------------------------- */

    if(handle->pPrivate != NULL)
    {
        pPrivate = (bspUartPrivate_t*)handle->pPrivate;

        if(pPrivate->initialized == true)
        {
            return BSP_OK;
        }

        return PLT_ERR_FAIL;
    }

    /* --------------------------------------------------
     * Allocate private runtime context
     * -------------------------------------------------- */

    pPrivate = calloc(1U, sizeof(bspUartPrivate_t));

    if(pPrivate == NULL)
    {
        return PLT_ERR_NO_MEM;
    }

    handle->pPrivate = pPrivate;

    /* --------------------------------------------------
     * Create per-UART mutex
     * -------------------------------------------------- */

    pPrivate->mutex = osMutexNew(NULL);

    if(pPrivate->mutex == NULL)
    {
        retVal = PLT_ERR_NO_MEM;
        goto init_fail;
    }

    /* --------------------------------------------------
     * Configure UART
     * -------------------------------------------------- */

    memset(&uartCfg, 0, sizeof(uartCfg));

    uartCfg.baud_rate = (int)handle->config.baudrate;
    uartCfg.data_bits = bspUartMapWordLength(handle->config.wordLength);
    uartCfg.parity    = bspUartMapParity(handle->config.parity);
    uartCfg.stop_bits = bspUartMapStopBits(handle->config.stopBits);
    uartCfg.flow_ctrl = (handle->config.flowControl == eBspUartFlowControlRtsCts) ?
                        UART_HW_FLOWCTRL_CTS_RTS :
                        UART_HW_FLOWCTRL_DISABLE;

    uartCfg.rx_flow_ctrl_thresh = (uint8_t)handle->config.rxThreshold;

    uartCfg.source_clk = UART_SCLK_DEFAULT;

    espErr = uart_param_config((uart_port_t)handle->config.port, &uartCfg);

    if(espErr != ESP_OK)
    {
        retVal = PLT_ERR_FAIL;
        goto init_fail;
    }

    /* --------------------------------------------------
     * Configure UART pins
     * -------------------------------------------------- */

    espErr = uart_set_pin((uart_port_t)handle->config.port, (int)handle->config.uartTxPin,
                          (int)handle->config.uartRxPin, (int)handle->config.uartRtsPin,
                          (int)handle->config.uartCtsPin);

    if(espErr != ESP_OK)
    {
        retVal = PLT_ERR_FAIL;
        goto init_fail;
    }

    /* --------------------------------------------------
     * Install UART driver
     * -------------------------------------------------- */

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)

    espErr = uart_driver_install((uart_port_t)handle->config.port,
                                 BSP_UART_RXTX_BUFFER_SIZE, BSP_UART_RXTX_BUFFER_SIZE,
                                 BSP_UART_EVT_QUEUE_DEPTH, &pPrivate->evtQ, 0U);

#else

    espErr = uart_driver_install((uart_port_t)handle->config.port, BSP_UART_RXTX_BUFFER_SIZE,
                                 BSP_UART_RXTX_BUFFER_SIZE, 0U, NULL, 0U);

#endif

    if(espErr != ESP_OK)
    {
        retVal = PLT_ERR_FAIL;
        goto init_fail;
    }

    /* --------------------------------------------------
     * Configure line inversion if requested
     * -------------------------------------------------- */

    uint32_t invertMask = UART_SIGNAL_INV_DISABLE;

    if(handle->config.invertTx == true)
    {
        invertMask |= UART_SIGNAL_TXD_INV;
    }

    if(handle->config.invertRx == true)
    {
        invertMask |= UART_SIGNAL_RXD_INV;
    }

    if(invertMask != UART_SIGNAL_INV_DISABLE)
    {
        espErr = uart_set_line_inverse((uart_port_t)handle->config.port, invertMask);

        if(espErr != ESP_OK)
        {
            retVal = PLT_ERR_FAIL;
            goto init_driver_fail;
        }
    }

    /* --------------------------------------------------
     * Runtime state
     * -------------------------------------------------- */
    pPrivate->rxBusy = false;
    pPrivate->txBusy = false;

    pPrivate->rxBuffer     = NULL;
    pPrivate->rxBufferSize = 0U;
    pPrivate->rxReceived   = 0U;

    pPrivate->rxCallback = NULL;
    pPrivate->txCallback = NULL;

    pPrivate->rxCallbackContext = NULL;
    pPrivate->txCallbackContext = NULL;

    pPrivate->initialized = true;

    return BSP_OK;


    /* --------------------------------------------------
     * Error handling
     * -------------------------------------------------- */

init_driver_fail:

    (void)uart_driver_delete((uart_port_t)handle->config.port);

init_fail:

    if(pPrivate->mutex != NULL)
    {
        (void)osMutexDelete(pPrivate->mutex);
        pPrivate->mutex = NULL;
    }

    free(pPrivate);
    handle->pPrivate = NULL;

    return retVal;
}

/**
 * @brief De-initialize a UART port.
 *
 * @param[in] ptHandle  Pointer to UART handle.
 *
 * @retval PLT_ERR_STS_OK            Success.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid handle.
 * @retval PLT_ERR_STS_FAIL          ESP-IDF driver deletion failed.
 *
 * @par Corner Cases
 * - Deinit of non-initialized port: returns OK silently.
 * - Last async port deinit: terminates shared event thread.
 * - Active RX operation: cancelled with error callback.
 */
bsp_err_sts_t bspUartDeInit(bspUartHandle_t* ptHandle)
{
    bsp_err_sts_t retVal;

    retVal = bspUartValidateHandle(ptHandle);
    if(retVal != PLT_ERR_STS_OK)
    {
        return retVal;
    }

    uint8_t port      = ptHandle->portNum;
    uartAsyncRt_t* rt = &s_uartRt[port];

    if(rt->initialized == false)
    {
        return PLT_ERR_STS_OK;
    }

    /* Mark as not initialized first to stop event processing */
    rt->initialized = false;

    /* ---- Cancel any active RX and notify user ---- */
    if(rt->mutex != NULL)
    {
        if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) == osOK)
        {
            if(rt->rxState != UART_RX_STATE_IDLE)
            {
                bspUartCallback_t cbFn = rt->cbFn;
                void* cbCtx            = rt->cbCtx;

                rt->rxState    = UART_RX_STATE_IDLE;
                rt->rxBuf      = NULL;
                rt->rxBufSize  = 0U;
                rt->rxReceived = 0U;

                (void)osMutexRelease(rt->mutex);

                /* Notify user that RX was cancelled */
                if(cbFn != NULL)
                {
                    cbFn(PLT_ERR_STS_FAIL, NULL, 0U, cbCtx);
                }
            }
            else
            {
                (void)osMutexRelease(rt->mutex);
            }
        }
    }

    /* ---- Uninstall ESP-IDF UART driver ---- */
    esp_err_t espErr = uart_driver_delete((int)port);
    if(espErr != ESP_OK)
    {
        UART_LOGE("UART%d driver_delete failed: %d", port, (int)espErr);
        rt->initialized = true; /* Rollback */
        return PLT_ERR_STS_FAIL;
    }

#if (BSP_UART_ENABLE_ASYNC == BSP_ENABLE)
    /*
     * Decrement async port count.
     * Terminate shared thread only when last port is deinitialized.
     *
     * CORNER CASE: Thread termination may fail if thread is blocked.
     * We accept this as a shutdown scenario.
     */
    if(s_asyncPortCount > 0U)
    {
        s_asyncPortCount--;
    }

    if((s_asyncPortCount == 0U) && (s_uartEvtThreadId != NULL))
    {
        (void)osThreadTerminate(s_uartEvtThreadId);
        s_uartEvtThreadId = NULL;
    }

    rt->evtQ = NULL;
#endif /* BSP_UART_ENABLE_ASYNC */

    /* ---- Delete per-port mutex ---- */
    if(rt->mutex != NULL)
    {
        (void)osMutexDelete(rt->mutex);
        rt->mutex = NULL;
    }

    /* Clear all runtime state */
    rt->cbFn  = NULL;
    rt->cbCtx = NULL;

    UART_LOGI("UART%d de-initialized", port);
    return PLT_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API: SYNCHRONOUS TX/RX                      */
/* -------------------------------------------------- */

/**
 * @brief Send data synchronously (blocking).
 *
 * @param[in] handle      Pointer to UART handle.
 * @param[in] data        Data buffer to transmit.
 * @param[in] length      Number of bytes to transmit.
 * @param[in] timeout_ms  Timeout in milliseconds.
 *
 * @retval PLT_ERR_STS_OK            All bytes transmitted.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid parameters.
 * @retval PLT_ERR_STS_FAIL          TX error or timeout.
 *
 * @par Corner Cases
 * - Partial write: logged as warning but returns OK if TX done completes.
 * - Zero timeout: immediate return if FIFO full.
 */
bsp_err_sts_t
bspUartSendSync(bspUartHandle_t* handle, const uint8_t* data, size_t length, uint32_t timeout_ms)
{
    if((handle == NULL) || (data == NULL) || (length == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(handle->portNum >= BSP_UART_PORT_MAX)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(s_uartRt[handle->portNum].initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    int port = (int)handle->portNum;

    /* Write bytes to ESP-IDF TX ring buffer */
    int written = uart_write_bytes(port, (const char*)data, length);
    if(written < 0)
    {
        UART_LOGE("UART%d write_bytes failed", port);
        return PLT_ERR_STS_FAIL;
    }

    if((size_t)written != length)
    {
        UART_LOGW("UART%d partial write: %d/%u", port, written, (unsigned)length);
    }

    /* Block until all bytes are physically transmitted */
    TickType_t ticks =
    (timeout_ms == 0U) ? 0U : (TickType_t)(timeout_ms / portTICK_PERIOD_MS);
    esp_err_t err = uart_wait_tx_done(port, ticks);
    if(err != ESP_OK)
    {
        UART_LOGE("UART%d wait_tx_done timeout", port);
        return PLT_ERR_STS_FAIL;
    }

    return PLT_ERR_STS_OK;
}

/**
 * @brief Receive data synchronously (blocking).
 *
 * @param[in]  handle      Pointer to UART handle.
 * @param[out] buffer        Buffer to store received bytes.
 * @param[in]  buffer_len    Maximum bytes to read.
 * @param[out] rx_len      Actual bytes received.
 * @param[in]  timeout_ms  Timeout in milliseconds.
 *
 * @retval PLT_ERR_STS_OK            Data received (check rx_len).
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid parameters.
 * @retval PLT_ERR_STS_FAIL          Read error.
 *
 * @par Corner Cases
 * - Timeout with no data: returns OK with *rx_len = 0.
 * - Partial read: returns available bytes within timeout.
 */
bsp_err_sts_t bspUartReceiveSync(bspUartHandle_t* handle,
                                 uint8_t* buffer,
                                 size_t buffer_len,
                                 size_t* rx_len,
                                 uint32_t timeout_ms)
{
    if((handle == NULL) || (buffer == NULL) || (rx_len == NULL) || (buffer_len == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(handle->portNum >= BSP_UART_PORT_MAX)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(s_uartRt[handle->portNum].initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    int port = (int)handle->portNum;
    TickType_t ticks =
    (timeout_ms == 0U) ? 0U : (TickType_t)(timeout_ms / portTICK_PERIOD_MS);

    int bytes = uart_read_bytes(port, buffer, buffer_len, ticks);
    if(bytes < 0)
    {
        UART_LOGE("UART%d read_bytes failed", port);
        *rx_len = 0U;
        return PLT_ERR_STS_FAIL;
    }

    *rx_len = (size_t)bytes;
    return PLT_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API: CALLBACK REGISTRATION                  */
/* -------------------------------------------------- */

/**
 * @brief Register async RX callback.
 *
 * @param[in] handle       Pointer to UART handle.
 * @param[in] callback     Callback function (NULL to unregister).
 * @param[in] userContext  User context for callback.
 *
 * @retval PLT_ERR_STS_OK            Registered successfully.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid handle.
 *
 * @par Corner Cases
 * - NULL callback: allowed (effectively disables async notifications).
 * - Changing callback during active RX: new callback used for completion.
 */
bsp_err_sts_t
bspUartSetCallback(bspUartHandle_t* handle, bspUartCallback_t callback, void* userContext)
{
    bsp_err_sts_t retVal;

    retVal = bspUartValidateHandle(handle);
    if(retVal != PLT_ERR_STS_OK)
    {
        return retVal;
    }

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];

    if(rt->initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
    {
        return PLT_ERR_STS_FAIL;
    }

    rt->cbFn  = callback;
    rt->cbCtx = userContext;

    (void)osMutexRelease(rt->mutex);

    return PLT_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API: ASYNC READ                             */
/* -------------------------------------------------- */

/**
 * @brief Arm an asynchronous receive operation.
 *
 * @param[in]  handle  Pointer to UART handle.
 * @param[out] buffer  Buffer to store received data.
 * @param[in]  length  Buffer size in bytes.
 *
 * @retval PLT_ERR_STS_OK            RX armed successfully.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid parameters.
 * @retval PLT_ERR_STS_BUSY          RX already in progress.
 * @retval PLT_ERR_STS_NOT_EXIST     Callback not registered.
 * @retval PLT_ERR_STS_FAIL          Port not initialized or mutex error.
 *
 * @par Corner Cases
 * - Called from within RX callback: safe because mutex is released
 *   before callback invocation in event thread.
 * - Buffer smaller than incoming data: only buffer-size bytes received,
 *   remaining stays in ESP-IDF ring buffer for next arm.
 */
bsp_err_sts_t bspUartReadAsync(bspUartHandle_t* handle, uint8_t* buffer, size_t length)
{
    bsp_err_sts_t retVal;

    if((buffer == NULL) || (length == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    retVal = bspUartValidateHandle(handle);
    if(retVal != PLT_ERR_STS_OK)
    {
        return retVal;
    }

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];

    if(rt->initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
    {
        return PLT_ERR_STS_FAIL;
    }

    /* Check callback is registered */
    if(rt->cbFn == NULL)
    {
        (void)osMutexRelease(rt->mutex);
        return PLT_ERR_STS_NOT_EXIST;
    }

    /* Check no RX already in progress */
    if(rt->rxState != UART_RX_STATE_IDLE)
    {
        (void)osMutexRelease(rt->mutex);
        return PLT_ERR_STS_BUSY;
    }

    /* Arm the RX buffer */
    rt->rxBuf      = buffer;
    rt->rxBufSize  = length;
    rt->rxReceived = 0U;
    rt->rxState    = UART_RX_STATE_ARMED;

    (void)osMutexRelease(rt->mutex);

    return PLT_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API: ASYNC WRITE                            */
/* -------------------------------------------------- */

/**
 * @brief Send data asynchronously (non-blocking, fire-and-forget).
 *
 * @param[in] handle  Pointer to UART handle.
 * @param[in] buffer  Data to transmit.
 * @param[in] length  Number of bytes.
 *
 * @retval PLT_ERR_STS_OK            Data queued in ESP-IDF TX ring buffer.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid parameters.
 * @retval PLT_ERR_STS_FAIL          Write error or port not initialized.
 *
 * @par Corner Cases
 * - TX ring buffer full: uart_write_bytes blocks until space available
 *   (ESP-IDF behavior with non-zero TX buffer). This means "async" is
 *   not truly non-blocking if TX buffer is full.
 * - Buffer can be reused immediately after return (ESP-IDF copies data
 *   into internal ring buffer).
 */
bsp_err_sts_t bspUartWriteAsync(bspUartHandle_t* handle, const uint8_t* buffer, size_t length)
{
    if((handle == NULL) || (buffer == NULL) || (length == 0U))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(handle->portNum >= BSP_UART_PORT_MAX)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(s_uartRt[handle->portNum].initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    int written = uart_write_bytes((int)handle->portNum, (const char*)buffer, length);
    if(written < 0)
    {
        return PLT_ERR_STS_FAIL;
    }

    return PLT_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API: IOCTL                                  */
/* -------------------------------------------------- */

/**
 * @brief Perform control operations on UART port.
 *
 * @param[in]     handle  Pointer to UART handle.
 * @param[in]     req     IOCTL request type.
 * @param[in,out] arg     Request-specific argument.
 *
 * @retval PLT_ERR_STS_OK            Operation successful.
 * @retval PLT_ERR_STS_INVALID_PARAM Invalid handle, request, or arg.
 * @retval PLT_ERR_STS_FAIL          Port not initialized.
 *
 * @par Supported Requests
 * - eBspUartGetRxCount: Returns bytes received (arg = size_t*).
 * - eBspUartIsRxBusy:   Returns RX active state (arg = bool*).
 * - eBspUartCancelRx:   Cancels active RX, invokes callback with FAIL.
 */
bsp_err_sts_t bspUartIoctl(bspUartHandle_t* handle, bspUartIoctlRequest_t req, void* arg)
{
    bsp_err_sts_t retVal;

    retVal = bspUartValidateHandle(handle);
    if(retVal != PLT_ERR_STS_OK)
    {
        return retVal;
    }

    uartAsyncRt_t* rt = &s_uartRt[handle->portNum];

    if(rt->initialized == false)
    {
        return PLT_ERR_STS_FAIL;
    }

#if !defined(BSP_UART_ENABLE_ASYNC)
    (void)req;
    (void)arg;
    return PLT_ERR_STS_UNSUPPORTED;
#else

    switch(req)
    {
        case eBspUartGetRxCount:
        {
            if(arg == NULL)
            {
                return PLT_ERR_STS_INVALID_PARAM;
            }

            if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
            {
                return PLT_ERR_STS_FAIL;
            }

            *(size_t*)arg = rt->rxReceived;

            (void)osMutexRelease(rt->mutex);
            return PLT_ERR_STS_OK;
        }

        case eBspUartIsRxBusy:
        {
            if(arg == NULL)
            {
                return PLT_ERR_STS_INVALID_PARAM;
            }

            if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
            {
                return PLT_ERR_STS_FAIL;
            }

            *(bool*)arg = (rt->rxState != UART_RX_STATE_IDLE);

            (void)osMutexRelease(rt->mutex);
            return PLT_ERR_STS_OK;
        }

        case eBspUartCancelRx:
        {
            if(osMutexAcquire(rt->mutex, BSP_UART_MUTEX_TIMEOUT_MS) != osOK)
            {
                return PLT_ERR_STS_FAIL;
            }

            if(rt->rxState == UART_RX_STATE_IDLE)
            {
                (void)osMutexRelease(rt->mutex);
                return PLT_ERR_STS_OK;
            }

            bspUartCallback_t cbFn = rt->cbFn;
            void* cbCtx            = rt->cbCtx;

            rt->rxState    = UART_RX_STATE_IDLE;
            rt->rxBuf      = NULL;
            rt->rxBufSize  = 0U;
            rt->rxReceived = 0U;

            /* Flush hardware FIFO to discard pending data */
            (void)uart_flush_input((int)handle->portNum);

            (void)osMutexRelease(rt->mutex);

            /* Notify user outside mutex */
            if(cbFn != NULL)
            {
                cbFn(PLT_ERR_STS_FAIL, NULL, 0U, cbCtx);
            }

            return PLT_ERR_STS_OK;
        }

        default:
        {
            return PLT_ERR_STS_INVALID_PARAM;
        }
    }

#endif /* BSP_UART_ENABLE_ASYNC */
}