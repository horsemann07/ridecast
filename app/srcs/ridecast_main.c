/**
 * @file    ridecast_main.c
 * @brief   RideCast application entry point.
 *
 * WIRING:
 *
 *   ┌────────┐  UART bytes   ┌───────────┐  rc_frame   ┌───────────┐
 *   │  Host  │ ────────────> │ uart_app  │ ──────────> │ wifi_app  │
 *   │        │               │  RX task  │             │   task    │
 *   │        │               │  decode   │             │           │
 *   │        │               │  dispatch │             │  AP + TCP │
 *   │        │               └───────────┘             │  + Auth   │
 *   │        │                    ▲                     └─────┬─────┘
 *   │        │  UART bytes        │  STATUS/DATA              │
 *   │        │ <──────────────────┴───────────────────────────┘
 *   └────────┘    uart_app TX task (single serialised sender)
 *
 * STARTUP SEQUENCE:
 *   1. uart_app_init()  -> UART hw up, RX callback armed, RX+TX tasks running
 *   2. wifi_app_init()  -> wifi task running, waiting for START cmd
 *   3. Heartbeat from uart_app every 10s -> Host knows RideCast is alive
 *   4. Host sends WIFI_START(ssid, pass) over UART
 *      -> uart_app RX task decodes frame
 *      -> uart_to_wifi_host_frame() called
 *      -> wifi_app_start() posts cmd to wifi task
 *   5. wifi_app brings up AP, TCP, auth
 *      -> each state change calls on_wifi_status()
 *      -> uart_app_send_status() sends STATUS frame to Host
 *   6. Phone connects, auth ok
 *      -> on_wifi_status(eWifiAppStsReady) -> Host notified
 *   7. Phone sends data over TCP
 *      -> on_wifi_rx() called
 *      -> uart_app_send_data() forwards to Host
 *   8. Host sends data for phone
 *      -> uart_app RX task decodes DATA frame
 *      -> uart_to_wifi_host_frame() -> wifi_app_send()
 *
 *
 *
 *
 * main()
 * └─ osKernelStart()
 *      └─ startup_task()
 *           ├─ uart_app_init()
 *           │    ├─ bspUartInit()          - UART hw up
 *           │    ├─ bspUartSetCallback()   - async RX armed
 *           │    ├─ bspUartReadAsync()     - always listening
 *           │    ├─ uart_rx_task started   - decodes frames, heartbeat
 *           │    └─ uart_tx_task started   - serialises all UART sends
 *           │
 *           ├─ wifi_app_init()
 *           │    └─ wapp_task started      - waiting for START cmd
 *           │
 *           └─ uart_app_send_status(READY) - Host told we are alive
 *                     │
 *                     ▼
 *             Host sends WIFI_START(ssid, pass)
 *                     │
 *             uart_rx_task decodes -> uart_to_wifi_host_frame()
 *                     │
 *             wifi_app_start() -> AP up -> TCP -> Auth
 *                     │
 *             each state -> on_wifi_status() -> uart_app_send_status()
 *                     │
 *             Phone connects, auth ok -> Host gets STATUS READY
 *                     │
 *             Phone sends data -> on_wifi_rx() -> uart_app_send_data()
 */

/*
 * =========================================================================
 * Change History:
 * =========================================================================
 *
 * 2026-05-23 - J.Raghav - Initial development
 */

#include "bsp_core.h"
#include "ridecast_main.h"
#include "uart_app.h"
#include "wifi_app.h"
#include "rc_protocol.h"
#include "app_logging.h"

#include <string.h>

/* =========================================================================
 * Board config - adjust pins/port for your hardware
 * ========================================================================= */
#define RC_UART_PORT      (1U)
#define RC_UART_BAUD      BSP_UART_BAUD_115200
#define RC_UART_TX_PIN    (17U)
#define RC_UART_RX_PIN    (16U)
#define RC_UART_FIFO_SIZE (256U)

/* =========================================================================
 * Dummy host test support
 * Set to 1U to auto-send WiFi credentials + WIFI_START after init.
 * ========================================================================= */
#ifndef UART_APP_ENABLE_DUMMY_HOST
    #define UART_APP_ENABLE_DUMMY_HOST (1U)
#endif

#if defined(UART_APP_ENABLE_DUMMY_HOST)
extern bsp_err_sts_t test_host_uart_init(uart_app_t* app);
#endif // UART_APP_ENABLE_DUMMY_HOST


/* =========================================================================
 * Application contexts (static - no heap)
 * ========================================================================= */
static uart_app_t gUartApp;
static wifi_app_t gWifiApp;

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static void uart_to_wifi_host_frame(const rc_frame_t* frame, void* user);
static void on_ble_host_frame(const rc_frame_t* frame, void* user);
static void on_bt_host_frame(const rc_frame_t* frame, void* user);
static void on_wifi_status(wifi_app_sts_t sts, uint32_t arg, void* user);
static void on_wifi_rx(const uint8_t* data, uint16_t len, void* user);

