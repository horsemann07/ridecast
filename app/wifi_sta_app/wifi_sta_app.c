/**
 * @file wifi_sta_app.c
 * @brief WiFi Station Application – implementation.
 *
 * State flow
 * ──────────
 *
 *  ┌──────┐   ┌─────────┐   ┌─────────────┐   ┌─────────────┐   ┌────────────┐
 *  │ SCAN │──►│ CONNECT │──►│ SHOW_STATUS │──►│ TCP_CONNECT │──►│ RX_TO_UART │
 *  └──────┘   └─────────┘   └─────────────┘   └─────────────┘   └────────────┘
 *                 ▲  │                               │                 │
 *           retry │  │ fail                    fail  │           disconnect
 *                 │  ▼                               ▼                 ▼
 *               ┌─────────────┐◄────────────────────────────────────────┘
 *               │  RECONNECT  │
 *               └─────────────┘
 *
 * UART is used for:
 *   - Printing scan results (networks table)
 *   - Printing connection status (IP, GW, RSSI, channel)
 *   - Forwarding data received from the TCP socket
 */

#include "wifi_sta_app.h"

#include "sys_config.h"
#include "bsp_wifi.h"
#include "bsp_uart.h"
#include "nal.h"
#include "backoff_algorithm.h"

#include "cmsis_os2.h"
#include "esp_log.h"
#include "esp_random.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* =========================================================================
 * Logging (ESP-IDF monitor output)
 * ========================================================================= */

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGI(fmt, ...)  ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)  ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)  ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)

/* =========================================================================
 * ─── USER CONFIGURATION ──────────────────────────────────────────────────
 * ========================================================================= */

/* WiFi credentials */
#define APP_WIFI_SSID         "myssid"
#define APP_WIFI_PASSWORD     "mypassword"
#define APP_WIFI_SECURITY     eWiFiSecurityWPA2

/* Maximum APs returned by scan */
#define APP_SCAN_MAX_APS      (16U)

/* TCP server to receive data from */
#define APP_TCP_SERVER_IP     "192.168.0.100"
#define APP_TCP_SERVER_PORT   (3333)
#define APP_TCP_CONN_TIMEOUT_MS (5000U)

/* How long to wait for TCP data before looping (poll interval) */
#define APP_TCP_POLL_MS       (100U)

/* UART port for status output and data forwarding */
#define APP_UART_PORT         (2)
#define APP_UART_TX_PIN       (17)
#define APP_UART_RX_PIN       (16)
#define APP_UART_BAUD         BSP_UART_BAUD_115200
#define APP_UART_FIFO_SIZE    (1024U)
#define APP_UART_OVERSAMPLING (16U)
#define APP_UART_RX_THRESH    (8U)
#define APP_UART_SEND_TIMEOUT (2000U)   /* ms */

/* Back-off parameters */
#define BACKOFF_BASE_MS       (500U)
#define BACKOFF_MAX_MS        (10000U)
#define BACKOFF_MAX_ATTEMPTS  (10U)

/* Task */
#define APP_TASK_STACK        (4096U)
#define APP_TASK_PRIO         osPriorityNormal
#define APP_TASK_NAME         "wifi_sta_app"

/* =========================================================================
 * ─── MODULE STATE ────────────────────────────────────────────────────────
 * ========================================================================= */

static volatile staAppState_t s_state = STA_APP_STATE_SCAN;

static bspWifiHandle_t    s_wifi;
static bspUartHandle_t    s_uart;
static nalHandle_t        s_tcp;
static bool               s_uartReady = false;

static BackoffAlgorithmContext_t s_backoff;

/* =========================================================================
 * ─── UART HELPERS ────────────────────────────────────────────────────────
 * ========================================================================= */

/**
 * @brief Write raw bytes to UART (fire-and-forget; log errors only).
 */
static void uartWrite(const uint8_t* data, size_t len)
{
    if (!s_uartReady || data == NULL || len == 0)
        return;

    bsp_err_sts_t sts = bspUartSendSync(&s_uart, data, len, APP_UART_SEND_TIMEOUT);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGE("[UART] Write failed: %s", bsp_err_sts_to_str(sts));
    }
}

/**
 * @brief printf-style helper – formats into a stack buffer then sends to UART.
 */
static void uartPrintf(const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
        uartWrite((const uint8_t*)buf, (size_t)len);
    }
}

/* =========================================================================
 * ─── BACK-OFF ─────────────────────────────────────────────────────────────
 * ========================================================================= */

static void backoffReset(void)
{
    BackoffAlgorithm_InitializeParams(&s_backoff,
                                      BACKOFF_BASE_MS,
                                      BACKOFF_MAX_MS,
                                      BACKOFF_MAX_ATTEMPTS);
}

