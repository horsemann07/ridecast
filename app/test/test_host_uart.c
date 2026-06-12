#include "uart_app.h"
#include "rc_protocol.h"

#include <string.h>
#include <stdint.h>

#include "app_logging.h"

/* -------------------------------------------------------------------------
 * Dummy Host UART Test
 *
 * Sends WiFi START CMD frame using rc_encode_cmd_start_wifi()
 *
 * Frame format (from rc_protocol.h):
 *   [hdr(1)] [len_hi(1)] [len_lo(1)] [SSID(32)] [PASS(64)]
 *   total = 99 bytes
 *
 * Note:
 *   Requires UART loopback or external TX/RX wiring.
 * ------------------------------------------------------------------------- */

#define TEST_HOST_UART_START_DELAY_MS (1000U)
#define TEST_HOST_UART_WIFI_SSID      "RideCastPhone"
#define TEST_HOST_UART_WIFI_PASSWORD  "12345678"

/* Frame size: hdr(1) + len(2) + SSID(32) + PASS(64) = 99 bytes */
#define TEST_HOST_UART_FRAME_LEN (1U + 2U + RC_WIFI_START_PARAMS)

typedef struct
{
    uart_app_t* app;
    osThreadId_t taskId;
    uint8_t running;
} test_host_uart_t;

static test_host_uart_t gTestHostUart;

static bsp_err_sts_t
test_host_uart_send_frame(test_host_uart_t* host, const uint8_t* frame, uint16_t len);
static void test_host_uart_task(void* arg);

/* -------------------------------------------------------------------------
 * Send frame over UART synchronously
 * ------------------------------------------------------------------------- */
static bsp_err_sts_t
test_host_uart_send_frame(test_host_uart_t* host, const uint8_t* frame, uint16_t len)
{
    bsp_err_sts_t sts;

    if((host == NULL) || (host->app == NULL) || (frame == NULL) || (len == 0U))
    {
        ALOGE("send_frame: invalid param");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    sts = bspUartSendSync(&host->app->cfg.uartHandle, frame, (size_t)len, 500U);

    if(sts != BSP_ERR_STS_OK)
    {
        ALOGE("send_frame: uart send failed len=%" PRIu16 " sts=%d", len, (int)sts);
    }

    return sts;
}
/**
 * @brief  Encode CMD_START (WiFi) frame: header + length + SSID + PASS
 * @param  buf   Output buffer (>= 1 + 2 + 96 bytes)
 * @param  intf  Interface (eRcIntfWifi)
 * @param  ssid  SSID string (max 32 bytes)
 * @param  pass  Password string (max 64 bytes)
 * @return Total bytes written
 */
static uint16_t
rc_encode_cmd_start_wifi(uint8_t* buf, const char* ssid, const char* pass)
{
uint8_t params[RC_WIFI_START_PARAMS];
uint16_t i;

/* Zero fill then copy strings safely */
for(i = 0U; i < RC_WIFI_START_PARAMS; i++)
{
    params[i] = 0U;
}

for(i = 0U; (i < RC_WIFI_SSID_LEN - 1U) && (ssid[i] != '\0'); i++)
{
    params[i] = (uint8_t)ssid[i];
}

for(i = 0U; (i < RC_WIFI_PASS_LEN - 1U) && (pass[i] != '\0'); i++)
{
    params[RC_WIFI_SSID_LEN + i] = (uint8_t)pass[i];
}

buf[0U] = rc_make_header(eRcIntfWifi, eRcTypeCmd, (uint8_t)eRcCmdSubStart);
buf[1U] = (uint8_t)((RC_WIFI_START_PARAMS >> 8U) & 0xFFU);
buf[2U] = (uint8_t)(RC_WIFI_START_PARAMS & 0xFFU);

for(i = 0U; i < RC_WIFI_START_PARAMS; i++)
{
    buf[3U + i] = params[i];
}

return (uint16_t)(3U + RC_WIFI_START_PARAMS);
}

/* -------------------------------------------------------------------------
 * Test task
 *
 * 1. Wait for device to settle
 * 2. Encode WiFi START frame via rc_encode_cmd_start_wifi()
 * 3. Send frame
 * ------------------------------------------------------------------------- */
static void test_host_uart_task(void* arg)
{
    test_host_uart_t* host = (test_host_uart_t*)arg;
    uint8_t frame[TEST_HOST_UART_FRAME_LEN];
    uint16_t frameLen;

    if((host == NULL) || (host->app == NULL))
    {
        ALOGE("task: invalid host/app");
        osThreadExit();
    }

    ALOGD("task: started");

    osDelay(TEST_HOST_UART_START_DELAY_MS);

    if(host->running == 0U)
    {
        ALOGW("task: stopped before send");
        osThreadExit();
    }

    /*
     * Encode WiFi START frame:
     *
     *   byte[0]      = header
     *                    bits[1:0] = INTF  = eRcIntfWifi  (0x01)
     *                    bits[3:2] = TYPE  = eRcTypeCmd   (0x00)
     *                    bits[7:4] = SUB   = eRcCmdSubStart (0x00)
     *   byte[1]      = payloadLen >> 8  (0x00)
     *   byte[2]      = payloadLen & 0xFF (0x60 = 96)
     *   byte[3..34]  = SSID (32 bytes, zero padded)
     *   byte[35..98] = PASS (64 bytes, zero padded)
     */
    frameLen = rc_encode_cmd_start_wifi(frame, TEST_HOST_UART_WIFI_SSID,
                                        TEST_HOST_UART_WIFI_PASSWORD);
    if(frameLen == 0U)
    {
        ALOGE("task: encode failed");
        host->running = 0U;
        osThreadExit();
    }

    ALOGD("task: encoded frameLen=%" PRIu16, frameLen);


    /* send to UART Queue */
    if(osMessageQueuePut(host->app->txQ, frame, 0U, 0U) != osOK)
    {
        ALOGE("task: send failed ssid=%s", TEST_HOST_UART_WIFI_SSID);
    }
    else
    {
        ALOGD("task: WiFi START sent ssid=%s", TEST_HOST_UART_WIFI_SSID);
    }

    host->running = 0U;
    osThreadExit();
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
bsp_err_sts_t test_host_uart_deinit(void)
{
    gTestHostUart.running = 0U;
    osDelay(100U);
    gTestHostUart.taskId = NULL;
    gTestHostUart.app    = NULL;
    return BSP_ERR_STS_OK;
}

bsp_err_sts_t test_host_uart_init(uart_app_t* app)
{
    if(app == NULL)
    {
        ALOGE("init: invalid app");
        return BSP_ERR_STS_INVALID_PARAM;
    }

    (void)memset(&gTestHostUart, 0, sizeof(gTestHostUart));
    gTestHostUart.app     = app;
    gTestHostUart.running = 1U;
    gTestHostUart.taskId = osThreadNew(test_host_uart_task, &gTestHostUart, NULL);

    if(gTestHostUart.taskId == NULL)
    {
        ALOGE("init: task create failed");
        return BSP_ERR_STS_FAIL;
    }

    return BSP_ERR_STS_OK;
}