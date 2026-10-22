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

#include <stdbool.h>
#include <stdint.h>

#include "bsp_config.h"
#include "bsp_console_transport.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus*/

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
        /*
         * Logger transport. Required even if logging starts disabled, because
         * bsp_log_enable(true) may be called later at runtime.
         *
         * Can be UART1, UART2, USB CDC, RTT, etc. It may be the same transport
         * as cliTransport.
         */
        const bspConsoleTransport_t* logTransport;

#if (BSP_CLI_ENABLE == 1U)
        /*
         * CLI transport when cliEnabled is true.
         *
         * May be the same transport as logTransport, or a different
         * UART/USB/RTT transport.
         */
        const bspConsoleTransport_t* cliTransport;
#endif /* #if (BSP_CLI_ENABLE == 1U) */

    } bspConsoleConfig_t;

    /**
     * @brief Initialize logger and optional CLI from one configuration.
     *
     * Logger is always initialized so it can be enabled later at runtime.
     * CLI is initialized only when both BSP_CLI_ENABLE and cliEnabled are true.
     */
    bsp_err_sts_t bsp_console_init(const bspConsoleConfig_t* config);

    /**
     * @brief Deinitialize BSP console services.
     *
     * The CLI is stopped first because it may still receive transport input.
     * Logging is stopped afterwards.
     *
     * The function supports all valid configurations:
     *
     * - Logger only
     * - Logger plus CLI on the same transport
     * - Logger and CLI on separate UART/USB/RTT transports
     * - Compile-time-disabled CLI
     *
     * @return PLT_ERR_STS_OK on successful shutdown.
     * @return PLT_ERR_STS_NOT_INIT if bsp_console_init() was not successful.
     * @return CLI deinitialization failure, if one occurs.
     * @return Logger deinitialization status otherwise.
     */
    bsp_err_sts_t bsp_console_deinit(void);

    /**
     * @brief Process deferred CLI and logger work.
     *
     * Call this function repeatedly from one dedicated console task or from the
     * bare-metal main loop.
     *
     * This function must not be called from ISR context.
     *
     * Processing order:
     *
     * 1. If runtime CLI is enabled, process received CLI bytes.
     *    The transport RX callback only stores bytes in the CLI RX ring buffer.
     *    Command parsing and command handlers run here in normal task context.
     *
     * 2. Process queued ISR log records.
     *    bsp_log_isr() does not use printf, mutexes, or transport writes in
     * ISR. It only queues a small log record. bsp_log_process() formats and
     * sends that record here in normal task context.
     *
     * @note When BSP_CLI_ENABLE is 0U, all CLI processing code is removed at
     *       compile time.
     */
    void bsp_console_process(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* BSP_CONSOLE_H */