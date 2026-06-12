/**
 * @file    wifi_app.h
 * @brief   WiFi Application Task
 *
 * RESPONSIBILITIES:
 *   - Start WiFi AP with SSID/PASS from Host command
 *   - Accept phone TCP connection and authenticate
 *   - Inform Host on every state change
 *   - Auto-reconnect on link loss (WiFi + TCP separately)
 *   - Sleep (idle) after max retries, wake on next Host start command
 *
 * APPLICATION USAGE:
 *   1. Call wifi_app_init() at boot
 *   2. When Host sends START cmd with SSID+PASS -> call wifi_app_start()
 *   3. When Host sends STOP cmd                 -> call wifi_app_stop()
 *   4. To send data to phone                    -> call wifi_app_send()
 *   5. Register wifi_app_cfg_t callbacks to receive data + status
 */

#ifndef WIFI_APP_H
#define WIFI_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "bsp_wifi.h"
#include "bsp_err_sts.h"
#include "rc_protocol.h"

/* =========================================================================
 * CONFIG LIMITS
 * ========================================================================= */
#define WIFI_APP_SSID_MAX_LEN    (32U)
#define WIFI_APP_PASS_MAX_LEN    (64U)
#define WIFI_APP_TOKEN_MAX_LEN   (96U)
#define WIFI_APP_MAX_RETRY       (5U)
#define WIFI_APP_RETRY_BASE_MS   (1000U)
#define WIFI_APP_RETRY_MAX_MS    (15000U)
#define WIFI_APP_AUTH_TIMEOUT_MS (5000U)

    /* =========================================================================
     * STATUS CODES - sent to Host on every state change
     * ========================================================================= */
    typedef enum
    {
        eWifiAppStsWifiStarting  = 0x01U, /**< WiFi AP starting */
        eWifiAppStsWifiReady     = 0x02U, /**< WiFi AP up, advertising SSID */
        eWifiAppStsWifiConnected = 0x03U, /**< Phone joined WiFi AP */
        eWifiAppStsWifiDisconnected = 0x04U, /**< WiFi link dropped */
        eWifiAppStsWifiRetrying = 0x05U, /**< WiFi retry attempt, arg=count */
        eWifiAppStsWifiFailed = 0x06U, /**< WiFi max retries reached -> sleep */

        eWifiAppStsTcpListening = 0x11U, /**< TCP server listening on port */
        eWifiAppStsTcpConnected = 0x12U, /**< Phone TCP app connected */
        eWifiAppStsTcpAuthOk = 0x13U, /**< Phone authenticated - secure channel */
        eWifiAppStsTcpAuthFail = 0x14U, /**< Auth failed - rejecting phone */
        eWifiAppStsTcpDisconnected = 0x15U, /**< TCP connection lost */
        eWifiAppStsTcpRetrying = 0x16U, /**< TCP reconnect attempt, arg=count */
        eWifiAppStsTcpFailed = 0x17U, /**< TCP max retries -> back to WiFi wait */

        eWifiAppStsReady = 0x20U, /**< WiFi+TCP+Auth OK -> data flow active  */
        eWifiAppStsSleeping = 0x21U, /**< Max retries hit -> waiting for Host */
        eWifiAppStsStopped = 0x22U /**< Host sent STOP                        */
    } wifi_app_sts_t;

    /* =========================================================================
     * CALLBACKS
     * ========================================================================= */

    /**
     * @brief  Status callback - fired on every state change.
     *         Application uses this to forward status to Host.
     * @param  sts     Status code (see wifi_app_sts_t)
     * @param  arg     Extra info (retry count, port number, 0 if unused)
     * @param  user    Application context pointer
     */
    typedef void (*wifi_app_sts_cb_t)(wifi_app_sts_t sts, uint32_t arg, void* user);

    /**
     * @brief  Data received from phone over TCP.
     *         Called only when connection is authenticated.
     * @param  data    Received bytes
     * @param  len     Byte count
     * @param  user    Application context pointer
     */
    typedef void (*wifi_app_rx_cb_t)(const uint8_t* data, uint16_t len, void* user);

    /* =========================================================================
     * START PARAMS - sent by Host in START command
     * ========================================================================= */
    typedef struct
    {
        char ssid[WIFI_APP_SSID_MAX_LEN]; /**< AP SSID to advertise */
        char pass[WIFI_APP_PASS_MAX_LEN]; /**< AP password */
    } wifi_app_start_params_t;

    /* =========================================================================
     * CONFIG - filled once at boot by application
     * ========================================================================= */
    typedef struct
    {
        char authToken[WIFI_APP_TOKEN_MAX_LEN]; /**< Phone must send "TOKEN <this>\n" */
        uint16_t tcpPort;           /**< TCP server port (default 5555)    */
        uint32_t authTimeoutMs;     /**< Max wait for phone token (ms)     */
        uint8_t maxRetry;           /**< Max reconnect attempts per link   */
        uint16_t retryBaseMs;       /**< Initial backoff (ms)              */
        uint16_t retryMaxMs;        /**< Max backoff (ms)                  */

        wifi_app_sts_cb_t onStatus; /**< REQUIRED: state change callback   */
        wifi_app_rx_cb_t onRx;      /**< REQUIRED: data received callback  */
        void* cbUser;               /**< Passed back in every callback     */
    } wifi_app_cfg_t;

    /* =========================================================================
     * INTERNAL COMMANDS
     * ========================================================================= */
    typedef enum
    {
        eWifiAppCmdStart = 1U, /**< Host: start WiFi with SSID+PASS   */
        eWifiAppCmdStop  = 2U, /**< Host: stop everything             */
        eWifiAppCmdSend  = 3U  /**< Application: send data to phone   */
    } wifi_app_cmd_t;

    /* =========================================================================
     * INTERNAL STATE
     * ========================================================================= */
    typedef enum
    {
        eWifiAppStateIdle      = 0U, /**< Waiting for Host start cmd        */
        eWifiAppStateWifiUp    = 1U, /**< WiFi AP starting/running          */
        eWifiAppStateTcpListen = 2U, /**< TCP server listening              */
        eWifiAppStateTcpAuth   = 3U, /**< Phone connected, doing auth       */
        eWifiAppStateReady     = 4U, /**< Fully connected, data flowing     */
        eWifiAppStateSleeping  = 5U  /**< Max retries hit, waiting Host cmd */
    } wifi_app_state_t;

    /* =========================================================================
     * TASK CONTEXT - one global instance in application
     * ========================================================================= */
    typedef struct
    {
        wifi_app_cfg_t cfg;
        wifi_app_start_params_t startParams;
        wifi_app_state_t state;

        osThreadId_t taskId;
        osMessageQueueId_t wifiQ;

        volatile uint8_t running;

        int32_t listenFd;
        int32_t clientFd;

        uint8_t wifiRetry;
        uint8_t tcpRetry;

        bspWifiHandle_t wifi;
    } wifi_app_t;

    /* =========================================================================
     * PUBLIC API
     * ========================================================================= */

    /** @brief Initialise task. Call once at boot. */
    bsp_err_sts_t wifi_app_init(wifi_app_t* app, const wifi_app_cfg_t* cfg);

    /** @brief Deinitialise and free resources. */
    bsp_err_sts_t wifi_app_deinit(wifi_app_t* app);

    /**
     * @brief  Host sent START command with SSID+PASS.
     *         If sleeping after max retries, this wakes up and starts fresh.
     */
    bsp_err_sts_t wifi_app_start(wifi_app_t* app, const wifi_app_start_params_t* params);

    /** @brief Host sent STOP command. Tears down WiFi+TCP. */
    bsp_err_sts_t wifi_app_stop(wifi_app_t* app);

    /**
     * @brief  Send data to phone over TCP.
     *         Silently dropped if not in READY state.
     * @param  data  Bytes to send (max WIFI_APP_TX_BUF_MAX)
     * @param  len   Byte count
     */
    bsp_err_sts_t wifi_app_send(wifi_app_t* app, const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_APP_H */