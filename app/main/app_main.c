/**
 * ============================================================================
 * @file    app_main.c
 * @brief   RideCast CMSIS-RTOS2 application entry point.
 *
 * @details
 * Creates centrally owned BSP/application RTOS objects and starts a single
 * application startup task. The startup task initializes each configured
 * module before creating its worker, respecting module dependencies.
 *
 * STARTUP FLOW:
 *
 *   app_main()
 *       |
 *       +-- Initialize CMSIS kernel (where required)
 *       +-- Create BSP RTOS objects
 *       +-- Create application RTOS objects
 *       +-- Create startup task from application thread map
 *       +-- Start kernel (where required)
 *               |
 *               v
 *       vAppStartupTask()
 *               |
 *               +-- Initialize/start BSP workers
 *               +-- Initialize/start connection manager
 *               +-- Initialize/start Wi-Fi, BLE, BT Classic workers
 *               +-- Initialize/start UART TX and RX workers
 *               +-- Initialize/start other configured workers
 *               +-- Report device readiness
 *
 * THREAD LIFECYCLE:
 *
 *   Central launcher: init(void) -> osThreadNew(wrapper, descriptor)
 *   Thread wrapper:  thread(descriptor->arg) -> deinit(void) -> return
 *
 * @warning
 *   Module init() must not turn on Wi-Fi, BLE or Bluetooth Classic.
 *   Wireless activation remains controlled by vehicle-cluster commands.
 *
 *   Module thread entry functions must return normally for the wrapper to
 *   call deinit(). Do not call osThreadExit() inside those functions.
 *
 *   All shared RTOS queues, mutexes, semaphores and flags are owned here;
 *   module deinit callbacks must not delete those shared objects.
 *
 *   ESP-IDF has already started FreeRTOS when app_main() executes.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Centralized module init/thread/deinit sequencing; fixed BSP
 *             thread map access, initialization order and startup duplication.
 * 2026-10-25  Fixed owner ranges, startup task and kernel handling.
 * 2026-05-23  Initial development - J.Raghav.
 * ============================================================================
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cmsis_os2.h"
#include "app_config.h"
#include "app_logging.h"
#include "ridecast_main.h"

/* -------------------------------------------------------------------------- */
/* Resource Failure Bitmap Macros                                             */
/* -------------------------------------------------------------------------- */
/* Number of bits in one bitmap word. */
#define APP_FAIL_BITS_PER_WORD (32U)

/* Calculate bitmap array size from the owner MAX value. */
#define APP_FAIL_BITMAP_SIZE(max) \
    (((max) + APP_FAIL_BITS_PER_WORD - 1U) / APP_FAIL_BITS_PER_WORD)

/* Set a resource failure bit. */
#define APP_FAIL_SET(bitmap, id)                         \
    (                                                    \
    (bitmap)[(uint32_t)(id) / APP_FAIL_BITS_PER_WORD] |= \
    (UINT32_C(1) << ((uint32_t)(id) % APP_FAIL_BITS_PER_WORD)))

/* Clear a resource failure bit. */
#define APP_FAIL_CLEAR(bitmap, id)                       \
    (                                                    \
    (bitmap)[(uint32_t)(id) / APP_FAIL_BITS_PER_WORD] &= \
    ~(UINT32_C(1) << ((uint32_t)(id) % APP_FAIL_BITS_PER_WORD)))

/* Check whether a resource failure bit is set. */
#define APP_FAIL_IS_SET(bitmap, id)                      \
    (                                                    \
    ((bitmap)[(uint32_t)(id) / APP_FAIL_BITS_PER_WORD] & \
     (UINT32_C(1) << ((uint32_t)(id) % APP_FAIL_BITS_PER_WORD))) != 0U)


/* -------------------------------------------------------------------------- */
/* Application Resource Initialization Failure Tracking                       */
/* -------------------------------------------------------------------------- */

/* Track application thread initialization failures. */
static uint8_t s_appThreadFailed[APP_THREAD_OWNER_MAX] = { 0U };

/* Track application queue initialization failures. */
static uint8_t s_appQueueFailed[APP_Q_OWNER_MAX] = { 0U };

/* Track application mutex initialization failures. */
static uint8_t s_appMutexFailed[APP_MUTEX_OWNER_MAX] = { 0U };

/* Track application semaphore initialization failures. */
static uint8_t s_appSemaFailed[APP_SEMAPHORE_OWNER_MAX] = { 0U };

