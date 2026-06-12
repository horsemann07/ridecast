/**
 * @file    uart_app.c
 * @brief   UART Application Task implementation.
 *
 * ARCHITECTURE:
 *
 *   ┌──────────┐   UART RX bytes    ┌────────────┐
 *   │   Host   │ ─────────────────> │  RX Task   │
 *   │  (MPU)   │                    │  decode    │
 *   │          │                    │  rc_frame  │
 *   │          │                    │     │      │
 *   │          │                    │     ▼      │
 *   │          │              intf dispatch      │
 *   │          │            WiFi / BLE / BT      │
 *   │          │                                 │
 *   │          │   UART TX bytes    ┌────────────┐
 *   │          │ <───────────────── │  TX Task   │
 *   │          │                    │  tx queue  │
 *   └──────────┘                    └────────────┘
 *
 *   Any task calls uart_app_send() -> posts to TX queue -> TX task sends via UART
 *
 * HEARTBEAT:
 *   RX task checks elapsed time each loop.
 *   Every UART_APP_HEARTBEAT_MS it sends a STATUS READY frame to Host.
 *
 * MISRA C:2012:
 *   - No heap
 *   - All return values checked
 *   - Explicit casts
 *   - No recursion
 *
 *
 *
 * ┌──────────────┐
 *  │   Host PC    │
 *  │  (Python)    │
 *  └──────┬───────┘
 *         │ UART bytes
 *         ▼
 *  ┌──────────────────────────────────────┐
 *  │   Embedded Device (ESP32)            │
 *  ├──────────────────────────────────────┤
 *  │                                      │
 *  │  ┌─────────────────────────────────┐ │
 *  │  │  UART BSP Driver                │ │
 *  │  │  (async RX callback)            │ │
 *  │  └────────┬──────────────────────┬─┘ │
 *  │           │ raw bytes            │   │
 *  │           ▼                      ▼   │
 *  │      ┌──────────┐          ┌──────────────┐
 *  │      │ RX Queue │          │ TX Queue     │
 *  │      │(messages)│          │(frames)      │
 *  │      └────┬─────┘          └──────┬───────┘
 *  │           │                       │
 *  │           ▼                       ▲
 *  │      ┌──────────────┐        ┌────────────┐
 *  │      │  RX Task     │        │  TX Task   │
 *  │      │  decode      │        │  send      │
 *  │      │  dispatch    │        │  via UART  │
 *  │      └──────┬───────┘        └────────────┘
 *  │             │ decoded frame
 *  │             ▼
 *  │        ┌─────────────┐
 *  │        │  Dispatch   │
 *  │        │  WiFi / BLE │
 *  │        │  / BT CB    │
 *  │        └─────────────┘
 *  │
 *  └──────────────────────────────────────┘
 */

/*
 * =========================================================================
 * Change History:
 * Format:
 * YYYY-MM-DD - Developer - Description
 * =========================================================================
 *
 * 2026-05-23 - J.Raghav - Initial development
 */

#include <string.h>

#include "app_config.h"

#include "uart_app.h"
#include "rc_protocol.h"

#include "app_logging.h"

/* =========================================================================
 * Fail-stop configuration
 * ========================================================================= */
#define UART_APP_RX_ERROR_LIMIT    (5U)
#define UART_APP_PARSE_ERROR_LIMIT (5U)

static volatile uint8_t s_uartRxErrorCount    = 0U;
static volatile uint8_t s_uartParseErrorCount = 0U;
static volatile uint8_t s_uartFailStopped     = 0U;

/* =========================================================================
 * Static TX scratch buffer (never on stack)
 * ========================================================================= */
static uint8_t gTxEncodeBuf[UART_APP_TX_BUF_LEN];

/* =========================================================================
 * Static RX buffer - used by async RX, always listening
 * ========================================================================= */
