/**
 * @file    wifi_app.c
 * @brief   WiFi Application Task implementation.
 *
 * STATE MACHINE:
 *
 *   [IDLE] ──START(ssid,pass)──> [WIFI_UP]
 *              ↑                     │ wifi ok
 *              │                     ↓
 *           [SLEEPING]          [TCP_LISTEN]
 *              ↑                     │ phone connects
 *           max retry                ↓
 *              │                 [TCP_AUTH]
 *           [WIFI_UP] <retry         │ auth ok
 *                  ↑                 ↓
 *                  │            [READY] ←──── data flowing
 *                  │                │
 *                  │           tcp lost → retry tcp
 *                  │           max tcp retry → retry wifi
 *                  │
 *               STOP ──────────> [IDLE]
 *
 * MISRA C:2012 notes:
 *   - All casts explicit
 *   - No heap/dynamic memory
 *   - All return values checked
 *   - No recursion
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
#include "app_config.h"

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include "wifi_app.h"
#include "backoff_algorithm.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <fcntl.h>

#include "app_logging.h"

/* =========================================================================
 * Internal constants
 * ========================================================================= */
#define WAPP_Q_LEN          (8U)
#define WAPP_LOOP_MS        (50U)
#define WAPP_AUTH_PFXLEN    (6U) /* length of "TOKEN " */

#define WIFI_APP_TX_BUF_MAX (appCfg_WIFI_RXTX_BUF_SIZE)
#define WIFI_APP_RX_BUF_MAX (appCfg_WIFI_RXTX_BUF_SIZE)
#define WIFI_APP_TCP_PORT   (appCfg_WIFI_TCP_PORT)


/* Internal message on the queue */
typedef struct
{
    uint8_t cmd;
    uint16_t len;
    uint8_t data[WIFI_APP_TX_BUF_MAX];
    wifi_app_start_params_t startParams; /* only used for eWifiAppCmdStart */
} wapp_msg_t;

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static void wapp_task(void* arg);

/* =========================================================================
 * Helper: fire status callback to application (which forwards to Host)
 * ========================================================================= */
static void wapp_notify(wifi_app_t* app, wifi_app_sts_t sts, uint32_t arg)
{
    if(app->cfg.onStatus != NULL)
    {
        app->cfg.onStatus(sts, arg, app->cfg.cbUser);
    }
}

/* =========================================================================
 * Helper: post a command to the internal queue (thread-safe)
 * ========================================================================= */
