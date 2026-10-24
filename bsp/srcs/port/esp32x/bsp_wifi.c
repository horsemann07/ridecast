/* ============================================================================
 * File        : bsp_wifi.c
 * Description : ESP-IDF 5.5 implementation of the portable BSP Wi-Fi API
 * Author      : Jha Raghav
 * Date        : 2026-10-24
 * ============================================================================
 * Copyright (c) 2026 Navyantra Tech Private Limited
 *
 * Description:
 *   Manages one ESP32 Wi-Fi instance using CMSIS-RTOS2 synchronization and
 *   ESP event handling. Supports station, access-point, combined mode,
 *   scanning, and application-provided profile callbacks.
 *
 * Integration:
 *   Link with esp_wifi, esp_netif, esp_event, and cmsis_esp.
 *   Public types are declared in bsp_wifi.h.
 *
 * Requirements:
 *   - ESP-IDF 5.5, CMSIS-RTOS2 adapter, and BSP error-status definitions.
 *   - Initialize the BSP after the CMSIS kernel is running.
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-24    Jha Raghav    Optional persistence callbacks and dual logger support
 * 2026-10-24    Jha Raghav    Unified statuses, API, events and NVS handling
 * 2026-10-23    Jha Raghav    Initial ESP Wi-Fi port (original supplied code)
 * ============================================================================ */

#include "bsp_wifi.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os2.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"

#if defined(configDEFAULT_LOGGER_ENABLED) && defined(ESP_LOGGER_ENABLED) && \
(configDEFAULT_LOGGER_ENABLED == ESP_LOGGER_ENABLED)
    #include "esp_log.h"
static const char s_wifi_log_tag[] = "bsp_wifi";
    #define WIFI_LOGE(...) ESP_LOGE(s_wifi_log_tag, __VA_ARGS__)
    #define WIFI_LOGW(...) ESP_LOGW(s_wifi_log_tag, __VA_ARGS__)
    #define WIFI_LOGI(...) ESP_LOGI(s_wifi_log_tag, __VA_ARGS__)
    #define WIFI_LOGD(...) ESP_LOGD(s_wifi_log_tag, __VA_ARGS__)
#elif defined(configDEFAULT_LOGGER_ENABLED) && defined(BSP_LOGGER_ENABLED) && \
(configDEFAULT_LOGGER_ENABLED == BSP_LOGGER_ENABLED)
    #include "bsp_log.h"
    #define WIFI_LOGE(...) LOGE(__VA_ARGS__)
    #define WIFI_LOGW(...) LOGW(__VA_ARGS__)
    #define WIFI_LOGI(...) LOGI(__VA_ARGS__)
    #define WIFI_LOGD(...) LOGD(__VA_ARGS__)
#else
    #define WIFI_LOGE(...) ((void)0)
    #define WIFI_LOGW(...) ((void)0)
    #define WIFI_LOGI(...) ((void)0)
    #define WIFI_LOGD(...) ((void)0)
#endif

/* Port-owned configuration; these options are not exposed in public headers. */
#ifndef configWIFI_EVENT_QUEUE_DEPTH
    #define configWIFI_EVENT_QUEUE_DEPTH (10U)
#endif
#ifndef configWIFI_WAIT_TIMEOUT_MS
    #define configWIFI_WAIT_TIMEOUT_MS (5000U)
#endif

#if (configWIFI_EVENT_QUEUE_DEPTH < 1U)
    #error "configWIFI_EVENT_QUEUE_DEPTH must be at least 1"
#endif

#define WIFI_FLAG_STA_IP     (1UL << 0)
#define WIFI_FLAG_STA_LOST   (1UL << 1)
#define WIFI_FLAG_AP_READY   (1UL << 2)
#define WIFI_FLAG_SCAN_DONE  (1UL << 3)
#define WIFI_EVENT_WAIT_MASK (WIFI_FLAG_STA_IP | WIFI_FLAG_STA_LOST)

/**
 * @brief Holds the ESP32 Wi-Fi instance state and owned resources.
 *
 * @note The ESP32 port supports one active instance. Access to mutable
 *       state is coordinated through the mutex or atomic fields as used
 *       by the implementation.
 */
typedef struct
{
    bspWifiHandle_t* owner; /**< Handle associated with this instance. */
    osMutexId_t mutex;      /**< Mutex protecting Wi-Fi operations. */
    osEventFlagsId_t flags; /**< Flags used to signal Wi-Fi state changes. */
    osMessageQueueId_t event_queue; /**< Queue of application-facing events. */
    esp_netif_t* sta_netif;         /**< ESP-IDF station network interface. */
    esp_netif_t* ap_netif; /**< ESP-IDF access-point network interface. */
    esp_event_handler_instance_t wifi_handler; /**< Registered Wi-Fi event handler. */
    esp_event_handler_instance_t ip_handler; /**< Registered IP event handler. */
    bool owns_event_loop; /**< Whether this instance created the default event loop. */
    bool driver_initialized; /**< Whether the ESP-IDF Wi-Fi driver is initialized. */
    bool started;            /**< Whether the Wi-Fi driver is started. */
    bool have_sta_config; /**< Whether station configuration is available. */
    bool scanning;        /**< Whether a scan is in progress. */
    bspWifiMode_t mode;   /**< Current BSP Wi-Fi mode. */
    bspWifiStaConfig_t sta_config; /**< Current station configuration. */
    bspWifiProfileCallbacks_t profile_callbacks; /**< Optional application-owned profile callbacks. */
    _Atomic bool station_ip_ready; /**< Whether the station has acquired an IP address. */
    _Atomic bool station_link_connected; /**< Whether the station link is connected. */
    _Atomic bool ap_ready; /**< Whether the access point is ready. */
    _Atomic bool auth_failed; /**< Whether the latest connection attempt failed authentication. */
    _Atomic int last_disconnect_reason; /**< Most recently recorded disconnect reason. */
} wifi_esp_context_t;

/* The ESP32 Wi-Fi driver is a singleton. The application must serialize
 * initialization and deinitialization of handles. */
static wifi_esp_context_t s_wifi;

/* -------------------------------------------------------------------------- */
/* Shared error-status mapping and internal helpers                           */
/* -------------------------------------------------------------------------- */

static ret_err_sts_t wifi_result(esp_err_t error)
{
    if(error == ESP_OK)
    {
        return RET_ERR_STS_OK;
    }

    /* Preserve the ESP-IDF error in the log before mapping it to a BSP status. */
    WIFI_LOGE("ESP-IDF Wi-Fi error: %s (0x%lX)", esp_err_to_name(error),
              (unsigned long)(uint32_t)error);
    if(error == ESP_ERR_INVALID_ARG)
    {
        return RET_ERR_STS_INVALID_PARAM;
    }
    if(error == ESP_ERR_INVALID_STATE)
    {
        return RET_ERR_STS_INVALID_STATE;
    }
    if(error == ESP_ERR_NO_MEM)
    {
        return RET_ERR_STS_NO_MEM;
    }
    if(error == ESP_ERR_TIMEOUT)
    {
        return RET_ERR_STS_TIMEOUT;
    }
    if(error == ESP_ERR_WIFI_NOT_INIT)
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }
    if(error == ESP_ERR_WIFI_NOT_STARTED)
    {
        return RET_ERR_STS_NOT_READY;
    }
    if(error == ESP_ERR_WIFI_TIMEOUT)
    {
        return RET_ERR_STS_TIMEOUT;
    }
    if(error == ESP_ERR_WIFI_STATE)
    {
        return RET_ERR_STS_BUSY;
    }
    return RET_ERR_STS_FAIL;
}

