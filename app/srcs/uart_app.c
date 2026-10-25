/**
 * ============================================================================
 * @file    uart_app.c
 * @brief   RideCast UART transport between vehicle cluster and application.
 *
 * @details
 *   Owns the BSP UART handle, receive callback, protocol-frame parser,
 *   heartbeat, interface dispatch, and serialized UART transmission.
 *
 *   Vehicle cluster (MASTER)          RideCast (SLAVE)
 *           |                               |
 *           +-- UART bytes --> BSP UART callback
 *                                            |
 *                                         RX queue
 *                                            |
 *                                        RX thread
 *                                            |
 *                                     Protocol decode
 *                                            |
 *                                     Wi-Fi / BLE / BT
 *
 *   Application modules -> uart_app_send() -> TX queue -> TX thread -> BSP UART
 *
 * ============================================================================
 * RTOS LIFECYCLE
 * ============================================================================
 *
 *   app_main.c owns the RX/TX queues, TX mutex and the two worker threads.
 *   The generic OS wrapper invokes:
 *       uart_app_init(void)
 *       vAppUartRxTask(arg) / vAppUartTxTask(arg)
 *       uart_app_deinit(void)
 *
 *   Both thread descriptors reference uart_app_init and uart_app_deinit.
 *   Initialization is protected by the existing UART TX mutex and
 *   executes only once. Cleanup executes after BOTH worker loops return.
 *   Neither this module nor its callbacks create or terminate threads.
 *
 *   Thread wrapper MUST pass cfg->arg after cfg->init() returns.
 *   No wireless radio is started by this UART module.
 *
 * ============================================================================
 * WARNINGS
 * ============================================================================
 *
 *   - UART RX callbacks may contain partial or multiple protocol frames.
 *   - A decoded payload pointer remains valid only during dispatch.
 *   - Callbacks must not block for extended periods.
 *   - The heartbeat currently uses eRcMsgGetStatus as an existing convention.
 *   - Wi-Fi, BLE and BT Classic register their own frame callbacks.
 *     Cluster-ready signaling must wait for essential registrations.
 *   - Global (transport NONE) command dispatch requires a finalized
 *     application command callback and message IDs.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Integrated central init/thread/deinit lifecycle; UART ownership.
 * 2026-10-25  Corrected RTOS accessors, context initialization and cleanup.
 * 2026-10-25  Updated frame parser and error handling.
 * 2026-05-23  Initial development - J.Raghav.
 * ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_config.h"
#include "app_logging.h"
#include "uart_app.h"
#include "rc_protocol.h"

/* =========================================================================
 * MODULE CONFIGURATION ALIASES
 * ========================================================================= */

#define UAPP_RX_BUF_LEN        configUART_APP_RX_BUF_LEN
#define UAPP_TX_BUF_LEN        configUART_APP_TX_BUF_LEN
#define UAPP_HEARTBEAT_MS      configUART_APP_HEARTBEAT_MS
#define UAPP_TX_TIMEOUT_MS     (500U)
#define UAPP_RX_ERROR_LIMIT    (5U)
#define UAPP_PARSE_ERROR_LIMIT (5U)
#define UAPP_RX_POLL_MS        (10U)
#if (UAPP_RX_BUF_LEN < RC_FRAME_MIN_LEN)
    #error "UART RX buffer must accommodate the minimum protocol frame"
#endif

#if (UAPP_TX_BUF_LEN < RC_FRAME_MIN_LEN)
    #error "UART TX buffer must accommodate the minimum protocol frame"
#endif

#if (UAPP_HEARTBEAT_MS == 0U)
    #error "UART heartbeat interval must be greater than zero"
#endif

/* =========================================================================
 * MODULE CONTEXT
 * ========================================================================= */
static uart_app_t gUartApp;

/* BSP must serialize callbacks; no concurrent writes to this message. */
static uart_app_msg_t g_rxCbMsg;

