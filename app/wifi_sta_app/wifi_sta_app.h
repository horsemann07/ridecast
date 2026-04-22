/**
 * @file wifi_sta_app.h
 * @brief WiFi Station Application – public API.
 *
 * Implements a state-machine task that performs:
 *
 *  1. SCAN        – Scan nearby WiFi networks; print table over UART.
 *  2. CONNECT     – Connect to configured SSID.
 *  3. SHOW_STATUS – Print IP / gateway / RSSI / channel over UART.
 *  4. TCP_CONNECT – Open a TCP connection to a remote server.
 *  5. RX_TO_UART  – Receive data from the TCP socket and forward to UART.
 *  6. RECONNECT   – Exponential back-off; decide to re-WiFi or re-TCP.
 *
 * State diagram
 * ─────────────
 *
 *   SCAN ──► CONNECT ──► SHOW_STATUS ──► TCP_CONNECT ──► RX_TO_UART
 *     ▲          │                            │               │
 *     │    (retry on fail)             (retry on fail)  (on disconnect)
 *     │          ▼                            ▼               ▼
 *     └────── RECONNECT ◄────────────────────────────── RECONNECT
 *
 * Usage
 * ─────
 *   wifiStaAppStart();   // call once from app_main after osKernelInitialize()
 */

#ifndef WIFI_STA_APP_H
#define WIFI_STA_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_err_sts.h"

/* =========================================================================
 * State enumeration (queryable at runtime)
 * ========================================================================= */

typedef enum
{
    STA_APP_STATE_SCAN = 0,      /**< Scanning nearby WiFi networks. */
    STA_APP_STATE_CONNECT,       /**< Connecting to configured AP. */
    STA_APP_STATE_SHOW_STATUS,   /**< Printing connection info on UART. */
    STA_APP_STATE_TCP_CONNECT,   /**< Connecting TCP socket to server. */
    STA_APP_STATE_RX_TO_UART,    /**< Forwarding WiFi (TCP) data to UART. */
    STA_APP_STATE_RECONNECT,     /**< Back-off delay; deciding next step. */

    STA_APP_STATE_COUNT
} staAppState_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Start the WiFi station application.
 *
 * Initialises BSP WiFi and UART, then spawns the state-machine task.
 * Non-blocking: all work happens inside the spawned task.
 *
 * Must be called once from app_main after osKernelInitialize().
 *
 * @return BSP_ERR_STS_OK on success, BSP error code on failure.
 */
bsp_err_sts_t wifiStaAppStart(void);

/**
 * @brief Return the current state of the WiFi station app (lock-free).
 */
staAppState_t wifiStaAppGetState(void);

/**
 * @brief Convert a staAppState_t to a human-readable string.
 */
const char* wifiStaAppStateToStr(staAppState_t state);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_STA_APP_H */