static uint8_t gRxAsyncBuf[UART_APP_RX_BUF_LEN];

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static void uart_rx_task(void* arg);
static void uart_tx_task(void* arg);
static void uart_rx_cb(bsp_err_sts_t status, uint8_t* data, uint16_t data_len, void* userContext);
static void uart_enter_fail_stop(uart_app_t* app, const char* reason);
static void uart_note_rx_error(uart_app_t* app, const char* reason);
static void uart_note_parse_error(uart_app_t* app, const char* reason);
static void uart_note_rx_success(void);
static void uart_note_parse_success(void);

/* =========================================================================
 * RX frame accumulator
 *
 * Problem: UART RX may give us bytes in chunks.
 * We must collect bytes until we have a complete rc_protocol frame.
 *
 * Strategy:
 *   - Read 1 byte at a time to get header
 *   - If DATA frame: read 2 more bytes for length, then read payload
 *   - If CMD/STATUS/ACK: 1 byte is the complete frame
 *
 * Uses bspUartReceiveSync in a loop (non-blocking with short timeout).
 * ========================================================================= */
typedef struct
{
    uint8_t buf[UART_APP_RX_BUF_LEN];
    uint16_t pos;    /**< Write position in buf          */
    uint8_t haveHdr; /**< 1 = header byte collected      */
    uint8_t haveLen; /**< 1 = length bytes collected     */
    uint16_t needed; /**< remaining bytes to complete    */
    rc_type_t frameType;
} rx_accum_t;

/* =========================================================================
 * RX callback - called from BSP UART event thread on data received
 *
 * This replaces the polling loop.
 * BSP calls this whenever bytes arrive -> we post raw bytes to RX queue
 * -> RX task accumulates and decodes frames.
 * ========================================================================= */

/* RX queue message */
typedef struct
{
    uint8_t buf[UART_APP_RX_BUF_LEN];
    uint16_t len;
} uart_rx_msg_t;

/* Static RX callback message buffer to avoid large stack usage in callback */
static uart_rx_msg_t g_rxCbMsg;

/* Reset accumulator for next frame */
static void accum_reset(rx_accum_t* a)
{
    uint16_t i;
    for(i = 0U; i < (uint16_t)sizeof(a->buf); i++)
    {
        a->buf[i] = 0U;
    }
    a->pos       = 0U;
    a->haveHdr   = 0U;
    a->haveLen   = 0U;
    a->needed    = 0U;
    a->frameType = eRcTypeCmd;
}

/* =========================================================================
 * Dispatch decoded frame to registered interface callback
 * ========================================================================= */
static void dispatch(uart_app_t* app, const rc_frame_t* frame)
{
    if((app == NULL) || (frame == NULL))
    {
        ALOGE("uart_app_dispatch: Invalid parameter");
        return;
    }

    ALOGD("uart_app_dispatch: Dispatching frame, intf=%d, type=%d, subcode=%u, "
          "payloadLen=%d",
          (int)frame->intf, (int)frame->type, (uint8_t)frame->subcode,
          (uint16_t)frame->payloadLen);

    switch(frame->intf)
    {
        case eRcIntfWifi:
            if(app->cfg.onWifiFrame != NULL)
            {
                app->cfg.onWifiFrame(frame, app->cfg.wifiUser);
            }
            break;

        case eRcIntfBle:
            if(app->cfg.onBleFrame != NULL)
            {
                app->cfg.onBleFrame(frame, app->cfg.bleUser);
            }
            break;

        case eRcIntfBtClassic:
            if(app->cfg.onBtFrame != NULL)
            {
                app->cfg.onBtFrame(frame, app->cfg.btUser);
            }
            break;

        default:
            /* Reserved interface - ignore */
            break;
    }
}

static void uart_note_rx_success(void)
{
    s_uartRxErrorCount = 0U;
}

static void uart_note_parse_success(void)
{
    s_uartParseErrorCount = 0U;
}