static volatile uint8_t s_uartRxErrorCount    = 0U;
static volatile uint8_t s_uartParseErrorCount = 0U;
static volatile uint8_t s_uartFailStopped     = 0U;

typedef struct
{
    uint16_t pos;
    uint16_t expectedLength;
} rx_accum_t;

/* =========================================================================
 * STATIC FUNCTION FORWARD DECLARATIONS
 * ========================================================================= */

static bool uart_workers_ready(void);
static void uart_accum_reset(rx_accum_t* acc);
static void uart_dispatch(uart_app_t* app, const rc_frame_t* frame);
// static void uart_note_rx_success(void);
// static void uart_note_parse_success(void);
// static void uart_enter_fail_stop(uart_app_t* app, const char* reason);
// static void uart_note_rx_error(uart_app_t* app, const char* reason);
// static void uart_note_parse_error(uart_app_t* app, const char* reason);
static void uart_rx_cb(ret_err_sts_t status, uint8_t* data, size_t data_len, void* userContext);
static ret_err_sts_t uart_send_heartbeat(void);

/* =========================================================================
 * TASK ENTRY FUNCTIONS
 * ========================================================================= */
void vAppUartRxTask(void* arg)
{
    ret_err_sts_t status;
    rx_accum_t accumulator;
    rc_frame_t frame;
    uart_app_msg_t msg;
    osStatus_t queueStatus;

    uint16_t index;
    uint16_t payloadLength;

    uint32_t now;
    uint32_t heartbeatTicks;

    (void)arg;


    /*
     * Wait until both UART RX and TX worker descriptors
     * have completed their initialization.
     */
    while((gUartApp.running != 0U) && !uart_workers_ready())
    {
        osDelay(1U);
    }

    uart_accum_reset(&accumulator);

    heartbeatTicks =
    (uint32_t)(((uint64_t)UAPP_HEARTBEAT_MS * osKernelGetTickFreq() + 999ULL) / 1000ULL);

    if(heartbeatTicks == 0U)
    {
        heartbeatTicks = 1U;
    }

    ALOGI("UART RX task started");

    /* */
    while(gUartApp.running)
    {
        /*
         * The application RX thread processes the BSP events.
         *
         * The BSP invokes uart_rx_cb() whenever available bytes
         * are received.
         *
         * No bspUartReadAsync() re-arming is required.
         */
        status = bspUartProcessRx(&gUartApp.cfg.uartHandle, UAPP_RX_POLL_MS);

        if(status != RET_ERR_STS_OK)
        {
            continue;
        }

        /*
         * RX callbacks enqueue byte chunks into rxQ.
         *
         * Do not block here, because this same thread must
         * continue processing BSP UART events.
         */
        queueStatus = osMessageQueueGet(gUartApp.rxQ, &msg, NULL, 0U);

        now = osKernelGetTickCount();

        /*
         * Periodically report application heartbeat.
         */
        if((uint32_t)(now - gUartApp.lastHeartbeatTick) >= heartbeatTicks)
        {
            gUartApp.lastHeartbeatTick = now;

            if(uart_send_heartbeat() != RET_ERR_STS_OK)
            {
                ALOGW("UART heartbeat enqueue failed");
            }
        }

        if(queueStatus != osOK)
        {
            continue;
        }

        /*
         * Process all bytes in the current RX chunk.
         *
         * A chunk can contain a partial frame, exactly one
         * frame, or multiple consecutive frames.
         */
        for(index = 0U; index < msg.len; index++)
        {
            uint8_t byte = msg.buf[index];

            /*
             * Prevent accumulation-buffer overflow.
             */
            if(accumulator.pos >= sizeof(gUartApp.rxBuf))
            {
                // uart_note_parse_error(&gUartApp, "RX frame overflow");

                uart_accum_reset(&accumulator);
                continue;
            }

            gUartApp.rxBuf[accumulator.pos] = byte;
            accumulator.pos++;

            /*
             * The protocol payload length occupies
             * header bytes 5 and 6.
             */
            if(accumulator.pos == 7U)
            {
                payloadLength = (uint16_t)(((uint16_t)gUartApp.rxBuf[5U] << 8U) |
                                           (uint16_t)gUartApp.rxBuf[6U]);

                if((payloadLength > RC_MAX_PAYLOAD_LEN) ||
                   ((uint32_t)payloadLength + RC_FRAME_MIN_LEN > sizeof(gUartApp.rxBuf)))
                {
                    ALOGE("Invalid UART payload length=%u", (unsigned int)payloadLength);

                    // uart_note_parse_error(&gUartApp, "Invalid frame length");

                    uart_accum_reset(&accumulator);
                    continue;
                }

                accumulator.expectedLength = (uint16_t)(RC_FRAME_MIN_LEN + payloadLength);
            }

            /*
             * Decode when the accumulated frame is complete.
             */
            if((accumulator.expectedLength != 0U) &&
               (accumulator.pos == accumulator.expectedLength))
            {
                status = rc_frame_decode(gUartApp.rxBuf, accumulator.pos, &frame);

                if(status == RET_ERR_STS_OK)
                {
                    // uart_note_parse_success();

                    uart_dispatch(&gUartApp, &frame);
                }
                else
                {
                    ALOGE("UART frame decode failed, len=%u",
                          (unsigned int)accumulator.pos);

                    // uart_note_parse_error(&gUartApp, "Frame validation failed");
                }

                uart_accum_reset(&accumulator);
            }
        }
    }

    ALOGW("UART RX task stopped");
    APP_THREAD_HANDLE_CLEAR(APP_THREAD_OWNER_UART_APP_RX);
    osThreadTerminate(gUartApp.rxTaskId);
}

