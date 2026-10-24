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

    if(ridecast_init() != RET_ERR_STS_OK)
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
 * Application RTOS Object Initialization
 * -------------------------------------------------------------------------
 * Creates all application-owned RTOS objects from the application
 * configuration maps and stores the generated runtime handles in the
 * corresponding map entries.
 *
 * Configuration:
 *   g_appThreadCfg[]
 *   g_appQueueCfg[]
 *   g_appMutexCfg[]
 *   g_appSemaphoreCfg[]
 *   g_appEventFlagsCfg[]
 *
 * Runtime handle:
 *   Each object's handle member is populated after successful creation.
 * ========================================================================= */
static void vInitAllAppOsDependentObjects(void)
{
    uint32_t index;


    /* ---------------------------------------------------------------------
     * Create Application Queues
     * ------------------------------------------------------------------ */
    for(index = 0U; index < APP_Q_OWNER_MAX; index++)
    {
        g_appQueueCfg[index].handle =
        osMessageQueueNew(g_appQueueCfg[index].msg_count, g_appQueueCfg[index].msg_size_bytes,
                          &g_appQueueCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create Application Mutexes
     * ------------------------------------------------------------------ */
    for(index = 0U; index < APP_MUTEX_OWNER_MAX; index++)
    {
        g_appMutexCfg[index].handle = osMutexNew(&g_appMutexCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create Application Semaphores
     * ------------------------------------------------------------------ */
    for(index = 0U; index < APP_SEMAPHORE_OWNER_MAX; index++)
    {
        g_appSemaphoreCfg[index].handle =
        osSemaphoreNew(g_appSemaphoreCfg[index].max_count,
                       g_appSemaphoreCfg[index].initial_count,
                       &g_appSemaphoreCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create Application Event Flags
     * ------------------------------------------------------------------ */
    for(index = 0U; index < APP_EVENT_FLAGS_OWNER_MAX; index++)
    {
        g_appEventFlagsCfg[index].handle =
        osEventFlagsNew(&g_appEventFlagsCfg[index].attr);
    }
}

/* =========================================================================
 * BSP RTOS Object Initialization
 * -------------------------------------------------------------------------
 * Member usage:
 *
 * g_bspThreadCfg[]
 *   - attr   : CMSIS thread attribute configuration
 *   - handle : Runtime thread handle
 *
 * g_bspQueueCfg[]
 *   - msg_count      : Maximum number of messages
 *   - msg_size_bytes : Size of each message
 *   - attr           : CMSIS queue attribute configuration
 *   - handle         : Runtime queue handle
 *
 * g_bspMutexCfg[]
 *   - attr   : CMSIS mutex attribute configuration
 *   - handle : Runtime mutex handle
 *
 * g_bspSemaphoreCfg[]
 *   - max_count     : Maximum semaphore count
 *   - initial_count : Initial semaphore count
 *   - attr           : CMSIS semaphore attribute configuration
 *   - handle         : Runtime semaphore handle
 *
 * g_bspEventFlagsCfg[]
 *   - flags  : Initial event flag value
 *   - attr   : CMSIS event flag attribute configuration
 *   - handle : Runtime event flag handle
 *
 * Thread handles are created separately because os_thread_map_t does not
 * contain a task/thread function pointer.
 * ========================================================================= */

static void vInitAllBspOsDepndentObjects(void)
{
    uint32_t index;


    /* ---------------------------------------------------------------------
     * Create BSP Queues
     * ------------------------------------------------------------------ */

    for(index = 0U; index < BSP_QUEUE_OWNER_MAX; index++)
    {
        g_bspQueueCfg[index].handle =
        osMessageQueueNew(g_bspQueueCfg[index].msg_count, g_bspQueueCfg[index].msg_size_bytes,
                          &g_bspQueueCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create BSP Mutexes
     * ------------------------------------------------------------------ */

    for(index = 0U; index < BSP_MUTEX_OWNER_MAX; index++)
    {
        g_bspMutexCfg[index].handle = osMutexNew(&g_bspMutexCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create BSP Semaphores
     * ------------------------------------------------------------------ */

    for(index = 0U; index < BSP_SEMAPHORE_OWNER_MAX; index++)
    {
        g_bspSemaphoreCfg[index].handle =
        osSemaphoreNew(g_bspSemaphoreCfg[index].max_count,
                       g_bspSemaphoreCfg[index].initial_count,
                       &g_bspSemaphoreCfg[index].attr);
    }


    /* ---------------------------------------------------------------------
     * Create BSP Event Flags
     * ------------------------------------------------------------------ */

    for(index = 0U; index < BSP_EVENT_FLAGS_OWNER_MAX; index++)
    {
        g_bspEventFlagsCfg[index].handle =
        osEventFlagsNew(&g_bspEventFlagsCfg[index].attr);
    }
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

    vInitAllBspOsDepndentObjects();

    vInitAllAppOsDependentObjects();

    (void)osThreadNew(startup_task, NULL,
                      &g_appThreadCfg[APP_THREAD_OWNER_STARTUP].attr);

    osKernelStart(); /* Never returns */

    return;
}