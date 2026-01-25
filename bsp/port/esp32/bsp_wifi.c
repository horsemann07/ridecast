/**
 * @file bsp_wifi.c
 * @brief Board Support Package (BSP) for WiFi functionality.
 *
 * This file provides the implementation of WiFi-related functions and interfaces
 * for the board support package. It abstracts the hardware-specific details of
 * WiFi operations, enabling higher-level application code to interact with WiFi
 * hardware in a platform-independent manner.
 *
 * The functionalities typically include initialization, configuration, connection
 * management, and data transmission for WiFi modules supported by the board.
 */

#define ESP_BOARD_LWIP
#define ESP_BOARD_LOGGING

// Include necessary headers
#include <string.h>

// Include the corresponding header file for WiFi BSP
#include "bsp_wifi.h"

/* ESP-IDF */
#include "esp_wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_smartconfig.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif_ip_addr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include "cmsis_os2.h"

/*
 ******************************************************************************
 *                            LOG   MACROS
 ******************************************************************************
 */

#define WIFI_LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)


/*
 ******************************************************************************
 *                            BSP CONFIGURED MACROS
 ******************************************************************************
 */
#define BSP_WIFI_EVENT_QUEUE_LEN              10
#define BSP_WIFI_WAIT_PERIOD_MS               (5000U)
#define BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS 5

/*
 *******************************************************************************
 *                            FILE MACROS
 ******************************************************************************
 */

#define CHECK_VALID_WIFI_SSID_LEN(x) ((x) > 0 && (x) <= BSP_WIFI_SSID_MAX_LEN)

#define CHECK_VALID_PASSPHRASE_LEN(x) \
    ((x) > 0 && (x) <= BSP_WIFI_PASSWORD_MAX_LEN)

#define CHECK_VALID_IP_ADDR_LEN(x) ((x) > 0 && (x) <= BSP_WIFI_IP_ADDR_MAX_LEN)

#define WIFI_MAC_MATCH(mac1, mac2) \
    ((memcmp((mac1), (mac2), BSP_WIFI_MAC_ADDR_MAX_LEN) == 0) ? true : false)


/**
 * @brief Semaphore flags
 *
 * @note These flags are used to signal the BSP wifi task.
 */
#define BSP_WIFI_F_STA_CONNECTED    (1 << 0)
#define BSP_WIFI_F_STA_DISCONNECTED (1 << 1)
#define BSP_WIFI_F_STA_STARTED      (1 << 2)
#define BSP_WIFI_F_STA_STOPPED      (1 << 3)
#define BSP_WIFI_F_AP_STARTED       (1 << 4)
#define BSP_WIFI_F_AP_STOPPED       (1 << 5)
#define BSP_WIFI_F_WPS_SUCCESS      (1 << 6)
#define BSP_WIFI_F_WPS_FAILED       (1 << 7)
#define BSP_WIFI_F_WIFI_STARTED     (1 << 8)
#define BSP_WIFI_F_WIFI_STOPPED     (1 << 9)
#define BSP_WIFI_F_AUTH_FAILED      (1 << 10)
#define BSP_WIFI_F_SCAN_DONE        (1 << 11)
#define BSP_WIFI_F_IP_READY         (1 << 12)


/**
 * @brief NVS namespace and keys
 *
 * @note These are used to store and retrieve wifi configuration data.
 */
#define BSP_WIFI_NVS_NAMESPACE     "bsp_wifi"
#define BSP_WIFI_NVS_STA_COUNT_KEY "sta_count"
#define BSP_WIFI_MAX_SAVED_STA     5
#define BSP_WIFI_NVS_STA_KEY_FMT   "sta_%u"


/*
 *******************************************************************************
 *                            PRIVATE STRUCTS
 ******************************************************************************
 */
typedef struct
{
    /* ---------- ESP-IDF objects ---------- */
    esp_netif_t* sta_netif;
    esp_netif_t* ap_netif;

    esp_event_handler_instance_t wifi_any_id;
    esp_event_handler_instance_t ip_got_ip;

    /* ---------- BSP internal sync ---------- */
    osEventFlagsId_t evt_flags; /**< Internal BSP sync flags */

    /* ---------- Platform runtime ---------- */
    bspWifiMode_t current_mode;
    uint8_t initialized;

} espWifiPlatformCtx_t;

/*
 *******************************************************************************
 *                            PRIVATE VARIABLES
 ******************************************************************************
 */
/* ESP event loop state */
static bool s_eventLoopInited = false;

/* ESP event handler instances */
static espWifiPlatformCtx_t g_espWifiCtx;


//-------------------------------------------------------------------
/*
 * WIFI Semaphore lock/Unlock
 */

#define WIFI_LOCK(h)                                                     \
    do                                                                   \
    {                                                                    \
        if((h) && (h->lock))                                             \
        {                                                                \
            if(osMutexAcquire(h->lock, BSP_WIFI_WAIT_PERIOD_MS) != osOK) \
            {                                                            \
                WIFI_LOGE("Failed to acquire WiFi semaphore");           \
                return BSP_ERR_STS_TIMEOUT;                              \
            }                                                            \
        }                                                                \
    } while(0)


#define WIFI_UNLOCK(h)               \
    do                               \
    {                                \
        if((h) && (h->lock))         \
        {                            \
            osMutexRelease(h->lock); \
        }                            \
    } while(0)

//-------------------------------------------------------------------
//-------------------------------------------------------------------
#define WIFI_EVENT_SET(evtGrp, F)                  \
    do                                             \
    {                                              \
        if(osEventFlagsSet((evtGrp), (F)) < 0)     \
        {                                          \
            WIFI_LOGE("Failed to set WiFi event"); \
            return BSP_ERR_STS_FAIL;               \
        }                                          \
    } while(0)

//-------------------------------------------------------------------
#define WIFI_EVENT_WAIT(evtGrp, F)                                                \
    do                                                                            \
    {                                                                             \
        uint32_t __ret =                                                          \
        osEventFlagsWait((evtGrp), (F), osFlagsWaitAny, BSP_WIFI_WAIT_PERIOD_MS); \
        if((__ret & (F)) == 0U)                                                   \
        {                                                                         \
            WIFI_LOGE("WiFi event wait timeout");                                 \
            return BSP_ERR_STS_TIMEOUT;                                           \
        }                                                                         \
    } while(0)
