/**
 * ============================================================================
 * @file    wifi_app.c
 * @brief   RideCast Wi-Fi AP and TCP application service.
 *
 * @details
 * All Wi-Fi application configuration, transport registration, protocol
 * dispatch, status reporting, and TCP processing reside in this module.
 *
 * The vehicle cluster is MASTER. Only conn_manager may authorize START,
 * STOP, and reconnect. The Wi-Fi radio stays OFF during initialization.
 *
 * ============================================================================
 * STATE FLOW
 * ============================================================================
 *
 *  IDLE (Wi-Fi OFF) --START--> WIFI_UP --> TCP_LISTEN --> TCP_AUTH
 *      ^                                                 |
 *      |                                          Authenticated
 *      |                                                 v
 *      +---------------- STOP ------------------------ READY
 *                                                        |
 *                                              Disconnect / failure
 *                                                        v
 *                                                     SLEEPING
 *                                                        |
 *                                             Manager-authorized START
 *                                                        v
 *                                                     WIFI_UP
 *
 * ============================================================================
 * THREAD LIFECYCLE / OWNERSHIP
 * ============================================================================
 *
 *  app_main.c creates queue and thread via the application OS map.
 *  Generic thread wrapper calls:
 *      wifi_app_init(void) -> vRcWifiTask(void* arg) -> wifi_app_deinit(void)
 *
 *  wifi_app_init sets up the Wi-Fi service, callbacks and registers its
 *  conn_manager operations. It DOES NOT start the radio or create a task.
 *  The wrapper must pass the updated descriptor .arg after calling init.
 *  Initialize conn_manager before the Wi-Fi thread wrapper.
 *
 *  ridecast_main.c provides only a generic UART send bridge and registers
 *  wifi_app_on_host_frame with the UART dispatcher.
 *
 * ============================================================================
 * WARNINGS
 * ============================================================================
 *
 *  - Development TOKEN authentication is not encrypted or production ready.
 *  - TCP is a stream, not a framed application protocol.
 *  - The manager alone owns reconnect counts and retry decisions.
 *  - OS wrapper cleanup requires the worker to return normally.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Consolidated Wi-Fi configuration, manager integration and
 *             UART protocol callbacks into wifi_app.c; no-arg lifecycle.
 * 2026-10-25  Centralized retry decisions in conn_manager.
 * 2026-10-25  Improved queue, token, and non-blocking TCP handling.
 * 2026-05-23  Initial development - J.Raghav.
 * ============================================================================
 */

#include "app_config.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "wifi_app.h"
#include "app_logging.h"
#include "conn_manager.h"
#include "rc_protocol.h"
#include "ridecast_main.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

/* =========================================================================
 * MODULE CONFIGURATION ALIASES
 * ========================================================================= */

#define WAPP_LOOP_MS            configWIFI_APP_LOOP_MS
#define WAPP_RXTX_BUF_SIZE      configWIFI_RXTX_BUF_SIZE
#define WAPP_TOKEN_MAX_LEN      configWIFI_APP_TOKEN_MAX_LEN
#define WAPP_TCP_PORT           configWIFI_TCP_PORT
#define WAPP_AUTH_TIMEOUT_MS    configWIFI_APP_AUTH_TIMEOUT_MS
#define WAPP_SSID_MAX_LEN       configWIFI_APP_SSID_MAX_LEN
#define WAPP_PASS_MAX_LEN       configWIFI_APP_PASS_MAX_LEN
#define WAPP_UART_TX_BUF_LEN    configHOST_UART_RXTX_BUF_SIZE

#define WAPP_AUTH_PREFIX        "TOKEN "
#define WAPP_AUTH_PREFIX_LEN    (6U)
#define WAPP_AUTH_BUFFER_LEN    (WAPP_TOKEN_MAX_LEN + WAPP_AUTH_PREFIX_LEN + 2U)
#define WAPP_START_PAYLOAD_LEN  (WAPP_SSID_MAX_LEN + WAPP_PASS_MAX_LEN)
#define WAPP_STATUS_PAYLOAD_LEN (5U)

/* Existing token; development builds ONLY. */
#define WAPP_DEMO_AUTH_TOKEN    "rc_secret_v1"

/* =========================================================================
 * MODULE CONTEXT AND STATIC FORWARD DECLARATIONS
 * ========================================================================= */

static wifi_app_t gWifiApp;
static wifi_app_start_params_t gWifiStartParams;
static bool gWifiStartConfigured    = false;
static bool gWifiDisconnectReported = false;

static uint32_t wapp_ms_to_ticks(uint32_t milliseconds);
static void wapp_notify(wifi_app_t* app, wifi_app_sts_t status, uint32_t arg);
static ret_err_sts_t wapp_post(wifi_app_t* app,
                               uint8_t cmd,
                               const uint8_t* data,
                               uint16_t len,
                               const wifi_app_start_params_t* startParams);
