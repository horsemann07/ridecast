/**
 * @file bsp_console.h
 * @brief BSP console common initialization interface.
 *
 * This module provides a common initialization entry point for the
 * BSP logger and BSP CLI.
 *
 * The logger and CLI remain independent modules:
 *
 *     bsp_log.h / bsp_log.c
 *     bsp_cli.h / bsp_cli.c
 *
 * This module only connects each module to its configured transport.
 *
 * Example:
 *
 *     Logger -> UART
 *     CLI    -> USB CDC
 *
 * or:
 *
 *     Logger -> USB CDC
 *     CLI    -> UART
 */

#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H


/*=============================================================================
 * SYSTEM INCLUDES
 *============================================================================*/

#include <stdbool.h>
#include <stdint.h>


/*=============================================================================
 * PROJECT INCLUDES
 *============================================================================*/

#include "bsp_err.h"
#include "bsp_log.h"

#if (BSP_CLI_ENABLE  == 1)
#include "bsp_cli.h"
#endif /* BSP_CLI_ENABLE */

#include "bsp_console_transport.h"


/*=============================================================================
 * CONSOLE CONFIGURATION
 *============================================================================*/

/**
 * @brief BSP console initialization configuration.
 *
 * Logger and CLI can use completely different transports.
 *
 * Example:
 *
 *     logTransport = UART
 *     cliTransport = USB CDC
 */
typedef struct
{
    /**
     * @brief Transport used by BSP logger.
     *
     * Mandatory.
     */
    const bspConsoleTransport_t* logTransport;


#if (BSP_CLI_ENABLE == 1U)

    /**
     * @brief Transport used by BSP CLI.
     *
     * Mandatory when CLI is enabled.
     */
    const bspConsoleTransport_t* cliTransport;

#endif

} bspConsoleConfig_t;

/*=============================================================================
 * PUBLIC API
 *============================================================================*/

/**
 * @brief Initialize BSP console components.
 *
 * This function initializes:
 *
 *     1. BSP logger
 *     2. BSP CLI, when BSP_CLI_ENABLE == 1U
 *
 * The logger and CLI remain independent modules.
 *
 * This function only supplies the appropriate transport to each module.
 *
 * @param[in] config
 *        Console configuration.
 *
 * @return BSP_ERR_STS_OK
 *         Console components initialized successfully.
 *
 * @return BSP_ERR_STS_INVALID_PARAM
 *         Invalid configuration.
 *
 * @return BSP_ERR_STS_ALREADY_INIT
 *         Console already initialized.
 *
 * @return BSP_ERR_STS_FAIL
 *         Component initialization failed.
 */
bsp_err_sts_t bsp_console_init(
    const bspConsoleConfig_t* config);


/**
 * @brief Deinitialize BSP console components.
 *
 * Deinitializes CLI first, when enabled, followed by logger.
 *
 * @return BSP_ERR_STS_OK
 *         Console components deinitialized successfully.
 *
 * @return BSP_ERR_STS_NOT_INIT
 *         Console was not initialized.
 */
bsp_err_sts_t bsp_console_deinit(void);


#endif /* BSP_CONSOLE_H */