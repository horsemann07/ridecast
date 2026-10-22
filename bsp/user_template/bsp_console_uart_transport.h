#ifndef BSP_CONSOLE_UART_TRANSPORT_H
#define BSP_CONSOLE_UART_TRANSPORT_H

#include "bsp_config.h"
#include "bsp_console_transport.h"
#include "bsp_uart.h"
#include "bsp_err_sts.h"

/**
 * @brief Initialize one UART console transport from BSP UART mapping.
 *
 * @param[in] owner BSP UART mapping index, for example
 *                  BSP_UART_OWNER_CONSOLE.
 *
 * @return PLT_ERR_STS_OK on success.
 */
bsp_err_sts_t bsp_console_uart_transport_init(bspUartOwner_t owner);

/**
 * @brief Deinitialize one UART console transport.
 *
 * @param[in] owner BSP UART mapping index.
 */
bsp_err_sts_t bsp_console_uart_transport_deinit(bspUartOwner_t owner);

/**
 * @brief Get the console transport for a mapped UART owner.
 *
 * @param[in] owner BSP UART mapping index.
 *
 * @return Initialized transport, or NULL when unavailable.
 */
const bspConsoleTransport_t*
bsp_console_uart_transport_get(bspUartOwner_t owner);

#endif /* BSP_CONSOLE_UART_TRANSPORT_H */