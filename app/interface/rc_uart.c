/*
 * =========================================================================
 * @file    rc_uart.c
 *
 * @brief   RideCast Host UART Interface
 *
 * Description:
 *   Provides the UART interface between Host and RideCast.
 *
 * Responsibilities:
 *   - Initialize / deinitialize Host UART
 *   - Receive UART data through BSP callback
 *   - Queue received raw UART chunks
 *   - Accumulate bytes until a complete RC frame is available
 *   - Validate / decode RC frame
 *   - Handle UART link-level PING / PONG
 *   - Forward valid non-link frames to RideCast Core
 *   - Transmit frames received from RideCast TX queue
 *
 * This module does NOT:
 *   - Select Wi-Fi / BLE / BT Classic
 *   - Interpret application payload
 *   - Handle navigation / media / notification / calling
 *   - Implement RideCast transport logic
 *
 * =========================================================================
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "rc_config.h"
#include "rc_uart.h"
#include "rc_protocol.h"

#include "bsp_uart.h"
#include "bsp_os.h"


/* =========================================================================
 * Configuration
 * =========================================================================
 */

#define RC_UART_RX_BUF_LEN    (256U)
#define RC_UART_TX_BUF_LEN    (RC_FRAME_MAX_LEN)

#define RC_UART_TX_TIMEOUT_MS (500U)
#define RC_UART_HEARTBEAT_MS  (10000U)


/* =========================================================================
 * RX / TX Message
 * =========================================================================
 */

typedef struct
{
    uint8_t buf[RC_UART_RX_BUF_LEN];
    uint16_t len;

} rc_uart_rx_msg_t;


typedef struct
{
    uint8_t buf[RC_UART_TX_BUF_LEN];
    uint16_t len;

} rc_uart_tx_msg_t;


/*
 * Message passed from RC UART to RideCast Core.
 *
 * Core receives the complete encoded RC frame.
 * Core is responsible for deciding what the frame means.
 */
typedef struct
{
    uint8_t buf[RC_FRAME_MAX_LEN];
    uint16_t len;

} rc_core_rx_msg_t;


/* =========================================================================
 * RX Frame Accumulator
 * =========================================================================
 */

typedef struct
{
    uint8_t buf[RC_FRAME_MAX_LEN];

    uint16_t pos;
    uint16_t expectedLen;

    bool active;

} rc_uart_rx_accum_t;


/* =========================================================================
 * Static Buffers
 * =========================================================================
 */

/*
 * Buffer owned by BSP during asynchronous RX.
 */
static uint8_t g_rcUartRxAsyncBuf[RC_UART_RX_BUF_LEN];


/*
 * Local buffer used for locally generated PONG.
 */
static uint8_t g_rcUartTxBuf[RC_UART_TX_BUF_LEN];


/* =========================================================================
 * Static State
 * =========================================================================
 */

static volatile bool g_rcUartRunning = false;

static uint32_t g_rcUartLastHeartbeatTick = 0U;


/* =========================================================================
 * Queue Handles
 * =========================================================================
 */

static osMessageQueueId_t g_rcUartRxQueue = NULL;

static osMessageQueueId_t g_rcUartTxQueue = NULL;

static osMessageQueueId_t g_rcCoreRxQueue = NULL;


/* =========================================================================
 * Forward Declarations
 * =========================================================================
 */

static bspUartHandle_t* rc_uart_get_handle(void);
static void
rc_uart_rx_callback(plt_err_sts_t status, uint8_t* data, uint16_t data_len, void* userContext);

static void rc_uart_process_rx_message(rc_uart_rx_accum_t* accumulator,
                                       const rc_uart_rx_msg_t* msg);

static bool
rc_uart_accumulate_byte(rc_uart_rx_accum_t* accumulator, uint8_t byte, rc_frame_t* frame);

static void
rc_uart_handle_frame(const uint8_t* rawFrame, uint16_t frameLen, const rc_frame_t* frame);

static void rc_uart_handle_ping(const rc_frame_t* frame);

static void rc_uart_handle_pong(const rc_frame_t* frame);

static void rc_uart_send_pong(const rc_frame_t* frame);

static void rc_uart_forward_to_rc_core(const uint8_t* frame, uint16_t frameLen);

static void rc_uart_reset_accumulator(rc_uart_rx_accum_t* accumulator);


/* =========================================================================
 * TX Task
 * =========================================================================
 */