void vAppUartTxTask(void* arg)
{
    uart_app_msg_t msg;

    gUartApp.txTaskId = osThreadGetId();

    /* Both worker wrappers must complete initialization first. */
    while((gUartApp.running != 0U) && !uart_workers_ready())
    {
        osDelay(1000U);
    }

    ALOGI("UART TX task started");

    while(gUartApp.running)
    {
        /*  */
        if(osMessageQueueGet(gUartApp.txQ, &msg, NULL, osKernelGetTickFreq()) != osOK)
        {
            continue;
        }

        if((msg.len == 0U) || (msg.len > sizeof(msg.buf)))
        {
            ALOGW("Invalid UART TX message length");
            continue;
        }

        if(bspUartSendSync(&gUartApp.cfg.uartHandle, msg.buf, msg.len,
                           UAPP_TX_TIMEOUT_MS) != RET_ERR_STS_OK)
        {
            ALOGE("UART TX failed, len=%u", (unsigned int)msg.len);
        }
    }

    ALOGW("UART TX task stopped");

    /* Return so the generic OS wrapper runs uart_app_deinit(). */
    APP_THREAD_HANDLE_CLEAR(APP_THREAD_OWNER_UART_APP_TX);
    osThreadTerminate(gUartApp.txTaskId);
}

/* =========================================================================
 * INITIALIZATION
 * ========================================================================= */

/**
 * @brief Initialize shared UART service once for the RX and TX wrappers.
 *
 * @details
 * Both wrappers call this no-argument function. The centrally created
 * UART TX mutex serializes concurrent calls, so BSP UART configuration,
 * callback registration and receive arming happen exactly once.
 *
 * Initialization does NOT create threads or start wireless services.
 */