static void wapp_tcp_close(wifi_app_t* app);
static ret_err_sts_t wapp_wifi_up(wifi_app_t* app);
static void wapp_wifi_down(wifi_app_t* app);
static void wapp_stop_services(wifi_app_t* app);
static void wapp_enter_sleeping(wifi_app_t* app, wifi_app_sts_t failureStatus);
static ret_err_sts_t wapp_tcp_listen(wifi_app_t* app);
static bool wapp_check_token(const wifi_app_t* app, const char* data, size_t length);
static ret_err_sts_t wapp_tcp_send(wifi_app_t* app, const uint8_t* data, uint16_t length);
static void wapp_process_command(wifi_app_t* app, const wifi_app_msg_t* msg);

static ret_err_sts_t wapp_manager_start(void* user);
static ret_err_sts_t wapp_manager_stop(void* user);
static ret_err_sts_t
wapp_send_simple_frame(rc_type_t type, uint8_t messageId, uint8_t transport, uint16_t sequence);
static ret_err_sts_t wapp_send_wifi_status(wifi_app_sts_t status, uint32_t arg);
static void wapp_ack_cluster_command(const rc_frame_t* frame);
static void on_wifi_status(wifi_app_sts_t sts, uint32_t arg, void* user);
static void on_wifi_rx(const uint8_t* data, uint16_t len, void* user);

/* =========================================================================
 * TASK ENTRY
 * ========================================================================= */

/**
 * @brief Process Wi-Fi application commands and TCP communication.
 *
 * @details
 * Called by the generic OS wrapper only after
 * wifi_app_init() succeeds.
 *
 * @param[in] arg Pointer to the initialized Wi-Fi context.
 *
 * @note
 *   This function must return normally so that the wrapper
 *   can execute wifi_app_deinit().
 */
