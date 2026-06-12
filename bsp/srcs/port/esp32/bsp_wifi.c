/**
 * @file bsp_wifi.c
 * @brief BSP Wi-Fi port layer for ESP32 (ESP-IDF + CMSIS-RTOS2).
 *
 * @par Architecture
 * - Platform-independent BSP API backed by ESP-IDF Wi-Fi driver.
 * - All RTOS synchronization uses CMSIS-RTOS2 only (no direct FreeRTOS).
 * - Event signaling uses osEventFlags for internal sync.
 * - Application notifications via osMessageQueue.
 * - Per-handle mutex with priority inheritance for thread safety.
 *
 * @par Thread Safety
 * - Public APIs acquire handle mutex before accessing shared state.
 * - ESP event handler runs in ESP system task context; uses only
 *   osEventFlagsSet which is ISR-safe in CMSIS-RTOS2.
 * - Callback invocation pattern: set flags only, never block.
 *
 * @see bsp_wifi.h
 */

#include <string.h>
#include <stdlib.h>

/* BSP */
#include "bsp_wifi.h"
#include "bsp_config.h"
#include "bsp_err_sts.h"

/* CMSIS-RTOS2 (only RTOS API used) */
#include "cmsis_os2.h"

/* ESP-IDF (hardware access only) */
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_smartconfig.h"
#include "esp_netif_ip_addr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

/* -------------------------------------------------- */
/* INTERNAL LOGGING                                   */
/* -------------------------------------------------- */

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define WIFI_LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define WIFI_LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)

/* -------------------------------------------------- */
/* CONFIGURATION                                      */
/* -------------------------------------------------- */

// /** @brief Application event queue depth. */
// #define BSP_WIFI_EVENT_QUEUE_LEN (10U)

/** @brief Default timeout for blocking operations (ms). */
#define BSP_WIFI_WAIT_PERIOD_MS (5000U)

/** @brief Maximum STA connections in AP mode. */
#define BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS (5U)

/** @brief NVS namespace for Wi-Fi profiles. */
#define BSP_WIFI_NVS_NAMESPACE "bsp_wifi"

/** @brief NVS key for stored STA profile count. */
#define BSP_WIFI_NVS_STA_COUNT_KEY "sta_count"

/** @brief Maximum number of stored STA profiles. */
#define BSP_WIFI_MAX_SAVED_STA (5U)

/** @brief NVS key format for individual STA profiles. */
#define BSP_WIFI_NVS_STA_KEY_FMT "sta_%u"

/* -------------------------------------------------- */
/* VALIDATION MACROS                                  */
/* -------------------------------------------------- */

#define CHECK_VALID_WIFI_SSID_LEN(x) \
    (((x) > 0U) && ((x) <= BSP_WIFI_SSID_MAX_LEN))

#define CHECK_VALID_PASSPHRASE_LEN(x) \
    (((x) > 0U) && ((x) <= BSP_WIFI_PASSWORD_MAX_LEN))

#define WIFI_MAC_MATCH(mac1, mac2) \
    (memcmp((mac1), (mac2), BSP_WIFI_MAC_ADDR_MAX_LEN) == 0)

/* -------------------------------------------------- */
/* EVENT FLAGS                                        */
/* -------------------------------------------------- */

#define BSP_WIFI_F_STA_CONNECTED    ((uint32_t)(1U << 0U))
#define BSP_WIFI_F_STA_DISCONNECTED ((uint32_t)(1U << 1U))
#define BSP_WIFI_F_STA_STARTED      ((uint32_t)(1U << 2U))
#define BSP_WIFI_F_STA_STOPPED      ((uint32_t)(1U << 3U))
#define BSP_WIFI_F_AP_STARTED       ((uint32_t)(1U << 4U))
#define BSP_WIFI_F_AP_STOPPED       ((uint32_t)(1U << 5U))
#define BSP_WIFI_F_WPS_SUCCESS      ((uint32_t)(1U << 6U))
#define BSP_WIFI_F_WPS_FAILED       ((uint32_t)(1U << 7U))
#define BSP_WIFI_F_WIFI_STARTED     ((uint32_t)(1U << 8U))
#define BSP_WIFI_F_WIFI_STOPPED     ((uint32_t)(1U << 9U))
#define BSP_WIFI_F_AUTH_FAILED      ((uint32_t)(1U << 10U))
#define BSP_WIFI_F_SCAN_DONE        ((uint32_t)(1U << 11U))
#define BSP_WIFI_F_IP_READY         ((uint32_t)(1U << 12U))

/* -------------------------------------------------- */
/* MUTEX LOCK/UNLOCK MACROS                           */
/* -------------------------------------------------- */

/**
 * @brief Acquire handle mutex with timeout.
 *
 * @par Corner Case
 * - If handle or lock is NULL, returns INVALID_PARAM immediately.
 * - If mutex cannot be acquired within timeout, returns TIMEOUT.
 *   This prevents indefinite blocking if another thread holds the
 *   mutex due to a long ESP-IDF operation.
 */
#define WIFI_LOCK(h)                                                   \
    do                                                                 \
    {                                                                  \
        if(((h) == NULL) || ((h)->lock == NULL))                       \
        {                                                              \
            return BSP_ERR_STS_INVALID_PARAM;                          \
        }                                                              \
        if(osMutexAcquire((h)->lock, BSP_WIFI_WAIT_PERIOD_MS) != osOK) \
        {                                                              \
            WIFI_LOGE("Failed to acquire WiFi mutex");                 \
            return BSP_ERR_STS_TIMEOUT;                                \
        }                                                              \
    } while(0)

#define WIFI_UNLOCK(h)                           \
    do                                           \
    {                                            \
        if(((h) != NULL) && ((h)->lock != NULL)) \
        {                                        \
            (void)osMutexRelease((h)->lock);     \
        }                                        \
    } while(0)

/* -------------------------------------------------- */
/* EVENT FLAG HELPER MACROS                           */
/* -------------------------------------------------- */

/**
 * @brief Wait for specific event flags with timeout.
 *
 * @par Corner Case
 * - osEventFlagsWait returns osFlagsErrorTimeout on timeout.
 * - Returns osFlagsErrorParameter if flags ID is invalid.
 * - Bit 31 set indicates error in CMSIS-RTOS2 convention.
 * - We check bit 31 (error indicator) rather than comparing to zero.
 */
#define WIFI_EVENT_WAIT(evtGrp, F)                                                       \
    do                                                                                   \
    {                                                                                    \
        uint32_t __flags =                                                               \
        osEventFlagsWait((evtGrp), (F), osFlagsWaitAny, BSP_WIFI_WAIT_PERIOD_MS);        \
        if((__flags & osFlagsError) != 0U)                                               \
        {                                                                                \
            WIFI_LOGE("WiFi event wait timeout/error: 0x%08lX", (unsigned long)__flags); \
            return BSP_ERR_STS_TIMEOUT;                                                  \
        }                                                                                \
    } while(0)

/**
 * @brief Wait for any of the specified flags (non-macro, returns flags).
 *
 * Used when caller needs to inspect which flag was set.
 */
#define WIFI_EVENT_WAIT_ANY(evtGrp, F) \
    osEventFlagsWait((evtGrp), (F), osFlagsWaitAny, BSP_WIFI_WAIT_PERIOD_MS)

/* -------------------------------------------------- */
/* INTERNAL PLATFORM CONTEXT                          */
/* -------------------------------------------------- */

/**
 * @brief ESP32 platform-specific Wi-Fi context.
 *
 * Stored statically (single Wi-Fi instance per ESP32).
 * Referenced via handle->platform_ctx opaque pointer.
 */