/* Track application semaphore initialization failures. */
static uint8_t s_appEventFailed[APP_EVENT_FLAGS_OWNER_MAX] = { 0U };

/* -------------------------------------------------------------------------- */
/**
 * @brief Report application availability to the vehicle cluster.
 *
 * @details
 * Called after all required application services have completed
 * their own initialization successfully.
 *
 * @note
 *   This function does not initialize UART or enable wireless.
 *
 * @return UART transmission request status.
 */
static ret_err_sts_t ridecast_report_ready(void);

/**
 * @brief Create every configured BSP queue, mutex, semaphore and event flag.
 */
static ret_err_sts_t vInitAllBspOsDependentObjects(void);

/**
 * @brief Create every configured application RTOS object.
 *
 * Application owner IDs start at their corresponding BSP OWNER_MAX.
 */
static ret_err_sts_t vInitAllAppOsDependentObjects(void);

/* =========================================================================
 * APPLICATION ENTRY POINT
 * ========================================================================= */

/**
 * @brief Create shared RTOS objects and launch the startup coordinator.
 */
void app_main(void)
{
#if !defined(CMAKE_ESPx_PLATFORM)
    osKernelState_t kernelState = osKernelGetState();

    if(kernelState == osKernelInactive)
    {
        if(osKernelInitialize() != osOK)
        {
            return;
        }
    }
    else if((kernelState != osKernelReady) && (kernelState != osKernelRunning))
    {
        return;
    }
#endif

    /* Initialize BSP RTOS objects */
    (void)vInitAllBspOsDependentObjects();

    /* Initialize application RTOS objects */
    (void)vInitAllAppOsDependentObjects();
    

    /* Inform the vehicle cluster, ridecast is ready */
    if(ridecast_report_ready() != RET_ERR_STS_OK)
    {
        ALOGE("RideCast startup failed; device not ready");
    }

#if !defined(CMAKE_ESPx_PLATFORM)
    if(osKernelGetState() == osKernelReady)
    {
        (void)osKernelStart();
    }
#endif
}

/**
 * @brief Report application availability to the vehicle cluster.
 *
 * @details
 * Called after all required application services have completed
 * their own initialization successfully.
 *
 * @note
 *   This function does not initialize UART or enable wireless.
 *
 * @return UART transmission request status.
 */
static ret_err_sts_t ridecast_report_ready(void)
{
    uint8_t buffer[RC_FRAME_MIN_LEN];
    uint16_t frameLen;

    frameLen = rc_encode_simple(buffer, (uint16_t)sizeof(buffer), (uint8_t)eRcTypeStatus,
                                RC_FLAG_NONE, (uint8_t)eRcMsgGetStatus,
                                (uint8_t)eRcTransportNone, 0U);

    if(frameLen == 0U)
    {
        ALOGE("Failed to encode device-ready frame");
        return RET_ERR_STS_FAIL;
    }

    return uart_app_send(buffer, frameLen);
}



/* =========================================================================
 * STATIC HELPER IMPLEMENTATIONS
 * ========================================================================= */

/**
 * @brief Create every configured BSP queue, mutex, semaphore and event flag.
 */
static ret_err_sts_t vInitAllBspOsDependentObjects(void)
{
    uint32_t index;
    ret_err_sts_t status = RET_ERR_STS_OK;

    for(index = 0U; index < BSP_QUEUE_OWNER_MAX; index++)
    {
        if((g_bspQueueCfg[index].msg_count == 0U) ||
           (g_bspQueueCfg[index].msg_size_bytes == 0U))
        {
            continue;
        }

        g_bspQueueCfg[index].handle =
        osMessageQueueNew(g_bspQueueCfg[index].msg_count, g_bspQueueCfg[index].msg_size_bytes,
                          &g_bspQueueCfg[index].attr);

        if(g_bspQueueCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appQueueFailed, index);
        }
    }

    for(index = 0U; index < BSP_MUTEX_OWNER_MAX; index++)
    {
        if(g_bspMutexCfg[index].attr.name == NULL)
        {
            continue;
        }

        g_bspMutexCfg[index].handle = osMutexNew(&g_bspMutexCfg[index].attr);

        if(g_bspMutexCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appMutexFailed, index);
        }
    }

    for(index = 0U; index < BSP_SEMAPHORE_OWNER_MAX; index++)
    {
        if(g_bspSemaphoreCfg[index].max_count == 0U)
        {
            continue;
        }

        g_bspSemaphoreCfg[index].handle =
        osSemaphoreNew(g_bspSemaphoreCfg[index].max_count,
                       g_bspSemaphoreCfg[index].initial_count,
                       &g_bspSemaphoreCfg[index].attr);

        if(g_bspSemaphoreCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appSemaFailed, index);
        }
    }

    for(index = 0U; index < BSP_EVENT_FLAGS_OWNER_MAX; index++)
    {
        if(g_bspEventFlagsCfg[index].attr.name == NULL)
        {
            continue;
        }

        g_bspEventFlagsCfg[index].handle =
        osEventFlagsNew(&g_bspEventFlagsCfg[index].attr);

        if(g_bspEventFlagsCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appEventFailed, index);
        }
    }

    /* BSP map is independent of the application map. */
    for(index = 0U; index < BSP_THREAD_OWNER_MAX; index++)
    {
        os_thread_map_t* current = &g_bspThreadCfg[index];

        if(current == NULL)
        {
            continue;
        }

        if(current->init != NULL)
        {
            status = current->init();
        }

        if(status == RET_ERR_STS_OK && current->thread != NULL)
        {
            current->handle =
            osThreadNew(current->thread, current->arg, &current->attr);
        }

        if(current->handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appThreadFailed, index);

            if(current->deinit != NULL)
            {
                current->deinit();
            }
        }
    }

    return RET_ERR_STS_OK;
}

