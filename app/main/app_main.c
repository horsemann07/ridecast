/**
 * @file    main.c
 * @brief   FreeRTOS / CMSIS-RTOS2 application entry.
 *          Only job: start scheduler then call ridecast_init().
 */

#include "cmsis_os2.h"
#include "ridecast_main.h"
#include "app_config.h"
#include "app_logging.h"

/* =========================================================================
 * Startup task  - runs once after scheduler starts
 * ========================================================================= */
static void startup_task(void* arg)
{
    (void)arg;

    if(ridecast_init() != BSP_ERR_STS_OK)
    {
        /* Fail-stop: do not continue with partially initialized system */
        ALOGE("ridecast_init failed - halting");
        for(;;)
        {
            osDelay(1000U);
        }
    }

    osThreadExit();
}

/* =========================================================================
 * app_main()
 *
 * Entry point called by ESP-IDF after boot and peripherals init.
 * Initialize RTOS kernel and start scheduler.
 * ========================================================================= */
void app_main(void)
{
    /* Board / clock / peripheral init (BSP-specific) */
    /* bsp_board_init(); */

    osKernelInitialize();

    (void)osThreadNew(startup_task, NULL,
                      &g_appThreadCfg[APP_THREAD_OWNER_STARTUP].attr);

    osKernelStart(); /* Never returns */

    return;
}