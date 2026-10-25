
/**
 * ============================================================================
 * @file    uart_app.h
 * @brief   RideCast UART application communication interface.
 *
 * @details
 * Provides UART communication between the vehicle cluster (MASTER)
 * and RideCast (SLAVE).
 *
 * RESPONSIBILITIES:
 *   - Use the BSP UART driver for hardware communication.
 *   - Receive raw UART bytes through the asynchronous BSP callback.
 *   - Queue received bytes for application-level frame processing.
 *   - Accumulate and validate complete rc_protocol frames.
 *   - Dispatch decoded frames to the registered transport callbacks.
 *   - Serialize outgoing frames through one UART TX worker.
 *   - Report UART communication health to the vehicle cluster.
 *
 * ============================================================================
 * COMMUNICATION ARCHITECTURE
 * ============================================================================
 *
 *                    VEHICLE CLUSTER
 *                         MASTER
 *                           |
 *                           | UART binary protocol
 *                           |
 *             +-------------+-------------+
 *             |                           |
 *             v                           ^
 *      +-------------+             +-------------+
 *      | BSP UART RX |             | BSP UART TX |
 *      | Async       |             | Sync Send   |
 *      +------+------+             +------^------+
 *             |                           |
 *             v                           |
 *      +-------------+             +-------------+
 *      | RX Callback |             | UART TX Task|
 *      +------+------+             +------^------+
 *             |                           |
 *             v                           |
 *      +-------------+             +-------------+
 *      | RX Queue    |             | TX Queue    |
 *      +------+------+             +------^------+
 *             |                           |
 *             v                           |
 *      +-------------+                    |
 *      | UART RX Task|                    |
 *      |-------------|                    |
 *      | Accumulate  |                    |
 *      | Decode      |                    |
 *      | Validate    |                    |
 *      | Dispatch    |                    |
 *      +------+------+                    |
 *             |                           |
 *             v                           |
 *      +-------------------------------+  |
 *      | RideCast Application          |  |
 *      |-------------------------------|  |
 *      | Wi-Fi callback                |  |
 *      | BLE callback                  |  |
 *      | BT Classic callback           |  |
 *      |                               |  |
 *      | uart_app_send() --------------+--+
 *      +-------------------------------+
 *
 * ============================================================================
 * RTOS OBJECT OWNERSHIP
 * ============================================================================
 *
 * app_main.c:
 *   - Creates application RTOS queues, mutexes and worker threads.
 *   - Stores created handles in g_appQueueCfg, g_appMutexCfg and
 *     g_appThreadCfg.
 *
 * uart_app_init():
 *   - Initializes the UART application context.
 *   - Obtains existing RTOS object handles from the application maps.
 *   - Initializes UART hardware through the BSP.
 *   - Registers and arms asynchronous UART reception.
 *
 * uart_app_deinit():
 *   - Stops UART processing and deinitializes UART hardware.
 *   - Does not delete RTOS objects owned by the central OS maps.
 *
 * ============================================================================
 * FRAME FORMAT
 * ============================================================================
 *
 * All frame types use the rc_protocol.h wire format:
 *
 *   +----------+------------+-----------+----------+-------------+
 *   | Header   | Message ID | Transport | Sequence | Payload Len |
 *   | 1 byte   | 1 byte     | 1 byte    | 2 bytes  | 2 bytes     |
 *   +----------+------------+-----------+----------+-------------+
 *   | Payload (0..RC_MAX_PAYLOAD_LEN bytes)        | CRC16       |
 *   | Variable length                             | 2 bytes     |
 *   +---------------------------------------------+-------------+
 *
 * Minimum frame size: 9 bytes.
 *
 * A UART callback may receive partial frames or multiple frames.
 * The RX worker must accumulate exactly one complete frame before
 * calling rc_frame_decode().
 *
 * ============================================================================
 * USAGE
 * ============================================================================
 *
 * 1. app_main.c creates the required application RTOS objects.
 * 2. ridecast_init() prepares uart_app_cfg_t.
 * 3. uart_app_init() initializes UART communication.
 * 4. UART RX worker decodes frames and invokes registered callbacks.
 * 5. Application modules call uart_app_send() for outgoing frames.
 * 6. uart_app_deinit() stops UART communication when required.
 *
 * ============================================================================
 * WARNINGS
 * ============================================================================
 *
 * @warning
 *   Wireless interfaces must not be enabled automatically by UART
 *   initialization. The cluster controls wireless START/STOP.
 *
 *   Callback functions execute in the UART RX task context.
 *   They must not perform long blocking operations.
 *
 *   The payload pointer in rc_frame_t references the RX frame buffer.
 *   Callbacks must copy the payload if it is needed after return.
 *
 *   UART queue messages have bounded storage. A valid protocol frame
 *   larger than the configured application buffer requires an explicit
 *   fragmentation/reassembly strategy or must be rejected.
 *
 *   The BSP UART callback context depends on the BSP implementation.
 *   ISR-unsafe CMSIS operations must not be called from interrupt context.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Updated protocol, configuration and RTOS ownership contract.
 * 2026-05-23  Initial development.
 * ============================================================================
 */