static void uart_enter_fail_stop(uart_app_t* app, const char* reason)
{
    if((app == NULL) || (s_uartFailStopped != 0U))
    {
        return;
    }

    s_uartFailStopped = 1U;
    app->running      = 0U;

    ALOGE("uart_app: FAIL-STOP entered: %s", reason);

    /* Stop new callbacks first */
    (void)bspUartSetCallback(&app->cfg.uartHandle, NULL, NULL);

    /* Cancel any active RX */
    (void)bspUartIoctl(&app->cfg.uartHandle, eBspUartCancelRx, NULL);
}

static void uart_note_rx_error(uart_app_t* app, const char* reason)
{
    if((app == NULL) || (s_uartFailStopped != 0U))
    {
        return;
    }

    if(s_uartRxErrorCount < 255U)
    {
        s_uartRxErrorCount++;
    }

    ALOGE("uart_app: RX error count=%u", (unsigned int)s_uartRxErrorCount);

    if(s_uartRxErrorCount >= UART_APP_RX_ERROR_LIMIT)
    {
        uart_enter_fail_stop(app, reason);
    }
}

static void uart_note_parse_error(uart_app_t* app, const char* reason)
{
    if((app == NULL) || (s_uartFailStopped != 0U))
    {
        return;
    }

    if(s_uartParseErrorCount < 255U)
    {
        s_uartParseErrorCount++;
    }

    ALOGE("uart_app: Parse error count=%u", (unsigned int)s_uartParseErrorCount);

    if(s_uartParseErrorCount >= UART_APP_PARSE_ERROR_LIMIT)
    {
        uart_enter_fail_stop(app, reason);
    }
}

/* =========================================================================
 * RX callback - called from BSP UART event thread on data received
 *
 * This replaces the polling loop.
 * BSP calls this whenever bytes arrive -> we post raw bytes to RX queue
 * -> RX task accumulates and decodes frames.
 * uart_rx_cb(status, data, data_len, userContext)
 *   │
 *   ├── check status OK
 *   ├── copy raw bytes into msg
 *   ├── post msg to RX queue
 *   │
 *   └── re-arm async RX
 *         └─ bspUartReadAsync again
 *            (auto re-arm for next chunk)
 * ========================================================================= */

static void uart_rx_cb(bsp_err_sts_t status, uint8_t* data, uint16_t data_len, void* userContext)
{
    uart_app_t* app = (uart_app_t*)userContext;
    bool rxBusy     = false;

    if((app == NULL) || (app->running == 0U) || (app->rxQ == NULL) ||
       (s_uartFailStopped != 0U))
    {
        ALOGW(
        "uart_rx_cb: App not running or fail-stopped, ignoring RX callback");
        return; // defensive: should not happen since we disarm callback on fail-stop, but check just in case
    }

    if((status == BSP_ERR_STS_OK) && (data != NULL) && (data_len > 0U) &&
       (data_len <= (uint16_t)sizeof(g_rxCbMsg.buf)))
    {
        (void)memcpy(g_rxCbMsg.buf, data, (size_t)data_len);
        g_rxCbMsg.len = data_len;

        if(osMessageQueuePut(app->rxQ, &g_rxCbMsg, 0U, 0U) != osOK)
        {
            ALOGE("uart_app_rx_cb: RX queue post failed, len=%u", (unsigned int)data_len);
            uart_note_rx_error(app, "RX queue post failure");
        }
        else
        {
            uart_note_rx_success(); /* reset consecutive RX error counter */
        }
    }
    else if(status != BSP_ERR_STS_OK)
    {
        ALOGE("uart_app_rx_cb: RX callback error, status=%d", (int)status);
        uart_note_rx_error(app, "BSP RX callback failure");
    }

    if((app->running != 0U) && (app->rxQ != NULL))
    {
        if((bspUartIoctl(&app->cfg.uartHandle, eBspUartIsRxBusy, &rxBusy) == BSP_ERR_STS_OK) &&
           (rxBusy == false))
        {
            if(bspUartReadAsync(&app->cfg.uartHandle, gRxAsyncBuf,
                                sizeof(gRxAsyncBuf)) != BSP_ERR_STS_OK)
            {
                ALOGE("uart_app_rx_cb: Re-arm async RX failed");
                uart_note_rx_error(app, "Async RX re-arm failure");
            }
        }
    }
}
/* =========================================================================
 * RX Task
 *
 * Loop:
 *   - Decode complete frame
 *   - Dispatch to interface callback
 *   - Check heartbeat timer - send if 10s elapsed
 * ========================================================================= */
