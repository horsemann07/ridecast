/**
 * =========================================================================
 * @file    rc_wifi.c
 * @brief   RideCast Wi-Fi transport implementation.
 *
 * Architecture:
 *
 *      RideCast Core
 *           |
 *           | rc_wifi_send()
 *           v
 *      RC WiFi Queue
 *           |
 *           v
 *      RC WiFi Task
 *           |
 *      +----+----------------------+
 *      |                           |
 *      v                           v
 *   TCP TX                      TCP RX
 *                                  |
 *                                  v
 *                           RC Core RX Queue
 *
 * Wi-Fi role:
 *   - RideCast acts as Wi-Fi Access Point
 *   - Phone connects as TCP client
 *   - Token authentication is performed before READY state
 *   - Payload is treated as opaque data
 *
 * Retry ownership:
 *   - rc_wifi.c DOES NOT implement retry/backoff policy
 *   - rc_connection.c owns retry/reconnect decisions
 *
 * =========================================================================
 */

#include "rc_wifi.h"
#include "rc_connection.h"
#include "rc_config.h"
#include "rc_protocol.h"

#include "bsp_wifi.h"
#include "app_logging.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <fcntl.h>

/* =========================================================================
 * Internal constants
 * ========================================================================= */

#define RC_WIFI_LOOP_MS              (50U)
#define RC_WIFI_AUTH_PREFIX          "TOKEN "
#define RC_WIFI_AUTH_PREFIX_LEN      (6U)
#define RC_WIFI_AUTH_RX_BUF_LEN      (128U)

/*
 * Queue message size must match the queue configuration in the
 * application map.
 */
#define RC_WIFI_TX_BUF_LEN           (RC_MAX_PAYLOAD_LEN)

/* =========================================================================
 * Internal types
 * ========================================================================= */

/**
 * @brief Internal Wi-Fi task command.
 */
typedef enum
{
    eRcWifiCmdStart = 0x00U,
    eRcWifiCmdStop,
    eRcWifiCmdReconnect,
    eRcWifiCmdSend
} rc_wifi_cmd_t;


/**
 * @brief Wi-Fi task command message.
 *
 * This structure must match the queue item size configured for
 * APP_Q_OWNER_RC_WIFI.
 */
typedef struct
{
    uint8_t  cmd;
    uint16_t len;
    uint8_t  data[RC_WIFI_TX_BUF_LEN];
} rc_wifi_msg_t;


/**
 * @brief Local Wi-Fi transport state.
 *
 * This is transport-internal state.
 * Connection manager state remains in rc_connection.c.
 */
typedef enum
{
    eRcWifiStateIdle = 0x00U,
    eRcWifiStateStarting,
    eRcWifiStateListen,
    eRcWifiStateAuth,
    eRcWifiStateReady
} rc_wifi_state_t;


/**
 * @brief RX message forwarded to RideCast core.
 *
 * APP_Q_OWNER_RC_CORE_RX must use this same queue item size.
 */
typedef struct
{
    uint8_t  buf[RC_FRAME_MAX_LEN];
    uint16_t len;

} rc_core_rx_msg_t;


/**
 * @brief RC Wi-Fi runtime context.
 */
typedef struct
{
    bspWifiHandle_t       wifi;
    rc_wifi_config_t      config;

    osMessageQueueId_t    wifiQ;
    osMessageQueueId_t    coreRxQ;
    osThreadId_t          taskId;

    int32_t               listenFd;
    int32_t               clientFd;

    rc_wifi_state_t       state;

    bool                  initialized;
    bool                  wifiInitialized;
    bool                  running;

} rc_wifi_ctx_t;


/* =========================================================================
 * Global context
 * ========================================================================= */

static rc_wifi_ctx_t g_rcWifi;


/* =========================================================================
 * Forward declarations
 * ========================================================================= */

static void rc_wifi_task(void* arg);

static ret_err_sts_t rc_wifi_post(
    rc_wifi_cmd_t cmd,
    const uint8_t* data,
    uint16_t len);