static void backoffWait(void)
{
    uint16_t delay = 0;
    BackoffAlgorithmStatus_t st =
        BackoffAlgorithm_GetNextBackoff(&s_backoff, esp_random(), &delay);

    if (st == BackoffAlgorithmRetriesExhausted)
    {
        LOGW("[BACKOFF] Exhausted – waiting 10 s");
        uartPrintf("[BACKOFF] Retries exhausted. Waiting 10 s...\r\n");
        osDelay(10000);
        backoffReset();
    }
    else
    {
        LOGW("[BACKOFF] Waiting %u ms", (unsigned)delay);
        uartPrintf("[BACKOFF] Retry in %u ms...\r\n", (unsigned)delay);
        osDelay(delay);
    }
}

/* =========================================================================
 * ─── STATE TRANSITION ─────────────────────────────────────────────────────
 * ========================================================================= */

static void goTo(staAppState_t next)
{
    LOGI("[STATE] %s → %s",
         wifiStaAppStateToStr(s_state), wifiStaAppStateToStr(next));
    s_state = next;
}

/* =========================================================================
 * ─── STATE HANDLER: SCAN ─────────────────────────────────────────────────
 * ========================================================================= */

/**
 * Scan nearby WiFi networks and print a formatted table over UART.
 * Always moves to CONNECT afterwards (scan is informational only).
 */
static void handleScan(void)
{
    LOGI("[SCAN] Scanning for WiFi networks...");
    uartPrintf("\r\n=== WiFi Scan Results ===\r\n");

    bspWifiApScanResult_t results[APP_SCAN_MAX_APS];
    memset(results, 0, sizeof(results));

    bsp_err_sts_t sts = bspWifiStartScan(&s_wifi, results, APP_SCAN_MAX_APS);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGW("[SCAN] Scan failed: %s – skipping", bsp_err_sts_to_str(sts));
        uartPrintf("[SCAN] Scan failed: %s\r\n", bsp_err_sts_to_str(sts));
        goTo(STA_APP_STATE_CONNECT);
        return;
    }

    /* num_result is stored in the first entry by the BSP */
    uint8_t total = results[0].num_result;
    if (total == 0 || total > APP_SCAN_MAX_APS)
        total = APP_SCAN_MAX_APS;

    /* Print table header to UART */
    uartPrintf("%-32s  %5s  %4s  %-10s\r\n", "SSID", "RSSI", "CH", "Security");
    uartPrintf("%-32s  %5s  %4s  %-10s\r\n",
               "--------------------------------",
               "-----", "----", "----------");

    uint8_t printed = 0;
    for (uint8_t i = 0; i < total; i++)
    {
        if (results[i].ssidLength == 0)
            break;

        char ssid[BSP_WIFI_SSID_MAX_LEN + 1];
        memcpy(ssid, results[i].ssid, results[i].ssidLength);
        ssid[results[i].ssidLength] = '\0';

        const char* sec;
        switch (results[i].security)
        {
            case eWiFiSecurityOpen:  sec = "Open";    break;
            case eWiFiSecurityWEP:   sec = "WEP";     break;
            case eWiFiSecurityWPA:   sec = "WPA";     break;
            case eWiFiSecurityWPA2:  sec = "WPA2";    break;
            case eWiFiSecurityWPA3:  sec = "WPA3";    break;
            default:                  sec = "Unknown"; break;
        }

        uartPrintf("%-32s  %3d dBm  ch%2d  %-10s\r\n",
                   ssid,
                   (int)results[i].rssi,
                   results[i].channel,
                   sec);

        LOGI("[SCAN] %-32s  %3d dBm  ch%2d  %s",
             ssid, (int)results[i].rssi, results[i].channel, sec);

        printed++;
    }

    uartPrintf("Total: %u network(s) found.\r\n\r\n", (unsigned)printed);
    LOGI("[SCAN] %u network(s) found", (unsigned)printed);

    goTo(STA_APP_STATE_CONNECT);
}

/* =========================================================================
 * ─── STATE HANDLER: CONNECT ──────────────────────────────────────────────
 * ========================================================================= */

/**
 * Connect to the configured SSID.
 * On success → SHOW_STATUS.
 * On failure → back-off, retry CONNECT.
 */