static void uart_rx_task(void* arg)
{
    uart_app_t* app = (uart_app_t*)arg;
    rx_accum_t acc;
    rc_frame_t frame;
    uart_rx_msg_t msg;
    osStatus_t qSts;
    uint32_t now;
    uint16_t payLen;
    uint16_t i;

    accum_reset(&acc);
    ALOGD("uart_app_rx_task: RX task started");

    while(app->running != 0U)
    {
        /* ------------------------------------------------------------------
         * Block on RX queue with heartbeat timeout
         * If no data for UART_APP_HEARTBEAT_MS -> send heartbeat then loop
         * ------------------------------------------------------------------ */
        qSts = osMessageQueueGet(app->rxQ, &msg, NULL, UART_APP_HEARTBEAT_MS);

        /* ------------------------------------------------------------------
         * HEARTBEAT check (runs on timeout OR after every received chunk)
         * ------------------------------------------------------------------ */
        now = osKernelGetTickCount();

        if((now - app->lastHeartbeatTick) >= UART_APP_HEARTBEAT_MS)
        {
            ALOGD("uart_app_rx_task: Heartbeat - sending STATUS READY to Host");
            app->lastHeartbeatTick = now;
            (void)uart_app_send_status(app, eRcIntfWifi, eRcStsSubReady);
        }

        /* Queue timeout - no data, loop back to wait */
        if(qSts != osOK)
        {
            continue;
        }

        /* ------------------------------------------------------------------
         * Feed received bytes into accumulator one by one
         * ------------------------------------------------------------------ */
        for(i = 0U; i < msg.len; i++)
        {
            uint8_t byte = msg.buf[i];

            /* STEP 1: collect header */
            if(acc.haveHdr == 0U)
            {
                acc.buf[0U] = byte;
                acc.pos     = 1U;
                acc.haveHdr = 1U;
                acc.frameType = (rc_type_t)((byte & RC_HDR_TYPE_MASK) >> RC_HDR_TYPE_SHIFT);

                /* All frame types (CMD, STATUS, DATA, ACK) need length field */
                acc.haveLen = 0U;
                acc.needed  = 2U; /* next: 2 length bytes */
                continue;
            }

            /* STEP 2: collect length bytes (ALL frame types) */
            if(acc.haveLen == 0U)
            {
                acc.buf[acc.pos] = byte;
                acc.pos++;
                acc.needed--;

                if(acc.needed == 0U)
                {
                    /* Both length bytes collected */
                    payLen =
                    (uint16_t)(((uint16_t)acc.buf[1U] << 8U) | (uint16_t)acc.buf[2U]);

                    if(payLen == 0U)
                    {
                        /* No payload - frame complete (STATUS, ACK, etc) */
                        acc.haveLen = 1U;
                        acc.needed  = 0U;
                    }
                    else if(payLen > RC_MAX_PAYLOAD_LEN)
                    {
                        ALOGE("Invalid payload length=%u, discarding frame",
                              (unsigned int)payLen);
                        uart_note_parse_error(app, "Invalid payload length");
                        accum_reset(&acc);
                    }
                    else
                    {
                        /* Payload exists - wait for it */
                        acc.haveLen = 1U;
                        acc.needed  = payLen;
                    }
                }
                continue;
            }

            /* STEP 3: collect payload bytes (if needed) */
            if(acc.needed > 0U)
            {
                acc.buf[acc.pos] = byte;
                acc.pos++;
                acc.needed--;
            }

            /* STEP 4: frame complete? */
            if((acc.haveHdr == 1U) && (acc.haveLen == 1U) && (acc.needed == 0U))
            {
                if(rc_frame_decode(acc.buf, acc.pos, &frame))
                {
                    uart_note_parse_success(); /* reset consecutive parse error counter */
                    dispatch(app, &frame);
                }
                else
                {
                    ALOGE("Frame decode failed, len=%u", (unsigned int)acc.pos);
                    uart_note_parse_error(app, "Frame decode failure");
                }

                accum_reset(&acc);
            }
        }
    }
    ALOGW("uart_app_rx_task: RX task stopped");
    osDelay(100U); /* Let log flush */
    osThreadExit();
}