typedef struct
{
    esp_netif_t* sta_netif;                   /**< STA network interface. */
    esp_netif_t* ap_netif;                    /**< AP network interface. */

    esp_event_handler_instance_t wifi_any_id; /**< Wi-Fi event handler instance. */
    esp_event_handler_instance_t ip_got_ip; /**< IP event handler instance. */

    osEventFlagsId_t evt_flags; /**< Internal synchronization flags. */

    bspWifiMode_t current_mode; /**< Active Wi-Fi mode. */
    uint8_t initialized;        /**< Platform initialization flag. */
} espWifiPlatformCtx_t;

/* -------------------------------------------------- */
/* STATIC STATE                                       */
/* -------------------------------------------------- */

/** @brief Singleton platform context. */
static espWifiPlatformCtx_t g_espWifiCtx;

/** @brief Tracks one-time event loop initialization. */
static bool s_eventLoopInited = false;

/* -------------------------------------------------- */
/* FORWARD DECLARATIONS (PLATFORM LAYER)              */
/* -------------------------------------------------- */

static bsp_err_sts_t bsp_platform_wifi_init(bspWifiHandle_t* handle);
static bsp_err_sts_t bsp_platform_wifi_set_mode(bspWifiHandle_t* handle, bspWifiMode_t mode);
static bsp_err_sts_t bsp_platform_wifi_start_sta(bspWifiHandle_t* handle,
                                                 const bspWifiStaConfig_t* params);
static bsp_err_sts_t bsp_platform_wifi_start_ap(bspWifiHandle_t* handle,
                                                const bspWifiApConfig_t* config);
static bsp_err_sts_t bsp_platform_wifi_stop_ap(bspWifiHandle_t* handle);
static bsp_err_sts_t bsp_platform_wifi_disconnect_sta(bspWifiHandle_t* handle);
static bsp_err_sts_t bsp_platform_wifi_get_conn_stations(bspWifiHandle_t* handle,
                                                         bspWifiStationInfo_t* stations,
                                                         uint8_t* numStations,
                                                         uint8_t maxStations);
static bsp_err_sts_t
bsp_platform_wifi_disconn_station(bspWifiHandle_t* handle,
                                  const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN]);
static bsp_err_sts_t bsp_platform_wifi_get_ap_config(bspWifiHandle_t* handle,
                                                     bspWifiApConfig_t* apCfg);
static bsp_err_sts_t bsp_platform_wifi_get_ap_statistics(bspWifiHandle_t* handle,
                                                         bspWifiStatistics_t* stats);
static bsp_err_sts_t bsp_platform_wifi_store_sta_profile(bspWifiHandle_t* handle,
                                                         const bspWifiStaConfig_t* station);
static bsp_err_sts_t bsp_platform_wifi_remove_sta_profile(bspWifiHandle_t* handle,
                                                          const uint8_t* ssid,
                                                          uint8_t ssid_len);
static bsp_err_sts_t bsp_platform_wifi_get_sta_profile(bspWifiHandle_t* handle,
                                                       uint8_t index,
                                                       bspWifiStaConfig_t* sta_cfg);
static bsp_err_sts_t bsp_platform_wifi_start_scan(bspWifiHandle_t* handle);
static bsp_err_sts_t bsp_platform_wifi_get_scan_results(bspWifiHandle_t* handle,
                                                        bspWifiApScanResult_t* results,
                                                        uint8_t max_results,
                                                        uint8_t* out_count);

/* -------------------------------------------------- */
/* HELPER FUNCTIONS                                   */
/* -------------------------------------------------- */

/**
 * @brief Map ESP-IDF station PHY capabilities to BSP PHY mode enum.
 *
 * @param[in] sta  Pointer to ESP-IDF station info.
 *
 * @return Highest supported PHY mode.
 *
 * @par Corner Case
 * - If no PHY flag is set (should not happen in practice),
 *   returns eBSPWifiPhyNone as safe default.
 */
static bspWifiPhyMode_t esp_map_phy_mode(const wifi_sta_info_t* sta)
{
    bspWifiPhyMode_t mode = eBSPWifiPhyNone;

    if(sta == NULL)
    {
        return mode;
    }

    /* Check in order of preference (newest standard first) */
    if(sta->phy_11ax != 0U)
    {
        mode = eBSPWifiPhy11ax;
    }
    else if(sta->phy_11ac != 0U)
    {
        mode = eBSPWifiPhy11ac;
    }
    else if(sta->phy_11n != 0U)
    {
        mode = eBSPWifiPhy11n;
    }
    else if(sta->phy_11g != 0U)
    {
        mode = eBSPWifiPhy11g;
    }
    else if(sta->phy_11b != 0U)
    {
        mode = eBSPWifiPhy11b;
    }
    else if(sta->phy_lr != 0U)
    {
        mode = eBSPWifiPhyLR;
    }
    else
    {
        /* No PHY flag set - unexpected but handled gracefully */
        mode = eBSPWifiPhyNone;
    }

    return mode;
}

/**
 * @brief Map ESP-IDF auth mode to BSP security enum.
 *
 * @param[in] mode  ESP-IDF authentication mode.
 *
 * @return Corresponding BSP security type.
 */
static bspWifiSecurity_t esp_authmode_to_bsp(wifi_auth_mode_t mode)
{
    bspWifiSecurity_t mapped;

    switch(mode)
    {
        case WIFI_AUTH_OPEN:
            mapped = eWiFiSecurityOpen;
            break;

        case WIFI_AUTH_WEP:
            mapped = eWiFiSecurityWEP;
            break;

        case WIFI_AUTH_WPA_PSK:
            mapped = eWiFiSecurityWPA;
            break;

        case WIFI_AUTH_WPA2_PSK:
            mapped = eWiFiSecurityWPA2;
            break;

        case WIFI_AUTH_WPA_WPA2_PSK:
            mapped = eWiFiSecurityWPA2;
            break;

        case WIFI_AUTH_WPA2_ENTERPRISE:
            mapped = eWiFiSecurityWPA2_ent;
            break;

        case WIFI_AUTH_WPA3_PSK:
            mapped = eWiFiSecurityWPA3;
            break;

        case WIFI_AUTH_WPA2_WPA3_PSK:
            mapped = eWiFiSecurityWPA3;
            break;

        default:
            mapped = eWiFiSecurityNotSupported;
            break;
    }

    return mapped;
}

/* -------------------------------------------------- */
/* ESP EVENT HANDLER                                  */
/* -------------------------------------------------- */

/**
 * @brief ESP-IDF Wi-Fi/IP event handler bridge.
 *
 * Maps ESP-IDF events to internal osEventFlags for BSP synchronization.
 *
 * @par Thread Safety
 * - This runs in ESP system event task context.
 * - Only osEventFlagsSet/Clear are called (both are thread-safe in CMSIS).
 * - Handle state fields (wifi_started, sta_connected, etc.) are written
 *   atomically (uint8_t) without mutex. This is acceptable because:
 *   1. They are only informational flags.
 *   2. BSP APIs always check authoritative state via event flags.
 *   3. Single-writer (this handler) guarantees no torn writes on uint8_t.
 *
 * @par Corner Cases
 * - NULL handle or platform_ctx: silently returns (defensive).
 * - Unknown event: ignored.
 * - Multiple disconnect reasons: all map to BSP_WIFI_F_STA_DISCONNECTED
 *   with auth_failed flag set for authentication-related failures.
 *
 * @param[in] arg         User argument (bspWifiHandle_t*).
 * @param[in] event_base  ESP event base (WIFI_EVENT or IP_EVENT).
 * @param[in] event_id    Specific event ID.
 * @param[in] event_data  Event-specific data pointer.
 */
