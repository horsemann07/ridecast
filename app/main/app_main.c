#include "cmsis_os2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp_err_sts.h"

#include "wifi_sta_app.h"

void app_main(void)
{
    // Initialize CMSIS-RTOS v2 kernel wrapper
    osKernelInitialize();

    // Start WiFi STA application:
    //   1. Scan WiFi networks  → print table on UART
    //   2. Connect to AP       → show status (IP / GW / RSSI) on UART
    //   3. Connect TCP socket  → receive data from server, forward to UART
    wifiStaAppStart();

    // Start scheduler
    osKernelStart();
}