#ifndef UART_APP_H
#define UART_APP_H

#include <stdint.h>
#include <stdbool.h>

#include "app_config.h"
#include "cmsis_os2.h"
#include "bsp_uart.h"
#include "bsp_err_sts.h"
#include "rc_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define UART_APP_TX_BUF_LEN configUART_APP_TX_BUF_LEN
#define UART_APP_RX_BUF_LEN configUART_APP_RX_BUF_LEN

    /* =========================================================================
     * INTERFACE DISPATCH CALLBACK
     * ========================================================================= */

    /**
     * @brief Called when a valid protocol frame arrives for a transport.
     *
     * @param[in] frame Decoded protocol frame.
     * @param[in] user  Registered application context.
     *
     * @note
     *   frame->payload points into the UART RX accumulation buffer.
     *   The pointer is valid only for the duration of this callback.
     */
    typedef void (*uart_app_intf_cb_t)(const rc_frame_t* frame, void* user);


    /* =========================================================================
     * UART RX QUEUE MESSAGE
     * ========================================================================= */

    /**
     * @brief Raw bytes received from one asynchronous UART callback.
     *
     * @note
     *   A queue message is not necessarily a complete protocol frame.
     */
    typedef struct
    {
        uint8_t buf[UART_APP_RX_BUF_LEN];
        uint16_t len;
    } uart_app_msg_t;


    /* =========================================================================
     * UART APPLICATION CONFIGURATION
     * ========================================================================= */

    /**
     * @brief UART hardware settings and transport dispatch callbacks.
     *
     * @note
     *   The current implementation stores bspUartHandle_t by value.
     *   The UART implementation must use the same handle consistently
     *   for BSP initialization, sending and receiving.
     */
    typedef struct
    {
        /* UART hardware configuration. */
        bspUartHandle_t uartHandle;

        /* Transport callbacks. */
        uart_app_intf_cb_t onWifiFrame;
        uart_app_intf_cb_t onBleFrame;
        uart_app_intf_cb_t onBtFrame;

        /* Callback-specific application contexts. */
        void* wifiUser;
        void* bleUser;
        void* btUser;

    } uart_app_cfg_t;


    /* =========================================================================
     * UART APPLICATION CONTEXT
     * ========================================================================= */

    /**
     * @brief Runtime state for the UART application service.
     *
     * @note
     *   RTOS handles refer to centrally created objects.
     *   They are not owned by this context.
     */
    typedef struct
    {
        uart_app_cfg_t cfg;

        /* Centrally created UART worker thread handles. */
        osThreadId_t rxTaskId;
        osThreadId_t txTaskId;

        /* Centrally created queues. */
        osMessageQueueId_t rxQ;
        osMessageQueueId_t txQ;

        /* Centrally created TX mutex. */
        osMutexId_t mutex;

        /* UART application running state. */
        volatile uint8_t running;

        /* RX accumulation workspace. */
        uint8_t rxBuf[UART_APP_RX_BUF_LEN];

        /* Heartbeat timing, expressed in kernel ticks. */
        uint32_t lastHeartbeatTick;

    } uart_app_t;


    /* =========================================================================
     * PUBLIC API
     * ========================================================================= */
    /**
     *
     */
    void vAppUartRxTask(void* arg);

    /**
     *
     */
    void vAppUartTxTask(void* arg);

    /**
     * @brief Initialize the UART application communication service.
     *
     * @details
     * Obtains pre-created RTOS objects from the application configuration
     * maps and initializes UART hardware and asynchronous reception.
     *
     * @param[in,out] app UART application context.
     * @param[in]     cfg UART hardware and callback configuration.
     *
     * @return RET_ERR_STS_OK on success or an appropriate error status.
     *
     * @warning
     *   The required RX/TX queues, TX mutex and worker threads must
     *   already have been created by the system initialization logic.
     */
    ret_err_sts_t uart_app_init(void);


    /**
     * @brief Stop and deinitialize UART application communication.
     *
     * @param[in,out] app UART application context.
     *
     * @return RET_ERR_STS_OK on success or an error status.
     *
     * @note
     *   Does not delete centrally owned RTOS objects.
     */
    ret_err_sts_t uart_app_deinit(void);


    /**
     * @brief Queue a complete, pre-encoded rc_protocol frame for transmission.
     *
     * @details
     * Copies the provided frame into the TX message queue. The UART TX
     * worker serializes all transmissions through the BSP driver.
     *
     * @param[in,out] app   UART application context.
     * @param[in]     frame Complete encoded protocol frame.
     * @param[in]     len   Frame length in bytes.
     *
     * @return RET_ERR_STS_OK if queued, or an error status.
     *
     * @note
     *   The call reports successful queueing, not physical transmission.
     *   The frame must fit within configUART_APP_TX_BUF_LEN.
     *
     * @warning
     *   This API is intended for task context.
     *   It must not be called from an ISR.
     */
    ret_err_sts_t uart_app_send(const uint8_t* frame, uint16_t len);


#ifdef __cplusplus
}
#endif

#endif /* UART_APP_H */