/* Log errors at warning or error level without defining Wi-Fi-specific
 * status codes. */
static ret_err_sts_t wifi_report(const char* operation, ret_err_sts_t status)
{
    /* Keep the parameter used when logger macros compile out. */
    (void)operation;
    if(status == RET_ERR_STS_OK)
    {
        return status;
    }
    switch(status)
    {
        case RET_ERR_STS_NO_DATA:
        case RET_ERR_STS_NOT_FOUND:
        case RET_ERR_STS_NOT_SUPPORTED:
        case RET_ERR_STS_INVALID_PARAM:
        case RET_ERR_STS_INVALID_SIZE:
        case RET_ERR_STS_BUSY:
        case RET_ERR_STS_TIMEOUT:
        case RET_ERR_STS_ALREADY_CONNECTED:
        case RET_ERR_STS_ALREADY_EXISTS:
        case RET_ERR_STS_AUTH_FAILED:
        case RET_ERR_STS_CONNECTION_FAILED:
        case RET_ERR_STS_NOT_READY:
        {
            WIFI_LOGW("%s: status=%d", operation, (int)status);
            break;
        }
        default:
        {
            WIFI_LOGE("%s: status=%d", operation, (int)status);
            break;
        }
    }
    return status;
}

static uint32_t wifi_ms_to_ticks(uint32_t ms)
{
    uint32_t hz = osKernelGetTickFreq();
    uint64_t ticks;

    /* Preserve CMSIS infinite-wait and poll semantics. */
    if(ms == UINT32_MAX)
    {
        return osWaitForever;
    }
    if(ms == 0U)
    {
        return 0U;
    }
    if(hz == 0U)
    {
        return 1U;
    }

    /* Round fractional tick durations up so nonzero waits do not expire early. */
    ticks = (((uint64_t)ms * (uint64_t)hz) + UINT64_C(999)) / UINT64_C(1000);
    if(ticks == 0U)
    {
        ticks = 1U;
    }
    if(ticks >= UINT32_MAX)
    {
        ticks = UINT32_MAX - 1U;
    }
    return (uint32_t)ticks;
}

