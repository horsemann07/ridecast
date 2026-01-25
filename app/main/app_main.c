#include "cmsis_os2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp_err_sts.h"

extern void appStart(void);
extern void wifi_sta_app_start(void);
extern bsp_err_sts_t appConnectionInit(void);
// void MyCmsisTask(void* argument)
// {
//     while(1)
//     {
//         ESP_LOGI(TAG, "Hello from CMSIS-RTOS2 task!");
//         osDelay(1000); // CMSIS API → calls vTaskDelay
//     }
// }

extern void AppUartSyncDemoStart(void);
// extern void AppUartAsyncDemoStart(void);


void app_main(void)
{
    // Initialize CMSIS-RTOS v2 kernel wrapper
    osKernelInitialize();

    // Create one task using CMSIS API
    // osThreadNew(MyCmsisTask, NULL, NULL);


    // AppUartSyncDemoStart();
    // AppUartAsyncDemoStart();

    //appStart();
    // wifi_sta_app_start();
    appConnectionInit();

    // Start scheduler (will call vTaskStartScheduler internally if not running)
    osKernelStart();
}