static ret_err_sts_t rc_wifi_up(void);

static void rc_wifi_down(void);

static ret_err_sts_t rc_wifi_tcp_listen(void);

static void rc_wifi_tcp_close(void);

static bool rc_wifi_tcp_auth(void);

static ret_err_sts_t rc_wifi_tcp_send(
    const uint8_t* data,
    uint16_t len);

static void rc_wifi_handle_rx(
    const uint8_t* data,
    uint16_t len);

static void rc_wifi_report(
    rc_connection_event_t event);


/* =========================================================================
 * Connection manager event reporting
 * ========================================================================= */

static void rc_wifi_report(rc_connection_event_t event)
{
    ret_err_sts_t status;

    status = rc_connection_set_state(
        eRcTransportWifi,
        event);

    if(status != RET_ERR_STS_OK)
    {
        ALOGW(
            "rc_wifi_report: connection state update failed "
            "event=%u status=%u",
            (unsigned int)event,
            (unsigned int)status);
    }
}


/* =========================================================================
 * Queue helper
 * ========================================================================= */

static ret_err_sts_t rc_wifi_post(
    rc_wifi_cmd_t cmd,
    const uint8_t* data,
    uint16_t len)
{
    rc_wifi_msg_t msg;

    if(g_rcWifi.wifiQ == NULL)
    {
        ALOGE("rc_wifi_post: WiFi queue not initialized");
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if((len > 0U) && (data == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(len > (uint16_t)sizeof(msg.data))
    {
        ALOGE(
            "rc_wifi_post: data too large len=%u max=%u",
            (unsigned int)len,
            (unsigned int)sizeof(msg.data));

        return RET_ERR_STS_INVALID_SIZE;
    }

    (void)memset(&msg, 0, sizeof(msg));

    msg.cmd = (uint8_t)cmd;
    msg.len = len;

    if((data != NULL) && (len > 0U))
    {
        (void)memcpy(msg.data, data, (size_t)len);
    }

    if(osMessageQueuePut(
           g_rcWifi.wifiQ,
           &msg,
           0U,
           0U) != osOK)
    {
        ALOGW(
            "rc_wifi_post: queue full cmd=%u",
            (unsigned int)cmd);

        return RET_ERR_STS_BUSY;
    }

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Wi-Fi bring-up
 *
 * RideCast operates as AP.
 *
 * The old wifi_app.c already used:
 *
 *     bspWifiInit()
 *     bspWifiOn(... eWiFiModeAP ...)
 *
 * We retain that behavior here.
 * ========================================================================= */

static ret_err_sts_t rc_wifi_up(void)
{
    ret_err_sts_t status;
    bspWifiApConfig_t apCfg;

    (void)memset(&apCfg, 0, sizeof(apCfg));

    /*
     * Copy configured AP parameters.
     */
    (void)memcpy(
        &apCfg,
        &g_rcWifi.config.apConfig,
        sizeof(apCfg));

    rc_wifi_report(eRcConnEvtConnecting);

    /*
     * Initialize BSP Wi-Fi only once per START cycle.
     */
    if(!g_rcWifi.wifiInitialized)
    {
        status = bspWifiInit(&g_rcWifi.wifi);

        if(status != RET_ERR_STS_OK)
        {
            ALOGE(
                "rc_wifi_up: bspWifiInit failed status=%u",
                (unsigned int)status);

            rc_wifi_report(eRcConnEvtFailed);
            return status;
        }

        g_rcWifi.wifiInitialized = true;
    }

    /*
     * Start Wi-Fi AP.
     */
    status = bspWifiOn(
        &g_rcWifi.wifi,
        eWiFiModeAP,
        &apCfg);

    if(status != RET_ERR_STS_OK)
    {
        ALOGE(
            "rc_wifi_up: bspWifiOn failed status=%u",
            (unsigned int)status);

        rc_wifi_down();
        rc_wifi_report(eRcConnEvtFailed);

        return status;
    }

    ALOGD("rc_wifi_up: AP started");

    g_rcWifi.state = eRcWifiStateListen;

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Wi-Fi down
 * ========================================================================= */

static void rc_wifi_down(void)
{
    if(g_rcWifi.wifiInitialized)
    {
        (void)bspWifiOff(&g_rcWifi.wifi);

        g_rcWifi.wifiInitialized = false;
    }
}


/* =========================================================================
 * TCP listen socket
 * ========================================================================= */

static ret_err_sts_t rc_wifi_tcp_listen(void)
{
    struct sockaddr_in addr;
    int32_t fd;
    int opt;
    int flags;

    fd = (int32_t)lwip_socket(
        AF_INET,
        SOCK_STREAM,
        0);

    if(fd < 0)
    {
        ALOGE(
            "rc_wifi_tcp_listen: socket failed errno=%d",
            errno);

        return RET_ERR_STS_FAIL;
    }

    /*
     * Allow immediate socket reuse after reconnect.
     */
    opt = 1;

    if(lwip_setsockopt(
           (int)fd,
           SOL_SOCKET,
           SO_REUSEADDR,
           &opt,
           (socklen_t)sizeof(opt)) != 0)
    {
        ALOGW(
            "rc_wifi_tcp_listen: SO_REUSEADDR failed errno=%d",
            errno);
    }

    (void)memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(g_rcWifi.config.tcpPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(lwip_bind(
           (int)fd,
           (const struct sockaddr*)&addr,
           (socklen_t)sizeof(addr)) != 0)
    {
        ALOGE(
            "rc_wifi_tcp_listen: bind failed port=%u errno=%d",
            (unsigned int)g_rcWifi.config.tcpPort,
            errno);

        (void)lwip_close((int)fd);

        return RET_ERR_STS_FAIL;
    }

    if(lwip_listen((int)fd, 1) != 0)
    {
        ALOGE(
            "rc_wifi_tcp_listen: listen failed errno=%d",
            errno);

        (void)lwip_close((int)fd);

        return RET_ERR_STS_FAIL;
    }

    /*
     * Non-blocking socket.
     */
    flags = lwip_fcntl(
        (int)fd,
        F_GETFL,
        0);

    if(flags < 0)
    {
        ALOGE(
            "rc_wifi_tcp_listen: F_GETFL failed errno=%d",
            errno);

        (void)lwip_close((int)fd);

        return RET_ERR_STS_FAIL;
    }

    if(lwip_fcntl(
           (int)fd,
           F_SETFL,
           flags | O_NONBLOCK) < 0)
    {
        ALOGE(
            "rc_wifi_tcp_listen: F_SETFL failed errno=%d",
            errno);

        (void)lwip_close((int)fd);

        return RET_ERR_STS_FAIL;
    }

    g_rcWifi.listenFd = fd;

    ALOGD(
        "rc_wifi_tcp_listen: listening port=%u",
        (unsigned int)g_rcWifi.config.tcpPort);

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * TCP socket cleanup
 * ========================================================================= */

static void rc_wifi_tcp_close(void)
{
    if(g_rcWifi.clientFd >= 0)
    {
        (void)lwip_close(
            (int)g_rcWifi.clientFd);

        g_rcWifi.clientFd = -1;
    }

    if(g_rcWifi.listenFd >= 0)
    {
        (void)lwip_close(
            (int)g_rcWifi.listenFd);

        g_rcWifi.listenFd = -1;
    }
}


/* =========================================================================
 * TCP authentication
 *
 * Expected:
 *
 *     TOKEN <secret>\n
 *
 * Empty token = authentication disabled.
 * ========================================================================= */

static bool rc_wifi_tcp_auth(void)
{
    char rx[RC_WIFI_AUTH_RX_BUF_LEN];

    uint32_t startTick;
    uint32_t elapsed;

    int32_t rxLen;

    const char* tokenValue;

    size_t tokenLength;
    size_t receivedTokenLength;

    if(g_rcWifi.config.authToken[0] == '\0')
    {
        return true;
    }

    startTick = osKernelGetTickCount();

    for(;;)
    {
        elapsed =
            osKernelGetTickCount() - startTick;

        if(elapsed >= g_rcWifi.config.authTimeoutMs)
        {
            ALOGW(
                "rc_wifi_tcp_auth: timeout");

            return false;
        }

        rxLen = (int32_t)lwip_recv(
            (int)g_rcWifi.clientFd,
            rx,
            sizeof(rx) - 1U,
            MSG_DONTWAIT);

        if(rxLen > 0)
        {
            rx[rxLen] = '\0';

            if(strncmp(
                   rx,
                   RC_WIFI_AUTH_PREFIX,
                   RC_WIFI_AUTH_PREFIX_LEN) == 0)
            {
                tokenValue =
                    &rx[RC_WIFI_AUTH_PREFIX_LEN];

                receivedTokenLength =
                    strcspn(tokenValue, "\r\n");

                tokenLength =
                    strlen(g_rcWifi.config.authToken);

                if((tokenLength == receivedTokenLength) &&
                   (strncmp(
                        tokenValue,
                        g_rcWifi.config.authToken,
                        receivedTokenLength) == 0))
                {
                    return true;
                }
            }

            ALOGW(
                "rc_wifi_tcp_auth: token rejected");

            return false;
        }

        if((rxLen < 0) &&
           (errno != EAGAIN) &&
           (errno != EWOULDBLOCK))
        {
            ALOGW(
                "rc_wifi_tcp_auth: recv error errno=%d",
                errno);

            return false;
        }

        osDelay(20U);
    }
}


/* =========================================================================
 * TCP TX
 *
 * Only the RC Wi-Fi task writes to the socket.
 * ========================================================================= */

static ret_err_sts_t rc_wifi_tcp_send(
    const uint8_t* data,
    uint16_t len)
{
    int32_t sent;

    if((data == NULL) || (len == 0U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(g_rcWifi.clientFd < 0)
    {
        return RET_ERR_STS_NOT_READY;
    }

    sent = (int32_t)lwip_send(
        (int)g_rcWifi.clientFd,
        data,
        (size_t)len,
        MSG_DONTWAIT);

    if(sent < 0)
    {
        if((errno == EAGAIN) ||
           (errno == EWOULDBLOCK))
        {
            return RET_ERR_STS_BUSY;
        }

        ALOGW(
            "rc_wifi_tcp_send: send failed errno=%d",
            errno);

        return RET_ERR_STS_TX_FAILED;
    }

    if(sent != (int32_t)len)
    {
        ALOGW(
            "rc_wifi_tcp_send: partial send sent=%ld expected=%u",
            (long)sent,
            (unsigned int)len);

        return RET_ERR_STS_TX_FAILED;
    }

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Forward RX data to RideCast core
 * ========================================================================= */

static void rc_wifi_handle_rx(
    const uint8_t* data,
    uint16_t len)
{
    rc_core_rx_msg_t msg;

    if((data == NULL) || (len == 0U))
    {
        return;
    }

    if(g_rcWifi.coreRxQ == NULL)
    {
        ALOGW(
            "rc_wifi_handle_rx: core RX queue unavailable");

        return;
    }

    if(len > (uint16_t)sizeof(msg.buf))
    {
        ALOGW(
            "rc_wifi_handle_rx: frame too large len=%u max=%u",
            (unsigned int)len,
            (unsigned int)sizeof(msg.buf));

        return;
    }

    (void)memset(&msg, 0, sizeof(msg));

    (void)memcpy(
        msg.buf,
        data,
        (size_t)len);

    msg.len = len;

    if(osMessageQueuePut(
           g_rcWifi.coreRxQ,
           &msg,
           0U,
           0U) != osOK)
    {
        ALOGW(
            "rc_wifi_handle_rx: core RX queue full");
    }
}


/* =========================================================================
 * Main RC Wi-Fi task
 * ========================================================================= */

static void rc_wifi_task(void* arg)
{
    rc_wifi_msg_t msg;

    osStatus_t queueStatus;

    uint8_t rxBuf[RC_MAX_PAYLOAD_LEN];

    int32_t rxLen;

    (void)arg;

    ALOGD("rc_wifi_task: started");

    while(g_rcWifi.running)
    {
        /*
         * ---------------------------------------------------------------
         * Process pending commands.
         * ---------------------------------------------------------------
         */
        queueStatus = osMessageQueueGet(
            g_rcWifi.wifiQ,
            &msg,
            NULL,
            0U);

        if(queueStatus == osOK)
        {
            switch((rc_wifi_cmd_t)msg.cmd)
            {
                case eRcWifiCmdStart:

                    if(!g_rcWifi.wifiInitialized)
                    {
                        g_rcWifi.state =
                            eRcWifiStateStarting;

                        if(rc_wifi_up() != RET_ERR_STS_OK)
                        {
                            g_rcWifi.state =
                                eRcWifiStateIdle;
                        }
                    }
                    else
                    {
                        ALOGW(
                            "rc_wifi_task: START while WiFi already running");
                    }

                    break;


                case eRcWifiCmdStop:

                    rc_wifi_tcp_close();
                    rc_wifi_down();

                    g_rcWifi.state =
                        eRcWifiStateIdle;

                    rc_wifi_report(
                        eRcConnEvtDisconnected);

                    break;


                case eRcWifiCmdReconnect:

                    /*
                     * Connection manager owns the retry decision.
                     * Transport only performs one reconnect action.
                     */
                    rc_wifi_report(
                        eRcConnEvtReconnecting);

                    rc_wifi_tcp_close();

                    if(g_rcWifi.wifiInitialized)
                    {
                        g_rcWifi.state =
                            eRcWifiStateListen;
                    }
                    else
                    {
                        g_rcWifi.state =
                            eRcWifiStateStarting;

                        if(rc_wifi_up() != RET_ERR_STS_OK)
                        {
                            g_rcWifi.state =
                                eRcWifiStateIdle;
                        }
                    }

                    break;


                case eRcWifiCmdSend:

                    if((g_rcWifi.state ==
                        eRcWifiStateReady) &&
                       (g_rcWifi.clientFd >= 0) &&
                       (msg.len > 0U))
                    {
                        if(rc_wifi_tcp_send(
                               msg.data,
                               msg.len) != RET_ERR_STS_OK)
                        {
                            ALOGW(
                                "rc_wifi_task: TCP TX failed");

                            (void)lwip_close(
                                (int)g_rcWifi.clientFd);

                            g_rcWifi.clientFd = -1;

                            g_rcWifi.state =
                                eRcWifiStateListen;

                            rc_wifi_report(
                                eRcConnEvtDisconnected);
                        }
                    }
                    else
                    {
                        ALOGW(
                            "rc_wifi_task: TX ignored state=%u",
                            (unsigned int)g_rcWifi.state);
                    }

                    break;


                default:

                    ALOGW(
                        "rc_wifi_task: unknown command=%u",
                        (unsigned int)msg.cmd);

                    break;
            }
        }


        /*
         * ---------------------------------------------------------------
         * Transport state machine.
         * ---------------------------------------------------------------
         */

        if(g_rcWifi.state ==
           eRcWifiStateListen)
        {
            /*
             * Open TCP server socket once.
             */
            if(g_rcWifi.listenFd < 0)
            {
                if(rc_wifi_tcp_listen() != RET_ERR_STS_OK)
                {
                    ALOGE(
                        "rc_wifi_task: TCP listen failed");

                    rc_wifi_report(
                        eRcConnEvtFailed);

                    /*
                     * Do not retry here.
                     * Connection manager decides what happens next.
                     */
                    osDelay(RC_WIFI_LOOP_MS);

                    continue;
                }
            }

            /*
             * Wait for phone connection.
             */
            {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen;
                int32_t clientFd;

                clientAddrLen =
                    (socklen_t)sizeof(clientAddr);

                clientFd = (int32_t)lwip_accept(
                    (int)g_rcWifi.listenFd,
                    (struct sockaddr*)&clientAddr,
                    &clientAddrLen);

                if(clientFd >= 0)
                {
                    int flags;

                    flags = lwip_fcntl(
                        (int)clientFd,
                        F_GETFL,
                        0);

                    if(flags >= 0)
                    {
                        if(lwip_fcntl(
                               (int)clientFd,
                               F_SETFL,
                               flags | O_NONBLOCK) < 0)
                        {
                            ALOGW(
                                "rc_wifi_task: client nonblocking setup failed");

                            (void)lwip_close(
                                (int)clientFd);

                            clientFd = -1;
                        }
                    }
                    else
                    {
                        ALOGW(
                            "rc_wifi_task: client F_GETFL failed");

                        (void)lwip_close(
                            (int)clientFd);

                        clientFd = -1;
                    }

                    if(clientFd >= 0)
                    {
                        g_rcWifi.clientFd =
                            clientFd;

                        g_rcWifi.state =
                            eRcWifiStateAuth;

                        ALOGD(
                            "rc_wifi_task: phone TCP connected");
                    }
                }
            }
        }


        /*
         * ---------------------------------------------------------------
         * Authentication.
         * ---------------------------------------------------------------
         */

        if(g_rcWifi.state ==
           eRcWifiStateAuth)
        {
            if(rc_wifi_tcp_auth())
            {
                g_rcWifi.state =
                    eRcWifiStateReady;

                rc_wifi_report(
                    eRcConnEvtConnected);

                ALOGD(
                    "rc_wifi_task: authentication successful");
            }
            else
            {
                ALOGW(
                    "rc_wifi_task: authentication failed");

                if(g_rcWifi.clientFd >= 0)
                {
                    (void)lwip_close(
                        (int)g_rcWifi.clientFd);

                    g_rcWifi.clientFd = -1;
                }

                g_rcWifi.state =
                    eRcWifiStateListen;

                /*
                 * Authentication failure is reported as transport
                 * failure. Connection manager decides whether to retry.
                 */
                rc_wifi_report(
                    eRcConnEvtFailed);
            }
        }


        /*
         * ---------------------------------------------------------------
         * READY state.
         * ---------------------------------------------------------------
         */

        if(g_rcWifi.state ==
           eRcWifiStateReady)
        {
            rxLen = (int32_t)lwip_recv(
                (int)g_rcWifi.clientFd,
                rxBuf,
                sizeof(rxBuf),
                MSG_DONTWAIT);

            if(rxLen > 0)
            {
                rc_wifi_handle_rx(
                    rxBuf,
                    (uint16_t)rxLen);
            }
            else if(rxLen == 0)
            {
                /*
                 * Graceful peer disconnect.
                 */
                ALOGW(
                    "rc_wifi_task: phone disconnected");

                (void)lwip_close(
                    (int)g_rcWifi.clientFd);

                g_rcWifi.clientFd = -1;

                g_rcWifi.state =
                    eRcWifiStateListen;

                rc_wifi_report(
                    eRcConnEvtDisconnected);
            }
            else
            {
                /*
                 * Ignore normal non-blocking state.
                 */
                if((errno != EAGAIN) &&
                   (errno != EWOULDBLOCK))
                {
                    ALOGW(
                        "rc_wifi_task: TCP RX error errno=%d",
                        errno);

                    (void)lwip_close(
                        (int)g_rcWifi.clientFd);

                    g_rcWifi.clientFd = -1;

                    g_rcWifi.state =
                        eRcWifiStateListen;

                    rc_wifi_report(
                        eRcConnEvtDisconnected);
                }
            }
        }

        osDelay(RC_WIFI_LOOP_MS);
    }


    /*
     * ---------------------------------------------------------------
     * Task cleanup.
     * ---------------------------------------------------------------
     */

    rc_wifi_tcp_close();

    rc_wifi_down();

    g_rcWifi.state =
        eRcWifiStateIdle;

    ALOGW("rc_wifi_task: stopped");

    osThreadExit();
}


/* =========================================================================
 * PUBLIC API
 * ========================================================================= */

ret_err_sts_t rc_wifi_init(void)
{
    if(g_rcWifi.initialized)
    {
        return RET_ERR_STS_ALREADY_INIT;
    }

    (void)memset(
        &g_rcWifi,
        0,
        sizeof(g_rcWifi));

    g_rcWifi.listenFd = -1;
    g_rcWifi.clientFd = -1;

    /*
     * Obtain resources from centralized application map.
     */
    g_rcWifi.wifiQ =
        g_appQueueCfg[
            APP_Q_OWNER_RC_WIFI].handle;

    if(g_rcWifi.wifiQ == NULL)
    {
        ALOGE(
            "rc_wifi_init: RC WiFi queue not initialized");

        return RET_ERR_STS_NOT_INITIALIZED;
    }

    g_rcWifi.coreRxQ =
        g_appQueueCfg[
            APP_Q_OWNER_RC_CORE_RX].handle;

    if(g_rcWifi.coreRxQ == NULL)
    {
        ALOGE(
            "rc_wifi_init: core RX queue not initialized");

        return RET_ERR_STS_NOT_INITIALIZED;
    }

    g_rcWifi.taskId =
        g_appThreadCfg[
            APP_THREAD_OWNER_RC_WIFI].handle;

    if(g_rcWifi.taskId == NULL)
    {
        ALOGE(
            "rc_wifi_init: RC WiFi task not initialized");

        return RET_ERR_STS_NOT_INITIALIZED;
    }

    g_rcWifi.state =
        eRcWifiStateIdle;

    g_rcWifi.initialized =
        true;

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Deinitialization
 * ========================================================================= */

ret_err_sts_t rc_wifi_deinit(void)
{
    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    g_rcWifi.running = false;

    rc_wifi_tcp_close();

    rc_wifi_down();

    g_rcWifi.state =
        eRcWifiStateIdle;

    g_rcWifi.wifiQ =
        NULL;

    g_rcWifi.coreRxQ =
        NULL;

    g_rcWifi.taskId =
        NULL;

    g_rcWifi.initialized =
        false;

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Configuration
 * ========================================================================= */

ret_err_sts_t rc_wifi_set_config(
    const rc_wifi_config_t* config)
{
    if(config == NULL)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(g_rcWifi.wifiInitialized ||
       (g_rcWifi.state != eRcWifiStateIdle))
    {
        return RET_ERR_STS_BUSY;
    }

    if(config->tcpPort == 0U)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    (void)memcpy(
        &g_rcWifi.config,
        config,
        sizeof(g_rcWifi.config));

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * Start
 * ========================================================================= */

ret_err_sts_t rc_wifi_start(void)
{
    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    return rc_wifi_post(
        eRcWifiCmdStart,
        NULL,
        0U);
}


/* =========================================================================
 * Stop
 * ========================================================================= */

ret_err_sts_t rc_wifi_stop(void)
{
    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    return rc_wifi_post(
        eRcWifiCmdStop,
        NULL,
        0U);
}


/* =========================================================================
 * Reconnect
 *
 * No retry calculation here.
 * rc_connection.c decides when this API should be called.
 * ========================================================================= */

ret_err_sts_t rc_wifi_reconnect(void)
{
    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    return rc_wifi_post(
        eRcWifiCmdReconnect,
        NULL,
        0U);
}


/* =========================================================================
 * Send data to phone
 * ========================================================================= */

ret_err_sts_t rc_wifi_send(
    const uint8_t* data,
    uint16_t len)
{
    if((data == NULL) || (len == 0U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(!g_rcWifi.initialized)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }

    if(len > RC_MAX_PAYLOAD_LEN)
    {
        return RET_ERR_STS_INVALID_SIZE;
    }

    return rc_wifi_post(
        eRcWifiCmdSend,
        data,
        len);
}


/* =========================================================================
 * End of file
 * ========================================================================= */