/* =========================================================================
 * uart_to_wifi_host_frame
 *
 * Called by uart_app RX task when a WiFi-interface frame arrives from Host.
 *
 * CMD  START  -> wifi_app_start()
 * CMD  STOP   -> wifi_app_stop()
 * DATA        -> wifi_app_send()  (forward to phone)
 * ========================================================================= */
static void uart_to_wifi_host_frame(const rc_frame_t* frame, void* user)
{
    (void)user;

    if(frame == NULL)
    {
        ALOGE("uart_to_wifi_host_frame: frame is NULL");
        return;
    }

    ALOGD("uart_to_wifi_host_frame: Received frame from Host, type=%u "
          "subcode=%u payloadLen=%u",
          (uint8_t)frame->type, (uint8_t)frame->subcode, (uint16_t)frame->payloadLen);

    switch(frame->type)
    {
        case eRcTypeCmd:
        {
            if((rc_cmd_sub_t)frame->subcode == eRcCmdSubStart)
            {
                ALOGI("uart_to_wifi_host_frame: Received WIFI START cmd from "
                      "Host, "
                      "payloadLen=%u",
                      (uint16_t)frame->payloadLen);
                wifi_app_start_params_t params;

                /* Params must carry SSID(32) + PASS(64) = 96 bytes */
                if((frame->payload == NULL) || (frame->payloadLen < RC_WIFI_START_PARAMS))
                {
                    /* Malformed WIFI START - notify Host */
                    (void)uart_app_send_status(&gUartApp, eRcIntfWifi, eRcStsSubAuthFail);
                    break;
                }

                (void)memset(&params, 0, sizeof(params));
                (void)memcpy(params.ssid, &frame->payload[0U], RC_WIFI_SSID_LEN);
                (void)memcpy(params.pass, &frame->payload[RC_WIFI_SSID_LEN], RC_WIFI_PASS_LEN);

                (void)wifi_app_start(&gWifiApp, &params);
            }
            else if((rc_cmd_sub_t)frame->subcode == eRcCmdSubStop)
            {
                ALOGI(
                "uart_to_wifi_host_frame: Received WIFI STOP cmd from Host");
                (void)wifi_app_stop(&gWifiApp);
            }
            else if((rc_cmd_sub_t)frame->subcode == eRcCmdSubReconnect)
            {
                ALOGI("uart_to_wifi_host_frame: Received WIFI RECONNECT cmd "
                      "from Host");
                /* RECONNECT = STOP then re-START with same params */
                (void)wifi_app_stop(&gWifiApp);
                /* wifi_app will fire eWifiAppStsStopped then go IDLE.
                 * Host must send a new START to wake it.           */
            }
            else
            {
                /* unknown cmd subcode - ignore */
                ALOGW("uart_to_wifi_host_frame: Unknown WIFI CMD subcode %u",
                      (uint8_t)frame->subcode);
            }
            break;
        }

        case eRcTypeData:
        {
            /* Host wants to send data to phone (e.g. config, ack) */
            if((frame->payload != NULL) && (frame->payloadLen > 0U))
            {
                ALOGI("uart_to_wifi_host_frame: Received DATA frame from Host, "
                      "payloadLen=%u",
                      (uint16_t)frame->payloadLen);
                (void)wifi_app_send(&gWifiApp, frame->payload, frame->payloadLen);
            }
            break;
        }

        default:
            /* STATUS / ACK from Host - not expected, ignore */
            ALOGW("uart_to_wifi_host_frame: Unexpected frame type %u from Host",
                  (uint8_t)frame->type);
            break;
    }
}

/* =========================================================================
 * on_ble_host_frame  (placeholder - extend when ble_app is ready)
 * ========================================================================= */
static void on_ble_host_frame(const rc_frame_t* frame, void* user)
{
    (void)frame;
    (void)user;
    /* TODO: ble_app_handle_host_frame() */
}

/* =========================================================================
 * on_bt_host_frame  (placeholder - extend when bt_app is ready)
 * ========================================================================= */
static void on_bt_host_frame(const rc_frame_t* frame, void* user)
{
    (void)frame;
    (void)user;
    /* TODO: bt_app_handle_host_frame() */
}

/* =========================================================================
 * on_wifi_status
 *
 * Called by wifi_app task on every state change.
 * Maps wifi_app status -> rc_protocol STATUS subcode -> sends to Host.
 * ========================================================================= */