void vRcUartTxTask(void* arg)
{
    rc_uart_tx_msg_t msg;

    (void)arg;

    while(g_rcUartRunning == true)
    {
        osStatus_t queueStatus;

        queueStatus = osMessageQueueGet(g_rcUartTxQueue, &msg, NULL, osWaitForever);

        if(queueStatus != osOK)
        {
            continue;
        }

        if(msg.len == 0U)
        {
            continue;
        }

        /*
         * Single task owns physical UART TX.
         */
        (void)bspUartSendSync(rc_uart_get_handle(), msg.buf, (size_t)msg.len,
                              RC_UART_TX_TIMEOUT_MS);
    }

    osThreadExit();
}


/* =========================================================================
 * Public Send API
 * =========================================================================
 */

plt_err_sts_t rc_uart_send(const uint8_t* frame, uint16_t len)
{
    rc_uart_tx_msg_t msg;

    if((frame == NULL) || (len == 0U) || (len > (uint16_t)sizeof(msg.buf)))
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    if(g_rcUartRunning == false)
    {
        return PLT_ERR_STS_FAIL;
    }

    /*
     * Copy complete encoded frame into TX queue message.
     */
    (void)memcpy(msg.buf, frame, (size_t)len);

    msg.len = len;

    /*
     * All UART TX goes through one TX task.
     */
    if(osMessageQueuePut(g_rcUartTxQueue, &msg, 0U, 0U) != osOK)
    {
        return PLT_ERR_STS_FAIL;
    }

    return PLT_ERR_STS_OK;
}

/* =========================================================================
 * RX Task
 * =========================================================================
 */

void vRcUartRxTask(void* arg)
{
    rc_uart_rx_accum_t accumulator;
    rc_uart_rx_msg_t msg;

    (void)arg;

    (void)memset(&accumulator, 0, sizeof(accumulator));

    while(g_rcUartRunning == true)
    {
        osStatus_t queueStatus;

        /*
         * Wait for raw UART data.
         *
         * Timeout allows periodic heartbeat handling.
         */
        queueStatus = osMessageQueueGet(g_rcUartRxQueue, &msg, NULL, RC_UART_HEARTBEAT_MS);

        /*
         * Heartbeat processing.
         */
        {
            uint32_t now;

            now = osKernelGetTickCount();

            if((now - g_rcUartLastHeartbeatTick) >= RC_UART_HEARTBEAT_MS)
            {
                /*
                 * Heartbeat functionality can be added here.
                 *
                 * Example:
                 *
                 *     Generate PING
                 *     -> rc_uart_send()
                 *
                 * For now this only maintains the timer.
                 */
                g_rcUartLastHeartbeatTick = now;
            }
        }

        if(queueStatus != osOK)
        {
            continue;
        }

        rc_uart_process_rx_message(&accumulator, &msg);
    }

    osThreadExit();
}


/* =========================================================================
 * Initialization
 * =========================================================================
 */

