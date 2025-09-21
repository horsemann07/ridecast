#include "cmsis_os2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


static const char *TAG = "CMSIS_DEMO";

void MyCmsisTask(void *argument) {
    while (1) {
        ESP_LOGI(TAG, "Hello from CMSIS-RTOS2 task!");
        osDelay(1000);   // CMSIS API → calls vTaskDelay
    }
}

void app_main(void) {
    // Initialize CMSIS-RTOS v2 kernel wrapper
    osKernelInitialize();

    // Create one task using CMSIS API
    osThreadNew(MyCmsisTask, NULL, NULL);

    // Start scheduler (will call vTaskStartScheduler internally if not running)
    osKernelStart();
}
