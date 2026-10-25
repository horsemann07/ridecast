
/**
 * ============================================================================
 * @file    rc_peripheral_map.h
 * @brief   RideCast application peripheral owner definitions.
 *
 * @details
 * Defines application-owned hardware peripheral identifiers and
 * declares their configuration handles.
 *
 * Peripheral hardware initialization remains the responsibility of the
 * existing BSP driver and application startup code.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Separated peripheral configuration from RTOS object maps.
 * ============================================================================
 */

#ifndef RC_PERIPHERAL_MAP_H
#define RC_PERIPHERAL_MAP_H

#include "bsp_uart.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * UART OWNER CONFIGURATION
     * ========================================================================= */

    typedef enum
    {
        APP_UART_OWNER_CONSOLE = 0,
        APP_UART_OWNER_HOST_COMM,

        APP_UART_OWNER_MAX

    } app_uart_owner_t;


    /* =========================================================================
     * UART PERIPHERAL CONFIGURATION
     * ========================================================================= */

    extern bspUartHandle_t g_appUartHandleCfg[APP_UART_OWNER_MAX];


#ifdef __cplusplus
}
#endif

#endif /* RC_PERIPHERAL_MAP_H */