static void
espWifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    bspWifiHandle_t* handle = (bspWifiHandle_t*)arg;

    /* Defensive: validate handle and context */
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(ctx->evt_flags == NULL)
    {
        return;
    }

    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                WIFI_LOGI("WIFI_EVENT_STA_START");
                handle->wifi_started = 1U;
                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_STOPPED);
                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_STA_STARTED);
                break;
            }

            case WIFI_EVENT_STA_STOP:
            {
                WIFI_LOGI("WIFI_EVENT_STA_STOP");
                handle->wifi_started = 0U;
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_STA_STOPPED);
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED:
            {
                wifi_event_sta_disconnected_t* disconn =
                (wifi_event_sta_disconnected_t*)event_data;

                WIFI_LOGW("STA disconnected, reason=%d",
                          (disconn != NULL) ? (int)disconn->reason : -1);

                handle->sta_connected = 0U;
                handle->auth_failed   = 0U;

                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED);
                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_IP_READY);
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);

                /*
                 * CORNER CASE: Classify disconnect reason.
                 * Authentication-related failures set auth_failed flag
                 * so BSP API can distinguish between signal loss and
                 * wrong credentials.
                 */
                if(disconn != NULL)
                {
                    switch(disconn->reason)
                    {
                        case WIFI_REASON_AUTH_EXPIRE:
                        case WIFI_REASON_ASSOC_EXPIRE:
                        case WIFI_REASON_AUTH_FAIL:
                        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                        case WIFI_REASON_HANDSHAKE_TIMEOUT:
                        case WIFI_REASON_NO_AP_FOUND:
                            handle->auth_failed = 1U;
                            break;

                        default:
                            handle->auth_failed = 1U;
                            break;
                    }
                }
                break;
            }

            case WIFI_EVENT_SCAN_DONE:
            {
                WIFI_LOGI("WIFI_EVENT_SCAN_DONE");
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_SCAN_DONE);
                break;
            }

            case WIFI_EVENT_AP_START:
            {
                WIFI_LOGI("WIFI_EVENT_AP_START");
                handle->ap_started = 1U;
                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_AP_STOPPED);
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_AP_STARTED);
                break;
            }

            case WIFI_EVENT_AP_STOP:
            {
                WIFI_LOGI("WIFI_EVENT_AP_STOP");
                handle->ap_started = 0U;
                (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_AP_STARTED);
                (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_AP_STOPPED);
                break;
            }

            default:
            {
                /* Unhandled Wi-Fi event - ignore */
                break;
            }
        }
    }
    else if(event_base == IP_EVENT)
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            WIFI_LOGI("IP_EVENT_STA_GOT_IP");
            handle->sta_connected = 1U;
            (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
            (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED);
            (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_IP_READY);
        }
        else
        {
            /*
             * CORNER CASE: IP lost event (e.g., DHCP renewal failure).
             * Treat as disconnection from IP perspective.
             */
            (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED);
            (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_IP_READY);
            (void)osEventFlagsSet(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);
        }
    }
    else
    {
        /* Unknown event base - ignore */
    }
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: INIT                               */
/* -------------------------------------------------- */

/**
 * @brief Initialize ESP32 Wi-Fi platform layer.
 *
 * Performs one-time initialization of:
 * - NVS flash (required by ESP Wi-Fi for calibration data)
 * - TCP/IP adapter (esp_netif)
 * - Default event loop
 * - Wi-Fi driver with default configuration
 * - Internal event flags for synchronization
 * - ESP event handler registration
 *
 * @param[in,out] handle  BSP Wi-Fi handle.
 *
 * @retval BSP_ERR_STS_OK            Success.
 * @retval BSP_ERR_STS_INVALID_PARAM NULL handle.
 * @retval BSP_ERR_STS_ALREADY_INIT  Already initialized.
 * @retval BSP_ERR_STS_FAIL          ESP-IDF initialization error.
 *
 * @par Corner Cases
 * - NVS partition full/corrupt: erases and reinitializes (data loss accepted
 *   as this is a recovery path during first boot or flash corruption).
 * - Double init: returns ALREADY_INIT without side effects.
 * - Event loop already created by another component: esp_event_loop_create_default()
 *   returns ESP_ERR_INVALID_STATE which we treat as success.
 */
static bsp_err_sts_t bsp_platform_wifi_init(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    esp_err_t espErr;
    espWifiPlatformCtx_t* ctx = &g_espWifiCtx;

    if(ctx->initialized != 0U)
    {
        handle->platform_ctx = ctx;
        return BSP_ERR_STS_ALREADY_INIT;
    }

    handle->platform_ctx = ctx;

    /* ---- NVS initialization ---- */
    esp_err_t nvsRet = nvs_flash_init();
    if((nvsRet == ESP_ERR_NVS_NO_FREE_PAGES) || (nvsRet == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        /*
         * CORNER CASE: NVS partition corrupted or version mismatch.
         * Erase and reinitialize. Stored Wi-Fi profiles will be lost.
         * This is acceptable as a recovery mechanism.
         */
        (void)nvs_flash_erase();
        nvsRet = nvs_flash_init();
    }

    if(nvsRet != ESP_OK)
    {
        WIFI_LOGE("NVS init failed: %d", (int)nvsRet);
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Event loop and netif (one-time) ---- */
    if(s_eventLoopInited == false)
    {
        espErr = esp_netif_init();
        if(espErr != ESP_OK)
        {
            WIFI_LOGE("esp_netif_init failed: %d", (int)espErr);
            return BSP_ERR_STS_FAIL;
        }

        espErr = esp_event_loop_create_default();
        if((espErr != ESP_OK) && (espErr != ESP_ERR_INVALID_STATE))
        {
            /*
             * CORNER CASE: ESP_ERR_INVALID_STATE means event loop already exists
             * (created by another component). This is not an error for us.
             */
            WIFI_LOGE("event loop create failed: %d", (int)espErr);
            return BSP_ERR_STS_FAIL;
        }

        ctx->sta_netif = esp_netif_create_default_wifi_sta();
        ctx->ap_netif  = esp_netif_create_default_wifi_ap();

        if((ctx->sta_netif == NULL) || (ctx->ap_netif == NULL))
        {
            WIFI_LOGE("netif create failed");
            return BSP_ERR_STS_FAIL;
        }

        s_eventLoopInited = true;
    }

    /* ---- Wi-Fi driver initialization ---- */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    espErr = esp_wifi_init(&cfg);
    if(espErr != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_init failed: %d", (int)espErr);
        return BSP_ERR_STS_FAIL;
    }

    /* Use RAM storage (profiles managed by BSP NVS layer) */
    (void)esp_wifi_set_storage(WIFI_STORAGE_RAM);

    /* ---- Event flags ---- */
    ctx->evt_flags = osEventFlagsNew(NULL);
    if(ctx->evt_flags == NULL)
    {
        WIFI_LOGE("Event flags creation failed");
        (void)esp_wifi_deinit();
        return BSP_ERR_STS_FAIL;
    }

    /* ---- Register event handlers ---- */
    espErr = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, espWifiEventHandler,
                                                 handle, &ctx->wifi_any_id);

    if(espErr != ESP_OK)
    {
        WIFI_LOGE("WiFi event handler register failed: %d", (int)espErr);
        (void)osEventFlagsDelete(ctx->evt_flags);
        ctx->evt_flags = NULL;
        (void)esp_wifi_deinit();
        return BSP_ERR_STS_FAIL;
    }

    espErr = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, espWifiEventHandler,
                                                 handle, &ctx->ip_got_ip);

    if(espErr != ESP_OK)
    {
        WIFI_LOGE("IP event handler register failed: %d", (int)espErr);
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                    ctx->wifi_any_id);
        ctx->wifi_any_id = NULL;
        (void)osEventFlagsDelete(ctx->evt_flags);
        ctx->evt_flags = NULL;
        (void)esp_wifi_deinit();
        return BSP_ERR_STS_FAIL;
    }

    ctx->initialized = 1U;

    WIFI_LOGI("Platform Wi-Fi init OK");
    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: MODE                               */
/* -------------------------------------------------- */