static ret_err_sts_t wifi_context(bspWifiHandle_t* handle, wifi_esp_context_t** out)
{
    RET_CHECK_POINTER(handle, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_POINTER(out, RET_ERR_STS_INVALID_PARAM);

    /* Verify that the handle still owns the singleton context. */
    if((s_wifi.owner != handle) || (handle->platform_ctx != &s_wifi))
    {
        return RET_ERR_STS_NOT_INITIALIZED;
    }
    *out = &s_wifi;
    return RET_ERR_STS_OK;
}

static ret_err_sts_t wifi_lock(wifi_esp_context_t* ctx)
{
    if(osMutexAcquire(ctx->mutex, wifi_ms_to_ticks(configWIFI_WAIT_TIMEOUT_MS)) != osOK)
    {
        WIFI_LOGW("Wi-Fi mutex acquisition failed or timed out");
        return RET_ERR_STS_TIMEOUT;
    }
    return RET_ERR_STS_OK;
}

static void wifi_unlock(wifi_esp_context_t* ctx)
{
    (void)osMutexRelease(ctx->mutex);
}

static ret_err_sts_t wifi_wait_flag(wifi_esp_context_t* ctx, uint32_t mask)
{
    uint32_t flags = osEventFlagsWait(ctx->flags, mask, osFlagsWaitAny,
                                      wifi_ms_to_ticks(configWIFI_WAIT_TIMEOUT_MS));
    if((flags & osFlagsError) != 0U)
    {
        WIFI_LOGW("Wi-Fi event wait failed: flags=0x%lX", (unsigned long)flags);
        return RET_ERR_STS_TIMEOUT;
    }
    return RET_ERR_STS_OK;
}

static size_t wifi_bounded_length(const uint8_t* buffer, size_t limit)
{
    size_t length = 0U;

    /* Stop at either the first NUL byte or the supplied buffer limit. */
    while((length < limit) && (buffer[length] != 0U))
    {
        ++length;
    }
    return length;
}

static bool wifi_valid_ssid(uint8_t length)
{
    return ((length > 0U) && (length <= 32U) && (length <= BSP_WIFI_SSID_MAX_LEN));
}

static bool wifi_hex_psk(const uint8_t* password)
{
    size_t i;

    /* A 64-character WPA pre-shared key must contain only hexadecimal digits. */
    for(i = 0U; i < 64U; ++i)
    {
        uint8_t c = password[i];
        if(!(((c >= (uint8_t)'0') && (c <= (uint8_t)'9')) ||
             ((c >= (uint8_t)'a') && (c <= (uint8_t)'f')) ||
             ((c >= (uint8_t)'A') && (c <= (uint8_t)'F'))))
        {
            return false;
        }
    }
    return true;
}

static ret_err_sts_t wifi_validate_station(const bspWifiStaConfig_t* station)
{
    RET_CHECK_POINTER(station, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM(wifi_valid_ssid(station->ssidLength), RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM(station->channel <= 14U, RET_ERR_STS_INVALID_PARAM);

    /* An open network must not provide a password. */
    if(station->security == eWiFiSecurityOpen)
    {
        return (station->passwordLength == 0U) ? RET_ERR_STS_OK : RET_ERR_STS_INVALID_PARAM;
    }
    if((station->security != eWiFiSecurityWPA) && (station->security != eWiFiSecurityWPA2) &&
       (station->security != eWiFiSecurityWPA3))
    {
        return RET_ERR_STS_NOT_SUPPORTED;
    }
    if((station->passwordLength < 8U) || (station->passwordLength > 64U) ||
       (station->passwordLength > BSP_WIFI_PASSWORD_MAX_LEN))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }
    if((station->passwordLength == 64U) && !wifi_hex_psk(station->password))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }
    return RET_ERR_STS_OK;
}

static ret_err_sts_t wifi_validate_ap(const bspWifiApConfig_t* ap)
{
    RET_CHECK_POINTER(ap, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM(wifi_valid_ssid(ap->ssidLength), RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM((ap->channel >= 1U) && (ap->channel <= 14U), RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM((ap->maxConnections >= 1U) && (ap->maxConnections <= 10U),
                    RET_ERR_STS_INVALID_PARAM);

    /* Open access points do not require a password. */
    if(ap->security == eWiFiSecurityOpen)
    {
        return RET_ERR_STS_OK;
    }
    if((ap->security != eWiFiSecurityWPA2) && (ap->security != eWiFiSecurityWPA3))
    {
        return RET_ERR_STS_NOT_SUPPORTED;
    }
    if((ap->passwordLength < 8U) || (ap->passwordLength > 63U))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }
    return RET_ERR_STS_OK;
}

static bspWifiSecurity_t wifi_auth_to_bsp(wifi_auth_mode_t auth)
{
    switch(auth)
    {
        case WIFI_AUTH_OPEN:
        {
            return eWiFiSecurityOpen;
        }
        case WIFI_AUTH_WEP:
        {
            return eWiFiSecurityWEP;
        }
        case WIFI_AUTH_WPA_PSK:
        {
            return eWiFiSecurityWPA;
        }
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
        {
            return eWiFiSecurityWPA2;
        }
        case WIFI_AUTH_WPA2_ENTERPRISE:
        {
            return eWiFiSecurityWPA2_ent;
        }
        case WIFI_AUTH_WPA3_PSK:
        case WIFI_AUTH_WPA2_WPA3_PSK:
        {
            return eWiFiSecurityWPA3;
        }
        default:
        {
            return eWiFiSecurityNotSupported;
        }
    }
}

static bspWifiPhyMode_t wifi_phy_to_bsp(const wifi_sta_info_t* station)
{
    /* Return the first matching mode in the port's precedence order. */
    if(station->phy_11n != 0U)
    {
        return eBSPWifiPhy11n;
    }
    if(station->phy_11g != 0U)
    {
        return eBSPWifiPhy11g;
    }
    if(station->phy_11b != 0U)
    {
        return eBSPWifiPhy11b;
    }
    if(station->phy_lr != 0U)
    {
        return eBSPWifiPhyLR;
    }
    return eBSPWifiPhyNone;
}
static bspWifiReason_t wifi_disconnect_reason(uint8_t reason)
{
    switch(reason)
    {
        case WIFI_REASON_AUTH_EXPIRE:
        {
            return eBSPWifiReasonAuthExpired;
        }
        case WIFI_REASON_AUTH_FAIL:
        {
            return eBSPWifiReasonAuthFailed;
        }
        case WIFI_REASON_ASSOC_EXPIRE:
        {
            return eBSPWifiReasonAssocExpired;
        }
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        {
            return eBSPWifiReason4WayTimeout;
        }
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
        {
            return eBSPWifiReason4WayFailed;
        }
        case WIFI_REASON_NO_AP_FOUND:
        {
            return eBSPWifiReasonAPNotFound;
        }
        case WIFI_REASON_BEACON_TIMEOUT:
        {
            return eBSPWifiReasonBeaconTimeout;
        }
        default:
        {
            return eBSPWifiReasonUnspecified;
        }
    }
}

static bool wifi_is_auth_failure(uint8_t reason)
{
    return ((reason == WIFI_REASON_AUTH_FAIL) || (reason == WIFI_REASON_AUTH_EXPIRE) ||
            (reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) ||
            (reason == WIFI_REASON_HANDSHAKE_TIMEOUT));
}

/*
 * This callback runs in the ESP event-loop task. It must not acquire the BSP
 * mutex because synchronous APIs may hold it while waiting for event flags.
 * Queue insertion is non-blocking so event processing cannot wait on the queue.
 */
static void wifi_post_event(wifi_esp_context_t* ctx, bspWifiEvent_t type, bspWifiReason_t reason)
{
    bspWifiContext_t event;
    (void)memset(&event, 0, sizeof(event));
    event.tEventType                                      = type;
    event.tEventData.tConnectionInfo.lastDisconnectReason = reason;
    if(ctx->event_queue != NULL)
    {
        if(osMessageQueuePut(ctx->event_queue, &event, 0U, 0U) != osOK)
        {
            WIFI_LOGW("Wi-Fi event queue full; dropping event=%u", (unsigned)type);
        }
    }
}

/* Translate ESP-IDF Wi-Fi and IP notifications into BSP state and queued events. */
static void wifi_event_handler(void* argument, esp_event_base_t base, int32_t id, void* data)
{
    wifi_esp_context_t* ctx = (wifi_esp_context_t*)argument;
    if((ctx == NULL) || (ctx->flags == NULL))
    {
        return;
    }

    if(base == WIFI_EVENT)
    {
        switch(id)
        {
            case WIFI_EVENT_STA_START:
            {
                wifi_post_event(ctx, eBSPWifiEventSTAStarted, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_STA_STOP:
            {
                atomic_store(&ctx->station_ip_ready, false);
                atomic_store(&ctx->station_link_connected, false);
                wifi_post_event(ctx, eBSPWifiEventSTAStopped, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_STA_CONNECTED:
            {
                atomic_store(&ctx->station_link_connected, true);
                WIFI_LOGI("STA link connected");
                wifi_post_event(ctx, eBSPWifiEventSTAConnected, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                wifi_event_sta_disconnected_t* d = (wifi_event_sta_disconnected_t*)data;
                uint8_t reason_code    = (d != NULL) ? d->reason : 0U;
                bspWifiReason_t reason = wifi_disconnect_reason(reason_code);
                atomic_store(&ctx->station_ip_ready, false);
                atomic_store(&ctx->station_link_connected, false);
                atomic_store(&ctx->auth_failed, wifi_is_auth_failure(reason_code));
                atomic_store(&ctx->last_disconnect_reason, (int)reason);
                WIFI_LOGW("STA disconnected: reason=%u auth_failure=%u", (unsigned)reason_code,
                          wifi_is_auth_failure(reason_code) ? 1U : 0U);
                (void)osEventFlagsSet(ctx->flags, WIFI_FLAG_STA_LOST);
                wifi_post_event(ctx, eBSPWifiEventSTADisconnected, reason);
                break;
            }
            case WIFI_EVENT_AP_START:
            {
                atomic_store(&ctx->ap_ready, true);
                WIFI_LOGI("SoftAP started");
                (void)osEventFlagsSet(ctx->flags, WIFI_FLAG_AP_READY);
                wifi_post_event(ctx, eBSPWifiEventAPStarted, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_AP_STOP:
            {
                atomic_store(&ctx->ap_ready, false);
                WIFI_LOGI("SoftAP stopped");
                wifi_post_event(ctx, eBSPWifiEventAPStopped, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_AP_STACONNECTED:
            {
                wifi_post_event(ctx, eBSPWifiEventAPStationConnected, eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED:
            {
                wifi_post_event(ctx, eBSPWifiEventAPStationDisconnected,
                                eBSPWifiReasonUnspecified);
                break;
            }
            case WIFI_EVENT_SCAN_DONE:
            {
                (void)osEventFlagsSet(ctx->flags, WIFI_FLAG_SCAN_DONE);
                wifi_post_event(ctx, eBSPWifiEventScanDone, eBSPWifiReasonUnspecified);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if(base == IP_EVENT)
    {
        if(id == IP_EVENT_STA_GOT_IP)
        {
            atomic_store(&ctx->station_ip_ready, true);
            atomic_store(&ctx->auth_failed, false);
            (void)osEventFlagsSet(ctx->flags, WIFI_FLAG_STA_IP);
            wifi_post_event(ctx, eBSPWifiEventIPReady, eBSPWifiReasonUnspecified);
        }
        else if(id == IP_EVENT_STA_LOST_IP)
        {
            atomic_store(&ctx->station_ip_ready, false);
            wifi_post_event(ctx, eBSPWifiEventIPFailed, eBSPWifiReasonUnspecified);
        }
        else
        {
            /* Ignore IP notifications unrelated to station address changes. */
        }
    }
}

static ret_err_sts_t wifi_prepare_station(const bspWifiStaConfig_t* station)
{
    wifi_config_t config;
    ret_err_sts_t status = wifi_validate_station(station);
    if(status != RET_ERR_STS_OK)
    {
        return status;
    }

    /* Build a zero-initialized native configuration from the validated input. */
    (void)memset(&config, 0, sizeof(config));
    (void)memcpy(config.sta.ssid, station->ssid, station->ssidLength);
    if(station->security != eWiFiSecurityOpen)
    {
        (void)memcpy(config.sta.password, station->password, station->passwordLength);
    }
    config.sta.channel = station->channel;

    /*
     * Keep the driver's minimum authentication mode aligned with the requested
     * security level, preventing an unintended downgrade to a weaker network.
     */
    if(station->security == eWiFiSecurityWPA3)
    {
        config.sta.threshold.authmode = WIFI_AUTH_WPA3_PSK;
    }
    else if(station->security == eWiFiSecurityWPA2)
    {
        config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    else if(station->security == eWiFiSecurityWPA)
    {
        config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
    }
    else
    {
        config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    config.sta.pmf_cfg.capable  = true;
    config.sta.pmf_cfg.required = false;
    return wifi_result(esp_wifi_set_config(WIFI_IF_STA, &config));
}

static ret_err_sts_t wifi_prepare_ap(const bspWifiApConfig_t* settings)
{
    wifi_config_t config;
    ret_err_sts_t status = wifi_validate_ap(settings);
    if(status != RET_ERR_STS_OK)
    {
        return status;
    }

    /* Build a zero-initialized native configuration from the validated input. */
    (void)memset(&config, 0, sizeof(config));
    (void)memcpy(config.ap.ssid, settings->ssid, settings->ssidLength);
    config.ap.ssid_len       = settings->ssidLength;
    config.ap.channel        = settings->channel;
    config.ap.max_connection = settings->maxConnections;
    config.ap.ssid_hidden    = settings->ssidHidden;
    if(settings->security == eWiFiSecurityOpen)
    {
        config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        (void)memcpy(config.ap.password, settings->password, settings->passwordLength);
        config.ap.authmode =
        (settings->security == eWiFiSecurityWPA3) ? WIFI_AUTH_WPA3_PSK : WIFI_AUTH_WPA2_PSK;
        config.ap.pmf_cfg.required = (settings->security == eWiFiSecurityWPA3);
    }
    return wifi_result(esp_wifi_set_config(WIFI_IF_AP, &config));
}

static ret_err_sts_t wifi_connect_locked(wifi_esp_context_t* ctx)
{
    uint32_t flags;
    if(!ctx->started || !ctx->have_sta_config)
    {
        return RET_ERR_STS_INVALID_STATE;
    }

    /* Clear stale completion flags before starting a new connection attempt. */
    (void)osEventFlagsClear(ctx->flags, WIFI_EVENT_WAIT_MASK);
    atomic_store(&ctx->auth_failed, false);
    {
        esp_err_t error = esp_wifi_connect();
        if(error != ESP_OK)
        {
            return wifi_result(error);
        }
    }

    /* Wait for either IP acquisition or station-link loss. */
    flags = osEventFlagsWait(ctx->flags, WIFI_EVENT_WAIT_MASK, osFlagsWaitAny,
                             wifi_ms_to_ticks(configWIFI_WAIT_TIMEOUT_MS));
    if((flags & osFlagsError) != 0U)
    {
        WIFI_LOGW("STA connection timed out waiting for IP");
        return RET_ERR_STS_TIMEOUT;
    }
    if((flags & WIFI_FLAG_STA_IP) != 0U)
    {
        WIFI_LOGI("STA acquired IP address");
        return RET_ERR_STS_OK;
    }
    if(atomic_load(&ctx->auth_failed))
    {
        WIFI_LOGW("STA authentication failed");
        return RET_ERR_STS_AUTH_FAILED;
    }
    WIFI_LOGW("STA connection failed");
    return RET_ERR_STS_CONNECTION_FAILED;
}

static ret_err_sts_t wifi_stop_locked(wifi_esp_context_t* ctx)
{
    esp_err_t error;
    if(!ctx->started)
    {
        return RET_ERR_STS_OK;
    }

    /* Treat an already-stopped driver as successfully stopped. */
    error = esp_wifi_stop();
    if((error != ESP_OK) && (error != ESP_ERR_WIFI_NOT_STARTED))
    {
        return wifi_result(error);
    }
    ctx->started = false;
    atomic_store(&ctx->station_ip_ready, false);
    atomic_store(&ctx->station_link_connected, false);
    atomic_store(&ctx->ap_ready, false);
    (void)osEventFlagsClear(ctx->flags, WIFI_FLAG_STA_IP | WIFI_FLAG_AP_READY |
                                        WIFI_FLAG_STA_LOST | WIFI_FLAG_SCAN_DONE);
    return RET_ERR_STS_OK;
}

/*
 * Release resources owned by this instance. Preserve default event-loop and
 * global network services that may have been created by other components.
 */
static void wifi_release_resources(wifi_esp_context_t* ctx)
{
    if(ctx->started)
    {
        (void)esp_wifi_stop();
        ctx->started = false;
    }
    if(ctx->wifi_handler != NULL)
    {
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                    ctx->wifi_handler);
        ctx->wifi_handler = NULL;
    }
    if(ctx->ip_handler != NULL)
    {
        (void)esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID,
                                                    ctx->ip_handler);
        ctx->ip_handler = NULL;
    }
    if(ctx->driver_initialized)
    {
        (void)esp_wifi_deinit();
        ctx->driver_initialized = false;
    }
    if(ctx->sta_netif != NULL)
    {
        esp_netif_destroy_default_wifi(ctx->sta_netif);
        ctx->sta_netif = NULL;
    }
    if(ctx->ap_netif != NULL)
    {
        esp_netif_destroy_default_wifi(ctx->ap_netif);
        ctx->ap_netif = NULL;
    }
    if(ctx->owns_event_loop)
    {
        (void)esp_event_loop_delete_default();
        ctx->owns_event_loop = false;
    }
    if(ctx->event_queue != NULL)
    {
        (void)osMessageQueueDelete(ctx->event_queue);
        ctx->event_queue = NULL;
    }
    if(ctx->flags != NULL)
    {
        (void)osEventFlagsDelete(ctx->flags);
        ctx->flags = NULL;
    }
    if(ctx->mutex != NULL)
    {
        (void)osMutexDelete(ctx->mutex);
        ctx->mutex = NULL;
    }
}
/* ==========================================================================
 * PUBLIC FUNCTIONS DEFINATION
 * ========================================================================== */
ret_err_sts_t bspWifiInit(bspWifiHandle_t* handle)
{
    esp_err_t error;
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    RET_CHECK_POINTER(handle, RET_ERR_STS_INVALID_PARAM);

    /* Repeated initialization for the active handle is idempotent. */
    if(s_wifi.owner == handle)
    {
        return RET_ERR_STS_OK;
    }
    if(s_wifi.owner != NULL)
    {
        return wifi_report("Wi-Fi already owned", RET_ERR_STS_BUSY);
    }
    if(handle->platform_ctx != NULL)
    {
        return wifi_report("Wi-Fi handle context not empty", RET_ERR_STS_INVALID_STATE);
    }
    if(osKernelGetState() != osKernelRunning)
    {
        return wifi_report("Wi-Fi requires a running CMSIS kernel", RET_ERR_STS_INVALID_STATE);
    }

    (void)memset(&s_wifi, 0, sizeof(s_wifi));
    s_wifi.owner = handle;
    s_wifi.mode  = eWiFiModeMax;
    atomic_init(&s_wifi.station_ip_ready, false);
    atomic_init(&s_wifi.station_link_connected, false);
    atomic_init(&s_wifi.ap_ready, false);
    atomic_init(&s_wifi.auth_failed, false);
    atomic_init(&s_wifi.last_disconnect_reason, (int)eBSPWifiReasonUnspecified);

    /* Create the synchronization objects used by the driver and event bridge. */
    s_wifi.mutex = osMutexNew(NULL);
    s_wifi.flags = osEventFlagsNew(NULL);
    s_wifi.event_queue =
    osMessageQueueNew(configWIFI_EVENT_QUEUE_DEPTH, sizeof(bspWifiContext_t), NULL);
    if((s_wifi.mutex == NULL) || (s_wifi.flags == NULL) || (s_wifi.event_queue == NULL))
    {
        WIFI_LOGE("Wi-Fi init failed: CMSIS resource allocation");
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return RET_ERR_STS_NO_MEM;
    }

    /*
     * Persistent storage belongs to the application. If ESP-IDF internal NVS
     * support is enabled, the application must initialize NVS before this call.
     * The Wi-Fi driver is configured to keep its settings in RAM.
     */
    error = esp_netif_init();
    if((error != ESP_OK) && (error != ESP_ERR_INVALID_STATE))
    {
        ret_err_sts_t result = wifi_result(error);
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return result;
    }

    /* Record ownership so teardown does not delete another component's loop. */
    error = esp_event_loop_create_default();
    if(error == ESP_OK)
    {
        s_wifi.owns_event_loop = true;
    }
    else if(error != ESP_ERR_INVALID_STATE)
    {
        ret_err_sts_t result = wifi_result(error);
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return result;
    }

    /* Create the default station and access-point interfaces. */
    s_wifi.sta_netif = esp_netif_create_default_wifi_sta();
    s_wifi.ap_netif  = esp_netif_create_default_wifi_ap();
    if((s_wifi.sta_netif == NULL) || (s_wifi.ap_netif == NULL))
    {
        WIFI_LOGE("Wi-Fi init failed: default interface allocation");
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return RET_ERR_STS_INIT_FAILED;
    }

    error = esp_wifi_init(&init_config);
    if(error != ESP_OK)
    {
        ret_err_sts_t result = wifi_result(error);
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return result;
    }
    s_wifi.driver_initialized = true;

    /* Keep Wi-Fi configuration in RAM rather than delegating persistence to IDF. */
    error = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(error == ESP_OK)
    {
        error = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                    wifi_event_handler, &s_wifi,
                                                    &s_wifi.wifi_handler);
    }
    if(error == ESP_OK)
    {
        error = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler,
                                                    &s_wifi, &s_wifi.ip_handler);
    }
    if(error != ESP_OK)
    {
        ret_err_sts_t result = wifi_result(error);
        wifi_release_resources(&s_wifi);
        (void)memset(&s_wifi, 0, sizeof(s_wifi));
        return result;
    }

    handle->platform_ctx = &s_wifi;
    WIFI_LOGI("Wi-Fi BSP initialized; application owns profile persistence");
    wifi_post_event(&s_wifi, eBSPWifiEventReady, eBSPWifiReasonUnspecified);
    return RET_ERR_STS_OK;
}

ret_err_sts_t bspWifiDeInit(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiOn configuration", result);
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiOn mutex", result);
    }
    result = wifi_stop_locked(ctx);
    wifi_unlock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * The caller must serialize deinitialization against every other API call,
     * including event-queue reads, so no task uses resources while they are freed.
     */
    wifi_release_resources(ctx);
    handle->platform_ctx = NULL;
    (void)memset(ctx, 0, sizeof(*ctx));
    WIFI_LOGI("Wi-Fi BSP deinitialized");
    return RET_ERR_STS_OK;
}

ret_err_sts_t bspWifiOn(bspWifiHandle_t* handle, bspWifiMode_t mode, const void* pvParam)
{
    wifi_esp_context_t* ctx;
    wifi_mode_t esp_mode;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    RET_CHECK_POINTER(pvParam, RET_ERR_STS_INVALID_PARAM);

    /* P2P is part of the portable mode enumeration but is not implemented here. */
    if(mode == eWiFiModeP2P)
    {
        return wifi_report("Wi-Fi P2P unsupported", RET_ERR_STS_NOT_SUPPORTED);
    }
    if((mode != eWiFiModeStation) && (mode != eWiFiModeAP) && (mode != eWiFiModeAPStation))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    /* Validate the configuration type selected by mode before changing driver state. */
    if(mode == eWiFiModeStation)
    {
        result = wifi_validate_station((const bspWifiStaConfig_t*)pvParam);
    }
    else if(mode == eWiFiModeAP)
    {
        result = wifi_validate_ap((const bspWifiApConfig_t*)pvParam);
    }
    else
    {
        const bspWifiApStaConfig_t* cfg = (const bspWifiApStaConfig_t*)pvParam;
        result                          = wifi_validate_ap(&cfg->apConfig);
        if(result == RET_ERR_STS_OK)
        {
            result = wifi_validate_station(&cfg->staConfig);
        }
    }
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Serialize reconfiguration and stop the previous mode before applying the new one. */
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_stop_locked(ctx);
    if(result != RET_ERR_STS_OK)
    {
        wifi_unlock(ctx);
        return result;
    }

    esp_mode = (mode == eWiFiModeStation) ?
               WIFI_MODE_STA :
               ((mode == eWiFiModeAP) ? WIFI_MODE_AP : WIFI_MODE_APSTA);
    result   = wifi_result(esp_wifi_set_mode(esp_mode));
    if(result == RET_ERR_STS_OK)
    {
        if(mode == eWiFiModeStation)
        {
            const bspWifiStaConfig_t* cfg = (const bspWifiStaConfig_t*)pvParam;
            result                        = wifi_prepare_station(cfg);
            if(result == RET_ERR_STS_OK)
            {
                ctx->sta_config      = *cfg;
                ctx->have_sta_config = true;
            }
        }
        else if(mode == eWiFiModeAP)
        {
            result = wifi_prepare_ap((const bspWifiApConfig_t*)pvParam);
            ctx->have_sta_config = false;
        }
        else
        {
            const bspWifiApStaConfig_t* cfg = (const bspWifiApStaConfig_t*)pvParam;
            result = wifi_prepare_ap(&cfg->apConfig);
            if(result == RET_ERR_STS_OK)
            {
                result = wifi_prepare_station(&cfg->staConfig);
            }
            if(result == RET_ERR_STS_OK)
            {
                ctx->sta_config      = cfg->staConfig;
                ctx->have_sta_config = true;
            }
        }
    }
    if(result == RET_ERR_STS_OK)
    {
        /* Discard stale signals before waiting for this mode's startup events. */
        (void)osEventFlagsClear(ctx->flags, WIFI_FLAG_AP_READY | WIFI_EVENT_WAIT_MASK);
        result = wifi_result(esp_wifi_start());
        if(result == RET_ERR_STS_OK)
        {
            ctx->started = true;
            ctx->mode    = mode;
        }
    }
    if((result == RET_ERR_STS_OK) && (mode != eWiFiModeStation))
    {
        result = wifi_wait_flag(ctx, WIFI_FLAG_AP_READY);
    }
    if((result == RET_ERR_STS_OK) && (mode != eWiFiModeAP))
    {
        result = wifi_connect_locked(ctx);
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiOn", result);
}

ret_err_sts_t bspWifiOff(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result == RET_ERR_STS_OK)
    {
        result = wifi_stop_locked(ctx);
        wifi_unlock(ctx);
    }
    return wifi_report("bspWifiOff", result);
}

ret_err_sts_t bspWifiConnectToAp(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if((ctx->mode != eWiFiModeStation) && (ctx->mode != eWiFiModeAPStation))
    {
        result = RET_ERR_STS_INVALID_STATE;
    }
    else if(atomic_load(&ctx->station_ip_ready))
    {
        result = RET_ERR_STS_ALREADY_CONNECTED;
    }
    else
    {
        result = wifi_connect_locked(ctx);
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiConnectToAp", result);
}

ret_err_sts_t bspWifiDisconnectToAp(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || ((ctx->mode != eWiFiModeStation) && (ctx->mode != eWiFiModeAPStation)))
    {
        result = RET_ERR_STS_INVALID_STATE;
    }
    else if(!atomic_load(&ctx->station_link_connected))
    {
        result = RET_ERR_STS_OK;
    }
    else
    {
        /* Clear any previous disconnect signal before waiting for this request. */
        (void)osEventFlagsClear(ctx->flags, WIFI_FLAG_STA_LOST);
        result = wifi_result(esp_wifi_disconnect());
        if(result == RET_ERR_STS_OK)
        {
            result = wifi_wait_flag(ctx, WIFI_FLAG_STA_LOST);
        }
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiDisconnectToAp", result);
}
ret_err_sts_t bspWifiStartAP(bspWifiHandle_t* handle, const bspWifiApConfig_t* apCfg)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_validate_ap(apCfg);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /* Do not replace an already-running Wi-Fi mode through this AP-only API. */
    if(ctx->started)
    {
        result = atomic_load(&ctx->ap_ready) ? RET_ERR_STS_ALREADY_EXISTS :
                                               RET_ERR_STS_NOT_SUPPORTED;
    }
    else
    {
        result = wifi_result(esp_wifi_set_mode(WIFI_MODE_AP));
        if(result == RET_ERR_STS_OK)
        {
            result = wifi_prepare_ap(apCfg);
        }
        if(result == RET_ERR_STS_OK)
        {
            /* Clear an old startup signal before waiting for this AP start. */
            (void)osEventFlagsClear(ctx->flags, WIFI_FLAG_AP_READY);
            result = wifi_result(esp_wifi_start());
            if(result == RET_ERR_STS_OK)
            {
                ctx->started         = true;
                ctx->mode            = eWiFiModeAP;
                ctx->have_sta_config = false;
                result               = wifi_wait_flag(ctx, WIFI_FLAG_AP_READY);
            }
        }
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiStartAP", result);
}

ret_err_sts_t bspWifiStopAP(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || !atomic_load(&ctx->ap_ready))
    {
        result = RET_ERR_STS_OK;
    }
    else if(ctx->mode == eWiFiModeAPStation)
    {
        /* Disable AP mode while leaving the station connection active. */
        result = wifi_result(esp_wifi_set_mode(WIFI_MODE_STA));
        if(result == RET_ERR_STS_OK)
        {
            ctx->mode = eWiFiModeStation;
            atomic_store(&ctx->ap_ready, false);
        }
    }
    else
    {
        result = wifi_stop_locked(ctx);
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiStopAP", result);
}

/* -------------------------------------------------------------------------- */
/* Access-point queries */
/* -------------------------------------------------------------------------- */

ret_err_sts_t bspWifiGetConnStations(bspWifiHandle_t* handle,
                                     bspWifiStationInfo_t* stations,
                                     uint8_t* numStations,
                                     uint8_t maxStations)
{
    wifi_esp_context_t* ctx;
    wifi_sta_list_t list;
    uint8_t copied;
    uint8_t available;
    uint8_t i;
    ret_err_sts_t result;
    RET_CHECK_POINTER(numStations, RET_ERR_STS_INVALID_PARAM);
    *numStations = 0U;
    RET_CHECK_POINTER(stations, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_NOT_ZERO(maxStations, RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || !atomic_load(&ctx->ap_ready))
    {
        wifi_unlock(ctx);
        return RET_ERR_STS_NOT_READY;
    }
    (void)memset(&list, 0, sizeof(list));
    result = wifi_result(esp_wifi_ap_get_sta_list(&list));
    if(result != RET_ERR_STS_OK)
    {
        wifi_unlock(ctx);
        return wifi_report("bspWifiGetConnStations", result);
    }
    if(list.num < 0)
    {
        wifi_unlock(ctx);
        return RET_ERR_STS_DATA_INVALID;
    }

    /* Copy only entries that fit in the caller-provided array. */
    available = (list.num > UINT8_MAX) ? UINT8_MAX : (uint8_t)list.num;
    copied    = (available > maxStations) ? maxStations : available;
    for(i = 0U; i < copied; ++i)
    {
        (void)memcpy(stations[i].mac, list.sta[i].mac, BSP_WIFI_MAC_ADDR_MAX_LEN);
        stations[i].rssi    = list.sta[i].rssi;
        stations[i].phyMode = wifi_phy_to_bsp(&list.sta[i]);
    }
    *numStations = copied;
    wifi_unlock(ctx);
    return (available > maxStations) ? RET_ERR_STS_INVALID_SIZE : RET_ERR_STS_OK;
}

ret_err_sts_t bspWifiDisconnectStation(bspWifiHandle_t* handle,
                                       const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN])
{
    wifi_esp_context_t* ctx;
    uint16_t aid = 0U;
    ret_err_sts_t result;
    RET_CHECK_POINTER(mac, RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || !atomic_load(&ctx->ap_ready))
    {
        result = RET_ERR_STS_NOT_READY;
    }
    else
    {
        /* Resolve the client's association ID before requesting deauthentication. */
        result = wifi_result(esp_wifi_ap_get_sta_aid(mac, &aid));
        if((result == RET_ERR_STS_OK) && (aid == 0U))
        {
            result = RET_ERR_STS_NOT_FOUND;
        }
        if(result == RET_ERR_STS_OK)
        {
            result = wifi_result(esp_wifi_deauth_sta(aid));
        }
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiDisconnectStation", result);
}

ret_err_sts_t bspWifiGetAPConfig(bspWifiHandle_t* handle, bspWifiApConfig_t* apCfg)
{
    wifi_esp_context_t* ctx;
    wifi_config_t config;
    size_t count;
    ret_err_sts_t result;
    RET_CHECK_POINTER(apCfg, RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || !atomic_load(&ctx->ap_ready))
    {
        wifi_unlock(ctx);
        return RET_ERR_STS_NOT_READY;
    }
    (void)memset(&config, 0, sizeof(config));
    result = wifi_result(esp_wifi_get_config(WIFI_IF_AP, &config));
    if(result == RET_ERR_STS_OK)
    {
        (void)memset(apCfg, 0, sizeof(*apCfg));

        /* Bound copied lengths by both the native field and BSP capacities. */
        count = (config.ap.ssid_len <= 32U) ? config.ap.ssid_len : 32U;
        (void)memcpy(apCfg->ssid, config.ap.ssid, count);
        apCfg->ssidLength = (uint8_t)count;
        count = wifi_bounded_length(config.ap.password, sizeof(config.ap.password));
        if(count > BSP_WIFI_PASSWORD_MAX_LEN)
        {
            count = BSP_WIFI_PASSWORD_MAX_LEN;
        }
        (void)memcpy(apCfg->password, config.ap.password, count);
        apCfg->passwordLength = (uint8_t)count;
        apCfg->security       = wifi_auth_to_bsp(config.ap.authmode);
        apCfg->channel        = config.ap.channel;
        apCfg->ssidHidden     = config.ap.ssid_hidden;
        apCfg->maxConnections = config.ap.max_connection;
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiGetAPConfig", result);
}

ret_err_sts_t bspWifiGetAPStatistics(bspWifiHandle_t* handle, bspWifiStatistics_t* stats)
{
    wifi_esp_context_t* ctx;
    wifi_sta_list_t list;
    ret_err_sts_t result;
    RET_CHECK_POINTER(stats, RET_ERR_STS_INVALID_PARAM);
    (void)memset(stats, 0, sizeof(*stats));
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || !atomic_load(&ctx->ap_ready))
    {
        wifi_unlock(ctx);
        return RET_ERR_STS_NOT_READY;
    }
    (void)memset(&list, 0, sizeof(list));
    result = wifi_result(esp_wifi_ap_get_sta_list(&list));
    if(result == RET_ERR_STS_OK)
    {
        if(list.num < 0)
        {
            result = RET_ERR_STS_DATA_INVALID;
        }
        else
        {
            /* This port currently reports the supported station-count statistic. */
            stats->connectedStations =
            (list.num > UINT8_MAX) ? UINT8_MAX : (uint8_t)list.num;
            stats->validMask = BSP_WIFI_STATS_VALID_STATIONS;
        }
    }
    wifi_unlock(ctx);
    return wifi_report("bspWifiGetAPStatistics", result);
} /* Optional application-owned station profile persistence.
   * Callbacks run outside the BSP mutex and may block. The caller must
   * serialize deinitialization and protect callback registration and context
   * lifetime against overlapping operations. */
ret_err_sts_t bspWifiSetProfileCallbacks(bspWifiHandle_t* handle,
                                         const bspWifiProfileCallbacks_t* callbacks)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiSetProfileCallbacks", result);
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiSetProfileCallbacks mutex", result);
    }
    if(callbacks == NULL)
    {
        (void)memset(&ctx->profile_callbacks, 0, sizeof(ctx->profile_callbacks));
        WIFI_LOGI("Station profile callbacks disabled");
    }
    else
    {
        ctx->profile_callbacks = *callbacks;
        WIFI_LOGI("Station profile callbacks configured");
    }
    wifi_unlock(ctx);
    return RET_ERR_STS_OK;
}

/* ------------------------ */

ret_err_sts_t bspWifiStoreStationProfile(bspWifiHandle_t* handle,
                                         const bspWifiStaConfig_t* station)
{
    wifi_esp_context_t* ctx;
    bspWifiProfileSaveCallback_t callback;
    void* callback_context;
    ret_err_sts_t result = wifi_validate_station(station);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("Invalid station profile", result);
    }
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiStoreStationProfile", result);
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiStoreStationProfile mutex", result);
    }

    /* Copy the callback and context while protected, then invoke it unlocked. */
    callback         = ctx->profile_callbacks.save;
    callback_context = ctx->profile_callbacks.context;
    wifi_unlock(ctx);

    if(callback == NULL)
    {
        WIFI_LOGD("Station profile save skipped (no callback)");
        return RET_ERR_STS_OK;
    }
    result = callback(station, callback_context);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("Station profile save callback", result);
    }
    WIFI_LOGI("Station profile saved by application");
    return RET_ERR_STS_OK;
}

/* ------------------------------------------------------------------------ */

ret_err_sts_t
bspWifiGetStationProfile(bspWifiHandle_t* handle, uint8_t index, bspWifiStaConfig_t* sta_cfg)
{
    wifi_esp_context_t* ctx;
    bspWifiProfileLoadCallback_t callback;
    void* callback_context;
    ret_err_sts_t result;
    RET_CHECK_POINTER(sta_cfg, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM(index < BSP_WIFI_MAX_SAVED_NETWORKS, RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiGetStationProfile", result);
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiGetStationProfile mutex", result);
    }

    /* Snapshot the callback registration before invoking application code. */
    callback         = ctx->profile_callbacks.load;
    callback_context = ctx->profile_callbacks.context;
    wifi_unlock(ctx);

    if(callback == NULL)
    {
        WIFI_LOGD("Station profile load unavailable (no callback)");
        return RET_ERR_STS_NOT_SUPPORTED;
    }
    (void)memset(sta_cfg, 0, sizeof(*sta_cfg));
    result = callback(index, sta_cfg, callback_context);
    if(result == RET_ERR_STS_OK)
    {
        /* Validate data returned from application-owned storage before use. */
        result = wifi_validate_station(sta_cfg);
        if(result != RET_ERR_STS_OK)
        {
            (void)memset(sta_cfg, 0, sizeof(*sta_cfg));
            WIFI_LOGE("Station profile load produced invalid data");
            return RET_ERR_STS_DATA_CORRUPT;
        }
        WIFI_LOGD("Station profile loaded from application (index=%u)", (unsigned)index);
        return RET_ERR_STS_OK;
    }
    (void)memset(sta_cfg, 0, sizeof(*sta_cfg));
    return wifi_report("Station profile load callback", result);
}

/* ------------------------------------------------------------------------ */

ret_err_sts_t
bspWifiRemoveStationProfile(bspWifiHandle_t* handle, const uint8_t* ssid, uint8_t ssid_len)
{
    wifi_esp_context_t* ctx;
    bspWifiProfileRemoveCallback_t callback;
    void* callback_context;
    ret_err_sts_t result;
    RET_CHECK_POINTER(ssid, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_PARAM(wifi_valid_ssid(ssid_len), RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiRemoveStationProfile", result);
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("bspWifiRemoveStationProfile mutex", result);
    }

    /* Snapshot the callback registration before invoking application code. */
    callback         = ctx->profile_callbacks.remove;
    callback_context = ctx->profile_callbacks.context;
    wifi_unlock(ctx);

    if(callback == NULL)
    {
        WIFI_LOGD("Station profile removal skipped (no callback)");
        return RET_ERR_STS_OK;
    }
    result = callback(ssid, ssid_len, callback_context);
    if(result != RET_ERR_STS_OK)
    {
        return wifi_report("Station profile remove callback", result);
    }
    WIFI_LOGI("Station profile removed by application");
    return RET_ERR_STS_OK;
}

/* ------------------------------------------------------------------------ */

/* Scanning and application event retrieval. */
ret_err_sts_t bspWifiScan(bspWifiHandle_t* handle,
                          bspWifiApScanResult_t* results,
                          uint8_t max_results,
                          uint8_t* out_count)
{
    wifi_esp_context_t* ctx;
    wifi_scan_config_t scan;
    wifi_ap_record_t* records = NULL;
    uint16_t available        = 0U;
    uint16_t fetch;
    uint16_t index;
    ret_err_sts_t result;
    RET_CHECK_POINTER(results, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_POINTER(out_count, RET_ERR_STS_INVALID_PARAM);
    RET_CHECK_NOT_ZERO(max_results, RET_ERR_STS_INVALID_PARAM);
    *out_count = 0U;
    (void)memset(results, 0, sizeof(results[0]) * max_results);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    result = wifi_lock(ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }
    if(!ctx->started || ((ctx->mode != eWiFiModeStation) && (ctx->mode != eWiFiModeAPStation)))
    {
        result = RET_ERR_STS_INVALID_STATE;
        goto scan_done;
    }
    if(ctx->scanning)
    {
        result = RET_ERR_STS_BUSY;
        goto scan_done;
    }
    ctx->scanning = true;
    (void)memset(&scan, 0, sizeof(scan));
    scan.show_hidden = true;
    result           = wifi_result(esp_wifi_scan_start(&scan, true));
    if(result != RET_ERR_STS_OK)
    {
        goto scan_exit;
    }
    result = wifi_result(esp_wifi_scan_get_ap_num(&available));
    if(result != RET_ERR_STS_OK)
    {
        (void)esp_wifi_clear_ap_list();
        goto scan_exit;
    }
    if(available == 0U)
    {
        (void)esp_wifi_clear_ap_list();
        result = RET_ERR_STS_OK;
        goto scan_exit;
    }

    /* Limit temporary allocation and result copying to the caller's capacity. */
    fetch = (available > (uint16_t)max_results) ? (uint16_t)max_results : available;
    records = (wifi_ap_record_t*)calloc((size_t)fetch, sizeof(*records));
    if(records == NULL)
    {
        (void)esp_wifi_clear_ap_list();
        result = RET_ERR_STS_NO_MEM;
        goto scan_exit;
    }
    result = wifi_result(esp_wifi_scan_get_ap_records(&fetch, records));
    if(result != RET_ERR_STS_OK)
    {
        (void)esp_wifi_clear_ap_list();
        goto scan_exit;
    }
    for(index = 0U; index < fetch; ++index)
    {
        size_t length =
        wifi_bounded_length(records[index].ssid, sizeof(records[index].ssid));
        if(length > BSP_WIFI_SSID_MAX_LEN)
        {
            length = BSP_WIFI_SSID_MAX_LEN;
        }
        (void)memcpy(results[index].ssid, records[index].ssid, length);
        results[index].ssidLength = (uint8_t)length;
        (void)memcpy(results[index].bssid, records[index].bssid, BSP_WIFI_MAC_ADDR_MAX_LEN);
        results[index].rssi     = records[index].rssi;
        results[index].channel  = records[index].primary;
        results[index].security = wifi_auth_to_bsp(records[index].authmode);
    }
    *out_count = (uint8_t)fetch;
    if(available > fetch)
    {
        result = RET_ERR_STS_INVALID_SIZE;
    }
    if(result == RET_ERR_STS_OK)
    {
        WIFI_LOGI("Wi-Fi scan completed: %u network(s)", (unsigned)fetch);
    }
    else if(result == RET_ERR_STS_INVALID_SIZE)
    {
        WIFI_LOGW("Wi-Fi scan results truncated: %u of %u", (unsigned)fetch,
                  (unsigned)available);
    }
scan_exit:
    /* Release scan records and clear the in-progress state on every exit path. */
    free(records);
    ctx->scanning = false;
scan_done:
    wifi_unlock(ctx);
    return wifi_report("bspWifiScan", result);
}

/* ------------------------------------------------------------------------ */

ret_err_sts_t bspWifiStartScan(bspWifiHandle_t* handle, bspWifiApScanResult_t* results, uint8_t max_results)
{
    uint8_t count        = 0U;
    ret_err_sts_t result = bspWifiScan(handle, results, max_results, &count);
    if((results != NULL) && (max_results > 0U))
    {
        results[0].num_result = count;
    }
    return result;
}

/* ------------------------ */

ret_err_sts_t bspWifiGetEvent(bspWifiHandle_t* handle, bspWifiContext_t* event, uint32_t timeout_ms)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result;
    RET_CHECK_POINTER(event, RET_ERR_STS_INVALID_PARAM);
    result = wifi_context(handle, &ctx);
    if(result != RET_ERR_STS_OK)
    {
        return result;
    }

    /*
     * Do not hold the mutex while waiting: other operations may need it to
     * process events. The caller must serialize this call against deinit.
     */
    if(osMessageQueueGet(ctx->event_queue, event, NULL, wifi_ms_to_ticks(timeout_ms)) != osOK)
    {
        return (timeout_ms == 0U) ? RET_ERR_STS_NO_DATA : RET_ERR_STS_TIMEOUT;
    }
    return RET_ERR_STS_OK;
}

/* ------------------------ */

ret_err_sts_t bspWifiReset(bspWifiHandle_t* handle)
{
    wifi_esp_context_t* ctx;
    ret_err_sts_t result = wifi_context(handle, &ctx);
    (void)ctx;
    return (result == RET_ERR_STS_OK) ? RET_ERR_STS_NOT_SUPPORTED : result;
}