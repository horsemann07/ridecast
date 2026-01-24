#include "sys_config.h"
#include "nal.h"
#include "bsp_wifi.h"


#include "cmsis_os2.h"
#include "bsp_log.h"

#include "esp_log.h"

static volatile uint8_t g_wifi_ready = 0;

#ifndef __FILENAME__
    #define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#endif

#define LOGI(fmt, ...) ESP_LOGI(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ESP_LOGW(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) ESP_LOGE(__FILENAME__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) ESP_LOGD(__FILENAME__, fmt, ##__VA_ARGS__)


#define TCP_ECHO_PORT  5000
#define TCP_BUF_SIZE   256



static void tcpEchoServerTask(void* arg)
{
    (void)arg;

    nalHandle_t handle;
    uint8_t rx_buf[TCP_BUF_SIZE];
    size_t rx_len;
    uint32_t counter = 0;

    /* Wait until Wi-Fi is ready */
    while(!g_wifi_ready)
    {
        osDelay(500);
    }

    LOGI("Starting TCP echo server");

    nalNetworkInit(&handle);

    /* Bind/listen is assumed inside platform NAL or simplified demo */
    bsp_err_sts_t rc =
    nalNetworkConnect(&handle, "0.0.0.0", TCP_ECHO_PORT, NAL_SCHEME_PLAIN, 0);
    if(rc != BSP_ERR_STS_OK)
    {
        LOGE("TCP server start failed");
        goto cleanup;
    }

    LOGI("TCP server started");

    while(1)
    {
        rc = nalNetworkRecvSync(&handle, rx_buf, sizeof(rx_buf) - 1, &rx_len, 0);
        if(rc != BSP_ERR_STS_OK || rx_len == 0)
        {
            LOGW("Client disconnected");
            break;
        }

        rx_buf[rx_len] = '\0';
        LOGI("RX: %s", rx_buf);

        /* Append counter */
        char tx_buf[300];
        int len =
        snprintf(tx_buf, sizeof(tx_buf), "[%lu] %s", (unsigned long)counter++, rx_buf);

        size_t sent;
        nalNetworkSendSync(&handle, tx_buf, len, &sent, 0);
    }

cleanup:
    nalNetworkDeinit(&handle);
    osThreadExit();
}


void appStart(void)
{
    if(appWifiInit() != BSP_ERR_STS_OK)
    {
        LOGE("Wi-Fi init failed");
        return;
    }

    const osThreadAttr_t tcp_task_attr = { .name       = "tcp_echo_srv",
                                           .priority   = osPriorityNormal,
                                           .stack_size = 4096 };

    osThreadNew(tcpEchoServerTask, NULL, &tcp_task_attr);
}