static void handleConnect(void)
{
    LOGI("[CONNECT] Connecting to '%s'...", APP_WIFI_SSID);
    uartPrintf("[WiFi] Connecting to '%s'...\r\n", APP_WIFI_SSID);

    bspWifiStaConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.ssidLength = (uint8_t)strlen(APP_WIFI_SSID);
    memcpy(cfg.ssid, APP_WIFI_SSID, cfg.ssidLength);

    cfg.passwordLength = (uint8_t)strlen(APP_WIFI_PASSWORD);
    memcpy(cfg.password, APP_WIFI_PASSWORD, cfg.passwordLength);

    cfg.security = APP_WIFI_SECURITY;

    bsp_err_sts_t sts = bspWifiOn(&s_wifi, eWiFiModeStation, &cfg);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGE("[CONNECT] Failed: %s", bsp_err_sts_to_str(sts));
        uartPrintf("[WiFi] Connection failed: %s\r\n", bsp_err_sts_to_str(sts));
        backoffWait();
        goTo(STA_APP_STATE_CONNECT);   /* retry */
        return;
    }

    backoffReset();
    LOGI("[CONNECT] Success");
    uartPrintf("[WiFi] Connected!\r\n");
    goTo(STA_APP_STATE_SHOW_STATUS);
}

/* =========================================================================
 * ─── STATE HANDLER: SHOW_STATUS ──────────────────────────────────────────
 * ========================================================================= */

/**
 * Drain the WiFi event queue to get IP details, then print a status
 * block over UART. Always moves to TCP_CONNECT.
 */