ret_err_sts_t uart_app_init(void)
{
    ret_err_sts_t status;

    gUartApp.mutex = APP_MUTEX_HANDLE(APP_MUTEX_OWNER_UART_APP);

    if(gUartApp.mutex == NULL)
    {
        ALOGI("UART mutex unavailable");
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(osMutexAcquire(gUartApp.mutex, osWaitForever) != osOK)
    {
        ALOGE("UART cleanup mutex acquisition failed");
        return RET_ERR_STS_FAIL;
    }

    /*
     * Preserve frame handlers registered by Wi-Fi/BLE/BT modules.
     * Do not memset the whole gUartApp context here.
     */
    gUartApp.running  = 0U;
    gUartApp.rxTaskId = APP_THREAD_HANDLE(APP_THREAD_OWNER_UART_APP_RX);
    gUartApp.txTaskId = APP_THREAD_HANDLE(APP_THREAD_OWNER_UART_APP_TX);
    (void)memset(gUartApp.rxBuf, 0, sizeof(gUartApp.rxBuf));

    /* Preserve the existing by-value BSP UART handle convention. */
    (void)memcpy(&gUartApp.cfg.uartHandle, &g_appUartHandleCfg[APP_UART_OWNER_HOST_COMM],
                 sizeof(bspUartHandle_t));

    gUartApp.rxQ = APP_QUEUE_HANDLE(APP_Q_OWNER_RC_UART_RX);
    gUartApp.txQ = APP_QUEUE_HANDLE(APP_Q_OWNER_RC_UART_TX);

    if((gUartApp.rxQ == NULL) || (gUartApp.txQ == NULL))
    {
        ALOGE("UART queues unavailable");
        status = RET_ERR_STS_NOT_INITIALIZED;
        goto init_failed;
    }

    if((osMessageQueueReset(gUartApp.rxQ) != osOK) ||
       (osMessageQueueReset(gUartApp.txQ) != osOK))
    {
        ALOGE("UART queue reset failed");
        status = RET_ERR_STS_FAIL;
        goto init_failed;
    }

    s_uartRxErrorCount    = 0U;
    s_uartParseErrorCount = 0U;
    s_uartFailStopped     = 0U;

    status = bspUartInit(&gUartApp.cfg.uartHandle);

    if(status != RET_ERR_STS_OK)
    {
        ALOGE("BSP UART initialization failed");
        goto init_failed;
    }

    /* RX callback must see a fully initialized application context. */
    gUartApp.running           = 1U;
    gUartApp.lastHeartbeatTick = osKernelGetTickCount();

    status = bspUartSetRxCallback(&gUartApp.cfg.uartHandle, uart_rx_cb, &gUartApp);

    if(status != RET_ERR_STS_OK)
    {
        ALOGE("UART callback registration failed");
        goto bsp_cleanup;
    }

    ALOGI("UART initialized; RX/TX workers use shared context");
    return RET_ERR_STS_OK;

bsp_cleanup:
    gUartApp.running = 0U;

    (void)bspUartSetRxCallback(&gUartApp.cfg.uartHandle, NULL, NULL);
    (void)bspUartDeInit(&gUartApp.cfg.uartHandle);

init_failed:
    gUartApp.running = 0U;
    (void)osMutexRelease(gUartApp.mutex);
    return status;
}

/* =========================================================================
 * DEINITIALIZATION
 * ========================================================================= */

/**
 * @brief Release UART service after both worker functions return.
 *
 * @details
 * Both wrappers call this no-argument function. The first returning
 * worker requests shutdown of the other; the last returning worker
 * releases the UART hardware. RTOS queues and mutex are NOT deleted.
 *
 * @note Only the central OS wrapper owns thread handles and termination.
 */
/**
 * @brief Deinitialize UART after both workers have returned.
 *
 * @details
 * The first worker returning requests shutdown.
 * The second worker returning releases the BSP UART.
 *
 * The central OS wrapper remains responsible for
 * thread creation and thread lifecycle.
 */
ret_err_sts_t uart_app_deinit(void)
{
    if(gUartApp.mutex == NULL)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(osMutexAcquire(gUartApp.mutex, osWaitForever) != osOK)
    {
        ALOGE("UART cleanup mutex acquisition failed");
        return RET_ERR_STS_FAIL;
    }

    /*
     * Both workers have now returned.
     *
     * The RX thread is no longer calling bspUartProcessRx(),
     * so it is safe to release the BSP event queue.
     */
    (void)bspUartSetRxCallback(&gUartApp.cfg.uartHandle, NULL, NULL);

    if(bspUartDeInit(&gUartApp.cfg.uartHandle) != RET_ERR_STS_OK)
    {
        ALOGE("BSP UART deinitialization failed");
    }

    /*
     * Queues are owned by the central OS configuration.
     * Reset them but do not delete them.
     */
    if(gUartApp.rxQ != NULL)
    {
        (void)osMessageQueueReset(gUartApp.rxQ);
    }

    if(gUartApp.txQ != NULL)
    {
        (void)osMessageQueueReset(gUartApp.txQ);
    }

    gUartApp.rxQ      = NULL;
    gUartApp.txQ      = NULL;
    gUartApp.mutex    = NULL;
    gUartApp.rxTaskId = NULL;
    gUartApp.txTaskId = NULL;

    s_uartRxErrorCount    = 0U;
    s_uartParseErrorCount = 0U;
    s_uartFailStopped     = 0U;

    (void)osMutexRelease(gUartApp.mutex);

    ALOGI("UART application deinitialized");
    return RET_ERR_STS_OK;
}
/* =========================================================================
 * OTHER PUBLIC APIS
 * ========================================================================= */

/**
 * @brief Queue a complete encoded RideCast frame for UART transmission.
 *
 * @details
 * Uses the module-owned gUartApp; other modules do not need the UART
 * context or a generic ridecast_main.c forwarding function.
 */
ret_err_sts_t uart_app_send(const uint8_t* frame, uint16_t len)
{
    uart_app_msg_t msg;

    if((frame == NULL) || (len < RC_FRAME_MIN_LEN) || (len > sizeof(msg.buf)))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if((gUartApp.txQ == NULL))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    (void)memcpy(msg.buf, frame, len);
    msg.len = len;

    if(osMessageQueuePut(gUartApp.txQ, &msg, 0U, 0U) != osOK)
    {
        ALOGE("UART TX queue full");
        return RET_ERR_STS_FAIL;
    }

    ALOGD("Queued UART frame, len=%u", (unsigned int)len);
    return RET_ERR_STS_OK;
}

/**
 * @brief Register a transport-specific protocol frame handler.
 *
 * @details
 * Wi-Fi, BLE and BT Classic modules can register their handlers from
 * their own init callbacks. The centrally owned mutex serializes handler
 * updates with UART RX dispatch and UART hardware initialization.
 *
 * @note Pass NULL to unregister an interface during coordinated shutdown.
 */
ret_err_sts_t
uart_app_register_handler(uint8_t transport, uart_app_intf_cb_t callback, void* user)
{
    if((gUartApp.mutex == NULL) || osMutexAcquire(gUartApp.mutex, osWaitForever) != osOK)
    {
        ALOGE("UART cleanup mutex acquisition failed");
        return RET_ERR_STS_FAIL;
    }

    switch(transport)
    {
        case eRcTransportWifi:
        {
            gUartApp.cfg.onWifiFrame = callback;
            gUartApp.cfg.wifiUser    = user;
            break;
        }

        case eRcTransportBle:
        {
            gUartApp.cfg.onBleFrame = callback;
            gUartApp.cfg.bleUser    = user;
            break;
        }

        case eRcTransportBtClassic:
        {
            gUartApp.cfg.onBtFrame = callback;
            gUartApp.cfg.btUser    = user;
            break;
        }

        default:
        {
            (void)osMutexRelease(gUartApp.mutex);
            return RET_ERR_STS_INVALID_PARAM;
        }
    }

    (void)osMutexRelease(gUartApp.mutex);
    return RET_ERR_STS_OK;
}


/* =========================================================================
 * STATIC HELPER IMPLEMENTATIONS
 * ========================================================================= */

/**
 * @brief Confirm that BOTH UART worker wrappers completed initialization.
 */
static bool uart_workers_ready(void)
{
    bool ready = false;
    if((gUartApp.mutex != NULL) && (osMutexAcquire(gUartApp.mutex, osWaitForever) == osOK))
    {
        /* If both task created meaans worker are ready */
        if(gUartApp.rxTaskId && gUartApp.txTaskId)
        {
            ready = true;
        }
        (void)osMutexRelease(gUartApp.mutex);
    }

    return ready;
}

static void uart_accum_reset(rx_accum_t* acc)
{
    if(acc == NULL)
    {
        return;
    }

    acc->pos            = 0U;
    acc->expectedLength = 0U;
}

static void uart_dispatch(uart_app_t* app, const rc_frame_t* frame)
{
    uart_app_intf_cb_t callback = NULL;
    void* user                  = NULL;

    if((app == NULL) || (frame == NULL) || (app->mutex == NULL))
    {
        ALOGE("UART dispatch: invalid parameters");
        return;
    }

    ALOGD("UART frame: transport=%u type=%u msg=%u len=%u",
          (unsigned int)frame->transport, (unsigned int)frame->type,
          (unsigned int)frame->messageId, (unsigned int)frame->payloadLen);

    /* Copy the handler under the shared UART configuration lock. */
    if(osMutexAcquire(app->mutex, osWaitForever) != osOK)
    {
        ALOGE("UART dispatch mutex unavailable");
        return;
    }

    switch(frame->transport)
    {
        case eRcTransportWifi:
        {
            callback = app->cfg.onWifiFrame;
            user     = app->cfg.wifiUser;
            break;
        }

        case eRcTransportBle:
        {
            callback = app->cfg.onBleFrame;
            user     = app->cfg.bleUser;
            break;
        }

        case eRcTransportBtClassic:
        {
            callback = app->cfg.onBtFrame;
            user     = app->cfg.btUser;
            break;
        }

        default:
        {
            /* Transport NONE needs the planned global-command handler. */
            break;
        }
    }

    (void)osMutexRelease(app->mutex);

    /* Never call application callbacks while holding the UART mutex. */
    if(callback != NULL)
    {
        callback(frame, user);
    }
    else
    {
        ALOGW("Unsupported/unregistered UART transport: %u", (unsigned int)frame->transport);
    }
}

/**
 * @brief Receive available bytes from the BSP UART driver.
 *
 * @details
 * Called synchronously by bspUartProcessRx() in the
 * application RX thread.
 *
 * Copies received bytes into the application RX queue.
 * Does not re-arm asynchronous reception.
 */
static void uart_rx_cb(ret_err_sts_t status, uint8_t* data, size_t data_len, void* userContext)
{
    uart_app_t* app = (uart_app_t*)userContext;

    if((app == NULL) || (app->rxQ == NULL))
    {
        return;
    }

    if(status != RET_ERR_STS_OK)
    {
        return;
    }

    if((data == NULL) || (data_len == 0U) ||
       (data_len > sizeof(g_rxCbMsg.buf)) || (data_len > UINT16_MAX))
    {
        return;
    }

    /*
     * The BSP owns the temporary RX chunk.
     * Copy it before the callback returns.
     */
    (void)memcpy(g_rxCbMsg.buf, data, data_len);

    g_rxCbMsg.len = (uint16_t)data_len;

    if(osMessageQueuePut(app->rxQ, &g_rxCbMsg, 0U, 0U) != osOK)
    {
        return;
    }
}

static ret_err_sts_t uart_send_heartbeat(void)
{
    uint8_t frame[RC_FRAME_MIN_LEN];
    uint16_t length;

    length = rc_encode_simple(frame, (uint16_t)sizeof(frame), (uint8_t)eRcTypeStatus,
                              RC_FLAG_NONE, (uint8_t)eRcMsgGetStatus,
                              (uint8_t)eRcTransportNone, 0U);

    if(length == 0U)
    {
        return RET_ERR_STS_FAIL;
    }

    return uart_app_send(frame, length);
}

/* =========================================================================
 * FILE END
 * ========================================================================= */
