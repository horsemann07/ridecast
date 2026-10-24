
/* -------------------------------------------------------------------------- */
/* BSP and SDK Validation Entry Point                                         */
/* -------------------------------------------------------------------------- */

#include <stddef.h>

#include "esp_log.h"
#include "cmsis_os2.h"
#include "bsp_uart.h"

static const char* TAG = "BSP_SDK_CHECK";

/* -------------------------------------------------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "BSP and SDK validation firmware started");

    // /*
    //  * Verify BSP UART API linkage through an invalid-parameter path.
    //  * No UART hardware initialization is attempted.
    //  */
    // const ret_err_sts_t result = bspUartInit(NULL);

    // if(result == RET_ERR_STS_INVALID_PARAM)
    // {
    //     ESP_LOGI(TAG, "BSP UART API linkage successful");
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "Unexpected BSP UART API result");
    // }

    /* Verify that the CMSIS-RTOS2 kernel interface is linked. */
    const osKernelState_t kernel_state = osKernelGetState();

    ESP_LOGI(TAG, "CMSIS-RTOS2 kernel state: %d", (int)kernel_state);
}

/* -------------------------------------------------------------------------- */