/**
 * @brief Set ESP-IDF Wi-Fi operating mode.
 *
 * @param[in] handle  BSP handle.
 * @param[in] mode    Desired Wi-Fi mode.
 *
 * @retval BSP_ERR_STS_OK           Mode set.
 * @retval BSP_ERR_STS_UNSUPPORTED  P2P mode not supported on ESP32.
 * @retval BSP_ERR_STS_FAIL         ESP-IDF error.
 */
static bsp_err_sts_t bsp_platform_wifi_set_mode(bspWifiHandle_t* handle, bspWifiMode_t mode)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

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
        WIFI_LOGE("esp_wifi_set_mode failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    ctx->current_mode = mode;
    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: STA CONNECT                        */
/* -------------------------------------------------- */

/**
 * @brief Configure and connect STA to an AP.
 *
 * @param[in] handle  BSP handle.
 * @param[in] params  STA connection parameters.
 *
 * @retval BSP_ERR_STS_OK           Connected successfully (IP obtained).
 * @retval BSP_ERR_STS_CONN_FAILED  Disconnected during attempt.
 * @retval BSP_ERR_STS_AUTH_FAIL    Authentication failure.
 * @retval BSP_ERR_STS_TIMEOUT      No event within timeout.
 * @retval BSP_ERR_STS_FAIL         ESP-IDF error.
 *
 * @par Corner Cases
 * - Open network: password not set (zero-length).
 * - SSID with embedded nulls: handled via ssidLength (not strlen).
 * - Timeout waiting for connection: caller should retry or report to user.
 * - esp_wifi_start() called when already started: ESP-IDF returns
 *   ESP_ERR_WIFI_NOT_INIT only if truly not initialized.
 */
static bsp_err_sts_t bsp_platform_wifi_start_sta(bspWifiHandle_t* handle,
                                                 const bspWifiStaConfig_t* params)
{
    if((handle == NULL) || (params == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if((ctx == NULL) || (ctx->evt_flags == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    wifi_config_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));

    /* Copy SSID (may contain binary data, use length not strlen) */
    if(params->ssidLength > sizeof(cfg.sta.ssid))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    (void)memcpy(cfg.sta.ssid, params->ssid, params->ssidLength);

    /* Copy password for secured networks */
    if(params->security != eWiFiSecurityOpen)
    {
        if(params->passwordLength > sizeof(cfg.sta.password))
        {
            return BSP_ERR_STS_INVALID_PARAM;
        }

        (void)memcpy(cfg.sta.password, params->password, params->passwordLength);
    }

    /* Set channel hint if provided */
    if(params->channel != 0U)
    {
        cfg.sta.channel = params->channel;
    }

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_set_config(STA) failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_start();
    if((err != ESP_OK) && (err != ESP_ERR_WIFI_NOT_INIT))
    {
        WIFI_LOGE("esp_wifi_start failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_connect();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_connect failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    /*
     * Wait for connection outcome.
     * Three possible outcomes:
     * 1. BSP_WIFI_F_STA_CONNECTED - IP obtained, success.
     * 2. BSP_WIFI_F_STA_DISCONNECTED - Connection failed.
     * 3. Timeout - No response from AP within wait period.
     */
    uint32_t flags = WIFI_EVENT_WAIT_ANY(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED |
                                                         BSP_WIFI_F_STA_DISCONNECTED);

    if((flags & osFlagsError) != 0U)
    {
        WIFI_LOGE("STA connect timeout");
        return BSP_ERR_STS_TIMEOUT;
    }

    if((flags & BSP_WIFI_F_STA_CONNECTED) != 0U)
    {
        WIFI_LOGI("STA connected with IP");
        return BSP_ERR_STS_OK;
    }

    if((flags & BSP_WIFI_F_STA_DISCONNECTED) != 0U)
    {
        if(handle->auth_failed != 0U)
        {
            WIFI_LOGE("STA auth failed");
            return BSP_ERR_STS_AUTH_FAIL;
        }

        WIFI_LOGE("STA connection failed");
        return BSP_ERR_STS_CONN_FAILED;
    }

    return BSP_ERR_STS_FAIL;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: AP START                           */
/* -------------------------------------------------- */

/**
 * @brief Configure and start Access Point.
 *
 * @param[in] handle  BSP handle.
 * @param[in] config  AP configuration.
 *
 * @retval BSP_ERR_STS_OK       AP started.
 * @retval BSP_ERR_STS_TIMEOUT  AP_STARTED event not received.
 * @retval BSP_ERR_STS_FAIL     ESP-IDF error.
 *
 * @par Corner Cases
 * - Open security: password ignored, authmode set to OPEN.
 * - maxConnections clamped to hardware limit by ESP-IDF internally.
 * - Channel 0: ESP-IDF auto-selects.
 */
static bsp_err_sts_t bsp_platform_wifi_start_ap(bspWifiHandle_t* handle,
                                                const bspWifiApConfig_t* config)
{
    if((handle == NULL) || (config == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if((ctx == NULL) || (ctx->evt_flags == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    wifi_config_t ap;
    (void)memset(&ap, 0, sizeof(ap));

    /* SSID */
    uint8_t ssidLen = config->ssidLength;
    if(ssidLen > sizeof(ap.ap.ssid))
    {
        ssidLen = (uint8_t)sizeof(ap.ap.ssid);
    }
    (void)memcpy(ap.ap.ssid, config->ssid, ssidLen);
    ap.ap.ssid_len = ssidLen;

    /* Channel and connections */
    ap.ap.channel        = config->channel;
    ap.ap.max_connection = config->maxConnections;
    ap.ap.ssid_hidden    = config->ssidHidden;

    /* Security */
    if(config->security == eWiFiSecurityOpen)
    {
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        uint8_t pwdLen = config->passwordLength;
        if(pwdLen > sizeof(ap.ap.password))
        {
            pwdLen = (uint8_t)sizeof(ap.ap.password);
        }
        (void)memcpy(ap.ap.password, config->password, pwdLen);
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_set_config(AP) failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    err = esp_wifi_start();
    if((err != ESP_OK) && (err != ESP_ERR_WIFI_NOT_INIT))
    {
        WIFI_LOGE("esp_wifi_start(AP) failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    /* Wait for AP started confirmation */
    WIFI_EVENT_WAIT(ctx->evt_flags, BSP_WIFI_F_AP_STARTED);

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: AP STOP                            */
/* -------------------------------------------------- */

/**
 * @brief Stop Access Point.
 *
 * @param[in] handle  BSP handle.
 *
 * @retval BSP_ERR_STS_OK            AP stopped.
 * @retval BSP_ERR_STS_INVALID_STATE AP not running.
 * @retval BSP_ERR_STS_TIMEOUT       Stop event timeout.
 * @retval BSP_ERR_STS_FAIL          ESP-IDF error.
 */
static bsp_err_sts_t bsp_platform_wifi_stop_ap(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(ctx->initialized == 0U)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    if((ctx->current_mode != eWiFiModeAP) && (ctx->current_mode != eWiFiModeAPStation))
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    esp_err_t err = esp_wifi_stop();
    if((err != ESP_OK) && (err != ESP_ERR_WIFI_NOT_STARTED))
    {
        WIFI_LOGE("esp_wifi_stop failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    WIFI_EVENT_WAIT(ctx->evt_flags, BSP_WIFI_F_AP_STOPPED);

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: STA DISCONNECT                     */
/* -------------------------------------------------- */

/**
 * @brief Disconnect STA from AP.
 *
 * @param[in] handle  BSP handle.
 *
 * @retval BSP_ERR_STS_OK       Disconnected.
 * @retval BSP_ERR_STS_TIMEOUT  Disconnect event timeout.
 * @retval BSP_ERR_STS_FAIL     ESP-IDF error.
 *
 * @par Corner Cases
 * - Already disconnected: returns OK immediately (no-op).
 * - esp_wifi_disconnect() may trigger immediate DISCONNECTED event
 *   before WIFI_EVENT_WAIT returns. The event flag is already set
 *   so wait returns immediately.
 */
static bsp_err_sts_t bsp_platform_wifi_disconnect_sta(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if(ctx->initialized == 0U)
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    /* If already not connected, no-op */
    if(handle->sta_connected == 0U)
    {
        return BSP_ERR_STS_OK;
    }

    esp_err_t err = esp_wifi_disconnect();
    if(err != ESP_OK)
    {
        WIFI_LOGE("esp_wifi_disconnect failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    WIFI_EVENT_WAIT(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: GET CONNECTED STATIONS             */
/* -------------------------------------------------- */

/**
 * @brief Get list of stations connected to AP.
 *
 * @param[in]  handle       BSP handle.
 * @param[out] stations     Output station array.
 * @param[out] numStations  Number of stations written.
 * @param[in]  maxStations  Maximum entries in stations array.
 *
 * @retval BSP_ERR_STS_OK            Success.
 * @retval BSP_ERR_STS_NOT_READY     AP not active.
 * @retval BSP_ERR_STS_BUF_TOO_SMALL More stations than buffer allows.
 * @retval BSP_ERR_STS_FAIL          ESP-IDF error.
 */
static bsp_err_sts_t bsp_platform_wifi_get_conn_stations(bspWifiHandle_t* handle,
                                                         bspWifiStationInfo_t* stations,
                                                         uint8_t* numStations,
                                                         uint8_t maxStations)
{
    if((handle == NULL) || (handle->platform_ctx == NULL) ||
       (stations == NULL) || (numStations == NULL) || (maxStations == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if((ctx->current_mode != eWiFiModeAP) && (ctx->current_mode != eWiFiModeAPStation))
    {
        *numStations = 0U;
        return BSP_ERR_STS_NOT_READY;
    }

    wifi_sta_list_t sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta_list);
    if(err != ESP_OK)
    {
        *numStations = 0U;
        return BSP_ERR_STS_FAIL;
    }

    uint8_t available = (uint8_t)sta_list.num;
    uint8_t toCopy    = (available > maxStations) ? maxStations : available;

    for(uint8_t i = 0U; i < toCopy; i++)
    {
        (void)memcpy(stations[i].mac, sta_list.sta[i].mac, BSP_WIFI_MAC_ADDR_MAX_LEN);
        stations[i].rssi    = (uint8_t)sta_list.sta[i].rssi;
        stations[i].phyMode = esp_map_phy_mode(&sta_list.sta[i]);
    }

    *numStations = toCopy;

    if(available > maxStations)
    {
        return BSP_ERR_STS_BUF_TOO_SMALL;
    }

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: DISCONNECT SPECIFIC STATION        */
/* -------------------------------------------------- */

/**
 * @brief Deauthenticate a specific station from AP by MAC.
 *
 * @param[in] handle  BSP handle.
 * @param[in] mac     MAC address of station to disconnect.
 *
 * @retval BSP_ERR_STS_OK         Station deauthenticated.
 * @retval BSP_ERR_STS_NOT_FOUND  MAC not in connected station list.
 * @retval BSP_ERR_STS_FAIL       ESP-IDF deauth error.
 *
 * @par Corner Cases
 * - Station disconnects between list query and deauth: esp_wifi_deauth_sta
 *   may return error which we report as FAIL. Not critical.
 * - AID (Association ID) is 1-based in ESP-IDF.
 */
static bsp_err_sts_t
bsp_platform_wifi_disconn_station(bspWifiHandle_t* handle,
                                  const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN])
{
    if((handle == NULL) || (handle->platform_ctx == NULL) || (mac == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if((ctx->initialized == 0U) || ((ctx->current_mode != eWiFiModeAP) &&
                                    (ctx->current_mode != eWiFiModeAPStation)))
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    bspWifiStationInfo_t stationList[BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS];
    uint8_t numStations = 0U;

    bsp_err_sts_t sts =
    bsp_platform_wifi_get_conn_stations(handle, stationList, &numStations,
                                        BSP_WIFI_MAX_STA_CONNECTIONS_SUPPORTS);

    if((sts != BSP_ERR_STS_OK) || (numStations == 0U))
    {
        return (sts != BSP_ERR_STS_OK) ? sts : BSP_ERR_STS_NOT_FOUND;
    }

    for(uint8_t i = 0U; i < numStations; i++)
    {
        if(WIFI_MAC_MATCH(stationList[i].mac, mac))
        {
            /* ESP-IDF AID is 1-based */
            esp_err_t err = esp_wifi_deauth_sta((uint16_t)(i + 1U));
            if(err != ESP_OK)
            {
                WIFI_LOGE("esp_wifi_deauth_sta failed: %d", (int)err);
                return BSP_ERR_STS_FAIL;
            }
            return BSP_ERR_STS_OK;
        }
    }

    return BSP_ERR_STS_NOT_FOUND;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: GET AP CONFIG                      */
/* -------------------------------------------------- */

/**
 * @brief Retrieve current AP configuration from ESP-IDF.
 *
 * @param[in]  handle  BSP handle.
 * @param[out] apCfg   Output AP configuration.
 *
 * @retval BSP_ERR_STS_OK   Success.
 * @retval BSP_ERR_STS_FAIL ESP-IDF error.
 */
static bsp_err_sts_t bsp_platform_wifi_get_ap_config(bspWifiHandle_t* handle,
                                                     bspWifiApConfig_t* apCfg)
{
    if((handle == NULL) || (handle->platform_ctx == NULL) || (apCfg == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    wifi_config_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));

    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &cfg);
    if(err != ESP_OK)
    {
        return BSP_ERR_STS_FAIL;
    }

    (void)memset(apCfg, 0, sizeof(*apCfg));

    /* SSID */
    if(CHECK_VALID_WIFI_SSID_LEN(cfg.ap.ssid_len))
    {
        (void)memcpy(apCfg->ssid, cfg.ap.ssid, cfg.ap.ssid_len);
        apCfg->ssidLength = (uint8_t)cfg.ap.ssid_len;
    }

    /* Password */
    size_t pwdLen = strnlen((const char*)cfg.ap.password, BSP_WIFI_PASSWORD_MAX_LEN);
    if(pwdLen > 0U)
    {
        (void)memcpy(apCfg->password, cfg.ap.password, pwdLen);
        apCfg->passwordLength = (uint8_t)pwdLen;
    }

    apCfg->channel        = cfg.ap.channel;
    apCfg->ssidHidden     = cfg.ap.ssid_hidden;
    apCfg->maxConnections = cfg.ap.max_connection;
    apCfg->security       = esp_authmode_to_bsp(cfg.ap.authmode);

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: AP STATISTICS                      */
/* -------------------------------------------------- */

/**
 * @brief Get AP statistics.
 *
 * @note ESP-IDF does not expose per-AP TX/RX counters via public API.
 *       This returns UNSUPPORTED with zeroed stats.
 */
static bsp_err_sts_t bsp_platform_wifi_get_ap_statistics(bspWifiHandle_t* handle,
                                                         bspWifiStatistics_t* stats)
{
    if((handle == NULL) || (stats == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    (void)memset(stats, 0, sizeof(*stats));

    return BSP_ERR_STS_UNSUPPORTED;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: NVS PROFILE MANAGEMENT             */
/* -------------------------------------------------- */

/**
 * @brief Store STA profile to NVS.
 *
 * @param[in] handle   BSP handle.
 * @param[in] station  STA configuration to store.
 *
 * @retval BSP_ERR_STS_OK      Stored (new or updated existing).
 * @retval BSP_ERR_STS_NO_MEM  Maximum profiles reached.
 * @retval BSP_ERR_STS_FAIL    NVS error.
 *
 * @par Corner Cases
 * - Duplicate SSID: updates existing entry (upsert behavior).
 * - NVS full: returns NO_MEM, existing profiles unaffected.
 * - Power loss during write: NVS commit is atomic per ESP-IDF docs.
 */
static bsp_err_sts_t bsp_platform_wifi_store_sta_profile(bspWifiHandle_t* handle,
                                                         const bspWifiStaConfig_t* station)
{
    if((handle == NULL) || (station == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count = 0U;
    char key[16];

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if(err != ESP_OK)
    {
        WIFI_LOGE("NVS open failed: %d", (int)err);
        return BSP_ERR_STS_FAIL;
    }

    /* Read current profile count */
    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if(err == ESP_ERR_NVS_NOT_FOUND)
    {
        count = 0U;
    }
    else if(err != ESP_OK)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_FAIL;
    }
    else
    {
        /* count read successfully */
    }

    /* Check for existing profile with same SSID (upsert) */
    for(uint8_t i = 0U; i < count; i++)
    {
        bspWifiStaConfig_t stored;
        size_t size = sizeof(stored);

        (void)snprintf(key, sizeof(key), BSP_WIFI_NVS_STA_KEY_FMT, (unsigned)i);

        if(nvs_get_blob(nvs, key, &stored, &size) == ESP_OK)
        {
            if((stored.ssidLength == station->ssidLength) &&
               (memcmp(stored.ssid, station->ssid, station->ssidLength) == 0))
            {
                /* Update existing profile */
                err = nvs_set_blob(nvs, key, station, sizeof(*station));
                if(err == ESP_OK)
                {
                    (void)nvs_commit(nvs);
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

    (void)snprintf(key, sizeof(key), BSP_WIFI_NVS_STA_KEY_FMT, (unsigned)count);

    err = nvs_set_blob(nvs, key, station, sizeof(*station));
    if(err != ESP_OK)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_FAIL;
    }

    (void)nvs_set_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, count + 1U);
    (void)nvs_commit(nvs);
    nvs_close(nvs);

    WIFI_LOGI("Stored STA profile: %.*s", (int)station->ssidLength, station->ssid);
    return BSP_ERR_STS_OK;
}

/**
 * @brief Remove STA profile from NVS by SSID.
 *
 * @param[in] handle    BSP handle.
 * @param[in] ssid      SSID to remove.
 * @param[in] ssid_len  SSID length.
 *
 * @retval BSP_ERR_STS_OK         Profile removed.
 * @retval BSP_ERR_STS_NOT_FOUND  SSID not in stored profiles.
 * @retval BSP_ERR_STS_FAIL       NVS error.
 *
 * @par Corner Cases
 * - Removing middle entry: profiles after it shift left to maintain
 *   contiguous index order. This avoids gaps in NVS key sequence.
 * - Last entry erased explicitly to prevent stale data.
 */
static bsp_err_sts_t bsp_platform_wifi_remove_sta_profile(bspWifiHandle_t* handle,
                                                          const uint8_t* ssid,
                                                          uint8_t ssid_len)
{
    if((handle == NULL) || (ssid == NULL) || (ssid_len == 0U) || (ssid_len > BSP_WIFI_SSID_MAX_LEN))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count        = 0U;
    bool found           = false;
    uint8_t remove_index = 0U;

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if(err != ESP_OK)
    {
        return BSP_ERR_STS_FAIL;
    }

    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if((err != ESP_OK) || (count == 0U))
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NOT_FOUND;
    }

    /* Find matching SSID */
    for(uint8_t i = 0U; i < count; i++)
    {
        bspWifiStaConfig_t cfg;
        size_t size = sizeof(cfg);
        char key[16];

        (void)snprintf(key, sizeof(key), BSP_WIFI_NVS_STA_KEY_FMT, (unsigned)i);

        err = nvs_get_blob(nvs, key, &cfg, &size);
        if(err != ESP_OK)
        {
            continue;
        }

        if((cfg.ssidLength == ssid_len) && (memcmp(cfg.ssid, ssid, ssid_len) == 0))
        {
            found        = true;
            remove_index = i;
            break;
        }
    }

    if(found == false)
    {
        nvs_close(nvs);
        return BSP_ERR_STS_NOT_FOUND;
    }

    /* Shift remaining profiles left to fill gap */
    for(uint8_t i = remove_index; i < (count - 1U); i++)
    {
        bspWifiStaConfig_t next;
        size_t size = sizeof(next);
        char keySrc[16];
        char keyDst[16];

        (void)snprintf(keySrc, sizeof(keySrc), BSP_WIFI_NVS_STA_KEY_FMT,
                       (unsigned)(i + 1U));
        (void)snprintf(keyDst, sizeof(keyDst), BSP_WIFI_NVS_STA_KEY_FMT, (unsigned)i);

        err = nvs_get_blob(nvs, keySrc, &next, &size);
        if(err == ESP_OK)
        {
            (void)nvs_set_blob(nvs, keyDst, &next, sizeof(next));
        }
    }

    /* Erase last entry */
    char lastKey[16];
    (void)snprintf(lastKey, sizeof(lastKey), BSP_WIFI_NVS_STA_KEY_FMT,
                   (unsigned)(count - 1U));
    (void)nvs_erase_key(nvs, lastKey);

    /* Update count */
    (void)nvs_set_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, count - 1U);
    (void)nvs_commit(nvs);
    nvs_close(nvs);

    return BSP_ERR_STS_OK;
}

/**
 * @brief Retrieve stored STA profile by index.
 *
 * @param[in]  handle   BSP handle.
 * @param[in]  index    Profile index (0-based).
 * @param[out] sta_cfg  Output configuration.
 *
 * @retval BSP_ERR_STS_OK         Profile retrieved.
 * @retval BSP_ERR_STS_NOT_FOUND  No profiles stored or index invalid.
 * @retval BSP_ERR_STS_FAIL       NVS read error.
 */
static bsp_err_sts_t bsp_platform_wifi_get_sta_profile(bspWifiHandle_t* handle,
                                                       uint8_t index,
                                                       bspWifiStaConfig_t* sta_cfg)
{
    if((handle == NULL) || (sta_cfg == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    nvs_handle_t nvs;
    esp_err_t err;
    uint8_t count = 0U;

    err = nvs_open(BSP_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if(err != ESP_OK)
    {
        return BSP_ERR_STS_FAIL;
    }

    err = nvs_get_u8(nvs, BSP_WIFI_NVS_STA_COUNT_KEY, &count);
    if((err != ESP_OK) || (count == 0U))
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
    (void)snprintf(key, sizeof(key), BSP_WIFI_NVS_STA_KEY_FMT, (unsigned)index);

    size_t size = sizeof(bspWifiStaConfig_t);
    err         = nvs_get_blob(nvs, key, sta_cfg, &size);
    nvs_close(nvs);

    if((err != ESP_OK) || (size != sizeof(bspWifiStaConfig_t)))
    {
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PLATFORM LAYER: SCAN                               */
/* -------------------------------------------------- */

/**
 * @brief Start Wi-Fi scan (non-blocking).
 *
 * @param[in] handle  BSP handle.
 *
 * @retval BSP_ERR_STS_OK            Scan started.
 * @retval BSP_ERR_STS_INVALID_STATE Not in STA or APSTA mode.
 * @retval BSP_ERR_STS_FAIL          ESP-IDF error.
 */
static bsp_err_sts_t bsp_platform_wifi_start_scan(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    if((ctx->current_mode != eWiFiModeStation) && (ctx->current_mode != eWiFiModeAPStation))
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    wifi_scan_config_t scan_cfg;
    (void)memset(&scan_cfg, 0, sizeof(scan_cfg));
    scan_cfg.show_hidden = true;

    esp_err_t err = esp_wifi_scan_start(&scan_cfg, false);
    return (err == ESP_OK) ? BSP_ERR_STS_OK : BSP_ERR_STS_FAIL;
}

/**
 * @brief Fetch scan results after scan completion.
 *
 * @param[in]  handle       BSP handle.
 * @param[out] results      Output scan result array.
 * @param[in]  max_results  Maximum entries.
 * @param[out] out_count    Actual entries written.
 *
 * @retval BSP_ERR_STS_OK      Results fetched.
 * @retval BSP_ERR_STS_NO_MEM  Allocation failure for temp buffer.
 *
 * @par Corner Cases
 * - Zero APs found: returns OK with *out_count = 0.
 * - More APs than max_results: only max_results returned.
 * - Memory allocation for temp buffer uses heap; freed before return.
 */
static bsp_err_sts_t bsp_platform_wifi_get_scan_results(bspWifiHandle_t* handle,
                                                        bspWifiApScanResult_t* results,
                                                        uint8_t max_results,
                                                        uint8_t* out_count)
{
    (void)handle;

    if((results == NULL) || (out_count == NULL) || (max_results == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    uint16_t ap_count = 0U;
    (void)esp_wifi_scan_get_ap_num(&ap_count);

    if(ap_count == 0U)
    {
        *out_count = 0U;
        return BSP_ERR_STS_OK;
    }

    uint16_t fetch = (ap_count > (uint16_t)max_results) ? (uint16_t)max_results : ap_count;

    wifi_ap_record_t* ap_list =
    (wifi_ap_record_t*)calloc(fetch, sizeof(wifi_ap_record_t));
    if(ap_list == NULL)
    {
        *out_count = 0U;
        return BSP_ERR_STS_NO_MEM;
    }

    (void)esp_wifi_scan_get_ap_records(&fetch, ap_list);

    for(uint16_t i = 0U; i < fetch; i++)
    {
        size_t ssidLen = strnlen((const char*)ap_list[i].ssid, BSP_WIFI_SSID_MAX_LEN);
        results[i].ssidLength = (uint8_t)ssidLen;
        (void)memcpy(results[i].ssid, ap_list[i].ssid, ssidLen);
        (void)memcpy(results[i].bssid, ap_list[i].bssid, BSP_WIFI_MAC_ADDR_MAX_LEN);
        results[i].rssi     = ap_list[i].rssi;
        results[i].channel  = ap_list[i].primary;
        results[i].security = esp_authmode_to_bsp(ap_list[i].authmode);
    }

    *out_count = (uint8_t)fetch;
    free(ap_list);

    return BSP_ERR_STS_OK;
}

/* -------------------------------------------------- */
/* PUBLIC API                                         */
/* -------------------------------------------------- */

/**
 * @brief Initialize BSP Wi-Fi subsystem.
 *
 * Creates RTOS primitives and initializes platform layer.
 *
 * @param[in,out] handle  BSP Wi-Fi handle (caller-allocated).
 *
 * @retval BSP_ERR_STS_OK      Initialized.
 * @retval BSP_ERR_STS_NO_MEM  RTOS resource creation failed.
 * @retval BSP_ERR_STS_FAIL    Platform init failed.
 */
bsp_err_sts_t bspWifiInit(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Clear handle to known state */
    (void)memset(handle, 0, sizeof(bspWifiHandle_t));

    handle->lock = osMutexNew(&g_bspMutexCfg[BSP_MUTEX_OWNER_WIFI].attr);
    if(handle->lock == NULL)
    {
        WIFI_LOGE("Mutex creation failed");
        return BSP_ERR_STS_NO_MEM;
    }

    /* Create application event queue */
    g_bspQueueCfg[BSP_WIFI_EVENT_QUEUE].msg_size_bytes = sizeof(bspWifiContext_t);

    handle->evt_q = osMessageQueueNew(g_bspQueueCfg[BSP_WIFI_EVENT_QUEUE].msg_count,
                                      sizeof(bspWifiContext_t),
                                      &g_bspQueueCfg[BSP_WIFI_EVENT_QUEUE].attr);

    if(handle->evt_q == NULL)
    {
        WIFI_LOGE("Event queue creation failed");
        (void)osMutexDelete(handle->lock);
        handle->lock = NULL;
        return BSP_ERR_STS_NO_MEM;
    }

    /* Initialize platform layer */
    bsp_err_sts_t sts = bsp_platform_wifi_init(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Platform init failed: %d", (int)sts);
        (void)osMessageQueueDelete(handle->evt_q);
        handle->evt_q = NULL;
        (void)osMutexDelete(handle->lock);
        handle->lock = NULL;
        return sts;
    }

    return BSP_ERR_STS_OK;
}

/**
 * @brief Deinitialize BSP Wi-Fi subsystem.
 *
 * @param[in,out] handle  BSP Wi-Fi handle.
 *
 * @retval BSP_ERR_STS_OK  Deinitialized.
 */
bsp_err_sts_t bspWifiDeInit(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    /* Stop Wi-Fi if running */
    if(handle->wifi_started != 0U)
    {
        (void)esp_wifi_stop();
    }

    WIFI_LOCK(handle);

    /* Unregister event handlers */
    if(ctx->wifi_any_id != NULL)
    {
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                    ctx->wifi_any_id);
        ctx->wifi_any_id = NULL;
    }

    if(ctx->ip_got_ip != NULL)
    {
        (void)esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                    ctx->ip_got_ip);
        ctx->ip_got_ip = NULL;
    }

    /* Deinit Wi-Fi driver */
    (void)esp_wifi_deinit();

    /* Destroy netifs */
    if(ctx->sta_netif != NULL)
    {
        esp_netif_destroy(ctx->sta_netif);
        ctx->sta_netif = NULL;
    }

    if(ctx->ap_netif != NULL)
    {
        esp_netif_destroy(ctx->ap_netif);
        ctx->ap_netif = NULL;
    }

    /* Delete event flags */
    if(ctx->evt_flags != NULL)
    {
        (void)osEventFlagsDelete(ctx->evt_flags);
        ctx->evt_flags = NULL;
    }

    /* Reset state */
    s_eventLoopInited        = false;
    handle->wifi_started     = 0U;
    handle->sta_connected    = 0U;
    handle->ap_started       = 0U;
    handle->auth_failed      = 0U;
    handle->scan_in_progress = 0U;
    handle->platform_ctx     = NULL;

    (void)memset(ctx, 0, sizeof(*ctx));

    WIFI_UNLOCK(handle);

    /* Delete event queue */
    if(handle->evt_q != NULL)
    {
        (void)osMessageQueueDelete(handle->evt_q);
        handle->evt_q = NULL;
    }

    /* Delete mutex last */
    if(handle->lock != NULL)
    {
        (void)osMutexDelete(handle->lock);
        handle->lock = NULL;
    }

    return BSP_ERR_STS_OK;
}

/**
 * @brief Start Wi-Fi in specified mode.
 */
bsp_err_sts_t bspWifiOn(bspWifiHandle_t* handle, bspWifiMode_t mode, void* pvParam)
{
    bsp_err_sts_t sts;

    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(mode >= eWiFiModeMax)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_PARAM;
    }

    /* Set mode */
    sts = bsp_platform_wifi_set_mode(handle, mode);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Set mode failed: %d", (int)sts);
        WIFI_UNLOCK(handle);
        return sts;
    }

    /* Start based on mode */
    switch(mode)
    {
        case eWiFiModeStation:
        {
            if(pvParam == NULL)
            {
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
                WIFI_UNLOCK(handle);
                return BSP_ERR_STS_INVALID_PARAM;
            }

            const bspWifiApStaConfig_t* cfg = (const bspWifiApStaConfig_t*)pvParam;

            sts = bsp_platform_wifi_start_ap(handle, &cfg->apConfig);
            if(sts == BSP_ERR_STS_OK)
            {
                sts = bsp_platform_wifi_start_sta(handle, &cfg->staConfig);
            }
            break;
        }

        case eWiFiModeP2P:
            sts = BSP_ERR_STS_UNSUPPORTED;
            break;

        default:
            sts = BSP_ERR_STS_INVALID_PARAM;
            break;
    }

    if(sts == BSP_ERR_STS_OK)
    {
        handle->wifi_started = 1U;
    }

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Stop Wi-Fi and deinitialize.
 */
bsp_err_sts_t bspWifiOff(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->wifi_started == 0U)
    {
        return BSP_ERR_STS_OK;
    }

    bsp_err_sts_t sts = bspWifiDeInit(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_LOGE("Deinit failed: %d", (int)sts);
        return sts;
    }

    WIFI_LOGI("Wi-Fi stopped");
    return BSP_ERR_STS_OK;
}

/**
 * @brief Connect STA to AP using stored context.
 */
bsp_err_sts_t bspWifiConnectToAp(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;

    WIFI_LOCK(handle);

    handle->sta_connected = 0U;
    handle->auth_failed   = 0U;

    (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_CONNECTED | BSP_WIFI_F_STA_DISCONNECTED |
                                            BSP_WIFI_F_AUTH_FAILED);

    bsp_err_sts_t sts =
    bsp_platform_wifi_start_sta(handle, &handle->wifi_context.tEventData.tStationCfg);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Disconnect STA from AP.
 */
bsp_err_sts_t bspWifiDisconnectToAp(bspWifiHandle_t* handle)
{
    if((handle == NULL) || (handle->platform_ctx == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->sta_connected == 0U)
    {
        return BSP_ERR_STS_OK;
    }

    WIFI_LOCK(handle);

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_STA_DISCONNECTED);

    bsp_err_sts_t sts = bsp_platform_wifi_disconnect_sta(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        WIFI_UNLOCK(handle);
        return sts;
    }

    handle->sta_connected = 0U;

    WIFI_UNLOCK(handle);
    return BSP_ERR_STS_OK;
}

/**
 * @brief Start AP.
 */
bsp_err_sts_t bspWifiStartAP(bspWifiHandle_t* handle, const bspWifiApConfig_t* apCfg)
{
    if((handle == NULL) || (apCfg == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->ap_started != 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_OK;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_AP_STARTED | BSP_WIFI_F_AP_STOPPED);

    bsp_err_sts_t sts = bsp_platform_wifi_start_ap(handle, apCfg);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Stop AP.
 *
 * @par Bug Fix
 * - Original code set ap_started = true after stop (was a bug).
 *   Now correctly set to 0 (false).
 */
bsp_err_sts_t bspWifiStopAP(bspWifiHandle_t* handle)
{
    if(handle == NULL)
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->ap_started == 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_OK;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_AP_STARTED | BSP_WIFI_F_AP_STOPPED);

    bsp_err_sts_t sts = bsp_platform_wifi_stop_ap(handle);
    if(sts == BSP_ERR_STS_OK)
    {
        handle->ap_started = 0U; /* BUG FIX: was incorrectly set to true */
    }

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Get connected stations list.
 */
bsp_err_sts_t bspWifiGetConnStations(bspWifiHandle_t* handle,
                                     bspWifiStationInfo_t* stations,
                                     uint8_t* numStations,
                                     uint8_t maxStations)
{
    if((handle == NULL) || (stations == NULL) || (numStations == NULL) || (maxStations == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->ap_started == 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_READY;
    }

    bsp_err_sts_t sts =
    bsp_platform_wifi_get_conn_stations(handle, stations, numStations, maxStations);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Disconnect specific station from AP.
 */
bsp_err_sts_t bspWifiDisconnectStation(bspWifiHandle_t* handle,
                                       const uint8_t mac[BSP_WIFI_MAC_ADDR_MAX_LEN])
{
    if((handle == NULL) || (mac == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->ap_started == 0U)
    {
        return BSP_ERR_STS_INVALID_STATE;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_disconn_station(handle, mac);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Get current AP configuration.
 */
bsp_err_sts_t bspWifiGetAPConfig(bspWifiHandle_t* handle, bspWifiApConfig_t* apCfg)
{
    if((handle == NULL) || (apCfg == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->ap_started == 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_READY;
    }

    bsp_err_sts_t sts = bsp_platform_wifi_get_ap_config(handle, apCfg);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Get AP statistics.
 */
bsp_err_sts_t bspWifiGetAPStatistics(bspWifiHandle_t* handle, bspWifiStatistics_t* stats)
{
    if((handle == NULL) || (stats == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->ap_started == 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_NOT_READY;
    }

    bsp_err_sts_t sts = bsp_platform_wifi_get_ap_statistics(handle, stats);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Store STA profile.
 */
bsp_err_sts_t bspWifiStoreStationProfile(bspWifiHandle_t* handle, bspWifiStaConfig_t* station)
{
    if((handle == NULL) || (station == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_store_sta_profile(handle, station);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Get stored STA profile by index.
 */
bsp_err_sts_t
bspWifiGetStationProfile(bspWifiHandle_t* handle, uint8_t index, bspWifiStaConfig_t* sta_cfg)
{
    if((handle == NULL) || (sta_cfg == NULL))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->platform_ctx == NULL)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_get_sta_profile(handle, index, sta_cfg);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Remove stored STA profile by SSID.
 */
bsp_err_sts_t
bspWifiRemoveStationProfile(bspWifiHandle_t* handle, const uint8_t* ssid, uint8_t ssid_len)
{
    if((handle == NULL) || (ssid == NULL) || (ssid_len == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    if(handle->platform_ctx == NULL)
    {
        return BSP_ERR_STS_NOT_INIT;
    }

    WIFI_LOCK(handle);

    bsp_err_sts_t sts = bsp_platform_wifi_remove_sta_profile(handle, ssid, ssid_len);

    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Start Wi-Fi scan and return results.
 *
 * @par Corner Cases
 * - Scan already in progress: returns BUSY.
 * - Wi-Fi not started: returns INVALID_STATE.
 * - Scan timeout: WIFI_EVENT_WAIT returns TIMEOUT.
 * - Zero results: returns OK with empty array.
 */
bsp_err_sts_t bspWifiStartScan(bspWifiHandle_t* handle, bspWifiApScanResult_t* results, uint8_t max_results)
{
    if((handle == NULL) || (results == NULL) || (max_results == 0U))
    {
        return BSP_ERR_STS_INVALID_PARAM;
    }

    WIFI_LOCK(handle);

    if(handle->wifi_started == 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_INVALID_STATE;
    }

    if(handle->scan_in_progress != 0U)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_BUSY;
    }

    espWifiPlatformCtx_t* ctx = (espWifiPlatformCtx_t*)handle->platform_ctx;
    if(ctx == NULL)
    {
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_FAIL;
    }

    handle->scan_in_progress = 1U;

    /* Clear stale scan done flag */
    (void)osEventFlagsClear(ctx->evt_flags, BSP_WIFI_F_SCAN_DONE);

    bsp_err_sts_t sts = bsp_platform_wifi_start_scan(handle);
    if(sts != BSP_ERR_STS_OK)
    {
        handle->scan_in_progress = 0U;
        WIFI_UNLOCK(handle);
        return sts;
    }

    /* Wait for scan completion */
    uint32_t flags = WIFI_EVENT_WAIT_ANY(ctx->evt_flags, BSP_WIFI_F_SCAN_DONE);
    if((flags & osFlagsError) != 0U)
    {
        handle->scan_in_progress = 0U;
        WIFI_UNLOCK(handle);
        return BSP_ERR_STS_TIMEOUT;
    }

    /* Fetch results */
    uint8_t out_count = 0U;
    sts = bsp_platform_wifi_get_scan_results(handle, results, max_results, &out_count);

    handle->scan_in_progress = 0U;
    WIFI_UNLOCK(handle);
    return sts;
}

/**
 * @brief Reset Wi-Fi (not supported on ESP32).
 */
bsp_err_sts_t bspWifiReset(bspWifiHandle_t* handle)
{
    (void)handle;
    return BSP_ERR_STS_UNSUPPORTED;
}