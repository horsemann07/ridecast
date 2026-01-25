


/*

WiFi BSP
  |
  +-- ConnMgr Task
        |
        +-- TCP Client Task (STA)
        |       nalHandle_t clientNal
        |
        +-- TCP Server Task (AP)
                nalHandle_t serverNal
                nalHandle_t clientNal

        +-- Backoff Algorithm Wrapper
                backoff_algorithm_t backoff
*/


#include "sys_config.h"
#include "bsp_wifi.h"
#include "nal.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"

#include "backoff_algorithm.h"

#include "esp_random.h"
#include "esp_log.h"
#include "esp_system.h"

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGI(fmt, ...)              ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)              ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)              ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)              ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)


#define WIFI_MGNR_WORKER_STACK_SIZE (1024)
#define WIFI_MGNR_WORKER_PRIORITY   (osPriorityNormal)

/* The maximum number of retries for the example code. */
#define WIFI_MGNR_RETRY_MAX_ATTEMPTS (10U)

/* The maximum back-off delay (in milliseconds) for between retries in the example. */
#define WIFI_MGNR_RETRY_MAX_BACKOFF_DELAY_MS (5000U)

/* The base back-off delay (in milliseconds) for retry configuration in the example. */
#define WIFI_MGNR_RETRY_BACKOFF_BASE_MS (500U)


#define WIFI_STA_SSID                   "myssid"
#define WIFI_STA_SSID_LEN               (strlen(WIFI_STA_SSID))
#define WIFI_STA_PASSWORD               "mypassword"
#define WIFI_STA_PASSWORD_LEN           (strlen(WIFI_STA_PASSWORD))
#define WIFI_STA_CHANNEL                (1)
#define WIFI_STA_SECURITY               (eWiFiSecurityWPA2)

#define WIFI_AP_SSID                    ("Ridecast")
#define WIFI_AP_SSID_LEN                (strlen(WIFI_AP_SSID))
#define WIFI_AP_PASSWORD                ("RidecastPassword")
#define WIFI_AP_PASSWORD_LEN            (strlen(WIFI_AP_PASSWORD))
#define WIFI_AP_CHANNEL                 (0)
#define WIFI_AP_HIDDEN                  (0)
#define WIFI_AP_MAX_CONNECTIONS         (4)
#define WIFI_AP_MAX_IDLE_PERIOD_SEC     (300)
#define WIFI_AP_SECURITY                (eWiFiSecurityWPA2)

#define TCP_SERVER_IP                   ("192.168.0.165")
#define TCP_SERVER_PORT                 (3333)
#define TCP_CONN_TIMEOUT_MS             (5000)
#define TCP_IO_TIMEOUT_MS               (3000)

#define AP_TCP_PORT                     (3333)
#define AP_TCP_IO_TIMEOUT_MS            (0) /* 3 seconds */
#define KEEPALIVE_SERVER_IDLE           (60)
#define KEEPALIVE_SERVER_INTERVAL       (5)
#define KEEPALIVE_SERVER_COUNT          (3)


static osThreadAttr_t connMgnrTaskAttr = {
    .name       = "conn_mgnr",
    .stack_size = WIFI_MGNR_WORKER_STACK_SIZE,
    .priority   = WIFI_MGNR_WORKER_PRIORITY,
};


#if defined(WIFI_STA_ENABLED)
static const osThreadAttr_t staTcpTaskAttr = {
    .name       = "sta_tcp",
    .stack_size = 4096, // TCP + lwIP needs more
    .priority   = osPriorityNormal,
};
#else
static const osThreadAttr_t apTcpTaskAttr = {
    .name       = "ap_tcp",
    .stack_size = 4096,
    .priority   = osPriorityNormal,
};
#endif


static osThreadId_t s_staTcpTask  = NULL;
static osThreadId_t s_apTcpTask   = NULL;
static osThreadId_t s_connMgrTask = NULL;

/* ------------ WIFI HANDLE ------------ */
static bspWifiHandle_t s_WifiHandle;

/* ---------- STA TCP client ---------- */
static nalHandle_t s_staTcpNal;
static bool s_staTcpConnected;

/* ---------- AP TCP server ---------- */
static nalHandle_t s_apServerNal;
static nalHandle_t s_apClientNal;

/* ---------- Conn manager ---------- */
static BackoffAlgorithmContext_t s_backoffCtx;


static bool s_apTcpListening = false;
static bool s_apTcpConnected = false;


static volatile bool s_staTcpRun = false;
static volatile bool s_apTcpRun  = false;

static bspWifiMode_t s_wifiMode = eWiFiModeStation;

static bspWifiStaConfig_t s_staCfg;
static bspWifiApConfig_t s_apCfg;


static void connMgnrBackoffInit(void)
{
    BackoffAlgorithm_InitializeParams(&s_backoffCtx, WIFI_MGNR_RETRY_BACKOFF_BASE_MS,
                                      WIFI_MGNR_RETRY_MAX_BACKOFF_DELAY_MS,
                                      WIFI_MGNR_RETRY_MAX_ATTEMPTS);
}

