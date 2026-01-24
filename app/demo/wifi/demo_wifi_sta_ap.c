
#include "sys_config.h"
#include "cmsis_os2.h"
#include "bsp_wifi.h"

#include "esp_log.h"

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGI(fmt, ...)        ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)        ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)        ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)        ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)


#define WIFI_AP_TASK_STACK    2048
#define WIFI_AP_TASK_PRIO     osPriorityNormal

#define WIFI_STA_TASK_STACK   2048
#define WIFI_STA_TASK_PRIO    osPriorityNormal

#define WIFI_STA_SSID         "Airtel_GhostHouse"
#define WIFI_STA_SSID_LEN     (strlen(WIFI_STA_SSID))
#define WIFI_STA_PASSWORD     "TheConjuring"
#define WIFI_STA_PASSWORD_LEN (strlen(WIFI_STA_PASSWORD))
#define WIFI_STA_CHANNEL      1


static bspWifiHandle_t g_wifiHandle = {0};
static osThreadId_t wifiApTaskId = NULL;

static void wifi_app_task(void* arg)
{
    bspWifiHandle_t* handle = (bspWifiHandle_t*)arg;
    bspWifiContext_t evt;

    for(;;)
    {
        /* Wait indefinitely for Wi-Fi events */
        if(osMessageQueueGet(handle->evt_q, &evt, NULL, osWaitForever) == osOK)
        {
            switch(evt.tEventType)
            {
                case eBSPWifiEventIPReady:
                    LOGI("WiFi connected, IP ready\n");
                    /* Start network services here */
                    break;

                case eBSPWifiEventSTADisconnected:
                    LOGI("WiFi disconnected (reason=%d)\n",
                         evt.tEventData.tConnectionInfo.lastDisconnectReason);
                    /* Retry logic or backoff */
                    break;

                case eBSPWifiEventScanDone:
                    LOGI("WiFi scan completed\n");
                    break;

                case eBSPWifiEventAPStarted:
                    LOGI("Access Point started\n");
                    break;

                case eBSPWifiEventAPStationConnected:
                    LOGI("Station connected\n");
                    break;

                case eBSPWifiEventAPStationDisconnected:
                    LOGI("Station disconnected\n");
                    break;

                default:
                    LOGI("WiFi event: %d\n", evt.tEventType);
                    break;
            }
        }
    }
}


static void fill_ap_config(bspWifiApConfig_t* cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    memcpy(cfg->ssid, "RideCast_AP", 11);
    cfg->ssidLength = 11;

    memcpy(cfg->password, "ridecast123", 11);
    cfg->passwordLength = 11;

    cfg->security         = eWiFiSecurityWPA2;
    cfg->channel          = 6;
    cfg->ssidHidden       = 0;
    cfg->maxConnections   = 4;
    cfg->maxIdlePeriodSec = 300;
}

void wifi_sta_app_start(void)
{
    bsp_err_sts_t sts;

    /* -------- Init Wi-Fi BSP -------- */
    sts = bspWifiInit(&g_wifiHandle);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("bspWifiInit failed: %s\n", bsp_err_sts_to_str(sts));
        return;
    }

    /* -------- STA configuration -------- */
    bspWifiStaConfig_t staCfg = { 0 };

    staCfg.ssidLength = WIFI_STA_SSID_LEN;
    memcpy(staCfg.ssid, WIFI_STA_SSID, staCfg.ssidLength);

    staCfg.passwordLength = WIFI_STA_PASSWORD_LEN;
    memcpy(staCfg.password, WIFI_STA_PASSWORD, staCfg.passwordLength);


    staCfg.security = eWiFiSecurityWPA2;

    /* -------- Start Wi-Fi -------- */
    sts = bspWifiOn(&g_wifiHandle, eWiFiModeStation, &staCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("bspWifiOn failed\n");
        return;
    }

    /* -------- Create application task -------- */
    wifiApTaskId = osThreadNew(wifi_app_task, &g_wifiHandle,
                               &(osThreadAttr_t){ .name = "wifi_sta_app",
                                                  .priority = WIFI_STA_TASK_PRIO,
                                                  .stack_size = WIFI_STA_TASK_STACK });

    if(!wifiApTaskId)
    {
        LOGE("Failed to create WiFi task\n");
    }
}

void wifi_ap_app_start(void)
{
    bsp_err_sts_t sts;
    bspWifiApConfig_t apCfg;

    /* -------- Create RTOS primitives -------- */
    g_wifiHandle.lock  = osMutexNew(NULL);
    g_wifiHandle.evt_q = osMessageQueueNew(8, sizeof(bspWifiContext_t), NULL);

    if(!g_wifiHandle.lock || !g_wifiHandle.evt_q)
    {
        LOGE("WiFi AP app: RTOS init failed\n");
        return;
    }

    /* -------- Initialize BSP -------- */
    sts = bspWifiInit(&g_wifiHandle);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("WiFi AP app: bspWifiInit failed\n");
        return;
    }

    /* -------- Fill AP config -------- */
    fill_ap_config(&apCfg);

    /* -------- Start Wi-Fi in AP mode -------- */
    sts = bspWifiOn(&g_wifiHandle, eWiFiModeAP, &apCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        LOGE("WiFi AP app: bspWifiOn(AP) failed\n");
        return;
    }

    /* -------- Create AP task -------- */
    wifiApTaskId = osThreadNew(wifi_app_task, &g_wifiHandle,
                               &(osThreadAttr_t){ .name     = "wifi_ap_task",
                                                  .priority = WIFI_AP_TASK_PRIO,
                                                  .stack_size = WIFI_AP_TASK_STACK });

    if(!wifiApTaskId)
    {
        LOGE("WiFi AP app: task creation failed\n");
    }
}