static void on_wifi_status(wifi_app_sts_t sts, uint32_t arg, void* user)
{
    rc_sts_sub_t sub;

    (void)arg;
    (void)user;

    switch(sts)
    {
        case eWifiAppStsWifiStarting:
            sub = eRcStsSubRetrying;
            break;
        case eWifiAppStsWifiReady:
            sub = eRcStsSubConnected;
            break;
        case eWifiAppStsWifiRetrying:
            sub = eRcStsSubRetrying;
            break;
        case eWifiAppStsWifiFailed:
            sub = eRcStsSubFailed;
            break;
        case eWifiAppStsWifiDisconnected:
            sub = eRcStsSubDisconnected;
            break;
        case eWifiAppStsTcpListening:
            sub = eRcStsSubConnected;
            break;
        case eWifiAppStsTcpConnected:
            sub = eRcStsSubConnected;
            break;
        case eWifiAppStsTcpAuthOk:
            sub = eRcStsSubAuthOk;
            break;
        case eWifiAppStsTcpAuthFail:
            sub = eRcStsSubAuthFail;
            break;
        case eWifiAppStsTcpRetrying:
            sub = eRcStsSubRetrying;
            break;
        case eWifiAppStsTcpFailed:
            sub = eRcStsSubFailed;
            break;
        case eWifiAppStsTcpDisconnected:
            sub = eRcStsSubDisconnected;
            break;
        case eWifiAppStsReady:
            sub = eRcStsSubReady;
            break;
        case eWifiAppStsStopped:
            sub = eRcStsSubDisconnected;
            break;
        case eWifiAppStsSleeping:
            sub = eRcStsSubSleeping;
            break;
        default:
            sub = eRcStsSubDisconnected;
            break;
    }

    (void)uart_app_send_status(&gUartApp, eRcIntfWifi, sub);
}

/* =========================================================================
 * on_wifi_rx
 *
 * Called by wifi_app task when data arrives from phone over TCP.
 * Forwards raw bytes to Host as a WiFi DATA frame.
 * ========================================================================= */
static void on_wifi_rx(const uint8_t* data, uint16_t len, void* user)
{
    (void)user;

    if((data == NULL) || (len == 0U))
    {
        return;
    }

    /* Try to decode as rc_protocol frame from phone */
    rc_frame_t frame;

    if(rc_frame_decode(data, len, &frame) && (frame.type == eRcTypeData))
    {
        /* Forward typed DATA frame to Host (NAV, NOTIF, RAW) */
        (void)uart_app_send_data(&gUartApp, eRcIntfWifi, frame.subcode,
                                 frame.payload, frame.payloadLen);
    }
    else
    {
        /* Not rc_protocol framed - send as RAW data */
        (void)uart_app_send_data(&gUartApp, eRcIntfWifi, (uint8_t)eRcDataSubRaw, data, len);
    }
}

/* =========================================================================
 * ridecast_deinit  (called on shutdown / OTA / fault)
 * ========================================================================= */
bsp_err_sts_t ridecast_deinit(void)
{
    (void)wifi_app_deinit(&gWifiApp);
    (void)uart_app_deinit(&gUartApp);
    return BSP_ERR_STS_OK;
}

/* =========================================================================
 * ridecast_init
 *
 * Called once from main() / app_main() after RTOS scheduler start.
 * ========================================================================= */
bsp_err_sts_t ridecast_init(void)
{
    bsp_err_sts_t sts;

    /* ------------------------------------------------------------------
     * 1. uart_app  - UART to Host
     * ------------------------------------------------------------------ */
    uart_app_cfg_t uCfg;
    (void)memset(&uCfg, 0, sizeof(uCfg));

    /* Hardware */
    (void)memcpy(&uCfg.uartHandle, &g_appUartHandleCfg[APP_UART_OWNER_HOST_COMM],
                 sizeof(bspUartHandle_t));

    /* Interface dispatch */
    uCfg.onWifiFrame = uart_to_wifi_host_frame;
    uCfg.onBleFrame  = on_ble_host_frame;
    uCfg.onBtFrame   = on_bt_host_frame;
    uCfg.wifiUser    = NULL;
    uCfg.bleUser     = NULL;
    uCfg.btUser      = NULL;

    sts = uart_app_init(&gUartApp, &uCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        return sts;
    }

    /* ------------------------------------------------------------------
     * 2. wifi_app  - WiFi AP + TCP + Auth
     * ------------------------------------------------------------------ */
    wifi_app_cfg_t wCfg;
    (void)memset(&wCfg, 0, sizeof(wCfg));

    /* Auth token - phone must send "TOKEN rc_secret_v1\n" to authenticate.
     * Set to empty string "" to disable auth.                            */
    (void)strncpy(wCfg.authToken, "rc_secret_v1", sizeof(wCfg.authToken) - 1U);

    wCfg.tcpPort       = appCfg_WIFI_TCP_PORT;
    wCfg.authTimeoutMs = WIFI_APP_AUTH_TIMEOUT_MS;
    wCfg.maxRetry      = WIFI_APP_MAX_RETRY;
    wCfg.retryBaseMs   = WIFI_APP_RETRY_BASE_MS;
    wCfg.retryMaxMs    = WIFI_APP_RETRY_MAX_MS;
    wCfg.onStatus      = on_wifi_status;
    wCfg.onRx          = on_wifi_rx;
    wCfg.cbUser        = BSP_NULL;

    sts = wifi_app_init(&gWifiApp, &wCfg);
    if(sts != BSP_ERR_STS_OK)
    {
        (void)uart_app_deinit(&gUartApp);
        return sts;
    }

    /* ------------------------------------------------------------------
     * 3. Notify Host: RideCast booted and waiting for commands
     * ------------------------------------------------------------------ */
    (void)uart_app_send_status(&gUartApp, eRcIntfWifi, eRcStsSubReady);

    // (void)test_host_uart_init(&gUartApp);

    return BSP_ERR_STS_OK;
}