static bsp_err_sts_t tcpConnect(void)
{
    bsp_err_sts_t sts = nalNetworkConnect(&s_staTcpNal, TCP_SERVER_IP, TCP_SERVER_PORT,
                                          NAL_SCHEME_PLAIN, TCP_CONN_TIMEOUT_MS);

    s_staTcpConnected = (sts == BSP_ERR_STS_OK);
    return sts;
}


static void tcpDisconnect(void)
{
    if(s_staTcpConnected)
    {
        nalNetworkDisconnect(&s_staTcpNal);
        s_staTcpConnected = 0;
    }
}

static bsp_err_sts_t apTcpStartServer(void)
{
    if(s_apTcpListening)
        return BSP_ERR_STS_OK;

    memset(&s_apServerNal, 0, sizeof(s_apServerNal));

    bsp_err_sts_t sts =
    nalNetworkStartServer(&s_apServerNal, AP_TCP_PORT, NAL_SCHEME_PLAIN, 1);

    if(sts == BSP_ERR_STS_OK)
    {
        s_apTcpListening = true;
        LOGI("AP TCP server listening on port %d", AP_TCP_PORT);
    }

    return sts;
}


static void staTcpClientTask(void* arg)
{
    (void)arg;
    bsp_err_sts_t sts;

    for(;;)
    {
        sts = nalNetworkConnect(&s_staTcpNal, TCP_SERVER_IP, TCP_SERVER_PORT,
                                NAL_SCHEME_PLAIN, TCP_CONN_TIMEOUT_MS);
        if(sts != BSP_ERR_STS_OK)
        {
            LOGI("STA TCP connect failed: %s", bsp_err_sts_to_str(sts));
            osDelay(1000);
            continue;
        }

        LOGI("STA TCP connected");

        while(s_staTcpRun && s_WifiHandle.sta_connected)
        {
            uint8_t rxBuf[256];
            size_t rxLen = 0;

            sts = nalNetworkRecvSync(&s_staTcpNal, rxBuf, sizeof(rxBuf), &rxLen,
                                     TCP_IO_TIMEOUT_MS);

            if(sts == BSP_ERR_STS_OK && rxLen)
            {
                nalNetworkSendSync(&s_staTcpNal, rxBuf, rxLen, &rxLen, TCP_IO_TIMEOUT_MS);
            }
            else if(sts == BSP_ERR_STS_CONN_LOST)
            {
                break;
            }
        }

        nalNetworkDisconnect(&s_staTcpNal);
        osDelay(500);
    }
}


static void apTcpServerTask(void* arg)
{
    (void)arg;
    bsp_err_sts_t sts;


    s_apServerNal.keepAlive    = 1;
    s_apServerNal.keepIdle     = KEEPALIVE_SERVER_IDLE;
    s_apServerNal.keepInterval = KEEPALIVE_SERVER_INTERVAL;
    s_apServerNal.keepCount    = KEEPALIVE_SERVER_COUNT;


    BackoffAlgorithmStatus_t st;
    uint16_t delayMs;

    for(;;)
    {
        uint32_t count = 0;

        sts = nalNetworkStartServer(&s_apServerNal, AP_TCP_PORT, NAL_SCHEME_PLAIN, 1);
        if(sts != BSP_ERR_STS_OK)
        {
            LOGE("AP TCP server start failed %s", bsp_err_sts_to_str(sts));
            goto retry;
        }
        s_backoffCtx.attemptsDone = 0;


        LOGI("AP TCP server waiting for client");
        sts = nalNetworkAccept(&s_apServerNal, &s_apClientNal, AP_TCP_IO_TIMEOUT_MS);
        if(sts != BSP_ERR_STS_OK)
        {
            LOGE("AP TCP accept failed %s", bsp_err_sts_to_str(sts));
            continue;
        }

        LOGI("AP TCP client connected");
        s_backoffCtx.attemptsDone = 0;

        while(s_WifiHandle.ap_started || s_WifiHandle.sta_connected)
        {
            uint8_t buf[256];
            size_t len = 0;


            LOGI("AP TCP waiting for client data");
            sts = nalNetworkRecvSync(&s_apClientNal, buf, sizeof(buf), &len, UINT16_MAX);
            if(sts == BSP_ERR_STS_OK && len)
            {
                if(len < sizeof(buf))
                {
                    buf[len] = '\0';
                }
                else
                {
                    buf[sizeof(buf) - 1] = '\0';
                }

                LOGI("Received... %s", buf);
                s_backoffCtx.attemptsDone = 0;
            }
            else if(sts == BSP_ERR_STS_TIMEOUT)
            {
                LOGE("AP TCP recv failed %s", bsp_err_sts_to_str(sts));
                s_backoffCtx.attemptsDone = 0;
                continue;
            }
            else
            {
                LOGE("AP TCP recv failed %s", bsp_err_sts_to_str(sts));
                s_backoffCtx.attemptsDone = 0;
                goto retry;
            }

            sts = nalNetworkSendSync(&s_apClientNal, buf, len, &len, AP_TCP_IO_TIMEOUT_MS);
            if(sts != BSP_ERR_STS_OK)
            {
                LOGE("AP TCP send failed %s", bsp_err_sts_to_str(sts));
            }

            LOGI("AP TCP send successfully %s", buf);
            s_backoffCtx.attemptsDone = 0;

            osDelay(1000);

            continue;


        retry:
            LOGW("AP TCP server retrying backoff algorithm...");
            st = BackoffAlgorithm_GetNextBackoff(&s_backoffCtx, esp_random(), &delayMs);

            if(st == BackoffAlgorithmRetriesExhausted)
            {
                LOGE("AP TCP server retries exhausted...");
                break;
            }
            LOGW("AP TCP server retrying in %d ms", delayMs);
        }

        LOGI("AP TCP server disconeccting....");
        nalNetworkDisconnect(&s_apClientNal);
        s_apTcpConnected = 0;
        osThreadExit();
    }
    LOGI("AP TCP server task exiting");
    nalNetworkDisconnect(&s_apClientNal);
    s_apTcpConnected = 0;

    osThreadExit();
}