/**
 * @brief Create every configured application RTOS object.
 *
 * Application owner IDs start at their corresponding BSP OWNER_MAX.
 */
static ret_err_sts_t vInitAllAppOsDependentObjects(void)
{
    uint32_t index;
    ret_err_sts_t status = RET_ERR_STS_OK;

    for(index = BSP_QUEUE_OWNER_MAX; index < APP_Q_OWNER_MAX; index++)
    {
        if((g_appQueueCfg[index].msg_count == 0U) ||
           (g_appQueueCfg[index].msg_size_bytes == 0U))
        {
            continue;
        }

        g_appQueueCfg[index].handle =
        osMessageQueueNew(g_appQueueCfg[index].msg_count, g_appQueueCfg[index].msg_size_bytes,
                          &g_appQueueCfg[index].attr);

        if(g_appQueueCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appQueueFailed, index);
        }
    }

    for(index = BSP_MUTEX_OWNER_MAX; index < APP_MUTEX_OWNER_MAX; index++)
    {
        if(g_appMutexCfg[index].attr.name == NULL)
        {
            continue;
        }

        g_appMutexCfg[index].handle = osMutexNew(&g_appMutexCfg[index].attr);

        if(g_appMutexCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appMutexFailed, index);
        }
    }

    for(index = BSP_SEMAPHORE_OWNER_MAX; index < APP_SEMAPHORE_OWNER_MAX; index++)
    {
        if(g_appSemaphoreCfg[index].max_count == 0U)
        {
            continue;
        }

        g_appSemaphoreCfg[index].handle =
        osSemaphoreNew(g_appSemaphoreCfg[index].max_count,
                       g_appSemaphoreCfg[index].initial_count,
                       &g_appSemaphoreCfg[index].attr);

        if(g_appSemaphoreCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appSemaFailed, index);
        }
    }

    for(index = BSP_EVENT_FLAGS_OWNER_MAX; index < APP_EVENT_FLAGS_OWNER_MAX; index++)
    {
        if(g_appEventFlagsCfg[index].attr.name == NULL)
        {
            continue;
        }

        g_appEventFlagsCfg[index].handle =
        osEventFlagsNew(&g_appEventFlagsCfg[index].attr);

        if(g_appEventFlagsCfg[index].handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appEventFailed, index);
        }
    }
    /* Application threads. */
    for(index = BSP_THREAD_OWNER_MAX; index < APP_THREAD_OWNER_MAX; index++)
    {
        os_thread_map_t* current = &g_appThreadCfg[index];
        if(current == NULL)
        {
            continue;
        }

        if(current->init != NULL)
        {
            status = current->init();
        }

        if(status == RET_ERR_STS_OK && current->thread != NULL)
        {
            current->handle =
            osThreadNew(current->thread, current->arg, &current->attr);
        }

        if(current->handle == NULL)
        {
            /* Set failure. */
            APP_FAIL_SET(s_appThreadFailed, index);

            if(current->deinit != NULL)
            {
                current->deinit();
            }
        }
    }

    return RET_ERR_STS_OK;
}
/* =========================================================================
 * FILE END
 * ========================================================================= */