/* =========================================================================
 * TX Task
 *
 * Blocks on TX queue.
 * When a message arrives, sends it over UART synchronously.
 * All sends from any task go through this single task -> no UART contention.
 * ========================================================================= */
static void uart_tx_task(void* arg)
{
    uart_app_t* app = (uart_app_t*)arg;
    uart_app_tx_msg_t msg;
    osStatus_t qSts;
    ALOGD("uart_app_tx_task: TX task started");
    while(app->running != 0U)
    {
        qSts = osMessageQueueGet(app->txQ, &msg, NULL, 1000U); /* was osWaitForever */

        if(qSts != osOK)
        {
            continue;
        }

        if(msg.len > 0U)
        {
            if(bspUartSendSync(&app->cfg.uartHandle, msg.buf, (size_t)msg.len, 500U) != BSP_ERR_STS_OK)
            {
                ALOGE("UART TX send failed, len=%u", (unsigned int)msg.len);
            }
        }
    }

    ALOGW("uart_app_tx_task: TX task stopped");
    osDelay(100U); /* Let log flush */
    osThreadExit();
}

/* =========================================================================
 * PUBLIC API
 * ========================================================================= */
bsp_err_sts_t uart_app_send(uart_app_t* app, const uint8_t* frame, uint16_t len)
{
    uart_app_tx_msg_t msg;

    if((app == NULL) || (frame == NULL) || (len == 0U) ||
       (len > (uint16_t)sizeof(msg.buf)))
    {
        ALOGE("uart_app_send: Invalid param");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if((app->running == 0U) || (s_uartFailStopped != 0U) || (app->txQ == NULL))
    {
        ALOGE("uart_app_send: UART app not running");
        return BSP_ERR_STS_FAIL;
    }

    /* Build TX message */
    (void)memset(&msg, 0, sizeof(msg));

    /* Copy frame into TX message */
    (void)memcpy(msg.buf, frame, (size_t)len);
    msg.len = len;

    if(osMessageQueuePut(app->txQ, &msg, 0U, 0U) != osOK)
    {
        ALOGE("uart_app_send: TX queue post failed, len=%u", (uint16_t)len);
        return BSP_ERR_STS_FAIL;
    }

    ALOGD("uart_app_send: Posted frame to TX queue, len=%u frame%.*s",
          (uint16_t)len, (uint16_t)len, (const char*)msg.buf);

    return BSP_ERR_STS_OK;
}
/* ------------------------------------------------------------------ */
bsp_err_sts_t uart_app_send_status(uart_app_t* app, rc_intf_t intf, rc_sts_sub_t sub)
{
    uint8_t buf[1U];
    uint8_t len;

    len = rc_encode_simple(buf, intf, eRcTypeStatus, (uint8_t)sub);
    if(len == 0U)
    {
        ALOGE("uart_app_send_status: Status frame encode failed");
        return BSP_ERR_STS_FAIL;
    }
    return uart_app_send(app, buf, (uint16_t)len);
}
/* ------------------------------------------------------------------ */
bsp_err_sts_t
uart_app_send_data(uart_app_t* app, rc_intf_t intf, uint8_t dataSub, const uint8_t* payload, uint16_t payloadLen)
{
    uint16_t frameLen;

    /* Lock scratch buffer - multiple tasks may call this concurrently */
    if(osMutexAcquire(app->txMutex, 100U) != osOK)
    {
        ALOGE("uart_app_send_data: TX mutex acquire timeout");
        return BSP_ERR_STS_TIMEOUT;
    }

    frameLen = rc_encode_data(gTxEncodeBuf, intf, dataSub, payload, payloadLen);

    (void)osMutexRelease(app->txMutex);

    if(frameLen == 0U)
    {
        ALOGE("uart_app_send_data: Data frame encode failed");
        return BSP_ERR_STS_FAIL;
    }

    return uart_app_send(app, gTxEncodeBuf, frameLen);
}
/* ------------------------------------------------------------------ */
bsp_err_sts_t uart_app_deinit(uart_app_t* app)
{
    if(app == NULL)
    {
        ALOGE("uart_app_deinit: app is NULL");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    app->running = 0U;

    /* Stop incoming RX first */
    (void)bspUartIoctl(&app->cfg.uartHandle, eBspUartCancelRx, NULL);
    osDelay(20U);

    /* Ensure worker tasks are gone before deleting queues/mutex */
    if(app->rxTaskId != NULL)
    {
        (void)osThreadTerminate(app->rxTaskId);
        app->rxTaskId = NULL;
    }

    if(app->txTaskId != NULL)
    {
        (void)osThreadTerminate(app->txTaskId);
        app->txTaskId = NULL;
    }

    if(app->txMutex != NULL)
    {
        osMutexId_t m = app->txMutex;
        app->txMutex  = NULL;
        (void)osMutexDelete(m);
    }

    if(app->txQ != NULL)
    {
        osMessageQueueId_t q = app->txQ;
        app->txQ             = NULL;
        (void)osMessageQueueDelete(q);
    }

    if(app->rxQ != NULL)
    {
        osMessageQueueId_t q = app->rxQ;
        app->rxQ             = NULL;
        (void)osMessageQueueDelete(q);
    }

    (void)bspUartDeInit(&app->cfg.uartHandle);

    s_uartRxErrorCount    = 0U;
    s_uartParseErrorCount = 0U;
    s_uartFailStopped     = 0U;

    return BSP_ERR_STS_OK;
}

/* ------------------------------------------------------------------ */
/*
uart_app_init(app, cfg)
    │
    ├── init UART hardware (bspUartInit)
    │
    ├── create RX queue  (hold raw byte chunks)
    ├── create TX queue  (hold complete frames)
    ├── create TX mutex  (protect shared TX buffer)
    │
    ├── set RX callback  (bspUartSetCallback)
    │     └─ register uart_rx_cb
    │
    ├── arm first async RX  (bspUartReadAsync)
    │     └─ tell BSP to start listening
    │
    ├── spawn RX task
    │     └─ loop: decode frames, dispatch
    │
    └── spawn TX task
          └─ loop: send from queue via UART
*/
bsp_err_sts_t uart_app_init(uart_app_t* app, const uart_app_cfg_t* cfg)
{
    if((app == NULL) || (cfg == NULL))
    {
        ALOGE("uart_app_init: Invalid param");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    s_uartRxErrorCount    = 0U;
    s_uartParseErrorCount = 0U;
    s_uartFailStopped     = 0U;

    /* Zero-fill context */
    (void)memset(app, 0, sizeof(uart_app_t));

    /* Copy config */
    (void)memcpy(&app->cfg, cfg, sizeof(uart_app_cfg_t));

    /* Init UART hardware via BSP */
    if(bspUartInit(&app->cfg.uartHandle) != BSP_ERR_STS_OK)
    {
        ALOGE("uart_app_init: bspUartInit failed");
        return BSP_ERR_STS_FAIL;
    }

    /* Create RX queue */
    g_appQueueCfg[APP_Q_OWNER_UART_APP_RX].msg_size_bytes = sizeof(uart_rx_msg_t);
    app->rxQ = osMessageQueueNew(g_appQueueCfg[APP_Q_OWNER_UART_APP_RX].msg_count,
                                 sizeof(uart_rx_msg_t),
                                 &g_appQueueCfg[APP_Q_OWNER_UART_APP_RX].attr);
    if(app->rxQ == NULL)
    {
        ALOGE("uart_app_init: RX queue creation failed");
        (void)bspUartDeInit(&app->cfg.uartHandle);
        return BSP_ERR_STS_FAIL;
    }

    /* Create TX queue */
    g_appQueueCfg[APP_Q_OWNER_UART_APP_TX].msg_size_bytes = sizeof(uart_app_tx_msg_t);
    app->txQ = osMessageQueueNew(g_appQueueCfg[APP_Q_OWNER_UART_APP_TX].msg_count,
                                 sizeof(uart_app_tx_msg_t),
                                 &g_appQueueCfg[APP_Q_OWNER_UART_APP_TX].attr);
    if(app->txQ == NULL)
    {
        ALOGE("uart_app_init: TX queue creation failed");
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        return BSP_ERR_STS_FAIL;
    }

    /* Create TX mutex */
    app->txMutex = osMutexNew(&g_appMutexCfg[APP_MUTEX_OWNER_UART_APP_TX].attr);
    if(app->txMutex == NULL)
    {
        ALOGE("uart_app_init: TX mutex creation failed");
        (void)osMessageQueueDelete(app->txQ);
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        return BSP_ERR_STS_FAIL;
    }

    app->running           = 1U;
    app->lastHeartbeatTick = osKernelGetTickCount();

    /* Register async RX callback */
    if(bspUartSetCallback(&app->cfg.uartHandle, uart_rx_cb, app) != BSP_ERR_STS_OK)
    {
        (void)osMutexDelete(app->txMutex);
        (void)osMessageQueueDelete(app->txQ);
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        ALOGE("uart_app_init: UART RX callback registration failed");
        return BSP_ERR_STS_FAIL;
    }

    /* Arm first async RX - will auto re-arm in callback */
    if(bspUartReadAsync(&app->cfg.uartHandle, gRxAsyncBuf, sizeof(gRxAsyncBuf)) != BSP_ERR_STS_OK)
    {
        (void)osMutexDelete(app->txMutex);
        (void)osMessageQueueDelete(app->txQ);
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        ALOGE("uart_app_init: UART async RX arm failed");
        return BSP_ERR_STS_FAIL;
    }

    /* Start RX task */
    app->rxTaskId =
    osThreadNew(uart_rx_task, app, &g_appThreadCfg[APP_THREAD_OWNER_UART_APP_RX].attr);
    if(app->rxTaskId == NULL)
    {
        (void)osMutexDelete(app->txMutex);
        (void)osMessageQueueDelete(app->txQ);
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        ALOGE("uart_app_init: RX task creation failed");
        return BSP_ERR_STS_FAIL;
    }

    /* Start TX task */
    app->txTaskId =
    osThreadNew(uart_tx_task, app, &g_appThreadCfg[APP_THREAD_OWNER_UART_APP_TX].attr);
    if(app->txTaskId == NULL)
    {
        app->running = 0U;
        osDelay(100U);
        (void)osMutexDelete(app->txMutex);
        (void)osMessageQueueDelete(app->txQ);
        (void)osMessageQueueDelete(app->rxQ);
        (void)bspUartDeInit(&app->cfg.uartHandle);
        ALOGE("uart_app_init: TX task creation failed");
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}