//-------------------------------------------------------------------
#define WIFI_EVENT_CLEAR(evtGrp, F)                  \
    do                                               \
    {                                                \
        if(osEventFlagsClear((evtGrp), (F)) != 0)    \
        {                                            \
            WIFI_LOGE("Failed to clear WiFi event"); \
            return BSP_ERR_STS_FAIL;                 \
        }                                            \
    } while(0)

//-------------------------------------------------------------------
#define WIFI_EVENT_WAIT_ANY(evtGrp, F) \
    osEventFlagsWait((evtGrp), (F), osFlagsWaitAny, BSP_WIFI_WAIT_PERIOD_MS)

//-------------------------------------------------------------------

/*
 *******************************************************************************
 *                            PRIAVTE HELPER FUNCITONS
 ******************************************************************************
 */

/* ********************************************************************** */
static bspWifiPhyMode_t esp_map_phy_mode(const wifi_sta_info_t* sta)
{
    if(sta->phy_11ax)
        return eBSPWifiPhy11ax;
    if(sta->phy_11ac)
        return eBSPWifiPhy11ac;
    if(sta->phy_11n)
        return eBSPWifiPhy11n;
    if(sta->phy_11g)
        return eBSPWifiPhy11g;
    if(sta->phy_11b)
        return eBSPWifiPhy11b;
    if(sta->phy_lr)
        return eBSPWifiPhyLR;

    return eBSPWifiPhyNone;
}
/* ********************************************************************** */
static bspWifiSecurity_t esp_authmode_to_bsp(wifi_auth_mode_t mode)
{
    switch(mode)
    {
        case WIFI_AUTH_OPEN:
            return eWiFiSecurityOpen;

        case WIFI_AUTH_WEP:
            return eWiFiSecurityWEP;

        case WIFI_AUTH_WPA_PSK:
            return eWiFiSecurityWPA;

        case WIFI_AUTH_WPA2_PSK:
            return eWiFiSecurityWPA2;

        case WIFI_AUTH_WPA_WPA2_PSK:
            return eWiFiSecurityWPA2;

        case WIFI_AUTH_WPA2_ENTERPRISE:
            return eWiFiSecurityWPA2_ent;

        case WIFI_AUTH_WPA3_PSK:
            return eWiFiSecurityWPA3;

        case WIFI_AUTH_WPA2_WPA3_PSK:
            return eWiFiSecurityWPA3;

        default:
            return eWiFiSecurityNotSupported;
    }
}
/* ********************************************************************** */
static bspWifiReason_t convertReasonCode(uint8_t u8ReasonCode)
{
    bspWifiReason_t retCode;
    switch(u8ReasonCode)
    {
        case WIFI_REASON_AUTH_EXPIRE:
            retCode = eBSPWifiReasonAuthExpired;
            break;
        case WIFI_REASON_ASSOC_EXPIRE:
            retCode = eBSPWifiReasonAssocExpired;
            break;
        case WIFI_REASON_AUTH_LEAVE:
            retCode = eBSPWifiReasonAuthLeaveBSS;
            break;
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            retCode = eBSPWifiReason4WayTimeout;
            break;
        case WIFI_REASON_BEACON_TIMEOUT:
            retCode = eBSPWifiReasonBeaconTimeout;
            break;
        case WIFI_REASON_AUTH_FAIL:
            retCode = eBSPWifiReasonAuthFailed;
            break;
        case WIFI_REASON_ASSOC_FAIL:
            retCode = eBSPWifiReasonAssocFailed;
            break;
        case WIFI_REASON_NO_AP_FOUND:
            retCode = eBSPWifiReasonAPNotFound;
            break;
        default:
            retCode = eBSPWifiReasonUnspecified;
            break;
    }
    return retCode;
}

/*
 *******************************************************************************
 *             PRIVATE WIFI EVENT HANDLER
 ******************************************************************************
 */
static void
espWifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    bspWifiHandle_t* handle = (bspWifiHandle_t*)arg;
    if(handle == NULL || handle->evt_q == NULL)
        return;

    espWifiPlatformCtx_t* wifiContext = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
                WIFI_LOGI("WIFI_EVENT_STA_START");
                handle->wifi_started = 1;
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_STOPPED);
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_STARTED);
                break;

            case WIFI_EVENT_STA_STOP:
                WIFI_LOGI("WIFI_EVENT_STA_STOP");
                handle->wifi_started = 0;
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_STOPPED);
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
            {
                wifi_event_sta_disconnected_t* disconn =
                (wifi_event_sta_disconnected_t*)event_data;

                WIFI_LOGW("STA disconnected, reason=%d", disconn->reason);

                handle->sta_connected = 0;
                handle->auth_failed   = 0;
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_STARTED);
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);


                switch(disconn->reason)
                {
                    case WIFI_REASON_AUTH_EXPIRE:
                    case WIFI_REASON_ASSOC_EXPIRE:
                    case WIFI_REASON_AUTH_FAIL:
                    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    case WIFI_REASON_BEACON_TIMEOUT:
                    case WIFI_REASON_HANDSHAKE_TIMEOUT:
                        WIFI_LOGI("WIFI_EVENT_AUTHICATION ERROR");
                        handle->auth_failed = 1;
                        break;
                    case WIFI_REASON_NO_AP_FOUND:
                        WIFI_LOGI("WIFI_EVENT_NO_AP_FOUND");
                        handle->auth_failed = 1;
                        break;

                    default:
                        handle->auth_failed = 1;
                        break;
                }
                break;
            }

            case WIFI_EVENT_SCAN_DONE:
                WIFI_LOGI("WIFI_EVENT_SCAN_DONE");
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_STARTED);
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_SCAN_DONE);
                break;

            case WIFI_EVENT_AP_START:
                WIFI_LOGI("WIFI_EVENT_AP_START");
                handle->ap_started = 1;
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_AP_STOPPED);
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_AP_STARTED);
                break;

            case WIFI_EVENT_AP_STOP:
                WIFI_LOGI("WIFI_EVENT_AP_STOP");
                handle->ap_started = 0;
                osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_AP_STARTED);
                osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_AP_STOPPED);
                break;

            default:
                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t* ip = (ip_event_got_ip_t*)event_data;

            WIFI_LOGI("IP_EVENT_STA_GOT_IP");

            handle->sta_connected = 1;
            osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
            osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_CONNECTED);
            osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_IP_READY);
        }
        else
        {
            osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_CONNECTED);
            osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_IP_READY);
            osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
        }
    }
    else
    {
        osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_STA_CONNECTED);
        osEventFlagsClear(wifiContext->evt_flags, BSP_WIFI_F_IP_READY);
        osEventFlagsSet(wifiContext->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
    }
}
/*
 ***********************************************************************
 *    BSP PLATFORM DEPENDENT API
 * ********************************************************************
 */