void vRcWifiTask(void* arg)
{
    UNUSED(arg);
    wifi_app_msg_t msg;


    uint8_t rx[WAPP_RXTX_BUF_SIZE];
    char authBuffer[WAPP_AUTH_BUFFER_LEN];

    size_t authLength  = 0U;
    uint32_t authStart = 0U;
    uint32_t loopTicks;

    int32_t result;
    int flags;

    gWifiApp.taskId = osThreadGetId();
    loopTicks       = wapp_ms_to_ticks(WAPP_LOOP_MS);

    ALOGI("Wi-Fi application task started");

    while(gWifiApp.running)
    {
        /* -------------------------------------------------------------
         * 1. Process queued commands
         * ------------------------------------------------------------- */

        if(osMessageQueueGet(gWifiApp.wifiQ, &msg, NULL, 0U) == osOK)
        {
            wapp_process_command(&gWifiApp, &msg);
        }

        if(gWifiApp.running == 0U)
        {
            break;
        }

        /* -------------------------------------------------------------
         * 2. IDLE / SLEEPING
         * ------------------------------------------------------------- */

        if((gWifiApp.state == eWifiAppStateIdle) || (gWifiApp.state == eWifiAppStateSleeping))
        {
            osDelay(loopTicks);
            continue;
        }

        /* -------------------------------------------------------------
         * 3. WIFI_UP
         * ------------------------------------------------------------- */

        if(gWifiApp.state == eWifiAppStateWifiUp)
        {
            if(wapp_wifi_up(&gWifiApp) == RET_ERR_STS_OK)
            {
                gWifiApp.state = eWifiAppStateTcpListen;
            }
            else
            {
                wapp_enter_sleeping(&gWifiApp, eWifiAppStsWifiFailed);
            }

            continue;
        }

        /* -------------------------------------------------------------
         * 4. TCP_LISTEN
         * ------------------------------------------------------------- */

        if(gWifiApp.state == eWifiAppStateTcpListen)
        {
            if(gWifiApp.listenFd < 0)
            {
                if(wapp_tcp_listen(&gWifiApp) != RET_ERR_STS_OK)
                {
                    wapp_enter_sleeping(&gWifiApp, eWifiAppStsTcpFailed);

                    continue;
                }
            }

            {
                struct sockaddr_in client;
                socklen_t clientLen = (socklen_t)sizeof(client);

                result = (int32_t)lwip_accept((int)gWifiApp.listenFd,
                                              (struct sockaddr*)&client, &clientLen);

                if(result >= 0)
                {
                    flags = lwip_fcntl((int)result, F_GETFL, 0);

                    if((flags < 0) ||
                       (lwip_fcntl((int)result, F_SETFL, flags | O_NONBLOCK) < 0))
                    {
                        (void)lwip_close((int)result);

                        wapp_enter_sleeping(&gWifiApp, eWifiAppStsTcpFailed);

                        continue;
                    }

                    gWifiApp.clientFd = result;
                    gWifiApp.state    = eWifiAppStateTcpAuth;

                    authLength = 0U;
                    authStart  = osKernelGetTickCount();

                    (void)memset(authBuffer, 0, sizeof(authBuffer));

                    wapp_notify(&gWifiApp, eWifiAppStsTcpConnected, 0U);
                }
                else if((errno != EAGAIN) && (errno != EWOULDBLOCK))
                {
                    wapp_enter_sleeping(&gWifiApp, eWifiAppStsTcpFailed);

                    continue;
                }
            }

            osDelay(loopTicks);
            continue;
        }

        /* -------------------------------------------------------------
         * 5. TCP_AUTH
         * ------------------------------------------------------------- */

        if(gWifiApp.state == eWifiAppStateTcpAuth)
        {
            bool authFailed   = false;
            bool lineComplete = false;
            size_t i;

            if((uint32_t)(osKernelGetTickCount() - authStart) >=
               wapp_ms_to_ticks(gWifiApp.cfg.authTimeoutMs))
            {
                authFailed = true;
            }
            else
            {
                result = (int32_t)lwip_recv((int)gWifiApp.clientFd, rx,
                                            sizeof(rx), MSG_DONTWAIT);

                if(result > 0)
                {
                    for(i = 0U; i < (size_t)result; i++)
                    {
                        if((rx[i] == '\n') || (rx[i] == '\r'))
                        {
                            lineComplete = true;
                            break;
                        }

                        if(authLength >= sizeof(authBuffer) - 1U)
                        {
                            authFailed = true;
                            break;
                        }

                        authBuffer[authLength++] = (char)rx[i];
                    }

                    if(lineComplete && !authFailed)
                    {
                        if(wapp_check_token(&gWifiApp, authBuffer, authLength))
                        {
                            gWifiApp.state = eWifiAppStateReady;

                            wapp_notify(&gWifiApp, eWifiAppStsTcpAuthOk, 0U);

                            wapp_notify(&gWifiApp, eWifiAppStsReady, 0U);

                            ALOGI("TCP client authenticated");
                        }
                        else
                        {
                            authFailed = true;
                        }
                    }
                }
                else if(result == 0)
                {
                    authFailed = true;
                }
                else if((errno != EAGAIN) && (errno != EWOULDBLOCK))
                {
                    authFailed = true;
                }
            }

            if(authFailed)
            {
                wapp_notify(&gWifiApp, eWifiAppStsTcpAuthFail, 0U);

                wapp_enter_sleeping(&gWifiApp, eWifiAppStsTcpFailed);
            }

            osDelay(loopTicks);
            continue;
        }

        /* -------------------------------------------------------------
         * 6. READY
         * ------------------------------------------------------------- */

        if(gWifiApp.state == eWifiAppStateReady)
        {
            result = (int32_t)lwip_recv((int)gWifiApp.clientFd, rx, sizeof(rx), MSG_DONTWAIT);

            if(result > 0)
            {
                if(gWifiApp.cfg.onRx != NULL)
                {
                    gWifiApp.cfg.onRx(rx, (uint16_t)result, gWifiApp.cfg.cbUser);
                }
            }
            else if((result == 0) ||
                    ((result < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)))
            {
                ALOGW("TCP client disconnected");

                wapp_notify(&gWifiApp, eWifiAppStsTcpDisconnected, 0U);

                wapp_enter_sleeping(&gWifiApp, eWifiAppStsTcpFailed);
            }

            osDelay(loopTicks);
            continue;
        }

        /* Unknown state: stop the service. */
        wapp_enter_sleeping(&gWifiApp, eWifiAppStsWifiFailed);
    }

    ALOGI("Wi-Fi application task stopped");

    /*
     * Do not call osThreadExit().
     * Return to the generic OS wrapper for cleanup.
     */
}

/* =========================================================================
 * INITIALIZATION
 * ========================================================================= */

/**
 * @brief Configure Wi-Fi and register operations with conn_manager.
 *
 * @note Invoked by the generic thread wrapper before vRcWifiTask().
 *       This API does not create the worker or activate the radio.
 *       conn_manager_init() must have completed before this call.
 */