static bsp_err_sts_t
wapp_post(wifi_app_t* app, uint8_t cmd, const uint8_t* data, uint16_t len, const wifi_app_start_params_t* sp)
{
    wapp_msg_t m;

    if((app == NULL) || (app->wifiQ == NULL))
    {
        ALOGE("wapp_post: invalid app/wifiQ");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if((len > 0U) && (len > (uint16_t)sizeof(m.data)))
    {
        ALOGE("wapp_post: len too large cmd=%" PRIu8 " len=%" PRIu16
              " max=%" PRIu16,
              cmd, len, (uint16_t)sizeof(m.data));
        return BSP_ERR_STS_INVALID_PARAM;
    }

    (void)memset(&m, 0, sizeof(m));
    m.cmd = cmd;
    m.len = len;

    if((data != NULL) && (len > 0U))
    {
        (void)memcpy(m.data, data, (size_t)len);
    }

    if(osMessageQueuePut(app->wifiQ, &m, 0U, 0U) != osOK)
    {
        ALOGW("wapp_post: queue full/drop cmd=%" PRIu8 " len=%" PRIu16, cmd, len);
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* =========================================================================
 * WiFi: bring up AP with SSID + password from Host command
 * On success: notifies eWifiAppStsWifiReady
 * On fail:    returns FAIL so caller can retry
 * ========================================================================= */
static bsp_err_sts_t wapp_wifi_up(wifi_app_t* app)
{
    bspWifiApConfig_t apCfg;

    (void)memset(&apCfg, 0, sizeof(apCfg));

    /* Copy SSID and password received from Host START command */
    (void)strncpy((char*)apCfg.ssid, app->startParams.ssid, sizeof(apCfg.ssid) - 1U);

    (void)strncpy((char*)apCfg.password, app->startParams.pass, sizeof(apCfg.password) - 1U);

    apCfg.channel = 0U; /* default channel */

    wapp_notify(app, eWifiAppStsWifiStarting, 0U);

    (void)bspWifiInit(&app->wifi);

    if(bspWifiOn(&app->wifi, eWiFiModeAP, &apCfg) == BSP_ERR_STS_OK)
    {
        ALOGD("WiFi AP up");
        app->wifiRetry = 0U;
        wapp_notify(app, eWifiAppStsWifiReady, 0U);
        return BSP_ERR_STS_OK;
    }

    return BSP_ERR_STS_FAIL;
}

/* =========================================================================
 * WiFi: tear down AP cleanly
 * ========================================================================= */
static void wapp_wifi_down(wifi_app_t* app)
{
    (void)bspWifiOff(&app->wifi);
    wapp_notify(app, eWifiAppStsWifiDisconnected, 0U);
}

/* =========================================================================
 * TCP: open non-blocking listen socket
 * On success: fires eWifiAppStsTcpListening
 * ========================================================================= */
static bsp_err_sts_t wapp_tcp_listen(wifi_app_t* app)
{
    struct sockaddr_in addr;
    int32_t fd;
    int opt;
    int flags;

    fd = (int32_t)lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        ALOGE("tcp_listen: socket failed errno=%d", errno);
        return BSP_ERR_STS_FAIL;
    }

    opt = 1;
    (void)lwip_setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(opt));

    (void)memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(app->cfg.tcpPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(lwip_bind((int)fd, (const struct sockaddr*)&addr, (socklen_t)sizeof(addr)) != 0)
    {
        ALOGE("tcp_listen: bind failed port=%" PRIu16 " errno=%d", app->cfg.tcpPort, errno);
        (void)lwip_close((int)fd);
        return BSP_ERR_STS_FAIL;
    }

    if(lwip_listen((int)fd, 1) != 0)
    {
        ALOGE("tcp_listen: listen failed errno=%d", errno);
        (void)lwip_close((int)fd);
        return BSP_ERR_STS_FAIL;
    }

    /* Non-blocking accept so task loop never stalls */
    flags = lwip_fcntl((int)fd, F_GETFL, 0);
    (void)lwip_fcntl((int)fd, F_SETFL, flags | O_NONBLOCK);

    app->listenFd = fd;
    ALOGD("tcp_listen: ready port=%" PRIu16, app->cfg.tcpPort);
    wapp_notify(app, eWifiAppStsTcpListening, (uint32_t)app->cfg.tcpPort);
    return BSP_ERR_STS_OK;
}

/* =========================================================================
 * TCP: close client and listen sockets
 * ========================================================================= */
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

/* =========================================================================
 * TCP Auth: wait for phone to send "TOKEN <secret>\n"
 * If authToken is empty -> auth disabled, always pass
 * Returns true  = authenticated (secure channel established)
 * Returns false = wrong token or timeout
 * ========================================================================= */
static bool wapp_tcp_auth(wifi_app_t* app)
{
    char rx[128U];
    int32_t r;
    uint32_t start;
    uint32_t elapsed;
    const char* tokenVal;
    size_t tokenLen;
    size_t rxTokenLen;

    /* Auth disabled */
    if(app->cfg.authToken[0] == '\0')
    {
        return true;
    }

    start = osKernelGetTickCount();

    for(;;)
    {
        elapsed = osKernelGetTickCount() - start;
        if(elapsed >= app->cfg.authTimeoutMs)
        {
            ALOGW("tcp_auth: timeout ms=%" PRIu32, app->cfg.authTimeoutMs);
            break; /* Timeout - reject */
        }

        r = (int32_t)lwip_recv((int)app->clientFd, rx, sizeof(rx) - 1U, MSG_DONTWAIT);

        if(r > 0)
        {
            rx[r] = '\0';

            /* Phone must send: "TOKEN <secret>\n" */
            if(strncmp(rx, "TOKEN ", WAPP_AUTH_PFXLEN) == 0)
            {
                tokenVal   = &rx[WAPP_AUTH_PFXLEN];
                rxTokenLen = strcspn(tokenVal, "\r\n");
                tokenLen   = strlen(app->cfg.authToken);

                if((tokenLen == rxTokenLen) &&
                   (strncmp(tokenVal, app->cfg.authToken, rxTokenLen) == 0))
                {
                    return true; /* Authenticated */
                }
            }
            ALOGW("tcp_auth: token rejected");
            return false; /* Wrong token - reject immediately */
        }

        osDelay(20U);
    }

    return false; /* Timeout */
}

/* =========================================================================
 * Backoff helper: compute next retry delay using exponential backoff
 * Returns delay in ms, or 0 if max retries exhausted
 * ========================================================================= */
static uint16_t wapp_backoff_ms(wifi_app_t* app, uint8_t* retryCount)
{
    BackoffAlgorithmContext_t bo;
    uint16_t nextMs = 0U;
    uint32_t rnd    = (uint32_t)osKernelGetTickCount();

    (void)BackoffAlgorithm_InitializeParams(&bo, app->cfg.retryBaseMs, app->cfg.retryMaxMs,
                                            (uint32_t)app->cfg.maxRetry);

    bo.attemptsDone = (uint32_t)(*retryCount);

    if(BackoffAlgorithm_GetNextBackoff(&bo, rnd, &nextMs) == BackoffAlgorithmSuccess)
    {
        *retryCount = (uint8_t)bo.attemptsDone;
        return nextMs;
    }

    return 0U; /* 0 = max retries exhausted */
}

/* =========================================================================
 * Main task state machine
 *
 * IDLE       -> wait for START cmd from Host
 * WIFI_UP    -> bring up AP, on fail retry/sleep
 * TCP_LISTEN -> open TCP server, wait for phone connection
 * TCP_AUTH   -> phone connected, verify token
 * READY      -> data flowing, monitor for disconnect
 * SLEEPING   -> max retries hit, notify Host, wait for START cmd
 * ========================================================================= */
static void wapp_task(void* arg)
{
    wifi_app_t* app = (wifi_app_t*)arg;
    wapp_msg_t msg;
    osStatus_t qSts;
    uint8_t rx[WIFI_APP_RX_BUF_MAX];
    int32_t r;
    uint16_t backMs;
    ALOGD("wapp_task: started");
    while(app->running != 0U)
    {
        /* ------------------------------------------------------------------
         * IDLE / SLEEPING: block on queue waiting for Host START command
         * ------------------------------------------------------------------ */
        if((app->state == eWifiAppStateIdle) || (app->state == eWifiAppStateSleeping))
        {
            /* Block indefinitely until Host sends a command */
            qSts = osMessageQueueGet(app->wifiQ, &msg, NULL, osWaitForever);

            if(qSts == osOK)
            {
                if((wifi_app_cmd_t)msg.cmd == eWifiAppCmdStart)
                {
                    /* Store SSID+PASS from Host and move to WiFi bring-up */
                    (void)memcpy(&app->startParams, &msg.startParams,
                                 sizeof(app->startParams));

                    app->wifiRetry = 0U;
                    app->tcpRetry  = 0U;
                    app->state     = eWifiAppStateWifiUp;
                    ALOGD("wapp_task: cmd START accepted");
                }
                else if((wifi_app_cmd_t)msg.cmd == eWifiAppCmdStop)
                {
                    /* Host said STOP -> tear everything down */
                    wapp_tcp_close(app);
                    wapp_wifi_down(app);
                    app->state = eWifiAppStateIdle;
                    wapp_notify(app, eWifiAppStsStopped, 0U);
                    ALOGW("wapp_task: cmd STOP processed");
                }
                else if(((wifi_app_cmd_t)msg.cmd == eWifiAppCmdSend) &&
                        (app->state == eWifiAppStateReady) &&
                        (app->clientFd >= 0) && (msg.len > 0U))
                {
                    /* Send data to phone - only when fully ready */
                    (void)lwip_send((int)app->clientFd, msg.data, (size_t)msg.len, 0);
                }
                else
                {
                    /* Ignore SEND commands while not connected */
                    ALOGW("wapp_task: cmd ignored state=%d cmd=%" PRIu8,
                          (int)app->state, msg.cmd);
                }
            }
            continue;
        }

        /* ------------------------------------------------------------------
         * STATE: WIFI_UP - bring up AP with SSID+PASS from Host
         * ------------------------------------------------------------------ */
        if(app->state == eWifiAppStateWifiUp)
        {
            if(wapp_wifi_up(app) == BSP_ERR_STS_OK)
            {
                /* WiFi AP up -> move to TCP listen */
                app->tcpRetry = 0U;
                app->state    = eWifiAppStateTcpListen;
            }
            else
            {
                /* WiFi failed - compute backoff */
                backMs = wapp_backoff_ms(app, &app->wifiRetry);

                if(backMs > 0U)
                {
                    /* Retry after backoff delay */
                    wapp_notify(app, eWifiAppStsWifiRetrying, (uint32_t)app->wifiRetry);
                    osDelay((uint32_t)backMs);
                    ALOGW("wapp_task: wifi retry=%" PRIu8 " backoffMs=%" PRIu16,
                          app->wifiRetry, backMs);
                    /* Stay in WIFI_UP state - will retry next loop */
                }
                else
                {
                    /* Max retries hit - inform Host and go to sleep */
                    wapp_notify(app, eWifiAppStsWifiFailed, 0U);
                    wapp_notify(app, eWifiAppStsSleeping, 0U);
                    app->state = eWifiAppStateSleeping;
                    ALOGW("wapp_task: wifi retry=%" PRIu8 " backoffMs=%" PRIu16,
                          app->wifiRetry, backMs);
                }
            }
            continue;
        }

        /* ------------------------------------------------------------------
         * STATE: TCP_LISTEN - open socket and wait for phone connection
         * ------------------------------------------------------------------ */
        if(app->state == eWifiAppStateTcpListen)
        {
            /* Open listen socket if not open yet */
            if(app->listenFd < 0)
            {
                if(wapp_tcp_listen(app) != BSP_ERR_STS_OK)
                {
                    /* Socket open failed - retry WiFi from scratch */
                    wapp_wifi_down(app);
                    app->state = eWifiAppStateWifiUp;
                    ALOGW("wapp_task: tcp listen failed, restarting wifi");
                    continue;
                }
            }

            /* Poll for incoming connection (non-blocking accept) */
            {
                struct sockaddr_in cli;
                socklen_t cliLen = (socklen_t)sizeof(cli);
                int32_t cfd;

                cfd = (int32_t)lwip_accept((int)app->listenFd,
                                           (struct sockaddr*)&cli, &cliLen);

                if(cfd >= 0)
                {
                    /* Phone connected -> move to auth */
                    app->clientFd = cfd;
                    wapp_notify(app, eWifiAppStsTcpConnected, 0U);
                    app->state = eWifiAppStateTcpAuth;
                    ALOGD("wapp_task: tcp client connected");
                }
                /* else: no connection yet - loop will poll again */
            }
            continue;
        }

        /* ------------------------------------------------------------------
         * STATE: TCP_AUTH - validate phone token (secure channel setup)
         * ------------------------------------------------------------------ */
        if(app->state == eWifiAppStateTcpAuth)
        {
            if(wapp_tcp_auth(app))
            {
                /* Auth passed -> secure channel established */
                app->tcpRetry = 0U;
                wapp_notify(app, eWifiAppStsTcpAuthOk, 0U);
                wapp_notify(app, eWifiAppStsReady, 0U); /* Tell Host: READY */
                app->state = eWifiAppStateReady;
            }
            else
            {
                ALOGW("wapp_task: tcp auth failed");
                /* Auth failed -> close this connection, wait for new phone */
                wapp_notify(app, eWifiAppStsTcpAuthFail, 0U);
                (void)lwip_close((int)app->clientFd);
                app->clientFd = -1;

                /* Count auth failure as TCP retry */
                backMs = wapp_backoff_ms(app, &app->tcpRetry);

                if(backMs > 0U)
                {
                    wapp_notify(app, eWifiAppStsTcpRetrying, (uint32_t)app->tcpRetry);
                    osDelay((uint32_t)backMs);
                    app->state = eWifiAppStateTcpListen; /* Wait for new connection */
                    ALOGW("wapp_task: tcp auth retry=%" PRIu8
                          " backoffMs=%" PRIu16,
                          app->tcpRetry, backMs);
                }
                else
                {
                    /* Too many auth failures -> restart WiFi */
                    wapp_notify(app, eWifiAppStsTcpFailed, 0U);
                    wapp_tcp_close(app);
                    wapp_wifi_down(app);
                    app->wifiRetry = 0U;
                    app->state     = eWifiAppStateWifiUp;
                    ALOGE("wapp_task: tcp auth failed: retries exhausted, "
                          "restarting wifi");
                }
            }
            continue;
        }

        /* ------------------------------------------------------------------
         * STATE: READY - data flowing, monitor for disconnect
         * ------------------------------------------------------------------ */
        if(app->state == eWifiAppStateReady)
        {
            /* Poll for incoming data from phone */
            r = (int32_t)lwip_recv((int)app->clientFd, rx, sizeof(rx), MSG_DONTWAIT);

            if(r > 0)
            {
                /* Data from phone -> forward to application callback */
                if(app->cfg.onRx != NULL)
                {
                    app->cfg.onRx(rx, (uint16_t)r, app->cfg.cbUser);
                }
            }
            else if(r == 0)
            {
                /* Graceful TCP disconnect by phone */
                ALOGW("wapp_task: tcp disconnected by peer");
                wapp_notify(app, eWifiAppStsTcpDisconnected, 0U);
                (void)lwip_close((int)app->clientFd);
                app->clientFd = -1;

                /* Try to reconnect TCP (WiFi still up) */
                backMs = wapp_backoff_ms(app, &app->tcpRetry);

                if(backMs > 0U)
                {
                    wapp_notify(app, eWifiAppStsTcpRetrying, (uint32_t)app->tcpRetry);
                    ALOGW("wapp_task: tcp reconnect retry=%" PRIu8
                          " backoffMs=%" PRIu16,
                          app->tcpRetry, backMs);

                    osDelay((uint32_t)backMs);
                    app->state = eWifiAppStateTcpListen;
                }
                else
                {
                    /* TCP max retries -> restart WiFi */
                    wapp_notify(app, eWifiAppStsTcpFailed, 0U);
                    wapp_tcp_close(app);
                    wapp_wifi_down(app);
                    app->wifiRetry = 0U;
                    app->state     = eWifiAppStateWifiUp;
                    ALOGE("wapp_task: tcp reconnect failed: retries exhausted, "
                          "restarting wifi");
                }
            }
            else
            {
                /* recv error - only act on real errors (not EAGAIN/EWOULDBLOCK) */
                if((errno != EWOULDBLOCK) && (errno != EAGAIN))
                {
                    ALOGW("wapp_task: tcp recv error errno=%d", errno);
                    wapp_notify(app, eWifiAppStsTcpDisconnected, 0U);
                    (void)lwip_close((int)app->clientFd);
                    app->clientFd = -1;

                    backMs = wapp_backoff_ms(app, &app->tcpRetry);

                    if(backMs > 0U)
                    {
                        wapp_notify(app, eWifiAppStsTcpRetrying, (uint32_t)app->tcpRetry);
                        ALOGW("tcp reconnect retry=%" PRIu8
                              " backoffMs=%" PRIu16,
                              app->tcpRetry, backMs);
                        osDelay((uint32_t)backMs);
                        app->state = eWifiAppStateTcpListen;
                    }
                    else
                    {
                        wapp_notify(app, eWifiAppStsTcpFailed, 0U);
                        wapp_tcp_close(app);
                        wapp_wifi_down(app);
                        app->wifiRetry = 0U;
                        app->state     = eWifiAppStateWifiUp;
                        ALOGE("wapp_task: tcp error: retries exhausted, "
                              "restarting wifi");
                    }
                }
            }
            continue;
        }
    }

    /* Cleanup on task exit */
    ALOGW("wapp_task: exiting");
    wapp_tcp_close(app);
    wapp_wifi_down(app);
    osThreadExit();
}

/* =========================================================================
 * PUBLIC API
 * ========================================================================= */

bsp_err_sts_t wifi_app_init(wifi_app_t* app, const wifi_app_cfg_t* cfg)
{
    if((app == NULL) || (cfg == NULL) || (cfg->onStatus == NULL) || (cfg->onRx == NULL))
    {
        ALOGE("wifi_app_init: invalid param");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    (void)memset(app, 0, sizeof(*app));
    (void)memcpy(&app->cfg, cfg, sizeof(wifi_app_cfg_t));

    app->listenFd = -1;
    app->clientFd = -1;
    app->state    = eWifiAppStateIdle;

    g_appQueueCfg[APP_Q_OWNER_WIFI_APP].msg_size_bytes = sizeof(wapp_msg_t);
    app->wifiQ = osMessageQueueNew(g_appQueueCfg[APP_Q_OWNER_WIFI_APP].msg_count,
                                   sizeof(wapp_msg_t),
                                   &g_appQueueCfg[APP_Q_OWNER_WIFI_APP].attr);
    if(app->wifiQ == NULL)
    {
        ALOGE("wifi_app_init: queue create failed");
        return BSP_ERR_STS_NO_MEM;
    }

    app->running = 1U;
    app->taskId =
    osThreadNew(wapp_task, app, &g_appThreadCfg[APP_THREAD_OWNER_WIFI_APP].attr);

    if(app->taskId == NULL)
    {
        ALOGE("wifi_app_init: task create failed");
        (void)osMessageQueueDelete(app->wifiQ);
        return BSP_ERR_STS_NO_MEM;
    }

    return BSP_ERR_STS_OK;
}

bsp_err_sts_t wifi_app_deinit(wifi_app_t* app)
{
    if(app == NULL)
    {
        ALOGE("wifi_app_deinit: invalid param");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    app->running = 0U;
    (void)wifi_app_stop(app);
    osDelay(200U);

    if(app->wifiQ != NULL)
    {
        (void)osMessageQueueDelete(app->wifiQ);
        app->wifiQ = NULL;
    }

    return BSP_ERR_STS_OK;
}

bsp_err_sts_t wifi_app_start(wifi_app_t* app, const wifi_app_start_params_t* params)
{
    return wapp_post(app, (uint8_t)eWifiAppCmdStart, NULL, 0U, params);
}

bsp_err_sts_t wifi_app_stop(wifi_app_t* app)
{
    return wapp_post(app, (uint8_t)eWifiAppCmdStop, NULL, 0U, NULL);
}

bsp_err_sts_t wifi_app_send(wifi_app_t* app, const uint8_t* data, uint16_t len)
{
    return wapp_post(app, (uint8_t)eWifiAppCmdSend, data, len, NULL);
}