static void handleShowStatus(void)
{
    uartPrintf("\r\n=== WiFi Connection Status ===\r\n");

    bspWifiContext_t ctx;
    bool gotIp = false;

    /* Drain any queued events – look for the IP-ready event */
    while (osMessageQueueGet(s_wifi.evt_q, &ctx, NULL, 0) == osOK)
    {
        if (ctx.tEventType == eBSPWifiEventIPReady ||
            ctx.tEventType == eBSPWifiEventSTAGotIP)
        {
            gotIp = true;
            bspWifiConnectionInfo_t* ci = &ctx.tEventData.tConnectionInfo;

            /* SSID */
            char ssid[BSP_WIFI_SSID_MAX_LEN + 1];
            memcpy(ssid, ci->ssid, ci->ssidLength);
            ssid[ci->ssidLength] = '\0';

            /* IPv4 is stored little-endian in address[0] */
            uint32_t ip = ci->ip.ipAddress.address[0];
            uint32_t gw = ci->ip.gateway.address[0];

            /* Print to UART */
            uartPrintf("  SSID     : %s\r\n", ssid);
            uartPrintf("  IP       : %lu.%lu.%lu.%lu\r\n",
                       (ip >>  0) & 0xFF, (ip >>  8) & 0xFF,
                       (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
            uartPrintf("  Gateway  : %lu.%lu.%lu.%lu\r\n",
                       (gw >>  0) & 0xFF, (gw >>  8) & 0xFF,
                       (gw >> 16) & 0xFF, (gw >> 24) & 0xFF);
            uartPrintf("  RSSI     : %d dBm\r\n", (int)ci->rssi);
            uartPrintf("  Channel  : %d\r\n",     (int)ci->channel);

            /* Mirror to ESP monitor */
            LOGI("[STATUS] SSID=%s  IP=%lu.%lu.%lu.%lu  RSSI=%d dBm",
                 ssid,
                 (ip >>  0) & 0xFF, (ip >>  8) & 0xFF,
                 (ip >> 16) & 0xFF, (ip >> 24) & 0xFF,
                 (int)ci->rssi);
        }
    }

    if (!gotIp)
    {
        /* Event was already consumed before we got here – show what we know */
        uartPrintf("  SSID     : %s\r\n",   APP_WIFI_SSID);
        uartPrintf("  State    : Connected (IP event not queued yet)\r\n");
        LOGI("[STATUS] Connected (IP event already consumed)");
    }

    uartPrintf("==============================\r\n\r\n");
    goTo(STA_APP_STATE_TCP_CONNECT);
}

/* =========================================================================
 * ─── STATE HANDLER: TCP_CONNECT ──────────────────────────────────────────
 * ========================================================================= */

/**
 * Open a plain-TCP connection to APP_TCP_SERVER_IP:APP_TCP_SERVER_PORT.
 * On success → RX_TO_UART.
 * On failure → back-off, retry; if WiFi is also gone → CONNECT.
 */
static void handleTcpConnect(void)
{
    /* Check WiFi is still up */
    if (!s_wifi.sta_connected)
    {
        LOGW("[TCP] WiFi lost – reconnecting");
        uartPrintf("[TCP] WiFi lost. Reconnecting...\r\n");
        goTo(STA_APP_STATE_CONNECT);
        return;
    }

    LOGI("[TCP] Connecting to %s:%d ...", APP_TCP_SERVER_IP, APP_TCP_SERVER_PORT);
    uartPrintf("[TCP] Connecting to %s:%d ...\r\n",
               APP_TCP_SERVER_IP, APP_TCP_SERVER_PORT);

    memset(&s_tcp, 0, sizeof(s_tcp));
    nalNetworkInit(&s_tcp);

    bsp_err_sts_t sts = nalNetworkConnect(&s_tcp,
                                           APP_TCP_SERVER_IP,
                                           APP_TCP_SERVER_PORT,
                                           NAL_SCHEME_PLAIN,
                                           APP_TCP_CONN_TIMEOUT_MS);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGE("[TCP] Connect failed: %s", bsp_err_sts_to_str(sts));
        uartPrintf("[TCP] Connect failed: %s\r\n", bsp_err_sts_to_str(sts));
        backoffWait();
        goTo(STA_APP_STATE_TCP_CONNECT);
        return;
    }

    backoffReset();
    LOGI("[TCP] Connected to %s:%d", APP_TCP_SERVER_IP, APP_TCP_SERVER_PORT);
    uartPrintf("[TCP] Connected to %s:%d\r\n",
               APP_TCP_SERVER_IP, APP_TCP_SERVER_PORT);
    uartPrintf("[TCP] Waiting for data from server...\r\n\r\n");

    goTo(STA_APP_STATE_RX_TO_UART);
}

/* =========================================================================
 * ─── STATE HANDLER: RX_TO_UART ───────────────────────────────────────────
 * ========================================================================= */

/**
 * Receive data from the TCP socket and forward it byte-for-byte to UART.
 *
 * Called repeatedly from the task loop while in this state.
 *   - Timeout on recv is normal (no data yet) – stay in state.
 *   - Any hard TCP error → RECONNECT.
 */
static void handleRxToUart(void)
{
    uint8_t buf[256];
    size_t  rxLen = 0;

    bsp_err_sts_t sts = nalNetworkRecvSync(&s_tcp, buf, sizeof(buf),
                                            &rxLen, APP_TCP_POLL_MS);

    if (sts == BSP_ERR_STS_OK && rxLen > 0)
    {
        LOGI("[RX→UART] %zu byte(s) received from TCP", rxLen);

        /* ── Write data over UART ── */
        uartWrite(buf, rxLen);
    }
    else if (sts == BSP_ERR_STS_TIMEOUT)
    {
        /* Normal poll timeout – no data this cycle, stay in state */
    }
    else
    {
        /* TCP connection broken */
        LOGW("[RX→UART] TCP disconnected: %s", bsp_err_sts_to_str(sts));
        uartPrintf("\r\n[TCP] Disconnected: %s\r\n", bsp_err_sts_to_str(sts));

        nalNetworkDisconnect(&s_tcp);
        goTo(STA_APP_STATE_RECONNECT);
    }
}

/* =========================================================================
 * ─── STATE HANDLER: RECONNECT ────────────────────────────────────────────
 * ========================================================================= */

/**
 * Apply back-off, then choose the right recovery path:
 *   WiFi down → go to CONNECT
 *   WiFi OK   → go to TCP_CONNECT
 */
static void handleReconnect(void)
{
    LOGI("[RECONNECT] Assessing recovery...");
    uartPrintf("[RECONNECT] Waiting before retry...\r\n");

    backoffWait();

    if (!s_wifi.sta_connected)
    {
        LOGW("[RECONNECT] WiFi gone → CONNECT");
        uartPrintf("[RECONNECT] WiFi lost – reconnecting to AP...\r\n");
        goTo(STA_APP_STATE_CONNECT);
    }
    else
    {
        LOGI("[RECONNECT] WiFi OK → TCP_CONNECT");
        uartPrintf("[RECONNECT] WiFi OK – reconnecting TCP...\r\n");
        goTo(STA_APP_STATE_TCP_CONNECT);
    }
}

/* =========================================================================
 * ─── APPLICATION TASK ────────────────────────────────────────────────────
 * ========================================================================= */

static void wifiStaAppTask(void* arg)
{
    (void)arg;

    backoffReset();

    /* ── 1. Initialise UART ── */
    static bspUartHandle_t uartCfg = {
        .portNum      = APP_UART_PORT,
        .baudrate     = APP_UART_BAUD,
        .wordLength   = eBspUartWordLength8,
        .parity       = eBspUartParityNone,
        .stopBits     = eBspUartStopBitsOne,
        .mode         = eBspUartModeTxRx,
        .uartTxPin    = APP_UART_TX_PIN,
        .uartRxPin    = APP_UART_RX_PIN,
        .fifoSize     = APP_UART_FIFO_SIZE,
        .oversampling = APP_UART_OVERSAMPLING,
        .rxThreshold  = APP_UART_RX_THRESH,
    };

    memcpy(&s_uart, &uartCfg, sizeof(s_uart));

    bsp_err_sts_t sts = bspUartInit(&s_uart);
    if (sts == BSP_ERR_STS_OK)
    {
        s_uartReady = true;
        LOGI("[UART] Initialized port %d  TX=%d  RX=%d  %lu baud",
             APP_UART_PORT, APP_UART_TX_PIN, APP_UART_RX_PIN,
             (unsigned long)APP_UART_BAUD);
        uartPrintf("\r\n============================\r\n");
        uartPrintf("  WiFi Station Application\r\n");
        uartPrintf("============================\r\n\r\n");
    }
    else
    {
        LOGE("[UART] Init failed: %s", bsp_err_sts_to_str(sts));
        /* UART not available – app continues using ESP monitor only */
    }

    /* ── 2. Initialise WiFi BSP ── */
    sts = bspWifiInit(&s_wifi);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGE("[WIFI] bspWifiInit failed: %s", bsp_err_sts_to_str(sts));
        uartPrintf("[WIFI] Init failed: %s\r\n", bsp_err_sts_to_str(sts));
        osThreadExit();
        return;
    }

    /* ── 3. Start WiFi in STA mode (required before scan) ── */
    bspWifiStaConfig_t initCfg;
    memset(&initCfg, 0, sizeof(initCfg));
    initCfg.ssidLength = (uint8_t)strlen(APP_WIFI_SSID);
    memcpy(initCfg.ssid, APP_WIFI_SSID, initCfg.ssidLength);
    initCfg.passwordLength = (uint8_t)strlen(APP_WIFI_PASSWORD);
    memcpy(initCfg.password, APP_WIFI_PASSWORD, initCfg.passwordLength);
    initCfg.security = APP_WIFI_SECURITY;

    sts = bspWifiOn(&s_wifi, eWiFiModeStation, &initCfg);
    if (sts != BSP_ERR_STS_OK)
    {
        LOGE("[WIFI] Initial bspWifiOn failed: %s – scan will be skipped",
             bsp_err_sts_to_str(sts));
        /* Fall straight to connect state; scan will be skipped */
        goTo(STA_APP_STATE_CONNECT);
    }

    /* ── 4. Init NAL TCP handle ── */
    nalNetworkInit(&s_tcp);

    /* ── 5. State machine loop ── */
    for (;;)
    {
        switch (s_state)
        {
            case STA_APP_STATE_SCAN:        handleScan();        break;
            case STA_APP_STATE_CONNECT:     handleConnect();     break;
            case STA_APP_STATE_SHOW_STATUS: handleShowStatus();  break;
            case STA_APP_STATE_TCP_CONNECT: handleTcpConnect();  break;
            case STA_APP_STATE_RX_TO_UART:  handleRxToUart();    break;
            case STA_APP_STATE_RECONNECT:   handleReconnect();   break;
            default:
                LOGE("[SM] Unknown state %d – reset to SCAN", (int)s_state);
                goTo(STA_APP_STATE_SCAN);
                break;
        }
    }
}