static bsp_err_sts_t bsp_platform_wifi_init(bspWifiHandle_t* handle)
{
    if(handle == NULL)
        return BSP_ERR_STS_INVALID_PARAM;

    esp_err_t espStatus;
    espWifiPlatformCtx_t* ctx = &g_espWifiCtx;

    if(ctx->initialized)
    {
        return BSP_ERR_STS_ALREADY_INIT;
    }

    handle->platform_ctx = ctx;

    /* -------------------------------------------------
     * NVS init
     * ------------------------------------------------- */
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    if(ret != ESP_OK)
    {
        WIFI_LOGE("NVS init failed: %d", ret);
        return BSP_ERR_STS_FAIL;
    }

    /* -------------------------------------------------
     * Event loop + netif (one-time)
     * ------------------------------------------------- */
    if(!s_eventLoopInited)
    {
        espStatus = esp_netif_init();
        if(espStatus != ESP_OK)
        {
            WIFI_LOGE("esp_netif_init failed");
            return BSP_ERR_STS_FAIL;
        }

        espStatus = esp_event_loop_create_default();
        if(espStatus != ESP_OK)
        {
            WIFI_LOGE("event loop create failed");
            return BSP_ERR_STS_FAIL;
        }

        ctx->sta_netif = esp_netif_create_default_wifi_sta();
        ctx->ap_netif  = esp_netif_create_default_wifi_ap();

        if(!ctx->sta_netif || !ctx->ap_netif)
        {
            WIFI_LOGE("netif create failed");
            return BSP_ERR_STS_FAIL;
        }

        s_eventLoopInited = true;
    }

    /* -------------------------------------------------
     * Wi-Fi driver init
     * ------------------------------------------------- */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    espStatus = esp_wifi_init(&cfg);
    if(espStatus != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_init failed: %d", espStatus);
        return BSP_ERR_STS_FAIL;
    }

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    /* -------------------------------------------------
     * Event flags (platform internal)
     * ------------------------------------------------- */
    ctx->evt_flags = osEventFlagsNew(NULL);
    if(ctx->evt_flags == NULL)
    {
        WIFI_LOGE("event flags create failed");
        return BSP_ERR_STS_FAIL;
    }

    /* -------------------------------------------------
     * Register ESP → BSP queue bridge
     * ------------------------------------------------- */
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, espWifiEventHandler,
                                        handle, &ctx->wifi_any_id);

    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        espWifiEventHandler, handle, &ctx->ip_got_ip);

    ctx->initialized     = 1;
    handle->platform_ctx = ctx;

    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_set_mode(bspWifiHandle_t* handle, bspWifiMode_t mode)
{
    if(!handle || !handle->platform_ctx)
        return BSP_ERR_STS_INVALID_PARAM;

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    wifi_mode_t esp_mode;

    switch(mode)
    {
        case eWiFiModeStation:
            esp_mode = WIFI_MODE_STA;
            break;

        case eWiFiModeAP:
            esp_mode = WIFI_MODE_AP;
            break;

        case eWiFiModeAPStation:
            esp_mode = WIFI_MODE_APSTA;
            break;

        default:
            return BSP_ERR_STS_UNSUPPORTED;
    }

    esp_err_t err = esp_wifi_set_mode(esp_mode);
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_set_mode failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    ctx->current_mode = mode;
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_start_sta(bspWifiHandle_t* handle,
                                                 const bspWifiStaConfig_t* params)
{
    if(!handle || !params)
        return BSP_ERR_STS_INVALID_PARAM;

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    wifi_config_t cfg         = { 0 };

    memcpy(cfg.sta.ssid, params->ssid, params->ssidLength);
    cfg.sta.ssid[params->ssidLength] = '\0';

    if(params->security != eWiFiSecurityOpen)
    {
        memcpy(cfg.sta.password, params->password, params->passwordLength);
        cfg.sta.password[params->passwordLength] = '\0';
    }

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_set_config(STA) failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_start();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_start failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_connect();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_connect failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    /* Wait for IP or disconnect event via queue */
    uint32_t flags = WIFI_EVENT_WAIT_ANY(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED |
                                                         BSP_WIFI_F_STA_DISCONNECTED |
                                                         BSP_WIFI_F_AUTH_FAILED);

    if(flags & BSP_WIFI_F_STA_DISCONNECTED)
    {
        WIFI_LOGE("STA disconnected");
        return BSP_ERR_STS_CONN_FAILED;
    }
    else if(flags & BSP_WIFI_F_AUTH_FAILED)
    {
        WIFI_LOGE("STA auth failed");
        return BSP_ERR_STS_AUTH_FAIL;
    }
    else if(flags & BSP_WIFI_F_STA_CONNECTED)
    {
        WIFI_LOGI("STA connected");
    }
    else
    {
        WIFI_LOGE("Unknown event");
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_start_ap(bspWifiHandle_t* handle,
                                                const bspWifiApConfig_t* config)
{
    if(!handle || !config)
        return BSP_ERR_STS_INVALID_PARAM;

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    wifi_config_t ap          = { 0 };

    memcpy(ap.ap.ssid, config->ssid, strlen((char*)config->ssid));
    ap.ap.ssid_len       = strlen((char*)config->ssid);
    ap.ap.channel        = config->channel;
    ap.ap.max_connection = config->maxConnections;
    ap.ap.authmode       = WIFI_AUTH_OPEN;

    if(config->security != eWiFiSecurityOpen)
    {
        memcpy(ap.ap.password, config->password, strlen((char*)config->password));
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_set_config(AP) failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_start();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_start(AP) failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    WIFI_EVENT_WAIT(ctx->evt_flags, BSP_WIFI_F_AP_STARTED);

    return BSP_ERR_STS_OK;
}
/* ********************************************************************** */
bsp_err_sts_t bsp_platform_wifi_stop_ap(bspWifiHandle_t* handle)
{
    if(!handle || !handle->platform_ctx)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(!ctx->initialized)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    /* Only stop if AP is currently active */
    if(ctx->current_mode != eWiFiModeAP && ctx->current_mode != eWiFiModeAPStation)
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    esp_err_t err = esp_wifi_stop();
    if(err != ESP_OK && err != ESP_ERR_WIFI_NOT_STARTED)
    {
        WIFI_LOGE("esp_wifi_stop failed: %d", err);
        return BSP_ERR_STS_FAIL;
    }

    WIFI_EVENT_WAIT(ctx->evt_flags, BSP_WIFI_F_AP_STOPPED);

    /* Mode transition handled by BSP after event */
    return BSP_ERR_STS_OK;
}
/* ********************************************************************** */

static bsp_err_sts_t bsp_platform_wifi_disconnect_sta(bspWifiHandle_t* handle)
{
    CHECK_PARAM(handle);
    CHECK_PARAM(handle->platform_ctx);

    espWifiPlatformCtx_t* plat = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(plat->initialized == 0U)
    {
        WIFI_LOGE("WiFi platform not initialized");
        return BSP_ERR_STS_INVALID_STATE;
    }

    /* If already not connected, treat as success */
    if(handle->sta_connected == 0U)
    {
        return BSP_ERR_STS_OK;
    }

    esp_err_t err = esp_wifi_disconnect();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_disconnect failed: %d", err);
        return BSP_ERR_STS_FAIL;
    }

    WIFI_EVENT_WAIT(plat->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);

    return BSP_ERR_STS_OK;
}
/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_get_conn_stations(bspWifiHandle_t* handle,
                                                         bspWifiStationInfo_t* stations,
                                                         uint8_t* numStations,
                                                         uint8_t maxStations)
{
    if(!handle || !handle->platform_ctx || !stations || !numStations || maxStations == 0)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    /* AP must be active */
    if(ctx->current_mode != eWiFiModeAP && ctx->current_mode != eWiFiModeAPStation)
    {
        *numStations = 0;
        return BSP_ERR_STS_NOT_READY;
    }

    wifi_sta_list_t sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta_list);
    if(err != ESP_OK)
    {
        *numStations = 0;
        return BSP_ERR_STS_FAIL;
    }

    uint8_t available = sta_list.num;
    uint8_t to_copy   = (available > maxStations) ? maxStations : available;

    for(uint8_t i = 0; i < to_copy; i++)
    {
        memcpy(stations[i].mac, sta_list.sta[i].mac, BSP_WIFI_MAC_ADDR_MAX_LEN);

        /* RSSI */
        stations[i].rssi = sta_list.sta[i].rssi;

        /* PHY mode (collapsed) */
        stations[i].phyMode = esp_map_phy_mode(&sta_list.sta[i]);
    }

    *numStations = to_copy;

    /* Signal buffer-too-small condition */
    if(available > maxStations)
    {
        return BSP_ERR_STS_BUF_TOO_SMALL;
    }

    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t
bsp_platform_wifi_disconn_station(bspWifiHandle_t* handle,
                                  const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN])
{
    if(!handle || !handle->platform_ctx || !mac)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* pctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    /* Only valid when AP is active */
    if(pctx->initialized == 0 ||
       (pctx->current_mode != eWiFiModeAP && pctx->current_mode != eWiFiModeAPStation))
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    bspWifiStationInfo_t stationList[BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS];
    uint8_t numStations = 0;
    bsp_err_sts_t err_sts =
    bsp_platform_wifi_get_conn_stations(handle, stationList, &numStations,
                                        BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS);


    if(err_sts != BSP_ERR_STS_OK || numStations == 0)
    {
        return err_sts;
    }

    for(uint8_t aid = 0; aid < numStations; aid++)
    {
        if(WIFI_MAC_MATCH(stationList[aid].mac, mac))
        {
            /* ESP-IDF expects reason code */
            esp_err_t err = esp_wifi_deauth_sta(aid + 1);
            if(err != ESP_OK)
            {
                WIFI_LOGE("esp_wifi_deauth_sta failed (%d)", err);
                return BSP_ERR_STS_FAIL;
            }
            return BSP_ERR_STS_OK;
        }
    }

    return BSP_ERR_STS_NOT_FOUND;
}

/* ********************************************************************** */

static bsp_err_sts_t bsp_platform_wifi_get_ap_config(bspWifiHandle_t* handle,
                                                     bspWifiApConfig_t* apCfg)
{
    espWifiPlatformCtx_t* pctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    wifi_config_t cfg          = { 0 };

    if(!pctx || !apCfg)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &cfg);
    if(err != ESP_OK)
    {
        return BSP_ERR_STS_FAIL;
    }

    memset(apCfg, 0, sizeof(*apCfg));

    strcpy((char*)&apCfg->password, (const char*)&cfg.ap.password);
    apCfg->passwordLength = strlen((const char*)&cfg.ap.password);

    if(CHECK_VALID_WIFI_SSID_LEN(cfg.ap.ssid_len))
    {
        memcpy(apCfg->ssid, cfg.ap.ssid, cfg.ap.ssid_len);
        apCfg->ssid[cfg.ap.ssid_len] = '\0';
        apCfg->ssidLength            = cfg.ap.ssid_len;
    }
    apCfg->channel          = cfg.ap.channel;
    apCfg->ssidHidden       = cfg.ap.ssid_hidden;
    apCfg->maxConnections   = cfg.ap.max_connection;
    apCfg->maxIdlePeriodSec = cfg.ap.bss_max_idle_cfg.period;
    apCfg->security         = esp_authmode_to_bsp(cfg.ap.authmode);
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_get_ap_statistics(bspWifiHandle_t* handle,
                                                         bspWifiStatistics_t* stats)
{
    if(!handle || !stats)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    memset(stats, 0, sizeof(*stats));

    /* ESP-IDF limitation:
     * No public API for AP TX/RX counters.
     * We can only provide RSSI/noise per station if queried separately.
     */

    return BSP_ERR_STS_UNSUPPORTED;
}


/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_store_sta_profile(bspWifiHandle_t* handle,
                                                         const bspWifiStaConfig_t* station)
{
    if(!handle || !station)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count = 0;
    size_t size;
    char key[16];

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if(err != ESP_OK)
    {
        WIFI_LOGE("NVS open failed (%d)", err);
        return BSP_ERR_STS_FAIL;
    }

    /* Read current count */
    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if(err == ESP_ERR_NVS_NOT_FOUND)
    {
        count = 0;
    }
    else if(err != ESP_OK)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_FAIL;
    }

    /* Update existing profile if SSID matches */
    for(uint8_t i = 0; i < count; i++)
    {
        bspWifiStaConfig_t stored;
        size = sizeof(stored);

        snprintf(key, sizeof(key), "sta_%u", i);

        if(nvs_get_blob(nvs, key, &stored, &size) == ESP_OK)
        {
            if(stored.ssidLength == station->ssidLength &&
               memcmp(stored.ssid, station->ssid, station->ssidLength) == 0)
            {
                err = nvs_set_blob(nvs, key, station, sizeof(*station));
                if(err == ESP_OK)
                {
                    nvs_commit(nvs);
                }

                nvs_close(nvs);
                return (err == ESP_OK) ? BSP_ERR_STS_OK : BSP_ERR_STS_FAIL;
            }
        }
    }

    /* Add new profile */
    if(count >= BSP_WIFI_MAX_SAVED_STA)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NO_MEM;
    }

    snprintf(key, sizeof(key), "sta_%u", count);

    err = nvs_set_blob(nvs, key, station, sizeof(*station));
    if(err != ESP_OK)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_FAIL;
    }

    nvs_set_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, count + 1);
    nvs_commit(nvs);
    nvs_close(nvs);

    WIFI_LOGI("Stored STA profile (ssid=%.*s)", station->ssidLength, station->ssid);

    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */

