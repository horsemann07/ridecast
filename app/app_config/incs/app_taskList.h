
/**
 * ============================================================================
 * @file    app_tasklist.h
 * @brief   RideCast application task entry-point declarations.
 *
 * @details
 * Declares the CMSIS-RTOS2 task entry functions used by the RideCast
 * application.
 *
 * Task attributes, owner identifiers and runtime handles are maintained
 * separately in rc_os_map.h and rc_config.c.
 *
 * Usage:
 *   - Include this file in rc_config.c.
 *   - Register the required task entry function in g_appThreadCfg[].
 *   - Create tasks through the existing CMSIS-RTOS2 startup mechanism.
 *
 * @note
 *   Every declared task must have exactly one corresponding definition
 *   in its application source file.
 *
 * @warning
 *   All task functions must use the signature void (*)(void *).
 *   Declaring a task here does not create or start that task.
 *
 *   Wireless task creation must not automatically enable wireless
 *   services. RideCast starts wireless services only on cluster command.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Added application task entry-point declarations.
 * ============================================================================
 */

#ifndef APP_TASKLIST_H
#define APP_TASKLIST_H


#include "uart_app.h"
#include "wifi_app.h"
#include "rc_ble.h"
#include "rc_ble_classic.h"

#ifdef __cplusplus
extern "C"
{
#endif


    /* =========================================================================
     * APPLICATION STARTUP TASK
     * ========================================================================= */

    /**
     * @brief Initialize RideCast application services.
     *
     * @param[in] arg Optional task argument.
     */
    void vAppStartupTask(void* arg);


    /* =========================================================================
     * UART APPLICATION TASKS
     * ========================================================================= */

    /**
     * @brief Process UART transmission requests to the vehicle cluster.
     *
     * @param[in] arg Optional task argument.
     */
    void vAppUartTxTask(void* arg);


    /**
     * @brief Process incoming UART data from the vehicle cluster.
     *
     * @param[in] arg Optional task argument.
     */
    void vAppUartRxTask(void* arg);


    /* =========================================================================
     * MANAGER WORKER
     * ========================================================================= */

    /**
     * @brief Serialized connection-manager worker.
     *
     * @note
     *   Registered in app_tasklist.h and rc_os_map.c.
     *   Normally created by conn_manager_init().
     */
    void vConnManagerTask(void* arg);

    /* =========================================================================
     * WIRELESS APPLICATION TASKS
     * ========================================================================= */

    /**
     * @brief Execute RideCast Wi-Fi application processing.
     *
     * @param[in] arg Optional task argument.
     */
    void vRcWifiTask(void* arg);


    /**
     * @brief Execute RideCast BLE application processing.
     *
     * @param[in] arg Optional task argument.
     */
    void vRcBleTask(void* arg);


    /**
     * @brief Execute RideCast Bluetooth Classic application processing.
     *
     * @param[in] arg Optional task argument.
     */
    void vRcBtClassicTask(void* arg);


#ifdef __cplusplus
}
#endif

#endif /* APP_TASKLIST_H */