bsp_err_sts_t appConnectionInit(void)
{
    bsp_err_sts_t sts = BSP_ERR_STS_OK;


    sts = bspWifiInit(&s_WifiHandle);
    if(sts != BSP_ERR_STS_OK)
    {
        return sts;
    }

    nalNetworkInit(&s_staTcpNal);
    nalNetworkInit(&s_apServerNal);
    nalNetworkInit(&s_apClientNal);


#ifdef WIFI_STA_ENABLED
    memset(&s_staCfg, 0, sizeof(s_staCfg));
    LOGI("Starting STA mode");
    memcpy(s_staCfg.ssid, WIFI_STA_SSID, WIFI_STA_SSID_LEN);
    s_staCfg.ssidLength = WIFI_STA_SSID_LEN;

    memcpy(s_staCfg.password, WIFI_STA_PASSWORD, WIFI_STA_PASSWORD_LEN);
    s_staCfg.passwordLength = WIFI_STA_PASSWORD_LEN;
    s_staCfg.security       = WIFI_STA_SECURITY;


    sts = bspWifiOn(&s_WifiHandle, eWiFiModeStation, &s_staCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("bspWifiOn failed\n");
        esp_restart();
        return sts;
    }

    s_staTcpTask = osThreadNew(staTcpClientTask, NULL, &staTcpTaskAttr);
    if(s_staTcpTask == NULL)
    {
        LOGE("staTcpClientTask failed\n");
        esp_restart();
        return BSP_ERR_STS_NO_MEM;
    }


#else

    // memset(&s_staCfg, 0, sizeof(s_staCfg));
    // LOGI("Starting STA mode");
    // memcpy(s_staCfg.ssid, WIFI_STA_SSID, WIFI_STA_SSID_LEN);
    // s_staCfg.ssidLength = WIFI_STA_SSID_LEN;

    // memcpy(s_staCfg.password, WIFI_STA_PASSWORD, WIFI_STA_PASSWORD_LEN);
    // s_staCfg.passwordLength = WIFI_STA_PASSWORD_LEN;
    // s_staCfg.security       = WIFI_STA_SECURITY;

    // sts = bspWifiOn(&s_WifiHandle, eWiFiModeStation, &s_staCfg);
    // if(sts != BSP_ERR_STS_OK)
    // {
    //     LOGE("bspWifiOn failed\n");
    //     esp_restart();
    //     return sts;
    // }

    memset(&s_apCfg, 0, sizeof(s_apCfg));
    memcpy(s_apCfg.ssid, WIFI_AP_SSID, WIFI_AP_SSID_LEN);
    s_apCfg.ssidLength = WIFI_AP_SSID_LEN;

    s_apCfg.passwordLength = WIFI_AP_PASSWORD_LEN;
    memcpy(s_apCfg.password, WIFI_AP_PASSWORD, s_apCfg.passwordLength);

    s_apCfg.security         = WIFI_AP_SECURITY;
    s_apCfg.channel          = WIFI_AP_CHANNEL;
    s_apCfg.ssidHidden       = WIFI_AP_HIDDEN;
    s_apCfg.maxConnections   = WIFI_AP_MAX_CONNECTIONS;
    s_apCfg.maxIdlePeriodSec = WIFI_AP_MAX_IDLE_PERIOD_SEC;


    sts = bspWifiOn(&s_WifiHandle, eWiFiModeAP, &s_apCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("bspWifiOn failed\n");
        esp_restart();
        return sts;
    }


    s_apTcpTask = osThreadNew(apTcpServerTask, NULL, &apTcpTaskAttr);
    if(s_apTcpTask == NULL)
    {
        LOGE("apTcpServerTask failed\n");
        return BSP_ERR_STS_NO_MEM;
    }
#endif
    // s_connMgrTask = osThreadNew(connMgnrTask, NULL, &connMgnrTaskAttr);
    // if(s_connMgrTask == NULL)
    // {
    //     return BSP_ERR_STS_NO_MEM;
    // }


    return BSP_ERR_STS_OK;
}
