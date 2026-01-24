/**
 * @file app_config.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-30
 *
 * @copyright Copyright (c) 2025
 *
 */


#ifndef APP_BSP_CONFIG_H
#define APP_BSP_CONFIG_H


#ifdef _cplusplus
extern "C"
{
#endif

#include "bsp_uart.h"
/* =========================
 * Application BSP Configuration
 * ========================= */
#define PIN_UNUSED                     ((uint8_t)0)

#define ENABLE                       ((uint8_t)1)
#define DISABLE                      ((uint8_t)0)   

/* Owner identifiers                 Index               */
#define BSP_UART_OWNER_CONSOLE      ((bspUartOwner_t)0u)                        // 0
#define BSP_UART_OWNER_GNSS         ((bspUartOwner_t)BSP_UART_OWNER_CONSOLE + 1)   // 1
#define BSP_UART_OWNER_MAX          ((bspUartOwner_t)BSP_UART_OWNER_GNSS + 1)   // 2


#define ESP_BOARD_LOGGING

    extern const bspUartHandle_t g_bspUartHandles[];


#ifdef __cplusplus
}
#endif

#endif // APP_BSP_CONFIG_H