/* =========================================================================
 * ─── PUBLIC API ───────────────────────────────────────────────────────────
 * ========================================================================= */

bsp_err_sts_t wifiStaAppStart(void)
{
    static const osThreadAttr_t attr = {
        .name       = APP_TASK_NAME,
        .stack_size = APP_TASK_STACK,
        .priority   = APP_TASK_PRIO,
    };

    osThreadId_t tid = osThreadNew(wifiStaAppTask, NULL, &attr);
    if (tid == NULL)
    {
        LOGE("Failed to create wifiStaAppTask");
        return BSP_ERR_STS_NO_MEM;
    }

    LOGI("WiFi STA app task spawned");
    return BSP_ERR_STS_OK;
}

staAppState_t wifiStaAppGetState(void)
{
    return s_state;
}

const char* wifiStaAppStateToStr(staAppState_t state)
{
    switch (state)
    {
        case STA_APP_STATE_SCAN:        return "SCAN";
        case STA_APP_STATE_CONNECT:     return "CONNECT";
        case STA_APP_STATE_SHOW_STATUS: return "SHOW_STATUS";
        case STA_APP_STATE_TCP_CONNECT: return "TCP_CONNECT";
        case STA_APP_STATE_RX_TO_UART:  return "RX_TO_UART";
        case STA_APP_STATE_RECONNECT:   return "RECONNECT";
        default:                        return "UNKNOWN";
    }
}