static bsp_err_sts_t bsp_platform_wifi_remove_sta_profile(bspWifiHandle_t* handle,
                                                          const uint8_t* ssid,
                                                          uint8_t ssid_len)
{
    if(!handle || !ssid || ssid_len == 0 || ssid_len > BSP_WIFI_SSID_MAX_LEN)
        return BSP_ERR_STS_INVALID_PARAM;

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count        = 0;
    bool found           = false;
    uint8_t remove_index = 0;

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if(err != ESP_OK)
        return BSP_ERR_STS_FAIL;

    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if(err != ESP_OK || count == 0)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NOT_FOUND;
    }

    /* -------------------------------------------------
     * Find matching SSID
     * ------------------------------------------------- */
    for(uint8_t i = 0; i < count; i++)
    {
        bspWifiStaConfig_t cfg;
        size_t size = sizeof(cfg);
        char key[16];

        snprintf(key, sizeof(key), "sta_%u", i);

        err = nvs_get_blob(nvs, key, &cfg, &size);
        if(err != ESP_OK)
            continue;

        if(cfg.ssidLength == ssid_len && memcmp(cfg.ssid, ssid, ssid_len) == 0)
        {
            found        = true;
            remove_index = i;
            break;
        }
    }

    if(!found)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NOT_FOUND;
    }

    /* -------------------------------------------------
     * Shift remaining profiles left
     * ------------------------------------------------- */
    for(uint8_t i = remove_index; i < (count - 1); i++)
    {
        bspWifiStaConfig_t next;
        size_t size = sizeof(next);
        char key_src[16];
        char key_dst[16];

        snprintf(key_src, sizeof(key_src), "sta_%u", i + 1);
        snprintf(key_dst, sizeof(key_dst), "sta_%u", i);

        err = nvs_get_blob(nvs, key_src, &next, &size);
        if(err == ESP_OK)
        {
            nvs_set_blob(nvs, key_dst, &next, sizeof(next));
        }
    }

    /* -------------------------------------------------
     * Erase last entry
     * ------------------------------------------------- */
    char last_key[16];
    snprintf(last_key, sizeof(last_key), "sta_%u", count - 1);
    nvs_erase_key(nvs, last_key);

    /* -------------------------------------------------
     * Update count
     * ------------------------------------------------- */
    count--;
    nvs_set_u8(nvs, "sta_count", count);
    nvs_commit(nvs);

    nvs_close(nvs);
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
static bsp_err_sts_t bsp_platform_wifi_get_sta_profile(bspWifiHandle_t* handle,
                                                       uint8_t index,
                                                       bspWifiStaConfig_t* sta_cfg)
{
    if(!handle || !sta_cfg)
        return BSP_ERR_STS_INVALID_PARAM;

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count = 0;

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if(err != ESP_OK)
        return BSP_ERR_STS_FAIL;

    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if(err != ESP_OK || count == 0)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NOT_FOUND;
    }

    if(index >= count)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_INVALID_PARAM;
    }

    char key[16];
    snprintf(key, sizeof(key), BSP_WIFI_NVS_STA_KEY_FMT, index);

    size_t size = sizeof(bspWifiStaConfig_t);
    err         = nvs_get_blob(nvs, key, sta_cfg, &size);

    nvs_close(nvs);

    if(err != ESP_OK || size != sizeof(bspWifiStaConfig_t))
        return BSP_ERR_STS_FAIL;

    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bsp_platform_wifi_start_scan(bspWifiHandle_t* handle)
{
    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(ctx);

    if(ctx->current_mode != eWiFiModeStation && ctx->current_mode != eWiFiModeAPStation)
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    wifi_scan_config_t scan_cfg = { .ssid = NULL, .bssid = NULL, .channel = 0, .show_hidden = true };

    esp_err_t err = esp_wifi_scan_start(&scan_cfg, false);
    return (err == ESP_OK) ? BSP_ERR_STS_OK : BSP_ERR_STS_FAIL;
}
/* ********************************************************************** */
bsp_err_sts_t bsp_platform_wifi_get_scan_results(bspWifiHandle_t* handle,
                                                 bspWifiApScanResult_t* results,
                                                 uint8_t max_results,
                                                 uint8_t* out_count)
{
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if(ap_count == 0)
    {
        *out_count = 0;
        return BSP_ERR_STS_OK;
    }

    uint16_t fetch = (ap_count > max_results) ? max_results : ap_count;

    wifi_ap_record_t* ap_list = calloc(fetch, sizeof(wifi_ap_record_t));
    if(!ap_list)
    {
        return BSP_ERR_STS_NO_MEM;
    }

    esp_wifi_scan_get_ap_records(&fetch, ap_list);

    for(uint16_t i = 0; i < fetch; i++)
    {
        results[i].ssidLength = strnlen((char*)ap_list[i].ssid, BSP_WIFI_SSID_MAX_LEN);

        memcpy(results[i].ssid, ap_list[i].ssid, results[i].ssidLength);

        memcpy(results[i].bssid, ap_list[i].bssid, BSP_WIFI_MAC_ADDR_MAX_LEN);

        results[i].rssi     = ap_list[i].rssi;
        results[i].channel  = ap_list[i].primary;
        results[i].security = esp_authmode_to_bsp(ap_list[i].authmode);
    }

    *out_count = fetch;
    free(ap_list);

    return BSP_ERR_STS_OK;
}


