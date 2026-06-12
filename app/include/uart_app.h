/**
 * @file    uart_app.h
 * @brief   UART Application Task
 *
 * RESPONSIBILITIES:
 *   - Init UART using bsp_uart API
 *   - Send heartbeat to Host every 10 seconds
 *   - Receive frames from Host, decode rc_protocol header
 *   - Dispatch decoded frame to registered interface callback
 *     (WiFi / BLE / BT Classic handler)
 *   - Send rc_protocol encoded frames to Host from any task
 *
 * FRAME FORMAT (rc_protocol.h):
 *   CMD/STATUS/ACK : 1 byte  (header only)
 *   DATA / CMD+params: header(1) + length(2) + payload(N)
 *
 * USAGE:
 *   1. Fill uart_app_cfg_t (UART pins/baud, interface callbacks)
 *   2. Call uart_app_init()  -> starts RX task + heartbeat timer
 *   3. Incoming frames auto-dispatched to registered callbacks
 *   4. Call uart_app_send() from any task to send frame to Host
 */

#ifndef UART_APP_H
#define UART_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "bsp_uart.h"
#include "bsp_err_sts.h"
#include "rc_protocol.h"


/* =========================================================================
 * LIMITS
 * ========================================================================= */
#define UART_APP_RX_BUF_LEN    (appCfg_HOST_UART_RXTX_BUF_SIZE)
#define UART_APP_TX_BUF_LEN    (appCfg_HOST_UART_RXTX_BUF_SIZE)
#define UART_APP_TX_Q_LEN      (8U)
#define UART_APP_HEARTBEAT_MS  (10000U) /**< Heartbeat to Host every 10s  */
#define UART_APP_RX_TIMEOUT_MS (100U)   /**< Sync RX poll timeout         */
#define UART_APP_RX_Q_LEN      (16U)    /* async RX chunks queue depth  */
#define UART_APP_TX_Q_LEN      (8U)
    /* =========================================================================
     * INTERFACE DISPATCH CALLBACK
     *
     * Registered per interface (WiFi / BLE / BT Classic).
     * Called from uart_app RX task when a frame for that interface arrives.
     *
     * @param frame    Decoded rc_protocol frame (payload points into RX buffer)
     * @param user     Application context registered with this callback
     * ========================================================================= */
    typedef void (*uart_app_intf_cb_t)(const rc_frame_t* frame, void* user);

    /* =========================================================================
     * TX MESSAGE - posted to TX queue from any task
     * ========================================================================= */
    typedef struct
    {
        uint8_t buf[UART_APP_TX_BUF_LEN]; /**< Pre-encoded rc_protocol frame  */
        uint16_t len;                     /**< Byte count                     */
    } uart_app_tx_msg_t;

    /* =========================================================================
     * CONFIG - filled once at boot
     * ========================================================================= */
    typedef struct
    {
        /* --- UART hardware config (passed to bspUartInit) --- */
        bspUartHandle_t uartHandle;

        /* --- Interface dispatch callbacks ---
         * Set to NULL if that interface is not used.
         * wifi_app calls wifi_app_handle_host_frame() internally,
         * so register wifi_app_handle_host_frame wrapper here.
         */
        uart_app_intf_cb_t onWifiFrame; /**< Called for INTF=WiFi frames    */
        uart_app_intf_cb_t onBleFrame;  /**< Called for INTF=BLE frames     */
        uart_app_intf_cb_t onBtFrame;   /**< Called for INTF=BT Classic     */

        void* wifiUser;                 /**< Context passed to onWifiFrame  */
        void* bleUser;                  /**< Context passed to onBleFrame   */
        void* btUser;                   /**< Context passed to onBtFrame    */
    } uart_app_cfg_t;

    /* =========================================================================
     * CONTEXT - one global instance
     * ========================================================================= */
    typedef struct
    {
        uart_app_cfg_t cfg;
        osThreadId_t rxTaskId;
        osThreadId_t txTaskId;
        osMessageQueueId_t rxQ; /* async RX chunks -> RX task   */
        osMessageQueueId_t txQ;
        osMutexId_t txMutex;
        volatile uint8_t running;

        /* RX accumulation buffer */
        uint8_t rxBuf[UART_APP_RX_BUF_LEN];

        /* Heartbeat tick tracking */
        uint32_t lastHeartbeatTick;
    } uart_app_t;

    /* =========================================================================
     * PUBLIC API
     * ========================================================================= */

    /**
     * @brief  Initialise UART app. Call once at boot.
     *         Starts RX task, TX task, heartbeat.
     */
    bsp_err_sts_t uart_app_init(uart_app_t* app, const uart_app_cfg_t* cfg);

    /**
     * @brief  Deinitialise and stop all tasks.
     */
    bsp_err_sts_t uart_app_deinit(uart_app_t* app);

    /**
     * @brief  Send a pre-encoded rc_protocol frame to Host.
     *         Thread-safe - safe to call from WiFi task, BLE task, etc.
     * @param  frame  Encoded bytes (output of rc_encode_* functions)
     * @param  len    Byte count
     */
    bsp_err_sts_t uart_app_send(uart_app_t* app, const uint8_t* frame, uint16_t len);

    /**
     * @brief  Send a STATUS frame to Host (1 byte, no payload).
     *         Convenience wrapper around uart_app_send + rc_encode_simple.
     */
    bsp_err_sts_t uart_app_send_status(uart_app_t* app, rc_intf_t intf, rc_sts_sub_t sub);

    /**
     * @brief  Send a DATA frame to Host.
     *         Convenience wrapper around uart_app_send + rc_encode_data.
     */
    bsp_err_sts_t uart_app_send_data(uart_app_t* app,
                                     rc_intf_t intf,
                                     uint8_t dataSub,
                                     const uint8_t* payload,
                                     uint16_t payloadLen);

#ifdef __cplusplus
}
#endif

#endif /* UART_APP_H */