ret_err_sts_t wifi_app_init(void)
{
    ret_err_sts_t status;
    wifi_app_cfg_t wifiConfig;
    static const conn_manager_ops_t wifiOps = { .start = wapp_manager_start,
                                                .stop  = wapp_manager_stop,
                                                .reconnect = wapp_manager_start };

    (void)memset(&wifiConfig, 0, sizeof(wifiConfig));
    (void)strncpy(wifiConfig.authToken, WAPP_DEMO_AUTH_TOKEN,
                  sizeof(wifiConfig.authToken) - 1U);

    wifiConfig.tcpPort       = WAPP_TCP_PORT;
    wifiConfig.authTimeoutMs = WAPP_AUTH_TIMEOUT_MS;
    wifiConfig.onStatus      = on_wifi_status;
    wifiConfig.onRx          = on_wifi_rx;
    wifiConfig.cbUser        = NULL;

    /* Deprecated retry fields are intentionally left at zero.
     * Retry/backoff policy belongs to conn_manager.c. */

    (void)memset(&gWifiApp, 0, sizeof(gWifiApp));
    (void)memset(&gWifiStartParams, 0, sizeof(gWifiStartParams));
    (void)memcpy(&gWifiApp.cfg, &wifiConfig, sizeof(wifiConfig));

    gWifiApp.listenFd = -1;
    gWifiApp.clientFd = -1;
    gWifiApp.state    = eWifiAppStateIdle;
    gWifiApp.wifiQ    = APP_QUEUE_HANDLE(APP_Q_OWNER_RC_WIFI);

    gWifiStartConfigured    = false;
    gWifiDisconnectReported = false;

    if(gWifiApp.wifiQ == NULL)
    {
        ALOGE("Wi-Fi command queue unavailable");
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(osMessageQueueReset(gWifiApp.wifiQ) != osOK)
    {
        ALOGE("Wi-Fi command queue reset failed");
        return RET_ERR_STS_FAIL;
    }

    /* The wrapper calls init() before thread(cfg->arg). */
    g_appThreadCfg[APP_THREAD_OWNER_RC_WIFI].arg = &gWifiApp;

    status = conn_manager_register_transport(eRcTransportWifi, &wifiOps, &gWifiApp);
    if(status != RET_ERR_STS_OK)
    {
        ALOGE("Failed to register Wi-Fi with connection manager");
        g_appThreadCfg[APP_THREAD_OWNER_RC_WIFI].arg = NULL;
        gWifiApp.wifiQ                               = NULL;
        return status;
    }

    gWifiApp.running = 1U;
    ALOGI("Wi-Fi worker initialized; radio remains OFF");
    return RET_ERR_STS_OK;
}

/* =========================================================================
 * DEINITIALIZATION
 * ========================================================================= */

/**
 * @brief Release Wi-Fi resources after the worker returns.
 *
 * @note Invoked by the generic thread wrapper; does not delete the queue
 *       or terminate the thread. The manager must already be stopping.
 */
ret_err_sts_t wifi_app_deinit(void)
{
    gWifiApp.running = 0U;
    wapp_tcp_close(&gWifiApp);

    if((gWifiApp.state != eWifiAppStateIdle) && (gWifiApp.state != eWifiAppStateSleeping))
    {
        (void)bspWifiOff(&gWifiApp.wifi);
    }

    gWifiApp.state  = eWifiAppStateIdle;
    gWifiApp.taskId = NULL;

    if(gWifiApp.wifiQ != NULL)
    {
        (void)osMessageQueueReset(gWifiApp.wifiQ);
        gWifiApp.wifiQ = NULL;
    }

    (void)memset(&gWifiStartParams, 0, sizeof(gWifiStartParams));
    gWifiStartConfigured                         = false;
    gWifiDisconnectReported                      = false;
    g_appThreadCfg[APP_THREAD_OWNER_RC_WIFI].arg = NULL;
    ALOGI("Wi-Fi worker deinitialized");
    return RET_ERR_STS_OK;
}

/* =========================================================================
 * OTHER PUBLIC APIs
 * ========================================================================= */

/**
 * @brief Queue Wi-Fi START on authorization from conn_manager.
 */
ret_err_sts_t wifi_app_start(const wifi_app_start_params_t* params)
{
    if(params == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    return wapp_post(&gWifiApp, (uint8_t)eWifiAppCmdStart, NULL, 0U, params);
}

/**
 * @brief Queue Wi-Fi STOP on authorization from conn_manager.
 */
ret_err_sts_t wifi_app_stop(void)
{
    return wapp_post(&gWifiApp, (uint8_t)eWifiAppCmdStop, NULL, 0U, NULL);
}

/**
 * @brief Queue a payload for an authenticated TCP client.
 */
ret_err_sts_t wifi_app_send(const uint8_t* data, uint16_t len)
{
    if((data == NULL) || (len == 0U) || (len > WAPP_RXTX_BUF_SIZE))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    return wapp_post(&gWifiApp, (uint8_t)eWifiAppCmdSend, data, len, NULL);
}

/**
 * @brief Request graceful thread exit at application shutdown.
 *
 * @note Use only after the manager has stopped posting commands.
 *       wifi_app_deinit() executes after vRcWifiTask returns.
 */
void wifi_app_request_shutdown(void)
{
    gWifiApp.running = 0U;
}

/**
 * @brief Dispatch a Wi-Fi transport frame received from the cluster.
 *
 * @details
 * All Wi-Fi command parsing and manager requests live here; the UART
 * application remains a transport-agnostic frame dispatcher.
 */
void wifi_app_on_host_frame(const rc_frame_t* frame, void* user)
{
    ret_err_sts_t status;

    (void)user;

    if((frame == NULL) || (frame->transport != (uint8_t)eRcTransportWifi))
    {
        ALOGW("Invalid Wi-Fi cluster frame");
        return;
    }

    switch((rc_type_t)frame->type)
    {
        case eRcTypeCmd:
        {
            switch(frame->messageId)
            {
                case eRcMsgWifiStart:
                {
                    wifi_app_start_params_t params;

                    if((frame->payload == NULL) || (frame->payloadLen != WAPP_START_PAYLOAD_LEN))
                    {
                        ALOGE("Invalid Wi-Fi START payload");
                        break;
                    }

                    (void)memset(&params, 0, sizeof(params));
                    (void)memcpy(params.ssid, frame->payload, sizeof(params.ssid));
                    (void)memcpy(params.pass, &frame->payload[sizeof(params.ssid)],
                                 sizeof(params.pass));

                    /* Preserve credentials for manager-scheduled retries. */
                    (void)memcpy(&gWifiStartParams, &params, sizeof(gWifiStartParams));
                    gWifiStartConfigured    = true;
                    gWifiDisconnectReported = false;

                    status = conn_manager_start(eRcTransportWifi);
                    if(status == RET_ERR_STS_OK)
                    {
                        wapp_ack_cluster_command(frame);
                        ALOGI("Cluster Wi-Fi START accepted");
                    }
                    else
                    {
                        ALOGE("Cluster Wi-Fi START rejected");
                    }
                    break;
                }

                case eRcMsgWifiStop:
                {
                    if(frame->payloadLen != 0U)
                    {
                        ALOGW("Wi-Fi STOP payload must be empty");
                        break;
                    }

                    status = conn_manager_stop(eRcTransportWifi);
                    if(status == RET_ERR_STS_OK)
                    {
                        wapp_ack_cluster_command(frame);
                        ALOGI("Cluster Wi-Fi STOP accepted");
                    }
                    else
                    {
                        ALOGE("Cluster Wi-Fi STOP rejected");
                    }
                    break;
                }

                default:
                {
                    ALOGW("Unsupported Wi-Fi command: %u", (unsigned int)frame->messageId);
                    break;
                }
            }
            break;
        }

        case eRcTypeData:
        {
            if((frame->messageId != (uint8_t)eRcMsgSendData) ||
               (frame->payload == NULL) || (frame->payloadLen == 0U))
            {
                ALOGW("Invalid Wi-Fi DATA frame");
                break;
            }

            if(wifi_app_send(frame->payload, frame->payloadLen) != RET_ERR_STS_OK)
            {
                ALOGE("Failed to forward cluster data to Wi-Fi");
            }
            break;
        }

        default:
        {
            ALOGW("Unsupported Wi-Fi frame type: %u", (unsigned int)frame->type);
            break;
        }
    }
}

/* =========================================================================
 * STATIC HELPER IMPLEMENTATIONS
 * ========================================================================= */

/** @brief Queue a manager-authorized Wi-Fi start/reconnect. */
static ret_err_sts_t wapp_manager_start(void* user)
{
    if((user != &gWifiApp) || !gWifiStartConfigured)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    return wifi_app_start(&gWifiStartParams);
}

/** @brief Queue a manager-authorized Wi-Fi stop. */
static ret_err_sts_t wapp_manager_stop(void* user)
{
    if(user != &gWifiApp)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    return wifi_app_stop();
}

/** @brief Encode an empty payload protocol frame and enqueue on UART. */
static ret_err_sts_t
wapp_send_simple_frame(rc_type_t type, uint8_t messageId, uint8_t transport, uint16_t sequence)
{
    uint8_t buffer[RC_FRAME_MIN_LEN];
    uint16_t length = rc_encode_simple(buffer, (uint16_t)sizeof(buffer), (uint8_t)type,
                                       RC_FLAG_NONE, messageId, transport, sequence);

    if(length == 0U)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    return uart_app_send(buffer, length);
}

/** @brief Encode the existing five-byte Wi-Fi status payload. */
static ret_err_sts_t wapp_send_wifi_status(wifi_app_sts_t status, uint32_t arg)
{
    uint8_t buffer[RC_FRAME_MIN_LEN + WAPP_STATUS_PAYLOAD_LEN];
    uint16_t crc;

    buffer[0U] = rc_make_header(RC_PROTOCOL_VERSION, eRcTypeStatus, RC_FLAG_NONE);
    buffer[1U] = (uint8_t)eRcMsgWifiStatus;
    buffer[2U] = (uint8_t)eRcTransportWifi;
    rc_write_u16(&buffer[3U], 0U);
    rc_write_u16(&buffer[5U], WAPP_STATUS_PAYLOAD_LEN);
    buffer[7U]  = (uint8_t)status;
    buffer[8U]  = (uint8_t)((arg >> 24U) & 0xFFU);
    buffer[9U]  = (uint8_t)((arg >> 16U) & 0xFFU);
    buffer[10U] = (uint8_t)((arg >> 8U) & 0xFFU);
    buffer[11U] = (uint8_t)(arg & 0xFFU);

    crc = rc_crc16(buffer, (uint16_t)(RC_FRAME_FIXED_LEN + WAPP_STATUS_PAYLOAD_LEN));
    rc_write_u16(&buffer[RC_FRAME_FIXED_LEN + WAPP_STATUS_PAYLOAD_LEN], crc);
    return uart_app_send(buffer, (uint16_t)sizeof(buffer));
}

/** @brief ACK an accepted cluster Wi-Fi command. */
static void wapp_ack_cluster_command(const rc_frame_t* frame)
{
    if(wapp_send_simple_frame(eRcTypeAck, (uint8_t)eRcMsgAck, frame->transport,
                              frame->sequence) != RET_ERR_STS_OK)
    {
        ALOGE("Failed to enqueue cluster Wi-Fi ACK");
    }
}

/** @brief Forward authenticated phone bytes to the cluster. */
static void on_wifi_rx(const uint8_t* data, uint16_t len, void* user)
{
    uint8_t buffer[WAPP_UART_TX_BUF_LEN];
    uint16_t frameLen;

    (void)user;

    if((data == NULL) || (len == 0U))
    {
        return;
    }

    if(((uint32_t)len + RC_FRAME_MIN_LEN) > sizeof(buffer))
    {
        ALOGW("Phone data exceeds UART TX frame capacity");
        return;
    }

    frameLen = rc_encode_data(buffer, (uint16_t)sizeof(buffer),
                              (uint8_t)eRcTransportWifi, 0U, data, len);
    if(frameLen == 0U)
    {
        ALOGE("Failed to encode phone data");
        return;
    }

    if(uart_app_send(buffer, frameLen) != RET_ERR_STS_OK)
    {
        ALOGE("Failed to forward phone data to cluster");
    }
}

/** @brief Report Wi-Fi status to cluster and lifecycle events to manager. */
static void on_wifi_status(wifi_app_sts_t sts, uint32_t arg, void* user)
{
    ret_err_sts_t status;
    conn_manager_event_t managerEvent = eRcConnEvtStop;
    uint8_t eventId                   = 0U;
    bool reportToManager              = false;

    (void)user;

    status = wapp_send_wifi_status(sts, arg);
    if(status != RET_ERR_STS_OK)
    {
        ALOGE("Failed to report Wi-Fi status to cluster");
    }

    switch(sts)
    {
        case eWifiAppStsWifiStarting:
        {
            gWifiDisconnectReported = false;
            managerEvent            = eRcConnEvtConnecting;
            reportToManager         = true;
            break;
        }
        case eWifiAppStsTcpConnected:
        {
            /* Socket accepted; not authenticated yet. */
            gWifiDisconnectReported = false;
            break;
        }
        case eWifiAppStsReady:
        {
            gWifiDisconnectReported = false;
            managerEvent            = eRcConnEvtConnected;
            reportToManager         = true;
            eventId                 = (uint8_t)eRcMsgConnected;
            break;
        }
        case eWifiAppStsTcpDisconnected:
        {
            gWifiDisconnectReported = true;
            managerEvent            = eRcConnEvtDisconnected;
            reportToManager         = true;
            eventId                 = (uint8_t)eRcMsgDisconnected;
            break;
        }
        case eWifiAppStsWifiFailed:
        {
            managerEvent    = eRcConnEvtFailed;
            reportToManager = true;
            eventId         = (uint8_t)eRcMsgConnectionFailed;
            break;
        }
        case eWifiAppStsTcpFailed:
        {
            if(!gWifiDisconnectReported)
            {
                managerEvent    = eRcConnEvtFailed;
                reportToManager = true;
            }
            gWifiDisconnectReported = false;
            eventId                 = (uint8_t)eRcMsgConnectionFailed;
            break;
        }
        case eWifiAppStsWifiRetrying:
        case eWifiAppStsTcpRetrying:
        {
            /* Legacy status values; module never schedules retries. */
            eventId = (uint8_t)eRcMsgReconnecting;
            break;
        }
        case eWifiAppStsStopped:
        {
            gWifiDisconnectReported = false;
            break;
        }
        default:
        {
            break;
        }
    }

    if(reportToManager &&
       (conn_manager_report_event(eRcTransportWifi, managerEvent) != RET_ERR_STS_OK))
    {
        ALOGW("Failed to report Wi-Fi event to connection manager");
    }

    if((eventId != 0U) && (wapp_send_simple_frame(eRcTypeStatus, eventId, (uint8_t)eRcTransportWifi,
                                                  0U) != RET_ERR_STS_OK))
    {
        ALOGE("Failed to forward Wi-Fi connection event to cluster");
    }
}

static uint32_t wapp_ms_to_ticks(uint32_t milliseconds)
{
    uint32_t frequency = osKernelGetTickFreq();
    uint64_t ticks;

    if(frequency == 0U)
    {
        return 1U;
    }

    ticks = ((uint64_t)milliseconds * frequency + 999ULL) / 1000ULL;

    if(ticks == 0ULL)
    {
        ticks = 1ULL;
    }

    if(ticks > UINT32_MAX)
    {
        ticks = UINT32_MAX;
    }

    return (uint32_t)ticks;
}


/**
 * @brief Notify the application about a Wi-Fi event.
 */
static void wapp_notify(wifi_app_t* app, wifi_app_sts_t status, uint32_t arg)
{
    if((app != NULL) && (app->cfg.onStatus != NULL))
    {
        app->cfg.onStatus(status, arg, app->cfg.cbUser);
    }
}


/**
 * @brief Enqueue a Wi-Fi application command.
 *
 * @details
 * START credentials are copied into the queue message because the
 * caller's original buffer may no longer exist when processed.
 */
static ret_err_sts_t
wapp_post(wifi_app_t* app, uint8_t cmd, const uint8_t* data, uint16_t len, const wifi_app_start_params_t* startParams)
{
    wifi_app_msg_t msg;

    if((app == NULL) || (app->wifiQ == NULL) || (app->running == 0U))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(len > sizeof(msg.data))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if((len > 0U) && (data == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if((cmd == (uint8_t)eWifiAppCmdStart) && (startParams == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    (void)memset(&msg, 0, sizeof(msg));

    msg.cmd = cmd;
    msg.len = len;

    if(len > 0U)
    {
        (void)memcpy(msg.data, data, len);
    }

    if(cmd == (uint8_t)eWifiAppCmdStart)
    {
        (void)memcpy(&msg.startParams, startParams, sizeof(msg.startParams));
    }

    if(osMessageQueuePut(app->wifiQ, &msg, 0U, 0U) != osOK)
    {
        ALOGW("Wi-Fi command queue full, cmd=%u", (unsigned int)cmd);

        return RET_ERR_STS_FAIL;
    }

    return RET_ERR_STS_OK;
}


/**
 * @brief Close TCP client and listening sockets.
 */
static void wapp_tcp_close(wifi_app_t* app)
{
    if(app->clientFd >= 0)
    {
        (void)lwip_close((int)app->clientFd);
        app->clientFd = -1;
    }

    if(app->listenFd >= 0)
    {
        (void)lwip_close((int)app->listenFd);
        app->listenFd = -1;
    }
}


/**
 * @brief Initialize and switch on Wi-Fi Access Point.
 */
static ret_err_sts_t wapp_wifi_up(wifi_app_t* app)
{
    bspWifiApConfig_t apCfg;
    ret_err_sts_t status;

    (void)memset(&apCfg, 0, sizeof(apCfg));

    (void)memcpy(apCfg.ssid, app->startParams.ssid, sizeof(apCfg.ssid) - 1U);

    (void)memcpy(apCfg.password, app->startParams.pass, sizeof(apCfg.password) - 1U);

    apCfg.channel = 0U;

    wapp_notify(app, eWifiAppStsWifiStarting, 0U);

    status = bspWifiInit(&app->wifi);

    if(status != RET_ERR_STS_OK)
    {
        ALOGE("Wi-Fi BSP initialization failed");
        return status;
    }

    status = bspWifiOn(&app->wifi, eWiFiModeAP, &apCfg);

    if(status != RET_ERR_STS_OK)
    {
        ALOGE("Wi-Fi AP startup failed");

        (void)bspWifiOff(&app->wifi);

        return status;
    }

    wapp_notify(app, eWifiAppStsWifiReady, 0U);

    ALOGI("Wi-Fi AP started");

    return RET_ERR_STS_OK;
}


/**
 * @brief Stop Wi-Fi Access Point.
 */
static void wapp_wifi_down(wifi_app_t* app)
{
    (void)bspWifiOff(&app->wifi);

    wapp_notify(app, eWifiAppStsWifiDisconnected, 0U);
}


/**
 * @brief Stop TCP and Wi-Fi services.
 */
static void wapp_stop_services(wifi_app_t* app)
{
    wapp_tcp_close(app);

    if((app->state != eWifiAppStateIdle) && (app->state != eWifiAppStateSleeping))
    {
        wapp_wifi_down(app);
    }
}


/**
 * @brief Report failure and enter SLEEPING.
 *
 * @details
 * This module never decides how many times to retry.
 * conn_manager owns that decision.
 */
static void wapp_enter_sleeping(wifi_app_t* app, wifi_app_sts_t failureStatus)
{
    wapp_notify(app, failureStatus, 0U);

    wapp_stop_services(app);

    app->state = eWifiAppStateSleeping;

    wapp_notify(app, eWifiAppStsSleeping, 0U);
}


/**
 * @brief Create a non-blocking TCP listening socket.
 */
static ret_err_sts_t wapp_tcp_listen(wifi_app_t* app)
{
    struct sockaddr_in addr;

    int fd;
    int flags;
    int opt = 1;

    fd = lwip_socket(AF_INET, SOCK_STREAM, 0);

    if(fd < 0)
    {
        ALOGE("TCP socket creation failed: %d", errno);

        return RET_ERR_STS_FAIL;
    }

    (void)lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(opt));

    (void)memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(app->cfg.tcpPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(lwip_bind(fd, (const struct sockaddr*)&addr, (socklen_t)sizeof(addr)) != 0)
    {
        ALOGE("TCP bind failed: %d", errno);

        (void)lwip_close(fd);
        return RET_ERR_STS_FAIL;
    }

    if(lwip_listen(fd, 1) != 0)
    {
        ALOGE("TCP listen failed: %d", errno);

        (void)lwip_close(fd);
        return RET_ERR_STS_FAIL;
    }

    flags = lwip_fcntl(fd, F_GETFL, 0);

    if((flags < 0) || (lwip_fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0))
    {
        ALOGE("TCP non-blocking configuration failed");

        (void)lwip_close(fd);
        return RET_ERR_STS_FAIL;
    }

    app->listenFd = fd;

    wapp_notify(app, eWifiAppStsTcpListening, (uint32_t)app->cfg.tcpPort);

    return RET_ERR_STS_OK;
}


/**
 * @brief Validate the development TOKEN authentication message.
 *
 * @warning
 *   This does not provide production-grade authentication.
 */
static bool wapp_check_token(const wifi_app_t* app, const char* data, size_t length)
{
    size_t tokenLength;

    if((app == NULL) || (data == NULL) || (length <= WAPP_AUTH_PREFIX_LEN))
    {
        return false;
    }

    if(app->cfg.authToken[0] == '\0')
    {
        return false;
    }

    tokenLength = strlen(app->cfg.authToken);

    if((length != (WAPP_AUTH_PREFIX_LEN + tokenLength)) ||
       (memcmp(data, WAPP_AUTH_PREFIX, WAPP_AUTH_PREFIX_LEN) != 0))
    {
        return false;
    }

    return (memcmp(&data[WAPP_AUTH_PREFIX_LEN], app->cfg.authToken, tokenLength) == 0);
}


/**
 * @brief Send data to the TCP client.
 *
 * @details
 * Handles partial writes with bounded retries for non-blocking
 * socket busy conditions.
 */
static ret_err_sts_t wapp_tcp_send(wifi_app_t* app, const uint8_t* data, uint16_t length)
{
    size_t sent        = 0U;
    uint32_t busyCount = 0U;
    int result;

    while((sent < length) && (app->running != 0U))
    {
        result = lwip_send((int)app->clientFd, &data[sent], (size_t)length - sent, MSG_DONTWAIT);

        if(result > 0)
        {
            sent += (size_t)result;
            busyCount = 0U;

            continue;
        }

        if((result < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)) && (busyCount < 5U))
        {
            busyCount++;

            osDelay(wapp_ms_to_ticks(10U));
            continue;
        }

        return RET_ERR_STS_FAIL;
    }

    return (sent == length) ? RET_ERR_STS_OK : RET_ERR_STS_FAIL;
}


/**
 * @brief Execute a queued Wi-Fi application command.
 */
static void wapp_process_command(wifi_app_t* app, const wifi_app_msg_t* msg)
{
    switch((wifi_app_cmd_t)msg->cmd)
    {
        case eWifiAppCmdStart:
        {
            /*
             * START also executes manager-authorized reconnection.
             * No local retry policy exists.
             */
            wapp_stop_services(app);

            (void)memcpy(&app->startParams, &msg->startParams, sizeof(app->startParams));

            app->state = eWifiAppStateWifiUp;

            ALOGI("Wi-Fi START accepted");
            break;
        }

        case eWifiAppCmdStop:
        {
            wapp_stop_services(app);

            app->state = eWifiAppStateIdle;

            wapp_notify(app, eWifiAppStsStopped, 0U);

            ALOGI("Wi-Fi STOP completed");
            break;
        }

        case eWifiAppCmdSend:
        {
            if((app->state != eWifiAppStateReady) || (app->clientFd < 0) || (msg->len == 0U))
            {
                ALOGW("Wi-Fi SEND rejected: not authenticated");

                break;
            }

            if(wapp_tcp_send(app, msg->data, msg->len) != RET_ERR_STS_OK)
            {
                ALOGE("TCP send failed");

                wapp_notify(app, eWifiAppStsTcpDisconnected, 0U);

                wapp_enter_sleeping(app, eWifiAppStsTcpFailed);
            }

            break;
        }

        default:
        {
            ALOGW("Unknown Wi-Fi command: %u", (unsigned int)msg->cmd);

            break;
        }
    }
}


/* =========================================================================
 * FILE END
 * ========================================================================= */