/*
 ***********************************************************************
 *    BSP API
 * ********************************************************************
 */
bsp_err_sts_t bspWifiInit(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Clear handle */
    memset(handle, 0, sizeof(bspWifiHandle_t));

    /* -------------------------------------------------
     * Create mutex
     * ------------------------------------------------- */
    handle->lock = osMutexNew(NULL);
    if(handle->lock == NULL)
    {
        WIFI_LOGE("Failed to create mutex");
        return BSP_ERR_STS_NO_MEM;
    }

    /* -------------------------------------------------
     * Create Wi-Fi event queue
     * ------------------------------------------------- */
    handle->evt_q =
    osMessageQueueNew(BSP_WIFI_EVENT_QUEUE_LEN, sizeof(bspWifiContext_t), NULL);
    if(handle->evt_q == NULL)
    {
        WIFI_LOGE("Failed to create event queue");
        osMutexDelete(handle->lock);
        handle->lock = NULL;
        return BSP_ERR_STS_NO_MEM;
    }

    /* -------------------------------------------------
     * Initialize platform Wi-Fi layer
     * ------------------------------------------------- */
    bsp_err_sts_t sts = bsp_platform_wifi_init(handle);
    if(sts != BSP_ERR_STS_OK || handle->platform_ctx == NULL)
    {
        WIFI_LOGE("Failed to initialize platform Wi-Fi layer %s", bsp_err_sts_to_str(sts));
        osMutexDelete(handle->lock);
        handle->lock = NULL;

        osMessageQueueDelete(handle->evt_q);
        handle->evt_q = NULL;

        return sts;
    }

    /* -------------------------------------------------
     * Initial runtime state
     * ------------------------------------------------- */
    handle->wifi_started     = 0U;
    handle->sta_connected    = 0U;
    handle->ap_started       = 0U;
    handle->auth_failed      = 0U;
    handle->scan_in_progress = 0U;

    /* Context is empty at init */
    memset(&handle->wifi_context, 0, sizeof(bspWifiContext_t));

    return BSP_ERR_STS_OK;
}