plt_err_sts_t rc_uart_init(void)
{
    plt_err_sts_t status;
    bspUartHandle_t* uartHandle;

    uartHandle = rc_uart_get_handle();

    if(uartHandle == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    /*
     * Get queues from centralized application map.
     */
    g_rcUartRxQueue = g_appQueueCfg[APP_Q_OWNER_RC_UART_RX].handle;

    g_rcUartTxQueue = g_appQueueCfg[APP_Q_OWNER_RC_UART_TX].handle;

    g_rcCoreRxQueue = g_appQueueCfg[APP_Q_OWNER_RC_CORE_RX].handle;

    if((g_rcUartRxQueue == NULL) || (g_rcUartTxQueue == NULL) || (g_rcCoreRxQueue == NULL))
    {
        return PLT_ERR_STS_NOT_INITIALIZED;
    }

    /*
     * Initialize UART hardware.
     */
    status = bspUartInit(uartHandle);

    if(status != PLT_ERR_STS_OK)
    {
        return status;
    }

    /*
     * Register asynchronous RX callback.
     */
    status = bspUartSetCallback(uartHandle, rc_uart_rx_callback, NULL);

    if(status != PLT_ERR_STS_OK)
    {
        (void)bspUartDeInit(uartHandle);
        return status;
    }

    /*
     * Start asynchronous RX.
     */
    status = bspUartReadAsync(uartHandle, g_rcUartRxAsyncBuf, sizeof(g_rcUartRxAsyncBuf));

    if(status != PLT_ERR_STS_OK)
    {
        (void)bspUartSetCallback(uartHandle, NULL, NULL);

        (void)bspUartDeInit(uartHandle);

        return status;
    }

    g_rcUartRunning = true;

    g_rcUartLastHeartbeatTick = osKernelGetTickCount();

    return PLT_ERR_STS_OK;
}


/* =========================================================================
 * Deinitialization
 * =========================================================================
 */

plt_err_sts_t rc_uart_deinit(void)
{
    bspUartHandle_t* uartHandle;

    uartHandle = rc_uart_get_handle();

    if(uartHandle == NULL)
    {
        return PLT_ERR_STS_INVALID_PARAM;
    }

    g_rcUartRunning = false;

    /*
     * Stop callback.
     */
    (void)bspUartSetCallback(uartHandle, NULL, NULL);

    /*
     * Cancel pending RX.
     */
    (void)bspUartIoctl(uartHandle, eBspUartCancelRx, NULL);

    return bspUartDeInit(uartHandle);
}


/* =========================================================================
 * UART Handle
 * =========================================================================
 */

static bspUartHandle_t* rc_uart_get_handle(void)
{
    /*
     * UART configuration is owned by rc_config.c.
     */
    return &g_appUartHandleCfg[APP_UART_OWNER_HOST_COMM];
}


/* =========================================================================
 * Reset RX Accumulator
 * =========================================================================
 */

static void rc_uart_reset_accumulator(rc_uart_rx_accum_t* accumulator)
{
    if(accumulator == NULL)
    {
        return;
    }

    accumulator->pos         = 0U;
    accumulator->expectedLen = 0U;
    accumulator->active      = false;
}


/* =========================================================================
 * RX Callback
 * =========================================================================
 *
 * BSP UART:
 *
 *      UART
 *       |
 *       v
 *   RX callback
 *       |
 *       v
 *   RC UART RX Queue
 *
 * No protocol processing is performed inside callback.
 *
 * =========================================================================
 */
static void
rc_uart_rx_callback(plt_err_sts_t status, uint8_t* data, uint16_t data_len, void* userContext)
{
    rc_uart_rx_msg_t msg;
    bspUartHandle_t* uartHandle;

    (void)userContext;

    if(g_rcUartRunning == false)
    {
        return;
    }

    if((status != PLT_ERR_STS_OK) || (data == NULL) || (data_len == 0U))
    {
        return;
    }

    if(data_len > (uint16_t)sizeof(msg.buf))
    {
        return;
    }

    /*
     * Copy data because BSP RX buffer can be reused.
     */
    (void)memcpy(msg.buf, data, (size_t)data_len);

    msg.len = data_len;

    /*
     * Put raw UART chunk into UART RX queue.
     */
    if(osMessageQueuePut(g_rcUartRxQueue, &msg, 0U, 0U) != osOK)
    {
        /*
         * Queue full.
         *
         * Do not process protocol here.
         */
    }

    /*
     * Re-arm asynchronous RX.
     */
    uartHandle = rc_uart_get_handle();

    if(uartHandle != NULL)
    {
        (void)bspUartReadAsync(uartHandle, g_rcUartRxAsyncBuf, sizeof(g_rcUartRxAsyncBuf));
    }
}


/* =========================================================================
 * Process RX Message
 * =========================================================================
 */

static void rc_uart_process_rx_message(rc_uart_rx_accum_t* accumulator,
                                       const rc_uart_rx_msg_t* msg)
{
    rc_frame_t frame;
    uint16_t i;

    if((accumulator == NULL) || (msg == NULL))
    {
        return;
    }

    for(i = 0U; i < msg->len; i++)
    {
        if(rc_uart_accumulate_byte(accumulator, msg->buf[i], &frame) == true)
        {
            /*
             * Complete valid frame received.
             */
            rc_uart_handle_frame(accumulator->buf, accumulator->pos, &frame);

            /*
             * Ready for next frame.
             */
            rc_uart_reset_accumulator(accumulator);
        }
    }
}


/* =========================================================================
 * Accumulate One Byte
 * =========================================================================
 */

static bool rc_uart_accumulate_byte(rc_uart_rx_accum_t* accumulator, uint8_t byte, rc_frame_t* frame)
{
    uint16_t frameLength;

    if((accumulator == NULL) || (frame == NULL))
    {
        return false;
    }

    /*
     * Start new frame.
     */
    if(accumulator->active == false)
    {
        accumulator->active      = true;
        accumulator->pos         = 0U;
        accumulator->expectedLen = 0U;
    }

    /*
     * Prevent buffer overflow.
     */
    if(accumulator->pos >= (uint16_t)sizeof(accumulator->buf))
    {
        rc_uart_reset_accumulator(accumulator);

        return false;
    }

    accumulator->buf[accumulator->pos] = byte;
    accumulator->pos++;

    /*
     * Once fixed header is available,
     * determine complete frame length.
     */
    if(accumulator->pos >= RC_FRAME_FIXED_LEN)
    {
        frameLength = rc_get_frame_length(accumulator->buf, accumulator->pos);

        if(frameLength == 0U)
        {
            /*
             * Invalid frame length.
             */
            rc_uart_reset_accumulator(accumulator);

            return false;
        }

        /*
         * Make sure protocol frame fits
         * into accumulator.
         */
        if(frameLength > (uint16_t)sizeof(accumulator->buf))
        {
            rc_uart_reset_accumulator(accumulator);

            return false;
        }

        accumulator->expectedLen = frameLength;

        /*
         * Complete frame received.
         */
        if(accumulator->pos >= accumulator->expectedLen)
        {
            if(rc_frame_decode(accumulator->buf, accumulator->expectedLen, frame) == eRcStatusOk)
            {
                return true;
            }

            /*
             * CRC / protocol validation failed.
             */
            rc_uart_reset_accumulator(accumulator);
        }
    }

    return false;
}


/* =========================================================================
 * Handle Complete Frame
 * =========================================================================
 */

static void rc_uart_handle_frame(const uint8_t* rawFrame, uint16_t frameLen, const rc_frame_t* frame)
{
    if((rawFrame == NULL) || (frame == NULL) || (frameLen == 0U))
    {
        return;
    }

    /*
     * Only UART link-level messages are handled here.
     */
    switch(frame->messageId)
    {
        case eRcMsgPing:
        {
            rc_uart_handle_ping(frame);
            break;
        }

        case eRcMsgPong:
        {
            rc_uart_handle_pong(frame);
            break;
        }

        default:
        {
            /*
             * Everything else belongs to RideCast Core.
             *
             * UART does not decide whether it is:
             * Wi-Fi / BLE / BT / Navigation / Media / etc.
             */
            rc_uart_forward_to_rc_core(rawFrame, frameLen);

            break;
        }
    }
}


/* =========================================================================
 * Handle PING
 * =========================================================================
 */

static void rc_uart_handle_ping(const rc_frame_t* frame)
{
    if(frame == NULL)
    {
        return;
    }

    /*
     * PING is a Host <-> RideCast link-level message.
     *
     * Respond locally.
     */
    rc_uart_send_pong(frame);
}


/* =========================================================================
 * Handle PONG
 * =========================================================================
 */

static void rc_uart_handle_pong(const rc_frame_t* frame)
{
    if(frame == NULL)
    {
        return;
    }

    /*
     * Host responded to our heartbeat.
     *
     * Update link state here if required.
     *
     * Example:
     *
     * g_rcHostConnected = true;
     * g_rcLastPongTick = osKernelGetTickCount();
     */
}


/* =========================================================================
 * Send PONG
 * =========================================================================
 */

static void rc_uart_send_pong(const rc_frame_t* pingFrame)
{
    size_t frameLen;

    if(pingFrame == NULL)
    {
        return;
    }

    /*
     * Generate PONG.
     *
     * Use same transport and sequence number
     * as received PING.
     */
    frameLen = rc_encode_simple(g_rcUartTxBuf, sizeof(g_rcUartTxBuf), eRcTypeAck,
                                eRcMsgPong, pingFrame->transport, pingFrame->sequence);

    if(frameLen == 0U)
    {
        return;
    }

    /*
     * Use normal TX path.
     */
    (void)rc_uart_send(g_rcUartTxBuf, (uint16_t)frameLen);
}


/* =========================================================================
 * Forward Frame To RideCast Core
 * =========================================================================
 */

static void rc_uart_forward_to_rc_core(const uint8_t* frame, uint16_t frameLen)
{
    rc_core_rx_msg_t msg;

    if((frame == NULL) || (frameLen == 0U) || (frameLen > (uint16_t)sizeof(msg.buf)))
    {
        return;
    }

    /*
     * Copy complete encoded frame.
     */
    (void)memcpy(msg.buf, frame, (size_t)frameLen);

    msg.len = frameLen;

    /*
     * IMPORTANT:
     *
     * This is NOT the UART RX queue.
     *
     * This queue belongs to RideCast Core.
     */
    if(osMessageQueuePut(g_rcCoreRxQueue, &msg, 0U, 0U) != osOK)
    {
        /*
         * Core RX queue full.
         */
    }
}