/* ********************************************************************** */
bsp_err_sts_t bspWifiDeInit(bspWifiHandle_t* handle)
{
    if(handle == NULL || handle->platform_ctx == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    esp_err_t espStatus;

    /* -------------------------------------------------
     * Stop Wi-Fi if running
     * ------------------------------------------------- */
    if(handle->wifi_started)
    {
        espStatus = esp_wifi_stop();
        if(espStatus != ESP_OK && espStatus != ESP_ERR_WIFI_NOT_INIT)
        {
            WIFI_LOGW("esp_wifi_stop failed: %d", espStatus);
        }
    }


    WIFI_LOCK(handle);
    /* -------------------------------------------------
     * Unregister ESP event handlers
     * ------------------------------------------------- */
    if(ctx->wifi_any_id)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, ctx->wifi_any_id);
        ctx->wifi_any_id = NULL;
    }

    if(ctx->ip_got_ip)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ctx->ip_got_ip);
        ctx->ip_got_ip = NULL;
    }

    /* -------------------------------------------------
     * Deinit Wi-Fi driver
     * ------------------------------------------------- */
    espStatus = esp_wifi_deinit();
    if(espStatus != ESP_OK && espStatus != ESP_ERR_WIFI_NOT_INIT)
    {
        WIFI_LOGW("esp_wifi_deinit failed: %d", espStatus);
    }

    /* -------------------------------------------------
     * Destroy netifs (only what we created)
     * ------------------------------------------------- */
    if(ctx->sta_netif)
    {
        esp_netif_destroy(ctx->sta_netif);
        ctx->sta_netif = NULL;
    }

    if(ctx->ap_netif)
    {
        esp_netif_destroy(ctx->ap_netif);
        ctx->ap_netif = NULL;
    }

    /* -------------------------------------------------
     * Delete platform event flags
     * ------------------------------------------------- */
    if(ctx->evt_flags)
    {
        osEventFlagsDelete(ctx->evt_flags);
        ctx->evt_flags = NULL;
    }

    /* -------------------------------------------------
     * Reset BSP handle state
     * ------------------------------------------------- */
    s_eventLoopInited        = false;
    handle->wifi_started     = 0;
    handle->sta_connected    = 0;
    handle->ap_started       = 0;
    handle->auth_failed      = 0;
    handle->scan_in_progress = 0;
    handle->platform_ctx     = NULL;

    /* -------------------------------------------------
     * Reset platform ctx (static, not freed)
     * ------------------------------------------------- */
    memset(ctx, 0, sizeof(*ctx));

    WIFI_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiOn(bspWifiHandle_t* handle, bspWifiMode_t mode, void* pvParam)
{
    bsp_err_sts_t sts;

    if(handle == NULL)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    /* -------------------------------------------------
     * Validate mode
     * ------------------------------------------------- */
    if(mode >= eWiFiModeMax)
    {
        WIFI_LOGE("Invalid mode");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* -------------------------------------------------
     * Set Wi-Fi mode at platform level
     * ------------------------------------------------- */
    sts = bsp_platform_wifi_set_mode(handle, mode);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("bsp_platform_wifi_set_mode failed %s", bsp_err_sts_to_str(sts));
        WIFI_UNLOCK(handle);
        return sts;
    }

    /* -------------------------------------------------
     * Start according to mode
     * ------------------------------------------------- */
    switch(mode)
    {
        case eWiFiModeStation:
        {
            if(pvParam == NULL)
            {
                WIFI_LOGE("Invalid param");
                WIFI_UNLOCK(handle);
                return BSP_ERR_STS_INVALID_PARAM;
            }


            sts = bsp_platform_wifi_start_sta(handle, (const bspWifiStaConfig_t*)pvParam);
            break;
        }

        case eWiFiModeAP:
        {
            if(pvParam == NULL)
            {
                WIFI_LOGE("Invalid param");
                WIFI_UNLOCK(handle);
                return BSP_ERR_STS_INVALID_PARAM;
            }

            sts = bsp_platform_wifi_start_ap(handle, (const bspWifiApConfig_t*)pvParam);
            break;
        }

        case eWiFiModeAPStation:
        {
            if(pvParam == NULL)
            {
                WIFI_LOGE("Invalid param");
                WIFI_UNLOCK(handle);
                return BSP_ERR_STS_INVALID_PARAM;
            }

            const bspWifiApStaConfig_t* cfg = (const bspWifiApStaConfig_t*)pvParam;

            sts = bsp_platform_wifi_start_ap(handle, &cfg->apConfig);
            if(sts != BSP_ERR_STS_OK)
            {
                WIFI_LOGE("bsp_platform_wifi_start_ap failed %s", bsp_err_sts_to_str(sts));
                WIFI_UNLOCK(handle);
                return sts;
            }

            sts = bsp_platform_wifi_start_sta(handle, &cfg->staConfig);
            break;
        }

        case eWiFiModeP2P:
            sts = BSP_ERR_STS_UNSUPPORTED;
            break;

        default:
            sts = BSP_ERR_STS_INVALID_PARAM;
            break;
    }

    /* -------------------------------------------------
     * Update BSP state
     * ------------------------------------------------- */
    if(sts == BSP_ERR_STS_OK)
    {
        handle->wifi_started = 1;
    }

    WIFI_UNLOCK(handle);
    return sts;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiOff(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->wifi_started == false)
    {
        WIFI_LOGW("Wi-Fi is not started, nothing to stop");
        return BSP_ERR_STS_OK;
    }

    bsp_err_sts_t status = bspWifiDeInit(handle);
    if(status != ESP_OK)
    {
        WIFI_LOGE("Failed to deinitialize Wi-Fi");
        return status;
    }

    WIFI_LOGI("Wi-Fi successfully stopped and deinitialized");
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiConnectToAp(bspWifiHandle_t* handle)
{
    if(!handle || !handle->platform_ctx)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }
    espWifiPlatformCtx_t* plat = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(plat);

    /* ---------------- BSP critical section ---------------- */
    WIFI_LOCK(handle);

    handle->sta_connected = 0;
    handle->auth_failed   = 0;

    WIFI_EVENT_CLEAR(plat->evt_flags, BSP_WIFI_F_STA_CONNECTED | BSP_WIFI_F_STA_DISCONNECTED |
                                      BSP_WIFI_F_AUTH_FAILED);

    /* Ask platform to start STA connection */
    bsp_err_sts_t ret =
    bsp_platform_wifi_start_sta(handle, &handle->wifi_context.tEventData.tStationCfg);
    if(ret != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Failed to start STA connection %s", bsp_err_sts_to_str(ret));
        WIFI_UNLOCK(handle);
        return ret;
    }

    WIFI_UNLOCK(handle);


    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiDisconnectToAp(bspWifiHandle_t* handle)
{
    CHECK_PARAM(handle);
    CHECK_PARAM(handle->platform_ctx);

    espWifiPlatformCtx_t* plat = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(plat);

    /* If not connected, nothing to do */
    if(handle->sta_connected == 0U)
    {
        WIFI_LOGE("Not connected, nothing to disconnect");
        return BSP_ERR_STS_OK;
    }

    WIFI_LOCK(handle);

    /* Clear stale events */
    WIFI_EVENT_CLEAR(plat->evt_flags, BSP_WIFI_F_STA_DISCONNECTED | BSP_WIFI_F_AUTH_FAILED);

    /* Ask platform to disconnect */
    bsp_err_sts_t sts = bsp_platform_wifi_disconnect_sta(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Failed to disconnect %s", bsp_err_sts_to_str(sts));
        WIFI_UNLOCK(handle);
        return sts;
    }

    /* Wait for disconnect confirmation */
    WIFI_EVENT_WAIT(plat->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);

    /* Update handle state */
    handle->sta_connected = 0U;

    WIFI_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiStartAP(bspWifiHandle_t* handle, const bspWifiApConfig_t* apCfg)
{
    if(!handle || !apCfg)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    espWifiPlatformCtx_t* plat = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(plat);

    if(handle->ap_started)
    {
        WIFI_LOGE("AP already started");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_OK; /* already running */
    }

    WIFI_EVENT_CLEAR(plat->evt_flags, BSP_WIFI_F_AP_STARTED | BSP_WIFI_F_AP_STOPPED);

    bsp_err_sts_t sts = bsp_platform_wifi_start_ap(handle, apCfg);

    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Failed to start AP %s", bsp_err_sts_to_str(sts));
        WIFI_UNLOCK(handle);
        return sts;
    }

    WIFI_EVENT_WAIT(plat->evt_flags, BSP_WIFI_F_AP_STARTED);

    WIFI_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiStopAP(bspWifiHandle_t* handle)
{
    if(!handle)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(!handle->ap_started)
    {
        WIFI_LOGE("AP not started, nothing to stop");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_OK;
    }

    espWifiPlatformCtx_t* plat = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(plat);

    WIFI_EVENT_CLEAR(plat->evt_flags, BSP_WIFI_F_AP_STARTED | BSP_WIFI_F_AP_STOPPED);

    bsp_err_sts_t sts = bsp_platform_wifi_stop_ap(handle);

    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Failed to stop AP %s", bsp_err_sts_to_str(sts));
        WIFI_UNLOCK(handle);
        return sts;
    }

    handle->ap_started = true;

    WIFI_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}
/* ********************************************************************** */
bsp_err_sts_t bspWifiGetConnStations(bspWifiHandle_t* handle,
                                     bspWifiStationInfo_t* stations,
                                     uint8_t* numStations,
                                     uint8_t maxStations)
{
    if(!handle || !stations || !numStations || maxStations == 0)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    /* AP must be running */
    if(handle->ap_started == 0U)
    {
        WIFI_LOGE("AP not started");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_READY;
    }

    /* Delegate to platform layer */
    bsp_err_sts_t sts =
    bsp_platform_wifi_get_conn_stations(handle, stations, numStations, maxStations);

    WIFI_UNLOCK(handle);
    return sts;
}
/* ********************************************************************** */
bsp_err_sts_t bspWifiDisconnectStation(bspWifiHandle_t* handle,
                                       const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN])
{
    if(!handle || !handle->platform_ctx || !mac)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Only valid in AP or AP+STA mode */
    if(handle->ap_started == 0)
    {
        WIFI_LOGE("AP not started");
        return BSP_ERR_STS_INVALID_STATE;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_disconn_station(handle, mac);

    WIFI_UNLOCK(handle);

    return sts;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiGetAPConfig(bspWifiHandle_t* handle, bspWifiApConfig_t* apCfg)
{
    if(!handle || !apCfg)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(!handle->ap_started)
    {
        WIFI_LOGE("AP not started");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_RUNNING;
    }

    bsp_err_sts_t sts = bsp_platform_wifi_get_ap_config(handle, apCfg);

    WIFI_UNLOCK(handle);
    return sts;
}


/* ********************************************************************** */
bsp_err_sts_t bspWifiGetAPStatistics(bspWifiHandle_t* handle, bspWifiStatistics_t* stats)
{
    if(!handle || !stats)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(!handle->ap_started)
    {
        WIFI_LOGE("AP not started");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_RUNNING;
    }

    bsp_err_sts_t sts = bsp_platform_wifi_get_ap_statistics(handle, stats);

    WIFI_UNLOCK(handle);
    return sts;
}
/* ********************************************************************** */
bsp_err_sts_t bspWifiStoreStationProfile(bspWifiHandle_t* handle, bspWifiStaConfig_t* station)
{
    if(!handle || !station)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_store_sta_profile(handle, station);

    WIFI_UNLOCK(handle);
    return sts;
}


/* ********************************************************************** */
bsp_err_sts_t
bspWifiGetStationProfile(bspWifiHandle_t* handle, uint8_t index, bspWifiStaConfig_t* sta_cfg)
{
    if(!handle || !sta_cfg)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(!handle->platform_ctx)
    {
        WIFI_LOGE("Platform not initialized");
        return BSP_ERR_STS_NOT_INIT;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_get_sta_profile(handle, index, sta_cfg);

    WIFI_UNLOCK(handle);
    return sts;
}
/* ********************************************************************** */
bsp_err_sts_t
bspWifiRemoveStationProfile(bspWifiHandle_t* handle, const uint8_t* ssid, uint8_t ssid_len)
{
    if(!handle || !ssid || ssid_len == 0)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(!handle->platform_ctx)
    {
        WIFI_LOGE("Platform not initialized");
        return BSP_ERR_STS_NOT_INIT;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_remove_sta_profile(handle, ssid, ssid_len);

    WIFI_UNLOCK(handle);
    return sts;
}
/* ********************************************************************** */

bsp_err_sts_t bspWifiStartScan(bspWifiHandle_t* handle, bspWifiApScanResult_t* results, uint8_t max_results)
{
    if(!handle || !results || max_results == 0)
    {
        WIFI_LOGE("Invalid handle");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(!handle->wifi_started)
    {
        WIFI_LOGE("Wi-Fi not started");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_STATE;
    }

    if(handle->scan_in_progress)
    {
        WIFI_LOGE("Scan already in progress");
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_BUSY;
    }

    espWifiPlatformCtx_t* plat_ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    CHECK_POINTER(plat_ctx);

    handle->scan_in_progress = 1;

    /* Start scan (platform) */
    bsp_err_sts_t sts = bsp_platform_wifi_start_scan(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Failed to start scan %s", bsp_err_sts_to_str(sts));
        handle->scan_in_progress = 0;
        WIFI_UNLOCK(handle);
        return sts;
    }


    WIFI_EVENT_WAIT(plat_ctx->evt_flags, BSP_WIFI_F_SCAN_DONE);


    /* Fetch results from platform */
    uint8_t out_count = 0;
    sts = bsp_platform_wifi_get_scan_results(handle, results, max_results, &out_count);

    handle->scan_in_progress = 0;
    WIFI_UNLOCK(handle);
    return sts;
}

/* ********************************************************************** */
bsp_err_sts_t bspWifiReset(bspWifiHandle_t* handle)
{
    WIFI_LOGE("Not supported");
    return BSP_ERR_STS_UNSUPPORTED;
}
/* ********************************************************************** */
